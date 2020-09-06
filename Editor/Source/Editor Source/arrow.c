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

void SetAnchorSelection(DocumentInfoHandle docInfo, SelDataPtr selection);

void HandleVerticalArrow(DocumentInfoHandle docInfo, short modifiers, Boolean upArrow)
{
	long tempOffset, startOffset;
	SelData selection;
	Boolean preserveLine, hadSelection;
	PETEStyleEntry theStyle; 
	
	/* Was there a selection at the start? */
	hadSelection = ((**docInfo).selStart.offset != (**docInfo).selEnd.offset);
	
	/* Recalc selection region so the positions are correct */
	if((**docInfo).flags.selectionDirty) {
		CalcSelectionRgn(docInfo, false);
	}
	
	/* If up, move from the start selection, down then from the end selection */
	selection = (((**(**docInfo).globals).flags.anchoredSelection && (modifiers & shiftKey)) ? (**docInfo).flags.anchorEnd : upArrow) ? (**docInfo).selStart : (**docInfo).selEnd;

	selection.lastLine = false;
	
	preserveLine = false;

	if(modifiers & cmdKey) {
		/* Command arrow beginning or end of text */
		if(upArrow) {
			selection.offset = 0L;
		} else {
			selection.offset = (**docInfo).textLen;
		}
	} else if(modifiers & optionKey) {
		/* Option arrow go up or down a paragraph (or CR-CR boundary) */
		
		/* First move one character in the desired direction */
		if(upArrow) {
			if(selection.offset != 0L) {
				--selection.offset;
			}
		} else {
			if(selection.offset < (**docInfo).textLen) {
				++selection.offset;
			}
		}
		
		/* Find the offsets to select from this point if it were a triple click (paragraph) */
		FindTripleOffsets(docInfo, ParagraphIndex(docInfo, selection.offset), selection.offset, &startOffset, &selection.offset);
		
		selection.leadingEdge = true;
		selection.lastLine = false;

		if(upArrow) {
			selection.offset = startOffset;
		} else {
			if((selection.offset > 0L) && ((*(**docInfo).theText)[selection.offset - 1L] == kCarriageReturnChar) && !(modifiers & shiftKey)) {
				--selection.offset;
			} else {
				selection.paraIndex = ParagraphIndex(docInfo, selection.offset);
				selection.lineIndex = 0L;
			}
		}
		
		/* Old code to do "real" paragraphs */
/*		if(!upArrow && (selection.paraIndex < (**docInfo).paraCount)) {
			++selection.paraIndex;
		}
		tempOffset = ParagraphOffset(docInfo, selection.paraIndex);
		if(upArrow) {
			if((selection.offset == tempOffset) && (selection.paraIndex != 0L)) {
				--selection.paraIndex;
				selection.offset = ParagraphOffset(docInfo, selection.paraIndex);
			} else {
				selection.offset = tempOffset;
			}
			selection.lineIndex = 0L;
		} else {
			--tempOffset;
			if((selection.offset == tempOffset) && (selection.paraIndex < (**docInfo).paraCount)) {
				selection.offset = ParagraphOffset(docInfo, selection.paraIndex + 1L) - 1L;
			} else {
				selection.offset = tempOffset;
			}
			--selection.paraIndex;
			selection.lineIndex = -1L;
		}*/
	} else {
		/* Plain arrow, so move by a line */
		
		/* If there was a selection and no shift key, the selection is set to the endpoint */
		if(!hadSelection || (modifiers & shiftKey)) {
			/* Move positionally 1 line vertically */
			if(upArrow) {
				selection.vPosition -= 1;
			} else {
				selection.vPosition += selection.vLineHeight + 1;
			}
			/* Recalc offset from new position */
			PositionToOffset(docInfo, &selection);
		}
		preserveLine = true;
	}
	
	/* Check for selection lock and pin selection to there */
	if(!((**docInfo).flags.ignoreSelectLock)) {
		if(PinSelectionLock(docInfo, upArrow ? (**docInfo).selStart.offset : (**docInfo).selEnd.offset, &selection.offset)) {
			preserveLine = false;
		}
	}
	
	/* Recalc selection position from offset */
	OffsetToPosition(docInfo, &selection, preserveLine);
	
	/* Show selection in the window */
	MakeSelectionVisible(docInfo, &selection);
	
	/* Save the old offsets */
	startOffset = (**docInfo).selStart.offset;
	tempOffset = (**docInfo).selEnd.offset;
	
	if((**(**docInfo).globals).flags.anchoredSelection && (modifiers & shiftKey)) {
		SetAnchorSelection(docInfo, &selection);
	} else {
		/* If the shift key wasn't down, the selection is now an insertion point */
		/* The starting selection changed if there was an up arrow */
		if(upArrow || !(modifiers & shiftKey)) {
			(**docInfo).selStart = selection;
		}
		/* The ending selection changed if there was a down arrow */
		if(!upArrow || !(modifiers & shiftKey)) {
			(**docInfo).selEnd = selection;
		}
	}
	if(modifiers & shiftKey) {
		PunctuateUndo(docInfo);
	} else if((startOffset != (**docInfo).selStart.offset) || (tempOffset != (**docInfo).selEnd.offset)) {
		tempOffset = (**docInfo).selStart.offset;
		if(!upArrow && (tempOffset != 0L)) {
			--tempOffset;
		}
		if(GetStyleFromOffset(docInfo, tempOffset, nil, &theStyle) == noErr) {
			theStyle.psStyle.textStyle.tsLock = peNoLock;
			if(theStyle.psGraphic) {
				SetPETEDefaultColor(theStyle.psStyle.textStyle.tsColor);
			}
			SetStyleAndKeyboard(docInfo, &theStyle.psStyle.textStyle);
		}
	}
	
	if(hadSelection || ((**docInfo).selStart.offset != (**docInfo).selEnd.offset)) {
		if((**docInfo).selectedGraphic != nil) {
			MyCallGraphicHit(docInfo, startOffset, nil);
			(**docInfo).selectedGraphic = nil;
		}
		CalcSelectionRgn(docInfo, true);
	}
}

void HandleHorizontalArrow(DocumentInfoHandle docInfo, short modifiers, Boolean leftArrow)
{
	SelData selection, selStart, selEnd;
	Boolean hadSelection, startLeft;
	short startDirection, endDirection;
	LongSTElement tempStyle;
	
	hadSelection = ((**docInfo).selStart.offset != (**docInfo).selEnd.offset);
	leftArrow = !(!(leftArrow));
	
	if((**docInfo).flags.selectionDirty) {
		CalcSelectionRgn(docInfo, false);
	}
	
	selStart = (**docInfo).selStart;
	selEnd = (**docInfo).selEnd;
	
	startDirection = (**((**docInfo).paraArray[selStart.paraIndex].paraInfo)).direction;
	endDirection = (**((**docInfo).paraArray[selEnd.paraIndex].paraInfo)).direction;
	
	if(hadSelection) {
		if((**(**docInfo).globals).flags.anchoredSelection && (modifiers & shiftKey)) {
			startLeft = (**docInfo).flags.anchorEnd ? leftArrow : !leftArrow;
		/* If the selection is all on one line, extend from the side identical to the arrow */
		} else if(selStart.vPosition == selEnd.vPosition) {
			if(startDirection == leftCaret) {
				startLeft = (selStart.lPosition < selEnd.lPosition);
			} else {
				startLeft = (selStart.rPosition < selEnd.rPosition);
			}
		/* Figure out which way to go based on the paragraph direction */
		} else {
			startLeft = ((startDirection == leftCaret) || (endDirection == leftCaret));
		}
	} else {
		/* No selection, so it doesn't matter which one we start from */
		startLeft = leftArrow;
	}
	selection = (leftArrow == startLeft) ? selStart : selEnd;
	if(leftArrow == startLeft) {
		selection.hPosition = (startDirection == leftCaret) ? selStart.lPosition : selStart.rPosition;
	} else {
		selection.hPosition = (endDirection == leftCaret) ? selEnd.lPosition : selEnd.rPosition;
	}
	
	GetHorizontalSelection(docInfo, &selection, modifiers, leftArrow, (hadSelection && !(modifiers & (shiftKey | optionKey))));
	
	MakeSelectionVisible(docInfo, &selection);
	
	if((**(**docInfo).globals).flags.anchoredSelection && (modifiers & shiftKey)) {
		SetAnchorSelection(docInfo, &selection);
	} else if(hadSelection && (modifiers & shiftKey)) {
		if(leftArrow == startLeft) {
			(**docInfo).selStart = selection;
		} else {
			(**docInfo).selEnd = selection;
		}
	} else {
		if((selection.offset < selStart.offset) || !(modifiers & shiftKey)) {
			(**docInfo).selStart = selection;
		}
		if((selection.offset > selEnd.offset) || !(modifiers & shiftKey)) {
			(**docInfo).selEnd = selection;
		}
	}

	if(modifiers & shiftKey) {
		PunctuateUndo(docInfo);
	} else if((selStart.offset != (**docInfo).selStart.offset) || (selEnd.offset != (**docInfo).selEnd.offset)) {
		GetSelectionStyle(docInfo, &selection, nil, nil, &tempStyle.stInfo.textStyle);
		tempStyle.stInfo.textStyle.tsLock = peNoLock;
		SetStyleAndKeyboard(docInfo, &tempStyle.stInfo.textStyle);
	}

	if(hadSelection || ((**docInfo).selStart.offset != (**docInfo).selEnd.offset)) {
		if((**docInfo).selectedGraphic != nil) {
			MyCallGraphicHit(docInfo, selStart.offset, nil);
			(**docInfo).selectedGraphic = nil;
		}
		CalcSelectionRgn(docInfo, true);
	}
}

void SetAnchorSelection(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	if((**docInfo).flags.anchorEnd) {
		if(selection->offset > (**docInfo).selEnd.offset) {
			(**docInfo).selStart = (**docInfo).selEnd;
			(**docInfo).selEnd = *selection;
			(**docInfo).flags.anchorEnd = false;
			(**docInfo).flags.anchorStart = true;
		} else {
			(**docInfo).selStart = *selection;
			(**docInfo).flags.anchorStart = false;
		}
	} else {
		if(selection->offset < (**docInfo).selStart.offset) {
			(**docInfo).selEnd = (**docInfo).selStart;
			(**docInfo).selStart = *selection;
			(**docInfo).flags.anchorEnd = true;
			(**docInfo).flags.anchorStart = false;
		} else {
			(**docInfo).selEnd = *selection;
			(**docInfo).flags.anchorStart = true;
		}
	}
}