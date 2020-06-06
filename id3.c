#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

/* If we can't read the tag, at least default to file names */
void
fillnotag(ID3v1 *id, int fd)
{
	char buf[512];
	char *slash, *dot;
	fd2path(fd, buf, 512);
	slash = strrchr(buf, '/');
	dot = strrchr(buf, '.');
	*dot = '\0';
	id->title = runesmprint("%s", slash+1);
}

/* The following function is stolen from https://git.sr.ht/~ft/libtags/ */
#define synchsafe(d) (uint)(((d)[0]&127)<<21 | ((d)[1]&127)<<14 | ((d)[2]&127)<<7 | ((d)[3]&127)<<0)
#define beuint(d) (uint)((d)[0]<<24 | (d)[1]<<16 | (d)[2]<<8 | (d)[3]<<0)

ID3v1*
readid3v2(int fd)
{
	int sz, framesz;
	int ver;
	char nu[8];
	uchar d[10];
	uchar buf[256];
	ID3v1 *id;

	read(fd, d, sizeof d);

	ver = d[3];
	sz = synchsafe(&d[6]);

	if(ver == 2 && (d[5] & (1<<6)) != 0) /* compression */
		return nil;

	if(ver > 2){
		if((d[5] & (1<<4)) != 0) /* footer */
			sz -= 10;
		if((d[5] & (1<<6)) != 0){ /* we don't do extended header */
			return nil;
		}
	}

	framesz = (ver >= 3) ? 10 : 6;

	id = emalloc(sizeof(ID3v1));
	id->title = id->album = nil;
	for(; sz > framesz;){
		int tsz;

		read(fd, d, framesz);
		sz -= framesz;

		if(memcmp(d, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", framesz) == 0)
			break;

		if(ver >= 3){
			tsz = (ver == 3) ? beuint(&d[4]) : synchsafe(&d[4]);
			if(tsz < 0 || tsz > sz)
				break;

			d[4] = 0;

			if((d[9] & 0x0c) != 0){ /* compression & encryption */
				return nil;
			}
		}else{
			tsz = beuint(&d[3]) >> 8;
			if(tsz > sz)
				return nil;
			d[3] = 0;
		}
		sz -= tsz;

		if(memcmp(d, "TIT2", 4) == 0){
			read(fd, nu, 1);
			switch(nu[0]){
			case 3:
				id->title = emalloc(sizeof(Rune)*tsz);
				read(fd, buf, tsz-1);
				runesprint(id->title, "%s", (char*)buf);
			}
		}else if(memcmp(d, "TALB", 4) == 0){
			read(fd, nu, 1);
			switch(nu[0]){
			case 3:
				id->album = emalloc(sizeof(Rune)*tsz);
				read(fd, buf, tsz-1);
				runesprint(id->album, "%s", (char*)buf);
			}
		}
	}

	if(id->title == nil || id->album == nil)
		return nil;

	return id;
}

ID3v1*
readid3(int fd)
{
	Dir *d;
	ID3v1 *id;
	char buf[128];
	char fieldbuf[31];
	int length;

	d = dirfstat(fd);
	if(d == nil)
		return nil;
	length = d->length;
	free(d);

	if(pread(fd, buf, 128, length-128) != 128)
		return nil;

	id = emalloc(sizeof(ID3v1));
	if(memcmp(buf, "TAG", 3) != 0){
		fillnotag(id, fd);
		return id;
	}

	memcpy(fieldbuf, buf+3, 30);
	fieldbuf[30] = '\0';
	id->title = emalloc(sizeof(Rune) * 31);
	runesprint(id->title, "%s", fieldbuf);

	memcpy(fieldbuf, buf+33, 30);
	id->artist = emalloc(sizeof(Rune) * 31);
	runesprint(id->artist, "%s", fieldbuf);

	memcpy(fieldbuf, buf+63, 30);
	id->album = emalloc(sizeof(Rune) * 31);
	runesprint(id->album, "%s", fieldbuf);

	memcpy(fieldbuf, buf+93, 4);
	fieldbuf[4] = '\0';
	id->year = atoi(fieldbuf);

	memcpy(fieldbuf, buf+97, 30);
	fieldbuf[30] = '\0';
	id->comment = emalloc(sizeof(Rune) * 31);
	runesprint(id->comment, "%s", fieldbuf);

	id->genre = buf[127];
	
	return id;
}

void
destroyid3(ID3v1 *id)
{
	free(id->title);
	free(id->artist);
	free(id->album);
	free(id->comment);
	free(id);
}

