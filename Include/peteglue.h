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

#ifndef PETEGLUE_H
#define PETEGLUE_H

/* Copyright (c) 1995 by Qualcomm, Inc. */

typedef enum {
	pgtHorizontalRule = 1,
	pgtIcon,
	pgtPICT,
	pgtImage,
	pgtResPict,
	pgtHTMLPending,
	pgtMovie,
	pgtNPlugin,
	pgtIndexPict,
	pgtPictHandle,
	pgtHeaderBodySep,
	pgtBodySigSep,
	pgtLimit
} PeteGraphicTypeEnum;

typedef struct {
	PETEGraphicInfo gi;
	short hi;
	short wi;
	Boolean groovy;
} HorizRuleGraphicInfo, *HRGraphicPtr, **HRGraphicHandle;

// (jp) Stuff to handle nickname hiliting/completion/expansion for PETE fields
typedef enum {
	nickNoScan = 0x00,	// No nickname scanning for this field
	nickHighlight = 0x01,	// Do nickname highlighting on this field
	nickComplete = 0x02,	// Do nickname auto-completion on this field
	nickExpand = 0x04,	// Do nickname expansion on this field
	nickCache = 0x08,	// Do nickname caching on this field
	nickSpaces = 0x10	// Allow spaces in nicknames in this field
} NickScanType;

typedef enum {
	noNickScanStatus = 0x00,
	typeAheadStatus = 0x01,	// A type ahead check is pending
	hiliteStatus = 0x02,	// A hiliting check is pending
	refreshStatus = 0x04	// A hiliting refresh is pending
} NickScanStatusType;

#define	kNickScanAllAliasFiles	(-1)

typedef Boolean(*NickFieldCheckProcPtr) (PETEHandle pte);
typedef OSErr(*NickFieldHiliteProcPtr) (PETEHandle pte, Boolean hilite);
typedef Boolean(*NickFieldRangeProcPtr) (PETEHandle pte, long *start,
					 long *end);

typedef struct {
	long typeAheadKeyTicks;
	long typeAheadSelStart;
	long typeAheadSelEnd;
	long typeAheadPrevSelEnd;	// Prior selection end (value of TypeAheadSelEnd prior to cycling via Up-Arrow and Down-Arrow.  Used by nickname hiliting.
	short typeAheadAliasIndex;	// alias index of the nickname used for the current nickname type-ahead selection
	short typeAheadNicknameIndex;	// nickname index of the nickname used for the current nickname type-ahead selection
	long hilitingKeyTicks;
	long hilitingStart;
	long hilitingEnd;
	short aliasIndex;	// The alias file to be used for searches; or -1 if all files are to be scanned
	Handle addresses;	// The addresses contained in the field pre-typing (for caching purposes)
	NickScanStatusType status;	// The status of nickname scanning for this field
	NickScanType scan;	// Flags to specify nickname scanning capabilities for this field
	NickFieldCheckProcPtr fieldCheck;	// Function that allows you to check to see if the field _really_ should perform nick operations
	NickFieldHiliteProcPtr fieldHilite;	// Function that allows you to perform any field specific hiliting or unhiliting
	NickFieldRangeProcPtr fieldRange;	// Function that calculates special content ranges for a field doing nickname expansion
} NickWatcherRec, *NickWatcherPtr, **NickWatcherHandle;

typedef struct PeteExtra {
	PETEHandle next;
	Boolean frame;
	Boolean isInactive;	// Is the field inactive independent of the window state?
	long urlScanned;
	long quoteScanned;
#ifdef WINTERTREE
	long spelled;
	Boolean spellReset;
#endif
//      struct TAESessionState taeSession;      // session record for scanner CK
	long taeScanned;	// spot we next need to scan
	long taeScannedSession;	// spot we last scanned at, to know if we need to reinit the session
	long taeScore;		// score we came up with
	Boolean taeDesired;	// do we like it?
	Boolean taeSpeak;	// do we want to speak this thing?
	Boolean emoDesired;	// do emoticons here?
	long emoScanned;	// our progress at emoticon scanning
	MyWindowPtr win;
	long headers;
	Str255 not;
	Boolean undoInsertion;
	Boolean numberField;
	long id;
	StackHandle partStack;
	Boolean infinitelyWide;
	NickWatcherRec nick;
	Boolean containsJunkMail;
	void (*dragPreProcess)(PETEHandle pte, DragTrackingMessage message,
			       DragReference drag);
	void (*dragPostProcess)(PETEHandle pte,
				DragTrackingMessage message,
				DragReference drag);
} PeteExtra, *PeteExtraPtr, **PeteExtraHandle;

#define	HasNickHiliting(pte)						((pte) ? (*PeteExtra(pte))->nick.scan & nickHighlight : false)
#define	HasNickCompletion(pte)					((pte) ? (*PeteExtra(pte))->nick.scan & nickComplete : false)
#define	HasNickExpansion(pte)						((pte) ? (*PeteExtra(pte))->nick.scan & nickExpand : false)
#define	HasNickCaching(pte)							((pte) ? (*PeteExtra(pte))->nick.scan & nickCache : false)
#define	HasNickSpaces(pte)							((pte) ? (*PeteExtra(pte))->nick.scan & nickSpaces : false)
#define	HasNickScanCapability(pte)			((pte) ? ((*PeteExtra(pte))->nick.scan ? true : false) : false)

#define	TypeAhead(pte)									((*PeteExtra(pte))->nick.status & typeAheadStatus)
#define	HilitePending(pte)							((*PeteExtra(pte))->nick.status & hiliteStatus)
#define	RefreshPending(pte)							((*PeteExtra(pte))->nick.status & refreshStatus)

#define	NeedTypeAhead(pte)							((*PeteExtra(pte))->nick.status |=  typeAheadStatus)
#define	ClearTypeAhead(pte)							((*PeteExtra(pte))->nick.status &= ~typeAheadStatus)

#define	NeedHilite(pte)									((*PeteExtra(pte))->nick.status |=  hiliteStatus)
#define	ClearHilite(pte)								((*PeteExtra(pte))->nick.status &= ~hiliteStatus)

#define	NeedRefresh(pte)								((*PeteExtra(pte))->nick.status |= 	refreshStatus)
#define	ClearRefresh(pte)								((*PeteExtra(pte))->nick.status &= ~refreshStatus)

#define	GetTypeAheadKeyTicks(pte)				((*PeteExtra(pte))->nick.typeAheadKeyTicks)
#define	GetTypeAheadSelStart(pte)				((*PeteExtra(pte))->nick.typeAheadSelStart)
#define	GetTypeAheadSelEnd(pte)					((*PeteExtra(pte))->nick.typeAheadSelEnd)
#define	GetTypeAheadPrevSelEnd(pte)			((*PeteExtra(pte))->nick.typeAheadPrevSelEnd)
#define	GetTypeAheadAliasIndex(pte)			((*PeteExtra(pte))->nick.typeAheadAliasIndex)
#define	GetTypeAheadNicknameIndex(pte)	((*PeteExtra(pte))->nick.typeAheadNicknameIndex)

#define	GetHilitingKeyTicks(pte)				((*PeteExtra(pte))->nick.hilitingKeyTicks)
#define	GetHilitingStart(pte)						((*PeteExtra(pte))->nick.hilitingStart)
#define	GetHilitingEnd(pte)							((*PeteExtra(pte))->nick.hilitingEnd)
#define	GetAliasFileToScan(pte)					((*PeteExtra(pte))->nick.aliasIndex)

#define	SetTypeAheadKeyTicks(pte,aLong)					do { (*PeteExtra(pte))->nick.typeAheadKeyTicks = aLong; } while (0)
#define	SetTypeAheadSelStart(pte,aLong)					do { (*PeteExtra(pte))->nick.typeAheadSelStart = aLong; } while (0)
#define	SetTypeAheadSelEnd(pte,aLong)						do { (*PeteExtra(pte))->nick.typeAheadSelEnd = aLong; } while (0)
#define	SetTypeAheadPrevSelEnd(pte,aLong)				do { (*PeteExtra(pte))->nick.typeAheadPrevSelEnd = aLong; } while (0)
#define	SetTypeAheadAliasIndex(pte,aShort)			do { (*PeteExtra(pte))->nick.typeAheadAliasIndex = aShort; } while (0)
#define	SetTypeAheadNicknameIndex(pte,aShort)		do { (*PeteExtra(pte))->nick.typeAheadNicknameIndex = aShort; } while (0)

#define	SetHilitingKeyTicks(pte,aLong) 					do { (*PeteExtra(pte))->nick.hilitingKeyTicks = aLong; } while (0)
#define	SetHilitingStart(pte,aLong) 						do { (*PeteExtra(pte))->nick.hilitingStart = aLong; } while (0)
#define	SetHilitingEnd(pte,aLong) 							do { (*PeteExtra(pte))->nick.hilitingEnd = aLong; } while (0)
#define	SetAliasFileToScan(pte,aShort)					do { (*PeteExtra(pte))->nick.aliasIndex = aShort; } while (0)

typedef struct {
	long first;
	long second;
	long right;
} PeteSaneMargin, *PSMPtr, **PSMHandle;

	 /* MJN *//* moved the macros for HLFTAB2FIX and FIX2HLFTAB from peteglue.c to here */
#define HLFTAB2FIX(x) ((width*(x))/2L)
#define FIX2HLFTAB(x) ((x)/(width/2L))

#ifdef USECMM
OSErr AppendEditItems(MyWindowPtr win, EventRecord * event,
		      MenuHandle contextMenu);
#endif
Rect *PeteRect(PETEHandle pte, Rect * r);
RgnHandle PeteRemoveFromRgn(RgnHandle rgn, PETEHandle pte);
void PeteConvertMarg(PETEHandle pte, long basePara, PSMPtr marg,
		     PETEParaInfoPtr pinfo);
OSErr PeteSetURLRescan(PETEHandle pte, long spot);
void PeteNickScan(PETEHandle pte);
OSErr PeteStyleAt(PETEHandle pte, long offset, PETEStyleEntryPtr theStyle);
PETEGraphicInfoHandle PeteGraphicHandle(PicHandle thePic, short resId);
void PeteLockStyle(PETEStyleListHandle style);
PETEHandle PetePrevious(PETEHandle head, PETEHandle pte);
PETEHandle PeteRemove(PETEHandle head, PETEHandle pte);
void PeteFocusPrevious(MyWindowPtr win);
void PeteFocusNext(MyWindowPtr win);
OSErr PeteAppendText(UPtr text, long size, PETEHandle pte);
OSErr PeteGetTextAndSelection(PETEHandle pte, Handle * textH,
			      long *selStart, long *selEnd);
long PeteBumpSelection(PETEHandle pte, long bumpAmount);
OSErr PeteInsertPlainParaAtEnd(PETEHandle pte);
Boolean PtInPETE(Point pt, PETEHandle pte);
Boolean PtInPETEView(Point pt, PETEHandle pte);
Boolean PeteSetDirty(PETEHandle pte);
OSErr PeteNoLabel(PETEHandle pte, long start, long stop, long labelMask);
#ifdef DEBUG
OSErr PeteScroll(PETEHandle pte, short horizontal, short vertical);
#else
#define PeteScroll(p,h,v)	PETEScroll(PETE,p,h,v)
#endif
#ifdef DEBUG
#define PeteCalcOn(p)	PETESetRecalcState(PETE,p,True)
#define PeteCalcOff(p)	do{if (!(BUG14) || !IsWindowVisible(GetMyWindowWindowPtr((*PeteExtra(p))->win))) PETESetRecalcState(PETE,p,False);}while(0)
#else
#define PeteCalcOn(p)	PETESetRecalcState(PETE,p,True)
#define PeteCalcOff(p)	PETESetRecalcState(PETE,p,False)
#endif
void PeteFocus(MyWindowPtr win, PETEHandle pte, Boolean focus);
#define ZapPETE(pte) while(pte){PeteDispose(pte);pte=nil;}
PStr PeteSelectedString(PStr string, PETEHandle pte);
#define PeteNext(pte) ((PETEHandle) (*(PeteExtraHandle)PETEGetRefCon(PETE,pte))->next)
PETEHandle PeteLink(PETEHandle head, PETEHandle pte);
void DefaultPII(MyWindowPtr win, Boolean ro, uLong flags,
		PETEDocInitInfoPtr pdi);
PStr PeteStringLo(PStr string, short size, PETEHandle pte);
#define PeteString(s,p) PeteStringLo(s,255,p)
#define PeteSString(s,p) PeteStringLo(s,sizeof(s),p)
uShort PeteCharAt(PETEHandle pte, long offset);
#define PeteCrAt(p,o) ((o)<=0 || PeteCharAt(p,o)=='\015')
OSErr PeteEdit(MyWindowPtr win, PETEHandle ph, PETEEditEnum what,
	       EventRecord * data);
OSErr PeteKey(MyWindowPtr win, PETEHandle pte, EventRecord * event);
OSErr PeteEditWFocus(MyWindowPtr win, PETEHandle ph, PETEEditEnum what,
		     EventRecord * data, Boolean * activated);
void PeteHotRect(PETEHandle pte, Boolean on);
OSErr PeteApplyResStyle(PETEHandle pte, long start, long stop,
			short styleId);
Boolean PeteCursorList(PETEHandle pte, Point mouse);
OSErr PeteSetTextPtr(PETEHandle pte, UPtr text, long len);
OSErr PeteSetText(PETEHandle pte, Handle text);
void PeteDidResize(PETEHandle pte, Rect * view);
OSErr PeteCreate(MyWindowPtr win, PETEHandle * ph, uLong flags,
		 PETEDocInitInfoPtr initInfo);
void PeteDispose(MyWindowPtr win, PETEHandle ph);
void PeteUpdate(PETEHandle ph);
void PeteFrame(PETEHandle pte, RGBColor * baseColor);
void CleanPII(PETEDocInitInfoPtr pii);
void PeteSelect(MyWindowPtr win, PETEHandle pte, long start, long stop);
long PeteFindString(PStr string, long start, PETEHandle pte);
void PeteCleanList(PETEHandle pte);
long PeteIsDirty(PETEHandle pte);
Boolean PeteIsDirtyList(PETEHandle pte);
OSErr PeteGetRawText(PETEHandle pte, UHandle * text);
OSErr PeteWrap(MyWindowPtr win, PETEHandle pte, Boolean unwrap);
OSErr PeteParaInset(PETEHandle pte, long fromOffset, long toOffset,
		    PSMPtr marg);
long PeteParaAt(PETEHandle pte, long offset);
OSErr PetePlain(PETEHandle pte, long start, long stop, long valid);
OSErr PetePlainParaAtLo(PETEHandle pte, long start, long stop, long valid);
OSErr PetePlainParaLo(PETEHandle pte, long index, long valid);
#define PetePlainParaAt(pte,start,stop) PetePlainParaAtLo(pte,start,stop,peAllParaValid&~peTabsValid)
#define PetePlainPara(pte,index) PetePlainParaLo(pte,index,peAllParaValid&~peTabsValid)
PETEParaInfoPtr DefaultPPI(PETEParaInfoPtr ppi);
OSErr PeteGraphicRect(Rect * r, PETEHandle pte,
		      PETEGraphicInfoHandle graphic, long offset);
long PeteLen(PETEHandle pte);
void PeteJustSayNoGraphic(PETEStyleEntryPtr theStyle);
#define PeteGetStyle(pte,offset,runLen,style) PeteGetStyleLo(pte,offset,runLen,false,style)
OSErr PeteGetStyleLo(PETEHandle pte, long offset, long *runLen,
		     Boolean allowGraphic, PETEStyleEntryPtr theStyle);
short PeteTextStyleDiff(PETETextStylePtr s1, PETETextStylePtr s2);
short PeteParaInfoDiff(PETEParaInfoPtr s1, PETEParaInfoPtr s2);
OSErr PeteInsert(PETEHandle pte, long offset, Handle text);
OSErr PeteInsertPtr(PETEHandle pte, long offset, Ptr text, long size);
#define PeteInsertStr(pte,offset,s) PeteInsertPtr(pte,offset,(s)+1,*(s))
OSErr PeteInsertChar(PETEHandle pte, long offset, unsigned char insertChar,
		     PETEStyleListHandle styles);
OSErr PetePasteQ(MyWindowPtr win, Boolean plain);
PETEUndoEnum PeteUndoType(PETEHandle pte);
OSErr PeteDrag(MyWindowPtr win, DragTrackingMessage message,
	       DragReference drag);
OSErr PeteParaRange(PETEHandle pte, long *startPtr, long *endPtr);
OSErr PeteParaConvert(PETEHandle pte, long start, long stop);
OSErr PeteEnsureRangeBreak(PETEHandle pte, long start, long stop);
OSErr Ascii2Margin(PStr source, PStr name, PSMPtr marg);
OSErr Res2PInfo(short source, PETEHandle pte, PStr name,
		PETEParaInfoPtr pinfop);
OSErr PeteDelete(PETEHandle pte, long start, long stop);
#define PeteEnsureBreak(pte,offset) PeteEnsureBreakLo(pte,offset,nil);
#define PeteEnsureCrAndBreak(pte,inOffset,newOffset) PeteEnsureCrAndBreakLo(pte,inOffset,newOffset,nil);
OSErr PeteEnsureBreakLo(PETEHandle pte, long offset, Boolean * did);
OSErr PeteEnsureCrAndBreakLo(PETEHandle pte, long inOffset,
			     long *newOffset, Boolean * did);
OSErr PeteEnsureTrailingEmpty(PETEHandle pte);
OSErr PeteTrimTrailingReturns(PETEHandle pte, Boolean leaveOne);
OSErr PeteCopy(PETEHandle fromPTE, PETEHandle toPTE, long fromFrom,
	       long fromTo, long toTo, long *endedAt, Boolean flatten);
OSErr PeteCopyNoLabel(PETEHandle fromPTE, PETEHandle toPTE, long fromFrom,
		      long fromTo, long toTo, long *endedAt,
		      Boolean flatten, long labelMask);
OSErr PeteCopyOld(PETEHandle fromPTE, PETEHandle toPTE, long fromFrom,
		  long fromTo, long toTo, long *endedAt, Boolean flatten);
OSErr PeteCopyPara(PETEHandle fromPTE, PETEHandle toPTE, long fromPara,
		   long fromFrom, long fromTo, long toTo, long *endedAt,
		   Boolean flatten);
OSErr PeteFontAndSize(PETEHandle pte, short fontID, short size);
OSErr PeteLock(PETEHandle pte, long start, long stop, short lock);
OSErr PeteExcerpt(PETEHandle pte, long fromOffset, long toOffset);
void PeteConvert2Marg(PETEParaInfoPtr pinfo, PSMPtr marg);
OSErr PeteSaveTEStyles(PETEHandle pte, FSSpecPtr spec);
OSErr PeteRemoveTEStyles(FSSpecPtr spec);
void PeteSetupTextColors(PETEHandle pte, Boolean justBlack);
void PeteSetRudeColor(void);
OSErr PeteSelectWord(MyWindowPtr win, PETEHandle pte, long offset);
Boolean MouseInPTE(PETEHandle pte);
OSErr PetePrepareUndo(PETEHandle pte, short undoWhat, long start,
		      long stop, long *uStart, long *uEnd);
OSErr PeteFinishUndo(PETEHandle pte, short undoWhat, long start,
		     long stop);
OSErr PeteKillUndo(PETEHandle pte);
void PeteSmallParas(PETEHandle pte);
OSErr PETESetTextStyle(PETEInst pi, PETEHandle ph, long start, long stop,
		       PETETextStylePtr textStyle, long validBits);
pascal OSErr PeteChanged(PETEHandle pte, long start, long stop);
pascal OSErr PeteCancelOnEvents(void);
pascal OSErr PeteBusy(void);
#define ONE_LINE_HI(win) (win->vPitch)
#define PeteExtra(pte)	((PeteExtraHandle)PETEGetRefCon(PETE,pte))
void PeteSelectAll(MyWindowPtr win, PETEHandle pte);
OSErr PeteLabel(PETEHandle pte, long start, long stop, short label,
		long labelMask);
OSErr PeteInsertRule(PETEHandle pte, long offset, short width,
		     short height, Boolean groovy, Boolean withParas,
		     Boolean undo);
OSErr PeteGetTextStyleScrap(PETEHandle pte, long from, long to,
			    Handle * text, PETEStyleListHandle * pslh,
			    PETEParaScrapHandle * pScrap);
OSErr ConvertExcerpt(PETEHandle pte, long start, long stop, long *newStart,
		     long *newStop);
OSErr LightQuote(PETEHandle pte, long startingFrom, long upTo,
		 long *newStart, long *newStop);
short PeteIsExcerptAt(PETEHandle pte, long offset);
Boolean PeteIsGraphicAt(PETEHandle pte, long offset);
Boolean PeteIsBullet(PETEHandle pte, long para);
Boolean PeteIsLabelled(PETEHandle pte, long start, long end, short label);
OSErr PeteSetExcerptLevelAt(PETEHandle pte, long offsetOfCR,
			    short quoteLevel);
void PeteRecalc(PETEHandle pte);
OSErr PeteWhompHiliteRegionBecausePeteWontFixIt(PETEHandle pte);
#define PeteText(p)	(PeteGetTextAndSelection(p,(UHandle*)&M_T1,nil,nil),(UHandle)M_T1)
void QuoteScan(void);
Boolean AmQuoteScanning(void);
void PeteQuoteScan(PETEHandle pte);
void PeteResetCurStyle(PETEHandle pte);
OSErr PeteHide(PETEHandle pte, long start, long stop);
OSErr PeteClearGraphic(PETEHandle pte, long start, long stop);
#define PeteIsJunk(pte) ((*PeteExtra(pte))->containsJunkMail)
#define PeteIsValid(pte) (!PETEDocCheck(PETE,(pte),true,true))
#ifdef DEBUG
OSErr PeteDumpLo(PETEHandle pte, short file, short line);
#define PeteDump(p)	PeteDumpLo((p),FILE_NUM,__LINE__)
#endif
#define ZapPeteStyleScrap(h)	do {if (h) PETEDisposeStyleScrap(PETE,(PETEStyleListHandle)(h));h=nil;} while(0)
#define ZapPeteParaScrap(h)	do {if (h) PETEDisposeParaScrap(PETE,(PETEParaScrapHandle)(h));h=nil;} while(0)
#define PeteSetString(s,pte) PeteSetTextPtr(pte,(s)+1,*(s))
#define pURLLabel	0x80
#define pQuoteLabel	0x40
#define pReplyLabel	0x20
#define pLinkLabel	0x10
#define pStationeryLabel 0x08
#ifdef WINTERTREE
#define pSpellLabel	0x04
#endif
#define	pNickHiliteLabel 0x02
#define pAnalMask				0x101
#define pTightAnalLabel	0x100
#define pLooseAnalLabel	0x001
#define LABEL_COPY_MASK (pURLLabel|pLinkLabel|pStationeryLabel)
#define pSigLabel	(0x200)

#define peAllParaValidButTabs (peAllParaValid&(~peTabsValid))

#define PETE_SCROLLY_DIFFERENCE 24
#define kPETEEndOfText	(0x7fffffff)
#define DEFAULT_COLOR(c)	((c).red = (c).green = (c).blue = 1)
#define IS_DEFAULT_COLOR(c)	((c).red==0 && (c).green==0 && (c).blue==0||(c).red==1 && (c).green==1 && (c).blue==1)

OSErr PeteExpandAliases(PETEHandle pte, BinAddrHandle * expandedAddresses,
			BinAddrHandle originalAddresses, short depth,
			Boolean wantComments);

typedef enum {
	peUndoMoreLink = peUndoLast + 1,
	peUndoMoreLimit
} MoreUndoEnum;
pascal OSErr HorizRuleGraphic(PETEHandle pte,
			      PETEGraphicInfoHandle graphic, long offset,
			      PETEGraphicMessage message, void *data);

void PeteActivateLo(PETEHandle pte, Boolean doc, Boolean scroll,
		    Boolean lock);
#define PeteActivate(p,d,s) PeteActivateLo(p,d,s,false)
#define PeteActiveLock(p) PeteActivateLo(p,false,false,true)
#endif
