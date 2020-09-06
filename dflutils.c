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

#include <Folders.h>

#include <conf.h>
#include <mydefs.h>

#include "CodeFragmentsSupplement.h"
#include "dflutils.h"
#include "dflsuppl.h"


#define FILE_NUM 110
#pragma segment LDAPUtils

//	Since Apple, in their infinite wisdom, obsoleted this identifier,
//	and provided nothing to replace it.....
enum { kNoConnectionID = 0 };

#if GENERATING68KSEGLOAD
#define USEDFLSTUBLOADER 1
#else
#define USEDFLSTUBLOADER 0
#endif


enum {
	kCFM68KFileType = 'INIT',
	kCFM68KCreatorType = 'cfm8'
};
enum {
	kRsrcModOverrideRsrcType = 'DFLS',
	kRsrcModOverrideRsrcID = 129
};


#if USEDFLSTUBLOADER

#define DFLStubSharedLibName "\pDFLStubLib"

static RoutineDescriptorPtr DFLStub_GetSharedLibraryUPP = nil;
static RoutineDescriptorPtr DFLStub_CloseConnectionUPP = nil;
static CFragConnectionID DFLStubLibConnID = kNoConnectionID;
static Boolean DFLStubLibLoaded = false;

#if GENERATING68KSEGLOAD
#pragma mpwc on
#endif
typedef pascal OSErr (*DFLStub_GetSharedLibraryProcPtr)(ConstStr63Param libName, OSType archType, CFragLoadOptions loadFlags, CFragConnectionID *connID, Ptr *mainAddr, Str255 errMessage);
typedef pascal OSErr (*DFLStub_CloseConnectionProcPtr)(CFragConnectionID *connID);

#define DFLStub_GetSharedLibrary(libName, archType, loadFlags, connID, mainAddr, errMessage) (*(DFLStub_GetSharedLibraryProcPtr)DFLStub_GetSharedLibraryUPP)((libName), (archType), (loadFlags), (connID), (mainAddr), (errMessage))
#define DFLStub_CloseConnection(connID) (*(DFLStub_CloseConnectionProcPtr)DFLStub_CloseConnectionUPP)(connID)
#if GENERATING68KSEGLOAD
#pragma mpwc reset
#endif

OSErr LoadDFLStubLib(OSType libArchType);
OSErr UnloadDFLStubLib(void);

#endif


#if USEDFLSTUBLOADER
Boolean useDFLStubLib = true;
#else
Boolean useDFLStubLib = false;
#endif

static CFragConnectionID curDFRLibConnID = (void *)kNoConnectionID;
static ISAType curDFRLibISA = kOld68kRTA | kM68kISA;
static OSErr curDFRProcInitErr = paramErr;



void ResetDFRLoaderGlobals(void)

{
	curDFRLibConnID = (void *)kNoConnectionID;
	curDFRLibISA = kOld68kRTA | kM68kISA;
	curDFRProcInitErr = paramErr;
}


void SetDFRLoaderGlobals(CFragConnectionID connID, ISAType connISA)

{
	curDFRLibConnID = connID;
	curDFRLibISA = connISA;
	curDFRProcInitErr = noErr;
}


OSErr DFRLoaderErr(void)

{
	return curDFRProcInitErr;
}


static void SetDFRRoutineDescriptor(RoutineDescriptorPtr routineRD, ProcInfoType procInfo, Ptr procAddr, ISAType procISA)

{
	if (!routineRD)
		return;

	/* FINISH *//* just set most of these fields during initial RD declaration */

	routineRD->goMixedModeTrap = _MixedModeMagic;
	routineRD->version = kRoutineDescriptorVersion;
	routineRD->routineDescriptorFlags = kSelectorsAreNotIndexable;
	routineRD->reserved1 = 0;
	routineRD->reserved2 = 0;
	routineRD->selectorInfo = 0;
	routineRD->routineCount = 0;
	routineRD->routineRecords[0].procInfo = procInfo;
	routineRD->routineRecords[0].reserved1 = 0;
	routineRD->routineRecords[0].ISA = procISA;
	routineRD->routineRecords[0].routineFlags = kProcDescriptorIsAbsolute | kFragmentIsPrepared | kUseNativeISA;
	routineRD->routineRecords[0].procDescriptor = (ProcPtr)procAddr;
	routineRD->routineRecords[0].reserved2 = 0;
	routineRD->routineRecords[0].selector = 0;
}


static void CopyCStrToPStr(unsigned char* theCString, Str255 thePString)

{
	unsigned char*	src;
	unsigned char*	dst;
	long						len;


	len = 0;
	src = theCString;
	dst = thePString + 1;
	while (*src && len < 255)
	{
		*dst++ = *src++;
		len++;
	}
	*thePString = len;
}


OSErr InitDFR(char* symbolNameCStr, RoutineDescriptorPtr routineRD, ProcInfoType procInfo)

{
	OSErr							err;
	Str255						symbolName;
	Ptr								symbolAddr;
	CFragSymbolClass	symbolClass;


	if (curDFRProcInitErr)
		return curDFRProcInitErr;
	if (!routineRD || !symbolNameCStr || !*symbolNameCStr || (curDFRLibConnID == (CFragConnectionID)kNoConnectionID))
	{
		err = paramErr;
		goto Error;
	}
	CopyCStrToPStr((unsigned char*)symbolNameCStr, symbolName);
	err = FindSymbol(curDFRLibConnID, symbolName, &symbolAddr, &symbolClass);
	if (err)
		goto Error;
	if ((long)symbolAddr == kUnresolvedCFragSymbolAddress)
	{
		err = cfragNoSymbolErr;
		goto Error;
	}
	if (symbolClass != kTVectorCFragSymbol)
	{
		err = cfragNoSymbolErr;
		goto Error;
	}
	SetDFRRoutineDescriptor(routineRD, procInfo, symbolAddr, curDFRLibISA);
	return noErr;


Error:
	curDFRProcInitErr = err;
	return err;
}


void InvalDFR(RoutineDescriptorPtr routineRD)

{
	if (!routineRD)
		return;

	/* FINISH *//* just set most of these fields during initial RD declaration */

	routineRD->goMixedModeTrap = 0xA89F;	// _Unimplemented;
	routineRD->version = kRoutineDescriptorVersion;
	routineRD->routineDescriptorFlags = kSelectorsAreNotIndexable;
	routineRD->reserved1 = 0;
	routineRD->reserved2 = 0;
	routineRD->selectorInfo = 0;
	routineRD->routineCount = 0;
	routineRD->routineRecords[0].procInfo = 0;
	routineRD->routineRecords[0].reserved1 = 0;
	routineRD->routineRecords[0].ISA = kOld68kRTA | kM68kISA;
	routineRD->routineRecords[0].routineFlags = kProcDescriptorIsAbsolute | kFragmentIsPrepared | kUseNativeISA;
	routineRD->routineRecords[0].procDescriptor = (ProcPtr)0;
	routineRD->routineRecords[0].reserved2 = 0;
	routineRD->routineRecords[0].selector = 0;
}


static OSErr GetSysNativeISA(ISAType *nativeISA)

{
	OSErr		err;
	long		response;


	err = Gestalt(gestaltSysArchitecture, &response);
	if (err)
	{
		err = Gestalt(gestaltNativeCPUtype, &response);
		if (err)
			return err;
		if ((response >= 0) && (response < 0x0100))
			*nativeISA = kM68kISA;
		else if ((response >= 0x0100) && (response < 0x0200))
			*nativeISA = kPowerPCISA;
		else
			return paramErr;
		return noErr;
	}
	switch (response)
	{
		case gestalt68k:
			*nativeISA = kM68kISA;
			break;
		case gestaltPowerPC:
			*nativeISA = kPowerPCISA;
			break;
		default:
			return paramErr;
	}
	return noErr;
}


static OSErr GetCFM68KVers(long *vers)

{
	OSErr				err;
	short				extFldrVRefNum;
	long				extFldrDirID;
	CInfoPBRec	catInfo;
	Str255			fileName;
	short				index;
	Boolean			found;
	long				parID;
	short				origResFile;
	short				initRefNum;
	Handle			versHdl;
	long				longVersNum;


	*vers = 0;

	err = FindFolder(kOnSystemDisk, kExtensionFolderType, kDontCreateFolder, &extFldrVRefNum, &extFldrDirID);
	if (err)
		return err;

	catInfo.hFileInfo.ioNamePtr = fileName;
	catInfo.hFileInfo.ioVRefNum = extFldrVRefNum;
	err = noErr;
	found = false;
	index = 0;
	while (!found && !err)
	{
		index++;
		catInfo.hFileInfo.ioFDirIndex = index;
		catInfo.hFileInfo.ioDirID = extFldrDirID;
		err = PBGetCatInfoSync(&catInfo);
		if (err)
			continue;
		if (catInfo.hFileInfo.ioFlAttrib & ioDirMask)
			continue;
		if ((catInfo.hFileInfo.ioFlFndrInfo.fdType == kCFM68KFileType) && (catInfo.hFileInfo.ioFlFndrInfo.fdCreator == kCFM68KCreatorType))
			found = true;
	}
	if (err)
		return err;
	if (!found)
		return fnfErr;

	parID = catInfo.hFileInfo.ioFlParID;

	origResFile = CurResFile();
	initRefNum = HOpenResFile(extFldrVRefNum, parID, fileName, fsRdWrPerm);
	err = ResError();
	if (err)
		return err;
	versHdl = Get1Resource('vers', 1);
	err = ResError();
	if (!err && (!versHdl || !*versHdl))
		err = resNotFound;
	if (!err && (GetHandleSize(versHdl) < 8))
		err = paramErr;
	if (err)
	{
		UseResFile(origResFile);
		return err;
	}
	longVersNum = **(long**)versHdl;
	CloseResFile(initRefNum);
	err = ResError();
	UseResFile(origResFile);
	if (err)
		return err;

	*vers = longVersNum;
	return noErr;
}


Boolean CFM68KOkay(void)

{
	OSErr		err;
	long		response;
	ISAType	nativeISA;


	err = GetSysNativeISA(&nativeISA);
	if (err)
		return false;
	if (nativeISA != kM68kISA)
		return true;

	err = Gestalt(gestaltSystemVersion, &response);
	if (err)
		return false;
	if ((response & 0x0000FF00) >= 0x00000800)
		return true;

	err = GetCFM68KVers(&response);
	if (err)
		return false;
	if ((response & 0xFF000000) < 0x04000000)
		return false;

	return true;
}


OSErr SystemSupportsDFRLibraries(OSType libArchType)
{
	return noErr;
}


#if USEDFLSTUBLOADER
static OSErr LoadDFLStubLib(OSType libArchType)

{
	OSErr		err, scratchErr;
	Ptr			mainAddr;
	Str255	errName;
	Ptr								symbolAddr;
	CFragSymbolClass	symbolClass;
	Handle	h;
	Boolean	doFragRsrcMod;
	Handle	code0Hdl;
	short		code0ResFile;
	short		origResFile;
	Handle	cfrgHdl;
	CodeFragRsrcPtr			cfrgPtr;
	CodeFragInfoRecPtr	curFragInfoRecPtr;
	long		numFrags;
	long		curFragIndex;
	Boolean	found;


	if (DFLStubLibLoaded)
		return noErr;

	doFragRsrcMod = false;
	h = GetResource(kRsrcModOverrideRsrcType, kRsrcModOverrideRsrcID);
	if (h)
	{
		if (*h && (GetHandleSize(h) == 1))
			doFragRsrcMod = **h ? true : false;
		ReleaseResource(h);
	}
	if (doFragRsrcMod)
	{
		code0Hdl = GetResource('CODE', 0);
		err = ResError();
		if (!err && (!code0Hdl || !*code0Hdl))
			err = resNotFound;
		if (err)
			return err;
		code0ResFile = HomeResFile(code0Hdl);
		err = ResError(); /* no error expected */
		if (err)
			return err;
		origResFile = CurResFile();
		UseResFile(code0ResFile);
		err = ResError(); /* no error expected */
		if (err)
			return err;
		cfrgHdl = Get1Resource(kCFragResourceType, kCFragResourceID);
		err = ResError();
		UseResFile(origResFile);
		if (!err && (!cfrgHdl || !*cfrgHdl))
			err = resNotFound;
		if (err)
			return err;
		HNoPurge(cfrgHdl);
		HLock(cfrgHdl);

		cfrgPtr = (CodeFragRsrcPtr)*cfrgHdl;
		if (cfrgPtr->cfrgRsrcVers != kCurrentCfrgRsrcVers)
			return paramErr;
		numFrags = cfrgPtr->numFragInfoRecs;

		found = false;
		curFragIndex = 0;
		curFragInfoRecPtr = (CodeFragInfoRecPtr)&cfrgPtr->firstFragInfoRec;
		while (!found && (curFragIndex < numFrags))
		{
			if ((curFragInfoRecPtr->fragArch == libArchType) && EqualString(DFLStubSharedLibName, curFragInfoRecPtr->fragName, true, true))
			{
				found = true;
				break;
			}
			curFragIndex++;
			curFragInfoRecPtr = (CodeFragInfoRecPtr)((long)curFragInfoRecPtr + curFragInfoRecPtr->infoRecLen);
		}
		HUnlock(cfrgHdl);
		if (!found)
			return paramErr;
		curFragInfoRecPtr->fragType = kIsApp;
	}

	err = GetSharedLibrary(DFLStubSharedLibName, libArchType, kLoadNewCopy, &DFLStubLibConnID, &mainAddr, errName);
	if (err)
		return err;

	err = FindSymbol(DFLStubLibConnID, "\pDFLStub_GetSharedLibraryUPP", &symbolAddr, &symbolClass);
	if (err)
		goto Error;
	if (symbolClass != kDataSym)
	{
		err = cfragNoSymbolErr;
		goto Error;
	}
	DFLStub_GetSharedLibraryUPP = *(RoutineDescriptorPtr*)symbolAddr;

	err = FindSymbol(DFLStubLibConnID, "\pDFLStub_CloseConnectionUPP", &symbolAddr, &symbolClass);
	if (err)
		goto Error;
	if (symbolClass != kDataSym)
	{
		err = cfragNoSymbolErr;
		goto Error;
	}
	DFLStub_CloseConnectionUPP = *(RoutineDescriptorPtr*)symbolAddr;

	DFLStubLibLoaded = true;
	return noErr;


Error:
	scratchErr = CloseConnection(&DFLStubLibConnID);
	if (!scratchErr)
		DFLStubLibConnID = kNoConnectionID;
	DFLStub_GetSharedLibraryUPP = nil;
	DFLStub_CloseConnectionUPP = nil;
	return err;
}
#endif


#if USEDFLSTUBLOADER
static OSErr UnloadDFLStubLib(void)

{
	OSErr		err;


	if (!DFLStubLibLoaded)
		return noErr;

	err = CloseConnection(&DFLStubLibConnID);
	if (err)
		return err;

	DFLStub_GetSharedLibraryUPP = nil;
	DFLStub_CloseConnectionUPP = nil;
	DFLStubLibConnID = kNoConnectionID;
	DFLStubLibLoaded = false;
	return noErr;
}
#endif


OSErr LoadDFRLibrary(ConstStr63Param libName, OSType libArchType, CFragConnectionID *connID, Str255 errMessage)

{
	OSErr			err;
	Ptr				mainAddr;
	Str255		errStr_local;
	StringPtr	errStrPtr;
	ISAType		libISA;
#if USEDFLSTUBLOADER
	OSErr			scratchErr;
#endif


	ResetDFRLoaderGlobals();
	*connID = (void *)kNoConnectionID;

	if (!libName || !*libName || !connID)
		return paramErr;

	if ((libArchType != kPowerPCCFragArch) && (libArchType != kMotorola68KCFragArch))
		if (libArchType == kAnyCFragArch)
		{
			if (GetCurrentISA() == kPowerPCISA)
				libArchType = kPowerPCCFragArch;
			else if (GetCurrentISA() == kM68kISA)
				libArchType = kMotorola68KCFragArch;
			else
				return cfragArchitectureErr;
		}
		else
			return cfragArchitectureErr;

	err = SystemSupportsDFRLibraries(libArchType);
	if (err)
		return err;

	libISA = (libArchType == kPowerPCCFragArch) ? (kPowerPCISA | kPowerPCRTA) : (kM68kISA | kCFM68kRTA);
	errStrPtr = errMessage ? errMessage : errStr_local;

#if USEDFLSTUBLOADER
	if (useDFLStubLib)
	{
		err = LoadDFLStubLib(libArchType);
		if (err)
			return err;
		err = DFLStub_GetSharedLibrary(libName, libArchType, kPrivateCFragCopy, connID, &mainAddr, errStrPtr);
		if (err)
		{
			scratchErr = UnloadDFLStubLib();
			return err;
		}
	}
	else
	{
		err = GetSharedLibrary(libName, libArchType, kPrivateCFragCopy, connID, &mainAddr, errStrPtr);
		if (err)
			return err;
	}
#else
	err = GetSharedLibrary(libName, libArchType, kPrivateCFragCopy, connID, &mainAddr, errStrPtr);
	if (err)
		return err;
#endif

	SetDFRLoaderGlobals(*connID, libISA);

	return noErr;
}


OSErr UnloadDFRLibrary(CFragConnectionID *connID)

{
	OSErr		err;
#if USEDFLSTUBLOADER
	OSErr		scratchErr;
#endif


#if USEDFLSTUBLOADER
	if (useDFLStubLib)
		err = DFLStub_CloseConnection(connID);
	else
		err = CloseConnection(connID);
#else
	err = CloseConnection(connID);
#endif
	if (!err)
		*connID = (void *)kNoConnectionID;
#if USEDFLSTUBLOADER
	if (!err)
		if (useDFLStubLib)
			scratchErr = UnloadDFLStubLib();
#endif
	return err;
}
