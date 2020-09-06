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

#include "squish.h"
#define FILE_NUM 44

#pragma segment Squish
	Boolean NeedsCompaction(FSSpecPtr spec,Boolean *opened);
/************************************************************************
 * DoCompact - compact all mailboxes
 ************************************************************************/
void DoCompact(short vRef,long dirId,CInfoPBRec *hfi,short suffixLen)
{
	FSSpec thisOne, resolved;
	FSSpecHandle names;
	short count,item;
	short index;
	Boolean wasAlias;
	short err;
	Boolean newTOC = PrefIsSet(PREF_NEW_TOC);
	short badIndices[] = {ALIAS_FILE,CACHE_ALIAS_FILE,PERSONAL_ALIAS_FILE,LOG_NAME,OLD_LOG,FILTERS_NAME,SIG_FOLDER,NICK_FOLDER,PHOTO_FOLDER,ATTACH_FOLDER,STATION_FOLDER,SPOOL_FOLDER,ITEMS_FOLDER,LINK_HISTORY_FOLDER,0};
	short bad;
	Boolean opened;
	Boolean bNeedsProgress = (dirId==MailRoot.dirId && SameVRef(vRef,MailRoot.vRef)) || (dirId==IMAPMailRoot.dirId && SameVRef(vRef,IMAPMailRoot.vRef));
	
	/*
	 * get started
	 */
	if ((names=NuHandle(0L))==nil)
		DieWithError(ALLO_MBOX_LIST,MemError());
	
	/*
	 * read names of mailbox files
	 */
	hfi->hFileInfo.ioNamePtr = thisOne.name;
	for (hfi->hFileInfo.ioFDirIndex=index=0;!DirIterate(vRef,dirId,hfi);hfi->hFileInfo.ioFDirIndex=++index)
	{
		/*
		 * make an FSSpec out of the info
		 */
		thisOne.vRefNum = vRef;
		thisOne.parID = dirId;
		
		/*
		 * skip special files
		 */
		if (dirId==Root.dirId && vRef==Root.vRef)
		{
			for (bad=0;badIndices[bad];bad++) if (EqualStrRes(thisOne.name,badIndices[bad])) break;
			if (badIndices[bad]) continue;
		}
		
		/*
		 * if alias, resolve it and get hfi of new file
		 */
		resolved = thisOne;
		if ((err=ResolveAliasNoMount(&resolved,&resolved,&wasAlias)) && err!=fnfErr) continue;
			/* fnfErr in the above context means a directory */
		if (wasAlias)
		{
			err = AFSpGetCatInfo(&resolved,&resolved,hfi);
			hfi->hFileInfo.ioNamePtr = thisOne.name;	/* o the tangled webs we weave */
			if (err) continue;
		}
		
		/*
		 * directory; set name to zero to indicate that we want the directory, and follow
		 * the alias (if any)
		 */
		if (0!=(hfi->hFileInfo.ioFlAttrib&0x10))
		{
			thisOne.vRefNum = resolved.vRefNum;
			thisOne.parID = SpecDirId(&resolved);
			*thisOne.name = 0;
		}
		
		/*
		 * regular file; we only need compact boxes with .toc's, so we look
		 * only for .toc files
		 */
		else if (!newTOC)
		{
			if (hfi->hFileInfo.ioFlFndrInfo.fdType!=TOC_TYPE) continue;
			if (*thisOne.name<=suffixLen) continue;	/* impossible filename */
			*thisOne.name -= suffixLen;					/* remove suffix */
			thisOne.name[*thisOne.name+1] = 0;	/* null-terminate */
			/*
			 * skip special files
			 */
			if (dirId==Root.dirId && vRef==Root.vRef)
			{
				for (bad=0;badIndices[bad];bad++) if (EqualStrRes(thisOne.name,badIndices[bad])) break;
				if (badIndices[bad]) continue;
			}
		}
		else if (!IsMailbox(&thisOne)) continue;
		
		PtrPlusHand_ (&thisOne,names,sizeof(thisOne));
		if (MemError()) DieWithError(ALLO_MBOX_LIST,MemError());
	}
	
	if (bNeedsProgress)
	{
		OpenProgress();
		ProgressMessageR(kpTitle,COMPACTING);
	}
	count = GetHandleSize_(names)/sizeof(thisOne);
	for (item=0;item<count;item++)
	{
		if (CommandPeriod || MiniEvents()) break;
		thisOne = (*names)[item];
		if (!*thisOne.name)	/* directory */
			DoCompact(thisOne.vRefNum,thisOne.parID,hfi,suffixLen);
		else
		{
			if (!EqualStrRes(thisOne.name,IMAP_HIDDEN_TOC_NAME)) ProgressMessage(kpSubTitle,thisOne.name);
			if (NeedsCompaction(&thisOne,&opened)) StackPush(&thisOne,CompactStack);
		}
	}
	if (bNeedsProgress) CloseProgress();
	ZapHandle(names);
}

/************************************************************************
 * NeedsCompaction - see if a toc and a mailbox are in perfect agreement
 ************************************************************************/
Boolean NeedsCompaction(FSSpecPtr spec,Boolean *opened)
{
	WindowPtr	tocWinWP;
	TOCHandle tocH;
	short mNum;
	long offset = 0;
	Boolean need=False;
	long eof;
	CInfoPBRec info;
	FSSpec tocSpec;
	long	thisOffset;
	
	*opened = !FindTOC(spec);
	
	if (tocH=TOCBySpec(spec))
	{
		if (PrefIsSet(PREF_NEW_TOC))
		{
			Box2TOCSpec(spec,&tocSpec);
			need = 0==FSpExists(&tocSpec);
		}
		for (mNum=0; mNum<(*tocH)->count; mNum++)
		{
			thisOffset = (*tocH)->sums[mNum].offset;
			if (thisOffset >= 0)	//	Ignore IMAP messages not downloaded
			{
				if (thisOffset != offset)
				{
					need = True;
					break;
				}
				else
					offset += (*tocH)->sums[mNum].length;
			}
		}
		
		if (!need)
		{
			if (!AFSpGetHFileInfo(spec,&info) &&
					info.hFileInfo.ioFlLgLen!=offset && !BoxFOpen(tocH))
			{
				if (!GetEOF((*tocH)->refN,&eof))
				{
					SetEOF((*tocH)->refN,offset);
					TOCSetDirty(tocH,true);
				}
				BoxFClose(tocH,false);
			}
		}
		tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
		if (!need && !IsWindowVisible(tocWinWP)) CloseMyWindow(tocWinWP);
	}
	return(need);
}

/************************************************************************
 * CompactMailbox - rewrite a mailbox to agree with a table of contents
 ************************************************************************/
void CompactMailbox(FSSpecPtr spec,Boolean opened)
{
	TOCHandle tocH=nil;
	short oldRefN=0, newRefN=0;
	long size=0;
	short mNum;
	int err=0;
	int rnErr=0;
	FInfo info;
	CInfoPBRec hfi;
	FSSpec newSpec,tmpSpec;
	long spot, len, toSpot;
	long curOff, curLen;
		
	CycleBalls();
	
	opened = nil==(tocH=FindTOC(spec)) || opened;
	if (!tocH && !(tocH=TOCBySpec(spec))) return;
	
	// Compact has a special meaning in the junk mailbox
	if (JunkPrefBoxArchive() && (*tocH)->which==JUNK) ArchiveJunk(tocH);
	
	IsAlias(spec,&newSpec);
	NewTempSpec(newSpec.vRefNum,newSpec.parID,nil,&tmpSpec);

	if (err=BoxFOpen(tocH)) goto done;
	oldRefN = (*tocH)->refN;
	if (!FSpGetHFileInfo(&tmpSpec,&hfi) && hfi.hFileInfo.ioFlLgLen)
		{FileSystemError(SQUISH_LEFTOVERS,tmpSpec.name,dupFNErr); goto done;}
	if (err=MakeResFile(tmpSpec.name,tmpSpec.vRefNum,tmpSpec.parID,CREATOR,MAILBOX_TYPE))
		{FileSystemError(COULDNT_SQUEEZE,tmpSpec.name,err); goto done;}
	if (err=FSpOpenDF(&tmpSpec,fsRdWrPerm,&newRefN))
		{FileSystemError(COULDNT_SQUEEZE,tmpSpec.name,err); goto done;}

	/*
	 * figure out how big it should be
	 */
	for (mNum=0;mNum<(*tocH)->count;mNum++)
	{
		if ((*tocH)->sums[mNum].offset >= 0)	//	Skip IMAP messages not yet downloaded
			size += (*tocH)->sums[mNum].length;
	}
	if (err=MyAllocate(newRefN,size))
		{FileSystemError(COULDNT_SQUEEZE,newSpec.name,err); goto done;}

	size = spot = len = toSpot = 0;
	for (mNum=0;mNum<(*tocH)->count;mNum++)
	{
	  CycleBalls();
	  curOff = (*tocH)->sums[mNum].offset;
	  curLen = (*tocH)->sums[mNum].length;
	  if (curOff < 0) continue;	//	Skip IMAP messages not yet downloaded
	  
	  /*
	   * do we have a range already?
	   */
	  if (len)
	  {
	  	/*
	  	 * we do.  can we just tack onto the end of it?
	  	 */
		 	if (spot+len==curOff)
		  {
		  	/* we can */
		  	len += curLen;
		  }
		  /*
		   * no, this message starts someplace else.  copy what we have
		   */
		  else
		  {
		  	if (err=CopyFBytes(oldRefN,spot,len,newRefN,toSpot))
					{FileSystemError(COULDNT_SQUEEZE,newSpec.name,err); goto done;}
				len = 0;	/* signal to start over */
			}
		}
		
		/*
		 * begin a new copy range?
		 */
		if (!len)
	  {
	  	/* nope; make one */
	  	spot = curOff;
	  	len = curLen;
	  	toSpot = size;
	  }
	  
	  /*
	   * for the next round
	   */
		(*tocH)->sums[mNum].offset = size;
		size += curLen;
	}
	
	/*
	 * leftover last range?
	 */
	if (len)
	{
  	if (err=CopyFBytes(oldRefN,spot,len,newRefN,toSpot))
			{FileSystemError(COULDNT_SQUEEZE,newSpec.name,err); goto done;}
	}

	/* success! */
	MyFSClose(newRefN); newRefN = 0;
	MyFSClose(oldRefN); oldRefN = 0;
	(*tocH)->refN = 0;
	
	/* now, copy the resource fork */
	KillTOC(nil,&newSpec);	/* dump toc; we'll rewrite */
	KillTOC(nil,&newSpec);	/* dump backup, too */
	(void) CopyRFork(tmpSpec.vRefNum,tmpSpec.parID,tmpSpec.name,
									 newSpec.vRefNum,newSpec.parID,newSpec.name,False);
	FSpGetFInfo(&newSpec,&info);
	
	/* do the deed */
	if (err=rnErr=ExchangeFiles(&newSpec,&tmpSpec))
		FileSystemError(BAD_COMP_RENAME,newSpec.name,err);
	FSpSetFInfo(&newSpec,&info);
	
	/* write the new toc RIGHT NOW */
	if (err = WriteTOC(tocH))
		{FileSystemError(COULDNT_SQUEEZE,newSpec.name,err); goto done;}
	
done:
	if (newRefN) MyFSClose(newRefN);
	if (oldRefN) {MyFSClose(oldRefN); (*tocH)->refN = 0;}
	if (tocH)
	{
		TOCSetDirty(tocH,TOCIsDirty(tocH) || err==0);
		(*tocH)->reallyDirty = (*tocH)->reallyDirty || err==0;
		(*tocH)->totalK = (*tocH)->usedK;	/* let's not do this again, shall we? */
		(*tocH)->updateBoxSizes = true;
		if (err && !opened)
		{
			/*
			 * Verily, this is bad.  close the message windows and close the toc
			 */
			for (mNum=0;mNum<(*tocH)->count;mNum++)
				if ((*tocH)->sums[mNum].messH)
				{
					(*(*tocH)->sums[mNum].messH)->win->isDirty = False;	/* better to lose work in progress than corrupt the mailbox */
					CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[mNum].messH)->win));
				}
		}
		if (err || opened) CloseMyWindow(GetMyWindowWindowPtr((*tocH)->win));
	}
	if (!rnErr)
	{
		if (PrefIsSet(PREF_WIPE)) WipeSpec(&tmpSpec);
		FSpDelete(&tmpSpec);
	}
}


/************************************************************************
 * NeedAutoCompact - should we compact this mailbox?
 ************************************************************************/
Boolean NeedAutoCompact(TOCHandle tocH)
{
	long waste = ((*tocH)->totalK-(*tocH)->usedK)K;
	FSSpec spec = GetMailboxSpec(tocH,-1);
	
	/*
	 * don't compact temp mailboxes, ever
	 */
	if ((*tocH)->virtualTOC || (*tocH)->which==IN_TEMP || (*tocH)->which==OUT_TEMP || IsDelivery(&spec))
		return false;
	
	/*
	 * is there room to do it?
	 */
	NoteFreeSpace(tocH);
	if ((*tocH)->volumeFree < ((*tocH)->usedK+10)K) return (False);
	
	/*
	 * is there too much waste space?
	 */
	if ((100*waste)/((*tocH)->totalK K + 1) > GetRLong(COMPACT_WASTE_PER)) return(True);
	
	/*
	 * are we too close to the disk limit?
	 */
	if ((100*waste)/(*tocH)->volumeFree > GetRLong(COMPACT_FREE_PER)) return(True);
		
	return(False);
}
