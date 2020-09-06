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

OSErr HandleSelection(DocumentInfoHandle docInfo, SelDataPtr selAnchor, Point curMouseLoc, short modifiers)
{
	SelData selStart, selEnd;
	long firstOffset, secondOffset, lockLowOffset, lockHighOffset;
	unsigned long ticks = 0L;
	Point lastMouseLoc;
	MyTextStyle tempStyle;
	LongStyleRun tempStyleRun;
	Boolean sameSelection, pastAnchor;
	long vPosition;
	short hPosition;
	OSErr errCode;
	unsigned long tickCount;
	
	lockLowOffset = 0L;
	lockHighOffset = LONG_MAX;

	pastAnchor = false;

	if(modifiers & shiftKey) {
		selStart = (**docInfo).selStart;
		selEnd = (**docInfo).selEnd;
		if((**(**docInfo).globals).flags.anchoredSelection) {
			if((**docInfo).flags.anchorEnd) {
				if(selAnchor->offset <= selEnd.offset) {
					selStart = *selAnchor;
					*selAnchor = selEnd;
					(**docInfo).flags.anchorStart = false;
				} else {
					pastAnchor = true;
					selStart = selEnd;
					selEnd = *selAnchor;
					*selAnchor = selStart;
					(**docInfo).flags.anchorEnd = false;
					(**docInfo).flags.anchorStart = true;
				}
			} else {
				if(selAnchor->offset >= selStart.offset) {
					pastAnchor = true;
					selEnd = *selAnchor;
					*selAnchor = selStart;
					(**docInfo).flags.anchorStart = true;
				} else {
					selEnd = selStart;
					selStart = *selAnchor;
					*selAnchor = selEnd;
					(**docInfo).flags.anchorEnd = true;
					(**docInfo).flags.anchorStart = false;
				}
			}
		} else if(selAnchor->offset >= selEnd.offset) {
			pastAnchor = true;
			selEnd = *selAnchor;
			*selAnchor = selStart;
			(**docInfo).flags.anchorStart = true;
			(**docInfo).flags.anchorEnd = false;
		} else if(selAnchor->offset <= selStart.offset) {
			selStart = *selAnchor;
			*selAnchor = selEnd;
			(**docInfo).flags.anchorStart = false;
			(**docInfo).flags.anchorEnd = true;
		} else if(((!((**docInfo).flags.anchorStart || (**docInfo).flags.anchorEnd)) && (selAnchor->offset > selStart.offset) && (selAnchor->offset < selEnd.offset)) || ((**docInfo).flags.anchorStart)) {
			pastAnchor = true;
			selEnd = *selAnchor;
			*selAnchor = selStart;
			(**docInfo).flags.anchorEnd = false;
		} else {
			selStart = *selAnchor;
			*selAnchor = selEnd;
			(**docInfo).flags.anchorEnd = true;
		}
		if(!((**docInfo).flags.ignoreSelectLock)) {
			if(PinSelectionLock(docInfo, selAnchor->offset, pastAnchor ? &selEnd.offset : &selStart.offset)) {
				errCode = OffsetToPosition(docInfo, pastAnchor ? &selEnd : &selStart, false);
				if(errCode != noErr) {
					return errCode;
				}
			}
		}
	} else {

		if((errCode = GetSelectionStyle(docInfo, selAnchor, &tempStyleRun, &firstOffset, &tempStyle)) != noErr) {
			return errCode;
		}
		if(!((**docInfo).flags.ignoreSelectLock)) {
			if(tempStyle.tsLock & peSelectLock) {
				if(tempStyle.tsLock & (peClickBeforeLock | peClickAfterLock)) {
					if(((selAnchor->leadingEdge && (selAnchor->offset < (**docInfo).textLen)) && (tempStyle.tsLock & peClickAfterLock)) ||
					     (!selAnchor->leadingEdge && (tempStyle.tsLock & peClickBeforeLock))) {
						selAnchor->leadingEdge = !selAnchor->leadingEdge;
						if(!selAnchor->leadingEdge) {
							selAnchor->offset += tempStyleRun.srIsGraphic ? tempStyleRun.srStyleLen - firstOffset : 1;
						}
					}
					if((errCode = RecalcSelectionPosition(docInfo, selAnchor, tempStyle.tsLock, false)) != noErr) {
						return errCode;
					}
				} else {
					if((selAnchor->offset > 0L) && (selAnchor->offset < (**docInfo).textLen)) {
						selAnchor->leadingEdge = !selAnchor->leadingEdge;
						if((errCode = GetSelectionStyle(docInfo, selAnchor, nil, nil, &tempStyle)) != noErr) {
							return errCode;
						}
						if(!(tempStyle.tsLock & peSelectLock) || (tempStyle.tsLock & (peClickBeforeLock | peClickAfterLock))) {
							if((selAnchor->leadingEdge && (tempStyle.tsLock & peClickAfterLock)) ||
							     (!selAnchor->leadingEdge && (tempStyle.tsLock & peClickBeforeLock))) {
								selAnchor->offset += selAnchor->leadingEdge ? 1 : -1;
							}
							if((errCode = RecalcSelectionPosition(docInfo, selAnchor, tempStyle.tsLock, true)) != noErr) {
								return errCode;
							}
						}
					}
					if((tempStyle.tsLock & peSelectLock) && !(tempStyle.tsLock & (peClickBeforeLock | peClickAfterLock))) {
						return editingNotAllowed;
					}
				}
				tempStyle.tsLock = peNoLock;
			}
		}

		/* Set the style information and keyboard menu to the right script */
		SetStyleAndKeyboard(docInfo, &tempStyle);
	
		selEnd = selStart = *selAnchor;
		
		(**docInfo).flags.anchorStart = (**docInfo).flags.anchorEnd = true;
	
	}

	if((**docInfo).flags.doubleClick) {
		/* Triple click means select the entire paragraph */
		if((**docInfo).flags.tripleClick) {
		
			errCode = FindTripleOffsets(docInfo, selStart.paraIndex, selStart.offset, &selStart.offset, &selEnd.offset);
			if(errCode != noErr) {
				return errCode;
			}
			if((selEnd.paraIndex < (**docInfo).paraCount - 1) && (ParagraphOffset(docInfo, selEnd.paraIndex + 1) == selEnd.offset)) {
				++selEnd.paraIndex;
				selEnd.lineIndex = 0;
				selEnd.lastLine = false;
			} else {
				selEnd.lastLine = true;
			}
		/* Double click means select the current word */
		} else {
		
			/* Get the word offsets around the current offset */
			errCode = GetWordOffset(docInfo, ((modifiers & shiftKey) ? (((**docInfo).flags.anchorStart) ? &selEnd : &selStart) : selAnchor), nil, &selStart.offset, &selEnd.offset, nil, nil);
			if(errCode != noErr) {
				return errCode;
			}
			selEnd.lastLine = true;
		}

		/* Set the flags appropriately */
		selStart.leadingEdge = true;
		selStart.lastLine = false;
		selEnd.leadingEdge = true;
		
		if(!(((**docInfo).flags.anchorStart) && ((**docInfo).flags.anchorEnd))) {
			if((**docInfo).flags.anchorStart) {
				selStart = *selAnchor;
			} else {
				selEnd = *selAnchor;
			}
		}
		if(!((**docInfo).flags.ignoreSelectLock)) {
			PinSelectionLock(docInfo, selAnchor->offset, &selEnd.offset);
			PinSelectionLock(docInfo, selAnchor->offset, &selStart.offset);
		}
	}

	if(((**docInfo).flags.anchorStart) && ((**docInfo).flags.anchorEnd)) {
		firstOffset = selStart.offset;
		secondOffset = selEnd.offset;
	} else {
		firstOffset = secondOffset = selAnchor->offset;
	}
	
	sameSelection = false;

	do {
		if(!sameSelection) {
			(**docInfo).flags.anchorStart = (selStart.offset == selAnchor->offset);
			(**docInfo).flags.anchorEnd = (selEnd.offset == selAnchor->offset);
		
			errCode = OffsetToPosition(docInfo, &selStart, !((**docInfo).flags.doubleClick));
			if(errCode == noErr) {
				errCode = OffsetToPosition(docInfo, &selEnd, !((**docInfo).flags.doubleClick));
			}
			if(errCode != noErr) {
				return errCode;
			}
			
			vPosition = (**docInfo).vPosition;
			hPosition = (**docInfo).hPosition;
			
			MakeSelectionVisible(docInfo, pastAnchor ? &selEnd : &selStart);
			
			vPosition -= (**docInfo).vPosition;
			hPosition -= (**docInfo).hPosition;
			
			(**docInfo).selStart = selStart;
			(**docInfo).selEnd = selEnd;
			if((errCode = CalcSelectionRgn(docInfo, true)) != noErr) {
				return errCode;
			}
			
			/* If MakeSelectionVisible moved the selection outside of the window */
			if((hPosition != 0L) || (vPosition != 0L)) {
				long tickDiff;
				
				tickCount = TickCount();
				tickDiff = (**(**docInfo).globals).autoScrollTicks - (tickCount - ticks);
				if(tickDiff > 0L) {
					Delay(tickDiff, &tickCount);
				}
				ticks = tickCount;

				curMouseLoc.v += vPosition;
				curMouseLoc.h += hPosition;
			}
		}

		if(!StillDown()) {
			break;
		}
		else
			Delay(1,&tickCount);
		
		lastMouseLoc = curMouseLoc;
		GetMouse(&curMouseLoc);
		if(!(sameSelection = EqualPt(curMouseLoc, lastMouseLoc))) {
			PtToSelPosition(docInfo, curMouseLoc, &selEnd);
			errCode = PositionToOffset(docInfo, &selEnd);
			if(errCode != noErr) {
				return errCode;
			}
			
			if(!((**docInfo).flags.ignoreSelectLock)) {
				if(PinSelectionLock(docInfo, selAnchor->offset, &selEnd.offset)) {
					if((errCode = OffsetToPosition(docInfo, &selEnd, false)) != noErr) {
						return errCode;
					}
				}
			}

			pastAnchor = (selEnd.offset > selAnchor->offset);
			
			if((**docInfo).flags.doubleClick) {
				
				if((**docInfo).flags.tripleClick) {
					errCode = FindTripleOffsets(docInfo, selEnd.paraIndex, selEnd.offset, &selStart.offset, &selEnd.offset);
					if(errCode != noErr) {
						return errCode;
					}
					if((selEnd.paraIndex < (**docInfo).paraCount - 1) && (ParagraphOffset(docInfo, selEnd.paraIndex + 1) == selEnd.offset)) {
						++selEnd.paraIndex;
						selEnd.lineIndex = 0;
						selEnd.lastLine = false;
					} else {
						selEnd.lastLine = true;
					}
				} else {
					errCode = GetWordOffset(docInfo, &selEnd, selAnchor, &selStart.offset, &selEnd.offset, nil, nil);
					if(errCode != noErr) {
						return errCode;
					}
					selEnd.lastLine = true;
				}

				/* Set the flags appropriately */
				selStart.leadingEdge = true;
				selStart.lastLine = false;
				selEnd.leadingEdge = true;
				
				if((selStart.offset < firstOffset) && !pastAnchor) {
					selEnd.offset = secondOffset;
				}
				if((selEnd.offset > secondOffset) && pastAnchor) {
					selStart.offset = firstOffset;
				}

				if(!((**docInfo).flags.ignoreSelectLock)) {
					PinSelectionLock(docInfo, selAnchor->offset, pastAnchor ? &selEnd.offset : &selStart.offset);
				}
	
			} else if(selEnd.offset < selAnchor->offset) {
				selStart = selEnd;
				selEnd = *selAnchor;
			} else if (selStart.offset != selAnchor->offset) {
				selStart = *selAnchor;
			}

		}
	} while(true);

	return noErr;
}

OSErr CalcSelectionRgn(DocumentInfoHandle docInfo, Boolean preserveLine)
{
	LSTable lineStarts;
	Rect marginRect, selectRect;
	LineInfo lineInfo;
	long paraIndex, lowOffset, highOffset, curStart, curEnd;
	short lineIndex, lineCount;
	short curV, vBottom;
	RgnHandle hiliteRgn, oldClip, scratchRgn;
	LayoutCache lineCache;
	Boolean hiliteLeft, hiliteRight, stayDirty;
	SelData selStart, selEnd;
	PETEPortInfo savedPortInfo;
	RGBColor backColor;
	OSErr errCode;
	Byte hState;
	
	if((**docInfo).flags.recalcOff) {
		UnionRgn((**docInfo).hiliteRgn, (**docInfo).updateRgn, (**docInfo).updateRgn);
		SetEmptyRgn((**docInfo).hiliteRgn);
		(**docInfo).flags.selectionDirty = true;
		return cantDoThatInCurrentMode;
	}
	
	/* Create a region for the selection */
	hiliteRgn = NewRgn();
	if(hiliteRgn == nil) {
		return memFullErr;
	}
	
	errCode = noErr;
	lineStarts = nil;
	scratchRgn = (**(**docInfo).globals).scratchRgn;
		
	if((**docInfo).flags.selectionDirty) {
		preserveLine = false;
	}
	
	/* Initialize the drawing rectangle */
	selectRect = (**docInfo).viewRect;

	/* Get the offsets for the selection */
	lowOffset = (**docInfo).selStart.offset;
	highOffset = (**docInfo).selEnd.offset;
	stayDirty = false;
	
	/* Check to make sure that there is a selection and that it's on the screen */
	if((lowOffset != highOffset) &&
	   ((**docInfo).selectedGraphic == nil) &&
	   !(stayDirty = !(FindFirstVisibleLine(docInfo, &lineInfo, selectRect.top) == noErr))) {
	
		/* Get the location of the first line of the selection */
		paraIndex = lineInfo.paraIndex;
		lineIndex = lineInfo.lineIndex;
		curV = lineInfo.vPosition;
		
		/* Save the bottom of the drawing rectangle and then set up the selection rectangle */
		vBottom = selectRect.bottom;
		selectRect.top = selectRect.bottom = 0;
		
		/* Loop through the the paragraphs until the bottom of the drawing rect */
		for(; paraIndex < (**docInfo).paraCount && curV < vBottom; ++paraIndex) {

			if(lineStarts != nil) {
				HSetState((Handle)lineStarts, hState);
			}

			/* If the height hasn't been calculated, get it */
			errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
			if(errCode != noErr) {
				goto DoReturn;
			}

			/* Get the line starts for this paragraph */
			lineStarts = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts;
			hState = HGetState((Handle)lineStarts);
			HNoPurge((Handle)lineStarts);
			
			lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;

			/* If the low offset isn't in this paragraph, short circuit the loop */
			if((*lineStarts)[lineCount].lsStartChar < lowOffset) {
				for(; lineIndex < lineCount; ++lineIndex) {
					curV += (*lineStarts)[lineIndex].lsMeasure.height;
				}
				lineIndex = 0L;
				continue;
			}
			
			/* If the high Offset has already been passed, stop the loop */
			if((*lineStarts)[lineIndex].lsStartChar > highOffset) {
				break;
			}

			/* Loop through the lines in the paragraph */
			for(; lineIndex < lineCount && curV < vBottom && (*lineStarts)[lineIndex].lsStartChar < highOffset; ++lineIndex) {

				/* Get the character range of this line */
				curStart = (*lineStarts)[lineIndex].lsStartChar;
				curEnd = (*lineStarts)[lineIndex + 1].lsStartChar;

				/* Check to see whether the selection starts after the start of this line */
				if(!(hiliteLeft = !(lowOffset >= curStart))) {
					curStart = lowOffset;
				}

				if(!(hiliteRight = !(highOffset < (*lineStarts)[lineIndex + 1].lsStartChar))) {
					curEnd = highOffset;
				}
				
				if((curStart == (*lineStarts)[lineIndex].lsStartChar) &&
				   (curEnd == (*lineStarts)[lineIndex + 1].lsStartChar)) {

					if(selectRect.top == selectRect.bottom) {
						selectRect.top = curV;
					}
					selectRect.bottom = curV + (*lineStarts)[lineIndex].lsMeasure.height;
					
				} else if(curStart < curEnd) {
				
					if(selectRect.top != selectRect.bottom) {
						RectAndRgn(hiliteRgn, scratchRgn, &selectRect);
						selectRect.top = selectRect.bottom = 0;
					}

					/* Layout the line to hilight */
					lineCache.lineIndex = lineIndex;
					lineCache.paraIndex = paraIndex;
					errCode = GetLineLayout(docInfo, &lineCache);
					if(errCode != noErr) {
						goto DoReturn;
					}
					
					if(hiliteLeft || hiliteRight) {
						if((**(**docInfo).paraArray[paraIndex].paraInfo).direction != leftCaret) {
							Exchange(hiliteLeft, hiliteRight);
						}
						
						marginRect.top = curV;
						marginRect.bottom = curV + (*lineStarts)[lineIndex].lsMeasure.height;
					}
					
					if(hiliteLeft) {
						marginRect.left = selectRect.left;
						marginRect.right = selectRect.left + lineCache.leftPosition;
						RectAndRgn(hiliteRgn, scratchRgn, &marginRect);
					}
					
					if(hiliteRight) {
						marginRect.left = selectRect.left + lineCache.leftPosition + lineCache.visibleWidth;
						marginRect.right = selectRect.right;
						RectAndRgn(hiliteRgn, scratchRgn, &marginRect);
					}
					
					errCode = AddLineToRegion(hiliteRgn, docInfo, &lineCache, curStart, curEnd, curV);
					if(errCode != noErr) {
						goto DoReturn;
					}
				}
				
				curV += (*lineStarts)[lineIndex].lsMeasure.height;
				
			}

			lineIndex = 0L;
		}
		if(selectRect.top != selectRect.bottom) {
			RectAndRgn(hiliteRgn, scratchRgn, &selectRect);
			selectRect.top = selectRect.bottom = 0;
		}
	}
	selStart = (**docInfo).selStart;
	errCode = OffsetToPosition(docInfo, &selStart, preserveLine);
	if((errCode == noErr) && (lowOffset != highOffset)) {
		selEnd = (**docInfo).selEnd;
		errCode = OffsetToPosition(docInfo, &selEnd, preserveLine);
	} else {
		selEnd = selStart;
	}

	if(errCode == noErr) {
		(**docInfo).selStart = selStart;
		(**docInfo).selEnd = selEnd;
		
		selectRect = (**docInfo).viewRect;
		RectRgn(scratchRgn, &selectRect);
		SectRgn(scratchRgn, hiliteRgn, hiliteRgn);
		
		if(!((**docInfo).flags.isActive)) {
			CopyRgn((**docInfo).hiliteRgn, scratchRgn);
			InsetRgn(scratchRgn, 1, 1);
			DiffRgn((**docInfo).hiliteRgn, scratchRgn, (**docInfo).hiliteRgn);
		}
	
		CopyRgn(hiliteRgn, scratchRgn);
		if(!((**docInfo).flags.isActive)) {
			InsetRgn(scratchRgn, 1, 1);
			DiffRgn(hiliteRgn, scratchRgn, scratchRgn);
		}
		
		XorRgn(scratchRgn, (**docInfo).hiliteRgn, (**docInfo).hiliteRgn);
		
		oldClip = NewRgn();
		if(oldClip != nil) {
			SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		
			backColor = DocOrGlobalBGColor(nil, docInfo);
			if(!IsPETEDefaultColor(backColor)) {
				RGBBackColorAndPat(&backColor);
			}

			GetClip(oldClip);
			RectRgn(scratchRgn, &selectRect);
			SectRgn(oldClip, scratchRgn, scratchRgn);
			SetClip(scratchRgn);
			
//			LMSetHiliteMode(LMGetHiliteMode() & ~(1 << hiliteBit));
//			InvertRgn((**docInfo).hiliteRgn);
			PenMode(hilite);
			PaintRgn((**docInfo).hiliteRgn);
		
			SetClip(oldClip);
			DisposeRgn(oldClip);
		
			ResetPortInfo(&savedPortInfo);
	
			ExchangePtr((**docInfo).hiliteRgn, hiliteRgn);
			
			if(!stayDirty) {
				(**docInfo).flags.selectionDirty = false;
			}
			
		} else {
			errCode = memFullErr;
		}
	}

DoReturn :
	DisposeRgn(hiliteRgn);
	if(lineStarts != nil) {
		HSetState((Handle)lineStarts, hState);
	}
	return errCode;
}

OSErr AddLineToRegion(RgnHandle hiliteRgn, DocumentInfoHandle docInfo, LayoutCachePtr lineCache, long startOffset, long endOffset, short vOffset)
{
	OrderHandle orderingHndl;
	ParagraphInfoHandle paraInfo;
	StringHandle theText;
	Ptr textPtr;
	short hPosition, lineHeight;
	short numRuns, runIndex, offIndex, curDirection;
	Byte hState;
	JustStyleCode styleRunPosition;
	Rect hiliteRect;
	Point scaling;
	OffsetTable offsets;
	LongSTElement tempStyle;
	RgnHandle scratchRgn;
	OSErr errCode;
	PETEPortInfo savedPortInfo;
	LoadParams loadParam;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	
	/* If the current paragraph hasn't been measured, measure it */
	errCode = CheckParagraphMeasure(docInfo, lineCache->paraIndex, true);
	if(errCode != noErr) {
		return errCode;
	}

	paraInfo = (**docInfo).paraArray[lineCache->paraIndex].paraInfo;
	lineHeight = (*(**paraInfo).lineStarts)[lineCache->lineIndex].lsMeasure.height;

	SetRect(&hiliteRect, 0, vOffset, 0, vOffset + lineHeight);

	hPosition = lineCache->leftPosition - (**docInfo).hPosition;
	orderingHndl = lineCache->orderingHndl;
	numRuns = InlineGetHandleSize((Handle)orderingHndl) / sizeof(OrderEntry);
	
	/* Not really necessary for un-justified text */
	styleRunPosition = (numRuns == 1) ? onlyStyleRun : leftStyleRun;
	
	/* Initialize scaling factors to 1:1 */
	scaling.h = scaling.v = 1;

	for(runIndex = 0; runIndex < numRuns; ++runIndex) {
		
		if((offsets[0].offFirst = (startOffset - (*orderingHndl)[runIndex].offset)) < 0) {
			offsets[0].offFirst = 0;
		}
		
		if((offsets[0].offSecond = (endOffset - (*orderingHndl)[runIndex].offset)) > (*orderingHndl)[runIndex].len) {
			offsets[0].offSecond = (*orderingHndl)[runIndex].len;
		}

		if((offsets[0].offFirst == 0) && (offsets[0].offSecond == (*orderingHndl)[runIndex].len)) {

			if(hiliteRect.left == hiliteRect.right) {
				hiliteRect.left = hPosition + (**docInfo).viewRect.left;
			}
			hiliteRect.right = hPosition + (*orderingHndl)[runIndex].width + (**docInfo).viewRect.left;

		} else {
		
			if(hiliteRect.left != hiliteRect.right) {
				RectAndRgn(hiliteRgn, scratchRgn, &hiliteRect);
				hiliteRect.left = hiliteRect.right = 0;
			}

			if(offsets[0].offFirst < offsets[0].offSecond) {
			
				loadParam.lpLenRequest = (*orderingHndl)[runIndex].len;
				errCode = LoadText(docInfo, (*orderingHndl)[runIndex].offset, &loadParam, true);
				
				if(errCode != noErr) {
					return errCode;
				}
			
				SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	
				/* Set the text style */
				GetStyle(&tempStyle, (**docInfo).theStyleTable, (*orderingHndl)[runIndex].styleIndex);
				SetTextStyleWithDefaults(nil, docInfo, &tempStyle.stInfo.textStyle, false, ((**docInfo).printData != nil));
	
				theText = (**docInfo).theText;
				hState = HGetState((Handle)theText);
				HLock((Handle)theText);
				
				/* Find the appropriate place in the text */
				textPtr = (Ptr)*theText + ((*orderingHndl)[runIndex].offset - (**docInfo).textOffset);
				
				curDirection = GetSysDirection();
				/* Set the direction for the paragraph */
				SetSysDirection((**paraInfo).direction);
				
				HiliteText(textPtr, (*orderingHndl)[runIndex].len, offsets[0].offFirst, offsets[0].offSecond, offsets);
				
				/* Restore line direction */
				SetSysDirection(curDirection);
	
				for(offIndex = 0; offIndex < sizeof(offsets) / sizeof(OffPair); ++offIndex) {
					if(offsets[offIndex].offFirst != offsets[offIndex].offSecond) {
						hiliteRect.left = hiliteRect.right = hPosition + (**docInfo).viewRect.left;
						hiliteRect.left += MyCharToPixel(textPtr, (*orderingHndl)[runIndex].len, 0L, offsets[offIndex].offFirst, kHilite, styleRunPosition, scaling, scaling, (**docInfo).globals);
						hiliteRect.right += MyCharToPixel(textPtr, (*orderingHndl)[runIndex].len, 0L, offsets[offIndex].offSecond, kHilite, styleRunPosition, scaling, scaling, (**docInfo).globals);

						if(hiliteRect.left > hiliteRect.right) {
							Exchange(hiliteRect.left, hiliteRect.right);
						}
	
						HSetState((Handle)theText, hState);
						
						RectAndRgn(hiliteRgn, scratchRgn, &hiliteRect);
						hiliteRect.left = hiliteRect.right = 0;
					}
				}
				HSetState((Handle)theText, hState);
				UnloadText(docInfo);
	
				ResetPortInfo(&savedPortInfo);
				
			}
		}
		hPosition += (*orderingHndl)[runIndex].width;

		/* Not really necessary for un-justified text */
		styleRunPosition = (runIndex == numRuns - 2) ? rightStyleRun : middleStyleRun;
	}

	if(hiliteRect.left != hiliteRect.right) {
		RectAndRgn(hiliteRgn, scratchRgn, &hiliteRect);
	}
	return noErr;
}

void InvertSelection(DocumentInfoHandle docInfo)
{
	RgnHandle scratchRgn;
	
	if(!EmptyRgn((**docInfo).hiliteRgn)) {
	
		if(!((**docInfo).flags.isActive)) {
			scratchRgn = (**(**docInfo).globals).scratchRgn;
			CopyRgn((**docInfo).hiliteRgn, scratchRgn);
			InsetRgn(scratchRgn, 1, 1);
			DiffRgn((**docInfo).hiliteRgn, scratchRgn, scratchRgn);
		}
		
//		LMSetHiliteMode(LMGetHiliteMode() & ~(1 << hiliteBit));
//		InvertRgn(((**docInfo).flags.isActive) ? (**docInfo).hiliteRgn : scratchRgn);
		PenMode(hilite);
		PaintRgn(((**docInfo).flags.isActive) ? (**docInfo).hiliteRgn : scratchRgn);

	}
}

Boolean PinSelectionLock(DocumentInfoHandle docInfo, long curOffset, long *newOffset)
{
	long curParaIndex, curStyleRunIndex, newParaIndex, newStyleRunIndex, tempNewOffset, numStyleRuns;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	ParagraphInfoHandle paraInfo;
	
	if(*newOffset < 0L) {
		*newOffset = 0L;
	} else if(*newOffset > (**docInfo).textLen) {
		*newOffset = (**docInfo).textLen;
	}
	
	if(curOffset == *newOffset) {
		return false;
	}
	
	/* Find the old paragraph */
	curParaIndex = ParagraphIndex(docInfo, curOffset);
	/* Find the new paragraph */
	newParaIndex = ParagraphIndex(docInfo, *newOffset);
	
	/* Find the style run index of the new location */
	tempNewOffset = *newOffset - ParagraphOffset(docInfo, newParaIndex);
	paraInfo = (**docInfo).paraArray[newParaIndex].paraInfo;
	if(tempNewOffset == (**docInfo).paraArray[newParaIndex].paraLength) {
		newStyleRunIndex = CountStyleRuns(paraInfo) - 1L;
		tempNewOffset = 0L;
	} else {
		newStyleRunIndex = StyleRunIndex(paraInfo, &tempNewOffset);
	}

	/* Find the style run index of the old location */
	tempNewOffset = curOffset - ParagraphOffset(docInfo, curParaIndex);
	paraInfo = (**docInfo).paraArray[curParaIndex].paraInfo;
	numStyleRuns = CountStyleRuns(paraInfo);
	if(tempNewOffset == (**docInfo).paraArray[curParaIndex].paraLength) {
		curStyleRunIndex = numStyleRuns - 1L;
		if(curStyleRunIndex < 0L) {
			curStyleRunIndex = 0L;
		} else {
			tempNewOffset = -1L;
		}
	} else {
		curStyleRunIndex = StyleRunIndex(paraInfo, &tempNewOffset);
	}

	if((tempNewOffset == 0L) && (curOffset > *newOffset)) {
		goto FirstZero;
	} else if(tempNewOffset == -1L) {
		tempNewOffset = 0L;
	}

	do {
		
		/* Get the style information for the current location */
		GetStyleRun(&tempStyleRun, paraInfo, curStyleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		
		/* If the current style is locked, the new offset is pinned to the current place */
		if(tempStyle.stInfo.textStyle.tsLock & peSelectLock) {
			*newOffset = curOffset;
			return true;
		/* Otherwise, move to the next style run to see if it is pinned */
		} else {
			/*
			 *	If the current location is past the new location,
			 *	move to the end of the previous style run.
			*/
			if(curOffset > *newOffset) {
				if(tempNewOffset == 0L) {
					curOffset -= tempStyleRun.srStyleLen;
				} else {
					curOffset -= tempNewOffset;
					tempNewOffset = 0L;
				}
		FirstZero :
				if((curStyleRunIndex == 0L) && (curParaIndex > newParaIndex)) {
					--curParaIndex;
					paraInfo = (**docInfo).paraArray[curParaIndex].paraInfo;
					curStyleRunIndex = CountStyleRuns(paraInfo);
				}

			/* Otherwise, move to the end of this style run */
			} else {
				curOffset += tempStyleRun.srStyleLen;
				curOffset -= tempNewOffset;
				tempNewOffset = 0L;
				
				if((curStyleRunIndex == numStyleRuns - 1L) && (curParaIndex < newParaIndex)) {
					++curParaIndex;
					paraInfo = (**docInfo).paraArray[curParaIndex].paraInfo;
					numStyleRuns = CountStyleRuns(paraInfo);
					curStyleRunIndex = -1L;
				}
			}
		}
	} while(((curParaIndex != newParaIndex) ||
	         (curStyleRunIndex != newStyleRunIndex)) &&
	        ((curStyleRunIndex += (curOffset > *newOffset) ? -1L : 1L), true));
	
	return false;
}

OSErr SetSelection(DocumentInfoHandle docInfo, long startOffset, long endOffset)
{
	OSErr errCode;
	PETEStyleEntry theStyle;
	
	/* If the caret was on, turn it off first */
	EraseCaret(docInfo);
	
	if(startOffset < 0L) {
		startOffset = 0L;
	} else if(startOffset > (**docInfo).textLen) {
		startOffset = (**docInfo).textLen;
	}
	
	if(endOffset < 0L) {
		endOffset = 0L;
	} else if(endOffset > (**docInfo).textLen) {
		endOffset = (**docInfo).textLen;
	}
	
	if(endOffset < startOffset) {
		endOffset = startOffset;
	}
	
	
	PunctuateUndo(docInfo);
	
	if((**docInfo).selectedGraphic != nil) {
		MyCallGraphicHit(docInfo, (**docInfo).selStart.offset, nil);
		(**docInfo).selectedGraphic = nil;
	}

	(**docInfo).selStart.offset = startOffset;
	(**docInfo).selEnd.offset = endOffset;
	(**docInfo).selStart.lastLine = (**docInfo).selEnd.lastLine = false;
	AddParagraphBreaks(docInfo);
	errCode = GetStyleFromOffset(docInfo, kPETECurrentSelection, nil, &theStyle);
	if(errCode == noErr) {
		SetStyleAndKeyboard(docInfo, &theStyle.psStyle.textStyle);
		errCode = CalcSelectionRgn(docInfo, false);
	}
	return errCode;
}

void GetSelection(DocumentInfoHandle docInfo, long *startOffset, long *endOffset)
{
	*startOffset = (**docInfo).selStart.offset;
	*endOffset = (**docInfo).selEnd.offset;
}

void PtToSelPosition(DocumentInfoHandle docInfo, Point thePt, SelDataPtr selection)
{
	/* Set the vertical position to the top of window plus the point position */
	selection->vPosition = (**docInfo).vPosition;
	selection->vPosition += thePt.v - (**docInfo).viewRect.top;
	
	/* Set the horizontal position to the left of window plus the point position */
	selection->hPosition = thePt.h - (**docInfo).viewRect.left + (**docInfo).hPosition;
	
	selection->vLineHeight = (**docInfo).shortLineHeight;
}

OSErr FindTripleOffsets(DocumentInfoHandle docInfo, long paraIndex, long offset, long *startOffset, long *endOffset)
{
	LoadParams loadParam;
	long tempOffset, searchOffset, newStartOffset, paraOffset;
	OSErr errCode;
	Boolean foundCR;
	unsigned char tempChar;
	
	paraOffset = newStartOffset = searchOffset = ParagraphOffset(docInfo, paraIndex);
	
	if(paraOffset != offset) {
		do {
			loadParam.lpLenRequest = offset - searchOffset;
			errCode = LoadText(docInfo, offset, &loadParam, false);
			if(errCode != noErr) {
				return errCode;
			}
			
			for(tempOffset = searchOffset + loadParam.lpLenRequest; --tempOffset >= searchOffset; ) {
				if(((tempChar = (*(**docInfo).theText)[tempOffset - (**docInfo).textOffset]) == kCarriageReturnChar) ||
				   (tempChar == kPageDownChar) ||
				   (tempChar == kLineFeedChar)) {
					++tempOffset;
					break;
				}
			}
			
			if(tempOffset >= searchOffset) {
				newStartOffset = tempOffset;
			}
			
			searchOffset += loadParam.lpLenRequest;
			
			UnloadText(docInfo);
		} while(searchOffset < offset);
		
	}
	
	paraOffset += (**docInfo).paraArray[paraIndex].paraLength;
	for(foundCR = false; !foundCR && offset < paraOffset; ) {
		loadParam.lpLenRequest = paraOffset - offset;
		errCode = LoadText(docInfo, offset, &loadParam, false);
		if(errCode != noErr) {
			return errCode;
		}

		for(tempOffset = offset + loadParam.lpLenRequest; !foundCR && offset < tempOffset; ++offset) {
			foundCR = (((tempChar = (*(**docInfo).theText)[offset - (**docInfo).textOffset]) == kCarriageReturnChar) || (tempChar == kPageDownChar) || (tempChar == kLineFeedChar));
		}
		
		UnloadText(docInfo);
		
	}
	
	*startOffset = newStartOffset;
	*endOffset = offset;
	return noErr;
}

OSErr RecalcSelectionPosition(DocumentInfoHandle docInfo, SelDataPtr selAnchor, Byte lock, Boolean always)
{
	Boolean preserveLine = true;
	OSErr errCode = noErr;
	
	if(!(**docInfo).flags.ignoreSelectLock && (lock & peClickAfterLock) && (selAnchor->offset > 0L)) {
		char theChar;
		
		errCode = GetText(docInfo, selAnchor->offset - 1L, selAnchor->offset, &theChar, 1L, nil);
		if((errCode == noErr) && (theChar == kCarriageReturnChar)) {
			preserveLine = false;
			always = true;
		}
	}
	if(always && (errCode == noErr)) {
		errCode = OffsetToPosition(docInfo, selAnchor, preserveLine);
	}
	return errCode;
}