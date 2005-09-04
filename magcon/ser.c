/* $Id: ser.c,v 1.10 2005/09/04 07:06:59 loxley Exp $ */
#include <PalmOS.h>
#include <SerialMgr.h>
#include <StringMgr.h>
#include "magellan.h"
#include "resource.h"
#include "mag1.h"

UInt16 serlib;
Boolean GPSunable=false;

Boolean send_string(char* str,Boolean ack);

Boolean ser_open(void){
	/*SerSettingsType portSettings;*/
	UInt16 timeoutdiv;
	char errpuff[512];
	Err err;
	
	timeoutdiv=get_lst_int(LST_TIMEOUT,3);
	err=SrmOpen(serPortCradleRS232Port, get_lst_int(LST_BAUD,4600),&serlib);
	if(err){
		SysErrString(err,errpuff,512);
		FrmCustomAlert(ALM_DLG1,"Could not open serial port!","System",errpuff);
		return false;
	}
	/*SerGetSettings(serlib,&portSettings);
	portSettings.baudRate=get_lst_int(LST_BAUD,4600);
	portSettings.flags=(serSettingsFlagStopBits1 |	serSettingsFlagBitsPerChar8 );//| serSettingsFlagRTSAutoM);
	SerSetSettings(serlib,&portSettings);
	*/
	SrmSendFlush(serlib);
	SrmReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);
	SrmSendFlush(serlib);
	SrmReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);

	return true;
}

void ser_close(void){
	SrmClose(serlib);
}

void error_dlg(Err err){
	char errpuff[512];

	if(err==serErrLineErr) SrmClearErr(serlib);
	SysErrString(err,errpuff,512);
	FrmCustomAlert(ALM_DLG1,"System error:",errpuff,"while reading");
}

Boolean get_string(char* string,UInt16 size){
	Err err;
	Boolean ret;
	UInt32 bytetran;
	char* puffer;
	UInt16 minsize;
	UInt16 timeout;
	UInt16 maxret=5;
	UInt16 nl, i;
	UInt16 filled=0;
	
	ret=false;
	bytetran=0;
	

	minsize=16;
	timeout=SysTicksPerSecond()/get_lst_int(LST_TIMEOUT,3);

	if (size<=0){return ret;}

	puffer=(char*) MemPtrNew(size+1024);
	MemSet(puffer,size+1024,0);
	if(puffer){
	    while (1) {
		if (filled > size+512) {
		    FrmCustomAlert(ALM_DLG1,"Too many input records!"," ","Discarding data...");
		     SrmReceiveFlush(serlib,timeout);
		     goto end;
		}
		while ((err=SrmReceiveWait(serlib,minsize,SysTicksPerSecond()))==serErrTimeOut && maxret-->=1);
		if(err && err!=serErrTimeOut){ 
		    error_dlg(err); 
		    break;
		}
		bytetran=SrmReceive(serlib,puffer+filled,size-1,timeout,&err);
		if(err && err!=serErrTimeOut){ 
		    error_dlg(err); 
		    break;
		}
		if (bytetran == 0) {
		    /* we have not received anything! */
		     FrmCustomAlert(ALM_DLG1,"Receive timeout!\nIs the GPS connected with the right baud rate?","","");
		     break;
		}
		filled=filled+bytetran;
		/* FrmCustomAlert(ALM_DLG1,"RECEIVED:\n",puffer,""); */
		if(puffer[filled-1] == '\n'){
		    /* Complete line received! Ignore all earlier
		       lines we got.  This accounts for the problem
		       when the GPS is in handshake mode initially and
		       when it has already resent a record after not
		       receiving an ack in time */
		    nl = 0;
		    for (i=filled-1;i>0;i--) {
			if (puffer[i-1] == '\n') {
			    nl=i;
			    break;
			}
		    }
		    StrNCopy(string,puffer+nl,size); /*puffer includes trailing \0*/
		    ret=true;
		    break;
		}
	    }
	} else {
	    FrmCustomAlert(ALM_DLG1,"ALLOC FAILURE"," "," ");
	}
end:	
	if (puffer) { MemPtrFree(puffer); }
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
		/* FrmCustomAlert(ALM_DLG1,"SENT:\n",sndstr," ");*/
		if(err1<0){
			FrmCustomAlert(ALM_DLG1,"Stringerror"," "," ");
		} else {
			SrmReceiveFlush(serlib,SysTicksPerSecond()/timeoutdiv);
			count=SrmSend(serlib,sndstr,StrLen(sndstr),&err);
			if(!err){
				err=SrmSendWait(serlib);
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
		/* FrmCustomAlert(ALM_DLG1,"GOT ACK:\n",puffer2," "); */
		if (StrNCompare(puffer2,"$PMGNCMD,UNABLE*",16) == 0) {
		    FrmCustomAlert(ALM_DLG1,"GPS is unable to complete its operation for the record: ",str,puffer1);
		    GPSunable = true;
		    return false;
		}
		ret=(puffer2[9]==puffer1[0] && puffer2[10]==puffer1[1] && 
		    StrNCompare(puffer2,"$PMGNCSM",8) == 0);
	}
	return ret;
}

/* vim: set fdm=indent: */
