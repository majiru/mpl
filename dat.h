enum cmsg{
	NEXT,
	PREV,
	STOP,
	START,
	PAUSE,
	DUMP,
};

enum volmsg{
	UP,
	DOWN,
	MUTE,
	UNMUTE,
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
	int	year;
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
	RADIO,
};

typedef struct Song Song;
struct Song{
	enum metatype type;
	union {
		FlacMeta *fmeta;
		VorbisMeta *vmeta;
		ID3v1 *idmeta;
		Rune *title;
	};
	char *path;
};

typedef struct Album Album;
struct Album{
	Rune *name;
	Image *cover;
	int nsong;
	Song *songs;

	int nocover;
};

typedef struct Lib Lib;
struct Lib{
	int nalbum, cursong;
	Album *start, *stop, *cur;
	char *name;
};

typedef struct Click Click;
struct Click{
	Rectangle r;
	Album *a;
	int songnum;
};

/*
 * Simple hashmap implementation.
 * Hnode key must be non nil.
 */
typedef struct Hmap Hmap;
typedef struct Hnode Hnode;
struct Hmap{
	RWLock;
	int size;
	int (*hashfn)(void*);
	struct Hnode{
		char *key;
		void *val;
		Hnode *next;
	} *nodes;
};
