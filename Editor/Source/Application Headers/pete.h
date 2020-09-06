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

#ifndef PETE_H
#define PETE_H

#if !rez

#ifndef PETEINLINE
#ifndef __COMPONENTS__
#include <Components.h>
#endif
#endif

#ifndef __CONTROLS__
#include <Controls.h>
#endif

#ifndef __DRAG__
#include <Drag.h>
#endif

#ifndef __EVENTS__
#include <Events.h>
#endif

#ifndef __TEXTEDIT__
#include <TextEdit.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_STRUCT_ALIGN
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#endif // rez

#define kPETEComponentManufact	'CSOm'
#define kPETEComponentType		'Edit'
#define kPETEComponentSubType	'Pete'
#define kPETECurrentVersion		0x00140098

/* ResTypes for the clipboard */
#define kPETEParaScrap			'Ppar'
#define kPETEStyleScrap			'Psty'
#define kPictureScrap			'PICT'
#define kStyleScrap				'styl'
#define kTextScrap				'TEXT'
#define kURLScrap				'url '

/* Flavor types for drags */
#define kPETEA822Flavor			'a822'
#define kPETEParaFlavor			kPETEParaScrap
#define kPETEStyleFlavor		kPETEStyleScrap
#define flavorTypePict			kPictureScrap
#define flavorTypeStyle			kStyleScrap
#define flavorTypeText			kTextScrap
#define flavorTypeURL			kURLScrap

#define kPETELastPara			LONG_MAX	/* Reference to the last paragraph */
#define kPETEInsertionPoint		-1L			/* Reference to the insertion point */
#define kPETECurrentSelection	-1L			/* Reference to the current selection */
#define kPETEDefaultMargin		-1L			/* Reference to the default margin; see editor.h if curious */
#define kPETEOffsetUnknown		-2L			/* For use with graphics; offset unknown */
#define kPETECurrentStyle		-2L			/* Reference to the style that the next typed thing will be */
#define kPETEDefaultStyle		-3L			/* Reference to the document "default style" */
#define kPETEDefaultPara		-3L			/* Reference to the documents default paragraph info */

#define kPETEDefaultFont		-1
#define kPETEDefaultFixed		-2
#define kPETEDefaultSize		-1
#define kPETERelativeSizeBase	-64

#define	kPETEFixedTab			-1 			/* Put into tabCount for a repeated fixed-width tab */

#define kPETELabelShift			20
#define kPETELockShift			16

#define IsPETEDefaultColor(x)	(((x).red == 1) && ((x).green == 1) && ((x).blue == 1))
#define SetPETEDefaultColor(x)	((x).red = (x).green = (x).blue = 1)

#define RectHeight(x) ((x)->bottom - (x)->top)
#define RectWidth(x) ((x)->right - (x)->left)

#if !rez

/***** The enums *****/

typedef enum {
	peNoLock = 0x00,			/* No lock at all */
	peModLock = 0x01,			/* Selectable, but not changeable */
	peClickAfterLock = 0x02,	/* Selectable past this character and not changeable */
	peClickBeforeLock = 0x04,	/* Selectable before this character and not changeable */
	peSelectLock = 0x08			/* Not selectable at all and not changeable */
} PETELockBits;

typedef enum {
	psePageUp = -10,
	psePageDn,
	pseLineUp,
	pseLineDn,
	pseSelection,	/* show selection */
	pseCenterSelection,
	pseForceCenterSelection,
	pseNoScroll = -1
} PETEScrollEnum;

typedef enum {

/* High Byte - label, Next Byte - lock, Next Byte - valid, Low Byte - face */

	peBoldValid = bold,
	peItalicValid = italic,
	peUnderlineValid = underline,
	peOutlineValid = outline,
	peShadowValid = shadow,
	peCondenseValid = condense,
	peExtendValid = extend,
	peFaceValid = 0x0000007F,		/* Face is always valid; the bits themselves are used */
	peFontValid = 0x00000100,
	peSizeValid = 0x00000200,
	peColorValid = 0x00000400,
	peLangValid = 0x00000800,
	peLockValid = 0x000F1000,
	peLabelValid = 0xFFF02000,
	peGraphicValid = 0x00004000,
	peGraphicColorChangeValid = 0x00008000,
	peAllValid = 0xFFFF3F7F			/* For backwards compatibility */
} PETEStyleEnum;

typedef enum {
	peStartMarginValid = 0x0001,
	peEndMarginValid = 0x0002,
	peIndentValid = 0x0004,
	peDirectionValid = 0x0008,
	peJustificationValid = 0x0010,
	peFlagsValid = 0x0020,
	peTabsValid = 0x0040,
	peQuoteLevelValid = 0x0080,
	peSignedLevelValid = 0x0100,
	peAllParaValid = 0x01FF
} PETEParaEnum;

typedef enum {
	peeCut,
	peeCopy,
	peePaste,
	peeClear,
	peeEvent,
	peeUndo,
	peeCutPlain,
	peeCopyPlain,
	peePastePlain,
	peeLimit = 0x7FFF
} PETEEditEnum;

typedef enum {
	peSetDragContents,
	peGetDragContents,
	peProgressLoop,
	peDocChanged,
	peHasBeenCalled,
	peWordBreak,
	peIntelligentCut,
	peIntelligentPaste,
	peCallbackLimit = 0x7FFF
} PETECallbackEnum;

typedef enum {
	peGraphicDraw,		/* data is PETEGraphicStylePtr */
	peGraphicClone,		/* data is (PETEGraphicInfoHandle *) into which to put duplicate */
	peGraphicTest,		/* data is (Point *) from top left; return errOffsetIsOutsideOfView if not hit */
	peGraphicHit,		/* data is (EventRecord *) if mouse down, nil if time to turn off */
	peGraphicRemove,	/* data is nil; just dispose a copy of graphic */
	peGraphicResize,	/* data is a (short *) of the max width */
	peGraphicRegion,	/* data is a RgnHandle which is to be changed to the graphic's region */
	peGraphicEvent,		/* data is (EventRecord *) */
	peGraphicInsert,	/* data is nil. Graphic has been inserted into a new document. */
	peGraphicNewText	/* data is (PETEGraphicTextPtr) to put new text. */
} PETEGraphicMessage;

typedef enum {
	peTextOnly = 0x01,
	pePlainTextOnly = 0x02,
	peNoParaPaste = 0x04,
	peDiskList = 0x10,
	peSquareList = 0x20,
	peCircleList = 0x40,
	peListBits = 0x70
} PETEParaInfoFlags;

typedef enum {
	peVScroll = 0x00000001,
	peHScroll = 0x00000002,
	peGrowBox = 0x00000004,
	peUseHLineWidth = 0x00000008,
	peUseOffscreen = 0x00000010,
	peDrawDebugSymbols = 0x00000020,
	peDragOnControl = 0x00000040,
	peNoStyledPaste = 0x00000080,
	peClearAllReturns = 0x00000100,
	peEmbedded = 0x00000200 /* Ignore offscreen if set */
} PETEDocInfoFlags;

typedef enum {
	peCantUndo,
	peUndoTyping,
	peUndoCut,
	peUndoPaste,
	peUndoClear,
	peUndoStyle,
	peUndoDrag,
	peUndoPara,
	peUndoCutPlain,
	peUndoStyleAndPara,
	peUndoLast = peUndoStyleAndPara,
	peUndoMaximum = 0x7FFF,
	peRedoTyping = -peUndoTyping,
	peRedoCut = -peUndoCut,
	peRedoPaste = -peUndoPaste,
	peRedoClear = -peUndoClear,
	peRedoStyle = -peUndoStyle,
	peRedoDrag = -peUndoDrag,
	peRedoPara = -peUndoPara,
	peRedoCutPlain = -peUndoCutPlain,
	peRedoStyleAndPara = -peUndoStyleAndPara,
	peRedoLast = -peUndoLast
} PETEUndoEnum;

/***** The structure placeholder definitions *****/

typedef struct PETEPrivateDocumentInfo **PETEHandle;
typedef struct PETEGraphicInfo PETEGraphicInfo, *PETEGraphicInfoPtr, **PETEGraphicInfoHandle;
typedef struct PETEStyleEntry PETEStyleEntry, *PETEStyleList, *PETEStyleEntryPtr, **PETEStyleListHandle;
typedef struct PETEParaScrapEntry PETEParaScrapEntry, *PETEParaScrapPtr, **PETEParaScrapHandle;

/***** Routine declarations for callbacks *****/
/*
	Note that the actual UPP definitions are missing. They're included from peteUPP.h
	at the bottom of this stuff.
*/

/* Callback to tell the caller that a PETE routine has been called */
typedef pascal void (*PETEHasBeenCalledProcPtr)(Boolean entry);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEHasBeenCalledUPP;
#else
typedef PETEHasBeenCalledProcPtr PETEHasBeenCalledUPP;
#endif

typedef PETEHasBeenCalledUPP PETEHasBeenCalled;

/* Callback to indicate that the document has changed. */

typedef pascal OSErr (*PETEDocHasChangedProcPtr)(PETEHandle ph,long start,long stop);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEDocHasChangedUPP;
#else
typedef PETEDocHasChangedProcPtr PETEDocHasChangedUPP;
#endif

typedef PETEDocHasChangedUPP PETEDocHasChanged;

/* Callback definition for setting drag contents.      */
/* Return handlerNotFoundErr for "handle it yourself". */

typedef pascal OSErr (*PETESetDragContentsProcPtr)(PETEHandle ph,DragReference theDragRef);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETESetDragContentsUPP;
#else
typedef PETESetDragContentsProcPtr PETESetDragContentsUPP;
#endif

typedef PETESetDragContentsUPP PETESetDragContents;

/* Callback definition for getting drag contents to insert after a drop. */
/* Return handlerNotFoundErr for "handle it yourself". */

typedef pascal OSErr (*PETEGetDragContentsProcPtr)(PETEHandle ph,StringHandle *theText,PETEStyleListHandle *theStyles,PETEParaScrapHandle *theParas,DragReference theDragRef,long dropLocation);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEGetDragContentsUPP;
#else
typedef PETEGetDragContentsProcPtr PETEGetDragContentsUPP;
#endif

typedef PETEGetDragContentsUPP PETEGetDragContents;

/* Callback definition for a simple SpinCursor-like routine. */
/* This will be called alot; don't just blindly call WNE every time */

typedef pascal OSErr (*PETEProgressLoopProcPtr)(void);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEProgressLoopUPP;
#else
typedef PETEProgressLoopProcPtr PETEProgressLoopUPP;
#endif

typedef PETEProgressLoopUPP PETEProgressLoop;

/* Callback definition for the graphic drawing/handling routines. */

typedef pascal OSErr (*PETEGraphicHandlerProcPtr)(PETEHandle ph,PETEGraphicInfoHandle graphic,long offset,PETEGraphicMessage message,void *data);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEGraphicHandlerUPP;
#else
typedef PETEGraphicHandlerProcPtr PETEGraphicHandlerUPP;
#endif

typedef PETEGraphicHandlerUPP PETEGraphicHandler;

/* Callback definition for the word break routine. */

typedef pascal OSErr (*PETEWordBreakProcPtr)(PETEHandle ph,long offset,Boolean leadingEdge,long *startOffset,long *endOffset,Boolean *ws,short *charType);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEWordBreakUPP;
#else
typedef PETEWordBreakProcPtr PETEWordBreakUPP;
#endif

typedef PETEWordBreakUPP PETEWordBreak;

/* Callback definition for the intelligent cut routine. */

typedef pascal OSErr (*PETEIntelligentCutProcPtr)(PETEHandle pi,long *start,long *stop);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEIntelligentCutUPP;
#else
typedef PETEIntelligentCutProcPtr PETEIntelligentCutUPP;
#endif

typedef PETEIntelligentCutUPP PETEIntelligentCut;

/* Callback definition for the intelligent paste routine. */

typedef pascal OSErr (*PETEIntelligentPasteProcPtr)(PETEHandle pi,long offset,StringHandle textScrap,PETEStyleListHandle styleScrap,PETEParaScrapHandle paraScrap,short *added);

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr PETEIntelligentPasteUPP;
#else
typedef PETEIntelligentPasteProcPtr PETEIntelligentPasteUPP;
#endif

typedef PETEIntelligentPasteUPP PETEIntelligentPaste;

/* Include the UPP declarations */
#include "peteUPP.h"

/***** The actual structure definitions *****/
typedef struct {
	Ptr text;
	long offset;
	long len;
	Boolean handle;
} PETEGraphicText, *PETEGraphicTextPtr;

struct PETEGraphicInfo {
	PETEGraphicHandlerProcPtr itemProc;
	short width;
	short height;
	short descent;
	long filler : 27;
	long drawInWindow : 1;
	long cloneReplaceText : 1;
	long cloneOnlyText : 1;
	long wantsEvents : 1;
	long forceLineBreak : 1;
	long isSelected : 1;
	long privateType;	/* For application's use */
	Byte privateData[];
};

typedef struct PETETextStyle {
	short tsFont;
	Style tsFace;
	char filler;
	short tsSize;
	RGBColor tsColor;
	LangCode tsLang;
	unsigned short tsLock : 4;
	unsigned short tsLabel : 12; /* For application's use */
} PETETextStyle, *PETETextStylePtr;

typedef struct PETEGraphicStyle {
	short tsFont;
	Style tsFace;
	char filler;
	short tsSize;
	PETEGraphicInfoHandle graphicInfo;
	short filler0;
	LangCode tsLang;
	unsigned short tsLock : 4;
	unsigned short tsLabel : 12; /* For application's use */
} PETEGraphicStyle, *PETEGraphicStylePtr;

typedef union PETEStyleInfo {
	PETETextStyle textStyle;
	PETEGraphicStyle graphicStyle;
} PETEStyleInfo, *PETEStyleInfoPtr;

struct PETEStyleEntry {
	long psStartChar;
	long psGraphic;		/* Set to 0 for text, 1 for graphic */
	PETEStyleInfo psStyle;
};

typedef struct {
	ScriptCode pdScript;	/* Script to specify */
	short pdFont;			/* Default font for this script */
	short pdSize;			/* Default size for this script */
	Boolean pdPrint;		/* Default for printing if true, screen if false */
	Boolean pdFixed;		/* Default for fixed width if true, proportional if false */
} PETEDefaultFontEntry, *PETEDefaultFontPtr, *PETEDefaultFontList, **PETEDefaultFontHandle;

typedef struct {
	unsigned short plLabel;
	unsigned short plValidBits;
	short plFont;
	Style plFace;
	char filler;
	short plSize;
	RGBColor plColor;
	short plColorWeight;
} PETELabelStyleEntry, *PETELabelStylePtr, *PETELabelStyleList, **PETELabelStyleHandle;

struct PETEParaScrapEntry {
	long paraLength;
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
	Fixed tabStops[];
};

typedef struct PETEParaInfo {
	long paraOffset;
	long paraLength;
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
	short **tabHandle;
} PETEParaInfo, *PETEParaInfoPtr;

typedef struct PETEDocInitInfo {
	WindowRef inWindow;
	Rect inRect;
	short docWidth;
	short filler;
	unsigned long docFlags;
	PETEParaInfo inParaInfo;
	PETETextStyle inStyle;
	ControlRef containerControl;
} PETEDocInitInfo, *PETEDocInitInfoPtr;

typedef struct PETEDocInfo {
	long selStart;
	long selStop;
	long styleRunStart;
	long styleRunStop;
	long docHeight;
	long paraCount;
	Rect docRect;
	Rect viewRect;
	PETETextStyle curTextStyle;
	unsigned long dirty;
	WindowRef docWindow;
	Boolean recalcOn : 1;
	Boolean doubleClick : 1;
	Boolean docActive : 1;
	Boolean printing : 1;
	Boolean filler5 : 1;
	Boolean filler6 : 1;
	Boolean filler7 : 1;
	Boolean filler8 : 1;
	PETEUndoEnum undoType;
	short padding;
} PETEDocInfo, *PETEDocInfoPtr;

typedef struct PETEStylesForMenu {
	Style usedFaces;
	Style allFaces;
	short **usedFonts;
	short **usedSizes;
	RGBColor **usedColors;
} PETEStylesForMenu, *PETEStylesForMenuPtr;

typedef struct {
	Boolean allParas;
	Boolean textTempPref;
	Boolean styleTempPref;
	Boolean paraTempPref;
	Boolean clearLock;
} PETEGetStyleScrapFlags;

typedef struct PETEPrivateStyleEntry **PETEGraphicStyleHandle;
typedef struct PETEPrivateLineStartEntry **PETEGraphicLSTable;

/* If conf.h defines PETEINLINE, the editor is a library, so don't treat it like a component. */
#ifdef PETEINLINE
#define PETECOMPONENT_H
#endif

/* The following is so the same headers can be used for component routines */
#ifndef PETECOMPONENT_H
/* Magic inline stuff for 68K application */
#define CallPETEComponent(callType) \
	FIVEWORDINLINE( 0x2F3C,kPETE##callType##ParamSize,kPETE##callType##Rtn,0x7000,_ComponentDispatch )
/* Definition of private component instance */
typedef ComponentInstance PETEInst;
/* Include information for component definitions */
#include "petecomponent.h"
#else
/* Special stuff for the component routines */
#define PETECOMPONENT_CFILE
#define CallPETEComponent(callType)
typedef struct PETEPrivateGlobalsInfo **PETEInst;
#endif

/***** The routines *****/


/* Call PETEInit once. This routine will be implemented as glue in the application. */
OSErr PETEInit(PETEInst *pi, PETEGraphicHandlerProcPtr *graphics);

/* Call PETECleanup only if PETEInit returns noErr. Implemented as glue in the application.*/
OSErr PETECleanup(PETEInst pi);

/* Create and Dispose of a PETE edit document */
pascal ComponentResult PETECreate(PETEInst pi,PETEHandle *ph,PETEDocInitInfoPtr initInfo)
	CallPETEComponent(Create);
pascal ComponentResult PETEDispose(PETEInst pi,PETEHandle ph)
	CallPETEComponent(Dispose);

/* RefCon is for application's use */
pascal ComponentResult PETESetRefCon(PETEInst pi,PETEHandle ph,long refCon)
	CallPETEComponent(SetRefCon);
pascal long PETEGetRefCon(PETEInst pi,PETEHandle ph)
	CallPETEComponent(GetRefCon);

/*
	For InsertText and InsertParagraph, nil style means current style.
	For InsertParagraph, nil paraInfo means use same settings as last paragraph.
	Both style and paraInfo are set to a default in the Create.
*/

/* BeforePara of -1 means append to end of document */
pascal ComponentResult PETEInsertPara(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Handle text,PETEStyleListHandle styles)
	CallPETEComponent(InsertPara);	/* disposes of text if returns noErr */
pascal ComponentResult PETEInsertParaPtr(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Ptr text,long len,PETEStyleListHandle styles)
	CallPETEComponent(InsertParaPtr);
pascal ComponentResult PETEInsertParaHandle(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Handle text,long len,long hOffset,PETEStyleListHandle styles)
	CallPETEComponent(InsertParaHandle);

/* Offset of -1 means insert at current insertion point */
pascal ComponentResult PETEInsertParaBreak(PETEInst pi,PETEHandle ph,long offset)
	CallPETEComponent(InsertParaBreak);

pascal ComponentResult PETEDeletePara(PETEInst pi,PETEHandle ph,long index)
	CallPETEComponent(DeletePara);
pascal ComponentResult PETEDeleteParaBreak(PETEInst pi,PETEHandle ph,long index)
	CallPETEComponent(DeleteParaBreak);

/* Offset of -1 means insert at current insertion point or replace selection */
pascal ComponentResult PETEInsertText(PETEInst pi,PETEHandle ph,long offset,Handle text,PETEStyleListHandle styles)
	CallPETEComponent(InsertText);	/* disposes of text if returns noErr */
pascal ComponentResult PETEInsertTextPtr(PETEInst pi,PETEHandle ph,long offset,Ptr text,long len,PETEStyleListHandle styles)
	CallPETEComponent(InsertTextPtr);
pascal ComponentResult PETEInsertTextHandle(PETEInst pi,PETEHandle ph,long offset,Handle text,long len,long hOffset,PETEStyleListHandle styles)
	CallPETEComponent(InsertTextHandle);

/* Offset of -1 means insert at current insertion point or replace selection */
pascal ComponentResult PETEInsertTextScrap(PETEInst pi,PETEHandle ph,long offset,Handle textScrap,PETEStyleListHandle styleScrap,PETEParaScrapHandle paraScrap,Boolean dispose)
	CallPETEComponent(InsertTextScrap);

/* Offset of -1 means start of selection. ReplaceLen of -1 means go to end of selection. */
pascal ComponentResult PETEReplaceText(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Handle text,PETEStyleListHandle styles)
	CallPETEComponent(ReplaceText);	/* disposes of text if returns noErr */
pascal ComponentResult PETEReplaceTextPtr(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Ptr text,long len,PETEStyleListHandle styles)
	CallPETEComponent(ReplaceTextPtr);
pascal ComponentResult PETEReplaceTextHandle(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Handle text,long len,long hOffset,PETEStyleListHandle styles)
	CallPETEComponent(ReplaceTextHandle);

pascal ComponentResult PETEEventFilter(PETEInst pi,EventRecord *event)
	CallPETEComponent(EventFilter);
pascal long PETEMenuSelectFilter(PETEInst pi,long menuResult)
	CallPETEComponent(MenuSelectFilter); 	/* true if menu has been handled */

pascal ComponentResult PETEDraw(PETEInst pi,PETEHandle ph)
	CallPETEComponent(Draw);
pascal ComponentResult PETEActivate(PETEInst pi,PETEHandle ph,Boolean activeText,Boolean activeScrolls)
	CallPETEComponent(Activate);

pascal ComponentResult PETESizeDoc(PETEInst pi,PETEHandle ph,short horizontal,short vertical)
	CallPETEComponent(SizeDoc);
pascal ComponentResult PETEMoveDoc(PETEInst pi,PETEHandle ph,short horizontal,short vertical)
	CallPETEComponent(MoveDoc);
/* Negative docWidth means set to according to current viewRect */
pascal ComponentResult PETEChangeDocWidth(PETEInst pi,PETEHandle ph,short docWidth,Boolean pinMargins)
	CallPETEComponent(ChangeDocWidth);

/* Positive values are absolute amount out of 32767 */
pascal ComponentResult PETEScroll(PETEInst pi,PETEHandle ph,short horizontal,short vertical)
	CallPETEComponent(Scroll);

/* Offsets are automatically pinned to actual text length */
pascal ComponentResult PETESelect(PETEInst pi,PETEHandle ph,long start,long stop)
	CallPETEComponent(Select);

pascal ComponentResult PETEDragTrackingHandler(PETEInst pi,PETEHandle ph,DragTrackingMessage message,DragReference theDragRef)
	CallPETEComponent(DragTrackingHandler);
pascal ComponentResult PETEDragReceiveHandler(PETEInst pi,PETEHandle ph,DragReference theDragRef)
	CallPETEComponent(DragReceiveHandler);

pascal ComponentResult PETESetControlDrag(PETEInst pi,PETEHandle ph,Boolean useControl)
	CallPETEComponent(SetControlDrag);

/* These two GetText routines do it the cheap way, but they work */
pascal ComponentResult PETEGetText(PETEInst pi,PETEHandle ph,long start,long stop,Handle *into)
	CallPETEComponent(GetText);
pascal ComponentResult PETEGetTextPtr(PETEInst pi,PETEHandle ph,long start,long stop,Ptr into,long intoSize,long *size)
	CallPETEComponent(GetTextPtr);

/* Offsets of -1 mean the current selection */
/* Pass nil in textHandle or paraHandle if unneeded */
pascal ComponentResult PETEGetTextStyleScrap(PETEInst pi,PETEHandle ph,long start,long stop,Handle *textHandle,PETEStyleListHandle *styleHandle,PETEParaScrapHandle *paraHandle,PETEGetStyleScrapFlags *flags)
	CallPETEComponent(GetTextStyleScrap);

pascal long PETEGetTextLen(PETEInst pi,PETEHandle ph)
	CallPETEComponent(GetTextLen);

/* Don't modify this handle!!!!!! */
pascal ComponentResult PETEGetRawText(PETEInst pi,PETEHandle ph,Handle *theText)
	CallPETEComponent(GetRawText);

pascal ComponentResult PETEDuplicateStyleScrap(PETEInst pi,PETEStyleListHandle *styleHandle)
	CallPETEComponent(DuplicateStyleScrap);
pascal ComponentResult PETEDisposeTextScrap(PETEInst pi,Handle theText)
	CallPETEComponent(DisposeTextScrap);
pascal ComponentResult PETEDisposeStyleScrap(PETEInst pi,PETEStyleListHandle styleHandle)
	CallPETEComponent(DisposeStyleScrap);
pascal ComponentResult PETEDisposeParaScrap(PETEInst pi,PETEParaScrapHandle paraHandle)
	CallPETEComponent(DisposeParaScrap);

pascal ComponentResult PETEFindText(PETEInst pi,PETEHandle ph,Ptr text,long len,long start,long stop,long *found,ScriptCode script)
	CallPETEComponent(FindText);

/* Offsets of -1 mean change the current selection */
/* Offsets of -2 mean only change the current style; don't change any text */
pascal ComponentResult PETESetStyle(PETEInst pi,PETEHandle ph,long start,long stop,PETEStyleInfoPtr style,long validBits)
	CallPETEComponent(SetStyle);


/* Offset of -1 means the style at the current selection */
/* Offset of -2 means the current style */
/* Offset of -3 mean the default style */
pascal ComponentResult PETEGetStyle(PETEInst pi,PETEHandle ph,long offset,long *length,PETEStyleEntryPtr theStyle)
	CallPETEComponent(GetStyle);

/* The event parameter is ignored if what is not peeEvent */
pascal ComponentResult PETEEdit(PETEInst pi,PETEHandle ph,PETEEditEnum what,EventRecord *event)
	CallPETEComponent(Edit);

pascal ComponentResult PETESetRecalcState(PETEInst pi,PETEHandle ph,Boolean recalc)
	CallPETEComponent(SetRecalcState);

pascal ComponentResult PETEGetDocInfo(PETEInst pi,PETEHandle ph,PETEDocInfoPtr info)
	CallPETEComponent(GetDocInfo);

/* ParaIndex of -1 means for all paragraphs in current selection */
pascal ComponentResult PETESetParaInfo(PETEInst pi,PETEHandle ph,long paraIndex,PETEParaInfoPtr paraInfo,short validBits)
	CallPETEComponent(SetParaInfo);

/* Index of -1 means the paragraph at the start of the current selection */
/* Index of -3 mean the default paragraph info */
pascal ComponentResult PETEGetParaInfo(PETEInst pi,PETEHandle ph,long index,PETEParaInfoPtr info)
	CallPETEComponent(GetParaInfo);

/* Offset of -1 means the paragraph at the start of the current selection */
pascal ComponentResult PETEGetParaIndex(PETEInst pi,PETEHandle ph,long offset,long *index)
	CallPETEComponent(GetParaIndex);

/* Checks to see if selection has modification lock */
/* Offsets are adjusted to current text length */
pascal long PETESelectionLocked(PETEInst pi,PETEHandle ph,long start,long stop)
	CallPETEComponent(SelectionLocked);

/* Gives offset of the character hit by the point (local coordinates) */
pascal ComponentResult PETEPositionToOffset(PETEInst pi,PETEHandle ph,Point position,long *offset)
	CallPETEComponent(PositionToOffset);
/* Gives the top left position of the offset specified */
pascal ComponentResult PETEOffsetToPosition(PETEInst pi,PETEHandle ph,long offset,Point *position,LHPtr lineHeight)
	CallPETEComponent(OffsetToPosition);

/*Not yet implemented */
//pascal ComponentResult PETEGetNextOffset(PETEInst pi,PETEHandle ph,long *offset)
//	CallPETEComponent(GetNextOffset);

/* Gets offsets of identical run of label (masked with mask) starting at offset */
pascal ComponentResult PETEFindLabelRun(PETEInst pi,PETEHandle ph,long offset,long *start,long *end,unsigned short label,unsigned short mask)
	CallPETEComponent(FindLabelRun);

pascal ComponentResult PETEMarkDocDirty(PETEInst pi,PETEHandle ph,Boolean dirty)
	CallPETEComponent(MarkDocDirty);

pascal ComponentResult PETEHonorLock(PETEInst pi,PETEHandle ph,Byte honorLockBits)
	CallPETEComponent(HonorLock);

pascal ComponentResult PETEAllowUndo(PETEInst pi,PETEHandle ph,Boolean allow,Boolean clear)
	CallPETEComponent(AllowUndo);
pascal ComponentResult PETEPunctuateUndo(PETEInst pi,PETEHandle ph)
	CallPETEComponent(PunctuateUndo);
pascal ComponentResult PETESetUndo(PETEInst pi,PETEHandle ph,long start,long stop,PETEUndoEnum undoType)
	CallPETEComponent(SetUndo);
pascal ComponentResult PETEInsertUndo(PETEInst pi,PETEHandle ph,long start,long stop,PETEUndoEnum undoType,Boolean append)
	CallPETEComponent(InsertUndo);
pascal ComponentResult PETEClearUndo(PETEInst pi,PETEHandle ph)
	CallPETEComponent(ClearUndo);


/* Uses handle in last parameter if not nil. If nil, allocates a new handle */
pascal ComponentResult PETEConvertTEScrap(PETEInst pi,StScrpHandle teScrap,PETEStyleListHandle *styleHandle)
	CallPETEComponent(ConvertTEScrap);
pascal ComponentResult PETEConvertToTEScrap(PETEInst pi,PETEStyleListHandle styleHandle,StScrpHandle *teScrap)
	CallPETEComponent(ConvertToTEScrap);


pascal ComponentResult PETECursor(PETEInst pi,PETEHandle ph,Point localPt,RgnHandle localMouseRgn,EventRecord *theEvent)
	CallPETEComponent(Cursor);
/* Help not yet implemented */
//pascal ComponentResult PETEHelp(PETEInst pi,PETEHandle ph,Point localPt)
//	CallPETEComponent(Help);

pascal long PETEGetMemInfo(PETEInst pi,PETEHandle ph)
	CallPETEComponent(GetMemInfo);

pascal ComponentResult PETESetCallback(PETEInst pi,PETEHandle ph,ProcPtr theProc,PETECallbackEnum procType)
	CallPETEComponent(SetCallback);
pascal ComponentResult PETEGetCallback(PETEInst pi,PETEHandle ph,ProcPtr *theProc,PETECallbackEnum procType)
	CallPETEComponent(GetCallback);

pascal ComponentResult PETEPrintSetup(PETEInst pi,PETEHandle ph)
	CallPETEComponent(PrintSetup);
pascal ComponentResult PETEPrintSelectionSetup(PETEInst pi,PETEHandle ph,long *paraIndex,long *lineIndex)
	CallPETEComponent(PrintSelectionSetup);
pascal ComponentResult PETEPrintPage(PETEInst pi,PETEHandle ph,CGrafPtr printPort,Rect *destRect,long *paraIndex,long *lineIndex)
	CallPETEComponent(PrintPage);
pascal ComponentResult PETEPrintCleanup(PETEInst pi,PETEHandle ph)
	CallPETEComponent(PrintCleanup);

/* If ph is nil, then the font or style is used globally. */
pascal ComponentResult PETESetDefaultFont(PETEInst pi,PETEHandle ph,PETEDefaultFontPtr defaultFont)
	CallPETEComponent(SetDefaultFont);
pascal ComponentResult PETESetLabelStyle(PETEInst pi,PETEHandle ph,PETELabelStylePtr labelStyle)
	CallPETEComponent(SetLabelStyle);
pascal ComponentResult PETESetDefaultColor(PETEInst pi,PETEHandle ph,RGBColor *defaultColor)
	CallPETEComponent(SetDefaultColor);
pascal ComponentResult PETESetDefaultBGColor(PETEInst pi,PETEHandle ph,RGBColor *defaultColor)
	CallPETEComponent(SetDefaultBGColor);

/* If ph is nil, use global defaults. If style is nil, use default style. */
pascal ComponentResult PETECompareStyles(PETEInst pi,PETEHandle ph,PETEStyleEntryPtr style1,PETEStyleEntryPtr style2,long validBits,Boolean printing,long *diffBits)
	CallPETEComponent(CompareStyles);

/* If ph is nil, then the global default font will be used, if any. */
pascal ComponentResult PETEStyleToFont(PETEInst pi,PETEHandle ph,PETETextStylePtr textStyle,short *fontID)
	CallPETEComponent(StyleToFont);

pascal ComponentResult PETEAllowIntelligentEdit(PETEInst pi,Boolean allow)
	CallPETEComponent(AllowIntelligentEdit);

pascal ComponentResult PETESelectGraphic(PETEInst pi,PETEHandle ph,long offset)
	CallPETEComponent(SelectGraphic);

pascal ComponentResult PETEGetSystemScrap(PETEInst pi,Handle *textScrap,PETEStyleListHandle *styleScrap,PETEParaScrapHandle *paraScrap)
	CallPETEComponent(GetSystemScrap);

pascal ComponentResult PETESetMemFail(PETEInst pi,Boolean *canFail)
	CallPETEComponent(SetMemFail);

pascal ComponentResult PETEFindStyle(PETEInst pi,PETEHandle ph,long startOffset,long *offset,PETEStyleInfoPtr theStyle,PETEStyleEnum validBits)
	CallPETEComponent(FindStyle);

pascal ComponentResult PETEDebugMode(PETEInst pi,Boolean debug)
	CallPETEComponent(DebugMode);

pascal ComponentResult PETELiveScroll(PETEInst pi,Boolean live)
	CallPETEComponent(LiveScroll);

pascal ComponentResult PETESetExtraHeight(PETEInst pi,PETEHandle ph,short height)
	CallPETEComponent(SetExtraHeight);

pascal ComponentResult PETEGetWord(PETEInst pi,PETEHandle ph,long offset,Boolean leadingEdge,long *startOffset,long *endOffset,Boolean *ws,short *charType)
	CallPETEComponent(GetWord);

pascal ComponentResult PETESetLabelCopyMask(PETEInst pi,Byte mask)
	CallPETEComponent(SetLabelCopyMask);

pascal ComponentResult PETEGetDebugStyleScrap(PETEInst pi,PETEHandle ph,long paraIndex,PETEStyleListHandle *styleHandle)
	CallPETEComponent(GetDebugStyleScrap);

pascal ComponentResult PETEAutoScrollTicks(PETEInst pi,unsigned long ticks)
	CallPETEComponent(AutoScrollTicks);

pascal ComponentResult PETEAnchoredSelection(PETEInst pi,Boolean anchored)
	CallPETEComponent(AnchoredSelection);

pascal ComponentResult PETEDocCheck(PETEInst pi,PETEHandle ph,Boolean checkNil,Boolean checkCorrupt)
	CallPETEComponent(DocCheck);

pascal ComponentResult PETEGraphicTextWidth(PETEInst pi,PETEHandle ph,long start,long stop,PETEGraphicStyleHandle styles,short *width)
	CallPETEComponent(GraphicTextWidth);
/* bounds.bottom is updated with actual value */
pascal ComponentResult PETEGraphicTextHeight(PETEInst pi,PETEHandle ph,long start,long stop,PETEGraphicStyleHandle styles,PETEGraphicLSTable *lineStarts,Rect *bounds)
	CallPETEComponent(GraphicTextHeight);
/* bounds.bottom is updated with actual value */
pascal ComponentResult PETEGraphicTextDraw(PETEInst pi,PETEHandle ph,long start,long stop,PETEGraphicStyleHandle styles,PETEGraphicLSTable *lineStarts,Rect *bounds)
	CallPETEComponent(GraphicTextDraw);

pascal ComponentResult PETEGraphicTextCreateStyle(PETEInst pi,PETEHandle ph,PETEGraphicStyleHandle *styles)
	CallPETEComponent(GraphicTextCreateStyle);
pascal ComponentResult PETEGraphicDisposeStyle(PETEInst pi,PETEHandle ph,PETEGraphicStyleHandle styles)
	CallPETEComponent(GraphicTextDisposeStyle);
pascal ComponentResult PETEGraphicCloneStyle(PETEInst pi,PETEHandle ph,PETEGraphicStyleHandle *styles)
	CallPETEComponent(GraphicTextCloneStyle);
pascal ComponentResult PETEGraphicTextSetStyle(PETEInst pi,PETEHandle ph,long start,long stop,PETEStyleInfoPtr style,long validBits,PETEGraphicStyleHandle styles)
	CallPETEComponent(GraphicTextSetStyle);

pascal ComponentResult PETEForceRecalc(PETEInst pi,PETEHandle ph,long start,long stop)
	CallPETEComponent(ForceRecalc);

pascal ComponentResult PETEUseScrap(PETEInst globals,ScrapRef scrap);

#if !defined(PETECOMPONENT_CFILE) && TARGET_RT_MAC_CFM
#include "petecfm.h"
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif

#endif // rez

#endif // PETE_H