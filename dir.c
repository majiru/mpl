#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

Song*
file2song(char *path, int needpic)
{
	char *dot;
	Song *s;
	int fd;

	dot = strrchr(path, '.');
	if(dot == nil)
		return nil;
	dot+=1;

	s = emalloc(sizeof(Song));
	if(strcmp(dot, "flac") == 0){
		s->type = FLAC;
		goto done;
	}
	if(strcmp(dot, "mp3") == 0){
		s->type = MP3;
		goto done;
	}
	if(strcmp(dot, "ogg") == 0){
		s->type = VORBIS;
		goto done;
	}
	/* Unsupported file suffix */
	goto error;

done:
	fd = open(path, OREAD);
	if(fd < 0)
		goto error;

	switch(s->type){
	case FLAC:
		s->fmeta = readflacmeta(fd, needpic);
		break;
	case MP3:
		s->idmeta = readid3(fd);
		break;
	case VORBIS:
		/* TODO parse raw ogg file */
		break;
	}
	close(fd);
	/* We can check the pointer without having to worry about which one it is */
	if(s->fmeta == nil)
		goto error;

	return s;

error:
	free(s);
	return nil;
}

int
songcmp(void *a, void *b)
{
	Song *s1 = *((Song**)a);
	Song *s2 = *((Song**)b);
	int t1, t2;
	t1 = t2 = 0;

	//ID3 does not hold tracknumber so we assume all are equal
	if(s1->type == MP3 || s2->type == MP3)
		return 0;

	switch(s1->type){
	case FLAC:
		t1 = s1->fmeta->com->tracknumber;
		break;
	case VORBIS:
		t1 = s1->vmeta->tracknumber;
		break;
	default:
		sysfatal("bad type in songcmp");
	}

	switch(s2->type){
	case FLAC:
		t2 = s2->fmeta->com->tracknumber;
		break;
	case VORBIS:
		t2 = s2->vmeta->tracknumber;
		break;
	default:
		sysfatal("bad type in songcmp");
	}

	if(t1 < t2)
		return -1;

	if(t1 > t2)
		return 1;

	return 0;
}

int
dir2album(Album *a, char *path)
{
	Dir *files;
	int fd;
	long n;
	long i;
	Rune *albumtitle;
	char buf[512];
	int songcount = 0;
	int needpic = 0;

	fd = open(path, OREAD);
	if(fd < 0)
		return 0;

	n = dirreadall(fd, &files);
	close(fd);
	if(n <= 0)
		return 0;

	/* Greedy alloc to start, we will trim down later */
	a->nsong = n;
	a->songs = emalloc(sizeof(Song*) * n);

	for(i=0;i<n;i++){
		snprint(buf, 512, "%s/%s", path, files[i].name);
		a->songs[songcount] = file2song(buf, needpic++);
		if(a->songs[songcount] == nil)
			continue;
		if(a->name == nil){
			switch(a->songs[songcount]->type){
			case FLAC:
				albumtitle = a->songs[songcount]->fmeta->com->album;
				break;
			case MP3:
				albumtitle = a->songs[songcount]->idmeta->album;
				break;
			case VORBIS:
				albumtitle = a->songs[songcount]->vmeta->album;
				break;
			default:
				albumtitle = nil;
			}
			if(albumtitle != nil)
				a->name = runesmprint("%S",  albumtitle);
		}
		a->songs[songcount]->path = strdup(buf);
		songcount++;
	}

	a->nsong = songcount;
	a->songs = realloc(a->songs, sizeof(Song*) * songcount);

	qsort(a->songs, songcount, sizeof(Song*), songcmp);

	free(files);
	return 1;
}

int
parselibrary(Album **als, char *path)
{
	Dir *files;
	int fd;
	uint n, i;
	uint numdirs = 0;
	uint alcount = 0;
	char buf[1024];

	fd = open(path, OREAD);
	if(fd < 0)
		return 0;

	n = dirreadall(fd, &files);
	close(fd);
	if(n <= 0)
		return 0;

	for(i=0;i<n;i++)
		if(files[i].qid.type&QTDIR)
			numdirs++;

	*als = emalloc(sizeof(Album)*numdirs);
	for(i=0;i<n;i++)
		if(files[i].qid.type&QTDIR){
			snprint(buf, 512, "%s/%s", path, files[i].name);
			dir2album(*als+alcount, buf);
			if(dir2album(*als+alcount, buf))
				alcount++;
		}

	*als = realloc(*als, sizeof(Album)*alcount);

	return alcount;
}