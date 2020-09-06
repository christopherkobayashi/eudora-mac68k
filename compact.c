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

#include "compact.h"
#define FILE_NUM 8
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment CompMng
/************************************************************************
 * private functions
 ************************************************************************/
	void CompSwitchFields(MessHandle messH, Boolean forward);
	void ScrollWinRect(MyWindowPtr win, Rect *scroll, Rect *good, short offset);
	void DrawCompWindowBars(MyWindowPtr win); /* MJN *//* new routine */
	uLong QueueDate2Secs(UPtr timeStr,UPtr dateStr);
	Rect *CompIconRect(Rect *r,MessHandle messH,short index);

void CompUpdateScore(MyWindowPtr win);
void CopyExtraSigs(MenuHandle onto);
OSErr CleanseStationery(MyWindowPtr win);
short SigIconId(MessHandle messH);
short SigMenuItem(MessHandle messH);
Boolean GetSigRect(MyWindowPtr win,Rect *pr);
Boolean GetAttachRect(MyWindowPtr win,Rect *pr);
#ifdef VCARD
Boolean GetVCardRect (MyWindowPtr win, Rect *pr);
#endif
Boolean GetAnalRect(MyWindowPtr win,Rect *pr);
void AttachMenu(MyWindowPtr win);
void SigMenu(MyWindowPtr win);
void SigMenuHelp(MyWindowPtr win, Rect *r);
void AttMenuHelp(MyWindowPtr win, Rect *r);
void RefreshSigButton(MessHandle messH);
#ifdef VCARD
void VCardBtnHelp(MyWindowPtr win,Rect *r);
#endif
OSErr QueueSizeWarning(MessHandle messH);
uLong SigFIDByName(PStr string);
OSErr SigSpecByName(PStr string,FSSpecPtr spec);
OSErr CompDrop(MyWindowPtr win,DragReference drag);
void AddrSelect(MessHandle messH);
void CompTranslatorHelp(Rect *r,short i);
void CompActivate(MyWindowPtr win);
OSErr AddSigPopup(MessHandle messH);
OSErr AddAttachPopup(MessHandle messH);
#ifdef VCARD
OSErr AddVCardButton (MessHandle messH);
#endif
OSErr AddAnalIcon(MessHandle messH);
void RefreshSigPopup(MyWindowPtr win);
void PersPopup(MyWindowPtr win,Point where);
OSErr CompAddPersPopup(MyWindowPtr win);
pascal Boolean SendStyleWarningFilter(DialogPtr dgPtr,EventRecord *event,short *item);
void CompSendBtnUpdate(MyWindowPtr win);
void RedrawPersPopup(MessHandle messH);
OSErr InsertGraphicFile(MyWindowPtr win,FSSpecPtr spec);
OSErr SendStyleWarning(MessHandle messH);
#ifdef USECMM_BAD
OSErr	CompBuildCMenu( MyWindowPtr win, EventRecord* contextEvent, CMenuInfoHndl* specCMenuInfo );
#endif

/* NOTE: the number of elements in the arrays fBits[] and icons[] must be the same
					as the value of the define ICON_BAR_NUM (defined in compact.h) */
#ifdef TWO
	static long fBits[]={-OPT_COMP_TOOLBAR_VISIBLE,FLAG_CAN_ENC,FLAG_BX_TEXT,FLAG_WRAP_OUT,FLAG_KEEP_COPY,FLAG_RR/*,FLAG_SIGN,FLAG_ENCRYPT*/};
	static short icons[]={FORMATBAR_ICON,QP_SICN,BX_TEXT_SICN,WRAP_SICN,KEEPCOPY_SICN,RR_SICN/*,SIGN_SICN,ENCRYPT_SICN*/};
#else
	static long fBits[]={FLAG_CAN_ENC,FLAG_BX_TEXT,FLAG_WRAP_OUT,FLAG_KEEP_COPY,FLAG_SIG};
	static short icons[]={QP_SICN,BX_TEXT_SICN,WRAP_SICN,KEEPCOPY_SICN,SIGNATURE_SICN};
#endif


/* #define ICON_BAR_NUM (sizeof(fBits)/sizeof(long)) */
/* MJN *//* moved define for ICON_BAR_NUM to compact.h */
#define I_WIDTH 24
#define I_HEIGHT 24 /* MJN *//* text formatting toolbar */

/**********************************************************************
 * CompClose - close a composition window.	save the contents, iff
 * they're dirty.
 **********************************************************************/
Boolean CompClose(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	MessType **messH = (MessType **)GetWindowPrivateData(winWP);
	int which;
	Boolean queued = IsQueued((*messH)->tocH,(*messH)->sumNum);
	Boolean nbk = SumOf(messH)->length==0 && !queued;

	if (!GrowZoning) win->isDirty = win->isDirty || PeteIsDirtyList(win->pte);

	if (win->isDirty && !NoSaves && !GrowZoning)
	{
		which = IsWindowVisible (winWP) ? WannaSave(win) : WANNA_SAVE_SAVE;
		if (which==CANCEL_ITEM) return(False);
		else if (which == WANNA_SAVE_SAVE)
		{
			if (!SaveComp(win)) return(False);
			nbk = False;
		}
	}
	
	(*messH)->persGraphic = nil;
	
	if (!GrowZoning && nbk)
		FixSourceStatus((*messH)->tocH,(*messH)->sumNum);	/* in case we're deleting a reply, set orig state back */
	
	LL_Remove(MessList,messH,(MessType **));
	(*(*messH)->tocH)->sums[(*messH)->sumNum].messH = nil;
	
	if (!GrowZoning)
	{
		if (nbk)
		{
			if (MessOptIsSet(messH,OPT_HAS_SPOOL) && !(*messH)->hStationerySpec) RemSpoolFolder(SumOf(messH)->uidHash);
			DeleteSum((*messH)->tocH,(*messH)->sumNum);
		}
	}
	AccuZap((*messH)->extras);
	ZapHandle(messH);
	if (queued) SetSendQueue();
	return(True);
}

/************************************************************************
 * QueueMessage - queue up a message
 ************************************************************************/
OSErr QueueMessage(TOCHandle tocH,short sumNum,SendTypeEnum st,long secs, Boolean noSpell, Boolean analDelay)
{
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	OSErr err = userCancelled;
	short state = secs ? TIMED : QUEUED;
	Boolean stripWithPrejuidice;
	long oldFlags, oldOpts;
	short	historyAB;
	
	// (jp) 7/9/98  Let's cache recently used addresses and save 'em to disk
	if (HasFeature (featureNicknameWatching) && messH && !PrefIsSet (PREF_NICK_CACHE)) {
		CompGatherRecipientAddresses (messH, true);
		historyAB = FindAddressBookType (historyAddressBook);
		if (historyAB >= 0)
			SaveIndNickFile (historyAB, true);
	}
	
	if (messH && (*messH)->hStationerySpec)
	{
		if (SaveComp((*messH)->win)) err = noErr;
		return err;
	}

	if (!secs) secs = GMTDateTime();
	
	if (st!=kEuSendNever && messH)
	{
		// first, figure out where we stand wrt rich text
		oldFlags = SumOf(messH)->flags;
		oldOpts = SumOf(messH)->opts;
		
		SetMessRich(messH);
		ClearMessOpt(messH,OPT_BLOAT);
		ClearMessOpt(messH,OPT_STRIP);
		stripWithPrejuidice = MessOptIsSet(messH,OPT_JUST_EXCERPT);
		if (!stripWithPrejuidice)
		{
			switch (GetPrefLong(PREF_SEND_STYLE))
			{
				case ssBloat: SetMessOpt(messH,OPT_BLOAT); break;
				case ssStrip:	SetMessOpt(messH,OPT_STRIP); break;
			}
		
			// now ask the user if need be
			if (MessIsRich(messH) && PrefIsSet(PREF_WARN_RICH) && !MessOptIsSet(messH,OPT_BULK))
				switch (SendStyleWarning(messH)-1)
				{
					case ssLimit: return(userCanceledErr); break;
					case ssBloat: SetMessOpt(messH,OPT_BLOAT); ClearMessOpt(messH,OPT_STRIP); break;
					case ssStrip: ClearMessOpt(messH,OPT_BLOAT); SetMessOpt(messH,OPT_STRIP); break;
					default: ClearMessOpt(messH,OPT_BLOAT); ClearMessOpt(messH,OPT_STRIP); break;
				}
		}
		
		// if flags changed, re-save
		if (oldFlags != SumOf(messH)->flags || oldOpts != SumOf(messH)->opts)
			(*messH)->win->isDirty = true;
	}
	
	if (st==kEuSendNever)
	{
		if (IsQueued(tocH,sumNum))
			SetState(tocH,sumNum,SENDABLE);
		err = noErr;
	}
	else if (!messH ||
					 (SumOf(messH)->length!=0 && !(*messH)->win->isDirty)
					 || SaveComp((*messH)->win))
	{
		if ((*tocH)->sums[sumNum].state == UNSENDABLE)
		{
			WarnUser(CANT_QUEUE,0);
			err = CANT_QUEUE;
		}
		else
		{
			if (PrefIsSet(PREF_SUBJECT_WARNING) &&
							!*(*tocH)->sums[sumNum].subj &&
							(!Mom(QUEUE_BTN,0,PREF_SUBJECT_WARNING,R_FMT,SUBJECT_WARNING)))
				err = CANT_QUEUE;
			else if (messH && AnalWarning(messH))
				err = CANT_QUEUE;
			else if (messH && QueueSizeWarning(messH))
				err = CANT_QUEUE;
#ifdef WINTERTREE
			else if (HasFeature (featureSpellChecking) && !noSpell && messH && !MessOptIsSet(messH,OPT_BULK) && SpellOptOnQueue && SpellAnyWrongHuh(TheBody) && ComposeStdAlert(Caution,SPELL_WARNING)!=1)
				err = CANT_QUEUE;
#endif
			else if (messH && (*messH)->hTranslators && (EMSR_NOW!=ETLCanTransOut(messH) || 
				ETLQueueMessage(messH) || ((*messH)->win->isDirty && !SaveComp((*messH)->win))))
				err = CANT_QUEUE;
			else if (!messH || CloseMyWindow(GetMyWindowWindowPtr((*messH)->win)))
			{
				// Moodwatch queue delay
				if (analDelay && AnalDelayOutgoing() && state!=TIMED && (st==kEuSendNow || st==kEuSendNext))
				{
					AnalBox(tocH,sumNum,sumNum);
					if ((*tocH)->sums[sumNum].score > GetRLong(ANAL_DELAY_LEVEL))
					{
						st = kEuSendLater;
						state = TIMED;
						secs += 60*GetRLong(ANAL_DELAY_MINUTES);
					}
				}
				SetState(tocH,sumNum,state);
				TimeStamp(tocH,sumNum,secs,ZoneSecs());
				err = noErr;
				if (st==kEuSendNow)
				{
					err = XferMail(False,True,False,False,True,0);
				}
			}
		}
	}
	SetSendQueue();
	return(err);
}

/************************************************************************
 * SendStyleWarning - ask the user what to do about styles
 ************************************************************************/
OSErr SendStyleWarning(MessHandle messH)
{
	Str255 messTitle;
	MyWindowPtr	dlogWin;
	DialogPtr dlog;
	short item;
	DECLARE_UPP(SendStyleWarningFilter,ModalFilter);

	INIT_UPP(SendStyleWarningFilter,ModalFilter);
	MyGetWTitle(GetMyWindowWindowPtr((*messH)->win),messTitle);
	MyParamText(messTitle,nil,nil,nil);

	dlogWin = GetNewMyDialog(SEND_STYLE_ALRT,nil,nil,InFront);
	if (!dlogWin)
	{
		WarnUser(MEM_ERR,ResError());
		return(ssLimit+1);
	}
	
	dlog = GetMyWindowDialogPtr (dlogWin);
	
	SetPort_(GetWindowPort(GetDialogWindow(dlog)));
	
	// Set the default button
	item = GetPrefLong(PREF_SEND_STYLE)+1;
	SetDialogDefaultItem(dlog,item);
			
	/*
	 * put up the alert
	 */
	AutoSizeDialog(dlog);
	StartMovableModal(dlog);
	ShowWindow(GetDialogWindow(dlog));
	SetMyCursor(arrowCursor);
	
	MovableModalDialog(dlog,SendStyleWarningFilterUPP,&item);

	//done
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
	
	return(item);
}

/************************************************************************
 * SendStyleWarningFilter - filter for normal dialogs
 ************************************************************************/
pascal Boolean SendStyleWarningFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	Boolean result;
	
	if (event->what==keyDown || event->what==autoKey) SpecialKeys(event);
	if (event->what==keyDown || event->what==autoKey)
	{
		short key = event->message & charCodeMask;
		if (!(event->modifiers&cmdKey))
			switch (key)
			{
				case enterChar:
				case returnChar:
					*item = GetPrefLong(PREF_SEND_STYLE)+1;
					return(True);
					break;
			}
	}
	else if (event->what==activateEvt || event->what==updateEvt)
		return(false);	// avoid hilitebuttonone
	
	result = DlgFilter(dgPtr,event,item);
	if (*item==CANCEL_ITEM) *item = ssLimit+1;
	return(result);
}
	

/**********************************************************************
 * QueueSizeWarning - check the size of a message, and warn the user if excessive
 **********************************************************************/
OSErr QueueSizeWarning(MessHandle messH)
{
	uLong max = GetRLong(MAX_MESSAGE_SIZE);
	uLong size;
	
	
	if (max && max<(size=ApproxMessageSize(messH)))
	{
		Boolean notOK;
		notOK = (Aprintf(YES_CANCEL_ALRT,Caution,SIZE_WARNING,size)!=1);
		if (notOK) return(CANT_QUEUE);
	}
	return(noErr);
}

/**********************************************************************
 * ApproxMessageSize - return approximate size of message, in K
 **********************************************************************/
uLong ApproxMessageSize(MessHandle messH)
{
	long size = (SumOf(messH)->length*41)/40;
	FSSpec spec;
	short index;
	long oneSize;
	
	for (index=1;;index++)
	{
		if (GetIndAttachment(messH,index,&spec,nil)) break;
		if (FSpIsItAFolder(&spec))
			FolderSizeHi(spec.vRefNum,SpecDirId(&spec),&oneSize);
		else
			oneSize = FSpFileSize(&spec);
		size += (4*oneSize)/3;
	}
	
	return(RoundDiv(size,1024));
}


/**********************************************************************
 * CountAttachments - count the attachments in a message
 **********************************************************************/
short CountAttachments(MessHandle messH)
{
	short nAttach;
	FSSpec spec;
	
	for (nAttach=0;1!=GetIndAttachment(messH,nAttach+1,&spec,nil);nAttach++);
	return(nAttach);
}

/************************************************************************
 * CompAttach - attach a document to a message
 ************************************************************************/
void CompAttach(MyWindowPtr win, Boolean insertDefault)
{
	CompAttachNav (win, insertDefault);
}


/************************************************************************
 * InsertGraphicFile - insert a graphic into the current window
 ************************************************************************/
OSErr InsertGraphicFile(MyWindowPtr win,FSSpecPtr spec)
{
	long spot;
	OSErr err = noErr;
	
	// (jp) Be a little more paranoid of the window we're passed, just in case this is _ever_
	//			called for windows other than composition windows
	if (IsMyWindow (GetMyWindowWindowPtr(win)) && win->pte)
	{
		UseFeature (featureStyleGraphics);
		CompReallyPreferBody(win);
		PetePrepareUndo(win->pte,peUndoPaste,-1,-1,&spot,nil);
		
		if (!(err = PeteInsertChar(win->pte,-1,' ',nil)))
			err = PeteFileGraphicRange(win->pte,spot,spot+1,spec,fgDisplayInline);

		if (!err)
		{
			PeteCalcOn(win->pte);
			PeteFinishUndo(win->pte,peUndoPaste,spot,spot+1);
		}
		else
		{
			PeteKillUndo(win->pte);
			WarnUser(PETE_ERR,err);
		}
	}
	return(err);
}

/**********************************************************************
 * CompReallyPreferBody - I'd really rather do something in the body now
 **********************************************************************/
void CompReallyPreferBody(MyWindowPtr win)
{
	if (GetWindowKind(GetMyWindowWindowPtr(win))==COMP_WIN)
	if (CompHeadCurrent(win->pte))
	{
		HeadSpec hs;
		NicknameWatcherFocusChange (win->pte); /* MJN */
		CompHeadFind(Win2MessH(win),0,&hs);
		PeteSelect(win,win->pte,hs.stop,hs.stop);
		RequestTFBEnableCheck(win); /* MJN *//* formatting toolbar */
	}
}

/************************************************************************
 * CompAttachSpec - attach a file to a comp window
 ************************************************************************/
void CompAttachSpec(MyWindowPtr win, FSSpecPtr spec)
{
	Str255 text;
	Str63 scratch;
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	HeadSpec hs;
	short oldStart;

	if (CompHeadFind(messH,ATTACH_HEAD,&hs))
	{
			PeteCalcOff(TheBody);
			ComposeRString(text,ATTACH_FMT,
			GetMyVolName(spec->vRefNum,scratch),
			spec->parID,
			spec->name);
		if (hs.stop!=hs.value)
			CompHeadAppendPtr(TheBody,&hs," ",1);
		oldStart = hs.stop;
		CompHeadAppendPtr(TheBody,&hs,text+1,*text);
#ifndef ATT_ICONS
		AttachSelect(messH);
#endif
		CompHeadFind(messH,ATTACH_HEAD,&hs);
		PeteLock(TheBody,hs.value,hs.stop,peModLock|peSelectLock);
#ifdef ATT_ICONS
		PeteFileGraphicRange(TheBody,oldStart,hs.stop,spec,fgAttachment+fgDontCopyToClip);
#endif
		PeteSetDirty(TheBody);
		win->isDirty = True;
		PeteCalcOn(TheBody);
		PeteKillUndo(TheBody);
	}
}

/**********************************************************************
 * CompUnattach - remove an attachment
 **********************************************************************/
void CompUnattach(MyWindowPtr win)
{
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	
	PETEInsertTextPtr(PETE,TheBody,-1,"",0,nil);
	AttachSelect(messH);
	win->isDirty = True;
}


#ifdef ETL
/**********************************************************************
 * InTranslator - do we need to do this translation?
 **********************************************************************/
Boolean InTranslator(TransInfoHandle translators,long id)
{
	short i;
	
	if (translators)
		for (i=HandleCount(translators);i--;)
			if ((*translators)[i].id==id) return(True);
	return(False);
}

/**********************************************************************
 * AddMessTranslator - add a translator from a message
 **********************************************************************/
OSErr AddMessTranslator(MessHandle messH,long which,Handle properties)
{
	short n;
	TransInfoHandle	localHandle;
	ControlHandle theCtl = FindControlByRefCon((*messH)->win,0xff000000|(which+ICON_BAR_NUM));
	long id = ETLIconToID(which);
	
	if (which==-1) return(fnfErr);
	
	if ((*messH)->hTranslators) n = HandleCount((*messH)->hTranslators);
	else n = 0;
	
	if (!n)
	{
		localHandle = NuHandle(sizeof(TransInfo));
		if (!localHandle) return(MemError());
		(*messH)->hTranslators = localHandle;
	}
	else
	{
		SetHandleBig_((*messH)->hTranslators,(n+1)*sizeof(TransInfo));
		if (MemError()) return(WarnUser(ETL_CANT_ADD_TRANS,MemError()));
	}
	(*(*messH)->hTranslators)[n].id = id;
	(*(*messH)->hTranslators)[n].properties = properties;
	if (theCtl) SetControlValue(theCtl,1);
	(*messH)->win->isDirty = True;
	return(noErr);
}

/**********************************************************************
 * RemoveMessTranslator - add or remove a translator from a message
 **********************************************************************/
OSErr RemoveMessTranslator(MessHandle messH,long which)
{
	short n;
	short i;
	ControlHandle theCtl = FindControlByRefCon((*messH)->win,0xff000000|(which+ICON_BAR_NUM));
	long id = ETLIconToID(which);
	
	if ((*messH)->hTranslators) n = HandleCount((*messH)->hTranslators);
	else n = 0;
	
	for (i=0;i<n;i++)
	{
		if ((*(*messH)->hTranslators)[i].id==id)
		{
			ZapHandle((*(*messH)->hTranslators)[i].properties);
			if (n==1) ZapHandle((*messH)->hTranslators);
			else
			{
				if (i!=n-1) BMD(&(*(*messH)->hTranslators)[i+1],&(*(*messH)->hTranslators)[i],(n-i-1)*sizeof(TransInfo));
				SetHandleBig_((*messH)->hTranslators,(n-1)*sizeof(TransInfo));
				break;
			}
		}
	}
	if (theCtl) SetControlValue(theCtl,0);
	(*messH)->win->isDirty = True;
	return(noErr);
}
#endif

void WarpQueue(uLong secs)
{
  TOCHandle tocH = GetOutTOC();
	MSumPtr sum;
  uLong now = GMTDateTime();
	
	if (tocH)
	{
	  for (sum=(*tocH)->sums;sum<(*tocH)->sums+(*tocH)->count;sum++)
		  if (sum->state == TIMED)
			{
			  if (!secs) sum->seconds = 0;
				else if (sum->seconds<secs+now)
				{
					sum->seconds = now;
					TOCSetDirty(tocH,true);
				}
			}
		SetSendQueue();
	}
}


#ifdef TWO
/************************************************************************
 * AttachDoc - attach a document to the topmost message, or create new if necessary
 ************************************************************************/
OSErr AttachDoc(MyWindowPtr win,FSSpecPtr spec)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle messH;
	HeadSpec hs;
	Str255 longName;
	
	if (winWP) messH = (MessHandle) GetWindowPrivateData (winWP);
	if (!winWP || !IsWindowVisible(winWP) || GetWindowKind(winWP) != COMP_WIN || SENT_OR_SENDING(SumOf(messH)->state))
	{
		win = DoComposeNew(0);
		if (CompHeadFind(Win2MessH(win),SUBJ_HEAD,&hs))
		{
			if (FSpGetLongName(spec,kTextEncodingUnknown,longName))
				PSCopy(longName,spec->name);
			CompHeadSetStr((*Win2MessH(win))->bodyPTE,&hs,longName);
		}
#ifdef TWO
		if (win) ApplyDefaultStationery(win,True,True);
#endif
		if (win) ShowMyWindow(GetMyWindowWindowPtr(win));
	}
	
	if (!win) return(1);	/* sorry, no cigar */
	
	CompAttachSpec(win,spec);
	return(noErr);
}

/************************************************************************
 * ApplyStationery - read and reconstitute a message from stationery
 ************************************************************************/
void ApplyStationery(MyWindowPtr win,FSSpecPtr spec,Boolean dontCleanse,Boolean personality)
{
	ApplyStationeryLo(win,spec,dontCleanse,personality,false);
}

/************************************************************************
 * ApplyStationery - read and reconstitute a message from stationery
 ************************************************************************/
void ApplyStationeryLo(MyWindowPtr win,FSSpecPtr spec,Boolean dontCleanse,Boolean personality,Boolean editStationery)
{
	Handle textH;
	
	UseFeature (featureStationery);

	/*
	 * grab the text
	 */
	if (Snarf(spec,&textH,0)) return;
	
	/*
	 * apply
	 */
	ApplyStationeryHandle(win,textH,dontCleanse,personality,editStationery);
		
	/*
	 * cleanup
	 */
	ZapHandle(textH);
}

/**********************************************************************
 * GetStationerySummary - get message summary from stationery text
 **********************************************************************/
OSErr GetStationerySum(Handle textH,MSumPtr pSum)
{
	UPtr spot, end, nl;

	/*
	 * fetch the summary info (first line)
	 */
	end = LDRef(textH) + GetHandleSize_(textH);
	for (spot=*textH;spot<end && *spot!=' ';spot++);
	spot++;
	for (nl=spot;nl<end && *nl!='\015';nl++);
	if (nl-spot!=2*sizeof(*pSum))
	{
		WarnUser(INVALID_STATIONERY,0);
		return -1;
	}
	Hex2Bytes(spot,nl-spot,(UPtr)pSum);
	return noErr;
}

/**********************************************************************
 * ApplyStationeryHandle - apply a handle as stationery
 **********************************************************************/
void ApplyStationeryHandle(MyWindowPtr win,Handle textH,Boolean dontCleanse,Boolean personality,Boolean editStationery)
{
	MSumType oldSum, newSum;
	MessHandle messH;
	Str255 scratch, origSubj;
	UPtr spot, end;
	long bodySpot;
	long size ;
	UPtr subj;
	Str255 into, sub;
	long newBodySpot;
	HeadSpec hs;
	PersHandle pers=nil;
	FSSpec	toSpec,fromSpec;
	short	label = editStationery ? 0 : pStationeryLabel;	//	Don't apply stationery label to text if opening stationery for edit
	
	if (!dontCleanse) CleanseStationery(win);
	
	/*
	 * fetch the summary info (first line)
	 */
	end = LDRef(textH) + GetHandleSize_(textH);
	if (GetStationerySum(textH,&oldSum))
		return;
	
	/*
	 * copy pertinent stuff
	 */
	messH = Win2MessH(win);
	newSum = *SumOf(messH);
	SumInfoCpy(&newSum,&oldSum);
	if (editStationery)
	{
		//	Editing stationery. Need to keep original uid
		newSum.msgIdHash = oldSum.msgIdHash;
		newSum.uidHash = oldSum.uidHash;
	}
	newSum.seconds = GMTDateTime();
	newSum.state = UNSENDABLE;
	if (!(oldSum.flags&FLAG_OLD_SIG)) newSum.sigId = SIG_NONE;
	*SumOf(messH) = newSum;
	
	if (personality && (pers = FindPersById(oldSum.persId)))
	{
		SetPers((*messH)->tocH,(*messH)->sumNum,pers,False);
		PushPers(pers);
	}

	for (spot = *textH;spot<end && (spot[0]!='\015'||spot[1]!='\015');spot++);
	bodySpot = spot<end-2 ? spot-(unsigned char *)*textH+2 : end-(unsigned char *)*textH;
	spot = *textH;

	/*
	 * from?
	 */
	size = bodySpot;
	if ((subj=FindHeaderString(spot,HeaderName(FROM_HEAD),&size,False)) && size)
	{
		Str255 from;
		
		MakePStr(from,subj,size);
		if (IsMe(from)) SetMessText(messH,FROM_HEAD,from+1,*from);
	}
	
	/*
	 * from?
	 */
	size = bodySpot;
	if ((subj=FindHeaderString(spot,HeaderName(TRANSLATOR_HEAD),&size,False)) && size)
	{
		AddTranslatorsFromPtr(messH,subj,size);
	}
	
	/*
	 * now, copy the headers
	 */
	TextFindAndCopyHeader(spot,bodySpot,messH,HeaderName(TO_HEAD),TO_HEAD,label);
	TextFindAndCopyHeader(spot,bodySpot,messH,HeaderName(CC_HEAD),CC_HEAD,label);
	TextFindAndCopyHeader(spot,bodySpot,messH,HeaderName(BCC_HEAD),BCC_HEAD,label);
	TextFindAndCopyHeader(spot,bodySpot,messH,HeaderName(ATTACH_HEAD),ATTACH_HEAD,label);
	
	/*
	 * subject gets special handling
	 */
	CompHeadGetStr(messH,SUBJ_HEAD,origSubj);
	if (*origSubj)
	{
		size = bodySpot;
		if ((subj=FindHeaderString(spot,HeaderName(SUBJ_HEAD),&size,False)) && size)
		{
			MakePStr(sub,subj,size);
			TrimWhite(sub);
			TrimInitialWhite(sub);
			if (*sub)
			{
				ComposeRString(into,STATION_SUBJ_FMT,sub,origSubj);
				SetMessText(messH,SUBJ_HEAD,into+1,*into);
				if (CompHeadFind(messH,SUBJ_HEAD,&hs))
					PeteLabel(TheBody,hs.value,hs.stop,label,label);
			}
			else
				*origSubj = 0;
		}
		else
			*origSubj = 0;
	}
	
	if (!*origSubj)
		TextFindAndCopyHeader(spot,bodySpot,messH,HeaderName(SUBJ_HEAD),SUBJ_HEAD,label);
	

	if (!editStationery && oldSum.opts&OPT_HAS_SPOOL)
	{
		//	Applying stationery. Need to copy stationery's spool folder.
		MakeAttSubFolder(messH,oldSum.uidHash,&fromSpec);
		MakeAttSubFolder(messH,newSum.uidHash,&toSpec);
		FSpDupFolder(&toSpec,&fromSpec,true,false);
	}


	/*
	 * we turn the text block into the body
	 */
	if (*textH+bodySpot<end)
	{
		newBodySpot = PETEGetTextLen(PETE,TheBody);
		if (oldSum.opts & OPT_HTML)
		{
			InsertRich(textH,bodySpot,end-(unsigned char *)*textH,newBodySpot,false,TheBody,nil,false);
			SetMessOpt(messH,OPT_HTML);
		}
		else
		{
			// This weird little dance avoids bug #3596
			// Sorta hacky?  Yes, indeed...
			Boolean oldSigOpt = MessOptIsSet(messH,OPT_INLINE_SIG);
			ClearMessOpt(messH,OPT_INLINE_SIG);
			
			// append the body
			AppendMessText(messH,0,*textH+bodySpot,end-(unsigned char *)*textH-bodySpot);
			
			// final steps of the hacky dance.
			if (oldSigOpt) SetMessOpt(messH,OPT_INLINE_SIG);
			
			if (oldSum.flags & FLAG_RICH)
			{
				PeteRich(TheBody,newBodySpot,PETEGetTextLen(PETE,TheBody),True);
				SetMessFlag(messH,FLAG_RICH);
			}
		}
		PeteLabel(TheBody,newBodySpot,PeteLen(TheBody),label,label);
		PeteLock(TheBody,newBodySpot,PeteLen(TheBody),0);
	}
	
	win->isDirty = False;
	PeteCleanList(win->pte);
	UL(textH);
	
	if (UseInlineSig) AddInlineSig(messH);
	
	RefreshSigButton(messH);
	
	UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
	
	// update the window's idea of what color it is
	(*messH)->win->label = GetSumColor((*messH)->tocH,(*messH)->sumNum);

	InvalTopMargin(win);
	CompIBarUpdate(messH);
	
	if (pers) PopPers();
	
	return;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr CleanseStationery(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	long start, stop;
	long len;
	
	/*
	 * remove every run labelled with stationery
	 */
	start = 0;
	while (!PETEFindLabelRun(PETE,TheBody,start,&start,&stop,pStationeryLabel,pStationeryLabel))
	{
		len = stop-start;
		PeteDelete(TheBody,start,stop);
		start = stop-len+1;	// on to next run
	}
	
	// Also, remove inline sig, if any
	RemoveInlineSig(Win2MessH(win));
	
	return(noErr);
}

#pragma segment Balloon
/************************************************************************
 * CompHelp - provide help for composition windows
 ************************************************************************/
void CompHelp(MyWindowPtr win,Point mouse)
{
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	Rect vr;
	short hnum=1;
	ControlHandle sendButton=(*messH)->sendButton;
	short i;
	
	if (GetPriorityRect(win,&vr))
	{
		if (PtInRect(mouse,&vr))
		{
			PriorMenuHelp(win,&vr);
			return;
		}
		
#ifdef TWO
		if (GetSigRect(win,&vr) && PtInRect(mouse,&vr))
		{
			SigMenuHelp(win,&vr);
			return;
		}
#endif
		
		if (GetAttachRect(win,&vr) && PtInRect(mouse,&vr))
		{
			AttMenuHelp(win,&vr);
			return;
		}

#ifdef VCARD
		if (IsVCardAvailable () && !PrefIsSet (PREF_HIDE_VCARD_BUTTON) && GetVCardRect(win,&vr) && PtInRect(mouse,&vr))
		{
			VCardBtnHelp(win,&vr);
			return;
		}
#endif
		
		if ((*messH)->analControl && PtInControl(mouse,(*messH)->analControl))
		{
			if ((*PeteExtra(TheBody))->taeScore)
			{
				Str255 s;
				ComposeRString(s,ANAL_COMP_HELP,OffensivenessStrn+(*PeteExtra(TheBody))->taeScore);
				GetControlBounds((*messH)->analControl,&vr);
				MyBalloon(&vr,90,0,0,0,s);
			}
			return;
		}
		
		GetControlBounds(sendButton,&vr);
		if (PtInRect(mouse,&vr))
		{
			if ((*messH)->hStationerySpec) hnum+=2;
			else if (PrefIsSet(PREF_AUTO_SEND)) hnum++;
			MyBalloon(&vr,90,0,COMP_HELP_STRN+hnum,0,nil);
			return;
		}
		hnum += 3;
		
		if (win->topMargin)
		{
			for (i=0;i<ICON_BAR_NUM+(*messH)->nTransIcons;i++,hnum+=2)
			{
				if (PtInRect(mouse,CompIconRect(&vr,messH,i)))
				{
					switch(i)
					{
						case 0:
							if (MessOptIsSet(messH,OPT_COMP_TOOLBAR_VISIBLE)) hnum++;
							break;
						case 1:
							if (SumOf(messH)->flags&FLAG_CAN_ENC) hnum++;
							break;
						case 2:
							if (SumOf(messH)->flags&FLAG_BX_TEXT) hnum++;
							break;
						case 3:
							if (SumOf(messH)->flags&FLAG_WRAP_OUT) hnum++;
							break;
						case 4:
							if (SumOf(messH)->flags&FLAG_KEEP_COPY) hnum++;
							break;
#ifdef TWO
						case 5:
							if (SumOf(messH)->flags&FLAG_RR) hnum++;
							break;
#else
						case 5:
							if (SumOf(messH)->flags&FLAG_SIG) hnum++;
							break;
#endif
						default:
							CompTranslatorHelp(&vr,i-ICON_BAR_NUM);
							return;
					}
					MyBalloon(&vr,90,0,COMP_HELP_STRN+hnum,0,nil);
					return;
				}
			}
			if (TextFormattingBarHelp(win,mouse)) return;
		}
	}
}

/**********************************************************************
 * CompTranslatorHelp - balloon help for the translators
 **********************************************************************/
void CompTranslatorHelp(Rect *r,short i)
{
	Str255 module, translator, help;
	
	if (!ETLIconToDescriptions(i,module,translator))
	{
		ComposeRString(help,ETL_ICON_HELP_FMT,module,translator);
		MyBalloon(r,90,0,0,0,help);
	}
	else HMRemoveBalloon();
}

#ifdef TWO
/************************************************************************
 * SigMenuHelp - balloon help for the signature menu
 ************************************************************************/
void SigMenuHelp(MyWindowPtr win,Rect *r)
{
	Str255 s1,s2;
	long fid = SumOf(Win2MessH(win))->sigId;
	Boolean sig = SIG_NONE!=(SumOf(Win2MessH(win))->sigId);

	if (!HMIsBalloon())
	{
		GetRString(s1,SIG_MENU_HELP);
		GetRString(s2,sig?(fid?(fid==1?ALT_SIG_HELP:ARB_SIG_HELP):SIG_HELP):NOSIG_HELP);
		PCatC(s1,' ');
		PSCat(s1,s2);
		MyBalloon(r,90,0,0,0,s1);
	}
}
#endif

/************************************************************************
 * AttMenuHelp - balloon help for the signature menu
 ************************************************************************/
void AttMenuHelp(MyWindowPtr win,Rect *r)
{
	Str255 s1,s2;
	long aType = AttachOptNumber((SumOf(Win2MessH(win)))->flags);

	if (!HMIsBalloon())
	{
		GetRString(s1,ATT_HELP_STRN+1);
		GetRString(s2,ATT_HELP_STRN+2+aType);
		PSCat(s1,s2);
		MyBalloon(r,90,0,0,0,s1);
	}
}

#ifdef VCARD
/************************************************************************
 * AttMenuHelp - balloon help for the signature menu
 ************************************************************************/
void VCardBtnHelp(MyWindowPtr win,Rect *r)
{

}
#endif

#pragma segment CompWin

/************************************************************************
 * CompZoomSize - figure the zoomed size of a comp window
 ************************************************************************/
void CompZoomSize(MyWindowPtr win,Rect *zoom)
{
#pragma unused(win)
	short hi,wi;
	
	wi = MessWi(win);
	
	zoom->right = zoom->left + MIN(zoom->right-zoom->left,wi);
	
	if (hi=GetPrefLong(PREF_MHEIGHT))
	{
		hi *= win->vPitch;
		zoom->bottom = MIN(zoom->bottom,zoom->top+hi);
	}
}
	
/**********************************************************************
 * CompDidResize - take care of a resized composition window
 **********************************************************************/
void CompDidResize(MyWindowPtr win, Rect *oldContR)
{
#pragma unused(oldContR)
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	MessHandle messH = (MessHandle)GetWindowPrivateData(winWP);
	ControlHandle sendButton;
	short i;
	ControlHandle cntl;
	Rect r,rCntl,rPort;
	short wi;
	
	GetPortBounds(GetWindowPort(winWP),&rPort);
	wi = RectWi(rPort);
	/*
	 * text
	 */
	PeteDidResize(TheBody,&win->contR);
	
	/*
	 * the send button
	 */
	if (sendButton = (*messH)->sendButton)
	{
		GetControlBounds(sendButton,&r);
	/* MJN *//* text formatting toolbar */
/*		OFFSET_RECT(&(*sendButton)->contrlRect,
			((GrafPtr)win)->portRect.right-(r.right-r.left)/4-r.right,
			(win->topMargin-3-r.bottom+r.top)/2-r.top); */
		OffsetRect(&r,wi-INSET-r.right,0);
		MySetCntlRect(sendButton,&r);
	}
	
	if ((*messH)->analControl)
	{
		GetAnalRect(win,&r);
		MySetCntlRect((*messH)->analControl,&r);
	}
	
	/*
	 * icon bar buttons
	 */
	for (i=0;i<ICON_BAR_NUM+(*messH)->nTransIcons;i++)
	{
		cntl = FindControlByRefCon(win,0xff000000 | i);
		if (cntl)
		{
			CompIconRect(&r,Win2MessH(win),i);
			MoveMyCntl(cntl,r.left,r.top,RectWi(r),RectHi(r));
		}
	}
	
	if (TextFormattingBarVisible(win))
	{
		// format bar
		if ((cntl=FindControlByRefCon(win,kControlUserPaneProc)))
		{
			GetControlBounds(cntl,&rCntl);
			MoveMyCntl(cntl,2,rCntl.top,wi-4,0);
			SetControlVisibility(cntl,true,false);
		}
			
		// format bar divider
		if (cntl=FindControlByRefCon(win,kSeparatorRefCon))
		{
			GetControlBounds(cntl,&rCntl);
			MoveMyCntl(cntl,INSET,rCntl.top,wi-2*INSET,0);
			SetControlVisibility(cntl,true,false);
		}
	}
	
	// gotta reposition error messages
	PlaceMessErrNote(messH);
	
	//InvalTopMargin(win); 	/* sorry... */
}


/************************************************************************
 * CompGonnaShow - select an appropriate field, setup the icon bar controls
 ************************************************************************/
OSErr CompGonnaShow(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	HeadSpec hs;
	ControlHandle cntl;
	short i;
	Rect r;
	
	i = GetRLong(COMP_TOP_MARGIN);
	if (TextFormattingBarVisible(win))
		i += GetTxtFmtBarHeight();
	SetTopMargin(win,i);
		
	for (i=0;i<ICON_BAR_NUM;i++)
	{
		CompIconRect(&r,Win2MessH(win),i);
		cntl = GetNewControl(ICON_BAR_CNTL,GetMyWindowWindowPtr(win));
		if (cntl)
		{
			EmbedControl(cntl,win->topMarginCntl);
			SetControlReference(cntl,0xff000000|i);
			SetBevelIcon(cntl,icons[i],0,0,nil);
			if (fBits[i]<0)
				SetControlValue(cntl,MessOptIsSet(Win2MessH(win),(-fBits[i])));
			else
				SetControlValue(cntl,MessFlagIsSet(Win2MessH(win),fBits[i]));
			SetControlVisibility(cntl,true,false);
		}
	}
	
	AddPriorityPopup(messH);
	AddSigPopup(messH);
	AddAttachPopup(messH);
#ifdef VCARD
	if (IsVCardAvailable () && !PrefIsSet (PREF_HIDE_VCARD_BUTTON))
		AddVCardButton (messH);
#endif
	if (BeingAnal()) AddAnalIcon(messH);
	
	// Add error note?
	AddMessErrNote(messH);
		
/* MJN */
	AddTextFormattingBarIcons(win,TextFormattingBarVisible(win),TextFormattingBarOK(win));
#ifdef ETL
	ETLAddIcons(win,ICON_BAR_NUM);
#endif

	win->update = DrawCompWindowBars; /* MJN *//* changed the update proc */
	win->activate = CompActivate;

	if (HasFeature (featureMultiplePersonalities) && PersCount()>1 && !SENT_OR_SENDING(SumOf(messH)->state)) CompAddPersPopup(win);
		
	SetBGColorsByPers(messH);

	PeteCleanList(win->pteList);
	win->isDirty = False;
	win->butch = true;
#ifdef USECMM_BAD
	win->buildCMenu = CompBuildCMenu;
#endif
	
#ifdef ATT_ICONS
	{
		FSSpec spec;
	
		for (i=1;1!=GetIndAttachment(messH,i,&spec,&hs);i++)
			PeteFileGraphicRange(TheBody,hs.start,hs.stop,&spec,fgAttachment);
	}
#endif

	// (jp) We'll supports lots of nickname scanning features for the headers
	SetNickScanning (TheBody, nickHighlight | nickComplete | nickExpand | nickCache | nickSpaces, kNickScanAllAliasFiles, IsHeaderNickField, HiliteCompHeader, GetCompNickFieldRange);
	// (jp) Special, for headers (other fields should leave this up to the nick scanning code)
	PeteNickScan (TheBody);

	// Grab the initial addresses for this message and hang onto them until we queue the message.
	// At that time we'll compare the before and after headers to determine what the user has
	// typed as "recent" (for the purpose of building our recently used addresses).
	CompGatherRecipientAddresses (messH, true);

	i = GetRLong(COMP_EXTRA_LINES)*win->vPitch;
	if (i) PETESetExtraHeight(PETE,TheBody,i);
	
	PeteCalcOn(TheBody);
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking))
		(*PeteExtra(TheBody))->spelled = 0;	// allow spelling here;
#endif

	CompActivateAppropriate(messH);
		
	PETEScroll(PETE,TheBody,0,0);

	EnableTxtFmtBarIfOK(win); /* MJN *//* text formatting bar */

	if (HasFeature (featureMultiplePersonalities))
		RedrawPersPopup(messH);
	
	if (IsQueued((*messH)->tocH,(*messH)->sumNum)) SetSendQueue();

	(*messH)->openToEnd = false;
	
	(*PeteExtra(TheBody))->taeDesired = !SENT_OR_SENDING(SumOf(messH)->state);
	(*PeteExtra(TheBody))->emoDesired = true;
	
	return(noErr);
}

/************************************************************************
 * AddPriorityPopup - add a priority popup to a window
 ************************************************************************/
OSErr AddPriorityPopup(MessHandle messH)
{
	MyWindowPtr win = (*messH)->win;
	short p = Prior2Display(SumOf(messH)->priority);
	Rect r;
	ControlHandle cntl;

	if (GetPriorityRect(win,&r))
		if (cntl = GetNewControlSmall(PRIOR_CNTL,GetMyWindowWindowPtr(win)))
		{
			SetControlReference(cntl,PRIOR_HIER_MENU);
			MySetCntlRect(cntl,&r);
			EmbedControl(cntl,win->topMarginCntl);
			SetBevelIcon(cntl,p==3 ? PRIORITY_HOLLOW_ICON : PRIOR_SICN_BASE+p-1,0,0,nil);
			SetBevelMenuValue(cntl,p);
			return(noErr);
		}
	return(fnfErr);
}

/************************************************************************
 * AddAttachPopup - add an attachment popup to a window
 ************************************************************************/
OSErr AddAttachPopup(MessHandle messH)
{
	MyWindowPtr win = (*messH)->win;
	short opt = AttachOptNumber(SumOf(messH)->flags);
	Rect r;
	ControlHandle cntl;

	if (GetAttachRect(win,&r))
		if (cntl = GetNewControlSmall(ATTACH_CNTL,GetMyWindowWindowPtr(win)))
		{
			SetControlReference(cntl,ATTACH_MENU);
			MySetCntlRect(cntl,&r);
			EmbedControl(cntl,win->topMarginCntl);
			SetBevelIcon(cntl,ATYPE_SICN_BASE+opt,0,0,nil);
			SetBevelMenuValue(cntl,opt+1);
			return(noErr);
		}
	return(fnfErr);
}

/************************************************************************
 * AddSigPopup - add a priority popup to a window
 ************************************************************************/
OSErr AddSigPopup(MessHandle messH)
{
	MyWindowPtr win = (*messH)->win;
	Rect r;
	ControlHandle cntl;

		if (GetSigRect(win,&r))
			if (cntl = GetNewControlSmall(SIG_CNTL,GetMyWindowWindowPtr(win)))
			{
				SetControlReference(cntl,SIG_HIER_MENU);
				MySetCntlRect(cntl,&r);
				EmbedControl(cntl,win->topMarginCntl);
				SetBevelIcon(cntl,SigIconId(messH),0,0,nil);
			return(noErr);
		}

	return(fnfErr);
}


#ifdef VCARD
/************************************************************************
 * AddVCardButton - add a vCard attachment button to a window
 ************************************************************************/
OSErr AddVCardButton (MessHandle messH)

{
	MyWindowPtr 	win;
	ControlHandle	cntl;
	Rect					r;
	
	// Only addd the vCard buttn if we actually have personal information to share
	if (AnyPersonalNicknames ()) {
		win = (*messH)->win;
		if (GetVCardRect (win, &r))
			if (cntl = NewControl (GetMyWindowWindowPtr (win), &r, "", true, 0, kControlBehaviorPushbutton | kControlContentIconSuiteRes, PERSONAL_NICKNAME_ICON, kControlBevelButtonSmallBevelProc, 0)) {
				SetControlReference(cntl, 'vcrd');
				MySetCntlRect (cntl, &r);
				EmbedControl (cntl, win->topMarginCntl);
				return(noErr);
			}
	}
	return (fnfErr);
}

#endif

/************************************************************************
 * AddAnalIcon - add the text analysis icon control
 ************************************************************************/
OSErr AddAnalIcon(MessHandle messH)
{
	MyWindowPtr win = (*messH)->win;
	Rect r;
	ControlHandle cntl;
	short score = SumOf(messH)->score;

	if (GetAnalRect(win,&r))
		if (cntl = NewControl(GetMyWindowWindowPtr(win),&r,"",false,score ? AnalIcon(score):0,0,0,kControlIconSuiteNoTrackProc,ANAL_ICON_BASE))
		{
			MySetCntlRect(cntl,&r);
			EmbedControl(cntl,win->topMarginCntl);
			(*messH)->analControl = cntl;
			SetControlReference(cntl,'anal');
			if (score) ShowControl((*messH)->analControl);
			return(noErr);
		}
	return(fnfErr);
}

/************************************************************************
 * CompActivateAppropriate - activate a reasonable field
 ************************************************************************/
void CompActivateAppropriate(MessHandle messH)
{
	HeadSpec hs;
	
	if ((*messH)->dontActivate) return;
	
	if (CompHeadFind(messH,TO_HEAD,&hs) && hs.value==hs.stop)
		CompHeadActivate(TheBody,&hs);
	else if (CompHeadFind(messH,SUBJ_HEAD,&hs) && hs.value==hs.stop)
		CompHeadActivate(TheBody,&hs);
	else if (CompHeadFind(messH,0,&hs))
	{
		if ((*messH)->openToEnd) hs.value = hs.stop;
		CompHeadActivate(TheBody,&hs);
	}
}


#define POPUP_FUDGE 40
typedef struct
{
	PETEGraphicInfo gi;
	ControlHandle popup;
} PersPopupType, *PersPopupPtr, **PersPopupHandle;
/**********************************************************************
 * CompAddPersPopup - install the personality popup
 **********************************************************************/
OSErr CompAddPersPopup(MyWindowPtr win)
{
	PersPopupHandle pp;
	MessHandle messH;
	ControlHandle cntl;
	HeadSpec hs;
	PETEStyleEntry pse;
	long junk, len, persLen;
	OSErr err;
	Str255 s;
	short hi;
	PersHandle messPers;
	MenuHandle mh;
	PersHandle pers;
	
	err = noErr;
	if (HasFeature (featureMultiplePersonalities)) {
		SAVE_STUFF;
		pp = NewZH(PersPopupType);
		messH = Win2MessH(win);
		messPers = PERS_FORCE(MESS_TO_PERS(messH));
		mh = GetMHandle(PERS_HIER_MENU);
		
		if (pp && CompHeadFind(messH,FROM_HEAD,&hs))
		{
			Rect r;
			ConfigFontSetup(win);
			len = 0;
			for (pers=PersList;pers;pers=(*pers)->next)
			{
				persLen = StringWidth(PCopy(s,(*pers)->name));
				len = MAX(len,persLen);
			}
			len += INSET;
			SetRect(&r,0,0,len,FontLead+1);
			cntl = NewControlSmall(GetMyWindowWindowPtr(win),&r,"",False,0,PERS_HIER_MENU,0,kControlPopupButtonProc,0);
			if (cntl)
			{
				SetControlMinimum(cntl,1);
				SetControlMaximum(cntl,PersCount()+1);
				SetControlValue(cntl,messPers==PersList ? 1 : Pers2Index(messPers)+2);
				SetControlReference(cntl,'pers');
				LetsGetSmall(cntl);
				ButtonFit(cntl);
				(*pp)->popup = cntl;
				EnableItem(GetMHandle(PERS_HIER_MENU),0);
				(*pp)->gi.width = len+POPUP_FUDGE;
				(*pp)->gi.height = hi = ControlHi(cntl)+3;
				if (hi<FontAscent) (*pp)->gi.descent = 0;
				else if (hi<FontLead) (*pp)->gi.descent = hi - FontAscent;
				else (*pp)->gi.descent = (hi-FontAscent)/2;
				(*pp)->gi.itemProc = PersGraphic;
				(*pp)->gi.cloneOnlyText = true;
				Zero(pse);
				PETEGetStyle(PETE,TheBody,hs.stop,&junk,&pse);
				pse.psStyle.graphicStyle.graphicInfo = (PETEGraphicInfoHandle)pp;
				if (err=PETESetStyle(PETE,TheBody,hs.stop,hs.stop+1,&pse.psStyle,(peAllValid&~peColorValid)|peGraphicValid|peGraphicColorChangeValid))
				{
					DisposeControl(cntl);
					ZapHandle(pp);
				}
				else (*messH)->persGraphic = (Handle)pp;
			}
			else ZapHandle(pp);
		}
		else err = fnfErr;
		REST_STUFF;
	}
	return(err);
}

/**********************************************************************
 * PersGraphic - Callback to handle a graphic that is a personality popup
 **********************************************************************/
pascal OSErr PersGraphic(PETEHandle pte,PETEGraphicInfoHandle graphic,long offset,PETEGraphicMessage message,void *data)
{
	OSErr err = noErr;
	Rect r;
	Handle picture=nil, alias=nil;
	Point pt;
	ControlHandle cntl = (*(PersPopupHandle)graphic)->popup;
	MyWindowPtr win = (*PeteExtra(pte))->win;
	MessHandle messH = Win2MessH(win);
	PETEDocInfo info;
	short track;
			
	switch (message)
	{
		case peGraphicDraw:	/* data is (Point *) with penLoc at baseline left as a point */
			if (cntl)
			{
				Zero(info);
				EnableItem(GetMHandle(PERS_HIER_MENU),0);
				PETEGetDocInfo(PETE,pte,&info);
				if (info.printing) return(noErr); // do nothing when printing
				if (PeteGraphicRect(&r,pte,graphic,offset))
				{
					(*Win2MessH(win))->redrawPersPopup = true;
					return noErr;	// try again at idle
				}
				r.left += 8;
				InsetRect(&r,0,2);
				//r.top += 2;
				MySetCntlRect(cntl,&r);
				SetControlVisibility(cntl,true,false);
				DrawControlInCurrentPort(cntl);
				SetControlVisibility(cntl,false,false);
			}
			break;

		case peGraphicClone:	/* data is (PETEGraphicInfoHandle *) into which to put duplicate */
			//	When user does copy function, make copy of graphic
			err = -108;
			break;
			
		case peGraphicTest:	/* data is (Point *) from top left; return errOffsetIsOutsideOfView if not hit */
			//	Can cancel hit by returning errOffsetIsOutsideOfView
			PeteGraphicRect(&r,pte,graphic,offset);
			pt = *(Point*)data;
			pt.h += r.left;
			pt.v += r.top;
			r.left += 8;
			InsetRect(&r,2,2);
			if (!PtInRect(pt,&r)) err = errOffsetIsOutsideOfView;
			else
			{
				long selStart, selEnd;
				PeteGetTextAndSelection(pte,nil,&selStart,&selEnd);
				(*messH)->testSelStart = selStart;
				(*messH)->testSelEnd = selEnd;
			}
			break;
			
		case peGraphicHit:	/* data is (EventRecord *) if mouse down, nil if time to turn off */
			if (cntl && data)
			{
				PersHandle oldPers = PERS_FORCE(MESS_TO_PERS(messH));
				short item = oldPers==PersList ? 1 : Pers2Index(oldPers)+2;
				PETEDocInfo info;
				
				// Restore the old selection
				PeteSelect(win,pte,(*messH)->testSelStart,(*messH)->testSelEnd);
				
				// activate window instead of hit control?
				PETEGetDocInfo(PETE,pte,&info);
				if (!info.docActive) return(tsmDocNotActiveErr);

				// pass control to control
				pt = ((EventRecord*)data)->where;
				GlobalToLocal(&pt);
				CheckNone(GetMHandle(PERS_HIER_MENU));
				PeteGraphicRect(&r,pte,graphic,offset);
				r.left += 8;
				InsetRect(&r,2,2);
				MySetCntlRect(cntl,&r);
				SetControlVisibility(cntl,true,false);
				SetControlValue(cntl,item);
				DrawControlInCurrentPort(cntl);
				track = TrackControl(cntl,pt,nil);
				SetControlVisibility(cntl,false,false);
				if (track)
				{
					PersHandle pers;
					short newItem;
					
					newItem = GetControlValue(cntl);
						
					if (newItem && newItem != item)
					{
						pers = newItem==1 ? PersList : Index2Pers(newItem-2);
						SetPers((*messH)->tocH,(*messH)->sumNum,pers,True);
					}
				}
			}
			break;		

		case peGraphicRemove:	/* data is nil; just dispose a copy of graphic */
			DisposeHandle((Handle)graphic);
			if (cntl) DisposeControl(cntl);
			break;
			
		case peGraphicResize:	/* data is a (short *) of the max width */
			break;
		
		case peGraphicRegion:	/* data is a RgnHandle which is to be changed to the graphic's region */
			//	Used mostly for changing the cursor
			PeteGraphicRect(&r,pte,graphic,offset);
			RectRgn((RgnHandle)data,&r);
			break;
	}
	
	return(err);
}

/************************************************************************
 * PersPopup - put up the priority menu, and set the priority
 ************************************************************************/
void PersPopup(MyWindowPtr win,Point where)
{
	MenuHandle pmh = GetMHandle(PERS_HIER_MENU);
	MessHandle messH = Win2MessH(win);
	PersHandle oldPers = PERS_FORCE(MESS_TO_PERS(messH));
	PersHandle pers;
	long res;
	short item, newItem;
	
	/*
	 * make sure the right priority gets selected
	 */
	CheckPers(win,False);
	
	/*
	 * use it
	 */
	if (pmh)
	{
		InsertMenu(pmh,-1);
		item = oldPers==PersList ? 1 : Pers2Index(oldPers)+2;
		res = AFPopUpMenuSelect(pmh,where.v,where.h,item);
		newItem = res&0xffff0000 ? res&0xff : 0;
		
		if (newItem && newItem != item)
		{
			pers = newItem==1 ? PersList : Index2Pers(newItem-2);
			SetPers((*messH)->tocH,(*messH)->sumNum,pers,True);
		}
	}
}

/**********************************************************************
 * CompActivate - activate a composition window; really just make sure
 *  we update the title bar
 **********************************************************************/
void CompActivate(MyWindowPtr win)
{
	short cur;
	MessHandle messH = Win2MessH(win);

	if (!win->isActive)
	{
		cur = CompHeadCurrent(TheBody);
		if (cur) CompLeaving(messH,cur);
	}
	else
		RequestTFBEnableCheck(win);
	SetTxtFmtBarEnabled(win,win->isActive?TextFormattingBarOK(win):false); /* MJN *//* text formatting bar */

	CompSendBtnUpdate(win);
	
	if (SENT_OR_SENDING(SumOf(Win2MessH(win))->state))
		DeactivateControl(win->topMarginCntl);
	else
		ActivateControl(win->topMarginCntl);

	if (HasFeature (featureMultiplePersonalities))
		RedrawPersPopup(messH);
}

/**********************************************************************
 * RedrawPersPopup - force the personality popup to redraw
 **********************************************************************/
void RedrawPersPopup(MessHandle messH)
{
	CGrafPtr messWinPort;
	Rect r;

	if (!HasFeature (featureMultiplePersonalities))
		return;
		
	(*messH)->redrawPersPopup = false;
	
	if (messH && TheBody)
	{
		if ((*messH)->persGraphic)
		{
			PETEStyleInfo info;
			long offset;
			
			Zero(info);
			info.graphicStyle.graphicInfo = (void*)(*messH)->persGraphic;
			if (!PETEFindStyle(PETE,TheBody,0,&offset,&info,peGraphicValid))
			{
				RgnHandle newClip = NewRgn();
				RgnHandle oldClip;
				if (newClip)
				{
					messWinPort = GetMyWindowCGrafPtr((*messH)->win);
					oldClip = SavePortClipRegion(messWinPort);
					PeteRect(TheBody,&r);
					r.right -= GROW_SIZE;
					RectRgn(newClip,&r);
					SetPortClipRegion(messWinPort,newClip);
					PushGWorld();
					SetPort_(messWinPort);
					PersGraphic(TheBody,(void*)(*messH)->persGraphic,offset,peGraphicDraw,nil);
					PopGWorld();
					RestorePortClipRegion(messWinPort,oldClip);
					DisposeRgn(newClip);
				}
			}
		}
	}
}
	
/**********************************************************************
 * CompIBarUpdate - update the icon bar
 **********************************************************************/
void CompIBarUpdate(MessHandle messH)
{
	short i;
	ControlHandle cntl;
	
	for (i=0;i<ICON_BAR_NUM;i++)
	{
		if (cntl=FindControlByRefCon((*messH)->win,0xff000000|i))
		{
			if (fBits[i]<0)
				SetControlValue(cntl,MessOptIsSet(messH,(-fBits[i])));
			else
				SetControlValue(cntl,MessFlagIsSet(messH,fBits[i]));
		}
	}
	RefreshSigButton(messH);
}


/**********************************************************************
 * CompClick - handle a click in a comp window.
 **********************************************************************/
void CompClick(MyWindowPtr win, EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	MessHandle messH = (MessHandle)GetWindowPrivateData(winWP);
	Point pt;
	short cur = CompHeadCurrent(TheBody);
	short new;
	Rect r;
	
	SetPort(GetWindowPort(winWP));
	pt = event->where;
	GlobalToLocal(&pt);
	
	if (GetSigRect(win,&r) && PtInRect(pt,&r))
	{
		RefreshSigPopup(win);
		HandleControl(pt,win);
	}
	else if (TextFormattingBarClick(win,event,pt)); /* MJN *//* text formatting bar */
	else 
	if (PtInPETE(pt,TheBody))
	{
		long fDirty;
		long fOldDirty = PeteIsDirty(TheBody);

		if (ClickType==Triple && IsAddressHead(CompHeadCurrent(TheBody)))
			AddrSelect(messH);
		else
		{
			PeteEditWFocus(win,win->pte,peeEvent,(void*)event,nil);
			new = CompHeadCurrent(TheBody);
			fDirty = PeteIsDirty(TheBody);
			/* MJN *//* text formatting bar */
			if (new!=cur)
			{
				if (cur!=0 && (*messH)->fieldDirty!=fDirty)
					CompLeaving(messH,cur); /* CompLeaving will call SetTxtFmtBarEnabled */
				else
					EnableTxtFmtBarIfOK(win); /* MJN *//* text formatting bar */
			}
			if (fDirty!=fOldDirty) (*messH)->fieldDirty = -1;
			if (new==ATTACH_HEAD) AttachSelect(messH);
		}
	}
	else if (win->labelCntl && PtInControl(pt,win->labelCntl))
	{
		BoxLabelMenu((*messH)->tocH,(*messH)->sumNum,messH,pt);
	}
	else
	{
		EnableMenuItems(False);
		SetMenuTexts(0,False);
		HandleControl(pt,win);
	}
}

OSErr CompLeaving(MessHandle messH,short head)
{
	short fDirty = PeteIsDirty(TheBody);
	OSErr err = noErr;
	MyWindowPtr win;

	win = (**messH).win;
	
	if ((*messH)->alreadyLeaving) return noErr;
	
	(*messH)->alreadyLeaving = true;

	if (fDirty != (*messH)->fieldDirty)
	{
		PushPers(PERS_FORCE(MESS_TO_PERS(messH)));
		if (IsAddressHead(head))
			err = NickExpandAndCacheHead (messH,head,false);
		UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
		fDirty = PeteIsDirty(TheBody);
		(*messH)->fieldDirty = fDirty;
		PopPers();
	}
	EnableTxtFmtBarIfOK(win); /* MJN *//* text formatting bar */

	(*messH)->alreadyLeaving = false;
	return(err);
}


OSErr NickExpandAndCacheHead(MessHandle messH,short head,Boolean cacheOnly)
{
	OSErr err=fnfErr;
	HeadSpec hs;
	UHandle text=nil;
	BinAddrHandle raw=nil;
	long len;
	
	if (CompHeadFind(messH,head,&hs) && hs.stop-hs.value)
	{
		if (!(err=CompHeadGetText(TheBody,&hs,&text)))
		{
			if (!(err=SuckAddresses(&raw,text,True,True,False,nil)))
			{
				ZapHandle(text);
				NicknameCachingScan (TheBody, raw);	
				if (PrefIsSet(PREF_NICK_AUTO_EXPAND) && !cacheOnly)
					if (!(err=ExpandAliases(&text,raw,0,True)))
					{
						ZapHandle(raw);
						CommaList(text);
						if (len = GetHandleSize(text)) {
						
						   // See if expansion caused any changes. Don't replace text
						   // if no changes so selection doesn't change
					   	Handle fieldText;
					   	long selStart, selEnd, fieldLen;

					   	PeteGetTextAndSelection(TheBody,&fieldText,&selStart,&selEnd);
   					   fieldLen = hs.stop-hs.value;
						   if (fieldLen != len || memcmp(*text,*fieldText+hs.value,fieldLen))
						   {
						      // Text has changed
   							PetePrepareUndo(TheBody,peCantUndo,hs.value,hs.stop,nil,nil);
   							if (!(err=PeteDelete(TheBody,hs.value,hs.stop)))
   							{
   								if (!(err=PeteInsertPtr(TheBody,hs.value,LDRef (text),GetHandleSize (text))))
   								{
         							// If previous selection was entire field, reselect entire field
   								   Boolean selectAll = selStart==hs.value && selEnd==hs.stop;
   									if (CompHeadCurrent(TheBody)==head && CompHeadFind(messH,head,&hs)) 
   									   PeteSelect((*messH)->win,TheBody,selectAll ? hs.value : hs.stop,hs.stop);
   								}
   								UL(text);
   							}
   							PeteFinishUndo(TheBody,peUndoPaste,hs.value,hs.value+len);
   						}
						}
						CompGatherRecipientAddresses (messH, true);
					}
			}
		}
	}
	ZapHandle(text);
	ZapHandle(raw);
	return(err);
}

#ifdef TWO

/************************************************************************
 * RefreshSigPopup - refresh the signature menu
 ************************************************************************/
void RefreshSigPopup(MyWindowPtr win)
{
	ControlHandle cntl = FindControlByRefCon(win,SIG_HIER_MENU);

	if (cntl)
	{
		MenuHandle mHandle=GetBevelMenu(cntl);
		if (mHandle)
		{
			TrashMenu(mHandle,sigmAlternate+1);
			CopyExtraSigs(mHandle);
			SetBevelMenuValue(cntl,SigMenuItem(Win2MessH(win)));
		}
	}
}

/**********************************************************************
 * SigFIDByName - turn the name of a sig file into the sig's fid
 **********************************************************************/
uLong SigFIDByName(PStr string)
{
	FSSpec spec;
	uLong fid;
	
	SigSpecByName(string,&spec);
	if (FSMakeFID(&spec,&fid)) return(0);
	return(fid);
}

/**********************************************************************
 * SigSpecByName - turn a filename into a signature FSSpec
 **********************************************************************/
OSErr SigSpecByName(PStr string,FSSpecPtr spec)
{
	OSErr err = SubFolderSpec(SIG_FOLDER,spec);
	
	if (!err) err = FSMakeFSSpec(spec->vRefNum,spec->parID,string,spec);
	return(err);
}

/**********************************************************************
 * CopyExtraSigs - copy extra sigs to end of sig menu
 **********************************************************************/
void CopyExtraSigs(MenuHandle onto)
{
	short item, ontoItem, count;
	Str63 string;
	Boolean	multipleSignatures;
	
	multipleSignatures = false;
	count = SignatureCount();
	ontoItem = CountMenuItems(onto);
	for (item=shmAlternate+1;item<=count;item++)
	{
		ontoItem++;
		GetSignatureName(item,string);
		MyAppendMenu(onto,string);
		if (!HasFeature (featureSignatures))
			SetMenuItemBasedOnFeature (onto, ontoItem, featureSignatures, -1);
		else
			SetItemReducedIcon(onto,ontoItem,GetSignatureIcon(item));
		multipleSignatures = true;
	}
	if (multipleSignatures && HasFeature (featureSignatures))
		UseFeature (featureSignatures);
}
		
/**********************************************************************
 * SetSig - set the signature of a message
 **********************************************************************/
#pragma optimization_level 0	// optimizer bug steps on messH here
OSErr SetSig(TOCHandle tocH,short sumNum,long sigId)
{
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	
	if (sigId==-1)
		(*tocH)->sums[sumNum].sigId = SIG_NONE;
	else
		(*tocH)->sums[sumNum].sigId = sigId;

	TOCSetDirty(tocH,true);
	if (messH)
	{
		Boolean peteDirtyWas = PeteIsDirty(TheBody)!=0;
		Boolean winDirtyWas = (*PeteExtra(TheBody))->win->isDirty;
		
		RefreshSigButton(messH);
		
		if ((*messH)->hStationerySpec) (*messH)->win->isDirty = True;
		
		if (MessOptIsSet(messH,OPT_INLINE_SIG)) RemoveInlineSig(messH);
		if (UseInlineSig) AddInlineSig(messH);
		else
		{
			if (!peteDirtyWas) PETEMarkDocDirty(PETE,TheBody,False);
			(*messH)->win->isDirty = winDirtyWas;
		}
	}
	return(noErr);
}
#pragma optimization_level reset

/**********************************************************************
 * RefreshSigButton - make sure the sig button is right
 **********************************************************************/
void RefreshSigButton(MessHandle messH)
{
	ControlHandle cntl;

	if (cntl = FindControlByRefCon((*messH)->win,SIG_HIER_MENU))
	{
		SetBevelMenuValue(cntl,SigMenuItem(messH));
		SetBevelIcon(cntl,SigIconId(messH),0,0,nil);
	}
}

#endif

/************************************************************************
 * AttachMenu - let the user set the attachment types
 ************************************************************************/
void AttachMenu(MyWindowPtr win)
{
	MenuHandle amh = GetMenu(ATTACH_MENU);
	Rect pr;
	MessHandle messH = Win2MessH(win);
	long flags = SumOf(messH)->flags;
	long res;
	short item,oldItem;
	long aType;
	Point p;
	
	if (amh)
	{
		GetAttachRect(win,&pr);
		p.h = pr.left; p.v = pr.top;
		LocalToGlobal(&p);
		
		aType = AttachOptNumber(flags);
		SetItemMark(amh,aType+1,checkMark);
		oldItem = aType+1;
		
		InsertMenu(amh,-1);
		res = PopUpMenuSelect(amh,p.v,p.h,aType+1);
		item = res&0xffff0000 ? res&0xff : 0;
		DeleteMenu(ATTACH_MENU);
		
		if (item && item!=oldItem)
		{
			SetAOptNumber(SumOf(messH)->flags,item-1);
			TOCSetDirty((*messH)->tocH,true);
			InvalWindowRect(GetMyWindowWindowPtr(win),&pr);
			TOCSetDirty((*messH)->tocH,true);
		}
		ReleaseResource_(amh);
	}
}

/**********************************************************************
 * CompMenu - handle menu choice for composition window
 **********************************************************************/
Boolean CompMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(modifiers)
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;
	uLong when;
	short state;
	Boolean now = False;
	short tableId;
	Boolean swap = False;
	TextAddrHandle addr;

	if (menu==MESSAGE_MENU && item==MESSAGE_QUEUE_ITEM && modifiers&optionKey)
	{
		menu = CHANGE_HIER_MENU;
	  item = CHANGE_QUEUEING_ITEM;
	  swap = True;
	}
	switch (menu)
	{
		case FILE_MENU:
			switch (item)
			{
				case FILE_SAVE_ITEM:
					if (win->isDirty) (void) SaveComp(win);
					return(True);
				case FILE_SAVE_AS_ITEM:
					SaveMessageAs(messH);
					return(True);
				case FILE_PRINT_ITEM:
				case FILE_PRINT_ONE_ITEM:
					PrintOneMessage((*messH)->win,0!=(modifiers&shiftKey),item==FILE_PRINT_ONE_ITEM);
					return(True);
					break;
			}
			break;
		case EDIT_MENU:
			switch (item)
			{
				case EDIT_CLEAR_ITEM:
					if (CompHeadCurrent(TheBody)==ATTACH_HEAD && win->hasSelection)
					{
						CompDelAttachment(messH,nil);
						return(True);
					}
					break;
#ifdef SPEECH_ENABLED
				case EDIT_SPEAK_ITEM:
					if (!(modifiers & shiftKey)) {
						(void) SpeakMessage (nil, (*messH)->tocH, (*messH)->sumNum, speakTo | speakSubject | speakBody, false);
						return (true);
					}
					break;
#endif
			}
			return(False); /* handle elsewhere */
			break;
			
		case MESSAGE_MENU:
			switch (item)
			{
				case MESSAGE_DELETE_ITEM:
#ifdef TWO
					AddXfUndo(tocH,GetTrashTOC(),sumNum);
#endif
					DeleteMessage(tocH,sumNum,(modifiers&(optionKey|shiftKey))==(optionKey|shiftKey));
					if ((*tocH)->win)
						BoxSelectAfter((*tocH)->win,sumNum);
					return(True);
				case MESSAGE_QUEUE_ITEM:
				  when = 0;
					now = PrefIsSet(PREF_AUTO_SEND);
				queueit:
					QueueMessage(tocH,sumNum,now?kEuSendNow:kEuSendNext,when,false,menu==MESSAGE_MENU);
					return(True);
				case MESSAGE_FORWARD_ITEM:
					DoForwardMessage(win,0,True);
					return(True);
					break;
				case MESSAGE_ATTACH_ITEM:
					CompAttach(win,false);
					return(True);
					break;
				case MESSAGE_SALVAGE_ITEM:
					DoSalvageMessage(win,False);
					return(True);
					break;
			}
			break;

		case STATE_HIER_MENU:
#ifdef THREADING_ON
			if (SendThreadRunning && (*tocH)->sums[sumNum].state==BUSY_SENDING)
			{
				WarnUser (SENDING_WARNING, 0);
				return (True);
			}
#endif
			if (item==statmQueued) QueueMessage(tocH,sumNum,kEuSendNext,0,true,false);
			else SetState(tocH,sumNum,Item2Status(item));
			return(True);
			break;

		case PERS_HIER_MENU:
			if (HasFeature (featureMultiplePersonalities))
			{
				Str63 name;
				
				SetPers(tocH,sumNum,FindPersByName(MyGetItem(GetMHandle(menu),item,name)),True);
				return(True);
			}
			break;
		
		case CHANGE_HIER_MENU:
			switch(item)
			{
				case CHANGE_QUEUEING_ITEM:
				  when = (*tocH)->sums[sumNum].seconds;
				  state = (*tocH)->sums[sumNum].state;
				  if (state!=TIMED) when = 0;
					if (ModifyQueue(&state,&when,swap))
				  {
					  if (now = (state==SENT)) state=QUEUED;
					  if (IsQueuedState(state)) goto queueit;
						if (IsQueued(tocH,sumNum))
						  SetState(tocH,sumNum,SENDABLE);
						SetSendQueue();
					}
					return(True);
				default:
					return(False);
			}
			break;
		
		case TLATE_ATT_HIER_MENU:
			ETLAttach(item,win);
			break;
		
		case FORWARD_TO_HIER_MENU:
			DoForwardMessage(win,(addr=MenuItem2Handle(menu,item)),True);
			ZapHandle(addr);
			return(True);
			break;
		case TABLE_HIER_MENU:
			if (Menu2TableId(tocH,GetMHandle(TABLE_HIER_MENU),item,&tableId))
				SetMessTable(tocH,sumNum,tableId);
			return(True);
			break;
		case PRIOR_HIER_MENU:
			SetPriority(tocH,sumNum,NewPrior(item,(*tocH)->sums[sumNum].priority));
			return(True);
			break;
		case LABEL_HIER_MENU:
			item = Menu2Label(item);
			SetSumColor(tocH,sumNum,item);
			return(True);
			break;
		case SPECIAL_MENU:
			switch(AdjustSpecialMenuSelection(item))
			{
				case SPECIAL_MAKE_NICK_ITEM:
					if (modifiers&shiftKey)
						MakeNickFromSelection(win);
					else
#ifdef VCARD
						MakeCompNick(win,nil);
#else
						MakeCompNick(win);
#endif
					return(True);
					break;
				case SPECIAL_MAKEFILTER_ITEM:
					DoMakeFilter(win);
					return(True);
					break;
#ifdef TWO
				case SPECIAL_FILTER_ITEM:
					FilterMessage(flkManual,tocH,sumNum);
					if (FrontWindow_()!=GetMyWindowWindowPtr(win) && (*tocH)->count)
					{
						SelectBoxRange(tocH,sumNum-1,sumNum-1,False,-1,-1);
					}
					return(True);
					break;
#endif
			}
			break;
#ifdef DEBUG
		case DEBUG_MENU:
			if(item == dbTestSpooler)
			{
				SpoolMessage(messH, nil, 0);
				return(True);
			}
			break;
#endif
		default:
			return(TransferMenuChoice(menu,item,tocH,sumNum,modifiers,
							!SENT_OR_SENDING(SumOf(messH)->state) && CompHeadCurrent(TheBody)==BCC_HEAD));
			break;
	}
	return(False);
}

/************************************************************************
 * CompSendBtnUpdate - make sure name is correct on send button
 ************************************************************************/
void CompSendBtnUpdate(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	Str31 sbName,shdName;

	GetControlTitle((*messH)->sendButton,sbName);
	GetRString(shdName,(*messH)->hStationerySpec ? SAVE_ITEXT	//	"Save" for stationery
		: PrefIsSet(PREF_AUTO_SEND)?SEND_BUTTON:QUEUE_BUTTON);
	if (!StringSame(sbName,shdName)) SetControlTitle((*messH)->sendButton,shdName);
}

/************************************************************************
 * CompUpdateScore - update the TAE score
 ************************************************************************/
void CompUpdateScore(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	short score = (*PeteExtra(TheBody))->taeScore;
	Boolean scoreDiff = score!=SumOf(messH)->score;
	
	if (SENT_OR_SENDING(SumOf(messH)->state)) return;
	if ((*messH)->analControl && score && (scoreDiff || !IsControlVisible((*messH)->analControl)))
	{
		SetTAEScore((*messH)->tocH,(*messH)->sumNum,score);
		if (gBetterAppearance)
		{
			SetBevelIcon((*messH)->analControl,AnalIcon(score),nil,nil,nil);
			ShowControl((*messH)->analControl);
		}
		else
		{
			DisposeControl((*messH)->analControl);
			(*messH)->analControl = nil;
			AddAnalIcon(messH);
		}
	}
}


/* MJN *//* new routine */
/************************************************************************
 * DrawCompWindowBars - draw the icon bar and any other bars in a window
 ************************************************************************/
void DrawCompWindowBars(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	
	if ((*messH)->sound)
	{
		Str255 soundName;
		PlayNamedSound(GetRString(soundName,(*messH)->sound));
		(*messH)->sound = 0;
	}
}

/* MJN *//* new routine */
/*****************************************************************************
 * ForceCompWindowRecalcAndRedraw - forces the composition window to
 * recalculate positions for anything that was based on topMargin, and then
 * forces a full redraw.  Currently only used by the text formatting toolbar
 * when its visible status is changed.
 *****************************************************************************/
void ForceCompWindowRecalcAndRedraw(MyWindowPtr win)
{
	MessHandle messH;

	if (HasHelp&&HMIsBalloon()) HMRemoveBalloon(); /* OK to ignore errors from HMRemoveBalloon */
	messH = (MessHandle)GetMyWindowPrivateData(win);
	PeteDidResize(TheBody,&win->contR);
	// gotta reposition error messages
	PlaceMessErrNote(messH);
	InvalContent(win);
}

/**********************************************************************
 * SetAttachmentType - set the type of an attachment
 **********************************************************************/
void SetAttachmentType(TOCHandle tocH, short sumNum, short type)
{
	ControlHandle cntl;
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	
	SetAOptNumber((*tocH)->sums[sumNum].flags,type);
	TOCSetDirty(tocH,true);
	
	if (messH && (cntl=FindControlByRefCon((*messH)->win,ATTACH_MENU)))
	{
		SetBevelIcon(cntl,ATYPE_SICN_BASE+type,0,0,nil);
		SetBevelMenuValue(cntl,type+1);
		if ((*messH)->hStationerySpec) (*messH)->win->isDirty = True;
	}
}						

/************************************************************************
 * GetSigRect - where does the sig thing belong?
 ************************************************************************/
Boolean GetSigRect(MyWindowPtr win,Rect *pr)
{
	if (GetPriorityRect(win,pr))
	{
#ifdef TWO
		OffsetRect(pr,RectWi(*pr)+INSET,0);
#endif
		return(True);
	}
	return(False);
}

/************************************************************************
 * GetAttachRect - where does the attachment thing belong?
 ************************************************************************/
Boolean GetAttachRect(MyWindowPtr win,Rect *pr)
{
	if (GetSigRect(win,pr))
	{
		OffsetRect(pr,RectWi(*pr)+INSET,0);
		return(True);
	}
	return(False);
}

#ifdef VCARD
/************************************************************************
 * GetVCardRect - where does the vCard thing belong?
 ************************************************************************/
Boolean GetVCardRect (MyWindowPtr win, Rect *pr)
{
	if (GetAttachRect(win,pr))
	{
		OffsetRect(pr,RectWi(*pr)+INSET,0);
		return(True);
	}
	return(False);
}
#endif

/************************************************************************
 * GetAnalRect - where does the anal thing belong?
 ************************************************************************/
Boolean GetAnalRect(MyWindowPtr win,Rect *pr)
{
	if (GetSigRect(win,pr))
	{
		Rect	r;
		
		InsetRect(pr,(RectWi(*pr)-16)/2,(RectHi(*pr)-16)/2);
		GetControlBounds((*Win2MessH(win))->sendButton,&r);
		OffsetRect(pr,r.left-pr->right-INSET,0);
		return(True);
	}
	return(False);
}

#ifdef TWO
/************************************************************************
 * SigIconId - return signature icon id for message
 ************************************************************************/
short SigIconId(MessHandle messH)
{
	long sigId = SumOf(messH)->sigId;
	short sicn;
	
	if (sigId==SIG_NONE) sicn = NO_SIG_SICN;
	else sicn = sigId==0 ? SIG_SICN : (sigId==1?ALT_SIG_SICN:N_SIG_SICN);
	return(sicn);
}

/************************************************************************
 * SigMenuItem - return signature menu item for message
 ************************************************************************/
short SigMenuItem(MessHandle messH)
{
	long sigId = SumOf(messH)->sigId;
	short item = sigmNone;
	MenuHandle smh;
	ControlHandle cntl;
	Str31 string;
	
	if (sigId==SIG_NONE)
		item = sigmNone;
	else if (sigId==SIG_STANDARD)
		item = sigmStandard;
	else if (sigId==SIG_ALTERNATE)
		item = sigmAlternate;
	else if (cntl=FindControlByRefCon((*messH)->win,SIG_HIER_MENU))
	{
		smh = GetBevelMenu(cntl);
		for (item=CountMenuItems(smh);item>sigmAlternate;item--)
		{
			MyGetItem(smh,item,string);
			MyLowerStr(string);
			if (sigId==Hash(string)) break;
		}
		if (item==sigmAlternate) item=sigmNone;
	}
	return(item);
}
#endif

/************************************************************************
 * DrawPopIBox - draw a popup box with a SICN in it
 ************************************************************************/
void DrawPopIBox(Rect *r, short sicnId)
{
	Rect ir = *r;
	
	InsetRect(&ir,-2,-2);
	DrawShadowBox(&ir);

	ir = *r;
	ir.right = ir.left+16;
	PlotIconID(&ir,atAbsoluteCenter,ttNone,sicnId);
}

/************************************************************************
 * DrawShadowBox - draw a shadowed box
 ************************************************************************/
void DrawShadowBox(Rect *r)
{
	EraseRect(r);
	FrameRect(r);
	MoveTo(r->left+2,r->bottom);
	Line(r->right-r->left-2,0);
	Line(0,r->top-r->bottom+1);
}

/************************************************************************
 * PlotFlag - plot an icon and associated flag
 ************************************************************************/
void PlotFlag(Rect *r,Boolean on,short which)
{
	Rect r2;
	short fid, fs;
	
	if (on)
	{
		fid = GetPortTextFont(GetQDGlobalsThePort());
		fs = GetPortTextSize(GetQDGlobalsThePort());
		TextFont(systemFont); TextSize(0);
		MoveTo(r->left+4,r->bottom-6);
		DrawChar(checkMark);
		TextFont(fid); TextSize(fs);
	}
	
	SetRect(&r2,r->left+16,r->bottom-20,r->left+32,r->bottom-4);
	PlotIconID(&r2, atAbsoluteCenter, ttNone, which);
}

#ifdef NEVER
		aType = AttachOptNumber(flags);
		switch(aType)
		{
			case 0:	SetItemMark(pmh,pymHQX,checkMark); break;
			case 1:	SetItemMark(pmh,pymDouble,checkMark); break;
#ifdef TWO
			case 2:	SetItemMark(pmh,pymUU,checkMark); break;
#endif
			case 3:	SetItemMark(pmh,pymSingle,checkMark); break;
		}
			else if (pymDouble<=item && item<pymBar2)
			{
				switch(item)
				{
					case pymHQX: aType = 0; break;
					case pymDouble: aType = 1; break;
#ifdef TWO
					case pymUU: aType = 2; break;
#endif
					case pymSingle: aType = 3; break;
				}
				SumOf(messH)->flags &= ~(FLAG_ATYPE_LO|FLAG_ATYPE_HI);
				SumOf(messH)->flags |= aType<<6;
			}
#endif

/**********************************************************************
 * 
 **********************************************************************/
OSErr CompDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{
	OSErr err=noErr;
	Rect r;
	Point mouse;

	if (DragSource==win ||	// don't drag attachments from ourself
			SENT_OR_SENDING(SumOf(Win2MessH(win))->state) ||
			DragIsInteresting(drag,A822_FLAVOR,nil) || // let the address flavor be handled elsewhere
			!DragIsInteresting(drag,flavorTypeHFS,flavorTypePromiseHFS,kStationeryDragType,kPersonalityDragType,nil))	// no file, stationery, or personality; boring
		return(dragNotAcceptedErr);

	SetPort(GetMyWindowCGrafPtr(win));

	if (!(DragOrMods(drag)&cmdKey) && !PrefIsSet(PREF_SEND_ENRICHED_NEW) && DragIsImage(drag))
	{
		GetDragMouse(drag,&mouse,nil);
		GlobalToLocal(&mouse);
		if (PtInPETE(mouse,win->pte))
		{
			if (!PeteDrag(win,which,drag)) return(noErr);
		}
		else PeteDrag(nil,which,drag);
	}
	
	if (which==0xfff) HideDragHilite(drag);
		

	switch (which)
	{
		case kDragTrackingInWindow:
			r = win->contR;
			r.right -= GROW_SIZE;
			ShowDragRectHilite(drag,&r,True);
			break;

		case 0xfff:
			err=CompDrop(win,drag);
			break;
			
		case kDragTrackingLeaveWindow:
			HideDragHilite(drag);
			break;
	}
	
	return(err);

}

/**********************************************************************
 * CompDrop - handle a drop on a composition window
 **********************************************************************/
OSErr CompDrop(MyWindowPtr win,DragReference drag)
{
	short item;
	OSErr err=noErr;
	short n = MyCountDragItems(drag);
	HFSFlavor **data;
	PromiseHFSFlavor **promise;
	FSSpec spec;
	AEDesc dropLocation;
	short nAttach;
	MessHandle messH = Win2MessH(win);
	OSType type;
	
	nAttach = CountAttachments(messH);
	
	NullADList(&dropLocation,nil);
	
	for (item=1;item<=n && !err;item++)	// loop through each item
	{
		FSSpecHandle	hSpec;
		PersHandle		**hPers;

		if (!MyGetDragItemData(drag,item,kStationeryDragType,(void*)&hSpec))
		{
			//	Stationery drag
			RemoveInlineSig(messH);
			ApplyStationery(win,LDRef(hSpec),true,true);
			ZapHandle(hSpec);
		}
		else if (!MyGetDragItemData(drag,item,kPersonalityDragType,(void*)&hPers))
		{
			//	Personality drag
			MessHandle	messH = (MessHandle)GetMyWindowPrivateData(win);

			SetPers((*messH)->tocH,(*messH)->sumNum,**hPers,false);
			ZapHandle(hPers);
		}
		else if (err=MyGetDragItemData(drag,item,flavorTypeHFS,(void*)&data))
		{
			/*
			 * promiseHFS - tell the sender to put it in our spool folder
			 */
			if (!(err=MyGetDragItemData(drag,item,flavorTypePromiseHFS,(void*)&promise)))
			{
				//Dprintf("\p%o",(*promise)->promisedFlavor);
				// first, see if the "promised" file is really there already
				// we only do this if the promised flavor is one that we know is ok, ie
				// from an app that handles promiseHFS in the particular way we need
				if (TypeIsOnList((*promise)->promisedFlavor,PRE_PROMISE_OK))
					err = MyGetDragItemData(drag,item,(*promise)->promisedFlavor,(void*)&data);
				// ok, a real promise.  we only accept these from a few apps
				// that we know aren't going to do really bad things to us
				else if (TypeIsOnList((*promise)->promisedFlavor,PROMISE_OK))
				{
					AliasHandle alias=nil;
					// the data isn't already there.  Set the droplocation and try again
					// create the spool folder
					MakeAttSubFolder(Win2MessH(win),SumOf(Win2MessH(win))->uidHash,&spec);
					if (!(err=NewAlias(nil,&spec,&alias)))
					{
						//set the droplocation to it
						AECreateDesc(typeAlias,LDRef(alias),GetHandleSize_(alias),&dropLocation); ZapHandle(alias);
						SetDropLocation(drag,&dropLocation);
						
						// now find the file
						err = MyGetDragItemData(drag,item,(*promise)->promisedFlavor,(void*)&data);
						DisposeADList(&dropLocation,nil);
					}
				}
				else
					err = dragNotAcceptedErr;
				ZapHandle(promise);
				if (!err)
				{
					spec = **(FSSpecHandle)data;
					ZapHandle(data);
					
					// and attach it
					if (!(err = AttachDoc(win,&spec)))
						nAttach++;
				}
			}
			else
			{
				err=dragNotAcceptedErr;
			}
		}
		else
		{
			/*
			 * real HFS, just attach, unless...
			 */
			spec = (*data)->fileSpec;
			type = (*data)->fileType;
			ZapHandle(data);
			if (type=='fldr' ||
					type=='disk' || type=='srvr')
				return(dragNotAcceptedErr);
			err = AttachDoc(win,&spec);
			nAttach++;
			
			/*
			 * the drag came from one of our windows.  When this is the case
			 * and the source window was incoming, we'd better spool in case
			 * the user deletes message & attachment
			 */
			if (!err && DragSource && GetWindowKind (GetMyWindowWindowPtr (DragSource))==MESS_WIN)
				err=SpoolIndAttachment(messH,nAttach);
		}
	}

	RequestTFBEnableCheck(win); /* MJN *//* formatting toolbar */

	return(err);
}

/**********************************************************************
 * CompKey - react to a keystroke in a composition window
 **********************************************************************/
Boolean CompKey(MyWindowPtr win, EventRecord *event)
{
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	long uLetter = UnadornMessage(event)&charCodeMask;
	short c = (event->message & charCodeMask);
	Boolean hadSelection = win->hasSelection;
	HeadSpec hSpec;
	long offset;
	OSErr err;
	short cur = CompHeadCurrent(TheBody);
	Boolean tabSwitch = c==TABKEY &&
											(!win->pte || win->ro || cur!=0 ||
												(event->modifiers&shiftKey) ||
												!PrefIsSetOrNot(PREF_TAB_IN_TEXT,event->modifiers,optionKey));
	long undoStart, undoEnd;
	Boolean excerpt = false;
	Boolean crFunnies = c=='\015' && !cur && !(event->modifiers&(optionKey|shiftKey));
	Boolean atEnd, afterReturn;
	short n;

	if (tabSwitch) event->modifiers &= ~optionKey;
	
	if (leftArrowChar<=uLetter && uLetter<=downArrowChar &&
			IsArrowSwitch(event->modifiers))
	{
		NextMess(tocH,messH,uLetter,event->modifiers,False);
		return(True);
	}
	else if (!(event->modifiers & cmdKey) && tabSwitch)
		CompSwitchFields(messH,!(event->modifiers & shiftKey));
	else if (c=='\015' && (cur!=0) && !PrefIsSet(PREF_HEAD_RETURN))
		CompSwitchFields(messH,!(event->modifiers & shiftKey));
	else if (event->modifiers&cmdKey)
	{
		return(False);
	}
	else
	{
		normalKey:
		//if (c=='\015') event->modifiers |= shiftKey;
		if (cur==ATTACH_HEAD && !(hadSelection && (c==backSpace || c==clearKey || c==delChar) && !SENT_OR_SENDING(SumOf(messH)->state)))
			WarnUser(READ_ONLY,0);
		else
		{
			PeteGetTextAndSelection(TheBody,nil,&undoStart,&undoEnd);
			if (cur==0 && (undoStart!=undoEnd||c==backSpace)) PeteEnsureRangeBreak(TheBody,MAX(undoStart-4,0),undoEnd);
			if (undoEnd<PeteLen(TheBody)-1) undoEnd++; // this avoids weird editor bug
			if (crFunnies && (excerpt=PeteIsExcerptAt(TheBody,undoEnd))) PetePrepareUndo(TheBody,peUndoTyping,-1,undoEnd,&undoStart,nil);
			undoEnd = undoStart;
			if (undoEnd<PeteLen(TheBody)-1) undoEnd++; // this avoids weird editor bug
			if ((err=PeteEdit(win,win->pte,peeEvent,(void*)event))==errAENotModifiable
				&& PETESelectionLocked(PETE,win->pte,-1,-1))
			{
				if (cur==ATTACH_HEAD)
				{
					if (hadSelection && (c==backSpace || c==clearKey || c==delChar))
					{
						CompDelAttachment(messH,nil);
					}
					else WarnUser(READ_ONLY,0);
				}
				else WarnUser(READ_ONLY,0);
			}
			if (hadSelection && !win->hasSelection && !CompHeadCurrent(TheBody) && CompHeadFind(messH,CompHeadCurrent(TheBody),&hSpec))
				if (hSpec.stop==hSpec.value || c!=backSpace && hSpec.stop==hSpec.value+1)
					PetePlainPara(TheBody,PeteParaAt(TheBody,PETEGetTextLen(PETE,TheBody)));
			if (!err) undoEnd++;
			if (crFunnies && !err)
			{
				PETEParaInfo pinfo;
				long pIndex = PeteParaAt(TheBody,-1);
				Zero(pinfo);
				PETEGetParaInfo(PETE,TheBody,pIndex,&pinfo);
				if (pinfo.quoteLevel)
				{
					long paraBefore;
					PETEParaInfo beforeInfo;
					Boolean atReturn;
					UHandle text;
					
					PeteGetTextAndSelection(TheBody,nil,&offset,nil);
					atEnd = offset == PeteLen(TheBody)-1;
					paraBefore = PeteParaAt(TheBody,offset-2);
					Zero(beforeInfo);
					PETEGetParaInfo(PETE,TheBody,paraBefore,&beforeInfo);
					if (!beforeInfo.quoteLevel)
					{
						PetePlainParaAt(TheBody,offset-1,offset-1);
						PetePlain(TheBody,offset-1,offset,peAllValid);
					}
					else				
					{
#ifdef DEBUG
						if (!BUG14)
#endif
							PeteCalcOff(TheBody);
						PeteGetTextAndSelection(TheBody,&text,&offset,nil);
						
						/*
							 Ok, we might have started and ended six ways:
							 
							 	char | char				char newCR | char				
							 	char | return			char newCR | return			atReturn
							 	return | char			return newCR | char			afterReturn
							 	return | return		return newCR | return		atReturn && afterReturn
							 	char | end				char return |	end				atEnd
							 	return | end			return return |	end			atEnd && afterReturn
							
							 We want to end up in one of two ways:
							 
							 	return plainCR | plainCR plainCR
							 	return plainCR | end											atEnd
							 
						*/

						atReturn = offset < PeteLen(TheBody) && (*text)[offset]=='\015';
						afterReturn = (*text)[offset-2]=='\015';	// offset-1 is the cr we just put in
						
						// We need either two or four returns, and we have one already
						n = atEnd ? 1 : 3;	// We have one already.
						if (atReturn) n--; //already there
						if (afterReturn) n--;	//already there
						// insert them
						while (n-->0) {PeteInsertChar(TheBody,offset,CrLf[1],nil);undoEnd++;}
						
						// Ok, we always want two returns before the IP.  This
						// is automatically the case if afterReturn.  Otherwise, bump the IP
						if (!afterReturn) offset++;
						
						// Now we're in one of the two "normal" situations.  All that's left to do
						// Is make sure the proper paragraphs are plain.
						if (atEnd) MessPlainBytes(messH,0,afterReturn&&atReturn?-2:-1);
						else
						{
							PetePlainParaAt(TheBody,offset-1,offset+2);
							PetePlain(TheBody,offset-1,offset+2,peAllValid);
						}
						
						PeteSelect(win,win->pte,offset,offset);
						PeteCalcOn(TheBody);
					}
				}
			}
			if (crFunnies && excerpt) PeteFinishUndo(TheBody,peUndoTyping,undoStart,undoEnd);
		}
		if (cur==ATTACH_HEAD) AttachSelect(messH);
	}
	if (SENT_OR_SENDING(SumOf(messH)->state)) PeteLock(TheBody,kPETECurrentStyle,kPETECurrentStyle,peModLock);
	return(True);
}

/************************************************************************
 * CompDelAttachment - delete selected attachment
 ************************************************************************/
void CompDelAttachment(MessHandle messH,HSPtr hs)
{
	long sel;
	UHandle textH;
	HeadSpec hSpec;
	
	if (hs)
	{
		PeteDelete(TheBody,hs->start,hs->stop);
		sel = hs->start;
		textH = PeteText(TheBody);
	}
	else
	{
		PETEInsertTextPtr(PETE,TheBody,-1,nil,0,nil);
		PeteGetTextAndSelection(TheBody,&textH,&sel,nil);
	}
	CompHeadFind(messH,ATTACH_HEAD,&hSpec);
	if (sel>hSpec.value && (*textH)[sel-1]==' ' && (*textH)[sel-2]==' ')
	{
		PeteDelete(TheBody,sel-1,sel);
		sel--;
		hSpec.stop--;
	}
	if (sel<hSpec.stop && (*textH)[sel]==' ' && (*textH)[sel-1]==' ')
	{
		PeteDelete(TheBody,sel,sel+1);
		hSpec.stop--;
	}
	if (!hs && hSpec.stop==hSpec.value) CompSwitchFields(messH,True);
}


/************************************************************************
 * CompSwitchFields - switch fields in a composition window
 ************************************************************************/
void CompSwitchFields(MessHandle messH, Boolean forward)
{
	MyWindowPtr win = (*messH)->win;
	short current = CompHeadCurrent(TheBody);
	long next;
	long fDirty = PeteIsDirty(TheBody);
	HeadSpec hs;

	if (!current)
		if (forward) next = TO_HEAD;
		else next = BCC_HEAD;
	else if (forward) next = current+1;
	else next = current-1;
	
	if (next==ATTACH_HEAD || next==FROM_HEAD && !PrefIsSet(PREF_EDIT_FROM))
		forward ? next++ : next--;
	if (next>(*PeteExtra(TheBody))->headers) next = 0;

	if (CompHeadFind(messH,next,&hs) || CompHeadFind(messH,0,&hs))
		CompHeadActivate(TheBody,&hs);
	
	CompLeaving(messH,current);
}
/************************************************************************
 * AttachSelect - broaden the current selection to include the entire
 * text of an attachment
 ************************************************************************/
void AttachSelect(MessHandle messH)
{
	long selStart, selEnd;
	long mid;
	HeadSpec hs;
	OSErr err = fnfErr;
	
	if (PeteGetTextAndSelection(TheBody,nil,&selStart,&selEnd)) return;
	
	if (!CompHeadFind(messH,ATTACH_HEAD,&hs)) return;
	
	if (selEnd<hs.value || selStart>hs.stop) return;
	
	mid = (selStart+selEnd)/2;
	if (mid-2>hs.value) err = PETESelectGraphic(PETE,TheBody,mid-2);
	else if (mid+4<hs.stop) err = PETESelectGraphic(PETE,TheBody,mid+4);
	
	if (err) CompSwitchFields(messH,True);
}

/************************************************************************
 * AddrSelect - broaden the current selection to include entire addresses
 ************************************************************************/
void AddrSelect(MessHandle messH)
{
	UHandle theText;
	long selStart, selEnd;
	HeadSpec hs;
	BinAddrHandle addresses;
	long **spots=nil;
	short which;
	
	if (PeteGetTextAndSelection(TheBody,&theText,&selStart,&selEnd)) return;
	
	if (!CompHeadFind(messH,CompHeadCurrent(TheBody),&hs)) return;
	
	if (selEnd<hs.start || selStart>hs.stop) return;
	
	if (SuckPtrAddresses(&addresses,LDRef(theText)+hs.value,hs.stop-hs.value,False,False,False,&spots))
		return;
	UL(theText);
	ZapHandle(addresses);
	
	selStart -= hs.value;
	selEnd -= hs.value;
	which = HandleCount(spots);
	while (which-->1)
	{
		if ((*spots)[which-1]<selEnd && selEnd<(*spots)[which])
			selEnd = (*spots)[which];
		if ((*spots)[which-1]<selStart && selStart<(*spots)[which])
			selStart = (*spots)[which-1];
	}
	selStart += hs.value;
	selEnd += hs.value;
	PeteSelect((*messH)->win,TheBody,selStart,selEnd);
}

/************************************************************************
 * CompSetFormatBarIcon - set the format bar icon's value
 ************************************************************************/
void CompSetFormatBarIcon(MyWindowPtr win, Boolean visible)
{
	ControlHandle cntl = FindControlByRefCon(win,0xff000000);

	if (cntl) SetControlValue(cntl,visible ? 1 : 0);
}

/************************************************************************
 * CompButton - handle the clicking of a button in the comp window
 ************************************************************************/
void CompButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle messH=Win2MessH(win);
	long which;
	Str63 s;
	Boolean wasCompToolbar = false;
	
	if (buttonHandle==(*messH)->sendButton)
		{if (part==kControlButtonPart) CompMenu(win,MESSAGE_MENU,MESSAGE_QUEUE_ITEM,modifiers);}
	else if (GetControlReference(buttonHandle) == mcMesgErrors)
			return;
	else if (GetControlReference(buttonHandle) == PRIOR_HIER_MENU)
		DoMenu(winWP,PRIOR_HIER_MENU<<16|GetBevelMenuValue(buttonHandle),modifiers);
	else if (GetControlReference(buttonHandle) == SIG_HIER_MENU)
	{
		which = GetBevelMenuValue(buttonHandle);
		if (which > sigmAlternate)
		{
			MyGetItem(GetBevelMenu(buttonHandle),which,s);
			MyLowerStr(s);
			which = Hash(s);
		}
		else
			which -= 2;
		SetSig((*messH)->tocH,(*messH)->sumNum,which);
	}
	else if (GetControlReference(buttonHandle) == ATTACH_MENU)
	{
		which = GetBevelMenuValue(buttonHandle);
		SetAttachmentType((*messH)->tocH,(*messH)->sumNum,which-1);
	}
#ifdef VCARD
	else if (GetControlReference(buttonHandle) == 'vcrd')
	{
		FSSpec	spec;
		short		ab;

		ab = FindAddressBookType (personalAddressBook);
		if (ValidAddressBook (ab))
			if (!MakeVCardFile (&spec, ab, 0))
				CompAttachSpec (win, &spec);
	}
#endif
	else if (buttonHandle == (*messH)->analControl)
		return;
	else
	{
		which = GetControlReference(buttonHandle)&0xff;
		if (0<=which && which<ICON_BAR_NUM)
		{
			if (GetControlValue(buttonHandle))
			{
				if (fBits[which]<0)
					ClearMessOpt(messH,(-fBits[which]));
				else
					ClearMessFlag(messH,fBits[which]);
				SetControlValue(buttonHandle,0);
				if (fBits[which]==-OPT_COMP_TOOLBAR_VISIBLE)
				{
					SetMessOpt(messH,(-fBits[which])); // oops; put it back
					HideTxtFmtBar(win);
					if ((*messH)->hStationerySpec) win->isDirty = True;
				}
				else
						win->isDirty = true;
			}
			else
			{
				if (fBits[which]<0)
					SetMessOpt(messH,(-fBits[which]));
				else
					SetMessFlag(messH,fBits[which]);
				SetControlValue(buttonHandle,1);
				if (fBits[which]==-OPT_COMP_TOOLBAR_VISIBLE)
				{
					ClearMessOpt(messH,(-fBits[which])); // oops; put it back
					ShowTxtFmtBar(win);
					EnableTxtFmtBarIfOK(win);
					if ((*messH)->hStationerySpec) win->isDirty = True;
				}
				else
						win->isDirty = true;
			}
		}
/* MJN *//* formatting toolbar */
		else if (wasCompToolbar=TextFormattingBarButton(win,buttonHandle,modifiers,part,which));
#ifdef ETL
		//	Translator
		else ETLSelect(which-ICON_BAR_NUM,!GetControlValue(buttonHandle),messH);
#endif
	}
	
	// Audit any non-toolbar control hits
	if (!wasCompToolbar)	
		AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),(GetControlReference(buttonHandle)&0xff)), mouseDown);
}

/* MJN *//* new routine */
/************************************************************************
 * CompIdle - idle proc that gets called periodically
 ************************************************************************/
void CompIdle(MyWindowPtr win)
{
	TextFormattingBarIdle(win);
	CompSendBtnUpdate(win);
	if ((*Win2MessH(win))->redrawPersPopup)
		RedrawPersPopup(Win2MessH(win));
	CompUpdateScore(win);
}

/************************************************************************
 * ModifyQueue - change the queueing of a message
 ************************************************************************/
Boolean ModifyQueue(short *state,uLong *when,Boolean swap)
{
	DialogPtr md;
	MyWindowPtr	mdWin;
	short dItem,item;
	Boolean done;
	Str63 timeStr,dateStr;
	uLong secs = *when ? *when+ZoneSecs() : LocalDateTime();
	extern ModalFilterUPP DlgFilterUPP;

	if ((mdWin=GetNewMyDialog(MODQ_DLOG,nil,nil,(void*)InFront))==nil)
	{
		WarnUser(MEM_ERR,MemError());
		return(False);
	}

	md = GetMyWindowDialogPtr (mdWin);
	
	TimeString(secs,False,timeStr,nil);
	DateString(secs,shortDate,dateStr,nil);
	SetDIText(md,MQDL_TIME,timeStr);
	SetDIText(md,MQDL_DATE,dateStr);
	SelectDialogItemText(md,MQDL_TIME,0,REAL_BIG);

	mdWin->update = DlgUpdate;
	
	if (*when)
	  SetDItemState(md,MQDL_LATER,True);
	else if (swap)
	{
		if (PrefIsSet(PREF_AUTO_SEND))
		  SetDItemState(md,MQDL_QUEUE,True);
		else
			SetDItemState(md,MQDL_NOW,True);
	}
	else
	{
		if (*state==QUEUED)
			SetDItemState(md,MQDL_QUEUE,True);
		else
			SetDItemState(md,MQDL_UNQUEUE,True);
	}

	/*
	 * now, display the dialog
	 */
	StartMovableModal(md);
	ShowWindow(GetDialogWindow(md));
	HiliteButtonOne(md);
	PushCursor(arrowCursor);
	do
	{
		done=False;
		MovableModalDialog(md,DlgFilterUPP,&dItem);
		switch (dItem)
		{
			case MQDL_LATER:
				SelectDialogItemText(md,MQDL_TIME,0,REAL_BIG);
				/* fall-through */
			case MQDL_TIME:
			case MQDL_DATE:
			  dItem = MQDL_LATER;
				/* fall-through */
			default:
				for (item=MQDL_NOW;item<=MQDL_UNQUEUE;item++)
				  if (GetDItemState(md,item)!=(item==dItem))
					  SetDItemState(md,item,item==dItem);
				break;
			case MQDL_OK:
			case MQDL_CANCEL:
			case CANCEL_ITEM:
				done = True;
				break;
		}
		if (done && dItem==MQDL_OK && GetDItemState(md,MQDL_LATER))
		{
			GetDIText(md,MQDL_DATE,dateStr);
			GetDIText(md,MQDL_TIME,timeStr);
			if (!(*when=QueueDate2Secs(timeStr,dateStr))) done=False;
		}
	}
	while(!done);
	PopCursor();
	
	if (done = (dItem!=MQDL_CANCEL && dItem!=CANCEL_ITEM))
	{
	  if (GetDItemState(md,MQDL_UNQUEUE))
		  *state = SENDABLE;
		else
		  *state = GetDItemState(md,MQDL_NOW) ? SENT : QUEUED;
		if (!GetDItemState(md,MQDL_LATER)) *when = 0;
		else *state = TIMED;
	}
	EndMovableModal(md);
	DisposDialog_(md);

  if (done) SetSendQueue();
	return(done);
}

uLong QueueDate2Secs(UPtr timeStr,UPtr dateStr)
{
  uLong when;
	uLong now = LocalDateTime();
  long lenUsed;
	LongDateRec dateLDR,timeLDR;
	DateCacheRecord dc;
	DateTimeRec dtr;
	uShort dateRet=fatalDateTime;
	uShort timeRet=fatalDateTime;
	
	// Incremental forms
	if (dateStr[1]=='+')
	{
		StringToNum(dateStr,&when);
		return now + when * 60*60*24 - ZoneSecs();
	}
	else if (timeStr[1]=='+')
	{
		StringToNum(timeStr,&when);
		return now + when * 60 - ZoneSecs();
	}
	
	InitDateCache(&dc);
	WriteZero(&dateLDR,sizeof(dateLDR));
	WriteZero(&timeLDR,sizeof(timeLDR));
	timeRet = StringToTime(timeStr+1,*timeStr,&dc,&lenUsed,&timeLDR);
	dateRet = StringToDate(dateStr+1,*dateStr,&dc,&lenUsed,&dateLDR);
	if (dateRet > fatalDateTime && timeRet > fatalDateTime)
	{
	  WarnUser(DATE_ERROR,0);
		return(0);
	}
	SecondsToDate(LocalDateTime(),&dtr);
	if (timeRet < fatalDateTime)
	{
	  dtr.hour = timeLDR.od.oldDate.hour;
		dtr.minute = timeLDR.od.oldDate.minute;
		dtr.second = timeLDR.od.oldDate.second;
	}
	if (dateRet < fatalDateTime)
	{
	  dtr.year = dateLDR.od.oldDate.year;
		dtr.month = dateLDR.od.oldDate.month;
		dtr.day = dateLDR.od.oldDate.day;
		dtr.dayOfWeek = dateLDR.od.oldDate.dayOfWeek;
	}
	DateToSeconds(&dtr,&when);
	if (when < now && dateRet >= fatalDateTime && when + 24*3600 > now) when += 24*3600;
	if (when < now) {WarnUser(THE_PAST,0); return(0);}
	return(when-ZoneSecs());
}

/************************************************************************
 * CompIconRect - get the rect for a particular icon in the icon bar
 ************************************************************************/
Rect *CompIconRect(Rect *r,MessHandle messH,short index)
{
	CGrafPtr	messWinPort = GetMyWindowCGrafPtr ((*messH)->win);
	short h1,h2,width,advance;
	Rect tr;
	short n = ICON_BAR_NUM+(*messH)->nTransIcons;
	short bWidth;
	Rect rPort,rCntl;
	
	/*
	 * how big are the buttons?
	 */
	GetAttachRect((*messH)->win,&tr);
	*r = tr;
	bWidth = RectWi(tr);
	
	/*
	 * the left and right margins of the icon area
	 */
	h1 = tr.right + I_WIDTH;
	GetPortBounds(messWinPort,&rPort);
	GetControlBounds((*messH)->sendButton,&rCntl);
	h2 = rPort.right - RectWi(rCntl) - 4*INSET;
	
	/*
	 * how wide is the whole thing?
	 */
	width = h2 - h1;
	advance = bWidth+INSET;
	if (n*advance>width) advance = bWidth;

	/*
	 * center the button group
	 */
	h1 += (width-n*advance)/2;
	
	/*
	 * ok, now we can make an answer
	 */
	r->left = h1 + index*advance;
	r->right = r->left+bWidth;
	
  return(r);
}

/************************************************************************
 * SaveStationeryStuff - save the header line for stationery, and change filetype
 ************************************************************************/
OSErr SaveStationeryStuff(short refN,MessHandle messH)
{
	short err;
	FSSpec spec;
	FInfo info;
	MSumType sum,suminhex[2];
	Str31 scratch;
	long count;
	
	if (!HasFeature (featureStationery))
		return (noErr);
	
	UseFeature (featureStationery);
	
	/*
	 * first, set the filetype correctly
	 */
	if (err = GetFileByRef(refN,&spec)) goto done;
	if (err = FSpGetFInfo(&spec,&info)) goto done;
	info.fdType = STATIONERY_TYPE;
	info.fdCreator = CREATOR;
	if (err = FSpSetFInfo(&spec,&info)) goto done;

	/*
	 * now, write the important info
	 */
	/* header */
	if (err = FSWriteP(refN,GetRString(scratch,X_STUFF))) goto done;

	/* summary */
	sum = *SumOf(messH);
	count = 2*sizeof(MSumType);
	if (sum.sigId==SIG_NONE) sum.flags &= ~FLAG_OLD_SIG; else sum.flags |= FLAG_OLD_SIG;
	if (err = AWrite(refN,&count,Bytes2Hex((UPtr)&sum,sizeof(sum),(UPtr)suminhex))) goto done;
	
	/* and newline */
	err = FSWriteP(refN,Cr);
	
#ifdef ETL
	if (!err && (*messH)->hTranslators) err = WriteTranslators(refN,(*messH)->hTranslators);
#endif

done:
	if (err) FileSystemError(COULDNT_SAVEAS,"",err);
	else if (spec.vRefNum==StationVRef && spec.parID==StationDirId) BuildStationeryList();
	return(err);
}

#ifdef USECMM_BAD
/************************************************************************************************
CompBuildCMenu - build a list of context-specific items to be added to the contextual menu when
the context click occurs over a composition window.  specCMenuInfo does not point to a valid
handle upon calling the function. The handle-style array it points to needs to be allocated
herein.
************************************************************************************************/
OSStatus CompBuildCMenu(MyWindowPtr win, EventRecord* contextEvent, MenuHandle contextMenu)
{
	short				numItems;					//number of items to add put into the list
	static CMenuInfo	locCMInfo[] =				//items to put into the list
		{	MESSAGE_MENU,	MESSAGE_SALVAGE_ITEM,		//"send again" item
			MESSAGE_MENU,	MESSAGE_ATTACH_ITEM,		//"attach document" item
			MESSAGE_MENU,	MESSAGE_DELETE_ITEM,		//"delete" item
			CONTEXT_MENU,	CONTEXT_TERM_ITEM };		//item to mark end of array

	//count the number of items to add
	for( numItems=0; !(locCMInfo[numItems].srcMenuID == CONTEXT_MENU && locCMInfo[numItems].srcMenuItem == CONTEXT_TERM_ITEM); numItems++ )
		;
	
	//allocate memory for as handle-style array
	*specCMenuInfo = (CMenuInfoHndl)NewHandle( sizeof( CMenuInfo ) * ( numItems + 1 ) );
	if( !(*specCMenuInfo) ) return( MemError() );

	//copy data into the array
	BlockMove( locCMInfo, **specCMenuInfo, sizeof( CMenuInfo ) * ( numItems + 1 ) );
	
	return( noErr );
}
#endif

#endif


//
//	Useful for grabbing the To, CC and BCC addresses of a message.
//
//	Base on code in makefilter.c
//
extern OSErr GatherAddresses(MessHandle messH, HeaderEnum header, Handle *dest, Boolean wantComments);

OSErr GatherRecipientAddresses (MessHandle messH, Handle *dest, Boolean wantComments)

{
	OSErr	theError;
	short index,
				headers[] = {TO_HEAD, CC_HEAD, BCC_HEAD};

	theError = noErr;
	for (index = 0; !theError && index < sizeof (headers) / sizeof (short); ++index)
		theError = GatherAddresses (messH, headers[index], dest, wantComments);
	return (theError);
}

