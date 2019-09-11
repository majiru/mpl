enum decmsg{
	START,
	STOP,
	PAUSE,
};

/*
* Dec represents a decoder process.
* ctl is the control channel for pausing, starting, and stoping the proc.
*/
typedef struct Dec Dec;
struct Dec{
	int		decpid;
	int		ctlpid;
	Channel	*ctl;
};

/*
* ID3v1 represents the first version of ID3 metainformation.
* The spec does not define character set, so we treat it as 
* UTF8, which should cover most bases.
* See: http://id3.org/ID3v1
*/
typedef struct ID3v1 ID3v1;
struct ID3v1{
	Rune 	*title;
	Rune 	*artist;
	Rune 	*album;
	int		year;
	Rune	*comment;
	char	genre;
};

typedef struct VorbisMeta VorbisMeta;
struct VorbisMeta{
	/* Raw values */
	uint ncom;
	Rune **key;
	Rune **val;

	/* Common Tags */
	Rune *title;
	Rune *artist;
	Rune *album;
	int year;
	int tracknumber;
};

typedef struct FlacPic FlacPic;
struct FlacPic{
		char *mime;
		Rune *desc;
		Point p;
		uvlong size;
		uchar *data;
};

typedef struct FlacMeta FlacMeta;
struct FlacMeta{
	VorbisMeta *com;
	FlacPic *pic;
};


enum metatype{
	FLAC,
	MP3,
	VORBIS,
};

typedef struct Song Song;
struct Song{
	enum metatype type;
	union {
		FlacMeta *fmeta;
		VorbisMeta *vmeta;
		ID3v1 *idmeta;
	};
	char *path;
};

typedef struct Album Album;
struct Album{
	char *path;
	Rune *name;
	Image *cover;
	int nsong;
	Song **songs;
};
