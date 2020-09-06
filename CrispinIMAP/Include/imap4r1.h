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

#ifndef IMAP4R1_H
#define IMAP4R1_H

/*
 * Program:	Interactive Mail Access Protocol 4rev1 (IMAP4R1) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	14 October 1988
 * Last Edited:	16 December 1996
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

/* Parameters */

//#define MAXLOGINTRIALS 1	/* maximum number of login trials */	moved to imapnetlib.h
#define IMAPLOOKAHEAD 20	/* envelope lookahead */
#define IMAPUIDLOOKAHEAD 1000	/* UID lookahead */
#define IMAPTCPPORT (long) 143	/* assigned TCP contact port */


/* Parsed reply message from imap_reply */

typedef struct imap_parsed_reply {
  char *line;			/* original reply string pointer */
  char *tag;			/* command tag this reply is for */
  char *key;			/* reply keyword */
  char *text;			/* subsequent text */
  
  Boolean fake;			/* set to true when this is a fake reply -jdboyd 01/19/00*/
} IMAPPARSEDREPLY;


#define IMAPTMPLEN 16*MAILTMPLEN
#define MAXAUTHENTICATORS 8

/* IMAP4 I/O stream local data */
	
typedef struct imap_local 
{
	IMAPPARSEDREPLY reply;						// last parsed reply
	MAILSTATUS *stat;							// status to fill in
	unsigned int imap4rev1 : 1;					// server is IMAP4rev1
	unsigned int imap4 : 1;						// server is IMAP4
	unsigned int imap2bis : 1;					// server is IMAP2bis
	unsigned int rfc1176 : 1;					// server is RFC-1176 IMAP2
	unsigned int use_status : 1;				// server has STATUS
	unsigned int use_scan : 1;					// server has SCAN
	unsigned int uidsearch : 1;					// UID searching
	unsigned int byeseen : 1;					// saw a BYE response
	unsigned int use_tls : 1;					// server has STARTTLS
	unsigned int use_auth : MAXAUTHENTICATORS;
	unsigned int uidplus : 1;					// server supports UIDPLUS
	unsigned int use_children : 1;				// server supports CHILDREN
	char *prefix;								// find prefix
	char *user;									// logged-in user
	char tmp[IMAPTMPLEN];						// temporary buffer
} IMAPLOCAL;


/* Convenient access to local data */

#define LOCAL ((IMAPLOCAL *) stream->local)

/* IMAP4rev1 level or better */

#define LEVELIMAP4rev1(stream) ((IMAPLOCAL *) stream->local)->imap4rev1


/* IMAP4 w/ STATUS level or better */

#define LEVELSTATUS(stream) (((IMAPLOCAL *) stream->local)->imap4rev1 || \
			     ((IMAPLOCAL *) stream->local)->use_status)


/* IMAP4 level or better */

#define LEVELIMAP4(stream) (((IMAPLOCAL *) stream->local)->imap4rev1 || \
			    ((IMAPLOCAL *) stream->local)->imap4)


/* IMAP4 RFC-1730 level */

#define LEVEL1730(stream) ((IMAPLOCAL *) stream->local)->imap4


/* IMAP2bis level or better */

#define LEVELIMAP2bis(stream) ((IMAPLOCAL *) stream->local)->imap2bis


/* IMAP2 RFC-1176 level or better */

#define LEVEL1176(stream) ((IMAPLOCAL *) stream->local)->rfc1176


/* Arguments to imap_send() */

typedef struct imap_argument {
  int type;			/* argument type */
  void *text;			/* argument text */
} IMAPARG;


/* imap_send() argument types */

#define ATOM 0
#define NUMBER 1
#define FLAGS 2
#define ASTRING 3
#define LITERAL 4
#define LIST 5
#define SEARCHPROGRAM 6
#define BODYTEXT 7
#define BODYPEEK 8
#define BODYCLOSE 9
#define SEQUENCE 10
#define LISTMAILBOX 11

/* Function prototypes */

DRIVER *imap_valid (char *name);
void *imap_parameters (long function,void *value);
void imap_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void imap_list (MAILSTREAM *stream,char *ref,char *pat);
void imap_lsub (MAILSTREAM *stream,char *ref,char *pat);
void imap_list_work (MAILSTREAM *stream,char *ref,char *pat,long list,
		     char *contents);
long imap_subscribe (MAILSTREAM *stream,char *mailbox);
long imap_unsubscribe (MAILSTREAM *stream,char *mailbox);
long imap_create (MAILSTREAM *stream,char *mailbox);
long imap_delete (MAILSTREAM *stream,char *mailbox);
long imap_rename (MAILSTREAM *stream,char *old,char *newname);
long imap_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2);
long imap_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *imap_open (MAILSTREAM *stream);
unsigned long imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr);
unsigned long imap_login (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr);
void *imap_challenge (void *stream,unsigned long *len);
long imap_response (void *stream,char *s,unsigned long size);
void imap_close (MAILSTREAM *stream,long options);
Boolean imap_fast (MAILSTREAM *stream,char *sequence,long flags);
Boolean imap_flags (MAILSTREAM *stream,char *sequence,long flags);
Boolean imap_flags_size (MAILSTREAM *stream,char *sequence,long flags);


// JOK - This is NEW:
ENVELOPE *imap_envelope (MAILSTREAM *stream, unsigned long msgno, long flags);
// JOK This has changed!!
IMAPBODY *imap_structure (MAILSTREAM *stream, unsigned long msgno, long flags);

//ENVELOPE *imap_structure (MAILSTREAM *stream,unsigned long msgno,IMAPBODY **body, long flags);

long imap_msgdata (MAILSTREAM *stream,unsigned long msgno,char *sequence,char *section,
		   unsigned long first,unsigned long last,STRINGLIST *lines,
		   long flags);
unsigned long imap_uid (MAILSTREAM *stream,unsigned long msgno);
unsigned long imap_msgno (MAILSTREAM *stream,unsigned long uid);
Boolean imap_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
Boolean imap_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
long imap_ping (MAILSTREAM *stream);
void imap_check (MAILSTREAM *stream);
long imap_expunge (MAILSTREAM *stream);
long imap_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long imap_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *msg);
void imap_gc (MAILSTREAM *stream,long gcflags);
void imap_gc_body (IMAPBODY *body);
Boolean imap_connected (MAILSTREAM *stream);	//JDB
unsigned long imap_rfc822size (MAILSTREAM *stream, unsigned long msgno, long flags);	//JDB

IMAPPARSEDREPLY *imap_send (MAILSTREAM *stream,char *cmd,IMAPARG *args[]);
IMAPPARSEDREPLY *imap_sout (MAILSTREAM *stream,char *tag,char *base,char **s);
long imap_soutr (MAILSTREAM *stream,char *string);
IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    char *t,unsigned long size,long wildok);
IMAPPARSEDREPLY *imap_send_literal (MAILSTREAM *stream,char *tag,char **s,
				    STRING *st);
IMAPPARSEDREPLY *imap_send_spgm (MAILSTREAM *stream,char *tag,char **s,
				 SEARCHPGM *pgm);
void imap_send_sset (char **s,SEARCHSET *set);
IMAPPARSEDREPLY *imap_send_slist (MAILSTREAM *stream,char *tag,char **s,
				  char *name,STRINGLIST *list);
void imap_send_sdate (char **s,char *name,unsigned short date);
IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag);
IMAPPARSEDREPLY *imap_parse_reply (MAILSTREAM *stream,char *text);
IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text);
long imap_OK (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_expunged (MAILSTREAM *stream,unsigned long msgno);
void imap_parse_data (MAILSTREAM *stream,unsigned long msgno,char *text,
		      IMAPPARSEDREPLY *reply);
void imap_parse_prop (MAILSTREAM *stream,MESSAGECACHE *elt,char *prop,
		      char **txtptr,IMAPPARSEDREPLY *reply);
void imap_parse_header (MAILSTREAM *stream,ENVELOPE **env,SIZEDTEXT *hdr);
void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,
			  IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr);
unsigned long imap_parse_user_flag (MAILSTREAM *stream,char *flag);
char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,GETS_DATA *md,
			 unsigned long *len);
void imap_parse_body (GETS_DATA *md,IMAPBODY **body,char *seg,char **txtptr,
		      IMAPPARSEDREPLY *reply);
void imap_parse_body_structure (MAILSTREAM *stream,IMAPBODY *body,char **txtptr,
				IMAPPARSEDREPLY *reply);
PARAMETER *imap_parse_body_parameter (MAILSTREAM *stream,char **txtptr,
				      IMAPPARSEDREPLY *reply);
void imap_parse_disposition (MAILSTREAM *stream,IMAPBODY *body,char **txtptr,
			     IMAPPARSEDREPLY *reply);
STRINGLIST *imap_parse_language (MAILSTREAM *stream,char **txtptr,
				 IMAPPARSEDREPLY *reply);
STRINGLIST *imap_parse_stringlist (MAILSTREAM *stream,char **txtptr,
				   IMAPPARSEDREPLY *reply);
void imap_parse_extension (MAILSTREAM *stream,char **txtptr,
			   IMAPPARSEDREPLY *reply);
char *imap_host (MAILSTREAM *stream);

#endif //IMAP4R1_H