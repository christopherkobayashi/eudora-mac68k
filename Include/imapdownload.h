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
 *	imapDownload.c
 *
 *		This file contains the functions that download messages and
 *	message bits from the server.
 **********************************************************************/
 
#ifndef IMAPDOWNLOAD_H
#define IMAPDOWNLOAD_H


// resource where we put IMAP index
#define INDEX_RES_TYPE	'IIND'
#define INDEX_RES_ID	128


// special values of offset field in summary
#define imapNeedToDownload -1

// flags for special header combinations
typedef enum 
{
	kNone = 0,
	kBody, 
	kAnyRecipient,
	kAllHeaders,
	kAnyWhere
} HeaderCombEnum;

// flags to tell empty trash routine what to do
typedef enum 
{
	kEmptyActiveTrashes = 0,
	kEmptyAutoCheckTrashes, 
	kEmptyAllTrashes
} IMAPEmptyTrashEnum;

// UIDNode struct.  Used for lists of UIDs.
typedef struct UIDNode UIDNode, *UIDNodePtr, **UIDNodeHandle;
struct UIDNode
{
	// uid of message
	unsigned long uid;
	
	// flags
	unsigned long l_seen:1;
	unsigned long l_deleted:1;
	unsigned long l_flagged:1;
	unsigned long l_answered:1;
	unsigned long l_draft:1;
	unsigned long l_recent:1;
	unsigned long l_IsNew:1;
	unsigned long l_sent:1;
	unsigned long unUsed:8;
	unsigned long boxIndex:16;	// used for IMAP searches
	
	unsigned long size;
	
	// next
	UIDNodeHandle next;
};

// IndexStruct.  Used to locate messages within a temporary IMAP message file
typedef struct IndexStruct IndexStruct, *IndexStructPtr;
struct IndexStruct
{
	unsigned long uid;
	long offset;
	long length;
};

// AttachmentStubStruct.  Used to save enough information to fecth an attachment later
typedef struct AttachmentStubStruct AttachmentStubStruct, *AttachmentStubPtr, **AttachmentStubHandle;
struct AttachmentStubStruct
{
	unsigned long persID;				// personality ID of owning personality
	unsigned long uid;					// uid of message
	char section[255];					// section string
	unsigned long sizeBytes;			// body size in bytes
	unsigned long sizeLines;			// body size in lines
};


// IMAPSCStruct - one for each header/value pair to be searched
typedef struct IMAPSCStruct IMAPSCStruct, *IMAPSCPtr, **IMAPSCHandle;
struct IMAPSCStruct
{
	Str255 string;						// String to search for
	HeaderCombEnum headerCombination;	// nothing special, body, or all headers
	short headerName;					// string id of header to search
};

// IMAPSResultStruct - structure returned for each match
typedef struct IMAPSResultStruct IMAPSResultStruct, *IMAPSResultPtr, **IMAPSResultHandle;
struct IMAPSResultStruct
{
	short box;		// index into BoxCount of match
	long uidHash;	// the matching summary
};

// special value indicating all messages in lcoal cache need to be trashed
#define MSUM_DELETE_ALL (Handle)(-1)
#define SEARCH_WINDOW (MailboxNodeHandle)(-1)

// DeliveryStruct.  Used to keep track of messages to be delivered to all opening IMAP mailboxes
typedef struct DeliveryNode DeliveryNode, *DeliveryNodePtr, **DeliveryNodeHandle;
struct DeliveryNode
{
	TOCHandle toc;				// toc to identify this deliverynode
	
	Boolean finished;			// set to true once finished
	Boolean aborted;			// set to true by the main thread to abort the download
	
	MailboxNodeHandle mailbox;	// handle to the IMAP mailbox we're updating
	Handle ta;					// stores list of summaries to be (a)dded
	Handle td;					// stores list of summaries to be (d)eleted
	Handle tu;					// stores list of summaries to be (u)pdated
	Handle tc;					// stores list of summaries to be (c)opied
		
	IMAPSResultHandle results;	// stores list of IMAP search results
	short threadCount;			// number of threads adding results to this node
	
	Boolean filter;				// set this flag if this mailbox needs to have filters run on it
	
	Boolean cleanupAttachments;	// set this flag to true when updating a mailbox after a transfer so we go clean up attachments if we ought to.
	
	DeliveryNodeHandle next;
};

// UpdateNode.  Used to keep a list of windows waiting to be updated
typedef struct UpdateNode UpdateNode, *UpdateNodePtr, **UpdateNodeHandle;
struct UpdateNode
{
	FSSpec mailboxSpec;			// the mailbox this message lives in
	unsigned long uid;			// uid of the message
	FSSpecHandle attachSpecs;	// specs pointing to the attachments that have been downloaded
	
	UpdateNodeHandle next;
};

// IMAPAppendStruct.  Contains the information necessary to upload a POP message to an IMAP server
typedef struct IMAPAppendStruct IMAPAppendStruct, *IMAPAppendPtr, **IMAPAppendHandle;
struct IMAPAppendStruct
{
	FSSpec spoolSpec;			// the spooled message
	StateEnum fromState;		// the state of the original message
	unsigned long fromFlags;	// the flags from the original message

	Boolean transferred;		// set to true when this message has been successfully transferred
	long serialNum;				// the serial number of the original POP message
	FSSpec mailbox;				// the POP mailbox this message came from
};

// UIDCopyStruct. Contains source mailbox, old UID, and new UID.  Used for main thread copys after a UIDPLUS response
typedef struct UIDCopyStruct UIDCopyStruct, *UIDCopyPtr;
struct UIDCopyStruct
{
	FSSpec toSpec;			// destination mailbox
	Handle hOldSums;		// a copy of the summaries transferred.
	Handle hNewUIDs;		// list of new UIDs from UIDPLUS response.
	Boolean copy;			// true if this was a copy
};

// global flag to turn off progress messages
extern Boolean gFilteringUnderway;

// macros to use for progress
#define CAN_PROGRESS (!(gFilteringUnderway && !InAThread()))
#define PROGRESS_START if (CAN_PROGRESS) OpenProgress()
#define PROGRESS_MESSAGE(t,m) if (CAN_PROGRESS) ProgressMessage(t,m)
#define PROGRESS_MESSAGER(t,r) if (CAN_PROGRESS) ProgressMessageR(t,r)
#define PROGRESS_BAR(a,b,c,d,e) if (CAN_PROGRESS) Progress(a,b,c,d,e);
#define BYTE_PROGRESS(a,b,c) if (CAN_PROGRESS) ByteProgress(a, b, c);
#define PROGRESS_END if (CAN_PROGRESS) CloseProgress()


// Note: if we're currently filtering, and the main thread makes a PROGRESS call, skip it.
// Otherwise, we're a foreground non-filtering task, or a background task that can display progress.

// resynch and related routines
MailboxNodeHandle FetchNewMessages(TOCHandle tocH, Boolean fetchFlags, Boolean sameThread, Boolean filter, Boolean isAutoCheck);
MailboxNodeHandle DoFetchNewMessages(FSSpecPtr mailboxSpec, Boolean fetchFlags, Boolean isAutoCheck);
Boolean IMAPDelivery(TOCHandle inToc, Handle *toAdd, Handle *toUpdate, Handle *toDelete, Handle *toCopy, Boolean *filter, IMAPSResultHandle *results, MailboxNodeHandle *mbox, Boolean *checkAttachments);
void IMAPAbortResync(TOCHandle toc);
OSErr RsyncCurPersInbox(void);
OSErr IMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task);
Boolean DoIMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task);
Boolean ResyncCurrentIMAPMailbox(void);
Boolean UpdatableIMAPState(StateEnum state);
OSErr SaveMinimalHeader(MAILSTREAM *stream);

// Message downloading routines
Boolean DoDownloadMessages(TOCHandle toc, Handle uids, Boolean attachmentsToo);
OSErr UIDDownloadMessage(TOCHandle inToc, unsigned long uid, Boolean forceForeground, Boolean attachmentsToo);
OSErr UIDDownloadMessages(TOCHandle inToc, Handle uids, Boolean forceForeground, Boolean attachmentsToo);
Boolean IMAPMessagesWaiting(TOCHandle tocH, FSSpecPtr spoolSpec);
void IMAPAbortMessageFetch(TOCHandle tocH, short sumNum);
void IMAPRemoveCachedContents(TOCHandle tocH, short sumNum);
void IMAPRemoveSelectedCachedContents(TOCHandle tocH);
void IMAPFetchSelectedMessages(TOCHandle tocH, Boolean attach);
OSErr IMAPTransferLocalCache(TOCHandle fromTocH, MSumPtr pOrigSum, TOCHandle toTocH, long newUid, Boolean copy);

// mailbox polling
void IMAPPoll(PersHandle pers);
void IMAPPollMailboxes(MailboxNodeHandle tree);
void IMAPPollMailboxTree(IMAPStreamPtr imapStream, MailboxNodeHandle tree, long numToPoll, long *remaining);

// Attachment downloading routines
Boolean IsIMAPAttachmentStub(FSSpecPtr spec);
unsigned long DownloadIMAPAttachment(FSSpecPtr spec, MailboxNodeHandle mailbox, Boolean forceForeground);
MailboxNodeHandle PETEHandleToMailboxNode(PETEHandle pte);
unsigned long DoDownloadIMAPAttachments(FSSpecHandle attachments, MailboxNodeHandle mailbox);
Boolean CanFetchAttachment(FSSpecPtr spec);
void UpdateIMAPWindows(void);
Boolean FetchAllIMAPAttachments(TOCHandle toc, short sumNum, Boolean forceForeground);
Boolean FetchAllIMAPAttachmentsBySpec(FSSpecPtr spec, MailboxNodeHandle mailbox, Boolean forceForeground);
Boolean HasStubFileAttachment(TOCHandle tocH, short sumNum);
OSErr FetchIMAPAttachment(PETEHandle pte, FSSpecPtr spec, Boolean forceForeground);
void RedisplayIMAPMessage(MyWindowPtr win);

// functions to determine the state of a given message in an IMAP mailbox
Boolean IMAPMessageDownloaded(TOCHandle t, short s);
Boolean IMAPMessageBeingDownloaded(TOCHandle t, short s);

// Message transfer
OSErr IMAPTransferMessage(TOCHandle fromTocH, TOCHandle toTocH, unsigned long uid, Boolean copy, Boolean forceForeground);
OSErr IMAPTransferMessages(TOCHandle fromTocH, TOCHandle toTocH, Handle uids, Boolean copy, Boolean forceForeground);
OSErr DoTransferMessages(TOCHandle fromTocH, TOCHandle toTocH, Handle uids, Boolean copy);
OSErr IMAPTransferMessagesToServer(TOCHandle fromTocH, TOCHandle toTocH, Handle sumNums, Boolean copy, Boolean forceForeground);
OSErr IMAPTransferMessageToServer(TOCHandle tocH, TOCHandle toTocH, short sumNum, Boolean copy, Boolean forceForeground);
OSErr DoTransferMessagesToServer(TOCHandle toTocH, IMAPAppendHandle spoolData, Boolean copy, Boolean forceForeground);
void CleanUpAttachmentsAfterIMAPTransfer(TOCHandle tocH, short sumNum);
char *FlagsString(char **flags, Boolean seen, Boolean deleted, Boolean flagged, Boolean answered, Boolean draft, Boolean recent, Boolean sent);
void UpdatePOPMailboxes(void);
OSErr IMAPTransferMessagesFromSearchWindow(TOCHandle fromTocH, TOCHandle toTocH, Boolean copy);
OSErr IMAPMoveIMAPMessages(TOCHandle fromTocH, TOCHandle toTocH, Boolean copy);

// Message deletion
Boolean IMAPDeleteMessage(TOCHandle tocH, unsigned long uid, Boolean nuke, Boolean expunge, Boolean undelete);
Boolean IMAPDeleteMessages(TOCHandle tocH, Handle uids, Boolean nuke, Boolean expunge, Boolean undelete,Boolean forceForeground);
Boolean DoDeleteMessage(TOCHandle tocH, Handle uids, Boolean nuke, Boolean expunge, Boolean undelete);
Boolean IMAPRemoveDeletedMessages(TOCHandle tocH);
Boolean DoExpungeMailbox(TOCHandle tocH);
Boolean DoExpungeMailboxLo(TOCHandle tocH, Boolean bCheckLocal);
Boolean IMAPEmptyPersTrash(void);
Boolean EmptyImapTrashes(IMAPEmptyTrashEnum which);
Boolean IMAPMarkMessageAsDeleted(TOCHandle tocH, unsigned long uid, Boolean undelete);
Boolean IMAPDeleteMessageDuringFiltering(TOCHandle tocH, PersHandle pers, unsigned long uid);
Boolean IMAPDeleteMessagesFromSearchWindow(TOCHandle tocH);

// offline commands
OSErr PerformQueuedCommands(PersHandle pers, IMAPStreamPtr imapStream, Boolean progress);
void ExecuteAllPendingIMAPCommands(void);
OSErr QueueMessFlagChange(TOCHandle tocH, short sumNum, StateEnum state, Boolean bTrashed);
Boolean PendingMessFlagChange(unsigned long uid, MailboxNodeHandle mbox);

// some ordered UID list functions
void UID_LL_OrderedInsert(UIDNodeHandle *head, UIDNodeHandle *item, Boolean isNew);
void UID_LL_Zap(UIDNodeHandle *list);

// Error reporting.
OSErr IMAPError(short operation, short explanation, OSErr err);
void IMAPAlert(IMAPStreamPtr stream, TaskKindEnum taskKind);
void IMAPWarnings(void);
void IMAPSpamWatchSupported(Boolean bSupported, Boolean bWarnIfNeeded);
void IMAPResetSpamSupportPrefs(void);

// Searching
Boolean IMAPSearch(TOCHandle searchWin, BoxCountHandle boxesToSearch, IMAPSCHandle searchCriteria,  Boolean matchAll);
Boolean DoIMAPServerSearch(TOCHandle searchWin, BoxCountHandle specs, Handle specsToSeach, IMAPSCHandle searchCriteria, Boolean matchAll, long firstUID);
void IMAPAbortSearch(TOCHandle searchWin);
Boolean IMAPSearchMailbox(TOCHandle searchWin, BoxCountHandle boxesToSearch, MailboxNodeHandle boxToSearch, IMAPSCHandle searchCriteria,  Boolean matchAlltocH, long firstUid);
void UpdateIncrementalIMAPSearches(void);
void IMAPProccessBoxesMainThread(Boolean bResync, Boolean bExpunge, Boolean bSearch);
#define IMAPUpdateIncrementalSearches() IMAPProccessBoxesMainThread(false,false,true)

// Filtering
Boolean IMAPStartFiltering(TOCHandle tocToFilter, Boolean connect);
void IMAPStartManualFiltering(void);
Boolean IMAPTermMatch(MTPtr mt, MSumPtr sum);
void IMAPStopFiltering(Boolean reallyDone);
#define IMAPPostFilterResync() IMAPProccessBoxesMainThread(true,false,false)
#define IMAPPostFilterExpunge() IMAPProccessBoxesMainThread(false,true,false)
Boolean IMAPFilteringUnderway(void);
void FlagForResync(TOCHandle tocH);
void FlagForExpunge(TOCHandle tocH);
void IMAPTocHBusy(TOCHandle tocH, Boolean busy);
void ResyncOpenMailboxes(PersHandle pers);
void ResetFilterFlags(TOCHandle tocH);
Handle IMAPFetchMessageHeadersForFiltering(TOCHandle tocH, short sumNum);
void GetNextWaitingIMAPToc(TOCHandle *toc);
OSErr IMAPMoveMessageDuringFiltering(TOCHandle fromTocH, short sumNum, TOCHandle toTocH, Boolean copy, FilterPBPtr fpb);
OSErr CacheIMAPMessageForSpamWatch(TOCHandle tocH, short sumNum);
OSErr IMAPFilterProgress(TOCHandle tocH);
OSErr DoIMAPFilterProgress(void);
void IMAPFilteringCancelled(Boolean bOverride);

// Miscellaneous utility functions
DeliveryNodeHandle FindNodeByToc(TOCHandle toc);
int pstrincmp(UPtr ps,CStr cs,short n);
Boolean IsIMAPOperationUnderway(TaskKindEnum task);
Boolean IsIMAPMailboxBusy(TOCHandle tocH);
Boolean IMAPDoQuit(void);

#ifdef	DEBUG
// Debugging
void IMAPCollectFlags(void);
#endif


// Anthony Roybal's growing mailbox problem
void MarkAsProcessed(FSSpec *spec);
Boolean HasBeenProcessed(FSSpec *spec);

#endif	//IMAPDOWNLOAD_H