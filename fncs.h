/* dec.c */
Dec* spawndecoder(char*);

/* mpl.c */
void	quit(char*);

/* util.c */
void*	emalloc(vlong);
u64int	bebtoi(uchar*,int);
u64int	lebtoi(uchar*,int);

/* id3.c */
ID3v1*	readID3v1(int);
void	destroyID3v1(ID3v1 *id);

/* vorbis.c */
VorbisMeta*	parseVorbisMeta(int, uvlong);

/* flac.c */
FlacMeta*	readFlacMeta(int);
void		convFlacPic(FlacPic*);
