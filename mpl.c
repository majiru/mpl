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

Dec 		*d;
Mousectl 	*mctl;
Keyboardctl *kctl;
Image		*cover;

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
			draw(screen, screen->r, cover, nil, ZP);
			break;
		case 'b':
			draw(screen, screen->r, display->black, nil, ZP);
			break;
		case 'p':
			msg = PAUSE;
			send(d->ctl, &msg);
			break;
		case 'l':
			msg = START;
			send(d->ctl, &msg);
			break;
	}
	flushimage(display, Refnone);
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

	d = spawndecoder(argv[1]);

	int fd = open(argv[1], OREAD);
	if(fd < 0)
		quit("can not open file");
	FlacMeta *f = readFlacMeta(fd);
	if(f == nil)
		print("failed to parse\n");
	if(f != nil && f->com == nil)
		print("failed to parse comm\n");

	convFlacPic(f->pic);
	if(f->pic->i == nil)
		quit("Could not load image\n");
	cover = f->pic->i;
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