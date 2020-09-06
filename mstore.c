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

#define FILE_NUM 93
#include "mstore.h"

#pragma segment MIMEStore

/************************************************************************
 * Private Function Prototypes
 ************************************************************************/
MStoreBoxHandle NewMSBoxH(void);
void DisposeMSBoxH(MStoreBoxHandle boxH);
OSErr MSDestroyMailbox(MStoreBoxHandle boxH);
MStoreBoxHandle MSFindMailbox(FSSpecPtr spec);
OSErr MSFixMailbox(MStoreBoxHandle boxH);

/************************************************************************
 * Public functions
 ************************************************************************/

#ifdef DEBUG
/************************************************************************
 * DebugMimeStore - do some debugging on the mime store
 ************************************************************************/
void DebugMimeStore(void)
{
	FSSpec spec;
	OSErr err;
	MStoreBoxHandle boxH;
	
	FSMakeFSSpec(Root.vRef,Root.dirId,"\pMimeStore",&spec);
	err = MSCreateMailbox(&spec,&boxH);
	if (boxH)
	{
		MSCloseMailbox(boxH,True);
		DisposeMSBoxH(boxH);
	}
}
#endif

/************************************************************************
 * MSCreateMailbox - create a new mailbox
 ************************************************************************/
OSErr MSCreateMailbox(FSSpecPtr spec,MStoreBoxHandle *boxHPtr)
{
	long dirId;
	OSErr err;
	MStoreBoxHandle boxH=nil;
	MSSubFileEnum i;
	
	// Create the folder
	if (!(err = FSpDirCreate(spec, smSystemScript, &dirId)))
	{
		// Create the box handle
		if (boxH=NewMSBoxH())
		{
			(*boxH)->dir.vRef = spec->vRefNum;
			(*boxH)->dir.dirId = dirId;
			(*boxH)->spec = *spec;
			BMD(MSSubs,(*boxH)->subs,mssfLimit*sizeof(MStoreSubFile));
			
			for (i=0;!err && i<mssfLimit;i++)
				err = (*(*boxH)->subs[i].func)(msfcCreate,boxH);
			if (!err) err = MSOpenMailbox(nil,&boxH);
		}
		else
			err = MemError();
			
		if (err)
		{
			// Something went wrong; destroy our creation
			MSDestroyMailbox(boxH);
			DisposeMSBoxH(boxH);
			boxH = nil;
		}
	}
	else
		FileSystemError(CREATING_MAILBOX,spec->name,err);
	
	if (boxHPtr)
		*boxHPtr = boxH;
	else if (boxH)
		MSCloseMailbox(boxH,True);
		
	return(err);
}

/************************************************************************
 * MSOpenMailbox - open a mailbox
 ************************************************************************/
OSErr MSOpenMailbox(FSSpecPtr spec,MStoreBoxHandle *boxH)
{
	OSErr err = noErr;
	FSSpec localSpec;
	MSSubFileEnum i;
	
	/*
	 * is it open already?
	 */
	if (spec && (*boxH = MSFindMailbox(spec)))
	{
		(**boxH)->refCount++;
		return(noErr);
	}
	
	/*
	 * if we're passed a filespec, use it
	 */
	if (spec && !FSpExists(spec))
	{
		IsAlias(spec,&localSpec);
		if (err=FSpIsItAFolder(&localSpec))
		{
			if (err==1) err = dupFNErr;
			FileSystemError(OPEN_MBOX,localSpec.name,err);
		}
		if (!(*boxH=NewMSBoxH()))
			WarnUser(OPEN_MBOX,err=MemError());
		else
		{
			(**boxH)->spec = *spec;
			localSpec.parID = SpecDirId(&localSpec);
			(**boxH)->dir.vRef = localSpec.vRefNum;
			(**boxH)->dir.dirId = localSpec.parID;
		}
	}
	
	/*
	 * the box handle has the file info in it.  Go open the files
	 */
	for (i=0;!err && i<mssfLimit;i++)
		err = (*(**boxH)->subs[i].func)(msfcOpen,*boxH);
	if (!err)
	{
		/*
		 * all files open.  check for consistency
		 */
		err = MSFixMailbox(*boxH);
	}
	else
	{
		MSCloseMailbox(*boxH,True);
		if (spec) ZapHandle(*boxH);
	}
	return(err);
}

/************************************************************************
 * MSFlushMailbox - flush a mailbox
 ************************************************************************/
OSErr MSFlushMailbox(MStoreBoxHandle boxH)
{
	OSErr err = noErr;
	MSSubFileEnum i;
	
	for (i=0;!err && i<mssfLimit;i++)
		err = (*(*boxH)->subs[i].func)(msfcFlush,boxH);

	return(err);
}

/************************************************************************
 * MSCloseMailbox - close a mailbox
 ************************************************************************/
OSErr MSCloseMailbox(MStoreBoxHandle boxH,Boolean force)
{
	OSErr err = noErr;
	MSSubFileEnum i;
	
	if (boxH)
	{
		// is the mailbox in use?
		if (--(*boxH)->refCount) return(fBsyErr);
		
		// write the files
		err = MSFlushMailbox(boxH);
		if (force) err = noErr;	// if force, we want it to close in spite of error
		
		// close the files
		for (i=0;!err && i<mssfLimit;i++)
			err = (*(*boxH)->subs[i].func)(force ? msfcCloseHard:msfcClose,boxH);

		if (err) ++(*boxH)->refCount;	// error; not closing after all
	}
	return(err);
}

/************************************************************************
 * Semiprivate functions - used only in the rest of mimestore
 ************************************************************************/

/************************************************************************
 * MSCreateSubFile - create a file in a mailbox
 ************************************************************************/
OSErr MSCreateSubFile(MStoreBoxHandle boxH,MSSubFileEnum file)
{
	FSSpec spec;
	Str63 name;
	
	SimpleMakeFSSpec((*boxH)->dir.vRef,(*boxH)->dir.dirId,GetRString(name,(*boxH)->subs[file].fileNameID),&spec);
	return(FSpCreate(&spec,CREATOR,(*boxH)->subs[file].fileType,smSystemScript));
}

/************************************************************************
 * MSCloseSubFile - close a subfile
 ************************************************************************/
OSErr MSCloseSubFile(MStoreBoxHandle boxH,MSSubFileEnum file)
{
	OSErr err = noErr;
	
	if ((*boxH)->subs[file].refN)
	{
		err = MyFSClose((*boxH)->subs[file].refN);
		if (!err) (*boxH)->subs[file].refN = 0;
	}
	
	return(err);
}

/************************************************************************
 * MSOpenSubFile - open a file in a mailbox
 ************************************************************************/
OSErr MSOpenSubFile(MStoreBoxHandle boxH,MSSubFileEnum file)
{
	FSSpec spec;
	Str63 name;
	short refN=0;
	OSErr err = FSMakeFSSpec((*boxH)->dir.vRef,(*boxH)->dir.dirId,GetRString(name,(*boxH)->subs[file].fileNameID),&spec);
	
	if (!err) err = FSpOpenDF(&spec,fsRdWrPerm,&refN);
	
	(*boxH)->subs[file].refN = refN;
	
	if (err) FileSystemError(OPEN_MBOX,name,err);
	
	return(err);	
}

/************************************************************************
 * MSDestroySubFile - destroy a file in a mailbox
 ************************************************************************/
OSErr MSDestroySubFile(MStoreBoxHandle boxH,MSSubFileEnum file)
{
	Str63 name;
	FSSpec spec;
	OSErr err = FSMakeFSSpec((*boxH)->dir.vRef,(*boxH)->dir.dirId,GetRString(name,(*boxH)->subs[file].fileNameID),&spec);

	if (!err) err = ChainDelete(&spec);
	
	if (err) FileSystemError(OPEN_MBOX,name,err);
	
	return(err);		
}

/************************************************************************
 * Private functions
 ************************************************************************/
 
/************************************************************************
 * NewMSBoxH - allocate a new mailbox handle
 ************************************************************************/
MStoreBoxHandle NewMSBoxH(void)
{
	MStoreBoxHandle boxH;
	StackHandle stack;
	OSErr err;
	
	// Allocate the handle and some stacks
	if (boxH = NewZH(MStoreBox))
	{
		(*boxH)->refCount = 1;
		if (!(err = StackInit(sizeof(MStoreMemIDDB),&stack)))
		{
			(*boxH)->iddb = stack;
			if (!(err = StackInit(sizeof(MStoreSummary),&stack)))
			{
				(*boxH)->summaries = stack;
			}
		}
	}
	else err = MemError();
	
	// Did we win?
	if (err)
	{
		WarnUser(MEM_ERR,MemError());
		DisposeMSBoxH(boxH);
		boxH = nil;
	}
	
	return(boxH);
}

/************************************************************************
 * DisposeMSBoxH - dispose of a mailbox storage object
 ************************************************************************/
void DisposeMSBoxH(MStoreBoxHandle boxH)
{
	if (boxH)
	{
		ZapHandle((*boxH)->iddb);
		ZapHandle((*boxH)->summaries);
	}
}

/************************************************************************
 * MSDestroyMailbox - Destroy a mailbox
 ************************************************************************/
OSErr MSDestroyMailbox(MStoreBoxHandle boxH)
{
	OSErr err;
	FSSpec spec = (*boxH)->spec;
	MSSubFileEnum i;
	
	// are we in use?
	if ((*boxH)->refCount>1) return(fBsyErr);
	
	// close it first
	if (!(err=MSCloseMailbox(boxH,True)))
		// destroy all the files
		for (i=0;!err && i<mssfLimit;i++)
			err = (*(*boxH)->subs[i].func)(msfcDestroy,boxH);

	// if all went well, we can nuke the directory
	if (!err)
		err = RemoveDir(&spec);

	return(err);
}

/************************************************************************
 * MSFixMailbox - make sure the mailbox is up to snuff
 ************************************************************************/
OSErr MSFixMailbox(MStoreBoxHandle boxH)
{
	return(noErr);
}

/************************************************************************
 * MSFindMailbox - see if a mailbox is already open
 ************************************************************************/
MStoreBoxHandle MSFindMailbox(FSSpecPtr spec)
{
	return(nil);
}
