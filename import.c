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

/* Copyright (c) 2000 by QUALCOMM Incorporated */
#define FILE_NUM 132

// routines to import mail from other applications

#include "import.h"

#pragma segment Importer

// these are the things OE can import
enum
{
	kISettings = 0,
	kIMessages,
	kIAddresses,
	kISignatures,
	kILimit
};

// these are the items in the importer selection dialog
enum
{
	kISOKButton = 1,
	kISCancelButton = 2,
	kISImporterPopup = 3
};

// these are the items in the import what dialog
enum
{
	kIWOKButton = 1,
	kIWCancelButton = 2,
	kIWSettingsCheckbox = 3,
	kIWMessagesCheckbox = 4,
	kIWAddressesCheckbox = 5,
	kIWSignaturesCheckbox = 6
};

/* Structure to hold data about what the user wants to import */
typedef struct
{
	Boolean importWhat[kILimit];
} ImportWhatStruct, *ImportWhatPtr;

#define MAX_MAILBOX_NAME_LENGTH 27
#define MAX_PROGRESS_LEN 33

static LineIOP 		Lip;
static long			gImportMsgEnd;
OSErr ImportErr;

/************************************************************************
 * Importer prototypes
 ************************************************************************/

// Open the importer window
static void OpenImportWindow(long pluginId, Boolean reopen);

// Is the import window open?
static Boolean IsImportWindowOpen(void);

// ask the user to select an importer plugin
Boolean ImportSelectPlugin(long *id);

// Import from the selected account
OSErr ImportAccount(ImportAccountInfoP account);

// Determine what that stuff is
Boolean ImportWhat(ImportAccountInfoP account, ImportWhatPtr what);

// Import mail from the selected account
OSErr ImportMail(ImportAccountInfoP account);

// import settings from an email account
OSErr ImportSettings(ImportAccountInfoP account);
Boolean PersDataIsGood(ImportPersDataP persD);
void AddNewPersonality(ImportPersDataP persD);

// import signatures from another email app
OSErr ImportSignatures(ImportAccountInfoP account);

// import contact information from another email app
OSErr ImportAddresses(ImportAccountInfoP account);
extern Boolean SaveIndNickFile(short which,Boolean saveChangeBits);

// read data from a file
OSErr ImportRecvLine(TransStream stream, UPtr buffer, long *size);

// Cleanup
void ZapImportablesHandle(ImportAccountInfoH *importables);

extern void GenAliasList(void);


/************************************************************************
 * DoMailImport - allow the user to select which application to import
 *	from, and display the import window.
 ************************************************************************/
void DoMailImport(void)
{
	long id = kUnspecifiedImportPluginId;
	
	// ask the user to select an applicaiton to import from
	if (ImportSelectPlugin(&id))
	{
		// display list of importable accounts
		OpenImportWindow(id, IsImportWindowOpen());
	}
}

/************************************************************************
 * ImportSelectPlugin - ask the user from what application to import
 ************************************************************************/
Boolean ImportSelectPlugin(long *id)
{
	MyWindowPtr	dgPtrWin;
	DialogPtr dgPtr;
	short item, i;
	extern ModalFilterUPP DlgFilterUPP;
	ControlHandle ip = nil;
	MenuHandle mh = nil;
	ImportAccountInfoH importables;
	short numImporters;
		
	*id = kUnspecifiedImportPluginId;
	
	//
	//	first, see what importers are around
	//
	
	// ask the importers what they can import ...
	importables = NuHandle(0L);
	if ((MemError()!=noErr) || !importables || !*importables)
	{
		ImportError(kUnspecifiedImportPluginId, kImportSetupErr, kImportMemError, MemError(), __LINE__);
		return(false);
	}
	ETLQueryImporters(&importables, kUnspecifiedImportPluginId, false);
	numImporters = GetHandleSize(importables)/sizeof(ImportAccountInfoS);
	if (numImporters <= 1)
	{
		// only one importer installed.  Use it.
		*id = 0;
		ZapImportablesHandle(&importables);
		return (true);
	}
		
	
	//
	// get the select dialog
	//
		
	if ((dgPtrWin = GetNewMyDialog(IMPORTER_SELECT_DLOG,nil,nil,InFront))==nil)
	{
		ImportError(kUnspecifiedImportPluginId, kImportSetupErr, kImportMemError, MemError(), __LINE__);
		return(false);
	}
	dgPtr = GetMyWindowDialogPtr(dgPtrWin);
	
				
	//
	// Build the list of importers
	//

	ip = GetDItemCtl (dgPtr, kISImporterPopup);	
	if (ip) mh = GetControlPopupMenuHandle (ip);
	if (!mh)
	{
		ImportError(kUnspecifiedImportPluginId, kImportSetupErr, kImportMemError, MemError(), __LINE__);
		return(false);
	}
	
	// add the things the importer can import to the popup menu ...
	i = GetHandleSize(importables)/sizeof(ImportAccountInfoS);
	while (i)
	{			
		MyInsMenuItem(mh, (*importables)[i-1].appName, 0);
		i--;			
	}
	
	// setup the popup menu control ...
	SetControlMaximum(ip, numImporters+1);
	SetControlMinimum(ip, 1);
	SetDItemState (dgPtr, kISImporterPopup, 1);
	
	
	//
	// Prepare the dialog for display
	//
			
	// Fix the controls
	SetPort(GetDialogPort(dgPtr));
	ConfigFontSetup(dgPtrWin);
	ReplaceAllControls(dgPtr);
	
	// Highlite the OK button
	HiliteButtonOne(dgPtr);
	
	// display it
	StartMovableModal(dgPtr);
	ShowWindow(GetDialogWindow(dgPtr));
	
	
	//
	//	Do the dialog
	//
	
	do
	{	
		MovableModalDialog(dgPtr,DlgFilterUPP,&item);
	}
	while ((item!=kIWOKButton) && (item!=kIWCancelButton));
	
	//	Remember what the user chose
	if (GetControlValue(ip) <= numImporters)
		*id = (*importables)[GetControlValue(ip) - 1].id;

	EndMovableModal(dgPtr);
	DisposeDialog(dgPtr);
	ZapImportablesHandle(&importables);

	return (item==kISOKButton);
}

/************************************************************************
 * ImportAccount - Import data from a mailstore
 ************************************************************************/
OSErr ImportAccount(ImportAccountInfoP account)
{
	OSErr err = noErr, importErr = noErr;
	ImportWhatStruct what;
	
	if (ImportWhat(account, &what))
	{
		// import settings
		if (what.importWhat[kISettings])
		{
			err = ImportSettings(account);
			if (importErr == noErr) importErr = err;
		}
	
		// import mail
		if ((err != memFullErr) && what.importWhat[kIMessages])
		{
			err = ImportMail(account);
			if (importErr == noErr) importErr = err;
		}
					
		// import addresses
		if ((err != memFullErr) && what.importWhat[kIAddresses])
		{
			err = ImportAddresses(account);
			if (importErr == noErr) importErr = err;
		}
				
		// import signatures
		if ((err != memFullErr) && what.importWhat[kISignatures])
		{
			err = ImportSignatures(account);
			if (importErr == noErr) importErr = err;
		}
					
		CloseProgress();
		
		// let the user know we finished importing
		if (importErr == noErr)
		{
			if (IsFreeMode() && (what.importWhat[kISettings] || what.importWhat[kISignatures]))
			{
				// we just imported some things the user won't have immediate access to.
				if(ComposeStdAlert(kAlertCautionAlert, LIGHT_IMPORT_COMPLETE)==1) NewClientModePlusOne = 1;
			}
			else 
			{
				// All is well with the world.
				ComposeStdAlert(kAlertNoteAlert, IMPORT_COMPLETE);
			}
		}
		else
		{
			// warn the user something went wrong during import.
			ComposeStdAlert(kAlertStopAlert, IMPORT_INCOMPLETE);
		}
	}
	return (importErr);
}

/************************************************************************
 * ImportWhat - ask the user what sorts of things to import
 ************************************************************************/
Boolean ImportWhat(ImportAccountInfoP account, ImportWhatPtr what)
{
	MyWindowPtr	dgPtrWin;
	DialogPtr dgPtr;
	short item;
	extern ModalFilterUPP DlgFilterUPP;

	Zero(*what);

	// get the what dialog	
	if ((dgPtrWin = GetNewMyDialog(IMPORT_WHAT_DLOG,nil,nil,InFront))==nil)
	{
		ImportError(kUnspecifiedImportPluginId, kImportSetupErr, kImportMemError, MemError(), __LINE__);
		return(false);
	}
	dgPtr = GetMyWindowDialogPtr (dgPtrWin);
		
	//fix the controls
	SetPort(GetDialogPort(dgPtr));
	ConfigFontSetup(dgPtrWin);
	ReplaceAllControls(dgPtr);
	
	// Highlite the OK button
	HiliteButtonOne(dgPtr);
	
	// Check the checkboxes
	for (item = kIWSettingsCheckbox; item <= kIWSignaturesCheckbox; item++)
		SetDItemState(dgPtr,item,!GetDItemState(dgPtr,item));	
	
	// display it
	MyParamText(account->accountName,"","","");
	StartMovableModal(dgPtr);
	ShowWindow(GetDialogWindow(dgPtr));
	
	do
	{
		MovableModalDialog(dgPtr,DlgFilterUPP,&item);
			
		if ((item>=kIWSettingsCheckbox) && (item<=kIWSignaturesCheckbox))
			SetDItemState(dgPtr,item,!GetDItemState(dgPtr,item));		
	}
	while ((item!=kIWOKButton) && (item!=kIWCancelButton));
	
	// remember what the user has checked
	what->importWhat[kISettings] = GetDItemState(dgPtr,kIWSettingsCheckbox);
	what->importWhat[kIMessages] = GetDItemState(dgPtr,kIWMessagesCheckbox);
	what->importWhat[kIAddresses] = GetDItemState(dgPtr,kIWAddressesCheckbox);
	what->importWhat[kISignatures] = GetDItemState(dgPtr,kIWSignaturesCheckbox);	
	
	EndMovableModal(dgPtr);
	DisposeDialog(dgPtr);
	
	return(item==kIWOKButton);
}

/************************************************************************
 * ImportMail - Import an account's mailboxes and messages
 ************************************************************************/
OSErr ImportMail(ImportAccountInfoP account)
{
	OSErr err = noErr;
	Str255 progress;
		
	// display some progress telling the user we're importing messages
	OpenProgress();
	ProgressMessageR(kpTitle,IMPORT_PROGRESS_TITLE);
	ComposeRString(progress, IMPORT_MESSAGE_PROGRESS_SUBTITLE, account->accountName);
	if (progress[0] > MAX_PROGRESS_LEN)
	{
		progress[0] = MAX_PROGRESS_LEN;
		progress[MAX_PROGRESS_LEN] = '�';
	}
	ProgressMessage(kpSubTitle,progress);
	
	err = ETLImportMail(account);
	if (err != noErr)
	{
		// an error occurred importing mail
		ImportError(account->id, kImportMessagesErr, kImportMessagesError, err, __LINE__);
	}
	return (err);	
}

/************************************************************************
 * ImportMakeMailboxCallback - callback that handles new mailbox creation
 ************************************************************************/
OSErr ImportMakeMailboxCallback(ImportMailboxOperationEnum command, FSSpecPtr boxSpec, Boolean isFolder, Boolean noSelect)
{
	OSErr err = noErr;
	FSSpec newFolder;
	TOCHandle tocH = nil;
	
	switch (command)
	{
		// importing mailboxes is about to begin
		case EMS_IMPORT_MAILBOX_Start:
		{
			// create a location for new mailboxes
			SimpleMakeFSSpec(MailRoot.vRef, MailRoot.dirId, boxSpec->name, boxSpec);
			UniqueSpec(boxSpec, MAX_MAILBOX_NAME_LENGTH);
			if (BadMailboxName(boxSpec, true)) err = dupFNErr;
			break;
		}
		
		case EMS_IMPORT_MAILBOX_Create_Mailbox:
		{
			// make sure the mailbox has a unique name
			UniqueSpec(boxSpec, MAX_MAILBOX_NAME_LENGTH);

			// are we creating a folder?
			if (isFolder)
			{
				// make the folder
				newFolder = *boxSpec;
				BadMailboxName(&newFolder, true);
			}
		
			// are we creating something that can hold messages?
			if (!noSelect)
			{
				// put it nside the folder if we create the folder, too.
				if (isFolder) boxSpec->parID = newFolder.parID;
				
				if (BadMailboxName(boxSpec, false))
					err = fnfErr;	
			}
				
			break;
		}
		
		// we're done importing messages into this mailbox
		case EMS_IMPORT_MAILBOX_Flush_Mailbox:
		{
			// Flush it.
			if (boxSpec)
			{
				tocH = FindTOC(boxSpec);
				if (tocH) BoxFClose(tocH,true);
			}
		}
		
		// mailbox creation is done
		case EMS_IMPORT_MAILBOX_Done:
		{
			// let the menus and mailboxes window know about it
			BuildBoxMenus();
			MBTickle(nil,nil);
			break;
		}
	
		default:
			break;
	}

		
	return (err);
}

/************************************************************************
 * CreateOutgoingMessage - create an outgoing message in the right place
 ************************************************************************/
OSErr ImportMakeOutMessCallback(emsMakeOutMessDataP o)
{
	OSErr err = noErr;
	FSSpec toSpec = o->boxSpec;
	MyWindowPtr win;		// the imported message
	MessHandle messH;
	Str255 html, scratch;
	char *scan;
	Boolean isHtml = false;
	short i;
	FSSpecPtr attach;
	TOCHandle tocH = nil;
	SignedByte state;
	short amount;
		
	//
	// Make a new outgoing message
	//
	
	if (win=DoComposeNew(nil))
	{
		messH = Win2MessH(win);
		
		// To:
		SetMessText(messH,TO_HEAD,LDRef(o->toAddresses),GetHandleSize(o->toAddresses)); 
		UL(o->toAddresses);
		
		// From:
		SetMessText(messH,FROM_HEAD,o->fromAddress+1,o->fromAddress[0]); 
		
		// Subject:
		SetMessText(messH,SUBJ_HEAD,o->subject+1,o->subject[0]); 

		// Cc:
		if (o->ccAddresses)
		{
			SetMessText(messH,CC_HEAD,LDRef(o->ccAddresses),GetHandleSize(o->ccAddresses)); 
			UL(o->ccAddresses);
		}
		
		// Bcc:
		if (o->bccAddresses)
		{
			SetMessText(messH,BCC_HEAD,LDRef(o->bccAddresses),GetHandleSize(o->bccAddresses)); 
			UL(o->bccAddresses);
		}
		
		// add the attachments
		for (i = 0; i < o->numAttachments; i++)
		{
			if (GetHandleSize(o->attachSpecs) >= (o->numAttachments*sizeof(FSSpec)))
			{
				attach = &((FSSpec *)*(o->attachSpecs))[i];
				CompAttachSpec(win, attach);
			}
		}
		
		// baa baa black sheep have you any wool?
		if (o->text && *(o->text) && GetHandleSize(o->text))
		{
			// Set the HTML flag if the first bit of the message contains the HTML string
			Zero(html);
			GetRString(html,HTMLTagsStrn+htmlTag);
			amount = MIN(255, GetHandleSize(o->text));
			BlockMove(*o->text, scratch, amount);
			
			scan = scratch;
			while (*scan && (scan <= (scratch+amount )))
			{
				if (*scan == '<')
				{
					if (!striscmp(scan+1, html+1))
					{					
						isHtml = true;
						TurnIntoEudoraHTML(&o->text);
						break;
					}
				}
				scan++;
			}
		}
				
		// The body of the message
		PETEInsertTextHandle(PETE,TheBody,PeteLen(TheBody),o->text,GetHandleSize(o->text),0,nil);

		// mark the message as sent if it's in the Sent Items folder
		if (o->isSent) 
			SetState((*messH)->tocH, (*messH)->sumNum, SENT);
		else
			SetState((*messH)->tocH, (*messH)->sumNum, UNSENT);
					
		//
		// transfer the message into the correct place
		//
		
		MoveMessage((*messH)->tocH, (*messH)->sumNum, &toSpec, true);
		DeleteSum((*messH)->tocH, (*messH)->sumNum);
		CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));

		//
		//	Polish the new message
		//
		
		tocH = TOCBySpec(&toSpec);
		if (tocH)
		{
			// set the html bit if we must
			if (isHtml) (*tocH)->sums[(*tocH)->count-1].opts |= OPT_HTML;
			
			// add the correct date, if there is one.
			if (o->date)
			{
				(*tocH)->sums[(*tocH)->count-1].seconds = o->date - ZoneSecs();
				state = HGetState(tocH);
				HSetState(tocH, state);
			}
		}
	}
	else
	{
		// failed to create a new comp window.  Memory is probably getting full.
		err = memFullErr;
	}	
	return (err);
}

/************************************************************************
 * ImportRecvLine - read a line at a time from a message database
 ************************************************************************/
OSErr ImportRecvLine(TransStream stream, UPtr buffer, long *size)
{
	static Boolean wasFrom;
	static Boolean wasNl=true;
	short lineType;
	
	if (!buffer) {Boolean retVal = wasFrom; wasFrom = false; return(retVal);}
	wasFrom=false;
	(*size)--;

	lineType = GetLine(buffer,*size,size,Lip);
	if (*buffer=='\012')
	{
		//	remove linefeed char
		BlockMoveData(buffer+1,buffer,*size-1);		
		(*size)--;
		buffer[*size] = 0;
	}
	if (!*size || !lineType || /*wasNl&&(wasFrom=IsFromLine(buffer)) ||*/ TellLine(Lip)>=gImportMsgEnd)
	{
		//	signal end-of-message
		*size = 2;
		buffer[0]='.'; buffer[1]='\015'; buffer[2]=0;
	}
	else if (lineType && wasNl && *buffer=='.')
	{
		//	insert '.' at beginning of line
		BlockMoveData(buffer,buffer+1,*size);
		(*size)++;
		*buffer = '.';
		buffer[*size] = 0;
	}
	wasNl = !lineType || buffer[*size-1]=='\015';
	return(noErr);
}

/**********************************************************************
 * ImportMessageCallback - Transfer raw MIME encoded messages 
 *	from a file
 **********************************************************************/
OSErr ImportMessageCallback(short ref, long offset, long len, short state, Handle attachments, FSSpecPtr boxSpec)
{
	OSErr err = noErr;	
	TransVector	saveCurTrans = CurTrans;
	static TransVector ImportTrans = {nil,nil,nil,nil,nil,nil,nil,nil,nil,ImportRecvLine,nil};
	FSSpec spec;
	TOCHandle toToc = nil;
	short count;
	long attachLength;
	
	ImportErr = noErr;
	
	//
	// Figure out where the message is going to go
	//
	
	toToc = TOCBySpec(boxSpec);
	if (toToc == nil)
	{
		ImportError(kUnspecifiedImportPluginId, kImportMessagesErr, kImportMessagesError, err=fnfErr, __LINE__);
		return (err);
	}
	
	//
	// Now read in the actual message
	//
	
	err = BoxFOpen(toToc);
	if (err == noErr)
	{
		GetFileByRef(ref, &spec);
		
		if (!Lip) Lip = NuPtrClear(sizeof(*Lip));
		if (!Lip)
		{
			err = MemError();
			ImportError(kUnspecifiedImportPluginId, kImportMessagesErr, kImportMemError, err, __LINE__);
		}
		else
		{
			if ((err=FSpOpenLine(&spec,fsRdPerm,Lip))==noErr)
			{
				PushPers(CurPers);
				
				// CurPers must be set to the owning personality
				CurPers = PersList;

				// now, grab messages
				CurTrans = ImportTrans;
				TOCSetDirty(toToc,true);
						
				BadBinHex = false;
				BadEncoding = 0;
				if (!AttachedFiles) AttachedFiles=NuHandle(0);
				SetHandleBig_(AttachedFiles,0);
					
				SeekLine(offset,Lip);
				gImportMsgEnd = offset+len;
				count=FetchMessageTextLo(NULL,len,nil,0,toToc,false,true);
				SetHandleBig_(AttachedFiles,0);
				SaveAbomination(nil,0);
				if (BadBinHex || BadEncoding) NoAttachments = true;
				else NoAttachments = false;
						
				// add on the attachment converted lines to the newly imported message				
				if (attachments && count)
				{
					for (count = 0; count < (GetHandleSize(attachments)/sizeof(FSSpec)); count++)
						RecordAttachment(&((FSSpecPtr)(*attachments))[count], nil);	
					
					attachLength = GetHandleSize(AttachedFiles);
					
					if (attachLength && ((err=WriteAttachNote((*toToc)->refN))==noErr))
					{
						// adjust the summary to reflect the new message length and the fact that there are attachments
						(*toToc)->sums[(*toToc)->count-1].length += attachLength;
						(*toToc)->sums[(*toToc)->count-1].flags |= FLAG_HAS_ATT;
					}
					
				}
									
				PopPers();
			}
			
			if (Lip && Lip->refN)
			{
				CloseLine(Lip);
				DisposePtr((Ptr)Lip);
				Lip = nil;
			}
			NoAttachments = false;		
			CurTrans = saveCurTrans;
			
			//
			// adjust the status of the message appropriately
			//
			
			(*toToc)->sums[(*toToc)->count-1].state = state;
		}
	}
	else
	{
		ImportError(kUnspecifiedImportPluginId, kImportMessagesErr, kImportMessagesError, err, __LINE__);
	}
	
	if (ImportErr) err = ImportErr;
	
	return (err);
}

/************************************************************************
 * ImportMboxCallback - Import an mbox'es worth of mail
 ************************************************************************/
OSErr ImportMboxCallback( FSSpecPtr sourceMBox, FSSpecPtr dest ) {
	OSErr		err = noErr;
	TOCHandle	destToc = nil;
	short		gotSome;
	
	destToc = TOCBySpec ( dest );
	if ( destToc == nil ) {
		ImportError ( kUnspecifiedImportPluginId, kImportMessagesErr, kImportMessagesError, err=fnfErr, __LINE__ );
		return err;
		}
	
	if ( noErr == ( err = BoxFOpen ( destToc ))) {
		err = GetUUPCMailSpecToMailBox ( sourceMBox, fsRdPerm, destToc, &gotSome );
		BoxFClose ( destToc, true );
		}
	
	return err;
	}


/************************************************************************
 * ImportSettings - Import an account's settings
 ************************************************************************/
OSErr ImportSettings(ImportAccountInfoP account)
{
	OSErr err = noErr;
	ImportPersDataH persData = nil;
	short i;
	Boolean added = false;
	
	err = ETLImportSettings(account, &persData);
	if ((err == noErr) && persData && *persData)
	{
		LDRef(persData);
		for (i=HandleCount(persData);i--;)
		{
			if (PersDataIsGood(&((*persData)[i])))
			{
				AddNewPersonality(&((*persData)[i]));
				added = true;
			}
		}
		UL(persData);
		
		// if we added a personality, whack the menus if we need to ...
		if (added) 
		{
			if (IMAPExists()) CreateNewPersCaches();
		}
		else
		{
			// warn user if no useful settings information was found
			ImportError(account->id, kImportSettingsErr, kImportNoSettingsError, err=eofErr, __LINE__);
		}
	}

	ZapHandle(persData);
	return (err);	
}

/************************************************************************
 * PersDataIsGood - can we make a personality out of this data?  Don't 
 *	bother creating a personality if these settings are incomplete.
 ************************************************************************/
Boolean PersDataIsGood(ImportPersDataP persD)
{
	return ((persD->accountName[0] && persD->userName[0] && persD->mailServer[0])			// here be enough server information
			|| (persD->accountName[0] && persD->realName[0] && persD->returnAddress[0]));	// here be enough user information
}

/************************************************************************
 * AddNewPersonality - given imported settings, create a new personality
 ************************************************************************/
void AddNewPersonality(ImportPersDataP persD)
{
	PersHandle pers = nil;
	Str255 scratch, iStr;
	Str31 name;
	short i;
	
	PushPers(CurPers);
	
	// use only the first 31 characters of the account name.
	persD->accountName[0] = MIN(persD->accountName[0], sizeof(Str31)-1);
	
	// Use the dominant personality?
	if (persD->makeDominant!=0)
	{
		// yes, but only if it's not yet filled in.
		CurPers = PersList;
		if (!*GetPOPPref(scratch))
			pers = CurPers;
	}
		
	// create a fresh personality
	if (pers == nil)
	{
		if (pers=PersNew())
		{								
			// pick a unique name for this new personality
			PCopy(name, persD->accountName);
			if (FindPersByName(name))
			{
				for (i = 1; i < 99; i++)
				{
					PCopy(scratch, name);
					iStr[0] = 0;
					PLCat(iStr, i);
					PCat(scratch, iStr);
					
					if (!FindPersByName(scratch))
					{
						PCopy(name, scratch);
						break;
					}
				}
			}
		}
		else
		{
			ImportError(kUnspecifiedImportPluginId, kImportSettingsErr, kImportMemError, MemError(), __LINE__);
		}
	}
		
	// set the settings
	if (pers != nil)
	{			
		// set the name of the personality if it's not the dominant
		if (pers != PersList) 
		{
			PersSetName(pers, name);
			PersSave(pers);
		}
		
		CurPers = pers;
		
		// set the imported settings
		SetPref(PREF_RETURN, persD->returnAddress);
		SetPref(PREF_REALNAME, persD->realName);
		
		SetPref(PREF_STUPID_USER, persD->userName);
		SetPref(PREF_STUPID_HOST, persD->mailServer);
		SetStupidStuff();
		
		SetPref(PREF_SMTP, persD->smtpServer);
		
		// turn on IMAP if necessary
		if (persD->isIMAP) SetPref(PREF_IS_IMAP, YesStr);
		else SetPref(PREF_IS_IMAP, NoStr);
			
		// tell the personalities window about the new account
		NotifyPersonalitiesWin();
	}
	
	PopPers();
}

/************************************************************************
 * ImportSignatures - Import an account's signatures
 ************************************************************************/
OSErr ImportSignatures(ImportAccountInfoP account)
{
	OSErr err = noErr;

	err = ETLImportSignatures(account);
	if (err != noErr)
	{
		ImportError(account->id, kImportSignaturesErr, kImportSignaturesError, err, __LINE__);
	}
	
	return (err);
}


/************************************************************************
 * ImportMakeNewSigCalback - given a name and some text, make a signature file
 ************************************************************************/
short ImportMakeNewSigCalback(Str255 name, Handle sigText)
{
	short err = noErr;
	FSSpec spec,folderSpec;
	long sigSize = GetHandleSize(sigText);
	Str255 scratch;
	
	if (sigSize)
	{
		// find the signature folder
		if (noErr != ( err = SubFolderSpec(SIG_FOLDER,&folderSpec))) return err;
		
		//	Make a uniquely named signature file
		name[0] = MIN(name[0], 31);
		SimpleMakeFSSpec(folderSpec.vRefNum,folderSpec.parID,name,&spec);
		UniqueSpec(&spec,31);
		
		// special case for signatures with the same name as the Standard sig
		if (StringSame(GetRString(scratch, FILE_ALIAS_STANDARD), spec.name))
		{
			// append a digit onto the end of the signature name
			PLCat(spec.name,1);
			
			// and make sure it's unique
			UniqueSpec(&spec,31);
		}
		
		if ((err=FSpCreate(&spec,CREATOR,'TEXT',smSystemScript))==noErr)
		{	
			// write the converted text out to a file
			LDRef(sigText);
			err = Blat(&spec, sigText, false);
			UL(sigText);
		}
		else
		{
			FileSystemError(CREATE_SIG,&spec.name,err);
		}

		BuildSigList();	
	}
		
	return (err);
}

/************************************************************************
 * ImportAddresses - Import an account's address book
 ************************************************************************/
OSErr ImportAddresses(ImportAccountInfoP account)
{
	OSErr err = noErr;
	
	err = ETLImportAddresses(account);
	if (err != noErr)
	{
		ImportError(account->id, kImportAddressesErr, kImportAddressesError, err, __LINE__);
	}
	
	// re-read the nickname files and update the address book if it's open.
	if (!AliasWinIsOpen()) GenAliasList();
	RegenerateAllAliases(true);
	ABTickleHardEnoughToMakeYouPuke();

	return (err);
}

/************************************************************************
 * ImportMakeAddressBookCallback - make an address book file
 ************************************************************************/
short ImportMakeAddressBookCallback(Str255 name)
{
	short which = -1;
	AliasDesc ad;
	FSSpec folderSpec;
	Str255 scratch;
	long dirID;
	OSErr err = noErr;
	
	// if we're in free mode, add nicknames to the existing Eudora Nicknames file.
	if (IsFreeMode())
	{
		return (0);
	}
	
	Zero(ad);
	
	// The name of the new nick file will be the name passed in
	PCopy(ad.spec.name, name);
	ad.spec.name[0] = MIN(ad.spec.name[0], MAX_MAILBOX_NAME_LENGTH);
	
	// check to see if a nickname file already exists there
	if (!SubFolderSpec(NICK_FOLDER,&folderSpec))
	{
		SimpleMakeFSSpec(folderSpec.vRefNum,folderSpec.parID,ad.spec.name,&ad.spec);
		UniqueSpec(&ad.spec, 31);
	}
	else
	{
		SimpleMakeFSSpec(Root.vRef,Root.dirId,GetRString(scratch,NICK_FOLDER),&folderSpec);
		FSpDirCreate(&folderSpec,smSystemScript,&dirID);
		SubFolderSpec(NICK_FOLDER,&folderSpec);
		SimpleMakeFSSpec(folderSpec.vRefNum,folderSpec.parID,ad.spec.name,&ad.spec);
	}
	
	// make the new nickname file.
	if (NickWinIsOpen()) 
	{
		which = NickMakeNewFile(ad.spec.name);
	}
	else
	{
		err = MakeResFile(ad.spec.name,ad.spec.vRefNum,ad.spec.parID,CREATOR,'TEXT');
		if (err == noErr) 
		{
			if (PtrPlusHand_(&ad,Aliases,sizeof(ad))) err = MemError();
			which = HandleCount (Aliases) - 1;
		}	
	}
	
	if ((err != noErr) || !which)
		ImportError(kUnspecifiedImportPluginId, kImportAddressesErr, kCreateAddressbookError, err, __LINE__);

	return (which);
}

/************************************************************************
 * ImportMakeABEntryCallback - make an address book entry
 ************************************************************************/
OSErr ImportMakeABEntryCallback(short which, Boolean isGroup, UPtr nickName, Handle addresses, Handle notes)
{
	OSErr err = noErr;
	Str255 name;
	Handle mungedAddresses = nil;
	
	// special case call to save address book
	if (!nickName && !addresses)
	{
		SaveIndNickFile(which, true);
		return (noErr);
	}
	
	BeautifyFrom(nickName);
	SanitizeFN(name,nickName,NICK_BAD_CHAR,NICK_REP_CHAR,false);
	
	if (addresses)
	{	
		if ((err=SuckAddresses(&mungedAddresses,addresses,true,false,false,nil))==noErr)
		{		
			// Add this nickname.  	If we're adding to an existing nickname file, warn about duplicates
			err = NewNickLow(mungedAddresses, notes, which, name, true, IsFreeMode()?nrDifferent:nrAdd, false);
		}
	}
	else
	{
		// Addresses passed in was nil.  Just convert the nickname
		PCopy(nickName, name);
	}
	
	return (err);
}

/************************************************************************
 * ZapImportablesHandle - clean up an ImportAccountInfoH
 ************************************************************************/
void ZapImportablesHandle(ImportAccountInfoH *importables)
{
	short i = GetHandleSize(*importables)/sizeof(ImportAccountInfoS);
	
	while (i)
	{			
		ZapHandle((**importables)[i-1].icon);
		i--;			
	}
	
	ZapHandle(*importables);
	*importables = nil;
}
	
/************************************************************************
 * ImportError - report an importer error
 ************************************************************************/
OSErr ImportError(long id, short importOperation, short importError, OSErr err, short line)
{
	Str255 message;
	Str255 importMessage;
	Str63 debugStr;
	Str31 rawNumber;
	Str255 importerError;
	
	// get the name of the importer that has reported the error
	importerError[0] = 0;
	if (id >= 0)
	{
		GetImporterName(id, importerError);
		PCat(importerError,"\p:");
	}
	
	importMessage[0] = 0;

	NumToString(err,rawNumber);
	PCat(importerError,rawNumber);
	GetRString(message,importOperation+ImportOperationsStrn);

	// pick a useful error message
	switch (err)
	{
		case memFullErr:
			GetRString(importMessage,ImportErrorsStrn+kImportMemError);
			break;
			
		default:
			GetRString(importMessage,importError+ImportErrorsStrn);
			break;
	}
	
	ComposeRString(debugStr,FILE_LINE_FMT,FNAME_STRN+FILE_NUM,line);
	MyParamText(message,importerError,importMessage,debugStr);
	ReallyDoAnAlert(BIG_OK_ALRT,Caution);
	
	return (err);
}

/************************************************************************
 * Import Window data structures and definitions
 ************************************************************************/
 
#define NUM_COLUMNS 3

#define THUMB_COLUMN 0
#define ACCOUNT_COLUMN 1
#define PROGRAM_COLUMN 2

#define MINI_BANNER_WIDTH 50
#define ACCOUNT_NAME_WIDTH 225

// Import window controls
enum
{
	kctlColumns1=0,
	kctlColumns2,
	kctlColumns3,
	kctlImport,
	kctlSelect,
	kctlOther
};

#define NUM_CONTROLS 3

typedef struct
{
	ViewList list;
	MyWindowPtr win;
	ControlHandle ctlImport, ctlSelect, ctlOther;
	Boolean	inited;
	short columnLefts[NUM_COLUMNS];
	ImportAccountInfoH Importables;
	long pluginId;
} WinData;
static WinData gWin;

 
/************************************************************************
 * Import Window prototypes
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR);
static void DoZoomSize(MyWindowPtr win,Rect *zoom);
static void DoUpdate(MyWindowPtr win);
static Boolean DoClose(MyWindowPtr win);
static void DoClick(MyWindowPtr win,EventRecord *event);
static void DoCursor(Point mouse);
static void DoActivate(MyWindowPtr win);
static Boolean DoKey(MyWindowPtr win, EventRecord *event);
static OSErr DoDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
static void DoShowHelp(MyWindowPtr win,Point mouse);
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers);
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message, long data);
static Boolean GetListData(VLNodeInfo *data,short selectedItem);
static void SetControls(void);
static void DoGrow(MyWindowPtr win,Point *newSize);
static Boolean ImportDrawRowCallBack(ViewListPtr pView,short item,Rect *pRect,CellRec *pCellData,Boolean select,Boolean eraseName);
static Boolean ImportFillBlankCallback(ViewListPtr pView);
static void ImportSelectedAccounts(void);
static void ManualImport(void);
static void GetCellRectsForImportWin(ViewListPtr pView,CellRec *pCellData,Rect *pRect,Rect *pIcon,Rect *pName,Rect *pDate,Point *pNamePt,Point *pDatePt);
static short ColumnRight(short column, short max);
static Boolean ImportGetCellRectsCallBack(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect);
static Boolean ImportInterestingClickCallBack(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Point pt);
static void SetImportWinBackgroundColor(void);
static Boolean ImportFind(MyWindowPtr win,PStr what);

static void PopuplateImportList(void);
static OSErr BuildListOfImportables(ImportAccountInfoH *importables);
static void SortImportablesHandle(ImportAccountInfoH toSort);
static int ImportAppCompare(ImportAccountInfoP i1, ImportAccountInfoP i2);
static int ImportAccountCompare(ImportAccountInfoP i1, ImportAccountInfoP i2);
static void SwapImportable(ImportAccountInfoP i1, ImportAccountInfoP i2);

/************************************************************************
 * OpenImportWindow - open the import window
 ************************************************************************/
static void OpenImportWindow(long pluginId, Boolean reopen)
{
	short err=0;
	Rect r;
	DrawDetailsStruct details;
	WindowPtr gWinWinPtr;
	ImportAccountInfoH importables;
	
	if (!gWin.inited || reopen)
	{
		// which plugin will be used for importing?
		gWin.pluginId = pluginId;
		
		// see if we can find anything to import
		importables = nil;
		err = BuildListOfImportables(&importables);
		if ((err != noErr) || (importables==nil))
		{
			// let the user select something by hand to import if we're using a specific importer plugin ...
			if (gWin.pluginId != kUnspecifiedImportPluginId)
			{
				if (ComposeStdAlert(Note,IMPORT_MESSAGE_BY_HAND)==1)
				{
					// manual import ...
					ManualImport();
				}
			}
			else
			{
				// Otherwise, the import failed.
				if (ComposeStdAlert(Note,IMPORT_MESSAGE_TRY_AGAIN)==1)
				{
					DoMailImport();
				}
			}
			
			// we're done
			return;
		}
		
		// if we're reopening, destroy the old list and importables
		if (reopen)
		{
			if (gWin.inited)
			{
				LVDispose(&gWin.list);
				ZapImportablesHandle(&(gWin.Importables));
			}
		}
	
		gWin.Importables = importables;	
	
		if (!gWin.inited)
		{	
			/*
			 * the window
		 	 */
		 
			if (!(gWin.win=GetNewMyWindow(IMPORTER_WIND,nil,nil,BehindModal,false,false,IMPORTER_WIN)))
				{err=MemError(); goto fail;}
			gWinWinPtr = GetMyWindowWindowPtr (gWin.win);
			SetWinMinSize(gWin.win,-1,-1);
			SetPort_(GetMyWindowCGrafPtr(gWin.win));
			ConfigFontSetup(gWin.win);	
			MySetThemeWindowBackground(gWin.win,kThemeListViewBackgroundBrush,False);
			
			
			/*
			 * controls
			 */
			 
			if (!(gWin.ctlImport = NewIconButton(IMPORT_IMPORT_CNTL,gWinWinPtr)) ||
				!(gWin.ctlSelect = NewIconButton(IMPORT_SELECT_CNTL,gWinWinPtr)) ||
				!(gWin.ctlOther = NewIconButton(IMPORT_OTHER_CNTL,gWinWinPtr)))
					goto fail;


			/*
			 * columns
			 */
			 
			gWin.columnLefts[THUMB_COLUMN] = 0;
			gWin.columnLefts[ACCOUNT_COLUMN] = MINI_BANNER_WIDTH+2;
			gWin.columnLefts[PROGRAM_COLUMN] = gWin.columnLefts[THUMB_COLUMN] + ACCOUNT_NAME_WIDTH;
			
			
			/*
			 * callbacks
			 */
			 		
			gWin.win->didResize = DoDidResize;
			gWin.win->close = DoClose;
			gWin.win->update = DoUpdate;
			gWin.win->position = PositionPrefsTitle;
			gWin.win->click = DoClick;
			gWin.win->bgClick = DoClick;
			gWin.win->dontControl = true;
			gWin.win->cursor = DoCursor;
			gWin.win->activate = DoActivate;
			gWin.win->help = DoShowHelp;
			gWin.win->menu = DoMenuSelect;
			gWin.win->key = DoKey;
			gWin.win->app1 = DoKey;
			gWin.win->drag = DoDragHandler;
			gWin.win->zoomSize = DoZoomSize;
			gWin.win->grow = DoGrow;
			gWin.win->idle = nil;
			gWin.win->find = ImportFind;
			gWin.win->showsSponsorAd = true;
		}
		
		
		/*
		 * the list
		 */
		
		details.arrowLeft = 2;
		details.iconTop = 2;
		details.iconLeft = details.arrowLeft;	//	First icon level
		details.iconLevelWidth = 0;
		details.textBottom = 5;
		details.rowHt = 36;
		details.nameAddMargin = 5;					//	Add to name width when drawing
		details.maxNameWidth = ACCOUNT_NAME_WIDTH;	//	Approximate maximum name width
		details.keyNavTicks = 60;					//	Delay accepted between navigation keys
		
		// callbacks for list drawing
		details.DrawRowCallback = ImportDrawRowCallBack;
		details.FillBlankCallback = ImportFillBlankCallback;
		details.GetCellRectsCallBack = ImportGetCellRectsCallBack;
		details.InterestingClickCallback = ImportInterestingClickCallBack;
		
		SetRect(&r,0,0,20,20);	
		if (LVNewWithDetails(&gWin.list,gWin.win,&r,1,ViewListCallBack,flavorTypeText,&details))
			{err=MemError(); goto fail;}
			
			
		/*
		 * show the window
		 */
		MyWindowDidResize(gWin.win,&gWin.win->contR);	
		ShowMyWindow(gWinWinPtr);
		gWin.inited = true;
		UserSelectWindow(GetMyWindowWindowPtr(gWin.win));
		return;
		
		fail:
			if (gWin.win) CloseMyWindow(GetMyWindowWindowPtr(gWin.win));
			if (err) WarnUser(COULDNT_WIN,err);
			return;
	}
	UserSelectWindow(GetMyWindowWindowPtr(gWin.win));
}

/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR)
{
#define kListInset 10 
#pragma unused(oldContR)
	ControlHandle	ctlList[NUM_CONTROLS];
	Rect r;
	short htAdjustment;
	
	//	buttons
	ctlList[0] = gWin.ctlImport;
	ctlList[1] = gWin.ctlSelect;
	ctlList[2] = gWin.ctlOther;
	PositionBevelButtons(win,NUM_CONTROLS,ctlList,kListInset,gWin.win->contR.bottom-INSET-kHtCtl,kHtCtl,RectWi(win->contR));

	// list
	SetRect(&r,kListInset,win->topMargin+kListInset,gWin.win->contR.right-kListInset,gWin.win->contR.bottom - 2*INSET - kHtCtl);
	if (gWin.win->sponsorAdExists && r.bottom >= gWin.win->sponsorAdRect.top - kSponsorBorderMargin)
		r.bottom = gWin.win->sponsorAdRect.top - kSponsorBorderMargin;
	LVSize(&gWin.list,&r,&htAdjustment);
	
	//	enable/disble controls
	SetControls();
		
	// redraw
	InvalContent(win);
}

/************************************************************************
 * SetControls - enable or disable the controls, based on current situation
 ************************************************************************/
static void SetControls(void)
{
	short count = LVCountSelection(&gWin.list);
	Boolean fSelect = count!=0;

	// enable/disable buttons
	SetGreyControl(gWin.ctlImport,!fSelect);	
	SetGreyControl(gWin.ctlSelect,(gWin.pluginId==kUnspecifiedImportPluginId));	
	SetGreyControl(gWin.ctlOther,false);	
}

/************************************************************************
 * DoZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void DoZoomSize(MyWindowPtr win,Rect *zoom)
{
	short zoomHi = zoom->bottom-zoom->top;
	short zoomWi = zoom->right-zoom->left;
	short hi, wi;
	
	LVMaxSize(&gWin.list, &wi, &hi);
	wi += 2*kListInset;
	hi += 2*kListInset + INSET + kHtCtl;

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;
}

/************************************************************************
 * DoGrow - adjust grow size
 ************************************************************************/
static void DoGrow(MyWindowPtr win,Point *newSize)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect	r;
	short	htControl = ControlHi(gWin.ctlImport);
	short	bottomMargin,sponsorMargin;
	
	//	Get list position
	bottomMargin = INSET*2 + htControl;
	if (win->sponsorAdExists)
	{
		Rect	rPort;
		
		GetPortBounds(GetMyWindowCGrafPtr(win),&rPort);
		sponsorMargin = rPort.bottom - win->sponsorAdRect.top + kSponsorBorderMargin;
		if (sponsorMargin > bottomMargin) bottomMargin  = sponsorMargin;
	}
	SetRect(&r,kListInset,win->topMargin+kListInset,newSize->h-kListInset,newSize->v - bottomMargin);
	LVCalcSize(&gWin.list,&r);
	
	//	Calculate new window height
	newSize->v = r.bottom + bottomMargin;
}

/************************************************************************
 * DoClose - close the window
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win)
{
#pragma unused(win)
	
	if (gWin.inited)
	{
		//	Dispose of list
		LVDispose(&gWin.list);
		gWin.inited = false;
		
		// dispose of importables list
		ZapImportablesHandle(&(gWin.Importables));
	}
	return(True);
}

/************************************************************************
 * IsImportWindowOpen - is the import window open?
 ************************************************************************/
static Boolean IsImportWindowOpen(void)
{
	return (gWin.inited);
}

/************************************************************************
 * DoUpdate - draw the window
 ************************************************************************/
static void DoUpdate(MyWindowPtr win)
{
	Rect r;
	
	// Tweak the import window colors
	SetImportWinBackgroundColor();
	
	// draw the list
	r = gWin.list.bounds;
	DrawThemeListBoxFrame(&r,kThemeStateActive);
	LVDraw(&gWin.list, MyGetPortVisibleRegion(GetMyWindowCGrafPtr(win)), true, false);
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate(MyWindowPtr win)
{
#pragma unused(win)
	LVActivate(&gWin.list, gWin.win->isActive);
	SetControls();
}

/************************************************************************
 * DoKey - key stroke
 ************************************************************************/
static Boolean DoKey(MyWindowPtr win, EventRecord *event)
{
#pragma unused(win)
	short key = (event->message & 0xff);

	if (LVKey(&gWin.list,event))
	{
		SetControls();
		return true;
	}

	return false;
}

/**********************************************************************
 * ImportFind - find in the window
 **********************************************************************/
static Boolean ImportFind(MyWindowPtr win,PStr what)
{
	return FindListView(win,&gWin.list,what);
}

/************************************************************************
 * DoClick - click in window
 ************************************************************************/
static void DoClick(MyWindowPtr win,EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point pt;
	ControlHandle	hCtl;
	
	SetPort(GetMyWindowCGrafPtr(win));

	if (!LVClick(&gWin.list,event))
	{
		pt = event->where;
		GlobalToLocal(&pt);
		if (!win->isActive)
		{
			SelectWindow_(winWP);
			UpdateMyWindow(winWP);	//	Have to update manually since no events are processed
		}
		
		if (FindControl(pt, winWP, &hCtl))
		{
			if (TrackControl(hCtl,pt,(void *)(-1)))
			{
				if (hCtl == gWin.ctlImport)
				{
					ImportSelectedAccounts();
					AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),kctlImport), event->what);
				}
				else if (hCtl == gWin.ctlSelect)
				{
					ManualImport();
					AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),kctlSelect), event->what);
				}
				else if (hCtl == gWin.ctlOther)
				{
 					DoMailImport();
					AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),kctlOther), event->what);
				}
			}
		}
	}
	SetControls();
}
					
/************************************************************************
 * DoCursor - set the cursor properly for the window
 ************************************************************************/
static void DoCursor(Point mouse)
{
	if (!PeteCursorList(gWin.win->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * DoShowHelp - provide help for the window
 ************************************************************************/
static void DoShowHelp(MyWindowPtr win,Point mouse)
{
	if (PtInRect(mouse,&gWin.list.bounds))
		MyBalloon(&gWin.list.bounds,100,0,IMPORT_HELP_STRN+1,0,nil);
	else
		ShowControlHelp(mouse,IMPORT_HELP_STRN+2,gWin.ctlImport,gWin.ctlSelect,gWin.ctlOther,nil);
}

/************************************************************************
 * GetListData - get data for selected item
 ************************************************************************/
static Boolean GetListData(VLNodeInfo *data,short selectedItem)
{
	return LVGetItem(&gWin.list,selectedItem,data,true);
}

/************************************************************************
 * DoMenuSelect - menu choice in the window
 ************************************************************************/
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(win,modifiers)
	
	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_OPENSEL_ITEM:
					ImportSelectedAccounts();
					return(True);
					break;
			}
			break;

		case EDIT_MENU:
			if (item==EDIT_SELECT_ITEM)
			{
				if (LVSelectAll(&gWin.list))
				{
					SetControls();
					return(true);
				}
			}
			break;
	}
	return(False);
}

/**********************************************************************
 * DoDragHandler - handle drags
 **********************************************************************/
static OSErr DoDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{	
#pragma unused(win)
#pragma unused(which)
#pragma unused(drag)

	return(dragNotAcceptedErr);	//	Nothing here we want
}

/************************************************************************
 * ImportFillBlankCallback - callback function for List View.  Fill unused 
 *	parts of the list.
 ************************************************************************/
static Boolean ImportFillBlankCallback(ViewListPtr pView)
{
	ListHandle	hList=pView->hList;
	Rect	rErase;
	GrafPtr	savePort;
	short column;
	Rect columnRect;
	
	if ((*hList)->visible.bottom > (*hList)->dataBounds.bottom)
	{
		SAVE_STUFF;

		SET_COLORS;
		GetPort(&savePort);
		SetPort(GetMyWindowCGrafPtr(pView->wPtr));
		Cell1Rect(hList,(*hList)->dataBounds.bottom+1,&rErase);
		rErase.bottom = pView->bounds.bottom;

		if (UseListColors)
		{					
			for (column = 0; column < NUM_COLUMNS; column++)
			{	
				// what is the rect describing this column?
				SetRect(&columnRect,rErase.left+gWin.columnLefts[column],rErase.top,rErase.left+ColumnRight(column,rErase.right),rErase.bottom);
				
				// color this column
				SetThemeBackground(column==PROGRAM_COLUMN ? kThemeBrushListViewSortColumnBackground:kThemeBrushListViewBackground,RectDepth(&columnRect),true);
				EraseRect(&columnRect);
			}
		}
			
		SetPort(savePort);
		REST_STUFF;
	} 
	return true;	/* mtc sez - this value is never checked 11-18-03 */
}

/************************************************************************
 * ColumnRight - return the right side of a column
 ************************************************************************/
static short ColumnRight(short column, short max)
{
	short right = 0;
	
	if ((column>=0) && (column<NUM_COLUMNS))
	{
		if (column < NUM_COLUMNS - 1) 
			right = gWin.columnLefts[column+1];
		else 
			right = max;
	}
	
	return (right);
}

/************************************************************************
 * ImportDrawRowCallBack - callback function for List View.  Draw a row.
 ************************************************************************/
static Boolean ImportDrawRowCallBack(ViewListPtr pView,short item,Rect *pRect,CellRec *pCellData,Boolean select,Boolean eraseName)
{
	Boolean result = true;
	Rect rIcon, rName, rApplication;
	Point ptName, ptApplication;
	Boolean hasColor = IsColorWin(GetMyWindowWindowPtr(pView->wPtr));
	Str255 scratch;
	short column;
	Rect columnRect;
	Handle theIcon;
	
	if (item && item <= (*pView->hList)->dataBounds.bottom)
	{	
		SAVE_STUFF;
		
		//
		// Figure out where things are in this row
		//
		
		GetCellRectsForImportWin(pView,pCellData,pRect,&rIcon,&rName,&rApplication,&ptName,&ptApplication);


		//
		//	Draw the list background color and seperator lines for this row
		//
		
		if (UseListColors)
		{				
			for (column = 0; column < NUM_COLUMNS; column++)
			{	
				// what is the rect describing this column?
				SetRect(&columnRect,pRect->left+gWin.columnLefts[column],pRect->top,pRect->left+ColumnRight(column,pRect->right),pRect->top + (*pView->hList)->cellSize.v - 1);
				
				// color this column
				if (column==PROGRAM_COLUMN)
					SetThemeBackground(kThemeBrushListViewSortColumnBackground,RectDepth(&columnRect),true);
				else
					SetThemeBackground(kThemeBrushListViewBackground,RectDepth(&columnRect),true);
				EraseRect(&columnRect);
			}
			
			SET_COLORS;
		}
	
		//
		// 	Draw the contents of this item
		//
		
		SET_COLORS;
		
		//	Plot the icon
		theIcon = GetImporterAppIcon(pCellData->iconID);
		if (theIcon) PlotIconSuite(&rIcon, atNone + atHorizontalCenter, select ? ttSelected : ttNone, theIcon);
		else PlotIconID(&rIcon,atNone + atHorizontalCenter,select ? ttSelected : ttNone,IMPORT_IMPORT_CNTL);
		
		//	Draw the account name
		MoveTo(ptName.h,ptName.v);
		if (eraseName)
			EraseRect(&rName);
		if (pCellData->misc.style)
			TextFace(pCellData->misc.style);

		TextFont(pView->font);
		TextSize(pView->fontSize);

		DrawString(pCellData->name);
		if (pCellData->misc.style)
			TextFace(0);

		if (select)
			InvertRect(&rName);

		// Draw the application name
		PCopy(scratch, (*(gWin.Importables))[item-1].appName);
		
		MoveTo(ptApplication.h,ptApplication.v);
		if (eraseName)
			EraseRect(&rApplication);
		if (pCellData->misc.style)
			TextFace(pCellData->misc.style);

			TextFont(pView->font);
			TextSize(pView->fontSize);

			DrawString(scratch);
			if (pCellData->misc.style)
				TextFace(0);
							
		REST_STUFF;
	}
	
	return (result);
}

/**********************************************************************
 * GetCellRects - get triangle, icon, and name rects
 **********************************************************************/
static void GetCellRectsForImportWin(ViewListPtr pView,CellRec *pCellData,Rect *pRect,Rect *pIcon,Rect *pName,Rect *pDate,Point *pNamePt,Point *pDatePt)
{
	short	offset;
	Point	pt;
	FontInfo	fInfo;
	short column;
	
	SAVE_STUFF;
	
	for (column = 0; column < NUM_COLUMNS; column++)
	{
		switch (column)
		{
			case THUMB_COLUMN:
				if (column < NUM_COLUMNS - 1)
				{
					// center of column, minus half of an icon
					offset = (gWin.columnLefts[column+1] - gWin.columnLefts[column])/2 - 16;	
				}
				else
				{
					// half an icon away from the leftmost part of the column
					offset = gWin.columnLefts[column] + 48;		
				}	
				SetRect(pIcon,pRect->left+offset,pRect->top+(pView->details.iconTop),pRect->left+offset+32,pRect->top+(pView->details.iconTop)+32);
				break;
				
			case ACCOUNT_COLUMN:
			case PROGRAM_COLUMN:
				offset = gWin.columnLefts[column] + (pView->details.nameAddMargin);
				TextFont(pView->font);
				TextSize(pView->fontSize);
				pt.h = pRect->left + offset;
				pt.v = pRect->top+((*pView->hList)->cellSize.v/2)+(pView->fontSize)/2;	// center the text vertically
				GetFontInfo(&fInfo);
				
				if (column==ACCOUNT_COLUMN)
				{
					*pNamePt = pt;
					SetRect(pName,pt.h,pt.v-fInfo.ascent-fInfo.leading,pt.h + StringWidth(pCellData->name),pt.v+fInfo.descent);
				}
				else if (column==PROGRAM_COLUMN)
				{
					*pDatePt = pt;
					SetRect(pDate,pt.h,pt.v-fInfo.ascent-fInfo.leading,pt.h + StringWidth(pCellData->name),pt.v+fInfo.descent);
				}
				break;
				
			default:
				break;
		}
	
	}
	

	
	REST_STUFF;
}

/************************************************************************
 * ViewListCallBack - callback function for List View
 ************************************************************************/
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message, long data)
{
	VLNodeInfo	*pInfo;
	OSErr	err = noErr;

	switch (message)
	{
		case kLVAddNodeItems:
			PopuplateImportList();
			break;
		
		case kLVOpenItem:
			ImportSelectedAccounts();
			break;
			
		case kLVDeleteItem:
			break;
		
		case kLVRenameItem:
			break;
		
		case kLVQueryItem:
			pInfo = (	VLNodeInfo *)data;
			switch (pInfo->query)
			{
				case kQuerySelect:
				case kQueryDCOpens:
					return true;
				
				case kQueryDrag:
				case kQueryDrop:
				case kQueryRename:
					return false;
			}
			break;

		case kLVSendDragData:
			break;
	}
	return err;
}

/************************************************************************
 * PopuplateImportList - search for importable things
 ************************************************************************/
static void PopuplateImportList(void)
{
	OSErr err = noErr;
	ViewListPtr pView = &gWin.list;
	VLNodeInfo	info;
	short count, i;
	
	// Add items from list of importable accounts we built earlier ...
	if (gWin.Importables)
	{
		count = GetHandleSize(gWin.Importables)/sizeof(ImportAccountInfoS);
		for (i=0;i<count;i++)
		{			
			Zero(info);
			PStrCopy(info.name,(*gWin.Importables)[i].accountName,sizeof(info.name));
			info.iconID = (*gWin.Importables)[i].id;
			LVAdd(pView, &info);				
		}
	}
}

/************************************************************************
 * BuildListOfImportables - build a list of all importable accounts on
 *	this machine.
 ************************************************************************/
static OSErr BuildListOfImportables(ImportAccountInfoH *importables)
{
	OSErr err = noErr;

	// initialize it
	*importables = NuHandle(0L);
	err = MemError();
	
	if ((err==noErr) && *importables)
	{	
		ETLQueryImporters(importables, gWin.pluginId, true);
		
		// did we find something?
		if (GetHandleSize(*importables) > 0)
		{
			// Sort the resulting list of importables
			if (importables && *importables && **importables)
				SortImportablesHandle(*importables);
		}
		else
		{
			ZapHandle(*importables);
		}
	}	
	return (err);
}

/************************************************************************
 * ImportSelectedAccounts - import all selected accounts
 ************************************************************************/
static void ImportSelectedAccounts(void)
{
	short i;
	VLNodeInfo info;

	for (i=1;i<=LVCountSelection(&gWin.list);i++)
	{				
		GetListData(&info,i);
		LDRef(gWin.Importables);
		ImportAccount(&(*(gWin.Importables))[info.rowNum-1]);
		UL(gWin.Importables);
	}
}

/************************************************************************
 * ManualImport - prompt the user to select a folder to import
 ************************************************************************/
static void ManualImport(void)
{
	OSErr err = -1;
	Str255 volName;
	short vRef;
	long dirId;
	ImportAccountInfoS importThis;
	
	// set up an account info structure to do the importing ...
	Zero(importThis);
	importThis.size = sizeof(ImportAccountInfoS);
	importThis.id = gWin.pluginId;
	
	while (err && (err != userCanceledErr))
	{
		if (GetFolder(volName,&vRef,&dirId,false,false,false,false))
		{
			SimpleMakeFSSpec(vRef, dirId, "\p", &importThis.importSpec);
			GetDirName(volName,vRef,dirId,importThis.importSpec.name);
			
			PCopy(importThis.accountName,importThis.importSpec.name);
			
			err = ImportAccount(&importThis);
		}
		else err = userCanceledErr;
	}
}


/************************************************************************
 * ImportInterestingClickCallBack - return true if this point is interesting
 ************************************************************************/
static Boolean ImportInterestingClickCallBack(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Point pt)
{
	Boolean result = false;
	Rect rIcon, rName, rApplication;
	Point ptName, ptApplication;
	
	GetCellRectsForImportWin(pView,cellData,cellRect,&rIcon,&rName,&rApplication,&ptName,&ptApplication);
	if (PtInRect(pt,&rIcon) || PtInRect(pt,&rName) || PtInRect(pt,&rApplication)) result = true;
	
	return (result);
}

/**********************************************************************
 * GetCellData - Get data for cell,item is 0-based
 **********************************************************************/
static short GetCellData(ViewListPtr pView,short item,CellRec *pCellData)
{
	short	dataLen;
	Cell	c;
	
	dataLen = sizeof(CellRec);
	c.h = 0; c.v = item;
	LGetCell((Ptr)pCellData, &dataLen, c, pView->hList);
	return dataLen;
}

/************************************************************************
 * ImportGetCellRectsCallBack - return true if this we should drag
 ************************************************************************/
static Boolean ImportGetCellRectsCallBack(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect)
{
	Rect rApplication;
	Point ptName, ptApplication;

	GetCellRectsForImportWin(pView,cellData,cellRect,iconRect,nameRect,&rApplication,&ptName,&ptApplication);
	
	return (true);
}

/************************************************************************
 * SetImportWinBackgroundColor - set the background color of this win
 ************************************************************************/
static void SetImportWinBackgroundColor(void)
{
	if (UseListColors) 
	{
		// Using finder list color scheme.  Punt on background color.
		gWin.win->backColor.red = gWin.win->backColor.green = gWin.win->backColor.blue = 0xffff;
	}
	else
	{
		// Using BACK_COLOR.
		GetRColor(&gWin.win->backColor,BACK_COLOR);	
	}
}
					
/************************************************************************
 * SortImportablesHandle - sort a list of importables
 ************************************************************************/
static void SortImportablesHandle(ImportAccountInfoH toSort)
{
	short count = 0;

	count = GetHandleSize(toSort)/sizeof(ImportAccountInfoS);
	QuickSort((void*)LDRef(toSort),sizeof(ImportAccountInfoS),0,count-1,ImportAccountCompare,(void*)SwapImportable);
	QuickSort((void*)LDRef(toSort),sizeof(ImportAccountInfoS),0,count-1,ImportAppCompare,(void*)SwapImportable);
	UL(toSort);
}

/**********************************************************************
 * ImportAppCompare - compare two cells based on their application
 **********************************************************************/ 
static int ImportAppCompare(ImportAccountInfoP i1, ImportAccountInfoP i2)
{
	return (StringComp(i2->appName,i1->appName));
}

/**********************************************************************
 * ImportAccountCompare - compare two cells based on thir name
 **********************************************************************/ 
static int ImportAccountCompare(ImportAccountInfoP i1, ImportAccountInfoP i2)
{
	return (StringComp(i2->importSpec.name,i1->importSpec.name));
}

/**********************************************************************
 * SwapImportable - swap two importable entries.
 **********************************************************************/ 
static void SwapImportable(ImportAccountInfoP i1, ImportAccountInfoP i2)
{
	ImportAccountInfoS tempI;
	
	tempI = *i1;
	*i1 = *i2;
	*i2 = tempI;
}