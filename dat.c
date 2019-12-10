#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>

#include "dat.h"
#include "fncs.h"

uvlong
string2hash(char *s)
{
	int i;
	uvlong hash;
	hash = 7;
	for(i=0;i<strlen(s);i++)
		hash = hash*31 + s[i];
	return hash;
}

Hmap*
allocmap(int size)
{
	Hmap *h = emalloc(sizeof(Hmap));
	h->size = size;
	h->nodes = emalloc(sizeof(Hnode)*size);
	return h;
}

void
mapinsert(Hmap *h, char *key, void *val)
{
	Hnode *n;

	wlock(h);
	n = h->nodes+(string2hash(key)%h->size);
	assert(n != nil);
	for(;n->next!=nil || n->key!=nil ;n=n->next)
		if(strcmp(key, n->key) == 0){
			/* update value */
			n->val = val;
			wunlock(h);
			return;
		}

	/* Set existing free node */
	if(n->key == nil){
		n->key = strdup(key);
		n->val = val;
		wunlock(h);
		return;
	}

	/* create new node */
	n->next = emalloc(sizeof(Hnode));
	n->next->key = strdup(key);
	n->next->val = val;
	wunlock(h);
}

void*
mapget(Hmap *h, char *key)
{
	Hnode *n;
	rlock(h);
	n = h->nodes+(string2hash(key)%h->size);
	for(;n!=nil;n=n->next){
		if(n->key == nil)
			continue;
		if(strcmp(key, n->key) == 0){
			runlock(h);
			return n->val;
		}
	}

	runlock(h);
	return nil;
}

int
mapdel(Hmap *h, char *key)
{
	Hnode *n;
	wlock(h);
	n = h->nodes+(string2hash(key)%h->size);
	for(;n!=nil;n=n->next){
		if(n->key == nil)
			continue;
		if(strcmp(key, n->key) == 0){
			free(n->key);
			n->key = nil;
			wunlock(h);
			return 1;
		}
	}

	wunlock(h);
	return 0;
}

int
mapdump(Hmap *h, void **buf, int size)
{
	Hnode *n;
	int i, c;

	rlock(h);
	for(i=c=0;i<h->size;i++)
		for(n=h->nodes+i;n!=nil && n->key!=nil;n=n->next){
			if(c >= size)
				return c;
			buf[c++] = n->val;
		}
	runlock(h);
	return c;
}

int
mapdumpkey(Hmap *h, char **buf, int size)
{
	Hnode *n;
	int i, c;

	rlock(h);
	for(i=c=0;i<h->size;i++)
		for(n=h->nodes+i;n!=nil && n->key!=nil;n=n->next){
			if(c >= size)
				return c;
			buf[c++] = n->key;
		}
	runlock(h);
	return c;
}

void
mapclear(Hmap *h)
{
	Hnode *n;
	int i;

	wlock(h);
	for(i=0;i<h->size;i++)
		for(n=h->nodes+i;n!=nil;n=n->next)
			if(n->key != nil){
				free(n->key);
				n->key=nil;
			}
	wunlock(h);
}

void
freemap(Hmap *h)
{
	mapclear(h);
	free(h->nodes);
	free(h);
}