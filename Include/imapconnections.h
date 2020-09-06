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

/* Copyright (c) 1998 by QUALCOMM Incorporated */

//must have already included imapnetlib.h

/**********************************************************************
 *	imapconnections.h
 *
 *		This file contains the functions that manage IMAP connections
 **********************************************************************/

#ifndef IMAPCONNECTIONS_H
#define IMAPCONNECTIONS_H

// IMAPConnectionStruct - an entity to maintain an open connection to an IMAP server
typedef struct IMAPConnectionStruct IMAPConnectionStruct,
    *IMAPConnectionPtr, **IMAPConnectionHandle;
struct IMAPConnectionStruct {
	IMAPStreamPtr imapStream;	// a stream to an IMAP server
	long owner;		// the ID of personality that owns this stream

	long lastUsed;		// TickCount of when this connection was last used
	long lifeTime;		// number of seconds to keep this connection open

	Boolean inUse;		// lock
	Boolean dontReuse;	// true if this connection can't be reused.

	TaskKindEnum task;	// what this connection is being used for

	Boolean rude;		// be rude and slam the connection closed.

	IMAPConnectionHandle next;
};


// Connection pool management routines

OSErr EnsureConnectionPool(PersHandle pers);
void CheckIMAPConnections(void);
void ZapAllIMAPConnections(Boolean force);
void IMAPInvalidatePerConnections(PersHandle pers);

// Connection use
IMAPStreamPtr GetIMAPConnectionLo(TaskKindEnum forWhat, Boolean bSilent,
				  Boolean bPerformUpdate);
#define GetIMAPConnection(forWhat, bSilent) GetIMAPConnectionLo(forWhat, bSilent, true)
void CleanupConnection(IMAPStreamPtr * imapStream);
void PrepareToExpunge(IMAPStreamPtr imapStream);
void IMAPRudeConnectionClose(IMAPStreamPtr imapStream);

#endif				//IMAPCONNECTIONS_H
