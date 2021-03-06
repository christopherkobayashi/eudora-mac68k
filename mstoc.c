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

#include <conf.h>
#include <mydefs.h>

#include "mstoc.h"
#define FILE_NUM 97
#pragma segment MIMEStore
OSErr MSCreateTOC(MStoreBoxHandle boxH);
OSErr MSOpenTOC(MStoreBoxHandle boxH);
OSErr MSCloseTOC(MStoreBoxHandle boxH);
OSErr MSDestroyTOC(MStoreBoxHandle boxH);
/************************************************************************
 * MSTOCFunc - dispatcher function
 ************************************************************************/
OSErr MSTOCFunc(MSSubCallEnum call, MStoreBoxHandle boxH)
{
	switch (call) {
	case msfcCreate:
		return (MSCreateTOC(boxH));
	case msfcOpen:
		return (MSOpenTOC(boxH));
	case msfcClose:
	case msfcCloseHard:
		return (MSCloseTOC(boxH));
	case msfcDestroy:
		return (MSDestroyTOC(boxH));
	case msfcFlush:
		return (noErr);
	default:
		ASSERT(0);
		return (fnfErr);
	}
}

/************************************************************************
 * MSCreateTOC - create and initialize the toc file
 ************************************************************************/
OSErr MSCreateTOC(MStoreBoxHandle boxH)
{
	OSErr err = MSCreateSubFile(boxH, mssfTOC);
	return (err);
}

/************************************************************************
 * MSOpenTOC - open the TOC database
 ************************************************************************/
OSErr MSOpenTOC(MStoreBoxHandle boxH)
{
	OSErr err = MSOpenSubFile(boxH, mssfTOC);

	return (err);
}

/************************************************************************
 * MSCloseTOC - close the TOC database
 ************************************************************************/
OSErr MSCloseTOC(MStoreBoxHandle boxH)
{
	OSErr err = noErr;

	err = MSCloseSubFile(boxH, mssfTOC);

	return (err);
}

/************************************************************************
 * MSDestroyTOC - destroy the TOC database
 ************************************************************************/
OSErr MSDestroyTOC(MStoreBoxHandle boxH)
{
	OSErr err;

	if (!(err = MSCloseTOC(boxH)))
		err = MSDestroySubFile(boxH, mssfTOC);
	return (err);
}
