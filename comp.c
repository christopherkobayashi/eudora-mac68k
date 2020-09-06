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

#include "comp.h"
#define FILE_NUM 7
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Outgoing

/************************************************************************
 * private function declarations
 ************************************************************************/
OSErr GetCompTexts(MessHandle messH,Boolean new);
void MakeCompTitle(UPtr string, TOCHandle tocH, MessHandle messH, int sumNum);
int WriteComp(MessHandle messH, short refN, long offset);
PStr GetMyHostname(PStr hostname);
OSErr CompStripHeaderReturns(MessHandle messH);
OSErr SuckDragAddresses(DragReference drag,Handle *addresses,Boolean leadingComma,Boolean trailingComma);
OSErr FindAndMarkSigSep(PETEHandle pte);
long FindSigSep(PETEHandle pte);
void SepStyle(PETEParaInfoPtr pip,PETETextStylePtr tsp,PETEGraphicInfoHandle *graphic,PeteGraphicTypeEnum pgt);
pascal OSErr CompGetDragContents(PETEHandle pte,UHandle *theText, PETEStyleListHandle *theStyles, PETEParaScrapHandle *theParas, DragReference drag, long dropLocation);
void CompBeautifyFrom(PStr name);
PStr CompCurAddr(MyWindowPtr win, PStr addr);

/**********************************************************************
 * OpenComp - open an outgoing message
 **********************************************************************/
MyWindowPtr OpenComp(TOCHandle tocH, int sumNum, WindowPtr winWP, MyWindowPtr win, Boolean showIt,Boolean new)
{
	Str255 title;
	MessHandle messH;
	void* grumble;
	Rect	rCntl;
	long sigID;
	short aquaAdjustment = 0;
	
	if (HaveOSX()) aquaAdjustment = 4;

	CycleBalls();
	if ((messH = (MessHandle)NuHandleClear(sizeof(MessType)))==nil)
		return(nil);
		
	win = GetNewMyWindow(MESSAGE_WIND,winWP,win,showIt||new?BehindModal:0,False,False,COMP_WIN);
	if (!win)
	{
		ZapHandle(messH);
		return(nil);
	}
	
	winWP = GetMyWindowWindowPtr (win);
	SetPort_(GetWindowPort(winWP));
	
	(*tocH)->sums[sumNum].messH = messH;
	MakeCompTitle(title,tocH,messH,sumNum);
	
	(*messH)->win = win;
	(*messH)->sumNum = sumNum;
	(*messH)->tocH = tocH;
	
	sigID = SigValidate(SumOf(messH)->sigId);
	SumOf(messH)->sigId = sigID;
	
	SetWindowPrivateData (winWP, (long)messH);
	win->vPitch = FontLead;
	win->hPitch = FontWidth;
	win->close = CompClose;
	win->curAddr = CompCurAddr;

	LL_Push(MessList,messH);

	/* MJN *//* formatting toolbar */
	(*tocH)->sums[sumNum].flags |= FLAG_ICON_BAR;
	if (PrefIsSet(PREF_COMP_TOOLBAR)) SetMessOpt(messH,OPT_COMP_TOOLBAR_VISIBLE);
	SetTopMargin(win,GetRLong(COMP_TOP_MARGIN));
	if (TextFormattingBarVisible(win)) win->topMargin += GetTxtFmtBarHeight();
	grumble = GetNewControl(SEND_NOW_CNTL,winWP);
	GetControlBounds((ControlHandle)grumble,&rCntl);
	MoveControl((ControlHandle)grumble,rCntl.left,(GetRLong(COMP_TOP_MARGIN)-ControlHi((ControlHandle)grumble))/2+2-aquaAdjustment);
	(*messH)->sendButton = grumble;
	if (gGoodAppearance) EmbedControl(grumble,win->topMarginCntl);
#ifdef THREADING_ON
	SetGreyControl((*messH)->sendButton,(*tocH)->sums[sumNum].state==SENT || (*tocH)->sums[sumNum].state==BUSY_SENDING);
#else
	SetGreyControl((*messH)->sendButton,(*tocH)->sums[sumNum].state==SENT);
#endif

	if (GetCompTexts(messH,new))
	{
		PeteCleanList(win->pteList);
		win->isDirty = False;
		CloseMyWindow(winWP);
		return(nil);
	}
	PeteSmallParas(TheBody);
	
	win->didResize = CompDidResize;
	win->click = CompClick;
	win->menu = CompMenu;
	win->key = CompKey;
	win->button = CompButton;
	win->position = MessagePosition;
	win->help = CompHelp;
	win->gonnaShow = CompGonnaShow;
	win->zoomSize = SumOf(messH)->state==SENT ? MessZoomSize : CompZoomSize;
	win->drag = CompDragHandler;
	win->idle = CompIdle; /* MJN *//* formatting toolbar */
	win->userSave = true;
	/* assumes win->idleInterval has already been initialized to zero */
	win->find = MessFind;
	SetWinMinSize(win,280,160);


	TextFont(0);
	TextSize(0);
	win->dontControl = True;
	if (IsColorWin(winWP)) win->label = GetSumColor((*messH)->tocH,(*messH)->sumNum);
	if (showIt)
		ShowMyWindow(winWP);
	InvalContent(win);
	SetWTitle_(winWP,title);
	AttachSelect(messH);
	win->isDirty = False;
	PeteCleanList(win->pteList);
	
	UpdateMyWindow(winWP);

	return(win);
}

/**********************************************************************
 * CompCurAddr - return the address most closely associated with this message
 **********************************************************************/
PStr CompCurAddr(MyWindowPtr win, PStr addr)
{
	BinAddrHandle addrList = NuHTempOK(0L);
	*addr = 0;
	
	if (win->hasSelection) return CurAddrSel(win,addr);
	
	if (!GatherCompAddresses(win,addrList))
	{
		PCopy(addr,*addrList);
		ShortAddr(addr,addr);
	}
	
	ZapHandle(addrList);
	return *addr ? addr : nil;
}

#pragma segment Outgoing
/**********************************************************************
 * BodyOffset - return the offset to the first character of the body
 *  of a message
 **********************************************************************/
long BodyOffset(Handle text)
{
	UPtr spot;
	long size = GetHandleSize(text);
	UPtr end = *text+size;
	
	for (spot=*text+2;spot<end;spot++)
		if (spot[-1]!='\015') spot++;
		else if (spot[-2]=='\015') break;
		
	return(spot-*text);
}

/************************************************************************
 * NewMessageId - create a new message id
 ************************************************************************/
PStr NewMessageId(PStr id)
{
	Str127 hostname;
	Str255 scratch;
	static short seq;
	Handle vers;
	struct {
		Byte four[4];
		long seconds;
		short ticks;
	} rawStuff;
	short oldRes;
	
	oldRes = CurResFile();
	UseResFile(AppResFile);
	vers = GetResource_('vers',1);
	if (vers)
	{
		rawStuff.four[0] = (*vers)[0];
		rawStuff.four[1] = (*vers)[1];
		rawStuff.four[2] = (*vers)[3];
		ReleaseResource_(vers);
	}
	UseResFile(oldRes);
	
	rawStuff.seconds = GMTDateTime();
	rawStuff.four[3] = seq++%256;
	rawStuff.ticks = TickCount()&0xffff;
	Bytes2Hex((void*)&rawStuff,10,scratch+1);
	*scratch = 20;
	MyLowerStr(scratch);
	
	GetMyHostname(hostname);
#ifdef ADWARE
	ComposeRString (id, (IsPayMode () ? PAY_MSGID_FMT : (IsAdwareMode () ? ADWARE_MSGID_FMT : FREEWARE_MSGID_FMT)), scratch, hostname);
#else
	ComposeRString (id, HasFeature (featureEudoraPro) ? MSGID_FMT : LIGHT_MSGID_FMT, scratch, hostname);
#endif
	return(id);
}

/************************************************************************
 * GetMyHostname - get the name of this macintosh
 ************************************************************************/
PStr GetMyHostname(PStr hostname)
{
	Str255 scratch;
	UPtr spot;
	
	if (*MyHostname) PCopy(hostname,MyHostname);
	else
	{
		GetPref(scratch,PREF_SMTP);
		if (*scratch && scratch[1]=='!')
		{
			spot = scratch+2;
			PToken(scratch,hostname,&spot,"!");
		}
		else if (*GetPref(scratch,PREF_LASTHOST)) PCopy(hostname,scratch);
		else
		{
			*hostname = 0;
			GetReturnAddr(scratch,false);
			if (spot=PIndex(scratch,'@'))
			{
				*hostname = *scratch - (spot-scratch) - 1;
				BMD(spot+1,hostname+1,*hostname);
			}
			if (!*hostname) GetRString(hostname,CTB_ME);
		}
	}
	return(hostname);
}

/**********************************************************************
 * GetCompTexts - get the fields of an under-composition message
 * First, we read ALL the message into a buffer.	Then, we grab the
 * header items, stuff them one by one into appropriate TERec's.  After
 * the headers are safely tucked away, we move the body to the
 * beginning of the buffer, and make a TERec out of that.
 * This routine assumes that the out box has been created in a very
 * particular format; to wit:
 *		Sendmail-style from line
 *		To:
 *		From:
 *		Subject:
 *		Cc:
 *		Date:
 *		<blank line>
 *		body of message
 * The header items must NOT contain newlines, and MUST be presented in
 * the proper order.
 * the "new" item means not to read the text, but to create it instead
 **********************************************************************/
OSErr GetCompTexts(MessHandle messH,Boolean new)
{
	MyWindowPtr messWin = (*messH)->win;
	WindowPtr	messWinWP = GetMyWindowWindowPtr (messWin);
	int sumNum = (*messH)->sumNum;
	TOCHandle tocH = (*messH)->tocH;
	UHandle buffer = nil;
	Accumulator extras;
	int which;
	OSErr err;
	Rect template;
	char *cp, *ep;
	UPtr stop;
	uLong uidHash;
	short len;
	PETEDocInitInfo pdi;
	Str63 headerName;
	long width;
	Boolean locked;
	short baseLock;
	long bo;
	Handle grumble;
	Boolean xDash;
	long baseWidth = RStringWidth(HeaderStrn+ATTACH_HEAD);
	
	Zero(pdi);
	Zero(extras);
	
	/*
	 * allocate space for the text
	 */
	if ((buffer=NuHTempBetter(new?1 K : GetMessageLength(tocH,sumNum)+1))==nil)
	{
		return(WarnUser(NO_MESS_BUF,MemError()));
	}
	
	/*
	 * read or create
	 */
	LDRef(buffer);
	if (!new)
	{
		/*
		 * read it
		 */
		if (err=ReadMessage(tocH,sumNum,*buffer))
		{
			ZapHandle(buffer);
			return(err);
		}
	}
	else
	{
		len = CreateMessageBody(*buffer,&uidHash);
		SetHandleBig(buffer,len+1);
		DBNoteUIDHash(SumOf(messH)->uidHash,uidHash);
		SumOf(messH)->uidHash = SumOf(messH)->msgIdHash = uidHash;
	}
	UL(buffer);
	
	/*
	 * now, set up the TERec's
	 */
	template = messWin->contR;			/* some size; it doesn't matter */
	InsetRect(&template,1,0);
	OffsetRect(&template,0,messWin->contR.bottom);
	DefaultPII(messWin,False,peVScroll|peGrowBox,&pdi);
	pdi.docWidth = MessWi(messWin)-PETE_SCROLLY_DIFFERENCE+9;
	err=PeteCreate(messWin,&messWin->pte,0,&pdi);
	if (err) goto failure;
	TheBody = messWin->pte;
	
#ifdef NEVER
	if (BUG14)
	{
		ShowMyWindow(messWinWP);
		UpdateMyWindow(messWinWP);
	}
	else
#endif
	PeteCalcOff(TheBody);

	/*
	 * set the drag callback
	 */
	PETESetCallback(PETE,TheBody,(void*)CompGetDragContents,peGetDragContents);

	/*
	 * put in the text...
	 */
	cp = LDRef(buffer);
	stop = cp + GetHandleSize(buffer)-1;
	*stop = '\015';	 /* a sentinel */
	while (*cp++ != '\015');		/* skip sendmail from line */

	/*
	 * the headers
	 */
	CycleBalls();
#ifdef THREADING_ON
	baseLock = (SumOf(messH)->state==SENT || SumOf(messH)->state==BUSY_SENDING) ? peModLock : 0;
#else
	baseLock = (SumOf(messH)->state==SENT) ? peModLock : 0;
#endif
	for (which=TO_HEAD; which < BODY_HEAD; which++)
	{
		locked = baseLock || which==ATTACH_HEAD || which==FROM_HEAD && !PrefIsSet(PREF_EDIT_FROM);

		GetRString(headerName,HeaderStrn+which);
		xDash = *headerName>2 && headerName[1]=='X' && headerName[2]=='-';
		width = StringWidth(headerName)+CharWidth(' ');
		if (xDash) width -= StringWidth("\pX-");
		pdi.inParaInfo.startMargin = baseWidth - width;
		pdi.inParaInfo.indent = -width;
	
		pdi.inParaInfo.paraFlags |= peNoParaPaste|peTextOnly|pePlainTextOnly;
		//if (which==ATTACH_HEAD) pdi.inParaInfo.paraFlags &= peTextOnly;
		
		// set the style
		pdi.inStyle.tsLock = peModLock|peSelectLock;
		//pdi.inStyle.tsFace = which;
		
		//insert the header name
		(*Pslh)->psGraphic = (*Pslh)->psStartChar = 0;
		(*Pslh)->psStyle.textStyle = pdi.inStyle;
		if (err=PETEInsertParaPtr(PETE,TheBody,kPETELastPara,&pdi.inParaInfo,headerName+1,*headerName,Pslh))
			goto failure;
		if (xDash)
		{
			long offset = PeteLen(TheBody)-*headerName;
			PeteHide(TheBody,offset,offset+2);
		}	
		
		//insert the space, with clickAfter
		if (!locked) pdi.inStyle.tsLock |= peClickAfterLock;
		(*Pslh)->psStyle.textStyle = pdi.inStyle;
		if (err=PETEInsertTextPtr(PETE,TheBody,kPETEEndOfText," ",1,Pslh)) goto failure;
		pdi.inStyle.tsLock &= ~peClickAfterLock;
		
		/*
		 * now, insert body of header
		 */
		if (cp >= stop) goto failure;
		for (ep=cp; *ep!='\015'; ep++);
		if (ep >= stop) goto failure;
		while (cp<ep && *cp++!=':');
		if (*cp == ' ') cp++;

		if (cp<ep)
		{
			if (!locked) pdi.inStyle.tsLock = baseLock;
			else pdi.inStyle.tsLock = peModLock;
			(*Pslh)->psStyle.textStyle = pdi.inStyle;
			if (err=PETEInsertTextPtr(PETE,TheBody,kPETEEndOfText,cp,ep-cp,Pslh)) goto failure;
		}

		// and trailing newline
		pdi.inStyle.tsLock = peModLock|peSelectLock;
		if (!locked) pdi.inStyle.tsLock |= peClickBeforeLock;
		(*Pslh)->psStyle.textStyle = pdi.inStyle;
		if (err=PETEInsertTextPtr(PETE,TheBody,kPETEEndOfText,"\015",1,Pslh)) goto failure;
		cp = ep+1;
#ifdef DEBUG
	if (BUG14) {InvalContent(messWin);UpdateMyWindow(messWinWP);}
#endif
	}
	(*PeteExtra(TheBody))->headers = which-1;
	
#ifdef ETL
	/*
	 * translators?
	 */
	GetRString(headerName,HeaderStrn+TRANSLATOR_HEAD);
	if (!CompareText(ep+1,headerName+1,*headerName,*headerName,nil))
	{
		cp = ++ep;
		while (ep[0]!='\015') ep++;
		cp += *headerName+1;
		AddTranslatorsFromPtr(messH,cp,ep-cp);
	}
#endif
		
	
	/*
	 * stash away the extra headers
	 */
	cp = ep+1;
	while (ep[0]!='\015' || ep[1]!='\015') ep++;
	if (ep>cp)
	{
		if (!AccuInit(&extras) && !AccuAddPtr(&extras,cp,ep-cp+1))
		{
			AccuTrim(&extras);
			(*messH)->extras = extras;
			Zero(extras);
		}
		else
		{
			err = MemError();
			goto failure;
		}
	}
	
	/*
	 * the body
	 */
	CycleBalls();

#ifdef DEBUG
	if (BUG14) {InvalContent(messWin);UpdateMyWindow(messWinWP);}
#endif
	
	/*
	 * separator
	 */
	SepStyle(&pdi.inParaInfo,&pdi.inStyle,&grumble,pgtHeaderBodySep);
	if (!baseLock) pdi.inStyle.tsLock |= peClickAfterLock;
	(*Pslh)->psStyle.textStyle = pdi.inStyle;
	(*Pslh)->psStartChar = 0;
	(*Pslh)->psStyle.graphicStyle.graphicInfo = (void*)grumble;
	(*Pslh)->psGraphic = 1;
	if (err=PETEInsertParaPtr(PETE,TheBody,kPETELastPara,&pdi.inParaInfo,"\015",1,Pslh)) goto failure;
	(*Pslh)->psGraphic = 0;

#ifdef DEBUG
	if (BUG14) {InvalContent(messWin);UpdateMyWindow(messWinWP);}
#endif

	/*
	 * rest of the body
	 */
	ep += 2;	/* skip newline and header/body separator */
	pdi.inParaInfo.startMargin = -1;
	pdi.inStyle.tsLock = baseLock;
	(*Pslh)->psStyle.textStyle = pdi.inStyle;
	bo = PeteLen(TheBody);
	if (ep<stop)
	{
		if (MessOptIsSet(messH,OPT_HTML))
		{
			long offset = ep-*buffer;
			long len = stop-*buffer;
			UL(buffer);
			if (err = InsertRich(buffer,offset,len,bo,false,TheBody,nil,false)) goto failure;
		}
		else
		{
			if (err=PETEInsertParaPtr(PETE,TheBody,kPETELastPara,&pdi.inParaInfo,ep,stop-ep,Pslh)) goto failure;
			if (MessFlagIsSet(messH,FLAG_RICH)) PeteRich(TheBody,bo,0,True);
		}
		if (!err && MessOptIsSet(messH,OPT_INLINE_SIG)) FindAndMarkSigSep(TheBody);
	}
	else if (err=PETEInsertParaPtr(PETE,TheBody,kPETELastPara,&pdi.inParaInfo,"",0,Pslh)) goto failure;
	
	if (buffer) ZapHandle(buffer);

#ifdef DEBUG
	if (BUG14) {InvalContent(messWin);UpdateMyWindow(messWinWP);}
#endif

	/*
	 * do precise sizing
	 */
	//MyWindowDidResize(messWin,&messWin->contR);
	CleanPII(&pdi);
	if (baseLock)
	{
		PeteLock(TheBody,0,PeteLen(TheBody),peModLock);
		PeteLock(TheBody,kPETEDefaultStyle,kPETEDefaultStyle,peModLock);
		PeteLock(TheBody,kPETECurrentStyle,kPETECurrentStyle,peModLock);
	}
  PETESetCallback(PETE,TheBody,(void*)PeteChanged,peDocChanged);
	return(noErr);
	
failure:
	AccuZap(extras);
	CleanPII(&pdi);
	if (buffer) ZapHandle(buffer);
	if (err != userCanceledErr)
		WarnUser(READ_MBOX,err);
	return(err);

}

/**********************************************************************
 * AddInlineSig - add the signature to a message
 **********************************************************************/
OSErr AddInlineSig(MessHandle messH)
{
	OSErr err = noErr;
	Str255 sep;
	MyWindowPtr openWin;
	MyWindowPtr win;
	FSSpec spec;
	HeadSpec hs;
	
	GetRString(sep,SIG_INTRO);
	
	// add sig if one is selected
	if (SumOf(messH)->sigId != SIG_NONE)
	{
		// if already present, mark it
		if (MessOptIsSet(messH,OPT_INLINE_SIG))
		{
			FindAndMarkSigSep(TheBody);
		}
		// add one
		else
		{
			PETEDocInitInfo pdi;
			Handle grumble;
			
			// Don't do any of this if the signature is empty-ish
			if (!(err=SigSpec(&spec,SumOf(messH)->sigId)))
			{
				if (!FSpDFSize(&spec))
				{
					SetSig((*messH)->tocH,(*messH)->sumNum,SIG_NONE);
					return noErr;
				}
			}

			UseFeature(featureInlineSig);
					
			// Make sure we have a blank line before the sig
			if (CompHeadFind(messH,0,&hs) && hs.stop==hs.value || PeteCharAt(TheBody,hs.stop-1)!='\015')
				PeteAppendText(Cr+1,*Cr,TheBody);
			
			// Add an extra blank line if there is an excerpt up there
			if (PeteIsExcerptAt(TheBody,PeteLen(TheBody)-2))
				PeteAppendText(Cr+1,*Cr,TheBody);
			
			// Lock the last return
			PeteLock(TheBody,PeteLen(TheBody)-1,PeteLen(TheBody),peModLock|peSelectLock|peClickBeforeLock);
			
			/*
			 * separator
			 */
			Zero(pdi);
			DefaultPII((*messH)->win,False,peVScroll|peGrowBox,&pdi);
			SepStyle(&pdi.inParaInfo,&pdi.inStyle,&grumble,pgtBodySigSep);
			(*Pslh)->psStyle.textStyle = pdi.inStyle;
			(*Pslh)->psStartChar = 0;
			(*Pslh)->psStyle.graphicStyle.graphicInfo = (void*)grumble;
			if ((*Pslh)->psStyle.graphicStyle.graphicInfo) (*Pslh)->psGraphic = 1;
			err = PETEInsertParaPtr(PETE,TheBody,kPETELastPara,&pdi.inParaInfo,sep+1,*sep,Pslh);
			CleanPII(&pdi);
			
			/*
			 * voodoo empty para at end of document
			 */
			PETEInsertParaPtr(PETE,TheBody,kPETELastPara,nil,nil,0,nil);
			PetePlainPara(TheBody,kPETELastPara);
			
			/*
			 * and now the body of the sig
			 */
			if (!err)
			if (!(err=SigSpec(&spec,SumOf(messH)->sigId)))
			{
				if (openWin=FindText(&spec))
					win = openWin;
				else
					win = OpenText(&spec,nil,nil,nil,false,nil,false,false);
				if (win)
				{
					long offset = FindSigSep(win->pte);
					if (offset<0) offset = 0;
					else offset += *sep;
					err = PeteCopy(win->pte,TheBody,offset,PeteLen(win->pte),PeteLen(TheBody),nil,false);
					if (!openWin) CloseMyWindow(GetMyWindowWindowPtr(win));
				}
			}
			
			if (!err) SetMessOpt(messH,OPT_INLINE_SIG);
			PeteKillUndo(TheBody);
		}
	}
	
	return err;
}

/**********************************************************************
 * SepStyle - setup styles for a separator bar
 **********************************************************************/
void SepStyle(PETEParaInfoPtr pip,PETETextStylePtr tsp,PETEGraphicInfoHandle *graphic,PeteGraphicTypeEnum pgt)
{
	tsp->tsLock = peModLock|peSelectLock;
	tsp->tsLock |= peClickAfterLock;
	pip->indent = 0;
	pip->startMargin = 0;
	pip->paraFlags = !PrefIsSet(PREF_SEND_ENRICHED_NEW) ?  0 : peTextOnly;
	*graphic = PeteGraphicHandle(nil,GREYLINE_PICT);
	if (*graphic) (**graphic)->privateType = pgt;
}


/**********************************************************************
 * RemoveInlineSig - delete the inline sig from a message
 **********************************************************************/
OSErr RemoveInlineSig(MessHandle messH)
{
	OSErr err = noErr;
	long offset;
	
	if ((offset = FindSigSep(TheBody))>=0)
	{
		PeteDelete(TheBody,offset,PeteLen(TheBody));
		ClearMessOpt(messH,OPT_INLINE_SIG);
		MessPlainBytes(messH,0,-1);	// clear lock from last para
		PeteKillUndo(TheBody);
	}
	return err;
}

/**********************************************************************
 * FindSigSep - find the signature separator
 **********************************************************************/
long FindSigSep(PETEHandle pte)
{
	long offset=0;
	long lastOffset = -1;
	Str255 sep;
	
	GetRString(sep,SIG_INTRO);
	if (*sep)
	{
		for (;;)
		{
			offset = PeteFindString(sep,offset,pte);
			//TODO Might not be a bad idea to make sure that it has a graphic over it
			if (offset<0) break;
			lastOffset = offset;
			offset += *sep;
		}
	}
	return lastOffset;
}

/**********************************************************************
 * FindAndMarkSigSep - find a plaintext sig separator and put the grey
 * sig line over it
 **********************************************************************/
OSErr FindAndMarkSigSep(PETEHandle pte)
{
	OSErr err = fnfErr;
	long offset = FindSigSep(pte);
	PETEDocInitInfo pdi;
	Str255 sep;
	Handle grumble;
	short para;
	PETEParaInfo info;

	// did we find the separator?
	if (offset >= 0)
	{
		// We must delete & (later) reinsert the text
		GetRString(sep,SIG_INTRO);
		PeteEnsureBreak(pte,offset);
		PeteDelete(pte,offset,offset+*sep);

		// Grab default styles
		Zero(pdi);
		DefaultPII((*PeteExtra(pte))->win,False,peVScroll|peGrowBox,&pdi);
		
		// Tweak them for the benefit of the separator
		SepStyle(&pdi.inParaInfo,&pdi.inStyle,&grumble,pgtBodySigSep);
		
		// Massage massage massage
		(*Pslh)->psStyle.textStyle = pdi.inStyle;
		(*Pslh)->psStartChar = 0;
		(*Pslh)->psStyle.graphicStyle.graphicInfo = (void*)grumble;
		(*Pslh)->psGraphic = 1;
		
		// now insert
		PeteCalcOn(pte);  //TODO - remove this when Pete fixes the editor
		err = PETEInsertParaPtr(PETE,pte,PeteParaAt(pte,offset),&pdi.inParaInfo,sep+1,*sep,Pslh);
		CleanPII(&pdi);
		
		// Now, lock the cr before the separator
		if (!err) err = PeteLock(pte,offset-1,offset,peModLock|peSelectLock|peClickBeforeLock);
		
		// And, after the separator, nobody can put in graphics
		para = PeteParaAt(pte,offset);
		info.tabHandle = nil;
		while (!PETEGetParaInfo(PETE,pte,para,&info))
		{
			info.paraFlags |= peTextOnly;
			PETESetParaInfo(PETE,pte,para,&info,peFlagsValid);
			para++;
		}
	}	
	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
AddTranslatorsFromPtr(MessHandle messH,UPtr text,long len)
{
	UPtr spot,end;
	long it;
	OSErr err;

	end = text+len;
	spot = text;
	for(end = text+len;spot<end;)
	{
		StringHandle	properties;
			
		properties = nil;
		Hex2Bytes(spot,8,(void*)&it);
		spot += 8;	//	Get past hex number

		if (*spot)
			properties = NewString(spot);

		spot += *spot+1;
		
		if (err=AddMessTranslator(messH,ETLIDToIndex(it),properties)) return(err);
	}
	return(noErr);
}

/**********************************************************************
 * CompGetDragContents - set the contents of a drop
 **********************************************************************/
pascal OSErr CompGetDragContents(PETEHandle pte,UHandle *theText, PETEStyleListHandle *theStyles, PETEParaScrapHandle *theParas, DragReference drag, long dropLocation)
{
	OSErr err = handlerNotFoundErr;
	HeadSpec hs;
	short para = PeteParaAt(pte,dropLocation);
	PETEStyleEntry pse;
	Accumulator text, styles;
	short item;
	short n;
	HFSFlavor **data;
	FSSpec spec;
	long junk;

	if (CompHeadFind(Win2MessH((*PeteExtra(pte))->win),PeteParaAt(pte,dropLocation)+1,&hs))
	{
		if (IsAddressHead(hs.index))
		{
			*theText = nil;
			err = SuckDragAddresses(drag,theText,hs.value!=dropLocation,hs.stop!=dropLocation);
		}
		else if (hs.index>=(*PeteExtra(pte))->headers && DragIsImage(drag))
		{
			if (!(err=AccuInit(&text))  && !(err=AccuInit(&styles)))
			{
				n = MyCountDragItems(drag);
				for (item=1;!err && item<=n;item++)	// loop through each item
					if (!(err=MyGetDragItemData(drag,item,flavorTypeHFS,(void*)&data)))
					{
						spec = (*data)->fileSpec;
						ZapHandle(data);
						Zero(pse);
						PETEGetStyle(PETE,pte,dropLocation,&junk,&pse);
						pse.psStyle.textStyle.tsLock = 0; // clear lock
						if (!(err=PeteFileGraphicStyle(pte,&spec,nil,&pse,fgDisplayInline)))
						{
							pse.psStartChar = text.offset;
							if (!(err=AccuAddChar(&text,' ')))
								err = AccuAddPtr(&styles,(void*)&pse,sizeof(pse));
						}
					}
				if (!err)
				{
					AccuTrim(&text);
					AccuTrim(&styles);
					*theStyles = (void*)styles.data;
					*theText = (void*)text.data;
					*theParas = nil;
					return(noErr);
				}
				AccuZap(text);
				AccuZap(styles);
			}
		}
	}
	return(err);
}

/**********************************************************************
 * SuckDragAddresses - get addresses out of a drag
 **********************************************************************/
OSErr SuckDragAddresses(DragReference drag,Handle *addresses,Boolean leadingComma, Boolean trailingComma)
{
	OSErr err = handlerNotFoundErr;
	short item;
	short n;
	Accumulator addrs;
	Handle data;
	Str15 comma;
	
	if (DragIsInteresting(drag,A822_FLAVOR/*,NICK_FLAVOR*/,nil))
	{
		/*
		 * grab the addresses
		 */
		Zero(addrs);
		GetRString(comma,COMMA_SPACE);
		if (!(err=AccuInit(&addrs)))
		{
			if (leadingComma) AccuAddStr(&addrs,comma);
			n = MyCountDragItems(drag);
			for (item=0;!err && item<n;item++)	// loop through each item
			{
				if (!(err = MyGetDragItemData(drag,item+1,A822_FLAVOR,&data)))
				{
					if (!(err = AccuAddHandle(&addrs,data)))
						err = AccuAddStr(&addrs,comma);
					ZapHandle(data);
				}
			}
			
			/*
			 * put them in the destination
			 */
			if (!err)
				if (addrs.offset>*comma)
				{
					// trim trailing comma
					if (!trailingComma) addrs.offset -= *comma;
					AccuTrim(&addrs);
					
					// do the deed
					*addresses = addrs.data;
					addrs.data = nil;
				}
				else err = fnfErr;	// none found
			ZapHandle(addrs.data);
		}
	}
	return(err);
}

/**********************************************************************
 * MakeCompTitle - title a composition window
 **********************************************************************/
void MakeCompTitle(UPtr string, TOCHandle tocH, MessHandle messH, int sumNum)
{
	Str255 scratch;
	Str15 commaSpace;
	FSSpecHandle	hSpec;

	if (hSpec = (*messH)->hStationerySpec)
	{
		PCopy(string,(*hSpec)->name);
	}
	else
	{
		GetRString(commaSpace,COMMA_SPACE);
		*string = 0;
		PCat(string,* (*tocH)->sums[sumNum].from ?
					(*tocH)->sums[sumNum].from : GetRString(scratch,NO_TO));
		if ((*tocH)->sums[sumNum].seconds)
		{
			PCat(string,commaSpace);
			PCat(string,ComputeLocalDate(&(*tocH)->sums[sumNum],scratch));
		}
		PCat(string,commaSpace);
		PCat(string,* (*tocH)->sums[sumNum].subj ?
				 (*tocH)->sums[sumNum].subj : GetRString(scratch,NO_SUBJECT));
	}
}

/************************************************************************
 * SaveComp - save a composition window
 ************************************************************************/
Boolean SaveComp(MyWindowPtr win)
{
	MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	long bytes;
	long offset;
	int err;
	HeadSpec hs;
	FSSpecHandle	hSpec;
	
	if (CompHeadFind(messH,ATTACH_HEAD,&hs) && hs.value<hs.stop)
		SetMessFlag(messH,FLAG_HAS_ATT);
	else
		ClearMessFlag(messH,FLAG_HAS_ATT);
		
	CompLeaving(messH,CompHeadCurrent(TheBody));
	bytes = CountCompBytes(messH);
	offset = FindTOCSpot(tocH,bytes);
	
	SetMessRich(messH);
		
	if (hSpec = (*messH)->hStationerySpec)
	{		
		FSSpec	newSpec;
		short	refN;

		if (!(err = AFSpOpenDF(LDRef(hSpec),&newSpec,fsRdWrPerm,&refN)))
		{
			long	eof;
			
			Stationery = True;
			err = SaveAsToOpenFile(refN,messH);
			Stationery = false;
			GetFPos(refN, &eof);
			SetEOF(refN, eof);	//	Set EOF in case file is shorter
			FSClose(refN);
		}
		if (err)
		{
			FileSystemError(WRITE_STA,(*hSpec)->name,err);
			UL(hSpec);
			return(False);		
		}
	}
	else
	{
		if ((err=BoxFOpen(tocH)) || (err = WriteComp(messH,(*tocH)->refN,offset)))
		{
			FileSystemError(WRITE_MBOX,LDRef(tocH)->mailbox.spec.name,err);
			UL(tocH);
			return(False);
		}
	}		
	FlushVol(nil,(*tocH)->mailbox.spec.vRefNum);

	bytes = (*tocH)->sums[(*messH)->sumNum].length;
	if ((*tocH)->refN) (void) SetEOF((*tocH)->refN,offset+bytes);
	ZapHandle((*tocH)->sums[(*messH)->sumNum].cache);
	UpdateSum(messH, offset, bytes);
	PeteCleanList(win->pte);
	win->isDirty = False;
	(*messH)->fieldDirty = 0;
	
	// let the preview pane know
	if ((*tocH)->previewID==SumOf(messH)->serialNum) Preview(tocH,-1);
		
#ifdef ETL
	if (MessFlagIsSet(messH,FLAG_ENCRYPT))
	{
		if (EMSR_NOW!=ETLCanTransOut(messH)) SetState((*messH)->tocH,(*messH)->sumNum,UNSENDABLE);
	}
#endif

	TOCSetDirty(tocH,true);
	(*tocH)->reallyDirty = True;
	return(True);
}
	
/************************************************************************
 * CountCompBytes - count up the bytes in a message under composition
 ************************************************************************/
long CountCompBytes(MessHandle messH)
{
	long bytes = 0;
	Str255 scratch;
	
	/*
	 * headers
	 */
	bytes = PeteLen(TheBody);
	
	/*
	 * extra headers
	 */
	bytes += (*messH)->extras.offset;
	
	SumToFrom(nil,scratch);
	bytes += strlen(scratch);
	return (bytes);
}

/************************************************************************
 * WriteComp - write a comp message to a particular spot in a particular
 * file.
 ************************************************************************/
int WriteComp(MessHandle messH, short refN, long offset)
{
	long count;
	UHandle text;
	int err;
	Str255 msgID;
	long bo;
	long spot;
	Accumulator enriched;
	Boolean html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
	//StackHandle stack = nil;

	if (SumOf(messH)->state!=TIMED)
		TimeStamp((*messH)->tocH,(*messH)->sumNum,GMTDateTime(),ZoneSecs());
	
	Zero(enriched);

	/*
	 * sendmail-style From line
	 */
	CycleBalls();
	SetFPos(refN, fsFromStart, offset);
	SumToFrom(nil,msgID);
	count = strlen(msgID);
	if (err=AWrite(refN,&count,msgID)) return(err);
	
	CompStripHeaderReturns(messH);
	
	PETEGetRawText(PETE,TheBody,&text);
	bo = count = BodyOffset(text);
	
	/*
	 * headers
	 */
	count--;
	if (err=AWrite(refN,&count,LDRef(text)))
	{
		UL(text);
		return(err);
	}
	
	/*
	 * translators
	 */
	if ((*messH)->hTranslators) WriteTranslators(refN,(*messH)->hTranslators);
	
	/*
	 * extra headers
	 */
	if ((*messH)->extras.offset)
	{
		count = (*messH)->extras.offset;
		err = AWrite(refN,&count,LDRef((*messH)->extras.data));
		UL((*messH)->extras.data);
		if (err) return(err);
		if ((*(*messH)->extras.data)[count-1] != '\015')
			if (err = FSWriteP(refN,Cr)) return(err);
	}

	/*
	 * body
	 */
	GetFPos(refN,&spot); // hold that thought
	TOCSetDirty((*messH)->tocH,true);
	CycleBalls();
	
	count = 1;
	AWrite(refN,&count,"\015");
	
	SetMessRich(messH);
 
	if (PrefIsSet(PREF_SEND_ENRICHED_NEW))
		if (html)
			SetMessOpt(messH,OPT_HTML);
		else
			SetMessFlag(messH,FLAG_RICH);
		
	if (MessOptIsSet(messH,OPT_HTML))
	{
		if (!(err=AccuInit(&enriched)))
		{
			//if (!(err=StackInit(sizeof(VolAndFID),&stack)))
			if (!(err=HTMLPreamble(&enriched,PCopy(msgID,SumOf(messH)->subj),0,True)))
			if (!(err=BuildHTML(&enriched,TheBody,nil,GetHandleSize(text),bo,nil,nil,1,CompGetMID(messH,msgID),nil,nil)))
			if (!(err=HTMLPostamble(&enriched,True)))
			{
				AccuTrim(&enriched);
				count = enriched.offset;
				err = AWrite(refN,&count,LDRef(enriched.data));
				UL(enriched.data);
			}
			else
			{
				WarnUser(CANT_SAVE_RICH,err);
				AccuZap(enriched);
				ClearMessOpt(messH,OPT_HTML);
			}
		}
	}
	else if (MessFlagIsSet(messH,FLAG_RICH))
	{
		if (!(err=AccuInit(&enriched)))
		{
			if (BuildEnriched(&enriched,TheBody,nil,GetHandleSize(text),bo,nil,True))
			{
				WarnUser(CANT_SAVE_RICH,0);
				ZapHandle(enriched.data);
				ClearMessFlag(messH,FLAG_RICH);
			}
			else
			{
				count = enriched.offset;
				err = AWrite(refN,&count,LDRef(enriched.data));
			}
		}
	}
			
	//ZapHandle(stack);
	if (enriched.data) ZapHandle(enriched.data);
	else
	{
		count = GetHandleSize(text)-bo;
		err = AWrite(refN,&count,LDRef(text)+bo);
	}
#ifdef DEBUG
	ASSERT(HGetState(text)&0x80);
#endif
	UL(text);
	if (err) return(err);
	err = EnsureNewline(refN);
	if (!err)
	{
		(*(*messH)->tocH)->sums[(*messH)->sumNum].bodyOffset = spot-offset;
		GetFPos(refN,&spot);
		SumOf(messH)->offset = offset;
		SumOf(messH)->length = spot-offset;
		TOCSetDirty((*messH)->tocH,true);
		(*(*messH)->tocH)->reallyDirty = True;
	}
	return(err);
}

/**********************************************************************
 * WriteTranslators - write out the translators
 **********************************************************************/
OSErr WriteTranslators(short refN,TransInfoHandle translators)
{
	Str255 scratch;
	OSErr err;
	short i;
	long	count;
	
	if (!(err=FSWriteP(refN,GetRString(scratch,HeaderStrn+TRANSLATOR_HEAD))))
	{
		scratch[0] = 1;
		scratch[1] = ' ';
		if (err = FSWriteP(refN,scratch))
			return err;

		for (i=HandleCount(translators);i--;)
		{
			//	Do translator ID
			ComposeString(scratch,"\p%x",(*translators)[i].id);
			if (err = FSWriteP(refN,scratch))
				return err;
			
			//	Do properties (if any)
			*scratch = 0;
			if ((*translators)[i].properties)
				PCopy(scratch,*(*translators)[i].properties);
			count = *scratch+1;
			if (err = AWrite(refN,&count,scratch))
				return err;				
		}
		scratch[0] = 1;
		scratch[1] = '\015';
		err = FSWriteP(refN,scratch);
	}
	return err;
}

/**********************************************************************
 * CompStripHeaderReturns - get the darn returns out of the headers
 **********************************************************************/
OSErr CompStripHeaderReturns(MessHandle messH)
{
	short h;
	HeadSpec hs;
	UPtr spot, end;
	long index;
	OSErr err=noErr;
	Handle text;
	Str15 mailtoBecauseNetscapeIsStupid;
	Str15 mightBeMailto;
	
	GetRString(mailtoBecauseNetscapeIsStupid,ProtocolStrn+proMail);
	
	for(h = (*PeteExtra(TheBody))->headers;h;h--)
	{
		if (CompHeadFind(messH,h,&hs))
		{
			PETEGetRawText(PETE,TheBody,&text);
			end = *text+hs.stop;
			for (spot=*text+hs.value;spot<end;spot++)
			{
				if (*spot=='\015')
				{
					/*
					 * replace return with space
					 */
					index = spot-*text;
					err = PETEInsertTextPtr(PETE,TheBody,index,nil,1,nil);
					if (!err) err = PETEInsertTextPtr(PETE,TheBody,index," ",1,nil);
					spot = *text+index;
					end = *text+hs.stop;
				}
				else if (*spot==':' && IsAddressHead(h) && spot-*text-hs.value >= *mailtoBecauseNetscapeIsStupid)
				{
					MakePStr(mightBeMailto,spot-*mailtoBecauseNetscapeIsStupid,*mailtoBecauseNetscapeIsStupid);
					if (StringSame(mightBeMailto,mailtoBecauseNetscapeIsStupid))
					{
						PeteDelete(TheBody,spot-*text-*mailtoBecauseNetscapeIsStupid,spot-*text+1);
						spot -= *mailtoBecauseNetscapeIsStupid+1;
						hs.stop -= *mailtoBecauseNetscapeIsStupid+1;
						end -= *mailtoBecauseNetscapeIsStupid+1;
					}
				}
			}
		}
	}
	return(err);
}
					

/************************************************************************
 * UpdateSum - stick values from comp message into sum
 ************************************************************************/
void UpdateSum(MessHandle messH, long offset, long length)
{
	TOCHandle tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;
	MSumType *sum;
	Str255 scratch;
	Boolean hasValidFrom;
	BinAddrHandle fromAddr = nil;

	sum = LDRef(tocH)->sums+sumNum;
	SuckHeaderText(messH,scratch,sizeof(scratch),FROM_HEAD);
	hasValidFrom = !SuckPtrAddresses(&fromAddr,scratch+1,*scratch,false,false,true,nil) && fromAddr && *fromAddr && **fromAddr;
	ZapHandle(fromAddr);
	SuckHeaderText(messH,scratch,sizeof(scratch),TO_HEAD);
	if (!*scratch)
		SuckHeaderText(messH,scratch,sizeof(scratch),BCC_HEAD);
	CompBeautifyFrom(scratch);
	PSCopy(sum->from,scratch); 
	SuckHeaderText(messH,sum->subj,sizeof(sum->subj),SUBJ_HEAD);
	sum->offset = offset;
	sum->length = length;
	if (*sum->from && hasValidFrom)
	{
		if (sum->state==UNSENDABLE) SetState(tocH,sumNum,SENDABLE);
	}
	else if (sum->state != UNSENDABLE)
	{
		SetState(tocH,sumNum,UNSENDABLE);
	}
	if (sum->seconds) PtrTimeStamp(sum,sum->seconds,ZoneSecs());
	sum->arrivalSeconds = GMTDateTime();
	UL(tocH); 
	
	/*
	 * retitle window
	 */
	if ((*messH)->win)
	{
		MakeCompTitle(scratch,tocH,messH,(*messH)->sumNum);
		SetWTitle_(GetMyWindowWindowPtr((*messH)->win),scratch);
	}
	
	/*
	 * finally, invalidate the line in the toc
	 */
#ifdef NEVER
	CalcSumLengths(tocH,(*messH)->sumNum);
#endif
	InvalSum(tocH,(*messH)->sumNum);
}

/**********************************************************************
 * CompBeautifyFrom - if a comp message is to a single address that
 * appears in the address book, turn the address into a realname
 **********************************************************************/
void CompBeautifyFrom(PStr name)
{
	OSErr err = fnfErr;
	BinAddrHandle rawAddrs = nil;
	Str255 localName;
	short which, index;
	
	// do we want to do this?
	if (*name && !PrefIsSet(PREF_NO_COMP_BEAUTIFY))
	// grab the addresses
	if (!(err=SuckPtrAddresses(&rawAddrs,name+1,*name,false,false,false,nil)))
	// must be only one
	if (!(err=(CountAddresses(rawAddrs,2)!=1)))
	{
		// is it a nickname already?
		if (FindNickExpansionFor(LDRef(rawAddrs),&which,&index))
			err = noErr;
		// No.  Look by address, then
		else
		{
			// is the name just an address?
			ShortAddr(localName,name);
			if (*localName==*name)
			{
				// is it in a nickname?
				err = NickExpFindNickFromAddr(LDRef(rawAddrs),&which,&index);
			}
			else
				err = fnfErr;
		}
	}
	
	if (!err)
	{
		// found it!  Grab the real name
		GetNicknamePhrasePStr(which,index,localName);
		if (*localName)
		{
			if (PrefIsSet(PREF_UNCOMMA)) Uncomma(localName);
			PCopy(name,localName);
		}
		else err = fnfErr;
	}
	
	// clean up our little turds...
	ZapHandle(rawAddrs);
	
	// if all the above failed, fix the old way
	if (err) BeautifyFrom(name);
}

/**********************************************************************
 * GetSigByName - get the hash of a signature from the sigs name
 **********************************************************************/
uLong GetSigByName(PStr name)
{
	Str31 lower;
	uLong sigId;
	
	if (EqualStrRes(name,SIGNATURE) || EqualStrRes(name,FILE_ALIAS_STANDARD)) sigId = SIG_STANDARD;
	else if (EqualStrRes(name,ALT_SIG) || EqualStrRes(name,FILE_ALIAS_ALTERNATE)) sigId = SIG_ALTERNATE;
	else if (!*name) sigId = SIG_NONE;
	else
	{
		PSCopy(lower,name);
		MyLowerStr(lower);
		sigId = Hash(lower);
	}
	return(sigId);
}

/************************************************************************
 * SuckHeaderText - get the text of a header for insertion into a summary
 ************************************************************************/
void SuckHeaderText(MessHandle messH,UPtr string,long size,short index)
{
	HeadSpec hs;
	UPtr cp;
	Str15 kiran;
	Str255 subj;
	UPtr spot;
	
	// find it
	if (CompHeadFind(messH,index,&hs))
	{
		if (index==SUBJ_HEAD && *GetRString(kiran,JUST_FOR_KIRAN))
		{
			CompHeadGetStr(messH,index,subj);
			if (spot=PPtrFindSub(kiran,subj+1,*subj)) hs.stop = hs.value+spot-subj-1;
		}
		CompHeadGetTextPtr(TheBody,&hs,0,string+1,size-2,&size);
		*string = size;
	}
	else *string = 0;
	string[*string+1] = 0;
	
	/*
	 * change newlines to spaces
	 */
	for (cp=string+1;*cp;cp++) if (*cp=='\015') *cp = ' ';
	
	TrimWhite(string);
}

/**********************************************************************
 * QueueSelectedMessages - queue all selected messages
 **********************************************************************/
int QueueSelectedMessages(TOCHandle tocH,short toState,uLong when)
{
	int sumNum;
	int err = 0;
	long zs = ZoneSecs();
#ifdef THREADING_ON
	Boolean busy = false;
#endif

	if (!when) when = GMTDateTime();
	
	for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
	{
		if ((*tocH)->sums[sumNum].selected && (*tocH)->sums[sumNum].state==UNSENDABLE)
		{
			err = CANT_QUEUE;
			goto done;
		}
	}
	
	for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
	{
		if ((*tocH)->sums[sumNum].selected)
		{
#ifdef THREADING_ON
			if ((*tocH)->sums[sumNum].state==BUSY_SENDING)
				busy = true;
			else
#endif
			{
	    	TimeStamp(tocH,sumNum,when,zs);
				if (IsQueuedState(toState))
				{
					if (*(*tocH)->sums[sumNum].from)
					{
						if ((*tocH)->sums[sumNum].state!=SENT)
						{
							MessHandle messH;
							
							SetState(tocH,sumNum,toState);
							if (messH = (*tocH)->sums[sumNum].messH)
								(*messH)->win->isDirty = true;
						}	
					}
					else
						err = 1;
				}
				else if ((*tocH)->sums[sumNum].state!=SENT)
				{
					SetState(tocH,sumNum,*(*tocH)->sums[sumNum].from? SENDABLE:UNSENDABLE);
				}
			}
		}
	}
done:
#ifdef THREADING_ON
	if (busy)
		WarnUser (SENDING_WARNING, 0);
#endif	
	if (err)
		WarnUser(CANT_QUEUE,err);
	SetSendQueue();
	return(err);
}

/************************************************************************
 * CreateMessageBody - put a blank message body into a buffer
 ************************************************************************/
short CreateMessageBody(UPtr buffer, uLong *hashPtr)
{
	Str255 s;
	UPtr ep;
	
	ep = buffer;
	
	/*
	 * from line
	 */
	strcpy(ep,"From "); ep+= 5;
	GetReturnAddr(s,False);
	strncpy(ep,s+1,*s);
	ep += *s;*ep++ = ' ';
	LocalDateTimeStr(ep);
	p2cstr(ep);
	ep += strlen(ep);
	
	/*
	 * headers
	 */
	GetRString(s,HEADER_STRN+TO_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';

	GetRString(s,HEADER_STRN+FROM_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ =' ';
	GetReturnAddr(s,True);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';

	GetRString(s,HEADER_STRN+SUBJ_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';
	GetRString(s,HEADER_STRN+CC_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';
	GetRString(s,HEADER_STRN+BCC_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';
	GetRString(s,HEADER_STRN+ATTACH_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';

	GetRString(s,HEADER_STRN+MSGID_HEAD);strncpy(ep,s+1,*s);ep+= *s;*ep++=' ';
	NewMessageId(s);strncpy(ep,s+1,*s);ep+= *s;*ep++ ='\015';
	*hashPtr = Hash(s);
		
	/*
	 * blank line for "body"
	 */
	strcpy(ep,"\015");
	return(ep-buffer+1);
}


/**********************************************************************
 * 
 **********************************************************************/
HSPtr CompHeadFind(MessHandle messH,short index,HSPtr hSpec)
{
	Str63 name;
	Handle text;
	long para = -1;
	PETEParaInfo info;
	long i;
	
	if ((*(*messH)->tocH)->which!=OUT)
		return(CompHeadFindStr(messH,GetRString(name,HeaderStrn+index),hSpec));
		
	if (index==0) i = (*PeteExtra(TheBody))->headers;
	else i = index-1;
	
	info.tabHandle = nil;
	
	if (!PETEGetParaInfo(PETE,TheBody,i,&info))
	{
		hSpec->index = index;
		hSpec->start = info.paraOffset;
		hSpec->stop = info.paraOffset+info.paraLength-1;
		
		if (!index)
		{
			hSpec->value = hSpec->start+1;
			hSpec->stop = PETEGetTextLen(PETE,TheBody);
			if (MessOptIsSet(messH,OPT_INLINE_SIG))
			{
				long sigOffset;
				if ((sigOffset = FindSigSep(TheBody))>=0)
					hSpec->stop = sigOffset-1;
			}
		}
		else
		{
			PETEGetRawText(PETE,TheBody,&text);
			if (!index) i=hSpec->start+1;
			else
			{
				for (i=hSpec->start;i<hSpec->stop;i++)
				{
					if ((*text)[i]==':')
					{
						for (++i;i<hSpec->stop;i++)
							if (!IsWhite((*text)[i])) break;
						break;
					}
				}
				hSpec->value = i;
			}
		}
		return(hSpec);
	}

	if (index==0)
	{
		PETEGetRawText(PETE,TheBody,&text);
		hSpec->value = BodyOffset(text);
		hSpec->stop = GetHandleSize(text);
		hSpec->start = hSpec->value - 1;
		*hSpec->name = 0;
		return(hSpec);
	}
	else
	{
		GetRString(name,HeaderStrn+index);
		if (CompHeadFindStr(messH,name,hSpec))
		{
			if (index==0)
			{
				hSpec->start = hSpec->stop+1;
				hSpec->value = hSpec->start+1;
				hSpec->stop = PeteLen(TheBody);
			}
			hSpec->index = index;
			return(hSpec);
		}
	}
	
	return(nil);
}

/**********************************************************************
 * CompHeadFindStr - find a header by name
 **********************************************************************/
HSPtr CompHeadFindStr(MessHandle messH,PStr name,HSPtr hSpec)
{
	Handle text;
	
	PETEGetRawText(PETE,TheBody,&text);
	return(HandleHeadFindStr(text,name,hSpec));
}

/************************************************************************
 * HandleHeadFindStr - find a header by name
 ************************************************************************/
HSPtr HandleHeadFindStr(UHandle text,PStr name,HSPtr hSpec)
{
	UPtr spot, nameEnd;
	long size;
	
	if (!text) return nil;

	size = GetHandleSize(text);
	
	if (!size) return nil;
	
	/*
	 * body?
	 */
	if (!*name)
	{
		hSpec->start = BodyOffset(text);
		hSpec->value = hSpec->start;
		hSpec->stop = size;
		return(hSpec);
	}
	
	/*
	 * search for header by name
	 */
	if (nameEnd=FindHeaderString(LDRef(text),name,&size,False))
	{
		/*
		 * found it.
		 */
		// copy name
		PSCopy(hSpec->name,name);
		
		// in case we found an indexed one
		hSpec->index = FindSTRNIndex(HeaderStrn,name);
		
		// back up to beginning of header
		for (spot=nameEnd-1; spot>*text && spot[-1]!='\015'; spot--);
		hSpec->start = spot-*text;
		
		// end of header
		hSpec->stop = hSpec->start+size + (nameEnd-spot);
		
		// first non-white character of header value
		hSpec->value = nameEnd-*text;
		
		// it would be bad to leave it locked...
		UL(text);
		
		// have fun
		return(hSpec);
	}
	
	// not found.  weep.
	return(nil);
}

/**********************************************************************
 * HandleHeadFindReply - find the header to use for reply
 **********************************************************************/
HSPtr HandleHeadFindReply(UHandle text,HSPtr hs)
{
	short i;
	Str255 scratch;
	
	// look for each header in turn
	for (i=1;*GetRString(scratch,ReplyStrn+i);i++)
		if (HandleHeadFindStr(text,scratch,hs)) return hs;
	
	return nil;
}

/************************************************************************
 * CompHeadAppendAddrStr - append an address string to a pete region 
 ************************************************************************/
OSErr CompHeadAppendAddrStr(MessHandle messH,HSPtr targetHS,PStr addr)
{
	OSErr err = noErr;
	
	if (!HSIsEmpty(targetHS))
		err = CompHeadAppendPtr(TheBody,targetHS,", ",2);
	
	if (!err)
		err = CompHeadAppendStr(TheBody,targetHS,addr);
	
	return err;
}

/************************************************************************
 * AddSelfAddrHashes - add the hashes of our own addresses to an adress hash
 *  accumulator 
 ************************************************************************/
OSErr AddSelfAddrHashes(AccuPtr addrAcc)
{
	Str255 dummy;
	Str255 temp;
	EAL_VARS_DECL;
	UHandle rawMyself=nil, cookedMyself=nil;
	UHandle myself=NuHTempBetter(0);
	long offset;
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
	if (!(err=SuckAddresses(&rawMyself,myself, false, false, false,nil)) &&
			!(err=ExpandAliasesLow(&cookedMyself,rawMyself, 0, false, "",EAL_VARS)))	// no autoqual
	{
		ZapHandle(rawMyself);
		ZapHandle(myself);
		
		for (offset=0;*MakeBinAddrStr(cookedMyself,offset,temp);offset=NextBinAddrOffset(cookedMyself,offset))
			AddAddressHashUniq(temp,addrAcc);
	}		
	
	ZapHandle(rawMyself);
	ZapHandle(cookedMyself);
	ZapHandle(myself);
	
	return err;
}

/************************************************************************
 * HandleHeadGetAddrs - get the addresses from a 
 ************************************************************************/
OSErr HandleHeadGetAddrs(UHandle text,HSPtr hs,BinAddrHandle *addrs)
{
	UHandle subText = nil;
	OSErr err = noErr;
	
	*addrs = nil;
	
	if (!(err=HandleHeadGetText(text,hs,&subText)))
	{
		err = SuckAddresses(addrs,subText,true,true,false,nil);
		ZapHandle(subText);
	}
	return err;
} 

/************************************************************************
 * HandleHeadCopyAddrs - copy the addresses from one header to another
 ************************************************************************/
OSErr HandleHeadCopyAddrs(UHandle text,HSPtr hs,MessHandle messH,short headerID,AccuPtr addrAcc,Boolean cacheThem)
{
	Str255 oneAddr;
	BinAddrHandle addrs;
	long offset;
	HeadSpec targetHS;
	OSErr err = noErr;
	
	if (!HandleHeadGetAddrs(text,hs,&addrs))
	{
		CompHeadFind(messH,headerID,&targetHS);
		for (offset=0;(*addrs)[offset];offset=NextBinAddrOffset(addrs,offset))
		{
			MakeBinAddrStr(addrs,offset,oneAddr);
			if (AddAddressHashUniq(oneAddr,addrAcc))
			{
				err = CompHeadAppendAddrStr(messH,&targetHS,oneAddr);
				if (cacheThem) CacheRecentNickname(oneAddr);
			}
		}
		ZapHandle(addrs);
	}
	else
		err = fnfErr;
		
	return err;
}

/**********************************************************************
 * HandleHeadGetPStr - Get a header as a string by index from a handle
 **********************************************************************/
PStr HandleHeadGetPStr(Handle text,short head,PStr val)
{
	HeadSpec hs;
	Str255 localStr;
	
	if (HandleHeadFindStr(text,GetRString(localStr,head),&hs))
	{
		// advance past colon, if need be
		if ((*text)[hs.value]==':' && hs.value<hs.stop) hs.value++;
		
		// make a string of it
		MakePStr(localStr,*text + hs.value,hs.stop-hs.value);
		
		// Trim off the space
		TrimAllWhite(localStr);
		
		// Come to poppa
		PCopy(val,localStr);
	}
	else *val = 0;
	
	return val;
}

/**********************************************************************
 * CompHeadActivate - activate a header
 **********************************************************************/
OSErr CompHeadActivate(PETEHandle pte,HSPtr hSpec)
{
	if (!hSpec) return(fnfErr);
	PeteSelect(nil,pte,hSpec->value,hSpec->stop);
	PeteSetDirty(pte);
	PeteScroll(pte,0,pseCenterSelection);
	return(noErr);
}

/**********************************************************************
 * CompHeadSet - set the value of a header
 **********************************************************************/
OSErr CompHeadSet(PETEHandle pte,HSPtr hSpec,Handle text)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	PeteDelete(pte,hSpec->value,hSpec->stop);
	err = PeteInsert(pte,hSpec->value,text);
	PeteSetDirty(pte);
	return(err);
}

/**********************************************************************
 * CompHeadAppend - append text to a header
 **********************************************************************/
OSErr CompHeadAppend(PETEHandle pte,HSPtr hSpec,Handle text)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	
	PeteSelect(nil,pte,hSpec->stop,hSpec->stop);
	err = PeteInsert(pte,-1,text);
	PeteSetDirty(pte);
	return(err);
}

/**********************************************************************
 * CompHeadSetPtr - set a header from a pointer
 **********************************************************************/
OSErr CompHeadSetPtr(PETEHandle pte,HSPtr hSpec,UPtr text,long size)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	PeteInsertPtr(pte,hSpec->value,nil,hSpec->stop-hSpec->value);
	err = PeteInsertPtr(pte,hSpec->value,text,size);
	if (hSpec->index==FROM_HEAD && !PrefIsSet(PREF_EDIT_FROM) || hSpec->index==ATTACH_HEAD)
		PeteLock(pte,hSpec->value,hSpec->value+size,peSelectLock);
	PeteSetDirty(pte);
	return(err);
}

/**********************************************************************
 * CompHeadSetIndexPtr - set a header from a pointer
 **********************************************************************/
OSErr CompHeadSetIndexPtr(PETEHandle pte,short index,UPtr text,long size)
{
	MessHandle messH = Win2MessH((*PeteExtra(pte))->win);
	HeadSpec hs;
	if (CompHeadFind(messH,index,&hs))
		return CompHeadSetPtr(pte,&hs,text,size);
	else
		return fnfErr;
}

/**********************************************************************
 * CompHeadAppendPtr - append text to a header from a pointer
 **********************************************************************/
OSErr CompHeadAppendPtr(PETEHandle pte,HSPtr hSpec,UPtr text,long size)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	err = PeteInsertPtr(pte,hSpec->stop,text,size);
	hSpec->stop += size;
	PeteSetDirty(pte);
	return(err);
}

/**********************************************************************
 * CompHeadPrependPtr - prepend text to a header from a pointer
 **********************************************************************/
OSErr CompHeadPrependPtr(PETEHandle pte,HSPtr hSpec,UPtr text,long size)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	PeteSelect(nil,pte,hSpec->value,hSpec->value);
	err = PeteInsertPtr(pte,-1,text,size);
	PeteSetDirty(pte);
	return(err);
}

/**********************************************************************
 * CompHeadGetText - get the text of a header and put it in a handle
 **********************************************************************/
OSErr CompHeadGetText(PETEHandle pte,HSPtr hSpec,Handle *text)
{
	Handle rawText;
	OSErr err = PETEGetRawText(PETE,pte,&rawText);
	
	if (err) return(err);
	return(HandleHeadGetText(rawText,hSpec,text));
}

/************************************************************************
 * HandleHeadGetIdText - get text of a header specified by id and put it in a handle
 ************************************************************************/
OSErr HandleHeadGetIdText(UHandle rawText,short id,Handle *text)
{
	HeadSpec hs;
	Str63 scratch;
	
	if (HandleHeadFindStr(rawText,GetRString(scratch,id),&hs))
		return HandleHeadGetText(rawText,&hs,text);
	else
		return fnfErr;
}

/************************************************************************
 * HandleHeadGetText - get text of a header and put it in a handle
 ************************************************************************/
OSErr HandleHeadGetText(UHandle rawText,HSPtr hSpec,Handle *text)
{
	OSErr err;
	
	if (!hSpec) return(fnfErr);
	*text = NuHTempOK(hSpec->stop-hSpec->value);
	err = MemError();
	
	if (rawText && !err)	BMD((*rawText)+hSpec->value,**text,hSpec->stop-hSpec->value);
	
	return(err);
}

/**********************************************************************
 * CompHeadGetTextPtr - get the text of a header and put it in a pointer
 **********************************************************************/
OSErr CompHeadGetTextPtr(PETEHandle pte,HSPtr hSpec,long offset,UPtr text,long textSize,long *bytes)
{
	Handle rawText;
	OSErr err = PETEGetRawText(PETE,pte,&rawText);
	long count = 0;
	
	if (!hSpec) return(fnfErr);
	if (!err)
	{
		count = MIN(textSize,hSpec->stop-hSpec->value);
		BMD((*rawText)+hSpec->value+offset,text,count);
	}
	if (bytes) *bytes = count;
	
	return(err);
}

/**********************************************************************
 * CompHeadGetStrLo - get a string from a field
 **********************************************************************/
OSErr CompHeadGetStrLo(MessHandle messH,short index,PStr string,short size)
{
	long maxSize = size-2;
	HeadSpec hs;
	OSErr err;
	
	*string = 0;
	if (!CompHeadFind(messH,index,&hs)) return(fnfErr);
	err = CompHeadGetTextPtr(TheBody,&hs,0,string+1,maxSize,&maxSize);
	if (!err) *string = maxSize;
	return(err);
}

/**********************************************************************
 * CompHeadCurrent - what header is the selection in?
 **********************************************************************/
short CompHeadCurrent(PETEHandle pte)
{
	long para;
	
	PETEGetParaIndex(PETE,pte,-1,&para);
	para++;
	if (para>(*PeteExtra(pte))->headers) para = 0;
	return(para);
}

/**********************************************************************
 * GetRHeaderAnywhere - get a header from main or weeded headers
 *  using resource index for header name and returning a handle
 **********************************************************************/
OSErr GetRHeaderAnywhere(MessHandle messH,short header,Handle *text)
{
	Str63 h;
	
	return(GetHeaderAnywhere(messH,GetRString(h,header),text));
}

/**********************************************************************
 * GetRHeaderAnywherePtr - get a header from main or weeded headers
 *  using resource index for header name and returning into a pointer
 **********************************************************************/
OSErr GetRHeaderAnywherePtr(MessHandle messH,short header,UPtr text,long textSize,long *bytes)
{
	Str63 h;
	
	return(GetHeaderAnywherePtr(messH,GetRString(h,header),text,textSize,bytes));
}

/**********************************************************************
 * GetHeaderAnywhere - get a header from main or weeded headers
 *  using string for header name and returning a handle
 **********************************************************************/
OSErr GetHeaderAnywhere(MessHandle messH,PStr header,Handle *text)
{
	HeadSpec hSpec;
	UPtr spot;
	long size;
	long offset;
	
	/*
	 * main message?
	 */
	if (CompHeadFindStr(messH,header,&hSpec))
		return(CompHeadGetText(TheBody,&hSpec,text));
	
	/*
	 * extras?
	 */
	size = (*messH)->extras.offset;
	spot = FindHeaderString(LDRef((*messH)->extras.data),header,&size,False);
	UL((*messH)->extras.data);
	
	if (spot)
	{
		offset = spot - *(*messH)->extras.data;
		
		if (*text = NuHTempBetter(size))
		{
			BMD(*(*messH)->extras.data+offset,**text,size);
			return(noErr);
		}
		else return(MemError());
	}
	else return(fnfErr);
}

/**********************************************************************
 * GetHeaderAnywherePtr - get a header from main or weeded headers
 *  using string for header name and returning into a pointer
 **********************************************************************/
OSErr GetHeaderAnywherePtr(MessHandle messH,PStr header,UPtr text,long textSize,long *bytes)
{
	HeadSpec hSpec;
	UPtr spot;
	long size;
	
	/*
	 * main message?
	 */
	if (CompHeadFindStr(messH,header,&hSpec))
		return(CompHeadGetTextPtr(TheBody,&hSpec,0,text,textSize,bytes));
	
	size = (*messH)->extras.offset;
	spot = FindHeaderString(LDRef((*messH)->extras.data),header,&size,False);
	UL((*messH)->extras.data);
	
	if (spot)
	{
		size = MIN(size,textSize);
		
		BMD(spot,text,size);
		if (bytes) *bytes = size;
		return(noErr);
	}
	else return(fnfErr);
}

/************************************************************************
 * CompGetMID - get the message-id from a comp message
 ************************************************************************/
PStr CompGetMID(MessHandle messH,PStr mid)
{
	long len;
	
	if (!GetRHeaderAnywherePtr(messH,InterestHeadStrn+hMessageId,mid,253,&len))
	{
		// ignore initial colon
		*mid = len-1;
		// ignore whitespace
		TrimWhite(mid);
		TrimInitialWhite(mid);
		// remove <>'s
		BMD(mid+2,mid+1,*mid-2);
		*mid -= 2;
	}
	else *mid = 0;
	return(mid);
}

/**********************************************************************
 * IsMe - is an address me?
 **********************************************************************/
Boolean IsMe(PStr address)
{
	Str31 me;
	Str255 shortAddr, scratch;
	BinAddrHandle rawMe=nil;
	BinAddrHandle cookedMe=nil;
	UPtr spot;
	Boolean result=False;
	
	GetRString(me,ME);
	ShortAddr(shortAddr,address);
	if (!SuckPtrAddresses(&rawMe,me+1,*me,False,False,False,nil))
		if (!ExpandAliases(&cookedMe,rawMe,0,False))
		{
			for (spot=LDRef(cookedMe);*spot;spot+=*spot+2)
			{
				ShortAddr(scratch,spot);
				if (StringSame(shortAddr,scratch)) {result=True; break;}
			}
		}
	ZapHandle(rawMe);
	ZapHandle(cookedMe);
	return(result);
}




//
//	Nickname expansion routines
//

Boolean IsHeaderNickField (PETEHandle pte)

{
	return (IsAddressHead (CompHeadCurrent (pte)));
}


//
//	NicknameHilitingUpdateCompHeader
//
//		Only called if we have a composition window

OSErr HiliteCompHeader (PETEHandle pte, Boolean hilite)

{
	MessHandle	messH;
	HeadSpec		headerFieldInfo;
	OSErr				theError;

	theError = noErr;
	
	if (messH = Win2MessH ((*PeteExtra(pte))->win)) {

		if (CompHeadFind (messH, TO_HEAD, &headerFieldInfo))
			if (hilite)
				theError = NicknameHilitingUpdateRange (pte, headerFieldInfo.value, headerFieldInfo.stop);
			else
				theError = PeteNoLabel (pte, headerFieldInfo.value, headerFieldInfo.stop, pNickHiliteLabel);

		if (CompHeadFind (messH, CC_HEAD, &headerFieldInfo))
			if (hilite)
				theError = NicknameHilitingUpdateRange (pte, headerFieldInfo.value, headerFieldInfo.stop);
			else
				theError = PeteNoLabel (pte, headerFieldInfo.value, headerFieldInfo.stop, pNickHiliteLabel);

		if (CompHeadFind (messH, BCC_HEAD, &headerFieldInfo))
			if (hilite)
				theError = NicknameHilitingUpdateRange (pte, headerFieldInfo.value, headerFieldInfo.stop);
			else
				theError = PeteNoLabel (pte, headerFieldInfo.value, headerFieldInfo.stop, pNickHiliteLabel);
	}
	return (theError);
}


Boolean GetCompNickFieldRange (PETEHandle pte, long *start, long *end)

{
	WindowPtr	pteWinWP;
	HeadSpec	headerFieldInfo;
	
	if (pte) {
		pteWinWP = GetMyWindowWindowPtr((*PeteExtra(pte))->win);
		if (GetWindowKind(pteWinWP) == COMP_WIN) {
			if (CompHeadFind (Win2MessH ((*PeteExtra(pte))->win), CompHeadCurrent (pte), &headerFieldInfo)) {
				*start = headerFieldInfo.value;
				*end	 = headerFieldInfo.stop;
				return (true);
			}
		}
	}
	return (false);
}

OSErr CompGatherRecipientAddresses (MessHandle messH, Boolean wantComments)

{
	Handle	addresses;
	OSErr		theError;
	
	theError = noErr;
	if (!PrefIsSet (PREF_NICK_CACHE)) {
		addresses = nil;
		if (messH)
			theError = GatherRecipientAddresses (messH, &addresses, wantComments);
		if (!theError)
			SetNickCacheAddresses (TheBody, addresses);
		else
			ZapHandle (addresses);
	}
	return (theError);
}

/**********************************************************************
 * CompAddExtraHeaderDangerDangerLookOutWillRobinson - 
 *  This is a super-hacky routine to append a header to extra headers.
 *  It doesn't do about a million things that it ought to do.  Go away,
 *  do not use this function.
 **********************************************************************/
OSErr CompAddExtraHeaderDangerDangerLookOutWillRobinson(MessHandle messH,PStr headName,Handle headContents)
{
	Accumulator extras;
	OSErr err;
	long oldOffset;
	
	extras = (*messH)->extras;
	oldOffset = extras.offset;
	
	if (!(err=AccuAddStr(&extras,headName)))
	if (!(err=AccuAddChar(&extras,' ')))
	if (!(err=AccuAddHandle(&extras,headContents)))
	{
		if ((*extras.data)[extras.offset-1]!='\015')
			err = AccuAddChar(&extras,'\015');
	}
	
	if (err) extras.offset = oldOffset;
	
	(*messH)->extras = extras;
	
	return err;
}

/**********************************************************************
 * IsAllLWSPMess - is a mesage all LWSP?
 **********************************************************************/
Boolean IsAllLWSPMess(MessHandle messH)
{
	HeadSpec hs;
	UHandle text;
	
	if (!CompHeadFind(messH,0,&hs)) return true;
	PeteGetTextAndSelection(TheBody,&text,nil,nil);
	
	return IsAllLWSPPtr(*text+hs.value,hs.stop-hs.value);
}

/**********************************************************************
 * PersonalizeSubject - add the user's identity to the subject of a message
 **********************************************************************/
void PersonalizeSubject(MessHandle messH)
{
	Str255 addr;
	UPtr spot;
	HeadSpec hs;
	
	PushPers(PERS_FORCE(MESS_TO_PERS(messH)));
	
	GetReturnAddr(addr,false);
	ShortAddr(addr,addr);
	if (spot=PIndex(addr,'@')) *addr = spot-addr-1;
	
	if (CompHeadFind(messH,SUBJ_HEAD,&hs))
	{
		CompHeadAppendPtr((*messH)->bodyPTE,&hs," ",1);
		CompHeadAppendPtr((*messH)->bodyPTE,&hs,addr+1,*addr);
	}
	
	PopPers();
}

/**********************************************************************
 * SerializeSubject - add a serial number to the subject of a message
 **********************************************************************/
void SerializeSubject(MessHandle messH)
{
	Str63 num;
	HeadSpec hs;
	
	PushPers(PERS_FORCE(MESS_TO_PERS(messH)));
	
	Long2Hex(num,GMTDateTime());
	
	if (CompHeadFind(messH,SUBJ_HEAD,&hs))
	{
		CompHeadAppendPtr((*messH)->bodyPTE,&hs,".",1);
		CompHeadAppendPtr((*messH)->bodyPTE,&hs,num+1,*num);
	}
	
	PopPers();
}

/**********************************************************************
 * CompSelectSecondUnquoted - put the insertion point after the first
 *  block of quoted text
 **********************************************************************/
void CompSelectSecondUnquoted(MessHandle messH)
{
	HeadSpec hs;
	
	if (CompHeadFind(messH,0,&hs))
	{
		hs.start++;
		
		while (hs.start<hs.stop && PeteIsExcerptAt((*messH)->bodyPTE,hs.start)) hs.start++;
		
		hs.start++;
		
		if (hs.start<hs.stop)
		{
			(*messH)->dontActivate = true;
			PeteSelect((*messH)->win,(*messH)->bodyPTE,hs.start,hs.start);
		}
	}
}
