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

#ifndef PH_H
#define PH_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * handling the ph panel
 **********************************************************************/

typedef enum {
	invalidLookupType = 0,
	firstDirSvcLookupType = 1,
	phLookup = firstDirSvcLookupType,
	fingerLookup,
	ldapLookup,
	genericLookup,
	firstUnusedDirSvcLookupType,
	lastDirSvcLookupType = firstUnusedDirSvcLookupType - 1
} DirSvcLookupType;


typedef enum {
	phWinLookupBtnItem = 1,
	phWinFingerBtnItem,
	phWinGlobeBtnItem,
	phWinToBtnItem,
	phWinCcBtnItem,
	phWinBccBtnItem,
	phWinQueryTextItem,
	phWinResponseTextItem,
	firstUnusedPhWinItemIndex,
	numPhWinItems = firstUnusedPhWinItemIndex - 1
} PhWinItemIndex;


DirSvcLookupType GetCurDirSvcType(void);
void SetCurDirSvcType(DirSvcLookupType lookupType);
OSErr OpenPh(PStr initialQuery);
Boolean PhNickFieldCheck (PETEHandle pte);
OSErr NetRecvLine(TransStream stream,UPtr line,long *size);
void PhFixFont(void);
void NewPhHost(void);
OSErr PhURLLookup(ProtocolEnum protocol,PStr host,PStr query,Handle *result);
OSErr InsertAddress(MyWindowPtr win,short txeIndex,PStr address);
Boolean PhCanMakeNick(void);
void PhKill(void);
void PhRememberServer(void);
#endif
