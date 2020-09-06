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

long GetEvenLength(StringPtr textPtr, long textLen, ScriptCode script);

long GetEvenLength(StringPtr textPtr, long textLen, ScriptCode script)
{
	Boolean doubleByte;
	CharByteTable table;
	long curLen, charLen;

	/* Only get the character length table if there are double byte characters */
	doubleByte = (((short)GetScriptVariable(script, smScriptFlags) & (1 << smsfSingByte)) == 0);
	if(!doubleByte) {
		return textLen;
	}
	
	FillParseTable(table, script);
	curLen = 0L;	
	while(curLen < textLen) {
		charLen = (table[textPtr[curLen]] == 0) ? 1L : 2L;
		if(curLen + charLen <= textLen) {
			curLen += charLen;
		} else {
			break;
		}
	}
	return curLen;
}

OSErr GetWordOffset(DocumentInfoHandle docInfo, SelDataPtr selection, SelDataPtr anchorSelection, long *startOffset, long *endOffset, Boolean *wordIsWhitespace, short *charType)
{
#pragma unused(anchorSelection)
	long evenLen, scriptRunOffset, scriptRunLen, textOffset, nextStyleRun, styleRunIndex/*, lineIndex, lineCount*/;
	LongSTElement tempStyle;
	LongStyleRun tempStyleRun;
	ParagraphInfoHandle paraInfo;
//	LSTable lineStarts;
	StringPtr textPtr;
	StringHandle theText;
	Byte hState;
	Boolean leadingEdge;
	OSErr errCode;
	LoadParams loadParam;
	ScriptCode script;
	OffsetTable offsets;
	
	leadingEdge = selection->leadingEdge && !selection->lastLine;
	nextStyleRun = 0L;
	textOffset = selection->offset;
	scriptRunLen = 0L;

	if((**(**docInfo).globals).wordBreakCallback != nil) {
		errCode = CallPETEWordBreakProc((**(**docInfo).globals).wordBreakCallback, docInfo, textOffset, leadingEdge, startOffset, endOffset, wordIsWhitespace, charType);
		
		if((errCode == noErr) || (errCode != handlerNotFoundErr)) {
			return errCode;
		}
	}

	paraInfo = (**docInfo).paraArray[selection->paraIndex].paraInfo;
	scriptRunOffset = ParagraphOffset(docInfo, selection->paraIndex);
	
/*	if(!selection->leadingEdge && !selection->lastLine &&
	   (anchorSelection != nil) && (anchorSelection->paraIndex == selection->paraIndex) &&
	   (anchorSelection->lineIndex >= selection->lineIndex)) {
		/* If the current paragraph hasn't been measured, measure it * /
		errCode = CheckParagraphMeasure(docInfo, selection->paraIndex, true);
		if(errCode != noErr) {
			return errCode;
		}
	
		lineStarts = (**paraInfo).lineStarts;
		
		lineCount = (InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement)) - 1L;
		
		for(lineIndex = 0L; (lineIndex <= lineCount) && ((*lineStarts)[lineIndex].lsStartChar <= selection->offset); ++lineIndex) {
			if(((*lineStarts)[lineIndex].lsStartChar == selection->offset) && (lineIndex == selection->lineIndex)) {
				leadingEdge = true;
				break;
			}
		}
	}
*/	
	if((textOffset != 0L) && (!leadingEdge || (textOffset == (**docInfo).textLen))) {
		--textOffset;
	}

	if((**docInfo).paraArray[selection->paraIndex].paraLength != 0L) {
		do {
			styleRunIndex = nextStyleRun;
			scriptRunOffset += scriptRunLen;
			scriptRunLen = ScriptRunLen((**docInfo).theStyleTable, paraInfo, styleRunIndex, &nextStyleRun);
			
		} while(nextStyleRun >= 0L && scriptRunLen > 0 && scriptRunLen + scriptRunOffset <= textOffset);
	}

	if(scriptRunLen > 0L) {
		textOffset = selection->offset - scriptRunOffset;

		loadParam.lpLenRequest = (scriptRunLen > SHRT_MAX) ? SHRT_MAX : scriptRunLen;
		errCode = LoadText(docInfo, textOffset, &loadParam, true);
		if(errCode != noErr) {
			return errCode;
		}
		
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		script = StyleToScript(&tempStyle.stInfo.textStyle);
		
		theText = (**docInfo).theText;
	
		hState = HGetState((Handle)theText);
		HLock((Handle)theText);
		
		textPtr = *theText + (scriptRunOffset - (**docInfo).textOffset);
		if(scriptRunLen > SHRT_MAX) {
			if(textOffset > SHRT_MAX / 2) {
				evenLen = GetEvenLength(textPtr, textOffset - SHRT_MAX / 2, script);
				textPtr += evenLen;
				scriptRunOffset += evenLen;
				scriptRunLen -= evenLen;
				textOffset -= evenLen;
			}
			if(scriptRunLen > SHRT_MAX) {
				scriptRunLen = GetEvenLength(textPtr, SHRT_MAX, script);
			}
		}
		
		if(textOffset == 0L) {
			++textOffset;
			leadingEdge = false;
		} else if(textOffset == scriptRunLen) {
			--textOffset;
			leadingEdge = true;
		}
		
		FindWordBreaks((Ptr)textPtr, scriptRunLen, textOffset, leadingEdge, nil, offsets, script);
		
		if(wordIsWhitespace != nil) {
			*wordIsWhitespace = (offsets[0].offFirst != offsets[0].offSecond) && IsWhitespace(textPtr + offsets[0].offFirst, offsets[0].offSecond - offsets[0].offFirst, script, (**(**docInfo).globals).whitespaceGlobals, nil);
		}
		
		if(charType != nil) {
			*charType = CharacterType((Ptr)textPtr, textOffset - (leadingEdge ? 0L : 1L), script) & 0xFF0F;
			if(!(GetScriptVariable(script, smScriptFlags) & (1L << smsfIntellCP))) {
				*charType |= kNoIntellCPFlag;
			}
			if(((*charType & smcTypeMask) == smCharPunct) && ((*charType & smcClassMask) == smPunctNormal)) {
				switch(textPtr[textOffset - (leadingEdge ? 0L : 1L)]) {
					case '�' :
					case '�' :
					case '�' :
					case '�' :
					case '�' :
						if((script != smRoman) || selection->leadingEdge) {
							break;
						}
						goto NotIntelligent;
					case '�' :
					case '�' :
					case '�' :
						if((script != smRoman) || !selection->leadingEdge) {
							break;
						}
						goto NotIntelligent;
					case '(' :
					case '{' :
					case '[' :
						if(selection->leadingEdge) {
							break;
						}
						goto NotIntelligent;
					case ')' :
					case '}' :
					case ']' :
					case '!' :
					case '.' :
					case '?' :
					case ',' :
					case ';' :
					case ':' :
						if(!selection->leadingEdge) {
							break;
						}
				NotIntelligent :
					default :
						*charType |= kNoIntellCPFlag;
				}
			}
		}
		
		HSetState((Handle)theText, hState);
		UnloadText(docInfo);
		
	} else {
		if(wordIsWhitespace != nil) {
			*wordIsWhitespace = false;
		}
		if(charType != nil) {
			*charType = kNoIntellCPFlag;
		}
		offsets[0].offFirst = 0;
		offsets[0].offSecond = 0;
	}
	
	if(startOffset != nil) {
		*startOffset = scriptRunOffset + offsets[0].offFirst;
	}
	if(endOffset != nil) {
		*endOffset = scriptRunOffset + offsets[0].offSecond;
	}
	
	return noErr;
}

OSErr GetWordFromOffset(DocumentInfoHandle docInfo, long offset, Boolean leadingEdge, long *startOffset, long *endOffset, Boolean *ws, short *charType)
{
	SelData selection;
	
	selection.offset = offset;
	selection.leadingEdge = leadingEdge;
	selection.lastLine = false;
	selection.paraIndex = ParagraphIndex(docInfo, offset - (leadingEdge ? 0L : 1L));
	return GetWordOffset(docInfo, &selection, nil, startOffset, endOffset, ws, charType);
}
