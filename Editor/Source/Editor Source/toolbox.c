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

pascal short MyTextWidth(StringPtr textPtr, short offset, short len, PETEGlobalsHandle globals)
{
	GrafPtr curPort;
	
	if(((**globals).romanFixedFontWidth != 0L) &&
	   (GetPort(&curPort), (GetPortTextFace(curPort) == 0)) &&
	   (GetPortTextFont(curPort) == (**globals).romanFixedFont) &&
	   (GetPortTextSize(curPort) == (**globals).romanFixedSize)) {
		
		return len * (**globals).romanFixedFontWidth;
	} else {
		return TextWidth(textPtr, offset, len);
	}
}

pascal short MyPixelToChar(Ptr textBuf, long textLen, short slop, short pixelWidth, Boolean *leadingEdge, short *widthRemaining, JustStyleCode styleRunPosition, Point numer, Point denom, PETEGlobalsHandle globals)
{
	short breakOffset;
	short charCount;
	GrafPtr curPort;
	
	if((slop == 0L) &&
	   ((**globals).romanFixedFontWidth != 0L) &&
	   (numer.h == 1) &&
	   (numer.v == 1) &&
	   (denom.h == 1) &&
	   (denom.v == 1) &&
	   (GetSysDirection() == leftCaret) &&
	   (FontScript() == smRoman) &&
	   (GetPort(&curPort), (GetPortTextFace(curPort) == 0)) &&
	   (GetPortTextFont(curPort) == (**globals).romanFixedFont) &&
	   (GetPortTextSize(curPort) == (**globals).romanFixedSize)) {

		charCount = pixelWidth / (**globals).romanFixedFontWidth;
		breakOffset = charCount;
		if(breakOffset >= textLen) {
			*widthRemaining = pixelWidth - breakOffset * (**globals).romanFixedFontWidth;
			breakOffset = textLen;
			*leadingEdge = true;
		} else {
			*widthRemaining = -1;
			if(!(*leadingEdge = !(charCount & 0x00001000))) {
				++breakOffset;
			}
		}
	} else {
		Fixed tempWidthRemaining;
		
		breakOffset = PixelToChar(textBuf, textLen > SHRT_MAX ? SHRT_MAX : textLen, Long2Fix(slop), Long2Fix(pixelWidth), leadingEdge, &tempWidthRemaining, styleRunPosition, numer, denom);
		*widthRemaining = tempWidthRemaining < 0 ? -1 : FixRound(tempWidthRemaining);
	}
	
	return breakOffset;
}

pascal short MyCharToPixel(Ptr textBuf, long textLen, short slop, long offset, short direction, JustStyleCode styleRunPosition, Point numer, Point denom, PETEGlobalsHandle globals)
{
	GrafPtr curPort;
	
	if((slop == 0L) &&
	   ((**globals).romanFixedFontWidth != 0L) &&
	   (numer.h == 1) &&
	   (numer.v == 1) &&
	   (denom.h == 1) &&
	   (denom.v == 1) &&
	   (GetSysDirection() == leftCaret) &&
	   (FontScript() == smRoman) &&
	   (GetPort(&curPort), (GetPortTextFace(curPort) == 0)) &&
	   (GetPortTextFont(curPort) == (**globals).romanFixedFont) &&
	   (GetPortTextSize(curPort) == (**globals).romanFixedSize)) {
		return textLen * (**globals).romanFixedFontWidth;
	} else {
		return CharToPixel(textBuf, textLen, slop, offset, direction, styleRunPosition, numer, denom);
	}
}

/* Return the address of the character immediately before a given character in memory */
Ptr PreviousChar(Ptr textPtr, Ptr curChar, CharByteTable table)
{
	short lastCharLen;
	StringPtr textStart, lastChar;
	
	/* Get the current character pointer */
	lastChar = (StringPtr)curChar;
	
	/* If the table is empty, there are only single byte characters */
	if(table == nil) {
		lastCharLen = 1;
	} else {
		/* There are double byte characters, so get the start of the text buffer */
		textStart = (StringPtr)textPtr;
		
		/* Loop through until we hit the last character */
		while(textStart < lastChar) {
			
			/* Check to see if it's a double byte character */
			if(table[*textStart++] != 0) {
				/* It is double byte, so increment textStart once more */
				++textStart;
				/* Length of a double byte character is 2 */
				lastCharLen = 2;
			} else {
				/* Length of a single byte character is 1 */
				lastCharLen = 1;
			}
		}
		lastChar = textStart;
	}
	
	/* Move the pointer back by the length of the character */
	lastChar -= lastCharLen;
	
	return (Ptr)lastChar;
}

#pragma export on
/* Replacement of system VisibleLength() that works */
pascal long MyVisibleLength(Ptr textPtr, long len, short direction)
{
	Boolean bidirectional, doubleByte;
	CharByteTable table;
	Ptr curChar;
	short charType;
	
	/* Check to see if current font is bidirectional */
	bidirectional = !(!(Boolean)GetScriptVariable(smCurrentScript, smScriptRight));

	/* Only get the character length table if there are double byte characters */
	doubleByte = (((short)GetScriptVariable(smCurrentScript, smScriptFlags) & (1 << smsfSingByte)) == 0);
	if(doubleByte) {
		/* Get the character length table */
		FillParseTable(table, smCurrentScript);
	}
	
	/* Start right after the last character in memory order */
	curChar = textPtr + len;
	
	/* Only strip if bidirectional, or unidirectional in a left to right paragraph */
	if(bidirectional || (direction == leftCaret)) {

		/* Loop backwards through the characters */
		do {
			/* Get the previous character */
			curChar = PreviousChar(textPtr, curChar, doubleByte ? table : nil);
			
			if(bidirectional || doubleByte || (*curChar != kNullChar && *curChar != kTabChar && *curChar != kCarriageReturnChar && *curChar != kSpaceChar && *curChar != kPageDownChar && *curChar != kLineFeedChar)) {
				
				/* Get the characters type */
				charType = CharacterType(curChar, (doubleByte && table[*curChar] != 0) ? 1 : 0, smCurrentScript);
			} else {
				/* Roman space character optimization */
				charType = smCharPunct | smPunctBlank | smCharHorizontal | smCharLeft | smCharLower | smChar1byte;
			}

			/* Stop if the current character isn't whitespace */
			if((charType & (short)(smcTypeMask | smcClassMask)) != (short)(smCharPunct | smPunctBlank)) {
				break;
			}
			
			/* Stop if bidirectional and the character and line direction don't match */
			if(bidirectional && ((direction == leftCaret) != ((charType & (short)smcRightMask) == smCharLeft))) {
				break;
			}
		} while((--len > 0) && (curChar != textPtr));
	}

	/* Return the length after stripping whitespace */
	return len;
}
#pragma export reset

pascal void RectAndRgn(RgnHandle destRgn, RgnHandle scratchRgn, Rect *sourceRect)
{
	RectRgn(scratchRgn, sourceRect);
	UnionRgn(scratchRgn, destRgn, destRgn);
}

pascal void GetControlRect(ControlRef theControl, Rect *controlRect)
{
	GetControlBounds(theControl,controlRect);
}

pascal void Update1Control(ControlRef theControl, RgnHandle visRgn)
{
	Rect controlRect;
	
	if(theControl != nil) {
		GetControlRect(theControl, &controlRect);
		
		if(RectInRgn(&controlRect, visRgn)) {
			Draw1Control(theControl);
		}
	}	
}

pascal void GetVisibleRgn(RgnHandle visRgn)
{
	GrafPtr tempPort;
	
	GetPort(&tempPort);
	GetPortVisibleRegion(tempPort,visRgn);
}

pascal void SetVisibleRgn(RgnHandle visRgn)
{
	GrafPtr tempPort;
	
	GetPort(&tempPort);
	SetPortVisibleRegion(tempPort,visRgn);
}

pascal void GetPortRect(Rect *portRect)
{
	GrafPtr tempPort;
	
	GetPort(&tempPort);
	GetPortBounds(tempPort,portRect);
}

pascal Style GetPortFace()
{
	GrafPtr tempPort;
	
	GetPort(&tempPort);
	return GetPortTextFace(tempPort);
}

pascal Handle MyNewHandle(long hndlSize, OSErr *errCode, short handleFlags)
{
	Handle newHndl;
	Ptr tempPtr;
	
	if((handleFlags & hndlTemp) && (handleFlags & hndlSys)) {
		*errCode = paramErr;
		return nil;
	}

	if(handleFlags & hndlTemp) {
		newHndl = TempNewHandle((handleFlags & hndlEmpty) ? 0L : hndlSize, errCode);
		if(*errCode != noErr) {
			goto RealHandle;
		}
		if(handleFlags & hndlEmpty) {
			EmptyHandle(newHndl);
		} else if(handleFlags & hndlClear) {
			tempPtr = *newHndl;
			while(--hndlSize >= 0L) {
				*tempPtr++ = '\0';
			}
		}
	} else {
RealHandle :
		if(handleFlags & hndlSys) {
			if(handleFlags & hndlEmpty) {
				newHndl = NewEmptyHandleSys();
			} else if(handleFlags & hndlClear) {
				newHndl = NewHandleSysClear(hndlSize);
			} else {
				newHndl = NewHandleSys(hndlSize);
			}
		} else {
			if(handleFlags & hndlEmpty) {
				newHndl = NewEmptyHandle();
			} else if(handleFlags & hndlClear) {
				newHndl = NewHandleClear(hndlSize);
			} else {
				newHndl = NewHandle(hndlSize);
			}
		}
		*errCode = MemError();
		if(*errCode != noErr) {
			return nil;
		}
	}
	
	return newHndl;
}

pascal OSErr MyPtrToHand(void *srcPtr, Handle *destHndl, long size, short handleFlags)
{
	OSErr errCode;
	
	*destHndl = MyNewHandle(size, &errCode, handleFlags);
	if(errCode == noErr) {
		BlockMoveData(srcPtr, **destHndl, size);
	}
	return errCode;
}

pascal OSErr MyHandToHand(Handle *theHndl, short handleFlags)
{
	long size;
	Handle newHndl;
	OSErr errCode;
	Byte hState;
	
	if(*theHndl == nil) return memWZErr;
	if(**theHndl == nil) return nilHandleErr;
	hState = HGetState((Handle)*theHndl);
	HNoPurge((Handle)*theHndl);
	size = InlineGetHandleSize(*theHndl);
	if(size < 0L) {
		return size;
	}
	
	newHndl = MyNewHandle(size, &errCode, handleFlags);
	HSetState((Handle)*theHndl, hState);
	if(errCode == noErr) {
		BlockMoveData(**theHndl, *newHndl, size);
		*theHndl = newHndl;
	}
	return errCode;
}

pascal void LocalToGlobalRect(Rect *theRect)
{
	Point offsetPt;
	
	offsetPt.h = offsetPt.v = 0;
	LocalToGlobal(&offsetPt);
	OffsetRect(theRect, offsetPt.h, offsetPt.v);
}

pascal void LocalToGlobalRgn(RgnHandle theRgn)
{
	Point offsetPt;
	
	offsetPt.h = offsetPt.v = 0;
	LocalToGlobal(&offsetPt);
	OffsetRgn(theRgn, offsetPt.h, offsetPt.v);
}

pascal void GlobalToLocalRect(Rect *theRect)
{
	Point offsetPt;
	
	offsetPt.h = offsetPt.v = 0;
	GlobalToLocal(&offsetPt);
	OffsetRect(theRect, offsetPt.h, offsetPt.v);
}

pascal void GlobalToLocalRgn(RgnHandle theRgn)
{
	Point offsetPt;
	
	offsetPt.h = offsetPt.v = 0;
	GlobalToLocal(&offsetPt);
	OffsetRgn(theRgn, offsetPt.h, offsetPt.v);
}

pascal Boolean EqualFont(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, short font1, short lang1, short font2, short lang2)
{
#pragma unused (globals, docInfo, lang1, lang2)
	return (font1 == font2);
}

pascal Boolean EqualFontSize(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, short size1, short size2)
{
#pragma unused (globals, docInfo)
	return (size1 == size2);
}

pascal short StyleToScript(MyTextStylePtr theStyle)
{
	if((theStyle->tsFont == kPETEDefaultFont) || (theStyle->tsFont == kPETEDefaultFixed)) {
		return LangToScript(theStyle->tsLang);
	} else {
		return FontToScript(theStyle->tsFont);
	}
}

pascal short RelativeSize(short **fontSizes, short defaultSize, short relativeSize);
pascal short RelativeSize(short **fontSizes, short defaultSize, short relativeSize)
{
	short offset = 0;
	long numFonts, index;
	
	offset = (relativeSize == kPETEDefaultSize) ? 0 : relativeSize - kPETERelativeSizeBase;
	if(offset == 0) {
		return defaultSize;
	}
	numFonts = InlineGetHandleSize((Handle)fontSizes) / sizeof(short);
	if(offset > 0) {
		for(index = 0; index < numFonts; ++index) {
			if(defaultSize <= (*fontSizes)[index]) {
				break;
			}
		}
	} else {
		for(index = numFonts; index > 0; --index) {
			if(defaultSize > (*fontSizes)[index - 1]) {
				break;
			}
		}
	}
	index += offset;
	if(index < 0) {
		index = 0;
	} else if(index >= numFonts) {
		index = numFonts - 1;
	}
	return (*fontSizes)[index];
}

pascal long DefaultFontIndex(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEDefaultFontHandle *defaultFonts, ScriptCode scriptCode, Boolean fixed, Boolean printing);
pascal long DefaultFontIndex(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, PETEDefaultFontHandle *defaultFonts, ScriptCode scriptCode, Boolean fixed, Boolean printing)
{
	long index;
	
	*defaultFonts = nil;
	if(docInfo != nil) {
		*defaultFonts = (**docInfo).defaultFonts;
		if(*defaultFonts != nil) {
			for(index = InlineGetHandleSize((Handle)*defaultFonts) / sizeof(PETEDefaultFontEntry); --index >= 0L; ) {
				if((scriptCode == (**defaultFonts)[index].pdScript) && (!fixed == !(**defaultFonts)[index].pdFixed) && (!printing == !(**defaultFonts)[index].pdPrint)) {
					if((**defaultFonts)[index].pdFont == kPETEDefaultFont) {
						break;
					} else {
						return index;
					}
				}
			}
		}
	}
	*defaultFonts = nil;
	if((globals == nil) && (docInfo != nil)) {
		globals = (**docInfo).globals;
	}
	if(globals != nil) {
		*defaultFonts = (**globals).defaultFonts;
		if(*defaultFonts != nil) {
			for(index = InlineGetHandleSize((Handle)*defaultFonts) / sizeof(PETEDefaultFontEntry); --index >= 0L; ) {
				if((scriptCode == (**defaultFonts)[index].pdScript) && (!fixed == !(**defaultFonts)[index].pdFixed) && (!printing == !(**defaultFonts)[index].pdPrint)) {
					return index;
				}
			}
		}
	}
	*defaultFonts = nil;
	return -1L;
}

pascal short StyleToFont(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean printing)
{
	ScriptCode scriptCode;
	long index;
	Boolean fixed;
	PETEDefaultFontHandle defaultFonts;
	
	if(!(!(fixed = (theStyle->tsFont == kPETEDefaultFixed))) || (theStyle->tsFont == kPETEDefaultFont)) {
		scriptCode = StyleToScript(theStyle);
		index = DefaultFontIndex(globals, docInfo, &defaultFonts, scriptCode, fixed, printing);
		
		if((index != -1L) && (defaultFonts != nil) && ((*defaultFonts)[index].pdFont != kPETEDefaultFont)) {
			return (*defaultFonts)[index].pdFont;
		} else {
			return HiWord(GetScriptVariable(scriptCode, fixed ? smScriptMonoFondSize : smScriptAppFondSize));
		}
	}
	return theStyle->tsFont;
}

pascal short StyleToFontSize(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean printing)
{
	ScriptCode scriptCode;
	Boolean fixed;
	long index, fondSize;
	PETEDefaultFontHandle defaultFonts;
	short relSize;
	
	if(theStyle->tsSize <= kPETEDefaultSize) {
		if(globals == nil) {
			globals = (**docInfo).globals;
		}
		scriptCode = StyleToScript(theStyle);
		fixed = (theStyle->tsFont == kPETEDefaultFixed);
		if(!fixed && (theStyle->tsFont != kPETEDefaultFont)) {
			index = DefaultFontIndex(globals, docInfo, &defaultFonts, scriptCode, true, printing);
			if((index >= 0L) && (defaultFonts != nil)) {
				if((*defaultFonts)[index].pdFont == theStyle->tsFont) {
					relSize = RelativeSize((**globals).fontSizes, (*defaultFonts)[index].pdSize, theStyle->tsSize);
					index = DefaultFontIndex(globals, docInfo, &defaultFonts, scriptCode, false, printing);
					if((index >= 0L) && (defaultFonts != nil) && ((*defaultFonts)[index].pdFont == theStyle->tsFont)) {
						return RelativeSize((**globals).fontSizes, (*defaultFonts)[index].pdSize, theStyle->tsSize);
					} else {
						return relSize;
					}
				}
			} else {
				fondSize = GetScriptVariable(scriptCode, smScriptMonoFondSize);
				if(HiWord(fondSize) == theStyle->tsFont) {
					return RelativeSize((**globals).fontSizes, LoWord(fondSize), theStyle->tsSize);
				}
			}
		}
		
		index = DefaultFontIndex(globals, docInfo, &defaultFonts, scriptCode, fixed, printing);
		if((index >= 0L) && (defaultFonts != nil)) {
			return RelativeSize((**globals).fontSizes, (*defaultFonts)[index].pdSize, theStyle->tsSize);
		} else {
			return RelativeSize((**globals).fontSizes, LoWord(GetScriptVariable(scriptCode, fixed ? smScriptMonoFondSize : smScriptAppFondSize)), theStyle->tsSize);
		}
	} else {
		return theStyle->tsSize;
	}
}

pascal short LangToScript(short langCode)
{
	ScriptCode scriptCode;
	
	switch(langCode) {
		default :
			scriptCode = 0;
			break;

		case langEnglish :			/* smRoman script */
		case langFrench :			/* smRoman script */
		case langGerman :			/* smRoman script */
		case langItalian :			/* smRoman script */
		case langDutch :			/* smRoman script */
		case langSwedish :			/* smRoman script */
		case langSpanish :			/* smRoman script */
		case langDanish :			/* smRoman script */
		case langPortuguese :		/* smRoman script */
		case langNorwegian :		/* smRoman script */
		case langFinnish :			/* smRoman script */
		case langIcelandic :		/* extended Roman script */
		case langMaltese :			/* extended Roman script */
		case langTurkish :			/* extended Roman script */
		case langCroatian :			/* Serbo-Croatian in extended Roman script */
		case langSaamisk :			/* ext. Roman script, lang. of the Sami/Lapp people of Scand. */
		case langFaeroese :			/* smRoman script */
		case langFlemish :			/* smRoman script */
		case langIrish :			/* smRoman script */
		case langAlbanian :			/* smRoman script */
		case langIndonesian :		/* smRoman script */
		case langTagalog :			/* smRoman script */
		case langMalayRoman :		/* Malay in smRoman script */
		case langSomali :			/* smRoman script */
		case langSwahili :			/* smRoman script */
		case langRuanda :			/* smRoman script */
		case langRundi :			/* smRoman script */
		case langChewa :			/* smRoman script */
		case langMalagasy :			/* smRoman script */
		case langEsperanto :		/* extended Roman script */
		case langWelsh :			/* smRoman script */
		case langBasque :			/* smRoman script */
		case langCatalan :			/* smRoman script */
		case langLatin :			/* smRoman script */
		case langQuechua :			/* smRoman script */
		case langGuarani :			/* smRoman script */
		case langAymara :			/* smRoman script */
		case langJavaneseRom :		/* Javanese in smRoman script */
		case langSundaneseRom :		/* Sundanese in smRoman script */
			scriptCode = smRoman;
			break;

		case langJapanese :			/* smJapanese script */
			scriptCode = smJapanese;
			break;
		
		case langTradChinese :		/* Chinese in traditional characters */
			scriptCode = smTradChinese;
			break;
			
		case langKorean :			/* smKorean script */
			scriptCode = smKorean;
			break;
			
		case langArabic :			/* smArabic script */
		case langUrdu :				/* smArabic script */
		case langFarsi :			/* smArabic script */
		case langAzerbaijanAr :		/* Azerbaijani in smArabic script (Iran) */
		case langPashto :			/* smArabic script */
		case langKurdish :			/* smArabic script */
		case langKashmiri :			/* smArabic script */
		case langMalayArabic :		/* Malay in smArabic script */
		case langUighur :			/* smArabic script */
			scriptCode = smArabic;
			break;
			
		case langHebrew :			/* smHebrew script */
		case langYiddish :			/* smHebrew script */
			scriptCode = smHebrew;
			break;
			
		case langGreek :			/* smGreek script */
			scriptCode = smGreek;
			break;
			
		case langRussian :			/* smCyrillic script */
		case langSerbian :			/* Serbo-Croatian in smCyrillic script */
		case langMacedonian :		/* smCyrillic script */
		case langBulgarian :		/* smCyrillic script */
		case langUkrainian :		/* smCyrillic script */
		case langByelorussian :		/* smCyrillic script */
		case langUzbek :			/* smCyrillic script */
		case langKazakh :			/* smCyrillic script */
		case langAzerbaijani :		/* Azerbaijani in smCyrillic script (USSR) */
		case langMoldavian :		/* smCyrillic script */
		case langKirghiz :			/* smCyrillic script */
		case langTajiki :			/* smCyrillic script */
		case langTurkmen :			/* smCyrillic script */
		case langMongolianCyr :		/* Mongolian in smCyrillic script */
		case langTatar :			/* smCyrillic script */
			scriptCode = smCyrillic;
			break;
			
		case langHindi :			/* smDevanagari script */
		case langNepali :			/* smDevanagari script */
		case langSanskrit :			/* smDevanagari script */
		case langMarathi :			/* smDevanagari script */
			scriptCode = smDevanagari;
			break;
			
		case langPunjabi :			/* smGurmukhi script */
			scriptCode = smGurmukhi;
			break;
			
		case langGujarati :			/* smGujarati script */
			scriptCode = smGujarati;
			break;
			
		case langOriya :			/* smOriya script */
			scriptCode = smOriya;
			break;
			
		case langBengali :			/* smBengali script */
		case langAssamese :			/* smBengali script */
			scriptCode = smBengali;
			break;
			
		case langTamil :			/* smTamil script */
			scriptCode = smTamil;
			break;
			
		case langTelugu :			/* smTelugu script */
			scriptCode = smTelugu;
			break;
			
		case langKannada :			/* smKannada script */
			scriptCode = smKannada;
			break;
			
		case langMalayalam :		/* smMalayalam script */
			scriptCode = smMalayalam;
			break;
			
		case langSinhalese :		/* smSinhalese script */
			scriptCode = smSinhalese;
			break;
			
		case langBurmese :			/* smBurmese script */
			scriptCode = smBurmese;
			break;
			
		case langKhmer :			/* smKhmer script */
			scriptCode = smKhmer;
			break;
			
		case langThai :				/* smThai script */
			scriptCode = smThai;
			break;
			
		case langLao :				/* smLaotian script */
			scriptCode = smLaotian;
			break;
			
		case langGeorgian :			/* smGeorgian script */
			scriptCode = smGeorgian;
			break;
			
		case langArmenian :			/* smArmenian script */
			scriptCode = smArmenian;
			break;
			
		case langSimpChinese :		/* Chinese in simplified characters */
			scriptCode = smSimpChinese;
			break;
		
		case langTibetan :			/* smTibetan script */
		case langDzongkha :			/* (lang of Bhutan) smTibetan script */
			scriptCode = smTibetan;
			break;
			
		case langMongolian :		/* Mongolian in smMongolian script */
			scriptCode = smMongolian;
			break;
			
		case langAmharic :			/* smEthiopic script */
		case langTigrinya :			/* smEthiopic script */
		case langGalla :			/* smEthiopic script */
			scriptCode = smGeez;
			break;

		case langLithuanian :		/* smEastEurRoman script */
		case langPolish :			/* smEastEurRoman script */
		case langHungarian :		/* smEastEurRoman script */
		case langEstonian :			/* smEastEurRoman script */
		case langRomanian :			/* smEastEurRoman script */
		case langCzech :			/* smEastEurRoman script */
		case langSlovak :			/* smEastEurRoman script */
		case langSlovenian :		/* smEastEurRoman script */
		case langLettish :			/* smEastEurRoman script */
			scriptCode = smEastEurRoman;
			break;
			
		case langVietnamese :		/* smVietnamese script */
			scriptCode = smVietnamese;
			break;
			
		case langSindhi :			/* smExtArabic script */
			scriptCode = smExtArabic;
			break;
			
		case systemCurLang :
			scriptCode = GetScriptManagerVariable(smSysScript);
	}
	return scriptCode;
}

//	Some Carbon replacement functions

#define RectHi(r) ((r).bottom-(r).top)
#define RectWi(r) ((r).right-(r).left)

ScrapRef	gUseThisScrap;

void GetCWMgrPort(CGrafPtr *wPort);
static void MyGetCurrentScrap(ScrapRef *pScrap);

/**********************************************************************
 * GetCWMgrPort - make a wmgr port
 **********************************************************************/
void GetCWMgrPort(CGrafPtr *wPort)
{
	static CGrafPtr wMgrPort;
	static Rect	rWMgrPort;
	
	if (!wMgrPort) MyCreateNewPort(wMgrPort);
	if (wMgrPort)
	{
		//	Set up size and position of port
		Rect	gdRect;
		GDHandle	gd;
		
		gd=GetMainDevice();
		if (gd)
		{
			gdRect = (*gd)->gdRect;
			if (!EqualRect(&gdRect,&rWMgrPort))
			{
				GrafPtr	savePort;
				
				GetPort(&savePort);
				SetPort(wMgrPort);
				MovePortTo(gdRect.left,gdRect.top);
				PortSize(RectWi(gdRect),RectHi(gdRect));
				SetPort(savePort);
				rWMgrPort = gdRect;
			}
		}
	}
	*wPort = wMgrPort;
}
void GetWMgrPort(GrafPtr *wPort) {GetCWMgrPort((CGrafPtr*)wPort);}

/**********************************************************************
 * GetScrap - old Scrap Manager replacement function
 **********************************************************************/
static void MyGetCurrentScrap(ScrapRef *pScrap)
{
	if (gUseThisScrap)
		*pScrap = gUseThisScrap;
	else
		GetCurrentScrap(pScrap);
}

/**********************************************************************
 * UseThisScrap - use specified scrap for scrap operations
 **********************************************************************/
void UseThisScrap(ScrapRef scrap)
{
	gUseThisScrap = scrap;
}

/**********************************************************************
 * GetScrap - old Scrap Manager replacement function
 **********************************************************************/
long GetScrap(Handle h,ScrapFlavorType flavorType,SInt32 *offset)
{
	ScrapRef	scrap;
	Size		byteCount=0;
	OSErr		err;
	
	MyGetCurrentScrap(&scrap);
	err = GetScrapFlavorSize(scrap,flavorType,&byteCount);
	
	// Hack for Safari, which puts URL text along with its image copies
	if (flavorType == kTextScrap && err == noErr)
	{
		Size junk;
		err = GetScrapFlavorSize(scrap,kURLScrap,&junk);
		if (!err) err = GetScrapFlavorSize(scrap,kPictureScrap,&junk);
		
		if (err) err = noErr;	// does not have TEXT, url, and PICT; TEXT is OK
		else err = noTypeErr;	// has all three.  Pretend it does NOT have TEXT
	}
	// End hack for Safari
		
	if (!err)
	{
		SetHandleSize(h,byteCount);
		if (!(err = MemError()))
		{
			HLock(h);
			err = GetScrapFlavorData(scrap,flavorType,&byteCount,*h);
			HUnlock(h);
		}
	}
	*offset = 0;
	if (err) err = noTypeErr;
	return err ? err : byteCount;
}

/**********************************************************************
 * PutScrap - old Scrap Manager replacement function
 **********************************************************************/
OSStatus PutScrap(SInt32 byteCount,ScrapFlavorType flavorType,const void *source)
{
	ScrapRef	scrap;
	
	MyGetCurrentScrap(&scrap);
	return PutScrapFlavor(scrap,flavorType,kScrapFlavorMaskNone,byteCount,source);
}
