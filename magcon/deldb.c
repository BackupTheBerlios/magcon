#include <PalmOS.h>
#include <DataMgr.h>
#include <List.h>
#include "resource.h"


ListType *lst;
char *dblist[256];
UInt16 dbcount;

void filllist(void){
	Err err;
	DmSearchStateType state; 
	UInt16 card; 
	LocalID currentDB = 0; 
	
	dbcount=1;

	err = DmGetNextDatabaseByTypeCreator(true, &state, Data, AppId, false, &card, &currentDB); 
	while (!err && currentDB) { 
		/* do something with currentDB */ 
		dblist[dbcount-1]=MemPtrNew(32);
		DmDatabaseInfo(card, currentDB, dblist[dbcount-1], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		/* now get the next DB */ 
		dbcount++;
		err = DmGetNextDatabaseByTypeCreator(false, &state, Data, AppId, false, &card, &currentDB); 
	}
	//void LstSetListChoices (ListType *listP, Char **itemsText, UInt16 numItems)
	LstSetListChoices (lst, dblist, dbcount-1);

}
	
	



static Boolean db_evt_handler(EventPtr eventP){
	Boolean handled = false;

	if ((eventP->eType == ctlSelectEvent) && (eventP->data.ctlSelect.controlID == BTN_DEL_DB)) {
		FrmCustomAlert(ALM_DLG1,"Delete it"," "," ");	
		handled=true;
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
	dbcount--;
	while(dbcount--){
		MemPtrFree(dblist[dbcount]);
	}

	return;
}
