/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ctb.h"
#define FILE_NUM 9
#ifdef CTB
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for i/o over a pseudo-telnet ctb stream
 * these functions are oriented toward CONVENIENCE, not performance
 ************************************************************************/

#pragma segment CTBTrans
#include <stdio.h>
static short CTBErr;
TransStream gCTBStream;

/************************************************************************
 * private functions
 ************************************************************************/
typedef struct {
	short pending;
	short errCode;
	CMBufferSizes asyncCount;
	short inUse;
} AsyncStuff, *AsyncStuffPtr;
Boolean CTBExists(void);
short ConfigureCTB(Boolean makeUser);
short CTBNavigateString(Uptr string);
short MyCMOpen(ConnHandle cn, long timeout);
short MyCMClose(ConnHandle cn);
short MyCMWrite(ConnHandle cn, UPtr buffer, long *size,
		AsyncStuffPtr callerStuff);
short MyCMBreak(ConnHandle cn, long howLong);
short MyCMRead(ConnHandle cn, UPtr buffer, long *size, long timeout);
typedef pascal void MyCTBCompleteType(ConnHandle cn);
SIMPLE_UPP(MyCTBComplete, ConnectionCompletion);
Handle GetNavResource(short id);
void CheckNavPWRes(Boolean * primary, Boolean * secondary, Handle resH);
long CTBDataCount(void);
void CTBAuxConfig(ConnHandle cnH, PStr toolName);
Boolean SilentCTB = False;
PStr StripHi(PStr string);
#define CTBTrouble(err) CTBTR(err,FNAME_STRN+FILE_NUM,__LINE__)
short CTBTR(short err, short file, short line);
#define ESC_AUXUSR 1
#define ESC_AUXUSRERR 2
#define ESC_AUXPW 4
#define ESC_USR 8
#define ESC_PW 16
short EscapeCopy(PStr to, PStr from, PStr expect, PStr hangup);

/************************************************************************
 * InitCTB - initialize the CommToolbox
 ************************************************************************/
Boolean InitCTB(Boolean makeUser)
{
	static int CTBInitted = 0;

	ASSERT(gCTBStream);

	CTBInitted = MIN(CTBInitted, 4);
	CTBSilenceTrans(gCTBStream, False);
	switch (CTBInitted) {
	case 0:
		if (!CTBExists()) {
			WarnUser(NO_CTB, 0);
			return (False);
		}
		CTBInitted++;
	case 1:
		if (CTBErr = InitCTBUtilities()) {
			WarnUser(NO_CTBU, CTBErr);
			return (False);
		}
		CTBInitted++;
	case 2:
		if (CTBErr = InitCRM()) {
			WarnUser(NO_CTBRM, CTBErr);
			return (False);
		}
		CTBInitted++;
	case 3:
		if (CTBErr = InitCM()) {
			WarnUser(NO_CTBCM, CTBErr);
			return (False);
		}
		CTBInitted++;
	case 4:
		if (CTBErr = ConfigureCTB(makeUser))
			return (False);
		CTBInitted++;
	}
	return (True);
}

/************************************************************************
 * CTBExists - is the CTB installed?
 ************************************************************************/
Boolean CTBExists(void)
{
#define CTBTrap 0x8b
#define UnimplTrap 0x9f
	return (NGetTrapAddress(UnimplTrap, OSTrap) !=
		NGetTrapAddress(CTBTrap, OSTrap));
}

/************************************************************************
 * DialThePhone - create a CTB connection
 ************************************************************************/
short DialThePhone(TransStream stream)
{
	CMStatFlags flags;
	CMBufferSizes sizes;

	if (!CnH || CMStatus(CnH, sizes, &flags)
	    || !(flags & cmStatusOpen)) {
		ProgressR(NoChange, NoChange, 0, INIT_CTB, nil);

		gCTBStream = stream;
		gCTBStream->Opening = True;
		if (!InitCTB(False))
			return (CTBErr ? CTBErr : cmGenericError);

		gCTBStream->RcvSpot = -1;
		if (!gCTBStream->RcvBuffer
		    && (gCTBStream->RcvBuffer =
			NuHandle(GetRLong(RCV_BUFFER_SIZE))) == nil) {
			CTBErr = MemError();
			WarnUser(MEM_ERR, CTBErr);
			return (CTBErr);
		}
		ProgressR(NoChange, NoChange, 0, MAKE_CONNECT, nil);
		CTBErr = MyCMOpen(CnH, 0 /* 60*GetRLong(OPEN_TIMEOUT) */ );
		gCTBStream->Opening = False;
		if (CTBErr) {
			CMDispose(CnH);
			CnH = nil;
			return (CTBErr);
		}
		return (CTBNavigateSTRN(NAVIN));
	}
	return (0);
}

/************************************************************************
 * CTBConnectTrans - connect to the remote host.	This version uses the CTB.
 ************************************************************************/
OSErr CTBConnectTrans(TransStream stream, UPtr serverName, short port,
		      Boolean silently)
{
#pragma unused (stream)
#pragma unused (silently)
	Str127 scratch;

#ifdef DEBUG
	if (BUG12)
		port += 10000;
#endif
	ComposeRString(scratch, TS_CONNECT_FMT, serverName, (uLong) port);
	ProgressMessage(kpMessage, scratch);
	CTBSendTrans(nil, scratch + 1, *scratch, nil);

	return (CTBErr);
}

/************************************************************************
 * CTBSendTrans - send some text to the remote host.	This version uses the CTB.
 ************************************************************************/
OSErr CTBSendTrans(TransStream stream, UPtr text, long size, ...)
{
#pragma unused (stream)
	long bSize;

	if (size == 0)
		return (noErr);	/* allow vacuous sends */
	CycleBalls();

	bSize = size;
	if (CTBErr = MyCMWrite(CnH, text, &bSize, nil))
		return (CTBErr);
	{
		Uptr buffer;
		va_list extra_buffers;
		va_start(extra_buffers, size);
		while (buffer = va_arg(extra_buffers, UPtr)) {
			CycleBalls();
			bSize = va_arg(extra_buffers, int);
			if (CTBErr = CTBSendTrans(nil, buffer, bSize, nil))
				break;
		}
		va_end(extra_buffers);
	}
	return (CTBErr);
}

/************************************************************************
 * CTBRecvTrans - get some text from the remote host.  This version uses the CTB.
 ************************************************************************/
OSErr CTBRecvTrans(TransStream stream, UPtr line, long *size)
{
#pragma unused (stream)
	long gotThis;
	Str31 tmStr;
	short factor = 60;
	UPtr spot = line;
	long want = *size;

	NumToString(CTBTimeout, tmStr);
	MyCMIdle();
	do {
		CTBErr =
		    SpinOn(&CTBHasChars, CTBTimeout * factor, True, False);
		if (CTBErr)
			CTBErr = cmTimeOut;
		else {
			factor = 10;
			gotThis = CTBDataCount();
			gotThis = MIN(gotThis, want);
			CTBErr = MyCMRead(CnH, line, &gotThis, 0);
			if (!CTBErr) {
				spot += gotThis;
				want -= gotThis;
			}
		}
	}
	while (want && spot == line && !SilentCTB && !CommandPeriod &&
	       AlertStr(TIMEOUT_ALRT, Caution, tmStr) == 1);

	*size = spot - line;
	if (!CTBErr && *size == 0)
		CTBErr = cmTimeOut;
	if (!CTBErr && *size)
		ResetAlrtStage();
	return (CTBErr);
}

/************************************************************************
 *
 ************************************************************************/
void HangUpThePhone()
{
	ASSERT(gCTBStream);

	if (CnH) {
		(void) CTBNavigateSTRN(NAVOUT);
		ProgressMessageR(kpSubTitle, CTB_CLOSING);
		if (!PrefIsSet(PREF_NO_HANGUP))
			(void) MyCMClose(CnH);
	}
	if (CnH) {
		CMDispose(CnH);
		CnH = nil;
	}
	if (gCTBStream->RcvBuffer)
		ZapHandle(gCTBStream->RcvBuffer);
	gCTBStream = 0;
}

/************************************************************************
 * CTBDisTrans - disconnect from the remote host.  This version uses the CTB.
 ************************************************************************/
OSErr CTBDisTrans(TransStream stream)
{
#pragma unused (stream)
	return (noErr);
}

/************************************************************************
 * CTBDestroyTrans - destory the connection.
 ************************************************************************/
OSErr CTBDestroyTrans(TransStream stream)
{
#pragma unused (stream)
	return (noErr);
}

/************************************************************************
 * CTBTransError - report our most recent error
 ************************************************************************/
OSErr CTBTransError(TransStream stream)
{
#pragma unused (stream)
	return (CTBErr);
}

/************************************************************************
 * CTBSilenceTrans - turn off error reports from tcp routines
 ************************************************************************/
void CTBSilenceTrans(TransStream stream, Boolean silence)
{
#pragma unused (stream)
	SilentCTB = silence;
}

/************************************************************************
 * ConfigureCTB - configure the communications toolbox
 ************************************************************************/
short ConfigureCTB(Boolean makeUser)
{
	Str63 toolName;
	short procID;
	CMBufferSizes bs;
	Point pt;
	short result;
	Handle cfigH;
	Boolean needConfig = True;

	CTBErr = 0;

	/*
	 * find the tool to use
	 */
	if (!*GetRStr(toolName, CTB_TOOL_STR))
		return (1);

	if ((procID = CMGetProcID(toolName)) < 0) {
		CRMGetIndToolName(classCM, 1, toolName);
		if (!*toolName)
			return (WarnUser(NO_CTB_TOOLS, 1));
		else if ((procID = CMGetProcID(toolName)) < 0)
			return (WarnUser(COULDNT_GET_TOOL, 1));
	}

	/*
	 * create a connection record
	 */
	WriteZero(bs, sizeof(bs));
	if (!(CnH = CMNew(procID,
			  cmNoMenus | (PrefIsSet(PREF_AUTO_DISMISS) ?
				       cmQuiet : 0), bs, nil, nil))) {
		WarnUser(MEM_ERR, MemError());
		return (1);
	}

	/*
	 * try to load the old configuration
	 */
	if (cfigH = GetNamedResource(CTB_CFIG_TYPE, toolName)) {
		if (GetPtrSize((*CnH)->config) == GetHandleSize_(cfigH)) {
			BMD(*cfigH, (*CnH)->config,
			    GetPtrSize((*CnH)->config));
			BMD(*cfigH, (*CnH)->oldConfig,
			    GetPtrSize((*CnH)->config));
			needConfig = CMValidate(CnH);
		}
		ReleaseResource_(cfigH);
	}

	/*
	 * let the user have a look at things
	 */
	if (makeUser || needConfig) {
		pt.h = 10;
		pt.v = GetMBarHeight() + 10;
		UseResFile(0);	/* make sure no Eudora rsrcs interfere */
		result = CMChoose(&CnH, pt, nil);
		UseResFile(SettingsRefN);
		switch (result) {
		case chooseDisaster:
		case chooseFailed:
		case chooseAborted:
			CTBErr = result;
			break;
		case chooseOKMajor:
		case chooseOKMinor:
			/* save the name string */
			CMGetToolName((*CnH)->procID, toolName);
			CTBAuxConfig(CnH, toolName);
			BMD((*CnH)->config, (*CnH)->oldConfig,
			    GetPtrSize((*CnH)->config));
			if (CTBErr =
			    SettingsPtr('STR ', nil, CTB_TOOL_STR,
					toolName, *toolName + 1))
				WarnUser(WRITE_SETTINGS, CTBErr);
			else if (CTBErr =
				 SettingsPtr(CTB_CFIG_TYPE, toolName,
					     CTB_CFIG_ID, (*CnH)->config,
					     GetPtrSize((*CnH)->config)))
				WarnUser(WRITE_SETTINGS, CTBErr);
			break;
		}
	}

	/*
	 * just configuring?
	 */
	if (CnH)
		if (makeUser) {
			CMDispose(CnH);
			CnH = nil;
		}

	return (CTBErr);
}

/************************************************************************
 * CTBAuxConfig - allow the user to set extra parameters
 ************************************************************************/
void CTBAuxConfig(ConnHandle cnH, PStr toolName)
{
	Str255 resName;
	Handle resH = nil;
	long zero = 0;

	ComposeRString(resName, CTB_SETCONFIG_FMT, toolName);

	if ((resH = GetNamedResource('TEXT', resName)) ||
	    (resH =
	     GetNamedResource('TEXT', GetRString(resName, SETCONFIG)))) {
		PtrPlusHand_(&zero, resH, 1);
		(void) CMSetConfig(cnH, LDRef(resH));
		ReleaseResource_(resH);
	}
}


/************************************************************************
 * CTBNavigateSTRN - navigate a serial connection by means of an STRN resource
 ************************************************************************/
short CTBNavigateSTRN(short id)
{
	short count, n;
	Str63 name;
	Uhandle strnH;
	Str255 string;
	ResType theType;
	Boolean oldSilence = SilentCTB;
	short err = noErr;

	ASSERT(gCTBStream);

	strnH = GetNavResource(id);

	if (strnH) {
		CTBTimeout = GetRLong(SHORT_TIMEOUT);
		count = (*strnH)[0] * 256 + (*strnH)[1];
		GetResInfo(strnH, &id, &theType, name);
		ReleaseResource_(strnH);
		ComposeLogR(LOG_NAV, nil, NAV_BEGIN, name, id, count);

		CTBSilenceTrans(gCTBStream, True);
		for (n = 1; n <= count; n++) {
			GetIndString(string, id, n);
			if (err = CTBNavigateString(string))
				break;
		}
		CTBSilenceTrans(gCTBStream, oldSilence);
		CTBTimeout =
		    InAThread()? GetRLong(THREAD_RECV_TIMEOUT) :
		    GetRLong(RECV_TIMEOUT);
		if (err != cmTimeOut)
			CTBTrouble(err);
	}

	return (err == cmTimeOut ? noErr : err);
}

/************************************************************************
 * GetNavResource - get a navigation resource
 ************************************************************************/
Handle GetNavResource(short id)
{
	Handle strnH;
	Str255 name, string;

	GetRString(name, id);
	GetRStr(string, CTB_TOOL_STR);
	PCatC(string, ' ');
	PCat(string, name);
	strnH = GetNamedResource('STR#', string);
	if (!strnH)
		strnH = GetNamedResource('STR#', name);
	if (!strnH)
		strnH =
		    GetResource_('STR#',
				 id == NAVIN ? CTB_NAV_IN_STRN : (id ==
								  NAVMID ?
								  CTB_NAV_MID_STRN
								  :
								  CTB_NAV_OUT_STRN));
	return (strnH);
}

/************************************************************************
 * CTBNavigateString - navigate a serial connection by a string
 ************************************************************************/
short CTBNavigateString(Uptr string)
{
	Str255 data, scratch, expect, hangup;
	long bSize;
	long tics;
	UPtr exSpot, dSpot, hangSpot;
	Boolean found = False, brk = False, wait = False, doHangup = False;
	short escFlags;

	ASSERT(gCTBStream);

	escFlags = EscapeCopy(data, string, expect, hangup);
	StripHi(expect);
	StripHi(hangup);

	/*
	 * user might have cancelled when entering password above
	 */
	if (CommandPeriod)
		return (userCancelled);

	/*
	 * no dialup username?
	 */
	if (escFlags & ESC_AUXUSRERR) {
		WarnUser(NO_AUXUSR, 0);
		return (userCancelled);
	}

	/*
	 * Progress.  If bullet, we hide
	 */
	if (*data && data[1] == bulletChar) {
		ProgressMessageR(kpMessage, SECRET);
		BMD(data + 2, data + 1, *data - 1);
		--*data;
	} else if (*data
		   && ((brk = data[1] == betaChar)
		       || (wait = data[1] == deltaChar))) {
		BMD(data + 2, data + 1, *data);
		--*data;
		tics = Atoi(data + 1);
		if (wait)
			tics *= 60;
		ProgressMessageR(kpMessage, brk ? BREAKING : WAITING);
	} else {
		ProgressMessage(kpMessage, data);
	}

	/*
	 * log our message
	 */
	CarefulLog(LOG_NAV, LOG_SENT, data + 1, *data);

	/*
	 * send a break
	 */
	if (brk) {
		if (CTBErr = MyCMBreak(CnH, tics))
			return (CTBErr);
	}

	/*
	 * delay
	 */
	else if (wait)
		for (tics += TickCount(); TickCount() < tics;)
			MiniEvents();

	/*
	 * send some data
	 */
	else if (CTBErr = CTBSendTrans(nil, data + 1, *data, nil))
		return (CTBErr);

	/*
	 * are we looking for a response?
	 */
	if (*expect) {
		CarefulLog(LOG_NAV, LOG_EXPECT, expect + 1, *expect);
		tics =
		    TickCount() +
		    (InAThread()? GetRLong(THREAD_RECV_TIMEOUT) :
		     GetRLong(RECV_TIMEOUT)) * 60;
		exSpot = expect + 1;
	}

	/*
	 * are we looking for something that will make us give up?
	 */
	if (*hangup) {
		CarefulLog(LOG_NAV, LOG_HANGUP, hangup + 1, *hangup);
		tics =
		    TickCount() +
		    (InAThread()? GetRLong(THREAD_RECV_TIMEOUT) :
		     GetRLong(RECV_TIMEOUT)) * 60;
		hangSpot = hangup + 1;
	}

	/*
	 * now, look!
	 */
	do {
		/*
		 * read response
		 */
		bSize = sizeof(data) - 1;
		CTBErr = RecvLine(gCTBStream, data + 1, &bSize);

		/*
		 * got a response
		 */
		if (bSize) {
			data[0] = bSize;
			ProgressMessage(kpMessage, data);
			CarefulLog(LOG_NAV, LOG_GOT, data + 1, *data);

			/*
			 * scan for expect or hangup strings
			 */
			if (*expect || *hangup) {
				for (dSpot = data + 1;
				     dSpot <= data + *data; dSpot++) {
					if (*expect)
						if ((*dSpot & 0x7f) ==
						    *exSpot) {
							if (++exSpot >
							    expect +
							    *expect) {
								ComposeLogR
								    (LOG_NAV,
								     nil,
								     LOG_FOUND);
								found =
								    True;
								break;
							}
						} else
							exSpot =
							    expect + 1;
					if (*hangup)
						if ((*dSpot & 0x7f) ==
						    *hangSpot) {
							if (++hangSpot >
							    hangup +
							    *hangup) {
								ComposeLogR
								    (LOG_NAV,
								     nil,
								     LOG_HANGUP_FOUND);
								doHangup =
								    True;
								break;
							}
						} else
							hangSpot =
							    hangup + 1;
				}
			}
		}

		/*
		 * timeout looking for an expect string
		 */
		if (*expect && TickCount() > tics) {
			GetRString(scratch,
				   InAThread()? THREAD_RECV_TIMEOUT :
				   RECV_TIMEOUT);
			CTBSilenceTrans(gCTBStream, False);

			/*
			 * does the user want us to try some more?
			 */
			if (AlertStr(TIMEOUT_ALRT, Caution, scratch) == 1)
				tics = TickCount() + 60 * ((InAThread()? GetRLong(THREAD_RECV_TIMEOUT) : GetRLong(RECV_TIMEOUT)));	/* Yes, try more */
			else {
				ComposeLogR(LOG_NAV, nil, LOG_NOTFOUND);	/* No, give up now */
				CTBErr = userCancelled;
			}
			CTBSilenceTrans(gCTBStream, True);
		}
	}
	while (!CommandPeriod
	       && (!CTBErr || CTBErr == cmTimeOut && *expect) && !found
	       && !doHangup);

	if (CTBErr == cmTimeOut)
		CTBErr = 0;

	/*
	 * We gave up on an expect string, or hit the hangup string
	 */
	if (*expect && !found || doHangup) {
		CTBErr = userCancelled;
		InvalidatePasswords(0 == (escFlags & ESC_PW),
				    0 == (escFlags & ESC_AUXPW), False);
	}
	return (CTBErr);
}


/**********************************************************************
 * StripHi - strip the high bits from a string
 **********************************************************************/
PStr StripHi(PStr string)
{
	UPtr spot;

	for (spot = string + *string; spot > string; spot--)
		*spot &= 0x7f;	/* strip parity */
	return (string);
}

/************************************************************************
 * MyCMClose - call CMClose
 ************************************************************************/
short MyCMClose(ConnHandle cn)
{
	AsyncStuff stuff;
	short err;		/* real error */

	DebugStr11("\pMyCMClose;g");
	(*cn)->refCon = (long) &stuff;	/* store address */
	stuff.pending = 1;
	if (PrefIsSet(PREF_SYNCH_CTB))
		err = CMClose(cn, False, nil, -1, True);
	else {
		(void) CMIOKill(cn, cmDataIn);
		(void) CMIOKill(cn, cmDataOut);
		if (!
		    (err =
		     CMClose(cn, True, MyCTBCompleteUPP, -1, False))) {
			err = SpinOn(&stuff.pending, 0, True, True);
			if (!err)
				err = stuff.errCode;
		}
	}
	CTBTrouble(err);
	return (err);		/* error? */
}

/************************************************************************
 * MyCMOpen - call CMOpen
 ************************************************************************/
short MyCMOpen(ConnHandle cn, long timeout)
{
	AsyncStuff stuff;
	short err;		/* real error */

	DebugStr11("\pMyCMOpen;g");
	(*cn)->refCon = (long) &stuff;	/* store address */
	stuff.pending = 1;
	if (PrefIsSet(PREF_SYNCH_CTB))
		err = CMOpen(cn, False, MyCTBCompleteUPP, timeout);
	else {
		if (!(err = CMOpen(cn, True, MyCTBCompleteUPP, timeout))) {
			err = SpinOn(&stuff.pending, 0, True, False);
			if (!err)
				err = stuff.errCode;
			if (err == userCancelled)
				(void) CMAbort(CnH);
		}
	}

	CTBTrouble(err);
	return (err);
}

/************************************************************************
 * MyCMRead - call CMRead
 ************************************************************************/
short MyCMRead(ConnHandle cn, UPtr buffer, long *size, long timeout)
{
	short err;		/* real error */
	CMFlags junk;

	DebugStr11("\pMyCMRead;g");
#ifdef DEBUG
	if (BUG11)
		Dprintf("\p%d bytes;g", *size);
#endif

#ifdef APPLE_FIXES_EM_TOOL
	AsyncStuff stuff;

	(*cn)->refCon = (long) &stuff;
	stuff.pending = 1;
	if (!
	    (err =
	     CMRead(cn, buffer, size, cmData, True, MyCTBCompleteUPP,
		    timeout, &junk))) {
		err = SpinOn(&stuff.pending, 0, False);
		if (!err)
			err = stuff.errCode;
		if (err == userCancelled)
			CMIOKill(cn, cmDataIn);
	}

	if (!err || err == cmTimeOut) {
		*size = stuff.asyncCount[cmDataIn];
		if (*size)
			err = noErr;
		else
			err = cmTimeOut;
	} else
		*size = 0;
#else
	DebugStr11("\pCMRead call;g");
	err = CMRead(cn, buffer, size, cmData, False, nil, timeout, &junk);
	DebugStr11("\pCMRead return;g");
#endif


	if (*size && LogLevel & LOG_TRANS)
		CarefulLog(LOG_TRANS, LOG_GOT, buffer, *size);
	if (err != cmTimeOut)
		CTBTrouble(err);
	return (err);
}

/************************************************************************
 * MyCMWrite - call CMWrite
 ************************************************************************/
short MyCMWrite(ConnHandle cn, UPtr buffer, long *size,
		AsyncStuffPtr callerStuff)
{
	AsyncStuffPtr stuff;
	AsyncStuff localStuff;
	short err;		/* real error */

	DebugStr11("\pMyCMWrite;g");
	stuff = callerStuff ? callerStuff : &localStuff;
	(*cn)->refCon = (long) stuff;
	stuff->pending = 1;
	if (LogLevel & LOG_TRANS)
		CarefulLog(LOG_TRANS, LOG_SENT, buffer, *size);
	if (!
	    (err =
	     CMWrite(cn, buffer, size, cmData, True, MyCTBCompleteUPP,
		     3600, 0))) {
		if (callerStuff)
			return (noErr);
		err = SpinOn(&stuff->pending, 0, True, False);
		if (!err)
			err = stuff->errCode;
		if (err == userCancelled)
			CMIOKill(cn, cmDataOut);
	}

	if (!err)
		*size = stuff->asyncCount[cmDataOut];
	else
		*size = 0;

	CTBTrouble(err);
	return (err);
}

/************************************************************************
 * MyCMBreak - call CMBreak
 ************************************************************************/
short MyCMBreak(ConnHandle cn, long howLong)
{
	AsyncStuff stuff;
	short err;		/* real error */
	(*cn)->refCon = (long) &stuff;
	stuff.pending = 1;
	CMBreak(cn, howLong, True, MyCTBCompleteUPP);

	err = SpinOn(&stuff.pending, 0, True, False);
	if (!err)
		err = stuff.errCode;
	if (err == userCancelled)
		CMIOKill(cn, cmDataOut);

	CTBTrouble(err);
	return (err);
}

/************************************************************************
 * CTBTR - report a CTB error
 ************************************************************************/
short CTBTR(short err, short file, short line)
{
	Uhandle strnH;

	ASSERT(gCTBStream);

	if (!SilentCTB && err
	    && (!(CommandPeriod || err == userCancelled)
		|| gCTBStream->Opening)) {
		strnH = GetResource_('STR#', CTB_ERR_STRN);
		if (strnH) {
			Str127 errString, debugStr, message;
			if (err + 2 > 0
			    && err + 2 <= (*strnH)[0] * 256 + (*strnH)[1])
				GetRString(errString,
					   CTB_ERR_STRN + err + 2);
			else
				NumToString(err, errString);
			ReleaseResource_(strnH);
			GetRString(message, CTB_PROBLEM);
			ComposeRString(debugStr, FILE_LINE_FMT, file,
				       line);
			MyParamText(message, errString, "", debugStr);
			if (gCTBStream->Opening) {
				if (2 ==
				    ReallyDoAnAlert(OPEN_ERR_ALRT,
						    Caution))
					SetPref(PREF_OFFLINE, YesStr);
			} else
				ReallyDoAnAlert(OK_ALRT, Caution);
			CTBSilenceTrans(gCTBStream, True);
		}
	}
	return (err);
}

/************************************************************************
 * CTBWhoAmI - return the mac's ctb name
 ************************************************************************/
UPtr CTBWhoAmI(TransStream stream, Uptr who)
{
#pragma unused (stream)
	return (GetRString(who, CTB_ME));
}

/************************************************************************
 * MyCMIdle - do the things I should do at CMIdle time
 ************************************************************************/
void MyCMIdle(void)
{
	CMIdle(CnH);
	CTBHasChars = CTBDataCount()? noErr : inProgress;
}

/************************************************************************
 * CTBDataCount - how much data do we have?
 ************************************************************************/
long CTBDataCount(void)
{
	CMStatFlags flags;
	CMBufferSizes sizes;

	CMStatus(CnH, sizes, &flags);
	return (flags & cmStatusDataAvail ? sizes[cmDataIn] : 0);
}

/************************************************************************
 * GetSecondPass - get the user's secondary password
 ************************************************************************/
short GetSecondPass(UPtr pass)
{
	Str31 save;
	short err;
	Str31 dialup;

	if (!*(*CurPers)->secondPass) {
		PCopy(save, (*CurPers)->password);
		*(*CurPers)->password = 0;
		GetRString(dialup, DIALUP);
		if (!(err = GetPassword(dialup, pass, 32, ENTER))) {
			PCopy((*CurPers)->secondPass, pass);
			if (PrefIsSet(PREF_SAVE_PASSWORD)
			    && *(*CurPers)->secondPass) {
				PCopy(dialup, (*CurPers)->secondPass);
				SetPref(PREF_AUXPW, dialup);
				MyUpdateResFile(SettingsRefN);
			}
		}
		PCopy((*CurPers)->password, save);
		return (err);
	} else {
		PCopy(pass, (*CurPers)->secondPass);
		return (noErr);
	}
}


/************************************************************************
 * CheckNavPW - see if the nav strings need passwords
 ************************************************************************/
void CheckNavPW(Boolean * primary, Boolean * secondary)
{
	CheckNavPWRes(primary, secondary, GetNavResource(NAVIN));
	CheckNavPWRes(primary, secondary, GetNavResource(NAVOUT));
	CheckNavPWRes(primary, secondary, GetNavResource(NAVMID));
}


/************************************************************************
 * CheckNavPWRes - see if a particular resource needs passwords
 ************************************************************************/
void CheckNavPWRes(Boolean * primary, Boolean * secondary, Handle strnH)
{
	short n;
	Str255 s;
	short id;
	OSType theType;
	UPtr spot;

	if (!strnH)
		return;
	GetResInfo(strnH, &id, &theType, s);
	n = CountStrn(id);
	while (n && (!*primary || !*secondary)) {
		GetIndString(s, id, n--);
		if (spot = PPtrFindSub("\p\\p", s + 1, *s)) {
			if (spot[1] == 'p')
				*primary = True;
			else
				*secondary = True;
		}
	}
}


#pragma segment Transport
/************************************************************************
 * EscapeCopy - copy a string, expanding escapes
 ************************************************************************/
short EscapeCopy(PStr to, PStr from, PStr expect, PStr hangup)
{
	UPtr end = from + *from;
	UPtr spot = to + 1;
	Boolean esc;
	Str127 scratch1, scratch2;
	short returnVal = 0;
	Str63 prompt;
	UPtr slash;

	GetRString(prompt, ANSWER_RESPONSE);

	*expect = *hangup = 0;
	from++;
	for (esc = false; from <= end; from++) {
		if (esc) {
			esc = False;
			switch (*from) {
			case 'U':
				GetPref(scratch1, PREF_AUXUSR);
				BMD(scratch1 + 1, spot, *scratch1);
				spot += *scratch1;
				returnVal |= ESC_AUXUSR;
				if (!*scratch1)
					returnVal |= ESC_AUXUSRERR;
				break;
			case 'P':
				if (GetSecondPass(scratch1)) {
					CommandPeriod = True;
					if (ProgressIsOpen())
						PressStop();
					break;
				}
				BMD(scratch1 + 1, spot, *scratch1);
				spot += *scratch1;
				returnVal |= ESC_AUXPW;
				break;
			case 'u':
				GetPOPInfo(scratch1, scratch2);
				BMD(scratch1 + 1, spot, *scratch1);
				spot += *scratch1;
				returnVal |= ESC_USR;
				break;
			case 'h':
				GetPOPInfo(scratch1, scratch2);
				BMD(scratch2 + 1, spot, *scratch2);
				spot += *scratch2;
				break;
			case 's':
				GetSMTPInfo(scratch1);
				BMD(scratch1 + 1, spot, *scratch1);
				spot += *scratch1;
				break;
			case '^':
				for (slash = from + 1; slash < end;
				     slash++)
					if (*slash == 92 && slash[1] != 92)
						break;
				MakePStr(prompt, from + 1,
					 slash - from - 1);
				from = slash - 1;
				break;
			case 'a':
				if (adlOk == GetAnswer(prompt, scratch1)) {
					BMD(scratch1 + 1, spot, *scratch1);
					spot += *scratch1;
				} else {
					CommandPeriod = True;
					if (ProgressIsOpen())
						PressStop();
				}
				break;
			case 'e':	/* switch to 'expect' string */
				*to = spot - to - 1;
				to[*to + 1] = 0;
				to = expect;
				spot = to + 1;
				break;
			case 'c':	/* switch to 'hangup' string */
				*to = spot - to - 1;
				to[*to + 1] = 0;
				to = hangup;
				spot = to + 1;
				break;
			case 'p':
				BMD((*CurPers)->password + 1, spot,
				    *(*CurPers)->password);
				spot += *(*CurPers)->password;
				returnVal |= ESC_PW;
				break;
			case 'n':
				*spot++ = '\012';
				break;
			case 'r':
				*spot++ = '\015';
				break;
			case 'b':
				*spot++ = bulletChar;
				break;
			case 'B':
				*spot++ = betaChar;
				break;
			case 'D':
				*spot++ = deltaChar;
				break;
			default:
				*spot++ = *from;
				break;
			}
		} else {
			esc = *from == '\\';
			if (!esc)
				*spot++ = *from;
		}
	}
	*to = spot - to - 1;
	to[*to + 1] = 0;	/* null terminate, just for fun */
	return (returnVal);
}

#pragma segment Main

/************************************************************************
 * MyCTBComplete - completion routine
 ************************************************************************/
pascal void MyCTBComplete(ConnHandle cn)
{
	AsyncStuffPtr stuffP = (AsyncStuffPtr) (*cn)->refCon;
	uLong *fp, *tp, *ep;

	//DebugStr11("\pComplete!;g");  // Ok, this was stupid.  Can't look at
	//ASSERT(!BugFlags);                                            // globals here, because no A5
	stuffP->errCode = (*cn)->errCode;
	fp = (uLong *) (*cn)->asyncCount;
	tp = (uLong *) stuffP->asyncCount;
	ep = tp + sizeof(CMBufferSizes) / sizeof(uLong);
	while (tp < ep)
		*tp++ = *fp++;
	stuffP->pending = 0;
}
#endif
