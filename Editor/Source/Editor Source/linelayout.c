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

/* Get the line layout indicated by the information in lineCache */
OSErr GetLineLayout(DocumentInfoHandle docInfo, LayoutCachePtr lineCache)
{
	LayoutCache tempLineCache;
	ParagraphInfoHandle paraInfo;
	OSErr errCode;
	LoadParams loadParam;
	long startChar, endChar;
	Byte hState;
	
	/* If the current paragraph hasn't been measured, measure it */
	errCode = CheckParagraphMeasure(docInfo, lineCache->paraIndex, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	/* Get the paragraph info and character range indicated by lineCache */
	paraInfo = (**docInfo).paraArray[lineCache->paraIndex].paraInfo;
	hState = HGetState((Handle)(**paraInfo).lineStarts);
	HNoPurge((Handle)(**paraInfo).lineStarts);
	
	startChar = (*(**paraInfo).lineStarts)[lineCache->lineIndex].lsStartChar;
	if(InlineGetHandleSize((Handle)(**paraInfo).lineStarts) > sizeof(LSElement)) {
		endChar = (*(**paraInfo).lineStarts)[lineCache->lineIndex + 1].lsStartChar;
	} else {
		endChar = startChar;
	}

	HSetState((Handle)(**paraInfo).lineStarts, hState);

	/* Load up the text for this line */
	loadParam.lpLenRequest = endChar - startChar;
	errCode = LoadText(docInfo, startChar, &loadParam, true);
	
	if(errCode != noErr) {
		return errCode;
	}

	/* Get the current line layout that is cached for this document */
	tempLineCache = (**docInfo).lineCache;
	
	/* If the requested line isn't cached, layout the requested line */
	if((tempLineCache.lineIndex != lineCache->lineIndex) ||
	   (tempLineCache.paraIndex != lineCache->paraIndex) ||
	   (tempLineCache.orderingHndl == nil) ||
	   (*tempLineCache.orderingHndl == nil)) {

		/* Empty the cache of what's there now */
		FlushLineCache(&tempLineCache);
		
		/* Set up the orderingHndl for the layout */
		lineCache->orderingHndl = tempLineCache.orderingHndl;
		
		/* Layout the requested line */
		errCode = LayoutLine(docInfo, lineCache, startChar, endChar);
		
		/* Set the document cache to this line layout */
		(**docInfo).lineCache = *lineCache;
	} else {
		/* Already have the line, so just set the return info */
		*lineCache = tempLineCache;
		errCode = noErr;
	}
	
	UnloadText(docInfo);
	return errCode;
}

/* Lay out the line of text */
OSErr LayoutLine(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, long startChar, long endChar)
{
	short numRuns;
	DirectionParam dirParam;
	OSErr errCode, tempErr;
	Boolean firstLine;
	ParagraphInfoHandle paraInfo;

	/* Get the paragraph info and character range indicated by lineCache */
	paraInfo = (**docInfo).paraArray[lineCache->paraIndex].paraInfo;
	firstLine = (lineCache->lineIndex == 0);

	/* Set up the parameters for measuring and ordering routines */
	dirParam.theStyleTable = (**docInfo).theStyleTable;
	dirParam.paraInfo = paraInfo;
	dirParam.paraOffset = ParagraphOffset(docInfo, lineCache->paraIndex);
	
	/* Figure out the length in bytes of the first and last run */
	numRuns = CalcRunLengths(startChar, endChar, &dirParam);
	
	/* Create the ordered line layout handle */
	if(lineCache->orderingHndl == nil) {
		lineCache->orderingHndl = (OrderHandle)MyNewHandle(sizeof(OrderEntry) * numRuns, &tempErr, hndlClear);
		errCode = tempErr;
	} else {
		SetHandleSize((Handle)lineCache->orderingHndl, sizeof(OrderEntry) * numRuns);
		errCode = MemError();
	}
	if(errCode != noErr)
		return errCode;

	/* If there's only one run in the line, fill in that information */
	if(numRuns == 1) {
		FillOrderEntry(lineCache->orderingHndl, &dirParam, startChar, 0, 0, false);
	} else if(numRuns != 0) {
		/* Get the order of the style runs */
		errCode = OrderRuns(lineCache->orderingHndl, &dirParam, startChar, numRuns);

		if(errCode != noErr) {
			DisposeHandle((Handle)lineCache->orderingHndl);
			return errCode;
		}
	
	}
	
	/* Fill in the widths of the style runs */
	MeasureRuns(docInfo, lineCache->paraIndex, lineCache->orderingHndl, numRuns, firstLine);
	
	/* Calculate the visible width of the line */
	VisibleWidth(lineCache, numRuns);
	
	lineCache->leftPosition = LeftPosition(paraInfo, (**docInfo).docWidth, lineCache->visibleWidth, nil, firstLine);
	
	/* Move left to draw whitespace on the left */
	lineCache->leftPosition -= lineCache->leftWhiteWidth;
	
	if(lineCache->leftPosition + lineCache->visibleWidth > (**docInfo).longLineWidth) {
		(**docInfo).longLineWidth = lineCache->leftPosition + lineCache->visibleWidth;
	}
	
	return noErr;
}

/* Routine for GetFormatOrder to determine style run direction */
pascal Boolean MyRlDirProc(short theFormat, void *dirParam)
{
	DirectionParamPtr myDirParam;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	
	/* Set up myDirParam */
	myDirParam = (DirectionParamPtr)dirParam;
	/* Get the style run given the index passed in */
	GetStyleRun(&tempStyleRun, myDirParam->paraInfo, theFormat + myDirParam->baseStyleRun);
	/* Get the style information for that style run */
	GetStyle(&tempStyle, myDirParam->theStyleTable, tempStyleRun.srStyleIndex);
	/* Return the direction based on the font */
	return (!(!(Boolean)GetScriptVariable(StyleToScript(&tempStyle.stInfo.textStyle), smScriptRight)));
}

/* Fill in the information for a particular style run, not including its width */
short FillOrderEntry(OrderHandle orderingHndl, DirectionParamPtr dirParam, long textOffset, short orderIndex, short runIndex, Boolean lastRun)
{
	LongStyleRun tempStyleRun;
	short runLen;

	/* Get the style information for this entry */
	GetStyleRun(&tempStyleRun, dirParam->paraInfo, dirParam->baseStyleRun + runIndex);

	/* Fill in the index number */
	(*orderingHndl)[orderIndex].index = runIndex;

	/* Clear out the width */
	(*orderingHndl)[orderIndex].width = 0L;
	(*orderingHndl)[orderIndex].leftWidth = 0L;
	(*orderingHndl)[orderIndex].rightWidth = 0L;
	
	/* Set the text offset in this element */
	(*orderingHndl)[orderIndex].offset = textOffset;
	
	/* Set the text offset of the beginning of the paragraph */
	(*orderingHndl)[orderIndex].paraOffset = dirParam->paraOffset;
	
	/* Set the graphic flag */
	(*orderingHndl)[orderIndex].isGraphic = tempStyleRun.srIsGraphic;
	
	/* Set the tab and whitespace flag */
	(*orderingHndl)[orderIndex].isTab = tempStyleRun.srIsTab;
	(*orderingHndl)[orderIndex].isWhitespace = tempStyleRun.srIsTab;
	
	/* Set the style index value */
	(*orderingHndl)[orderIndex].styleIndex = tempStyleRun.srStyleIndex;
	
	/* Check to see if this is the first or the last style run */
	if(runIndex == 0) {
		/* It's the first, so set the length to startLen */
		runLen = dirParam->startLen;
	} else if(lastRun) {
		/* It's the last, so set the length to endLen */
		runLen = dirParam->endLen;
	} else {
		/* It's a middle style run, so set the length to the full length */
		runLen = tempStyleRun.srStyleLen;
	}
	return (*orderingHndl)[orderIndex].len = runLen;
}

/* Return a handle containing the ordering for style runs for a particular line */
FormatOrderPtr * GetFormatOrderHandle(DirectionParamPtr dirParam, short numRuns)
{
	FormatOrderPtr *formatOrderHndl;
	OSErr errCode;
	DECLARE_UPP(MyRlDirProc,StyleRunDirection);
	
	INIT_UPP(MyRlDirProc,StyleRunDirection);
	
	/* Allocate memory for the results of GetFormatOrder */
	formatOrderHndl = (FormatOrderPtr *)MyNewHandle(sizeof(short) * numRuns, &errCode, hndlClear);
	if((formatOrderHndl != nil) && (numRuns > 1)) {
		/* Fill in handle with indexes from GetFormatOrder */
		HLock((Handle)formatOrderHndl);
		GetFormatOrder(*formatOrderHndl, 0, numRuns - 1, ((**dirParam->paraInfo).direction != leftCaret), MyRlDirProcUPP, (Ptr)dirParam);
		HUnlock((Handle)formatOrderHndl);
	}
	
	return formatOrderHndl;
}

/* Fill in the layout information for a single line of style runs */
OSErr OrderRuns(OrderHandle orderingHndl, DirectionParamPtr dirParam, long textOffset, short numRuns)
{
	FormatOrderPtr *formatOrderHndl;
	short runIndex, orderIndex, *indexEntry;

	/* Allocate memory for the results of GetFormatOrder */
	formatOrderHndl = GetFormatOrderHandle(dirParam, numRuns);
	if(formatOrderHndl == nil) {
		return MemError();
	}

	/* Go through and fill in the rest of the fields for each entry */
	for(runIndex = 0; runIndex < numRuns; ++runIndex) {
		
		/* Set dirEntry to the first entry in the array */
		indexEntry = (short *)*formatOrderHndl;
	
		/* Look for the appropriate entry in the array */
		for(orderIndex = 0; *indexEntry++ != runIndex; ++orderIndex)
			;

		textOffset += FillOrderEntry(orderingHndl, dirParam, textOffset, orderIndex, runIndex, (runIndex == numRuns - 1));
		
	}

	/* Get rid of the memory for the results of GetFormatOrder */
	DisposeHandle((Handle)formatOrderHndl);
	return noErr;
}

void FlushLineCache(LayoutCachePtr lineCache)
{
	Handle orderingHndl;

	orderingHndl = (Handle)lineCache->orderingHndl;
	lineCache->paraIndex = -1L;
	lineCache->lineIndex = -1L;
	if(orderingHndl != nil) {
		lineCache->orderingHndl = nil;
		DisposeHandle(orderingHndl);
	}
}

void FlushDocInfoLineCache(DocumentInfoHandle docInfo)
{
	LayoutCache tempLineCache;
	
	tempLineCache = (**docInfo).lineCache;
	FlushLineCache(&tempLineCache);
	(**docInfo).lineCache = tempLineCache;
}