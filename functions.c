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

#include <AEObjects.h>
#include <ColorPicker.h>
#include <Resources.h>
#include <string.h>

#include <conf.h>
#include <mydefs.h>

#include "functions.h"
// #include "regcode_v2.h" CK
#define FILE_NUM 15
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Misc

Boolean CloseRemaining(WindowPtr theWindow);
short ReallyChangePassword(TransStream stream, UPtr user, UPtr host,
			   UPtr old, UPtr new);
short SendPwCommand(TransStream stream, short cmd, UPtr arg);
short CheckResult(TransStream stream, UPtr buffer);
void PICTUpdate(MyWindowPtr win);
void PICTDidResize(MyWindowPtr win, Rect * oldContR);
void PICTRect(short id, Rect * r);
OSErr AddWSService(FSSpecPtr spec, PStr menuItem);
OSErr GetWSMenuItem(AEAddressDescPtr aead, PStr itemText);
OSErr GetAppProperty(AEAddressDescPtr aead, DescType prop,
		     DescType wantType, AEDescPtr ad);
OSErr VerifySpeller(short item);
OSErr StartSpeller(short item, Boolean runningOnly);
OSErr Spell(short item, AEDescPtr objAD);
void JustArrow(Point mouse);
OSErr GetProperty(AEAddressDescPtr aead, AEDescPtr objAD,
		  DescType propType, DescType wantType, AEDescPtr ad);
Boolean IsSigWindow(WindowPtr theWindow);

/**********************************************************************
 * DoQuit - let's get outta here
 **********************************************************************/
void DoQuit(Boolean changeUserState)
{
	uLong never = 3600 * GetRLong(NEVER_WARN);
	Str31 hours;
	uLong now = GMTDateTime();
	Boolean remember = !EjectBuckaroo;


	Done = EjectBuckaroo = False;
	AmQuitting = True;

#ifdef THREADING_ON
// may want to have a warning preference?
	if (GetNumBackgroundThreads())
		switch (AlertStr(QUIT_THREAD_RUN_ALRT, Caution, nil)) {
		case QTR_WAIT:
			// JDB 12/8/98. Do some processing while waiting to quit.  This allows
			// OT/PPP to finish connecting, if the user picked a bad time to leave.

			// JDB 17/8/04. Wait for all open IMAP mailboxes to finish up as well.
			while (!IMAPDoQuit() || GetNumBackgroundThreads()) {
				SetMyCursor(watchCursor);
				MiniEvents();
				if (CommandPeriod) {
					AmQuitting = False;
					return;
				}
			}
			break;
		case QTR_DONT:
			KillThreads();
			break;
		case QTR_CANCEL:
			AmQuitting = False;
			return;
		}
#endif

	if (!PrefIsSet(PREF_EASY_QUIT) && !Offline && ForceSend > now
	    && ForceSend - now < never && !changeUserState) {
		GetRString(hours, NEVER_WARN);
		switch (AlertStr(QUIT_MQ_ALRT, Caution, hours)) {
		case QMQ_SEND_ALL:
			WarpQueue(0);
			XferMail(False, True, False, False, False, 0);
			break;
		case QMQ_SEND:
			WarpQueue(never);
			XferMail(False, True, False, False, False, 0);
			break;
		case QMQ_DONT:
			break;
		default:
			AmQuitting = False;
			return;
		}
	}
	if (!PrefIsSet(PREF_EASY_QUIT) && !Offline && !changeUserState) {
		SetSendQueue();
		if (SendQueue) {
			switch (ReallyDoAnAlert(QUIT_QUEUE_ALRT, Caution)) {
			case QQL_SEND:
				XferMail(False, True, False, False, False,
					 0);
				break;
			case QQL_QUIT:
				break;
			default:
				AmQuitting = False;
				return;
			}
		}
	}
	NoMenus = True;
	EnableMenus(nil, false);
	if (remember)
		RememberOpenWindows();

	// execute all pending IMAP commands
	ExecuteAllPendingIMAPCommands();

	// empty the IMAP trash now if we're asked to, before our windows get closed and our network is killed.
	if (PrefIsSet(PREF_AUTO_EMPTY) && IMAPExists() && AutoCheckOK())
		EmptyImapTrashes(kEmptyAutoCheckTrashes);

	if (!CloseAll(True)) {
		AmQuitting = False;
		return;
	}
	// Kill Kerberos if the preference is set no matter if the dominant personailty is using Kerberos or not.
	if (KerberosWasUsed() && PrefIsSet(PREF_DIE_DOG))
		KerbDestroy();
	if (KerberosWasUsed() && PrefIsSet(PREF_DIE_DOG_USER))
		KerbDestroyUser();

#ifdef TWO
	CleanTempFolder();
#endif
#ifdef WINTERTREE
	if (HasFeature(featureSpellChecking) && SpellSession)
		SpellClose(true);
#endif

	/*
	 * account for face time in audit log
	 */
	if (FMBMain) {
		uLong face = 0, rear = 0, connect = 0, total = 0;

		FaceMeasureReport(FMBMain, &face, &rear, &connect, &total);
		AuditShutdown(face, rear, connect, total);
		ZapFaceMeasure(FMBMain);
	}
	AdShutdown();
	ShutdownStats();
	OutgoingMIDListSave();

	Done = True;
}

/************************************************************************
 * CloseAll - close all windows
 ************************************************************************/
Boolean CloseAll(Boolean toolbarToo)
{
	if (!CloseAllMessages())
		return (False);
	if (!CloseAllBoxes())
		return (False);
#ifdef	FLOAT_WIN
	if (!CloseRemaining(FrontWindow()))
		return (False);
#else				//FLOAT_WIN
	if (!CloseRemaining(FrontWindow_()))
		return (False);
#endif				//FLOAT_WIN
	if (toolbarToo) {
		if (ToolbarShowing())
			OpenToolbar();
		CloseAdWindow();
	}
	return (True);
}

/************************************************************************
 * CloseRemaining - get rid of any other windows there might be
 ************************************************************************/
Boolean CloseRemaining(WindowPtr theWindow)
{
	if (!theWindow)
		return (True);
	if (!CloseRemaining(GetNextWindow(theWindow)))
		return (False);
	if (IsKnownWindowMyWindow(theWindow)) {
		if ((GetWindowKind(theWindow) == TBAR_WIN)
		    || (GetWindowKind(theWindow) == PROG_WIN)
		    || (GetWindowKind(theWindow) == AD_WIN))
			return (True);
		else
			return (CloseMyWindow(theWindow));
	} else if (GetWindowKind(theWindow) == dialogKind) {
		DisposDialog_(GetDialogFromWindow(theWindow));
	} else if (IsPlugwindow(theWindow))
		return (PlugwindowClose(theWindow));
	return (True);
}

void ChangeTEFont(TEHandle teh, short font, short size);
#define aboutCreditsBtn 9
#define aboutWebBtn 6
/**********************************************************************
 * DoAboutUIUCmail - put up the about box
 **********************************************************************/
void DoAboutUIUCmail()
{
	Str63 versionString;
	Str255 partition, regInfo;
	MyWindowPtr dlogWin;
	DialogPtr dlog;
	extern ModalFilterUPP DlgFilterUPP;
	short item;
	short hideItem;
	OSType t;
	Handle h;
	Rect r, portR;
	ControlRef webButton;

	VersString(versionString);
	if (HaveOSX())
		//      Memory size is meaningless under OS X
		*partition = 0;
	else
		ComposeRString(partition, MEM_PARTITION,
			       CurrentSize() / (1 K),
			       EstimatePartitionSize(true) / (1 K));
	AboutRegisterString(regInfo);

	MyParamText(versionString, regInfo, "", "");
	SetDialogFont(SmallSysFontID());
	dlogWin = GetNewMyDialog(ABOUT_ALRT, nil, nil, InFront);
	ConfigFontSetup(dlogWin);
	SetDialogFont(0);
	if (!dlogWin)
		return;

	dlog = GetMyWindowDialogPtr(dlogWin);

	/*
	 * put up the dialog
	 */
	StartMovableModal(dlog);
	hideItem = CountDITL(dlog);
	for (item = aboutCreditsBtn + 2; item < hideItem; item++)
		HideDialogItem(dlog, item);
	HideDialogItem(dlog, aboutCreditsBtn + 1);

	// size to fit
	GetDialogItem(dlog, aboutCreditsBtn, &t, &h, &r);
	GetPortBounds(GetDialogPort(dlog), &portR);
	SizeWindow(GetDialogWindow(dlog), portR.right - portR.left,
		   r.bottom + (3 * INSET) / 2, False);

	// Get the ControlRef of the web button
	GetDialogItem(dlog, aboutWebBtn, &t, &h, &r);
	webButton = (ControlRef) h;

	ShowWindow(GetDialogWindow(dlog));
	do {
#if 0
		Boolean buttonSaysWeb = true;
		Boolean optionKeySaysWeb;

		optionKeySaysWeb = (optionKey & CurrentModifiers()) == 0;
		if (optionKeySaysWeb != buttonSaysWeb) {
			buttonSaysWeb = optionKeySaysWeb;
			SetControlTitle(webButton,
					buttonSaysWeb ? "\pWeb" :
					"\pFolder");
		}
#endif

		MovableModalDialog(dlog, DlgFilterUPP, &item);
		switch (item) {
		case aboutWebBtn:
			if ((optionKey & CurrentModifiers()) == 0 /* buttonSaysWeb */ ) {	// Open the web page and close the dialog
				OpenAdwareURL(GetNagState(), UPDATE_SITE,
					      actionSite, siteQuery, nil);
				item = 1;
			} else {	// Open the data folder and close the dialog
				FSSpec aSpec;
				aSpec.vRefNum = Root.vRef;
				aSpec.parID = Root.dirId;
				aSpec.name[0] = 0;
				(void) FinderOpen(&aSpec, False, False);
				item = 1;
			}
			break;
		case aboutCreditsBtn:	// Credits
			HideDialogItem(dlog, aboutCreditsBtn);	// hide this button to show next
			ShowDialogItem(dlog, aboutCreditsBtn + 1);
		case aboutCreditsBtn + 1:	// More Credits;
			ShowDialogItem(dlog, hideItem - 1);
			HideDialogItem(dlog, hideItem--);
			if (hideItem == aboutCreditsBtn + 2)
				HideDialogItem(dlog, aboutCreditsBtn + 1);
			break;

		default:
			break;

		}
	}
	while (item != 1);

	/*
	 * close
	 */
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
}

/**********************************************************************
 * NotImplemented - tell the user a feature is not implemented
 **********************************************************************/
void NotImplemented(void)
{
	DoAnAlert(Stop, "\pSorry, I'm (still) too stupid to do that.");
}



#ifdef TWO
/************************************************************************
 * DoCompWStationery - compose a message, using stationery
 ************************************************************************/
OSErr DoCompWStationery(FSSpecPtr spec)
{
	MyWindowPtr win;

	if (win = DoComposeNew(0)) {
		WindowPtr winWP = GetMyWindowWindowPtr(win);

		MessHandle messH =
		    (MessHandle) GetWindowPrivateData(winWP);
		ApplyStationery(win, spec, True, True);
		UpdateSum(messH, SumOf(messH)->offset,
			  SumOf(messH)->length);
		ShowMyWindow(winWP);
		return (0);
	}
	return (1);
}

/************************************************************************
 * ApplyDefaultStationery - apply custom stationery to a message
 ************************************************************************/
void ApplyDefaultStationery(MyWindowPtr win, Boolean bodyToo,
			    Boolean personality)
{
	FSSpec spec;
	Str31 name;

	//Stationery - no support for default stationery in Light       
	if (HasFeature(featureStationery))
		if (!FSMakeFSSpec
		    (StationVRef, StationDirId,
		     GetRString(name, STATIONERY), &spec))
			ApplyStationery(win, &spec, bodyToo, personality);
		else if (UseInlineSig)
			AddInlineSig(Win2MessH(win));
}

/************************************************************************
 * ApplyIndexStationery - apply custom stationery to a message
 ************************************************************************/
void ApplyIndexStationery(MyWindowPtr win, short which, Boolean bodyToo,
			  Boolean personality)
{
	FSSpec spec;
	Str31 name;
	OSErr err;

	//Stationery - no support for this in Light
	if (!HasFeature(featureStationery))
		return;

	MyGetItem(GetMHandle(REPLY_WITH_HIER_MENU), which, name);

	if (err = FSMakeFSSpec(StationVRef, StationDirId, name, &spec))
		FileSystemError(READ_STATION, name, err);
	else
		ApplyStationery(win, &spec, bodyToo, personality);
}

#endif

/************************************************************************
 * DoComposeNew - start a new outgoing message
 ************************************************************************/
MyWindowPtr DoComposeNew(TextAddrHandle toWhom)
{
	TOCHandle tocH;
	MSumType sum;
	MyWindowPtr newWin;
	Boolean oldReallyDirty;

#ifdef THREADING_ON
	// fix filter reply bug-- we should always be creating new mesgs in the real out toc
	if (!(tocH = GetRealOutTOC()))
		return (nil);
#else
	if (!(tocH = GetOutTOC()))
		return (nil);
#endif

	Zero(sum);

	sum.state = UNSENDABLE;
	sum.flags = DefaultOutFlags();
	sum.tableId = DEFAULT_TABLE;
	sum.origZone = ZoneSecs() / 60;
	sum.seconds = GMTDateTime();
	sum.persId = (*CurPers)->persId;
	sum.sigId = SigValidate(GetPrefLong(PREF_SIGFILE));

	oldReallyDirty = (*tocH)->reallyDirty;
	if (!SaveMessageSum(&sum, &tocH)) {
		WarnUser(MEM_ERR, MemError());
		return (nil);
	}
	(*tocH)->reallyDirty = oldReallyDirty;	// no need to get terribly excited about a new
	// message that doesn't even exist on disk yet

	newWin = OpenComp(tocH, (*tocH)->count - 1, nil, nil, False, True);
	if (newWin && toWhom) {
		MessHandle newMessH =
		    (MessHandle) GetMyWindowPrivateData(newWin);
		SetMessText(newMessH, TO_HEAD, LDRef(toWhom),
			    GetHandleSize(toWhom));
		UL(toWhom);
		NickExpandAndCacheHead(newMessH, TO_HEAD, false);
	}
	if (newWin) {
		newWin->isDirty = False;
		PeteResetCurStyle(newWin->pte);
		PeteCleanList(newWin->pte);
	}
	return (newWin);
}

/**********************************************************************
 * OpenAttachedApp - ask the user for confirmation, then unbundle and launch an application
 **********************************************************************/
OSErr OpenAttachedApp(FSSpecPtr spec)
{
	Str63 appName;
	OSType creator;
	short id;
	short result;
	OSErr err = noErr;
	OSType type;
	OSType **creatorList;

	if (ExtractCreatorFromBndl(spec, &creator))
		creator = '????';
	id = (TypeIsOnList(creator, SEA_LIST_TYPE)
	      || CreatorToName(creator,
			       appName)) ? ATTACH_APP2_ALRT :
	    ATTACH_APP_ALRT;
	MyParamText(appName, spec->name, "", "");
	result = ReallyDoAnAlert(id, Caution);
	if (result != kAlertStdAlertCancelButton) {
		type = FileTypeOf(spec);
		if (type == kFakeAppType)
			type = 'APPL';
		else {
			id = (type & 0xff) - '0';
			if (creatorList =
			    GetIndResource(EXECUTABLE_TYPE_LIST, 1))
				type = (*creatorList)[id];
			else
				type = 'APPL';
		}
		err = TweakFileType(spec, type, creator);
		if (!err && result == 1)
			err = OpenOtherDoc(spec, False, false, nil);
	} else
		err = userCancelled;
	return (err);
}

/**********************************************************************
 * CreatorToName - turn a creator into the name of its app
 **********************************************************************/
OSErr CreatorToName(OSType creator, PStr appName)
{
	short volIndex, vRef, dtRef;
	OSErr err;
	FSSpec appSpec;

	for (volIndex = 1; !(err = IndexVRef(volIndex, &vRef)); volIndex++) {
		if (!(err = DTRef(vRef, &dtRef))) {
			if (!
			    (err =
			     DTGetAppl(vRef, dtRef, creator, &appSpec))) {
				if (FileCreatorOf(&appSpec) == creator) {
					PCopy(appName, appSpec.name);
					return (noErr);
				}
			}
		}
	}
	if (!err)
		err = fnfErr;
	return (err);
}


/************************************************************************
 * DefaultOutFlags - get the default flags for an outgoing message
 ************************************************************************/
uLong DefaultOutFlags(void)
{
	uLong flags = 0;
	short aType = 0;

	flags |= FLAG_OUT;
	if (!PrefIsSet(PREF_NO_QP))
		flags |= FLAG_CAN_ENC;
	if (PrefIsSet(PREF_DOUBLE))
		aType = atmDouble;
	else if (PrefIsSet(PREF_SINGLE))
		aType = atmSingle;
	else if (PrefIsSet(PREF_UUENCODE))
		aType = atmUU;
	else if (PrefIsSet(PREF_BINHEX))
		aType = atmHQX;
	if (!aType)
		aType = atmDouble;
	SetAOptNumber(flags, aType - 1);
	if (PrefIsSet(PREF_WRAP_OUT))
		flags |= FLAG_WRAP_OUT;
	if (PrefIsSet(PREF_BX_TEXT))
		flags |= FLAG_BX_TEXT;
	if (PrefIsSet(PREF_KEEP_OUTGOING))
		flags |= FLAG_KEEP_COPY;
	if (PrefIsSet(PREF_TAB_IN_TEXT))
		flags |= FLAG_TABS;
	if (PrefIsSet(PREF_SEND_ENRICHED_NEW))
		flags |= FLAG_RICH;
	return (flags);
}

/**********************************************************************
 * RemSpoolFolder - remove a message's spool folder
 **********************************************************************/
OSErr RemSpoolFolder(long uidHash)
{
	OSErr err = noErr;
	Str255 scratch;
	FSSpec spec;

	NumToString(uidHash, scratch);
	if (noErr == (err = SubFolderSpec(SPOOL_FOLDER, &spec)))
		if (noErr ==
		    (err =
		     FSMakeFSSpec(spec.vRefNum, spec.parID, scratch,
				  &spec)))
			err = RemoveDir(&spec);
	return err;
}

/************************************************************************
 * CloseAllBoxes
 ************************************************************************/
Boolean CloseAllBoxes(void)
{
	TOCHandle thisTOC, nextTOC;

	for (thisTOC = TOCList; thisTOC; thisTOC = nextTOC) {
		nextTOC = (*thisTOC)->next;
		// don't close temp mailbox if thread is using it!!!
#ifdef THREADING_ON
		if (GetNumBackgroundThreads()
		    && ((thisTOC == GetTempOutTOC())
			|| (thisTOC == GetTempInTOC())))
			continue;
#endif
		if (!CloseMyWindow(GetMyWindowWindowPtr((*thisTOC)->win)))
			return (False);
	}
	return (True);
}

/************************************************************************
 *
 ************************************************************************/
Boolean CloseAllMessages(void)
{
	MessHandle thisMess, nextMess;

	for (thisMess = MessList; thisMess; thisMess = nextMess) {
		nextMess = (*thisMess)->next;
		// don't close message if thread is using it!!!
#ifdef THREADING_ON
		if (GetNumBackgroundThreads()
		    && (((*thisMess)->tocH == GetTempOutTOC())
			|| ((*thisMess)->tocH == GetTempInTOC())))
			continue;
#endif
		if (!CloseMyWindow(GetMyWindowWindowPtr((*thisMess)->win)))
			return (False);
	}
	return (True);
}

/************************************************************************
 * Type2Select - handle events for type-2-select
 ************************************************************************/
void Type2Select(EventRecord * event)
{
	static uLong delay;
	short key;

	/* 
	 * load up the delay
	 */
	if (!delay)
		delay = GetRLong(PARTIAL_TICKS);

	/*
	 * if it's been a long time, toss the string
	 */
	if (event->when - Type2SelTicks > delay)
		*Type2SelString = 0;

	/*
	 * now, the event
	 */
	switch (event->what) {
	case mouseDown:
	case app1Evt:
	case activateEvt:
		*Type2SelString = 0;
		break;

	case keyDown:
		if (event->modifiers & cmdKey)
			*Type2SelString = 0;
		else {
			key = event->message & charCodeMask;
			if (key == backSpace || key == tabChar
			    || key == returnChar || key == delChar
			    || key == escChar || !DirtyKey(key))
				*Type2SelString = 0;
			else if (*Type2SelString <
				 sizeof(Type2SelString) - 1) {
				Type2SelTicks = event->when;
				PCatC(Type2SelString, key);
			}
		}
		break;
	}
}

#pragma segment Balloon
void HelpRect(Rect * tipRect, Point tip, short method, short resSelect);
void HelpPict(Rect * tipRect, Point tip, short method, short resId);
void HelpRectString(Rect * tipRect, Point tip, short method, UPtr string);

/**********************************************************************
 * 
 **********************************************************************/
void MyBalloon(Rect * tipRect, short percentRight, short method,
	       short resSelect, short pict, PStr message)
{
	static Rect oldTipRect;
	static oldPercentRight;
	static oldResSelect;
	static oldPict;
	static oldMethod;
	static Str255 oldMessage;
	Rect r;
	Point tip;

	if (!HMIsBalloon() ||
	    !AboutSameRect(tipRect, &oldTipRect) ||
	    percentRight != oldPercentRight ||
	    resSelect != oldResSelect ||
	    pict != oldPict ||
	    method != oldMethod ||
	    (message ? !StringSame(message, oldMessage) : *oldMessage)) {
		oldTipRect = *tipRect;
		oldPercentRight = percentRight;
		oldResSelect = resSelect;
		oldPict = pict;
		oldMethod = method;
		if (message)
			PCopy(oldMessage, message);
		else
			*oldMessage = 0;

		r = *tipRect;
		LocalToGlobal((Point *) & r);
		LocalToGlobal((Point *) & r + 1);
		tip.h = r.left + (percentRight * RectWi(r)) / 100;
		tip.v = r.top + RectHi(r) / 2;

		if (resSelect)
			HelpRect(&r, tip, method, resSelect);
		else if (pict)
			HelpPict(&r, tip, method, pict);
		else if (message)
			HelpRectString(&r, tip, method, message);
	}
}

/************************************************************************
 * HelpRect - show a standard help balloon
 ************************************************************************/
void HelpRect(Rect * tipRect, Point tip, short method, short resSelect)
{
#if TARGET_RT_MAC_CFM
	HMMessageRecord hmmr;

	Zero(hmmr);
	hmmr.u.hmmStringRes.hmmResID = (resSelect / 100) * 100;
	hmmr.u.hmmStringRes.hmmIndex = resSelect % 100;
	hmmr.hmmHelpType = khmmStringRes;

	HMShowBalloon(&hmmr, tip, tipRect, nil, 0, 0, method);
#else
#warning "Need to use stuff in HIToolbox/MacHelp.h"
#endif
}

Boolean ManualTag;		// have we just put up a manual help tag?

/************************************************************************
 * MyHMHideTag - hide a tag, if it was put up by our routines
 ************************************************************************/
Boolean MyHMHideTag(void)
{
	if (ManualTag) {
		ManualTag = false;
		HMHideTag();
		return true;
	}
	return false;
}

/************************************************************************
 * MyHMDisplayTag - display a tag manually, and have it manually removed
 ************************************************************************/
OSErr MyHMDisplayTag(HMHelpContentPtr hmp)
{
	ManualTag = true;
	return HMDisplayTag(hmp);
}

/************************************************************************
 * HelpPict - show a help balloon with a pict in it
 ************************************************************************/
void HelpPict(Rect * tipRect, Point tip, short method, short resId)
{
#if TARGET_RT_MAC_CFM
	HMMessageRecord hmmr;

	Zero(hmmr);
	hmmr.u.hmmPict = resId;
	hmmr.hmmHelpType = khmmPict;
	HMShowBalloon(&hmmr, tip, tipRect, nil, 0, 0, method);
#else
#warning "Need to use stuff in HIToolbox/MacHelp.h"
#endif
}

/************************************************************************
 * HelpRectString - show a standard help balloon
 ************************************************************************/
void HelpRectString(Rect * tipRect, Point tip, short method, UPtr string)
{
#if TARGET_RT_MAC_CFM
	HMMessageRecord hmmr;

	Zero(hmmr);
	PCopy(hmmr.u.hmmString, string);
	hmmr.hmmHelpType = khmmString;
	HMShowBalloon(&hmmr, tip, tipRect, nil, 0, 0, method);
#else
#warning "Need to use stuff in HIToolbox/MacHelp.h"
#endif
}

/************************************************************************
 * DoHelp - do help for a window
 ************************************************************************/
void DoHelp(WindowPtr winWP)
{
	MyWindowPtr win;
	Point mouse;
	GrafPtr oldPort;
	Str255 sHelp, sTitle;
	Rect rPort;

	if (!winWP || !IsMyWindow(winWP))
		return;

	win = GetWindowMyWindowPtr(winWP);
	GetPort(&oldPort);
	SetPort_(GetWindowPort(winWP));
	GetMouse(&mouse);

	if (win->sponsorAdExists && PtInRect(mouse, &win->sponsorAdRect)) {
		//      do sponsor ad help
		GetSponsorAdTitle(sTitle);
		ComposeRString(sHelp, SPONSOR_AD_HELP, sTitle);
		MyBalloon(&win->sponsorAdRect, 100, 0, 0, 0, sHelp);
	} else
	    if (PtInRect
		(mouse, GetPortBounds(GetWindowPort(winWP), &rPort))
		&& !WazooHelp(win, mouse) && win->help)
		(*win->help) (win, mouse);

	SetPort_(oldPort);
}

#pragma segment Misc

/************************************************************************
 * HandleWindowChoice - handle the user choosing a window menu item
 ************************************************************************/
void HandleWindowChoice(short item)
{
	WindowPtr theWindow = FrontWindow_();

	switch (item) {
	case WIN_BEHIND_ITEM:
		SendBack(theWindow);
		break;

	case WIN_MINIMIZE_ITEM:
		CollapseWindow(theWindow, true);
		break;

	case WIN_BRINGALLTOFRONT_ITEM:
		{
			ProcessSerialNumber psn;

			if (GetCurrentProcess(&psn) == noErr)
				SetFrontProcess(&psn);
			break;
		}

	default:
		item -= WIN_BAR_ITEM;
		// we use a slightly different defn of FrontWindow here
		for (theWindow = FrontWindow(); theWindow;
		     theWindow = GetNextWindow(theWindow)) {
			if (WinWPWindowsMenuEligible(theWindow) && !--item) {
				ShowMyWindow(theWindow);
				UserSelectWindow(theWindow);
				break;
			}
		}
		break;
	}
}

/**********************************************************************
 * SendBack - send a window to the back
 **********************************************************************/
void SendBack(WindowPtr theWindow)
{
	WindowPtr behind;

	if (theWindow) {
		for (behind = theWindow; GetNextWindow(behind);
		     behind = GetNextWindow(behind))
			if (GetWindowKind(GetNextWindow(behind)) ==
			    TBAR_WIN)
				break;

		if (behind != theWindow)
			SendBehind(theWindow, behind);
	}
}

/**********************************************************************
 * IsSigWindow - is a window a signature window?
 **********************************************************************/
Boolean IsSigWindow(WindowPtr theWindow)
{
	FSSpec spec;
	FSSpec winSpec =
	    (*(TextDHandle) GetWindowPrivateData(theWindow))->spec;

	SigSpec(&spec, 0);
	if (SameSpec(&spec, &winSpec))
		return (True);
#ifdef TWO
	SigSpec(&spec, 1);
	if (SameSpec(&spec, &winSpec))
		return (True);
#endif
	return False;
}

/**********************************************************************
 * DoSFOpen - let the user choose a doc to open
 **********************************************************************/
void DoSFOpen(short modifiers)
{
	DoSFOpenNav(modifiers);
}

/**********************************************************************
 * InsertTextFile - insert a file into an open window
 **********************************************************************/
OSErr InsertTextFile(MyWindowPtr win, FSSpecPtr spec, Boolean rich,
		     Boolean delSP)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	OSErr err = noErr;
	Handle text;
	OSType type = FileTypeOf(spec);
	long pStart;

	if (type != 'TEXT' && type != 'ttro')
		return (WarnUser(TEXT_ONLY, 1));

	if (err = MemoryPreflight(FSpDFSize(spec)))
		return (err);

	// (jp) Be a little more paranoid of the window we're passed, just in case this is _ever_
	//                      called for windows other than composition windows
	if (IsMyWindow(winWP)) {

		if (!(err = Snarf(spec, &text, 0))) {
			if (win->pte) {
				CompReallyPreferBody(win);
				PetePrepareUndo(win->pte, peUndoPaste, -1,
						-1, &pStart, nil);
				//PeteCalcOn(win->pte);
				if (rich) {
					StackHandle stack;

					StackInit(sizeof(PartDesc),
						  &stack);
					err =
					    InsertRich(text, 0, -1, -1,
						       true, win->pte,
						       stack, delSP);
					(*PeteExtra(win->pte))->partStack =
					    stack;
				} else
					err =
					    PETEInsertText(PETE, win->pte,
							   -1, text, nil);

				if (!err) {
					text = nil;
					if (!rich)
						PeteSmallParas(win->pte);
					PeteCalcOn(win->pte);
					if (PrefIsSet(PREF_ZOOM_OPEN))
						ReZoomMyWindow(winWP);
					PeteFinishUndo(win->pte,
						       peUndoPaste, pStart,
						       -1);
				} else
					PeteKillUndo(win->pte);
			}
			ZapHandle(text);
		}
	}
	return (err);
}

/************************************************************************
 * ChangePassword - change the user's password
 ************************************************************************/
short ChangePassword(void)
{
	Str31 user, old, new, candidate;
	Str127 uName, hName, persName;
	short err;
	TransStream passStream = 0;

	if (Offline && GoOnline())
		return (OFFLINE);

	if ((err = NewTransStream(&passStream)) == noErr) {
		*(*CurPersSafe)->password = 0;	/* make the user reenter it */
		(*CurPersSafe)->popSecure = 0;
		GetPassStuff(persName, uName, hName);
		if (!
		    (err =
		     GetPassword(persName, uName, hName, old, sizeof(old),
				 ENTER))) {
			if (!
			    (err =
			     GetPassword(persName, uName, hName, candidate,
					 sizeof(candidate), NEW))
			    && !(err =
				 GetPassword(persName, uName, hName, new,
					     sizeof(new), VERIFY_NEW))) {
				if (!EqualString
				    (candidate, new, True, True)) {
					WarnUser(PW_MISMATCH, 0);
					err = 1;
				} else {
					GetRString(hName,
						   PW_CHANGE_SERVER);
					if (err =
					    GetPOPInfo(user,
						       *hName ? nil :
						       hName))
						return (err);
					PCopy((*CurPersSafe)->password,
					      old);
					err =
					    ReallyChangePassword
					    (passStream, user, hName, old,
					     new);
				}
			}
		}
		if (err)
			InvalidatePasswords(True, False, False);
		ZapTransStream(&passStream);
	}
	return (err);
}

/************************************************************************
 * ReallyChangePassword - change the user's password, really
 ************************************************************************/
short ReallyChangePassword(TransStream stream, UPtr user, UPtr host,
			   UPtr old, UPtr new)
{
	enum { PW_USER = 801, PW_PASS, PW_NEWPASS, PW_QUIT };
	short err = 1;
	long port = GetRLong(PW_PORT);
	Str255 buffer, junk;

	OpenProgress();
	ProgressMessageR(kpTitle, CHANGING_PW);
#ifdef CTB
	if (!UseCTB || !DialThePhone(stream))
#endif
		if (!ConnectTrans
		    (stream, host, port, False, GetRLong(OPEN_TIMEOUT))) {
			if (!CheckResult(stream, buffer)
			    && !SendPwCommand(stream, PW_USER, user)
			    && !CheckResult(stream, buffer)
			    && !SendPwCommand(stream, PW_PASS, old)
			    && !CheckResult(stream, buffer)
			    && !SendPwCommand(stream, PW_NEWPASS, new)
			    && !CheckResult(stream, buffer)) {
				err = 0;
				SilenceTrans(stream, True);
				if (!SendPwCommand(stream, PW_QUIT, ""))
					CheckResult(stream, junk);
				PCopy((*CurPersSafe)->password, new);
				(*CurPersSafe)->popSecure = True;
				if (PrefIsSet(PREF_SAVE_PASSWORD))
					PersSavePw(CurPersSafe);
			}
		}
#ifdef CTB
	if (UseCTB)
		HangUpThePhone();
#endif
	DestroyTrans(stream);
	CloseProgress();
	if (!err) {
		SetAlertBeep(False);
		AlertStr(OK_ALRT, Normal, buffer);
		SetAlertBeep(True);
	}
	return (err);
}

/************************************************************************
 * SendPwCommand - send a password command
 ************************************************************************/
short SendPwCommand(TransStream stream, short cmd, UPtr arg)
{
	Str255 buffer;
	ComposeRString(buffer, cmd, arg, NewLine);
	return (SendTrans(stream, buffer + 1, *buffer, nil));
}

/************************************************************************
 * CheckResult - did the command succeed?
 ************************************************************************/
short CheckResult(TransStream stream, UPtr buffer)
{
	short err;

	do {
		err = GetReply(stream, buffer + 1, 253, False, False);
		*buffer = strlen(buffer + 1);
		TransLitString(buffer);
		if (err >= 400 && err < 600) {
			Str255 pwErr;
			GetRString(pwErr, PW_ERROR);
			MyParamText(pwErr, buffer, "", "");
			ReallyDoAnAlert(OK_ALRT, Stop);
		} else
			ProgressMessage(kpMessage, buffer);
	}
	while (err < 200);
	return (err > 399);
}

/************************************************************************
 * FindPSNByCreator - find a process by creator code
 ************************************************************************/
OSErr FindPSNByCreator(ProcessSerialNumberPtr psnPtr, OSType creator)
{
	ProcessSerialNumber psn;
	ProcessInfoRec pi;
	short err;

	Zero(psn);

	for (err = GetNextProcess(&psn); !err; err = GetNextProcess(&psn)) {
		pi.processInfoLength = sizeof(pi);
		pi.processName = nil;
		pi.processAppSpec = nil;
		if (err = GetProcessInformation(&psn, &pi))
			return (err);
		if (pi.processSignature == creator) {
			if (psnPtr)
				*psnPtr = psn;
			return (noErr);
		}
	}
	return (procNotFound);
}

/************************************************************************
 * FindPSNByAlias - find a process by creator code
 ************************************************************************/
OSErr FindPSNByAlias(ProcessSerialNumberPtr psnPtr, AliasHandle alias)
{
	short err;
	FSSpec spec;
	Boolean wasChanged;

	if (err = ResolveAlias(nil, alias, &spec, &wasChanged))
		return (err);
	return (FindPSNBySpec(psnPtr, &spec));
}

/**********************************************************************
 * FindPSNBySpec - find a process by the file it belongs to
 **********************************************************************/
OSErr FindPSNBySpec(ProcessSerialNumberPtr psnPtr, FSSpecPtr spec)
{
	OSErr err;
	ProcessSerialNumber psn;
	FSRef appRef, processRef;
	ProcessInfoRec pi;
	FSSpec processSpec;

	FSpMakeFSRef(spec, &appRef);	//      Need an FSRef for comparing with process bundle

	Zero(psn);

	for (err = GetNextProcess(&psn); !err; err = GetNextProcess(&psn)) {
		Boolean found = false;

		//      Search first for application bundle             
		if (HaveOSX())	// Check for OS X, so we don't go boom!  SD 4/8/02
		{
			GetProcessBundleLocation(&psn, &processRef);
			found = !FSCompareFSRefs(&appRef, &processRef);
		}

		// Not OS X or Not Bundle
		if (!found) {
			//      Try reference to application itself
			pi.processInfoLength = sizeof(pi);
			pi.processName = nil;
			pi.processAppSpec = &processSpec;
			if (err = GetProcessInformation(&psn, &pi))
				return (err);
			found = SameSpec(pi.processAppSpec, spec);
		}
		if (found) {
			//      Found it
			if (psnPtr)
				*psnPtr = psn;
			return (noErr);
		}
	}
	return (procNotFound);
}

/**********************************************************************
 * LocateApp - locate an application alias
 **********************************************************************/
OSErr LocateApp(AliasHandle alias)
{
	FSSpec spec;
	OSErr err;
	Boolean wasChanged;

	if (!(err = ResolveAlias(nil, alias, &spec, &wasChanged)))
		if (wasChanged) {
			HNoPurge((Handle) alias);
			ChangedResource((Handle) alias);	/* fix up the alias */
		}
	return (err);
}

/************************************************************************
 * LaunchByAlias - launch an app from an alias
 ************************************************************************/
OSErr LaunchByAlias(AliasHandle alias)
{
	FSSpec spec;
	Boolean wasAlias;
	short err;

	err = ResolveAlias(nil, alias, &spec, &wasAlias);
	if (!err)
		err = LaunchAppWith(&spec, nil, false);
	if (err && err != userCanceledErr)
		return (WarnUser(COULDNT_LAUNCH, err));
	return err;
}

#pragma segment Balloon

void HelpTextWin(short itm)
{
	Str255 name, existName;
	MenuHandle hm;
	WindowPtr theWindow;
	Boolean same;

	if (!HMGetHelpMenuHandle(&hm)) {
		MyGetItem(hm, itm, name);

		/*
		 * is window already open?
		 */
		for (theWindow = FrontWindow_(); theWindow;
		     theWindow = GetNextWindow(theWindow)) {
			if (GetWindowKind(theWindow) == HELP_WIN) {
				same =
				    StringSame(name,
					       MyGetWTitle(theWindow,
							   existName));
				if (same) {
					UserSelectWindow(theWindow);
					if (ModalWindow)
						SelectWindow_(ModalWindow);
					return;
				}
			}
		}

		/*
		 * nope.  open it
		 */
		TextResWin(name);
	}
}

void TextResWin(PStr name)
{
	Handle picH = nil;
	StringHandle textH;
	short resID;
	ResType resType;
	PETEDocInitInfo pi;
	OSErr err;
	MyWindowPtr win;
	Boolean dispose = false;
	Str255 html;

	if (((textH =
	      (StringHandle) GetNamedResource('TEXT', name)) == nil)
	    && ((picH = GetNamedResource('PICT', name)) == nil))
		return;
	if (textH == nil) {
		dispose = true;
		GetResInfo(picH, &resID, &resType, name);
		ComposeRString(html, PICT_HELP_HTML, resID);
		if ((textH = (StringHandle) NuHTempBetter(html[0])) == nil) {
			WarnUser(MEM_ERR, MemError());
			return;
		}
		BMD(&html[1], *textH, html[0]);
	} else {
		ComposeRString(html, MIME_RICH_OFF, EnrichedStrn + enHTML);
		if (Munger(textH, 0, &html[1], html[0], nil, 0) < 0) {
			StringHandle support;

			dispose = true;
			DetachResource(textH);
			support =
			    (StringHandle) GetResource('TEXT',
						       IsFreeMode()? 3001 :
						       3000);
			HandAndHand(support, textH);
		}
	}

	win =
	    GetNewMyWindow(MESSAGE_WIND, nil, nil, nil, False, False,
			   HELP_WIN);
	if (win) {
		WindowPtr winWP = GetMyWindowWindowPtr(win);
		DefaultPII(win, True, peVScroll | peGrowBox, &pi);

		pi.inParaInfo.startMargin = 0;
		pi.inParaInfo.endMargin = WindowWi(winWP) + 1;

		if (!(err = PeteCreate(win, &win->pte, 0, &pi))) {
			long hOff = 0L, iOff = 0L;

			PeteCalcOff(win->pte);
			err =
			    InsertHTML(textH, &hOff, GetHandleSize(textH),
				       &iOff, win->pte, kDontEnsureCR);
			PeteCalcOn(win->pte);

			if (!err) {
				SetWTitle_(winWP, name);
				win->position = PositionPrefsTitle;
				win->didResize = TextDidResize;
				win->zoomSize = CompZoomSize;
				win->dontControl = True;
				win->find = TextFind;
				PETEMarkDocDirty(PETE, win->pte, False);
				win->isDirty = False;
				PeteSelect(win, win->pte, 0, 0);
				PeteLock(win->pte, 0, LONG_MAX, peModLock);
				PeteLock(win->pte, kPETECurrentStyle,
					 kPETECurrentStyle, peModLock);
				ShowMyWindow(winWP);
				UpdateMyWindow(winWP);
			}
		}
		if (err)
			CloseMyWindow(winWP);
	} else {
		err = MemError();
	}
	if (dispose)
		ZapHandle(textH);
	if (err)
		WarnUser(PETE_ERR, err);
}

/************************************************************************
 * HelpPict - display a help picture
 ************************************************************************/
void HelpPICTWin(short itm)
{
	Str255 name, existName;
	MenuHandle hm;
	WindowPtr theWindow;
	Boolean same;

	if (!HMGetHelpMenuHandle(&hm)) {
		MyGetItem(hm, itm, name);

		/*
		 * is window already open?
		 */
		for (theWindow = FrontWindow_(); theWindow;
		     theWindow = GetNextWindow(theWindow)) {
			if (GetWindowKind(theWindow) == PICT_WIN) {
				same =
				    StringSame(name,
					       MyGetWTitle(theWindow,
							   existName));
				if (same) {
					UserSelectWindow(theWindow);
					if (ModalWindow)
						SelectWindow_(ModalWindow);
					return;
				}
			}
		}

		/*
		 * nope.  open it
		 */
		PICTResWin(name);
	}
}

/************************************************************************
 * PICTResWindow - display a pict window from a named resource
 ************************************************************************/
void PICTResWin(PStr name)
{
	PicHandle resH;
	Rect r;
	short id;
	OSType type;
	MyWindowPtr win;
	short h, v;
	RGBColor color;
	HSVColor hsv;

	if (resH = (PicHandle) GetNamedResource('PICT', name)) {
		r = (*resH)->picFrame;
		win =
		    GetNewMyWindow(HELP_WIND, nil, nil, BehindModal, True,
				   True, PICT_WIN);
		if (win) {
			WindowPtr winWP = GetMyWindowWindowPtr(win);
			h = RoundDiv(r.right - r.left, win->hPitch);
			v = RoundDiv(r.bottom - r.top, win->vPitch);
			SetWTitle_(winWP, name);
#ifdef NEVER
			SizeWindow(winWP, h * win->hPitch + GROW_SIZE + 1,
				   v * win->vPitch + GROW_SIZE + 1, False);
#endif
			MyWindowMaxes(win, h, v);
			if (ThereIsColor) {
				GetBackColor(&color);
				RGB2HSV(&color, &hsv);
				hsv.value = MAX(40000, hsv.value);
				HSV2RGB(&hsv, &color);
				RGBBackColor(&color);
			}
			GetResInfo((Handle) resH, &id, &type, name);
			SetWindowPrivateData(winWP, id);
			win->position = PositionPrefsTitle;
			win->update = PICTUpdate;
			win->zoomSize = MaxSizeZoom;
			win->cursor = JustArrow;
			//win->dontControl = True;
			ShowMyWindow(winWP);
			if (ModalWindow)
				SelectWindow_(ModalWindow);
			return;
		}
	}
}

#pragma segment Help
/************************************************************************
 * JustArrow - make the cursor an arrow
 ************************************************************************/
void JustArrow(Point mouse)
{
#pragma unused(mouse)
	SetMyCursor(arrowCursor);
}

/************************************************************************
 * PICTUpdate - update a PICT window
 ************************************************************************/
void PICTUpdate(MyWindowPtr win)
{
	PicHandle pic = GetResource_('PICT', GetMyWindowPrivateData(win));
	Rect r;

	if (pic) {
		HNoPurge_(pic);
		r = (*pic)->picFrame;
		OffsetRect(&r,
			   -r.left -
			   GetControlValue(win->hBar) * win->hPitch,
			   -r.top -
			   GetControlValue(win->vBar) * win->vPitch);
		ClipRect(&win->contR);
		DrawPicture(pic, &r);
		InfiniteClip(GetMyWindowCGrafPtr(win));
		HPurge((Handle) pic);
	}
}

/************************************************************************
 * PICTRect - find a pict's rect
 ************************************************************************/
void PICTRect(short id, Rect * r)
{
	PicHandle pic = GetResource_('PICT', id);

	if (pic)
		*r = (*pic)->picFrame;
	else
		SetRect(r, 0, 0, 0, 0);
}

/**********************************************************************
 * PurchaseEudora - buy Eudora
 **********************************************************************/
void PurchaseEudora(void)
{
	Str255 url, s;
	ComposeRString(url, PURCH_URL, URLEscape(VersString(s)));
	ParseURL(url, s, nil, nil);
	if (EqualStrRes(s, ProtocolStrn + proMail))
		OpenLocalURL(url, nil);
	else
		OpenOtherURLPtr(s, url + 1, *url);
}

#ifdef TWO
#pragma segment Spell

/************************************************************************
 * FindSpeller - use the PPC browser to find a spell-checker
 ************************************************************************/
OSErr FindSpeller(void)
{
	SFTypeList types;
	FSSpec spec;
	Str255 prompt;
	OSErr theError;
	Boolean good;

	types[0] = 'APPL';
	types[1] = 'adrp';
	types[2] = 'appe';
	types[3] = 0;

	GetRString(prompt, SPELL_PROMPT);

	theError =
	    GetFileNav(types, CHOOSE_WORD_SERVICE_NAV_TITLE, prompt, 0,
		       false, &spec, &good, nil);
	if (!good)
		return (userCanceledErr);
	if (FileCreatorOf(&spec) == CREATOR) {
		WarnUser(NOT_SPELLER, 0);
		return (fnfErr);
	}

	UpdateAllWindows();

	InstallSpeller(&spec);
	return (noErr);
}

/**********************************************************************
 * InstallSpeller - Install a spelling app from an FSSpec
 **********************************************************************/
OSErr InstallSpeller(FSSpecPtr spec)
{
	Str255 menuItem;
	OSErr err;
	AEAddressDesc spellerAD;
	short item;
	ProcessSerialNumber psn;

	/*
	 * launch it
	 */
	LaunchAppWith(spec, nil, false);

	/*
	 * look for it
	 */
	if (!(err = FindPSNByCreator(&psn, FileCreatorOf(spec)))) {
		if (!
		    (err =
		     AECreateDesc(typeProcessSerialNumber, &psn,
				  sizeof(psn), &spellerAD))) {
			/*
			 * ask server for menu name
			 */
			if (!(err = GetWSMenuItem(&spellerAD, menuItem))) {
				if (item =
				    FindItemByName(GetMHandle(EDIT_MENU),
						   menuItem))
					DelWSService(item, false);
				AddWSService(spec, menuItem);
				QuitApp(&spellerAD);
			} else if (!CommandPeriod)
				WarnUser(NOT_SPELLER, err);
			AEDisposeDesc(&spellerAD);
		}
	}
	return (err);
}

/************************************************************************
 * DelWSService - get rid of a word service
 ************************************************************************/
void DelWSService(short item, Boolean toolbarToo)
{
	short n;
	Handle res;
	Str255 name;

	/*
	 * remove psn
	 */
	if (WordServices) {
		n = GetHandleSize_(WordServices) / sizeof(**WordServices);
		if (item < n - 1)
			BMD((*WordServices) + item,
			    (*WordServices) + item - 1,
			    (n - item - 1) * sizeof(**WordServices));
		SetHandleBig_(WordServices,
			      (n - 1) * sizeof(**WordServices));
	}

	/*
	 * remove alias from settings file
	 */
	MyGetItem(GetMHandle(EDIT_MENU), item, name);
	if (res = GetNamedResource(WS_ALIAS_TYPE, name)) {
		RemoveResource(res);
		ZapHandle(res);
	}

	/*
	 * and menu item
	 */
	DeleteMenuItem(GetMHandle(EDIT_MENU), item);

	/*
	 * and toolbar button, if any
	 */
	if (toolbarToo && ToolbarShowing())
		TBRemoveDefunctMenuButton(EDIT_MENU, item);
}

/************************************************************************
 * AddWSService - add a new service to the edit menu
 ************************************************************************/
OSErr AddWSService(FSSpecPtr spec, PStr menuItem)
{
	AliasHandle alias;
	Handle old;
	short err;
	OSType creator;
	ProcessSerialNumber psn;

	/*
	 * ask the server for its location
	 */
	if (err = NewAlias(nil, spec, &alias))
		return (err);
	(*alias)->userType = creator = FileCreatorOf(spec);

	/*
	 * do we have one already?
	 */
	if (old = GetNamedResource(WS_ALIAS_TYPE, menuItem)) {
		RemoveResource(old);
		ZapHandle(old);
	}

	/*
	 * add the new one
	 */
	AddMyResource_(alias, WS_ALIAS_TYPE, MyUniqueID(WS_ALIAS_TYPE),
		       menuItem);
	err = ResError();
	if (err)
		ZapHandle(alias);
	else {
		FindPSNByCreator(&psn, creator);
		if (!WordServices)
			WordServices = NuHandle(0);
		err =
		    PtrPlusHand_(&psn, WordServices,
				 sizeof(**WordServices));
		if (!err)
			MyAppendMenu(GetMHandle(EDIT_MENU), menuItem);
	}

	/*
	 * frontify ourselves
	 */
	SetFrontProcess(CurrentPSN(&psn));

	return (err);
}

/************************************************************************
 * GetWSMenuItem - get the menu item from a word services app
 ************************************************************************/
OSErr GetWSMenuItem(AEAddressDescPtr aead, PStr itemText)
{
	short err;
	AEDesc ad;

//      if (err = GetAppProperty(aead,pBatchMenuString,typeChar,&ad)) return(err); CK

	*itemText = MIN(61, AEGetDescDataSize(&ad));
	AEGetDescData(&ad, itemText + 1, *itemText);
	itemText[*itemText + 1] = 0;

	AEDisposeDesc(&ad);
	return (noErr);
}

/************************************************************************
 * ServeThemWords - 
 ************************************************************************/
OSErr ServeThemWords(short item, AEDescPtr objAD)
{
	OSErr err;

	err = VerifySpeller(item);
	if (err == 2)
		item = CountMenuItems(GetMHandle(EDIT_MENU));
	if (!err)
		err = Spell(item, objAD);

	return (err);
}

/************************************************************************
 * VerifySpeller - is the speller running?
 ************************************************************************/
OSErr VerifySpeller(short item)
{
	return (StartSpeller(item, False));
}

/************************************************************************
 * StartSpeller - start the speller running
 ************************************************************************/
OSErr StartSpeller(short item, Boolean runningOnly)
{
	Str255 name;
	AliasHandle alias;
	OSErr err;
	ProcessSerialNumber psn;

	MyGetItem(GetMHandle(EDIT_MENU), item, name);

	if (!(alias = (AliasHandle) GetNamedResource(WS_ALIAS_TYPE, name))) {
		WarnUser(SPELL_ALIAS_GONE, 0);
		DelWSService(item, false);
		return (1);
	}

	if (err = FindPSNByCreator(&psn, (*alias)->userType)) {
		if (!runningOnly && !(err = LaunchByAlias(alias))) {
			MiniEvents();
			return (StartSpeller(item, True));
		}
	} else
		(*WordServices)[item - EDIT_SPELL_ITEM - 1] = psn;

	ReleaseResource_(alias);

	if ((err == fnfErr || err == nsvErr || err == dirNFErr
	     || err == userCanceledErr))
		switch (AlertStr(REMOVE_SPELL_ALRT, Note, name)) {
		case kAlertStdAlertOKButton:	// locate application
			FindSpeller();
			TBarHasChanged = true;
			err = 2;	// signal caller to retry
			break;

		case kAlertStdAlertCancelButton:	// cancel
			break;

		case kAlertStdAlertOtherButton:	// remove application
			DelWSService(item, true);
			break;
		}

	return (err);
}

/************************************************************************
 * Spell - tell the speller to go for it
 ************************************************************************/
OSErr Spell(short item, AEDescPtr objAD)
{
	ProcessSerialNumber psn =
	    (*WordServices)[item - EDIT_SPELL_ITEM - 1];
	AppleEvent ae;
	AEDesc spellerAD;
	OSErr err;

	NullADList(&ae, &spellerAD, nil);

	if (err =
	    AECreateDesc(typeProcessSerialNumber, &psn, sizeof(psn),
			 &spellerAD))
		goto done;

//      if (err = AECreateAppleEvent(kWordServicesClass,kWSBatchCheckMe,&spellerAD,kAutoGenerateReturnID,kAnyTransactionID,&ae)) CK
//              goto done; CK

#ifdef DEBUG
	if (RunType == Debugging) {
		AEGetDescDataSize(objAD);
		if (MemError())
			Debugger();
	}
#endif
	if (err = AEPutParamDesc(&ae, keyDirectObject, objAD))
		goto done;

	err =
	    MyAESend(&ae, nil, kAEQueueReply | kAECanInteract,
		     kAENormalPriority, 0);

      done:
	if (err)
		WarnUser(AE_TROUBLE, err);
	DisposeADList(&ae, &spellerAD, nil);
	return (noErr);
}

/************************************************************************
 * GetProperty - get a property
 ************************************************************************/
OSErr GetProperty(AEAddressDescPtr aead, AEDescPtr objAD,
		  DescType propType, DescType wantType, AEDescPtr ad)
{
	AppleEvent ae, reply;
	AEDesc propAD, propObj;
	short err;

	NullADList(&ae, &reply, &propObj, &propAD, nil);

	/*
	 * create the event
	 */
	if (err =
	    AECreateAppleEvent(kAECoreSuite, kAEGetData, aead,
			       kAutoGenerateReturnID, kAnyTransactionID,
			       &ae))
		goto done;

	/*
	 * create the property descriptor
	 */
	if (err =
	    AECreateDesc(typeType, &propType, sizeof(propType), &propAD))
		goto done;

	/*
	 * create the property specifier
	 * Boy, this all sure is fun.
	 */
	if (err =
	    CreateObjSpecifier(typeProperty, objAD, formPropertyID,
			       &propAD, False, &propObj))
		goto done;

	/*
	 * add object to param list
	 */
	if (err = AEPutParamDesc(&ae, keyDirectObject, &propObj))
		goto done;

	/*
	 * now, send the event
	 */
	if (err =
	    MyAESend(&ae, &reply, kAEWaitReply | kAECanInteract,
		     kAENormalPriority, GetRLong(AE_SHORT_TIMEOUT_TICKS)))
		goto done;

	/*
	 * was there an error?
	 */
	if (err = GetAEError(&reply))
		goto done;

	/*
	 * finally; get the value
	 */
	err = AEGetParamDesc(&reply, keyDirectObject, wantType, ad);

      done:
	DisposeADList(&ae, &reply, &propObj, &propAD, nil);
	return (err);
}

/************************************************************************
 * GetAppProperty - get a property from an app
 ************************************************************************/
OSErr GetAppProperty(AEAddressDescPtr aead, DescType propType,
		     DescType wantType, AEDescPtr ad)
{
	AEDesc appAD;
	short err;

	NullADList(&appAD, nil);
	err = GetProperty(aead, &appAD, propType, wantType, ad);

	return (err);
}
#endif

/************************************************************************
 * EmptyTrash - empty the trash mailbox
 ************************************************************************/
void EmptyTrash(Boolean localOnly, Boolean currentOnly, Boolean all)
{
	Str15 suffix;
	TOCHandle tocH;
	short refN;
	FSSpec spec;
	short sum;
	short imapTrash =
	    IMAPCountTrashMessages(localOnly, currentOnly, all);

	spec.vRefNum = MailRoot.vRef;
	spec.parID = MailRoot.dirId;
	GetRString(spec.name, TRASH);

	/*
	 * check with Mom first
	 */
	if (!PrefIsSet(PREF_NO_TRASH_WARN)) {
		if ((tocH = GetTrashTOC())
		    && ((*tocH)->count || imapTrash)) {
			short totalMessages = 0;

			if (localOnly)
				totalMessages = (*tocH)->count;
			else if (currentOnly)
				totalMessages =
				    imapTrash ? imapTrash : (*tocH)->count;
			else
				totalMessages = (*tocH)->count + imapTrash;

			if (!Mom
			    (EMPTY_BUTTON, 0, PREF_NO_TRASH_WARN,
			     EMPTY_TRASH_FMT, totalMessages,
			     totalMessages))
				return;
		} else
			return;
	}

#ifdef TWO
	if (tocH = GetTrashTOC()) {
		if (PrefIsSet(PREF_SERVER_DEL)) {
			for (sum = 0; sum < (*tocH)->count; sum++) {
				CycleBalls();
				if (IdIsOnPOPD
				    (PERS_POPD_TYPE
				     (TS_TO_PPERS(tocH, sum)), POPD_ID,
				     (*tocH)->sums[sum].uidHash))
					AddIdToPOPD(PERS_POPD_TYPE
						    (TS_TO_PPERS
						     (tocH, sum)),
						    DELETE_ID,
						    (*tocH)->sums[sum].
						    uidHash, False);
			}
		}
		for (sum = 0; sum < (*tocH)->count; sum++) {
			CycleBalls();
			if ((*tocH)->sums[sum].opts & OPT_HAS_SPOOL)
				RemSpoolFolder((*tocH)->sums[sum].uidHash);
		}
	}
#endif

	// empty all the IMAP trash mailboxes, return if there's nothing left to do
	if (!IMAPEmptyTrash(localOnly, currentOnly, all))
		return;

	NukeXfUndo();

	/*
	 * close messages
	 */
//      tocH = FindTOC(&spec);
//      if (tocH)
	if (tocH = GetTrashTOC())
		for (sum = 0; sum < (*tocH)->count; sum++) {
			if ((*tocH)->sums[sum].messH)
				CloseMyWindow(GetMyWindowWindowPtr
					      ((*(*tocH)->sums[sum].
						messH)->win));
			DeleteMesgError(tocH, sum);
		}

	/*
	 * open and truncate the mailbox
	 */
	if (!AFSpOpenDF(&spec, &spec, fsRdWrPerm, &refN)) {
		SetEOF(refN, 0);
		MyFSClose(refN);
		FixSpecUnread(&spec, False);
		FixMenuUnread(GetMHandle(MAILBOX_MENU), MAILBOX_TRASH_ITEM,
			      False);
	}

	/* 
	 * fix visible window
	 */
	if (tocH && (*tocH)->win) {
		(*tocH)->count = 0;
		TOCSetDirty(tocH, true);
		SetHandleSize((Handle) tocH, sizeof(TOCType));
		MyWindowMaxes((*tocH)->win, 0, 0);
		MyWindowDidResize((*tocH)->win, nil);
		InvalContent((*tocH)->win);
	} else {
		/*
		 * delete the toc
		 */
		FSpKillRFork(&spec);
		PCat(spec.name, GetRString(suffix, TOC_SUFFIX));
		FSpDelete(&spec);
	}

	//      Notify any search windows that we nuked the trash
	SearchUpdateSum(nil, 0, nil, 0, false, true);
}

PStr AboutRegisterString(PStr s)
{
	Str255 regCode, regName;
	Str63 oldRegCode;
	UserStateType state;
	int policyCode, regMonth;
	short id;

	state = GetNagState();
	GetRegFirst(state, regName);
	if (regName[0])
		PCatC(regName, ' ');
	PCat(regName, GetRegLast(state, s));
	GetRegCode(state, regCode);
	*regCode = MIN(*regCode, 254);
	*regName = MIN(*regName, 254);

	if (!regName[0] || !regCode[0]
	    || (regName[++regName[0]] = 0, regCode[++regCode[0]] =
		0, !RegCodeVerifies(&regCode[1], &regName[1], nil, nil))) {
		regCode[0] = regName[0] = 0;
	}
	if (!regName[0])
		GetRString(regName, NO_REG_NAME_TEXT);
	if (!regCode[0])
		GetRString(regCode, NO_REG_CODE_TEXT);

	if (PayMode(state))
		id = paidModeName;
	else if (AdwareMode(state))
		id = adModeName;
	else if (FreeMode(state))
		id = freeModeName;
	else
		id = unknownModeName;
	// Look for a Paid reg code and append the regMonth to the about string
	if (ValidRegCode(paidUser, &policyCode, &regMonth))
		ComposeRString(s, ABOUT_REG_WITH_MONTH, regName, regCode,
			       ModeNamesStrn + id, regMonth);
	else
		ComposeRString(s,
			       *GetPref(oldRegCode,
					PREF_SITE) ? ABOUT_REG_W_OLD :
			       ABOUT_REG, regName, regCode,
			       ModeNamesStrn + id, oldRegCode);
	return s;
}

/************************************************************************
 * ReportABug - help the user submit a bug report
 ************************************************************************/
void ReportABug(void)
{
	MyWindowPtr win;
	Handle textH = GetResource('TEXT', BUG_TEXT);

	if (textH) {
		HNoPurge(textH);
		if (win = DoComposeNew(0)) {
			WindowPtr winWP = GetMyWindowWindowPtr(win);
			MessHandle messH =
			    (MessHandle) GetWindowPrivateData(winWP);

			ApplyStationeryHandle(win, textH, true, false,
					      false);
			InsertSystemConfig((*messH)->bodyPTE);
			PersonalizeSubject(messH);
			SerializeSubject(messH);
			CompSelectSecondUnquoted(messH);
			UpdateSum(messH, SumOf(messH)->offset,
				  SumOf(messH)->length);
			ShowMyWindow(winWP);
		}
		HPurge(textH);
	}
}

/************************************************************************
 * MakeASuggestion - help the user submit a suggestion
 ************************************************************************/
void MakeASuggestion(void)
{
	MyWindowPtr win;
	Handle textH = GetResource('TEXT', SUGGEST_TEXT);

	if (textH) {
		HNoPurge(textH);
		if (win = DoComposeNew(0)) {
			WindowPtr winWP = GetMyWindowWindowPtr(win);

			MessHandle messH =
			    (MessHandle) GetWindowPrivateData(winWP);
			ApplyStationeryHandle(win, textH, true, false,
					      false);
			PersonalizeSubject(messH);
			SerializeSubject(messH);
			UpdateSum(messH, SumOf(messH)->offset,
				  SumOf(messH)->length);
			ShowMyWindow(winWP);
		}
		HPurge(textH);
	}
}
