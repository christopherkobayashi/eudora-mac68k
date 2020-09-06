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
#define	FILE_NUM 114
#ifdef	IMAP


/**********************************************************************
 *	imapDownload.c
 *
 *		This file contains the functions that download messages and
 *	message bits from the server.
 **********************************************************************/
 
#include "imapdownload.h"

/* structs and stuff ... */

#define IMAP_SEEN_STRING_FLAG				"\\Seen"
#define IMAP_DELETED_STRING_FLAG			"\\Deleted"
#define IMAP_FLAGGED_STRING_FLAG			"\\Flagged"
#define IMAP_ANSWERED_STRING_FLAG			"\\Answered"
#define IMAP_DRAFT_STRING_FLAG				"\\Draft"

unsigned char *dashes = "\p--";

// for message decoding
LineIOP Lip;
long gIMAPMsgEnd;

// download error codes
enum
{
	IMAPParamErr=1,
	IMAPOperationAlreadyUnderwayErr,
	IMAPResyncFailed
};

// global handle to list of pending deliveries
DeliveryNodeHandle gDeliveryQueue = nil;

// global handle of uids that need updating on screen
UpdateNodeHandle gWindowUpdates = nil;

// global IMAP Stream for FILTERING only
IMAPStreamPtr gFilterStream = nil;

// global pershandle for FILTERING only
PersHandle gFilterPers = nil;

// global flag to turn off progress messages
Boolean gFilteringUnderway = false;
long gManualFilteringUnderway = false;

// global list of changes needed to be made to POP mailboxes
IMAPAppendHandle gSpoolData = nil;
Boolean gSpoolDataLocked = false;

// globals indicating what mailbox is being filtered
TOCHandle 	gFilterTocH;
short		gNumUnfiltered;
Boolean		gFilteringCancelled;

// global indicating incremental searches need to be updated
Boolean gUpdateIncrementalSearches;

// This driver reads messages chunk by chunk from an IMAP server
static void StreamAppendDriverInit(STRING *s, void *data, unsigned long size);
static char StreamAppendDriverNext(STRING *s);
static void StreamAppendDriverSetpos(STRING *s, unsigned long i);

STRINGDRIVER StreamAppendDriver = 
{
	StreamAppendDriverInit,
	StreamAppendDriverNext,
	StreamAppendDriverSetpos
};

// STRINGDataStruct.  Contains the data necessary for a STRING to connect and fetch a message from the source server
typedef struct STRINGDataStruct STRINGDataStruct, *STRINGDataStructPtr;
struct STRINGDataStruct
{
	unsigned long uid;			// uid of message to fetch
	UPtr buffer;				// buffer to store partial fetch in
	long bufferSize;			// size of buffer
	long bytesRead;				// number of bytes read
	IMAPStreamPtr imapStream;	// where to connect to.
	Boolean result;				// set to FALSE if something's gone wrong
};

// This driver reads messages chunk by chunk from a spooled IMAP message
static void FileAppendDriverInit(STRING *s, void *data, unsigned long size);
static char FileAppendDriverNext(STRING *s);
static void FileAppendDriverSetpos(STRING *s, unsigned long i);

STRINGDRIVER FileAppendDriver = 
{
	FileAppendDriverInit,
	FileAppendDriverNext,
	FileAppendDriverSetpos
};

// STRINGFileStruct.  Contains the data necessary for a STRING to open and read from a file
typedef struct STRINGFileStruct STRINGFileStruct, *STRINGFileStructPtr;
struct STRINGFileStruct
{
	FSSpec	spoolSpec;			// spec of spooled message
	UPtr buffer;				// buffer to store partial fetch in
	long bufferSize;			// size of buffer
	long bytesRead;				// number of bytes read
};

/* functions defined herewithintotherefore */

//
// Mailbox Resync
//

OSErr CheckIMAPSettingsForPers(void);
OSErr SpecToUIDList(TOCHandle tocH, UIDNodeHandle *uidList, DeliveryNodeHandle node);
void MergeUidLists(UIDNodeHandle *localList, UIDNodeHandle *remoteList, UIDNodeHandle *updateList);
Boolean UIDFetchMessages(IMAPStreamPtr imapStream, MailboxNodeHandle mailboxInfo, UIDNodeHandle uidList, DeliveryNodeHandle delivery, Boolean isAutoCheck);
Handle UIDNodeList2Handle(UIDNodeHandle *uidList);
unsigned long GetLocalHighestUid(UIDNodeHandle list);
unsigned long GetUIDNodeCount(UIDNodeHandle list);
void ParseHeaderInMemory (MSumPtr sum, Handle headersH);

//
//	Delivery Queue
//
DeliveryNodeHandle NewDeliveryNode(TOCHandle tocH);
void QueueDeliveryNode(DeliveryNodeHandle node);
void DequeueDeliveryNode(DeliveryNodeHandle node);
void AbortDeliveryNode(DeliveryNodeHandle node);
void ZapDeliveryNode(DeliveryNodeHandle *node);
Boolean SameTOC(TOCHandle toc1, TOCHandle toc2);


//
// Message Fetching
//
Boolean DoDownloadSingleMessage(IMAPStreamPtr imapStream, unsigned long uid, Boolean progress, Boolean attachmentsToo);
short OpenMessDestFile(MailboxNodeHandle mailboxInfo, unsigned long uid, FSSpecPtr spoolSpec);
void IMAPTempFileName(UIDVALIDITY uv, unsigned long uid, Str31 fileName);
OSErr UpdateIMAPTempFileIndex(IMAPStreamPtr imapStream, unsigned long uid, long oldFilePosition, long newFilePosition);
OSErr GetMessDestFilePos(IMAPStreamPtr imapStream, long *pos);
Boolean IsSpooledMessageFile(Str255 name, MailboxNodeHandle node);
Boolean DownloadSimpleBodyToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, IMAPBODY *body, char *section);
Boolean AppendBodyTextToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, char *section, long totalSize);
Boolean DownloadMultipartBodyToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, IMAPBODY *parentBody, char * parentSection, Boolean doingRelated);
void IMAPWriteBoundary(IMAPStreamPtr stream, IMAPBODY *body, unsigned long uid, char *section, Boolean outerBoundary);
Boolean PrefMakeAttachmentStub(long size);
Boolean GonnaGetThisAppleDouble(IMAPBODY *body);
Boolean WhackPString(UPtr line);
Boolean SpecialCaseDownload(IMAPBODY *bodyInQuestion);
Boolean DoUIDMarkAsDeleted(IMAPStreamPtr stream, Handle uids, Boolean undelete);
Boolean IMAPMessageBeingPreviewed(TOCHandle tocH, short sumNum);
void CleanUpResyncTaskErrors(MailboxNodeHandle mbox);

//
//	Message Transfer
//
OSErr TransferMessageBetweenServers(PersHandle fromPersPers,  IMAPStreamPtr fromStream, MailboxNodeHandle fromBox, TOCHandle fromTocH, PersHandle toPers, IMAPStreamPtr toStream, MailboxNodeHandle toBox, Handle uids, Boolean copy);
OSErr TransferMessageOnServer(PersHandle fromPers, IMAPStreamPtr fromStream, MailboxNodeHandle fromBox, TOCHandle fromTocH, MailboxNodeHandle toBox, Handle uids, Boolean copy);
Boolean CopyMessages(IMAPStreamPtr imapStream, MailboxNodeHandle mbox, Handle uids, Boolean copy, Boolean silent);
OSErr UpdateLocalSummaries(TOCHandle fromTocH, Handle uids, Boolean undelete, Boolean expunge, Boolean cleanupAttachments);
OSErr AppendSpoolFile(IMAPStreamPtr imapStream, IMAPAppendPtr spoolData);
Boolean NeedCleanUpAttachmentsAfterIMAPTransfer(TOCHandle tocH, short sumNum);
OSErr SpoolOnePopMessage(TOCHandle tocH, short sumNum, IMAPAppendPtr spoolData);

//
// UID list manipulation
//
short GenerateUIDString(Handle uids, short index, Str255 uidStr);
short GenerateUIDStringFromUIDNodeHandle(UIDNodeHandle uids, short index,  unsigned long *firstUID, unsigned long *lastUID, Str255 uidStr);
int UIDListCompare(unsigned long *uid1, unsigned long *uid2);
void SwapUID(unsigned long *uid1, unsigned long *uid2);
void SortUIDListHandle(Handle toSort);

//
//	Fancy Trash
//
OSErr FTMExpunge(IMAPStreamPtr imapStream, TOCHandle tocH);
Boolean IMAPWipeBox(TOCHandle toc, MailboxNodeHandle node);

//
//	Stub File
//
void GetFilenameParameter(IMAPBODY *pBody, Str255 fileName);
Boolean SaveAttachmentStub(IMAPStreamPtr stream, unsigned long uid, char *section, IMAPBODY *body);
PStr BodyTypeCodeToPString (short type, PStr string);
void XMacHeaders(IMAPBODY *body, ResType *type, ResType *creator);
OSErr GetStubInfo(FSSpecPtr spec, AttachmentStubPtr stub);

//
// Attachment downloading and decoding
//
Boolean DownloadAttachmentToSpoolFile(IMAPStreamPtr imapStream, IMAPBODY *parentBody, unsigned long uid, char *parentSection, char *sectionToFetch);
Boolean SpoolFileToAttachment(MailboxNodeHandle mailbox, unsigned long uid, FSSpecPtr spoolSpec, FSSpecPtr attachSpec);
short NewScratchFile(FSSpecPtr where, FSSpecPtr scratchFile);
OSErr DeleteScratchFile(short ref, FSSpecPtr scratchFile);
void FindFirstAttachmentInSpec(FSSpecPtr inSpec, FSSpecPtr attachSpec);
OSErr IMAPRecvLine(TransStream stream, UPtr buffer, long *size);
unsigned long DownloadIMAPAttachments(FSSpecHandle attachments, MailboxNodeHandle mailbox, Boolean forceForeground);
void PrepareDownloadProgress(IMAPStreamPtr imapStream, long totalSize);
Boolean RedoIMAPAttachmentIcons(MyWindowPtr win, PETEHandle pte, FSSpecHandle attachSpecs);

//
// Searching
//
Boolean IMAPSearchServer(TOCHandle searchWin, PersHandle pers, BoxCountHandle boxesToSearch, Handle specsToSeach, IMAPSCHandle searchCriteria, Boolean matchAll, long firstUID, Boolean bAlreadyOnline);
void BuildSearchResults(DeliveryNodeHandle delivery, UIDNodeHandle results);
Boolean ReturnSearchHits(IMAPStreamPtr imapStream, DeliveryNodeHandle searchNode, FSSpecPtr spec, UIDNodeHandle *uids);
void BuildHeaderSearchString(Boolean anyHeader, Boolean anyRec, Str255 pHeaders);
void IMAPMailboxUIDRange(IMAPStreamPtr imapStream, unsigned long numMessages, unsigned long *first, unsigned long *last);
void IMAPMailboxChanged(MailboxNodeHandle mbox);

//
// Filtering
//
void IMAPMailboxPostProcess(MailboxNodeHandle tree, Boolean resync, Boolean expunge, Boolean search);
void RNtoR(Handle text);
Boolean IMAPFilteringWarnOffline(void);
MailboxNodeHandle GetNextWaitingMailboxNode(MailboxNodeHandle tree);
long IMAPCountUnfilteredMessages(MailboxNodeHandle mbox);

//
// Error reporting
//
OSErr IE(IMAPStreamPtr imapStream, short operation, short explanation, OSErr err);

//
// Offline Message Flag updates
//
OSErr PerformQueuedCommandsLo(PersHandle pers, MailboxNodeHandle tree, IMAPStreamPtr imapStream, Boolean progress);
OSErr UpdateMessFlags(IMAPStreamPtr imapStream, MailboxNodeHandle mailbox, Boolean progress);
Boolean LockMailboxNodeFlags(MailboxNodeHandle mailbox);
Boolean UnlockMailboxNodeFlags(MailboxNodeHandle mailbox);
Boolean SameFlags(LocalFlagChangePtr one, LocalFlagChangePtr two);
Boolean ProcessSimilarFlagChanges(IMAPStreamPtr imapStream, MailboxNodeHandle mailbox, Handle uidsToChange, LocalFlagChangePtr theFlags, Boolean progress, long totalFlags, long *processedFlags);
Boolean FastIMAPMessageDelete(TOCHandle tocH, Handle uids, Boolean bFTM);

#ifdef DEBUG
//
// Debug
//
void IMAPCollectFlagsFromTree(IMAPStreamPtr imapStream, MailboxNodeHandle *tree, short refN);
short OpenFlagsFile(void);
#endif

/*	Let the fun begin ... */


/**********************************************************************
 *	CheckIMAPSettingsForPers - get the user's password if we need it and
 *	make sure the special mailboxes have been defined.
 **********************************************************************/
OSErr CheckIMAPSettingsForPers(void)
{
	OSErr err = noErr;
	
	if (!CurPers) return fnfErr;
	
	// prompt for the user's password if we need it
	if ((!PrefIsSet(PREF_KERBEROS)) && !(*(*CurPers)->password))
		err = PersFillPw(CurPers,0);
	
	// make sure the user has selected the special mailboxes already
	if (!EnsureSpecialMailboxes(CurPers))
		err = userCancelled;
	
	return err;
}

/**********************************************************************
 *	FetchNewMessages - fetch the new messages for a given mailbox.
 *		Assume CurPers is the personality the mailbox belongs to.
 *
 *	If filter is true, run filters on this mailbox if it's approproate.
 *
 *	Return pointer to the mailboxNode that we just modified,
 *	nil if we fail.
 *
 *	Call IMAPDelivery() routine to get at results of download.
 **********************************************************************/
MailboxNodeHandle FetchNewMessages(TOCHandle tocToSync, Boolean fetchFlags, Boolean sameThread, Boolean filter, Boolean isAutoCheck)
{
	MailboxNodeHandle mailboxNode = nil;
	OSErr err = noErr;
	XferFlags flags;
	DeliveryNodeHandle node;
	IMAPTransferRec imapInfo;
	PersHandle pers = nil, oldPers = CurPers;
	FSSpec mailboxSpec;
	
	// Something bad happened.  We weren't told which mailbox to resync.
	if (!tocToSync) return (nil);
		
	// if there's already a delivery node set up for this TOC, then the resync is already underway.
	if (tocToSync && FindNodeByToc(tocToSync)) 
		return (nil);

	mailboxSpec = (*tocToSync)->mailbox.spec;
	
	// if we're in a thread, we must have already asked to go online.
	ASSERT(InAThread()?!Offline:true);
		
	// we must be o(nline ...
	if (Offline && (sameThread || GoOnline())) return(nil);
	
	// don't resync mailboxes if we're offline and not connected.  Allow manual mailchecks to go through, though.
	if (!IMAPCheckThreadRunning && (!AutoCheckOK())) return (nil);
			
	// set up the structure that the summaries will be returned in.
	node = NewDeliveryNode(tocToSync);
	if (node)
	{										
		// figure out who this mailbox belongs to
		mailboxNode = TOCToMbox((*node)->toc);
		pers = TOCToPers((*node)->toc);
		
		// remove any old task errors
		CleanUpResyncTaskErrors(mailboxNode);
				
		// if this is the inbox, set the flag in the delivery node so filtering gets done later, unless we're told not to filter.
		CurPers = pers;
		if (filter && (mailboxNode == LocateInboxForPers(pers)) && !PrefIsSet(PREF_IMAP_NO_FILTER_INBOX)) 
			(*node)->filter = true;
		else 
			(*node)->filter = false;
		CurPers = oldPers;
		
		// and queue it in the global list of deliveries.  It will be removed in the idle loop or the thread
		QueueDeliveryNode(node);
		
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable() || sameThread)
		{
			mailboxNode = DoFetchNewMessages(&mailboxSpec, fetchFlags, isAutoCheck);
			// Succeeded?  Write potentially new mailbox info back to the mailboxes 'IMAP' resource.
			if (mailboxNode && !sameThread)
			{
				err = WriteIMAPMailboxInfo(&mailboxSpec, mailboxNode);
			}
		}
		else 
		{				
			// collect password now if we need it.
			CurPers = pers;
			err = CheckIMAPSettingsForPers();

			// if we were given a password, set up the thread
			if (err == noErr)
			{
				Zero(flags);
				Zero(imapInfo);
				SimpleMakeFSSpec(mailboxSpec.vRefNum, mailboxSpec.parID, mailboxSpec.name, &(imapInfo.targetSpec));
				imapInfo.command = IMAPResyncTask;

				err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
				
				// if there was an error, the delivery node we created is now expendable.
				if (err != noErr) AbortDeliveryNode(node);
				
				if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
			}
			CurPers = oldPers;
		}
	}
	else
		WarnUser(MEM_ERR,MemError());
		
	return (mailboxNode);
}

/**********************************************************************
 *	IMAPProcessMailboxes - fetch the new messages in all the mailboxes
 *		specified.  Filters will be run on the appropriate mailboxes.
 *
 *	Do this all over one single connection.
 **********************************************************************/
OSErr IMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task)
{
	OSErr err = noErr;
	IMAPTransferRec imapInfo;
	FSSpec spec;
	short i,numSpecs;
	short numResync = 0;
	IMAPMailboxAttributes att;
	FSSpecHandle resync = nil;
	PersHandle pers = CurPers;
	MailboxNodeHandle node;
	XferFlags flags;
		
	// must be the right kind of command
	if ((task != IMAPMultResyncTask) && (task != IMAPMultExpungeTask))
		return (paramErr);
	
	// we must be online
	if (Offline && GoOnline()) return(paramErr);
		
	// must have some mailboxes to resync
	if (!mailboxes || !*mailboxes) return (paramErr);
	numSpecs = GetHandleSize(mailboxes)/sizeof(FSSpec);
	
	resync = NuHandle(numSpecs * sizeof(FSSpec));
	if (resync && (err=MemError())==noErr)
	{			
		// Go through the mailboxes, and create a list of IMAP mailbox pointers to resync.
		for (i = 0; i < numSpecs; i++)
		{
			spec = ((FSSpecPtr)(*mailboxes))[i];
				
			// Get this mailbox' attributes
			if (MailboxAttributes(&spec, &att))
			{
				// if this is a selectable IMAP mailbox, add it to the list of boxes to be resynced.
				if (!(att.noSelect))
				{
					BMD(&spec, &((FSSpecPtr)(*resync))[numResync++], sizeof(FSSpec));
					
					// also, collect the password for this personality if we need it.
					LocateNodeBySpecInAllPersTrees(&spec, &node, &CurPers);
					err = CheckIMAPSettingsForPers();
														
					// and remove any old task errors
					CleanUpResyncTaskErrors(node);
				}
			}
			// else
				// no attributes found - probably not an IMAP mailbox.
		}
	}
	else
	{
		WarnUser(MEM_ERR, err);
		ZapHandle(resync);
	}

	// done with the mailboxes handle.
	ZapHandle(mailboxes);
	
	// reset personality
	CurPers = pers;
	
	if (resync && numResync)
	{
		// shorten the resync handle
		SetHandleSize(resync, numResync*sizeof(FSSpec));
		
		// if there are any mailboxes to resynchronize, do it.
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable()) err = DoIMAPProcessMailboxes(resync, task) ? noErr : 1;
		else 
		{				
			Zero(imapInfo);
			Zero(flags);
			imapInfo.command = task;
			imapInfo.toResync = resync;
			err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
		}	

		// if we're using the keychain, go forget the passwords we just asked for
		// unless we're in a thread!  In that case, the password will get zapped when the thread ends.
		if (!InAThread() && KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) 
		{
			for (pers = PersList; pers; pers = (*pers)->next) 
				Zero((*CurPers)->password);
		}
	}
	else
	{
		ZapHandle(resync);
	}
	
	return err;
}

/**********************************************************************
 * ResyncCurrentIMAPMailbox - fetch the new messages in the currently 
 * 	active and frontmost IMAP mailbox
 **********************************************************************/
Boolean ResyncCurrentIMAPMailbox(void)
{
	OSErr err = noErr;
	IMAPTransferRec imapInfo;
	FSSpec spec;
	Boolean resyncMe = false;
	IMAPMailboxAttributes att;
	FSSpecHandle resync = nil;
	PersHandle pers = CurPers;
	MailboxNodeHandle node;
	XferFlags flags;
	WindowPtr	winWP = MyFrontWindow();
	MyWindowPtr win = GetWindowMyWindowPtr (winWP);
	short kind = winWP?GetWindowKind(winWP) : 0;
	TOCHandle tocH = nil;
	
	// must have a mailbox to resync
	if (!win || !(kind == MBOX_WIN)) return (false);
	
	tocH = (TOCHandle)GetMyWindowPrivateData(win);
	
	// must be an IMAP mailbox
	if (!(*tocH)->imapTOC) return (false);
	
	// we must be online
	if (Offline && GoOnline()) return(true);	// return as if the resync got started.
	
	resync = NuHandle(sizeof(FSSpec));
	if (resync && (err=MemError())==noErr)
	{			
		spec = (*tocH)->mailbox.spec;
		
		// Get this mailbox' attributes
		if (MailboxAttributes(&spec, &att))
		{
			// if this is a selectable IMAP mailbox, add it to the list of boxes to be resynced.
			if (!(att.noSelect))
			{
				BMD(&spec, *resync, sizeof(FSSpec));
				
				// also, collect the password for this personality if we need it.
				node = TOCToMbox(tocH);
				CurPers = TOCToPers(tocH);
			
				err = CheckIMAPSettingsForPers();
				
				// and remove any old task errors
				CleanUpResyncTaskErrors(node);
						
				resyncMe = true;	// we have something to resync
			}
		}
		// else
			// no attributes found - probably not an IMAP mailbox.
	}
	else
	{
		WarnUser(MEM_ERR, err);
		ZapHandle(resync);
	}
	
	// reset personality
	CurPers = pers;
	
	if (resync && resyncMe)
	{
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable()) err = DoIMAPProcessMailboxes(resync, IMAPMultResyncTask) ? noErr : 1;
		else 
		{				
			Zero(imapInfo);
			Zero(flags);
			imapInfo.command = IMAPMultResyncTask;
			imapInfo.toResync = resync;
			err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
		}	

		// if we're using the keychain, go forget the passwords we just asked for
		if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) 
		{
			for (pers = PersList; pers; pers = (*pers)->next) 
				Zero((*CurPers)->password);
		}
	}
	else
	{
		ZapHandle(resync);
	}
	
	return (true);
}

/**********************************************************************
 *	DoIMAPProcessMailboxes - Process the specified mailboxes.  Assume the
 *	 mailboxes specified are IMAP mailboxes that are SELECTable.
 **********************************************************************/
Boolean DoIMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task)
{
	Boolean result = true;
	FSSpec specToProcess;
	TOCHandle tocToProcess;
	short numSpecs = GetHandleSize(mailboxes)/sizeof(FSSpec);
	short i;
	DeliveryNodeHandle node;
	MailboxNodeHandle mailboxNode;
	PersHandle pers = nil, oldPers = CurPers;
			
	// loop through the mailboxes and process each, one by one.
	for (i = 0; (i < numSpecs) && !CommandPeriod; i++) 
	{
		// find the mailbox we want to resynchronize, open it if necessary
		specToProcess = ((FSSpecPtr)(*mailboxes))[i];
		tocToProcess = TOCBySpec(&specToProcess);
		
		if (tocToProcess)
		{		
			// don't let the toc go anywhere
			IMAPTocHBusy(tocToProcess, true);
				
			if (task == IMAPMultResyncTask)
			{
				// if there's already a delivery node set up for this TOC, then the resync is already underway.
				if (tocToProcess && FindNodeByToc(tocToProcess)) 
				{
					IMAPTocHBusy(tocToProcess, false);
					continue;
				}
				
				// set up the structure that the summaries will be returned in.
				node = NewDeliveryNode(tocToProcess);
				if (node)
				{								
					// figure out who this mailbox belongs to
					mailboxNode = TOCToMbox((*node)->toc);
					pers = TOCToPers((*node)->toc);
					
					// if this is the inbox, set the flag in the delivery node so filtering gets done later, unless we're told not to filter.
					CurPers = pers;
					if ((mailboxNode == LocateInboxForPers(pers)) && !PrefIsSet(PREF_IMAP_NO_FILTER_INBOX)) 
						(*node)->filter = true;
					else 
						(*node)->filter = false;
					CurPers = oldPers;
					
					// and queue it in the global list of deliveries.  It will be removed in the idle loop or the thread
					QueueDeliveryNode(node);
					
					// resync the mailbox ...
					DoFetchNewMessages(&specToProcess, true, false);
				}	
			}
			else if (task = IMAPMultExpungeTask)
			{
				DoExpungeMailboxLo(tocToProcess, true);
			}
								
			// the toc can be flushed now ...
			IMAPTocHBusy(tocToProcess, false);	
		}
	}
	
	// we're done with this handle
	ZapHandle(mailboxes);
	
	return (result);
}

/**********************************************************************
 *	DoFetchNewMessages - fetch the new messages for a given mailbox.
 *		Assume CurPers is the personality the mailbox belongs to.
 *
 *	Builds a DeliveryNode and sticks it in the list of deliveries.
 *	the deliveryNode is filled with:
 *
 *	fills toAdd with summaries to be added to the toc
 *	fills toUpdate with summaries containing UID and new flags
 * 	fills toDelete with UID only, or -1 if delete all.
 *
 *	Return pointer to the mailboxNode that we just modified,
 *	nil if we fail.
 **********************************************************************/
MailboxNodeHandle DoFetchNewMessages(FSSpecPtr mailboxSpec, Boolean fetchFlags, Boolean isAutoCheck)
{
	OSErr err = noErr;
	IMAPStreamPtr imapStream = nil;
	UIDNodeHandle localList = nil, remoteList = nil, updateList = nil;
	long noMessages = 0;
	UIDVALIDITY	newUIDValidity = 0;
	MailboxNodeHandle mailboxInfo = nil;
	PersHandle pers = nil;
	PersHandle oldPers = CurPers;
	Boolean redownloadAll = false;
	unsigned long localUIDHighest;
	Boolean result = false;
	TOCHandle tocH = nil;
	Str255 s;
	Str63 m;
	DeliveryNodeHandle delivery = nil;
	SignedByte dState;
	
	// find the toc for this mailbox, open it if we must.
	tocH = TOCBySpec(mailboxSpec);
	if (!tocH) return (nil);
		
	// find the right deliveryNode
	delivery = FindNodeByToc(tocH);
	if (delivery && !(*delivery)->aborted)
	{	
		// find the MailboxNode the spec describes					
		mailboxInfo = TOCToMbox(tocH);
		pers = TOCToPers(tocH);
						
		if (pers && mailboxInfo)
		{
			// this is used later to update the mailbox information
			(*delivery)->mailbox = mailboxInfo;
			
			// make sure this is an IMAP mailbox, and that it can be SELECTed.
			if (((*mailboxInfo)->attributes & LATT_NOT_IMAP) || ((*mailboxInfo)->attributes & LATT_NOSELECT)) 
			{
				AbortDeliveryNode(delivery);
				return (nil);
			}
			
			CurPers = pers;
			
			// display some progress indicating which mailbox we're synching
			LockMailboxNodeHandle(mailboxInfo);
			PathToMailboxName((*mailboxInfo)->mailboxName, m, (*mailboxInfo)->delimiter);
			UnlockMailboxNodeHandle(mailboxInfo);
			ComposeRString(s,IMAP_RESYNC_MAILBOX,m);
			PROGRESS_START;
			PROGRESS_MESSAGE(kpTitle,s);
			// display a little progress before we check the local cache.  It could take a while on big mailboxes.
			PROGRESS_MESSAGER(kpSubTitle,LOOK_MAIL);
			
			// Those Progress calls yield, dammit.  Make sure the mailbox resync wasn't cancelled by the
			// user while they were being displayed. -jdboyd 5/24/02
			if (!(*delivery)->aborted)
			{
				// build a list containing all the UIDs of the cached messages
				err = SpecToUIDList(tocH, &localList, delivery);
				if (err == noErr)
				{
					// and remember the highest UID we have locally.
					localUIDHighest = GetLocalHighestUid(localList);
									
					// create a new IMAP stream to talk through
					imapStream = GetIMAPConnection(IMAPResyncTask, CAN_PROGRESS);
					if (!imapStream) 
					{
						PROGRESS_END;
						CurPers = oldPers;
						AbortDeliveryNode(delivery);
						return (nil);
					}

					// log into the server and SELECT the mailbox
					PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
					LockMailboxNodeHandle(mailboxInfo);
					if (IMAPOpenMailbox(imapStream, (*mailboxInfo)->mailboxName,false))
					{                     
						// now look for some mail
						PROGRESS_MESSAGER(kpSubTitle,LOOK_MAIL);
						
						// figure out what needs to be downloaded.
						noMessages = GetMessageCount(imapStream);
						newUIDValidity = UIDValidity(imapStream);
						
						// check to make sure we haven't aborted ...
						if (!(*delivery)->aborted)
						{				
							// if the remote mailbox contains no messages, updating the cache is as easy as deleteing everything.
							if (noMessages == 0)
							{
								// tell caller to remove everything from the local cahce
								(*delivery)->td = MSUM_DELETE_ALL;
								err = noErr;

								// Audit the fact that a mailcheck started and finished, but that nothing was fetched.
								AuditCheckStart(++gCheckSessionID,(*CurPersSafe)->persId,isAutoCheck);
								AuditCheckDone(gCheckSessionID,0,0);
							}
							// we have some more complicated synching to do
							else
							{
								// if we've seen this mailbox before and the UIDVALIDITY of the mailbox has changed, 
                                // we'll have to download everything.
                                if (((*mailboxInfo)->uidValidity != 0) && (newUIDValidity != (*mailboxInfo)->uidValidity)) redownloadAll = true;
								
								// if we're redownloading everything, the we must fetch flags, too.
								if (redownloadAll) fetchFlags = true;

								// if fetchFlags is true, download flags for all of the messages	
								if (fetchFlags) result = FetchAllFlags(imapStream, &remoteList);
								else // otherwise, just pay attention to new messages
								{
									unsigned long serverUIDHighest = UIDFetchLastUid(imapStream);
									Str255 seq;
														
									if (serverUIDHighest > localUIDHighest)
									{
										WriteZero(seq, sizeof(seq));
										ComposeString(seq, "\p%lu:*", localUIDHighest + 1);
										result = FetchFlags(imapStream, seq + 1, &remoteList);
									}
								}
								
								if (remoteList && result)
								{
									// if we're redownloading everything, zap our local cache
									if (redownloadAll)
									{
										// local UID list should be empty
										UID_LL_Zap(&localList);
								
										// tell caller to remove everything from the local cache
										(*delivery)->td = MSUM_DELETE_ALL;
									}
									else
									{
										if (!fetchFlags)
										{
											// if we didn't fetch new uid's, then we're not going to change
											// the local cache at all.  Delete the local list so we don't
											// end up deleting anything locally.
											
											UID_LL_Zap(&localList);
											
											// tell caller that nothing is to be removed from the local cache
											(*delivery)->td = nil;
										}
										else
										{
											// Merge the two lists.
											// The three lists will end up in the following states:
											// 1. localList will contain uid's no longer on the server ONLY.
											// 2. remoteList will contain UIDs of messages that need to be fetched
											// 3. updateList will contain new flags for local messages still on the server.
											
											MergeUidLists(&localList, &remoteList, &updateList);
										}
									}
									
									// Update mailbox info.
									(*mailboxInfo)->uidValidity = newUIDValidity;

									// Actually go fetch the new messages.
									UIDFetchMessages(imapStream, mailboxInfo, remoteList, delivery, isAutoCheck);
								}
							 	else
								{
									// an error occurred building the remoteList.
									// abort the resync
									AbortDeliveryNode(delivery);
								}
							}
							
							if (!(*delivery)->aborted)
							{
								dState = HGetState((Handle)delivery);
								LDRef(delivery);
								// create the list of summaries to be updated, destroy the updateList
								if (updateList) (*delivery)->tu = UIDNodeList2Handle(&updateList);
							
								// create the list of summaries to be deleted, destroy the localList
								if (localList) (*delivery)->td = UIDNodeList2Handle(&localList);
								HSetState((Handle)delivery, dState);
								
								// are there summaries to be added?
								if ((*delivery)->ta) 
									IMAPMailboxChanged(mailboxInfo);
							}
						}
						else
						{
							if (!(*delivery)->aborted) err = IMAPError(kIMAPResync, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
							else err = userCanceledErr;	// we aborted.  Don't display an error message.
						}
					}
					else // failed to open the mailbox.
						err = IE(imapStream, kIMAPResync, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
					
					UnlockMailboxNodeHandle(mailboxInfo);
						
					PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
					
					// Zap the remote list.  toAdd was created in UIDFetchMessages.
					UID_LL_Zap(&remoteList);
					
					// close down and clean up
					CleanupConnection(&imapStream);
				}
				else
				{
					// an error occurred reading in the local cache.
					AbortDeliveryNode(delivery);
					if (err != userCanceledErr) IMAPError(kIMAPResync, kIMAPMemErr, errIMAPOutOfMemory);
				}	
			}
			
			// complete or abort the delivery ...
			if ((*delivery)->aborted)
			{
				// remember that the user cancelled.  The delievery node will be removed later.
				err = userCancelled;
			}
			else
			{
				// we're really, truely done now.
				(*delivery)->finished = true;
			}
				
			CurPers = oldPers;
			
			PROGRESS_END;
		}
		else	// dooesn't seem to be an IMAP personality
		{
			err = IMAPError(kIMAPResync, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);
			AbortDeliveryNode(delivery);
		}
	}
		
	return (err==noErr?mailboxInfo:nil);
}
 

/**********************************************************************
 *	SpecToUIDList - given an FSSpec, return a list of all the UIDs in
 *		that mailbox.
 *
 *	Since we're looking at every message in the mailbox anyway, process
 *	 junk if this is an IMAP junk mailbox as well.  That is, assign all
 *	 scoreless messages a score.  They've had ample time to be scored.
 **********************************************************************/
OSErr SpecToUIDList(TOCHandle tocH, UIDNodeHandle *list, DeliveryNodeHandle deliveryNode)
{
	OSErr err = noErr;
	UIDNodeHandle node = nil;
	MailboxNodeHandle mBox;
	Boolean bIsJunkMbox = false;
	short defaultScore;
	short sumNum;
	TOCHandle hidTocH;
	
	// make sure we were passed a toc
	if (tocH == nil) return (nil);
	
	// find the mailbox associated with this TOC
	mBox = TOCToMbox(tocH);
	
	if (mBox) 
	{
		// turn off sorting for this mailbox while we count the local messages
		SetIMAPMailboxNeeds(mBox, kNeedsSortLock, true);
		
		// take care of unscored messages if this is an IMAP junk mailbox
		if ((bIsJunkMbox = IsIMAPJunkMailbox(mBox))==true)
			defaultScore = GetRLong(IMAP_DEFAULT_JUNK_SCORE);
	}
	
	//iterate through the toc
	for (sumNum = 0; (sumNum < (*tocH)->count) && (err==noErr); sumNum++)
	{
		CycleBalls();
		
		// stop if the mailbox went away
		// since the CycleBalls() call was added above, it's been possible to have the mailbox
		// get closed and the TOC flushed out from under us.  If we notice the TOC has vanished,
		// stop. jdboyd 03/22/02
		if ((*deliveryNode)->aborted)
		{
			break;
		}
		
		// assign junk score if this is an unscored message in an IMAP junk mailbox
		if (bIsJunkMbox)
		{
			if ((*tocH)->sums[sumNum].spamBecause == 0)
			{
				(*tocH)->sums[sumNum].spamBecause = JUNK_BECAUSE_IMAP_SUCKS;
				(*tocH)->sums[sumNum].spamScore = defaultScore;
			}
		}
		
		node = NewZH(UIDNode);
		if (node)
		{
			(*node)->uid = (*tocH)->sums[sumNum].uidHash;
						
			// ordered insert this node into the list
			UID_LL_OrderedInsert(list, &node, false);
		}
		else
		{
			err = MemError();
		}
	}
	
	// are the deleted messages hidden?		
	hidTocH = GetHiddenCacheMailbox(mBox, false, false);
	if (hidTocH)
	{
		// then consider the messages in the hidden cache as well.
		IMAPTocHBusy(hidTocH, true);
		for (sumNum = 0; (sumNum < (*hidTocH)->count) && (err==noErr); sumNum++)
		{
			CycleBalls();

			node = NewZH(UIDNode);
			if (node)
			{
				(*node)->uid = (*hidTocH)->sums[sumNum].uidHash;
							
				// ordered insert this node into the list
				UID_LL_OrderedInsert(list, &node, false);
			}
			else
			{
				err = MemError();
			}
		}
		IMAPTocHBusy(hidTocH, false);
	}
	
	// restore sorting for this mailbox
	if (mBox) SetIMAPMailboxNeeds(mBox, kNeedsSortLock, false);
	
	return (err);
}

/**********************************************************************
 *	UID_LL_Zap - destroy a list node by node.
 **********************************************************************/
void UID_LL_Zap(UIDNodeHandle *list)
{
	UIDNodeHandle node;
	
	while (node=*list)
	{
		LL_Remove(*list,node,(UIDNodeHandle));
		ZapHandle(node);
	}
	*list = nil;
}

/**********************************************************************
 *	GetUIDNodeCount - return the number of nodes in a list
 **********************************************************************/
unsigned long GetUIDNodeCount(UIDNodeHandle list)
{
	UIDNodeHandle scan = list;
	unsigned long count = 0;
	
	while (scan)
	{
		count++;
		scan = (*scan)->next;
	}
	
	return (count);
}

/**********************************************************************
 *	GetLocalHighestUid - return the highest UID in the list
 **********************************************************************/
unsigned long GetLocalHighestUid(UIDNodeHandle list)
{
	UIDNodeHandle scan = list;
	
	if (list) LL_Last(list,scan);
	
	return (scan?(*scan)->uid:0);
}

/**********************************************************************
 *	UID_LL_OrderedInsert - insert a UIDNode into a UID list where it
 *		belongs.  This will build an UID list ordered by UID.
 **********************************************************************/
void UID_LL_OrderedInsert(UIDNodeHandle *head, UIDNodeHandle *item, Boolean isNew)
{																																	
	UIDNodeHandle t = *head, p = 0;
																		
	if (head && *head)																														
	{		
		// find the first node with a smaller UID than the one we're inserting
		while (t && (*t)->uid < (**item)->uid)
			t = (*t)->next;
			
		// if this node has the same uid, replace it with this new node.
		if ((t) && ((*t)->uid == (**item)->uid))													
		{																								
			(*t)->l_seen = (**item)->l_seen;
			(*t)->l_deleted = (**item)->l_deleted;
			(*t)->l_flagged = (**item)->l_flagged;
			(*t)->l_answered = (**item)->l_answered;
			(*t)->l_draft = (**item)->l_draft;
			(*t)->l_recent = (**item)->l_recent;			
			(*t)->l_sent = (**item)->l_sent;
													
			ZapHandle(*item);																			
		}	
		// otherwise insert it ahead of this node																							
		else																							
		{																	
			LL_Parent(*head, t, p);
			
			// if this node has a parent, make it point to the new item.
			if (p) (*p)->next = *item;
			// if it doesn't, it's the head.  Make the head point here now.
			else *head = *item;
			
			(**item)->next = t;
		}	
		if (t && *t) (*t)->l_IsNew = isNew;																	
	}																																		
	else																																
		*head = *item;																											
}

/**********************************************************************
 *	MergeUidLists - Merge two UID lists, creating a third.
 *	 - localList will contain uid's no longer on the server ONLY.
 *	 - remoteList will contain UIDs of messages that need to be fetched
 *	 - updateList will contain new flags for local messages still on 
 *		the server
 **********************************************************************/
void MergeUidLists(UIDNodeHandle *localList, UIDNodeHandle *remoteList, UIDNodeHandle *updateList)
{
	UIDNodeHandle localScan = nil;
	UIDNodeHandle remoteScan = nil;
	UIDNodeHandle nextLocal = nil, nextRemote = nil;
	
	// must have been passed three UIDNodeHandles ...
	if (!localList || !remoteList || !updateList) return;
	
	// start with empty update list ...
	if (*updateList!=nil) UID_LL_Zap(updateList);
	
	// look at each node in the local list.
	localScan = *localList;
	while (localScan)
	{
		CycleBalls();
		nextLocal = (*localScan)->next;
				
		// compare this message with each remote message
		remoteScan = *remoteList;
		while (remoteScan && localScan && ((*remoteScan)->uid<=(*localScan)->uid))
		{
			nextRemote = (*remoteScan)->next;
			
			// this message is still on the server
			if ((*remoteScan)->uid==(*localScan)->uid)
			{
				// remove this node from the localList and delete it
				LL_Remove(*localList, localScan, (UIDNodeHandle));
				ZapHandle(localScan);
				localScan = nil;

				// remove this node from the remoteList
				LL_Remove(*remoteList, remoteScan, (UIDNodeHandle));
				
				// add the node from the remoteList to the updateList.
				(*remoteScan)->next = nil;
				UID_LL_OrderedInsert(updateList, &remoteScan, false);
			}
			
			// move on the next remote message
			remoteScan = nextRemote;
		}
		
		// move on to next local message
		localScan = nextLocal;
	}
}

/**********************************************************************
 * UIDFetchMessages - fetch the messages in uidList.
 *	now does this in chunks for better performance	
 **********************************************************************/				
Boolean UIDFetchMessages(IMAPStreamPtr imapStream, MailboxNodeHandle mailboxInfo, UIDNodeHandle uidList, DeliveryNodeHandle delivery, Boolean isAutoCheck)
{
	Boolean result = true;
	unsigned long initialMessageCount, remainingCount;
	UIDNodeHandle node = nil;
	OSErr err = noErr;
	FSSpec spoolSpec;
	long totalReceived;
	long failed = 0;
	Str255 pShortHeaderFields;
	Str255 sequence;
	long i,n, fetchedMessages;
	unsigned long firstUID, lastUID;
	Boolean fetchFullMessages = PrefIsSet(PREF_IMAP_FULL_MESSAGE) || PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS);
	
	// audit the fact an IMAP mailcheck is about to happen ...
	AuditCheckStart(++gCheckSessionID,(*CurPersSafe)->persId,isAutoCheck);
	StartStreamAudit(imapStream->mailStream->transStream, kAuditBytesReceived);

	// these are the headers we'll be fetching ...
	Zero(pShortHeaderFields);
	GetRString(pShortHeaderFields, IMAP_SHORT_HEADER_FIELDS);
			
	// figure out how many messages we're going to be downloading
	remainingCount = initialMessageCount = GetUIDNodeCount(uidList);
	fetchedMessages = 0;
		
	if (initialMessageCount > 0)
	{		
		// cough up some progress information at this point
		PROGRESS_MESSAGER(kpSubTitle,LEFT_TO_TRANSFER);
		PROGRESS_BAR(remainingCount/initialMessageCount,initialMessageCount,nil,nil,nil);
	
		// if we're doing a full download, create the spool file now.
		if ((PrefIsSet(PREF_IMAP_FULL_MESSAGE) || PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS)) && uidList) 
		{
			imapStream->mailStream->refN = OpenMessDestFile(mailboxInfo, (*uidList)->uid, &spoolSpec);
			if (imapStream->mailStream->refN == -1) result = false;
		}
		
		// set up the IMAP stream to do a chunked header fetch
		imapStream->mailStream->fUIDResults = uidList;
		imapStream->mailStream->delivery = (Handle)delivery;
		imapStream->mailStream->chunkHeaders = true;

		// set up the IMAP stream to display a little progress
		imapStream->mailStream->showProgress = !fetchFullMessages;
		imapStream->mailStream->lastProgress = 0;
		imapStream->mailStream->currentTransfer = imapStream->mailStream->totalTransfer = initialMessageCount;
		
		// tell the IMAP stream what mailbox we're operating on
		imapStream->mbox = mailboxInfo;
		
		// now go fetch them
		for (i = 0; i < initialMessageCount; i++)
		{
			// if we were aborted, stop the resync now.
			if ((*delivery)->aborted) 
			{
				result = false;
				break;
			}
			
			// build the range string for the command ...
			if (PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE))
			{
				node = uidList;
				for (n = 0; n < i; n++) node = (*node)->next;
				firstUID = lastUID = (*node)->uid;
				sprintf (sequence,"%lu",(*node)->uid);
			}
			else
				i = GenerateUIDStringFromUIDNodeHandle(uidList, i, &firstUID, &lastUID, &sequence);
			
			if (result=UIDFetchRFC822HeaderFields(imapStream, -1, sequence, pShortHeaderFields+1))
			{
				// download the messages as well, if the user is a few fries short of a happy meal
	 			if (fetchFullMessages) 
	 			{
	 				for (n = firstUID; (n <= lastUID) && (!(*delivery)->aborted) && !CommandPeriod; n++)
					{
						// increment the progress bar now
	 					PROGRESS_BAR(((100 * fetchedMessages++)/initialMessageCount),remainingCount-(n - firstUID),nil,nil,nil);
	 				
	 					imapStream->mailStream->chunkHeaders = false;
	 					result = DoDownloadSingleMessage(imapStream, n, false, PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS));
	 					imapStream->mailStream->chunkHeaders = true;
					}
	 			}
			}
		 	else 
		 	{
		 		// We failed to fetch one message.  Remember to warn the user, but continue on with the download.
				failed++;
		 	}	
			
			if (CommandPeriod) 
			{
				AbortDeliveryNode(delivery);
				IMAPRudeConnectionClose(imapStream);
			}
			
			remainingCount = initialMessageCount - i;
		}	
	}
	// else
		// no messages to download.  We're done.
done:	
	// Audit the number of messages we received with this check.
	totalReceived = initialMessageCount-remainingCount;
	AuditCheckDone(gCheckSessionID,totalReceived,totalReceived ? ReportStreamAudit(imapStream->mailStream->transStream) : 0);
	
	// display error	
	if (failed) IE(imapStream, kIMAPCompleteResync, 0, errIMAPOneDownloadFailed);
	
	// reset these bits of the MAILSTREAM
	imapStream->mailStream->chunkHeaders = false;
	imapStream->mailStream->fUIDResults = imapStream->mailStream->delivery = nil;
	imapStream->mailStream->showProgress = false;
	imapStream->mailStream->lastProgress = imapStream->mailStream->currentTransfer = imapStream->mailStream->totalTransfer = 0;
	imapStream->mbox = NULL;
			
	return (result);
}

/**********************************************************************
 * SaveMinimalHeader - this is called each time a minimal header is
 *	fetched from the IMAP server
 **********************************************************************/	
OSErr SaveMinimalHeader(MAILSTREAM *stream)
{
	OSErr err = noErr;
	UIDNodeHandle node = stream->fUIDResults;
	DeliveryNodeHandle delivery = (DeliveryNodeHandle)(stream->delivery);
	MSumType sum;
	long offset = 0;
	SignedByte dState;
	Str255 buf;
	long ticks;
	
	if (node && delivery)
	{
		// find the node this header belongs to
		while (node)
		{
			if ((*node)->uid == stream->headerUID) break;
			node = (*node)->next;
		}
		
		// found it.  Now parse the headers and stick them into the delivery node
		if (node && (*node)->uid)
		{
			// display a little progress
			if (stream->showProgress)
			{
				if (((ticks=TickCount()) - (stream->lastProgress) > 60) || (stream->currentTransfer)<=1)
				{
					PROGRESS_BAR(100-(((stream->currentTransfer))*100/(stream->totalTransfer)),(stream->currentTransfer),nil,nil,nil);
					stream->lastProgress = ticks;
				}
			}
			
			// clear the summary about to be added
			Zero(sum);
			
			// if this message has been marked as a draft, treat it as an outgoing message
			if ((*node)->l_sent) sum.opts |= OPT_IMAP_SENT;
				
			// fill in the summary with the header information we just got
			ParseHeaderInMemory(&sum,stream->fNetData);
			ZapHandle(stream->fNetData);
			
			// Provide a little info on what we're doing
			if (PrefIsSet(PREF_IMAP_VERBOSE_RESYNC))
			{
				PCopy(buf,sum.from);
				*buf = MIN(*buf,31);
				PCatC(buf,',');
				PCatC(buf,' ');
				PSCat(buf,sum.subj);
				PROGRESS_MESSAGE(kpMessage,buf);
			}
		
			// grab the uid of the message we're downloading
			sum.uidHash = (*node)->uid;

			// if the /seen flag is set, mark this message as read.
			if ((*node)->l_seen) sum.state = READ;

			// if this message has been answered, mark it as replied.
			if ((*node)->l_answered) sum.state = REPLIED;

			// if this message has been marked as deleted, flag it as deleted
			if ((*node)->l_deleted) sum.opts |= OPT_DELETED;
			
			// if this message is flagged, mark it with the flagged label
			if ((*node)->l_flagged) 
			{
				short color = GetRLong(IMAP_FLAGGED_LABEL);
				if (color > 0 && color < 16)
				{
					sum.flags &= ~(FLAG_HUE1|FLAG_HUE2|FLAG_HUE3|FLAG_HUE4);
					sum.flags |= (color<<14);
				}
			}

			// if this message is being put into a mailbox that will be filtered, flag it.
			if ((*delivery)->filter) sum.flags |= FLAG_UNFILTERED;

			// remember the size of the message as well.
			if ((sum.length=(*node)->size)==0) sum.length = 1 K;

			// flag this summary so we know to go download the message later
			sum.offset = imapNeedToDownload;
	
			// remember when this summary arrived ...
			sum.arrivalSeconds = GMTDateTime();
			
			// spam score
			sum.spamScore = -1;
			sum.spamBecause = 0;
			
			// stick the summary into the sum list
			dState = HGetState((Handle)delivery);
			LDRef(delivery);
			if ((*delivery)->ta)
			{
				offset = GetHandleSize((*delivery)->ta);
				SetHandleBig_((*delivery)->ta,offset + sizeof(MSumType));
				if (err=MemError())
				{
					WarnUser(MEM_ERR, err);
					HSetState((Handle)delivery, dState);
					goto done;
				}
			}
			else
			{
				(*delivery)->ta = NuHTempBetter(sizeof(MSumType));
				if (!(*delivery)->ta)
				{
					WarnUser(MEM_ERR, MemError());
					HSetState((Handle)delivery, dState);
					goto done;
				}
			}
			BMD(&sum,*((*delivery)->ta)+offset,sizeof(MSumType));
			HSetState((Handle)delivery, dState);
		}
	}
	
	// we processed one minimal header
	stream->currentTransfer--;
	
done:
	return (err);
}		


									
/**********************************************************************
 *	ParseHeaderInMemory - Parse headers in a memory buffer and fill in 
 *		the summary info into sum.
 *		The headers are in lines separated by "\r\n".
 *
 *		The header buffer may contain any number of header lines. This 
 *		function can be used to parse a minimal set of headers or the 
 *		full set.
 **********************************************************************/
void ParseHeaderInMemory (MSumPtr sum, Handle headersH)
{
	char buf[512];
	char *scan = 0;
	Str255 headerName;
	Str255 toLine;
	Str255 scratch;
	unsigned long secs;
	long origZone;
	long len;	
	Boolean out = (sum->opts&OPT_IMAP_SENT)!=0;
	Boolean foundMultipartMixed = false;
	Str255 boundary;
	char *headers = nil;
	uLong addrHash = kNoMessageId;
	
	// We need a summary to continue
	if (!sum) return;
	
	// find the headers
	if (headersH) headers = LDRef(headersH);
	else GetRString(sum->from,UNKNOWN_SENDER);
	
	// fill in some of the fields
	sum->state = UNREAD;
	sum->tableId = DEFAULT_TABLE;
	sum->persId = sum->popPersId = (*CurPersSafe)->persId;
	if (PrefIsSet(PREF_SHOW_ALL)) sum->flags |= FLAG_SHOW_ALL;
	if(HasUnicode()) sum->flags |= FLAG_UTF8;
	sum->offset = 0;
	sum->length = 0;

	// figre out some header names
	Zero(boundary);
	GetRString(boundary,AttributeStrn+aBoundary);
	
	// now parse the headers headers points to.
	while (headers && *headers)
	{
		// BUG: We must search for headers spanning multiple lines!!!

		buf[0] = '\0';

		scan = strstr(headers, "\r\n");

		// Did we get a line?
		if (scan)
		{
			if (scan == headers)
			{
				// Blank line. This is the end of the header.
				headers = NULL;
				buf[0] = '\0';
				break;
			}
			else
			{
				// Don't overfill buffer.
				len = (scan - headers);

				if ((len + 2) < sizeof (buf))
				{ 
					strncpy (buf, headers, len);
					buf[len] = '\0';
				}

				// Skip over \r\n
				headers = scan + 2;
			}
		}
		else
		{
			// Perhaps a last line not terminated by "\r\n"?
			if (strlen(headers) < sizeof(buf))
				strcpy (buf, headers);

			// No more lines.
			headers = NULL;
		}

		// Did we get anything in our buffer?
		if (*buf)
		{
			// Date:
			if (!sum->seconds && !pstrincmp(buf,GetRString(headerName,HeaderStrn+DATE_HEAD),headerName[0]))
			{
				CopyHeaderLine(toLine,sizeof(toLine),buf);
				if (secs=BeautifyDate(toLine,&origZone)) 
				{
					PtrTimeStamp(sum,secs,origZone);
				}
			}
			// To:
			else if (out && !*(sum->from) && !pstrincmp(buf,GetRString(headerName,HeaderStrn+TO_HEAD),headerName[0]))
			{
				CopyHeaderLine(toLine,sizeof(toLine),buf);
				if(sum->flags & FLAG_UTF8) HeaderToUTF8(toLine);
				else WhackPString(toLine);
				BeautifyFrom(toLine);
				PSCopy(sum->from,toLine);
				if(sum->flags & FLAG_UTF8) TrimUTF8(sum->from);
			}
			// From:
			else if (!out && !*(sum->from) && !pstrincmp(buf,GetRString(headerName,HeaderStrn+FROM_HEAD),headerName[0]))
			{
				CopyHeaderLine(toLine,sizeof(toLine),buf);
				if(sum->flags & FLAG_UTF8) HeaderToUTF8(toLine);
				else WhackPString(toLine);
				BeautifyFrom(toLine);
				PSCopy(sum->from,toLine);
				if(sum->flags & FLAG_UTF8) TrimUTF8(sum->from);
				
				// fromHash
				CopyHeaderLine(toLine,sizeof(toLine),buf);
				ShortAddr(scratch,toLine);
				MyLowerStr(scratch);
				addrHash = Hash(scratch);
				
			}
			// Subject:
			else if (!*(sum->subj) && !pstrincmp(buf, GetRString(headerName,HeaderStrn+SUBJ_HEAD),headerName[0]))
			{	
				CopyHeaderLine(toLine,sizeof(toLine),buf);
				if(sum->flags & FLAG_UTF8) HeaderToUTF8(toLine);
				else WhackPString(toLine);
				TrimWhite(toLine);
				PSCopy(sum->subj,toLine);
				if(sum->flags & FLAG_UTF8) TrimUTF8(sum->subj);
			}
			// Priority:	
			else if (!pstrincmp(buf,GetRString(headerName,HeaderStrn+PRIORITY_HEAD),headerName[0]))
			{
				sum->priority = sum->origPriority = Display2Prior(Atoi(buf+headerName[0]));
			}
			// Content-type:
			else if (!pstrincmp(buf, GetRString(headerName,InterestHeadStrn+hContentType), headerName[0]))
			{	
				// enriched text?
				GetRString(scratch,MIME_RICHTEXT);
				scratch[scratch[0]+1] = nil;
				if (strstr(buf, scratch+1)) 
					sum->flags |= FLAG_RICH;
				
				// html	or x-html
				GetRString(scratch,HTMLTagsStrn+htmlTag);
				if (strstr(buf, scratch+1)) 				
					sum->opts |= OPT_HTML;
					
				// multipart
				GetRString(scratch,MIME_MULTIPART);
				scratch[scratch[0]+1] = nil;
				if (strstr(buf, scratch+1)) 
				{
					// mixed
					GetRString(scratch,MIME_MIXED);
					scratch[scratch[0]+1] = nil;
					if (strstr(buf, scratch+1)) 
					{
						// remember that we saw multipart/mixed
						foundMultipartMixed = true;	
						
						// with a boundary ... assume this message has an attachment
						if (strstr(buf, boundary+1)) sum->flags |= FLAG_HAS_ATT;
					}
				}
			}
			// boundary, on it's own line
			else if (strstr(buf, boundary+1))				
			{
				// if we've come across multipart/mixed already, 
				// and we find a boundary on it's own line, there's probably and attachment
				if (foundMultipartMixed) sum->flags |= FLAG_HAS_ATT;
			}			
		} // End parsing this line.
	} // End While.
	
	// set the fromHash
	sum->fromHash = addrHash;
	
	if (headersH) UL(headersH);
}

// defined in lex822.c.
Boolean Fix1342(UPtr chars,long *len);

/************************************************************************
 * WhackPString - translate RFC 1342 stuff in a pascal string, and 
 *	transliterate.
 ************************************************************************/
Boolean WhackPString(UPtr line)
{
	Boolean result = true;
	long len = line[0];
	
	// translate RFC 1342 stuff
	result = Fix1342(line+1, &len);
	if (result) line[0] = len;

	// Transliterate the string
	if (line[0]) TransLitString(line);
	
	return (result);
}

/************************************************************************
 * pstrincmp - compare two strings, one pascal, one c, ignore case
 ************************************************************************/
int pstrincmp(CStr cs,UPtr ps,short n)
{
	register c1, c2;
	
	++ps;	// skip over length of PString
	
	for (c1= *ps, c2= *cs; n--; c1 = *++ps, c2= *++cs)
	{
		if (c1-c2)
		{
			if (isupper(c1)) c1=tolower(c1);
			if (isupper(c2)) c2=tolower(c2);
			if (c1-c2) return (c1-c2);
		}
	}
	return(0);
}

/************************************************************************
 * UIDNodeList2Handle - given a list of UIDNodes, convert it to
 *	a handle packed with summaries.  Used to create the toDelete and
 *	toUpdate lists, since they only need information from the FetchFlags
 *	call.
 *	Note; the UIDNodeList is deleted.
 ************************************************************************/
Handle UIDNodeList2Handle(UIDNodeHandle *uidList)
{
	UIDNodeHandle scan = *uidList;
	Accumulator a;
	MSumType sum;
	Handle h = nil;
	
	// nothing to convert.
	if (!uidList || !*uidList) return (nil);

	if (AccuInit(&a) == noErr)
	{
		while (scan = *uidList)
		{
			// clear sum
			Zero(sum);
			
			// uid
			sum.uidHash = (*scan)->uid;
			
			// flags
			sum.state = UNREAD;
			if ((*scan)->l_seen) sum.state = READ;					
			if ((*scan)->l_answered) sum.state = REPLIED;
			if ((*scan)->l_deleted) sum.opts |= OPT_DELETED;
			if ((*scan)->l_sent) sum.opts |= OPT_IMAP_SENT;
			
			// if this message is flagged, mark it with the flagged label
			if ((*scan)->l_flagged) 
			{
				short color = GetRLong(IMAP_FLAGGED_LABEL);
				if (color > 0 && color < 16)
				{
					sum.flags &= ~(FLAG_HUE1|FLAG_HUE2|FLAG_HUE3|FLAG_HUE4);
					sum.flags |= (color<<14);
				}
			}
			
			// add this sum to the Accumulator
			AccuAddPtr(&a, &sum, sizeof(MSumType));
			
			// remove this UIDNode and zap it
			LL_Remove(*uidList, scan, (UIDNodeHandle));
			ZapHandle(scan);
		}
		
		AccuTrim(&a);
		h = a.data;
		a.data = nil;
		AccuZap(a);
	}
	return (h);
}

/************************************************************************
 * IMAPDelivery - return true if there are messages waiting to be
 *	delivered into a specified mailbox.
 *
 *	Fill toAdd, toUpdate, and toDelete with summaries to be added.
 *	Set mbox to the MailboxNodeHandle of the mailbox we updated when finished.
 ************************************************************************/
Boolean IMAPDelivery(TOCHandle inToc, Handle *toAdd, Handle *toUpdate, Handle *toDelete, Handle *toCopy, Boolean *filter, IMAPSResultHandle *results, MailboxNodeHandle *mbox, Boolean *checkAttachments)
{
	Boolean messagesWaiting= false;
	DeliveryNodeHandle node = gDeliveryQueue;
	DeliveryNodeHandle next;
	PersHandle oldPers = CurPers;
	SignedByte dState;
		
	if (filter) *filter = false;
	*mbox = nil;
	
	// loop through list of deliveries
	while (node)
	{
		dState = HGetState((Handle)node);
		LDRef(node);
		if (((*node)->aborted == false) && (SameTOC(inToc, (*node)->toc)))
		{
			HSetState((Handle)node, dState);
			// grab the lists of summaries built so far
			*toAdd = (*node)->ta;
			*toUpdate = (*node)->tu;
			*toDelete = (*node)->td;
			*toCopy = (*node)->tc;
			*results = (*node)->results;
			if (checkAttachments) *checkAttachments = (*node)->cleanupAttachments;
			
			// return true if there are messages waiting
			if ((*toAdd) || (*toUpdate) || (*toDelete) || (*toCopy) || (*results)) messagesWaiting = true;
			
			// remember if messages are going to be added to the mailbox.
			if (*toAdd) 
				SetIMAPMailboxNeeds((*node)->mailbox, kNeedsSelect, true);
			
			// set the lists back to nil
			(*node)->ta = (*node)->tu = (*node)->td = (*node)->tc = (*node)->results = nil;
			
			// if we're finished with this mailbox, remove the delivery node.
			if ((*node)->finished)
			{
				// Set the filter flag for foreground filtering
				if (filter) 
					*filter = (*node)->filter;
				
				// Mark the mailbox as needing to be filtered for background filtering
				if ((*node)->filter && (*node)->mailbox && !PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING))
					SetIMAPMailboxNeeds((*node)->mailbox, kNeedsFilter, true);
				
				*mbox = (*node)->mailbox;
				DequeueDeliveryNode(node);
				ZapDeliveryNode(&node);
				messagesWaiting = true;
				
				// set the messages waiting flag in the toc if we have to.
				if (*mbox!=nil)
				{
					CurPers = MailboxNodeToPers(*mbox);
					if (CurPers)
					{
						if (PrefIsSet(PREF_IMAP_FULL_MESSAGE) || PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS)) 
						{
							(*inToc)->imapMessagesWaiting = 1;
						}
					}
					CurPers = oldPers;
				}
				else	// no mailbox.  This was a search.
				{
					if ((*inToc)->virtualTOC) *mbox = SEARCH_WINDOW;
				}
				
				// Moodmail?  Rescan this mailbox.
				(*inToc)->analScanned = false;
			}	
			break;
		}	
		else HSetState((Handle)node, dState);
		
		// clean up the delivery queue of aborted nodes 
		// if we're sure they're not in use by a thread somewere.
		if ((*node)->aborted && !GetNumBackgroundThreads())
		{
			next = (*node)->next;
			DequeueDeliveryNode(node);
			ZapDeliveryNode(&node);
			node = next;
		}
		else
			node = (*node)->next;
	}
	
	return (messagesWaiting);
}

/************************************************************************
 * IMAPAbortResync - the mailbox is closing.  Take care of cleaning up
 *	after we IMAPed all over the place.
 *
 *	Call the from a thread an be very sorry
 ************************************************************************/
void IMAPAbortResync(TOCHandle toc)
{
	DeliveryNodeHandle node = gDeliveryQueue;
	SignedByte state;
	
	// Is there a resync going on this mailbox?
	while (node)
	{
		// it's possible we'll come across a node in the list that's been aborted but not cleaned up.
		// this could happen during quit, if we're aborting multiple resync operations.  Ignore
		// nodes that are already aborted. jdboyd 03/22/02
		if ((*node)->aborted == false)
		{
			state = HGetState((Handle)node);
			LDRef(node);
			if (SameTOC(toc, (*node)->toc))
			{
				HSetState((Handle)node, state);
				// is this mailbox done with the network part of delivery?
				if ((*node)->finished)
				{
					// then remove the delivery information form the list
					DequeueDeliveryNode(node);
					ZapDeliveryNode(&node);
				}
				else
				{
					// if not, mark the node as aborted.  Let the thread take care of cleaning up.
					AbortDeliveryNode(node);
					
					// also, clean up marked for filtering messages
					if ((*node)->filter) ResetFilterFlags(toc);	
				}
				break;	
			}
			else HSetState((Handle)node, state);
		}
		
		node = (*node)->next;
	}
}

/************************************************************************
 *	NewDeliveryNode - set up a new delivery node
 ************************************************************************/
DeliveryNodeHandle NewDeliveryNode(TOCHandle tocH)
{
	DeliveryNodeHandle node = NULL;
	
	if (tocH)
	{
		node = NewZH(DeliveryNode);
		if (node)
		{	
			(*node)->toc = tocH;
			(*node)->mailbox = TOCToMbox(tocH);
		}
	}
	
	return (node);		
}

/************************************************************************
 *	QueueDeliveryNode - queue a delivery node into our global list
 ************************************************************************/
void QueueDeliveryNode(DeliveryNodeHandle node)
{			
	if (node && ((*node)->mailbox || ((*node)->toc && (*(*node)->toc)->virtualTOC)))
	{
		if ((*node)->mailbox)
		{
			// ensure realistic tocRef
			ASSERT((*(*node)->mailbox)->tocRef < 1000);
			
			(*(*node)->mailbox)->tocRef++;
		}
		LL_Queue(gDeliveryQueue, node, (DeliveryNodeHandle));
	}
	else
	{
		// the caller has not properly set up the DeliverNodeHandle.
		ASSERT(0);
	}
}

/************************************************************************
 *	DequeueDeliveryNode - dequeue a delivery node from our global list
 ************************************************************************/
void DequeueDeliveryNode(DeliveryNodeHandle node)
{		
	if (node && ((*node)->mailbox || ((*node)->toc && (*(*node)->toc)->virtualTOC)) || ((*node)->aborted))
	{
		if ((*node)->mailbox)
		{
			// ensure realistic tocRef
			ASSERT((*(*node)->mailbox)->tocRef < 1000);
		
			if ((*(*node)->mailbox)->tocRef) (*(*node)->mailbox)->tocRef--;
		}
	}
	else
	{
		// the DeliveryNodeHandle is no longer properly set up.
		ASSERT(0);
	}
	LL_Remove(gDeliveryQueue, node, (DeliveryNodeHandle));
}

/************************************************************************
 *	AbortDeliveryNode - Given a DeilveryNode, mark it so it gets aborted
 ************************************************************************/
void AbortDeliveryNode(DeliveryNodeHandle node)
{
	if (node)
	{
		(*node)->aborted = true;	// set the aborted flag
		(*node)->toc = nil;			// forget about the mailbox, it may go away
	}
}

/************************************************************************
 *	ZapDeliveryNode - Given a DeilveryNode, zap it and everyone it loves
 ************************************************************************/
void ZapDeliveryNode(DeliveryNodeHandle *node)
{
	ZapHandle((**node)->ta);
	ZapHandle((**node)->tu);
	ZapHandle((**node)->tc);
	if ((**node)->td != MSUM_DELETE_ALL) ZapHandle((**node)->td);
	ZapHandle((**node)->results);
	
	ZapHandle(*node);
}

/************************************************************************
 *	SameTOC - return true if two TOCs describe the same mailbox
 ************************************************************************/
Boolean SameTOC(TOCHandle toc1, TOCHandle toc2)
{
	Boolean result = false;
	FSSpec spec1;
	FSSpec spec2;
	
	if (toc1 && toc2)
	{
		spec1 = (*toc1)->mailbox.spec;
		spec2 = (*toc2)->mailbox.spec;
		result = SameSpec(&spec1, &spec2);
	}
		
	return (result);
}

/************************************************************************
 *	FindNodeByToc - given a toc, find the delivery node handle
 ************************************************************************/
DeliveryNodeHandle FindNodeByToc(TOCHandle toc)
{
	DeliveryNodeHandle node = gDeliveryQueue;
	SignedByte state;
		
	while (node)
	{
		state = HGetState((Handle)node);
		LDRef(node);
		if (SameTOC(toc,(*node)->toc)) 
		{
			HSetState((Handle)node,state);
			break;
		}
		HSetState((Handle)node,state);
		
		node = (*node)->next;
	}
	
	return (node);
}

/************************************************************************
 * UpdatableIMAPState - return true if this is a message state we
 *	can replace with the state of the message on the server.
 *
 *	Updatable Summary states:
 *
 *	- Messages marked as REPLIED should have their state updated.
 *	- Messages marked as READ should be updated to what the server says.
 *	- Messages marked as UNREAD should have their state updated.
 *	
 *	All other messages are storing a state that's not stored on the IMAP
 *	server.  Leave them alone.
 ************************************************************************/
Boolean UpdatableIMAPState(StateEnum state)
{
	Boolean result = false;
	StateEnum index;
	StateEnum updateableStates[] = {READ,UNREAD,REPLIED};
	
	// If the message state is anything not updatable, ignore the state on the server.
	for (index = 0; (!result) && (index < (sizeof (updateableStates) / sizeof (StateEnum))); index++)
		if (updateableStates[index] == state)
			result = true;
	
	return (result);
}

/************************************************************************
 * DoDownloadSingleMessage - given a summary, download the message, 
 *	including, possibly, its attachments.  This does the real work.
 *
 * Note: to fetch the message selectively, the server must return 
 *	boundaries in the bodystructure fetch.  If it doesn't, we're in the
 *	dark as to what the message looks like on the server, and better
 *	just fetch the whole message. JDB 2-8-99
 *
 * attachmentsToo == true, download message in one big chunk.
 ************************************************************************/
Boolean DoDownloadSingleMessage(IMAPStreamPtr imapStream, unsigned long uid, Boolean progress, Boolean attachmentsToo)
{
	Boolean result = false;
	IMAPBODY *body = nil;
	PARAMETER *param;
	long fPos, fPosNew;
	Boolean html = false, rich = false;
	Boolean showProgress = progress && !IMAPFilteringUnderway();
	Boolean foundBoundary = false;
	
	// Must have an open IMAP connection, and a mailbox must be SELECTed.
	if (!IsSelected(imapStream->mailStream)) goto done;
	
	// figure out where our file is at
	if (GetMessDestFilePos(imapStream, &fPos) != noErr) goto done;
	
	// get an idea of what this message looks like ...
	if ((body=UIDFetchStructure(imapStream, uid))!=nil)
	{
		// iterate through the body's parameter, looking for BOUNDARY
		param = body->parameter;
		while (param && !foundBoundary)
		{
			if (param->attribute && strlen(param->attribute) && !striscmp(param->attribute,"BOUNDARY")) foundBoundary = true;
			param = param->next;
		}
	}
	
	// now fetch the message completely if the preference is set, or if no boundaries were found.
	if (attachmentsToo || !foundBoundary || SpecialCaseDownload(body) || PrefIsSet(PREF_IMAP_FETCH_ATTACHMENTS_WITH_MESSAGE))
	{
		// POP mode - grab the whole thing right now.
		if (imapStream->mailStream->showProgress = showProgress) 
			PrepareDownloadProgress(imapStream, GetRfc822Size(imapStream, uid));
	
		result = UIDFetchMessage(imapStream, uid, true);        // fetch the message, but don't mark it as \Seen
		 
		imapStream->mailStream->showProgress = false;
	}
	else
	{
		// get the message selectively
		if (body)
		{	
			// Fetch and save the header
			if (result=UIDFetchHeader(imapStream, uid, true))
			{
				// Grab parts of the body now.
				imapStream->mailStream->showProgress = showProgress;
				if (body->type == TYPEMULTIPART)
				{
					result = DownloadMultipartBodyToSpoolFile(imapStream, uid, body, nil, false);
				}
				else
				{
					// not multipart.  Just download the whole spiel.
					result = DownloadSimpleBodyToSpoolFile(imapStream, uid, body, "1");
				}
				imapStream->mailStream->showProgress = false;
			}
		}
	}
	
	// terminate the current message
	FSWriteP(imapStream->mailStream->refN,"\p\r");
	
	// where's our file now?
	if (GetMessDestFilePos(imapStream, &fPosNew) != noErr) result = false;

done:
	// update the index of messages in this temporary file
	if (result) UpdateIMAPTempFileIndex(imapStream, uid, fPos, fPosNew);
	
	// Cleanup
	if (body) FreeBodyStructure(body);
		
	return (result);
}

/**********************************************************************
 *	UIDDownloadMessage - start a thread that goes and
 *		downloads message with UID uid into the mailbox described
 *		by TOCHandle tocH.
 **********************************************************************/
OSErr UIDDownloadMessage(TOCHandle inToc, unsigned long uid, Boolean forceForeground, Boolean attachmentsToo)
{
	Handle uidH = nil;
	OSErr err = noErr;
	
	uidH = NuHandle(sizeof(unsigned long));
	if (uidH)
	{
		*((unsigned long *)(*uidH)) = uid;
		err = UIDDownloadMessages(inToc, uidH, forceForeground, attachmentsToo);
	}
	else
	{
		WarnUser(err=MemError(),MEM_ERR);
	}
	
	return(err);
}

/**********************************************************************
 *	UIDDownloadMessages - start a thread that goes and downloads all
 *		messages specified in the uids handle
 **********************************************************************/
OSErr UIDDownloadMessages(TOCHandle inToc, Handle uids, Boolean forceForeground, Boolean attachmentsToo)
{
	IMAPTransferRec imapInfo;
	OSErr err = noErr;
	MailboxNodeHandle mailboxNode;
	PersHandle oldPers = CurPers;
	XferFlags flags;
			
	// make sure we where given a tocH
	if (!inToc) return (IMAPParamErr);

	// we must be online
	if (Offline && GoOnline()) return(OFFLINE);
	
	// figure out who this mailbox belongs to
	mailboxNode = TOCToMbox(inToc);
	CurPers = TOCToPers(inToc);
	
	if (mailboxNode && CurPers)
	{
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable() || forceForeground)
		{
			// download the attachment
			err = DoDownloadMessages(inToc, uids, attachmentsToo)?noErr:1;
		}
		else
		{
			// collect password now if we need it.
			err = CheckIMAPSettingsForPers();				
			
			// if we were given a password, set up the thread
			if (err==noErr)
			{
				Zero(flags);
				Zero(imapInfo);
				imapInfo.uids = uids;
				imapInfo.destToc = inToc;
				imapInfo.command = IMAPFetchingTask;
				imapInfo.attachmentsToo = attachmentsToo;
				err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
				
				// and remove any old task errors
				RemoveTaskErrors(IMAPFetchingTask,(*CurPers)->persId);
				
				// forget the keychain password so it's not written to the settings file
				if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
			}
		}
	}
	
	CurPers = oldPers;
		
	return (err);
}

/************************************************************************
 *	DownloadSingleMessage - given a uid, download the message, 
 *		including, possibly, its attachments.
 ************************************************************************/
Boolean DoDownloadMessages(TOCHandle tocH, Handle uids, Boolean attachmentsToo)
{
	Boolean result = false;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers;
	PersHandle pers = nil;
	MailboxNodeHandle mailboxNode = nil;
	Str255 progressMessage, mName;
	FSSpec spoolSpec;
	short numUids;
	long sumNum;
	unsigned long newUIDValidity;
	SignedByte tstate;
	short totalUids;
	Boolean fetchedAtLeastOne = false;
	
	// must have a toc
	if (!tocH) return (false);
	
	// must have a list of uids to download
	if (!uids || !*uids) return (false);
	
	// must have at least one message to download
	if ((numUids = GetHandleSize(uids)/sizeof(unsigned long))==0) return (false);
	
	// see if this is indeed an IMAP mailbox
	mailboxNode = TOCToMbox(tocH);
	pers = TOCToPers(tocH);
	if (mailboxNode && pers)
	{
		CurPers = pers;

		// display some progress indicating which mailbox we're downloading to.
		LockMailboxNodeHandle(mailboxNode);
		PathToMailboxName ((*mailboxNode)->mailboxName, mName, (*mailboxNode)->delimiter);
		UnlockMailboxNodeHandle(mailboxNode);
		PROGRESS_START;
		ComposeRString(progressMessage,IMAP_FETCHING_MESSAGE,mName);
		PROGRESS_MESSAGE(kpTitle,progressMessage);
		PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
			
		// Create a new IMAP stream
		imapStream = GetIMAPConnection(IMAPFetchingTask, CAN_PROGRESS);
		if (imapStream) 
		{	
			// SELECT the mailbox this message resides in on the server	
			LockMailboxNodeHandle(mailboxNode);
			if (IMAPOpenMailbox(imapStream, (*mailboxNode)->mailboxName,false))
			{	
				PROGRESS_MESSAGER(kpSubTitle,LEFT_TO_TRANSFER);
	
				// make sure the uidvalidty of the mailbox hasn't changed.  If it has, then we shouldn't fetch the specified messages before doing a resync.
				newUIDValidity = UIDValidity(imapStream);
				if (newUIDValidity == (*mailboxNode)->uidValidity)
				{			
					// iterate through the list of uids, and fetch each message.
					totalUids = numUids;
					for (numUids = 0; (numUids < totalUids) && !CommandPeriod; numUids++)
					{
						PROGRESS_BAR((100*numUids)/totalUids,totalUids - numUids,nil,nil,nil);				
						if (TOCFindMessByMID(((unsigned long *)(*uids))[numUids], tocH, &sumNum)==noErr)
						{
							// indicate which message we're downloading
							tstate = HGetState((Handle)tocH);
							LDRef(tocH);
							PCopy(progressMessage,(*tocH)->sums[sumNum].from);
							*progressMessage = MIN(*progressMessage,31);
							PCatC(progressMessage,',');
							PCatC(progressMessage,' ');
							PSCat(progressMessage,(*tocH)->sums[sumNum].subj);
							PROGRESS_MESSAGE(kpMessage,progressMessage);
							BYTE_PROGRESS(nil,0,(*tocH)->sums[sumNum].length);	
							HSetState((Handle)tocH, tstate);
						
							// go download the message
							result = false;
							
							// tell the stream where to put it.
							imapStream->mailStream->refN = OpenMessDestFile(mailboxNode, ((unsigned long *)(*uids))[numUids], &spoolSpec);
							
							// get the message
							if (imapStream->mailStream->refN > 0)
							{
								result = DoDownloadSingleMessage(imapStream, ((unsigned long *)(*uids))[numUids],true,attachmentsToo);
								
								// if we failed to get the message, it's probably because the mailbox has changed.
								if (!result) 
								{
									IMAPError(kIMAPFetch, kIMAPMailboxChangedErr, errIMAPMailboxChangedErr);
									
									// Zap the spool file
									FSpDelete(&spoolSpec);
								}
								else fetchedAtLeastOne = true;
								
								//close the file
								MyFSClose(imapStream->mailStream->refN);
								imapStream->mailStream->refN = -1;
							}
						}
					}
				}
				else
					IMAPError(kIMAPFetch, kIMAPMailboxChangedErr, errIMAPMailboxChangedErr);
			}
			else	// failed to open the destination mailbox.
				IE(imapStream,kIMAPFetch, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
			
			UnlockMailboxNodeHandle(mailboxNode);
						
			PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
		
			// close down and clean up.  This closes the destination file, but leaves the connection open
			CleanupConnection(&imapStream);
						
			// if we succeeded, then there's a message waiting now.
			if (fetchedAtLeastOne)
			{
				(*tocH)->imapMessagesWaiting = 1;
				TOCSetDirty(tocH,true);
			}
		}
		
		PROGRESS_END;	
		CurPers = oldPers;
	}
	else
		IMAPError(kIMAPFetch, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);
	
	ZapHandle(uids);
	
	return (result);
}

/************************************************************************
 *	OpenMessDestFile - create and open a temp file for a mailbox.
 *		We'll close the file when the imapStream is zapped.
 ************************************************************************/
short OpenMessDestFile(MailboxNodeHandle mailboxInfo, unsigned long uid, FSSpecPtr spoolSpec)
{
	short ref = -1;
	Str31 tempName;
	OSErr err = noErr;
	FSSpec spool;
	long dirID;
	Str255 ctext;
	long creator;
		
	// figure out the name of the temporary mailbox.  Must be unique per message fetch operation.
	LockMailboxNodeHandle(mailboxInfo);
	IMAPTempFileName((*mailboxInfo)->uidValidity, uid, tempName);
	UnlockMailboxNodeHandle(mailboxInfo);
	
	// create (or open) the spool file in the spool folder.
	if ((err=SubFolderSpec(SPOOL_FOLDER,&spool))==noErr)
	{
		// locate the spool file
		dirID = SpecDirId(&spool);         
		err = FSMakeFSSpec(spool.vRefNum,spool.parID,tempName,spoolSpec);
		if (err == fnfErr)
		{
			// not found in spool folder.  Create the file.
			GetPref(ctext,PREF_CREATOR);
			if (*ctext!=4) GetRString(ctext,TEXT_CREATOR);
			BMD(ctext+1,&creator,4);
			err = HCreate(spoolSpec->vRefNum,spoolSpec->parID,spoolSpec->name,creator,'TEXT');
		}
		
		// open the file
		if (err == noErr)
		{
			if ((err=AFSHOpen(spoolSpec->name,spoolSpec->vRefNum,spoolSpec->parID,&ref,fsRdWrPerm))==noErr)
			{
				// kill any existing resources
				FSpKillRFork(spoolSpec);
				
				// Start writing at the beginning of the file
				if ((err=SetEOF(ref,0))==noErr)
					err = SetFPos(ref, fsFromStart, 0);
			}
		}
	}
	
	if ((err != noErr) || (ref == -1))
	{
		WarnUser(NO_TEMP_FILE, err);
		ref = -1;
	}
	
	return (ref);
}

/************************************************************************
 *	GetMessDestFilePos - get file position of destination file
 ************************************************************************/
OSErr GetMessDestFilePos(IMAPStreamPtr imapStream, long *pos)
{
	OSErr err = GetFPos(imapStream->mailStream->refN, pos);
	
	return (err);
}

/************************************************************************
 *	IMAPTempFileName - build the name of the temporary mailbox for
 *		the imap message fetch operation.  The name will be
 *		<mailbox uidvalidity><uid of first message>.tmp
 ************************************************************************/
void IMAPTempFileName(UIDVALIDITY uv, unsigned long uid, Str31 fileName)
{
	Str31 tempSuffix;
	Str31 uvString;
	Str31 uidString;
	
	NumToString(uv,uvString);
	NumToString(uid,uidString);
	GetRString(tempSuffix,TEMP_SUFFIX);
	
	ComposeRString(fileName, IMAP_TEMP_MAILBOX_FORMAT, uvString, uidString, tempSuffix);
}

/***************************************************************************
 * UpdateIMAPTempFileIndex - we keep track of the offset and length of 
 *	each message in a temp IMAP file in the 'IIND' resource.
 ***************************************************************************/
OSErr UpdateIMAPTempFileIndex(IMAPStreamPtr imapStream, unsigned long uid, long oldFilePosition, long newFilePosition)
{
	OSErr err = noErr;
	FSSpec tempSpec;
	long count = 0;
	long oldSize = 0;
	short oldResFile = CurResFile();
	Handle resource = 0;
	short resRef = -1;
	Boolean newRes = false;
	
	// figure out where the temporary file is
	if ((err=GetFileByRef(imapStream->mailStream->refN, &tempSpec))!=noErr) return (err);
	
	// create a resource fork if we have to
	if ((err=MakeResFile(tempSpec.name, tempSpec.vRefNum, tempSpec.parID, CREATOR, 'TEXT'))==noErr)
	{
		resRef = FSpOpenResFile(&tempSpec, fsRdWrPerm);
		if ((resRef > 0) && (err=ResError())==noErr)
		{
			UseResFile(resRef);
		
			// Get the existing index resource
			resource = Get1Resource(INDEX_RES_TYPE,INDEX_RES_ID);
			if (ResError()!=noErr)
			{
				ZapHandle(resource);
			}
			
			// add the latest uid, offset, and length to the index handle.
			if (resource)
			{
				oldSize = GetHandleSize(resource);
				SetHandleSize(resource, oldSize + sizeof(IndexStruct));
				err = ResError();
			}
			else
			{
				resource = NuHandle(sizeof(IndexStruct));
				err = ResError();
				newRes = true;
			}
			
			if ((err==noErr) && resource)
			{
				// where does the new indexStruct end up?
				count = (oldSize/sizeof(IndexStruct));
				
				// add it to the resource handle.
				((IndexStructPtr)(*resource))[count].uid = uid;
				((IndexStructPtr)(*resource))[count].offset = oldFilePosition;
				((IndexStructPtr)(*resource))[count].length = newFilePosition-oldFilePosition;
							
				// put the resource back into the file
				if (resource)
				{
					if (newRes) AddResource(resource,INDEX_RES_TYPE,INDEX_RES_ID,"");
					else ChangedResource(resource);
					if ((err=ResError())==noErr)
					{
						MyUpdateResFile(resRef);
						err = ResError();
					}
				}
			}
		}
		
		if (err != noErr)
		{
			// error saving to temporary file
			WarnUser(NO_TEMP_FILE, err);
		}
		CloseResFile(resRef);
	}
	
	UseResFile(oldResFile);
	return(err);
}

/************************************************************************
 *	IMAPMessagesWaiting - fill in spoolSpec with the oldest waiting
 *		message for the mailbox described by tocH.  Return false if
 *		there's nothing to deliver.
 ************************************************************************/
Boolean IMAPMessagesWaiting(TOCHandle tocH, FSSpecPtr spoolSpec)
{
	Boolean foundOne = false;
	OSErr err = noErr;
	MailboxNodeHandle mailboxInfo = nil;
	CInfoPBRec hfi;
	FSSpec spoolFolder;
	Str255 name;
	
	// don't do anything if we know there isn't anything in the spool folder.
	if ((*tocH)->imapMessagesWaiting == 0) return (false);
	
	// see if this is indeed an IMAP mailbox
	mailboxInfo = TOCToMbox(tocH);
	
	spoolSpec->name[0] = 0;
	
	if (mailboxInfo != nil)
	{
		// Scan through Spool Folder, return first file that belongs to this box.
		// Note, it just so happens that the "first" file will be the oldest file.
		
		// Find the Spool Folder
		if ((err=SubFolderSpec(SPOOL_FOLDER,&spoolFolder))==noErr)
		{
			// iterate through it
			hfi.hFileInfo.ioNamePtr = name;
			hfi.hFileInfo.ioFDirIndex = 0;
			while (!foundOne && !DirIterate(spoolFolder.vRefNum,spoolFolder.parID,&hfi))
			{
				// only interested in files
				if (!(hfi.hFileInfo.ioFlAttrib & ioDirMask))	
				{			
					// check the name of this file. Maybe it's a spool file for this mailbox.
					if (IsSpooledMessageFile(name, mailboxInfo)) 
					{		
						short ref = -1;
										
						SimpleMakeFSSpec(spoolFolder.vRefNum,spoolFolder.parID,name,spoolSpec);
						
						
						// is the file open?  That is, is it still being written?
						if ((err=AFSHOpen(spoolSpec->name,spoolSpec->vRefNum,spoolSpec->parID,&ref,fsRdWrPerm))==noErr)
						{
							MyFSClose(ref);
							
							// has this temp file already been processed?
							if (HasBeenProcessed(spoolSpec))
							{
								// delete it
								if (!FSpDelete(spoolSpec))
									hfi.hFileInfo.ioFDirIndex--;
								
								continue;	
							}
							else
							{
								// the file is ready to go.  We found one.
								foundOne = true;
								break;
							}
						}
						// else
							// the file is opened for writing.  The message is still being fetched, so leave it alone for now.
					}
				}
			}	
		}
	 	else // spool folder not found.  No waiting messages.
	 	{
	 		(*tocH)->imapMessagesWaiting = 0;
	 	}	
	}
	
	// if there isn't anything in the spool folder, don't check again.
	if (!foundOne && (spoolSpec->name[0]==0)) (*tocH)->imapMessagesWaiting = 0;
	
	return (foundOne);
}

/************************************************************************
 *	IsSpooledMessageFile - return true if name is the name of a spool
 *		file waiting to be spooled into mailbox node.
 ************************************************************************/
Boolean IsSpooledMessageFile(Str255 name, MailboxNodeHandle node)
{
	Boolean isSpool = false;
	Str31 uvString;
	Str31 tmp;
	
	LockMailboxNodeHandle(node);
	NumToString((*node)->uidValidity,uvString);
	UnlockMailboxNodeHandle(node);
	uvString[uvString[0]+1] = '.';
	uvString[0] = uvString[0] + 1;
	
	// name must begin with the UIDValidity of the mailbox, uvString.
	if (BeginsWith(name,uvString))
	{
		// name must end with a .tmp suffix
		GetRString(tmp,TEMP_SUFFIX);
		if (EndsWith(name,tmp))
		{
			// then this is a valid spool file.
			isSpool = true;
		}
	}
	
	return (isSpool);
}

/************************************************************************
 *	DownloadSimpleBodyToSpoolFile - download simple body part to the
 *		mailbox file.  If it's text, save it.  Otherwise, break it out.
 ************************************************************************/
Boolean DownloadSimpleBodyToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, IMAPBODY *body, char *section)
{
	Boolean result = false;

	// must have uid, body, and a section
	if (!uid || !body || !section) return (false);

	// Make sure we're connected to an IMAP stream.
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (false);

	// doesn't really matter what we're downloading.  Stick it in the spool file.  Someone else will knwo what to do with it.
	// get the body part ...
	switch (body->type)
	{
		case TYPETEXT:
			// Get body text ...
			result = AppendBodyTextToSpoolFile(imapStream, uid, section, body->size.bytes);
			break;

		case TYPEMESSAGE:
		case TYPEMULTIPART:
			return (false);
			break;

		// For all others, break out to file.
		default:
			result = AppendBodyTextToSpoolFile(imapStream, uid, section, body->size.bytes);
			break;
	}

	return (result);
}

/************************************************************************
 *	AppendBodyTextToMbxFile - Download the contents of the body section 
 *	 as text and append it to the spool file
 ************************************************************************/
Boolean AppendBodyTextToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, char *section, long totalSize)
{
	Boolean result = false;
	
	// must have uid and a section
	if (!uid || !section) return (false);

	// must be connected to an IMAP server, and must have a mailbox selected ...
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (false);

	PrepareDownloadProgress(imapStream, totalSize?totalSize:GetRfc822Size(imapStream,uid));

	// actually go get the body text now.  Try to fetch it in chunks.
	if (totalSize) result = UIDFetchBodyTextInChunks(imapStream, uid, section, true, totalSize);
	else result = UIDFetchBodyText(imapStream, uid, section, true);
	
	return result;
}

/************************************************************************
 *	DownloadMultipartBodyToSpoolFile - Main multipart body dispatcher. 
 * 	Handles Multipart/Alternative, Multipart/Related specially.
 ************************************************************************/
Boolean DownloadMultipartBodyToSpoolFile(IMAPStreamPtr imapStream, unsigned long uid, IMAPBODY *parentBody, char *parentSection, Boolean doingRelated)
{
	Str255 pSection, section;
	Boolean result = false;
	Boolean topLevel = true;
	PART *part = nil;
	IMAPBODY *body = nil;
	Boolean doingMRelated = doingRelated, doingMAlternative = false, doingAppleDouble = false;
	short partNum = 0;
	Str255 pRelated, pAlternative, pAttachment, pAppledouble;
	Str255 attachment;
	Boolean wroteBoundary = false;	// set to true once we know we need a closing boundary
		
	// Make sure we have an imap stream and we've SELECTed a mailbox.
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (false);

	// Must have a body.
	if (!parentBody) return false;

	// The body MUST have a subtype, otherwise treat as plain text.
	if (!parentBody->subtype)
	{
		return (AppendBodyTextToSpoolFile(imapStream, uid, ((parentSection && *parentSection) ? parentSection : "1"), parentBody->size.bytes));		
	}
	
	// Copy parent section string.
	if (parentSection == nil) topLevel = true;
	else if (*parentSection == nil) topLevel = true;
	else topLevel = false;

	// If the subtype is not multipart alternative or related, then we might hafta save an attachment stub
	GetRString(pRelated,MIME_RELATED);
	if (!pstrincmp(parentBody->subtype, pRelated, pRelated[0]))
		doingMRelated = true;
	
	GetRString(pAlternative,MIME_ALTERNATIVE);
	if (!pstrincmp(parentBody->subtype, pAlternative, pAlternative[0]))
		doingMAlternative = true;
	
	// If the subtype is AppleDouble, then we need to skip a part.
	GetRString(pAppledouble,MIME_APPLEDOUBLE);
	if (!pstrincmp(parentBody->subtype, pAppledouble,pAppledouble[0]))
		doingAppleDouble = true;
		
	// Loop through all parts:
	part = parentBody->nested.part;
	while (part)
	{
		// Is this the first part??
		if (topLevel)
		{
			NumToString(++partNum,pSection);	// no dot
			PtoCcpy(section, pSection);
		}
		else
		{
			// Copy parent section first. MUST have a non-NULL parent section if not top level!
			ComposeString(pSection, "\p%s.%d", parentSection, ++partNum);
			PtoCcpy(section, pSection);
		}
		
		body = &(part->body);
				
		//
		// Make a stub file ...
		//
		// 	... if we're doing part of an apple double OR if we're looking at an attachment bigger than the max download size
		//	AND
		// 	if this part is an attachment, and there are additional parameters (which, we hope, will contain the filename)
		//

		GetRString(pAttachment, ATTACH);
		PtoCcpy(attachment,pAttachment);
		
		// if this part is going into a stub file, don't let any of it end up in the spool file.	
		if (!doingMRelated && !doingMAlternative 
			&& (doingAppleDouble?!GonnaGetThisAppleDouble(parentBody):PrefMakeAttachmentStub(body->size.bytes))
			&& ((body->disposition.type && !striscmp(attachment, body->disposition.type) || (!body->disposition.type && doingAppleDouble))
			&& ((body->disposition.parameter) || (body->parameter && body->parameter->attribute))))
		{			
			// if we're doing apple double, skip over the current part, 'cause we're looking at a resource fork.
			if (doingAppleDouble && part->next)
			{
				// new section string
				ComposeString(pSection, "\p%s.%d", parentSection, ++partNum);
				PtoCcpy(section, pSection);
				
				// new part
				part = part->next;
				
				// new body
				body = &(part->body);
			}
			// save the attachment stub to the IMAP attachments folder
			result = SaveAttachmentStub(imapStream, uid, section, body);
		}
		else
		{								
			// write the boundary and content type if we're doing a related part, a non-pAppledouble part, or an appledouble that will end up being downloaded now.
			if (pstrincmp(body->subtype, pAppledouble, pAppledouble[0]) || doingMRelated || GonnaGetThisAppleDouble(body))
			{
				IMAPWriteBoundary(imapStream, parentBody, uid, section, false);
				wroteBoundary = true;
			}
		
			switch (body->type)
			{
				case TYPETEXT:
					result = AppendBodyTextToSpoolFile(imapStream, uid, section, body->size.bytes);	
					break;
					
				case TYPEMULTIPART:					
					result =  DownloadMultipartBodyToSpoolFile(imapStream, uid, body, section, doingMRelated);		
					break;

				default:
					// otherwise, stick the attachment into the spool file.
					result = AppendBodyTextToSpoolFile(imapStream, uid, section, body->size.bytes);
					break;
			}  // switch
		}
		
		// Stop if we fail at any point ...
		if (!result) break; 
	
		// Next part.
		part = part->next;
	}

	// write the outer boundary if needed
	if (wroteBoundary) IMAPWriteBoundary(imapStream, parentBody, uid, nil, true);
								
	return (result);

}

/************************************************************************
 * GonnaGetThisAppleDouble - Given a body, return true if the body
 *	describes an Appledouble part that we're going to be downloading
 *	immediately
 ************************************************************************/
Boolean GonnaGetThisAppleDouble(IMAPBODY *body)
{
	Boolean getIt = false;
	Str255 appledouble;
	PART *part;
	IMAPBODY *b = body;
	unsigned long size = 0;
			
	GetRString(appledouble,MIME_APPLEDOUBLE);
	
	// the body's subtype must be appledouble
	if (b && !pstrincmp(body->subtype, appledouble,appledouble[0]))
	{
		part = body->nested.part;
		
		// add up the sizes of the parts
		while (b && part)
		{
			b = &(part->body);
			if (b) size += b->size.bytes;
			
			part = part->next;
		}
		
		getIt = !PrefMakeAttachmentStub(size);
	}
	
	return (getIt);
}

/************************************************************************
 * IMAPDeleteMessage - Delete a single message from an IMAP server.
 ************************************************************************/
Boolean IMAPDeleteMessage(TOCHandle tocH, unsigned long uid, Boolean nuke, Boolean expunge, Boolean undelete)
{
	Handle uidH = nil;
	Boolean result;
	
	uidH = NuHandle(sizeof(unsigned long));
	if (uidH)
	{
		*((unsigned long *)(*uidH)) = uid;
		result = IMAPDeleteMessages(tocH, uidH, nuke, expunge, undelete, false);	
	}
	else
	{
		WarnUser(MemError(),MEM_ERR);
	}
	
	return(result);
}

/************************************************************************
 *	IMAPDeleteMessages - Delete a series of messages from an IMAP server
 ************************************************************************/
Boolean IMAPDeleteMessages(TOCHandle tocH, Handle uids, Boolean nuke, Boolean expunge, Boolean undelete, Boolean forceForeground)
{
	Boolean result = false;
	XferFlags flags;
	IMAPTransferRec imapInfo;
	MailboxNodeHandle mailboxNode;
	short numUids, sumNum, count;
	OSErr err = noErr;
	MailboxNodeHandle trash = NULL;
	
	// must have a tocH and some messages to delete
	if (!tocH || !uids) return (false);
	
	PushPers(CurPers);
		
	// how many messages are to be deleted?
	numUids  = GetHandleSize(uids)/sizeof(unsigned long);
	
	// figure out who this mailbox belongs to
	mailboxNode = TOCToMbox(tocH);
	CurPers = TOCToPers(tocH);
	
	// Fancy Trash Mode - transfer a copy of the message to the trash mailbox.  
	if (FancyTrashForThisPers(tocH))
	{				
		// first, make sure we can find the trash
		trash = GetIMAPTrashMailbox (CurPers, true, false);
		
		// next, Close all messages to be deleted
		for (count=0;count<numUids;count++)
		{
			for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
			{
				if ((*tocH)->sums[sumNum].uidHash == ((unsigned long *)(*uids))[count])
				{
					if ((*tocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[sumNum].messH)->win));
					break;
				}
			}
		}
	}
	
	// are we deleting?
	if (!undelete)
	{
		// are deleted messages hidden?
		if (!DoesIMAPMailboxNeed(mailboxNode,kShowDeleted))
		{
			// then hide and queue deletions.
			PopPers();
			return (FastIMAPMessageDelete(tocH, uids, FancyTrashForThisPers(tocH)));
		}
	}
	// else
		// perform all other cases immediately.  Like undelete.
	
	// we must be online
	if (Offline && GoOnline()) 
	{
		PopPers();
		return(OFFLINE);
	}
		
	// if we're doing FTM, 
	if (FancyTrashForThisPers(tocH))
	{					
		// make sure there's a trash mailbox somewhere
		if (trash == nil) 
		{
			PopPers();
			return (false);
		}
		
		// delete in the trash mailbox means nuke
		if (trash == mailboxNode) nuke = true;
	}
			
	if (forceForeground || PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable())
	{
		result = DoDeleteMessage(tocH, uids, nuke, expunge, undelete);
		if (result) UpdateIMAPMailbox(tocH);	
	}
	else 
	{		
		// collect password now if we need it.
		err = CheckIMAPSettingsForPers();
							
		// if we were given a password, set up the thread and hide the summaries
		if (err==noErr)
		{	
			Zero(flags);
			Zero(imapInfo);
			
			imapInfo.delToc = tocH;
			imapInfo.uids = uids;
			imapInfo.nuke = nuke;
			imapInfo.expunge = expunge;
			imapInfo.command = undelete?IMAPUndeleteTask:IMAPDeleteTask;
			
			if (SetupXferMailThread(false, false, true, false, flags, &imapInfo) == noErr)
				result = true;
			
			// and remove any old task errors
			RemoveTaskErrors((undelete?IMAPUndeleteTask:IMAPDeleteTask),(*CurPers)->persId);
			
			// forget the keychain password so it's not written to the settings file
			if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
		}
	}
	
	PopPers();
	return (result);
}

/************************************************************************
 * IMAPDeleteMessageDuringFiltering - Delete a single message from an 
 *	IMAP mailbox.  To be used during filtering.  
 *
 *	This routine is filter-friendly:
 *
 *	- copies the message to the trash if FTM is on
 * 	- marks the message as deleted
 *	- flags the mailbox as needing an expunge, if FTM is on
 ************************************************************************/
Boolean IMAPDeleteMessageDuringFiltering(TOCHandle tocH, PersHandle pers, unsigned long uid)
{
	Boolean result = false;
	short sumNum = UIDToSumNum(uid, tocH);
	
	// must have a valid sumNum
	if (sumNum < 0) return (false);
	
	// must have an IMAP tocH
	if (!tocH || !(*tocH)->imapTOC) return (false);
	
	// must have a valid sumNum
	if ((sumNum < 0) || (sumNum > (*tocH)->count)) return (false);
	
	// must know the personality that's currently filtering
	if (!pers) return (false);
	
	// If FTM is off, just mark the message as deleted
	if (!FancyTrashForThisPers(tocH))
	{
		result = IMAPMarkMessageAsDeleted(tocH, uid, false);
	}
	else
	{
		MailboxNodeHandle trashNode = nil;
		TOCHandle trashToc = nil;
		
		// locate the trash mailbox
		trashNode = GetIMAPTrashMailbox(pers, false, false);
		if (trashNode)
		{
			LockMailboxNodeHandle(trashNode);
			trashToc = TOCBySpec(&(*trashNode)->mailboxSpec);
			UnlockMailboxNodeHandle(trashNode);
			
			if (trashToc)
			{
				// copy the message from this mailbox to the trash
				if (IMAPTransferMessage(tocH,trashToc,uid,true,true) == noErr)
				{
					// mark it as deleted.  We'll expunge when filtering is complete
					result = IMAPMarkMessageAsDeleted(tocH, uid, false);
					if (result) FlagForExpunge(tocH);
				}
			}
		}
		// else
			// do nothing.  The trash wasn't found
	}	
	
	return (result);
}

/************************************************************************
 * IMAPDeleteMessageFromSearchWindow - Delete all selected IMAP messages
 *	from the Search Window.
 *
 * This routine is search window-friendly.  It connects to each IMAP 
 *	mailbox once, and deletes all selected messages.
 ************************************************************************/
Boolean IMAPDeleteMessagesFromSearchWindow(TOCHandle tocH)
{
	Boolean result = false;
	TOCHandle realTocH = nil, curTocH = nil;
	Handle uids = nil;
	Handle selectedSearchMessages;
	short searchSumNum, sumNum, realSum;
	long count;
			
	// Must have a tocH, and it must be a search window.
	if (!tocH || !*tocH || !(*tocH)->virtualTOC) return (false);
	
	// keep track of which mmessages we process
	selectedSearchMessages = NuHandleClear(((*tocH)->count)*sizeof(Boolean));
	if (selectedSearchMessages)
	{
		for (searchSumNum = 0; (searchSumNum < (*tocH)->count); searchSumNum++)
			if ((*tocH)->sums[searchSumNum].selected) (*selectedSearchMessages)[searchSumNum] = true;
		
		// go through all messages in the search toc
		for (searchSumNum = 0; (searchSumNum < (*tocH)->count); searchSumNum++)
		{
			// if this message is selected ...
			if ((*selectedSearchMessages)[searchSumNum])
			{	
				if (!(realTocH = GetRealTOC(tocH,searchSumNum,&realSum))) 
				{
					(*selectedSearchMessages)[searchSumNum] = false;	// let's not try to process this one again
					continue;
				}
				
				// process the mailbox this selected message belongs to
				curTocH = realTocH;
				count = 0;
				
				if ((*curTocH)->imapTOC)
				{
					// count number of messages in the search window to be deleted from this mailbox
					for (sumNum = 0; (sumNum < (*tocH)->count); sumNum++)
					{
						if ((*selectedSearchMessages)[sumNum])
						{
							if (!(realTocH = GetRealTOC(tocH,sumNum,&realSum)))
							{
								(*selectedSearchMessages)[sumNum] = false;	// let's not try to process this one again
								continue;
							}
							if (realTocH == curTocH) count++;
						}
					}
						
					// delete them
					uids = NuHandleClear(count*sizeof(unsigned long));
					if (uids)
					{
						LDRef(curTocH);
						for (sumNum = 0; (sumNum < (*tocH)->count); sumNum++)
						{
							if ((*selectedSearchMessages)[sumNum])
							{
								if (!(realTocH = GetRealTOC(tocH,sumNum,&realSum)))
								{
									(*selectedSearchMessages)[sumNum] = false;	// let's not try to process this one again
									continue;
								}
								if (realTocH == curTocH)
								{
									BMD(&((*curTocH)->sums[realSum].uidHash),&((unsigned long *)(*uids))[--count],sizeof(unsigned long));
									(*selectedSearchMessages)[sumNum] = false;
									
									// no matter if the message moved or not, mark it as processed so we don't try to delete it again.
									(*tocH)->sums[sumNum].u.virtualMess.virtualMBIdx = -1;
									InvalSum(tocH, sumNum);
								}
							}
						}
						UL(curTocH);
						
						// and delete them.
						result = IMAPDeleteMessages(curTocH, uids, false, false,false, false);
					}											
				}
				else
				{
					(*selectedSearchMessages)[searchSumNum] = false;
				}
			}
		}
		
		// Cleanup
		ZapHandle(selectedSearchMessages);
	}
		
	return (result);
}

/************************************************************************
 * IMAPTransferMessagesFromSearchWindow - transfer selected messages
 *	from the search window. 
 *
 * This is no straightforward.  Every message could come from a different
 *	mailbox.
 ************************************************************************/
OSErr IMAPTransferMessagesFromSearchWindow(TOCHandle fromTocH, TOCHandle toTocH, Boolean copy)
{
	OSErr err = noErr;
	short sumNum;
	TOCHandle realTocH;
	short realSum;
	short count, i;
	
	// structure to keep
	typedef struct TransferInfoStruct TransferInfoStruct, *TransferInfoPtr, **TransferInfoHandle;
	struct TransferInfoStruct
	{
		TOCHandle fromTocH;
		Accumulator ids;
	};

	short numTransfers;
	TransferInfoStruct transfer;
	Accumulator transfers;
	TransferInfoPtr curT;
	Handle ids, idsToFetch;
	short numIds;
	long id;
	long sum;
	short numToFetch;
	
	//
	//	First group the messages to be transferred into the transInfo struct.
	//
	
	numTransfers = 0;
	err = AccuInit(&transfers);
	
	for (sumNum = 0; (sumNum < (*fromTocH)->count) && (err == noErr); sumNum++)
	{
		if ((*fromTocH)->sums[sumNum].selected)
		{
			if (!(realTocH = GetRealTOC(fromTocH,sumNum,&realSum))) continue;
			{
				// only consider this message if we're going to or from an IMAP mailbox
				if ((*realTocH)->imapTOC || (*toTocH)->imapTOC)
				{
					// see if this TOCH has already been added to the transfers
					for (count = 0; count < numTransfers; count++)
					{
						curT = &((TransferInfoPtr)(*transfers.data))[count];
						if (curT->fromTocH == realTocH) break;
					}
		
					if (!curT || (count >= numTransfers))
					{
						// it hasn't.  Add a new one.
						Zero(transfer);
						transfer.fromTocH = realTocH;
						err = AccuInit(&transfer.ids);
						if (err == noErr)
						{	
							err = AccuAddPtr(&transfers, &transfer, sizeof(TransferInfoStruct));
							curT = &((TransferInfoPtr)(*transfers.data))[numTransfers];
							numTransfers++;
						}
					}

					// add the appropriate ID to the accumulator				
					if (curT && (err == noErr))
					{	
						if ((*realTocH)->imapTOC)
						{
							// if we're transferring an IMAp message, we'll do it by uid
							id = (*realTocH)->sums[realSum].uidHash;
							err = AccuAddPtr(&curT->ids, &id, sizeof(long));
						}
						else
						{
							// this is a pop message.  We'll do it by sum nmber
							err = AccuAddPtr(&curT->ids, &realSum, sizeof(short));
						}
						
						if (!copy && (err == noErr))
						{
							// invaidate the summary in the search window
							(*fromTocH)->sums[sumNum].u.virtualMess.virtualMBIdx = -1;
							InvalSum(fromTocH, sumNum);
						}
					}
				}
			}
		}
	}
	
	// trim the transfers
	AccuTrim(&transfers);
	
	// trim the id handles
	for (count = 0; count < numTransfers; count++)
	{
		curT = &((TransferInfoPtr)(*transfers.data))[count];
		AccuTrim(&curT->ids);
	}
	
	//
	//	Now actually go do the transfers
	//
	
	LDRef(transfers.data);
	for (count = 0; count < numTransfers; count++)
	{
		curT = &((TransferInfoPtr)(*transfers.data))[count];
		realTocH = curT->fromTocH;
		ids = curT->ids.data;
		curT->ids.data = nil;
		
		// these messages are coming out of an IMAP mailbox
		if ((*realTocH)->imapTOC)
		{
			if ((*toTocH)->imapTOC)		
			{
				// and they're going to an IMAP maiblox
				err = IMAPTransferMessages(realTocH, toTocH, ids, copy, false);
			}
			else
			{						
				// transferring to a POP mailbox
							
				// make a handle containing the messages to transfer
				numIds = GetHandleSize(ids)/sizeof(long);
				numToFetch = 0;
				
				for (i=0;i<numIds;i++)
				{
					err = TOCFindMessByMID(((unsigned long *)(*ids))[i], realTocH, &sum);
					if (err == noErr)
						if (!(IMAPMessageBeingDownloaded(realTocH, sum) || IMAPMessageDownloaded(realTocH, sum))) numToFetch++;
				}
				
				if (err == noErr)
				{
					if (numToFetch) idsToFetch = NuHandleClear(numToFetch*sizeof(unsigned long));
					if (idsToFetch || !numToFetch)
					{
						if (numToFetch)
						{
							short idNum = 0;
							
							for (i=0;i<numIds;i++)
							{
								TOCFindMessByMID(((unsigned long *)(*ids))[i], realTocH, &sum);
								if (!(IMAPMessageBeingDownloaded(realTocH, sum) || IMAPMessageDownloaded(realTocH, sum)))
									BMD(&((*realTocH)->sums[sum].uidHash),&((unsigned long *)(*idsToFetch))[idNum++],sizeof(unsigned long));
							}
				
							// copy the messages
							err = UIDDownloadMessages(realTocH, idsToFetch, true, true);
						}
						
						if (err == noErr)
						{				
							// now move the messages to the POP mailbox
							for (i = 0; (err == noErr) && (i < numIds); i++)
							{
								err = TOCFindMessByMID(((unsigned long *)(*ids))[i], realTocH, &sum);
								if (err == noErr) 
								{
									// wait for the downloaded message to show up
									while (!IMAPMessageDownloaded(realTocH,sum) && !CommandPeriod)
									{
										CycleBalls();
										UpdateIMAPMailbox(realTocH);
									}
											
									err = AppendMessage(realTocH,sum,toTocH,copy,false,false);
								}
							}	
							
							// delete the messages if they were successfully transferred
							if (!copy && (err == noErr))
							{
								IMAPDeleteMessages(realTocH, ids, false, false,false, false);						
							}
							else ZapHandle(ids);
						}
					}
					else 
						err = MemError();
				}
			}		
		}
		else
		{
			// these messages are coming out of a POP mailbox
			if ((*toTocH)->imapTOC)
			{		
				// and will be transferred to an IMAP mailbox.
				err = IMAPTransferMessagesToServer(realTocH, toTocH, ids, copy, false);
			}
		}
	}
	UL(transfers.data);
	
	//
	//	Clean up
	//

	for (count = 0; count < numTransfers; count++)
	{
		curT = &((TransferInfoPtr)(*transfers.data))[count];
		AccuZap(curT->ids);
	}
	AccuZap(transfers);
		
	return (err);
}

/************************************************************************
 * IMAPMoveIMAPMessages - move IMAP messages around.  This spawns threads.
 ************************************************************************/
OSErr IMAPMoveIMAPMessages(TOCHandle fromTocH, TOCHandle toTocH, Boolean copy)
{
	OSErr err = noErr;
	Handle uids = nil;
	long c =  CountSelectedMessages(fromTocH);
	short sumNum;
	Boolean IMAPtoIMAP = ((*toTocH)->imapTOC && (*fromTocH)->imapTOC);
	Boolean POPtoIMAP = ((*toTocH)->imapTOC && !(*fromTocH)->imapTOC);
	
	// if we're moving messages from a virtual mailbox, the work has already been done.
	if ((*fromTocH)->virtualTOC) return (noErr);
	 
	// build a list of uids to be transferred
	if (IMAPtoIMAP) uids = NuHandleClear(c*sizeof(unsigned long));
	else if (POPtoIMAP) uids = NuHandleClear(c*sizeof(short));
	
	if (uids)
	{	
		// IMAP to IMAP. Do an IMAP transfer.
		if (IMAPtoIMAP)
		{
			// build a list of uids to transfer
			LDRef(fromTocH);
			for (sumNum=0;sumNum<(*fromTocH)->count && c;sumNum++)
				if ((*fromTocH)->sums[sumNum].selected)
					BMD(&((*fromTocH)->sums[sumNum].uidHash),&((unsigned long *)(*uids))[--c],sizeof(unsigned long));
			UL(fromTocH);
		
			// and send them on their merry way ...
			err = IMAPTransferMessages(fromTocH, toTocH, uids, copy, false);
		}
		// POP to IMAP
		else if (POPtoIMAP)
		{
			// build a list of sumNums to transfer
			LDRef(fromTocH);
			for (sumNum=0;sumNum<(*fromTocH)->count && c;sumNum++)
				if ((*fromTocH)->sums[sumNum].selected)
					BMD(&sumNum,&((short *)(*uids))[--c],sizeof(short));
			UL(fromTocH);				
			
			// and send them to the IMAP server ....
			err = IMAPTransferMessagesToServer(fromTocH, toTocH, uids, copy, false);
		}
	}
	else
	{
		WarnUser(MemError(), MEM_ERR);
	}
	
	return (err);
}

/************************************************************************
 * IMAPMarkMessageAsDeleted - Mark a single message in an imap mailbox
 *	for deletion.  Don't care about FTM. 
 *	NOT THREAD SAFE.
 ************************************************************************/
Boolean IMAPMarkMessageAsDeleted(TOCHandle tocH, unsigned long uid, Boolean undelete)
{
	Boolean result = false;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers;
	MailboxNodeHandle mailboxNode = nil;
	Str255 uidStr;
	
	// see if this is indeed an IMAP mailbox
	mailboxNode = TOCToMbox(tocH);
	CurPers = TOCToPers(tocH);
	if (mailboxNode && CurPers)
	{		
		// Create a new IMAP stream
		if (imapStream = GetIMAPConnection(IMAPDeleteTask, CAN_PROGRESS)) 
		{
			// SELECT the mailbox this message resides in on the server
			LockMailboxNodeHandle(mailboxNode);
			if (IMAPOpenMailbox(imapStream, (*mailboxNode)->mailboxName,false))
			{		
				sprintf (uidStr,"%lu",uid);
				result = undelete ? UIDUnDeleteMessages(imapStream, uidStr) : UIDDeleteMessages(imapStream, uidStr, false);
				if (result)
				{
					short sumNum;
						
					if ((sumNum = UIDToSumNum(uid, tocH)) >= 0)
					{
						if (undelete) MarkSumAsDeleted(tocH, sumNum, false);								
						else 
						{
							MarkSumAsDeleted(tocH, sumNum, true);
							
							// Close the message as well ...
							if ((*tocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[sumNum].messH)->win));
						}
						
						// if fancy trash mode is off, then redraw the summary.
						if (PrefIsSet(PREF_IMAP_NO_FANCY_TRASH)) InvalSum(tocH,sumNum);
					}
				}
			}
			UnlockMailboxNodeHandle(mailboxNode);
			
			CleanupConnection(&imapStream);			
		}
	}

	CurPers = oldPers;
	return (result);
}

/************************************************************************
 *	DoDeleteMessage - Mark a single message in an imap mailbox for
 *	 deletion.  Transfer it to the Trash mailbox if !nuke, and expunge.
 ************************************************************************/
Boolean DoDeleteMessage(TOCHandle tocH, Handle uids, Boolean nuke, Boolean expunge, Boolean undelete)
{
	Boolean result = false;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers;
	MailboxNodeHandle mailboxNode = nil;
	Str255 progressMessage, mName;
	unsigned long numUids = GetHandleSize(uids)/sizeof(unsigned long);
	unsigned long count = numUids;
	MailboxNodeHandle trashNode = nil;	
		
	// Make sure we have something to delete ...
	if (!tocH || !uids) return (false);
	
	// see if this is indeed an IMAP mailbox
	mailboxNode = TOCToMbox(tocH);
	CurPers = TOCToPers(tocH);

	if (mailboxNode && CurPers)
	{
		// Make sure we have a trash mailbox
		if (FancyTrashForThisPers(tocH) && ((trashNode=GetIMAPTrashMailbox (CurPers, false, false))==nil))
		{
			CurPers = oldPers;
			ZapHandle(uids);
		 	return (false);
		}
		
		// display some progress indicating which mailbox we're downloading to.
		LockMailboxNodeHandle(mailboxNode);
		PathToMailboxName ((*mailboxNode)->mailboxName, mName, (*mailboxNode)->delimiter);
		UnlockMailboxNodeHandle(mailboxNode);
		if (undelete) ComposeRString(progressMessage,IMAP_UNDELETING_MESSAGES,mName);
		else ComposeRString(progressMessage,IMAP_DELETING_MESSAGES,mName);
		PROGRESS_START;
		PROGRESS_MESSAGE(kpTitle,progressMessage);
	 		
		// Create a new IMAP stream
		if (imapStream = GetIMAPConnection(IMAPDeleteTask, CAN_PROGRESS)) 
		{
			// SELECT the mailbox this message resides in on the server
			PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
			LockMailboxNodeHandle(mailboxNode);
			if (IMAPOpenMailbox(imapStream, (*mailboxNode)->mailboxName, false))
			{		
				// if this stream is read only, don't even try to delete messages.
				if (IsReadOnly(imapStream->mailStream))
				{
					IMAPError(undelete ? kIMAPUndelete : kIMAPDelete, kIMAPMailboxReadOnly, errIMAPReadOnlyStreamErr);		
				}
				else
				{
					// FTM - copy messages to trash mailbox	
					if (FancyTrashForThisPers(tocH))
					{		
						// copy all the messages to the trash mailbox, marking them as deleted, and expunging	
						if (!IsIMAPTrashMailbox(mailboxNode) && !nuke)
						{
							// now copy the messages to the trash
						 	result = CopyMessages(imapStream, trashNode, uids, false, false);
						 	
						 	// do an FTMExpunge to clean out the mailbox of deleted messages, UNLESS we're in the middle of filtering.
						 	if (result && !IMAPFilteringUnderway()) FTMExpunge(imapStream, tocH);
						}
						
						// if nuking, mark all messages for deletion, and expunge
						if (IsIMAPTrashMailbox(mailboxNode) || nuke)
						{
							// mark the message for deletion.
							result = DoUIDMarkAsDeleted(imapStream, uids, undelete);
							if (result) FTMExpunge(imapStream, tocH);
						}
						
						// update the summaries on screen if all went well ...
						if (result) UpdateLocalSummaries(tocH, uids, true, true, false);
					}
					else
					{
						// go through the lists of sums, marking them each as deleted or undeleted, whichever we were told to do
						result = DoUIDMarkAsDeleted(imapStream, uids, undelete);

						// update the summary on screen if all went well ...
						if (result) UpdateLocalSummaries(tocH, uids, undelete, false, false);
					}
				}
			}
			else	// failed to open mailbox
				IE(imapStream, undelete ? kIMAPUndelete : kIMAPDelete, kIMAPSelectMailboxErr, errIMAPSelectMailbox);		
			
			UnlockMailboxNodeHandle(mailboxNode);
			
			// close down and clean up
			PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
			CleanupConnection(&imapStream);
			
			PROGRESS_END;
		}
				
		CurPers = oldPers;
	}
	else //this wasn't an IMAP mailbox.	
		IMAPError(undelete ? kIMAPUndelete : kIMAPDelete, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);
	
	// if we succeeded, and we're in FTM, and the trash mailbox is open, resync it why don't we.
	if (FancyTrashForThisPers(tocH) && result && trashNode && !nuke)
	{
		TOCHandle trashToc = nil;
		
		LockMailboxNodeHandle(trashNode);
		if (tocH=FindTOC(&((*trashNode)->mailboxSpec)))
		{
			if ((*tocH)->win && IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win))) FetchNewMessages(tocH, true, false, false, false);
		}
		UnlockMailboxNodeHandle(trashNode);
	}

	// kill the list of uids that we were supposed to delete
	ZapHandle(uids);
			
	return (result);
}

/************************************************************************
 * DoUIDMarkAsDeleted -  Delete a bunch of messages quickly, if allowed.
 ************************************************************************/
Boolean DoUIDMarkAsDeleted(IMAPStreamPtr stream, Handle uids, Boolean undelete)
{
	short i, numMessages = GetHandleSize(uids)/sizeof(long);
	Str255 progressMessage, uidStr;
	long ticks, lastProgress = 0;
	long uid;
	Boolean result = true;
	
	// Sort the list of messages to be copied for maximum performance
	if (!PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE)) SortUIDListHandle(uids);
	
	for (i=0; i<numMessages && result; i++)
	{
		// display some progress every second
		if ((((ticks=TickCount())-lastProgress) > 60))
		{
			ComposeRString(progressMessage,IMAP_MARKING_MESSAGES, i+1, numMessages);
			PROGRESS_MESSAGE(kpSubTitle,progressMessage);
			lastProgress = ticks;
		}
		
		// build the range string for the command ...
		if (PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE))
		{
			uid = ((unsigned long *)(*uids))[i];
			sprintf (uidStr,"%lu",uid);
		}
		else
			i = GenerateUIDString(uids, i, &uidStr);
		
		// delete this (bunch of) message(s) ...
		result = undelete ? UIDUnDeleteMessages(stream, uidStr) : UIDDeleteMessages(stream, uidStr, false);
		
		// if we failed to delete messages from the server it's most likely that they've
		// been removed already by another client.  Simply cotninue as if nothing has 
		// gone wrong and the'll be removed them from the cache.
		if (!undelete && !result)
			result = true;
		
		// display an error if we failed.
		if (!result) IMAPError(kIMAPTransfer,  kIMAPUndeleteMessage, errIMAPUndeleteMessage);
	}
	
	return (result);
}

/************************************************************************
 *	IMAPTransfer -  Transfer a single message from one mailbox to
 *	 another, where one or more of the mailboxes are IMAP mailboxes.
 ************************************************************************/
OSErr IMAPTransferMessage(TOCHandle fromTocH, TOCHandle toTocH, unsigned long uid, Boolean copy, Boolean forceForeground)
{
	OSErr err = noErr;
	Handle uidH = nil;
	
	uidH = NuHandle(sizeof(unsigned long));
	if (uidH)
	{
		*((unsigned long *)(*uidH)) = uid;
		err = IMAPTransferMessages(fromTocH, toTocH, uidH, copy, forceForeground);
		// don't zap the handle.  Someone else will.
	}
	else
	{
		WarnUser(MemError(),MEM_ERR);
	}
	
	return (err);
}

/************************************************************************
 *	IMAPTransferMessages -  Transfer a series of messages from one mailbox 
 *	 to another, where the source and/or destination is an IMAP mailbox.
 ************************************************************************/
OSErr IMAPTransferMessages(TOCHandle fromTocH, TOCHandle toTocH, Handle uids, Boolean copy, Boolean forceForeground)
{
	OSErr err = noErr;
	XferFlags flags;
	IMAPTransferRec imapInfo;
	PersHandle oldPers = CurPers;
	PersHandle fromPers=nil, toPers=nil;
	MailboxNodeHandle fromBox=nil, toBox=nil;
	short sumNum, numUids, count;
	
	// must have a source toc
	if (fromTocH == toTocH) return (false);
		
	// must have a destination toc
	if (!fromTocH || !toTocH || !uids) return (false);

	// we must be online, or be in the middle of filtering ...
	if (IMAPFilteringWarnOffline() && Offline && GoOnline()) return(OFFLINE);
		
	// Problem: once a message has been transferred to a new IMAP server, there's no
	// way to keep track of it.  It's assigned a new UID, and IMAP servers do not
	// report this new UID.  So, close all messages before transferring them.
	
	if (!copy)
	{
		numUids = GetHandleSize(uids)/sizeof(unsigned long);
		for (count=0;count<numUids;count++)
		{
			for (sumNum=0;sumNum<(*fromTocH)->count;sumNum++)
			{
				if ((*fromTocH)->sums[sumNum].uidHash == ((unsigned long *)(*uids))[count])
				{
					if ((*fromTocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*fromTocH)->sums[sumNum].messH)->win));
					break;
				}
			}
		}
	}
		
	if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable() || forceForeground)
	{
		err = DoTransferMessages(fromTocH, toTocH, uids, copy);
		if (err==noErr) UpdateIMAPMailbox(fromTocH);
	}
	else 
	{		
		// figure out where the source mailbox is
		fromBox = TOCToMbox(fromTocH);
		fromPers = TOCToPers(fromTocH);

		// if we're doing FTM, and we're not copying ...
		if (!copy && FancyTrashForThisPers(fromTocH))
		{				
			MailboxNodeHandle trash = nil;
			
			// and create the trash mailbox, or make the user select one.
			trash = GetIMAPTrashMailbox(fromPers, true, false);
			
			// make sure there's a trash mailbox somewhere
			if (trash == nil) return (errIMAPNoTrash);
		}
	
		// get the password if we need it	
		if ((CurPers=fromPers) && PrefIsSet(PREF_IS_IMAP))
			err = CheckIMAPSettingsForPers();		
		
		// figure out where the source mailbox is
		toBox = TOCToMbox(toTocH);
		toPers = TOCToPers(toTocH);

		// get the password if we need it	
		if ((CurPers=toPers) && PrefIsSet(PREF_IS_IMAP))
			err = CheckIMAPSettingsForPers();		
						
		// if the user didn't cancel, then set up the thread
		if (err==noErr)
		{			
			Zero(flags);
			Zero(imapInfo);	
			imapInfo.destToc = toTocH;
			imapInfo.sourceToc = fromTocH;
			imapInfo.uids = uids;
			imapInfo.copy = copy;
			imapInfo.command = IMAPTransferTask;
			
			err = SetupXferMailThread(false, false, true, false, flags, &imapInfo);
				
			// and remove any old task errors
			RemoveTaskErrors(IMAPTransferTask,(*fromPers)->persId);
			RemoveTaskErrors(IMAPTransferTask,(*toPers)->persId);
		}
		CurPers = oldPers;
		
		// forget the keychain password so it's not written to the settings file
		if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) 
		{
			Zero((*fromPers)->password);
			Zero((*toPers)->password);
		}
	}
	
	return (err);	
}

/************************************************************************
 *	DoTransferMessages - Transfer a message from a mailbox to another.
 ************************************************************************/
OSErr DoTransferMessages(TOCHandle fromTocH, TOCHandle toTocH, Handle uids, Boolean copy)
{
	OSErr err = noErr;
	PersHandle oldPers = CurPers;
	PersHandle fromPers = nil, toPers = nil;
	MailboxNodeHandle fromBox = nil, toBox = nil;
	Boolean fromIsIMAP = false, toIsIMAP = false;
	IMAPStreamPtr fromStream = nil, toStream = nil;
	Boolean bNoResync = false;
	
	//
	// first figure out what sort of transfer this is
	//
	
	// figure out where the source mailbox is
	fromBox = TOCToMbox(fromTocH);
	fromPers = TOCToPers(fromTocH);
	if ((CurPers=fromPers) && PrefIsSet(PREF_IS_IMAP)) fromIsIMAP = true;
	
	// figure out where the destination mailbox is
	toBox = TOCToMbox(toTocH);
	toPers = TOCToPers(toTocH);
	if ((CurPers=toPers) && PrefIsSet(PREF_IS_IMAP)) toIsIMAP = true;
	
	//
	// now do the transfer
	//
	
	// if we allocated our streams and found that at least one personality is an IMAP personality ...
	if ((err == noErr) && (fromIsIMAP || toIsIMAP))
	{					
		// be progressive		
		PROGRESS_START;
		if (copy) 
		{
			PROGRESS_MESSAGER(kpTitle,IMAP_COPY_MESSAGES);
		}
		else 
		{
			PROGRESS_MESSAGER(kpTitle,IMAP_TRANSFER_MESSAGES);
		}

		// allocate the stream we'll need to talk to this mailbox
		CurPers = fromPers;
		if (fromIsIMAP) fromStream = GetIMAPConnection(IMAPTransferTask, CAN_PROGRESS);
		
		// allocate the stream we'll need to talk to this mailbox, if it belongs to a different personality
		CurPers = toPers;
		if (toIsIMAP && (fromPers!=toPers)) toStream = GetIMAPConnection(IMAPAppendTask, CAN_PROGRESS);
				
		// from and to are IMAP servers ...
		if (fromIsIMAP && toIsIMAP)
		{
			// they're different personalities ...
			if (fromPers != toPers) err = TransferMessageBetweenServers(fromPers, fromStream, fromBox, fromTocH, toPers, toStream, toBox, uids, copy);
			// they're the same server
			else 
			{
				err = TransferMessageOnServer(fromPers, fromStream, fromBox, fromTocH, toBox, uids, copy);
				
				// if UIDPLUS is available, there's no need to resync the destination mailbox when we're done.
				bNoResync = !PrefIsSet(PREF_NO_USE_UIDPLUS) && JunkPrefHasIMAPSupport();
			}
			
			// update the mailboxes on screen
			if (err == noErr)
			{
				// add new summaries to destination mailbox, and remove or mark the originals
				if (!copy) UpdateLocalSummaries(fromTocH, uids, false, FancyTrashForThisPers(fromTocH),FancyTrashForThisPers(fromTocH));
			}
		}	
		PROGRESS_END;
	}
	//else
		// neither the to nor the from is an IMAP personality.  Something went wrong with one of the tocs.  Do nothing.
	
	//Cleanup
	
	CleanupConnection(&fromStream);
	CleanupConnection(&toStream);
	
	// No matter what the outcome is, resync the destination mailbox to reflect any changes we may have made
	if (!bNoResync) 
	{
		if (IMAPFilteringUnderway()) SetIMAPMailboxNeeds(toBox, kNeedsResync, true);
		else
		{
			Boolean oldCommandPeriod = CommandPeriod;
			
			// resync right now if we're not in the process of filtering.
			CommandPeriod = false;
			if ((*toTocH)->win && IsWindowVisible(GetMyWindowWindowPtr((*toTocH)->win))) FetchNewMessages(toTocH, true, true, false, false);
			CommandPeriod = CommandPeriod | oldCommandPeriod;
		}
	}
	
	// did we change an IMAP mailbox?
	if (toIsIMAP && toBox)
		IMAPMailboxChanged(toBox);
		
	CurPers = oldPers;
	ZapHandle(uids);
	
	return (err);
}

/************************************************************************
 *	TransferMessageBetweenServers -  move a message from one mailbox
 *	 on one server to another mailbox on another server
 ************************************************************************/
OSErr TransferMessageBetweenServers(PersHandle fromPers, IMAPStreamPtr fromStream, MailboxNodeHandle fromBox, TOCHandle fromTocH, PersHandle toPers, IMAPStreamPtr toStream, MailboxNodeHandle toBox, Handle uids, Boolean copy)
{
	OSErr err = noErr;
	Boolean result;
	STRING	ms;
	short numSums = GetHandleSize(uids)/sizeof(unsigned long);
	short count;
	long totalSize = 0;
	long seconds = 0;
	STRINGDataStruct msData;
	PersHandle oldPers = CurPers;
	char *flags = nil;
	UIDNodeHandle uidNode = nil;
	Str255 seq;
	unsigned long uid = 0;
	
	// must have a source server and mailbox
	if (!fromStream || !fromBox) return (IMAPParamErr);
	
	// must have a destination server and mailbox
	if (!toStream || !toBox) return (IMAPParamErr);
	
	// must have a list of uids to transfer
	if (!uids || (numSums==0)) return (IMAPParamErr);
	
	// open and select the remote source mailbox
	CurPers = fromPers;
	PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
	LockMailboxNodeHandle(fromBox);
	if ((result=IMAPOpenMailbox(fromStream, (*fromBox)->mailboxName, false))==true)
	{	
		// open and select the destination mailbox
		CurPers = toPers;
		LockMailboxNodeHandle(toBox);
		if ((result=IMAPOpenMailbox(toStream, (*toBox)->mailboxName, false))==true)
		{
			// set up the STRINGDataStruct we'll use to read the source message
			Zero(msData);
			msData.buffer = NuPtr(GetRLong(IMAP_TRANSFER_BUFFER_SIZE) + 4);
			if (msData.buffer)
			{
				msData.bufferSize = GetRLong(IMAP_TRANSFER_BUFFER_SIZE);
				msData.imapStream = fromStream;

				// iterate through the list of messages to be transferred
				for (count = 0; (count < numSums) && result; count++)
				{
					uid = ((unsigned long *)(*uids))[count];
					
					PROGRESS_MESSAGER(kpSubTitle,LEFT_TO_TRANSFER);
					PROGRESS_BAR((100*count)/numSums,numSums - count,nil,nil,nil);

					//set up the STRINGDataStruct for the current transfer
					msData.uid = uid;
					msData.bytesRead = 0;
					
					// Get the total size of the message to be transferred
					if ((totalSize=GetRfc822Size(fromStream, uid)) > 0)
					{
						// Initialize the STRING.
						Zero(ms);
						ms.dtb = &StreamAppendDriver;
						ms.data = (void *)&msData;
						(ms.dtb)->init(&ms, nil, totalSize);
						
						// did we successfully start the read?
						if ((result = msData.result) == true)
						{
							// the destination message will retain the same flags
							sprintf (seq,"%lu",uid);
							if (FetchFlags(fromStream, seq, &uidNode))
							{
								if (uidNode)
								{
									if ((*uidNode)->uid == uid)
									{
										FlagsString(&flags, (*uidNode)->l_seen, (*uidNode)->l_deleted, (*uidNode)->l_flagged, (*uidNode)->l_answered, (*uidNode)->l_draft, (*uidNode)->l_recent, (*uidNode)->l_sent);
									}
									ZapHandle(uidNode);
								}
							}

							// now actually go do the append
							result = IMAPAppendMessage(toStream, flags, seconds, &ms);
							
							// display an error if the copy failed.
							if (!result) IMAPError(kIMAPTransfer, kIMAPCopyErr, errIMAPCopyFailed);
													
							// delete the old message from the source mailbox
							if (result && !copy)
							{
								Str255 uidStr;
															
								sprintf (uidStr,"%lu",uid);
								result = UIDDeleteMessages(fromStream, uidStr, false);
							}
						}
						else
							IMAPError(kIMAPTransfer, kIMAPPartialFetchErr, errIMAPCopyFailed);
					}
				}
				
				// get rid of the flags string
				if (flags) ZapPtr(flags);
				
				// get rid of the transfer buffer
				ZapPtr(msData.buffer);			
			}
			else
			{
				// failed to allocate buffer
				WarnUser(MEM_ERR, err=MemError());
			}
		}
		else // failed to open destination.  Display some error message.
			IE(toStream, kIMAPTransfer, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
				
		UnlockMailboxNodeHandle(toBox);
	}
	else	// failed to open source mailbox
		IE(fromStream, kIMAPTransfer, kIMAPSelectMailboxErr, errIMAPSelectMailbox);

	// if FTM is on, then take care of resident deleted messages and expunge.
	CurPers = fromPers;
	if (!copy && result && FancyTrashForThisPers(fromTocH)) FTMExpunge(fromStream, fromTocH);
				
	UnlockMailboxNodeHandle(fromBox);
	CurPers = oldPers;
	
	return(err==noErr?(result?noErr:1):err);
}

/************************************************************************
 *	TransferMessageOnServer -  move a message from one mailbox
 *	 to another on an IMAP server
 ************************************************************************/
OSErr TransferMessageOnServer(PersHandle fromPers, IMAPStreamPtr fromStream, MailboxNodeHandle fromBox, TOCHandle fromTocH, MailboxNodeHandle toBox, Handle uids, Boolean copy)
{
	Boolean result = false;
	PersHandle oldPers = CurPers;
	
	// must have a destination and source mailbox specified.
	if (!fromBox || !toBox) return (IMAPParamErr);
	
	//must have a list of messages to transfer
	if (!uids || !*uids) return (IMAPParamErr);
	
	// must have a stream to talk to the IMAP server through
	if (!fromStream) return (IMAPParamErr);
	
	//SELECT the mailbox to transfer from
	CurPers = fromPers;
	LockMailboxNodeHandle(fromBox);
	
	// copy the messages from the source to the destination
	PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
	if ((result=IMAPOpenMailbox(fromStream, (*fromBox)->mailboxName, false))==true) 
	{	
		result = CopyMessages(fromStream, toBox, uids, copy, false);
		
		// if FTM is on, then take care of resident deleted messages and expunge.
		if (result && !copy && FancyTrashForThisPers(fromTocH)) FTMExpunge(fromStream, fromTocH);
	}
	else IE(fromStream, kIMAPTransfer, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
		
	UnlockMailboxNodeHandle(fromBox);
	
	//Cleanup
	CurPers = oldPers;
	
	return (result?noErr:1);
}

/************************************************************************
 *	IMAPTransferMessagesToServer - Transfer POP messages to an IMAP mbox.
 ************************************************************************/
OSErr IMAPTransferMessageToServer(TOCHandle tocH, TOCHandle toTocH, short sumNum, Boolean copy, Boolean forceForeground)
{
	Handle sumsH = nil;
	OSErr err = noErr;
	
	sumsH = NuHandle(sizeof(short));
	if (sumsH)
	{
		*((short *)(*sumsH)) = sumNum;
		err = IMAPTransferMessagesToServer(tocH, toTocH, sumsH, copy, forceForeground);
	}
	else
	{
		WarnUser(err=MemError(),MEM_ERR);
	}
	
	return(err);
}

/************************************************************************
 *	IMAPTransferMessagesToServer - Transfer POP messages to an IMAP mbox.
 ************************************************************************/
OSErr IMAPTransferMessagesToServer(TOCHandle fromTocH, TOCHandle toTocH, Handle sumNums, Boolean copy, Boolean forceForeground)
{
	OSErr err = noErr;
	XferFlags flags;
	IMAPTransferRec imapInfo;
	PersHandle oldPers = CurPers;
	PersHandle toPers=nil;
	MailboxNodeHandle fromBox=nil, toBox=nil;
	short sumNum, numSums, count, i;
	IMAPAppendHandle spoolData = nil;
	Boolean progress;
	Str255 progressMessage;
	
	// must have a source toc
	if (fromTocH == toTocH) return (-1);
		
	// must have a destination toc
	if (!fromTocH || !toTocH || !sumNums) return (-1);

	// we must be online, or be in the middle of filtering ...
	if (IMAPFilteringWarnOffline() && Offline && GoOnline()) return(OFFLINE);
	
	// we must have been told to transfer at least one message
	numSums = GetHandleSize(sumNums)/sizeof(short);
	if (numSums <= 0) return (-1);

	//
	// first, figure out where this message is going
	//
	
	toBox = TOCToMbox(toTocH);
	toPers = TOCToPers(toTocH);
	if (!toPers || !toBox) return (-1);
	
	//
	// encode all of the messages to be transferred to the server
	//
	
	// We'll need to remember each spool file:
	
	spoolData = NuHandleClear(numSums * sizeof(IMAPAppendStruct));
	if (((err=MemError())!=noErr) || !spoolData)
		 return (err);
		
	// Keep the tocH around.  We'll let it go below if there's an error setting up,
	// or from DoTransferMessagesToServer when it's called below or from a thread.
	IMAPTocHBusy(toTocH, true);
	
	// Display some proress if this might take a while ...
	progress = (numSums > 5);
	if (progress)
	{
		PROGRESS_START;
		PROGRESS_MESSAGER(kpTitle,IMAP_PREPARE_MESSAGES);
	}
	
	// encode each message.  
	for (count=0;(count<numSums) && (err==noErr);count++)
	{
		TOCHandle realTocH;
		short realSumNum;
		
		if (progress)
		{
			ComposeRString(progressMessage,IMAP_PREPARE_FMT, count + 1, numSums);
			PROGRESS_MESSAGE(kpSubTitle,progressMessage);	
		}
		
		if (!(realTocH=GetRealTOC(fromTocH,((short *)(*sumNums))[count],&realSumNum))) err = 1;
		else
		{
			LDRef(spoolData);
			err = SpoolOnePopMessage(realTocH, realSumNum, &((*spoolData)[count]));
			UL(spoolData);
		}
	}
	
	// if an error occurred, go kill all the spool specs
	if (err != noErr)
	{
		for (i=0;(i<count);i++)
			FSpDelete(&((*spoolData)[i]).spoolSpec);
		
		ZapHandle(spoolData);
		IMAPTocHBusy(toTocH, false);
		return (err);
	}
	
	if (progress) PROGRESS_END;
	
	//
	//	if we're not copying the messages, close them all.  We'll delete them once they've been sent.
	//	
	
	if (!copy)
	{
		for (count=0;count<numSums;count++)
		{
			sumNum = ((short *)(*sumNums))[count];
			if ((*fromTocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*fromTocH)->sums[sumNum].messH)->win));	
		}
	}
	
	//
	//	Now upload them to the server.  This can be done from a thread.
	//
		
	if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable() || forceForeground)
	{
		err = DoTransferMessagesToServer(toTocH, spoolData, copy, true);
	}
	else 
	{		
		// get the password for the destination mailbox, if we need it.
		if ((CurPers=toPers) && PrefIsSet(PREF_IS_IMAP))
			err = CheckIMAPSettingsForPers();				
						
		// set up the thread
		if (err==noErr)
		{			
			Zero(flags);
			Zero(imapInfo);	
			imapInfo.destToc = toTocH;
			imapInfo.appendData = spoolData;
			imapInfo.copy = copy;
			imapInfo.command = IMAPUploadTask;
			
			err = SetupXferMailThread(false, false, true, false, flags, &imapInfo);
				
			// and remove any old task errors
			RemoveTaskErrors(IMAPUploadTask,(*toPers)->persId);
		}
		else
		{
			// if we couldn't set up the transfer, we're done with the tocH.
			IMAPTocHBusy(toTocH, false);
		}
		CurPers = oldPers;
		
		// forget the keychain password so it's not written to the settings file
		if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) 
		{
			Zero((*toPers)->password);
		}
	}
	
	ZapHandle(sumNums);
	return (err);	
}

/************************************************************************
 *	SpoolOnePopMessage -  spool a message to transfer it to an IMAP server
 ************************************************************************/
OSErr SpoolOnePopMessage(TOCHandle fromTocH, short fromSumNum, IMAPAppendPtr spoolData)
{
	OSErr err = noErr;
	MyWindowPtr messWin = nil;
	WindowPtr		messWinWP;
	MessHandle messH = nil;
	Boolean openedMessage = false;
	FSSpecPtr spoolSpec = &(spoolData->spoolSpec);
			
	// get the message from the toc and sumnum
	if (messH = (*fromTocH)->sums[fromSumNum].messH) 
	{
		messWin = (*messH)->win;
		messWinWP = GetMyWindowWindowPtr(messWin);
		UsingWindow(messWinWP);
	}

	// if the message is not open, open it.
	openedMessage = false;
	if (messWin == nil) 
	{
		if (messWin = GetAMessage(fromTocH, fromSumNum, nil, nil, false))
		{
			messWinWP = GetMyWindowWindowPtr(messWin);
			messH = (*fromTocH)->sums[fromSumNum].messH;
			openedMessage = true;	// keep track that we were the ones that opened this message
		}
	}
	
	// providing we've opened the pop message to transfer ...	
	if (messWin)
	{
		// create a new spool file for this message
		if ((err=SubFolderSpec(SPOOL_FOLDER,spoolSpec))==noErr)
		{
			spoolSpec->parID = SpecDirId(spoolSpec);
			EnsureMID(fromTocH,fromSumNum);
			ComposeRString(spoolSpec->name,IMAP_SPOOL_FMT,(*fromTocH)->sums[fromSumNum].msgIdHash);   
			UniqueSpec(spoolSpec,31);
		
			// spool the message into a new file
			err = SpoolMessage(messH, spoolSpec, 0);
			
			// remember the state, flags, and serialNum of the message
			spoolData->fromState = (*fromTocH)->sums[fromSumNum].state;
			spoolData->fromFlags = (*fromTocH)->sums[fromSumNum].flags;
			spoolData->serialNum = (*fromTocH)->sums[fromSumNum].serialNum;
			
			// also remember where this message came from
			spoolData->mailbox = (*fromTocH)->mailbox.spec;
			
			if (err == noErr) if (CommandPeriod) err = userCanceledErr;
		}
		
		// close the window if we need to ...
		if (openedMessage) 
			CloseMyWindow(messWinWP);
		else
			NotUsingWindow(messWinWP);
	}
	else err = -1;
		
	return (err);
}
					
/************************************************************************
 *	DoTransferMessagesToServer -  upload spool files to the IMAP server
 ************************************************************************/
OSErr DoTransferMessagesToServer(TOCHandle toTocH, IMAPAppendHandle spoolData, Boolean copy, Boolean forceForeground)
{
	OSErr err = noErr;
	PersHandle oldPers = CurPers;
	PersHandle toPers = nil;
	MailboxNodeHandle toBox = nil;
	IMAPStreamPtr imapStream = nil;
	Str255 toUser, toHost;
	short numMessages = GetHandleSize_(spoolData)/sizeof(IMAPAppendStruct);
	short count;
	Str255 progressMessage;
	MyWindowPtr messWin = nil;
	MessHandle messH = nil;
	Boolean openedMessage = false;
									
	// must have a destination
	if (!toTocH) 
	{
		err = IMAPParamErr;
		goto done;
	}
	
	// must have a list of POP messages to transfer
	if (!numMessages || !spoolData) 
	{
		err = IMAPParamErr;
		goto done;
	}
	
	// must be online
	if (Offline && GoOnline())
	{
		err = OFFLINE;
		goto done;
	}
		
	// figure out where the destination mailbox is
	toBox = TOCToMbox(toTocH);
	toPers = TOCToPers(toTocH);

	if ((CurPers=toPers) && PrefIsSet(PREF_IS_IMAP)) 
	{
		GetPOPInfo(toUser,toHost);
		
		// be progressive		
		PROGRESS_START;
		PROGRESS_MESSAGER(kpTitle,copy ? IMAP_COPY_MESSAGES : IMAP_TRANSFER_MESSAGES);
			
		// allocate the stream we'll need to talk to this mailbox
		PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);
		imapStream = GetIMAPConnection(IMAPTransferTask, CAN_PROGRESS);
		if (imapStream)
		{					
			// open the destination mailbox
			LockMailboxNodeHandle(toBox);
			if (IMAPOpenMailbox(imapStream, (*toBox)->mailboxName,false))
			{					
				for (count = 0; (count < numMessages) && (err == noErr); count++)
				{	
					// indicate which message we're transferring
					ComposeRString(progressMessage,POP_STATUS_FMT, count + 1, numMessages);
					PROGRESS_MESSAGE(kpSubTitle,progressMessage);		
					
					// Append the message to the destination mailbox
					LDRef(spoolData);
					err = AppendSpoolFile(imapStream, &(*spoolData)[count]);
					UL(spoolData);
					if (err == noErr) if (CommandPeriod) err = userCanceledErr;
					
					// rememeber that the message was successfully transferred
					if (err == noErr) 
					{
						(*spoolData)[count].transferred = true;
						IMAPMailboxChanged(toBox);
					}
					else IMAPError(kIMAPTransfer, err, errIMAPCopyFailed);
				}
			}
			else 
			{
				if (CommandPeriod) err = userCancelled;
				else IE(imapStream, kIMAPTransfer, err=kIMAPSelectMailboxErr, errIMAPSelectMailbox);
			}
			UnlockMailboxNodeHandle(toBox);
							
			// close the connection
			CleanupConnection(&imapStream);		
						
			// wipe out all the spool specs
			LDRef(spoolData);
			for (count = 0; (count < numMessages);count++)
				FSpDelete(&(*spoolData)[count].spoolSpec);
			UL(spoolData);
			
			// remember the POP messages so we can delete them from the main thread ...
			if ((err == noErr) && !copy)
			{
				// wait for the handle listing POP messages to be deleted is not in use
				while (gSpoolDataLocked)
				{
					 while (!CommandPeriod && gSpoolDataLocked)
       				 {
              		 	CycleBalls();
                		if (MyYieldToAnyThread()) break;
        			}
				}
				
				// remember the list of spool specs so we can kill the POP messages later ...
				if (!CommandPeriod)
				{
					if (gSpoolData==nil) gSpoolData = NuHandle(0);
					if (HandPlusHand(spoolData, gSpoolData)==noErr)
					{
					 	spoolData = nil;
					}
					else
					{
						ZapHandle(gSpoolData);
					}
				}	
				
				// if we're in the foreground, remove the mesage from the POP mailbox now.
				if (forceForeground) UpdatePOPMailboxes();
			}
			
			// and resync the destination mailbox
			if ((*toTocH)->imapTOC)
			{
				// No matter what the outcome is, resync the destination mailbox to reflect any changes we may have made
				if (IMAPFilteringUnderway()) SetIMAPMailboxNeeds(toBox, kNeedsResync, true);
				else
				{
					Boolean oldCommandPeriod = CommandPeriod;
					
					// start a resync if we're not in the process of filtering.
					CommandPeriod = false;
					if ((*toTocH)->win && IsWindowVisible(GetMyWindowWindowPtr((*toTocH)->win))) FetchNewMessages(toTocH, true, false, false, false);
					CommandPeriod = CommandPeriod | oldCommandPeriod;
				}
			}									 	
		}
		else
		{
			WarnUser(MEM_ERR, err);
		}
	}

	// Cleanup any leftovers
	CurPers = oldPers;
	
	PROGRESS_END;

	if (spoolData) ZapHandle(spoolData);
done:
	IMAPTocHBusy(toTocH, false);
	return (err);
}

/************************************************************************
 * UpdatePOPMailboxes - update any POP mailboxes that have been 
 *	tormented by IMAP
 ************************************************************************/
void UpdatePOPMailboxes(void)
{
	short numMessages = gSpoolData ? GetHandleSize_(gSpoolData)/sizeof(IMAPAppendStruct) : 0;
	short delSum;
	TOCHandle tocH;
	
	if (gSpoolData && numMessages)
	{
		gSpoolDataLocked = true;	// don't let anybody muck with this
		while (numMessages)
		{
			// was this message successfully transferred?
			if ((*gSpoolData)[numMessages-1].transferred)
			{
				// find the TOC
				tocH = TOCBySpec(&((*gSpoolData)[numMessages-1].mailbox));
				if (tocH)
				{
					// can we find the message in the source toc still?
					delSum = FindSumBySerialNum(tocH,(*gSpoolData)[numMessages-1].serialNum);
					if ((delSum >= 0) && (delSum < (*tocH)->count))
					{												
						//	we don't know where this message is anymore.  Remove it from the Search Window.
						SearchUpdateSum(tocH, delSum, tocH, (*tocH)->sums[delSum].serialNum, false, true);
						
						// delete it
						DeleteMessageLo(tocH, delSum, true); 
					}
				}
			}
			
			//next message
			numMessages--;
		}
		
		ZapHandle(gSpoolData);
		gSpoolData = nil;
		gSpoolDataLocked = false;	// it's open season again
	}
}

/************************************************************************
 *	AppendSpoolFile -  given a spec, append it to the currently
 *	 selected mailbox.
 ************************************************************************/
OSErr AppendSpoolFile(IMAPStreamPtr imapStream, IMAPAppendPtr spoolData)
{
	OSErr err = noErr;
	Boolean result;
	STRING	ms;
	long totalSize = 0;
	long seconds = 0;
	STRINGFileStruct msFile;
	char *flags = nil;
	Boolean sent = spoolData->fromFlags&FLAG_OUT ? true : false;	// set the /Sent flag if this message comes from the Out box.
	
	// set up the STRINGFileStruct we'll use to read the spool file
	Zero(msFile);
	msFile.buffer = NuPtr(GetRLong(IMAP_TRANSFER_BUFFER_SIZE) + 4);
	if (msFile.buffer)
	{
		msFile.bufferSize = GetRLong(IMAP_TRANSFER_BUFFER_SIZE);
		SimpleMakeFSSpec(spoolData->spoolSpec.vRefNum, spoolData->spoolSpec.parID, spoolData->spoolSpec.name, &(msFile.spoolSpec));
		msFile.bytesRead = 0;
			
		// Get the total size of the message to be transferred
		if ((totalSize = FSpFileSize(&(spoolData->spoolSpec))) > 0)
		{
			// Initialize the STRING.
			Zero(ms);
			ms.dtb = &FileAppendDriver;
			ms.data = (void *)&msFile;
			(ms.dtb)->init(&ms, nil, totalSize);
			
			// the destination message will have the same flags as the source
			FlagsString(&flags, (spoolData->fromState != UNREAD), false, false, (spoolData->fromState == REPLIED), false, true, sent);

			// now actually go do the append
			result = IMAPAppendMessage(imapStream, flags, seconds, &ms);
			
			// display an error if the append failed.
			if (!result) IMAPError(kIMAPTransfer, kIMAPCopyErr, err=errIMAPCopyFailed);
		}
		
		// get rid of the flags string
		if (flags) ZapPtr(flags);
		
		// get rid of the transfer buffer
		ZapPtr(msFile.buffer);			
	}
	else
	{
		// failed to allocate buffer
		WarnUser(MEM_ERR, err=MemError());
	}	

	return (err);
}

/************************************************************************
 *	CopyMessages -  copy messages from the SELECTed mailbox to the
 *	 destination mailbox.
 ************************************************************************/
Boolean CopyMessages(IMAPStreamPtr imapStream, MailboxNodeHandle mbox, Handle uids, Boolean copy, Boolean silent)
{
	Boolean result = true;
	Str255 uidStr;
	unsigned long uid;
	short numMessages = GetHandleSize(uids)/sizeof(unsigned long);
	short i, j, first, sumNum;
	MailboxNodeHandle fromMBox;
	TOCHandle fromTocH, hidTocH = NULL;
	long numCopies = 0;
	UIDCopyPtr ucptr;
	DeliveryNodeHandle node;
	Boolean setup = false;
				
	// must be connected, and have a mailbox SELECTed.
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (false);
	
	// Sort the list of messages to be copied for maximum performance
	if (!PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE)) SortUIDListHandle(uids);
	
	// UIDPlus stuff.  
	if (!PrefIsSet(PREF_NO_USE_UIDPLUS))
	{
		// locate the source mailbox.  Maybe this should be stored within the imapStream, Einstein.
		fromMBox =  LocateNodeByMailboxName((*CurPers)->mailboxTree, imapStream->mailboxName);
		if (fromMBox)
		{
			fromTocH = TOCBySpec(&(*fromMBox)->mailboxSpec);	
			if (fromTocH)
			{
				// create a delivery node for the UIDPLUS responses.
				node = NewDeliveryNode(fromTocH);
				
				// set up the node with the proper things, include the original UIDs and enough room for the new UIDs.
				if (node)
				{			
					// Allocate a new UIDPLUS response item in the delivery node
					if ((*node)->tc == NULL)
						(*node)->tc = NuHandleClear(sizeof(UIDCopyStruct));
					else
					{
						// some copies are already pending
						numCopies = GetHandleSize((*node)->tc);
						SetHandleBig_((*node)->tc,(numCopies + 1)*sizeof(UIDCopyStruct));
					}
					if (MemError() != noErr)
						ZapHandle((*node)->tc);
						
					// Store the interesting bits about this copy
					ucptr = &((UIDCopyPtr)(*((*node)->tc)))[numCopies];
					if ((*node)->tc != NULL)
					{
						ucptr->toSpec = (*mbox)->mailboxSpec;
						ucptr->copy = copy;
						
						// store copies of the old summaries.  We need to do this becuase the sums may
						// change between now and when we get around to transferring the local cache.
						ucptr->hOldSums = NewHandleClear(numMessages*sizeof(MSumType));
						if (MemError() == noErr)
						{
							for (i=0; i<numMessages; i++)
							{
								sumNum = UIDToSumNum(((unsigned long *)(*uids))[i], fromTocH);
								if (sumNum != -1)
									CopySum(&((*fromTocH)->sums[sumNum]), &(((MSumPtr)(*(ucptr->hOldSums)))[i]), 0);
								else
								{
									// look in the hidden toc for the original message
									if (hidTocH || ((hidTocH = GetHiddenCacheMailbox(TOCToMbox(fromTocH), false, false))!=NULL)) 
									{
										sumNum = UIDToSumNum(((unsigned long *)(*uids))[i], hidTocH);
										if (sumNum != -1)
											CopySum(&((*hidTocH)->sums[sumNum]), &(((MSumPtr)(*(ucptr->hOldSums)))[i]), 0);
									}
								}
							}
								
							// Allocate enough room for the new ones
							ucptr->hNewUIDs = NewHandleClear(numMessages*sizeof(unsigned long));
							if (MemError() == noErr)
								setup = true;
						}
					}
				}
			}
		}
	}
				
	// iterate through (unordered) list of uids, copying each one to the destination mailbox
	PROGRESS_MESSAGER(kpSubTitle,LEFT_TO_TRANSFER);
	for (i=0; i<numMessages && result; i++)
	{
		PROGRESS_BAR((100*i)/numMessages,numMessages - i,nil,nil,nil);
		
		first = i;
		
		// build the range string for the command ...
		if (PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE))
		{
			uid = ((unsigned long *)(*uids))[i];
			sprintf (uidStr,"%lu",uid);
		}
		else
			i = GenerateUIDString(uids, i, &uidStr);
		
		LockMailboxNodeHandle(mbox);
		result = UIDCopy(imapStream, uidStr, (*mbox)->mailboxName);
		UnlockMailboxNodeHandle(mbox);
		
		// Store the UIDPLUS responses
		// finally, update the cache if we can
		if (!PrefIsSet(PREF_NO_USE_UIDPLUS) && result && imapStream && imapStream->mailStream)
		{
			// Do we have the latest information on the destination mailbox?
			if (setup && (((*mbox)->uidValidity == 0) || (imapStream->mailStream->UIDPLUSuv == (*mbox)->uidValidity)))
			{	
				AccuTrim(&imapStream->mailStream->UIDPLUSResponse);
	
				// move these responses to the delivery node
				for (j = 0; first <= i; first++)
					((unsigned long *)*(ucptr->hNewUIDs))[first] = ((unsigned long *)*(imapStream->mailStream->UIDPLUSResponse.data))[j++];
				
				AccuZap(imapStream->mailStream->UIDPLUSResponse);
			}
			else
			{
				// Either the UIDValidity has changed, or we don't have any responses.
				setup = false;
			}
		}
	
		// display an error if the copy failed.
		if (!result && !silent) 
			IMAPError(kIMAPTransfer, kIMAPCopyErr, errIMAPCopyFailed);
	}
	PROGRESS_BAR(100,0,nil,nil,nil);

	// if the copies succeeded, mark the messages as deleted as well.
	if (result && !copy)
	{
		for (i=0; (i<numMessages) && result; i++)
		{		
			// build the range string for the command ...
			if (PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE)) 
			{
				uid = ((unsigned long *)(*uids))[i];
				sprintf (uidStr,"%lu",uid);
			}
			else
				i = GenerateUIDString(uids, i, &uidStr);
			
			// mark the message for deletion.  Do not expunge here.
			result = UIDDeleteMessages(imapStream, uidStr, false);
		}
	}
	
	// finally, make the UIDPLUS responses available.  Or Zap 'em if something went wrong.
	if (!PrefIsSet(PREF_NO_USE_UIDPLUS))
	{
		if (result && node && setup)
		{
			// we're done!
			QueueDeliveryNode(node);
			(*node)->finished = true;
		}
		else	// something went wrong setting up ...
			ZapDeliveryNode(&node);
	}
	
	return (result);
}

/************************************************************************
 * GenerateUIDString - given some uids, and an index into them, generate
 *	a string in the form of "x:y" (or just "x")
 ************************************************************************/
short GenerateUIDString(Handle uids, short index, Str255 uidStr)
{
	unsigned long uid, lastUid, nextUid;
	short numMessages = GetHandleSize(uids)/sizeof(unsigned long);

	uid = ((unsigned long *)(*uids))[index];
	lastUid = uid;
	
	// Is there a message after this one?
	if (index < (numMessages - 1))
	{ 
		nextUid = ((unsigned long *)(*uids))[index+1];
		lastUid = uid;
		
		// Increment through the list until we find a UID out of sequence
		while (((nextUid - lastUid) == 1) && (index < (numMessages - 1)))
		{
			index++;
			lastUid = nextUid;
			nextUid = ((unsigned long *)(*uids))[index+1];
		}
	}
	
	if (lastUid != uid)
	{
		// There were some messages in a row with sequential UIDs.
		sprintf (uidStr,"%lu:%lu",uid,lastUid);
	}
	else
	{
		// operate on only a single message
		sprintf (uidStr,"%lu",uid);
	}
	
	return (index);
}

/************************************************************************
 * GenerateUIDStringFromUIDNodeHandle - the same, only now for UID
 *	lists.
 ************************************************************************/
short GenerateUIDStringFromUIDNodeHandle(UIDNodeHandle uids, short index, unsigned long *first, unsigned long *last, Str255 uidStr)
{
	unsigned long uid, lastUid, nextUid;
	short numMessages = GetUIDNodeCount(uids);
	UIDNodeHandle node;
	long n;
	
	if (index <= numMessages)
	{
		node = uids;
		for (n = 0; n < index; n++) node = (*node)->next;
		
		uid = (*node)->uid;
		lastUid = uid;
		
		// Is there a message after this one?
		if (index < (numMessages - 1))
		{ 
			node = (*node)->next;
			nextUid = (*node)->uid;
			lastUid = uid;
			
			// Increment through the list until we find a UID out of sequence
			while ((node) && ((nextUid - lastUid) == 1) && (index < (numMessages - 1)))
			{
				index++;
				lastUid = nextUid;
				node = (*node)->next;
				if (node) nextUid = (*node)->uid;
			}
		}
	}
	
	if (lastUid != uid)
	{
		// There were some messages in a row with sequential UIDs.
		sprintf (uidStr,"%lu:%lu",uid,lastUid);
	}
	else
	{
		// operate on only a single message
		sprintf (uidStr,"%lu",uid);
	}
	
	*first = uid;
	*last = lastUid;
	
	return (index);
}

/**********************************************************************
 * UIDListCompare - compare two entries
 **********************************************************************/ 
int UIDListCompare(unsigned long *uid1, unsigned long *uid2)
{
	return (*uid1 - *uid2);
}

/**********************************************************************
 * SwapUID - swap two uid entries.
 **********************************************************************/ 
void SwapUID(unsigned long *uid1, unsigned long *uid2)
{
	unsigned long tempUid;
	
	tempUid = *uid1;
	*uid1 = *uid2;
	*uid2 = tempUid;
}

/************************************************************************
 * SortUIDListHandle - sort a handle packed with UIDs
 ************************************************************************/
void SortUIDListHandle(Handle toSort)
{
	short count = 0;
	SignedByte state;
	unsigned long realBig = REAL_BIG;
	
	state = HGetState(toSort);
	count = GetHandleSize(toSort)/sizeof(unsigned long);
	PtrPlusHand_(&realBig,toSort,sizeof(unsigned long));
	QuickSort((void*)LDRef(toSort),sizeof(unsigned long),0,count-1,(void*)UIDListCompare,(void*)SwapUID);
	SetHandleBig_(toSort,count*sizeof(unsigned long));
	HSetState(toSort, state);
}

/************************************************************************
 *	UpdateLocalSummaries -  remove the summaries from a mailbox window.
 *	 If !expunge, just mark the specified messages as deleted.
 ************************************************************************/
OSErr UpdateLocalSummaries(TOCHandle fromTocH, Handle uids, Boolean undelete, Boolean expunge, Boolean cleanupAttachments)
{
	OSErr err = noErr;
	DeliveryNodeHandle node = nil;
	short numUids = GetHandleSize(uids)/sizeof(unsigned long);
	short sumNum, count;
	MSumType sum;
	SignedByte state;
	
	// mus thave a list of UIDs to work with, and a toc to transfer from.
	if (numUids && fromTocH)
	{
		// set up delete request.
		node = NewDeliveryNode(fromTocH);
		if (node)
		{
			(*node)->filter = false;
			
			// expunge?  Then remove the summaries from the sourcemailbox
			if (expunge)
			{
				state = HGetState((Handle)node);
				LDRef(node);
				(*node)->td = NuHandle(numUids*sizeof(MSumType));
				HSetState((Handle)node, state);
				if ((*node)->td) 
				{
					Zero(sum);
					
					(*node)->cleanupAttachments = cleanupAttachments;
					
					// scan through fromTocH, copying summaries from there to the new tocH
					for (count=0;count<numUids;count++)
					{
						sum.uidHash = ((unsigned long *)(*uids))[count];
						BMD(&sum,&((MSumPtr)(*((*node)->td)))[count],sizeof(MSumType));
					}
					(*node)->finished = true;
					TOCSetDirty(fromTocH,true);
					QueueDeliveryNode(node);	
				}
				else
				{
					IMAPError(kIMAPUpdateLocal, kIMAPMemErr, err=MemError());
					ZapHandle(node);
				}
			}
			// just mark as deleted.
			else
			{
				state = HGetState((Handle)node);
				LDRef(node);
				(*node)->tu = NuHandle(numUids*sizeof(MSumType));
				HSetState((Handle)node, state);
				if ((*node)->tu) 
				{
					Zero(sum);
					
					// scan through fromTocH, copying summaries from there to the new tocH
					for (count=0;count<numUids;count++)
					{
						for (sumNum=0;sumNum<(*fromTocH)->count;sumNum++)
						{
							if ((*fromTocH)->sums[sumNum].uidHash == ((unsigned long *)(*uids))[count])
							{
								sum.uidHash = (*fromTocH)->sums[sumNum].uidHash;
								if (undelete)
								{
									sum.opts = (*fromTocH)->sums[sumNum].opts & ~OPT_DELETED;								
								}
								else
								{
									sum.opts = (*fromTocH)->sums[sumNum].opts | OPT_DELETED;
								}
								sum.state = (*fromTocH)->sums[sumNum].state;
								BMD(&sum,&((MSumPtr)(*((*node)->tu)))[count],sizeof(MSumType));
								sumNum = (*fromTocH)->count;
							}
						}
					}
					(*node)->finished = true;
					TOCSetDirty(fromTocH,true);
					QueueDeliveryNode(node);	
				}
				else
				{
					IMAPError(kIMAPUpdateLocal, kIMAPMemErr, err=MemError());
					ZapHandle(node);
				}
			}	
		}
	 	else
		{
			IMAPError(kIMAPUpdateLocal, kIMAPMemErr, err=MemError());
		}
	}
	return (err);
}


/************************************************************************
 *	Set up a thread to expunge a toc.
 ************************************************************************/
Boolean IMAPRemoveDeletedMessages(TOCHandle tocH)
{
	MailboxNodeHandle mailboxNode = nil;
	Boolean result = false;
	OSErr err = noErr;
	XferFlags flags;
	IMAPTransferRec imapInfo;
	short sum;

	
	// must have a toc
	if (tocH == nil) return (false);

	// we must be online
	if (Offline && GoOnline()) return(OFFLINE);
	
	// are there messages in the mailbox to be deleted?
	if (CountDeletedIMAPMessages(tocH) == 0) return (true);
		
	// first, go close all the open messages in this toc that are to be deleted
	for (sum=0;sum<(*tocH)->count;sum++)
	{
		if (((*tocH)->sums[sum].opts&OPT_DELETED) && ((*tocH)->sums[sum].messH)) 
		{
			CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[sum].messH)->win));
		}
	}
		
	if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable())
	{
		result = DoExpungeMailbox(tocH);
	}
	else 
	{
		PersHandle oldPers = CurPers;
		
		// figure out who this mailbox belongs to
		mailboxNode = TOCToMbox(tocH);
		CurPers = TOCToPers(tocH);
			
		// collect password now if we need it.
		err = CheckIMAPSettingsForPers();
				
		// if we were given a password, set up the thread
		if (err == noErr)
		{
			Zero(flags);
			Zero(imapInfo);
			imapInfo.delToc = tocH;
			imapInfo.command = IMAPExpungeTask;
			err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
			if (err == noErr) result = true;
	
			// remove any old task errors
			RemoveTaskErrors(IMAPExpungeTask,(*CurPers)->persId);
			
			// forget the keychain password so it's not written to the settings file
			if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
		}
		CurPers = oldPers;
	}
	
	return (result);
}

/************************************************************************
 *	DoExpungeMailbox - remove the deleted messages from an mbox
 ************************************************************************/
Boolean DoExpungeMailbox(TOCHandle tocH)
{
	// EXPUNGE the mailbox only if there's messages marked for deleted locally.
	return (DoExpungeMailboxLo(tocH, true));
}

/************************************************************************
 *	DoExpungeMailbox - remove the deleted messages from an mbox
 ************************************************************************/
Boolean DoExpungeMailboxLo(TOCHandle tocH, Boolean bCheckLocal)
{
	Boolean result = false;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers;
	MailboxNodeHandle mailboxNode = nil;
	Str255 progressMessage, mName;
	Handle uids = nil;
	short count, sumNum=0;
	TOCHandle hidTocH;
	
	// must have a tocH
	if (!tocH) return (false);
	
	// and it must belong to an IMAP personality
	mailboxNode = TOCToMbox(tocH);
	CurPers = TOCToPers(tocH);
	
	// might need to look at the hidden toc as well
	hidTocH = GetHiddenCacheMailbox(mailboxNode, false, false);
	
	if (mailboxNode && CurPers)
	{
		//
		// scan through the tocH, seeing which uids are going to be expunged
		//
		
		// how many messages are marked for deletion?
		sumNum = CountDeletedIMAPMessages(tocH);
		
		// are some messages marked to be deleted?  Or do we not care?
		if (sumNum || !bCheckLocal)
		{
			// figure out which ones ...
			uids = NuHandle(sumNum*sizeof(unsigned long));
			if (uids)
			{
				// check out the toc for deleted messages.  We'll remove them once the EXPUNGE completes.
				for (count=0;(count<(*tocH)->count) && (sumNum > 0);count++)
				{
					if ((*tocH)->sums[count].opts&OPT_DELETED)
					{
						((unsigned long *)(*uids))[sumNum-1] = (*tocH)->sums[count].uidHash;
						sumNum--;
					}
				}
				
				// check out the hidden toc as well.
				for (count=0;hidTocH && (count<(*hidTocH)->count) && (sumNum > 0);count++)
				{
					if ((*hidTocH)->sums[count].opts&OPT_DELETED)
					{
						((unsigned long *)(*uids))[sumNum-1] = (*hidTocH)->sums[count].uidHash;
						sumNum--;
					}
				}
			
				//
				// connect and do the actual expunge
				//
				
				// display some progress indicating which mailbox we're going to expunge
				if (InAThread())
				{
					LockMailboxNodeHandle(mailboxNode);
					PathToMailboxName ((*mailboxNode)->mailboxName, mName, (*mailboxNode)->delimiter);
					UnlockMailboxNodeHandle(mailboxNode);
					ComposeRString(progressMessage,IMAP_EXPUNGE_MAILBOX,mName);
					PROGRESS_START;
					PROGRESS_MESSAGE(kpTitle,progressMessage);
				 	PROGRESS_MESSAGER(kpSubTitle,IMAP_LOGGING_IN);	
				}
							 		
				// Create a new IMAP stream
				imapStream = GetIMAPConnection(IMAPExpungeTask, CAN_PROGRESS);
				if (imapStream)
				{
					LockMailboxNodeHandle(mailboxNode);
					if (IMAPOpenMailbox(imapStream, (*mailboxNode)->mailboxName,false))
					{					
						if (result = Expunge(imapStream))
						{
							// remove these messages from the tocH on screen
							// note, this will take care of the hidden toc as well.
							UpdateLocalSummaries(tocH, uids, false, true, false);
						}
						else	// couldn't expunge.  We're read only or something right now.
							IMAPError(kIMAPExpunge, kIMAPExpungeMailbox, errIMAPCantExpunge);
					}
					UnlockMailboxNodeHandle(mailboxNode);
					
					// No longer need to consider this mailbox in auto expunge.  Even if we failed above.
					SetIMAPMailboxNeeds(mailboxNode, kNeedsAutoExp, false);
						
					CleanupConnection(&imapStream);
				}
				ZapHandle(uids);			
			}
			else
			{
				IMAPError(kIMAPExpunge, kIMAPMemErr, MemError());
			}	
		}
		// else 
			// no summaries are marked for deletion.
	}	
	else	// could not open mailbox.
		IMAPError(kIMAPExpunge, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);
	
	CurPers = oldPers;
	return (result);
}

/************************************************************************
 *	StreamAppendDriverInit - initialize a STRING and read the first bit
 ************************************************************************/
static void StreamAppendDriverInit(STRING *s, void *data, unsigned long size)
{
#pragma unused(data)

	unsigned long first, nBytes;
	unsigned long chunksize = 0;
	STRINGDataStructPtr msDataPtr = (STRINGDataStructPtr)(s->data);
	
	// makes no sense to do anything if we don't have a string
	if (!s || !msDataPtr) return;

	// "size" is the rfc822 size of the message. Set it and don't change it
	s->size = size;

	// Set STRING to point to the buffer in the file reader.
	s->chunk = s->curpos = msDataPtr->buffer;

	// Nothing in the buffer yet.
	s->chunksize = s->cursize = 0;

	// Go read the first chunk from the source message
	first	= 0;
	nBytes	= MIN(s->size, msDataPtr->bufferSize);

	// Read:
	msDataPtr->result = UIDFetchPartialContentsToBuffer(msDataPtr->imapStream, msDataPtr->uid, nil, first, nBytes, msDataPtr->buffer, msDataPtr->bufferSize, &chunksize);
	
	s->cursize = s->chunksize = chunksize;

	// Catch error.
	if (s->cursize <= 0)
	{
		s->cursize = 0;
		s->chunksize = 0;
	}

	// Increment bytes read
	msDataPtr->bytesRead += s->cursize;

	// never set offset 
	s->data1 = s->offset = 0;
}

/************************************************************************
 *	StreamAppendDriverNext - grab the next chunk of a message on a server
 ************************************************************************/
static char StreamAppendDriverNext (STRING *s)
{
	unsigned long first, nBytes;
	STRINGDataStructPtr msDataPtr = (STRINGDataStructPtr)(s->data);
		
	// Make sure we have a string
	if (!s || !msDataPtr) return (nil);

	// Re-initiallize bits of the string
	s->chunk = s->curpos = msDataPtr->buffer;
	s->cursize = 0;
	*s->chunk = 0;

	// Fetch next chunk. 
	first = msDataPtr->bytesRead;
	
	if (msDataPtr->bytesRead >= s->size) nBytes = 0;
	else nBytes = MIN(msDataPtr->bufferSize, s->size - msDataPtr->bytesRead);

	if (nBytes <= 0)
	{
		// Error.
		s->cursize = 0;
		s->curpos = NULL;
		return (nil);
	}

	// Go read another chunk-full from the message.
	UIDFetchPartialContentsToBuffer(msDataPtr->imapStream, msDataPtr->uid, nil, first, nBytes, msDataPtr->buffer, msDataPtr->bufferSize, &(s->cursize));

	// Did we get something back?
	if (s->cursize < 0)
	{
		// Error.
		s->cursize = 0;
		s->curpos = nil;
	}

	// Increment bytes read
	msDataPtr->bytesRead += s->cursize;

	return (*s->chunk);
}

/************************************************************************
 *	StreamAppendDriverSetpos - update the current position.
 ************************************************************************/
static void StreamAppendDriverSetpos (STRING *s, unsigned long i)
{
	if (s)
	{
		s->curpos = s->chunk + i;
		s->cursize = s->chunksize - i;
	}
}

/************************************************************************
 *	FileAppendDriverInit - initialize a STRING and read the first bit
 ************************************************************************/
static void FileAppendDriverInit(STRING *s, void *data, unsigned long size)
{
#pragma unused(data)

	unsigned long first, nBytes;
	STRINGFileStructPtr msDataPtr = (STRINGFileStructPtr)(s->data);
	short refN;
	OSErr err;
		
	// makes no sense to do anything if we don't have a string
	if (!s || !msDataPtr) return;

	// "size" is the size of the spooled message. Set it and don't change it
	s->size = size;

	// Set STRING to point to the buffer in the file reader.
	s->chunk = s->curpos = msDataPtr->buffer;

	// Nothing in the buffer yet.
	s->chunksize = s->cursize = 0;

	// Go read the first chunk from the source message
	first	= 0;
	nBytes	= MIN(s->size, msDataPtr->bufferSize);

	// Read:
	if ((err=AFSpOpenDF(&msDataPtr->spoolSpec,&msDataPtr->spoolSpec,fsRdPerm,&refN))==noErr)
	{
		if ((err=ARead(refN,&nBytes,msDataPtr->buffer))!=noErr)
		{
			// clear out the buffer if there was an error
			WriteZero(msDataPtr->buffer, msDataPtr->bufferSize);
		}
		MyFSClose(refN);
	}
		
	s->cursize = s->chunksize = nBytes;

	// Catch error.
	if (s->cursize <= 0)
	{
		s->cursize = 0;
		s->chunksize = 0;
	}

	// Increment bytes read
	msDataPtr->bytesRead += s->cursize;

	// never set offset 
	s->data1 = s->offset = 0;
}

/************************************************************************
 *	StreamAppendDriverNext - grab the next chunk of a message from a file
 ************************************************************************/
static char FileAppendDriverNext(STRING *s)
{
	unsigned long first, nBytes;
	STRINGFileStructPtr msDataPtr = (STRINGFileStructPtr)(s->data);
	OSErr err = noErr;
	short refN;
			
	// Make sure we have a string
	if (!s || !msDataPtr) return (nil);

	// Re-initiallize bits of the string
	s->chunk = s->curpos = msDataPtr->buffer;
	s->cursize = 0;
	*s->chunk = 0;

	// Fetch next chunk. 
	first = msDataPtr->bytesRead;
	
	if (msDataPtr->bytesRead >= s->size) nBytes = 0;
	else nBytes = MIN(msDataPtr->bufferSize, s->size - msDataPtr->bytesRead);

	if (nBytes <= 0)
	{
		// Error.
		s->cursize = 0;
		s->curpos = NULL;
		return (nil);
	}

	// Go read another chunk-full from the message.
	if (!(err=AFSpOpenDF(&msDataPtr->spoolSpec,&msDataPtr->spoolSpec,fsRdPerm,&refN)))
	{
		if (!(err=SetFPos(refN,fsFromStart,msDataPtr->bytesRead)))
		{
			err = ARead(refN,&nBytes,msDataPtr->buffer);
		}
		
		MyFSClose(refN);
	}

	// clear out the buffer if there was an error
	if (err != noErr) WriteZero(msDataPtr->buffer, msDataPtr->bufferSize);

	s->cursize = s->chunksize = nBytes;
	
	// Did we get something back?
	if (s->cursize < 0)
	{
		// Error.
		s->cursize = 0;
		s->curpos = nil;
	}

	// Increment bytes read
	msDataPtr->bytesRead += s->cursize;

	return (*s->chunk);
}

/************************************************************************
 *	StreamAppendDriverSetpos - update the current position.
 ************************************************************************/
static void FileAppendDriverSetpos(STRING *s, unsigned long i)
{
	if (s)
	{
		s->curpos = s->chunk + i;
		s->cursize = s->chunksize - i;
	}
}

/************************************************************************
 *	IMAPMessageDownloaded - return true is a given summary has been dld
 ************************************************************************/
Boolean IMAPMessageDownloaded(TOCHandle t, short s) 
{
	return ((*t)->sums[s].offset >= 0);
}

/************************************************************************
 *	IMAPMessageBeingDownloaded - return true if a summary needs to be dld
 ************************************************************************/
Boolean IMAPMessageBeingDownloaded(TOCHandle tocH, short s) 
{
	Boolean onItsWay = false;
	MailboxNodeHandle mailboxNode;
	PersHandle oldPers = CurPers;
	threadDataHandle index;
	short numUids;
	SignedByte state;
	
	// it's not being downloaded if it's not an IMAP message
	if (!(*tocH)->imapTOC) return (false);
	
	// it's not being downloaded if it's already here
	if ((*tocH)->sums[s].offset >= 0) return (false);
	
	// if the message lives in an IMAP mailbox ...
	if (!onItsWay)
	{
		mailboxNode = TOCToMbox(tocH);
		CurPers = TOCToPers(tocH);
		
		if (mailboxNode && CurPers)
		{
			// and the mailbox is set up to receive message bodies during a resync ...
			if (PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS) || PrefIsSet(PREF_IMAP_FULL_MESSAGE))
			{
				// and there's a resync underway ...
				if (FindNodeByToc(tocH))
				{
					// then the message is already being downloaded.
					onItsWay = true;
				}
			}
		}
		
		CurPers = oldPers;
	}
	
	// look at all fetching tasks and see if this summary is being fetched currently
	if (!onItsWay)
	{	
		// are there any threads currently downloading this message?
		for (index=gThreadData;index && !onItsWay;index=(*index)->next)
		{
			if ((*index)->currentTask == IMAPFetchingTask)
			{
				state = HGetState((Handle)index);
				LDRef(index);
				if (SameTOC((*index)->imapInfo.destToc, tocH))
				{
					if ((*index)->imapInfo.uids)
					{
						for (numUids = 0; numUids < (GetHandleSize((*index)->imapInfo.uids)/sizeof(unsigned long)); numUids++)
						{
							if (((unsigned long *)(*((*index)->imapInfo.uids)))[numUids] == (*tocH)->sums[s].uidHash)
							{
								onItsWay = true;
							}
						}
					}
				}
				HSetState((Handle)index,state);
			}
		}	
	}
	return (onItsWay);
}

/************************************************************************
 *	IMAPAbortMessageFetch - cancel a message download that's underway
 ************************************************************************/
void IMAPAbortMessageFetch(TOCHandle tocH, short sumNum)
{
	threadDataHandle index;
	short numUids;
	SignedByte state;
	
	// must have a TOC
	if (!tocH) return;
	
	// must have a summary
	if (sumNum < 0 || sumNum > (*tocH)->count) return;
	
	// nothing to do if this is not an IMAP toc ...
	if (!(*tocH)->imapTOC) return;
	
	// no need to cancel if this message isn't even being downloaded.
	if (!IMAPMessageBeingDownloaded(tocH, sumNum)) return;
	
	// don't cancel the download if this message is being previewd
	if (IMAPMessageBeingPreviewed(tocH, sumNum)) return;
	
	// don't cancel the download if the message window is open anywhere.
	if ((*tocH)->sums[sumNum].messH && (*(*tocH)->sums[sumNum].messH)->win) return;
	
	// are there any threads currently downloading this message?
	for (index=gThreadData;index;index=(*index)->next)
	{
		if ((*index)->currentTask == IMAPFetchingTask)
		{
			state = HGetState((Handle)index);
			LDRef(index);
			if (SameTOC((*index)->imapInfo.destToc, tocH))
			{
				if ((*index)->imapInfo.uids)
				{
					for (numUids = 0; numUids < (GetHandleSize((*index)->imapInfo.uids)/sizeof(unsigned long)); numUids++)
					{
						if (((unsigned long *)(*((*index)->imapInfo.uids)))[numUids] == (*tocH)->sums[sumNum].uidHash)
						{
							// cancel the thread
							SetThreadGlobalCommandPeriod ((*index)->threadID, true);
							HSetState((Handle)index,state);
							break;
						}
					}
				}
			}
			HSetState((Handle)index,state);
		}
	}
}

/************************************************************************
 *	IMAPMessageBeingPreviewed - is this IMAP message in a preview pane?
 ************************************************************************/
Boolean IMAPMessageBeingPreviewed(TOCHandle tocH, short sumNum)
{
	Boolean previewed = false;
	TOCHandle curToc, realTocH;
	short curSum, realSumNum;
	WindowPtr	winWP;
	
	// must have a TOC
	if (!tocH) return false;
	
	// must have a summary
	if (sumNum < 0 || sumNum > (*tocH)->count) return false;
	
	// if this message is being previewed in this TOC, nothing special to do.
	if ((*tocH)->previewID == (*tocH)->sums[sumNum].serialNum)
	{
		previewed = true;
	}
	else
	{
		// go through all open windows ...
		for (winWP=FrontWindow_();winWP && !previewed;winWP=GetNextWindow(winWP))
		{
			// only look at search windows ...
			if (IsSearchWindow(winWP))
			{
				// get the toc that this search window uses
				GetSearchTOC(GetWindowMyWindowPtr(winWP),&curToc);
				if (curToc && (*curToc)->virtualTOC)
				{
					// is this TOC previewing a message?
					if ((*curToc)->previewID!=0)
					{
						// see if it's the same message as the one we're about to close
						curSum = FindSumBySerialNum(curToc, (*curToc)->previewID);
						if (curSum < (*curToc)->count)
						{
							if ((realTocH = GetRealTOC(curToc,curSum,&realSumNum)))
							{
								// if it is, don't stop the music ...
								if ((*realTocH)->sums[realSumNum].uidHash == (*tocH)->sums[sumNum].uidHash)
									previewed = true;
							}
						}
					}
				}
			}
		}
	}
		
	return (previewed);
}
			
/************************************************************************
 *	FlagsString - build a string of flags
 ************************************************************************/
char *FlagsString(char **flags, Boolean seen, Boolean deleted, Boolean flagged, Boolean answered, Boolean draft, Boolean recent, Boolean sent)
{
#pragma unused(recent)
	short maxSize = strlen(IMAP_SEEN_STRING_FLAG) + strlen(IMAP_DELETED_STRING_FLAG) 
					+ strlen(IMAP_FLAGGED_STRING_FLAG) + strlen(IMAP_ANSWERED_STRING_FLAG) 
					+ strlen(IMAP_DRAFT_STRING_FLAG) + 16;
	Boolean first = true;
	  
	if (!flags) return (nil);
	   
	*flags = (char *)fs_get(maxSize);
	if (*flags)
	{
		strcat(*flags, "(");
		
		if (seen)
		{
			if (!first) strcat(*flags, " ");
			strcat(*flags, IMAP_SEEN_STRING_FLAG);
			first = false;
		}

		if (deleted)
		{
			if (!first) strcat(*flags, " ");
			strcat(*flags, IMAP_DELETED_STRING_FLAG);
			first = false;
		}

		if (flagged)
		{
			if (!first) strcat(*flags, " ");
			strcat(*flags, IMAP_FLAGGED_STRING_FLAG);
			first = false;
		}


		if (answered)
		{
			if (!first) strcat(*flags, " ");
			strcat(*flags, IMAP_ANSWERED_STRING_FLAG);
			first = false;
		}


		if (draft)
		{
			if (!first) strcat(*flags, " ");
			strcat(*flags, IMAP_DRAFT_STRING_FLAG);
			first = false;
		}

		if (sent)
		{
			Str255 sentFlag;
			
			GetRString(sentFlag, IMAP_SENT_FLAG);
			if (sentFlag[0])
			{
				sentFlag[sentFlag[0] + 1] = 0;
				
				if (!first) strcat(*flags, " ");
				strcat(*flags, sentFlag+1);
				first = false;
			}
		}
		
		strcat(*flags, ")");
	}
	else
	{
		WarnUser(MEM_ERR, MemError());
	}
	
	return (*flags);
}

/************************************************************************
 *	RsyncCurPersInbox - resync the inbox for the current personality
 ************************************************************************/
OSErr RsyncCurPersInbox(void)
{
	OSErr err = false;
	Boolean isImap = false;
	MailboxNodeHandle inBox = nil;
 	
 	// locate the INBOX of this personality
 	inBox = LocateInboxForPers(CurPers);
 	
 	// and resync it.
 	if (inBox) 
 	{
 		LockMailboxNodeHandle(inBox);
		err = GetMailbox(&(*inBox)->mailboxSpec, false);
 		UnlockMailboxNodeHandle(inBox);
 	}
 	
 	return (err);
}			

/************************************************************************
 *	WriteBoundary - write a boundary between parts in a spool file.
 ************************************************************************/
void IMAPWriteBoundary(IMAPStreamPtr stream, IMAPBODY *body, unsigned long uid, char *section, Boolean outerBoundary)
{
	long len = 0;
	IMAPBODY *mainBody = nil;
	PARAMETER *param = nil;
	Boolean foundIt = false;
	
	// if we weren't passed a body, write out the top level boundary
	if (!body)
	{
		mainBody = UIDFetchStructure(stream, uid);
		body = mainBody;
	}

	if (body)
	{
		// iterate through the body's parameter, looking for BOUNDARY
		param = body->parameter;
		while (param && !foundIt)
		{
			len = strlen(param->value);
			if (param->attribute && !striscmp(param->attribute,"BOUNDARY"))
			{
				foundIt = true;
				
				FSWriteP(stream->mailStream->refN,CrLf);
				FSWriteP(stream->mailStream->refN,dashes);
				AWrite(stream->mailStream->refN,&len,param->value);
				if (outerBoundary) FSWriteP(stream->mailStream->refN,dashes);
				FSWriteP(stream->mailStream->refN,CrLf);
				
				// if we were passed a section number, write the section's headers, too.
				if (section) 
				{
					FetchMIMEHeader(stream, uid, section, 0L);
//					FSWriteP(stream->mailStream->refN,CrLf);
				}
			}
					
			// next parameter
			param = param->next;
		}
	}
		
	// Cleanup
	if (mainBody) FreeBodyStructure(mainBody);
}

/************************************************************************
 *	SaveAttachmentStub - save the information needed to fetch an
 *	 attachment later.  Put the directly into the spool file.
 ************************************************************************/
Boolean SaveAttachmentStub(IMAPStreamPtr stream, unsigned long uid, char *section, IMAPBODY *body)
{
	Str255 name, typeString, subTypeString;
	ResType type, creator;
	MIMEMap	mm;	
	short index = 0;
	Str255 scratch, scratch2;
	Str255 buffer;
			
	// figure out the name
	GetFilenameParameter(body, name);
	if (*name)
	{	
		// figure out the type and creator
		creator = CREATOR;
		type = kFakeAppType;
		
		// turn the subtype into a p-string
		subTypeString[0] = strlen(body->subtype);
		strncpy(subTypeString+1, body->subtype, MIN(subTypeString[0], sizeof(subTypeString)));
		
		// make a reasonable guess at this part's type and creator
		if (FindMIMEMapPtr(BodyTypeCodeToPString(body->type, typeString),subTypeString,name,&mm))
		{
			type = mm.type;
			creator = mm.creator;
		}

		// don't bother creating a stub at all if this is something we're just going to end up tossing.
		if (mm.flags&mmDiscard) return (true);
		
		// make a better guess from x-mac-type and x-mac-creator headers
		XMacHeaders(body, &type, &creator);
		
		// if we're dealing with an executable, then pack it.  See SafeInfo() in hexbin.c
		if (TypeIsOnListWhereAndIndex(type,EXECUTABLE_TYPE_LIST,nil,&index)) type = index ? (kFakeAppType&0xffffff00)|('0'+index) : kFakeAppType;

		// write a fake mime header	in the spool file so our decoding routines know to decode this into the IMAP Attachments folder.
		IMAPWriteBoundary(stream, nil, uid, nil, false);
		
		// write a fake section header to tell the decoder to put this part into an attachment stub
		ComposeRString(buffer,MIME_CT_PFMT, InterestHeadStrn+hContentType, GetRString(scratch, MIME_TEXT), GetRString(scratch2, MIME_PLAIN),  AttributeStrn+aName, name, CrLf);
		FSWriteP(stream->mailStream->refN,buffer);

		ComposeRString(buffer,MIME_CT_ANNOTATE,  AttributeStrn+aMacType,Long2Hex(scratch,type), CrLf);
		FSWriteP(stream->mailStream->refN,buffer);

		ComposeRString(buffer,MIME_CT_ANNOTATE, AttributeStrn+aMacCreator,Long2Hex(scratch,creator), CrLf);
		FSWriteP(stream->mailStream->refN,buffer);

		ComposeRString(buffer,MIME_CD_FMT, InterestHeadStrn+hContentDisposition, ATTACHMENT, AttributeStrn+aFilename, name, CrLf);
		FSWriteP(stream->mailStream->refN,buffer);

		ComposeRString(buffer,MIME_V_FMT, InterestHeadStrn+hContentEncoding, IMAP_STUB_ENCODING, CrLf);
		FSWriteP(stream->mailStream->refN,buffer);

		FSWriteP(stream->mailStream->refN,CrLf);
		
		// save the stub information to the message spool file	
		NumToString((*CurPersSafe)->persId, buffer);
		PCat(buffer, "\p,");			
		FSWriteP(stream->mailStream->refN,buffer);

		NumToString(uid, buffer);				
		PCat(buffer, "\p,");	
		FSWriteP(stream->mailStream->refN,buffer);
				
		ComposeString(buffer, "\p%s,", section);
		FSWriteP(stream->mailStream->refN,buffer);
			
		NumToString(body->size.bytes, buffer);
		PCat(buffer, "\p,");			
		FSWriteP(stream->mailStream->refN,buffer);
				
		NumToString(body->size.lines, buffer);		
		PCat(buffer, "\p,");	
		FSWriteP(stream->mailStream->refN,buffer);;
		
		FSWriteP(stream->mailStream->refN,CrLf);
	}
	
	return (*name != 0);
}

/************************************************************************
 *	XMacHeaders - search the body for x-mac-type and x-mac-creator
 ************************************************************************/
void XMacHeaders(IMAPBODY *body, ResType *type, ResType *creator)
{
	Str255 macType, macCreator, scratch;
	PARAMETER *param = nil;
	
	// must have a body for this to make any sense
	if (!body) return;
	
	// will be looking for x-mac-type and x-mac-creator headers
	GetRString(macType, AttributeStrn+aMacType);
	GetRString(macCreator, AttributeStrn+aMacCreator);

	// iterate through the body's parameter, looking for these headers	
	param = body->parameter;
	while (param)
	{
		// x-mac-type?
		if (!pstrincmp(param->attribute, macType,macType[0]))
		{
			if (noErr==Hex2Bytes(param->value,8,scratch)) BMD(scratch,type,sizeof(ResType));
		}
		// x-mac-creator?
		else if (!pstrincmp(param->attribute, macCreator,macCreator[0]))
		{
			if (noErr==Hex2Bytes(param->value,8,scratch)) BMD(scratch,creator,sizeof(ResType));	
		}
		
		// next parameter
		param = param->next;
	}
}
					
/************************************************************************
 *	IMAPError - alert the user that an IMAP operation has failed.
 ************************************************************************/
OSErr IMAPError(short operation, short explanation, OSErr err)
{				
	return (IE(nil, operation, explanation, err));
}

OSErr IE(IMAPStreamPtr imapStream, short operation, short explanation, OSErr err)
{
	Str255 scratch;
	Str255 operationText;
	Str255 explanationText;
	Str255 errText;
	Str63 mboxName;
	
	short realSettingsRef = SettingsRefN;
	
	// if the user cancelled, don't display an error message
	if (CommandPeriod) return (userCanceledErr);
	
	// if we've already displayed an error, just return
	if (imapStream && imapStream->mailStream && imapStream->mailStream->errorred)
	{
		imapStream->mailStream->errorred = false;
		return (err);
	}
	
	SettingsRefN = GetMainGlobalSettingsRefN();

	// describe what went wrong
	
	// Stash mbox name, if any
	if (imapStream && imapStream->mbox)
	{
		*mboxName = 1;
		mboxName[1] = ' ';
		PCopy(mboxName,(*imapStream->mbox)->mailboxSpec.name);
	}
	else
		*mboxName = 0;
	
	// special case error descriptions that want the name of the mailbox	
	if ((operation == kIMAPCompleteResync) && imapStream && imapStream->mbox)
	{
		ComposeString(explanationText, GetRString(scratch,IMAP_OPERATIONS_STRN+operation), (*imapStream->mbox)->mailboxSpec.name);
		ComposeRString(operationText,IMAP_ERR_OPERATION_FMT,explanationText);
		// have mbox name, do not append
		*mboxName = 0;
	}
	else
		ComposeRString(operationText,IMAP_ERR_OPERATION_FMT,GetRString(scratch,IMAP_OPERATIONS_STRN+operation));	
		
	// give an explanation, if none was provided from the server.
	switch (err)
	{
		case errIMAPOutOfMemory:
		case errIMAPNoServer:
		case errIMAPNoMailstream:
		case errIMAPNoAccount:
		case errIMAPNoMailbox:
		case errIMAPMailboxNameInvalid:
		case errIMAPStreamIsLocked:
		case errNotIMAPPers:
		case errNotIMAPMailboxErr:
		case errIMAPListInUse:
		case errIMAPNoTrash:
		case errIMAPStubFileBad:
		case errIMAPBadEncodingErr:
		case errIMAPMailboxChangedErr:
		case errIMAPReadOnlyStreamErr:
			GetRString(explanationText,IMAP_EXPLANATIONS_STRN+explanation);
			break;
			
		default:
			// otherwise, use the error reported by the IMAP server
			if (!gIMAPErrorString[0]) GetRString(explanationText,IMAP_EXPLANATIONS_STRN+explanation);
			else
			{
				PCopy(explanationText, gIMAPErrorString);
				gIMAPErrorString[0] = 0;
			}
	}

	SettingsRefN = realSettingsRef;

	
	// and the error number
	NumToString(err, errText);
	PSCat(errText,mboxName);
	
	MyParamText(operationText,errText,explanationText,"");
	ReallyDoAnAlert(BIG_OK_ALRT,Stop);

	if (imapStream && imapStream->mailStream && imapStream->mailStream->errorred) imapStream->mailStream->errorred = false;
	
	return (err);
}

/************************************************************************
 *	FTMExpunge - move all messages marked for deletion to the trash
 *		mailbox, and expunge
 ************************************************************************/
OSErr FTMExpunge(IMAPStreamPtr imapStream, TOCHandle tocH)
{
	OSErr err = noErr;
	short sumNum;
	MailboxNodeHandle trash = GetIMAPTrashMailbox(CurPersSafe, false, false);
	Handle uids = nil;
	long messageCount = 0;
	long count;
	unsigned long uid;
	Str255 uidStr;
	
	// must have a toc to look at
	if (!tocH) 
		return (noErr);
	
	// nothing to do if we're caching deleted messages
	if (GetHiddenCacheMailbox(TOCToMbox(tocH), false, false))
		return (noErr);
			
	// only makes sense to do this when FTM is on
	if (FancyTrashForThisPers(tocH))
	{
		// must have a trash mailbox
		if (trash != nil)
		{
			// must have a stream and be connected
			if (imapStream && IsSelected(imapStream->mailStream))
			{
				//
				// build the list of deleted messages
				//
				
				// how many deleted messages are there?
				messageCount = CountDeletedIMAPMessages(tocH);
				
				// continue if there are messages to be deleted
				if (messageCount > 0)
				{				
					// build a list of uids to move
					uids = NuHandleClear(messageCount*sizeof(unsigned long));
					if (uids)
					{
						// grab the messages in the visible tocH
						count = messageCount;
						for (sumNum=0;sumNum<(*tocH)->count && count;sumNum++)
							if ((*tocH)->sums[sumNum].opts&OPT_DELETED)
								BMD(&((*tocH)->sums[sumNum].uidHash),&((unsigned long *)(*uids))[--count],sizeof(unsigned long));
					}
					else
					{
						WarnUser(MemError(), MEM_ERR);
						return (false);
					}
					
					//
					// now COPY those messages to the trash mailbox
					//
					if (uids) 
					{	
						//unmark the messages for deletion
						count = messageCount;
						while (count--)
						{
							// mark the message for deletion.  Do not expunge.
							uid = ((unsigned long *)(*uids))[count];
							sprintf (uidStr,"%lu",uid);
							UIDUnDeleteMessages(imapStream, uidStr);
						}
					
						// copy the messages to the trash mailbox
						CopyMessages(imapStream, trash, uids, true, true);
						
						// and remark them for deletion
						count = messageCount;
						while (count--)
						{
							// mark the message for deletion.  Do not expunge.
							uid = ((unsigned long *)(*uids))[count];
							sprintf (uidStr,"%lu",uid);
							UIDDeleteMessages(imapStream, uidStr, false);
						}
					}
					
				}
				// else
					// no messages to be deleted.  Continue with no error.
				
				//
				//	Now do the actual expunge
				//
				if (DoesIMAPMailboxNeed(TOCToMbox(tocH), kNeedsFilter))
				{
					// this mailbox is currently being filtered.  Mark it for an expunge and do it later.
					FlagForExpunge(tocH);
				}
				else
				{
					if (Expunge(imapStream))
						UpdateLocalSummaries(tocH, uids, true, true, false);
					
					// No longer need to consider this mailbox in auto expunge
					SetIMAPMailboxNeeds(TOCToMbox(tocH), kNeedsAutoExp, false);
				}
			}
			else
				err = errIMAPNotConnected;
		}
		else
			err = errIMAPNoTrash;
	}
	
	return (err);
}	

/************************************************************************
 *	EmptyAllImapTrashes - zap all the IMAP trash mailboxes
 ************************************************************************/
Boolean EmptyImapTrashes(IMAPEmptyTrashEnum which)
{
	Boolean result = false;
	PersHandle oldPers = CurPers;
	Boolean online = false;
			
	// loop through all personalities and empty the trash.
	for (CurPers = PersList; CurPers; CurPers = (*CurPers)->next)
	{
		// is this an IMAP personality?
		if (IsIMAPPers(CurPers))
		{
					
			if ((which == kEmptyAllTrashes)											// all trash mailboxes								
				|| ((which == kEmptyAutoCheckTrashes) && PrefIsSet(PREF_AUTO_CHECK))// only those belonging to personalities that check mail automatically			
				|| ((which == kEmptyActiveTrashes) && IMAPPersActive(CurPers)))		// only "active" IMAP trash mailboxes	
			{
				// is this personality currently using the fancy trash scheme?
				if (!PrefIsSet(PREF_IMAP_NO_FANCY_TRASH))
				{
					// make sure we're online
					if (!online)
					{
						if (Offline && GoOnline()) return(false);
						else online = true;
					}
					
					result = IMAPEmptyPersTrash();
				}
			}
		}
	}

	return (result);
}
	
/************************************************************************
 *	IMAPEmptyPersTrash - empty the trash for the current personality.
 *	 Return if something got expunged.
 ************************************************************************/
Boolean IMAPEmptyPersTrash(void)
{
	Boolean result = false;
	MailboxNodeHandle persTrash = nil;
	TOCHandle trashToc = nil;
	
	// locate the trash mailbox
	if ((persTrash=GetIMAPTrashMailbox(CurPers, false, true)))
	{
		LockMailboxNodeHandle(persTrash);
		trashToc=TOCBySpec(&((*persTrash)->mailboxSpec));
		UnlockMailboxNodeHandle(persTrash);
			
		// remove all deleted messages from the trash mailbox	
		if (trashToc) result = IMAPWipeBox(trashToc, persTrash);	
	}
		
	return (result);
}	

/************************************************************************
 *	IMAPWipeBox - wipe out an entire mailbox on the server.  Not
 *	 thread safe.
 ************************************************************************/
Boolean IMAPWipeBox(TOCHandle tocToWipe, MailboxNodeHandle nodeToWipe)
{
	Boolean result = false;
	IMAPStreamPtr imapStream = nil;
	short sum;
	
	// must have a box to wipe
	if (!tocToWipe || !nodeToWipe) return (false);
	
	// first, go close all the open messages in this toc
	for (sum=0;sum<(*tocToWipe)->count;sum++)
	{
		if ((*tocToWipe)->sums[sum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(*tocToWipe)->sums[sum].messH)->win));
	}
		
	// Create a new IMAP stream
	if (imapStream = GetIMAPConnection(IMAPDeleteTask, CAN_PROGRESS)) 
	{
		// SELECT the mailbox this message resides in on the server
		LockMailboxNodeHandle(nodeToWipe);
		if (IMAPOpenMailbox(imapStream, (*nodeToWipe)->mailboxName,false))
		{		
			// mark all the messages for deletion and expunge
			result = UIDDeleteMessages(imapStream, "1:*", true);
			
			// No longer need to consider this mailbox in auto expunge
			SetIMAPMailboxNeeds(nodeToWipe, kNeedsAutoExp, false);
			
			if (!result)	// delete failed
				IMAPError(kIMAPEmptyTrash, kIMAPDeleteMessage, errIMAPDeleteMessage);

		}
		else	// failed to open mailbox
			IE(imapStream, kIMAPEmptyTrash, kIMAPSelectMailboxErr, errIMAPSelectMailbox);		
			
		UnlockMailboxNodeHandle(nodeToWipe);
			
		// close down and clean up
		PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
		CleanupConnection(&imapStream);
	}
	
	// empty the trash mailbox if the delete succeeded.  Don't care if it was open.
	if (result)
	{
		// this is happening in the foreground, so go ahead an just wipe out all the summaries in the mailbox.
		for(sum=(*tocToWipe)->count;sum--;) DeleteIMAPSum(tocToWipe,sum);
	}

	return (result);
}

/************************************************************************
 * GetFilenameParameter - Search the Content-Disposition parameter list 
 *	for a "filename" parameter.   Returns a p-string.
 ************************************************************************/
void GetFilenameParameter(IMAPBODY *body, Str255 fileName)
{
	Str255 mimeName;
	Str255 mimeCDName;
	PARAMETER *param = nil;
	
	GetRString(mimeName, NAME);							// "name"
	GetRString(mimeCDName, AttributeStrn+aFilename);	// "filename"
		
	// Initialize in case we fail:
	*fileName = 0;

	// Must have a body:
	if (!body) return;

	// If there is a content-disposition, use that.
	param = body->disposition.parameter;
	while (param)
	{
		if (param->attribute && (strincmp(param->attribute, mimeCDName+1, mimeCDName[0]) == 0))
		{
			if (param->value)
			{
				fileName[0] = MIN(strlen(param->value), 255);
				BMD(param->value, fileName+1, fileName[0]);
				break;
			}
		}
		param = param->next;
	}

	if (fileName[0] == nil)
	{
		// If there is a parameter list from content-type:, use the filename from there
		param = body->parameter;
		while (param)
		{
			if (param->attribute && (((strincmp(param->attribute, mimeName+1, mimeName[0]) == 0 ))
				|| (strincmp(param->attribute, mimeCDName+1, mimeCDName[0]) == 0)))
			{
				if (param->value)
				{
					fileName[0] = MIN(strlen(param->value), 255);
					BMD(param->value, fileName+1, fileName[0]);
					break;
				}
			}
			param = param->next;
		}
	}
	
	if (fileName[0] == nil)
	{
		// if we can't find a filename at all, assign one.
		GetRString(fileName, UNTITLED);
	}
}	

/************************************************************************
 * BodyTypeCodeToPString - given an IMAPBODY body type, return a p-string
 ************************************************************************/
PStr BodyTypeCodeToPString(short type, PStr string)
{
	short index = 0;
	
	string[0] = 0;

	switch (type)
	{
		case TYPETEXT:
			index = MIME_TEXT;
			break;

		case TYPEIMAGE:
			index = IMAGE;
			break;

		case TYPEAPPLICATION:
			index = MIME_APPLICATION;
			break;

		case TYPEMESSAGE:
			index = MIME_MESSAGE;
			break;

		case TYPEMULTIPART:
			index = MIME_MULTIPART;
			break;
	}
	
	if (index) GetRString(string, index);
	
	return (string);
}

/************************************************************************
 * IsIMAPAttachmentStub - return true if the spec points to an IMAP
 *	attachment stub.
 ************************************************************************/
Boolean IsIMAPAttachmentStub(FSSpecPtr spec)
{
	Boolean isIMAPAttachment = false;
	OSErr err = noErr;
	Str255 scratch, parentName;
	
	// figure out the name of the parent folder of this file
	if ((err=GetDirName(nil,spec->vRefNum,spec->parID,parentName))==noErr)
	{
		// is its parent's name "IMAP Attachments"?
		if (StringSame(parentName, GetRString(scratch, IMAP_ATTACH_FOLDER)))
		{
			// then this is an IMAP attachment.
			isIMAPAttachment = true;
		}
	}
	return (isIMAPAttachment);
}

/************************************************************************
 * DownloadIMAPAttachment - given a spec pointing to a stub file,
 *	go download the message from the server.
 ************************************************************************/
unsigned long DownloadIMAPAttachment(FSSpecPtr attachSpec, MailboxNodeHandle mailbox, Boolean forceForeground)
{
	unsigned long result = 0;
	FSSpecHandle attach = nil;
	OSErr err = noErr;
	
	attach = NuHandle(sizeof(FSSpec));
	if (attach && (err=MemError())==noErr)
	{
		BMD(attachSpec, *attach, sizeof(FSSpec));
		result = DownloadIMAPAttachments(attach, mailbox, forceForeground);
	}
	else
	{
		WarnUser(MEM_ERR, err);
		ZapHandle(attach);
	}
	
	return (result);
}

/************************************************************************
 * DownloadIMAPAttachments - given a spec pointing to a stub file,
 *	go download the message from the server.
 ************************************************************************/
unsigned long DownloadIMAPAttachments(FSSpecHandle attachments, MailboxNodeHandle mailbox, Boolean forceForeground)
{
	unsigned long uid = 0;
	IMAPTransferRec imapInfo;
	OSErr err = noErr;
	PersHandle oldPers = CurPers;
	XferFlags flags;

	// make sure we where given something to download
	if (!attachments) return (0);

	// we must be online
	if (Offline && GoOnline()) return(OFFLINE);
	
	if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable() || forceForeground)
	{
		// download the attachment
		uid = DoDownloadIMAPAttachments(attachments, mailbox);
	}
	else
	{
		// figure out who this mailbox belongs to
		CurPers = MailboxNodeToPers(mailbox);		
		if (CurPers)
		{
			// collect password now if we need it.
			err = CheckIMAPSettingsForPers();		
		
			// if we were given a password, set up the thread
			if (err==noErr)
			{
				Zero(flags);
				Zero(imapInfo);
				imapInfo.attachments = attachments;
				imapInfo.targetBox = mailbox;
				imapInfo.command = IMAPAttachmentFetch;
				err = SetupXferMailThread (false, false, true, false, flags, &imapInfo);
								
				// and remove any old task errors
				RemoveTaskErrors(IMAPAttachmentFetch,(*CurPers)->persId);
				
				// forget the keychain password so it's not written to the settings file
				if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
			}
		}
		CurPers = oldPers;
	}		
	
	return (uid);
}

/************************************************************************
 * DoDownloadIMAPAttachments - quit talking and start doing it.
 ************************************************************************/
unsigned long DoDownloadIMAPAttachments(FSSpecHandle attachments, MailboxNodeHandle mailbox)
{
	OSErr err = noErr;
	AttachmentStubStruct stub;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers;
	PersHandle pers = nil;
	FSSpec stubSpec, attachSpool, attachSpec, exist, newExist;
	short attachCount = 0;
	short ref = -1;
	unsigned long result = 0;
	Str255 mName, progressMessage;
	FInfo decodedInfo;
	Boolean tweakInfo;
	Boolean fetched = false;
	UpdateNodeHandle update = nil;
	TOCHandle tocH;
	long sumNum;
								
	// must have a stub folder, and a mailbox
	if (!attachments || !mailbox) return (false);
	
	// iterate through the list of stubs to download
	for (attachCount = 0; (attachCount < (GetHandleSize_(attachments)/sizeof(FSSpec))) && !CommandPeriod; attachCount++)
	{
		stubSpec = (*attachments)[attachCount];
	
		//
		// read the info from the stub file
		//
		
		if ((err=GetStubInfo(&stubSpec, &stub))==noErr)
				
		//
		//	Download the attachment into a spool file
		//
		
		if (pers=FindPersById(stub.persID))
		{
			CurPers = pers;

			// display some progress indicating which mailbox we're downloading to?
			LockMailboxNodeHandle(mailbox);
			PathToMailboxName ((*mailbox)->mailboxName, mName, (*mailbox)->delimiter);
			UnlockMailboxNodeHandle(mailbox);
			ComposeRString(progressMessage,IMAP_FETCHING_MESSAGE,mName);
			PROGRESS_START;
			PROGRESS_MESSAGE(kpTitle,progressMessage);
			PROGRESS_MESSAGER(kpSubTitle,IMAP_FETCH_ATTACHMENT);
		 		 	
			// Create a new IMAP stream
			imapStream = GetIMAPConnection(IMAPAttachmentFetch, CAN_PROGRESS);
			if (imapStream) 
			{					
				// SELECT the mailbox the attachment resides in on the server
				LockMailboxNodeHandle(mailbox);
				if (IMAPOpenMailbox(imapStream, (*mailbox)->mailboxName,false))
				{
					// indicate what we're downloading
					ComposeRString(progressMessage, IMAP_FETCH_ATTACHMENT_NAME, stubSpec.name);
					PROGRESS_MESSAGE(kpMessage,progressMessage);			
					BYTE_PROGRESS(nil,0,stub.sizeBytes);
				
					// put the attachment into a new file
					SimpleMakeFSSpec(stubSpec.vRefNum, stubSpec.parID, stubSpec.name, &attachSpool);
					if (UniqueSpec(&attachSpool,31)) continue;
					
					// open the file
					if ((err=HCreate(attachSpool.vRefNum,attachSpool.parID,attachSpool.name,CREATOR,'TEXT'))==noErr)
					{
						if ((err=AFSHOpen(attachSpool.name,attachSpool.vRefNum,attachSpool.parID,&ref,fsRdWrPerm))==noErr)
						{
							// kill any existing resources
							FSpKillRFork(&attachSpool);
							
							// Start writing at the beginning of the file
							if ((err=SetEOF(ref,0))==noErr) err = SetFPos(ref, fsFromStart, 0);

							// tell the imap stream where to put it
							imapStream->mailStream->refN = ref;
							
							// go download the part(s)
							if (imapStream->mailStream->refN > 0)
							{									
								// go download the part(s) to the spool file	
								if (!(fetched = DownloadAttachmentToSpoolFile(imapStream, nil, stub.uid, nil, stub.section)))
								{
									// failed to get the attachment.
									err = IMAPError(kIMAPFetchAttachment, kIMAPAttachmentFetchErr, errIMAPCouldNotFetchPart);	
								}
							}
						}
						else
						{
							err = IMAPError(kIMAPFetchAttachment, kIMAPTempFileExistErr, err);	
						}
					}
					else
					{
						err = IMAPError(kIMAPFetchAttachment, kIMAPTempFileExistErr, err);			
					}
				}
				else	// failed to open the destination mailbox.
				{
					IE(imapStream, kIMAPFetchAttachment, kIMAPSelectMailboxErr, errIMAPSelectMailbox);
				}
				
				UnlockMailboxNodeHandle(mailbox);
				
				// close down and clean up
				PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
				CleanupConnection(&imapStream);	
				PROGRESS_END;
				
				// if there was an error, or we cancelled ...
				if (err || CommandPeriod)
				{
					// failed - close and delete the spool file
					FSClose(ref);
					FSpDelete(&attachSpool);	
					fetched = false;
				}	
			}
			else
			{
				err = IMAPError(kIMAPFetchAttachment, kIMAPMemErr, errIMAPOutOfMemory);
			}
			CurPers = oldPers;
		}
		else
			err = IMAPError(kIMAPFetchAttachment, kIMAPStubFileErr, errIMAPStubFileBad);


		//
		// decode the spool file into an attachment, and swap it with the stub file
		//
		
		if (fetched)
		{
			Boolean renamedExisting = false;
			Str31 otherName;
			
			// reset the progress indicator ...
			BYTE_PROGRESS(nil, 0, stub.sizeLines);
			PROGRESS_MESSAGER(kpSubTitle,IMAP_DECODING_ATTACHMENT);	
			if ((err == noErr) && SpoolFileToAttachment(mailbox, stub.uid, &attachSpool, &attachSpec))
			{
				// final progress message ...
				BYTE_PROGRESS(nil, stub.sizeLines, stub.sizeLines);
				PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
				
				// get the decoded attachment's file info.
				tweakInfo = (AFSpGetFInfo(&attachSpec,&attachSpec,&decodedInfo) == noErr);

				// we're going to be moving files around.  Figure out the next available name in the Attachment folder.
				exist = AttFolderSpec;
				PCopy(exist.name,stubSpec.name);
				newExist = exist;
				UniqueSpec(&newExist,31);
						
				// swap file information with the stub file
				if ((FSpExchangeFiles(&attachSpec, &stubSpec))==noErr)
				{	
					// delete the stub file (now attachSpec)
					FSpDelete(&attachSpec);
					
					// move the attachment (now stubSpec) into the attachments folder
					if ((err = SpecMove(&stubSpec,&AttFolderSpec))==dupFNErr)
					{
						// failed to move it.  Rename conflicting file to the next available name
						if (!FSpRename(&exist,newExist.name))
						{
							renamedExisting = true;
							err = SpecMove(&stubSpec,&AttFolderSpec);
						}
					}
					
					if (err == noErr)
					{
						// stubSpec was moved to the attachments folder ...
						stubSpec.parID = AttFolderSpec.parID;
					}
					
					// give it it's real life decoded name.  
					// AttachSpec.name is guaranteed to be unique, since the decoder created it right into the attachments folder.
					// only do this if the stub file name is a MacToOther() name of the decoded name, so AttLine2Spec() can find it later.
					otherName[0] = nil;
					Mac2OtherName(otherName, attachSpec.name);
				
					// leave the encoded name if it's been uniquified.  Otherwise, the message will lose track of it!
					if (otherName[0]==stubSpec.name[0])
					{
						if (!strincmp(otherName+1,stubSpec.name+1,otherName[0]))
						{	
							FSpRename(&stubSpec,attachSpec.name);
							PCopy(stubSpec.name,attachSpec.name);
						}
					}
										
					// and name the duplicate file back to it's old name
					if (renamedExisting) FSpRename(&newExist, exist.name);
					
					// make sure the finder finds out about this new file and it's type
					if (tweakInfo) TweakFileType(&stubSpec,decodedInfo.fdType,decodedInfo.fdCreator);				
				}
				
				// record for statistics
				if (tocH = TOCBySpec(&(*mailbox)->mailboxSpec))
				{
					long sumNum;
					
					if (!TOCFindMessByMID(stub.uid,tocH,&sumNum))
						UpdateNumStatWithTime(kStatReceivedAttach,1,(*tocH)->sums[sumNum].seconds+ZoneSecs());
				}

				// update the FSSpecHandle with the location of the downloaded attachment.  Need this for updates later.
				(*attachments)[attachCount] = stubSpec;
	
				// return the uid of the message we just fetched part of
				result = stub.uid;
			}
			else
			{
				// Something went wrong.  We may have to display an error.
				
				// find the summary of the message who'se attachment we just fetched.
				tocH = TOCBySpec(&(*mailbox)->mailboxSpec);
				if (tocH && *tocH)
				{
					if (noErr==(TOCFindMessByMID(stub.uid,tocH,&sumNum)))
					{
						if (sumNum < (*tocH)->count)
						{
							if ((*tocH)->sums[sumNum].opts&OPT_EMSR_DELETE_REQUESTED)
							{
								// a translator asked to delete this message.  Ignore any errors here, the attachment was probably processed.
							}
							else
							{
								// there was an error decoding the attachment.
								IMAPError(kIMAPFetchAttachment, kIMAPAttachmentDecodeErr, BadEncoding || errIMAPBadEncodingErr);	
						
								// delete the attachspec as well
								FSpDelete(&attachSpec);	
							}
						}
					}	
				}
				
				// delete the spool file
				FSClose(ref);
				FSpDelete(&attachSpool);	
			}
		}
	}
	
	// make the window aware of the new attachment
	update = NewZH(UpdateNode);
	if (update)
	{
		(*update)->mailboxSpec = (*mailbox)->mailboxSpec;
		(*update)->uid = stub.uid;
		(*update)->attachSpecs = attachments;
		LL_Queue(gWindowUpdates, update, (UpdateNodeHandle));
	}	
											
	return (result);
}			
			
/************************************************************************
 * GetStubInfo - retrieve the stub info from inside a stub file
 ************************************************************************/
OSErr GetStubInfo(FSSpecPtr spec, AttachmentStubPtr stub)
{
	OSErr err = noErr;
	UHandle stubH = nil;
	unsigned long len = 0;
	char *begin, *scan, *end;
	Str255 buf;
	const unsigned long bufSize = sizeof(Str255) - 1;
	
	// clear out the stub
	WriteZero(stub, sizeof(AttachmentStubStruct));
	
	// read the stub info from the stub file
	if ((err=Snarf(spec, &stubH, 0))==noErr)
	{				
		// stubH contains a comma separated list.  Parse through it.
		len = GetHandleSize(stubH);
		if (len)
		{
			LDRef(stubH);
			end = *stubH + len;
				
			// start at the beginning of the comma separated line
			scan = begin = *stubH;
			while ((*scan != ',') && (scan < end)) scan++;
			len = scan - begin;
			if ((scan < end) && (len <= bufSize))
			{
				// persID
				*scan = 0;
				strcpy(buf, begin);
				stub->persID = atol(buf);
				
				begin = scan + 1;
				while ((*scan != ',') && (scan < end)) scan++;
				len = scan - begin;
				if ((scan < end) && (len <= bufSize))
				{
					// uid
					*scan = 0;
					strcpy(buf, begin);
					stub->uid = atol(buf);
					
					begin = scan + 1;
					while ((*scan != ',') && (scan < end)) scan++;
					len = scan - begin;
					if ((scan < end) && (len <= bufSize))
					{
						// section string
						*scan = 0;
						strcpy(stub->section, begin);
						
						begin = scan + 1;
						while ((*scan != ',') && (scan < end)) scan++;
						len = scan - begin;
						if ((scan < end) && (len <= bufSize))
						{
							// sizeBytes
							*scan = 0;
							strcpy(buf, begin);
							stub->sizeBytes = atol(buf);
							
							begin = scan + 1;
							while ((*scan != ',') && (scan < end)) scan++;
							len = scan - begin;
							if ((scan < end) && (len <= bufSize))
							{
								// sizeLines
								*scan = 0;
								strcpy(buf, begin);
								stub->sizeLines = atol(buf);		
							}
						}
					}
				}
			}			
			UL(stubH);
		}
		
		// Cleanup
		ZapHandle(stubH);		
	}
	
	return (err);
}

/************************************************************************
 * PETEHandleToMailboxNode - turn a PETEHandle into a MailboxNode
 ************************************************************************/
MailboxNodeHandle PETEHandleToMailboxNode(PETEHandle pte)
{
	MyWindowPtr win = nil;
	TOCHandle tocH = nil;
	MailboxNodeHandle node = nil;
	short kind = 0;
	TOCHandle	realTOC;
	short		realSumNum;
					
	if (win = (*PeteExtra(pte))->win)
	{
		kind = GetWindowKind(GetMyWindowWindowPtr(win));
		
		if (kind == MESS_WIN) tocH = (*Win2MessH(win))->tocH;
		else if (kind == MBOX_WIN) tocH = (TOCHandle)GetMyWindowPrivateData(win);
		
		// get the real TOC in case this message is in a search window.
		realTOC = GetRealTOC(tocH,0,&realSumNum);
		if (realTOC)
		{	
			node = TOCToMbox(realTOC);
		}
	}
	
	return (node);
}	 			

/************************************************************************
 * FetchIMAPAttachment - Given a spec and a pte, download the attachment.
 *	Also update the spec paramter to point to the downloaded attachment.
 ************************************************************************/
OSErr FetchIMAPAttachment(PETEHandle pte, FSSpecPtr spec, Boolean forceForeground)
{
	OSErr err = noErr;
	AttachmentStubStruct stub;
	TOCHandle mailboxTocH = nil;
	long sumNum;
	MailboxNodeHandle mailbox = PETEHandleToMailboxNode(pte);
	long fileId;
	CInfoPBRec	hfi;
			
	// must have a stub file spec ...
	if (!spec || !IsIMAPAttachmentStub(spec)) return (paramErr);

	// must have a valid IMAP mailbox ...
	if (!mailbox) return (errNotIMAPMailboxErr); 
	
	// read the attachment info from the stub file
	if ((err=GetStubInfo(spec, &stub))==noErr)
	{
		// which toc are we dealing with?
		LockMailboxNodeHandle(mailbox);
		mailboxTocH = FindTOC(&(*mailbox)->mailboxSpec);
		UnlockMailboxNodeHandle(mailbox);
		
		if (mailboxTocH)
		{
			// which message owns this attachment?
			if ((err=TOCFindMessByMID(stub.uid, mailboxTocH, &sumNum))==noErr)
			{
				// remember where the stub file lives
				err = HGetCatInfo(spec->vRefNum, spec->parID, spec->name, &hfi);
				fileId = hfi.hFileInfo.ioDirID;
				
				// download the stub
				if ((err==noErr) && !DownloadIMAPAttachment(spec, mailbox, true)) err = errIMAPCouldNotFetchPart;
				
				// Update the spec to point ot the downloaded attachment
				if (err == noErr)
				{
					err = FSResolveFID(spec->vRefNum, fileId, spec);
				}
			}
		}
	}
	
	return (err);
}


/************************************************************************
 *	DownloadAttachmentToSpoolFile - download a given section to a spool
 *	 file, performing no decoding.  Special case for AppleDouble.
 ************************************************************************/
Boolean DownloadAttachmentToSpoolFile(IMAPStreamPtr imapStream, IMAPBODY *parentBody, unsigned long uid, char *parentSection, char *sectionToFetch)
{
	Str255 pSection, section, previousSection, boundarySection;
	IMAPBODY *previousBody = nil;
	Boolean result = false;
	Boolean topLevel = true;
	PART *part = nil;
	IMAPBODY *body = nil;
	Str255 scratch;
	short partNum = 0;
	Str255 attachment, appledouble;
	Boolean fetched = false;
		
	// must have a section to download
	if (!sectionToFetch || !*sectionToFetch) return (false);
	
	// Make sure we have an imap stream and we've SELECTed a mailbox.
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (false);

	// special case if we're fetching an Appledoubled attachment
	GetRString(appledouble,MIME_APPLEDOUBLE);

	// figure out if this is the top level or not
	if (parentSection == nil) topLevel = true;
	else if (*parentSection == nil) topLevel = true;
	else topLevel = false;
	
	// first get the message body
	if (!parentBody) parentBody = UIDFetchStructure(imapStream, uid);
	
	// now locate the section we want to download
	if (parentBody)
	{	
		// Fetch and save the header
		if (!topLevel || (result=UIDFetchHeader(imapStream, uid, true)))
		{		
			// Loop through all parts:
			part = parentBody->nested.part;
			while (part && !fetched)
			{
				// save the previous body and section
				strcpy(previousSection, section);
				previousBody = body;
				
				body = &(part->body);
							
				// Is this the first part??
				if (topLevel)
				{
					NumToString(++partNum,pSection);	// no dot
					PtoCcpy(section, pSection);
				}
				else
				{
					// Copy parent section first. MUST have a non-NULL parent section if not top level!
					ComposeString(pSection, "\p%s.%d", parentSection, ++partNum);
					PtoCcpy(section, pSection);
				}
	
				// if this is an attachment ...
				PtoCcpy(attachment,GetRString(scratch, ATTACH));
				if (body->disposition.type && !striscmp(attachment, body->disposition.type))
				{
					// and it's the section we're looking for
					if (!strcmp(sectionToFetch, section))
					{
						// turn on progress
						imapStream->mailStream->showProgress = !IMAPFilteringUnderway();
						
						// grab the previous part first if we're doing AppleDouble.
						if (!pstrincmp(parentBody->subtype, appledouble,appledouble[0]))
						{
							// Some versions of OE don't properly label the appledouble section as an attachment.  
							// Make sure we don't write the boundary twice 
							if (striscmp(boundarySection, previousSection)) IMAPWriteBoundary(imapStream, parentBody, uid, previousSection, false);
							result = AppendBodyTextToSpoolFile(imapStream, uid, previousSection, previousBody->size.bytes);
						}
					
						// grab the part.
						IMAPWriteBoundary(imapStream, parentBody, uid, section, false);	
						result = AppendBodyTextToSpoolFile(imapStream, uid, section, body->size.bytes);
						
						// done with progress ...
						imapStream->mailStream->showProgress = false;	
									
						// we got it.
						if (result) fetched = true;
					}
				}
				else 
				{
					strcpy(boundarySection, section);
					IMAPWriteBoundary(imapStream, parentBody, uid, section, false);	
				}
										
				// if this is a multipart part, and we haven't fetched the section yet, we may have to recurse.
				if (!fetched && (body->type == TYPEMULTIPART)) fetched = DownloadAttachmentToSpoolFile(imapStream, body, uid, section, sectionToFetch);
							
				// Next part.
				part = part->next;
			}	

			// write outer boundary
			IMAPWriteBoundary(imapStream, parentBody, uid, nil, true);	
		}
	}
	
	return (fetched);
}

/************************************************************************
 *	SpoolFileToAttachment - convert a raw downloaded IMAP messages into 
 *	 an attachment.  Delete the spool file when done.
 *	This is absolutely, positively, not thread safe.
 ************************************************************************/
Boolean SpoolFileToAttachment(MailboxNodeHandle mailbox, unsigned long uid, FSSpecPtr spoolSpec, FSSpecPtr attachSpec)
{
	TransVector	saveCurTrans = CurTrans;
	short 		saveRefN=CurResFile();
	static TransVector IMAPTrans = {nil,nil,nil,nil,nil,nil,nil,nil,nil,IMAPRecvLine,nil};
	TOCHandle toTocH = nil;
	unsigned long messNum = 0;
	HeaderDHandle hdh=NewHeaderDesc(nil);
	short lastHeaderTokenType;
	Str255 buf;	
	short scratchRefN = -1;
	FSSpec scratchSpec;
	Boolean progressed = false;
	long ticks = TickCount();
	static Boolean inUse = false;
	
	if (!hdh)
	{
		IMAPError(kIMAPFetchAttachment, kIMAPMemErr, MemError());
		return (false);
	}
	
	// the decoder routines aren't currently threadsafe.  The quick and dirty thing to do
	// is to only allow one attachment decode at a time.
	while (inUse && !CommandPeriod)
	{
		// put up a progress message if we've waiting for more than a second ...
		if (!progressed && ((TickCount() - ticks)>60))
		{
			PROGRESS_MESSAGER(kpMessage,IMAP_WAITING_FOR_DECODER);
			progressed = true;
		}
								
		CycleBalls();
		if (MyYieldToAnyThread()) break;
	}
	
	inUse = true;
		
	// must have a spool file of some length
	if ((gIMAPMsgEnd = FSpDFSize(spoolSpec)) <= 0) 
	{
		inUse = false;
		return (false);	
	}

	// must also have a scratch file
	if ((scratchRefN = NewScratchFile(spoolSpec, &scratchSpec))==-1) 
	{
		inUse = false;
		return (false);
	}

	// clear out the attachment's spec's name
	attachSpec->name[0] = 0;
		
	// convert the uid to a messNum
	LockMailboxNodeHandle(mailbox);
	toTocH = TOCBySpec(&(*mailbox)->mailboxSpec);
	UnlockMailboxNodeHandle(mailbox);
	if (!toTocH) 
	{
		inUse = false;
		return (false);
	}
	for (messNum = 0; (messNum < (*toTocH)->count) && (uid!=(*toTocH)->sums[messNum].uidHash); messNum++);
	
	/*
	 * allocate the lineio pointer
	 */
	if (!Lip) Lip = NuPtrClear(sizeof(*Lip));
	if (!Lip)
	{
		(WarnUser(MEM_ERR,MemError()));
		goto msgDone;
	}
	if (FSpOpenLine(spoolSpec,fsRdWrPerm,Lip))
		goto msgDone;

	
	// grab the message from the spool file
	CurTrans = IMAPTrans;

	BadBinHex = False;
	BadEncoding = 0;
	if (!AttachedFiles) AttachedFiles=NuHandle(0);
	SetHandleBig_(AttachedFiles,0);
			
	SeekLine(0,Lip);
	lastHeaderTokenType = ReadHeader(nil,hdh,0,scratchRefN,False);
	if (lastHeaderTokenType==EndOfHeader)
	{	
		NoAttachments = false;
		ReadEitherBody(nil,scratchRefN,hdh,buf,sizeof(buf),0,EMSF_ON_ARRIVAL);

		// did we decode an attachment?  Our scratch file will contain the information we need.
		FindFirstAttachmentInSpec(&scratchSpec, attachSpec);
		
		SetHandleBig_(AttachedFiles,0);
		SaveAbomination(nil,0);
	}			
	CurTrans = saveCurTrans;

msgDone:
	if (Lip && Lip->refN)
	{
		CloseLine(Lip);
		DisposePtr((Ptr)Lip);
		Lip = nil;
	}

	// we're done with decoding.
	inUse = false;
		
	//	Don't need spool file anymore
	FSpDelete(spoolSpec);
	
	// Nor do we need the scratch file
	DeleteScratchFile(scratchRefN, &scratchSpec);
	
	UseResFile(saveRefN);
	
	return ((attachSpec->name[0] != 0) && !BadBinHex && (BadEncoding==0));
}	

/************************************************************************
 * IMAPRecvLine - read a line at a time from the spool file. Returns ".\015"
 * at the ends of messages.
 ************************************************************************/
static OSErr IMAPRecvLine(TransStream stream, UPtr buffer, long *size)
{
#pragma unused(stream)
	static Boolean wasFrom;
	static Boolean wasNl=True;
	short lineType;
	
	if (!buffer) {Boolean retVal = wasFrom; wasFrom = False; return(retVal);}
	wasFrom=False;
	(*size)--;

	lineType = GetLine(buffer,*size,size,Lip);
	if (*buffer=='\012')
	{
		//	remove linefeed char
		BMD(buffer+1,buffer,*size-1);		
		(*size)--;
		buffer[*size] = 0;
	}
	if (!*size || !lineType || wasNl&&(wasFrom=IsFromLine(buffer)) || TellLine(Lip)>=gIMAPMsgEnd)
	{
		//	signal end-of-message
		*size = 2;
		buffer[0]='.'; buffer[1]='\015'; buffer[2]=0;
	}
	else if (lineType && wasNl && *buffer=='.')
	{
		//	insert '.' at beginning of line
		BMD(buffer,buffer+1,*size);
		(*size)++;
		*buffer = '.';
		buffer[*size] = 0;
	}
	wasNl = !lineType || buffer[*size-1]=='\015';
	return(noErr);
}

/************************************************************************
 * NewScratchFile - create a scratch file
 ************************************************************************/
short NewScratchFile(FSSpecPtr where, FSSpecPtr scratchFile)
{
	short ref = -1;
	
	SimpleMakeFSSpec(where->vRefNum, where->parID, where->name, scratchFile);
	if (UniqueSpec(scratchFile,31)==noErr)
	{
		if (HCreate(scratchFile->vRefNum,scratchFile->parID,scratchFile->name,CREATOR,'TEXT')==noErr)
		{
			if (AFSHOpen(scratchFile->name,scratchFile->vRefNum,scratchFile->parID,&ref,fsRdWrPerm)!=noErr)
			{
				ref = -1;
				FSpDelete(scratchFile);
			}
		}
	}
	
	return (ref);
}

/************************************************************************
 * NewScratchFile - create a scratch file
 ************************************************************************/
OSErr DeleteScratchFile(short ref, FSSpecPtr scratchFile)
{
	FSClose(ref);
	return (FSpDelete(scratchFile));
}

/************************************************************************
 * FindFirstAttachmentInSpec - find the first attachment converted line
 *	in a file
 ************************************************************************/
void FindFirstAttachmentInSpec(FSSpecPtr inSpec, FSSpecPtr attachSpec)
{
	OSErr err = noErr;
	UHandle text = nil;
	
	Zero(*attachSpec);
	
	if ((err=Snarf(inSpec, &text, 0))==noErr)
	{			
		if (text) FindAnAttachment(text,0,attachSpec,true,nil,nil,nil);

		ZapHandle(text);		
	}
}

/************************************************************************
 * CanFetchAttachment - return true if this attachment can be fetched.
 ************************************************************************/
Boolean CanFetchAttachment(FSSpecPtr spec)
{
	Boolean result = false;
	threadDataHandle index;
	short ascan;
	SignedByte state;
		
	if (spec && spec->name[0])
	{
		if (IsIMAPAttachmentStub(spec))
		{
			result = true;
			
			// are there any threads currently operating on this stub?
			for (index=gThreadData;index && result;index=(*index)->next)
			{
				state = HGetState((Handle)index);
				LDRef(index);
				// scan through this thread's attachments specs
				if ((*index)->imapInfo.attachments)
				{
					LDRef((*index)->imapInfo.attachments);
					for (ascan = 0; result && ascan < (GetHandleSize((*index)->imapInfo.attachments)/sizeof(FSSpec)); ascan++)
					{
						if (SameSpec(spec, &(*((*index)->imapInfo.attachments))[ascan])) result = false;

					}
					UL((*index)->imapInfo.attachments);
				}
				HSetState((Handle)index,state);
			}
		}
	}
	
	return (result);
}

/************************************************************************
 * UpdateIMAPWindows - scan through the gWindowUpdates and update
 *	windows as needed.
 ************************************************************************/
void UpdateIMAPWindows(void)
{
	UpdateNodeHandle node = gWindowUpdates;
	UpdateNodeHandle next;
	TOCHandle tocH = nil;
	long sumNum;
	MyWindowPtr win = nil;
	MessHandle messH = nil;
	WindowPtr	winWP;
	TOCHandle searchTocH;
	short searchSum;
			
	// go through the list of nodes ...
	while (node)
	{
		next = (*node)->next;
		
		// remove the node from the list
		LL_Remove(gWindowUpdates, node, (UpdateNodeHandle));
				
		// Which mailbox does this message live in?
		LDRef(node);
		tocH =  TOCBySpec(&(*node)->mailboxSpec);
		
		if (tocH)	// the toc is still open ...
		{				
			// which message needs to be updated?
			if (TOCFindMessByMID((*node)->uid,tocH,&sumNum) == noErr)
			{
				// update the message's summary.
				if (HasStubFileAttachment(tocH, sumNum)) (*tocH)->sums[sumNum].opts |= OPT_FETCH_ATTACHMENTS;
				else (*tocH)->sums[sumNum].opts &= ~OPT_FETCH_ATTACHMENTS;
				
				// redraw the server column
				InvalTocBox(tocH,sumNum,blServer);
				
				// is there a message handle associated with this message?
				if (messH = (*tocH)->sums[sumNum].messH)
				{
					// is the message open?
					if (win = (*messH)->win)
					{			
						// is the message visible?
						if (IsWindowVisible(GetMyWindowWindowPtr(win)))
						{
							// then refresh the message
							if (!RedoIMAPAttachmentIcons(win, nil, (*node)->attachSpecs))
							{
								// reopen the window if there was no attachment info, or if we failed.
								RedisplayIMAPMessage(win);
							}
						}
					}
				}	
				
				// If this message is being previewed, cause it to be redisplayed
				if ((*tocH)->previewID==(*tocH)->sums[sumNum].serialNum)
				{
					if (!RedoIMAPAttachmentIcons(nil, (*tocH)->previewPTE, (*node)->attachSpecs))
						(*tocH)->previewID = 0;
				}
				
				// if this message is being previewed in a search window, cause it to be redisplayed.
				for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
				{
					// consider all open search windows ...
					if (IsSearchWindow(winWP))
					{
						GetSearchTOC(GetWindowMyWindowPtr(winWP),&searchTocH);
						{
							// is this search window previewing a message?
							if ((*searchTocH)->previewID!=0)
							{
								// see if it's the same message as the one we're updating
								searchSum = FindSumBySerialNum(searchTocH, (*searchTocH)->previewID);
								if (searchSum < (*searchTocH)->count)
								{
									if ((*tocH)->sums[sumNum].uidHash == (*searchTocH)->sums[searchSum].uidHash)
									{
										// redisplay it
										if (!RedoIMAPAttachmentIcons(nil, (*searchTocH)->previewPTE, (*node)->attachSpecs))
											(*searchTocH)->previewID = 0;
									}
								}
							}
						}
					}
				}
		
			}
		}
		
		UL(node);	
		
		// Cleanup
		ZapHandle((*node)->attachSpecs);
		ZapHandle(node);
		
		// move on to the next node in the list
		node = next;
	}
}

/************************************************************************
 * RedoIMAPAttachmentIcons - make new graphics for the specified
 *	imap attachments
 ************************************************************************/
Boolean RedoIMAPAttachmentIcons(MyWindowPtr win, PETEHandle previewPte, FSSpecHandle attachSpecs)
{
	WindowPtr winWP = nil; 
	short attachCount;
	Handle textH;
	UPtr begin, end;						/* beginning and end of all text */
	UPtr lBegin, lEnd;					/* beginning and end of current line */
	Str255 line;
	Byte state;
	Boolean found = false;
	FSSpec spec, curAtt;
	short i;
	long offset;
	PETEHandle pte = previewPte;
	
	// must have a window or a pte to update
	if (!win && !previewPte) return (false);
	
	// update the window if the window was passed in
	if (win) 
	{
		winWP = GetMyWindowWindowPtr(win);
		pte = win->pte;
	}
	
	// must have some attachments to update
	if (!attachSpecs || !*attachSpecs) return(false);
	
	attachCount = GetHandleSize_(attachSpecs)/sizeof(FSSpec);
	if (!attachCount) return (false);

	// must have a valid window
	if (!PeteIsValid(pte)) return(false);
	
	if (PeteGetTextAndSelection(pte,&textH,nil,nil)) return(False);
	if (!textH || !GetHandleSize(textH)) return(false);
	state = HGetState(textH);
	
	// set initial values
	begin = LDRef(textH);
	end = begin + GetHandleSize(textH);
	
	// examine each line
	for (lBegin=begin;lBegin<end;lBegin=lEnd+1)
	{
		for (lEnd=lBegin;lEnd<end && lEnd[0]!='\015';lEnd++);
		MakePStr(line,lBegin,lEnd-lBegin);
		if (!AttLine2Spec(line,&spec,false))
		{
			offset = lBegin-begin;
			for (i = 0; i < attachCount; i++)
			{
				curAtt = (*attachSpecs)[i];

				if (SameSpec(&curAtt, &spec))
				{
					if (FileGraphicChangeGraphic(pte, offset,&curAtt)==noErr)
						found = true;
				}
			}
		}
	}
	
	// rezoom the window
	if (winWP && found && PrefIsSet(PREF_ZOOM_OPEN)) 
		ReZoomMyWindow(winWP);
	
	HSetState(textH,state);
	return (found);
}

/************************************************************************
 * RedisplayIMAPMessage - Redisplay an IMAP message that has just come in
 ************************************************************************/
void RedisplayIMAPMessage(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	ControlHandle blah;
	MessHandle messH;
	PETETextStyle style;
	Rect	rState;
	
	SetPort_(GetWindowPort(winWP));
	rState = CurState(winWP);	//	Make sure we rezoom correctly
	SetWindowStandardState(winWP,&rState);

	// Enable, but don't show, the GetGraphics control, 
	// in case this message has downloadable images
	blah = FindControlByRefCon(win,mcGetGraphics);
	if (blah)  HiliteControl(blah,0);
	
	// Redisplay the message
	if (ReopenMessage(win)!=nil)
	{
		// properly enable the message icon buttons
		messH = (MessHandle)GetMyWindowPrivateData(win);
		if (messH) MessIBarUpdate(messH);
		EnableMsgButtons(win,true);
		
		// Rezoom the window
		if (PrefIsSet(PREF_ZOOM_OPEN)) ReZoomMyWindow(winWP);
		else
		{
			// fake the window out so it redraws itself.
			Rect	rPort;
		 	MyWindowDidResize(win,nil);
			InvalWindowRect(winWP,GetPortBounds(GetWindowPort(winWP),&rPort));
			UpdateMyWindow(winWP);
		}
		
		// don't allow changes to the body yet
       	style.tsLock = peModLock;
      	PETESetTextStyle(PETE,TheBody,0L,0x7fffffff,&style,peLockValid);
      	PETEMarkDocDirty(PETE,TheBody,False);
		win->isDirty = False;
	}					
}

/**********************************************************************
 * FetchAllIMAPAttachments - go get the IMAP attachments for a message
 **********************************************************************/
Boolean FetchAllIMAPAttachments(TOCHandle tocH, short sumNum, Boolean forceForeground)
{
	Boolean result = true;
	MyWindowPtr win = nil;
	WindowPtr	theWindow;
	long offset;
	Handle text;
	FSSpec attachSpec;
	MailboxNodeHandle mailbox;
	PersHandle pers;
	FSSpecHandle attachments = nil;
	short numSpecs = 0;
	OSErr err = noErr;
	MessHandle messH = nil;
	Boolean openedMessage = false;
				
	// must have a tocH
	if (!tocH) return (false);
	
	// the toc must refer to an IMAP mailbox
	mailbox = TOCToMbox(tocH);
	pers = TOCToPers(tocH);	
	if (pers && mailbox)
	{	
		// if the message is already open, nothing to do.		
		if (messH = (*tocH)->sums[sumNum].messH) 
		{
			win = (*messH)->win;
			UsingWindow(GetMyWindowWindowPtr(win));
		}

		// if the message is not open, open it.
		if (win == nil) 
		{
			win = GetAMessage(tocH,sumNum,nil,nil,false);
			openedMessage = true;
		}
				
		if (win)
		{
			theWindow = GetMyWindowWindowPtr (win);
			CacheMessage(tocH,sumNum);
			if (!(text=(*tocH)->sums[sumNum].cache)) return false;
			HNoPurge(text);
			offset = (*tocH)->sums[sumNum].bodyOffset-1;
			
			// scan message for attachments
			while ((0<=(offset = FindAnAttachment(text,offset+1,&attachSpec,true,nil,nil,nil))))
			{	
				// did we find an attachment stub?
				if (IsIMAPAttachmentStub(&attachSpec))
				{
					// can we download it?
					if (CanFetchAttachment(&attachSpec))
					{
						// add it to the list of attachments to fetch
						if (attachments == nil) 
						{
							attachments = NuHandle(sizeof(FSSpec));
							numSpecs = 1;
						}
						else
						{
							numSpecs++;
							SetHandleSize(attachments, numSpecs*sizeof(FSSpec));
						}
						
						if (!attachments || (err=MemError()))
						{
							WarnUser(MEM_ERR, err);
							ZapHandle(attachments);
							break;
						}
						else
						{
							BMD(&attachSpec, &(*attachments)[numSpecs-1], sizeof(FSSpec));
						}
					}
				}
			}
			
			// now fetch the attachments
			if (attachments) result = (DownloadIMAPAttachments(attachments, mailbox, forceForeground) > 0);
			
			// cleanup
			if (openedMessage && !IsWindowVisible (theWindow))
				CloseMyWindow(theWindow);
			else
				NotUsingWindow(theWindow);
		}
	}
					
	return (result);
}

/**********************************************************************
 * FetchAllIMAPAttachmentsBySpec - given a spec, fetch all the IMAP
 *	attachments in the message that has this spec as a stub
 **********************************************************************/
Boolean FetchAllIMAPAttachmentsBySpec(FSSpecPtr spec, MailboxNodeHandle mailbox, Boolean forceForeground)
{
	Boolean result = false;
	OSErr err = noErr;
	AttachmentStubStruct stub;
	TOCHandle mailboxTocH = nil;
	long sumNum;
		
	// must have a stub file spec ...
	if (!spec || !IsIMAPAttachmentStub(spec)) return false;
	
	// must have a mailbox
	if (!mailbox) return (false);

	// read the attachment info from the stub file
	if ((err=GetStubInfo(spec, &stub))==noErr)
	{
		// which toc are we dealing with?
		LockMailboxNodeHandle(mailbox);
		mailboxTocH = FindTOC(&(*mailbox)->mailboxSpec);
		UnlockMailboxNodeHandle(mailbox);
		
		if (mailboxTocH)
		{
			// which message owns this attachment?
			if ((err=TOCFindMessByMID(stub.uid, mailboxTocH, &sumNum))==noErr)
			{
				result = FetchAllIMAPAttachments(mailboxTocH, sumNum, forceForeground);
			}
		}
	}
	
	return (result);
}

/**********************************************************************
 * HasStubFileAttachment - return true if this message has one or more
 *	stub files as attachments
 **********************************************************************/
Boolean HasStubFileAttachment(TOCHandle tocH, short sumNum)
{
	Boolean result = false;
	MailboxNodeHandle mailbox;
	PersHandle pers;
	MessHandle messH = nil;
	MyWindowPtr win = nil;
	WindowPtr	winWP;
	Boolean openedMessage = false;
	Handle text;
	long offset;
	FSSpec attachSpec;
				
	// must have a tocH
	if (!tocH) return (false);
	
	// the toc must refer to an IMAP mailbox
	mailbox = TOCToMbox(tocH);
	pers = TOCToPers(tocH);	
	if (pers && mailbox)
	{	
		// if the message is already open, nothing to do.		
		if (messH = (*tocH)->sums[sumNum].messH) 
		{
			win = (*messH)->win;
			UsingWindow(GetMyWindowWindowPtr(win));
		}

		// if the message is not open, open it.
		if (win == nil) 
		{
			win = GetAMessage(tocH,sumNum,nil,nil,false);
			openedMessage = true;
		}
				
		if (win)
		{
			winWP = GetMyWindowWindowPtr (win);
			CacheMessage(tocH,sumNum);
			if (!(text=(*tocH)->sums[sumNum].cache)) return false;
			HNoPurge(text);
			offset = (*tocH)->sums[sumNum].bodyOffset-1;
			
			// scan message for attachments
			while (0<=(offset = FindAnAttachment(text,offset+1,&attachSpec,true,nil,nil,nil)))
			{
				// did we find an attachment stub?
				if (IsIMAPAttachmentStub(&attachSpec))
				{
					// then one of the attachments is a stub file.
					result = true;
					break;
				}
			}
			
			// cleanup
			if (openedMessage && !IsWindowVisible (winWP))
				CloseMyWindow(winWP);
			else
				NotUsingWindow(winWP);
		}
	}
					
	return (result);
}

/**********************************************************************
 * IMAPSearch - Given a list of mailboxes and search criteria, perform
 *	the search.  This may involve downloading summaries.
 **********************************************************************/
Boolean IMAPSearch(TOCHandle searchWin, BoxCountHandle boxesToSearch, IMAPSCHandle searchCriteria, Boolean matchAll)
{
	OSErr err = noErr;
	PersHandle pers, mboxPers;
	MailboxNodeHandle mailbox;
	short i, menuId, numBoxes = GetHandleSize_(boxesToSearch)/sizeof(BoxCountElem), numBoxesToSearch;
	FSSpec spec;
	Handle toSearch = nil;
 	DeliveryNodeHandle node = nil;
	short numStarted = 0;			// number of searches that were actually started
			
	// must have some mailboxes to search
	if (!boxesToSearch || !*boxesToSearch) return (false);
	
	// must thave some search criteria
	if (!searchCriteria || !*searchCriteria) return (false);
	
	// set up the deliveryNode that the results will be returned in
	
	// is there one already set up?
	if (node = FindNodeByToc(searchWin)) return (false);
				
	// Search each personality as needed.
	for (pers = PersList; pers && (err==noErr); pers = (*pers)->next)
	{
		// create a handle to store the indices into boxesToSearch
		if (!(toSearch = NuHandle(0)) || (err = MemError()))
		{
			WarnUser(MEM_ERR,err);
			return (false);
		}

		for (i = 0; i < numBoxes; i++)
		{
			spec.parID = (*boxesToSearch)[i].dirId;
			spec.vRefNum = (*boxesToSearch)[i].vRef;
			menuId = (spec.parID==MailRoot.dirId && SameVRef(spec.vRefNum,MailRoot.vRef)) ? MAILBOX_MENU : FindDirLevel(spec.vRefNum,spec.parID);
			MailboxMenuFile(menuId,(*boxesToSearch)[i].item,spec.name);
			
			// is this an IMAP mailbox?
			if (IsIMAPCacheFolder(&spec))
			{
				// then look at the mailbox cache file itself.
				spec.parID = SpecDirId(&spec);
				
				// does this mailbox belong to the current personality?
				LocateNodeBySpecInAllPersTrees(&spec, &mailbox, &mboxPers);
				if (mailbox && (mboxPers == pers))
				{
					// then add it to the list of mailboxes to be searched
					numBoxesToSearch = (GetHandleSize(toSearch)/sizeof(short)) + 1;
					SetHandleSize(toSearch, numBoxesToSearch * sizeof(short));
					if (err=MemError())
					{
						WarnUser(MEM_ERR,err);
						ZapHandle(toSearch);
						return (false);
					}
					BMD(&i, &((short *)(*toSearch))[numBoxesToSearch-1], sizeof(short));
				}
			}
		}
		
		// were any mailboxes belonging to this personality supposed to be searched?
		if (toSearch && (GetHandleSize_(toSearch) > 0))
		{
			// create a delivery node
			if (node == NULL)
			{
				node = NewDeliveryNode(searchWin);
				if (node)
				{	
					(*node)->filter = false;
					
					// queue the delivery node
					QueueDeliveryNode(node);
				}
				else
				{
					WarnUser(MEM_ERR,MemError());
					return (false);
				}
			}
			
			// actually do the search
			if (IMAPSearchServer(searchWin, pers, boxesToSearch, toSearch, searchCriteria, matchAll, 0, (numStarted != 0)))
				numStarted++;
			else
				break;
		}
		else
		{
			ZapHandle(toSearch);
		}
	}
	
	// Cleanup
	ZapHandle(searchCriteria);
	searchCriteria = nil;
	
	// did we fail to start a search?
	if (numStarted == 0)
	{
		// then clean up
		DequeueDeliveryNode(node);
		ZapHandle(node);
	}
	
	// return true if at least one search was started
	return (numStarted != 0);
}

/**********************************************************************
 * IMAPSearch - Given a mailbox and search criteria, perform the search.  
 * 	Start the search from the uid where the caller asks.
 **********************************************************************/
Boolean IMAPSearchMailbox(TOCHandle searchWin, BoxCountHandle allBoxes, MailboxNodeHandle boxToSearch, IMAPSCHandle searchCriteria,  Boolean matchAll, long firstUID)
{
	OSErr err = noErr;
	Handle toSearch = nil;
 	DeliveryNodeHandle node = nil;
	short i, numBoxes;
	short menuId;
	FSSpec spec;
		
	// must have a mailbox to search
	if (!boxToSearch || !*boxToSearch) return (false);
	
	// must thave some search criteria
	if (!searchCriteria || !*searchCriteria) return (false);
	
	// set up the deliveryNode that the results will be returned in
	
	// is there one already set up?
	if (node = FindNodeByToc(searchWin)) return (false);
	
	node = NewDeliveryNode(searchWin);
	if (node)
	{	
		(*node)->filter = false;
		
		// grumble grumble
		numBoxes = GetHandleSize_(allBoxes)/sizeof(BoxCountElem);
		for (i = 0; i < numBoxes; i++)
		{
			spec.parID = (*allBoxes)[i].dirId;
			spec.vRefNum = (*allBoxes)[i].vRef;
			menuId = (spec.parID==MailRoot.dirId && SameVRef(spec.vRefNum,MailRoot.vRef)) ? MAILBOX_MENU : FindDirLevel(spec.vRefNum,spec.parID);
			MailboxMenuFile(menuId,(*allBoxes)[i].item,spec.name);
			spec.parID = SpecDirId(&spec);
			
			if (((*boxToSearch)->mailboxSpec.vRefNum == spec.vRefNum)
			 && ((*boxToSearch)->mailboxSpec.parID == spec.parID))
			 {
				toSearch = NuHandle(sizeof(short));
				if (toSearch)
				{
					*((short *)(*toSearch)) = i;
					break;
			 	} 
			}
		}
		
		if (toSearch)
		{
			// queue the delivery node.
			QueueDeliveryNode(node);
			
			// then do the search
			IMAPSearchServer(searchWin, FindPersById((*boxToSearch)->persId), allBoxes, toSearch, searchCriteria, matchAll, firstUID, false);
		}
		else ZapHandle(node);
	}
	else WarnUser(MEM_ERR,MemError());
	
	// Cleanup
	ZapHandle(searchCriteria);
	searchCriteria = nil;
	
	return ((toSearch != nil));	
}

/**********************************************************************
 * IMAPSearchServer - do a search on a set of mailboxes on the same
 *	server.
 **********************************************************************/
Boolean IMAPSearchServer(TOCHandle searchWin, PersHandle pers, BoxCountHandle boxesToSearch, Handle toSearch, IMAPSCHandle searchCriteria, Boolean matchAll, long firstUID, Boolean bAlreadyOnline)
{
 	MailboxNodeHandle mailboxNode = nil;
	OSErr err = noErr;
	XferFlags flags;
	IMAPTransferRec imapInfo;
	Boolean result = false;
	PersHandle oldPers = CurPers;
	DeliveryNodeHandle delivery = FindNodeByToc(searchWin);
	IMAPSCHandle dupSearchCriteria = nil;
	BoxCountHandle dupBoxes = nil;
	Handle dupSearch = nil;
			
	// picky picky picky.
	if (!delivery || !pers || !searchWin || !toSearch || !searchCriteria) return (false);

	// we must be online
	if (!bAlreadyOnline && Offline && GoOnline()) return(false);
	
	// pass the search thread it's own copy of the search parameters -jdboyd 08/05/04
	dupSearchCriteria = DupHandle((Handle)searchCriteria);
	dupBoxes = DupHandle((Handle)boxesToSearch);
	dupSearch = DupHandle((Handle)toSearch);
	
	if (dupSearchCriteria && dupBoxes && dupSearch)
	{
		if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable())
		{
			result = DoIMAPServerSearch(searchWin, dupBoxes, dupSearch, searchCriteria, matchAll, firstUID);
		}
		else 
		{					
			// collect password now if we need it.
			CurPers = pers;
			err = CheckIMAPSettingsForPers();		

			// if we were given a password, set up the thread
			if (err==noErr)
			{
				Zero(flags);
				Zero(imapInfo);
				imapInfo.destToc = searchWin;
				imapInfo.toSearch = dupSearch;
				imapInfo.boxesToSearch = dupBoxes;
				imapInfo.searchC = dupSearchCriteria;
				imapInfo.matchAll = matchAll;
				imapInfo.firstUID = firstUID;
				imapInfo.command = IMAPSearchTask;
				result = ((err=SetupXferMailThread (false, false, true, false, flags, &imapInfo))==noErr);
				
				// and remove any old task errors
				RemoveTaskErrors(IMAPSearchTask,(*CurPers)->persId);
			
				// forget the keychain password so it's not written to the settings file
				if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) Zero((*CurPers)->password);
			}
			CurPers = oldPers;
		}
	}
	else
	{
		// error.  Cleanup.
		ZapHandle(dupSearchCriteria);
		ZapHandle(dupBoxes);
		ZapHandle(dupSearch);
		WarnUser(MEM_ERR, err=MemError());
	}
		
	return (result);
 }


/**********************************************************************
 * DoIMAPServerSearch - make a connection and do a search on one pers
 **********************************************************************/
Boolean DoIMAPServerSearch(TOCHandle searchWin, BoxCountHandle allBoxes, Handle boxesToSearch, IMAPSCHandle searchCriteria, Boolean matchAll, long firstUID)
{
	Boolean result = false;
	OSErr err = noErr;
	IMAPStreamPtr imapStream;
	PersHandle oldPers = CurPers, boxPers = nil;
	MailboxNodeHandle mboxToSearch = nil;
	Str255 progressMessage, mName;
	short criteriaCount;
	short numCriteria = GetHandleSize_(searchCriteria)/sizeof(IMAPSCStruct);
	short boxCount;
	short numBoxes = GetHandleSize_(boxesToSearch)/sizeof(short);
	Str255 cSearchString, scratch;
	Str255 pHeaders, cHeaders;
	UIDNodeHandle boxSearchResults = nil;
	UIDNodeHandle allScan = nil, boxScan = nil, nextScan = nil;
	UIDNodeHandle allSearchResults = nil;
	Boolean searchTheBody;	
	DeliveryNodeHandle delivery = FindNodeByToc(searchWin);
	FSSpec specToSearch;
	short menuId;
	SignedByte pState;
	unsigned long start, next, last;
	unsigned long chunkLen = GetRLong(IMAP_SEARCH_CHUNK_SIZE);
	
	// must have some place to put the results
	if (!delivery || (*delivery)->aborted) return (false);
	
	// must have a list of all mailboxes
	if (!allBoxes || !*allBoxes) return (false);
	
	// must have a list of mailboxes to search
	if (!boxesToSearch || !*boxesToSearch) return (false);
	
	// must have something to search for
	if (!searchCriteria || !*searchCriteria) return (false);

	// increment the delivery node since we're starting a search thread
	(*delivery)->threadCount++;
			
	// the personalitiy we're searching is the owner of the first mailbox to search ...
	specToSearch.parID = (*allBoxes)[((short *)(*boxesToSearch))[0]].dirId;
	specToSearch.vRefNum = (*allBoxes)[((short *)(*boxesToSearch))[0]].vRef;
	menuId = (specToSearch.parID==MailRoot.dirId && SameVRef(specToSearch.vRefNum,MailRoot.vRef)) ? MAILBOX_MENU : FindDirLevel(specToSearch.vRefNum,specToSearch.parID);
	MailboxMenuFile(menuId,(*allBoxes)[((short *)(*boxesToSearch))[0]].item,specToSearch.name);
	specToSearch.parID = SpecDirId(&specToSearch);
	
	LocateNodeBySpecInAllPersTrees(&specToSearch, &mboxToSearch, &CurPers);
	
	if (CurPers)
	{			
		// display some progress indicating which server we're searching
		pState = HGetState((Handle)CurPers);
		LDRef(CurPers);
		ComposeRString(progressMessage,IMAP_SEARCHING_PERS,(*CurPers)->name);
		HSetState(CurPers, pState);
		PROGRESS_START;
		PROGRESS_MESSAGE(kpTitle,progressMessage);
				
		// Create a new IMAP stream ...
		imapStream = GetIMAPConnection(IMAPSearchTask, CAN_PROGRESS);
		if (imapStream)
		{		
			// open a connection to the server ...
			if (OpenControlStream(imapStream))
			{						
				// iterate through the mailboxes to search
				for (boxCount = 0; (boxCount < numBoxes) && !((*delivery)->aborted); boxCount++)
				{
					// figure out who this mailbox belongs to
					specToSearch.parID = (*allBoxes)[((short *)(*boxesToSearch))[boxCount]].dirId;
					specToSearch.vRefNum = (*allBoxes)[((short *)(*boxesToSearch))[boxCount]].vRef;
					menuId = (specToSearch.parID==MailRoot.dirId && SameVRef(specToSearch.vRefNum,MailRoot.vRef)) ? MAILBOX_MENU : FindDirLevel(specToSearch.vRefNum,specToSearch.parID);
					MailboxMenuFile(menuId,(*allBoxes)[((short *)(*boxesToSearch))[boxCount]].item,specToSearch.name);
					specToSearch.parID = SpecDirId(&specToSearch);
						
					LocateNodeBySpecInAllPersTrees(&specToSearch, &mboxToSearch, &boxPers);
					
					if (mboxToSearch)
					{
						// open the mailbox on the server
						LockMailboxNodeHandle(mboxToSearch);
						if (IMAPOpenMailbox(imapStream, (*mboxToSearch)->mailboxName,false))
						{
							// take this opportunity to update the UIDVALIDITY of the mailbox if it hasn't been resynched before.
							if ((*mboxToSearch)->uidValidity == 0)
							 	(*mboxToSearch)->uidValidity = UIDValidity(imapStream);
				
							UnlockMailboxNodeHandle(mboxToSearch);

							// determine the range of messages ...
							IMAPMailboxUIDRange(imapStream, imapStream->mailStream->nmsgs, &start, &last);
							
							// remember the highest for later use ...
							(*mboxToSearch)->searchUID = last;
							
							// start the search in the appropriate place
							if (firstUID)
								start = firstUID;
							
							// searches will start at the lowest UID in the mailbox, and go to the end
							if (chunkLen) 
							{
								// make sure we got back a valid range.  
								// If we didn't, we can still attempt the search with no chunking.
								if (last < start)
								{
									start = 0;
								}
							}
							else 
							{
								start = last = 0;	// user has requested no chunking
							}
							next = MIN(start + chunkLen, last);
							
							while (!((*delivery)->aborted) && (start <= last))
							{
								// iterate through the criteria, and do one search for each criteria element
								for (criteriaCount = 0; (criteriaCount < numCriteria) && !((*delivery)->aborted); criteriaCount++)
								{
									// make a c-string out of what we're searching for
									PtoCcpy(cSearchString, (*searchCriteria)[criteriaCount].string);
												
									// build the header list to search trhough							
									pHeaders[0] = cHeaders[0] = nil;
									switch ((*searchCriteria)[criteriaCount].headerCombination)
									{							
										case kNone:
										{
											PCopy(pHeaders, GetRString(scratch, (*searchCriteria)[criteriaCount].headerName));
											pHeaders[0]--;	//drop :
										
											PtoCcpy(cHeaders, pHeaders);
											
											break;
										}
										
										case kAllHeaders:
										case kAnyWhere:
										case kAnyRecipient:
										{
											BuildHeaderSearchString(((*searchCriteria)[criteriaCount].headerCombination==kAllHeaders) || ((*searchCriteria)[criteriaCount].headerCombination==kAnyWhere), 
												((*searchCriteria)[criteriaCount].headerCombination==kAnyRecipient), pHeaders);

											PtoCcpy(cHeaders, pHeaders);
											
											break;
										}
										
										default:
											break;
									}
										
									// display some progress indicating which mailbox we're searching
									LockMailboxNodeHandle(mboxToSearch);
									PathToMailboxName ((*mboxToSearch)->mailboxName, mName, (*mboxToSearch)->delimiter);
									UnlockMailboxNodeHandle(mboxToSearch);
									ComposeRString(progressMessage,IMAP_SEARCHING_MAILBOX,mName);
									PROGRESS_MESSAGE(kpSubTitle,progressMessage);
									
									// and do the search.
									searchTheBody = ((*searchCriteria)[criteriaCount].headerCombination == kBody) || ((*searchCriteria)[criteriaCount].headerCombination == kAnyWhere);		
									if (UIDFind(imapStream, cHeaders, searchTheBody, false, cSearchString, start, next, &boxSearchResults))
									{									
										// no results found?
										if (boxSearchResults == nil)
										{								
											// if we're doing a match all, and this criteria failed, 
											// We're done with this cunk.
											if (matchAll) 
											{
												if (allSearchResults) UID_LL_Zap(&allSearchResults);
												goto hell;
											}
										}
										else
										{
											// prepare the search results we just got ...
											for (boxScan = boxSearchResults; boxScan; boxScan = (*boxScan)->next) (*boxScan)->boxIndex = ((short *)(*boxesToSearch))[boxCount];

											// now save the search results
											if (allSearchResults == nil)
											{
												// no results yet.  The results up to this point are the results of the last search
												allSearchResults = boxSearchResults;
												boxSearchResults = nil;
											}
											else
											{
												if (matchAll)
												{
													// match all.  The results are what is common in what we just found and in what we have accumulated
													allScan = allSearchResults;
													while (allScan)
													{
														nextScan = (*allScan)->next;
														
														for (boxScan = boxSearchResults; boxScan; boxScan = (*boxScan)->next)
															if ((*boxScan)->uid == (*allScan)->uid) break;
														
														if (!boxScan)
														{
															boxScan = (*allScan)->next;
															LL_Remove(allSearchResults,allScan,(UIDNodeHandle));
															ZapHandle(allScan);						
														}
														allScan = nextScan;
													}
													
													// done with the last search results
													UID_LL_Zap(&boxSearchResults);
													boxSearchResults = nil;
			
													// if allSearchResults is now empty, stop the search.
													if (allSearchResults == nil) goto hell;
												}
												else
												{
													// match any.  Results are what we have PLUS what we have just found.
													while (boxScan = boxSearchResults)
													{
														boxSearchResults = (*boxScan)->next;
														(*boxScan)->next = nil;
														UID_LL_OrderedInsert(&allSearchResults, &boxScan, false);
													}
												}
											}
										}		
									}	// end single mailbox, single criteria search	
									else // the search failed
										if (CommandPeriod) AbortDeliveryNode(delivery);
															
								}	// end criteria loop	
								
								// return the search results for this mailbox now.  This clears allSearchResults.
								if (allSearchResults) ReturnSearchHits(imapStream, delivery, &specToSearch, &allSearchResults);
							
hell:								
								start = next+1;
								next = MIN(start + chunkLen, last);
							}	// end chunk loop
						}	// end open mailbox
						else 
							UnlockMailboxNodeHandle(mboxToSearch);
					}
				}	// end box loop			
				PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);				
			}	// end connection
		}
		else
		{
			WarnUser(MEM_ERR, MemError());
		}
		
		CleanupConnection(&imapStream);
		
		PROGRESS_END;	
	}
	
	// aborted? decrement the thread count.  
	// Other threads will abort, and node will be removed at idle time from main thread
	if (delivery && ((*delivery)->aborted || CommandPeriod))
		(*delivery)->threadCount--;	
	else
	{	
		(*delivery)->threadCount--;
		if ((*delivery)->threadCount <= 0) (*delivery)->finished = true;
	}
						
	// cleanup
	UID_LL_Zap(&allSearchResults);
	allSearchResults = nil;
	
	// dispose of the search criteria
	ZapHandle(allBoxes);
	ZapHandle (boxesToSearch);
	ZapHandle(searchCriteria);
	
	// dispose of the list of boxes we were asked to search
	ZapHandle(boxesToSearch);
	boxesToSearch = nil;
	
	// dispose of our own copy of all the boxes to search
	// uhhhh ... no.  SearchWin depends on this list later! -JDB
	//ZapHandle(allBoxes);
	//allBoxes = nil;
	
	CurPers = oldPers;
	return (result);
}

/**********************************************************************
 * IMAPMailboxUIDRange - look at the selected mailbox, and
 *	return the UID of the first and the last message
 **********************************************************************/
void IMAPMailboxUIDRange(IMAPStreamPtr imapStream, unsigned long numMessages, unsigned long *first, unsigned long *last)
{
	*first = *last = 0;		//default
	
	*first = FetchUID(imapStream, 1);
	*last = FetchUID(imapStream, numMessages);
}

/**********************************************************************
 * IMAPMailboxChanged - a message has been added to an IMAP 
 *	mailbox.
 **********************************************************************/
void IMAPMailboxChanged(MailboxNodeHandle mbox)
{
	if (!PrefIsSet(PREF_NO_LIVE_SEARCHES))
	{
		SetIMAPMailboxNeeds(mbox, kNeedsSearch, true);
		gUpdateIncrementalSearches = true;	
	}
}

/**********************************************************************
 * UpdateIncrementalIMAPSearches - go through all IMAP mailboxes that
 *	have changed recently and make sure all live searches are updated
 **********************************************************************/
void UpdateIncrementalIMAPSearches(void)
{
	if (gUpdateIncrementalSearches)
	{
		if (!PrefIsSet(PREF_NO_LIVE_SEARCHES) && !GetNumBackgroundThreads() && !IMAPFilteringUnderway())
		{
			IMAPUpdateIncrementalSearches();
			gUpdateIncrementalSearches = false;	
		}
	}
}

/**********************************************************************
 * ReturnSearchHits - given a UIDNode list, return the results
 *	to the main thread.
 *
 *	- pull out all UIDs that exist locally, and return them.
 *	- fetch minimal headers of those that don't
 *	- wait for them to download, and then return them.
 **********************************************************************/
Boolean ReturnSearchHits(IMAPStreamPtr imapStream, DeliveryNodeHandle searchNode, FSSpecPtr spec, UIDNodeHandle *uids)
{
	Boolean result = false;
	DeliveryNodeHandle deliveryNode;
	TOCHandle tocToSync = TOCBySpec(spec);
	MailboxNodeHandle mailbox = nil;
	UIDNodeHandle uidsToFetch = nil, node, next;
	long sumNum;
	TOCHandle hidTocH;
	
	// must have a TOC to sync
	if (!tocToSync) return (false);
	
	// must have a delivery node for the search results
	if (!searchNode) return (false);
	
	// must have some UIDs to fetch
	if (!uids || !*uids) return (true);

	// if there's already a delivery node set up for this TOC, then the resync is already underway.  Just return all the results
	if (tocToSync && FindNodeByToc(tocToSync))
	{
		BuildSearchResults(searchNode, *uids);
		UID_LL_Zap(uids);
		*uids = nil;	
		return (true);
	}
		
	// must be connected and have a mailbox selected
	if (!imapStream || !IsConnected(imapStream->mailStream) || !IsSelected(imapStream->mailStream)) return (false);
	
	// UIDFetchMessages needs to have the mailbox information
	mailbox = TOCToMbox(tocToSync);
	if (!mailbox)
		return (false);
	
	// may have to consider locally hidden toc entries
	hidTocH = GetHiddenCacheMailbox(mailbox, false, false);
			
	// build a list of uids to fetch
	next = nil;
	for (node = *uids; node; node = next)
	{
		next = (*node)->next;
		
		// if the summary can't be found ...
		if ((TOCFindMessByMID((*node)->uid,tocToSync,&sumNum) != noErr)
	     && (!hidTocH || (TOCFindMessByMID((*node)->uid,hidTocH,&sumNum) != noErr)))
		{
			// remove this node from the search results ...
			LL_Remove(*uids, node, (UIDNodeHandle));
			(*node)->next = nil;
			
			// and put it in the list of uids to fetch.
			LL_Queue(uidsToFetch, node, (UIDNodeHandle));
		}
	}

	// Return the remaining results.  These messages exist locally.
	if (*uids && **uids)
	{
		BuildSearchResults(searchNode, *uids);
		UID_LL_Zap(uids);
		*uids = nil;
	}
									
	// go fetch the missing summaries
	if (uidsToFetch && *uidsToFetch)
	{
		// set up the structure that the summaries will be returned in.
		deliveryNode = NewDeliveryNode(tocToSync);
		if (deliveryNode)
		{		
			(*deliveryNode)->filter = false;
			
			// queue it in the global list of deliveries.  It will be removed in the idle loop or the thread
			QueueDeliveryNode(deliveryNode);
		
			result = UIDFetchMessages(imapStream, mailbox, uidsToFetch, deliveryNode, false);

			if (result)
			{
				(*deliveryNode)->finished = true;
				
				// no threading.  Get results immediately
				if ((PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable())) UpdateIMAPMailbox(tocToSync);
				// else
					// hope Alan remember to UpdateIMAPMailbox() before grabbing summaries.
								
				// and return the new results
				BuildSearchResults(searchNode, uidsToFetch);
			}
			else
			{
				// about this deliverynode, let the idle routine dequeue it.
				AbortDeliveryNode(deliveryNode);
			}
	
			// Cleanup.
			UID_LL_Zap(&uidsToFetch);
			uidsToFetch = nil;
		}
		else
			WarnUser(MEM_ERR,MemError());
	}
		
	return (result);
}
							
/**********************************************************************
 * BuildSearchResults - convert from a UIDNode list to something the
 *	search window can understand
 **********************************************************************/
void BuildSearchResults(DeliveryNodeHandle delivery, UIDNodeHandle results)
{
	long numResults = 0;
	long offset = 0;
	UIDNodeHandle resultScan;
	IMAPSResultStruct imapResult;
	OSErr err = noErr;
	SignedByte state;
				
	// must have some results
	if (results && *results)
	{
		// must have someplace to put the results
		if (delivery && !(*delivery)->aborted)
		{
			// allocate a handle big enough for all the results
			for (resultScan = results; resultScan; resultScan = (*resultScan)->next) numResults++;
			state = HGetState((Handle)delivery);
			LDRef(delivery);
			if ((*delivery)->results)
			{
				offset = GetHandleSize((*delivery)->results);
				SetHandleBig_((*delivery)->results,offset + (numResults*sizeof(IMAPSResultStruct)));
				if (err=MemError())
				{
					WarnUser(MEM_ERR, err);
					HSetState(delivery, state);
					return;
				}
			}
			else
			{
				(*delivery)->results = NuHTempBetter(numResults*sizeof(IMAPSResultStruct));
				if (!(*delivery)->results)
				{
					WarnUser(MEM_ERR, MemError());
					HSetState(delivery, state);
					return;
				}
			}
			
			// put the results into the handle
			for ((resultScan=results) && (offset=0); resultScan; (resultScan=(*resultScan)->next) && offset++)
			{
				imapResult.box = (*resultScan)->boxIndex;
				imapResult.uidHash = (*resultScan)->uid;
				BMD(&imapResult,(*((*delivery)->results))+offset,sizeof(IMAPSResultStruct));

			}
			HSetState(delivery, state);			
		}
	}
}

/**********************************************************************
 * IMAPAbortSearch - stop the search in progress
 **********************************************************************/
void IMAPAbortSearch(TOCHandle searchWin)
{
	DeliveryNodeHandle node = FindNodeByToc(searchWin);
	OSErr err = noErr;
	threadDataHandle index;
	SignedByte state;
		
	// Is there a search going on this window?	
	if (node)
	{
		// has the search finished?
		if (!(*node)->finished)
		{
			// if not, mark the node as aborted.
			AbortDeliveryNode(node);
			
			// set command period for all the search threads for this window
			for (index=gThreadData;index;index=(*index)->next)
			{
				// is this a search thread?
				if ((*index)->currentTask == IMAPSearchTask)
				{
					// does this search thread belong to the current search window?
					state = HGetState((Handle)index);
					LDRef(index);
					if (SameTOC((*index)->imapInfo.destToc, searchWin))
					{
						// cancel the thread
						SetThreadGlobalCommandPeriod ((*index)->threadID, true);

						// continue through all threads like this
					}
					HSetState((Handle)index,state);	
				}
			}
		}
		else
		{
			// the search is finished, we can kill the node.
			DequeueDeliveryNode(node);
			ZapDeliveryNode(&node);
		}
	}
}

/**********************************************************************
 * IMAPStartFiltering - prepare to talk to the server about messages.
 *	NOT thread safe
 **********************************************************************/
Boolean IMAPStartFiltering(TOCHandle tocToFilter, Boolean connect)
{
	Boolean result = false;
	MailboxNodeHandle boxToFilter = nil;
	PersHandle oldPers = CurPers;
	Str255 progressMessage, mName;
	
	// must have something to filter
	if (!tocToFilter) return (false);
	
	// if we're already connected, nothing to do.
	if (gFilterStream != nil && IsSelected(gFilterStream->mailStream)) return (true);
	
	// don't display any progress during filtering
	gFilteringUnderway = true;
					
	// if asked, open a connection to the mailbox tocToFilter
	if (connect)
	{
		boxToFilter = TOCToMbox(tocToFilter);
		gFilterPers = TOCToPers(tocToFilter);	
		if (boxToFilter && gFilterPers)
		{
			oldPers = CurPers;
			CurPers = gFilterPers;
			
			// display some progress information
			LockMailboxNodeHandle(boxToFilter);
			PathToMailboxName ((*boxToFilter)->mailboxName, mName, (*boxToFilter)->delimiter);
			UnlockMailboxNodeHandle(boxToFilter);
			ComposeRString(progressMessage,IMAP_SEARCHING_MAILBOX,mName);
			PROGRESS_MESSAGE(kpSubTitle,progressMessage);
									
			// Connect to the server
			if (gFilterStream || (gFilterStream = GetIMAPConnection(IMAPSearchTask, CAN_PROGRESS))) 
			{
				// Select the mailbox
				LockMailboxNodeHandle(boxToFilter);
				if ((result = IMAPOpenMailbox(gFilterStream, (*boxToFilter)->mailboxName,false))==false)
				{
					IMAPError(kIMAPSearching, kIMAPSelectMailboxErr, errIMAPSearchMailboxErr);	
					CleanupConnection(&gFilterStream);
					gFilterStream = nil;
					gFilteringUnderway = false;
					CommandPeriod = true;	// stop all filtering
				}
				UnlockMailboxNodeHandle(boxToFilter);
			}
			else
			{
				if (!CommandPeriod) WarnUser(MEM_ERR, MemError());
				gFilteringUnderway = false;
				CommandPeriod = true;	// stop all filtering
			}		
			CurPers = oldPers;
		}
	}
			
	return (result || !connect);
}

/**********************************************************************
 * IMAPTermMatch - does an IMAP message match a term?
 *	NOT thread safe
 **********************************************************************/
Boolean IMAPTermMatch(MTPtr mt, MSumPtr sum)
{
	Boolean match = false;
	UIDNodeHandle searchResults = nil;
	Str255 cSearchString;
	Str255 pHeaders, cHeaders;
					
	// must have a connection open to the server ...
	if (!gFilterStream || !IsSelected(gFilterStream->mailStream))
	{
		IMAPError(kIMAPSearching, kIMAPNotConnectedErr, errIMAPSearchMailboxErr);
		CommandPeriod = true;
		return (false);
	}
	
	// build the string we're looking for ...
	PtoCcpy(cSearchString, mt->value);
	
	// build the header to look for ...
	pHeaders[0] = cHeaders[0] = nil;
	switch (mt->headerID)
	{							
		case FILTER_ANY:
		case FILTER_ADDRESSEE:
		{
			BuildHeaderSearchString(mt->headerID==FILTER_ANY, mt->headerID==FILTER_ADDRESSEE, pHeaders);
			
			PtoCcpy(cHeaders, pHeaders);
			
			break;
		}
		
		case FILTER_BODY:
			break;
			
		default:
		{		
			// the header name ends with a ':'.  Remove it.
			PCopy(pHeaders, mt->header);
			pHeaders[0]--;
			
			PtoCcpy(cHeaders, pHeaders);
			
			break;
		}
	}	
	
	// and do the search ...
	if (UIDFind(gFilterStream, cHeaders, mt->headerID==FILTER_BODY, mt->verb==mbmNotContains, cSearchString, sum->uidHash, sum->uidHash, &searchResults))
	{
		if (searchResults && *searchResults)
		{
			if ((*searchResults)->uid == sum->uidHash) match = true;
			ZapHandle(searchResults);
		}
	}
	else	// the search failed.  Stop filtering.
		CommandPeriod = true;
		
	return (match);
}

/**********************************************************************
 *	BuildHeaderSearchString - build the list of headers to search for
 *	 when searching on Any Header or AnyRecipient
 **********************************************************************/
void BuildHeaderSearchString(Boolean anyHeader, Boolean anyRec, Str255 pHeaders)
{	
	pHeaders[0] = nil;
	
	if (anyHeader)
	{
		PCatR(pHeaders,HEADER_STRN+FROM_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+SUBJ_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+ATTACH_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+REPLYTO_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+IN_REPLY_TO_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+DATE_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
	}
	
	if (anyRec || anyHeader)
	{
		PCatR(pHeaders,HEADER_STRN+TO_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
		PCatR(pHeaders,HEADER_STRN+CC_HEAD);
		pHeaders[pHeaders[0]] = ',';	// turn : into a ,
	}
	
	// drop last comma
	pHeaders[0]--;
	
	return;
}
		
/**********************************************************************
 * IMAPStopFiltering - clean up after filtering.
 *	NOT thread safe
 **********************************************************************/
void IMAPStopFiltering(Boolean reallyDone)
{
	if (gFilterStream != nil)
	{
		// if we're really done filering, display some progress
		if (reallyDone) PROGRESS_MESSAGER(kpSubTitle,CLEANUP_CONNECTION);
			
		CleanupConnection(&gFilterStream);
		gFilterStream = nil;
		
		gFilterPers = nil;
	}
	if (reallyDone) 
	{
		gFilteringUnderway = false;
		gManualFilteringUnderway = 0;
	}
			
	return;
}


/**********************************************************************
 * IMAPFilteringUnderway - return true if we're in the middle of
 *	filtering messages.
 **********************************************************************/
Boolean IMAPFilteringUnderway(void)
{
	return (gFilteringUnderway);
}


/**********************************************************************
 * IMAPStartManualFiltering - manual filtering is now underway
 **********************************************************************/
void IMAPStartManualFiltering(void)
{
	gManualFilteringUnderway++;
}

/**********************************************************************
 * IMAPFilteringWarnOffline - return true if the user should be alerted
 *	about going online during manual filtering
 **********************************************************************/
Boolean IMAPFilteringWarnOffline(void)
{
	Boolean warn = true;
	
	// if the user is manually filtering, and hasn't yet been warned, warn him.
	if (gManualFilteringUnderway==1)
	{
		gManualFilteringUnderway++;
	}
	else
	{
		// otherwise, don't if we're filtering
		warn = !IMAPFilteringUnderway();
	}
		
	return (warn);
}

/**********************************************************************
 * IMAPProccessBoxesMainThread - fire off threads to handle IMAP 
 *	mailboxes now that they're idle.
 **********************************************************************/
void IMAPProccessBoxesMainThread(Boolean bResync, Boolean bExpunge, Boolean bSearch)
{
	PersHandle pers;
	SignedByte state;
	
	// go through all mailboxes, resynching and expunging the ones that need it.
	for (pers = PersList; pers; pers = (*pers)->next)
	{
		if (IsIMAPPers(pers))
		{
			if ((*pers)->mailboxTree)
			{
				state = HGetState((Handle)pers);
				LDRef(pers);
				IMAPMailboxPostProcess((*pers)->mailboxTree, bResync, bExpunge, bSearch);
				HSetState(pers, state);
			}
		}
	}
}

/**********************************************************************
 * IMAPMailboxPostProcess - resync the mailboxes in the tree that
 *	are marked.  NOT thread safe!
 **********************************************************************/
void IMAPMailboxPostProcess(MailboxNodeHandle tree, Boolean resync, Boolean expunge, Boolean search)
{
	MailboxNodeHandle scan = nil;
	TOCHandle scanTOC = nil;
	
	// first, go through and expunge all mailboxes that need it
	if (expunge)
	{
		scan = tree;
		while (scan)
		{
			if (DoesIMAPMailboxNeed(scan, kNeedsExpunge))
			{
				SetIMAPMailboxNeeds(scan, kNeedsExpunge, false);
				
				LockMailboxNodeHandle(scan);
				if (scanTOC=FindTOC(&((*scan)->mailboxSpec)))
				{
					// expunge the mailbox
					DoExpungeMailboxLo(scanTOC, false);
				}
				UnlockMailboxNodeHandle(scan);
			}
			
			// check out the children
			if ((*scan)->childList)
			{
				LockMailboxNodeHandle(scan);
				IMAPMailboxPostProcess((*scan)->childList, resync, expunge, search);
				UnlockMailboxNodeHandle(scan);
			}
			
			scan = (*scan)->next;
		}
	}
	
	if (resync)
	{	
		// now go through and start resyncs on all mailboxes that need it.
		scan = tree;
		while (scan)
		{
			// is this mailbox marked for resync?
			if (DoesIMAPMailboxNeed(scan,kNeedsResync))
			{
				if (PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING))
					SetIMAPMailboxNeeds(scan, kNeedsResync, false);
				
				// is it visible?
				LockMailboxNodeHandle(scan);
				if (scanTOC=FindTOC(&((*scan)->mailboxSpec)))
				{
					if ((*scanTOC)->win && IsWindowVisible(GetMyWindowWindowPtr((*scanTOC)->win))) 
					{
						SetIMAPMailboxNeeds(scan, kNeedsResync, false);
						FetchNewMessages(scanTOC, true, false, false, false);
					}	
				}
				UnlockMailboxNodeHandle(scan);
			}
			
			// check out the children
			if ((*scan)->childList)
			{
				LockMailboxNodeHandle(scan);
				IMAPMailboxPostProcess((*scan)->childList, resync, expunge, search);
				UnlockMailboxNodeHandle(scan);
			}
			
			scan = (*scan)->next;
		}
	}
	
	if (search)
	{	
		// now go through and update incremental searches
		scan = tree;
		while (scan)
		{
			// is this mailbox marked as having changed?
			if (DoesIMAPMailboxNeed(scan,kNeedsSearch))
			{
				SetIMAPMailboxNeeds(scan, kNeedsSearch, false);
				
				// update all incremental searches
				IMAPSearchIncremental(scan);
			}
			
			// check out the children
			if ((*scan)->childList)
			{
				LockMailboxNodeHandle(scan);
				IMAPMailboxPostProcess((*scan)->childList, resync, expunge, search);
				UnlockMailboxNodeHandle(scan);
			}
			
			scan = (*scan)->next;
		}
	}
}

/**********************************************************************
 * PrefMakeAttachmentStub - given the size of a part, and the myriad
 *	of download preferences, return true if we should make a stub.
 **********************************************************************/
Boolean PrefMakeAttachmentStub(long size)
{
	Boolean make = true;
	long bigMessage = GetRLong(BIG_MESSAGE) K;
	
	// Fetch everything
	if (PrefIsSet(PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS)) make = false;
	// Fetch attachments up to x K
	else if (PrefIsSet(PREF_IMAP_FULL_MESSAGE) && (size < bigMessage)) make = false;
	
	return (make);
}

/**********************************************************************
 * FlagForResync - given a mailbox, set a flag so it gets resynced
 **********************************************************************/
void FlagForResync(TOCHandle tocH)
{
	MailboxNodeHandle box = TOCToMbox(tocH);
	if (box)
		SetIMAPMailboxNeeds(box, kNeedsResync, true);
}

/**********************************************************************
 * FlagForExpunge - given a mailbox, set a flag so it gets expunged
 **********************************************************************/
void FlagForExpunge(TOCHandle tocH)
{
	MailboxNodeHandle box = TOCToMbox(tocH);			
	if (box)
		SetIMAPMailboxNeeds(box, kNeedsExpunge, true);
}

/**********************************************************************
 * IMAPTocHBusy - mark an IMAP mailbox as busy or not.  This prevents
 *	it's toc from getting flushed.
 **********************************************************************/
void IMAPTocHBusy(TOCHandle tocH, Boolean busy)
{
	MailboxNodeHandle box = TOCToMbox(tocH);			
	if (box)
	{
		if (busy) (*box)->tocRef++;
		else if ((*box)->tocRef) (*box)->tocRef--;
	}
}

/**********************************************************************
 * ResyncOpenMailboxes - resync all open IMAP mailboxes
 **********************************************************************/
void  ResyncOpenMailboxes(PersHandle pers)
{
	OSErr err = noErr;
	Accumulator mailboxesAcc;
	MailboxNodeHandle b = nil;
	PersHandle p;
	TOCHandle tocH;
	FSSpec boxSpec;
	MailboxNodeHandle inbox = LocateInboxForPers(pers);
	
	// build the list of mailboxes to resynchronize
	AccuInit(&mailboxesAcc);
	for (tocH=TOCList; tocH && (err == noErr); tocH = (*tocH)->next)
	{
		boxSpec = GetMailboxSpec(tocH,-1);
		b = TOCToMbox(tocH);
		p = TOCToPers(tocH);	
		if (p && (p == pers) && b && (b!=inbox))
		{
			err = AccuAddPtr(&mailboxesAcc, &boxSpec, sizeof(FSSpec));
		}
	}
	AccuTrim(&mailboxesAcc);
	
	if ((err == noErr) && (mailboxesAcc.size > 0))
	{
		// resynchronize the mailboxes
		err = IMAPProcessMailboxes(mailboxesAcc.data,IMAPMultResyncTask);
	}
	else
	{
		// an error occurred.  Clean up the list
		AccuZap(mailboxesAcc);
	}
}

/**********************************************************************
 * ResetFilterFlags - given a mailbox, reset the summaries need to 
 *	filter flag.
 **********************************************************************/
void ResetFilterFlags(TOCHandle tocH)
{
	short count;
	
	// only do this to IMAP mailboxes
	if ((*tocH)->imapTOC)
	{
		for (count = 0; count < (*tocH)->count; count++)
			(*tocH)->sums[count].flags &= ~FLAG_UNFILTERED;
	}
		
}

/**********************************************************************
 * NeedCleanUpAttachmentsAfterIMAPTransfer - return true if this IMAP 
 *	message's attachments ought to be deleted.
 **********************************************************************/
Boolean NeedCleanUpAttachmentsAfterIMAPTransfer(TOCHandle tocH, short sumNum)
{
	Boolean deleteThem = false;
	MailboxNodeHandle mbox = nil;
	PersHandle oldPers = CurPers, pers = nil;
	
	// only makes sense to check IMAP toc's
	if (tocH && (*tocH)->imapTOC)
	{
		// is the pref set to clean up after a transfer?
		mbox = TOCToMbox(tocH);
		pers = TOCToPers(tocH);
		if (mbox && pers)
		{
			CurPers = pers;
			if (!PrefIsSet(PREF_IMAP_SAVE_ORPHANED_ATT))
			{
				deleteThem = true;
			}
			CurPers = oldPers;
		}
	}
	
	return (deleteThem);
}

/**********************************************************************
 * CleanUpAttachmentsAfterSingleIMAPTransfer - clean up an IMAP message's
 *	attachments after a transfer.
 **********************************************************************/
void CleanUpAttachmentsAfterIMAPTransfer(TOCHandle tocH, short sumNum)
{
	if (NeedCleanUpAttachmentsAfterIMAPTransfer(tocH, sumNum))
	{
		// move this message's attachments to the trash.
		MovingAttachments(tocH,sumNum,True,false,True,false);
	}
	
}

/**********************************************************************
 * PrepareDownloadProgress() - set up the stream to properly display
 *	progress,
 **********************************************************************/
void PrepareDownloadProgress(IMAPStreamPtr imapStream, long totalSize)
{
	imapStream->mailStream->totalTransfer = totalSize;
	imapStream->mailStream->currentTransfer = 0;
	imapStream->mailStream->lastProgress = TickCount();
}

/************************************************************************
 * IMAPFetchMessageHeadersForFiltering - return a handle to the remote 
 *	message's headers.
 ************************************************************************/
Handle IMAPFetchMessageHeadersForFiltering(TOCHandle tocH, short sumNum)
{
	Boolean result = false;
	PersHandle oldPers = CurPers;
	PersHandle pers = nil;
	MailboxNodeHandle mailboxNode = nil;
	unsigned long uid;
	Handle headers = nil;
					
	// must have a toc
	if (!tocH) return (nil);
	
	// must be an IMAP toc
	if (!(*tocH)->imapTOC) return (nil);
	
	// sumNum must have a uid
	uid = (*tocH)->sums[sumNum].uidHash;
	if (!(uid > 0)) return (nil);
	
	// must have a connection open to the server ...
	if (!gFilterStream || !IsSelected(gFilterStream->mailStream))
	{
		IMAPError(kIMAPSearching, kIMAPNotConnectedErr, errIMAPSearchMailboxErr);
		CommandPeriod = true;
		return (nil);
	}
	
	// see if this is indeed an IMAP mailbox
	mailboxNode = TOCToMbox(tocH);
	pers = TOCToPers(tocH);
	if (mailboxNode && pers)
	{
		CurPers = pers;

		if (UIDFetchHeader(gFilterStream, uid, false))
		{
			if (gFilterStream->mailStream->fNetData && *(gFilterStream->mailStream->fNetData))
			{
				headers = gFilterStream->mailStream->fNetData;
				gFilterStream->mailStream->fNetData = nil;
			}
		}
			
		CurPers = oldPers;
	}
	else
		IMAPError(kIMAPFetch, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);

	// convert /r/n to /r
	RNtoR(headers);
	
	return (headers);
}

/************************************************************************
 * GetNextWaitingMailboxNode - return a pointer the the next IMAP node 
 *	waiting to be filtered
 ************************************************************************/
MailboxNodeHandle GetNextWaitingMailboxNode(MailboxNodeHandle tree)
{
	MailboxNodeHandle scan = tree;
	MailboxNodeHandle node = nil;
		
	while (scan)
	{
		// is this the node we're looking for?
		if (DoesIMAPMailboxNeed(scan, kNeedsFilter)) 
		{
			node = scan;
			break;
		}
		
		// is the node we're looking for one of the children of this node?
		LockMailboxNodeHandle(scan);
		if ((*scan)->childList) node = GetNextWaitingMailboxNode(((*scan)->childList));
		UnlockMailboxNodeHandle(scan);
		
		if (node) break;
		
		// otherwise, check the next node.
		scan = (*scan)->next;
	}
	
	// Return the node
	return (node);
}

/************************************************************************
 * GetNextWaitingIMAPToc - return a pointer the the next IMAP toc waiting
 *	to be filtered
 ************************************************************************/
void GetNextWaitingIMAPToc(TOCHandle *toc)
{
	FSSpec spec;
	PersHandle pers;
	MailboxNodeHandle scan = nil;
	TOCHandle tocH = nil, hidTocH = nil;
	SignedByte state;
		
	for (pers=PersList;pers;pers=(*pers)->next)
	{
		state = HGetState((Handle)pers);
		LDRef(pers);
		scan = GetNextWaitingMailboxNode((*pers)->mailboxTree);
		HSetState((Handle)(pers), state);
		
		if (scan) break;
	}
	
	if (scan)
	{
		spec = (*scan)->mailboxSpec;
		tocH = TOCBySpec(&spec);
		if (tocH)
		{
			//are there no unfiltered messages in this toc?
			if (CountFlaggedMessages(tocH) == 0)
			{
				// then check the hidden toc
				hidTocH = GetHiddenCacheMailbox(scan, false, false);
				
				// return the hidden toc if there are unfiltered messages in it
				if (hidTocH && CountFlaggedMessages(hidTocH))
					tocH = hidTocH;
			}
		}
		else
		{
			// something bad happened trying to open this mailbox.
			// forget about filtering it.
			SetIMAPMailboxNeeds(scan, kNeedsFilter, false);	
		}
	}
	
	if (toc)
		*toc = tocH;
}

/************************************************************************
 * IMAPFilterProgress - filter incoming messages for an IMAP personality.
 *	For now, all this does is start a dumb thread to monitor filter
 *	progress and display something in the TP window.
 ************************************************************************/
OSErr IMAPFilterProgress(TOCHandle tocH)
{
	OSErr err = noErr;
	XferFlags flags;
	threadDataHandle index;
	IMAPTransferRec imapInfo;
	
	// notify thread of filter target
	gFilterTocH = tocH;
	gNumUnfiltered = tocH ? IMAPCountUnfilteredMessages(TOCToMbox(tocH)) : 0;
	
	if (gNumUnfiltered)
	{	
		// is there already a thread set up to filter this toc?
		for (index=gThreadData; index; index=(*index)->next)
		{
			if ((*index)->currentTask == IMAPFilterTask)
				return (noErr);
		}
		
		// nope.  Set up the thread.
		if (!PrefIsSet(PREF_THREADING_OFF) && ThreadsAvailable())
		{								
			Zero(flags);
			Zero(imapInfo);
			
			PushPers(CurPers);
			CurPers = TOCToPers(tocH);
			
			imapInfo.command = IMAPFilterTask;
			err = SetupXferMailThread(false, false, true, false, flags, &imapInfo);
			
			// ... and remove any old task errors
			RemoveTaskErrors(IMAPFilterTask,(*CurPers)->persId);
			
			PopPers();
		}	
		
		// watch for cancel
		gFilteringCancelled = false;
	}
	return (err);
}

/************************************************************************
 * IMAPCountUnfilteredMessages - count the number of unfiltered messages 
 *	in an IMAP mailbox.
 ************************************************************************/
long IMAPCountUnfilteredMessages(MailboxNodeHandle mbox)
{
	long count = 0;
	TOCHandle tocH;
	
	if (mbox)
	{
		tocH = FindTOC(&((*mbox)->mailboxSpec));
		if (tocH)
		{
			count = CountFlaggedMessages(tocH);
			
			// look in the hidden mailbox as well ...
			tocH = GetHiddenCacheMailbox(mbox, false, false);
			if (tocH)
				count += CountFlaggedMessages(tocH);
		}
	}
	return (count);	
}

/************************************************************************
 * DoIMAPFilterProgress - filter incoming messages for an IMAP 
 *	personality.
 ************************************************************************/
OSErr DoIMAPFilterProgress(void)
{
	OSErr err = noErr;
	TOCHandle tocH = 0;
	Str255 progressMessage;
	MailboxNodeHandle mBox;
	short initial = gNumUnfiltered;
	short remaining = 0;
	
	PushPers(CurPers);
	
	PROGRESS_START;
	
	// display progress while this mailbox is being filtered
	while ((err == noErr) && gFilterTocH && !gFilteringCancelled)
	{	
		// check for changes  ...
		if (tocH != gFilterTocH)
		{	
			tocH = gFilterTocH;
			
			// which mailbox is being filtered?
			mBox = TOCToMbox(tocH);
			CurPers = TOCToPers(tocH);
			if (!CurPers) CurPers = PersList;
			
			// update the progress message
			ComposeRString(progressMessage, IMAP_FILTERING_MESSAGES, (*CurPers)->name);
			PROGRESS_MESSAGE(kpTitle,progressMessage);
			PROGRESS_MESSAGER(kpSubTitle,LEFT_TO_FILTER);
			remaining = 0;
		}
		
		if (remaining != gNumUnfiltered)
		{
			remaining = gNumUnfiltered;
			PROGRESS_BAR(100 - (remaining * 100)/initial,remaining,nil,nil,nil);
		}

		// sit and spin.
		CycleBalls();
		err = MyYieldToAnyThread();
		
		// cancel filtering
		if (CommandPeriod)
			gFilteringCancelled = true;
	}
	
	PROGRESS_END;	
	
	PopPers();
	
	return (err);
}

/**********************************************************************
 * IMAPCancelFiltering - mark all mailboxes that need to be filtered
 *	as not.  NOT THREAD SAFE
 **********************************************************************/
void IMAPFilteringCancelled(Boolean bOverride)
{
	TOCHandle tocH;
	MailboxNodeHandle node;
	short count;
	
	// was filtering cancelled?
	if (gFilteringCancelled || bOverride)
	{
		do
		{
			GetNextWaitingIMAPToc(&tocH);
			if (tocH)
			{
				node = TOCToMbox(tocH);
				
				// mailbox no longer needs filtering
				SetIMAPMailboxNeeds(node, kNeedsFilter, false);	
				
				// reset filter flag and show messages, unless we just quit.
				if (!AmQuitting)
				{
					for (count = (*tocH)->count - 1; count >= 0 ; count--)
					{
						if ((*tocH)->sums[count].flags & FLAG_UNFILTERED)
						{
							(*tocH)->sums[count].flags &= ~FLAG_UNFILTERED;
							ShowHideFilteredSummary(tocH, count);
						}
					}
				}	
			}
		} while (tocH);
	
		gFilteringCancelled = false;
	}
}

/**********************************************************************
 * RNtoR() - convert /r/n to /r.  Useful 'cause IMAP code gives back
 *	exactly what was received on the network.
 **********************************************************************/
void RNtoR(Handle text)
{
	SignedByte state;
	long textSize;
	char *scan, *fix;
	
	if (text)
	{
		state = HGetState(text);
		LDRef(text);
		textSize = GetHandleSize(text);
		
		scan = *text;
		while (scan < (*text + textSize))
		{
			if ((*scan == '\r') && (*(scan+1) == '\n'))
			{
				scan++;
				fix = scan;
				while (fix < (*text + textSize - 1))
				{
					*fix = *(fix + 1);
					fix++;
				}
				textSize--;
			}
			else
				scan++;
		}
		
		SetHandleSize(text, textSize);
		HSetState(text, state);
	}
}

/**********************************************************************
 * SpecialCaseDownload - recurse through this body, return true if
 *	we should download this message in one big chunk because
 *	- it's got a registration part
 *	- we're too supid to handle this
 **********************************************************************/
Boolean SpecialCaseDownload(IMAPBODY *bodyInQuestion)
{
	Boolean result = false;
	PART *part = nil;
	IMAPBODY *body = nil;
	PARAMETER *param = nil;
	MIMEMap	mm;	
	Str255 name, typeString, subTypeString, espString;
	
	// Must have a body.
	if (!bodyInQuestion) return (false);

	//
	// Can't yet handle messages that contain sub messages.
	//
	
	if (bodyInQuestion->type == TYPEMESSAGE) return (true);
	
	
	// 
	//	Look for X-Eudora-Plugin-Info header, see if a plugin needs this message
	//
	
	GetRString(espString, PLUGIN_INFO);
	param = bodyInQuestion->parameter;
	while (param)
	{
		if (param->attribute)
			if (!pstrincmp(param->attribute,espString,espString[0]))
			{
				// Someday, when we wish to be smarter, pass the value parameter to the translators to see if anyone needs this message.
				// For now, just fetch it.  Peanut's the only plugin that uses it now.
				
				return (true);
			}
				
		param = param->next;
	}
	
	//
	// Loop through all parts, looking for special cases
	//
	
	part = bodyInQuestion->nested.part;
	while (part && !result)
	{
		body = &(part->body);
		if (body)
		{
			if (body->type == TYPEMESSAGE) // this might be a sub message
			{
				result = true;	
			}
			else if ((body->type == TYPEAPPLICATION))	// this might be a registration part
			{
				if (body->subtype)
				{
					// look closer at the attachment part
					GetFilenameParameter(body, name);
					if (*name)
					{		
						// make a reasonable guess at this part's type and creator
						subTypeString[0] = strlen(body->subtype);
						strncpy(subTypeString+1, body->subtype, MIN(subTypeString[0], sizeof(subTypeString)));
						
						if (FindMIMEMapPtr(BodyTypeCodeToPString(body->type, typeString),subTypeString,name,&mm))
						{
							// If our MIME map tells us this is a 'eReg' file, download the whole message
							if (REG_FILE_TYPE == mm.type)
								result = true;
						}
					}
				}
			}
			else // check the nested body
			{
				result = SpecialCaseDownload(body);
			}
		}
		
		// Next part.
		part = part->next;
	}
	
	return (result);
}

/************************************************************************
 * IsIMAPOperationUnderway - return true if the specificed operation is
 *	underway.
 ************************************************************************/
Boolean IsIMAPOperationUnderway(TaskKindEnum task)
{
	Boolean result = false;
	threadDataHandle index;
		
	if (task != UndefinedTask)
	{
		for (index=gThreadData;index && !result;index=(*index)->next)
		{
			if ((*index)->imapInfo.command == task)
				result = true;
		}
	}
	
	return (result);
}

/************************************************************************
 * IsIMAPMailboxBusy - return true if this IMAP mailbox is in use and
 *	absolutely should not be closed.
 ************************************************************************/
Boolean IsIMAPMailboxBusy(TOCHandle tocH)
{
	threadDataHandle index;
	MailboxNodeHandle mbox;
	
	if (tocH && (*tocH)->imapTOC)
	{	
		// has this tocH been added to the delivery queue, or
		// has someone asked to keep it around?
		mbox = TOCToMbox(tocH);
		if (mbox && ((*mbox)->tocRef > 0))
			return true;
		
		// is this toc in use by a thread somewhere?
		for (index=gThreadData;index;index=(*index)->next)
		{
			if (SameTOC((*index)->imapInfo.sourceToc, tocH) 
				|| SameTOC((*index)->imapInfo.destToc, tocH)
				|| SameTOC((*index)->imapInfo.delToc, tocH))
				return true;
		}
	}
		
	return false;
}

/************************************************************************
 * IMAPRemoveSelectedCachedContents - wipe the local cache of the 
 * 	selected messages.
 ************************************************************************/
void IMAPRemoveSelectedCachedContents(TOCHandle tocH)
{
	short c =  CountSelectedMessages(tocH);
	short sumNum;

	for (sumNum=0;sumNum<(*tocH)->count && c;sumNum++)
	{
		if ((*tocH)->sums[sumNum].selected)
		{
			CycleBalls();
			IMAPRemoveCachedContents(tocH, sumNum);
		}	
	}	
}

/************************************************************************
 * IMAPRemoveCachedContents - wipe the local cache of a message
 ************************************************************************/
void IMAPRemoveCachedContents(TOCHandle tocH, short sumNum)
{											
	// don't do anything to minimal headers.  Nothing to remove.
	if ((*tocH)->sums[sumNum].offset > -1)
	{			
		// delete any attachments associated with this message
		if ((*tocH)->sums[sumNum].opts & OPT_HAS_SPOOL) RemSpoolFolder((*tocH)->sums[sumNum].uidHash);
		if ((*tocH)->sums[sumNum].flags&FLAG_HAS_ATT)
		{
			// move the attachments to the trash if they haven't been orphaned.
			if (!((*tocH)->sums[sumNum].opts & OPT_ORPHAN_ATT)) 
			{
				MovingAttachments(tocH,sumNum,True,false,True,false);	// attachments
				MovingAttachments(tocH,sumNum,False,false,True,false);	// inline parts
			}
		}
		
		// close the message window
		if ((MessHandle)(*tocH)->sums[sumNum].messH) CloseMyWindow(GetMyWindowWindowPtr((*(MessHandle)(*tocH)->sums[sumNum].messH)->win));

		// adjust the preview pane
		if ((*tocH)->previewID==(*tocH)->sums[sumNum].serialNum && (*tocH)->previewPTE)
			Preview(tocH,-1);
	
		// clean up the summary		
		ZapHandle((*tocH)->sums[sumNum].cache);
		DeleteMesgError(tocH,sumNum);
		(*tocH)->sums[sumNum].offset = imapNeedToDownload;
		(*tocH)->sums[sumNum].bodyOffset = 0;
		(*tocH)->sums[sumNum].msgIdHash = 0;
		(*tocH)->sums[sumNum].subjId = 0;
		(*tocH)->sums[sumNum].mesgErrH = nil;
		(*tocH)->sums[sumNum].cache = nil;
		(*tocH)->sums[sumNum].messH = nil;
		
		// Clear decoding errors from Removed messages
		(*tocH)->sums[sumNum].flags &= ~FLAG_SKIPPED;
		if ((*tocH)->sums[sumNum].state == MESG_ERR) (*tocH)->sums[sumNum].state = READ;

#ifdef	NOBODY_SPECIAL
		// Clear the nobody flag
		(*tocH)->sums[sumNum].opts &= ~OPT_JUSTSUB;
#endif	//NOBODY_SPECIAL

		InvalSum(tocH, sumNum);
	}
	else 
		(*tocH)->sums[sumNum].offset = imapNeedToDownload;
}

/************************************************************************
 * IMAPFetchSelectedMessages - fetch the selected messages
 ************************************************************************/
void IMAPFetchSelectedMessages(TOCHandle tocH, Boolean attach)
{
	Handle uids = nil;
	short c;
	short sumNum;

	//
	// download attachments of messages that are already present if attach
	//
	
	if (attach)
	{
		for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
		{
			if ((*tocH)->sums[sumNum].selected)
				if (IMAPMessageDownloaded(tocH,sumNum)) 
					FetchAllIMAPAttachments(tocH, sumNum, false);
		}
	}

	//
	// Fetch Messages that haven't been fetched yet
	//
	
	if ((c= CountSelectedMessages(tocH))>0)
	{	
		// figure out how many of the selected messages need to be downloaded
		for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
			if ((*tocH)->sums[sumNum].selected)
				if (IMAPMessageBeingDownloaded(tocH, sumNum) || IMAPMessageDownloaded(tocH, sumNum)) c--;
		
		// Make a handle big enough for them
		uids = NuHandleClear(c*sizeof(unsigned long));
		if (uids)
		{	
			// and stick them in the handle
			for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
				if ((*tocH)->sums[sumNum].selected)
					if (!IMAPMessageBeingDownloaded(tocH, sumNum) && !IMAPMessageDownloaded(tocH, sumNum))
						BMD(&((*tocH)->sums[sumNum].uidHash),&((unsigned long *)(*uids))[--c],sizeof(unsigned long));
											
			// fetch the messages all in the background.  Fetch completely if doing attachments.
			UIDDownloadMessages(tocH, uids, false, attach);
		}
		else
		{
			WarnUser(MemError(), MEM_ERR);
		}	
	}
}

/************************************************************************
 * QueueMessFlagChange - remember a change to an IMAP message flag.
 *	Assume we're called if something's changed.
 ************************************************************************/
OSErr QueueMessFlagChange(TOCHandle tocH, short sumNum, StateEnum state, Boolean bTrashed)
{
	OSErr err = noErr;
	PersHandle pers = nil;
	MailboxNodeHandle mb = nil;
	LocalFlagChangeStruct flags;
	long queuedFlagsSize = 0, oldFlagSize = 0;
	Boolean done = false;
	Boolean flagged = false;
	short color = GetRLong(IMAP_FLAGGED_LABEL);

	// must have been passed a TOC.
	if (!tocH) return (paramErr);

	// this must be an IMAP mailbox
	if (!(*tocH)->imapTOC) return (paramErr);

	// Is this message labeled as flagged?
	if (color && (GetSumColor(tocH,sumNum) == color))
	flagged = true;

	// first, determine which mailbox this message belongs to.
	mb = TOCToMbox(tocH);
	pers = TOCToPers(tocH);
	if (mb && pers)
	{
		//
		// figure out what flags are to be set and reset.
		//

		Zero(flags);

		flags.uid = (*tocH)->sums[sumNum].uidHash;
		flags.mailbox = (*mb)->uidValidity;
		flags.seen = ((*tocH)->sums[sumNum].state==READ)?1:0;                   // set seen flag if message is marked READ
		flags.deleted = (((*tocH)->sums[sumNum].opts & OPT_DELETED)!=0)?1:0; 	// set deleted flag if message is marked for deleting locally
		flags.flagged = flagged;                                                // set flagged if the flagged label is used
		flags.answered = ((*tocH)->sums[sumNum].state==REPLIED)?1:0;    		// set answered flag if message has been replied to
		flags.draft = 0;                                                        // draft is currently unused
		flags.recent = 0;                                                       // recent is set only when mail is delivered
		if (flags.deleted && bTrashed)											// transferring to the trash?
		{
			flags.deleted = 0;
			flags.trashed = 1;	
		}						
		flags.processed = 0;
		
		//
		// stuff this flag change into the personalities flag change handle
		//

		LockMailboxNodeHandle(mb);
		if (LockMailboxNodeFlags(mb))
		{
			// is this flag change already queued?
			if ((*mb)->queuedFlags != nil)
			{
				short numChanges = 0;
				LocalFlagChangePtr curFlags;

				queuedFlagsSize = GetHandleSize((*mb)->queuedFlags);
				numChanges = queuedFlagsSize / sizeof(LocalFlagChangeStruct);

				while(numChanges>0)
				{
					curFlags = &((*((*mb)->queuedFlags))[numChanges-1]);

					if (curFlags->uid == flags.uid)
					{
						BMD(&flags, curFlags, sizeof(LocalFlagChangeStruct));
						done = true;
						break;
					}
					numChanges--;
				}
			}

			// must add this new flag change ...
			if (!done)
			{
				// grow the queued flag handle
				if ((*mb)->queuedFlags == nil)
				{
					queuedFlagsSize = sizeof(LocalFlagChangeStruct);
					(*mb)->queuedFlags = NewZH(LocalFlagChangeStruct);
					err = MemError();
				}
				else
				{
					oldFlagSize = GetHandleSize((*mb)->queuedFlags);
					queuedFlagsSize = oldFlagSize + sizeof(LocalFlagChangeStruct);
					SetHandleSize((Handle)((*mb)->queuedFlags), queuedFlagsSize);
					err = MemError();
				}

				// save the flag change
				if (((*mb)->queuedFlags) && (err==noErr))
					BMD(&flags, &(((LocalFlagChangePtr)(*(*mb)->queuedFlags))[(oldFlagSize/sizeof(LocalFlagChangeStruct))]), sizeof(LocalFlagChangeStruct));
				else
					err = IMAPError(kIMAPQueueFlagChange, kIMAPMemErr, err);
			}

			UnlockMailboxNodeFlags(mb);
		}
		else
		{
			err = userCanceledErr;
		}
		UnlockMailboxNodeHandle(mb);
	
		// mark this mailbox so we can recognize it when we process these new commands
		if (err == noErr)
		{
			SetIMAPMailboxNeeds(mb, kNeedsExecCmd, true);
			SetIMAPMailboxNeeds((*pers)->mailboxTree, kNeedsExecCmd, true);
		}
	}
	else
		err = IMAPError(kIMAPQueueFlagChange, kIMAPNotIMAPMailboxErr, errNotIMAPMailboxErr);

	return (err);
}

/************************************************************************
 * PerformQueuedOperations - perform all of the operations queued for 
 *	this IMAP personality.
 ************************************************************************/
OSErr PerformQueuedCommands(PersHandle pers, IMAPStreamPtr imapStream, Boolean progress)
{
	static Boolean bUnderway = false;
	long ticks = TickCount();
	Boolean progressed = false;
	OSErr err = noErr;
	
	// sanity
	if (!imapStream || !pers || !IsIMAPPers(pers)) return noErr;
	
	// no commands queued?
	if (!DoesIMAPMailboxNeed((*pers)->mailboxTree, kNeedsExecCmd)) return noErr; 
	
	// is some other thread already doing this?
	while (bUnderway && !CommandPeriod)
	{
		// put up a progress message if we've waiting for more than a second
		if (!progressed && ((TickCount() - ticks)>60))
		{
			PROGRESS_MESSAGER(kpMessage,IMAP_WAITING_FOR_CONNECTION);
			progressed = true;
		}						
		CycleBalls();
		if (MyYieldToAnyThread() != noErr) break;
	}
	
	// do nothing if the user cancelled
	if (CommandPeriod) return (userCanceledErr);
	
	// perform the queued commands
	err = PerformQueuedCommandsLo(pers, (*pers)->mailboxTree, imapStream, progress);
	
	// success?
	if (err == noErr)
		SetIMAPMailboxNeeds((*pers)->mailboxTree, kNeedsExecCmd, false);
		
	return err;
}

/************************************************************************
 * PerformQueuedCommandsLo - perform all of the operations queued for 
 *	this IMAP personality.
 ************************************************************************/
OSErr PerformQueuedCommandsLo(PersHandle pers, MailboxNodeHandle tree, IMAPStreamPtr imapStream, Boolean progress)
{
	OSErr err = noErr, retErr = noErr;
	Str255 scratch;
		
	// check each mailbox belonging to this personality
	while (tree)
	{
		// are there queued flags waiting?
		if ((*tree)->queuedFlags)
		{
			// SELECT the mailbox
			if (IMAPOpenMailbox(imapStream, (*tree)->mailboxName,false))
			{	
				// display some progress
				if (progress)
				{
					ComposeRString(scratch,IMAP_QUEUED_COMMANDS,(*tree)->mailboxSpec.name);	
					PROGRESS_START;
					PROGRESS_MESSAGE(kpSubTitle,scratch);
				}
				
				// update message flags ...
				if ((*tree)->queuedFlags)
					err = UpdateMessFlags(imapStream, tree, progress);
					
				// perform queued transfers once supported ...
				
				// success?  Then don't check again.
				if (err == noErr)
					SetIMAPMailboxNeeds(tree, kNeedsExecCmd, false);
				
				// write mailbox info back out to the file.
				WriteIMAPMailboxInfo(&(*tree)->mailboxSpec, tree);
				
				// return an error if any of this fails
				retErr = retErr ? retErr : err;
			}
		}
		
		// check the children
		if ((*tree)->childList)
			retErr |= PerformQueuedCommandsLo(pers, (*tree)->childList, imapStream, progress);
			
		// next box
		tree = (*tree)->next;
	}
	
	if (progress) PROGRESS_END;
	
	// Any Undos we may have left around are probably stale ...
	NukeXfUndo();
	
	return (retErr);
}

/************************************************************************
 * ExecuteAllPendingIMAPCommands - execute all pending IMAP commands
 *	for each personality that is set to do so on quit.
 ************************************************************************/
void ExecuteAllPendingIMAPCommands(void)
{
	IMAPStreamPtr imapStream;
	
	PushPers(CurPers);
	for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next)
	{
		if (IsIMAPPers(CurPers) 
		 && PrefIsSet(PREF_IMAP_EXECUTE_QUEUED_COMMANDS_ON_QUIT)
		 && DoesIMAPMailboxNeed((*CurPers)->mailboxTree, kNeedsExecCmd))
		{
			// execute all pending IMAP commands for this pers and clean up immediately
			imapStream = GetIMAPConnection(UndefinedTask, false);
			if (imapStream) CleanupConnection(&imapStream);
		}
	}
	PopPers();
}

/************************************************************************
 * UpdateMessFlags - update the flags of a message in the IMAP mailbox
 * 	we're currently connected to.  This routine is slightly more
 *	efficient, as it attempts to chunk flag change operations together.
 ************************************************************************/
OSErr UpdateMessFlags(IMAPStreamPtr imapStream, MailboxNodeHandle mailbox, Boolean progress)
{
	OSErr err = noErr;
	long queuedFlagsSize = 0;
	long numChanges = 0, totalFlags = 0, processedFlags = 0;
	Accumulator uidsToChange;
	LocalFlagChangePtr curFlags, scan;
	short i;
	Boolean bPendingTrash = false;
	TOCHandle srcToc;
	PersHandle pers;
	MailboxNodeHandle trash = NULL;
	TOCHandle hidTocH;
	short sumNum;
	
	// must be connected, authenticated, and selected
	if (!imapStream || !IsSelected(imapStream->mailStream)) return (paramErr);

	// must have been passed a personality, must have some queued flags
	if (!mailbox || !((*mailbox)->queuedFlags)) return (noErr);
	
	// lock the mailbox node.  Don't let it go anywhere
	LockMailboxNodeHandle(mailbox);
	
	// lock the flags handle.  Don't let anyone else mess with it
	if (LockMailboxNodeFlags(mailbox))
	{
		// make sure there are still some flags to update.  Another thread 
		// may have processed them while we were waiting for the lock.
		if (((*mailbox)->queuedFlags!=nil) && ((queuedFlagsSize = GetHandleSize((*mailbox)->queuedFlags)) != 0))
		{
			// now update the flags
		
			// don't  let them go anywhere
			LDRef((*mailbox)->queuedFlags);
			
			// first update the flags
			totalFlags = numChanges = queuedFlagsSize/sizeof(LocalFlagChangeStruct);
			while(numChanges>0 && !CommandPeriod)
			{
				curFlags = &((LocalFlagChangePtr)(*((*mailbox)->queuedFlags)))[numChanges-1];
				
				// remember if we see a message that needs to be trashed ...
				bPendingTrash |= curFlags->trashed;
				
				if (curFlags->processed == 0)
				{
					curFlags->processed = 1;	// we won't consider this flag later
					
					// add it to the list of flags to be processed this time through ...
					if (AccuInit(&uidsToChange) == noErr)
					{
						AccuAddLong(&uidsToChange, curFlags->uid);
						
						// find all flag changes that are similar.  We can execute them at once.
						for (i = 0; i < totalFlags; i++)
						{
							scan = &((LocalFlagChangePtr)(*((*mailbox)->queuedFlags)))[i];
									
							if (scan->processed == 0)	// this flag has not been processed yet
							{
								if (SameFlags(curFlags, scan))	// this flag change is the same
								{	
									scan->processed = 1;	// this flag is being processed now, too.
									AccuAddLong(&uidsToChange, scan->uid);
								}
							}
						}
						
						AccuTrim(&uidsToChange);
					
						// process the flag changes.
						if (!ProcessSimilarFlagChanges(imapStream, mailbox, uidsToChange.data, curFlags, progress, totalFlags, &processedFlags))
							err = paramErr;
						AccuZap(uidsToChange);
					}
				}
				            
				numChanges--;
			}
			
			// the do the queued transfers to the Trash
			if (bPendingTrash)
			{				
				// find the toc we're trashing messages from
				srcToc = TOCBySpec(&(*mailbox)->mailboxSpec);
				
				// find the hidden toc, if it exists
				hidTocH = GetHiddenCacheMailbox(mailbox, false, false);
					
				// where is the trash?
				pers = TOCToPers(srcToc);
				if (pers)
					trash = GetIMAPTrashMailbox(pers, true, true);
				
				if (trash)
				{				
					// build the list of UIDs to transfer to the trash.
					numChanges = 0;
					totalFlags = queuedFlagsSize/sizeof(LocalFlagChangeStruct);
					for (i = 0; i < totalFlags; i++)
					{
						curFlags = &((LocalFlagChangePtr)(*((*mailbox)->queuedFlags)))[i];
						if (curFlags->trashed)
							numChanges++;
					}
					
					// are there any messages to transfer to the trash?
					if (numChanges)
					{
						// make a list of uids to transfer ...
						if (AccuInit(&uidsToChange) == noErr)
						{
							for (i = 0; i < totalFlags; i++)
							{
								curFlags = &((LocalFlagChangePtr)(*((*mailbox)->queuedFlags)))[i];
								if (curFlags->trashed)
								{
									AccuAddLong(&uidsToChange, curFlags->uid);
									
									// also, unmark the summary as deleted.  It's not, it's just hidden
									if (hidTocH)
									{
										sumNum = FindSumByHash(hidTocH, curFlags->uid);
										if (sumNum >= 0)
											(*hidTocH)->sums[sumNum].opts &= ~OPT_DELETED;
									}
								}
							}
							AccuTrim(&uidsToChange);
							
							// copy messages to the trash mailbox then delete them from the source
							if ((mailbox == trash) || CopyMessages(imapStream, trash, uidsToChange.data, true, true))
								DoUIDMarkAsDeleted(imapStream, uidsToChange.data, false);
							
							// if we deleted messages from the trash, expunge it immediately
							if (mailbox == trash)
								if (Expunge(imapStream))
									UpdateLocalSummaries(srcToc, uidsToChange.data, true, true, false);

							// cleanup
							AccuZap(uidsToChange);
						}
					}		
				}
			}

			// Cleanup
			UL((*mailbox)->queuedFlags);
			ZapHandle((*mailbox)->queuedFlags);
			(*mailbox)->queuedFlags = nil;    
		}
		// we're done with the flags.  Other people are free to muck with them
		UnlockMailboxNodeFlags(mailbox);	
	}
	// done with the mailbox as well
	UnlockMailboxNodeHandle(mailbox);   
	return (err);
}


/**********************************************************************
 * SameFlags - see if two flag change structures are "equal"
 **********************************************************************/
Boolean SameFlags(LocalFlagChangePtr one, LocalFlagChangePtr two)
{
	Boolean result = false;
	
	if (one->mailbox == two->mailbox)	// must be talking about the same mailbox
		if (one->deleted == two->deleted)
			if (one->seen == two->seen)
				if (one->answered == two->answered)
					if (one->draft == two->draft)
						if (one->recent == two->recent)
							result = true;
	
	return (result);
}

/************************************************************************
 * ProcessSimilarFlagChanges - execute the same flag changes over
 *	a range of messages
 ************************************************************************/
Boolean ProcessSimilarFlagChanges(IMAPStreamPtr imapStream, MailboxNodeHandle mailbox, Handle uidsToChange, LocalFlagChangePtr theFlags, Boolean progress, long totalFlags, long *processedFlags)
{
	Boolean result = true;
	short i, numFlags = GetHandleSize(uidsToChange)/sizeof(unsigned long);
	long uid;
	IMAPFLAGS flags;
	Str255 uidStr;
	long ticks, lastProgress = 0;
	Str255 progressMessage;
	Boolean bDeleteToo = !DoesIMAPMailboxNeed(mailbox, kShowDeleted);
	
	// Sort the list of messages to be copied for maximum performance
	if (!PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE)) SortUIDListHandle(uidsToChange);
	
	for (i = 0; i < numFlags; i++)
	{	
		// build the range string for the command ...
		if (PrefIsSet(PREF_IMAP_DONT_USE_UID_RANGE))
		{
			uid = ((unsigned long *)(*uidsToChange))[i];
			sprintf (uidStr,"%lu",uid);
		}
		else
			i = GenerateUIDString(uidsToChange, i, &uidStr);
				
		// display some progress
		if (progress && ((ticks=TickCount()) - lastProgress > 60))
		{
			ComposeRString(progressMessage,IMAP_MARKING_MESSAGES, MIN(*processedFlags + i + 1, totalFlags), totalFlags);
			PROGRESS_MESSAGE(kpMessage,progressMessage);
			lastProgress = ticks;
		}	
		
		// do the flag change
		Zero(flags);

		// turn off the flags to turn off ...
		flags.SEEN = !theFlags->seen;
		flags.ANSWERED = !theFlags->answered;
		flags.FLAGGED = !theFlags->flagged;
		if (bDeleteToo) 
			flags.DELETED = !theFlags->deleted;
		
		if (flags.SEEN || flags.ANSWERED || flags.FLAGGED || flags.DELETED)
		    result = UIDSaveFlags(imapStream, 0, uidStr, &flags, false, true);

		// turn on the flags to turn on ...
		flags.SEEN = theFlags->seen;
		flags.ANSWERED = theFlags->answered;
		flags.FLAGGED = theFlags->flagged;
		if (bDeleteToo) 
			flags.DELETED = theFlags->deleted;

		if (flags.SEEN || flags.ANSWERED || flags.FLAGGED || flags.DELETED)
		    result &= UIDSaveFlags(imapStream, 0, uidStr, &flags, true, true);	    
	}
	
	*processedFlags += numFlags;
	return (result);
}
 
/**********************************************************************
 * LockMailboxNodeFlags - lock the flags before making changes to them
 **********************************************************************/
Boolean LockMailboxNodeFlags(MailboxNodeHandle mailbox)
{
	if (!mailbox) return (false);

	// if already locked, wait for unlock
	while (!CommandPeriod && (*mailbox)->flagLock)
	{
	    CycleBalls();
	    if (MyYieldToAnyThread()) return(false);
	}
	(*mailbox)->flagLock = true;

	return(true);
}

/**********************************************************************
 * UnlockMailboxNodeFlags - unlock the flags when done changing them
 **********************************************************************/
Boolean UnlockMailboxNodeFlags(MailboxNodeHandle mailbox)
{
	(*mailbox)->flagLock = false;
	return (true);
}

/**********************************************************************
 * PendingMessFlagChange - is this message waiting for a flag update?
 **********************************************************************/
Boolean PendingMessFlagChange(unsigned long uid, MailboxNodeHandle mailbox)
{
	Boolean result = false;
	long queuedFlagsSize;
	long numChanges;
	LocalFlagChangePtr curFlags;
	
	if (!mailbox || !*mailbox) return (false);
	
	if ((*mailbox)->queuedFlags)
	{
		queuedFlagsSize = GetHandleSize((*mailbox)->queuedFlags);
		if (queuedFlagsSize > 0)
		{
			numChanges = queuedFlagsSize/sizeof(LocalFlagChangeStruct);
			while (numChanges && !result)
			{
				curFlags = &((LocalFlagChangePtr)(*((*mailbox)->queuedFlags)))[numChanges-1];
				if ((curFlags->mailbox == (*mailbox)->uidValidity) && (curFlags->uid == uid))
					result = true;
				numChanges--;
			}
		}
	}
	
	return (result);
}

/**********************************************************************
 * FastIMAPMessageDelete - queue a delete request for a message
 *	and remove it from the toc immediately.  Handle FTM as well.
 *
 *	NOT THREAD SAFE
 **********************************************************************/
Boolean FastIMAPMessageDelete(TOCHandle tocH, Handle uids, Boolean bFTM)
{
	Boolean result = false;
	MailboxNodeHandle mbox;
	TOCHandle hidTocH;
	short count, sumNum, numUids;
	short lastSelected = -1;
	
	// sanity check
	if (!uids || !*uids || !tocH || !*tocH)
		return false;
	
	// must have at least one message to delete ...
	if ((numUids = GetHandleSize(uids)/sizeof(unsigned long))==0) 
		return false;
	
	//find the hidden toc ...
	mbox = TOCToMbox(tocH);
	hidTocH = GetHiddenCacheMailbox(mbox, false, true);
	if (!hidTocH)
		return false;
	
	for (count=0;count<numUids;count++)
	{
		sumNum = UIDToSumNum(((unsigned long *)(*uids))[count], tocH);
		if (sumNum < (*tocH)->count)
		{
			// Queue delete request.
			
			// first mark the original message as deleted.
			MarkSumAsDeleted(tocH, sumNum, true);
			
			// do not filter this message ever again
			(*tocH)->sums[sumNum].flags &= ~FLAG_UNFILTERED;
			
			// queue the flag change
			result |= (noErr == QueueMessFlagChange(tocH, sumNum, (*tocH)->sums[sumNum].state, bFTM));
			
			// move the message to the hidden toc
			HideShowSummary(tocH, tocH, hidTocH, sumNum);
			
			// remember the last message that was deleted ...
			lastSelected = sumNum;
		}
	}
	
	// select the next message ...
	if ((*tocH)->win)
		BoxSelectAfter((*tocH)->win,lastSelected);
			
	// add the undo if all went well ...
	if (result)
		AddIMAPXfUndoUIDs(tocH, hidTocH, uids,true);
		
	return (result);
}

/**********************************************************************
 * CleanUpResyncTaskErrors - clean up the task progress window before
 *	a resync operation gets underway
 **********************************************************************/
void CleanUpResyncTaskErrors(MailboxNodeHandle mbox)
{
	PersHandle pers;
	
	if (mbox && (*mbox))
	{	
		// who does this mailbox belong to?
		pers = MailboxNodeToPers(mbox);
		if (pers)
		{
			// remove any Resync related errors
			RemoveTaskErrors(IMAPResyncTask,(*pers)->persId);
			
			// remove any errors about multple resyncs
			RemoveTaskErrors(IMAPMultResyncTask,(*pers)->persId);
			
			// remove any checking mail errors if this was a mail check
			if (mbox==LocateInboxForPers(pers)) RemoveTaskErrors(CheckingTask,(*pers)->persId);
		}
	}
}

/************************************************************************
 *	IMAPAlert - display the last ALERT message in the TP window
 ************************************************************************/
void IMAPAlert(IMAPStreamPtr stream, TaskKindEnum taskKind)
{					
	static TaskKindEnum lastTaskToAlert = UndefinedTask;
	 
	// if the user cancelled, don't display an error message
	if (CommandPeriod) return;
	
	// make sure there's an ALERT string to display
	if (stream && stream->mailStream && stream->mailStream->alertStr[0])
	{
		// remember that this task was the last to display an ALERT message
		lastTaskToAlert = taskKind;
		
		if (!PrefIsSet(PREF_IGNORE_IMAP_ALERTS) && InAThread())
			AddTaskErrorsS("",stream->mailStream->alertStr,IMAPAlertTask,(*CurPers)->persId);
		
		// clear the alert string
		stream->mailStream->alertStr[0] = 0;
	}
	else
	{
		// no ALERt to display.  If this task is the same sort of task that ALERTed
		// last time, remove the ALERT from the TP Window
		if (taskKind == lastTaskToAlert)
			RemoveTaskErrors(IMAPAlertTask,(*CurPers)->persId);
	}
	
	return;
}

/************************************************************************
 * IMAPTransferLocalCache - transfer the local cache as possible via
 * 	a UIDPLUS response.
 *
 * Note, the copied message will never be filtered again if it's moved
 * back to the Inbox.
 *
 * A myriad of things can go wrong here, so give it our best shot.  Since
 * we're dealing with the local cache, that should suffice.
 ************************************************************************/
OSErr IMAPTransferLocalCache(TOCHandle fromTocH, MSumPtr pOrigSum, TOCHandle toTocH, long newUid, Boolean copy)
{		
	OSErr err = noErr;
	long origSumNum, newSumNum;
	MSumType newSum, *pNewSum;
	Boolean bNoSaveCache = false;
	
	// sanity check
	if (!fromTocH || !toTocH || !pOrigSum)
		return (paramErr);

	// determine if we're transferring to a mailbox that's not interested in retaining the cache.
	bNoSaveCache = PrefIsSet(PREF_DROP_TRASH_CACHE) && IsIMAPTrashMailbox(TOCToMbox(toTocH));
	
	// locate original message
	origSumNum = FindSumByHash(fromTocH, pOrigSum->uidHash);
	
	// look in hidden toc if it's not found
	if (origSumNum == -1)
	{
		fromTocH = GetHiddenCacheMailbox(TOCToMbox(fromTocH), false, false);
		if (fromTocH) origSumNum = FindSumByHash(fromTocH, pOrigSum->uidHash);
	}
	
	if (origSumNum != -1)
	{
		// does this message already exist in the cache?  Replace it.
		if ((newSumNum = FindSumByHash(toTocH, newUid)) != -1)
		{
			CopySum(pOrigSum, &(*toTocH)->sums[newSumNum], 0);
			(*toTocH)->sums[newSumNum].flags &= ~FLAG_UNFILTERED;
			(*toTocH)->sums[newSumNum].uidHash = newUid;			// restore the uid. -jdboud 12/16/03
			InvalSum(toTocH,newSumNum);
		}
		else
		{
			// copy the summary and save the cache, if appropriate.
			//
			// Note, if the message being copied has even a single IMAP Stub file,
			// don't copy the cache.  If we did, we'd need to orphan the IMAP stub
			// files, which are completely useless on their own and simplt take
			// up disk space.  Force the user to refetch the message instead, wiping
			// out old stub files.
			
			// is there some message to copy?
			if (!bNoSaveCache && ((pOrigSum->opts&OPT_FETCH_ATTACHMENTS)==0) && (pOrigSum->offset!=imapNeedToDownload))
			{
				// AppendMessage uses the cache if it's around. Make sure's it's not,
				// so the entire message is copied from the cache file on disk. (-39 error bug)
				ZapHandle((*fromTocH)->sums[origSumNum].cache);
				
				// copy the cached message to the new location
				err = AppendMessage(fromTocH,origSumNum,toTocH,true,false,false);
				if (err==noErr)
				{
					pNewSum = &(*toTocH)->sums[((*toTocH)->count)-1];
					
					// set the UID to the new value
					pNewSum->uidHash = newUid;
					
					// grab the same attributes of the original summary
					pNewSum->opts = pOrigSum->opts;
					pNewSum->flags = pOrigSum->flags;
					
					// reset the filter flag
					pNewSum->flags &= ~FLAG_UNFILTERED;
					 
					// orphan the attachments from the new message if this was a copy.  
					// Removing it shouldn't remove the orignal message's attachments.
					if (copy)
						pNewSum->opts |= OPT_ORPHAN_ATT;
						
					// orphan the attachments from the original message.  This way, they
					// won't be cleaned up if we delete the message later.
					(*fromTocH)->sums[origSumNum].opts |= OPT_ORPHAN_ATT;
				}
			}
			else
			{
				// Just copy the summary info
				Zero(newSum);
				CopySum(pOrigSum, &newSum, 0);
				newSum.uidHash = newUid;
				
				// clean up the summary.
				newSum.offset = imapNeedToDownload;
				newSum.bodyOffset = 0;
				newSum.msgIdHash = 0;
				newSum.subjId = 0;
				newSum.mesgErrH = nil;
				newSum.cache = nil;
				newSum.messH = nil;
#ifdef	NOBODY_SPECIAL
				newSum.opts &= ~OPT_JUSTSUB;
#endif	//NOBODY_SPECIAL
				newSum.flags &= ~FLAG_UNFILTERED;
				
				// put the adjusted summary into the mailbox
				if (!SaveMessageSum(&newSum,&toTocH))
					err = 1;
			}
		}
	}		
	// else
		// can't locate original message.  Forgeddaboudid.
		
	return (err);
}

/**********************************************************************
 * IMAPMoveMessageDuringFiltering - transfer a message during filtering
 **********************************************************************/
OSErr IMAPMoveMessageDuringFiltering(TOCHandle fromTocH, short sumNum, TOCHandle toTocH, Boolean copy, FilterPBPtr fpb)
{
	OSErr err = noErr;
	Boolean filteringUnderway = IMAPFilteringUnderway();
	long uid;
	
	uid = (*fromTocH)->sums[sumNum].uidHash;
	
	// deselect the current message, and highlight the previous one.
	(*fromTocH)->sums[sumNum].selected = false;
	InvalSum(fromTocH,sumNum);
	BoxSelectAfter((*fromTocH)->win,sumNum);

	// Filtering takes place in the foreground.  Free up the connection for the upcoming transfer
	if (filteringUnderway) IMAPStopFiltering(false);

	// IMAP to IMAP transfer
	if ((toTocH && (*toTocH)->imapTOC) && ((*fromTocH)->imapTOC))
	{	
		// copy the message from this mailbox to the destination
		err = IMAPTransferMessage(fromTocH,toTocH,uid,true,true);
		if (err==noErr)
		{
			// the destination mailbox has new mail if the transferred message was unread
			if ((*fromTocH)->sums[sumNum].state==UNREAD) IMAPMailboxHasUnread(toTocH, true);
			
			// then mark it as deleted.  We'll expunge when filtering is complete
			if (!copy)
			{
				IMAPMarkMessageAsDeleted(fromTocH, uid, false);
				if (FancyTrashForThisPers(fromTocH)) FlagForExpunge(fromTocH);
			}
		}
	}
	// IMAP to POP transfer
	else if ((toTocH && !(*toTocH)->imapTOC) && ((*fromTocH)->imapTOC))
	{
		// copy this message to the local mailbox
		err = MoveMessageLo(fromTocH,sumNum,&(*toTocH)->mailbox.spec,true,false,true);
		
		// turn off notifications for this message if a translator deleted it
		if (fpb && ((*fromTocH)->sums[sumNum].opts&OPT_EMSR_DELETE_REQUESTED))
		{
			fpb->openMailbox = fpb->openMessage = fpb->print = false;
			fpb->dontReport = fpb->dontUser = true;
		}
		
		// then mark it as deleted.  We'll expunge when filtering is complete
		if ((err==noErr) && !copy) 
		{
			IMAPMarkMessageAsDeleted(fromTocH, uid, false);
			if (FancyTrashForThisPers(fromTocH)) FlagForExpunge(fromTocH);
		}
	}
	// POP to IMAP transfer
	else if ((toTocH && (*toTocH)->imapTOC) && !((*fromTocH)->imapTOC))
	{
		// transfer the message to the IMAP mailbox
		err = IMAPTransferMessageToServer(fromTocH,toTocH,sumNum, copy, true);
	}
	
	// go back to filtering
	if (filteringUnderway) IMAPStartFiltering(fromTocH, true);
	
	return (err);
}

/**********************************************************************
 * CacheIMAPMessageForSpamWatch - cache as much of the given message
 *	as we can.  Simply cache the message if it's downloaded.  If it's
 *	not, fill the local memory cache with the headers of the message.
 *
 *	Note, if we really wanted to, we could fetch parts of the message
 *	body as well for better Bayesian filtering.  Someday, maybe.
 **********************************************************************/
OSErr CacheIMAPMessageForSpamWatch(TOCHandle tocH, short sumNum)
{
	Boolean filteringUnderway;	// to interact better with IMAP filtering
	Handle cache;
				
	// Sanity check
	if (!tocH || !*tocH 				// must have a toc
		|| !(*tocH)->imapTOC 			// it must be an IMAP toc
		|| (sumNum > (*tocH)->count))	// and the sumnum must be valid
		return (paramErr);
		
	// minimal header present
	if ((*tocH)->sums[sumNum].offset == imapNeedToDownload)
	{
		// if this is an IMAP minimal header, go fetch the headers from the server.
		filteringUnderway = IMAPFilteringUnderway();
        if (!filteringUnderway) IMAPStartFiltering(tocH, true);  
        cache = IMAPFetchMessageHeadersForFiltering(tocH, sumNum);
		HPurge(cache);          
        if (!filteringUnderway) IMAPStopFiltering(true);
  
        ZapHandle((*tocH)->sums[sumNum].cache);      
        (*tocH)->sums[sumNum].cache = cache;
	}
	// full message already downloaded
	else
	{
		CacheMessage (tocH, sumNum);
	}
	
	return (noErr);
}

/************************************************************************
 * IMAPWarnings - display pending IMAP warnings once threads complete. 
 *	UIDPLUS and junk warnings, for example.
 ************************************************************************/
void IMAPWarnings(void)
{
	PersHandle pers;
	
	if (gbDisplayIMAPWarnings && !GetNumBackgroundThreads())
	{
		gbDisplayIMAPWarnings = false;
		
		for (pers = PersList; pers; pers = (*pers)->next)
		{
			if (IsIMAPPers(pers))
			{
				PushPers(pers);
				CurPers = pers;
				
				//
				// 	"No SpamWatch because of lacking IMAP support" warning?
				//
				//	Warn if IMAP support is lacking
				//	AND 
				//	we haven't warned before
				//
				
				if (!JunkPrefHasIMAPSupport() 
					&& !JunkPrefNoIMAPSupportWarning())
				{
					// Warn the user that SpamWatch has been disabled.
					ComposeStdAlert(Note,JUNK_PREFNOIMAP_WARNING,(*pers)->name);
					// Don't warn again.
					SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefNoIMAPSupportWarning);
					// But do warn about support that appears in the future
					SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)&~bJunkPrefNewIMAPSupportWarning);
				}
				
				//
				//	"IMAP Support for SpamWatch is available"  warning?
				//
				//	Warn if UIDPLUS is available 
				//	AND we warned about lacking UIDPLUS sipport in the past 
				//  AND we haven't warned about the new support.
				//
				
				if (!PrefIsSet(PREF_NO_USE_UIDPLUS) && JunkPrefHasIMAPSupport()
					&& JunkPrefNoIMAPSupportWarning() 
					&& !JunkPrefNewIMAPSupportWarning())
				{
					// Warn the user that SpamWatch has been re-enabled
					ComposeStdAlert(Note,JUNK_PREFIMAPAVAIL_WARNING,(*pers)->name);
					// Don't warn again.  Ever.  Even if it appears to go away.
					SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefNewIMAPSupportWarning);
					SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefNoIMAPSupportWarning);
				}
				
				PopPers();
			}
		}
	}
}

/************************************************************************
 * IMAPSpamWatchSupported - enable or disable SpamWatch
 ************************************************************************/
void IMAPSpamWatchSupported(Boolean bSupported, Boolean bWarnIfNeeded)
{
	//
	// it's not supported, and we haven't warned before
	//
	if (!bSupported && !JunkPrefNoIMAPSupportWarning())
	{
		// turn off SpamWatch plugins
		SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefIMAPNoRunPlugins);
		gbDisplayIMAPWarnings = bWarnIfNeeded;
	}
	//
	// it is supported, and we've not yet noted that
	//
	else if (bSupported && !JunkPrefHasIMAPSupport() && !PrefIsSet(PREF_NO_USE_UIDPLUS))
	{
		// remember that we (now) have IMAP support
		SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefHasIMAPSupport);
		
		if (JunkPrefNoIMAPSupportWarning())
		{
			// We've warned about lacking IMAP Support in the past.  Warn about the new support.
			SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)&~bJunkPrefIMAPNoRunPlugins);
			gbDisplayIMAPWarnings = bWarnIfNeeded;
		}
		else
		{
			// we haven't warned in the past.  Never warn if UIDPLUS support appears to go away.
			SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefNoIMAPSupportWarning);
			SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefNewIMAPSupportWarning);
		}
	}
}

/************************************************************************
 * IMAPResetSpamSupportPrefs - reset all preferences related to support
 *	of SpamWatch on IMAP
 ************************************************************************/
void IMAPResetSpamSupportPrefs(void)
{
	SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)&~bJunkPrefHasIMAPSupport);
	SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)&~bJunkPrefNoIMAPSupportWarning);
	SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)&~bJunkPrefNewIMAPSupportWarning);
}

/**********************************************************************
 *	IMAPPoll - poll for new messages
 **********************************************************************/
void IMAPPoll(PersHandle pers)
{
	threadDataHandle index;
		
	// Something bad happened.  We weren't told which mailbox to personality to poll.
	if (!pers) return;
		
	// we're Offline.  Don't do anything.
	if (Offline) return;
	
	// do nothing if there's already a polling operation underway
	for (index=gThreadData;index;index=(*index)->next)
		if (((*index)->imapInfo.command == IMAPPollingTask) && ((*index)->imapInfo.targetBox == (*pers)->mailboxTree))
			return;
			
	// remove any old task errors
	RemoveTaskErrors(IMAPPollingTask,(*pers)->persId);
	
	// poll away
	IMAPPollMailboxes((*pers)->mailboxTree);
}

/************************************************************************
 * IMAPPollMailboxes - poll the mailboxes of a personality for new mail
 ************************************************************************/
void IMAPPollMailboxes(MailboxNodeHandle tree)
{
	IMAPStreamPtr imapStream = nil;
	Str255 s;
	PersHandle pers = FindPersById((*tree)->persId);
	long numToPoll, remaining;
	
	// how many boxes must we poll?
	numToPoll = remaining = IMAPCountMailboxes(tree, kNeedsPoll);
	if (numToPoll)
	{
		PROGRESS_START;
		ComposeRString(s,IMAP_POLLING_MAILBOXES,(*pers)->name);		
		PROGRESS_MESSAGE(kpTitle,s);
	
		// open an imap stream to the server.			
		imapStream = GetIMAPConnection(IMAPPollingTask, CAN_PROGRESS);
		if (imapStream)
		{		
			IMAPPollMailboxTree(imapStream, tree, numToPoll, &remaining);					
			CleanupConnection(&imapStream);
		}
					
		PROGRESS_END;	
	}
}

/************************************************************************
 * IMAPPollMailboxTree - check a tree for new messages
 ************************************************************************/
void IMAPPollMailboxTree(IMAPStreamPtr imapStream, MailboxNodeHandle tree, long numToPoll, long *remaining)
{
	short numMessages;
	TOCHandle tocH, hidTocH;
	Str63 m;
	Str255 s;
	WindowPtr behindWP = OpenBehindMePlease();
	MailboxNodeHandle inbox = LocateInboxForPers(CurPers);
	
	while (tree)
	{
		// stop if command period was hit
		if (CommandPeriod)
			return;
	
		LockMailboxNodeHandle(tree);
			
		// is this an IMAP mailbox that can contain messages?
		if (!((*tree)->attributes & LATT_NOSELECT) && !((*tree)->attributes & LATT_ROOT))
		{	
			// does it need to be polled?
			if (DoesIMAPMailboxNeed(tree, kNeedsPoll))
			{	
				// display some progress indicating which mailbox we're polling
				PathToMailboxName((*tree)->mailboxName, m, (*tree)->delimiter);
				ComposeRString(s,IMAP_POLL_MAILBOX,m);
				PROGRESS_MESSAGE(kpSubTitle,s);
				BYTE_PROGRESS(nil,numToPoll-(*remaining),numToPoll);
				(*remaining)--;
				
				// how many messages are in the local mailbox cache?
				tocH = TOCBySpec(&(*tree)->mailboxSpec);
				if (tocH)
				{
					// make sure this toc doesn't go anywhere.
					IMAPTocHBusy(tocH, true);
					
					// is it not currently being synched or scheduled to be?
					if (!DoesIMAPMailboxNeed(tree, kNeedsResync))
					{
						numMessages = (*tocH)->count;

						// check the hidden toc, too
						if ((hidTocH=GetHiddenCacheMailbox(tree, false, false))!=NULL)
							numMessages += (*hidTocH)->count;
				
						// how many messages does the server think are in the mailbox?
						if (IMAPOpenMailbox(imapStream, (*tree)->mailboxName,false))
						{
							// are there more than the local cache knows about?
							if(GetMessageCount(imapStream) != numMessages)
							{
								// fetch new messages in this mailbox.
								FetchNewMessages(tocH, true, true, false, true);
								
								// Open the mailbox if the preference is set.
								if (!PrefIsSet(PREF_NO_OPEN_IN))
								{
									ShowBoxAt(tocH,(*tocH)->previewPTE ? -1:FumLub(tocH),behindWP);
									if (PrefIsSet(PREF_ZOOM_OPEN)) ReZoomMyWindow(GetMyWindowWindowPtr((*tocH)->win));
									behindWP = GetMyWindowWindowPtr((*tocH)->win);
								}	
							}
						}
					}
					
					// don't need the toc anymore
					IMAPTocHBusy(tocH, false);
				}
			}
		}
		// check this node's children
		if ((*tree)->childList) IMAPPollMailboxTree(imapStream, (*tree)->childList, numToPoll, remaining);
		
		UnlockMailboxNodeHandle(tree);
		
		// check the next node.
		tree = (*tree)->next;
	}
}

/**********************************************************************
 * 	Code to prevent virus scanners and other stupid software from 
 * screwing us up when we're decoding IMAP messages.
 *
 *	IMAP downloads its messages to a temp file in the spool folder.
 * During idle time, Eudora goes through the Spool folder looking
 * for these temp files.  When one is found, the messages are decoded
 * and placed in the proper IMAP mailbox.  The temp file is then erased
 *
 *	Now the bad part: certain virus scanners like Norton AnitVirus
 * look for viruses in messages and scan the spool folder.  In some
 * cases, the scanner may open the file as read only before Eudora
 * gets a chance to delete it.  If that happens, we'll be unable to
 * delete the file and the messages may be copied into the mailbox
 * many times.
 *
 *	The correct solution is to turn off you virus software.  The last
 * virus I saw was 'WDEF' in 1987 and it didn't even arrive via email.
 *
 *	As we're in the business of offering alternative solutions, the
 * code below will work around the problem with minimal impact on the
 * folks out there smart enough not to get into this predicament in
 * the first place:
 *
 *	Once an IMAP message temp file is processed, MarkAsProcessed()
 * marks the file as processed.  In the idle loop looking for temp 
 * files, look for the processed resource with HasBeenProcessed().  
 * If it's there, just toss the file.
 **********************************************************************/

/**********************************************************************
 * MarkAsProcessed - set the label of this temp file so we know it's
 *	been processed.
 **********************************************************************/
void MarkAsProcessed(FSSpec *spec)
{
	// set the label of this file so we know it's been processed.
	FixSpecUnread(spec,true);
}

/**********************************************************************
 * HasBeenProcessed - returns true if the messages in this temp
 *	file have already been prceesed. 
 **********************************************************************/
Boolean HasBeenProcessed(FSSpec *spec)
{
	Boolean bHas = false;
	OSErr err = noErr;
	CInfoPBRec mailboxFileInfo;

	//	Find the mailbox cache file on disk ...
	Zero(mailboxFileInfo);
	err = HGetCatInfo(spec->vRefNum,spec->parID,spec->name,&mailboxFileInfo);
	if (err == noErr)		
		bHas = (mailboxFileInfo.hFileInfo.ioFlFndrInfo.fdFlags&0xe) != 0;
	
	return (bHas);
}

/**********************************************************************
 * Return TRUE when it's all right to quit.  Process pending IMAP
 * changes while we're waiting.
 **********************************************************************/
Boolean IMAPDoQuit(void)
{
	Boolean bCanQuit = true;
	TOCHandle tocH;
	MailboxNodeHandle mBox;
	
	// first off, cancel filtering.  We can resume that when Eudora next starts up.
	gFilteringCancelled = true;
	
	// next, wait for all background threads to complete
	if (GetNumBackgroundThreads()) return (false);
	
	// finally, perform all waiting mailbox updates
	for (tocH=TOCList; tocH; tocH = (*tocH)->next)
	{
		// is this an IMAP mailbox?
		mBox = TOCToMbox(tocH);
		if (mBox)
		{
			// is this mailbox waiting for updates?
			if ((*mBox)->tocRef)
			{
				// perform them.
				UpdateIMAPMailbox(tocH);
				
				// we can't quit yet.
				bCanQuit = false;
			}
			else
			{
				// forget about filtering and resyncs.  Perform those later.
				SetIMAPMailboxNeeds(mBox, kNeedsFilter, false);
				SetIMAPMailboxNeeds(mBox, kNeedsResync, false);
			}
		}
	}
	
	// we're done!  We can quit.
	return (bCanQuit);
}

#ifdef	DEBUG
/**********************************************************************
 * IMAPCollectFlags() - fetch flags for all IMAP mailboxes, allow cancel
 **********************************************************************/
void IMAPCollectFlags(void)
{
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	SignedByte state;
	Str255 p;
	short flagsFileRefN = OpenFlagsFile();
	
	if (flagsFileRefN > 0)
	{
		OpenProgress();
		ProgressMessage(kpTitle,"\pCollecting flags from mailboxes");
		
		for (CurPers = PersList; CurPers && !CommandPeriod; CurPers = (*CurPers)->next)
		{
			if (MailboxTreeGood(CurPers))
			{
				ComposeString(p, "\p%p", (*CurPers)->name);
				PROGRESS_MESSAGE(kpSubTitle,p);
				
				imapStream = GetIMAPConnection(IMAPResyncTask, CAN_PROGRESS);
				if (imapStream) 
				{	
					state = HGetState((Handle)CurPers);
					LDRef(CurPers);
					IMAPCollectFlagsFromTree(imapStream,(*CurPers)->mailboxTree, flagsFileRefN);
					HSetState(CurPers, state);
				}
				CleanupConnection(&imapStream);
			}
		}
		CurPers = oldPers;
		
		CloseProgress();
		MyFSClose(flagsFileRefN);
	}
}

/**********************************************************************
 * IMAPCollectFlagsFromTree() - fetch flags for all IMAP mailboxes in 
 *	the tree
 **********************************************************************/
void IMAPCollectFlagsFromTree(IMAPStreamPtr imapStream, MailboxNodeHandle *tree, short refN)
{
	MailboxNodeHandle scan = tree;
	Str255 p;
	
	while (scan && !CommandPeriod)
	{
		LockMailboxNodeHandle(scan);
	
		ComposeString(p, "\pScanning %p.", (*scan)->mailboxSpec.name);
		ProgressMessage(kpSubTitle,p);
			
		// open the mailbox on the server
		if (IMAPOpenMailbox(imapStream, (*scan)->mailboxName,false))
		{
			// open a file to dump the flags into
			imapStream->mailStream->flagsRefN = refN;
						
			// fetch all the flags
			UIDFetchFlags(imapStream, nil);
			
			// ignore the file.
			imapStream->mailStream->flagsRefN = -1;
		}
		
		if ((*scan)->childList) IMAPCollectFlagsFromTree(imapStream, (*scan)->childList, refN);
		
		UnlockMailboxNodeHandle(scan);
		scan = (*scan)->next;
	}
}

/************************************************************************
 *	OpenMessDestFile - create and open a temp file for a mailbox.
 *		We'll close the file when the imapStream is zapped.
 ************************************************************************/
short OpenFlagsFile(void)
{
	short ref = -1;
	OSErr err = noErr;
	FSSpec flagsSpec;
	const unsigned char *flagFileName = "\pIMAPFlagUsage";
	Str255 ctext;
	long creator;
	
	// create (or open) the spool file in the Eudora folder.     
	err = FSMakeFSSpec(Root.vRef,Root.dirId,flagFileName,&flagsSpec);
	if (err == fnfErr)
	{
		// not found in root folder.  Create the file.
		GetPref(ctext,PREF_CREATOR);
		if (*ctext!=4) GetRString(ctext,TEXT_CREATOR);
		BMD(ctext+1,&creator,4);
		err = HCreate(flagsSpec.vRefNum,flagsSpec.parID,flagsSpec.name,creator,'TEXT');
	}
		
	// open the file
	if (err == noErr) err = AFSHOpen(flagsSpec.name,flagsSpec.vRefNum,flagsSpec.parID,&ref,fsRdWrPerm);		
	
	if ((err != noErr) || (ref == -1))
	{
		WarnUser(NO_TEMP_FILE, err);
		ref = -1;
	}
	else SetFPos(ref,fsFromLEOF,0);
	
	return (ref);
}					
#endif

#endif	//IMAP