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

/* Check to see if the given text is all whitespace */
Boolean IsWhitespace(StringPtr theText, long textLen, ScriptCode script, WhiteInfoHandle whitespaceGlobals, long *leadWS)
{
	Boolean doubleByte, whiteSpace;
	short entryNum, numEntries;
	char *table;
	StringPtr entry, textPtr;
	WhiteSpaceTablePtr wTable;
	unsigned char textChars[3];

	if(leadWS != nil) {
		*leadWS = 0L;
	}
		
	if(textLen <= 0) {
		return false;
	}
	
	GetWhitespaceGlobals(whitespaceGlobals, script);
	doubleByte = (**whitespaceGlobals).doubleByte;	
	table = &(**whitespaceGlobals).table[0];
	wTable = ((WhiteSpaceTablePtr)(*(**whitespaceGlobals).itlHandle + (**whitespaceGlobals).offset));
	numEntries = wTable->numEntries;
	whiteSpace = false;
	
	do {
		textPtr = textChars;
		*textPtr++ = 1;
		
		switch(*textPtr = *theText++) {
			case kSpaceChar :
			case kTabChar :
			case kCarriageReturnChar :
			case kPageDownChar :
			case kLineFeedChar :
			case kNullChar :
				whiteSpace = true;
				if(leadWS != nil) {
					++*leadWS;
				}
				continue;
			default :
				;
		}
		
		if(doubleByte && table[*textPtr++]) {
			*textPtr = *theText++;
			++textChars[0];
		}
		
		for(whiteSpace = false, entryNum = 0;
		    !whiteSpace && (entryNum < numEntries);
		    ++entryNum) {
			
			textPtr = textChars;
			entry = (StringPtr)wTable + wTable->offset[entryNum];

			if((*entry++ == *textPtr++) && (*entry++ == *textPtr++) &&
			   ((textChars[0] == 1) || (*entry++ == *textPtr++))) {
				whiteSpace = true;
				if(leadWS != nil) {
					*leadWS += textChars[0];
				}
			}
		}
	} while (((textLen -= textChars[0]) > 0L) && whiteSpace);
	
	return whiteSpace;
}

/* Search for the next tab in the text */
long FindNextTab(Ptr theText, long textLen, long lastTab, long *tabCount)
{
	long textOffset;
	
	/* Start the search at past the last tab run */
	textOffset = lastTab + *tabCount;
	
	/* Clear the tab count */
	*tabCount = 0L;

	/* Make sure the offset is still within the text length */
	if(textOffset < textLen) {

		while((textOffset < textLen) && (theText[textOffset] != kTabChar)) {
			++textOffset;
		}
		
		if(textOffset == textLen) {
			textOffset = -1L;
		} else {
		
		/* A tab was found */
			/* Count the number of tabs in this run */
			do {
				++*tabCount;
			} while((textOffset + *tabCount < textLen) && (theText[textOffset + *tabCount] == kTabChar));
		}
	} else {
		textOffset = LONG_MAX;
	}

	return (textOffset >= 0L) ? textOffset : textLen;
}

Boolean FindNextFF(Ptr theText, long startOffset, long endOffset, long *ffLoc)
{
	register Ptr text;
	register long start, end;
	
	for(text = theText, start = startOffset, end = endOffset; start < end; ++start) {
		if(*text++ == kPageDownChar) {
			*ffLoc = startOffset;
			return true;
		}
	}
	return false;
}