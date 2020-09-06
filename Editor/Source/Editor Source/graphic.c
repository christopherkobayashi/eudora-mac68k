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

OSErr CallGraphic(DocumentInfoHandle docInfo, PETEGraphicInfoHandle graphicInfo, long offset, PETEGraphicMessage message, void *data)
{
	OSErr errCode;
	CGrafPtr oldPort;
	GDHandle oldGDevice;
	
	if((graphicInfo == nil) || ((**graphicInfo).itemProc == nil)) {
		return invalidHandler;
	}
	if(!GraphicInList(docInfo ? (**docInfo).globals : nil, graphicInfo))
	{
		return invalidHandler;
	}
	if (docInfo) (**docInfo).flags.inGraphicCall = true;
	GetGWorld(&oldPort, &oldGDevice);
	errCode = CallPETEGraphicHandlerProc((**graphicInfo).itemProc, docInfo, graphicInfo, offset, message, data);
	SetGWorld(oldPort, oldGDevice);
	if (docInfo) (**docInfo).flags.inGraphicCall = false;
	return errCode;
}

Boolean GraphicInList(PETEGlobalsHandle globals, PETEGraphicInfoHandle graphicInfo)
{
	Boolean in;
	static PETEGlobalsHandle lastGlobals;
	
	if(!globals) globals = lastGlobals;
	else lastGlobals = globals;
	
	if(!(in = (graphicInfo == nil)))
	{
		if((in = (*graphicInfo != nil)))
		{
			if(!(in = ((**graphicInfo).itemProc == nil)))
			{
				PETEGraphicHandlerProcPtr *graphicProcs = (**globals).graphicProcs;
				
				while(*graphicProcs != nil)
				{
					if((in = ((**graphicInfo).itemProc == *graphicProcs++)))
						break;
				}
			}
		}
	}
	if(!in && (**globals).flags.debugMode)
		DebugStr("\pHey! What kind of graphic is *that* supposed to be?");
	return in;
}

OSErr MyCallGraphicHit(DocumentInfoHandle docInfo, long offset, EventRecord *theEvent)
{
	OSErr errCode;
	PETEPortInfo savedPortInfo;
	RgnHandle scratchRgn;
	Rect viewRect;
	
	if((**docInfo).selectedGraphic == nil) {
		return errAENoUserSelection;
	}
	
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	viewRect = (**docInfo).viewRect;
	GetClip(scratchRgn);
	ClipRect(&viewRect);
	(**docInfo).flags.dragActive = true;
	errCode = CallGraphic(docInfo, (**docInfo).selectedGraphic, offset >= 0L ? offset : (**docInfo).selStart.offset, peGraphicHit, theEvent);
	(**docInfo).flags.dragActive = false;
	(**docInfo).flags.dragHasLeft = false;
	SetClip(scratchRgn);
	ResetPortInfo(&savedPortInfo);
	return errCode;
}

OSErr SelectGraphic(DocumentInfoHandle docInfo, long offset)
{
	OSErr errCode;
	SelData selection;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	EventRecord theEvent /* = {nullEvent} */;
	
	Zero(theEvent);
	
	if(offset < 0L) {
		return errOffsetInvalid;
	}
	
	if(offset >= (**docInfo).textLen) {
		offset = (**docInfo).textLen - 1L;
	}
	
	selection.offset = offset;
	selection.paraIndex = ParagraphIndex(docInfo, offset);
	selection.lastLine = false;
	selection.leadingEdge = true;
	errCode = GetSelectionStyle(docInfo, &selection, &tempStyleRun, &offset, nil);
	if(errCode != noErr) {
		return errCode;
	}
	if(!tempStyleRun.srIsGraphic) {
		return errOffsetInvalid;
	}
	selection.offset -= offset;
	GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	if((**docInfo).selectedGraphic != nil) {
		if((**docInfo).selStart.offset != selection.offset) {
			MyCallGraphicHit(docInfo, kPETECurrentSelection, nil);
		} else {
			return noErr;
		}
	}
	(**docInfo).selectedGraphic = tempStyle.stInfo.graphicStyle.graphicInfo;
	(**docInfo).selEnd = (**docInfo).selStart = selection;
	(**docInfo).selEnd.offset += tempStyleRun.srStyleLen;
	CalcSelectionRgn(docInfo, false);
	MyCallGraphicHit(docInfo, kPETECurrentSelection, &theEvent);
	return noErr;
}

void OffsetIdleGraphics(DocumentInfoHandle docInfo, long startChar, long offset)
{
	IdleGraphicHandle idleGraphics = (**docInfo).idleGraphics;
	long graphicIndex = InlineGetHandleSize((Handle)idleGraphics) / sizeof(IdleGraphic);
	long newOffset = startChar;
	
	while(--graphicIndex >= 0L) {
		if((*idleGraphics)[graphicIndex].offset >= startChar) {
			(*idleGraphics)[graphicIndex].offset += offset;
			if((*idleGraphics)[graphicIndex].offset < startChar) {
				goto DeleteIt;
			}
		} else if((*idleGraphics)[graphicIndex].offset + (*idleGraphics)[graphicIndex].length > startChar) {
			if((*idleGraphics)[graphicIndex].offset < newOffset) {
				newOffset = (*idleGraphics)[graphicIndex].offset;
			}
	DeleteIt :
			Munger((Handle)idleGraphics, graphicIndex * sizeof(IdleGraphic), nil, sizeof(IdleGraphic), (Ptr)-1L, 0L);
		}
	}
	NewIdleGraphics(docInfo, newOffset, startChar + offset);
}

void DeleteIdleGraphics(DocumentInfoHandle docInfo, long startChar, long endChar)
{
	IdleGraphicHandle idleGraphics = (**docInfo).idleGraphics;
	long graphicIndex = InlineGetHandleSize((Handle)idleGraphics) / sizeof(IdleGraphic);
	long startOffset, endOffset;
	
	while(--graphicIndex >= 0L) {
		startOffset = (*idleGraphics)[graphicIndex].offset;
		endOffset = startOffset + (*idleGraphics)[graphicIndex].length;
		if((startOffset >= startChar && startOffset < endChar) || (endOffset <= endChar && endOffset > startChar)) {
			Munger((Handle)idleGraphics, graphicIndex * sizeof(IdleGraphic), nil, sizeof(IdleGraphic), (Ptr)-1L, 0L);
		}
	}
}

void NewIdleGraphics(DocumentInfoHandle docInfo, long startChar, long endChar)
{
	IdleGraphicHandle idleGraphics = (**docInfo).idleGraphics;
	IdleGraphic tempGraphic;
	PETEStyleEntry theStyle;
	long offset, length;
	
	offset = startChar;
	while(offset < endChar) {
		if((GetStyleFromOffset(docInfo, offset, &length, &theStyle) == noErr) && (length > 0L)) {
			if(theStyle.psGraphic && (theStyle.psStyle.graphicStyle.graphicInfo != nil) && (**theStyle.psStyle.graphicStyle.graphicInfo).wantsEvents) {
				tempGraphic.offset = theStyle.psStartChar;
				tempGraphic.length = length;
				tempGraphic.graphicInfo = theStyle.psStyle.graphicStyle.graphicInfo;
				tempGraphic.filler = 0L;
				PtrAndHand(&tempGraphic, (Handle)idleGraphics, sizeof(IdleGraphic));
			}
		} else {
			break;
		}
		offset = theStyle.psStartChar + length;
	}	
}

void DisposeScrapGraphics(PETEStyleListHandle styleScrap, long startIndex, long endIndex, Boolean killPictures)
{
	long styleIndex;
	GraphicInfoHandle graphicInfo;
	
	if(styleScrap != nil) {
		if(startIndex < 0L) {
			styleIndex = 0L;
		}
		styleIndex = (InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry)) - 1L;
		if(styleIndex <= 0L) {
			return;
		}
		if((endIndex < 0L) || (endIndex > styleIndex)) {
			endIndex = styleIndex;
		}
		for(styleIndex = startIndex; styleIndex <= endIndex; ++styleIndex) {
			if((*styleScrap)[styleIndex].psGraphic && ((graphicInfo = ((*styleScrap)[styleIndex].psStyle.graphicStyle.graphicInfo)) != nil) && !(**graphicInfo).cloneOnlyText) {
				if((**graphicInfo).itemProc != nil) {
					CallGraphic(nil, graphicInfo, kPETEOffsetUnknown, peGraphicRemove, nil);
				} else if(killPictures || (--(**(PictGraphicInfoHandle)graphicInfo).counter == 0L)) {
					Handle tempHandle = (Handle)(**(PictGraphicInfoHandle)graphicInfo).pict;
					if(!(HGetState(tempHandle) & (1 << 5))) {
						DisposeHandle(tempHandle);
					}
					DisposeHandle((Handle)graphicInfo);
				}
			}
		}
	}
}

OSErr CloneScrapGraphics(PETEStyleListHandle styleScrap)
{
	OSErr errCode;
	PETEGraphicInfoHandle graphicInfo;
	long styleIndex;
	
	for(errCode = noErr, styleIndex = InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry); errCode == noErr && --styleIndex >= 0L; ) {
		if((*styleScrap)[styleIndex].psGraphic && ((graphicInfo = (*styleScrap)[styleIndex].psStyle.graphicStyle.graphicInfo) != nil) && !(**graphicInfo).cloneOnlyText) {
			if((**graphicInfo).itemProc != nil) {
				errCode = CallGraphic(nil, graphicInfo, kPETEOffsetUnknown, peGraphicClone, &graphicInfo);
			} else {
				++(**(PictGraphicInfoHandle)graphicInfo).counter;
			}
			(*styleScrap)[styleIndex].psStyle.graphicStyle.graphicInfo = graphicInfo;
			if(errCode != noErr) {
				DisposeScrapGraphics(styleScrap, ++styleIndex, InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry), false);
				break;
			}
		}
	}
	return errCode;
}

long GraphicCount(LongSTTable theStyleTable, GraphicInfoHandle graphicInfo);
long GraphicCount(LongSTTable theStyleTable, GraphicInfoHandle graphicInfo)
{
	LongSTElement tempStyle;
	long styleIndex, graphicCount;
	
	if(graphicInfo == nil) {
		return -1L;
	} else {
		styleIndex = NumStyles(theStyleTable);
		for(graphicCount = 0L; --styleIndex >= 0L; ) {
			GetStyle(&tempStyle, theStyleTable, styleIndex);
			if((tempStyle.stCount > 0L) && (tempStyle.stInfo.graphicStyle.graphicInfo == graphicInfo)) {
				++graphicCount;
			}
		}
		return graphicCount;
	}
}

void DeleteGraphicFromStyle(LongSTTable theStyleTable, long styleIndex)
{
	LongSTElement theStyle;
	
	GetStyle(&theStyle, theStyleTable, styleIndex);
	if(theStyle.stIsGraphic && (GraphicCount(theStyleTable, theStyle.stInfo.graphicStyle.graphicInfo) == 0L)) {
		if((**theStyle.stInfo.graphicStyle.graphicInfo).itemProc != nil) {
			CallGraphic((**theStyleTable).docInfo, theStyle.stInfo.graphicStyle.graphicInfo, kPETEOffsetUnknown, peGraphicRemove, nil);
		} else if(--(**(PictGraphicInfoHandle)theStyle.stInfo.graphicStyle.graphicInfo).counter == 0L) {
			Handle tempHandle = (Handle)(**(PictGraphicInfoHandle)theStyle.stInfo.graphicStyle.graphicInfo).pict;
			if(!(HGetState(tempHandle) & (1 << 5))) {
				DisposeHandle(tempHandle);
			}
			DisposeHandle((Handle)theStyle.stInfo.graphicStyle.graphicInfo);
		}
	}
}