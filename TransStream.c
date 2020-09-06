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

#include "TransStream.h"
#define FILE_NUM 100
// Copyright � 1997 by QUALCOMM Incorporated
/**********************************************************************
 * code to support multiple simultaneous network connections
 **********************************************************************/
 

/**********************************************************************
 * NewTransStream - allocate memory for a new TransStream
 **********************************************************************/
 
OSErr NewTransStream(TransStream *newStream)
{
	OSErr err = noErr;
	
	*newStream = New(TransStreamStruct);

	if (*newStream)
	{
		WriteZero(*newStream,sizeof(TransStreamStruct));
	}
	else err = MemError();

	if (err != noErr)
	{
		WarnUser(MEM_ERR,err);
		*newStream = 0;
	}	
			
	return (err);
}

/**********************************************************************
 * ZapTransStream - clean up and destroy a transstream.
 **********************************************************************/
void ZapTransStream(TransStream *theStream)
{	
	// Disconnect the stream if we have to.
	if (DestroyTrans) DestroyTrans(*theStream);
	
	// Verify that the streams sub structures have been destroyed.
	ASSERT((*theStream)->OTTCPStream == 0);
	ASSERT((*theStream)->RcvBuffer == 0);
	
	// Now kill it.
	ZapPtr(*theStream);
	*theStream = 0;
}

/**********************************************************************
 * StartStreamAudit - start auditing a network stream
 **********************************************************************/
void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what)
{
	if (theStream)
	{
		theStream->auditType = what;
		theStream->bytesTransferred = 0;
	}
}

/**********************************************************************
 * StopStreamAudit - stop auditing a stream
 **********************************************************************/
void StopStreamAudit(TransStream theStream)
{
	if (theStream)
	{
		theStream->auditType = kAuditNothing;
	}
}

/**********************************************************************
 * ReportStreamAudit - report audit results
 **********************************************************************/
long ReportStreamAudit(TransStream theStream)
{
	long byteMe = 0;
	
	if (theStream)
	{
		byteMe = theStream->bytesTransferred;
	}
	
	return (byteMe);
}