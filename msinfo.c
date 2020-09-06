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

#include "MSInfo.h"
#define FILE_NUM 95
#pragma segment MIMEStore
OSErr MSCreateInfo(MStoreBoxHandle boxH);
OSErr MSOpenInfo(MStoreBoxHandle boxH);
OSErr MSCloseInfo(MStoreBoxHandle boxH);
OSErr MSDestroyInfo(MStoreBoxHandle boxH);
/************************************************************************
 * MSInfoFunc - dispatcher function
 ************************************************************************/
OSErr MSInfoFunc(MSSubCallEnum call,MStoreBoxHandle boxH)
{
	switch (call)
	{
		case msfcCreate: return(MSCreateInfo(boxH));
		case msfcOpen: return(MSOpenInfo(boxH));
		case msfcClose: case msfcCloseHard: return(MSCloseInfo(boxH));
		case msfcDestroy: return(MSDestroyInfo(boxH));
		case msfcFlush: return(noErr);
		default: ASSERT(0); return(fnfErr);
	}
}

/************************************************************************
 * MSCreateInfo - create and initialize the info file
 ************************************************************************/
OSErr MSCreateInfo(MStoreBoxHandle boxH)
{
	OSErr err = MSCreateSubFile(boxH,mssfInfo);
	return(err);
}

/************************************************************************
 * MSOpenInfo - open the Info database
 ************************************************************************/
OSErr MSOpenInfo(MStoreBoxHandle boxH)
{
	OSErr err = MSOpenSubFile(boxH,mssfInfo);
	
	return(err);
}

/************************************************************************
 * MSCloseInfo - close the Info database
 ************************************************************************/
OSErr MSCloseInfo(MStoreBoxHandle boxH)
{
	OSErr err = noErr;
	
	err = MSCloseSubFile(boxH,mssfInfo);
	
	return(err);
}

/************************************************************************
 * MSDestroyInfo - destroy the Info database
 ************************************************************************/
OSErr MSDestroyInfo(MStoreBoxHandle boxH)
{
	OSErr err;
	
	if (!(err = MSCloseInfo(boxH)))
		err = MSDestroySubFile(boxH,mssfInfo);
	return(err);
}