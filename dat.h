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
	uint ncom;
	Rune **key;
	Rune **val;
};

typedef struct FlacPic FlacPic;
struct FlacPic{
		char *mime;
		Rune *desc;
		Point p;
		uvlong size;
		uchar *data;
		Image *i;
};

typedef struct FlacMeta FlacMeta;
struct FlacMeta{
	VorbisMeta *com;
	FlacPic *pic;
};