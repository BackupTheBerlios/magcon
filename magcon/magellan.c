/* $Id: magellan.c,v 1.8 2003/02/24 18:10:25 niki Exp $ */
#include <PalmOS.h>
#include <Progress.h>
#include <DataMgr.h>
#include <SerialMgrOld.h>
#include "resource.h"
#include "ser.h"

typedef enum _msgtype {track=1,waypt} msgtype;

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

void magchksum (char *data, char* chksum,UInt32 size){
	UInt16 CheckVal;
	int           len, idx;
	
	CheckVal = 0;
	len = StrLen (data) - 1;  //Don't include the '*' at end of message!

	//idx starts at 1 becuase the '$' isn't part of the checksum!
	for (idx = 1; idx < len && data[idx]!='*'; idx++) {
		CheckVal = CheckVal ^ data[idx];
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

static char* input_name(void){
	FormPtr frmP;
	FieldType *fld;
	static char name[16];
	char* tmp;
	
	MemSet(name,0,16);
	frmP = FrmInitForm(NameForm);
	fld=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, FLD_NAME));
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, FLD_NAME));
	FrmDoDialog(frmP);
	tmp=FldGetTextPtr(fld);
	if(tmp){
		StrNCopy(name,tmp,15);
	}
	FrmDeleteForm(frmP);

	return name;
}
/* Get the ID of the magellanunit */
void get_mag_id(void){
	char *msr="$PMGNCMD,VERSION*";
	char msg[512];
	char field[512];

	if(ser_open()){
		send_string(msr,false);
		if(get_string(msg,512)){
			get_field(msg,field,512,4);
			StrCat(field,"  ");
			get_field(msg,field+StrLen(field),512-StrLen(field),3);
			FrmCustomAlert(INF_DLG1,"Found Magellan:",field," ");
			/*WinDrawChars(field,StrLen(field),2,105);*/
		} else {
			FrmCustomAlert(ALM_DLG1,"Could not get Magellan ID"," "," ");
		}
		ser_close();
	}
}

/* Callback for progressdialog */
static Boolean progress_cb (PrgCallbackDataPtr cbp){
	StrPrintF(cbp->textP,"Got %i Waypoints",cbp->stage);
	return true;
}

static void addrecord(DmOpenRef dbref, char* msg){
	MemHandle mh;
	MemPtr    mp;
	UInt16 pos = dmMaxRecordIndex;
	
	mh = DmNewRecord(dbref, &pos, (UInt32) StrLen(msg)+1);
	if(mh==0){
		ErrAlert(DmGetLastErr());
		return;
	}
	mp = MemHandleLock(mh);

	DmWrite(mp, 0, msg,(UInt32) StrLen(msg)+1);
	
	MemHandleUnlock(mh);
	DmReleaseRecord(dbref, pos, true);
}

static Boolean get_data(msgtype type, DmOpenRef dbref){
	char *handon="$PMGNCMD,HANDON*";
	char *handoff="$PMGNCMD,HANDOFF*";
	char *msgtrack="$PMGNCMD,TRACK,2*";
	char *msgwaypt="$PMGNCMD,WAYPOINT*";
	char *msr;
	char msg[1024];
	char field[512];
	char chk[3];
	char chk2[3];
	UInt16 timeout;
	
	Boolean done,complete;
	ProgressPtr progptr;
	UInt16 msgcount;
	Boolean cancelled;
	EventType evt;

	done=false;
	complete=false;
	msgcount=0;
	cancelled=false;

	switch(type){
		case track: msr=msgtrack; break;
		case waypt: msr=msgwaypt; break;
		default: return false;
	}

	if(!ser_open()){return false;}

	if(!send_string(handon,false)){ser_close(); return false;} 
	if(!send_string(msr,true)){ send_string(handoff,true); ser_close(); return false;}
	
	/* The real McCoy - Get everything the Magellan wants to give */
	timeout=SysSetAutoOffTime(0);
	progptr=PrgStartDialogV31(msr,progress_cb);
	while(!done && progptr){
		MemSet(msg,1024,0);
		if(get_string(msg,1024)){
			chk[2]=0; chk2[2]=0;
			get_checksum(msg,chk,3);
			magchksum(msg,chk2,3);
			StrPrintF(field,"$PMGNCSM,%s*",chk);
			send_string(field,false);
			if(StrCompare(chk,chk2)!=0) break;
			/*FrmCustomAlert(ALM_DLG1,msg,field," ");*/
			EvtGetEvent(&evt,10);
			while(evt.eType!=nilEvent){
				if(!PrgHandleEvent(progptr,&evt)){
					FrmCustomAlert(ALM_DLG1,"Sorry!","You can't cancel.","It upsets the Magellan");
				}
				EvtGetEvent(&evt,1);
			}
			PrgUpdateDialog(progptr,0,msgcount++,NULL,true);
			if(StrNCompare(msg,"$PMGNCMD,END*",13)==0){
				done=true;
				complete=true;
			} else {
				addrecord(dbref,msg);
			}
		} else {
			done=true;
		}
	}
	send_string(handoff,true);
	SerReceiveFlush(serlib,SysTicksPerSecond()/2);
	SysSetAutoOffTime(timeout);
	
	if(progptr){PrgStopDialog(progptr,false);}
	ser_close();
	
	return complete;
}

/* Get the waypoints from PS*/
void mag_get_data(msgtype msgt){
	char*t="";
	LocalID dbid;
	DmOpenRef dbref;
	UInt16 vers=DbVers;
	UInt16 newdbattr=dmHdrAttrBackup;

	t=input_name();
	if(StrLen(t)==0){return;}

	if(DmCreateDatabase(0,t,AppId,Data,false)!=errNone){ 
		FrmCustomAlert(ALM_DLG1,"Could not create Database!"," "," ");
		return;
	}
	
	dbid=DmFindDatabase(0,t);
	if(dbid==0){
		FrmCustomAlert(ALM_DLG1,"Could not find Database!"," "," ");
		return;
	}
	
	DmSetDatabaseInfo(0,dbid,NULL,&newdbattr,&vers,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	
	dbref=DmOpenDatabase(0,dbid,dmModeWrite|dmModeExclusive);
	if(dbref==NULL){
		FrmCustomAlert(ALM_DLG1,"Could not open Database!"," "," ");
		return;
	}

	if(get_data(msgt,dbref)){
		DmCloseDatabase(dbref);
		FrmCustomAlert(INF_DLG1,"Saved database:",t," ");
	} else{
		DmCloseDatabase(dbref);
		DmDeleteDatabase(0,dbid);
		FrmCustomAlert(ALM_DLG1,"Could not get waypoints!"," "," ");
	}
}

/* vim: set fdm=indent: */
