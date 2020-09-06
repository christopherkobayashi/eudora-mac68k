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

/* Copyright (c) 1998 by QUALCOMM Incorporated */

//must have already included imapnetlib.h

/**********************************************************************
 *	imapauth.h
 *
 *		This file contains the functions that do IMAP authentication.
 **********************************************************************/
 
#ifndef IMAPAUTH_H
#define IMAPAUTH_H

//
// IMAP CRAM-MD5.
//

long CramMD5Authenticator(authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s, unsigned long *trial, char *user);

//			      
// KERBEROS V4
//

long KrbV4Authenticator(authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s,  unsigned long *trial, char *user);
void UsedKerberos(void);
Boolean KerberosWasUsed(void);

//
// GSSAPI (Kerberos V5)
//

long GssapiAuthenticator(authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s,  unsigned long *trial, char *user);

//
// Hack for Kfm40 bug
//
void Kfm40Hack(void);

		      
#endif //IMAPAUTH_H