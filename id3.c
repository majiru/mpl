#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

ID3v1*
readID3v1(int fd)
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
	
	if(memcmp(buf, "TAG", 3) != 0)
		return nil;

	id = emalloc(sizeof(ID3v1));

	memcpy(fieldbuf, buf+3, 30);
	fieldbuf[30] = '\0';
	id->title = emalloc(sizeof(Rune) * utflen(fieldbuf) + 1);
	runesprint(id->title, "%s", fieldbuf);

	memcpy(fieldbuf, buf+33, 30);
	id->artist = emalloc(sizeof(Rune) * utflen(fieldbuf) + 1);
	runesprint(id->artist, "%s", fieldbuf);

	memcpy(fieldbuf, buf+63, 30);
	id->album = emalloc(sizeof(Rune) * utflen(fieldbuf) + 1);
	runesprint(id->album, "%s", fieldbuf);

	memcpy(fieldbuf, buf+93, 4);
	fieldbuf[4] = '\0';
	id->year = atoi(fieldbuf);

	memcpy(fieldbuf, buf+97, 30);
	fieldbuf[30] = '\0';
	id->comment = emalloc(sizeof(Rune) * utflen(fieldbuf) + 1);
	runesprint(id->comment, "%s", fieldbuf);

	id->genre = buf[127];
	
	return id;
}

void
destroyID3v1(ID3v1 *id)
{
	free(id->title);
	free(id->artist);
	free(id->album);
	free(id->comment);
	free(id);
}
