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

Album*
dir2album(char *path)
{
	Album *a;
	Dir *files;
	int fd;
	long n;
	long i;
	char *dot;
	Rune *albumtitle;
	char buf[512];
	int songcount = 0;
	int needpic = 1;

	fd = open(path, OREAD);
	if(fd < 0)
		return nil;

	n = dirreadall(fd, &files);
	close(fd);
	if(n <= 0)
		return nil;

	a = emalloc(sizeof(Album));

	/* Do a single pass to find cover.^(jp^(eg g) png) */
	for(i=0;i<n;i++){
		dot = strstr(path, "cover.");
		if(dot == nil)
			continue;
		dot+=1;
		snprint(buf, 512, "%s/%s", path, files[i].name);
		fd = open(buf, OREAD);
		if(fd<0)
			continue;

		a->cover = convpic(fd, dot);
		if(a->cover != nil){
			needpic = 0;
			break;
		}
	}

	/* Greedy alloc to start, we will trim down later */
	a->nsong = n;
	a->songs = emalloc(sizeof(Song*) * n);

	for(i=0;i<n;i++){
		snprint(buf, 512, "%s/%s", path, files[i].name);
		a->songs[songcount] = file2song(buf, needpic);
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
		if(needpic == 1 && a->songs[songcount]->type == FLAC && a->songs[songcount]->fmeta->pic != nil){
			FlacPic *p = a->songs[songcount]->fmeta->pic;
			a->cover = convpicbuf(p->data, p->size, p->mime);
			if(a->cover == nil)
				quit("dir2album: Could not convert image");
			needpic--;
		}
		songcount++;
	}

	a->nsong = songcount;
	a->songs = realloc(a->songs, sizeof(Song*) * songcount);

	free(files);
	return a;
}