#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

const uchar none[] = {0};

/*
 * Rune and str marshal functions do not write null.
 */

void
marshalrune(int fd, Rune *r)
{
	static char buf[128];
	uchar n;

	if(r == nil || r[0] == '\0')
		write(fd, none, sizeof none);
	else{
		n = snprint(buf, 128, "%S", r);
		write(fd, &n, sizeof n);
		write(fd, buf, n);
	}
}

Rune*
unmarshalrune(int fd)
{
	static char buf[128];
	Rune *r;
	uchar n;
	read(fd, &n, sizeof n);
	if(n == 0)
		return nil;
	read(fd, buf, n);
	buf[n] = '\0';
	r = runesmprint("%s", buf);
	setmalloctag(r, getcallerpc(&fd));
	return r;
}

void
marshalstr(int fd, char *s)
{
	uint n = strlen(s);
	write(fd, &n, sizeof n);
	write(fd, s, n);
}

char*
unmarshalstr(int fd)
{
	uint n;
	char *s;
	read(fd, &n, sizeof n);
	s = emalloc(n+1);
	setmalloctag(s, getcallerpc(&fd));
	s[n] = '\0';
	read(fd, s, n);
	return s;
}

void
marshalvorbis(int fd, VorbisMeta *v)
{
	//TODO: store whole key-value pair
	marshalrune(fd, v->title);
	marshalrune(fd, v->artist);
	marshalrune(fd, v->album);
	write(fd, &(v->year), sizeof v->year);
	write(fd, &(v->tracknumber), sizeof v->tracknumber);
}

void
unmarshalvorbis(int fd, VorbisMeta *v)
{
	v->title = unmarshalrune(fd);
	v->artist = unmarshalrune(fd);
	v->album = unmarshalrune(fd);
	read(fd, &(v->year), sizeof v->year);
	read(fd, &(v->tracknumber), sizeof v->tracknumber);
}

void
marshalflacpic(int fd, FlacPic *p)
{
	//TODO store other fields
	marshalstr(fd, p->mime);
	write(fd, &(p->size), sizeof p->size);
	write(fd, p->data, p->size);
}

void
unmarshalflacpic(int fd, FlacPic **p)
{
	*p = emalloc(sizeof(FlacPic));
	(*p)->mime = unmarshalstr(fd);
	read(fd, &((*p)->size), sizeof (*p)->size);
	read(fd, (*p)->data, (*p)->size);
}

void
marshalflacmeta(int fd, FlacMeta *f)
{
	marshalvorbis(fd, f);
//	marshalflacpic(fd, f->pic);
}

void
unmarshalflacmeta(int fd, FlacMeta **f)
{
	*f = emalloc(sizeof(FlacMeta));
	unmarshalvorbis(fd, &((*f)->VorbisMeta));
//	unmarshalflacpic(fd, &((*f)->pic));
}

void
marshalid3(int fd, ID3v1 *id)
{
	marshalrune(fd, id->title);
	marshalrune(fd, id->artist);
	marshalrune(fd, id->album);
	write(fd, &(id->year), sizeof id->year);
	marshalrune(fd, id->comment);
	write(fd, &(id->genre), sizeof id->genre);
}

void
unmarshalid3(int fd, ID3v1 **id)
{
	*id = emalloc(sizeof(ID3v1));
	(*id)->title = unmarshalrune(fd);
	(*id)->artist = unmarshalrune(fd);
	(*id)->album = unmarshalrune(fd);
	read(fd, &((*id)->year), sizeof (*id)->year);
	(*id)->comment	= unmarshalrune(fd);
	read(fd, &((*id)->genre), sizeof (*id)->genre);
}

void
marshalsong(int fd, Song *s)
{
	write(fd, &(s->type), sizeof s->type);
	switch(s->type){
	case FLAC:
		marshalflacmeta(fd, s->fmeta);
		break;
	case MP3:
		marshalid3(fd, s->idmeta);
		break;
	default:
		sysfatal("not recognized or unsupported format");
	}
	marshalstr(fd, s->path);
}

void
unmarshalsong(int fd, Song *s)
{
	read(fd, &(s->type), sizeof s->type);
	switch(s->type){
	case FLAC:
		unmarshalflacmeta(fd, &(s->fmeta));
		break;
	case MP3:
		unmarshalid3(fd, &(s->idmeta));
		break;
	default:
		sysfatal("not recognized or unsupported format");
	}
	s->path = unmarshalstr(fd);
}

void
marshalalbum(int fd, Album *a)
{
	int i;
	int havepic = a->cover == nil ? 0 : 1;
	marshalrune(fd, a->name);
	write(fd, &(a->nsong), sizeof a->nsong);
	write(fd, &havepic, sizeof havepic);
	if(havepic)
		writeimage(fd, a->cover, 0);
	for(i=0;i<a->nsong;i++)
		marshalsong(fd, a->songs+i);
}

void
unmarshalalbum(int fd, Album *a)
{
	int i, havepic;
	a->name = unmarshalrune(fd);
	read(fd, &(a->nsong), sizeof a->nsong);
	read(fd, &havepic, sizeof havepic);
	if(havepic)
		a->cover = readimage(display, fd, 0);
	a->songs = emalloc(sizeof(Song)*a->nsong);
	setmalloctag(a->songs, getcallerpc(&fd));
	for(i=0;i<a->nsong;i++)
		unmarshalsong(fd, a->songs+i);
}

void
marshallib(int fd, Lib *l)
{
	int i;
	write(fd, &(l->nalbum), sizeof l->nalbum);
	for(i=0;i<l->nalbum;i++)
		marshalalbum(fd, l->start+i);
}

void
unmarshallib(int fd, Lib *l)
{
	int i;
	l->cursong = 0;
	read(fd, &(l->nalbum), sizeof l->nalbum);
	l->start = emalloc(sizeof(Album)*l->nalbum);
	setmalloctag(l->start, getcallerpc(&fd));
	for(i=0;i<l->nalbum;i++)
		unmarshalalbum(fd, l->start+i);
	l->stop = l->start+i;
	l->cur = l->start;
}
