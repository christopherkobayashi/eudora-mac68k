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

#ifndef HTML_H
#define HTML_H

enum { kDontEnsureCR = 1 << 0,
	kNoMargins = 1 << 1
};

OSErr InsertHTML(UHandle text, long *htmlOffset, long textLen,
		 long *inOffset, PETEHandle pte, long flags);
OSErr InsertHTMLLo(UHandle text, long *htmlOffset, long textLen,
		   long *inOffset, PETEHandle pte, TextEncoding encoding,
		   long flags, StackHandle partRefStack);
OSErr BuildHTML(AccuPtr html, PETEHandle pte, Handle text, long len,
		long offset, PETEStyleListHandle pslh,
		PETEParaScrapHandle paraScrap, long partID, PStr mid,
		StackHandle vfidStack, FSSpecPtr errSpec);
OSErr HTMLPreamble(AccuPtr html, StringPtr title, long id, Boolean local);
OSErr HTMLPostamble(AccuPtr html, Boolean local);
OSErr HTMLWriteForBrowser(UHandle text, long htmlOffset, long textLen,
			  short refNum);

#ifdef OLDHTML
OSErr PeteHTML(PETEHandle pte, long start, long stop, Boolean unwrap);
OSErr HTMLToken(UHandle html, short *cmdId, Boolean * neg, long *tStart,
		long *tStop, unsigned char *literal);
#endif

#endif
