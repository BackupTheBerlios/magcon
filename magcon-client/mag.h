
struct maghead{
	unsigned char length;		/*13*/
	unsigned char signature[11];	/*"4D533336 MS"*/
	unsigned char version[2];	/*"36"*/
	unsigned int type;		/*2 -Track*/
}__attribute__((packed));

struct trkrec{
	double x;
	double y;
	float altitude;
	time_t time; 			/*time.h*/
	int validity;
	unsigned char centisecs;
}__attribute__((packed));
	
