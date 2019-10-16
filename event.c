#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>

#include "dat.h"
#include "fncs.h"

Click click[128];
Click *next;
int size;

enum{
	MIN,
	CIN,
	ROLL,
};

Click*
findpoint(Point xy)
{
	int i;
	for(i=0;i<size;i++)
		if(ptinrect(xy, click[i].r))
			return click+i;
	return nil;
}

void
eventthread(void *arg)
{
	Mouse m;
	Click c;
	Channel **chans = arg;
	Channel *min = chans[0];
	Channel *cin = chans[1];
	Channel *out = chans[2];
	Channel *roll = chans[3];
	free(chans);

	Alt alts[] = {
		{min, &m, CHANRCV},
		{cin, &c, CHANRCV},
		{roll, nil, CHANRCV},
		{nil, nil, CHANEND},
	};
	for(;;)
		switch(alt(alts)){
		case CIN:
			size++;
			assert(size < 128);
			click[size] = c;
			break;
		case MIN:
			if(m.buttons != 1)
				continue;
			next = findpoint(m.xy);
			if(next != nil)
				send(out, next);
			break;
		case ROLL:
			size = 0;
			break;
		}
}

void
spawnevent(Channel *min, Channel *cin, Channel *out, Channel *rollback)
{
	Channel **chans;
	size = 0;
	chans = emalloc(sizeof(Channel*)*4);
	chans[0] = min;
	chans[1] = cin;
	chans[2] = out;
	chans[3] = rollback;
	threadcreate(eventthread, chans, 8192);
}

