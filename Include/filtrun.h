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

#ifndef FILTRUN_H
#define FILTRUN_H

#ifdef TWO
OSErr FilterSelectedMessages(FilterKeywordEnum fType,TOCHandle tocH,FilterPBPtr fpb);
OSErr FilterMessagesFrom(FilterKeywordEnum fType,TOCHandle tocH,short startWith,FilterPBPtr fpb,Boolean noXfer);
OSErr FilterFlaggedMessages(FilterKeywordEnum fType,TOCHandle tocH,FilterPBPtr fpb);
OSErr FilterIMAPTocIncrementally(TOCHandle tocH,FilterPBPtr fpb,Boolean noXfer);
OSErr FilterMessage(FilterKeywordEnum fType,TOCHandle tocH,short sumNum);
void GenSpecWindow(CSpecHandle specList);
OSErr FilterNoteMatch(short filter,long secs);
uLong FilterLastMatchHi(short filter);
void NonSequitur(PStr subject, TOCHandle tocH, short sumNum);
void FilterPostprocess(FilterKeywordEnum fType,FilterPBPtr fpb);
void AddSpecToList(FSSpecPtr spec,CSpecHandle specList);
Boolean FilterMatchHi(short f,TOCHandle tocH,short sumNum);
#define FU_TYPE 'FU  '
#define FU_ID	1001
Boolean HaveManualFilters(void);

#ifdef BATCH_DELIVERY_ON
OSErr InitFPB(FilterPBPtr fpb,Boolean zapAddrs,Boolean listsToo);		
OSErr FilterMessageLo(FilterKeywordEnum fType,TOCHandle tocH,short sumNum,FilterPBPtr fpb,Boolean noXfer);	
#endif

#endif

#endif