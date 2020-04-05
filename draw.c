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
	close(fd);
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
	procexecl(a->cpid, a->cmd, a->cmd, "-9", nil);
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

Image*
readcover(Song *s)
{
	char buf[512], cover[512];
	char *dot, *end;
	int fd, n, i;
	Dir *files;
	Image *im;

	dot = strrchr(s->path, '/');
	if(dot == nil)
		sysfatal("readcover: bad song path");
	end = buf+(dot-s->path)+1;
	if(end - buf >= sizeof buf)
		sysfatal("readcover: buffer too small");
	seprint(buf, end, "%s", s->path);

	fd = open(buf, OREAD);
	if(fd < 0)
		sysfatal("readcover: %r");
	n = dirreadall(fd, &files);
	close(fd);
	if(n <= 0)
		sysfatal("readcover: no files in dir");

	for(i=0;i<n;i++){
		dot = cistrstr(files[i].name, "cover.");
		if(dot == nil){
			dot = cistrstr(files[i].name, "folder.");
			if(dot == nil)
				continue;
		}
		dot = strrchr(dot, '.');
		dot++;
		snprint(cover, 512, "%s/%s", buf, files[i].name);
		fd = open(cover, OREAD);
		if(fd<0)
			continue;
		im = convpic(fd, dot);
		close(fd);
		return im;
	}
	return nil;
}

Point
drawalbum(Album *a, Image *textcolor, Image *active, Point start, int cursong, Channel *clickout)
{
	uint i;
	Font *f = screen->display->defaultfont;
	Rune *tracktitle = nil;
	Point p = start;
	Click c;

	if(a->nocover == 0 && a->cover == nil)
		if((a->cover = readcover(a->songs)) == nil)
			a->nocover = 1; /* Don't search again */

	if(a->cover != nil){
		draw(screen, Rpt(p, addpt(p, a->cover->r.max)), a->cover, nil, ZP);
		p.x += a->cover->r.max.x;
	}

	runestring(screen, p, textcolor, ZP, f, a->name);
	p.y += f->height * 2;

	for(i=0;i<a->nsong;i++){
		switch((a->songs+i)->type){
		case FLAC:
			tracktitle = (a->songs+i)->fmeta->title;
			break;
		case MP3:
			tracktitle = (a->songs+i)->idmeta->title;
			break;
		case VORBIS:
			tracktitle = (a->songs+i)->vmeta->title;
			break;
		case RADIO:
			tracktitle = (a->songs+i)->title;
			if(tracktitle == nil)
				return start;
			break;
		default:
			sysfatal("Unknown song type");
		}
		runestring(screen, p, i == cursong ? active : textcolor, ZP, f, tracktitle);
		c.r = Rpt(p, Pt(p.x+runestrlen(tracktitle)*f->width,p.y+f->height));
		c.a = a;
		c.songnum = i;
		c.type = CSONG;
		send(clickout, &c);
		p.y += f->height;
	}
	if(p.y > start.y+256)
		start.y = p.y;
	else
		start.y+=256;
	return start;
}

void
drawlibrary(Lib *l, Point p, Image *textcolor, Image *active, Channel *clickout)
{
	Album *start, *stop, *cur, *screenstop;
	int cursong, height;

	start = l->cur;
	stop = l->stop;
	cur = l->cur;
	cursong = l->cursong;
	height = screen->r.max.y - screen->r.min.y;
	screenstop = start+(height/256)-1;
	stop = screenstop < stop ? screenstop : stop;
	stop+=1;

	if(start==stop)
		stop++;

	for(;start!=stop;start++){
		p = drawalbum(start, textcolor, active, p, start == cur ? cursong : -1, clickout);
	}
}

void
drawlists(Point p, Image *textcolor, Image *background, Channel *clickout)
{
	Rectangle r;
	int n, i;
	int fd;
	Dir *files;
	char buf[512];
	Font *f = screen->display->defaultfont;
	Hmap *h;
	char **keys, *dot;
	Click c;

	libdir(buf, sizeof buf);
	fd = open(buf, OREAD);
	if(fd < 0)
		sysfatal("could not open libdir");

	n = dirreadall(fd, &files);
	close(fd);
	if(n <= 0){
		free(files);
		return;
	}

	h = allocmap(32);
	r.min = p;
	r.max.x = r.min.x + 256;
	r.max.y = screen->r.max.y;
	draw(screen, r, background, nil, ZP);
	for(i=0;i<n;i++){
		if((dot = strrchr(files[i].name, '.')) != nil)
			*dot = '\0';
		mapinsert(h, files[i].name, nil);
	}
	keys = emalloc(sizeof(char*)*n);
	n = mapdumpkey(h, keys, n);
	for(i=0;i<n;i++){
		string(screen, p, textcolor, ZP, f, keys[i]);
		c.r = Rpt(p, Pt(p.x+strlen(keys[i])*f->width,p.y+f->height));
		c.list = strdup(keys[i]);
		c.type = CLIST;
		send(clickout, &c);
		p.y += f->height;
	}

	freemap(h);
	free(keys);
	free(files);
}

void
drawvolume(int level, Image* color)
{
	Point p;
	char buf[128];
	Font *f = screen->display->defaultfont;
	int n = snprint(buf, sizeof buf, "Vol: %d%%", level);
	p.y = screen->r.min.y;
	p.x = screen->r.max.x;
	p.x-=(n*f->width);
	string(screen, p, color, ZP, f, buf);
}
