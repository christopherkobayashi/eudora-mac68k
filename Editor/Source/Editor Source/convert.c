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

OSErr TEScrapToPETEStyle(StScrpHandle teScrap, PETEStyleListHandle *styleHandle)
{
	PETEStyleEntry tempStyle;
	OSErr errCode;
	short numRuns;
	Boolean zeroStart;
	
	zeroStart = (**teScrap).scrpStyleTab[0].scrpStartChar == 0L;
	numRuns = (**teScrap).scrpNStyles + (zeroStart ? 0 : 1);
	
	if(*styleHandle == nil) {
		*styleHandle = (PETEStyleListHandle)MyNewHandle(numRuns * sizeof(PETEStyleEntry), &errCode, 0);
	} else {
		if(**styleHandle == nil) {
			ReallocateHandle((Handle)*styleHandle, numRuns * sizeof(PETEStyleEntry));
		} else {
			SetHandleSize((Handle)*styleHandle, numRuns * sizeof(PETEStyleEntry));
		}
		errCode = MemError();
	}
	
	if(errCode == noErr) {
		
		while(--numRuns >= 0) {
			if(numRuns > 0 || zeroStart) {
				*(ScrpSTElement *)&tempStyle = (**teScrap).scrpStyleTab[numRuns - (zeroStart ? 0 : 1)];
			} else {
				tempStyle.psStartChar = 0L;
				tempStyle.psStyle.textStyle.tsFont = kPETEDefaultFont;
				tempStyle.psStyle.textStyle.tsFace = 0;
				tempStyle.psStyle.textStyle.tsSize = kPETEDefaultSize;
				SetPETEDefaultColor(tempStyle.psStyle.textStyle.tsColor);
			}
				
			tempStyle.psGraphic = 0L;
			tempStyle.psStyle.textStyle.tsLang = (short)GetScriptVariable(FontToScript(tempStyle.psStyle.textStyle.tsFont), smScriptLang);
			tempStyle.psStyle.textStyle.tsLock = 0;
			tempStyle.psStyle.textStyle.tsLabel = 0;
			(**styleHandle)[numRuns] = tempStyle;
		}
	}
		
	return errCode;
}

OSErr PETEStyleToTEScrap(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleListHandle styleHandle, StScrpHandle *teScrap)
{
	PETEStyleEntry tempStyle;
	ScrpSTElement scrapEntry;
	FontInfo info;
	PETEPortInfo savedPortInfo;
	long numRuns, runIndex;
	OSErr errCode, tempErr;
	Boolean created;
	
	numRuns = InlineGetHandleSize((Handle)styleHandle) / sizeof(PETEStyleEntry);
	if((errCode = MemError()) != noErr) {
		return errCode;
	}
	
	if(*teScrap == nil) {
		created = true;
		*teScrap = (StScrpHandle)MyNewHandle(sizeof(short), &tempErr, 0);
		errCode = tempErr;
	} else {
		created = false;
		if(**teScrap == nil) {
			ReallocateHandle((Handle)*teScrap, sizeof(short));
		} else {
			SetHandleSize((Handle)*teScrap, sizeof(short));
		}
		errCode = MemError();
	}
	if(errCode != noErr) {
		return errCode;
	}
	
	(***teScrap).scrpNStyles = 0;
	
	SavePortInfo(nil, &savedPortInfo);

	for(runIndex = 0L; runIndex < numRuns && errCode == noErr; ++runIndex) {
		tempStyle = (*styleHandle)[runIndex];
		if((runIndex == 0) || !EqualPETEandTEScrap(globals, docInfo, &tempStyle, &scrapEntry)) {
			
			SetTextStyleWithDefaults(globals, docInfo, &tempStyle.psStyle.textStyle, false, false);
			GetFontInfo(&info);
			
			scrapEntry.scrpStartChar = tempStyle.psStartChar;
			scrapEntry.scrpHeight = info.ascent + info.descent + info.leading;
			scrapEntry.scrpAscent = info.ascent;
			scrapEntry.scrpFont = StyleToFont(globals, docInfo, &tempStyle.psStyle.textStyle, false);
			scrapEntry.scrpFace = tempStyle.psStyle.textStyle.tsFace;
			scrapEntry.scrpSize = StyleToFontSize(globals, docInfo, &tempStyle.psStyle.textStyle, false);
			if(tempStyle.psGraphic || IsPETEDefaultColor(tempStyle.psStyle.textStyle.tsColor)) {
				scrapEntry.scrpColor = DocOrGlobalColor(globals, docInfo);
			} else {
				scrapEntry.scrpColor = tempStyle.psStyle.textStyle.tsColor;
			}

			errCode = PtrAndHand(&scrapEntry, (Handle)*teScrap, sizeof(ScrpSTElement));
			if((errCode == noErr) && (++(***teScrap).scrpNStyles > sizeof(ScrpSTTable) / sizeof(ScrpSTElement))) {
				errCode = teScrapSizeErr;
			}
		}
	}
	
	ResetPortInfo(&savedPortInfo);

	if((errCode != noErr) && created) {
		DisposeHandle((Handle)*teScrap);
		*teScrap = nil;
	}
	return errCode;
}

Boolean EqualPETEandTEScrap(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEStyleEntry *peteScrap, ScrpSTElement *scrapEntry)
{
	RGBColor tempColor;
	
	if(peteScrap->psGraphic || IsPETEDefaultColor(peteScrap->psStyle.textStyle.tsColor)) {
		tempColor = DocOrGlobalColor(globals, docInfo);
	} else {
		tempColor = peteScrap->psStyle.textStyle.tsColor;
	}
	
	return(EqualFont(globals, docInfo, peteScrap->psStyle.textStyle.tsFont, 0, scrapEntry->scrpFont, 0) &&
	       (peteScrap->psStyle.textStyle.tsFace == scrapEntry->scrpFace) &&
	       EqualFontSize(globals, docInfo, peteScrap->psStyle.textStyle.tsSize, scrapEntry->scrpSize) &&
	       (tempColor.red == scrapEntry->scrpColor.red) &&
	       (tempColor.green == scrapEntry->scrpColor.green) &&
	       (tempColor.blue == scrapEntry->scrpColor.blue));
}

OSErr GetParaScrap(DocumentInfoHandle docInfo, long paraIndex, PETEParaScrapHandle *paraScrap, Boolean tempMem)
{
	OSErr errCode;
	long paraSize;
	short tabCount;
	
	tabCount = ABS((**(**docInfo).paraArray[paraIndex].paraInfo).tabCount);
	paraSize = tabCount * sizeof((***paraScrap).tabStops[0]) + sizeof(PETEParaScrapEntry);
	*paraScrap = (PETEParaScrapHandle)MyNewHandle(paraSize, &errCode, tempMem ? hndlTemp : 0);
	if(errCode== noErr) {
		(***paraScrap).paraLength = 0L;
		BlockMoveData(&(**(**docInfo).paraArray[paraIndex].paraInfo).startMargin,
		              &(***paraScrap).startMargin,
		              offsetof(PETEParaInfo, tabHandle) - offsetof(PETEParaInfo, startMargin));
		while(--tabCount >= 0) {
			(***paraScrap).tabStops[tabCount] = Long2Fix((**(**docInfo).paraArray[paraIndex].paraInfo).tabStops[tabCount]);
		}
	}
	return errCode;
}

OSErr GetTextStyleScrap(DocumentInfoHandle docInfo, long startOffset, long endOffset, Handle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, Boolean allParas, Boolean tempTextMem, Boolean tempStyleMem, Boolean tempParaMem,Boolean clearLock)
{
	OSErr errCode, memErr;
	PETEGraphicInfoHandle graphicInfo;
	PETEGraphicText graphicText;
	long styleIndex, styleCount, addedLen = 0L, paraOffset = 0L, paraLen, lastTextLen, textLen = 0L;
	
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
	
	if(startOffset >= endOffset) {
		return errAEImpossibleRange;
	}

	errCode = GetStyleScrap(docInfo, startOffset, endOffset, styleScrap, paraScrap, allParas, tempStyleMem, tempParaMem, clearLock);
	if(errCode != noErr) {
		return errCode;
	}
	
	if((paraScrap != nil) && (*paraScrap != nil)) {
		paraLen = (***paraScrap).paraLength;
	}
	
	if(textScrap != nil) {
		*textScrap = MyNewHandle(0L, &memErr, tempTextMem ? hndlTemp : 0);
		errCode = memErr;
	}
	
#define PARASCRAPPTR ((PETEParaScrapPtr)(((Ptr)**paraScrap) + paraOffset))
	for(styleIndex = 0L, styleCount = InlineGetHandleSize((Handle)*styleScrap) / sizeof(PETEStyleEntry); errCode == noErr && styleIndex < styleCount; ++styleIndex) {
		lastTextLen = textLen;
		(**styleScrap)[styleIndex].psStartChar += addedLen;
		if((paraScrap != nil) && (*paraScrap != nil) && (paraLen <= (**styleScrap)[styleIndex].psStartChar)) {
			paraOffset += (sizeof(PETEParaScrapEntry) + (sizeof(PARASCRAPPTR->tabStops[0]) * ABS(PARASCRAPPTR->tabCount)));
			paraLen += PARASCRAPPTR->paraLength;
		}
		if((**styleScrap)[styleIndex].psGraphic && ((graphicInfo = (**styleScrap)[styleIndex].psStyle.graphicStyle.graphicInfo) != nil)) {
			if((**graphicInfo).cloneOnlyText) {
				(**styleScrap)[styleIndex].psGraphic = false;
				SetPETEDefaultColor((**styleScrap)[styleIndex].psStyle.textStyle.tsColor);
			}
			if((**graphicInfo).itemProc != nil) {
				if(!(**graphicInfo).cloneOnlyText) {
					errCode = CallGraphic(docInfo, graphicInfo, kPETEOffsetUnknown, peGraphicClone, &graphicInfo);
					if(errCode != noErr) {
						break;
					}
					(**styleScrap)[styleIndex].psStyle.graphicStyle.graphicInfo = graphicInfo;
				}
				if((**graphicInfo).cloneReplaceText) {
					errCode = CallGraphic(docInfo, graphicInfo, startOffset + (**styleScrap)[styleIndex].psStartChar, peGraphicNewText, &graphicText);
					if(errCode == noErr) {
						if((paraScrap != nil) && (*paraScrap != nil)) {
							PARASCRAPPTR->paraLength += graphicText.len;
							paraLen += graphicText.len;
						}
						addedLen += graphicText.len;
					}
					textLen += graphicText.len;
					if(textScrap != nil) {
						SetHandleSize(*textScrap, textLen);
						errCode = MemError();
						if(errCode == noErr) {
							BlockMoveData(graphicText.handle ? *(Handle)graphicText.text : graphicText.text, **textScrap + lastTextLen, textLen - lastTextLen);
						}
					}
				} else {
					goto DoAddText;
				}
			} else {
				if(!(**graphicInfo).cloneOnlyText) {
					++(**(PictGraphicInfoHandle)graphicInfo).counter;
				}
				goto DoAddText;
			}
		} else {
		DoAddText :
			textLen += (styleIndex + 1L == styleCount ? endOffset : startOffset + (**styleScrap)[styleIndex + 1L].psStartChar) - (startOffset + (**styleScrap)[styleIndex].psStartChar);
			if(textScrap != nil) {
				SetHandleSize(*textScrap, textLen);
				errCode = MemError();
				if(errCode == noErr) {
					errCode = LoadTextIntoHandle(docInfo, startOffset + (**styleScrap)[styleIndex].psStartChar, startOffset + (**styleScrap)[styleIndex].psStartChar + (textLen - lastTextLen), *textScrap, lastTextLen);
				}
			}
		}
	}
#undef PARASCRAPPTR
	if(errCode != noErr) {
		DisposeScrapGraphics(*styleScrap, 0L, styleIndex - 1L, false);

		if(textScrap != nil) {
			DisposeHandle(*textScrap);
		}
		if(paraScrap != nil) {
			DisposeHandle((Handle)*paraScrap);
		}
		DisposeHandle((Handle)styleScrap);
	}
	return errCode;
}

OSErr GetStyleScrap(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, Boolean allParas, Boolean tempStyleMem, Boolean tempParaMem, Boolean clearLock)
{
	long styleRunIndex, endStyleRunIndex, paraIndex, endParaIndex, curOffset, endRunOffset, paraScrapLen, nextStartChar, lastParaStartChar;
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	PETEStyleEntry scrapEntry;
	OSErr errCode, tempErr;
	PETEStyleListHandle newStyleScrap;
	PETEParaScrapPtr tempScrapPtr;
	PETEParaScrapHandle newParaScrap;

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
	
	paraIndex = ParagraphIndex(docInfo, startOffset);

	if((startOffset == endOffset) && (paraScrap != nil) && allParas) {
		*styleScrap = nil;
		return GetParaScrap(docInfo, paraIndex, paraScrap, tempParaMem);
	}
	
	if(startOffset >= endOffset) {
		return errAEImpossibleRange;
	}
	
	newParaScrap = nil;
	
	newStyleScrap = (PETEStyleListHandle)MyNewHandle(0L, &tempErr, tempStyleMem ? hndlTemp : 0);
	if((errCode = tempErr) != noErr) {
		goto DoReturn;
	}
	
	lastParaStartChar = nextStartChar = 0L;

	endParaIndex = ParagraphIndex(docInfo, endOffset - 1L);

	curOffset = startOffset - ParagraphOffset(docInfo, paraIndex);
	endRunOffset = endOffset - ParagraphOffset(docInfo, endParaIndex) - 1L;

	endStyleRunIndex = StyleRunIndex((**docInfo).paraArray[endParaIndex].paraInfo, &endRunOffset);
	
	++endRunOffset;
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	
	styleRunIndex = StyleRunIndex(paraInfo, &curOffset);
	
	do {
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		
		scrapEntry.psStartChar = nextStartChar;
		scrapEntry.psGraphic = tempStyleRun.srIsGraphic;
		scrapEntry.psStyle = tempStyle.stInfo;
		scrapEntry.psStyle.textStyle.tsLabel &= (**(**docInfo).globals).labelMask;
		if (clearLock) scrapEntry.psStyle.textStyle.tsLock = peNoLock;

		if(scrapEntry.psGraphic) {
			scrapEntry.psStyle.graphicStyle.graphicInfo = tempStyle.stInfo.graphicStyle.graphicInfo;
		}
		
		if((paraIndex == endParaIndex) && (styleRunIndex == endStyleRunIndex)) {
			nextStartChar += endRunOffset;
		} else {
			nextStartChar += tempStyleRun.srStyleLen;
		}
		nextStartChar -= curOffset;
		curOffset = 0L;
		
		errCode = PtrAndHand(&scrapEntry, (Handle)newStyleScrap, sizeof(PETEStyleEntry));
		if(errCode != noErr) {
			break;
		}
		
		++styleRunIndex;
		
		if(allParas && (paraScrap != nil) && (paraIndex == endParaIndex) && (styleRunIndex > endStyleRunIndex)) {
			goto AddParaInfo;
		}
		
		if(styleRunIndex == CountStyleRuns(paraInfo)) {
			if((paraScrap != nil) && ((paraIndex != endParaIndex) || ((allParas || (paraIndex != (**docInfo).paraCount - 1L)) && (endRunOffset == tempStyleRun.srStyleLen)))) {
		AddParaInfo :
				if(newParaScrap == nil) {
					newParaScrap = (PETEParaScrapHandle)MyNewHandle(0L, &tempErr, tempParaMem ? hndlTemp : 0);
					paraScrapLen = tempErr;
				} else {
					paraScrapLen = InlineGetHandleSize((Handle)newParaScrap);
				}
				if(paraScrapLen < 0L) {
					errCode = paraScrapLen;
				} else {
					SetHandleSize((Handle)newParaScrap, paraScrapLen + sizeof(PETEParaScrapEntry) + (ABS((**paraInfo).tabCount) * sizeof((**newParaScrap).tabStops[0])));
					errCode = MemError();
				}
				
				if(errCode == noErr) {
					short tabIndex;
					
					tempScrapPtr = (PETEParaScrapPtr)(*(Handle)newParaScrap + paraScrapLen);
					tempScrapPtr->paraLength = nextStartChar - lastParaStartChar;
					BlockMoveData(&(**paraInfo).startMargin, &tempScrapPtr->startMargin, (offsetof(ParagraphInfo, tabStops) - offsetof(ParagraphInfo, startMargin)));
					for(tabIndex = ABS((**paraInfo).tabCount); --tabIndex >= 0; ) {
						tempScrapPtr->tabStops[tabIndex] = Long2Fix((**paraInfo).tabStops[tabIndex]);
					} 
					lastParaStartChar = nextStartChar;
				}
			}
			
			if(paraIndex != endParaIndex) {
				styleRunIndex = 0L;
				++paraIndex;
				paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
			}
		}
				
	} while((paraIndex != endParaIndex) || (styleRunIndex <= endStyleRunIndex));
	
DoReturn :
	if(errCode != noErr) {
		if(newParaScrap != nil) {
			DisposeHandle((Handle)newParaScrap);
		}
		if(newStyleScrap != nil) {
			DisposeHandle((Handle)newStyleScrap);
		}
	} else {
		*styleScrap = newStyleScrap;
		if(paraScrap != nil) {
			*paraScrap = newParaScrap;
		}
	}
	
	return errCode;
}

OSErr MakePlainStyles(PETEStyleListHandle styleScrap, PETEStyleListHandle *newStyleScrap, Boolean plainTextOnly, Boolean textOnly, Boolean tempMemPref)
{
	long styleIndex, numStyles;
	PETEStyleEntry tempStyle, tempStyle1;
	OSErr errCode, tempErr;
	
	numStyles = InlineGetHandleSize((Handle)styleScrap);
	if(numStyles < 0L) {
		return numStyles;
	}
	numStyles /= sizeof(PETEStyleEntry);
	
	for(styleIndex = 0L; styleIndex < numStyles; ++styleIndex) {
		if(textOnly && (*styleScrap)[styleIndex].psGraphic) {
			break;
		}
		if(plainTextOnly) {
			if(((*styleScrap)[styleIndex].psStyle.textStyle.tsFont != kPETEDefaultFont) ||
			   ((*styleScrap)[styleIndex].psStyle.textStyle.tsFace != 0) ||
			   (((*styleScrap)[styleIndex].psStyle.textStyle.tsSize != kPETEDefaultSize) &&
			    ((*styleScrap)[styleIndex].psStyle.textStyle.tsSize != kPETERelativeSizeBase))) {
				break;
			}
			if(!(*styleScrap)[styleIndex].psGraphic) {
				if(!IsPETEDefaultColor((*styleScrap)[styleIndex].psStyle.textStyle.tsColor)) {
					break;
				}
			}
		}
	}
	
	if(styleIndex >= numStyles) {
		*newStyleScrap = nil;
		return noErr;
	}
	
	*newStyleScrap = (PETEStyleListHandle)MyNewHandle(0L, &tempErr, tempMemPref ? hndlTemp : 0);
	errCode = tempErr;
	
	for(tempStyle1.psGraphic = -1L, styleIndex = 0L;
	    styleIndex < numStyles && errCode == noErr;
	    tempStyle1 = tempStyle, ++styleIndex) {
		tempStyle = (*styleScrap)[styleIndex];
		tempStyle.psStyle.textStyle.tsLabel = 0;
		if(plainTextOnly) {
			tempStyle.psStyle.textStyle.tsFont = kPETEDefaultFont;
			tempStyle.psStyle.textStyle.tsFace = 0;
			tempStyle.psStyle.textStyle.filler = 0;
			tempStyle.psStyle.textStyle.tsSize = kPETEDefaultSize;
			if(tempStyle.psGraphic) {
				tempStyle.psStyle.graphicStyle.filler0 = 0;
			} else {
				SetPETEDefaultColor(tempStyle.psStyle.textStyle.tsColor);
			}
		}
		if(textOnly && tempStyle.psGraphic) {
			tempStyle.psGraphic = false;
			tempStyle.psStyle.graphicStyle.graphicInfo = nil;
			tempStyle.psStyle.graphicStyle.filler0 = 0;
		}

		if((tempStyle.psGraphic == tempStyle1.psGraphic) &&
		   (tempStyle.psStyle.textStyle.tsFont == tempStyle1.psStyle.textStyle.tsFont) &&
		   (tempStyle.psStyle.textStyle.tsFace == tempStyle1.psStyle.textStyle.tsFace) &&
		   (tempStyle.psStyle.textStyle.tsSize == tempStyle1.psStyle.textStyle.tsSize) &&
		   (tempStyle.psStyle.textStyle.tsColor.red == tempStyle1.psStyle.textStyle.tsColor.red) &&
		   (tempStyle.psStyle.textStyle.tsColor.green == tempStyle1.psStyle.textStyle.tsColor.green) &&
		   (tempStyle.psStyle.textStyle.tsColor.blue == tempStyle1.psStyle.textStyle.tsColor.blue) &&
		   (tempStyle.psStyle.textStyle.tsLang == tempStyle1.psStyle.textStyle.tsLang) &&
		   (tempStyle.psStyle.textStyle.tsLock == tempStyle1.psStyle.textStyle.tsLock) &&
		   (tempStyle.psStyle.textStyle.tsLabel == tempStyle1.psStyle.textStyle.tsLabel) &&
		   (tempStyle.psStyle.graphicStyle.graphicInfo == tempStyle1.psStyle.graphicStyle.graphicInfo)) {
			continue;
		}
		
		errCode = PtrAndHand(&tempStyle, (Handle)*newStyleScrap, sizeof(PETEStyleEntry));
	}
	return errCode;
}

OSErr GetDebugStyleScrap(DocumentInfoHandle docInfo, long paraIndex, PETEStyleListHandle *styleHandle)
{
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	PETEStyleEntry scrapEntry;
	OSErr errCode, tempErr;
	PETEStyleListHandle newStyleScrap;
	long styleRunIndex, nextStartChar = 0L;
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	newStyleScrap = (PETEStyleListHandle)MyNewHandle(0L, &tempErr, hndlTemp);
	errCode = tempErr;
	
	for(styleRunIndex = 0; errCode == noErr; ++styleRunIndex) {
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		scrapEntry.psStartChar = nextStartChar;
		scrapEntry.psGraphic = tempStyleRun.srIsGraphic;
		scrapEntry.psStyle = tempStyle.stInfo;

		if(scrapEntry.psGraphic) {
			scrapEntry.psStyle.graphicStyle.graphicInfo = tempStyle.stInfo.graphicStyle.graphicInfo;
		}
		
		errCode = PtrAndHand(&scrapEntry, (Handle)newStyleScrap, sizeof(PETEStyleEntry));

		if(tempStyleRun.srStyleLen < 0L) break;
		
		nextStartChar += tempStyleRun.srStyleLen;
	}
	
	if(errCode == noErr) *styleHandle = newStyleScrap;
	return errCode;
}

OSErr StyleToFontAndString();
OSErr FontAndStringToStyle();