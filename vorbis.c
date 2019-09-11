#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

void
finddefmeta(VorbisMeta *v)
{
	uint i;
	for(i=0;i<v->ncom;i++){
		if(runestrcmp(v->key[i], L"ALBUM") == 0){
			v->album = v->val[i];
			continue;
		}
		if(runestrcmp(v->key[i], L"TITLE") == 0){
			v->title = v->val[i];
			continue;
		}
		if(runestrcmp(v->key[i], L"ARTIST") == 0){
			v->artist = v->val[i];
			continue;
		}
	}
}

VorbisMeta*
parsevorbismeta(int fd, uvlong offset)
{
	u32int size;
	uchar buf[1024];
	uint i;
	VorbisMeta *v;
	char *sep;

	v = emalloc(sizeof(VorbisMeta));

	/* Vendor String */
	pread(fd, buf, 4, offset);
	size = lebtoi(buf, 4);
	offset+=size+4;

	pread(fd, buf, 4, offset);
	v->ncom = lebtoi(buf, 4);
	v->key = emalloc(sizeof(Rune*) * v->ncom);
	v->val = emalloc(sizeof(Rune*) * v->ncom);
	offset+=4;

	for(i=0;i<v->ncom;i++){
		pread(fd, buf, 4, offset);
		size = lebtoi(buf, 4);
		offset+=4;

		pread(fd, buf, size, offset);
		offset+=size;

		buf[size] = '\0';
		sep = strchr((char*)buf, '=');
		if(sep == nil){
			fprint(2, "Invalid vorbis header format\n");
			quit("Invalid vorbis header format");
			continue;
		}
		*sep = '\0';
		v->key[i] = runesmprint("%s", (char*)buf);
		v->val[i] = runesmprint("%s", sep+1);
	}

	finddefmeta(v);
	return v;
}
