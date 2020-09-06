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

#ifndef BUILDTOC_H
#define BUILDTOC_H

void BeautifySum(MSumPtr sum);
TOCHandle BuildTOC(FSSpecPtr spec);
TOCHandle RebuildTOC(FSSpecPtr spec,TOCHandle oldTocH, Boolean resource, Boolean tempToc);
OSErr ReadSum(MSumPtr sum,Boolean isOut,LineIOP lip,Boolean lookEnvelope);
int SumToFrom(MSumPtr sum, UPtr fromLine);
void CopyHeaderLine(UPtr to,int size,UPtr from);
OSStatus HeaderToUTF8(PStr head);
Boolean IsFromLine(UPtr line);
Boolean MessagePosition(Boolean save,MyWindowPtr win);
long FindTOCSpot(TOCHandle tocH, long length);
void BeautifyFrom(UPtr fromStr);
PStr ComputeLocalDateLo(uLong secs,long origZone,PStr dateStr);
#ifdef DEBUG
#define ComputeLocalDate(sum,dateStr) ComputeLocalDateLo(BUG7 ? (sum)->arrivalSeconds : (sum)->seconds,(sum)->origZone,dateStr)
#else
#define ComputeLocalDate(sum,dateStr) ComputeLocalDateLo((sum)->seconds,(sum)->origZone,dateStr)
#endif
void SecsToLocalDateTime(long secs, long timeZone, Str31 zone, Str255 product);
uLong BeautifyDate(UPtr dateStr,long *zoneSecs);
uLong UnixDate2Secs(PStr date);
void BeautifySubj(PStr subject,short size);
Boolean RemoveUTF8FromSum(MSumPtr sum);

#endif
