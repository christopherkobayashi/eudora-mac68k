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

#ifndef RICH_H
#define RICH_H

typedef struct Accumulator *AccuPtr;

typedef struct
{
	Str31 styleName;
	PETETextStyle textStyle;
	long textValid;
	PETEParaInfo paraInfo;
	long paraValid;
	Boolean wholePara;
	Boolean formatBar;
} EuStyleSheet, *ESSPtr, **ESSHandle;
	

OSErr BuildEnriched(AccuPtr enriched,PETEHandle pte,UHandle text,long len,long offset,PETEStyleListHandle pslh,Boolean xrich);
OSErr PeteRich(PETEHandle pte,long start, long stop,Boolean unwrap);
short IncrementTextSize(short size,short increment);
Boolean SetMessRich(MessHandle messH);
typedef enum {hasNoStyle, hasOnlyExcerpt, hasTonsOCrap} StyleLevelEnum;
StyleLevelEnum HasStyles(PETEHandle pte,long from,long to,Boolean allowGraphics);
OSErr ParaIndent2Margin(PSMPtr marg,PStr string);
OSErr InsertRichLo(UHandle text,long textOffset,long textLen,long offset,Boolean headers,Boolean unwrap,PETEHandle pte,StackHandle partStack,MessHandle messH,Boolean delSP);
OSErr InsertRich(UHandle text,long textOffset,long textLen,long offset,Boolean unwrap,PETEHandle pte,StackHandle partStack,Boolean delSP);
PStr Style2String(ESSPtr ess,PStr string);
OSErr String2Style(ESSPtr ess,PStr string);
short FindSizeInc(short size);

#endif
