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
#define FILE_NUM 112
#ifdef	IMAP

/**********************************************************************
 *	imapnetlib.c
 *
 *		This file contains the imap network functions Eudora needs to 
 *	do online IMAP.  These include low-level IMAP fetch,
 *	store, etc. for a  single mailbox or TransStream stream.
 **********************************************************************/
#include "imapnetlib.h"

#pragma segment IMAP

#define MAILTMPLEN 1024		/* size of a temporary buffer */

Boolean OpenStream(IMAPStreamPtr imapStream, char *Mailbox, Boolean readOnly);
Boolean IMAPOpenMailboxSpecifiedInStream(IMAPStreamPtr imapStream, Boolean readOnly);
Boolean LockStream(MAILSTREAM *stream);
void UnlockStream(MAILSTREAM *stream);
STRINGLIST *CommaSeparatedTextToStringlist(char *Fields);
void Trim(char *string);
Boolean SetORSearchCriteria(SEARCHPGM *pPgm, char *pHeaderList, Boolean bBody, char *pSearchString);
Boolean SetORHeaderSearchCriteria(SEARCHPGM *pPgm, char *pHeaderList, char *pSearchString);
void SetMailGets(MAILSTREAM *stream, mailgets_t oldMailGets, mailgets_t newMailGets);
void ResetMailGets(MAILSTREAM *stream, mailgets_t oldMailGets);

static char *file_gets(readfn_t readfn, void *stream, unsigned long size, GETS_DATA *md);

// this routine moves data into the MAILSTREAM's text handle.
static char *buffer_gets (readfn_t readfn, void *read_data, unsigned long size, GETS_DATA *md);

/**********************************************************************
 *	NewImapStream - initialize an IMAPStream.
 **********************************************************************/
OSErr NewImapStream(IMAPStreamPtr *imapStream, UPtr ServerName, unsigned long PortNum)
{
	OSErr err = noErr;
	
	*imapStream = NuPtr(sizeof(IMAPStreamStruct));

	if (*imapStream) WriteZero(*imapStream,sizeof(IMAPStreamStruct));
	else err = MemError();

	if (err == noErr)
	{
		(*imapStream)->MessageSizeLimit = 2000000;	// Arbitrary

		// IMAP Server identification:
		if (ServerName)
			PSCopy((*imapStream)->pServerName,ServerName);
		else
			(*imapStream)->pServerName[0] = 0;

		// Port number
		(*imapStream)->portNumber = PortNum;
	}
	else	//failed to get a new IMAPStreamStruct.
	{
		WarnUser(MEM_ERR,err);
		*imapStream = 0;
	}
		
	return (err);
}


/**********************************************************************
 *	ZapImapStream - Zap an IMAPStream.  Kill everything else it loves.
 **********************************************************************/
void ZapImapStream(IMAPStreamPtr *imapStream)
{
	// if there is no stream, nothing to clean up.
	if (!imapStream || !*imapStream) return;
	
	if ((*imapStream)->mailStream != nil)
	{
		// close the spool file if there's one open.
		if ((*imapStream)->mailStream->refN > 0) MyFSClose((*imapStream)->mailStream->refN);

#ifdef	DEBUG
		if ((*imapStream)->mailStream->flagsRefN > 0) MyFSClose((*imapStream)->mailStream->flagsRefN);
#endif
	
		// If still connected, close.
		mail_free_stream (&((*imapStream)->mailStream));
	}
		
	//Now kill it.
	ZapPtr(*imapStream);
	*imapStream = NULL;
}


/**********************************************************************
 *	OpenControlStream - Open a "control" stream to the server.
 *		Establishes the initial connection to the IMAP server.
 * 		Issues the LOGIN command.
 *		 Does NOT select a mailbox.
 **********************************************************************/
Boolean OpenControlStream(IMAPStreamPtr imapStream)
{
	// If we have an open and Authenticated stream, we're done.
	if (imapStream && IsAuthenticated(imapStream->mailStream)) return true;

	return (OpenStream(imapStream, NULL, false));
}


/**********************************************************************
 *	IsAuthenticated - return true if the IMAPStream is authenticated.
 **********************************************************************/
Boolean IsAuthenticated(MAILSTREAM *stream)
{
	if (!stream) return false;

	// If we are not connected, we can't be selected.
	if (!IsConnected(stream)) return false;

	// See if the stream is selected.
	return stream->bAuthenticated;
}


/**********************************************************************
 *	IsConnected - return true if we're connected
 **********************************************************************/
Boolean IsConnected(MAILSTREAM *stream)
{
	if (!stream || !stream->dtb) return false;
	else return (stream->dtb->connected(stream));
}


/**********************************************************************
 *	IsSelected - return true if there's a mailbox selected
 **********************************************************************/
Boolean IsSelected(MAILSTREAM *stream)
{
	if (!stream) return false;

	// If we are not connected, we can't be selected.
	if (!IsConnected(stream)) return false;

	// See if the stream is selected.
	return stream->bSelected && stream->bAuthenticated;
}


/**********************************************************************
 * OpenStream - Open an IMAP stream and select a Mailbox..  If Mailbox 
 *	is NULL, open a  "control" stream instead.  Set readOnly to true
 *	to open a READ-ONLY connection to the server.
 **********************************************************************/
Boolean OpenStream(IMAPStreamPtr imapStream, char *Mailbox, Boolean readOnly)
{
	MAILSTREAM *ps;
	Boolean result = false;
	long options = 0L;
	
	// Must have a server name or ip address.
	if (!imapStream || !imapStream->pServerName[0]) return(false);

	// If we have an open control stream, re-use it
	if (imapStream->mailStream)
	{	
		// If we are just opening a control stream, return if we are AUTHENTICATED.
		if (!Mailbox)
		{
			if (IsAuthenticated(imapStream->mailStream))
			{
				return (true);
			}
		}
		else
		{
			// if we are already connected to the given mailbox, and readOnly hasn't changed, return.
			if (IsSelected(imapStream->mailStream) 
				&& (IsReadOnly(imapStream->mailStream)==readOnly))
			{
				if (imapStream->mailStream->mailbox)
				{
					if (!strcmp(Mailbox, imapStream->mailStream->mailbox))
					{
						// We can reuse this stream. Update the message count
						FetchMailboxStatus(imapStream, Mailbox, SA_MESSAGES);
						return (true);
					}
					else
					{
						char tmp[MAILTMPLEN];
						Str255 pUser, user, pHost, host;
						unsigned long port;
						
						// the name of the mailbox stored in the mailStream might be in the {<location>}<name> format
						port = GetRLong(IMAP_PORT);
						GetPOPInfoLo(pUser, pHost, &port);

						Zero(user);
						Zero(host);
						
						strncpy(user, pUser+1, pUser[0]);
						strncpy(host, pHost+1, pHost[0]);
						sprintf(tmp,"{%s:%lu/imap/user=%s}",host,port,user);
						strcat(tmp, Mailbox);
						
						if (!strcmp(tmp, imapStream->mailStream->mailbox))
						{
							// We can reuse this stream. Update the message count
							FetchMailboxStatus(imapStream, Mailbox, SA_MESSAGES);
						 	return (true);
						}
					}
				}
			}
		}
	}

	// We have to open the stream.  Try to re-use if there's already one.
	if (!imapStream->mailStream)
	{
		// Allocate a new "stream" to pass to imapmail_open. 
		mail_new_stream (&imapStream->mailStream, imapStream->pServerName, &(imapStream->portNumber), NULL);

		if (!imapStream->mailStream) return (false);
	}
	
	// If we compiling the debug version, set the stream's debugging on.
#ifdef _DEBUG
	options |= OP_DEBUG;
#endif

	// If no mailbox, halfopen (i.e. a control stream).
	if (!Mailbox) options |= (OP_HALFOPEN | OP_SILENT);

	// open read only if we were asked to
	if (readOnly) options |= OP_READONLY;
	
	//  Note here: c-client will return our stream if success, otherwise NIL.
	// In any case, it no longer frees our stream.
	ps = mail_open (imapStream->mailStream, Mailbox, options);

	if (ps && (ps == imapStream->mailStream))
	{
		// If we were trying to SELECT a mailbox and the stream is still "half-open", fail.
		if (Mailbox && imapStream->mailStream->halfopen) result = false;
		else result = true;
	}

	return (result);
}


/**********************************************************************
 *	Noop
 **********************************************************************/	
Boolean Noop (IMAPStreamPtr imapStream)
{	
	Boolean result = false;
	
	// Must have a stream.
	if (!imapStream || !imapStream->mailStream) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Do the command
	result = mail_ping(imapStream->mailStream);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}

/**********************************************************************
 *	IMAPOpenMailbox -  Performs an IMAP "SELECT" on the mailbox. If the 
 *		connection to the mailbox has not already been made, do that 
 *		as well.
 **********************************************************************/	
Boolean IMAPOpenMailbox(IMAPStreamPtr imapStream, const char *MailboxName, Boolean readOnly)
{
	if (!MailboxName) return (false);

	// Copy pMailboxname.
	if (imapStream->mailboxName) WriteZero(imapStream->mailboxName, sizeof(imapStream->mailboxName));
	strcpy(imapStream->mailboxName, MailboxName);

	// Now call above method.
	return IMAPOpenMailboxSpecifiedInStream(imapStream, readOnly);
}


/**********************************************************************
 *	IMAPOpenMailboxSpecifiedInStream -  Performs an IMAP "SELECT" on the 
 *		mailbox stored in the imapstream.
 **********************************************************************/	
Boolean IMAPOpenMailboxSpecifiedInStream(IMAPStreamPtr imapStream, Boolean readOnly)
{
	Boolean result = false;

	// Must have a name.
	if (!imapStream->mailboxName[0]) return (false);

	// Go open or re-open the stream.
	result =  OpenStream(imapStream, imapStream->mailboxName, readOnly);
	if (result)
	{
		// Fill in some mailbox state information.
		imapStream->messageCount = imapStream->mailStream->nmsgs;
		imapStream->uidvalidity = imapStream->mailStream->uid_validity;
	}

	return result;
}


/**********************************************************************
 *	CreateIMAPMailbox -  create a mailbox with the name mailboxName
 **********************************************************************/
Boolean CreateIMAPMailbox(IMAPStreamPtr imapStream, const char *mailboxName)
{
	char pattern[MAILTMPLEN + 4];
	Boolean result = false;
		
	if (!mailboxName) return (false);

	// If the name is too long, reject it.
	if (strlen (mailboxName) > MAILTMPLEN) return (false);

	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Attempt to create it.
	strcpy(pattern, mailboxName);
	result = (mail_create(imapStream->mailStream, pattern) != 0);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}


/**********************************************************************
 *	DeleteIMAPMailbox -  delete the mailbox with the name mailboxName
 **********************************************************************/
Boolean DeleteIMAPMailbox (IMAPStreamPtr imapStream, const char *mailboxName)
{
	char pattern[MAILTMPLEN + 4];
	Boolean result = false;
	
	if (!mailboxName) return (false);

	// If the name is too long, reject it.
	if (strlen (mailboxName) > MAILTMPLEN) return (false);

	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Attempt to create it.
	strcpy(pattern, mailboxName);
	result = (mail_delete(imapStream->mailStream, pattern) != 0);

	UnlockStream(imapStream->mailStream);

	return (result);
}

/**********************************************************************
 *	RenameMailbox -  rename a mailbox
 **********************************************************************/
Boolean RenameIMAPMailbox(IMAPStreamPtr imapStream, const char *oldName, const char *newName)
{
	char oldN[MAILTMPLEN + 4];
	char newN[MAILTMPLEN + 4];
	Boolean result = false;
		
	if (!oldName || !newName) return (false);

	// If the name is too long, reject it.
	if (strlen (newName) > MAILTMPLEN) return (false);
	if (strlen (oldName) > MAILTMPLEN) return (false);
	
	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Attempt to create it.
	strcpy(oldN, oldName);
	strcpy(newN, newName);
	result = (mail_rename(imapStream->mailStream, oldN, newN) != 0);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}

/**********************************************************************
 *	Check - perform a mail check.  Works once a stream has been
 *		established, and a beeb has been SELECTED 
 **********************************************************************/
void Check(IMAPStreamPtr imapStream)
{
	if (!imapStream || !imapStream->mailStream) return;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return;

	// Do the command.
	mail_check (imapStream->mailStream);
	
	UnlockStream(imapStream->mailStream);
}


/**********************************************************************
 *	Expunge -  do a simple expunge on a stream, using the mailbox 
 *		specified therein.  
 **********************************************************************/
Boolean Expunge (IMAPStreamPtr imapStream)
{	
	Boolean result = false;
	
	if (!imapStream || !imapStream->mailStream) return (false);
	if (!IsConnected(imapStream->mailStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);
	
	// Close down idle connections to the mailbox
	if (PrefIsSet(PREF_IMAP_EXPUNGE_EXCLUSIVITY)) PrepareToExpunge(imapStream);
	
	// Do the expunge.
	result = mail_expunge (imapStream->mailStream);
	
	UnlockStream(imapStream->mailStream);
	
	return (result);
}


/**********************************************************************
 *	Logout -  you ain't seen nuthin yet
 **********************************************************************/
Boolean Logout (IMAPStreamPtr imapStream)
{
	return (false);
}


/**********************************************************************
 *	GetMessageCount - return the number of messages in the currently
 *		selected mailbox.
 **********************************************************************/
unsigned long GetMessageCount(IMAPStreamPtr imapStream)
{
	return (imapStream->messageCount);
}


/**********************************************************************
 * GetSTATUSMessageCount - return the number of messages frm the last
 *	STATUS command.	
 **********************************************************************/
unsigned long GetSTATUSMessageCount(IMAPStreamPtr imapStream)
{
	return (imapStream->mailStream->mailboxStatus.messages);
}


/**********************************************************************
 *	FetchMailboxAttributes -  Do a LIST on the given mailbox so we will
 * 		get it's attributes.
 *		mm_list is called once for each mailbox returned.
 **********************************************************************/
Boolean FetchMailboxAttributes (IMAPStreamPtr imapStream, const char *mailboxName)
{
	Boolean result = noErr;
	char pattern[MAILTMPLEN + 4];

	// allow NULL mailbox name		
//	if (!(mailboxName)) return(false);

	// If the name is too long, reject it.
	if ((mailboxName != NULL) && ((strlen (mailboxName) > MAILTMPLEN))) return(false);

	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return(false);

	if (mailboxName) strcpy (pattern, mailboxName);
	else pattern[0] = 0;
	
	// Do a LIST on the mailbox.
	mail_list(imapStream->mailStream, "", pattern);

	UnlockStream(imapStream->mailStream);
	
	return (true);
}


/**********************************************************************
 *	FetchMailboxStatus - do a STATUS on the mailbox
 **********************************************************************/
Boolean FetchMailboxStatus (IMAPStreamPtr imapStream, const char *mailboxName, long flags)
{
	long result = 0;
	char pattern[MAILTMPLEN + 4];

	// If the name is too long, reject it.
	if (strlen (mailboxName) > MAILTMPLEN) return(false);

	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return(false);

	if (mailboxName) strcpy (pattern, mailboxName);
	else pattern[0] = 0;
	
	// Fetch the status of the mailbox.
	result = mail_status (imapStream->mailStream, pattern, flags);
	
	UnlockStream(imapStream->mailStream);
	
	return (result!=0);
}


/**********************************************************************
 *	IMAPListUnSubscribed -  fetch a list of un-subscribed mailboxes
 *		from the server specified in the imapStream.  If includeMailbox 
 *		is true, Fetch inbox as a separate command.
 *		mm_list is called once for each mailbox returned.
 **********************************************************************/
Boolean IMAPListUnSubscribed (IMAPStreamPtr imapStream, const char *pReference, Boolean includeMailbox)
{
	Boolean result = false;
	char pattern[2048];
	Str255 pInbox, cInbox;
	
	// must have an imapStream
	if (!imapStream) return (false);
	
	// If we have a stream, use it, else open temporary one.
	if (!OpenControlStream(imapStream)) return (false);

	// Catch locked streams here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Fetch inbox if asked to.
	GetRString(pInbox, IMAP_INBOX_NAME);
	PtoCcpy(cInbox, pInbox);
	if (includeMailbox) mail_list (imapStream->mailStream, "", cInbox);

	// Fetch the rest.

	// Create a pattern based on the reference.
	if (pReference && *pReference)
	{
		// Make sure we don't exceed pattern buffer length.
		if (strlen (pReference) < (sizeof (pattern) - 1)) sprintf (pattern, "%s%%", pReference);
		else sprintf (pattern, "%%");
	}
	else sprintf (pattern, "%%"); 

	// Get the folder list now.
	mail_list (imapStream->mailStream, "", pattern);	
	
	UnlockStream(imapStream->mailStream);
		
	return (true);
}

/**********************************************************************
 *	UIDDeleteMessages -  delete list of messages from the currently
 *		selected mailbox.  Expunge as well if we're told to.
 **********************************************************************/
Boolean UIDDeleteMessages (IMAPStreamPtr imapStream, char *pUIDList, Boolean Expunge)
{
	Boolean result = false;
	
	if (!pUIDList) return (false);

	// Must have a MAILSTREAM ...
	if (!imapStream || !imapStream->mailStream) return (false);
	
	// and already be connected. 
	if (!IsConnected(imapStream->mailStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Set the message flags.
	result = mail_flag (imapStream->mailStream, pUIDList, "\\Deleted", ST_UID | ST_SET | ST_SILENT);

	// and do the expunge, if we ought to
	if (Expunge) mail_expunge (imapStream->mailStream);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}

/**********************************************************************
 *	UIDUnDeleteMessages -  Undelete a list of messages
 **********************************************************************/
Boolean UIDUnDeleteMessages (IMAPStreamPtr imapStream, char *pUIDList)
{
	Boolean result = true;

	// Must have a message to delete
	if (!pUIDList) return (false);

	// Must have a MAILSTREAM
	if (!imapStream || !imapStream->mailStream) return (false);
	
	// We must already be connected. 
	if (!IsConnected(imapStream->mailStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// Set the message flags.
	// NOTE: Not setting a ST_SET flag clears the flag.
	result = mail_flag (imapStream->mailStream, pUIDList, "\\Deleted", ST_UID | ST_SILENT);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}


/**********************************************************************
 *	UIDFetchEnvelope - return the envelope of the message uid
 **********************************************************************/
ENVELOPE *UIDFetchEnvelope(IMAPStreamPtr imapStream, unsigned long uid)
{
	ENVELOPE *pEnv = NULL;

	if (!imapStream || !imapStream->mailStream) return NULL;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return NULL;

	// Do the command.
	pEnv = mail_fetch_envelope (imapStream->mailStream, uid, FT_UID | FT_PEEK);

	UnlockStream(imapStream->mailStream);
	
	return pEnv;
}


/**********************************************************************
 *	UIDFetchStructure
 **********************************************************************/
IMAPBODY *UIDFetchStructure(IMAPStreamPtr imapStream, unsigned long uid)
{
	IMAPBODY *pBody = NULL;

	if (!imapStream || !imapStream->mailStream) return NULL;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return NULL;

	// special logging case
	if (PrefIsSet(PREF_IMAP_EXTRA_LOGGING) && !(LogLevel&LOG_TRANS))
	{
		// turn on all bytes logging
		LogLevel |= LOG_TRANS;
		
		// Get the BODY.
		pBody = mail_fetch_structure (imapStream->mailStream, uid, FT_UID | FT_PEEK);
	
		// turn off all bytes logging
		LogLevel &= ~LOG_TRANS;
	}
	else 
	{
		// Get the BODY.
		pBody = mail_fetch_structure (imapStream->mailStream, uid, FT_UID | FT_PEEK);
	}

	UnlockStream(imapStream->mailStream);
	
	return pBody;
}


/**********************************************************************
 *	FreeBodyStructure -  Free a body structure reutrned by 
 *		UIDFetchStructure
 **********************************************************************/
void FreeBodyStructure(IMAPBODY *pBody)
{
	if (pBody) mail_free_body (&pBody);
}


/**********************************************************************
 *	UIDFetchFlags - fetch the flags of a sequence of messages
 *		mm_elt_flags is called to do something with the recvd flags.
 **********************************************************************/
Boolean UIDFetchFlags(IMAPStreamPtr imapStream, const char *pSequence)
{
	char *pS = NULL;
	Boolean result = false;

	// Must have an open stream.
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// clear out the old results handle if there is one
	if (imapStream->mailStream->fUIDResults!=nil) 
	{
		UID_LL_Zap(&(imapStream->mailStream->fUIDResults));
		imapStream->mailStream->fUIDResults = nil;
	}
	
	// Make our own copy.
	if (!pSequence) pS = cpystr("1:*");
	else pS = cpystr(pSequence);

	if (pS)
	{
		result = mail_fetch_flags(imapStream->mailStream, pS, FT_UID | FT_PEEK);
		fs_give ((void **)&pS);
	}

	UnlockStream(imapStream->mailStream);
	
	return result;
}

/**********************************************************************
 *	FetchAllFlags - fetch all flags (1:*) and accumulate the results
 *		into a UIDList.
 **********************************************************************/	
Boolean FetchAllFlags (IMAPStreamPtr imapStream, UIDNodeHandle *uidList)
{
	return (FetchFlags (imapStream, "1:*", uidList));
}


/**********************************************************************
 *	FetchFlags - fetch UID and FLAGS of the given UID sequence.
 **********************************************************************/	
Boolean FetchFlags (IMAPStreamPtr imapStream, const char *sequence, UIDNodeHandle *uidList)
{
	Boolean result = false;
	mailgets_t oldMailGets = NULL;
	
	// Must have a UID list object. But a sequence is optional
	if (!uidList) return false;
	
	result = UIDFetchFlags (imapStream, sequence);
	
	// return the result of UIDFetchFlags.
	*uidList = imapStream->mailStream->fUIDResults;
	imapStream->mailStream->fUIDResults = nil;
	
	return (result);
}

/**********************************************************************
 *	UIDFetchFast - does nothing yet
 **********************************************************************/
IMAPFULL *UIDFetchFast(IMAPStreamPtr imapStream, unsigned long uid)
{
	return NULL;
}

/**********************************************************************
 *	UIDFetchAll - does nothing yet
 **********************************************************************/
IMAPFULL *UIDFetchAll(IMAPStreamPtr imapStream, unsigned long uid)
{
	return NULL;
}

/**********************************************************************
 *	UIDFetchFull - does nothing yet
 **********************************************************************/
IMAPFULL *UIDFetchFull(IMAPStreamPtr imapStream, unsigned long uid)
{
	return NULL;
}

/**********************************************************************
 *	UIDFetchInternalDate - does nothing yet
 **********************************************************************/
char *UIDFetchInternalDate(IMAPStreamPtr imapStream, unsigned long uid)
{
	return NULL;
}


/**********************************************************************
 *	UIDFetchHeader - get the header of the message with UID uid.
 **********************************************************************/
Boolean UIDFetchHeader(IMAPStreamPtr imapStream, unsigned long uid, Boolean file)
{
	Boolean	result = true;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;
	
	// Must have a stream, and be open
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID | FT_PEEK;		// We are using the UID* command.

	SetMailGets(imapStream->mailStream, oldMailGets, file?file_gets:buffer_gets);
	result = (mail_fetch_header(imapStream->mailStream, uid, NULL, NULL, NULL, flags) != NIL);
	ResetMailGets(imapStream->mailStream, oldMailGets);

	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDFetchMessage - Fetches complete rfc822 message, including header
 **********************************************************************/
Boolean UIDFetchMessage(IMAPStreamPtr imapStream, unsigned long uid, Boolean Peek)
{
	Boolean	result = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;

	// Must have a stream, and it would help for it to be open.	
	if (!imapStream || !imapStream->mailStream) return false;
		
	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file_gets);
	result = (mail_fetch_message(imapStream->mailStream, uid, flags) != NULL);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}

#ifdef NO_LONGER_USED
/**********************************************************************
 *	UIDFetchMessageBody - fetch message without header.
 **********************************************************************/
Boolean UIDFetchMessageBody(IMAPStreamPtr imapStream, unsigned long uid, Boolean Peek)
{
	Boolean result = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;

	// Must have a stream, and it should be open	
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file_gets);
	result = mail_fetch_text (imapStream->mailStream, uid, "1", flags);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}
#endif //NO_LONGER_USED

/**********************************************************************
 *	UIDFetchPartialMessage - fetch a range of bytes of a message
 **********************************************************************/
Boolean UIDFetchPartialMessage(IMAPStreamPtr imapStream, unsigned long uid, unsigned long first, unsigned long nBytes, Boolean Peek)
{
	Boolean result = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;

	// Must have a stream, and it should be open	
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file_gets);
	result = mail_partial_body (imapStream->mailStream, uid, "", first, nBytes, flags);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDFetchPartialMessageBody - can't be right.  Does the same as UIDFetchPartialMessage
 **********************************************************************/
Boolean	UIDFetchPartialMessageBody(IMAPStreamPtr imapStream, unsigned long uid, unsigned long first, unsigned long nBytes, Boolean Peek)
{
	Boolean result = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;
	
	// Must have a stream, and it should be open
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file_gets);
	result = mail_partial_body (imapStream->mailStream, uid, NULL, first, nBytes, flags);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDFetchPartialBodyText - fetch a section of the message
 **********************************************************************/
Boolean	UIDFetchPartialBodyText(IMAPStreamPtr imapStream, unsigned long uid, char *section, unsigned long first, unsigned long nBytes, Boolean Peek, Boolean file)
{
	Boolean result = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;
	
	// Must have a stream and it should be open	
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file?file_gets:buffer_gets);
	result = mail_partial_body (imapStream->mailStream, uid, section, first, nBytes, flags);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	FetchUID - get the UID of the msgNum message
 **********************************************************************/
unsigned long FetchUID(IMAPStreamPtr imapStream, unsigned long msgNum)
{
	unsigned long result = 0L;

	// Must have a stream
	if (!imapStream || !imapStream->mailStream) return 0L;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return 0L;

	result = mail_uid (imapStream->mailStream, msgNum);

	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDFetchRFC822Header - not yet implemented
 **********************************************************************/
Boolean UIDFetchRFC822Header(IMAPStreamPtr imapStream, unsigned long uid, char *sequence)
{
	return false;
}

/**********************************************************************
 *	UIDFetchRFC822Text - not yet implemented
 **********************************************************************/
Boolean UIDFetchRFC822Text(IMAPStreamPtr imapStream, unsigned long uid, char *sequence)
{
	return false;
}

/**********************************************************************
 *	UIDFetchRFC822HeaderFields
 **********************************************************************/
Boolean UIDFetchRFC822HeaderFields(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, char *Fields)
{
	STRINGLIST *slist;
	Boolean result = false;
	mailgets_t oldMailGets = NULL;

	// Check out the parameters
	if (!uid || !Fields) return false;

	// Must have a stream.
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Convert Fields to a STRINGLIST.
	slist = CommaSeparatedTextToStringlist (Fields);
	if (slist)
	{
		SetMailGets(imapStream->mailStream, oldMailGets, buffer_gets);
		result = (mail_fetch_header (imapStream->mailStream, uid, sequence, NULL, slist, FT_UID | FT_PEEK) != NIL);
		ResetMailGets(imapStream->mailStream, oldMailGets);
		
		// Cleanup.
		mail_free_stringlist (&slist);
	}			

	UnlockStream(imapStream->mailStream);
	
	return result;
}

/**********************************************************************
 *	UIDFetchRFC822HeaderFieldsNot - not yet implemented
 **********************************************************************/
Boolean UIDFetchRFC822HeaderFieldsNot(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, char *fields)
{
	return false;
}


/**********************************************************************
 *	UIDFetchMimeHeader - not yet implemented
 **********************************************************************/
Boolean	UIDFetchMimeHeader(IMAPStreamPtr imapStream, unsigned long uid, char *sequence)
{
	return false;
}

/**********************************************************************
 *	UIDFetchBodyText
 **********************************************************************/
Boolean	UIDFetchBodyText(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, Boolean Peek)
{
	Boolean results = false;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;
	
	// Must have a stream,ld be open
	if (!imapStream || !imapStream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Setup flags.
	flags = FT_UID;		// We are using the UID* command.
	if (Peek) flags |= FT_PEEK;

	SetMailGets(imapStream->mailStream, oldMailGets, file_gets);
	results = (mail_fetch_body (imapStream->mailStream, uid, sequence, flags) != NULL);
	ResetMailGets(imapStream->mailStream, oldMailGets);
	
	UnlockStream(imapStream->mailStream);
	
	return results;
}

/**********************************************************************
 * UIDFetchBodyTextInChunks - like UIDFetchBodyText, but nicer
 *	to cancel out of
 **********************************************************************/
Boolean	UIDFetchBodyTextInChunks(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, Boolean Peek, long size)
{
	mailgets_t oldMailGets = NULL;
	long bufferSize = 2*GetRLong(IMAP_TRANSFER_BUFFER_SIZE);
	long last = 0, nBytes = MIN(size,bufferSize);
	
	// Must have a stream, should be open
	if (!imapStream || !imapStream->mailStream) return false;

	while(!CommandPeriod && (last<size))
	{
		if (UIDFetchPartialBodyText(imapStream, uid, sequence, last, nBytes, Peek, true)) 
			last += nBytes;
		else 
			break;
		
		nBytes = MIN(nBytes, (size-last));	//set nBytes so we don't try to read past end of message
	}
	
	return ((last>=size));
}

/**********************************************************************
 *	UIDFetchPreamble - not yet implemented
 **********************************************************************/
Boolean UIDFetchPreamble(IMAPStreamPtr imapStream, unsigned long uid, char *sequence)
{
	return false;
}

/**********************************************************************
 *	UIDFetchTrailer
 **********************************************************************/
Boolean UIDFetchTrailer(IMAPStreamPtr imapStream, unsigned long uid, char *sequence)
{
	return false;
}

/**********************************************************************
 *	UIDSaveFlags - 
 **********************************************************************/
Boolean UIDSaveFlags(IMAPStreamPtr imapStream, unsigned long uid, char *uidList, IMAPFLAGS *Flags, Boolean Set, Boolean Silent)
{
	Boolean result = false;
	Str255 cUidStr;
	char *flagsStr = nil;
	
	// Must have a MAILSTREAM ...
	if (!imapStream || !imapStream->mailStream) return (false);
	
	// must have a Flags struct
	if (!Flags) return (false);
	
	// and already be connected. 
	if (!IsConnected(imapStream->mailStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// build the flag string to send to the server
	flagsStr = FlagsString(&flagsStr, Flags->SEEN, Flags->DELETED, Flags->FLAGGED, Flags->ANSWERED, Flags->DRAFT, Flags->RECENT, false);
	if (!flagsStr) return (false);
	
	// Set the message flags.
	if (!uidList)
	{
		sprintf (cUidStr,"%lu",uid);
		uidList = cUidStr;
	}
	else
	{
		strcpy(cUidStr, uidList);
	}
	result = mail_flag (imapStream->mailStream, cUidStr, flagsStr, ST_UID | (Set ? ST_SET : 0) | (Silent ? ST_SILENT : 0));

	// get rid of the flags string
	if (flagsStr) ZapPtr(flagsStr);
				
	UnlockStream(imapStream->mailStream);
			
	return (result);
}

/**********************************************************************
 *	UIDAddFlags - 
 **********************************************************************/
Boolean UIDAddFlags(IMAPStreamPtr imapStream, unsigned long uid, IMAPFLAGS *Flags, Boolean Silent)
{
	Boolean result = false;
	
	return (result);
}


/**********************************************************************
 *	UIDRemoveFlags - 
 **********************************************************************/
Boolean UIDRemoveFlags(IMAPStreamPtr imapStream, unsigned long uid, IMAPFLAGS *Flags, Boolean Silent)
{
	Boolean result = false;
	
	return (result);
}

/**********************************************************************
 *	UIDMarkAsReplied - mark a message as answered
 **********************************************************************/
Boolean UIDMarkAsReplied(IMAPStreamPtr imapStream, unsigned long uid)
{
	Boolean result = false;
	IMAPFLAGS flagsToChange;

	Zero(flagsToChange);
	flagsToChange.ANSWERED = true;
	
	result = UIDSaveFlags(imapStream, uid, nil, &flagsToChange, true, true);
	 
	return (result);
}
	
/**********************************************************************
 *	UIDCopy -  Copy a list of messages from the current mailbox to
 *		a destination mailbox.  The destination must be on the same
 *		server!
 **********************************************************************/
Boolean UIDCopy (IMAPStreamPtr imapStream, char *pUidlist, char *pDestMailbox)
{
	Boolean result = false;
	char *pList = NULL;
	char *pMbox = NULL;

	// must have valid arguments
	if (!pUidlist || !pDestMailbox) return (false);

	// must have a MAILSTREAM
	if (!imapStream || !imapStream->mailStream) return (false);
	
	// The stream MUST be open, otherwise fail.
	if (!IsConnected(imapStream->mailStream)) return (false);

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return (false);

	// clear old response
  	AccuZap(imapStream->mailStream->UIDPLUSResponse);
  	AccuInit(&imapStream->mailStream->UIDPLUSResponse);
  	imapStream->mailStream->UIDPLUSuv = 0;
  	
	result = (mail_copy_full(imapStream->mailStream, pUidlist, pDestMailbox, CP_UID) != 0);

	UnlockStream(imapStream->mailStream);
	
	return (result);
}


/**********************************************************************
 *	AppendMessage - append a message to the current mailbox
 **********************************************************************/
Boolean IMAPAppendMessage (IMAPStreamPtr imapStream, const char* Flags, long seconds, STRING *pMsg)
{
	char flgs [MAILTMPLEN + 4];
	Boolean result = false;

	// Sanity
	if (!pMsg) return false;

	// Must have a mailStrem
	if (!imapStream->mailStream) return false;

	// Must also have a mailbox name.
	if (!imapStream->mailboxName) return false;

	// Must be SELECTed
	if (!IsSelected(imapStream->mailStream)) return false;

	// Catch a locked stream here.
	if (!LockStream(imapStream->mailStream)) return false;

	// Copy Flags to an internal buffer
	*flgs = 0;
	if ((Flags) && (strlen(Flags) < MAILTMPLEN)) strcpy (flgs, Flags);
	else *flgs = 0;
	
	result = (mail_append_full(imapStream->mailStream, imapStream->mailboxName, flgs, nil, pMsg) != 0);
	
	UnlockStream(imapStream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDMessageIsMultipart -  Return true if the message is multipart
 **********************************************************************/
Boolean	 UIDMessageIsMultipart(IMAPStreamPtr stream, unsigned long uid)
{
	IMAPBODY *body = NULL;
	Boolean result = false;

	if (!stream || !stream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(stream->mailStream)) return false;

	// Look at the top body's type.
	body = mail_fetch_structure (stream->mailStream, uid, FT_UID | FT_PEEK);

	result = body && (body->type == TYPEMULTIPART);

	UnlockStream(stream->mailStream);
	
	// I was just using you for your body
	FreeBodyStructure(body);
	
	return result;
}


/**********************************************************************
 *	UIDGetTime -  
 **********************************************************************/
long UIDGetTime(IMAPStreamPtr stream, IMAPUID uid)
{
#if 0 // BUG - MUST FIX
	struct tm time;
	MESSAGECACHE	*elt = NULL;

	// Initialize.
	memset (&time, 0, sizeof(struct tm));

	// Sanity.
	if (!stream || !stream->mailStream) return 0L;

	// Get the MESSAGECACHE for this message.
	// BUG:: Should make sure the msgno is valid!!
	elt = mail_elt (stream->mailStream, UidToMsgmno (uid));

	if (elt)
	{
		time.tm_mon = elt->month;
		// elt->year is years since 1969.
		if (elt->year < 1)
			time.tm_year = 0;
		else
			time.tm_year = elt->year - 1;
		time.tm_mday = elt->day;
		time.tm_hour = elt->hours;
		time.tm_min  = elt->minutes;
		time.tm_sec  = elt->seconds;
	
		time.tm_isdst = -1;
	}

	time_t seconds = mktime(&time);

	return (seconds < 0L ? 0L : seconds);

#endif // JOK BUG

	return 0;  // FORNOW
}


/**********************************************************************
 *	GetRfc822Size - return the size of the message with UID uid
 **********************************************************************/
unsigned long GetRfc822Size(IMAPStreamPtr stream, IMAPUID uid)
{
	unsigned long result = 0;

	// Must have a stream, and be connected.
	if (!stream || !IsConnected(stream->mailStream)) return 0;

	// Catch a locked stream here.
	if (!LockStream(stream->mailStream)) return 0;

	result = mail_fetch_rfc822size(stream->mailStream, uid, FT_UID | FT_PEEK);

	UnlockStream(stream->mailStream);
	
	return result;
}


/**********************************************************************
 *	UIDVALIDITY -  return UIDVALIDITY of selected mailbox
 **********************************************************************/
UIDVALIDITY	UIDValidity(IMAPStreamPtr stream)
{
	if (stream && stream->mailStream && stream->mailboxName) return (stream->uidvalidity);
	else return NULL;
}


/**********************************************************************
 *	IsReadOnly -  Return true if this stream is rdonly.
 **********************************************************************/
Boolean IsReadOnly(MAILSTREAM *stream)
{
	return (stream->rdonly);
}

/**********************************************************************
 * UIDFind -  Do an IMAP Search of a mailbox. 
 *	Returns a list of UID nodes, ordered by UID.	
 **********************************************************************/
Boolean UIDFind(IMAPStreamPtr stream, const char *headerList, Boolean body, Boolean not, char *string, unsigned long firstUID, unsigned long lastUID, UIDNodeHandle *results)
{
	unsigned long flags = 0;
	Boolean result = false;
	SEARCHPGM* pPgm = nil;
	SEARCHPGMLIST *pNotList = nil;

	// Must have a stream.
	if (!stream || !stream->mailStream) return false;

	// Must be searching either a list of headers, or the body
	if (!body && (!headerList || !*headerList)) return (false);
	
	// Must have something to search for
	if (!string || !*string) return (false);

	// UidFirst MUST be non-zero.
	if (firstUID == 0) firstUID = 1;

	// clear any old results we may have laying around
	if (stream->mailStream->fUIDResults!=nil) 
	{
		UID_LL_Zap(&(stream->mailStream->fUIDResults));
		stream->mailStream->fUIDResults = nil;
	}
		
	// Attempt to lock the stream:
	if (!LockStream(stream->mailStream)) return (false);

	// Fill a search program with the criteria.
	pPgm = NuPtrClear(sizeof(SEARCHPGM));
	if (!pPgm) return (false);

	// Set the Uid range in the top level spgm.
	if (lastUID)
	{
		pPgm->uid = NuPtrClear(sizeof(SEARCHSET));
		if (!pPgm->uid)
		{
			mail_free_searchpgm(&pPgm);
			return (false);
		}
		pPgm->uid->first = firstUID;
		pPgm->uid->last  = (lastUID==firstUID)?0:lastUID;
	}
	
	// Go accumulate the search criteria.
	if (not)
	{
		//
		// Allocate a SEARCHPGMLIST off the ->not member, allocate a 
		// new SPGM in it, and fill that with the OR'd list.
		//

		pNotList = NuPtrClear(sizeof(SEARCHPGMLIST));

		if (!pNotList)
		{
			result = false;
			goto cleanup;
		}

		pPgm->not = pNotList;

		pNotList->next = NULL;

		// Allocate a new SPGM.
		pNotList->pgm = NuPtrClear(sizeof(SEARCHPGM));
		if (!pNotList->pgm)
		{
			result = false;
			goto cleanup;
		}

		// Set the OR criteria into this now.
		result = SetORSearchCriteria(pNotList->pgm, (char *)headerList, body, string);
	}
	else
	{
		// Uses a series of OR's
		result = SetORSearchCriteria(pPgm, (char *)headerList, body, string);
	}

	if (result)
	{
		// Do UID search
		flags |= SE_UID;
	
		// Do the search.
		result = mail_search_full(stream->mailStream, NULL, pPgm, flags);

		if (result)
		{
			// Copy the stream's results.
			*results = stream->mailStream->fUIDResults;
			stream->mailStream->fUIDResults = nil;
		}
		else
		{
			IMAPError(kIMAPSearching, kIMAPNotConnectedErr, errIMAPSearchMailboxErr);
		}
	}

cleanup:
	// Cleanup.
	mail_free_searchpgm(&pPgm);

	UnlockStream(stream->mailStream);

	return (result);
}


/**********************************************************************
 *	FetchHeader - fetch the header of the msgNum message in the current
 *		mailbox.
 **********************************************************************/
Boolean FetchHeader(IMAPStreamPtr stream, unsigned long msgNum)
{
	Boolean result = true;
	unsigned long flags;
	mailgets_t oldMailGets = NULL;

	// Should be open.
	if (!stream || !stream->mailStream) return false;

	// Catch a locked stream here.
	if (!LockStream(stream->mailStream)) return false;

	// Setup flags.
	flags = 0L;			// msgno fetch - nu UID this time.

	SetMailGets(stream->mailStream, oldMailGets, file_gets);
	result = (mail_fetch_header (stream->mailStream, msgNum, NULL, NULL, NULL, flags) != NIL);
	ResetMailGets(stream->mailStream, oldMailGets);
	
	UnlockStream(stream->mailStream);
	
	return result;
}

/**********************************************************************
 *	FetchMIMEHeader - fetch the mime header of a given message part
 **********************************************************************/
long FetchMIMEHeader(IMAPStreamPtr stream, unsigned long uid, char* section, unsigned long flags)
{
	long result = 0;
	mailgets_t oldMailGets = NULL;

	// Should be open.
	if (!stream || !stream->mailStream) return 0;

	// Catch a locked stream here.
	if (!LockStream(stream->mailStream)) return 0;

	// set up flags
	flags |= FT_UID;
	flags |= FT_PEEK;	// don't mark un-recent the message, silly -jdboyd 11/07/00

	SetMailGets(stream->mailStream, oldMailGets, file_gets);
	result = mail_fetch_mime (stream->mailStream, uid, section, flags);
	ResetMailGets(stream->mailStream, oldMailGets);
	
	UnlockStream(stream->mailStream);
	
	return result;
}


/**********************************************************************
 *	OrderedInsert - this is called once per message when doing a 
 *		UIDFetchFlags.
 **********************************************************************/
void OrderedInsert(MAILSTREAM *mailStream, unsigned long uid, Boolean seen, Boolean deleted, Boolean flagged, Boolean answered, Boolean draft, Boolean recent, Boolean sent, unsigned long size)
{
	UIDNodeHandle node = nil;
	
	node = NewZH(UIDNode);
	if (node)
	{
		(*node)->uid = uid;
		(*node)->l_seen = seen;
		(*node)->l_deleted = deleted;
		(*node)->l_flagged = flagged;
		(*node)->l_answered = answered;
		(*node)->l_draft = draft;
		(*node)->l_recent = recent;
		(*node)->l_sent = sent;
		(*node)->size = size;
		
		// ordered insert this node into the list
		UID_LL_OrderedInsert(&(mailStream->fUIDResults), &node, true);
	}
	else
	{
		WarnUser(MEM_ERR,MemError());
	}
}

/**********************************************************************
 *	LockStream - lock the stream.  Return false if it was already locked.
 **********************************************************************/
Boolean LockStream(MAILSTREAM *stream)
{	
	ASSERT(stream && (stream->lock==0));
	
	if (!stream) return false;
	
	if (stream->lock != 0) return false;
	else stream->lock = 1;
	
	return true;
}

/**********************************************************************
 *	UnlockStream - unlock a stream.
 **********************************************************************/
void UnlockStream(MAILSTREAM *stream)
{	
	ASSERT(stream && stream->lock);
	
	if (stream) stream->lock = 0;
}


/**********************************************************************
 *	CommaSeparatedTextToStringlist - Convert a comma-separated string 
 *		of strings into a STRINGLIST.
 **********************************************************************/
STRINGLIST *CommaSeparatedTextToStringlist (char *Fields)
{
	size_t		len;
	char		buf [MAILTMPLEN];
	char		*p, *q;
	STRINGLIST	*first = NULL, *last, *m;

	if (!Fields) return NULL;

	// Format the comma-separated list of fields into a STRINGLIST.
	p = Fields;
	q = NULL;
	*buf = 0;

	// Wade through Fields.
	while (p && *p)
	{
		q = strchr (p, ',');

		// Get token.
		if (q)
		{
			len = q - p;
			if (len >= MAILTMPLEN)		// Can't handle long strings.
				*buf = 0;
			else
			{
				strncpy ( buf, p, len );
				buf[len] = 0;
				Trim(buf);
			}					
			p = q + 1;
		}
		else
		{
			len = strlen (buf);
			// Must be last or only one.
			if (strlen(p) >= MAILTMPLEN)
				*buf = 0;
			else
			{
				strcpy (buf, p);
				Trim(buf);
			}
			p = NULL;   // So we stop.
		}

		// Add to stringlist if not blank.
		if (*buf)
		{
			// Get new stringlist.
			m = mail_newstringlist ();
			if (m)
			{
				m->text.data = cpystr (buf);
				m->text.size = strlen (buf);
				m->next = NULL;

				// Link in:
				if (!first)
					first = m;
				else
				{
					last = first;
					while (last->next)
						last = last->next;

					last->next = m;
				}
			}
		}
	}

	return first;
}


/**********************************************************************
 *	Trim - Trim leading and trailing blanks from the given string.
 **********************************************************************/
void Trim(char *string)
{
	char *p, *q;

	if (string)
	{
		p = q = string;

		// Look for first non-blank char.
		while (q && (*q) && (*q == ' ' || *q == '\t'))
			q++;

		// All blank?
		if (!*q)
			*p = 0;	// We're done.
		else
		{
			while (*q)
			{
				*p++ = *q++;
			}
			*p = 0;	// Tie off.

			// Strip trailing;
			q = p + strlen (p) - 1;
			while (q && (q >= p) && !(*q == ' ' || *q == '\t'))
				*q-- = 0;
		}
	}
}


/**********************************************************************
 *	SetORSearchCriteria - Accumulate the searech criteria into the 
 *		given SEARCHPGM.
 **********************************************************************/
Boolean SetORSearchCriteria (SEARCHPGM *pPgm, char *pHeaderList, Boolean bBody, char *pSearchString)
{
	Boolean result = false;

	// must have a search struct to fill, and some search criteria
	if (!pPgm || !pSearchString) return false;

	// Set this to TRUE if we succeed.
	result = false;
	
	// Do we want to search the message body?
	if (bBody)
	{
		// If headers also, use an OR.
		if (pHeaderList && *pHeaderList)
		{
			pPgm->or = NuPtrClear(sizeof(SEARCHOR));
			if (pPgm->or)
			{
				// Fill body criterion;
				pPgm->or->first = NuPtrClear(sizeof(SEARCHPGM));
				if (pPgm->or->first)
				{
					// Fill the body criterion only!
					result = SetORSearchCriteria (pPgm->or->first, NULL, bBody, pSearchString);
					if (result)
					{
						// Restart.
						result = FALSE;

						// Go add the header list.
						pPgm->or->second = NuPtrClear(sizeof(SEARCHPGM));
						if (pPgm->or->second)
						{
							// This will recursively add the headers.
							result = SetORHeaderSearchCriteria(pPgm->or->second, pHeaderList, pSearchString);
						}
					}
				}
			}
		}
		else
		{
			// No header list. We are searching just the body.
			result = false;

			pPgm->body = NuPtrClear(sizeof(STRINGLIST));
			if (pPgm->body)
			{
				pPgm->body->next = NULL;

				pPgm->body->text.data = cpystr(pSearchString);
				pPgm->body->text.size = strlen(pSearchString);

				result = TRUE;
			}
		}
	}
	else if (pHeaderList && *pHeaderList)
	{
		// Add header list.
		result = SetORHeaderSearchCriteria (pPgm, pHeaderList, pSearchString);
	}

	return result;
}


/**********************************************************************
 *	SetORHeaderSearchCriteria - pHeaderList is a comma-separated list 
 *		of headers that must be put into a OR'd list of searchprograms. 
 **********************************************************************/
Boolean SetORHeaderSearchCriteria (SEARCHPGM *pPgm, char *pHeaderList, char *pSearchString)
{
	Boolean result = false;
	char Comma = ',';
	char *p = 0;
	
	// These must be valid
	if (!pPgm || !pHeaderList || !pSearchString) return (false);

	// Set this to TRUE if we succeed.
	result = false;

	// If multiple headers, use an OR.

	// Is this the last or only header?
	p = strchr(pHeaderList, Comma);
	if (p)
	{
		// Tie off temporarily.
		*p = '\0';

		// Use an OR.
		pPgm->or = NuPtrClear(sizeof(SEARCHOR));
		if (pPgm->or)
		{
			// Add single header.
			pPgm->or->first = NuPtrClear(sizeof(SEARCHPGM));
			if (pPgm->or->first)
			{
				result = SetORSearchCriteria (pPgm->or->first, pHeaderList, false, pSearchString);
			}

			// Add rest of headers.
			if (result)
			{
				result = false;

				// Put the comma back.
				*p++ = Comma;

				// Second criteria..
				pPgm->or->second = NuPtrClear(sizeof (SEARCHPGM));
				if (pPgm->or->second)
				{
					// This will recursively add the headers.
					result = SetORSearchCriteria (pPgm->or->second, p, false, pSearchString);
				}
			}
		}
	} // if p.
	else
	{
		result = false;

		// Single header. Add it.
		pPgm->IMAPheader = NuPtrClear(sizeof(SEARCHHEADER));
		if (pPgm->IMAPheader)
		{
			pPgm->IMAPheader->line = cpystr(pHeaderList);
			pPgm->IMAPheader->text = cpystr(pSearchString);

			pPgm->IMAPheader->next = NULL;

			result = true;
		}
	}

	return result;
}

/**********************************************************************
 *	SetMailGets - set the function that will get called when a line
 *		of data is received from the server.  Remember the old one.
 **********************************************************************/
void SetMailGets(MAILSTREAM *stream, mailgets_t oldMailGets, mailgets_t newMailGets)
{
	if (stream)
	{
		oldMailGets = (mailgets_t) mail_parameters(stream, GET_GETS, NULL);
		mail_parameters(stream, SET_GETS, newMailGets);
	}
}

/**********************************************************************
 *	ResetMailGets - set the function that gets called per line of data
 **********************************************************************/
void ResetMailGets(MAILSTREAM *stream, mailgets_t oldMailGets)
{
	if (stream)
	{
		mail_parameters(stream, SET_GETS, oldMailGets);
	}
}

/**********************************************************************
 *	UIDFetchLastUid - fetch the highest uid in the selected mailbox
 **********************************************************************/
unsigned long UIDFetchLastUid(IMAPStreamPtr imapStream)
{
	unsigned long uid = 0;
	UIDNodeHandle uidList = nil, node = nil;
	
	// If we don't have a mailbox open, fail.
	if (!imapStream) return 0;

	// If no messages in the mailbox, we can't do this.
	if (GetMessageCount(imapStream) <= 0) return 0;

	// Fetch flags
	if (FetchFlags(imapStream, "*", &uidList))
	{
		if (uidList)
		{
			LL_Last(uidList,node);
			if (node)
			{
				uid = (*node)->uid;
			}
			UID_LL_Zap(&uidList);
		}
	}
	return uid;
}

/**********************************************************************
 *	UIDFetchPartialContentsToBuffer - Fetch bytes between the offsets 
 *	 "first" and "last" of the body part.   Return the length of text 
 *	 obtained in pBuffer in the output parameter: pLen.
 **********************************************************************/
Boolean UIDFetchPartialContentsToBuffer(IMAPStreamPtr imapStream, unsigned long uid, char *sequence, int first, unsigned long nBytes, char *buffer, unsigned long bufferSize, unsigned long *len)
{
	Boolean	result = false;
	unsigned long length = 0;
	mailgets_t oldMailGets = NULL;

	// must have a buffer and some length
	if (!buffer || bufferSize < 1 || !len || nBytes < 1) return false;
		
	// Must have a stream and it should be open	
	if (!imapStream || !imapStream->mailStream) return false;

	// Init
	*len = 0;

	// go do the partial fetch now
	result = UIDFetchPartialBodyText(imapStream, uid, sequence, first, nBytes, true, false);
	if (result)
	{
		// we must have received something from the server.
		if (!imapStream->mailStream->fNetData) 
		{
			result = false;
			CommandPeriod = true;
		}
		else
		{
			// figure out how much data we got
			if ((length = strlen(*(imapStream->mailStream->fNetData))) == nBytes)
			{
				// copy it to the buffer we were passed
				WriteZero(buffer, bufferSize);
				LDRef(imapStream->mailStream->fNetData);
				strncpy(buffer,*(imapStream->mailStream->fNetData),length);
				UL(imapStream->mailStream->fNetData);
			}
			else
				// we didn't get the amount of data we asked for, most likely because the server doesn't support partial fetches correctly.
				result = false;	
			
			// destroy the stream's buffer
			ZapHandle(imapStream->mailStream->fNetData);
		}
	}	

	// Output: 
	*len = length;

	return (result);
}

/**********************************************************************
 *	header_string_builder - the imap library can call this function
 *		to move data from some buffer (primarily the network buffer)
 *		to a handle inside the stream we can get at later.
 **********************************************************************/
static char *buffer_gets (readfn_t readfn, void *read_data, unsigned long size, GETS_DATA *md)
{
	MAILSTREAM *mailStream = NULL;
	Boolean result = false;
	
	// must have a read function and some data to read
	if (!readfn || !read_data) return (nil);

	// Nothing to read?
	if (size <= 0) return (nil);
	
	// must have a mailstream
	if (!md) return (nil);

	// Extract our stream.
	mailStream = md->stream;
	
	// is there an existing buffer?  Trash it.
	if (mailStream->fNetData) ZapHandle(mailStream->fNetData);
	
	// Allocate our MAILSTREAM buffer.  Add a char 'cause it's gonna get NULL terminated.
	if (!mailStream->fNetData) mailStream->fNetData = NuHandle(size+1);

	// Did we get a buffer??
	if (!mailStream->fNetData || !*(mailStream->fNetData)) return (nil);

	// Nothing in the buffer yet.
	**(mailStream->fNetData) = 0;

	// read the data into the buffer
	LDRef(mailStream->fNetData);
	result = (*readfn)(read_data, size, *(mailStream->fNetData));
	UL(mailStream->fNetData);
	
	// throw away what we got if there's a problem
	if (!result) ZapHandle(mailStream->fNetData);
	
	return (nil);
}

// This is what c-client calls to save the data.
// NOTES
// The "size" parameter if the size of the total data.
// END NOTES
static char *file_gets (readfn_t readfn, void *read_data, unsigned long size, GETS_DATA *md)
{
	MAILSTREAM *mailStream = NULL;
	Boolean result;
	Str255 buffer;
	long readSize, totalSize;
	OSErr err = noErr;

	// must have been passed a function to read bytes
	if (!readfn || !read_data) return (nil);

	// must have been asked to read some bytes 
	if (size < 0) return (nil);

	// Nothing to read?
	if (size == 0) return (nil);
	
	// must have a stream
	if (!md) return (nil);
	
	// Extract our stream.
	mailStream = md->stream;
	
	// now write the stuff we get from the network line by line to the spool file
	totalSize = 0;
	do
	{
		readSize = MIN(sizeof(buffer)-1,size-totalSize);
		result = (*readfn)(read_data, readSize, buffer);
			
		// remember the number of bytes we've read.  
		totalSize += readSize;

#ifdef	DEBUG
		ASSERT (readSize==strlen(buffer));
#endif

		ASSERT(!InAThread() || CurThreadGlobals != &ThreadGlobals);

		// write the line to the spool file
		if (result) NCWrite(mailStream->refN,&readSize,buffer);
		
		// and display some progress when downloading message bodies, once a second, and when we're done
		if (mailStream->showProgress && mailStream->totalTransfer)
		{
			long ticks = TickCount();
			
			mailStream->currentTransfer += readSize;
			if (((ticks - mailStream->lastProgress) > 60) || (mailStream->currentTransfer==mailStream->totalTransfer))
			{
				ByteProgress(nil,mailStream->currentTransfer,mailStream->totalTransfer);
				mailStream->lastProgress = ticks;	
			}
		}
	}
	while ((totalSize < size) && result && (err==noErr));
	
	return (nil);
}  	
#endif	//IMAP

#ifdef DEBUG
/**********************************************************************
 * LoMemCheck - periodically check the value of $20.  Tracking down
 *	a reported bug where IMAP messages get stuffed into lo mem.
 **********************************************************************/
long LoMemCheck(void)
{
	long ret = 0;
	static Str255 lastTwenty;
	Str255 last;
	char *c;
	short i;
	const short goodBit = 20;
	
	c = (char *)0x20;
	last[0] = goodBit;
	for (i = 1; i <= last[0]; i++) last[i] = *c++;
		
	ASSERT (StringSame(last, lastTwenty));
	
	PCopy(lastTwenty, last);
		
#undef TickCount
	ret = TickCount();
#define TickCount LoMemCheck

	return (ret);
}
#endif