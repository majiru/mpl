/* dec.c */
void	spawndec(Channel**,Channel**,Channel**);

/* mpl.c */
void	quit(char*);

/* util.c */
void*	emalloc(vlong);
u64int	bebtoi(uchar*,int);
u64int	lebtoi(uchar*,int);
void	kill(int);
int		runecstrcmp(Rune*,Rune*);

/* id3.c */
ID3v1*	readid3(int);
void	destroyid3(ID3v1 *id);

/* vorbis.c */
void	parsevorbismeta(int, uvlong, VorbisMeta*);

/* flac.c */
FlacMeta*	readflacmeta(int, int);

/* draw.c */
Point	drawalbum(Album*, Image*, Image*, Point, int, Channel*);
Image*	convpic(int, char*);
Image*	convpicbuf(uchar*, uvlong, char*);
void	drawlibrary(Lib*, Point, Image*, Image*, Channel*);
void	drawvolume(int, Image*);
void	drawlists(Point,Image*,Image*,Channel*);

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
void	freemap(Hmap*);
int		mapdump(Hmap*,void**,int);
int		mapdumpkey(Hmap*,char**,int);

/* lib.c */
void	spawnlib(Channel*,Channel*,Channel*,Channel*,Channel*,char*);

/* vol.c */
void	spawnvol(Channel*,Channel*);

/* event.c */
void	spawnevent(Channel*,Channel*,Channel*,Channel*);

/* index.c */
void	marshalstr(int,char*);
char*	unmarshalstr(int);
void	marshalrune(int,Rune*);
Rune*	unmarshalrune(int);
void	marshalalbum(int,Album*);
void	unmarshalalbum(int,Album*);
void	marshallib(int,Lib*);
void	unmarshallib(int,Lib*);

/* list.c */
void	libdir(char*,int);
void	dumplib(Lib*);
void	loadlib(Lib*);
