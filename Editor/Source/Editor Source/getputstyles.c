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

void FlushStyleRunCache(ParagraphInfoHandle paraInfo)
{
	(**paraInfo).styleRunCacheIndex = 0L;
	(**paraInfo).styleRunCacheOffset = 0L;
}

long CountStyleRuns(ParagraphInfoHandle paraInfo)
{
	return ((InlineGetHandleSize((Handle)paraInfo) - (offsetof(ParagraphInfo, tabStops) + (ABS((**paraInfo).tabCount) * sizeof(short)))) / sizeof(LongStyleRun)) - 1L;
}

/* Get the style run record. This will be fancier, reading off the disk, later */
long GetStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex)
{
	LongStyleRunPtr styleRunPtr;
	
	styleRunPtr = (LongStyleRunPtr)&(**paraInfo).tabStops[ABS((**paraInfo).tabCount)];
	*theStyleRun = styleRunPtr[styleRunIndex];
	return theStyleRun->srStyleLen;
}

/* Get the style record. This will be fancier, reading off the disk, later */
void GetStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long styleIndex)
{
	*theStyle = (**theStyleTable).styleList[styleIndex];
}

/* Add a style run to the paragraph info. Fancier later */
OSErr AddStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex)
{
	if(theStyleRun->srIsGraphic)
		theStyleRun->srIsTab = false;
	Munger((Handle)paraInfo,
	       sizeof(ParagraphInfo) + (sizeof(short) * (ABS((**paraInfo).tabCount) - 1)) + (styleRunIndex * sizeof(LongStyleRun)),
	       nil, 0L, theStyleRun, sizeof(LongStyleRun));
	return MemError();
}

OSErr DeleteStyle(LongSTTable theStyleTable, long styleIndex)
{
	LongSTElement tempStyle;
	OSErr errCode;
	long numStyles;
	
	GetStyle(&tempStyle, theStyleTable, styleIndex);
	
	if (tempStyle.stCount > 0)
	{
		--tempStyle.stCount;
		errCode = ChangeStyle(&tempStyle, theStyleTable, styleIndex);
		if (errCode == noErr && tempStyle.stCount == 0L)
			DeleteGraphicFromStyle(theStyleTable,styleIndex);
	}

	if(tempStyle.stCount == 0L) {
		styleIndex = numStyles = NumStyles(theStyleTable);
		do {
			GetStyle(&tempStyle, theStyleTable, --styleIndex);
		} while((tempStyle.stCount == 0L) && (styleIndex > 0L));
		++styleIndex;
		if(styleIndex != numStyles) {
			SetHandleSize((Handle)theStyleTable, styleIndex * sizeof(LongSTElement) + sizeof(LongSTTable));
		}
	}
	return errCode;
}

long FindStyleIndex(PETEStyleInfoPtr theStyle, LongSTTable theStyleTable, Boolean graphic);
long FindStyleIndex(PETEStyleInfoPtr theStyle, LongSTTable theStyleTable, Boolean graphic)
{
	long styleIndex, numStyles;
	LongSTElement tempStyle;

	for(styleIndex = 0L, numStyles = NumStyles(theStyleTable); styleIndex < numStyles; ++styleIndex) {

		GetStyle(&tempStyle, theStyleTable, styleIndex);
		
		/* Check to see if the text styles are equal */
		if((!graphic == !tempStyle.stIsGraphic) &&
		   ((!graphic && EqualStyle((PETETextStylePtr)&tempStyle.stInfo.textStyle, &theStyle->textStyle)) ||
		    (graphic && EqualGraphic((PETEGraphicStylePtr)&tempStyle.stInfo.graphicStyle, &theStyle->graphicStyle)))) {
			break;
		}
	}
	return styleIndex;
}

/* Increment the style count or append the style to the style table */
OSErr AddStyle(PETEStyleInfoPtr newStyle, LongSTTable theStyleTable, unsigned long *curStyleIndex, Boolean graphic, Boolean printing)
{
	long styleIndex;
	LongSTElement tempStyle;
	FMetricRec theMetrics;
	PETEPortInfo savedPortInfo;
	OSErr errCode;
	
	styleIndex = FindStyleIndex(newStyle, theStyleTable, graphic);
	if(styleIndex < NumStyles(theStyleTable)) {
		GetStyle(&tempStyle, theStyleTable, styleIndex);
		/* Increment the count field; there's on more occurance of this style */
		++tempStyle.stCount;
		
		/* Update the entry */
		errCode = ChangeStyle(&tempStyle, theStyleTable, styleIndex);
	} else {
		/* The style was not found, so append a new one */
		tempStyle.stInfo = *(StyleInfoPtr)newStyle;
		tempStyle.stIsGraphic = !(!graphic);
		SavePortInfo(nil, &savedPortInfo);
		SetTextStyleWithDefaults(nil, (**theStyleTable).docInfo, &tempStyle.stInfo.textStyle, false, printing);
		FontMetrics(&theMetrics);
		ResetPortInfo(&savedPortInfo);
		tempStyle.ascent = FixRound(theMetrics.ascent);
		tempStyle.descent = FixRound(theMetrics.descent);
		tempStyle.leading = FixRound(theMetrics.leading);
		tempStyle.stCount = 1;
		errCode = AppendStyle(&tempStyle, theStyleTable, &styleIndex);

		if((errCode == noErr) && tempStyle.stIsGraphic) {
			errCode = CallGraphic((**theStyleTable).docInfo, tempStyle.stInfo.graphicStyle.graphicInfo, kPETEOffsetUnknown, peGraphicInsert, nil);
			if(errCode == invalidHandler) {
				errCode = noErr;
			}
		}
		
	}
	
	/* If a style index needs to be returned, fill in the value */
	if(curStyleIndex != nil) {
		*curStyleIndex = styleIndex;
	}

	return errCode;
}

OSErr CompareStyles(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleEntryPtr style1, PETEStyleEntryPtr style2, long validBits, Boolean printing, long *diffBits)
{
	PETEStyleEntry s1, s2;
	LongSTElement tempStyle;
	
	*diffBits = 0L;
	if(style1 == style2) {
		return noErr;
	}
	
	if((style1 == nil) || (style2 == nil)) {
		GetStyle(&tempStyle, (**docInfo).theStyleTable, 0L);
		if(style1 == nil) {
			s1.psGraphic = tempStyle.stIsGraphic;
			s1.psStyle = tempStyle.stInfo;
			s2 = *style2;
		} else {
			s2.psGraphic = tempStyle.stIsGraphic;
			s2.psStyle = tempStyle.stInfo;
			s1 = *style1;
		}
	} else {
		s1 = *style1;
		s2 = *style2;
	}
	
	if(validBits & peGraphicValid) {
		if(!(!s1.psGraphic) != !(!s2.psGraphic)) {
			*diffBits |= peGraphicValid;
		} else if(s1.psGraphic) {
			if(s1.psStyle.graphicStyle.graphicInfo != s2.psStyle.graphicStyle.graphicInfo) {
				*diffBits |= peGraphicValid;
			}
			validBits &= ~peColorValid;
		}
		validBits &= ~peGraphicValid;
	}
	if(validBits & peColorValid) {
		if(s1.psGraphic) {
			s1.psGraphic = false;
			SetPETEDefaultColor(s1.psStyle.textStyle.tsColor);
		}
		if(s2.psGraphic) {
			s2.psGraphic = false;
			SetPETEDefaultColor(s2.psStyle.textStyle.tsColor);
		}
	}
	/* Do size before font because size might depend on original font */
	if(validBits & peSizeValid) {
		s1.psStyle.textStyle.tsSize = StyleToFontSize(globals, docInfo, &s1.psStyle.textStyle, printing);
		s2.psStyle.textStyle.tsSize = StyleToFontSize(globals, docInfo, &s2.psStyle.textStyle, printing);
	} else {
		s1.psStyle.textStyle.tsSize = kPETEDefaultFont;
		s2.psStyle.textStyle.tsSize = kPETEDefaultFont;
	}
	if(validBits & peFontValid) {
		s1.psStyle.textStyle.tsFont = StyleToFont(globals, docInfo, &s1.psStyle.textStyle, printing);
		s2.psStyle.textStyle.tsFont = StyleToFont(globals, docInfo, &s2.psStyle.textStyle, printing);
	} else {
		s1.psStyle.textStyle.tsFont = kPETEDefaultFont;
		s2.psStyle.textStyle.tsFont = kPETEDefaultFont;
	}
	if(validBits & peColorValid) {
		if(IsPETEDefaultColor(s1.psStyle.textStyle.tsColor)) {
			s1.psStyle.textStyle.tsColor = DocOrGlobalColor(globals, docInfo);
		}
		if(IsPETEDefaultColor(s2.psStyle.textStyle.tsColor)) {
			s2.psStyle.textStyle.tsColor = DocOrGlobalColor(globals, docInfo);
		}
	}
	*diffBits |= ChangeStyleInfo(&s1.psStyle, &s2.psStyle, validBits, false);
	return noErr;
}

/* Compare two text styles for equality */
Boolean EqualStyle(PETETextStylePtr theStyle1, PETETextStylePtr theStyle2)
{
	return((theStyle1->tsFont == theStyle2->tsFont) &&
	       (theStyle1->tsFace == theStyle2->tsFace) &&
	       (theStyle1->tsSize == theStyle2->tsSize) &&
	       (theStyle1->tsColor.red == theStyle2->tsColor.red) &&
	       (theStyle1->tsColor.green == theStyle2->tsColor.green) &&
	       (theStyle1->tsColor.blue == theStyle2->tsColor.blue) &&
	       (theStyle1->tsLang == theStyle2->tsLang) &&
	       (theStyle1->tsLock == theStyle2->tsLock) &&
	       (theStyle1->tsLabel == theStyle2->tsLabel));
}

Boolean EqualGraphic(PETEGraphicStylePtr theStyle1, PETEGraphicStylePtr theStyle2)
{
	return((theStyle1->tsFont == theStyle2->tsFont) &&
	       (theStyle1->tsFace == theStyle2->tsFace) &&
	       (theStyle1->tsSize == theStyle2->tsSize) &&
	       (theStyle1->graphicInfo == theStyle2->graphicInfo) &&
	       (theStyle1->tsLang == theStyle2->tsLang) &&
	       (theStyle1->tsLock == theStyle2->tsLock) &&
	       (theStyle1->tsLabel == theStyle2->tsLabel));
}

/* Count the number of styles in the style table */
long NumStyles(LongSTTable theStyleTable)
{
	return (InlineGetHandleSize((Handle)theStyleTable) - sizeof(LongSTTable)) / sizeof(LongSTElement);
}

/* Add a new style to the style table */
OSErr AppendStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long *styleIndex)
{
	long styleCount = NumStyles(theStyleTable);
	LongSTElement tempStyle;
	
	if(theStyle->stIsGraphic && !GraphicInList((**(**theStyleTable).docInfo).globals, theStyle->stInfo.graphicStyle.graphicInfo))
	{
		return invalidHandler;
	}
	for(*styleIndex = 0L; *styleIndex < styleCount; ++*styleIndex) {
		GetStyle(&tempStyle, theStyleTable, *styleIndex);
		if(tempStyle.stCount == 0L) {
			return ChangeStyle(theStyle, theStyleTable, *styleIndex);
		}
	}
	return PtrAndHand(theStyle, (Handle)theStyleTable, sizeof(LongSTElement));
}

/* Update the information in a style entry; usually for incrementing the count */
OSErr ChangeStyle(LongSTPtr theStyle, LongSTTable theStyleTable, long styleIndex)
{
	if(theStyle->stIsGraphic && !GraphicInList((**(**theStyleTable).docInfo).globals, theStyle->stInfo.graphicStyle.graphicInfo))
	{
		return invalidHandler;
	}
	(**theStyleTable).styleList[styleIndex] = *theStyle;
	
	return noErr;
}

OSErr ChangeStyleRun(LongStyleRunPtr theStyleRun, ParagraphInfoHandle paraInfo, long styleRunIndex)
{
	LongStyleRunPtr styleRunPtr;
	
	styleRunPtr = (LongStyleRunPtr)&(**paraInfo).tabStops[ABS((**paraInfo).tabCount)];
	styleRunPtr[styleRunIndex] = *theStyleRun;
	
	return noErr;
}

void CompressStyleRuns(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex, long runsAdded)
{
	LongStyleRun tempStyleRun1, tempStyleRun2;
	
	if(styleRunIndex >= CountStyleRuns(paraInfo))
		return;

	do {
		if(styleRunIndex != 0L) {
			GetStyleRun(&tempStyleRun1, paraInfo, styleRunIndex - 1L);
			GetStyleRun(&tempStyleRun2, paraInfo, styleRunIndex);
			
			if((tempStyleRun1.srStyleIndex == tempStyleRun2.srStyleIndex) &&
			   !(tempStyleRun1.srIsTab || tempStyleRun2.srIsTab) &&
			   (tempStyleRun2.srStyleLen >= 0L)) {
				tempStyleRun1.srStyleLen += tempStyleRun2.srStyleLen;
				ChangeStyleRun(&tempStyleRun1, paraInfo, styleRunIndex - 1L);
				DeleteStyleRun(paraInfo, theStyleTable, styleRunIndex);
			} else if(tempStyleRun1.srStyleLen == 0L) {
				DeleteStyleRun(paraInfo, theStyleTable, styleRunIndex - 1L);
			} else {
				++styleRunIndex;
			}
			--runsAdded;
		} else {
			++styleRunIndex;
		}
	} while(runsAdded > 0L);

}

/* Delete all of the styles of a given paragraph from the style table */
OSErr DeleteParaStyleRuns(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex, long runsToDelete)
{
	OSErr errCode;
	
	if(runsToDelete < 0L) {
		runsToDelete = CountStyleRuns(paraInfo) - styleRunIndex;
	}
	
	/* Loop through all of the style runs in the paragraph */
	for(errCode = noErr; (errCode == noErr) && (--runsToDelete >= 0L); ) {
		errCode = DeleteStyleRun(paraInfo, theStyleTable, styleRunIndex + runsToDelete);
	}
	
	return (errCode == invalidIndexErr) ? noErr : errCode;
}

OSErr DeleteStyleRun(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, long styleRunIndex)
{
	LongStyleRun tempStyleRun;
	OSErr errCode;

	if(styleRunIndex < CountStyleRuns(paraInfo)) {

		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		errCode = RemoveStyleRuns(paraInfo, styleRunIndex, 1L);
		
		if(errCode == noErr) {
			errCode = DeleteStyle(theStyleTable, tempStyleRun.srStyleIndex);
		}			
	} else {
		errCode = invalidIndexErr;
	}
	
	return errCode;
}

OSErr RemoveStyleRuns(ParagraphInfoHandle paraInfo, long styleRunIndex, long styleRunCount)
{
	long currentCount;
	
	if(styleRunCount == 0L) {
		return noErr;
	}
	
	currentCount = CountStyleRuns(paraInfo);
	if(styleRunIndex == currentCount) {
		return noErr;
	}
	
	if(styleRunIndex + styleRunCount > currentCount) {
		return inputOutOfBounds;
	}
	
	if((styleRunIndex < 0L) || (styleRunCount < 0L)) {
		return inputOutOfBounds;
	}	
	
	Munger((Handle)paraInfo,
	       sizeof(ParagraphInfo) + (sizeof(short) * (ABS((**paraInfo).tabCount) - 1)) + (styleRunIndex * sizeof(LongStyleRun)),
	       nil, sizeof(LongStyleRun) * styleRunCount, (Ptr)1L, 0L);

	return MemError();		
}

OSErr GetGraphicInfoFromOffset(DocumentInfoHandle docInfo, long offset, GraphicInfoHandle *graphicInfo, long *length)
{
	OSErr errCode;
	PETEStyleEntry tempStyle;
	
	errCode = GetStyleFromOffset(docInfo, offset, length, &tempStyle);
	if(errCode == noErr) {
		*graphicInfo = tempStyle.psStyle.graphicStyle.graphicInfo;
	}
	return errCode;
}

OSErr GetStyleFromOffset(DocumentInfoHandle docInfo, long offset, long *length, PETEStyleEntryPtr theStyle)
{
	SelData selection;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	OSErr errCode;
	
	if(offset == kPETEDefaultStyle) {
		if(theStyle != nil) {
			GetStyle(&tempStyle, (**docInfo).theStyleTable, 0L);
			theStyle->psStartChar = 0L;
			theStyle->psGraphic = tempStyle.stIsGraphic;
			theStyle->psStyle = tempStyle.stInfo;
		}
		return noErr;
	} else if(offset == kPETECurrentStyle) {
		if(theStyle != nil) {
			theStyle->psStartChar = 0L;
			theStyle->psGraphic = 0L;
			theStyle->psStyle.textStyle = (**docInfo).curTextStyle;
		}
		return noErr;
	}
	
	if(offset == kPETECurrentSelection) {
		offset = (**docInfo).selStart.offset;
	}
	
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
		return noErr;
	}
	GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
	if(length != nil) {
		*length = tempStyleRun.srStyleLen;
	}
	if(theStyle != nil) {
		theStyle->psStartChar = selection.offset - offset;
		theStyle->psGraphic = tempStyle.stIsGraphic;
		theStyle->psStyle = tempStyle.stInfo;
	}
	return noErr;
}

OSErr GetSelectionStyle(DocumentInfoHandle docInfo, SelDataPtr selection, LongStyleRunPtr styleRun, long *styleRunOffset, MyTextStylePtr textStyle)
{
	ParagraphInfoHandle paraInfo;
	long offset;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	OSErr errCode;
	
	if(!selection->lastLine && !selection->leadingEdge) {
		errCode = CheckParagraphMeasure(docInfo, selection->paraIndex, true);
		if(errCode != noErr) {
			return errCode;
		}
	
	}

	offset = selection->offset;

	/* Get the line starts information for the paragraph */
	paraInfo = (**docInfo).paraArray[selection->paraIndex].paraInfo;

	if(selection->lastLine || (!selection->leadingEdge && (offset != (*(**paraInfo).lineStarts)[selection->lineIndex].lsStartChar))) {
		--offset;
	}

	offset -= ParagraphOffset(docInfo, selection->paraIndex);
	
	if(offset < 0L)
		offset = 0L;
	
	GetStyleRun(&tempStyleRun, paraInfo, StyleRunIndex(paraInfo, &offset));
	if(styleRun != nil) {
		*styleRun = tempStyleRun;
	}
	if(styleRunOffset != nil) {
		*styleRunOffset = offset;
	}
	if(textStyle != nil) {
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		*textStyle = tempStyle.stInfo.textStyle;
		if(tempStyleRun.srIsGraphic) {
			SetPETEDefaultColor(textStyle->tsColor);
		}
	}
	return noErr;
}

/* Insert style information into a paragraph and style table */
OSErr StyleListToStyles(ParagraphInfoHandle paraInfo, LongSTTable theStyleTable, Ptr theText, long textSize, long hOffset, long textOffset, Handle listHandle, long listOffset, MyTextStylePtr textStyle, Boolean printing)
{
	PETEStyleList styleList;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	long listSize, runOffset, nextStart, nextTab, tabCount, thisStart, textLen, runsAdded, styleRunIndex, tempOffset;
	unsigned long tempStyleIndex;
	OSErr errCode;
	Boolean splitStyleRun;
	
	/* Initialize stuff */
	runsAdded = 0L;
	nextTab = -1L;
	tabCount = 1L;
	errCode = noErr;
	splitStyleRun = false;
	
	if(textSize < 0L) {
		textLen = InlineGetHandleSize((Handle)theText);
		hOffset = 0L;
	} else {
		textLen = textSize;
	}

	/* Is there a style list provided */
	if((listHandle != nil) && (listHandle != (Handle)-1L)) {
		/* Yes, so get the size of styles */
		listSize = InlineGetHandleSize(listHandle) - listOffset;
	} else {
		/* No, so this is just one style run */
		listSize = sizeof(PETEStyleEntry);
	}
	
	tempOffset = textOffset;
	styleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	if(tempOffset != 0L) {
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, theStyleTable, tempStyleRun.srStyleIndex);
		errCode = AddStyle(&tempStyle.stInfo, theStyleTable, &tempStyleIndex, tempStyleRun.srIsGraphic, printing);
		tempStyleRun.srStyleIndex = tempStyleIndex;
		if(errCode == noErr) {
			tempStyleRun.srStyleLen -= tempOffset;
			errCode = AddStyleRun(&tempStyleRun, paraInfo, styleRunIndex + 1L);
			if(errCode == noErr) {
				tempStyleRun.srStyleLen = tempOffset;
				errCode = ChangeStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
				if(errCode == noErr) {
					splitStyleRun = true;
					++styleRunIndex;
				} else {
					DeleteStyleRun(paraInfo, theStyleTable, styleRunIndex + 1L);
				}
			} else {
				DeleteStyle(theStyleTable, tempStyleRun.srStyleIndex);
			}
		}
	}
	
	/* Loop through all of the style runs in the scrap */
	for(runOffset = 0L; errCode == noErr && runOffset < listSize; runOffset += sizeof(PETEStyleEntry)) {

		if((listHandle != nil) && (listHandle != (Handle)-1L)) {
			/* The beginning of the style list is listOffset into listHandle */
			styleList = (PETEStyleList)(*listHandle + listOffset + runOffset);

			/* Get the style from the list */
			tempStyle.stInfo = styleList->psStyle;
			tempStyleRun.srIsGraphic = !(!(styleList->psGraphic & 1L));
			
			/* Get the starting position of this style run */
			thisStart = styleList->psStartChar;
			
		} else {
			/* There is no style list, so set up for plain text in the current style */
			if(listHandle != nil) {
				GetStyle(&tempStyle, theStyleTable, 0L);
			} else {
				tempStyle.stInfo.textStyle = *textStyle;
				tempStyleRun.srIsGraphic = false;
			}
			thisStart = 0L;
		}
		
		if(runOffset + sizeof(PETEStyleEntry) != listSize) {
			/* If this isn't the last run, get the start of the next run */
			nextStart = (styleList + 1L)->psStartChar;
			
			if(nextStart >= textLen) {
				runOffset = listSize;
				goto SetToTextLen;
			}
		} else {
		SetToTextLen :
			/* If this is the last run, get the length of the whole text */
			nextStart = textLen;
		}
		
		

		do {
			/* Find the next tab in the text if it's not set up */
			if(thisStart > nextTab) {
				do {
					nextTab = FindNextTab((((textSize < 0L) || (hOffset >= 0L)) ? *(Handle)theText + hOffset : theText), textLen, nextTab, &tabCount);
				
				/* Skip tabs in graphics */
				} while(tempStyleRun.srIsGraphic && (nextStart < nextTab) && (nextTab < textLen));
			}
			
			tempStyleRun.srIsTab = false;
			
			/* Is there a tab in this style run? */
			if(nextTab <= nextStart) {
				/* Yes. Is the tab at the beginning of the style run? */
				if(nextTab > thisStart) {
					/* No, so say that this style run length is up to the tab */
					tempStyleRun.srStyleLen = nextTab - thisStart;
					/* Next time around, the style run begins at the tab */
					thisStart = nextTab;
				} else {
					/* Move thisStart past the tab */
					++thisStart;
					/* Only one tab per style run */
					tempStyleRun.srStyleLen = 1L;
					/* Is there more than one tab in this style run? */
					if(tabCount > 1L) {
						/* Get the next tab next time */
						--tabCount;
						nextTab = thisStart;
					}
					tempStyleRun.srIsTab = true;
				}
			} else {
				/* No tabs, so just use the regular length */
				tempStyleRun.srStyleLen = nextStart - thisStart;
			}

			/* Add to the style table */
			errCode = AddStyle(&tempStyle.stInfo, theStyleTable, &tempStyleIndex, tempStyleRun.srIsGraphic, printing);
			tempStyleRun.srStyleIndex = tempStyleIndex;
			
			if(errCode == noErr) {
				/* Add the style run to the paragraph info */			
				errCode = AddStyleRun(&tempStyleRun, paraInfo, styleRunIndex + runsAdded);
				
				/* If an error occurred adding the style run, remove the style entry */
				if(errCode != noErr) {
					DeleteStyle(theStyleTable, tempStyleRun.srStyleIndex);
				} else {
					++runsAdded;
				}
			}
			
		/* Loop around while dealing with tabs */
		} while((errCode == noErr) && (nextTab <= nextStart) && (thisStart < nextStart));
	}
	
	/* If an error occurred, clean out all of the style entries */
	if(errCode != noErr) {
		DeleteParaStyleRuns(paraInfo, theStyleTable, styleRunIndex, runsAdded);
	}
	
	if((runsAdded > 0L) || splitStyleRun) {
		CompressStyleRuns(paraInfo, theStyleTable, styleRunIndex, runsAdded + (splitStyleRun ? 1L : 0L));
	}
	
	return errCode;
	
}

OSErr ChangeStyleRange(DocumentInfoHandle docInfo, long startOffset, long endOffset, StyleInfoPtr newStyle, long validBits)
{
	long styleRunIndex, startStyleRunIndex, endStyleRunIndex, paraIndex, endParaIndex, curOffset, endRunOffset;
	unsigned long tempStyleIndex;
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun, oldStyleRun;
	LongSTElement tempStyle, endStyle;
	Boolean endIsTab, doUndo, wasChanged;
	OSErr errCode;
	
	if(((startOffset < -1L) || (endOffset < -1L)) && (startOffset != endOffset)) {
		return errAEImpossibleRange;
	}
	
	if((validBits & peGraphicValid) && ((validBits & peColorValid) || (startOffset == kPETEDefaultStyle) || (startOffset == kPETECurrentStyle))) {
		return paramErr;
	}
	
	/* Turn off caret just in case there's a redraw */
	EraseCaret(docInfo);
	
	/* If kPETEDefaultStyle, just change style 0 in the table */
	if(startOffset == kPETEDefaultStyle) {
		GetStyle(&tempStyle, (**docInfo).theStyleTable, 0L);
		if(ChangeStyleInfo(newStyle, &tempStyle.stInfo, validBits, false) && (ChangeStyle(&tempStyle, (**docInfo).theStyleTable, 0L), ((validBits & ~peLockValid) != 0L))) {
			return ResetAndInvalidateDocument(docInfo);
		} else {
			return noErr;
		}
	}
	
	doUndo = false;
	
	/* If the values are both negative, the current style must change */
	if((startOffset < 0L) && (endOffset < 0L)) {
		tempStyle.stInfo.textStyle = (**docInfo).curTextStyle;
		ChangeStyleInfo(newStyle, &tempStyle.stInfo, validBits, false);
		(**docInfo).curTextStyle = tempStyle.stInfo.textStyle;
		if(validBits & peFontValid) {
			SetStyleAndKeyboard(docInfo, &tempStyle.stInfo.textStyle);
		}
		if(startOffset == kPETECurrentSelection) {
			doUndo = true;
		}
	}
	
	/* If only the current style is changing, return */
	if(startOffset == kPETECurrentStyle) {
		return noErr;
	}
	
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
	
	if(doUndo) {
		errCode = SetParaStyleUndo(docInfo, startOffset, endOffset, peUndoStyle);
		if(errCode != noErr) {
			return errCode;
		}
	}
	
	/* Get the starting and ending paragraphs */
	paraIndex = ParagraphIndex(docInfo, startOffset);
	endParaIndex = ParagraphIndex(docInfo, endOffset - 1L);

	/* Get the offsets into the starting and ending paragraph */
	curOffset = startOffset - ParagraphOffset(docInfo, paraIndex);
	endRunOffset = endOffset - ParagraphOffset(docInfo, endParaIndex) - 1L;

	paraInfo = (**docInfo).paraArray[endParaIndex].paraInfo;

	/* Get the ending style run index and the offset into that style run */
	endStyleRunIndex = StyleRunIndex(paraInfo, &endRunOffset);
	
	/* Get the ending style itself */
	GetStyleRun(&tempStyleRun, paraInfo, endStyleRunIndex);
	endIsTab = tempStyleRun.srIsTab;
	GetStyle(&endStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);

	++endRunOffset;
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	/* Get the starting style run index and offset into that style run */
	styleRunIndex = StyleRunIndex(paraInfo, &curOffset);
	/* Increment before entering the loop */
	startStyleRunIndex = styleRunIndex + 1L;
	
	/* If only one style run is being changed, subtract off the beginning of the style run */
	if((paraIndex == endParaIndex) && (styleRunIndex == endStyleRunIndex)) {
		endRunOffset -= curOffset;
	}
	
	wasChanged = false;
	do {
		
		(**docInfo).flags.scrollsDirty = true;
		
		/* Get the style of the current style run */
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		
		/* Change the style according to the parameters */
		if(ChangeStyleInfo(newStyle, &tempStyle.stInfo, validBits, tempStyleRun.srIsGraphic)) {
			wasChanged = true;
		}
		
		if(wasChanged) {
			
			/* Save the style run info */
			oldStyleRun = tempStyleRun;
			
			if(validBits & peGraphicColorChangeValid) {
				/* Set the graphic flag if needed */
				if(validBits & peGraphicValid) {
					tempStyleRun.srIsGraphic = true;
					tempStyleRun.srIsTab = false;
					endIsTab = false;
				} else if(validBits & peColorValid) {
					tempStyleRun.srIsGraphic = false;
//!					/* Probably have to look for tabs here some day */
				}
			}
						
			/* Add the new style to the style table */
			errCode = AddStyle(&tempStyle.stInfo, (**docInfo).theStyleTable, &tempStyleIndex, tempStyleRun.srIsGraphic, ((**docInfo).printData != nil));
			tempStyleRun.srStyleIndex = tempStyleIndex;
			if(errCode != noErr) {
				break;
			}
			
			/* If part way into a style run, split the style run */
			if(curOffset != 0L) {
				/* Set the new length correctly */
				tempStyleRun.srStyleLen -= curOffset;
				/* Add the second part of the style run as new and increment the index */
				errCode = AddStyleRun(&tempStyleRun, paraInfo, ++styleRunIndex);
				if(errCode != noErr) {
					/* Delete what was added to the table on errors */
					DeleteStyle((**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
					break;
				}
				/* Change the first part of the style run to the length of whatever's unchanged */
				oldStyleRun.srStyleLen = curOffset;
				errCode = ChangeStyleRun(&oldStyleRun, paraInfo, styleRunIndex - 1L);
				if(errCode != noErr) {
					/* Delete the newly added style run on errors */
					DeleteStyleRun(paraInfo, (**docInfo).theStyleTable, styleRunIndex);
					break;
				}
				/* If the start and end paras were the same, increment the end style run too */
				if(paraIndex == endParaIndex) {
					++endStyleRunIndex;
				}
				
				/* The next time through will be from the beginning of a style run */
				curOffset = 0L;
			} else {
				/* Delete the style for this style run; it will be changed to the new style */
				errCode = DeleteStyle((**docInfo).theStyleTable, oldStyleRun.srStyleIndex);
				if(errCode != noErr) {
					DeleteStyle((**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
					break;
				}
				
				/* Change the style run to the newly created style */
				ChangeStyleRun(&tempStyleRun, paraInfo, styleRunIndex++);
			}
			
			if((paraIndex == endParaIndex) && (styleRunIndex == endStyleRunIndex + 1L)) {
				if((endRunOffset < tempStyleRun.srStyleLen) && !endIsTab) {
					errCode = AddStyle(&endStyle.stInfo, (**docInfo).theStyleTable, &tempStyleIndex, endStyle.stIsGraphic, ((**docInfo).printData != nil));
					if(errCode != noErr) {
						break;
					}
					tempStyleRun.srStyleLen -= endRunOffset;
					tempStyleRun.srIsGraphic = endStyle.stIsGraphic;
					tempStyleRun.srIsTab = false;
					Exchange(tempStyleRun.srStyleIndex, tempStyleIndex);
					errCode = AddStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
					if(errCode != noErr) {
						DeleteStyle((**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
						break;
					}
					tempStyleRun.srStyleLen = endRunOffset;
					tempStyleRun.srStyleIndex = tempStyleIndex;
					errCode = ChangeStyleRun(&tempStyleRun, paraInfo, styleRunIndex - 1L);
					if(errCode != noErr) {
						DeleteStyleRun(paraInfo, (**docInfo).theStyleTable, styleRunIndex);
						break;
					}
				}
				DoParaRecalc(docInfo, paraIndex, startOffset, endOffset, startStyleRunIndex, styleRunIndex, validBits);
			} else {
				if(styleRunIndex == CountStyleRuns(paraInfo)) {
					DoParaRecalc(docInfo, paraIndex, startOffset, endOffset, startStyleRunIndex, styleRunIndex, validBits);
					styleRunIndex = 0L;
					++paraIndex;
					if(paraIndex < (**docInfo).paraCount) {
						startStyleRunIndex = 1L;
						paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
					}
				}
			}
		} else {
			errCode = noErr;
			curOffset = 0L;
			++styleRunIndex;
			++startStyleRunIndex;
			if((paraIndex == endParaIndex) && (styleRunIndex == endStyleRunIndex + 1L)) {
				continue;
			} else if(styleRunIndex == CountStyleRuns(paraInfo)) {
				styleRunIndex = 0L;
				++paraIndex;
				if(paraIndex < (**docInfo).paraCount) {
					startStyleRunIndex = 1L;
					paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
				}
			}
		}
	} while((paraIndex < (**docInfo).paraCount) && ((paraIndex != endParaIndex) || (styleRunIndex <= endStyleRunIndex)));
	
	if((errCode == noErr) && wasChanged) {
		if((validBits & ~peLockValid) != 0) {
			ResetScrollbars(docInfo);
			DeleteIdleGraphics(docInfo, startOffset, endOffset);
			NewIdleGraphics(docInfo, startOffset, endOffset);
		}
		
		FlushDocInfoLineCache(docInfo);
		SetDirty(docInfo, startOffset, endOffset, true);
	}
	
	return errCode;
}

long ChangeStyleInfo(StyleInfoPtr sourceStyle, StyleInfoPtr destStyle, long validBits, Boolean isGraphic)
{
	long changedBits = 0L, tempBits;
	
	if((validBits & peColorValid) && (validBits & peGraphicValid)) {
		return 0L;
	}
	if((validBits & peGraphicValid) && (validBits & peGraphicColorChangeValid) && !isGraphic) {
		changedBits |= peColorValid;
		changedBits |= peGraphicValid;
	}
	if((validBits & peColorValid) && (validBits & peGraphicColorChangeValid) && isGraphic) {
		changedBits |= peColorValid;
		changedBits |= peGraphicValid;
	}
	if((validBits & peFontValid) && (destStyle->textStyle.tsFont != sourceStyle->textStyle.tsFont)) {
		destStyle->textStyle.tsFont = sourceStyle->textStyle.tsFont;
		changedBits |= peFontValid;
	}
	if((tempBits = ((destStyle->textStyle.tsFace ^ sourceStyle->textStyle.tsFace) & validBits)) != 0L) {
		destStyle->textStyle.tsFace &= ~validBits;
		destStyle->textStyle.tsFace |= (validBits & sourceStyle->textStyle.tsFace);
		changedBits |= tempBits;
	}
	if((validBits & peSizeValid) && (destStyle->textStyle.tsSize != sourceStyle->textStyle.tsSize)) {
		destStyle->textStyle.tsSize = sourceStyle->textStyle.tsSize;
		changedBits |= peSizeValid;
	}
	if((validBits & peColorValid) && ((validBits & peGraphicColorChangeValid) || !isGraphic) && ((destStyle->textStyle.tsColor.red != sourceStyle->textStyle.tsColor.red) || (destStyle->textStyle.tsColor.green != sourceStyle->textStyle.tsColor.green) || (destStyle->textStyle.tsColor.blue != sourceStyle->textStyle.tsColor.blue))) {
		destStyle->textStyle.tsColor = sourceStyle->textStyle.tsColor;
		changedBits |= peColorValid;
	}
	if((validBits & peGraphicValid) && ((validBits & peGraphicColorChangeValid) || isGraphic) && (destStyle->graphicStyle.graphicInfo != sourceStyle->graphicStyle.graphicInfo)) {
		destStyle->graphicStyle.graphicInfo = sourceStyle->graphicStyle.graphicInfo;
		changedBits |= peGraphicValid;
	}
	if((validBits & peLangValid) && (destStyle->textStyle.tsLang != sourceStyle->textStyle.tsLang)) {
		destStyle->textStyle.tsLang = sourceStyle->textStyle.tsLang;
		changedBits |= peLangValid;
	}
	if((tempBits = ((((long)(destStyle->textStyle.tsLock ^ sourceStyle->textStyle.tsLock)) << kPETELockShift) & validBits)) != 0L){
		destStyle->textStyle.tsLock &= ~((validBits >> kPETELockShift) & 0x0000000F);
		destStyle->textStyle.tsLock |= (sourceStyle->textStyle.tsLock & ((validBits >> kPETELockShift) & 0x0000000F));
		changedBits |= tempBits;
	}
	if((tempBits = ((((long)(destStyle->textStyle.tsLabel ^ sourceStyle->textStyle.tsLabel)) << kPETELabelShift) & validBits)) != 0L) {
		destStyle->textStyle.tsLabel &= ~((validBits >> kPETELabelShift) & 0x00000FFF);
		destStyle->textStyle.tsLabel |= (sourceStyle->textStyle.tsLabel & ((validBits >> kPETELabelShift) & 0x00000FFF));
		changedBits |= tempBits;
	}
	
	return changedBits;
}

OSErr FindLabelRun(DocumentInfoHandle docInfo, long offset, long *start, long *end, unsigned short label, unsigned short mask)
{
	long paraIndex, styleRunIndex, startOffset, tempOffset, styleRunLen, styleRunCount;
	ParagraphInfoHandle paraInfo;
	LongSTElement tempStyle;
	LongStyleRun tempStyleRun;
	
	if(offset >= (**docInfo).textLen) {
		return errOffsetInvalid;
	}
	
	*start = -2L;
	startOffset = offset;
	paraIndex = ParagraphIndex(docInfo, offset);
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	tempOffset = offset - ParagraphOffset(docInfo, paraIndex);
	styleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	offset -= tempOffset;
	styleRunCount = CountStyleRuns(paraInfo);
	do {
		styleRunLen = GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);

		if((tempStyle.stInfo.textStyle.tsLabel & mask) == (label & mask)) {
			if(offset <= startOffset) {
				if(*start < -1L) {
					++*start;
				} else if(styleRunIndex != 0L || paraIndex != 0L) {
					offset -= styleRunLen;
				}
				if(--styleRunIndex >= 0L || paraIndex != 0L) {
					if(styleRunIndex < 0L) {
						--paraIndex;
						paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
						styleRunCount = CountStyleRuns(paraInfo);
						styleRunIndex = styleRunCount - 1L;
					}
				} else {
					offset = 0L;
					startOffset = -1L;
					++styleRunIndex;
				}
				continue;
			} else if(*start < 0L) {
				*start = offset;
			}
		} else if(*start >= 0L) {
			break;
		} else if((offset <= startOffset) && (*start == -1L)) {
			startOffset = offset - 1L;
			offset -= styleRunLen;
		}
		
		offset += styleRunLen;
		if(++styleRunIndex >= styleRunCount) {
			if(++paraIndex < (**docInfo).paraCount) {
				paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
				styleRunCount = CountStyleRuns(paraInfo);
				styleRunIndex = 0L;
			} else {
				break;
			}
		}
	} while(true);
	
	if(*start < 0L) {
		return errEndOfDocument;
	} else {
		*end = offset;
		return noErr;
	}
}

OSErr ApplyStyleList(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEStyleListHandle styleScrap)
{
	long styleCount, styleIndex, currentLength, currentOffset;
	short validBits;
	OSErr errCode;
	StyleInfo style;
	
	styleCount = InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry);
	if(styleCount < 0L) {
		return MemError();
	}
	
	if((styleCount == 0L) || ((*styleScrap)[styleCount - 1L].psStartChar + startOffset >= endOffset)) {
		return errOffsetInvalid;
	}
	
	errCode = noErr;
	
	for(styleIndex = 0L; styleIndex < styleCount && errCode == noErr; ++styleIndex) {
		style = (*styleScrap)[styleIndex].psStyle;
		validBits = peAllValid;
		if((*styleScrap)[styleIndex].psGraphic) {
			validBits &= ~peColorValid;
		}
		
		currentOffset = startOffset + (*styleScrap)[styleIndex].psStartChar;
		if(styleIndex == styleCount - 1L) {
			currentLength = endOffset - currentOffset;
		} else {
			currentLength = (*styleScrap)[styleIndex + 1L].psStartChar - (*styleScrap)[styleIndex].psStartChar;
		}
		errCode = ChangeStyleRange(docInfo, currentOffset, currentOffset + currentLength, &style, validBits);
	}
	return errCode;
}

Boolean SameGraphic(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	if(((**docInfo).selectedGraphic == nil) || !selection->graphicHit) {
		return false;
	}
	
	if(selection->offset == (**docInfo).selStart.offset) {
		return selection->leadingEdge;
	}
	
	if(selection->offset == (**docInfo).selEnd.offset) {
		return !selection->leadingEdge;
	}
	
	return((selection->offset > (**docInfo).selStart.offset) && (selection->offset < (**docInfo).selEnd.offset));
}

void RecalcStyleHeights(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, Boolean printing)
{
	long styleIndex;
	PETEPortInfo savedPortInfo;
	FMetricRec theMetrics;
	LongSTElement tempStyle;
	
	SavePortInfo(nil, &savedPortInfo);
	for(styleIndex = NumStyles((**docInfo).theStyleTable); --styleIndex >= 0L; ) {
		GetStyle(&tempStyle, (**docInfo).theStyleTable, styleIndex);
		if(!tempStyle.stIsGraphic) {
			SetTextStyleWithDefaults(globals, docInfo, &tempStyle.stInfo.textStyle, false, printing);
			FontMetrics(&theMetrics);
			tempStyle.ascent = FixRound(theMetrics.ascent);
			tempStyle.descent = FixRound(theMetrics.descent);
			tempStyle.leading = FixRound(theMetrics.leading);
			ChangeStyle(&tempStyle, (**docInfo).theStyleTable, styleIndex);
		}
	}
	ResetPortInfo(&savedPortInfo);
}

void AddScrapStyles(LongSTTable theStyleTable, PETEStyleListHandle styleScrap)
{
	long entryCount;
	PETEStyleInfo tempStyle;
	
	entryCount = InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry);
	while(--entryCount >= 0L) {
		tempStyle = (*styleScrap)[entryCount].psStyle;
		AddStyle(&tempStyle, theStyleTable, nil, (*styleScrap)[entryCount].psGraphic, ((**(**theStyleTable).docInfo).printData != nil));
	}
}

void DeleteScrapStyles(LongSTTable theStyleTable, PETEStyleListHandle styleScrap)
{
	long entryCount, styleIndex;
	PETEStyleInfo tempStyle;
	
	entryCount = InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry);
	while(--entryCount >= 0L) {
		tempStyle = (*styleScrap)[entryCount].psStyle;
		styleIndex = FindStyleIndex(&tempStyle, theStyleTable, (*styleScrap)[entryCount].psGraphic);
		DeleteStyle(theStyleTable, styleIndex);
	}
}

void GetCurrentTextStyle(DocumentInfoHandle docInfo, MyTextStylePtr textStyle)
{
	ScriptCode scriptCode;
	long scriptCount;
	
	*textStyle = (**docInfo).curTextStyle;
	
	if(((**docInfo).flags.isActive) && ((**(**docInfo).globals).flags.hasMultiScript)) {
		scriptCode = (short)GetScriptManagerVariable(smKeyScript);
		if(scriptCode != StyleToScript(textStyle)) {
			scriptCount = (**(**docInfo).lastFont).scriptCount;
			while(--scriptCount >= 0L) {
				if((**(**docInfo).lastFont).scriptFontList[scriptCount].theScript == scriptCode) {
					textStyle->tsFont = (**(**docInfo).lastFont).scriptFontList[scriptCount].theFont;
					break;
				}
			}
			if(scriptCount < 0L) {
				textStyle->tsFont = kPETEDefaultFont;
			}
			textStyle->tsLang = (short)GetScriptVariable(scriptCode, smScriptLang);
			(**docInfo).curTextStyle = *textStyle;
		}
	}
}

OSErr GetStylesForMenu(DocumentInfoHandle docInfo, MyTextStylePtr curTextStyle, PETEStylesForMenuPtr selectionStyles)
{
#pragma unused (docInfo,curTextStyle,selectionStyles)
	return paramErr;
}