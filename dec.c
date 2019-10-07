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
} Writearg;

void
decodeproc(void *arg)
{
	Decodearg *a = arg;
	int nfd;

	nfd = open("/dev/null", OREAD|OWRITE);
	dup(nfd, 0);
	dup(nfd, 2);
	dup(a->outpipe, 1);
	close(nfd);
	close(a->outpipe);
	procexecl(a->cpid, "/bin/play", "play", "-o", "/fd/1", a->file, nil);
}

void
writethread(void *arg)
{
	int afd;
	int bufsize;
	int n;
	enum cmsg msg;
	char *buf;
	
	Writearg *a = arg;
	Channel *c = a->ctl;
	int inpipe = a->inpipe;

	afd = open("/dev/audio", OWRITE);
	if(afd < 0)
		sysfatal("could not open audio device");
	bufsize = iounit(afd);
	/*
	* On native plan9 installs this will return 0,
	* so we default to the default iosize.
	*/
	if(bufsize == 0)
		bufsize = 8192;
	buf = emalloc(bufsize);

	for(;;){
		if(nbrecv(c, &msg) != 0)
			switch(msg){
			case STOP:
				free(buf);
				close(afd);
				threadexits(nil);
				break;
			case PAUSE:
				/* Block until we get a START message */
				while(msg != START)
					recv(c, &msg);
				break;
			}
		n = read(inpipe, buf, bufsize);
		write(afd, buf, n);
		yield();
	}
}

enum{
	QUEUE,
	CTL,
	WAIT,
};

void
ctlproc(void *arg)
{
	Channel **chans = arg;
	Channel *q = chans[0];
	Channel *c = chans[1];
	Channel *pop = chans[2];
	Channel *w = threadwaitchan();
	free(chans);

	char *path;
	enum cmsg msg;
	Waitmsg *wmsg;

	Decodearg a;
	Writearg wr;
	int p[2];
	/* This allows for the main thread to kill the decoder on exit */
	extern int decpid = -1;

	Alt alts[] = {
		{q, &path, CHANRCV},
		{c, &msg,  CHANRCV},
		{w, &wmsg, CHANRCV},
		{nil, nil, CHANEND},
	};

	pipe(p);

	a.cpid = chancreate(sizeof(int), 0);
	a.outpipe = p[0];

	wr.ctl = chancreate(sizeof(enum cmsg), 0);
	wr.inpipe = p[1];

	/* Start first song to stop blocks on writethread read */
	a.file = recvp(q);
	procrfork(decodeproc, &a, 8192, RFFDG);
	recv(a.cpid, &decpid);
	threadcreate(writethread, &wr, 8192);

	for(;;){
		switch(alt(alts)){
			case WAIT:
				if(strstr(wmsg->msg, "eof")){
					decpid = -1;
					send(pop, nil);
				}
				free(wmsg);
				break;
			case CTL:
				if(msg == NEXT){
					killgrp(decpid);
					decpid = -1;
					send(pop, nil);
				}else
					send(wr.ctl, &msg);
				break;
			case QUEUE:
				a.file = path;
				if(decpid != -1)
					killgrp(decpid);
				procrfork(decodeproc, &a, 8192, RFFDG);
				recv(a.cpid, &decpid);
				break;
			default:
				goto cleanup;
		}
	}


cleanup:
	if(decpid != -1)
		killgrp(decpid);
	chanfree(wr.ctl);
	chanfree(a.cpid);
	close(p[0]);
	close(p[1]);

}

/*
* Spawns the decoder processes.
* q is a queue for next songs. chan char*
* c is for sending control messages. chan enum cmsg
* nil msg is sent over pop on song change.
*/
void
spawndec(Channel **q, Channel **c, Channel **pop)
{
	Channel **chans = emalloc(sizeof(Channel*) * 3);

	if(*q == nil)
		*q = chancreate(sizeof(char*), 0);
	if(*c == nil)
		*c = chancreate(sizeof(enum cmsg), 0);
	if(*pop == nil)
		*pop = chancreate(sizeof(char), 0);

	chans[0] = *q;
	chans[1] = *c;
	chans[2] = *pop;

	procrfork(ctlproc, chans, 8192, RFFDG);
}