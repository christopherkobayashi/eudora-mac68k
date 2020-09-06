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

#include "message.h"
#define FILE_NUM 25
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Message
UHandle GetMessText(MessHandle messH);
int MessErr;
#ifndef	IMAP
void MovingAttachments(TOCHandle tocH,short sumNum,Boolean attach,Boolean wipe,Boolean nuke,Boolean IMAPStubsOnly);
#endif
int FindAndCopyHeader(MessHandle origMH,MessHandle newMH,UPtr fromHead,short toHead);
void WeedHeaders(UHandle buffer,long *weeded,short toWeed,AccuPtr weeds);
#ifdef	IMAP
MyWindowPtr OpenMessage(TOCHandle tocH,short sumNum,WindowPtr winWP,MyWindowPtr win, Boolean showIt, Boolean preview);
#else
MyWindowPtr OpenMessage(TOCHandle tocH,short sumNum,WindowPtr winWP,MyWindowPtr win, Boolean showIt);
#endif
OSErr RemoveSelf(MessHandle messH,short head,Boolean wantErrors);
void FindFrom(UPtr who, PETEHandle pte);
void Attribute(short attrId,MessHandle origMessH,MessHandle newMessH,Boolean atEnd);
void XferCustomTable(MessHandle origMessH,MessHandle newMessH);
void WeedXAttachments(MessHandle messH,Boolean errReport);
void RemoveIndAttachment(MessHandle messH, short index);
void PeteApplyStyles(PETEHandle pte,OffsetAndStyleHandle styles);
OSErr CopyToOut(TOCHandle fromTocH,short sumNum,TOCHandle toTocH);
OSErr UniqueHeader(MessHandle messH,short head,Boolean wantErrors);
	OSErr FindMessageByMID(uLong mid,TOCHandle *tocH,short *sumNum);
OSErr WipeMessage(TOCHandle tocH,short sumNum);
	long StripTrailingNewlines(Handle buffer,long stop);
OSErr MessageWarnings(TOCHandle tocH, short sumNum,Boolean toTrash,Boolean nuke,
	Boolean *queuedWarning,Boolean *unsentWarning,Boolean *unreadWarning,Boolean *busyWarning);
OSErr SelectedWarnings(TOCHandle tocH,Boolean toTrash,Boolean nuke);
OSErr SingleWarnings(TOCHandle tocH,short sumNum,Boolean toTrash,Boolean nuke);
void DeleteMessageLo(TOCHandle tocH, int sumNum,Boolean nuke);
long CompBodyOffset(MessHandle messH);
OSErr SpoolAttachments(MessHandle messH);
OSErr CopyAttachments(MessHandle messH);
OSErr ReplyReferences(MessHandle origMessH,MessHandle newMessH);
void HTMLifyText(MyWindowPtr win,Handle text);
Boolean DoMessageMenu(short item,TOCHandle tocH,short sumNum,short toWhom,TextAddrHandle addr,long modifiers,Boolean nuke,Boolean *busy);
OSErr CopyNewsgroups(MessHandle origMH,MessHandle newMH);
Boolean AttStillInFolder(FSSpecPtr att, FSSpecPtr folder);
PStr MessCurAddr(MyWindowPtr win, PStr addr);

void DoCFStringGlobalReplace(CFMutableStringRef theString, CFStringRef stringToFind, CFStringRef replacement);
CFStringRef MakeCFMessTitle(TOCHandle tocH,int sumNum);

/************************************************************************
 * GetAMessage - grab a message
 ************************************************************************/
MyWindowPtr GetAMessageLo(TOCHandle origTocH,int origSumNum,WindowPtr winWP, MyWindowPtr win, Boolean showIt, Boolean *newWin)
{
	WindowPtr	messWinWP;
	MessHandle messH;
	TOCHandle tocH;
	short sumNum;
	
	if(newWin) *newWin = true;
	tocH = GetRealTOC(origTocH,origSumNum,&sumNum);
	if (!tocH || (*tocH)->count<=sumNum) return(nil);
	if (messH = (*tocH)->sums[sumNum].messH)
	{
		messWinWP = GetMyWindowWindowPtr ((*messH)->win);
		if(newWin) *newWin = false;
		if (showIt)
		{
			if (!IsWindowVisible (messWinWP))
				ShowMyWindow(messWinWP);
			UserSelectWindow(messWinWP);
		}
		UsingWindow(messWinWP);
		win = (*messH)->win;
	}
	else if ((*tocH)->which==OUT)
		win = OpenComp(tocH,sumNum,winWP,win,showIt,False);
#ifdef NEVER
	else if (IsALink(tocH,sumNum))
		win = OpenLink(tocH,sumNum,winWP,win,showIt);
#endif
	else
#ifdef	IMAP
		win = OpenMessage(tocH,sumNum,winWP,win,showIt,false);
#else
		win = OpenMessage(tocH,sumNum,winWP,win,showIt);
#endif
	if (win)
	{
		(*Win2MessH(win))->openedFromTocH = origTocH;
		(*Win2MessH(win))->openedFromSerialNum = (*origTocH)->sums[origSumNum].serialNum;
	}
	return(win);
}

/**********************************************************************
 * OpenMessage - open a message in its own window
 **********************************************************************/
#ifdef	IMAP
MyWindowPtr OpenMessage(TOCHandle tocH,short sumNum,WindowPtr winWP,MyWindowPtr win, Boolean showIt, Boolean preview)
#else
MyWindowPtr OpenMessage(TOCHandle tocH,short sumNum,WindowPtr winWP,MyWindowPtr win, Boolean showIt)
#endif
{
	MessHandle messH;
	Boolean turvy = showIt && (CurrentModifiers()&optionKey)!=0;
	UHandle text=nil;
	short ezOpenSum;
	PETEDocInitInfo pdi;
	OSErr err;
	long size;
	long partial = GetRLong(PETE_NIBBLE) K;
	StackHandle stack;
	Boolean	useLizzie,disableButtons = false;
	PETETextStyle style;	
			
	CycleBalls();
	
	if (!(tocH = GetRealTOC(tocH,sumNum,&sumNum)))
		return nil;
	
	if ((messH = (MessHandle)NuHandleClear(sizeof(MessType)))==nil)
		return(nil);
		
	win = GetNewMyWindow(MESSAGE_WIND,winWP,win,BehindModal,False,False,MESS_WIN);
	if (!win)
	{
		ZapHandle(messH);
		return(nil);
	}
	
	winWP = GetMyWindowWindowPtr (win);
	
	(*tocH)->sums[sumNum].messH = messH;
	(*messH)->win = win;
	(*messH)->sumNum = sumNum;
	(*messH)->tocH = tocH;
	/* apply FLAG_OUT ex post facto */
	if ((*tocH)->sums[sumNum].state==SENT || (*tocH)->sums[sumNum].state==UNSENT)
		(*tocH)->sums[sumNum].flags |= FLAG_OUT;
	
	SetMyWindowPrivateData(win, (long)messH);
	win->close = MessClose;
	LL_Push(MessList,messH);

	if (MessOptIsSet(messH,OPT_WRITE)) ClearMessOpt(messH,OPT_WRITE);

#ifdef	IMAP
	// Actually go fetch this message if we must
	if ((*tocH)->imapTOC && !IMAPMessageDownloaded(tocH,sumNum))
	{
		// threading is off.  Download message now
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable())
		{
			if (EnsureMsgDownloaded(tocH,sumNum,true))
				text = GetMessText(messH);
		}
		else
		{
			Str255 scratch;
			Boolean reallyGetIt = !preview || (!Offline && AutoCheckOK());
			
			// start the message download thread.  If we're opening this message to be previewed ONLY, don't fetch it if we're offline.
			err = noErr;
			if (!IMAPMessageBeingDownloaded(tocH, sumNum) && !IMAPMessageDownloaded(tocH, sumNum) && reallyGetIt) err = UIDDownloadMessage(tocH, (*tocH)->sums[sumNum].uidHash, false, false);
			if (err) goto Abort;
			
			// put the "waiting for download" text in the message window.		
			if (!reallyGetIt) GetRString(scratch,IMAP_GETTING_MESSAGE_OFFLINE);
			else GetRString(scratch,IMAP_GETTING_MESSAGE);
			text = PStr2Handle(scratch);
			disableButtons = true;
		}
	}
	else
#endif
		text = GetMessText(messH);
	useLizzie = false;
	
	DefaultPII(win,0==((*tocH)->sums[sumNum].opts&OPT_WRITE),useLizzie?peGrowBox:peVScroll|peGrowBox,&pdi);
	pdi.inParaInfo.paraFlags = (!PrefIsSet(PREF_SEND_ENRICHED_NEW) ? 0:peTextOnly);
	pdi.docWidth = MessWi(win)-(useLizzie?0:PETE_SCROLLY_DIFFERENCE)+9;
	err=PeteCreate(win,&win->pte,0,&pdi);
	CleanPII(&pdi);
	if (err || !text) goto Abort;
	TheBody = win->pte;
	if ( MessFlagIsSet ( messH, FLAG_FIXED_WIDTH )) {
		short fixedID, fixedSize;
		
#ifdef USEFIXEDDEFAULTFONT
		fixedSize = FixedSize;
		fixedID = FixedID;
#else
		// what size to use?
		fixedSize = GetRLong(DEF_FIXED_SIZE);
		if (!fixedSize) fixedSize = FontSize;
		// what font to use?
		fixedID = GetFontID(GetRString(s,FIXED_FONT));
		if (fixedID==applFont)
			fixedID = (ScriptVar(smScriptMonoFondSize)>>16)&0xffff;
#endif
		PeteFontAndSize(TheBody, fixedID, fixedSize);
		}

	{
		StackInit(sizeof(PartDesc),&stack);
		(*PeteExtra(win->pte))->partStack = stack;
	
		PeteCalcOff(win->pte);
		if (!showIt||MessIsRich(messH)||MessOptIsSet(messH,OPT_CHARSET)) partial = 0;
		size = GetHandleSize(text);
		if (!partial || size<partial)
		{
			// stick in the text
			if (!MessFlagIsSet(messH,FLAG_SHOW_ALL) &&
			    (MessFlagIsSet(messH,FLAG_RICH)
#ifndef OLDHTML
			    || MessOptIsSet(messH,OPT_HTML)
#endif
			    || MessOptIsSet(messH,OPT_FLOW)
			    || MessOptIsSet(messH,OPT_CHARSET)
			    ))
			{
				err = InsertRichLo(text,0,-1,-1,True,True,TheBody,stack,messH,MessOptIsSet(messH,OPT_DELSP));
				ZapHandle(text);
			}
			else
				err = PETEInsertPara(PETE,TheBody,kPETELastPara,nil,text,nil);

			if (!err)
			{
				text = nil;	// so we don't dispose of it
				PeteTrimTrailingReturns(TheBody,true);
			}
		}
		else
		{
			err = PETEInsertTextPtr(PETE,win->pte,kPETELastPara,LDRef(text),partial,nil);
			UL(text);
		}
	}
	if (err)
	{
		if (err != userCanceledErr)
			WarnUser(PETE_ERR,err);
	}
	else
	{
		CFStringRef cfTitle;
		
		cfTitle = MakeCFMessTitle(tocH, sumNum);
		SetWindowTitleWithCFString(winWP, cfTitle);
		CFRelease(cfTitle);
		win->menu = MessMenu;
		win->gonnaShow = MessGonnaShow;
		win->position = MessagePosition;
		win->cursor = MessCursor;
		win->button = CompButton;	/* it will do */
		win->app1 = MessApp1;
		win->find = MessFind;
		win->curAddr = MessCurAddr;

#ifdef OLDHTML
		/*
		 * interpret rich text
		 */
		if (!MessFlagIsSet(messH,FLAG_SHOW_ALL))
		{
			if (MessOptIsSet(messH,OPT_HTML))
				PeteHTML(TheBody,SumOf(messH)->bodyOffset-(*messH)->weeded+1,0,True);
		}
#endif
	
		if (showIt)
		{		
			/*
			 * prepare for easy open
			 */
			ezOpenSum = EzOpenFind(tocH,sumNum);
			if (ShowMyWindow(winWP)) {return(nil);}
					
			/*
			 * scroll to show first line of body
			 */
			ShowMessageSeparator(TheBody,true);
			PeteFocus(win,TheBody,True);
			
			/*
			 * rest of the text?
			 */
			if (text && !useLizzie)
			{
				PeteCalcOff(win->pte);
				err = PETEInsertTextPtr(PETE,win->pte,partial,LDRef(text)+partial,size-partial,nil);
				PeteSmallParas(win->pte);
				PeteCalcOn(win->pte);
				ZapHandle(text);
				PeteSetURLRescan(win->pte,0);
				if (err)
				{
					if (err != userCanceledErr)
						WarnUser(PETE_ERR,err);
				}
			}
	
			/*
			 * prepare for ezopen
			 */
			if (!err && ezOpenSum >= 0)
			{
				(*messH)->ezOpenSerialNum = (*tocH)->sums[ezOpenSum].serialNum;
				CacheMessage(tocH,ezOpenSum);
			}
			if (disableButtons)
				EnableMsgButtons(win,false);
		}
	}

	style.tsLock = peModLock;
	PETESetTextStyle(PETE,TheBody,0,0x7fffffff,&style,peLockValid);
	PETEMarkDocDirty(PETE,TheBody,False);
	win->isDirty = False;
	
	if (err)
	{
	  Abort:
		PeteCleanList(win->pte);
		win->isDirty = False;
		CloseMyWindow(winWP);
		win = nil;
		if (text) ZapHandle(text);
	}

	return(win);
}
#pragma segment Message

/**********************************************************************
 * MessCurAddr - return the address most closely associated with this message
 **********************************************************************/
PStr MessCurAddr(MyWindowPtr win, PStr addr)
{
	MessHandle messH = Win2MessH(win);
	extern PStr CompCurAddr(MyWindowPtr win, PStr addr);
	
	*addr = 0;
	
	if (messH)
	{
		if (MessFlagIsSet(messH,FLAG_OUT))
			CompCurAddr(win,addr);
		else
		{
			if (win->hasSelection) return CurAddrSel(win,addr);
			else
			{
				SuckHeaderText(messH,addr,sizeof(Str255),FROM_HEAD);
				ShortAddr(addr,addr);
			}
		}
	}

	return *addr ? addr : nil;
}

/**********************************************************************
 * MakeMessTitle - make a reasonable message title from a summary
 **********************************************************************/
void MakeMessTitle(UPtr title,TOCHandle tocH,int sumNum, Boolean useSummary)
{
	Str63 from, date, time, mailbox, subject;
	Str63 pattern;
	Str63 datetime;
	long secs;
	Str31 zoneStr;
	long zone;
	
	PCopy(from,(*tocH)->sums[sumNum].from);
	if (useSummary) *from = MIN(*from,(*BoxWidths)[blFrom-1]);
	
	if (useSummary)
	{
		if ((*BoxWidths)[blDate-1]>1)
		{
			ComputeLocalDate((*tocH)->sums+sumNum,datetime);
			*datetime = MIN(*datetime,(*BoxWidths)[blDate-1]);
		}
		else *datetime = 0;
	}
	else
	{
		if (secs = (*tocH)->sums[sumNum].seconds)
		{
			zone = PrefIsSet(PREF_LOCAL_DATE) ? ZoneSecs() : 60*(*tocH)->sums[sumNum].origZone;
			secs += zone;
			FormatZone(zoneStr,zone);
			TimeString(secs,False,time,nil);
			DateString(secs,shortDate,date,nil);
			utl_PlugParams(GetRString(pattern,DATE_SUM_FMT),datetime,date,time,zoneStr,"");
		}
		else *datetime = 0;
	}
	
	GetMailboxName(tocH,sumNum,mailbox);
	PCopy(subject,(*tocH)->sums[sumNum].subj);
	
	utl_PlugParams(GetRString(pattern,MESS_TITLE_PLUG),title,mailbox,from,datetime,subject);
}

void DoCFStringGlobalReplace(CFMutableStringRef theString, CFStringRef stringToFind, CFStringRef replacement)
{
	CFArrayRef ranges;
	CFRange wholeString, *range;
	CFIndex i;
	
	wholeString.location = 0;
	wholeString.length = CFStringGetLength(theString);
	ranges = CFStringCreateArrayWithFindResults(NULL, theString, stringToFind, wholeString, 0);
	if(!ranges) return;
	for(i = CFArrayGetCount(ranges); --i >= 0; )
	{
		range = (CFRange *)CFArrayGetValueAtIndex(ranges, i);
		if(replacement != NULL)
			CFStringReplace(theString, *range, replacement);
		else
			CFStringDelete(theString, *range);
	}
	CFRelease(ranges);
}

CFStringRef MakeCFMessTitle(TOCHandle tocH,int sumNum)
{
	CFStringRef temp;
	CFMutableStringRef pattern;
	CFRange wholeString;
	static CFStringRef upZero = NULL, upOne = NULL, upTwo = NULL, upThree = NULL;
	Str63 tempS;
	long secs;
	long zone;
	
	if(!upZero) upZero = CFSTR("^0");
	if(!upOne) upOne = CFSTR("^1");
	if(!upTwo) upTwo = CFSTR("^2");
	if(!upThree) upThree = CFSTR("^3");

	pattern = CFStringCreateMutable(NULL, 0);

	if (secs = (*tocH)->sums[sumNum].seconds)
	{
		GetRString(tempS,DATE_SUM_FMT);
		CFStringAppendPascalString(pattern, tempS, CFStringGetSystemEncoding());

		zone = PrefIsSet(PREF_LOCAL_DATE) ? ZoneSecs() : 60*(*tocH)->sums[sumNum].origZone;
		secs += zone;
		FormatZone(tempS,zone);
		temp = CFStringCreateWithPascalString(NULL, tempS, CFStringGetSystemEncoding());
		DoCFStringGlobalReplace(pattern, upTwo, temp);
		CFRelease(temp);
		
		TimeString(secs,False,tempS,nil);
		temp = CFStringCreateWithPascalString(NULL, tempS, CFStringGetSystemEncoding());
		DoCFStringGlobalReplace(pattern, upOne, temp);
		CFRelease(temp);

		DateString(secs,shortDate,tempS,nil);
		temp = CFStringCreateWithPascalString(NULL, tempS, CFStringGetSystemEncoding());
		DoCFStringGlobalReplace(pattern, upZero, temp);
		CFRelease(temp);
		
	}
	temp = CFStringCreateCopy(NULL, pattern);
	wholeString.location = 0;
	wholeString.length = CFStringGetLength(pattern);
	if(wholeString.length != 0)
		CFStringDelete(pattern, wholeString);
	
	GetRString(tempS,MESS_TITLE_PLUG);
	CFStringAppendPascalString(pattern, tempS, CFStringGetSystemEncoding());
	
	DoCFStringGlobalReplace(pattern, upTwo, temp);

	temp = CFStringCreateWithPascalString(NULL, (*tocH)->sums[sumNum].from, ((*tocH)->sums[sumNum].flags & FLAG_UTF8) ? kCFStringEncodingUTF8 : CFStringGetSystemEncoding());
	if(temp == NULL)
	{
		temp = CFStringCreateWithPascalString(NULL, (*tocH)->sums[sumNum].from, kCFStringEncodingMacRoman);
	}
	DoCFStringGlobalReplace(pattern, upOne, temp);
	CFRelease(temp);

	temp = CFStringCreateWithPascalString(NULL, (*tocH)->sums[sumNum].subj, ((*tocH)->sums[sumNum].flags & FLAG_UTF8) ? kCFStringEncodingUTF8 : CFStringGetSystemEncoding());
	if(temp == NULL)
	{
		temp = CFStringCreateWithPascalString(NULL, (*tocH)->sums[sumNum].subj, kCFStringEncodingMacRoman);
	}
	DoCFStringGlobalReplace(pattern, upThree, temp);
	CFRelease(temp);

	GetMailboxName(tocH,sumNum,tempS);
	temp = CFStringCreateWithPascalString(NULL, tempS, CFStringGetSystemEncoding());
	DoCFStringGlobalReplace(pattern, upZero, temp);
	CFRelease(temp);
	
	return pattern;
}

/**********************************************************************
 * PeteApplyStyles - apply a saved set of styles
 **********************************************************************/
void PeteApplyStyles(PETEHandle pte,OffsetAndStyleHandle styles)
{
	short n;
	long endOffset;
	OffsetAndStyle current;
	
	if (!styles || !*styles) return;
	
	n = HandleCount(styles);
	if (!n) return;
	
	endOffset = 0x7fffffff;
	
	while (n--)
	{
		current = (*styles)[n];
		PETESetTextStyle(PETE,pte,current.offset,endOffset,&current.style,current.validBits);
		endOffset = current.offset;
	}
	PETEMarkDocDirty(PETE,pte,False);
}

/**********************************************************************
 * GenerateReceipt - generate a receipt for a message, if need be
 **********************************************************************/
OSErr GenerateReceipt(MessHandle messH,short forWhat,short forWhatLocal,short actionId,short sentId)
{
	Accumulator a;
	Str255 scratch;
	Str63 date, subject, realname;
	Str15 boundary;
	OSErr err = userCancelled;
	Str15 returnStr;
	long len;
	MyWindowPtr win;
			
	PushPers(PERS_FORCE(MESS_TO_PERS(messH)));
			
	// ok, the user wants to do it
	PCopy(boundary,"\p--_");
	PCopy(returnStr,Cr);
	if (!(err=AccuInit(&a)))
	{
		// multipart/report
		AccuComposeR(&a,MIME_MP_FMT,
							 InterestHeadStrn+hContentType,
							 MIME_MULTIPART,
							 MIME_REPORT,
							 AttributeStrn+aBoundary,
							 "\p_",
							 returnStr);
		AccuAddStr(&a,GetRString(scratch,MDN_REPORT_PARAM));
		AccuAddChar(&a,'\015');
		
		// inital boundary
		AccuAddStr(&a,boundary);
		AccuAddChar(&a,'\015');
		
		// text part
		AccuComposeR(&a,MIME_TEXTPLAIN,InterestHeadStrn+hContentType,
			MIME_TEXT,MIME_PLAIN,"",returnStr);
		AccuAddChar(&a,'\015');
		CompHeadGetStr(messH,DATE_HEAD,date);
		CompHeadGetStr(messH,SUBJ_HEAD,subject);
		GetRealname(realname);
		AccuComposeR(&a,MDN_DESCRIP,date,subject,forWhatLocal,realname);
		
		// next boundary
		AccuAddStr(&a,boundary);
		AccuAddChar(&a,'\015');
		
		// disposition part
		AccuComposeR(&a,MIME_TEXTPLAIN,InterestHeadStrn+hContentType,
			MIME_MESSAGE,MDN_DISPO_NOTIFY,"",returnStr);
		AccuAddChar(&a,'\015');
		
		//AccuComposeR(&a,MDN_REPORTING_UA,returnStr);
		AccuComposeR(&a,MDN_FINAL_RECIP,GetReturnAddr(scratch,False),returnStr);
		if (!GetRHeaderAnywherePtr(messH,HeaderStrn+MSGID_HEAD,scratch+1,sizeof(scratch)-1,&len))
		{
			*scratch = len;
			AccuComposeR(&a,MDN_ORIG_MID,scratch,returnStr);
		}
		AccuComposeR(&a,MDN_DISPOSITION,actionId,sentId,forWhat,returnStr);
		AccuAddChar(&a,'\015');
		AccuAddChar(&a,'\015');
		
		//final boundary
		AccuAddStr(&a,boundary);
		AccuAddStr(&a,"\p--\015");
		
		/*
		 * message built, now create & address it
		 */
		if (!GetRHeaderAnywherePtr(messH,InterestHeadStrn+hMDN,scratch,sizeof(scratch)-1,&len))
		{
			*scratch = len-1;
			if (win=DoComposeNew(0))
			{
				MessHandle newMessH = Win2MessH(win);
				SetMessText(newMessH,TO_HEAD,scratch+1,*scratch);
				SetMessText(newMessH,0,LDRef(a.data),a.offset);
				ComposeRString(scratch,MDN_SUBJECT,subject);
				SetMessText(newMessH,SUBJ_HEAD,scratch+1,*scratch);
				SetMessOpt(newMessH,OPT_RECEIPT);
				SetMessOpt(newMessH,OPT_BULK);
				SetSig((*newMessH)->tocH,(*newMessH)->sumNum,SIG_NONE);
				QueueMessage((*newMessH)->tocH,(*newMessH)->sumNum,kEuQueue,0,true,false);
			}
		}
		AccuZap(a);
	}
	PopPers();
	return(err);
}


/**********************************************************************
 * EnsureMID - make sure a message has an id
 **********************************************************************/
OSErr EnsureMID(TOCHandle tocH,short sumNum)
{
	OSErr err = noErr;
	
	if ((*tocH)->sums[sumNum].uidHash==kNeverHashed || (*tocH)->sums[sumNum].msgIdHash==kNeverHashed)
	{
		if (!(err=CacheMessage(tocH,sumNum)))
			RehashLo(tocH,sumNum,(*tocH)->sums[sumNum].cache,true);
	}
	return(err);
}

/**********************************************************************
 * EnsureFromHash - make sure a message has a from id
 **********************************************************************/
OSErr EnsureFromHash(TOCHandle tocH,short sumNum)
{
	OSErr err = noErr;
	Str255 scratch, shortAddr;
	uLong addrHash;
	
	if ((*tocH)->sums[sumNum].fromHash==kNeverHashed)
	{
		if (!(err=CacheMessage(tocH,sumNum)))
		{
			HNoPurge((*tocH)->sums[sumNum].cache);
			
			HeaderName(FROM_HEAD); // weird--goes into scratch
			TrimWhite(scratch);
			if (*HandleHeadGetPStr((*tocH)->sums[sumNum].cache,HEADER_STRN+FROM_HEAD,scratch))
			{
				ShortAddr(shortAddr,scratch);
				MyLowerStr(shortAddr);
				addrHash = Hash(shortAddr);
			}
			else
				addrHash = kNoMessageId;
			
			HPurge((*tocH)->sums[sumNum].cache);
			
			(*tocH)->sums[sumNum].fromHash = addrHash;
			TOCSetDirty(tocH,true);
		}
	}
	return(err);
}

/**********************************************************************
 * CacheMessage - put a message into the cache
 **********************************************************************/
OSErr CacheMessage(TOCHandle tocH, short sumNum)
{
	Handle cache;
	OSErr err = noErr;
	
	if (0>sumNum || sumNum>=(*tocH)->count) return(fnfErr);

#ifdef	IMAP
	// don't do anything with IMAP messages that have not been downloaded yet.
	if ((*tocH)->sums[sumNum].offset < 0) return (fnfErr);
#endif
	
	/*
	 * is it there?
	 */
	if ((*tocH)->sums[sumNum].cache)
	{
		if (*(*tocH)->sums[sumNum].cache) return(noErr);	/* in the cache */
		else ZapHandle((*tocH)->sums[sumNum].cache);			/* wipe out remnant */
	}
	
	/*
	 * allocate it
	 */
	if (cache=NuHTempOK(GetMessageLength(tocH,sumNum)))
	{
		/*
		 * read it
		 */
		err = ReadMessage(tocH,sumNum,LDRef(cache));
		if (err) ZapHandle(cache);
		else
		{
			UL(cache);
			HPurge(cache);
			(*tocH)->sums[sumNum].cache = cache;
			ASSERT((*cache)[GetHandleSize(cache)-1]=='\015');
		}
	}
	else
		err = MemError();
	
	return(err);
}
		
/**********************************************************************
 * GetMessText - put the text of a message
 **********************************************************************/
UHandle GetMessText(MessHandle messH)
{
	MyWindowPtr win = (*messH)->win;
	TOCHandle tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;
	UHandle buffer = nil;
	long weeded;
	short tableId;
	Handle table;
	Handle cache=nil;
	Accumulator weeds;
	
	Zero(weeds);
	
	/*
	 * grab cached text, if any
	 */
	if ((*tocH)->sums[sumNum].cache && *(*tocH)->sums[sumNum].cache)
	{
		cache = (*tocH)->sums[sumNum].cache;
		HNoPurge(cache);
	}
	else
		ZapHandle((*tocH)->sums[sumNum].cache);
	
	/*
	 * allocate buffer
	 */
	if (GetMessageLength(tocH,sumNum) > (GetRLong(PETE_NIBBLE) K))
		buffer = NuHTempBetter(GetMessageLength(tocH,sumNum));
	else
		buffer = NuHTempOK(GetMessageLength(tocH,sumNum));
	if (buffer==nil)
	{
		if (!cache)
		{
			WarnUser(NO_MESS_BUF,MemError());
			return(nil);
		}
	}
	
	/*
	 * read it
	 */
	if (cache)
	{
		if (buffer)
		{
			BMD(*cache,*buffer,GetHandleSize(buffer));
			HPurge(cache);
		}
		else
		{
			buffer = cache;
			cache = nil;
			(*tocH)->sums[sumNum].cache = nil;
		}
	}
	else
	{
		LDRef(buffer);
		if (MessErr=ReadMessage(tocH,sumNum,*buffer))
			goto failure;
		UL(buffer);
		
		if (cache = NuHTempOK(GetMessageLength(tocH,sumNum)))
		{
			HPurge(cache);
			BMD(*buffer,*cache,GetMessageLength(tocH,sumNum));
			(*tocH)->sums[sumNum].cache = cache;
		}
	}
	
	/*
	 * set hash, if we haven't already
	 */
	if ((*tocH)->sums[sumNum].uidHash == kNeverHashed)
		Rehash(tocH,sumNum,buffer);
			
	/*
	 * weed headers?
	 */
	if (!(SumOf(messH)->flags & FLAG_SHOW_ALL))
	{
		WeedHeaders(buffer,&weeded,BadHeadStrn,&weeds);
		(*messH)->weeded = weeded;
		StripTrailingNewlines(buffer,SumOf(messH)->bodyOffset-weeded+1);
	}
	else
		(*messH)->weeded = 0;
	WeedHeaders(buffer,&weeded,FROM_STRN,nil);
	(*messH)->weeded += weeded;
	AccuTrim(&weeds);
	(*messH)->extras = weeds;
	
	/*
	 * translate?
	 */
	tableId = (*tocH)->sums[sumNum].tableId;
	if (tableId==DEFAULT_TABLE) tableId = GetPrefLong(PREF_IN_XLATE);
	if (tableId!=NO_TABLE)
	{
		if (!(table=GetResource_('taBL',tableId)))
		{
			tableId -= 1000;	/* try 1000 lower */
			table=GetResource_('taBL',tableId);
		}
		if (table)
		{
			Byte *p,*end,*t;
			long size = GetHandleSize_(buffer);
			end = *buffer+size;
			t = *table;
			for (p=*buffer;p<end;p++) *p = t[*p];
		}
	}
	
	win->ro = !MessOptIsSet(messH,OPT_WRITE);
	return(buffer);
	
failure:
	if (buffer) ZapHandle(buffer);
	return(nil);
}

/**********************************************************************
 * ReadMessage - read a given message into a preallocated buffer
 **********************************************************************/
int ReadMessage(TOCHandle tocH,int sumN,UPtr buffer)
{
	long count;
	Str63 name;
	short	sumNum;
	
	tocH = GetRealTOC(tocH,sumN,&sumNum);
	if (!tocH) return fnfErr;	// unable to find real TOC from virtual TOC
	GetMailboxName(tocH,sumNum,name);
	count = (*tocH)->sums[sumNum].length;

	if (!(MessErr=BoxFOpenLo(tocH,sumNum)))
	  if ((MessErr=SetFPos((*tocH)->refN,fsFromStart,
											   (*tocH)->sums[sumNum].offset)) ||
			  (MessErr=ARead((*tocH)->refN,&count,buffer)))
			FileSystemError(READ_MBOX,name,MessErr);

	return(MessErr);
}

/**********************************************************************
 * MoveMessage - transfer a message from one box to another
 * called when the transfer menu is invoked with a message frontmost
 **********************************************************************/
int MoveMessage(TOCHandle tocH,int sumNum,FSSpecPtr toSpec,Boolean copy)
{
	TOCHandle toTocH;

	CycleBalls();

	if ((toTocH = TOCBySpec(toSpec))==nil)
		return(1);
	
	if ((*toTocH)->which==OUT && !copy && !((*tocH)->sums[sumNum].flags&FLAG_OUT))
		if (ReallyDoAnAlert(XFER_TO_OUT,Caution)!=1) return(1);
	
	if (!copy)
	{
		if (SingleWarnings(tocH,sumNum,(*toTocH)->which==TRASH,False))
			return(1);
	}
	
	return(MoveMessageLo(tocH,sumNum,toSpec,copy,false,true));
}

/**********************************************************************
 * MoveMessageLo - transfer a message from one box to another, no warnings
 **********************************************************************/
int MoveMessageLo(TOCHandle tocH,int sumNum,FSSpecPtr toSpec,Boolean copy,Boolean toTemp,Boolean holdOpen)
{
	TOCHandle toTocH;
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	Str32	name;
	long	serialNum;
	short	realSumNum;
	Boolean isIMAPtoPopTransfer = false;
	
#ifdef THREADING_ON
// can't transfer a message we're sending
	if (!copy && ((*tocH)->sums[sumNum].state == BUSY_SENDING)) 
		return (-1);
#endif
	if (LogLevel&LOG_MOVE)
		ComposeLogS(LOG_MOVE,nil,"\p%p �%p,%p�  �%p�->�%p�\015", copy ? "\pCopy" : "\pTransfer",(*tocH)->sums[sumNum].from,(*tocH)->sums[sumNum].subj,GetMailboxName(tocH,sumNum,name),toSpec->name);

	CycleBalls();

	if ((toTocH = TOCBySpec(toSpec))==nil)
		return(1);

	tocH = GetRealTOC(tocH,sumNum,&realSumNum);
	sumNum = realSumNum;

#ifdef	IMAP
	// handle special transfer cases for IMAP mailbox transfers
	if ((*toTocH)->imapTOC)
	{
		OSErr err = noErr;
			
		// IMAP to IMAP. Do an IMAP transfer.
		if ((*toTocH)->imapTOC && (*tocH)->imapTOC) 
		{
			err = IMAPTransferMessage(tocH, toTocH, (*tocH)->sums[sumNum].uidHash, copy, false);
		}
		// POP to IMAP
		else if ((*toTocH)->imapTOC && !(*tocH)->imapTOC)
		{
			err = IMAPTransferMessageToServer(tocH, toTocH, sumNum, copy, false);
		}
		
		return (err);
	}
#endif
		
	if ((*tocH)->which==OUT && !copy) FixSourceStatus(tocH,sumNum);	/* in case we're deleting a reply, set orig state back */
	
	if (messH && (*messH)->subPTE && PeteIsDirty((*messH)->subPTE))
		MessSaveSub(messH);

#ifdef	IMAP
	// if this is an IMAP to POP transfer, close the message window
	if ((*tocH)->imapTOC)
	{
		// IMAP to POP.  Download the message.
		Boolean downloaded = EnsureMsgDownloaded(tocH,sumNum,true);
		
		// drop the message if a translator asked to delete it
		if ((*tocH)->sums[sumNum].opts&OPT_EMSR_DELETE_REQUESTED)
		{
			// message was fetched, but a translator deleted it.
			(*tocH)->sums[sumNum].opts |= OPT_ORPHAN_ATT;	// be real sure the attachments are left alone
#ifdef DEBUG
			ComposeLogS(LOG_PLUG,nil,"\pA plugin has deleted an IMAP message: '%p' in '%p'",(*tocH)->sums[sumNum].subj,(*tocH)->mailbox.spec.name);
#endif
			return (noErr);	
		}
		else if (!downloaded) return (1);
		
		// if the message is open, close it.				
		if (!copy && messH) 
			CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
	}
#endif

	CycleBalls();
	
#ifdef	IMAP
	// if this is an IMAP to POP transfer, the POP copy will point to the attachment
	isIMAPtoPopTransfer = tocH && (*tocH)->imapTOC && toTocH && !(*toTocH)->imapTOC;
#endif	//IMAP
	
	MessErr=AppendMessage(tocH,sumNum,toTocH,copy,toTemp,isIMAPtoPopTransfer);

	if (MessErr) return(MessErr);
	if (!holdOpen)
	{
		(void) BoxFClose(tocH,false);
		(void) BoxFClose(toTocH,true);
	}

	CycleBalls();

#ifdef	IMAP
	// if we moved an IMAP message to a POP mailbox, forget about its attachments so they don't get tidied up.
	if ((*tocH)->imapTOC && !(*toTocH)->imapTOC) (*tocH)->sums[sumNum].opts |= OPT_ORPHAN_ATT;	
#endif

	if (!copy)
#ifdef	IMAP
	{
		serialNum = (*tocH)->sums[sumNum].serialNum;
		// if this was an IMAP message, we'll need to delete it from the server.
		if ((*tocH)->imapTOC)
		{		
			MessErr = IMAPDeleteMessage(tocH, (*tocH)->sums[sumNum].uidHash, false, false, false) ? noErr : 1;
		}
		else DeleteSum(tocH,sumNum);
		//	Check for updates to search results
		SearchUpdateSum(toTocH,(*toTocH)->count-1,tocH,serialNum,true,false);
	}
#else
	{
		oldSerialNum = (*tocH)->sums[sumNum].serialNum;
		DeleteSum(tocH,sumNum);
		//	Check for updates to search results
		SearchUpdateSum(toTocH,(*toTocH)->count-1,fromTocH,serialNum,true,false);
	}
#endif
		
	CheckBox(GetWindowMyWindowPtr(FrontWindow_()),False);
	return(MessErr);
}

/**********************************************************************
 * AppendMessage - add a message to a mailbox.	Message comes from another
 * mailbox.  Things in here are a little touchy, as there are several
 * things to do, any one of which could fail.  In order, this is
 * what is done:
 * 1. Move the bytes from one mailbox to the other.
 * 2. Copy the message summary from one toc to the other, updating
 *		tocH and sumNum in the message handle (if any).
 * 3. If the message window is open, fix pointers so the message
 *		belongs to the new box, not the old one.
 * Steps 1 and 2 could fail.	In either case, no real harm is done,
 * except that we might waste some space in the new mailbox.
 * Step 3 shouldn't ever fail.
 **********************************************************************/
int AppendMessage(TOCHandle fromTocH,int fromN,TOCHandle toTocH,Boolean copy,Boolean toTemp,Boolean destHasAtt)
{
	UHandle buffer = nil;
	MSumType sum;
	long eof;
	long count;
	short err=0;
	MessHandle fromMH;
	long newBodyOffset, newLength;
	mesgErrorHandle mesgErrH;
	FSSpec	toSpec;
	/*
	 * if it's an outgoing message, save it first
	 */
	fromMH = (*fromTocH)->sums[fromN].messH;
	if (fromMH)
	{
		MyWindowPtr win = (*fromMH)->win;
		if (win->saveSize && win->position) (*win->position)(True,win);
		if ((*fromTocH)->which==OUT)
		{
			MyWindowPtr win = (*fromMH)->win;
			if ((win->isDirty||!(*fromTocH)->sums[fromN].length) &&
					!SaveComp(win)) return(1);
		}
		else if (win->isDirty)
		{
			MessFocus(fromMH,(*fromMH)->bodyPTE);
			if (!SaveMessHi(win,False)) return(1);
		}
	}
	
	/*
	 * open the relevant mailboxes
	 */
	MessErr = BoxFOpen(fromTocH);
	if (MessErr) return(MessErr);
	MessErr = BoxFOpen(toTocH);
	if (MessErr) return(MessErr);

	/*
	 * is there space?
	 */
	toSpec = GetMailboxSpec(toTocH,-1);
	if (MessErr = VolumeMargin(toSpec.vRefNum,(*fromTocH)->sums[fromN].length))
		return(MessErr);
	
	/*
	 * Are there too many messages in the destination?
	 */
	if ((*toTocH)->count >= MAX_MESSAGES_PER_MAILBOX)
	{
		Str255 s;
		ComposeStdAlert(Stop,TOO_MANY_MESSAGES,PCopy(s,(*toTocH)->mailbox.spec.name));
		return MessErr = 1;
	}
		
	/*
	 * copy the message from one to the other
	 */
	if (!copy) (*fromTocH)->sums[fromN].flags &= ~FLAG_SKIPWARN;
	eof = FindTOCSpot(toTocH,(*fromTocH)->sums[fromN].length);
	if ((*toTocH)->which == OUT && !((*fromTocH)->sums[fromN].flags&FLAG_OUT))
	{
		if (MessErr = CopyToOut(fromTocH,fromN,toTocH)) return(MessErr);
	}
	else
	{
#ifdef TWO
		if (!copy && ((*fromTocH)->which==TRASH || (*toTocH)->which==TRASH))
		{
			if (PrefIsSet(PREF_TIDY_FOLDER) && ((*fromTocH)->sums[fromN].flags & FLAG_HAS_ATT) &&
					!((*fromTocH)->sums[fromN].flags & FLAG_OUT))
				MovingAttachments(fromTocH,fromN,True,False,False,false);
			if ((*fromTocH)->sums[fromN].flags & FLAG_HAS_ATT)
				MovingAttachments(fromTocH,fromN,False,False,False,false);
		}
#endif
		if ((*fromTocH)->sums[fromN].cache && *(*fromTocH)->sums[fromN].cache)
		{
			count = (*fromTocH)->sums[fromN].length;
			HNoPurge((*fromTocH)->sums[fromN].cache);
			MessErr = SetFPos((*toTocH)->refN,fsFromStart,eof);
			if (!MessErr)
				MessErr = AWrite((*toTocH)->refN,&count,LDRef((*fromTocH)->sums[fromN].cache));
#ifdef DEBUG
			if (RunType!=Production)
			{
				long controls=0;
				UPtr dbspot = *(*fromTocH)->sums[fromN].cache;
				UPtr dbend = dbspot + (*fromTocH)->sums[fromN].length;
				for (;dbspot<dbend;dbspot++)
					if (*dbspot!='\015' && *dbspot<' ' && *dbspot!='\t') controls++;
				if (controls*20>(*fromTocH)->sums[fromN].length)
				{
					Str255 buffer;
					ComposeString(buffer,"\pWarning: %d control characters; may be a problem",controls);
					AlertStr(OK_ALRT,Stop,buffer);
				}
			}
#endif
			UL((*fromTocH)->sums[fromN].cache);
			HPurge((*fromTocH)->sums[fromN].cache);
		}
		else
			MessErr = CopyFBytes((*fromTocH)->refN,(*fromTocH)->sums[fromN].offset,
				(*fromTocH)->sums[fromN].length,(*toTocH)->refN,eof);
		if (MessErr)
		{
			LDRef(toTocH);
			FileSystemError(COPY_FAILED,toSpec.name,MessErr);
			UL(toTocH);
			return(MessErr);
		}
		(void) SetEOF((*toTocH)->refN,eof+(*fromTocH)->sums[fromN].length);
		newBodyOffset = (*fromTocH)->sums[fromN].bodyOffset;
		newLength = (*fromTocH)->sums[fromN].length;
	
		/*
		 * now, create a new summary for the copied message, and put it in the
		 * new TOC.
		 */
		sum = (*fromTocH)->sums[fromN];
		sum.offset = eof;
		sum.length = newLength;
		sum.bodyOffset = newBodyOffset;
		sum.selected = False;
		sum.messH = nil;			/* break connection with open message window */
		sum.mesgErrH = nil;	/* clear mesgErrH. add it below */
		sum.serialNum = (*toTocH)->nextSerialNum++;
		
		// Junk processing
		if ((*toTocH)->which==JUNK && sum.spamBecause==0)
		{
			sum.spamScore = GetRLong(JUNK_XFER_SCORE);
			sum.spamBecause = JUNK_BECAUSE_XFER;
		}
		else if ((*fromTocH)->which==JUNK && (*fromTocH)->sums[fromN].spamBecause == JUNK_BECAUSE_XFER)
		{
			sum.spamScore = 0;
			sum.spamBecause = 0;
		}
		
		if (copy)
			sum.cache = nil;		// because we're copying, leave the cache alone
		else
			(*fromTocH)->sums[fromN].cache = nil;	// let the cache go with the transferred message

		// the copy won't have an attachment.  That way, if it gets deleted, the attachment
		// will remain until the original is deleted. - jdboyd12/14/04
		if (copy && !destHasAtt)
			sum.flags &= ~FLAG_HAS_ATT;
		
		if (!sum.seconds) sum.seconds = GMTDateTime();
		if ((*fromTocH)->which==OUT && !toTemp)
		{
			if (sum.state!=SENT) sum.state=UNSENT;
			sum.tableId = NO_TABLE;		/* don't translate */
			sum.flags |= FLAG_OUT;
		}
			
		if ((*toTocH)->count)
		{
			MessErr = PtrPlusHand_(&sum,toTocH,sizeof(sum));
			if (MessErr)
			{
				ZapHandle(sum.cache);
				return(MessErr);
			}
		}
		else
			(*toTocH)->sums[0] = sum;
		TOCSetDirty(toTocH,true);
		(*toTocH)->reallyDirty = True;
		(*toTocH)->resort = kResortWhenever;
		(*toTocH)->count++;
		(*toTocH)->analScanned = false;
	}
	TOCSetDirty(toTocH,true);
	(*toTocH)->needRedo = MIN((*toTocH)->needRedo,(*toTocH)->count-1);
	
	/*
	 * add mesg error to new toc entry
	 */
	if (mesgErrH=(*fromTocH)->sums[fromN].mesgErrH)
	{
		Str255 errorStr;
		AddMesgError (toTocH, (*toTocH)->count-1,PCopy(errorStr,(*mesgErrH)->errorStr), (*mesgErrH)->errorCode);
	}	

	/*
	 * the message window, if any, should be closed
	 */
	if (!copy && (*fromTocH)->sums[fromN].messH)
		CloseMyWindow(GetMyWindowWindowPtr((*(MessHandle)(*fromTocH)->sums[fromN].messH)->win));

	return (MessErr);	//return(noErr);	// 12/11/97 ccw
}

/**********************************************************************
 * MoveSelectedMessages - transfer all selected messages from one mail
 * box to another.
 **********************************************************************/
int MoveSelectedMessages(TOCHandle tocH,FSSpecPtr toSpec,Boolean copy)
{
	return (MoveSelectedMessagesLo(tocH, toSpec, copy, false, true, true));
}

/**********************************************************************
 * MoveSelectedMessages - transfer all selected messages from one mail
 * box to another.
 **********************************************************************/
int MoveSelectedMessagesLo(TOCHandle tocH,FSSpecPtr toSpec,Boolean copy,Boolean delete,Boolean undo,Boolean warnings)
{
	TOCHandle toTocH;
	int sumNum;
	int lastSelected = -1;
	Str31 trashName;
	short oldCount;
	Boolean toTrash = IsRoot(toSpec) && StringSame(toSpec->name,GetRString(trashName,TRASH));
	long needRoom=0;
	Boolean outWarning;
	long count;
	uLong pTicks = TickCount();
#ifdef	IMAP
	Handle uidsH = nil;
	OSErr err = noErr;
#endif	
	TOCHandle realTocH;
	short		realSum;
	Str255 name;
		
	if ((toTocH = TOCBySpec(toSpec))==nil)
		return(1);
	outWarning = !copy && (*toTocH)->which==OUT;

	// 
	// Do transfers from the search window.  Fall through to handle POP messages.
	//
	
	if ((*tocH)->virtualTOC)
	{
		if (delete) IMAPDeleteMessagesFromSearchWindow(tocH);
		else
		{	
			// do search window transfers
			err = IMAPTransferMessagesFromSearchWindow(tocH, toTocH, copy);
			if (err != noErr) return (err);
		}		
	}

	//
	// Perform transfers to IMAP mailboxes.  There will be nothing left to do after this.
	//
	
	if ((*toTocH)->imapTOC)
	{
		return (IMAPMoveIMAPMessages(tocH, toTocH, copy));
	}
	
	//
	//	Handle the rest of the messages that have not been transferred yet.
	//
	
	if (!copy && warnings && SelectedWarnings(tocH,toTrash,False)) return(0);
		
	count = CountSelectedMessages(tocH);
	for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
		if ((*tocH)->sums[sumNum].selected)
		{
			FixSourceStatus(tocH,sumNum);	/* in case we're deleting a reply, set orig state back */
			needRoom += (*tocH)->sums[sumNum].length+sizeof(MSumType);
			if (outWarning && !((*tocH)->sums[sumNum].flags&FLAG_OUT))
			{
				outWarning = False;	/* don't need anymore */
				if (ReallyDoAnAlert(XFER_TO_OUT,Caution)!=1) return(0);
				else break;
			}
		}
	
	NoteFreeSpace(toTocH);
	if (needRoom>(*toTocH)->volumeFree-GetRLong(VOLUME_MARGIN)-sizeof(MSumType)*count)
	{
		WarnUser(NOT_ENOUGH_ROOM,dskFulErr);
		return(0);
	}
	
#ifdef TWO
	if (!copy && undo) AddXfUndo(tocH,toTocH,-1);
#endif

	if (count>10) OpenProgress();
	ProgressMessageR(kpSubTitle,LEFT_TO_TRANSFER);
	Progress(NoChange,count,nil,nil,nil);
	
	//
	// If we are transferring from an IMAP mailbox to a POP mailbox,
	// ensure that the messages have been fetched.
	//
	
	if ((*tocH)->imapTOC) 
	{
		short c, totalToDownload;
		
		// must be online to do an IMAP to POP transfer, no matter if the message is downloaded or not.
		if (!copy && Offline && GoOnline()) return (0);
		
		// go through selected messages, and make sure they've all been downloaded.
		if ((totalToDownload = CountSelectedMessages(tocH))>0)
		{
			// figure out how many of the selected messages need to be downloaded
			for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
				if ((*tocH)->sums[sumNum].selected)
					if (IMAPMessageBeingDownloaded(tocH, sumNum) || IMAPMessageDownloaded(tocH, sumNum)) totalToDownload--;
			
			// Make a handle big enough for them
			if (totalToDownload > 0)
			{
				uidsH = NuHandleClear(totalToDownload*sizeof(unsigned long));
				if (uidsH)
				{	
					// and stick them in the handle
					c = totalToDownload;
					for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
						if ((*tocH)->sums[sumNum].selected)
							if (!IMAPMessageBeingDownloaded(tocH, sumNum) && !IMAPMessageDownloaded(tocH, sumNum))
								BMD(&((*tocH)->sums[sumNum].uidHash),&((unsigned long *)(*uidsH))[--c],sizeof(unsigned long));
					
					// fetch them all in the foreground
					err = UIDDownloadMessages(tocH, uidsH, true, false);
					
					if (err == noErr)
					{
						short i;
						
						// put ALL of the messages in the mailbox. 
						for (i=0; i < totalToDownload; i++) UpdateIMAPMailbox(tocH);		
					}	
				}
				else
				{
					err = MemError();
				}	
			}
		}
		
		if (err==noErr)
		{
			// make a new handle to store uids of deleted messages.  We'll build this on the fly.
			uidsH = NuHandleClear(0);		
			if (!uidsH) err = MemError();	
		}
						
		if (err != noErr) 
		{
			if (!CommandPeriod && (err!=OFFLINE)) WarnUser(err, MEM_ERR);
			return (err);
		}
	}
	for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
	{
		if ((*tocH)->sums[sumNum].selected)
		{	
			CycleBalls();
			if (count>1) MiniEvents();
			if (EjectBuckaroo || CommandPeriod) break;
			if (!(--count%10) || TickCount()-pTicks>30)
			{
				Progress(NoBar,count,nil,nil,nil);
				pTicks = TickCount();
			}

#ifdef	IMAP
			// if this is a virtual TOC, and the message has already been processed, skip it.
			if ((*tocH)->virtualTOC && ((*tocH)->sums[sumNum].u.virtualMess.virtualMBIdx == -1)) continue;
					
			if ((*tocH)->imapTOC && !(*tocH)->virtualTOC)
			{
				// Make sure IMAP message has been downloaded and stuck in the mailbox
				if (!EnsureMsgDownloaded(tocH,sumNum,true))
					continue;	//	Couldn't download message
			}
#endif
			oldCount = (*tocH)->count;
			if (!(realTocH = GetRealTOC(tocH,sumNum,&realSum))) continue;
#ifdef	IMAP
			// if this is a virtual mailbox, and the real mailbox is an IMAP mailbox, don't do anything with this message
			if ((*tocH)->virtualTOC && (*realTocH)->imapTOC) MessErr = noErr;
			else
#endif
			MessErr=AppendMessage(realTocH,realSum,toTocH,copy,false,false);
			if (MessErr) {if (MessErr!=userCanceledErr) WarnUser(WRITE_MBOX,MessErr); break;}
			
#ifdef	IMAP
			// if we moved this IMAP message to a POP mailbox, forget about its attachments so they don't get tidied up.
			if ((*tocH)->imapTOC && !(*toTocH)->imapTOC) (*tocH)->sums[sumNum].opts |= OPT_ORPHAN_ATT;	
#endif

			// Log the transfer
			if (LogLevel&LOG_MOVE)
				ComposeLogS(LOG_MOVE,nil,"\p%p �%p,%p�  �%p�->�%p�\015", copy ? "\pCopy" : "\pTransfer",(*realTocH)->sums[realSum].from,(*realTocH)->sums[realSum].subj,GetMailboxName(realTocH,realSum,name),toSpec->name);

			if (oldCount!=(*tocH)->count)
			{
				lastSelected = sumNum;
				sumNum--;
			}
			else if (!copy)
			{
				long	serialNum = (*realTocH)->sums[realSum].serialNum;
				
				if (!(*tocH)->virtualTOC)
				{
#ifdef IMAP
					if ((*tocH)->imapTOC && uidsH)
					{	
						//	If not copying, we'll need to delete original IMAP message when done.
						if (uidsH && !copy)
						{
							unsigned long	uid = (*tocH)->sums[sumNum].uidHash;
							err = PtrAndHand(&uid,uidsH,sizeof(uid));
							if (err != noErr)
							{
								WarnUser(err,MEM_ERR);
								ZapHandle(uidsH);
								return(err);
							}
						}
					}
#endif
					else if (!DeleteSum(tocH,sumNum))
					{ 
						lastSelected = sumNum;
						sumNum--; 		/* back up, so we can try again */
					}
					else
						lastSelected = sumNum;
				}

				if (realTocH != tocH)
					// delete real message summary
					DeleteSum(realTocH,realSum);

				//	Check for updates to search results
				SearchUpdateSum(toTocH,(*toTocH)->count-1,realTocH,serialNum,true,false);
			}
		}
	}

#ifdef IMAP
	// now delete the IMAP messages that were transferred, if the transfer completed successfully.
	if (!copy && (*tocH)->imapTOC && (MessErr==noErr) && !CommandPeriod && !EjectBuckaroo)
	{	
		// uidsH contains uids of messages that have been successfully transferred.
		IMAPDeleteMessages(tocH,uidsH,false,false,false,false);
	}
#endif

	(void) BoxFClose(tocH,false);
	(void) BoxFClose(toTocH,true);
	CloseProgress();

	if ((*tocH)->win && !copy && !CommandPeriod) BoxSelectAfter((*tocH)->win,lastSelected);
	CheckBox(GetWindowMyWindowPtr(FrontWindow_()),False);
#ifdef TWO
	if (MessErr) NukeXfUndo();
#endif
	ShowBoxSizes((*tocH)->win);
	return(MessErr);
}

/**********************************************************************
 * SelectedWarnings - Warnings for the selected message
 **********************************************************************/
OSErr SelectedWarnings(TOCHandle tocH,Boolean toTrash,Boolean nuke)
{
	Boolean queuedWarning, unsentWarning, unreadWarning, busyWarning;
	OSErr err = noErr;
	short i;
	
	MessageWarnings(tocH,-1,toTrash,nuke,&queuedWarning,&unsentWarning,&unreadWarning, &busyWarning);
	
	for (i=0;i<(*tocH)->count && !err;i++)
		if ((*tocH)->sums[i].selected)
			err =  MessageWarnings(tocH,i,toTrash,nuke,
								&queuedWarning,&unsentWarning,&unreadWarning,&busyWarning);
	
	return(err);
}

/**********************************************************************
 * SingleWarnings - Give warnings for a single message
 **********************************************************************/
OSErr SingleWarnings(TOCHandle tocH,short sumNum,Boolean toTrash,Boolean nuke)
{
	Boolean queuedWarning, unsentWarning, unreadWarning, busyWarning;
	
	MessageWarnings(tocH,-1,toTrash,nuke,&queuedWarning,&unsentWarning,&unreadWarning,&busyWarning);
	
	return(MessageWarnings(tocH,sumNum,toTrash,nuke,
								&queuedWarning,&unsentWarning,&unreadWarning,&busyWarning));
}


/**********************************************************************
 * 
 **********************************************************************/
OSErr MessageWarnings(TOCHandle tocH, short sumNum,Boolean toTrash,Boolean nuke,
	Boolean *queuedWarning,Boolean *unsentWarning,Boolean *unreadWarning,Boolean *busyWarning)
{
	if (sumNum<0)
	{
		*queuedWarning = (*tocH)->which==OUT && !PrefIsSet(PREF_EASY_DEL_QUEUED);
		*unreadWarning = !PrefIsSet(PREF_EASY_DEL_UNREAD);
		*unsentWarning = (*tocH)->which==OUT && !PrefIsSet(PREF_EASY_DEL_UNSENT);
		*busyWarning = true;	// for now, until/if we get a pref for it
	}
	else
	{
		short button, verb;
		
		if (toTrash)
			if (nuke)
			{
				button = NUKE_BTN;
				verb = NUKE_VERB;
			}
			else
			{
				button = TRASH_BTN;
				verb = TRASH_VERB;
			}
		else
		{
			button = XFER_BTN;
			verb = XFER_VERB;
		}

		// clarence 4/25/97
		if ((*tocH)->sums[sumNum].state == BUSY_SENDING) {
			WarnUser (SENDING_WARNING, 0);
			*busyWarning = False;
			return(1);
		}

		if ((*tocH)->sums[sumNum].flags&FLAG_SKIPWARN) return(noErr);
		if (toTrash && *unreadWarning && ((*tocH)->sums[sumNum].state==UNREAD))
		{
			if (!Mom(button,0,PREF_EASY_DEL_UNREAD,UNREAD_WARNING,verb)) return(1);
			*unreadWarning = False; /* been there, done that. */
		}
		if (*queuedWarning && IsQueued(tocH,sumNum))
		{
			if (!Mom(button,0,PREF_EASY_DEL_QUEUED,QUEUED_WARNING,verb)) return(1);
			*queuedWarning = *unsentWarning = False; /* been there, done that. */
		}
		if (*unsentWarning && ((*tocH)->sums[sumNum].state!=SENT))
		{
			if (!Mom(button,0,PREF_EASY_DEL_UNSENT,UNSENT_WARNING,verb)) return(1);
			*unsentWarning = False; /* been there, done that. */
		}
	}
	return(noErr);
}


/**********************************************************************
 * FixSourceStatus - fix the status of a source message
 **********************************************************************/
void FixSourceStatus(TOCHandle tocH,short sumNum)
{
	MessHandle messH = (*tocH)->sums[sumNum].messH;
	TOCHandle sourceTocH;
	short sourceNum;
	uLong **midList;
	short i;
	
	if ((*tocH)->which == OUT && messH && SumOf(messH)->state != SENT &&(*messH)->aSourceMID.offset)
	{
		midList = (*messH)->aSourceMID.data;
		for (i=(*messH)->aSourceMID.offset/(3*sizeof(long))-1;i>=0;i--)
		{
			uLong sourceMID = (*midList)[i*3];
			short sourceOrigState = (*midList)[i*3+1];
			short sourceNewState = (*midList)[i*3+2];
			if (sourceMID && sourceMID != kNeverHashed && sourceOrigState != sourceNewState)
			{
				if (!FindMessageByMID(sourceMID,&sourceTocH,&sourceNum))
				{
					if (sourceNewState == (*sourceTocH)->sums[sourceNum].state)
						SetState(sourceTocH,sourceNum,sourceOrigState);
				}
			}
		}
		AccuZap((*messH)->aSourceMID);
	}
}

/**********************************************************************
 * FindMessageByMID - find a message by mid.  Works only on open toc's at the moment
 **********************************************************************/
OSErr FindMessageByMID(uLong mid,TOCHandle *tocH,short *sumNum)
{
	TOCHandle lTocH;
	long lsum;

	for (lTocH=TOCList;lTocH;lTocH = (*lTocH)->next)
	{
		if (!TOCFindMessByMID(mid,lTocH,&lsum))
		{
			*sumNum = lsum;
			*tocH = lTocH;
			return(noErr);
		}
	}
	return(fnfErr);
}

/**********************************************************************
 * TOCFindMessByMID - find a message in a toc by uid hash
 **********************************************************************/
OSErr TOCFindMessByMID(uLong mid,TOCHandle tocH,long *sumNum)
{
	short lSumNum;
	
	for (lSumNum=(*tocH)->count-1;lSumNum>=0;lSumNum--)
		if ((*tocH)->sums[lSumNum].uidHash == mid)
		{
			*sumNum = lSumNum;
			return(noErr);
		}
 	return(fnfErr);
}

/**********************************************************************
 * TOCFindMessByMsgID - find a message in a toc by message id
 **********************************************************************/
OSErr TOCFindMessByMsgID(uLong mid,TOCHandle tocH,long *sumNum)
{
	short lSumNum;
	
	for (lSumNum=(*tocH)->count-1;lSumNum>=0;lSumNum--)
		if ((*tocH)->sums[lSumNum].msgIdHash == mid)
		{
			*sumNum = lSumNum;
			return(noErr);
		}
 	return(fnfErr);
}

#ifdef TWO
/************************************************************************
 * MovingAttachments - we're moving attachments into or out of the trash
 ************************************************************************/
void MovingAttachments(TOCHandle tocH,short sumNum,Boolean attach,Boolean wipe,Boolean nuke,Boolean IMAPStubsOnly)
{
	FSSpec attFolder, trashFolder;
	FSSpecPtr fromSpec, toSpec;
	
	if (attach)
	{
		if (GetAttFolderSpec(&attFolder)) return;	/* no folder set */
	}
	else
		SubFolderSpec(PARTS_FOLDER,&attFolder);
	if (GetTrashSpec(attFolder.vRefNum,&trashFolder)) return;	/* oh well; unimportant error */
	if (!nuke && (*tocH)->which==TRASH)
	{
		fromSpec = &trashFolder;
		toSpec = &attFolder;
	}
	else
	{
		fromSpec = &attFolder;
		toSpec = &trashFolder;
	}
	MovingAttachmentsLo(tocH,sumNum,attach,wipe,nuke,IMAPStubsOnly,fromSpec,toSpec);
}

/************************************************************************
 * MovingAttachmentsLo - we're moving attachments to specific folders
 ************************************************************************/
void MovingAttachmentsLo(TOCHandle tocH,short sumNum,Boolean attach,Boolean wipe,Boolean nuke,Boolean IMAPStubsOnly,FSSpecPtr fromSpec,FSSpecPtr toSpec)
{
	Handle text;
	long offset;
	FSSpec attSpec, dupSpec, newSpec;
	Boolean iOpened = (*tocH)->sums[sumNum].messH == nil;
	TOCHandle attTOCH;
	FInfo info;
	
	CacheMessage(tocH,sumNum);
	if (!(text=(*tocH)->sums[sumNum].cache)) return;
	HNoPurge(text);
	offset = (*tocH)->sums[sumNum].bodyOffset-1;
	
	while (0<=(offset = FindAnAttachment(text,offset+1,&attSpec,attach,nil,nil,nil)))
	{
#ifdef IMAP
		if (IsIMAPAttachmentStub(&attSpec))
		{
			//	Just delete IMAP attachment stubs
			if (wipe || nuke) FSpDelete(&attSpec);
			continue;
		}
		if (IMAPStubsOnly)
			continue;
#endif
		if (SameVRef(attSpec.vRefNum,toSpec->vRefNum) &&
				(!fromSpec || AttStillInFolder(&attSpec,fromSpec)))
		{
			/*
			 * is it a mailbox?
			 */
			FSpGetFInfo(&attSpec,&info);
			if (!(info.fdFlags&kIsAlias) && IsMailbox(&attSpec))
			{
				dupSpec = attSpec;
				
				/*
				 * if it's open, we must close it
				 */
				if (attTOCH = FindTOC(&attSpec))
				{
					Boolean oldSuper = PrefIsSet(PREF_SUPERCLOSE);
					if (!oldSuper) TogglePref(PREF_SUPERCLOSE);
					CloseMyWindow(GetMyWindowWindowPtr((*attTOCH)->win));
					if (!oldSuper) TogglePref(PREF_SUPERCLOSE);
				}
				Box2TOCSpec(&attSpec,&dupSpec);
				
				if (wipe) WipeSpec(&dupSpec);
				else SpecMove(&dupSpec,toSpec);  /* if toc move fails, at least we tried */
				
				/*
				 * now, if in the menus, kill the menu item
				 */
				if (!FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,attSpec.name,&dupSpec))
				{
					if (IsAlias(&dupSpec,&dupSpec) && SameSpec(&attSpec,&dupSpec))
					{
						FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,attSpec.name,&dupSpec);
						RemoveBoxHigh(&dupSpec);
						FSpDelete(&dupSpec);	/* get rid of alias */
						Box2TOCSpec(&dupSpec,&dupSpec);
						FSpDelete(&dupSpec);	/* and toc alias */
					}
				} 
			}
			
			/*
			 * move the file into the trash
			 */
			if (wipe) WipeSpec(&attSpec);
			else if (dupFNErr==SpecMove(&attSpec,toSpec))
			{
				/* dup filename.  rename. */
				dupSpec = *toSpec;
				PCopy(dupSpec.name,attSpec.name);
				newSpec = dupSpec;
				UniqueSpec(&newSpec,31);
				if (!FSpRename(&dupSpec,newSpec.name))
					SpecMove(&attSpec,toSpec);
			}
		}
	}
}

/************************************************************************
 * AttStillInFolder - is an attachment still in the attachments folder?
 ************************************************************************/
Boolean AttStillInFolder(FSSpecPtr att, FSSpecPtr folder)
{
	if (PrefIsSet(PREF_ATT_SUBFOLDER_TRASH))
		return(SpecInSubfolderOf(att,folder));
	else
		return(SameVRef(att->vRefNum,folder->vRefNum) && att->parID==folder->parID);
}
#endif

/************************************************************************
 * FindAnAttachment - find an attachment from a line of text
 ************************************************************************/
long FindAnAttachment(Handle text,long offset,FSSpecPtr spec,Boolean attach,uLong *cid, uLong *relURL, uLong *absURL)
{
	UPtr spot,newLine,end;
	Boolean result=False;
	Str255   line;
	
	LDRef(text);
	end = *text + GetHandleSize_(text);
	spot=*text+offset;
	while ( spot<end && *spot++!='\015');
	
	for (;spot<end;spot=newLine+1)
	{
		for (newLine=spot;newLine<end && *newLine!='\015';newLine++);
		if (newLine-spot>24 && newLine-spot<255)
		{
			MakePStr(line,spot,newLine-spot);
			if (attach)
			{
				result = !AttLine2Spec(line,spec,False);
				if (result) break;
			}
			else
			{
				result = !RelLine2Spec(line,spec,cid,relURL,absURL);
				if (result) break;
			}
		}
	}
	offset = result ? spot-*text : -1;
	UL(text);
	return(offset);
}

/**********************************************************************
 * InitAttachmentFinder - initialize data for finding attachments in 
 *    outgoing or received messages
 **********************************************************************/
void InitAttachmentFinder(FindAttPtr pData,Handle text,Boolean attach,TOCHandle tocH,MSumPtr sum)
{
   pData->text = text;
   pData->outgoing = (*tocH)->which==OUT || sum->state==SENT || sum->state==UNSENT || sum->flags & FLAG_OUT;
   pData->attach = attach;
   if (pData->outgoing)
   {
      // Outgoing message. Find Attachments header
   	Str63 hdrName;
   
      GetRString(hdrName,HeaderStrn+ATTACH_HEAD);   
      HandleHeadFindStr(text,hdrName,&pData->hs);
   }
   else
   {
      // Received message. Start at beginning of message body
      pData->offset = sum->bodyOffset-1;
   }
}

/**********************************************************************
 * GetNextAttachment - find next attachment
 **********************************************************************/
Boolean GetNextAttachment(FindAttPtr pData,FSSpecPtr spec)
{
   Boolean  result = false;
   
   if (pData->text)
   {
      if (pData->outgoing)
      {
         result = pData->attach ? (GetIndAttachmentLo(pData->text,1,spec,nil,&pData->hs)!=1) : false;
      }
      else
      {
         pData->offset = FindAnAttachment(pData->text,pData->offset,spec,pData->attach,nil,nil,nil);
         result = pData->offset != -1;
      }
      if (!result)
         // No more attachments. Signal we are done
         pData->text = nil;
   }
   return result;
}

/**********************************************************************
 * DeleteMessage - delete a summary from a toc, and fix the screen, too
 **********************************************************************/
void DeleteMessage(TOCHandle tocH, int sumNum,Boolean nuke)
{	
#ifdef	IMAP
	if ((*tocH)->imapTOC) 
	{
		// close the IMAP message to be deleted, even if it's just going to be marked.
		MessHandle messH = (MessHandle)(*tocH)->sums[sumNum].messH;
		if (messH) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
		
		IMAPDeleteMessage(tocH, (*tocH)->sums[sumNum].uidHash, nuke, false, false);

		return;
	}
#endif

	if (SingleWarnings(tocH,sumNum,True,nuke||(*tocH)->which==TRASH)) return;
	DeleteMessageLo(tocH,sumNum,nuke);
}

/**********************************************************************
 * DeleteMessage - delete a summary from a toc, and fix the screen, too
 **********************************************************************/
void DeleteMessageLo(TOCHandle tocH, int sumNum,Boolean nuke)
{
	MessHandle messH = (MessHandle)(*tocH)->sums[sumNum].messH;
	Boolean dirt = 0;
	FSSpec trashSpec;
	int oldN = (*tocH)->count;
	Boolean wipe = PrefIsSet(PREF_WIPE)&&((*tocH)->sums[sumNum].opts&OPT_WIPE);
	
	if ((*tocH)->which!=TRASH && !wipe && !nuke)
	{
		trashSpec.vRefNum = MailRoot.vRef;
		trashSpec.parID = MailRoot.dirId;
		GetRString(trashSpec.name,TRASH);
		MoveMessageLo(tocH,sumNum,&trashSpec,False,False,true);
	}
	else
	{
#ifdef TWO
		if (!(*tocH)->imapTOC) NukeXfUndo();
#endif	
		if (wipe) WipeMessage(tocH,sumNum);
#ifdef TWO
		if ((!(*tocH)->imapTOC) && PrefIsSet(PREF_SERVER_DEL))
		{
			AddTSToPOPD(DELETE_ID,tocH,sumNum,False);
		}
		if ((*tocH)->sums[sumNum].opts & OPT_HAS_SPOOL) RemSpoolFolder((*tocH)->sums[sumNum].uidHash);
		if (nuke &&((*tocH)->sums[sumNum].flags&FLAG_HAS_ATT))
		{
#ifdef IMAP
			// move the attachment to the trash if the pref is set, as long as they haven't been orphaned.
			if (PrefIsSet(PREF_TIDY_FOLDER) && !((*tocH)->sums[sumNum].opts & OPT_ORPHAN_ATT)) 
				MovingAttachments(tocH,sumNum,True,wipe,True,false);
			else 
				MovingAttachments(tocH,sumNum,True,wipe,True,true);
#else
			if (PrefIsSet(PREF_TIDY_FOLDER)) MovingAttachments(tocH,sumNum,True,wipe,True,false);
#endif

#ifdef	IMAP
			// move the inline parts to the trash, unless this is an IMAP message that has been copied to a local mailbox
			if (!((*tocH)->sums[sumNum].opts & OPT_ORPHAN_ATT))
#endif
				MovingAttachments(tocH,sumNum,False,wipe,True,false);
		}
#endif
		if (messH) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
		if ((*tocH)->count==oldN) DeleteSum(tocH,sumNum);
	}
}

/**********************************************************************
 * WipeMessage - clear the contents of a message, really
 **********************************************************************/
OSErr WipeMessage(TOCHandle tocH,short sumNum)
{
	OSErr err;
	
	MovingAttachments(tocH,sumNum,True,True,False,false);
	MovingAttachments(tocH,sumNum,False,True,False,false);
	
	if (err=BoxFOpen(tocH)) return(err);
	
	err = WipeDiskArea((*tocH)->refN,(*tocH)->sums[sumNum].offset,(*tocH)->sums[sumNum].length);
	return err;
}

/************************************************************************
 * MessageError - return the most recent error code from these functions
 ************************************************************************/
int MessageError(void)
{
	return(MessErr);
}

/**********************************************************************
 * StripTrailingNewlines - strip the newlines off the end of a text block
 **********************************************************************/
long StripTrailingNewlines(Handle buffer,long stop)
{
	long size = GetHandleSize(buffer);
	Ptr spot = *buffer + size;
	long newSize;
	
	while (spot[-1]=='\015' && spot>*buffer+stop) spot--;
	
	newSize = spot - *buffer;
	SetHandleSize(buffer,newSize);
	return(size-newSize);
}
	

/************************************************************************
 * WeedHeaders - weed a message's headers, leaving only the interesting
 * ones.
 ************************************************************************/
void WeedHeaders(UHandle buffer,long *weeded,short toWeed,AccuPtr weeds)
{
	long size;
	short bad;
	Handle badH = GetResource('STR#',toWeed);
	UPtr badSpot;	//pointer to bad header string we're currently looking for
	Byte old;
	short res;
	
	if (badH)
	{
		UPtr spot;	// char we're examining
		UPtr done;	// just past end of good chars that have been copied
		UPtr end;		// end of the whole shebang
		UPtr found;	// beginning of text we found
		short badN;
		
		HNoPurge(badH);
		PtrAndHand("",badH,1);	/* room for a null terminator */
		LDRef(badH);
		
		done = spot = LDRef(buffer);
		badN = CountStrn(toWeed);
		end = spot + GetHandleSize_(buffer);

		// while there are chars left
		while (spot<end)
		{
			if (*spot=='\015') break;	//two returns in a row are the end of the headers
			
			// search for the header name
			badSpot = *badH+2;
			for (bad=0;bad<badN;bad++)
			{
				old = badSpot[*badSpot+1];
				badSpot[*badSpot+1] = 0;
				res = striscmp(spot,badSpot+1);
				badSpot[*badSpot+1] = old;
				// found it!
				if (!res)
				{
					// skip the header
					found = spot;
					while (spot<end)
						if (*spot++=='\015')
							if (*spot!=' ' && *spot!='\t') break;
					// copy?
					if (weeds) AccuAddPtr(weeds,found,spot-found);
					goto nextHead;	// continue looking at next header
				}
				else badSpot += *badSpot+1;
			}
			// copy the current header
			while (spot<end)
				if ((*done++ = *spot++)=='\015')
					if (*spot!=' ' && *spot!='\t') break;
			nextHead:;
		}
		
		while (spot<end) *done++ = *spot++;
		size = done - *buffer;
		if (weeded) *weeded = end-done;
		UL(badH);
		HPurge(badH);
		HUnlock(buffer);
		SetHandleBig_(buffer,size);
	}
}

/************************************************************************
 * SetMessText - stick some text into one of the fields of a message.
 ************************************************************************/
OSErr SetMessText(MessHandle messH,short whichTXE,UPtr string,long size)
{
	HeadSpec hs;
	
	if (CompHeadFind(messH,whichTXE,&hs))
		return(CompHeadSetPtr(TheBody,&hs,string,size));
	else return(fnfErr);
}

/**********************************************************************
 * Fix1MessServerArea - fix the server display of a single message
 **********************************************************************/
void Fix1MessServerArea(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	uLong uidHash;
	MessHandle messH;
	ControlHandle fetch;
	ControlHandle trash;
	Boolean onFetch, onDelete, lmos;
	OSType popdType;

	PushGWorld();

	if (GetWindowKind(winWP)==MESS_WIN && IsWindowVisible(winWP))
	{
		messH = Win2MessH(win);
		uidHash = SumOf(messH)->uidHash;
		popdType = PERS_POPD_TYPE(MESS_TO_PPERS(messH));
		
		fetch = FindControlByRefCon(win,mcFetch);
		trash = FindControlByRefCon(win,mcTrash);
		
		if (IdIsOnPOPD(popdType,POPD_ID,uidHash))
		{
			lmos = PrefIsSet(PREF_LMOS);
			onFetch = IdIsOnPOPD(popdType,FETCH_ID,uidHash);
			onDelete = IdIsOnPOPD(popdType,DELETE_ID,uidHash);
			if ((*messH)->hasFetchIcon = MessFlagIsSet(messH,FLAG_SKIPPED))
				if (fetch) {SetControlValue(fetch,onFetch); ShowControl(fetch);}
			(*messH)->hasDelIcon = True;
			if (trash)
			{
				SetControlValue(trash,onDelete);
				HiliteControl(trash,onFetch && !lmos ? 255 : 0);
				ShowControl(trash);
			}
		}
		else
		{
			(*messH)->hasFetchIcon = (*messH)->hasDelIcon = False;
			if (fetch) HideControl(fetch);
			if (trash) HideControl(trash);
		}
	}
	else if (GetWindowKind(winWP)==MBOX_WIN || GetWindowKind(winWP)==CBOX_WIN)
	{
		InvalTocBox((TOCHandle)GetMyWindowPrivateData(win),-2,blServer);
	}
	PopGWorld();
}

/**********************************************************************
 * RecordTransAttachments - record the fact that we have attachments from a translator
 **********************************************************************/
OSErr RecordTransAttachments(FSSpecPtr spec)
{
	WindowPtr	InsertWinWP = GetMyWindowWindowPtr (InsertWin);
	MessHandle messH;
	FSSpecHandle h;
	
	if (InsertWin && GetWindowKind(InsertWinWP)==MESS_WIN)
	{
		messH = Win2MessH(InsertWin);
		if (!(*messH)->etlFiles)
		{
			h = NuHTempBetter(0);
			if (!h) return(MemError());
			(*messH)->etlFiles = h;
		}
#ifdef NEVER
		if (RunType!=Production)
		{
			Str255 title;
			MyGetWTitle(InsertWinWP,title);
			Dprintf("\p;log RecordTransAttachments;sc;g");
			Dprintf("\p�%p�;g",title);
			Dprintf("\p;log;g",spec->name);
		}
#endif
		return(PtrPlusHand_(spec,(*messH)->etlFiles,sizeof(*spec)));
	}
	return noErr;
}

/************************************************************************
 * CleanSpoolFolder - clean up the spool folder
 ************************************************************************/
OSErr CleanSpoolFolder(uLong age)
{
	CInfoPBRec hfi;
	FSSpec spec;
	
	if (SubFolderSpec(SPOOL_FOLDER,&spec))
		return(noErr);
		
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = spec.name;
	age = LocalDateTime()-24*3600*age;
	while (!DirIterate(spec.vRefNum,spec.parID,&hfi))
	{
		if (EventPending()) return(userCanceledErr);
		MiniEvents();
		if (AllDigits(spec.name+1,*spec.name))	// we have a spool folder
		if (hfi.dirInfo.ioDrMdDat<age)
		if (!RemoveDir(&spec)) hfi.hFileInfo.ioFDirIndex--;
	}
	return(noErr);
}


/************************************************************************
 * AppendMessText - stick some text after one of the fields of a message.
 ************************************************************************/
OSErr AppendMessText(MessHandle messH,short whichTXE,UPtr string,long size)
{
	HeadSpec hs;
	
	if (CompHeadFind(messH,whichTXE,&hs))
	{
		return(CompHeadAppendPtr(TheBody,&hs,string,size));
	}
	else return(fnfErr);
}

/************************************************************************
 * MessPlainBytes - make sure bytes are plain
 ************************************************************************/
OSErr MessPlainBytes(MessHandle messH,short whichTXE,short bytes)
{
	OSErr err = noErr;
	HeadSpec hs;
	long start,stop;
	
	if (!CompHeadFind(messH,whichTXE,&hs))
		err = fnfErr;
	else
	{
		if (bytes<0)
		{
			start = hs.stop+bytes;
			start = MAX(start,hs.value);
			stop = hs.stop;
		}
		else
		{
			start = hs.value;
			stop = start+bytes;
		}
		PeteParaConvert(TheBody,start,stop);
		PetePlain(TheBody,start,stop,peAllValid);
		PeteParaRange(TheBody,&start,&stop);
		if (!whichTXE && bytes<0) PetePlainPara(TheBody,kPETELastPara);
		PetePlainParaAt(TheBody,start,stop);
	}
	return err;
}

/************************************************************************
 * DoIterativeThingyLo - do something over all selected messages
 ************************************************************************/
void DoIterativeThingyLo(TOCHandle tocH,int item,long modifiers,TextAddrHandle addr,Boolean warnings)
{
	int sumNum;
	short toWhom = (short) addr;
	short lastSelected = -1;
	Str255 title;
	FSSpec trashSpec;
	long count;
	long size;
	long gran;
	uLong pTicks = TickCount();
	Boolean nuke = item==MESSAGE_DELETE_ITEM&&((*tocH)->which==TRASH || (optionKey|shiftKey)==(modifiers&(optionKey|shiftKey)))&&(!(*tocH)->imapTOC||PrefIsSet(PREF_ALLOW_IMAP_NUKE));
#ifdef THREADING_ON
	Boolean	busy = false;
#endif
	uLong oldEzOpenSerialNum = (*tocH)->previewID ? (*tocH)->ezOpenSerialNum: 0;
	
#ifdef PERF
	PerfControl(ThePGlobals,True);
#endif PERF

#ifdef	IMAP
	if (item==MESSAGE_DELETE_ITEM && (*tocH)->imapTOC && !nuke) 
	{
		Handle uids = nil;
		long c =  CountSelectedMessages(tocH);
		long sumNum;
		
		// only do something if there are some selected messages
		if (c)
		{
			// build a list of uids to be deleted
			uids = NuHandleClear(c*sizeof(unsigned long));
			if (uids)
			{
				LDRef(tocH);
				for (sumNum=0;sumNum<(*tocH)->count && c;sumNum++)
					if ((*tocH)->sums[sumNum].selected)
					{
						// Close this message if it's open
						if ((*tocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[sumNum].messH)->win));
										
						BMD(&((*tocH)->sums[sumNum].uidHash),&((unsigned long *)(*uids))[--c],sizeof(unsigned long));
					}
				UL(tocH);
				
				// and delete them.
				IMAPDeleteMessages(tocH, uids, nuke, false, false, false);
				
				// preview the next message ...
				if (oldEzOpenSerialNum && (sumNum=FindSumBySerialNum(tocH,oldEzOpenSerialNum))>=0 && !(modifiers&shiftKey))
					Preview(tocH,sumNum);
			}
			else
			{
				WarnUser(MemError(), MEM_ERR);
			}
		}
				
		return;
	}
#endif

	if (item==MESSAGE_DELETE_ITEM && !nuke)
	{
		trashSpec.vRefNum = MailRoot.vRef;
		trashSpec.parID = MailRoot.dirId;
		GetRString(trashSpec.name,TRASH);
		MoveSelectedMessagesLo(tocH,&trashSpec,False,True,true,warnings);
		if (oldEzOpenSerialNum && (sumNum=FindSumBySerialNum(tocH,oldEzOpenSerialNum))>=0 && !(modifiers&shiftKey))
			Preview(tocH,sumNum);
		(*tocH)->userActive = !(modifiers&shiftKey);
		return;
	}
	
	if (nuke && warnings && SelectedWarnings(tocH,True,True)) return;
	
	// memory preflight
	count = CountSelectedMessages(tocH);
	if (item==MESSAGE_FORWARD_ITEM || item==MESSAGE_REDISTRIBUTE_ITEM || item==MESSAGE_REPLY_ITEM || item==MESSAGE_SALVAGE_ITEM)
	{
		if (item==MESSAGE_REDISTRIBUTE_ITEM && PrefIsSetOrNot(PREF_TURBO_REDIRECT,modifiers,optionKey) && toWhom)
			;	// nevermind, turbo redirect
		else
		{
			size = count * 7 K;	// account just for the windows
			if (item!=MESSAGE_REPLY_ITEM || !(modifiers&shiftKey))
			size += SizeSelectedMessages(tocH,true);
			if (MemoryPreflight(size)) return;	// too much memory asked for
			
			if (count > GetRLong(WIN_GEN_WARNING_THRESH))
				if (!MultiMessageOpOK(WIN_GEN_WARNING,count))
					return;
		}
	}
	
	
#ifdef TWO
	if (item == STATE_HIER_MENU)
		toWhom = Item2Status(toWhom);
#endif

	/*
	 * progress stuff
	 */
	switch (item)
	{
		case PERS_HIER_MENU:
			//No Personalities menu in Light
			if (HasFeature (featureMultiplePersonalities))
				gran = (*tocH)->which==OUT ? 1 : 50;
			break;
		case MESSAGE_DELETE_ITEM:
			gran = 10;
			break;
		case STATE_HIER_MENU:
		case LABEL_HIER_MENU:
		case PRIOR_HIER_MENU:
		case TABLE_HIER_MENU:
		case SERVER_HIER_MENU:
			gran = 50;
			break;
		default:
			gran = 1;
			break;
	}	
	
#ifdef	IMAP
	if ((*tocH)->imapTOC				// some IMAP operations will open their own progress window
		&& (item==MESSAGE_DELETE_ITEM	// no progress for deletes from the message menu, or server menu options
			|| (item == SERVER_HIER_MENU)));
	else
	{
#endif
		if (count>gran) OpenProgress();
		ProgressMessageR(kpSubTitle,LEFT_TO_PROCESS);
		Progress(NoBar,count,nil,nil,nil);
#ifdef	IMAP
	}
#endif

	if (!nuke && item==MESSAGE_DELETE_ITEM) 	AddXfUndo(tocH,GetTrashTOC(),-1);
		
	for (sumNum=(*tocH)->count-1;sumNum>=0;sumNum--)
	{
		if ((*tocH)->sums[sumNum].selected)
		{
			Boolean	doVirtualMB;
			TOCHandle	realTOC;
			short		realSum;
			
			lastSelected = sumNum;
			MakeMessTitle(title,tocH,sumNum,False);
			if (count>1) MiniEvents();
			if (CommandPeriod) break;
			if (!(--count%gran) || TickCount()-pTicks>30)
			{
				Progress(NoBar,count,nil,nil,title);
				pTicks = TickCount();
			}
			realTOC = GetRealTOC(tocH,sumNum,&realSum);
			if (nuke && realTOC == tocH)
				SearchUpdateSum(tocH,sumNum,tocH,(*tocH)->sums[sumNum].serialNum,false,true);				
			doVirtualMB = DoMessageMenu(item,tocH,sumNum,toWhom,addr,modifiers,nuke,&busy);
			if (!nuke && realTOC == tocH && doVirtualMB)
				//	Check for updates to search results
				SearchUpdateSum(tocH,sumNum,tocH,(*tocH)->sums[sumNum].serialNum,false,false);
			if (realTOC && realTOC != tocH && doVirtualMB)
				// do real mailbox also if working in virtual mailbox
				DoMessageMenu(item,realTOC,realSum,toWhom,addr,modifiers,nuke,&busy);
#ifdef	IMAP
			// hack: The  Delete from Server, Fetch Message Text, and Fetch Attachments menu choices handle all selected messages for IMAP boxes
			if ((*tocH)->imapTOC && (item == SERVER_HIER_MENU) && ((toWhom == isvmDelete) || (toWhom == isvmFetchMessage) || (toWhom == isvmFetchAttachments))) break;
#endif
		}
		MonitorGrow(True);
	}
out:
	
	CloseProgress();
	ShowBoxSizes((*tocH)->win);
#ifdef THREADING_ON
	if (busy)
		WarnUser (SENDING_WARNING, 0);
#endif
	if (!CommandPeriod)
	{
#ifdef TWO
		if (item==MESSAGE_REDISTRIBUTE_ITEM &&
				PrefIsSetOrNot(PREF_TURBO_REDIRECT,modifiers,optionKey) &&
				!(modifiers&shiftKey)
				&& toWhom)
			DoIterativeThingy(tocH,MESSAGE_DELETE_ITEM,0,0);
#endif
		BoxSelectAfter((*tocH)->win,lastSelected);
	}
	if (oldEzOpenSerialNum && !(*tocH)->previewID && (sumNum=FindSumBySerialNum(tocH,oldEzOpenSerialNum))>=0 && !(modifiers&shiftKey))
		Preview(tocH,sumNum);
	CheckBox(GetWindowMyWindowPtr(FrontWindow_()),False);
#ifdef PERF
	PerfControl(ThePGlobals,False);
#endif
}


/************************************************************************
 * DoMessageMenu - do something to a messages
 ************************************************************************/
Boolean DoMessageMenu(short item,TOCHandle tocH,short sumNum,short toWhom,TextAddrHandle addr,long modifiers,Boolean nuke,Boolean *busy)
{
	MyWindowPtr win;
	Boolean	doVirtualMB = true;
	Str255	s;
			
	switch(item)
	{
		case MESSAGE_DELETE_ITEM:
			DeleteMessageLo(tocH,sumNum,nuke);
			break;
		case STATE_HIER_MENU:
#ifdef THREADING_ON
			if ((*tocH)->sums[sumNum].state==BUSY_SENDING)
				*busy = true;
			else
#endif
			{
				SetState(tocH,sumNum,toWhom);
				doVirtualMB = false;	//	Already took care of this
			}
			break;
		case PERS_HIER_MENU:
			//No personalities menu in Light
			if (HasFeature (featureMultiplePersonalities))
				SetPers(tocH,sumNum,FindPersByName(MyGetItem(GetMHandle(item),toWhom,s)),True);
			break;
		case LABEL_HIER_MENU:
			SetSumColor(tocH,sumNum,Menu2Label(toWhom));
			doVirtualMB = false;	//	Already took care of this
			break;
		case SERVER_HIER_MENU:
#ifdef	IMAP
			ServerMenuChoice(tocH,sumNum,toWhom, (modifiers&shiftKey)!=0);
#else
			ServerMenuChoice(tocH,sumNum,toWhom);
#endif
			break;
		case TABLE_HIER_MENU:
			SetMessTable(tocH,sumNum,toWhom);
			break;
		case FILE_MENU:
			ExportHTMLSum(tocH,sumNum);
			break;
		case PRIOR_HIER_MENU:
			SetPriority(tocH,sumNum,NewPrior(toWhom,(*tocH)->sums[sumNum].priority));
			doVirtualMB = false;	//	Already took care of this
			break;
		default:
#ifdef	IMAP
			// must have the message before we can do anything here.
			// right now, fetch the entire message, including attachments, unless we're just replying.
			if (EnsureMsgDownloaded(tocH,sumNum,item!=MESSAGE_REPLY_ITEM))
			{
#endif
				if (win = GetAMessage(tocH,sumNum,nil,nil,False))
				{
					WindowPtr	winWP = GetMyWindowWindowPtr(win);
					switch(item)
					{
						case MESSAGE_SALVAGE_ITEM:
							if (!DoSalvageMessage(win,False)) CommandPeriod=true;
							break;
						case MESSAGE_FORWARD_ITEM:
							if (!DoForwardMessage(win,addr,True)) CommandPeriod=true;
							break;
						case MESSAGE_REPLY_ITEM:
							{
								Boolean all, quote, self;
								ReplyDefaults(modifiers,&all,&self,&quote);
								if (!DoReplyMessage(win,all,self,quote,True,toWhom,True,True,True)) CommandPeriod=true;
							}
							break;
						case MESSAGE_REDISTRIBUTE_ITEM:
							if (!DoRedistributeMessage(win,addr,PrefIsSetOrNot(PREF_TURBO_REDIRECT,modifiers,optionKey),False,True)) CommandPeriod=true;
							break;
					}
					if (!IsWindowVisible(winWP))
						CloseMyWindow(winWP);
					else
						NotUsingWindow(winWP);
				}
				else CommandPeriod=true;
#ifdef	IMAP
			}
			else CommandPeriod=true;
#endif
			doVirtualMB = false;
			break;

	}
	return doVirtualMB;
}

/************************************************************************
 * ReplyDefaults - get the defaults for reply options
 ************************************************************************/
void ReplyDefaults(short modifiers,Boolean *all, Boolean *self, Boolean *quote)
{
 	*all = PrefIsSetOrNot(PREF_REPLY_ALL,modifiers,optionKey);
	*self = !PrefIsSet(PREF_NOT_ME);
	*quote = (modifiers&shiftKey) == 0;
}

/************************************************************************
 * BoxNextSelected - find first selection in a mailbox
 ************************************************************************/
short BoxNextSelected(TOCHandle tocH,short afterNum)
{
	int sNum, count;
	
	count = (*tocH)->count;
	
	for (sNum=afterNum+1;sNum<count;sNum++)
		if ((*tocH)->sums[sNum].selected) return(sNum);
	
	return(-1);
}

/**********************************************************************
 * SaveTextAsMessage - save a text block as a message
 **********************************************************************/
OSErr SaveTextAsMessage(Handle preText,Handle text,TOCHandle tocH,long *fromLen)
{
	long size = GetHandleSize(text);
	OSErr err;
	
	if (size && (*text)[size-1]!='\015')
	{
		PtrPlusHand("\015",text,1);
		size++;
	}
	err = SavePtrAsMessage(preText ? LDRef(preText):nil,preText?GetHandleSize(preText):nil,LDRef(text),size,tocH,fromLen);
	UL(text);
	if (preText) UL(preText);
	
	return(err);
}

/**********************************************************************
 * SavePtrAsMessage - save a text block as a message
 **********************************************************************/
OSErr SavePtrAsMessage(UPtr preText,long preSize,UPtr text, long size, TOCHandle tocH,long *fromLen)
{
	OSErr err;
	long eof;
	Str31 name;
	LineIOD lid;
	MSumType sum;
	FSSpec	spec;
	
	GetMailboxName(tocH,-1,name);
	
	/*
	 * open mailbox and write the bytes
	 */
	if (!(err=BoxFOpen(tocH)))
	{
		eof = FindTOCSpot(tocH,size);
		err = SetFPos((*tocH)->refN,fsFromStart,eof);
		if (!err) err = PutOutFromLine((*tocH)->refN,fromLen);
		if (!err && preText) err = AWrite((*tocH)->refN,&preSize,preText);
		if (!err) err = AWrite((*tocH)->refN,&size,text);
		if (!err) err = TruncAtMark((*tocH)->refN);
		TOCSetDirty(tocH,true);
		BoxFClose(tocH,true);
	}
	
	/*
	 * did it work?
	 */
	if (err)
	{
		FileSystemError(WRITE_MBOX,name,err);
		return(err);
	}
	
	/*
	 * read it back
	 */
	spec = GetMailboxSpec(tocH,-1);
	if (err=OpenLine(spec.vRefNum,spec.parID,name,fsRdWrPerm,&lid))
		return(FileSystemError(READ_MBOX,name,err));
	if (err=SeekLine(eof,&lid)) return(FileSystemError(READ_MBOX,name,err));
	ReadSum(nil,False,&lid,True);
	Zero(sum);
	if (!ReadSum(&sum,False,&lid,False))
		err = !SaveMessageSum(&sum,&tocH);
	else err = 1;
	CloseLine(&lid);
	return(err);
}

#pragma segment MsgOps
/************************************************************************
 * DoSalvageMessage - glean what you can from a bounced message's headers
 ************************************************************************/
MyWindowPtr DoSalvageMessage(MyWindowPtr win,Boolean forXfer)
{
	return DoSalvageMessageLo(win, forXfer, false);
}

/************************************************************************
 * DoSalvageMessageLo - glean what you can from a bounced message's headers
 ************************************************************************/
MyWindowPtr DoSalvageMessageLo(MyWindowPtr win,Boolean forXfer,Boolean forIMAP)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
	MessHandle newMessH;
	MyWindowPtr newWin;
	WindowPtr	newWinWP;
	Str255 scratch;
	short field;
	HeadSpec oldHS, newHS;
	OSErr err=noErr;
	UHandle text;
	long newBo;
	Boolean html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);

	NicknameWatcherFocusChange (win->pte); /* MJN */

	if (GetWindowKind(winWP)!=COMP_WIN && !SaveMessHi(win,False)) return(nil);

#ifdef	IMAP
	//	Make sure message has been downloaded if IMAP
	if (!EnsureMsgDownloaded((*origMessH)->tocH,(*origMessH)->sumNum,true))
		return nil;
#endif
	
	PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
	if (newWin=DoComposeNew(0))
	{
		long sigLen;
		
		newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
		XferCustomTable(origMessH,newMessH);
		if (GetWindowKind(winWP)==COMP_WIN || MessFlagIsSet(origMessH,FLAG_OUT))
		{
			//if (win->qWindow.windowKind!=COMP_WIN) AlignHeaders(origMessH);
			for (field=0;!err && field<=ATTACH_HEAD;field++)
			{
				if (field!=FROM_HEAD ||
						!CompHeadGetStr(origMessH,field,scratch) && (MessOptIsSet(origMessH,OPT_REDIRECTED) || IsMe(scratch)))
				{
					if (CompHeadFind(origMessH,field,&oldHS))
					{
						// If we have the body, the oldHS will not include the sig
						// This is bad for copying
						// However, we do need to remember this fact, so we can
						// make sure we lock things correctly later
						if (field==0)
						{
							sigLen = PeteLen((*origMessH)->bodyPTE) - oldHS.stop;
							oldHS.stop += sigLen;
						}
						
						if (oldHS.value<oldHS.stop)
						{
							if (CompHeadFind(newMessH,field,&newHS))
							{
								if (field==FROM_HEAD)
									err = CompHeadSetStr((*newMessH)->bodyPTE,&newHS,scratch);
								else
									err = PeteCopy((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
												oldHS.value,oldHS.stop,newHS.stop,nil,field);
							}
							if (!err && CompHeadFind(newMessH,field,&newHS))
							{
								PeteLock((*newMessH)->bodyPTE,newHS.value,newHS.stop,0);
								if (field==0 && sigLen)
									PeteLock((*newMessH)->bodyPTE,newHS.stop-sigLen,newHS.stop-sigLen+*GetRString(scratch,SIG_INTRO)+1,peModLock|peSelectLock|peClickBeforeLock);
							}
						}
					}		
				}
			}
			TOCSetDirty((*newMessH)->tocH,true);
			SetSumColor((*newMessH)->tocH,(*newMessH)->sumNum,GetSumColor((*origMessH)->tocH,(*origMessH)->sumNum));
			SumOf(newMessH)->flags = SumOf(origMessH)->flags;
			SumOf(newMessH)->opts = SumOf(origMessH)->opts;
			SumOf(newMessH)->sigId = SigValidate(SumOf(origMessH)->sigId);
			SumOf(newMessH)->origPriority = SumOf(newMessH)->priority = SumOf(origMessH)->priority;
			if (GetWindowKind(winWP)!=COMP_WIN && MessOptIsSet(newMessH,OPT_INLINE_SIG))
				AddInlineSig(newMessH);
						
			if (GetWindowKind(winWP)==COMP_WIN && (*origMessH)->hTranslators)
			{
				text = DupHandle((Handle)(*origMessH)->hTranslators);
				(*newMessH)->hTranslators = (void*)text;
			}
			else if (!GetRHeaderAnywhere(origMessH,HEADER_STRN+TRANSLATOR_HEAD,&text))
			{
				AddTranslatorsFromPtr(newMessH,LDRef(text),GetHandleSize(text));
				ZapHandle(text);
			}
		}
		else
		{
			UPtr spot, oldSpot, beginning, end;
			long size, total;
			Boolean toFound;
			Str63 received;
			long offset;
			
			if (SumOf(origMessH)->flags & FLAG_OUT)
			{
				SumOf(newMessH)->flags = SumOf(origMessH)->flags;
				SumOf(newMessH)->sigId = SumOf(origMessH)->sigId;
			}
			
			PeteGetTextAndSelection((*origMessH)->bodyPTE,&text,nil,nil);
			beginning = spot = LDRef(text);
			total = size = GetHandleSize_(text);
			end = spot+size;
			
			if (forXfer || SumOf(origMessH)->flags & FLAG_OUT) oldSpot = nil;
			else
			{
				/*
				 * find the last "Received:" header
				 */
				GetRString(received,RECEIVED_HEAD); TrimWhite(received);
				oldSpot = nil;
				while (spot=FindHeaderString(spot,received,&size,True))
				{
					oldSpot=spot;
					spot += size;
					size = end - spot;
				}
			}
			
			/*
			 * copy the relevant parts
			 */
			if (!oldSpot) oldSpot=beginning;
			{
				if (oldSpot!=beginning)
					SumOf(newMessH)->sigId = SIG_NONE;
				size = total - (oldSpot-beginning);
				TextFindAndCopyHeader(oldSpot,size,newMessH,HeaderName(SUBJ_HEAD),SUBJ_HEAD,0);
				TextFindAndCopyHeader(oldSpot,size,newMessH,HeaderName(CC_HEAD),CC_HEAD,0);
				TextFindAndCopyHeader(oldSpot,size,newMessH,HeaderName(ATTACH_HEAD),ATTACH_HEAD,0);
				toFound = TextFindAndCopyHeader(oldSpot,size,newMessH,HeaderName(TO_HEAD),TO_HEAD,0);
				TextFindAndCopyHeader(oldSpot,size,newMessH,HeaderName(BCC_HEAD),BCC_HEAD,0);
				/*
				 * find the body
				 */
				for (spot=oldSpot;spot<oldSpot+size-1;spot++)
					if (spot[0]=='\015' && spot[1]=='\015') break;
				spot += 2;
				offset = spot-beginning;
				UL(text);
				if (spot<oldSpot+size-1)
				{
					newBo = PETEGetTextLen(PETE,(*newMessH)->bodyPTE);
					err = PeteCopyNoLabel((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
									 offset,total,newBo,nil,False,html?0:0xffffffff);
					//err = SetMessText(newMessH,0,spot,total-(spot-beginning));
					if (!err) PeteLock((*newMessH)->bodyPTE,newBo,0x7fffffff,0);
				}
				if (MessFlagIsSet(origMessH,FLAG_HAS_ATT))
				{
					CopyAttachments(newMessH);
					SpoolAttachments(newMessH);
				}
			}
			UL(text);
		}
		newWinWP = GetMyWindowWindowPtr(newWin);
		if (err && newWin)
		{
			NoSaves = True; CloseMyWindow(newWinWP); NoSaves = False;
			WarnUser(PETE_ERR,err);
			PopPers();
			return(nil);
		}
		if (!forXfer || forIMAP)
		{
			WeedXAttachments(newMessH, !forIMAP);
			if (UseInlineSig && !MessOptIsSet(newMessH,OPT_INLINE_SIG)) AddInlineSig(newMessH);
			UpdateSum(newMessH,SumOf(newMessH)->offset,SumOf(newMessH)->length);
			if(!forIMAP)
			{
				ShowMyWindow(newWinWP);
				newWin->isDirty = False;
				PeteCleanList(newWin->pte);
			}
		}
	}
	PopPers();
	return(newWin);
}

/************************************************************************
 * UniqueHeader - make sure the addresses in a header are unique
 ************************************************************************/
OSErr UniqueHeader(MessHandle messH,short head,Boolean wantErrors)
{
	Handle addresses = nil;
	short oldSize;
	HeadSpec hs;
	OSErr err = noErr;
	
	if (CompHeadFind(messH,head,&hs) && !CompHeadGetText(TheBody,&hs,&addresses))
	{
	 	oldSize = GetHandleSize_(addresses);
		err = NickUniq(addresses,"\p, ",wantErrors);
		if (oldSize != GetHandleSize_(addresses))
			CompHeadSet(TheBody,&hs,addresses);
		ZapHandle(addresses);
	}
	return err;
}

/************************************************************************
 * RemoveSelf - Remove "me" from a list of addresses
 *	Ray Davison, SFU
 ************************************************************************/
OSErr RemoveSelf(MessHandle messH,short head,Boolean wantErrors)
{
	Str63 dummy;
	Str255 temp;
	EAL_VARS_DECL;
	UHandle rawMyself=nil, cookedMyself=nil;
	UHandle rawAddress=nil, spewHandle=nil;
	UHandle myself=NuHTempBetter(0);
	long offset, meOffset;
	Boolean removed = False;
	Handle text=nil;
	Handle oldText=nil;
	HeadSpec hs;
	Boolean group;
	Boolean groupWas = False;
	OSErr err = noErr;
	
	/* Get a definition of who I am */
	
	GetRString(temp, ME);
	PtrPlusHand_(temp+1, myself, temp[0]);
	PtrPlusHand_(",", myself, 1);
	GetPOPInfo(temp, dummy);
	PtrPlusHand_(temp+1, myself, temp[0]);
	PtrPlusHand_(",", myself, 1);
	GetReturnAddr(temp ,True);
	PtrPlusHand_(temp+1, myself, temp[0]);
	if (!(err=SuckAddresses(&rawMyself,myself, False, wantErrors, False,nil)) &&
			!(err=ExpandAliasesLow(&cookedMyself,rawMyself, 0, False, "",EAL_VARS)))	// no autoqual
	{
		ZapHandle(rawMyself);
		ZapHandle(myself);
		
		/* expand the text */
		
		if (CompHeadFind(messH,head,&hs) && !CompHeadGetText(TheBody,&hs,&oldText))
		{
			if (!(err=SuckAddresses(&rawAddress,oldText, True, wantErrors, False,nil)) && (text=NuHTempBetter(0L)))
			{
				/* Remove myself from address */
				if (rawAddress && cookedMyself)
				{
					LDRef(cookedMyself);
					for (offset=0; (*rawAddress)[offset]; offset += (*rawAddress)[offset]+2)
					{
						/* clean up the address */
						LDRef(rawAddress);
						SuckPtrAddresses(&spewHandle,(*rawAddress)+offset+1,
																					(*rawAddress)[offset], False, wantErrors, true,nil);
						UL(rawAddress);
						if (spewHandle)
						{
							LDRef(spewHandle);
							group = (*spewHandle)[**spewHandle]==':';
							/* look for this in the "me" addresses */
							for (meOffset=0; (*cookedMyself)[meOffset]; meOffset += (*cookedMyself)[meOffset]+2)
							{
							if (StringSame(*spewHandle,*cookedMyself+meOffset))
								break;
							}
							ZapHandle(spewHandle);
						}
		
						/* if we didn't find it, then add this address to the result */
						if (!(*cookedMyself)[meOffset])
						{
							
							LDRef(rawAddress);
							if (!groupWas && GetHandleSize_(text)) PtrPlusHand_(", ", text, 2);
							PtrPlusHand_((*rawAddress)+offset+1,text,(*rawAddress)[offset]);
							HUnlock(rawAddress);
						}
						else removed = True;
						groupWas = group;
					}
				}
			}
		}	
	}		
	
	ZapHandle(oldText);
	ZapHandle(rawMyself);
	ZapHandle(cookedMyself);
	ZapHandle(rawAddress);
	ZapHandle(spewHandle);
	ZapHandle(myself);
	
	if (removed) {
		CompHeadSet(TheBody,&hs,text);
		CompGatherRecipientAddresses (messH, true);
	}

	ZapHandle(text);
	return err;
}

/**********************************************************************
 * MessText - get the text of a message
 **********************************************************************/
UHandle MessText(MessHandle messH)
{
	UHandle text=nil;
	PeteGetRawText(TheBody,&text);
	return(text);
}

/**********************************************************************
 * MessVisibleText - get only the visible text of a message
 **********************************************************************/
UHandle MessVisibleText(MessHandle messH)
{
	long runLen, offset;
	UHandle text=nil;
	PETEStyleEntry pse;
	long len;
	OSErr err = noErr;
	Accumulator a;
	
	err = PeteGetRawText(TheBody,&text);
	if (err) return nil;
	
	len = GetHandleSize(text);
	
	Zero(pse);
	Zero(a);
	
	for (offset=0;offset<len;offset += runLen)
	{
		err = PeteGetStyle(TheBody, offset, &runLen, &pse);
		if (err) break;
		
		if (runLen==0) 
		{
			offset++;	// keep moving!
			continue;
		}
		
		// add all text except invisible text (graphic with no handle)
		if (!(pse.psGraphic && !pse.psStyle.graphicStyle.graphicInfo))
		{
			err = AccuAddFromHandle(&a,text,offset,runLen);
			if (err) break;
		}
	}
	
	if (err) AccuZap(a);
	else AccuTrim(&a);
	
	return a.data;
}

/************************************************************************
 * ReopenMessage - reopen the current message
 ************************************************************************/
MyWindowPtr ReopenMessage(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	UHandle text;
	MessHandle messH = Win2MessH(win);
	OSErr err = noErr;
	
	text = GetMessText(Win2MessH(win));
	
	if (text)
	{		
		// stick in the text
		{
			PeteSetTextPtr(TheBody,nil,0);
			PeteKillUndo(TheBody);
			PeteCalcOff(TheBody);
			
			(*PeteExtra(TheBody))->emoDesired = !MessFlagIsSet(messH,FLAG_SHOW_ALL);
			
			PetePlain(TheBody,kPETECurrentStyle,kPETECurrentStyle,peAllValid);
			PetePlainPara(TheBody,0);

			if (!MessFlagIsSet(messH,FLAG_SHOW_ALL) &&
			    (MessFlagIsSet(messH,FLAG_RICH)
#ifndef OLDHTML
			    	|| MessOptIsSet(messH,OPT_HTML)
#endif
			    	|| MessOptIsSet(messH,OPT_FLOW)
			    	|| MessOptIsSet(messH,OPT_CHARSET)
			))
			{
				err = InsertRichLo(text,0,-1,-1,True,True,TheBody,(*PeteExtra(win->pte))->partStack,messH,MessOptIsSet(messH,OPT_DELSP));
				ZapHandle(text);
			}
			else
				err = PETEInsertPara(PETE,TheBody,kPETELastPara,nil,text,nil);

			if (!err)
			{
				PeteSmallParas(TheBody);
				text = nil;
				// align headers if need be
				//if (!PrefIsSet(PREF_DONT_ALIGN_HEADERS)) AlignHeaders(messH);
				
#ifdef OLDHTML
				// interpret rich text
				if (!MessFlagIsSet(messH,FLAG_SHOW_ALL))
				{
					if (MessOptIsSet(messH,OPT_HTML))
						err = PeteHTML(TheBody,SumOf(messH)->bodyOffset-(*messH)->weeded,0,True);
				}
#endif

				if (!err) HiliteOddReply(messH);
				
				if (!err) PeteTrimTrailingReturns(TheBody,true);

				if (!err)
				{
					// recalculate
					PeteCalcOn(TheBody);
					
					//	add notification control if necessary
					CheckAddNotifyControls(win, messH);

					if (PrefIsSet(PREF_ZOOM_OPEN)) ReZoomMyWindow(winWP);
					
					// scroll to correct position
					if (!MessFlagIsSet(messH,FLAG_SHOW_ALL)) ShowMessageSeparator(TheBody,true);
					InvalContent(win);
				}
			}
		}
				
		// mark document as clean
		PETEMarkDocDirty(PETE,TheBody,False);
		win->isDirty = False;
		PeteSetURLRescan(TheBody,0);
		
		// kill text if still here
		ZapHandle(text);
		
		if (err)
		{
			WarnUser(DOC_DAMAGED_ERR,err);
			CloseMyWindow(winWP);
			win=nil;
		}
	}
	else
	{
		CloseMyWindow(winWP);
		win = nil;
	}
	if (win) PeteCalcOn(TheBody);
	return(win);
}

/************************************************************************
 * FindFrom - find a (nicely formatted) From address
 ************************************************************************/
void FindFrom(UPtr who, PETEHandle pte)
{
	UPtr found;
	Str31 header;
  long len;
  Handle text;
	
	PeteGetRawText(pte,&text);
 	len = GetHandleSize(text);
	GetRString(header,FROM_HEAD+HEADER_STRN);
	if (found = FindHeaderString(LDRef(text),header,&len,False))
	{
		*who = MIN(62,len);
		BMD(found,who+1,*who);
		who[*who+1] = 0;
		BeautifyFrom(who);
	}
	else *who = 0;
	UL(text);
}

/************************************************************************
 * QuoteLines - put a quote prefix before the specified TextEdit lines
 ************************************************************************/
void QuoteLines(PETEHandle pte,long from,long to,short pfid, long *qEnd)
{
	long this;
	Str15 prefix;
	UHandle text;
	long count = 0;
	Boolean first = True;
	PETEStyleEntry pse;
	RGBColor color;
	Boolean withSpace = false;
	long numSpaces = 0;
	Byte quoteChar;

	Zero(pse);
		
	if (qEnd) *qEnd = to;
	
	GetRString(prefix,pfid);
	if (!*prefix) return;
	
	// if trailing space, remove and note
	if (*prefix==2 && prefix[2]==' ')
	{
		withSpace = true;
		--*prefix;
		quoteChar = prefix[1];
	}
	
	PETEGetRawText(PETE,pte,&text);
	this = GetHandleSize(text);
	to = MIN(to,this);

	for (this=to-2;this>=from;this--)
		if (!this || (*text)[this]=='\015')
		{
			if (withSpace && (*text)[this+1]!=quoteChar)
			{
				PeteInsertChar(pte,this?this+1:this,' ',nil);
				numSpaces++;
			}
			PeteInsertPtr(pte,this ? this+1:this,prefix+1,*prefix);
			count++;
		}
	if (qEnd) *qEnd = to + count * *prefix + numSpaces;	// adjust for inserted prefixes

	// do the scanner's work for it
	if (!Black(GetRColor(&color,QUOTE_COLOR)))
	{
		pse.psStyle.textStyle.tsLabel = pQuoteLabel;
		PETESetTextStyle(PETE,pte,from,to + count * *prefix,&pse.psStyle.textStyle,peLabelValid);
	}
}

/************************************************************************
 * PrependMessText - stick some text before one of the fields of a message.
 ************************************************************************/
OSErr PrependMessText(MessHandle messH,short whichTXE,UPtr string,long size)
{
	HeadSpec hs;
	
	if (CompHeadFind(messH,whichTXE,&hs))
		return(CompHeadPrependPtr(TheBody,&hs,string,size));
	else return(fnfErr);
}

/************************************************************************
 * FindHeaderString - pick the given header out of a message, by name
 *  Pointer returned is to 
 ************************************************************************/
UPtr FindHeaderString(UPtr text,UPtr headerName,long *size,Boolean bodyToo)
{
	UPtr spot,end, colon;
	char header[MAX_HEADER];
	short mLen = MIN(MAX_HEADER-2,*headerName);
#ifdef TWO
	Boolean any = EqualStrRes(headerName,FILTER_ANY);
	Boolean addressee = EqualStrRes(headerName,FILTER_ADDRESSEE);
	static UHandle addrRes;
#endif
	
	if (addressee) mLen = MAX_HEADER-2;
	
#ifdef TWO
	if (!addrRes || !*addrRes)
	{
		if (addrRes) LoadResource(addrRes);
		else addrRes = GetResource_('STR#',AddrHeadsStrn);
		if (!addrRes || !*addrRes) return(nil);
		HNoPurge(addrRes);
	}
#endif
	
	for (end = text+*size; text<end; text = spot+1)
	{
		for (spot=text;spot<end;spot++) if (*spot == '\015' && (spot==end-1||!IsWhite(spot[1]))) break;
		if (spot==text && !bodyToo) break;
		*header = MIN(mLen,end-text);
		BMD(text,header+1,*header);
#ifdef TWO
		if (addressee)
		{
			if (colon=PIndex(header,':'))
				*header = colon-header;
		}
#endif
		if (
#ifdef TWO
				any ||
				addressee && FindSTRNIndexRes(addrRes,header) ||
#endif
				EqualString(header,headerName,False,True))
		{
#ifdef TWO
			if (!any && !addressee)
#endif
				text += *header;
			while (IsWhite(*text)) text++;
			for (spot--; IsWhite(*spot); spot--);
			*size = MAX(0,spot-text+1);
			return(text);
		}
	}
	return(nil);
}
/************************************************************************
 * DoReplyMessage - craft a reply to a message
 ************************************************************************/
MyWindowPtr DoReplyMessage(MyWindowPtr win, Boolean all, Boolean self, Boolean quote, Boolean doFcc, short withWhich,Boolean vis,Boolean station,Boolean caching)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
	MessHandle newMessH;
	Str255 subj,scratch,replyTo;
	long bodyOffset = -1;
	MyWindowPtr newWin=nil;
	WindowPtr		newWinWP;
	short r;
	long len;
	Handle text;
	long origLen;
	long selStart, selEnd;
	Str255 soundName;
	Boolean doSound;
	HeadSpec hs;
	OSErr err = noErr;
	PETETextStyle style;
	long newBo;
	//PeteSaneMargin marg;
	Boolean rich;
	Boolean html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
	PETEHandle copyFromPTE;
	TOCHandle origTOCH;
	Boolean listReply = MessOptIsSet(origMessH,OPT_BULK) && PrefIsSet(PREF_LIST_REPLYTO);
	Boolean cacheReplyTo = !PrefIsSet (PREF_NICK_CACHE) && !PrefIsSet (PREF_NICK_CACHE_NOT_ADD_REPLY_TO);
	Boolean wantErrors = true;
	
	// Dunno why this was done
	// if (GetWindowKind(winWP)!=COMP_WIN && !SaveMessHi(win,False)) return(nil);
	
#ifdef	IMAP
	//	Make sure message has been downloaded if IMAP.  Don't care about attachments.
	if (!EnsureMsgDownloaded((*origMessH)->tocH,(*origMessH)->sumNum,false))
		return nil;
#endif

	PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
	if (newWin=DoComposeNew(0))
	{
		newWinWP = GetMyWindowWindowPtr(newWin);
		newMessH = (MessHandle) GetMyWindowPrivateData(newWin);
		replyTo[0] = 0;
		rich = UseFlowOutExcerpt || (MessIsRich(origMessH) || MessIsRich(newMessH));

		XferCustomTable(origMessH,newMessH);
		
		PeteGetTextAndSelection((*origMessH)->bodyPTE,&text,&selStart,&selEnd);
		origLen = GetHandleSize(text);
		
		/* handle the subject */
		FindAndCopyHeader(origMessH,newMessH,HeaderName(SUBJ_HEAD),SUBJ_HEAD);
		GetRString(scratch,REPLY_INTRO);
		TrimWhite(scratch);
		CompHeadGetStr(newMessH,SUBJ_HEAD,subj);
		if (!ReMatch(subj,scratch))
		{
			GetRString(scratch,REPLY_INTRO);
			PrependMessText(newMessH,SUBJ_HEAD,scratch+1,*scratch);
		}
		
		/* reply to sender */
		replyTo[0] = 0;
		if (listReply && !all) {
			FindAndCopyHeader(origMessH,newMessH,HeaderName(FROM_HEAD),TO_HEAD);
			if (HasFeature (featureNicknameWatching) && cacheReplyTo)
				SuckHeaderText (origMessH, replyTo, sizeof (replyTo), FROM_HEAD);
		}
		else
			for (r=1;*GetRString(scratch,ReplyStrn+r);r++)
			{
				TrimWhite(scratch);
				if (FindAndCopyHeader(origMessH,newMessH,scratch,TO_HEAD))
				{
					doSound = MessOptIsSet(origMessH,OPT_WEIRD_REPLY);
					if (!doSound) listReply = false;	// as you were
					// (jp) added to grab the reply to address
					if (!PrefIsSet (PREF_NO_SPOKEN_WARNINGS) || cacheReplyTo)
						SuckHeaderText (newMessH, replyTo, sizeof (replyTo), TO_HEAD);
					break;
				}
			}
		if (caching && HasFeature (featureNicknameWatching) && cacheReplyTo && replyTo[0])
			CacheRecentNickname (replyTo);
		
		/* bring over the other recipients and cc's, if desired */
		if (all)
		{
			FindAndCopyHeader(origMessH,newMessH,HeaderName(TO_HEAD),PrefIsSet(PREF_CC_REPLY)?CC_HEAD:TO_HEAD);
			FindAndCopyHeader(origMessH,newMessH,HeaderName(CC_HEAD),CC_HEAD);
			
			// If we've found a reply-to header and we have a mailing list, also copy From to the cc: field
			if (doSound && listReply)
				FindAndCopyHeader(origMessH,newMessH,HeaderName(FROM_HEAD),CC_HEAD);
			
			/* remove self, if desired */
			if (!self)
			{
				if (RemoveSelf(newMessH,TO_HEAD,wantErrors)) wantErrors=false;
				if (RemoveSelf(newMessH,CC_HEAD,wantErrors)) wantErrors=false;
			}
			
			if (UniqueHeader(newMessH,TO_HEAD,wantErrors)) wantErrors=false;
			UniqueHeader(newMessH,CC_HEAD,wantErrors);
		}
		
		/*
		 * for Larry's benefit
		 */
		if (PrefIsSet(PREF_NEWSGROUP_HANDLING)) CopyNewsgroups(origMessH,newMessH);
		
		/*
		 * auto-fcc
		 */
		//Folder Carbon Copy - don't allow the auto FCC in Light  
#ifdef	IMAP
		if (HasFeature (featureFcc) && doFcc && PrefIsSet(PREF_AUTO_FCC) && !(*(*origMessH)->tocH)->which && !IMAPDontAutoFccMailbox((*origMessH)->tocH))
#else
		if (HasFeature (featureFcc) && doFcc && PrefIsSet(PREF_AUTO_FCC) && !(*(*origMessH)->tocH)->which)
#endif
		{
			FSSpec spec = GetMailboxSpec((*origMessH)->tocH,-1);
			Fcc(newMessH,&spec);
		}
		
		/*
		 * in-reply-to and references
		 */
		ReplyReferences(origMessH,newMessH);
		
		/*
		 * copy the body.  Use the preview pane if that's what we have
		 */
		copyFromPTE = (*origMessH)->bodyPTE;
		origTOCH = (*origMessH)->openedFromTocH ? (*origMessH)->openedFromTocH : (*origMessH)->tocH;
		if ((*origTOCH)->win==GetWindowMyWindowPtr(FrontWindow_()) && (*origTOCH)->previewID && 
				(*origTOCH)->previewID==(*origMessH)->openedFromSerialNum &&
				!quote && *PeteSelectedString(scratch,(*origTOCH)->previewPTE))
		{
			copyFromPTE = (*origTOCH)->previewPTE;
			PeteGetTextAndSelection(copyFromPTE,&text,&selStart,&selEnd);
		}
		
		if (quote)
		{
			bodyOffset = 0;
			while (bodyOffset<origLen-1)
			{
				if ((*text)[bodyOffset+1]!='\015') bodyOffset += 2;
				else if ((*text)[bodyOffset]=='\015') break;
				else bodyOffset++;
			}
				
			while (bodyOffset<origLen && (*text)[bodyOffset] == '\015') bodyOffset++;
			len = origLen-bodyOffset;
		}
		else
		{				
			if (selEnd != selStart)
			{
				bodyOffset = selStart;
				len = selEnd - bodyOffset;
			}
		}
		
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif

		if (len > 0 && bodyOffset >= 0)
		{
			while (len && (*text)[bodyOffset+len-1]=='\015') len--;
			newBo = PETEGetTextLen(PETE,(*newMessH)->bodyPTE);
			if (len>0) err = PeteCopyNoLabel(copyFromPTE,(*newMessH)->bodyPTE,bodyOffset,bodyOffset+len,newBo,nil,!rich,html?0:0xffffffff);
			HUnlock(text);
			style.tsLock = 0;
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif
			PETESetTextStyle(PETE,(*newMessH)->bodyPTE,newBo,0x7fffffff,&style,peLockValid);	
			if (rich) PeteExcerpt((*newMessH)->bodyPTE,newBo,PETEGetTextLen(PETE,(*newMessH)->bodyPTE)-1);
			
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif
			if (err)
			{
				NoSaves = True; CloseMyWindow(newWinWP); NoSaves = False;
				PopPers();
				WarnUser(PETE_ERR,err);
				return(nil);
			}

			/*
			 * make sure the last line is blank
			 */
			EnsureMessNewline(newMessH);
			
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif
			if (rich) PetePlainPara((*newMessH)->bodyPTE,PeteParaAt((*newMessH)->bodyPTE,PETEGetTextLen(PETE,(*newMessH)->bodyPTE)));
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif

			/*
			 * quote them
			 */
			if (!rich)
			{
				CompHeadFind(newMessH,0,&hs);
				QuoteLines((*newMessH)->bodyPTE,hs.start,hs.stop-1,QUOTE_PREFIX,nil);
			}
			else if (PrefIsSet(PREF_SCHEERDER))
			{
				CompHeadFind(newMessH,0,&hs);
				PeteSelect(nil,(*newMessH)->bodyPTE,hs.value,PeteLen((*newMessH)->bodyPTE));
				PeteWrap((*newMessH)->win,(*newMessH)->bodyPTE,false);
			}
#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif
			
			/*
			 * annotate
			 */
			if (rich) Attribute(QUOTH,origMessH,newMessH,False);
			Attribute(all ? ATTRIBUTION:REP_SEND_ATTR,origMessH,newMessH,False);
			if (rich) Attribute(UNQUOTH,origMessH,newMessH,True);
		}

#ifdef DEBUG
	if (BUG14) {InvalContent(win);UpdateMyWindow(winWP);}
#endif
		/*
		 * copy priority (not my idea)
		 */
		if (!PrefIsSet(PREF_NO_XF_PRIOR))
			SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
				SumOf(origMessH)->origPriority;
		
		SumOf(newMessH)->outType = OUT_REPLY;	//	for statistics

		/*
		 * encryption
		 */
		if (MessOptIsSet(origMessH,OPT_WIPE)) SetMessFlag(newMessH,FLAG_ENCRYPT);
		
		if (station)
		{
			if (!withWhich) ApplyDefaultStationery(newWin,True,True);
			//Stationery - no support for this in Light
			else if (HasFeature (featureStationery)) ApplyIndexStationery(newWin,withWhich,True,True);
		}
		
		(*PeteExtra((*newMessH)->bodyPTE))->quoteScanned = -1;	// don't need to run the scanner, quotelines did it already
		
		if (vis)
		{
			// shall we select the quote?
			quote = quote || selEnd-selStart>=GetRLong(OPEN_AT_END_THRESH);
			(*newMessH)->openToEnd = !quote;
			
			AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->uidHash);
			AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->state);
			AccuAddLong(&(*newMessH)->aSourceMID,REPLIED);
			SetState((*origMessH)->tocH,(*origMessH)->sumNum,REPLIED);
			UpdateSum(newMessH,SumOf(newMessH)->offset,SumOf(newMessH)->length);
			ShowMyWindow(newWinWP);
			UpdateMyWindow(newWinWP);
			if (doSound)
				if (PrefIsSet (PREF_NO_SPOKEN_WARNINGS))
					PlayNamedSound(GetRString(soundName,REPLY_SOUND));
				else
					(void) ComposeStdAlert (kAlertNoteAlert, PASSIVE_REPLY_TO_ASTR, replyTo);
		}
		newWin->isDirty = False;
		PeteCleanList(newWin->pte);
	}
	PopPers();
	return(newWin);
}

/**********************************************************************
 * ReplyReferences
 **********************************************************************/
OSErr ReplyReferences(MessHandle origMessH,MessHandle newMessH)
{
	OSErr err = noErr;
	UHandle /*t1=nil, */t2=nil, t3=nil;
	Accumulator extras = (*newMessH)->extras;
	long origLen = extras.offset;
	
	/* Get the bodies of all three headers */
//	GetRHeaderAnywhere(origMessH,HeaderStrn+IN_REPLY_TO_HEAD,&t1);
	GetRHeaderAnywhere(origMessH,HeaderStrn+REFERENCES_HEAD,&t2);
	GetRHeaderAnywhere(origMessH,HeaderStrn+MSGID_HEAD,&t3);
	
	/* Strip returns - I don't know why we do this for IRT and MID, but whatever */
	/* Should probably strip anything that's not an MID (like phrases) */
//	if (t1)
//	{
//		SetHandleSize(t1,RemoveChar('\015',LDRef(t1),GetHandleSize(t1)));
//		UL(t1);
//	}
	if (t2)
	{
		SetHandleSize(t2,RemoveChar('\015',LDRef(t2),GetHandleSize(t2)));
		UL(t2);
	}
	if (t3)
	{
		SetHandleSize(t3,RemoveChar('\015',LDRef(t3),GetHandleSize(t3)));
		UL(t3);
	}
	
	/* If there's a message ID, add IRT header */
	if(t3)
	{
		/* Add the header and the colon */
		if(!(err = AccuAddRes(&extras, HeaderStrn+IN_REPLY_TO_HEAD)))
			/* Add a space */
			if(!(err = AccuAddChar(&extras, ' ')))
				/* Add the message ID */
				if(!(err = AccuAddHandle(&extras, t3)))
					/* Add a return */
					err = AccuAddChar(&extras, '\015');
	}

	/* If there's any id's (from Refs, IRT, or MID), add Refs header */
	if (!err && (/*t1 || */t2 || t3))
	{
		/* Add the header and the colon */
		if(!(err = AccuAddRes(&extras, HeaderStrn+REFERENCES_HEAD)))
			/* Add a space */
			if(!(err = AccuAddChar(&extras, ' ')))
				/* Add the Refs message IDs if they exist */
				if (!t2 || !(err = AccuAddHandle(&extras,t2)))
				{
//					/* Add the IRT message ID if it exists */
//					if (t1 /* && t1 does not appear in t2 */)
//						/* Add a space if there were Refs message IDs */
//						if (!t2 || !(err = AccuAddChar(&extras,' ')))
//							err = AccuAddHandle(&extras,t1);

					/* Add the MID if it exists */
					if (!err && t3)
						/* Add a space if there were Refs or IRT message IDs */
						if ((/*!t1 && */!t2) || !(err = AccuAddChar(&extras,' ')))
							err = AccuAddHandle(&extras,t3);
					
					/* Add a return */
					if (!err) err = AccuAddChar(&extras,'\015');
				}
	}
	
	
	if (err) extras.offset = origLen;
	AccuTrim(&extras);
	(*newMessH)->extras = extras;
//	ZapHandle(t1);
	ZapHandle(t2);
	ZapHandle(t3);
	return(err);
}
/************************************************************************
 * EnsureMessNewline - make sure a message ends in a newline
 ************************************************************************/
OSErr EnsureMessNewline(MessHandle messH)
{
	Handle text;
	long size;
	OSErr err;
	
	if (!(err=PETEGetRawText(PETE,TheBody,&text)))
	{
		size = GetHandleSize(text);
		if (size<1||(*text)[size-1]!='\015')
			err = PETEInsertTextPtr(PETE,TheBody,0x7fffffff,"\015\015",2,nil);
		else if (!err && (size<2||(*text)[size-2]!='\015'))
			err = PETEInsertTextPtr(PETE,TheBody,0x7fffffff,"\015",1,nil);
		if (!err)
			err = PETEInsertParaPtr(PETE,TheBody,kPETELastPara,nil,nil,0,nil);
		MessPlainBytes(messH,0,-1);
	}
	return(err);
}

/************************************************************************
 * Attribute - make an attribution
 ************************************************************************/
void Attribute(short attrId,MessHandle origMessH,MessHandle newMessH,Boolean atEnd)
{
	Str255 attribution;
	
	if (*GrabAttribution(attrId,(*origMessH)->win,attribution))
	{
		if (atEnd)
		{
			long endOfText = PeteLen((*newMessH)->bodyPTE);
			PeteInsertPtr((*newMessH)->bodyPTE,endOfText-1,attribution+1,*attribution);
			//if (MessIsRich(newMessH) || MessIsRich(origMessH))	//quote color leaking, so disable check
				MessPlainBytes(newMessH,0,-(short)*attribution-1);
		}
		else
		{
			PCatC(attribution,'\015');
			PrependMessText(newMessH,0,attribution+1,*attribution);
			//if (MessIsRich(newMessH) || MessIsRich(origMessH))	//quote color leaking, so disable check
				MessPlainBytes(newMessH,0,(short)*attribution);
		}
	}
}

/************************************************************************
 * GrabAttribution - compute the attribution for a message
 ************************************************************************/
PStr GrabAttribution(short attrId,MyWindowPtr win,PStr attribution)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Str255 template;
	Str63 date,time;
	Str127 who;
	long secs;
	long zone;
	long sumNum;
	MSumType sum;
	
	*attribution = 0;
	if (IsMessWindow(winWP))
		sum = *SumOf(Win2MessH(win));
	else if (GetWindowKind(winWP)==MBOX_WIN && (0<=(sumNum=LastMsgSelected(Win2TOC(win)))))
		sum = (*Win2TOC(win))->sums[sumNum];
	else return(attribution);
	
	GetRString(template,attrId);
	if (*template)
	{
		FindFrom(who,win->pte);
		if (*who)
		{
			zone = /* whine, whine, whine PrefIsSet(PREF_LOCAL_DATE) ? ZoneSecs() : */ 60*sum.origZone;
			secs = sum.seconds + zone;
			TimeString(secs,False,attribution,nil);
			FormatZone(date,zone);
			ComposeRString(time,ATTR_TIME_FMT,attribution,date);
			DateString(secs,shortDate,date,nil);
			if (date[1]==optSpace) date[1] = ' ';
			utl_PlugParams(template,attribution,who,date,sum.subj,time);
		}
	}
	return(attribution);
}

/************************************************************************
 * DoRedistributeMessage - craft a reply to a message
 ************************************************************************/
MyWindowPtr DoRedistributeMessage(MyWindowPtr win,TextAddrHandle toWhom,Boolean turbo,Boolean andDelete,Boolean showIt)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
	MessHandle newMessH;
	Str255 scratch;
	int bodyOffset;
	MyWindowPtr newWin;
	long newBo;
	OSErr err;
	PETETextStyle style;
	Boolean html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
	
	if (GetWindowKind(winWP)!=COMP_WIN && !SaveMessHi(win,False)) return(nil);

#ifdef	IMAP
	//	Make sure message has been downloaded if IMAP
	if (!EnsureMsgDownloaded((*origMessH)->tocH,(*origMessH)->sumNum,true))
		return nil;
#endif

	PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
	
	if (newWin=DoComposeNew(toWhom))
	{
		WindowPtr	newWinWP = GetMyWindowWindowPtr (newWin);
		newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
		XferCustomTable(origMessH,newMessH);
		SetMessOpt(newMessH,OPT_REDIRECTED);		
		SetMessText(newMessH,FROM_HEAD,"",0);
		FindAndCopyHeader(origMessH,newMessH,HeaderName(SUBJ_HEAD),SUBJ_HEAD);
		FindAndCopyHeader(origMessH,newMessH,HeaderName(FROM_HEAD),FROM_HEAD);
		if (MessFlagIsSet(origMessH,FLAG_OUT)) FindAndCopyHeader(origMessH,newMessH,HeaderName(ATTACH_HEAD),ATTACH_HEAD);
		RedirectAnnotation(newMessH);

		bodyOffset = CompBodyOffset(origMessH);
		newBo = PETEGetTextLen(PETE,(*newMessH)->bodyPTE);
		
#ifdef DEBUG
		if (BUG14) UpdateMyWindow(winWP);
#endif
		err = PeteCopyNoLabel((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
										bodyOffset,PETEGetTextLen(PETE,(*origMessH)->bodyPTE),newBo,nil,False,html?0:0xffffffff);
#ifdef DEBUG
		if (BUG14) UpdateMyWindow(winWP);
#endif
		style.tsLock = PrefIsSet(PREF_LOCK_REDIR) ? peModLock : peNoLock;
#ifdef DEBUG
		if (BUG14) UpdateMyWindow(winWP);
#endif
		PETESetTextStyle(PETE,(*newMessH)->bodyPTE,newBo,0x7fffffff,&style,peLockValid);
#ifdef DEBUG
		if (BUG14) UpdateMyWindow(winWP);
#endif

		if (err)
		{
			NoSaves = True; CloseMyWindow(newWinWP); NoSaves = False;
			WarnUser(PETE_ERR,err);
			PopPers();
			return(nil);
		}
		if (SumOf(origMessH)->state!=SENT && SumOf(origMessH)->state!=UNSENT)
			SumOf(newMessH)->sigId = SIG_NONE;


		/*
		 * copy priority
		 */
		SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
			SumOf(origMessH)->origPriority;
		
		SumOf(newMessH)->outType = OUT_REDIRECT;	//	for statistics

		/*
		 * state stuff
		 */
		AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->uidHash);
		AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->state);
		AccuAddLong(&(*newMessH)->aSourceMID,REDIST);
		SetState((*origMessH)->tocH,(*origMessH)->sumNum,REDIST);
		UpdateSum(newMessH,SumOf(newMessH)->offset,SumOf(newMessH)->length);
		WeedXAttachments(newMessH,true);
		if (MessFlagIsSet(origMessH,FLAG_HAS_ATT)) CopyAttachments(newMessH);
		SpoolAttachments(newMessH);
#ifdef TWO
		if (turbo && toWhom)
		{
			QueueMessage((*newMessH)->tocH,(*newMessH)->sumNum,PrefIsSet(PREF_AUTO_SEND)?kEuSendNow:kEuSendNext,0,true,false);
			if (andDelete) DeleteMessage((*origMessH)->tocH,(*origMessH)->sumNum,False);
		}
		
		if (showIt && (!turbo || !toWhom))
#endif
		{
			ShowMyWindow(newWinWP);
			newWin->isDirty = False;
			PeteCleanList(newWin->pte);
		}
	}
	PopPers();
	return(newWin);
}

/**********************************************************************
 * RedirectAnnotation - add proper annotation to redirect
 **********************************************************************/
OSErr RedirectAnnotation(MessHandle messH)
{
	Str255 scratch, who, orig;
	OSErr err = noErr;
	
	CompHeadGetStr(messH,FROM_HEAD,orig);
	if (!IsMe(orig))
	{
		// Trim the trailing comment
		UPtr spot = orig+*orig;
		if (*spot==')')
		{
			while (spot>orig && *spot!='(' && *spot!='<') spot--;
			if (spot>orig && *spot=='(') *orig = spot-orig-1;
		}
		
		// Now massage
		if (*GetRealname(who))
		{
			ComposeRString(scratch,REDIST_ANNOTATE,who);
			TrimWhite(orig);
			PCat(orig,scratch);
			err = SetMessText(messH,FROM_HEAD,orig+1,*orig);
		}
	}
	return err;
}

/**********************************************************************
 * CopyAttachments - copy attachments out of the message body and into
 *  the attachments line
 **********************************************************************/
OSErr CopyAttachments(MessHandle messH)
{
	long offset, absOffset;
	UHandle text;
	FSSpec attSpec;
	UPtr spot;
	HeadSpec hs;
	
	if (!PeteGetTextAndSelection(TheBody,&text,nil,nil))
	{
		CompHeadFind(messH,0,&hs);
		for (offset = 0;0<=(absOffset = FindAnAttachment(text,offset+hs.start,&attSpec,False,nil,nil,nil));)
		{
			offset = absOffset-hs.start-1;
			for (spot=*text+absOffset;*spot!='\015' && spot<*text+hs.stop;spot++);
			PETEInsertTextPtr(PETE,TheBody,absOffset,nil,(spot-*text)-absOffset,nil);
			CompAttachSpec((*messH)->win,&attSpec);
			CompHeadFind(messH,0,&hs);
		}
		for (offset = 0;0<=(absOffset = FindAnAttachment(text,offset+hs.start,&attSpec,True,nil,nil,nil));)
		{
			offset = absOffset-hs.start-1;
			for (spot=*text+absOffset;spot<*text+hs.stop && *spot!='\015';spot++);
			PETEInsertTextPtr(PETE,TheBody,absOffset,nil,(spot-*text)-absOffset,nil);
			CompAttachSpec((*messH)->win,&attSpec);
			CompHeadFind(messH,0,&hs);
		}
	}
	return noErr;	/* I have no idea what kinds of errors can happen here - mtc 11-18-03 */
}


/************************************************************************
 * DoForwardMessage - forward a message to someone
 ************************************************************************/
MyWindowPtr DoForwardMessage(MyWindowPtr win,TextAddrHandle toWhom,Boolean showIt)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
	MessHandle newMessH;
	MyWindowPtr newWin=nil;
	WindowPtr		newWinPtr;
	Str255 scratch, subj;
	long offset;
	PETETextStyle style;
	Boolean rich;
	long origBo;
	HeadSpec hs;
	Boolean html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);

	NicknameWatcherFocusChange (win->pte); /* MJN */

	if (GetWindowKind(winWP)!=COMP_WIN && !SaveMessHi(win,False)) return(nil);
	
#ifdef	IMAP
	//	Make sure message has been downloaded if IMAP
	if (!EnsureMsgDownloaded((*origMessH)->tocH,(*origMessH)->sumNum,true))
		return nil;
#endif

	PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
	if (newWin=DoComposeNew(toWhom))
	{
		newWinPtr = GetMyWindowWindowPtr (newWin);
#ifdef DEBUG
		if (BUG14)
		{
			ShowMyWindow(newWinPtr);
			UpdateMyWindow(newWinPtr);
		}
#endif
		newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
		rich = UseFlowOutExcerpt || (MessIsRich(origMessH) || MessIsRich(newMessH));

		XferCustomTable(origMessH,newMessH);		
		if (SumOf(origMessH)->flags&FLAG_OUT)
		{
			BuildDateHeader(scratch,SumOf(origMessH)->seconds);
			AppendMessText(newMessH,0,scratch+1,*scratch);
			AppendMessText(newMessH,0,"\015",1);
		}
		
		/* handle the subject */
		FindAndCopyHeader(origMessH,newMessH,HeaderName(SUBJ_HEAD),SUBJ_HEAD);
		if (*GetRString(scratch,FWD_PREFIX))
		{
			TrimWhite(scratch);
			CompHeadGetStr(newMessH,SUBJ_HEAD,subj);
			if (!ReMatch(subj,scratch))
			{
				GetRString(scratch,FWD_PREFIX);
				PrependMessText(newMessH,SUBJ_HEAD,scratch+1,*scratch);
			}
		}
		
		/*
		 * stick it in
		 */
		offset = PETEGetTextLen(PETE,(*newMessH)->bodyPTE);
		CompHeadFind(origMessH,0,&hs);
		origBo = hs.value;
		PeteCopy((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
							0,origBo,offset,nil,True);
		PETEInsertParaPtr(PETE,(*newMessH)->bodyPTE,kPETELastPara,nil,nil,0,nil);	// make sure we have an empty para at end
		PeteCopyNoLabel((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
							origBo,PETEGetTextLen(PETE,(*origMessH)->bodyPTE),
							PETEGetTextLen(PETE,(*newMessH)->bodyPTE),nil,False,html?0:0xffffffff);
#ifdef NEVER
		PeteCopy((*origMessH)->bodyPTE,(*newMessH)->bodyPTE,
							0,PETEGetTextLen(PETE,(*origMessH)->bodyPTE),
							offset,nil,rich);
#endif
		style.tsLock = 0;
		PETESetTextStyle(PETE,(*newMessH)->bodyPTE,offset,0x7fffffff,&style,peLockValid);

		if (rich && !(MainEvent.modifiers&optionKey) && *GetRString(scratch,FWD_QUOTE))
		{
			PeteExcerpt((*newMessH)->bodyPTE,offset,PETEGetTextLen(PETE,(*newMessH)->bodyPTE)-1);
			if (PrefIsSet(PREF_SCHEERDER))
			{
				CompHeadFind(newMessH,0,&hs);
				PeteSelect(nil,(*newMessH)->bodyPTE,hs.value,PeteLen((*newMessH)->bodyPTE));
				PeteWrap((*newMessH)->win,(*newMessH)->bodyPTE,false);
			}
		}


		/*
		 * make sure the last line is blank
		 */
		EnsureMessNewline(newMessH);

		/*
		 * deal with attachments
		 */
		if (SumOf(origMessH)->flags&FLAG_OUT)
		{
			HeadSpec hs;
			UHandle text = nil;
			if (CompHeadFind(origMessH,ATTACH_HEAD,&hs) && !CompHeadGetText((*origMessH)->bodyPTE,&hs,&text) &&
					CompHeadFind(newMessH,ATTACH_HEAD,&hs))
			{
				CompHeadSet((*newMessH)->bodyPTE,&hs,text);
			}
			ZapHandle(text);
		}
		else
		{
			if (MessFlagIsSet(origMessH,FLAG_HAS_ATT)) CopyAttachments(newMessH);
			SpoolAttachments(newMessH);
		}
		
		/*
			* quote them
			*/
		if (!rich && !(MainEvent.modifiers&optionKey))
			QuoteLines((*newMessH)->bodyPTE,CompBodyOffset(newMessH)-1,0x7fffffff,FWD_QUOTE,nil);

		if (GetWindowKind(winWP)!=COMP_WIN)
		{
			AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->uidHash);
			AccuAddLong(&(*newMessH)->aSourceMID,SumOf(origMessH)->state);
			AccuAddLong(&(*newMessH)->aSourceMID,FORWARDED);
			SetState((*origMessH)->tocH,(*origMessH)->sumNum,FORWARDED);
		}
	  
		/*
		 * Attributions
		 */
		Attribute(FWD_INTRO,origMessH,newMessH,False);
		Attribute(FWD_TRAIL,origMessH,newMessH,True);

		/*
		 * copy priority (not my idea)
		 */
		if (!PrefIsSet(PREF_NO_XF_PRIOR))
			SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
				SumOf(origMessH)->origPriority;
		
		SumOf(newMessH)->outType = OUT_FORWARD;	//	for statistics
		
#ifdef TWO
		ApplyDefaultStationery(newWin,True,True);
#endif
		UpdateSum(newMessH,SumOf(newMessH)->offset,SumOf(newMessH)->length);
		if (showIt)
		{
			ShowMyWindow(newWinPtr);
			newWin->isDirty = False;
			PeteCleanList(newWin->pte);
		}
	}
	PopPers();
	return(newWin);
}

/**********************************************************************
 * CompBodyOffset - find the body in a comp message
 **********************************************************************/
long CompBodyOffset(MessHandle messH)
{
	HeadSpec hs;
	
	if (CompHeadFind(messH,0,&hs)) return(hs.value);
	else return(0);
}

/**********************************************************************
 * DoFordirectMessage - forward or redirect a message, silently
 **********************************************************************/
OSErr DoFordirectMessage(TOCHandle tocH,short sumNum,short flk,PStr addresses,Boolean bulk)
{
	Boolean iOpened = !(*tocH)->sums[sumNum].messH;
	MyWindowPtr origWin = GetAMessage(tocH,sumNum,nil,nil,False);
	MessHandle newMessH;
	OSErr err;
	MyWindowPtr win;
	
	//Enhanced Filters	no such filter functionality in Light
	if (!HasFeature (flk==flkForward ? featureFilterForward : featureFilterRedirect))
		return (1);
		
	UseFeature (flk==flkForward ? featureFilterForward : featureFilterRedirect);
	if (!origWin) return(1);
	
	win = flk==flkForward ? DoForwardMessage(origWin,0,False) :
													DoRedistributeMessage(origWin,0,False,False,False);
	
	if (iOpened) CloseMyWindow(GetMyWindowWindowPtr(origWin));
	
	if (!win) return(1);
	
	SetState(tocH,sumNum,flk==flkForward ? FORWARDED : REDIST);
	newMessH = Win2MessH(win);
	if (bulk) SetMessOpt(newMessH,OPT_BULK);
	SetMessText(newMessH,TO_HEAD,addresses+1,*addresses);
	if (err=QueueMessage((*newMessH)->tocH,(*newMessH)->sumNum,kEuSendNext,0,true,true))
		DeleteMessage((*newMessH)->tocH,(*newMessH)->sumNum,False);
	return(err);
}

/**********************************************************************
 * DoReplyClosed - reply to a closed message, silently
 **********************************************************************/
OSErr DoReplyClosed(TOCHandle tocH,short sumNum,Boolean all, Boolean self, Boolean quote, Boolean doFcc, short withWhich,Boolean vis,Boolean station)
{
	Boolean iOpened = !(*tocH)->sums[sumNum].messH;
	MyWindowPtr origWin = GetAMessage(tocH,sumNum,nil,nil,False);
	MessHandle newMessH;
	OSErr err;
	MyWindowPtr win;
	
	if (!origWin) return(1);
	
	win = DoReplyMessage(origWin, all, self, quote, doFcc, withWhich,False,station,True);
	
	if (iOpened) CloseMyWindow(GetMyWindowWindowPtr(origWin));
	
	if (!win) return(1);
	
	SetState(tocH,sumNum,REPLIED);
	newMessH = Win2MessH(win);
	SetMessOpt(newMessH,OPT_BULK);
	if (err=QueueMessage((*newMessH)->tocH,(*newMessH)->sumNum,kEuSendNext,0,true,true))
		DeleteMessage((*newMessH)->tocH,(*newMessH)->sumNum,False);
	return(err);
}

/************************************************************************
 * FindAndCopyHeader - pick the given header out of a message, and
 * copy it to a composition message
 ************************************************************************/
int FindAndCopyHeader(MessHandle origMH,MessHandle newMH,UPtr fromHead,short toHead)
{
	UPtr body;
	long size;
	int result;
	Handle text;
	
	PeteGetRawText((*origMH)->bodyPTE,&text);
	
	body = LDRef(text);
	size = BodyOffset(text);
	result = TextFindAndCopyHeader(body,size,newMH,fromHead,toHead,0);
	HUnlock(text);
	return(result);
}

/************************************************************************
 * CopyNewsgroups - copy newsgroups into new message
 ************************************************************************/
OSErr CopyNewsgroups(MessHandle origMH,MessHandle newMH)
{
	Str255 s;
	HeadSpec hs, origHS;
	UHandle text=nil;
	UHandle addresses = nil;
	OSErr err = fnfErr;
	UPtr address;
	Boolean first = true;
		
	GetRString(s,NEWSGROUPS);
		
	if (CompHeadFindStr(origMH,s,&origHS))
	if (!(err=CompHeadGetText((*origMH)->bodyPTE,&origHS,&text)))
	if (!(err=SuckAddresses(&addresses,text,false,false,false,nil)) && addresses && *addresses && **addresses)
	{
		if (CompHeadFind(newMH,BCC_HEAD,&hs))
		{
			if (*GetRString(s,MAIL2NEWS))
			{
				if (hs.value!=hs.stop) InsertCommaIfNeedBe((*newMH)->bodyPTE,&hs);
				CompHeadAppendPtr((*newMH)->bodyPTE,&hs,s+1,*s);
				first = false;
			}
			for(address=LDRef(addresses);*address;address+=*address+2)
			{
				if (!first || hs.value!=hs.stop)
					if (!InsertCommaIfNeedBe((*newMH)->bodyPTE,&hs))
						CompHeadAppendPtr((*newMH)->bodyPTE,&hs," ",1);
				
				if (err=CompHeadAppendPtr((*newMH)->bodyPTE,&hs,"�",1)) break;
				if (err=CompHeadAppendPtr((*newMH)->bodyPTE,&hs,address+1,*address)) break;
				first = false;
			}
		}
	}
	return(err);
}

/************************************************************************
 * TextFindAndCopyHeader - pick the given header out of a text block, and
 * copy it to a composition message
 ************************************************************************/
int TextFindAndCopyHeader(UPtr body,long size,MessHandle newMH,UPtr fromHead,short toHead,short label)
{
	MyWindowPtr newWin = (*newMH)->win;
	UPtr bodyEnd = body+size;
	UPtr spot;
	Boolean first = True;
	HeadSpec hs;
	long labStart;
		
	if ((body = FindHeaderString(body,fromHead,&size,False)) && size)
	{
		if (CompHeadFind(newMH,toHead,&hs))
		{
			labStart = hs.stop;
			for(;;)
			{
				if (!first || hs.value!=hs.stop)
					if (!InsertCommaIfNeedBe((*newMH)->bodyPTE,&hs))
						CompHeadAppendPtr((*newMH)->bodyPTE,&hs," ",1);
				
				CompHeadAppendPtr((*newMH)->bodyPTE,&hs,body,size);
	
				first = False;
				
				body += size;
				while (IsWhite(*body)&&body<bodyEnd) body++;			/* skip to newline */
				body++; 																					/* skip newline */
				if (body<bodyEnd && IsWhite(*body)) 							/* continuation */
				{
					while (IsWhite(*body)&&body<bodyEnd) body++;
					for (spot=body;spot<bodyEnd&&*spot!='\015';spot++);
					do {spot--;} while (IsWhite(*spot));
					size = spot-body+1;
					if (!size) break;
				}
				else
					break;
			}
			if (label && CompHeadFind(newMH,toHead,&hs) && hs.stop>labStart)
				PeteLabel((*newMH)->bodyPTE,labStart,hs.stop,label,label);
		}
	}
	return(body!=nil);
}

/************************************************************************
 * XferCustomTable - transfer a custom table from a message to its reply
 * (or forward or redirect)
 ************************************************************************/
void XferCustomTable(MessHandle origMessH,MessHandle newMessH)
{
	short origTable;
	Boolean isOut;
	
	/*
	 * under the old regime, we don't do this step
	 */
	if (!NewTables) return;
	
	origTable = SumOf(origMessH)->tableId;

	/*
	 * if the original message had the default table, give the default table
	 * to the new message (which has already been done, so return)
	 */
	if (origTable == DEFAULT_TABLE) return;

	/*
	 * if no table was on the original mail, use the default table
	 * this is different from non-MIME mail
	 */
	if (!origTable) return;
	
	/*
	 * if original was outgoing, use same table
	 */
	isOut = (SumOf(origMessH)->flags&FLAG_OUT)!=0;
	if (isOut) SumOf(newMessH)->tableId = origTable;
		
	/*
	 * Otherwise, look for an out table that corresponds to the In table
	 * and use that.
	 */
	else if (GetResource_('taBL',origTable+1))
		SumOf(newMessH)->tableId = origTable+1;
}


/************************************************************************
 * WeedXAttachments - figure out if all the attachments are there
 ************************************************************************/
void WeedXAttachments(MessHandle messH,Boolean errReport)
{
	short i;
	FSSpec spec;
	short err;
	short removed = 0;
	Boolean foundAny = False;
	HeadSpec hs;
	
	for (i=1;1!=(err=GetIndAttachment(messH,i,&spec,nil));i++)
	{
		if (err)
		{
			/* attachment does not exist */
			removed++;
			RemoveIndAttachment(messH,i);
			i--;
		}
		else foundAny = True;
	}
	if (!foundAny && CompHeadFind(messH,ATTACH_HEAD,&hs) && hs.value!=hs.stop)
	{
		CompHeadSetPtr(TheBody,&hs,"",0);
		removed = True;
	}
	if (removed && errReport) WarnUser(ATTACH_REMOVED,removed);	
}

/**********************************************************************
 * SpoolAttachments - copy attachments to the spool area
 **********************************************************************/
OSErr SpoolAttachments(MessHandle messH)
{
	short n;
	short i;
	OSErr err = noErr;
	FSSpec spec;
	
	/*
	 * count the attachments
	 */
	for (n=0;!GetIndAttachment(messH,n+1,&spec,nil);n++);
	
	for (i=1;i<=n;i++) if (err=SpoolIndAttachment(messH,1)) break;
	
	return(err);
}

/**********************************************************************
 * SpoolIndAttachment - spool a single attachment
 **********************************************************************/
OSErr SpoolIndAttachment(MessHandle messH,short i)
{
	FSSpec spec, newSpec;
	OSErr err = noErr;
	
	
	/*
	 * make the folder
	 */
	if (err = MakeAttSubFolder(messH,SumOf(messH)->uidHash,&newSpec))
		return(FileSystemError(COPY_ATTACHMENT,newSpec.name,err));

	/*
	 * grab it
	 */
	if (!GetIndAttachment(messH,i,&spec,nil))
	{
		/*
		 * Copy the attachment
		 */
		PCopy(newSpec.name,spec.name);
		if (!SameSpec(&spec,&newSpec) && !FSpIsItAFolder(&spec))
		{
			Str255 longName;

			if ((err=FSpDupFile(&newSpec,&spec,False,False)))
				return(FileSystemError(COPY_ATTACHMENT,spec.name,err));
						
			// handle long filename
			if (!FSpGetLongName(&spec,kTextEncodingUnknown,longName) && *longName>31)
				FSpSetLongName(&newSpec,kTextEncodingUnknown,longName,&newSpec);

			CompAttachSpec((*messH)->win,&newSpec);
			RemoveIndAttachment(messH,i);
		}
	}
	return(err);
}

/**********************************************************************
 * MakeAttSubFolder - make the subfolder for attachment spooling
 **********************************************************************/
OSErr MakeAttSubFolder(MessHandle messH,uLong uidHash,FSSpecPtr folder)
{
	OSErr err;
	FSSpec spool;
	Str255 scratch;
	long dirID;
	
	if (messH && !MessOptIsSet(messH,OPT_HAS_SPOOL)) SetMessOpt(messH,OPT_HAS_SPOOL);
	GetRString(folder->name,SPOOL_FOLDER);	// in case of error
	
	/*
	 * find the folder
	 */
	if (err=SubFolderSpec(SPOOL_FOLDER,&spool))
		return(err);
	
	/*
	 * message id
	 */
	NumToString(uidHash,scratch);
	
	/*
	 * specify
	 */
	SimpleMakeFSSpec(spool.vRefNum,spool.parID,scratch,folder);
	
	/*
	 * create
	 */
	err = FSpDirCreate(folder,smSystemScript,&dirID);
	if (err==dupFNErr)
	{
		dirID = SpecDirId(folder);
		if (dirID) err = noErr;
	}
	if (err) return(err);
	
	/*
	 * and make it point into the folder
	 */
	folder->parID = dirID;
	*folder->name = 0;
	
	return(err);
}

/************************************************************************
 * RemoveIndAttachment - remove a particular attachment
 ************************************************************************/
void RemoveIndAttachment(MessHandle messH, short index)
{
	HeadSpec where;
	FSSpec spec;
	OSErr err = GetIndAttachment(messH,index,&spec,&where);
	
	if (err!=1)
	{
		CompDelAttachment(messH,&where);
		AttachSelect(messH);
	}
}

/************************************************************************
 * CopyToOut - copy a message to the Out mailbox
 ************************************************************************/
OSErr CopyToOut(TOCHandle fromTocH,short sumNum,TOCHandle toTocH)
{
	MessHandle messH;
	short err = 1;
	MyWindowPtr win=nil, newWin=nil;
	MSumType newSum,oldSum;
	
	if (!(win = GetAMessage(fromTocH,sumNum,nil,nil,False))) return(1);
	messH = Win2MessH(win);
	fromTocH = (*messH)->tocH;
	sumNum = (*messH)->sumNum;
	
	if (newWin = DoSalvageMessage((*messH)->win,True))
	{
		if (SaveComp(newWin))
		{
			newSum = (*toTocH)->sums[(*toTocH)->count-1];
			oldSum = (*fromTocH)->sums[sumNum];
			SumInfoCpy(&newSum,&oldSum);
			newSum.flags |= FLAG_OUT;
			(*toTocH)->sums[(*toTocH)->count-1] = newSum;
			err = noErr;
		}
		NoSaves = True; CloseMyWindow(GetMyWindowWindowPtr(newWin)); NoSaves = False;
	}
	if (win) CloseMyWindow(GetMyWindowWindowPtr(win));
	return(err);
}

/************************************************************************
 * SumInfoCpy - copy non-essential summary fields
 ************************************************************************/
void SumInfoCpy(MSumPtr newSum, MSumPtr oldSum)
{
	newSum->state = oldSum->state;
	PCopy(newSum->from,oldSum->from);
	PCopy(newSum->subj,oldSum->subj);
	newSum->seconds = oldSum->seconds;
	newSum->flags = oldSum->flags;
	newSum->tableId = oldSum->tableId;
	newSum->sigId = SigValidate(oldSum->sigId);
	newSum->priority = oldSum->priority;
	newSum->origPriority = oldSum->origPriority;
	BMD(&oldSum->spareShort2,&newSum->spareShort2,sizeof(newSum->spareShort2));
	if (oldSum->opts&OPT_INLINE_SIG) newSum->opts |= OPT_INLINE_SIG;
}

/* The following hashing algorithm, KRHash, is derived from Karp & Rabin, 
    Harvard Center for Research in Computing Technology Tech Report TR-31-81. */ 
/* The prime number in use is KRHashPrime.  It happens to be 
    the largest prime number that will fit in 31 bits, except for 2^31-1 itself. */ 

#define KRHashPrime (2147483629) 

/************************************************************************
 * HashWithSeed - generate a hash from a string and a seed.
 ************************************************************************/
uLong HashWithSeedLo(UPtr s, uLong n, uLong seed)
{
	uLong sum = seed-1; 
	int Bit;

	for (;n;n--,s++) { 
			for (Bit = 0x80; Bit != 0; Bit >>= 1) { 
					sum += sum; 
					if (sum >= KRHashPrime) sum -= KRHashPrime; 
					if ((*s) & Bit) ++sum; 
					if (sum >= KRHashPrime) sum -= KRHashPrime; 
			} 
	} 
	return(sum+1); 
}

/************************************************************************
 * MIDHash - hash a message id, stripping <>'s first
 ************************************************************************/
uLong MIDHash(UPtr text,long size)
{
	Str255 scratch;
	UHandle addresses;
	
	SuckPtrAddresses(&addresses,text,size,False,False,False,nil);
	
	if (addresses)
	{
		PCopy(scratch,*addresses);
		ZapHandle(addresses);
		return(Hash(scratch));
	}
	else return(kNoMessageId);
}

/************************************************************************
 * SetHash - set a message's hash function
 ************************************************************************/
void SetHashLo(TOCHandle tocH,short sumNum,uLong hash,Boolean soft)
{
	DBNoteUIDHash((*tocH)->sums[sumNum].uidHash,hash);
	if (!soft || !ValidHash((*tocH)->sums[sumNum].uidHash))
		(*tocH)->sums[sumNum].uidHash = hash;
	if (!soft || !ValidHash((*tocH)->sums[sumNum].msgIdHash))
		(*tocH)->sums[sumNum].msgIdHash = hash;
	TOCSetDirty(tocH,true);
}

/************************************************************************
 * Rehash - recompute the hash for a message
 ************************************************************************/
void RehashLo(TOCHandle tocH,short sumNum,UHandle text,Boolean soft)
{
	Str255 scratch;
	UPtr spot;
	long size = GetHandleSize_(text);
	uLong hash;
	
	spot = LDRef(text);
	GetRString(scratch,HEADER_STRN+MSGID_HEAD);
	if (spot = FindHeaderString(spot,scratch,&size,False))
		hash = MIDHash(spot,size);
	else
	{
		NewMessageId(scratch);
		hash = MIDHash(scratch+1,*scratch);
	}
	SetHashLo(tocH,sumNum,hash,soft);
	UL(text);	
}

#define PREVIEW_ID_MULT_REDO	(-3)
#define PREVIEW_ID_MULT (-2)
/************************************************************************
 * Preview - preview a message, if desired
 ************************************************************************/
void Preview(TOCHandle tocH,short sumNum)
{
	WindowPtr	tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
	MessHandle messH;
	long id;
	PETEHandle pte;
	MyWindowPtr messWin = nil;
	Boolean active = false;
	short ezOpenSum;
	OSErr err;
	Str63 profileName;
#ifdef	IMAP
	short oldPreview;
#endif
		
	if (!(pte = (*tocH)->previewPTE)) return;

	messH = (*tocH)->sums[sumNum].messH;
	if (sumNum>=0)
	{
		EnsureMID(tocH,sumNum);
		id = (*tocH)->sums[sumNum].serialNum;
	}
	else if (sumNum==-2)
	{
		if (!(*tocH)->conConMultiScan) id = (*tocH)->previewID;
		else if (ConConMultipleAppropriate(tocH))
		{
			(*tocH)->previewID = PREVIEW_ID_MULT_REDO;
			id = PREVIEW_ID_MULT;
		}
		else
			id = 0;
	}
	else
		id = 0;
	
	if ((*tocH)->previewID==id)
	{
		PeteCalcOn((*tocH)->previewPTE);
		if (id && (*tocH)->lastSameTicks!=1 && id!=PREVIEW_ID_MULT)	// we have a message and we're not already done
		{
			if (!InBG	// front only
					&& IsWindowVisible(tocWinWP) // visible
					&& tocWinWP==FrontWindow_()	// frontmost
					&& (*tocH)->userActive)	// user has recently clicked or typed at me
			{
		    if (TickCount()-(*tocH)->lastSameTicks>10 // it's been a while since we last checked
		    		&& PreviewReadTimer // the user wants this marking
		    		&& TickCount()-(*tocH)->lastSameTicks>GetRLong(PREVIEW_READ_SECS)*60)	// and it's been long enough
				{
					(*tocH)->lastSameTicks = 1;
#ifdef	IMAP
					// do not automatically mark IMAP minimal headers as read, ever.
					if (!(*tocH)->imapTOC || IMAPMessageDownloaded(tocH,sumNum))
#endif					
						BeenThereDoneThat(tocH,sumNum);
				}
			}
			else
				(*tocH)->lastSameTicks = TickCount();	// reset counter if user not active	
		}
		return;
	}
	else (*tocH)->lastSameTicks = TickCount();

#ifdef	IMAP
	if ((*tocH)->previewID && (*tocH)->imapTOC)
	{
		// Cancel the IMAP download if this is an imap message ...
		FindRealSummary(tocH, (*tocH)->previewID, &oldPreview);
		IMAPAbortMessageFetch(tocH, oldPreview);
	}
#endif
	
	(*tocH)->previewID = id;
	(*tocH)->ezOpenSerialNum = 0;
	
	PeteSetTextPtr(pte," ",1);	// put something into it so that all the plains work
	PetePlain(pte,0,1,peAllValid);
	PetePlainParaAt(pte,0,1);
	PeteLock(pte,0,1,peModLock|peSelectLock);
	PeteSetTextPtr(pte,"",0);	// clear it now
	
	if (id && id!=PREVIEW_ID_MULT)
	{
		SelectBoxRange(tocH,sumNum,sumNum,false,-1,-1);

		// need to open?
		if (!messH)
		{
			short	realSumNum;
			TOCHandle	realTocH;
			if (!(realTocH = GetRealTOC(tocH,sumNum,&realSumNum)))
				return;
		
			if (!(messH = (*realTocH)->sums[realSumNum].messH))
			{
				messWin = (*realTocH)->which==OUT ? OpenComp(realTocH,realSumNum,nil,nil,false,false) :
#ifdef	IMAP
									OpenMessage(realTocH,realSumNum,nil,nil,false,true);
#else
									OpenMessage(realTocH,realSumNum,nil,nil,false);
#endif
				if (messWin) messH = Win2MessH(messWin);
			}
#ifdef IMAP
			//
			// fire off a thread to fetch the next message as well, if it's not already there.
			// ... but don't do this for the search window.
			//	
			
			if (!Offline && AutoCheckOK() && !((*tocH)->virtualTOC))
			{
				short sumToOpenNext;

				// open the next summary according to EZOpen
				sumToOpenNext = EzOpenFind(realTocH,realSumNum);
				if (sumToOpenNext >= 0)
				{
					if (!IMAPMessageDownloaded(realTocH, sumToOpenNext) && !IMAPMessageBeingDownloaded(realTocH, sumToOpenNext))
						UIDDownloadMessage(realTocH, (*realTocH)->sums[sumToOpenNext].uidHash,false, false);
				}
			}
#endif
		}
		
		if (messH)
		{
			Boolean junk = SumOf(messH)->spamScore >= GetRLong(JUNK_MAILBOX_THRESHHOLD);

			(*PeteExtra(pte))->containsJunkMail = junk;
			PeteCalcOff(pte);
			PETEAllowUndo(PETE,pte,false,true);
			if (!ConConMess(messH,pte,BoxPreviewProfile(profileName,tocH,CONCON_PREVIEW_PROFILE),nil,nil))
			{
				// concentrator did it!
				// lock all the text
				PeteLock(pte,0,PeteLen(pte),peModLock);
				PeteCalcOn(pte);
			}
			else
			{
				long body = SumOf(messH)->bodyOffset - (*messH)->weeded;
				long len;
				long scanned;
				UHandle text;
				long oldID;
				long para;
				PETEParaInfo pinfo;
				long bite;
				
				// no drawing, please
				PeteCalcOff(pte);
				
				// Make sure we examine it for funny business
				HiliteOddReply(messH);

				// figure out what we need to copy
				bite = len = PETEGetTextLen(PETE,TheBody);
								
				// find a paragraph that includes 3K of text
				if (bite>3 K) bite = 3 K;
				PeteParaConvert(TheBody,bite-100,bite+100);
				para = PeteParaAt(TheBody,bite-1);
				Zero(pinfo);
				PETEGetParaInfo(PETE,TheBody,para,&pinfo);
				bite = pinfo.paraOffset+pinfo.paraLength;
				
				// Reset the copy mask to include the reply-to label for now
				PETESetLabelCopyMask(PETE,LABEL_COPY_MASK|pReplyLabel);
				
				PeteGetTextAndSelection(TheBody,&text,nil,nil);
				if (bite==len) while (bite && (*text)[bite-1]=='\015' || IsWhite((*text)[bite-1])) bite--;
				
				// do the copy, but fool the graphics into thinking it's the same Pete record
				oldID = (*PeteExtra(pte))->id;
				(*PeteExtra(pte))->id = (*PeteExtra(TheBody))->id;
				PeteCopy(TheBody,pte,0,bite,0,nil,false);
				(*PeteExtra(pte))->id = oldID;

				// pre-scan for some url's and attachments
				PeteSetURLRescan(pte,0);
				for (scanned=(*PeteExtra(pte))->urlScanned;
						 scanned!=-1 && scanned<3 K;
						 scanned=(*PeteExtra(pte))->urlScanned)
					PeteURLScan((*tocH)->win,pte);
				for (scanned=(*PeteExtra(pte))->quoteScanned;
						 scanned!=-1 && scanned<3 K;
						 scanned=(*PeteExtra(pte))->quoteScanned)
					PeteQuoteScan(pte);

	#ifdef ATT_ICONS
				// Note: code assumes preview shows same headers as message
				if (MessFlagIsSet(messH,FLAG_OUT))
				{
					FSSpec spec;
					short i;
					HeadSpec hs;
				
					for (i=1;1!=GetIndAttachment(messH,i,&spec,&hs);i++)
						PeteFileGraphicRange(pte,hs.start,hs.stop,&spec,fgAttachment);
				}
	#endif
				
				// scroll to proper spot
				PeteSelect(nil,pte,body+1,body+1);
				PETESetCallback(PETE,pte,(void*)PeteCancelOnEvents,peProgressLoop);
				if (err=PeteCalcOn(pte))
				{
					(*tocH)->previewID = 0;
					PeteSetTextPtr(pte,"",0);
				}
				PETESetCallback(PETE,pte,(void*)PeteBusy,peProgressLoop);
				if (!err)
				{
					ShowMessageSeparator(pte,((*tocH)->previewHi/(*tocH)->win->vPitch)>GetRLong(PREVIEW_HEADER_HIDE));
					
					// Now copy the remainder
					if (bite<len)
					{
						PeteCalcOff(pte);
						PeteGetTextAndSelection(TheBody,&text,nil,nil);
						PeteSetURLRescan(pte,3 K - 255);
						while (len && (*text)[len-1]=='\015') len--;
						if (bite<len)
						{
							(*PeteExtra(pte))->id = (*PeteExtra(TheBody))->id;
							PeteCopy(TheBody,pte,bite,len,PETEGetTextLen(PETE,pte),nil,false);
							(*PeteExtra(pte))->id = oldID;
							PETESetCallback(PETE,pte,(void*)PeteCancelOnEvents,peProgressLoop);
							if (PeteCalcOn(pte))
							{
								(*tocH)->previewID = 0;
								PeteSetTextPtr(pte,"",0);
							}
							PETESetCallback(PETE,pte,(void*)PeteBusy,peProgressLoop);
						}
					}
				}

				// Reset the copy mask to include the reply-to label for now
				PETESetLabelCopyMask(PETE,LABEL_COPY_MASK);
				
				// And lock all the text
				PeteLock(pte,0,PeteLen(pte),peModLock);
			}
		}		

		if (messWin) CloseMyWindow(GetMyWindowWindowPtr(messWin));
		if (!PrefIsSet(PREF_NO_EZ_OPEN))
		{
			ezOpenSum = EzOpenFind(tocH,sumNum);
			if (ezOpenSum>=0)
			{
				(*tocH)->ezOpenSerialNum = (*tocH)->sums[ezOpenSum].serialNum;
				CacheMessage(tocH,ezOpenSum);
			}
		}
		
		if (AnalSpeakPhrases())
		{
			(*PeteExtra(pte))->taeDesired = true;
			(*PeteExtra(pte))->taeSpeak = true;
		}
	}
	else if (id==PREVIEW_ID_MULT)
	{
		ConConMultiple(tocH,pte,BoxPreviewProfile(profileName,tocH,CONCON_MULTI_PREVIEW_PROFILE),conConOutSeparatorRule,nil,nil);
		(*tocH)->previewID = id;
	}
	else
		BoxPreviewProfile(nil,tocH,0);
	
	PeteActivate(pte,(*tocH)->win->isActive && !(*tocH)->listFocus,false);
}
			
/************************************************************************
 * HTMLifyText - insert appropriate BR's, process and remove "related:" lines, etc.
 ************************************************************************/
void HTMLifyText(MyWindowPtr win,Handle text)
{
	Accumulator a;
	Ptr	spot,end,lastSpot;
	long	len;
	Str32	sBR;
	char	lastChar;
	long	offset = 0;
	PartDesc pd;
	StackHandle stack;

	//	Insert <BR> after each header line
	if (!AccuInit(&a))
	{
		
		GetRString(sBR,HTMLTagsStrn+htmlBR);
		
		len = 0;
		spot = LDRef(text);
		lastChar = 0;
		lastSpot = spot;
		for(end = spot + GetHandleSize(text); spot < end; spot++)
		{
			if (*spot=='\r')
			{
				AccuAddPtr(&a,lastSpot,spot-lastSpot);
				AccuAddChar(&a,'<');	//	Add <BR>
				AccuAddStr(&a,sBR);
				AccuAddChar(&a,'>');
				if (lastChar == '\r')
					break;	//	Hit 2 newlines in a row. End of headers.
				lastSpot = spot+1;
			}
			lastChar = *spot;
		}
		
		UL(text);
		Munger(text,0,nil,spot-*text,LDRef(a.data),a.offset);
		offset = a.offset;
		AccuZap(a);		
	}
	
	//	Build parts stack from "related:" lines and remove them
	StackInit(sizeof(PartDesc),&stack);
	while (0<=(offset = FindAnAttachment(text,offset,&pd.spec,false,&pd.cid,&pd.relURL,&pd.absURL)))
	{
		Ptr	lineEnd,end;
		long	len;
		
		StackQueue(&pd,stack);
		//	Remove related line
		end = *text + GetHandleSize_(text);
		for (lineEnd=*text+offset;lineEnd<end && *lineEnd!='\015';lineEnd++);
		len = lineEnd-*text;
		if (lineEnd<end) len++;	//	Get past CR
		Munger(text,offset,nil,len,end,nil);	//	Delete the line
	}
	(*PeteExtra(win->pte))->partStack = stack;
}


#ifdef	IMAP
/************************************************************************
 * EnsureMsgDownloaded - if IMAP message, make sure it is downloaded
 ************************************************************************/
Boolean EnsureMsgDownloaded(TOCHandle tocH,short sumNum,Boolean attachmentsToo)
{
	// Actually go fetch this message if we must
	if ((*tocH)->imapTOC)
	{
		short	n;
		Boolean result;
		
		// make sure this message has already been downloaded ...
		if (!IMAPMessageDownloaded(tocH, sumNum))
		{ 
			if (IMAPMessageBeingDownloaded(tocH,sumNum))
			{
				//	This message is currently being downloaded. Wait for it to finish
				while (IMAPMessageBeingDownloaded(tocH,sumNum))
				{
					CycleBalls();
					if (MyYieldToAnyThread())
						break;
					if (CommandPeriod) return (false);
				}
			}
			else
			{
				//	Download this message in the foreground
				if (UIDDownloadMessage(tocH, (*tocH)->sums[sumNum].uidHash, true, false)!=noErr)
					return false;
			}
		}		

		// put the message in the mailbox
		for(n=100;n && !IMAPMessageDownloaded(tocH,sumNum);n--)
			//	call a reasonable number of times to get the job done
			//	there may be other messages that also need to be processed
			UpdateIMAPMailbox(tocH);	
		
		// did we get the message ok?
		if ((result=IMAPMessageDownloaded(tocH,sumNum)) && attachmentsToo)
		{
			// go fetch all the attachments for this message.  Wait for them.
			if (!FetchAllIMAPAttachments(tocH, sumNum, true))
				result = false;
		}			
		return result;
	}
	return true;
}
#endif

/************************************************************************
 * EnableMsgButtons - enable or disable message buttons
 ************************************************************************/
void EnableMsgButtons(MyWindowPtr win,Boolean enable)
{
	MControlsEnum cIndex;
	ControlHandle ctl;

	for (cIndex=mcWrite;cIndex<=mcTrash;cIndex++)
	{
		if (ctl=FindControlByRefCon(win,cIndex))
		{
			if (enable)	ActivateControl(ctl);
			else	DeactivateControl(ctl);
		}
	}
	
}

/************************************************************************
 * GetMessageLength - return length of message
 ************************************************************************/
uLong GetMessageLength(TOCHandle tocH,short sumNum)
{
	short	realSum;
	TOCHandle	realTOC;
	
	return (realTOC = GetRealTOC(tocH,sumNum,&realSum)) ? (*realTOC)->sums[realSum].length : 0;
	
}

/************************************************************************
 * RedateTS - redate a message
 ************************************************************************/
OSErr RedateTS(TOCHandle tocH,short sumNum)
{
	Str255 dateStr;
	uLong secs;
	uLong zoneSecs;
	OSErr err =  CacheMessage(tocH,sumNum);

	if (!err && (*tocH)->sums[sumNum].cache)
	{
		HNoPurge((*tocH)->sums[sumNum].cache);
		HandleHeadGetPStr((*tocH)->sums[sumNum].cache,HeaderStrn+DATE_HEAD,dateStr);
		if (!*dateStr) return fnfErr;
		secs = BeautifyDate(dateStr,&zoneSecs);
		TimeStamp(tocH,sumNum,secs,zoneSecs);
		HPurge((*tocH)->sums[sumNum].cache);
	}
	return err;
}

/************************************************************************
 * CurAddr - extract the current address from a window, if we have one
 ************************************************************************/
PStr CurAddr(MyWindowPtr win,PStr addr)
{
	if (win->curAddr) return win->curAddr(win,addr);
	else return CurAddrSel(win,addr);
}

/************************************************************************
 * CurAddrSel - extract the current address from the selection, if we have one
 ************************************************************************/
PStr CurAddrSel(MyWindowPtr win,PStr addr)
{
	if (win->pte && *PeteSelectedString(addr,win->pte) && *ShortAddr(addr,addr)) return addr;
	return nil;
}