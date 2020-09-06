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

#include "fileutil.h"
#define FILE_NUM 13
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

pascal Boolean FolderFilter(FileParam *pb);
pascal short FolderItems(short item,DialogPtr dlog);
OSErr FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest);
Boolean	HasFileIDs(const GetVolParmsInfoBuffer *volParms);
static OSErr GenerateUniqueName(short volume,long *startSeed,long dir1,long dir2,StringPtr uniqueName);
Boolean FileIsInvisible(CInfoPBRec *hfi);

/**********************************************************************
 * various useful functions related to the filesystem
 **********************************************************************/

#pragma segment FileUtil
#define FILL(pb,name,vRef,dirId) do { 									\
	(pb).ioNamePtr = name;																\
	(pb).ioVRefNum = vRef;																\
	(pb).ioDirID = dirId; 																\
	} while (0)

void FileIDHack(void);	// JDB 980720 Hack to work around apple's fileID bug

/**********************************************************************
 * GetMyVR - get a volume ref number
 **********************************************************************/
short GetMyVR(UPtr name)
{
	HParamBlockRec vInfo;
	Str255	fileName;
	
	PCopy(fileName,name);	//	Make copy, name might get overwritten
	// PBHGetVInfo doesn't work right without ':' at end of volume name
	if (fileName[*fileName] != ':') PCatC(fileName,':');
	FILL(*((HFileInfo*)&vInfo),fileName,0,0);
	vInfo.volumeParam.ioVolIndex = -1;
	if (PBHGetVInfoSync((HParmBlkPtr)&vInfo))
		return(REAL_BIG);
	return(vInfo.volumeParam.ioVRefNum);
}

/************************************************************************
 * ParentSpec - get the FSSpec of a parent
 ************************************************************************/
OSErr ParentSpec(FSSpecPtr child,FSSpecPtr parent)
{
	CInfoPBRec pb;
	OSErr err;
	
	Zero(pb);
	pb.dirInfo.ioNamePtr = parent->name;
	pb.dirInfo.ioVRefNum = child->vRefNum;
	pb.dirInfo.ioDrDirID = child->parID;
	pb.dirInfo.ioFDirIndex = -1;		/* use ioDirID */
	err = PBGetCatInfoSync(&pb);
	if (!err)
	{
		*parent->name = 0;
		parent->parID = pb.dirInfo.ioDrParID;
	}
	return(err);
}

/**********************************************************************
 * get a name, given a vRefNum
 **********************************************************************/
short GetDirName(UPtr volName,short vRef, long dirId,UPtr name)
{
	CInfoPBRec catBlock;	/* to get the name of a directory */
	short err;
	
	if (volName && !vRef)
		//	Get vRef from volume name
		vRef = GetMyVR(volName);
	
	*name = 0;
	
	/*
	 * get name of directory
	 */
	catBlock.dirInfo.ioDrDirID = dirId;
	catBlock.dirInfo.ioNamePtr = name;
	catBlock.dirInfo.ioVRefNum = vRef;
	catBlock.dirInfo.ioFDirIndex = -1;		/* use ioDirID */
	if ((err=PBGetCatInfo(&catBlock,0))!=noErr) /* get working directory info */
		*name = 0;
	return(err);
}

/**********************************************************************
 * get a volume name, given a vRefNum
 **********************************************************************/
UPtr GetMyVolName(short refNum,UPtr name)
{
	HParamBlockRec vInfo;
		
	*name = 0;
	vInfo.volumeParam.ioNamePtr = name;
	vInfo.volumeParam.ioVRefNum = refNum;
	vInfo.volumeParam.ioVolIndex = 0;
	PBHGetVInfoSync((HParmBlkPtr)&vInfo); 				/* get the volume info */

	return(name);
}

/************************************************************************
 * IndexVRef - return vref's by index
 ************************************************************************/
OSErr IndexVRef(short index, short *vRef)
{
	HParamBlockRec myPB;
	int err;

	myPB.volumeParam.ioNamePtr = nil;
	myPB.volumeParam.ioVolIndex = index;
	myPB.volumeParam.ioCompletion = nil;
	
	if (err=PBHGetVInfoSync((HParmBlkPtr)&myPB)) return(err);
	*vRef = myPB.volumeParam.ioVRefNum;
	return(noErr);
}

/************************************************************************
 * BlessedVRef - find the vref of the blessed folder
 ************************************************************************/
short BlessedVRef(void)
{
	short	vRefNum;
	long	dirID;
	
	FindFolder(kOnSystemDisk,kSystemFolderType,false,&vRefNum,&dirID);
	return vRefNum;
}

/**********************************************************************
 * MakeResFile - create a resource file in a given directory
 **********************************************************************/
int MakeResFile(UPtr name,int vRef,long dirId,long creator,long type)
{
	int err;
	FSSpec spec;
	
	FSMakeFSSpec(vRef,dirId,name,&spec);
	FSpCreateResFile(&spec,creator,type,smSystemScript);
	err=ResError();
	// (jp) NetWare servers incorrectly report noMacDskErr when attempting to
	//			create a resource file (the signature bytes are apparently wrong)
	//			We can create a file with both forks, however.
	if (err == noMacDskErr)
		err=FSpCreate(&spec,creator,type,smSystemScript);
	return (err==dupFNErr ? noErr : err);
}

/**********************************************************************
 * DirIterate - iterate over the files in a directory.
 **********************************************************************/
short DirIterate(short vRef,long dirId,CInfoPBRec *hfi)
{
	OSErr err;
	short idx;
	StringPtr name;
	do
	{
	//	Save off bits
		idx = hfi->hFileInfo.ioFDirIndex;
		name = hfi->hFileInfo.ioNamePtr;

		Zero ( *hfi );
		name [ 0 ] = 0;
		
		hfi->hFileInfo.ioNamePtr = name;
		hfi->hFileInfo.ioFDirIndex = ++idx;		
		hfi->hFileInfo.ioDirID = dirId;
		hfi->hFileInfo.ioVRefNum = vRef;

		err = PBGetCatInfoSync((CInfoPBPtr)hfi);
	} while (!err && FileIsInvisible(hfi));	// ignore invisible files
	
#ifdef DEBUG
	if (RunType!=Production && err==-43)
	{
		CInfoPBRec cpb;
		Str63 lName;
		OSErr myErr;
		
		Zero(cpb);
		Zero(lName);
		cpb.dirInfo.ioNamePtr = lName;
		cpb.dirInfo.ioDrDirID = dirId;
		cpb.dirInfo.ioVRefNum = vRef;
		myErr = PBGetCatInfoSync(&cpb);
		ASSERT(!myErr);
		ASSERT(cpb.dirInfo.ioDrNmFls==idx-1);
	}
#endif
	return err;
}

/**********************************************************************
 * FileIsInvisible - is this file invisible?
 **********************************************************************/
Boolean FileIsInvisible(CInfoPBRec *hfi)
{
	return (hfi->hFileInfo.ioFlFndrInfo.fdFlags & fInvisible) ||
		(HaveOSX() && hfi->hFileInfo.ioNamePtr[0] && hfi->hFileInfo.ioNamePtr[1]=='.');
}

/**********************************************************************
 * CopyFBytes - copy bytes from one file to another
 **********************************************************************/
int CopyFBytes(short fromRefN,long fromOffset,long length,short toRefN,long toOffset)
{
	int err;
	Handle buffer;
	long size;
	long count;
	long fromEnd = fromOffset + length;
	long toEnd;
	
	if (err = GetEOF(toRefN,&toEnd)) return(err);
	
	if (toEnd < toOffset+length-1)
		if (err = SetEOF(toRefN,toOffset + length - 1)) return(err);
	toEnd = toOffset + length;
		
	buffer = NewIOBHandle(255,MIN(OPTIMAL_BUFFER,length));
	if (buffer==nil) return(WarnUser(MEM_ERR,MemError()));
	LDRef(buffer);
	size = GetHandleSize_(buffer);
	
	do
	{
		CycleBalls();
		count = size > length ? length : size;
		
		if (err = SetFPos(fromRefN,fsFromStart,fromEnd-count)) break;
		if (err = ARead(fromRefN,&count, (UPtr) *buffer)) break;
		
		if (err = SetFPos(toRefN,fsFromStart,toEnd-count)) break;
		if (err = NCWrite(toRefN,&count, (UPtr) *buffer)) break;
		
		length -= count;
		fromEnd -= count;
		toEnd -= count;
	}
	while (length);
	ZapHandle(buffer);
	return(err);
}


#define HNLSIZE 2048
/************************************************************************
 * NuntNewline - find a newline in a file
 ************************************************************************/
OSErr HuntNewline(short refN,long aroundSpot,long *newline,Boolean *realNl)
{
	UHandle buffer = (UHandle) NuHTempOK(HNLSIZE);
	long spot, count;
	UPtr nl1,nl2,end;
	UPtr aSpot;
	short err;
	
	if (!buffer) return(WarnUser(MEM_ERR,MemError()));
	LDRef(buffer);
	
	/*
	 * read in a buffer containing aSpot
	 */
	spot = MAX(0,aroundSpot-HNLSIZE/2);
	count = HNLSIZE;
	
	if (err = SetFPos(refN,fsFromStart,spot))
		{FileSystemError(READ_MBOX,"\p",err);goto done;}
	
	err = ARead(refN,&count, (UPtr) *buffer);
	if (err == eofErr && count>0) err = noErr;		/* ignore running off the end of the file as long as we got
																									 some bytes */
	if (err)
		{FileSystemError(READ_MBOX,"\p",err);goto done;}
	
	aSpot = (UPtr) *buffer + (aroundSpot-spot);
	end = (UPtr) *buffer + count;
	
	/*
	 * search both forwards and backwards for newlines
	 */
	for (nl1 = aSpot;nl1>=*buffer;nl1--) if (*nl1 == '\015') break;
	for (nl2 = aSpot;nl2<end;nl2++) if (*nl2 == '\015') break;
	
	/*
	 * take the closest newline to the desired spot
	 */
	if (nl1<*buffer)
	{
		if (nl2<end) aSpot = nl2;
	}
	else if (nl2>end) aSpot = nl1;
	else
		aSpot = ((nl2-aSpot)<(aSpot-nl1)) ? nl2 : nl1;
		
	*realNl = *aSpot == '\015';
	*newline = spot + (aSpot-*buffer)+1;
		
done:
	ZapHandle(buffer);
	return(err);
}

/************************************************************************
 * TruncOpenFile - truncate an open file to a given spot
 ************************************************************************/
OSErr TruncOpenFile(short refN, long spot)
{
	short err;
	
	if (err=SetFPos(refN,fsFromStart,spot)) return(err);
	return(SetEOF(refN,spot));
}

/************************************************************************
 * TruncAtMark - truncate an open file at the current spot
 ************************************************************************/
OSErr TruncAtMark(short refN)
{
	short err;
	long spot;
	
	if (err=GetFPos(refN,&spot)) return(err);
	return(SetEOF(refN,spot));
}

/************************************************************************
 * StdFilespot - figure out where a stdfile dialog should go
 ************************************************************************/
void StdFileSpot(Point *where, short id)
{
	Rect r,in;
	DialogTHndl dTempl; 			
	if ((dTempl=(DialogTHndl)GetResource_('ALRT',id)) ||
			(dTempl=(DialogTHndl)GetResource_('DLOG',id)))
	{
		BitMap	screenBits;
		
		r = (*dTempl)->boundsRect;
		GetQDGlobalsScreenBits(&screenBits);
		in = screenBits.bounds;
		in.top += GetMBarHeight();
		ThirdCenterRectIn(&r,&in);
		where->h = r.left;
		where->v = r.top;
	}
	else
	{
		where->h = 100;
		where->v = 100;
	}
}

/************************************************************************
 * AFSHOpen - like FSOpen, but takes a dirId and permissions, too.
 ************************************************************************/
short AFSHOpen(UPtr name,short vRefN,long dirId,short *refN,short perm)
{
	HParamBlockRec pb;
	int err;
	Str255 newName;
	
	Zero(pb);
	PCopy(newName,name);
	if (err=MyResolveAlias(&vRefN,&dirId,newName,nil)) return(err);
	pb.ioParam.ioNamePtr = newName;
	pb.ioParam.ioVRefNum = vRefN;
	pb.ioParam.ioPermssn = perm;
	pb.ioParam.ioMisc = nil;
	pb.fileParam.ioDirID = dirId;
	err = PBHOpenSync((HParmBlkPtr)&pb);
	if (!err) *refN = pb.ioParam.ioRefNum;
	return(err);
}

/************************************************************************
 * ARFHOpen - like RFOpen, but with dirId and permissions
 ************************************************************************/
short ARFHOpen(UPtr name,short vRefN,long dirId,short *refN,short perm)
{
	HParamBlockRec pb;
	int err;
	Str255 newName;
	
	PCopy(newName,name);
	Zero(pb);
	if (err=MyResolveAlias(&vRefN,&dirId,newName,nil)) return(err);	
	pb.ioParam.ioCompletion = nil;
	pb.ioParam.ioNamePtr = newName;
	pb.ioParam.ioVRefNum = vRefN;
	pb.ioParam.ioVersNum = 0;
	pb.ioParam.ioPermssn = perm;
	pb.ioParam.ioMisc = nil;
	pb.fileParam.ioDirID = dirId;
	err = PBHOpenRFSync((HParmBlkPtr)&pb);
	if (!err) *refN = pb.ioParam.ioRefNum;
	return(err);
}

/************************************************************************
 * VolumeMargin - is there enough space on a volume for something?
 ************************************************************************/
OSErr VolumeMargin(short vRef,long spaceNeeded)
{
	long margin = GetRLong(VOLUME_MARGIN);
	
	if (margin && VolumeFree(vRef)<spaceNeeded+margin) return(dskFulErr);

	return(noErr);
}

/************************************************************************
 * MyAllocate - do a PBAllocate call
 ************************************************************************/
int MyAllocate(short refN,long size)
{
	IOParam pb;
	
	pb.ioCompletion = nil;
	pb.ioRefNum = refN;
	pb.ioReqCount = size;
	return(PBAllocateSync((ParmBlkPtr)&pb));
}

/************************************************************************
 * SFPutOpen - open a file for write, using stdfile
 ************************************************************************/
short SFPutOpen(FSSpecPtr spec,long creator,long type,short *refN,ModalFilterYDUPP filter,DlgHookYDUPP hook,short id,FSSpecPtr defaultSpec,PStr windowTitle, PStr message)
{
	FInfo				info;
	ScriptCode	script;
	OSErr				theError;
	short				ditlID;

	if (!MommyMommy(ATTENTION,nil)) return(1);

	if (UseNavServices ()) {
		switch (id) {
			case SAVEAS_DLOG:
				ditlID = SAVEAS_NAV_DITL;
				break;
			default:
				ditlID = 0;
				break;
		}
		theError = SFPutOpenNav (spec, creator, type, refN, ditlID, &script, defaultSpec, windowTitle, message);
	}
	if (theError)
		return (theError);

	/*
	 * create && open the file
	 */
	if (theError=FSpCreate(spec,creator,type, script))
	{
		if (theError == dupFNErr) theError = noErr;
		else
		{
			FileSystemError(COULDNT_SAVEAS,spec->name,theError);
			return(theError);
		}
	}

	if (theError=FSpOpenDF(spec,fsRdWrPerm,refN))
	{
		FileSystemError(COULDNT_SAVEAS,spec->name,theError);
		FSpDelete(spec);
		return(theError);
	}
	
	if (!(theError=FSpGetFInfo(spec,&info)))
	{
		info.fdType = type;
		FSpSetFInfo(spec,&info);
	}

	if (theError=SetEOF(*refN,0))
	{
		FileSystemError(COULDNT_SAVEAS,spec->name,theError);
		FSpDelete(spec);
		return(theError);
	}
	
	WhackFinder(spec);
	
	return(noErr);	
}

/**********************************************************************
 * FSpModDate - get the mod date of a file
 **********************************************************************/
uLong FSpModDate(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	
	if (FSpGetHFileInfo(spec,&hfi)) return(0);
	return(hfi.hFileInfo.ioFlMdDat);
}

/************************************************************************
 * Snarf - read a whole file into a handle
 ************************************************************************/
OSErr Snarf(FSSpecPtr spec, Handle *hp, long limit)
{
	short refN;
	long bytes;
	short err;
	
	if (!(err=AFSpOpenDF(spec,spec,fsRdPerm,&refN)))
	{
		if (!(err=GetEOF(refN,&bytes)))
		{
			if (limit) bytes = MIN(bytes,limit);
			*hp = NuHTempOK(bytes);
			if (!*hp) err = MemError();
			else if (err=ARead(refN,&bytes,(UPtr)(LDRef(*hp))))
			{
				ZapHandle(*hp);
				*hp = nil;
			}
			else UL(*hp);
		}
		MyFSClose(refN);
	}
	return(err);
}

/************************************************************************
 * SnarfRoman - read a whole file into a handle, and make it roman text if we can
 ************************************************************************/
OSErr SnarfRoman(FSSpecPtr spec, Handle *hp, long limit)
{
	// grab the actual text
	OSErr err = Snarf(spec,hp,limit);
	
	if (!err) err = SniffAndConvertHandleToRoman(hp);
	
	if (err) ZapHandle(*hp);
	
	return err;
}

/************************************************************************
 * Blat - blat a handle out to a text file
 ************************************************************************/
OSErr Blat(FSSpecPtr spec,Handle text,Boolean append)
{
	OSErr err;
	
	LDRef(text);
	err = BlatPtr(spec,*text,GetHandleSize_(text),append);
	UL(text);
	return(err);
}

/************************************************************************
 * BlatPtr - blat text out to a text file
 ************************************************************************/
OSErr BlatPtr(FSSpecPtr spec,Ptr text,long size,Boolean append)
{
	FSSpec newSpec;
	OSErr err;
	short refN;
		
	(void) FSpCreate(spec,DefaultCreator(),'TEXT',smSystemScript);
	
	if (err=AFSpOpenDF(spec,&newSpec,fsRdWrPerm,&refN))
		FileSystemError(TEXT_WRITE,spec->name,err);
	else
	{
		if (append && (err=SetFPos(refN,fsFromLEOF,0)))
			FileSystemError(TEXT_WRITE,spec->name,err);
		if (err=AWrite(refN,&size, (UPtr) text))
			FileSystemError(TEXT_WRITE,spec->name,err);
		else if (err=TruncAtMark(refN))
			FileSystemError(TEXT_WRITE,spec->name,err);
		MyFSClose(refN);
	}
	return(err);
}

/**********************************************************************
 * FileTypeOf - get the type of a file
 **********************************************************************/
OSType FileTypeOf(FSSpecPtr spec)
{
	FInfo info;
	if (!AFSpGetFInfo(spec,spec,&info)) return(info.fdType);
	else return(nil);
}

/**********************************************************************
 * FlushFile - flush a file buffer
 **********************************************************************/
OSErr FlushFile(short refN)
{
	ParamBlockRec pb;
	
	Zero(pb);
	pb.ioParam.ioRefNum = refN;
	return(PBFlushFileSync(&pb));
}

/**********************************************************************
 * MyUpdateResFile - udpate a resource file, and MAKE darn SURE
 **********************************************************************/
OSErr MyUpdateResFile(short resFile)
{
	OSErr err = noErr;
	
	if (GetResFileAttrs(resFile)&mapChanged)
	{
		UpdateResFile(resFile);
		if (!(err=ResError()) && !PrefIsSet(PREF_CORVAIR))
			err = ,  MakeDarnSure(resFile);
	}
	return(err);
}

/**********************************************************************
 * MakeDarnSure - a file is intact on disk
 **********************************************************************/
OSErr MakeDarnSure(short refN)
{
	OSErr err;
	FSSpec spec;
	
	if (!(err = FlushFile(refN)))
		if (!(err = GetFileByRef(refN,&spec)))
			err = FlushVol(nil,spec.vRefNum);
	return(err);
}

/**********************************************************************
 * FileCreatorOf - get the creator of a file
 **********************************************************************/
OSType FileCreatorOf(FSSpecPtr spec)
{
	FInfo info;
	if (!AFSpGetFInfo(spec,spec,&info)) return(info.fdCreator);
	else return(nil);
}

/************************************************************************
 * IsText - is a file of type TEXT or not?
 ************************************************************************/
Boolean IsText(FSSpecPtr spec)
{
	FInfo info;
	FSSpec newSpec;
	short err;
	
	err = AFSpGetFInfo(spec,&newSpec,&info);
	return(!err && info.fdType=='TEXT');	
}

/************************************************************************
 * SanitizeFN - make a filename more palatable
 ************************************************************************/
PStr SanitizeFN(PStr callerShortName,PStr name,short badCharId,short repCharId,Boolean kill8)
{
	short i,j;
	short where;
	Str63 badChars;
	Str63 repChars;
	Str255 newName;
	Str255 shortName;
	UPtr quoteExtensionUnquote = repChars;
	
	GetRString(badChars,badCharId);
	GetRString(repChars,repCharId);
	*newName = MIN(255,*name);
	where = 0;
	for (i=1;i<=*newName;i++)
	{
		if (name[i]<' ')
		{
			if (repChars[1]!=bulletChar) newName[++where] = repChars[1];
			continue;
		}
		else if (kill8 && name[i]>'~')
		{
			if (repChars[2]!=bulletChar) newName[++where] = repChars[2];
			continue;
		}
		else for (j=3;j<=*badChars;j++)
		{
			if (name[i] == badChars[j])
			{
				if (repChars[j]!=bulletChar) newName[++where] = repChars[j];
				goto loop;
			}
		}
		newName[++where] = name[i];
		loop:;
	}
	*newName = where;
	if (newName[1]=='.') newName[1] = '_';	/* initial period bad */
	
	// Stupid filename convention crap that I thought we'd gotten rid of
	// With MacOS...
	i = SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(newName,shortName,quoteExtensionUnquote,31);
	*shortName = MIN(*shortName,i);
	PCopy(callerShortName,shortName);
	if (*quoteExtensionUnquote)
	{
		PCatC(callerShortName,'.');
		PCat(callerShortName,quoteExtensionUnquote);
	}
	
	return(callerShortName);
}

/************************************************************************
 * Mac2OtherName - transmogrify mac name to acceptable outworld name
 ************************************************************************/
PStr Mac2OtherName(PStr other,PStr mac)
{
	Str31 translitted;
	
	PSCopy(translitted,mac);
	TransLitRes(translitted+1,*translitted,ktMacUSHidden);
	return(SanitizeFN(other,translitted,OTHER_FN_BAD,OTHER_FN_REP,true));
}

/************************************************************************
 * ResolveAliasNoMount - resolve an alias, but don't mount any volumes
 ************************************************************************/
OSErr ResolveAliasNoMount(FSSpecPtr alias,FSSpecPtr orig,Boolean *wasAlias)
{
	FInfo info;
	short err;
	FSSpec spec;
	AliasHandle ah;
	short refN;
	short count=1;
	Boolean junk;
	short oldResF = CurResFile();
	
	// If it's a folder, it's not an alias.
	if (FSpIsItAFolder(alias))
	{
		if (*wasAlias) *wasAlias = false;
		if (orig) *orig = *alias;
		return noErr;
	}
	
	/*
	 * is it an alias?
	 */
	if (err=FSpGetFInfo(alias,&info)) return(err);
	
	// This used to just copy alias into orig
	// That meant that the alias fsspec would get
	// turned into the original file.  There are places
	// where that was not helpful.  It may have been relied
	// on elsewhere, however, so we'll preserve
	// that behavior in the case where no orig pointer
	// is given
	if (!orig) orig = alias;
	else *orig = *alias;
	
	if (wasAlias) *wasAlias = (info.fdFlags & kIsAlias)!=0;
	if ((info.fdFlags & kIsAlias)==0) return(noErr);
	
	/*
	 * it's an alias; open it and extract the record
	 */
	if (0>(refN=FSpOpenResFile(alias,fsRdPerm))) return(err);
	ah = GetResource_('alis',0);
	if (ah) DetachResource((Handle)ah);
	CloseResFile(refN);
	UseResFile (oldResF);

	if (!ah) return(resNotFound);

	/*
	 * resolve the record
	 */
	FSMakeFSSpec(Root.vRef,Root.dirId,"\p",&spec);	/* Eudora Folder as base */
	err=MatchAlias(&spec,
											kARMSearch | 					/* allow id search */
											kARMSearchRelFirst |	/* darn fileid's; denigrate */
											kARMNoUI,							/* don't bug the user */
																						/* note we do not specify kARMMountVol */
										  ah,&count,orig,&junk,nil,nil);
	
	ZapHandle(ah);
	return(err);
}

/************************************************************************
 * SpinOn - spin until a return code is not inProgress or cacheFault
 ************************************************************************/
short SpinOnLo(volatile OSErr *rtnCodeAddr,long maxTicks,Boolean allowCancel,Boolean forever,Boolean remainCalm, Boolean allowMouseDown)
{
	long ticks=TickCount();
	long startTicks=ticks+120;
	long now;
#ifdef CTB
	extern ConnHandle CnH;
#endif
	Boolean oldCommandPeriod = CommandPeriod;
	Boolean slow = False;
	static short slowThresh;
	
	if (!slowThresh) slowThresh = GetRLong(SPIN_LENGTH);
		
	if (allowCancel) YieldTicks = 0;
	if (allowCancel || *rtnCodeAddr==inProgress || *rtnCodeAddr==cacheFault)
	{
		CommandPeriod = False;
		do
		{
			now = TickCount();
			if (now>startTicks && now-ticks>slowThresh) {slow = True;if (!InAThread()) CyclePendulum(); else MyYieldToAnyThread();ticks=now;}
			if (slow && !InAThread()) YieldTicks = 0;
			MiniEventsLo((!remainCalm || GetNumBackgroundThreads()) ? 0 : 300, allowMouseDown ? MINI_MASK|mDownMask : MINI_MASK);
			if (CommandPeriod  && !forever) return(userCancelled);
			if (maxTicks && startTicks+maxTicks < now+120) break;
		}
		while (*rtnCodeAddr == inProgress || *rtnCodeAddr == cacheFault);
		if (CommandPeriod) return(userCancelled);
		CommandPeriod = oldCommandPeriod;
	}
	return(*rtnCodeAddr);
}

/**********************************************************************
 * FSpTrash - trash a file
 **********************************************************************/
OSErr FSpTrash(FSSpecPtr spec)
{
	FSSpec trashSpec;
	OSErr err;
	FSSpec exist, newExist;
	
	if (!(err=GetTrashSpec(spec->vRefNum,&trashSpec)))
	{
		if (!FSMakeFSSpec(trashSpec.vRefNum,trashSpec.parID,spec->name,&exist))
		{	
			newExist = exist;
			UniqueSpec(&newExist,31);
			FSpRename(&exist,newExist.name);
		}
		err = SpecMove(spec,&trashSpec);
	}
	return(err);
}

/**********************************************************************
 * UniqueSpec - make a unique filename
 **********************************************************************/
UniqueSpec(FSSpecPtr spec,short max)
{
	short i;
	CInfoPBRec hfi;
	Str31 dfName;
	Str31 dfQuoteExtensionUnquote;
	Str15 iAscii;
	OSErr err;
	
	//
	// Now that we've entered unix-land, we have to pay attention to
	// "extensions".
	//
	max = SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(spec->name,dfName,dfQuoteExtensionUnquote,max);		
	
	for (i=1;i<9999;i++)
	{
		/*
		 * does file exist?
		 */
		err = HGetCatInfo(spec->vRefNum,spec->parID,spec->name,&hfi);
#ifdef NEVER
		if (RunType!=Production) Dprintf("\pHGetCatInfo %p: %d;sc;g",spec->name,err);
#endif
		if (err==fnfErr) return(noErr);
		
		/*
		 * oops.  file exists.  increment number on end of filename
		 */
		PCopy(spec->name,dfName);
		*iAscii = 0;
		PLCat(iAscii,i);
		if (*spec->name + *iAscii < max)
			PCat(spec->name,iAscii);
		else
		{
			BMD(iAscii+1,spec->name+(max+1)-*iAscii,*iAscii);
			*spec->name = max;
		}
		
		//
		// add "extension"
		if (*dfQuoteExtensionUnquote)
		{
			PSCatC(spec->name,'.');
			PSCat(spec->name,dfQuoteExtensionUnquote);
		}
	}
	return(dupFNErr);
}

/**********************************************************************
 * SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote -
 *   we hate Windows
 **********************************************************************/
short SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(PStr name,PStr dfName,PStr dfQuoteExtensionUnquote,short max)
{		
	UPtr spot;
	Str31 localQuoteExtensionUnquote;
	
	PCopy(dfName,name);
	*dfQuoteExtensionUnquote = 0;
	
	if (spot = PRIndex(dfName,'.'))
	{
		// found the last period
		// create the "extension"
		MakePStr(localQuoteExtensionUnquote,spot+1,*dfName-(spot-dfName));
		PCopy(dfQuoteExtensionUnquote,localQuoteExtensionUnquote);
		
		// the "extension" must be nonzero and mustn't be, like, rilly big
		if (*dfQuoteExtensionUnquote && *dfQuoteExtensionUnquote < *dfName-2 && *dfQuoteExtensionUnquote<8)
		{
			*dfName = spot-dfName-1;
			max -= *dfQuoteExtensionUnquote + 1;
		}
		else
			*dfQuoteExtensionUnquote = 0;
	}
	
	return max;
}


/**********************************************************************
 * TweakFileType - set a file's type, and make sure the Finder catches on
 **********************************************************************/
OSErr TweakFileType(FSSpecPtr spec,OSType type,OSType creator)
{
	FInfo info;
	OSErr err;
	FSSpec dirSpec;
	
	/*
	 * get parent info
	 */
	if (!(err=FSpGetFInfo(spec,&info)))
	{
		info.fdType = type;
		info.fdCreator = creator;
		info.fdFlags &= ~kHasBeenInited;
		if (type=='APPL') info.fdFlags |= fHasBundle;
		if (!(err=FSpSetFInfo(spec,&info)))
		{
			if (!(err=FSMakeFSSpec(spec->vRefNum,spec->parID,"\p",&dirSpec)))
				err = FSpTouch(&dirSpec);
		}
	}
	return(err);
}

/**********************************************************************
 * FSpTouch - set a file's mod date to now
 **********************************************************************/
OSErr FSpTouch(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	OSErr err;
	Str31 name;
	
	PSCopy(name,spec->name);
	if (!(err=HGetCatInfo(spec->vRefNum,spec->parID,name,&hfi)))
	{
		hfi.hFileInfo.ioFlMdDat = LocalDateTime();
		PSCopy(name,spec->name);
		err = HSetCatInfo(spec->vRefNum,spec->parID,name,&hfi);
	}
	return(err);
}

/**********************************************************************
 * FSpExists - see if a file exists
 **********************************************************************/
OSErr FSpExists(FSSpecPtr spec)
{
	FInfo info;
	return(FSpGetFInfo(spec,&info));
}

/**********************************************************************
 * FSpRFSane - is a resource file sane?
 **********************************************************************/
OSErr FSpRFSane(FSSpecPtr spec,Boolean *sane)
{
	OSErr err;
	
	err = utl_RFSanity(spec,sane);
	return(err);
}

/**********************************************************************
 * FSpKillRFork - kill the resource fork of a file
 **********************************************************************/
OSErr FSpKillRFork(FSSpecPtr spec)
{
	OSErr err;
	short refN;
	
	if (!(err=FSpOpenRF(spec,fsRdWrPerm,&refN)))
	{
		err = SetEOF(refN,0);
		MyFSClose(refN);
	}
	return err;
}

/************************************************************************
 * AliasFolderType - does alias'es filetype represent a folder?
 ************************************************************************/
Boolean AliasFolderType(OSType type)
{
	OSType types[] = {kExportedFolderAliasType,kContainerServerAliasType,
		kContainerFloppyAliasType,kContainerFolderAliasType,kContainerHardDiskAliasType,
		kMountedFolderAliasType,kSharedFolderAliasType};
	short i = sizeof(types)/sizeof(OSType);
	
	while(i--) if (type==types[i]) return(True);
	return(False);
}

/************************************************************************
 * IsItAFolder - is the specified file a folder?
 ************************************************************************/
Boolean IsItAFolder(short vRef,long inDirId,UPtr name)
{
	CInfoPBRec hfi;
	short err;
	
	hfi.hFileInfo.ioCompletion=nil;
	hfi.hFileInfo.ioNamePtr=name;
	hfi.hFileInfo.ioVRefNum=vRef;
	hfi.hFileInfo.ioDirID=inDirId;
	hfi.hFileInfo.ioFDirIndex=0;
	if (err=PBGetCatInfoSync((CInfoPBPtr)&hfi)) return(false);
	return(0!=(hfi.hFileInfo.ioFlAttrib&0x10));
}

/************************************************************************
 * HFIIsFolder - is this thing a folder?  special case since we have hfi already
 ************************************************************************/
Boolean HFIIsFolder(CInfoPBRec *hfi)
{
	return 0!=(hfi->hFileInfo.ioFlAttrib&0x10);
}

/************************************************************************
 * HFIIsFolderOrAlias - is this thing a folder?  special case since we have hfi already
 ************************************************************************/
Boolean HFIIsFolderOrAlias(CInfoPBRec *hfi)
{
	if (hfi->hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
		return(AliasFolderType(hfi->hFileInfo.ioFlFndrInfo.fdType));
	else
		return HFIIsFolder(hfi);
}

/************************************************************************
 * FolderSizeHi - how big is a folder?
 ************************************************************************/
void FolderSizeHi(short vRef,long dirID,uLong *cumSize)
{
	CInfoPBRec hfi;
	Str63 name;
	
	hfi.hFileInfo.ioNamePtr = name;
	*cumSize = 0;
	
	FolderSize(vRef,dirID,&hfi,cumSize);
	
	return;
}

/************************************************************************
 * FolderSize - how big is a folder?
 ************************************************************************/
void FolderSize(short vRef,long dirID,CInfoPBRec *hfi,uLong *cumSize)
{
	short index;
	
	for (hfi->hFileInfo.ioFDirIndex=index=0;!DirIterate(vRef,dirID,hfi);hfi->hFileInfo.ioFDirIndex=++index)
	{
		if (HFIIsFolder(hfi))
			FolderSize(vRef,hfi->dirInfo.ioDrDirID,hfi,cumSize);
		else
			*cumSize += hfi->hFileInfo.ioFlLgLen + hfi->hFileInfo.ioFlRLgLen;
	}
}

/************************************************************************
 * HGetCatInfo - get cat info for a file?
 ************************************************************************/
short HGetCatInfo(short vRef,long inDirId,UPtr name,CInfoPBRec *hfi)
{
	Zero(*hfi);
	hfi->hFileInfo.ioCompletion=nil;
	hfi->hFileInfo.ioNamePtr=name;
	hfi->hFileInfo.ioVRefNum=vRef;
	hfi->hFileInfo.ioDirID=inDirId;
	if (!name || !*name)
	{
		hfi->hFileInfo.ioFDirIndex = -1;
		hfi->dirInfo.ioDrDirID = inDirId;
	}
	else
		hfi->hFileInfo.ioFDirIndex=0;
	return(PBGetCatInfoSync((CInfoPBPtr)hfi));
}

/************************************************************************
 * HSetCatInfo - get cat info for a file?
 ************************************************************************/
short HSetCatInfo(short vRef,long inDirId,UPtr name,CInfoPBRec *hfi)
{
	hfi->hFileInfo.ioCompletion=nil;
	hfi->hFileInfo.ioNamePtr=name;
	hfi->hFileInfo.ioVRefNum=vRef;
	hfi->hFileInfo.ioDirID=inDirId;
	if (!name || !*name)
	{
		hfi->hFileInfo.ioFDirIndex = -1;
		hfi->dirInfo.ioDrDirID = inDirId;
	}
	else
		hfi->hFileInfo.ioFDirIndex=0;
	return(PBSetCatInfoSync((CInfoPBPtr)hfi));
}

/**********************************************************************
 * ExtractCreatorFromBndl - figure out what an app's creator used to be
 **********************************************************************/
OSErr ExtractCreatorFromBndl(FSSpecPtr spec,OSType *creator)
{
	OSErr err;
	short refN;
	Handle bndl;
	short oldResF = CurResFile();
	
	if (-1!=(refN=FSpOpenResFile(spec,fsRdPerm)))
	{
		if (bndl = GetIndResource('BNDL',1))
		{
			*creator = *(long*)*bndl;
			err = noErr;
		}
		else err = resNotFound;
		CloseResFile(refN);
		UseResFile (oldResF);
	}
	else err = ResError();
	return(err);
}

/************************************************************************
 * AFSpGetCatInfo - cat info, resolving aliases
 ************************************************************************/
short AFSpGetCatInfo(FSSpecPtr spec,FSSpecPtr newSpec,CInfoPBRec *hfi)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(HGetCatInfo(newSpec->vRefNum,newSpec->parID,newSpec->name,hfi));
}

/************************************************************************
 * FolderFileCount - count the files in a folder
 ************************************************************************/
short FolderFileCount(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	FSSpec newSpec;
	
	if (AFSpGetCatInfo(spec,&newSpec,&hfi)) return(-1);
	return(hfi.hFileInfo.ioFlStBlk);
}

/**********************************************************************
 * RemoveDir - remove a directory
 **********************************************************************/
OSErr RemoveDir(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	Str255 name;
	FSSpec die;
	OSErr err = noErr;
	long dirId;
	FSSpec folder;
	
	folder = *spec;
	IsAlias(&folder,&folder);
	dirId = SpecDirId(&folder);
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	
	while (!err)
	{
		hfi.hFileInfo.ioFDirIndex=1;
		hfi.hFileInfo.ioDirID = dirId;
		hfi.hFileInfo.ioVRefNum = folder.vRefNum;
		if (!(err=PBGetCatInfoSync((CInfoPBPtr)&hfi)))
		if (!(err=FSMakeFSSpec(folder.vRefNum,dirId,name,&die)))
			err = FSpDelete(&die);
	}
	return(ChainDelete(spec));
}

/************************************************************************
 * ChainDelete - delete an entire alias chain
 ************************************************************************/
OSErr ChainDelete(FSSpecPtr spec)
{
	FSSpec chain;
	Boolean wasAlias, isFolder;
	
	chain = *spec;
	if (!ResolveAliasFile(&chain,False,&isFolder,&wasAlias) && wasAlias)
		ChainDelete(&chain);
	return(FSpDelete(spec));
}

/************************************************************************
 * Move - Move a file or directory
 ************************************************************************/
short HMove(short vRef,long fromDirId,UPtr fromName,long toDirId,UPtr toName)
{
	CMovePBRec pb;
	
	pb.ioNamePtr = fromName;
	pb.ioDirID = fromDirId;
	pb.ioNewDirID = toDirId;
	pb.ioNewName = toName;
	pb.ioVRefNum = vRef;
				
	return(PBCatMoveSync((CMovePBPtr)&pb));
}


/************************************************************************
 * AHGetFileInfo - get info on a file
 ************************************************************************/
short AHGetFileInfo(short vRef,long dirId,UPtr name,CInfoPBRec *hfi)
{
	short err;
	Str255 newName;
	
	PCopy(newName,name);
	if (err=MyResolveAlias(&vRef,&dirId,newName,nil)) return(err);
	WriteZero(hfi,sizeof(*hfi));
	FILL(hfi->hFileInfo,newName,vRef,dirId);
	return(PBHGetFInfoSync((HParmBlkPtr)hfi));
}

#pragma segment FileUtil2

/************************************************************************
 * FSpGetHFileInfo - get info, don't resolve alias
 ************************************************************************/
OSErr FSpGetHFileInfo(FSSpecPtr spec,CInfoPBRec *hfi)
{
	Str63 name;
	PCopy(name,spec->name);
	Zero(*hfi);
	FILL(hfi->hFileInfo,name,spec->vRefNum,spec->parID);
	return(PBGetCatInfoSync(hfi));
}

short AHSetFileInfo(short vRef,long dirId,UPtr name,CInfoPBRec *hfi)
{
	short err;
	Str255 newName;
	
	PCopy(newName,name);
	if (err=MyResolveAlias(&vRef,&dirId,newName,nil)) return(err);
	FILL(hfi->hFileInfo,newName,vRef,dirId);
	return(PBHSetFInfoSync((HParmBlkPtr)hfi));
}

/************************************************************************
 * I am indebted to Tim Maroney (tim@toad.com) for the following routines.
 ************************************************************************/
static Boolean good, noSys, needWrite, allowFloppy, allowDesktop;

pascal Boolean FolderFilter(FileParam *pb)
{
#pragma unused(pb)
				return true;
}

pascal short FolderItems(short item,DialogPtr dlog)
{
#pragma unused(dlog)

				if (item == 2) {
								good = true;
								item = 3;
				}
				return item;
}

Boolean
GetFolder(char *name,short *volume,long *folder,Boolean writeable,Boolean system,Boolean floppy,Boolean desktop)
{
	Boolean	results;
	
	results = GetFolderNav (name, volume, folder);
	return (results);
}

/************************************************************************
 * CopyFork - copy the resource fork from one file to another
 ************************************************************************/
short CopyFork(short vRef,long dirId,UPtr name,short fromVRef,
								long fromDirId,Uptr fromName,Boolean rFork,Boolean progress)
{
	short err;
	short fromRef,toRef;
	Handle buffer=NuHTempOK(OPTIMAL_BUFFER);
	long bSize=OPTIMAL_BUFFER;
	long eof=0;
	
	if (!buffer) return(MemError());
	LDRef(buffer);
	if (!(err=(rFork?ARFHOpen:AFSHOpen)(fromName,fromVRef,fromDirId,&fromRef,fsRdPerm)))
	{
		if (!(err=(rFork?ARFHOpen:AFSHOpen)(name,vRef,dirId,&toRef,fsRdWrPerm)))
		{
			GetEOF(fromRef,&eof);
			for (bSize=MIN(OPTIMAL_BUFFER,eof);!err&&eof;bSize=MIN(OPTIMAL_BUFFER,eof))
			{
				if (!(err=ARead(fromRef,&bSize,(UPtr)*buffer)))
				{
					if (progress) ByteProgress(nil,-1,bSize);
					eof -= bSize;
					err = AWrite(toRef,&bSize,(UPtr)*buffer);
					if (progress) ByteProgress(nil,-1,bSize);
				}
			}
			TruncAtMark(toRef);
			MyFSClose(toRef);
		}
		MyFSClose(fromRef);
	}
	ZapHandle(buffer);
	return(err);	
}

/**********************************************************************
 * FSpDupFile - duplicate a file
 **********************************************************************/
OSErr FSpDupFile(FSSpecPtr to,FSSpecPtr from,Boolean replace,Boolean progress)
{
	OSErr err;
	Boolean hasRFork = FSpRFSize(from)!=0;
#ifdef DEBUG
	Str255 s;
#endif
	
	if (hasRFork)
	{
		FSpCreateResFile(to,'----','----',smSystemScript);
		err = ResError();
	}
	else 
		err =	FSpCreate(to,'----','----',smSystemScript);
	
	if (err && err==dupFNErr && replace) err = noErr;
#ifdef DEBUG
	if (err)
	{
		ComposeString(s,"\pFSpDupFile: create failed %d.%d.%p; %d",to->vRefNum,to->parID,to->name,err);
		AlertStr(OK_ALRT,Note,s);
	}
#endif
	if (err) return(err);
	
	if (progress) ByteProgress(nil,0,2*(FSpDFSize(to)+FSpRFSize(to)));
	if (!hasRFork || !(err=FSpCopyRFork(to,from,progress)))
	{
		
		if (!(err=FSpCopyDFork(to,from,progress)))
		{
			if (!progress)
				MiniEvents();
			else
				Progress(100,0,nil,nil,nil);
			if (!(err=FSpCopyFInfo(to,from)))
				return(noErr);
		}
#ifdef DEBUG
		else
		{
			ComposeString(s,"\pFSpDupFile: dfork failed %d.%d.%p->%d.%d.%p; %d",from->vRefNum,from->parID,from->name,to->vRefNum,to->parID,to->name,err);
			AlertStr(OK_ALRT,Note,s);
		}
#endif
	}
#ifdef DEBUG
	else if (hasRFork)
	{
		ComposeString(s,"\pFSpDupFile: rfork failed %d.%d.%p->%d.%d.%p; %d",from->vRefNum,from->parID,from->name,to->vRefNum,to->parID,to->name,err);
		AlertStr(OK_ALRT,Note,s);
	}
#endif
	
		
		
	
	FSpDelete(to);
	return err;
}

/**********************************************************************
 * FSpDupFolder - duplicate a folder
 **********************************************************************/
OSErr FSpDupFolder(FSSpecPtr toSpec,FSSpecPtr fromSpec,Boolean replace,Boolean progress)
{
	CInfoPBRec hfi;
	FSSpec	to,from;
	OSErr	err = noErr;
	
	to = *toSpec;
	from = *fromSpec;
	hfi.hFileInfo.ioNamePtr = from.name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!err && !DirIterate(from.vRefNum,from.parID,&hfi))
	{
		PCopy(to.name,from.name);
		if ((hfi.hFileInfo.ioFlAttrib & ioDirMask))
		{
			//	copy folder
			long	saveFrom,saveTo,createdDirID;
			
			//	save current parID's so we can reuse specs
			//	point specs to the folder
			if (!FSpDirCreate(&to,smSystemScript,&createdDirID))
			{
				saveFrom = from.parID; from.parID = SpecDirId(&from);
				saveTo = to.parID; to.parID = createdDirID;
				//	recurse
				err = FSpDupFolder(&to,&from,replace,progress);
				//	restore the parID's
				from.parID = saveFrom;
				to.parID = saveTo;
			}
		}
		else
		{
			//	copy file
			FSpDupFile(&to,&from,replace,progress);
		}
	}
	return err;
}

/************************************************************************
 * CopyFInfo - copy the file info from one file to another
 ************************************************************************/
short CopyFInfo(short vRef,long dirId,UPtr name,short fromVRef,
								long fromDirId,Uptr fromName)
{
	short err;
	CInfoPBRec hfi;
	
	if (!(err=HGetCatInfo(fromVRef,fromDirId,fromName,&hfi)))
	{
		hfi.hFileInfo.ioFlFndrInfo.fdFlags &= ~fInited;	// make the finder move it someplace rational
		Zero(hfi.hFileInfo.ioFlFndrInfo.fdLocation);
		err = HSetCatInfo(vRef,dirId,name,&hfi);
	}
	return(err);
}

/************************************************************************
 * MyResolveAlias - resolve an alias
 ************************************************************************/
short MyResolveAlias(short *vRef,long *dirId,UPtr name,Boolean *wasAlias)
{
	FSSpec theSpec;
	Boolean folder;
	long haveAlias;
	short err=noErr;
	Boolean wasIt;
	
	if (wasAlias) *wasAlias = False;
	if (!Gestalt(gestaltAliasMgrAttr,&haveAlias) && haveAlias&0x1)
	{
		if (!(err=FSMakeFSSpec(*vRef,*dirId,name,&theSpec)) &&
				!(err=ResolveAliasFile(&theSpec,True,&folder,&wasIt)))
		{
			if (wasIt)
			{
				*vRef = theSpec.vRefNum;
				*dirId = theSpec.parID;
				PCopy(name,theSpec.name);
				name[*name+1] = 0;
				if (wasAlias) *wasAlias = True;
			}
		}
	}
	return(err);
}

/**********************************************************************
 * SimpleResolveAlias - resolve an alias without all the fuss
 **********************************************************************/
OSErr SimpleResolveAlias(AliasHandle alias,FSSpecPtr spec)
{
	Boolean junk;
	FSSpec localSpec;
	
	if (!spec) spec = &localSpec;	// allow the caller to pass nil
	Zero(*spec);
	return(ResolveAlias(nil,alias,spec,&junk));
}

/**********************************************************************
 * SimpleResolveAliasNoUI - resolve an alias without bugging the user
 **********************************************************************/
OSErr SimpleResolveAliasNoUI(AliasHandle alias,FSSpecPtr spec)
{
	Boolean junk;
	FSSpec localSpec;
	short justOne = 1;
	
	if (!spec) spec = &localSpec;	// allow the caller to pass nil
	
	Zero(*spec);
	return(MatchAlias(nil,kARMMountVol|kARMNoUI|kARMMultVols|kARMSearch,alias,&justOne,spec,&junk,nil,nil));
}

/************************************************************************
 * ExchangeAndDel - exchange two files, deleting one
 ************************************************************************/
OSErr ExchangeAndDel(FSSpecPtr tmpSpec,FSSpecPtr spec)
{
	short err;
	
	if (err=ExchangeFiles(tmpSpec,spec))
	{
		FileSystemError(TEXT_WRITE,tmpSpec->name,err);
		return(err);
	}
	FSpDelete(tmpSpec);
	return(noErr);
}

/************************************************************************
 * ExchangeFiles - FSpExchangeFiles, with support for dopey AFP servers
 ************************************************************************/
OSErr ExchangeFiles(FSSpecPtr tmpSpec,FSSpecPtr spec)
{
	OSErr err = FSpExchangeFiles(tmpSpec,spec);

	if (err) err = FSpExchangeFilesCompat(tmpSpec,spec);
	return(err);
}

/************************************************************************
 * FSpExchangeFilesCompat - do FSpExchangeFiles if FSpExchangeFiles not supported
 *		from MoreFiles
 ************************************************************************/
OSErr FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest)
{
	HParamBlockRec			pb;
	CInfoPBRec				catInfoSource, catInfoDest;
	OSErr					result;
	Str31					unique1, unique2;
	StringPtr				unique1Ptr, unique2Ptr, swapola;
#ifdef YOU_CARE_MORE_ABOUT_FILEIDS_THAN_FUNCTIONING
	OSErr					result2;
	GetVolParmsInfoBuffer	volInfo;
#endif
	long					theSeed, temp;
	
	/* Make sure the source and destination are on the same volume */
	if ( source->vRefNum != dest->vRefNum )
	{
		result = diffVolErr;
		goto errorExit3;
	}
	
	/* Try PBExchangeFiles first since it preserves the file ID reference */
	pb.fidParam.ioNamePtr = (StringPtr) &(source->name);
	pb.fidParam.ioVRefNum = source->vRefNum;
	pb.fidParam.ioDestNamePtr = (StringPtr) &(dest->name);
	pb.fidParam.ioDestDirID = dest->parID;
	pb.fidParam.ioSrcDirID = source->parID;

	result = PBExchangeFilesSync(&pb);

	/* Note: The compatibility case won't work for files with *Btree control blocks. */
	/* Right now the only *Btree files are created by the system. */
	if ( result != noErr )
	{
		// this code bails if fileid's are supported, because
		// the fileid can't be preserved
		// well, folks, I don't care!  SD 4/13/03
#ifdef YOU_CARE_MORE_ABOUT_FILEIDS_THAN_FUNCTIONING
		pb.ioParam.ioNamePtr = NULL;
		pb.ioParam.ioBuffer = (Ptr) &volInfo;
		pb.ioParam.ioReqCount = sizeof(volInfo);
		result2 = PBHGetVolParmsSync(&pb);
		
		/* continue if volume has no fileID support (or no GetVolParms support) */
		if ( (result2 == noErr) && HasFileIDs(&volInfo) )
		{
			goto errorExit3;
		}
#endif

		/* Get the catalog information for each file */
		/* and make sure both files are *really* files */
		catInfoSource.hFileInfo.ioVRefNum = source->vRefNum;
		catInfoSource.hFileInfo.ioFDirIndex = 0;
		catInfoSource.hFileInfo.ioNamePtr = (StringPtr) &(source->name);
		catInfoSource.hFileInfo.ioDirID = source->parID;
		catInfoSource.hFileInfo.ioACUser = 0; /* ioACUser used to be filler2 */
		result = PBGetCatInfoSync(&catInfoSource);
		if ( result != noErr )
		{
			goto errorExit3;
		}
		if ( (catInfoSource.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )
		{
			result = notAFileErr;
			goto errorExit3;
		}
		
		catInfoDest.hFileInfo.ioVRefNum = dest->vRefNum;
		catInfoDest.hFileInfo.ioFDirIndex = 0;
		catInfoDest.hFileInfo.ioNamePtr = (StringPtr) &(dest->name);
		catInfoDest.hFileInfo.ioDirID = dest->parID;
		catInfoDest.hFileInfo.ioACUser = 0; /* ioACUser used to be filler2 */
		result = PBGetCatInfoSync(&catInfoDest);
		if ( result != noErr )
		{
			goto errorExit3;
		}
		if ( (catInfoDest.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )
		{
			result = notAFileErr;
			goto errorExit3;
		}
		
		/* generate 2 filenames that are unique in both directories */
		theSeed = 0x64666A6C;	/* a fine unlikely filename */
		unique1Ptr = (StringPtr)&unique1;
		unique2Ptr = (StringPtr)&unique2;
		
		result = GenerateUniqueName(source->vRefNum, &theSeed, source->parID, dest->parID, unique1Ptr);
		if ( result != noErr )
		{
			goto errorExit3;
		}

		GenerateUniqueName(source->vRefNum, &theSeed, source->parID, dest->parID, unique2Ptr);
		if ( result != noErr )
		{
			goto errorExit3;
		}

		/* rename source to unique1 */
		pb.fileParam.ioNamePtr = (StringPtr) &(source->name);
		pb.ioParam.ioMisc = (Ptr) unique1Ptr;
		pb.ioParam.ioVersNum = 0;
		result = PBHRenameSync(&pb);
		if ( result != noErr )
		{
			goto errorExit3;
		}
		
		/* rename dest to unique2 */
		pb.ioParam.ioMisc = (Ptr) unique2Ptr;
		pb.ioParam.ioVersNum = 0;
		pb.fileParam.ioNamePtr = (StringPtr) &(dest->name);
		pb.fileParam.ioDirID = dest->parID;
		result = PBHRenameSync(&pb);
		if ( result != noErr )
		{
			goto errorExit2;	/* back out gracefully by renaming unique1 back to source */
		}
			
		/* If files are not in same directory, swap their locations */
		if ( source->parID != dest->parID )
		{
			/* move source file to dest directory */
			pb.copyParam.ioNamePtr = unique1Ptr;
			pb.copyParam.ioNewName = NULL;
			pb.copyParam.ioNewDirID = dest->parID;
			pb.copyParam.ioDirID = source->parID;
			result = PBCatMoveSync((CMovePBPtr) &pb);
			if ( result != noErr )
			{
				goto errorExit1;	/* back out gracefully by renaming both files to original names */
			}
			
			/* move dest file to source directory */
			pb.copyParam.ioNamePtr = unique2Ptr;
			pb.copyParam.ioNewDirID = source->parID;
			pb.copyParam.ioDirID = dest->parID;
			result = PBCatMoveSync((CMovePBPtr) &pb);
			if ( result != noErr)
			{
				/* life is very bad.  We'll at least try to move source back */
				pb.copyParam.ioNamePtr = unique1Ptr;
				pb.copyParam.ioNewName = NULL;
				pb.copyParam.ioNewDirID = source->parID;
				pb.copyParam.ioDirID = dest->parID;
				(void) PBCatMoveSync((CMovePBPtr) &pb);	/* ignore errors */
				goto errorExit1;	/* back out gracefully by renaming both files to original names */
			}
		}
		
		/* Make unique1Ptr point to file in source->parID */
		/* and unique2Ptr point to file in dest->parID */
		/* This lets us fall through to the rename code below */
		swapola = unique1Ptr;
		unique1Ptr = unique2Ptr;
		unique2Ptr = swapola;

		/* At this point, the files are in their new locations (if they were moved) */
		/* Source is named Unique1 (name pointed to by unique2Ptr) and is in dest->parID */
		/* Dest is named Unique2 (name pointed to by unique1Ptr) and is in source->parID */
		/* Need to swap attributes except mod date and swap names */

		/* swap the catalog info by re-aiming the CInfoPB's */
		catInfoSource.hFileInfo.ioNamePtr = unique1Ptr;
		catInfoDest.hFileInfo.ioNamePtr = unique2Ptr;
		
		catInfoSource.hFileInfo.ioDirID = source->parID;
		catInfoDest.hFileInfo.ioDirID = dest->parID;
		
		/* Swap the original mod dates with each file */
		temp = catInfoSource.hFileInfo.ioFlMdDat;
		catInfoSource.hFileInfo.ioFlMdDat = catInfoDest.hFileInfo.ioFlMdDat;
		catInfoDest.hFileInfo.ioFlMdDat = temp;
		
		/* Here's the swap (ignore errors) */
		(void) PBSetCatInfoSync(&catInfoSource); 
		(void) PBSetCatInfoSync(&catInfoDest);
		
		/* rename unique2 back to dest */
errorExit1:
		pb.ioParam.ioMisc = (Ptr) &(dest->name);
		pb.ioParam.ioVersNum = 0;
		pb.fileParam.ioNamePtr = unique2Ptr;
		pb.fileParam.ioDirID = dest->parID;
		(void) PBHRenameSync(&pb);	/* ignore errors */

		/* rename unique1 back to source */
errorExit2:
		pb.ioParam.ioMisc = (Ptr) &(source->name);
		pb.ioParam.ioVersNum = 0;
		pb.fileParam.ioNamePtr = unique1Ptr;
		pb.fileParam.ioDirID = source->parID;
		(void) PBHRenameSync(&pb); /* ignore errors */
	}
errorExit3: { /* null statement */ }
	return ( result );
}

/**********************************************************************
 * HasFileIDs - does volume support file ID's?
 **********************************************************************/
Boolean	HasFileIDs(const GetVolParmsInfoBuffer *volParms)
{
	return ( (volParms->vMAttrib & (1L << bHasFileIDs)) != 0 );
}

/************************************************************************
 * GenerateUniqueName - generates a name that is unique in both dir1 and dir2
 ************************************************************************/
static OSErr GenerateUniqueName(short volume,long *startSeed,long dir1,long dir2,StringPtr uniqueName)
{
	OSErr			error = noErr;
	long			i;
	CInfoPBRec		cinfo;
	unsigned char	hexStr[16];
	
	for ( i = 0; i < 16; ++i )
	{
		if ( i < 10 )
		{
			hexStr[i] = 0x30 + i;
		}
		else
		{
			hexStr[i] = 0x37 + i;
		}
	}
	
	cinfo.hFileInfo.ioVRefNum = volume;
	cinfo.hFileInfo.ioFDirIndex = 0;
	cinfo.hFileInfo.ioNamePtr = uniqueName;

	while ( error != fnfErr )
	{
		(*startSeed)++;		
		cinfo.hFileInfo.ioNamePtr[0] = 8;
		for ( i = 1; i <= 8; i++ )
		{
			cinfo.hFileInfo.ioNamePtr[i] = hexStr[((*startSeed >> ((8-i)*4)) & 0xf)];
		}
		cinfo.hFileInfo.ioDirID = dir1;
		error = fnfErr;
		for ( i = 1; i <= 2; i++ )
		{
			error = error & PBGetCatInfoSync(&cinfo);
			cinfo.hFileInfo.ioDirID = dir2;
			if ( (error != fnfErr) && (error != noErr) )
			{
				return ( error );
			}
		}
	}
	return ( noErr );
}

/**********************************************************************
 * MorphDesktop - if a spec points to the boot disk desktop and the
 *  requested volume is not the desktop, then set the spec to the requested
 *  volume's desktop
 **********************************************************************/
OSErr MorphDesktop(short vRef,FSSpecPtr where)
{
	FSSpec desk;
	OSErr err;
	
	if (*where->name) return(noErr);	// not pointing to folder
	if (SameVRef(vRef,where->vRefNum)) return(noErr);	// same volume
	if (err=FindFolder(kOnSystemDisk,kDesktopFolderType,False,&desk.vRefNum,&desk.parID)) return(err);
	if (SameVRef(vRef,desk.vRefNum)) return(noErr);	// same volume as system
	if (SameVRef(where->vRefNum,desk.vRefNum) && desk.parID==where->parID)
	{
		// ok, spec points to desktop folder
		// point instead to desktop folder on volume
		err=FindFolder(vRef,kDesktopFolderType,False,&where->vRefNum,&where->parID);
	}
	return(err);
}

/************************************************************************
 * AFSpIsItAFolder - is a file a folder?
 ************************************************************************/
Boolean AFSpIsItAFolder(FSSpecPtr spec)
{
	FInfo info;
	
	FSpGetFInfo(spec,&info);
	if ((info.fdFlags & kIsAlias) && info.fdType==kContainerFolderAliasType)
		return(True);
	return(FSpIsItAFolder(spec));
}

/************************************************************************
 * FSWriteP - write a Pascal string
 ************************************************************************/
short FSWriteP(short refN,UPtr pString)
{
	long count = *pString;
	return(AWrite(refN,&count,pString+1));
}

/************************************************************************
 * GetFileByRef - figure out the name & vol of a file from an open file
 ************************************************************************/
short GetFileByRef(short refN,FSSpecPtr specPtr)
{
	FCBPBRec fcb;
	short err;
	Str63 name;
	
	fcb.ioCompletion = nil;
	fcb.ioVRefNum = nil;
	fcb.ioRefNum = refN;
	fcb.ioFCBIndx = 0;
	fcb.ioNamePtr = name;
	if (err=PBGetFCBInfo(&fcb,False)) return(err);
	return(FSMakeFSSpec(fcb.ioFCBVRefNum,fcb.ioFCBParID,name,specPtr));
}

/************************************************************************
 * VolumeFree - return the free space on a volume
 ************************************************************************/
long VolumeFree(short vRef)
{
	HParamBlockRec pb;
	
	pb.volumeParam.ioNamePtr = nil;
	pb.volumeParam.ioVolIndex = 0;
	pb.volumeParam.ioVRefNum = vRef;
	if (PBHGetVInfoSync((HParmBlkPtr)&pb)) return(0);
	return(pb.volumeParam.ioVFrBlk*pb.volumeParam.ioVAlBlkSiz);
}

/************************************************************************
 * FSTabWrite - write, expanding tabs
 ************************************************************************/
short FSTabWrite(short refN,long *count,UPtr buf)
{
	UPtr p;
	UPtr end = buf+*count;
	long written=0;
	short err=noErr;
	long writing;
	static short charsOnLine=0;
	UPtr nl;
	short stops=0;
	
	if (!FakeTabs) return(FSZWrite(refN,count,buf));
	for (p=buf;p<end;p=buf=p+1)
	{
		nl = buf-charsOnLine-1;
		while (p<end && *p!=tabChar)
		{
			if (*p=='\015') nl=p;
			p++;
		}
		writing = p-buf;
		err=FSZWrite(refN,&writing,buf);
		written += writing;
		if (err) break;
		charsOnLine = p-nl-1;
		if (p<end)
		{
			if (!stops) stops = GetRLong(TAB_DISTANCE);
			writing = stops-(charsOnLine)%stops;
			charsOnLine = 0;
			err = FSZWrite(refN,&writing, (UPtr) "              ");
			written += writing;
			if (err) break;
		}
	}
	*count = written;
	return(err);
}

/**********************************************************************
 * AWrite - write async
 **********************************************************************/
short ARead(short refN,long *count,UPtr buf)
{
	OSErr err;
	ParamBlockRec pb;
	
#ifdef TWO
	if (!SyncRW)
	{
		Zero(pb);
		pb.ioParam.ioRefNum = refN;
		pb.ioParam.ioBuffer = (char *) buf;
		pb.ioParam.ioReqCount = *count;
		*count = 0;
		
#ifdef NEVER
		if (VM) HoldMemory(&pb,sizeof(pb));
		if (VM) HoldMemory(buf,*count);
#endif
		PBReadAsync(&pb);
		// (jp) 'ioResult' is now volatile in Universal Headers 3.4
		err = SpinOn(&pb.ioParam.ioResult,0,False,True);
		*count = pb.ioParam.ioActCount;
#ifdef NEVER
		if (VM) UnholdMemory(&pb,sizeof(pb));
		if (VM) UnholdMemory(buf,*count);
#endif
	}
	else
#endif
		err = FSRead(refN,count,buf);

	return(err);
}

/************************************************************************
 * NCWriteP - write a Pascal string
 ************************************************************************/
short NCWriteP(short refN,UPtr pString)
{
	long count = *pString;
	return(NCWrite(refN,&count,pString+1));
}

/************************************************************************
 * AWriteP - write a Pascal string
 ************************************************************************/
short AWriteP(short refN,UPtr pString)
{
	long count = *pString;
	return(AWrite(refN,&count,pString+1));
}

/**********************************************************************
 * AWrite - write async
 **********************************************************************/
short AWrite(short refN,long *count,UPtr buf)
{
	OSErr err;
	ParamBlockRec pb;
	
#ifdef TWO
	if (!*count) return(noErr);
	if (!SyncRW)
	{
		Zero(pb);
		pb.ioParam.ioRefNum = refN;
		pb.ioParam.ioBuffer = (char *) buf;
		pb.ioParam.ioReqCount = *count;
		
#ifdef NEVER
		if (VM) HoldMemory(&pb,sizeof(pb));
		if (VM) HoldMemory(buf,*count);
#endif
		PBWriteAsync(&pb);
		err = SpinOn(&pb.ioParam.ioResult,0,False,True);
		*count = pb.ioParam.ioActCount;
#ifdef NEVER
		if (VM) UnholdMemory(&pb,sizeof(pb));
		if (VM) UnholdMemory(buf,*count);
#endif
	}
	else
#endif
		err = FSWrite(refN,count,buf);
	
	return(err);
}

/**********************************************************************
 * WipeSpec - wipe a file
 **********************************************************************/
OSErr WipeSpec(FSSpecPtr spec)
{
	short refN;
	long eof;
	OSErr err;
	
	if (!(err=FSpOpenDF(spec,fsRdWrPerm,&refN)))
	{
		if (!(err=GetEOF(refN,&eof)))
			err=WipeDiskArea(refN,0,eof);
		MyFSClose(refN);
		if (!(err=FSpOpenRF(spec,fsRdWrPerm,&refN)))
		{
			if (!(err=GetEOF(refN,&eof)))
				err=WipeDiskArea(refN,0,eof);
			MyFSClose(refN);
		}
	}
	if (!err)
	{
		FlushVol(nil,spec->vRefNum);
		err = FSpDelete(spec);
	}
	
	if (err && err!=fnfErr) FileSystemError(WIPE_ERROR,spec->name,err);
	
	return(err);
}

/**********************************************************************
 * WipeDiskArea - wipe part of a disk
 **********************************************************************/
OSErr WipeDiskArea(short refN,long offset, long len)
{
	long bSize = MIN(len,OPTIMAL_BUFFER);
	UHandle h;
	long size;
	UPtr spot,end;
	OSErr err;
	
	if (!bSize) return(noErr);
	h = (UHandle) NuHTempBetter(bSize);
	if (!h) return(MemError());
	
	/*
	 * fill it with returns
	 */
	end = LDRef(h) + bSize;
	for (spot=*h;spot<end;spot++) *spot = ' ';
	spot[-1] = '\015';
	for (spot=*h;spot<end;spot+=50) *spot = '\015';
	
	/*
	 * blat it over the disk area
	 */
	if (!(err=SetFPos(refN,fsFromStart,offset)))
		for (size=bSize;len;size=MIN(bSize,len))
		{
			err = NCWrite(refN,&size,*h);
			if (err) break;
			len -= size;
		}
	
	ZapHandle(h);
	return(err);
}


/**********************************************************************
 * NCWrite - Write, avoiding the cache
 **********************************************************************/
short NCWrite(short refN,long *count,UPtr buf)
{
	OSErr err;
	ParamBlockRec pb;
	
	if (SyncRW) return(FSWrite(refN,count,buf));
	Zero(pb);
	pb.ioParam.ioRefNum = refN;
	pb.ioParam.ioBuffer = (char *) buf;
	pb.ioParam.ioReqCount = *count;
	pb.ioParam.ioPosMode = 1<<5;	/* no cache */
	
#ifdef NEVER
	if (VM) HoldMemory(&pb,sizeof(pb));
	if (VM) HoldMemory(buf,*count);
#endif
	PBWriteAsync(&pb);
	err = SpinOn(&pb.ioParam.ioResult,0,False,True);
	*count = pb.ioParam.ioActCount;
#ifdef NEVER
	if (VM) UnholdMemory(&pb,sizeof(pb));
	if (VM) UnholdMemory(buf,*count);
#endif
	return(err);
}


/************************************************************************
 * EnsureNewline - make sure there is a newline at or just before the
 * current file position.
 ************************************************************************/
OSErr EnsureNewline(short refN)
{
	Str15 chars;
	long offset,count;
	short err;
	
	/*
	 * where are we?
	 */
	if (err=GetFPos(refN,&offset)) return(err);
	
	/*
	 * BOF counts as newline
	 */
	if (!offset) return(noErr);
	
	/*
	 * back up one character
	 */
	if (err=SetFPos(refN,fsFromStart,offset-1)) return(err);
	
	/*
	 * read it
	 */
	count = 1;
	if (err=ARead(refN,&count,chars)) return(err);
	
	/*
	 * is newline?
	 */
	if (*chars == '\015') return(noErr);
	
	/*
	 * make it so
	 */
	return(FSWriteP(refN,Cr));
}

/************************************************************************
 * AFSpOpenDF - OpenDF, but resolve the alias first
 ************************************************************************/
OSErr AFSpOpenDF (FSSpecPtr spec,FSSpecPtr newSpec, SignedByte permission, short *refNum)
{
	OSErr err;
	Boolean folder, wasIt;
	short localRef;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	err = FSpOpenDF(newSpec,permission,&localRef);
	if (!err) *refNum = localRef;
	return err;
}


/************************************************************************
 * AFSpOpenRF
 ************************************************************************/
OSErr AFSpOpenRF (FSSpecPtr spec,FSSpecPtr newSpec, SignedByte permission, short *refNum)
{
	OSErr err;
	Boolean folder, wasIt;
	short localRef;
	
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	err = FSpOpenRF(newSpec,permission,&localRef);
	if (!err) *refNum = localRef;
	return err;
}

/************************************************************************
 * AFSpDelete
 ************************************************************************/
OSErr AFSpDelete(FSSpecPtr spec,FSSpecPtr newSpec)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(FSpDelete(newSpec));
}

/************************************************************************
 * AFSpGetFInfo
 ************************************************************************/
OSErr AFSpGetFInfo (FSSpecPtr spec,FSSpecPtr newSpec, FInfo *fndrInfo)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(FSpGetFInfo (newSpec,fndrInfo));
}

/************************************************************************
 * FSpFileSize - get the size of a file
 ************************************************************************/
long FSpFileSize(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	
	if (!AFSpGetHFileInfo(spec,&hfi)) return(hfi.hFileInfo.ioFlLgLen+hfi.hFileInfo.ioFlRLgLen);
	else return(0);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AFSpSetMod(FSSpecPtr spec,uLong mod)
{
	CInfoPBRec hfi;
	OSErr err;
	
	if (err=AFSpGetHFileInfo(spec,&hfi)) return(err);
	hfi.hFileInfo.ioFlMdDat = mod;
	return(AFSpSetHFileInfo(spec,&hfi));
}

/**********************************************************************
 * 
 **********************************************************************/
uLong AFSpGetMod(FSSpecPtr spec)
{
	CInfoPBRec hfi;

	if (!AFSpGetHFileInfo(spec,&hfi)) return(hfi.hFileInfo.ioFlMdDat);
	else return(0);
}

/************************************************************************
 * FSpDFSize - get the size of the data fork a file
 ************************************************************************/
long FSpDFSize(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	
	if (!AFSpGetHFileInfo(spec,&hfi)) return(hfi.hFileInfo.ioFlLgLen);
	else return(0);
}

/************************************************************************
 * FSpRFSize - get the size of the data fork a file
 ************************************************************************/
long FSpRFSize(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	
	if (!AFSpGetHFileInfo(spec,&hfi)) return(hfi.hFileInfo.ioFlRLgLen);
	else return(0);
}

/************************************************************************
 * FSpSetFXInfo - set the FXInfo for a file
 ************************************************************************/
OSErr FSpSetFXInfo(FSSpecPtr spec,FXInfo *fxInfo)
{
	OSErr err;
	CInfoPBRec hfi;
	
	if (!(err=AFSpGetHFileInfo(spec,&hfi)))
	{
		hfi.hFileInfo.ioFlXFndrInfo = *fxInfo;
		err = HSetCatInfo(spec->vRefNum,spec->parID,spec->name,&hfi);
	}
	return(err);
}


/************************************************************************
 * AFSpSetFInfo
 ************************************************************************/
OSErr AFSpSetFInfo (FSSpecPtr spec,FSSpecPtr newSpec, FInfo *fndrInfo)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(FSpSetFInfo (newSpec,fndrInfo));
}

/************************************************************************
 * AFSpSetFLock
 ************************************************************************/
OSErr AFSpSetFLock (FSSpecPtr spec,FSSpecPtr newSpec)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(FSpSetFLock(newSpec));
}

/************************************************************************
 * AFSpRstFLock
 ************************************************************************/
OSErr AFSpRstFLock (FSSpecPtr spec,FSSpecPtr newSpec)
{
	OSErr err;
	Boolean folder, wasIt;
	*newSpec = *spec;
	if (err=ResolveAliasFile(newSpec,True,&folder,&wasIt)) return(err);
	
	return(FSpRstFLock(newSpec));
}

/************************************************************************
 * IsAlias - is a file an alias?
 ************************************************************************/
Boolean IsAlias(FSSpecPtr spec,FSSpecPtr newSpec)
{
	Boolean folder, wasIt=false;
	*newSpec = *spec;
	ResolveAliasFile(newSpec,True,&folder,&wasIt);	// if error, wasIt will either
																									// be set correctly or else still
																									// be false
	return(wasIt);
}

/************************************************************************
 * IsAliasNoMount - is a file an alias (but don't mount it if so)?
 ************************************************************************/
Boolean IsAliasNoMount(FSSpecPtr spec,FSSpecPtr newSpec)
{
	Boolean isAlias=false;
	*newSpec = *spec;
	
	ResolveAliasNoMount(spec,newSpec,&isAlias);
	
	return(isAlias);
}

/************************************************************************
 * ResolveAliasOrElse - Resolve an alias or fail
 ************************************************************************/
OSErr ResolveAliasOrElse(FSSpecPtr spec,FSSpecPtr newSpec,Boolean *wasIt)
{
	Boolean folder, isAlias=false;
	OSErr err;
	FSSpec resolvedSpec = *spec;
	err = ResolveAliasFile(&resolvedSpec,true,&folder,&isAlias);	// if error, isAlias will either
																													// be set correctly or else still
																													// be false
	if (wasIt) *wasIt = isAlias;
	if (newSpec) *newSpec =  err ? *spec : resolvedSpec;	// if error, put original file there
	return(err);
}

/**********************************************************************
 * SubFolderSpec - get the FSSpec for the signature folder
 **********************************************************************/
OSErr SubFolderSpec(short nameId,FSSpecPtr spec)
{
	Str63 string;
	OSErr err;
	static StackHandle specStack;
	CSpec cSpec;
	short i;
	
	// clear cache?
	if (!spec)
	{
		if (specStack) (*specStack)->elCount = 0;
		return noErr;
	}
	
	// search for folder in cache
	if (specStack)
		for (i=0;i<(*specStack)->elCount;i++)
		{
			StackItem(&cSpec,i,specStack);
			if (cSpec.count == nameId)
			{
				*spec = cSpec.spec;
				return noErr;
			}
		}
	
	// not in cache.  Go look for it
	if (!(err=FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(string,nameId),spec)))
	{
		/*
		 * maybe the folder is an alias
		 */
		IsAlias(spec,spec);
		
		/*
		 * point inside the folder
		 */
		spec->parID = SpecDirId(spec);
		*spec->name = 0;
		
		/*
		 * cache it, now that we have it
		 */
		if (specStack || !StackInit(sizeof(CSpec),&specStack))
		{
			cSpec.spec = *spec;
			cSpec.count = nameId;
			StackPush(&cSpec,specStack);
		}
	}
	return(err);
}

/************************************************************************
 * FindSubFolderSpec - Find our sub folder of a specific system folder
 ************************************************************************/
OSErr FindSubFolderSpec(long domain,long folder,short subfolderID,Boolean create,FSSpecPtr spec)
{
	FSSpec localSpec;
	OSErr err = FindFolder(domain,folder,create,&localSpec.vRefNum,&localSpec.parID);
	
	if (!err)
		err = SubFolderSpecOf(&localSpec,subfolderID,create,spec);
	
	return err;
}

/************************************************************************
 * SubFolderSpecOf - find a subfolder of a given fsspec
 ************************************************************************/
OSErr SubFolderSpecOf(FSSpecPtr inSpec,short subfolderID,Boolean create,FSSpecPtr subSpec)
{
	Str63 subfolderName;
	
	GetRString(subfolderName,subfolderID);
	return SubFolderSpecOfStr(inSpec,subfolderName,create,subSpec);
}

OSErr SubFolderSpecOfStr(FSSpecPtr inSpec,PStr subfolderName,Boolean create,FSSpecPtr subSpec)
{
	FSSpec localSpec = *inSpec;
	long dirID;
	
	PCopy(localSpec.name,subfolderName);
	if (create) FSpDirCreate(&localSpec,smSystemScript,&dirID);
	
	IsAlias(&localSpec,&localSpec);
	localSpec.parID = SpecDirId(&localSpec);
	if (localSpec.parID==0) return fnfErr;
	if (subSpec)
	{
		*localSpec.name = 0;
		*subSpec = localSpec;
	}
	return noErr;
}

/************************************************************************
 * StuffFolderSpec - find the stuff folder
 ************************************************************************/
OSErr StuffFolderSpec(FSSpecPtr spec)
{
	FSSpec localSpec;
	Str31 name;
	OSErr err = GetFileByRef(AppResFile,&localSpec);
	
	if (!err) err = FSMakeFSSpec(localSpec.vRefNum,localSpec.parID,GetRString(name,STUFF_FOLDER),&localSpec);
	if (!err)
	{
		IsAlias(&localSpec,&localSpec);
		spec->vRefNum = localSpec.vRefNum;
		spec->parID = SpecDirId(&localSpec);
		*spec->name = 0;
	}
	return err;
}

/************************************************************************
 * SpecInSubfolderOf - is a spec in a folder or subfolder
 ************************************************************************/
Boolean SpecInSubfolderOf(FSSpecPtr att,FSSpecPtr folder)
{
	FSSpec parent = *att;
	
	for(;;)
	{
		if (SameVRef(parent.vRefNum,folder->vRefNum) && parent.parID==folder->parID)
			return(true);
		if (parent.parID==2) return(false);
		if (ParentSpec(&parent,&parent)) return(false);
		
	}
}




/************************************************************************
 * FSMakeFID - make a fileid for a spec
 ************************************************************************/
OSErr FSMakeFID(FSSpecPtr spec,long *fid)
{
	HParamBlockRec fidpb;
	short err;
	
	Zero(fidpb);

	fidpb.fidParam.ioCompletion = nil;
	fidpb.fidParam.ioNamePtr = spec->name;
	fidpb.fidParam.ioVRefNum = spec->vRefNum;
	fidpb.fidParam.ioSrcDirID = spec->parID;
	
	err = PBCreateFileIDRefSync((HParmBlkPtr)&fidpb);
	FileIDHack();
	if (err==fidExists || err==afpIDExists) err = noErr;	/* ignore; ioFileID is good */
	
	if (!err) *fid = fidpb.fidParam.ioFileID;
	return(err);
}

/************************************************************************
 * FileIDHack - hack to work around apple fileID bug.
 *	 JDB 980720 
 *
 *		There's a problem with OS 8.1 machines that create fileIDs for
 *	files saved onto standard HFS partitions.  If a PBSetFInfo is done
 *	too soon afterwards (like happens with SetMod later during attachment
 *	receiving), the fileID can become corrupt and the attachments can;t be
 *	located agan.
 *  SD 8/6/98 - The workaround is to GetCatInfo on a different file.
 ************************************************************************/
void FileIDHack(void)
{
	long sysVers = 0;
	long affected = 0;
	OSErr err = noErr;
	FSSpec spec;
	CInfoPBRec info;
	
	// get the system version of this machine
	err = Gestalt(gestaltSystemVersion, &sysVers);
	if ((err == noErr))
	{
		// is this system version affected by the bug?
		affected = GetRLong(FILEID_AFFECTED_SYSVERSION);
		if (affected == sysVers)
		{
			Zero(spec);
			GetFileByRef(SettingsRefN,&spec);
			AFSpGetCatInfo(&spec,&spec,&info);
		}
	}
}

/************************************************************************
 * FSResolveFID - resolve a vRef & fileid into a spec
 ************************************************************************/
OSErr FSResolveFID(short vRef,long fid,FSSpecPtr spec)
{
	HParamBlockRec fidpb;
	short err;
	Str63 name;

	Zero(fidpb);
	
	*name = 0;
	fidpb.fidParam.ioCompletion = nil;
	fidpb.fidParam.ioNamePtr = name;
	fidpb.fidParam.ioFileID = fid;
	fidpb.fidParam.ioVRefNum = vRef;
	
	err = PBResolveFileIDRefSync((HParmBlkPtr)&fidpb);
	
	if (!err) err = FSMakeFSSpec(vRef,fidpb.fidParam.ioSrcDirID,name,spec);
	
	return(err);
}

/************************************************************************
 * SpecMove - move a file from one place to another
 ************************************************************************/
OSErr SpecMove(FSSpecPtr moveMe,FSSpecPtr moveTo)
{
	FInfo info;
	
	if (!FSpGetFInfo(moveMe,&info))
	{
		info.fdFlags &= ~fInited;
		Zero(info.fdLocation);
		FSpSetFInfo(moveMe,&info);
	}
	return HMove(moveMe->vRefNum,moveMe->parID,moveMe->name,moveTo->parID,nil);
}

/************************************************************************
 * SpecMoveAndRename - move a file from one place to another, and rename
 ************************************************************************/
OSErr SpecMoveAndRename(FSSpecPtr moveMe,FSSpecPtr moveTo)
{
	OSErr err;
	
	if (err = SpecMove(moveMe,moveTo))
	  return err;
	
	if (err = HRename(moveTo->vRefNum,moveTo->parID,moveMe->name,moveTo->name))
	{
		OSErr undoErr = HMove(moveTo->vRefNum,moveTo->parID,moveMe->name,moveMe->parID,nil);
		ASSERT(!undoErr);	// this would be a bad place to error, as it
											// would mean we couldn't move the file back
	}
	
	return err;
}

/************************************************************************
 * DiskSpunUp - is the disk a'spinnin'?
 ************************************************************************/
Boolean DiskSpunUp(void)
{
	// They killed HardDiskPowered -- Those bxxxxxxs!!
	// Even worse, they made it return false when they killed it, not true
	// They should go to hell.  They should go to hell and they should die.
	// if (HardDiskPowered && !HardDiskPowered()) return false;
	return true;
}

/************************************************************************
 * GetTrashSpec - get an FSSpec describing the trash;
 ************************************************************************/
OSErr GetTrashSpec(short vRef,FSSpecPtr spec)
{
	spec->name[0] = 0;
	spec->vRefNum = vRef;
	return(FindFolder(vRef,kTrashFolderType,kCreateFolder,&vRef,&spec->parID));
}

/************************************************************************
 * DTRef - return the ref number for the desktop db
 ************************************************************************/
OSErr DTRef(short vRef, short *dtRef)
{
	DTPBRec pb;
	short err;
	
	pb.ioNamePtr = nil;
	pb.ioVRefNum = vRef;
	pb.ioCompletion = nil;
	
	if (err=PBDTGetPath(&pb)) return(err);
	
	*dtRef = pb.ioDTRefNum;
	return(pb.ioResult);
}

/************************************************************************
 * DTGetAppl - find an app for a creator
 ************************************************************************/
OSErr DTGetAppl(short vRef,short dtRef,OSType creator,FSSpecPtr appSpec)
{
	DTPBRec pb;
	short err;
	Str63 appName;
	
	pb.ioCompletion = nil;
	pb.ioNamePtr = appName;
	pb.ioDTRefNum = dtRef;
	pb.ioIndex = 0;
	pb.ioFileCreator = creator;
	
	if (err=PBDTGetAPPL(&pb,False)) return(err);
	if (pb.ioResult) return(pb.ioResult);
	
	return(FSMakeFSSpec(vRef,pb.ioAPPLParID,appName,appSpec));
}

/************************************************************************
 * DTFindAppl - find which dtRef an application lives in
 ************************************************************************/
short DTFindAppl(OSType creator)
{
	OSErr err;
	short volIndex;
	short vRef;
	short dtRef;
	FSSpec junk;
	
	for (volIndex=1;!IndexVRef(volIndex,&vRef);volIndex++)
		if (!(err=DTRef(vRef,&dtRef)))
			if (!(err=DTGetAppl(vRef,dtRef,creator,&junk)))
				return(dtRef);
	return(0);
}

/************************************************************************
 * DTSetComment - set the comment for an attachment
 ************************************************************************/
OSErr DTSetComment(FSSpecPtr spec,PStr comment)
{
	DTPBRec pb;
	short dtRef;
	OSErr err;
	
	if (!HaveTheDiseaseCalledOSX() && !(err=DTRef(spec->vRefNum,&dtRef)))
	{
		Zero(pb);
		pb.ioNamePtr = spec->name;
		pb.ioDTRefNum = dtRef;
		pb.ioDTBuffer = (char *) comment+1;
		pb.ioDTReqCount = MIN(200,*comment);
		pb.ioDirID = spec->parID;
		return(PBDTSetCommentSync(&pb));
	}
	return(err);
}

/************************************************************************
 * SameSpec - do two specs refer to same file?
 ************************************************************************/
Boolean SameSpec(FSSpecPtr sp1,FSSpecPtr sp2)
{
	return(sp1->parID==sp2->parID && SameVRef(sp1->vRefNum,sp2->vRefNum) &&
				 StringSame(sp1->name,sp2->name));
}

/************************************************************************
 * SpecDirId - find the dirId of the directory referenced by a spec 
 ************************************************************************/
long SpecDirId(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	FSSpec newSpec;
	Str31 name;
	
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	AFSpGetCatInfo(spec,&newSpec,&hfi);
	return(hfi.hFileInfo.ioDirID);
}

/************************************************************************
 * CanWrite - can we write on a file?  Only one way to tell on a macintosh
 ************************************************************************/
OSErr CanWrite(FSSpecPtr spec, Boolean *can)
{
	FSSpec newSpec = *spec;
	short refN;
	Byte buff=13;
	long len;
	CInfoPBRec hfi;
	OSErr err;
	Boolean b;
	
	*can = False;
	if (!(err = ResolveAliasFile(&newSpec,True,&b,&b)) && 
			!(err=AFSpGetHFileInfo(&newSpec,&hfi)))
	if (!(err=FSpOpenDF(&newSpec,fsRdWrPerm,&refN)))
	{
		len = 1;
		if (!(err=SetFPos(refN,fsFromLEOF,0)))
			if (!FSWrite(refN,&len,&buff))
			{
				*can = True;
				SetFPos(refN,fsFromLEOF,-1);
				if (!GetFPos(refN,&len)) TruncOpenFile(refN,len);
			}
		MyFSClose(refN);
		AFSpSetHFileInfo(&newSpec,&hfi);	/* restore mod date */
	}
	else if ((err==permErr||err==afpAccessDenied) && !(err=FSpOpenDF(&newSpec,fsRdPerm,&refN)))
	{
		MyFSClose(refN);
		*can = False;
	}
	return(err);
}

#ifdef DEBUG
/**********************************************************************
 * MyFSClose - call FSClose
 **********************************************************************/
OSErr MyFSClose(short refN)
{
	if (!PrefIsSet(PREF_CORVAIR)) MakeDarnSure(refN);
	return(FSClose(refN));
}
#endif

/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
OSErr NewTempSpec(short vRef,long dirId,PStr name,FSSpecPtr spec)
{
	long tempId;
	OSErr err;
	Str31 fName;
	static unsigned char n;

	if (err = FindTemporaryFolder(vRef,dirId,&tempId,&vRef)) return err;
	
	n++;
	
	if (name) PCopy(fName,name);
	else
	{
		NumToString(TickCount(),fName);
		PCatC(fName,'+');
		PLCat(fName,n);
	}
	
	err = FSMakeFSSpec(vRef,tempId,fName,spec);
	return(UniqueSpec(spec,27));
}

/**********************************************************************
 * FindTemporaryFolder - find the Temporary Folder
 *	use spool folder if not available on server
 **********************************************************************/
OSErr FindTemporaryFolder(short vRef,long dirId,long *tempDirId,short *tempVRef)
{
	OSErr err = noErr;
	
//	tell FindFolder to forget everything it knows, and look at the disk
	err = InvalidateFolderDescriptorCache ( 0, 0L );
	ASSERT ( noErr == err );
	err = FindFolder(vRef,kTemporaryFolderType,True,tempVRef,tempDirId);
	
#ifdef DEBUG
	// Some versions of OS X will return noErr and a bad value for the temp folder
	// if it existed once, but does no longer. So, we check to see if it exists.
	//	*** Darn their eyes! ***
	if ( noErr == err )
	{
		CInfoPBRec pb;
		
		Zero ( pb );
		pb.dirInfo.ioFDirIndex	= -1;		// use ioVRefNum and ioDrDirID only
		pb.dirInfo.ioVRefNum 	= *tempVRef;
		pb.dirInfo.ioDrDirID	= *tempDirId;
		err = PBGetCatInfoSync ( &pb );
		ASSERT ( noErr == err );
	}
#endif
		
	// If findfolder fails, or if it returns a different volume, use
	// the spool folder
	if (err || *tempVRef!=vRef)
	{
		err = noErr;
		*tempVRef = vRef;
		if (dirId) *tempDirId = dirId;
		else
		{
			FSSpec netbootSucksSpec;
			
			if (SubFolderSpec(SPOOL_FOLDER,&netbootSucksSpec) || netbootSucksSpec.vRefNum!=vRef)
				*tempDirId = 2;
			else
				*tempDirId = netbootSucksSpec.parID;
		}
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AddUniqueExt(FSSpecPtr spec,short extId)
{
	FSSpec newSpec;
	short n = 0;
	Str31 extStr, nStr;
	OSErr err;
	
	PCopy(extStr,"\p.");
	PCatR(extStr,extId);
	*nStr = 0;
	
	for (;;)
	{
		newSpec = *spec;
		*newSpec.name = MIN(*newSpec.name,31-*extStr-*nStr);
		if (n++) PCat(newSpec.name,nStr);
		PCat(newSpec.name,extStr);
		if (err=FSpExists(&newSpec)) break;
		NumToString(n,nStr);
	}
	
	if (err==fnfErr)
	{
		err = FSpRename(spec,newSpec.name);
		if (!err) PCopy(spec->name,newSpec.name);
	}
	
	return(err);
}
		
/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
OSErr NewTempExtSpec(short vRef,PStr name,short extId,FSSpecPtr spec)
{
	long dirId;
	OSErr err = FindTemporaryFolder(vRef,0L,&dirId,&vRef);
	Str31 fName;
	static short n;
	
	if (err) return(err);
	
	do
	{
		n++;
		if (n>REAL_BIG) n = 0;
		
		if (name) PCopy(fName,name);
		else
		{
			NumToString(TickCount(),fName);
			PCatC(fName,'+');
			PLCat(fName,n);
		}
		PCatC(fName,'.');
		PCatR(fName,extId);
		
	}
	while (!FSMakeFSSpec(vRef,dirId,fName,spec));
	if (err==fnfErr) err = noErr;
	return(err);
}

/************************************************************************
 * MakeAFinderAlias - make an alias to a file
 ************************************************************************/
OSErr MakeAFinderAlias(FSSpecPtr originalSpec,FSSpecPtr aliasSpec)
{
	AliasHandle alias=nil;
	short err;
	FSSpec spec, localSpec;
	FInfo origInfo,aliasInfo;
	short refN;
	short oldResF = CurResFile();
	
	err = FSMakeFSSpec(aliasSpec->vRefNum,aliasSpec->parID,"\p",&spec); 
	
	if (!(err=NewAlias(&spec,originalSpec,&alias)) && !(err=FSpGetFInfo(originalSpec,&origInfo)))
	{
		err = FSMakeFSSpec(aliasSpec->vRefNum,aliasSpec->parID,aliasSpec->name[0]?aliasSpec->name:originalSpec->name,&localSpec);
		
		/*
		 * does file exist?
		 */
		if (!FSpGetFInfo(&localSpec,&aliasInfo))
		{
			/*
			 * don't replace real file with alias
			 */
			if (!IsAlias(&localSpec,&spec))
			{
				err = dupFNErr;
				goto done;
			}
			else if (SameSpec(originalSpec,&spec))
				goto done;	/* we already have an alias to the file in ? */
		}
		
		/*
		 * create alias file
		 */
		FSpCreateResFile(&localSpec,origInfo.fdCreator,origInfo.fdType,smSystemScript);
		err=ResError();
		if (err = dupFNErr) err = noErr;	/* ignore; we'll change an existing alias */
		
		/*
		 * write alias into it
		 */
		if (!err)
		{
			err = FSpGetFInfo(&localSpec,&aliasInfo);
			aliasInfo.fdFlags |= kIsAlias;
			err = FSpSetFInfo(&localSpec,&aliasInfo);

			/*
			 * now we have an alias file; stick the alias into it
			 */
			if (0<=(refN=FSpOpenResFile(&localSpec,fsRdWrPerm)))
			{
				AddResource_(alias,'alis',0,"");
				if (!(err=ResError())) alias=nil;
				CloseResFile(refN);
			}
			else err = ResError();
			if (err) {FSpDelete(&localSpec);}
		}
	}
done:
	if (!err) *aliasSpec = localSpec;
	ZapHandle(alias);
	UseResFile (oldResF);
	return(err);
}

/************************************************************************
 * SimpeMakeFSSpec - make an FSSpec, but don't hit the filesystem
 ************************************************************************/
void SimpleMakeFSSpec(short vRef,long dirId,PStr name, FSSpecPtr spec)
{
	spec->vRefNum = vRef;
	spec->parID = dirId;
	PSCopy(spec->name,name);
}

/************************************************************************
 * FindMyFile - Like FindFile, only Eudora-related
 ************************************************************************/
OSErr FindMyFile(FSSpecPtr spec,long whereToLook,short fileName)
{
	FSSpec mySpec;
	OSErr err = fnfErr;
	Str31 nameStr;
	
	if (whereToLook & kStuffFolderBit)
	{
		if (!(err=GetFileByRef(AppResFile,&mySpec)))
		if (!(err=FSMakeFSSpec(mySpec.vRefNum,mySpec.parID,GetRString(nameStr,STUFF_FOLDER),&mySpec)))
		{
			IsAlias(&mySpec,&mySpec);
			mySpec.parID = SpecDirId(&mySpec);
			if (!(err=FSMakeFSSpec(mySpec.vRefNum,mySpec.parID,GetRString(nameStr,fileName),&mySpec)))
			{
				IsAlias(&mySpec,&mySpec);
				*spec = mySpec;
				return noErr;
			}
		}
	}
	
	return err;
}

#ifdef DEBUG
#undef FSpDirCreate
#undef DirCreate
/************************************************************************
 * FSpDirCreate - call FSpDirCreate but pacify SpotLight
 ************************************************************************/
OSErr MyFSpDirCreate(FSSpecPtr spec, ScriptCode scriptTag, long *createdDirID)
{
	OSErr err;
	SLDisable();
	err = FSpDirCreate(spec,scriptTag,createdDirID);
	SLEnable();
	return(err);
}

/************************************************************************
 * MyDirCreate - call DirCreate but pacify SpotLight
 ************************************************************************/
OSErr MyDirCreate(short vRefNum, long parentDirID, PStr directoryName, long *createdDirID)
{
	OSErr err;
	SLDisable();
	err = DirCreate( vRefNum,  parentDirID,  directoryName, createdDirID);
	SLEnable();
	return(err);
}

/************************************************************************
 * MyFSpDelete - bottleneck for deleting files
 ************************************************************************/
#undef FSpDelete
OSErr MyFSpDelete(FSSpecPtr spec)
{
	OSErr err;
	
	ASSERT(!SameSpec(spec,&SettingsSpec));
	
	err = FSpDelete(spec);
#ifdef NEVER
	if (RunType!=Production && spec.vRefNum==AttFolderSpec.vRefNum && spec.parID==AttFolderspec.parID)
		Dprintf("\pFSpDelete %d.%d.�%p�",spec->vRefNum,spec->parID,spec->name);
#endif
	return err;
}

/************************************************************************
 * MyCloseResFile - bottleneck for closing resource files
 ************************************************************************/
#undef CloseResFile
void MyCloseResFile(short refN)
{	
	ASSERT(refN!=ThreadGlobals.tSettingsRefN);
	
	CloseResFile(refN);
}
#define CloseResFile MyCloseResFile
#endif

/************************************************************************
 * MakeUniqueUntitledSpec - Make a unique "untitled" name in some folder
 ************************************************************************/
void MakeUniqueUntitledSpec (short vRefNum, long dirID, short strResID, FSSpec *spec)

{
	Str31		name,
					s;
	long		suffix;
	Byte		saveLen;

	//	Make a unique "untitled" name
	GetRString (name, strResID);
	suffix = 2;
	saveLen = *name;
	while (!FSMakeFSSpec (vRefNum, dirID, name, spec)) {
		//	No error means that the file/folder exists. Change the file name by adding a numeric suffix
		*name = saveLen;	//	Remove any suffix
		NumToString (suffix++, s);
		PCatC (name, ' ');
		PCat (name, s);
	}
}


OSErr MisplaceItem (FSSpec *spec)

{
	FSSpec	misplacedFolder,
					exist,
					newExist;
	OSErr		theError;
	long		dirID;
	
	// Find the Misplaced Items folder
	if (theError = SubFolderSpec (MISPLACED_FOLDER, &misplacedFolder)) {
		SimpleMakeFSSpec (Root.vRef, Root.dirId, GetRString (misplacedFolder.name, MISPLACED_FOLDER), &misplacedFolder);
		theError = FSpDirCreate (&misplacedFolder, smSystemScript, &dirID);
		if (!theError)
			misplacedFolder.parID = dirID;
	}
	if (!theError) {
		IsAlias (&misplacedFolder, &misplacedFolder);
		if (!FSMakeFSSpec (misplacedFolder.vRefNum, misplacedFolder.parID, spec->name, &exist)) {
			newExist = exist;
			UniqueSpec (&newExist, 31);
			FSpRename (&exist, newExist.name);
		}
		theError = SpecMove (spec, &misplacedFolder);
	}
	return (theError);
}


OSErr FSpGetLongName ( FSSpec *spec, TextEncoding destEncoding, Str255 longName ) {
	OSErr	err = noErr;
	HFSUniStr255	uniName;
	
	if (destEncoding==kTextEncodingUnknown) destEncoding = CreateTextEncoding(kTextEncodingMacRoman,0,0);
	
//	Get the unicode name
	err = FSpGetLongNameUnicode ( spec, &uniName );
	if ( err == noErr ) {
		UnicodeToTextInfo	info;
		
	//	Convert the name back to UTF-8 or something
		err = CreateUnicodeToTextInfoByEncoding ( destEncoding, &info );
		if ( err == noErr ) {
			//err = ConvertFromUnicodeToPString ( info, uniName.length * sizeof ( UniChar ), uniName.unicode, longName );
			long charsUsed=0, charsOut=0;
			err = ConvertFromUnicodeToText ( info, uniName.length * sizeof ( UniChar ), uniName.unicode, 
																			 kUnicodeUseFallbacksMask|kUnicodeLooseMappingsMask, 0, nil, nil, nil, 127, &charsUsed, &charsOut, longName+1 );
			*longName = charsOut;
			ASSERT(!err || err==kTECUsedFallbacksStatus);
			if (*longName) err = noErr;	// if we got something, use it
			
  		(void) DisposeUnicodeToTextInfo ( &info );
  		}
		}

	return err;
	}

OSErr FSpGetLongNameUnicode ( FSSpec *spec, HFSUniStr255 *longName ) {
	OSErr	err = noErr;
	FSRef	aRef;

	err = FSpMakeFSRef ( spec, &aRef );
	if ( err == noErr ) {
		err = FSGetCatalogInfo ( &aRef, kFSCatInfoNone, NULL, longName, NULL, NULL );
		}

	return err;
	}


OSErr FSpSetLongName ( FSSpec *spec, TextEncoding srcEncoding, ConstStr255Param longName, FSSpec *newSpec ) {
	OSErr				err = noErr;
	HFSUniStr255		uniName;
	TextToUnicodeInfo	info;

	if (srcEncoding == kTextEncodingUnknown) srcEncoding = CreateTextEncoding(kTextEncodingMacRoman,0,0);
	
	err = CreateTextToUnicodeInfoByEncoding ( srcEncoding, &info );
	if ( err == noErr ) {
		ByteCount uniStrLen;
		err = ConvertFromPStringToUnicode ( info, longName, 255 * sizeof ( UniChar ), &uniStrLen, uniName.unicode );
		uniName.length = uniStrLen/2;
		if ( err == noErr )
			err = FSpSetLongNameUnicode ( spec, &uniName, newSpec );
		DisposeTextToUnicodeInfo ( &info );
		}
	return err;
	}


OSErr FSpSetLongNameUnicode ( FSSpec *spec, ConstHFSUniStr255Param longName, FSSpec *newSpec ) {
	OSErr	err = noErr;
	FSRef	aRef;

	err = FSpMakeFSRef ( spec, &aRef );
	if ( err == noErr ) {
		FSRef newRef;
		FSRef *refPtr = newSpec != NULL ? &newRef : NULL;
		err = FSRenameUnicode ( &aRef, longName->length, longName->unicode, kTextEncodingUnicodeDefault, refPtr );
	//	Convert the FSRef back into a FSSpec for the caller
		if ( err == noErr && refPtr != NULL )
			(void) FSGetCatalogInfo ( refPtr, kFSCatInfoNone, NULL, NULL, newSpec, NULL );
		}

	return err;
	}


OSErr MakeUniqueLongFileName ( short vRefNum, long dirID, StringPtr name, TextEncoding srcEncoding, short maxLen ) {
	OSStatus			err;
	Str31				s;
	HFSUniStr255		uniName;
	TextToUnicodeInfo	info;
	FSRef				parent;
	FSRefParam			fs;
	
	ASSERT ( name != NULL );	
	ASSERT ( maxLen >= 63 );	

//	Make an FSRef to the parent directory
	Zero ( s );
	Zero ( fs );
	fs.ioVRefNum		= vRefNum;
	fs.ioDirID			= dirID;
	fs.ioNamePtr		= s;
	fs.newRef			= &parent;
	if ( noErr != ( err = PBMakeFSRefSync ( &fs )))
		return err;
		
	if ( srcEncoding == kTextEncodingUnknown )
		srcEncoding = CreateTextEncoding ( kTextEncodingMacRoman, 0, 0 );

	err = CreateTextToUnicodeInfoByEncoding ( srcEncoding, &info );
	if ( err == noErr ) {
		Str255		base, suffix;
		long		nextFile = 1;
		FSRef		ref;
	
	//	Split the file name into base name and suffix
		SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote ( name, base, suffix, maxLen );
		base [ ++base [ 0 ]] = ' ';	// tack a space on the end
		
	//	Set up the parameter block
		Zero ( fs );
		fs.ref				= &parent;
		fs.name				= uniName.unicode;
		fs.newRef			= &ref;

		while ( true ) {
			ByteCount uniStrLen;

		//	See if the file exists
			err = ConvertFromPStringToUnicode ( info, name, 255 * sizeof ( UniChar ), &uniStrLen, uniName.unicode );
			fs.nameLength = uniStrLen / sizeof ( UniChar );

			err = PBMakeFSRefUnicodeSync ( &fs );
			if ( err != noErr )
				break;

			NumToString ( nextFile++, s );
		
		//	If the new string will be too long, then trim the base
			if ( base[0] + suffix[0] + s[0] > maxLen - 1 ) {
				short newLen;
				base[0] = newLen = maxLen - ( suffix[0] + s[0]);
				base [ newLen ] = ' ';
				base [ newLen - 1 ] = '�';
				}
		
		//	Build the whole string
			PCopy ( name, base );
			PCat ( name, s );
			PCat ( name, "\p." );
			PCat ( name, suffix );
			}

		ASSERT ( err != noErr );	
	//	Map fnfErr --> success
		if ( err == fnfErr )
			err = noErr;

		DisposeTextToUnicodeInfo ( &info );
		}
	
	return err;
	}

/**********************************************************************
 * The following block of functions is intended to allow Eudora to wait
 * for access to its files, in case other programs are using them.  It will
 * wait up to a certain number of seconds (currently 10) for files
 * to become free.  It waits if it gets opWrErr or permErr.  This latter
 * is what most calls seem to return for busy files, though you'd think
 * opWrErr would be more appropriate.  Unfortunately, permErr is what
 * you also get for a locked file, so if we get it we have to rule it out.
 * We get afpAccessDenied for permission errors, even local unix ones,
 * so at least we don't have to worry about that
 **********************************************************************/

#undef FSpOpenResFile
short FSpOpenResFilePersistent(FSSpecPtr spec,short mode)
{
	uLong ticks = TickCount();
	short refN;
	
	for(;;)
	{
		refN = FSpOpenResFile(spec,mode);
		if (refN==-1)
		{
			OSErr err = ResError();
			if ((err==opWrErr || err==permErr)&&!FSpIsLocked(spec))
			{
				if (TickCount()-ticks > GetRLong(LOCKED_FILE_PERSISTENCE)*60) break;
				YieldCPUNow();
			}
			else break;
		}
		else
			break;
	}
	return refN;
}
#define FSpOpenResFile FSpOpenResFilePersistent

#undef FSpOpenDF
OSErr FSpOpenDFPersistent(FSSpecPtr spec,short mode,short *refN)
{
	uLong ticks = TickCount();
	OSErr err;
	
	for(;;)
	{
		err = FSpOpenDF(spec,mode,refN);
		if ((err==opWrErr || err==permErr)&&!FSpIsLocked(spec))
		{
			if (TickCount()-ticks > GetRLong(LOCKED_FILE_PERSISTENCE)*60) break;
			YieldCPUNow();
		}
		else
			break;
	}
	return err;
}
#define FSpOpenDF FSpOpenDFPersistent

#undef FSpOpenRF
OSErr FSpOpenRFPersistent(FSSpecPtr spec,short mode,short *refN)
{
	uLong ticks = TickCount();
	OSErr err;
	
	for(;;)
	{
		err = FSpOpenRF(spec,mode,refN);
		if ((err==opWrErr || err==permErr)&&!FSpIsLocked(spec))
		{
			if (TickCount()-ticks > GetRLong(LOCKED_FILE_PERSISTENCE)*60) break;
			YieldCPUNow();
		}
		else
			break;
	}
	return err;
}
#define FSpOpenRF FSpOpenRFPersistent

Boolean FSpIsLocked(FSSpecPtr spec)
{
	CInfoPBRec cfi;
	if (!HGetCatInfo(spec->vRefNum,spec->parID,spec->name,&cfi))
	{
		return (cfi.hFileInfo.ioFlAttrib&kioFlAttribLockedMask)!=0;
	}
	return false;
}

/**********************************************************************
 * end file wait functions
 **********************************************************************/

/**********************************************************************
 * IsPDFFile - is a spec a pdf file?
 **********************************************************************/
Boolean IsPDFFile(FSSpecPtr spec,OSType fileType)
{
	if (fileType == 'PDF ') return true;
	// Avi must die.
	if (EndsWithR(spec->name,PDF_QUOTE_EXTENSION_UNQUOTE)) return true;
	return false;
}

/**********************************************************************
 * SpecEndsWithExtensionR - does a spec (long name) end with an extension
 *  on a list?
 **********************************************************************/
Boolean SpecEndsWithExtensionR(FSSpecPtr spec,short resID)
{
	Str255 longName;
	
	if (FSpGetLongName(spec,0,longName)) PCopy(longName,spec->name);
	
	return EndsWithItem(longName,resID);
}