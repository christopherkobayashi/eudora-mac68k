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

OSErr InitForPrinting(DocumentInfoHandle docInfo)
{
	OSErr errCode, memErr;
	PicHandle docPic;
	PrintingDataHandle printData;

	printData = (PrintingDataHandle)MyNewHandle(sizeof(PrintingData), &memErr, 0);
	if((errCode = memErr) == noErr) {
		errCode = CreateDocPict(docInfo, &docPic);
		if(errCode == noErr) {
			RecalcStyleHeights(nil, docInfo, true);
			(**printData).docPic = docPic;
			(**printData).oldWidth = (**docInfo).docWidth;
			(**printData).oldHPosition = (**docInfo).hPosition;
			(**printData).oldVPosition = (**docInfo).vPosition;
			(**printData).oldSelEnd = (**docInfo).selEnd;
			(**printData).oldDocFlags = (**docInfo).flags;
			(**printData).oldDocFlags.selectionDirty = true;
			(**printData).printSelection = false;

			SetEmptyRgn((**docInfo).hiliteRgn);
			(**docInfo).selEnd = (**docInfo).selStart;
			(**docInfo).flags.offscreenBitMap = false;
			(**docInfo).flags.caretOn = false;
			(**docInfo).flags.scrollsDirty = false;
			(**docInfo).flags.selectionDirty = false;
			(**docInfo).printData = printData;
		} else {
			DisposeHandle((Handle)printData);
		}
	}
	return errCode;
}

OSErr PrintSelectionSetup(DocumentInfoHandle docInfo, long *paraIndex, long *lineIndex)
{
	OSErr errCode;
	SelData selStart, selEnd;
	
	selEnd = (**(**docInfo).printData).oldSelEnd;
	errCode = OffsetToPosition(docInfo, &selEnd, false);
	if(errCode == noErr) {
		selStart = (**docInfo).selStart;
		errCode = OffsetToPosition(docInfo, &selStart, false);
	}
	if(errCode == noErr) {
		(**(**docInfo).printData).oldSelEnd = selEnd;
		*paraIndex = selStart.paraIndex;
		*lineIndex = selStart.lineIndex;
		(**(**docInfo).printData).printSelection = true;
	}
	return errCode;
}

OSErr CleanupPrinting(DocumentInfoHandle docInfo)
{
	PrintingDataHandle printData;
	Boolean scrolls, active;
	
	printData = (**docInfo).printData;
	(**docInfo).docWidth = (**printData).oldWidth;
	(**docInfo).hPosition = (**printData).oldHPosition;
	(**docInfo).vPosition = (**printData).oldVPosition;
	(**docInfo).selEnd = (**printData).oldSelEnd;
	scrolls = (**docInfo).flags.scrollsVisible;
	active = (**docInfo).flags.isActive;
	(**docInfo).flags = (**printData).oldDocFlags;
	(**docInfo).flags.scrollsVisible = scrolls;
	(**docInfo).flags.isActive = active;
	KillPicture((**printData).docPic);
	DisposeHandle((Handle)printData);
	(**docInfo).printData = nil;
	RecalcStyleHeights(nil, docInfo, false);
	ResetAndInvalidateDocument(docInfo);
	return noErr;
}

OSErr PrintPage(DocumentInfoHandle docInfo, CGrafPtr printPort, Rect *destRect, long *paraIndex, long *lineIndex)
{
	OSErr errCode, tempErr;
	Rect oldViewRect, tempViewRect;
	long vPosition, tempPosition;
	SelData selection;
	RgnHandle tempRgn;
	Boolean pastEnd, selectionEnd, formFeedFound;
	Handle tempTextHandle;
	long startOffset, index;
	PETEStyleEntry theStyle;
	LSTable lineStarts;

	errCode = CheckParagraphMeasure(docInfo, *paraIndex, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	lineStarts = (**(**docInfo).paraArray[*paraIndex].paraInfo).lineStarts;
	if(*lineIndex >= InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement)) {
		if(*paraIndex + 1L >= (**docInfo).paraCount) {
			selection.offset = startOffset = (**docInfo).textLen;
		} else {
			++*paraIndex;
			*lineIndex = 0L;
			errCode = CheckParagraphMeasure(docInfo, *paraIndex, true);
			if(errCode != noErr) {
				return errCode;
			}
			lineStarts = (**(**docInfo).paraArray[*paraIndex].paraInfo).lineStarts;
			selection.offset = startOffset = (*lineStarts)[*lineIndex].lsStartChar;
		}
	} else {
		selection.offset = startOffset = (*lineStarts)[*lineIndex].lsStartChar;
	}
	selection.paraIndex = *paraIndex;
	selection.lineIndex = *lineIndex;
	selection.leadingEdge = true;
	selection.lastLine = false;
	errCode = OffsetToPosition(docInfo, &selection, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	vPosition = selection.vPosition;

	selection.hPosition = 0L;
	selection.vPosition += RectHeight(destRect);

	if((**(**docInfo).printData).printSelection &&
	   (tempPosition = (**(**docInfo).printData).oldSelEnd.vPosition,
	    tempPosition += (**(**docInfo).printData).oldSelEnd.vLineHeight,
	    (selection.vPosition > tempPosition))) {

		selection = (**(**docInfo).printData).oldSelEnd;
		selectionEnd = true;
		pastEnd = true;
	} else {
		
		errCode = PositionToOffset(docInfo, &selection);
		if(errCode != noErr) {
			return errCode;
		}
		
		selectionEnd = false;
		pastEnd = (selection.offset == (**docInfo).textLen);
	}
	
	tempTextHandle = MyNewHandle(selection.offset - startOffset, &tempErr, hndlTemp);
	if((errCode = tempErr) != noErr) {
		return errCode;
	}
	
	errCode = LoadTextIntoHandle(docInfo, startOffset, selection.offset, tempTextHandle, 0L);
	if(errCode != noErr) {
		return errCode;
	}
	
	for(formFeedFound = false, index = 0L; startOffset + index < selection.offset; ++index) {
		if((*tempTextHandle)[index] == kPageDownChar) {
			errCode = GetStyleFromOffset(docInfo, startOffset + index, nil, &theStyle);
			if(errCode != noErr) {
				break;
			}
			if(!theStyle.psGraphic) {
				if(selection.offset != index + startOffset + 1L) {
					selectionEnd = false;
					selection.offset = index + startOffset + 1L;
					selection.lastLine = false;
					formFeedFound = true;
					pastEnd = false;
				}
			}
		}
	}
	
	DisposeHandle(tempTextHandle);
	
	if(errCode != noErr) {
		return errCode;
	}

	if(!selectionEnd) {
		errCode = OffsetToPosition(docInfo, &selection, !formFeedFound);
		if(errCode != noErr) {
			return errCode;
		}
	}
	
	if(printPort != nil) {
		
		tempRgn = NewRgn();
		if(tempRgn == nil) {
			return memFullErr;
		}
		
		tempPosition = selection.vPosition;
		if(pastEnd) {
			tempPosition += selection.vLineHeight;
		}
		tempPosition -= vPosition;
		
		SetRect(&tempViewRect, destRect->left, destRect->top, destRect->right, destRect->top + tempPosition);
		RectRgn(tempRgn, &tempViewRect);
		
		(**docInfo).vPosition = vPosition;
		oldViewRect = (**docInfo).viewRect;
		(**docInfo).viewRect = tempViewRect;
		(**(**docInfo).printData).pPort = printPort;

		errCode = DrawDocument(docInfo, tempRgn, false);
		
		(**docInfo).viewRect = oldViewRect;
		DisposeRgn(tempRgn);
	}
		
	if(errCode == noErr) {
		if(pastEnd) {
			errCode = errEndOfDocument;
		} else if((*paraIndex == selection.paraIndex) && (*lineIndex == selection.lineIndex)) {
			++*lineIndex;
		} else {
			*paraIndex = selection.paraIndex;
			*lineIndex = selection.lineIndex;
		}
	}

	return errCode;
}