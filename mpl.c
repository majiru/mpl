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
	RESIZEC,
	KEYC,
	NONE
};

Mousectl 	*mctl;
Keyboardctl *kctl;
Channel		*ctl, *lout;
Channel		*vctl, *vlevel;
Channel		*clickin, *clickreset;
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

	send(clickreset, nil);
	draw(screen, screen->r, background, nil, ZP);
	recv(lout, &lib);
	drawlibrary(lib.cur, lib.stop, lib.cur, black, red, lib.cursong, clickin);
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
usage(void)
{
	fprint(2, "Usage: %s file", argv0);
	sysfatal("usage");
}

void
threadmain(int argc, char *argv[])
{
	Rune kbd;
	Channel *clickout;
	int resize[2];
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

	clickin = chancreate(sizeof(Click), 0);
	clickout = chancreate(sizeof(Click), 0);
	clickreset = chancreate(1, 0);
	spawnevent(mctl->c, clickin, clickout, clickreset);

	ctl = chancreate(sizeof(enum cmsg), 0);
	lout = chancreate(sizeof(Lib), 0);
	spawnlib(ctl, lout, clickout, mctl->resizec, argv[1]);

	vctl = chancreate(sizeof(enum volmsg), 0);
	vlevel = chancreate(sizeof(int), 0);
	spawnvol(vctl, vlevel);

	red = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlue);
	black = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlack);
	background = allocimagemix(display, DPaleyellow, DPalegreen);

	eresized(0);

	Alt alts[] = {
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