/* $Id: 2mag.c,v 1.3 2003/02/08 13:32:37 niki Exp $ */
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

/*Cygwin hack*/
#if !defined(O_BINARY)
	#define O_BINARY 0
#endif



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
					 sscanf(puff,"%2i%2i%2i.%2hi",&tm.tm_hour,&tm.tm_min,&tm.tm_sec,(short int*)&trk->centisecs);
					 break;
				case  9: trk->validity=!strncmp("A",puff,1); ret=1; break;
				case 11: sscanf(puff,"%2i%2i%2i",&tm.tm_mday,&tm.tm_mon,&tm.tm_year);
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

void parsemagconfile(struct pdb *pdb, const int fd, const char* fname){ /*{{{1*/
	struct trkrec trk;
	struct maghead hdr = {13, "4D533336 MS", "36",2};
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

#define AppId                   'Mag1'
#define Data                    'DATA'

int main(int argcount,char* arg[]){ /*{{{1*/
	struct stat sta;
	int fd,fd2;
	char *defname="MacCon";
	char *name;
	struct pdb *pdb;

	memset(&sta,0,sizeof(struct stat));
	if(argcount>=4){name=arg[3];} else {name=defname;}

	if (argcount>=3){
		if(stat(arg[1],&sta)==0 && S_ISREG(sta.st_mode)){
			fd=open(arg[1],O_RDONLY);
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

				fd2=open(arg[2],O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IRUSR|S_IWUSR);
				if(fd2!=-1){
					parsemagconfile(pdb,fd2,name);
					close(fd2);

				} else {
					perror(NULL);
				}
			}
			close(fd);
		} else {
			perror(NULL);
		}
	} else {
		printf("\nUsage: %s <inputfile> <ouputfile> [Trackname]\n\n",arg[0]);
	}

	return 0;
}

/* vim: set fdm=indent:*/
