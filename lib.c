#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

Channel *queuein, *queueout, *decctl;
Channel *radiochan;
Lib lib;

char*
nextsong(Lib *lib)
{
	if(lib->cursong < 0){
		lib->cur--;
		if(lib->cur < lib->start)
			lib->cur = lib->stop;
		lib->cursong = lib->cur->nsong-1;
	}
	if(lib->cursong > lib->cur->nsong-1){
		lib->cur++;
		if(lib->cur > lib->stop)
			lib->cur = lib->start;
		lib->cursong = 0;
	}
	return (lib->cur->songs+(lib->cursong))->path;
}

void
handlemsg(enum cmsg msg)
{
	switch(msg){
	case NEXT:
		lib.cursong++;
		sendp(queuein, nextsong(&lib));
		break;
	case PREV:
		lib.cursong--;
		sendp(queuein, nextsong(&lib));
		break;
	case STOP:
	case START:
	case PAUSE:
		send(decctl, &msg);
		break;	
	}
}

void
libproc(void *arg)
{
	Channel **chans = arg;
	Channel *lctl = chans[0];
	Channel *out = chans[1];
	Channel *ein = chans[2];
	Channel *redraw = chans[3];
	Channel *loadc = chans[4];
	free(chans);

	enum cmsg msg;
	Click c;

	char *radiotitle = nil;
	char *toload;

	Alt alts[] = {
		{lctl, &msg, CHANRCV},
		{queueout, nil, CHANRCV},
		{ein, &c, CHANRCV},
		{out, &lib, CHANSND},
		{radiochan, &radiotitle, CHANRCV},
		{loadc,	&toload, CHANRCV},
		{nil, nil, CHANEND},
	};
	enum{
		LMSG,
		QUEUEPOP,
		EIN,
		OUT,
		RADIOIN,
		LOADC,
	};

	for(;;){
		switch(alt(alts)){
		case LMSG:
			if(msg == DUMP){
				/* TODO: rename chan to imply this use */
				toload = recvp(loadc);
				free(lib.name);
				lib.name = toload;
				dumplib(&lib);
			}
			handlemsg(msg);
			break;
		case QUEUEPOP:
			handlemsg(NEXT);
			break;
		case EIN:
			switch(c.type){
			case CSONG:
				lib.cur = c.a;
				lib.cursong = c.songnum;
				sendp(queuein, nextsong(&lib));
				break;
			case CLIST:
				//TODO clean more
				free(lib.name);
				lib.name = strdup(c.list);
				goto Load;
			}
			break;
		case OUT:
			continue;
		case RADIOIN:
			/* Eat everything in the channel;
			 * This leaks memory,
			 * but fixes a race.
			 * TODO: Find a better fix.
			 */
			while(nbrecv(radiochan, &radiotitle))
				;
			if(lib.cur->songs->title != nil)
				free(lib.cur->songs->title);
			lib.cur->songs->title = runesmprint("%s", radiotitle);
			free(radiotitle);
			break;
		case LOADC:
			//TODO: clean more
			free(lib.name);
			lib.name = strdup(toload);
Load:
			loadlib(&lib);
			lib.stop = lib.start+(lib.nalbum-1);
			lib.cursong = 0;
			lib.cur = lib.start;
			break;
		}
		send(redraw, nil);
	}
}

void
radioproc(void *arg)
{
	char *path = arg;
	char buf[512];
	char *dot, *end;
	Biobuf *b;

	dot = strrchr(path, '/');
	if(dot == nil)
		sysfatal("readradiosong: bad song path");
	end = buf+(dot-path)+1;
	if(end - buf > sizeof buf)
		sysfatal("readradiosong: buffer too small");
	seprint(buf, end, "%s", path);
	snprint(buf, 512, "%s/playing", buf);
	free(path);
	b = Bopen(buf, OREAD);
	if(b == nil)
		sysfatal("readradiosong: Bopen %r");
	for(;;){
		path = Brdstr(b, '\n', 1);
		sendp(radiochan, path);
	}
}

void
spawnlib(Channel *ctl, Channel *out, Channel *ein, Channel *redraw, Channel *loadc, char *path)
{
	Channel **chans;
	char *name;

	queuein = queueout = decctl = nil;
	radiochan = nil;

	spawndec(&queuein, &decctl, &queueout);
	radiochan = chancreate(sizeof(char*), 1024);

	name = strrchr(path, '/');
	lib.name = strdup(name == nil ? path : name);

	extern int mflag;
	extern int sflag;
	extern int rflag;
	extern int pflag;
	extern int fflag;

	if(mflag == 1){
		chanclose(radiochan);
		lib.nalbum = parselibrary(&(lib.start), path);
		if(lib.nalbum == 0)
			sysfatal("no songs found");
		lib.stop = lib.start+(lib.nalbum-1);
	}else if(sflag == 1){
		chanclose(radiochan);
		lib.nalbum = 1;
		lib.start = emalloc(sizeof(Album));
		if(!dir2album(lib.start, path))
			sysfatal("no songs found");
		lib.stop = lib.start;
	}else if(rflag == 1){
		lib.nalbum = 1;
		lib.start = emalloc(sizeof(Album));
		lib.stop = lib.start;
		radio2album(lib.start, path);
		proccreate(radioproc, strdup(path), 8192);
	}else if(pflag == 1){
		chanclose(radiochan);
		loadlib(&lib);
		lib.stop = lib.start+(lib.nalbum-1);
	}else if(fflag == 1){
		lib.nalbum = 1;
		lib.start = emalloc(sizeof(Album));
		lib.stop = lib.start;
		file2album(lib.start, L"", path);
	}

	lib.cursong = 0;
	lib.cur = lib.start;

	chans = emalloc(sizeof(Channel*)*5);
	chans[0] = ctl;
	chans[1] = out;
	chans[2] = ein;
	chans[3] = redraw;
	chans[4] = loadc;

	sendp(queuein, nextsong(&lib));
	threadcreate(libproc, chans, 8192);
}
