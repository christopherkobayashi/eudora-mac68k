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

/* Copyright (c) 1996 by Qualcomm, Inc. */

#include "netscapeplugins.h"
#include "npapi.h"
#include "npupp.h"

//	!! This area needs work !!
static Byte	sPluginFolder[] = "\pNetscape Plug-ins";
#define OPEN_NPLUGIN OPEN_SETTINGS
#define FILE_NUM 82

//	�� Constants ��
enum	{ kMinorVersion = NPVERS_HAS_STREAMOUTPUT };			//	This is the minimum version of Netscape we support
enum	{ kPluginResType = 'NSPL', kPluginResID = 128 };	//	Code resource for 68K code

#pragma segment NPLUGINS

//	�� Structs ��

//	Info for an active plugin
typedef struct
{
	ConnectionID			connID;		//	Used for PPC code
	Handle						hCode;		//	Used for 68K code
	short							fRefNum;	//	Resource file refnum
	NPPluginFuncs			pluginFuncs;
	NPP_ShutdownUPP		unloadFunc;
	NPSavedData				*savedData;
} ActivePluginInfo, *ActivePluginPtr, **ActivePluginHandle;

typedef struct
{
	NPWindow	wind;
	NP_Port	npPort;
} ndataRec;

//	Info for each plugin file
typedef struct
{
	Str32								fileName;
	Handle							hMIMEandFileExt;	//	STR# resource 128
	short								instanceCount;		//	How many instances are active?
	ActivePluginHandle	hData;
} PluginFileInfo, *PluginFilePtr, **PluginFileHandle;

typedef struct
{
	short			pluginIdx;	//	Index into ghPlugins
	NPP_t			instance;
} InstanceInfo, *InstancePtr, **InstanceHandle;

//	�� Globals ��
static	short	gPluginCount;
static	PluginFileHandle	ghPlugins;
static	short	gPVRefNum;
static	long	gPDirID;
static	NPNetscapeFuncs	gNetscapeFuncs;

//	�� Prototypes ��
static void GetToken(StringPtr sPtr,StringPtr sToken,short *pIdx);
static Boolean InitPlugin(PluginFilePtr pPlugin);
static void RegisterPlugins(void);

/**********************************************************************
 * CheckNetscapePlugin - Find any Netscape plugins
 **********************************************************************/
Handle NPluginCheck(FSSpec *pSpec, Rect *pRect)
{
	//	Find a .extension
	short	i;
	Str32	sExtension;
	Str32	sToken;
	StringPtr	sPtr;
	InstanceHandle	hInstance=nil;
	Str255	sMIMEcstring;
	
	RegisterPlugins();
	for(i=*pSpec->name,sPtr=pSpec->name+i;i;i--)
	{
		if (*sPtr--=='.')
		{
			BMD(sPtr+2,sExtension+1,*sExtension=*pSpec->name-i);
			break;
		}
	}
	
	if (i)
	{
		//	Search for the plugin
		short	plugin;
		
		for(plugin=0;plugin<gPluginCount;plugin++)
		{
			Handle	hStrings;
			short		count,idx;
			
			hStrings = (*ghPlugins)[plugin].hMIMEandFileExt;
			sPtr = (StringPtr)*hStrings;
			count = *(short *)sPtr;
			sPtr += 2;
			for (idx=0;idx<count;idx+=2)
			{
				short	idxToken;
				StringPtr	sMIME;
				
				sMIME = sPtr;		//	Save pointer to MIME
				sPtr += *sPtr+1;
				idxToken = 1;
				while (idxToken)
				{					
					GetToken(sPtr,sToken,&idxToken);
					if (*sToken==*sExtension && strincmp(sExtension+1,sToken+1,*sExtension)==0)
					{
						//	Found a file name extension match!
						//	Save the MIME as c-string
						PtoCcpy(sMIMEcstring, sMIME);
						
						//	Create an instance
						HLock((Handle)ghPlugins);
						if ((*ghPlugins)[plugin].instanceCount || InitPlugin(&(*ghPlugins)[plugin]))
						{
							if (hInstance = NuHandleClear(sizeof(InstanceInfo)))
							{
								NPP_NewUPP	pNewProc;
								char *argn[] = { "HEIGHT", "WIDTH" };
								char *argv[] = { "100", "100" };
								short	argc = 2;
								
								HLock((Handle)hInstance);
								(*hInstance)->pluginIdx = plugin;
								pNewProc = (*(*ghPlugins)[plugin].hData)->pluginFuncs.newp;
								if (CallNPP_NewProc(pNewProc,sMIMEcstring, &(*hInstance)->instance, NP_FULL, argc, 
									argn, argv, (*(*ghPlugins)[plugin].hData)->savedData))
								{
									//	Error
									ZapHandle(hInstance);
								}
								else
								{
									(*hInstance)->instance.ndata = NewPtrClear(sizeof(ndataRec));
									(*ghPlugins)[plugin].instanceCount++;
									HUnlock((Handle)hInstance);
								}
							}					
						}
						
						HUnlock((Handle)ghPlugins);
						goto Done;
					}
				}
				sPtr += *sPtr+1;
			}
		}
	}
	
 Done:	
	return (Handle)hInstance;
}

/**********************************************************************
 * NPluginClose - Close an instance
 **********************************************************************/
void NPluginClose(Handle h)
{
	InstanceHandle			hInstance = (InstanceHandle)h;
	short								pluginIdx = (*hInstance)->pluginIdx;
	ActivePluginHandle	hData = (*ghPlugins)[pluginIdx].hData;
	
	//	Close the instance
	HLock((Handle)hData);
	HLock((Handle)hInstance);
	CallNPP_DestroyProc((*hData)->pluginFuncs.destroy,&(*hInstance)->instance,&(*hData)->savedData);
	ZapHandle(hInstance);
	
	if (--(*ghPlugins)[pluginIdx].instanceCount == 0)
	{
		//	Last instance. Close down the plugin.
		CallNPP_ShutdownProc((*hData)->unloadFunc);	//	Call function's NPP_Destroy
		CloseConnection(&(*hData)->connID);	//	Get rid of code fragment
		if ((*hData)->savedData)
		{
			//	Get rid of savedData
			if ((*hData)->savedData->buf)
				DisposePtr((*hData)->savedData->buf);
			DisposePtr((Ptr)(*hData)->savedData);
		}
		ZapHandle(hData);	//	Get rid of active data
	}

	if (hData)
		UL(hData);
}

/**********************************************************************
 * NPluginDraw - Draw the plugin
 **********************************************************************/
void NPluginDraw(Handle h, Rect *pRect, CGrafPtr	port, Boolean fSetWindow)
{
	EventRecord	event;
	InstanceHandle			hInstance = (InstanceHandle)h;
	short								pluginIdx = (*hInstance)->pluginIdx;
	ActivePluginHandle	hData = (*ghPlugins)[pluginIdx].hData;
	
	HLock(h);
	if (fSetWindow)
	{
		InstancePtr		pInstance;
		ndataRec	*pNData;
		
		pInstance = *hInstance;
		pNData = (ndataRec*)(pInstance->instance.ndata);
		pNData->npPort.port = port;
		pNData->npPort.portx = -pRect->left;
		pNData->npPort.porty = -pRect->top;
		pNData->wind.window = &pNData->npPort;
		pNData->wind.x = pRect->left;
		pNData->wind.y = pRect->top;
		pNData->wind.width = pRect->right - pRect->left;
		pNData->wind.height = pRect->bottom - pRect->top;
		pNData->wind.clipRect.top = pRect->top;
		pNData->wind.clipRect.left = pRect->left;
		pNData->wind.clipRect.bottom = pRect->bottom;
		pNData->wind.clipRect.right = pRect->right;
		CallNPP_SetWindowProc((*hData)->pluginFuncs.setwindow,&pInstance->instance, &pNData->wind);		
	}
	
	//	Since we never received an update event, we need to create one
	EventAvail(everyEvent, &event);
	event.what = updateEvt;
	event.message = (long)port;
	NPluginEvent(h, &event);
	HUnlock(h);
}

/**********************************************************************
 * NPluginEvent - Handle an event
 **********************************************************************/
void NPluginEvent(Handle h, EventRecord *pEvent)
{
	InstanceHandle			hInstance = (InstanceHandle)h;
	short								pluginIdx = (*hInstance)->pluginIdx;
	ActivePluginHandle	hData = (*ghPlugins)[pluginIdx].hData;

	HLock((Handle)hInstance);	
	CallNPP_HandleEventProc((*hData)->pluginFuncs.event,&(*hInstance)->instance, pEvent);

	//	For now, pass a null event also
	{
		EventRecord	nullEv;
		
		nullEv = *pEvent;
		nullEv.what = nullEvent;
		CallNPP_HandleEventProc((*hData)->pluginFuncs.event,&(*hInstance)->instance,&nullEv);		
	}

	UL(hInstance);
}

/**********************************************************************
 * RegisterPlugins - Find any Netscape plugins
 **********************************************************************/
static void RegisterPlugins(void)
{
	static	Boolean	fRegistered;
	
	if (!fRegistered)
	{
		FSSpec	spec;
		OSErr		err;
		
		if (!GetFileByRef(AppResFile,&spec) &&
				!(err = FSMakeFSSpec(spec.vRefNum,spec.parID,sPluginFolder/*GetRString(name,STUFF_FOLDER)*/,&spec)))
			{
				CInfoPBRec hfi;
				Str31 name;
				short refN;
				short saveRefN = CurResFile();

				//	Resolve the plugins folder
				IsAlias(&spec,&spec);
				spec.parID = SpecDirId(&spec);
				gPVRefNum = spec.vRefNum;
				gPDirID = spec.parID;

				hfi.hFileInfo.ioNamePtr = name;
				hfi.hFileInfo.ioFDirIndex = 0;
				
				ghPlugins = NuHandle(0);
				
				//	Find each plugin
				while(!DirIterate(gPVRefNum,gPDirID,&hfi))
					if (hfi.hFileInfo.ioFlFndrInfo.fdType=='NSPL')
					{
						if ((refN=HOpenResFile(gPVRefNum, gPDirID, name, fsRdPerm))==-1)
							FileSystemError(OPEN_NPLUGIN,name,ResError());
						else
						{
							PluginFileInfo	plugin;
						
							Zero(plugin);
							if (plugin.hMIMEandFileExt = Get1Resource('STR#',128))
							{
								DetachResource(plugin.hMIMEandFileExt);
								PCopy(plugin.fileName,name);
								if (!PtrAndHand(&plugin, (Handle)ghPlugins, sizeof(plugin)))
									gPluginCount++;
							}
							CloseResFile(refN);
						}
					}
		
			UseResFile(saveRefN);
		}
		
		//	Set up some data that will be needed by the plugins
		//	Address of callback functions
		gNetscapeFuncs.size = sizeof(gNetscapeFuncs);
    gNetscapeFuncs.version = (NP_VERSION_MAJOR << 8) + NPVERS_HAS_STREAMOUTPUT;
    gNetscapeFuncs.geturl = NewNPN_GetURLProc(NPN_GetURL);
    gNetscapeFuncs.posturl = NewNPN_PostURLProc(NPN_PostURL);
    gNetscapeFuncs.requestread = NewNPN_RequestReadProc(NPN_RequestRead);
    gNetscapeFuncs.newstream = NewNPN_NewStreamProc(NPN_NewStream);
    gNetscapeFuncs.write = NewNPN_WriteProc(NPN_Write);
    gNetscapeFuncs.destroystream = NewNPN_DestroyStreamProc(NPN_DestroyStream);
    gNetscapeFuncs.status = NewNPN_StatusProc(NPN_Status);
    gNetscapeFuncs.uagent = NewNPN_UserAgentProc(NPN_UserAgent);
    gNetscapeFuncs.memalloc = NewNPN_MemAllocProc(NPN_MemAlloc);
    gNetscapeFuncs.memfree = NewNPN_MemFreeProc(NPN_MemFree);
    gNetscapeFuncs.memflush = NewNPN_MemFlushProc(NPN_MemFlush);
    gNetscapeFuncs.reloadplugins = NewNPN_ReloadPluginsProc(NPN_ReloadPlugins);
    gNetscapeFuncs.getJavaEnv = NewNPN_GetJavaEnvProc(NPN_GetJavaEnv);
    gNetscapeFuncs.getJavaPeer = NewNPN_GetJavaPeerProc(NPN_GetJavaPeer);
    gNetscapeFuncs.geturlnotify = NewNPN_GetURLNotifyProc(NPN_GetURLNotify);
    gNetscapeFuncs.posturlnotify = NewNPN_PostURLNotifyProc(NPN_PostURLNotify);

		fRegistered = true;
	}
}

/**********************************************************************
 * GetToken - get a token from a comma delimited string
 **********************************************************************/
static void GetToken(StringPtr sPtr,StringPtr sToken,short *pIdx)
{
	short	idx;
	
	for (idx = *pIdx;idx <= *sPtr && sPtr[idx]!=',';idx++);	//	Find end of string or comma
	BMD(sPtr+*pIdx,sToken+1,*sToken = idx-*pIdx);	//	Copy of token
	*pIdx = idx < *sPtr ? idx+1 : 0;	//	Where do we start next time (if at all)?
	TrimInitialWhite(sToken);
}

/**********************************************************************
 * InitPlugin - Initialize a plugin
 **********************************************************************/
static Boolean InitPlugin(PluginFilePtr pPlugin)
{
		FSSpec	spec;
		Boolean	result = false;
		Str255	s;
		Ptr		mainAddr;
		ActivePluginHandle	hData;
		ActivePluginPtr			pData;
		short	fRefNum;
		short	saveResFile;
	
		pPlugin->hData = hData = (ActivePluginHandle)NuHandleClear(sizeof(ActivePluginInfo));
		if (!hData)
			return false;
		
		saveResFile = CurResFile();
		HLock((Handle)hData);
		pData = *hData;
		if (!FSMakeFSSpec(gPVRefNum,gPDirID,pPlugin->fileName,&spec))
		{
			IsAlias(&spec,&spec);
			
			fRefNum = FSpOpenResFile(&spec, fsCurPerm);
			if (fRefNum != -1)
			{
				short	saveResFile = CurResFile();
				UniversalProcPtr	pMainUPP;
				short		whatType = 0;
				Handle	hCode;
				
				UseResFile(fRefNum);
				pData->fRefNum = fRefNum;
				
				//	Try PPC code fragment first
				if (!GetDiskFragment(&spec, 0, kWholeFork, "", kLoadNewCopy, &pData->connID, &mainAddr, s))
					whatType = 1;	//	PPC

				//	Failed, try 68K code resource
				else if (hCode = Get1Resource(kPluginResType,kPluginResID))
				{
					//	Got 68K code
					HLock(hCode);
					mainAddr = *hCode;
					whatType = 2;	//	68K
				}
				
				if (whatType)
				{
					NPError	NErr;
					
					pMainUPP = NewRoutineDescriptor((ProcPtr)mainAddr, uppNPP_MainEntryProcInfo, whatType==2 ? kM68kISA : kPowerPCISA);
					HLock((Handle)hData);
					pData->pluginFuncs.size = sizeof(NPPluginFuncs);
    			pData->pluginFuncs.version = (NP_VERSION_MAJOR << 8) + NPVERS_HAS_STREAMOUTPUT;
					NErr = CallNPP_MainEntryProc(pMainUPP, &gNetscapeFuncs, &pData->pluginFuncs, &pData->unloadFunc);
					HUnlock((Handle)hData);
					DisposeRoutineDescriptor(pMainUPP);
					if (NErr)
					{
						if (whatType==1) CloseConnection(&pData->connID);
					}
					else
						result = true;
				}
				
				if (!result)
					CloseResFile(fRefNum);	//	Failed, closed the resource file			
				UseResFile(saveResFile);
			}
		}
		UL(hData);
		UseResFile(saveResFile);
		return result;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Netscape callback functions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//	NPN_DestroyStream
NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
	DebugStr("\pDestroy Stream");	
}

//	NPN_GetJavaEnv
JRIEnv* NPN_GetJavaEnv(void){}				//	Unsupported
//	NPN_GetJavaPeer
jref NPN_GetJavaPeer(NPP instance){}	//	Unsupported

//	NPN_GetURL
NPError NPN_GetURL(NPP instance, const char* url,const char* target)
{
	return NPERR_FILE_NOT_FOUND;
}

//	NPN_GetURLNotify
NPError NPN_GetURLNotify(NPP instance, const char* url,const char* target, void* notifyData)
{	
	return NPERR_FILE_NOT_FOUND;
}

//	NPN_MemAlloc
void* NPN_MemAlloc(uint32 size)
{
	return NewPtr(size);
}

//	NPN_MemFlush
uint32 NPN_MemFlush(uint32 size)
{
	uint32	startFree,needed;
	
	//	Start by compacting memory. If that's not enough, do a purge also
	startFree = FreeMem();
	needed = startFree+size;
	CompactMem(needed);
	if (FreeMem() < needed)
		PurgeMem(needed);
	return FreeMem() - startFree;	//	Amount of freed memory
}

//	NPN_MemFree
void NPN_MemFree(void* ptr)
{
	DisposePtr(ptr);
}

//	NPN_NewStream
NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
	DebugStr("\pNew Stream");
	return NPERR_NO_DATA;
}

//	NPN_PostURL
NPError NPN_PostURL(NPP instance, const char* url, const char* target, uint32 len,
							const char* buf, NPBool file)
{
	DebugStr("\pPost URL");
	return NPERR_NO_DATA;
}

//	NPN_PostURLNotify
NPError NPN_PostURLNotify(NPP instance, const char* url, const char* target, uint32 len,
		const char* buf, NPBool file, void* notifyData)
{
	DebugStr("\pPost URL");
	return NPERR_NO_DATA;
}
								
//	NPN_RequestRead
NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
	DebugStr("\pRequest Read");
	return NPERR_NO_DATA;
}

//	NPN_Status
void NPN_Status(NPP instance, const char* message)
{
}

//	NPN_UserAgent
const char* NPN_UserAgent(NPP instance)
{
	return "Eudora";
}

//	NPN_Write
int32 NPN_Write(NPP instance, NPStream* stream, int32 len, void* buffer)
{
	DebugStr("\pRequest Read");
	return NPERR_NO_DATA;
}

//	NPN_ReloadPlugins
void NPN_ReloadPlugins(NPBool reloadPages)
{
	DebugStr("\Reload Plugins");
}
