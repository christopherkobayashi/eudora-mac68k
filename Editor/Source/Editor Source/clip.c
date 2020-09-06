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

void SetupScrapID(PETEGlobalsHandle globals);
Boolean HasScrapChanged(PETEGlobalsHandle globals);

OSErr CopySelectionToClip(DocumentInfoHandle docInfo, Boolean plainText)
{
	OSErr errCode, tempMemErr;
	StringHandle clipText;
	long startOffset, endOffset;
	PETEStyleListHandle styleScrap, tempStyleScrap;
	StScrpHandle teScrap;
	PETEParaScrapHandle paraScrap;
	Boolean hasTEStyles;

	startOffset = (**docInfo).selStart.offset;
	endOffset = (**docInfo).selEnd.offset;
	if(startOffset == endOffset) {
		return noErr;
	}
	
	errCode = GetTextStyleScrap(docInfo, startOffset, endOffset, (Handle *)&clipText, &styleScrap, &paraScrap, false, true, true, true, true);
	if(errCode != noErr) {
		return errCode;
	}
	
	ZeroScrap();
	DisposeScrapGraphics((**(**docInfo).globals).clip.styleScrap, 0L, -1L, false);
	DisposeHandle((Handle)(**(**docInfo).globals).clip.styleScrap);
	(**(**docInfo).globals).clip.styleScrap = nil;
	
	HLock((Handle)clipText);
	errCode = PutScrap(endOffset - startOffset, kTextScrap, *clipText);
	DisposeHandle((Handle)clipText);

	if((errCode == noErr) && (paraScrap != nil)) {
		HLock((Handle)paraScrap);
		errCode = PutScrap(InlineGetHandleSize((Handle)paraScrap), kPETEParaScrap, *paraScrap);
	}
	DisposeHandle((Handle)paraScrap);

	if(errCode == noErr) {
		if(plainText) {
			errCode = MakePlainStyles(styleScrap, &tempStyleScrap, true, false, true);
			if((errCode == noErr) && (tempStyleScrap != nil)) {
				DisposeHandle((Handle)styleScrap);
				styleScrap = tempStyleScrap;
			}
		}
		tempMemErr = errCode;
		if(errCode == noErr) {
			if(!(!(hasTEStyles = HasTEStyles(docInfo, startOffset, endOffset, plainText)))) {
				teScrap = (StScrpHandle)MyNewHandle(sizeof(short), &tempMemErr, hndlTemp);
				if((errCode = tempMemErr) == noErr) {
					errCode = PETEStyleToTEScrap(nil, docInfo, styleScrap, &teScrap);
				}
			} else {
				tempMemErr = noTypeErr;
			}
		}
		if((errCode == noErr) && hasTEStyles) {
			HLock((Handle)teScrap);
			errCode = PutScrap(InlineGetHandleSize((Handle)teScrap), kStyleScrap, *teScrap);
		}
		if(tempMemErr == noErr) {
			DisposeHandle((Handle)teScrap);
		}
	}
	
	if(errCode == noErr) {
		SetupScrapID((**docInfo).globals);
		(**(**docInfo).globals).clip.styleScrap = styleScrap;
	} else {
		DisposeScrapGraphics(styleScrap, 0L, -1L, false);
		DisposeHandle((Handle)styleScrap);
	}
	return errCode;
}

OSErr PasteText(DocumentInfoHandle docInfo, long textOffset, StringHandle textScrap, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, Boolean plainText, Boolean dispose)
{
	OSErr errCode, memErrCode;
	long textLen, insertOffset, paraScrapSize, paraIndex, paraOffset, lastParaIndex;
	PETEParaInfo paraInfo;
	PETEStyleListHandle convertedStyles;
	Boolean textOnly, hadLock, turnRecalcOn = false, emptyPara;
	
	if(textOffset < 0L) {
		insertOffset = (**docInfo).selStart.offset;
		paraIndex = (**docInfo).selStart.paraIndex;
		lastParaIndex = (**docInfo).selEnd.paraIndex;
	} else {
		insertOffset = textOffset;
		lastParaIndex = paraIndex = ParagraphIndex(docInfo, insertOffset);
	}
	
	emptyPara = ((**docInfo).paraArray[lastParaIndex].paraLength == 0L);
	
	if(!plainText) {
		plainText = (((**docInfo).flags.noStyledPaste) || !(!((**(**docInfo).paraArray[paraIndex].paraInfo).paraFlags & pePlainTextOnly)));
	}
	textOnly = !(!((**(**docInfo).paraArray[paraIndex].paraInfo).paraFlags & peTextOnly));
	if((styleScrap != nil) && (plainText || textOnly)) {
		errCode = MakePlainStyles(styleScrap, &convertedStyles, plainText, textOnly, true);
		if(errCode != noErr) {
			goto DoDispose;
		}
	} else {
		convertedStyles = nil;
	}
	
	hadLock = ((textOffset >= 0L) && !((**docInfo).flags.ignoreModLock));
	if(hadLock) {
		(**docInfo).flags.ignoreModLock = true;
	}
	
	if((**docInfo).flags.clearAllReturns) {
		StringPtr textPtr;
		
		textLen = InlineGetHandleSize((Handle)textScrap);
		textPtr = *textScrap + textLen;
		while(--textLen >= 0L) {
			if((*--textPtr == kCarriageReturnChar) || (*textPtr == kLineFeedChar) || (*textPtr == kPageDownChar)) {
				*textPtr = kSpaceChar;
			}
		}
	}
	
	if((paraScrap != nil) && !(**docInfo).flags.recalcOff) {
		(**docInfo).flags.recalcOff = turnRecalcOn = true;
	}
	
	{
		textLen = InlineGetHandleSize((Handle)textScrap);
		errCode = InsertText(docInfo, textOffset, (Ptr)textScrap, textLen, 0L, convertedStyles != nil ? convertedStyles : styleScrap, true);
	}
	
	if(hadLock) {
		(**docInfo).flags.ignoreModLock = false;
	}

	if((errCode == noErr) && (paraScrap != nil)) {
		paraScrapSize = InlineGetHandleSize((Handle)paraScrap);
		if(paraScrapSize < 0L) {
			errCode = paraScrapSize;
		} else {
			if(!((**(**docInfo).paraArray[paraIndex].paraInfo).paraFlags & peNoParaPaste) && !((**docInfo).flags.clearAllReturns)) {
				paraInfo.tabHandle = (short **)MyNewHandle(0L, &memErrCode, hndlTemp);
				if(memErrCode != noErr) {
					errCode = memErrCode;
				}
				
#define PARASCRAPPTR ((PETEParaScrapPtr)(((Ptr)*paraScrap) + paraOffset))
				
				for(paraOffset = 0L;
				    errCode == noErr && paraOffset < paraScrapSize;
				    ++paraIndex, paraOffset += (sizeof(PETEParaScrapEntry) + (sizeof(PARASCRAPPTR->tabStops[0]) * ABS(PARASCRAPPTR->tabCount)))) {
		
					insertOffset += PARASCRAPPTR->paraLength;
					if(insertOffset > (**docInfo).textLen) {
						errCode = badSectionErr;
					} else {
						char aChar;
						
						GetText(docInfo, insertOffset - 1L, insertOffset, &aChar, 1, nil);
						if((insertOffset != (**docInfo).textLen) || (emptyPara && (aChar == kCarriageReturnChar)) || (paraOffset + (sizeof(PETEParaScrapEntry) + (sizeof(PARASCRAPPTR->tabStops[0]) * ABS(PARASCRAPPTR->tabCount))) < paraScrapSize)) {
							errCode = InsertParagraphBreak(docInfo, insertOffset);
						}
					}
					if(errCode == noErr) {
						SetHandleSize((Handle)paraInfo.tabHandle, sizeof(**paraInfo.tabHandle) * ABS(PARASCRAPPTR->tabCount));
						if((errCode = MemError()) == noErr) {
							short tabIndex;
							for(tabIndex = ABS(PARASCRAPPTR->tabCount); --tabIndex >= 0; ) {
								(*paraInfo.tabHandle)[tabIndex] = FixRound(PARASCRAPPTR->tabStops[tabIndex]);
							}
							BlockMoveData(&PARASCRAPPTR->startMargin, &paraInfo.startMargin, offsetof(PETEParaInfo, tabHandle) - offsetof(PETEParaInfo, startMargin));
							errCode = SetParagraphInfo(docInfo, paraIndex, &paraInfo, ((**docInfo).flags.noStyledPaste) ? (peDirectionValid | peFlagsValid | peTabsValid | peQuoteLevelValid) : peAllParaValid);
						}
					}
				}
				DisposeHandle((Handle)paraInfo.tabHandle);

#undef PARASCRAPPTR
				
			}
		}
	}
	
	if(turnRecalcOn) {
		SetRecalcState(docInfo, true);
	}
	
DoDispose :
	DisposeHandle((Handle)convertedStyles);
	if(dispose) {
		DisposeHandle((Handle)textScrap);
		DisposeHandle((Handle)paraScrap);
		if(errCode != noErr) {
			DisposeScrapGraphics(styleScrap, 0L, -1L, false);
		}
		DisposeHandle((Handle)styleScrap);
	}

	return errCode;
}

OSErr GetClipContents(PETEGlobalsHandle globals, StringHandle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap)
{
	long scrapCode, tempOffset;
	OSErr errCode;
	Handle tempHandle;
	PicHandle thePic;
	PETEStyleEntry pictureScrap;
	
	*styleScrap = nil;
	*paraScrap = nil;
	*textScrap = (StringHandle)MyNewHandle(1L, &errCode, hndlTemp);
	if(errCode != noErr) {
		return errCode;
	} else {
		***textScrap = kSpaceChar;
	}

	scrapCode = GetScrap((Handle)*textScrap, kTextScrap, &tempOffset);
	
	if(scrapCode == noTypeErr) {
		if(HasScrapChanged(globals) || ((**globals).clip.styleScrap == nil)) {
			tempHandle = MyNewHandle(0L, &errCode, 0);
			if((scrapCode = errCode) == noErr) {
				scrapCode = GetScrap(tempHandle, kPictureScrap, &tempOffset);
				if(scrapCode > 0L) {
					thePic = (PicHandle)tempHandle;
					tempHandle = MyNewHandle(sizeof(PictGraphicInfo), &errCode, hndlClear);
					if((scrapCode = errCode) == noErr) {
						(**(PictGraphicInfoHandle)tempHandle).gi.width = RectWidth(&(**thePic).picFrame);
						(**(PictGraphicInfoHandle)tempHandle).gi.height = RectHeight(&(**thePic).picFrame) + 1;
						(**(PictGraphicInfoHandle)tempHandle).pict = thePic;
						(**(PictGraphicInfoHandle)tempHandle).counter = 1L;
						pictureScrap.psStartChar = 0L;
						pictureScrap.psGraphic = true;
						pictureScrap.psStyle.graphicStyle.tsFont = kPETEDefaultFont;
						pictureScrap.psStyle.graphicStyle.tsFace = 0;
						pictureScrap.psStyle.graphicStyle.filler = 0;
						pictureScrap.psStyle.graphicStyle.tsSize = kPETEDefaultSize;
						pictureScrap.psStyle.graphicStyle.graphicInfo = (PETEGraphicInfoHandle)tempHandle;
						pictureScrap.psStyle.graphicStyle.tsLang = systemCurLang;
						pictureScrap.psStyle.graphicStyle.tsLock = 0;
						pictureScrap.psStyle.graphicStyle.tsLabel = 0;
						scrapCode = MyPtrToHand(&pictureScrap, &tempHandle, sizeof(PETEStyleEntry), hndlTemp);
						if(scrapCode < 0L) {
							DisposeHandle((Handle)thePic);
						}
					}
				}
				if(scrapCode < 0L) {
					DisposeHandle(tempHandle);
				} else {
					DisposeScrapGraphics((**globals).clip.styleScrap, 0L, -1L, false);
					DisposeHandle((Handle)(**globals).clip.styleScrap);
					SetupScrapID(globals);
					(**globals).clip.styleScrap = (PETEStyleListHandle)tempHandle;
				}
			}
		} else {
			scrapCode = 0L;
		}
	}
	
	if(scrapCode < 0L) {
		DisposeHandle((Handle)*textScrap);
		return scrapCode;
	}
	
	scrapCode = 0L;
	if(!HasScrapChanged(globals)) {
		tempHandle = (Handle)(**globals).clip.styleScrap;
		if(tempHandle != nil) {
			scrapCode = MyHandToHand(&tempHandle, hndlTemp);
			if(scrapCode == noErr) {
				scrapCode = CloneScrapGraphics((PETEStyleListHandle)tempHandle);
				if(scrapCode != noErr) {
					DisposeHandle(tempHandle);
				} else {
					*styleScrap = (PETEStyleListHandle)tempHandle;
				}
			}
		}
	} else {
		DisposeScrapGraphics((**globals).clip.styleScrap, 0L, -1L, false);
		DisposeHandle((Handle)(**globals).clip.styleScrap);
		(**globals).clip.styleScrap = nil;
	}
	
	if((scrapCode == noErr) && (*styleScrap == nil)) {
		tempHandle = MyNewHandle(0L, &errCode, hndlTemp);
		if((scrapCode = errCode) == noErr) {
			scrapCode = GetScrap(tempHandle, kStyleScrap, &tempOffset);
			if(scrapCode > 0L) {
				scrapCode = TEScrapToPETEStyle((StScrpHandle)tempHandle, (PETEStyleListHandle *)&tempHandle);
			}
			if(scrapCode < 0L) {
				DisposeHandle(tempHandle);
			} else {
				*styleScrap = (PETEStyleListHandle)tempHandle;
			}
		}
	}
		
	if((scrapCode < 0L) && (scrapCode != noTypeErr)) {
		DisposeHandle((Handle)*textScrap);
		return scrapCode;
	}
	
	tempHandle = MyNewHandle(0L, &errCode, hndlTemp);
	if((scrapCode = errCode) == noErr) {
		scrapCode = GetScrap(tempHandle, kPETEParaScrap, &tempOffset);
		if(scrapCode < 0L) {
			DisposeHandle(tempHandle);
		} else {
			*paraScrap = (PETEParaScrapHandle)tempHandle;
		}
	}
	
	if((scrapCode < 0L) && (scrapCode != noTypeErr)) {
		if(*styleScrap != nil) {
			DisposeScrapGraphics(*styleScrap, 0L, -1L, false);
			DisposeHandle((Handle)*styleScrap);
		}
		DisposeHandle((Handle)*textScrap);
		return scrapCode;
	} else {
		return noErr;
	}
}

OSErr AddIntelligentPasteSpace(DocumentInfoHandle docInfo, SelDataPtr selection, StringHandle textScrap, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, short *added)
{
	unsigned char spaceChar = kSpaceChar;
	OSErr errCode;
	long tempLong, paraOffset, tempOffset, textLen;
	PETEParaScrapPtr curPara, endPara;
	SelData tempSelection, testSelection;
	short charType;
	Boolean endIsSpace;
	
	if(added != nil) {
		*added = 0;
	}
	
	errCode = noErr;
	
	if(!((**(**docInfo).globals).flags.noIntelligentEdit)) {

		if(selection == nil) {
			tempSelection = (**docInfo).selStart;
			selection = &tempSelection;
		}
		
		if((**(**docInfo).globals).intelligentPasteCallback != nil) {
			errCode = CallPETEIntelligentPasteProc((**(**docInfo).globals).intelligentPasteCallback, docInfo, selection->offset, textScrap, styleScrap, paraScrap, added);
			
			if(errCode != handlerNotFoundErr) {
				return errCode;
			}
			errCode = noErr;
		}
		
		paraOffset = ParagraphOffset(docInfo, selection->paraIndex);
		
		testSelection = *selection;
		switch(**textScrap) {
			case kSpaceChar :
			case kTabChar :
			case kCarriageReturnChar :
			case kPageDownChar :
			case kLineFeedChar :
			case kNullChar :
				endIsSpace = true;
				break;
			default :
				endIsSpace = false;
		}
		if((selection->offset > paraOffset) && !endIsSpace) {
			testSelection.leadingEdge = false;
			errCode = GetWordOffset(docInfo, &testSelection, nil, nil, &tempOffset, nil, &charType);
			if(selection->offset == tempOffset) {
				if((charType & kNoIntellCPFlag) || (((charType & smcTypeMask) == smCharPunct) && ((charType & smcClassMask) > smPunctNumber))) {
					goto CheckForWordAfter;
				}
				Munger((Handle)textScrap, 0L, nil, 0, &spaceChar, 1L);
				if((errCode = MemError()) == noErr) {
					if(added != nil) {
						*added = -1;
					}
					if(styleScrap != nil) {
						for(tempLong = InlineGetHandleSize((Handle)styleScrap) / sizeof(PETEStyleEntry); --tempLong > 0L; ) {
							++(*styleScrap)[tempLong].psStartChar;
						}
					}
					if(paraScrap != nil) {
						++(**paraScrap).paraLength;
					}
				}
			}
		} else {
		CheckForWordAfter :
			textLen = InlineGetHandleSize((Handle)textScrap);
			switch((*textScrap)[textLen - 1L]) {
				case kSpaceChar :
				case kTabChar :
				case kCarriageReturnChar :
				case kPageDownChar :
				case kLineFeedChar :
				case kNullChar :
					endIsSpace = true;
					break;
				default :
					endIsSpace = false;
			}
			if(!endIsSpace && (selection->offset < paraOffset + (**docInfo).paraArray[selection->paraIndex].paraLength)) {
				
				testSelection.leadingEdge = true;
				errCode = GetWordOffset(docInfo, &testSelection, nil, &tempOffset, nil, nil, &charType);
				if((errCode == noErr) && (selection->offset <= tempOffset) && !((charType & kNoIntellCPFlag) || (((charType & smcTypeMask) == smCharPunct) && ((charType & smcClassMask) > smPunctNumber)))) {
					errCode = PtrAndHand(&spaceChar, (Handle)textScrap, 1L);
					if(errCode == noErr) {
						if(added != nil) {
							*added = 1;
						}
						if(paraScrap != nil) {
							tempLong = InlineGetHandleSize((Handle)paraScrap);
							curPara = *paraScrap;
							endPara = (PETEParaScrapPtr)(tempLong + (long)curPara);
							for(tempLong = 0L; textLen -= curPara->paraLength, tempLong + (long)curPara < (long)endPara;) {
								tempLong = sizeof(PETEParaScrapEntry) + (sizeof(curPara->tabStops[0]) * ABS(curPara->tabCount));
								curPara = (PETEParaScrapPtr)(tempLong + (long)curPara);
							}
							if(textLen == 0) {
								++curPara->paraLength;
							}
						}
					}
				}
			}
		}
	}
	return errCode;
}

OSErr GetIntelligentCutBoundaries(DocumentInfoHandle docInfo, long *start, long *end)
{
	SelData selection;
	long paraOffset, styleRunOffset;
	OSErr errCode;
	LongStyleRun theStyleRun;
	MyTextStyle textStyle;
	
	*start = (**docInfo).selStart.offset;
	*end = (**docInfo).selEnd.offset;
	if((*start == *end) || ((**(**docInfo).globals).flags.noIntelligentEdit)) {
		return noErr;
	}
	
	if((**(**docInfo).globals).intelligentCutCallback != nil) {
		errCode = CallPETEIntelligentCutProc((**(**docInfo).globals).intelligentCutCallback, docInfo, start, end);
		
		if(errCode != handlerNotFoundErr) {
			return errCode;
		}
	}
	
	if(((*(**docInfo).theText)[*start] == kSpaceChar) || ((*(**docInfo).theText)[*end - 1L] == kSpaceChar)) {
		return noErr;
	} else if((*start != 0L) && ((*(**docInfo).theText)[*start - 1L] == kSpaceChar) && ((*(**docInfo).theText)[*end - 1L] != kSpaceChar)) {
		--*start;
		PinSelectionLock(docInfo, (**docInfo).selStart.offset, start);
		return noErr;
	} else if((*end < (**docInfo).textLen) && ((*(**docInfo).theText)[*end - 1L] != kSpaceChar) && ((*(**docInfo).theText)[*end] == kSpaceChar)) {
		++*end;
		PinSelectionLock(docInfo, (**docInfo).selEnd.offset, end);
		return noErr;
	}
	return noErr;
	
	selection = (**docInfo).selStart;
	selection.leadingEdge = true;
	errCode = GetSelectionStyle(docInfo, &selection, &theStyleRun, &styleRunOffset, &textStyle);
	if(errCode != noErr) {
		return errCode;
	}
	
	errCode = GetWordOffset(docInfo, &selection, nil, start, nil, nil, nil);
	if(*start < (**docInfo).selStart.offset) {
		*start = (**docInfo).selStart.offset;
	}
	
	paraOffset = ParagraphOffset(docInfo, selection.paraIndex);
	if(selection.offset != 0L) {
		if(!selection.leadingEdge) {
			--selection.offset;
		} else {
			selection.leadingEdge = false;
		}
		if(selection.offset <= paraOffset) {
			--selection.paraIndex;
			selection.lastLine = true;
		}
	}
	
	selection = (**docInfo).selEnd;
	paraOffset = ParagraphOffset(docInfo, selection.paraIndex);
	if(selection.offset != (**docInfo).textLen) {
		if((selection.leadingEdge) && (selection.offset < paraOffset + (**docInfo).paraArray[selection.paraIndex].paraLength)) {
			++selection.offset;
		} else {
			selection.leadingEdge = true;
		}
		if((selection.offset >= paraOffset + (**docInfo).paraArray[selection.paraIndex].paraLength) && (selection.offset != (**docInfo).textLen)) {
			++selection.paraIndex;
			selection.lineIndex = 0L;
			selection.lastLine = false;
		}
	}
	errCode = GetWordOffset(docInfo, &selection, nil, nil, end, nil, nil);
	return noErr;
}

/**********************************************************************
 * SetupScrapID - save scrap ID so we know when it has change
 **********************************************************************/
void SetupScrapID(PETEGlobalsHandle globals)
{
	ScrapRef	scrap;
	GetCurrentScrap(&scrap);
	(**globals).clip.scrapID = scrap;;
}

/**********************************************************************
 * HasScrapChanged - has something else been copied to the scrap?
 **********************************************************************/
Boolean HasScrapChanged(PETEGlobalsHandle globals)
{
	ScrapRef	scrap;
	GetCurrentScrap(&scrap);
	return scrap != (**globals).clip.scrapID;
}