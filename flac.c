#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

enum blocktype{
	STREAMINFO,
	PADDING,
	APPLICATION,
	SEEKTABLE,
	VORBIS_COMMENT,
	CUESHEET,
	PICTURE,
};

void
readflacpic(int fd, vlong offset, FlacPic *pic)
{
	uchar buf[1024];
	uint len;

	/* We skip the picture type */
	offset+=4;

	pread(fd, buf, 4, offset);
	len = bebtoi(buf, 4);
	offset+=4;

	pread(fd, buf, len, offset);
	buf[len] = '\0';
	pic->mime = strdup((char*)buf);
	offset+=len;

	pread(fd, buf, 4, offset);
	len = bebtoi(buf, 4);
	offset+=4;

	/* This seems to be taking a large chunk of time;
	 * For now we don't need it
	pread(fd, buf, len, offset);
	buf[len] = '\0';
	pic->desc = runesmprint("%s", (char*)buf);
	*/

	offset+=len;

	pread(fd, buf, 4, offset);
	pic->p.x = bebtoi(buf, 4);
	offset+=4;

	pread(fd, buf, 4, offset);
	pic->p.y = bebtoi(buf, 4);
	offset+=4;

	/*We skip color depth and index */
	offset+=8;

	pread(fd, buf, 4, offset);
	pic->size = bebtoi(buf, 4);
	offset+=4;

	pic->data = emalloc(pic->size);
	pread(fd, pic->data, pic->size, offset);
}

FlacMeta*
readflacmeta(int fd, int readpic)
{
	uvlong off;
	char type;
	FlacMeta *f;
	uchar buf[32];

	pread(fd, buf, 4, 0);

	if(memcmp(buf, "fLaC", 4) != 0)
		return nil;
	off=4;

	f = emalloc(sizeof(FlacMeta));
	while(pread(fd, buf, 1, off)){
		type = *buf;
		off++;

		pread(fd, buf, 3, off);
		off+=3;

		switch(type & 0x7f){
		case STREAMINFO:
			break;
		case PADDING:
			break;
		case APPLICATION:
			break;
		case SEEKTABLE:
			break;
		case VORBIS_COMMENT:
			parsevorbismeta(fd, off, f);
			break;
		case CUESHEET:
			break;
		case PICTURE:
			if(readpic > 0)
				readflacpic(fd, off, f);
			break;
		}
		off+=bebtoi(buf, 3);
		if((type & 0x80) > 0)
			break;
	}

	return f;
}
