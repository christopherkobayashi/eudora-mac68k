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

#include <Folders.h>

#include <conf.h>
#include <mydefs.h>

#include "messact.h"
#define FILE_NUM 24
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
#pragma segment Messact

OSErr MakeSubjectTXE(MessHandle messH);
void MessUpdate(MyWindowPtr win);
void MessSubjRect(MyWindowPtr win, Rect * r);
Boolean MessScroll(MyWindowPtr win, int h, int v);
void MessHelp(MyWindowPtr win, Point mouse);
short MyFSWrite(short refN, long *bytes, UPtr buffer);
Boolean IsAttLine(PStr line, Boolean wantToOpen);
OSErr OpenAttLine(PETEHandle pte, PStr line, Boolean finderSelect,
		  Boolean printIt);
Boolean GetBlahRect(MyWindowPtr win, short which, Rect * r);
Boolean TogglePOPD(OSType theType, short id, MessHandle messH);
void SADL_Hilite(DialogPtr dgPtr);
void MessButton(MyWindowPtr win, ControlHandle button, long modifiers,
		short part);
void MessTxChanged(MyWindowPtr win, TEHandle teh, short oldNl, short newNl,
		   Boolean scroll);
#define I_WIDTH 29
OSErr KillTransAttachments(FSSpecHandle files);
Boolean GetTruckRect(MyWindowPtr win, Rect * c);
void AddNotifyControls(MessHandle messH);
void PlaceNotifyControls(MyWindowPtr win);
static short SaveAsToOpenFileLo(short refN, MessHandle messH);
void MessProxy(MyWindowPtr win, EventRecord * event);
OSErr LopPoundHexDigits(PStr fileName);
#ifdef USECMM_BAD
OSErr MessBuildCMenu(MyWindowPtr win, EventRecord * contextEvent,
		     CMenuInfoHndl * specCMenuInfo);
#endif

/**********************************************************************
 * MessClose - close a message window
 **********************************************************************/
Boolean MessClose(MyWindowPtr win)
{
	MessType **messH = (MessType **) GetMyWindowPrivateData(win);
	TOCType **tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;

	if (!GrowZoning && TheBody && (*messH)->subPTE
	    && win->pte == (*messH)->subPTE)
		MessFocus(messH, TheBody);
	if (PeteIsDirty((*messH)->subPTE))
		MessSaveSub(messH);
	if (!GrowZoning)
		win->isDirty = win->isDirty || PeteIsDirtyList(TheBody);

	if (!NoSaves && !GrowZoning)
		if (!SaveMessHi(win, True))
			return (False);

	LL_Remove(MessList, messH, (MessType **));
	AccuZap((*messH)->extras);
	AccuZap((*messH)->aSourceMID);
	if ((*messH)->etlFiles)
		KillTransAttachments((*messH)->etlFiles);
	ZapHandle((*messH)->etlFiles);
	SetMyWindowPrivateData(win, nil);
	AccuZap((*messH)->newsGroupAcc);
	ZapHandle(messH);
	(*tocH)->sums[sumNum].messH = nil;

#ifdef	IMAP
	// Cancel the IMAP download if this is an imap message ...
	if ((*tocH)->imapTOC)
		IMAPAbortMessageFetch(tocH, sumNum);
#endif

	return (True);
}


/**********************************************************************
 * SaveMessHi - save a message, if the user wants and if its dirty
 **********************************************************************/
Boolean SaveMessHi(MyWindowPtr win, Boolean closing)
{
	short which;

	if (win->isDirty) {
		which = WannaSave(win);
		if (which == CANCEL_ITEM)
			return (False);
		else if (which == WANNA_SAVE_SAVE) {
			if (!SaveMess(win))
				return (False);
		} else if (which == WANNA_SAVE_DISCARD) {
			if (!closing)
				ReopenMessage(win);
		}
	}
	return (True);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr KillTransAttachments(FSSpecHandle files)
{
	FSSpec spec;
	OSErr err, anyErr = noErr;
	short n;

	for (n = HandleCount(files); n; n--) {
		spec = (*files)[n - 1];
		err = WipeSpec(&spec);
		if (!anyErr)
			anyErr = err;
	}
	return (anyErr);
}

/**********************************************************************
 * NewPrior - compute a new priority from a menu and an old priorty
 **********************************************************************/
short NewPrior(short item, short prior)
{
	switch (item) {
	case pymRaise:
		prior = Prior2Display(prior);
		prior = MAX(pymHighest, prior - 1);
		prior = Display2Prior(prior);
		break;
	case pymLower:
		prior = Prior2Display(prior);
		prior = MIN(pymLowest, prior + 1);
		prior = Display2Prior(prior);
		break;
	default:
		prior = Display2Prior(item);
		break;
	}
	return (prior);
}

/************************************************************************
 * TransferMenuChoice - handle a menu choice from a transfer menu
 ************************************************************************/
Boolean TransferMenuChoice(short menu, short item, TOCHandle tocH,
			   short sumNum, long modifiers, Boolean fcc)
{
	FSSpec spec, toSpec, resolvToSpec;
	short function = REAL_BIG;

	if (!IsMailboxChoice(menu, item))
		return (False);

	if (menu == TRANSFER_MENU)
		function = TRANSFER;
	else if (IsMailboxSubmenu(menu))
		function =
		    (menu -
		     (g16bitSubMenuIDs ? BOX_MENU_START : 1)) /
		    MAX_BOX_LEVELS;
	if (function == TRANSFER) {
		spec = GetMailboxSpec(tocH, sumNum);
		if (GetTransferParams(menu, item, &toSpec, nil)
		    && !SameSpec(&spec, &toSpec))
			if (!ResolveAliasOrElse
			    (&toSpec, &resolvToSpec, nil)
			    && !SameSpec(&spec, &resolvToSpec)) {
				//Folder Carbon Copy - no FCC in Light!
				if (HasFeature(featureFcc) && fcc)
					Fcc((*tocH)->sums[sumNum].messH,
					    &toSpec);
				else if (sumNum >= 0) {
#ifdef TWO
					if (!(modifiers & optionKey)) {
#ifdef	IMAP
						if (!(*tocH)->imapTOC)
#endif
							AddXfUndo(tocH,
								  TOCBySpec
								  (&toSpec),
								  sumNum);
						EzOpen(tocH, sumNum, 0,
						       modifiers, True,
						       True);
					}
#endif
					MoveMessage(tocH, sumNum, &toSpec,
						    (modifiers & optionKey)
						    != 0);
				} else
					MoveSelectedMessages(tocH, &toSpec,
							     (modifiers &
							      optionKey) !=
							     0);
			}
		return (True);
	}
	return (False);
}


/**********************************************************************
 * TogglePOPD - put a message on or off a list, 
 **********************************************************************/
Boolean TogglePOPD(OSType theType, short id, MessHandle messH)
{
	Boolean was;

	if (was = IdIsOnPOPD(theType, id, SumOf(messH)->uidHash)) {
		RemIdFromPOPD(theType, id, SumOf(messH)->uidHash);
	} else {
		AddIdToPOPD(theType, id, SumOf(messH)->uidHash, False);
	}
	return (!was);
}


/************************************************************************
 * PetePaneDraw - draw a pete record in a user pane
 ************************************************************************/
pascal void PetePaneDraw(ControlHandle cntl, SInt16 part)
{
#pragma unused(part)
	PETEHandle pte = (PETEHandle) GetControlReference(cntl);

	PeteUpdate(pte);
}

/************************************************************************
 * MakeSubjectTXE - make a TERec for the subject
 ************************************************************************/
OSErr MakeSubjectTXE(MessHandle messH)
{
	Str63 subject;
	Rect view;
	MyWindowPtr win = (*messH)->win;
	GrafPtr oldPort;
	OSErr err;
	PETEDocInitInfo pi;
	PETEHandle pte;
	ControlHandle cntl;
	long offset = 0;
	DECLARE_UPP(PetePaneDraw, ControlUserPaneDraw);

	GetPort(&oldPort);
	SetPort_(GetMyWindowCGrafPtr(win));

	PSCopy(subject, SumOf(messH)->subj);

	MessSubjRect(win, &view);
	OffsetRect(&view, 0, 1);

	DefaultPII(win, False, 0, &pi);
	pi.inRect = view;
	pi.docWidth = MessWi(win) - 9;

	if (!(err = PeteCreate(win, &pte, peClearAllReturns, &pi))) {
		(*PeteExtra(pte))->frame = True;
		if (!MessFlagIsSet(messH, FLAG_UTF8) ||
		    !HasUnicode() ||
		    EncodingError(err =
				  PeteInsertIntlText(pte, &offset,
						     (Handle) (subject +
							       1), -1,
						     *subject, nil,
						     UTF8_ENCODING, false,
						     true))) {
			err =
			    PETEInsertTextPtr(PETE, pte, 0, subject + 1,
					      *subject, nil);
		}
		if (MessFlagIsSet(messH, FLAG_UTF8) && !HasUnicode()) {
			PeteLock(pte, 0, PeteLen(pte), peModLock);
		}
		PeteFontAndSize(pte, FontID, FontSize);
		PETEMarkDocDirty(PETE, pte, False);
		(*messH)->subPTE = pte;

		if (cntl =
		    NewControl(GetMyWindowWindowPtr(win), &view, "", True,
			       0, 0, 0, kControlUserPaneProc,
			       (uLong) pte)) {
			INIT_UPP(PetePaneDraw, ControlUserPaneDraw);
			SetControlData(cntl, 0,
				       kControlUserPaneDrawProcTag,
				       sizeof(PetePaneDrawUPP),
				       (void *) &PetePaneDrawUPP);
			EmbedControl(cntl, win->topMarginCntl);
		}
	}
	CleanPII(&pi);
	if (err)
		WarnUser(PETE_ERR, err);

	SetPort_(oldPort);
	return (!err);
}


/************************************************************************
 * AddXlateTables - add named translate tables to the current menu
 ************************************************************************/
short AddXlateTables(Boolean isOut, short nowId, Boolean ph,
		     MenuHandle pmh)
{
	Boolean added = CountMenuItems(pmh) == 0;
	short i, n;
	Handle h;
	Str31 name;
	ResType type;
	short id;
	short defltId =
	    ph ? 0 : GetPrefLong((isOut ? PREF_OUT_XLATE : PREF_IN_XLATE));
	short item = 0;

	n = CountResources('taBL');
	SetResLoad(False);
	for (i = 1; i <= n; i++)
		if (h = GetIndResource_('taBL', i)) {
			GetResInfo(h, &id, &type, name);
			ReleaseResource_(h);
			if (!ResError() && *name
			    && ((id % 2) == 0) == isOut) {
				if (!added) {
					added = True;
					AppendMenu(pmh, "\p-");
				}
				MyAppendMenu(pmh, name);
				if (id == defltId)
					SetItemStyle(pmh,
						     CountMenuItems(pmh),
						     outline);
				if (id == nowId
				    || (id == defltId
					&& nowId == DEFAULT_TABLE)) {
					item = CountMenuItems(pmh);
					SetItemMark(pmh,
						    CountMenuItems(pmh),
						    checkMark);
				}
			}
		}
	SetResLoad(True);
	return (item);
}

/************************************************************************
 * Menu2TableId - what table id does a menu choice represent?
 ************************************************************************/
Boolean Menu2TableId(TOCHandle tocH, MenuHandle pmh, short item,
		     short *tableId)
{
	short newId;
	short mark = 0;
	ResType type;
	Str31 name;
	Handle h;
	Boolean isOut = (*tocH)->which == OUT;
	Boolean makeDefault = 0 != (CurrentModifiers() & shiftKey);

	if (makeDefault)
		GetItemStyle(pmh, item, (Style *) & mark);
	else
		GetItemMark(pmh, item, &mark);
	if (mark)
		newId = NO_TABLE;
	else {
		MyGetItem(pmh, item, name);
		SetResLoad(False);
		if (h = GetNamedResource('taBL', name)) {
			GetResInfo(h, &newId, &type, name);
			if (ResError())
				newId = *tableId;	/* do nothing */
			ReleaseResource_(h);
		}
		SetResLoad(True);
	}
	if (makeDefault) {
		NumToString(newId, name);
		SetPref((isOut ? PREF_OUT_XLATE : PREF_IN_XLATE), name);
		return (False);
	} else {
		*tableId = newId;
		return (True);
	}
}

/************************************************************************
 * SetSubject - set the subject in a message summary
 ************************************************************************/
void SetSubject(TOCHandle tocH, short sumNum, PStr sub)
{
	Str63 oldSubj;
	Str255 title;
	MessHandle messH = (*tocH)->sums[sumNum].messH;

	PSCopy(oldSubj, (*tocH)->sums[sumNum].subj);
	if (!EqualString(oldSubj, sub, True, True)) {
		PSCopy((*tocH)->sums[sumNum].subj, sub);
		InvalSum(tocH, sumNum);
#ifdef NEVER
		CalcSumLengths(tocH, sumNum);
#endif
		TOCSetDirty(tocH, true);
		if (messH) {
			MakeMessTitle(title, tocH, sumNum, True);
			SetWTitle_(GetMyWindowWindowPtr((*messH)->win),
				   title);
			if ((*messH)->subPTE) {
				if (MessFlagIsSet(messH, FLAG_UTF8)
				    && HasUnicode())
					PeteSetIntlText((*messH)->subPTE,
							sub + 1, -1, *sub,
							nil,
							UTF8_ENCODING);
				else
					PeteSetTextPtr((*messH)->subPTE,
						       sub + 1, *sub);
			}
		}
		SearchUpdateSum(tocH, sumNum, tocH, (*tocH)->sums[sumNum].serialNum, false, false);	//      Notify search window
	}
}

/************************************************************************
 * SetSender - set the sender in a message summary
 ************************************************************************/
void SetSender(TOCHandle tocH, short sumNum, PStr sender)
{
	Str63 oldSender;
	Str255 title;
	MessHandle messH = (*tocH)->sums[sumNum].messH;

	PCopy(oldSender, (*tocH)->sums[sumNum].from);
	if (!EqualString(oldSender, sender, True, True)) {
		PCopy((*tocH)->sums[sumNum].from, sender);
		InvalSum(tocH, sumNum);
#ifdef NEVER
		CalcSumLengths(tocH, sumNum);
#endif
		TOCSetDirty(tocH, true);
		if (messH) {
			MakeMessTitle(title, tocH, sumNum, True);
			SetWTitle_(GetMyWindowWindowPtr((*messH)->win),
				   title);
		}
	}
}

/************************************************************************
 * SetFlag - set one of the message flags
 ************************************************************************/
void SetFlag(TOCHandle tocH, short sumNum, long flag, Boolean on)
{
	if (on)
		(*tocH)->sums[sumNum].flags |= flag;
	else
		(*tocH)->sums[sumNum].flags &= ~flag;
	if ((*tocH)->sums[sumNum].messH)
		InvalTopMargin((*(*tocH)->sums[sumNum].messH)->win);
	TOCSetDirty(tocH, true);
}

/************************************************************************
 * SetOpt - set one of the message options
 ************************************************************************/
void SetOpt(TOCHandle tocH, short sumNum, long flag, Boolean on)
{
	if (on)
		(*tocH)->sums[sumNum].opts |= flag;
	else
		(*tocH)->sums[sumNum].opts &= ~flag;
	TOCSetDirty(tocH, true);
}


#ifdef TWO

/************************************************************************
 * OpenSelectedAtt - open selected attachments (or tell if attachments are selected)
 ************************************************************************/
Boolean AttIsSelected(MyWindowPtr win, PETEHandle pte, long startWith,
		      long endWith, short what, long *aStart, long *aEnd)
{
	Handle textH;
	UPtr begin, end;	/* beginning and end of all text */
	UPtr sBegin, sEnd;	/* beginning and end of selected bit */
	UPtr lBegin, lEnd;	/* beginning and end of current line */
	Str255 line;
	long selStart, selEnd;
	Byte state;
	Boolean found = false;
	FSSpec spec;
	Boolean colorThem = 0 != (what & attColor);
	Boolean selectThem = 0 != (what & attSelect);
	Boolean openThem = 0 != (what & attOpen);
	Boolean finderSelect = 0 != (what & attFinder);
	Boolean printThem = 0 != (what & attPrint);

	if (!PeteIsValid(pte) && win)
		pte = win->pte;
	if (!PeteIsValid(pte))
		return (False);
	if (PeteIsValid(pte) && !win)
		win = (*PeteExtra(pte))->win;

	if (PeteGetTextAndSelection(pte, &textH, &selStart, &selEnd))
		return (False);
	if (!textH || !GetHandleSize(textH))
		return (False);
	state = HGetState(textH);

	/*
	 * set initial values
	 */
	begin = LDRef(textH);
	end = begin + GetHandleSize(textH);
	sBegin = begin + ((startWith < 0) ? selStart : startWith);
	sEnd = begin + ((endWith < 0) ? selEnd : endWith);
	sBegin = MAX(sBegin, begin);
	if (sBegin >= end)
		goto done;
	sEnd = MIN(end, sEnd);
	if (sEnd <= begin)
		goto done;

	/*
	 * expand "selection" to whole lines
	 */
	while (sBegin > begin && sBegin[-1] != '\015')
		sBegin--;
	if (selStart != selEnd && sEnd[-1] == '\015')
		sEnd--;
	else
		while (sEnd < end && sEnd[0] != '\015')
			sEnd++;

	/*
	 * examine each line
	 */
	for (lBegin = sBegin; lBegin < sEnd; lBegin = lEnd + 1) {
		for (lEnd = lBegin; lEnd < sEnd && lEnd[0] != '\015';
		     lEnd++);
		MakePStr(line, lBegin, lEnd - lBegin);
		if (!PeteIsExcerptAt(pte, lBegin - begin + 1)
		    && !AttLine2Spec(line, &spec, openThem)) {
			found = True;
			if (colorThem) {
#ifdef ATT_ICONS
				MyWindowPtr pteWin =
				    (*PeteExtra(pte))->win;
				WindowPtr pteWinWP =
				    GetMyWindowWindowPtr(pteWin);
				TOCHandle tocH;

				if (GetWindowKind(pteWinWP) == MESS_WIN ||
				    (GetWindowKind(pteWinWP) == MBOX_WIN &&
				     //      Don't display attachment icons in search windows unless it's in the preview pane
				     (!IsSearchWindow(pteWinWP)
				      ||
				      ((tocH =
					(TOCHandle)
					GetMyWindowPrivateData(pteWin))
				       && (*tocH)->previewPTE == pte))))
					PeteFileGraphicRange(pte,
							     lBegin -
							     begin,
							     lEnd - begin,
							     &spec,
							     fgAttachment +
							     fgDontCopyToClip
							     +
							     (PrefIsSet
							      (PREF_NO_INLINE)
							      ? 0 :
							      fgDisplayInline));
				else
#endif
					URLStyle(pte, lBegin - begin,
						 lEnd - begin, False);
			}
			if (selectThem) {
				PeteSelect(nil, pte, lBegin - begin,
					   lEnd - begin);
				PeteSetDirty(pte);
			}
			if (openThem || printThem)
				OpenAttLine(pte, line, finderSelect,
					    printThem);
			if (aStart)
				*aStart = lBegin - begin;
			if (aEnd)
				*aEnd = lEnd - begin;
		}
	}
      done:
	HSetState(textH, state);
	return (found);
}

/************************************************************************
 * IsAttLine - is a line an attachment line?
 ************************************************************************/
Boolean IsAttLine(PStr line, Boolean wantToOpen)
{
	return (!AttLine2Spec(line, nil, wantToOpen));
}

/************************************************************************
 * OpenAttLine - open an attachment described by a line
 ************************************************************************/
OSErr OpenAttLine(PETEHandle pte, PStr line, Boolean finderSelect,
		  Boolean printIt)
{
	short err = noErr;
	FSSpec spec;

	if (!(err = AttLine2Spec(line, &spec, True))) {
#ifdef	IMAP
		// If this is an IMAP attachment, download it.
		if (IsIMAPAttachmentStub(&spec)) {
			// fetch the attachment, wait for it to be downloaded.  
			// spec will point to the downloaded attachment.
			err = FetchIMAPAttachment(pte, &spec, true);
		}
#endif
		if (err == noErr)
			err =
			    OpenOtherDoc(&spec, finderSelect, printIt,
					 nil);
	}

	return (err);
}

/************************************************************************
 * AttLine2Spec - get an FSSpec from an attachment line
 *	Heuristic
 *	 - must end in 8 hex digits in []'s
 *	 - must have at least two colons
 *	 - text between first two colons must be volume name
 ************************************************************************/
OSErr AttLine2Spec(PStr line, FSSpecPtr spec, Boolean wantToOpen)
{
	Str31 colostomy;	/* bad joke, I know */
	Str63 fName;
	PStr colon1, colon2;
	UPtr spot;
	long fid = 0;
	short dig = 8;
	short err;
	Boolean isFolder = nil != PFindSub("\p (fldr/macs) (", line);	//crude check for our folder creator

	TrimWhite(line);

	if (!IsWordChar[line[1]])
		return fnfErr;	// don't do quoted attachment lines

	/*
	 * pick out the fid
	 */
	if (line[*line] != ']' && line[*line] != ')')
		return (fnfErr);
	if (*line < 10 || line[*line - 9] != '[' && line[*line - 9] != '(')
		return (fnfErr);
	for (colon1 = line + *line - 8; dig--; colon1++)
		if ('0' <= *colon1 && *colon1 <= '9')
			fid = (fid << 4) | (*colon1 - '0');
		else if ('a' <= *colon1 && *colon1 <= 'f')
			fid = (fid << 4) | (10 + *colon1 - 'a');
		else if ('A' <= *colon1 && *colon1 <= 'F')
			fid = (fid << 4) | (10 + *colon1 - 'A');
		else
			return (fnfErr);

	/*
	 * pick out the volume name
	 */
	colon1 = strchr(line + 1, ':');
	if (!colon1)
		return (fnfErr);
	colon2 = strchr(colon1 + 2, ':');
	if (!colon2)
		return (fnfErr);

	if (spec) {
		MakePStr(colostomy, colon1 + 2, colon2 - colon1 - 1);
		MakePStr(fName, colon2 + 1, *line - (colon2 - line) - 23);
		if (*fName < 1 || *fName > 31)
			err = fnfErr;
		else {
			if (isFolder) {
				spec->vRefNum = GetMyVR(colostomy);
				spec->parID = fid;
				err = ParentSpec(spec, spec);
				if (!err)
					err =
					    GetDirName(nil, spec->vRefNum,
						       fid, spec->name);
			} else
				err =
				    FSResolveFID(GetMyVR(colostomy), fid,
						 spec);

			if (err
			    || !(spot =
				 PPtrFindSub(fName, spec->name + 1,
					     *spec->name))
			    || spot != spec->name + 1) {
#ifdef	IMAP
				//
				// The names of IMAP attachments could get macified once downloaded.
				// if the name the fileID resolved to is a Mac2OtherName()ed fName, then call it a match.
				//

				Str31 otherName;

				// check only if FSResolveFID succeeded.
				if (err == noErr)
					Mac2OtherName(otherName,
						      spec->name);
				else
					otherName[0] = 0;

				// If fName and the Mac2OtherName match, we've found the file.  
				if ((otherName[0] == 0)
				    || !(spot =
					 PPtrFindSub(fName, otherName + 1,
						     *otherName))
				    || spot != otherName + 1) {
					// otherwise, call it a match only if a file with the same name exists in the attachments folder.
#endif
					GetAttFolderSpec(spec);
					PCopy(spec->name, fName);
					err = FSpExists(spec);

					if (err && IsIMAPPers(CurPers))
						if (!
						    (err =
						     GetIMAPAttachFolder
						     (spec))) {
							PCopy(spec->name,
							      fName);
							err =
							    FSpExists
							    (spec);
						}
					// the increasing prevalence of long filenames means we increasingly
					// cannot find attachments after a disk move.  So, if this is a mangled
					// name, look for the non-hex part as a prefix of a filename in the attachments folder
					if (err
					    &&
					    PrefIsSet
					    (PREF_PROMISCUOUS_REATTACH)) {
						Str31 stubName;
						Str31 stubExtension;

						// split into fershlugginer extension
						SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote
						    (fName, stubName,
						     stubExtension, 31);
						if (!LopPoundHexDigits
						    (stubName)) {

							// Ok, now we have determined that the filename is of the form:
							// stubName # hex-digits stubExtension
							CInfoPBRec hfi;
							Str31
							    foundFileName;
							Str31
							    possibleFileName;
							Boolean
							    lastOneMatched
							    = false;

							Zero(hfi);
							hfi.hFileInfo.
							    ioNamePtr =
							    possibleFileName;

							while (!DirIterate
							       (spec->
								vRefNum,
								spec->
								parID,
								&hfi)) {
								Str31
								    possibleStub,
								    possibleExtension;

								SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote
								    (possibleFileName,
								     possibleStub,
								     possibleExtension,
								     31);
								if (!(err = LopPoundHexDigits(possibleStub)))	// a shortened filename
									if (StringSame(possibleExtension, stubExtension))
										if (!striscmp(possibleStub + 1, stubName + 1) && *possibleStub > 10) {
											// we have a match!
											if (lastOneMatched) {
												// Sadness.  We match more than one file.  
												err = dupFNErr;
												break;
											} else {
												PCopy
												    (foundFileName,
												     possibleFileName);
												lastOneMatched
												    =
												    true;
												continue;
											}
										}
								// we only get here if the current possible file does NOT match.
								// if that's so, and the *last* one matched, then there is one and only
								// one match, and we shout Bingo!, losing our dentures in the process
								if (lastOneMatched) {
									PCopy
									    (spec->
									     name,
									     foundFileName);
									err = noErr;	// hurray!
									break;
								}
							}
						}
					}
				}
#ifdef	IMAP
			}
#endif
		}
		if (wantToOpen && err)
			WarnUser(ATTACH_GONE, err);
		return (err);
	} else
		return (colon2 - colon1 > 1
			&& colon2 - colon1 < 33 ? noErr : fnfErr);
}

/************************************************************************
 * LopPoundHexDigits - strip the # and hex digits from a shortened filename
 * returns fnfErr if no # or followed by non-hex digits
 * returns dupFNErr if remaining filename is very short.
 * null-terminates the string for convenience' sake
 ************************************************************************/
OSErr LopPoundHexDigits(PStr fileName)
{
	Str31 hexDigits;
	PStr poundSign = PIndex(fileName, '#');
	short i;

	if (!poundSign)
		return fnfErr;
	MakePStr(hexDigits, poundSign + 1,
		 *fileName - (poundSign - fileName + 2));
	if (poundSign - fileName < 10)
		return dupFNErr;
	for (i = 1; i < *hexDigits; i++)
		if (!IsHexDig(hexDigits[i]))
			return fnfErr;
	*fileName = poundSign - fileName - 1;
	PTerminate(fileName);
	return noErr;
}

/************************************************************************
 * RelLine2Spec - get an FSSpec from a "related" line
 *	 - begin with "related:"
 *	 - must have at least two colons
 *	 - text between first two colons must be volume name
 ************************************************************************/
OSErr RelLine2Spec(PStr line, FSSpecPtr spec, uLong * cid, uLong * relURL,
		   uLong * absURL)
{
	Str255 token;
	Str31 name;
	UPtr spot = line + 1;
	Byte colon[2];
	short vRef;
	uLong fid;
	OSErr err;

	colon[0] = ':';
	colon[1] = 0;

	// related
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	if (!EqualStrRes(token, MIME_RELATED))
		return (fnfErr);

	// space
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);

	// volume name
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	vRef = GetMyVR(token);
	if (vRef == REAL_BIG)
		return (fnfErr);

	// filename
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	PSCopy(name, token);
	FixURLString(name);

	// fid
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	if (*token != 8)
		return (fnfErr);
	Hex2Bytes(token + 1, 8, (void *) &fid);

	// cid
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	if (*token != 8)
		return (fnfErr);
	if (cid)
		Hex2Bytes(token + 1, 8, (void *) cid);

	// rel
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	if (*token != 8)
		return (fnfErr);
	if (relURL)
		Hex2Bytes(token + 1, 8, (void *) relURL);

	// abs
	if (!PToken(line, token, &spot, colon))
		return (fnfErr);
	if (*token != 8)
		return (fnfErr);
	if (absURL)
		Hex2Bytes(token + 1, 8, (void *) absURL);


	if (spec) {
		err = FSResolveFID(vRef, fid, spec);
		if (err) {
			SubFolderSpec(PARTS_FOLDER, spec);
			PSCopy(spec->name, name);
			err = FSpExists(spec);
			if (err == fnfErr) {
				FixURLString(name);
				if (*name != *spec->name)
					err = FSpExists(spec);
			}
		}
		return (err);
	} else
		return (noErr);
}
#endif

#pragma segment Balloon
/************************************************************************
 * MessHelp - help for the message window
 ************************************************************************/
void MessHelp(MyWindowPtr win, Point mouse)
{
	Rect subRect, bodRect, priorRect, r;
	MessHandle messH = Win2MessH(win);
	Rect blahRect;
	Rect pencilRect;
	Rect fixedRect;
	PETEDocInfo info;
	ControlHandle cntl, ctlGetGraphics;

	PETEGetDocInfo(PETE, TheBody, &info);
	bodRect = info.docRect;
	PETEGetDocInfo(PETE, (*messH)->subPTE, &info);
	subRect = info.docRect;


	if (PtInRect(mouse, &bodRect))
		MyBalloon(&bodRect, 90, 0, MESS_HELP, 0, nil);
	else if (win->topMargin) {
		GetPriorityRect(win, &priorRect);
		GetBlahRect(win, 1, &blahRect);
		GetBlahRect(win, 0, &pencilRect);
		if (!FontIsFixed)
			GetBlahRect(win, 2, &fixedRect);

		if (PtInRect(mouse, &subRect))
			MyBalloon(&subRect, 90, 0, SUB_EDIT_HELP, 0, nil);
		else if (!FontIsFixed && PtInRect(mouse, &fixedRect) &&
			 (cntl = FindControlByRefCon(win, mcFixed)))
			MyBalloon(&fixedRect, 90, 0,
				  GetControlValue(cntl) ? MESS_FIXED_HELP :
				  NO_MESS_FIXED_HELP, 0, nil);
		else if (PtInRect(mouse, &pencilRect))
			MyBalloon(&pencilRect, 90, 0,
				  MessOptIsSet(Win2MessH(win),
					       OPT_WRITE) ? MESS_WRITE_HELP
				  : NO_MESS_WRITE_HELP, 0, nil);
		else if (PtInRect(mouse, &blahRect))
			MyBalloon(&blahRect, 90, 0,
				  MessFlagIsSet(Win2MessH(win),
						FLAG_SHOW_ALL) ?
				  SHOW_ALL_HELP : NO_SHOW_ALL_HELP, 0,
				  nil);
		else if (PtInRect(mouse, &priorRect))
			PriorMenuHelp(win, &priorRect);
		else if ((ctlGetGraphics =
			  FindControlByRefCon(win, mcGetGraphics))
			 && IsControlVisible(ctlGetGraphics)
			 && PtInControl(mouse, ctlGetGraphics)) {
			Rect getGraphicsRect;

			GetControlBounds(ctlGetGraphics, &getGraphicsRect);
			MyBalloon(&getGraphicsRect, 90, 0,
				  IsControlActive(ctlGetGraphics) ?
				  GET_GRAPHICS_HELP : NO_GET_GRAPHICS_HELP,
				  0, nil);
		}
#ifdef TWO
		else {
			GetServerRect(win, 3, &r);
			if (PtInRect(mouse, &r))
				MyBalloon(&r, 90, 0, MESS_DRAG_HELP, 0,
					  nil);
			else {
				r = win->contR;
				r.bottom = win->topMargin;
				r.top = GetRLong(COMP_TOP_MARGIN);
				if (r.bottom > r.top && PtInRect(mouse, &r)
				    && MessOptIsSet(messH, OPT_RECEIPT))
					MyBalloon(&r, 90, 0,
						  MESS_NOTIFY_HELP, 0,
						  nil);
				else if ((*messH)->hasDelIcon) {
					GetServerRect(win, 1, &r);
					if (PtInRect(mouse, &r)) {
						if ((*messH)->hasFetchIcon)
							MyBalloon(&r, 90,
								  0,
								  MESS_FETCH_HELP
								  +
								  (IdIsOnPOPD
								   (PERS_POPD_TYPE
								    (MESS_TO_PPERS
								     (messH)),
								    FETCH_ID,
								    SumOf
								    (messH)->
								    uidHash)
								   ? 0 :
								   1), 0,
								  nil);
					} else {
						GetServerRect(win, 2, &r);
						if (PtInRect(mouse, &r))
							MyBalloon(&r, 90,
								  0,
								  MESS_DELETE_HELP
								  +
								  (IdIsOnPOPD
								   (PERS_POPD_TYPE
								    (MESS_TO_PPERS
								     (messH)),
								    DELETE_ID,
								    SumOf
								    (messH)->
								    uidHash)
								   ? 0 :
								   1), 0,
								  nil);
						else {
							GetServerRect(win,
								      0,
								      &r);
							if (PtInRect
							    (mouse, &r))
								MyBalloon
								    (&r,
								     90, 0,
								     MESS_SERVER_HELP,
								     0,
								     nil);
						}
					}
				}
			}
		}
#endif
	}
}

/************************************************************************
 * PriorMenuHelp - help for the priority menu
 ************************************************************************/
void PriorMenuHelp(MyWindowPtr win, Rect * priorRect)
{
	Str255 s1, s2;
	short p = Prior2Display((SumOf(Win2MessH(win)))->priority);

	if (!HMIsBalloon()) {
		GetRString(s1, PRIOR_MENU_HELP);
		GetRString(s2, PRIOR_STRN + 5 + p);
		PCat(s1, s2);
		MyBalloon(priorRect, 90, 0, 0, 0, s1);
	}
}

#pragma segment MessWin
/**********************************************************************
 * SaveMess - save a message into its own mailbox
 **********************************************************************/
Boolean SaveMess(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	MSumPtr oldSum, newSum;
	TOCHandle tocH = (*messH)->tocH;
	long fromLen;
	Handle text = MessText(messH);
	HeadSpec hSpec;
	Accumulator enriched;
	OSErr err = noErr;
	Boolean richSave = False;
	Str255 title;
	Boolean blahBlah = MessFlagIsSet(messH, FLAG_SHOW_ALL);

	/*
	 * rich text?
	 */
	if (!blahBlah) {
		SetMessRich(messH);
		Zero(enriched);
		if (MessIsRich(messH)) {
			if (!(err = AccuInit(&enriched))) {
				CompHeadFind(messH, 0, &hSpec);
				if (!(*messH)->extras.offset
				    || !(err =
					 AccuAddHandle(&enriched,
						       (*messH)->extras.
						       data)))
					if (!
					    (err =
					     AccuAddFromHandle(&enriched,
							       text, 0,
							       hSpec.
							       value))) {
						if (MessOptIsSet
						    (messH, OPT_HTML)) {
							if (!
							    (err =
							     HTMLPreamble
							     (&enriched,
							      PCopy(title,
								    SumOf
								    (messH)->
								    subj),
							      0, True))) {
								NumToString
								    (SumOf
								     (messH)->
								     msgIdHash,
								     title);
								if (!
								    (err =
								     BuildHTML
								     (&enriched,
								      TheBody,
								      nil,
								      GetHandleSize
								      (text),
								      hSpec.
								      value,
								      nil,
								      nil,
								      1,
								      title,
								      nil,
								      nil)))
									err = HTMLPostamble(&enriched, True);
							}
						} else {
							err =
							    BuildEnriched
							    (&enriched,
							     TheBody, nil,
							     PETEGetTextLen
							     (PETE,
							      TheBody),
							     hSpec.value,
							     nil, True);
						}
						if (!err) {
							AccuTrim
							    (&enriched);
							err =
							    SaveTextAsMessage
							    (nil,
							     enriched.data,
							     (*messH)->
							     tocH,
							     &fromLen);
							ZapHandle(enriched.
								  data);
							if (err)
								return
								    (False);
							richSave = True;
						}
					}
			}
		}
	}

	if (err) {
		WarnUser(CANT_SAVE_RICH, err);
		ZapHandle(enriched.data);
		ClearMessFlag(messH, FLAG_RICH);
		ClearMessOpt(messH, OPT_HTML);
	}

	/*
	 * save plain text to the end of the mailbox
	 */
	if (!richSave)
		if (SaveTextAsMessage
		    (!blahBlah
		     && (*messH)->extras.offset ? (*messH)->extras.
		     data : nil, text, (*messH)->tocH, &fromLen))
			return (False);

	/*
	 * copy the offset parameters
	 */
	oldSum = &(*tocH)->sums[(*messH)->sumNum];
	newSum = &(*tocH)->sums[(*tocH)->count - 1];

	(*tocH)->usedK -= oldSum->length / 1024;	/* we're going to waste the old one */
	(*tocH)->updateBoxSizes = true;
	oldSum->offset = newSum->offset;
	oldSum->length = newSum->length;
	oldSum->bodyOffset = newSum->bodyOffset;
	if (newSum->seconds && !(oldSum->flags & FLAG_OUT)) {
		oldSum->seconds = newSum->seconds;
		oldSum->origZone = newSum->origZone;
	}
	// outgoing IMAP messages have the recipient stored in the From: field of the summary.  
	// Save it.
	if ((*tocH)->imapTOC && (oldSum->opts & OPT_IMAP_SENT)) {
		if (*oldSum->from)
			PCopy(newSum->from, oldSum->from);
	} else {
		if (*newSum->from)
			PCopy(oldSum->from, newSum->from);
	}

	InvalSum(tocH, (*messH)->sumNum);
	// retitle window
	if ((*messH)->win) {
		MakeMessTitle(title, tocH, (*messH)->sumNum, true);
		SetWTitle_(GetMyWindowWindowPtr((*messH)->win), title);
	}


	/*
	 * shrink the .toc
	 */
	(*tocH)->count--;
	SetHandleBig_(tocH, GetHandleSize_(tocH) - sizeof(MSumType));

	/*
	 * reset this to the right stuff
	 */
	(*messH)->weeded =
	    fromLen + (blahBlah ? 0 : (*messH)->extras.offset);

	/*
	 * rescan for URL's, quotes
	 */
	PeteSetURLRescan(TheBody, 0);

	/*
	 * not dirty any more
	 */
	PeteCleanList(win->pteList);
	win->isDirty = False;

	/*
	 * cache is toast
	 */
	ZapHandle(SumOf(messH)->cache);
	SetMessOpt(messH, OPT_EDITED);
	ZapHandle((*messH)->etlFiles);	// we've recorded them permanently now

	// let the preview pane know
	if ((*tocH)->previewID == SumOf(messH)->serialNum)
		(*tocH)->previewID = 0;

	return (True);
}

/**********************************************************************
 * ShowMessageSeparator - show the message separator
 **********************************************************************/
void ShowMessageSeparator(PETEHandle pte, Boolean center)
{
	long body = BodyOffset(PeteText(pte));

	PeteSelect(nil, pte, body, body);
	PeteCalcOn(pte);
	if (center) {
		PeteScroll(pte, 0, 0);
		PeteScroll(pte, 0, pseCenterSelection);
	} else {
		PeteScroll(pte, 0, 0x7fff);
		PeteScroll(pte, 0, pseSelection);
	}
}

/**********************************************************************
 * MessMenu - handle menu choices for a message window
 **********************************************************************/
Boolean MessMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
	MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;
	Boolean result = False;
	short tableId;
	Boolean turbo = False;
	uLong ezOpenSerialNum;
	TextAddrHandle addr = nil;

	switch (menu) {
	case FILE_MENU:
		switch (item) {
		case FILE_PRINT_ITEM:
		case FILE_PRINT_ONE_ITEM:
			MessFocus(messH, TheBody);
			if ((modifiers & shiftKey) && AttIsSelected(win, win->pte, -1, -1, attOpen + attPrint, nil, nil));	// done
			else
				PrintOneMessage((*messH)->win,
						(modifiers & shiftKey) !=
						0,
						item ==
						FILE_PRINT_ONE_ITEM);
			result = True;
			break;
		case FILE_SAVE_ITEM:
			if (win->isDirty) {
				if ((*messH)->subPTE
				    && PeteIsDirty((*messH)->subPTE))
					MessSaveSub(messH);
				if ((*messH)->subPTE
				    && PeteIsDirty((*messH)->bodyPTE))
					SaveMess(win);
				PeteCleanList(win->pteList);
				win->isDirty = False;
			}
			result = True;
			break;
		case FILE_BROWSE_ITEM:
			ExportHTML(messH);
			result = True;
			break;
		case FILE_SAVE_AS_ITEM:
			SaveMessageAs(messH);
			result = True;
			break;
		}
		break;
	case SERVER_HIER_MENU:
#ifdef	IMAP
		ServerMenuChoice((*messH)->tocH, (*messH)->sumNum, item,
				 (modifiers & shiftKey) != 0);
#else
		ServerMenuChoice((*messH)->tocH, (*messH)->sumNum, item);
#endif
		result = True;
		break;
	case EDIT_MENU:
		switch (item) {
#ifdef SPEECH_ENABLED
		case EDIT_SPEAK_ITEM:
			if (!(modifiers & shiftKey)) {
				(void) SpeakMessage(nil, (*messH)->tocH,
						    (*messH)->sumNum,
						    speakSender |
						    speakSubject |
						    speakBody, false);
				return (true);
			}
			break;
#endif
		default:
			return (False);
			break;
		}
		break;
	case MESSAGE_MENU:
		MessFocus(messH, TheBody);
		switch (item) {
		case MESSAGE_REPLY_ITEM:
			{
				Boolean all, quote, self;
				ReplyDefaults(modifiers, &all, &self,
					      &quote);
				DoReplyMessage(win, all, self, quote, True,
					       0, True, True, True);
			}
			result = True;
			break;
		case MESSAGE_REDISTRIBUTE_ITEM:
			DoRedistributeMessage(win, 0,
					      PrefIsSetOrNot
					      (PREF_TURBO_REDIRECT,
					       modifiers, optionKey),
					      !(modifiers & shiftKey),
					      True);
			result = True;
			break;
		case MESSAGE_FORWARD_ITEM:
			DoForwardMessage(win, 0, True);
			result = True;
			break;
		case MESSAGE_SALVAGE_ITEM:
			DoSalvageMessage(win, False);
			result = True;
			break;
		case MESSAGE_JUNK_ITEM:
		case MESSAGE_NOTJUNK_ITEM:
			Junk(tocH, sumNum, item == MESSAGE_JUNK_ITEM,
			     true);
			break;
		case MESSAGE_DELETE_ITEM:
			EzOpen((*messH)->openedFromTocH, -1,
			       (*messH)->openedFromSerialNum, modifiers,
			       True, True);
			NoSaves = !MessOptIsSet(Win2MessH(win), OPT_WRITE);
			if (CloseMyWindow(GetMyWindowWindowPtr(win))) {
#ifdef TWO
				AddXfUndo(tocH, GetTrashTOC(), sumNum);
#endif
				DeleteMessage(tocH, sumNum,
					      (modifiers &
					       (optionKey | shiftKey)) ==
					      (optionKey | shiftKey));
			}
			NoSaves = False;
			result = True;
			break;
		}
		break;
	case REPLY_WITH_HIER_MENU:
		//Stationery - no support for this in Light
		if (HasFeature(featureStationery)) {
			Boolean all, quote, self;
			ReplyDefaults(modifiers, &all, &self, &quote);
			DoReplyMessage(win, all, self, quote, True, item,
				       True, True, True);
		}
		result = True;
		break;
	case FORWARD_TO_HIER_MENU:
		DoForwardMessage(win, (addr = MenuItem2Handle(menu, item)),
				 True);
		result = True;
		break;
	case REDIST_TO_HIER_MENU:
#ifdef TWO
		turbo =
		    PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers,
				   optionKey);
		if (turbo)
			EzOpen(tocH, sumNum, 0, modifiers, True, True);
#endif
		DoRedistributeMessage(win,
				      (addr =
				       MenuItem2Handle(menu, item)), turbo,
				      !(modifiers & shiftKey), True);
		result = True;
		ZapHandle(addr);
		break;
	case TABLE_HIER_MENU:
		if (Menu2TableId
		    (tocH, GetMHandle(TABLE_HIER_MENU), item, &tableId))
			SetMessTable(tocH, sumNum, tableId);
		result = True;
		break;
	case STATE_HIER_MENU:
		SetState(tocH, sumNum, Item2Status(item));
		result = True;
		break;
	case LABEL_HIER_MENU:
		item = Menu2Label(item);
		SetSumColor(tocH, sumNum, item);
		result = True;
		break;
	case SPECIAL_MENU:
		switch (AdjustSpecialMenuSelection(item)) {
		case SPECIAL_MAKE_NICK_ITEM:
			if (modifiers & shiftKey)
				MakeNickFromSelection(win);
			else
				MakeMessNick(win, modifiers);
			result = True;
			break;
		case SPECIAL_MAKEFILTER_ITEM:
			DoMakeFilter(win);
			break;
#ifdef TWO
		case SPECIAL_FILTER_ITEM:
			ezOpenSerialNum = (*messH)->ezOpenSerialNum;
			FilterMessage(flkManual, tocH, sumNum);
			if (FrontWindow_() != GetMyWindowWindowPtr(win)
			    && (*tocH)->count)
				if (ezOpenSerialNum)
					EzOpen(tocH, sumNum,
					       ezOpenSerialNum, modifiers,
					       False, False);
				else
					BoxSelectAfter((*tocH)->win,
						       sumNum);
			result = True;
			break;
#endif
		}
		break;
	case PRIOR_HIER_MENU:
		SetPriority(tocH, sumNum,
			    NewPrior(item,
				     (*tocH)->sums[sumNum].priority));
		result = True;
		break;
	case PERS_HIER_MENU:
		//No Personalities menu in Light
		if (HasFeature(featureMultiplePersonalities)) {
			Str63 name;

			SetPers(tocH, sumNum,
				FindPersByName(MyGetItem
					       (GetMHandle(menu), item,
						name)), True);
			return (True);
		}
		break;
#ifdef DEBUG
	case DEBUG_MENU:
		if (item == dbTestSpooler) {
			SpoolMessage(messH, nil, 0);
			return (True);
		}
		break;
#endif
	default:
		if ((*messH)->openedFromTocH != (*messH)->tocH)
			sumNum =
			    FindSumBySerialNum((*messH)->openedFromTocH,
					       (*messH)->
					       openedFromSerialNum);
		result =
		    TransferMenuChoice(menu, item,
				       (*messH)->openedFromTocH, sumNum,
				       modifiers, False);
		break;
	}
	ZapHandle(addr);
	return (result);
}

/************************************************************************
 * Fcc - add a mailbox to 
 ************************************************************************/
void Fcc(MessHandle messH, FSSpecPtr box)
{
	Str255 scratch;
	HeadSpec hs;
	long start;
	Str255 path;

	//Folder Carbon Copy    No FCC in Light
	if (!HasFeature(featureFcc))
		return;

	UseFeature(featureFcc);

	if (Box2Path(box, path))
		PCopy(path, box->name);

	*scratch = 0;
	PCatC(scratch, '"');
	PCatR(scratch, FCC_PREFIX);
	PCat(scratch, path);
	PCatC(scratch, '"');
	SetPort(GetMyWindowCGrafPtr((*messH)->win));
	if (CompHeadFind(messH, BCC_HEAD, &hs)) {
		PetePrepareUndo(TheBody, peCantUndo, hs.stop, hs.stop,
				&start, nil);
		InsertCommaIfNeedBe(TheBody, &hs);
		CompHeadAppendStr(TheBody, &hs, scratch);
		CompHeadFind(messH, BCC_HEAD, &hs);
		PeteSelect(nil, TheBody, hs.stop, hs.stop);
		PeteFinishUndo(TheBody, peUndoPaste, start, hs.stop);
		ClearMessFlag(messH, FLAG_KEEP_COPY);
		CompIBarUpdate(messH);
	}
}

/**********************************************************************
 * EzOpenFind - find next candidate for EzOpen.  If none, return -1
 **********************************************************************/
short EzOpenFind(TOCHandle tocH, short origSum)
{
	short newSum;
	long ez = GetPrefLong(PREF_NO_EZ_OPEN);

	if (origSum < (*tocH)->count - 1) {
		if (ez == 1)
			return (origSum + 1);	// say no more
		if ((*tocH)->sums[origSum + 1].state == UNREAD) {
			if (ez == 3 || ez == 2)
				return (origSum + 1);	// say no more
			if (ez == 4) {
				Str63 s1, s2;
				PSCopy(s1, (*tocH)->sums[origSum].subj);
				PSCopy(s2,
				       (*tocH)->sums[origSum + 1].subj);
				if (!SubjCompare(s1, s2))
					return origSum + 1;
			}
		}
	}

	if (ez == 2)
		for (newSum = origSum + 2; newSum < (*tocH)->count;
		     newSum++)
			if ((*tocH)->sums[newSum].state == UNREAD)
				return (newSum);

	return (-1);
}

/************************************************************************
 * EzOpen - easy open, if things are right
 ************************************************************************/
void EzOpen(TOCHandle tocH, short sumNum, uLong serialNum, long modifiers,
	    Boolean hideFront, Boolean willDelete)
{
	WindowPtr oldFrontWP = FrontWindow_();
	short newSumNum;
	Boolean oldVis;
	short ez;
	Boolean preview = false;

	if ((*tocH)->win) {
		if (sumNum < 0) {
			sumNum = FindSumBySerialNum(tocH, serialNum);
			if (sumNum < 0)
				return;	//      unable to find message with this serial number
		}

		/*
		 * if the frontmost window is our cue, grab its hash value
		 */
		if (hideFront) {
			TOCHandle realTOC;
			short realSum;

			realTOC = GetRealTOC(tocH, sumNum, &realSum);
			if ((*realTOC)->sums[realSum].messH)
				serialNum =
				    (*(*realTOC)->sums[realSum].messH)->
				    ezOpenSerialNum;
			else if ((*tocH)->previewPTE && (*tocH)->previewID) {
				preview = true;
				serialNum = (*tocH)->ezOpenSerialNum;
			}
		} else
			sumNum = FindSumBySerialNum(tocH, serialNum);

		/*
		 * does message exist?
		 */
		if (serialNum
		    && (newSumNum =
			FindSumBySerialNum(tocH, serialNum)) >= 0
		    && (*tocH)->sums[newSumNum].state == UNREAD)
			/* we have our man */ ;
		else {
			/*
			 * if we're not hiding the front window, that means it was deleted before
			 * the easy open; back up one
			 */
			if (!hideFront)
				newSumNum = EzOpenFind(tocH, sumNum - 1);
			else
				newSumNum = EzOpenFind(tocH, sumNum);
		}

		/*
		 * did we find one to easy open?
		 */
		if (newSumNum >= 0) {
			ez = GetPrefLong(PREF_NO_EZ_OPEN);
			if (modifiers & shiftKey)
				ez = 0;
			SelectBoxRange(tocH, newSumNum, newSumNum, False,
				       -1, -1);
			if (ez) {
				if (preview)
					Preview(tocH, newSumNum);
				else {
					MyWindowPtr oldFront =
					    GetWindowMyWindowPtr
					    (oldFrontWP);
					if (hideFront && oldFront) {
						oldVis = oldFront->hideme;
						oldFront->hideme = True;
						UglyHackFrontWindow =
						    oldFrontWP;
					}
					BoxOpen((*tocH)->win);
					if (hideFront && oldFront)
						oldFront->hideme = oldVis;
					UglyHackFrontWindow = nil;
				}
			}
			BoxCenterSelection((*tocH)->win);
		} else {
			if (willDelete)
				newSumNum =
				    sumNum ==
				    (*tocH)->count - 1 ? sumNum -
				    1 : sumNum + 1;
			else
				newSumNum = sumNum;
			newSumNum = MIN(newSumNum, (*tocH)->count - 1);
			newSumNum = MAX(newSumNum, 0);
			SelectBoxRange(tocH, newSumNum, newSumNum, False,
				       -1, -1);
		}
	}
}

/************************************************************************
 * SaveMessageAs - save a message in text only form
 ************************************************************************/
void SaveMessageAs(MessHandle messH)
{
	long creator;
	short refN;
	short err;
	Str31 name;
	FSSpec spec;
	Str255 scratch;
	ModalFilterYDUPP filter = nil;

	NicknameWatcherFocusChange((**messH).win->pte);	/* MJN */

	/*
	 * tickle stdfile
	 */
	creator = DefaultCreator();
	MakeMessFileName((*messH)->tocH, (*messH)->sumNum, spec.name);
#ifdef TWO
	//Stationery - prevent the user from saving a file as stationery
	Stationery = (HasFeature(featureStationery) && (*(*messH)->tocH)->which == OUT && !DateWarning(false)) ? False : 2;	/* 2 is our signal to the filter to hide the item */
#endif
	if (err =
	    SFPutOpen(&spec, creator, 'TEXT', &refN, nil, filter,
		      SAVEAS_DLOG, nil, nil, nil))
		return;

	/*
	 * do it
	 */
	if (Stationery) {
		//      Need to save with different msg ID hash
		TOCHandle tocH = (*messH)->tocH;
		short sumNum = (*messH)->sumNum;
		long saveHash = (*tocH)->sums[sumNum].msgIdHash;

		NewMessageId(scratch);
		SetHash(tocH, sumNum, MIDHash(scratch + 1, *scratch));
		err = SaveAsToOpenFile(refN, messH);
		SetHash(tocH, sumNum, saveHash);
	} else
		err = SaveAsToOpenFile(refN, messH);

	/*
	 * close
	 */
	(void) MyFSClose(refN);

	/*
	 * report error
	 */
	if (err) {
		FileSystemError(COULDNT_SAVEAS, name, err);
		FSpDelete(&spec);
	}
}

/************************************************************************
 * SaveAsToOpenFile - continue the message saving process
 ************************************************************************/
short SaveAsToOpenFile(short refN, MessHandle messH)
{
	OSErr err;
	unsigned long saveOpts = SumOf(messH)->opts;

	err = SaveAsToOpenFileLo(refN, messH);
	if (Stationery && !err
	    && ((saveOpts & OPT_HAS_SPOOL) !=
		(SumOf(messH)->opts & OPT_HAS_SPOOL))) {
		//      The first save didn't save the OPT_HAS_SPOOL flag properly. Save again.
		//      Shameful, but this should happen only the first time a stationery
		//      is saved with graphics.
		SetFPos(refN, fsFromStart, 0);
		err = SaveAsToOpenFileLo(refN, messH);
	}
	return err;
}

/************************************************************************
 * SaveAsToOpenFileLo - continue the message saving process
 ************************************************************************/
static short SaveAsToOpenFileLo(short refN, MessHandle messH)
{
	long bytes;
	short err = 0;
	UPtr where;
	UHandle text;
	Str63 scratch;
	Boolean para = PrefIsSet(PREF_PARAGRAPHS);
	Boolean exclHead = PrefIsSet(PREF_EXCLUDE_HEADERS)
	    || (SumOf(messH)->flags & FLAG_SUBSEQUENT);
	Boolean isOut = (*(*messH)->tocH)->which == OUT;
	HeadSpec hs;
	Accumulator enriched;
	StackHandle stack = nil;

	Zero(enriched);

	SetMessRich(messH);
	PETEGetRawText(PETE, TheBody, &text);
	CompHeadFind(messH, 0, &hs);

#ifdef TWO
	//Stationery - prevent the user from saving a file as stationery
	if (HasFeature(featureStationery) && Stationery == True) {
		if (err = SaveStationeryStuff(refN, messH))
			return (err);
		para = exclHead = False;
	}
#endif

	/*
	 * build and save date header
	 */
	if (!exclHead && isOut && SumOf(messH)->seconds) {
		BuildDateHeader(scratch, SumOf(messH)->seconds);
		PCatC(scratch, '\015');
		FSWriteP(refN, scratch);
	}

	where = LDRef(text);
	bytes = GetHandleSize(text);
	if (exclHead) {
		where = LDRef(text) + hs.value;
		bytes = hs.stop - hs.value;
		while (bytes > 1 && *where == '\015') {
			where++;
			bytes--;
		}
		while (bytes > 1 && where[bytes - 1] == where[bytes - 2]
		       && where[bytes - 1] == '\015')
			bytes--;
	}
	//Stationery - prevent the user from saving a file as stationery        
	if (HasFeature(featureStationery) && Stationery) {
		if (MessIsRich(messH) && !(err = AccuInit(&enriched))) {
			AccuAddPtr(&enriched, where, hs.start);
			AccuAddChar(&enriched, '\015');
			if (MessOptIsSet(messH, OPT_HTML)) {
				if (!
				    (err =
				     StackInit(sizeof(FSSpec), &stack)))
					if (!
					    (err =
					     HTMLPreamble(&enriched,
							  PCopy(scratch,
								SumOf
								(messH)->
								subj), 0,
							  True)))
						if (!
						    (err =
						     BuildHTML(&enriched,
							       TheBody,
							       nil,
							       PETEGetTextLen
							       (PETE,
								TheBody),
							       hs.value,
							       nil, nil, 1,
							       CompGetMID
							       (messH,
								scratch),
							       nil, nil)))
							err =
							    HTMLPostamble
							    (&enriched,
							     True);
			} else
				err =
				    BuildEnriched(&enriched, TheBody, nil,
						  PETEGetTextLen(PETE,
								 TheBody),
						  hs.value, nil, True);
			ZapHandle(stack);
			if (!err) {
				AccuTrim(&enriched);
				text = enriched.data;
				where = LDRef(enriched.data);
				bytes = enriched.offset;
			}
		}
	}

	if (!err && !para) {
		err = AWrite(refN, &bytes, where);
		EnsureNewline(refN);
		UL(text);
	} else {
		err = UnwrapSave(where, bytes, 0, refN);
		UL(text);
	}
	if (!err)
		BeenThereDoneThat((*messH)->tocH, (*messH)->sumNum);
	AccuZap(enriched);
	return (err);
}

/************************************************************************
 * UnwrapSave - save and unwrap a message
 ************************************************************************/
#define flushBChars() do {																			\
	if (bytes=bSpot-buffer) 																			\
	{ 																														\
		 err = refN ? AWrite(refN,&bytes,buffer) : 								\
		 (WrapHandle ? PtrPlusHand_(buffer,WrapHandle,bytes) : noErr);\
		 if (err) goto done;																				\
		 bSpot = buffer;																						\
	} 																														\
} while (0)
#define putBChar(theC) do { 																		\
	*bSpot++ = theC;																							\
	if (bSpot==bEnd) flushBChars(); 															\
} while (0)
#define UnwrapSpaceRuns 		((unwrapPref&1)==0)
#define UnwrapCenteredBreak ((unwrapPref&2)==0)
#define UnwrapIndent 				((unwrapPref&4)==0)
#define UnwrapQuote 				((unwrapPref&8)==0)
#define UnwrapShortLine 		((unwrapPref&16)==0)
#define UnwrapBlankLine 		((unwrapPref&32)==0)
int UnwrapSave(UPtr text, long length, long offset, short refN)
{
	struct LineInfo {
		int length;
		int indent;
		int quote;
		int needReturn;
	};
	struct LineInfo lines[2];
	struct LineInfo *this, *last;
	int flip;
	int c;
	UPtr tSpot;
	int begin;
	int spaces;
	UPtr buffer = NuPtr(BUFFER_SIZE);
	UPtr bSpot, bEnd;
	int err = 0;
	long bytes;
	Str31 s;
	Byte quote;
	Boolean quoteSpace;
	short i;
	uLong unwrapPref = GetPrefLong(PREF_UNWRAP_OPTIONS);

	quote = GetRString(s, QUOTE_PREFIX)[1];
	quoteSpace = s[*s] == ' ';

	if (!buffer)
		return (MemError());
	bEnd = buffer + BUFFER_SIZE;
	bSpot = buffer;

	WriteZero(lines, sizeof(lines));
	this = lines;
	last = lines + 1;
	flip = this->length = this->indent = this->needReturn =
	    last->length = last->indent = 0;
	last->needReturn = begin = 1;

	for (tSpot = text + offset; tSpot < text + length; tSpot++) {
		c = *tSpot;
		if (c == '\015') {
			this->needReturn = this->needReturn ||
			    (UnwrapCenteredBreak && this->indent > 20) ||
			    (UnwrapShortLine && this->length < 40) ||
			    (UnwrapBlankLine && this->length == 0);
			if (this->needReturn) {
				if (this->length == 0) {
					if (!last->needReturn)
						putBChar('\015');
					for (i = 0; i < this->quote; i++)
						putBChar(quote);
				}
				putBChar('\015');
			} else
				putBChar(' ');
			last = this;
			flip = 1 - flip;
			this = lines + flip;
			this->length = this->quote = this->indent =
			    this->needReturn = 0;
			begin = 1;
			spaces = 0;
		} else if (c == ' ') {
			if (begin)
				this->indent++;
			else
				spaces++;
		} else if (begin && c == quote) {
			this->quote++;
		} else {
			if (!begin) {
				if (spaces > 4
				    && (void *) SendTrans !=
				    (void *) WrapSendTrans
				    && UnwrapSpaceRuns) {
					putBChar('\t');
					this->needReturn = 1;
				} else if (spaces) {
					putBChar(' ');
					this->length++;
				}
			} else
			    if (((UnwrapIndent)
				 && this->indent > last->indent
				 || UnwrapQuote
				 && this->quote != last->quote)
				&& !last->needReturn) {
				putBChar('\015');
				for (i = 0; i < this->quote; i++)
					putBChar(quote);
				this->length += this->quote;
				if (quoteSpace) {
					this->length++;
					putBChar(' ');
				}
			} else if (this->quote && last->needReturn) {
				for (i = 0; i < this->quote; i++)
					putBChar(quote);
				this->length += this->quote;
				if (quoteSpace) {
					this->length++;
					putBChar(' ');
				}
			}
			begin = 0;
			spaces = 0;
			putBChar(c);
			this->length++;
		}
	}
	flushBChars();
      done:
	ZapPtr(buffer);
	return (err);
}

/************************************************************************
 * MessKey - handle a keydown in a message window
 ************************************************************************/
Boolean MessKey(MyWindowPtr win, EventRecord * event)
{
	MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	long uLetter = UnadornMessage(event) & charCodeMask;
	Boolean bodyEdit = !win->ro && win->pte == TheBody;
	Boolean shift = 0 != (event->modifiers & shiftKey);

	if (leftArrowChar <= uLetter && uLetter <= downArrowChar &&
	    IsArrowSwitch(event->modifiers)) {
		NextMess(tocH, messH, uLetter, event->modifiers, False);
		return (True);
	} else if ((event->modifiers & cmdKey) != 0
		   && (uLetter == delChar || uLetter == deleteKey)) {
		DoMenu2(GetMyWindowWindowPtr(win), MESSAGE_MENU,
			MESSAGE_DELETE_ITEM, event->modifiers);
		return (true);
	} else if (!bodyEdit
		   && (uLetter == tabChar || uLetter == enterChar)) {
		MessFocus(messH,
			  win->pte ==
			  (*messH)->subPTE ? TheBody : (*messH)->subPTE);
		if (win->pte == (*messH)->subPTE)
			PeteSelectAll(win, win->pte);
	} else if (event->modifiers & cmdKey) {
		return (False);
	} else if (win->ro && uLetter == ' ') {
		if (PeteScroll(TheBody, 0, shift ? psePageUp : psePageDn)
		    && !shift) {
			NextMess(tocH, messH, downArrowChar, 0, True);
			return (True);
		}
	} else if (win->ro && DirtyKey(event->message))
		AlertStr(READ_ONLY_ALRT, Stop, nil);
	else if (!bodyEdit && !win->ro && uLetter == returnChar)
		SysBeep(20L);
	else {
		PeteEdit(win, win->pte, peeEvent, (void *) event);
	}
	PeteSetDirty(win->pte);
	return (True);
}

/************************************************************************
 * NextMess - skip to the next message
 ************************************************************************/
void NextMess(TOCHandle tocH, MessHandle messH, short whichWay,
	      long modifiers, Boolean ezOpen)
{
	WindowPtr messWinWP = GetMyWindowWindowPtr((*messH)->win);
	short next;
	Boolean close, diffTOC;
	short sumNum;

	if (IsArrowSwitch(modifiers))
		modifiers &= ~GetPrefLong(PREF_SWITCH_MODIFIERS);

	close = !(modifiers & optionKey);	/* should we close the current one? */

	if (ezOpen && tocH == (*messH)->openedFromTocH) {
		EzOpen(tocH, (*messH)->sumNum, 0, 0, True, False);
		CloseMyWindow(messWinWP);
		return;
	}

	if ((*messH)->openedFromTocH && tocH != (*messH)->openedFromTocH) {
		tocH = (*messH)->openedFromTocH;
		if (!FindRealSummary
		    (tocH, (*messH)->openedFromSerialNum, &sumNum))
			return;
		diffTOC = true;
	} else {
		sumNum = (*messH)->sumNum;
		diffTOC = false;
	}

	switch (whichWay) {
	case upArrowChar:
	case leftArrowChar:
		next = sumNum - 1;
		break;
	case downArrowChar:
	case rightArrowChar:
		next = sumNum + 1;
		break;
	default:
		return;
	}
	if (next != sumNum) {
		if (close && (*messH)->win->isDirty) {	/* close before switch if dirty */
			if (!CloseMyWindow(messWinWP))
				return;
			close = False;
		}

		if (next >= 0 && next < (*tocH)->count) {
			if ((*tocH)->sums[next].messH) {
				WindowPtr tocMessWinWP =
				    GetMyWindowWindowPtr((*(MessHandle)
							  (*tocH)->
							  sums[next].
							  messH)->win);
				ShowMyWindow(tocMessWinWP);
				UserSelectWindow(tocMessWinWP);
				UpdateMyWindow(tocMessWinWP);
			} else {
				if (close)
					(*messH)->win->hideme = true;
				(void) GetAMessage(tocH, next, nil, nil,
						   True);
				if (close)
					(*messH)->win->hideme = false;
				if (diffTOC)
					BeenThereDoneThat(tocH, next);	//      set status to READ in virtual mailbox
			}
			if ((*tocH)->sums[next].messH)
				UpdateMyWindow(GetMyWindowWindowPtr
					       ((*(*tocH)->sums[next].
						 messH)->win));
			SelectBoxRange(tocH, next, next, False, -1, -1);
			BoxCenterSelection((*tocH)->win);
		}
	}
	if (close)
		CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
}

/**********************************************************************
 * 
 **********************************************************************/
void SADL_Hilite(DialogPtr dgPtr)
{
	short junk;
	ControlHandle cntl;
	Rect r;

	GetDialogItem(dgPtr, SADL_EXCLUDE_HEADERS, &junk,
		      (Handle *) & cntl, &r);
	HiliteControl(cntl, Stationery == 1 ? 255 : 0);
	GetDialogItem(dgPtr, SADL_PARAGRAPHS, &junk, (Handle *) & cntl,
		      &r);
	HiliteControl(cntl, Stationery == 1 ? 255 : 0);
	GetDialogItem(dgPtr, SADL_STATIONERY_FOLDER, &junk,
		      (Handle *) & cntl, &r);
	HiliteControl(cntl, Stationery == 1 ? 0 : 255);
}

/**********************************************************************
 * MessFind - find in the window
 **********************************************************************/
Boolean MessFind(MyWindowPtr win, PStr what)
{
	return FindInPTE(win, (*Win2MessH(win))->bodyPTE, what);
}

/************************************************************************
 * MessProxy - handle a click in the proxy icon of a message window
 ************************************************************************/
void MessProxy(MyWindowPtr win, EventRecord * event)
{
	Point pt = event->where;
	MessHandle messH = Win2MessH(win);

	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(win));
	GlobalToLocal(&pt);
	DragXfer(pt, nil, messH);
	PopGWorld();
}

/************************************************************************
 * MessClick - handle a click in the message window
 ************************************************************************/
void MessClick(MyWindowPtr win, EventRecord * event)
{
	Point pt = event->where;
	PETEHandle pte = nil;
	MessHandle messH = Win2MessH(win);

	SetPort_(GetMyWindowCGrafPtr(win));
	GlobalToLocal(&pt);

	if (!win->isActive) {
		SelectWindow_(GetMyWindowWindowPtr(win));
		return;
	}
	if (PtInPETE(pt, TheBody)) {
		pte = TheBody;
	} else if (PtInPETE(pt, (*messH)->subPTE)) {
		pte = (*messH)->subPTE;
	}

#ifdef IMAP
	// Don't allow subjects of undownloaded messages to be changed.
	if (pte
	    && (!(*((*messH)->tocH))->imapTOC
		|| IMAPMessageDownloaded((*messH)->tocH,
					 (*messH)->sumNum)))
#else
	if (pte)
#endif
	{
		if (PeteEdit(win, pte, peeEvent, (void *) event) ==
		    tsmDocNotActiveErr) {
			MessFocus(messH, pte);
			PeteEdit(win, pte, peeEvent, (void *) event);
		}
	} else if (win->labelCntl && PtInControl(pt, win->labelCntl)) {
		BoxLabelMenu((*messH)->tocH, (*messH)->sumNum, messH, pt);
	} else {
		EnableMenuItems(False);
		SetMenuTexts(0, False);
		HandleControl(pt, win);
	}
}

/**********************************************************************
 * MessButton - click on a button in the message
 **********************************************************************/
void MessButton(MyWindowPtr win, ControlHandle button, long modifiers,
		short part)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	MessHandle messH = Win2MessH(win);
	short value = GetControlValue(button);
	PETEDocInfo info;
#ifndef USEFIXEDDEFAULTFONT
	Str63 s;
#endif
	short oldWidth, width, fixedWidth, fixedID, fixedSize, newWidth;
	Rect rWin;

	switch (GetControlReference(button)) {
	case NOTIFY_CNTL:
		if (!(modifiers & optionKey))
			GenerateReceipt(messH, MDN_DISPLAYED,
					MDN_DISPLAYED_LOCAL,
					MessOptIsSet(messH,
						     OPT_AUTO_OPENED) ?
					MDN_AUTO_ACTION : MDN_MAN_ACTION,
					MDN_MAN_SENT);
		ClearMessOpt(messH, OPT_RECEIPT);
		DisposeControl(button);
		if (button = FindControlByRefCon(win, MDN_REQUEST))
			DisposeControl(button);
		if (button = FindControlByRefCon(win, MDN_REQUEST + 1))
			DisposeControl(button);
		SetTopMargin(win, GetRLong(COMP_TOP_MARGIN));
		MyWindowDidResize(win, nil);
		break;

	case PRIOR_HIER_MENU:
		DoMenu(winWP,
		       PRIOR_HIER_MENU << 16 | GetBevelMenuValue(button),
		       modifiers);
		break;

	case mcMesgErrors:
		break;

	case mcWrite:
		MessMakeEditable(win, 1 - value);
		break;

	case mcBlahBlah:
		PETEGetDocInfo(PETE, TheBody, &info);
		if (info.dirty) {
			if (!PrefIsSet(PREF_EZ_SAVE)) {
				if (!SaveMessHi(win, False))
					return;
			} else if (!SaveMess(win))
				return;
		}
		SetControlValue(button, 1 - value);
		if (value)
			ClearMessFlag(messH, FLAG_SHOW_ALL);
		else
			SetMessFlag(messH, FLAG_SHOW_ALL);
		ReopenMessage(win);
		break;

	case mcFixed:
#ifdef USEFIXEDDEFAULTFONT
		fixedSize = FixedSize;
		fixedID = FixedID;
#else
		// what size to use?
		fixedSize = GetRLong(DEF_FIXED_SIZE);
		if (!fixedSize)
			fixedSize = FontSize;
		// what font to use?
		fixedID = GetFontID(GetRString(s, FIXED_FONT));
		if (fixedID == applFont)
			fixedID =
			    (ScriptVar(smScriptMonoFondSize) >> 16) &
			    0xffff;
#endif
		// calculate window widths
		fixedWidth =
		    GetRLong(DEF_FIXED_MWIDTH) * CharWidthInFont('0',
								 fixedID,
								 fixedSize)
		    + PETE_SCROLLY_DIFFERENCE;
		width = MessWi(win);
		newWidth = 0;
		GetWindowPortBounds(winWP, &rWin);
		oldWidth = RectWi(rWin);

		// change font
		if (GetControlValue(button)) {
			PeteFontAndSize(TheBody, kPETEDefaultFont,
					kPETEDefaultSize);
			PeteFontAndSize((*messH)->subPTE, kPETEDefaultFont,
					kPETEDefaultSize);
			if (ABS(oldWidth - fixedWidth) < 10)
				newWidth = width;
			ClearMessFlag(messH, FLAG_FIXED_WIDTH);
		} else {
			PeteFontAndSize(TheBody, fixedID, fixedSize);
//                              PeteFontAndSize((*messH)->subPTE,fixedID,fixedSize);
			if (ABS(oldWidth - width) < 10)
				newWidth = fixedWidth;
			SetMessFlag(messH, FLAG_FIXED_WIDTH);
		}
		SetControlValue(button, 1 - value);

		// do we need to resize the window?
		if (newWidth) {
			SizeWindow(winWP, newWidth, RectHi(rWin), false);
			MyWindowDidResize(win, nil);
		}
		break;

	case mcFetch:
		SetControlValue(button, 1 - value);
		if (value) {
			RemIdFromPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),
				      FETCH_ID, SumOf(messH)->uidHash);
			if (!PrefIsSet(PREF_LMOS)) {
				RemIdFromPOPD(PERS_POPD_TYPE
					      (MESS_TO_PPERS(messH)),
					      DELETE_ID,
					      SumOf(messH)->uidHash);
				SetControlValue(FindControlByRefCon
						(win, mcTrash), 0);
				HiliteControl(FindControlByRefCon
					      (win, mcTrash), 0);
			}
		} else {
			AddIdToPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),
				    FETCH_ID, SumOf(messH)->uidHash,
				    False);
			if (!PrefIsSet(PREF_LMOS)) {
				AddIdToPOPD(PERS_POPD_TYPE
					    (MESS_TO_PPERS(messH)),
					    DELETE_ID,
					    SumOf(messH)->uidHash, False);
				SetControlValue(FindControlByRefCon
						(win, mcTrash), 1);
				HiliteControl(FindControlByRefCon
					      (win, mcTrash), 255);
			}
		}
		InvalTocBox((*messH)->tocH, (*messH)->sumNum, blServer);
		break;

	case mcGetGraphics:
		HiliteControl(FindControlByRefCon(win, mcGetGraphics), 255);	//      Disabling button indicates that graphics should now be downloaded
		PeteRecalc(win->pte);
		break;

	case mcTrash:
		SetControlValue(button, 1 - value);
		if (value)
			RemIdFromPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),
				      DELETE_ID, SumOf(messH)->uidHash);
		else
			AddIdToPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),
				    DELETE_ID, SumOf(messH)->uidHash,
				    True);
		InvalTocBox((*messH)->tocH, (*messH)->sumNum, blServer);
		break;
	}

	AuditHit((modifiers & shiftKey) != 0,
		 (modifiers & controlKey) != 0,
		 (modifiers & optionKey) != 0, (modifiers & cmdKey) != 0,
		 false, GetWindowKind(winWP), GetControlReference(button),
		 mouseDown);
}

/************************************************************************
 * MessMakeEditable - flip the pencil for a message
 ************************************************************************/
OSErr MessMakeEditable(MyWindowPtr win, Boolean value)
{
	PETEDocInfo info;
	MessHandle messH = Win2MessH(win);
	PETETextStyle style;

	if (!value) {
		PETEGetDocInfo(PETE, TheBody, &info);
		if (info.dirty) {
			if (!PrefIsSet(PREF_EZ_SAVE)) {
				if (!SaveMessHi(win, False))
					return userCanceledErr;
			} else if (!SaveMess(win))
				return userCanceledErr;
		}
		PETESetCallback(PETE, TheBody, nil, peDocChanged);
		ClearMessOpt(messH, OPT_WRITE);
		win->ro = win->pte == TheBody;
		win->userSave = false;
		PETEHonorLock(PETE, TheBody, peModLock | peSelectLock);
		style.tsLock = peModLock;
		PETESetTextStyle(PETE, TheBody, 0L, 0x7fffffff, &style,
				 peLockValid);
		win->isDirty = False;
		PeteCleanList(win->pteList);
	} else {
		SetMessOpt(messH, OPT_WRITE);
		win->userSave = true;
		win->ro = False;
		PETESetCallback(PETE, TheBody, (void *) PeteChanged,
				peDocChanged);
		PETEHonorLock(PETE, TheBody, peNoLock);
	}
	SetControlValue(FindControlByRefCon(win, mcWrite), value);
	return noErr;
}


/************************************************************************
 * MessGonnaShow - get ready to show a message window
 ************************************************************************/
OSErr MessGonnaShow(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
	short margin;
	Rect r;
	ControlHandle blah;
	Boolean on = MessOnPOPD(POPD_ID, messH);
	MControlsEnum cIndex;
	short icon;
	short value;
	long scanned;
	Boolean junk =
	    SumOf(messH)->spamScore >= GetRLong(JUNK_MAILBOX_THRESHHOLD);

	win->didResize = MessDidResize;
	win->key = MessKey;
	win->update = MessUpdate;
	win->help = MessHelp;
	win->button = MessButton;
	win->dontControl = True;
	win->zoomSize = MessZoomSize;
	win->label = SumColor(SumOf(messH));
	margin = GetRLong(COMP_TOP_MARGIN);
	if (!MessFlagIsSet(messH, FLAG_OUT)
	    && MessOptIsSet(messH, OPT_RECEIPT)
	    && !GetPrefLong(PREF_RECEIPT))
		margin += GROW_SIZE + 2 * INSET;
	SetTopMargin(win, margin);
	if (MakeSubjectTXE(messH)) {
		win->click = win->bgClick = MessClick;
#ifdef USECMM_BAD
		win->buildCMenu = MessBuildCMenu;
#endif
	} else {
		PeteCleanList(win->pte);
		win->isDirty = False;
		CloseMyWindow(winWP);
		return (-108);
	}
	PeteFocus(win, TheBody, True);
	(*PeteExtra(TheBody))->containsJunkMail = junk;


	SetBGColorsByPers(messH);

	/*
	 * Align headers
	 */
	//if (GetPrefLong(PREF_DONT_ALIGN_HEADERS)!=2) AlignHeaders(messH);

	/*
	 * some attachments
	 */
	for (scanned = (*PeteExtra(TheBody))->urlScanned;
	     scanned != -1 && scanned < 3 K;
	     scanned = (*PeteExtra(TheBody))->urlScanned)
		PeteURLScan(win, TheBody);
	for (scanned = (*PeteExtra(TheBody))->quoteScanned;
	     scanned != -1 && scanned < 3 K;
	     scanned = (*PeteExtra(TheBody))->quoteScanned)
		PeteQuoteScan(TheBody);

	(*PeteExtra(TheBody))->emoDesired =
	    !MessFlagIsSet(messH, FLAG_SHOW_ALL);
	if ((*PeteExtra(TheBody))->emoDesired && EmoDo()) {
		for (scanned = (*PeteExtra(TheBody))->emoScanned;
		     scanned != -1 && scanned < 3 K;
		     scanned = (*PeteExtra(TheBody))->emoScanned)
			EmoScanPete(TheBody, false);
	}

	/*
	 * calculate
	 */
	PeteCalcOn(TheBody);

	/*
	 * controls
	 */
	Zero(r);
	r.bottom = r.right = 2;

	for (cIndex = mcWrite; cIndex <= mcTrash; cIndex++) {
		if (FontIsFixed && cIndex == mcFixed)
			continue;	// only if default font is proportional
		blah = GetNewControlSmall(ICON_BAR_CNTL, winWP);
		if (blah) {
			EmbedControl(blah, win->topMarginCntl);
			SetControlReference(blah, cIndex);
			switch (cIndex) {
			case mcWrite:
				icon = PENCIL_SICN;
				break;
			case mcBlahBlah:
				icon = BLAH_SICN;
				break;
			case mcFixed:
				icon = FIXED_WIDTH_ICON;
				if (!FontIsFixed
				    && !(MessOptIsSet(messH, OPT_HTML)
					 || MessFlagIsSet(messH,
							  FLAG_RICH))
				    && PrefIsSet(PREF_FIXIFY))
					MessButton(win, blah, 0,
						   kControlButtonPart);
				break;
			case mcFetch:
				icon = FETCH_SICN;
				break;
			case mcGetGraphics:
				icon = MISSING_IMAGE_ICON;
				break;
			case mcTrash:
				icon = TRASH_SICN;
				value = MessOnPOPD(DELETE_ID, messH);
				break;
			}
			SetBevelIcon(blah, icon, 0, 0, nil);
		} else {
			NoSaves = True;
			CloseMyWindow(winWP);
			NoSaves = False;
			return (-108);
		}
	}

	AddPriorityPopup(messH);

	PushGWorld();
	SetWindowProxyCreatorAndType(winWP, CREATOR, MAILBOX_TYPE,
				     kOnSystemDisk);
	win->proxy = MessProxy;
	PopGWorld();

	MessIBarUpdate(messH);

	/*
	 * notification stuff
	 */
	CheckAddNotifyControls(win, messH);

	// error notification
	AddMessErrNote(messH);

	BeenThereDoneThat((*messH)->tocH, (*messH)->sumNum);

	HiliteOddReply(messH);

	if (AnalSpeakPhrases()) {
		(*PeteExtra(TheBody))->taeDesired = true;
		(*PeteExtra(TheBody))->taeSpeak = true;
	}

	if (!win->ro)
		PETESetCallback(PETE, TheBody, (void *) PeteChanged,
				peDocChanged);

	return (noErr);
}

#define CARBON_NOTIFY_BUTTON_V_ADJUSTMENT 4

/************************************************************************
 * CheckAddReturnReceipt - see if we need to add return receipt to this message
 ************************************************************************/
Boolean CheckAddNotifyControls(MyWindowPtr win, MessHandle messH)
{
	Boolean result = false;

	if (!MessFlagIsSet(messH, FLAG_OUT)
	    && MessOptIsSet(messH, OPT_RECEIPT)
	    && !FindControlByRefCon(win, NOTIFY_CNTL)) {
		if (!GetPrefLong(PREF_RECEIPT))
			SetTopMargin(win,
				     GetRLong(COMP_TOP_MARGIN) +
				     GROW_SIZE + 2 * INSET +
				     2 *
				     CARBON_NOTIFY_BUTTON_V_ADJUSTMENT);
		switch (GetPrefLong(PREF_RECEIPT)) {
		case 2:
			(*messH)->sound = NOTIFY_SOUND;
			GenerateReceipt(messH, MDN_DISPLAYED,
					MDN_DISPLAYED_LOCAL,
					MessOptIsSet(messH,
						     OPT_AUTO_OPENED) ?
					MDN_AUTO_ACTION : MDN_MAN_ACTION,
					MDN_AUTO_SENT);
			ClearMessOpt(messH, OPT_RECEIPT);
			break;
		case 1:
			ClearMessOpt(messH, OPT_RECEIPT);
			break;
		case 0:
			AddNotifyControls(messH);
			result = true;
			break;
		}
	}
	return result;
}

/************************************************************************
 * function - purpose
 ************************************************************************/
void AddNotifyControls(MessHandle messH)
{
	ControlHandle blah;
	MyWindowPtr messWin = (*messH)->win;
	WindowPtr messWinWP = GetMyWindowWindowPtr(messWin);
	Rect r;
	Str255 s;

	SetRect(&r, 2, 2, 4, 4);
	if (blah = GetNewControlSmall(NOTIFY_CNTL, messWinWP)) {
		(*messH)->sound = NOTIFY_SOUND;
		SetControlReference(blah, NOTIFY_CNTL);
		if (gGoodAppearance)
			EmbedControl(blah, messWin->topMarginCntl);

		// add message
		if (blah =
		    NewControlSmall(messWinWP, &r, "", False, 0, 0, 1,
				    kControlStaticTextProc, MDN_REQUEST)) {
			SetTextControlText(blah,
					   GetRString(s, MDN_REQUEST),
					   nil);
			if (gGoodAppearance)
				EmbedControl(blah, messWin->topMarginCntl);
		}

		// add separator
		if (blah =
		    NewControlSmall(messWinWP, &r, "\p", True, 0, 0, 0,
				    kControlSeparatorLineProc,
				    MDN_REQUEST + 1))
			if (gGoodAppearance)
				EmbedControl(blah, messWin->topMarginCntl);

		PlaceNotifyControls(messWin);

	}
}

/************************************************************************
 * function - purpose
 ************************************************************************/
void PlaceNotifyControls(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Rect r, notifyR, rWin;
	ControlHandle blah;

	if (blah = FindControlByRefCon(win, NOTIFY_CNTL)) {
		ButtonFit(blah);
		GetWindowPortBounds(winWP, &rWin);
		MoveMyCntl(blah, rWin.right - INSET - ControlWi(blah),
			   win->topMargin - ControlHi(blah) -
			   MAX_APPEAR_RIM - 2 -
			   CARBON_NOTIFY_BUTTON_V_ADJUSTMENT, 0, 0);
		ShowMyControl(blah);
		GetControlBounds(blah, &notifyR);

		// add message
		r = notifyR;
		r.right = r.left - INSET;
		r.left = INSET;
		InsetRect(&r, 0, MAX_APPEAR_RIM - 1);
		if (blah = FindControlByRefCon(win, MDN_REQUEST)) {
			MySetCntlRect(blah, &r);
			ShowMyControl(blah);
		}

		// add separator
		r = notifyR;
		SetRect(&r, INSET, r.top, rWin.right - INSET, r.top + 3);
		OffsetRect(&r, 0, -5);
		if (blah = FindControlByRefCon(win, MDN_REQUEST + 1)) {
			MySetCntlRect(blah, &r);
			ShowMyControl(blah);
		}
	}
}

/************************************************************************
 * AddMessErrNote - add the error note to a message, if need be
 ************************************************************************/
void AddMessErrNote(MessHandle messH)
{
	Rect r, c;
	ControlHandle cntl;
	mesgErrorHandle mesgErrH;
	MyWindowPtr messWin = (*messH)->win;
	WindowPtr messWinWP = GetMyWindowWindowPtr(messWin);

	/* if this message has a mesgErrH or the toc status is x, add note */
	if ((mesgErrH = (*(*messH)->tocH)->sums[(*messH)->sumNum].mesgErrH)
	    || ((*(*messH)->tocH)->sums[(*messH)->sumNum].state ==
		MESG_ERR)) {
		// make divider
		SetRect(&r, -REAL_BIG, messWin->topMargin - 1, REAL_BIG,
			messWin->topMargin + 1);
		cntl =
		    NewControlSmall(messWinWP, &r, "", True, 0, 0, 0,
				    kControlSeparatorLineProc, nil);
		EmbedControl(cntl, messWin->topMarginCntl);

		// add to topmargin
		SetTopMargin(messWin,
			     messWin->topMargin +
			     GetMesgErrorsHeight(messWin));
		GetMesgErrorsRect(messWin, &r);

		// add separator
		SetRect(&c, INSET, r.top, messWin->contR.right - INSET, r.top + 4);	// (jp) 2-6-01 Bug 3637  bottom used to be r.top+3
		OffsetRect(&c, 0, -5);
		if (cntl =
		    NewControlSmall(messWinWP, &c, "\p", True, 0, 0, 0,
				    kControlSeparatorLineProc,
				    mcMesgErrSep))
			EmbedControl(cntl, messWin->topMarginCntl);

		// add icon
		r.top += 3;
		SetRect(&c, 0, 0, 16, 16);
		CenterRectIn(&c, &r);
		cntl =
		    NewControlSmall(messWinWP, &c, "", True, MESS_ERR_ICON,
				    0, 0, kControlIconSuiteNoTrackProc,
				    mcMesgErrors);
		if (cntl && GetControlDataHandle(cntl)) {
			(*messH)->sound = NOTIFY_SOUND;
			if (gGoodAppearance)
				EmbedControl(cntl, messWin->topMarginCntl);
		}

		// add message
		r.left = r.right + INSET;
		r.right = messWin->contR.right - INSET;
		if (cntl =
		    NewControlSmall(messWinWP, &r, "", False, 0, 0, 1,
				    kControlStaticTextProc,
				    mcMesgErrText)) {
			/* try mesgErrH first. if not, use generic */
			if (mesgErrH) {
				SetTextControlText(cntl,
						   LDRef(mesgErrH)->
						   errorStr, nil);
				UL(mesgErrH);
			} else {
				Str255 item;
				SetTextControlText(cntl,
						   GetRString(item,
							      UNKNOWN_ERR_TEXT),
						   nil);
			}
			LetsGetSmall(cntl);
			ShowMyControl(cntl);
			if (gGoodAppearance)
				EmbedControl(cntl, messWin->topMarginCntl);
		}
	}
}

/************************************************************************
 * PlaceMessErrNote - move the error note to the right spot
 ************************************************************************/
void PlaceMessErrNote(MessHandle messH)
{
	ControlHandle cntl;
	MyWindowPtr win = (*messH)->win;
	Rect r;
	long junk;
	short h, dH;


	if (cntl = FindControlByRefCon(win, mcMesgErrors)) {
		GetMesgErrorsRect(win, &r);
		r.left = r.right + INSET;
		r.right = win->contR.right - INSET;
		r.top -= 1;
		if (cntl = FindControlByRefCon(win, mcMesgErrText)) {
			HideControl(cntl);
			MySetCntlRect(cntl, &r);
			SetBevelStyle(cntl, 0);
			GetControlData(cntl, 0,
				       kControlStaticTextTextHeightTag,
				       sizeof(short), (void *) &h, &junk);
			if (h > RectHi(r)) {
				SetBevelStyle(cntl, condense);
				GetControlData(cntl, 0,
					       kControlStaticTextTextHeightTag,
					       sizeof(short), (void *) &h,
					       &junk);
			}
			dH = (h - RectHi(r)) / 2;
			if (dH < 0) {
				OffsetRect(&r, 0, -dH);
				r.bottom = r.top + h;
				MySetCntlRect(cntl, &r);
			}
			ShowControl(cntl);
		}
	}
}


/**********************************************************************
 * MessIBarUpdate - update the icon bar
 **********************************************************************/
void MessIBarUpdate(MessHandle messH)
{
	MControlsEnum cIndex;
	ControlHandle blah;
	long value;
	Boolean on = MessOnPOPD(POPD_ID, messH);
	Boolean lmos = PrefIsSet(PREF_LMOS);
	Boolean fetch = on && MessOnPOPD(FETCH_ID, messH);
	Boolean del = on && (fetch && !lmos
			     || MessOnPOPD(DELETE_ID, messH));

	for (cIndex = mcWrite; cIndex <= mcTrash; cIndex++)
		if (blah = FindControlByRefCon((*messH)->win, cIndex)) {
			value = GetControlValue(blah);
			switch (cIndex) {
			case mcWrite:
				value = MessOptIsSet(messH, OPT_WRITE);
				break;
			case mcBlahBlah:
				value =
				    MessFlagIsSet(messH, FLAG_SHOW_ALL);
				break;
			case mcFetch:
				value = fetch;
				if (!on
				    || !MessFlagIsSet(messH, FLAG_SKIPPED))
					HideControl(blah);
				(*messH)->hasFetchIcon =
				    IsControlVisible(blah);
				break;
			case mcGetGraphics:
				if (!DisplayGetGraphics((*messH)->win))
					HideControl(blah);
				else
					ShowControl(blah);
				break;
			case mcFixed:
				value =
				    MessFlagIsSet(messH, FLAG_FIXED_WIDTH);
				break;
			case mcTrash:
				value = del;
				if (!on)
					HideControl(blah);
				else {
					if (!lmos && fetch)
						DeactivateControl(blah);
					else
						ActivateControl(blah);
				}
				(*messH)->hasDelIcon =
				    IsControlVisible(blah);
				break;
			}
			SetControlValue(blah, value);
		}
}


#ifdef NEVER
/**********************************************************************
 * AlignHeaders - align the header names in a message
 **********************************************************************/
void AlignHeaders(MessHandle messH)
{
	short len = 0;
	short thisLen;
	Handle text;
	long start, colon, stop, end;
	short nPara = 0;
	PETEParaInfo pinfo;

	PETEGetRawText(PETE, TheBody, &text);

	end = SumOf(messH)->bodyOffset - (*messH)->weeded;

	/*
	 * insert all the paragraphs, and store the width of the header name
	 * in the paragraph indent, and keep track of the max width
	 */
	for (start = 0; start < end; start = stop + 1) {
		for (colon = start; colon < end && (*text)[colon] != ':';
		     colon++);
		for (stop = colon; stop < end; stop++)
			if ((*text)[stop] == '\015'
			    && !IsWhite((*text)[stop + 1]))
				break;
		thisLen = TextWidth(LDRef(text), start, colon - start + 1);
		UL(text);
		if (len < thisLen)
			len = thisLen;
		PeteEnsureBreak(TheBody, stop + 1);
		pinfo.indent = thisLen;
		PETESetParaInfo(PETE, TheBody, nPara++, &pinfo,
				peIndentValid);
	}

	/*
	 * Now, go back through the paragraphs and set the margins based
	 * on the maximum length and the header length stored in the indent
	 */
	pinfo.tabHandle = nil;
	len += 2 * FontWidth;
	if (thisLen = FontWidth * GetRLong(ALIGN_LIMIT))
		len = MIN(len, thisLen);

	(*PeteExtra(TheBody))->headers = nPara;

	while (nPara--) {
		PETEGetParaInfo(PETE, TheBody, nPara, &pinfo);
		pinfo.startMargin = (len - pinfo.indent) << 16;
		pinfo.indent = (-pinfo.indent - FontWidth) << 16;
		PETESetParaInfo(PETE, TheBody, nPara, &pinfo,
				peIndentValid | peStartMarginValid);
	}
}
#endif
/**********************************************************************
 * ExportHTMLSum - export html from a message summary
 **********************************************************************/
OSErr ExportHTMLSum(TOCHandle tocH, short sumNum)
{
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	MyWindowPtr win = nil;
	OSErr err;

	// if this is an IMAP message, make sure it's been downloaded.
	if ((*tocH)->imapTOC)
		EnsureMsgDownloaded(tocH, sumNum, false);

	if (!messH) {
		win = GetAMessage(tocH, sumNum, nil, nil, false);
		if (!win)
			return (MemError());
		messH = Win2MessH(win);
	}

	err = ExportHTML(messH);

	if (win)
		CloseMyWindow(GetMyWindowWindowPtr(win));
	return (err);
}

#ifdef USE_FANCY_HTML_EXPORT
/**********************************************************************
 * 
 **********************************************************************/
OSErr ExportHTML(MessHandle messH)
{
	OSErr err;
	Str31 html;
	Str31 testMe;
	Boolean neg;
	short inHTML = 0;
	long offset, startOffset, endOffset;
	FSSpec spec;
	short refN;
	UHandle cache;
	long len = 0;
	long grandLen;
	Byte less = '<';

	Zero(spec);

	if (!(err = CacheMessage((*messH)->tocH, (*messH)->sumNum))) {
		cache = SumOf(messH)->cache;
		HNoPurge(cache);
		ComposeString(html, "\p%r>",
			      HTMLDirectiveStrn + htmlXHTMLDir);
		if (!
		    (err =
		     NewTempExtSpec(Root.vRef, nil, HTML_SUFFIX, &spec))
		    && !(err =
			 FSpCreate(&spec, 0L, 'TEXT', smSystemScript))
		    && !(err = FSpOpenDF(&spec, fsRdWrPerm, &refN))) {
			LDRef(cache);
			grandLen = GetHandleSize(cache);
			startOffset = grandLen;

			for (offset =
			     SearchPtrHandle(&less, 1, cache, 0, True,
					     False, nil); offset > 0;
			     offset =
			     SearchPtrHandle(&less, 1, cache, offset + 1,
					     True, False, nil)) {
				/*
				 * what is the word we're looking at?
				 */
				if ((*cache)[offset + 1] == '/') {
					neg = True;
					MakePStr(testMe,
						 *cache + offset + 2,
						 *html);
				} else {
					neg = False;
					MakePStr(testMe,
						 *cache + offset + 1,
						 *html);
				}

				/*
				 * no-brainers first
				 */
				if (!StringSame(html, testMe))
					continue;	//anything that's not html is insignificant
				if (neg && !inHTML)
					continue;	//hmmm.  </html> in free text.  No likee.
				if (!neg && inHTML) {
					inHTML++;
					continue;
				}	// more deeply nested html

				/*
				 * ok, first let's look at <html>
				 */
				if (!neg) {
					if (!inHTML)
						startOffset = offset;	//html begins
					inHTML++;
				}

				/*
				 * ok, now the fun one: </html>
				 */
				else {
					inHTML--;
					if (!inHTML) {
						endOffset =
						    offset + *html + 3;
						endOffset =
						    MIN(endOffset,
							grandLen);
						if (startOffset <
						    endOffset) {
							len =
							    endOffset -
							    startOffset;
							err =
							    HTMLWriteForBrowser
							    (cache,
							     startOffset,
							     len, refN);
							if (err)
								goto out;
						}
						offset = endOffset;
					}
				}
			}
		      out:
			if (!err && inHTML) {
				len = grandLen - startOffset;
				err =
				    HTMLWriteForBrowser(cache, startOffset,
							len, refN);
			}
			UL(cache);
			FSClose(refN);
		}
		HPurge(cache);
	}

	// an error occurred if we didn't actually write anything to the temp file -JDB 290499
	if (!err && (len == 0))
		err = eofErr;

	/*
	 * Did we get some?
	 */

	if (!err) {
		AliasHandle browser = nil;
		FSSpec browserSpec;
		Boolean wasChanged;

		if (HaveTheDiseaseCalledOSX()
		    && !PrefIsSet(PREF_USE_OWN_URL_HELPERS))
			err = OpenOtherDoc(&spec, false, false, nil);
		else {
			if (!
			    (err =
			     FindURLApp(GetRString(html, HTTP), &browser,
					kWildCardOK)))
				if (!
				    (err =
				     ResolveAlias(nil, browser,
						  &browserSpec,
						  &wasChanged)))
					err =
					    OpenDocWith(&spec,
							&browserSpec,
							false);
			ZapHandle(browser);
		}
	}

	if (err)
		FSpDelete(&spec);

	if (err && err != userCancelled && err != 1)
		return (WarnUser(SAVING_HTML, err));
	return err;
}
#else
/**********************************************************************
 * 
 **********************************************************************/
OSErr ExportHTML(MessHandle messH)
{
	OSErr err;
	Str31 html;
	Str31 testMe;
	Boolean neg;
	short inHTML = 0;
	long offset, startOffset, endOffset;
	FSSpec spec;
	short refN;
	UHandle cache;
	long len;
	long grandLen;
	Byte less = '<';

	Zero(spec);

	if (!(err = CacheMessage((*messH)->tocH, (*messH)->sumNum))) {
		cache = SumOf(messH)->cache;
		HNoPurge(cache);
		ComposeString(html, "\p%r>", HTMLTagsStrn + htmlTag);
		if (!
		    (err =
		     NewTempExtSpec(Root.vRef, nil, HTML_SUFFIX, &spec))
		    && !(err =
			 FSpCreate(&spec, CREATOR, 'TEXT', smSystemScript))
		    && !(err = FSpOpenDF(&spec, fsRdWrPerm, &refN))) {
			LDRef(cache);
			grandLen = GetHandleSize(cache);
			startOffset = grandLen;

			for (offset =
			     SearchPtrHandle(&less, 1, cache, 0, True,
					     False, nil); offset > 0;
			     offset =
			     SearchPtrHandle(&less, 1, cache, offset + 1,
					     True, False, nil)) {
				/*
				 * what is the word we're looking at?
				 */
				if ((*cache)[offset + 1] == '/') {
					neg = True;
					MakePStr(testMe,
						 *cache + offset + 2,
						 *html);
				} else {
					neg = False;
					MakePStr(testMe,
						 *cache + offset + 1,
						 *html);
				}

				/*
				 * no-brainers first
				 */
				if (!StringSame(html, testMe))
					continue;	//anything that's not html is insignificant
				if (neg && !inHTML)
					continue;	//hmmm.  </html> in free text.  No likee.
				if (!neg && inHTML) {
					inHTML++;
					continue;
				}	// more deeply nested html

				/*
				 * ok, first let's look at <html>
				 */
				if (!neg) {
					if (!inHTML)
						startOffset = offset;	//html begins
					inHTML++;
				}

				/*
				 * ok, now the fun one: </html>
				 */
				else {
					inHTML--;
					if (!inHTML) {
						endOffset =
						    offset + *html + 3;
						endOffset =
						    MIN(endOffset,
							grandLen);
						if (startOffset <
						    endOffset) {
							len =
							    endOffset -
							    startOffset;
							err =
							    AWrite(refN,
								   &len,
								   *cache +
								   startOffset);
							if (err)
								goto out;
						}
						offset = endOffset;
					}
				}
			}
		      out:
			if (!err && inHTML) {
				len = grandLen - startOffset;
				err =
				    AWrite(refN, &len,
					   *cache + startOffset);
			}
			UL(cache);
			FSClose(refN);
		}
		HPurge(cache);
	}

	/*
	 * Did we get some?
	 */
	if (!err) {
		AliasHandle browser = nil;
		FSSpec browserSpec;
		Boolean wasChanged;

		if (!
		    (err =
		     FindURLApp(GetRString(html, HTTP), &browser,
				kWildCardOK)))
			if (!
			    (err =
			     ResolveAlias(nil, browser, &browserSpec,
					  &wasChanged)))
				err = OpenDocWith(&spec, &browserSpec);
		ZapHandle(browser);
	}

	if (err)
		FSpDelete(&spec);

	if (err && err != userCancelled && err != 1)
		return (WarnUser(SAVING_HTML, err));
}
#endif

/**********************************************************************
 * HiliteOddReply - hilite reply-to: and friends in color
 **********************************************************************/
void HiliteOddReply(MessHandle messH)
{
	Str255 scratch, scratch2;
	UHandle text;
	short r;
	long size;
	HeadSpec hs, fromHS;
	PETETextStyle ts;

	PETEGetRawText(PETE, TheBody, &text);
	LDRef(text);
	size = GetHandleSize(text);

	for (r = 1; *GetRString(scratch, ReplyStrn + r); r++) {
		if (EqualStrRes(scratch, HEADER_STRN + FROM_HEAD))
			break;
		if (CompHeadFindStr(messH, scratch, &hs)) {
			if (CompHeadFindStr
			    (messH,
			     GetRString(scratch, HEADER_STRN + FROM_HEAD),
			     &fromHS)) {
				MakePStr(scratch, (*text) + hs.value,
					 hs.stop - hs.value);
				MakePStr(scratch2, (*text) + fromHS.value,
					 fromHS.stop - fromHS.value);
				if (SameAddressStr(scratch, scratch2))
					continue;
			}
			ts.tsLabel = pReplyLabel;
			PETESetTextStyle(PETE, TheBody, hs.start, hs.stop,
					 &ts, peLabelValid);
			PeteCleanList(TheBody);
			if (!MessOptIsSet(messH, OPT_WEIRD_REPLY))
				SetMessOpt(messH, OPT_WEIRD_REPLY);
		}
	}

	UL(text);
}

/************************************************************************
 * MessUpdate - update a message window
 ************************************************************************/
void MessUpdate(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);

	if (IsWindowVisible(winWP)) {
		MessHandle messH =
		    (MessHandle) GetMyWindowPrivateData(win);

		if (IsWindowVisible(winWP) && (*messH)->sound) {
			Str255 soundName;
			PlayNamedSound(GetRString
				       (soundName, (*messH)->sound));
			(*messH)->sound = 0;
		}
	}
}


#ifdef TWO
/**********************************************************************
 * MessBuildDragRgn - build the drag region for a message
 **********************************************************************/
RgnHandle MessBuildDragRgn(MessHandle messH)
{
	WindowPtr messWinWP = GetMyWindowWindowPtr((*messH)->win);
	RgnHandle rgn = NewRgn();

	if (rgn) {
		GetWindowRegion(messWinWP, kWindowTitleProxyIconRgn, rgn);
		LocalizeRgn(rgn);
	}
	return (rgn);
}

/**********************************************************************
 * GetServerRect - get rectangle for a server icon
 **********************************************************************/
Boolean GetServerRect(MyWindowPtr win, short which, Rect * r)
{
	CGrafPtr winPort = GetMyWindowCGrafPtr(win);
	short wi;
	Rect rPort;

	GetBlahRect(win, 0, r);
	wi = r->right - r->left;
	GetPortBounds(winPort, &rPort);
	r->right = rPort.right - (2 - which) * (INSET + wi) - INSET;
	r->left = r->right - wi;
	return (True);
}
#endif

/**********************************************************************
 * GetMesgErrorsHeight - get rectangle for a transport errors icon
 **********************************************************************/

short GetMesgErrorsHeight(MyWindowPtr win)
{
	short height, fs;
	FontInfo info;

	fs = GetPortTextSize(GetQDGlobalsThePort());
	TextSize(10);
	GetFontInfo(&info);
	TextSize(fs);
	height = (info.ascent + info.descent + info.leading) * 2;
	return ((height < MESG_ERR_WIDTH) ? MESG_ERR_WIDTH : height);
}

/**********************************************************************
 * GetMesgErrorsRect - get rectangle for a transport errors icon
 **********************************************************************/
Boolean GetMesgErrorsRect(MyWindowPtr win, Rect * r)
{
	CGrafPtr winPort = GetMyWindowCGrafPtr(win);
	Rect rPort;

	GetPortBounds(winPort, &rPort);
	r->left = rPort.left + INSET;
	r->right = r->left + MESG_ERR_WIDTH;
	r->bottom = win->topMargin - MAX_APPEAR_RIM;
	r->top = r->bottom - MESG_ERR_WIDTH + 6;
	return (True);
}

/************************************************************************
 * MessFocus - switch txe's in a message window
 ************************************************************************/
void MessFocus(MessHandle messH, PETEHandle pte)
{
	MyWindowPtr win = (*messH)->win;
	Boolean wasSub = win->pte == (*messH)->subPTE;

	PeteFocus(win, pte, True);
	win->ro = win->pte == TheBody && !MessOptIsSet(messH, OPT_WRITE);

	if (wasSub && win->pte != (*messH)->subPTE)
		MessSaveSub(messH);
	else
		win->showInsert = NOOP;
}

/**********************************************************************
 * MessSaveSub - save the subject of a message
 **********************************************************************/
OSErr MessSaveSub(MessHandle messH)
{
	Str255 newSubj;

	if (MessFlagIsSet(messH, FLAG_UTF8) && HasUnicode()) {
		OSErr err;
		long iUsed, oUsed;

		err =
		    PeteGetUTF8Text((*messH)->subPTE, 0, LONG_MAX, &iUsed,
				    &(newSubj[1]), sizeof(newSubj) - 1,
				    &oUsed);
		if (err && err != kTECOutputBufferFullStatus)
			return err;
		newSubj[0] = oUsed;
	} else {
		PeteSString(newSubj, (*messH)->subPTE);
	}
	SetSubject((*messH)->tocH, (*messH)->sumNum, newSubj);
	PETEMarkDocDirty(PETE, (*messH)->subPTE, False);
	if (PeteIsDirtyList((*messH)->win->pteList))
		(*messH)->win->isDirty = True;
	else
		(*messH)->win->isDirty = False;
	return noErr;
}

/************************************************************************
 * MessDidResize - resize a message window
 ************************************************************************/
void MessDidResize(MyWindowPtr win, Rect * oldContR)
{
	MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
	Rect view;
	Rect paneR;
	PETEHandle bodTeh = TheBody;
	PETEHandle subTeh = (*messH)->subPTE;
	ControlHandle blah, errorCntl = nil;

	if (!subTeh)
		TextDidResize(win, oldContR);
	else {
		view = win->contR;
		PeteDidResize(bodTeh, &view);
	}
	PlaceMessErrNote(messH);

	if (!oldContR
	    || oldContR->right - oldContR->left !=
	    win->contR.right - win->contR.left) {
		SetRect(&view, 0, 0, REAL_BIG / 2, win->topMargin - 3);
		InvalWindowRect(GetMyWindowWindowPtr(win), &view);

		/*
		 * the icon bar
		 */
		if (GetBlahRect(win, 0, &view)) {
			if (blah = FindControlByRefCon(win, mcWrite))
				MySetCntlRect(blah, &view);

			GetBlahRect(win, 1, &view);
			if (blah = FindControlByRefCon(win, mcBlahBlah))
				MySetCntlRect(blah, &view);

			GetBlahRect(win, 2, &view);
			if (blah = FindControlByRefCon(win, mcFixed))
				MySetCntlRect(blah, &view);

			GetServerRect(win, 1, &view);
			if (blah = FindControlByRefCon(win, mcFetch))
				MySetCntlRect(blah, &view);

			GetServerRect(win, 1, &view);
			if (blah = FindControlByRefCon(win, mcGetGraphics))
				MySetCntlRect(blah, &view);

			GetServerRect(win, 2, &view);
			if (blah = FindControlByRefCon(win, mcTrash))
				MySetCntlRect(blah, &view);

			if (blah = FindControlByRefCon(win, NOTIFY_CNTL)) {
				PlaceNotifyControls(win);
			}

			MessSubjRect(win, &view);
			paneR = view;
			if (blah =
			    FindControlByRefCon(win, (uLong) subTeh)) {
				InsetRect(&paneR, -MAX_APPEAR_RIM,
					  -MAX_APPEAR_RIM);
				SetControlVisibility(blah, false, false);
				MySetCntlRect(blah, &paneR);
				SetControlVisibility(blah, true, false);
			}
			PETESizeDoc(PETE, subTeh, RectWi(view),
				    RectHi(view));
			PETEChangeDocWidth(PETE, win->pte, -1, True);
		}
	}
}

/************************************************************************
 * MessSubjRect - find the rect for the subject teh
 ************************************************************************/
void MessSubjRect(MyWindowPtr win, Rect * r)
{
	MessHandle messH = Win2MessH(win);
	short hi = ONE_LINE_HI(win);
	Rect pr, br;

	GetPriorityRect(win, &pr);
	GetBlahRect(win, FontIsFixed ? 1 : 2, &br);
	pr.right = br.right;

	*r = win->contR;
	r->left += pr.right + INSET + win->hPitch;
	r->top = pr.top + (RectHi(pr) - hi - 1) / 2;
	r->bottom = r->top + hi;
#ifdef TWO
	GetServerRect(win, 1, &br);
	r->right = br.left - INSET;
#endif
}

/************************************************************************
 * GetBlahRect - get the rect for the blah blah icon
 ************************************************************************/
Boolean GetBlahRect(MyWindowPtr win, short which, Rect * r)
{
	if (!GetPriorityRect(win, r))
		return (False);
	OffsetRect(r, (which + 1) * (RectWi(*r) + INSET), 0);
	return (True);
}

/************************************************************************
 * MessCursor - set the cursor properly
 ************************************************************************/
void MessCursor(Point mouse)
{
	MyWindowPtr win = GetWindowMyWindowPtr(FrontWindow_());
	MessHandle messH = Win2MessH(win);

	if (!PeteCursorList(win->pteList, mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * GetPriorityRect - where does the priority thing belong?
 ************************************************************************/
Boolean GetPriorityRect(MyWindowPtr win, Rect * pr)
{
	if (!win->topMargin) {
		SetRect(pr, 0, 0, 0, 0);
		return (False);
	} else {
		pr->left = INSET;
		pr->top = 2 * MAX_APPEAR_RIM + 2;
		pr->bottom = pr->top + 22;
		pr->right = pr->left + 22;
		return (True);
	}
}

/************************************************************************
 * DrawPriority - draw the message priority
 ************************************************************************/
void DrawPriority(Rect * pr, short p)
{
	WindowPtr theWindow = GetWindowFromPort(GetQDGlobalsThePort());
	Rect ir;

	if (GetWindowKind(theWindow) != CBOX_WIN
	    && GetWindowKind(theWindow) != MBOX_WIN) {
		ir = *pr;
		InsetRect(&ir, -2, -2);
		EraseRect(&ir);
		FrameRect(&ir);
		MoveTo(ir.left + 1, ir.bottom);
		Line(ir.right - ir.left - 1, 0);
		Line(0, ir.top - ir.bottom + 1);
	}
	p = Prior2Display(p);
	SetRect(&ir, 0, 0, 16, 16);
	CenterRectIn(&ir, pr);
	if (p == pymRaise)
		PlotIconID(&ir, atAbsoluteCenter, ttNone,
			   RAISE_PRIOR_SICN);
	else if (p == pymLower)
		PlotIconID(&ir, atAbsoluteCenter, ttNone,
			   LOWER_PRIOR_SICN);
	else
		PlotIconID(&ir, atAbsoluteCenter, ttNone,
			   PRIOR_SICN_BASE + p - 1);
}


/************************************************************************
 * SetMessTable - set the xlit table for a message
 ************************************************************************/
void SetMessTable(TOCHandle tocH, short sumNum, short newId)
{
	if ((*tocH)->sums[sumNum].tableId != newId) {
		(*tocH)->sums[sumNum].tableId = newId;
		TOCSetDirty(tocH, true);
		if ((*tocH)->previewID == (*tocH)->sums[sumNum].serialNum)
			(*tocH)->previewID = 0;
		if ((*tocH)->which != OUT && (*tocH)->sums[sumNum].messH)
			ReopenMessage((*(*tocH)->sums[sumNum].messH)->win);
	}
}

/************************************************************************
 * MessZoomSize - figure the size of a message window
 ************************************************************************/
void MessZoomSize(MyWindowPtr win, Rect * zoom)
{
	long wi, hi;
	PETEHandle pte = (*Win2MessH(win))->bodyPTE;
	PETEDocInfo info;

	wi = MessWi(win);
	zoom->right = zoom->left + MIN(zoom->right - zoom->left, wi);

	if (PeteIsValid(pte)) {
		//PETEGetDocInfo(PETE,pte,&info);
		wi = zoom->right - zoom->left - PETE_SCROLLY_DIFFERENCE +
		    9;
		PETEChangeDocWidth(PETE, pte, wi, True);
		PETEGetDocInfo(PETE, pte, &info);

		hi = info.docHeight + win->topMargin + win->vPitch;
		if (hi < zoom->bottom - zoom->top)
			zoom->bottom = zoom->top + hi;
	}
}

/**********************************************************************
 * MessWi - how wide is a window?
 **********************************************************************/
short MessWi(MyWindowPtr win)
{
	short wi;

	if (!(wi = GetPrefLong(PREF_MWIDTH)))
		wi = GetRLong(DEF_MWIDTH);
	wi *= win->hPitch;
	wi += PETE_SCROLLY_DIFFERENCE;
	return (wi);
}


/************************************************************************
 * MessApp1 - scroll the message, not the subject
 ************************************************************************/
Boolean MessApp1(MyWindowPtr win, EventRecord * event)
{
	event->what = keyDown;
	PeteEdit(win, (*Win2MessH(win))->bodyPTE, peeEvent,
		 (void *) event);
	return (True);
}

/************************************************************************
 * IncrementQuoteLevel - adjust the quote level in a message
 ************************************************************************/
OSErr IncrementQuoteLevel(PETEHandle pte, long start, long stop,
			  short increment)
{
	OSErr err;
	short fromPara, toPara;
	PETEParaInfo pInfo;
	PETEStyleEntry pse;
	short level;

	// I'm only going to tell you this once...
	pse.psStyle.textStyle.tsLabel = pQuoteLabel;
	Zero(pInfo);

	// avoid messy redraw
	PeteCalcOff(pte);

	// Expand offsets to para boundaries
	start = stop = -1;	// use selection
	PeteParaRange(pte, &start, &stop);
	PeteParaConvert(pte, start, stop);

	// Undo to para boundaries
	PetePrepareUndo(pte, peUndoStyleAndPara, start, stop, nil, nil);

	// Find the start and stop points
	fromPara = PeteParaAt(pte, start);
	toPara = PeteParaAt(pte, stop - 1);
	toPara = MAX(toPara, fromPara);	// darn fiddly end conditions

	// Ok, do some work
	for (; fromPara <= toPara; fromPara++) {
		// get current quote level
		if (err = PETEGetParaInfo(PETE, pte, fromPara, &pInfo))
			break;

		// bump it, but don't allow negative or > 15
		level = pInfo.quoteLevel + increment;
		level = MIN(level, 15);
		level = MAX(level, 0);
		pInfo.quoteLevel = level;

		// and set it
		if (err =
		    PETESetParaInfo(PETE, pte, fromPara, &pInfo,
				    peQuoteLevelValid))
			break;

		// and give it the quote label
		if (level)
			PETESetTextStyle(PETE, pte, pInfo.paraOffset,
					 pInfo.paraOffset +
					 pInfo.paraLength,
					 &pse.psStyle.textStyle,
					 pQuoteLabel << kPETELabelShift);
	}

	// And finish it off with a little undo
	if (err)
		PeteKillUndo(pte);
	else
		PeteFinishUndo(pte, peUndoStyleAndPara, start, stop);

	// now draw
	PeteCalcOn(pte);

	return (err);
}

#ifdef USECMM_BAD
/************************************************************************************************
MessBuildCMenu - build a list of context-specific items to be added to the contextual menu when
the context click occurs over a recevied message window.  specCMenuInfo does not point to a valid
handle upon calling the function. The handle-style array it points to needs to be allocated
herein.
************************************************************************************************/
OSStatus MessBuildCMenu(MyWindowPtr win, EventRecord * contextEvent,
			MenuHandle contextMenu)
{
	short numItems;		//number of items to add put into the list
	static CMenuInfo locCMInfo[] =	//items to put into the list
	{ MESSAGE_MENU, MESSAGE_REPLY_ITEM,	//"reply" item
		MESSAGE_MENU, MESSAGE_FORWARD_ITEM,	//"forward" item
		MESSAGE_MENU, MESSAGE_DELETE_ITEM,	//"delete" item
		CONTEXT_MENU, CONTEXT_TERM_ITEM
	};			//item to mark end of array

	//count the number of items to add
	for (numItems = 0;
	     !(locCMInfo[numItems].srcMenuID == CONTEXT_MENU
	       && locCMInfo[numItems].srcMenuItem == CONTEXT_TERM_ITEM);
	     numItems++);

	//allocate memory for as handle-style array
	*specCMenuInfo =
	    (CMenuInfoHndl) NewHandle(sizeof(CMenuInfo) * (numItems + 1));
	if (!(*specCMenuInfo))
		return (MemError());

	//copy data into the array
	BlockMove(locCMInfo, **specCMenuInfo,
		  sizeof(CMenuInfo) * (numItems + 1));

	return (noErr);
}
#endif
