/* $Id: 2gpx.c,v 1.4 2003/08/24 12:48:30 niki Exp $ */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mag.h"
#include "coldsync/palm.h"
#include "coldsync/pdb.h"

#define PUFFSIZE 256
#define AppId                   'Mag1'
#define Data                    'DATA'

/*Cygwin hack*/
#if !defined(O_BINARY)
	#define O_BINARY 0
#endif

typedef void(*_parsemagconffile)(struct pdb *pdb, const int fd);

static double mag2degrees(double mag_val) /*{{{1*/
{
        double minutes;
        double tmp_val;
        double return_value;
        int deg;

        tmp_val = mag_val / 100.0;
        deg = (int) tmp_val;
        minutes = (tmp_val - deg) * 100.0;
        minutes /= 60.0;
        return_value = (double) deg + minutes;
        return return_value;
	
} 

static int parsemagdatastring(const char *data, struct trkrec *trk){ /*{{{1*/
        unsigned int field;
	unsigned int ret;
	char puff[PUFFSIZE];
	char *puffptr;
	struct tm tm;


        field=0;
	ret=0;
	memset(puff,0,PUFFSIZE);
	puffptr=puff;
	
	if(data[0]=='#'){return ret;}
	
        do{
                if(*data==',' || *data=='*') {
			switch (++field) {
				case  2: trk->y=mag2degrees(atof(puff)); break;
				case  3: if(strncmp("N",puff,1)==0){trk->y=-trk->y;} break;
				case  4: trk->x=mag2degrees(atof(puff)); break;
				case  5: if(strncmp("W",puff,1)==0){trk->x=-trk->x;} break;
				case  6: trk->altitude=atof(puff); break;
				case  8: memset(&tm,0,sizeof(struct tm));
					 sscanf(puff,"%2u%2u%2u.%2hu",&tm.tm_hour,&tm.tm_min,&tm.tm_sec,(short int*)&trk->centisecs);
					 break;
				case  9: trk->validity=!strncmp("A",puff,1); ret=1; break;
				case 11: sscanf(puff,"%2u%2u%2u",&tm.tm_mday,&tm.tm_mon,&tm.tm_year);
					 tm.tm_mon-=1;
					 tm.tm_year+=100;
					 trk->time=mktime(&tm); 
					 break;
			}
			memset(puff,0,PUFFSIZE);
			puffptr=puff;
                } else {
			if((char*)puffptr < (char*)&puff[PUFFSIZE]) *puffptr++=*data;
                }
		++data;
        }while(*data!='\r' && *data!='\n' && *data!='\0');

        return ret;
}

void parsemagconfileimapsend(struct pdb *pdb, const int fd){ /*{{{1*/
        struct trkrec trk;
        struct maghead hdr = {13, "4D533336 MS", "36",2};
	char *fname="magcon";
        int j,reccount;
        char buff[PUFFSIZE];
        unsigned char len;
        struct pdb_record *pdb_rec;

        memset(buff,0,PUFFSIZE);
        j=0;
        reccount=0;

        strcat(buff,fname);
        len=strlen(buff);
        write(fd,&hdr,sizeof(struct maghead));
        write(fd,&len,sizeof(len));
        write(fd,buff,len);
        write(fd,&j,sizeof(int));

        for(pdb_rec = pdb->rec_index.rec; pdb_rec; pdb_rec=pdb_rec->next) {

                if(parsemagdatastring((char*)pdb_rec->data,&trk)){
                        write(fd,&trk,sizeof(struct trkrec));
                        reccount++;
                }
        }
        lseek(fd,sizeof(struct maghead)+sizeof(len)+len,SEEK_SET);
        write(fd,&reccount,sizeof(int));
}

void parsemagconfilecsv(struct pdb *pdb, const int fd){ /*{{{1*/
	struct trkrec trk;
	int reccount;
        struct pdb_record *pdb_rec;
	char buff[1024];
	char buff2[1024];
	char *trkpt="%f,%f,%f,%s.%dZ\n";
	reccount=0;

	for(pdb_rec = pdb->rec_index.rec; pdb_rec; pdb_rec=pdb_rec->next) {
	
		if(parsemagdatastring((char*)pdb_rec->data,&trk)){
			memset(buff,0,1024);
			strftime(buff2,1024,"%Y-%m-%dT%H:%M:%S",gmtime(&trk.time));
			sprintf(buff,trkpt,-trk.y,trk.x,trk.altitude,buff2,trk.centisecs/3*5);
			write(fd,buff,strlen(buff));
			reccount++;
		}
	}
}
void parsemagconfilegpx(struct pdb *pdb, const int fd){ /*{{{1*/
	struct trkrec trk;
	int reccount;
        struct pdb_record *pdb_rec;
	char buff[1024];
	char buff2[1024];
	char *gpxhead="<?xml version=\"1.0\"?>\n<gpx version=\"1.0\" creator=\"MagCon 2gpx exporter - http://www.hansche.de/magcon\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
	char *gpxfoot="</trkseg></trk></gpx>\n";
	char *trkpt="<trkpt lat=\"%f\" lon=\"%f\">\n\t<ele>%f</ele>\n\t<time>%s.%dZ</time>\n</trkpt>\n";
	reccount=0;

	write(fd,gpxhead,strlen(gpxhead));
	
	for(pdb_rec = pdb->rec_index.rec; pdb_rec; pdb_rec=pdb_rec->next) {
	
		if(parsemagdatastring((char*)pdb_rec->data,&trk)){
			memset(buff,0,1024);
			strftime(buff2,1024,"%Y-%m-%dT%H:%M:%S",gmtime(&trk.time));
			sprintf(buff,trkpt,-trk.y,trk.x,trk.altitude,buff2,trk.centisecs/3*5);
			write(fd,buff,strlen(buff));
			reccount++;
		}
	}
	
	write(fd,gpxfoot,strlen(gpxfoot));
	
}

int main(int argcount,char* arg[]){ /*{{{1*/
	struct stat sta;
	int fd,fd2;
	struct pdb *pdb;
	_parsemagconffile parsemagconfile;

	memset(&sta,0,sizeof(struct stat));

	if (argcount<4){ 
		printf("\nUsage: %s <track|csv|mapsend> <inputfile> <ouputfile>\n\n",arg[0]); 
		return 1;
	}
	
	if(strcmp("csv",arg[1])==0) parsemagconfile=parsemagconfilecsv;
	else if(strcmp("track",arg[1])==0) parsemagconfile=parsemagconfilegpx;
	else if(strcmp("mapsend",arg[1])==0) parsemagconfile=parsemagconfileimapsend;
	else {printf("Unknown ouputformat \"%s\"\n",arg[1]); return 1;}

	if(stat(arg[2],&sta)==0 && S_ISREG(sta.st_mode)){
		fd=open(arg[2],O_RDONLY|O_BINARY);
		if(fd!=-1){
			if (NULL == (pdb = pdb_Read(fd))) {
				printf("Could not read pdbfile.\n");
				close(fd);
				return 1;
			}

			if ((pdb->creator != AppId) || (pdb->type != Data)) {
				printf("Not a MagCon file.\n");
				close(fd);
				return 1;
			}

			fd2=argcount>=4?open(arg[3],O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IRUSR|S_IWUSR):STDIN_FILENO;
			if(fd2!=-1){
				parsemagconfile(pdb,fd2);
				close(fd2);

			} else {
				perror(NULL);
			}
		}
		close(fd);
	} else {
		perror(NULL);
	}

return 0;
}

/* vim: set fdm=indent:*/
