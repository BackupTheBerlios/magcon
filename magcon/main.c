/* $Id: main.c,v 1.7 2003/02/11 16:10:26 niki Exp $ */
#include <PalmOS.h>
#include <Window.h>
#include <ExgMgr.h>
#include <SerialMgrOld.h>
#include <Field.h>
#include "resource.h"
#include "magellan.h"
#include "ser.h"
#include "deldb.h"

/* Pointer to the currently-active form */
static FormPtr gpForm;
static UInt32 osv;

/* Functions */

static Err StartApplication() 
{	Err err;

	err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osv);
	if(err){osv=0;}

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
	char* howto="On your GPS:\n1. Turn  NMEA off\n2. Set baud to 4800\n\nHit \"Get Track\" and enter a name for your track.";
	FieldType *fld;

	switch (event->eType) {
		case frmOpenEvent: {
					   gpForm = FrmGetActiveForm();
					   FrmDrawForm(gpForm);
					   handled = true;
					   fld=FrmGetObjectPtr(gpForm, FrmGetObjectIndex(gpForm, FLD_MAIN));
					   FldSetTextPtr(fld,howto);
					   FldRecalculateField(fld,true);
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
