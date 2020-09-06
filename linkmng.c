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

#include <Resources.h>
#include <ToolUtils.h>
#include <conf.h>
#include <mydefs.h>

#include "linkmng.h"
#define FILE_NUM 128

/* Copyright (c) 1999 by QUALCOMM Incorporated */

/************************************************************************
 *  routines to manage multiple link history lists
 ************************************************************************/
 
#include "linkmng.h"
#define FILE_NUM 128

#define LINK_HISTORY_VERSION 4		// increment this when changing the HistoryStruct

#define LINK_TOC_TYPE			'LToc'
#define LINK_RESID				130

// until ListView is taught about items with names > 31 characters
typedef Str31 URLNameStr;


//
// Data structures for link management
//

/* LinkTypeEnum - types of links stored in the link history */
typedef enum {
	ltAd=0,
	ltHttp,
	ltFtp,
	ltMail,
	ltDirectoryServices,
	ltSetting,
	ltFile,
	LinkTypeLimit
} LinkTypeEnum;

/* LinkLabelEnum - labels for links */
typedef enum {
	llRemindMe=1,
	llBookmarked,
	llAttempted,
	llNone,
	llNotDisplayed,
	LinkLabelLimit
} LinkLabelEnum;

/* Structure for a given history entry, version 1 */
typedef struct
{
	short 			version;				/* the version of the link history entry */
	long			hashName; 				/* hash value on history name */
	URLNameStr		name;					/* the name of the url */
	unsigned long	cacheSeconds;			/* date this url was last clicked */
	LinkTypeEnum	type;					/* the type of url this is */
	long			dirty:1;				/* has the history entry been modified */
	long			deleted:1;				/* has the history entry been deleted */
	long			thumb:1;				/* does this history entry have a thumbnail */
	long			label:4;				/* label */
	long 			incompleteAd:1;			/* this entry is an ad that's not ready yo be displayed in the history */				
	long			remind:1;				/* remind the user to visit this link */
	long			spare:23;				/* leftovers */
	long			urlOffset;				/* offset in file where the real url is at */
	long			imageOffset;			/* offset in file where the location of the icon is at */
	Handle			hUrl;					/* Handle to URL this entry will link to */
	AdId			adId;					/* ID of Ad this entry refers to */
} HistoryStruct, *HistoryStructPtr, **HistoryStructHandle;

/* Structure to keep track of an infividual history file */
typedef struct HistoryDStruct
{
	FSSpec spec;							/* the history file */
	HistoryStructHandle theData;			/* the toc */
	Boolean ro;								/* read only */
	Boolean dirty;							/* is the history file dirty? */
} HistoryDesc, *HistoryDPtr, **HistoryDHandle;

/* Structure just enough info for the history window.  Will sort this puppy. */
typedef struct
{
	URLNameStr		name;					/* the name of the url */
	unsigned long	cacheSeconds;			/* date this url was last clicked */
	LinkTypeEnum	type;					/* the type of url this is */
	VLNodeID		nodeId;					/* id of this node, calculated from which and index */
	LinkLabelEnum	label;					/* the label of this history entry */
} ShortHistoryStruct, *ShortHistoryStructPtr, **ShortHistoryStructHandle;

/* Structure to maintain a cache of preview icon handles */
typedef struct LHPIconCacheStruct
{
	Handle theIcon;
	AdId adId;
	ICacheHandle next;
} LHPIconCacheStruct, *LHPIconCachePtr, **LHPIconCacheHandle;

//
// Globals for link management
//

// Global list of all known history files
HistoryDHandle gHistories = nil;	

// Global list of loaded preview icons
LHPIconCacheHandle gPreviewIcons = nil;

// Global spec pointing inside the Link History FOlder
FSSpec gLinkHistoryFolder;

// GLobal list of all preview icons

// Types of links we care about if following them fails with some error
LinkTypeEnum gLabelTheseLinks[] = {ltAd, ltHttp, ltFtp, ltMail, ltDirectoryServices};

//
// Some helpful #defines
//

#ifdef DEBUG
	#define COMPACT_THRESHHOLD 2
#else
	#define COMPACT_THRESHHOLD 10
#endif

#define AGE_INTERVAL 60*60*60	// age once an hour

#define MAIN_HISTORY_FILE 0
#define DEFAULT_LINK_TYPE_ICON HTTP_LINK_TYPE_ICON
#define This (*gHistories)[which]
#define NHistoryFiles (gHistories ? GetHandleSize_(gHistories)/sizeof(HistoryDesc) : 0)

//
// Link management prototypes
//
 
OSErr WriteHistTOC(short which);
Handle GetHistoryData(short which,short index,Boolean readFromDisk);
OSErr ReadHistTOC(short which);
static void ZapHistoryFile(short which, Boolean destroy);
static OSErr RegenerateLinkHistory(short which, Boolean rebuild);
long HistMatchFound(long hashName,Handle theUrl,short which);
void ReadHistFileList(FSSpec *pSpec,Boolean reread);
OSErr AddHistoryToTOC(short which,UPtr name,long hashName,LinkTypeEnum type, LinkLabelEnum label,Boolean thumb,Handle url,AdId adId);
OSErr SaveIndHistoryFile(short which);
void DeleteHistEntryFromTOC(short which, short index);
Boolean TimeToCompactTOC(short which);
OSErr CompactHistTOC(short which);
short LinkTypeToIconID(LinkTypeEnum type);
VLNodeID EntryToNodeId(short which, short index);
void NodeIdToEntry(VLNodeID id, short *which, short *index);
OSErr AddURLToHistory(short which, PStr url, PStr name, OSErr urlOpenErr);
short MenuItemNameToIconID(short menuID, short item);
LinkTypeEnum LinkType(ProtocolEnum protocol, PStr proto);
void LinkUpdateCacheDate(short which, short index);
short HistoryCount(short which);
Boolean LocateAdInHistories(AdId adId, short *which, short *index);
short AgeHistoryFile(short which);
void LabelToString(LinkLabelEnum label, Str255 dateStr);
Boolean LabelableLink(LinkTypeEnum linkType);
Boolean CorrectVersion(HistoryStructHandle theToc);
void UpdateLinkLabel(HistoryStructPtr entry, OSErr err);
LinkLabelEnum OpenErrToLabel(OSErr err);
Boolean InterestingProtocol(Str255 proto);
void MakeLinkName(Str255 host, Str255 query, URLNameStr urlName);

// Sorting
OSErr BuildListOfHistoriesForWindow(ShortHistoryStructHandle *histories, Boolean needsSort, LinkSortTypeEnum sortType);
void SortShortHistoryHandle(ShortHistoryStructHandle toSort, int (*compare)());
int HistTypeCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistNameCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistDateCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistRemindCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
void SwapHist(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);

// Ad preview stuff
OSErr CreateIconFromAdGraphic(AdId adId, FSSpecPtr adGraphic);
void AdIdToName(AdId adId, URLNameStr name);
Boolean NameToAdId(URLNameStr name, AdId *ad);
OSErr DeleteAdGraphic(AdId adId);
OSErr IconFromAd(FSSpecPtr iconSpec, FSSpecPtr adSpec);
OSErr MakeLHIconFile(GWorldPtr gWorld, Rect *pRect, FSSpecPtr iconSpec, GraphicsImportComponent importer, FSSpecPtr sourceSpec);
void PurgeLinkHistoryPreviewOrphans(void);

// Icon cache management
void AddIconToPVICache(Handle theIcon, AdId adId);
LHPIconCacheHandle FindPVICache(AdId adId);
void RemoveIconFromPVICache(AdId adId);
void RemovePVIFromPVICache(LHPIconCacheHandle *toRemove);

// Icon building routines, mostly taken from MakeIcon
OSErr SetUpPixMap( short depth, Rect *bounds,CTabHandle colors,PixMapHandle aPixMap);
void TearDownPixMap(PixMapHandle pix);
Handle MakeIconMask(GWorldPtr srcGWorld, Rect *srcRect, short iconSize);
Handle MakeIconLo(GWorldPtr srcGWorld, Rect *srcRect, short dstDepth, short iconSize, RGBColor *transColor);
void FreeBitMap(BitMap *Bits);
void CalcOffScreen(register Rect *frame,register long *needed, register short *rows);
void NewBitMap(Rect *frame,BitMap *theMap);
void NewMaskedBitMap(BitMap	*srcBits, BitMap *maskBits, Rect *srcRect);

// Nickname routines we've assimilated
extern OSErr KillNickTOC(FSSpecPtr spec);
extern Boolean NeatenLine(UPtr line, long *len);

/************************************************************************
 * AddURLToMainHistory - add a URL to the main history file
 ************************************************************************/
OSErr AddURLToMainHistory(PStr url, PStr name, OSErr urlOpenErr)
{
	return (AddURLToHistory(MAIN_HISTORY_FILE, url, name, urlOpenErr));
}

/************************************************************************
 * AddURLToMainHistory - add a URL to the main history file
 ************************************************************************/
OSErr AddURLToHistory(short which, PStr url, PStr name, OSErr urlOpenErr)
{
	OSErr err = noErr;
	ProtocolEnum protocol;
	Str255 proto,host,query;
	long hashName;
	Handle hUrl = nil;
	URLNameStr urlName;
	LinkLabelEnum linkLabel = llNone;
	LinkTypeEnum linkType;
	short index;
	
	// don't add the URL if it wasn't parsable
	if (urlOpenErr == 1) return (urlOpenErr);	
	
	// parse the URL.  See what it is.
	if (!(err=ParseURL(url,proto,host,query)))
	{
		// see if this is one of the protocols we should be keeping history entries for
		if (!InterestingProtocol(proto)) return (userCanceledErr);
		
		protocol = FindSTRNIndex(ProtocolStrn,proto);
		FixURLString(host);
		if (protocol!=proMail) FixURLString(query);
	
		//
		//	Figure out the history entry data
		//
		
		// make sure the name of the url contains something interesting
		if (name && name[0])
		{
			// use the name passed in
			PStrCopy(urlName, name, sizeof(urlName));
		}
		else
		{
			if (host[0]) 	
			{
				// make a name for this link entry out of the host and query portions of the URL itself	
				MakeLinkName(host, query, urlName);
			}
			else
			{
				// URL had no host.  Use the actual URL
				PStrCopy(urlName, url, sizeof(urlName));
			}
		}

		// The name of the history entry will be a hash of the url itself
		hashName = NickHashString(url);
		
		// What sort of link is this?
		linkType = LinkType(protocol, proto);
		
		// Was this link successfully launched?
		if (urlOpenErr != noErr)
		{
			// is this a link type we should label?
			if (LabelableLink(linkType))
			{
				// the link was attempted, but probably failed.
				linkLabel = OpenErrToLabel(urlOpenErr);
			}
		}
		
		// Turn the url into a handle
		hUrl = NuHTempOK(0);
		if (!hUrl) return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,MemError()));
		err = PtrPlusHand(url+1,hUrl,url[0]);
		if (err) return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,err));
		
		//
		//	Add the new history entry
		//
		
		// Make sure the history files are around somewhere
		err = GenHistoriesList();
		
		if (err == noErr)
		{
			// Is this entry already in the history file?
			if ((index=HistMatchFound(hashName, hUrl, which)) >= 0)
			{	
				// adjust the label of the link ...
				UpdateLinkLabel(&((*(*gHistories)[which].theData)[index]), urlOpenErr);
				
				// update the date in the TOC ...
				LinkUpdateCacheDate(which, index);
			}
			else
			{
				// If not, add the history to the history file
				AdId junk;
				
				junk.server = junk.ad = 0;
				err =  AddHistoryToTOC(which, urlName, hashName, linkType, linkLabel, false, hUrl, junk);
				if (err == noErr)
				{
					// save the history file
					err = SaveIndHistoryFile(which);
				}
			}
			
			// Update the Link History window ...
			LinkTickle();
		}
	}
	
	return (err);
}

/************************************************************************
 * MakeLinkName - build a name for a link history entry
 ************************************************************************/
void MakeLinkName(Str255 host, Str255 query, URLNameStr urlName)
{
	short linkNameLength = sizeof(URLNameStr);
	
	// initialize the name
	WriteZero(urlName, linkNameLength);
	
	// start with the host name
	PCopy(urlName,host);
	
	// more room?
	if (urlName[0] < linkNameLength-1)
	{
		// add a slash
		urlName[urlName[0]+1] = '/';
		urlName[0] += 1;
		
		// tack on the query
		if (query[0]) PSCat_C(urlName,query,linkNameLength);
	}
	
	// replace last character with elipsis, if we reached the maximum length
	if (urlName[0] == linkNameLength-1)
	{
		urlName[urlName[0]] = '�';
	}
}

/************************************************************************
 * InterestingProtocol - see if this is a protocol worth remembering
 ************************************************************************/
Boolean InterestingProtocol(Str255 proto)
{
	Boolean remember = false;
	Str255 insterestingProtocols, token;
	UPtr spot;
	
	if (proto[0])
	{
		// read in the list of interesting protocols
		GetRString(insterestingProtocols,LINK_INTERESTING_PROTO);
		
		// see if proto is one of them
		for(spot=insterestingProtocols+1;PToken(insterestingProtocols,token,&spot,",") && !remember;)
		{
			if (StringSame(token, proto)) remember = true;
		}
	}
	
	return (remember);
}

/************************************************************************
 * UpdateLinkLabel - update the label of a link according to an error
 ************************************************************************/
void UpdateLinkLabel(HistoryStructPtr entry, OSErr err)
{	
	if (LabelableLink(entry->type) && (err!=noErr))
	{
		// adjust the label unless the user cancelled ...
		if (err != olaCancel) entry->label = OpenErrToLabel(err);
	 	
	 	// remind the user later if we should ...
	 	entry->remind = (err == oldaRemind);
	 	gNeedRemind |= (entry->remind!=0);
	}
	else entry->label = llNone;
}

/************************************************************************
 * OpenErrToLabel - given an error code, return the label we should use
 ************************************************************************/
LinkLabelEnum OpenErrToLabel(OSErr err)
{
	LinkLabelEnum result = llNone;
	
	if ((err!=noErr))
	{
	 	if (err == oldaBookmark) result = llBookmarked;
	 	else if (err == oldaRemind) result = llRemindMe;
	 	else result = llAttempted;
	}
	
	return (result);
}
			
/************************************************************************
 * RegenerateLinkHistory - make sure the history list is in memory
 ************************************************************************/
static OSErr RegenerateLinkHistory(short which, Boolean rebuild)
{
	OSErr err = noErr;
	UHandle hand;
	
	if (rebuild) ZapHistoryFile(which, true);
	
	if (!(*gHistories)[which].theData) // If handle for data doesn't exist, create it
	{
		hand = NuHTempOK(0L);
		if (!hand) err = WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,MemError());
		(*gHistories)[which].theData = (HistoryStructHandle)hand;
	}
	else // Handle exists
	{
		if (!rebuild)
			return (noErr);
	}
	
	if (err != noErr || rebuild)
	{
		err = ReadHistTOC(which);
	}

	if (err)
		ZapHandle((*gHistories)[which].theData);

	return(err);
}

/************************************************************************
 * ZapHistoryFile - release memory for all histories in a file
 ************************************************************************/
static void ZapHistoryFile(short which, Boolean destroy)
{
	short	i;
	HistoryStructHandle history = gHistories ? (*gHistories)[which].theData : nil;

	if (history)
	{
		// throw away all the URL and image handles we have laying around
		for (i=0;i<HistoryCount(which);i++)
		{
			if ((*history)[i].hUrl)
			{
				ZapHandle((*history)[i].hUrl);
				(*history)[i].hUrl = nil;
			}
		}
		
		// if we're destroying, trash the actual history list as well
		if (destroy)
		{
			ZapHandle((*gHistories)[which].theData);
			(*gHistories)[which].theData = nil;
		}
	}
}

/************************************************************************
 * ZapHistoriesList - destroy the history file list, or make it smaller
 ************************************************************************/
void ZapHistoriesList(Boolean destroy)
{
	short which;
	short n = gHistories ? NHistoryFiles : 0;
	
	// clean up the inidividual history files
	for (which = 0; which < n; which++)
	{
		ZapHistoryFile(which, destroy);
	}
	
	// if we're destroying, zap the handle to all the histories as well.
	if (destroy) 
	{
		if (gHistories) ZapHandle(gHistories);
		gHistories = nil;
	}
}

/************************************************************************
 * GenHistoriesList - generate the history file list
 ************************************************************************/
OSErr GenHistoriesList(void)
{
	OSErr err = noErr;
	Str31 name;
	FSSpec folderSpec;
	HistoryDesc ad;
	
	Zero(ad);
	
	// do nothing if the history files have already been loaded.
	if (gHistories) return(noErr);
	
	/*
	 * allocate empty handle for new
	 */
	 
	if (!(gHistories=NuHandle(0L)))
	{
		WarnUser(MEM_ERR,err = MemError());
		return (err);
	}
	
	/*
	 * add the Eudora History file
	 */
	 
	FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(name,LINK_HISTORY_FILE),&ad.spec);
	if (PtrPlusHand_(&ad,gHistories,sizeof(ad))) DieWithError(MEM_ERR,MemError());
	RegenerateLinkHistory(MAIN_HISTORY_FILE, true);

	/*
	 * add any additional history files in the Link History Folder
	 */
	 
	ReadHistFileList(&folderSpec,false);

	return (err);
}

/************************************************************************
 * ReadHistFileList - find extra link history files
 ************************************************************************/
void ReadHistFileList(FSSpec *pSpec,Boolean reread)
{
	Str31 name;
	CInfoPBRec hfi;
	HistoryDesc ad;
	long dirId;
	short count = 1;
	
	/*
	 * create the folder if we need it ...
	 */
	if (SubFolderSpec(LINK_HISTORY_FOLDER,pSpec)!=noErr)
	{
		DirCreate(Root.vRef,Root.dirId,GetRString(name,LINK_HISTORY_FOLDER),&dirId);
		SimpleMakeFSSpec(pSpec->vRefNum,dirId,"\p",&gLinkHistoryFolder);
		return;		
	}
	else
		gLinkHistoryFolder = *pSpec;
	
	/*
	 * read in the history files ...
	 */
	
	Zero(ad);	
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(pSpec->vRefNum,pSpec->parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType=='TEXT' && hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			SimpleMakeFSSpec(pSpec->vRefNum,pSpec->parID,name,&ad.spec);
			if (!CanWrite(&ad.spec,&ad.ro)) ad.ro = !ad.ro;	/* opposite sense! */
			if (PtrPlusHand_(&ad,gHistories,sizeof(ad))) DieWithError(MEM_ERR,MemError());
			
			// read it in
			RegenerateLinkHistory(count++, true);
		}
	}
}

/************************************************************************
 * AddHistoryToTOC - add a history entry to the TOC
 ************************************************************************/
OSErr AddHistoryToTOC(short which,UPtr name,long hashName,LinkTypeEnum type,LinkLabelEnum label,Boolean thumb,Handle url,AdId adId)
{
	long currHistCount;
	HistoryStructHandle histories = (*gHistories)[which].theData;
	OSErr err = noErr;
	HistoryStruct histInfo;
	 
	//
	//	Make sure the history list is around
	//
	
	if (RegenerateLinkHistory(which, false)!=noErr) return (err);

	//
	// Make room for the new history entry
	//
	
	HUnlock((Handle)histories);
	currHistCount = HistoryCount(which);
	if (currHistCount > 0)
	{
		SetHandleBig_((Handle)histories,(currHistCount + 1)*sizeof(HistoryStruct));
		if (err = MemError()) 
			return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,err));

	}
	else
	{
		ZapHandle((*gHistories)[which].theData);
		histories = NuHTempOK(sizeof(HistoryStruct));
		if (!histories)
			return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,MemError()));
		else
			(*gHistories)[which].theData = histories;
	}

	// 
	// Add the TOC entry
	//
	
	// Fill in the basic data
	WriteZero(&histInfo,sizeof(histInfo));
	histInfo.version = LINK_HISTORY_VERSION;
	histInfo.hashName = hashName;
	PSCopy(histInfo.name, name);
	histInfo.cacheSeconds = LocalDateTime();
	histInfo.type = type;
	histInfo.dirty = true;
	histInfo.deleted = false;
	histInfo.thumb = thumb;
	histInfo.label = label;
	histInfo.incompleteAd = ((type == ltAd) && !(url && *url && **url));
	histInfo.urlOffset = (-1L);
	histInfo.imageOffset = (-1L);
	histInfo.adId = adId;
	
	// now the potentially large url
	if (url && *url && **url)
	{		
		histInfo.hUrl = url;
	}
	
	//	Put hist info into the history TOC array
	(*(histories))[currHistCount] = histInfo;
	
	UL(histories);
	(*gHistories)[which].dirty = true;
	return(0);
}

/************************************************************************
 * SaveAllHistoryFiles - save all history files that need it
 ************************************************************************/
OSErr SaveAllHistoryFiles(void)
{
	OSErr err = noErr;
	short hFiles;
	
	for (hFiles = 0; hFiles < NHistoryFiles; hFiles++)
	{
		if ((*gHistories)[hFiles].dirty)
			err = err || SaveIndHistoryFile(hFiles);
	}
	
	return (err);
}

/************************************************************************
 * SaveIndHistoryFile - save an individual History file
 ************************************************************************/
OSErr SaveIndHistoryFile(short which)
{
	Str31 aliasCmd;
	int err;
	long bytes,offset;
	short refN=0;
	long i, count;
	FSSpec spec,tmpSpec;
	Boolean junk;
	Handle	tempHandle;

#ifdef	DEBUG
	if (InAThread()) return noErr;
#endif

	/*
	 * do we need to save it?
	 */
	if (!(*gHistories)[which].dirty) return(noErr);
		
	/*
	 * find the file
	 */
	spec = (*gHistories)[which].spec;
	if (err=FSpMyResolve(&spec,&junk))
	{
		FileSystemError(SAVE_LINK_HISTORY,spec.name,err);
		return(err);
	}
	
	/*
	 * make && open a temp file
	 */
	if (err=NewTempSpec(spec.vRefNum,spec.parID,nil,&tmpSpec))
		goto done;
	if (err=FSpCreate(&tmpSpec,CREATOR,MAILBOX_TYPE,smSystemScript))
		{FileSystemError(SAVE_LINK_HISTORY,tmpSpec.name,err); goto done;}
	if (err=FSpOpenDF(&tmpSpec,fsRdWrPerm,&refN))
		{FileSystemError(OPEN_LINK_HISTORY,tmpSpec.name,err); goto done;}
	
		
	count = HistoryCount(which); // Get history entry count

	/*
	 *	Write out the url
	 */
	 
	GetRString(aliasCmd,LINK_CMD);
	PCatC(aliasCmd,' ');
	HLock((Handle)(*gHistories)[which].theData);
	for (i=0;!err && i<count;i++)
	{
		CycleBalls();
		if ((*((*gHistories)[which].theData))[i].dirty) // Nickname has been modified...use in memory copy
			tempHandle = GetHistoryData(which,i,false);
		else // Nickname hasn't been modified, use on disk copy if necessary
			tempHandle = GetHistoryData(which,i,true);
		
		(*((*gHistories)[which].theData))[i].dirty = false;
		
		bytes = (tempHandle && *tempHandle && **tempHandle) ? GetHandleSize_(tempHandle) : 0;		
		
		if (bytes >0 && !(*((*gHistories)[which].theData))[i].deleted)
		{	
			GetFPos(refN,&offset);
			(*((*gHistories)[which].theData))[i].urlOffset = offset;
		}
		else
		{
			// (*gHistories)[which] entry is missing a URL.  If it's not an incomplete ad, remove it from the TOC, and skip it.
			if (!(*((*gHistories)[which].theData))[i].incompleteAd) (*((*gHistories)[which].theData))[i].deleted = true;
			continue;
		}
					
		if ((!(*((*gHistories)[which].theData))[i].deleted) && bytes > 0 && !(err = FSWriteP(refN,aliasCmd)))
		{
			HLock(tempHandle);
			if (!(err = AWrite(refN,&bytes,*tempHandle)))
			{
				bytes = 1;
				err = AWrite(refN,&bytes,"\015");
			}
			HUnlock(tempHandle);
		}
	}

	UL((*gHistories)[which].theData);

	GetFPos(refN,&bytes);
	SetEOF(refN,bytes);
	MyFSClose(refN); refN = 0;

	/* do the deed */
	if (!err) err = ExchangeAndDel(&tmpSpec,&spec);
	if (!err) (*gHistories)[which].dirty = False;

	WriteHistTOC(which);

done:
	if ((*gHistories)[which].theData) UL((*gHistories)[which].theData);
	if (refN) MyFSClose(refN);
	if (err) FSpDelete(&tmpSpec);
	return(err);
}

/**********************************************************************
 * WriteHistTOC - write the toc for a history file
 **********************************************************************/
OSErr WriteHistTOC(short which)
{
	uLong	fileModDate;
	FSSpec	spec = (*gHistories)[which].spec;
	OSErr	err = noErr;
	short refN;
	HistoryStructHandle theData;
	short oldResF = CurResFile();

#ifdef	DEBUG
	if (InAThread()) return noErr;
#endif

	if (TimeToCompactTOC(which)) CompactHistTOC(which);

	theData = DupHandle((*gHistories)[which].theData);
	if ((theData) && ((err=MemError())==noErr))
	{
		IsAlias(&spec,&spec);
		KillNickTOC(&spec);
		FSpCreateResFile(&spec,CREATOR,'TEXT',smSystemScript);
		fileModDate = AFSpGetMod(&spec);

		if (-1!=(refN=FSpOpenResFile(&spec,fsRdWrPerm)))
		{
			AddResource(theData,LINK_TOC_TYPE,LINK_RESID,"");
			err = ResError();
			if (!err)
			{
				UpdateResFile(refN);
				err = ResError();
			}
			CloseResFile(refN);
		}
		else err = ResError();

		if (!err) err = AFSpSetMod(&spec,fileModDate);
		
		UseResFile (oldResF);
	}
	
	return (err);
}

/************************************************************************
 *  GetHistoryData - Get the url or image of a given history
 ************************************************************************/
Handle GetHistoryData(short which,short index,Boolean readFromDisk)
{
	Handle	tempHandle;
	HistoryStructHandle histories = (*gHistories)[which].theData;
	FSSpec	spec;
	Boolean finished = false;
	int err;
	Str255 line;
	short type;
	Boolean exLine=False;
	long len;
	Handle	dataHandle;
	LineIOD lid;
	long	theOffset;
	Str31	theCmd;
	
	spec = (*gHistories)[which].spec;

	if (index < 0 || which < 0 || index >= HistoryCount(which) || (*histories)[index].deleted)
		return (nil);

	tempHandle = (*histories)[index].hUrl;

	if (!readFromDisk)
	{
		if ((*histories)[index].dirty)
			return (tempHandle);
			
		if (tempHandle !=nil && *tempHandle !=nil)
			return (tempHandle);
	}
			
	if (tempHandle != nil)
		ZapHandle(tempHandle);

	theOffset = (*histories)[index].urlOffset;
	GetRString(theCmd,LINK_CMD);

	if ( theOffset >= 0)
	{
		if (err=FSpOpenLine(&spec,fsRdPerm,&lid))
		{
			if (err !=fnfErr) FileSystemError(OPEN_LINK_HISTORY,spec.name,err);
			return (nil);
		}
		
		dataHandle = NuHTempOK(0L);
		if (!dataHandle)
		{
			WarnUser(LINK_HISTORY_GET_DATA_ERR,MemError());
			return (nil);

		}
		
		/*
		 * Offset is from the beginning of the line; add in length of the command and length of name
		 * and two spaces
		 */
		theOffset += *theCmd + 1;
		SeekLine(theOffset,&lid);
		type = GetLine(line,sizeof(line),&len,&lid);
		if (type==LINE_START)
		do
		{
			// process current line
			len = strlen(line);
			exLine = NeatenLine(line,&len);
			if (exLine && !issep(*line)) // If line was escaped and the first character isn't a space, add one
			{
				if (err=PtrPlusHand_(" ",dataHandle,1)) break;
			}
			if (err=PtrPlusHand_(line,dataHandle,len)) break;
			
			// grab the next line; may or may not be ours
			type = GetLine(line,sizeof(line),&len,&lid);
			if (exLine && type==LINE_START) type = LINE_MIDDLE;	// extended line means new line is really part of this line
		}
		while (type==LINE_MIDDLE);
		
		CloseLine(&lid);
		if (err)
		{
			WarnUser(LINK_HISTORY_GET_DATA_ERR,err);
			ZapHandle(dataHandle);
			return (nil);
		}

		(*histories)[index].hUrl = dataHandle;

		UL(dataHandle);
		return (dataHandle);
	}
	else	
		return (nil);
}

/************************************************************************
 * ReadHistTOC - read in the history list TOC
 ************************************************************************/
OSErr ReadHistTOC(short which)
{
	OSErr err = noErr;
	FSSpec lSpec = (*gHistories)[which].spec;
	Boolean sane;
	short refN=0;
	short oldResF = CurResFile();
	HistoryStructHandle theToc = nil;
	UHandle hand = nil;
		
	//
	//	Open the History file
	//
	
	IsAlias(&(*gHistories)[which].spec,&lSpec);	
	if (err=FSpRFSane(&lSpec,&sane)) return(-1);
	if (!sane)
	{
		FSpKillRFork(&lSpec);
		return (-1);
	}
	else if (-1!=(refN=FSpOpenResFile(&lSpec,fsRdPerm)))
	{		
		// Clear out the old handle
		if ((*gHistories)[which].theData) ZapHandle((*gHistories)[which].theData);
		
		//
		// Read to TOC handle out of the file
		//
				
		theToc = Get1Resource(LINK_TOC_TYPE,LINK_RESID);
		if (noErr==(err=ResError()))
		{ 	
			short i, count;
			
			if (theToc != nil)
			{
				// is this toc the right version?
				if (CorrectVersion(theToc))
				{
					DetachResource(theToc);
					(*gHistories)[which].theData = theToc;
					
					// iterate through the toc and reset garbage fields
					count = GetHandleSize_(theToc)/sizeof(HistoryStruct);
					for (i = 0; i < count; i++) (*theToc)[i].hUrl = nil;
				}
				else
				{
					// wrong version of the toc. Lose the history.
					theToc = nil;
				}
			}
		}
		
		// got nothing from the resource file
		if (theToc == nil)
		{
			// create a new, empty handle.  No history entries are defined.
			hand = NuHTempOK(0L);
			err = MemError();
			if (hand && (err==noErr)) (*gHistories)[which].theData = (HistoryStructHandle)hand;
		}
		
		// was there an error?
		if (err != noErr)
		{
		 	WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,err);
		 	ZapHandle((*gHistories)[which].theData);
		 	(*gHistories)[which].theData = nil;
		}
	}
		
	if (refN) CloseResFile(refN);
	UseResFile (oldResF);

	return(err);
} 

/************************************************************************
 * CorrectVersion - is this HistoryStructHandle something we can handle?
 ************************************************************************/
Boolean CorrectVersion(HistoryStructHandle theToc)
{
	Boolean result = false;
	
	if (theToc)
	{
		if (GetHandleSize(theToc) > 0)
		{
			if ((*theToc)->version == LINK_HISTORY_VERSION)
				result = true;
		}
	}
	
	return (result);
}

/************************************************************************
 * HistMatchFound - find a given history in a history file
 ************************************************************************/
long HistMatchFound(long hashName,Handle theUrl,short which)
{
	long i;
	Handle hUrl = nil;
	URLNameStr tempStr, tempName;
	HistoryStructHandle theHistories = (*gHistories)[which].theData;
	long stop = (GetHandleSize_(theHistories)/sizeof(HistoryStruct));
	Boolean	needStringMatch = false;
	long matched = -1;
	HistoryStruct *theStruct, *endStruct;
	
	endStruct = (*theHistories)+stop;
	for (theStruct=*theHistories; theStruct<endStruct; theStruct++)
		if (theStruct->hashName == hashName && !theStruct->deleted)
			if (matched==-1) matched = theStruct-*theHistories;		
			else {needStringMatch = True; break;}

	if (!needStringMatch) return(matched);
	
	// two names with equal hash values found, compare names.
	PSCopy(tempName,*theUrl);
	*tempName = RemoveChar(' ',tempName+1,*tempName);
	for (i=0;i<stop;i++)
	{
		if ((*theHistories)[i].hashName == hashName && !(*theHistories)[i].deleted)
		{
			hUrl = GetHistoryData(which,i,true);	// don't zap the returned handle.  It's a part of the history toc.
			if (hUrl)
			{
				PCopy(tempStr, *hUrl);
				*tempStr = RemoveChar(' ',tempStr+1,*tempStr);
				if (StringSame(tempName,tempStr))
					return (i);
			}
		}
	}

	return (-1);	
}

/************************************************************************
 * DeleteHistEntryFromTOC - delete a history entry from the TOC.
 ************************************************************************/
void DeleteHistEntryFromTOC(short which, short index)
{
	(*(*gHistories)[which].theData)[index].deleted = true;
	(*gHistories)[which].dirty = true;
	
	if ((*(*gHistories)[which].theData)[index].type == ltAd)	// did we just delete an ad?
		if ((*(*gHistories)[which].theData)[index].thumb!=0)	// does it think it has a thumbnail?
			DeleteAdGraphic((*(*gHistories)[which].theData)[index].adId);
}

/************************************************************************
 * TimeToCompactTOC - is it worth our time to compact this TOC?
 ************************************************************************/
Boolean TimeToCompactTOC(short which)
{
	short deletedCount = 0;
	short historyCount = HistoryCount(which);
	HistoryStructHandle histories = (*gHistories)[which].theData;
	
	while (historyCount > 0)
	{
		if ((*histories)[historyCount-1].deleted) deletedCount++;
		historyCount--;
	}
	
	return (deletedCount >= COMPACT_THRESHHOLD);
}
 	
 /************************************************************************
 * CompactHistTOC - remove deleted entries from the TOC.
 ************************************************************************/
OSErr CompactHistTOC(short which)
{
	OSErr err = noErr;
	Accumulator histAccu;
	short count = 0, historyCount = HistoryCount(which);
	HistoryStructHandle histories = (*gHistories)[which].theData;
			
	if ((err = AccuInit(&histAccu))==noErr)
	{
		while ((count != historyCount) && (err==noErr))
		{
			if ((*histories)[count].deleted)
				;	// skip this TOC entry.  It's deleted.
			else
			{
				// add this history entry to the Accumulator
				err = AccuAddPtr(&histAccu, &(*histories)[count], sizeof(HistoryStruct));
			}
			count++;
		}
		
		if (err == noErr)
		{
			AccuTrim(&histAccu);
			ZapHandle((*gHistories)[which].theData);
			(*gHistories)[which].theData = histAccu.data;
			histAccu.data = nil;
		}
		
		AccuZap(histAccu);
	}
		
	return (err);
}

/************************************************************************
 * HistoryCount - return # of history entries
 ************************************************************************/
short HistoryCount(short which)
{
	if ((*gHistories)[which].theData)
		return ((GetHandleSize_((*gHistories)[which].theData)/sizeof(HistoryStruct)));
	else
		return (0);
}

/************************************************************************
 * AddAllHistoryItems - add all the history items to the list
 ************************************************************************/
void AddAllHistoryItems(ViewListPtr pView, Boolean needsSort, LinkSortTypeEnum sortType)
{
	OSErr err = noErr;
	short i,count,cur = 0;
	VLNodeInfo	info;
	ShortHistoryStructHandle histories = NULL;
	Boolean reverseSort = (sortType&kLinkReverseSort)!=0;
	
	// get a sorted list of all history entries to add
	err = BuildListOfHistoriesForWindow(&histories, needsSort, sortType);
	if ((err==noErr) && histories)
	{
		count = GetHandleSize(histories)/sizeof(ShortHistoryStruct);
		for (i=0;i<count;i++)
		{
			cur = reverseSort ? (count - 1 - i) : i;
			
			Zero(info);
			PStrCopy(info.name,(*histories)[cur].name,sizeof(info.name));
			info.nodeID = (*histories)[cur].nodeId;
			info.iconID = LinkTypeToIconID((*histories)[cur].type);
			LVAdd(pView, &info);				
		}
		
		// cleanup, don't need this anymore.
		ZapHandle(histories);
	}
}

/************************************************************************
 * ShortHistoryStructHandle - return a sorted list of structures that
 *	contain just enough information to add to the history window.
 ************************************************************************/
OSErr BuildListOfHistoriesForWindow(ShortHistoryStructHandle *histories, Boolean needsSort, LinkSortTypeEnum sortType)
{
	OSErr err = noErr;
	short which, i;
	short numEntries, count;
	int (*compare)() = nil;
	
	*histories = nil;
	
	// How many history entries are there in all files?
	numEntries = 0;
	for (which = 0; which < NHistoryFiles; which++)
		numEntries += HistoryCount(which);

	// Make a new handle that's big enough for them all
	*histories = NuHandle(numEntries*sizeof(ShortHistoryStruct));
	err = MemError();
	if (*histories && (err == noErr))
	{
		// fill the new handle with all of the entries
		count = 0;
		for (which = 0; which < NHistoryFiles; which++)
		{
			for (i = 0; i < HistoryCount(which); i++)
			{
				// only add non-deleted items
				if (!((*(*gHistories)[which].theData)[i].deleted) && !((*(*gHistories)[which].theData)[i].incompleteAd))
				{
					PCopy((**histories)[count].name,(*(*gHistories)[which].theData)[i].name);				
					(**histories)[count].cacheSeconds = (*(*gHistories)[which].theData)[i].cacheSeconds;	
					(**histories)[count].type = (*(*gHistories)[which].theData)[i].type;		
					(**histories)[count].nodeId = EntryToNodeId(which, i);	
					(**histories)[count].label = (*(*gHistories)[which].theData)[i].label;
					count++;
				}
			}
		}
		
		// Resize the handle down if we hafta.
		if (count != numEntries) SetHandleSize(*histories, count*sizeof(ShortHistoryStruct));
		
		// Sort the handle if we need to
		if (needsSort) 
		{
			switch (sortType&kLinkSortTypeMask)
			{
				case sType:
					compare = HistTypeCompare;
					break;
					
				case sName:
					compare = HistNameCompare;
					break;
					
				case sDate:
					compare = HistDateCompare;
					break;
					
				case sSpecialRemind:
					compare = HistRemindCompare;
					break;
			}
			
			if (compare) SortShortHistoryHandle(*histories, compare);
		}
	}
	else
	{
		WarnUser(MEM_ERR,err);
		*histories = nil;
	}
		
	return (err);
}

/************************************************************************
 * LinkType - Given a protocol, return the type of link
 ************************************************************************/
 LinkTypeEnum LinkType(ProtocolEnum protocol, PStr proto)
 {
 	LinkTypeEnum type = ltHttp;	// assume web link
 	Str255 scratch;
 	
	switch (protocol)
	{
		case proFinger:
		case proPh:
		case proPh2:
		case proLDAP:
			type = ltDirectoryServices;
			break;
			
		case proMail:
			type = ltMail;
			break;
			
		case proFile:
		case proCompFile:
			type = ltFile;
			break;
			
		case proSetting:
			type = ltSetting;
			break;
			
		default:
		{
			// make sure this isn't an ftp link
			if (StringSame(proto, GetRString(scratch,ANARCHIE_FTP)))
				type = ltFtp;
			break;
		}
	}
	
	return (type);
 }

/************************************************************************
 * LinkTypeToIconID - Given a protocol, return the icon to be used
 ************************************************************************/
short LinkTypeToIconID(LinkTypeEnum type)
{
	short iconID;
	
	if (type==ltAd)
	{
		// Use the thumbnail if present, a web link otherwise.
		iconID = kCustomIconResource;
	}
	else
	{
		// assign an icon based on the type of URL this is
		switch (type)
		{
			case ltDirectoryServices:
				iconID = MenuItemNameToIconID(WINDOW_MENU,WIN_PH_ITEM);
				break;
				
			case ltMail:
				iconID = MAILTO_LINK_TYPE_ICON;
				break;
			
			case ltHttp:
				iconID = HTTP_LINK_TYPE_ICON;
				break;
				
			case ltFtp:
				iconID = FTP_LINK_TYPE_ICON;
				break;
				
			case ltFile:
				iconID = MenuItemNameToIconID(FILE_MENU,FILE_SAVE_AS_ITEM);
				break;
				
			case ltSetting:
				iconID = SETTINGS_ICON;
				break;
				
			default:
				iconID = DEFAULT_LINK_TYPE_ICON;	// assume it's a web link
				break;
		}
	}
		
	return (iconID);
}

/************************************************************************
 * MenuItemNameToIconID - return the icon id of a menu item by name
 ************************************************************************/
short MenuItemNameToIconID(short menuID, short item)
{
	MenuHandle mh;
	short id = DEFAULT_LINK_TYPE_ICON;
	Str255 itemText;
		
	if (mh=GetMHandle(menuID))
	{
		GetMenuItemText(mh, item, itemText);
		id = Names2Icon(itemText,"\p");
	}
	
	return (id);
}

/************************************************************************
 * EntryToNodeId - create an ID out of file # and index
 ************************************************************************/
VLNodeID EntryToNodeId(short which, short index)
{
	VLNodeID id;
	
	id = (which*0x10000) + index;
	
	return (id);
}

/************************************************************************
 * NodeIdToEntry - return the file # and index of a given id
 ************************************************************************/
void NodeIdToEntry(VLNodeID id, short *which, short *index)
{
	*which = HiWord(id);
	*index = LoWord(id);
}

/************************************************************************
 * DeleteHistoryEntry - delete a history entry from the History window
 ************************************************************************/
void DeleteHistoryEntry(VLNodeInfo *info)
{
	short which;
	short index;
	
	NodeIdToEntry(info->nodeID, &which, &index);
	DeleteHistEntryFromTOC(which, index);
}
	
/************************************************************************
 * OpenHistoryEntry - open a history entry from the History window
 ************************************************************************/
OSErr OpenHistoryEntry(VLNodeInfo *info)
{
	short which;
	short index;
	Handle hUrl;
	Str255	proto;
	short len;
	OSErr openErr = noErr;
			
	// Figure out which file this entry is in ...
	NodeIdToEntry(info->nodeID, &which, &index);

	// read in the URL from the file
	// Note: GetHistoryData returns a handle that belongs to the history toc struct.  Don't trash it!
	if ((*(*gHistories)[which].theData)[index].dirty) hUrl = GetHistoryData(which,index,false);
	else hUrl = GetHistoryData(which,index,true);
	
	if (hUrl && (len = GetHandleSize(hUrl)))
	{
		char *url = LDRef(hUrl);
		
        // parse and open this URL     
		if (fnfErr==(openErr=OpenLocalURLPtr(url,len,nil,nil,false)))
			if ((openErr=ParseProtocolFromURLPtr(url,len,proto))==noErr)
				openErr = OpenOtherURLPtr(proto,url,len);
		UL(hUrl);
		
		// update the link's label
		UpdateLinkLabel(&((*(*gHistories)[which].theData)[index]), openErr);

		// Update the date in the History TOC
		LinkUpdateCacheDate(which, index);
		
		// Update the Link history window
		LinkTickle();
	}	
	
	return (openErr);
}	

/************************************************************************
 * GetDateString - given the ID of a node, return the date in a string
 ************************************************************************/
Boolean GetDateString(VLNodeID id, Str255 dateStr)
{
	short which;
	short index;
	Str31 zone;
	
	// Find the history entry
	NodeIdToEntry(id, &which, &index);
	
	// Does this entry have a label?
	if ((*(*gHistories)[which].theData)[index].label!=llNone)
	{
		LabelToString((*(*gHistories)[which].theData)[index].label,dateStr);
	}
	else
	{	
		// return the standard date string
		*zone = 0;
		SecsToLocalDateTime((*(*gHistories)[which].theData)[index].cacheSeconds-ZoneSecs(), ZoneSecs(), zone, dateStr);
	}
	
	return (true);
}

/************************************************************************
 * IsMarkedRemind - is this cell one we need to remind the user about?
 ************************************************************************/
Boolean IsMarkedRemind(VLNodeID id)
{
	Boolean result = false;
	short which;
	short index;
	
	NodeIdToEntry(id, &which, &index);
	result = (*(*gHistories)[which].theData)[index].label == llRemindMe;
	
	return (result);
}

/**********************************************************************
 * LabelToString - given a history label, return the string of that
 *	label.
 **********************************************************************/
void LabelToString(LinkLabelEnum label, Str255 dateStr)
{
	GetRString(dateStr,LinkHistoryLabelsStrn+label);
}

/**********************************************************************
 * LabelableLink - return true if this is a link type we should label
 **********************************************************************/
Boolean LabelableLink(LinkTypeEnum linkType)
{
	Boolean result = false;
	LinkTypeEnum scan;
	short count = sizeof(gLabelTheseLinks)/sizeof(LinkTypeEnum);
	
	for (scan = 0; !result && (scan < count); scan++)
	{
		if (gLabelTheseLinks[scan]==linkType)
			result = true;
	}
	
	return (result);
}

/**********************************************************************
 * GetLHPreviewIcon - return a handle to the icon for this id.
 **********************************************************************/
Handle GetLHPreviewIcon(VLNodeID id)
{
	Handle theIcon = nil, rIconSuite = nil;
	short which;
	short index;
	AdId adId;
	FSSpec adGraphicSpec;
	URLNameStr adGraphicName;
	OSErr err = noErr;
	LHPIconCacheHandle iconCache;
	
	if (LinkHasCustomIcons())
	{
		// Find the history entry
		NodeIdToEntry(id, &which, &index);
		
		// Make sure it's an ad
		if ((*(*gHistories)[which].theData)[index].type == ltAd)
		{	
			adId = (*(*gHistories)[which].theData)[index].adId;
			
			// Is the ad icon already loaded?
			if (iconCache=FindPVICache(adId))
			{
				theIcon = (*iconCache)->theIcon;
				if (theIcon && *theIcon)
				{
					HNoPurge(theIcon);	// don't purge this again until we're done with it.
					return (theIcon);
				}
				// else
					// the icon has been purged.  Read it back in.
			}

			// Find the icon preview file
			AdIdToName(adId, adGraphicName);
			if (noErr == FSMakeFSSpec(gLinkHistoryFolder.vRefNum,gLinkHistoryFolder.parID,adGraphicName,&adGraphicSpec))
			{
				short iconRes, oldResFile = CurResFile();
				
				// open the file
				iconRes = FSpOpenResFile(&adGraphicSpec,fsRdPerm);
				if (iconRes != -1)
				{										
					// read in the icon
					err = GetIconSuite(&rIconSuite, kCustomIconResource, svAllAvailableData);
					if (rIconSuite && *rIconSuite && (err==noErr))
					{	
						err = DupIconSuite(rIconSuite, &theIcon, false);
						if (err == noErr)
						{
							// Remember to mark it as purgable when we're done with it!
							HNoPurge(theIcon);
							
							// Add this icon to the list of loaded icons
							AddIconToPVICache(theIcon, adId);
						}
					}
					else theIcon = nil;
					
					CloseResFile(iconRes);
				}
				UseResFile(oldResFile);
			}
		}
	}		
	return (theIcon);
}

/************************************************************************
 * LinkUpdateCacheDate - this link was clicked on today
 ************************************************************************/
void LinkUpdateCacheDate(short which, short index)
{
	// make sure the link histories are around
	if (gHistories || (GenHistoriesList()==noErr))
	{
		// update the date for this TOC entry to today
		(*(*gHistories)[which].theData)[index].cacheSeconds = LocalDateTime();
		
		// save the TOC part of the file only
		WriteHistTOC(which);
	}
}

/************************************************************************
 * SortShortHistoryHandle - sort the monster link history handle
 ************************************************************************/
void SortShortHistoryHandle(ShortHistoryStructHandle toSort, int (*compare)())
{
	short count = 0;

	if (compare)
	{
		count = GetHandleSize(toSort)/sizeof(ShortHistoryStruct);
		QuickSort((void*)LDRef(toSort),sizeof(ShortHistoryStruct),0,count-1,(void*)compare,(void*)SwapHist);
		UL(toSort);
	}
}

/**********************************************************************
 * HistTypeCompare - compare two cells based on the url type
 **********************************************************************/ 
int HistTypeCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2)
{
	return (hist1->type - hist2->type);
}

/**********************************************************************
 * HistNameCompare - compare two cells based on the url itself
 **********************************************************************/ 
int HistNameCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2)
{
	return (StringComp(hist1->name,hist2->name));
}

/**********************************************************************
 * HistDateCompare - compare two cells based on the cache date
 **********************************************************************/ 
int HistDateCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2)
{
	int result;
	
	// does either entry have a label?
	if ((hist1->label != llNone) || (hist2->label != llNone))
	{
		// the entry with the most important label is newest ...
		result = hist1->label - hist2->label;
	}
	else
	{
		// the entry with the largest cacheSecods is newest ...
		result = hist2->cacheSeconds - hist1->cacheSeconds;
	}
	
	return (result);
}

/**********************************************************************
 * HistRemindCompare - compare two cells based on the cache date. 
 *	Special case for Remind sorting.
 **********************************************************************/ 
int HistRemindCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2)
{
	int result;
	
	// does either entry have a Remind Me label?
	if ((hist1->label == llRemindMe) || (hist2->label == llRemindMe))
	{
		// both are set to Remind Me
		if ((hist1->label == llRemindMe) && (hist2->label == llRemindMe)) result = HistNameCompare(hist1, hist2);
		// one is set to Remind Me
		else if (hist1->label == llRemindMe) result = -1;
		else result = 1;
	}
	else
	{
		// the entry with the largest cacheSecods is newest ...
		result = hist2->cacheSeconds - hist1->cacheSeconds;
	}
	
	return (result);
}

/**********************************************************************
 * SwapHist - swap two history entries.
 **********************************************************************/ 
void SwapHist(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2)
{
	ShortHistoryStruct tempHist;
	
	tempHist = *hist1;
	*hist1 = *hist2;
	*hist2 = tempHist;
}

/************************************************************************
 * GetLinkURL - Return a handle to the URL of a link.  (*gHistories)[which] URL should
 *	NOT be dumped, even after a one night stand.
 ************************************************************************/
Handle GetLinkURL(VLNodeInfo *info)
{
	short which;
	short index;
	Handle hUrl;
	
	// Figure out which file this entry is in ...
	NodeIdToEntry(info->nodeID, &which, &index);

	// read in the URL from the file
	// Note: GetHistoryData returns a handle that belongs to the history toc struct.  Don't trash it!
	if ((*((*gHistories)[which].theData))[index].dirty) hUrl = GetHistoryData(which,index,false);
	else hUrl = GetHistoryData(which,index,true);
		
	return (hUrl);
}

/**********************************************************************
 * AdWasClicked - call this when an Ad is clicked on in the Ad window
 **********************************************************************/
void AdWasClicked(AdId adId, OSErr openErr)
{
	short which, index;

	if (LocateAdInHistories(adId, &which, &index))
	{	
		// Update the label of this ad.
		UpdateLinkLabel(&((*(*gHistories)[which].theData)[index]), openErr);
		
		// Give it a new cache date when found.
		LinkUpdateCacheDate(which, index);
		
		// update the Link History window
		LinkTickle();
	}
}

/**********************************************************************
 * LocateAdInHistories - find the ad in the histories files
 **********************************************************************/
Boolean LocateAdInHistories(AdId adId, short *which, short *index)
{
	short count;
	Boolean foundIt = false;
	short w, i;
	
	// must have somewhere to put our results
	if (!which || !index) return (false);
	
	// Make sure the histories are around ...
	if (gHistories || (GenHistoriesList()==noErr))
	{
		// Iterate through all History files, looking for this Ad.
		for (w = 0; !foundIt && (w < NHistoryFiles); w++)
		{
			count = HistoryCount(w);
			for (i = 0; !foundIt && (i < count); i++)
			{
				if (((*(*gHistories)[w].theData)[i].adId.server == adId.server)
					&& ((*(*gHistories)[w].theData)[i].adId.ad == adId.ad))
				{
			 		*which = w;
					*index = i;
					foundIt = true;
			 	}
			}
		}
	}	
	
	return (foundIt);
}

/**********************************************************************
 * FindRemindLink - see if any links are marked as remind me
 **********************************************************************/
Boolean FindRemindLink(void)
{
	short count;
	Boolean foundOne = false;
	short w, i;
	
	// Make sure the histories are around ...
	if (gHistories || (GenHistoriesList()==noErr))
	{
		// Iterate through all History files
		for (w = 0; !foundOne && (w < NHistoryFiles); w++)
		{
			count = HistoryCount(w);
			for (i = 0; !foundOne && (i < count); i++)
			{
				if ((*(*gHistories)[w].theData)[i].remind)
					foundOne = true;
			}
		}
	}	
	
	return (foundOne);
}

/**********************************************************************
 * UnRemindLinks - forget about it!
 **********************************************************************/
void UnRemindLinks(Boolean labelToo)
{
	short count;
	short w, i;
	Boolean foundOne;
	
	// Make sure the histories are around ...
	if (gHistories || (GenHistoriesList()==noErr))
	{
		// Iterate through all History files
		for (w = 0; (w < NHistoryFiles); w++)
		{
			count = HistoryCount(w);
			foundOne = false;
			for (i = 0; (i < count); i++)
			{
				if ((*(*gHistories)[w].theData)[i].remind)
				{
					foundOne = true;
					(*(*gHistories)[w].theData)[i].remind = 0;
				}
				
				if (labelToo && ((*(*gHistories)[w].theData)[i].label==llRemindMe)) 
				{
					foundOne = true;
					(*(*gHistories)[w].theData)[i].label = llBookmarked;
				}
			}
			if (foundOne) WriteHistTOC(w);
		}
	}	
}

/**********************************************************************
 * AddAdToLinkHistory - call this to add an ad to the Link History
 *
 *	If adURL is non-null, we're adding an ad to the history list
 *	if adGraphic is non-nul, we're creating a preview for the lw window
 **********************************************************************/
OSErr AddAdToLinkHistory(AdId adId, StringPtr pUrl, Str255 adTitle, FSSpecPtr adGraphic)
{
	OSErr err = noErr;
	short which = 0, index;
	ProtocolEnum protocol;
	Str255 proto,host,query;
	OSErr labelableErrors[] = {fnfErr};
	long hashName;
	Handle hUrl = nil;
	URLNameStr urlName;
	Boolean needSave = false;
	
	//	Add the Ad to the link History Window
	if (pUrl && *pUrl && adTitle)
	{	
		//	Add the ad's URL to the link history file.
		if (!(err=ParseURL(pUrl,proto,host,query)))
		{
			protocol = FindSTRNIndex(ProtocolStrn,proto);
			FixURLString(host);
			if (protocol!=proMail) FixURLString(query);
		
			// make sure the name of the url contains something interesting
			if (adTitle && adTitle[0])
			{
				// use the name passed in
				PStrCopy(urlName, adTitle, sizeof(urlName));
			}
			else
			{
				if (host[0]) 	
				{
					// use the host of the actual url as the name		
					PCopy(urlName, host);
				}
				else
				{
					// URL had no host.  Use the actual URL
					PStrCopy(urlName, pUrl, sizeof(urlName));
				}
			}

			// The name of the history entry will be a hash of the url itself
			hashName = NickHashString(pUrl);
			
			// Turn the url into a handle we can keep around ...
			hUrl = NuHTempOK(0);
			if (!hUrl) return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,MemError()));
			err = PtrPlusHand(pUrl+1,hUrl,pUrl[0]);
			if (err) return(WarnUser(LINK_HISTORY_NEW_HISTORY_ERR,err));
			
			// Make sure the history files are around somewhere
			err = GenHistoriesList();
		
			// See if the ad already exists in a history file ...	
			if (LocateAdInHistories(adId, &which, &index))
			{
				// we'll need to save these changes if they were major ...
				if ((*(*gHistories)[which].theData)[index].incompleteAd) needSave = true;
				
				// update it with the new history information
				(*(*gHistories)[which].theData)[index].hashName = hashName;
				//(*(*gHistories)[which].theData)[index].cacheSeconds = LocalDateTime();	// - leave the date alone jdboyd 2/9/01
				(*(*gHistories)[which].theData)[index].dirty = true;
				(*(*gHistories)[which].theData)[index].incompleteAd = false;
				(*(*gHistories)[which].theData)[index].hUrl = hUrl;
				PCopy((*(*gHistories)[which].theData)[index].name,urlName);	
			}
			else
			{
				// add the ad as a new history entry
				if (err == noErr)
				{
					// Add the history to the history file
					err =  AddHistoryToTOC(MAIN_HISTORY_FILE, urlName, hashName, ltAd, llNotDisplayed, false, hUrl, adId);
					if (err == noErr) needSave = true;
				}
			}
		}
	}
	
	// Add the graphic to the Link History, if all has gone well
	if ((err == noErr) && adGraphic && LinkHasCustomIcons())
	{
		// Is the ad graphic a valid graphic file?
		if (FSpFileSize(adGraphic) > 0)
		{
			// Make sure the history files are loaded ...
			err = GenHistoriesList();
			
			if (err == noErr)
			{
				// Create an icon out of the graphic, save it in the History folder.
				if (CreateIconFromAdGraphic(adId, adGraphic) == noErr)
				{
					// find the ad this graphic belongs to
					if (!LocateAdInHistories(adId, &which, &index))
					{
						// it does not exist.  Add it now, using a bogus name and hashname
						err =  AddHistoryToTOC(MAIN_HISTORY_FILE, "\p", -1, ltAd, llNotDisplayed, true, hUrl, adId);
					}
					else
					{
						// (*gHistories)[which] history entry now has a thumbnail ...
						(*(*gHistories)[which].theData)[index].thumb = true;
					}
					if (err == noErr) needSave = true;
				}
			}		
		}
		// else
			// the ad graphic file was 0 bytes in length.  Nothing to do, we'll get called later when it's downloaded.
	}
	
	// save the history file
	if (needSave)
	{
		(*gHistories)[which].dirty = true;
		err = SaveIndHistoryFile(which);
		
		// Update the Link History window
		LinkTickle();
	}

	return (err);
}

/**********************************************************************
 * AgeLinks - go through history files, throw out old links.
 *	Warning: This is expensive!
 **********************************************************************/	
void AgeLinks(void)
{
	short which;
	short totalRemoved = 0;
	static long lastAged;
	
	// Nothing to do if there are no history files ...
	if (gHistories==nil) return;
	
	// Only do this once every hour ...
	if (lastAged == 0) lastAged = TickCount();
	else
	{	
		if ((TickCount() - lastAged) < AGE_INTERVAL)
			return;
	}
	
	for (which = 0; which < NHistoryFiles; which++)
		totalRemoved += AgeHistoryFile(which);
	
	// refresh the Link History window if we removed any entries
	if (totalRemoved > 0) 
		LinkTickle();
		
	// Trash orphaned link history previews
	PurgeLinkHistoryPreviewOrphans();
	
	// Let's not do this anytime soon ...
	lastAged = TickCount();
}

/**********************************************************************
 * AgeHistoryFile - go through a history file, throw out old links
 **********************************************************************/	
short AgeHistoryFile(short which)
{
	short count = HistoryCount(which);
	short i;
	long age;
	long maxAge = 60*60*24*GetRLong(LINK_AGE);
	long today;
	short removed = 0;
	
	// when is today?
	today = GMTDateTime();
	
	// Go through each history entry ...
	for (i = 0; i < count; i++)
	{
		// skip deleted entries.  They are already gone ...
		if ((*(*gHistories)[which].theData)[i].deleted == 0)
		{
			//
		 	//	Age Ads only if they're not on the current playlist
		 	//	Treat ads differently only for adware users that *have* a playlist.
		 	//
		 		
			if (IsAdwareMode() && (*(*gHistories)[which].theData)[i].type == ltAd)
			{		 
		 		if (IsAdInPlaylist((*(*gHistories)[which].theData)[i].adId))
					continue;
		 	}
		 	 	
	 		//
	 		//	Throw out any links older than a LINK_AGE days
	 		//
	 		
			age = today - (*(*gHistories)[which].theData)[i].cacheSeconds;
			if (age > maxAge) 
			{
				DeleteHistEntryFromTOC(which, i);
				removed++;
			}
		}
	}
	
	// Save the history TOC if we removed anything.
	if (removed > 0) WriteHistTOC(which);
	
	return (removed);
}

/**********************************************************************
 * PurgeLinkHistoryPreviewOrphans - iterate through the Link History
 *	folder, and trash preview files no longer in use.
 **********************************************************************/
void PurgeLinkHistoryPreviewOrphans(void)
{
	URLNameStr name;
	CInfoPBRec hfi;
	short count = 1;
	AdId adId;
	short which, index;
	
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(gLinkHistoryFolder.vRefNum,gLinkHistoryFolder.parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==LINK_HISTORY_PREVIEW_TYPE && hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			// Found a preview icon.  Figure out the name of the file
			if (NameToAdId(name, &adId))
			{
				// Is this ad on in any history file?
				if (!LocateAdInHistories(adId, &which, &index))
				{
					// It's not.  Delete thje graphic from the folder.
					if (DeleteAdGraphic(adId)==noErr)
						hfi.hFileInfo.ioFDirIndex--;
					//else
						//error deleting it.  Skip it, try it next time we purge.
				}
			}
		}
	}
}
	
/**********************************************************************
 * CreateIconFromAdGraphic - given an Ad, create a file with an icon
 *	representing the ad.
 **********************************************************************/
OSErr CreateIconFromAdGraphic(AdId adId, FSSpecPtr adGraphic)
{
	OSErr err = noErr;
	URLNameStr graphicName;
	FSSpec adIconSpec;
	
	// The name of the graphic file will be the adId.
	AdIdToName(adId, graphicName);
	
	// See if the graphic exists already;
	if (FSMakeFSSpec(gLinkHistoryFolder.vRefNum, gLinkHistoryFolder.parID, graphicName, &adIconSpec)!=noErr)
	{
		// the icon does not yet exist.  Create it from the adGraphic file.
		err = IconFromAd(&adIconSpec, adGraphic);
	}
	else err = dupFNErr;
	
	return (err);
}
		
/**********************************************************************
 * AdIdToName - given an AdId, return the name of its preview file
 **********************************************************************/
void AdIdToName(AdId adId, URLNameStr name)
{	
	Str255 scratch;
	
	scratch[0] = 0;
	name[0] = 0;
	
	NumToString(adId.server,name);
	NumToString(adId.ad,scratch);
	PCat(name,"\p,");
	PCat(name,scratch);
}

/**********************************************************************
 * NameToAdId - given a name, return the id of the ad it belongs to
 **********************************************************************/
Boolean NameToAdId(URLNameStr name, AdId *ad)
{	
	Boolean result = false;
	PStr scan;
	Str255 scratch;
	
	ad->server = ad->ad = 0;
	
	if (name && name[0]) 
	{
		// the name is <server,ad>
		scan = PPtrFindSub("\p,", name, name[0]);
		if (scan)
		{
			// figure out the server id.
			PStrCopy(scratch,name,scan - name);
			StringToNum(scratch, &(ad->server));
			
			// figure out the ad id. 
			scratch[0] = name[0] - (scan - name);
			BlockMove(scan+1,scratch+1,scratch[0]);
			StringToNum(scratch, &(ad->ad));
			
			result = true;
		}
	};
		
	return (result);
}

/**********************************************************************
 * DeleteAdGraphic - given an AdId, delete its preview file
 **********************************************************************/
OSErr DeleteAdGraphic(AdId adId)
{
	OSErr err = noErr;
	FSSpec adGraphicSpec;
	URLNameStr adGraphicName;
	
	// locate the ad file in the Link History Folder
	AdIdToName(adId, adGraphicName);
	err = FSMakeFSSpec(gLinkHistoryFolder.vRefNum,gLinkHistoryFolder.parID,adGraphicName,&adGraphicSpec);
	if (err == noErr)
	{
		// Delete the ad preview we've found.
		err = FSpDelete(&adGraphicSpec);
		
		// Remove the icon handle from the icon cache
		RemoveIconFromPVICache(adId);
	}
	// else
		// the ad preview can't be found.  Too bad.
	
	return (err);
}

/**********************************************************************
 * AddIconToPVICache - remember a loaded ad preview icon
 **********************************************************************/
void AddIconToPVICache(Handle theIcon, AdId adId)
{
	LHPIconCacheHandle newPVI;
	
	newPVI = NewZH(LHPIconCacheStruct);
	if (newPVI)
	{
		(*newPVI)->theIcon = theIcon;
		(*newPVI)->adId = adId;
		LL_Queue(gPreviewIcons, newPVI, (LHPIconCacheHandle));
	}	
}

/**********************************************************************
 * FindPVICache - find an icon in the preview icon cache
 **********************************************************************/
LHPIconCacheHandle FindPVICache(AdId adId)
{
	LHPIconCacheHandle scan;
	
	for (scan = gPreviewIcons; scan && (((*scan)->adId.server != adId.server) || ((*scan)->adId.ad != adId.ad)); scan = (*scan)->next);
	
	return (scan);
}


/**********************************************************************
 * RemoveIconFromPVICache - remove an icon from the cache
 **********************************************************************/
void RemoveIconFromPVICache(AdId adId)
{
	LHPIconCacheHandle scan;
	
	// Locate the cache entry associated with this adId.
	for (scan = gPreviewIcons; scan; scan = (*scan)->next)
	{
		if (((*scan)->adId.server == adId.server) && ((*scan)->adId.ad == adId.ad))
		{
			RemovePVIFromPVICache(&scan);
			break;
		}
	}
}

/**********************************************************************
 * RemovePVIFromPVICache - remove a PVI cache entry frm the list
 **********************************************************************/
void RemovePVIFromPVICache(LHPIconCacheHandle *toRemove)
{
	if (gPreviewIcons && toRemove && *toRemove)
	{			
		// remove it from the list
		LL_Remove(gPreviewIcons,*toRemove,(LHPIconCacheHandle));
		
		// nuke the icon cache
		DisposeIconSuite((**toRemove)->theIcon,true);
		
		// and now the cache entry
		ZapHandle(*toRemove);
		*toRemove = nil;
	}
}

/**********************************************************************
 * ZapPVICache = destroy the icon cache
 **********************************************************************/
void ZapPVICache(void)
{
	LHPIconCacheHandle scan = gPreviewIcons, next = nil;
	
	while (scan)
	{
		next = (*scan)->next;
		RemovePVIFromPVICache(&scan);
		scan = next;
	}	
}

/**********************************************************************
 * IconFromAd - create a file containing an icon representing the
 *	picture in the adSpec.  This assumes the adSpec is a valid file.
 **********************************************************************/
OSErr IconFromAd(FSSpecPtr iconSpec, FSSpecPtr adSpec)
{
	OSErr err = noErr;
	GraphicsImportComponent	importer;
	MatrixRecord whatIsTheMatrix;
	GWorldPtr	gWorld = nil;
	short depth = 8;	// Listview supports 8 bit icons ...
	Rect r,srcRect;
		
	// figure out which importer can open the ad picture file ...
	err = GetGraphicsImporterForFile(adSpec, &importer);
	if ((err == noErr))
	{
		// Create an offscreen GWorld to do the deed in ...
		SetRect(&r, 0 ,0, 32, 32);
		err = NewGWorld(&gWorld, depth, &r, nil, nil, useTempMem);	//	Try temp memory first
		if (err) err = NewGWorld(&gWorld, depth, &r, nil, nil, nil);	//	Failed, use application heap
		if (err == noErr)
		{		
			//	(jp)  12-5-99  Many of the graphics importer routines actually can (and do) return
			//	errors.  For example, if for what ever reason, we have a blank ad (as I currently have)
			//	and which could conceivably happen if we didn't have enough memory to display an ad in
			//	the ad window, GraphicsImportGetBoundsRect and GraphicsImportDraw return errors which
			//	results in a "TV snow" icon.  I added more error checking... which means that legitimate
			//	errors really might be returned.
			
			// Tell the importer where to put it ...
			err= GraphicsImportSetGWorld(importer, gWorld, nil);
			if (!err) {			
				// Tell the importer how to scale the picture when drawn ...
				err = GraphicsImportGetBoundsRect(importer, &srcRect);
			}
			if (!err) {
				RectMatrix(&whatIsTheMatrix, &srcRect, &r); 
				err = GraphicsImportSetMatrix(importer, &whatIsTheMatrix);
			}
			if (!err)
				err = GraphicsImportDraw(importer);
			
			// Now make an icon out of this GWorld ...
			if (!err)
				err = MakeLHIconFile(gWorld, &r, iconSpec, importer, adSpec);	
			else {
			// We'll still need to put something reasonable in the icon...
			// How about the gray logo pict?  Not pretty, but somthing for now...
				PicHandle pic;
				if (pic = GetResource_('PICT',GRAY_LOGO_PICT)) {
					PushGWorld();
					SetGWorld(gWorld,nil);
					HNoPurge_(pic);
					DrawPicture(pic,&r);
					HPurge((Handle)pic);
					err = MakeLHIconFile(gWorld, &r, iconSpec, importer, adSpec);
					PopGWorld();	
				}
			}			
		}
		
		// Cleanup
		CloseComponent(importer);
	}
	
	// Cleanup
	if (gWorld) DisposeGWorld(gWorld);
	
	return (err);
}

/**********************************************************************
 * MakeLHIconFile - create file containing a 32*32 bit icon in 1 and
 *	8 bits.
 **********************************************************************/
OSErr MakeLHIconFile(GWorldPtr gWorld, Rect *pRect, FSSpecPtr iconSpec, GraphicsImportComponent importer, FSSpecPtr sourceSpec)
{
	OSErr err = noErr;
	FInfo info;
	short iconRes;
	short oldResFile = CurResFile();
	RGBColor	color,*transparentColor=nil;
	
	//
	// First, see if this icon file already exists
	//
	
	if (noErr==FSpExists(iconSpec)) return noErr;
	
	
	//
	// Create a new res file to store the icons in.
	//
	
	FSpCreateResFile(iconSpec,CREATOR,LINK_HISTORY_PREVIEW_TYPE,smSystemScript);
	if ((err=ResError())!=noErr)
	{
	 	return (err);
	}
	
	iconRes = FSpOpenResFile(iconSpec,fsRdWrPerm);
	if (((err=ResError())!=noErr) || (iconRes ==-1))
	{
		FSpDelete(iconSpec);
		return (err);
	}
	
	//
	// Now create and save the icons
	//
	if (GetPNGTransColor(importer,sourceSpec,&color))
		transparentColor = &color;	//	Use PNG transparent color when setting up mask

	err = MakeIconSuite (gWorld, pRect, transparentColor, "\p");

	CloseResFile(iconRes);
	UseResFile(oldResFile);
		
		
	//
	// set the icon of the new file to the custom icon
	//
	
	if (err == noErr)
	{
		FSpGetFInfo(iconSpec,&info);
		info.fdFlags |= kHasCustomIcon;
		FSpSetFInfo(iconSpec,&info);
	}
	
	// If there was an error, forget about it
	if (err)
	{
		FSpDelete(iconSpec);
	}
	
	return (err);
}


OSErr MakeIconSuite (GWorldPtr gWorld, Rect *pRect, RGBColor *transparentColor, PStr name)

{
	Handle	icon = nil;
	OSErr		err = noErr;
	
	icon = MakeIcon(gWorld,pRect,1,16);			// make 16x16 1-bit color icon
	if(icon)
	{
		AddResource( icon, 'SICN', kCustomIconResource, name);
		err = err || ResError();
	}

	icon =	MakeICN_pound(gWorld, pRect, 32, transparentColor);		// create 32x32 1-bit color icon AND MASK
	if(icon)
	{
		AddResource( icon, 'ICN#', kCustomIconResource, name);
		err = err || ResError();
	}

	icon =	MakeICN_pound(gWorld, pRect, 16, transparentColor);		// create 16x16 1-bit color icon AND MASK
	if(icon)
	{
		AddResource( icon, 'ics#', kCustomIconResource, name);
		err = err || ResError();
	}

	icon = MakeIcon(gWorld,pRect,8,32);			// make 32x32 8-bit color icon
	if(icon)
	{
		AddResource( icon, 'icl8', kCustomIconResource, name);
		err = err || ResError();
	}

	icon = MakeIcon(gWorld,pRect,4,32);			// make 32x32 4-bit color icon
	if(icon)
	{
		AddResource( icon, 'icl4', kCustomIconResource, name);
		err = err || ResError();
	}

	icon = MakeIcon(gWorld,pRect,8,16);				// make 16x16 8-bit color mask
	if(icon)
	{
		AddResource( icon, 'ics8', kCustomIconResource, name);
		err = err || ResError();
	}

	icon = MakeIcon(gWorld,pRect,4,16);				// make 16x16 4-bit color mask
	if(icon)
	{
		AddResource( icon, 'ics4', kCustomIconResource, name);
		err = err || ResError();
	}
	
	return (err);
}

/**********************************************************************
 * MakeICN_pound - create a one bit icon and mask from gwp
 **********************************************************************/
Handle MakeICN_pound(GWorldPtr gwp, Rect *srcRect, short iconDimension, RGBColor *transparentColor)
{
	GWorldPtr	scaledGWorld;
	Rect	scaledIcon;
	Handle icon = nil;
	Handle iconMask = nil;
	Size iconSize;
	OSErr	theError;
	
	scaledGWorld = nil;
	// If we're making a scaled icon with transparency (one that is not of the same dimensions as the source rect),
	// we'll have to create a second GWorld of the proper size in order to correctly create the icon mask.
	if (transparentColor && RectWi (*srcRect) != iconDimension) {
		SetRect (&scaledIcon, 0, 0, iconDimension, iconDimension);
		theError = NewGWorld (&scaledGWorld, 8, &scaledIcon, nil, nil, useTempMem);	//	Try temp memory first
		if (theError)
			theError = NewGWorld (&scaledGWorld, 8, &scaledIcon, nil, nil, nil);			//	Failed, use application heap
		if (!theError) {
			PushGWorld ();
			SetGWorld (scaledGWorld, nil);
			EraseRect (&scaledIcon);
			CopyBits(GetPortBitMapForCopyBits(gwp),
				GetPortBitMapForCopyBits(scaledGWorld),
				srcRect,
				&scaledIcon,
				srcCopy | ditherCopy, nil);
			PopGWorld ();	
		}
	}
	
	icon = MakeIcon(gwp, srcRect, 1, iconDimension);			
	if(icon)
	{
		if (transparentColor)
		{
			//	Set every bit in mask except for transparent color
			if (transparentColor && RectWi (*srcRect) != iconDimension)
				iconMask = MakeIconLo(scaledGWorld, &scaledIcon, 1, iconDimension, transparentColor);		
			else
				iconMask = MakeIconLo(gwp, srcRect, 1, iconDimension, transparentColor);		
		}
		else
			iconMask = MakeIconMask(gwp, srcRect, iconDimension);
		if(iconMask)
		{
			iconSize = GetHandleSize(icon);
			SetHandleSize(icon, iconSize + GetHandleSize(iconMask));
			if (MemError()==noErr) BlockMove(*iconMask, (Ptr)(((long)*icon) + iconSize), GetHandleSize(iconMask));			
			DisposeHandle(iconMask);
		}
	}

	if (scaledGWorld)
		DisposeGWorld (scaledGWorld);

	return (icon);
}

/**********************************************************************
 * MakeIconMask - create an icon mask for an ad preview.  This is
 *	just a black box as big as the srcRect.
 **********************************************************************/
Handle MakeIconMask(GWorldPtr srcGWorld, Rect *srcRect, short iconSize)
{
	Handle	iconBits;
	OSErr err = noErr;
	GWorldPtr gWorld = nil;
	PixMapHandle hPM;
	RGBColor saveBackColor;
	RGBColor blackColor = { 0,0,0 };
									
	// Create an offscreen graphics world ...
	err = NewGWorld(&gWorld, 1, srcRect, nil, nil, useTempMem);	//	Try temp memory first
	if (err) err = NewGWorld(&gWorld, 1, srcRect, nil, nil, nil);	//	Failed, use application heap
	if (err == noErr)
	{		
		// draw a 32x32 black box
		PushGWorld();
		SetGWorld(gWorld,nil);
				
		hPM = GetGWorldPixMap(gWorld);
		LockPixels(hPM);	
		
		GetBackColor(&saveBackColor);
		RGBBackColor(&blackColor);	
		EraseRect(srcRect);
		RGBBackColor(&saveBackColor);
		
		UnlockPixels(hPM);
		PopGWorld();

		// Make an icon out of it ...
		iconBits = MakeIcon(gWorld, srcRect, 1, iconSize);
		
	}

	// Cleanup
	if (gWorld) DisposeGWorld(gWorld);
	
	return (iconBits);
}

/*
 *	These routines are taken from the MakeIcon sample code
 */
Handle MakeIcon(GWorldPtr srcGWorld, Rect *srcRect, short dstDepth, short iconSize)
{
	return MakeIconLo(srcGWorld, srcRect, dstDepth, iconSize, nil);
}

Handle MakeIconLo(GWorldPtr srcGWorld, Rect *srcRect, short dstDepth, short iconSize, RGBColor *transColor)
/*
	Creates a handle to the image data for an icon, or nil if an error.
	The source image is specified by GWorld and regtangle defining the area
	to create the icon from.
	The type of icon is specified by the depth and Size paramters.
	iconSize is used for both height and width.
	For example, to create an Icl8 pass 8 for dstDepth and 32 for iconSize.
	to create an ics8 pass 8 for the dstDepth and 16 for iconSize.
	to create an ICON pass 1 for the dstDepth and 32 for iconSize.
	
	
*/
{
	long			bytesPerRow;
	long			imageSize;
	Handle			dstHandle;
	PixMapHandle	pix;
	Rect			iconFrame;
	QDErr			err;
	
	PushGWorld();	// save Graphics env state

	SetGWorld(srcGWorld,nil);
	
	iconFrame.top = 0;
	iconFrame.left = 0;
	iconFrame.bottom = iconSize;
	iconFrame.right = iconSize;
	
	// make a gworld for the icl resource
	pix = (PixMapHandle)NewHandleClear(sizeof(PixMap));
	
	/* See Tech Note #120 - for info on creating a PixMap by hand as SetUpPixMap
		does.  SetUpPixMap was taken from that Tech Note....
	*/
	err =  SetUpPixMap(dstDepth,&iconFrame,GetCTable(dstDepth),pix);

	if(err)
	{
		PopGWorld();
		return nil;
	}
		
	LockPixels(GetGWorldPixMap(srcGWorld));
	LockPixels(pix);
			
	if (transColor)
	{
		long	transIdx = Color2Index(transColor);
		short	row,col;
		RGBColor	color;
		unsigned char	mask;
		Ptr		pBitMap;
		short	offset;
		
		//	Set bit in mask for every pixel not same color as transparency color
		pBitMap = (*pix)->baseAddr;
		offset = 0;
		mask = 0x80;
		for(row=0;row<iconSize;row++)
		{
			for(col=0;col<iconSize;col++)
			{
				GetCPixel(srcRect->left+col,srcRect->top+row,&color);
				if (Color2Index(&color)!=transIdx)
					//	Not transparent. Set this one
					pBitMap[offset] |= mask;
				mask >>= 1;
				if (!mask)
				{
					offset++;
					mask = 0x80;
				}
			}
			pBitMap += (*pix)->rowBytes & 0x3fff;
			offset = 0;
			mask = 0x80;
		}
	}
	else
		CopyBits(GetPortBitMapForCopyBits(srcGWorld),
				(BitMapPtr)*pix,
				srcRect,
				&iconFrame,
				srcCopy | ditherCopy, nil);
 
 
	UnlockPixels(GetGWorldPixMap(srcGWorld));
 
 	bytesPerRow = ((dstDepth * ((**pix).bounds.right - (**pix).bounds.left) + 15) / 16) * 2;
	imageSize  = (bytesPerRow) * ((**pix).bounds.bottom - (**pix).bounds.top);

	dstHandle = NewHandle(imageSize);
	err = MemError ();
	if(err || dstHandle == nil)
	{
		PopGWorld();
		return nil;	
	}
	HLock((Handle)dstHandle);
	BlockMove(GetPixBaseAddr(pix),*dstHandle,imageSize);
	HUnlock(dstHandle);
	UnlockPixels(pix);
	TearDownPixMap(pix);
	
	PopGWorld(); // Restore graphics env to previous state
	
	HNoPurge(dstHandle);
	return dstHandle;
}

void FreeBitMap(BitMap *Bits)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Dumps a BitMap created by NewBitMap below.
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
{
	DisposePtr(Bits->baseAddr);
	Bits->baseAddr=nil;
	SetRect(&Bits->bounds,0,0,0,0);
	Bits->rowBytes=0;
}

void CalcOffScreen(register Rect *frame,register long *needed, register short *rows)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Calculates how much memory and rowbytes a bitmap of the size frame would require.
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
{
	*rows=((((frame->right) - (frame->left)) + 15)/16) *2;
	*needed=(((long)(*rows)) * (long)((frame->bottom) - (frame->top)));
}
 
void NewBitMap(Rect *frame,BitMap *theMap)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Creates a new empty bitmap.
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
{
	Size	size;
	short	rbytes;
	
	CalcOffScreen(frame,&size,&rbytes);
	
	theMap->rowBytes=rbytes;
	theMap->bounds=*frame;
	theMap->baseAddr=NewPtrClear(size);
}

void NewMaskedBitMap(BitMap	*srcBits, BitMap *maskBits, Rect *srcRect)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Pass a pointer to an existing bitmap, and this creates a bitmap that is an
	equivelent mask.
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
{
	short	rowbytes,height,words;
	long	needed;

	NewBitMap(srcRect,maskBits);
	
	if(MemError())		
	{
		maskBits->baseAddr=nil;
	 	SetRect(&maskBits->bounds,0,0,0,0);
	 	maskBits->rowBytes=0;
	 }
	 else		/*	memory was allocated for Mask BitMap ok */
	 {
	 	CalcOffScreen(srcRect,&needed,&rowbytes);
	 	words=rowbytes/2;
	 	height=srcRect->bottom - srcRect->top;
	 	CalcMask(srcBits->baseAddr,maskBits->baseAddr,rowbytes,rowbytes,height,words);
	 }
}

#define kDefaultRes 0x00480000 /* Default resolution is 72 DPI; Fixed type */

OSErr SetUpPixMap(
    short        depth,       /* Desired number of bits/pixel in off-screen */
    Rect         *bounds,     /* Bounding rectangle of off-screen */
    CTabHandle   colors,      /* Color table to assign to off-screen */
    PixMapHandle aPixMap      /* Handle to the PixMap being initialized */
    )
{
    CTabHandle newColors;   	/* Color table used for the off-screen PixMap */
    Ptr        offBaseAddr; 	/* Pointer to the off-screen pixel image */
    OSErr      error;       	/* Returns error code */
    short      bytesPerRow;		/* Number of bytes per row in the PixMap */


    error = noErr;
    newColors = nil;
    offBaseAddr = nil;

  	bytesPerRow = ((depth * (bounds->right - bounds->left) + 15) / 16) * 2;

   /* Clone the clut if indexed color; allocate a dummy clut if direct color*/
    if (depth <= 8)
        {
        newColors = colors;
//        error = HandToHand((Handle *) &newColors);	// This doesn't appear to be necessary and creates a memory leak alb 2/00
        }
    else
        {
        newColors = (CTabHandle) NewHandle(sizeof(ColorTable) -
                sizeof(CSpecArray));
        error = MemError();
        }
    if (error == noErr)
        {
        /* Allocate pixel image; long integer multiplication avoids overflow */
        offBaseAddr = NewPtrClear((unsigned long) bytesPerRow * (bounds->bottom -
                bounds->top));
        if (offBaseAddr != nil)
            {
            /* Initialize fields common to indexed and direct PixMaps */
            (**aPixMap).baseAddr = offBaseAddr;  /* Point to image */
            (**aPixMap).rowBytes = bytesPerRow | /* MSB set for PixMap */
                    0x8000;
            (**aPixMap).bounds = *bounds;        /* Use given bounds */
//          (**aPixMap).pmVersion = 0;           /* No special stuff */
//          (**aPixMap).packType = 0;            /* Default PICT pack */
//          (**aPixMap).packSize = 0;            /* Always zero in mem */
            (**aPixMap).hRes = kDefaultRes;      /* 72 DPI default res */
            (**aPixMap).vRes = kDefaultRes;      /* 72 DPI default res */
            (**aPixMap).pixelSize = depth;       /* Set # bits/pixel */
//          (**aPixMap).planeBytes = 0;          /* Not used */
//          (**aPixMap).pmReserved = 0;          /* Not used */

            /* Initialize fields specific to indexed and direct PixMaps */
            if (depth <= 8)
                {
                /* PixMap is indexed */
//              (**aPixMap).pixelType = 0;       /* Indicates indexed */
                (**aPixMap).cmpCount = 1;        /* Have 1 component */
                (**aPixMap).cmpSize = depth;     /* Component size=depth */
                (**aPixMap).pmTable = newColors; /* Handle to CLUT */
                }
            else
                {
                /* PixMap is direct */
                (**aPixMap).pixelType = RGBDirect; /* Indicates direct */
                (**aPixMap).cmpCount = 3;          /* Have 3 components */
                if (depth == 16)
                    (**aPixMap).cmpSize = 5;       /* 5 bits/component */
                else
                    (**aPixMap).cmpSize = 8;       /* 8 bits/component */
                (**newColors).ctSeed = 3 * (**aPixMap).cmpSize;
//              (**newColors).ctFlags = 0;
//              (**newColors).ctSize = 0;
                (**aPixMap).pmTable = newColors;
                }
            }
        else
            error = MemError();
        }
    else
        newColors = nil;

    /* If no errors occured, return a handle to the new off-screen PixMap */
    if (error != noErr)
        {
        if (newColors != nil)
            DisposeCTable(newColors);
        }

    /* Return the error code */
    return error;
    }

void TearDownPixMap(PixMapHandle pix)
{
	DisposeCTable((*pix)->pmTable);
	DisposePtr((*pix)->baseAddr);
	DisposeHandle((Handle)pix);	
}
