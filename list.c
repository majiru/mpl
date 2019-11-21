#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

void
dumpalbum(int fd, Album *a)
{
	int i;
	char nl = '\n';
	char buf[128];
	int n = snprint(buf, 128, "%S", a->name);

	write(fd, buf, n);
	write(fd, &nl, 1);

	for(i=0;i<a->nsong;i++){
		write(fd, a->songs[i].path, strlen(a->songs[i].path));
		write(fd, &nl, 1);
	}
}

void
libdir(char *buf, int n)
{
	char *home;
	int s;
	home = getenv("home");
	s = snprint(buf, n, "%s/lib/mpl", home);
	buf[s] = '\0';
	free(home);
}

void
createlibdir(char *path)
{
	int fd;
	fd = open(path, OREAD);
	if(fd>0){
		close(fd);
		return;
	}
	fd = create(path, OREAD, DMDIR|0755);
	if(fd<0)
		sysfatal("could not create lib dir: %r");
	close(fd);
}

void
dumplib(Lib *l)
{
	int fd;
	int i;
	char buf[512];
	char fname[512];
	char sep[] = "\n\n";

	libdir(buf, 512);
	assert(l->name != nil);
	createlibdir(buf);
	i = snprint(fname, 512, "%s/%s.list", buf, l->name);
	fname[i] = '\0';
	fd = create(fname, ORDWR, 0644);
	if(fd<0){
		fprint(2, "Could not create list file: %r\n");
		quit(nil);
	}
	for(i=0;i<l->nalbum;i++){
		dumpalbum(fd, l->start+i);
		write(fd, sep, sizeof sep - 1);
	}
	close(fd);
	snprint(fname, 512, "%s/%s.db", buf, l->name);
	fd = create(fname, ORDWR, 0644);
	if(fd<0){
		fprint(2, "Count not create list db file: %r");
		quit(nil);
	}
	marshallib(fd, l);
	close(fd);
}

void
loadalbum(Biobuf *b, Album *a)
{
	int size, cap;
	char *dot;
	dot = Brdstr(b, '\n', 1);
	if(dot == nil){
		return;
	}
	a->name = runesmprint("%s", dot);
	free(dot);

	cap = 10;
	a->songs = emalloc(sizeof(Song)*cap);
	for(size=0;(dot = Brdstr(b, '\n', 1));size++){
		if(strlen(dot) == 0){
			free(dot);
			break;
		}
		if(size == cap-1){
			cap = cap * 2;
			a->songs = realloc(a->songs, sizeof(Song)*cap);
		}
		if(file2song(a->songs+size, dot, 0) == 0)
			sysfatal("Could not parse song %s", dot);
	}

	a->songs = realloc(a->songs, sizeof(Song)*size);
	a->nsong = size;
	a->cover = nil;
	return;
}

void
loadlib(Lib *l)
{
	Biobuf *b;
	char buf[512];
	char fname[512];
	int size, cap;
	int fd;
	long r;

	assert(l->name != nil);
	libdir(buf, 512);

	/* Check for db cache file first */
	snprint(fname, 512, "%s/%s.db", buf, l->name);
	if((fd = open(fname, OREAD))>0){
		unmarshallib(fd, l);
		close(fd);
		return;
	}

	snprint(fname, 512, "%s/%s.list", buf, l->name);
	b = Bopen(fname, OREAD);
	if(b == nil){
		fprint(2, "Could not open list file: %s\n", fname);
		quit(nil);
	}
	cap = 10;
	l->start = emalloc(sizeof(Album)*cap);
	for(size = 0;(r = Bgetrune(b)) > 0;size++){
		if(size == cap-1){
			cap = cap * 2;
			l->start = realloc(l->start, sizeof(Album)*cap);
		}
		if(r != L'\n'){
			Bungetrune(b);
		}
		(l->start+size)->name = nil;
		loadalbum(b, l->start+size);
	}
	if((l->start+size)->name == nil){
		size--;
	}
	close(Bfildes(b));
	free(b);
	l->start = realloc(l->start, sizeof(Album)*size);
	l->nalbum = size;
}
