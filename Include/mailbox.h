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

#ifndef MAILBOX_H
#define MAILBOX_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * the basic TOC types
 **********************************************************************/
typedef struct mstruct **MessHandle;	/* message.h */
typedef enum {
/*    1 */ UNREAD = 1,
/*    2 */ READ,
/*    3 */ REPLIED,
/*    4 */ REDIST,
/*    5 */ UNSENDABLE,
/*    6 */ SENDABLE,
/*    7 */ QUEUED,
/*    8 */ FORWARDED,
/*    9 */ SENT,
/*   10 */ UNSENT,
/*   11 */ TIMED,
/*   12 */ BUSY_SENDING,
/*   13 */ MESG_ERR,
/*   14 */ REBUILT,
	STATE_ENUM_LIMIT
} StateEnum;

typedef enum {
/*    0 */ OUT_NEW_MSG,
/*    1 */ OUT_FORWARD,
/*    2 */ OUT_REPLY,
/*    3 */ OUT_REDIRECT,
	OUT_TYPE_ENUM_LIMIT
} OutTypeEnum;

typedef struct BoxMapStruct {
	short vRef;
	long dirId;
} BoxMapType, *BoxMapPtr, **BoxMapHandle;
#define BoxMapCount (GetHandleSize_(BoxMap)/sizeof(BoxMapType))

#define MESG_ERR_TYPE 'MERR'

typedef struct mesgErrorStruct {
	uLong uidHash;		/* hash function of message-id */
	Str255 errorStr;	/* error message */
	int errorCode;		/* what's the problem */
} mesgError, *mesgErrorPtr, **mesgErrorHandle;

// This struct can be no larger than 8 bytes (sizeof(Rect))
typedef struct {
	long linkSerialNum;	// Serial # link to original message
	short virtualMBIdx;	/* FSSpec index for virtual mailboxes */
} VirtualMessData;

typedef struct {
	long offset;		/* byte offset in file */
	long length;		/* length of message, in bytes */
	int bodyOffset;		/* byte where the body begins, relative to offset */
	StateEnum state;	/* current state of the message */
	long spamScore:8;	// spam score
	uLong spamBecause:3;	// where the spam score came from
	long spare21:21;
	uLong arrivalSeconds;	// when the message arrived at this machine
	mesgErrorHandle mesgErrH;
	uLong fromHash;		// hash of the from address
	uLong spare[3];
	long serialNum;		// unique message serial number
	unsigned long seconds;	/* the value of seconds represented by the date field */
	unsigned long flags;	/* some binary values */
	union {
		Rect savedPos;	/* saved window position */
		VirtualMessData virtualMess;	// virtual message data, used if virtualTOC is set
	} u;			// reuse space not needed for virtual links
	Byte priority;		/* display as 1-5, keep as 1-200 */
	Byte origPriority;
	short tableId;		/* resid of xlate table to use (0 for none, -1 for default) */
	short score:4;		// for the text analysis engine
	short outType:4;	// for statistics: forward, reply, redirect
	short unused:8;		/* take it if you need it */
	short spareShort2;
	short sumRandBytes;	/* bytes for various uses */
	short origZone;		/* message's original timezone */
	uLong sigId;		/* fileid of signature; 0 for main sig, 1 for alternate */
	char from[48];		/* header items to keep handy */
	uLong popPersId;	/* personality id it came from */
	uLong persId;		/* the personality id */
	long msgIdHash;		/* hash of the message-id */
	short subjId;		/* subject id */
	short spareShort;
	char subj[60];		/* header items to keep handy */
	unsigned long opts;
	uLong uidHash;		/* hash of message id */
	Handle cache;		/* cache of message text */
	Boolean selected;	/* is it selected? */
	MessHandle messH;	/* message structure (and window) if any */
} MSumType, *MSumPtr;

typedef enum { kSearchMB } VirtualMBType;
typedef struct {
	VirtualMBType type;
	Handle data;		// data for virtual mailbox type
	FSSpecHandle specList;	// list of FSSpecs indexed by virtualMBIdx in MSumType
	short specListCount;	// # of specs in list
} VirtualMBData;

#define CURRENT_TOC_VERS 1
#define CURRENT_TOC_MINOR	9
#define TOC_MINOR_HAS_MOOD	3
#define TOC_MINOR_HAS_LONG_K	4
#define TOC_MINOR_HAS_PROFILE	5
#define TOC_MINOR_NO_DATE_STRING	6
#define TOC_MINOR_FIXED_OUTLOOKISMS	7
#define TOC_MINOR_NO_UNNECESSARY_UTF8 9	// used to be 8, but botched
#ifdef NEVER
#define dateTrunc sumRandBytes[0]
#define fromTrunc sumRandBytes[1]
#endif
#define kNeverHashed 0
#define kNoMessageId 0xffffffff
#define ValidHash(hash) ((hash)!=kNeverHashed && (hash)!=kNoMessageId)
typedef struct TOCType TOCType, *TOCPtr, **TOCHandle;
struct TOCType {
	short version;		/* TOC version number */
	short refN;		/* path ref number of open file */
	Str31 spareStr;
	uLong unused:14;
	uLong drawer:1;		// display drawer?
	uLong conConMultiScan:1;	// do we need to do a multiple selection scan for the content concentrator
	uLong updateBoxSizes:1;	// have any box size values changed?
	uLong hasFileView:1;	// has file view? (for ESP mailboxes)
	uLong fileView:1;	// is in file view mode?
	uLong analScanned:1;	// is the anal scan complete?
	uLong noInvalBox:1;	// suppress invalidation of toc window
	uLong searchFocus:1;	// search window with focus up in search area
	uLong beingWritten:4;
	uLong imapMessagesWaiting:1;	// set to true when there's a message waiting to be delivered
	uLong imapTOC:1;	// flag to mark toc as belonging to an IMAP mailbox
	uLong virtualTOC:1;	// virtual mailbox
	uLong listFocus:1;
	uLong userActive:1;
	uLong laurence:1;
	uLong oldKValues;
	long volumeFree;	/* free space on TOC volume */
	short needRedo;		/* min summary we need to look at */
	short maxValid;		/* max summary known to not need redraw */
	/* there are two bytes of padding here */
	union {
		FSSpec spec;	/* mailbox spec if not virtual mailbox */
		/* there are two bytes of padding here */
		VirtualMBData virtualMB;	// virtual mailbox data, used if virtualTOC is set
	} mailbox;
	long lastSort;		/* last sort key chosen */
	Byte sorts[12];		/* sort indicators */
	long pluginKey;		/* plug-in associated with this mailbox */
	long pluginValue;	/* stored value from plug-in */
	short previewHi;	// % of window to be used for preview
	short spareShort;
	uLong previewID;	// msgid of message currently being previewed
	PETEHandle previewPTE;	// pte for preview
	uLong lastSameTicks;	// ticks for same previewed message
	uLong mouseTicks;
	Point mouseSpot;
	uLong ezOpenSerialNum;	// message serial # to open next
	uLong lastSortTicks;	// when was the last time we sorted the mailbox?
	long nextSerialNum;	// next message serial number
	long profile[2];	// stored profile value
	Handle hFileView;	// file view data
	MailboxNodeHandle imapMBH;	// pointer to the mailbox node if this is an IMAp mailbox
	uLong internalUseOnly;
	long waste[3];		// available
	uLong singlePreviewProfileHash;
	uLong multiPreviewProfileHash;
	MyWindowPtr drawerWin;	// Drawer window ptr
	long unreadCount;	// # of unread messages
	uLong usedK;		/* number of Kbytes used by contents */
	uLong totalK;		/* number of Kbytes total */
	short unreadBase;	// what count was last time we updated unread
	short minorVersion;
	uLong doesExpire:1;	/* does the mailbox expire? */
	uLong expUnits:3;	/* days/weeks/months/years */
	uLong expInterval:28;	/* # of units */
	uLong writeDate;	/* date mailbox was written */
	long boxSize;		/* the size of the mailbox when last we wrote to it */
	Boolean temp;
	Boolean unread;		/* does mailbox contain unread mail? */
	Boolean resort;
	Boolean reallyDirty;
	uLong spareLong;
	short count;		/* number of messages described */
	Boolean durty;		/* whether or not this toc needs to be written out */
	TOCHandle next;		/* the next TOC in the chain */
	MyWindowPtr win;	/* the window for this toc */
	short which;		/* which type of mailbox (in, out, or regular) */
	Boolean building;	/* are we building it? */
	MSumType sums[1];	/* summaries of the messages contained in block */
};

#define TOCIsDirty(tocH) (*tocH)->durty
void TOCSetDirty(TOCHandle tocH, Boolean dirty);

#define FLAG_SKIPWARN				1	/* don't warn user about deleting this */
#define FLAG_OLD_SIG		(1<<1)	/* DO add signature to this message */
#define FLAG_BX_TEXT		(1<<2)	/* binhex any text attachments */
#define FLAG_WRAP_OUT 	(1<<3)	/* send this message right away */
#define FLAG_KEEP_COPY	(1<<4)	/* keep a copy of this message */
#define FLAG_TABS 			(1<<5)	/* put tabs in the body of this message */
#define FLAG_ATYPE_LO		(1<<6)	/* high bit of attachment type */
#define FLAG_ATYPE_HI		(1<<7)	/* low bit of attachment type
					   0 = BinHex
					   1 = AppleDouble
					   2 = Uuencode
					   3 = AppleSingle      */
#define FLAG_ENCBOD			 (1<<8)	/* writeable? */
#define FLAG_CAN_ENC		 (1<<9)	/* may we encode body? */
#define FLAG_RICH			  (1<<10)	/* message has richtext components */
#define FLAG_SHOW_ALL	  (1<<11)	/* show all headers & richtext stuff */
#define FLAG_RR				  (1<<12)	/* want return receipt */
#define FLAG_HAS_ATT	  (1<<13)	/* message has attachments */
#define FLAG_HUE1		  (1<<14)
#define FLAG_HUE2		  (1<<15)
#define FLAG_HUE3			(1<<16)
#define FLAG_HUE4			(1<<17)
#define FLAG_FIXED_WIDTH	(1<<18)
#define	FLAG_KNOWS_ME	(1<<19)	// message refers to one of my messages
#define FLAG_SIGN	(1<<21)	/* message marked for deletion at server */
#define FLAG_ENCRYPT	(1<<22)	/* message marked for deletion at server */
#define FLAG_UNFILTERED	(1<<23)	/* message hasn't been filtered yet */
#define FLAG_FIRST  		(1<<24)	/* first message of a split message */
#define FLAG_SUBSEQUENT		(1<<25)	/* subsequent message of a split message */
#define FLAG_SKIPPED		(1<<26)	/* this was a skipped message */
#define FLAG_OUT			(1<<27)	/* message was outgoing */
#define FLAG_ADDRERR		(1<<28)	/* addressing error in outgoing message */
#define FLAG_ZOOMED 		(1<<29)	/* was window zoomed when saved */
#define FLAG_ICON_BAR		(1<<30)	/* use icon bar in this message */
#define FLAG_UTF8			(1<<31)	/* summary is UTF-8 */

#define OPT_OPEN		(1<<0)
#define OPT_REPORT	(1<<1)
#define OPT_NOTIFY	(1<<2)
#define OPT_WIPE		(1<<3)
#define OPT_WRITE		(1<<4)
#define OPT_WILL_SEL		(1<<5)
#define OPT_ATT_DEL	(1<<6)
#define OPT_LOCKED	(1<<7)
#define OPT_EDITED	(1<<8)
#define OPT_WEIRD_REPLY	(1<<9)
#define OPT_BULK	(1<<10)
#define OPT_HTML	(1<<11)
#define OPT_RECEIPT	(1<<12)
#define OPT_COMP_TOOLBAR_VISIBLE (1<<13) /* MJN */	/* formatting toolbar visible */
#define OPT_AUTO_OPENED	(1<<14)	// filters opened the message
#define OPT_HAS_SPOOL (1<<15)	// message has spool folder
#define OPT_BLOAT (1<<16)	// send multipart/alternative
#define OPT_STRIP (1<<17)	// ignore styles when sending
#define	OPT_DELETED (1<<18)	/* this message is marked for deletion. */
#define	OPT_FLOW (1<<19)	/* this message has some format=flowed. */
#define OPT_FETCH_ATTACHMENTS (1<<20)	/* this message needs to have its attachments fetched */
#define OPT_JUST_EXCERPT (1<<21)	/* this message has only excerpt in it, no other styles */
#define OPT_ORPHAN_ATT (1<<22)	/* This message has attachments we should not clean up after */
#define OPT_CHARSET		(1<<23)	/* message has non-ascii charset */
#ifdef NOBODY_SPECIAL
#define OPT_JUSTSUB	(1<<24)	// Message has no non-white body
#endif				//NOBODY_SPECIAL
#define OPT_REDIRECTED (1<<25)	// This message was redirected
#define OPT_IMAP_SENT (1<<26)	/* This is a sent IMAP message */
#define OPT_SEND_REGINFO (1<<27)	// Send message with Reg Info headers
#define OPT_INLINE_SIG (1<<28)	// Message has inline sig
#define OPT_EMSR_DELETE_REQUESTED (1<<29)	// A translator has requested that this message be deleted
#define OPT_DELSP	(1<<30)	// DelSP parameter found on flowed
#define DEFAULT_TABLE (-1)
#define NO_TABLE 0
#define SORT_ASCEND 2
#define NO_SORT 0
#define SORT_DESCEND 1

#define SumOptIsSet(t,i,o)	(0!=((*t)->sums[i].opts & (o)))
#define SumFlagIsSet(t,i,f)	(0!=((*t)->sums[i].flags & (f)))
#define SetSumFlag(t,i,f)	do {(*t)->sums[i].flags|=(f);TOCSetDirty(t,true);}while(0)
#define SetSumOpt(t,i,o)	do {(*t)->sums[i].opts|=(o);TOCSetDirty(t,true);}while(0)
#define ClearSumOpt(t,i,o)	do {(*t)->sums[i].opts&=~(o);TOCSetDirty(t,true);}while(0)
#define ClearSumFlag(t,i,f)	do {(*t)->sums[i].flags&=~(f);TOCSetDirty(t,true);}while(0)
#define MAX_BOX_NAME	(PrefIsSet(PREF_NEW_TOC)?31:27)
#define MAX_MESSAGES_PER_MAILBOX 32000

#define IsBoxWindow(aWindowPtr)		(GetWindowKind(aWindowPtr)==MBOX_WIN || GetWindowKind(aWindowPtr)==CBOX_WIN)
#define IsMessWindow(aWindowPtr)	(GetWindowKind(aWindowPtr)==MESS_WIN || GetWindowKind(aWindowPtr)==COMP_WIN)

enum { kDontResort, kNoSlowResort, kResortWhenever, kResortNow };

typedef struct {
	short item;
	short vRef;
	long dirId;
} BoxCountElem, *BoxCountPtr, **BoxCountHandle;

/**********************************************************************
 * function prototypes
 **********************************************************************/
short FindSumByHash(TOCHandle tocH, uLong hash);
short FindSumBySerialNum(TOCHandle tocH, long serialNum);
PStr MailboxAlias(short which, PStr name);
PStr MailboxFile(short which, PStr name);
PStr MailboxMenuFile(short mid, short item, PStr name);
PStr MailboxSpecAlias(FSSpecPtr spec, PStr name);
#ifdef BATCH_DELIVERY_ON
Boolean IsDelivery(FSSpecPtr spec);
#endif
Boolean IsRoot(FSSpecPtr spec);
Boolean IsSpool(FSSpecPtr spec);
Boolean BadMailboxName(FSSpecPtr spec, Boolean folder);
Boolean BadMailboxNameChars(FSSpecPtr spec);
int BoxFClose(TOCHandle tocH, Boolean flush);
int BoxFOpen(TOCHandle tocH);
int BoxFOpenLo(TOCHandle tocH, short sumNum);
void CalcSumLengths(TOCHandle tocH, int sumNum);
OSErr DeleteSum(TOCHandle tocH, int sumNum);
int GetMailbox(FSSpecPtr spec, Boolean showIt);
Boolean GetNewMailbox(short vRef, long inDirId, FSSpecPtr spec,
		      Boolean * folder, Boolean * xfer);
void InvalSum(TOCHandle tocH, int sum);
OSErr Spec2Menu(FSSpecPtr spec, Boolean forXfer, short *menu, short *item);
OSErr TOCH2Menu(TOCHandle tocH, Boolean forXfer, short *menu, short *item);
int RemoveMailbox(FSSpecPtr spec, Boolean trashChain);
int RenameMailbox(FSSpecPtr spec, UPtr newName, Boolean folder);
Boolean SaveMessageSum(MSumPtr sum, TOCHandle * tocH);
void SetState(TOCHandle tocH, int sumNum, int state);
short FirstMsgSelected(TOCHandle tocH);
short LastMsgSelected(TOCHandle tocH);
long CountSelectedMessages(TOCHandle tocH);
long SizeSelectedMessages(TOCHandle tocH, Boolean countOpenOnes);
#ifdef	IMAP
long CountFlaggedMessages(TOCHandle tocH);
#endif
void CalcAllSumLengths(TOCHandle toc);
Boolean MessagePosition(Boolean save, MyWindowPtr win);
void TooLong(UPtr name);
short FindDirLevel(short vRef, long dirId);
void AddBoxCountMenu(short menuID, short item, short vRef, long dirId,
		     Boolean includeIMAP);
void Rebox(void);
Boolean GetTransferParams(short menu, short item, FSSpecPtr spec,
			  Boolean * xfer);
void TempWarning(UPtr filename);
OSErr BoxSpecByName(FSSpecPtr spec, PStr name);
void SelectMessage(TOCHandle tocH, short mNum);
OSErr AddBoxMap(short vRef, long dirId);
Boolean IsMailbox(FSSpecPtr spec);
#ifdef TWO
short GetMBDirName(short vRef, long dirId, UPtr name);
OSErr AskGraft(short vRef, long dirId, FSSpecPtr spec);
OSErr GraftMailbox(short vRef, long dirId, FSSpecPtr realSpec,
		   FSSpecPtr boxSpec, Boolean temporary);
short GetSumColor(TOCHandle tocH, short sumNum);
#define SumColor(sum) (((sum)->flags>>14)&0xf)
void SetSumColor(TOCHandle tocH, short sumNum, short color);
#endif
void Menu2VD(MenuHandle mh, short *vRef, long *dirId);
void MenuID2VD(short menuID, short *vRef, long *dirId);
Boolean IsMailboxChoice(short menu, short item);
void RedoTOCs(void);
short VD2MenuId(short vRef, long dirId);
void FixMenuUnread(MenuHandle mh, short item, Boolean unread);
void FixSpecUnread(FSSpecPtr spec, Boolean unread);
void AddBoxHigh(FSSpecPtr spec);
void RemoveBoxHigh(FSSpecPtr spec);
OSErr OpenFilterMessages(FSSpecPtr spec);
long TOCDelDup(TOCHandle tocH);
OSErr Box2Path(FSSpecPtr box, PStr path);
OSErr Path2Box(PStr path, FSSpecPtr box);
void PopupMailboxPath(MyWindowPtr win, TOCHandle tocH, short sumNum,
		      Point pt);

OSErr AddOutgoingMesgError(short sumNum, uLong uidHash, int errorCode,
			   short template, ...);
OSErr DeleteMesgError(TOCHandle tocH, short sum);
OSErr AddMesgError(TOCHandle tocH, short sum, PStr error, int errorCode);
OSErr FillMesgErrors(TOCHandle tocH);
FSSpec GetMailboxSpec(TOCHandle tocH, short sum);
UPtr GetMailboxName(TOCHandle tocH, short sum, UPtr name);
TOCHandle GetRealTOC(TOCHandle tocH, short sum, short *realSum);
void AddBoxCountItem(short item, short vRef, long dirId);
int OpenMailbox(FSSpecPtr spec, Boolean showIt, TOCHandle toc);
int OpenMailboxWithWin(MyWindowPtr win, FSSpecPtr spec, Boolean showIt,
		       TOCHandle toc);
void UpdateIMAPMailbox(TOCHandle toc);
Boolean IsMailboxSubmenu(short menu);
void BuildBoxCount(void);
MSumPtr FindRealSummary(TOCHandle tocH, long serialNum, short *realSum);
#ifdef	IMAP
void DeleteIMAPSum(TOCHandle tocH, int sumNum);
#endif
void InitMailboxWin(MyWindowPtr win, TOCHandle toc, Boolean showIt);
OSErr AppendXferSelection(PETEHandle pte, MenuHandle contextMenu);

#ifdef NOT_USED
void BuildMailboxTree(void);
#endif

#define SIG_STANDARD 0L
#define SIG_ALTERNATE	1L
#define SIG_NONE ((uLong)-1)

#define IsQueued(t,s)	IsQueuedState((*t)->sums[s].state)
#define IsQueuedState(s)	((s)==QUEUED || (s)==TIMED)

#define Label2Menu(l) (l+(l>7 ? 3 : (l>0 ? 2 : 1)))
#define Menu2Label(m) (m-(m>10 ? 3 : (m>1 ? 2 : 1)))
#endif
