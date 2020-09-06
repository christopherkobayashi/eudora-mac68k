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

long LineHeights(LSTable lineStarts, long startIndex, long endIndex)
{
	long height = 0;
	
	while(--endIndex >= startIndex) {
		height += (*lineStarts)[endIndex].lsMeasure.height;
	}
	return height;
}

long ResetLineCache(ParagraphInfoHandle paraInfo, long lineIndex)
{
	long curIndex;
	long curHeight;
	LSTable lineStarts;
	
	lineStarts = (**paraInfo).lineStarts;
	curIndex = (**paraInfo).lineStartCacheIndex;
	if(curIndex > lineIndex) {
		if(curIndex - lineIndex > lineIndex) {
			curHeight = LineHeights(lineStarts, 0L, lineIndex);
		} else {
			curHeight = (**paraInfo).lineStartCacheHeight - LineHeights(lineStarts, lineIndex, curIndex);
		}
		curIndex = lineIndex;
	} else {
		curHeight = (**paraInfo).lineStartCacheHeight;
	}
	curHeight += LineHeights(lineStarts, curIndex, lineIndex);
	(**paraInfo).lineStartCacheIndex = lineIndex;
	return ((**paraInfo).lineStartCacheHeight = curHeight);
}

long ResetLineCacheToVHeight(ParagraphInfoHandle paraInfo, long vHeight, Boolean previousLine)
{
	long curIndex;
	long curHeight;
	LSTable lineStarts;
	
	lineStarts = (**paraInfo).lineStarts;
	curIndex = (**paraInfo).lineStartCacheIndex;
	curHeight = (**paraInfo).lineStartCacheHeight;
	if(vHeight < curHeight) {
		do {
			curHeight -= (*lineStarts)[--curIndex].lsMeasure.height;
		} while(vHeight < curHeight);
		if(!previousLine) {
			curHeight += (*lineStarts)[curIndex++].lsMeasure.height;
		}
	} else {
		do {
			curHeight += (*lineStarts)[curIndex++].lsMeasure.height;
		} while(vHeight >= curHeight);
		if(previousLine) {
			curHeight -= (*lineStarts)[--curIndex].lsMeasure.height;
		}
	}
	(**paraInfo).lineStartCacheIndex = curIndex;
	return((**paraInfo).lineStartCacheHeight = curHeight);
}

/* Get the character offset of the first character in a paragraph */
long ParagraphOffset(DocumentInfoHandle docInfo, long paraIndex)
{
	long textOffset, paraCount;
	
	/* Loop through all of the paragraphs before the indicated one */
	for(textOffset = 0L, paraCount = 0L; paraCount < paraIndex && paraCount < (**docInfo).paraCount; ++paraCount) {
		/* Add the length of the paragraph to the offset */
		textOffset += (**docInfo).paraArray[paraCount].paraLength;
	}
	return textOffset;
}

long ParagraphIndex(DocumentInfoHandle docInfo, long textOffset)
{
	long paraIndex, paraCount, totalLength;
	
	/* Loop through the paragraphs until the one that contains textOffset */
	for(totalLength = 0L, paraCount = (**docInfo).paraCount, paraIndex = 0L;
	    paraIndex < paraCount && (totalLength += (**docInfo).paraArray[paraIndex].paraLength) <= textOffset;
	    ++paraIndex) {
		;
	}
	
	if(paraIndex < paraCount) {
		return paraIndex;
	} else if(textOffset == totalLength) {
		return paraIndex - 1L;
	} else {
		return -1L;
	}
		
}

OSErr ParagraphPosition(DocumentInfoHandle docInfo, long paraIndex, long *vPosition)
{
	OSErr errCode;
	long tempIndex;
	
	if(paraIndex >= (**docInfo).paraCount) {
		return invalidIndexErr;
	}
	*vPosition = 0L;
	for(tempIndex = 0L; tempIndex < paraIndex; ++tempIndex) {
		errCode = CheckParagraphMeasure(docInfo, paraIndex, false);
		if(errCode != noErr) {
			return errCode;
		}
		*vPosition += (**docInfo).paraArray[tempIndex].paraHeight;
	}
	return noErr;
}

/* Find the total length (in bytes) of the styles in the same font script */
long ScriptRunLen(LongSTTable theStyleTable, ParagraphInfoHandle paraInfo, long styleRunIndex, long *nextStyleRun)
{
	long lastLen, runLen;
	short currentScript, lastScript;
	LongStyleRun styleRun;
	LongSTElement stElement;
	
	/* smCurrentScript is just used as a flag for the first time through */
	lastScript = smCurrentScript;
	runLen = 0L;
	lastLen = 0L;
	do {
		/* Add the length of the last style run */
		runLen += lastLen;
		
		/* Get the style run and increment our counter */
		lastLen = GetStyleRun(&styleRun, paraInfo, styleRunIndex++);
		
		/* End of paragraph marker, so we're done */
		if(styleRun.srStyleLen < 0L) {
			if(lastScript == smCurrentScript) {
				runLen = -1L;
			}
			break;
		}
		
		/* Get the style associated with this run */
		GetStyle(&stElement, theStyleTable, styleRun.srStyleIndex);
		
		/* Check to see if the font is in the same script as the last one, */
		/* and if it's smCurrentScript, just assign the current script to it */ 
	} while((lastScript == (currentScript = StyleToScript(&stElement.stInfo.textStyle))) ||
	        ((lastScript == smCurrentScript) && ((lastScript = currentScript), true)));
	
	if(nextStyleRun != nil) {
		*nextStyleRun = (runLen == -1L) ? -1L : styleRunIndex - 1L;
	}
	
	return runLen;
}

/* Find the style run given the offset into the para and calc the offset into that run */
long StyleRunIndex(ParagraphInfoHandle paraInfo, long *charOffset)
{
	long styleRunIndex;
	LongStyleRun styleRun;
	
	if(*charOffset < (**paraInfo).styleRunCacheOffset) {
		FlushStyleRunCache(paraInfo);
	}
	styleRunIndex = (**paraInfo).styleRunCacheIndex;
	*charOffset -= (**paraInfo).styleRunCacheOffset;
	while(true) {
		/* Get the next style run */
		GetStyleRun(&styleRun, paraInfo, styleRunIndex);
		
		/* Make sure the end of the paragraph wasn't hit */
		if(styleRun.srStyleLen < 0L) {
			*charOffset = -(*charOffset);
			break;
		}

		/* Is it past this style run? */
		if(*charOffset < styleRun.srStyleLen) {
			break;
		}
		
		/* Subtract off the length of this style run */
		*charOffset -= styleRun.srStyleLen;
		++styleRunIndex;
		
		(**paraInfo).styleRunCacheIndex = styleRunIndex;
		(**paraInfo).styleRunCacheOffset += styleRun.srStyleLen;
	}

	return styleRunIndex;
}

/* Find the byte length of the first and last style run and return the number of runs */
short CalcRunLengths(long startChar, long endChar, DirectionParamPtr dirParam)
{
	long startOffset, endOffset, startIndex, endIndex;
	LongStyleRun tempStyleRun;

	/* Save the offsets for future use */
	startOffset = startChar -= dirParam->paraOffset;
	endOffset = endChar -= dirParam->paraOffset;

	if(startOffset == endOffset) {
		startIndex = CountStyleRuns(dirParam->paraInfo) - 1L;
		endIndex = startIndex - 1L;
	} else {
	
		/* Get the style run index for the starting position */
		startIndex = StyleRunIndex(dirParam->paraInfo, &startChar);
	
		/* Move back one byte since endChar actually follows the last byte */
		--endChar;
		/* Get the style run index for the ending position */
		endIndex = StyleRunIndex(dirParam->paraInfo, &endChar);
	}
	
	/* Does this start and end in the same style run? */
	if(endIndex > startIndex) {
		/* Length is just one more than how far into the run it is */
		dirParam->endLen = endChar + 1;
		/* Get the style run information for the first run */
		GetStyleRun(&tempStyleRun, dirParam->paraInfo, startIndex);
		/* Subtract off the offset into the run */
		dirParam->startLen = (tempStyleRun.srStyleLen - startChar);
	} else {
		/* If this is all within one style run, use the full length */
		dirParam->startLen = dirParam->endLen = endOffset - startOffset;
	}

	/* Set the base style run to the starting style run */
	dirParam->baseStyleRun = startIndex;
	
	/* Return the number of style runs */
	return (endIndex - startIndex) + 1;
	
}

/* Find the first line that appears after drawTop */
OSErr FindFirstVisibleLine(DocumentInfoHandle docInfo, LineInfoPtr lineInfo, short drawTop)
{
	long vPosition;
	long paraIndex;
	Byte hState;
	OSErr errCode;
	
	vPosition = (**docInfo).vPosition;
	vPosition += (drawTop - (**docInfo).viewRect.top);
	
	if(vPosition < 0L) {
		errCode = errOffsetIsOutsideOfView;
	} else {
		errCode = noErr;
	}
	
	for(paraIndex = 0L; errCode == noErr && paraIndex < (**docInfo).paraCount; ++paraIndex) {
		errCode = CheckParagraphMeasure(docInfo, paraIndex, false);
		if(errCode == noErr) {
			if(vPosition <= (**docInfo).paraArray[paraIndex].paraHeight) {
				break;
			}
			vPosition -= (**docInfo).paraArray[paraIndex].paraHeight;
		}
	}
	
	if(paraIndex == (**docInfo).paraCount) {
		errCode = errOffsetIsOutsideOfView;
	}
	
	if(errCode == noErr) {
		errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
	}
	
	if(errCode != noErr) {
		lineInfo->paraIndex = -1L;
		return errCode;
	}
	
	hState = HGetState((Handle)(**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts);
	HNoPurge((Handle)(**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts);
	vPosition -= ResetLineCacheToVHeight((**docInfo).paraArray[paraIndex].paraInfo, vPosition, true);
	HSetState((Handle)(**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts, hState);
	
	lineInfo->vPosition = drawTop - vPosition;
	lineInfo->paraIndex = paraIndex;
	lineInfo->lineIndex = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStartCacheIndex;
	return noErr;
}

/* Calculate the vertical position given a character offset into the text */
OSErr OffsetToVPosition(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean preserveLine)
{
	long charOffset, nextOffset, lineIndex, lineCount, paraIndex;
	long vOffset;
	LSTable lineStarts;
	OSErr errCode;
	Byte hState;
	
	/* Initialize variables to top of document */
	vOffset = 0L;
	nextOffset = paraIndex = 0L;
	lineStarts = nil;
	
	/* Loop through the paragraphs looking for the given offset */
	do {
		if(lineStarts != nil) {
			HSetState((Handle)lineStarts, hState);
		}

		/* Save the current vertical position */
		selection->vPosition = vOffset;
		
		/* If the current paragraph hasn't been measured, measure it */
		errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
		if(errCode != noErr) {
			return errCode;
		}
		
		/* Get the line starts information for the paragraph */
		lineStarts = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts;
		hState = HGetState((Handle)lineStarts);
		HNoPurge((Handle)lineStarts);
		lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
		
		/* Add the height of the paragraph */
		vOffset += (**docInfo).paraArray[paraIndex].paraHeight;

		if(preserveLine) {
			if(paraIndex == selection->paraIndex) {
				lineIndex = selection->lineIndex;
				if((lineIndex >= 0L) && (lineIndex <= lineCount)) {
					break;
				} else {
					preserveLine = false;
				}
			}
		}
		if(!preserveLine) {
			/* Get the number of lines in this paragraph */
			lineIndex = lineCount + 1L;

			/* Look for the character offset in this paragraph */
			do {
				charOffset = nextOffset;
				--lineIndex;
				nextOffset = (*lineStarts)[lineIndex].lsStartChar;
			} while(nextOffset > selection->offset && lineIndex > 0L);
			
			if((nextOffset == selection->offset) && (selection->lastLine || ((**docInfo).paraArray[paraIndex].paraLength == 0L))) {
				charOffset = nextOffset;
				--lineIndex;
			}
			
			/* If the offset has been past, break */
			if(charOffset >= selection->offset) {
				break;
			}
		}
		
	} while(++paraIndex < (**docInfo).paraCount);
	
	/* If the offset is past the end of the document, fill the result with -1 */
	if(paraIndex >= (**docInfo).paraCount) {
		paraIndex = (**docInfo).paraCount - 1L;
		lineIndex = lineCount - 1L;
	}
	
	/* Fill in the correct paragraph and line number */
	selection->paraIndex = paraIndex;
	selection->lineIndex = lineIndex;
	
	/* Save the height of this line */
	selection->vLineHeight = (*lineStarts)[lineIndex].lsMeasure.height;
	selection->vLineAscent = (*lineStarts)[lineIndex].lsMeasure.ascent;
	
	if(lineIndex != 0L) {
		/* Add the height of the lines above to the vertical position */
		selection->vPosition += ResetLineCache((**docInfo).paraArray[paraIndex].paraInfo, lineIndex);
	}
	
	HSetState((Handle)lineStarts, hState);
		
	return noErr;
} 

/* Calculate the horizontal position given character offset and paragraph/line number */
OSErr OffsetToHPosition(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	LayoutCache lineCache;
	LongSTElement tempStyle;
	OrderHandle orderingHndl;
	StringHandle theText;
	short lPosition, rPosition, lMargin, rMargin, width;
	JustStyleCode styleRunPosition;
	short runIndex, numRuns, len, curDirection, charDirection, pixelLoc;
	long offset;
	Point scaling;
	Ptr textPtr;
	char spaceChar;
	Byte hState;
	Boolean leftDone, rightDone, leftToRight, measureOnce;
	ParagraphInfoHandle paraInfo;
	PETEPortInfo savedPortInfo;
	LoadParams loadParam;
	OSErr errCode;

	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);

	/* Get the line layout for the selected line/paragraph */
	lineCache.paraIndex = selection->paraIndex;
	lineCache.lineIndex = selection->lineIndex;
	errCode = GetLineLayout(docInfo, &lineCache);
	
	if(errCode != noErr) {
		return errCode;
	}
	
	/* Initialize variables */
	orderingHndl = lineCache.orderingHndl;
	theText = (**docInfo).theText;
	numRuns = InlineGetHandleSize((Handle)orderingHndl) / sizeof(OrderEntry);
	lPosition = rPosition = lineCache.leftPosition;
	styleRunPosition = (numRuns == 1) ? onlyStyleRun : leftStyleRun;
	
	/* If the current paragraph hasn't been measured, measure it */
	errCode = CheckParagraphMeasure(docInfo, selection->paraIndex, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	/* Get the line starts information for the paragraph */
	paraInfo = (**docInfo).paraArray[selection->paraIndex].paraInfo;

	if(numRuns == 0) {
		rightDone = leftDone = true;
	} else if((selection->offset == (*(**paraInfo).lineStarts)[selection->lineIndex + 1].lsStartChar) && ((**paraInfo).direction != leftCaret)) {
		leftDone = false;
		rightDone = true;
	} else if((selection->offset == (*(**paraInfo).lineStarts)[selection->lineIndex].lsStartChar) && ((**paraInfo).direction == leftCaret)) {
		leftDone = true;
		rightDone = false;
	} else {
		leftDone = rightDone = false;
	}
	
	pixelLoc = -1L;
	
	measureOnce = (!leftDone && !rightDone && ((**paraInfo).direction == leftCaret) && !(Boolean)GetScriptManagerVariable(smBidirect));
	
	/* Loop through style runs from left to right looking for the offset */
	for(runIndex = 0; runIndex < numRuns && !(leftDone && rightDone); ++runIndex) {
		/* Get the offset of the first character in the style run */
		offset = (*orderingHndl)[runIndex].offset;
		/* Get the length of the style run */
		len = (*orderingHndl)[runIndex].len;
		/* Get the width of the style run */
		width = (*orderingHndl)[runIndex].width;
		
		/* Is the offset within this style run? */
		if((selection->offset < offset) || (selection->offset > offset + len)) {
			/* No, so just advance the position to the next style run */
			if(!leftDone) {
				lPosition += width;
			}
			if(!rightDone) {
				rPosition += width;
			}
		} else {
			/* Set the text style */
			GetStyle(&tempStyle, (**docInfo).theStyleTable, (*orderingHndl)[runIndex].styleIndex);
			SetTextStyleWithDefaults(nil, docInfo, &tempStyle.stInfo.textStyle, false, ((**docInfo).printData != nil));

			/* Is this regular text, or a tab or graphic? */
			if((*orderingHndl)[runIndex].isGraphic || (*orderingHndl)[runIndex].isTab) {
				leftToRight = ((Byte)GetScriptVariable(smCurrentScript, smScriptRight) == 0);
				if(leftToRight) {
					spaceChar = kSpaceChar;
				} else {
					spaceChar = kRightToLeftSpaceChar;
				}
				textPtr = &spaceChar;
				len = 1;
				if(offset != selection->offset) {
					offset = selection->offset - 1;
				}
			} else {
				loadParam.lpLenRequest = len;
				errCode = LoadText(docInfo, offset, &loadParam, true);

				if(errCode != noErr) {
					return errCode;
				}
				
				hState = HGetState((Handle)theText);
				HLock((Handle)theText);
				
				/* Find the appropriate place in the text */
				textPtr = (Ptr)*theText + (offset - (**docInfo).textOffset);
	
				leftToRight = !(Boolean)GetScriptVariable(smCurrentScript, smScriptRight) || ((CharacterType(textPtr, (offset == selection->offset) ? 0 : (len - 1), smCurrentScript) & (smcDoubleMask | smcRightMask)) != smcRightMask);
			}
			
			if((offset == selection->offset) || (offset + len == selection->offset)) {
				if(leftToRight) {
					if(leftDone) {
						rPosition = lPosition;
						rightDone = true;
					} else {
						charDirection = leftCaret;
						if(!rightDone) {
							rPosition += width;
						}
					}
				} else {
					if(rightDone) {
						lPosition = rPosition;
						leftDone = true;
					} else {
						charDirection = rightCaret;
						if(!leftDone) {
							lPosition += width;
						}
					}
				}
			} else {
				charDirection = kHilite;
			}

			/* Set the line direction */
			curDirection = GetSysDirection();
			SetSysDirection((**paraInfo).direction);

			/* Initialize scaling factors to 1:1 */
			scaling.h = scaling.v = 1;
			
			if((charDirection != rightCaret) && !leftDone) {
				if(!measureOnce || (pixelLoc < 0L)) {
					pixelLoc = MyCharToPixel(textPtr, len, 0L, selection->offset - offset, leftCaret, styleRunPosition, scaling, scaling, (**docInfo).globals);
				}
				if((textPtr == &spaceChar) && (pixelLoc != 0)) {
					lPosition += width;
				} else {
					lPosition += pixelLoc;
				}
				leftDone = true;
			}
			if((charDirection != leftCaret) && !rightDone) {
				if(!measureOnce || (pixelLoc < 0L)) {
					pixelLoc = MyCharToPixel(textPtr, len, 0L, selection->offset - offset, rightCaret, styleRunPosition, scaling, scaling, (**docInfo).globals);
				}
				if((textPtr == &spaceChar) && (pixelLoc != 0)) {
					rPosition += width;
				} else {
					rPosition += pixelLoc;
				}
				rightDone = true;
			}

			/* Restore line direction */
			SetSysDirection(curDirection);
			
			if(textPtr != &spaceChar) {
				HSetState((Handle)theText, hState);
				UnloadText(docInfo);
			}
		}
		
		
		/* Advance the style run position indicator */
		styleRunPosition = (runIndex == numRuns - 2) ? rightStyleRun : middleStyleRun;
	}
	
	lMargin = (**paraInfo).startMargin;
	rMargin = (**paraInfo).endMargin;
	if(rMargin < 0L) {
		rMargin += (**docInfo).docWidth;
	}
	if((**paraInfo).direction != leftCaret) {
		Exchange(lMargin, rMargin);
		lMargin = (**docInfo).docWidth - lMargin;
		rMargin = (**docInfo).docWidth - rMargin;
	}
	
	if(lPosition < lMargin) {
		lPosition = lMargin;
	}
	if(rPosition < lMargin) {
		rPosition = lMargin;
	}
	if(lPosition > rMargin) {
		lPosition = rMargin;
	}
	if(rPosition > rMargin) {
		rPosition = rMargin;
	}
	
	/* Set the position of the selected character */
	selection->lPosition = lPosition;
	selection->rPosition = rPosition;
	
	ResetPortInfo(&savedPortInfo);
	return noErr;
}

OSErr OffsetToPosition(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean preserveLine)
{
	OSErr errCode;
	
	if((errCode = OffsetToVPosition(docInfo, selection, preserveLine)) == noErr) {
		errCode = OffsetToHPosition(docInfo, selection);
	}
	return errCode;
}

/* Calculate character offset given paragraph/line number and the horizontal position */
long HPositionToOffset(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, short hPosition, short vPosition, Boolean *leadingEdge, Boolean *graphicHit, Boolean *wasGraphic)
{
	short runIndex, numRuns, offset, curDirection, lineDirection;
	JustStyleCode styleRunPosition;
	Point scaling, location;
	Byte hState;
	LongSTElement tempStyle;
	Ptr textPtr;
	StringHandle theText;
	OrderHandle orderingHndl;
	long returnOffset;
	PETEPortInfo savedPortInfo;

	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);

	/* Initialize some stuff */
	theText = (**docInfo).theText;
	orderingHndl = lineCache->orderingHndl;
	numRuns = InlineGetHandleSize((Handle)orderingHndl) / sizeof(OrderEntry);
	styleRunPosition = (numRuns == 1) ? onlyStyleRun : leftStyleRun;
	*graphicHit = false;
	
	/* Initialize scaling factors to 1:1 */
	scaling.h = scaling.v = 1;

	/* Loop through the runs looking for a style run which intersects position */
	for(runIndex = 0; (runIndex < numRuns) && (hPosition >= 0L); ++runIndex) {

		/* Set the text style */
		GetStyle(&tempStyle, (**docInfo).theStyleTable, (*orderingHndl)[runIndex].styleIndex);
		SetTextStyleWithDefaults(nil, docInfo, &tempStyle.stInfo.textStyle, false, ((**docInfo).printData != nil));

		hState = HGetState((Handle)theText);
		HLock((Handle)theText);
		
		/* Find the appropriate place in the text */
		textPtr = (Ptr)*theText + ((*orderingHndl)[runIndex].offset - (**docInfo).textOffset);

		*wasGraphic = (*orderingHndl)[runIndex].isGraphic;
		/* Handle tabs and graphics by hand */
		if(*wasGraphic || (*orderingHndl)[runIndex].isTab) {
			/* Subtract off the width of the run */
			hPosition -= (*orderingHndl)[runIndex].width;

			/* See if the position has been passed */
			if(hPosition < 0L) {
				
				/* Less than half way through is the left side */
				*leadingEdge = (((*orderingHndl)[runIndex].width + hPosition) < ((*orderingHndl)[runIndex].width >> 1));
			
				if((*orderingHndl)[runIndex].isGraphic) {
					location.h = hPosition + (*orderingHndl)[runIndex].width;
					location.v = vPosition;
					*graphicHit = (CallGraphic(docInfo, tempStyle.stInfo.graphicStyle.graphicInfo, (*orderingHndl)[runIndex].offset, peGraphicTest, &location) == noErr);
				}

				/* Set hPosition to indicate the intersection */
				hPosition = -1L;
				
			} else {
				/* Past the end of the line is the right side */
				*leadingEdge = false;
			}
				
			/* If it was right to left text, reverse the sense of leadingEdge */
			if(!(!(Boolean)GetScriptVariable(smCurrentScript, smScriptRight))) {
				*leadingEdge = !*leadingEdge;
			}
			
			/* Set offset to before or after the character */
			offset = *leadingEdge ? 0 : (*orderingHndl)[runIndex].len;
			
		} else {
			/* Set the line direction */
			curDirection = GetSysDirection();
			lineDirection = (**((**docInfo).paraArray[lineCache->paraIndex].paraInfo)).direction;
			SetSysDirection(lineDirection);

			/* Regular text, so just call PixelToChar */
			offset = MyPixelToChar(textPtr, (*orderingHndl)[runIndex].len, 0L, hPosition, leadingEdge, &hPosition, styleRunPosition, scaling, scaling, (**docInfo).globals);

			/* Restore line direction */
			SetSysDirection(curDirection);
		}

		HSetState((Handle)theText, hState);
		
		/* Advance the style run position indicator */
		styleRunPosition = (runIndex == numRuns - 2) ? rightStyleRun : middleStyleRun;

	};
	
	--runIndex;
	
	if((runIndex < 0L) || (hPosition > 0L)) {
		*leadingEdge = false;
		returnOffset = -1L;
	} else {
		returnOffset = (*orderingHndl)[runIndex].offset + offset;
	}
	
	ResetPortInfo(&savedPortInfo);
	
	return returnOffset;
}

OSErr PositionToOffset(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	long vPosition, lastPosition;
	LSTable lineStarts;
	short hPosition;
	long paraIndex, lineCount, lineIndex;
	LayoutCache lineCache;
	LoadParams loadParam;
	OSErr errCode;
	Byte hState;
	unsigned char tempChar;
	Boolean wasGraphic;
	
	/* Set vPosition to the selection location */
	vPosition = selection->vPosition;
	
	/* Loop through the paragraphs */
	for(paraIndex = 0L; paraIndex < (**docInfo).paraCount; ++paraIndex) {
		/* Save last position */
		lastPosition = vPosition;
		
		/* Reflow paragraph if necessary */
		errCode = CheckParagraphMeasure(docInfo, paraIndex, false);
		if(errCode != noErr) {
			return errCode;
		}

		/* Subtract the height of the paragraph */
		vPosition -= (**docInfo).paraArray[paraIndex].paraHeight;
		
		/* Once the position goes negative, the correct paragraph has been reached */
		if(vPosition < 0L)
			break;
	}

	/* If past the end of the document, choose the last paragraph */
	if(paraIndex == (**docInfo).paraCount)
		--paraIndex;

	/* Get the line starts for the paragraph */
	errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
	if(errCode != noErr) {
		return errCode;
	}
	lineStarts = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts;
	hState = HGetState((Handle)lineStarts);
	HNoPurge((Handle)lineStarts);
	
	/* Get the number of lines in the paragraph */
	lineIndex = lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
		
	/* If the position is in a zero length paragraph, then set it to the beginning of that */
	if((**docInfo).paraArray[paraIndex].paraLength == 0L) {
		selection->offset = ParagraphOffset(docInfo, paraIndex);
		selection->paraIndex = paraIndex;
		selection->lineIndex = 0L;
		selection->vLineHeight = (*lineStarts)[0L].lsMeasure.height;
		selection->leadingEdge = true;
		selection->lastLine = false;
		selection->graphicHit = false;
		goto FinalReturn;
	}
	
	/* If the selection is not past the end of this paragraph, search by line */
	if((vPosition < 0L) && (lastPosition >= 0L)) {
	
		/* Reset the position to the distance from the top of the paragraph */
		vPosition = lastPosition;
		
		vPosition -= ResetLineCacheToVHeight((**docInfo).paraArray[paraIndex].paraInfo, vPosition, true);
		lineIndex = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStartCacheIndex;
	}
	
	errCode = noErr;
	wasGraphic = false;
	
	/* If the last value was negative, it's before the beginning of the document */
	if(lastPosition < 0L) {
		selection->offset = 0L;
		paraIndex = 0L;
		lineIndex = 0L;
		selection->leadingEdge = true;
		selection->lastLine = false;
		selection->graphicHit = false;
		goto FillAndReturn;
	}

	/* If the position is past the last line, then set it to the last character */
	if(lineIndex == lineCount) {
		selection->offset = (*lineStarts)[lineIndex].lsStartChar;
		if(lineIndex != 0L) {
			--lineIndex;
		}
		selection->leadingEdge = true;
		selection->lastLine = true;
		selection->graphicHit = false;
		goto FillAndReturn;
	}
	
	loadParam.lpLenRequest = (*lineStarts)[lineIndex + 1L].lsStartChar - (*lineStarts)[lineIndex].lsStartChar;
	errCode = LoadText(docInfo, (*lineStarts)[lineIndex].lsStartChar, &loadParam, true);
	if(errCode != noErr) {
		goto FinalReturn;
	}
	
	lineCache.lineIndex = lineIndex;
	lineCache.paraIndex = paraIndex;
	errCode = GetLineLayout(docInfo, &lineCache);
	if(errCode != noErr) {
		goto FinalReturn;
	}
	
	hPosition = selection->hPosition - lineCache.leftPosition;
	if(hPosition < 0L) {
		if((**(**docInfo).paraArray[paraIndex].paraInfo).direction == leftCaret) {
			selection->offset = (*lineStarts)[lineIndex].lsStartChar;
			selection->leadingEdge = false;
		} else {
			selection->offset = (*lineStarts)[lineIndex + 1].lsStartChar;
			selection->leadingEdge = true;
		}
		selection->graphicHit = false;
	} else {
	
		selection->offset = HPositionToOffset(docInfo, &lineCache, hPosition, vPosition, &selection->leadingEdge, &selection->graphicHit, &wasGraphic);
	
		if(selection->offset < 0L) {
			if((**(**docInfo).paraArray[paraIndex].paraInfo).direction != leftCaret) {
				selection->offset = (*lineStarts)[lineIndex].lsStartChar;
				selection->leadingEdge = false;
			} else {
				selection->offset = (*lineStarts)[lineIndex + 1].lsStartChar;
				selection->leadingEdge = true;
			}
		}
	}
	
	selection->lastLine = ((selection->offset == (*lineStarts)[lineIndex + 1].lsStartChar) && selection->leadingEdge);
	
FillAndReturn :
	selection->paraIndex = paraIndex;
	selection->lineIndex = lineIndex;
	selection->vLineHeight = (*lineStarts)[lineIndex].lsMeasure.height;
	
	if((selection->offset == (*lineStarts)[lineIndex + 1L].lsStartChar) &&
	   ((selection->offset != (**docInfo).textLen) || (lineIndex + 1L < lineCount) || (selection->paraIndex + 1L < (**docInfo).paraCount)) &&
	   (((tempChar = (*(**docInfo).theText)[selection->offset - (**docInfo).textOffset - 1L]) == kCarriageReturnChar) || (tempChar == kPageDownChar) || (tempChar == kLineFeedChar)) &&
	   (!wasGraphic)) {
		--selection->offset;
		selection->lastLine = false;
	}
	
FinalReturn :
	HSetState((Handle)lineStarts, hState);
	UnloadText(docInfo);
	return errCode;
}

OSErr GetHorizontalSelection(DocumentInfoHandle docInfo, SelDataPtr selection, short modifiers, Boolean leftArrow, Boolean gotoEnd)
{
	long anchorOffset, tempOffset, entryCount, entryIndex;
	Boolean wordIsWhitespace, bidirect;
	LayoutCache lineCache;
	OSErr errCode = noErr;

	bidirect = false; // !(!(Boolean)GetScriptManagerVariable(smBidirect));
	anchorOffset = selection->offset;
	
	if(modifiers & cmdKey) {
		if(leftArrow) {
			selection->hPosition = (SHRT_MIN/2);
		} else {
			selection->hPosition = (SHRT_MAX/2);
		}
		errCode = PositionToOffset(docInfo, selection);
		selection->leadingEdge = !selection->leadingEdge;
	} else if(!gotoEnd) {
	MoveOver :
		if(bidirect) {
			/* Must fill in */
			lineCache.paraIndex = selection->paraIndex;
			lineCache.lineIndex = selection->lineIndex;
			errCode = GetLineLayout(docInfo, &lineCache);
			if(errCode == noErr) {
				entryCount = InlineGetHandleSize((Handle)lineCache.orderingHndl) / sizeof(OrderEntry);
				for(entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
					tempOffset = selection->offset - (*lineCache.orderingHndl)[entryIndex].offset;
					if(tempOffset < 0L) {
						continue;
					}
					if(tempOffset == 0L) {
						goto CheckDirection;
					}
					tempOffset -= (*lineCache.orderingHndl)[entryIndex].len;
					if(tempOffset > 0L) {
						continue;
					}
					if(tempOffset == 0L) {
				CheckDirection :
						;
					} else {
						break;
					}
				}
			}
		} else {
			if(leftArrow == selection->leadingEdge) {
				selection->leadingEdge = !selection->leadingEdge;
			}
			selection->lastLine = false;
			if(!(modifiers & optionKey)) {
				selection->offset = GetNextChar(docInfo, selection->offset, leftArrow);
				selection->leadingEdge = leftArrow;
			} else {
				if(leftArrow && (selection->offset > 0L) && (selection->offset == ParagraphOffset(docInfo, selection->paraIndex))) {
					--selection->paraIndex;
				}
				selection->leadingEdge = (!leftArrow || (selection->offset == 0L));
				errCode = GetWordOffset(docInfo, selection, nil, leftArrow ? &selection->offset : nil, leftArrow ? nil : &selection->offset, &wordIsWhitespace, nil);
				selection->leadingEdge = leftArrow;
				if((errCode == noErr) && wordIsWhitespace && ((leftArrow && (selection->offset > 0L)) || (!leftArrow && (selection->offset < (**docInfo).textLen)))) {
					errCode = OffsetToPosition(docInfo, selection, false);
					if(errCode == noErr) {
						goto MoveOver;
					}
				}
			}
		}
	} else {
		if(bidirect) {
			; /* Must fill in */
		} else {
			selection->leadingEdge = leftArrow;
		}
	}
	
	if(errCode == noErr) {
		if(!((**docInfo).flags.ignoreSelectLock)) {
			if(PinSelectionLock(docInfo, anchorOffset, &selection->offset)) {
				selection->lastLine = false;
			}
		}

		errCode = OffsetToPosition(docInfo, selection, false);
		
		selection->hPosition = selection->lPosition;
	}
	
	return errCode;
}

long GetNextChar(DocumentInfoHandle docInfo, long curOffset, Boolean previousChar)
{
	long paraIndex, styleRunIndex, textOffset, newOffset, tempIndex;
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun;
	LongSTElement theStyle;
	LoadParams loadParam;
	CharByteTable table;
	Boolean doubleByte;
	Byte hState;
	StringPtr textPtr;
	ScriptCode script;
	
StartOver :
	if((curOffset == 0L) && previousChar)
		return 0L;
	
	if(curOffset == (**docInfo).textLen) {
		if(!previousChar)
			return curOffset;
		paraIndex = (**docInfo).paraCount - 1L;
	} else {
		paraIndex = ParagraphIndex(docInfo, curOffset);
	}
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	textOffset = ParagraphOffset(docInfo, paraIndex);
	
	if(curOffset == (**docInfo).textLen) {
		styleRunIndex = CountStyleRuns(paraInfo);
		newOffset = 0L;
	} else {
		newOffset = curOffset - textOffset;
		styleRunIndex = StyleRunIndex(paraInfo, &newOffset);
	}
	
	if(previousChar) {
		if(newOffset == 0L) {
			if(styleRunIndex == 0L) {
				paraInfo = (**docInfo).paraArray[--paraIndex].paraInfo;
				textOffset = ParagraphOffset(docInfo, paraIndex);
				styleRunIndex = CountStyleRuns(paraInfo);
			}
			newOffset = GetStyleRun(&tempStyleRun, paraInfo, --styleRunIndex);
		}

	}

	tempStyleRun.srStyleLen = 0L;
	tempIndex = 0L;
	do {
		textOffset += tempStyleRun.srStyleLen;
		GetStyleRun(&tempStyleRun, paraInfo, tempIndex++);
	} while(tempIndex <= styleRunIndex);
	
	GetStyle(&theStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);

	if(theStyle.stIsGraphic) {
		curOffset = textOffset + (previousChar ? 0L : tempStyleRun.srStyleLen);
		/* Fly by hidden text */
		if(theStyle.stInfo.graphicStyle.graphicInfo == nil) {
			goto StartOver;
		} else {
			return curOffset;
		}
	}
	
	script = StyleToScript(&theStyle.stInfo.textStyle);
	/* Only get the character length table if there are double byte characters */
	doubleByte = (((**(**docInfo).globals).flags.hasDoubleByte) && (((short)GetScriptVariable(script, smScriptFlags) & (1 << smsfSingByte)) == 0));
	if(doubleByte) {
		/* Get the character length table */
		FillParseTable(table, script);
	}
	
	if(!doubleByte && !previousChar) {
		newOffset = curOffset + 1L;
	} else {

		loadParam.lpLenRequest = tempStyleRun.srStyleLen;
		LoadText(docInfo, textOffset, &loadParam, true);
		
		hState = HGetState((Handle)(**docInfo).theText);
		HLock((Handle)(**docInfo).theText);
	
		textPtr = *(**docInfo).theText + (textOffset - (**docInfo).textOffset);
	
		if(previousChar) {
			newOffset = curOffset - ((long)(textPtr + newOffset) - (long)PreviousChar((Ptr)textPtr, (Ptr)(textPtr + newOffset), doubleByte ? table : nil));
		} else {
			newOffset = curOffset + ((table[*(textPtr + newOffset)] != 0) ? 2 : 1);
		}
		
		HSetState((Handle)(**docInfo).theText, hState);
		UnloadText(docInfo);
	}
	
	return newOffset;
}

OSErr FindNextInsertLoc(DocumentInfoHandle docInfo, SelDataPtr selection, Boolean hasPicture)
{
	ParagraphInfoHandle paraInfo;
	long offset;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	OSErr errCode;
	
	/* Get the line starts information for the paragraph */
	paraInfo = (**docInfo).paraArray[selection->paraIndex].paraInfo;

	if(hasPicture && ((**paraInfo).paraFlags & peTextOnly)) {
		return dragNotAcceptedErr;
	}
	
	if((**docInfo).flags.ignoreSelectLock || (**docInfo).flags.ignoreModLock) {
		return noErr;
	}
	
	if(!selection->lastLine && !selection->leadingEdge) {
		errCode = CheckParagraphMeasure(docInfo, selection->paraIndex, true);
	} else {
		errCode = noErr;
	}

	if(errCode != noErr) {
		return errCode;
	}

	offset = selection->offset;

	if(selection->lastLine || (!selection->leadingEdge && (offset != (*(**paraInfo).lineStarts)[selection->lineIndex].lsStartChar))) {
		--offset;
	}

	offset -= ParagraphOffset(docInfo, selection->paraIndex);
	
	if(offset < 0L)
		offset = 0L;
	
	GetStyleRun(&tempStyleRun, paraInfo, StyleRunIndex(paraInfo, &offset));
	GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	if(!(tempStyle.stInfo.textStyle.tsLock & peModLock)) {
		return noErr;
	}
	if(tempStyle.stInfo.textStyle.tsLock & peClickAfterLock) {
		selection->offset += tempStyleRun.srStyleLen - offset;
		if(selection->offset > (**docInfo).textLen) {
			selection->offset = (**docInfo).textLen;
		}
		return 1;
	} else if(tempStyle.stInfo.textStyle.tsLock & peClickBeforeLock) {
		selection->offset -= offset;
		if(selection->offset < 0L) {
			selection->offset = 0L;
		}
		return 1;
	}

	return errAENotModifiable;
}

OSErr FindMyText(DocumentInfoHandle docInfo, Ptr theText, long textLen, long startOffset, long endOffset, long *foundOffset, ScriptCode scriptCode)
{
#pragma unused (scriptCode)
	OSErr errCode;
	LoadParams loadParam;
	long textIndex, curOffset;
	Ptr docText;
	
	if(startOffset < 0L) {
		startOffset = (**docInfo).selStart.offset;
	} else if(startOffset > (**docInfo).textLen) {
		startOffset = (**docInfo).textLen;
	}
	if(endOffset < 0L) {
		endOffset = (**docInfo).selEnd.offset;
	} else if(endOffset > (**docInfo).textLen) {
		endOffset = (**docInfo).textLen;
	}
	*foundOffset = -1L;
	if(startOffset == endOffset) {
		return errOffsetInvalid;
	}
	if(textLen > ABS(endOffset - startOffset)) {
		return (startOffset < endOffset) ? errEndOfBody : errTopOfBody;
	}
	curOffset = startOffset;
	if(startOffset < endOffset) {
		do {
			loadParam.lpLenRequest = textLen;
			errCode = LoadText(docInfo, curOffset, &loadParam, true);
			if(errCode != noErr) {
				return errCode;
			}
			docText = *((Handle)(**docInfo).theText) + (curOffset - (**docInfo).textOffset);
			for(textIndex = 0L; theText[textIndex] == docText[textIndex]; ) {
				++textIndex;
				if(textIndex == textLen) {
					*foundOffset = curOffset;
					return noErr;
				}
				if(curOffset + textIndex == endOffset) {
					return errEndOfBody;
				}
			}
		} while(++curOffset < endOffset);
	} else {
		do {
			loadParam.lpLenRequest = textLen;
			errCode = LoadText(docInfo, curOffset, &loadParam, true);
			if(errCode != noErr) {
				return errCode;
			}
			docText = *((Handle)(**docInfo).theText) + (curOffset - (**docInfo).textOffset);
			for(textIndex = 0L; theText[textLen + textIndex - 1] == docText[textIndex - 1]; ) {
				--textIndex;
				if(curOffset + textIndex <= endOffset) {
					return errTopOfBody;
				}
				if(textIndex == -textLen) {
					*foundOffset = curOffset + textIndex;
					return noErr;
				}
			}
		} while(--curOffset > endOffset);
	}

	return (startOffset < endOffset) ? errEndOfBody : errTopOfBody;
}

OSErr PtToOffset(DocumentInfoHandle docInfo, Point position, long *offset)
{
	SelData selection;
	OSErr errCode;
	
	PtToSelPosition(docInfo, position, &selection);
	errCode = PositionToOffset(docInfo, &selection);
	if(errCode == noErr) {
		if(!selection.leadingEdge) {
			--selection.offset;
		}
		*offset = selection.offset;
	}
	return errCode;
}

OSErr OffsetToPt(DocumentInfoHandle docInfo, long offset, Point *position, LHPtr lineHeight)
{
	OSErr errCode;
	SelData selection;
	long vPosition;
	
	selection.offset = offset;
	selection.leadingEdge = true;
	selection.lastLine = false;
	errCode = OffsetToPosition(docInfo, &selection, false);
	if(errCode == noErr) {
		if(lineHeight != nil) {
			lineHeight->lhHeight = selection.vLineHeight;
			lineHeight->lhAscent = selection.vLineAscent;
		}
		selection.vPosition -= (**docInfo).vPosition;
		vPosition = selection.vPosition;
		if((vPosition < SHRT_MIN) || (vPosition > SHRT_MAX)) {
			errCode = errOffsetIsOutsideOfView;
		} else if(position != nil) {
			position->v = vPosition + (**docInfo).viewRect.top;
			position->h = selection.lPosition - (**docInfo).hPosition + (**docInfo).viewRect.left;
		}
	}
	return errCode;
}

OSErr FindStyle(DocumentInfoHandle docInfo, long startOffset, long *offset, PETEStyleInfoPtr theStyle,PETEStyleEnum validBits)
{
	PETEStyleEntry style;
	long len;
	OSErr errCode;
	
	do {
		if(startOffset >= (**docInfo).textLen) {
			return errEndOfDocument;
		}
		errCode = GetStyleFromOffset(docInfo, startOffset, &len, &style);
		if(errCode != noErr) {
			return errCode;
		}
		if((style.psStartChar == startOffset) && !ChangeStyleInfo(theStyle, &style.psStyle, validBits|peGraphicColorChangeValid, style.psGraphic)) {
			if(offset != nil) {
				*offset = startOffset;
			}
			return noErr;
		}
		startOffset = style.psStartChar + len;
	} while(true);
}