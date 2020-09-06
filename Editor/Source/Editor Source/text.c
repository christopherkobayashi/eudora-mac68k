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

OSErr InsertKeyChar(DocumentInfoHandle docInfo, unsigned char charCode)
{
	struct CharBuffer {
		unsigned char c[4];
	} charBuffer;
	OSErr errCode;
	long insertOffset;
	Boolean fastInsert;
	MyTextStyle textStyle;
	Byte hState;
	LSTable lineStarts;
	
	if((**docInfo).selStart.offset != (**docInfo).selEnd.offset) {
		if((**docInfo).selectedGraphic != nil) {
			MyCallGraphicHit(docInfo, kPETECurrentSelection, nil);
			(**docInfo).selectedGraphic = nil;
		}
		errCode = DeleteWithKey(docInfo, (**docInfo).selStart.offset, (**docInfo).selEnd.offset);
		if(errCode != noErr) {
			return errCode;
		}
	} else {
		errCode = noErr;
	}
	
	charBuffer = *(struct CharBuffer *)&(**docInfo).charBuffer;
	
	charBuffer.c[++charBuffer.c[0]] = charCode;

	if(((charBuffer.c[0] != 1) && ((**(**docInfo).globals).flags.hasDoubleByte)) ||
	   (GetCurrentTextStyle(docInfo, &textStyle), (CharacterByteType((Ptr)&charCode, 0, StyleToScript(&textStyle)) != smFirstByte))) {
		
		insertOffset = (**docInfo).selStart.offset;
		
		lineStarts = (**(**docInfo).paraArray[(**docInfo).selStart.paraIndex].paraInfo).lineStarts;
		hState = HGetState((Handle)lineStarts);
		HNoPurge((Handle)lineStarts);
		fastInsert = ShouldDoFastInsert(docInfo, charBuffer.c);
		if(fastInsert) {
			(**docInfo).flags.recalcOff = true;
		} else {
			HSetState((Handle)lineStarts, hState);
		}
		
		errCode = InsertText(docInfo, -1L, (Ptr)&charBuffer.c[1], charBuffer.c[0], -1L, nil, !fastInsert);
		if(errCode == noErr) {
			SetInsertUndo(docInfo, insertOffset, charBuffer.c[0], insertOffset, insertOffset, peUndoTyping, false);
			if(fastInsert) {
				DrawOneChar(docInfo, charBuffer.c);
			} else if(!((**docInfo).flags.selectionDirty)) {
				if(GetPrimaryLineDirection(docInfo, (**docInfo).selStart.offset) == leftCaret) {
					(**docInfo).selStart.hPosition = (**docInfo).selStart.lPosition;
				} else {
					(**docInfo).selStart.hPosition = (**docInfo).selStart.rPosition;
				}
				(**docInfo).selEnd = (**docInfo).selStart;
			}
		}
		if(fastInsert) {
			(**docInfo).flags.recalcOff = false;
			HSetState((Handle)lineStarts, hState);
		}
		charBuffer.c[0] = 0;
	}
	if(errCode == noErr) {
		*(struct CharBuffer *)&(**docInfo).charBuffer = charBuffer;
	}
	return errCode;
}

Boolean ShouldDoFastInsert(DocumentInfoHandle docInfo, StringPtr insertText)
{
	ParagraphInfoPtr paraInfo;
	LSTable lineStarts;
	short maxWidth, rPosition;
	long vPosition;
	SelData selection;
	MyTextStyle textStyle;
	long nextStartChar;
	ScriptCode script;
	
	if((insertText[0] == 1) && (insertText[1] < kSpaceChar)) {
		return false;
	}
	selection = (**docInfo).selStart;
	if(selection.offset != (**docInfo).selEnd.offset) {
		return false;
	}
	if((**docInfo).flags.recalcOff || (**docInfo).flags.selectionDirty) {
		return false;
	}
	if((**docInfo).paraArray[selection.paraIndex].paraLSDirty) {
		return false;
	}
	vPosition = (**docInfo).vPosition;
	vPosition -= selection.vPosition;
	if(vPosition > 0L) {
		return false;
	}
	vPosition += RectHeight(&(**docInfo).viewRect) - selection.vLineHeight;
	if(vPosition < 0L) {
		return false;
	}
	if(selection.hPosition < (**docInfo).hPosition) {
		return false;
	}
	rPosition = (**docInfo).hPosition + RectWidth(&(**docInfo).viewRect);
	if(selection.hPosition > rPosition) {
		return false;
	}
	paraInfo = *(**docInfo).paraArray[selection.paraIndex].paraInfo;
	maxWidth = paraInfo->endMargin;
	if(maxWidth < 0L) {
		if(maxWidth == kPETEDefaultMargin) {
			maxWidth = -kDefaultMargin;
		}
		maxWidth += (**docInfo).docWidth;
	}
	if(maxWidth > rPosition) {
		maxWidth = selection.lPosition - rPosition;
	} else {
		maxWidth -= selection.lPosition;
	}
	if(maxWidth <= 0L) {
		return false;
	}
	lineStarts = paraInfo->lineStarts;
	if(*lineStarts == nil) {
		return false;
	}
	
	if(InlineGetHandleSize((Handle)lineStarts) <= sizeof(LSElement)) {
		return false;
	}
	
	if((*lineStarts)[selection.lineIndex].lsULFlag) {
		(*lineStarts)[selection.lineIndex].lsULFlag = false;
		return false;
	}
	
	if(paraInfo->direction != leftCaret) {
		return false;
	}
	
	if((paraInfo->justification != teFlushLeft) && ((paraInfo->justification != teFlushDefault) || ((Byte)GetScriptVariable(smSystemScript, smScriptJust) != 0))) {
		return false;
	}
	
	GetCurrentTextStyle(docInfo, &textStyle);
	if((**(**docInfo).globals).flags.hasMultiScript) {
		script = StyleToScript(&textStyle);
		if((Byte)GetScriptVariable(script, smScriptRight)) {
			return false;
		}
		if((short)GetScriptVariable(script, smScriptFlags) & (1 << smsfContext)) {
			return false;
		}
	}
	
	nextStartChar = (*lineStarts)[selection.lineIndex + 1L].lsStartChar;
	
	if(!((selection.offset == nextStartChar) && ((*lineStarts)[selection.lineIndex + 1L].lsScriptRunLen <= 0L))) {
		LoadParams loadParam;

		do {
			PETEStyleEntry theStyle;
			long styleLen;

			if((GetStyleFromOffset(docInfo, selection.offset, &styleLen, &theStyle) != noErr) ||
			   (theStyle.psGraphic) ||
			   (((**(**docInfo).globals).flags.hasMultiScript) && (selection.offset != nextStartChar) && ((Byte)GetScriptVariable(StyleToScript(&theStyle.psStyle.textStyle), smScriptRight)))) {
				return false;
			}
			loadParam.lpLenRequest = nextStartChar - selection.offset;
			if((loadParam.lpLenRequest <= 0L) || (loadParam.lpLenRequest > styleLen)) {
				loadParam.lpLenRequest = styleLen - theStyle.psStartChar;
			}
			if(LoadText(docInfo, selection.offset, &loadParam, false) != noErr) {
				return false;
			} else {
				Boolean ws;
				Byte hState;
				
				hState = HGetState((Handle)(**docInfo).theText);
				HLock((Handle)(**docInfo).theText);
				ws = IsWhitespace((*(**docInfo).theText) + (selection.offset - (**docInfo).textOffset), loadParam.lpLenRequest, StyleToScript(&theStyle.psStyle.textStyle), (**(**docInfo).globals).whitespaceGlobals, nil);
				HSetState((Handle)(**docInfo).theText, hState);
				
				if(!ws) {
					return false;
				}
			}
		} while((selection.offset += loadParam.lpLenRequest) < nextStartChar);
	}
	
	{
		PETEPortInfo savedPortInfo;
		FMetricRec theMetrics;
		Boolean needReflow;
		
		SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		SetTextStyleWithDefaults(nil, docInfo, &textStyle, false, false);
		FontMetrics(&theMetrics);
		needReflow = ((((*lineStarts)[selection.lineIndex].lsMeasure.ascent >= FixRound(theMetrics.ascent)) ||
		               ((*lineStarts)[selection.lineIndex].lsMeasure.height >= FixRound(theMetrics.ascent + theMetrics.descent + theMetrics.leading))) &&
		              (MyTextWidth(insertText, 1, insertText[0], (**docInfo).globals) <= maxWidth));
		ResetPortInfo(&savedPortInfo);

		return needReflow;
	}
}

OSErr DoInsertText(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long len, long hOffset, PETEStyleListHandle styles)
{
	unsigned short honorLockBits;
	OSErr errCode;

	honorLockBits = peNoLock;
	if(!((**docInfo).flags.ignoreSelectLock)) {
		honorLockBits |= peSelectLock;
	}
	if(!((**docInfo).flags.ignoreModLock)) {
		honorLockBits |= peModLock;
	}
	SetHonorLock(docInfo, peNoLock);
	if(textOffset > (**docInfo).textLen) {
		textOffset = (**docInfo).textLen;
	}
	errCode = InsertText(docInfo, textOffset, theText, len, hOffset, styles, true);
	SetHonorLock(docInfo, honorLockBits);
	return errCode;
}

OSErr InsertText(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long len, long hOffset, PETEStyleListHandle styles, Boolean doRedraw)
{
	OSErr errCode;
	long textLen, paraIndex, tempIndex, paraOffset;
	MyTextStyle textStyle;
	ParagraphInfoHandle paraInfo;
	LSTable oldLineStarts;
	
	if(((textLen = len) == 0L) ||
	   (theText == nil) ||
	   ((textLen < 0L) && ((hOffset = 0L, (*(Handle)theText == nil)) || ((textLen = InlineGetHandleSize((Handle)theText)) <= 0L)))) {
		
		if(textLen > 0L) {
			errCode = DeleteText(docInfo, textOffset, textOffset + textLen);
		} else if(textOffset < 0L) {
			errCode = DeleteText(docInfo, (**docInfo).selStart.offset, (**docInfo).selEnd.offset);
		} else {
			errCode = noErr;
		}
	} else {
	
		/* If the caret was on before drawing started, turn it off */
		EraseCaret(docInfo);
		
		if(textOffset < 0L) {
			errCode = DeleteText(docInfo, (**docInfo).selStart.offset, (**docInfo).selEnd.offset);
		} else {
			errCode = noErr;
		}
		if(errCode == noErr) {
			
			GetCurrentTextStyle(docInfo, &textStyle);

			if(!((**docInfo).flags.ignoreModLock) && (textStyle.tsLock != peNoLock)) {
				errCode = errAENotModifiable;
			} else {

				paraIndex = ParagraphIndex(docInfo, textOffset >= 0L ? textOffset : (**docInfo).selStart.offset);
				
				errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
				if(errCode == cantDoThatInCurrentMode) {
					errCode = noErr;
				}
				
				if(errCode == noErr) {
					paraOffset = ParagraphOffset(docInfo, paraIndex);
					
					paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
					
					errCode = StyleListToStyles(paraInfo,
					                            (**docInfo).theStyleTable,
					                            theText,
					                            len,
					                            hOffset,
					                            (textOffset >= 0L ?
					                             textOffset :
					                             (**docInfo).selStart.offset) - paraOffset,
					                            (Handle)styles,
					                            0L,
					                            &textStyle,
					                            ((**docInfo).printData != nil));
				}
			}
		}
		
		if(errCode == noErr) {

			if(len < 0L) {
				errCode = AddText(docInfo, textOffset, (Handle)theText);
			} else {
				errCode = AddTextPtr(docInfo, textOffset, theText, textLen, hOffset);
			}
			if(errCode != noErr) {
				(**docInfo).flags.docCorrupt = true;
			}
		}
	
		if(errCode == noErr) {
		
			(**docInfo).paraArray[paraIndex].paraLength += textLen;
			
			if(textOffset < 0L) {
				textOffset = (**docInfo).selStart.offset;
				(**docInfo).paraArray[paraIndex].paraLength -= ((**docInfo).selEnd.offset - textOffset);
				(**docInfo).selStart.offset += textLen;
				(**docInfo).selEnd = (**docInfo).selStart;
				(**docInfo).selEnd.lastLine = (**docInfo).selStart.lastLine = false;
			} else if(textOffset < (**docInfo).selEnd.offset) {
				(**docInfo).selEnd.offset += textLen;
				if(textOffset <= (**docInfo).selStart.offset) {
					(**docInfo).selStart.offset += textLen;
				}
			}
			
			FlushStyleRunCache(paraInfo);
			
			OffsetLineStarts((**paraInfo).lineStarts, textOffset, textLen);
			RecalcScriptRunLengths(docInfo, paraIndex, textOffset);

			/* Loop through all of the subsequent paragraphs */
			for(tempIndex = paraIndex; ++tempIndex < (**docInfo).paraCount;) {
				/* If line starts were calculated, offset to account for the new text */
				if(!((**docInfo).paraArray[tempIndex].paraLSDirty)) {
					OffsetLineStarts((**(**docInfo).paraArray[tempIndex].paraInfo).lineStarts, 0L, textLen);
				}
			}
			
			OffsetIdleGraphics(docInfo, textOffset, textLen);

			if(doRedraw) {
				ResetScrollbars(docInfo);
				(**docInfo).flags.selectionDirty = (**docInfo).flags.scrollsDirty = true;
				oldLineStarts = (**paraInfo).lineStarts;
				if(MyHandToHand((Handle *)&oldLineStarts, hndlTemp) == noErr) {
					RedrawParagraph(docInfo, paraIndex, textOffset, textOffset + textLen, oldLineStarts);
					
					DisposeHandle((Handle)oldLineStarts);
				} else {
					SetParaDirty(docInfo, paraIndex);
					InvalidateDocument(docInfo, true);
				}
			}
			
			SetDirty(docInfo, textOffset, textOffset+ textLen, true);

		}
		
	}
	
	return errCode;
}

OSErr DeleteWithKey(DocumentInfoHandle docInfo, long startOffset, long endOffset)
{
	OSErr errCode;
	PETEStyleEntry tempStyle;
	long startIndex, endIndex;
	
	errCode = GetStyleFromOffset(docInfo, startOffset, nil, &tempStyle);
	if(errCode != noErr) {
		return errCode;
	}

	errCode = SetDeleteUndo(docInfo, startOffset, endOffset, peUndoTyping);
	if(errCode != noErr) {
		return errCode;
	}
	
	startIndex = ParagraphIndex(docInfo, startOffset);
	endIndex = ParagraphIndex(docInfo, endOffset - 1L);
	if((startIndex != endIndex) &&
	   ((**(**docInfo).paraArray[startIndex].paraInfo).quoteLevel != (**(**docInfo).paraArray[endIndex].paraInfo).quoteLevel) &&
	   (startOffset != ParagraphOffset(docInfo, startIndex))) {
		char r = kCarriageReturnChar;
		
//		(**docInfo).selEnd.offset = 0L;
		errCode = InsertText(docInfo, startOffset, &r, 1L, -1L, nil, false);
		if(errCode != noErr) {
			return errCode;
		}
		
		++startOffset;
		++endOffset;
		errCode = InsertParagraphBreakLo(docInfo, startOffset, false);
		if(errCode != noErr) {
			return errCode;
		}
		
		SetInsertUndo(docInfo, startOffset - 1L, 1, 0L, 0L, peUndoTyping, false);
	}

	errCode = DeleteText(docInfo, startOffset, endOffset);
	if(errCode != noErr) {
		return errCode;
	}
	
	if(tempStyle.psGraphic) {
		tempStyle.psGraphic = false;
		SetPETEDefaultColor(tempStyle.psStyle.textStyle.tsColor);
	}
	ChangeStyleRange(docInfo, kPETECurrentStyle, kPETECurrentStyle, &tempStyle.psStyle, (peAllValid & ~(peLockValid | peLabelValid)));
	if(GetPrimaryLineDirection(docInfo, (**docInfo).selStart.offset) == leftCaret) {
		(**docInfo).selStart.hPosition = (**docInfo).selStart.lPosition;
	} else {
		(**docInfo).selStart.hPosition = (**docInfo).selStart.rPosition;
	}
	(**docInfo).selEnd = (**docInfo).selStart;

	return noErr;
}

OSErr DeleteText(DocumentInfoHandle docInfo, long startChar, long endChar)
{
	long startParaIndex, endParaIndex, startStyleRunIndex, endStyleRunIndex, startOffset, endOffset;
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun;
	Boolean paraDelete, paraChange;
	LSTable oldLineStarts;
	OSErr errCode;

	if(startChar < 0L) {
		startChar = 0L;
	} else if(startChar > (**docInfo).textLen) {
		startChar = (**docInfo).textLen;
	}
	
	if(endChar < 0L) {
		endChar = 0L;
	} else if(endChar > (**docInfo).textLen) {
		endChar = (**docInfo).textLen;
	}
	
	if(startChar > endChar) {
		return errAEImpossibleRange;
	}

	if(startChar == endChar) {
		return noErr;
	}

	if(!((**docInfo).flags.ignoreModLock) && SelectionHasLock(docInfo, startChar, endChar)) {
		return errAENotModifiable;
	}

	/* If the caret was on before drawing started, turn it off */
	EraseCaret(docInfo);

	if(startChar < (**docInfo).selEnd.offset) {
		if(((**docInfo).selectedGraphic != nil) && (startChar <= (**docInfo).selStart.offset) && (endChar >= (**docInfo).selEnd.offset)) {
			MyCallGraphicHit(docInfo, kPETECurrentSelection, nil);
			(**docInfo).selectedGraphic = nil;
		}
		if(endChar < (**docInfo).selStart.offset) {
			(**docInfo).selStart.offset -= (endChar - startChar);
			(**docInfo).selEnd.offset -= (endChar - startChar);
		} else if(startChar >= (**docInfo).selStart.offset) {
			if(endChar <= (**docInfo).selEnd.offset) {
				(**docInfo).selEnd.offset -= (endChar - startChar);
			} else {
				(**docInfo).selEnd.offset = startChar;
			}
		} else if(endChar < (**docInfo).selEnd.offset) {
			(**docInfo).selEnd.offset -= (endChar - startChar);
			(**docInfo).selStart.offset = startChar;
		} else {
			(**docInfo).selEnd.offset = (**docInfo).selStart.offset = startChar;
			(**docInfo).selEnd.lastLine = (**docInfo).selStart.lastLine = false;
		}
	}

	startParaIndex = ParagraphIndex(docInfo, startChar);
	endParaIndex = ParagraphIndex(docInfo, endChar);
	
	if(endParaIndex == -1L) {
		endParaIndex = (**docInfo).paraCount - 1L;
	}
	
	for(errCode = noErr, paraDelete = false; errCode == noErr && endParaIndex - 1L > startParaIndex; --endParaIndex) {
		endChar -= (**docInfo).paraArray[startParaIndex + 1L].paraLength;
		errCode = DeleteParagraph(docInfo, startParaIndex + 1L);
		paraDelete = true;
	}
	
	if(errCode != noErr) {
		return errCode;
	}
	
	if((endParaIndex != startParaIndex) ||
	   ((++endParaIndex < (**docInfo).paraCount) && (endChar == ParagraphOffset(docInfo, endParaIndex)))) {
		paraDelete = true;
		paraChange = CombineParagraphs(docInfo, startParaIndex);
	} else {
		paraChange = false;
	}
	
	/* Loop through all of the subsequent paragraphs */
	for(; endParaIndex < (**docInfo).paraCount; ++endParaIndex) {
		
		/* If line starts were calculated, offset to account for the deleted text */
		if(!((**docInfo).paraArray[endParaIndex].paraLSDirty)) {
			SubtractFromLineStarts((**docInfo).paraArray[endParaIndex].paraInfo, nil, 0L, endChar - startChar, false);
		}
	}
	
	paraInfo = (**docInfo).paraArray[startParaIndex].paraInfo;
	
	startOffset = startChar - ParagraphOffset(docInfo, startParaIndex);
	endOffset = startOffset + (endChar - startChar);
	
	(**docInfo).paraArray[startParaIndex].paraLength -= (endChar - startChar);
	
	startStyleRunIndex = StyleRunIndex(paraInfo, &startOffset);
	endStyleRunIndex = StyleRunIndex(paraInfo, &endOffset);

	while(endStyleRunIndex - 1L > startStyleRunIndex) {
		--endStyleRunIndex;
		DeleteStyleRun(paraInfo, (**docInfo).theStyleTable, endStyleRunIndex);
	}
	
	if((startOffset == 0L) && (startStyleRunIndex != endStyleRunIndex)) {
		DeleteStyleRun(paraInfo, (**docInfo).theStyleTable, startStyleRunIndex);
		--endStyleRunIndex;
	}
	
	if((**docInfo).paraArray[startParaIndex].paraLength != 0L) {
		GetStyleRun(&tempStyleRun, paraInfo, startStyleRunIndex);
		if(startStyleRunIndex != endStyleRunIndex) {
			tempStyleRun.srStyleLen = startOffset;
		} else {
			tempStyleRun.srStyleLen -= (endOffset - startOffset);
		}
		ChangeStyleRun(&tempStyleRun, paraInfo, startStyleRunIndex);
	}
		
	FlushStyleRunCache(paraInfo);
	
	if(startStyleRunIndex < endStyleRunIndex) {
		GetStyleRun(&tempStyleRun, paraInfo, endStyleRunIndex);
		tempStyleRun.srStyleLen -= endOffset;
		ChangeStyleRun(&tempStyleRun, paraInfo, endStyleRunIndex);
		FlushStyleRunCache(paraInfo);
	}
	
	CompressStyleRuns(paraInfo, (**docInfo).theStyleTable, startStyleRunIndex + 1L, 0L);
	
	RemoveText(docInfo, startChar, endChar - startChar);

	if((**docInfo).paraArray[startParaIndex].paraLength == 0L) {
		EmptyHandle((Handle)(**paraInfo).lineStarts);
		SetParaDirty(docInfo, startParaIndex);
	}
		
	RecalcScriptRunLengths(docInfo, startParaIndex, endChar);

	if((**docInfo).paraArray[startParaIndex].paraLSDirty) {
		paraDelete = true;
	}
	
	if(!paraDelete) {
		oldLineStarts = (**paraInfo).lineStarts;
		if(MyHandToHand((Handle *)&oldLineStarts, hndlTemp) != noErr) {
			paraDelete = true;
			SetParaDirty(docInfo, startParaIndex);
		} else {
			SubtractFromLineStarts(nil, oldLineStarts, startChar, endChar - startChar, false);
		}
	}
		
	if(!(**docInfo).paraArray[startParaIndex].paraLSDirty) {
		SubtractFromLineStarts(paraInfo, nil, startChar, endChar - startChar, true);
	}
	
	OffsetIdleGraphics(docInfo, startChar, startChar - endChar);

	(**docInfo).flags.selectionDirty = (**docInfo).flags.scrollsDirty = true;
	ResetScrollbars(docInfo);
	
	if(!paraDelete) {
		errCode = RedrawParagraph(docInfo, startParaIndex, startChar, startChar, oldLineStarts);
		DisposeHandle((Handle)oldLineStarts);
	} else {
		if(!(**docInfo).paraArray[startParaIndex].paraLSDirty) {
			if(ReflowParagraph(docInfo, startParaIndex, paraChange ? 0L : startChar, startChar) != noErr) {
				SetParaDirty(docInfo, startParaIndex);
			}
		}
		errCode = RedrawDocument(docInfo, paraChange ? 0L : startChar);
	}
	
	SetDirty(docInfo, startChar, endChar, true);
	
	return noErr;
}

OSErr MyReplaceText(DocumentInfoHandle docInfo, long textOffset, long replaceLen, Ptr theText, long len, long hOffset, PETEStyleListHandle styles)
{
	OSErr errCode;
	long startChar, endChar;
	
	if(textOffset < 0L && replaceLen < 0L) {
		return InsertText(docInfo, textOffset, theText, len, hOffset, styles, true);
	}
	if(textOffset < 0L) {
		startChar = (**docInfo).selStart.offset;
	} else {
		startChar = textOffset;
	}
	if(replaceLen < 0L) {
		endChar = (**docInfo).selEnd.offset;
	} else {
		endChar = startChar + replaceLen;
	}
	if(startChar > endChar) {
		return errAEImpossibleRange;
	}
	if((errCode = DeleteText(docInfo, startChar, endChar)) != noErr) {
		return errCode;
	} else {
		return InsertText(docInfo, startChar, theText, len, hOffset, styles, true);
	}
}

OSErr RemoveText(DocumentInfoHandle docInfo, long textOffset, long textSize)
{
	Handle theText;
	
	theText = (Handle)(**docInfo).theText;
	BlockMoveData(*theText + textOffset + textSize, *theText + textOffset, (**docInfo).textLen - (textOffset + textSize));
	(**docInfo).textLen -= textSize;
	textSize = InlineGetHandleSize(theText);
	if(textSize > (**docInfo).textLen + 1024L) {
		SetHandleSize(theText, IntegralSize((**docInfo).textLen));
		return MemError();
	} else {
		return noErr;
	}
}

/* Insert text into the document text. Disk backing stuff later. */
OSErr AddTextPtr(DocumentInfoHandle docInfo, long textOffset, Ptr theText, long textSize, long hOffset)
{
	OSErr errCode, tempErr;
	Handle tempText, curText;
	long replaceLen, textLen;
	
	replaceLen = 0L;
	textLen = (**docInfo).textLen;
	
	/* If there was no text, create a new handle */
	if((**docInfo).theText == nil) {
		tempText = MyNewHandle(IntegralSize(textSize), &tempErr, 0);
		if((errCode = tempErr) == noErr) {
			if(hOffset >= 0L) {
				theText = *(Handle)theText + hOffset;
			}
			BlockMoveData(theText, *tempText, textSize);
		}
		(**docInfo).theText = (StringHandle)tempText;
	} else {
		curText = (Handle)(**docInfo).theText;
		if(textOffset < 0L) {
			textOffset = (**docInfo).selStart.offset;
			replaceLen = (**docInfo).selEnd.offset - textOffset;
		}
		
		if(InlineGetHandleSize(curText) < textLen + textSize) {
			HUnlock(curText);	// Make sure it's not locked before resizing.
			SetHandleSize(curText, IntegralSize(textLen + textSize));
			errCode = MemError();
		} else {
			errCode = noErr;
		}
		if(errCode == noErr) {
			BlockMoveData((*curText) + textOffset + replaceLen, (*curText) + textOffset + textSize, textLen - textOffset - replaceLen);
			if(hOffset >= 0L) {
				theText = *(Handle)theText + hOffset;
			}
			switch(textSize) {
				case 2L :
					(*curText)[textOffset + 1L] = theText[1L];
				case 1L :
					(*curText)[textOffset] = theText[0L];
					break;
				default :
					BlockMoveData(theText, (*curText) + textOffset, textSize);
			}
		}
	}
	if(errCode == noErr) {
		(**docInfo).textLen += textSize - replaceLen;
		SetDirty(docInfo, textOffset, textOffset + textSize, true);
	}
	
	return errCode;
}

/* Insert text into the document text. Disk backing stuff later. */
OSErr AddText(DocumentInfoHandle docInfo, long textOffset, Handle theText)
{
	long oldSize, newSize, replaceLen, textLen;
	Handle oldText;
	OSErr errCode;
	
	/* Get the size of the new text */
	newSize = InlineGetHandleSize(theText);
	
	if((errCode = MemError()) != noErr) {
		return errCode;
	}
	
	/* If there was no text, just use the new handle */
	if((**docInfo).theText == nil) {
		(**docInfo).theText = (StringHandle)theText;
		(**docInfo).textLen = newSize;
		return noErr;
	}
	
	oldSize = InlineGetHandleSize((Handle)(**docInfo).theText);
	
	/* Figure out where to insert the text */
	if(textOffset < 0L) {
		textOffset = (**docInfo).selStart.offset;
		replaceLen = (**docInfo).selEnd.offset - textOffset;
	} else {
		replaceLen = 0L;
	}
	
	/* Get the old text handle */
	oldText = (Handle)(**docInfo).theText;
	textLen = (**docInfo).textLen;

	/* Grow the size of the currently biggest text handle */
	SetHandleSize((oldSize >= newSize) ? oldText : theText, IntegralSize(textLen + newSize - replaceLen));
	
	if((errCode = MemError()) != noErr) {
		return errCode;
	}
		
	/* Move the text into the biggest text handle */
	if(oldSize >= newSize) {
		/* Move the new text into the old handle */
		BlockMoveData(*oldText + textOffset + replaceLen, *oldText + textOffset + newSize, textLen - textOffset - replaceLen);
		BlockMoveData(*theText, *oldText + textOffset, newSize);
	} else {
		/* Move the old text into the new handle */
		BlockMoveData(*theText, *theText + textOffset, newSize);
		BlockMoveData(*oldText, *theText, textOffset);
		BlockMoveData(*oldText + textOffset + replaceLen, *theText + textOffset + newSize, textLen - textOffset - replaceLen);
		/* Move the new handle into the document structure */
		ExchangePtr((**docInfo).theText, theText);
	}
	
	/* Dispose of the smaller text handle */
	DisposeHandle(theText);
	
	/* Set the new size */
	(**docInfo).textLen = textLen + newSize - replaceLen;
	
	SetDirty(docInfo, textOffset, textOffset + newSize - replaceLen, true);
	
	return noErr;
}

/* Try to load up the requested text. Will be fancier, with disk I/O and proper breaks */
OSErr LoadText(DocumentInfoHandle docInfo, long textStart, LoadParamsPtr loadParam, Boolean forceLoad)
{
	if(!((textStart == textStart) && (forceLoad == forceLoad))) {
		return noErr;
	}
	loadParam->lpStyleRunIndex = -1L;
	loadParam->lpStyleRunLen = 0L;
	loadParam->lpScriptRunLeft = 0L;
	(**docInfo).textLoaded = (**docInfo).textLen;
	return noErr;
}

/* Mark the text as purgable. Fancier later */
void UnloadText(DocumentInfoHandle docInfo)
{
	if(docInfo == docInfo)
		return;
}

OSErr LoadTextIntoHandle(DocumentInfoHandle docInfo, long startOffset, long endOffset, Handle destHandle, long destOffset)
{
	long curOffset;
	OSErr errCode;
	LoadParams loadParam;
	
	curOffset = startOffset;
	do {
		loadParam.lpLenRequest = endOffset - curOffset;
		errCode = LoadText(docInfo, curOffset, &loadParam, false);
		
		if(errCode == noErr) {
			BlockMoveData(*(**docInfo).theText + (curOffset - (**docInfo).textOffset), *destHandle + destOffset + curOffset - startOffset, loadParam.lpLenRequest);
			
			if(loadParam.lpLenRequest < endOffset - curOffset) {
				curOffset = (**docInfo).textOffset + (**docInfo).textLoaded;
			} else {
				curOffset = endOffset;
			}
		}
		UnloadText(docInfo);
	} while((errCode == noErr) && (curOffset < endOffset));
	return errCode;
}

OSErr GetText(DocumentInfoHandle docInfo, long startChar, long endChar, Ptr theText, long intoSize, long *textLen)
{
	LoadParams loadParam;
	OSErr errCode, tempErr;
	long paraIndex, tempOffset, styleRunOffset;
	ParagraphInfoHandle paraInfo;
	StringHandle textHandle;
	Byte hState;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	
	if(endChar > (**docInfo).textLen) {
		endChar = (**docInfo).textLen;
	}
	
	if((startChar >= endChar) || (startChar < 0L) || (endChar < 0L)) {
		errCode = errAEImpossibleRange;
	} else {
	
		if(textLen == (long *)-1L) {
			if(*(Handle *)theText == nil) {
				*(Handle *)theText = MyNewHandle(endChar - startChar, &tempErr, 0);
				errCode = tempErr;
			} else {
				if(**(Handle *)theText == nil) {
					ReallocateHandle(*(Handle *)theText, endChar - startChar);
				} else {
					SetHandleSize(*(Handle *)theText, endChar - startChar);
				}
				errCode = MemError();
			}
			if(errCode == noErr) {
				BlockMoveData(*(**docInfo).theText + startChar, **(Handle *)theText, endChar - startChar);
			}
		} else {
			if(intoSize > endChar - startChar) {
				intoSize = endChar - startChar;
			}
			if(intoSize < 0L) {
				intoSize = 0L;
			}
			BlockMoveData(*(**docInfo).theText + startChar, theText, intoSize);
			if(textLen != nil) {
				*textLen = intoSize;
			}
			errCode = noErr;
		}
	}

	return errCode;

	paraIndex = ParagraphIndex(docInfo, startChar);
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	tempOffset = startChar;
	GetStyleRun(&tempStyleRun, paraInfo, StyleRunIndex(paraInfo, &tempOffset));
	GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	
	loadParam.lpLenRequest = endChar - startChar;
	errCode = LoadText(docInfo, startChar, &loadParam, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	styleRunOffset = (startChar - tempOffset) - (**docInfo).textOffset;
	if(styleRunOffset < 0L) {
		styleRunOffset = 0L;
	}

	textHandle = (**docInfo).theText;
	hState = HGetState((Handle)textHandle);
	HLock((Handle)textHandle);
	
}