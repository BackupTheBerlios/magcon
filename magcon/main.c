/* $Id: main.c,v 1.2 2003/02/02 12:22:53 niki Exp $ */
#include <PalmOS.h>
#include <Window.h>
#include <ExgMgr.h>
#include <SerialMgrOld.h>
#include <Progress.h>
#include <Field.h>
#include "resource.h"
#include "ser.h"
#include "magellan.h"

/* Globals */

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

/* Get the ID of the magellanunit */
static void get_mag_id(void){
	char *msr="$PMGNCMD,VERSION*";
	char msg[512];
	char field[512];
	
	if(ser_open()){
		send_string(msr,false);
		if(get_string(msg,512)){
                        get_field(msg,field,512,4);
                        StrCat(field,"  ");
                        get_field(msg,field+StrLen(field),512-StrLen(field),3);
                        WinDrawChars(field,StrLen(field),2,105);
		} else {
			FrmCustomAlert(ALM_DLG1,"Could not get Magellan ID"," "," ");
		}
		ser_close();
	}
}

/* Callback for progressdialog */
Boolean progress_cb (PrgCallbackDataPtr cbp){

	StrPrintF(cbp->textP,"Got %i Waypoints",cbp->stage);
	return true;
}

/* Get the track from PS*/
static void get_trk(void){
	char *magdata;
	UInt16 magdatasize;
	char *handon="$PMGNCMD,HANDON*";
	char *handoff="$PMGNCMD,HANDOFF*";
	char *msr="$PMGNCMD,TRACK,2*";
	char *tmpptr;
	char msg[1024];
	char field[512];
	char chk[3];
	Boolean done;
	ProgressPtr progptr;
	UInt16 msgcount;
	Boolean cancelled;
	EventType evt;
	
	done=false;
	magdata=NULL;
	msgcount=0;
	cancelled=false;
	
	if(!ser_open()){return;}
	if(send_string(handon,false) && send_string(msr,true)){
		if(!magdata){
			magdata=(char*)MemPtrNew(4096);
			MemSet(magdata,4096,0);
			magdatasize=4096;
			StrCat(magdata,"# <Name>\r\n");
		}
		progptr=PrgStartDialogV31(msr,progress_cb);
		while(!done && magdata && progptr){
			MemSet(msg,1024,0);
			if(get_string(msg,1024)){
				get_checksum(msg,chk,3);
				StrPrintF(field,"$PMGNCSM,%s*",chk);
				send_string(field,false);
				/*FrmCustomAlert(ALM_DLG1,msg,field," ");*/
				EvtGetEvent(&evt,10);
				while(evt.eType!=nilEvent){
					if(!PrgHandleEvent(progptr,&evt)){
						FrmCustomAlert(ALM_DLG1,"Sorry!","You can't cancel.","It upsets the Magellan");
					}
					EvtGetEvent(&evt,1);
				}
				PrgUpdateDialog(progptr,0,++msgcount,NULL,true);
				if(StrLen(magdata)+StrLen(msg)>=magdatasize){
					tmpptr=(char*)MemPtrNew(magdatasize+magdatasize/2);
					if(tmpptr){
						magdatasize+=magdatasize/2;
						MemSet(tmpptr,magdatasize,0);
						StrCat(tmpptr,magdata);
						MemPtrFree(magdata);
						magdata=tmpptr;
					}
				}
				if(magdata){
					if(StrNCompare(msg,"$PMGNCMD,END*",13)==0){
						done=true;
					} else {
						StrCat(magdata,msg);
					}
				}
			} else {
				done=true;
			}
		}
		send_string(handoff,false);
		SerReceiveFlush(serlib,SysTicksPerSecond()/2);
		if(progptr){PrgStopDialog(progptr,false);}
		//FrmCustomAlert(ALM_DLG1,magdata,chk," ");
		if(magdata){
			data_to_memo(magdata);
			MemPtrFree(magdata);
		}
	} else {
		FrmCustomAlert(ALM_DLG1,"Could not get a track!"," "," ");
	}
	ser_close();
}

/* MainForm eventhandler */
Boolean MainFormEventHandler(EventPtr event) 
{
	Boolean handled = false;
	char* howto="On your GPS:\n1. Turn  NMEA off\n2. Set baud to 4600\n\nHit \"Get Track\"\n\nIn Memopad substitute <Name> with a meaningfull name";
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
						   get_trk();
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
						   get_trk();
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
