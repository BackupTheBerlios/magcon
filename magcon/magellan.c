/* $Id: magellan.c,v 1.10 2003/10/05 10:17:52 niki Exp $ */
#include <PalmOS.h>
#include <Progress.h>
#include <DataMgr.h>
#include <SerialMgrOld.h>
#include "magellan.h"
#include "resource.h"
#include "ser.h"
#include "dbdia.h"

static char *elemtype;
static char *operation;

static void strtohex(UInt32 data,char* puffer,UInt32 size){
	char puffer1[512];
	int i,j;
	
	j=size-1;
	// Assumption: The generated hex number is always as long 
	// as size-1 or longer
	StrIToH(puffer1,data);
	for(i=StrLen(puffer1),j=size;j>0;j--,i--){
	    puffer[j-1]=puffer1[i];
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
	
	MemSet(name,16,0);
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

/* Connect to GPS (and get ID string!) */

Boolean gps_connect(char *idstring, UInt16 size) {
    char *msr="$PMGNCMD,VERSION*";
    char *mrep="$PMGNVER"; 
    char *mcsm="$PMGNCSM";
    char *handon="$PMGNCMD,HANDON*";
    Boolean handshake=false;
    char msg[512];
    char field[512];

    idstring[0] = '\0';
    if(!ser_open()){
	return false;
    }
    send_string(msr,false);
    msg[0]='\0';
    if(get_string(msg,512)) {
	if (StrNCompare(msg,mcsm,StrLen(mcsm))==0) {
	    handshake = true;
	    // this is an ack record to our version record: ignore
	    if (!get_string(msg,512)) {
		ser_close();
		return false;
	    }
	}
	if (StrNCompare(msg,mrep,StrLen(mrep))!=0) {
	    FrmCustomAlert(ALM_DLG1,
			   "Expected version record from GPS but got:\n",
			   msg,"");
	    ser_close();
	    return false;
	} else {
	    get_field(msg,field,512,4);
	    StrCat(field,"  ");
	    get_field(msg,field+StrLen(field),512-StrLen(field),3);
	    StrNCopy(idstring,field,size);
	}
    } else {
	ser_close();
	return false;
    }
    if (!handshake) { // if handshake is not enabled yet, do it
	send_string(handon,false);
    }
    return true;
}

void gps_disconnect() {
    char *handoff="$PMGNCMD,HANDOFF*";

    send_string(handoff,false);
    SerReceiveFlush(serlib,SysTicksPerSecond()/2);
    ser_close();
}


/* Get the ID of the magellanunit */
void get_mag_id(void){
    char idstring[512];
    
    if (gps_connect(idstring,512)) {
	FrmCustomAlert(INF_DLG1,"Found Magellan:",idstring," ");
	gps_disconnect();
    }
}

/* Callback for progressdialog */
static Boolean progress_cb (PrgCallbackDataPtr cbp){
	StrPrintF(cbp->textP,"%s %i %s",operation,cbp->stage,elemtype);
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
	char *msgtrack="$PMGNCMD,TRACK,2*";
	char *msgwaypt="$PMGNCMD,WAYPOINT*";
	char *msgroute="$PMGNCMD,ROUTE*";
	char *progtrack="trackpoints";
	char *progwaypts="waypoints";
	char *progroute="route records";
	char *progop="Got";
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

	operation = progop;
	switch(type){
		case track: msr=msgtrack; elemtype=progtrack; break;
		case waypt: msr=msgwaypt; elemtype=progwaypts; break;
		case route: msr=msgroute; elemtype=progroute; break;
		default: return false;
	}

	if(!gps_connect(field,512)){return false;}

	if(!send_string(msr,true)){ 
	    /* if the unit does not reply, we assume that we were not
	       successful in switching to handshake mode -- and do not
	       try to disable the mode again */
	    ser_close(); return false;
	}
	
	/* The real McCoy - Get everything the Magellan wants to give */
	timeout=SysSetAutoOffTime(0);
	progptr=PrgStartDialogV31(msr,progress_cb);
	while(!done && progptr){
		MemSet(msg,1024,0);
		if(get_string(msg,1024)){
			chk[2]=0; chk2[2]=0;
			get_checksum(msg,chk,3);
			magchksum(msg,chk2,3);
			if(StrCompare(chk,chk2)!=0) {
			    /* csum error forces a resend by not sending
			       an ack! */
			    FrmCustomAlert(ALM_DLG1,"Checksum error! We try to recover."," "," ");
			} else {
			    StrPrintF(field,"$PMGNCSM,%s*",chk2);
			    send_string(field,false);
			    /* FrmCustomAlert(ALM_DLG1,msg,field," "); */
			    EvtGetEvent(&evt,10);
			    while(evt.eType!=nilEvent){
				if(!PrgHandleEvent(progptr,&evt)){
				    /* Actually, it is not too bad
				       to cancel a transfer. Just switch the
				       GPS off & on again if necessary*/ 
				    FrmCustomAlert(ALM_DLG1,"Transfer cancelled!","Switch GPS off & on","to reset communcation.");
				    goto end;
				}
				EvtGetEvent(&evt,1);
			    }
			    PrgUpdateDialog(progptr,0,msgcount++,NULL,true);
			    if(StrNCompare(msg,"$PMGNCMD,END*",13)==0){
				done=true;
				complete=true;
			    } else if (StrNCompare(msg,"$PMGNCMD,UNABLE*",16)==0){
				done=true;
				FrmCustomAlert(ALM_DLG1,"GPS was unable to complete operation!","","");
			    } else {
				addrecord(dbref,msg);
			    }
			}
		} else {
			done=true;
		}
	}
 end:
	gps_disconnect();
	SysSetAutoOffTime(timeout);
	if(progptr){PrgStopDialog(progptr,false);}
	
	return complete;
}

/* Get the waypoints from GPS */
void mag_get_data(msgtype msgt){
	char*t="";
	LocalID dbid;
	DmOpenRef dbref;
	UInt16 vers=DbVers;
	UInt16 newdbattr=dmHdrAttrBackup;
	Boolean success;

	GPSunable=false;

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
	if (msgt == route) {
	    success = get_data(waypt,dbref);
	    if (!success) {
		DmCloseDatabase(dbref);
		DmDeleteDatabase(0,dbid);
		FrmCustomAlert(ALM_DLG1,"Error while downloading from GPS!"," "," ");
	    return;
	    }
	}
	if(get_data(msgt,dbref)){
	    DmCloseDatabase(dbref);
	    FrmCustomAlert(INF_DLG1,"Saved database:",t," ");
	} else{
	    DmCloseDatabase(dbref);
	    DmDeleteDatabase(0,dbid);
	    FrmCustomAlert(ALM_DLG1,"Error while downloading from GPS!"," "," ");
	}
}

/* Upload something to the GPS receiver */
void mag_upload() {
    LocalID dbid;
    DmOpenRef dbref;
    UInt16 ix,maxix;
    MemHandle mh;
    MemPtr    mp;
    UInt16    retry;
    UInt16 timeout;
    ProgressPtr progptr;
    UInt16 msgcount=0;
    char *msr="Uploading Data";
    char *progop="Sent";
    char *progrec="records";
    char field[512];
    EventType evt;

    GPSunable=false;
	
    operation=progop;
    elemtype=progrec;

    UploadDbName[0] = '\0';
    db_diag(UploadDbForm);
    if (UploadDbName[0] == '\0') {
	return;
    UploadDbName[31] = '\0';
    }

    dbid=DmFindDatabase(0,UploadDbName);
    if(dbid==0) {
	FrmCustomAlert(ALM_DLG1,"Could not find Database'",UploadDbName,"'");
	return;
    }
	
    if (!gps_connect(field,512)) {
	return;
    }

    dbref=DmOpenDatabase(0,dbid,dmModeReadOnly);
    if (dbref==NULL) {
	FrmCustomAlert(ALM_DLG1,"Could not open Database'",UploadDbName,"'");
	gps_disconnect();
	return;
    }

    timeout=SysSetAutoOffTime(0);
    progptr=PrgStartDialogV31(msr,progress_cb);
    
    maxix = DmNumRecords(dbref);
    for (ix=0;ix<maxix;ix++) {
	mh = DmQueryRecord(dbref,ix);
	if (mh==0) {
	    FrmCustomAlert(ALM_DLG1,"Internal error!",
			   "No handle for index","");
	    goto end;
	}
	mp = MemHandleLock(mh);
	StrCopy(field,mp);
	if (field[StrLen(field)-1] != '\n' || field[StrLen(field)-2] != '\r') {
	    FrmCustomAlert(ALM_DLG1,"Record does not end in CR/LF",
			   "Aborting ...", " ");
	    goto end;
	}
	retry = 5;
	while (!send_string(field,true) && retry > 1 && !GPSunable) { retry--; }; 
	MemHandleUnlock(mh);
	if (GPSunable) { goto end; }
	if (retry == 0) {
	    FrmCustomAlert(ALM_DLG1,"GPS does not respond anymore",
			   " "," ");
	    goto end;
	} else {
	    EvtGetEvent(&evt,10);
	    while(evt.eType!=nilEvent){
		if(!PrgHandleEvent(progptr,&evt)){
		    /* Actually, it is not too bad
		       to cancel a transfer. Just switch the
		       GPS off & on again if necessary*/ 
		    FrmCustomAlert(ALM_DLG1,"Upload incomplete!","","");
		    goto end;
		}
		EvtGetEvent(&evt,1);
	    }
	    PrgUpdateDialog(progptr,0,++msgcount,NULL,true);
	}
    }
 end:
    SysSetAutoOffTime(timeout);
    DmCloseDatabase(dbref);
    if(progptr){PrgStopDialog(progptr,false);}
    gps_disconnect();
    }

/* vim: set fdm=indent: */
