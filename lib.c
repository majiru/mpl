#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

Channel *queuein, *queueout, *decctl;
Lib lib;

enum{
	LMSG,
	QUEUEPOP,
	EIN,
	OUT,
};

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
	return lib->cur->songs[lib->cursong]->path;
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
	Channel *resize = chans[3];
	free(chans);

	enum cmsg msg;
	Click c;

	Alt alts[] = {
		{lctl, &msg, CHANRCV},
		{queueout, nil, CHANRCV},
		{ein, &c, CHANRCV},
		{out, &lib, CHANSND},
		{nil, nil, CHANEND},
	};
	for(;;){
		switch(alt(alts)){
		case LMSG:
			handlemsg(msg);
			break;
		case QUEUEPOP:
			handlemsg(NEXT);
			break;
		case EIN:
			lib.cur = c.a;
			lib.cursong = c.songnum;
			sendp(queuein, nextsong(&lib));
			break;
		case OUT:
			continue;
		}
		send(resize, nil);
	}
}

void
spawnlib(Channel *ctl, Channel *out, Channel *ein, Channel *resize, char *path)
{
	Channel **chans;

	queuein = queueout = decctl = nil;
	spawndec(&queuein, &decctl, &queueout);

	lib.cursong = 0;
	lib.nalbum = parselibrary(&(lib.start), path);
	if(lib.nalbum == 0)
		quit("No songs found");
	lib.cur = lib.start;
	lib.stop = lib.start+(lib.nalbum-1);

	chans = emalloc(sizeof(Channel*)*4);
	chans[0] = ctl;
	chans[1] = out;
	chans[2] = ein;
	chans[3] = resize;

	sendp(queuein, nextsong(&lib));
	threadcreate(libproc, chans, 8192);
}