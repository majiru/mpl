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
Point	drawalbum(Album*, Image*, Image*, Point, int);
Image*	convpic(int, char*);
Image*	convpicbuf(uchar*, uvlong, char*);
void	drawlibrary(Album*, Album*, Album*, Image*, Image*, int);

/* dir.c */
void	dir2album(Album*,char*);
int		parselibrary(Album**,char*);