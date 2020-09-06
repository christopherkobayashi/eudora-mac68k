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

#ifndef TRANSSTREAM_H
#define TRANSSTREAM_H

typedef enum
{
	kAuditNothing,
	kAuditBytesReceived,
	kAuditBytesSent
} StreamAuditTypeEnum;

// Stream info structure, contains info about a connection
typedef struct TransStreamStruct
{										   
	// stuff needed for an OT TCP conenction
	MyOTTCPStreamPtr OTTCPStream;
	
	// stuff needed for an SSL connection
 	long ESSLSetting;
// 	Ptr ESSLContext;
	SSL_CTX *ctx;
	SSL 	*ssl;
	BIO 	*bio;
	
	// stuff needed for a TCP connection
	int CharsAvail;					
	UPtr TcpBuffer;
	long TcpStream;

	// stuff needed for all conenctions
	UHandle RcvBuffer;				
	short RcvSize;
	short RcvSpot;

	Boolean BeSilent;				// set to true to avoid error messages	
	Boolean Opening;				// set to true when this connection is being opened.
	Boolean DontWait;				// set to true to return immediately if there is no data
	OSErr	streamErr;				// error specific to this stream

	unsigned long port;			// port (unused if not an IMAP or SSL stream)
	Str255 serverName;				// server name (unused if not an IMAP or SSL stream)
	Str255 localHostName;			// our name (unused if not an IMAP stream)
	
	// auditing
	StreamAuditTypeEnum auditType;	// type of audit we're performing
	long bytesTransferred;			// number of bytes sent or received during audit
} TransStreamStruct, *TransStreamPtr;

#define IsSendAudit(s) (stream && stream->auditType == kAuditBytesSent)
#define IsRecAudit(s) (stream && stream->auditType == kAuditBytesReceived)

OSErr NewTransStream(TransStream *newStream);	// create a TransStream
void ZapTransStream(TransStream *theStream);	// kill a transStream

void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what);	// start auditing a network stream
void StopStreamAudit(TransStream theStream);	// stop auditing a stream
long ReportStreamAudit(TransStream theStream);	// report audit results

#define	ShouldUseSSL(ts)	(NULL != (ts)->ctx)

#endif	//TRANSSTREAM_H