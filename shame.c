/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

#include <conf.h>
#include <mydefs.h>

#include "shame.h"
#define FILE_NUM 35
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * functions of which I am not proud
 **********************************************************************/
#pragma segment Util
static Boolean AlertBeep=True;
void DeepTrouble(UPtr);
void SilenceAlert(int template);
void TrashAlert(int template);
pascal void OkWhatIsIt(QElemPtr nmReqPtr);
extern ModalFilterUPP DlgFilterUPP;

/************************************************************************
 *
 ************************************************************************/
void Dprintf(PStr fmt,...)
{
	Str255 message;
	va_list args;
	va_start(args,fmt);
	VaComposeString(message,fmt,args);
	va_end(args);
	DebugStr(message);
}

/************************************************************************
 *
 ************************************************************************/
int Aprintf(short template,short which,short rFormat,...)
{
	Str255 message;
	va_list args;
	va_start(args,rFormat);
	VaComposeRString(message,rFormat,args);
	va_end(args);
	return(AlertStr(template,which,message));
}
	
/************************************************************************
 *
 ************************************************************************/
int AlertStr(int template, int which, UPtr str)
{
	MyParamText(str,"","","");
	return(ReallyDoAnAlert(template,which));
}

/**********************************************************************
 * print an alert
 **********************************************************************/
int ReallyDoAnAlert(int template,int which)
{
	int item;
	Boolean commandPeriodWas = CommandPeriod;
#if __profile__
	short profilerWas = ProfilerGetStatus();
	//ProfilerSetStatus(False);
#endif
	
	CommandPeriod = False;

	// use ComposeStdAlert when we can
	if ((template>ALRTStringsStrn) && (template<ALRTStringsOnlyStrn+LIMIT_ASTR_ONLY))
	{
		AlertType type;
		switch(which)
		{
			case Note: type = kAlertNoteAlert;	break;
			case Caution:	type = kAlertCautionAlert;break;
			case Stop:type = kAlertStopAlert;break;
			case Normal:
			default: type = kAlertPlainAlert;break;
		}
		return(ComposeStdAlert(type,template,P1,P2,P3,P4));
	}
	
	LogAlert(template);
	
	if (!MommyMommy(ATTENTION,nil))
	  item = 1;	//	default item
	else
	{
		TBDisable();
		WNE(nil,nil,0); 							/* make sure InBG is right */
		PushCursor(arrowCursor);
		if (!AlertBeep) SilenceAlert(template);

		item = MovableAlert(template,which,DlgFilterUPP);
		if (!AlertBeep) TrashAlert(template);
	}
	ComposeLogR(LOG_ALRT,nil,ALERT_DISMISSED_ITEM,item);
	PopCursor();
#if __profile__
	//ProfilerSetStatus(profilerWas);
#endif
	if (!CommandPeriod) CommandPeriod = commandPeriodWas;
	return(item);
}

/**********************************************************************
 * ReallyStandardAlert - call standard alert
 *
 **********************************************************************/
short ReallyStandardAlert(AlertType alertType, StringPtr error, StringPtr explanation, AlertStdAlertParamPtr alertParam)
{
	Str255	logString;
	short 	item;
	Boolean commandPeriodWas = CommandPeriod;
	UPtr		text;
	Byte 		meta[2];
	Boolean	spokenWarning = false;
	
	alertParam->helpButton = true;

#if __profile__
	//short profilerWas = ProfilerGetStatus();
	//ProfilerSetStatus(False);
#endif

// If this is an alert that will be spoken instead of displayed, strip the opt-s metacharacter
// and skip over this character
	text = error;
	if (*text && text[1] == 0xA7) {
		spokenWarning = !PrefIsSet (PREF_NO_SPOKEN_WARNINGS);
		text[1] = text[0] - 1;
		error = text + 1;
	}

	CommandPeriod = False;

	// Parse the error and explanation strings for the log (we only log the visible parts)
	meta[0] = 0xB7;
	meta[1] = 0;
	if (*error) {
		text = error + (error[1] == 0xC2 ? 2 : 1);
		PToken (error, logString, &text, meta);
		ComposeLogS (LOG_ALRT, nil, "\p%p", logString);
	}
	if (*explanation) {
		text = explanation + (explanation[1] == 0xC2 ? 2 : 1);
		PToken (explanation, logString, &text, meta);
		ComposeLogS (LOG_ALRT, nil, "\p%p", logString);
	}

#ifdef THREADING_ON
	if (InAThread())
	{
		AddTaskErrorsS(error,explanation,GetCurrentTaskKind(),(*CurPersSafe)->persId);
		return 1;
 	}
#endif

	// Are we exporting?
	if (ExportErrors)
	{
		AccuAddStr(ExportErrors,error);
		AccuAddStr(ExportErrors,explanation);
		AccuAddStr(ExportErrors,Cr);
		return 1;
	}
	
	if (!MommyMommy(ATTENTION,nil))
	  item = alertParam->defaultButton;
	else
	{
		TBDisable();
		WNE(nil,nil,0); 							/* make sure InBG is right */
		PushCursor(arrowCursor);

#ifdef SPEECH_ENABLED
		TalkingAlert (spokenWarning, alertType, error, explanation, alertParam, &item);
#else
		MyStandardAlert(alertType,error,explanation,alertParam,&item);
#endif

		ActiveTicks = TickCount();	/* Set ActiveTicks */
	}
	ComposeLogR(LOG_ALRT,nil,ALERT_DISMISSED_ITEM,item);
	PopCursor();
#if __profile__
	//ProfilerSetStatus(profilerWas);
#endif
	if (!CommandPeriod) CommandPeriod = commandPeriodWas;
	return(item);
}

/**********************************************************************
 * MyStandardAlert - yet another wrapper around StandardAlert.  How many
 * levels deep are we going to go, huh?
 **********************************************************************/
OSErr MyStandardAlert(AlertType inAlertType,PStr inError,PStr inExplanation, AlertStdAlertParamRec *  inAlertParam,short *outItemHit)
{
	OSErr err;
	
	inAlertParam->helpButton = true;
	
	do
	{
		err = StandardAlert (inAlertType,inError,inExplanation,inAlertParam,outItemHit);
		if (!err && *outItemHit == kAlertStdAlertHelpButton)
			GoGetHelp(inError,inExplanation);
	}
	while (!err && *outItemHit == kAlertStdAlertHelpButton);

	return err;
}

/**********************************************************************
 * GoGetHelp - go to the web for help
 **********************************************************************/
OSErr GoGetHelp(PStr error, PStr explanation)
{
	Handle url = GenerateAdwareURL(GetNagState (), TECH_SUPPORT_SITE, actionSupport, helpQuery, topicQuery);
	Accumulator a;
	Str31 keyword;
	
	if (url)
	{
		a.data = url;
		a.size = a.offset = GetHandleSize(url);
		a.offset--;	// get rid of null added by GenerateAdwareURL
		
		if (*error)
		{
			AccuAddChar(&a,'&');
			TrLo(error+1,*error," ","+");
			AccuAttributeValuePair(&a,GetRString(keyword,ERROR_KEYWORD),error);
			TrLo(error+1,*error,"+"," ");
			a.offset--;	// aavp adds one at the end
		}
		
		if (*explanation)
		{
			AccuAddChar(&a,'&');
			TrLo(explanation+1,*explanation," ","+");
			AccuAttributeValuePair(&a,GetRString(keyword,EXPLANATION_KEYWORD),explanation);
			TrLo(explanation+1,*explanation,"+"," ");
			a.offset--;	// aavp adds one at the end
		}
		
		AccuTrim(&a);
		if (!ParseProtocolFromURLPtr (LDRef (url), a.offset, keyword))
			OpenOtherURLPtr (keyword, *url, a.offset);
		ZapHandle(url);
	}
	
	return noErr;
}

/**********************************************************************
 * MemoryPreflight - do we have enough memory to do this?
 **********************************************************************/
OSErr MemoryPreflight(long size)
{
	long total, contig;
	
	if (!NoPreflight)
	{
		PurgeSpace(&total,&contig);
		if (size>total)
		{
			Aprintf(BIG_OK_ALRT,Stop,TOO_MUCH_MEMORY,(size-total)/(1 K));
			return(-108);
		}
	}
	return(0);
}

/**********************************************************************
 * DeepTrouble - tell the user that BAD things are happening
 **********************************************************************/
void DeepTrouble(UPtr str)
{
	MyParamText(str,"","","");
	InitCursor();
//	(void) StopAlert(OK_ALRT,nil);
	ComposeStdAlert(kAlertStopAlert,OK_ASTR+ALRTStringsStrn,P1,P2,P3,P4);
	ExitToShell();
}

/************************************************************************
 * SetAlertBeep - set whether or not an alert should beep
 ************************************************************************/
void SetAlertBeep(Boolean onOrOff)
{
	AlertBeep = onOrOff;
}

/************************************************************************
 * SilenceAlert - turn the beeps off in an alert template
 ************************************************************************/
void SilenceAlert(int template)
{
	AlertTHndl aTempl;

	if (aTempl=(AlertTHndl)GetResource_('ALRT',template))
		(*aTempl)->stages &= ~0x3333;
}

/************************************************************************
 * TrashAlert - get rid of a mucked alert template
 ************************************************************************/
void TrashAlert(int template)
{
	AlertTHndl aTempl;

	if (aTempl=(AlertTHndl)GetResource_('ALRT',template))
		ReleaseResource_(aTempl);
}

/************************************************************************
 * MommyMommy - get the user's attention
 ************************************************************************/
Boolean MommyMommy(short sId,UPtr string)
{
	NMRec nm;
	Str255 scratch;
	short pend;
	short nmResult;
	DECLARE_UPP(OkWhatIsIt,NM);
	
	if (!InBG) return(True);
	if (Dragging) return(False);	// darn drag mangler
	
	AttentionNeeded = true;

	Zero(nm);
	
	if (PrefIsSet(PREF_NEW_SOUND))
	{
		nm.nmSound =
#ifndef ONE
		 	*GetPref(scratch,PREF_ERROR_SOUND) ? GetNamedResource('snd ',scratch) :
#endif
			GetResource_('snd ',ATTENTION_SND);
		if (nm.nmSound) HNoPurge_(nm.nmSound);
	}
	INIT_UPP(OkWhatIsIt,NM);
	nm.nmResp = (void*)OkWhatIsItUPP;
	nm.nmStr = PrefIsSet(PREF_NEW_ALERT) ? (string ? string : (sId ? GetRString(scratch,sId) : nil)) : nil;
	nm.nmRefCon = (long) &pend;
	nm.nmMark = 1;
	if (!PrefIsSet(PREF_NO_APPLE_FLASH))
		GetIconSuite(&nm.nmIcon,EUDORA_SICN,svAllSmallData);
	nm.qType = nmType;
	nmResult = NMInstall(&nm);
	if (pend!=inProgress)
		pend=SpinOnLo(&InBG,0,True,False,true,true);
	if (!nmResult) NMRemove(&nm);
	if (nm.nmIcon) DisposeIconSuite(nm.nmIcon,True);
	if (nm.nmSound) HPurge(nm.nmSound);
	return(pend==0);
}

#pragma segment Main
/************************************************************************
 *
 ************************************************************************/
pascal void OkWhatIsIt(QElemPtr nmReqPtr)
{
	SLDisable();
	*(short *)((NMRec *)nmReqPtr)->nmRefCon = 0;
	SLEnable();
}
#pragma segment Util

/**********************************************************************
 * PtrPlusHand - PtrAndHand, using my own routines for resizing
 **********************************************************************/
OSErr PtrPlusHand(const void *pointer, Handle hand, long size)
{
	long newSize;
	
	if (size)
	{
		newSize = GetHandleSize(hand)+size;
		SetHandleBig(hand,newSize);
		if (MemError()) return(MemError());
		BMD(pointer,*hand+(newSize-size),size);
	}
	return(noErr);
}

/**********************************************************************
 * HandPlusHand - HandAndHand, using my own routines for resizing
 **********************************************************************/
OSErr HandPlusHand(Handle h1, Handle h2)
{
	long h1Size = GetHandleSize(h1);
	long h2Size = GetHandleSize(h2);
	
	if (h1Size)
	{
		SetHandleBig(h2,h1Size+h2Size);
		if (MemError()) return(MemError());
		BMD(*h1,*h2+h2Size,h1Size);
	}
	return(noErr);
}

/**********************************************************************
 * DupHandle - duplicate a handle
 * 							8.14.98  -- jp -- safely copy purgeable handles
 **********************************************************************/
Handle DupHandle(Handle inHandle)
{
	long len = GetHandleSize(inHandle);
	char flags = HGetState (inHandle);
	Handle outHandle;
	
	HNoPurge (inHandle);
	outHandle = NuHTempBetter(len);
	if (outHandle) BMD(*inHandle,*outHandle,len);
	HSetState (inHandle, flags);
	return(outHandle);
}

/**********************************************************************
 * MyHandToHand - handtohand using my allocation routines
 **********************************************************************/
OSErr MyHandToHand(Handle *inHandle)
{
	Handle result = DupHandle(*inHandle);
	if (!result) return(MemError());
	*inHandle = result;
	return(noErr);
}

/************************************************************************
 * SetHandleBig - set the size of a handle, carefully
 ************************************************************************/
void SetHandleBig(Handle h,long size)
{
	long grow;
	ASSERT(!(HGetState(h)&0xC0) || size<=GetHandleSize_(h));
	
	RANDOM_FAILURE_PROC;
	MemCanFail = True;
	
	/*
	 * are we running low on memory?
	 */
	if (size>GetHandleSize(h) && MonitorGrow(False))
	{
		{
			LMSetMemErr(-108);
			return;
		}
	}
	
	SetHandleSize(h,size);
	if (MemError())
	{
		MoveHHi(h);
		CompactMem(size);
		SetHandleSize(h,size);
		if (MemError())
		{
			MoveHHi(h);
			MaxMem(&grow);
			SetHandleSize(h,size);
		}
	}
	MemCanFail = False;
}

