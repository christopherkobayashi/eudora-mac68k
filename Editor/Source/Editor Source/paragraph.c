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

/* Insert a new paragraph. A negative text size means theText is a handle. */
OSErr InsertParagraph(DocumentInfoHandle docInfo, long beforeIndex, PETEParaInfoPtr paraInfoPtr, Ptr theText, long textSize, long hOffset, PETEStyleListHandle styleList)
{

	ParagraphInfoHandle paraInfo, tempParaInfo;
	LSTable lineStarts;
	ParagraphElement paraElement;
	MyTextStyle textStyle;
	long oldPosition, newPosition;
	long paraIndex, paraOffset, theTextSize;
	OSErr errCode, tempErr;
	short tabIndex;
	Boolean replacePara;
	
	/* Make sure the paragraph index is within the number of paragraphs */
	
	if((beforeIndex < 0L) || ((beforeIndex > (**docInfo).paraCount) && (beforeIndex != kPETELastPara))) {
		return invalidIndexErr;
	} else if(beforeIndex == (**docInfo).paraCount) {
		beforeIndex = kPETELastPara;
	}
	
	if(textSize < 0L) {
		theTextSize = InlineGetHandleSize((Handle)theText);
		hOffset = 0;
	} else {
		theTextSize = textSize;
	}
	
	switch(beforeIndex) {
		/* -1 means append to the end of the document */
		case kPETELastPara :
			/* Set it to the end of the document */
			beforeIndex = (**docInfo).paraCount;
			/* Use the previous paragraph handle as default settings */
			tempParaInfo = (**docInfo).paraArray[beforeIndex - 1L].paraInfo;
			/* Is the last paragraph empty? */
			replacePara = ((**docInfo).paraArray[beforeIndex - 1L].paraLength <= 0L);
			if(replacePara) {
				/* Yes, so we will replace the old one with this one */
				errCode = noErr;
				break;
			}
			
		/* Take defaults from the previous paragraph, or first if inserting at the top */
		default :
			replacePara = false;
			tempParaInfo = (**docInfo).paraArray[(beforeIndex == 0L) ? 0L : (beforeIndex - 1L)].paraInfo;
			SetHandleSize((Handle)docInfo, sizeof(DocumentInfo) + (((**docInfo).paraCount + 1L) * sizeof(ParagraphElement)));
			errCode = MemError();
	}

	if(errCode != noErr) {
		return errCode;
	}
	
	/* Save the position of the next paragraph */
	if(beforeIndex < (**docInfo).paraCount) {
		errCode = ParagraphPosition(docInfo, beforeIndex, &oldPosition);
		if(errCode != noErr) {
			return errCode;
		}
	}
	
	lineStarts = (**tempParaInfo).lineStarts;
	
	/* Create the new paragraph handle */
	paraInfo = (ParagraphInfoHandle)MyNewHandle(sizeof(ParagraphInfo) + (sizeof(short) * (ABS(paraInfoPtr == nil ? (**tempParaInfo).tabCount : paraInfoPtr->tabCount) - 1)) + sizeof(LongStyleRun), &tempErr, hndlClear);

	if((errCode = tempErr) != noErr) {
		return errCode;
	}
	
	/* Move the data into the new handle */
	if(paraInfoPtr == nil) {
		BlockMoveData(&(**tempParaInfo).startMargin, &(**paraInfo).startMargin, (sizeof(ParagraphInfo) + (sizeof(short) * (ABS((**tempParaInfo).tabCount) - 1))) - offsetof(ParagraphInfo, startMargin));
	} else {
		BlockMoveData(&paraInfoPtr->startMargin, &(**paraInfo).startMargin, offsetof(ParagraphInfo, tabStops) - offsetof(ParagraphInfo, startMargin));
		if (paraInfoPtr->tabHandle)
			BlockMoveData(*paraInfoPtr->tabHandle, &(**paraInfo).tabStops, ABS(paraInfoPtr->tabCount) * sizeof(short));
	}

	/* Set up default values where requested */
	if((**paraInfo).startMargin < 0L) {
		/* Default starting margin is #defined */
		(**paraInfo).startMargin = kDefaultMargin;

		/* Move over the tabs that might have been given */
		for(tabIndex = (**paraInfo).tabCount; --tabIndex >= 0; ) {
			(**paraInfo).tabStops[tabIndex] += (**paraInfo).startMargin;
		}
	}
	if((**paraInfo).endMargin == kPETEDefaultMargin) {
		/* Default ending margin is short of the document width */
		(**paraInfo).endMargin = -kDefaultMargin;
	}
	if((**paraInfo).direction == kHilite) {
		/* Default direction is the system direction */
		(**paraInfo).direction = (short)(SignedByte)GetScriptVariable(smSystemScript, smScriptRight);
	}
	if((**paraInfo).justification == teFlushDefault) {
		/* Default justification is the system justification */
		(**paraInfo).justification = ((Byte)GetScriptVariable(smSystemScript, smScriptJust) == 0) ? teFlushLeft : teFlushRight;
	}

	FlushStyleRunCache(paraInfo);
	
	/* Set the last style run to the end of paragraph mark */
	((LongStyleRunPtr)&(**paraInfo).tabStops[ABS((**paraInfo).tabCount)])->srStyleLen = -1L;
	((LongStyleRunPtr)&(**paraInfo).tabStops[ABS((**paraInfo).tabCount)])->srStyleIndex = 0L;
	
	/* If this is a totally new paragraph, create an empty line starts handle */
	if(replacePara && (lineStarts != nil)) {
		(**paraInfo).lineStarts = lineStarts;
	} else {
		(**paraInfo).lineStarts = (LSTable)MyNewHandle(0L, &tempErr, hndlEmpty);
		errCode = tempErr;
	}
	
	if(errCode == noErr) {
		textStyle = (**docInfo).curTextStyle;
		
		/* Add the style info to the paragraph and style table */
		errCode = StyleListToStyles(paraInfo, (**docInfo).theStyleTable, theText, textSize, hOffset, 0L, (Handle)styleList, 0L, &textStyle, ((**docInfo).printData != nil));
	}
	
	if(errCode == noErr) {
		
		/* Initialize the paragraph data for the document handle */
		paraElement.paraHeight = 0L;
		paraElement.paraLSDirty = true;
		paraElement.paraLength = theTextSize;
		paraElement.paraInfo = paraInfo;
		
		/* Find the length of all the text before the inserted paragraph */
		paraOffset = ParagraphOffset(docInfo, beforeIndex);
		
		/* Add the text to the document handle */
		if(textSize < 0L) {
			errCode = AddText(docInfo, paraOffset, (Handle)theText);
		} else {
			errCode = AddTextPtr(docInfo, paraOffset, theText, textSize, hOffset);
		}
		
		if(errCode != noErr) {
			/* On errors, delete the style info from the document */
			DeleteParaStyleRuns(paraInfo, (**docInfo).theStyleTable, 0L, -1L);
		} else {

			if(!replacePara) {
				++(**docInfo).paraCount;
			}
			
			/* Loop through all of the subsequent paragraphs starting at the end */
			for(paraIndex = (**docInfo).paraCount - 1L; paraIndex > beforeIndex; --paraIndex) {

				if(!replacePara) {
					/* Move the paragraph info down one in the list */
					(**docInfo).paraArray[paraIndex] = (**docInfo).paraArray[paraIndex - 1L];
				}
				
				/* If line starts were calculated, offset to account for the new text */
				if(!((**docInfo).paraArray[paraIndex].paraLSDirty)) {
					OffsetLineStarts((**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts, 0L, paraElement.paraLength);
				}
			}
			
			/* Clear the line cache */
			FlushDocInfoLineCache(docInfo);
		
			/* Is the last paragraph being replaced? */
			if(replacePara) {
				/* Yes, so dispose of the handle */
				DisposeHandle((Handle)(**docInfo).paraArray[beforeIndex - 1L].paraInfo);
				/* Move the insertion point back to where the old one is */
				--beforeIndex;
				/* Use the old paragraph height */
				paraElement.paraHeight = (**docInfo).paraArray[beforeIndex].paraHeight;
			} else if((beforeIndex > 0L) && !(**docInfo).paraArray[beforeIndex - 1L].paraLSDirty) {
				errCode = ReflowParagraph(docInfo, beforeIndex - 1L, paraOffset - 1L, paraOffset);
				if(errCode != noErr) {
					SetParaDirty(docInfo, beforeIndex - 1L);
				}
			}
			
			if(errCode == noErr) {
				/* Insert the new paragraph */
				(**docInfo).paraArray[beforeIndex] = paraElement;
				
				SetParaDirty(docInfo, beforeIndex);

				/* Recalculate the line starts */
				errCode = ReflowParagraph(docInfo, beforeIndex, 0L, LONG_MAX);
			}
			
			OffsetIdleGraphics(docInfo, paraOffset, paraElement.paraLength);
			ResetScrollbars(docInfo);
			
			if(errCode != noErr) {
				SetParaDirty(docInfo, beforeIndex);
			} else {
				(**docInfo).flags.scrollsDirty = true;
				
				if(beforeIndex + 1L < (**docInfo).paraCount) {
					ParagraphPosition(docInfo, beforeIndex + 1L, &newPosition);
					newPosition -= oldPosition;
					if(newPosition != 0L) {
						ScrollAndDrawDocument(docInfo, oldPosition, newPosition);
					}
				} else {
					RedrawDocument(docInfo, ParagraphOffset(docInfo, beforeIndex));
				}
			}
			errCode = noErr;
		}
	}
	
	/* Cleanup on errors */
	if(errCode != noErr) {
		/* Dispose of any newly create line starts array */
		if(((**paraInfo).lineStarts != nil) && !replacePara) {
			DisposeHandle((Handle)(**paraInfo).lineStarts);
		}
		/* Dispose of the new paragraph handle */
		DisposeHandle((Handle)paraInfo);
	}

	return errCode;
}

OSErr DeleteParagraph(DocumentInfoHandle docInfo, long paraIndex)
{
	long paraOffset, paraLength;
	ParagraphInfoHandle paraInfo;
	OSErr errCode;
	
	if(paraIndex == -1L) {
		paraIndex = (**docInfo).paraCount - 1L;
	}
	
	paraOffset = ParagraphOffset(docInfo, paraIndex);
	paraLength = (**docInfo).paraArray[paraIndex].paraLength;
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	errCode = DeleteParaStyleRuns(paraInfo, (**docInfo).theStyleTable, 0L, -1L);
	if(errCode == noErr) {
		(**docInfo).docHeight -= (**docInfo).paraArray[paraIndex].paraHeight;
		DisposeHandle((Handle)(**paraInfo).lineStarts);
		DisposeHandle((Handle)paraInfo);
		Munger((Handle)docInfo, sizeof(DocumentInfo) + (paraIndex * sizeof(ParagraphElement)), nil, sizeof(ParagraphElement), (Ptr)1L, 0L);
		RemoveText(docInfo, paraOffset, paraLength);
		--(**docInfo).paraCount;

		if((**docInfo).lineCache.paraIndex >= paraIndex) {
			FlushDocInfoLineCache(docInfo);
		}	
		
		/* Loop through all of the subsequent paragraphs */
		for( ; paraIndex < (**docInfo).paraCount; ++paraIndex) {
			
			/* If line starts were calculated, offset to account for the new text */
			if(!(**docInfo).paraArray[paraIndex].paraLSDirty) {
				SubtractFromLineStarts((**docInfo).paraArray[paraIndex].paraInfo, nil, 0L, paraLength, false);
			}
		}
		
		OffsetIdleGraphics(docInfo, paraOffset, -paraLength);
		if((**docInfo).selStart.offset > paraOffset) {
			(**docInfo).selStart.offset -= paraLength;
			(**docInfo).flags.selectionDirty = true;
		}
		if((**docInfo).selEnd.offset > paraOffset) {
			(**docInfo).selEnd.offset -= paraLength;
			(**docInfo).flags.selectionDirty = true;
		}
		
	}
	return errCode;
}

OSErr DeleteParagraphBreak(DocumentInfoHandle docInfo, long paraIndex)
{
	CombineParagraphs(docInfo, paraIndex);
	SetParaDirty(docInfo, paraIndex);
	if(!(**docInfo).flags.recalcOff)
		return RedrawDocument(docInfo, ParagraphOffset(docInfo, paraIndex));
	else
		return noErr;
}

Boolean CombineParagraphs(DocumentInfoHandle docInfo, long paraIndex)
{
	ParagraphInfoHandle startParaInfo, endParaInfo;
	ParagraphInfoPtr start, end;
	long styleRunIndex;
	LongStyleRun tempStyleRun;
	Boolean formatChange;
	short tabIndex;
	
	(**docInfo).docHeight -= (**docInfo).paraArray[paraIndex].paraHeight;
	(**docInfo).paraArray[paraIndex + 1L].paraLength += (**docInfo).paraArray[paraIndex].paraLength;
	SetParaDirty(docInfo, paraIndex + 1L);
	startParaInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	endParaInfo = (**docInfo).paraArray[paraIndex + 1L].paraInfo;
	
	FlushStyleRunCache(endParaInfo);
	for(styleRunIndex = CountStyleRuns(startParaInfo); styleRunIndex > 0L; ) {
	    GetStyleRun(&tempStyleRun, startParaInfo, --styleRunIndex);
		AddStyleRun(&tempStyleRun, endParaInfo, 0L);
	}
	
	start = *startParaInfo;
	end = *endParaInfo;
	end->lineStartCacheIndex = 0L;
	end->lineStartCacheHeight = 0L;
	
	formatChange = ((start->startMargin != end->startMargin) ||
	                (start->endMargin != end->endMargin) ||
	                (start->indent != end->indent) ||
	                (start->direction != end->direction) ||
	                (start->justification != end->justification) ||
	                (start->signedLevel != end->signedLevel) ||
	                (start->quoteLevel != end->quoteLevel) ||
	                (start->tabCount != end->tabCount));
	
	for(tabIndex = ABS(start->tabCount); --tabIndex >= 0 && !formatChange; ) {
		formatChange = (start->tabStops[tabIndex] != end->tabStops[tabIndex]);
	}
	
	DisposeHandle((Handle)start->lineStarts);
	DisposeHandle((Handle)startParaInfo);
	Munger((Handle)docInfo, sizeof(DocumentInfo) + (paraIndex * sizeof(ParagraphElement)), nil, sizeof(ParagraphElement), (Ptr)1L, 0L);
	--(**docInfo).paraCount;

	if((**docInfo).lineCache.paraIndex >= paraIndex) {
		FlushDocInfoLineCache(docInfo);
	}
	
	return formatChange;
}

short GetPrimaryLineDirection(DocumentInfoHandle docInfo, long textOffset)
{
	long paraIndex;
	
	paraIndex = ParagraphIndex(docInfo, textOffset);
	return (**(**docInfo).paraArray[paraIndex].paraInfo).direction;
}

OSErr AddParagraphBreaks(DocumentInfoHandle docInfo)
{
	long offset;
	OSErr errCode = noErr;
	char ret = kCarriageReturnChar;
	SelData selection;
	Boolean loop = true;
	
	selection = (**docInfo).selStart;
	if(selection.offset == (**docInfo).selEnd.offset) {
		return noErr;
	}
	do {
		selection.paraIndex = ParagraphIndex(docInfo, selection.offset);
		offset = ParagraphOffset(docInfo, selection.paraIndex);
		if((selection.offset != offset) && !((**(**docInfo).paraArray[selection.paraIndex].paraInfo).paraFlags & peNoParaPaste)) {
			errCode = FindMyText(docInfo, &ret, 1, selection.offset, offset, &offset, smRoman);
			if((errCode != noErr) && (errCode != errEndOfBody) && (errCode != errTopOfBody)) {
				return errCode;
			}
			if((++offset <= 0L) ||
			   ((selection.paraIndex + 1 != (**docInfo).paraCount) &&
			    (offset == ParagraphOffset(docInfo, selection.paraIndex + 1))) ||
			   ((**(**docInfo).paraArray[selection.paraIndex].paraInfo).indent != 0) ||
			   ((errCode = InsertParagraphBreakLo(docInfo, offset, false)) == errOffsetInvalid)) {
				errCode = noErr;
			}
		}
	} while(errCode == noErr && loop && (loop = false, selection = (**docInfo).selEnd, true));

	return errCode;
}

OSErr InsertParagraphBreak(DocumentInfoHandle docInfo, long breakChar)
{
	return InsertParagraphBreakLo(docInfo, breakChar, true);
}

OSErr InsertParagraphBreakLo(DocumentInfoHandle docInfo, long breakChar, Boolean redraw)
{
	long paraIndex, paraOffset, textOffset, styleRunIndex, styleRunCount;
	ParagraphInfoHandle paraInfo;
	ParagraphElement newParaElement;
	Handle tempHandle;
	LongSTElement tempStyle;
	LongStyleRun tempStyleRun;
	OSErr errCode, tempErr;

	if(breakChar < 0L) {
		breakChar = (**docInfo).selStart.offset;
	}
	
	if(breakChar > (**docInfo).textLen) {
		breakChar = (**docInfo).textLen;
	}

	/* Get the paragraph to break and its offset */
	paraIndex = ParagraphIndex(docInfo, breakChar);
	paraOffset = breakChar - ParagraphOffset(docInfo, paraIndex);

	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	/* Get the style run to break and the offset into it */
	textOffset = paraOffset;
	styleRunIndex = StyleRunIndex(paraInfo, &textOffset);
	
	if((styleRunIndex == 0L) && (textOffset == 0L)) {
		return errOffsetInvalid;
	}
	
	GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
	
	if(tempStyleRun.srIsGraphic && ((textOffset != 0L) && (textOffset != tempStyleRun.srStyleLen))) {
		return errOffsetInvalid;
	}
	
	/* Create a new paragraph info handle */
	newParaElement.paraInfo = paraInfo;
	errCode = MyHandToHand((Handle *)&newParaElement.paraInfo, 0);
	if(errCode != noErr) {
		return errCode;
	}
	
	if(styleRunIndex > 0L) {
		/* Remove style runs from the beginning of the new paragraph */
		errCode = RemoveStyleRuns(newParaElement.paraInfo, 0L, styleRunIndex);
		if(errCode != noErr) {
			goto Cleanup;
		}
	}

	if(textOffset != 0L) {
		/* Change the first style run of the new paragraph to the proper length */
		tempStyleRun.srStyleLen -= textOffset;
		errCode = ChangeStyleRun(&tempStyleRun, newParaElement.paraInfo, 0L);
		if(errCode != noErr) {
			goto Cleanup;
		}
	}

	/* Make a copy of the old paragraph */
	errCode = MyHandToHand((Handle *)&paraInfo, 0);
	if(errCode != noErr) {
		goto Cleanup;
	}
	
	
	/* Remove style runs from the old paragraph */
	styleRunCount = CountStyleRuns(paraInfo);
	if((textOffset == 0L) || (styleRunIndex + 1L < styleRunCount)) {
		errCode = RemoveStyleRuns(paraInfo, styleRunIndex + ((textOffset == 0L) ? 0L : 1L), styleRunCount - (styleRunIndex + ((textOffset == 0L) ? 0L : 1L)));
		if(errCode != noErr) {
			goto CleanupParaInfo;
		}
	}
	
	/* Change the last style run to the correct length */
	if(textOffset != 0L) {
		tempStyleRun.srStyleLen = textOffset;
		errCode = ChangeStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		if(errCode != noErr) {
			goto CleanupParaInfo;
		}
	}
	
	/* Create a new lineStarts handle */
	tempHandle = MyNewHandle(0L, &tempErr, hndlEmpty);
	if((errCode = tempErr) != noErr) {
		goto CleanupParaInfo;
	}
	
	/* Make space in the document info for the new paragraph */
	SetHandleSize((Handle)docInfo, sizeof(DocumentInfo) + (((**docInfo).paraCount + 1L) * sizeof(ParagraphElement)));
	if((errCode = MemError()) != noErr) {
		goto CleanupAll;
	}
	
	/* Add a style to the style table if a style run was split */
	if((textOffset != 0L) && (breakChar != (**docInfo).textLen)) {
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		++tempStyle.stCount;
		errCode = ChangeStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	}
	
	if(errCode == noErr) {
		if(paraIndex + 1L < (**docInfo).paraCount) {
			BlockMoveData(&(**docInfo).paraArray[paraIndex + 1L],
			              &(**docInfo).paraArray[paraIndex + 2L],
			              sizeof(ParagraphElement) * ((**docInfo).paraCount - (paraIndex + 1L)));
		}
		++(**docInfo).paraCount;
		FlushStyleRunCache(newParaElement.paraInfo);
		(**newParaElement.paraInfo).lineStarts = (LSTable)tempHandle;
		newParaElement.paraHeight = 0L;
		newParaElement.paraLength = (**docInfo).paraArray[paraIndex].paraLength - paraOffset;
		(**docInfo).paraArray[paraIndex + 1L] = newParaElement;
		SetParaDirty(docInfo, paraIndex + 1L);
		FlushStyleRunCache(paraInfo);
		DisposeHandle((Handle)(**docInfo).paraArray[paraIndex].paraInfo);
		(**docInfo).paraArray[paraIndex].paraLength = paraOffset;
		(**docInfo).paraArray[paraIndex].paraInfo = paraInfo;
		RecalcScriptRunLengths(docInfo, paraIndex, breakChar - 1L);
		SubtractFromLineStarts(paraInfo, nil, breakChar - 1L, (**docInfo).paraArray[paraIndex + 1L].paraLength, true);
		if(ReflowParagraph(docInfo, paraIndex, breakChar - 1L, breakChar) == noErr) {
			if(redraw) {
				RedrawDocument(docInfo, breakChar - 1L);
			} else {
				SelData selection;
				
				RecalcDocHeight(docInfo);
				if((**docInfo).selEnd.offset >= breakChar) {
					if((**docInfo).selStart.offset >= breakChar) {
						selection = (**docInfo).selStart;
						++selection.paraIndex;
						RecalcSelectionPosition(docInfo, &selection, peNoLock, false);
						(**docInfo).selStart = selection;
					}
					selection = (**docInfo).selEnd;
					++selection.paraIndex;
					RecalcSelectionPosition(docInfo, &selection, peNoLock, false);
					(**docInfo).selEnd = selection;
				}
			}
		} else {
			SetParaDirty(docInfo, paraIndex);
		}
	} else {
		SetHandleSize((Handle)docInfo, sizeof(DocumentInfo) + ((**docInfo).paraCount * sizeof(ParagraphElement)));
CleanupAll :
		DisposeHandle(tempHandle);
CleanupParaInfo :
		DisposeHandle((Handle)paraInfo);
Cleanup :
		DisposeHandle((Handle)newParaElement.paraInfo);
	}
	
	return errCode;
}

OSErr ApplyParaScrapInfo(DocumentInfoHandle docInfo, long offset, PETEParaScrapHandle paraScrap)
{
	OSErr errCode, memErrCode;
	PETEParaInfo paraInfo;
	long paraScrapSize, paraIndex, paraOffset;
	
	paraScrapSize = InlineGetHandleSize((Handle)paraScrap);
	if(paraScrapSize < 0L) {
		return paraScrapSize;
	}
	paraInfo.tabHandle = (short **)MyNewHandle(0L, &memErrCode, hndlTemp);
	if((errCode = memErrCode) != noErr) {
		return errCode;
	}

#define PARASCRAPPTR ((PETEParaScrapPtr)(((Ptr)*paraScrap) + paraOffset))
				
	for(paraIndex = ParagraphIndex(docInfo, offset), paraOffset = 0L;
	    errCode == noErr && paraOffset < paraScrapSize;
	    ++paraIndex, paraOffset += (sizeof(PETEParaScrapEntry) + (sizeof(PARASCRAPPTR->tabStops[0]) * ABS(PARASCRAPPTR->tabCount)))) {

		SetHandleSize((Handle)paraInfo.tabHandle, sizeof(short) * ABS(PARASCRAPPTR->tabCount));
		if((errCode = MemError()) == noErr) {
			short tabIndex;
			for(tabIndex = ABS(PARASCRAPPTR->tabCount); --tabIndex >= 0; ) {
				(*paraInfo.tabHandle)[tabIndex] = FixRound(PARASCRAPPTR->tabStops[tabIndex]);
			}
			BlockMoveData(&PARASCRAPPTR->startMargin, &paraInfo.startMargin, offsetof(PETEParaInfo, tabHandle) - offsetof(PETEParaInfo, startMargin));
			errCode = SetParagraphInfo(docInfo, paraIndex, &paraInfo, peAllParaValid);
		}
	}
	DisposeHandle((Handle)paraInfo.tabHandle);
	
#undef PARASCRAPPTR

	return errCode;
}

OSErr SetParagraphInfo(DocumentInfoHandle docInfo, long paraIndex, PETEParaInfoPtr paraInfo, short validBits)
{
	long startParaIndex, endParaIndex, tabIndex;
	ParagraphInfoHandle curParaInfo;
	OSErr errCode;
	Boolean justDraw;
	LSTable oldLineStarts;
	Byte hState;
	
	if(paraIndex == kPETELastPara) {
		paraIndex = (**docInfo).paraCount - 1;
	}
	
	if(paraIndex >= (**docInfo).paraCount) {
		return invalidIndexErr;
	}
	
	if(paraIndex < 0L) {
		errCode = SetParaStyleUndo(docInfo, -1L, -1L, peUndoPara);
		if(errCode != noErr) {
			return errCode;
		}
		startParaIndex = ParagraphIndex(docInfo, (**docInfo).selStart.offset);
		endParaIndex = ((**docInfo).selStart.offset == (**docInfo).selEnd.offset) ? startParaIndex : ParagraphIndex(docInfo, (**docInfo).selEnd.offset - 1L);
	} else {
		startParaIndex = endParaIndex = paraIndex;
	}
	
	EraseCaret(docInfo);
	
	for(paraIndex = startParaIndex, justDraw = false; paraIndex <= endParaIndex; ++paraIndex) {
		curParaInfo = (**docInfo).paraArray[paraIndex].paraInfo;
		if(validBits & peTabsValid) {
			hState = HGetState((Handle)paraInfo->tabHandle);
			HLockHi((Handle)paraInfo->tabHandle);
			Munger((Handle)curParaInfo,
			       offsetof(ParagraphInfo, tabStops),
			       nil,
			       sizeof(short) * ABS((**curParaInfo).tabCount),
			       *paraInfo->tabHandle,
			       sizeof(short) * ABS(paraInfo->tabCount));
			errCode = MemError();
			HSetState((Handle)paraInfo->tabHandle, hState);
			if(errCode != noErr) {
				return errCode;
			}
			(**curParaInfo).tabCount = paraInfo->tabCount;
		}
		if(validBits & peStartMarginValid) {
			(**curParaInfo).startMargin = paraInfo->startMargin;
		}
		if(validBits & peEndMarginValid) {
			(**curParaInfo).endMargin = paraInfo->endMargin;
		}
		if(validBits & peIndentValid) {
			(**curParaInfo).indent = paraInfo->indent;
		}
		if(validBits & peDirectionValid) {
			(**curParaInfo).direction = paraInfo->direction;
		}
		if(validBits & peJustificationValid) {
			(**curParaInfo).justification = paraInfo->justification;
		}
		if(validBits & peSignedLevelValid) {
			(**curParaInfo).signedLevel = paraInfo->signedLevel;
		}
		if(validBits & peQuoteLevelValid) {
			(**curParaInfo).quoteLevel = paraInfo->quoteLevel;
		}
		if(validBits & peFlagsValid) {
			(**curParaInfo).paraFlags = paraInfo->paraFlags;
		}
		
		/* Set up default values where requested */
		if((**curParaInfo).startMargin < 0L) {
			/* Default starting margin is #defined */
			(**curParaInfo).startMargin = kDefaultMargin;
	
			/* Move over the tabs that might have been given */
			if(validBits & peTabsValid) {
				for(tabIndex = (**curParaInfo).tabCount; --tabIndex >= 0; ) {
					(**curParaInfo).tabStops[tabIndex] += (**curParaInfo).startMargin;
				}
			}
		}
		if((**curParaInfo).endMargin == kPETEDefaultMargin) {
			/* Default ending margin is short of the document width */
			(**curParaInfo).endMargin = -kDefaultMargin;
		}
		if((**curParaInfo).direction == kHilite) {
			/* Default direction is the system direction */
			(**curParaInfo).direction = (short)(SignedByte)GetScriptVariable(smSystemScript, smScriptRight);
		}
		if((**curParaInfo).justification == teFlushDefault) {
			/* Default justification is the system justification */
			(**curParaInfo).justification = ((Byte)GetScriptVariable(smSystemScript, smScriptJust) == 0) ? teFlushLeft : teFlushRight;
		}
	
		if(!justDraw) {
			justDraw = true;
			if(!(**docInfo).paraArray[paraIndex].paraLSDirty) {
				oldLineStarts = (**curParaInfo).lineStarts;
				if(MyHandToHand((Handle *)&oldLineStarts, hndlTemp) == noErr) {
					if(RedrawParagraph(docInfo, paraIndex, 0L, LONG_MAX, oldLineStarts) == noErr) {
						justDraw = false;
					}
					DisposeHandle((Handle)oldLineStarts);
				}
			}
		}
		if(justDraw) {
			SetParaDirty(docInfo, paraIndex);
		}
	}
	
	if(justDraw) {
		if(RedrawDocument(docInfo, ParagraphOffset(docInfo, startParaIndex)) != noErr) {
			InvalidateDocument(docInfo, false);
		}
	}
	
	SetDirty(docInfo, ParagraphOffset(docInfo, startParaIndex), ParagraphOffset(docInfo, endParaIndex + 1L), true);
	
	return noErr;
}

void SetParaDirty(DocumentInfoHandle docInfo, long paraIndex)
{
	(**docInfo).paraArray[paraIndex].paraLSDirty = true;
	(**docInfo).flags.docHeightValid = false;
}