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

#ifndef SEARCH_H
#define SEARCH_H

typedef struct Accumulator *AccuPtr;

// Search for a pointer in another pointer
long SearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset,
		  long inLen, Boolean sensitive, Boolean words,
		  AccuPtr cache);

// Search for a pstring in a pointer
#define SearchStrPtr(l,t,o,tLen,s,w,c)	SearchPtrPtr(l+1,*l,t,o,tLen,s,w,c)

// Search for a pointer in another pointer, using Find defaults for case-sensitivity, word-sensitivity
#define FindPtrPtr(l,lLen,t,tLen,c)	SearchPtrPtr(l,lLen,t,tLen,Sensitive,WholeWord,c)

// Search for a string in a pointer, using Find defaults
#define FindStrPtr(l,t,o,tLen,c)	FindPtrPtr(l+1,*l,t,o,tLen,c)

// Search for a string in another string
#define SearchStrStr(l,t,s,w)	SearchStrPtr(l,t+1,0,*t,s,w,nil)

// Search for a string in another string using Find defaults
#define FindStrStr(l,t)	SearchStrStr(l,t,Sensitive,WholeWord)

// Search for a pointer in a handle
long SearchPtrHandle(UPtr lookPtr, short lookLen, UHandle textHandle,
		     long offset, Boolean sensitive, Boolean words,
		     AccuPtr cache);

// Search for a pointer in a handle, using find defaults
#define FindPtrHandle(l,lLen,t,o,c)	SearchPtrHandle(l,lLen,t,o,Sensitive,WholeWord,c)

// Search for a string in a handle
#define SearchStrHandle(l,t,o,s,w,c)	SearchPtrHandle(l+1,*l,t,o,s,w,c)

// Search for a string in a handle, using find defaults
#define FindStrHandle(l,t,o,c)	SearchStrHandle(l,t,o,Sensitive,WholeWord,c)

OSErr BulkSearch(PStr string, FSSpecPtr spec, long *offset,
		 AccuPtr allOffsets);

#endif
