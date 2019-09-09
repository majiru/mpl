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

FlacPic*
readFlacPic(int fd, vlong offset)
{
	uchar buf[1024];
	uint len;
	FlacPic *pic;

	pic = emalloc(sizeof(FlacPic));
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

	pread(fd, buf, len, offset);
	buf[len] = '\0';
	pic->desc = runesmprint("%s", (char*)buf);
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

	return pic;
}


typedef struct{
	int fd;
	uvlong size;
	uchar *data;
} WriteArg;

void
picwriteproc(void *arg)
{
	int i, n;
	WriteArg *a = arg;
	int fd = a->fd;
	uchar *data = a->data;
	uvlong towrite = a->size;

	free(a);
	for(i = 0;towrite>0;){
		n = write(fd, data+i, towrite);
		i+=n;
		towrite-=n;
	}
}

typedef struct{
	int fdin;
	int fdout;
	Channel *cpid;
	char *cmd;
} ExecArg;

void
picexecproc(void *arg)
{
	ExecArg *a = arg;
	dup(a->fdin, 0);
	dup(a->fdout, 1);
	procexecl(a->cpid, a->cmd, a->cmd, "-c", nil);
}

void
picresizeproc(void *arg)
{
	ExecArg *a = arg;
	dup(a->fdin, 0);
	dup(a->fdout, 1);
	procexecl(a->cpid, "/bin/resize", "resize", "-x", "256", "-y", "256", nil);
}

char*
mime2bin(char *mime)
{
	char *slash = strchr(mime, '/');
	if(slash==nil)
		return nil;
	slash+=1;

	if(strcmp(slash, "jpeg") == 0)
		return strdup("/bin/jpg");
	if(strcmp(slash, "png") == 0)
		return strdup("/bin/png");

	return nil;
}

void
convFlacPic(FlacPic *pic)
{
	ExecArg *e;
	WriteArg *w;
	int convin[2];
	int convout[2];
	int resize[2];

	pipe(convin);
	pipe(convout);
	pipe(resize);

	w = emalloc(sizeof(WriteArg));
	w->fd = convin[0];
	w->data = pic->data;
	w->size = pic->size;
	procrfork(picwriteproc, w, 8192, RFFDG);
	/* other proc frees w */

	e = emalloc(sizeof(ExecArg));
	e->cpid = chancreate(sizeof(int), 0);
	e->fdin = convin[1];
	e->fdout = convout[0];
	e->cmd = mime2bin(pic->mime);
	procrfork(picexecproc, e, 8192, RFFDG);
	recv(e->cpid, nil);
	chanfree(e->cpid);
	free(e->cmd);
	free(e);

	e = emalloc(sizeof(ExecArg));
	e->cpid = chancreate(sizeof(int), 0);
	e->fdin = convout[1];
	e->fdout = resize[0];
	procrfork(picresizeproc, e, 8192, RFFDG);
	recv(e->cpid, nil);
	chanfree(e->cpid);
	free(e);

	pic->i = readimage(display, resize[1], 0);
	close(convin[0]);
	close(convin[1]);
	close(convout[0]);
	close(convout[1]);
	close(resize[0]);
	close(resize[1]);
}

FlacMeta*
readFlacMeta(int fd)
{
	uvlong off;
	char type;
	FlacMeta *f;
	uchar buf[32];

	pread(fd, buf, 4, 0);

	if(memcmp(buf, "fLaC", 4) != 0){
		return nil;
	}
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
			f->com = parseVorbisMeta(fd, off);
			break;
		case CUESHEET:
			break;
		case PICTURE:
			f->pic = readFlacPic(fd, off);
			break;
		}
		off+=bebtoi(buf, 3);
		if((type & 0x80) > 0)
			break;
	}

	return f;
}