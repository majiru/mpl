#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <keyboard.h>
#include <cursor.h>
#include <mouse.h>

#include "dat.h"
#include "fncs.h"

enum {
	MOUSEC,
	RESIZEC,
	KEYC,
	NONE
};

enum volmsg{
	UP,
	DOWN,
	MUTE,
	UNMUTE,
};


Mousectl 	*mctl;
Keyboardctl *kctl;
Channel		*ctl, *lout;
Channel		*vctl, *vlevel;
int			decpid;

Image *black;
Image *red;
Image *background;

int
cleanup(void*,char*)
{
	killgrp(decpid);
	closedisplay(display);
	closemouse(mctl);
	closekeyboard(kctl);
	return 0;
}

void
quit(char *err)
{
	closedisplay(display);
	closemouse(mctl);
	closekeyboard(kctl);
	threadexitsall(err);
}

void
eresized(int isnew)
{
	int level;
	Lib lib;
	if(isnew && getwindow(display, Refnone) < 0)
		quit("eresized: Can't reattach to window");

	draw(screen, screen->r, background, nil, ZP);
	recv(lout, &lib);
	drawlibrary(lib.cur, lib.stop, lib.cur, black, red, lib.cursong);
	recv(vlevel, &level);
	drawvolume(level, black);
	flushimage(display, Refnone);
}

void
handleaction(Rune kbd)
{
	enum volmsg vmsg;
	enum cmsg msg;
	switch(kbd){
		case Kbs:
		case Kdel:
			killgrp(decpid);
			quit(nil);
			return;
		case 'w':
			break;
		case 'p':
			msg = PAUSE;
			send(ctl, &msg);
			return;
		case 'l':
			msg = START;
			send(ctl, &msg);
			return;
		case 'n':
			msg = NEXT;
			send(ctl, &msg);
			break;
		case 'm':
			msg = PREV;
			send(ctl, &msg);
			break;
		case '9':
			vmsg = DOWN;
			send(vctl, &vmsg);
			break;
		case '0':
			vmsg = UP;
			send(vctl, &vmsg);
			break;
	}
	eresized(0);
}

void
readvol(int fd, int *level)
{
	int n;
	char *dot;
	char buf[512];
	n = pread(fd, buf, (sizeof buf) - 1, 0);
	buf[n] = '\0';
	dot = strchr(buf, '\n');
	if(dot == nil)
		goto error;

	*dot = '\0';
	dot = strstr(buf, "left");
	if(dot != nil){
		*level = atoi(dot+4+1);
		return;
	}

	dot = strstr(buf, "out");
	if(dot == nil)
		goto error;

	*level = atoi(dot+3);
	return;

error:
	*level = -1;
}

void
writevol(int fd, int level)
{
	char buf[16];
	int n = snprint(buf, sizeof buf, "%d", level);
	write(fd, buf, n);
}

void
volthread(void *arg)
{
	Channel **chans = arg;
	Channel *ctl = chans[0];
	Channel *out = chans[1];

	int fd;
	int level;
	int muted = 0;
	enum volmsg vmsg;

	if((fd = open("/dev/volume", ORDWR))<0){
		/* Make volume controls NOP */
		chanclose(ctl);
		chanclose(out);
		return;
	}

	Alt alts[] = {
		{ctl, &vmsg, CHANRCV},
		{out, &level, CHANSND},
		{nil, nil, CHANEND},
	};

	readvol(fd, &level);
	for(;;){
		if(alt(alts) != 0)
			continue;
		readvol(fd, &level);
		switch(vmsg){
		case UP:
			level+=5;
			writevol(fd, muted == 0 ? level : 0);
			break;
		case DOWN:
			level-=5;
			writevol(fd, muted == 0 ? level : 0);
			break;
		case MUTE:
			muted = 1;
			writevol(fd, 0);
			break;
		case UNMUTE:
			muted = 0;
			writevol(fd, level);
			break;
		}
	}
}

void
usage(void)
{
	fprint(2, "Usage: %s file", argv0);
	sysfatal("usage");
}

void
threadmain(int argc, char *argv[])
{
	Mouse mouse;
	Rune kbd;
	int resize[2];
	Channel *vchans[2];
	ctl = vctl = vlevel = nil;

	//TODO: Use ARGBEGIN
	argv0 = argv[0];
	threadnotify(cleanup, 1);

	if(argc != 2)
		usage();

	if(initdraw(nil, nil, "mpl") < 0)
		sysfatal("%s: Failed to init screen %r", argv0);
	if((mctl = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	if((kctl = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");

	ctl = chancreate(sizeof(enum cmsg), 0);
	lout = chancreate(sizeof(Lib), 0);
	spawnlib(ctl, lout, argv[1]);

	vctl = chancreate(sizeof(enum volmsg), 0);
	vlevel = chancreate(sizeof(int), 0);
	vchans[0] = vctl;
	vchans[1] = vlevel;
	threadcreate(volthread, vchans, 8192);

	red = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlue);
	black = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlack);
	background = allocimagemix(display, DPaleyellow, DPalegreen);

	eresized(0);

	Alt alts[] = {
		{mctl->c, &mouse, CHANRCV},
		{mctl->resizec, resize, CHANRCV},
		{kctl->c, &kbd, CHANRCV},
		{nil, nil, CHANEND},
	};

	for(;;){
		switch(alt(alts)){
			case KEYC:
				handleaction(kbd);
				break;
			case RESIZEC:
				eresized(1);
				break;
		}
	}
}