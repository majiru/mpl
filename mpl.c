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
	REDRAW,
	NONE
};

Mousectl 	*mctl;
Keyboardctl *kctl;
Channel		*ctl, *lout, *loadc;
Channel		*vctl, *vlevel;
Channel		*clickin, *clickreset;
int			decpid;
int 		showlists;

int mflag, sflag, pflag, rflag, fflag;

Image *black;
Image *red;
Image *background;
Image *listbackground;

int
cleanup(void*,char*)
{
	postnote(PNGROUP, decpid, "kill");
	closedisplay(display);
	closemouse(mctl);
	closekeyboard(kctl);
	return 0;
}

void
quit(char *err)
{
	cleanup(nil, nil);
	threadexitsall(err);
}

void
eresized(int isnew)
{
	int level;
	Lib lib;
	Point p;
	if(isnew && getwindow(display, Refnone) < 0)
		quit("eresized: Can't reattach to window");

	p = screen->r.min;
	send(clickreset, nil);
	draw(screen, screen->r, background, nil, ZP);
	recv(lout, &lib);
	if(showlists) {
		drawlists(p, black, listbackground, clickin);
		p.x+=256;
	}
	drawlibrary(&lib, p, black, red, clickin);
	recv(vlevel, &level);
	drawvolume(level, black);
	flushimage(display, Refnone);
}

void
handleaction(Rune kbd)
{
	enum volmsg vmsg;
	enum cmsg msg;
	char buf[512] = {0};
	switch(kbd){
		case Kbs:
		case Kdel:
			postnote(PNGROUP, decpid, "kill");
			quit(nil);
			return;
		case 'w':
			break;
		case 's':
			showlists = !showlists;
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
		case 'd':
			msg = DUMP;
			send(ctl, &msg);
			/* lib proc is waiting for us to give it a name */
			enter("name?", buf, sizeof buf, mctl, kctl, nil);
			sendp(loadc, buf);
			break;
		case 'o':
			enter("Playlist?", buf, sizeof buf, mctl, kctl, nil);
			sendp(loadc, buf);
			break;
	}
	eresized(0);
}

void
usage(void)
{
	fprint(2, "Usage: -mspr %s file", argv0);
	sysfatal("usage");
}

void
threadmain(int argc, char *argv[])
{
	Rune kbd;
	Channel *clickout;
	int resize[2];
	ctl = vctl = vlevel = nil;
	mflag = sflag = pflag = rflag = fflag = 0;
	showlists = 0;

	/*
	 * This shouldn't need to be buffered,
	 * but for some reason it likes to still be
	 * blocked by the time the libproc is asked
	 * to produce the Lib struct. TODO: Investigate.
	 */
	Channel *redraw = chancreate(1, 1);

	ARGBEGIN{
	case 'm': mflag++; break;
	case 's': sflag++; break;
	case 'p': pflag++; break;
	case 'r': rflag++; break;
	case 'f': fflag++; break;
	default: usage();
	}ARGEND

	if(mflag+sflag+pflag+rflag+fflag < 1){
		fprint(2, "Please specify a playlist flag(m, s, r, f, or p)\n");
		threadexits(nil);
	}

	threadnotify(cleanup, 1);

	if(argc != 1)
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
	loadc = chancreate(sizeof(char*), 0);
	spawnlib(ctl, lout, clickout, redraw, loadc, argv[0]);

	vctl = chancreate(sizeof(enum volmsg), 0);
	vlevel = chancreate(sizeof(int), 0);
	spawnvol(vctl, vlevel);

	red = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlue);
	black = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DBlack);
	listbackground = allocimagemix(display, DPalebluegreen, DPalegreen);
	background = allocimagemix(display, DYellow, DPalegreyblue);

	eresized(0);

	Alt alts[] = {
		{mctl->resizec, resize, CHANRCV},
		{kctl->c, &kbd, CHANRCV},
		{redraw, nil, CHANRCV},
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
			case REDRAW:
				eresized(0);
				break;
		}
	}
}
