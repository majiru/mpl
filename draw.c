#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

typedef struct{
	int fd;
	uvlong size;
	uchar *data;
} WriteArg;

void
picwriteproc(void *arg)
{
	int i, n;
	WriteArg *a = arg;
	int fd = a->fd;
	uchar *data = a->data;
	uvlong towrite = a->size;

	free(a);
	for(i = 0;towrite>0;){
		n = write(fd, data+i, towrite);
		i+=n;
		towrite-=n;
	}
}

typedef struct{
	int fdin;
	int fdout;
	Channel *cpid;
	char *cmd;
} ExecArg;

void
picexecproc(void *arg)
{
	ExecArg *a = arg;
	dup(a->fdin, 0);
	dup(a->fdout, 1);
	procexecl(a->cpid, a->cmd, a->cmd, "-c", nil);
}

void
picresizeproc(void *arg)
{
	ExecArg *a = arg;
	dup(a->fdin, 0);
	dup(a->fdout, 1);
	procexecl(a->cpid, "/bin/resize", "resize", "-x", "256", "-y", "256", nil);
}

char*
mime2bin(char *mime)
{
	char *start = strchr(mime, '/');
	if(start!=nil)
		start+=1;
	else
		start = mime;

	if(strcmp(start, "png") == 0)
		return strdup("/bin/png");
	if(strcmp(start, "jpeg") == 0 || strcmp(start, "jpg") == 0)
		return strdup("/bin/jpg");

	return nil;
}

Image*
convpic(int fd, char *mime)
{
	ExecArg *e;
	Image *i;
	int convout[2];
	int resize[2];

	pipe(convout);
	pipe(resize);

	e = emalloc(sizeof(ExecArg));
	e->cpid = chancreate(sizeof(int), 0);
	e->fdin = fd;
	e->fdout = convout[0];
	e->cmd = mime2bin(mime);
	procrfork(picexecproc, e, 8192, RFFDG);
	recv(e->cpid, nil);
	chanfree(e->cpid);
	free(e->cmd);

	e->cpid = chancreate(sizeof(int), 0);
	e->fdin = convout[1];
	e->fdout = resize[0];
	procrfork(picresizeproc, e, 8192, RFFDG);
	recv(e->cpid, nil);
	chanfree(e->cpid);
	free(e);

	i = readimage(display, resize[1], 0);
	close(convout[0]);
	close(convout[1]);
	close(resize[0]);
	close(resize[1]);

	return i;
}

Image*
convpicbuf(uchar *buf, uvlong size, char *mime)
{
	WriteArg *w;
	int p[2];
	Image *i;

	pipe(p);

	w = emalloc(sizeof(WriteArg));
	w->fd = p[0];
	w->data = buf;
	w->size = size;
	procrfork(picwriteproc, w, 8192, RFFDG);
	/* other proc frees w */

	i = convpic(p[1], mime);

	close(p[0]);
	close(p[1]);

	return i;
}

Point
drawalbum(Album *a, Image *textcolor, Image *active, Point start, int cursong)
{
	uint i;
	Font *f = screen->display->defaultfont;
	Rune *tracktitle = nil;
	Point p = start;

	if(a->cover != nil){
		draw(screen, Rpt(p, addpt(p, a->cover->r.max)), a->cover, nil, ZP);
		p.x += a->cover->r.max.x;
	}

	runestring(screen, p, textcolor, ZP, f, a->name);
	p.y += f->height * 2;

	for(i=0;i<a->nsong;i++){
		switch(a->songs[i]->type){
		case FLAC:
			tracktitle = a->songs[i]->fmeta->com->title;
			break;
		case MP3:
			tracktitle = a->songs[i]->idmeta->title;
			break;
		case VORBIS:
			tracktitle = a->songs[i]->vmeta->title;
			break;
		}
		runestring(screen, p, i == cursong ? active : textcolor, ZP, f, tracktitle);
		p.y += f->height;
	}
	start.y+=256;
	return start;
}

void
drawlibrary(Album *start, Album *stop, Album *cur, Image *textcolor, Image *active, int cursong)
{
	Point p = screen->r.min;
	int height = screen->r.max.y - screen->r.min.y;
	Album *screenstop = start+(height/256)-1;
	stop = screenstop < stop ? screenstop : stop;
	stop+=1;

	for(;start!=stop;start++)
		p = drawalbum(start, textcolor, active, p, start ==  cur ? cursong : -1);
}