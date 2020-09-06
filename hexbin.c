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

#include "hexbin.h"
#define FILE_NUM 17
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions to convert files to binhex and back again
 * much of this code is patterned after xbin by Dave Johnson, Brown University
 * Some is lifted straight from xbin.  Our thanks to Dave & Brown.
 ************************************************************************/

#pragma segment HexBin

/************************************************************************
 * Declarations for private routines
 ************************************************************************/
	void HexBinInputChar(Byte c,long estMessageSize);
	void HexBinDataChar(short d,long estMessageSize);
	int FoundHexBin(void);
	int HexBinDecode(Byte c,long estMessageSize);
	void AbortHexBin(Boolean error);
	void OpenDataFork(void);
	int ForkRoll(void);
	int FlushBuffer(void);
	void ResetHexBin(void);
	void comp_q_crc(unsigned short c);
	void CrcError(void);
	void SaveHexBin(UPtr text,long size,long estMessageSize);
	void ForceAttachFolder(PStr volName, long *dirId);

/************************************************************************
 * Private globals
 ************************************************************************/
typedef struct
{
	long type;
	long author;
	short flags;
	long dataLength;
	long rzLength;
	unsigned short hCrc;
} HexBinHead;
typedef struct HexBinGlobals_ HexBinGlobals, *HBGPtr, **HBGHandle;
struct HexBinGlobals_
{
	short state;
	long oSpot;
	UHandle buffer;
	long bSize;
	long bSpot;
	short refN;
	FSSpec spec;
	Byte lastData;
	Byte state68;
	Byte b8;
	Byte runCount;
	Boolean run;
	HeaderDHandle hdh;	// enclosing header
	long count;
	long size;
	Str63 binHexIntro;
	long origOffset;
	Boolean gotOne;
	short mailboxRefN;
	union
	{
		HexBinHead bxHead;
		Byte bxhBytes[sizeof(HexBinHead)];
	} BHHUnion;
	unsigned long calcCrc;
	unsigned long crc;
};
#define Hdh (*HBG)->hdh
#define State (*HBG)->state
#define OSpot (*HBG)->oSpot
#define Buffer (*HBG)->buffer
#define BSize (*HBG)->bSize
#define BSpot (*HBG)->bSpot
#define RefN (*HBG)->refN
#define Spec (*HBG)->spec
#define Name (*HBG)->spec.name
#define Type (*HBG)->BHHUnion.bxHead.type
#define Author (*HBG)->BHHUnion.bxHead.author
#define Flags (*HBG)->BHHUnion.bxHead.flags
#define RzLength (*HBG)->BHHUnion.bxHead.rzLength
#define DataLength (*HBG)->BHHUnion.bxHead.dataLength
#define HCrc (*HBG)->BHHUnion.bxHead.hCrc
#define BxhBytes (*HBG)->BHHUnion.bxhBytes
#define LastData (*HBG)->lastData
#define State68 (*HBG)->state68
#define B8 (*HBG)->b8
#define RunCount (*HBG)->runCount
#define Run (*HBG)->run
#define Count (*HBG)->count
#define Size (*HBG)->size
#define CalcCrc (*HBG)->calcCrc
#define Crc (*HBG)->crc
#define BinHexIntro (*HBG)->binHexIntro
#define OrigOffset (*HBG)->origOffset
#define GotOne (*HBG)->gotOne
#define MailboxRefN (*HBG)->mailboxRefN

#define RUNCHAR 0x90

#define DONE 0x7F
#define SKIP 0x7E
#define FAIL 0x7D

Byte HexBinTable[256] = {
/* 0*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*							\t   \015								\012								*/
				FAIL, SKIP, SKIP, FAIL, FAIL, SKIP, FAIL, FAIL,
/*      ' '																							*/
/* 1*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/* 2*/	SKIP, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
				0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, FAIL, FAIL,
/* 3*/	0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, FAIL,
/*									: 																	*/
				0x14, 0x15, DONE, FAIL, FAIL, FAIL, FAIL, FAIL,
/* 4*/	0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
				0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, FAIL,
/* 5*/	0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, FAIL,
				0x2C, 0x2D, 0x2E, 0x2F, FAIL, FAIL, FAIL, FAIL,
/* 6*/	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, FAIL,
				0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, FAIL, FAIL,
/* 7*/	0x3D, 0x3E, 0x3F, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
      	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
				FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
};
	
#pragma segment POP


/************************************************************************
 * ConvertHexBin - the BinHex to mac converter
 *	returns True if a BinHex file is being converted
 *	may write own data into buf (conversion note)
 ************************************************************************/
Boolean ConvertHexBin(short refN,UPtr buf,long *size,POPLineType lineType,long estSize)
{
	long offset;

	if (!HBG) return(False);	/* can't work without globals */
	
	switch(State)
	{
		/*
		 * BinHex detection
		 */
		case HexDone:
			if (lineType==plComplete && *size >= *BinHexIntro && !strncmp(LDRef(HBG)->binHexIntro+1,buf,*BinHexIntro))
			{
				State = NotHex;
				GetFPos(refN,&offset);  /* save binhex start */
				OrigOffset = offset;
				GotOne = False;
				MailboxRefN = refN;
			}
			UL(HBG);
			break;
		
		/*
		 * making up our minds
		 */
		case NotHex:
		case CollectName:
		case CollectInfo:
			SaveHexBin(buf,*size,estSize);
			if (State>CollectInfo && State!=HexDone)
			{
				SetFPos(refN,fsFromStart,OrigOffset);	/* toss the saved stuff */
				SetEOF(refN,OrigOffset);
				*size = 0;
			}
			break;
		
		/*
		 * cruising right along
		 */
		default:
			SaveHexBin(buf,*size,estSize);
			*size = 0;
			break;
	}
	
	/*
	 * We're hexing unless we're not
	 */
	return(State!=HexDone);
}

/************************************************************************
 * SaveHexBin - save a binhex file, if one is found.	Returns the 
 * state of the converter
 ************************************************************************/
void SaveHexBin(UPtr text,long size,long estMessageSize)
{	
	if (State==HexDone)
		State = NotHex; 		/* start the conversion */
		
	for (Count=0;Count<size;Count++)
		HexBinInputChar(text[Count],estMessageSize);
}

/************************************************************************
 * EndHexBin - done with a HexBin session
 ************************************************************************/
void EndHexBin(void)
{
	if (HBG)
	{
		if (Spec.vRefNum && !CommandPeriod) {WarnUser(BINHEX_SHORT,0);BadBinHex=True;}
		AbortHexBin(False);
		ZapHandle(HBG);
		HBG = nil;
	}
	return;
}

/************************************************************************
 * BeginHexBin - ready to begin a HexBin session
 ************************************************************************/
void BeginHexBin(HeaderDHandle hdh)
{
	Str63 intro;
	HBG = NewH(HexBinGlobals);
	if (HBG)
	{
		WriteZero(*HBG,sizeof(HexBinGlobals));
		State = HexDone;
		GetRString(intro,BINHEX);
		PCopy(BinHexIntro,intro);
		Hdh = hdh;
	}
}
#pragma segment HexBin

/************************************************************************
 * HexBinInputChar - read a char from the binhex data, decode it, and
 * let HexBinDataChar do (most of) the rest.
 ************************************************************************/
void HexBinInputChar(Byte c,long estMessageSize)
{
	short d;

reSwitch:
	switch (State)
	{
		case HexDone:
			break;
			
		case NotHex:
			if (c==':') State=FoundHexBin();
			break;

		case Excess:
			c = HexBinTable[c];
			if (c==DONE)
			{
				State = HexDone;
				if (BSpot>4)
				{
					WarnUser(BINHEXEXCESS,BSpot-1);
					BadBinHex = True;
				}
				PopProgress(True);
			}
			else if (c!=SKIP)
				BSpot++;
			break;
	
		case CollectName:
			(*Buffer)[BSpot++] = c;
			/* fall-throught to default */
		default:
			if ((d=HexBinDecode(c,estMessageSize))>=0)
				HexBinDataChar(d,estMessageSize);
			break;
	}
}

/************************************************************************
 * HexBinDataChar - the main engine for the de-binhexer
 * Unfortunately, much of the real work happens as side-effects to functions.
 * State and BSpot are almost always manipulated directly in this routine, but
 * the rest of HBG is up for grabs.
 ************************************************************************/
void HexBinDataChar(short d,long estMessageSize)
{
reSwitch:
	switch (State)
	{
		case CollectName:
			comp_q_crc(d);
			Name[OSpot] = d;
			if (OSpot > sizeof(Name)-2 || OSpot>Name[0])
			{
				State = CollectInfo;
				BSpot = 0;
				Name[0] = MIN(Name[0],31);
				while (Name[0] && Name[Name[0]]==0) Name[0]--;
				Name[Name[0]+1] = 0;
			}
			else
			{
				if (++OSpot>sizeof(Name)-2)
				{
					WarnUser(BAD_HEXBIN_FORMAT,State);
					AbortHexBin(True);
				}
			}
			break;
			
		case CollectInfo:
			BxhBytes[BSpot++] = d;
			switch (sizeof(HexBinHead)-BSpot)
			{
				case 0:
					{
						FSSpec spec = Spec;
						/*
						 * Note:	The length test can't be 100% correct because of
						 * run-length encoding.  I therefore give it some slop and
						 * hope for the best.  Eudora doesn't produce RLE, and neither
						 * does StuffIt, so perhaps this won't be a problem.  It's
						 * better than the alternative, anyway...
						 */
						if ((estMessageSize<GetRLong(HEX_SIZE_THRESH) ||
								DataLength+RzLength < (estMessageSize*100)/GetRLong(HEX_SIZE_PERCENT)) &&
								(AutoWantTheFile(&spec,False,Hdh && (*Hdh)->relatedPart)/*|| WantTheFile(&spec)*/))
						{
							Spec = spec;
							Crc = HCrc;
							BSpot = 0;
							CrcError();
							State = DataWrite;
							OpenDataFork();
						}
						else
							AbortHexBin(False);
					}
					break;
				case 1:
					break;
				default:
					comp_q_crc(d);
					break;
			}
			break;
			
		case DataWrite:
		case RzWrite:
			if (OSpot==0)
			{
				State++;
				goto reSwitch;
			}
			else
			{
				(*Buffer)[BSpot++] = d;
				comp_q_crc(d);
				OSpot--;
				if (BSpot==BSize)
				{
					if (FlushBuffer()) AbortHexBin(True);
					BSpot = 0;
				}
			}
			break;
		case DataCrc1:
		case RzCrc1:
			Crc = d << 8;
			State++;
			break;
		case DataCrc2:
		case RzCrc2:
			State++;
			Crc = Crc | d;
			State = ForkRoll();
			BSpot = 0;
			break;
	}
}

/************************************************************************
 * FoundHexBin - the second level of initialization.
 * This function is called when we have detected a HexBin file.  Its
 * major purpose is to allocate a buffer for data
 * If it has allocated a buffer, it continues the HexBin process by
 * returning the next state, CollectHeader; otherwise, it returns HexDone,
 * effectively aborting the HexBin process.
 ************************************************************************/
int FoundHexBin(void)
{
	BSize = GetRLong(BUFFER_SIZE);
	if (!Buffer) Buffer = NuHTempBetter(BSize);
	if (!Buffer)
	{
		WarnUser(BINHEX_MEM,MemError());
		BadBinHex = True;
		return(HexDone);
	}
	else
	{
		ResetHexBin();
		return(CollectName);
	}
}

/************************************************************************
 * HexBinDecode - decode a data byte.
 * The binhex data format encodes 3 data bytes into four encoded bytes.
 * There are some "magic" values for encoded bytes:
 *			newline, cr 		ignore
 *			: 							end of binhex data
 * There is also a magic data byte, 0x90.  This repeats the PREVIOUS
 * data byte n times, where n is the value of the NEXT data byte.
 * If n is zero, 0x90 itself is output.
 ************************************************************************/
int HexBinDecode(Byte c,long estMessageSize)
{
	Byte b6;
	short data;
		
	if ((b6=HexBinTable[c])>64)
	{
		switch (b6)
		{
			case SKIP:
				return(-1);
			case DONE:
				WarnUser(BINHEX_SHORT,0);
				AbortHexBin(True);
				return(-1);
			default:
				if (!PrefIsSet(PREF_HEX_PERMISSIVE))
				{
					WarnUser(BINHEX_BADCHAR,c);
					AbortHexBin(True);
				}
				return(-1);
		}
	}
	else
	{
		switch(State68++)
		{
			case 0:
				B8 = b6<<2;
				return(-1);
			case 1:
				data = B8 | (b6 >> 4);
				B8 = (b6 & 0xf) << 4;
				break;
			case 2:
				data = B8 | (b6>>2);
				B8 = (b6 & 0x3) << 6;
				break;
			case 3:
				data = B8 | b6;
				State68 = 0;
				break;
		}
		if (!Run)
		{
			if (data == RUNCHAR)
			{
				Run = 1;
				RunCount = 0;
				return(-1);
			}
			else
				return(LastData = data);
		}
		else
		{
			Run = False;
			if (!data)
				return(LastData = RUNCHAR);
			while (--data > 0) HexBinDataChar(LastData,estMessageSize);
			return(-1);
		}
	}
}


/************************************************************************
 * AbortHexBin - something bad has happened.	Get rid of loose ends.
 ************************************************************************/
void AbortHexBin(Boolean error)
{
	if (Buffer) {ZapHandle(Buffer); Buffer=0;}
	if (RefN) {MyFSClose(RefN); RefN=0;}
	if (Spec.vRefNum)
	{
		LDRef(HBG);
		FSpDelete(&Spec);
		ASSERT(0);
		UL(HBG);
		Spec.vRefNum = 0;
	}
	State = HexDone;
	BadBinHex = BadBinHex || error;
	PopProgress(True);
}

/************************************************************************
 * OpenDataFork - open the data fork of the file
 ************************************************************************/
void OpenDataFork(void)
{
	int err;
	short refN;
	FInfo info;
	
	LDRef(HBG);
	err=FSpCreate(&Spec,Author,Type,smSystemScript);
	if (err == dupFNErr) err = noErr;
	if (err)
	{
		FileSystemError(BINHEX_CREATE,Name,err);
		Spec.vRefNum = 0;
		AbortHexBin(True);
	}
	else if (err=FSpGetFInfo(&Spec,&info))
	{
		FileSystemError(BINHEX_CREATE,Name,err);
		AbortHexBin(True);
	}
	else
	{
		info.fdFlags = Flags;
		SafeInfo(&info,nil);
		if (err=FSpSetFInfo(&Spec,&info))
		{
			FileSystemError(BINHEX_OPEN,Name,err);
			AbortHexBin(True);
		}
		else if (err=FSpOpenDF(&Spec,fsRdWrPerm,&refN))
		{
			FileSystemError(BINHEX_OPEN,Name,err);
			AbortHexBin(True);
		}
		else
			RefN = refN;
	}
	OSpot = DataLength;
	UL(HBG);
}

/**********************************************************************
 * SafeInfo - make changes to finderinfo to make things safer
 **********************************************************************/
void SafeInfo(FInfo *info, FXInfo *fxInfo)
{
	short index;
	
	if (info)
	{
		info->fdFlags &= ~fOnDesk;
		info->fdFlags &= ~fInvisible;
		info->fdFlags &= ~fInited;
		if (TypeIsOnListWhereAndIndex(info->fdType,EXECUTABLE_TYPE_LIST,nil,&index))
		{
			info->fdType = index ? (kFakeAppType&0xffffff00)|('0'+index) : kFakeAppType;
			info->fdCreator = CREATOR;
			info->fdFlags &= ~fHasBundle;
		}
	}
	if (fxInfo)
	{
		fxInfo->fdComment = 0;
		fxInfo->fdPutAway = 0;
	}
}


/************************************************************************
 * ForkRoll - roll from the data fork to the resource fork, or else from
 * the resource fork to cleanup.	Returns the next state.
 ************************************************************************/
int ForkRoll(void)
{
	int err=0;
	short refN;
	long pos;
	
	/*
	 * flush and close the current file
	 */
	if (RefN)
	{
		CrcError();
		if (err=FlushBuffer())
			AbortHexBin(True);
		else
		{
			if (!GetFPos(RefN,&pos)) SetEOF(RefN,pos);
			if (err=MyFSClose(RefN))
			{
				AbortHexBin(True);
				LDRef(HBG);
				FileSystemError(BINHEX_WRITE,Name,err);
				UL(HBG);
			}
			else RefN = 0;	// successfully closed
		}
	}
	if (err) return(HexDone);
	
	/*
	 * open the new one if necessary
	 */
	if (State==RzWrite)
	{
		LDRef(HBG);
		if (err=FSpOpenRF(&Spec,fsRdWrPerm,&refN))
		{
			FileSystemError(BINHEX_OPEN,Name,err);
			AbortHexBin(True);
		}
		else
			RefN = refN;
		UL(HBG);
		OSpot = RzLength;
		return(err ? HexDone : RzWrite);
	}
	else
	{
		Str31 fileName;
		FSSpec spec = Spec;
		GotOne = True;
		PCopy(fileName,Name);
		if (err=RecordAttachment(&spec,HBG ? (*HBG)->hdh : nil))
		{
			AbortHexBin(True);
			return(HexDone);
		}
		// If there is a long filename, the spec may have changed in RecordAttachment
		// Make sure to copy the new spec back
		Spec = spec;
		ZapHandle(Buffer);
		Spec.vRefNum = 0;
		return(Excess);
	}
}

/************************************************************************
 * FlushBuffer - write out what we have so far
 ************************************************************************/
int FlushBuffer(void)
{
	long writeBytes = BSpot;
	int err;
	
	if (err=NCWrite(RefN,&writeBytes,LDRef(Buffer)))
		{LDRef(HBG); FileSystemError(BINHEX_WRITE,Name,err); UL(HBG);}
	UL(Buffer);
	BSpot = 0;
	return(err);
}

#ifdef	IMAP
/************************************************************************
 * AutoWantTheFile - see if we can auto-receive the file
 * ohYesYouDo is true if we have no choice (ie, the stanfile timed out)
 ************************************************************************/
Boolean AutoWantTheFile(FSSpecPtr specPtr,Boolean ohYesYouDo,Boolean relatedPart)
{
	return (AutoWantTheFileLo(specPtr, ohYesYouDo, relatedPart, false));
}

/************************************************************************
 * AutoWantTheFile - see if we can auto-receive the file
 * ohYesYouDo is true if we have no choice (ie, the stanfile timed out)
 ************************************************************************/
Boolean AutoWantTheFileLo(FSSpecPtr specPtr,Boolean ohYesYouDo,Boolean relatedPart, Boolean imapStub)
#else
Boolean AutoWantTheFile(FSSpecPtr specPtr,Boolean ohYesYouDo,Boolean relatedPart)
#endif
{
	Str127 message;
	FSSpec attFSpec;
	
	/*
	 * grab the volume name and directory id
	 */
	if (relatedPart)
		SubFolderSpec(PARTS_FOLDER,&attFSpec);
#ifdef	IMAP
	else if (imapStub)
		GetIMAPAttachFolder(&attFSpec);
#endif		
	else {
		GetCurrentAttFolderSpec(&attFSpec);
		if ( attFSpec.vRefNum == 0 || attFSpec.parID == 0 )
			CurrentAttFolderSpec = attFSpec = AttFolderSpec;
	}
	ASSERT ( attFSpec.vRefNum != 0 && attFSpec.parID != 0 );

	/*
	 * Darn AppleLink and NULL in filenames
	 */
	*specPtr->name = RemoveChar(NULL,specPtr->name+1,*specPtr->name);
	
	/*
	 * make sure we don't overwrite anything.
	 * add a number to the end of the file until we don't find the filename
	 */
	specPtr->vRefNum = attFSpec.vRefNum;
	specPtr->parID = attFSpec.parID;
	
	if (UniqueSpec(specPtr,31)) return(False);
	
	//PushProgress();
	ProgressMessage(kpMessage,ComposeRString(message,BINHEX_RECV_FMT,specPtr->name));
	return(True);
}

/**********************************************************************
 * ForceAttachFolder - find an attachment folder, come hell or high water
 **********************************************************************/
void ForceAttachFolder(PStr volName, long *dirId)
{
	Str31 folder;
	CInfoPBRec hfi;
	
	/*
	 * on the same volume as Eudora Folder
	 */
	GetMyVolName(Root.vRef,volName);
	
	/*
	 * create the folder
	 */
	*dirId = Root.dirId;
	GetRString(folder,ATTACH_FOLDER);
	(void) DirCreate(Root.vRef,Root.dirId,folder,dirId);
	
	/*
	 * is it a folder?
	 */
	if (!HGetCatInfo(Root.vRef,Root.dirId,folder,&hfi) &&
			hfi.hFileInfo.ioFlAttrib&0x10)
		*dirId = hfi.hFileInfo.ioDirID;
	else
		*dirId = 2;
}

/************************************************************************
 * ResetHexBin - ready ourselves for a new HexBin file.
 ************************************************************************/
void ResetHexBin(void)
{
	OSpot = 0;
	BSpot = 0;
	RefN = 0;
	Spec.vRefNum = 0;
	*Name = 0;
	Crc = 0;
	LastData = 0;
	State68 = 0;
	B8 = 0;
	Run = False;
	CalcCrc = 0;
}

/************************************************************************
 * comp_q_crc - lifted from xbin
 ************************************************************************/
#define BYTEMASK 0xff
#define BYTEBIT 0x100
#define WORDMASK 0xffff
#define WORDBIT 0x10000
#define CRCCONSTANT 0x1021

void comp_q_crc(unsigned short c)
{
				register unsigned long temp = CalcCrc;

/* Never mind why I call it WOP... */
#define WOP { \
								c <<= 1; \
								if ((temp <<= 1) & WORDBIT) \
												temp = (temp & WORDMASK) ^ CRCCONSTANT; \
								temp ^= (c >> 8); \
								c &= BYTEMASK; \
				}
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				CalcCrc = temp;
}

/************************************************************************
 * CrcError - does the computed crc (CalcCrc) match the given crc (Crc)
 ************************************************************************/
void CrcError(void)
{
	comp_q_crc(0);
	comp_q_crc(0);
	if ((Crc&WORDMASK) != (CalcCrc&WORDMASK))
	{
		WarnUser(CRC_ERROR,CalcCrc);
		BadBinHex = True;
	}
	CalcCrc = 0;
}

