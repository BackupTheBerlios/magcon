/* $Id: deldb.c,v 1.4 2003/02/15 18:07:57 niki Exp $ */
#include <PalmOS.h>
#include <DataMgr.h>
#include <List.h>
#include "resource.h"

#define MAXDBS 256
ListType *lst;
char *dblist[MAXDBS];
UInt16 dbcount;

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
		}
	}

	return handled;
}

void deldb_diag(void){
	FormPtr frmP;

	frmP = FrmInitForm(DelDbForm);
	lst=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, LST_DBLST));
	FrmSetEventHandler(frmP,  db_evt_handler);
	filllist();
	FrmDoDialog(frmP);

	FrmDeleteForm(frmP);
	freedblist();
	return;
}

/* vim: set fdm=indent: */
