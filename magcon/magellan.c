/* $Id: magellan.c,v 1.1 2003/02/02 10:41:16 niki Exp $ */
#include <PalmOS.h>

static void strtohex(UInt32 data,char* puffer,UInt32 size){
	char puffer1[512];
	int i,j;
	Boolean ever;
	
	j=0;
	ever=false;
	
	StrIToH(puffer1,data);
	for(i=0;i<StrLen(puffer1)&&j<size;i++){
		if(puffer1[i]!='0' || ever){
			puffer[j++]=puffer1[i];
			ever=true;
		}
	}
}

void magchksum (char *Data, char* chksum,UInt32 size){
	UInt16 CheckVal;
	int           len, idx;
	
	CheckVal = 0;
	len = StrLen (Data) - 1;  //Don't include the '*' at end of message!

	//idx starts at 1 becuase the '$' isn't part of the checksum!
	for (idx = 1; idx < len; idx++) {
		CheckVal = CheckVal ^ Data[idx];
	}
	strtohex(CheckVal,chksum, size);
}

void get_field(char* data,char* field,UInt32 fieldsize, UInt16 fieldno){
	UInt16 i,j;
	UInt16 curfield;
	
	j=0;
	curfield=1;
	
	for(i=0;i<StrLen(data)&&curfield<=fieldno;i++){
		if (data[i]=='*') break;
		if (curfield==fieldno && data[i]!=','&&i<fieldsize) field[j++]=data[i];
		if (data[i]==',') curfield++;
	}
	field[j]=0;
		
}

void get_checksum(char* data,char* field,UInt32 fieldsize){
	UInt16 i,j;
	
	i=0;
	j=StrLen(data);
	while(i<j && data[i++]!='*');
	if (fieldsize>2 && i<j-2){
		field[0]=data[i];
		field[1]=data[i+1];
		field[2]=0;
	}
}

/* vim: set fdm=indent: */
