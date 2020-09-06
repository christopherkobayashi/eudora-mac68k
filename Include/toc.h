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

#ifndef TOC_H
#define TOC_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1992-1995 by QUALCOMM Incorporated */

FSSpecPtr Box2TOCSpec(FSSpecPtr boxSpec,FSSpecPtr tocSpec);
TOCHandle CheckTOC(FSSpecPtr spec);
OSErr TOCDates(FSSpecPtr spec, uLong *box, uLong *res, uLong *file);
TOCHandle FindTOC(FSSpecPtr spec);
int FlushTOCs(Boolean andClose,Boolean canSkip);

#ifdef THREADING_ON
#define GetTempInTOC()	GetSpecialTOC(IN_TEMP)
#define GetTempOutTOC()	GetSpecialTOC(OUT_TEMP)
#define GetRealInTOC()	GetSpecialTOC(IN)
#define GetRealOutTOC()	GetSpecialTOC(OUT)
#define GetInTOC()	(InAThread () ? GetTempInTOC() : GetRealInTOC())
#define GetOutTOC()	(InAThread () ? GetTempOutTOC() : GetRealOutTOC())
Boolean AmTempToc(TOCHandle tocH);
#else
#define GetInTOC()	GetSpecialTOC(IN)
#define GetOutTOC()	GetSpecialTOC(OUT)
#endif

#define GetTrashTOC()	GetSpecialTOC(TRASH)

#define Win2TOC(aMyWindowPtr) 		((TOCHandle)GetMyWindowPrivateData(aMyWindowPtr))
#define	WinPtr2TOC(aWindowPtr)		((TOCHandle)GetWindowPrivateData(aWindowPtr))

short GetMailboxType(FSSpecPtr spec);
TOCHandle GetSpecialTOC(short nameId);
TOCHandle ReadTOC(FSSpecPtr spec,Boolean resource);
OSErr ReadRForkTOC(FSSpecPtr spec,TOCHandle *tocH);
OSErr ReadDForkTOC(FSSpecPtr spec,TOCHandle *tocH);
TOCHandle TOCBySpec(FSSpecPtr spec);
int WriteTOC(TOCHandle tocH);
short GetTOCK(TOCHandle tocH,uLong *usedK, uLong *totalK);
void CleanseTOC(TOCHandle tocH);
short GetTOCByFSS(FSSpecPtr specPtr,TOCHandle *tocH);
Boolean TOCUnread(TOCHandle tocH);
short TOCUnreadCount(TOCHandle tocH,Boolean recentOnly);
void NoteFreeSpace(TOCHandle tocH);
OSErr InsaneTOC(TOCHandle tocH);
OSErr KillTOC(short refN,FSSpecPtr spec);
TOCHandle FixErrantTOC(FSSpecPtr spec,TOCHandle tocH,short why);
OSErr PeekTOC(FSSpecPtr spec,TOCType *tocPart);
TOCHandle IsTOCValid(TOCHandle tocH);

#endif
