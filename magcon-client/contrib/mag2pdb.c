/* mag2pdb: Converts an ASCII file into a PDB database for the
 * Palm. Can be used for uploading waypoint, route, and track files to
 * a Magellan GPS using the magcon program. */
#include <stdio.h>
#include <sys/time.h>

#define PDBSIZE 65537

void usage() {
    fprintf(stderr,"Usage: mag2pdb <file> [<pdb-file>|-] Translates an ASCII file with Magellan track, route, and waypoint information into a PDB database for the Palm.\n");
}

put2b(unsigned char *ptr, unsigned int val) {
    *ptr = val/256;
    *(++ptr) = val & 255;
}

put4b(unsigned char *ptr, unsigned int val) {
    *ptr = (val>>24) & 255 ;
    *(++ptr) = (val>>16) & 255;
    *(++ptr) = (val>>8) & 255;
    *(++ptr) = val & 255;
}


int main(int argc, char **argv) {
    FILE *fpdb;
    FILE *fmag;
    unsigned char outbuf[PDBSIZE],inbuf[PDBSIZE];
    int len, recnum;
    char outname[1024];
    char pdbname[1024];
    char *ptr;
    int i, ix, recix, linelen;
    char *pdir, *prec, *pline, *peol;
    long secs;

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
	strcat(outname,".pdb");
    } else {
	strcpy(outname,argv[2]);
    }

    if (strcmp(outname,"-") == 0) {
	fpdb = stdout;
    } else {
	if ((fpdb = fopen(outname,"wb")) == NULL) {
	perror(argv[1]);
	return 1;
	}
    }
    if ((fmag  = fopen(argv[1], "rb")) == NULL) {
	perror(argv[1]);
	return 1;
    }
    len = fread(inbuf,1,PDBSIZE-1,fmag);
    if (len == PDBSIZE-1) {
	fprintf(stderr,"File to large for conversion\n");
    }
    inbuf[len] = '\0';

    ptr = strrchr(outname,'/');
    if (ptr == NULL) {
	strcpy(pdbname,outname);
    } else {
	strcpy(pdbname,ptr+1);
    }
    ptr = strrchr(pdbname,'.');
    if (ptr != NULL) {
	*ptr = '\0';
    }
    pdbname[31] = '\0';

    memset(outbuf,'\0',PDBSIZE);
    /* Create DB header */

    /* DB name: 32 bytes*/
    strcpy(outbuf,pdbname);
    /* flags: 2 bytes */
    /* version: 2 bytes */
    /* creation time: 4 bytes */
    secs = 2082844800L + time(NULL);
    put4b(outbuf+36,secs);
    /* modified time: 4 bytes */
    put4b(outbuf+40,secs);
    /* backup time: 4 bytes */
    /* modified number: 4 bytes */
    /* application info size: 4 bytes */
    /* sort info size: 4 bytes */
    /* type: 4 bytes */
    strcpy(outbuf+60,"DATA");
    /* creator ID: 4 bytes */
    strcpy(outbuf+64,"Mag1");
    /* 8 zeros */
    
    /* count records: */
    recnum = 0;
    for (i=0;i<len;i++) {
	if (inbuf[i] == '\n') {
	    recnum++;
	}
    }
    /* store record number: 2 bytes */
    put2b(outbuf+76,recnum);

    /* recheck size: */
    if (len + 9*recnum + 78 > PDBSIZE-2) {
	fprintf(stderr,"File to large for conversion\n");
	return 1;
    }

    /* now compute the toc table, and fill the records */
    ix = 1;
    pdir = outbuf+78;
    pline = inbuf;
    recix = 78+recnum*8+2;    
    prec = outbuf+recix;
    while (ix <= recnum) {
	peol = strchr(pline,'\n');
	if (peol == NULL) {
	    fprintf(stderr,"Cannot find LF in input file. Format error?\n");
	    return 1;
	}
	linelen = peol-pline+1;
	put4b(pdir,recix);
	put2b(pdir+6,ix);
	strncpy(prec,pline,linelen);

	/* update values */
	ix++;
	pdir  = pdir + 8;
	pline = pline + linelen;
	recix = recix + linelen+1;
	prec  = prec + linelen+1;
    }
    fwrite(outbuf,1,recix,fpdb);
    return 0;
}
