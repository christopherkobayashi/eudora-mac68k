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

OSStatus
MyConvertFromUnicodeToTextRun		(UnicodeToTextRunInfo 	iUnicodeToTextInfo,
								 ByteCount 				iUnicodeLen,
								 ConstUniCharArrayPtr 	iUnicodeStr,
								 OptionBits 			iControlFlags,
								 ItemCount 				iOffsetCount,
								 ByteOffset 			iOffsetArray[], /* can be NULL */
								 ItemCount *			oOffsetCount, /* can be NULL */
								 ByteOffset 			oOffsetArray[], /* can be NULL */
								 ByteCount 				iOutputBufLen,
								 ByteCount *			oInputRead,
								 ByteCount *			oOutputLen,
								 LogicalAddress 		oOutputStr,
								 ItemCount 				iEncodingRunBufLen,
								 ItemCount *			oEncodingRunOutLen,
								 TextEncodingRun 		oEncodingRuns[]);

OSStatus
MyTECFlushText					(TECObjectRef 			encodingConverter,
								 TextPtr 				outputBuffer,
								 ByteCount 				outputBufferLength,
								 ByteCount *			actualOutputLength);

OSStatus
MyRevertTextEncodingToScriptInfo	(TextEncoding 			iEncoding,
								 ScriptCode *			oTextScriptID,
								 LangCode *				oTextLanguageID, /* can be NULL */
								 Str255 				oTextFontname) /* can be NULL */;

short
MyCharacterType					(Ptr 					textBuf,
								 short 					textOffset,
								 ScriptCode 			script);

OSStatus
MyConvertFromUnicodeToTextRun		(UnicodeToTextRunInfo 	iUnicodeToTextInfo,
								 ByteCount 				iUnicodeLen,
								 ConstUniCharArrayPtr 	iUnicodeStr,
								 OptionBits 			iControlFlags,
								 ItemCount 				iOffsetCount,
								 ByteOffset 			iOffsetArray[], /* can be NULL */
								 ItemCount *			oOffsetCount, /* can be NULL */
								 ByteOffset 			oOffsetArray[], /* can be NULL */
								 ByteCount 				iOutputBufLen,
								 ByteCount *			oInputRead,
								 ByteCount *			oOutputLen,
								 LogicalAddress 		oOutputStr,
								 ItemCount 				iEncodingRunBufLen,
								 ItemCount *			oEncodingRunOutLen,
								 TextEncodingRun 		oEncodingRuns[])
{
	return ConvertFromUnicodeToTextRun(iUnicodeToTextInfo,
								 iUnicodeLen,
								 iUnicodeStr,
								 iControlFlags,
								 iOffsetCount,
								 iOffsetArray,
								 oOffsetCount,
								 oOffsetArray,
								 iOutputBufLen,
								 oInputRead,
								 oOutputLen,
								 oOutputStr,
								 iEncodingRunBufLen,
								 oEncodingRunOutLen,
								 oEncodingRuns);
		
}

OSStatus
MyTECFlushText					(TECObjectRef 			encodingConverter,
								 TextPtr 				outputBuffer,
								 ByteCount 				outputBufferLength,
								 ByteCount *			actualOutputLength)
{
	return TECFlushText(encodingConverter, outputBuffer, outputBufferLength, actualOutputLength);
}

OSStatus
MyRevertTextEncodingToScriptInfo	(TextEncoding 			iEncoding,
								 ScriptCode *			oTextScriptID,
								 LangCode *				oTextLanguageID, /* can be NULL */
								 Str255 				oTextFontname) /* can be NULL */
{
	return RevertTextEncodingToScriptInfo(iEncoding, oTextScriptID, oTextLanguageID, oTextFontname);
}

short
MyCharacterType					(Ptr 					textBuf,
								 short 					textOffset,
								 ScriptCode 			script)
{
	return CharacterType(textBuf, textOffset, script);
}

#define TECFlushText(a,b,c,d) MyTECFlushText(a,b,c,d)
#define ConvertFromUnicodeToTextRun(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) MyConvertFromUnicodeToTextRun(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#define RevertTextEncodingToScriptInfo(a,b,c,d) MyRevertTextEncodingToScriptInfo(a,b,c,d)
#define CharacterType(a,b,c) MyCharacterType(a,b,c)