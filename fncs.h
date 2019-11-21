/* dec.c */
void	spawndec(Channel**,Channel**,Channel**);

/* mpl.c */
void	quit(char*);

/* util.c */
void*	emalloc(vlong);
u64int	bebtoi(uchar*,int);
u64int	lebtoi(uchar*,int);
void	kill(int);
void	killgrp(int);
int		runecstrcmp(Rune*,Rune*);

/* id3.c */
ID3v1*	readid3(int);
void	destroyid3(ID3v1 *id);

/* vorbis.c */
VorbisMeta*	parsevorbismeta(int, uvlong);

/* flac.c */
FlacMeta*	readflacmeta(int, int);

/* draw.c */
Point	drawalbum(Album*, Image*, Image*, Point, int, Channel*);
Image*	convpic(int, char*);
Image*	convpicbuf(uchar*, uvlong, char*);
void	drawlibrary(Album*, Album*, Album*, Image*, Image*, int, Channel*);
void	drawvolume(int, Image*);

/* dir.c */
int		file2song(Song*, char*,int);
int		dir2album(Album*,char*);
int		parselibrary(Album**,char*);
void	radio2album(Album*,char*);
void	file2album(Album*,Rune*,char*);

/* dat.c */
Hmap*	allocmap(int);
void	mapinsert(Hmap*,char*,void*);
int		mapdel(Hmap*,char*);
void*	mapget(Hmap*,char*);

/* lib.c */
void	spawnlib(Channel*,Channel*,Channel*,Channel*,Channel*,char*);

/* vol.c */
void	spawnvol(Channel*,Channel*);

/* event.c */
void	spawnevent(Channel*,Channel*,Channel*,Channel*);

/* index.c */
void	marshalstr(int,char*);
void	unmarshalstr(int,char**);
void	marshalrune(int,Rune*);
void	unmarshalrun(int,Rune**);
void	marshalalbum(int,Album*);
void	unmarshalalbum(int,Album*);
void	marshallib(int,Lib*);
void	unmarshallib(int,Lib*);

/* list.c */
void	dumplib(Lib*);
void	loadlib(Lib*);
