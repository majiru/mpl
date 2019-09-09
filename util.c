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
