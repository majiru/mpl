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
	QUEUEPOP,
	NONE
};

enum volmsg{
	UP,
	DOWN,
	MUTE,
	UNMUTE,
	DRAW,
};


Mousectl 	*mctl;
Keyboardctl *kctl;
Channel		*queuein;
Channel		*queueout;
Channel		*ctl;
Channel		*vctl;
Album		*start, *cur, *stop;
int			cursong;
int			decpid;

Image *black;
Image *red;
Image *background;

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
	enum volmsg vmsg = DRAW;
	if(isnew && getwindow(display, Refnone) < 0)
		quit("eresized: Can't reattach to window");

	draw(screen, screen->r, background, nil, ZP);
	drawlibrary(cur, stop, cur, black, red, cursong);
	send(vctl, &vmsg);
}

char*
nextsong(void)
{
	if(cursong < 0){
		cur--;
		if(cur < start)
			cur = stop;
		cursong = cur->nsong-1;
	}
	if(cursong > cur->nsong-1){
		cur++;
		if(cur > stop)
			cur = start;
		cursong = 0;
	}
	return cur->songs[cursong]->path;
}

void
handleaction(Rune kbd)
{
	enum volmsg vmsg;
	enum decmsg msg;
	switch(kbd){
		case Kbs:
		case Kdel:
			killgrp(decpid);
			quit(nil);
			break;
		case 'w':
			eresized(0);
			break;
		case 'p':
			msg = PAUSE;
			send(ctl, &msg);
			break;
		case 'l':
			msg = START;
			send(ctl, &msg);
			break;
		case 'n':
			cursong++;
			nextsong();
			sendp(queuein, nextsong());
			eresized(0);
			break;
		case 'm':
			cursong--;
			nextsong();
			sendp(queuein, nextsong());
			eresized(0);
			break;
		case '9':
			vmsg = DOWN;
			send(vctl, &vmsg);
			eresized(0);
			break;
		case '0':
			vmsg = UP;
			send(vctl, &vmsg);
			eresized(0);
			break;
	}
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
usage(void)
{
	fprint(2, "Usage: %s file", argv0);
	sysfatal("usage");
}

void
volthread(void *arg)
{
	Channel *ctl = arg;
	int fd;
	int mlevel, level;
	enum volmsg vmsg;

	if((fd = open("/dev/volume", ORDWR))<0){
		/* Make volume controls NOP */
		chanclose(ctl);
		return;
	}
	for(;;){
		recv(ctl, &vmsg);
		readvol(fd, &level);
		switch(vmsg){
		case UP:
			level+=5;
			writevol(fd, level);
			break;
		case DOWN:
			level-=5;
			writevol(fd, level);
			break;
		case MUTE:
			mlevel = level;
			level = 0;
			writevol(fd, level);
			break;
		case UNMUTE:
			level = mlevel;
			writevol(fd, level);
			break;
		case DRAW:
			drawvolume(level, black);
		}
	}
}

void
threadmain(int argc, char *argv[])
{
	Mouse mouse;
	Rune kbd;
	int resize[2];
	int nalbum;
	cursong = 0;
	queuein = queueout = ctl = vctl = nil;

	//TODO: Use ARGBEGIN
	argv0 = argv[0];	

	if(argc != 2)
		usage();

	if(initdraw(nil, nil, "mpl") < 0)
		sysfatal("%s: Failed to init screen %r", argv0);
	if((mctl = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	if((kctl = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");

	vctl = chancreate(sizeof(enum volmsg), 0);
	threadcreate(volthread, vctl, 8192);

	red = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlue);
	black = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlack);
	background = allocimagemix(display, DPaleyellow, DPalegreen);

	nalbum = parselibrary(&start, argv[1]);
	if(nalbum == 0)
		quit("No songs found");
	cur = start;
	stop = start+(nalbum-1);
	spawndec(&queuein, &ctl, &queueout);
	send(queuein, &(cur->songs[0]->path));
	handleaction('w');

	Alt alts[] = {
		{mctl->c, &mouse, CHANRCV},
		{mctl->resizec, resize, CHANRCV},
		{kctl->c, &kbd, CHANRCV},
		{queueout, nil, CHANRCV},
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
			case QUEUEPOP:
				handleaction(L'n');
				break;
		}
	}
}