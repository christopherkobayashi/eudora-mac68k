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

#ifndef MESSAGE_H
#define MESSAGE_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
typedef struct
{	
	long		id;
	Handle	properties;
} TransInfo,*TransInfoPtr,**TransInfoHandle;

typedef struct
{
	FSSpec spec;
	uLong cid;
	uLong absURL;
	uLong relURL;
} PartDesc, *PDPtr, **PDHandle;

/**********************************************************************
 * structure to describe message
 **********************************************************************/
typedef struct mstruct MessType, *MessPtr, **MessHandle;
struct mstruct
{
	TOCHandle tocH; 		/* the table of contents to which this message belongs */
	int sumNum; 				/* the summary number of this message's summary */
	MyWindowPtr win;		/* window I'm displayed in */
	Boolean dirty;			/* whether or not message is dirty */
	Boolean forceUnread;	/* don't set status to read on update */
	long weeded;					/* number of header bytes "weeded" out */
	ControlHandle sendButton;
	ControlHandle analControl;
	PETEHandle bodyPTE;
	PETEHandle subPTE;
	TOCHandle openedFromTocH;	/* toc from which we were requested (links) */
	long openedFromSerialNum;	// serial # of message from which we were requested (links)
	Accumulator extras;			/* extra header lines */
	MessHandle next;				/* next message in the list */
	long fieldDirty;	/* the dirty value when we entered this field */
	Boolean hasDelIcon;
	Boolean hasFetchIcon;
	Accumulator aSourceMID;
	uLong ezOpenSerialNum;
	TransInfoHandle hTranslators;
	short nTransIcons;
	short sound;
	FSSpecHandle etlFiles;
	FSSpecHandle hStationerySpec;	//	Nil if not editing stationery
	Boolean textFormatBarEnabled;
	Handle persGraphic;
	Boolean openToEnd;
	Accumulator newsGroupAcc;	// to hold newsgroup names
	Boolean redrawPersPopup;	// used if we try to draw the popup but fail
	Boolean dontActivate;		// skip the activation of a particular field on showing
														// a comp window
	long testSelStart, testSelEnd;	// stash the selection for personality popup
	Boolean alreadyLeaving;		// flag to indicate we're in CompLeaving, and it shouldn't be called again
};

/**********************************************************************
 * prototypes
 **********************************************************************/
PStr CurAddr(MyWindowPtr win,PStr addr);
PStr CurAddrSel(MyWindowPtr win,PStr addr);
OSErr RecordTransAttachments(FSSpecPtr spec);
OSErr SpoolIndAttachment(MessHandle messH,short i);
OSErr CacheMessage(TOCHandle tocH, short sumNum);
int ReadMessage(TOCHandle tocH,int sumNum,UPtr buffer);
int MoveMessage(TOCHandle tocH,int sumNum,FSSpecPtr toSpec,Boolean copy);
int MoveMessageLo(TOCHandle tocH,int sumNum,FSSpecPtr toSpec,Boolean copy,Boolean toTemp,Boolean holdOpen);
int MoveSelectedMessages(TOCHandle tocH,FSSpecPtr toSpec,Boolean copy);
int MoveSelectedMessagesLo(TOCHandle tocH,FSSpecPtr toSpec,Boolean copy,Boolean delete,Boolean undo,Boolean warnings);
void DeleteMessage(TOCHandle tocH, int sumNum,Boolean nuke);
void DeleteMessageLo(TOCHandle tocH, int sumNum,Boolean nuke);
int MessageError(void);
#define DoIterativeThingy(tocH,item,modifiers,toWhom) DoIterativeThingyLo(tocH,item,modifiers,toWhom,true)
void DoIterativeThingyLo(TOCHandle tocH,int item,long modifiers,TextAddrHandle toWhom,Boolean warnings);
OSErr MessPlainBytes(MessHandle messH,short whichTXE,short bytes);
MyWindowPtr DoReplyMessage(MyWindowPtr win, Boolean all, Boolean self, Boolean quote, Boolean doFcc, short toWhom,Boolean vis, Boolean station, Boolean caching);
MyWindowPtr DoRedistributeMessage(MyWindowPtr win,TextAddrHandle toWhom,Boolean turbo, Boolean andDelete,Boolean showIt);
MyWindowPtr DoForwardMessage(MyWindowPtr win,TextAddrHandle toWhom,Boolean showIt);
MyWindowPtr DoSalvageMessage(MyWindowPtr win,Boolean forXfer);
MyWindowPtr DoSalvageMessageLo(MyWindowPtr win,Boolean forXfer,Boolean forIMAP);
OSErr DoFordirectMessage(TOCHandle tocH,short sumNum,short flk,PStr addresses,Boolean bulk);
OSErr RedirectAnnotation(MessHandle messH);
OSErr SetMessText(MessHandle messH,short whichTXE,UPtr string,long size);
OSErr AppendMessText(MessHandle messH,short whichTXE,UPtr string,long size);
OSErr PrependMessText(MessHandle messH,short whichTXE,UPtr string,long size);
short BoxNextSelected(TOCHandle tocH,short afterNum);
void QuoteLines(PETEHandle pte,long from,long to,short pfid, long *qEnd);
void MakeMessTitle(UPtr title,TOCHandle tocH,int sumNum,Boolean useSummary);
#define GetAMessage(tocH, sumNum, wp, win, showIt) GetAMessageLo(tocH, sumNum, wp, win, showIt, nil)
MyWindowPtr GetAMessageLo(TOCHandle tocH,int sumNum,WindowPtr theWindow, MyWindowPtr win, Boolean showIt, Boolean *newWin);
MyWindowPtr ReopenMessage(MyWindowPtr win);
UPtr FindHeaderString(UPtr text,UPtr headerName,long *size,Boolean bodyToo);
int TextFindAndCopyHeader(UPtr body,long size,MessHandle newMH,UPtr fromHead,short toHead,short label);
void InstallMessText(MessHandle messH,short whichTXE,Handle text);
void SumInfoCpy(MSumPtr newSum, MSumPtr oldSum);
void ReplyDefaults(short modifiers,Boolean *all, Boolean *self, Boolean *quote);
uLong HashWithSeedLo(UPtr s, uLong n,uLong seed);
#define HashWithSeed(p,s)	HashWithSeedLo((p)+1,*(p),s)
#define Hash(x) HashWithSeed(x,1)
uLong MIDHash(UPtr text, long size);
void SetHashLo(TOCHandle tocH,short sumNum,uLong hash,Boolean soft);
void RehashLo(TOCHandle tocH,short sumNum,UHandle text,Boolean soft);
#define Rehash(tocH,sum,text) RehashLo(tocH,sum,text,false)
#define SetHash(tocH,sum,hash) SetHashLo(tocH,sum,hash,false)
OSErr EnsureFromHash(TOCHandle tocH,short sumNum);
OSErr TOCFindMessByMID(uLong mid,TOCHandle tocH,long *sumNum);
OSErr TOCFindMessByMsgID(uLong msgID,TOCHandle tocH,long *sumNum);
OSErr DoReplyClosed(TOCHandle tocH,short sumNum,Boolean all, Boolean self, Boolean quote, Boolean doFcc, short withWhich,Boolean vis,Boolean station);
void FixSourceStatus(TOCHandle tocH,short sumNum);
OSErr SaveTextAsMessage(Handle preText, Handle text,TOCHandle tocH,long *fromLen);
OSErr SavePtrAsMessage(UPtr preText, long preSize,UPtr text,long size,TOCHandle tocH,long *fromLen);
UHandle MessText(MessHandle messH);
UHandle MessVisibleText(MessHandle messH);
OSErr EnsureMID(TOCHandle tocH,short sumNum);
OSErr GenerateReceipt(MessHandle messH,short forWhat,short forWhatLocal,short actionId,short sentId);
OSErr MakeAttSubFolder(MessHandle messH,uLong uidHash,FSSpecPtr folder);
long FindAnAttachment(Handle text,long offset,FSSpecPtr spec,Boolean attach,uLong *cid, uLong *relURL, uLong *absURL);
void MovingAttachmentsLo(TOCHandle tocH,short sumNum,Boolean attach,Boolean wipe,Boolean nuke,Boolean IMAPStubsOnly,FSSpecPtr fromSpec,FSSpecPtr toSpec);
void Fix1MessServerArea(MyWindowPtr win);
OSErr CleanSpoolFolder(uLong age);
void EnableMsgButtons(MyWindowPtr win,Boolean enable);
uLong GetMessageLength(TOCHandle tocH,short sumNum);
OSErr EnsureMessNewline(MessHandle messH);
OSErr RedateTS(TOCHandle tocH,short sumNum);
#ifdef	IMAP
Boolean EnsureMsgDownloaded(TOCHandle tocH,short sumNum,Boolean attachments);
void MovingAttachments(TOCHandle tocH,short sumNum,Boolean attach,Boolean wipe,Boolean nuke,Boolean IMAPStubsOnly);
#endif
PStr GrabAttribution(short attriId,MyWindowPtr win,PStr attribution);
int AppendMessage(TOCHandle fromTocH,int fromN,TOCHandle toTocH,Boolean copy,Boolean toTemp,Boolean destHasAtt);
#define MAX_HEADER 64
#define WinPtr2MessH(aWindowPtr) 		((MessHandle)GetWindowPrivateData(aWindowPtr))
#define	Win2MessH(aMyWindowPtr) 		((MessHandle)GetMyWindowPrivateData(aMyWindowPtr))
#define SumOf(mH) (&(*(*mH)->tocH)->sums[(*mH)->sumNum])
#define BodyOf(mH) ((*mH)->txes[BODY])
#define MessFlagIsSet(mH,f)	(0!=(SumOf(mH)->flags&(f)))
#define SetMessFlag(mH,f)	do {SumOf(mH)->flags|=(f);TOCSetDirty((*mH)->tocH,true);}while(0)
#define MessOptIsSet(mH,f)	(0!=(SumOf(mH)->opts&f))
#define SetMessOpt(mH,f)	do {SumOf(mH)->opts|=f;TOCSetDirty((*mH)->tocH,true);}while(0)
#define ClearMessOpt(mH,f)	do {SumOf(mH)->opts&=~f;TOCSetDirty((*mH)->tocH,true);}while(0)
#define ClearMessFlag(mH,f)	do {SumOf(mH)->flags&=~f;TOCSetDirty((*mH)->tocH,true);}while(0)
#define OldWin2Body(win) BodyOf(Win2MessH(win))
#define HeaderName(num) (GetRString(scratch,HEADER_STRN+num),TrimWhite(scratch),scratch)
#define IsAddressHead(head) (head==TO_HEAD || head==BCC_HEAD || head==CC_HEAD)
#define MessIsRich(mH) (MessFlagIsSet((mH),FLAG_RICH)||MessOptIsSet((mH),OPT_HTML)||UseFlowInExcerpt&&MessOptIsSet((mH),OPT_FLOW))
#define TheBody ((*messH)->bodyPTE)
typedef enum
{
	mcSend = ' ce0',
	mcWrite,
	mcBlahBlah,
	mcFetch,
	mcGetGraphics,
	mcFixed,
	mcTrash,
	mcMesgErrors,
	mcMesgErrText,
	mcJunk,
	mcMesgErrSep,
	mcLimit
} MControlsEnum;

void Preview(TOCHandle tocH,short sumNum);

/*
 * functions for dealing with headers in composition windows
 */
typedef struct HeadSpec
{
	short index;	/* field index in AddressStrn or 0 (unspecial field) */
	long start;		/* first character of header */
	long value;		/* first character of content (follows whitespace) */
	long stop;		/* after last character of content */
	Str63 name;		/* field name, including ":", excluding space */
}	HeadSpec, *HSPtr, **HSHandle;

typedef struct
{
   Handle text;
   long  offset;
   HeadSpec hs;
   Boolean  outgoing;
   Boolean  attach;
} FindAttData, *FindAttPtr;

void InitAttachmentFinder(FindAttPtr pData,Handle text,Boolean attach,TOCHandle tocH,MSumPtr sum);
Boolean GetNextAttachment(FindAttPtr pData,FSSpecPtr spec);

#ifdef DEBUG
#define DBNoteUIDHash(old,new)	do{if (BUG15) Dprintf("\puidHash %x->%x;sc;g",(old),(new));}while(0)
#else
#define DBNoteUIDHash(old,new)
#endif
#endif
