#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

/* Malloc and memset to 0, quit gracefully on error */
void*
emalloc(vlong size)
{
	void *v = malloc(size);
	if(v == nil)
		quit("Out of memory");
	setmalloctag(v, getcallerpc(&size));
	v = memset(v, 0, size);
	return v;
}

/*
* Unmarshal LSB ordered buffer into unsigned int
* https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html
*/
u64int
lebtoi(uchar *buf, int nbyte)
{
	int i;
	u64int out = 0;
	for(i=0;i<nbyte;i++)
		out = out | buf[i]<<(i*8);
	return out;
}

/* Unmarshal MSB ordered buffer into unsigned int */
u64int
bebtoi(uchar *buf, int nbyte)
{
	int i;
	int n = nbyte;
	u64int out = 0;
	for(i=0;i<nbyte;i++)
		out = out | buf[i]<<(--n*8);
	return out;
}

void
kill(int pid)
{
	int nfd;
	char *note = smprint( "/proc/%d/note", pid);
	nfd = open(note, OWRITE);
	if(nfd<0)
		sysfatal("proc doesn't exist");
	if(write(nfd, "kill", 4)!=4)
		sysfatal("could not write to note");
	close(nfd);
	free(note);
}

int
runecstrcmp(Rune *s1, Rune *s2)
{
	Rune c1, c2;

	for(;;) {
		c1 = tolowerrune(*s1++);
		c2 = tolowerrune(*s2++);
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
}
