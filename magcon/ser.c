/* $Id: ser.c,v 1.2 2003/02/06 20:01:05 niki Exp $ */
#include <PalmOS.h>
#include <SerialMgrOld.h>
#include "magellan.h"
#include "resource.h"

UInt16 serlib;
UInt16 timeoutdiv=3;

Boolean ser_open(void){
	SerSettingsType portSettings;
	Err err;

	err=SerOpen(serlib,0,4800);
	if(err){
		FrmCustomAlert(ALM_DLG1,"Could not open serial port!"," "," ");
		if(err==serErrAlreadyOpen){SerClose(serlib);}
		return false;
	}
	SerGetSettings(serlib,&portSettings);
	portSettings.baudRate=4800;
	portSettings.flags=(serSettingsFlagStopBits1 |	serSettingsFlagBitsPerChar8 | serSettingsFlagRTSAutoM);
	SerSetSettings(serlib,&portSettings);

	SerSendFlush(serlib);

	SerReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);

	return true;
}

void ser_close(void){
	SerClose(serlib);
}

void error_dlg(Err err){
	char errpuff[512];

	if(err==serErrLineErr) SerClearErr(serlib);
	SysErrString(err,errpuff,512);
	FrmCustomAlert(ALM_DLG1,"SHIT!",errpuff,"while reading");
}

Boolean get_string(char* string,UInt16 size){
	Err err;
	Boolean ret;
	UInt32 numbytes;
	UInt32 bytetran;
	char* puffer;
	UInt16 minsize;
	UInt16 timeout;

	ret=false;
	bytetran=0;

	minsize=16;
	timeout=SysTicksPerSecond()/timeoutdiv;

	if (size<=0){return ret;}

	puffer=(char*) MemPtrNew(size);
	if(puffer){
		MemSet(puffer,size,0);
		err=SerReceiveWait(serlib,minsize,timeout);
		if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}
		err=SerReceiveCheck(serlib,&numbytes);
		if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}

		if(numbytes<size){
			bytetran=SerReceive(serlib,puffer,size-1,timeout,&err);
			/*FrmCustomAlert(ALM_DLG1,puffer," "," ");*/
			if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}
		} else {
			FrmCustomAlert(ALM_DLG1,"Receivebuffer too small!"," "," ");
		}

		if(bytetran>0 && puffer[bytetran-1]=='\n'){
			StrNCopy(string,puffer,size); /*puffer includes trailing \0*/
			ret=true;
		}else{
			if(bytetran>0) FrmCustomAlert(ALM_DLG1,"Receivebuffer may be too small!","Or TimeoutDiv to large","Discarding data...");
			SerReceiveFlush(serlib,timeout);
		}
end:	
		MemPtrFree(puffer);
	} else {
		FrmCustomAlert(ALM_DLG1,"ALLOC FAILURE"," "," ");
	}
	return ret;
}

Boolean send_string(char* str,Boolean ack){
	char* sndstr;
	Err err;
	UInt16 err1;
	UInt32 count;
	char puffer1[3];
	char puffer2[17];
	Boolean ret;

	ret=false;
	sndstr=(char*) MemPtrNew(StrLen(str)*2);
	MemSet(sndstr,StrLen(str)*2,0);
	MemSet(puffer1,3,0);	
	if (sndstr){
		magchksum(str,puffer1,3);
		err1=StrPrintF(sndstr,"%s%s\r\n",str,puffer1);
		if(err1<0){
			FrmCustomAlert(ALM_DLG1,"Stringerror"," "," ");
		} else {
			SerReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);
			count=SerSend(serlib,sndstr,StrLen(sndstr),&err);
			if(!err){
				err=SerSendWait(serlib,-1);
				if(err){
					FrmCustomAlert(ALM_DLG1,"Could not send"," "," ");
				} else {
					ret=true;
				}
			} else {
				FrmCustomAlert(ALM_DLG1,"Could not send"," "," ");
			}
		} 
		MemPtrFree(sndstr);
	}
	if (ret && ack){
		ret=get_string(puffer2,17);
	}
	return ret;
}

/* vim: set fdm=indent: */
