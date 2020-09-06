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


/*
 * Program:	Interactive Message Access Protocol 4rev1 (IMAP4R1) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 June 1988
 * Last Edited:	4 March 1997
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1997 by the University of Washington
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
#include "imap4r1.h"
#include "rfc822.h"
#include "misc.h"
#include "myssl.h"

#pragma segment IMAP

/* allow ucase(NULL) */
#define ucase MyUpperCase

static char *extraheaders =
  " BODY.PEEK[HEADER.FIELDS (Path Message-ID Newsgroups Followup-To References)]";


/* Driver dispatch used by MAIL */

DRIVER imapdriver = 
{
	"imap",							// driver name
	DR_MAIL|DR_NEWS|DR_NAMESPACE,	// driver flags
	(DRIVER *) NULL,				// next driver
	imap_valid,						// mailbox is valid for us
	imap_parameters,				// manipulate parameters
	imap_scan,						// scan mailboxes
	imap_list,						// find mailboxes
	imap_lsub,						// find subscribed mailboxes
	imap_subscribe,					// subscribe to mailbox
	imap_unsubscribe,				// unsubscribe from mailbox
	imap_create,					// create mailbox
	imap_delete,					// delete mailbox
	imap_rename,					// rename mailbox
	imap_status,					// status of mailbox
	imap_open,						// open mailbox
	imap_close,						// close mailbox
	imap_fast,						// fetch message "fast" attributes
	imap_flags,						// fetch message flags
	NIL,							// fetch overview
// JOK - added imap_envelope
	imap_envelope,
	imap_structure,					// fetch message envelopes
	NULL,							// fetch message IMAPheader
	NULL,							// fetch message body
	imap_msgdata,					// fetch partial message
	imap_uid,						// unique identifier
	imap_msgno,						// message number
	imap_flag,						// modify flags
	NULL,							// per-message modify flags
	imap_search,					// search for message based on criteria
	NULL,							// sort messages
	NULL,							// thread messages
	imap_ping,						// ping mailbox to see if still alive
	imap_check,						// check for new messages
	imap_expunge,					// expunge deleted messages
	imap_copy,						// copy messages to another mailbox
	imap_append,					// append string message to mailbox
	imap_gc,						// garbage collect stream
// added these, too ...
	imap_connected,					// return if this stream is connected.
	imap_rfc822size
};

				/* driver parameters */
static unsigned long imap_maxlogintrials = MAXLOGINTRIALS;
static long imap_lookahead = IMAPLOOKAHEAD;
static long imap_uidlookahead = IMAPUIDLOOKAHEAD;
static long imap_defaultport = 0;
static long imap_prefetch = IMAPLOOKAHEAD;
static long imap_closeonerror = NULL;

// UIDPLUS Support
void VerifyUIDValidity(MAILSTREAM *stream, char *pUids, int len);	      
OSErr UIDStringToUIDs(char *pUids, int len, Accumulator *pAccu);
OSErr StoreUIDPLUSResponses(MAILSTREAM *stream, IMAPPARSEDREPLY *reply);

/* IMAP validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NULL otherwise
 */

DRIVER *imap_valid (char *name)
{
  return mail_valid_net (name,&imapdriver,NULL,NULL);
}


/* IMAP manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *imap_parameters (long function,void *value)
{
	switch ((int) function) 
	{
		case SET_MAXLOGINTRIALS:
			imap_maxlogintrials = (long) value;
			break;
		
		case GET_MAXLOGINTRIALS:
			value = (void *) imap_maxlogintrials;
			break;
		
		case SET_LOOKAHEAD:
			imap_lookahead = (long) value;
			break;
		
		case GET_LOOKAHEAD:
			value = (void *) imap_lookahead;
			break;
		
		case SET_UIDLOOKAHEAD:
			imap_uidlookahead = (long) value;
			break;
		
		case GET_UIDLOOKAHEAD:
			value = (void *) imap_uidlookahead;
			break;
		
		case SET_IMAPPORT:
			imap_defaultport = (long) value;
			break;
		
		case GET_IMAPPORT:
			value = (void *) imap_defaultport;
			break;
		
		case SET_PREFETCH:
			imap_prefetch = (long) value;
			break;
		
		case GET_PREFETCH:
			value = (void *) imap_prefetch;
			break;
		
		case SET_CLOSEONERROR:
			imap_closeonerror = (long) value;
			break;
		
		case GET_CLOSEONERROR:
			value = (void *) imap_closeonerror;
			break;
			
		default:
			value = NULL;	// error case
			break;
	}
	return value;
}


/* IMAP scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void imap_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
	imap_list_work (stream,ref,pat,T,contents);
}


/* IMAP list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void imap_list (MAILSTREAM *stream,char *ref,char *pat)
{
	imap_list_work (stream,ref,pat,T,NULL);
}


/* IMAP list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */
void imap_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
	// we don't currently do anything with subscriptions
	// JDB 6-23-98 
#ifdef	NOT_USED
	void *sdb = NULL;
	char *s;
	
	imap_list_work (stream,ref,pat,NULL,NULL);
	
	// only if null stream and * requested 
	if (!stream && !ref && !strcmp (pat,"*") && (s = sm_read (&sdb)))
	{
		do 
			if (imap_valid (s)) mm_lsub (stream,NULL,s,NULL);
		while (s = sm_read (&sdb));	// until no more subscriptions
	}
#endif
}



/* IMAP find list of mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    list flag
 *	    string to scan
 */

void imap_list_work (MAILSTREAM *stream,char *ref,char *pat,long list,  char *contents)
{
 	char *s, prefix[MAILTMPLEN], mbx[MAILTMPLEN];
  	IMAPARG *args[4],aref,apat,acont;

	// Must have an open stream (JOK).
	if (!(stream && LOCAL && stream->transStream)) return;

	// have a reference? 
	if (ref && *ref)
	{	
		if (strlen (ref) >= sizeof (prefix))
			return;
		strcpy (prefix, ref);	// build prefix 
		LOCAL->prefix = prefix;	// note prefix
	}
	else
	{
		LOCAL->prefix = NIL;	// no prefix
	}

	if (contents)
	{		// want to do a scan?
		if (LOCAL->use_scan)
		{	// can we?
			args[0] = &aref; args[1] = &apat; args[2] = &acont; args[3] = NIL;
			aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
			apat.type = LISTMAILBOX; apat.text = (void *) pat;
			acont.type = ASTRING; acont.text = (void *) contents;
			imap_send (stream,"SCAN",args);
		}
		else mm_log ("Scan not valid on this IMAP server",WARN);
	}
	else if (LEVELIMAP4 (stream))
	{// easy if IMAP4 
	    args[0] = &aref; args[1] = &apat; args[2] = NIL;
		aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
		apat.type = LISTMAILBOX; apat.text = (void *) pat;
		imap_send (stream,list ? "LIST" : "LSUB",args);
	}
	else if (LEVEL1176 (stream))
	{// convert to IMAP2 format wildcard
				// kludgy application of reference
		if (ref && *ref)
			sprintf (mbx,"%s%s",ref,pat);
		else
			strcpy (mbx,pat);

		for (s = mbx; *s; s++)
		{
			if (*s == '%') *s = '*';
		}

		args[0] = &apat; args[1] = NIL;
		apat.type = LISTMAILBOX; apat.text = (void *) mbx;
		if (!(list &&		// if list, try IMAP2bis, then RFC-1176 form
			strcmp (imap_send (stream,"FIND ALL.MAILBOXES",args)->key,"BAD")) ||
			!strcmp (imap_send (stream,"FIND MAILBOXES",args)->key,"BAD"))
		{
			LOCAL->rfc1176 = NIL;	// must be RFC-1064
		}
	}

	LOCAL->prefix = NIL;		// no more prefix 

}


/* IMAP subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NULL on failure
 */

long imap_subscribe (MAILSTREAM *stream,char *mailbox)
{
	return imap_manage (stream,mailbox,"Subscribe Mailbox",NULL);
}


/* IMAP unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from manage list
 * Returns: T on success, NULL on failure
 */

long imap_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
	return imap_manage (stream,mailbox,"Unsubscribe Mailbox",NULL);
}


/* IMAP create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NULL on failure
 */

long imap_create (MAILSTREAM *stream,char *mailbox)
{
	return imap_manage (stream,mailbox,"Create",NULL);
}


/* IMAP delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NULL on failure
 */

long imap_delete (MAILSTREAM *stream,char *mailbox)
{
	return imap_manage (stream,mailbox,"Delete",NULL);
}


/* IMAP rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NULL on failure
 */

long imap_rename (MAILSTREAM *stream,char *old,char *newname)
{
	return imap_manage (stream,old,"Rename",newname);
}


/* IMAP manage a mailbox
 * Accepts: mail stream
 *	    mailbox to manipulate
 *	    command to execute
 *	    optional second argument
 * Returns: T on success, NULL on failure
 */

long imap_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2)
{
	MAILSTREAM *st = stream;
	IMAPPARSEDREPLY *reply;
	long ret = NULL;
	char mbx[MAILTMPLEN],mbx2[MAILTMPLEN];
	IMAPARG *args[3],ambx,amb2;

	ambx.type = amb2.type = ASTRING; ambx.text = (void *) mbx;
	amb2.text = (void *) mbx2;
	args[0] = &ambx; args[1] = args[2] = NULL;
	
	// Must have a valid open stream.
	if (!(stream && stream && stream->transStream))
		return NIL;

	// Validate network stuff.
	if ( imapmail_valid_net (stream, &imapdriver, NIL) ) 
    {
		// JOK Don't use what was put into mbx. Assume caller specified a correct mailbox name.
		strcpy (mbx, mailbox);

	    // Also, if second argument, use arg2 as passed in.
		if (arg2)
		{
			strcpy (mbx2, arg2);
			args[1] = &amb2;	/* second arg present? */
		}

		// JOK
		reply = imap_send (stream,command,args);
		if (reply)
		{
			ret = imap_OK (stream, reply);
			mm_log (reply->text,ret ? (long) NIL : IMAP_ERROR);
		}
	}

	return ret;
}


/* IMAP status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NULL on failure
 */

long imap_status (MAILSTREAM *stream,char *mbx,long flags)
{
	IMAPARG *args[3],ambx,aflg;
	char tmp[MAILTMPLEN];
	NETMBX mb;
	long ret = NIL;

	// Must have a valid and open stream (JOK)!!
	if (!(stream && LOCAL && stream->transStream))
		return NIL;

	// must have a mailbox name
	if (!mbx) return NIL;
	
	// Copy stuff into NETMBX.
	imapmail_valid_net_parse (stream, &mb);

	// use the name of the mailbox passed in, please - JDB 060899
	strcpy(mb.mailbox, mbx);
	
	/* can't use stream if not IMAP4rev1, STATUS,
	 * or halfopen and right host
	 */
	if ((!(LEVELSTATUS (stream) || stream->halfopen) || 
			strcmp (ucase (strcpy (tmp,imap_host (stream))),
			    ucase (mb.host))))
	{	
		// JOK - fail,
		return NIL;
	}

	args[0] = &ambx;args[1] = NIL;/* set up first argument as mailbox */

	ambx.type = ASTRING; ambx.text = (void *) mb.mailbox;

	if (LEVELSTATUS (stream))
	{	
		/* have STATUS command? */
		aflg.type = FLAGS; aflg.text = (void *) tmp;
		args[1] = &aflg; args[2] = NIL;
		tmp[0] = tmp[1] = '\0';	/* build flag list */

		if (flags & SA_MESSAGES) strcat (tmp," MESSAGES");
		if (flags & SA_RECENT) strcat (tmp," RECENT");
		if (flags & SA_UNSEEN) strcat (tmp," UNSEEN");
		if (flags & SA_UIDNEXT) strcat (tmp,LEVELIMAP4rev1 (stream) ?
				    "UIDNEXT" : " UID-NEXT");
		if (flags & SA_UIDVALIDITY) strcat (tmp,LEVELIMAP4rev1 (stream) ?
					"UIDVALIDITY" : " UID-VALIDITY");
		tmp[0] = '(';
		strcat (tmp,")");

		/* send "STATUS mailbox flag" */
		if (imap_OK (stream,imap_send (stream,"STATUS",args)))
			ret = T;
	}

	// JOK - Don't do imap2 status.

	return T;
}


/* IMAP authenticate
 * Accepts: stream to authenticate
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    place to return user name
 * Returns: T on success, NULL on failure
 */

unsigned long imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr)
{
	unsigned long trial,ua;
	char tag[16];
	AUTHENTICATOR *at;
	IMAPPARSEDREPLY *reply = nil;
	
	for (ua = LOCAL->use_auth; stream->transStream && ua;) 
	{
		if (!(at = mail_lookup_auth(find_rightmost_bit (&ua) + 1))) fatal ("Authenticator logic bug!");
		trial = 0;
		
		do
		{			
			sprintf (tag,"A%05ld",stream->gensym++);	// gensym a new tag
			sprintf (tmp,"%s AUTHENTICATE %s",tag,at->name);	// build command
			if (imap_soutr (stream,tmp) && (*at->client) (imap_challenge,imap_response,mb,stream,&trial,usr)) 
			{
				// This replaces original (JOK).
				while (1)
				{
					reply = imap_reply (stream, tag);
					if (!reply) break;
	
					if (strcmp(reply->tag, tag)) imap_soutr(stream,"*");
					else break;
				}

#ifdef JOK_ORIGINAL // (JOK)
				while (strcmp ((reply = imap_reply (stream,tag))->tag,tag))
					imap_soutr (stream,"*");
#endif 
				
				// done if got success response
				if (imap_OK (stream,reply)) return T;
				mm_log (reply->text,WARN);
			}
		}
		while (stream->transStream && !LOCAL->byeseen && trial && (trial < imap_maxlogintrials));
	}
	
	if (PrefIsSet(PREF_IMAP_STUPID_PASSWORD))
	{
		// the login failed for some reason.  Wipe the password.
		InvalidatePasswords(false,false,false);	
	}
	else
	{
		// invalidate the password, if we got a real reply back from the server.  The password was bad.
		if (reply && (!reply->fake) && (strcmp(reply->key,"BAD") || strcmp(reply->key,"NO"))) 
			InvalidatePasswords(false,false,false);	
	}
						
	return NULL;	// ran out of authenticators
}


/* IMAP login
 * Accepts: stream to login
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    place to return user name
 * Returns: T on success, NULL on failure
 */

unsigned long imap_login (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr)
{
	unsigned long trial = 0;
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[3];
	IMAPARG ausr,apwd;
	
	//Must have a stream
	if (!stream) return NIL;
	
	ausr.type = apwd.type = ASTRING;
	ausr.text = (void *) usr;
	apwd.text = (void *) tmp;
	args[0] = &ausr; args[1] = &apwd; args[2] = NULL;
	while (stream->transStream && !LOCAL->byeseen && (trial < imap_maxlogintrials))
	{
		tmp[0] = 0;	// get password
		
		// automatic if want anonymous access
		if (mb->anoflag || stream->anonymous) 
		{
			strcpy (usr,"anonymous");
			strcpy (tmp,net_localhost (stream->transStream));
			trial = imap_maxlogintrials;
		}
		else 	// otherwise get the user's login information.
		{	
			mm_login (mb,usr,tmp,trial++);
			
			if (!tmp[0])	// user refused to give a password
			{		
				mm_log ("Login aborted",IMAP_ERROR);
				return NULL;
			}
		}
		
		// send "LOGIN usr tmp"
		if (imap_OK (stream,reply = imap_send (stream,"LOGIN",args))) 
		{	
			// login successful, note if anonymous
			stream->anonymous = strcmp (usr,"anonymous") ? NULL : T;
				
			// If we have succeeded, tell that to the stream
			stream->bAuthenticated = TRUE;
			
			return T;	// successful login
		}
		
		mm_log (reply->text,WARN);
	}
	
	// Make sure the stream knows.
	stream->bAuthenticated	= FALSE;
	stream->bSelected		= FALSE;
	
	if (PrefIsSet(PREF_IMAP_STUPID_PASSWORD))
	{
		// the login failed for some reason.  Wipe the password.
		InvalidatePasswords(false,false,false);	
	}
	else
	{
		// invalidate the password, if we got a real reply back from the server.  The password was bad.
		if (reply && (!reply->fake) && (strcmp(reply->key,"BAD") || strcmp(reply->key,"NO"))) 
			InvalidatePasswords(false,false,false);	
	}
	
	mm_log ("Incorrect password specified.",IMAP_ERROR);
	return NULL;
}


/* Get challenge to authenticator in binary
 * Accepts: stream
 *	    pointer to returned size
 * Returns: challenge or NULL if not challenge
 */

//
// NOTE: (JOK) - This function returns a "ready response" from the server, already
// base64 decoded. That is, the response from the server that follows the "+" sign.
// NOTE: This returns allocated memory that must be freed by the caller!!
// 
void *imap_challenge (void *s, unsigned long *len)
{
	MAILSTREAM *stream = (MAILSTREAM *) s;
	IMAPPARSEDREPLY *reply = NIL;
	char *p = net_getline (stream->transStream);

	// JOK
	if (p)
	{
		reply = imap_parse_reply (stream, p);
	
		if (reply)
		{
			return strcmp (reply->tag,"+") ? NIL :
			    rfc822_base64 ((unsigned char *) reply->text, strlen (reply->text), len);
		}
	}

	return NIL;
}


/* Send authenticator response in BASE64
 * Accepts: MAIL stream
 *	    string to send
 *	    length of string
 * Returns: T if successful, else NULL
 */

long imap_response (void *s,char *response,unsigned long size)
{
	MAILSTREAM *stream = (MAILSTREAM *) s;
	unsigned long i,j,ret;
	char *t,*u;
	if (size)	// make CRLFless BASE64 string
	{
		for (t = (char *) rfc822_binary ((void *) response,size,&i),u = t,j = 0;j < i; j++) 
			if (t[j] > ' ') *u++ = t[j];
		
		*u = '\0';	// tie off string for mm_dlog()
		if (stream->debug) mm_dlog (t);
	
		// append CRLF
		*u++ = '\015'; *u++ = '\012';
		ret = net_sout (stream->transStream,t,i = u - t);
		fs_give ((void **) &t);
	}
	else if (response)
		ret = imap_soutr (stream,"");
	else	// abort requested
		ret = imap_soutr (stream,"*");

	return ret;
}


/* IMAP close
 * Accepts: MAIL stream
 *	    option flags
 */

void imap_close (MAILSTREAM *stream,long options)
{
	IMAPPARSEDREPLY *reply;
	
	if (stream && LOCAL)	// send "LOGOUT"
	{
		if (!LOCAL->byeseen)	// don't even think of doing it if saw a BYE
		{
			// expunge silently if requested
			if (options & CL_EXPUNGE) imap_send (stream,"EXPUNGE",NULL);
			
			// don't bother waiting for the logout response unless we're being polite
			if (!PrefIsSet(PREF_IMAP_POLITE_LOGOUT)) stream->fastLogout = true;
			
			if (stream->transStream && !imap_OK (stream,reply = imap_send (stream,"LOGOUT",NULL))) mm_log (reply->text,WARN);
		}
		
		// close NET connection if still open
		if (stream->transStream) net_close (stream->transStream);
		stream->transStream = NULL;
	
		// reset the stream state
		stream->bConnected		= FALSE;
		stream->bAuthenticated	= FALSE;
		stream->bSelected		= FALSE;

		// throw away any list results.
		if (stream->fListResultsHandle) DisposeMailboxTree(&(stream->fListResultsHandle));
		if (stream->fUIDResults) UID_LL_Zap(&(stream->fUIDResults));
		
		// free up any network data we may have stored
		if (stream->fNetData) DisposeHandle(stream->fNetData);
		
		// free up memory
		if (LOCAL->user) fs_give ((void **) &LOCAL->user);
		if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
		
		// nuke the local data
		fs_give ((void **) &stream->local);
	}
}


/* IMAP fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 *
 * Generally, imap_fetchstructure is preferred
 */
//JDB now returns result

Boolean imap_fast (MAILSTREAM *stream,char *sequence,long flags)
{
	char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[3],aseq,aatt;
	Boolean result = false;
	
	//Format command
	aseq.type = SEQUENCE; aseq.text = (void *) sequence;
	aatt.type = ATOM; aatt.text = (LEVELIMAP4 (stream)) ? (void *) "(FLAGS INTERNALDATE RFC822.SIZE UID)" : (void *) "FAST";
	args[0] = &aseq; args[1] = &aatt; args[2] = NULL;
	
	reply = imap_send(stream, cmd, args);
	if (reply)
	{
		if (imap_OK(stream, reply))
			result = true;

		if (!result)
			mm_log (reply->text,IMAP_ERROR);
	}

	return (result);
}


/* IMAP fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */
//JDB returns result now.

Boolean imap_flags (MAILSTREAM *stream,char *sequence,long flags)
{
	char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[3],aseq,aatt;
	Boolean result = false;
	
	//Format the command.
	aseq.type = SEQUENCE; aseq.text = (void *) sequence;
	aatt.type = ATOM; 
	
	// JDB 1-11-99 fetch the size of the message along with the flags for efficiency
#ifdef	DEBUG
	if (stream->flagsRefN > 0) aatt.text = (void *) "FLAGS";
	else
#endif
	if (!PrefIsSet(PREF_IMAP_NOTHING_BUT_HEAD)) aatt.text = (void *) "(FLAGS RFC822.SIZE)";
	else aatt.text = (void *) "FLAGS";
	
	args[0] = &aseq; args[1] = &aatt; args[2] = NULL;
	
	//Send it.
	reply = imap_send (stream,cmd,args);
	if (reply)
	{
		if ( imap_OK (stream, reply) )
			result = true;

		if (!result)
			mm_log (reply->text,IMAP_ERROR);
	}

	return result;
}


/* IMAP fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

// NOTE:
// JOK - This is a new imap_structure!! It fetches just the body structure.
// The parsing routine allocates a BODY and attaches it to the "current elt" in thye
// stream. We check for the body, detaches it from the elt and passes it to the caller.
// The caller MUST free the body when done with it.
// END NOTE

IMAPBODY *imap_structure (MAILSTREAM *stream,unsigned long msgno, long flags) 
{
	char seq[128],tmp[MAILTMPLEN];
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[3],aseq,aatt;
	IMAPBODY	*b;
	MESSAGECACHE *elt = NULL;

	// Cannot have a zero msgno.
	if (msgno <= 0)
		return NULL;

	// Initialize: "b" is what's returned.
	b = NULL;

	// Setup for IMAP call.
	args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
	aseq.type = SEQUENCE; aseq.text = (void *) seq;
	aatt.type = ATOM; aatt.text = NIL;

	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);

	// Allocate a new one.
	elt = mail_elt (stream);
	if (elt)
	{
		if (flags & FT_UID)
			elt->privat.uid = msgno;
		else
			elt->msgno = msgno;
	}

	// NOTE: "msgno" can be a UID or a message sequence number.
 	sprintf (seq,"%lu",msgno);	/* initial sequence (UID or msgno) */

	// Format command based on server capability.
	// NOTE: Can't handle any IMAP version older than imap2bis!!
	if (LEVELIMAP4 (stream) && (flags & FT_UID))
	{
		sprintf (tmp,"(UID BODYSTRUCTURE)");

		aatt.text = (void *) tmp;	/* do the built command */

		if (!imap_OK (stream, reply = imap_send (stream,"UID FETCH",args)))
		{
			if (reply)
				mm_log (reply->text,IMAP_ERROR);
		}
    }
	else if (LEVELIMAP2bis (stream))
	{
		/* has non-extensive body and no UID. */
		sprintf (tmp,"(BODY)");

		aatt.text = (void *) tmp;	/* do the built command */

		if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
		{
			if (reply)
				mm_log (reply->text,IMAP_ERROR);
		}
    }


	// "b" is what's returned.
	b = NULL;

	// Did we get anything?
	if (stream->CurrentElt)
	{
		// Make sure the UID's or msgno's matched.
		if (flags & FT_UID)
		{
			if (stream->CurrentElt->privat.uid == msgno)
				b = stream->CurrentElt->privat.msg.body;

		}
		else
		{
			if (stream->CurrentElt->msgno == msgno)
				b = stream->CurrentElt->privat.msg.body;
		}

		// Make sure we detach the body pointer if there was one.
		// If it's not our body, we'd want to delete it.
		if (b)
		stream->CurrentElt->privat.msg.body = NULL;

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}

	return b;
}


/* IMAP fetch message data
 * Accepts: MAIL stream
 *	    message number
 *	    section specifier
 *	    offset of first designated byte or 0 to start at beginning
 *	    maximum number of bytes or 0 for all bytes
 *	    lines to fetch if IMAPheader
 *	    flags
 * Returns: T on success, NULL on failure
 */

long imap_msgdata (MAILSTREAM *stream, unsigned long msgno, char *sequence, char *section, unsigned long first,unsigned long last,STRINGLIST *lines,long flags)
{
	char *t,tmp[MAILTMPLEN],part[40];
	char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[5],aseq,aatt,alns,acls;
	
	if (sequence)
	{
		aseq.type = SEQUENCE; 
		aseq.text = (void *) sequence;
	}
	else
	{
		aseq.type = NUMBER; 
		aseq.text = (void *) msgno;
	}
	aatt.type = ATOM;	// assume atomic attribute
	alns.type = LIST; alns.text = (void *) lines;
	acls.type = BODYCLOSE; acls.text = (void *) part;
	args[0] = &aseq; args[1] = &aatt; args[2] = args[3] = args[4] = NULL;
	part[0] = '\0';	// initially no partial specifier
	if (LEVELIMAP4rev1 (stream))	// easy case if IMAP4rev1 server
	{
		aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
		if (lines) 	// want specific IMAPheader lines?
		{
			sprintf (tmp,"%s.FIELDS%s",section,(flags & FT_NOT) ? ".NOT" : "");
			aatt.text = (void *) tmp;
			args[2] = &alns; args[3] = &acls;
		}
		else 
		{
			aatt.text = (void *) section;
			args[2] = &acls;
		}

		if (first || last) sprintf (part,"<%lu.%lu>",first,last ? last:-1);
	}
	else if (!strcmp (section,"HEADER"))	// IMAPBODY.PEEK[HEADER] becomes RFC822.HEADER
	{
		if (flags & FT_PEEK) aatt.text = (void *) "RFC822.HEADER";
		else 
		{
			mm_notify (stream,"[NOTIMAP4] Can't do non-peeking IMAPheader fetch",WARN);
			return NULL;
		}
	}
	else if ((flags & FT_PEEK) && !LEVEL1730 (stream))	// other peeking was introduced in RFC-1730 
	{
		mm_notify (stream,"[NOTIMAP4] Can't do peeking fetch",WARN);
		return NULL;
	}
	else if (!strcmp (section,"TEXT"))	// IMAPBODY[TEXT] becomes RFC822.TEXT
	{
		aatt.text = (void *)((flags & FT_PEEK) ? "RFC822.TEXT.PEEK" : "RFC822.TEXT");
	}
	else if (!section[0])	// IMAPBODY[] becomes RFC822
	{
		aatt.text = (void *)((flags & FT_PEEK) ? "RFC822.PEEK" : "RFC822");
	}
	else if (t = strstr (section,".HEADER"))	// nested IMAPheader
	{
		if (!LEVEL1730 (stream))	// this was introduced in RFC-1730
		{	
			mm_notify (stream,"[NOTIMAP4] Can't do nested IMAPheader fetch",WARN);
			return NULL;
		}
		aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
		args[2] = &acls;	// will need to close section
		aatt.text = (void *) tmp;	// convert .HEADER to .0 for RFC-1730 server
		strncpy (tmp,section,t-section);
		strcpy (tmp+(t-section),".0");
	}
	else if (strstr (section,".MIME") || strstr (section,".TEXT"))	// extended nested text
	{
		mm_notify (stream,"[NOTIMAP4REV1] Can't do extended body part fetch",WARN);
		return NULL;
	}
	else if (LEVELIMAP2bis (stream))	// nested message
	{
		aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
		args[2] = &acls;		/* will need to close section */
		aatt.text = (void *) section;
	}
	else	// ancient server
	{			
		mm_notify (stream,"[NOTIMAP2BIS] Can't do body part fetch",WARN);
		return NULL;
	}
	
	//Note to self: just what exactly happens to the reply after this?  Who cleans it up?
	
	if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))	// send the fetch command
	{
		mm_log (reply->text,IMAP_ERROR);
		return NULL;	// failure
	}
	
	return T;
}

/* IMAP fetch UID
 * Accepts: MAIL stream
 *	    message number
 * Returns: UID
 */

// NOTE: Modified by JOK.

unsigned long imap_uid (MAILSTREAM *stream,unsigned long msgno)
{
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[3],aseq,aatt;
	char seq[MAILTMPLEN];
	unsigned long uid;
	MESSAGECACHE  *elt = NULL;

	/* IMAP2 didn't have UIDs */
	if (!LEVELIMAP4 (stream))
		return msgno;

	// Cannot have a zero msgno.
	if (msgno <= 0)
		return msgno;

	// Initialize:
	uid = 0;

	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);

	// Allocate a new one.
	elt = mail_elt (stream);
	if (elt)
		elt->msgno = msgno;

	// Setup for IMAP call.
 	sprintf (seq, "%lu", msgno);
    aseq.type = SEQUENCE;
	aseq.text = (void *) seq;
    aatt.type = ATOM;
	aatt.text = (void *) "UID";
    args[0] = &aseq;
	args[1] = &aatt;
	args[2] = NIL;

	/* send "FETCH msgno UID" */
    if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
	{
		if (reply)
			mm_log (reply->text,IMAP_ERROR);
	}

	// Did we get anything?
	if (stream->CurrentElt)
	{
		uid = stream->CurrentElt->privat.uid;

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}

	return uid;
}


/* IMAP fetch message number from UID
 * Accepts: MAIL stream
 *	    UID
 * Returns: message number
 */

// NOTE: Modified by JOK.

unsigned long imap_msgno (MAILSTREAM *stream,unsigned long uid)
{
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[3],aseq,aatt;	
	char seq[MAILTMPLEN];
	unsigned long msgno;
	MESSAGECACHE *elt = NULL;

	/* IMAP2 didn't have UIDs */
	if (!LEVELIMAP4 (stream))
		return uid;

	// Initialize
	msgno = 0;

	/* have server hunt for UID */
	aseq.type = SEQUENCE; aseq.text = (void *) seq;
	aatt.type = ATOM; aatt.text = (void *) "UID";
	args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
	sprintf (seq,"%lu",uid);

	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);

	// Allocate a new one.
	elt = mail_elt (stream);
	if (elt)
		elt->privat.uid = uid;


	/* send "UID FETCH uid UID" */
	if (!imap_OK (stream,reply = imap_send (stream,"UID FETCH",args)))
	{
		if (reply)
			mm_log (reply->text,IMAP_ERROR);
	}

	// Did we get anything?
	if (stream->CurrentElt)
	{
		// Make sure the uid's matched.
		if (stream->CurrentElt->privat.uid == uid)
			msgno = stream->CurrentElt->msgno;

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}

	return msgno;			/* didn't find the UID anywhere */
}


/* IMAP modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */
//	JDB - added Boolean return to indicate success

Boolean imap_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
	char *cmd;
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[4],aseq,ascm,aflg;
	Boolean	result = true;
	MESSAGECACHE *elt = NULL;

	// Must have a stream
	if (!stream) return false;
	
	cmd = (LEVELIMAP4 (stream) && (flags & ST_UID)) ? "UID STORE":"STORE";
	
	aseq.type = SEQUENCE; 
	aseq.text = (void *) sequence;
	ascm.type = ATOM; 
	ascm.text = (void *)((flags & ST_SET) ? ((LEVELIMAP4 (stream) && (flags & ST_SILENT)) ? "+Flags.silent" : "+Flags") : ((LEVELIMAP4 (stream) && (flags & ST_SILENT)) ? "-Flags.silent" : "-Flags"));
	aflg.type = FLAGS; 
	aflg.text = (void *) flag;
	args[0] = &aseq; 
	args[1] = &ascm; 
	args[2] = &aflg; 
	args[3] = NULL;
	
	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);

	// Allocate a new one.
	elt = mail_elt (stream);
	
	// send "STORE sequence +Flags flag"
	if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
	{
		result = false;
		mm_log (reply->text,IMAP_ERROR);
	}

	// Did we get anything?
	if (stream->CurrentElt)
	{
		// BUG: Is ST_SILENT is NOT set, we should check to see if the flags were
		// returned from the server correctly.

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}
	return (result);
}


/* IMAP search for messages
 * Accepts: MAIL stream
 *	    character set
 *	    search program
 *	    option flags
 */
// JOK
// This now just sends the command to the server. Untagged respponses will 
// be sent to a callback function.
//
Boolean imap_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags)
{
	Boolean result = true;
	IMAPPARSEDREPLY *reply;
	IMAPARG *args[3],apgm,aseq,aatt;

	args[1] = args[2] = NIL;
	apgm.type = SEARCHPROGRAM; apgm.text = (void *) pgm;
	aseq.type = SEQUENCE;
	aatt.type = ATOM;

	if (charset)
	{
		args[0] = &aatt; args[1] = &apgm;
		aatt.text = (void *) charset;
	}
	else
		args[0] = &apgm;

	/* do the SEARCH */
	if (!imap_OK (stream, reply = imap_send (stream, (flags & SE_UID) ? "UID SEARCH" : "SEARCH", args)))
	{
		if (reply)
		{
			mm_log (reply->text,IMAP_ERROR);
			result = false;	// the search failed.
		}
	}
	
	return (result);
}


/* IMAP ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, else NULL
 */

long imap_ping (MAILSTREAM *stream)
{
  return (stream->transStream &&	/* send "NOOP" */
	  imap_OK (stream,imap_send (stream,"NOOP",NULL))) ? T : NULL;
}


/* IMAP check mailbox
 * Accepts: MAIL stream
 */

void imap_check (MAILSTREAM *stream)
{
				/* send "CHECK" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"CHECK",NULL);
  mm_log (reply->text,imap_OK (stream,reply) ? (long) NULL : IMAP_ERROR);
}


/* IMAP expunge mailbox
 * Accepts: MAIL stream
 */

long imap_expunge (MAILSTREAM *stream)
{
	long result = NULL;
	
	/* send "EXPUNGE" */
  	IMAPPARSEDREPLY *reply = imap_send (stream,"EXPUNGE",NULL);
  
  	result = imap_OK (stream,reply) ? (long) NULL : IMAP_ERROR;
  	mm_log (reply->text,result);
  	
  	return (result ? NULL : T);
}


/* IMAP copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    option flags
 * Returns: T if successful else NULL
 */

long imap_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long flags)
{
  char *cmd = (LEVELIMAP4 (stream) && (flags & CP_UID)) ? "UID COPY" : "COPY";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,ambx;
  
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ambx.type = ASTRING; ambx.text = (void *) mailbox;
  args[0] = &aseq; args[1] = &ambx; args[2] = NULL;
				/* send "COPY sequence mailbox" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args))) {
    mm_log (reply->text,IMAP_ERROR);
    return NULL;
  }
  				/* check for UIDPLUS responses */
  StoreUIDPLUSResponses(stream, reply);
  
				/* delete the messages if the user said to */
  if (flags & CP_MOVE) imap_flag (stream,sequence,"\\Deleted",
				  ST_SET + ((flags & CP_UID) ? ST_UID : NULL));
  return T;
}


/* IMAP append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NULL on failure
 */

 /* Modified by JOK, May, 1997. */

long imap_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *msg)
{
	char tmp[MAILTMPLEN];
	long ret = NIL;
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[5],ambx,aflg,adat,amsg;

// START JOK
	// Must have a valid mailbox
	if (!mailbox)
		return ret;
	// Copy mailbox into tmp.
	strcpy (tmp, mailbox);

// END JOK

	/* Must have a valid and open stream. (JOK) */
	if (IsConnected(stream) && !stream->halfopen)
	{
		if (imapmail_valid_net (stream, &imapdriver,NIL))
		{
			ambx.type = ASTRING; ambx.text = (void *) tmp;
			aflg.type = FLAGS; aflg.text = (void *) flags;
			adat.type = ASTRING; adat.text = (void *) date;
			amsg.type = LITERAL; amsg.text = (void *) msg;
			if (flags || date)
			{	/* IMAP4 form? */
				int i = 0;
				args[i++] = &ambx;
				if (flags) args[i++] = &aflg;
				if (date) args[i++] = &adat;
				args[i++] = &amsg;
				args[i++] = NIL;

				reply = imap_send (stream,"APPEND",args);
				if ( reply && !strcmp (reply->key,"OK") )
					ret = T;
			}

			/* try IMAP2bis form if IMAP4 form fails */
			if (!(ret || (reply && strcmp (reply->key,"BAD"))))
			{
				args[0] = &ambx; args[1] = &amsg; args[2] = NIL;

				if (imap_OK (stream, reply = imap_send (stream,"APPEND",args)))
					ret = T;
			}
			if (!ret)
			{
				if (reply)
					mm_log (reply->text,IMAP_ERROR);
			}
		}
	}
	else
		mm_log ("Can't access server for append",IMAP_ERROR);

  return ret;			/* return */
}

/* IMAP garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void imap_gc (MAILSTREAM *stream,long gcflags)
{
	unsigned long i;
	MESSAGECACHE *elt;
	mailcache_t mc = (mailcache_t) mail_parameters (NULL,GET_CACHE,NULL);
	
	// make sure the cache is large enough
	(*mc) (stream,stream->nmsgs,CH_SIZE);
	if (gcflags & GC_TEXTS)	// garbage collect texts?
	{
		if (!stream->scache) for (i = 1; i <= stream->nmsgs; ++i)
		if (elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT))
		imap_gc_body (elt->privat.msg.body);
		imap_gc_body (stream->body);
	}
	
	// gc cache if requested and unlocked
	if (gcflags & GC_ELT) for (i = 1; i <= stream->nmsgs; ++i)
	if ((elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT)) && (elt->lockcount == 1)) (*mc) (stream,i,CH_FREE);
}


/* IMAP garbage collect body texts
 * Accepts: body to GC
 */

void imap_gc_body (IMAPBODY *body)
{
	PART *part;
	
	if (body)	// have a body?
	{
		if (body->mime.text.data)	// flush MIME data
		fs_give ((void **) &body->mime.text.data);
		
		// flush text contents
		if (body->contents.text.data)
		fs_give ((void **) &body->contents.text.data);
		body->mime.text.size = body->contents.text.size = 0;
		
		// multipart?
		if (body->type == TYPEMULTIPART)
		{
			for (part = body->nested.part; part; part = part->next) imap_gc_body (&part->body);
		}
		
		else if ((body->type == TYPEMESSAGE) && !strcmp (body->subtype,"RFC822")) // MESSAGE/RFC822?
		{
			imap_gc_body (body->nested.msg->body);
			if (body->nested.msg->full.text.data)
			fs_give ((void **) &body->nested.msg->full.text.data);
			if (body->nested.msg->IMAPheader.text.data)
			fs_give ((void **) &body->nested.msg->IMAPheader.text.data);
			if (body->nested.msg->text.text.data)
			fs_give ((void **) &body->nested.msg->text.text.data);
			body->nested.msg->full.text.size = body->nested.msg->IMAPheader.text.size = body->nested.msg->text.text.size = 0;
		}
	}
}


/* Internal routines */

/* IMAP send atom-string
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    atom-string to output
 *	    string length
 *	    flag if list_wildcards allowed
 * Returns: error reply or NULL if success
 */

IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    char *t,unsigned long size,long wildok)
{
	unsigned long j;
	STRING st;
	int quoted = size ? NULL : T;	// default to not quoted unless empty
	
	for (j = 0; j < size; j++) 
		switch (t[j]) 
		{
			case '\0':					// not a CHAR
			case '\012': case '\015':	// not a TEXT-CHAR
			case '"': case '\\':		// quoted-specials (IMAP2 required this)
				INIT (&st,mail_string,(void *) t,size);
				return imap_send_literal (stream,tag,s,&st);
	
			default:					// all other characters
				if (t[j] > ' ') break;	// break if not a CTL
			
			case '*': case '%':			// list_wildcards
				if (wildok) break;		// allowed if doing the wild thing

			// atom_specials
			case '(': case ')': case '{': case ' ':
				quoted = T;				// must use quoted string format
				break;
	}
	
	if (quoted) *(*s)++ = '"';		// write open quote
	while (size--) *(*s)++ = *t++;	// write the characters
	if (quoted) *(*s)++ = '"';		// write close quote
	
	return NULL;
}


/* IMAP send literal
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    literal to output as stringstruct
 * Returns: error reply or NULL if success
 */

IMAPPARSEDREPLY *imap_send_literal (MAILSTREAM *stream,char *tag,char **s,STRING *st)
{
	IMAPPARSEDREPLY *reply;
	unsigned long i = SIZE (st);
	
	sprintf (*s,"{%ld}",i);	// write literal count
	*s += strlen (*s);		// size of literal count
	
	// send the command
	reply = imap_sout (stream,tag,LOCAL->tmp,s);
	if (strcmp (reply->tag,"+"))	// prompt for more data?
	{
		//No longer
		//mail_unlock (stream);	// no, give up
		return reply;
	}
	
	while (i) 	// dump the text
	{	
		if (!net_sout (stream->transStream,st->curpos,st->cursize)) 
		{
			//No longer 
			//mail_unlock (stream);
			return imap_fake (stream,tag,"IMAP connection broken (data)");
		}
	
		i -= st->cursize;	// note that we wrote out this much
		st->curpos += (st->cursize - 1);
		st->cursize = 0;
		(*st->dtb->next) (st);	// advance to next buffer's worth
	}
	
	return NULL;	// success
}

/* IMAP send search program
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    search program to output
 * Returns: error reply or NIL if success
 */

IMAPPARSEDREPLY *imap_send_spgm (MAILSTREAM *stream,char *tag,char **s,
				 SEARCHPGM *pgm)
{
	IMAPPARSEDREPLY *reply;
	SEARCHHEADER *hdr;
	SEARCHOR *pgo;
	SEARCHPGMLIST *pgl;
//	char *t = "ALL";
	char *t = "";

	while (*t)
		 *(*s)++ = *t++;	/* default initial text */

	/* message sequences */
	if (pgm->msgno)
		imap_send_sset (s,pgm->msgno);

	if (pgm->uid)
	{		/* UID sequence */
		for (t = "UID "; *t; *(*s)++ = *t++);

		imap_send_sset (s,pgm->uid);
	}
				/* message sizes */
	if (pgm->larger)
	{
		sprintf (*s,"LARGER %lu",pgm->larger);
		*s += strlen (*s);
	}

	if (pgm->smaller)
	{
		sprintf (*s,"SMALLER %lu",pgm->smaller);
		*s += strlen (*s);
	}

	/* message flags */
	if (pgm->answered) for (t = "ANSWERED "; *t; *(*s)++ = *t++);
	if (pgm->unanswered) for (t ="UNANSWERED "; *t; *(*s)++ = *t++);
	if (pgm->deleted) for (t ="DELETED "; *t; *(*s)++ = *t++);
	if (pgm->undeleted) for (t ="UNDELETED "; *t; *(*s)++ = *t++);
	if (pgm->draft) for (t ="DRAFT "; *t; *(*s)++ = *t++);
	if (pgm->undraft) for (t ="UNDRAFT "; *t; *(*s)++ = *t++);
	if (pgm->flagged) for (t ="FLAGGED "; *t; *(*s)++ = *t++);
	if (pgm->unflagged) for (t ="UNFLAGGED "; *t; *(*s)++ = *t++);
	if (pgm->recent) for (t ="RECENT "; *t; *(*s)++ = *t++);
	if (pgm->old) for (t ="OLD "; *t; *(*s)++ = *t++);
	if (pgm->seen) for (t ="SEEN "; *t; *(*s)++ = *t++);
	if (pgm->unseen) for (t ="UNSEEN "; *t; *(*s)++ = *t++);
	if ((pgm->keyword &&		/* keywords */
		(reply = imap_send_slist (stream,tag,s,"KEYWORD",pgm->keyword))) ||
      (pgm->unkeyword &&
		(reply = imap_send_slist (stream,tag,s,"UNKEYWORD",pgm->unkeyword))))
		return reply;


	/* sent date ranges */
	if (pgm->sentbefore) imap_send_sdate (s,"SENTBEFORE",pgm->sentbefore);
	if (pgm->senton) imap_send_sdate (s,"SENTON",pgm->senton);
	if (pgm->sentsince) imap_send_sdate (s,"SENTSINCE",pgm->sentsince);
				/* internal date ranges */
	if (pgm->before) imap_send_sdate (s,"BEFORE",pgm->before);
	if (pgm->on) imap_send_sdate (s,"ON",pgm->on);
	if (pgm->since) imap_send_sdate (s,"SINCE",pgm->since);
				/* search texts */
	if ((pgm->bcc && (reply = imap_send_slist (stream,tag,s,"BCC",pgm->bcc))) ||
		(pgm->cc && (reply = imap_send_slist (stream,tag,s,"CC",pgm->cc))) ||
		(pgm->from && (reply = imap_send_slist(stream,tag,s,"FROM",pgm->from)))||
		(pgm->to && (reply = imap_send_slist (stream,tag,s,"TO",pgm->to))))
		return reply;

	if ((pgm->subject &&
		(reply = imap_send_slist (stream,tag,s,"SUBJECT",pgm->subject))) ||
		(pgm->body && (reply = imap_send_slist(stream,tag,s,"BODY",pgm->body)))||
		(pgm->text && (reply = imap_send_slist (stream,tag,s,"TEXT",pgm->text))))
		return reply;

	if (hdr = pgm->IMAPheader) do
	{
		for (t = "HEADER "; *t; *(*s)++ = *t++);

		for (t = hdr->line; *t; *(*s)++ = *t++);

		// JOK - Add a space!!
		for (t = " "; *t; *(*s)++ = *t++);

		if (reply = imap_send_astring (stream,tag,s,hdr->text,
				   (unsigned long) strlen (hdr->text),NIL))
			return reply;
	}
	while (hdr = hdr->next);

	if (pgo = pgm->or) do {
		for (t = "OR ("; *t; *(*s)++ = *t++);
		if (reply = imap_send_spgm (stream,tag,s,pgm->or->first)) return reply;
		for (t = ") ("; *t; *(*s)++ = *t++);

		if (reply = imap_send_spgm (stream,tag,s,pgm->or->second)) return reply;
		*(*s)++ = ')';
	} while (pgo = pgo->next);

	if (pgl = pgm->not) do {
		for (t = "NOT ("; *t; *(*s)++ = *t++);
		if (reply = imap_send_spgm (stream,tag,s,pgl->pgm)) return reply;
		*(*s)++ = ')';
	}
	while (pgl = pgl->next);

	return NIL;			/* search program written OK */
}



/* IMAP send search set
 * Accepts: pointer to current position pointer of output bigbuf
 *	    search set to output
 */

void imap_send_sset (char **s,SEARCHSET *set)
{
	char c = 0;
	char *t;
	
	// Sanity: Must have at least these.
	if (!(set && set->first))
	{
		ASSERT (0);
		return;
	}

	do {				/* run down search set */
		if (c)
		{
			// If last is 0xFFFFFFFF, replace by "*". (JOK)
			//
			if (set->last == 0xFFFFFFFF)
			{
				sprintf (*s, "%c%lu:*", c, set->first);
			}
			else
			{
				sprintf (*s, set->last ? "%c%lu:%lu" : "%c%lu",c,set->first, set->last);
			}
		}
		else
		{
			if (set->last == 0xFFFFFFFF)
			{
				sprintf (*s, "%lu:*",set->first);
			}
			else
			{
				sprintf (*s, set->last ? "%lu:%lu" : "%lu",set->first,set->last);
			}
		}

		*s += strlen (*s);
		c = ',';			/* if there are any more */
	}
	while (set = set->next);

	// (JOK) Add a space after the list.
	for (t = " "; *t; *(*s)++ = *t++);
}


/* IMAP send search list
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    name of search list
 *	    search list to output
 * Returns: NIL if success, error reply if error
 */

IMAPPARSEDREPLY *imap_send_slist (MAILSTREAM *stream,char *tag,char **s,
				  char *name,STRINGLIST *list)
{
	char *t;
	IMAPPARSEDREPLY *reply = NIL;

	do {
// (JOK) Space screws up some servers!!	    *(*s)++ = ' ';		/* output name of search list */
	    for (t = name; *t; *(*s)++ = *t++);

		*(*s)++ = ' ';
		reply=imap_send_astring (stream,tag,s,list->text.data,list->text.size,NIL);
	}
	while (!reply && (list = list->next));

	return reply;
}


/* IMAP send search date
 * Accepts: pointer to current position pointer of output bigbuf
 *	    field name
 *	    search date to output
 */

void imap_send_sdate (char **s,char *name,unsigned short date)
{
  sprintf (*s," %s %d-%s-%d",name,date & 0x1f,
	   months[((date >> 5) & 0xf) - 1],BASEYEAR + (date >> 9));
  *s += strlen (*s);
}


/* IMAP send null-terminated string to sender
 * Accepts: MAIL stream
 *	    string
 * Returns: T if success, else NIL
 */

long imap_soutr (MAILSTREAM *stream,char *string)
{
	char tmp[MAILTMPLEN];
	
	if (stream->debug) mm_dlog (string);
	return (net_soutr (stream->transStream,strcat (strcpy (tmp,string),"\015\012")));
}


/* IMAP fake reply
 * Accepts: MAIL stream
 *	    tag
 *	    text of fake reply
 * Returns: parsed reply
 *
 *	Added field to IMAPPARSEDREPLY to distinguish a fake reply from a real one -jdboyd 01/19/00
 */

IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text)
{
	IMAPPARSEDREPLY *reply = nil;
	
	mm_notify (stream,text,BYE);	// send bye alert
	
	// don't kill the net stream anymore.  Take care of this elsewhere. -JDB
	
	//if (stream->transStream) net_close (stream->transStream);
	//stream->transStream = NIL;	// farewell, dear NET stream...
	
	// build fake reply string
	sprintf (LOCAL->tmp,"%s NO [CLOSED] %s",tag ? tag : "*",text);
	
	// parse and return it
	reply = imap_parse_reply (stream,cpystr (LOCAL->tmp));
	if (reply) reply->fake = true;
	
	return (reply);	
}


/* IMAP message has been expunged
 * Accepts: MAIL stream
 *	    message number
 *
 * Calls external "mail_expunged" function to notify main program
 */

void imap_expunged (MAILSTREAM *stream,unsigned long msgno)
{
 	// Must have a stream.
	if (!stream) return;

	// All we do is to pass this to the upper layers.
	mail_expunged (stream, msgno);
}


/* IMAP parse data
 * Accepts: MAIL stream
 *	    message #
 *	    text to parse
 *	    parsed reply
 *
 *  This code should probably be made a bit more paranoid about malformed
 * S-expressions.
 */

void imap_parse_data (MAILSTREAM *stream,unsigned long msgno,char *text,IMAPPARSEDREPLY *reply)
{
	char *prop;
	MESSAGECACHE *elt = mail_elt (stream);
	
	if (!elt) return;
	
	// Must have a reply ...
	if (!reply) return;
	
	++text;	// skip past open parenthesis
	
	// parse Lisp-form property list
	while (prop = (char *) strtok (text," )")) 
	{
		// point at value
		text = (char *) strtok (NIL,"\n");
		// parse the property and its value
		imap_parse_prop (stream,elt,ucase (prop),&text,reply);
	}
}


/* IMAP parse property
 * Accepts: MAIL stream
 *	    cache item
 *	    property name
 *	    property value text pointer
 *	    parsed reply
 */

void imap_parse_prop (MAILSTREAM *stream,MESSAGECACHE *elt,char *prop,char **txtptr,IMAPPARSEDREPLY *reply)
{
	char *s;
	ENVELOPE **env;
	IMAPBODY **body;
	GETS_DATA md;
	
	// Make sure we have a reply
	if (!reply) return;
		
	INIT_GETS (md,stream,elt->msgno,NIL,0,0);
	
	if (!strcmp (prop,"ENVELOPE")) 
	{
		imapenvelope_t ie =
		(imapenvelope_t) mail_parameters (NIL,GET_IMAPENVELOPE,NIL);
	
		if (stream->scache)	// short cache, flush old stuff
		{
			mail_free_body (&stream->body);
			stream->msgno=elt->msgno;	// set new current message number
			env = &stream->env;	 // get pointer to envelope
		}
		else env = &elt->privat.msg.env;
		imap_parse_envelope (stream,env,txtptr,reply);
		
		// do callback if requested
		if (ie) (*ie) (stream,elt->msgno,*env);
	}
	else if (!strcmp (prop,"FLAGS"))
	{
		imap_parse_flags (stream,elt,txtptr);

		// JOK (6/23/97) Pass the elt to top level to record the info.

		// Do this only if the uid has already been obtained and flags are valid.
		if (elt->privat.uid && elt->valid)
		{
			mm_elt_flags (stream, elt);

			// After we have notified the upper layers, reset the elt's status
			// because it mey be re-used.
			elt->privat.uid = 0;
			elt->valid = NIL;
		}

	} 
	else if (!strcmp (prop,"INTERNALDATE")) 
	{
		if (s = imap_parse_string (stream,txtptr,reply,NIL,NIL)) 
		{
			if (!mail_parse_date (elt,s)) 
			{
				sprintf (LOCAL->tmp,"Bogus date: %.80s",s);
				mm_log (LOCAL->tmp,WARN);
			}
			fs_give ((void **) &s);
		}
	}
	else if (!strcmp (prop,"UID"))	// unique identifier
	{
		elt->privat.uid = strtoul (*txtptr,txtptr,10);

		// JOK (6/23/97) Pass the elt to top level to record the info.

		// Do this only if the uid has already been obtained and flags are valid.
		if (elt->privat.uid && elt->valid)
		{
			mm_elt_flags (stream, elt);

			// After we have notified the upper layers, reset the elt's status
			// because it mey be re-used.
			elt->privat.uid = 0;
			elt->valid = NIL;
		}
		
		// return the UID in the stream if we're chunking headers
		if (stream->chunkHeaders)
		{
			stream->headerUID = elt->privat.uid;
		}
	}
	else if (!strcmp (prop,"RFC822.HEADER") || !strcmp (prop,"BODY[HEADER]") || !strcmp (prop,"BODY[0]")) 
	{
		if (elt->privat.msg.IMAPheader.text.data)
		fs_give ((void **) &elt->privat.msg.IMAPheader.text.data);
		md.what = "HEADER";
		
		elt->privat.msg.IMAPheader.text.data = imap_parse_string (stream,txtptr,reply,&md,&elt->privat.msg.IMAPheader.text.size);
	}
	else if (!strcmp (prop,"RFC822.SIZE"))
		elt->rfc822_size = strtoul (*txtptr,txtptr,10);
	else if (!strcmp (prop,"RFC822.TEXT") || !strcmp (prop,"BODY[TEXT]")) 
	{
		if (elt->privat.msg.text.text.data)
		fs_give ((void **) &elt->privat.msg.text.text.data);
		md.what = "TEXT";
		elt->privat.msg.text.text.data = imap_parse_string (stream,txtptr,reply,&md,&elt->privat.msg.text.text.size);
	}
	else if (!strcmp (prop,"RFC822") || !strcmp (prop,"BODY[]")) 
	{
		if (elt->privat.msg.full.text.data)
		fs_give ((void **) &elt->privat.msg.full.text.data);
		md.what = "";
		
		elt->privat.msg.full.text.data = imap_parse_string (stream,txtptr,reply,&md,&elt->privat.msg.full.text.size);
	}
	else if (prop[0] == 'B' && prop[1] == 'O' && prop[2] == 'D' && prop[3] == 'Y') 
	{
		s = cpystr (prop+4);	// copy segment specifier
		if (stream->scache) 	// short cache, flush old stuff
		{
			if (elt->msgno != stream->msgno) 
			{
				/* losing real bad here */
				mail_free_envelope (&stream->env);
				sprintf (LOCAL->tmp,"Body received for %lu when current is %lu",
				elt->msgno,stream->msgno);
				mm_log (LOCAL->tmp,WARN);
				stream->msgno = elt->msgno;
			}
			body = &stream->body;	/* get pointer to body */
		}
		else body = &elt->privat.msg.body;
		imap_parse_body (&md,body,s,txtptr,reply);
		fs_give ((void **) &s);
	}
	else 
	{
		sprintf (LOCAL->tmp,"Unknown message property: %.80s",prop);
		mm_log (LOCAL->tmp,WARN);
	}
}


/* Parse RFC822 message IMAPheader
 * Accepts: MAIL stream
 *	    envelope to parse into
 *	    IMAPheader as sized text
 */

void imap_parse_header (MAILSTREAM *stream,ENVELOPE **env,SIZEDTEXT *hdr)
{
	ENVELOPE *nenv;
	
	// parse what we can from this IMAPheader
	rfc822_parse_msg (&nenv,NIL,hdr->data,hdr->size,NIL,imap_host (stream));
	
	if (*env)	// need to merge this IMAPheader into envelope?
	{			
		if (!(*env)->newsgroups) 	// need Newsgroups?
		{
			(*env)->newsgroups = nenv->newsgroups;
			nenv->newsgroups = NIL;
		}
		if (!(*env)->followup_to)	// need Followup-To?
		{
			(*env)->followup_to = nenv->followup_to;
			nenv->followup_to = NIL;
		}
		if (!(*env)->references)	// need References?
		{
			(*env)->references = nenv->references;
			nenv->references = NIL;
		}
		mail_free_envelope (&nenv);
	}
	else *env = nenv;	// otherwise set it to this
}


/* IMAP parse envelope
 * Accepts: MAIL stream
 *	    pointer to envelope pointer
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,IMAPPARSEDREPLY *reply)
{
	ENVELOPE *oenv = *env;
	char c = *((*txtptr)++);	// grab first character
	
	// ignore leading spaces
	while (c == ' ') c = *((*txtptr)++);
	
	switch (c)	// dispatch on first character
	{
		case '(':	// if envelope S-expression
			*env = mail_newenvelope ();	// parse the new envelope
			(*env)->date = imap_parse_string (stream,txtptr,reply,NIL,NIL);
			(*env)->subject = imap_parse_string (stream,txtptr,reply,NIL,NIL);
			(*env)->from = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->sender = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->reply_to = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->to = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->cc = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->bcc = imap_parse_adrlist (stream,txtptr,reply);
			(*env)->in_reply_to = imap_parse_string (stream,txtptr,reply,NIL,NIL);
			(*env)->message_id = imap_parse_string (stream,txtptr,reply,NIL,NIL);
			if (oenv)	// need to merge old envelope?
			{
				(*env)->newsgroups = oenv->newsgroups;
				oenv->newsgroups = NIL;
				(*env)->followup_to = oenv->followup_to;
				oenv->followup_to = NIL;
				(*env)->references = oenv->references;
				oenv->references = NIL;
				mail_free_envelope(&oenv);	// free old envelope
			}
			if (**txtptr != ')') 
			{
				sprintf (LOCAL->tmp,"Junk at end of envelope: %.80s",*txtptr);
				mm_log (LOCAL->tmp,WARN);
			}
			else ++*txtptr;	// skip past delimiter
			break;
		
		case 'N':	// if NIL
		case 'n':
			++*txtptr;			
			++*txtptr;	
			break;
		
		default:
			sprintf (LOCAL->tmp,"Not an envelope: %.80s",*txtptr);
			mm_log (LOCAL->tmp,WARN);
			break;
	}
}


/* IMAP parse address list
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address list, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	ADDRESS *adr = NIL;
	char c = **txtptr;	// sniff at first character
	
	// ignore leading spaces
	while (c == ' ') c = *++*txtptr;
	++*txtptr;	// skip past open paren
	switch (c) 
	{
		case '(':	// if envelope S-expression
			adr = imap_parse_address (stream,txtptr,reply);
			if (**txtptr != ')') // need to merge old envelope?
			{
				sprintf (LOCAL->tmp,"Junk at end of address list: %.80s",*txtptr);
				mm_log (LOCAL->tmp,WARN);
			}
			else ++*txtptr;		// skip past delimiter
			break;
		
		case 'N':	//If NIL
		case 'n':
			++*txtptr;		
			++*txtptr;
			break;
			
		default:
			sprintf (LOCAL->tmp,"Not an address: %.80s",*txtptr);
			mm_log (LOCAL->tmp,WARN);
			break;
	}
	return adr;
}


/* IMAP parse address
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	ADDRESS *adr = NIL;
	ADDRESS *ret = NIL;
	ADDRESS *prev = NIL;
		
	char c = **txtptr;		/* sniff at first address character */
	switch (c) 
	{
		case '(':	// if envelope S-expression
			while (c == '(')	// recursion dies on small stack machines
			{		
				++*txtptr;	// skip past open paren
				if (adr) prev = adr;	// note previous if any
				adr = mail_newaddr ();	// instantiate address and parse its fields
				adr->personal = imap_parse_string (stream,txtptr,reply,NIL,NIL);
				adr->adl = imap_parse_string (stream,txtptr,reply,NIL,NIL);
				adr->mailbox = imap_parse_string (stream,txtptr,reply,NIL,NIL);
				adr->host = imap_parse_string (stream,txtptr,reply,NIL,NIL);
				if (**txtptr != ')')	// handle trailing paren
				{	
					sprintf (LOCAL->tmp,"Junk at end of address: %.80s",*txtptr);
					mm_log (LOCAL->tmp,WARN);
				}
				else ++*txtptr;		// skip past close paren
				c = **txtptr;		// set up for while test
				// ignore leading spaces in front of next
				while (c == ' ') c = *++*txtptr;
				if (!ret) ret = adr;	// if first time note first adr
				// if previous link new block to it
				if (prev) prev->next = adr;
			}
			break;

		case 'N':	//Nil
		case 'n':
			*txtptr += 3;	// bump past NIL
			break;
		
		default:
			sprintf (LOCAL->tmp,"Not an address: %.80s",*txtptr);
			mm_log (LOCAL->tmp,WARN);
			break;
	}
	return ret;
}

/* IMAP parse flags
 * Accepts: current message cache
 *	    current text pointer
 *
 * Updates text pointer
 */

void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr)
{
	char *flag;
	char c = '\0';
	Str255 sentFlag;
	
	// read the flag that we'll treat as //Sent from the settings ...
	GetRString(sentFlag, IMAP_SENT_FLAG);		
	if (sentFlag[0]) 
	{
		sentFlag[sentFlag[0] + 1] = 0;
		ucase(sentFlag);
	}
								
	elt->valid = T;	// mark have valid flags now
	elt->user_flags = NIL;	// zap old flag values
	elt->seen = elt->deleted = elt->flagged = elt->answered = elt->recent = elt->sent = NIL;

#ifdef	DEBUG
	if (stream->flagsRefN > 0)
	{
		char *s = *txtptr;
		long count = strlen(*txtptr);
		
		while ((s < *txtptr + count) && (*s != ')')) s++;
		if (*s==')') s++;
		
		count = s - *txtptr;
		
		// write the line to the spool file
		if (count) 
		{
			AWrite(stream->flagsRefN,&count,*txtptr);
			FSWriteP(stream->flagsRefN,Cr);
		}
	}
#endif

	while (c != ')') 	// parse list of flags
	{
		// point at a flag
		while (*(flag = ++*txtptr) == ' ');
	
		// scan for end of flag
		while (**txtptr != ' ' && **txtptr != ')') ++*txtptr;
	
		c = **txtptr;		// save delimiter
		**txtptr = '\0';	// tie off flag
		if (!*flag) break;	// null flag
 		else if (*ucase (flag) == '\\')		// if starts with \ must be sys flag
 		{
 			if (sentFlag[0] && !strcmp (flag,sentFlag+1)) elt->sent = T;
			else if (!strcmp (flag,"\\SEEN")) elt->seen = T;
			else if (!strcmp (flag,"\\DELETED")) elt->deleted = T;
			else if (!strcmp (flag,"\\FLAGGED")) elt->flagged = T;
			else if (!strcmp (flag,"\\ANSWERED")) elt->answered = T;
			else if (!strcmp (flag,"\\RECENT")) elt->recent = T;
			else if (!strcmp (flag,"\\DRAFT")) elt->draft = T;
		}
		else	// otherwise user flag
			elt->user_flags |= imap_parse_user_flag (stream,flag);
	}
	++*txtptr;	// bump past delimiter
	mm_flags (stream,elt->msgno);	// make sure top level knows
}


/* IMAP parse user flag
 * Accepts: MAIL stream
 *	    flag name
 * Returns: flag bit position
 */

unsigned long imap_parse_user_flag (MAILSTREAM *stream,char *flag)
{
	char tmp[MAILTMPLEN];
	long i;
	
	// sniff through all user flags
	for (i = 0; i < NUSERFLAGS; ++i)
		if (stream->user_flags[i] && !strcmp (flag,ucase (strcpy (tmp,stream->user_flags[i])))) return (1 << i);	// found it! 
	
	return (unsigned long) 0;	// not found
}

/* IMAP parse string
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *	    mailgets data
 *	    returned string length
 * Returns: string
 *
 * Updates text pointer
 */

char *imap_parse_string (MAILSTREAM *stream,char **txtptr, IMAPPARSEDREPLY *reply,GETS_DATA *md, unsigned long *len)
{
	char *st;
	char *string = NIL;
	unsigned long i,j,k;
	char c = **txtptr;	// sniff at first character
	
	mailgets_t mg = (mailgets_t) mail_parameters (stream,GET_GETS,NIL);
	readprogress_t rp = (readprogress_t) mail_parameters (NIL,GET_READPROGRESS,NIL);
	
	while (c == ' ') c = *++*txtptr;	// ignore leading spaces
	st = ++*txtptr;	// remember start of string
	switch (c) 
	{
		case '"':	// if quoted string
			i = 0;			// initial byte count
		while (**txtptr != '"')	// search for end of string
		{
			if (**txtptr == '\\') ++*txtptr;
			++i;			// bump count
			++*txtptr;		// bump pointer
		}
		++*txtptr;	// bump past delimiter

		// JOK - If we have a mailgets, do that instead.
		if (md && mg)
		{		
			// Allocate a ParenStr data object to send data to caller.
			ParenStrData strData;
			
			md->flags |= MG_COPY;/* otherwise flag need to copy */

			strData.s = st;
			strData.size = i;

			(*mg) (str_getbuffer, &strData, i, md);

			// Doesn't return a value.
			string = NULL;
		}
		/* else must copy into free storage */
		else
		{			
			string = (char *) fs_get ((size_t) i + 1);
			if (!string) return (NULL);	//fs_get returns nil if it fails
				
			for (j = 0; j < i; j++)	// copy the string
			{
				if (*st == '\\') ++st;	// quoted character
				string[j] = *st++;
			}
			string[j] = '\0';	// tie off string
			if (len) *len = i;	// set return value too
		}
		break;
	
	case 'N':	// if NIL
	case 'n':
		++*txtptr;	// bump past "I"
		++*txtptr;	// bump past "L"
		if (len) *len = 0;
		break;
		
	case '{':	// if literal string
		// get size of string
		i = strtoul (*txtptr,txtptr,10);
		if (len) *len = i;	// set return value
		if (md && mg) 	// have special routine to slurp string?
		{
			if (md->first)	// partial fetch?
			{
				md->first--;	// restore origin octet
				md->last = i;	// number of octets that we got
			}
			else md->flags |= MG_COPY;	// otherwise flag need to copy
			string = (*mg) (net_getbuffer,stream->transStream,i,md);
		}
		else	// must slurp into free storage
		{
			string = (char *) fs_get ((size_t) i + 1);
			if (!string) return (NULL);	//fs_get returns nil if it fails
			
			*string = '\0';	// init in case getbuffer fails
			if (rp) for (k = 0; j = min ((long) MAILTMPLEN,(long) i); i -= j) 
			{
				net_getbuffer (stream->transStream,j,string + k);
				(*rp) (md,k += j);
			}
			else net_getbuffer (stream->transStream,i,string);
		}
		fs_give ((void **) &reply->line);
		
		// get new reply text line
		reply->line = net_getline (stream->transStream);
		if (stream->debug) mm_dlog (reply->line);
		*txtptr = reply->line;	// set text pointer to point at it
		break;
		
	default:
		sprintf (LOCAL->tmp,"Not a string: %c%.80s",c,*txtptr);
		mm_log (LOCAL->tmp,WARN);
		if (len) *len = 0;
		break;
	}
	return string;
}

/* IMAP parse body structure or contents
 * Accepts: mailgets_data
 *	    pointer to body pointer
 *	    pointer to segment
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer, stores body
 */

void imap_parse_body (GETS_DATA *md,IMAPBODY **body,char *seg,char **txtptr,IMAPPARSEDREPLY *reply)
{
	char *s;
	unsigned long size;
	STRINGLIST *stl = NIL;
	char *tmp = ((IMAPLOCAL *) md->stream->local)->tmp;
	MESSAGECACHE *elt;
	
	// Get the stream's "CurrentElt" (JOK)
	// This will allocate one if necessary.
	elt = mail_elt (md->stream);	
	if (!elt)
		return;

	/* dispatch based on type of data */
	switch (*seg++)
	{		
		case 'S':			/* extensible body structure */
			if (strcmp (seg,"TRUCTURE"))
			{
				sprintf (tmp,"Bad body fetch: %.80s",seg);
				mm_log (tmp,WARN);
				return;
			}

			/* falls through */
		case '\0':			/* body structure */
			mail_free_body (body);	/* flush any prior body */

				/* instantiate and parse a new body */
			imap_parse_body_structure (md->stream, *body = mail_newbody(), txtptr, reply);
			break;

		// JOK: 4/24/98 - Added .PEEK because the Novell groupwise server returns BODY.PEEK!!!
		case '.':
			ucase (seg);		/* make sure uppercase */

			// Better be this:
			if (strncmp (seg,"PEEK[", 5))
			{
				sprintf (tmp,"Bad body fetch: %.80s",seg);
				mm_log (tmp,WARN);
				return;
			}
			// Othersize, go pass PEEK[ and fall through:
			seg += 5;
			
		case '[':			/* body section text */
			ucase (seg);		/* make sure uppercase */

			/* header lines case? */
			if (!(s = strchr(seg,']')))
			{
				/* skip leading nesting */
				for (s = seg; *s && (isdigit (*s) || (*s == '.')); s++);

				/* better be one of these */
				if (strcmp (s,"HEADER.FIELDS") && strcmp (s,"HEADER.FIELDS.NOT"))
				{
					sprintf (tmp,"Unterminated section specifier: %.80s",seg);
					mm_log (tmp,WARN);
					return;
				}

				/* get list of headers */
				if (!(stl = imap_parse_stringlist (md->stream,txtptr,reply)))
				{
					sprintf (tmp,"Bogus header field list: %.80s",*txtptr);
					mm_log (tmp,WARN);
					return;
				}

				// JOK - We don't really need the string list!!!
				mail_free_stringlist (&stl);
				// END JOK 

				/* make sure terminated */
				if (**txtptr != ']')
				{	
					sprintf (tmp,"Unterminated header section specifier: %.80s",*txtptr);
					mm_log (tmp,WARN);
					mail_free_stringlist (&stl);
					return;
				}

				/* point after the text */
				if (*txtptr = strchr (s = *txtptr,' '))
					 *(*txtptr)++ = '\0';
			}


		    *s++ = '\0';		/* tie off section specifier */
				
			/* partial specifier? */
			if (*s == '<')
			{
				md->first = strtoul (s+1,&s,10) + 1;

				// Some servers spit back a <x.y> in the partial specifiers.  
				// They are both wrong and stupid. See RFC 2060, the FETCH response. -jdboyd 8/19/99
				 
				if (*s == '.') 
				{
					s++;						// skip over the period
					while (isdigit(*s)) s++;	// and any numbers that follow.
				}
			
				/* make sure properly terminated */
				if ((*s == NULL) || (*s++ != '>'))
				{
					sprintf (tmp,"Unterminated partial data specifier: %.80s",s-1);
					mm_log (tmp,WARN);
					mail_free_stringlist (&stl);
					return;
				}
			}

			/* make sure no junk follows */
		    if (*s)
			{
				sprintf (tmp,"Junk after section specifier: %.80s",s);
				mm_log (tmp,WARN);
				mail_free_stringlist (&stl);
				return;
			}

			md->what = seg;		/* get the body section text */

			// JOK - Calling "imap_parse_string()" will send the data to the caller.
			s = NULL;	// So we won't try to free it.
			imap_parse_string (md->stream, txtptr, reply, md, &size);

			// done if partial 
			if (md->first || md->last)
			{
				mail_free_stringlist (&stl);
				return;
			}

			// JOK - Ignore the rest.

			break;
			  
		default:			/* bogon */
		    sprintf (tmp,"Bad body fetch: %.80s",seg);
			mm_log (tmp,WARN);
			return;
	 }  // switch
}


/* IMAP parse body structure
 * Accepts: MAIL stream
 *	    body structure to write into
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_body_structure (MAILSTREAM *stream,IMAPBODY *body,char **txtptr,IMAPPARSEDREPLY *reply)
{
  	int i;
  	char *s;
  	PART *part = NIL;
  	char c;	
  
 	// Must have these ...
  	if (!stream || !body || !txtptr || !(*txtptr)) return;
  	
 	// grab first character
  	c = *((*txtptr)++);
  
	// ignore leading spaces
  	while (c == ' ') c = *((*txtptr)++);
  	
  	// dispatch on first character
 	switch (c) 
 	{			
		case '(':			// body structure list
	    	if (**txtptr == '(') 	// multipart body?
	    	{	
	      		body->type= TYPEMULTIPART;	// yes, set its type
	      		// instantiate new body part
	      		do
	      		{			
					if (part) part = part->next = mail_newbody_part ();
					else body->nested.part = part = mail_newbody_part ();
					
					// parse it
					imap_parse_body_structure (stream,&part->body,txtptr,reply);
					
					// ignore possible spaces until the next '(' (JOK)
					while (**txtptr == ' ') ++(*txtptr);
					
	     		} while (**txtptr == '(');	// for each body part
	      
		      	if (body->subtype = imap_parse_string (stream,txtptr,reply,NIL,NIL))
					ucase (body->subtype);
		      	else 
		      	{
		      		// Set it to "Multipart/Mixed" (JOK)
				//	body->subtype = cpystr ("Mixed");
					mm_log ("Missing multipart subtype",WARN);
				}
				
				// multipart parameters
		      	if (**txtptr == ' ')	
					body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
		      	
		      	// disposition
		      	if (**txtptr == ' ')	
					imap_parse_disposition (stream,body,txtptr,reply);
		      
		      	// language
		      	if (**txtptr == ' ')	
					body->language = imap_parse_language (stream,txtptr,reply);
		      
		      	while (**txtptr == ' ') imap_parse_extension (stream,txtptr,reply);
		      
		      	// validate ending
		      	if (**txtptr != ')') 
		      	{	
					sprintf (LOCAL->tmp,"Junk at end of multipart body: %.80s",*txtptr);
					mm_log (LOCAL->tmp,WARN);
		      	}
		      	else ++*txtptr;		// skip past delimiter
		    }
		    else 
		    {			
		    	// not multipart, parse type name 
		      	if (**txtptr == ')') 
		      	{	
		      		// empty body?
					++*txtptr;		// bump past it
					break;			// and punt
		      	}
		      
		      	body->type = TYPEOTHER;		// assume unknown type
		      	body->encoding = ENCOTHER;	// and unknown encoding
		      
				// parse type
		      	if (s = ucase (imap_parse_string (stream,txtptr,reply,NIL,NIL))) 
		      	{
					for (i=0;(i<=TYPEMAX) && body_types[i] && strcmp(s,body_types[i]);i++);
			
					if (i <= TYPEMAX) 
					{	
						// only if found a slot
			  			body->type = (unsigned short)i;	// set body type
			 
			 			if (body_types[i]) fs_give ((void **) &s);
			  			else body_types[i]=s;	// assign empty slot
					}
		      	}
		      
		      	// parse subtype
		      	body->subtype =	ucase (imap_parse_string (stream,txtptr,reply,NIL,NIL));
		      
		      	// parse parameter
		      	body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
		      
		      	// parse id
		      	body->id = imap_parse_string (stream,txtptr,reply,NIL,NIL);
		      
		      	// parse description
		      	body->description = imap_parse_string (stream,txtptr,reply,NIL,NIL);
		      
		      	if (s = ucase (imap_parse_string (stream,txtptr,reply,NIL,NIL))) 
		      	{
					// search for body encoding
					for (i = 0; (i <= ENCMAX) && body_encodings[i] && strcmp (s,body_encodings[i]); i++);
					
					if (i > ENCMAX) body->type = ENCOTHER;
					else 
					{	// only if found a slot
			 			body->encoding = (unsigned short) i;	// set body encoding
			  			
			  			if (body_encodings[i]) fs_give ((void **) &s);
			 			 else body_encodings[i] = s;
					}

#ifdef	WINDERZ
					//
					// If the subtype is binhex and the encoding is read as just text,
					// this should be decoded with binhex. Since we only pass an encoding type to
					// "FetchAttachmentContentsToFile" below, set an appropriate encoding flag.
					// 
					CRString szBinhex (IDS_MIME_BINHEX);
					CString  szSubtype = body->subtype;

					if ( (body->subtype != NULL) &&
					     (!strnicmp ( (LPCSTR)szSubtype, (LPCSTR)szBinhex, szBinhex.GetLength() ) ) )
					{
						if ( body->encoding == ENC7BIT || body->encoding == ENCOTHER )
						{
							body->encoding = ENCBINHEX;
						}
					}
#endif
		      	}
				
				// parse size of contents in bytes
		      	body->size.bytes = strtoul (*txtptr,txtptr,10);
		      	
		      	// possible extra stuff
		      	switch (body->type) 
		      	{	
		      		case TYPEMESSAGE:		// message envelope and body
						if (strcmp (body->subtype,"RFC822")) break;
			
						body->nested.msg = mail_newmsg ();
						imap_parse_envelope (stream,&body->nested.msg->env,txtptr,reply);
						body->nested.msg->body = mail_newbody ();
						imap_parse_body_structure(stream,body->nested.msg->body,txtptr,reply);
						// drop into text case
			
					// size in lines
		      		case TYPETEXT:		
						body->size.lines = strtoul (*txtptr,txtptr,10);
						break;
		    	
		    		// otherwise nothing special
		     		default:
						break;
		      	}
		      
		     	// if extension data
		      	if (**txtptr == ' ') body->md5 = imap_parse_string (stream,txtptr,reply,NIL,NIL);
		      
		      	// disposition
		      	if (**txtptr == ' ') imap_parse_disposition (stream,body,txtptr,reply);
		      
		      	// language
		      	if (**txtptr == ' ') body->language = imap_parse_language (stream,txtptr,reply);
		      
		      	while (**txtptr == ' ') imap_parse_extension (stream,txtptr,reply);
		      	
		      	// validate ending
		      	if (**txtptr != ')') 
		      	{	
					sprintf (LOCAL->tmp,"Junk at end of body part: %.80s",*txtptr);
					mm_log (LOCAL->tmp,WARN);
		      	}
		      	else 
		      	{
		      		++*txtptr;		// skip past delimiter
		      		
		      		// ignore possible spaces until the next '(' (JOK)
					while (**txtptr == ' ') ++(*txtptr);
				}
		    }
		    break;
	    
		case 'N':				// if NIL
		case 'n':
	    	++*txtptr;			// bump past "I"
	    	++*txtptr;			// bump past "L"
	    	break;
	    	
		default:			// otherwise quite bogus
	    	sprintf (LOCAL->tmp,"Bogus body structure: %.80s",*txtptr);
	    	mm_log (LOCAL->tmp,WARN);
	    	break;
	  }
}

/* IMAP parse body parameter
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: body parameter
 * Updates text pointer
 */

PARAMETER *imap_parse_body_parameter (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	PARAMETER *ret = NIL;
	PARAMETER *par = NIL;
	char c,*s;
	
	// ignore leading spaces
	while ((c = *(*txtptr)++) == ' ');
	
	// parse parameter list
	if (c == '(') 
	{
		while (c != ')') 
		{
			// append new parameter to tail
			if (ret) par = par->next = mail_newbody_parameter ();
			else ret = par = mail_newbody_parameter ();
			if(!(par->attribute=imap_parse_string (stream,txtptr,reply,NIL,NIL))) 
			{
				mm_log ("Missing parameter attribute",WARN);
				par->attribute = cpystr ("UNKNOWN");
			}
			if (!(par->value = imap_parse_string (stream,txtptr,reply,NIL,NIL))) 
			{
				sprintf (LOCAL->tmp,"Missing value for parameter %.80s",par->attribute);
				mm_log (LOCAL->tmp,WARN);
				par->value = cpystr ("UNKNOWN");
			}
	
			switch (c = **txtptr) 	// see what comes after
			{	
				case ' ':	// flush whitespace
					while ((c = *++*txtptr) == ' ');
					break;
				
				case ')':	// end of attribute/value pairs
					++*txtptr;	// skip past closing paren
					break;
				
				default:
					sprintf (LOCAL->tmp,"Junk at end of parameter: %.80s",*txtptr);
					mm_log (LOCAL->tmp,WARN);
					break;
			}
		}
	}
	// empty parameter, must be NIL
	else if (((c == 'N') || (c == 'n')) && ((*(s = *txtptr) == 'I') || (*s == 'i')) && ((s[1] == 'L') || (s[1] == 'l'))) *txtptr += 2;
	else 
	{
		sprintf (LOCAL->tmp,"Bogus body parameter: %c%.80s",c,(*txtptr) - 1);
		mm_log (LOCAL->tmp,WARN);
	}
	return ret;
}


/* IMAP parse body disposition
 * Accepts: MAIL stream
 *	    body structure to write into
 *	    current text pointer
 *	    parsed reply
 */

void imap_parse_disposition (MAILSTREAM *stream,IMAPBODY *body,char **txtptr,IMAPPARSEDREPLY *reply)
{
	switch (*++*txtptr) 
	{
		case '(':
			++*txtptr;	// skip open paren
			body->disposition.type = imap_parse_string (stream,txtptr,reply,NIL,NIL);
			body->disposition.parameter =
			imap_parse_body_parameter (stream,txtptr,reply);
			if (**txtptr != ')')	// validate ending
			{
				sprintf (LOCAL->tmp,"Junk at end of disposition: %.80s",*txtptr);
				mm_log (LOCAL->tmp,WARN);
			}
			else ++*txtptr;	// skip past delimiter
			break;
		
		case 'N':	// if NIL
		case 'n':
			++*txtptr;	// bump past "N"
			++*txtptr;	// bump past "I"
			++*txtptr;	// bump past "L"
			break;
		
		default:
			sprintf (LOCAL->tmp,"Unknown body disposition: %.80s",*txtptr);
			mm_log (LOCAL->tmp,WARN);
			// try to skip to next space
			while ((*++*txtptr != ' ') && (**txtptr != ')') && **txtptr);
		break;
	}
}


/* IMAP parse body language
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: string list or NIL if empty or error
 */

STRINGLIST *imap_parse_language (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	unsigned long i;
	char *s;
	STRINGLIST *ret = NIL;
	
	// language is a list
	if (*++*txtptr == '(') ret = imap_parse_stringlist (stream,txtptr,reply);
	else if (s = imap_parse_string (stream,txtptr,reply,NIL,&i)) 
	{
		(ret = mail_newstringlist ())->text.data = s;
		ret->text.size = i;
	}
	
	return ret;
}


/* IMAP parse string list
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: string list or NIL if empty or error
 */

STRINGLIST *imap_parse_stringlist (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	STRINGLIST *stl = NIL;
	STRINGLIST *stc;
	char c,*s;
	char *t = *txtptr;
	
	// parse the list
	if (*t++ == '(')
	{
		while (*t != ')') 
		{
			if (stl) stc = stc->next = mail_newstringlist ();
			else stc = stl = mail_newstringlist ();
		
			// atom
			if ((*t != '{') && (*t != '"') && (s = strpbrk (t," )"))) 
			{
				c = *s;	// note delimiter
				*s = '\0';	// tie off atom and copy it
				stc->text.size = strlen (stc->text.data = cpystr (t));
				if (c == ' ') t = ++s;	// another atom follows
				else *(t = s) = c;	// restore delimiter
			}
			// string
			else if (!(stc->text.data = imap_parse_string (stream,&t,reply,NIL,&stc->text.size))) 
			{
				sprintf (LOCAL->tmp,"Bogus string list member: %.80s",t);
				mm_log (LOCAL->tmp,WARN);
				mail_free_stringlist (&stl);
				break;
			}
		}
	}
	
	if (stl) *txtptr = ++t;	// update return string
	return stl;
}


/* IMAP parse unknown body extension data
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_extension (MAILSTREAM *stream,char **txtptr,IMAPPARSEDREPLY *reply)
{
	unsigned long i,j;
	
	switch (*++*txtptr)	// action depends upon first character
	{
		case '(':
			while (**txtptr != ')') imap_parse_extension (stream,txtptr,reply);
			++*txtptr;			// bump past closing parenthesis
			break;
	
		case '"':			// if quoted string
			while (*++*txtptr != '"') if (**txtptr == '\\') ++*txtptr;
			++*txtptr;			// bump past closing quote
			break;
	
		case 'N':			// if NIL
		case 'n':
			++*txtptr;			// bump past "N"
			++*txtptr;			// bump past "I"
			++*txtptr;			// bump past "L"
			break;
	
		case '{':	// get size of literal
			++*txtptr;	// bump past open squiggle
			if (i = strtoul (*txtptr,txtptr,10)) do
			net_getbuffer (stream->transStream,j = min (i,(long)IMAPTMPLEN),LOCAL->tmp);	// was "max", which makes little sense. -jdboyd 030304
			while (i -= j);
			// get new reply text line
			reply->line = net_getline (stream->transStream);
			if (stream->debug) mm_dlog (reply->line);
			*txtptr = reply->line;	// set text pointer to point at it
			break;
	
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			strtoul (*txtptr,txtptr,10);
			break;
	
		default:
			sprintf (LOCAL->tmp,"Unknown extension token: %.80s",*txtptr);
			mm_log (LOCAL->tmp,WARN);
			// try to skip to next space
			while ((*++*txtptr != ' ') && (**txtptr != ')') && **txtptr);
			break;
	}
}


/* IMAP return host name
 * Accepts: MAIL stream
 * Returns: host name
 */

char *imap_host (MAILSTREAM *stream)
{
	/* return host name on stream if open */
	return (LOCAL && stream->transStream) ? net_host (stream->transStream) : ".NO-IMAP-CONNECTION.";
}


/* IMAP open
 * Accepts: stream to open
 * Returns: stream to use on success, NULL on failure
 */

MAILSTREAM *imap_open (MAILSTREAM *stream)
{
	unsigned long i;
	unsigned long alive;
	char *s;
	char tmp[MAILTMPLEN];
	char userName[MAILTMPLEN];
	NETMBX mb;
	IMAPPARSEDREPLY *reply = NULL;
 	
	// return prototype for OP_PROTOTYPE call
	if (!stream) return ((MAILSTREAM *)&imapdriver);
	
	// fill in the mb structure from the command line.
	imapmail_valid_net_parse(stream,&mb);
	
	userName[0] = NULL;	// Username initially empty
	if (LOCAL)	// if stream opened earlier by us
	{	
		// if the stream is not autenticated at this point, close it and re-open it.
		if (!stream->bAuthenticated) imap_close(stream,nil);		
		// if hosts are different, close the old one.  Note, mb.host is a pstring
		else if (((mb.host[0] && imap_host(stream)) && !pstrincmp(mb.host,imap_host(stream),mb.host[0])) 
			|| (mb.user[0] && LOCAL->user && strcmp(mb.user,LOCAL->user))) 
		{				
		  sprintf(tmp,"Closing connection to %s",imap_host(stream));
		  if (!stream->silent) mm_log(tmp,(long) NULL);
		  imap_close(stream,NULL);
		}	
		else if (stream->transStream) 
		{
			// else recycle if still alive
			i = stream->silent;	
			stream->silent = true;	
//			alive = imap_ping(stream);	// learn if stream still alive 
			alive = true;	// imapconnections.c has already determined this stream is still alive. - JDB 160799
			stream->silent = i;
			if (alive) 
			{	
				sprintf(tmp,"Reusing connection to %s",mb.host);
				if (LOCAL->user) sprintf(tmp + strlen (tmp),"/user=%s",LOCAL->user);
				if (!stream->silent) mm_log(tmp,(long) NULL);
			}
			else imap_close(stream,NULL);
		}
	}	// if a re-open
	
	// in case /debug switch given 
	if (mb.dbgflag) stream->debug = true;

	// open new connection if no recycle
	if (!LOCAL) 
	{			
		stream->local = fs_get(sizeof (IMAPLOCAL));
		if (!stream->local) return NULL;

FillInLocal :
		// assume IMAP2bis server
		LOCAL->imap2bis = LOCAL->rfc1176 = true;

		if (!stream->transStream) // didn't get rimap?
		{
			unsigned long prt;
		
			// use the given port, or the default port if non specified.
			if(mb.port)
				prt = mb.port;
			else
#ifdef ESSL
				if(GetPrefLong(PREF_SSL_IMAP_SETTING) & esslUseAltPort)
					prt = GetRLong(IMAP_SSL_PORT);
				else
#endif
					prt = IMAPTCPPORT;
						
			s = mb.host;
			
			// try to open ordinary connection
	 		if ((stream->transStream = net_open(stream,s,"imap",prt)) 
	 		 &&  !imap_OK(stream,reply = imap_reply(stream,NULL))) 
	 		{
				mm_log (reply->text,IMAP_ERROR);
				return NULL;
			}
		}
		
		if (stream->transStream) // if have a connection
		{
			// non-zero if not preauthenticated
	  		if (( reply->key != NULL ) && ( i = strcmp(reply->key,"PREAUTH" )))
	  			userName[0] = '\0';
			// get server capabilities
	 	 	reply = imap_send (stream,"CAPABILITY",NULL);
#ifdef ESSL
			if(reply && imap_OK(stream,reply) && ShouldUseSSL(stream->transStream) && !(stream->transStream->ESSLSetting & esslSSLInUse))
			{
		//	OK, there are several cases here.
		//	(1) Server doesn't offer TLS
		//		(a) User has required TLS -- error
		//		(b) TLS is optional - do nothing
		//	(2) Server has offered TLS	--> fire it up!
		//		(a) ESSLStartSSL succeeeds --> We're good to go
		//		(b) ESSLStartSSL fails
		//			(1) User has required TLS - error
		//			(2) TLS is optional - continue w/o TLS
				Boolean sslRequired = 0 == ( stream->transStream->ESSLSetting & esslOptional );
				
				if (!LOCAL->use_tls)		// No TLS for this server!
				{
					if ( sslRequired )
					{
						ComposeStdAlert ( Note, ALRTStringsStrn+NO_SERVER_SSL );
						goto DoSSLErr;
					}
				}
				else
				{
					IMAPPARSEDREPLY *sslReply;
					
					// starttls
					sslReply = imap_send (stream,"STARTTLS",NULL);
					if(!sslReply || !imap_OK(stream, sslReply) || (ESSLStartSSL(stream->transStream) != noErr))
					{
DoSSLErr :
						if ( sslRequired )
						{
							net_close(stream->transStream);
							stream->transStream = NULL;
							mm_log(GetRString(tmp, SSL_ERR_STRING)+1, IMAP_ERROR);
							return NIL;
						}
					}
					else if(stream->transStream->ESSLSetting & esslSSLInUse)
					{
						WriteZero(LOCAL, sizeof(IMAPLOCAL));
						goto FillInLocal;
					}
				}
			}
#endif
			if (reply)
			{
				// need to authenticate?
		  		if (stream->transStream 
		  		  && i 
		  		  && !((LOCAL->use_auth && !(mb.anoflag || stream->anonymous) && !PrefIsSet(PREF_IMAP_DONT_AUTHENTICATE)) ? imap_auth (stream,&mb,tmp,userName) : imap_login (stream,&mb,tmp,userName)))
				{
					// Close the stream if we failed to authenticate.
					if (stream->transStream) net_close (stream->transStream);
					stream->transStream = NULL;
					return NIL;		// authentication failed
				}
				
				// We succeeded.  And we are now authenticated.
				stream->bAuthenticated = true;
			}
			else 
			{
				// User must have quit.
				if (stream->transStream) net_close(stream->transStream);
				stream->transStream = NULL;
				return NIL;	// authentication failed
			}
		}
	}
	
	if (stream->transStream) // still have a connection?
	{
		stream->perm_seen = stream->perm_deleted = stream->perm_answered = stream->perm_draft = LEVELIMAP4(stream) ? NULL : true;
		stream->perm_user_flags = LEVELIMAP4(stream) ? NULL : 0xffffffff;
		stream->sequence++;
		if ((i = net_port(stream->transStream)) & 0xffff0000) i = imap_defaultport ? imap_defaultport : IMAPTCPPORT;
		// record user name
		if (!LOCAL->user && userName[0]) LOCAL->user = cpystr(userName);
		sprintf (tmp,LOCAL->user ? "{%s:%lu/imap/user=%s}" : "{%s:%lu/imap}", net_host(stream->transStream),i,LOCAL->user);
	
		if (!stream->halfopen) // wants to open a mailbox?
		{
			IMAPARG *args[2];
			IMAPARG ambx;
			ambx.type = ASTRING;
			ambx.text = (void *) mb.mailbox;
			args[0] = &ambx; args[1] = NULL;
			
			reply = imap_send (stream,stream->rdonly ? "EXAMINE": "SELECT",args);
			
			if (reply && imap_OK(stream,reply)) 
			{
				// We've succeeded.
				stream->bSelected = TRUE;

	  			strcat(tmp,mb.mailbox);
	  			
				// note if server said it was readonly
				stream->rdonly = !strncmp(ucase (reply->text),"[READ-ONLY]",11);
				
				if (!stream->nmsgs && !stream->silent)  mm_log ("Mailbox is empty",(long) NULL);
	 		}
	  		else // failed
	  		{
	  			// We're not selected
				stream->bSelected = FALSE;
					
				mm_log (reply->text,IMAP_ERROR);
				if (imap_closeonerror) return NULL;
				stream->halfopen = true;	/* let him keep it half-open */
	  		}
		}
	
		if (stream->halfopen) // half-open connection?
		{
	 		// Not selected
			stream->bSelected = FALSE;
			
			strcat(tmp,"<no_mailbox>");
			// make sure dummy message counts
	  		mail_exists(stream,(long) 0);
	  		mail_recent(stream,(long) 0);
		}
		fs_give ((void **) &stream->mailbox);
		stream->mailbox = cpystr (tmp);
	}
				/* success if stream open */
	return stream->transStream ? stream : NULL;
}


/* IMAP get reply
 * Accepts: MAIL stream
 *	    tag to search or NIL if want a greeting
 * Returns: parsed reply, never NIL
 */

IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag)
{
	IMAPPARSEDREPLY *reply;
	
	if (!stream) return (0);
	
	while (imap_connected(stream) && stream->transStream)	
	{		   
	    // parse reply from server
	    if (reply = imap_parse_reply(stream,net_getline(stream->transStream)))
	    {
			if (!strcmp (reply->tag,"+")) return reply;
			else if (!strcmp (reply->tag,"*")) 
			{
				imap_parse_unsolicited (stream,reply);
				if (!tag) return reply;	/* return if just wanted greeting */
			}
			else	// tagged data
			{
				if (tag && !strcmp (tag,reply->tag)) return reply;
				sprintf (LOCAL->tmp,"Unexpected tagged response: %.80s %.80s %.80s", reply->tag,reply->key,reply->text);
				mm_log (LOCAL->tmp,WARN);
			}
		}
	}
	
	return imap_fake(stream,tag,"IMAP connection broken (server response)");
}

/* IMAP check for OK response in tagged reply
 * Accepts: MAIL stream
 *	    parsed reply
 * Returns: T if OK else NIL
 */

long imap_OK(MAILSTREAM *stream, IMAPPARSEDREPLY *reply)
{
	// OK - operation succeeded
	if (!strcmp (reply->key, "OK") || (!strcmp (reply->tag,"*") && !strcmp (reply->key,"PREAUTH"))) return T;
	// NO - operation failed
	else if (strcmp (reply->key,"NO")) 
	{
		// BAD - operation rejected
		if (!strcmp (reply->key,"BAD"))
			sprintf (LOCAL->tmp,"IMAP error: %.80s",reply->text);
		else sprintf (LOCAL->tmp,"Unexpected IMAP response: %.80s %.80s",reply->key,reply->text);
		mm_log (LOCAL->tmp,WARN);	/* log the sucker */
	}
	else
	{
		// we received a NO response.  See if we got an ALERT response along with it and handle it -jdboyd
		if (!strncmp (reply->text,"[ALERT]", 7))
			mm_alert(stream, reply->text);
	}
	return NIL;
}


/* IMAP parse reply
 * Accepts: MAIL stream
 *	    text of reply
 * Returns: parsed reply, or NIL if can't parse at least a tag and key
 */

IMAPPARSEDREPLY *imap_parse_reply(MAILSTREAM *stream, char *text)
{
	if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
	if (!(LOCAL->reply.line = text)) 	// NIL text means the stream died
	{			
		// Don't close the connection to the server anymore.  We'll reuse it later.
		
		//if (stream->transStream) net_close (stream->transStream);
		//stream->transStream = NIL;
		return NIL;
	}
	
	if (stream->debug) mm_dlog (LOCAL->reply.line);
	LOCAL->reply.key = NIL;	// init fields in case error
	LOCAL->reply.text = NIL;
	LOCAL->reply.fake = false;
	
	if (!(LOCAL->reply.tag = (char *) strtok (LOCAL->reply.line," "))) 
	{
		mm_log ("IMAP server sent a blank line",WARN);
		return NIL;
	}
	
	// non-continuation replies
	if (strcmp (LOCAL->reply.tag,"+")) 	// parse key
	{	
		if (!(LOCAL->reply.key = (char *) strtok (NIL," ")))	//determine what is missing
		{
		 	sprintf (LOCAL->tmp,"Missing IMAP reply key: %.80s",LOCAL->reply.tag);
	 	 	mm_log (LOCAL->tmp,WARN);	// pass up the barfage
	  		return NIL;		// can't parse this text */
		}
		
		ucase (LOCAL->reply.key);	// make sure key is upper case
		if (!(LOCAL->reply.text = (char *) strtok (NIL,"\n"))) 	//get text as well, allow empty text
	  		LOCAL->reply.text = LOCAL->reply.key + strlen (LOCAL->reply.key);
	}
	else	// special handling of continuation
	{
		LOCAL->reply.key = "BAD";	// so it barfs if not expecting continuation
		if (!(LOCAL->reply.text = (char *) strtok (NIL,"\n")))
		LOCAL->reply.text = "Ready for more command";
	}
	
	return (&LOCAL->reply);
}


/* IMAP send command
 * Accepts: MAIL stream
 *	    command
 *	    argument list
 * Returns: parsed reply
 */

#define MAXSEQUENCE 1000

IMAPPARSEDREPLY *imap_send(MAILSTREAM *stream, char *cmd, IMAPARG *args[])
{
	IMAPPARSEDREPLY *reply;
	IMAPARG *arg,**arglst;
	STRINGLIST *list;
	char c,*s,*t,tag[16];
#ifdef DEBUG	// For debugging (JDB)
	char *tmp_buf = LOCAL->tmp;
#endif

	if (!stream->transStream) return imap_fake (stream,tag,"No-op dead stream");

	//No longer needed.  Already locked at this point.
	//mail_lock (stream);	// lock up the stream
	
	sprintf (tag,"A%05ld",stream->gensym++);	// gensym a new tag
	for (s = LOCAL->tmp,t = tag; *t; *s++ = *t++);

	*s++ = ' ';	// delimit and write command
	for (t = cmd; *t; *s++ = *t++);
	if (arglst = args) while (arg = *arglst++) 
	{
		*s++ = ' ';	// delimit argument with space
		
		switch (arg->type) 
		{
			case ATOM:	// atom
				for (t = (char *) arg->text; *t; *s++ = *t++);
				break;
	
			case NUMBER:	// number
				sprintf (s,"%lu",(unsigned long) arg->text);
				s += strlen (s);
				break;
	
			case FLAGS:	// flag list as a single string
				if (*(t = (char *) arg->text) != '(') 
				{
					*s++ = '(';	// wrap parens around string
					while (*t) *s++ = *t++;
					*s++ = ')';	// wrap parens around string
				}
				else while (*t) *s++ = *t++;
				break;
	
			case ASTRING:	// atom or string, must be literal?
				if (reply = imap_send_astring(stream,tag,&s,arg->text,(unsigned long) strlen (arg->text),NULL))
				return reply;
				break;

			case LITERAL:	// literal, as a stringstruct
				if (reply = imap_send_literal(stream,tag,&s,arg->text)) return reply;
				break;
	
			case LIST:	// list of strings
				list = (STRINGLIST *) arg->text;
				c = '(';
				do
				{
					*s++ = c;	// write prefix character
					if (reply = imap_send_astring (stream,tag,&s,list->text.data, list->text.size,NULL)) return reply;
					c = ' ';	// prefix character for subsequent strings
				}
				while (list = list->next);
				*s++ = ')';		// close list
				break;
	
			case SEARCHPROGRAM:		// search program 
				if (reply = imap_send_spgm(stream,tag,&s,arg->text)) return reply;
				break;
			
			case BODYTEXT:		/* body section */
				// JOK - Now put "(" around BODY fetches (11/7/97).
				// Some servers expect () even with one fetch argument.
				for (t = "(BODY["; *t; *s++ = *t++);
				for (t = (char *) arg->text; *t; *s++ = *t++);
				break;

			case BODYPEEK:		/* body section */
				// JOK - Now put "(" around BODY fetches (11/7/97).
				for (t = "(BODY.PEEK["; *t; *s++ = *t++);
				for (t = (char *) arg->text; *t; *s++ = *t++);
				break;

			case BODYCLOSE:		/* close bracket and possible length */
				s[-1] = ']';		/* no leading space */
				for (t = (char *) arg->text; *t; *s++ = *t++);
				// JOK - Now put ")" around BODY fetches (11/7/97).
				for (t = ")"; *t; *s++ = *t++);
				break;

			
			case SEQUENCE:		// sequence 
				// JOK - Don't recurse - send the complete sequence
				// falls through 
	
			case LISTMAILBOX:		// astring with wildcards 
				if (reply = imap_send_astring (stream,tag,&s, (char *)arg->text, (unsigned long) strlen(arg->text),T))
				{
					return reply;
				}
				break;
	
			default:
				fatal ("Unknown argument type in imap_send()!");
		}
	}
	
	// send the command 
	reply = imap_sout(stream,tag,LOCAL->tmp,&s);
	//no longer
	//mail_unlock(stream);		// unlock stream 
	return reply;
}

/* IMAP send buffered command to sender
 * Accepts: MAIL stream
 *	    reply tag
 *	    string
 *	    pointer to string tail pointer
 * Returns: reply
 */

IMAPPARSEDREPLY *imap_sout(MAILSTREAM *stream, char *tag, char *base, char **s)
{
	IMAPPARSEDREPLY *reply;

	if (stream->debug)	// output debugging telemetry
	{		
		**s = '\0';
		mm_dlog (base);
	}
	
	*(*s)++ = '\015';	// append CRLF
	*(*s)++ = '\012';
	**s = '\0';

	// send the command, parse the reply unless we're doing a fast logout
	if (net_sout(stream->transStream,base,*s - base) && !(stream->fastLogout))	
		reply = imap_reply (stream,tag);
	else
		reply = imap_fake (stream,tag,"IMAP connection broken (command)");
	
	*s = base;			/* restart buffer */
	stream->fastLogout = false;	// reset this just in case
	return reply;
}


/* IMAP parse and act upon unsolicited reply
 * Accepts: MAIL stream
 *	    parsed reply
 */
//Modified by JDB

void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply)
{
	unsigned long i = 0;
	unsigned long msgno;
	char *s,*t;
	char *keyptr,*txtptr;
	
	// Must have a stream, and a reply
	if (!stream || !reply) return;
	
	// create a NEW CurrentElt in the stream
	if (stream->CurrentElt) mail_free_elt (&stream->CurrentElt);
	stream->CurrentElt = mail_elt (stream);
	
	// see if key is a number
	msgno = strtoul (reply->key,&s,10);
	
	// if non-numeric
	if (*s) 
	{			
		if (!strcmp (reply->key,"FLAGS")) 
		{
			// flush old user flags if any
			while ((i < NUSERFLAGS) && stream->user_flags[i])
				fs_give ((void **) &stream->user_flags[i++]);
			
			i = 0;			// add flags 
			if (s = (char *) strtok (reply->text+1," )"))
				do 
					if (*s != '\\') stream->user_flags[i++] = cpystr (s);
				while (s = (char *) strtok (NIL," )"));
		}
		else if (!strcmp (reply->key,"SEARCH")) 
		{
			// only do something if have text
			if (reply->text && (t = (char *) strtok (reply->text," "))) 
				do 
				{
					i = atol (t);
					if (!LOCAL->uidsearch) mail_elt (stream)->searched = T;
					mm_searched (stream,i);
				} while (t = (char *) strtok (NIL," "));
		}
		else if (!strcmp (reply->key,"STATUS")) 
		{
			MAILSTATUS status;
			char *txt;
			
			switch (*reply->text) 	// mailbox is an astring
			{	
				case '"':			// quoted string?
				case '{':			// literal?
					txt = reply->text;	// status data is in reply
					t = imap_parse_string (stream,&txt,reply,NIL,NIL);
				break;
	
				default:			// must be atom
					t = cpystr (reply->text);
					if (txt = strchr (t,' ')) *txt++ = '\0';
					break;
			}
			
			// JDB 060899, txt seems to point to " (", extra space after STATUS
			if (*txt == ' ') txt++;
			
			if (t && txt && (*txt++ == '(') && (s = strchr (txt,')')) && (s - txt) && !s[1]) 
			{	
				char *scan = nil;
				
				*s = '\0';		// tie off status data
				// initialize data block
				status.flags = status.messages = status.recent = status.unseen = status.uidnext = status.uidvalidity = 0;
				ucase (txt);		// do case-independent match
				while (*txt && (s = strchr (txt,' '))) 
				{
					*s++ = '\0';		// tie off status attribute name
					i = strtoul (s,&s,10);// get attribute value
					if (!strcmp (txt,"MESSAGES")) 
					{
						status.flags |= SA_MESSAGES;
						status.messages = i;
					}
					else if (!strcmp (txt,"RECENT")) 
					{
						status.flags |= SA_RECENT;
						status.recent = i;
					}
					else if (!strcmp (txt,"UNSEEN")) 
					{
						status.flags |= SA_UNSEEN;
						status.unseen = i;
					}
					else if (!strcmp (txt,"UIDNEXT") || !strcmp (txt,"UID-NEXT")) 
					{
						status.flags |= SA_UIDNEXT;
						status.uidnext = i;
					}
					else if (!strcmp (txt,"UIDVALIDITY")|| !strcmp (txt,"UID-VALIDITY"))
					{
						status.flags |= SA_UIDVALIDITY;
						status.uidvalidity = i;
					}
					
					// next attribute
					txt = (*s == ' ') ? s + 1 : s;
				}
				
				// was:
				// strcpy (strchr (strcpy (LOCAL->tmp,stream->mailbox),'}') + 1,t);
				
				strcpy (LOCAL->tmp,stream->mailbox);
				scan = strchr(LOCAL->tmp, '}');
				if (scan) strcpy (scan+1,t);
				
				// pass status to main program
				mm_status (stream,LOCAL->tmp,&status);
			}
			fs_give ((void **) &t);
		}
		else if ((!strcmp (reply->key,"LIST") || !strcmp (reply->key,"LSUB")) && (*reply->text == '(') && (s = strchr (reply->text,')')) && (s[1] == ' ')) 
		{
			char delimiter = '\0';
	
			*s++ = '\0';		// tie off attribute list
			
			// parse attribute list
			if (t = (char *) strtok (reply->text+1," ")) 
				do 
				{
					if (!strcmp (ucase (t),"\\NOINFERIORS")) i |= LATT_NOINFERIORS;
					else if (!strcmp (t,"\\NOSELECT")) i |= LATT_NOSELECT;
					else if (!strcmp (t,"\\MARKED")) i |= LATT_MARKED;
					else if (!strcmp (t,"\\UNMARKED")) i |= LATT_UNMARKED;
					else if (LOCAL->use_children && !strcmp (t,"\\HASNOCHILDREN")) i |= LATT_HASNOCHILDREN;
					
					// ignore extension flags
				} while (t = (char *) strtok (NIL," "));
	
			switch (*++s) 	// process delimiter
			{		
				case 'N':			// NIL
				case 'n':
					s += 4;			// skip over NIL<space>
					break;
	
				case '"':			// have a delimiter
					delimiter = (*++s == '\\') ? *++s : *s;
					s += 3;			// skip over <delimiter><quote><space>
			}
	
			// need to prepend a prefix?
			if (LOCAL->prefix) 
				strcpy (LOCAL->tmp,LOCAL->prefix);
			else 
				LOCAL->tmp[0] = '\0';// no prefix needed
	
			// need to do string parse?
			if ((*s == '"') || (*s == '{')) 
			{
				strcat (LOCAL->tmp,t = imap_parse_string (stream,&s,reply,NIL,NIL));
				fs_give ((void **) &t);
			}
			else strcat(LOCAL->tmp,s);	// atom is easy
			
			if (reply->key[1] == 'S') mm_lsub (stream,delimiter,LOCAL->tmp,i);
			else mm_list (stream,delimiter,LOCAL->tmp,i);
		}
		else if (!strcmp (reply->key,"MAILBOX")) 
		{
			if (LOCAL->prefix) sprintf (t = LOCAL->tmp,"%s%s",LOCAL->prefix,reply->text);
			else t = reply->text;
		
			mm_list (stream,NIL,t,NIL);
		}
		else if (!strcmp (reply->key,"OK") || !strcmp (reply->key,"PREAUTH")) 
		{
			if ((*reply->text == '[') && (t = strchr (s = reply->text + 1,']')) && ((i = t - s) < IMAPTMPLEN)) 
			{
				// get text code
				strncpy (LOCAL->tmp,s,(size_t) i);
				LOCAL->tmp[i] = '\0';	// tie off text
				if (!strcmp (ucase (LOCAL->tmp),"READ-ONLY")) stream->rdonly = T;
				else if (!strcmp (LOCAL->tmp,"READ-WRITE")) stream->rdonly = NIL;
				else if (!strncmp (LOCAL->tmp,"UIDVALIDITY ",12)) 
				{
					stream->uid_validity = strtoul (LOCAL->tmp+12,NIL,10);
					return;
				}
				else if (!strncmp (LOCAL->tmp,"PERMANENTFLAGS (",16)) 
				{
					if (LOCAL->tmp[i-1] == ')') LOCAL->tmp[i-1] = '\0';
					stream->perm_seen = stream->perm_deleted = stream->perm_answered = stream->perm_draft = stream->kwd_create = NIL;
					stream->perm_user_flags = NIL;
					if (s = strtok (LOCAL->tmp+16," ")) 
						do 
						{
							if (!strcmp (s,"\\SEEN")) stream->perm_seen = T;
							else if (!strcmp (s,"\\DELETED")) stream->perm_deleted = T;
							else if (!strcmp (s,"\\FLAGGED")) stream->perm_flagged = T;
							else if (!strcmp (s,"\\ANSWERED")) stream->perm_answered = T;
							else if (!strcmp (s,"\\DRAFT")) stream->perm_draft = T;
							else if (!strcmp (s,"\\*")) stream->kwd_create = T;
							else stream->perm_user_flags |= imap_parse_user_flag (stream,s);
						} while (s = strtok (NIL," "));
					return;
				}
				else if (!strncmp (LOCAL->tmp,"ALERT", 5))				// see if this is an [ALERT] -jdboyd
					mm_alert(stream, reply->text);
			}
			mm_notify (stream,reply->text,(long) NIL);
		}
		else if (!strcmp (reply->key,"NO")) 
		{
			if (reply->text && !strncmp (reply->text,"[ALERT]", 7)) 	// see if an ALERT response was included -jdboyd
				mm_alert(stream, reply->text);
			
			if (!stream->silent) mm_notify (stream,reply->text,WARN);
		}
		else if (!strcmp (reply->key,"BYE")) 
		{
			if (reply->text && !strncmp (reply->text,"[ALERT]", 7)) 	// see if an ALERT response was included -jdboyd
				mm_alert(stream, reply->text);
				
			LOCAL->byeseen = T;	// note that a BYE seen
			if (!stream->silent) mm_notify (stream,reply->text,BYE);
		}
		else if (!strcmp (reply->key,"BAD"))
		{
			if (reply->text && !strncmp (reply->text,"[ALERT]", 7)) 	// see if an ALERT response was included -jdboyd
				mm_alert(stream, reply->text);

			mm_notify (stream,reply->text,IMAP_ERROR);
		}
		else if (!strcmp (reply->key,"CAPABILITY")) 
		{
			// only do something if have text
			if (reply->text && (t = (char *) strtok (ucase (reply->text)," "))) 
				do 
				{
					if (!strcmp (t,"IMAP4")) 
						LOCAL->imap4 = T;
					else if (!strcmp (t,"IMAP4REV1")) 
						LOCAL->imap4rev1 = T;
					else if (!strcmp (t,"SCAN")) 
						LOCAL->use_scan = T;
					else if (!strcmp (t,"STARTTLS")) 
						LOCAL->use_tls = T;
					else if (!strncmp (t,"AUTH",4) && ((t[4] == '=') || (t[4] == '-')) && (i = mail_lookup_auth_name(t+5)) && (--i < MAXAUTHENTICATORS)) 
						LOCAL->use_auth |= (1 << i);
					else if (!strcmp (t,"CHILDREN"))
						LOCAL->use_children = T;
					else if (!strcmp (t,"UIDPLUS")) 
					{
						LOCAL->uidplus = T;
						
						// Trust servers that advertise UIDPLUS do enough of it to support SpamWatch.
						IMAPSpamWatchSupported(true, true);
					}
					// unsupported IMAP4 extension
					else if (!strcmp (t,"STATUS")) 
						LOCAL->use_status = T;
					
					// ignore other capabilities
				} while (t = (char *) strtok (NIL," "));
		}
		else 
		{
			sprintf (LOCAL->tmp,"Unexpected unsolicited message: %.80s",reply->key);
			mm_log (LOCAL->tmp,WARN);
		}
	}
	else 	// if numeric, a keyword follows
	{			
		// deposit null at end of keyword
		keyptr = ucase ((char *) strtok (reply->text," "));
		// and locate the text after it
		txtptr = (char *) strtok (NIL,"\n");
		// now take the action
		// change in size of mailbox
		if (!strcmp (keyptr,"EXISTS")) mail_exists (stream,msgno);
		else if (!strcmp (keyptr,"RECENT")) mail_recent (stream,msgno);
		else if (!strcmp (keyptr,"EXPUNGE")) imap_expunged (stream,msgno);
		else if (!strcmp (keyptr,"FETCH")) 
		{
			imap_parse_data (stream,msgno,txtptr,reply);
			
			// take care of minimal headers.
			if (stream->chunkHeaders && stream->headerUID) SaveMinimalHeader(stream);
		}
		else if (!strcmp (keyptr,"STORE")) imap_parse_data (stream,msgno,txtptr,reply);
		else if (strcmp (keyptr,"COPY")) 
		{
			sprintf (LOCAL->tmp,"Unknown message data: %lu %.80s",msgno,keyptr);
			mm_log (LOCAL->tmp,WARN);
		}
	}
}

/*
 * imap_connected - see if we're connected
 *
 * Accepts: MAIL stream
 * Returns: true if connected, NULL if not.
 */
 
Boolean imap_connected (MAILSTREAM *stream)
{
	if (stream && LOCAL && stream->transStream && !TransError(stream->transStream) && stream->bConnected && !CommandPeriod) return true;
	else return NULL;
}	

/* IMAP fetch RFC822.SIZE
 * Accepts: MAIL stream
 *	    message uid
 * Returns: size, or 0 if error.
 */

// NOTE: Added by JOK.
// FUNCTION
// Fetch the RFC822.SIZE of the message.
// Return size, or 0 if error.
// END FUNCTION

unsigned long imap_rfc822size (MAILSTREAM *stream, unsigned long msgno, long flags)
{
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[3],aseq,aatt;
	char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
	char seq[MAILTMPLEN];
	unsigned long rfc822size = 0;
	MESSAGECACHE  *elt = NULL;

	// Cannot have a zero msgno.
	if (msgno <= 0)
		return msgno;

	// Initialize:
	rfc822size = 0;

	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);
	stream->CurrentElt = NULL;

	// Allocate a new one.
	elt = mail_elt (stream);
	if (elt)
	{
		if (flags & FT_UID)
			elt->privat.uid == msgno;
		else
			elt->msgno = msgno;
	}
	else
		return 0;

	// Setup for IMAP call.
 	sprintf (seq, "%lu", msgno);
    aseq.type = SEQUENCE;
	aseq.text = (void *) seq;
    aatt.type = ATOM;
	aatt.text = (void *) "RFC822.SIZE";
    args[0] = &aseq;
	args[1] = &aatt;
	args[2] = NIL;

	/* send "FETCH msgno UID" */
    if (!imap_OK (stream,reply = imap_send (stream, cmd, args)))
	{
		if (reply) mm_log (reply->text,WARN);
		return 0;
	}

	// Did we get anything?
	if (stream->CurrentElt)
	{
		// Make sure the uid's matched.
		if (flags & FT_UID)
		{
			if (stream->CurrentElt->privat.uid == msgno)
				rfc822size = stream->CurrentElt->rfc822_size;
		}
		else
		{
			if (stream->CurrentElt->msgno == msgno)
				rfc822size = stream->CurrentElt->rfc822_size;
		}

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}

	return rfc822size;
}

/* IMAP fetch envelope. */

// NOTE:
// JOK - This is a new driver member. It fetches just the message envelope.
// The parsing routine allocates an ENVELOPE structure and attaches it to the 
// stream's "current elt". We check for the envelope, detaches it from the stream and passes it to the caller.
// The caller MUST free the structure when done with it.
// END NOTE

/* Accepts: MAIL stream
 *	    message # to fetch
 *	    option flags
 * Returns: Returns an allocated ENVELOPE structure.
 *
 */

ENVELOPE *imap_envelope (MAILSTREAM *stream,unsigned long msgno, long flags) 
{
	char seq[128],tmp[MAILTMPLEN];
	ENVELOPE *env;
	IMAPPARSEDREPLY *reply = NIL;
	IMAPARG *args[3],aseq,aatt;
	MESSAGECACHE	*elt = NULL;

	// Cannot have a zero msgno.
	if (msgno <= 0)
		return NULL;

	// Initialize: "env" is what's returned.
	env = NULL;

	args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
	aseq.type = SEQUENCE; aseq.text = (void *) seq;
	aatt.type = ATOM; aatt.text = NIL;

	// IF there is a CurrentElt in the stream, delete it.
	if (stream->CurrentElt)
		mail_free_elt (&stream->CurrentElt);

	// Allocate a new one.
	elt = mail_elt (stream);
	if (elt)
	{
		if (flags & FT_UID)
			elt->privat.uid = msgno;
		else
			elt->msgno = msgno;
	}

	// NOTE: "msgno" can be a UID or a message sequence number.
 	sprintf (seq,"%lu",msgno);	/* initial sequence (UID or msgno) */

	// Format command based on server capability.
	// NOTE: Can't handle any IMAP version older than imap2bis!!
	if (LEVELIMAP4 (stream) && (flags & FT_UID))
	{
		sprintf (tmp,"(UID ENVELOPE)");

		aatt.text = (void *) tmp;	/* do the built command */

		if (!imap_OK (stream, reply = imap_send (stream,"UID FETCH",args)))
		{
			if (reply)
				mm_log (reply->text,IMAP_ERROR);
		}
    }
	else if (LEVELIMAP2bis (stream))
	{
		/* has non-extensive body and no UID. */
		sprintf (tmp,"(BODY)");

		aatt.text = (void *) tmp;	/* do the built command */

		if (!imap_OK (stream, reply = imap_send (stream,"FETCH",args)))
		{
			if (reply)
				mm_log (reply->text,IMAP_ERROR);
		}
    }


	// "env" is what's returned.
	env = NULL;

	// Did we get anything?
	if (stream->CurrentElt)
	{
		// Make sure the UID's or msgno's matched.
		if (flags & FT_UID)
		{
			if (stream->CurrentElt->privat.uid == msgno)
				env = stream->CurrentElt->privat.msg.env;


		}
		else
		{
			if (stream->CurrentElt->msgno == msgno)
				env = stream->CurrentElt->privat.msg.env;
		}

		// Make sure we detach the body pointer if there was one.
		// If it's not our env, we'd want to delete it.
		if (env)
			stream->CurrentElt->privat.msg.env = NULL;

		// Now free the elt.
		mail_free_elt (&stream->CurrentElt);
	}

	return env;
}

/************************************************************************
 * StoreUIDPLUSResponses - Store the UIDPLUS responses given in a reply.
 *	Currently works for COPYUID only.
 ************************************************************************/
OSErr StoreUIDPLUSResponses(MAILSTREAM *stream, IMAPPARSEDREPLY *reply)
{
	OSErr err = noErr;
  	char *s,*e;
   	
 	// Does this reply contain COPYUID responses?
	if (!strncmp(ucase (reply->text),"[COPYUID",8)) 
	{
		// There are COPYUID reponses available
		IMAPSpamWatchSupported(true, true);
		
		// RFC 2359
		// First number in the COPYUID response in the UIDVALIDITY of the destination mailbox
		s = reply->text + 9;
		e = strchr(s, ' ');
		if (e)
		{
			// make sure the UIDVALIDITY of the destination mailbox hasn't changed on us.
			// if it has, we'll need to discard the responses we've received so far.  They're invalid.
		  	VerifyUIDValidity(stream, s, e-s);
		  	
		  	// next block is the range of messages transferred
		  	e++;
		  	if (*e)
		  	{
		  		e = strchr(e, ' ');
		  		if (e)
		  		{
		  			// Last block is the range of new UIDs
		  			s = ++e;
		  			e = strchr(s, ']');
		  			if (e)
		  			{
		  				UIDStringToUIDs(s, e-s, &(stream->UIDPLUSResponse));
		  			}
		  		}
		  	}
		}
	}	
	else
	{
	 	// UID COPY was executed, but no COPYUID repsonse was detected.
 	 	IMAPSpamWatchSupported(false, true);
	}
			
	return (err);
}
	  	
/************************************************************************
 * VerifyUIDValidity - zap the UIDPlus responses up to now if we
 *	find the UIDVALIDITY of the mailbox has changed on us.
 ************************************************************************/
void VerifyUIDValidity(MAILSTREAM *stream, char *pUids, int len)
{
	Str255 scratch;
	int newUV;
	
	MakePStr(scratch, pUids, MIN(len, sizeof(scratch)));
	StringToNum(scratch, &newUV);

	// first time?
	if (stream->UIDPLUSuv == 0)
		stream->UIDPLUSuv = newUV;
	else if (stream->UIDPLUSuv != newUV)
	{
		// Zap Resposes so far, store new uidvalidity
		AccuZap(stream->UIDPLUSResponse);
  		AccuInit(&stream->UIDPLUSResponse);
  		stream->UIDPLUSuv = newUV;
	}
	// else
		// they match
}

/************************************************************************
 * UIDStringToUIDs - given a UID set, add the individual UIDs to the
 *	accumulator passed in.
 ************************************************************************/
OSErr UIDStringToUIDs(char *pUids, int len, Accumulator *pAccu)
{
	OSErr err = noErr;
	long start, stop;
	char *pEnd, *pUidEnd = pUids + len;
	Str255 scratch;
	
	// only handles x and x:y type sets at this point
	pEnd = strchr(pUids, ':');
	if (pEnd)
	{
		MakePStr(scratch, pUids, pEnd-pUids);
		StringToNum(scratch, &start);
		pUids = pEnd + 1;
	}
		
	MakePStr(scratch, pUids, pUidEnd - pUids);
	StringToNum(scratch, &stop);
	
	// no range was specified
	if (!pEnd)
		start = stop;
		
	for (;(err == noErr) && (start <= stop); start++)
		err = AccuAddPtr(pAccu, &start, sizeof(long));
	
	return (err);
}