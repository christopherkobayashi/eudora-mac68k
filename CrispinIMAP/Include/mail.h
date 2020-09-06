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

#ifndef MAIL_H
#define MAIL_H

/*
 * Program:	Mailbox Access routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 November 1989
 * Last Edited:	3 March 1997
 *
 * Copyright 1997 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "imapnetlib.h"

/* Build parameters */

#define CACHEINCREMENT 250	/* cache growth increments */
#define MAILTMPLEN 1024		/* size of a temporary buffer */
#define MAXMESSAGESIZE 65000	/* MS-DOS: maximum text buffer size
				 * other:  initial text buffer size */
#define NUSERFLAGS 30		/* # of user flags (current servers 30 max) */
#define BASEYEAR 1970		/* the year time began on Unix (note: mx
				 * driver depends upon this matching Unix) */
				/* default for unqualified addresses */
#define BADHOST ".MISSING-HOST-NAME."
				/* default for syntax errors in addresses */
#define ERRHOST ".SYNTAX-ERROR."


/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#include "shortsym.h"
#endif


/* Constants */

#define NIL 0			/* convenient name */
#define T 1			/* opposite of NIL */
#define LONGT (long) 1		/* long T */

#define WARN (long) 1		/* mm_log warning type */
#define IMAP_ERROR (long) 2		/* mm_log error type */
#define PARSE (long) 3		/* mm_log parse error type */
#define BYE (long) 4		/* mm_notify stream dying */

#define DELIM '\377'		/* strtok delimiter character */


/* Bits from mail_parse_flags().  Don't change these, since the IMAPheader format
 * used by tenex, mtx, dawz, and tenexdos corresponds to these bits.
 */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8
#define fOLD 16
#define fDRAFT 32


/* Global and Driver Parameters */

	/* 0xx: driver flags */
#define ENABLE_DRIVER (long) 1
#define DISABLE_DRIVER (long) 2
	/* 1xx: c-client globals */
#define GET_DRIVERS (long) 101
#define SET_DRIVERS (long) 102
#define GET_GETS (long) 103
#define SET_GETS (long) 104
#define GET_CACHE (long) 105
#define SET_CACHE (long) 106
#define GET_SMTPVERBOSE (long) 107
#define SET_SMTPVERBOSE (long) 108
#define GET_RFC822OUTPUT (long) 109
#define SET_RFC822OUTPUT (long) 110
#define GET_READPROGRESS (long) 111
#define SET_READPROGRESS (long) 112
	/* 2xx: environment */
#define GET_USERNAME (long) 201
#define SET_USERNAME (long) 202
#define GET_HOMEDIR (long) 203
#define SET_HOMEDIR (long) 204
#define GET_LOCALHOST (long) 205
#define SET_LOCALHOST (long) 206
#define GET_SYSINBOX (long) 207
#define SET_SYSINBOX (long) 208
	/* 3xx: TCP/IP */
#define GET_OPENTIMEOUT (long) 300
#define SET_OPENTIMEOUT (long) 301
#define GET_READTIMEOUT (long) 302
#define SET_READTIMEOUT (long) 303
#define GET_WRITETIMEOUT (long) 304
#define SET_WRITETIMEOUT (long) 305
#define GET_CLOSETIMEOUT (long) 306
#define SET_CLOSETIMEOUT (long) 307
#define GET_TIMEOUT (long) 308
#define SET_TIMEOUT (long) 309
#define GET_RSHTIMEOUT (long) 310
#define SET_RSHTIMEOUT (long) 311

	/* 4xx: network drivers */
#define GET_MAXLOGINTRIALS (long) 400
#define SET_MAXLOGINTRIALS (long) 401
#define GET_LOOKAHEAD (long) 402
#define SET_LOOKAHEAD (long) 403
#define GET_IMAPPORT (long) 404
#define SET_IMAPPORT (long) 405
#define GET_PREFETCH (long) 406
#define SET_PREFETCH (long) 407
#define GET_CLOSEONERROR (long) 408
#define SET_CLOSEONERROR (long) 409
#define GET_POP3PORT (long) 410
#define SET_POP3PORT (long) 411
#define GET_UIDLOOKAHEAD (long) 412
#define SET_UIDLOOKAHEAD (long) 413
#define GET_NNTPPORT (long) 414
#define SET_NNTPPORT (long) 415
#define GET_IMAPENVELOPE (long) 416
#define SET_IMAPENVELOPE (long) 417
	/* 5xx: local file drivers */
#define GET_MBXPROTECTION (long) 500
#define SET_MBXPROTECTION (long) 501
#define GET_DIRPROTECTION (long) 502
#define SET_DIRPROTECTION (long) 503
#define GET_LOCKPROTECTION (long) 504
#define SET_LOCKPROTECTION (long) 505
#define GET_FROMWIDGET (long) 506
#define SET_FROMWIDGET (long) 507
#define GET_NEWSACTIVE (long) 508
#define SET_NEWSACTIVE (long) 509
#define GET_NEWSSPOOL (long) 510
#define SET_NEWSSPOOL (long) 511
#define GET_NEWSRC (long) 512
#define SET_NEWSRC (long) 513
#define GET_EXTENSION (long) 514
#define SET_EXTENSION (long) 515
#define GET_DISABLEFCNTLLOCK (long) 516
#define SET_DISABLEFCNTLLOCK (long) 517
#define GET_LOCKEACCESERROR (long) 518
#define SET_LOCKEACCESERROR (long) 519
#define GET_LISTMAXLEVEL (long) 520
#define SET_LISTMAXLEVEL (long) 521
#define GET_ANONYMOUSHOME (long) 522
#define SET_ANONYMOUSHOME (long) 523
#define GET_FTPHOME (long) 524
#define SET_FTPHOME (long) 525
#define GET_PUBLICHOME (long) 526
#define SET_PUBLICHOME (long) 527
#define GET_SHAREDHOME (long) 528
#define SET_SHAREDHOME (long) 529
#define GET_MHPROFILE (long) 530
#define SET_MHPROFILE (long) 531
#define GET_MHPATH (long) 532
#define SET_MHPATH (long) 533


/* Driver flags */

#define DR_DISABLE (long) 1	/* driver is disabled */
#define DR_LOCAL (long) 2	/* local file driver */
#define DR_MAIL (long) 4	/* supports mail */
#define DR_NEWS (long) 8	/* supports news */
#define DR_READONLY (long) 16	/* driver only allows readonly access */
#define DR_NOFAST (long) 32	/* "fast" data is slow (whole msg fetch) */
#define DR_NAMESPACE (long) 64	/* driver has a special namespace */
#define DR_LOWMEM (long) 128	/* low amounts of memory available */


/* Cache management function codes */

#define CH_INIT (long) 10	/* initialize cache */
#define CH_SIZE (long) 11	/* (re-)size the cache */
#define CH_MAKEELT (long) 30	/* return elt, make if needed */
#define CH_ELT (long) 31	/* return elt if exists */
#define CH_FREE (long) 40	/* free space used by elt */
#define CH_EXPUNGE (long) 45	/* delete elt pointer from list */


/* Open options */

#define OP_DEBUG (long) 1	/* debug protocol negotiations */
#define OP_READONLY (long) 2	/* read-only open */
#define OP_ANONYMOUS (long) 4	/* anonymous open of newsgroup */
#define OP_SHORTCACHE (long) 8	/* short (elt-only) caching */
#define OP_SILENT (long) 16	/* don't pass up events (internal use) */
#define OP_PROTOTYPE (long) 32	/* return driver prototype */
#define OP_HALFOPEN (long) 64	/* half-open (IMAP connect but no select) */
#define OP_EXPUNGE (long) 128	/* silently expunge recycle stream */


/* Close options */

#define CL_EXPUNGE (long) 1	/* expunge silently */


/* Fetch options */

#define FT_UID (long) 1		/* argument is a UID */
#define FT_PEEK (long) 2	/* peek at data */
#define FT_NOT (long) 4		/* NOT flag for IMAPheader lines fetch */
#define FT_INTERNAL (long) 8	/* text can be internal strings */
#define FT_PREFETCHTEXT (long) 16 /* IMAP prefetch text when fetching IMAPheader */


/* Flagging options */

#define ST_UID (long) 1		/* argument is a UID sequence */
#define ST_SILENT (long) 2	/* don't return results */
#define ST_SET (long) 4		/* set vs. clear */


/* Copy options */

#define CP_UID (long) 1		/* argument is a UID sequence */
#define CP_MOVE (long) 2	/* delete from source after copying */


/* Search/sort options */

#define SE_UID (long) 1		/* return UID */
#define SE_FREE (long) 2	/* free search program after finished */
#define SE_NOPREFETCH (long) 4	/* no search prefetching */
#define SO_FREE (long) 8	/* free sort program after finished */


/* Status options */

#define SA_MESSAGES (long) 1	/* number of messages */
#define SA_RECENT (long) 2	/* number of recent messages */
#define SA_UNSEEN (long) 4	/* number of unseen messages */
#define SA_UIDNEXT (long) 8	/* next UID to be assigned */
#define SA_UIDVALIDITY (long) 16/* UID validity value */


/* Mailgets flags */

#define MG_UID (long) 1		/* message number is a UID */
#define MG_COPY (long) 2	/* must return copy of argument */


/* Garbage collection flags */

#define GC_ELT (long) 1		/* message cache elements */
#define GC_ENV (long) 2		/* envelopes and bodies */
#define GC_TEXTS (long) 4	/* cached texts */



/* Bits for mm_list() and mm_lsub() */

/*	defined in imapnetlib.h now -JDB
#define LATT_NOINFERIORS (long) 1
#define LATT_NOSELECT (long) 2
#define LATT_MARKED (long) 4
#define LATT_UNMARKED (long) 8
*/

/* Sort functions */

#define SORTDATE 0		/* date */
#define SORTARRIVAL 1		/* arrival date */
#define SORTFROM 2		/* from */
#define SORTSUBJECT 3		/* subject */
#define SORTTO 4		/* to */
#define SORTCC 5		/* cc */
#define SORTSIZE 6		/* size */


#define NETMAXHOST 65
#define NETMAXUSER 65
#define NETMAXMBX 255
#define NETMAXSRV 21

/* In-memory sized-text */


#ifdef	ORIGINAL	//Now defined in imapnetlib.h

#define SIZEDTEXT struct mail_text


/* Sized text */

SIZEDTEXT 
{
	char *data;					// text
	unsigned long size;			// size of text in octets
};


/* String list */

#define STRINGLIST struct string_list

STRINGLIST 
{
	SIZEDTEXT text;				// string text
	STRINGLIST *next;
};

#endif	//ORIGINAL

/* Sort program */

#define SORTPGM struct sort_program

SORTPGM 
{
	unsigned int reverse : 1;	// sort function is to be reversed
	short function;				// sort function
	SORTPGM *next;				// next function
};


#ifdef	ORIGINAL	//Now defined in imapnetlib.h
/* Item in an address list */

#define ADDRESS struct mail_address

ADDRESS 
{
	char *personal;				// personal name phrase
	char *adl;					// at-domain-list source route
	char *mailbox;				// mailbox name
	char *host;					// domain name of mailbox's host
	char *error;				// error in address from SMTP module
	ADDRESS *next;				// pointer to next address in list
};


/* Message envelope */

typedef struct mail_envelope 
{
	char *remail;				// remail IMAPheader if any
	ADDRESS *return_path;		// error return address
	char *date;					// message composition date string
	ADDRESS *from;				// originator address list
	ADDRESS *sender;			// sender address list
	ADDRESS *reply_to;			// reply address list
	char *subject;				// message subject string
	ADDRESS *to;				// primary recipient list
	ADDRESS *cc;				// secondary recipient list
	ADDRESS *bcc;				// blind secondary recipient list
	char *in_reply_to;			// replied message ID
	char *message_id;			// message ID
	char *newsgroups;			// USENET newsgroups
	char *followup_to;			// USENET reply newsgroups
	char *references;			// USENET references
} ENVELOPE;
#endif	//ORIGINAL

/* Primary body types */
/* If you change any of these you must also change body_types in rfc822.c */

extern char *body_types[];	/* defined body type strings */

#define TYPETEXT 0		/* unformatted text */
#define TYPEMULTIPART 1		/* multiple part */
#define TYPEMESSAGE 2		/* encapsulated message */
#define TYPEAPPLICATION 3	/* application data */
#define TYPEAUDIO 4		/* audio */
#define TYPEIMAGE 5		/* static image */
#define TYPEVIDEO 6		/* video */
#define TYPEMODEL 7		/* model */
#define TYPEOTHER 8		/* unknown */
#define TYPEMAX 15		/* maximum type code */


/* Body encodings */
/* If you change any of these you must also change body_encodings in rfc822.c
 */

extern char *body_encodings[];	/* defined body encoding strings */

#define ENC7BIT 0		/* 7 bit SMTP semantic data */
#define ENC8BIT 1		/* 8 bit SMTP semantic data */
#define ENCBINARY 2		/* 8 bit binary data */
#define ENCBASE64 3		/* base-64 encoded data */
#define ENCQUOTEDPRINTABLE 4	/* human-readable 8-as-7 bit data */
#define ENCOTHER 5		/* unknown */
#define ENCMAX 10		/* maximum encoding code */


/* Body contents */

#define IMAPBODY struct mail_bodystruct
#define MESSAGE struct mail_body_message
#define PARAMETER struct mail_body_parameter
#define PART struct mail_body_part
#define PARTTEXT struct mail_body_text

#ifdef	ORIGINAL	//Now defined in imapnetlib.h
/* Message body text */

PARTTEXT 
{
	unsigned long offset;		// offset from body origin
	SIZEDTEXT text;				// text
};


/* Message body structure */

IMAPBODY 
{
	unsigned short type;		// body primary type
	unsigned short encoding;	// body transfer encoding
	char *subtype;				// subtype string
	PARAMETER *parameter;		// parameter list
	char *id;					// body identifier
	char *description;			// body description
	struct						// body disposition
	{
		char *type;				// disposition type
		PARAMETER *parameter;	// disposition parameters
	} disposition;
	
	STRINGLIST *language;		// body language
	PARTTEXT mime;				// MIME IMAPheader
	PARTTEXT contents;			// body part contents
	
	union						// different ways of accessing contents
	{
		PART *part;				// body part list
		MESSAGE *msg;			// body encapsulated message
	} nested;
	
	struct 
	{
		unsigned long lines;	// size of text in lines
		unsigned long bytes;	// size of text in octets
	} size;
	
	char *md5;					// MD5 checksum
};


/* Parameter list */

PARAMETER 
{
	char *attribute;			// parameter attribute name
	char *value;				// parameter value
	PARAMETER *next;			// next parameter in list
};


/* Multipart content list */

PART 
{
	IMAPBODY body;				// body information for this part
	PART *next;					// next body part
};


/* RFC-822 Message */

MESSAGE 
{
	ENVELOPE *env;				// message envelope
	IMAPBODY *body;				// message body
	PARTTEXT full;				// full message
	STRINGLIST *lines;			// lines used to filter IMAPheader
	PARTTEXT IMAPheader;		// IMAPheader text
	PARTTEXT text;				// body text
};

#endif	//ORIGINAL

/* Entry in the message cache array */

typedef struct message_cache 
{
	unsigned long msgno;		// message number
	unsigned int lockcount : 8;	// non-zero if multiple references
	unsigned long rfc822_size;	// # of bytes of message as raw RFC822

	struct 						// c-client internal use only
	{
		unsigned long uid;		// message unique ID
		PARTTEXT special;		// special text pointers
		MESSAGE msg;			// internal message pointers
	} privat;
	
	/* internal date */
	unsigned int day : 5;		// day of month (1-31)
	unsigned int month : 4;		// month of year (1-12)
	unsigned int year : 7;		// year since BASEYEAR (expires in 127 yrs)
	unsigned int hours: 5;		// hours (0-23)
	unsigned int minutes: 6;	// minutes (0-59)
	unsigned int seconds: 6;	// seconds (0-59)
	unsigned int zoccident : 1;	// non-zero if west of UTC
	unsigned int zhours : 4;	// hours from UTC (0-12)
	unsigned int zminutes: 6;	// minutes (0-59)
	
	/* system flags */
	unsigned int seen : 1;		// system Seen flag
	unsigned int deleted : 1;	// system Deleted flag
	unsigned int flagged : 1; 	// system Flagged flag
	unsigned int answered : 1;	// system Answered flag
	unsigned int draft : 1;		// system Draft flag
	unsigned int recent : 1;	// system Recent flag
	
	/* message status */
	unsigned int valid : 1;		// elt has valid flags
	unsigned int searched : 1;	// message was searched
	unsigned int sequence : 1;	// message is in sequence
	
	/* reserved for use by main program */
	unsigned int spare : 1;		// first spare bit
	unsigned int spare2 : 1;	// second spare bit
	unsigned int spare3 : 1;	// third spare bit
	void *sparep;				// spare pointer
	
	unsigned long sent:1;		// user defined //Sent flag
	unsigned long user_flags:31;	// user-assignable flags
} MESSAGECACHE;


#ifdef	ORIGINAL	//Now defined in imapnetlib.h

/* String structure */

#define STRINGDRIVER struct string_driver

typedef struct mailstring 
{
	void *data;					// driver-dependent data
	unsigned long data1;		// driver-dependent data
	unsigned long size;			// total length of string
	char *chunk;				// base address of chunk
	unsigned long chunksize;	// size of chunk
	unsigned long offset;		// offset of this chunk in base
	char *curpos;				// current position in chunk
	unsigned long cursize;		// number of bytes remaining in chunk
	STRINGDRIVER *dtb;			// driver that handles this type of string
} STRING;


/* Dispatch table for string driver */

STRINGDRIVER 
{
	/* initialize string driver */
	void (*init) (STRING *s,void *data,unsigned long size);
	
	/* get next character in string */
	char (*next) (STRING *s);
	
	/* set position in string */
	void (*setpos) (STRING *s,unsigned long i);
};

#endif	//ORIGINAL

/* Stringstruct access routines */

#define INIT(s,d,data,size) ((*((s)->dtb = &d)->init) (s,data,size))
#define SIZE(s) ((s)->size - GETPOS (s))
#define CHR(s) (*(s)->curpos)
#define SNX(s) (--(s)->cursize ? *(s)->curpos++ : (*(s)->dtb->next) (s))
#define GETPOS(s) ((s)->offset + ((s)->curpos - (s)->chunk))
#define SETPOS(s,i) (*(s)->dtb->setpos) (s,i)


/* Search program */

#define SEARCHPGM struct search_program
#define SEARCHHEADER struct search_header
#define SEARCHSET struct search_set
#define SEARCHOR struct search_or
#define SEARCHPGMLIST struct search_pgm_list


SEARCHHEADER 				// IMAPheader search
{
	char *line;				// IMAPheader line
	char *text;				// text in IMAPheader
	SEARCHHEADER *next;		// next in list
};


SEARCHSET 					// message set
{
	unsigned long first;	// sequence number
	unsigned long last;		// last value, if a range
	SEARCHSET *next;		// next in list
};


SEARCHOR 
{
	SEARCHPGM *first;		// first program
	SEARCHPGM *second;		// second program
	SEARCHOR *next;			// next in list
};


SEARCHPGMLIST 
{
	SEARCHPGM *pgm;			// search program
	SEARCHPGMLIST *next;	// next in list
};


SEARCHPGM						// search program
{
	SEARCHSET *msgno;			// message numbers
	SEARCHSET *uid;				// unique identifiers
	SEARCHOR *or;				// or'ed in programs
	SEARCHPGMLIST *not;			// and'ed not program
	SEARCHHEADER *IMAPheader;	// list of headers
	STRINGLIST *bcc;			// bcc recipients
	STRINGLIST *body;			// text in message body
	STRINGLIST *cc;				// cc recipients
	STRINGLIST *from;			// originator
	STRINGLIST *keyword;		// keywords
	STRINGLIST *unkeyword;		// unkeywords
	STRINGLIST *subject;		// text in subject
	STRINGLIST *text;			// text in headers and body
	STRINGLIST *to;				// to recipients
	unsigned long larger;		// larger than this size
	unsigned long smaller;		// smaller than this size
	unsigned short sentbefore;	// sent before this date
	unsigned short senton;		// sent on this date
	unsigned short sentsince;	// sent since this date
	unsigned short before;		// before this date
	unsigned short on;			// on this date
	unsigned short since;		// since this date
	unsigned int answered : 1;	// answered messages
	unsigned int unanswered : 1;// unanswered messages
	unsigned int deleted : 1;	// deleted messages
	unsigned int undeleted : 1;	// undeleted messages
	unsigned int draft : 1;		// message draf
	unsigned int undraft : 1;	// message undraft
	unsigned int flagged : 1;	// flagged messages
	unsigned int unflagged : 1;	// unflagged messages
	unsigned int recent : 1;	// recent messages
	unsigned int old : 1;		// old messages
	unsigned int seen : 1;		// seen messages
	unsigned int unseen : 1;	// unseen messages
};


/* Mailbox status */

typedef struct mbx_status 
{
  long flags;					// validity flags
  unsigned long messages;		// number of messages
  unsigned long recent;			// number of recent messages
  unsigned long unseen;			// number of unseen messages
  unsigned long uidnext;		// next UID to be assigned
  unsigned long uidvalidity;	// UID validity value
} MAILSTATUS;


/* Mail Access I/O stream */


/* Parse results from mail_valid_net_parse */

typedef struct net_mailbox 
{
	char host[NETMAXHOST];			// host name
	char user[NETMAXUSER];			// user name
	char mailbox[NETMAXMBX];		// mailbox name
	char service[NETMAXSRV];		// service name
	unsigned long port;				// TCP port number
	unsigned int anoflag : 1;		// anonymous
	unsigned int dbgflag : 1;		// debug flag
} NETMBX;

/* Network access I/O stream */

/* Mailgets data identifier */

typedef struct GETS_DATA 
{
	MAILSTREAM *stream;
	unsigned long msgno;
	char *what;
	STRINGLIST *stl;
	unsigned long first;
	unsigned long last;
	long flags;
} GETS_DATA;

#define INIT_GETS(md,s,m,w,f,l) \
  md.stream = s, md.msgno = m, md.what = w, md.first = f, md.last = l, \
  md.stl = NIL, md.flags = NIL;


/* Jacket into external interfaces */

typedef long (*readfn_t) (void *stream,unsigned long size,char *buffer);
typedef char *(*mailgets_t) (readfn_t f,void *stream,unsigned long size,
			     GETS_DATA *md);
typedef char *(*readprogress_t) (GETS_DATA *md,unsigned long octets);
typedef void *(*mailcache_t) (MAILSTREAM *stream,unsigned long msgno,long op);
typedef long (*tcptimeout_t) (long time);
typedef void *(*authchallenge_t) (void *stream,unsigned long *len);
typedef long (*authrespond_t) (void *stream,char *s,unsigned long size);
typedef long (*authclient_t) (authchallenge_t challenger,
			      authrespond_t responder,NETMBX *mb,void *s,
			      unsigned long *trial,char *user);
typedef char *(*authresponse_t) (void *challenge,unsigned long clen,
				 unsigned long *rlen);
typedef char *(*authserver_t) (authresponse_t responder,int argc,char *argv[]);
typedef void (*smtpverbose_t) (char *buffer);
typedef void (*imapenvelope_t) (MAILSTREAM *stream,unsigned long msgno,
				ENVELOPE *env);


#define AUTHENTICATOR struct mail_authenticator

AUTHENTICATOR 
{
	char *name;					// name of this authenticator
	authclient_t client;		// client function that supports it
	authserver_t server;		// server function that supports it
	AUTHENTICATOR *next;		// next authenticator
};


/* Structure for mail driver dispatch */

#define DRIVER struct driver


/* Mail I/O stream handle */

typedef struct mail_stream_handle 
{
	MAILSTREAM *stream;				// pointer to mail stream
	unsigned short sequence;		// sequence of what we expect stream to be
} MAILHANDLE;

/* Mail I/O stream */
	
typedef struct mail_stream 
{
	DRIVER *dtb;					// dispatch table for this driver
	void *local;					// pointer to driver local data
	char *mailbox;					// mailbox name
	unsigned short use;				// stream use count
	unsigned short sequence;		// stream sequence
	unsigned int lock : 1;			// stream lock flag
	unsigned int debug : 1;			// stream debug flag
	unsigned int silent : 1;		// silent stream from Tenex
	unsigned int rdonly : 1;		// stream read-only flag
	unsigned int anonymous : 1;		// stream anonymous access flag
	unsigned int scache : 1;		// stream short cache flAG
	unsigned int halfopen : 1;		// stream half-open flag
	unsigned int perm_seen : 1;		// permanent Seen flag
	unsigned int perm_deleted : 1;	// permanent Deleted flag
	unsigned int perm_flagged : 1;	// permanent Flagged flag
	unsigned int perm_answered :1;	// permanent Answered flag
	unsigned int perm_draft : 1;	// permanent Draft flag
	unsigned int kwd_create : 1;	// can create new keywords
	unsigned long perm_user_flags;	// mask of permanent user flags
	unsigned long gensym;			// generated tag
	unsigned long nmsgs;			// # of associated msgs
	unsigned long recent;			// # of recent msgs
	unsigned long uid_validity;		// UID validity sequence
	unsigned long uid_last;			// last assigned UID
	char *user_flags[NUSERFLAGS];	// pointers to user flags in bit order
	unsigned long cachesize;		// size of message cache

// NOTE: (JOK) The stream now has a "current elt". Fetch body and fetch envelope
// places stuff into elt.privat.msg.body and elt.privat.msg.env.
	MESSAGECACHE	*CurrentElt;
// END NOTE JOK.

	//This can go away
	MESSAGECACHE **cache;			// message cache array

	unsigned long msgno;			// message number of `current' message
	ENVELOPE *env;					// scratch buffer for envelope
	IMAPBODY *body;					// scratch buffer for body
	SIZEDTEXT IMAPheader;			// scratch buffer for IMAPheader text
	SIZEDTEXT text;					// scratch buffer for text
	union 
	{								// internal use only
		struct 
		{							// search temporaries
	 		char *charset;			// character set
	  		STRINGLIST *string;		// string(s) to search
	  		long result;			// search result
		} search;
	} private;

// added by JDB

	// stream to talk to do the network stuff
	
	TransStream transStream;		// Eudora network stream information
	
	short bConnected;				// Set to true once we're connected
	short bAuthenticated;			// Set to true once we're authenticated
	short bSelected;				// Set to true once we've selected an mbox

	unsigned char pHost[NETMAXHOST+1];
	unsigned long port;
	
	Boolean errorred;				// we already displayed an error
		
	// mailbox list results
	MailboxNodeHandle fListResultsHandle;	// handle to tree of mailboxes
	const char *fRef;
	Boolean fIncludeInbox;
	Boolean fProgress;				// should we display progress as we're fetching mailboxes?
	
	mailgets_t mailgets;
	
	// fetch flags results
	UIDNodeHandle fUIDResults;
		
	// network traffic results handle
	Handle fNetData;				// header/message data read from network
	
	// mailbox resync specific
	Boolean chunkHeaders;			// set to true to tell imap engine we're fetching minimal headers in chunks
	unsigned long headerUID;		// the uid of the message we just fetched headers for
	Handle delivery;				// place to put minimal headers
	
	// used to save messages
	short refN;						// refNum of open file waiting to accept message

	// progress
	Boolean showProgress;			// set to true to display progress
	long totalTransfer;				// total # of bytes to transfer
	long currentTransfer;			// current # of bytes transferred
	long lastProgress;				// last time some progress was displayed
	
	// mailbox status
	MAILSTATUS mailboxStatus;		// place to store results of last STATUS command.
	
	// logout options
	Boolean fastLogout;				// set to true if we don't want to wait for the server to respond to a LOGOUT
	
#ifdef	DEBUG
	short flagsRefN;				// refNum of file to save flags to.
#endif
	
	// ALERT status responses
	Str255 alertStr;				// string containing text returned from the IMAP server in an ALERT response.
	
	Accumulator UIDPLUSResponse;	// uids returned in a UIDPLUS response
	long UIDPLUSuv;					// the uidvalidity noted during a series of UIDPLUS responses.  
									// if this changes before messages are transferred, the responses are discarded.
	
//End added by JDB
	
} MAILSTREAM;

// Structure for allowing a callback to read from a string.
typedef struct {
	char *s;
	unsigned long size;
} ParenStrData;

/* Message overview */

typedef struct mail_overview 
{
	char *subject;					// message subject string
	ADDRESS *from;					// originator address list
	char *date;						// message composition date string
	char *message_id;				// message ID
	char *references;				// USENET references
	struct 							// may be 0 or NUL if unknown/undefined
	{
		unsigned long octets;		// message octets (probably LF-newline form)
		unsigned long lines;		// message lines
		char *xref;					// cross references
	} optional;
} OVERVIEW;


typedef void (*overview_t) (MAILSTREAM *stream,unsigned long uid,OVERVIEW *ov);


/* Mail driver dispatch */

DRIVER 
{
	char *name;				// driver name
	unsigned long flags;	// driver flags
	DRIVER *next;			// next driver
	
	// mailbox is valid for us
	DRIVER *(*valid) (char *mailbox);
	
	// manipulate driver parameters
	void *(*parameters) (long function,void *value);
	
	// scan mailboxes
	void (*scan) (MAILSTREAM *stream,char *ref,char *pat,char *contents);
	
	// list mailboxes
	void (*list) (MAILSTREAM *stream,char *ref,char *pat);
	
	// list subscribed mailboxes
	void (*lsub) (MAILSTREAM *stream,char *ref,char *pat);
	
	// subscribe to mailbox
	long (*subscribe) (MAILSTREAM *stream,char *mailbox);
	
	// unsubscribe from mailbox
	long (*unsubscribe) (MAILSTREAM *stream,char *mailbox);
	
	// create mailbox
	long (*create) (MAILSTREAM *stream,char *mailbox);
	
	// delete mailbox
	long (*mbxdel) (MAILSTREAM *stream,char *mailbox);
	
	// rename mailbox
	long (*mbxren) (MAILSTREAM *stream,char *old,char *newname);
	
	// status of mailbox
	long (*status) (MAILSTREAM *stream,char *mbx,long flags);

	// open mailbox
	MAILSTREAM *(*open) (MAILSTREAM *stream);

	// close mailbox
	void (*close) (MAILSTREAM *stream,long options);

	// fetch message "fast" attributes
	Boolean (*fast) (MAILSTREAM *stream,char *sequence,long flags);
	
	// fetch message flags
	Boolean (*msgflags) (MAILSTREAM *stream,char *sequence,long flags);
	
	// fetch message overview
	Boolean (*overview) (MAILSTREAM *stream,char *sequence,overview_t ofn);

// Added by JOK!!!
 	ENVELOPE *(*envelope) (MAILSTREAM *stream,unsigned long msgno, long flags);
				/* fetch message envelopes */
// Modified by JOK!!!
  	IMAPBODY *(*structure) (MAILSTREAM *stream,unsigned long msgno, long flags);
	
	// fetch message envelopes
//	ENVELOPE *(*structure) (MAILSTREAM *stream,unsigned long msgno,IMAPBODY **body,long flags);
	
	// return RFC-822 IMAPheader
	char *(*IMAPheader) (MAILSTREAM *stream,unsigned long msgno, unsigned long *length,long flags);
	
	// return RFC-822 text
	long (*text) (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
	
	// load cache
	long (*msgdata) (MAILSTREAM *stream,unsigned long msgno,char *sequence,char *section, unsigned long first,unsigned long last,STRINGLIST *lines, long flags);
	
	// return UID for message
	unsigned long (*uid) (MAILSTREAM *stream,unsigned long msgno);
	
	// return message number from UID
	unsigned long (*msgno) (MAILSTREAM *stream,unsigned long uid);
	
	// modify flags
	Boolean (*flag) (MAILSTREAM *stream,char *sequence,char *flag,long flags);
	
	// per-message modify flags
	void (*flagmsg) (MAILSTREAM *stream,MESSAGECACHE *elt);
	
	// search for message based on criteria	was void, now Boolean JDB
	Boolean (*search) (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
	
	// sort messages
	unsigned long *(*sort) (MAILSTREAM *stream,char *charset,SEARCHPGM *spg, SORTPGM *pgm,long flags);
	
	// thread messages
	void *(*thread) (MAILSTREAM *stream,char *seq,long function,long flag);
	
	// ping mailbox to see if still alive
	long (*ping) (MAILSTREAM *stream);
	
	// check for new messages
	void (*check) (MAILSTREAM *stream);
	
	// expunge deleted messages
	long (*expunge) (MAILSTREAM *stream);
	
	// copy messages to another mailbox
	long (*copy) (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
	
	// append string message to mailbox
	long (*append) (MAILSTREAM *stream,char *mailbox,char *flags,char *date, STRING *message);
	
	// garbage collect stream
	void (*gc) (MAILSTREAM *stream,long gcflags);
	
	// see if the stream is connected.
	Boolean (*connected) (MAILSTREAM *stream);
	
	// resturn the rfc822size
	unsigned long (*rfc822size) (MAILSTREAM *stream, unsigned long msgno, long flags);
};





/* Other symbols */

extern const char *days[];	/* day name strings */
extern const char *months[];	/* month name strings */


// Stuff that used to be in linkage.h
extern DRIVER imapdriver; 
extern DRIVER dummydriver; 
extern AUTHENTICATOR auth_log; 

/* Function prototypes */

void mm_searched (MAILSTREAM *stream,unsigned long number);
void mm_exists (MAILSTREAM *stream,unsigned long number);
void mm_expunged (MAILSTREAM *stream,unsigned long number);
void mm_flags (MAILSTREAM *stream,unsigned long number);
void mm_elt_flags (MAILSTREAM *stream, MESSAGECACHE *elt);	//added by JOK
void mm_notify (MAILSTREAM *stream,char *string,long errflg);
void mm_list (MAILSTREAM *stream,int delimiter,char *name,long attributes);
void mm_lsub (MAILSTREAM *stream,int delimiter,char *name,long attributes);
void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status);
void mm_alert (MAILSTREAM *stream, char *string);
void mm_log (char *string,long errflg);
void pmm_log(Str255 pErrorString,long errflg);
void mm_dlog (char *string);
void mm_login (NETMBX *mb,char *user,char *pwd,long trial);
void mm_critical (MAILSTREAM *stream);
void mm_nocritical (MAILSTREAM *stream);
long mm_diskerror (MAILSTREAM *stream,long errcode,long serious);
void mm_fatal (char *string);
void *mm_cache (MAILSTREAM *stream,unsigned long msgno,long op);

extern STRINGDRIVER mail_string;
void mail_string_init (STRING *s,void *data,unsigned long size);
char mail_string_next (STRING *s);
void mail_string_setpos (STRING *s,unsigned long i);
void mail_link (DRIVER *driver);
void *mail_parameters (MAILSTREAM *stream,long function,void *value);
DRIVER *mail_valid (MAILSTREAM *stream,char *mailbox,char *purpose);
DRIVER *mail_valid_net (char *name,DRIVER *drv,char *host,char *mailbox);
long mail_valid_net_parse (char *name,NETMBX *mb);
void mail_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mail_list (MAILSTREAM *stream,char *ref,char *pat);
//void mail_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mail_subscribe (MAILSTREAM *stream,char *mailbox);
long mail_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mail_create (MAILSTREAM *stream,char *mailbox);
long mail_delete (MAILSTREAM *stream,char *mailbox);
long mail_rename (MAILSTREAM *stream,char *old,char *newname);
long mail_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *mail_open (MAILSTREAM *oldstream,char *name,long options);
MAILSTREAM *mail_close_full (MAILSTREAM *stream,long options);
MAILHANDLE *mail_makehandle (MAILSTREAM *stream);
void mail_free_handle (MAILHANDLE **handle);
MAILSTREAM *mail_stream (MAILHANDLE *handle);


Boolean mail_fetch_fast (MAILSTREAM *stream,char *sequence,long flags);
Boolean mail_fetch_flags (MAILSTREAM *stream,char *sequence,long flags);

//Modfified
Boolean mail_fetch_overview (MAILSTREAM *stream,char *sequence,overview_t ofn);

//Added by JOK
unsigned long mail_fetch_rfc822size (MAILSTREAM *stream, unsigned long msgno, long flags);

// JOK - These have been modified.
IMAPBODY *mail_fetch_structure (MAILSTREAM *stream,unsigned long msgno, long flags);
// JOK - THis is new.
ENVELOPE  *mail_fetch_envelope (MAILSTREAM *stream,unsigned long msgno, long flags);
// END JOK

long mail_fetch_message (MAILSTREAM *stream,unsigned long msgno, long flags);
long mail_fetch_header (MAILSTREAM *stream, unsigned long msgno, char *sequence, char *section, STRINGLIST *lines, long flags);
long mail_fetch_text (MAILSTREAM *stream, unsigned long msgno, char *section, long flags);
long mail_fetch_mime (MAILSTREAM *stream, unsigned long msgno, char *section, long flags);
long mail_fetch_body (MAILSTREAM *stream, unsigned long msgno, char *section, long flags);
/*
char *mail_fetch_header (MAILSTREAM *stream,unsigned long msgno,char *section,STRINGLIST *lines,unsigned long *len,long flags);
char *mail_fetch_text (MAILSTREAM *stream,unsigned long msgno,char *section,unsigned long *len,long flags);
char *mail_fetch_mime (MAILSTREAM *stream,unsigned long msgno,char *section,unsigned long *len,long flags);
char *mail_fetch_body (MAILSTREAM *stream,unsigned long msgno,char *section,unsigned long *len,long flags);
*/
long mail_partial_text (MAILSTREAM *stream,unsigned long msgno,char *section,unsigned long first,unsigned long last,long flags);
long mail_partial_body (MAILSTREAM *stream,unsigned long msgno,char *section,unsigned long first,unsigned long last,long flags);
unsigned long mail_uid (MAILSTREAM *stream,unsigned long msgno);
unsigned long mail_msgno (MAILSTREAM *stream,unsigned long uid);

MESSAGECACHE *mail_elt (MAILSTREAM *stream);
//MESSAGECACHE *mail_elt (MAILSTREAM *stream,unsigned long msgno);


Boolean mail_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
Boolean mail_search_full (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
long mail_ping (MAILSTREAM *stream);
void mail_check (MAILSTREAM *stream);
long mail_expunge (MAILSTREAM *stream);
long mail_copy_full (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mail_append_full (MAILSTREAM *stream,char *mailbox,char *flags,char *date,STRING *message);
void mail_gc (MAILSTREAM *stream,long gcflags);
void mail_gc_msg (MESSAGE *msg,long gcflags);

#if 0 // JOK - No longer used - replaced by mail_sub_body().
BODY *mail_body (MAILSTREAM *stream,unsigned long msgno,char *section);
#endif // JOK

// Added by JOK
IMAPBODY *mail_sub_body (IMAPBODY *body, char *section);
// END JOK

char *mail_date (char *string,MESSAGECACHE *elt);
char *mail_cdate (char *string,MESSAGECACHE *elt);
long mail_parse_date (MESSAGECACHE *elt,char *string);
void mail_exists (MAILSTREAM *stream,unsigned long nmsgs);
void mail_recent (MAILSTREAM *stream,unsigned long recent);
void mail_expunged (MAILSTREAM *stream,unsigned long msgno);
#ifdef	NOT_NEEDED
void mail_lock (MAILSTREAM *stream);
void mail_unlock (MAILSTREAM *stream);
#endif	//NOT_NEEDED
void mail_debug (MAILSTREAM *stream);
void mail_nodebug (MAILSTREAM *stream);
long mail_match_lines (STRINGLIST *lines,STRINGLIST *msglines,long flags);
unsigned long mail_filter (char *text,unsigned long len,STRINGLIST *lines,long flags);
long mail_search_msg (MAILSTREAM *stream,unsigned long msgno,char *charset,SEARCHPGM *pgm);
long mail_search_keyword (MAILSTREAM *stream,MESSAGECACHE *elt,STRINGLIST *st);
long mail_search_addr (ADDRESS *adr,char *charset,STRINGLIST *st);
long mail_search_string (char *txt,char *charset,STRINGLIST *st);
char *mail_search_gets (readfn_t f,void *stream,unsigned long size,MAILSTREAM *ms,unsigned long msgno,char *what,long flags);
long mail_search_text (char *txt,long len,char *charset,STRINGLIST *st);
SEARCHPGM *mail_criteria (char *criteria);
int mail_criteria_date (unsigned short *date);
int mail_criteria_string (STRINGLIST **s);
unsigned long *mail_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,SORTPGM *pgm,long flags);
unsigned long *mail_sort_msgs (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,SORTPGM *pgm,long flags);
int mail_sort_compare (const void *a1,const void *a2);
int mail_compare_msg (MAILSTREAM *stream,short function,unsigned long m1,unsigned long m2);
int mail_compare_ulong (unsigned long l1,unsigned long l2);
int mail_compare_cstring (char *s1,char *s2);
int mail_compare_sstring (char *s1,char *s2);
int mail_compare_address (ADDRESS *a1,ADDRESS *a2);
unsigned long mail_longdate (MESSAGECACHE *elt);
char *mail_skip_re (char *s);
char *mail_skip_fwd (char *s);
long mail_sequence (MAILSTREAM *stream,char *sequence);
long mail_uid_sequence (MAILSTREAM *stream,char *sequence);
long mail_parse_flags (MAILSTREAM *stream,char *flag,unsigned long *uf);


MESSAGECACHE *mail_new_cache_elt (void);
//MESSAGECACHE *mail_new_cache_elt (unsigned long msgno);
ENVELOPE *mail_newenvelope (void);
ADDRESS *mail_newaddr (void);
IMAPBODY *mail_newbody (void);
IMAPBODY *mail_initbody (IMAPBODY *body);
PARAMETER *mail_newbody_parameter (void);
PART *mail_newbody_part (void);
MESSAGE *mail_newmsg (void);
STRINGLIST *mail_newstringlist (void);
SEARCHPGM *mail_newsearchpgm (void);
SEARCHHEADER *mail_newsearchheader (char *line);
SEARCHSET *mail_newsearchset (void);
SEARCHOR *mail_newsearchor (void);
SEARCHPGMLIST *mail_newsearchpgmlist (void);
SORTPGM *mail_newsortpgm (void);
void mail_free_body (IMAPBODY **body);
void mail_free_body_data (IMAPBODY *body);
void mail_free_body_parameter (PARAMETER **parameter);
void mail_free_body_part (PART **part);
//void mail_free_cache (MAILSTREAM *stream);
void mail_free_elt (MESSAGECACHE **elt);
void mail_free_envelope (ENVELOPE **env);
void mail_free_address (ADDRESS **address);
void mail_free_stringlist (STRINGLIST **string);
void mail_free_searchpgm (SEARCHPGM **pgm);
void mail_free_searchheader (SEARCHHEADER **hdr);
void mail_free_searchset (SEARCHSET **set);
void mail_free_searchor (SEARCHOR **orl);
void mail_free_searchpgmlist (SEARCHPGMLIST **pgl);
void mail_free_sortpgm (SORTPGM **pgm);
AUTHENTICATOR *mail_lookup_auth (unsigned long i);
unsigned int mail_lookup_auth_name (char *mechanism);


TransStream net_open (MAILSTREAM *stream,char *host,char *service,unsigned long prt);
char *net_getline (TransStream stream);
/* stream must be void* for use as readfn_t */
long net_getbuffer (void *stream,unsigned long size,char *buffer);
long net_soutr (TransStream stream,char *string);
long net_sout (TransStream stream,char *string,unsigned long size);
void net_close (TransStream stream);
char *net_host (TransStream stream);
unsigned long net_port (TransStream stream);
char *net_localhost (TransStream stream);

// not used
#ifdef	NOT_USED
long sm_subscribe (char *mailbox);
long sm_unsubscribe (char *mailbox);
char *sm_read (void **sdb);
#endif

//JDB
OSErr mail_new_stream (MAILSTREAM **stream, unsigned char *host, unsigned long *port, const char *user);
void mail_free_stream (MAILSTREAM **stream);
DRIVER *imapmail_valid_net (MAILSTREAM *stream, DRIVER *drv, char *host);
long imapmail_valid_net_parse (MAILSTREAM *stream, NETMBX *mb);
char *MyUpperCase(char *string);

// Callback routine to read from a string.
Boolean str_getbuffer (void *st,unsigned long size,char *buffer);

#endif //MAIL_H
