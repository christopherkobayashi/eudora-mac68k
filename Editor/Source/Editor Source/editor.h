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

#ifndef PETE_EDITOR_H
#define PETE_EDITOR_H

#define PETEINLINE

#include <limits.h>
#include "pete.h"

/*	kScrollbarAdjust and kScrollbarWidth are used in calculating
	values for control positioning and sizing. */
#define kScrollbarWidth			16
#define kScrollbarAdjust		(kScrollbarWidth - 1)

#define kDefaultMargin			4

#define kSplitParaSize			(SHRT_MAX / 4)
#define kSplitParaSearch		(kSplitParaSize / 16)

#define kTooSmallForScroll		50

#define kNoIntellCPFlag			0x0010

#define	IsColorGrafPort(aCGrafPtr)	(((aCGrafPtr)->portVersion & 0xC000) == 0xC000)

//#define Exchange(a, b) ((a) ^= ((b) ^= ((a) ^= (b))))
//#define Exchange(a, b) ((a) = (a) ^ ((b) = (b) ^ ((a) = (a) ^ (b))))
//#define ExchangePtr(a, b) ((a) = (void *)((unsigned long)(a) ^ ((unsigned long)((b) = (void *)((unsigned long)(b) ^ ((unsigned long)((a) = (void *)((unsigned long)(a) ^ ((unsigned long)(b))))))))))
#define Exchange(a, b) (((a) = (a) ^ (b)), ((b) = (b) ^ (a)), ((a) = (a) ^ (b)))
#define ExchangePtr(a, b) (((a) = (void *)((unsigned long)(a) ^ (unsigned long)(b))), ((b) = (void *)((unsigned long)(b) ^ (unsigned long)(a))), ((a) = (void *)((unsigned long)(a) ^ (unsigned long)(b))))

#define DocOrGlobal(x, y) ((**x).y ? (**x).y : (**(**x).globals).y)

#define IntegralSize(theSize) (theSize + (theSize % 512) + 512)

#define myqd (*(QDGlobalsPtr)(*(long *)(LMGetCurrentA5()) - (sizeof(QDGlobals) - sizeof(GrafPtr))))

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#define G_MEMCANFAIL(x) do {if((**(x)).memCanFail != nil) *(**(x)).memCanFail = true;}while(false)
#define G_MEMCANTFAIL(x) do {if((**(x)).memCanFail != nil) *(**(x)).memCanFail = true;}while(false)
#define DI_MEMCANFAIL(x) G_MEMCANFAIL((**(x)).globals)
#define DI_MEMCANTFAIL(x) G_MEMCANTFAIL((**(x)).globals)

#if TARGET_RT_MAC_CFM
#define UPP_DECLARATION(procType,routineName,routineVar) \
RoutineDescriptor routineVar##TempRD = BUILD_ROUTINE_DESCRIPTOR(upp##procType##ProcInfo, routineName); \
RoutineDescriptor routineVar##RD; \
procType##UPP routineVar = (procType##UPP)&routineVar##RD; \
BlockMove(&routineVar##TempRD, &routineVar##RD, sizeof(RoutineDescriptor))
#else
#define UPP_DECLARATION(procType,routineName,routineVar) \
procType##UPP routineVar = routineName
#endif

typedef enum {
	kNullChar = 0,
	kHomeChar,
	kControlBChar,
	kEnterChar,
	kEndChar,
	kHelpChar,
	kControlFChar,
	kControlGChar,
	kBackspaceChar,
	kTabChar,
	kLineFeedChar,
	kPageUpChar,
	kPageDownChar,
	kCarriageReturnChar,
	kControlNChar,
	kControlOChar,
	kFKeyChar,
	kControlQChar,
	kControlRChar,
	kControlSChar,
	kControlTChar,
	kControlUChar,
	kControlVChar,
	kControlWChar,
	kControlXChar,
	kControlYChar,
	kControlZChar,
	kClearChar,
	kLeftArrowChar,
	kRightArrowChar,
	kUpArrowChar,
	kDownArrowChar,
	kSpaceChar,
	kForwardDeleteChar = 0x7F,
	kRightToLeftSpaceChar = 0xA0
} CharacterCodes;

typedef enum {
	kClearKey = 71
} KeyCodes;

typedef struct {
	short length;
	short numEntries;
	short offset[];
} WhiteSpaceTable, *WhiteSpaceTablePtr;

typedef struct {
	unsigned long hasVScroll : 1;
	unsigned long hasHScroll : 1;
	unsigned long hasGrowBox : 1;
	unsigned long useHLineWidth : 1;
	unsigned long offscreenBitMap : 1;
	unsigned long drawDebugSymbols : 1;
	unsigned long dragOnControlOnly : 1;
	unsigned long noStyledPaste : 1;
	unsigned long clearAllReturns : 1;
	unsigned long embedded : 1;
	unsigned long ignoreModLock : 1;
	unsigned long docCorrupt : 1;
	unsigned long ignoreSelectLock : 1;
	unsigned long progressLoopCalled : 1;
	unsigned long drawWithoutErase : 1;
	unsigned long drawInProgressLoop : 1;
	unsigned long clearOnDontUndo : 1;
	unsigned long typingUndoCurrent : 1;
	unsigned long dontSaveToUndo : 1;
	unsigned long recalcOff : 1;
	unsigned long scrollsVisible : 1;
	unsigned long scrollsDirty : 1;
	unsigned long selectionDirty : 1;
	unsigned long isScrolling : 1;
	unsigned long caretOn : 1;
	unsigned long doubleClick : 1;
	unsigned long tripleClick : 1;
	unsigned long isActive : 1;
	unsigned long anchorStart : 1;
	unsigned long anchorEnd : 1;
	unsigned long docHeightValid : 1;
	unsigned long dragActive : 1;
	unsigned long dragHasLeft : 1;
	unsigned long reposition : 1;
	unsigned long inGraphicCall : 1;
	unsigned long updating : 1;
} DocFlagsBlock;

typedef enum {
	hndlClear = 0x0001,
	hndlSys = 0x0002,
	hndlTemp = 0x0004,
	hndlEmpty = 0x0008
} HandleFlags;

typedef PETEGraphicInfo GraphicInfo, *GraphicInfoPtr, **GraphicInfoHandle;

typedef PETETextStyle MyTextStyle, *MyTextStylePtr;

typedef PETEGraphicStyle GraphicStyle, *GraphicStylePtr;

typedef PETEStyleInfo StyleInfo, *StyleInfoPtr;

typedef struct PETEPrivateGlobalsInfo PETEGlobals, *PETEGlobalsPtr, **PETEGlobalsHandle;

typedef struct PETEPrivateDocumentInfo DocumentInfo, **DocumentInfoHandle;

typedef struct PETEPrivateStyleEntry LongStyleRun, *LongStyleRunPtr, **LongStyleRunHandle;
typedef struct PETEPrivateLineStartEntry LSElement, *LSPtr, **LSTable;

typedef struct {
	short height;
	short ascent;
} LineMeasure;

struct PETEPrivateLineStartEntry {
	long lsStartChar;
	long lsScriptRunLen;
	LineMeasure lsMeasure;
	unsigned long lsULFlag : 1;
};

typedef struct {
	unsigned long stIsGraphic : 1;
	unsigned long stCount : 31;
	short ascent;
	short descent;
	short leading;
	StyleInfo stInfo;
} LongSTElement, *LongSTPtr;

typedef struct {
	DocumentInfoHandle docInfo;
	LongSTElement styleList[];
} **LongSTTable;

struct PETEPrivateStyleEntry {
	long srStyleLen;
	unsigned long srIsGraphic : 1;
	unsigned long srIsTab : 1;
	unsigned long srStyleIndex : 30;
};

typedef struct {
	CGrafPtr oldPort;
	GDHandle oldGDevice;
	MyTextStyle textInfo;
	RGBColor backColor;
	PenState pnState;
} PETEPortInfo, *PETEPortInfoPtr;

typedef struct {
	long offset;
	long length;
	GraphicInfoHandle graphicInfo;
	long filler;
} IdleGraphic, **IdleGraphicHandle;

typedef struct {
	LSTable lineStarts;
	long lineStartCacheIndex;
	long lineStartCacheHeight;
	long styleRunCacheIndex;
	long styleRunCacheOffset;
	long longLineIndex;
	short longLineWidth;
	short startMargin;
	short filler1;
	short endMargin;
	short filler2;
	short indent;
	short filler3;
	short direction;
	short justification;
	Byte signedLevel : 4;
	Byte quoteLevel : 4;
	Byte paraFlags;
	short tabCount;
	short tabStops[1];
	LongStyleRun theStyleRuns[];
} ParagraphInfo, *ParagraphInfoPtr, **ParagraphInfoHandle;

typedef struct {
	long paraHeight;
	unsigned long paraLSDirty : 1;
	unsigned long paraLength : 31;
	ParagraphInfoHandle paraInfo;
} ParagraphElement;

typedef struct {
	long baseStyleRun;
	short startLen;
	short endLen;
	LongSTTable theStyleTable;
	ParagraphInfoHandle paraInfo;
	long paraOffset;
} DirectionParam, *DirectionParamPtr;

typedef struct {
	short vPosition;
	long paraIndex;
	long lineIndex;
} LineInfo, *LineInfoPtr;

typedef struct {
	long offset;
	long paraIndex;
	long lineIndex;
	short hPosition;
	short lPosition;
	short rPosition;
	short vLineHeight;
	short vLineAscent;
	long vPosition;
	Boolean leadingEdge;
	Boolean lastLine;
	Boolean graphicHit;
	Boolean filler;
} SelData, *SelDataPtr;

typedef struct {
	short index;
	short len;
	short leftLen;
	short rightLen;
	short width;
	short leftWidth;
	short rightWidth;
	unsigned short isGraphic : 1;
	unsigned short isTab : 1;
	unsigned short isWhitespace : 1;
	unsigned short filler : 13;
	long offset;
	long paraOffset;
	unsigned long styleIndex;
	unsigned long filler1;
} OrderEntry, *OrderArray, **OrderHandle;

typedef struct {
	OrderHandle orderingHndl;
	long paraIndex;
	long lineIndex;
	short leftPosition;
	short visibleWidth;
	short leftWhiteWidth;
	short rightWhiteWidth;
	short filler;
} LayoutCache, *LayoutCachePtr;

typedef struct {
	long startParaIndex;
	long startStyleRunIndex;
	long endParaIndex;
	long endStyleRunIndex;
} SelectionLockCache, *SelectionLockCachePtr;

typedef struct {
	StringHandle theText;
	PETEStyleListHandle styleScrap;
	PETEParaScrapHandle paraScrap;
	long selStart;
	long selEnd;
	long deleteOffset;
	long startOffset;
	long endOffset;
	PETEUndoEnum undoType;
} UndoInfo;

typedef struct {
	RgnHandle autoScrollRgn;
	unsigned long timeEnteredAutoscroll;
	unsigned long lastCaret;
	SelData caretLocation;
	Boolean isHilited;
	Boolean caretOn;
	Boolean dragAcceptable;
	Boolean dragTextOnly;
} DragData, *DragDataPtr, **DragDataHandle;

typedef struct {
	PETESetDragContentsProcPtr setDragContents;
	PETEGetDragContentsProcPtr getDragContents;
	PETEProgressLoopProcPtr progressLoop;
	PETEDocHasChangedProcPtr docChanged;
} CallbackRoutines;

typedef struct {
	CGrafPtr pPort;
	PicHandle docPic;
	short oldWidth;
	short oldHPosition;
	long oldVPosition;
	SelData oldSelEnd;
	DocFlagsBlock oldDocFlags;
	Boolean printSelection;
} PrintingData, **PrintingDataHandle;

typedef struct {
	ScriptCode theScript;
	short theFont;
} LastScriptFontEntry;

typedef struct {
	long scriptCount;
	LastScriptFontEntry scriptFontList[];
} LastScriptFont, **LastScriptFontHandle;

struct PETEPrivateDocumentInfo {
	short docWidth;
	short hPosition;
	short longLineWidth;
	short shortLineHeight;
	long docHeight;
	long vPosition;
	short extraHeight;
	WindowRef docWindow;
	Rect docRect;
	Rect viewRect;
	ControlRef vScroll;
	ControlRef hScroll;
	ControlRef containerControl;
	RgnHandle hiliteRgn;
	RgnHandle graphicRgn;
	RgnHandle updateRgn;
	PETEGlobalsHandle globals;
	long refCon;
	short textRefNum;
	short styleRefNum;
	unsigned char charBuffer[4];
	MyTextStyle curTextStyle;
	SelData selStart;
	SelData selEnd;
	GraphicInfoHandle selectedGraphic;
	PrintingDataHandle printData;
	LastScriptFontHandle lastFont;
	PETEDefaultFontHandle defaultFonts;
	PETELabelStyleHandle labelStyles;
	RGBColor defaultColor;
	RGBColor defaultBGColor;
	UndoInfo undoData;
	DragDataHandle dragData;
	CallbackRoutines callbacks;
	long lastClickOffset;
	unsigned long lastClickTime;
	unsigned long lastCaret;
	DocFlagsBlock flags;
	unsigned long dirty;
	LayoutCache lineCache;
	SelectionLockCache lockCache;
	IdleGraphicHandle idleGraphics;
	long textOffset;
	long textLoaded;
	long textLen;
	StringHandle theText;
	LongSTTable theStyleTable;
	long paraCount;
	ParagraphElement paraArray[];
};

typedef struct {
	DocumentInfoHandle docInfo;
	GraphicInfoHandle graphic;
} GraphicEntry;

typedef struct {
	ScrapRef scrapID;	//	current scrap
	short filler;
	PETEStyleListHandle styleScrap;
} PrivateClip;

typedef struct {
	Handle itlHandle;
	long offset;
	short curScript;
	short parseScriptCode;
	CharByteTable table;
	Boolean doubleByte;
	Boolean hasDoubleByte;
} WhiteInfo, *WhiteInfoPtr, **WhiteInfoHandle;

typedef struct {
	unsigned long filler : 22;
	unsigned long anchoredSelection : 1;
	unsigned long hasControlManager : 1;
	unsigned long progressLoopCalled : 1;
	unsigned long useLiveScrolling : 1;
	unsigned long hasAppearanceManager : 1;
	unsigned long debugMode : 1;
	unsigned long noIntelligentEdit : 1;
	unsigned long hasMultiScript : 1;
	unsigned long hasDoubleByte : 1;
	unsigned long hasDragManager : 1;
} GlobalFlags;

struct PETEPrivateGlobalsInfo {
	GlobalFlags flags;
	short **fontSizes;
	unsigned long autoScrollTicks;
	PETEHasBeenCalledProcPtr hasBeenCalledCallback;
	PETEWordBreakProcPtr wordBreakCallback;
	PETEIntelligentCutProcPtr intelligentCutCallback;
	PETEIntelligentPasteProcPtr intelligentPasteCallback;
	PETEGraphicHandlerProcPtr *graphicProcs;
	GWorldPtr offscreenGWorld;
	GWorldPtr savedGWorld;
	RgnHandle scratchRgn;
	PrivateClip clip;
	WhiteInfoHandle whitespaceGlobals;
	Handle romanWordWrapTable;
	long romanWordWrapOffset;
	PETEDefaultFontHandle defaultFonts;
	short romanFixedFontWidth;
	short romanFixedFont;
	short romanFixedSize;
	Byte labelMask;
	PETELabelStyleHandle labelStyles;
	RGBColor defaultColor;
	RGBColor defaultBGColor;
	Boolean *memCanFail;
	DocumentInfoHandle docInfoArray[];
};

typedef struct {
	long lpLenRequest;
	long lpScriptRunLeft;
	long lpStyleRunIndex;
	long lpStyleRunLen;
} LoadParams, *LoadParamsPtr;

typedef struct {
	GraphicInfo gi;
	PicHandle pict;
	long counter;
} PictGraphicInfo, **PictGraphicInfoHandle;

#ifndef PEE /* For Steve */

#define Zero(x) do { char *t = (char *)&(x); long s = sizeof(x); while(--s >= 0L) *t++ = 0; } while(false);

pascal ComponentResult main(ComponentParameters *params, Handle globals);
pascal ComponentInstance MyGetInstance(ComponentInstance inst);
pascal ComponentResult MyCanDo(short selector);
OSErr InitializeGlobals(PETEGlobalsHandle *globals);
OSErr DisposeGlobals(PETEGlobalsHandle globals);
OSErr CreateNewDocument(PETEGlobalsHandle globals, DocumentInfoHandle *docInfo, PETEDocInitInfoPtr initInfo);
OSErr DisposeDocument(PETEGlobalsHandle globals, DocumentInfoHandle docInfo);
OSErr InsertParagraph(DocumentInfoHandle docInfo, long beforeIndex, PETEParaInfoPtr paraInfoPtr, Ptr theText, long textSize, long hOffset, PETEStyleListHandle styleList);
OSErr InsertSplitParagraphsPtr(DocumentInfoHandle docInfo, long beforeIndex, PETEParaInfoPtr paraInfoPtr, Ptr theText, long textSize);
OSErr InsertSplitParagraphs(DocumentInfoHandle docInfo, long beforeIndex, PETEParaInfoPtr paraInfoPtr, Handle theText);
long ParagraphOffset(DocumentInfoHandle docInfo, long paraIndex);
long ParagraphIndex(DocumentInfoHandle docInfo, long textOffset);
OSErr StyleListToStyles(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, Ptr theText, long textSize, long hOffset, long textOffset, Handle listHandle, long listOffset, MyTextStylePtr textStyle, Boolean printing);
void CompressStyleRuns(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex, long runsAdded);
long FindNextTab(Ptr theText, long textLen, long lastTab, long *tabCount);
void OffsetLineStarts(LSTable lineStarts, long startOffset, long textOffset);
void SubtractFromLineStarts(ParagraphInfoHandle paraInfo, LSTable lineStarts, long startOffset, long textOffset, Boolean removeLines);
void RecalcScriptRunLengths(DocumentInfoHandle docInfo, long paraNum, long startOffset);
OSErr DeleteParagraph(DocumentInfoHandle docInfo, long paraIndex);
OSErr DeleteParaStyleRuns(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex, long runsToDelete);
OSErr DeleteStyleRun(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex);
OSErr UpdateDocument(DocumentInfoHandle docInfo);
OSErr DrawDocument(DocumentInfoHandle docInfo, RgnHandle clipRgn, Boolean preserveLine);
OSErr DrawDocumentWithPicture(DocumentInfoHandle docInfo);
OSErr FindFirstVisibleLine(DocumentInfoHandle docInfo, LineInfoPtr lineInfo, short drawTop);
OSErr ReflowParagraph(DocumentInfoHandle docInfo, long paraNum, long startChar, long endChar);
OSErr GetLineBreak(DocumentInfoHandle docInfo, LSPtr lineBreak, long paraIndex, Boolean firstLine);
OSErr GetTabbedLineBreak(DocumentInfoHandle docInfo, LSPtr lineBreak, long paraIndex, Boolean firstLine);
long StyleRunIndex(ParagraphInfoHandle paraInfo, long *charOffset);
void FlushStyleRunCache(ParagraphInfoHandle paraInfo);
long CountStyleRuns(ParagraphInfoHandle paraInfo);
long GetStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex);
void GetStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long styleIndex);
void SavePortInfo(CGrafPtr newPort, PETEPortInfoPtr savedInfo);
void ResetPortInfo(PETEPortInfoPtr savedInfo);
void SetTextStyle(MyTextStylePtr theStyle, Boolean doColor);
long ScriptRunLen(LongSTTable theStyleTable, ParagraphInfoHandle paraInfo, long styleRunIndex, long *nextStyleRun);
void GetDefaultLineHeight(DocumentInfoHandle docInfo, long paraNum, LineMeasure *theMeasure);
void AddRunHeight(StringPtr theText, long textLen, LongSTPtr theStyle, LineMeasure *theMeasure, WhiteInfoHandle whitespaceGlobals, Boolean graphic);
short TabWidth(ParagraphInfoHandle paraInfo, short textPosition, short docWidth);
short CalcRunLengths(long startChar, long endChar, DirectionParamPtr dirParam);
short FillOrderEntry(OrderHandle orderingHndl, DirectionParamPtr dirParam, long textOffset, short orderIndex, short runIndex, Boolean lastRun);
FormatOrderPtr * GetFormatOrderHandle(DirectionParamPtr dirParam, short numRuns);
OSErr OrderRuns(OrderHandle orderingHndl, DirectionParamPtr dirParam, long textOffset, short numRuns);
OSErr AddOrderEntry(OrderHandle orderingHndl, DirectionParamPtr dirParam, long textOffset, short *orderIndex, short runIndex);
Ptr PreviousChar(Ptr textPtr, Ptr lastChar, CharByteTable table);
pascal long MyVisibleLength(Ptr textPtr, long len, short direction);
void MeasureRuns(DocumentInfoHandle docInfo, long paraIndex, OrderHandle orderingHndl, short numRuns, Boolean firstLine);
void VisibleWidth(LayoutCachePtr lineCache, short numRuns);
short IndentWidth(ParagraphInfoHandle paraInfo, Boolean firstLine);
OSErr GetLineLayout(DocumentInfoHandle docInfo, LayoutCachePtr lineCache);
OSErr LayoutLine(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, long startChar, long endChar);
OSErr RenderLine(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, Rect *drawRect, RgnHandle drawRgn, Boolean offscreen);
pascal Boolean MyRlDirProc(short theFormat, void *dirParam);
Boolean IsWhitespace(StringPtr theText, long textLen, ScriptCode script, WhiteInfoHandle whitespaceGlobals, long *leadWS);
OSErr AddTextPtr(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long textSize, long hOffset);
OSErr AddText(DocumentInfoHandle docInfo, long textOffset, Handle theText);
OSErr LoadText(DocumentInfoHandle docInfo, long textStart, LoadParamsPtr loadParam, Boolean forceLoad);
void UnloadText(DocumentInfoHandle docInfo);
OSErr DeleteStyle(LongSTTable theStyleTable, long styleIndex);
OSErr AddStyle(PETEStyleInfoPtr newStyle, LongSTTable theStyleTable, unsigned long *curStyleIndex, Boolean graphic, Boolean printing);
OSErr ChangeStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long styleIndex);
OSErr ChangeStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex);
OSErr RemoveStyleRuns(ParagraphInfoHandle paraInfo, long styleRunIndex, long styleRunCount);
Boolean EqualStyle(PETETextStylePtr theStyle1, PETETextStylePtr theStyle2);
Boolean EqualGraphic(PETEGraphicStylePtr theStyle1, PETEGraphicStylePtr theStyle2);
long NumStyles(LongSTTable theStyleTable);
OSErr AppendStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long *styleIndex);
OSErr AddStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex);
OSErr OffsetToVPosition(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean preserveLine);
OSErr OffsetToHPosition(DocumentInfoHandle docInfo, SelDataPtr selection);
OSErr OffsetToPosition(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean preserveLine);
long HPositionToOffset(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, short hPosition, short vPosition, Boolean *leadingEdge, Boolean *graphicHit, Boolean *wasGraphic);
OSErr PositionToOffset(DocumentInfoHandle docInfo, SelDataPtr selection);
void SetStyleAndKeyboard(DocumentInfoHandle docInfo, MyTextStylePtr newStyle);
void SetSelectionStyleAndKeyboard(DocumentInfoHandle docInfo, SelDataPtr selection);
void FlushLineCache(LayoutCachePtr lineCache);
pascal void ScrollActionProc(ControlRef theControl, short partCode);
OSErr HandleClick(DocumentInfoHandle docInfo, EventRecord *theEvent);
OSErr HandleDragging(DocumentInfoHandle docInfo, EventRecord *theEvent);
OSErr HandleSelection(DocumentInfoHandle docInfo, SelDataPtr selAnchor, Point curMouseLoc, short modifiers);
OSErr MyHandleEvent(DocumentInfoHandle docInfo, EventRecord *theEvent);
OSErr HandleKey(DocumentInfoHandle docInfo, EventRecord *theEvent);
OSErr InsertKeyChar(DocumentInfoHandle docInfo, unsigned char charCode);
void DrawCaret(DocumentInfoHandle docInfo, SelDataPtr selection);
OSErr CalcSelectionRgn(DocumentInfoHandle docInfo, Boolean preserveLine);
OSErr AddLineToRegion(RgnHandle hiliteRgn, DocumentInfoHandle docInfo, LayoutCachePtr lineCache, long startOffset, long endOffset, short vOffset);
pascal void RectAndRgn(RgnHandle destRgn, RgnHandle scratchRgn, Rect *sourceRect);
OSErr GetWordOffset(DocumentInfoHandle docInfo, SelDataPtr selection, SelDataPtr anchorSelection, long *startOffset, long *endOffset, Boolean *wordIsWhitespace, short *charType);
void SetDocPosition(DocumentInfoHandle docInfo, long vPosition, short hPosition);
void SetPositionFromScroll(DocumentInfoHandle docInfo, Boolean vScroll, Boolean hScroll);
void ResetScrollbars(DocumentInfoHandle docInfo);
void ScrollDoc(DocumentInfoHandle docInfo, short dh, short dv);
void SetScrollPosition(DocumentInfoHandle docInfo, Boolean horizontal);
OSErr GetDocInfo(DocumentInfoHandle docInfo, PETEDocInfoPtr info);
pascal void Update1Control(ControlRef theControl, RgnHandle visRgn);
pascal void GetControlRect(ControlRef theControl, Rect *controlRect);
void SetRefCon(DocumentInfoHandle docInfo,long refCon);
long GetRefCon(DocumentInfoHandle docInfo);
short GetScrollingWidth(DocumentInfoHandle docInfo);
void RepositionDocument(DocumentInfoHandle docInfo);
void ResizeDocRect(DocumentInfoHandle docInfo, short h, short v);
void MoveDocRect(DocumentInfoHandle docInfo, short h, short v);
void GetVScrollbarLocation(Rect *docRect, Rect *controlRect, Boolean hasHScroll, Boolean hasGrowBox);
void GetHScrollbarLocation(Rect *docRect, Rect *controlRect, Boolean hasVScroll, Boolean hasGrowBox);
void ChangeDocWidth(DocumentInfoHandle docInfo, short docWidth, Boolean pinMargins);
OSErr MyDoActivate(DocumentInfoHandle docInfo, Boolean activeText, Boolean activeScrolls);
void InvertSelection(DocumentInfoHandle docInfo);
Handle GetDocTextHandle(DocumentInfoHandle docInfo);
OSErr TEScrapToPETEStyle(StScrpHandle teScrap, PETEStyleListHandle *styleHandle);
OSErr PETEStyleToTEScrap(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleListHandle styleHandle, StScrpHandle *teScrap);
void MyEventFilter(PETEGlobalsHandle globals, EventRecord *theEvent);
Boolean MyMenuSelectFilter(PETEGlobalsHandle pi, long menuResult);
OSErr MyHandleEditCall(DocumentInfoHandle docInfo, PETEEditEnum what, EventRecord *theEvent);
void SetCorrectCursor(DocumentInfoHandle docInfo, Point mouseLoc, RgnHandle curRgn, EventRecord *theEvent);
OSErr DoInsertText(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long len, long hOffset, PETEStyleListHandle styles);
OSErr InsertText(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long len, long hOffset, PETEStyleListHandle styles, Boolean doRedraw);
OSErr DeleteText(DocumentInfoHandle docInfo, long startChar, long endChar);
long GetNextChar(DocumentInfoHandle docInfo, long curOffset, Boolean previousChar);
Boolean CombineParagraphs(DocumentInfoHandle docInfo, long paraIndex);
OSErr SetDirty(DocumentInfoHandle docInfo, long startOffset, long endOffset, Boolean dirty);
OSErr SetHonorLock(DocumentInfoHandle docInfo, Byte honorLockBits);
long GetDocSize(DocumentInfoHandle docInfo);
OSErr CopySelectionToClip(DocumentInfoHandle docInfo, Boolean plainText);
OSErr PasteText(DocumentInfoHandle docInfo, long textOffset, StringHandle textScrap, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, Boolean plainText, Boolean dispose);
void HandleVerticalArrow(DocumentInfoHandle docInfo, short modifiers, Boolean upArrow);
void HandleHorizontalArrow(DocumentInfoHandle docInfo, short modifiers, Boolean leftArrow);
void GetSelectionVisibleDistance(DocumentInfoHandle docInfo, SelDataPtr selection, Rect *viewRect, long *vDistance, short *hDistance);
void MakeSelectionVisible(DocumentInfoHandle docInfo, SelDataPtr selection);
Boolean PinSelectionLock(DocumentInfoHandle docInfo, long curOffset, long *newOffset);
OSErr SetSelection(DocumentInfoHandle docInfo, long startOffset, long endOffset);
OSErr GetSelectionStyle(DocumentInfoHandle docInfo, SelDataPtr selection, LongStyleRunPtr styleRun, long *styleRunOffset, MyTextStylePtr textStyle);
OSErr RedrawParagraph(DocumentInfoHandle docInfo, long paraIndex, long startOffset, long endOffset, LSTable oldLineStarts);
Boolean SelectionHasLock(DocumentInfoHandle docInfo, long startOffset, long endOffset);
short GetPrimaryLineDirection(DocumentInfoHandle docInfo, long textOffset);
OSErr RedrawDocument(DocumentInfoHandle docInfo, long textOffset);
OSErr InsertParagraphBreak(DocumentInfoHandle docInfo, long breakChar);
OSErr ScrollAndDrawDocument(DocumentInfoHandle docInfo, long vPosition, long distance);
pascal void GetVisibleRgn(RgnHandle visRgn);
OSErr ParagraphPosition(DocumentInfoHandle docInfo, long paraIndex, long *vPosition);
OSErr GetText(DocumentInfoHandle docInfo, long startChar, long endChar, Ptr theText, long intoSize, long *textLen);
OSErr DoScroll(DocumentInfoHandle docInfo, short horizontal, short vertical);
void GetSelection(DocumentInfoHandle docInfo, long *startOffset, long *endOffset);
OSErr GetStyleScrap(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, Boolean allParas, Boolean tempStyleMem, Boolean tempParaMem, Boolean clearLock);
OSErr ChangeStyleRange(DocumentInfoHandle docInfo, long startOffset, long endOffset, StyleInfoPtr newStyle, long validBits);
long ChangeStyleInfo(StyleInfoPtr sourceStyle, StyleInfoPtr destStyle, long validBits, Boolean isGraphic);
pascal void GetPortRect(Rect *portRect);
Boolean IsSelectionLocked(DocumentInfoHandle docInfo, long startOffset, long endOffset);
Boolean EqualPETEandTEScrap(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleEntry *peteScrap, ScrpSTElement *scrapEntry);
OSErr GetClipContents(PETEGlobalsHandle globals, StringHandle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap);
OSErr SetDeleteUndo(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEUndoEnum undoType);
void SetInsertUndo(DocumentInfoHandle docInfo, long startOffset, long textLen, long selStart, long selEnd, PETEUndoEnum undoType, Boolean append);
OSErr DeleteWithKey(DocumentInfoHandle docInfo, long startOffset, long endOffset);
OSErr LoadTextIntoHandle(DocumentInfoHandle docInfo, long startOffset, long endOffset, Handle destHandle, long destOffset);
pascal Handle MyNewHandle(long hndlSize, OSErr *errCode, short handleFlags);
pascal OSErr MyPtrToHand(void *srcPtr, Handle *destHndl, long size, short handleFlags);
pascal OSErr MyHandToHand(Handle *theHndl, short handleFlags);
pascal void LocalToGlobalRgn(RgnHandle theRgn);
pascal void LocalToGlobalRect(Rect *theRect);
pascal void GlobalToLocalRgn(RgnHandle theRgn);
pascal void GlobalToLocalRect(Rect *theRect);
void PenAndColorNormal(void);
long GetEndTextLen(PETEParaScrapHandle paraScrap, long endTextLen);
OSErr AppendToScrap(StringHandle *undoTextNew, PETEStyleListHandle *styleScrapNew, PETEParaScrapHandle *paraScrapNew, StringHandle undoText, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, long *endTextLen, Boolean tempMem);
OSErr MyDoUndo(DocumentInfoHandle docInfo);
OSErr SetUndoFlag(DocumentInfoHandle docInfo, Boolean allowUndo, Boolean clearUndo);
OSErr PunctuateUndo(DocumentInfoHandle docInfo);
OSErr FindLabelRun(DocumentInfoHandle docInfo, long offset, long *start, long *end, unsigned short label, unsigned short mask);
RgnHandle GetDragAutoscrollRgn(DocumentInfoHandle docInfo);
OSErr CheckDragAcceptable(DocumentInfoHandle docInfo, Boolean dragTextOnly);
pascal OSErr MyTrackingHandler(DragTrackingMessage message, WindowPtr theWindow, void *handlerRefCon, DragReference theDragRef);
pascal OSErr MyReceiveHandler(WindowPtr theWindow, void *handlerRefCon, DragReference theDragRef);
OSErr GetOffsetStyle(DocumentInfoHandle docInfo, long offset, MyTextStylePtr textStyle);
OSErr GetParaInfo(DocumentInfoHandle docInfo, long index, PETEParaInfoPtr info);
OSErr GetParaIndex(DocumentInfoHandle docInfo, long offset, long *index);
long LineHeights(LSTable lineStarts, long startIndex, long endIndex);
long ResetLineCache(ParagraphInfoHandle paraInfo, long lineIndex);
long ResetLineCacheToVHeight(ParagraphInfoHandle paraInfo, long vHeight, Boolean previousLine);
Boolean FindAndCompressLineStarts(LSTable lineStarts, LSPtr curLine, long lineIndex, long* lineCount);
OSErr SetRecalcState(DocumentInfoHandle docInfo, Boolean recalc);
long ClosestValue(long target, long source1, long source2);
OSErr FindNextInsertLoc(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean hasPicture);
OSErr DragTextOnly(DragReference theDragRef);
OSErr SetParagraphInfo(DocumentInfoHandle docInfo, long paraIndex, PETEParaInfoPtr paraInfo, short validBits);
OSErr SetParaStyleUndo(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEUndoEnum undoType);
void ClearUndo(DocumentInfoHandle docInfo);
OSErr ApplyStyleList(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEStyleListHandle styleScrap);
long GetTextLen(DocumentInfoHandle docInfo);
void PtToSelPosition(DocumentInfoHandle docInfo, Point thePt, SelDataPtr selection);
OSErr GetStyleFromOffset(DocumentInfoHandle docInfo, long offset, long *length, PETEStyleEntryPtr theStyle);
void InvalidateDocument(DocumentInfoHandle docInfo, Boolean viewOnly);
OSErr SetCallbackRoutine(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, ProcPtr theProc, PETECallbackEnum procType);
OSErr CreateDocPict(DocumentInfoHandle docInfo, PicHandle *thePic);
pascal void SetVisibleRgn(RgnHandle visRgn);
OSErr InitForPrinting(DocumentInfoHandle docInfo);
OSErr PrintSelectionSetup(DocumentInfoHandle docInfo, long *paraIndex, long *lineIndex);
OSErr CleanupPrinting(DocumentInfoHandle docInfo);
OSErr PrintPage(DocumentInfoHandle docInfo, CGrafPtr printPort, Rect *destRect, long *paraIndex, long *lineIndex);
void FlushDocInfoLineCache(DocumentInfoHandle docInfo);
OSErr GetGraphicInfoFromOffset(DocumentInfoHandle docInfo, long offset, GraphicInfoHandle *graphicInfo, long *length);
Boolean SameGraphic(DocumentInfoHandle docInfo, SelDataPtr selection);
OSErr MyCallProgressLoop(DocumentInfoHandle docInfo);
pascal short StyleToScript(MyTextStylePtr theStyle);
pascal Boolean EqualFont(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, short font1, short lang1, short font2, short lang2);
pascal Boolean EqualFontSize(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, short size1, short size2);
pascal short StyleToFont(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean printing);
pascal short StyleToFontSize(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean printing);
pascal short StyleToScript(MyTextStylePtr theStyle);
pascal short LangToScript(short langCode);
void SetTextStyleWithDefaults(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean doColor, Boolean printing);
OSErr AliasAEToDir(AEDesc *aliasAE, short *volumeID, long *directoryID);
Boolean DragWasToTrash(DragReference dragRef);
OSErr ResetAndInvalidateDocument(DocumentInfoHandle docInfo);
OSErr CheckParagraphMeasure(DocumentInfoHandle docInfo, long paraNum, Boolean needLineStarts);
OSErr SetDefaultStyle(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEDefaultFontPtr defaultFont);
OSErr SetLabelStyle(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETELabelStylePtr labelStyle);
void RecalcStyleHeights(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, Boolean printing);
OSErr FindTripleOffsets(DocumentInfoHandle docInfo, long paraIndex, long offset, long *startOffset, long *endOffset);
OSErr MakePlainStyles(PETEStyleListHandle styleScrap, PETEStyleListHandle *newStyleScrap, Boolean plainTextOnly, Boolean textOnly, Boolean tempMemPref);
Boolean HasTEStyles(DocumentInfoHandle docInfo, long startOffset, long endOffset, Boolean plainText);
OSErr SetControlDrag(DocumentInfoHandle docInfo, Boolean useControl);
Boolean DragItemHasFlavor(DragReference theDragRef, ItemReference theItemRef, FlavorType targetType);
void AddScrapStyles(LongSTTable theStyleTable, PETEStyleListHandle styleScrap);
void DeleteScrapStyles(LongSTTable theStyleTable, PETEStyleListHandle styleScrap);
void GetCurrentTextStyle(DocumentInfoHandle docInfo, MyTextStylePtr textStyle);
long GetDocHeight(DocumentInfoHandle docInfo);
Boolean FindNextFF(Ptr theText, long startOffset, long endOffset, long *ffLoc);
OSErr RemoveText(DocumentInfoHandle docInfo, long textOffset, long textSize);
OSErr FindMyText(DocumentInfoHandle docInfo, Ptr theText, long textLen, long startOffset, long endOffset, long *foundOffset, ScriptCode scriptCode);
OSErr RecalcDocHeight(DocumentInfoHandle docInfo);
void SetParaDirty(DocumentInfoHandle docInfo, long paraIndex);
OSErr ApplyParaScrapInfo(DocumentInfoHandle docInfo, long offset, PETEParaScrapHandle paraScrap);
void DrawQuoteMarks(DocumentInfoHandle docInfo, long paraIndex, short lineHeight);
void EraseCaret(DocumentInfoHandle docInfo);
Boolean ShouldDoFastInsert(DocumentInfoHandle docInfo, StringPtr insertText);
OSErr GetParaScrap(DocumentInfoHandle docInfo, long paraIndex, PETEParaScrapHandle *paraScrap, Boolean tempMem);
OSErr OffsetToPt(DocumentInfoHandle docInfo, long offset, Point *position, LHPtr lineHeight);
OSErr PtToOffset(DocumentInfoHandle docInfo, Point position, long *offset);
void ScrollRectInDoc(DocumentInfoHandle docInfo, Rect *scrollRect, short dh, short dv, RgnHandle scrollRgn);
void DrawOneChar(DocumentInfoHandle docInfo, StringPtr theChar);
OSErr MyCallGraphicHit(DocumentInfoHandle docInfo, long offset, EventRecord *theEvent);
RGBColor DocOrGlobalColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo);
RGBColor DocOrGlobalBGColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo);
OSErr SetDefaultColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, RGBColor *defaultColor);
OSErr SetDefaultBGColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, RGBColor *defaultColor);
pascal Style GetPortFace(void);
OSErr MyReplaceText(DocumentInfoHandle docInfo, long textOffset, long replaceLen, Ptr theText, long len, long hOffset, PETEStyleListHandle styles);
void InvalidateDocUpdate(DocumentInfoHandle docInfo);
void GetWhitespaceGlobals(WhiteInfoHandle whitespaceGlobals, ScriptCode script);
pascal StyledLineBreakCode MyStyledLineBreak(Ptr textPtr, long textLen, long textStart, long textEnd, long flags, short *textWidth, long *textOffset, PETEGlobalsHandle globals);
pascal short MyTextWidth(StringPtr textPtr, short offset, short len, PETEGlobalsHandle globals);
pascal short MyPixelToChar(Ptr textBuf, long textLen, short slop, short pixelWidth, Boolean *leadingEdge, short *widthRemaining, JustStyleCode styleRunPosition, Point numer, Point denom, PETEGlobalsHandle globals);
pascal short MyCharToPixel(Ptr textBuf, long textLen, short slop, long offset, short direction, JustStyleCode styleRunPosition, Point numer, Point denom, PETEGlobalsHandle globals);
Boolean ShouldCenterSelection(DocumentInfoHandle docInfo, Boolean force);
OSErr CallGraphic(DocumentInfoHandle docInfo, PETEGraphicInfoHandle graphicInfo, long offset, PETEGraphicMessage message, void *data);
GWorldPtr GetGlobalGWorld(PETEGlobalsHandle globals);
OSErr GetStylesForMenu(DocumentInfoHandle docInfo, MyTextStylePtr curTextStyle, PETEStylesForMenuPtr selectionStyles);
OSErr GetIntelligentCutBoundaries(DocumentInfoHandle docInfo, long *start, long *end);
OSErr AddIntelligentPasteSpace(DocumentInfoHandle docInfo, SelDataPtr selection, StringHandle textScrap, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, short *added);
OSErr SelectGraphic(DocumentInfoHandle docInfo, long offset);
short LeftPosition(ParagraphInfoHandle paraInfo, short docWidth, short visibleWidth, short *totalSlop, Boolean firstLine);
OSErr FindStyle(DocumentInfoHandle docInfo, long startOffset, long *offset, PETEStyleInfoPtr theStyle,PETEStyleEnum validBits);
void OffsetIdleGraphics(DocumentInfoHandle docInfo, long startChar, long offset);
void DeleteIdleGraphics(DocumentInfoHandle docInfo, long startChar, long endChar);
void NewIdleGraphics(DocumentInfoHandle docInfo, long startChar, long endChar);
void DisposeScrapGraphics(PETEStyleListHandle styleScrap, long startIndex, long endIndex, Boolean killPictures);
OSErr CloneScrapGraphics(PETEStyleListHandle styleScrap);
OSErr GetTextStyleScrap(DocumentInfoHandle docInfo, long startOffset, long endOffset, Handle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, Boolean allParas, Boolean tempTextMem, Boolean tempStyleMem, Boolean tempParaMem, Boolean clearLock);
void LiveScroll(PETEGlobalsHandle globals, Boolean live);
void DeleteGraphicFromStyle(LongSTTable theStyleTable, long styleIndex);
OSErr GetCallbackRoutine(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, ProcPtr *theProc, PETECallbackEnum procType);
OSErr GetWordFromOffset(DocumentInfoHandle docInfo, long offset, Boolean leadingEdge, long *startOffset, long *endOffset, Boolean *ws, short *charType);
OSErr CompareStyles(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleEntryPtr style1, PETEStyleEntryPtr style2, long validBits, Boolean printing, long *diffBits);
OSErr GetHorizontalSelection(DocumentInfoHandle docInfo, SelDataPtr selection, short modifiers, Boolean leftArrow, Boolean gotoEnd);
OSErr AddParagraphBreaks(DocumentInfoHandle docInfo);
OSErr RecalcSelectionPosition(DocumentInfoHandle docInfo, SelDataPtr selAnchor, Byte lock, Boolean always);
OSErr GetDebugStyleScrap(DocumentInfoHandle docInfo, long paraIndex, PETEStyleListHandle *styleHandle);
Boolean TrackScrolls(DocumentInfoHandle docInfo, ControlRef theControl, Point where, short partCode);
OSErr InsertParagraphBreakLo(DocumentInfoHandle docInfo, long breakChar, Boolean redraw);
OSErr ForceRecalc(DocumentInfoHandle docInfo, long startOffset, long endOffset);
OSErr DoParaRecalc(DocumentInfoHandle docInfo, long paraIndex, long startOffset, long endOffset, long startStyleRun, long endStyleRun, long validBits);
OSErr DeleteParagraphBreak(DocumentInfoHandle docInfo, long paraIndex);
Boolean GraphicInList(PETEGlobalsHandle globals, PETEGraphicInfoHandle graphicInfo);
void UseThisScrap(ScrapRef scrap);
void RGBBackColorAndPat(RGBColor *color);

#endif PEE /* For Steve */

#endif