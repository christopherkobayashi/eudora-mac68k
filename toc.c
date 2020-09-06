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

#define FILE_NUM 72
#include "toc.h"

#pragma segment TOC

OSErr WriteRForkTOC(TOCHandle tocH);
Boolean HasExternalTOC(FSSpecPtr spec);
OSErr PeekRTOC(FSSpecPtr spec, TOCType * tocPart);
void FixBoxUnread(TOCHandle tocH);
TOCHandle FixErrantTOC(FSSpecPtr spec, TOCHandle tocH, short why);
Boolean WantRebuildTOC(UPtr boxName, OSErr why, Boolean isIMAP);
OSErr PeekDTOC(FSSpecPtr spec, TOCType * tocPart);
void CheckStringLen(UPtr s, char maxLen, short fillLen);
#define TOCSizeShouldBe(tocH) (sizeof(TOCType)+MAX(0,(*(tocH))->count-1)*sizeof(MSumType))
#define TOCCountShouldBe(tocH) (1+(GetHandleSize_(tocH)-sizeof(TOCType))/sizeof(MSumType))
/************************************************************************
 * TOCBySpec - take a spec, return a TOC
 ************************************************************************/
TOCHandle TOCBySpec(FSSpecPtr spec)
{
	if (!GetMailbox(spec, False))
		return (FindTOC(spec));
	return (nil);
}

/************************************************************************
 * GetTOCByFSS - open a toc from an FSS
 ************************************************************************/
short GetTOCByFSS(FSSpecPtr specPtr, TOCHandle * tocH)
{
	*tocH = TOCBySpec(specPtr);
	return (*tocH ? noErr : 1);
}

/**********************************************************************
 * KillTOC - kill the toc for a file
 **********************************************************************/
OSErr KillTOC(short refN, FSSpecPtr spec)
{
	Boolean sane;
	OSErr err;
	Handle r;
	OSType type;
	Str255 name;
	short id;
	short oldResF = CurResFile();

	/*
	 * if the resource fork is bad or we can't open it, just remove it
	 */
	if (!refN
	    && (FSpRFSane(spec, &sane) || !sane
		|| -1 == (refN = FSpOpenResFile(spec, fsRdWrPerm))))
		err = FSpKillRFork(spec);
	else {
		/*
		 * delete the backup resource
		 */
		Zap1Resource(TOC_TYPE, 1002);

		if (PrefIsSet(PREF_NO_RF_TOC_BACKUP)) {
			/*
			 * remove old resource
			 */
			Zap1Resource(TOC_TYPE, 1001);
			err = ResError();
		} else {
			/*
			 * try to move the old resource
			 */
			SetResLoad(False);
			r = Get1Resource(TOC_TYPE, 1001);
			err = ResError();
			if (!r && !err)
				err = resNotFound;
			SetResLoad(True);
			if (!err) {
				GetResInfo(r, &id, &type, name);
				if (!(err = ResError())) {
					SetResInfo(r, 1002, name);
					if (spec && !(err = ResError()))
						err =
						    MyUpdateResFile(refN);
				}
			}
		}
		if (spec)
			CloseResFile(refN);

		/*
		 * if we couldn't delete the resource, kill the resource fork
		 */
		if (spec && err && err != resNotFound)
			err = FSpKillRFork(spec);
	}
	UseResFile(oldResF);
	return (err);
}

/**********************************************************************
 * CheckTOC - check a file for a table of contents, and build it if
 * necessary.
 * slated for removal
 **********************************************************************/
TOCHandle CheckTOC(FSSpecPtr spec)
{
	uLong box, res, file, mod;
	Boolean resource;
	OSErr err;

	if ((err = TOCDates(spec, &box, &res, &file)) && err != fnfErr)
		return (nil);

	/*
	 * Which one do we want?
	 */
	if (res && res >= file)	// resource exists and is newer
	{
		resource = True;
		mod = res;
	} else if (file)	// file exists and is newer
	{
		resource = False;
		mod = file;
	} else
		return (BuildTOC(spec));	// no dice

#ifdef NEVER
	/*
	 * Is it new enough?
	 */
	if (!PrefIsSet(PREF_IGNORE_OUTDATE) && box > mod)
		switch (AlertStr(NEW_TOC_ALRT, Caution, spec->name)) {
		case TOC_CREATE_NEW:
			return (RebuildTOC(spec, nil, resource));
			break;
		case CANCEL_ITEM:
		case TOC_CANCEL:
			return (nil);
			break;
		default:
			AFSpSetMod(spec, mod);
			break;
		}
#endif

	return (ReadTOC(spec, resource));
}

/**********************************************************************
 * TOCDates - get the dates off a mailbox
 **********************************************************************/
OSErr TOCDates(FSSpecPtr spec, uLong * box, uLong * res, uLong * file)
{
	FSSpec tocSpec;
	TOCType partTOC;
	OSErr err;
	long boxSize = FSpDFSize(spec);

	*box = AFSpGetMod(spec);

	Box2TOCSpec(spec, &tocSpec);
	*file = AFSpGetMod(&tocSpec);

	if (!(err = PeekRTOC(spec, &partTOC))) {
		*res = partTOC.writeDate;

		if (boxSize == partTOC.boxSize - 1)
			*res = *box;	// if the sizes match, forget the date

		if (!*res) {
			// somebody didn't fill in writeDate
			if (*file);	// we have a file-based toc.  Let's assume it has the good info
			else
				*res = *box - 1;	// no date in resource toc; might be bad
		}
	} else
		*res = 0;
	if (err && boxSize > 0)
		ComposeLogS(LOG_TOC | LOG_ALRT, nil, "\pTOCDates(%p): %d",
			    spec->name, err);
	return (err);
}

/************************************************************************
 * Box2TOCSpec - make a toc spec out of a mailbox spec
 ************************************************************************/
FSSpecPtr Box2TOCSpec(FSSpecPtr boxSpec, FSSpecPtr tocSpec)
{
	Str31 suffix;
	*tocSpec = *boxSpec;
	PCat(tocSpec->name, GetRString(suffix, TOC_SUFFIX));
	return (tocSpec);
}


/**********************************************************************
 * ReadTOC - read the toc file for a mailbox
 **********************************************************************/
TOCHandle ReadTOC(FSSpecPtr spec, Boolean resource)
{
	TOCHandle tocH = nil;
	OSErr insane = noErr;

	insane =
	    resource ? ReadRForkTOC(spec, &tocH) : ReadDForkTOC(spec,
								&tocH);

	if (tocH) {
		/*
		 * don't take these for granted...
		 */
		(*tocH)->durty = (*tocH)->reallyDirty = False;
		(*tocH)->mailbox.spec = *spec;
		(*tocH)->refN = 0;
		(*tocH)->win = nil;
		(*tocH)->volumeFree = 0;
		(*tocH)->previewID = 0;
		(*tocH)->previewPTE = 0;
		(*tocH)->lastSameTicks = 0;
		(*tocH)->mouseTicks = 0;
		(*tocH)->mouseSpot.h = (*tocH)->mouseSpot.v = 0;
		(*tocH)->userActive = 0;
		(*tocH)->drawerWin = 0;
#ifdef	IMAP
		LDRef(tocH);
		(*tocH)->imapTOC =
		    IsIMAPMailboxFileLo(spec, &((*tocH)->imapMBH)) ? 1 : 0;
		UL(tocH);
#endif
		/*
		 * and make sure it hasn't become special or unspecial
		 */
		(*tocH)->which = GetMailboxType(spec);

		/*
		 * make sure we don't have any leftovers
		 */
		CleanseTOC(tocH);

		/*
		 * check toc for reasonableness
		 */
		if (insane = InsaneTOC(tocH))
			return (FixErrantTOC(spec, tocH, insane));

		/*
		 * and sizes
		 */
		GetTOCK(tocH, &(*tocH)->usedK, &(*tocH)->totalK);
		(*tocH)->updateBoxSizes = true;

		/*
		 * check unread status
		 */
		(*tocH)->unread = TOCUnread(tocH);

		/*
		 * hurray for our side!
		 */
		UL(tocH);
		return (tocH);
	} else if (CommandPeriod || insane == memFullErr)
		return (nil);
	else
		return (FixErrantTOC(spec, nil, euCorruptTOC));
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr ReadDForkTOC(FSSpecPtr aSpec, TOCHandle * inTOC)
{
	long count;
	int err = noErr;
	short refN = 0;
	TOCHandle tocH = nil;
	FSSpec tocSpec;

	/*
	 * toc is kept in name.toc; try to open it
	 */
	Box2TOCSpec(aSpec, &tocSpec);
	if (!(err = AFSpOpenDF(&tocSpec, &tocSpec, fsRdPerm, &refN))) {
		/*
		 * allocate space for the toc
		 */
		if (!(err = GetEOF(refN, &count)))
			if (count < sizeof(TOCType))
				err = sizeof(TOCType) - count;
			else if (tocH = NuHTempOK(count)) {
				/*
				 * try to read it
				 */
				LDRef(tocH);
				err = ARead(refN, &count, (void *) *tocH);
				(void) MyFSClose(refN), refN = 0;
				if (err)
					ZapHandle(tocH);
			} else {
				MyFSClose(refN);
				WarnUser(MEM_ERR, err = MemError());
			}
	}

	if (err)
		FileSystemError(READ_TOC, aSpec->name, err);

	*inTOC = tocH;
	ComposeLogS(err ? LOG_ALRT | LOG_TOC : LOG_TOC, nil,
		    "\pReadDForkTOC(%p): %d", aSpec->name, err);
	return (err);
}

/**********************************************************************
 * ReadRForkTOC - read a TOC from the resource fork
 **********************************************************************/
OSErr ReadRForkTOC(FSSpecPtr aSpec, TOCHandle * inTOC)
{
	short refN = 0;
	Handle r;
	TOCHandle tocH = nil;
	long size;
	OSErr err;
	FSSpec lSpec = *aSpec;
	short oldResF = CurResFile();

	IsAlias(aSpec, &lSpec);

	if (-1 != (refN = FSpOpenResFile(&lSpec, fsRdPerm))) {
		SetResLoad(False);
		r = Get1Resource(TOC_TYPE, 1001);
		if (!r)
			r = Get1Resource(TOC_TYPE, 1002);
		SetResLoad(True);
		if (r) {
			size = GetResourceSizeOnDisk(r);
			if (err = ResError())
				FileSystemError(CORRUPT_RFORK, &lSpec.name,
						err);
			else {
				tocH = NuHTempOK(size);
				if (!tocH)
					WarnUser(MEM_ERR, err =
						 MemError());
				else {
					ReadPartialResource(r, 0,
							    LDRef(tocH),
							    size);
					if (err = ResError())
						FileSystemError
						    (CORRUPT_RFORK,
						     &lSpec.name, err);
				}
			}
		} else
			FileSystemError(CORRUPT_RFORK, &lSpec.name,
					resNotFound);
	}

	if (refN)
		CloseResFile(refN);
	if (err)
		ZapHandle(tocH);

	ComposeLogS(err ? LOG_ALRT | LOG_TOC : LOG_TOC, nil,
		    "\pReadRForkTOC(%p): %d", aSpec->name, err);
	*inTOC = tocH;
	UseResFile(oldResF);
	return (err);
}

/**********************************************************************
 * HasExternalTOC - does a file have an external table of contents?
 **********************************************************************/
Boolean HasExternalTOC(FSSpecPtr spec)
{
	FSSpec tocSpec;
	Box2TOCSpec(spec, &tocSpec);
	return (noErr == FSpExists(&tocSpec));
}

/**********************************************************************
 * WriteTOC - write a toc to the proper file
 **********************************************************************/
int WriteTOC(TOCHandle tocH)
{
	int err;
	short refN = 0;
	long count;
	FSSpec tocSpec, spec;
	uLong writeDate = LocalDateTime() & ~(1L);	/* make it even because of idiot novell */
	uLong size;
	uLong deadTicks;

	// Avoid multiple simultaneous writes
	ASSERT(!(*tocH)->beingWritten);
	deadTicks = TickCount();
	while ((*tocH)->beingWritten && (TickCount() - deadTicks < 5 * 60))	// wait up to 5 seconds
	{
		MyYieldToAnyThread();
		MiniEvents();
	}
	ASSERT(!(*tocH)->beingWritten);
	if ((*tocH)->beingWritten++) {
		// Oh, sxxt.  Somebody either left the beingWritten flag set, or there's a verrrrrrryyyyy long
		// write going on.
		(*tocH)->beingWritten = 1;	// rest the counter and hope for the best
	}
	// Save this dainty little morsel
	if ((*tocH)->which == OUT)
		BMD(&NoAdsRec, &(*tocH)->internalUseOnly,
		    sizeof((*tocH)->internalUseOnly));

	spec = GetMailboxSpec(tocH, -1);
	size = FSpDFSize(&spec);

	(*tocH)->boxSize = size + 1;	/* add 1 to signal that we know it's ok */
	(*tocH)->writeDate = writeDate;

	if (PrefIsSet(PREF_NEW_TOC))
		err = WriteRForkTOC(tocH);

	LDRef(tocH);

	/*
	 * build the name
	 */
	Box2TOCSpec(&spec, &tocSpec);

	if (PrefIsSet(PREF_NEW_TOC)) {
		if (!err) {
			ChainDelete(&tocSpec);
			AFSpSetMod(&spec, writeDate);
		}
	} else {
		/*
		 * open the file
		 */
		err = AFSpOpenDF(&tocSpec, &tocSpec, fsRdWrPerm, &refN);
		if (err == fnfErr) {
			if (err =
			    FSpCreate(&tocSpec, CREATOR, TOC_TYPE,
				      smSystemScript))
				goto failure;
			err = FSpOpenDF(&tocSpec, fsRdWrPerm, &refN);
		}
		if (err)
			goto failure;


		/*
		 * write the file
		 */
		count = GetHandleSize_(tocH);
		(*tocH)->unreadBase = (*tocH)->count;
		if (err = AWrite(refN, &count, (void *) *tocH))
			goto failure;

		/*
		 * fix up menu items
		 */
		FixBoxUnread(tocH);

		/*
		 * done
		 */
		GetFPos(refN, &count);
		SetEOF(refN, count);
		(void) MyFSClose(refN);
		(*tocH)->durty = (*tocH)->reallyDirty = False;
		AFSpSetMod(&tocSpec, writeDate);
		AFSpSetMod(&spec, writeDate);
		ComposeLogS(LOG_FLOW, nil, "\pWriteTOC %p\015",
			    tocSpec.name);
		UL(tocH);

		/*
		 * the size area
		 */
		if ((*tocH)->win
		    && IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win)))
			InvalBoxSizeBox((*tocH)->win);

		(*tocH)->beingWritten--;
		ComposeLogS(err ? LOG_ALRT | LOG_TOC : LOG_TOC, nil,
			    "\pWriteTOC(%p): %d", tocSpec.name, err);
		return (noErr);

	      failure:
		ComposeLogS(err ? LOG_ALRT | LOG_TOC : LOG_TOC, nil,
			    "\pWriteTOC(%p): %d", tocSpec.name, err);
		FileSystemError(WRITE_TOC, tocSpec.name, err);
		UL(tocH);
		if (refN) {	/* get rid of partial file */
			(void) MyFSClose(refN);
			(void) FSpDelete(&tocSpec);
		}
	}
	UL(tocH);
	(*tocH)->beingWritten--;
	return (err);
}

/************************************************************************
 * FixBoxUnread - fix the unread status of a mailbox in the menus
 ************************************************************************/
void FixBoxUnread(TOCHandle tocH)
{
	Boolean unread = TOCUnread(tocH);
	short myMenu;
	short myItem;
	FSSpec spec;
	uLong total, used;

	(*tocH)->unread = unread;
	(*tocH)->unreadBase = (*tocH)->count;

	myItem = 0;
	spec = GetMailboxSpec(tocH, -1);
	Spec2Menu(&spec, False, &myMenu, &myItem);

	if (myItem > 0) {
		FixSpecUnread(&spec, unread);
		FixMenuUnread(GetMHandle(myMenu), myItem, unread);
	}

	GetTOCK(tocH, &used, &total);
	if ((*tocH)->usedK != used || (*tocH)->totalK != total) {
		(*tocH)->usedK = used;
		(*tocH)->totalK = total;
		(*tocH)->updateBoxSizes = true;
	}

}

/**********************************************************************
 * WriteRForkTOC - write a table of contents to the resource fork of a file
 **********************************************************************/
OSErr WriteRForkTOC(TOCHandle tocH)
{
	FSSpec spec = GetMailboxSpec(tocH, -1);
	short refN;
	OSErr err;
	Boolean sane;
	short oldResF = CurResFile();

	IsAlias(&spec, &spec);
	if (FSpRFSane(&spec, &sane) || !sane)
		FSpKillRFork(&spec);
	FSpCreateResFile(&spec, CREATOR, 'TEXT', smSystemScript);

	if (-1 != (refN = FSpOpenResFile(&spec, fsRdWrPerm))) {
		KillTOC(refN, nil);
		(*tocH)->unreadBase = (*tocH)->count;
		AddResource((Handle) tocH, TOC_TYPE, 1001, "");
		err = ResError();
		if (!err)
			MyUpdateResFile(refN);
		DetachResource((Handle) tocH);
		CloseResFile(refN);
	} else
		err = ResError();

	if (err)
		FileSystemError(WRITE_TOC, spec.name, err);
	else {
		/*
		 * fix up menu items
		 */
		FixBoxUnread(tocH);

		/*
		 * the size area
		 */
		if ((*tocH)->win
		    && IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win)))
			InvalBoxSizeBox((*tocH)->win);

		/*
		 * done
		 */
		(*tocH)->durty = (*tocH)->reallyDirty = False;
	}
	UseResFile(oldResF);
	ComposeLogS(err ? LOG_ALRT | LOG_TOC : LOG_TOC, nil,
		    "\pWriteRForkTOC(%p): %d", spec.name, err);
	return (err);
}

/**********************************************************************
 * TOCUnread - does a table of contents contain unread messages?
 **********************************************************************/
Boolean TOCUnread(TOCHandle tocH)
{
	Boolean unread = False;
	uLong minDate;
	short myItem;

	// don't underline Junk if the user has asked us not to.
	if ((((*tocH)->which == JUNK)
	     || (IsIMAPJunkMailbox(TOCToMbox(tocH))))
	    && JunkPrefBoxNoUnread())
		return false;

	minDate = GetRLong(UNREAD_LIMIT) * 24 * 3600;
	if (minDate)
		minDate = GMTDateTime() - minDate;

	for (myItem = 0; myItem < (*tocH)->count; myItem++)
		if ((*tocH)->sums[myItem].state == UNREAD &&
		    (!minDate || (*tocH)->sums[myItem].seconds > minDate))
		{
			unread = True;
			break;
		}
	return (unread);
}

/************************************************************************
 * TOCUnreadCount - how many messages are unread in this toc?
 ************************************************************************/
short TOCUnreadCount(TOCHandle tocH, Boolean recentOnly)
{
	short count = 0;
	short i;
	uLong minDate;

	if (!tocH)
		return 0;

	if (recentOnly) {
		minDate = GetRLong(UNREAD_LIMIT) * 24 * 3600;
		if (minDate)
			minDate = GMTDateTime() - minDate;
	}

	if (recentOnly && minDate) {
		for (i = (*tocH)->count - 1; i >= 0; i--)
			if ((*tocH)->sums[i].state == UNREAD
			    && (*tocH)->sums[i].seconds > minDate)
				count++;
	} else {
		for (i = (*tocH)->count - 1; i >= 0; i--)
			if ((*tocH)->sums[i].state == UNREAD)
				count++;
	}

	return count;
}

/**********************************************************************
 * FindTOC - find a TOC in the TOC window list
 **********************************************************************/
TOCHandle FindTOC(FSSpecPtr spec)
{
	TOCHandle tocH;
	FSSpec boxSpec;
	FSSpec newSpec;

	for (tocH = TOCList; tocH; tocH = (*tocH)->next) {
		boxSpec = GetMailboxSpec(tocH, -1);
		if (SameSpec(&boxSpec, spec))
			break;
		if (IsAlias(&boxSpec, &boxSpec)
		    && SameSpec(&boxSpec, spec))
			break;
	}

	if (!tocH && IsAlias(spec, &newSpec) && !SameSpec(spec, &newSpec))
		return (FindTOC(&newSpec));
	else
		return (tocH);
}


/************************************************************************
 * FlushTOCs - make sure all toc's are quiescent
 ************************************************************************/
int FlushTOCs(Boolean andClose, Boolean canSkip)
{
	TOCHandle tocH, nextTocH;
	WindowPtr tocWinWP;
	int shdBe;
	short err;
	static long lastTime;
	static short delay;
	Boolean dontCloseIMAPToc;

#ifdef THREADING_ON
	if (GetNumBackgroundThreads())	// thread might be using toc, so don't close 'em
		return (noErr);
#endif

	if (canSkip && lastTime && TickCount() - lastTime < delay)
		return (noErr);

	err = 0;

	for (tocH = TOCList; tocH; tocH = nextTocH) {
		if ((long) *tocH & 1) {
#ifdef DEBUG
			if (RunType != Production)
				DebugStr("\pbad tocH!");
#endif	/* DEBUG */
			if (tocH == TOCList)
				TOCList = nil;
			else {
				for (nextTocH = TOCList;
				     (*nextTocH)->next != tocH;
				     nextTocH = (*nextTocH)->next);
				(*nextTocH)->next = nil;
			}
			break;
		}

		shdBe = TOCSizeShouldBe(tocH);
		if (GetHandleSize_(tocH) != shdBe) {
#ifdef DEBUG
			if (RunType != Production) {
				Str127 debug;
				Str32 name;
				GetMailboxName(tocH, -1, name);
				ComposeString(debug,
					      "\p%p: size %d, should be %d (%d + (%d-1)*%d)",
					      name, GetHandleSize_(tocH),
					      shdBe, sizeof(TOCType),
					      (*tocH)->count,
					      sizeof(MSumType));
				SysBreakStr(debug);
			}
#endif	/* DEBUG */
			SetHandleBig_(tocH, shdBe);
		}
		nextTocH = (*tocH)->next;

		if (err = BoxFClose(tocH, true))
			break;
		if ((*tocH)->reallyDirty) {
			if (err = WriteTOC(tocH))
				break;
		} else if ((*tocH)->unreadBase != (*tocH)->count)
			FixBoxUnread(tocH);
		tocWinWP = GetMyWindowWindowPtr((*tocH)->win);

		// Make sure the toc we're about to close isn't an IMAP toc that's in use somewhere ...
		if (andClose)
			dontCloseIMAPToc = (*tocH)->imapTOC
			    && IMAPTOCInUse(TOCToMbox(tocH));

		if (andClose && !IsWindowVisibleClassic(tocWinWP)
		    && !(MyNMRec && (*tocH)->which == IN)
		    && !dontCloseIMAPToc) {
			int sNum;
			for (sNum = 0; sNum < (*tocH)->count; sNum++)
				if ((*tocH)->sums[sNum].messH)
					break;
			if (sNum == (*tocH)->count)
				CloseMyWindow(tocWinWP);
		}
		if (NEED_YIELD)
			break;
	}

	if (err) {
		lastTime = TickCount();
		if (!delay)
			delay = 60 * 30;
		else
			delay = MIN(60 * 60 * 5, (delay * 3) / 2);
	} else
		lastTime = delay = 0;
	return (err);
}

/************************************************************************
 * GetSpecialTOC - find the toc of a special mailbox
 ************************************************************************/
TOCHandle GetSpecialTOC(short nameId)
{
	FSSpec spec;
	Str31 name;

	GetRString(name, nameId);

#ifdef THREADING_ON
	if (nameId == IN_TEMP || nameId == OUT_TEMP) {
		OSErr err;
		FSSpec spool;
		long dirID;

		err = SubFolderSpec(SPOOL_FOLDER, &spool);

		if (!err) {
			dirID = SpecDirId(&spool);
			err =
			    FSMakeFSSpec(spool.vRefNum, spool.parID, name,
					 &spec);
		}
		ASSERT(!err);
		if (err)
			return nil;
	} else
#endif

		FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, name, &spec);
	return (TOCBySpec(&spec));
}

/**********************************************************************
 * PeekRTOC - peek at a resource fork toc
 **********************************************************************/
OSErr PeekRTOC(FSSpecPtr spec, TOCType * tocPart)
{
	FSSpec tocSpec;
	Boolean sane;
	OSErr err;
	Handle res;
	short refN;
	short oldResF = CurResFile();
	Boolean emptyBox = FSpDFSize(spec) == 0;

	IsAlias(spec, &tocSpec);
	if (err = FSpRFSane(&tocSpec, &sane))
		ComposeLogS(LOG_TOC | LOG_ALRT, nil, "\pFSpRFSane(%p): %d",
			    spec->name, err);
	else {
		if (!sane) {
			ComposeLogS(LOG_TOC | LOG_ALRT, nil,
				    "\pTOC %p insane", spec->name);
			err = fnfErr;
		} else if (-1 !=
			   (refN = FSpOpenResFile(&tocSpec, fsRdWrPerm))) {
			SetResLoad(False);
			res = Get1Resource(TOC_TYPE, 1001);
			SetResLoad(True);

			if (!res && !emptyBox)
				ComposeLogS(LOG_TOC | LOG_ALRT, nil,
					    "\p%p primary toc missing",
					    spec->name);

			if (!res) {
				SetResLoad(False);
				res = Get1Resource(TOC_TYPE, 1002);
				SetResLoad(True);

				if (!res && !emptyBox)
					ComposeLogS(LOG_TOC | LOG_ALRT,
						    nil,
						    "\p%p secondary toc missing",
						    spec->name);
			}

			if (!res)
				err = fnfErr;
			else {
				ReadPartialResource(res, 0, tocPart,
						    sizeof(TOCType));
				err = ResError();
				if (err)
					ComposeLogS(LOG_TOC | LOG_ALRT,
						    nil,
						    "\pReadPartialResource(%p): %d",
						    spec->name, err);

			}
			CloseResFile(refN);
		} else {
			ComposeLogS(LOG_TOC | LOG_ALRT, nil,
				    "\p%p can't open resource fork",
				    spec->name);
			err = fnfErr;
		}
	}
	UseResFile(oldResF);
	return (err);
}


#ifdef BATCH_DELIVERY_ON	//NEVER
/**********************************************************************
 * PeekDTOC - peek at a data fork toc
 **********************************************************************/
OSErr PeekDTOC(FSSpecPtr spec, TOCType * tocPart)
{
	FSSpec tocSpec;
	OSErr err;
	short refN;
	long bytes;

	Box2TOCSpec(spec, &tocSpec);
	if (!(err = AFSpOpenDF(&tocSpec, &tocSpec, fsRdPerm, &refN))) {
		bytes = sizeof(TOCType);
		err = FSRead(refN, &bytes, tocPart);
		if (!err && bytes != sizeof(TOCType))
			err = eofErr;
		// close it 
		FSClose(refN);
	}

	return (err);
}
#endif

/**********************************************************************
 * PeekTOC - peek into a .toc file
 **********************************************************************/
OSErr PeekTOC(FSSpecPtr spec, TOCType * tocPart)
{
	uLong box, file, res;
	OSErr err;
	TOCHandle tocH = FindTOC(spec);

	if (tocH)		// already open; I hope the caller is being careful
	{
		*tocPart = **tocH;
		return (noErr);
	}

	err = TOCDates(spec, &box, &res, &file);

	if (err)
		return (err);

	if (res && res >= file)
		return (PeekRTOC(spec, tocPart));
	else if (file)
		return (PeekDTOC(spec, tocPart));
	else
		return (fnfErr);
}

/************************************************************************
 * InsaneTOC - see if a TOC is nuts
 ************************************************************************/
OSErr InsaneTOC(TOCHandle tocH)
{
	long boxSize;
	MSumPtr start, stop;
	Str31 name;
	short tocSize = sizeof(TOCType);
	short sumSize = sizeof(MSumType);
	long handleSize = GetHandleSize_(tocH);
	FSSpec spec = GetMailboxSpec(tocH, -1);

	PCopy(name, spec.name);

	/*
	 * is the toc the right size?
	 */
	if ((*tocH)->count < 0)
		return (True);
	if (sizeof(TOCType) +
	    ((*tocH)->count ? (*tocH)->count - 1 : 0) * sizeof(MSumType) !=
	    GetHandleSize_(tocH)) {
		ComposeLogS(LOG_ALRT | LOG_TOC, nil,
			    "\p%p toc/count size mismatch; #%d != %d",
			    name, (*tocH)->count, GetHandleSize_(tocH));
		return (euCorruptTOC);
	}

	/*
	 * figure out how big the mailbox is
	 */
	boxSize = FSpDFSize(&spec);

	/*
	 * right size?
	 */
	if ((*tocH)->boxSize && (*tocH)->boxSize - 1 != boxSize) {
		ComposeLogS(LOG_ALRT | LOG_TOC, nil,
			    "\p%p file size mismatch; #%d != %d", name,
			    (*tocH)->boxSize, boxSize);
		return (euMismatchTOC);
	}

	/*
	 * right date?
	 */
	// don't do the date check anymore, since it breaks under HFS+
	//if ((*tocH)->writeDate && (*tocH)->writeDate!=AFSpGetMod(&spec)) return(euMismatchTOC);

	/*
	 * check for out of range pointers
	 */
	for (start = (*tocH)->sums, stop = start + (*tocH)->count;
	     start < stop; start++) {
		if ((start->offset < 0 && !(*tocH)->imapTOC)
		    || start->length < 0 || start->bodyOffset < 0
		    || start->bodyOffset > start->length
		    || ((start->offset + start->length > boxSize)
			&& !(*tocH)->imapTOC)) {
			ComposeLogS(LOG_ALRT | LOG_TOC, nil,
				    "\p%p bad sum; o%d b%d l%d s%d", name,
				    start->offset, start->bodyOffset,
				    start->length, boxSize);
			return (euCorruptTOC);
		}
	}

	/*
	 * wrong version number?
	 */
	if ((*tocH)->version > CURRENT_TOC_VERS) {
		ComposeLogS(LOG_ALRT | LOG_TOC, nil,
			    "\p%p version mismatch; #%d != %d", name,
			    (*tocH)->version, CURRENT_TOC_VERS);
		return (euBadVersion);
	}

	/*
	 * everything looks ok
	 */
	return (noErr);
}

/************************************************************************
 * GetMailboxType
 ************************************************************************/
short GetMailboxType(FSSpecPtr spec)
{
	short which = 0;

	if (IsRoot(spec)) {
		if (EqualStrRes(spec->name, IN))
			which = IN;
		else if (EqualStrRes(spec->name, OUT))
			which = OUT;
		else if (EqualStrRes(spec->name, TRASH))
			which = TRASH;
		else if (EqualStrRes(spec->name, JUNK))
			which = JUNK;
	}
#ifdef THREADING_ON
	else if (IsSpool(spec)) {
		if (EqualStrRes(spec->name, IN_TEMP))
			which = IN_TEMP;
		else if (EqualStrRes(spec->name, OUT_TEMP))
			which = OUT_TEMP;
	}
#ifdef BATCH_DELIVERY_ON
	else if (IsDelivery(spec))
		which = DELIVERY_BATCH;
#endif
#endif
	return which;
}


/**********************************************************************
 * FixErrantTOC - we have determined that something is wrong with a TOC
 * fix or cancel
 **********************************************************************/
TOCHandle FixErrantTOC(FSSpecPtr spec, TOCHandle tocH, short why)
{
	short result = -1;

#ifdef THREADING_ON
// if we're any of the temp tocs, automatically rebuild them
	short which = GetMailboxType(spec);
	if (which == IN_TEMP || which == OUT_TEMP
	    || which == DELIVERY_BATCH)
		return (RebuildTOC(spec, tocH, False, True));
#endif

	switch (result =
		WantRebuildTOC(spec->name, why, ((*tocH)->imapTOC != 0))) {
	case 0:
		if (tocH) {
			TOCSetDirty(tocH, true);
			(*tocH)->reallyDirty = True;	/* see that it gets updated */
		}
		return (tocH);
	case 1:
		return (RebuildTOC(spec, tocH, False, False));
	case 2:
		ZapHandle(tocH);
		return (nil);
	}
	ASSERT(result != -1);
	return nil;
}

/************************************************************************
 * WantRebuildTOC - see if the user wants us to remake the toc
 ************************************************************************/
Boolean WantRebuildTOC(UPtr boxName, OSErr why, Boolean isIMAP)
{
	if (!PrefIsSet(PREF_TOC_REBUILD_ALERTS))
		return true;
	if (why == euMismatchTOC) {
		short whatToDo;

		if (isIMAP)
			whatToDo =
			    AlertStr(NEW_IMAP_TOC_ALRT, Caution, boxName);
		else {
			// "Encourage" users to rebuild
			if (TOC_USE_OLD ==
			    (whatToDo =
			     AlertStr(NEW_TOC_ALRT, Caution, boxName))
			    && TOC_USE_OLD == (whatToDo =
					       ComposeStdAlert(Caution,
							       REBUILD_TOC_ALRT_2,
							       boxName)))
				whatToDo =
				    ComposeStdAlert(Stop,
						    REBUILD_TOC_ALRT_3,
						    boxName);
		}

		switch (whatToDo) {
		case TOC_CREATE_NEW:
			return (1);
		case TOC_USE_OLD:
			return (0);
		case CANCEL_ITEM:
		case TOC_CANCEL:
			return (2);
		}
	} else
		switch (AlertStr(REB_TOC_ALRT, Caution, boxName)) {
		case 1:
			return (1);
		default:
			return (2);
		}
	ASSERT(false);
	return true;
}

/************************************************************************
 * GetTOCK - grab the K counts for a mailbox
 ************************************************************************/
short GetTOCK(TOCHandle tocH, uLong * usedK, uLong * totalK)
{
	MSumPtr sum;
	long used = 0;
	short err;
	CInfoPBRec info;
	FSSpec spec;

	for (sum = (*tocH)->sums; sum < (*tocH)->sums + (*tocH)->count;
	     sum++) {
		if (sum->offset >= 0)
			used += sum->length;
	}
	*usedK = used / 1024;
	LDRef(tocH);
	spec = GetMailboxSpec(tocH, -1);
	*totalK = (err = AFSpGetHFileInfo(&spec, &info)) ?
	    0 : info.hFileInfo.ioFlLgLen / 1024;
	UL(tocH);
	ASSERT(*totalK < 4096 K);
	return (err);
}

/************************************************************************
 * CheckStringLen - make sure string field is not too long
 ************************************************************************/
void CheckStringLen(UPtr s, char maxLen, short fillLen)
{
	if (*s > fillLen) {
		// total junk; destroy
		WriteZero(s, fillLen);
	} else if (*s > maxLen) {
		// oops, too long
		WriteZero(s + maxLen + 1, fillLen - maxLen - 1);	// zero out remainder
		*s = maxLen;
	}
}

/************************************************************************
 * IsTOCValid - is the toc on the list of currently-opened toc's
 ************************************************************************/
TOCHandle IsTOCValid(TOCHandle testTOC)
{
	TOCHandle tocH;
	for (tocH = TOCList; tocH; tocH = (*tocH)->next)
		if (tocH == testTOC)
			return (tocH);
	return (nil);
}

/************************************************************************
 * CleanseTOC - free a newly-read toc of vestiges of its past life
 ************************************************************************/
void CleanseTOC(TOCHandle tocH)
{
	short count, i;
	MSumPtr sum;
	Byte vers = (*tocH)->version;
	short minor = (*tocH)->minorVersion;
	long size = GetHandleSize((Handle) tocH);
	Boolean needSerialNum = vers < 1 || (vers == 1 && minor < 2);
	Boolean zeroMood = vers < 1 || (vers == 1
					&& minor < TOC_MINOR_HAS_MOOD);
	Boolean fixOutlookisms = vers == 1
	    && minor < TOC_MINOR_FIXED_OUTLOOKISMS;
	Boolean fixUTF8 = vers == 1
	    && minor < TOC_MINOR_NO_UNNECESSARY_UTF8;
	uLong now = GMTDateTime();
	Str63 subj;

	if (needSerialNum)
		(*tocH)->nextSerialNum = 1;

	/*
	 * unset the selected flags; they might have been saved from last time
	 * unset the messH; it may have been filled the last time
	 */
	count = TOCCountShouldBe(tocH);
	count = MIN((*tocH)->count, count);
	for (i = 0; i < count; i++) {
		sum = (*tocH)->sums + i;
		sum->mesgErrH = 0;
		sum->messH = 0;

		// let's see about restoring the selection... */ sum->selected = False;
		sum->cache = 0;
		if (vers < 1) {
			DBNoteUIDHash(sum->uidHash, kNeverHashed);
			sum->uidHash = sum->msgIdHash = kNeverHashed;
		}
		if (minor < 1)
			sum->msgIdHash = kNeverHashed;

		if (zeroMood)
			sum->score = 0;

		// make sure older versions don't have strings that are too long
		CheckStringLen(sum->from, sizeof(sum->from) - 1, 64);
		CheckStringLen(sum->subj, sizeof(sum->subj) - 1, 64);
		if ((*tocH)->minorVersion < TOC_MINOR_NO_DATE_STRING) {
			short j;
			sum->spamScore = -1;
			sum->spamBecause = 0;
			sum->spare21 = 0;
			sum->arrivalSeconds = 0;
			for (j = 0;
			     j <
			     sizeof(sum->spare) / sizeof(sum->spare[0]);
			     j++)
				sum->spare[j] = 0;
		}

		if (fixOutlookisms) {
			PSCopy(subj, sum->subj);
			BeautifySubj(subj, sizeof(sum->subj));
			PSCopy(sum->subj, subj);
		}

		if (fixUTF8)
			RemoveUTF8FromSum(sum);

		if (!sum->arrivalSeconds)
			sum->arrivalSeconds = sum->seconds;

		if (needSerialNum)
			sum->serialNum = (*tocH)->nextSerialNum++;
	}
	if ((*tocH)->minorVersion < TOC_MINOR_HAS_LONG_K)
		(*tocH)->oldKValues = 0;
	if ((*tocH)->minorVersion < TOC_MINOR_HAS_PROFILE)
		(*tocH)->profile[0] = (*tocH)->profile[1] = 0;
	if ((*tocH)->minorVersion < TOC_MINOR_NO_DATE_STRING)
		(*tocH)->spareLong = 0;
	(*tocH)->version = CURRENT_TOC_VERS;
	(*tocH)->minorVersion = CURRENT_TOC_MINOR;
	(*tocH)->virtualTOC = false;	// not a virtual toc, nossir!
	(*tocH)->beingWritten = false;	// not being written at the moment
	(*tocH)->analScanned = false;	// need to do a scan for flames
	(*tocH)->hFileView = nil;

	// Junk processing
	if ((*tocH)->which == JUNK)
		JunkTOCCleanse(tocH);
}

#pragma segment Main
/************************************************************************
 * RedoTOCs - make sure right info is displayed in all TOC's
 ************************************************************************/
void RedoTOCs(void)
{
	TOCHandle tocH = TOCList;

      overAgain:
	for (tocH = TOCList; tocH && !NeedYield(); tocH = (*tocH)->next)
		// only do this for visible windows
		if (IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win))
		    && RedoTOC(tocH)) {
			// we just did some work.

			if (NeedYield())
				break;	// need to go process events

			goto overAgain;	// take it from the top; trying to avoid
			// race condition with threading.
		}
}
