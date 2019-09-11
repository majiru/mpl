#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

/* Proc argument structs */
typedef struct {
	int outpipe;
	char *file;
	Channel *cpid;
} Decodearg;

typedef struct {
	int inpipe;
	Channel *ctl;
} Ctlarg;

void
decodeproc(void *arg)
{
	Decodearg *a = arg;
	int nfd;

	nfd = open("/dev/null", OREAD|OWRITE);
	dup(nfd, 0);
	dup(nfd, 2);
	dup(a->outpipe, 1);
	procexecl(a->cpid, "/bin/play", "play", "-o", "/fd/1", a->file, nil);
}

void
ctlproc(void *arg)
{
	int afd;
	int bufsize;
	int n;
	enum decmsg msg;
	char *buf;
	
	Ctlarg *a = arg;
	Channel *c = a->ctl;
	int inpipe = a->inpipe;
	free(a);

	afd = open("/dev/audio", OWRITE);
	if(afd < 0)
		threadexits("could not open audio device");
	bufsize = iounit(afd);
	buf = emalloc(bufsize);

	for(;;){
		if(nbrecv(c, &msg) != 0)
			switch(msg){
			case STOP:
				threadexits(nil);
				break;
			case PAUSE:
				//Block until we get a START message
				while(msg != START)
					recv(c, &msg);
				break;
			}
		n = read(inpipe, buf, bufsize);
		write(afd, buf, n);
	}
}

void
playfile(Dec *d, char *file)
{
	Decodearg *a;
	Ctlarg *c;
	int p[2];

	if(d->ctl != nil)
		chanfree(d->ctl);
	
	pipe(p);
	a = emalloc(sizeof(Decodearg));
	a->cpid = chancreate(sizeof(int), 0);
	a->outpipe = p[1];
	a->file = file;
	procrfork(decodeproc, a, 8192, RFFDG);
	recv(a->cpid, &(d->decpid));
	chanfree(a->cpid);
	free(a);

	c = emalloc(sizeof(Ctlarg));
	c->ctl = d->ctl = chancreate(sizeof(enum decmsg), 0);
	c->inpipe = p[0];
	d->ctlpid = procrfork(ctlproc, c, 8192, RFFDG);
	/* Other proc frees c */

	close(p[0]);
	close(p[1]);
}
