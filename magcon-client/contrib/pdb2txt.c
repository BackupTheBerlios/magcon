/* convert a pdb file, that contains consists of records being lines
 * of text into an ASCII file */
#include <stdio.h>
#define PDBSIZE 65537

void usage() {
    fprintf(stderr,"Usage: pdb2txt pdbfile [<outfile>|-] Converts a Palm PDB database consisting of text lines into a text file.  If no outfile name is specified, the name is constructed from the PDB file name. If '-', stdout is used.\n");
}

int main(int argc, char **argv) {
    FILE *fpdb;
    FILE *outf;
    unsigned char buf[PDBSIZE];
    int len, recnum, recix;
    char *ptr;
    char outname[1024];

    if (argc < 2 || argc > 3 || (strcmp(argv[1],"-h") == 0) 
	|| (strcmp(argv[1],"--help") == 0)) {
	usage();
	return 1;
    }

    if (argc == 2) { 
	ptr = strrchr(argv[1],'/');
	if (ptr == NULL) {
	    strcpy(outname,argv[1]);
	} else {
	    strcpy(outname,ptr+1);
	}
	ptr = strrchr(outname,'.');
	if (ptr != NULL) {
	    *ptr = '\0';
	}
	strcat(outname,".txt");
    } else {
	strcpy(outname,argv[2]);
    }

    if (strcmp(outname,"-") == 0) {
	outf = stdout;
    } else {
	if ((outf = fopen(outname,"w")) == NULL) {
	perror(argv[1]);
	return 1;
	}
    }
    
    if ((fpdb  = fopen(argv[1], "rb")) == NULL) {
	perror(argv[1]);
	return 1;
    }
    len = fread(buf,1,PDBSIZE,fpdb);
    buf[len] = '\0';
    if (len <= 78) {
	fprintf(stderr,"PDB file is too small\n");
	return 1;
    }
    recnum = buf[76]*256+buf[77];
    recix = recnum*8+78;
    if (buf[recix] == 0 && buf[recix+1] == 0) {
	recix = recix+2;
    }
    while (recix < len) {
	if (strrchr(buf+recix,'\n') != 0) { // no need for newline
	    fprintf(outf,"%s",buf+recix);
	} else {
	    fprintf(outf,"%s\n",buf+recix);
	}
	recix = recix+strlen(buf+recix)+1;
    }
    return 0;
}
