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
 *	imapmailboxes.h
 *
 *		This file contains declarations for the functions that maintain
 *	a list of remote IMAP mailboxes.
 *
 **********************************************************************/

#ifndef IMAPMAILBOXES_H
#define IMAPMAILBOXES_H

// MailboxNeedsEnum - what a mailboxNode needs
typedef enum
{
	kNeedsNothing	= 0x00000000,		// mailbox is idle
	kNeedsFilter	= 0x00000001,		// mailbox needs to be filtered
	kNeedsResync	= 0x00000002,		// mailbox needs to be resynced
	kNeedsExpunge	= 0x00000004,		// mailbox needs to be expunged
	kNeedsSelect	= 0x00000008,		// need to select new mail in this mailbox
	kNeedsSortLock	= 0x00000010,		// need to disable sorting in this mailbox
	kNeedsPoll		= 0x00000020,		// need to poll this mailbox
	kNeedsExecCmd	= 0x00000040,		// need to execute queued commands for this mailbox
	kNeedsAutoExp	= 0x00000080,		// this mailbox has deleted messages and should be expunged
	kNeedsSearch	= 0x00000100,		// this mailbox has changed and incremental searches should be updated
	kShowDeleted	= 0x00010000,		// show deleted messages in this mailbox		
} MailboxNeedsEnum;

// IMAP message flags
typedef enum
{
	kDeleted=1,
	kSeen,
	kFlagged,
	kAnswered,
	kDraft,
	kRecent,
	kLastImapFlag
} IMAPMessFlagEnum;

// LocalFlagChangeStruct.  Used to keep track changes to IMAP message flags in the local cache.
typedef struct LocalFlagChangeStruct LocalFlagChangeStruct, *LocalFlagChangePtr, **LocalFlagChangeHandle;
struct LocalFlagChangeStruct
{
	unsigned long uid;              // uid of affected message
	UIDVALIDITY mailbox;            // UIDVALIDITY of mailbox that contains this message

	// Note, these MUST be in the same order as in IMAPFLAGS!
	unsigned short deleted:1;
	unsigned short seen:1;          
	unsigned short flagged:1;
	unsigned short answered:1;
	unsigned short draft:1;
	unsigned short recent:1;

	unsigned short unUsed:8;		// spare bits
	
	unsigned short trashed:1;		// true if the message is to be trashed
	unsigned short processed:1;		// used to mark change as processed
};

// Mailboxnode - the thing mailboxTrees are made of
typedef struct MailboxNode MailboxNode, *MailboxNodePtr, **MailboxNodeHandle;
struct MailboxNode
{
	char* mailboxName;						// name of the mailbox on the server
	char delimiter;							// delimiter
	long attributes;						// IMAP mailbox attributes
	UIDVALIDITY uidValidity;				// uidValidity of the mailbox
	unsigned long messageCount;				// number of messages known to be in the mailbox
	FSSpec mailboxSpec;						// where the local mailbox cache lives
	MailboxNodeHandle next;					// next mailbox in the same directory
	MailboxNodeHandle childList;			// first child

	LocalFlagChangeHandle queuedFlags;		// list of pending flag changes for this mailbox
	Boolean flagLock;						// lock the list of flags when processing them

	MailboxNeedsEnum mailboxneeds;			// pending IMAP operations

	short lockCount;						// number of threads that have this mailbox locked
	
	long persId;							// who does this mailbox belong to?
	long tocRef;							// references to this mailbox's TOC Handle.
	
	long searchUID;							// last UID searched to in this mailbox.
};

// IMAPMailboxAttributes - structure to return attributes of an IMAP mailbox
typedef struct IMAPMailboxAttributes IMAPMailboxAttributes, *IMAPMailboxAttributesPtr;
struct IMAPMailboxAttributes
{
	Boolean noSelect;				// this mailbox can't be selected (it's a simple folder)
	Boolean noInferiors;			// this mailbox can't have children (it's a simple mailbox)
	Boolean marked;					// this mailbox is marked
	Boolean unmarked;				// this mailbox is unmarked
	Boolean hasChildren;			// this mailbox actually has children
	Boolean hasNoChildren;			// the server claims this mailbox has no children
	unsigned long messageCount;		// the number of messages in the mailbox at last check
	long latt;						// what type of IMAP mailbox is this?
};

// Mailbox Cache
void AddMailbox(MAILSTREAM *mailStream, char *name, char delimiter, long attributes);
Boolean GetIMAPMailboxes(IMAPStreamPtr imapStream, Boolean progress);
OSErr CreateNewPersCaches(void);
void CreateIMAPMailFolder(void);
void RebuildAllIMAPMailboxTrees(void);
Boolean IsSpecialIMAPName(unsigned char *name, Boolean *bIsDir);
Boolean IsIMAPSubPers(FSSpecPtr spec);
OSErr ReadIMAPMailboxAttributes(FSSpecPtr spec, long *attributes);
void PathToMailboxName(CStr path, Str255 mboxName, char delimiter);
void PersNameToCacheName(PersHandle pers, UPtr cacheName);
OSErr CreateLocalCache(void);
void DisposePersIMAPMailboxTrees(void);
void DisposeMailboxTree(MailboxNodeHandle *tree);
void ZapMailboxNode(MailboxNodeHandle *node);
Boolean IsIMAPCacheName(unsigned char *name);
OSErr WriteIMAPMailboxInfo(FSSpecPtr spec, MailboxNodeHandle node);
Boolean IMAPExists(void);
OSErr IMAPRefreshAllCaches(void);
OSErr IMAPRefreshPersCaches(void);
OSErr RemoveIMAPCacheDir(FSSpec toDelete);
Boolean CanModifyMailboxTrees(void);
OSErr UpdateLocalCache(Boolean progress);
Boolean MailboxTreeGood(PersHandle pers);
OSErr GetIMAPAttachFolder(FSSpecPtr attachSpec);
MailboxNodeHandle TOCToMbox(TOCHandle tocH);
PersHandle TOCToPers(TOCHandle tocH);
PersHandle MailboxNodeToPers(MailboxNodeHandle mbox);
void SalvageIMAPTOC(TOCHandle oldTocH, TOCHandle newTocH, short *newCount);
void LockMailboxNodeHandle(MailboxNodeHandle node);
void UnlockMailboxNodeHandle(MailboxNodeHandle node);
void ClosePersMailboxes(PersHandle pers);
void IMAPCloseChildMailboxes(FSSpecPtr spec);
Boolean IMAPMailboxHasUnread(TOCHandle tocH, Boolean itDoesNow);
void PathToMailboxName (CStr path, Str63 mboxName, char delimiter);
OSErr UpdateIMAPMailboxInfo(TOCHandle tocH);
short UIDToSumNum(unsigned long uid, TOCHandle tocH);
Boolean IMAPMailboxTitle(TOCHandle tocH, Str255 title);
void IMAPPersIDChanged(PersHandle pers, MailboxNodeHandle tree);

// Mailbox Information
Boolean IsIMAPMailboxFile(FSSpecPtr spec);
Boolean IsIMAPMailboxFileLo(FSSpecPtr spec, MailboxNodeHandle *node);
Boolean IsIMAPCacheFolder(FSSpecPtr spec);
Boolean IsIMAPVD(short vRef, long dirId);
void LocateNodeBySpecInAllPersTrees(FSSpecPtr spec, MailboxNodeHandle *node, PersHandle *pers);
MailboxNodeHandle LocateNodeBySpec(MailboxNodeHandle tree, FSSpecPtr spec);
MailboxNodeHandle LocateNodeByMailboxName(MailboxNodeHandle tree, char *mailboxName);
Boolean MailboxAttributes(FSSpecPtr spec, IMAPMailboxAttributesPtr attributes);
Boolean CanHaveChildren(MailboxNodeHandle node);
Boolean IsIMAPMailboxEmpty(FSSpecPtr mailboxSpec);
long IMAPMailboxMessageCount(FSSpecPtr mailboxSpec, Boolean check);
Boolean ReallyIsIMAPMailbox(FSSpecPtr spec);
void SetIMAPMailboxNeeds(MailboxNodeHandle node, MailboxNeedsEnum flag, Boolean on);
#define DoesIMAPMailboxNeed(n,f) (n ? ((*n)->mailboxneeds&f)!=0 : false)
long IMAPCountMailboxes(MailboxNodeHandle tree, MailboxNeedsEnum needs);
#define IMAPTOCInUse(n) (n ? (DoesIMAPMailboxNeed(n,kNeedsFilter) || DoesIMAPMailboxNeed(n,kNeedsResync) || ((*n)->tocRef > 0)) : false)

// Mailbox Creation and Deletion
Boolean IMAPAddMailbox(FSSpecPtr spec, Boolean folder, Boolean *success, Boolean silent);
Boolean IMAPDeleteMailbox(FSSpecPtr toDelete);
Boolean IMAPRenameMailbox(FSSpecPtr spec, UPtr name);
Boolean IMAPMoveMailbox(FSSpecPtr fromFolderSpec, FSSpecPtr toSpec, Boolean lastToMove, Boolean oneMoved, Boolean *dontWarn);

// Special IMAP mailboxes, Trash and Junk
Boolean IsIMAPSpecialMailbox(MailboxNodeHandle node, long att);
#define IsIMAPTrashMailbox(node) IsIMAPSpecialMailbox(node, LATT_TRASH)
#define IsIMAPJunkMailbox(node) IsIMAPSpecialMailbox(node, LATT_JUNK)
MailboxNodeHandle GetSpecialMailbox(PersHandle pers, Boolean createIfNeeded, Boolean silent, long mboxAtt);
#define GetIMAPTrashMailbox(pers, createIfNeeded, silent) GetSpecialMailbox(pers, createIfNeeded, silent, LATT_TRASH)
#define GetIMAPJunkMailbox(pers, createIfNeeded, silent) GetSpecialMailbox(pers, createIfNeeded, silent, LATT_JUNK)
TOCHandle LocateIMAPJunkToc(TOCHandle tocH, Boolean createIfNeeded, Boolean silent);
void ResetSpecialMailbox(PersHandle pers, long mboxAtt);
#define ResetTrashMailbox(pers) ResetSpecialMailbox(pers, LATT_TRASH)
#define ResetJunkMailbox(pers) ResetSpecialMailbox(pers, LATT_JUNK)
Boolean EnsureSpecialMailboxes(PersHandle pers);

// Delete Messages cache
TOCHandle GetHiddenCacheMailbox(MailboxNodeHandle mbox, Boolean bForce, Boolean bCreateIfNeeded);
OSErr HideDeletedMessages(MailboxNodeHandle mbox, Boolean bForce, Boolean bShow);
short CountDeletedIMAPMessages(TOCHandle tocH);
Boolean HideShowSummary(TOCHandle toc, TOCHandle tocH, TOCHandle hidTocH, short sumNum);

// IMAP Auto Expunge
#define bIMAPAutoExpungeDisabled	(1<<0)
#define bIMAPAutoExpungeAlways		(1<<1)
#define bIMAPAutoExpungeNoThreshold	(1<<2)
#define bIMAPAutoExpungeWarned		(1<<9)
#define IMAPAutoExpungeDisabled() 	(GetPrefBit(PREF_IMAP_AUTOEXPUNGE,bIMAPAutoExpungeDisabled))
#define IMAPAutoExpungeAlways() 	(GetPrefBit(PREF_IMAP_AUTOEXPUNGE,bIMAPAutoExpungeAlways))
#define IMAPAutoExpungeThreshold() 	(!GetPrefBit(PREF_IMAP_AUTOEXPUNGE,bIMAPAutoExpungeNoThreshold))
#define IMAPAutoExpungeWarned() 	(GetPrefBit(PREF_IMAP_AUTOEXPUNGE,bIMAPAutoExpungeWarned))

void IMAPAutoExpungeWarning(void);
Boolean IMAPAutoExpunge(void);
Boolean IMAPAutoExpungeMailbox(TOCHandle tocH);
void MarkSumAsDeleted(TOCHandle tocH, short sumNum, Boolean bDeleted);

// Hidden Summary Filtering
Boolean ShowHideFilteredSummary(TOCHandle toc, short sumNum);
MailboxNodeHandle GetRealIMAPSpec(FSSpec orig, FSSpecPtr spec);

// IMAP Fancy Trash Mode
Boolean IMAPEmptyTrash(Boolean localOnly, Boolean currentOnly, Boolean all);
short IMAPCountTrashMessages(Boolean localOnly, Boolean currentOnly, Boolean all);
Boolean FancyTrashForThisPers(TOCHandle tocH);

// Other
Boolean SpecIsFilled(FSSpecPtr spec);
MailboxNodeHandle LocateInboxForPers(PersHandle pers);
Boolean IMAPDontAutoFccMailbox(TOCHandle tocH);

#endif	//IMAPMAILBOXES_H