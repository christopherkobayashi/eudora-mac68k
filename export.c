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

#include <Folders.h>

#include <conf.h>
#include <mydefs.h>

#include "export.h"

#define FILE_NUM 27
OSErr ExportMailFromMenu(MenuHandle mh,FSSpecPtr mailFolderSpec);
OSErr ExportMailboxFromItem(MenuHandle mh,short item, FSSpecPtr mailFolderSpec);
OSErr ExportMailFolderFromItem(MenuHandle mh,short item, FSSpecPtr mailFolderSpec);
OSErr ExportEnvelope(TOCHandle tocH,short sumNum,short refN);
OSErr ExportMozillaHeaders(TOCHandle tocH,short sumNum,short refN);


/************************************************************************
 * DoFileExport - export Eudora's data to a folder of the user's choosing.
 ************************************************************************/
void DoFileExport(void)
{
	FSSpec exportFolder;
	Str63 folderName;
	Str63 volName;
	
	OSErr err = FindFolder(kOnSystemDisk,kDesktopFolderType,False,&exportFolder.vRefNum,&exportFolder.parID);
	
	if (err) return;	// Bail.  Should warn user that the system can't find the desktop, but geez...
	
	{
		if (GetFolder(volName,&exportFolder.vRefNum,&exportFolder.parID,true,true,true,true))
		{
			// Get the name of the folder chosen
			GetDirName(volName,exportFolder.vRefNum,exportFolder.parID,folderName);
			
			// If the user has chosen a folder with the magic name, use that.
			if (EqualStrRes(folderName,EXPORT_FOLDER_NAME))
			{
				// Pop up a level and then fill in the filename
				if (!(err = ParentSpec(&exportFolder,&exportFolder)))
					PCopy(exportFolder.name,folderName);
				else
				{
					FileSystemError(EXPORT_MAIL_ERR,folderName,err);
					return;
				}
			}
			// If the user didn't choose a magically named folder, make the magically named folder
			else
				FSMakeFSSpec(exportFolder.vRefNum,exportFolder.parID,GetRString(folderName,EXPORT_FOLDER_NAME),&exportFolder);
			
			OpenProgress();
			ExportTo(&exportFolder);
			CloseProgress();
		}
	}
}

/************************************************************************
 * ExportTo - export Eudora's data to a specified folder
 *  Exports all data by calling the various sub-exporters
 ************************************************************************/
OSErr ExportTo(FSSpecPtr exportFolderSpec)
{
	OSErr err = noErr;
	long parID;
	FSSpec subFolderSpec;
	
	ProgressMessageR(kpTitle,EXPORTING_MAIL);
	
	// Check to see that we either have or can create a folder
	if (!FSpIsItAFolder(exportFolderSpec))
	{
		err = FSpDirCreate(exportFolderSpec,smSystemScript,&parID);
		if (err) return FileSystemError(EXPORT_MAIL_ERR,exportFolderSpec->name,err);
	}
	
	// Now that we've verified the folder exists, descend into it for the various
	// exporters
	
	{
		short folderIDs[] = {	EXPORT_MAIL_FOLDER, 0 };
		ExporterFunc *funcs[] = {	ExportMailTo, 0 };
		short i;
		Accumulator errors;
		
		// Record errors, don't present them
		AccuInit(&errors);
		ExportErrors = &errors;
		
		for (i=0; !err && folderIDs[i] && !CommandPeriod ;i++)
		{
			// Point at the appropriate subfolder
			subFolderSpec = *exportFolderSpec;
			subFolderSpec.parID = SpecDirId(exportFolderSpec);
			GetRString(subFolderSpec.name,EXPORT_MAIL_FOLDER);
			
			// Delete what's there already
			if ((err = FSpTrash(&subFolderSpec)))
			{
				if (err == fnfErr) err = noErr;	// ok if it's not there
				else
					return (FileSystemError(EXPORT_MAIL_ERR,subFolderSpec.name,err));
			}
			
			// Create the subfolder
			if ((err= FSpDirCreate(&subFolderSpec,smSystemScript,&parID)))
				return (FileSystemError(EXPORT_MAIL_ERR,subFolderSpec.name,err));
			
			// Descend into it...
			subFolderSpec.parID = parID;
			*subFolderSpec.name = 0;
			
			// Now, call the exporter
			err = funcs[i](&subFolderSpec);
		}
		
		AccuZap(errors);
		ExportErrors = nil;
	}

	return err;
}

/************************************************************************
 * ExportMailTo - Export mail to a particular folder
 ************************************************************************/
OSErr ExportMailTo(FSSpecPtr mailFolderSpec)
{
	MenuHandle mh = GetMHandle(MAILBOX_MENU);
	return ExportMailFromMenu(mh,mailFolderSpec);
}

/************************************************************************
 * ExportMailFrom - Export mail from a particular menu, including submenus
 ************************************************************************/
OSErr ExportMailFromMenu(MenuHandle mh,FSSpecPtr mailFolderSpec)
{
	short item;
	Boolean topLevel = GetMenuID(mh)==MAILBOX_MENU;
	OSErr err = noErr;
	short nItems = CountMenuItems(mh);
	
	// Do In/Out/Junk/Trash
	if (topLevel)
	{
		for (item=1;item<MAILBOX_BAR1_ITEM && !err;item++)
			err = ExportMailboxFromItem(mh,item,mailFolderSpec);
	}
	
	item = MAILBOX_FIRST_USER_ITEM;
	if (!topLevel) item -= MAILBOX_BAR1_ITEM;
	
	for (; item <= nItems && !MenuItemIsSeparator(mh,item) && !err && !CommandPeriod; item++)
		err = (HasSubmenu(mh,item) ? 
						ExportMailFolderFromItem(mh,item,mailFolderSpec) :
						ExportMailboxFromItem(mh,item,mailFolderSpec)) ;

	return err;
}

/************************************************************************
 * ExportMailFolderFromItem - Export mail from a menu item for a subfolder
 ************************************************************************/
OSErr ExportMailFolderFromItem(MenuHandle mh,short item, FSSpecPtr mailFolderSpec)
{
	FSSpec exportSpec;
	OSErr err = noErr;
	long subFolderParID;
	short subMenuID = SubmenuId(mh,item);
	
	// Create a subfolder named after the item
	exportSpec = *mailFolderSpec;
	MailboxMenuFile(GetMenuID(mh),item,exportSpec.name);
	if ((err= FSpDirCreate(&exportSpec,smSystemScript,&subFolderParID)))
		return FileSystemError(EXPORT_MAIL_ERR,exportSpec.name,err);
	
	// Descend into the folder for our next round...
	exportSpec.parID = subFolderParID;
	*exportSpec.name = 0;
	return ExportMailFromMenu(GetMHandle(subMenuID),&exportSpec);	
}

/************************************************************************
 * ExportMailboxFromItem - Export mail from a menu item for a mailbox
 ************************************************************************/
OSErr ExportMailboxFromItem(MenuHandle mh,short item, FSSpecPtr mailFolderSpec)
{
	FSSpec exportSpec = *mailFolderSpec;
	OSErr err;
	short refN=0;
	TOCHandle tocH;
	short i;
	FSSpec boxSpec, origSpec;
	Boolean openedBox, openedMessage;
		
	ProgressMessage(kpMessage,MailboxMenuFile(GetMenuID(mh),item,exportSpec.name));
	if (err = FSpCreate(&exportSpec,'CSOm','TEXT',smSystemScript))
		return FileSystemError(EXPORT_MAIL_ERR,exportSpec.name,err);
	
	if ((err = FSpOpenDF(&exportSpec,fsRdWrPerm,&refN)))
		return FileSystemError(EXPORT_MAIL_ERR,exportSpec.name,err);
	
	GetTransferParams(GetMenuID(mh),item,&boxSpec,nil);
	
	// Don't resolve aliases
	if (err = ResolveAliasNoMount(&boxSpec,&origSpec,nil))
		return FileSystemError(EXPORT_MAIL_ERR,boxSpec.name,err);
		
	openedBox = !(tocH=FindTOC(&boxSpec));
	if (openedBox && !(tocH = TOCBySpec(&boxSpec)))
		return FileSystemError(EXPORT_MAIL_ERR,boxSpec.name,-1);
	
	if ((*tocH)->count) ByteProgress(nil,-1,(*tocH)->count);
	
	for (i=0;i<(*tocH)->count && !CommandPeriod;i++)
	{
		MyWindowPtr win;
		UHandle text;
		Str15 oldNewLine;
		
		PCopy(oldNewLine,NewLine);
		PCopy(NewLine,Lf);
		openedMessage = !(*tocH)->sums[i].messH;
		win = GetAMessage(tocH,i,nil,nil,false);
		if (!win  || !(*tocH)->sums[i].messH || (PETEGetRawText(PETE,(*(*tocH)->sums[i].messH)->bodyPTE,&text),!text))
			; // not sure what's useful to do here...
		else
		{
			ExportEnvelope(tocH,i,refN);
			ExportMozillaHeaders(tocH,i,refN);
			err = SpoolMessage((*tocH)->sums[i].messH,&exportSpec,refN);
		}
		if (openedMessage)
		{
			CloseMyWindow(GetMyWindowWindowPtr(win));
			ZapHandle((*tocH)->sums[i].cache);
		}
		ByteProgress(nil,1,0);
		PCopy(NewLine,oldNewLine);
	}

	FSClose(refN);
	if (openedBox)
		CloseMyWindow((*tocH)->win);
	
	return noErr;
}

/************************************************************************
 * ExportEnvelope - write an envelope for a message to the export file
 ************************************************************************/
OSErr ExportEnvelope(TOCHandle tocH,short sumNum,short refN)
{
	OSErr err = noErr;
	long count = 1;
	UHandle cache;
	Str63 envelope;
	
	CacheMessage(tocH,sumNum);
	cache = (*tocH)->sums[sumNum].cache;
	
	if (cache && *cache)
	{
		UPtr spot;
		count = GetHandleSize(cache);
		MakePStr(envelope,*cache,count);
		spot = PIndex(envelope,Cr[1]);
		if (spot)
		{
			*envelope = spot - envelope - 1;
			if (sumNum>0) PInsert(envelope,sizeof(envelope)-1,NewLine,envelope+1);
			PCat(envelope,NewLine);
			count = *envelope;
			err = AWrite(refN,&count,envelope+1);
		}
	}
	return err;
}

/************************************************************************
 * ExportMozillaHeaders - add Mozilla-specific headers to an export file
 ************************************************************************/
OSErr ExportMozillaHeaders(TOCHandle tocH,short sumNum,short refN)
{
	OSErr err = noErr;
	Str63 flagStr;
	long flags;
	Accumulator keywords;
	
	// first set of flags
	
	flags = 0;
	
	// status
	switch ((*tocH)->sums[sumNum].state)
	{
		case UNREAD: break; // zero works here
		case READ: flags |= MOZ_MSG_FLAG_READ; break;
		case REPLIED: flags |= MOZ_MSG_FLAG_REPLIED; break;
		case QUEUED: flags |= MOZ_MSG_FLAG_FORWARDED; break;
		case FORWARDED: flags |= MOZ_MSG_FLAG_FORWARDED; break;
		case BUSY_SENDING: break; // irrelevant
		case SENT:
		case UNSENT:
		case TIMED:
		case REDIST:
		case UNSENDABLE:
		case SENDABLE:
		case MESG_ERR:
		case REBUILT:
			// none of these are currently supported by mozilla
			;
	}
	
	// Mozilla stores priority high up in the flags
	if (Prior2Display((*tocH)->sums[sumNum].priority) != 3)
		flags |= (1L+Prior2Display((*tocH)->sums[sumNum].priority)) << 13;
	
	// write out the flag if it's nonzero
	if (flags) err = AWriteP(refN,ComposeRString(flagStr,MOZILLA_FLAGS_FMT,flags,NewLine));
	
	if (err) return err;
	
	// Second set of flags
	
	flags = 0;
	
	// We record the a receipt is requested, and then clear the record.
	// Mozilla rediscovers the request when the message is opened, and remembers once
	// you've sent the receipt.  So, if our receipt request flag is NOT set,
	// we will set mozilla's receipt-sent flag.
	// It may be wise to only do this if there is an actual receipt request inside the
	// message, but I'll leave that for another day.
	if (!SumOptIsSet(tocH,sumNum,OPT_RECEIPT)) flags |= MOZ_MSG_FLAG_MDN_REPORT_SENT;
	
	// write out the flag if it's nonzero
	if (flags) err = AWriteP(refN,ComposeRString(flagStr,MOZILLA_FLAGS2_FMT,flags,NewLine));
	
	// Write label out as a keyword
	
		
	// We record the a receipt is requested, and then clear the record.
	// Mozilla rediscovers the request when the message is opened, and remembers once
	// you've sent the receipt.  So, if our receipt request flag is NOT set,
	// we will set mozilla's receipt-sent flag.
	// It may be wise to only do this if there is an actual receipt request inside the
	// message, but I'll leave that for another day.
	if (!SumOptIsSet(tocH,sumNum,OPT_RECEIPT)) flags |= MOZ_MSG_FLAG_MDN_REPORT_SENT;
	
	// write out the flag if it's nonzero
	if (flags) err = AWriteP(refN,ComposeRString(flagStr,MOZILLA_FLAGS2_FMT,flags,NewLine));
	
	return err;
}


OSErr ExportAddressBookTo(FSSpecPtr addressBookFolderSpec)
{
	return unimpErr;
}

OSErr ExportFiltersTo(FSSpecPtr filterFileSpec)
{
	return unimpErr;
}

OSErr ExportStationeryTo(FSSpecPtr stationeryFolderSpec)
{
	return unimpErr;
}

