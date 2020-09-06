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

#include "msmaildb.h"
#include "mstore.h"
#pragma segment MIMEStore
OSErr MSCreateMailDB(MStoreBoxHandle boxH);
OSErr MSOpenMailDB(MStoreBoxHandle boxH);
OSErr MSCloseMailDB(MStoreBoxHandle boxH, Boolean force);
OSErr MSDestroyMailDB(MStoreBoxHandle boxH);
OSErr MSFlushMailDB(MStoreBoxHandle boxH);

#define FILE_NUM 98
/************************************************************************
 * MSMailDBFunc - dispatcher function
 ************************************************************************/
OSErr MSMailDBFunc(MSSubCallEnum call, MStoreBoxHandle boxH)
{
	switch (call) {
	case msfcCreate:
		return (MSCreateMailDB(boxH));
	case msfcOpen:
		return (MSOpenMailDB(boxH));
	case msfcClose:
		return (MSCloseMailDB(boxH, False));
	case msfcCloseHard:
		return (MSCloseMailDB(boxH, True));
	case msfcDestroy:
		return (MSDestroyMailDB(boxH));
	case msfcFlush:
		return (noErr);
	default:
		ASSERT(0);
		return (fnfErr);
	}
}

/************************************************************************
 * MSCreateMailDB - create and initialize a maildb file
 ************************************************************************/
OSErr MSCreateMailDB(MStoreBoxHandle boxH)
{
	OSErr err = MSCreateSubFile(boxH, mssfMailDB);

	(*boxH)->mailDBHeader.magic = kMStoreMailDBMagic;
	MStoreCurrentVersion(&(*boxH)->mailDBHeader.highVersion);
	MStoreCurrentVersion(&(*boxH)->mailDBHeader.lastVersion);
	(*boxH)->subs[mssfMailDB].dirty = True;

	return (err);
}

/************************************************************************
 * MSOpenMailDB - open the maildb file
 ************************************************************************/
OSErr MSOpenMailDB(MStoreBoxHandle boxH)
{
	OSErr err;

	if ((*boxH)->subs[mssfMailDB].refN)
		return (noErr);

	err = MSOpenSubFile(boxH, mssfMailDB);

	return (err);
}

/************************************************************************
 * MSCloseMailDB - close the maildb file
 ************************************************************************/
OSErr MSCloseMailDB(MStoreBoxHandle boxH, Boolean force)
{
	OSErr err = noErr;

	if ((*boxH)->subs[mssfMailDB].refN) {
		// write changes
		err = MSFlushMailDB(boxH);
		if (err && force)
			err = 0;	// ignore errors if force

		// close it up
		err = MSCloseSubFile(boxH, mssfMailDB);
	}

	return (err);
}

/************************************************************************
 * MSFlushMailDB - write the mailDB file
 ************************************************************************/
OSErr MSFlushMailDB(MStoreBoxHandle boxH)
{
	MStoreBlockHeader hdr;
	MStoreMailDBHeaderPtr mdbhPtr = (MStoreMailDBHeaderPtr) & hdr;
	OSErr err = noErr;

	if (!(*boxH)->subs[mssfMailDB].dirty)
		return (noErr);

	// flush dirty blocks

	// flush the header
	Zero(hdr);
	*mdbhPtr = (*boxH)->mailDBHeader;

	// update the version numbers && mod time
	MStoreCurrentVersion(&mdbhPtr->lastVersion);
	if (MStoreOlderVersion(&mdbhPtr->highVersion))
		MStoreCurrentVersion(&mdbhPtr->highVersion);
	mdbhPtr->modSeconds = MStoreDateTime();

	return (err);
}

/************************************************************************
 * MSDestroyMailDB - destroy the maildb file
 ************************************************************************/
OSErr MSDestroyMailDB(MStoreBoxHandle boxH)
{
	OSErr err;

	if (!(err = MSCloseMailDB(boxH, False)))
		err = MSDestroySubFile(boxH, mssfMailDB);
	return (err);
}
