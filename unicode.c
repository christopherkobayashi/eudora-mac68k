/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define MAX_UTF8_CHAR_LEN (((sizeof(UniChar) * 8) / 6) + ((6 - ((sizeof(UniChar) * 8) / 6)) < ((sizeof(UniChar) * 8) % 6) ? 2 : 1))

#if __profile__
#include "unicodeprofile.h"
#endif

typedef enum {
	umFlowedDir,
	umCharsetDir,
	umHtmlDir,
	umRichDir,
	umDirCount
} umDirectives;

typedef enum {
	umHeaderState,
	umFlowedState,
	umCharsetState,
	umHtmlState,
	umRichState,
	umTextState,
} umStates;

typedef struct {
	Byte xState;
	Byte tState;
	Byte cState;
	Byte oState;
} UniGlobalsState;

static struct UniGlobals {
	TECObjectRef internetToUTF8;
	TextEncoding internetToUTF8Encoding;
	Boolean maclatin1;
	UnicodeToTextRunInfo UTF8ToMac;
	IntlConverter quickConverter;
	ByteCount origTextLen;
	TextPtr *origText;
	ByteCount convTextLen;
	TextPtr *convText;
	ItemCount convRunsCount;
	TextEncodingRun **convRuns;
	FormatOrderPtr *order;
	UInt16 tecVersion;
	Boolean hasTextRunFlag;
	Boolean has88591VariantsFlag;
} uGlobals = { 0 };

OSStatus ConvertUTF8Text(BytePtr theText, ByteCount bufLen);
OSStatus DrawConvertedUTF8(short width, ScriptFontInfo fonts);
OSStatus MeasureConvertedUTF8(short *width, FontInfo * maxFont,
			      Boolean * rightToLeft, ScriptFontInfo fonts);
pascal long MyVisibleLength(Ptr textPtr, long len, short direction);
pascal Boolean MyRlDirProc(short theFormat, void *dirParam);
long EnsureHandleSize(Handle h, Size s);
void NoPurgeUniGlobals(UniGlobalsState * uniState);
void ResetUniGlobals(UniGlobalsState * uniState);
OSStatus GetUnicodeHint(TextEncoding encoding, StringPtr lang,
			UniChar * hint);
OSStatus ClearIntlConverterContext(UnicodeToTextRunInfo converter,
				   Boolean utf8);
OSStatus UpdateIntlConverterLo(IntlConverter * converter,
			       StringPtr charset, TextEncoding encoding);
OSStatus UpdateTECConverter(TECObjectRef * converter, StringPtr charset,
			    TextEncoding newEncoding,
			    TextEncoding * fromEncoding,
			    TextEncoding toEncoding, Boolean * maclatin1,
			    UniChar * hint);
UniCharCount UTF8CharCount(BytePtr utf8, ByteCount bufLen);
OSErr AccuEnsureSize(AccuPtr a, long len, long incr);
short GetIntlFont(ScriptCode script, ScriptFontInfo fonts);
short GetIntlSize(ScriptCode script, ScriptFontInfo fonts);
void CleanISO2022(TextPtr text, long len, IntlConverter * converter);
OSStatus InsertIntlHeaders(Handle text, long len, long tOff, AccuPtr a,
			   TextEncoding encoding, PETEHandle pte,
			   long *pOff);
long ParseCharset(Ptr textPtr, long len, PStr charset, umDirectives c);
OSErr PeteGetStyleRun(PETEHandle pte, long offset, long *len,
		      PETEStyleInfoPtr style, long validBits);
OSStatus MyTECConvertText(TECObjectRef encodingConverter,
			  ConstTextPtr inputBuffer,
			  ByteCount inputBufferLength,
			  ByteCount * actualInputLength,
			  TextPtr outputBuffer,
			  ByteCount outputBufferLength,
			  ByteCount * actualOutputLength,
			  Boolean maclatin1);
long UnicodeMappingCount(TextEncoding encoding);

OSStatus InitUnicode()
{
	UnicodeMapping tempMapping;
	TECInfoHandle info;
	OSStatus statusCode;

	Zero(uGlobals);

	if ((long) TECGetInfo == kUnresolvedCFragSymbolAddress) {
		Log(-1, "\pNo Unicode");
		return noErr;
	}

	statusCode = TECGetInfo(&info);
	if (statusCode != noErr) {
		Log(-1, "\pTECGetInfo failed");
		return noErr;
	}

	uGlobals.tecVersion = (**info).tecVersion;
	uGlobals.hasTextRunFlag =
	    !(!((**info).
		tecUnicodeConverterFeatures &
		kTECAddTextRunHeuristicsMask));
	uGlobals.has88591VariantsFlag =
	    UnicodeMappingCount(DefaultEncoding
				(kTextEncodingMacRomanLatin1)) > 1;
	tempMapping.unicodeEncoding =
	    CreateTextEncoding(kTextEncodingUnicodeDefault,
			       kTextEncodingDefaultVariant,
			       kUnicodeUTF8Format);
	statusCode =
	    CreateUnicodeToTextRunInfo(0, &tempMapping,
				       &uGlobals.UTF8ToMac);
	if (statusCode != noErr) {
		Log(-1, "\pUTF8 To Mac failed");
		return statusCode;
	}

	statusCode =
	    CreateIntlConverter(&uGlobals.quickConverter,
				kTextEncodingUnknown);
	if (statusCode == noErr) {
		if (((uGlobals.origText = NuHTempBetter(0)) == nil) ||
		    ((uGlobals.convText = NuHTempBetter(0)) == nil) ||
		    ((uGlobals.convRuns =
		      NuHTempBetter(2 * sizeof(TextEncodingRun))) == nil)
		    || ((uGlobals.order = NuHTempBetter(0)) == nil)) {
			statusCode = memFullErr;
		} else {
			HPurge((Handle) uGlobals.origText);
			HPurge((Handle) uGlobals.convText);
			HPurge((Handle) uGlobals.convRuns);
			HPurge((Handle) uGlobals.order);
		}
	}
	if (statusCode != noErr)
		CleanupUnicode();
	return statusCode;
}

void CleanupUnicode()
{
	if (uGlobals.tecVersion > 0)
		return;

	DisposeUnicodeToTextRunInfo(&uGlobals.UTF8ToMac);
	if (uGlobals.internetToUTF8) {
		TECDisposeConverter(uGlobals.internetToUTF8);
		uGlobals.internetToUTF8 = 0;
	}
	DisposeIntlConverter(uGlobals.quickConverter);
	ZapHandle(uGlobals.origText);
	ZapHandle(uGlobals.convText);
	ZapHandle(uGlobals.convRuns);
	ZapHandle(uGlobals.order);
	Zero(uGlobals);
}

Boolean UTF8ToRoman(BytePtr theText, ByteCount * textLen, ByteCount bufLen)
{
	if (ConvertUTF8Text(theText, *textLen) != noErr)
		return false;
	if (uGlobals.convRunsCount != 1)
		return false;
	if (ResolveDefaultTextEncoding
	    ((*uGlobals.convRuns)[0].textEncoding) !=
	    ResolveDefaultTextEncoding(DefaultEncoding
				       (kTextEncodingMacRoman)))
		return false;
	*textLen = MIN(bufLen, uGlobals.convTextLen);
	BMD(*(uGlobals.convText), theText, *textLen);
	return true;
}

Boolean RomanToUTF8(PStr s, long strSize)
{
	UniChar temp[128];
	TextToUnicodeInfo info;
	UnicodeMapping tempMapping;
	OSStatus statusCode;
	ByteCount len;

	Zero(tempMapping);
	tempMapping.mappingVersion = kUnicodeUseLatestMapping;
	tempMapping.unicodeEncoding =
	    CreateTextEncoding(kTextEncodingUnicodeDefault,
			       kTextEncodingDefaultVariant,
			       kUnicodeUTF8Format);
	tempMapping.otherEncoding =
	    CreateTextEncoding(kTextEncodingMacRoman,
			       kTextEncodingDefaultVariant,
			       kTextEncodingDefaultFormat);
	statusCode = CreateTextToUnicodeInfo(&tempMapping, &info);
	if (statusCode)
		return statusCode;
	statusCode =
	    ConvertFromPStringToUnicode(info, s, strSize - 1, &len, temp);
	if (statusCode && statusCode != kTECOutputBufferFullStatus
	    && statusCode != kTECUsedFallbacksStatus)
		return statusCode;
	BlockMoveData(&temp, &s[1], len);
	s[0] = len;
	return noErr;
}

OSStatus DrawUTF8Text(BytePtr theText, ByteCount bufLen, short width,
		      ScriptFontInfo fonts)
{
	OSStatus statusCode;

	statusCode = ConvertUTF8Text(theText, bufLen);
	if (statusCode != noErr) {
		return statusCode;
	}
	return DrawConvertedUTF8(width, fonts);
}

OSStatus DrawConvertedUTF8(short width, ScriptFontInfo fonts)
{
	ItemCount run, runDisp;
	OSStatus statusCode;
	ScriptCode script;
	short fontID, oldFont, oldSize;
	GrafPtr port;
	Boolean lineRight;
	ByteOffset offset;
	ByteCount runLen;
	UniGlobalsState uniState;

	NoPurgeUniGlobals(&uniState);
	GetPort(&port);
	oldFont = GetPortTextFont(port);
	oldSize = GetPortTextSize(port);
	for (statusCode = noErr, run = 0;
	     statusCode == noErr && run < uGlobals.convRunsCount; ++run) {
		runDisp = (**uGlobals.order)[run];
		statusCode =
		    RevertTextEncodingToScriptInfo((*uGlobals.
						    convRuns)[runDisp].
						   textEncoding, &script,
						   nil, GlobalTemp);
		if (statusCode == noErr) {
			if (GlobalTemp[0] != 0) {
				GetFNum(GlobalTemp, &fontID);
			} else {
				fontID = GetIntlFont(script, fonts);
			}
			TextFont(fontID);
			TextSize(GetIntlSize(script, fonts));
			if (run == 0) {
				lineRight =
				    !(!(Boolean)
				      GetScriptVariable(script,
							smScriptRight));
				if (lineRight) {
					short fullWidth;

					statusCode =
					    MeasureConvertedUTF8
					    (&fullWidth, nil, nil, fonts);
					if (statusCode != noErr)
						break;
					if (width < fullWidth) {
						Move(width - fullWidth, 0);
					}
				}
			}
			offset = (*uGlobals.convRuns)[runDisp].offset;
			runLen =
			    ((runDisp ==
			      uGlobals.convRunsCount -
			      1) ? uGlobals.convTextLen : (*uGlobals.
							   convRuns)
			     [runDisp + 1].offset) - offset;
			HLock((Handle) uGlobals.convText);
			if ((**uGlobals.order)[run] ==
			    uGlobals.convRunsCount - 1) {
				runLen =
				    MyVisibleLength(*uGlobals.convText +
						    offset, runLen,
						    lineRight ? rightCaret
						    : leftCaret);
			}
			DrawText(*uGlobals.convText, offset, runLen);
			HUnlock((Handle) uGlobals.convText);
		}
	}
	TextSize(oldSize);
	TextFont(oldFont);
	ResetUniGlobals(&uniState);
	return statusCode;
}

OSStatus MeasureUTF8Text(BytePtr theText, ByteCount bufLen, short *width,
			 FontInfo * maxFont, Boolean * rightToLeft,
			 ScriptFontInfo fonts)
{
	OSStatus statusCode;

	statusCode = ConvertUTF8Text(theText, bufLen);
	if (statusCode != noErr) {
		return statusCode;
	}
	return MeasureConvertedUTF8(width, maxFont, rightToLeft, fonts);
}

OSStatus MeasureConvertedUTF8(short *width, FontInfo * maxFont,
			      Boolean * rightToLeft, ScriptFontInfo fonts)
{
	GrafPtr port;
	short fontID, oldFont, oldSize;
	OSStatus statusCode;
	ItemCount run;
	ByteOffset offset;
	ByteCount runLen;
	ScriptCode script;
	FontInfo fInfo;
	Boolean lineRight;
	UniGlobalsState uniState;

	NoPurgeUniGlobals(&uniState);
	if (width != nil)
		*width = 0;
	if (maxFont != nil)
		Zero(*maxFont);
	if (rightToLeft != nil)
		*rightToLeft = false;
	GetPort(&port);
	oldFont = GetPortTextFont(port);
	oldSize = GetPortTextSize(port);
	for (statusCode = noErr, run = 0;
	     statusCode == noErr && run < uGlobals.convRunsCount; ++run) {
		statusCode =
		    RevertTextEncodingToScriptInfo((*uGlobals.
						    convRuns)[run].
						   textEncoding, &script,
						   nil, GlobalTemp);
		if (statusCode == noErr) {
			if (GlobalTemp[0] != 0) {
				GetFNum(GlobalTemp, &fontID);
			} else {
				fontID = GetIntlFont(script, fonts);
			}
			TextFont(fontID);
			TextSize(GetIntlSize(script, fonts));
			if (maxFont != nil) {
				GetFontInfo(&fInfo);
				if (fInfo.ascent > maxFont->ascent) {
					maxFont->ascent = fInfo.ascent;
				}
				if (fInfo.descent > maxFont->descent) {
					maxFont->descent = fInfo.descent;
				}
				if (fInfo.widMax > maxFont->widMax) {
					maxFont->widMax = fInfo.widMax;
				}
				if (fInfo.leading > maxFont->leading) {
					maxFont->leading = fInfo.leading;
				}
			}
			if (run == 0) {
				lineRight =
				    !(!(Boolean)
				      GetScriptVariable(script,
							smScriptRight));
				if (rightToLeft != nil)
					*rightToLeft = lineRight;
			}
			if (width != nil) {
				offset = (*uGlobals.convRuns)[run].offset;
				runLen =
				    ((run ==
				      uGlobals.convRunsCount -
				      1) ? uGlobals.
				     convTextLen : (*uGlobals.
						    convRuns)[run +
							      1].offset) -
				    offset;
				HLock((Handle) uGlobals.convText);
				if ((**uGlobals.order)[run] ==
				    uGlobals.convRunsCount - 1) {
					runLen =
					    MyVisibleLength(*uGlobals.
							    convText +
							    offset, runLen,
							    lineRight ?
							    rightCaret :
							    leftCaret);
				}
				*width +=
				    TextWidth(*uGlobals.convText, offset,
					      runLen);
				HUnlock((Handle) uGlobals.convText);
			} else if (maxFont == nil) {
				break;
			}
		}
	}
	TextSize(oldSize);
	TextFont(oldFont);
	ResetUniGlobals(&uniState);
	return statusCode;
}

OSStatus ConvertUTF8Text(BytePtr theText, ByteCount bufLen)
{
	OSStatus statusCode = noErr;
	long textSize, runCount;
	ByteCount inputRead, readTotal, outputLen, tempLen;
	ItemCount numRuns;
	OptionBits flags;
	ScriptCode script;
	UniGlobalsState uniState;
	BytePtr tempText;
	DECLARE_UPP(MyRlDirProc, StyleRunDirection);

	INIT_UPP(MyRlDirProc, StyleRunDirection);
	flags =
	    kUnicodeKeepInfoMask | kUnicodeUseFallbacksMask |
	    kUnicodeDefaultDirectionMask | kUnicodeLooseMappingsMask |
	    kUnicodeTextRunMask;
	if (uGlobals.hasTextRunFlag)
		flags |=
		    /*kUnicodeKeepSameEncodingMask | */
		    kUnicodeTextRunHeuristicsMask;
	if ((*uGlobals.convText != nil) && (*uGlobals.convRuns != nil)
	    && (*uGlobals.order != nil) && (*uGlobals.origText != nil)
	    && (bufLen == uGlobals.origTextLen)
	    && (!memcmp(*uGlobals.origText, theText, bufLen))) {
		return noErr;
	}

	NoPurgeUniGlobals(&uniState);

	uGlobals.convTextLen = 0;
	uGlobals.convRunsCount = 0;
	readTotal = 0;

	if ((uGlobals.tecVersion == 0x0180) && (GetOSVersion() >= 0x1020)) {
		// The 10.2 version of the TEC is converting 16-bit Unicode, not UTF-8
		// So, convert the UTF-8 to 16-bit Unicode and put it into uGlobals.origText temporarily
		tempText = theText;
		tempLen =
		    UTF8CharCount(tempText, bufLen) * sizeof(UniChar);
		statusCode =
		    EnsureHandleSize((Handle) uGlobals.origText, tempLen);
		if (statusCode > noErr) {
			UniChar *curUniChar =
			    (UniChar *) * (uGlobals.origText);
			UniCharCount c = tempLen / sizeof(UniChar);

			while (c--) {
				*curUniChar++ =
				    UTF8ToUniChar(tempText, &tempText);
			}
			statusCode = noErr;
		}
	} else {
		tempLen = GoodUTF8Len(theText, bufLen);
	}
	runCount =
	    EnsureHandleSize((Handle) uGlobals.convRuns,
			     2 * sizeof(TextEncodingRun)) /
	    sizeof(TextEncodingRun);
	textSize = EnsureHandleSize((Handle) uGlobals.convText, bufLen);

	if (!statusCode && ((statusCode = runCount) > noErr)
	    && ((statusCode = textSize) > noErr))
		do {
			HLock((Handle) uGlobals.convText);
			HLock((Handle) uGlobals.convRuns);

			if ((uGlobals.tecVersion == 0x0180)
			    && (GetOSVersion() >= 0x1020)) {
				HLock((Handle) uGlobals.origText);
				tempText = *(uGlobals.origText);
			} else {
				tempText = theText;
			}


			statusCode =
			    ConvertFromUnicodeToTextRun(uGlobals.UTF8ToMac,
							tempLen -
							readTotal,
							(ConstUniCharArrayPtr)
							(tempText +
							 readTotal), flags,
							0, nil, nil, nil,
							textSize -
							uGlobals.
							convTextLen,
							&inputRead,
							&outputLen,
							*uGlobals.
							convText +
							uGlobals.
							convTextLen,
							runCount -
							uGlobals.
							convRunsCount,
							&numRuns,
							*uGlobals.
							convRuns +
							uGlobals.
							convRunsCount);

			HUnlock((Handle) uGlobals.convText);
			HUnlock((Handle) uGlobals.convRuns);
			if ((uGlobals.tecVersion == 0x0180)
			    && (GetOSVersion() >= 0x1020))
				HUnlock((Handle) uGlobals.origText);

			uGlobals.convTextLen += outputLen;
			uGlobals.convRunsCount += numRuns;
			readTotal += inputRead;

			if (statusCode == kTECArrayFullErr) {
				SetHandleSize((Handle) uGlobals.convRuns,
					      ++runCount *
					      sizeof(TextEncodingRun));
				goto CheckError;
			} else if (statusCode ==
				   kTECOutputBufferFullStatus) {
			      ResizeBuffer:
				SetHandleSize((Handle) uGlobals.convText,
					      textSize += 1 K);
			      CheckError:
				if ((statusCode = MemError()) != noErr) {
					break;
				}
			} else {
				if (statusCode == kTECUsedFallbacksStatus)
					statusCode = noErr;
				break;
			}

		} while (true);

	if ((statusCode == noErr)
	    &&
	    ((statusCode =
	      EnsureHandleSize((Handle) uGlobals.order,
			       sizeof(FormatOrder) *
			       uGlobals.convRunsCount)) > noErr)) {
		if (uGlobals.convRunsCount == 1) {
			(**uGlobals.order)[0] = 0;
			statusCode = noErr;
		} else {
			statusCode =
			    RevertTextEncodingToScriptInfo((*uGlobals.
							    convRuns)[0].
							   textEncoding,
							   &script, nil,
							   nil);
			if (statusCode == noErr) {
				HLock((Handle) uGlobals.order);
				GetFormatOrder(**uGlobals.order, 0,
					       uGlobals.convRunsCount - 1,
					       !(!(Boolean)
						 GetScriptVariable(script,
								   smScriptRight)),
					       MyRlDirProcUPP,
					       (Ptr) uGlobals.convRuns);
				HUnlock((Handle) uGlobals.order);
			}
		}
	}
	if ((statusCode == noErr)
	    &&
	    ((textSize =
	      EnsureHandleSize((Handle) uGlobals.origText,
			       bufLen)) > noErr)) {
		uGlobals.origTextLen = bufLen;
		BMD(theText, *uGlobals.origText, bufLen);
	} else {
		uGlobals.origTextLen = 0L;
	}
	ClearIntlConverterContext(uGlobals.UTF8ToMac, true);
	ResetUniGlobals(&uniState);
	return statusCode;
}

pascal Boolean MyRlDirProc(short theFormat, void *dirParam)
{
	ScriptCode script;
	TextEncodingRun **convRuns = (TextEncodingRun **) dirParam;

	return ((RevertTextEncodingToScriptInfo
		 ((*convRuns)[theFormat].textEncoding, &script, nil,
		  nil) == noErr)
		&& (Boolean) GetScriptVariable(script, smScriptRight));
}

long EnsureHandleSize(Handle h, Size s)
{
	OSErr errCode;
	long s2;

	if (h == nil)
		return nilHandleErr;

	if (*h == nil)
		ReallocateHandle(h, s);
	else {
		s2 = InlineGetHandleSize(h);
		if (s2 < s) {
			SetHandleSize(h, s);
		} else
			s = s2;
	}
	errCode = MemError();
	return (errCode == noErr) ? s : errCode;
}

void NoPurgeUniGlobals(UniGlobalsState * uniState)
{
	uniState->xState = HGetState((Handle) uGlobals.origText);
	HNoPurge((Handle) uGlobals.convText);
	uniState->tState = HGetState((Handle) uGlobals.convText);
	HNoPurge((Handle) uGlobals.convText);
	uniState->cState = HGetState((Handle) uGlobals.convRuns);
	HNoPurge((Handle) uGlobals.convRuns);
	uniState->oState = HGetState((Handle) uGlobals.order);
	HNoPurge((Handle) uGlobals.order);
}

void ResetUniGlobals(UniGlobalsState * uniState)
{
	HSetState((Handle) uGlobals.origText, uniState->xState);
	HSetState((Handle) uGlobals.convText, uniState->tState);
	HSetState((Handle) uGlobals.convRuns, uniState->cState);
	HSetState((Handle) uGlobals.order, uniState->oState);
}

/* Pass -1 for inOff if inText is a pointer instead of a handle. Pass either charset or encoding. */
OSStatus InternetToUTF8Text(StringPtr charset, TextEncoding encoding,
			    ConstTextPtr * inText, long inOff,
			    ByteCount inLen, AccuPtr a, Boolean hint)
{
	OSStatus err;
	Byte iState, aState;
	ByteCount usedILen, usedOLen;
	UniChar hintChar;
	ConstTextPtr textPtr;

	err =
	    UpdateTECConverter(&uGlobals.internetToUTF8, charset, encoding,
			       &uGlobals.internetToUTF8Encoding,
			       UTF8_ENCODING, &uGlobals.maclatin1,
			       &hintChar);
	if (err && err != kTECNoConversionPathErr)
		return err;
	if (hint && hintChar) {
		unsigned char hint8[MAX_UTF8_CHAR_LEN];
		ByteCount len;

		len = UniCharToUTF8(hintChar, hint8, sizeof(hint8));
		AccuAddPtr(a, hint8, len);
	}
	if (err == kTECNoConversionPathErr) {
		if (inOff >= 0) {
			err =
			    AccuAddFromHandle(a, (Handle) inText, inOff,
					      inLen);
		} else {
			err = AccuAddPtr(a, (Ptr) inText, inLen);
		}
		return err;
	}
	aState = HGetState(a->data);
	if (inOff >= 0) {
		iState = HGetState((Handle) inText);
	} else {
		textPtr = (ConstTextPtr) inText;
	}
	do {
		if ((err =
		     AccuEnsureSize(a, MAX(inLen, 1 K), 1 K)) == noErr) {
			HLock(a->data);
			if (inOff >= 0) {
				textPtr = LDRef(inText) + inOff;
			}
			err =
			    MyTECConvertText(uGlobals.internetToUTF8,
					     textPtr, inLen, &usedILen,
					     *(a->data) + a->offset,
					     a->size - a->offset,
					     &usedOLen,
					     uGlobals.maclatin1);
			if (inOff >= 0) {
				HSetState((Handle) inText, iState);
			}
			HSetState(a->data, aState);
			if (!err || (err == kTECOutputBufferFullStatus)) {
				a->offset += usedOLen;
				inLen -= usedILen;
				if (inOff >= 0) {
					inOff += usedILen;
				} else {
					textPtr += usedILen;
				}
			}
			if (!err) {
				do {
					if ((err =
					     AccuEnsureSize(a,
							    kUnicodeMinBufSize,
							    1 K)) ==
					    noErr) {
						HLock(a->data);
						err =
						    TECFlushText(uGlobals.
								 internetToUTF8,
								 *(a->
								   data) +
								 a->offset,
								 a->size -
								 a->offset,
								 &usedOLen);
						HSetState(a->data, aState);
						if (!err
						    || (err ==
							kTECOutputBufferFullStatus))
						{
							a->offset +=
							    usedOLen;
						}
					}
				} while (err ==
					 kTECOutputBufferFullStatus);
			}
		}
	} while (err == kTECOutputBufferFullStatus);
	TECClearConverterContextInfo(uGlobals.internetToUTF8);
	return err;
}

OSStatus GetUnicodeHint(TextEncoding encoding, StringPtr lang,
			UniChar * hint)
{
	ScriptCode script;
	OSStatus err;

// LocaleStringToLangAndRegionCodes(string, &langCode, nil);

	if (uGlobals.tecVersion < 0x0150)
		return kTextUnsupportedEncodingErr;

	err = NearestMacTextEncodings(encoding, &encoding, nil);
	if (err)
		return err;

	err = RevertTextEncodingToScriptInfo(encoding, &script, nil, nil);
	if (!err)
		switch (script) {
		case smJapanese:
			*hint = kUnicodeSourceHintJapanese;
			break;
		case smTradChinese:
			*hint = kUnicodeSourceHintChineseTraditional;
			break;
		case smKorean:
			*hint = kUnicodeSourceHintKorean;
			break;
		case smSimpChinese:
			*hint = kUnicodeSourceHintChineseSimplified;
			break;
		default:
			*hint = kUnicodeSourceHintDefault;
		}
	return err;
}

OSStatus CreateIntlConverter(IntlConverter * converter,
			     TextEncoding encoding)
{
	OSStatus err;

	converter->uniCount = 0L;
	converter->lastCharScript = smSystemScript;
	converter->lastCharType = 0;
	converter->flags = kUnicodeDefaultDirectionMask;
	converter->uniHandle = NuHTempBetter(129);
	if (!converter->uniHandle) {
		Log(-1, "\pCreate uniHandle failed");
		return MemError();
	}
	converter->inToUnicode = converter->unicodeToMac = 0;
	if (uGlobals.tecVersion == 0) {
		err = noErr;
	} else {
		converter->inToUnicodeEncoding =
		    (encoding ==
		     kTextEncodingUnknown) ? CreateSystemRomanEncoding() :
		    encoding;
		err = UpdateIntlConverter(converter, nil);
		if (err)
			goto HDispose;
		err =
		    CreateUnicodeToTextRunInfo(0, nil,
					       &converter->unicodeToMac);
	}
	if (err) {
		Log(-1L, "\pUnicode To Mac failed");
		TECDisposeConverter(converter->inToUnicode);
	      HDispose:
		ZapHandle(converter->uniHandle);
	}
	return err;
}

OSStatus ConvertIntlText(IntlConverter * converter, StringPtr inText,
			 ByteCount * inLen, StringPtr outText,
			 ByteCount * outLen, TextEncoding * encoding,
			 WordSpaceEnum addSpace, ByteOffset * spaceOffset)
{
	OptionBits flags;
	OSStatus err;
	ByteCount origILen, origOLen, usedOLen, actILen, usedUniLen,
	    uniBufSize;
	Boolean full;
	ItemCount runCount, spaceCount;
	ByteOffset iSpaceOffset, oSpaceOffset;
	TextEncodingRun encodingRun;
//      ScriptCode script;
	Byte hState;

	if (converter == nil) {
		return paramErr;
	}

	if (converter->unicodeToMac == nil) {
		*encoding = kTextEncodingUnknown;
		if (spaceOffset != nil) {
			*spaceOffset = ULONG_MAX;
		}
		if (addSpace != dontAddWordSpace) {
			*outText++ = ' ';
			--*outLen;
		}
		err =
		    (*inLen <=
		     *outLen) ? noErr : kTECOutputBufferFullStatus;
		*inLen = *outLen = MIN(*outLen, *inLen);
		BMD(inText, outText, *outLen);
		if (addSpace != dontAddWordSpace) {
			++*outLen;
		}
		if (converter->table != NO_TABLE) {
			TransLitRes(outText, *outLen, converter->table);
		}
		return err;
	}

	hState = HGetState(converter->uniHandle);
	HLock(converter->uniHandle);
	uniBufSize = GetHandleSize(converter->uniHandle);

	origILen = *inLen;
	origOLen = *outLen;
	*outLen = *inLen = usedOLen = 0L;

	flags =
	    converter->
	    flags | kUnicodeUseFallbacksMask | kUnicodeLooseMappingsMask |
	    kUnicodeTextRunMask | kUnicodeKeepInfoMask;
	if (uGlobals.hasTextRunFlag)
		flags |=
		    /*kUnicodeKeepSameEncodingMask | */
		    kUnicodeTextRunHeuristicsMask;

	do {
		if (addSpace != dontAddWordSpace) {
			spaceCount = 1;
			iSpaceOffset =
			    (converter->uniCount * sizeof(UniChar));
			(*converter->uniHandle)[converter->uniCount++] =
			    kUnicodeSpaceChar;
			addSpace = dontAddWordSpace;
		} else {
			spaceCount = 0;
		}
		if ((*inLen <= origILen || origILen == 0L)
		    && (converter->uniCount <
			((uniBufSize -
			  sizeof(UniChar)) / sizeof(UniChar)))) {
			UniChar *curBuf =
			    (*converter->uniHandle) + converter->uniCount;
			ByteCount curSize =
			    uniBufSize -
			    ((converter->uniCount + 1) * sizeof(UniChar));

			if (converter->inToUnicode == nil) {
				actILen = usedUniLen =
				    MIN(origILen - *inLen, curSize);
				if (usedUniLen > curSize)
					err = kTECOutputBufferFullStatus;
				else
					err = noErr;
				BMD(inText + *inLen, curBuf, usedUniLen);
			} else if ((origILen != 0L)
				   && (origILen != *inLen)) {
				if (converter->iso2022)
					CleanISO2022(inText + *inLen,
						     origILen - *inLen,
						     converter);
				err =
				    MyTECConvertText(converter->
						     inToUnicode,
						     inText + *inLen,
						     origILen - *inLen,
						     &actILen, curBuf,
						     curSize, &usedUniLen,
						     converter->maclatin1);
			} else {
				actILen = 0L;
				err =
				    TECFlushText(converter->inToUnicode,
						 curBuf, curSize,
						 &usedUniLen);
			}

			full = ((err == kTECOutputBufferFullStatus)
				|| (err == kTECBufferBelowMinimumSizeErr));
			if (!full && err)
				break;

			converter->uniCount +=
			    (usedUniLen / sizeof(UniChar));
			*inLen += actILen;
		} else {
			full = (origILen != 0L);
		}
	      ConvertUnicode:
		err =
		    ConvertFromUnicodeToTextRun(converter->unicodeToMac,
						converter->uniCount *
						sizeof(UniChar),
						*converter->uniHandle,
						flags, spaceCount,
						&iSpaceOffset, &spaceCount,
						&oSpaceOffset,
						origOLen - *outLen,
						&usedUniLen, &usedOLen,
						outText + *outLen, 1,
						&runCount, &encodingRun);
		*encoding = encodingRun.textEncoding;
		switch (err) {
		case kTECUsedFallbacksStatus:
			err = noErr;
			goto MoveIt;
		case kTECBufferBelowMinimumSizeErr:
			err = kTECOutputBufferFullStatus;
		case kTECOutputBufferFullStatus:
		case kTECArrayFullErr:
		case noErr:
		      MoveIt:
			if ((runCount > 0) && (usedOLen > 0)) {
/*					RevertTextEncodingToScriptInfo(encodingRun.textEncoding, &script, nil, nil);
					if(spaceCount != 0) {
						short charType;
						
						charType = CharacterType(outText, 0, script);
					}
					
					converter->lastCharScript = script;
					converter->lastCharType = CharacterType(outText, *outLen - 1, script);
*/
				if (spaceOffset != nil) {
					if (spaceCount != 0) {
						*spaceOffset =
						    oSpaceOffset + *outLen;
					} else {
						*spaceOffset = ULONG_MAX;
					}
				}
			}
			converter->uniCount -=
			    usedUniLen / sizeof(UniChar);
			if (converter->uniCount > 0) {
				BMD((*converter->uniHandle) +
				    (usedUniLen / sizeof(UniChar)),
				    *converter->uniHandle,
				    converter->uniCount * sizeof(UniChar));
				if ((usedOLen == 0)
				    && (err == kTECArrayFullErr))
					goto ConvertUnicode;
			}

			*outLen += usedOLen;
			break;
		default:
			if (converter->inToUnicode == nil) {
				converter->uniCount -=
				    (actILen / sizeof(UniChar)) +
				    spaceCount;
			}
		}

	} while (full && !err);
	if (!err && (inText == (StringPtr) - 1L)
	    && (converter->unicodeToMac != nil)) {
		ClearIntlConverterContext(converter->unicodeToMac, false);
	}
	HSetState(converter->uniHandle, hState);
	return err;
}

void DisposeIntlConverter(IntlConverter converter)
{
	if (converter.unicodeToMac != nil) {
		TECDisposeConverter(converter.inToUnicode);
		DisposeUnicodeToTextRunInfo(&converter.unicodeToMac);
		converter.inToUnicode = converter.unicodeToMac = nil;
	}
	ZapHandle(converter.uniHandle);
}

OSStatus UpdateIntlConverter(IntlConverter * converter, StringPtr charset)
{
	OSStatus err;
	UniChar hint;

	if (uGlobals.tecVersion == 0) {
		if ((charset == nil) || (charset[0] == 0)
		    || EqualStrRes(charset, MIME_MAC)) {
			converter->table = NO_TABLE;
			return noErr;
		} else {
			converter->table = FindMIMECharset(charset);
			return (converter->table ==
				NO_TABLE) ? kTextUnsupportedEncodingErr :
			    noErr;
		}
	}

	err =
	    UpdateTECConverter(&converter->inToUnicode, charset,
			       charset ? kTextEncodingUnknown : converter->
			       inToUnicodeEncoding,
			       &converter->inToUnicodeEncoding,
			       CreateTextEncoding
			       (kTextEncodingUnicodeDefault,
				kTextEncodingDefaultVariant,
				kUnicode16BitFormat),
			       &converter->maclatin1, &hint);
	if (!err) {
		TextEncodingBase base;

		if (hint) {
			(*converter->uniHandle)[converter->uniCount++] =
			    hint;
		}
		base = GetTextEncodingBase(converter->inToUnicodeEncoding);
		converter->iso2022 = (base <= kTextEncodingISO_2022_KR
				      && base >= kTextEncodingISO_2022_JP);
		converter->inDouble = false;
	} else {
		converter->iso2022 = false;
	}

	// If the transfer code has already taken a stab at
	// transliteration, it will pass in a charset of
	// orginal-charset/new-charset.  Right now, we take
	// this as a signal we'd better not do further transliteration
	// At some future date, we may be able to undo some or all
	// of the damage that was done if this original transliteration
	// proved to be in error
	if (charset && PIndex(charset, '/'))
		converter->alreadyTransliterated = true;
	return err;
}

OSStatus EncodingPlusPeteStyle(TextEncoding encoding, PETEStyleEntry * pse,
			       ScriptCode * outScript)
{
	ScriptCode script, oldScript;
	LangCode lang;
	OSStatus err;

	if (uGlobals.tecVersion == 0)
		return noErr;
	err =
	    RevertTextEncodingToScriptInfo(encoding, &script, &lang,
					   GlobalTemp);
	if (err)
		return err;

	if (GlobalTemp[0] != 0) {
		GetFNum(GlobalTemp, &pse->psStyle.textStyle.tsFont);
		pse->psStyle.textStyle.tsLang = langUnspecified;
		script = smUninterp;
	} else {
		Boolean defaultFont =
		    (pse->psStyle.textStyle.tsFont == kPETEDefaultFont)
		    || (pse->psStyle.textStyle.tsFont ==
			kPETEDefaultFixed);

		if (pse->psStyle.textStyle.tsLang == langUnspecified) {
			oldScript = smUninterp;
		} else if (!defaultFont) {
			oldScript =
			    FontToScript(pse->psStyle.textStyle.tsFont);
			pse->psStyle.textStyle.tsLang =
			    (short) GetScriptVariable(oldScript,
						      smScriptLang);
		} else {
			TextEncoding tempEncoding;

			err =
			    UpgradeScriptInfoToTextEncoding
			    (kTextScriptDontCare,
			     pse->psStyle.textStyle.tsLang,
			     kTextRegionDontCare, nil, &tempEncoding);
			if (err)
				return err;

			err =
			    RevertTextEncodingToScriptInfo(tempEncoding,
							   &oldScript, nil,
							   nil);
			if (err)
				return err;
		}
		if (lang != kTextLanguageDontCare) {
			if (pse->psStyle.textStyle.tsLang != lang) {
				pse->psStyle.textStyle.tsLang = lang;
				if (!defaultFont) {
					pse->psStyle.textStyle.tsFont =
					    kPETEDefaultFont;
				}
			}
		} else {
			if (script != oldScript) {
				pse->psStyle.textStyle.tsLang =
				    (short) GetScriptVariable(script,
							      smScriptLang);
				if (!defaultFont) {
					pse->psStyle.textStyle.tsFont =
					    kPETEDefaultFont;
				}
			}
		}
	}
	if (outScript != nil)
		*outScript = script;
	return noErr;
}

OSStatus ClearIntlConverterContext(UnicodeToTextRunInfo converter,
				   Boolean utf8)
{
	UniChar twoChars[2];
	unsigned char twoUTF8Chars[2 * MAX_UTF8_CHAR_LEN];
	void *iUnicodeStr;
	unsigned char buffer[kUnicodeMinBufSize];
	TextEncodingRun encodingRun;
	ByteCount oLen, iLen;
	ItemCount runs;
	OptionBits flags;

	flags =
	    kUnicodeUseFallbacksMask | kUnicodeDefaultDirectionMask |
	    kUnicodeLooseMappingsMask | kUnicodeTextRunMask;
	if (uGlobals.hasTextRunFlag)
		flags |=
		    /*kUnicodeKeepSameEncodingMask | */
		    kUnicodeTextRunHeuristicsMask;
	if (utf8
	    && (uGlobals.tecVersion != 0x0180
		|| (GetOSVersion() < 0x1020))) {
		iLen =
		    UniCharToUTF8(kUnicodeSourceHintDefault,
				  &twoUTF8Chars[0], sizeof(twoUTF8Chars));
		iLen +=
		    UniCharToUTF8(kUnicodeSpaceChar, &twoUTF8Chars[iLen],
				  sizeof(twoUTF8Chars) - iLen);
		iUnicodeStr = &twoUTF8Chars[0];
	} else {
		twoChars[0] = kUnicodeSourceHintDefault;
		twoChars[1] = kUnicodeSpaceChar;
		iUnicodeStr = &twoChars[0];
		iLen = 2 * sizeof(UniChar);
	}
	return ConvertFromUnicodeToTextRun(converter, iLen, iUnicodeStr,
					   flags, 0, nil, 0, nil,
					   sizeof(buffer), &iLen, &oLen,
					   buffer, 1, &runs, &encodingRun);
}

OSStatus UpdateTECConverter(TECObjectRef * converter, StringPtr charset,
			    TextEncoding newEncoding,
			    TextEncoding * fromEncoding,
			    TextEncoding toEncoding, Boolean * maclatin1,
			    UniChar * hint)
{
	TECObjectRef tempConverter;
	OSStatus err = noErr;
	UPtr spot = nil;

	*maclatin1 = false;
	if (charset && charset[0] != 0) {
		Str255 string;

		spot = &charset[1];
		PTokenPtr(&charset[1], charset[0], string, &spot, "*");
		if (EqualStrRes(string, MIME_ISO_LATIN1)) {

			newEncoding =
			    DefaultEncoding(kTextEncodingWindowsLatin1);
		} else {
			TextEncoding tempEncoding;

			err =
			    TECGetTextEncodingFromInternetName
			    (&tempEncoding, string);
			if (err) {
				if (newEncoding == kTextEncodingUnknown)
					return err;
			} else
				newEncoding = tempEncoding;

			// Work around another Apple bug
			if ((uGlobals.tecVersion >= 0x0170 && uGlobals.tecVersion < 0x0190) && ((newEncoding & 0x0000FF00) == 0x00000100))	// It's Unicode
				newEncoding &= 0xFFFFFF00;	// Make it default Unicode instead of explicit version
		}
	}
	if (!*converter || newEncoding != *fromEncoding) {
		if (toEncoding != kTextEncodingUnknown)
			err =
			    TECCreateConverter(&tempConverter, newEncoding,
					       toEncoding);
		else
			err =
			    TECCreateOneToManyConverter(&tempConverter,
							newEncoding, 0,
							nil);
		if (!err) {
			if (*converter)
				TECDisposeConverter(*converter);
			*converter = tempConverter;
			*fromEncoding = newEncoding;
		}
	}
	if (!err && hint) {
		Str255 string;

		if (GetUnicodeHint
		    (newEncoding,
		     (spot
		      && PTokenPtr(&charset[1], charset[0], string, &spot,
				   "*")) ? string : nil, hint))
			*hint = 0;
	}
	return err;
}

ByteCount UniCharToUTF8(UniChar c, unsigned char *utf8, ByteCount maxLen)
{
	/* Bonehead! */
	if (maxLen == 0)
		return 0;

	/* Plain ASCII just return it */
	if (c <= 0x7F) {
		*utf8++ = c;
		/* Null terminate if there's room */
		if (maxLen > 1)
			*utf8 = 0;
		return 1;
	} else {
		unsigned long lc;
		long mask = 0xFFFFFFE0;
		ByteCount len;

		/*
		 *      Count the bytes needed; trim off low six bits and see if the
		 *      remainder will fit in the first byte
		 */
		for (lc = c, len = 2; (lc >>= 6) & mask;
		     mask >>= 1, ++len);

		/* More than can fit? */
		if (len > maxLen)
			return 0;

		/* Fill it in from last byte to first */
		utf8 += len;
		/* Null terminate if there's room */
		if (len < maxLen)
			*utf8 = 0;
		lc = c;
		do {
			/* Drop in the low six bits and set the high bit */
			*--utf8 = ((lc & 0x3F) | 0x80);
			lc >>= 6;
		} while (lc & mask);
		/* Put the rest of the bits in the first byte */
		*--utf8 = (((mask << 1) & 0x000000FF) | lc);
		return len;
	}
}

UniChar UTF8ToUniChar(unsigned char *utf8, unsigned char **next)
{
	UniChar r;

	/* Plain ASCII just return it */
	if (*utf8 <= 0x7F) {
		r = *utf8++;
	} else {
		int c;
		unsigned char b, m;

		/*
		 *      Count how many bytes there are; check how many high bits are set
		 *      and that'll be the byte count. Also, form the mask to grab the
		 *      low bits out of the first byte.
		 */
		for (c = 0, b = *utf8, m = 0x3F; ((b <<= 1) & 0x80); ++c)
			m >>= 1;

		/* Grab the low bits of the first byte */
		r = *utf8++ & m;
		while (c--) {
			/* Grab the low six bits of each byte and shift them into r */
			r <<= 6;
			r |= (*utf8++ & 0x3F);
		}
	}
	if (next)
		*next = utf8;
	return r;
}

UniCharCount UTF8CharCount(BytePtr utf8, ByteCount bufLen)
{
	UniCharCount count = 0;
	UTF8Char b;
	ByteCount tempLen;

	while (bufLen) {
		tempLen = 1;
		if ((b = *utf8++) > 0x7F) {
			while ((b <<= 1) & 0x80) {
				++tempLen;
			}
			if (tempLen > bufLen)
				break;

			utf8 += tempLen;
		}
		bufLen -= tempLen;
		++count;
	}
	return count;
}

/* Return the length of the buffer that contains legal UTF-8 (remove trailing partials) */
ByteCount GoodUTF8Len(BytePtr utf8, ByteCount bufLen)
{
	UTF8Char b;
	ByteCount newLen = 0, tempLen;

	while (bufLen) {
		// It's at least 1 if it's US-ASCII
		tempLen = 1;

		// If it's not US-ASCII....
		if ((b = *utf8++) > 0x7F) {
			/*
			 *      The first byte of any UTF-8 sequence has the high bit set followed by
			 *      a bit set for each subsequent byte in the sequence. So, count how many
			 *      bits are set from high to low (not including the first one) and that
			 *      tells how many bytes follow this one
			 */
			while ((b <<= 1) & 0x80) {
				++tempLen;
			}

			/*
			 *      If the first byte said there are more than the remaining buffer in the
			 *      sequence, don't add that to the length.
			 */
			if (tempLen > bufLen)
				break;

			// Move past this sequence of UTF-8 bytes (we already moved past the first)
			utf8 += tempLen - 1;
		}
		// Add the length used to the total, and subtract it from what's left
		newLen += tempLen;
		bufLen -= tempLen;
	}
	return newLen;
}

OSErr AccuEnsureSize(AccuPtr a, long len, long incr)
{
	if (a->data && (a->size - a->offset >= len))
		return noErr;
	if (!a->data) {
		a->offset = 0;
		a->size = MAX(len, 1 K);
		a->data = NuHTempBetter(a->size);
	} else {
		a->size += len + incr;
		SetHandleBig_(a->data, a->size);
	}
	return (a->err = MemError());
}

short GetIntlFont(ScriptCode script, ScriptFontInfo fonts)
{
	if (fonts[script].fontID > applFont)
		return fonts[script].fontID;
	else
		return
		    LoWord(GetScriptVariable(script, smScriptAppFondSize));
}

short GetIntlSize(ScriptCode script, ScriptFontInfo fonts)
{
	if (fonts[script].fontSize > 0)
		return fonts[script].fontSize;
	else
		return
		    HiWord(GetScriptVariable(script, smScriptAppFondSize));
}

void CleanISO2022(TextPtr text, long len, IntlConverter * converter)
{
//      state = 2022AnyByte;

	for (; --len >= 0; ++text) {
/*		switch state :
			case 2022AnyByte :
				if(text[offset] == 0x1B) {
					state = 2022Esc;
					break;
				}
				if(text[offset] <= 0x7F)
					break;
*/

		if (*text == 0x1B) {
			if (++text, --len >= 0) {
				--len;
				switch (*text++) {
				case '$':
					converter->inDouble = true;
					break;
				case '(':
					converter->inDouble = false;
				}
			}
		} else {
			if (converter->inDouble && (len > 0)) {
				--len;
				if ((*text++ < 0x21)
				    || (*(text - 1) > 0x7E)
				    || (*text < 0x21) || (*text > 0x7E)) {
					*(text - 1) = 0x21;
					*text = 0x21;
				}
			} else if (*text > 0x7F) {
				*text = '?';
			}
		}
	}
}

Boolean Find2047(UPtr chars, long len, long *ewOff, long *ewTextOff,
		 long *ewEndOff, long *ewWSLen, Boolean * qp);

OSStatus PeteInsertHeader(PETEHandle pte, long *pOff, Handle text,
			  long len, long tOff)
{
	return InsertIntlHeaders(text, len, tOff, nil,
				 kTextEncodingUnknown, pte, pOff);
}

/*
 *	Pass encoding and a or if you want UTF8 in an accumulator; pass pte and pOff if you want a PETE handle
 *	Pass -1 for tOff if Text is pointer instead of handle
 */
OSStatus InsertIntlHeaders(Handle text, long len, long tOff, AccuPtr a,
			   TextEncoding encoding, PETEHandle pte,
			   long *pOff)
{
	long curOff,		// Current offset into the text
	 ewOff,			// Offset of the encoded word
	 ewTextOff,		// Offset of the text portion of the encoded word
	 ewEndOff,		// Offset of the last question mark in the encoded word
	 ewWSLen,		// Whitespace after the encoded word
	 ewLastWSLen,		// Whitespace after previous encoded word
	 skipOff;		// Stuff that looked like an encoded word but wasn't
	OSStatus err = noErr;
	Boolean qp;		// TRUE=quoted-printable; FALSE=base64
	Ptr textPtr;		// temp pointer
	long inTOff;		// Offset into the text passed in

	inTOff = tOff;
	// < 0 means it was a pointer
	if (tOff < 0)
		tOff = 0;
	if (len == 0)
		return noErr;
	// Current offset into the text
	curOff = tOff;
	ewWSLen = 0;
	skipOff = 0;

	/*
	 *      Go through a single header. Find an encoded word. Output what came before the
	 *      encoded word. Convert the encoded word. Output the converted stuff.
	 */
	do {
		// Deref if it's a handle
		textPtr = inTOff < 0 ? (Ptr) text : *text;

		// Save whether there was any trailing WS on the last encoded word
		ewLastWSLen = ewWSLen;
		if (Find2047
		    (textPtr + curOff + skipOff,
		     len - ((curOff + skipOff) - tOff), &ewOff, &ewTextOff,
		     &ewEndOff, &ewWSLen, &qp)) {
			// If we immediately find an encoded word after the last one, ignore the WS
			if (ewOff == ewLastWSLen) {
				curOff += ewOff;
				ewTextOff -= ewOff;
				ewEndOff -= ewOff;
				ewOff = 0;
			}
		}
		// If there was some text before the encoded word (or there was some skipped text)...
		if (ewOff + skipOff > 0) {
			// Add that text to the accumulator or document
			if (a) {
				if (encoding == kTextEncodingUnknown)
					encoding =
					    CreateSystemRomanEncoding();
				err =
				    InternetToUTF8Text(nil, encoding,
						       ((Ptr) text) +
						       (inTOff <
							0 ? curOff : 0),
						       inTOff <
						       0 ? inTOff : curOff,
						       ewOff + skipOff, a,
						       false);
			} else {
				if (inTOff < 0) {
					err =
					    PETEInsertTextPtr(PETE, pte,
							      *pOff,
							      textPtr +
							      curOff,
							      ewOff +
							      skipOff,
							      nil);
				} else {
					err =
					    PETEInsertTextHandle(PETE, pte,
								 *pOff,
								 text,
								 ewOff +
								 skipOff,
								 curOff,
								 nil);
				}
			}
		}

		if (!err) {
			// If we need to keep track of the PETE offset, add what we just inserted
			if (pOff && *pOff != -1)
				*pOff += ewOff + skipOff;
			// Move curOff past what we just inserted
			curOff += ewOff + skipOff;
			skipOff = 0;
			// Make the offsets relative to curOff
			ewTextOff -= ewOff;
			ewEndOff -= ewOff;
			ewOff = 0;

			// At the end?
			if (ewEndOff != 0) {
				Str255 word;

				// Get the text of the encoded word and un-quoted-printable or un-base64
				textPtr = inTOff < 0 ? (Ptr) text : *text;
				MakePStr(word,
					 textPtr + curOff + ewTextOff,
					 (ewEndOff - 2) - ewTextOff);
				if ((qp && (PseudoQP(word), true)
				     || !DecodeB64String(word))) {
					Str31 charset;
					long iOff, oOff;

					// Get the charset name
					MakePStr(charset,
						 textPtr + curOff + 2,
						 ewTextOff - 5);
					if (EqualStrRes
					    (charset,
					     UNKNOWN_CHARSET_NAME))
						GetRString(charset,
							   UNSPECIFIED_CHARSET);

					if (a) {
						// Adding the text to the accumulator using the charset
						iOff = a->offset;
						if (InternetToUTF8Text
						    (charset,
						     kTextEncodingUnknown,
						     &word[1], -1L,
						     word[0], a,
						     false) != noErr) {
							a->offset = iOff;
							goto BadConversion;
						} else {
							curOff += ewEndOff;
						}
					}
					// If we add to the document, we update the converter using the charset
					else if (UpdateIntlConverter
						 (&uGlobals.quickConverter,
						  charset) == noErr) {
						// Adding the text to the document with the converter
						if ((iOff = *pOff) < 0L)
							PeteGetTextAndSelection
							    (pte, nil,
							     &iOff, nil);
						err =
						    PeteInsertIntlText(pte,
								       pOff,
								       &word
								       [1],
								       -1L,
								       word
								       [0],
								       &uGlobals.
								       quickConverter,
								       kTextEncodingUnknown,
								       false,
								       true);
						if (!err) {
							curOff += ewEndOff;
						} else
						    if (EncodingError(err))
						{
							if ((oOff =
							     *pOff) < 0L)
								PeteGetTextAndSelection
								    (pte,
								     nil,
								     &oOff,
								     nil);
							PeteDelete(pte,
								   iOff,
								   oOff);
							goto BadConversion;
						}
					} else
						goto BadConversion;
				} else {
				      BadConversion:
					/*
					 *      Couldn't convert, so add the entire encoded word to the skip so that
					 *      it can be added before the next encoded word.
					 */
					curOff -= ewLastWSLen;
					skipOff =
					    ewEndOff + ewWSLen +
					    ewLastWSLen;
					ewWSLen = 0;
				}
			}
		}
	} while ((!err || EncodingError(err)) && curOff < tOff + len);
	return err;
}

Boolean Find2047(UPtr chars, long len, long *ewOff, long *ewTextOff,
		 long *ewEndOff, long *ewWSLen, Boolean * qp)
{
	UPtr q[4];		// Pointers to the 4 questions marks in the encoded word
	UPtr end = chars + len;	// Past the end of the text
	UPtr spot = chars;	// Start out at the start of the text
	long ewEncOff;		// Offset of the encoding type (Q or B)

	while (true) {
		// If we don't have enough text left for an encoded word, break out
		if (end - spot < 8) {
			*ewOff = *ewTextOff = *ewEndOff = len;
			*ewWSLen = 0;
			break;
		}
		// Encoded words start with "=?"
		if ((*spot++ == '=') && (*spot == '?')) {
			int i;

			// Find 3 more questions marks, break out if we hit the end                     
			for (q[0] = spot++, i = 1; i < 4; ++i) {
				q[i] = q[i - 1];
				while (q[i] < end && *++(q[i]) != '?');
				if (q[i] >= end)
					break;
			}

			// If we found 4 question marks and the next thing is an "="...
			if ((i == 4) && (q[3] < end) && (q[3][1] == '=')) {
				// Encoded word starts one before the 1st '?'
				*ewOff = q[0] - chars - 1;
				// The encoding char is one after the 2nd '?'
				ewEncOff = q[1] - chars + 1;
				// The text is one after the 3rd '?'
				*ewTextOff = q[2] - chars + 1;
				// The end is two after the last '?'
				*ewEndOff = q[3] - chars + 2;

				// Get the length of WS after the encoded word to ignore
				for (spot = q[3] + 2; spot < end; ++spot) {
					switch (*spot) {
					case ' ':
					case '\t':
					case '\012':
					case '\014':
					case '\015':
						continue;
					}
					break;
				}
				*ewWSLen = spot - (q[3] + 2);
				break;
			}
		}
	}

	// See if we've got quoted-printable or base64
	if (*ewTextOff - ewEncOff == 2) {
		switch (chars[ewEncOff]) {
		default:
			goto badEncoding;
		case 'Q':
		case 'q':
			*qp = true;
			break;
		case 'B':
		case 'b':
			*qp = false;
		}
	} else {
	      badEncoding:
		ewOff = ewEndOff;
	}
	return (*ewOff < *ewEndOff);
}

OSErr PeteSetIntlText(PETEHandle pte, Handle text, long start, long end,
		      IntlConverter * converter, TextEncoding encoding)
{
	OSErr err;
	long offset = -1;

	PeteDelete(pte, 0, 0x7fffffff);
	PeteScroll(pte, 0, pseCenterSelection);
	err =
	    PeteInsertIntlText(pte, &offset, text, start, end, converter,
			       encoding, false, true);
	PeteSetURLRescan(pte, 0);
	PeteNickScan(pte);
	return err;
}

OSErr PeteInsertIntlText(PETEHandle pte, long *offset, Handle text,
			 long start, long end, IntlConverter * converter,
			 TextEncoding encoding, Boolean needSpace,
			 Boolean flush)
{
	PETEStyleEntry pse;
	OSErr err = noErr;
	Str255 outText;
	long outLen, inLen, usedInLen = 0;
	Byte hState;
	Ptr textPtr;
	Boolean bufRep, hand;

	if (converter == nil) {
		converter = &uGlobals.quickConverter;
		err =
		    UpdateTECConverter(&converter->inToUnicode, nil,
				       encoding,
				       &converter->inToUnicodeEncoding,
				       CreateTextEncoding
				       (kTextEncodingUnicodeDefault,
					kTextEncodingDefaultVariant,
					kUnicode16BitFormat),
				       &converter->maclatin1, nil);
		if (err)
			return err;
		err = UpdateIntlConverter(converter, nil);
		if (err)
			return err;
	}

	PETEGetStyle(PETE, pte, kPETECurrentStyle, nil, &pse);
	hand = ((text != nil) && (start >= 0));
	if (hand) {
		hState = HGetState(text);
	} else {
		textPtr = (Ptr) text;
	}
	flush = flush && (text != nil);
	do {
		do {
			bufRep = false;
			inLen = end - usedInLen;
			if (start >= 0)
				inLen -= start;
			outLen = sizeof(outText);
			if (hand)
				textPtr = LDRef(text) + start;
			err =
			    ConvertIntlText(converter,
					    textPtr ? textPtr +
					    usedInLen : (Ptr) - 1L, &inLen,
					    outText, &outLen, &encoding,
					    needSpace ?
					    addWordSpaceConditional :
					    dontAddWordSpace, nil);
			if (hand)
				HSetState(text, hState);
			if (outLen == 0)
				break;
			needSpace = false;
			switch (err) {
			case kTECOutputBufferFullStatus:
			case kTECArrayFullErr:
				bufRep = true;
			case noErr:
				err =
				    EncodingPlusPeteStyle(encoding, &pse,
							  nil);
				if (err == noErr) {
					**Pslh = pse;
					err =
					    PETEInsertTextPtr(PETE, pte,
							      offset ==
							      nil ?
							      kPETECurrentSelection
							      : *offset,
							      outText,
							      outLen,
							      Pslh);
					if ((err == noErr)
					    && (offset != nil)
					    && (*offset >= 0L))
						*offset += outLen;
				}
			}
			usedInLen += inLen;
		} while ((err == noErr) && bufRep);
		textPtr = nil;
		hand = false;
		start = end = usedInLen = 0L;
	} while ((err == noErr) && !(flush = !flush));
	if (!err && (converter->unicodeToMac != nil))
		ClearIntlConverterContext(converter->unicodeToMac, false);
	return err;
}

Boolean EncodingError(OSStatus err)
{
	switch (err) {
	case kTextUnsupportedEncodingErr:
	case kTextMalformedInputErr:
	case kTextUndefinedElementErr:
	case kTECPartialCharErr:
	case kTECUnmappableElementErr:
	case kTECIncompleteElementErr:
		return true;
	default:
		return false;
	}
}

Boolean HasUnicode()
{
	return uGlobals.tecVersion != 0;
}

// End buffer *before* the CR
OSStatus MessageToUTF8(Handle inText, long inOff, ByteCount inLen,
		       AccuPtr a, int *context)
{
	static Str31 charset;
	static Str15 dirs[umDirCount] = { 0 };
	Str15 temp;
	ByteOffset next = inOff, last;
	long offset;
	Byte hState;
	TextEncoding encoding;
	OSStatus err = noErr;

	if (inLen == 0)
		return noErr;

	if (!HasUnicode())
		return paramErr;

	err =
	    UpgradeScriptInfoToTextEncoding(FontToScript(FontID),
					    kTextLanguageDontCare,
					    kTextRegionDontCare, nil,
					    &encoding);
	if (err)
		return err;

	if (dirs[umFlowedDir][0] == 0)
		GetRString(dirs[umFlowedDir], EnrichedStrn + enXFlowed);
	if (dirs[umCharsetDir][0] == 0)
		GetRString(dirs[umCharsetDir], EnrichedStrn + enXCharset);
	if (dirs[umHtmlDir][0] == 0)
		GetRString(dirs[umHtmlDir], EnrichedStrn + enXHTML);
	if (dirs[umRichDir][0] == 0)
		GetRString(dirs[umRichDir], EnrichedStrn + enXRich);
	hState = HGetState(inText);

	while ((err == noErr) && (next < inOff + inLen)) {
		switch (*context) {
		case umHeaderState:
			charset[0] = 0;
			temp[0] = temp[1] = 13;
			offset =
			    SearchPtrPtr(temp, 2, LDRef(inText), next,
					 inLen - (next - inOff), false,
					 false, nil);
			HSetState(inText, hState);
			last = next;
			if (offset < 0) {
				next = inLen + inOff;
			} else {
				next += offset;
				*context = umTextState;
			}
			err =
			    InsertIntlHeaders(inText, last, next - last, a,
					      encoding, nil, nil);
			break;
		case umTextState:
			charset[0] = 0;
			temp[0] = 13;
			temp[1] = '<';
			offset =
			    SearchPtrPtr(temp, 2, LDRef(inText), next,
					 inLen - (next - inOff), false,
					 false, nil);
			HSetState(inText, hState);
			last = next;
			if (offset < 0) {
				next = inLen + inOff;
				offset = 0;
			} else {
				int i;

				// Move by the return
				next += offset + 1;
				for (i = umFlowedDir; i < umDirCount; ++i) {
					if ((inLen - (next - inOff) >
					     dirs[i][0])
					    &&
					    (PPtrFindSub
					     (dirs[i], *inText + next + 1,
					      dirs[i][0]) != nil)) {
						break;
					}
				}
				if (i < umDirCount) {
					*context = i;
					offset = next + dirs[i][0] + 1;
					offset =
					    ParseCharset(*inText + offset,
							 inLen - (offset -
								  inOff),
							 charset, i);
				} else
					offset = 0;
			}
			err =
			    InternetToUTF8Text(nil, encoding, inText, last,
					       next - last, a, false);
			next += offset;
			break;
		case umFlowedState:
		case umCharsetState:
		case umHtmlState:
		case umRichState:
			temp[0] = 3;
			temp[1] = 13;
			temp[2] = '<';
			temp[3] = '/';
			PCat(temp, dirs[*context]);
			temp[++temp[0]] = '>';
			offset =
			    SearchPtrPtr(&temp[1], temp[0], LDRef(inText),
					 next, inLen - (next - inOff),
					 false, false, nil);
			HSetState(inText, hState);
			last = next;
			if (offset < 0) {
				next = inLen + inOff;
				offset = 0;
			} else {
				// Move by the return
				next += offset + 1;
				*context = umTextState;
				offset = temp[0];
			}
			err =
			    InternetToUTF8Text(charset, encoding, inText,
					       last, next - last, a,
					       false);
			next += offset;
		}
	}
	return err;
}

long ParseCharset(Ptr textPtr, long len, PStr charset, umDirectives dir)
{
	static Str15 paramDir = { 0 }, charsetAttr = { 0 };
	Ptr tempPtr;
	long offset;
	Boolean done = false;

	if (paramDir[0] == 0)
		ComposeRString(paramDir, MIME_RICH_ON,
			       EnrichedStrn + enParam);
	if (charsetAttr[0] == 0) {
		charsetAttr[++charsetAttr[0]] = ' ';
		PCatR(charsetAttr, HTMLAttributeStrn + htmlCharsetAttr);
		charsetAttr[++charsetAttr[0]] = '=';
	}

	charset[0] = 0;
	offset = 0;
	switch (dir) {
	case umHtmlDir:
		tempPtr = memchr(textPtr, '>', len);
		if (tempPtr) {
			offset = (tempPtr - textPtr) + 1;
			tempPtr =
			    PPtrFindSub(charsetAttr, textPtr, offset);
			if (tempPtr) {
				len -= textPtr - tempPtr;
				while ((len > 0) && (charset[0] < 32)
				       &&
				       ((charset[charset[0] + 1] =
					 *++tempPtr) != '"')) {
					--len;
					++charset[0];
				}
				done = (*tempPtr == '"');
			}
		}
		break;
	case umRichDir:
		while ((len > 0) && (*textPtr != '>')) {
			--len;
			++textPtr;
			++offset;
		}
		--len;
		++textPtr;
		++offset;
		if ((len > paramDir[0])
		    && PPtrFindSub(paramDir, textPtr, paramDir[0])) {
			textPtr += paramDir[0];
			offset += paramDir[0];
			len -= paramDir[0];
			while ((len > 0) && (charset[0] < 32)
			       && ((charset[charset[0] + 1] = *++textPtr)
				   != '<')) {
				--len;
				++offset;
				++charset[0];
			}
			if ((*textPtr++ == '<') && (*textPtr++ == '/')
			    && !strincmp(textPtr, &paramDir[2],
					 paramDir[0] - 1)) {
				done = true;
				offset += paramDir[0] + 1;
			}
		}
		break;
	case umFlowedDir:
	case umCharsetDir:
		while ((len > 0) && (*textPtr != ' ') && (*textPtr != '>')) {
			--len;
			++textPtr;
			++offset;
		}
		++offset;
		if (*textPtr == ' ') {
			while ((len > 0) && (charset[0] < 32)
			       && ((charset[charset[0] + 1] = *++textPtr)
				   != '>')) {
				--len;
				++offset;
				++charset[0];
			}
		}
		done = (*textPtr == '>');
	}
	if (!done) {
		offset = 0;
		charset[0] = 0;
	}
	return offset;
}

OSStatus PeteGetUTF8Text(PETEHandle pte, long offset, long iLen,
			 long *iUsed, UPtr out, long oLen, long *oUsed)
{
	long runLen, usedIn, usedOut;
	OSStatus err = noErr;
	PETEStyleInfo style;
	LangCode lang;
	StringPtr fontName;
	UHandle text;
	TextEncoding encoding;
	Byte hState;

	*iUsed = 0;
	*oUsed = 0;
	runLen = 0;
	err = PETEGetRawText(PETE, pte, &text);
	if (err)
		return err;

	hState = HGetState(text);
	if (iLen > PeteLen(pte) - offset)
		iLen = PeteLen(pte) - offset;
	while ((iLen > 0) && (oLen > 0) && !err) {
		if (runLen == 0) {
			err =
			    PeteGetStyleRun(pte, offset + *iUsed, &runLen,
					    &style,
					    peFontValid | peLangValid);
			if (err)
				return err;
		}
		if (runLen > iLen)
			runLen = iLen;

		if ((style.textStyle.tsFont != kPETEDefaultFont)
		    && (style.textStyle.tsFont != kPETEDefaultFixed)) {
			lang = kTextLanguageDontCare;
			fontName = GlobalTemp;
			GetFontName(style.textStyle.tsFont, fontName);
		} else {
			lang = style.textStyle.tsLang;
			fontName = nil;
		}
		err =
		    UpgradeScriptInfoToTextEncoding(kTextScriptDontCare,
						    lang,
						    kTextRegionDontCare,
						    fontName, &encoding);
		if (err)
			return err;

		err =
		    UpdateTECConverter(&uGlobals.internetToUTF8, nil,
				       encoding,
				       &uGlobals.internetToUTF8Encoding,
				       UTF8_ENCODING, &uGlobals.maclatin1,
				       nil);
		if (err)
			return err;

		HLock(text);
		err =
		    MyTECConvertText(uGlobals.internetToUTF8,
				     *text + offset + *iUsed, runLen,
				     &usedIn, out + *oUsed, oLen, &usedOut,
				     uGlobals.maclatin1);
		HSetState(text, hState);
		if (err && err != kTECOutputBufferFullStatus)
			return err;

		oLen -= usedOut;
		*oUsed += usedOut;
		iLen -= usedIn;
		*iUsed += usedIn;
		runLen -= usedIn;
	}
	return err;
}

OSErr PeteGetStyleRun(PETEHandle pte, long offset, long *len,
		      PETEStyleInfoPtr style, long validBits)
{
	OSErr err;
	long runLen, fullLen;
	PETEStyleEntry pse;

	*len = 0;
	fullLen = PETEGetTextLen(PETE, pte);
	err = PETEGetStyle(PETE, pte, offset, &runLen, &pse);
	if (err)
		return err;
	*style = pse.psStyle;
	runLen -= (offset - pse.psStartChar);
	while ((runLen + offset + *len) < fullLen) {
		long tempLen, diffBits;
		PETEStyleEntry pse2;

		err =
		    PETEGetStyle(PETE, pte, offset + *len + runLen,
				 &tempLen, &pse2);
		if (err)
			break;

		err =
		    PETECompareStyles(PETE, pte, &pse, &pse2, validBits,
				      false, &diffBits);
		if (err)
			break;

		if (diffBits)
			break;

		*len += runLen;
		runLen = tempLen - pse2.psStartChar;
	}
	*len += runLen;
	return err;
}

TextEncoding CreateSystemRomanEncoding()
{
	TextEncoding encoding;

	if (noErr !=
	    UpgradeScriptInfoToTextEncoding(smRoman,
					    (LangCode)
					    LoWord(GetScriptVariable
						   (smRoman,
						    smScriptLang)),
					    kTextRegionDontCare, nil,
					    &encoding))
		return DefaultEncoding(kTextEncodingMacRoman);
	else
		return encoding;
}

OSStatus MyTECConvertText(TECObjectRef encodingConverter,
			  ConstTextPtr inputBuffer,
			  ByteCount inputBufferLength,
			  ByteCount * actualInputLength,
			  TextPtr outputBuffer,
			  ByteCount outputBufferLength,
			  ByteCount * actualOutputLength,
			  Boolean maclatin1)
{
	OSStatus err;

	if (!maclatin1) {
		err =
		    TECConvertText(encodingConverter, inputBuffer,
				   inputBufferLength, actualInputLength,
				   outputBuffer, outputBufferLength,
				   actualOutputLength);
		if (err == kTECUsedFallbacksStatus)
			err = noErr;
	} else {
		if (actualInputLength)
			*actualInputLength = 0;
		if (actualOutputLength)
			*actualOutputLength = 0;
		do {
			ByteCount curLen, usedILen, usedOLen;

			curLen =
			    MIN(inputBufferLength, sizeof(GlobalTemp));
			BMD(inputBuffer, GlobalTemp, curLen);
			TransLitRes(GlobalTemp, curLen, TRANS_IN_TABL);
			err =
			    TECConvertText(encodingConverter, GlobalTemp,
					   curLen, &usedILen, outputBuffer,
					   outputBufferLength, &usedOLen);
			if (err == kTECUsedFallbacksStatus)
				err = noErr;
			if (actualInputLength)
				*actualInputLength += usedILen;
			if (actualOutputLength)
				*actualOutputLength = usedOLen;
			inputBufferLength -= usedILen;
			outputBufferLength -= usedOLen;
			inputBuffer += usedILen;
			outputBuffer += usedOLen;
		} while (inputBufferLength && !err);
	}
	return err;
}

long UnicodeMappingCount(TextEncoding encoding)
{
	OptionBits matchFilter;
	UnicodeMapping matchMapping;
	ItemCount foundCount;

	matchMapping.unicodeEncoding =
	    DefaultEncoding(kTextEncodingUnicodeDefault);
	matchMapping.otherEncoding = encoding;
	matchMapping.mappingVersion = kUnicodeUseLatestMapping;

	matchFilter = (kUnicodeMatchUnicodeBaseMask |
		       kUnicodeMatchUnicodeVariantMask |
		       kUnicodeMatchUnicodeFormatMask |
		       kUnicodeMatchOtherBaseMask |
		       kUnicodeMatchOtherFormatMask);

	return CountUnicodeMappings(matchFilter, &matchMapping,
				    &foundCount) == noErr ? foundCount : 0;
}

/************************************************************************
 * SniffAndConvertHandleToRoman - figure out what sort of text is in a handle and Romanize it
 ************************************************************************/
OSErr SniffAndConvertHandleToRoman(Handle * hp)
{
	OSErr err;
	uLong snifferCount = 0;
	uLong textSize = GetHandleSize(*hp);
	unsigned char utf8Magic[] = { 0xef, 0xbb, 0xbf };

	// if it's unicode, it may have a byte order mark at the start, which is fffe (intel), feff (network), or efbbbf (utf-8)
	// Handle these specially
	if (textSize > 2
	    && (*(uShort *) ** hp == 0xfffe
		|| *(uShort *) ** hp == 0xfeff))
		return ConvertHandleToRoman(hp,
					    CreateTextEncoding
					    (kTextEncodingUnicodeDefault,
					     kTextEncodingDefaultVariant,
					     kUnicode16BitFormat), 0);
	else if (textSize > 3 && !memcmp(**hp, utf8Magic, 3))
		return ConvertHandleToRoman(hp,
					    CreateTextEncoding
					    (kTextEncodingUnicodeDefault,
					     kTextEncodingDefaultVariant,
					     kUnicodeUTF8Format), 3);

	// if it's ascii now, leave it
	if (!AnyFunny(*hp, 0))
		return noErr;

	// sniffers are available in tec 1.2 & up
	// don't bother sniffing an empty handle
	if (uGlobals.tecVersion >= 0x0120 && textSize)
		// count all the sniffers we can; we'll sniff for everything
		if (!TECCountAvailableSniffers(&snifferCount))
			if (snifferCount) {
				TextEncoding *encodings =
				    NuPtr(snifferCount *
					  sizeof(TextEncoding));
				uLong *errors =
				    NuPtr(snifferCount * sizeof(uLong));
				uLong *features =
				    NuPtr(snifferCount * sizeof(uLong));
				if (encodings && errors && features) {
					// ok, we have room for everything!
					TECSnifferObjectRef theSniffer;
					if (!TECGetAvailableSniffers
					    (encodings, snifferCount,
					     &snifferCount))
						if (snifferCount)
							if (!TECCreateSniffer(&theSniffer, encodings, snifferCount)) {
								// Finally, we've made the sniffer, so let's sniff!
								TECSniffTextEncoding
								    (theSniffer,
								     LDRef
								     (*hp),
								     textSize,
								     encodings,
								     snifferCount,
								     errors,
								     textSize
								     / 10,
								     features,
								     textSize
								     / 10);
								TECDisposeSniffer
								    (theSniffer);
								UL(*hp);

								// Ok, the "best" encoding will be on top.  See if it's good enough
								if (errors
								    [0] <
								    textSize
								    / 10) {
									// Now what?
									err = ConvertHandleToRoman(hp, encodings[0], 0);
								} else {
									// No.  We can't figure out what the thing is.  Bail.
									err = kTextUnsupportedEncodingErr;
								}
							}
					ZapPtr(encodings);
					ZapPtr(errors);
					ZapPtr(features);
				}
			}
	return (err);
}

/************************************************************************
 * ConvertHandleToRoman - Romanize text in a handle, if we can
 ************************************************************************/
OSErr ConvertHandleToRoman(Handle * hp, TextEncoding encoding,
			   uLong offset)
{
	TECObjectRef converter;
	OSErr err =
	    TECCreateConverter(&converter, encoding,
			       kTextEncodingMacRoman);
	uLong inLen = GetHandleSize(*hp) - offset;
	uLong outLen;
	UHandle newHandle;

	if (!err) {
		if (newHandle = NuHandle(inLen)) {
			err =
			    TECConvertText(converter, LDRef(*hp) + offset,
					   inLen, &inLen, LDRef(newHandle),
					   inLen, &outLen);
			if (err)
				ZapHandle(newHandle);
			UL(*hp);
			UL(newHandle);
		} else
			err = MemError();
		TECDisposeConverter(converter);
	}

	if (!err) {
		ZapHandle(*hp);
		SetHandleSize(newHandle, outLen);
		*hp = newHandle;
	}

	return err;
}
