/* $Id: main.c,v 1.8 2003/02/19 20:58:17 niki Exp $ */
#include <PalmOS.h>
#include <Window.h>
#include <ExgMgr.h>
#include <SerialMgrOld.h>
#include <Field.h>
#include "resource.h"
#include "magellan.h"
#include "ser.h"
#include "deldb.h"

typedef struct{
	UInt32 baud;
	UInt32 timeout;
} apppref;



/* Pointer to the currently-active form */
static FormPtr gpForm;
static UInt32 osv;
apppref pref;
UInt16 prefsize;

/* Functions */
UInt32 get_lst_int(const UInt16 lst_const,const UInt32 def){
	ListType *lst;
	char *s_baud;
	UInt32 i_baud=def;

	lst=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, lst_const));
	if(!lst) return i_baud;
	
	s_baud=LstGetSelectionText(lst,LstGetSelection(lst));
	if(!s_baud) return i_baud;

	i_baud=StrAToI (s_baud);
		
	return i_baud;
}

void set_lst(const UInt16 lst_const,const UInt16 val){
	ListType *lst;

	lst=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, lst_const));
	if(lst){
		LstSetSelection(lst,val);
		LstMakeItemVisible(lst,val);
		LstDrawList(lst);
	}
}

void set_lst_int(const UInt16 lst_const, const UInt32 val){
	if(lst_const==LST_BAUD){
		switch(val){
			case 4800:  set_lst(lst_const,0); break;
			case 9600:  set_lst(lst_const,1); break;
			case 19200:  set_lst(lst_const,2); break;
			case 57600:  set_lst(lst_const,3); break;
			case 115200:  set_lst(lst_const,4); break;
		}
	}
	else if(lst_const==LST_TIMEOUT){
		switch(val){
			case 3:  set_lst(lst_const,0); break;
			case 4:  set_lst(lst_const,1); break;
			case 5:  set_lst(lst_const,2); break;
			case 6:  set_lst(lst_const,3); break;
			case 7:  set_lst(lst_const,4); break;
			case 8:  set_lst(lst_const,5); break;
			case 9:  set_lst(lst_const,6); break;
			case 10:  set_lst(lst_const,7); break;
			case 11:  set_lst(lst_const,8); break;
			case 12:  set_lst(lst_const,9); break;
			case 13:  set_lst(lst_const,10); break;
			case 14:  set_lst(lst_const,11); break;
			case 15:  set_lst(lst_const,12); break;
				 
		}
	}
}

static Err StartApplication() 
{	Err err;

	err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osv);
	if(err){osv=0;}
	
	prefsize=sizeof(pref);
	if(PrefGetAppPreferences(AppId,1,&pref,&prefsize,false)==noPreferenceFound){
		pref.baud=4800;
		pref.timeout=3;
	}
	
	err=SysLibFind("Serial Library",&serlib);
	if(err){
		FrmCustomAlert(ALM_DLG1,"Could not find serial Library"," "," ");
		return 1;
	}
	
	FrmGotoForm(MainForm);
	return 0;
}

static void StopApplication() 
{
	pref.baud=get_lst_int(LST_BAUD,4600);
	pref.timeout=get_lst_int(LST_TIMEOUT,3);
	PrefSetAppPreferences(AppId,1,1,&pref,sizeof(pref),false);
	FrmCloseAllForms();
}

/* Export a string to memopad */
static void data_to_memo(char* data){
	ExgSocketPtr soc;
	Err err=0;

	soc=(ExgSocketPtr) MemPtrNew(sizeof(ExgSocketType));
	if(soc){
		MemSet(soc,sizeof(ExgSocketType),0);
		soc->localMode=1;
		soc->description="Magellan Datatransfer";
		soc->name="magexp.txt";
		if(osv>=0x03503000){
			soc->noGoTo=1;
		}

		err=ExgPut(soc);
		if(!err && data && StrLen(data)>0){
			ExgSend(soc,data,StrLen(data),&err);
			ExgDisconnect(soc,err);
		} else {
			FrmCustomAlert(ALM_DLG1,"Could not communicate with Memo-App"," "," ");
		}
		MemPtrFree(soc);
	}
}

/* MainForm eventhandler */
Boolean MainFormEventHandler(EventPtr event) 
{
	Boolean handled = false;
	char* howto="On your GPS:\n1. Turn  NMEA off\n2. Set baud to the value selected below.";
	FieldType *fld;

	switch (event->eType) {
		case frmOpenEvent: {
					   gpForm = FrmGetActiveForm();
					   FrmDrawForm(gpForm);
					   handled = true;
					   fld=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, FLD_MAIN));
					   FldSetTextPtr(fld,howto);
					   FldRecalculateField(fld,true);
	
					   set_lst_int(LST_BAUD,pref.baud);
					   set_lst_int(LST_TIMEOUT,pref.timeout);
					   break;
				   }

		case frmCloseEvent:
				   FrmEraseForm(gpForm);
				   FrmDeleteForm(gpForm);
				   gpForm = 0;
				   handled = true;
				   break;
		case ctlSelectEvent:
				   switch(event->data.ctlSelect.controlID){
					   case BTN_ID:
						   //Get MagID
						   get_mag_id();
						   handled = true;
						   break;
					   case BTN_TRK:
						   mag_get_data(track);
						   handled=true;
						   break;
					   case BTN_WPT:
						   mag_get_data(waypt);
						   handled=true;
						   break;
					   default:
						   break;
				   }

		default:
				   // -Wall warning eater: switch must handle all enum
				   // elements.
				   break;
	}

	return handled;
}


/* Application eventhandler */
static Boolean ApplicationEventHandler(EventPtr event) 
{
	Boolean handled = false;

	switch (event->eType) {
		case frmLoadEvent: {
					   FormPtr pForm = FrmInitForm(event->data.frmLoad.formID);
					   FrmSetActiveForm(pForm);
					   FrmSetEventHandler(pForm, MainFormEventHandler);
					   handled = true;
					   break;
				   }

		case menuEvent:
				   switch (event->data.menu.itemID) {
					   case HelpMenuAbout:
						   FrmAlert(AboutAlert);
						   break;
					   case MainMenuGetId:
						   get_mag_id();
						   break;
					   case MainMenuGetTrack:
						   mag_get_data(track);
						   break;
					   case MainMenuGetWaypts:
						   mag_get_data(waypt);
						   break;
					   case MainMenuDelDb:
						   deldb_diag();
						   break;
				   }

				   handled = true;
				   break;

		default:
				   // -Wall warning eater: switch must handle all enum
				   // elements.
				   break;
	}

	return handled;
}


/* Global event loop */
static void EventLoop() 
{
	EventType event;
	UInt16 error;
	do {
		EvtGetEvent(&event, evtWaitForever);

		if (!SysHandleEvent(&event)) {
			if (!MenuHandleEvent(0, &event, &error)) {
				if (!ApplicationEventHandler(&event)) {
					FrmDispatchEvent(&event);
				}
			}
		}
	}
	while (event.eType != appStopEvent);
}

/* Main main */
UInt32 PilotMain(UInt16 launchCode, MemPtr cmdPBP, UInt16 launchFlags) 
{
	Err err = 0;

	if (launchCode == sysAppLaunchCmdNormalLaunch) {
		if ((err = StartApplication()) == 0) {
			EventLoop();
			StopApplication();
		}
	}

	return err;
}
/* vim: set fdm=indent: */
