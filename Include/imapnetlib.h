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

/* Copyright (c) 1997 by QUALCOMM Incorporated */

/**********************************************************************
 *	imapnetlib.h
 *
 *		This file contains declarations for the imap network functions
 *	Eudora needs to do online IMAP.  It provideslow-level IMAP fetch,
 *	store, etc. for a  single mailbox or TransStream stream.
 *
 *		Note, this closely mirrors what JOK has done for Windows with
 *	the CImapStream class.  Thanks, JOK!
 **********************************************************************/

#ifndef IMAPNETLIB_H
#define IMAPNETLIB_H

/* TYPEDEFS for the exported IMAP structures. */

typedef unsigned long UIDVALIDITY;
typedef unsigned long IMAPUID;

#define MAXLOGINTRIALS 1	/* maximum number of login trials */

/* Primary body types */

#define TYPETEXT 0			//unformatted text
#define TYPEMULTIPART 1		// multiple part
#define TYPEMESSAGE 2		// encapsulated message
#define TYPEAPPLICATION 3	// application data
#define TYPEAUDIO 4			// audio
#define TYPEIMAGE 5			// static image
#define TYPEVIDEO 6			// video
#define TYPEMODEL 7			// model
#define TYPEOTHER 8			// unknown
#define TYPEMAX 15			// maximum type code


/* Body encodings */

#define ENC7BIT 0				// 7 bit SMTP semantic data
#define ENC8BIT 1				// 8 bit SMTP semantic data
#define ENCBINARY 2				// 8 bit binary data
#define ENCBASE64 3				// base-64 encoded data
#define ENCQUOTEDPRINTABLE 4	// human-readable 8-as-7 bit data
#define ENCOTHER 5				// unknown
#define ENCMAX 10				// maximum encoding code


/* Bits for mm_list() and mm_lsub() */

#define LATT_NOINFERIORS (long) 1
#define LATT_NOSELECT (long) 2
#define LATT_MARKED (long) 4
#define LATT_UNMARKED (long) 8
#define LATT_NOT_IMAP (long) 16	
#define LATT_JUNK (long) 32	
#define LATT_HASNOCHILDREN (long) 64
#define LATT_TRASH (long) 512	
#define LATT_ROOT (long) 1024		// to mark the topmost node in a mailbox tree

/*  Structures. */

#define IMAPBODY struct mail_bodystruct
#define MESSAGE struct mail_body_message
#define PARAMETER struct mail_body_parameter
#define PART struct mail_body_part
#define PARTTEXT struct mail_body_text
#define SIZEDTEXT struct mail_text

/* Sized text */
SIZEDTEXT
{
	char *data;					// text
	unsigned long size;			// size of text in octets
};


/* Message body text */
PARTTEXT
{
	unsigned long offset;		// offset from body origin
	SIZEDTEXT text;				// text
};


/* String list */
typedef struct string_list
{
	SIZEDTEXT text;				// string text
	struct string_list *next;
} STRINGLIST;


typedef struct mail_address
{
	char	*personal;			// personal name phrase
	char	*adl;				// at-domain-list source route
	char	*mailbox;			// mailbox name
	char	*host;				// domain name of mailbox's host
	char	*error;				// error in address from SMTP module
	struct mail_address *next;	// pointer to next address in list
} ADDRESS;


typedef struct mail_envelope
{
	char	*remail;			// remail header if any
	ADDRESS *return_path;		// error return address
	char	*date;				// message composition date string
	ADDRESS *from;				// originator address list
	ADDRESS *sender;			// sender address list
	ADDRESS *reply_to;			// reply address list
	char	*subject;			// message subject string
	ADDRESS *to;				// primary recipient list
	ADDRESS *cc;				// secondary recipient list
	ADDRESS *bcc;				// blind secondary recipient list
	char	*in_reply_to;		// replied message ID
	char	*message_id;		// message ID
	char	*newsgroups;		// USENET newsgroups
	char	*followup_to;		// USENET reply newsgroups
	char	*references;		// USENET references
} ENVELOPE;


/* Message body structure */
IMAPBODY
{
	unsigned short type;		// body primary type
	unsigned short encoding;	// body transfer encoding
	char *subtype;				// subtype string
	PARAMETER *parameter;		// parameter list
	char *id;					// body identifier
	char *description;			// body description

	struct 						// body disposition
	{
		char *type;				// disposition type
		PARAMETER *parameter;	// disposition parameters
	} disposition;

	STRINGLIST *language;		// body language
	PARTTEXT mime;				// MIME header
	PARTTEXT contents;			// body part contents

	union 						// different ways of accessing contents
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
	IMAPBODY body;					// body information for this part
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


// STRINGDRIVER declarations.
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

/* Reader type declaration. Reads a chunk at a time from the network. */
//typedef Boolean (*readfn_t) (void *stream,unsigned long size,char *buffer);


/* For passing IMAP flag info. */
// Note: NEED to extend this to handle Eudora's flags via annotations. In that
// case, we'd probably need a way to indicate which flags IMAP can handle (via annotations).
typedef struct
{
	Boolean	DELETED;
	Boolean	SEEN;
	Boolean	FLAGGED;
	Boolean	ANSWERED;
	Boolean	DRAFT;
	Boolean	RECENT;
} IMAPFLAGS;


/* IMAPFULL structure for returning info from FetchFast, FetchFlags and FetchFull. */
typedef struct
{
	IMAPFLAGS		*Flags;
	char			*InternalDate;
	ENVELOPE		*Env;
	IMAPBODY			*Body;
} IMAPFULL;

// MAILSTREAM is currently defined in mail.h
typedef struct mail_stream MAILSTREAM;

// IMAP structure, contains info about an IMAP connection
typedef struct IMAPStreamStruct
{		
	unsigned long currentMsgNum;
	unsigned long currentUID;
	unsigned long messageCount;
	unsigned long MessageSizeLimit;
	Str255 mailboxName;					// Name of mailbox.  Does NOT contain stuff like braces.
	UIDVALIDITY uidvalidity;
	MAILSTREAM *mailStream;
	Str255 pServerName;					// remote host name
	unsigned long portNumber;			// port number
	MailboxNodeHandle mbox;				// The mailbox this stream is connected to
} IMAPStreamStruct, *IMAPStreamPtr;

// IMAP related errors.  Gonna be a lot of these ...
enum
{
	errIMAPOutOfMemory=0,			// out of memory error
	errIMAPNoServer,				// no server was specified
	errIMAPNoMailstream,			// no current mailstream
	errIMAPNoAccount,				// no account
	errIMAPNoMailbox,				// no mailbox specified
	errIMAPSelectMailbox,			// failed to select a mailbox
	errIMAPMailboxNameInvalid,		// the name contains an invalid character
	errIMAPCreateStream,			// error opening stream
	errIMAPStreamIsLocked,			// the stream is locked
	errIMAPCreateMailbox,			// can't create the mailbox
	errIMAPDeleteMailbox,			// can't delete the mailbox
	errIMAPRenameMailbox,			// can't rename the mailbox
	errIMAPMoveMailbox,				// can't move the mailbox
	errIMAPNotConnected,			// we're inexplicably not connected
	errIMAPNoMessagesSpecified,		// no messages were specified
	errIMAPDeleteMessage,			// could not delete the message
	errIMAPUndeleteMessage,			// could not undelete the message
	errIMAPCopyFailed,				// could not copy the messages
	errNotIMAPPers,					// This personality doesn't seem to be an IMAP personality.
	errNotIMAPMailboxErr,			// This personality doesn't seem to be an IMAP mailbox.	
	errIMAPListErr,					// the LIST command has failed.
	errIMAPListInUse,				// the mailbox list can't be modified.
	errIMAPNoTrash,					// can't locate the trash mailbox.
	errIMAPStubFileBad,				// the Stub file seems to be corrupt
	errIMAPCouldNotFetchPart,		// failed to fetch the attachment from the server
	errIMAPBadEncodingErr,			// faield to decode the attachment
	errIMAPSearchMailboxErr,		// failed to search the mailbox.
	errIMAPMailboxChangedErr,		// this mailbox needs to be resynchronized
	errIMAPReadOnlyStreamErr,		// this mailbox is read-only.
	errIMAPCantExpunge,				// this mailbox can't be expunged for some reason
	errIMAPOneDownloadFailed,		// at least one message failed to be downloaded during resync.
	errIMAPNoJunk,					// can't locate the junk mailbox.
	errIMAPLastError
};

/* UIDNodeHandle is used to build a list of UIDs */
typedef struct UIDNode UIDNode, *UIDNodePtr, **UIDNodeHandle;

/*
 *	Setup and teardown
 */

/* create a "control" stream */
OSErr NewImapStream(IMAPStreamPtr *imapStream, UPtr ServerName, unsigned long PortNum);

/* destroy an IMAP stream */
void ZapImapStream(IMAPStreamPtr *imapStream);

/* Open a "control" stream without selecting a mailbox. */
Boolean OpenControlStream(IMAPStreamPtr imapStream);

/* Open a connection to the server, SELECTing a mailbox. */
//If MailboxName == NULL, then open a connection but don't SELECT anything.
Boolean IMAPOpenMailbox(IMAPStreamPtr imapStream, const char *MailboxName, Boolean readOnly);


/*
 *	Mailbox management functions
 */


/* Create mailbox */
Boolean CreateIMAPMailbox (IMAPStreamPtr imapStream, const char *mailboxName);

/* Delete a mailbox */
Boolean DeleteIMAPMailbox (IMAPStreamPtr imapStream, const char *mailboxName);

/* Rename a mailbox */
Boolean RenameIMAPMailbox(IMAPStreamPtr imapStream, const char *oldName, const char *newName);

/* Do a LIST on the mailbox to get it's attributes. */
Boolean FetchMailboxAttributes (IMAPStreamPtr imapStream, const char *mailboxName);

/* Do a STATUS to check the mailbox for messages */
Boolean FetchMailboxStatus (IMAPStreamPtr imapStream, const char *mailboxName, long flags);

/* List unsubscribed mailboxes. */
Boolean IMAPListUnSubscribed (IMAPStreamPtr imapStream, const char *pReference, Boolean includeMailbox);


/*
 *	Mailbox-level commands that do not require a SELECTED mailbox
 */


/* Check and NOOP */
void Check(IMAPStreamPtr imapStream);
Boolean Noop(IMAPStreamPtr imapStream);

/* delete list of messages. */
Boolean UIDDeleteMessages(IMAPStreamPtr imapStream, char *pList, Boolean Expunge);

/* Undelete a list of messages */
Boolean UIDUnDeleteMessages(IMAPStreamPtr imapStream, char *pList);

/* Do a UID Expunge. */	//ummm ... what's this do, anyway?
OSErr UIDExpunge(IMAPStreamPtr imapStream, const char *pUidList);

/* Do a simple Expunge. */
Boolean	Expunge(IMAPStreamPtr imapStream);

/* Logout. */		//Hey!  This doesn't do anything yet!
Boolean	Logout(IMAPStreamPtr imapStream);

/* Access to internal state */
Boolean IsSelected(MAILSTREAM *imapStream);
Boolean IsConnected(MAILSTREAM *imapStream);
Boolean IsAuthenticated(MAILSTREAM *imapStream);

/* Utility functions */
unsigned long GetMessageCount(IMAPStreamPtr imapStream);
unsigned long GetSTATUSMessageCount(IMAPStreamPtr imapStream);

/* Mark a message as answered */
Boolean UIDMarkAsReplied(IMAPStreamPtr imapStream, unsigned long uid);

/*
 * ========== Message Fetch ==================
 */
 
/* Top level message only. */
ENVELOPE *UIDFetchEnvelope(IMAPStreamPtr imapStream, unsigned long uid);
IMAPBODY *UIDFetchStructure(IMAPStreamPtr imapStream, unsigned long uid);
void FreeBodyStructure(IMAPBODY *pBody);
Boolean UIDFetchFlags(IMAPStreamPtr imapStream, const char *pSequence);
Boolean FetchAllFlags (IMAPStreamPtr imapStream, UIDNodeHandle *uidList);
Boolean FetchFlags (IMAPStreamPtr imapStream, const char *sequence, UIDNodeHandle *uidList);
unsigned long UIDFetchLastUid(IMAPStreamPtr imapStream);
IMAPFULL *UIDFetchFast(IMAPStreamPtr imapStream, unsigned long uid);
IMAPFULL *UIDFetchAll(IMAPStreamPtr imapStream, unsigned long uid);
IMAPFULL *UIDFetchFull(IMAPStreamPtr imapStream, unsigned long uid);
char *UIDFetchInternalDate(IMAPStreamPtr imapStream, unsigned long uid);
Boolean UIDFetchHeader(IMAPStreamPtr imapStream, unsigned long uid, Boolean file);
Boolean UIDFetchMessage(IMAPStreamPtr imapStream, unsigned long uid, Boolean Peek); // Fetches complete rfc822 message, including header..
Boolean UIDFetchMessageBody(IMAPStreamPtr imapStream, unsigned long uid, Boolean Peek); // Message without header.

Boolean UIDFetchPartialMessage(IMAPStreamPtr imapStream, unsigned long uid, unsigned long first, unsigned long nBytes, Boolean Peek);
Boolean	UIDFetchPartialMessageBody(IMAPStreamPtr imapStream, unsigned long uid, unsigned long first, unsigned long nBytes, Boolean Peek);
Boolean	UIDFetchPartialBodyText (IMAPStreamPtr imapStream, unsigned long uid, char *section, unsigned long first, unsigned long nBytes, Boolean Peek, Boolean file);

unsigned long FetchUID(IMAPStreamPtr imapStream, unsigned long msgNum);	// sequence-to-UID.

/* These apply to a "message/rfc822"part. (Can also be used for top-level message.)*/
Boolean UIDFetchRFC822Header(IMAPStreamPtr imapStream, unsigned long uid, char *sequence);
Boolean UIDFetchRFC822Text(IMAPStreamPtr imapStream, unsigned long uid, char *sequence);
Boolean UIDFetchRFC822HeaderFields(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, char *Fields);
Boolean UIDFetchRFC822HeaderFieldsNot(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, char *fields);

/* IMAPBODY part level fetches. */
Boolean	UIDFetchMimeHeader(IMAPStreamPtr imapStream, unsigned long uid, char *sequence);
Boolean	UIDFetchBodyText(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, Boolean PEEK);
Boolean	UIDFetchBodyTextInChunks(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, Boolean PEEK, long size);

/* Preamble and trailer text fetch (use complicated method). */
// If "sequence" is NULL, apply to top level message. Must refer to a multipart message,
// an embedded message/rfc822 part that has a multipart body, or a multipart sub-part.
// or part.
Boolean UIDFetchPreamble(IMAPStreamPtr imapStream, unsigned long uid, char *sequence);
Boolean UIDFetchTrailer(IMAPStreamPtr imapStream, unsigned long uid, char *sequence);

/* Methods for STORE'ing flags. */
// For IMAP commands that return new values of flags, the new values are returned in 
// the IMAPFLAGS parameter.  BUG: These methods should be extended to handle uid sets.
Boolean UIDSaveFlags(IMAPStreamPtr imapStream, unsigned long uid, char *uidList, IMAPFLAGS *Flags, Boolean Set, Boolean Silent);
Boolean UIDAddFlags(IMAPStreamPtr imapStream, unsigned long uid, IMAPFLAGS *Flags, Boolean Silent);
Boolean UIDRemoveFlags(IMAPStreamPtr imapStream, unsigned long uid, IMAPFLAGS *Flags, Boolean Silent);

/* COPY/MOVE methods. */
// Destination mailbox MUST be on the same server.
// BUG: Should be extended to handle message sets.
Boolean UIDCopy(IMAPStreamPtr imapStream, char *pUidlist, char *pDestMailbox);

/* Append */
Boolean IMAPAppendMessage(IMAPStreamPtr imapStream, const char* Flags, long seconds, STRING *pMsg);
Boolean UIDFetchPartialContentsToBuffer(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, int first, unsigned long nBytes, char *buffer, unsigned long bufferSize, unsigned long *len);

/* Utilities */
Boolean	 UIDMessageIsMultipart(IMAPStreamPtr stream, unsigned long uid);
long UIDGetTime(IMAPStreamPtr stream, IMAPUID uid);
unsigned long GetRfc822Size(IMAPStreamPtr stream, IMAPUID uid);

/* Return the mailbox's current UIDVALIDITY number. */
// The mailbox must have already been OpenMailbox()'d before this will return a valid value,
// otherwise it returns 0. (Note: UIDVALIDITY cannot be zero (see Imap4rev1 formal spec.)).
UIDVALIDITY	UIDValidity(IMAPStreamPtr stream);

Boolean IsReadOnly(MAILSTREAM *stream);


/* Search a mailbox */
Boolean UIDFind(IMAPStreamPtr stream, const char *headerList, Boolean body, Boolean not, char *string, unsigned long firstUID, unsigned long lastUID, UIDNodeHandle *results);

/*
 *  non_UID functions
 */

Boolean FetchHeader (IMAPStreamPtr stream, unsigned long msgNum);
long FetchMIMEHeader(IMAPStreamPtr stream, unsigned long uid, char* section, unsigned long flags);

/*
 * These routines take the data returned from the IMAP server and do something useful with it.
 */
 
void OrderedInsert(MAILSTREAM *mailStream, unsigned long uid, Boolean seen, Boolean deleted, Boolean flagged, Boolean answered, Boolean draft, Boolean recent, Boolean sent, unsigned long size);

#ifdef DEBUG
long LoMemCheck(void);	// make sure lo mem hasn't gone bad
#endif
#endif //IMAPNETLIB_H