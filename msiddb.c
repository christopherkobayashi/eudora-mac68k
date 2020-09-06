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

#include <ToolUtils.h>

#include <conf.h>
#include <mydefs.h>

#include "msiddb.h"
#define FILE_NUM 96
#pragma segment MIMEStore
OSErr MSCreateIDDB(MStoreBoxHandle boxH);
OSErr MSOpenIDDB(MStoreBoxHandle boxH);
OSErr MSCloseIDDB(MStoreBoxHandle boxH);
OSErr MSDestroyIDDB(MStoreBoxHandle boxH);

/************************************************************************
 * MSIDDBFunc - dispatcher function
 ************************************************************************/
OSErr MSIDDBFunc(MSSubCallEnum call,MStoreBoxHandle boxH)
{
	switch (call)
	{
		case msfcCreate: return(MSCreateIDDB(boxH));
		case msfcOpen: return(MSOpenIDDB(boxH));
		case msfcClose: case msfcCloseHard: return(MSCloseIDDB(boxH));
		case msfcDestroy: return(MSDestroyIDDB(boxH));
		case msfcFlush: return(noErr);
		default: ASSERT(0); return(fnfErr);
	}
}

/************************************************************************
 * MSCreateIDDB - create and initialize the iddb file
 ************************************************************************/
OSErr MSCreateIDDB(MStoreBoxHandle boxH)
{
	OSErr err = MSCreateSubFile(boxH,mssfIDDB);
	return(err);
}

/************************************************************************
 * MSOpenIDDB - open the IDDB database
 ************************************************************************/
OSErr MSOpenIDDB(MStoreBoxHandle boxH)
{
	OSErr err = MSOpenSubFile(boxH,mssfIDDB);

	return(err);
}

/************************************************************************
 * MSCloseIDDB - close the IDDB database
 ************************************************************************/
OSErr MSCloseIDDB(MStoreBoxHandle boxH)
{
	OSErr err = noErr;
	
	err = MSCloseSubFile(boxH,mssfIDDB);
	
	return(err);
}

/************************************************************************
 * MSDestroyIDDB - destroy the IDDB database
 ************************************************************************/
OSErr MSDestroyIDDB(MStoreBoxHandle boxH)
{
	OSErr err;
	
	if (!(err = MSCloseIDDB(boxH)))
		err = MSDestroySubFile(boxH,mssfIDDB);
	return(err);
}
