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


Mousectl 	*mctl;
Keyboardctl *kctl;
Channel		*queuein;
Channel		*queueout;
Channel		*ctl;
Album		*start, *cur, *stop;
int			cursong;
int			decpid;

Image *black;
Image *red;
Image *background;

enum decmsg msg;

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
	if(isnew && getwindow(display, Refnone) < 0)
		quit("eresized: Can't reattach to window");

	draw(screen, screen->r, background, nil, ZP);
	drawlibrary(cur, stop, cur, black, red, cursong);
	flushimage(display, Refnone);
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
	int nalbum;
	cursong = 0;
	queuein = queueout = ctl = nil;

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