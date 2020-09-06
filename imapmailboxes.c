/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Keychain.h>

#include <conf.h>
#include <mydefs.h>

/* Copyright (c) 1998 by QUALCOMM Incorporated */
#define FILE_NUM 113

/**********************************************************************
 *	imapmailboxes.c
 *
 *		This file contains the functions necessary to keep track of the
 *	remote IMAP mailboxes.  Everything defined here is designed to work
 *	in the main thread only.
 **********************************************************************/

#include "imapmailboxes.h"

#pragma segment IMAP

#define IMAP_TYPE 'IMAP'
#define BOX_INFO_TYPE 'imap'
#define BOX_NAME_TYPE 'inam'
#define BOX_FLAGS_TYPE 'ifag'
#define IMAP_ID 128

// mailbox error codes
enum {
	IMAPParamErr = 1,
	IMAPTreeInUse
};

MailboxNodeHandle GetIMAPMailboxLevel(IMAPStreamPtr imapStream,
				      const char *ref,
				      Boolean includeInbox,
				      Boolean progress);
OSErr UpdateLocalCacheMailboxes(MailboxNodeHandle * tree, FSSpecPtr inSpec,
				Boolean progress);
Boolean CreateIMAPCacheMailbox(FSSpecPtr spec, Boolean folder);
OSErr CleanIMAPFolder(void);
OSErr CleanCacheFolder(FSSpecPtr folderToClean,
		       MailboxNodeHandle treeToClean);
OSErr ReadIMAPMailboxInfo(FSSpecPtr spec, MailboxNodeHandle node);
OSErr RebuildIMAPMailboxTree(FSSpecPtr dir, PersHandle pers,
			     MailboxNodeHandle * tree,
			     Boolean * bHasQueuedCommands);
MailboxNodeHandle RebuildPersIMAPMailboxTree(PersHandle pers);
Boolean StrCmpNotIncludingEndingDelimiter(UPtr str1, UPtr str2,
					  char delimiter);
void LocateNodeByVDInAllPersTrees(short vRef, long dirId,
				  MailboxNodeHandle * node,
				  PersHandle * pers);
MailboxNodeHandle LocateNodeByVD(MailboxNodeHandle tree, short vRef,
				 long dirId);
MailboxNodeHandle LocateParentNode(MailboxNodeHandle tree,
				   MailboxNodeHandle child);
MailboxNodeHandle LocateParentNodeInChildren(MailboxNodeHandle tree,
					     MailboxNodeHandle child);
OSErr BuildIMAPMailboxName(MailboxNodeHandle parent, FSSpecPtr newSpec,
			   Boolean folder, CStr * newMailboxName);
Boolean DeleteIMAPMailboxNode(IMAPStreamPtr imapStream,
			      MailboxNodeHandle node);
Boolean FetchDelimiter(IMAPStreamPtr imapStream, MailboxNodeHandle node);
void TransferLocalTreeInfo(MailboxNodeHandle oldTree,
			   MailboxNodeHandle newTree);
char *NewIMAPMailboxName(MailboxNodeHandle node, UPtr name, char *newName);
MailboxNodeHandle LocateSpecialMailbox(MailboxNodeHandle tree,
				       long mboxAtt);
Boolean ChooseSpecialMailbox(PersHandle pers, short msg,
			     FSSpecPtr specialSpec);
OSErr EnsureIMAPCacheFolders(FSSpecPtr attach, FSSpecPtr imapStubSpec);
void AttachOpenTocsToIMAPMailboxTrees(void);
MailboxNodeHandle CreateSpecialMailbox(Boolean tryCreate, long mboxAtt);
#define SpecialMailboxIsTrash(a) (a == LATT_TRASH)
#define SpecialMailboxIsJunk(a)	(a == LATT_JUNK)
Boolean IMAPMailboxExists(Str255 mailboxName);
Boolean GetNextMailboxToExpunge(MailboxNodeHandle tree, FSSpec * spec);
Boolean MarkOrExpungeMailboxIfNeeded(MailboxNodeHandle mbox, FSSpec * spec,
				     Boolean bNow);
void CloseIMAPMailboxImmediately(TOCHandle tocH, Boolean bHiddenToo);
OSErr CleanHiddenCacheMailbox(TOCHandle hidTocH);

/**********************************************************************
 * CreateIMAPMailFolder - make the folder to store the IMAP cache in
 **********************************************************************/
void CreateIMAPMailFolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	OSErr err;
	Boolean isFolder, boolJunk;

	// create the IMAP Folder
	err =
	    FSMakeFSSpec(Root.vRef, Root.dirId,
			 GetRString(folder, IMAP_MAIL_FOLDER_NAME), &spec);
	if (err == fnfErr)
		err = FSpDirCreate(&spec, smSystemScript, &junk);

	if (!err) {
		err = ResolveAliasFile(&spec, True, &isFolder, &boolJunk);
		if (!err && !folder)
			err = fnfErr;
	}
	if (err)
		DieWithError(MAIL_FOLDER, err);

	IMAPMailRoot.vRef = spec.vRefNum;
	IMAPMailRoot.dirId = SpecDirId(&spec);

	// clean up the IMAP Cache folder, removing all unused personality caches
	CleanIMAPFolder();
}


/**********************************************************************
 *	AddMailbox - this is called once per mailbox when receiving status
 *		stuff from the server.  Add the mailbox to the current pers
 *		IMAPmailboxtree.
 **********************************************************************/
void AddMailbox(MAILSTREAM * mailStream, char *name, char delimiter,
		long attributes)
{
	MailboxNodeHandle mbox = 0;
	char *mailboxName = 0;
	Str255 scratch, inbox;
	Str255 progressMessage;
	static long lastProgress = 0;

	GetRString(scratch, IMAP_INBOX_NAME);
	PtoCcpy(inbox, scratch);

	// if this mailbox is the same as the ref, then ignore it.  Some servers do this.  bxxxxxxs.
	if (mailStream->fRef
	    && !StrCmpNotIncludingEndingDelimiter(name,
						  (char *) (mailStream->
							    fRef),
						  delimiter))
		return;

	// Make sure we're not adding a duplicate INBOX
	if (*name && (strlen(name) == strlen(inbox))
	    && !striscmp(name, inbox)) {
		if (mailStream->fIncludeInbox)
			mailStream->fIncludeInbox = false;
		else
			return;	//INBOX has already been added.
	}
	// display some periodic progress, once a secod
	if (mailStream->fProgress) {
		if ((TickCount() - lastProgress) > 30) {
			PathToMailboxName(name, progressMessage,
					  delimiter);
			PROGRESS_MESSAGE(kpMessage, progressMessage);
			lastProgress = TickCount();
		}
	}
	// create a new node for the mailbox list
	mbox = NewZH(MailboxNode);

	if (mbox) {
		// Should we strip delimiters from the mailbox name?
		mailboxName = cpystr(name);

		(*mbox)->mailboxName = mailboxName;
		(*mbox)->delimiter = delimiter;
		(*mbox)->attributes = attributes;
		(*mbox)->uidValidity = 0;	// this is the default uid_validity.  It's bogus.
		(*mbox)->messageCount = 0;	// we also don't know how many messages are in this mailbox until we SELECT it.
		(*mbox)->next = (*mbox)->childList = 0;
		(*mbox)->queuedFlags = 0;
		(*mbox)->flagLock = false;
		(*mbox)->persId = (*CurPersSafe)->persId;

		// add this mailbox to the list.
		LL_Queue(mailStream->fListResultsHandle, mbox,
			 (MailboxNodeHandle));
	} else
		WarnUser(MEM_ERR, MemError());

}

/**********************************************************************
 *	GetIMAPMailboxes - get a list of mailboxes on the server pointed
 *		to by the specifiec personality
 **********************************************************************/
Boolean GetIMAPMailboxes(IMAPStreamPtr imapStream, Boolean progress)
{
	Str255 ref, scratch;
	MailboxNodeHandle root = nil;

	// Create a new mailStream structure within imapStream.  Our results will end up in there.
	if (imapStream->mailStream == nil) {
		mail_new_stream(&imapStream->mailStream,
				imapStream->pServerName,
				&(imapStream->portNumber), NULL);
		if (!imapStream->mailStream)
			return (false);
	}
	// kill the old imapStream->mailStream->fListResultsHandle
	if (imapStream->mailStream->fListResultsHandle) {
		LDRef(imapStream->mailStream->fListResultsHandle);
		DisposeMailboxTree(&
				   (imapStream->mailStream->
				    fListResultsHandle));
		UL(imapStream->mailStream->fListResultsHandle);
	}
	// create the first node.  This will be a root node pointing to the cache folder.
	root = NewZH(MailboxNode);

	if (!root)
		WarnUser(MEM_ERR, MemError());
	else {
		// set up the root node ...
		(*root)->attributes = LATT_ROOT;
		(*root)->persId = (*CurPers)->persId;

		// go fetch the mailbox list
		imapStream->mailStream->fListResultsHandle =
		    GetIMAPMailboxLevel(imapStream,
					PtoCcpy(ref,
						GetRString(scratch,
							   IMAP_MAILBOX_LOCATION_PREFIX)),
					!PrefIsSet
					(PREF_IMAP_SKIP_TOP_LEVEL_INBOX),
					progress);

		// if successful, push the root node into the queue ...
		if (imapStream->mailStream->fListResultsHandle)
			LL_Push(imapStream->mailStream->fListResultsHandle,
				root);
		// otherwise, throw away the root node we made.
		else
			ZapMailboxNode(&root);
	}

	// don't return anything if command period was pressed ...
	if (CommandPeriod && imapStream->mailStream->fListResultsHandle) {
		LDRef(imapStream->mailStream->fListResultsHandle);
		DisposeMailboxTree(&
				   (imapStream->mailStream->
				    fListResultsHandle));
		UL(imapStream->mailStream->fListResultsHandle);
	}
	// get the mailbox hierarchy
	return (imapStream->mailStream->fListResultsHandle != 0);
}

/**********************************************************************
 *	GetIMAPMailboxLevel - list all the child mailboxes of ref
 **********************************************************************/
MailboxNodeHandle GetIMAPMailboxLevel(IMAPStreamPtr imapStream,
				      const char *ref,
				      Boolean includeInbox,
				      Boolean progress)
{
	MailboxNodeHandle thisLevel = 0;
	MailboxNodeHandle node = 0;

	// Remember the ref.  Some servers like to return it as a child mailbox.  bxxxxxxs.
	imapStream->mailStream->fRef = ref;
	imapStream->mailStream->fIncludeInbox = includeInbox;
	imapStream->mailStream->fProgress = progress;

	// Get a list of all the mailboxes at this level
	if (IMAPListUnSubscribed(imapStream, ref, includeInbox)) {
		thisLevel = imapStream->mailStream->fListResultsHandle;
		imapStream->mailStream->fListResultsHandle = 0;

		// go through this level and see who has child mailboxes
		for (node = thisLevel; node; node = (*node)->next)
			if (((*node)->mailboxName && *((*node)->mailboxName))	//make sure this node has a name
			    && !((*node)->attributes & LATT_NOINFERIORS)	//and this mailbox can have children
			    && !((*node)->attributes & LATT_HASNOCHILDREN))	//and the server thinks it does
			{
				char childRef[NETMAXMBX];
				short nameLen =
				    strlen((*node)->mailboxName);

				// Put the delimiter at the end of the mailbox name

				strcpy(childRef, (*node)->mailboxName);
				childRef[nameLen] = (*node)->delimiter;
				childRef[nameLen + 1] = NULL;

				LDRef(node);
				(*node)->childList =
				    GetIMAPMailboxLevel(imapStream,
							childRef, false,
							progress);
				UL(node);
			}
	}
	return (thisLevel);
}

/**********************************************************************
 *	CreateNewPersCaches - create pers caches for new personalities.
 **********************************************************************/
OSErr CreateNewPersCaches(void)
{
	OSErr err = noErr;
	IMAPStreamPtr iStream = 0;
	PersHandle oldPers = CurPers;
	Str255 cacheName;
	FSSpec spec;

	for (CurPers = PersList; CurPers && (err == noErr);
	     CurPers = (*CurPers)->next) {
		// if this is an IMAP personality ...
		if (IsIMAPPers(CurPers)) {
			// see if the personality already has a cache
			PersNameToCacheName(CurPers, cacheName);

			err =
			    FSMakeFSSpec(IMAPMailRoot.vRef,
					 IMAPMailRoot.dirId, cacheName,
					 &spec);
			if (err == fnfErr)	// it doesn't.
			{
				// create a cache folder for this personality
				if (!CreateIMAPCacheMailbox(&spec, true)) {
					// rebuild the mailbox menus to reflect the change.
					BuildBoxMenus();
					MBTickle(nil, nil);
					err = noErr;
				}
			}
		}
	}

	CurPers = oldPers;
	return (err);
}

/**********************************************************************
 *	UpdateLocalCache - make sure the local cache belonging to the 
 *		current personality jives with what's on the remote server
 **********************************************************************/
OSErr UpdateLocalCache(Boolean progress)
{
	OSErr err = noErr;
	FSSpec spec, itemSpec;
	Str63 cacheName;
	Str63 name;
	long junk;

	// pick the name for the cache
	PersNameToCacheName(CurPers, cacheName);

	// see if the personality already has a cache
	err =
	    FSMakeFSSpec(IMAPMailRoot.vRef, IMAPMailRoot.dirId, cacheName,
			 &spec);
	if (err == fnfErr)	// create it if we need to
	{
		if (!CreateIMAPCacheMailbox(&spec, true))
			err = noErr;
	} else
		spec.parID = SpecDirId(&spec);

	// see if the personality has an attachment folder.     
	if (err == noErr) {
		err =
		    FSMakeFSSpec(spec.vRefNum, spec.parID,
				 GetRString(name, IMAP_ATTACH_FOLDER),
				 &itemSpec);
		if (err == fnfErr) {
			err =
			    FSpDirCreate(&itemSpec, smSystemScript, &junk);
		}
	}
	// now go update this personality's cache mailboxes.
	if (err == noErr) {
		LDRef(CurPers);
		err =
		    UpdateLocalCacheMailboxes(&(*CurPers)->mailboxTree,
					      &spec, progress);
		UL(CurPers);
	}
	// make sure the first node in the mailboxtree points inside the cache folder itself.
	LDRef(CurPers);
	SimpleMakeFSSpec(spec.vRefNum, spec.parID, cacheName,
			 &((*(*CurPers)->mailboxTree)->mailboxSpec));
	UL(CurPers);

	// rebuild the mailbox menus to reflect the change.
	BuildBoxMenus();
	MBTickle(nil, nil);

	// Update all open TOCs so they point to valid IMAP mailbox nodes
	AttachOpenTocsToIMAPMailboxTrees();

	return (err);
}


 /**********************************************************************
 *	UpdateLocalCacheMailboxes - create the mailboxes in the cache of
 *		the current personalty
 **********************************************************************/
OSErr UpdateLocalCacheMailboxes(MailboxNodeHandle * tree, FSSpecPtr inSpec,
				Boolean progress)
{
	OSErr err = noErr;
	MailboxNodeHandle node, scan;
	FSSpec spec;
	Str63 mName;		// this is the mailbox's actual name
	Str255 pInbox, cInbox;
	Str255 progressMessage;
	static long lastProgress = 0;

	GetRString(pInbox, IMAP_INBOX_NAME);
	PtoCcpy(cInbox, pInbox);

	err = CleanCacheFolder(inSpec, *tree);

	// create the IMAP cache mailboxes                      
	node = *tree;
	while ((node != nil) && (err == noErr)) {
		if ((*node)->mailboxName) {
			// figure out the mailbox name.  The node contains the full pathname
			LDRef(node);
			PathToMailboxName((*node)->mailboxName, mName,
					  (*node)->delimiter);
			UL(node);

			// create the folder if we have to
			err =
			    FSMakeFSSpec(inSpec->vRefNum, inSpec->parID,
					 mName, &spec);

			// display some periodic progress if we're going to create a new mailbox
			if (progress && err) {
				if ((TickCount() - lastProgress) > 30) {
					ComposeRString(progressMessage,
						       IMAP_CACHE_CREATE,
						       mName);
					PROGRESS_MESSAGE(kpMessage,
							 progressMessage);
					lastProgress = TickCount();
				}
			}
			//
			// special case: if the user has specified a location prefix, we may end up with two Inboxes in the root level.
			// so, if the name of this mailbox is Inbox, the pathname is not "Inbox", and it's in the root level, then
			// create a second INBOX with a unique name.
			//

			// if a folder with that name was found ...
			if ((err == noErr) && AFSpIsItAFolder(&spec)) {
				// and the name of the mailbox is inbox ...
				if (StringSame(pInbox, mName)) {
					LDRef(node);

					// but it's not in the root directory on the server ...
					if (striscmp
					    (cInbox,
					     (*node)->mailboxName)) {
						// and it's in the top level of mailboxes locally ...
						scan =
						    (*CurPers)->
						    mailboxTree;
						while (scan) {
							if (scan == node)
								break;
							else
								scan =
								    (*scan)->
								    next;
						}

						// and there IS another (real) inbox ...
						if (scan
						    &&
						    LocateInboxForPers
						    (CurPers)) {
							// ... then give this second inbox a unique name
							UniqueSpec(&spec,
								   MAX_BOX_NAME);
							PCopy(mName,
							      spec.name);
							err = fnfErr;	// and create it.
						}
					}

					UL(node);
				}
			}
			// we found something, but it's not a folder
			if ((err == noErr) && (!AFSpIsItAFolder(&spec))) {
				// then try again with a unique name
				if ((err =
				     UniqueSpec(&spec,
						MAX_BOX_NAME)) == noErr) {
					PCopy(mName, spec.name);
					err =
					    FSMakeFSSpec(inSpec->vRefNum,
							 inSpec->parID,
							 mName, &spec);
				}
			}

			if (err != noErr) {
				// create a folder for this mailbox
				if (err == fnfErr) {
					if (!CreateIMAPCacheMailbox
					    (&spec, true))
						err = noErr;
				}
				//else
				//stop processing this level.
			} else {
				// Look inside the folder since it already exists
				if ((spec.parID = SpecDirId(&spec)) != 0) {
					// Remove from this folder all things that aren't on the server
					LDRef(node);
					err =
					    CleanCacheFolder(&spec,
							     (*node)->
							     childList);
					UL(node);
				}
			}

			// create the mailbox file.  It has the same name as the folder
			if ((err == noErr)) {
				//
				//      Note - here we could look at ((*node)->attributes & LATT_NOSELECT) and
				//      create a mailbox file only for SELECTable mailboxes.  I choose to create
				//      a mailbox file and not use it to simplify other things.
				//

				LDRef(node);
				err =
				    FSMakeFSSpec(spec.vRefNum, spec.parID,
						 mName,
						 &((*node)->mailboxSpec));
				if (err == fnfErr)
					if (!CreateIMAPCacheMailbox
					    (&((*node)->mailboxSpec),
					     false))
						err = noErr;
				UL(node);

				//update the mailbox imap information
				if (err == noErr) {
					LDRef(node);
					err =
					    WriteIMAPMailboxInfo(&
								 ((*node)->
								  mailboxSpec),
								 node);
					UL(node);
				}
			}
			// now do this mailbox's children
			if ((err == noErr) && ((*node)->childList)) {
				LDRef(node);
				err =
				    UpdateLocalCacheMailboxes(&
							      ((*node)->
							       childList),
							      &spec,
							      progress);
				UL(node);
			}
		}
		node = (*node)->next;
	}

	return (err);
}


/**********************************************************************
 * CreateIMAPCacheMailbox - create a mailbox or folder
 **********************************************************************/
Boolean CreateIMAPCacheMailbox(FSSpecPtr spec, Boolean folder)
{
	int err;
	char *cp;
	long newDirId;

	if (*spec->name > MAX_BOX_NAME) {
		TooLong(spec->name);
		return (True);
	}
	// change all ':'s to something nicer
	for (cp = spec->name + *spec->name; cp > spec->name; cp--)
		if (*cp == ':')
			*cp = '-';

	if (folder) {
		if (BoxMapCount > MAX_BOX_LEVELS) {
			WarnUser(TOO_MANY_LEVELS, MAX_BOX_LEVELS);
			return (True);
		}

		if (err = FSpDirCreate(spec, smSystemScript, &newDirId)) {
			FileSystemError(CREATING_MAILBOX, spec->name, err);
			return (True);
		}
		spec->parID = newDirId;
	} else			// create a mailbox
	{
		err =
		    FSpCreate(spec, CREATOR, IMAP_MAILBOX_TYPE,
			      smSystemScript);
		if (err) {
			FileSystemError(CREATING_MAILBOX, spec->name, err);
			return (True);
		}
	}
	return (False);
}

/**********************************************************************
 * CleanIMAPFolder - loop through personality caches, and remove unused
 *	cache folders.
 **********************************************************************/
OSErr CleanIMAPFolder(void)
{
	OSErr err = noErr;
	CInfoPBRec hfi;
	Str63 name;
	Str63 cacheName;
	Boolean current = false;
	FSSpec toDelete;
	Str63 IMAPAttachFolderName;

	// only wipe outdated IMAP caches if we have multiple personalities ...
	if (!HasFeature(featureMultiplePersonalities))
		return (noErr);

	GetRString(IMAPAttachFolderName, IMAP_ATTACH_FOLDER);

	PushPers(nil);

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(IMAPMailRoot.vRef, IMAPMailRoot.dirId, &hfi)) {
		err = noErr;	//clear the error condition
		current = false;
		// compare the name of the file we just found to the list of personalities.
		for (CurPers = PersList; CurPers && (err == noErr);
		     CurPers = (*CurPers)->next) {
			PersNameToCacheName(CurPers, cacheName);

			// if this item has the same name as a personality or the attach folder, save it.
			if (StringSame(cacheName, name)) {
				current = true;
				break;
			}
		}
		if (!current) {
			if ((err =
			     FSMakeFSSpec(IMAPMailRoot.vRef,
					  IMAPMailRoot.dirId, name,
					  &toDelete)) == noErr) {
				if ((err =
				     RemoveIMAPCacheDir(toDelete)) ==
				    noErr)
					hfi.hFileInfo.ioFDirIndex--;
			}
		}
	}

	PopPers();

	return (err);
}


/**********************************************************************
 * CleanCacheFolder - loop through a folder, trash what doesn't belong.
 *	Do this for the current personality only.
 **********************************************************************/
OSErr CleanCacheFolder(FSSpecPtr folderToClean,
		       MailboxNodeHandle treeToClean)
{
	OSErr err = noErr;
	CInfoPBRec hfi;
	Str63 name;
	Str63 cacheName;
	Boolean current = false;
	FSSpec toDelete;
	MailboxNodeHandle node;

	//make sure we have a folder to clean AND a tree to clean.
#ifdef	DEBUG
	ASSERT(!(folderToClean->parID == Root.dirId));
#endif

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate
	       (folderToClean->vRefNum, folderToClean->parID, &hfi)) {
		current = false;
		// compare the name of the file we just found to the mailboxes in the tree to clean
		for (node = treeToClean; node && (err == noErr);
		     node = (*node)->next) {
			if (node) {
				LDRef(node);
				PathToMailboxName((*node)->mailboxName,
						  cacheName,
						  (*node)->delimiter);
				UL(node);

				if (StringSame(cacheName, name)
				    || IsSpecialIMAPName(name, NULL)) {
					current = true;
					break;
				}
			}
		}
		if (!current && (hfi.hFileInfo.ioFlAttrib & ioDirMask))	// this folder wasn't found to belong
		{
			if ((err =
			     FSMakeFSSpec(folderToClean->vRefNum,
					  folderToClean->parID, name,
					  &toDelete)) == noErr) {
				if ((err =
				     RemoveIMAPCacheDir(toDelete)) ==
				    noErr) {
					hfi.hFileInfo.ioFDirIndex--;
				}
			}
		}
		// leave stray items alone.  User gets what he deserves by screwing around in here.
	}
	return (err);
}


/**********************************************************************
 * RemoveIMAPCacheDir - iterate through an IMAP cache folder, trashing
 *	everything.
 **********************************************************************/
OSErr RemoveIMAPCacheDir(FSSpec toDelete)
{
	OSErr err = noErr;
	CInfoPBRec hfi;
	Str63 name;
	FSSpec child;
	long targetDir = SpecDirId(&toDelete);

	if (targetDir) {
		hfi.hFileInfo.ioNamePtr = name;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate(toDelete.vRefNum, targetDir, &hfi)
		       && err == noErr) {
			// Process subdirectories
			if (hfi.hFileInfo.ioFlAttrib & ioDirMask) {
				SimpleMakeFSSpec(toDelete.vRefNum,
						 targetDir, name, &child);
				err = RemoveIMAPCacheDir(child);
				if (!err)
					hfi.hFileInfo.ioFDirIndex--;	//      Don't skip next entry
			}
		}

		if (err == noErr)	// delete the folder itself
		{
			err = RemoveDir(&toDelete);
			if (err)
				err = FSpTrash(&toDelete);
		}
	}

	return (err);
}


/**********************************************************************
 * PathToMailboxName - return the mailbox name, given a pathname
 **********************************************************************/
void PathToMailboxName(CStr path, Str63 mboxName, char delimiter)
{
	char *scan;
	Str255 pInbox;

	// initialize mboxName
	WriteZero(mboxName, sizeof(Str63));

	// must have been given a path to convert
	if (!path || (*path == nil))
		return;

	if (delimiter) {
		scan = path + strlen(path) - 1;
		if (*scan == delimiter)
			scan--;	// if the name ENDS with a delimiter, ignore it.
		while ((scan > path) && (*scan != delimiter))
			scan--;
		if (*scan == delimiter)
			scan++;	// reached a delimiter
	} else
		scan = path;

	// convert the good bit to a PString.
	mboxName[0] = MIN(strlen(scan), MAX_BOX_NAME);
	BlockMoveData(scan, &mboxName[1], mboxName[0]);

	// make sure the PString name doesn't end with the delimiter
	if (mboxName[mboxName[0]] == delimiter)
		mboxName[0] -= 1;

	// clean up the name.  Turn all :'s into something else:
	for (scan = mboxName + mboxName[0]; scan > mboxName; scan--)
		if (*scan == ':')
			*scan = '-';

	// Special case - use prettier INBOX name
	GetRString(pInbox, IMAP_INBOX_NAME);
	if (StringSame(pInbox, scan))
		PCopy(mboxName, pInbox);
}

/***************************************************************************
 * IsSpecialIMAPName - is this the name of a special file, IMAPly speaking?
 ***************************************************************************/
Boolean IsSpecialIMAPName(unsigned char *name, Boolean * bIsDir)
{
	Boolean isSpecial = false;

	// is this the IMAP Attachments folder?
	isSpecial = EqualStrRes(name, IMAP_ATTACH_FOLDER);
	if (bIsDir)
		*bIsDir = isSpecial;


	// is this the cache file for hidden messages?
	if (!isSpecial)
		isSpecial = EqualStrRes(name, IMAP_HIDDEN_TOC_NAME);

	return (isSpecial);
}

/************************************************************************
 * IsIMAPSubPers - does this spec point to an IMAP sub personality?
 ************************************************************************/
Boolean IsIMAPSubPers(FSSpecPtr spec)
{
	Boolean result = false;
	Str63 compareName;

	// is this spec in the root level of the IMAP cache folder?
	if (spec->parID == IMAPMailRoot.dirId
	    && SameVRef(spec->vRefNum, IMAPMailRoot.vRef)) {
		// does it have a name other than the name of the dominant personality?

		GetRString(compareName, DOMINANT);
		result = !StringSame(spec->name, compareName);
	}

	return (result);
}

/***************************************************************************
 * PersNameToCacheName - given a personality, figure out the name of its
 *	imap cache folder.
 ***************************************************************************/
void PersNameToCacheName(PersHandle pers, UPtr cacheName)
{
	unsigned char *spot;
	SignedByte state = HGetState((Handle) pers);

	LDRef(pers);
	PCopy(cacheName, (*pers)->name);
	for (spot = cacheName + 1; spot <= cacheName + cacheName[0];
	     spot++)
		if (*spot == ':')
			*spot = '-';
	HSetState((Handle) pers, state);
}

/***************************************************************************
 * WriteIMAPMailboxInfo - write the IMAP mailbox info from node to the mbox
 ***************************************************************************/
OSErr WriteIMAPMailboxInfo(FSSpecPtr spec, MailboxNodeHandle node)
{
	OSErr err = noErr;
	short refN = -1;
	long count = 0;
	short oldResFile = CurResFile();
	Handle resource = 0;

	// create a resource fork if we have to
	FSpCreateResFile(spec, CREATOR, IMAP_MAILBOX_TYPE, smSystemScript);

	// open the mailbox file for writing
	if ((refN = FSpOpenResFile(spec, fsRdWrPerm)) != -1) {
		LockMailboxNodeHandle(node);
		UseResFile(refN);

		// Zap the old info resource
		SetResLoad(False);
		resource = Get1Resource(BOX_INFO_TYPE, IMAP_ID);
		SetResLoad(True);
		if (resource) {
			RemoveResource(resource);
			ZapHandle(resource);
		}
		// Zap the old name resource
		SetResLoad(False);
		resource = Get1Resource(BOX_NAME_TYPE, IMAP_ID);
		SetResLoad(True);
		if (resource) {
			RemoveResource(resource);
			ZapHandle(resource);
		}
		// add the info to the resource
		resource = DupHandle((Handle) node);
		if (resource) {
			AddResource(resource, BOX_INFO_TYPE, IMAP_ID,
				    (*node)->mailboxSpec.name);
			if ((err = ResError()) == noErr) {
				WriteResource(resource);
				err = ResError();
			}
		}
		// Zap the old flags resource
		SetResLoad(False);
		resource = Get1Resource(BOX_FLAGS_TYPE, IMAP_ID);
		SetResLoad(True);
		if (resource) {
			RemoveResource(resource);
			ZapHandle(resource);
		}
		// add the flag info to the resource
		if ((*node)->queuedFlags
		    && GetHandleSize((*node)->queuedFlags)) {

			resource =
			    DupHandle((Handle) ((*node)->queuedFlags));
			if (resource) {
				AddResource(resource, BOX_FLAGS_TYPE,
					    IMAP_ID,
					    (*node)->mailboxSpec.name);
				if ((err = ResError()) == noErr) {
					WriteResource(resource);
					err = ResError();
				}
			}
		}
		//Add the name to the resource as well
		resource = NuHandle(0);
		if (resource && (err = MemError()) == noErr) {
			err =
			    PtrPlusHand((*node)->mailboxName, resource,
					strlen((*node)->mailboxName) + 1);
			if (err == noErr) {
				AddResource(resource, BOX_NAME_TYPE,
					    IMAP_ID,
					    (*node)->mailboxSpec.name);
				if ((err = ResError()) == noErr) {
					WriteResource(resource);
					err = ResError();
				}

			}
		}
		CloseResFile(refN);
		UnlockMailboxNodeHandle(node);
	} else
		err = ResError();

	UseResFile(oldResFile);
	return (err);
}

/***************************************************************************
 * UpdateIMAPMailboxInfo - update the IMAP mailbox info in a cache file
 ***************************************************************************/
OSErr UpdateIMAPMailboxInfo(TOCHandle tocH)
{
	OSErr err = noErr;
	MailboxNodeHandle node = nil;
	FSSpec cacheSpec;

	// must be an IMAP mailbox
	if (!(*tocH)->imapTOC)
		return (noErr);

	cacheSpec = ((*tocH)->mailbox.spec);

	// find the mailbox node
	node = TOCToMbox(tocH);
	if (node) {
		// save the info
		err = WriteIMAPMailboxInfo(&cacheSpec, node);
	}

	return (err);
}


/***************************************************************************
 * RebuildAllIMAPMailboxTrees - go through the personalities and rebuild
 *	the imap mailbox trees
 ***************************************************************************/
void RebuildAllIMAPMailboxTrees(void)
{
	OSErr err = noErr;
	PersHandle oldPers = CurPers;

	for (CurPers = PersList; CurPers && (err == noErr);
	     CurPers = (*CurPers)->next) {
		// build the IMAP tree from the files in the IMAP Folder.
		(*CurPers)->mailboxTree =
		    RebuildPersIMAPMailboxTree(CurPers);
	}
	CurPers = oldPers;

	// Update all open TOCs so they point to valid IMAP mailbox nodes
	AttachOpenTocsToIMAPMailboxTrees();
}

/***************************************************************************
 * RebuildPersIMAPMailboxTree - given a personality, rebuild it's mailbox
 *	tree by reading through it's IMAP cache folder
 ***************************************************************************/
MailboxNodeHandle RebuildPersIMAPMailboxTree(PersHandle pers)
{
	MailboxNodeHandle tree = NewZH(MailboxNode);
	OSErr err = noErr;
	Str63 cacheName;
	Boolean bHasQueuedCommands = false;

	// make sure we got our first node.
	if (tree == nil)
		WarnUser(MEM_ERR, MemError());
	else {
		(*tree)->attributes = LATT_ROOT;
		(*tree)->persId = (*pers)->persId;
	}

	// Kill the cache in LocateNodeBySpecInAllPersTrees
	(void) LocateNodeBySpecInAllPersTrees(nil, nil, nil);

	PersNameToCacheName(pers, cacheName);

	// locate this personality's IMAP cache directory
	LDRef(tree);
	err =
	    FSMakeFSSpec(IMAPMailRoot.vRef, IMAPMailRoot.dirId, cacheName,
			 &(*tree)->mailboxSpec);
	if (err == noErr) {
		// make sure the first node in the mailboxtree points inside the cache folder itself.
		(*tree)->mailboxSpec.parID =
		    SpecDirId(&((*tree)->mailboxSpec));

		// rebuild the tree by reading through the cache.
		err =
		    RebuildIMAPMailboxTree(&((*tree)->mailboxSpec), pers,
					   &tree, &bHasQueuedCommands);
	}
	UL(tree);

	// mark this entire tree if one of its leaves has queued commands waiting
	if (bHasQueuedCommands)
		SetIMAPMailboxNeeds(tree, kNeedsExecCmd, true);

	return (err == noErr ? tree : NIL);
}

/***************************************************************************
 * RebuildIMAPMailboxTree - given a directory, iterate through it and
 *	build a IMAPMailbox tree.  Give it our best shot, this shouldn't fail
 ***************************************************************************/
OSErr RebuildIMAPMailboxTree(FSSpecPtr dir, PersHandle pers,
			     MailboxNodeHandle * tree,
			     Boolean * bHasQueuedCommands)
{
	OSErr err = noErr;
	CInfoPBRec hfi;
	Str63 name, attachDirName;
	FSSpec mboxFile, mboxFolder;
	MailboxNodeHandle node;

	GetRString(attachDirName, IMAP_ATTACH_FOLDER);

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(dir->vRefNum, dir->parID, &hfi)) {
		if (hfi.hFileInfo.ioFlAttrib & ioDirMask)	// this is a directory
		{
			if (!StringSame(name, attachDirName))	// and it's not the attachments directory
			{
				// it must be an imap mailbox.  Create an imap node from the file within this folder
				SimpleMakeFSSpec(dir->vRefNum, dir->parID,
						 name, &mboxFolder);
				FSMakeFSSpec(dir->vRefNum,
					     SpecDirId(&mboxFolder), name,
					     &mboxFile);
				if (err == noErr) {
					// read in the mailbox info from the mailbox file
					node = NewZH(MailboxNode);
					if (node) {
						err =
						    ReadIMAPMailboxInfo
						    (&mboxFile, node);
						if (err == noErr) {
							// attach the right personality to it
							(*node)->persId =
							    (*pers)->
							    persId;

							// add this node to the tree
							LL_Queue(*tree,
								 node,
								 (MailboxNodeHandle));

							// mark the tree if there are queued commands in this mailbox
							if (DoesIMAPMailboxNeed(node, kNeedsExecCmd))
								*bHasQueuedCommands
								    = true;

							// and process any subfolders of this mailbox
							LDRef(node);
							RebuildIMAPMailboxTree
							    (&mboxFile,
							     pers,
							     &((*node)->
							       childList),
							     bHasQueuedCommands);
							UL(node);
						}
					} else
						WarnUser(MEM_ERR,
							 MemError());
				}
				// else
				// something wrong with this folder.  Skip it.
			}
		}
		// leave stray items alone.  User gets what he deserves by screwing around in here.
	}
	return (err);
}

/***************************************************************************
 * ReadIMAPMailboxInfo - read the IMAP mailbox info from spec into the node
 ***************************************************************************/
OSErr ReadIMAPMailboxInfo(FSSpecPtr spec, MailboxNodeHandle node)
{
	OSErr err = noErr;
	short refN = -1;
	short oldResFile = CurResFile();
	Handle resource = 0;
	char *mailboxName = 0;

	Zero(**node);

	// open the mailbox file for reading
	if ((refN = FSpOpenResFile(spec, fsRdPerm)) != -1) {
		// read the full pathname from the mailbox.  Store it in some string somewhere.
		UseResFile(refN);
		if ((resource = Get1Resource(BOX_NAME_TYPE, IMAP_ID)) == 0)
			err = resNotFound;
		if ((err == noErr) && ((err = ResError()) == noErr)
		    && resource != 0) {
			LDRef(resource);
			mailboxName = cpystr(*resource);	// copy the pathname
			UL(resource);

			// read in the node from the mailbox.
			if ((resource =
			     Get1Resource(BOX_INFO_TYPE, IMAP_ID)) == 0)
				err = resNotFound;
			if ((err == noErr) && ((err = ResError()) == noErr)
			    && resource != 0) {
				BlockMove(*resource, *node,
					  MIN(GetHandleSize(resource),
					      sizeof(MailboxNode)));

				// initialize garbage fields
				(*node)->next = 0;
				(*node)->childList = 0;
				(*node)->queuedFlags = 0;
				(*node)->flagLock = false;
				(*node)->mailboxName = mailboxName;	//the node will point to the mailboxname.
				(*node)->mailboxSpec = *spec;
				(*node)->tocRef = 0;

				// read in the queued flag info from the mailbox.  Don't care if it's not there.
				resource =
				    Get1Resource(BOX_FLAGS_TYPE, IMAP_ID);
				if (((ResError()) == noErr)
				    && resource != 0) {
					DetachResource(resource);
					(*node)->queuedFlags = resource;
				}
			}
		}
		CloseResFile(refN);
	} else
		err = ResError();

	UseResFile(oldResFile);
	return (err);
}

/**********************************************************************
 *	CreateLocalCache - Build the local cache of IMAP mailboxes for the
 *		IMAP server pointed to by the current personality.
 **********************************************************************/
OSErr CreateLocalCache(void)
{
	OSErr err = noErr;
	IMAPStreamPtr iStream = 0;
	Boolean progress = true;

	// display some progress
	if (progress) {
		PROGRESS_START;
		PROGRESS_MESSAGER(kpTitle, IMAP_CACHE_MESSAGE);
	}
	// don't do anything if there's a background thread running.  It may be using the mailbox tree.
	if (((*CurPers)->mailboxTree != nil) && !CanModifyMailboxTrees())
		return (IMAPTreeInUse);

	// Nothing to do if this isn't an IMAP personality
	if (PrefIsSet(PREF_IS_IMAP)) {
		// create a new imap stream.
		if (iStream = GetIMAPConnection(UndefinedTask, false)) {
			if (progress)
				PROGRESS_MESSAGER(kpSubTitle,
						  IMAP_MAILBOX_LIST_FETCH_GENERAL);

			// get the mailbox tree from the server
			if (GetIMAPMailboxes(iStream, progress)) {
				// tell the user we're about to create some local mailboxes
				if (progress)
					PROGRESS_MESSAGER(kpSubTitle,
							  IMAP_CACHE_CREATE_GENERAL);

				// (GetIMAPMailboxes will create the mailStream bit of our stream)
				if (iStream->mailStream->
				    fListResultsHandle != nil) {
					LDRef(CurPers);
					// get the uidValidity's out of the old tree, and put them in the new tree.
					TransferLocalTreeInfo((*CurPers)->
							      mailboxTree,
							      iStream->
							      mailStream->
							      fListResultsHandle);

					// kill the old tree if there is one
					if ((*CurPers)->mailboxTree)
						DisposeMailboxTree(&
								   ((*CurPers)->mailboxTree));
					UL(CurPers);

					// and remember the new one
					(*CurPers)->mailboxTree =
					    iStream->mailStream->
					    fListResultsHandle;
					iStream->mailStream->
					    fListResultsHandle = nil;

					// then update the local cache
					if ((*CurPers)->mailboxTree)
						err =
						    UpdateLocalCache
						    (progress);
				}
			} else
				err =
				    IMAPError(kIMAPList, kIMAPListErr,
					      errIMAPListErr);

			CleanupConnection(&iStream);
		}
	}

	if (progress)
		PROGRESS_END;

	return (err);
}

/**********************************************************************
 * StrCmpNotIncludingEndingDelimiter - compare two strings.
 *	return true if they're the same, not taking the final character
 *	into account if it's the delimiter.
 **********************************************************************/
Boolean StrCmpNotIncludingEndingDelimiter(UPtr str1, UPtr str2,
					  char delimiter)
{
	short len1 = strlen(str1);
	short len2 = strlen(str2);

	// don't bother with the compare if one or the other strings is nil.
	if ((len1 == 0) || (len2 == 0))
		return true;

	if (str1[len1 - 1] == delimiter)
		len1--;
	if (str2[len2 - 1] == delimiter)
		len2--;

	if (len1 != len2)
		return (true);

	return (strncmp(str1, str2, MIN(len1, len2)));
}

/**********************************************************************
 * DisposePersIMAPMailboxTrees - loop through the personalities, and
 *	dispose each IMAP mailbox tree.
 **********************************************************************/
void DisposePersIMAPMailboxTrees(void)
{
	PersHandle pers;

	for (pers = PersList; pers; pers = (*pers)->next) {
		LDRef(pers);
		DisposeMailboxTree(&((*pers)->mailboxTree));
		UL(pers);
	}
}

/**********************************************************************
 *	DisposeMailboxTree - recurse through a tree of mailboxes,
 *		and dispose of all the leaves.
 **********************************************************************/
void DisposeMailboxTree(MailboxNodeHandle * tree)
{
	MailboxNodeHandle node;

	// Kill the cache in LocateNodeBySpecInAllPersTrees
	(void) LocateNodeBySpecInAllPersTrees(nil, nil, nil);

	while (node = *tree) {
		LL_Remove(*tree, node, (MailboxNodeHandle));
		LDRef(node);
		if ((*node)->childList)
			DisposeMailboxTree(&((*node)->childList));
		if ((*node)->queuedFlags)
			ZapHandle((*node)->queuedFlags);
		UL(node);
		ZapMailboxNode(&node);
	}
	*tree = nil;
}

/**********************************************************************
 *	ZapMailboxNode - dispose a mailbox node
 **********************************************************************/
void ZapMailboxNode(MailboxNodeHandle * node)
{
	if ((**node)->mailboxName) {
		DisposePtr((**node)->mailboxName);
		(**node)->mailboxName = 0;
	}
	ZapHandle(*node);
}

/***************************************************************************
 * LocateNodeBySpecInAllPersTrees - given an FSSpec, locate the node in
 *	all the personality trees with the same fsspec in them.
 ***************************************************************************/
void LocateNodeBySpecInAllPersTrees(FSSpecPtr spec,
				    MailboxNodeHandle * node,
				    PersHandle * pers)
{
	MailboxNodeHandle tree = nil;
	SignedByte state;
	static MailboxNodeHandle oldNode;
	static FSSpec oldSpec;
	static PersHandle oldPers;

	// cache flush?
	if (!spec) {
		Zero(oldSpec);
		oldNode = nil;
		oldPers = nil;
		return;
	}

	*node = nil;
	*pers = nil;

	// Is it in the cache?
	if (!InAThread() && SameSpec(spec, &oldSpec)) {
		*node = oldNode;
		*pers = oldPers;
		return;
	}

	for (*pers = PersList; *pers; *pers = (**pers)->next) {
		state = HGetState((Handle) (*pers));
		LDRef(*pers);
		if ((**pers)->mailboxTree) {
			tree = (**pers)->mailboxTree;
			if (tree) {
				if (*node =
				    LocateNodeBySpec((**pers)->mailboxTree,
						     spec)) {
					HSetState((Handle) (*pers), state);
					break;
				}
			}
		}
		HSetState((Handle) (*pers), state);
	}

	if (!InAThread()) {
		// cache values
		oldSpec = *spec;
		oldNode = *node;
		oldPers = *pers;
	}
}

/***************************************************************************
 * LocateNodeBySpec - given an FSSpec, locate the node in the tree
 ***************************************************************************/
MailboxNodeHandle LocateNodeBySpec(MailboxNodeHandle tree, FSSpecPtr spec)
{
	MailboxNodeHandle node = nil;

	while (tree) {
		// is this the node we're looking for?
		LockMailboxNodeHandle(tree);
		if (SameSpec(&((*tree)->mailboxSpec), spec)) {
			node = tree;
			UnlockMailboxNodeHandle(tree);
			break;
		}
		// is the node we're looking for one of the children of this node?
		if ((*tree)->childList)
			node =
			    LocateNodeBySpec(((*tree)->childList), spec);
		UnlockMailboxNodeHandle(tree);

		if (node)
			break;

		// otherwise, check the next node.
		tree = (*tree)->next;
	}
	return (node);
}

/***************************************************************************
 * LocateNodeByVDInAllPersTrees - given an vRef and dirID, return the
 *	personality of the mailboxTree the values point to, and the parent
 *	node of where this mbox would end up.
 ***************************************************************************/
void LocateNodeByVDInAllPersTrees(short vRef, long dirId,
				  MailboxNodeHandle * node,
				  PersHandle * pers)
{
	SignedByte state;

	*node = nil;
	*pers = nil;

	for (*pers = PersList; *pers; *pers = (**pers)->next) {
		state = HGetState((Handle) * pers);
		LDRef(*pers);
		if (*node =
		    LocateNodeByVD((**pers)->mailboxTree, vRef, dirId))
			break;
		HSetState((Handle) (*pers), state);
	}
}


/***************************************************************************
 * LocateNodeByVD - given an dirID and a vRef, see if this file is
 *  inside an IMAP cache folder.
 ***************************************************************************/
MailboxNodeHandle LocateNodeByVD(MailboxNodeHandle tree, short vRef,
				 long dirId)
{
	MailboxNodeHandle scan = tree;
	MailboxNodeHandle node = nil;

	while (scan) {
		// is this the node we're looking for?
		if (((*scan)->mailboxSpec.vRefNum == vRef)
		    && ((*scan)->mailboxSpec.parID == dirId)) {
			node = scan;
			break;
		}
		// is the node we're looking for one of the children of this node?
		LockMailboxNodeHandle(scan);
		if ((*scan)->childList)
			node =
			    LocateNodeByVD(((*scan)->childList), vRef,
					   dirId);
		UnlockMailboxNodeHandle(scan);
		if (node) {
			// we found a node with the same vRef and dirId.  Stop the search.
			break;
		}
		// otherwise, check the next node.
		scan = (*scan)->next;
	}
	return (node);
}

/***************************************************************************
 * LocateNodeByMailboxName - given a name, locate the node in the tree
 ***************************************************************************/
MailboxNodeHandle LocateNodeByMailboxName(MailboxNodeHandle tree,
					  char *mailboxName)
{
	MailboxNodeHandle scan = tree;
	MailboxNodeHandle node = nil;

	// must have a mailbox name ...
	if (!mailboxName)
		return (nil);

	while (scan) {
		// makes no sense to check the root node
		if ((*scan)->mailboxName) {
			// is this the node we're looking for?
			if (!strcmp(mailboxName, (*scan)->mailboxName)) {
				node = scan;
				break;
			}
			// is the node we're looking for one of the children of this node?
			LockMailboxNodeHandle(scan);
			if ((*scan)->childList)
				node =
				    LocateNodeByMailboxName(((*scan)->
							     childList),
							    mailboxName);
			UnlockMailboxNodeHandle(scan);

			if (node)
				break;
		}
		// otherwise, check the next node.
		scan = (*scan)->next;
	}
	return (node);
}

MailboxNodeHandle LocateParentNodeInChildren(MailboxNodeHandle parent,
					     MailboxNodeHandle child)
{
	MailboxNodeHandle scan = (*parent)->childList;
	MailboxNodeHandle p = nil;

	while (scan && !p) {
		if (scan == child)
			p = parent;
		else
			p = LocateParentNodeInChildren(scan, child);

		scan = (*scan)->next;
	}

	return (p);
}

/***************************************************************************
 * LocateParentNode - given a mailbox node, figure out it's parent
 ***************************************************************************/
MailboxNodeHandle LocateParentNode(MailboxNodeHandle tree,
				   MailboxNodeHandle child)
{
	MailboxNodeHandle scan = tree;
	MailboxNodeHandle parent = nil;

	// first see if the child is a top level mailbox.
	if ((*scan)->attributes & LATT_ROOT) {
		while (scan) {
			if (scan == child) {
				parent = tree;
				break;
			}
			scan = (*scan)->next;
		}
	}
	// otherwise, look through all the nodes' children.
	scan = tree;
	while (scan && !parent) {
		parent = LocateParentNodeInChildren(scan, child);
		scan = (*scan)->next;
	}

	return (parent);
}

/***************************************************************************
 * ReadIMAPMailboxAttributes - get the IMAP mailbox attributes
 ***************************************************************************/
OSErr ReadIMAPMailboxAttributes(FSSpecPtr spec, long *attributes)
{
	OSErr err;
	MailboxNodeHandle node;
	PersHandle pers;

	// initialize
	err = fnfErr;
	*attributes = 0;

	LocateNodeBySpecInAllPersTrees(spec, &node, &pers);
	if (node != NULL) {
		*attributes = (*node)->attributes;
		err = noErr;
	}
	return (err);
}

/***************************************************************************
 * ReallyIsIMAPMailbox - get the IMAP mailbox attributes, see if it's IMAP
 *	Reads from disk.
 ***************************************************************************/
Boolean ReallyIsIMAPMailbox(FSSpecPtr spec)
{
	OSErr err = noErr;
	short refN = -1;
	short oldResFile = CurResFile();
	short numResources = 0;

	// open the mailbox file for reading
	if ((refN = FSpOpenResFile(spec, fsRdPerm)) != -1) {
		// read in the node from the mailbox.
		numResources = Count1Resources(BOX_INFO_TYPE);
		err = ResError();
		CloseResFile(refN);
	} else
		err = ResError();

	UseResFile(oldResFile);
	return ((err == noErr) && (numResources > 0));
}

/***************************************************************************
 * MailboxAttributes - fill in interesting mailbox attributes.  Return
 *	false if this wasn't an IMAP mailbox at all. 
 *	Reads from the IMAP mailbox tree.
 ***************************************************************************/
Boolean MailboxAttributes(FSSpecPtr spec,
			  IMAPMailboxAttributesPtr attributes)
{
	Boolean result = false;
	MailboxNodeHandle node = nil;
	PersHandle pers = nil;

	LocateNodeBySpecInAllPersTrees(spec, &node, &pers);

	if (node && !((*node)->attributes & LATT_ROOT)) {
		result = true;	// this was an IMAP mailbox
		attributes->noSelect =
		    ((*node)->attributes & LATT_NOSELECT);
		attributes->noInferiors =
		    ((*node)->attributes & LATT_NOINFERIORS);
		attributes->marked = ((*node)->attributes & LATT_MARKED);
		attributes->unmarked =
		    ((*node)->attributes & LATT_UNMARKED);
		attributes->hasChildren = (((*node)->childList) != nil);
		attributes->hasNoChildren =
		    ((*node)->attributes & LATT_HASNOCHILDREN);
		attributes->messageCount = (*node)->messageCount;
		attributes->latt = (*node)->attributes;
	}

	return (result);
}

/***************************************************************************
 * IsIMAPMailboxFile - return true if the spec points to an IMAP cache
 *	mailbox file.  That is, an actual local mailbox file.
 *	Reads from IMAP mailbox tree.
 ***************************************************************************/
Boolean IsIMAPMailboxFile(FSSpecPtr spec)
{
	MailboxNodeHandle node = nil;
	return (IsIMAPMailboxFileLo(spec, &node));
}

/***************************************************************************
 * IsIMAPMailboxFileLo - is this file an IMAP cache mailbox?
 ***************************************************************************/
Boolean IsIMAPMailboxFileLo(FSSpecPtr spec, MailboxNodeHandle * node)
{
	PersHandle pers = nil;

	LocateNodeBySpecInAllPersTrees(spec, node, &pers);

	if (*node == nil) {
		FSSpec s = *spec;

		// this might be a hidden toc file.  Find the mailbox file it belongs to
		*node = GetRealIMAPSpec(*spec, &s);
	}

	return ((*node != nil) && !((**node)->attributes & LATT_ROOT));
}

/***************************************************************************
 * IsIMAPCacheFolder - return true if the spec points to an IMAP cache folder.
 *	Reads from IMAP mailbox tree.
 ***************************************************************************/
Boolean IsIMAPCacheFolder(FSSpecPtr spec)
{
	FSSpec internalMailboxFileSpec;
	MailboxNodeHandle node = nil;
	PersHandle pers = nil;
	CInfoPBRec cinfo;

	// file must exist and be a directory
	if (HGetCatInfo(spec->vRefNum, spec->parID, spec->name, &cinfo)
	    || !HFIIsFolder(&cinfo))
		return false;

	SimpleMakeFSSpec(spec->vRefNum, cinfo.hFileInfo.ioDirID,
			 spec->name, &internalMailboxFileSpec);
	LocateNodeBySpecInAllPersTrees(&internalMailboxFileSpec, &node,
				       &pers);

	return (node != nil);
}

/***************************************************************************
 * IsIMAPVD - return true if the vRef and dirID indicate an IMAP cache
 *	Reads from IMAP mailbox tree
 ***************************************************************************/
Boolean IsIMAPVD(short vRef, long dirId)
{
	MailboxNodeHandle node = nil;
	PersHandle pers = nil;

	LocateNodeByVDInAllPersTrees(vRef, dirId, &node, &pers);

	return (node != nil);
}

/***************************************************************************
 * IMAPAddMailbox - Determine where a new mailbox is going to end up.  If it
 *	is going to be created inside an IMAP cache folder, try to create an 
 *	IMAP mailbox.
 *
 *	Return true if we tried to create an IMAP mailbox.
 ***************************************************************************/
Boolean IMAPAddMailbox(FSSpecPtr spec, Boolean folder, Boolean * success,
		       Boolean silent)
{
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
	MailboxNodeHandle newNode = nil;
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	char *newMailboxName = 0;
	OSErr err = noErr;
	Boolean createdIMAPMailbox = false;
	Boolean result = false;
	Str255 scratch;

	if (success)
		*success = false;

	LocateNodeByVDInAllPersTrees(spec->vRefNum, spec->parID, &node,
				     &pers);

	if ((pers != nil) && (node != nil)) {
		createdIMAPMailbox = true;

		// must be online to do this
		if (Offline && GoOnline())
			return (createdIMAPMailbox);

		// must be able to modify the mailbox tree, too.
		// sit an spin until the mailbox tree can be modified ...
		while (!CanModifyMailboxTrees() && !CommandPeriod)
			CycleBalls();

		if (CommandPeriod)
			return (createdIMAPMailbox);

		// Set up to connect to the server
		CurPers = pers;
		if (imapStream =
		    GetIMAPConnection(UndefinedTask, CAN_PROGRESS)) {
			// we're going to need a delimiter if we're creating a folder or a sub-mailbox.
			if (folder || (*node)->mailboxName)
				if ((*node)->delimiter == 0)
					FetchDelimiter(imapStream, node);

			//
			// figure out the name of the new mailbox                                               
			//

			if ((err =
			     BuildIMAPMailboxName(node, spec, folder,
						  &newMailboxName)) ==
			    noErr) {
				//
				// create the mailbox on the server
				//

				if (result =
				    CreateIMAPMailbox(imapStream,
						      newMailboxName)) {
					// fetch the attributes of the new mailbox to see if it exists
					result =
					    FetchMailboxAttributes
					    (imapStream, newMailboxName);
					if (result
					    && (imapStream->mailStream->
						fListResultsHandle ==
						nil)) {
						// nothing came back.  Look for the mailbox, but forget about the delimiter.
						Zero(scratch);
						BMD(newMailboxName,
						    scratch,
						    MIN(strlen
							(newMailboxName) -
							1,
							sizeof(Str255)));
						result =
						    FetchMailboxAttributes
						    (imapStream, scratch);
					}
					newNode =
					    imapStream->mailStream->
					    fListResultsHandle;
					imapStream->mailStream->
					    fListResultsHandle = nil;

					if (result && newNode) {
						//
						// add the mailbox to the local tree.   
						//

						// kill the children we got.  Could be unfortunate enough to have someone else adding mailboxes
						// between the time we created our and doing the LIST.  
						LDRef(newNode);
						if ((*newNode)->next)
							DisposeMailboxTree
							    (&
							     ((*newNode)->
							      next));
						if ((*newNode)->childList)
							DisposeMailboxTree
							    (&
							     ((*newNode)->
							      childList));
						UL(newNode);

						// if this is a top level mailbox, add it to the list of top level mailboxes. 
						if ((*node)->
						    attributes & LATT_ROOT)
						{
							LL_Queue((*CurPers)->mailboxTree, newNode, (MailboxNodeHandle));
						} else	// Otherwise, add this mailbox to the parent's childlist
						{
							LL_Queue((*node)->
								 childList,
								 newNode,
								 (MailboxNodeHandle));
						}

						//
						// update the local cache
						//

						if ((UpdateLocalCache
						     (false) != noErr)
						    && !silent)
							IMAPError
							    (kIMAPCreateMailbox,
							     kIMAPCreateMailboxErr,
							     err);

						// make sure the right mailbox is returned.  The cache name may be different than the actual name
						*spec =
						    (*newNode)->
						    mailboxSpec;

						// clean up the conneciton we used to create the mailbox
						CleanupConnection
						    (&imapStream);
					}
				} else	// create mailbox failed, and we were told it did.
				{
					// report the IMAP error
					if (!silent)
						IMAPError
						    (kIMAPCreateMailbox,
						     kIMAPCreateMailboxErr,
						     errIMAPCreateMailbox);
					ZapPtr(newMailboxName);

				}
			} else
				WarnUser(MEM_ERR, err);
		} else
			WarnUser(MEM_ERR, err);

		// clean up the conneciton we used to create the mailbox
		if (imapStream)
			CleanupConnection(&imapStream);

		CurPers = oldPers;
	}
	// else 
	// this mailbox will end up outside an IMAP cache directory.  Regular POP mailbox.

	// tell the caller if the mailbox got created on the server
	if (success)
		*success = result;

	return (createdIMAPMailbox);
}

/***************************************************************************
 * BuildIMAPMailboxName - given a parent node and a new name, build the new
 *	mailbox's pathname.  Building a C-String.  Probably a way to do this
 *	with a stringutil.c function or something.
 *
 *	Note - this looks up the prefix and adds it if necessary.  Make sure
 *	 CurPers is set right when you call this.
 ***************************************************************************/
OSErr BuildIMAPMailboxName(MailboxNodeHandle parent, FSSpecPtr newSpec,
			   Boolean folder, CStr * newMailboxName)
{
	OSErr err = noErr;
	short len = 0;
	Str255 pPrefix;

	*newMailboxName = nil;

	pPrefix[0] = 0;
	GetRString(pPrefix, IMAP_MAILBOX_LOCATION_PREFIX);

	// figure out the name of the new mailbox
	len = ((*parent)->mailboxName ? strlen((*parent)->mailboxName) + 1 : pPrefix[0])	// length of pathname + delimiter character
	    + newSpec->name[0]	// length of new name
	    + (folder ? 1 : 0)	// trailing delimiter if this is a folder
	    + 1;		// null

	if ((*newMailboxName = NuPtr(len)) != 0) {
		if ((*parent)->mailboxName != 0) {
			// the pathname of the new mailbox
			strcpy(*newMailboxName, (*parent)->mailboxName);
			// add a delimiter
			(*newMailboxName)[strlen((*parent)->mailboxName)] =
			    (*parent)->delimiter;
			// NULL terminate before we do the strncat
			(*newMailboxName)[strlen((*parent)->mailboxName) +
					  1] = 0;
			// the new mailbox name
			strncat(*newMailboxName, (newSpec->name) + 1,
				newSpec->name[0]);
			// if we're creating a folder, add the delimiter to the end of the name
			if (folder)
				(*newMailboxName)[len - 2] =
				    (*parent)->delimiter;
			// NULL terminate just to be safe
			(*newMailboxName)[len - 1] = 0;
		} else		// no previous pathname
		{
			// prepend the location prefix to the new mailbox name. Assume it ends with a delimiter
			PtoCcpy(*newMailboxName, pPrefix);
			// the name of the new mailbox
			strncat(*newMailboxName, (newSpec->name) + 1,
				newSpec->name[0]);
			// if we're creating a folder, add the delimiter char at the end of the name
			if (folder)
				(*newMailboxName)[len - 2] =
				    (*parent)->delimiter;
			// NULL terminate just to be safe
			(*newMailboxName)[len - 1] = 0;
		}
	} else
		err = MemError();

	return (err);
}

/***************************************************************************
 * IMAPDeleteMailbox - Delete an IMAP mailbox.
 ***************************************************************************/
Boolean IMAPDeleteMailbox(FSSpecPtr toDelete)
{
	Boolean result = false;
	MailboxNodeHandle node = nil;
	MailboxNodeHandle parent = nil;
	PersHandle pers = nil;
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	OSErr err = noErr;

	// must be online to do this
	if (Offline && GoOnline())
		return (false);

	// must be able to modify the mailbox tree, too.
	if (!CanModifyMailboxTrees())
		return (false);

	// see if this is an IMAP cache mailbox we're deleting
	LocateNodeBySpecInAllPersTrees(toDelete, &node, &pers);

	if ((pers != nil) && (node != nil)) {
		//
		// delete the mailbox from the server.
		//

		CurPers = pers;
		if (imapStream =
		    GetIMAPConnection(UndefinedTask, CAN_PROGRESS)) {
			result = DeleteIMAPMailboxNode(imapStream, node);
			if (result) {
				//
				// close the mailbox and all of its open messages
				//

				CloseIMAPMailboxImmediately(FindTOC
							    (toDelete),
							    true);

				//
				// remove the mailbox from the mailbox tree     
				//

				// find parent of this mailbox
				parent =
				    LocateParentNode((*CurPers)->
						     mailboxTree, node);

				if ((*parent)->attributes & LATT_ROOT) {
					// remove this node from the top level mailboxes
					LL_Remove((*parent)->next, node,
						  (MailboxNodeHandle));
				} else {
					// remove this node from it's parent's childlist.
					LL_Remove((*parent)->childList,
						  node,
						  (MailboxNodeHandle));
				}

				// get rid of this node and it's children only.
				(*node)->next = nil;
				DisposeMailboxTree(&node);

				//
				// update the local cache
				//

				if ((UpdateLocalCache(false) != noErr))
					IMAPError(kIMAPDeleteMailbox,
						  kIMAPDeleteMailboxErr,
						  err);
			} else {
				IMAPError(kIMAPDeleteMailbox,
					  kIMAPDeleteMailboxErr,
					  errIMAPDeleteMailbox);
			}

			// clean up the connection we used to delete the mailbox
			CleanupConnection(&imapStream);
		} else
			WarnUser(MEM_ERR, err);

		CurPers = oldPers;
	}

	return (result);
}

/************************************************************************
 * IsIMAPMailboxEmpty - return true if the mailbox is really, truely empty
 ************************************************************************/
Boolean IsIMAPMailboxEmpty(FSSpecPtr mailboxSpec)
{
	MailboxNodeHandle node;
	PersHandle pers;

	if (IMAPMailboxMessageCount(mailboxSpec, false) == 0) {
		// there are no messages in this mailbox.  Does it have children?
		LocateNodeBySpecInAllPersTrees(mailboxSpec, &node, &pers);

		// it's empty unless it has children
		if (node && !(*node)->childList)
			return (true);
	}
	// there's something in this mailbox
	return (false);
}

/************************************************************************
 * IMAPMailboxMessageCount - return the number of messages in a mailbox.
 *	Hit the server with an EXAMINE command if we're allowed to.
 ************************************************************************/
long IMAPMailboxMessageCount(FSSpecPtr mailboxSpec, Boolean check)
{
	long messageCount = 0, localMessageCount = 0;
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	OSErr err = noErr;
	long flags = SA_MESSAGES;
	TOCHandle tocH;

	// Look in the mailbox for messages
	tocH = TOCBySpec(mailboxSpec);
	if (tocH && ((*tocH)->count))
		localMessageCount = (*tocH)->count;

	// Return the number of messages in the local mailbox if we don't care about an accurate count
	if (!check)
		messageCount = localMessageCount;

	if (messageCount == 0) {
		// Check the remote mailbox for the actual message count
		LocateNodeBySpecInAllPersTrees(mailboxSpec, &node, &pers);

		if (node && pers) {
			// the mailbox is not empty if it has children or if it has messages in the local cache
			if (!((*node)->childList)
			    && !((*node)->messageCount)) {
				CurPers = pers;

				// Only do the EXAMINE if the user hasn't told us not to.
				if (!PrefIsSet
				    (PREF_IMAP_NO_EXAMINE_ON_DELETE)) {
					if (!Offline || check) {
						// nor is it empty if the mailbox on the server claims it's not empty
						if (imapStream =
						    GetIMAPConnection
						    (UndefinedTask,
						     CAN_PROGRESS)) {
							LockMailboxNodeHandle
							    (node);
							// get the number of messages using the STATUS command
							if (FetchMailboxStatus(imapStream, (*node)->mailboxName, flags)) {
								messageCount
								    =
								    GetSTATUSMessageCount
								    (imapStream);
								messageCount = MAX(messageCount, localMessageCount);	// return the local message count if it's bigger
							}
							UnlockMailboxNodeHandle
							    (node);

							CleanupConnection
							    (&imapStream);;
						} else
							WarnUser(MEM_ERR,
								 err);
					}
				}

				CurPers = oldPers;
			}
		}
	}

	return (messageCount);
}

/***************************************************************************
 * DeleteIMAPMailboxNode - Given a mailbox node, delete the mailbox and all
 *	it's children from the IMAP server.
 ***************************************************************************/
Boolean DeleteIMAPMailboxNode(IMAPStreamPtr imapStream,
			      MailboxNodeHandle node)
{
	Boolean result = true;
	MailboxNodeHandle scan = nil;

	// kill this mailbox's child mailboxes
	if ((*node)->childList) {
		scan = (*node)->childList;
		while (scan && result) {
			result = DeleteIMAPMailboxNode(imapStream, scan);
			scan = (*scan)->next;
		}
	}
	//delete the mailbox itself.
	if (result) {
		LDRef(node);
		result =
		    DeleteIMAPMailbox(imapStream, (*node)->mailboxName);
		UL(node);
	}

	return (result);
}

/***************************************************************************
 * FetchDelimiter - Given an imap connection and a mailbox node, fill in 
 *	the delimiter field of the node with the delimiter char used by the
 *	mailbox.  We'll do a [LIST "" ""]
 ***************************************************************************/
Boolean FetchDelimiter(IMAPStreamPtr imapStream, MailboxNodeHandle node)
{
	Boolean result = false;
	MailboxNodeHandle n = nil;

	// Do a LIST "" ""
	LockMailboxNodeHandle(node);
	if (FetchMailboxAttributes(imapStream, nil)) {
		// FetchMailboxAttributes places its result in the mailStream ...
		n = imapStream->mailStream->fListResultsHandle;
		imapStream->mailStream->fListResultsHandle = nil;

		if (n) {
			(*node)->delimiter = (*n)->delimiter;
			ZapMailboxNode(&n);
		}
	}
	UnlockMailboxNodeHandle(node);
	(node);
	return (result);
}

/***************************************************************************
 * IsIMAPCacheName - is this name reserved for an IMAP cache folder?
 *	Reads from PersList
 ***************************************************************************/
Boolean IsIMAPCacheName(unsigned char *name)
{
	Str63 cacheName;
	Boolean isIMAPName = false;
	PersHandle pers = nil;

	for (pers = PersList; pers; pers = (*pers)->next) {
		PersNameToCacheName(pers, cacheName);
		if (StringSame(name, cacheName)) {
			isIMAPName = true;
			break;
		}
	}
	return (isIMAPName);
}

/***************************************************************************
 * IMAPExists - return true if there is at least one personality that
 *	foolishly wishes to do IMAP
 ***************************************************************************/
Boolean IMAPExists(void)
{
	Boolean exists = false;
	PersHandle oldPers = CurPers;

	for (CurPers = PersList; CurPers; CurPers = (*CurPers)->next) {
		if (PrefIsSet(PREF_IS_IMAP)) {
			exists = true;
			break;
		}
	}
	CurPers = oldPers;

	return (exists);
}

/***************************************************************************
 * SpecIsFilled - return true if the spec contains something useful
 ***************************************************************************/
Boolean SpecIsFilled(FSSpecPtr spec)
{
	return (spec && (spec->parID != 0) && (spec->name[0] != 0));
}

/***************************************************************************
 * IMAPRefreshAllCaches - refresh all the IMAP caches, connecting to 
 *	each server, fetching the mailbox list, and updating the local mailboxes
 ***************************************************************************/
OSErr IMAPRefreshAllCaches(void)
{
	PersHandle pers;

	for (pers = PersList; pers; pers = (*pers)->next)
		(*pers)->imapRefresh = 1;

	return (IMAPRefreshPersCaches());
}

/***************************************************************************
 * IMAPRefreshPersCaches - refresh all the IMAP caches or personalities
 *	that need it.
 ***************************************************************************/
OSErr IMAPRefreshPersCaches(void)
{
	OSErr err = noErr;
	PersHandle oldPers = CurPers;

	// must be online to do this
	if (Offline && GoOnline())
		return (OFFLINE);

	// collect passwords for the personalities to be refreshed
	for (CurPers = PersList; CurPers && (err != userCancelled);
	     CurPers = (*CurPers)->next) {
		// only do this for IMAP personalities.
		if (PrefIsSet(PREF_IS_IMAP) && ((*CurPers)->imapRefresh)) {
			if (!PrefIsSet(PREF_KERBEROS)
			    && (*CurPers)->password[0] == 0) {
				err = PersFillPw(CurPers, 0);
			}
		}
	}

	// do the actual refresh now.
	for (CurPers = PersList; CurPers && (err != userCanceledErr);
	     CurPers = (*CurPers)->next) {
		if (PrefIsSet(PREF_IS_IMAP) && ((*CurPers)->imapRefresh)
		    && (err != userCancelled)) {
			err = CreateLocalCache();
		}
		(*CurPers)->imapRefresh = 0;

		// forget the keychain password so it's not written to the settings file
		if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
			Zero((*CurPers)->password);

		if (CommandPeriod)
			err = userCanceledErr;
	}

	CurPers = oldPers;

	// Update all open TOCs so they point to valid IMAP mailbox nodes
	AttachOpenTocsToIMAPMailboxTrees();

	return (err);
}

/**********************************************************************
 * TransferLocalTreeInfo - go through new tree, transfer 
 *	- uidvalidity
 *	- if it's a trash mailbox
 *	- whether the mailbox needs to be filtered or not
 *	to the new tree.  This is data we store only locally.
 **********************************************************************/
void TransferLocalTreeInfo(MailboxNodeHandle oldTree,
			   MailboxNodeHandle newTree)
{
	MailboxNodeHandle newScan = newTree, oldScan = nil;

	if (newTree && oldTree) {
		while (newScan) {
			if ((*newScan)->mailboxName) {
				LDRef(newScan);
				if ((oldScan =
				     LocateNodeByMailboxName(oldTree,
							     (*newScan)->
							     mailboxName))
				    != 0) {
					(*newScan)->uidValidity =
					    (*oldScan)->uidValidity;
					if (IsIMAPTrashMailbox(oldScan))
						(*newScan)->attributes |=
						    LATT_TRASH;
					else if (IsIMAPJunkMailbox
						 (oldScan))
						(*newScan)->attributes |=
						    LATT_JUNK;
					if (DoesIMAPMailboxNeed
					    (oldScan, kNeedsFilter))
						SetIMAPMailboxNeeds
						    (newScan, kNeedsFilter,
						     true);
					if (DoesIMAPMailboxNeed
					    (oldScan, kNeedsPoll))
						SetIMAPMailboxNeeds
						    (newScan, kNeedsPoll,
						     true);
				}
				// do the children
				TransferLocalTreeInfo(oldTree,
						      (*newScan)->
						      childList);
				UL(newScan);
			}
			// next
			newScan = (*newScan)->next;
		}
	}
}

/***************************************************************************
 * LocateInboxForPers - return a handle to the INBOX of a given personality
 ***************************************************************************/
MailboxNodeHandle LocateInboxForPers(PersHandle pers)
{
	MailboxNodeHandle node;
	Str255 inbox, mName;

	// a non-personality has no inbox ...
	if (pers == nil)
		return (nil);

	GetRString(inbox, IMAP_INBOX_NAME);

	// scan only the top level of mailboxes for the inbox
	node = (*pers)->mailboxTree;
	while (node) {
		LockMailboxNodeHandle(node);
		PathToMailboxName((*node)->mailboxName, mName,
				  (*node)->delimiter);
		UnlockMailboxNodeHandle(node);

		if (StringSame(inbox, mName))
			break;

		node = (*node)->next;
	}

	return (node);
}

/***************************************************************************
 * IMAPRenameMailbox - rename an imap mailbox
 ***************************************************************************/
Boolean IMAPRenameMailbox(FSSpecPtr cacheFolderSpec, UPtr name)
{
	Boolean result = false;
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	OSErr err = noErr;
	char newName[MAILTMPLEN + 4];
	FSSpec cacheFileSpec, newSpec;
	Str63 newCacheName;
	TOCHandle tocH = nil;

	// make sure we have an old name and a new name
	if (!cacheFolderSpec || !name)
		return (false);

	// figure out who this mailbox belongs to
	SimpleMakeFSSpec(cacheFolderSpec->vRefNum,
			 SpecDirId(cacheFolderSpec), cacheFolderSpec->name,
			 &cacheFileSpec);
	LocateNodeBySpecInAllPersTrees(&cacheFileSpec, &node, &pers);

	// figure out the new name of the mailbox       
	if ((pers != nil) && (node != nil)) {
		// must be online to do this
		if (Offline && GoOnline())
			return (false);

		// make sure the new name doesn't have any delimiters in it.
		if (PIndex(name, (*node)->delimiter))
			IMAPError(kIMAPRenameMailbox,
				  kIMAPMailboxNameInvalid,
				  errIMAPMailboxNameInvalid);
		else {
			// first, figure out the name of the new mailbox.  It's going to have the same path as the original.
			NewIMAPMailboxName(node, name, newName);
			if (newName[0]) {
				// the cache file name is going to be something like the path name ...
				PathToMailboxName(newName, newCacheName,
						  (*node)->delimiter);

				// tell the filters we're about to rename a mailbox
				newSpec = cacheFileSpec;
				PCopy(newSpec.name, newCacheName);
				TellFiltMBRename(&cacheFileSpec, &newSpec,
						 false, true, false);

				// Set up connection to the server
				CurPers = pers;
				if (imapStream =
				    GetIMAPConnection(UndefinedTask,
						      CAN_PROGRESS)) {
					// if the mailbox rename fails, but the new name is the same as the old name, continue
					// to rename the cache file.  Maybe it was a case change in the name.
					LDRef(node);
					if (((result =
					      RenameIMAPMailbox(imapStream,
								(*node)->
								mailboxName,
								newName))
					     == true)
					    || StringSame(cacheFileSpec.
							  name,
							  newCacheName)) {
						//
						// rename the local cache folder and file
						//

						if (HRename
						    (cacheFileSpec.vRefNum,
						     cacheFileSpec.parID,
						     cacheFileSpec.name,
						     newCacheName) ==
						    noErr)
							HRename
							    (cacheFolderSpec->
							     vRefNum,
							     cacheFolderSpec->
							     parID,
							     cacheFolderSpec->
							     name,
							     newCacheName);

						// update the mailboxNode with the new name.  Do this so TransferUIDValidty does the right thing.
						fs_give((void **)
							&((*node)->
							  mailboxName));
						(*node)->mailboxName =
						    cpystr(newName);
						PCopy((*node)->mailboxSpec.
						      name, newCacheName);

						// rename the window if it's open
						if (tocH =
						    FindTOC
						    (&cacheFileSpec)) {
							TOCSetDirty(tocH,
								    true);
							PCopy((*tocH)->
							      mailbox.spec.
							      name,
							      newCacheName);
							SetWTitle_
							    (GetMyWindowWindowPtr
							     ((*tocH)->
							      win),
							     newCacheName);
						}
						// clean up the connection used to rename the mailbox
						CleanupConnection
						    (&imapStream);

						// tell the filters about the renamed mailbox
						TellFiltMBRename
						    (&cacheFileSpec,
						     &newSpec, false,
						     false, false);

						//
						// update the local cache
						//

						// if this node has children, we may have to rename or move them around.
						if ((*node)->childList)
							err =
							    CreateLocalCache
							    ();
						// otherwise, just rename the mailbox
						else
							err =
							    UpdateLocalCache
							    (false);
					} else	// rename mailbox failed, and we were told it did.
					{
						IMAPError
						    (kIMAPRenameMailbox,
						     kIMAPRenameMailboxErr,
						     errIMAPRenameMailbox);
					}
					UL(node);

					// clean up after our connection
					if (imapStream)
						CleanupConnection
						    (&imapStream);
				}

				CurPers = oldPers;
			}
			// else
			// the new name is invalid
		}
	}
	// else 
	// this wasn't an IMAP mailbox.

	return (true);
}

/***************************************************************************
 * IMAPMoveMailbox - move an IMAP mailbox
 *	If lastToMove, then the cache is refreshed.  Also, if oneMoved and
 *	the rename fails, we refresh the cache as well
 ***************************************************************************/
Boolean IMAPMoveMailbox(FSSpecPtr fromFolderSpec, FSSpecPtr toSpec,
			Boolean lastToMove, Boolean oneMoved,
			Boolean * dontWarn)
{
	Boolean result = false;
	PersHandle toPers = nil, fromPers = nil;
	MailboxNodeHandle toNode = nil, fromNode = nil;
	IMAPStreamPtr imapStream = nil;
	PersHandle oldPers = CurPers;
	OSErr err = noErr;
	char *newMailboxName = 0;
	FSSpec fromSpec, newSpec;

	// make sure we have an old name and a new name
	if (!fromFolderSpec || !toSpec)
		return (false);

	// if the dirIDs of the two specs are the same, then we're not going to actually have to transfer anythin.
	if (fromFolderSpec->parID == toSpec->parID)
		return (true);

	// we'll be dealing with the file inside the fromFolder
	SimpleMakeFSSpec(fromFolderSpec->vRefNum, fromFolderSpec->parID,
			 fromFolderSpec->name, &fromSpec);
	fromSpec.parID = SpecDirId(&fromSpec);

	// figure out who the mailboxes belong to
	LocateNodeBySpecInAllPersTrees(toSpec, &toNode, &toPers);
	if (toNode && toPers) {
		LocateNodeBySpecInAllPersTrees(&fromSpec, &fromNode,
					       &fromPers);
		if (fromNode && fromPers) {
			// Currently only support moving mailboxes on the same server
			if ((fromPers == toPers)) {
				// allow moves to same location.  Do nothing.
				if (toNode == fromNode)
					return (true);

				// must be online to do this
				if (Offline && GoOnline())
					return (false);

				// must be able to modify the mailbox tree, too.
				if (!CanModifyMailboxTrees())
					return (false);

				CurPers = toPers;
				// the new mailbox name will be the destination plus the source name
				if (BuildIMAPMailboxName
				    (toNode, &fromSpec,
				     ((*fromNode)->childList != nil),
				     &newMailboxName) == noErr) {
					// Set up connection to the server
					if (imapStream =
					    GetIMAPConnection
					    (UndefinedTask,
					     CAN_PROGRESS)) {
						LDRef(fromNode);
						if ((result =
						     RenameIMAPMailbox
						     (imapStream,
						      (*fromNode)->
						      mailboxName,
						      newMailboxName)) ==
						    true) {
							// tell the filters we're about to move a mailbox
							TellFiltMBRename
							    (&fromSpec,
							     &fromSpec,
							     false, true,
							     false);

							// try to move the cache folder so we won't have to redownload everything
							if (HMove
							    (fromFolderSpec->
							     vRefNum,
							     fromFolderSpec->
							     parID,
							     fromFolderSpec->
							     name,
							     toSpec->parID,
							     nil) ==
							    noErr) {
								// update the mailboxNode with the new path info.  Do this so TransferUIDValidty does the right thing.
								fs_give((void **) &((*fromNode)->mailboxName));
								(*fromNode)->mailboxName = cpystr(newMailboxName);
							}
							// newSpec will point to the newly moved cache file ...
							newSpec = *toSpec;
							PCopy(newSpec.name,
							      fromSpec.
							      name);
							newSpec.parID =
							    SpecDirId
							    (&newSpec);

							// and tell the filters about the renamed mailbox
							TellFiltMBRename
							    (&fromSpec,
							     &newSpec,
							     false, false,
							     *dontWarn);

							// and don't warn again.
							*dontWarn = true;
						} else {
							IMAPError
							    (kIMAPMoveMailbox,
							     kIMAPMoveMailboxErr,
							     errIMAPMoveMailbox);
						}
						UL(fromNode);

						// clean up
						CleanupConnection
						    (&imapStream);

						// rebuild the mailbox tree for this personality if we need to.
						if (lastToMove
						    || (!result
							&& oneMoved))
							err =
							    CreateLocalCache
							    ();

						ZapPtr(newMailboxName);
						if (imapStream)
							CleanupConnection
							    (&imapStream);
					} else
						WarnUser(MEM_ERR, err);

				} else	// failed to get the new name.  Memory Error!
					WarnUser(MEM_ERR, err);

				CurPers = oldPers;
			}
		}
	}

	return (result);
}

/***************************************************************************
 * NewIMAPMailboxName - given a mailbox path name, and a new name, build
 *	a new path name.
 ***************************************************************************/
char *NewIMAPMailboxName(MailboxNodeHandle node, UPtr name, char *newName)
{
	char *scan;

	if (!node || !name || !*name || !newName)
		return (nil);

	// the new name will contain all but the last directory in the old name
	strcpy(newName, (*node)->mailboxName);
	// chop off trailing delimiter if there is one
	if (newName[strlen(newName) - 1] == (*node)->delimiter)
		newName[strlen(newName) - 1] = nil;
	// chop off everything up until the last delimiter
	while (scan = &newName[strlen(newName) - 1]) {
		if ((*scan != (*node)->delimiter) && scan >= newName)
			*scan = nil;
		else
			break;
	}
	// now add the new mailbox name
	strncat(newName, name + 1, name[0]);

	return (newName);
}

/***************************************************************************
 * CanModifyMailboxTrees - return whether we are allowed to touch the tree
 ***************************************************************************/
Boolean CanModifyMailboxTrees(void)
{
	threadDataHandle index;

	if (GetNumBackgroundThreads() != 0) {
		// there are threads going.  If there is anything other than a
		// filtering thread going, we shouldn't touch the tree.
		for (index = gThreadData; index; index = (*index)->next) {
			if ((*index)->imapInfo.command != IMAPFilterTask)
				return (false);
		}
	}
	return (true);
}

/***************************************************************************
 * GetSpecialMailbox - return a handle to the special mailbox node.
 ***************************************************************************/
MailboxNodeHandle GetSpecialMailbox(PersHandle pers,
				    Boolean createIfNeeded, Boolean silent,
				    long mboxAtt)
{
	MailboxNodeHandle specialMBox = nil;
	PersHandle oldPers = CurPers;
	SignedByte state;

	// locate the mailbox flagged as the trash mailbox.
	CurPers = pers;
	state = HGetState((Handle) CurPers);
	LDRef(CurPers);
	if ((specialMBox =
	     LocateSpecialMailbox((*CurPers)->mailboxTree,
				  mboxAtt)) == nil) {
		// node was found.  Try and create it if we were asked to
		if (!createIfNeeded
		    || ((specialMBox = CreateSpecialMailbox(true, mboxAtt))
			== nil)) {
			// failed to locate the special mailbox.  Operation must not continue.
			if (!silent) {
				// Display proper error message depending on what we're doing
				if (SpecialMailboxIsTrash(mboxAtt))
					IMAPError(kIMAPTrashLocate,
						  kIMAPTrashLocateErr,
						  errIMAPNoTrash);
				else if (SpecialMailboxIsJunk(mboxAtt)
					 && !JunkPrefCantCreateJunk())
					IMAPError(kIMAPJunkLocate,
						  kIMAPJunkLocateErr,
						  errIMAPNoTrash);
			}
		} else {
			MBTickle(nil, nil);
		}
	}
	HSetState((Handle) CurPers, state);
	CurPers = oldPers;

	return (specialMBox);
}

/***************************************************************************
 * ResetSpecialMailbox - forget about a special mailbox for a personality
 ***************************************************************************/
void ResetSpecialMailbox(PersHandle pers, long mboxAtt)
{
	MailboxNodeHandle specialMBox = nil;
	PersHandle oldPers = CurPers;
	SignedByte state;

	// locate the mailbox flagged as the trash mailbox.
	CurPers = pers;
	state = HGetState((Handle) CurPers);
	LDRef(CurPers);

	while ((specialMBox =
		LocateSpecialMailbox((*CurPers)->mailboxTree,
				     mboxAtt)) != nil)
		(*specialMBox)->attributes &= ~mboxAtt;

	HSetState((Handle) CurPers, state);
	CurPers = oldPers;

	MBTickle(nil, nil);
}

/***************************************************************************
 * LocateSpecialMailbox - given a tree, find the first (and only) occurence
 *	of a special mailbox for the current personality.
 ***************************************************************************/
MailboxNodeHandle LocateSpecialMailbox(MailboxNodeHandle tree,
				       long mboxAtt)
{
	MailboxNodeHandle scan = tree;
	MailboxNodeHandle node = nil;

	while (scan) {
		// is this the node we're looking for?
		if (IsIMAPSpecialMailbox(scan, mboxAtt)) {
			node = scan;
			break;
		}
		// is the node we're looking for one of the children of this node?
		LockMailboxNodeHandle(scan);
		if ((*scan)->childList)
			node =
			    LocateSpecialMailbox(((*scan)->childList),
						 mboxAtt);
		UnlockMailboxNodeHandle(scan);

		if (node)
			break;

		// otherwise, check the next node.
		scan = (*scan)->next;
	}
	return (node);
}

/***************************************************************************
 * CreateSpecialMailbox - create a special mailbox for the current pers
 ***************************************************************************/
MailboxNodeHandle CreateSpecialMailbox(Boolean tryCreate, long mboxAtt)
{
	MailboxNodeHandle specialMbox = nil;
	MailboxNodeHandle inbox = nil;
	FSSpec specialSpec;
	Str255 specialMailboxName, scratch;
	OSErr err = noErr;
	Boolean created = false;
	SignedByte state;
	long reuseWarning, chooseMessage, selectMessage;

	// Don't do this if the user knows better.  Yeah right.
	if (JunkPrefCantCreateJunk())
		return (nil);

	// can't do this from a background thread, or if there's one running.
	if (!CanModifyMailboxTrees())
		return (nil);

	// set up mailbox name and messages depending on what kind of
	// special mailbox we're going to create.
	if (SpecialMailboxIsTrash(mboxAtt)) {
		reuseWarning = IMAP_TRASH_REUSE;
		chooseMessage = CHOOSE_IMAP_TRASH_MAILBOX;
		selectMessage = IMAP_TRASH_SELECT;
		GetRString(specialMailboxName, TRASH);
	} else if (SpecialMailboxIsJunk(mboxAtt)) {
		reuseWarning = IMAP_JUNK_SELECT;
		chooseMessage = CHOOSE_IMAP_JUNK_MAILBOX;
		selectMessage = IMAP_JUNK_SELECT;
		GetRString(specialMailboxName, JUNK);
	}
	specialMailboxName[specialMailboxName[0] + 1] = NULL;

	if (tryCreate) {
		// locate this personality's INBOX
		inbox = LocateInboxForPers(CurPers);
		if (inbox) {
			// try to create the special mailbox at the same level as the inbox
			PersNameToCacheName(CurPers, scratch);
			err =
			    FSMakeFSSpec(IMAPMailRoot.vRef,
					 IMAPMailRoot.dirId, scratch,
					 &specialSpec);
			if (err == noErr) {
				specialSpec.parID =
				    SpecDirId(&specialSpec);
				PCopy(specialSpec.name,
				      specialMailboxName);
				IMAPAddMailbox(&specialSpec, false,
					       &created, true);
				if (created) {
					// locate the special mailbox we just created.
					state =
					    HGetState((Handle) CurPers);
					LDRef(CurPers);
					specialMbox =
					    LocateNodeByMailboxName((*CurPers)->mailboxTree, specialMailboxName + 1);
					HSetState((Handle) CurPers, state);
				}
			}
		}
	}
	// Failed to create the special mailbox.  See if it already exists in the root level
	if (!specialMbox) {
		// find the special mailbox
		state = HGetState((Handle) CurPers);
		LDRef(CurPers);
		specialMbox =
		    LocateNodeByMailboxName((*CurPers)->mailboxTree,
					    specialMailboxName + 1);
		HSetState((Handle) CurPers, state);

		// did we find a mailbox with the same name?
		if (specialMbox) {
			// warn the user about using it
			if (ComposeStdAlert
			    (Note, reuseWarning, specialMailboxName,
			     (*CurPers)->name) == 2)
				specialMbox = nil;
		}
	}
	// Still no special mailbox.  Have the user pick one from the menus, unless we've failed before.
	if (!specialMbox) {
		if (ChooseSpecialMailbox
		    (CurPers, chooseMessage, &specialSpec)) {
			state = HGetState((Handle) CurPers);
			LDRef(CurPers);
			specialMbox =
			    LocateNodeBySpec((*CurPers)->mailboxTree,
					     &specialSpec);
			HSetState((Handle) CurPers, state);

			// Make sure the user really wants to use this mailbox as the special mailbox.
			if (specialMbox
			    &&
			    (ComposeStdAlert
			     (Note, selectMessage, specialSpec.name,
			      (*CurPers)->name) == 2))
				specialMbox = nil;
		}
	}
	// a special mailbox was either created or selected. Now go tell the mailbox it's been annointed
	if (specialMbox) {
		(*specialMbox)->attributes |= mboxAtt;
		LockMailboxNodeHandle(specialMbox);
		err =
		    WriteIMAPMailboxInfo(&((*specialMbox)->mailboxSpec),
					 specialMbox);
		UnlockMailboxNodeHandle(specialMbox);
	}

	return (specialMbox);
}

/************************************************************************
 * LocateIMAPJunkToc - given an IMAP mailbox, return the appropriate
 *	junk TOC.	
 ************************************************************************/
TOCHandle LocateIMAPJunkToc(TOCHandle tocH, Boolean createIfNeeded,
			    Boolean silent)
{
	PersHandle pers;
	MailboxNodeHandle mbox;

	if (tocH && (*tocH)->imapTOC) {
		pers = TOCToPers(tocH);
		if (pers) {
			mbox =
			    GetIMAPJunkMailbox(pers, createIfNeeded,
					       silent);
			if (mbox) {
				return (TOCBySpec(&(*mbox)->mailboxSpec));
			}
		}
	}
	return (NULL);
}

/************************************************************************
 *	IsIMAPSpecialMailbox - return true if this is a special IMAP mailbox
 ************************************************************************/
Boolean IsIMAPSpecialMailbox(MailboxNodeHandle node, long att)
{
	return ((node != NULL) && (((*node)->attributes & att) != 0));
}

/************************************************************************
 *	CanHaveChildren - return true if this node can have children
 ************************************************************************/
Boolean CanHaveChildren(MailboxNodeHandle node)
{
	return (((*node)->attributes & LATT_NOINFERIORS) == 0);
}

/************************************************************************
 * IMAPEmptyTrash - empty the trash mailboxes that need it.  Return
 *	true if no other emptying needs to be done.	
 ************************************************************************/
Boolean IMAPEmptyTrash(Boolean localOnly, Boolean currentOnly, Boolean all)
{
	Boolean emptyLocal = false;

	if (all) {
		//
		// empty all trash mailboxes
		//

		EmptyImapTrashes(kEmptyAllTrashes);
		emptyLocal = true;	// and have caller empty the local trash, too.
	} else if (localOnly) {
		//
		// empty local trash mailbox only
		//

		emptyLocal = true;
	} else if (currentOnly) {
		//
		// empty current trash mailbox only
		//

		WindowPtr winWP = MyFrontWindow();
		MyWindowPtr win = GetWindowMyWindowPtr(winWP);
		short kind = winWP ? GetWindowKind(winWP) : 0;
		TOCHandle tocH = nil;

		// look at the frontmost window
		if (win && kind == MBOX_WIN)
			tocH = (TOCHandle) GetMyWindowPrivateData(win);

		if (tocH) {
			if ((*tocH)->imapTOC) {
				// frontmost window is an IMAP mailbox.  Empty the trash for the personality it belongs to.
				PushPers(TOCToPers(tocH));
				if (CurPers)
					IMAPEmptyPersTrash();
				PopPers();

				emptyLocal = false;	// empty the local trash.
			} else {
				// frontmost window is a POP mailbox.  Empty the local trash.
				emptyLocal = true;
			}
		} else {
			// no front window.  Empty the local trash.
			emptyLocal = true;
		}
	} else {
		//
		// Local trash, and any active IMAP trashes
		//

		EmptyImapTrashes(kEmptyActiveTrashes);
		emptyLocal = true;	// and have caller empty the local trash, too.
	}

	return (emptyLocal);
}

/************************************************************************
 *	IMAPCountTrashMessages - return the number of messages in the local
 *	 IMAP trash mailboxes.
 ************************************************************************/
short IMAPCountTrashMessages(Boolean localOnly, Boolean currentOnly,
			     Boolean all)
{
	short totalFound = 0;
	MailboxNodeHandle trash = nil;
	TOCHandle tocH = nil;
	PersHandle pers;
	FSSpec mailboxSpec;

	if (!localOnly) {
		// We are checking remote IMAP mailboxes for messages, at least.  We must be online
		if (Offline && GoOnline())
			return (0);

		if (currentOnly) {
			//
			// Only Current trash mailbox
			//

			WindowPtr winWP = MyFrontWindow();
			MyWindowPtr win = GetWindowMyWindowPtr(winWP);
			short kind = winWP ? GetWindowKind(winWP) : 0;
			TOCHandle tocH = nil;
			TOCHandle trashToc = nil;

			// look at the frontmost window
			if (win && kind == MBOX_WIN)
				tocH =
				    (TOCHandle)
				    GetMyWindowPrivateData(win);

			if (tocH && ((*tocH)->imapTOC)) {
				// figure out which IMAP personality this mailbox belongs to
				pers = TOCToPers(tocH);
				if (pers) {
					// find the trash mailbox of the personality
					trash =
					    GetIMAPTrashMailbox(pers,
								false,
								true);
					if (trash) {
						// see how many messages are in the trash
						LockMailboxNodeHandle
						    (trash);
						trashToc =
						    TOCBySpec(&
							      ((*trash)->
							       mailboxSpec));
						UnlockMailboxNodeHandle
						    (trash);
						if (trashToc) {
							mailboxSpec =
							    GetMailboxSpec
							    (trashToc, -1);
							totalFound +=
							    IMAPMailboxMessageCount
							    (&mailboxSpec,
							     true);
						}
					}
				}
			}
		} else {
			//
			// All Trash Mailboxes or Active Trash Mailboxes
			//
			for (pers = PersList; pers; pers = (*pers)->next) {
				// is this an IMAP personality?
				if (IsIMAPPers(pers)) {
					// consider only active personalities, unless we all is true
					if (all || IMAPPersActive(pers)) {
						trash =
						    GetIMAPTrashMailbox
						    (pers, false, true);
						if (trash) {
							LockMailboxNodeHandle
							    (trash);
							tocH =
							    TOCBySpec(&
								      ((*trash)->mailboxSpec));
							UnlockMailboxNodeHandle
							    (trash);
							if (tocH) {
								mailboxSpec
								    =
								    GetMailboxSpec
								    (tocH,
								     -1);
								totalFound
								    +=
								    IMAPMailboxMessageCount
								    (&mailboxSpec,
								     true);
							}
						}
					}
				}
			}
		}
	}
	return (totalFound);
}

/************************************************************************
 * ChooseSpecialMailbox - let the user pick a mailbox from the menus
 ************************************************************************/
Boolean ChooseSpecialMailbox(PersHandle pers, short msg,
			     FSSpecPtr specialSpec)
{
	short mId;
	WindowPtr theWindow;
	EventRecord event;
	MyWindowPtr dgPtrWin;
	DialogPtr dgPtr;
	Boolean rslt = false;
	Str255 msgStr;
	MenuHandle mh = GetMHandle(MAILBOX_MENU);
	short count;
	Str255 itemText;
	MenuHandle submh;
	short submId;
	short subCount;
	Str255 inboxStr;

	PushGWorld();

	//
	// put up message and disable all but the Mailbox menu
	//

	// disable all the menus but the Mailbox menu
	for (mId = APPLE_MENU; mId < MENU_LIMIT; mId++)
		DisableItem(GetMHandle(mId), 0);
	DisableItem(GetMHandle(WINDOW_MENU), 0);
	EnableItem(mh, 0);
	DrawMenuBar();

	// disable all the items in the Mailbox menu except for the personality cache
	for (count = 1; count <= CountMenuItems(mh); count++) {
		MyGetItem(mh, count, itemText);
		EnableIf(mh, count, StringSame((*pers)->name, itemText));
		if (StringSame((*pers)->name, itemText)) {
			// disable the Inbox item within the personality cache, unless it has children
			GetRString(inboxStr, IMAP_INBOX_NAME);
			if (submId = SubmenuId(mh, count)) {
				if (submh = GetMHandle(submId)) {
					for (subCount = 1;
					     subCount <=
					     CountMenuItems(submh);
					     subCount++) {
						MyGetItem(submh, subCount,
							  itemText);
						if (StringSame
						    (inboxStr, itemText)) {
							if (SubmenuId
							    (submh,
							     subCount) ==
							    0) {
								EnableIf
								    (submh,
								     subCount,
								     !StringSame
								     (inboxStr,
								      itemText));
							}
							break;
						}
					}
				}
			}
		}
	}

	ParamText(GetRString(msgStr, msg), "", "", "");
	dgPtrWin = GetNewMyDialog(XFER_MENU_DLOG, nil, nil, InFront);
	dgPtr = GetMyWindowDialogPtr(dgPtrWin);
//      dgPtr = GetNewDialog(XFER_MENU_DLOG,nil,InFront);
	AutoSizeDialog(dgPtr);
	DrawDialog(dgPtr);
	SetMyCursor(arrowCursor);
	SFWTC = True;

	// Forget about any clicking that was done to get here.  
	// We may be doing this immediately after a drag, for example.
	FlushEvents(mDownMask | mUpMask | keyDownMask, 0);

	//
	//  now, let the user choose something:
	//

	for (;;) {
		if (WNE
		    (mDownMask | keyDownMask | updateMask, &event,
		     REAL_BIG)) {
			if (event.what == updateEvt)
				MiniMainLoop(&event);
			//      Check for mouseDown and keyDown events. WNE in OS X
			//      is currently return more than what's in the event mask. 
			else if (event.what == mouseDown
				 || event.what == keyDown)
				break;
		}
	}
	if (event.what == mouseDown) {
		if (inMenuBar == FindWindow_(event.where, &theWindow)) {
			long mSelect = MenuSelect(event.where);
			short mnu, itm;

			mnu = (mSelect >> 16) & 0xffff;
			itm = mSelect & 0xffff;
			if (mnu == MAILBOX_MENU || IsMailboxSubmenu(mnu)) {
				rslt = itm
				    && GetTransferParams(mnu, itm,
							 specialSpec, nil);
			}
			HiliteMenu(0);
		}
	}
	DisposDialog_(dgPtr);
	// re-enable the menus
	for (count = 1; count <= CountMenuItems(mh); count++) {
		MyGetItem(mh, count, itemText);
		EnableIf(mh, count, true);
		if (StringSame((*pers)->name, itemText)) {
			// re-enable the Inbox item within the personality cache
			if (submId = SubmenuId(mh, count)) {
				if (submh = GetMHandle(submId)) {
					for (subCount = 1;
					     subCount <=
					     CountMenuItems(submh);
					     subCount++) {
						EnableIf(submh, subCount,
							 true);
					}
				}
			}
		}
	}
	EnableMenus(FrontWindow_(), False);
	PopGWorld();
	return (rslt);
}

/************************************************************************
 * FancyTrashForThisPers - return true is FTM is on for the personality
 *	that owns the mailbox specified by the TOC
 ************************************************************************/
Boolean FancyTrashForThisPers(TOCHandle tocH)
{
	Boolean result = false;
	MailboxNodeHandle node = nil;
	PersHandle pers = nil;
	PersHandle oldPers = CurPers;

	// must have a tocH
	if (tocH) {
		// must be an IMAP toc
		if (!(*tocH)->imapTOC)
			return (false);
		{
			// who does this toc belong to?
			node = TOCToMbox(tocH);
			pers = TOCToPers(tocH);
			if (node && pers) {
				// see if FTM is on for this personality
				CurPers = pers;
				result =
				    !PrefIsSet(PREF_IMAP_NO_FANCY_TRASH);
				CurPers = oldPers;
			}
		}
	}
	return (result);
}

/************************************************************************
 * MailboxTreeGood - return true if the mailbox list for pers is good
 ************************************************************************/
Boolean MailboxTreeGood(PersHandle pers)
{
	return ((pers) && ((*pers)->mailboxTree)
		&& (((*((*pers)->mailboxTree))->next)
		    || ((*((*pers)->mailboxTree))->childList)));
}

/************************************************************************
 * GetIMAPAttachFolder - make a spec for a file in the current
 *	personality's imap attachments directory.
 *
 *	Note, we use FSPExchangeFiles to turn stubs into full attachments.
 *	So the stub has to start its life on the same volume.
 ************************************************************************/
OSErr GetIMAPAttachFolder(FSSpecPtr attachSpec)
{
	OSErr err = noErr;
	FSSpec spec, attachFolderSpec, imapStubSpec;
	Str255 cacheName, name;

	PersNameToCacheName(CurPers, cacheName);

	// if the stub folder and attachment folder is on the same drive, use the IMAP Attachments folder in
	// the IMAP cache directory
	GetAttFolderSpec(&attachFolderSpec);
	if (attachFolderSpec.vRefNum == IMAPMailRoot.vRef) {
		if ((err =
		     FSMakeFSSpec(IMAPMailRoot.vRef, IMAPMailRoot.dirId,
				  cacheName, &spec)) == noErr) {
			if ((err =
			     FSMakeFSSpec(spec.vRefNum, SpecDirId(&spec),
					  GetRString(name,
						     IMAP_ATTACH_FOLDER),
					  &spec)) == noErr) {
				attachSpec->vRefNum = spec.vRefNum;
				attachSpec->parID = SpecDirId(&spec);
			}
		}
	} else {
		// otherwise, make sure there's a stub directory in the attachment folder
		if ((err =
		     EnsureIMAPCacheFolders(&attachFolderSpec,
					    &imapStubSpec)) == noErr) {
			// and use the stub folder on the other volume
			attachSpec->vRefNum = imapStubSpec.vRefNum;
			attachSpec->parID = SpecDirId(&imapStubSpec);
		}
	}
	return (err);
}

/************************************************************************
 * EnsureIMAPCacheFolders - build a hierarchy of attachment folders for
 *	IMAP stubs inside the attach directory.  Stub is returned pointing
 *	to the IMAP Attachment Stub directory of the current personality.
 ************************************************************************/
OSErr EnsureIMAPCacheFolders(FSSpecPtr attach, FSSpecPtr imapAttach)
{
	OSErr err = noErr;
	Str255 name, cacheName;
	PersHandle oldPers = CurPers;
	FSSpec spec, persSpec, stubSpec;
	long junk;

	//
	// create a folder inside the attach folder to enclose the personality caches
	//

	err =
	    FSMakeFSSpec(attach->vRefNum, attach->parID,
			 GetRString(name, IMAP_SAFE_ATTACH_FOLDER), &spec);
	if (err == fnfErr) {
		// create a folder for all the personality caches
		err = FSpDirCreate(&spec, smSystemScript, &junk);
	}

	if (err == noErr) {
		//
		// Loop through the personalities, creating a folder with the same name as the personality
		//

		for (CurPers = PersList; CurPers && (err == noErr);
		     CurPers = (*CurPers)->next) {
			// if this is an IMAP personality ...
			if (IsIMAPPers(CurPers)) {
				// see if the personality already has a cache
				PersNameToCacheName(CurPers, cacheName);

				err =
				    FSMakeFSSpec(spec.vRefNum,
						 SpecDirId(&spec),
						 cacheName, &persSpec);
				if (err == fnfErr)	// it doesn't.
				{
					// create a cache folder for this personality
					err =
					    FSpDirCreate(&persSpec,
							 smSystemScript,
							 &junk);
				}

				if (err == noErr) {
					// create an IMAP Attachments folder inside this folder
					err =
					    FSMakeFSSpec(persSpec.vRefNum,
							 SpecDirId
							 (&persSpec),
							 GetRString(name,
								    IMAP_ATTACH_FOLDER),
							 &stubSpec);
					if (err == fnfErr)
						err =
						    FSpDirCreate(&stubSpec,
								 smSystemScript,
								 &junk);

					// remember this directory
					if (CurPers == oldPers)
						*imapAttach = stubSpec;
				}
			}
		}
		CurPers = oldPers;
	}

	return (err);
}

/************************************************************************
 * TOCToMbox - given a TOC, return a handle to it's mailboxnode
 ************************************************************************/
MailboxNodeHandle TOCToMbox(TOCHandle tocH)
{
	return ((*tocH)->imapMBH);
}

/************************************************************************
 * TOCToPers - given a TOC, return the owning personality
 ************************************************************************/
PersHandle TOCToPers(TOCHandle tocH)
{
	return (tocH ? MailboxNodeToPers((*tocH)->imapMBH) : NULL);
}

/************************************************************************
 * MailboxNodeToPers - given a MailboxNodeHandle, return the owning 
 *	personality
 ************************************************************************/
PersHandle MailboxNodeToPers(MailboxNodeHandle mbox)
{
	return (mbox ? FindPersById((*mbox)->persId) : NULL);
}

/************************************************************************
 * SalvageIMAPTOC - additional step to salvage an IMAP toc.
 *
 * Since IMAP messages reside on the server, this is not as big of a deal
 *	as a local mailbox.  To rebuild an IMAP toc, we do the following:
 *
 *	- SlavageTOC() to save all downloaded messages.  This is currently
 *		done by the caller.
 *
 *	- If there were some salvagable messages, the TOC is probably more or 
 *		less in order.  Save all the minimal headers.
 *
 *	- If we saved any minimal headers, go through the new TOC and check 
 *		for duplicate minimal headers.
 *
 *	Also, be sure to update the subject, from, and status when fetching
 *	 an IMAP message from the server, just in case.
 ************************************************************************/
void SalvageIMAPTOC(TOCHandle oldTocH, TOCHandle newTocH, short *newCount)
{
	short sumNum;
	short minHeaderCount;
	short i, j;

	// Only do this on an IMAP toc!
	if ((*oldTocH)->imapTOC && (*newTocH)->imapTOC) {
		//
		// remove all messages with bogus UIDs
		//

		for (sumNum = (*newTocH)->count - 1; sumNum >= 0; sumNum--) {
			if (((*newTocH)->sums[sumNum].msgIdHash == (*newTocH)->sums[sumNum].uidHash)	// bogus UID
			    || !ValidHash((*newTocH)->sums[sumNum].uidHash))	// no UID
			{
				DeleteIMAPSum(newTocH, sumNum);
				if (newCount && *newCount)
					*newCount = *newCount - 1;
			}
		}

		//
		//      Save the minimal headers if any messages were slavagable
		//

		if (*newCount > 0) {
			minHeaderCount = 0;
			for (i = 0; i < (*oldTocH)->count; i++) {
				if (!IMAPMessageDownloaded(oldTocH, i)) {
					SaveMessageSum(&(*oldTocH)->
						       sums[i], &newTocH);
					minHeaderCount++;
				}
			}

			//
			// Check for headers with duplicate ids.
			//

			if (minHeaderCount > 0) {
				// we copied some minimal headers.  Go through each one and make sure it has a unique UID.
				for (i = 0; i < (*newTocH)->count; i++) {
					if (!IMAPMessageDownloaded
					    (newTocH, i)) {
						for (j = 0;
						     j < (*newTocH)->count;
						     j++) {
							if ((i != j)
							    &&
							    !IMAPMessageDownloaded
							    (newTocH, j)) {
								if ((*newTocH)->sums[i].uidHash == (*newTocH)->sums[j].uidHash) {
									// duplicate found.  Remove the duplicate
									DeleteIMAPSum
									    (newTocH,
									     j);
									minHeaderCount--;
									j--;	// and make sure we don't skip the next one.
								}
							}
						}
					}
				}
			}
			// add the number of salvaged minimal headers to the new count
			*newCount += minHeaderCount;
		}
	}
}

/************************************************************************
 * LockMailboxNodeHandle - makes sure the mailbox node stays locked in
 *	memory
 ************************************************************************/
void LockMailboxNodeHandle(MailboxNodeHandle node)
{
	LDRef(node);
	(*node)->lockCount++;
}

/************************************************************************
 * UnlockMailboxNodeHandle - decrements lock count, unlocks mailbox node
 *	in memory once count reaches 0.
 ************************************************************************/
void UnlockMailboxNodeHandle(MailboxNodeHandle node)
{
	(*node)->lockCount--;
	if ((*node)->lockCount < 1) {
		(*node)->lockCount = 0;	// handicap for my mental retardation
		UL(node);
	}
}

/************************************************************************
 * ClosePersMailboxes - close all mailboxes belonging to a personality.
 *	This is necessary when an IMAP cache is removed.
 ************************************************************************/
void ClosePersMailboxes(PersHandle pers)
{
	TOCHandle tocH, nextTocH;

	for (tocH = TOCList; tocH; tocH = nextTocH) {
		nextTocH = (*tocH)->next;

		// should we close this toc?
		if (TOCToPers(tocH) == pers) {
			// close the IMAP mailbox, skip any final processing
			CloseIMAPMailboxImmediately(tocH, false);
		}
	}
}

/************************************************************************
 * CloseChildMailboxes - close all child mailboxes.  This is necessary 
 *	when an IMAP mailbox is removed.
 ************************************************************************/
void IMAPCloseChildMailboxes(FSSpecPtr spec)
{
	MailboxNodeHandle node;
	PersHandle pers;
	TOCHandle tocH, nextTocH;
	FSSpec tocSpec;

	// does this mailbox have children?
	LocateNodeBySpecInAllPersTrees(spec, &node, &pers);
	if (node && (*node)->childList) {
		// iterate over all open windows ...
		for (tocH = TOCList; tocH; tocH = nextTocH) {
			nextTocH = (*tocH)->next;

			// should we close this toc?
			tocSpec = (*tocH)->mailbox.spec;
			if (LocateNodeBySpec((*node)->childList, &tocSpec)) {
				// close the IMAP mailbox, skip any final processing
				CloseIMAPMailboxImmediately(tocH, false);
			}
		}
	}
}

/************************************************************************
 * CloseIMAPMailboxImmediately - close an IMAP mailbox, skip final
 *  processing.  Close the hidden mailbox if requested
 ************************************************************************/
void CloseIMAPMailboxImmediately(TOCHandle tocH, Boolean bHiddenToo)
{
	MailboxNodeHandle mbox;
	short sumNum;

	if (tocH) {
		// skip final processing
		mbox = TOCToMbox(tocH);
		if (mbox) {
			// don't try to execute its queued commands
			SetIMAPMailboxNeeds(mbox, kNeedsExecCmd, false);

			// don't expunge it on quit
			SetIMAPMailboxNeeds(mbox, kNeedsAutoExp, false);

			// handle hidden mailbox as well.
			if (bHiddenToo)
				CloseIMAPMailboxImmediately
				    (GetHiddenCacheMailbox
				     (mbox, true, false), false);
		}
		// close all message windows
		TOCSetDirty(tocH, false);
		for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
			if ((*tocH)->sums[sumNum].messH)
				CloseMyWindow(GetMyWindowWindowPtr
					      ((*(*tocH)->sums[sumNum].
						messH)->win));

		// close the mailbox window
		if ((*tocH)->win)
			CloseMyWindow(GetMyWindowWindowPtr((*tocH)->win));
	}
}


/**********************************************************************
 * IMAPMailboxHasUnread - delude ourselves into beliving this mailbox
 *	has something important in it.
 **********************************************************************/
Boolean IMAPMailboxHasUnread(TOCHandle tocH, Boolean itDoesNow)
{
	OSErr err = noErr;
	CInfoPBRec mailboxFileInfo;
	FSSpec boxSpec = (*tocH)->mailbox.spec;
	short myMenu;
	short myItem;
	Style oldStyle;
	Boolean alreadyHadUnread = false;

	myItem = 0;
	Spec2Menu(&boxSpec, False, &myMenu, &myItem);

	// is the mailbox in the menus?
	if (myItem > 0) {
		// is the mailbox already marked as having unread messages?
		GetItemStyle(GetMHandle(myMenu), myItem, &oldStyle);
		alreadyHadUnread = oldStyle == UnreadStyle;

		// mark it as having unread messages if we ought to
		if (!alreadyHadUnread && itDoesNow) {
			//      Find the mailbox cache file on disk ...
			Zero(mailboxFileInfo);
			err =
			    HGetCatInfo(boxSpec.vRefNum, boxSpec.parID,
					boxSpec.name, &mailboxFileInfo);
			if (err == noErr) {
				// Set the label of this mailbox so Eudora sees it as having unread mail.
				mailboxFileInfo.hFileInfo.ioFlFndrInfo.
				    fdFlags |= 0xe;
				// Tweak the mod date, too.
				mailboxFileInfo.hFileInfo.ioFlMdDat =
				    LocalDateTime();

				err =
				    HSetCatInfo(boxSpec.vRefNum,
						boxSpec.parID,
						boxSpec.name,
						&mailboxFileInfo);
				if (err == noErr) {
					// whack the menus ...
					FixSpecUnread(&boxSpec, 1);
					FixMenuUnread(GetMHandle(myMenu),
						      myItem, 1);
				}
			}
		}
	}

	return (alreadyHadUnread);
}

/**********************************************************************
 * IMAPDontFccMailbox - return true if this mailbox shouldn't be auto-
 *	fcc'ed to.
 **********************************************************************/
Boolean IMAPDontAutoFccMailbox(TOCHandle tocH)
{
	Boolean result = false;
	MailboxNodeHandle mbox;
	PersHandle pers;

	// must have a toc to do Fcc at all
	if (!tocH)
		return (true);

	// if this isn't an IMAP toc, assume we can Fcc to it
	if (!(*tocH)->imapTOC)
		return (false);

	// find which mailbox this TOC points to
	mbox = TOCToMbox(tocH);
	pers = TOCToPers(tocH);

	// is this mailbox one that shouldn't be auto-fcc'ed to?        
	if (mbox && pers) {
		// Don't auto Fcc to this mailbox if this is an IMAP Inbox
		result = (mbox == LocateInboxForPers(pers));

		// Don't auto Fcc to this mailbox if this is an IMAP trash mailbox
		if (!result)
			result = IsIMAPTrashMailbox(mbox);
	}

	return (result);
}

/**********************************************************************
 * AttachOpenTocsToIMAPMailboxTrees - tell all open Tocs about a change
 *	in the mailbox trees
 **********************************************************************/
void AttachOpenTocsToIMAPMailboxTrees(void)
{
	TOCHandle tocH;
	PersHandle pers;
	MailboxNodeHandle mbox;
	FSSpec mboxSpec;

	for (tocH = TOCList; tocH; tocH = (*tocH)->next) {
		if ((*tocH)->imapTOC) {
			mboxSpec = ((*tocH)->mailbox.spec);
			LocateNodeBySpecInAllPersTrees(&mboxSpec, &mbox,
						       &pers);
			(*tocH)->imapMBH = mbox;
		}
	}
}

/**********************************************************************
 * SetIMAPMailboxNeeds - Set the needs member of an mailbox
 **********************************************************************/
void SetIMAPMailboxNeeds(MailboxNodeHandle node, MailboxNeedsEnum flag,
			 Boolean on)
{
	if (node) {
		if (on)
			(*node)->mailboxneeds |= flag;
		else
			(*node)->mailboxneeds &= ~flag;
	}
}

/************************************************************************
 * IMAPCountMailboxes - count the number of mailboxes with a certain
 *	flag set
 ************************************************************************/
long IMAPCountMailboxes(MailboxNodeHandle tree, MailboxNeedsEnum needs)
{
	long found = 0;

	while (tree) {
		if (DoesIMAPMailboxNeed(tree, needs))
			found++;
		if ((*tree)->childList)
			found +=
			    IMAPCountMailboxes((*tree)->childList, needs);
		tree = (*tree)->next;
	}
	return (found);
}

/**********************************************************************
 * UIDToSumNum - return the sumnum of the message with a given UID.
 **********************************************************************/
short UIDToSumNum(unsigned long uid, TOCHandle tocH)
{
	short sumNum;

	for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
		if ((*tocH)->sums[sumNum].uidHash == uid)
			return (sumNum);

	return (-1);
}

/**********************************************************************
 * IMAPMailboxTitle - provide custom titles for special IMAP boxes.
 *	Do this for all non-Dominant IMAP mailboxes.
 **********************************************************************/
Boolean IMAPMailboxTitle(TOCHandle tocH, Str255 title)
{
	MailboxNodeHandle mbox;
	PersHandle pers;

	if (tocH && (*tocH)->imapTOC) {
		mbox = TOCToMbox(tocH);
		pers = TOCToPers(tocH);
		if (mbox && pers && (pers != PersList)) {
			ComposeRString(title, IMAP_MAILBOXTITLE_FMT,
				       (*tocH)->mailbox.spec.name,
				       (*pers)->name);
			return (true);
		}
	}
	return (false);
}

/**********************************************************************
 * IMAPMailboxExists - does this IMAP mailbox exist on the server?
 **********************************************************************/
Boolean IMAPMailboxExists(Str255 mailboxName)
{
	Boolean bExists = false;
	IMAPStreamPtr imapStream = nil;
	Str255 cName;

	// must be online to do this
	if (Offline && GoOnline())
		return (nil);

	Zero(cName);
	BMD(mailboxName + 1, cName, mailboxName[0]);

	// Set up to connect to the server
	if (imapStream = GetIMAPConnection(UndefinedTask, CAN_PROGRESS)) {
		// fetch the attributes of the new mailbox to see if it exists
		if (FetchMailboxAttributes(imapStream, cName)) {
			// the mailbox exists!  Add it to the tree
			if (imapStream->mailStream->fListResultsHandle) {
				ZapHandle(imapStream->mailStream->
					  fListResultsHandle);
				bExists = true;
			}
		}
	}
	// clean up the conneciton we used to create the mailbox
	if (imapStream)
		CleanupConnection(&imapStream);

	return (bExists);
}

/**********************************************************************
 * GetHiddenCacheMailbox - find and open the cache mailbox where
 *	deleted messages are stored.  Create it if needed.
 **********************************************************************/
TOCHandle GetHiddenCacheMailbox(MailboxNodeHandle mbox, Boolean bForce,
				Boolean bCreateIfNeeded)
{
	TOCHandle tocH = NULL;
	FSSpec hidSpec;
	Boolean bExists;

	if (mbox && (bForce || !DoesIMAPMailboxNeed(mbox, kShowDeleted))) {
		// figure out what the deleted cache would be called
		hidSpec = (*mbox)->mailboxSpec;
		GetRString(hidSpec.name, IMAP_HIDDEN_TOC_NAME);

		// does it already exist?
		bExists = (FSpExists(&hidSpec) == noErr);

		// create it if needed
		if (bCreateIfNeeded && !bExists) {
			if (FSpCreate
			    (&hidSpec, CREATOR, IMAP_MAILBOX_TYPE,
			     smSystemScript) == noErr)
				bExists = true;
		}
		// open the mailbox
		if (bExists)
			tocH = TOCBySpec(&hidSpec);

		// and associate it with the mbox
		if (tocH)
			(*tocH)->imapMBH = mbox;
	}

	return (tocH);
}

/**********************************************************************
 * CleanHiddenCacheMailbox - clear out the hidden cache mailbox, 
 *	hopefully clearing it of any corrupted messages.
 **********************************************************************/
OSErr CleanHiddenCacheMailbox(TOCHandle hidTocH)
{
	OSErr err = noErr;
	FSSpec spec;
	short mNum;

	// something happened to our hidden cache mailbox.
	ASSERT(0);

	// was a toc specified?
	if (!hidTocH || !*hidTocH)
		return (paramErr);

	// is this really a hidden toc?
	spec = GetMailboxSpec(hidTocH, -1);
	if (EqualStrRes(spec.name, IMAP_HIDDEN_TOC_NAME)) {
		// close and remove all of its messages
		for (mNum = (*hidTocH)->count - 1; mNum >= 0; mNum--) {
			if ((*hidTocH)->sums[mNum].messH)
				CloseMyWindow(GetMyWindowWindowPtr
					      ((*(*hidTocH)->sums[mNum].
						messH)->win));
			DeleteIMAPSum(hidTocH, mNum);
		}

		// compact it to be sure to remove all leftovers
		CompactMailbox(&spec, false);
	}

	return (err);
}

/**********************************************************************
 * HideDeletedMessages - Hide/Show deleted messages in a mailbox.
 **********************************************************************/
OSErr HideDeletedMessages(MailboxNodeHandle mbox, Boolean bForce,
			  Boolean bShow)
{
	TOCHandle tocH, hidTocH;
	short sumNum;
	OSErr err = noErr;

	// find the local cache of deleted messages
	hidTocH = GetHiddenCacheMailbox(mbox, bForce, !bShow);

	// open the mailbox we're interested in if needed
	LockMailboxNodeHandle(mbox);
	tocH = TOCBySpec(&(*mbox)->mailboxSpec);
	UnlockMailboxNodeHandle(mbox);

	// if we're showing but this mailbox doesn't have a deleted cache,
	// there's nothing to do.
	if (bShow && !hidTocH)
		return (noErr);

	// if no toc was found for this mailbox, or if we couldn't create the
	// deleted toc, error.
	if (!tocH || !hidTocH)
		return (fnfErr);

	// don't let these tocs go away
	IMAPTocHBusy(tocH, true);
	IMAPTocHBusy(hidTocH, true);

	if (bShow) {
		// move all summaries from the deleted cache to the real mailbox
		for (sumNum = (*hidTocH)->count - 1;
		     (sumNum >= 0) && (err == noErr); --sumNum) {
			CycleBalls();
			err =
			    IMAPTransferLocalCache(hidTocH,
						   &((*hidTocH)->
						     sums[sumNum]), tocH,
						   (*hidTocH)->
						   sums[sumNum].uidHash,
						   false);
			DeleteIMAPSum(hidTocH, sumNum);
		}
	} else {
		// move all deleted messages from the mailbox to the deleted cache
		for (sumNum = (*tocH)->count - 1;
		     (sumNum >= 0) && (err == noErr); sumNum--) {
			if ((*tocH)->sums[sumNum].opts & OPT_DELETED) {
				CycleBalls();
				err =
				    IMAPTransferLocalCache(tocH,
							   &((*tocH)->
							     sums[sumNum]),
							   hidTocH,
							   (*tocH)->
							   sums[sumNum].
							   uidHash, false);
				DeleteIMAPSum(tocH, sumNum);
			}
		}
	}

	// if an error occurred writing to or reading from the hidden toc, recreate it.
	if (err != noErr)
		CleanHiddenCacheMailbox(hidTocH);

	// we're done with these tocs
	IMAPTocHBusy(tocH, false);
	IMAPTocHBusy(hidTocH, false);

	return (noErr);
}

/**********************************************************************
 * CountDeletedMessages - Count the number of deleted messages in a
 *	mailbox.  Consider the hidden toc as well, if appropriate.
 **********************************************************************/
short CountDeletedIMAPMessages(TOCHandle tocH)
{
	short count, sumNum;
	MailboxNodeHandle mbox = TOCToMbox(tocH);
	TOCHandle hidTocH;

	for (count = sumNum = 0; sumNum < (*tocH)->count; sumNum++)
		if ((*tocH)->sums[sumNum].opts & OPT_DELETED)
			count++;

	// do we need to consider the hidden toc as well?
	hidTocH = GetHiddenCacheMailbox(mbox, false, false);
	for (sumNum = 0; hidTocH && (sumNum < (*hidTocH)->count); sumNum++)
		if ((*hidTocH)->sums[sumNum].opts & OPT_DELETED)
			count++;

	return (count);
}

/**********************************************************************
 * HideShowSummary - make sure a summary is in the right toc.  If it's
 *	not, move it.  Return true if the summary is shown.
 **********************************************************************/
Boolean HideShowSummary(TOCHandle toc, TOCHandle tocH, TOCHandle hidTocH,
			short sumNum)
{
	Boolean bShown = false;
	Boolean bDeleted;
	OSErr err = noErr;

	// must have a source toc, a visible toc and a hidden toc
	if (toc && tocH && hidTocH) {
		// is this summary marked as deleted?
		bDeleted = ((*toc)->sums[sumNum].opts & OPT_DELETED) != 0;

		// is this a deleted summary in the visible toc?
		if (bDeleted && (toc == tocH)) {
			// move it to the hidden toc
			err =
			    IMAPTransferLocalCache(tocH,
						   &((*tocH)->
						     sums[sumNum]),
						   hidTocH,
						   (*tocH)->sums[sumNum].
						   uidHash, false);
			DeleteIMAPSum(tocH, sumNum);
		}
		// is this a non deleted summary in the hidden toc?
		else if (!bDeleted) {
			if (toc == hidTocH) {
				// move it to the visible toc
				err =
				    IMAPTransferLocalCache(hidTocH,
							   &((*hidTocH)->
							     sums[sumNum]),
							   tocH,
							   (*hidTocH)->
							   sums[sumNum].
							   uidHash, false);
				DeleteIMAPSum(hidTocH, sumNum);
			}
			bShown = true;
		}
		// else
		// the summary is in the right toc already.

		// was there a problem with the hidden toc?  Recreate it.
		if (err)
			CleanHiddenCacheMailbox(hidTocH);
	}
	return (bShown);
}

/**********************************************************************
 * ShowHideFilteredSummary - move a filtered IMAP summary to the proper
 *	toc.  Returns true if the summary was moved.
 **********************************************************************/
Boolean ShowHideFilteredSummary(TOCHandle toc, short sumNum)
{
	Boolean bMoved = false;
	MailboxNodeHandle mbox;
	TOCHandle tocH, hidTocH;
	short junkThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
	Boolean bIsJunk;
	TOCHandle junkTocH = LocateIMAPJunkToc(toc, false, true);

	if (toc && *toc) {
		mbox = TOCToMbox(toc);
		if (mbox) {
			// find the real toc ...
			LockMailboxNodeHandle(mbox);
			tocH = TOCBySpec(&(*mbox)->mailboxSpec);
			UnlockMailboxNodeHandle(mbox);

			// is there a hidden mailbox?
			if (DoesIMAPMailboxNeed(mbox, kShowDeleted))
				return (false);

			// locate the hidden toc as well ...
			hidTocH =
			    GetHiddenCacheMailbox(mbox, false, false);

			// skip junked messages
			bIsJunk = (junkTocH && HasFeature(featureJunk)
				   && ((*toc)->sums[sumNum].spamScore >=
				       junkThresh));

			if (toc == hidTocH) {
				// only move non-deleted, filtered, not junk messages to the visible toc.
				if ((((*hidTocH)->sums[sumNum].opts & OPT_DELETED) == 0)	// not deleted
				    && (((*hidTocH)->sums[sumNum].flags & FLAG_UNFILTERED) == 0)	// already filtered
				    && !bIsJunk)	// not about to be filtered to Junk
					bMoved =
					    HideShowSummary(toc, tocH,
							    hidTocH,
							    sumNum);
			} else {
				// move all appropriate messages to the invisible toc
				bMoved =
				    !HideShowSummary(toc, tocH, hidTocH,
						     sumNum);
			}
		}
	}
	return (bMoved);
}

/**********************************************************************
 * GetRealIMAPSpec - given a tocH, either hidden or visible, return
 *	the appropriate IMAP mailbox cache spec
 *
 *	Warning - this routine hits the disk.
 **********************************************************************/
MailboxNodeHandle GetRealIMAPSpec(FSSpec orig, FSSpecPtr spec)
{
	MailboxNodeHandle node = NULL;	// initialize
	TOCHandle tocH;
	PersHandle pers;
	CInfoPBRec pb;
	OSErr err;

	// initialize
	*spec = orig;

	// don't do anything if it doesn't have the right name
	if (EqualStrRes(orig.name, IMAP_HIDDEN_TOC_NAME)) {
		// first, see if this is already open
		tocH = FindTOC(&orig);
		if (tocH) {
			// it is!  This is easy.
			node = TOCToMbox(tocH);
			if (node)
				*spec = (*node)->mailboxSpec;
		} else {
			// nope, we're going to have to poke around in the file system to find out

			// The real IMAP cache folder has the same name as the enclosing folder
			// determine the name of the enclosing folder
			Zero(pb);
			pb.dirInfo.ioNamePtr = spec->name;
			pb.dirInfo.ioVRefNum = orig.vRefNum;
			pb.dirInfo.ioDrDirID = orig.parID;
			pb.dirInfo.ioFDirIndex = -1;	/* use ioDirID */
			err = PBGetCatInfoSync(&pb);
			if (!err) {
				// is this an IMAP mailbox file?                
				LocateNodeBySpecInAllPersTrees(spec, &node,
							       &pers);
			}
		}

		// did we fail to find the real TOC?
		if (node == NULL)
			spec->name[0] = 0;
	}

	return (node);
}

/**********************************************************************
 * EnsureSpecialMailboxes - make sure the user has selected
 *	a Junk and a Trash mailbox if required.
 **********************************************************************/
Boolean EnsureSpecialMailboxes(PersHandle pers)
{
	Boolean bGotBox;

	// if there's no mailbox tree at all, then we can't do any of this.
	// assume the caller is smart enough to know to get the mailbox tree
	if (!MailboxTreeGood(pers))
		return (true);

	PushPers(pers);
	CurPers = pers;

	// ask the user for the trash mailbox
	bGotBox = PrefIsSet(PREF_IMAP_NO_FANCY_TRASH)
	    || (GetIMAPTrashMailbox(CurPers, true, true) != NULL);
	if (!bGotBox) {
		// user has refused to pick a trash mailbox.  Offer to turn off FTM.
		if (ComposeStdAlert
		    (Stop, IMAP_MISSING_TRASH, (*pers)->name) == 2) {
			SetPref(PREF_IMAP_NO_FANCY_TRASH, YesStr);
			bGotBox = true;
		}
	}

	if (bGotBox) {
		// ask the user for the junk mailbox
		bGotBox = JunkPrefCantCreateJunk()
		    || JunkPrefIMAPNoRunPlugins()
		    || (GetIMAPJunkMailbox(CurPers, true, true) != NULL);
		if (!bGotBox) {
			// user has refused to pick a junk mailbox.  Offter to turn off Junk.
			if (ComposeStdAlert
			    (Stop, IMAP_MISSING_JUNK,
			     (*pers)->name) == 2) {
				SetPrefLong(PREF_JUNK_MAILBOX,
					    GetPrefLong(PREF_JUNK_MAILBOX)
					    | bJunkPrefIMAPNoRunPlugins);
				bGotBox = true;
			}
		}
	}

	PopPers();

	// warn the user about IMAP Auto-Expunge, if appropriate
	IMAPAutoExpungeWarning();

	return (bGotBox);
}

/**********************************************************************
 * IMAPAutoExpungeWarning - warn the user about autoexpunge.  
 *	Do this once, and ONLY if the user has an IMAP personality set
 *	up currently with the autoexpunge pref set.
 **********************************************************************/
void IMAPAutoExpungeWarning(void)
{
	PersHandle pers;
	Boolean bWarn = false;

	PushPers(CurPers);

	// have we warned before?
	CurPers = PersList;
	if (!IMAPAutoExpungeWarned()) {
		// Are there any IMAP personalities defined?
		for (pers = PersList; pers && !bWarn; pers = (*pers)->next) {
			CurPers = pers;
			bWarn = PrefIsSet(PREF_IS_IMAP)
			    && !IMAPAutoExpungeDisabled();
		}

		if (bWarn) {
			// disable auto-expunge 
			if (ComposeStdAlert
			    (Caution,
			     IMAP_AUTOEXPUNGE_WARNING) ==
			    kAlertStdAlertCancelButton) {
				for (pers = PersList; pers;
				     pers = (*pers)->next) {
					CurPers = pers;
					if (PrefIsSet(PREF_IS_IMAP)) {
						// reset auto-expunge preference
						SetPrefLong
						    (PREF_IMAP_AUTOEXPUNGE,
						     bIMAPAutoExpungeDisabled
						     |
						     bIMAPAutoExpungeNoThreshold);
					}
				}
			}
		}
		// either way, don't warn again
		CurPers = PersList;
		SetPrefLong(PREF_IMAP_AUTOEXPUNGE,
			    GetPrefLong(PREF_IMAP_AUTOEXPUNGE) |
			    bIMAPAutoExpungeWarned);
	}
	PopPers();
}

/**********************************************************************
 * IMAPAutoExpungeMailbox - check to see if this mailbox needs to be
 *	automatically expunged.
 **********************************************************************/
Boolean IMAPAutoExpungeMailbox(TOCHandle tocH)
{
	Boolean bNeeds = false;
	FSSpec spec;

	// are we quitting?
	if (AmQuitting)
		return (false);

	// are we offline?
	if (Offline)
		return (false);

	// is filtering or someother background thread currently underway?
	if (GetNumBackgroundThreads() || IMAPFilteringUnderway())
		return (false);

	// is this an IMAP toc?
	if (tocH && (*tocH)->imapTOC) {
		// make sure this isn't a hidden cache mailbox.  We should ignore those.
		spec = (*tocH)->mailbox.spec;
		if (!EqualStrRes(spec.name, IMAP_HIDDEN_TOC_NAME)) {
			// is the auto expunge preference set?
			PushPers(CurPers);
			if (!IMAPAutoExpungeDisabled()) {
				// then expunge it if the threshold has been reached
				bNeeds =
				    MarkOrExpungeMailboxIfNeeded(TOCToMbox
								 (tocH),
								 NULL,
								 true);
			}
			PopPers();
		}
	}

	return (bNeeds);
}

/**********************************************************************
 * IMAPAutoExpunge - periodic check to see if we need to perform any
 *	automatic IMAP mailboxe EXPUNGEs.  Return TRUE if we end up 
 *	starting an EXPUNGE command.
 **********************************************************************/
Boolean IMAPAutoExpunge(void)
{
	static long throttle = 0;
	Boolean bPerformed = false;
	FSSpec spec;
	FSSpecHandle mailboxes;
	OSErr err;

	// don't do anything if there's a background thread runing, 
	// if filtering is underway or if we're offline.
	if (GetNumBackgroundThreads() || IMAPFilteringUnderway()
	    || Offline)
		return (false);

	// do this once early on, then no more than about once an hour
	if ((throttle == 0) || ((TickCount() - throttle) > 60 * 60 * 60))	// once an hour
	{
		PushPers(nil);
		for (CurPers = PersList; CurPers;
		     CurPers = (*CurPers)->next) {
			// is this personality set to automatically expunge IMAP mailboxes?
			if (PrefIsSet(PREF_IS_IMAP)
			    && !IMAPAutoExpungeDisabled()) {
				// find the next mailbox to expunge ...
				LDRef(CurPers);
				if (GetNextMailboxToExpunge
				    ((*CurPers)->mailboxTree, &spec)) {
					mailboxes =
					    NuHandle(sizeof(FSSpec));
					if (mailboxes) {
						*((FSSpec *) (*mailboxes))
						    = spec;
						err =
						    IMAPProcessMailboxes
						    (mailboxes,
						     IMAPMultExpungeTask);

						// did we successfully start some EXPUNGEs?
						if (err == noErr)
							bPerformed = true;
						else
							ZapHandle
							    (mailboxes);
					}
				}
				UL(CurPers);
			}
			// let this expunge finish before we start another.
			if (bPerformed) {
				ComposeLogS(LOG_MOVE, nil,
					    "\pAuto-expunging mailbox %p",
					    spec.name);
				break;
			}
		}
		PopPers();

		// don't do this again for a while if we've handled all of the mailboxes
		if (!bPerformed)
			throttle = TickCount();
	}

	return (bPerformed);
}

/**********************************************************************
 * GetNextMailboxToExpunge - find the next mailbox for this personality
 *	that needs to be expunged
 **********************************************************************/
Boolean GetNextMailboxToExpunge(MailboxNodeHandle tree, FSSpec * spec)
{
	Boolean bFound = false;
	MailboxNodeHandle scan = tree;

	while (scan) {
		// does this mailbox need to be expunged?
		bFound = MarkOrExpungeMailboxIfNeeded(scan, spec, false);

		// Check the children
		if (!bFound) {
			LockMailboxNodeHandle(scan);
			if ((*scan)->childList)
				bFound =
				    GetNextMailboxToExpunge(((*scan)->
							     childList),
							    spec);
			UnlockMailboxNodeHandle(scan);
		}

		if (bFound)
			break;

		// go to the next node.
		scan = (*scan)->next;
	}

	return (bFound);
}

/**********************************************************************
 * ExpungeMailboxIfNeeded - expunge a mailbox if needed.  Return TRUE
 *	and the spec of the mailbox if expunge is needed.
 *
 *	ASSUMES CURPERS = IMAP PERS
 *	NOT THREAD SAFE
 **********************************************************************/
Boolean MarkOrExpungeMailboxIfNeeded(MailboxNodeHandle mbox, FSSpec * spec,
				     Boolean bNow)
{
	Boolean bNeeds = false;
	TOCHandle tocH;
	short numDeleted, numTotal;

	ASSERT(!InAThread());

	// might this mailbox need to be expunged?
	if (DoesIMAPMailboxNeed(mbox, kNeedsAutoExp)) {
		// does this mailbox really need to be expunged now?
		LockMailboxNodeHandle(mbox);
		tocH = TOCBySpec(&(*mbox)->mailboxSpec);
		UnlockMailboxNodeHandle(mbox);

		if (tocH) {
			// count the number of deleted messages
			numDeleted = CountDeletedIMAPMessages(tocH);

			// check threshold only if the pref is set to ...
			if (IMAPAutoExpungeAlways())
				bNeeds = (numDeleted != 0);
			else {
				// how many total messages are there?
				numTotal = (*tocH)->count;
				if (!DoesIMAPMailboxNeed
				    (mbox, kShowDeleted))
					numTotal += numDeleted;

				// are we above the threshold?
				if (numTotal
				    && ((numDeleted * 100) / numTotal >=
					GetRLong
					(IMAP_AUTOEXPUNGE_THRESHOLD)))
					bNeeds = true;
			}

			if (bNeeds) {
				// return the spec if requested ...
				if (spec)
					*spec = (*mbox)->mailboxSpec;

				// expunge right now if we ought to
				if (bNow) {
					IMAPRemoveDeletedMessages(tocH);
					SetIMAPMailboxNeeds(mbox,
							    kNeedsAutoExp,
							    false);
				}
			} else {
				// we're not at the threshold yet.  Ignore this mailbox until
				// another delete happens in it.
				SetIMAPMailboxNeeds(mbox, kNeedsAutoExp,
						    false);
			}
		}
	}

	return (bNeeds);
}

/**********************************************************************
 * MarkSumAsDeleted - mark an IMAP summary as deleted.  Set the auto
 *	expunge flag for the mailbox as well.
 **********************************************************************/
void MarkSumAsDeleted(TOCHandle tocH, short sumNum, Boolean bDeleted)
{
	if (bDeleted) {
		(*tocH)->sums[sumNum].opts |= OPT_DELETED;
		SetIMAPMailboxNeeds(TOCToMbox(tocH), kNeedsAutoExp, true);
	} else
		(*tocH)->sums[sumNum].opts &= ~OPT_DELETED;
}

 /**********************************************************************
 * IMAPPersIDChanged - called when the persID of an existing personality
 *	changes due to a rename, for example
 **********************************************************************/
void IMAPPersIDChanged(PersHandle pers, MailboxNodeHandle tree)
{
	while (pers && tree) {
		// change the pers id of this mailbox
		(*tree)->persId = (*pers)->persId;

		// do the children
		if ((*tree)->childList)
			IMAPPersIDChanged(pers, (*tree)->childList);

		// check the next node.
		tree = (*tree)->next;
	}
}
