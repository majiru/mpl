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


Mousectl 	*mctl;
Keyboardctl *kctl;
Album		*a;
Dec 		d;
int		cursong;

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
	if(isnew && getwindow(display, Refnone) < 0)
		quit("eresized: Can't reattach to window");

	draw(screen, screen->r, background, nil, ZP);
	drawalbum(a, black, red, cursong);
	flushimage(display, Refnone);
}

void
play(void)
{
	enum decmsg msg = STOP;
	send(d.ctl, &msg);
	kill(d.decpid);
	cursong = cursong < 0 ? a->nsong-1 : cursong;
	cursong = cursong > a->nsong-1 ? 0 : cursong;
	playfile(&d, a->songs[cursong]->path);
	eresized(0);
}

void
handleaction(Rune kbd)
{
	enum decmsg msg;
	switch(kbd){
		case Kbs:
		case Kdel:
			quit(nil);
			break;
		case 'w':
			eresized(0);
			break;
		case 'p':
			msg = PAUSE;
			send(d.ctl, &msg);
			break;
		case 'l':
			msg = START;
			send(d.ctl, &msg);
			break;
		case 'n':
			cursong++;
			play();
			break;
		case 'm':
			cursong--;
			play();
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
	cursong = 0;

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

	memset(&d, 0, sizeof d);
	red = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlue);
	black = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlack);
	background = allocimagemix(display, DPaleyellow, DPalegreen);

	a = dir2album(argv[1]);
	if(a == nil)
		quit("nil album");
	if(a->nsong == 0)
		quit("no songs");
	playfile(&d, a->songs[0]->path);
	handleaction('w');

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