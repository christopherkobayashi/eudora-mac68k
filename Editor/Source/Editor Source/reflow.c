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
#define USEOWNSTYLEDLINEBREAK 	1

OSErr CheckParagraphMeasure(DocumentInfoHandle docInfo, long paraNum, Boolean needLineStarts)
{
	if(paraNum > (**docInfo).paraCount) {
		return invalidIndexErr;
	}
	if((**docInfo).paraArray[paraNum].paraLSDirty ||
	   (needLineStarts && *((**(**docInfo).paraArray[paraNum].paraInfo).lineStarts) == nil)) {
		return ReflowParagraph(docInfo, paraNum, 0L, LONG_MAX);
	} else {
		return noErr;
	}
}

Boolean FindAndCompressLineStarts(LSTable lineStarts, LSPtr curLine, long lineIndex, long* lineCount)
{
	long startLineIndex;
	
	for(startLineIndex = lineIndex; (lineIndex < *lineCount) && (curLine->lsStartChar >= (*lineStarts)[lineIndex].lsStartChar); ++lineIndex) {
		if((curLine->lsStartChar == (*lineStarts)[lineIndex].lsStartChar) &&
		   (curLine->lsScriptRunLen == (*lineStarts)[lineIndex].lsScriptRunLen)) {
			Munger((Handle)lineStarts, startLineIndex * sizeof(LSElement), nil, (lineIndex - startLineIndex) * sizeof(LSElement), nil, 0L);
			if(MemError() == noErr) {
				*lineCount -= lineIndex - startLineIndex;
				return true;
			} else {
				break;
			}
		}
	}
	return false;
}

/* Recalculate the line starts for a paragraph between two offsets */
OSErr ReflowParagraph(DocumentInfoHandle docInfo, long paraNum, long startChar, long endChar)
{
	long lineCount, lineIndex, tempOffset;
	LSElement tempLine;
	LSTable lineStarts;
	Byte hState;
	OSErr errCode;
	
	if((**docInfo).flags.recalcOff || (**docInfo).flags.progressLoopCalled) {
		return cantDoThatInCurrentMode;
	}
	
	/* Get the lineStarts handle */
	lineStarts = (**(**docInfo).paraArray[paraNum].paraInfo).lineStarts;
	
	/* If the lineStarts handle has been purged, reallocate it */
	if(*lineStarts == nil) {
		ReallocateHandle((Handle)lineStarts, 0L);
		if((errCode = MemError()) != noErr) {
			(**docInfo).flags.docCorrupt = true;
			return errCode;
		}
		HPurge((Handle)lineStarts);
	}
	
	errCode = noErr;
	
	/* Make the lineStarts handle non-purgable */
	hState = HGetState((Handle)lineStarts);
	HNoPurge((Handle)lineStarts);
	
	/* Determine how many lines are now in the lineStarts handle */
	lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
	
	/* Subtract the paragraph height from the document height */
	(**docInfo).docHeight -= (**docInfo).paraArray[paraNum].paraHeight;
	
	(**docInfo).paraArray[paraNum].paraHeight = 0L;

	for(lineIndex = 0L; lineIndex < lineCount - 1L; ++lineIndex) {
		if((startChar < (*lineStarts)[lineIndex + 2L].lsStartChar) ||
		   ((lineIndex == lineCount - 2L) && (startChar == (*lineStarts)[lineIndex + 2L].lsStartChar))) {
			break;
		}
	}

	/* Get the first line to start with if found */
	if(lineIndex > 0L) {
		if(lineIndex <= (**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheIndex) {
			if(lineCount < (**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheIndex) {
				(**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheIndex = 0L;
				(**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheHeight = 0L;
			}
		}
		(**docInfo).paraArray[paraNum].paraHeight += ResetLineCache((**docInfo).paraArray[paraNum].paraInfo, lineIndex);
		tempLine.lsStartChar = (*lineStarts)[lineIndex].lsStartChar;
		tempLine.lsScriptRunLen = (*lineStarts)[lineIndex].lsScriptRunLen;
	/* Otherwise, initialize the starting line to zero */
	} else {
		tempLine.lsStartChar = ParagraphOffset(docInfo, paraNum);
		tempLine.lsScriptRunLen = 0L;
		lineIndex = 0L;
		
	}
	
	if(lineIndex < (**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheIndex) {
		(**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheIndex = lineIndex;
		(**(**docInfo).paraArray[paraNum].paraInfo).lineStartCacheHeight = (**docInfo).paraArray[paraNum].paraHeight;
	}

	/* If the first script run length needs to be reset, do it */
	if(tempLine.lsScriptRunLen <= 0L) {
		tempOffset = tempLine.lsStartChar - ParagraphOffset(docInfo, paraNum);
		tempLine.lsScriptRunLen = ScriptRunLen((**docInfo).theStyleTable, (**docInfo).paraArray[paraNum].paraInfo, StyleRunIndex((**docInfo).paraArray[paraNum].paraInfo, &tempOffset), nil);
		tempLine.lsScriptRunLen -= tempOffset;
		if(tempLine.lsScriptRunLen < 0L) {
			tempLine.lsScriptRunLen = 0L;
		}
	}
	
	/* Flush the old measurement information, if any */
	if(((**docInfo).lineCache.paraIndex == paraNum) && ((**docInfo).lineCache.lineIndex >= lineIndex)) {
		FlushDocInfoLineCache(docInfo);
	}
	
	do {
		
		errCode = MyCallProgressLoop(docInfo);
		if(errCode != noErr) {
			break;
		}
		
		if((tempLine.lsScriptRunLen == 0L) && (paraNum < (**docInfo).paraCount - 1L)) {
			tempLine.lsScriptRunLen = -1L;
		}
		
		/* Check to see if there's enough room in the lineStarts handle */
		if(lineIndex == lineCount + 1L) {
			/* There's not enough room, so add another entry */
			errCode = PtrAndHand(&tempLine, (Handle)lineStarts, sizeof(LSElement));
			++lineCount;
		/* Check to see if this line start was already calculated */
		} else if((tempLine.lsStartChar > endChar) && FindAndCompressLineStarts(lineStarts, &tempLine, lineIndex, &lineCount)) {
			for(; lineIndex < lineCount; ++lineIndex) {
				(**docInfo).paraArray[paraNum].paraHeight += (*lineStarts)[lineIndex].lsMeasure.height;
			}
			break;
		/* Check to see if we should insert a new line start */
		} else if((startChar != endChar) &&
		          (tempLine.lsStartChar + 1L < (*lineStarts)[lineIndex].lsStartChar) &&
		          ((*lineStarts)[lineIndex].lsStartChar < (**docInfo).paraArray[paraNum].paraLength)) {
			Munger((Handle)lineStarts, lineIndex * sizeof(LSElement), nil, 0L, &tempLine, sizeof(LSElement));
			if((errCode = MemError()) == noErr) {
				++lineCount;
			}
		/* Otherwise, set the entry in the lineStarts handle to the current line */
		} else {
			(*lineStarts)[lineIndex].lsStartChar = tempLine.lsStartChar;
			(*lineStarts)[lineIndex].lsScriptRunLen = tempLine.lsScriptRunLen;
		}
		
		/* If this is the end of the text, stop processing */
		if(errCode == noErr) {
			if(tempLine.lsScriptRunLen <= 0L) {
				GetDefaultLineHeight(docInfo, paraNum, &tempLine.lsMeasure);
			}
			
			if(tempLine.lsScriptRunLen < 0L) {
				/* If there are no lines, fill in default height information */
				if(lineIndex == 0L) {

					/* Move the line height measurements into the line start entry */
					(*lineStarts)[lineIndex].lsMeasure = tempLine.lsMeasure;
			
					(**docInfo).paraArray[paraNum].paraHeight = tempLine.lsMeasure.height;
				}
				break;
			}

			if(tempLine.lsScriptRunLen == 0L) {
				tempLine.lsScriptRunLen = -1L;
				goto NextLine;
			} else {
				/* Get the next line break and the current line height */
				errCode = GetLineBreak(docInfo, &tempLine, paraNum, (lineIndex == 0L));
				if(errCode == noErr) {

	NextLine :					
					/* Move the line height measurements into the line start entry */
					(*lineStarts)[lineIndex].lsMeasure = tempLine.lsMeasure;
					
					(**docInfo).paraArray[paraNum].paraHeight += tempLine.lsMeasure.height;
					/* Move on to the next line start */
					++lineIndex;
				}
			}
		}
		
	} while(errCode == noErr);
	
	if((errCode != noErr) && (errCode != userCanceledErr)) {
		(**docInfo).flags.docCorrupt = true;
		EmptyHandle((Handle)lineStarts);
	} else {
		/* If the lineStarts handle is bigger than the number of lines, shrink it */
		if(lineIndex < lineCount) {
			SetHandleSize((Handle)lineStarts, sizeof(LSElement) * (lineIndex + 1L));
		}
		
		(**docInfo).docHeight += (**docInfo).paraArray[paraNum].paraHeight;
			
		(**docInfo).paraArray[paraNum].paraLSDirty = (errCode != noErr);
	}

	HSetState((Handle)lineStarts, hState);
	
	return errCode;
}

/* Find the line break and line height given the current offset into the text */
OSErr GetLineBreak(DocumentInfoHandle docInfo, LSPtr lineBreak, long paraIndex, Boolean firstLine)
{
	long textOffset, scriptRunLen, runIndex, styleRunLen, textStart, textEnd;
	LSElement firstLineBreak;
	LoadParams loadParam;
	LongStyleRun tempStyleRun;
	ParagraphInfoHandle paraInfo;
	LongSTTable theStyleTable;
	StringHandle theText;
	StyledLineBreakCode breakCode;
	LongSTElement theStyle;
	PETEPortInfo savedPortInfo;
	short textWidth, lineWidth;
	Boolean tabRight, justRight, firstScriptRun;
	short curDirection;
	OSErr errCode;
	Byte hState;
	unsigned char tempChar;

	/* Save old port info */
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);

	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	theStyleTable = (**docInfo).theStyleTable;
	theText = (**docInfo).theText;

	/* Save the original value of lineBreak */
	firstLineBreak = *lineBreak;

	/* Zero out the line height and ascent measurements */
	lineBreak->lsMeasure.height = 0L;
	lineBreak->lsMeasure.ascent = 0L;
	
	/* Get the full width of the line */
	textWidth = (**paraInfo).endMargin;
	if(textWidth < 0L) {
		textWidth += (**docInfo).docWidth;
	}
	textWidth -= (**paraInfo).startMargin;
	
	/* Deal with the indentation */
	textWidth -= IndentWidth(paraInfo, firstLine);
	
	lineWidth = textWidth;
	
	/* Set textOffset to the first character for this line */
	textOffset = lineBreak->lsStartChar - ParagraphOffset(docInfo, paraIndex);
	/* Get the index of the style run that contains the first character */
	runIndex = StyleRunIndex(paraInfo, &textOffset);
	/* Get the length of the style run and subtract off the amount into the run */
	styleRunLen = GetStyleRun(&tempStyleRun, paraInfo, runIndex) - textOffset;

	/* Initialize values */
	loadParam.lpLenRequest = lineBreak->lsScriptRunLen;
	breakCode = smBreakOverflow;
	scriptRunLen = 0L;
	firstScriptRun = true;

	/* Load in the text to measure and start looking for the line end */
	goto LoadTextAndLoop;

	do {
		/* Set the port to the current font, style, color */
		GetStyle(&theStyle, theStyleTable, tempStyleRun.srStyleIndex);
		SetTextStyleWithDefaults(nil, docInfo, &theStyle.stInfo.textStyle, tempStyleRun.srIsGraphic, ((**docInfo).printData != nil));
		
		/* Check to see if this is text */
		if(!(tempStyleRun.srIsGraphic)) {
			/* Is this a tab run, not on a centered line? */
			if((tempStyleRun.srIsTab) && ((**paraInfo).justification != teCenter)) {
			
				/* Are the tabs are in the opposite direction as the justification? */
				justRight = ((((**paraInfo).justification == teFlushDefault) && ((**paraInfo).direction != leftCaret)) || ((**paraInfo).justification == teFlushRight));
				tabRight = !(!(Boolean)GetScriptVariable(smCurrentScript, smScriptRight));
				if(tabRight != justRight) {
#ifdef INTELLIGENTTABS
					/* Need to measure the line piecewise. Ugh! */
					*lineBreak = firstLineBreak;
					return GetTabbedLineBreak(theText, lineBreak, firstLine, paraInfo, theStyleTable);
#else
					textWidth = 0L;
					textOffset = textEnd;
					breakCode = smBreakWord;
#endif
				} else {
					/* Otherwise, just subtract the width of the tabs by hand */
					textOffset = textEnd;
					textWidth -= TabWidth(paraInfo, ((**paraInfo).endMargin + (((**paraInfo).endMargin < 0L) ? (**docInfo).docWidth : 0L)) - textWidth, (**docInfo).docWidth);
					
					/* Continue if it still fits or if there's more in this script run */
					if((textWidth > 0L) || ((textWidth = 0L), (lineBreak->lsScriptRunLen - (textEnd - textStart) > 0L))) {
						breakCode = smBreakOverflow;
					} else {
						breakCode = smBreakWord;
					}
				}
			} else {
				/* Regular old text; just look for a line break */
				
				/* The textOffset is a flag to indicate the first run on a line */
				textOffset = firstScriptRun;

				hState = HGetState((Handle)theText);
				HLock((Handle)theText);

				/* Set the line direction */
				curDirection = GetSysDirection();
				SetSysDirection((**paraInfo).direction);

#ifdef USEOWNSTYLEDLINEBREAK

				breakCode = MyStyledLineBreak((Ptr)*theText + (lineBreak->lsStartChar - (**docInfo).textOffset), scriptRunLen, textStart, textEnd, 0L, &textWidth, &textOffset, (**docInfo).globals);

#else
				breakCode = StyledLineBreak((Ptr)*theText + (lineBreak->lsStartChar - (**docInfo).textOffset),
											scriptRunLen > SHRT_MAX ? SHRT_MAX : scriptRunLen,
											textStart > SHRT_MAX ? SHRT_MAX - 1L : textStart,
											textEnd > SHRT_MAX ? SHRT_MAX : textEnd,
											0L, &textWidth, &textOffset);
				
				if(FindNextFF((Ptr)*theText + (lineBreak->lsStartChar - (**docInfo).textOffset), textStart > SHRT_MAX ? SHRT_MAX - 1L : textStart, (breakCode == smBreakOverflow) ? (textEnd > SHRT_MAX ? SHRT_MAX : textEnd) : textOffset, &textOffset)) {
					++textOffset;
					breakCode = smBreakWord;
				}
#endif

				/* Restore line direction */
				SetSysDirection(curDirection);
				
				HSetState((Handle)theText, hState);
			}
		} else {
			/* It's a picture, so do it by hand */
			if(theStyle.stInfo.graphicStyle.graphicInfo != nil) {
				if((**theStyle.stInfo.graphicStyle.graphicInfo).itemProc != nil) {
					errCode = CallGraphic(docInfo, theStyle.stInfo.graphicStyle.graphicInfo, (lineBreak->lsStartChar - (**docInfo).textOffset) + textStart, peGraphicResize, &lineWidth);
					if(errCode == cantDoThatInCurrentMode) {
						errCode = noErr;
					} else if(errCode) {
						return errCode;
					}
				}
				theStyle.descent = (**theStyle.stInfo.graphicStyle.graphicInfo).descent;
				theStyle.ascent = (**theStyle.stInfo.graphicStyle.graphicInfo).height - theStyle.descent;
				if((**(theStyle.stInfo.graphicStyle.graphicInfo)).forceLineBreak) {
					textWidth = 0L;
				} else {
					textWidth -= (**(theStyle.stInfo.graphicStyle.graphicInfo)).width;
				}
			}
			if(textWidth > 0L) {
				breakCode = smBreakOverflow;
				textOffset = textEnd;
			} else {
				breakCode = smBreakWord;
				if(firstScriptRun && (textStart == 0L)) {
					textOffset = textEnd;
				} else {
					textOffset = textStart;
				}
			}
		}
		
		/* If this is the end, reset some of the variables */
		if(breakCode != smBreakOverflow) {
			scriptRunLen = 0L;
			styleRunLen = textOffset - textStart;
		}

		/* Subtract from the script run length: What's left might be on the next line */
		lineBreak->lsScriptRunLen -= styleRunLen;
		
		if(styleRunLen > 0L) {
			hState = HGetState((Handle)theText);
			HLock((Handle)theText);

			/* Adjust the line height if needed */
			AddRunHeight(*theText + (lineBreak->lsStartChar - (**docInfo).textOffset) + textStart,
			             styleRunLen,
			             &theStyle,
			             &lineBreak->lsMeasure,
			             (**(**docInfo).globals).whitespaceGlobals,
			             tempStyleRun.srIsGraphic);

			HSetState((Handle)theText, hState);
		}
		
		/* Get ready to load up the next style run */
		if(runIndex == loadParam.lpStyleRunIndex) {
			/* If that was the last style run in a script run that couldn't be loaded
			 * completely, then get the rest of the style run and reload.
			 */
			styleRunLen = tempStyleRun.srStyleLen - loadParam.lpStyleRunLen;
			loadParam.lpLenRequest = loadParam.lpScriptRunLeft;
			goto LoadTextAndLoop;
		} else {
			/* Get the next style run */
			styleRunLen = GetStyleRun(&tempStyleRun, paraInfo, ++runIndex);
			/* Is this the last style run in a script run that couldn't all load? */
			if(runIndex == loadParam.lpStyleRunIndex) {
				styleRunLen = loadParam.lpStyleRunLen;
			}
		}
		
		/* If there's something left in this script run, just advance through it */
		if(lineBreak->lsScriptRunLen != 0L) {
			textStart = textEnd;
			textEnd += styleRunLen;
		} else if((styleRunLen < 0) &&
		          !theStyle.stIsGraphic &&
		          (((tempChar = (*theText)[lineBreak->lsStartChar + textOffset - (**docInfo).textOffset - 1L]) == kCarriageReturnChar) ||
		           (tempChar == kPageDownChar))) {
			loadParam.lpScriptRunLeft = 0L;
		/* Otherwise start the next script run */
		} else {

			firstScriptRun = false;
		
			/* Get the length of the entire script run */
			loadParam.lpLenRequest = lineBreak->lsScriptRunLen = ScriptRunLen(theStyleTable, paraInfo, runIndex, nil);

	LoadTextAndLoop :
			/* Advance to the start of the next script run */
			lineBreak->lsStartChar += scriptRunLen;

			/* Start at the begining of the script run and measure the style run */
			textStart = 0L;
			textEnd = styleRunLen;
			
			/* Load as much of the script run as possible */
			errCode = LoadText(docInfo, lineBreak->lsStartChar, &loadParam, false);
			if(errCode != noErr) {
				return errCode;
			}
			scriptRunLen = loadParam.lpLenRequest;
		}

		/* If that was the end of the paragraph, that's the end */
		if((breakCode == smBreakOverflow) && (tempStyleRun.srStyleLen == -1L)) {
			textOffset = 0L;
			loadParam.lpScriptRunLeft = -(lineBreak->lsScriptRunLen + 1L);
			breakCode = smBreakWord;
		}
		
	/* Keep going until a line break */
	} while(breakCode == smBreakOverflow);
	lineBreak->lsStartChar += textOffset;
	lineBreak->lsScriptRunLen += loadParam.lpScriptRunLeft;
	UnloadText(docInfo);
	
	if(lineBreak->lsMeasure.height == 0L) {
		GetDefaultLineHeight(docInfo, paraIndex, &lineBreak->lsMeasure);
	}
	
	ResetPortInfo(&savedPortInfo);
	return noErr;
}

pascal StyledLineBreakCode MyStyledLineBreak(Ptr textPtr, long textLen, long textStart, long textEnd, long flags, short *textWidth, long *textOffset, PETEGlobalsHandle globals)
{
#pragma unused(flags)
	
	short breakOffset, curDirection;
	Boolean leadingEdge;
	Point scaling;
	long scriptFlags, itlOffset, itlLength, tempOffset;
	Ptr tempText;
	OffsetTable offsets;
	ScriptCode script;
	Handle itlHandle;
	Byte hState;
	char *table;
	
	curDirection = GetSysDirection();
	script = FontScript();
	scriptFlags = GetScriptVariable(script, smScriptFlags);

	scaling.h = scaling.v = 1;
	
	/* Set the line direction */
	if(curDirection != 0) {
		SetSysDirection(0);
	}
	
	if(scriptFlags & (1 << smsfReverse)) {
		SetScriptVariable(script, smScriptFlags, scriptFlags & ~(1 << smsfReverse));
	}
	breakOffset = MyPixelToChar(textPtr + textStart, textEnd - textStart, 0L, *textWidth, &leadingEdge, textWidth, onlyStyleRun, scaling, scaling, globals);
	if(scriptFlags & (1 << smsfReverse)) {
		SetScriptVariable(script, smScriptFlags, scriptFlags);
	}
	
	if(curDirection != 0) {
		SetSysDirection(curDirection);
	}
		
	if((breakOffset == 0) && (*textOffset == 0L)) {
		*textOffset = textStart;
		return smBreakWord;
	}
	
	for(tempText = textPtr + textStart, tempOffset = 0; tempOffset < breakOffset; ++tempOffset) {
		if((*tempText == kPageDownChar) || (*tempText == kLineFeedChar)) {
			break;
		} else if (*tempText++ == kCarriageReturnChar) {
			if((textStart + tempOffset + 1L < textLen) && (*tempText == kLineFeedChar)) {
				continue;
			} else {
				break;
			}
		}
	}
	
	if(tempOffset < breakOffset)  {
		*textOffset = textStart + tempOffset + 1;
		return smBreakWord;
	}
	
	if((breakOffset == textEnd - textStart) && leadingEdge) {
		*textOffset = breakOffset + textStart;
		return smBreakOverflow;
	}

	if(!(scriptFlags & (1 << smsfSingByte))) {
		GetWhitespaceGlobals((**globals).whitespaceGlobals, script);
		table = &(**(**globals).whitespaceGlobals).table[0];
	}

	for(tempOffset = 0L; textStart + breakOffset > SHRT_MAX; ) {
		unsigned char tempChar;
		do {
			
			--textStart;
			--textLen;
			--breakOffset;
			++tempOffset;
			tempChar = *textPtr++;
		} while(!(scriptFlags & (1 << smsfSingByte)) && table[tempChar]);
	}
	
	if((script != smRoman) || ((itlOffset = (**globals).romanWordWrapOffset), ((itlHandle = (**globals).romanWordWrapTable) == nil)) || (*itlHandle == nil)) {
		GetIntlResourceTable(script, smWordWrapTable, &itlHandle, &itlOffset, &itlLength);
	}
	
	if(script == smRoman) {
		(**globals).romanWordWrapTable = itlHandle;
		(**globals).romanWordWrapOffset = itlOffset;
	}
	
	hState = HGetState(itlHandle);
	HLock(itlHandle);
	
	FindWordBreaks(textPtr, textLen > SHRT_MAX ? SHRT_MAX : textLen, breakOffset + textStart, leadingEdge, (BreakTablePtr)(*itlHandle + itlOffset), offsets, script);
	
	HSetState(itlHandle, hState);
	
	if((!curDirection == !(Byte)GetScriptVariable(script, smScriptRight)) && IsWhitespace((StringPtr)textPtr + offsets[0].offFirst, offsets[0].offSecond - offsets[0].offFirst, script, (**globals).whitespaceGlobals, nil)) {
		*textOffset = offsets[0].offSecond + tempOffset;
		return smBreakWord;
	}
	
	if ((offsets[0].offFirst != 0) || (*textOffset == 0)) {
		*textOffset = offsets[0].offFirst + tempOffset;
		return smBreakWord;
	}
	
	if(!leadingEdge) {
		--breakOffset;
		if(!(scriptFlags & (1 << smsfSingByte)) && table[*(textPtr + breakOffset + textStart - 1L)]) {
			--breakOffset;
		}
	}
	
	*textOffset = breakOffset + textStart + tempOffset;
	if(*textOffset <= 0L) {
		*textOffset = 1L;
		if(!(scriptFlags & (1 << smsfSingByte)) && table[*(textPtr + 1L)]) {
			++*textOffset;
		}
	}
	
	return smBreakChar;	
}

/* Add an amount to each of the line start entries */
void OffsetLineStarts(LSTable lineStarts, long startOffset, long textOffset)
{
	long startLineIndex, lineIndex, numLines;
	
	/* If there's nothing to add, just return */
	if(textOffset <= 0L)
		return;
	
	/* If there's no line start array, just return */
	if((lineStarts == nil) || (*lineStarts == nil))
		return;
	
	/* Get the number of lines in the array */
	numLines = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
	
	/* Look for first line after startOffset */
	for(startLineIndex = 0L; (startLineIndex < numLines && (*lineStarts)[startLineIndex].lsStartChar <= startOffset) || (startLineIndex == numLines && (*lineStarts)[startLineIndex].lsStartChar < startOffset); ++startLineIndex) {
		/* Just loop through */
		;
	}
	
	/* Loop through the line starts, adding the offset to each */
	for(lineIndex = startLineIndex; lineIndex <= numLines; ++lineIndex) {
		(*lineStarts)[lineIndex].lsStartChar += textOffset;
	}
	
}

void SubtractFromLineStarts(ParagraphInfoHandle paraInfo, LSTable lineStarts, long startOffset, long textOffset, Boolean removeLines)
{
	long startLineIndex, lineIndex, deleteLineIndex, numLines;

	/* If there's nothing to subtract, just return */
	if(textOffset <= 0L)
		return;
	
	/* If there's no line start array, just return */
	if(((lineStarts == nil) && ((paraInfo == nil) || ((lineStarts = (**paraInfo).lineStarts) == nil))) || (*lineStarts == nil))
		return;
	
	/* Get the number of lines in the array */
	numLines = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
	
	/* Look for first line after startOffset */
	for(startLineIndex = 0L; startLineIndex <= numLines && (*lineStarts)[startLineIndex].lsStartChar < startOffset; ++startLineIndex) {
		/* Just loop through */
		;
	}
	
	if(removeLines && (startLineIndex <= numLines)) {

		deleteLineIndex = startLineIndex;

		for(lineIndex = deleteLineIndex; lineIndex <= numLines && startOffset + textOffset >= (*lineStarts)[lineIndex + 1L].lsStartChar; ++lineIndex) {
			/* Just loop through */
			;
		}

		if(lineIndex > deleteLineIndex) {

			if(paraInfo != nil) {
				if(deleteLineIndex <= 0L) {
					(**paraInfo).lineStartCacheIndex = 0L;
					(**paraInfo).lineStartCacheHeight = 0L;
				} else {
					ResetLineCache(paraInfo, deleteLineIndex - 1L);
				}
			}
			Munger((Handle)lineStarts,
			       sizeof(LSElement) * deleteLineIndex,
			       nil,
			       sizeof(LSElement) * (lineIndex - deleteLineIndex),
			       (Ptr)1L,
			       0L);
			numLines -= (lineIndex - deleteLineIndex);
		}		
	}
	
	/* Loop through the line starts, subtracting the offset to each */
	for(lineIndex = startLineIndex; lineIndex <= numLines; ++lineIndex) {
		if((lineIndex != 0L) && ((*lineStarts)[lineIndex].lsStartChar < textOffset)) {
			(*lineStarts)[lineIndex].lsStartChar = (*lineStarts)[lineIndex - 1L].lsStartChar;
		} else {
			(*lineStarts)[lineIndex].lsStartChar -= textOffset;
		}
	}

}

void RecalcScriptRunLengths(DocumentInfoHandle docInfo, long paraNum, long startOffset)
{
	long numLines, lineIndex, runIndex, tempOffset, tempLen, paraOffset;
	LSTable lineStarts;
	ParagraphInfoHandle paraInfo;

	paraInfo = (**docInfo).paraArray[paraNum].paraInfo;

	/* If there's no line start array, just return */
	if(((lineStarts = (**paraInfo).lineStarts) == nil) || (*lineStarts == nil))
		return;
	
	paraOffset = ParagraphOffset(docInfo, paraNum);
	
	/* Get the number of lines in the array */
	numLines = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement);
	
	for(lineIndex = 0L; lineIndex < numLines && (*lineStarts)[lineIndex].lsStartChar <= startOffset; ++lineIndex) {
		if((lineIndex == 0L) || ((*lineStarts)[lineIndex - 1L].lsScriptRunLen + (*lineStarts)[lineIndex - 1L].lsStartChar < (*lineStarts)[lineIndex].lsStartChar)) {
			tempOffset = (*lineStarts)[lineIndex].lsStartChar - paraOffset;
			runIndex = StyleRunIndex(paraInfo, &tempOffset);
			tempLen = ScriptRunLen((**docInfo).theStyleTable, paraInfo, runIndex, nil);
			(*lineStarts)[lineIndex].lsScriptRunLen = tempLen - tempOffset;
		} else {
			(*lineStarts)[lineIndex].lsScriptRunLen = (*lineStarts)[lineIndex - 1L].lsScriptRunLen - ((*lineStarts)[lineIndex].lsStartChar - (*lineStarts)[lineIndex - 1L].lsStartChar);
		}
	}
}

OSErr ResetAndInvalidateDocument(DocumentInfoHandle docInfo)
{
	long paraIndex;
	
	FlushDocInfoLineCache(docInfo);
	for(paraIndex = (**docInfo).paraCount; --paraIndex >= 0L; ) {
		FlushStyleRunCache((**docInfo).paraArray[paraIndex].paraInfo);
		SetParaDirty(docInfo, paraIndex);
	}
	InvalidateDocument(docInfo, true);
	(**docInfo).flags.selectionDirty = true;
	(**docInfo).flags.scrollsDirty = true;
	return noErr;
}

OSErr ForceRecalc(DocumentInfoHandle docInfo, long startOffset, long endOffset)
{
	long paraIndex, endParaIndex, curOffset, endRunOffset;
	OSErr errCode;

	if(((startOffset < -1L) || (endOffset < -1L)) && (startOffset != endOffset)) {
		return errAEImpossibleRange;
	}
	
	/* Turn off caret just in case there's a redraw */
	EraseCaret(docInfo);
	
	/* Fix up the offsets */
	if(endOffset < 0L) {
		endOffset = (**docInfo).selEnd.offset;
	} else if(endOffset > (**docInfo).textLen) {
		endOffset = (**docInfo).textLen;
	}
	
	if(startOffset < 0L) {
		startOffset = (**docInfo).selStart.offset;
	}
	
	if(startOffset > endOffset) {
		return errAEImpossibleRange;
	} else if(startOffset == endOffset) {
		return noErr;
	}
	
	DeleteIdleGraphics(docInfo, startOffset, endOffset);
	NewIdleGraphics(docInfo, startOffset, endOffset);
	
	/* Get the starting and ending paragraphs */
	paraIndex = ParagraphIndex(docInfo, startOffset);
	endParaIndex = ParagraphIndex(docInfo, endOffset - 1L);

	/* Get the offsets into the starting and ending paragraph */
	curOffset = startOffset - ParagraphOffset(docInfo, paraIndex);
	endRunOffset = endOffset - ParagraphOffset(docInfo, endParaIndex);

	for(errCode = noErr; errCode == noErr && paraIndex <= endParaIndex; ++paraIndex)
	{
		errCode = DoParaRecalc(docInfo, paraIndex, curOffset, paraIndex == endParaIndex ? endRunOffset : LONG_MAX, 0L, 0L, peGraphicValid);
	}
	return errCode;
}

OSErr DoParaRecalc(DocumentInfoHandle docInfo, long paraIndex, long startOffset, long endOffset, long startStyleRun, long endStyleRun, long validBits)
{
	ParagraphInfoHandle paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	LSTable oldLineStarts;
	OSErr errCode = noErr;
	
	if((**docInfo).flags.inGraphicCall) return cantDoThatInCurrentMode;
	CompressStyleRuns(paraInfo, (**docInfo).theStyleTable, startStyleRun, endStyleRun - startStyleRun);
	FlushStyleRunCache(paraInfo);
	if((validBits & ~peLockValid) != 0) {
		if(validBits & peFontValid) {
			RecalcScriptRunLengths(docInfo, paraIndex, startOffset);
		}
		oldLineStarts = (**paraInfo).lineStarts;
		if(MyHandToHand((Handle *)&oldLineStarts, hndlTemp) == noErr) {
			errCode = RedrawParagraph(docInfo, paraIndex, startOffset, endOffset, oldLineStarts);
			DisposeHandle((Handle)oldLineStarts);
		}
	}
	return errCode;
}
