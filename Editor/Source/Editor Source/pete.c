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

#include "petepch.h"

#pragma export on

#define PHCHECK(checkNil,checkCorrupt) \
	do {	\
		ComponentResult errCode = DocCheckWithDebug(pi,ph,checkNil,checkCorrupt); \
		if(errCode != noErr) {	\
			return errCode;	\
		}	\
	} while(false)

ComponentResult DocCheckWithDebug(PETEInst pi,PETEHandle ph,Boolean checkNil,Boolean checkCorrupt);
ComponentResult DocCheckWithDebug(PETEInst pi,PETEHandle ph,Boolean checkNil,Boolean checkCorrupt)
{
	ComponentResult errCode = PETEDocCheck(pi,ph,checkNil,checkCorrupt);
	if((**pi).flags.debugMode) {
		if(errCode == errAECorruptData) {
			DebugStr("\pCorrupt ph!");
		} else if (errCode == nilHandleErr) {
			DebugStr("\pNil ph!");
		}
	}
	return errCode;
}

OSErr PETEInit(PETEInst *pi, PETEGraphicHandlerProcPtr *graphics)
{
	OSErr err = InitializeGlobals(pi);
	
	if(!err) (***pi).graphicProcs = graphics;
	return err;
}

OSErr PETECleanup(PETEInst pi)
{
	return DisposeGlobals(pi);
}

pascal ComponentResult PETECreate(PETEInst pi,PETEHandle *ph,PETEDocInitInfoPtr initInfo)
{
	return CreateNewDocument(pi, ph, initInfo);
}

pascal ComponentResult PETEDispose(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,false);
	return DisposeDocument(pi, ph);
}

pascal ComponentResult PETESetRefCon(PETEInst pi,PETEHandle ph,long refCon)
{
	PHCHECK(true,false);
	(**ph).refCon = refCon;
	return noErr;
}

pascal long PETEGetRefCon(PETEInst pi,PETEHandle ph)
{
	if(PETEDocCheck(pi, ph, true, false)) {
		if((**pi).flags.debugMode) {
			DebugStr("\pBad ph!");
		}
		return 0L;
	} else {
		return (**ph).refCon;
	}
}

pascal ComponentResult PETEInsertPara(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Handle text,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return InsertParagraph(ph, beforePara, paraInfo, (Ptr)text, -1L, 0L, styles);
}

pascal ComponentResult PETEInsertParaPtr(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Ptr text,long len,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return InsertParagraph(ph, beforePara, paraInfo, text, len, -1L, styles);
}

pascal ComponentResult PETEInsertParaHandle(PETEInst pi,PETEHandle ph,long beforePara,PETEParaInfoPtr paraInfo,Handle text,long len,long hOffset,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return InsertParagraph(ph, beforePara, paraInfo, (Ptr)text, len, hOffset, styles);
}

pascal ComponentResult PETEInsertParaBreak(PETEInst pi,PETEHandle ph,long offset)
{
	PHCHECK(true,true);
	return InsertParagraphBreak(ph, offset);
}

pascal ComponentResult PETEDeletePara(PETEInst pi,PETEHandle ph,long index)
{
	PHCHECK(true,true);
	if(index < 0L || index >= (**ph).paraCount) return invalidIndexErr;
	return DeleteParagraph(ph, index);
}

pascal ComponentResult PETEDeleteParaBreak(PETEInst pi,PETEHandle ph,long index)
{
	PHCHECK(true,true);
	if(index < 0L || index + 1 >= (**ph).paraCount) return invalidIndexErr;
	return DeleteParagraphBreak(ph, index);
}

pascal ComponentResult PETEInsertText(PETEInst pi,PETEHandle ph,long offset,Handle text,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return DoInsertText(ph, offset, (Ptr)text, -1L, 0L, styles);
}

pascal ComponentResult PETEInsertTextPtr(PETEInst pi,PETEHandle ph,long offset,Ptr text,long len,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return DoInsertText(ph, offset, text, len, -1L, styles);
}

pascal ComponentResult PETEInsertTextHandle(PETEInst pi,PETEHandle ph,long offset,Handle text,long len,long hOffset,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return DoInsertText(ph, offset, (Ptr)text, len, hOffset, styles);
}

pascal ComponentResult PETEInsertTextScrap(PETEInst pi,PETEHandle ph,long offset,Handle textScrap,PETEStyleListHandle styleScrap,PETEParaScrapHandle paraScrap,Boolean dispose)
{
	PHCHECK(true,true);
	return PasteText(ph, offset, (StringHandle)textScrap, styleScrap, paraScrap, false, dispose);
}

pascal ComponentResult PETEReplaceText(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Handle text,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return MyReplaceText(ph, offset, replaceLen, (Ptr)text, -1L, 0L, styles);
}

pascal ComponentResult PETEReplaceTextPtr(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Ptr text,long len,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return MyReplaceText(ph, offset, replaceLen, text, len, -1L, styles);
}

pascal ComponentResult PETEReplaceTextHandle(PETEInst pi,PETEHandle ph,long offset,long replaceLen,Handle text,long len,long hOffset,PETEStyleListHandle styles)
{
	PHCHECK(true,true);
	return MyReplaceText(ph, offset, replaceLen, (Ptr)text, len, hOffset, styles);
}

pascal ComponentResult PETEEventFilter(PETEInst pi,EventRecord *event)
{
	MyEventFilter(pi, event);
	return noErr;
}

pascal long PETEMenuSelectFilter(PETEInst pi,long menuResult)
{
	return MyMenuSelectFilter(pi, menuResult);
}

pascal ComponentResult PETEDraw(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return UpdateDocument(ph);
}

pascal ComponentResult PETEActivate(PETEInst pi,PETEHandle ph,Boolean activeText,Boolean activeScrolls)
{
	PHCHECK(true,true);
	return MyDoActivate(ph, activeText, activeScrolls);
}

pascal ComponentResult PETESizeDoc(PETEInst pi,PETEHandle ph,short horizontal,short vertical)
{
	PHCHECK(true,true);
	ResizeDocRect(ph, horizontal, vertical);
	return noErr;
}

pascal ComponentResult PETEMoveDoc(PETEInst pi,PETEHandle ph,short horizontal,short vertical)
{
	PHCHECK(true,true);
	MoveDocRect(ph, horizontal, vertical);
	return noErr;
}

pascal ComponentResult PETEChangeDocWidth(PETEInst pi,PETEHandle ph,short docWidth, Boolean pinMargins)
{
	PHCHECK(true,true);
	ChangeDocWidth(ph, docWidth, pinMargins);
	return noErr;
}

pascal ComponentResult PETEScroll(PETEInst pi,PETEHandle ph, short horizontal, short vertical)
{
	PHCHECK(true,true);
	return DoScroll(ph, horizontal, vertical);
}

pascal ComponentResult PETESelect(PETEInst pi,PETEHandle ph,long start,long stop)
{
	PHCHECK(true,true);
	return SetSelection(ph, start, stop);
} 

pascal ComponentResult PETEDragTrackingHandler(PETEInst pi,PETEHandle ph,DragTrackingMessage message,DragReference theDragRef)
{
	PHCHECK(true,true);
	return MyTrackingHandler(message, nil, ph, theDragRef);
}

pascal ComponentResult PETEDragReceiveHandler(PETEInst pi,PETEHandle ph,DragReference theDragRef)
{
	PHCHECK(true,true);
	return MyReceiveHandler(nil, ph, theDragRef);
}

pascal ComponentResult PETESetControlDrag(PETEInst pi,PETEHandle ph,Boolean useControl)
{
	PHCHECK(true,true);
	return SetControlDrag(ph, useControl);
}

pascal ComponentResult PETEGetText(PETEInst pi,PETEHandle ph, long start, long stop, Handle *into)
{
	PHCHECK(true,true);
	return GetText(ph, start, stop, (Ptr)into, 0L, (long *)-1L);
}

pascal ComponentResult PETEGetTextPtr(PETEInst pi,PETEHandle ph, long start, long stop, Ptr into,long intoSize, long *size)
{
	PHCHECK(true,true);
	return GetText(ph, start, stop, into, intoSize, size);
}

pascal ComponentResult PETEGetTextStyleScrap(PETEInst pi,PETEHandle ph,long start,long stop,Handle *textHandle,PETEStyleListHandle *styleHandle,PETEParaScrapHandle *paraHandle,PETEGetStyleScrapFlags *flags)
{
	PHCHECK(true,true);
	return GetTextStyleScrap(ph, start, stop, textHandle, styleHandle, paraHandle, flags->allParas, flags->textTempPref, flags->styleTempPref, flags->paraTempPref, flags->clearLock);
}

pascal ComponentResult PETEGetRawText(PETEInst pi,PETEHandle ph,Handle *theText)
{
	PHCHECK(true,false);
	*theText = GetDocTextHandle(ph);
	return noErr;
}

pascal long PETEGetTextLen(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return (**ph).textLen;
}

pascal ComponentResult PETEDuplicateStyleScrap(PETEInst pi,PETEStyleListHandle *styleHandle)
{
#pragma unused(pi)
	OSErr err;
	
	if((err = MyHandToHand((Handle *)styleHandle, hndlTemp)) == noErr)
		err = CloneScrapGraphics(*styleHandle);
	return err;
}

pascal ComponentResult PETEDisposeTextScrap(PETEInst pi,Handle theText)
{
#pragma unused(pi)
	DisposeHandle(theText);
	return MemError();
}

pascal ComponentResult PETEDisposeStyleScrap(PETEInst pi,PETEStyleListHandle styleHandle)
{
#pragma unused(pi)
	DisposeScrapGraphics(styleHandle, 0L, -1L, false);
	DisposeHandle((Handle)styleHandle);
	return MemError();
}

pascal ComponentResult PETEDisposeParaScrap(PETEInst pi,PETEParaScrapHandle paraHandle)
{
#pragma unused(pi)
	DisposeHandle((Handle)paraHandle);
	return MemError();
}

pascal ComponentResult PETEFindText(PETEInst pi,PETEHandle ph,Ptr text,long len,long start,long stop,long *found,ScriptCode script)
{
	PHCHECK(true,true);
	return FindMyText(ph, text, len, start, stop, found, script);
}

pascal ComponentResult PETESetStyle(PETEInst pi,PETEHandle ph,long start,long stop,PETEStyleInfoPtr style,long validBits)
{
	PHCHECK(true,true);
	return ChangeStyleRange(ph, start, stop, style, validBits);
}

pascal ComponentResult PETEGetStyle(PETEInst pi,PETEHandle ph,long offset,long *length,PETEStyleEntryPtr theStyle)
{
	PHCHECK(true,true);
	return GetStyleFromOffset(ph, offset, length, theStyle);
}

pascal ComponentResult PETEEdit(PETEInst pi,PETEHandle ph,PETEEditEnum what,EventRecord *event)
{
	PHCHECK(true,true);
	return MyHandleEditCall(ph, what, event);
}

pascal ComponentResult PETESetRecalcState(PETEInst pi,PETEHandle ph,Boolean recalc)
{
	PHCHECK(true,true);
	return SetRecalcState(ph, recalc);
}

pascal ComponentResult PETEGetDocInfo(PETEInst pi,PETEHandle ph,PETEDocInfoPtr info)
{
	PHCHECK(true,true);
	return GetDocInfo(ph, info);
}

pascal ComponentResult PETESetParaInfo(PETEInst pi,PETEHandle ph,long paraIndex,PETEParaInfoPtr paraInfo,short validBits)
{
	PHCHECK(true,true);
	return SetParagraphInfo(ph, paraIndex, paraInfo, validBits);
}

pascal ComponentResult PETEGetParaInfo(PETEInst pi,PETEHandle ph,long index,PETEParaInfoPtr info)
{
	PHCHECK(true,true);
	return GetParaInfo(ph, index, info);
}

pascal ComponentResult PETEGetParaIndex(PETEInst pi,PETEHandle ph,long offset,long *index)
{
	PHCHECK(true,true);
	return GetParaIndex(ph, offset, index);
}

pascal long PETESelectionLocked(PETEInst pi,PETEHandle ph, long start, long stop)
{
	PHCHECK(true,true);
	return IsSelectionLocked(ph, start, stop);
}

pascal ComponentResult PETEPositionToOffset(PETEInst pi,PETEHandle ph,Point position,long *offset)
{
	PHCHECK(true,true);
	return PtToOffset(ph, position, offset);
}

pascal ComponentResult PETEOffsetToPosition(PETEInst pi,PETEHandle ph,long offset,Point *position, LHPtr lineHeight)
{
	PHCHECK(true,true);
	return OffsetToPt(ph, offset, position, lineHeight);
}

pascal ComponentResult PETEFindLabelRun(PETEInst pi,PETEHandle ph,long offset,long *start,long *end,unsigned short label,unsigned short mask)
{
	PHCHECK(true,true);
	return FindLabelRun(ph, offset, start, end, label, mask);
}

pascal ComponentResult PETEMarkDocDirty(PETEInst pi,PETEHandle ph,Boolean dirty)
{
	PHCHECK(true,true);
	return SetDirty(ph, -1L, -1L, dirty);
}

pascal ComponentResult PETEHonorLock(PETEInst pi,PETEHandle ph,Byte honorLockBits)
{
	PHCHECK(true,true);
	return SetHonorLock(ph, honorLockBits);
}

pascal ComponentResult PETEAllowUndo(PETEInst pi,PETEHandle ph,Boolean allow,Boolean clear)
{
	PHCHECK(true,true);
	return SetUndoFlag(ph, allow, clear);
}

pascal ComponentResult PETEPunctuateUndo(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return PunctuateUndo(ph);
}

pascal ComponentResult PETESetUndo(PETEInst pi,PETEHandle ph,long start,long stop,PETEUndoEnum undoType)
{
	Boolean wasIgnoreLock;
	ComponentResult errCode;
	
	PHCHECK(true,true);
	wasIgnoreLock = (**ph).flags.ignoreModLock;
	if(!wasIgnoreLock) {
		(**ph).flags.ignoreModLock = true;
	}
	switch(undoType) {
		case peUndoStyleAndPara :
		case peRedoStyleAndPara :
		case peUndoPara :
		case peRedoPara :
		case peUndoStyle :
		case peRedoStyle :
			errCode = SetParaStyleUndo(ph, start, stop, undoType);
			break;
		default :
			errCode = SetDeleteUndo(ph, start, stop, undoType);
	}
	if(!wasIgnoreLock) {
		(**ph).flags.ignoreModLock = false;
	}
	return errCode;
}

pascal ComponentResult PETEInsertUndo(PETEInst pi,PETEHandle ph,long start,long stop,PETEUndoEnum undoType,Boolean append)
{
	PHCHECK(true,true);
	SetInsertUndo(ph, start, stop - start, -1L, -1L, undoType, append);
	return noErr;
}

pascal ComponentResult PETEClearUndo(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	ClearUndo(ph);
	return noErr;
}

pascal ComponentResult PETEConvertTEScrap(PETEInst pi,StScrpHandle teScrap,PETEStyleListHandle *styleHandle)
{
#pragma unused(pi)
	return TEScrapToPETEStyle(teScrap, styleHandle);
}

pascal ComponentResult PETEConvertToTEScrap(PETEInst pi,PETEStyleListHandle styleHandle,StScrpHandle *teScrap)
{
	return PETEStyleToTEScrap(pi, nil, styleHandle, teScrap);
}

pascal ComponentResult PETECursor(PETEInst pi,PETEHandle ph, Point localPt, RgnHandle localMouseRgn, EventRecord *theEvent)
{
	PHCHECK(true,true);
	SetCorrectCursor(ph, localPt, localMouseRgn, theEvent);
	return noErr;
}

pascal long PETEGetMemInfo(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return GetDocSize(ph);
}

pascal ComponentResult PETESetCallback(PETEInst pi,PETEHandle ph,ProcPtr theProc,PETECallbackEnum procType)
{
	PHCHECK(false,true);
	return SetCallbackRoutine(pi, ph, theProc, procType);
}

pascal ComponentResult PETEGetCallback(PETEInst pi,PETEHandle ph,ProcPtr *theProc,PETECallbackEnum procType)
{
	return GetCallbackRoutine(pi, ph, theProc, procType);
}

pascal ComponentResult PETEPrintSetup(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return InitForPrinting(ph);
}

pascal ComponentResult PETEPrintSelectionSetup(PETEInst pi,PETEHandle ph,long *paraIndex,long *lineIndex)
{
	PHCHECK(true,true);
	return PrintSelectionSetup(ph, paraIndex, lineIndex);
}

pascal ComponentResult PETEPrintPage(PETEInst pi,PETEHandle ph,CGrafPtr printPort,Rect *destRect,long *paraIndex,long *lineIndex)
{
	PHCHECK(true,true);
	return PrintPage(ph, printPort, destRect, paraIndex, lineIndex);
}

pascal ComponentResult PETEPrintCleanup(PETEInst pi,PETEHandle ph)
{
	PHCHECK(true,true);
	return CleanupPrinting(ph);
}

pascal ComponentResult PETESetDefaultFont(PETEInst pi,PETEHandle ph,PETEDefaultFontPtr defaultFont)
{
	PHCHECK(false,true);
	return SetDefaultStyle(pi, ph, defaultFont);
}

pascal ComponentResult PETESetLabelStyle(PETEInst pi,PETEHandle ph,PETELabelStylePtr labelStyle)
{
	PHCHECK(false,true);
	return SetLabelStyle(pi, ph, labelStyle);
}

pascal ComponentResult PETESetDefaultColor(PETEInst pi,PETEHandle ph,RGBColor *defaultColor)
{
	PHCHECK(false,true);
	return SetDefaultColor(pi, ph, defaultColor);
}

pascal ComponentResult PETESetDefaultBGColor(PETEInst pi,PETEHandle ph,RGBColor *defaultColor)
{
	PHCHECK(false,true);
	return SetDefaultBGColor(pi, ph, defaultColor);
}

pascal ComponentResult PETECompareStyles(PETEInst pi,PETEHandle ph,PETEStyleEntryPtr style1,PETEStyleEntryPtr style2,long validBits,Boolean printing,long *diffBits)
{
	PHCHECK(false,true);
	return CompareStyles(pi, ph, style1, style2, validBits, printing, diffBits);
}

pascal ComponentResult PETEStyleToFont(PETEInst pi,PETEHandle ph,PETETextStylePtr textStyle,short *fontID)
{
	PHCHECK(false,true);
	*fontID = StyleToFont(pi, ph, textStyle, false);
	return noErr;
}

pascal ComponentResult PETEAllowIntelligentEdit(PETEInst pi,Boolean allow)
{
	(**pi).flags.noIntelligentEdit = !allow;
	return noErr;
}

pascal ComponentResult PETESelectGraphic(PETEInst pi,PETEHandle ph,long offset)
{
	PHCHECK(true,true);
	return SelectGraphic(ph, offset);
}

pascal ComponentResult PETEGetSystemScrap(PETEInst pi,Handle *textScrap,PETEStyleListHandle *styleScrap,PETEParaScrapHandle *paraScrap)
{
	return(GetClipContents(pi, (StringHandle *)textScrap, styleScrap, paraScrap));
}

pascal ComponentResult PETESetMemFail(PETEInst pi,Boolean *canFail)
{
	(**pi).memCanFail = canFail;
	return noErr;
}

pascal ComponentResult PETEFindStyle(PETEInst pi,PETEHandle ph,long startOffset,long *offset,PETEStyleInfoPtr theStyle,PETEStyleEnum validBits)
{
	PHCHECK(true,true);
	return(FindStyle(ph, startOffset, offset, theStyle, validBits));
}

pascal ComponentResult PETEDebugMode(PETEInst pi,Boolean debug)
{
	(**pi).flags.debugMode = debug;
	return noErr;
}

pascal ComponentResult PETELiveScroll(PETEInst pi,Boolean live)
{
	LiveScroll(pi, live);
	return noErr;
}

pascal ComponentResult PETESetExtraHeight(PETEInst pi,PETEHandle ph,short height)
{
	PHCHECK(true,true);
	(**ph).docHeight -= (**ph).extraHeight;
	(**ph).docHeight += ((**ph).extraHeight = height);
	return noErr;
}

pascal ComponentResult PETEGetWord(PETEInst pi,PETEHandle ph,long offset,Boolean leadingEdge,long *startOffset,long *endOffset,Boolean *ws,short *charType)
{
	PHCHECK(true,true);
	return GetWordFromOffset(ph, offset, leadingEdge, startOffset, endOffset, ws, charType);
}

pascal ComponentResult PETESetLabelCopyMask(PETEInst pi,Byte mask)
{
	(**pi).labelMask = mask;
	return noErr;
}

pascal ComponentResult PETEGetDebugStyleScrap(PETEInst pi,PETEHandle ph,long paraIndex,PETEStyleListHandle *styleHandle)
{
	PHCHECK(true,true);
	return GetDebugStyleScrap(ph, paraIndex,styleHandle);
}

pascal ComponentResult PETEAutoScrollTicks(PETEInst pi,unsigned long ticks)
{
	(**pi).autoScrollTicks = ticks;
	return noErr;
}

pascal ComponentResult PETEAnchoredSelection(PETEInst pi,Boolean anchored)
{
	(**pi).flags.anchoredSelection = anchored;
	return noErr;
}

pascal ComponentResult PETEDocCheck(PETEInst pi,PETEHandle ph,Boolean checkNil,Boolean checkCorrupt)
{
	long docIndex;
	
	if(ph == nil) {
		if(checkNil) {
			return nilHandleErr;
		} else {
			return noErr;
		}
	}
	docIndex = (InlineGetHandleSize((Handle)pi) - sizeof(PETEGlobals)) / sizeof(DocumentInfoHandle);
	while(--docIndex >= 0L) {
		if((**pi).docInfoArray[docIndex] == ph) {
			break;
		}
	}
	if(docIndex < 0L) {
		if((**pi).flags.debugMode) {
			DebugStr("\pph not a document!");
		}
		return tsmInvalidDocIDErr;
	}
	if(checkCorrupt && (**ph).flags.docCorrupt) {
		return errAECorruptData;
	}
	return noErr;
}

pascal ComponentResult PETEForceRecalc(PETEInst pi,PETEHandle ph,long start,long stop)
{
	PHCHECK(true,true);
	return ForceRecalc(ph, start, stop);
}

pascal ComponentResult PETEUseScrap(PETEInst pi,ScrapRef scrap)
{
#pragma unused(pi)
	UseThisScrap(scrap);
	return noErr;
}

#pragma export reset