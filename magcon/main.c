/* $Id: main.c,v 1.12 2004/05/23 17:09:05 loxley Exp $ */
#include <PalmOS.h>
#include <Window.h>
#include <ExgMgr.h>
#include <SerialMgrOld.h>
#include <Field.h>
#include "resource.h"
#include "magellan.h"
#include "ser.h"
#include "dbdia.h"

typedef struct{
	UInt32 baud;
	UInt32 timeout;
	UInt32 action;
} apppref;



/* Pointer to the currently-active form */
static FormPtr gpForm;
static UInt32 osv;
apppref pref;
UInt16 prefsize;

/* Functions */

UInt32 get_lst(const UInt16 lst_const){
	ListType *lst;

	lst=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, lst_const));
	if(!lst) return 0;
	
	return LstGetSelection(lst);
}

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
	ControlPtr trg; 

	lst=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, lst_const));
	if(lst){
		LstSetSelection(lst,val);
		LstMakeItemVisible(lst,val);
		LstDrawList(lst);
	}
	
	/*ATTN: Ugly hack: lst_const+1 means that the popuptrigger has to have LIST-ID plus one!*/
	trg=(ControlPtr) FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, lst_const+1));
	if(trg && lst){
		CtlSetLabel(trg, LstGetSelectionText(lst,val));
	}
	
}

void set_lst_int(const UInt16 lst_const, const UInt32 val){
	if(lst_const==LST_BAUD){
		switch(val){
			case 1200:  set_lst(lst_const,0); break;
			case 4800:  set_lst(lst_const,1); break;
			case 9600:  set_lst(lst_const,2); break;
			case 19200:  set_lst(lst_const,3); break;
			case 57600:  set_lst(lst_const,4); break;
			case 115200:  set_lst(lst_const,5); break;
		}
	}
	else if(lst_const==LST_TIMEOUT){
		switch(val){
			case 3:  set_lst(lst_const,0); break;
			case 10:  set_lst(lst_const,1); break;
			case 20:  set_lst(lst_const,2); break;
			case 50:  set_lst(lst_const,3); break;
				 
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
		pref.action=0;
	}
	
	/*err=SysLibFind("Serial Library",&serlib);
	if(err){
		FrmCustomAlert(ALM_DLG1,"Could not find serial Library"," "," ");
		return 1;
	}
	*/
	
	FrmGotoForm(MainForm);
	return 0;
}

static void StopApplication() 
{
	pref.baud=get_lst_int(LST_BAUD,4800);
	pref.timeout=get_lst_int(LST_TIMEOUT,3);
	pref.action=get_lst(LST_ACTION);
	PrefSetAppPreferences(AppId,1,1,&pref,sizeof(pref),false);
	FrmCloseAllForms();
}

/* MainForm eventhandler */
Boolean MainFormEventHandler(EventPtr event) 
{
    ListType *lst;
	Boolean handled = false;
	char* howto="On your GPS:\n1. Turn  NMEA off\n2. Set baud to the value below";
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
					   set_lst(LST_ACTION,pref.action);
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
				       case BTN_EXEC:
					   lst=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, LST_ACTION));
					   switch(LstGetSelection(lst)) {
					       case 0:
						   get_mag_id();
						   handled = true;
						   break;
					       case 1: 
						   mag_get_data(track);
						   handled = true;
						   break;
					       case 2:
						   mag_get_data(waypt);
						   handled = true;
						   break;
					       case 3:
						   mag_get_data(route);
						   handled = true;
						   break;
					       case 4:
						   mag_upload();
						   handled = true;
						   break;
					       default:
						   break;
					   }
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
					   case MainMenuGetRoute:
						   mag_get_data(route);
						   break;
					   case MainMenuUpload:
						   mag_upload();
						   break;
					   case MainMenuDelDb:
						   db_diag(DelDbForm);
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
