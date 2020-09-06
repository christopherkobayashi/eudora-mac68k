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

void GetDefaultLineHeight(DocumentInfoHandle docInfo, long paraNum, LineMeasure *theMeasure)
{
	LongStyleRun tempStyleRun;
	LongSTElement theStyle;
	
	GetStyleRun(&tempStyleRun, (**docInfo).paraArray[paraNum].paraInfo, 0L);
	GetStyle(&theStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	
	theMeasure->ascent = theStyle.ascent;
	theMeasure->height = theStyle.ascent + theStyle.descent + theStyle.leading;
}

/* Add the height of this line if it's taller */
void AddRunHeight(StringPtr /* theText */, long /* textLen */, LongSTPtr theStyle, LineMeasure *theMeasure, WhiteInfoHandle /* whitespaceGlobals */, Boolean /* graphic */)
{
	short ascent, descent, height;

	ascent = theStyle->ascent;
	descent = theStyle->descent;

	/* If it's a TrueType font, actually measure the characters */
//	if(textLen > 0L) {
//		Point scaling;

		/* Initialize scaling factors to 1:1 */
//		scaling.h = scaling.v = 1;
	
//		if(IsOutline(scaling, scaling)) {
//			ascent = descent = 0L;
//			OutlineMetrics(textLen, theText, scaling, scaling, (short *)&ascent, (short *)&descent, nil, nil, nil);
//		}
//	}

	height = descent + theStyle->leading;
	height += (ascent > theMeasure->ascent) ? ascent : theMeasure->ascent;

//	if((textLen > 0L) &&
//	   ((height > theMeasure->height) || (ascent > theMeasure->ascent)) &&
//	   (!graphic && IsWhitespace(theText, textLen, smCurrentScript, whitespaceGlobals, nil))) {
//		ascent = 0L;
//		height = 0L;
//	}
	if(ascent > theMeasure->ascent) {
		theMeasure->ascent = ascent;
	}
	if(height > theMeasure->height) {
		theMeasure->height = height;
	}
}

/* Find the width of the tab run */
short TabWidth(ParagraphInfoHandle paraInfo, short textPosition, short docWidth)
{
	short tabCounter;
	short tabWidth, theMargin, tabPosition;
	Boolean fixedTab;
	
	/* Special case for tab to the point of paragraph indent */
	tabWidth = (**paraInfo).indent;
	if(tabWidth < 0L) {
		tabWidth = -tabWidth;
	}
	tabWidth += (**paraInfo).startMargin;
	tabWidth -= textPosition;
	
	/* If tab was before the paragraph indent, count that as an implicit tab and return */
	if(tabWidth > 0L) {
		goto DoReturn;
	}

	fixedTab = ((**paraInfo).tabCount < 0);
	
	/* If justification is default for direction, get the next tab after textPosition */
	if(((**paraInfo).justification == teFlushDefault) ||
	   (((**paraInfo).direction == leftCaret) && ((**paraInfo).justification == teFlushLeft)) ||
	   (((**paraInfo).direction != leftCaret) && ((**paraInfo).justification == teFlushRight))) {

		theMargin = (**paraInfo).endMargin;
		if(theMargin < 0L) {
			theMargin += docWidth;
		}
		
		tabPosition = (**paraInfo).startMargin;
		if((**paraInfo).tabCount != 0) {
			tabCounter = -1;
			do {
				++tabCounter;
				if(fixedTab) {
					tabPosition += (**paraInfo).tabStops[0];
				} else {
					tabPosition = (**paraInfo).tabStops[tabCounter];
				}
			
				/* Find the tab stop that is just past the current position */
			} while((fixedTab || (tabCounter < (**paraInfo).tabCount)) &&
			         (textPosition >= tabPosition) &&
			         (theMargin > tabPosition));
			
			/* Get the distance between the tab stop and the current position */
			tabWidth = ((tabPosition <= theMargin) ? tabPosition : theMargin) - textPosition;
		} else {
			/* The tab stop is the ending margin */
			tabWidth = theMargin - textPosition;
		}
	/* If justification opposite of direction, get the tab preceeding textPosition */
	} else {

		theMargin = (**paraInfo).startMargin;
		
		if(fixedTab) {
			tabCounter = 0;
			tabPosition = theMargin + (((textPosition - theMargin) / (**paraInfo).tabStops[0]) * (**paraInfo).tabStops[0]);
		} else {
			/* Start at the last tab */
			tabCounter = (**paraInfo).tabCount;
			
			/* Find the tab stop that is just before the current position */
			do {
				--tabCounter;
				tabPosition = (**paraInfo).tabStops[tabCounter];
			} while(tabCounter >= 0 && tabPosition > textPosition && tabPosition > theMargin);
		}
		
		/* Get the distance between the tab stop and the current position */
		if(tabCounter >= 0 && tabPosition > theMargin) {
			tabWidth = textPosition - tabPosition;
		} else {
			/* The tab stop is the beginning margin */
			tabWidth = textPosition - theMargin;
		}
	}

	/* Check for negatives */
	if(tabWidth < 0L) {
		tabWidth = -tabWidth;
	}

DoReturn :
	/* Return the width of the space to move over */
	return tabWidth;
}

/* Fill in the text measurements in the ordering structure */
void MeasureRuns(DocumentInfoHandle docInfo, long paraIndex, OrderHandle orderingHndl, short numRuns, Boolean firstLine)
{
	StringHandle theText;
	ParagraphInfoHandle paraInfo;
	short totalWidth, textWidth, rightWidth, leftWidth;
	long rightLen, leftLen, runLen;
	short runIndex, countIndex;
	StringPtr textPtr;
	LongSTElement tempStyle;
	Byte hState;
	PETEPortInfo savedInfo;

	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedInfo);

	theText = (**docInfo).theText;
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	/* Get the starting position on the line (for measuring tabs) */
	totalWidth = IndentWidth(paraInfo, firstLine) + (**paraInfo).startMargin;

	/* If flush right, count backwards from the end */
	if(((((**paraInfo).justification == teFlushDefault) && ((**paraInfo).direction != leftCaret)) || ((**paraInfo).justification == teFlushRight))) {
		runIndex = numRuns - 1;
		countIndex = -1;
	/* If flush left, count forwards from the right */
	} else {
		runIndex = 0;
		countIndex = 1;
	}

	/* Loop through the style runs */
	for( ; (runIndex >= 0 && runIndex < numRuns); totalWidth += textWidth, runIndex += countIndex) {
	
		/* Get the style info for this style run */
		GetStyle(&tempStyle, (**docInfo).theStyleTable, (*orderingHndl)[runIndex].styleIndex);
		SetTextStyleWithDefaults(nil, docInfo, &tempStyle.stInfo.textStyle, false, ((**docInfo).printData != nil));
		
		/* If the styleIndex is less than zero, it's a graphic */
		if((*orderingHndl)[runIndex].isGraphic) {
			leftWidth = rightWidth = textWidth = ((tempStyle.stInfo.graphicStyle.graphicInfo != nil) ? (**(tempStyle.stInfo.graphicStyle.graphicInfo)).width : 0L);
			leftLen = rightLen = 1L;
		/* If it's a tab and the paragraph is not centered, measure the tab width */
		} else if(((*orderingHndl)[runIndex].isTab) && ((**paraInfo).justification != teCenter)) {
			textWidth = TabWidth(paraInfo, totalWidth, (**docInfo).docWidth);

			/* The visible width is either the whole width or 0 */
			if(!(Boolean)GetScriptVariable(smCurrentScript, smScriptRight)) {
				/* Left to right tab, so the left to right width is 0 */
				leftWidth = 0L;
				rightWidth = textWidth;
			} else {
				/* Right to left tab, so the right to left width is 0 */
				rightWidth = 0L;
				leftWidth = textWidth;
			}

			leftLen = rightLen = 1L;

		/* It's normal text, so just measure it (if it's not already measured) */
		} else if(((textWidth = (*orderingHndl)[runIndex].width) == 0L) && ((runLen = (*orderingHndl)[runIndex].len) != 0L)){
		
			hState = HGetState((Handle)theText);
			HLock((Handle)theText);
			
			/* Find the appropriate place in the text */
			textPtr = *theText + ((*orderingHndl)[runIndex].offset - (**docInfo).textOffset);
	
			/* Get the full text width */
			textWidth = MyTextWidth(textPtr, 0, runLen, (**docInfo).globals);

			/* Get the length if this was at the end of a left to right line */
			leftLen = MyVisibleLength((Ptr)textPtr, runLen, leftCaret);

			/* See if we still have the full length */
			if(leftLen == 0L) {
				/* All whitespace, so the width is 0 */
				leftWidth = 0L;
				(*orderingHndl)[runIndex].isWhitespace = true;
			} else if(leftLen == runLen) {
				/* Full length, so the width is the same */
				leftWidth = textWidth;
			} else {
				/* Measure the shortened text */
				leftWidth = MyTextWidth(textPtr, 0, leftLen, (**docInfo).globals);
			}

			/* Get the length if this was at the end of a right to left line */
			rightLen = MyVisibleLength((Ptr)textPtr, runLen, rightCaret);
			
			/* See if we still have the full length */
			if(rightLen == 0L) {
				/* All whitespace, so the width is 0 */
				rightWidth = 0L;
			} else if(rightLen == runLen) {
				/* Full length, so the width is the same */
				rightWidth = textWidth;
			} else {
				/* Measure the shortened text */
				rightWidth = MyTextWidth(textPtr, 0, rightLen, (**docInfo).globals);
			}

			HSetState((Handle)theText, hState);
	
		/* If it's already been measured, don't reset the values */
		} else {
			continue;
		}

		/* Set the width fields */
		(*orderingHndl)[runIndex].leftLen = leftLen;
		(*orderingHndl)[runIndex].rightLen = rightLen;
		(*orderingHndl)[runIndex].width = textWidth;
		(*orderingHndl)[runIndex].leftWidth = leftWidth;
		(*orderingHndl)[runIndex].rightWidth = rightWidth;

	}
	
	ResetPortInfo(&savedInfo);
}

/* Calculate the width of the line, subtracting trailing whitespace */
void VisibleWidth(LayoutCachePtr lineCache, short numRuns)
{
	short runIndex;
	OrderHandle orderingHndl;
	short textWidth;
	
	/* Initalize the trailing whitespace entries */
	lineCache->leftWhiteWidth = 0L;
	lineCache->rightWhiteWidth = 0L;
	
	orderingHndl = lineCache->orderingHndl;
	
	/* Start with the left hand style run */
	runIndex = 0;
	for(runIndex = 0, textWidth = 0L; ((textWidth == 0L) && (runIndex < numRuns)); ++runIndex) {
		/* Get the width as if this were the end of a right to left line */
		textWidth = (*orderingHndl)[runIndex].rightWidth;
		
		/* Measure the amount of trailing right to left whitespace */
		lineCache->leftWhiteWidth += (*orderingHndl)[runIndex].width - textWidth;
	
	}
	
	/* Start with the right hand style run */
	runIndex = numRuns - 1;
	for(runIndex = numRuns - 1, textWidth = 0L; ((textWidth == 0L) && (runIndex >= 0)); --runIndex) {
		/* Get the width as if this were the end of a left to right line */
		textWidth = (*orderingHndl)[runIndex].leftWidth;
		
		/* Measure the amount of trailing left to right whitespace */
		lineCache->rightWhiteWidth += (*orderingHndl)[runIndex].width - textWidth;
	
	}

	/* Add up the widths for all of the runs */
	for(textWidth = 0L, runIndex = numRuns - 1; runIndex >= 0; --runIndex) {
		textWidth += (*orderingHndl)[runIndex].width;
	}
	
	/* Visible width is the full width excluding the trailing whitespace */
	lineCache->visibleWidth = textWidth - (lineCache->rightWhiteWidth + lineCache->leftWhiteWidth);
}

/* Calculate the indentation for this line */
short IndentWidth(ParagraphInfoHandle paraInfo, Boolean firstLine)
{
	short indentWidth = (**paraInfo).indent;
	
	/* Is this the first line in a paragraph? */
	if(!firstLine) {
		/* Middle line, so indentation is going to be negative */
		indentWidth = -indentWidth;
	}
	
	/* If indentation is now negative, this isn't a line to indent */
	if(indentWidth < 0L) {
		indentWidth = 0L;
	}
	
	if((**paraInfo).paraFlags & peListBits) {
		indentWidth += (((**paraInfo).tabCount == 0) ? (4 * kDefaultMargin) : ((**paraInfo).tabStops[0] / 3));
	}
	
	indentWidth += (((**paraInfo).quoteLevel + (**paraInfo).signedLevel) * kDefaultMargin * 2);
	
	return indentWidth;
}

short LeftPosition(ParagraphInfoHandle paraInfo, short docWidth, short visibleWidth, short *totalSlop, Boolean firstLine)
{
	short leftPosition, slop, indentWidth;
	
	/* Slop width is the full display width less the visible width of the text */
	slop = (**paraInfo).endMargin;
	if(slop < 0L) {
		slop += docWidth;
	}
	slop -= (**paraInfo).startMargin;
	slop -= visibleWidth;
	
	/* Initialize the left hand screen position */
	if((**paraInfo).direction == leftCaret) {
		leftPosition = (**paraInfo).startMargin;
	} else {
		leftPosition = -(**paraInfo).endMargin;
		if(leftPosition < 0L) {
			leftPosition += docWidth;
		}
	}
	
	/* Decide how much to move over for slop */
	switch((**paraInfo).justification) {
		case teCenter :
			/* Centered, move over half the slop */
			leftPosition += slop >> 1;
			goto ZeroSlop;
		case teFlushDefault :
			/* Default line direction, move over only if it's flush right */
			if((**paraInfo).direction != leftCaret) {
		case teFlushRight :
			/* Flush right, move over the full slop */
				leftPosition += slop;
			}		
		case teFlushLeft :
			/* Flush left, don't move over at all */
		ZeroSlop :
			/* For all above cases, zero out slop */
			slop = 0L;
		default :
			/* If justified text is ever supported, leave slop to be distributed */
			;
	}

	if(totalSlop != nil) {
		*totalSlop = slop;
	}
	/* Get the indent width for this line */
	indentWidth = IndentWidth(paraInfo, firstLine);

	/* Is this a left to right paragraph? */
	if((**paraInfo).direction == leftCaret) {
		/* If flush right, the indentation is already accounted for */
		if((**paraInfo).justification == teFlushRight) {
			indentWidth = 0L;
		}
	} else {
		/* In a right to left paragraph, indentation is on the right (i.e. reversed) */
		if(((**paraInfo).justification == teFlushRight) || ((**paraInfo).justification == teFlushDefault)) {
			indentWidth = -indentWidth;
		} else {
			/* If flush left, indentation is already accounted for */
			indentWidth = 0L;
		}

		if((**paraInfo).endMargin < 0L) {
			indentWidth -= (**paraInfo).endMargin;
		} else {
			indentWidth += docWidth - (**paraInfo).endMargin;
		}
	}
	
	/* Add the indentation to the line position */
	return leftPosition + indentWidth;
	
}

OSErr RecalcDocHeight(DocumentInfoHandle docInfo)
{
	OSErr errCode;
	long paraIndex;
	
	if((**docInfo).flags.docHeightValid) {
		return noErr;
	}
	
	for(paraIndex = (**docInfo).paraCount, errCode = noErr; --paraIndex >= 0L && errCode == noErr;) {
		errCode = CheckParagraphMeasure(docInfo, paraIndex, false);
	}
	if(errCode == noErr) {
		(**docInfo).flags.docHeightValid = true;
	}
	return errCode;
}

long GetDocHeight(DocumentInfoHandle docInfo)
{
	RecalcDocHeight(docInfo);
	return (**docInfo).docHeight;
}