/* $Id: dbdia.c,v 1.1 2003/10/05 10:14:54 niki Exp $ */
#include <PalmOS.h>
#include <DataMgr.h>
#include <List.h>
#include "resource.h"
#include "magellan.h"

#define MAXDBS 256
ListType *lst;
char *dblist[MAXDBS];
UInt16 dbcount;
char UploadDbName[32];

void freedblist(void){

	dbcount--;
	while(dbcount--){
		MemPtrFree(dblist[dbcount]);
	}
}

Int16 compstr (void *a, void *b, Int32 other){
	return StrCompare(*(char**)a,*(char**)b);
}

void filllist(void){
	Err err;
	DmSearchStateType state; 
	UInt16 card; 
	LocalID currentDB = 0; 
	
	dbcount=1;

	err = DmGetNextDatabaseByTypeCreator(true, &state, Data, AppId, false, &card, &currentDB); 
	while (!err && currentDB && dbcount<=MAXDBS) { 
		/* do something with currentDB */ 
		dblist[dbcount-1]=MemPtrNew(32);
		DmDatabaseInfo(card, currentDB, dblist[dbcount-1], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		/* now get the next DB */ 
		dbcount++;
		err = DmGetNextDatabaseByTypeCreator(false, &state, Data, AppId, false, &card, &currentDB); 
	}
	SysQSort(dblist,dbcount-1,sizeof(char*),compstr,0);
	//void LstSetListChoices (ListType *listP, Char **itemsText, UInt16 numItems)
	LstSetListChoices (lst, dblist, dbcount-1);

}

char *getDbType(DmOpenRef dbref, char string[]) {
    char *ptrack = "$PMGNTRK";
    char *pwaypt = "$PMGNWPL";
    char *proute = "$PMGNRTE";
    msgtype type = 0;
    UInt16 ix, recnum;
    MemHandle mh;
    MemPtr mp;

    StrCopy(string,"?");
    recnum = DmNumRecords(dbref);
    for (ix=0;ix<recnum;ix++) {
	mh = DmQueryRecord(dbref,ix);
	if (mh==0) {
	    FrmCustomAlert(ALM_DLG1,"Internal error!",
			   "No handle for index","");
	    return string;
	}
	mp = MemHandleLock(mh);
	if (StrNCompare(ptrack,mp,8) == 0 && type == 0) {
	    type = track;
	}
	if (StrNCompare(pwaypt,mp,8) == 0 && type < waypt) {
	    type = waypt;
	}
	if (StrNCompare(proute,mp,8) == 0 && type < route) {
	    type = route;
	}
	MemHandleUnlock(mh);
    }
    switch (type) {
	case track:
	    StrCopy(string,"track");
	    break;
	case waypt:
	    StrCopy(string,"waypoints");
	    break;
	case route:
	    StrCopy(string,"route (with waypoints)");
	    break;
	default:
	    break;
    }
    return string;
}
	    

char *printDateTime(UInt32 secs, char string[]) {
    DateTimeType dt;
    char clock[512];

    TimSecondsToDateTime(secs,&dt);
    DateToAscii(dt.month,dt.day,dt.year,dfDMYLong,string);
    StrCat(string," / ");
    TimeToAscii(dt.hour,dt.minute,tfColon24h,clock);
    StrCat(string,clock);
    return string;
}

void info_form(const char *name) {
    FormType *form;
    DmOpenRef dbref;
    LocalID dbid;
    UInt16 dbattr, dbversion;
    UInt32 dbcreated, dbbackuped, dbmod, dbrecords, dbsize;
    char string[512];

    dbid=DmFindDatabase(0,name);
    if (dbid == 0) {
	FrmCustomAlert(ALM_DLG1,"Internal error!", 
		       "Cannot find DB:", name);
	return; 
    }
    DmDatabaseSize(0,dbid,&dbrecords,&dbsize,NULL);
    dbref=DmOpenDatabase(0,dbid,dmModeReadOnly);
    if (dbref==NULL) {
	FrmCustomAlert(ALM_DLG1,
		       "Internal error: Could not open Database",
		       name,"'");
	return;
    }
    DmDatabaseInfo(0,dbid,NULL,&dbattr,&dbversion,
		   &dbcreated,&dbmod,&dbbackuped,
		   NULL,NULL,NULL,NULL,NULL);


    form = FrmInitForm(InfoForm);
    FrmCopyLabel(form,INFO_NAME,name);
    FrmCopyLabel(form,INFO_TYPE,getDbType(dbref,string));
    StrPrintF(string,"%d",dbrecords);
    FrmCopyLabel(form,INFO_RECORDS,StrIToA(string,dbrecords));
    StrPrintF(string,"%d",dbsize);
    FrmCopyLabel(form,INFO_SIZE,StrIToA(string,dbsize));
    if (dbcreated == 0) {
	    FrmCopyLabel(form,INFO_CREATED,"---");
    } else {
	FrmCopyLabel(form,INFO_CREATED,printDateTime(dbcreated,string));
    }
    if (dbbackuped == 0) {
	    FrmCopyLabel(form,INFO_BACKUPED,"no backup yet");
    } else {
	FrmCopyLabel(form,INFO_BACKUPED,printDateTime(dbbackuped,string));
    }
    StrPrintF(string,"%d",dbversion);
    FrmCopyLabel(form,INFO_VERSION,string);
    StrPrintF(string,"%04X",dbattr);
    FrmCopyLabel(form,INFO_FLAGS,string);

    FrmDoDialog(form);
    FrmDeleteForm(form);
    DmCloseDatabase(dbref);
}

Boolean db_evt_handler(EventPtr eventP){
	Boolean handled = false;
	char *name;
	UInt16 newdbattr=dmHdrAttrBackup;
	
	if (eventP->eType == ctlSelectEvent){
		name=LstGetSelectionText(lst,LstGetSelection(lst));	
		switch (eventP->data.ctlSelect.controlID){
			case BTN_DEL_DB:
				if(name && 0==FrmCustomAlert(DeleteAlert,name," "," ")){
					DmDeleteDatabase(0,DmFindDatabase(0,name));
				}
				freedblist();
				filllist();
				LstDrawList(lst);
				handled=true;
				break;
			case BTN_BCK_DB:
				if(name){
					DmSetDatabaseInfo(0,DmFindDatabase(0,name),NULL,&newdbattr,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
				}
				handled=true;
				break;
			case BTN_UP_DB:
				if(name){
					StrNCopy(UploadDbName,name,32);
				}
				handled=false;
				break;
		        case BTN_INFO_DB:
			    info_form(name);
			    handled = true;
			    break;
		}
	}

	return handled;
}

void db_diag(const UInt16 form_const){
	FormPtr frmP;
	
	frmP = FrmInitForm(form_const);
	lst=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, LST_DBLST));
	FrmSetEventHandler(frmP,  db_evt_handler);
	filllist();
	FrmDoDialog(frmP);

	FrmDeleteForm(frmP);
	freedblist();
	return;
}

/* vim: set fdm=indent: */
