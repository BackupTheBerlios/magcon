/* $Id: ser.c,v 1.6 2003/02/19 20:58:17 niki Exp $ */
#include <PalmOS.h>
#include <SerialMgrOld.h>
#include <StringMgr.h>
#include "magellan.h"
#include "resource.h"
#include "mag1.h"

UInt16 serlib;

Boolean send_string(char* str,Boolean ack);

Boolean ser_open(void){
	SerSettingsType portSettings;
	UInt16 timeoutdiv;
	Err err;
	
	timeoutdiv=get_lst_int(LST_TIMEOUT,3);
	err=SerOpen(serlib,0,get_lst_int(LST_BAUD,4600));
	if(err){
		FrmCustomAlert(ALM_DLG1,"Could not open serial port!"," "," ");
		if(err==serErrAlreadyOpen){SerClose(serlib);}
		return false;
	}
	SerGetSettings(serlib,&portSettings);
	portSettings.baudRate=get_lst_int(LST_BAUD,4600);
	portSettings.flags=(serSettingsFlagStopBits1 |	serSettingsFlagBitsPerChar8 );//| serSettingsFlagRTSAutoM);
	SerSetSettings(serlib,&portSettings);
	
	SerSendFlush(serlib);
	SerReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);
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
	char puff[maxStrIToALen];
	char puff2[maxStrIToALen];
	char puff3[maxStrIToALen];
	UInt16 minsize;
	UInt16 timeout;
	
	ret=false;
	bytetran=0;

	minsize=16;
	timeout=SysTicksPerSecond()/get_lst_int(LST_TIMEOUT,3);

	if (size<=0){return ret;}

	puffer=(char*) MemPtrNew(size);
	if(puffer){
		MemSet(puffer,size,0);
		while ((err=SerReceiveWait(serlib,minsize,timeout))==serErrTimeOut);
		if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}
		err=SerReceiveCheck(serlib,&numbytes);
		if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}
		/*if(numbytes<size){*/
			bytetran=SerReceive(serlib,puffer,size-1,timeout,&err);
			/*FrmCustomAlert(ALM_DLG1,puffer," "," ");*/
			if(err && err!=serErrTimeOut){ error_dlg(err);goto end;}
		/*} else {
			bytetran=SerReceive(serlib,puff3,maxStrIToALen,timeout,&err);
			FrmCustomAlert(ALM_DLG1,puff3,StrIToA(puff,numbytes),StrIToA(puff2,size));
		}*/

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
	UInt32 timeoutdiv;

	ret=false;
	timeoutdiv=get_lst_int(LST_TIMEOUT,3);
	sndstr=(char*) MemPtrNew(StrLen(str)*2);
	MemSet(sndstr,StrLen(str)*2,0);
	MemSet(puffer1,3,0);	
	if (sndstr){
		magchksum(str,puffer1,3);
		err1=StrPrintF(sndstr,"%s%s\r\n",str,puffer1);
		/*FrmCustomAlert(ALM_DLG1,sndstr," "," ");*/
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
		get_string(puffer2,17);
		ret=(puffer2[9]==puffer1[0] && puffer2[10]==puffer1[1]);
	}
	return ret;
}

/* vim: set fdm=indent: */
