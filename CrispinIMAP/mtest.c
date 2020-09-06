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

#ifdef	IMAP

/*
 * Program:	Mail library test program
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	8 July 1988
 * Last Edited:	18 December 1996
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1996 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "mail.h"
#include "osdep.h"
#include "rfc822.h"

/* Excellent reasons to hate ifdefs, and why my real code never uses them */

#ifdef __MINT__
# define unix 1
#endif

#ifndef unix
# define unix 0
#endif

#if unix
# define UNIXLIKE 1
# define MACOS 0
# include <pwd.h>
char *getpass ();
#else
# define UNIXLIKE 0
# ifdef noErr
#  define MACOS 1
#  include <Memory.h>
# else
#  define MACOS 0
# endif
#endif
#include "misc.h"

char *curhst = NIL;		/* currently connected host */
char *curusr = NIL;		/* current login user */
char personalname[MAILTMPLEN];	/* user's personal name */

static char *hostlist[] = {	/* SMTP server host list */
  "mailhost",
  "localhost",
  NIL
};

static char *newslist[] = {	/* Netnews server host list */
  "news",
  NIL
};


/* Main program - initialization */

/* Interfaces to C-client */


void mm_searched (MAILSTREAM *stream,unsigned long number)
{
	if (stream)
	{
		// insert this UID into our results list
		OrderedInsert(stream, number, 0, 0, 0, 0, 0, 0, 0, 0);
	}
}


void mm_exists (MAILSTREAM *stream,unsigned long number)
{
}


void mm_expunged (MAILSTREAM *stream,unsigned long number)
{
}


void mm_flags (MAILSTREAM *stream,unsigned long number)
{
}


void mm_elt_flags (MAILSTREAM *stream, MESSAGECACHE *elt)
{
	if (elt && stream)
	{
		// Call a routine to do something with the flags we got ...
		OrderedInsert(stream,
						  elt->privat.uid, 
						  elt->seen,
						  elt->deleted,
						  elt->flagged,
						  elt->answered,
						  elt->draft,
						  elt->recent,
						  elt->sent,
						  elt->rfc822_size);
	}
}

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  mm_log (string,errflg);
}


void mm_list (MAILSTREAM *stream,int delimiter,char *mailbox,long attributes)
{
	if (stream)
	{
		// Call a routine to do something with the line of status we got ...
		AddMailbox(stream, mailbox, delimiter, attributes);
	}
}


void mm_lsub (MAILSTREAM *stream,int delimiter,char *mailbox,long attributes)
{
	mm_list (stream, delimiter, mailbox, attributes);
}


void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
	// Sanity.
	if ( ! (stream && mailbox && status) )
		return;

	// Copy the valuse from "status" to the stream's "MailStatus" structure.
	stream->mailboxStatus.flags = status->flags;
	stream->mailboxStatus.recent = status->recent;
	stream->mailboxStatus.messages = status->messages;
	stream->mailboxStatus.unseen = status->unseen;
	stream->mailboxStatus.uidnext = status->uidnext;
	stream->mailboxStatus.uidvalidity = status->uidvalidity;

	// if the STATUS command was done on the SELECTed mailbox, update these, too.
	if (stream->bSelected && (strcmp(stream->mailbox, mailbox)== 0))
	{
		// nmsgs
		if (status->flags & SA_MESSAGES)
			stream->nmsgs = status->messages;

		// uid_validity
		if (status->flags & SA_UIDVALIDITY)
			stream->uid_validity = status->uidvalidity;

		// recent
		if (status->flags & SA_RECENT)
			stream->recent = status->recent;
	}	
}


void mm_log (char *string,long errflg)
{
	if (errflg)
	{
		gIMAPErrorString[0] = nil;
		if (string)
		{
			gIMAPErrorString[0] = MIN(strlen(string), 255);
			strncpy(gIMAPErrorString+1, string, gIMAPErrorString[0]);
		}
	}
}

// Handle ALERT status codes -jdboyd
void mm_alert (MAILSTREAM *stream, char *string)
{
	if (string)
	{
		stream->alertStr[0] = MIN(strlen(string),sizeof(stream->alertStr));
		strncpy(stream->alertStr+1, string, stream->alertStr[0]);
	}
}

void pmm_log (Str255 pErrorString, long errflg)
{
	if (errflg)
	{
		// ensure NULL termination
		pErrorString[MIN(pErrorString[0]+1,sizeof(Str255))] = NULL;
		mm_log((char *)(pErrorString + 1), errflg);
	}
}

void mm_dlog (char *string)
{
//  puts (string);
}


void mm_login (NETMBX *mb,char *user,char *pwd,long trial)
{
	Str255 pUser, scratch;
	
	// Get the user name.
	GetPOPInfo(pUser, scratch);
	PtoCcpy(user, pUser);
	
	// collect password for current personality
	if (!*(*CurPers)->password)
	{
		// prompt for password, unless we're running from a thread.
		if (!InAThread())
		{
			if (PersFillPw(CurPers,0)==noErr)
		
			// return the password as a C String
			PtoCcpy(pwd, (*CurPers)->password);	
			
			// and don't let the password be written to the disk
			if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
		}
	}
	else PtoCcpy(pwd, (*CurPers)->password);
}


void mm_critical (MAILSTREAM *stream)	//Not Used
{
}


void mm_nocritical (MAILSTREAM *stream)
{
}


long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
#if UNIXLIKE
  kill (getpid (),SIGSTOP);
#else
  abort ();
#endif
  return NIL;
}


void mm_fatal (char *string)
{
	//log the error.  Maybe put some more information in here, too.
	if (string) ComposeLogS(LOG_ALRT,nil,"\p%p�",string);
}

#endif	//IMAP