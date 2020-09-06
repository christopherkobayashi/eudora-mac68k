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

#if !defined(__UNICODE_H__)
#define __UNICODE_H__

#include <TextEncodingConverter.h>
#include <UnicodeConverter.h>

#define kUnicodeSourceHintDefault				((UniChar)0xF850)
#define kUnicodeSourceHintChineseSimplified		((UniChar)0xF85C)
#define kUnicodeSourceHintChineseTraditional	((UniChar)0xF85D)
#define kUnicodeSourceHintJapanese				((UniChar)0xF85E)
#define kUnicodeSourceHintKorean				((UniChar)0xF85F)
#define kUnicodeSpaceChar						((UniChar)0x0020)
#define kUnicodeLeftToRightMark					((UniChar)0x200E)
#define kUnicodeRightToLeftMark					((UniChar)0x200F)
#define kUnicodeNonBreakingSpace				((UniChar)0x00A0)

#define kUnicodeMinBufSize						32

#define UTF8_ENCODING							CreateTextEncoding(kTextEncodingUnicodeDefault,kTextEncodingDefaultVariant,kUnicodeUTF8Format)
#define DefaultEncoding(x)						CreateTextEncoding((x),kTextEncodingDefaultVariant,kTextEncodingDefaultFormat)

typedef enum {
	addWordSpaceConditional = -1,
	dontAddWordSpace = 0,
	addWordSpaceUnconditional
} WordSpaceEnum;

typedef struct {
	TECObjectRef inToUnicode;
	TextEncoding inToUnicodeEncoding;
	UnicodeToTextRunInfo unicodeToMac;
	UniChar **uniHandle;
	UniCharCount uniCount;
	ScriptCode lastCharScript;
	short table;
	short lastCharType;
	OptionBits flags;
	Boolean iso2022;
	Boolean inDouble;
	Boolean maclatin1;
	Boolean alreadyTransliterated;
} IntlConverter;

typedef struct {
	short fontID;
	short fontSize;
} OneFontInfo;

typedef OneFontInfo ScriptFontInfo[32];

OSStatus InitUnicode();
void CleanupUnicode();
OSStatus DrawUTF8Text(BytePtr theText, ByteCount bufLen, short width, ScriptFontInfo fonts);
OSStatus MeasureUTF8Text(BytePtr theText, ByteCount bufLen, short *width, FontInfo *maxFont, Boolean *rightToLeft, ScriptFontInfo fonts);

// Pass -1 for inOff if inText is a pointer instead of a handle
OSStatus InternetToUTF8Text(StringPtr charset, TextEncoding encoding, ConstTextPtr *inText, long inOff, ByteCount inLen, AccuPtr a, Boolean hint);

// Pass encoding and a if you want UTF8 in an accumulator; pass pte and pOff if you want a PETE handle
// Pass -1 for tOff if Text is pointer instead of handle
OSStatus InsertIntlHeaders(Handle text, long len, long tOff, AccuPtr a, TextEncoding encoding, PETEHandle pte, long *pOff);

OSStatus CreateIntlConverter(IntlConverter *converter, TextEncoding encoding);
OSStatus ConvertIntlText(IntlConverter *converter, StringPtr inText, ByteCount *inLen, StringPtr outText, ByteCount *outLen, TextEncoding *encoding, WordSpaceEnum addSpace, ByteOffset *spaceOffset);
void DisposeIntlConverter(IntlConverter converter);
OSStatus UpdateIntlConverter(IntlConverter *converter, StringPtr charset);
OSStatus EncodingPlusPeteStyle(TextEncoding encoding, PETEStyleEntry *pse, ScriptCode *outScript);
OSStatus PeteInsertHeader(PETEHandle pte, long *pOff, Handle text, long len, long tOff);
OSErr PeteSetIntlText(PETEHandle pte, Handle text, long start, long end, IntlConverter *converter, TextEncoding encoding);
OSErr PeteInsertIntlText(PETEHandle pte, long *offset, Handle text, long start, long end, IntlConverter *converter, TextEncoding encoding, Boolean needSpace, Boolean flush);
Boolean EncodingError(OSStatus err);
Boolean HasUnicode();
// End buffer *before* the CR
OSStatus MessageToUTF8(Handle inText, long inOff, ByteCount inLen, AccuPtr a, int *context);
OSStatus PeteGetUTF8Text(PETEHandle pte, long offset, long iLen, long *iUsed, UPtr out, long oLen, long *oUsed);

ByteCount UniCharToUTF8(UniChar c, unsigned char *utf8, ByteCount maxLen);
UniChar UTF8ToUniChar(unsigned char *utf8, unsigned char **next);

TextEncoding CreateSystemRomanEncoding();
Boolean UTF8ToRoman(BytePtr theText, ByteCount *textLen, ByteCount bufLen);
Boolean RomanToUTF8(PStr s, long strSize);
ByteCount GoodUTF8Len(BytePtr utf8, ByteCount bufLen);
#define TrimUTF8(s) (s[0] = GoodUTF8Len(&s[1], s[0]))

OSErr SniffAndConvertHandleToRoman(Handle *hp);
OSErr ConvertHandleToRoman(Handle *hp,TextEncoding encoding,uLong offset);

#endif // __UNICODE_H__
