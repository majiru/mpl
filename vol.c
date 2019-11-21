#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

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
	free(chans);

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
			level = level > 100 ? 100 : level;
			writevol(fd, muted == 0 ? level : 0);
			break;
		case DOWN:
			level-=5;
			level = level < 0 ? 0 : level;
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
spawnvol(Channel *ctl, Channel *out)
{
	Channel **chans;
	chans = emalloc(sizeof(Channel*)*2);
	chans[0] = ctl;
	chans[1] = out;
	threadcreate(volthread, chans, 8192);
}