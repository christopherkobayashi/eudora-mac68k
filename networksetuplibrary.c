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

#include <conf.h>
#include <mydefs.h>

#include "networksetuplibrary.h"
#define FILENUM 122
/* Copyright (c) 1999 by Qualcomm, Inc. */
/************************************************************************
 * Functions to use Apple's Network Setup Library included with Mac OS
 * 8.5 and beyond.  The library is used to determine the TCP/IP connection
 * method, as well as read in ARA PPP dialing preferences.
 ************************************************************************/
 
#pragma segment TcpTrans

// TCP/IP Will Dial Result Codes
enum 
{
	kNSHTCPDialUnknown = 0,
	kNSHTCPDialTCPDisabled,
	kNSHTCPDialYes,
	kNSHTCPDialNo
};

// Globals to keep track of NS Library database
Boolean gNSLValid = false;
MNSDatabadeRef gRef;

//
// Network Setup Library prototypes
//

// General
Boolean RemoteVersionAcceptable(UInt32 version);
#ifdef	USE_NETWORK_SETUP
short SysVersForNSL(void);
Boolean IsNSBroken(void);
OSErr GetCurrentPortNameFromDatabase(char *portNameLong, char *portName, Boolean *enabled);
OSErr ReEnableNSLibrary(void);
OSErr FindNSLibrary(short vRef, long dirId, FSSpecPtr spec);

// OT/PPP and MacSLIP cooperation
static OSStatus GetDialingInfoFromRemoteEntity(const MNSDatabadeRef *ref, const CfgEntityRef *entityID, Boolean *redial, unsigned long *numRedials, unsigned long *delay);
static OSStatus GetPortNameFromTCPEntity(const MNSDatabadeRef *ref, const CfgEntityRef *entityID, unsigned char *longPortName, char *portName, Boolean *enabled);
static OSStatus FindCurrentConnection(const MNSDatabadeRef *ref, OSType protocol, CfgEntityRef *currentEntity);
#endif	//USE_NETWORK_SETUP

// TCPWillDial
static OSStatus NSHTCPWillDial(UInt32 *willDial);
static OSStatus GetTCPInfo(Boolean *enabled, unsigned char *portNameLong, char *portName);
static OSStatus GetPortNameFromTCPPrefs(Ptr buffer, SInt32 prefSize, char *portName);
static void UnpackTCPPrefs(Ptr *buffer, NSHTCPv4ConfigurationDigest *configurationDigest);

/*********************************************************************************
 * UseNetworkSetup - return true if the Network Setup Library is to be used.
 *********************************************************************************/	
Boolean UseNetworkSetup(void)
{
	Boolean useIt = false;
	
#ifdef USE_NETWORK_SETUP
	// NS Library must be used if the OS version is greater than 8.6
	if (SysVersForNSL() > 0x0860)
	{
		if (IsNetworkSetupAvailable())
	 	{
	 		useIt = true;
	 		gMissingNSLib = false;		// the Network Setup Library may have become available
	 	}
	 	else
	 	{
	 		// we need to NS Library, but the extension is missing.  Warn the user about this once.
	 		if (!gMissingNSLib && !InAThread() && !PrefIsSet(PREF_NO_NS_LIB_WARNING))
	 		{
	 			{
	 				// The user has opted to continue without it.  Never warn them again.
	 				SetPref(PREF_NO_NS_LIB_WARNING, YesStr);
	 			}
	 		}
	 		
	 		// grey out the dialup features, because they won't function properly without the NS Libary
			gMissingNSLib = true;
	 	}
	}
	else
	{
		// otherwise, use the NS Library if the preference to use it is set, and it's available.
		if (PrefIsSet(PREF_USE_NETWORK_SETUP) && IsNetworkSetupAvailable()) useIt = true;
	}
#endif
	
	return (useIt);
}

#ifdef USE_NETWORK_SETUP
/*********************************************************************************
 * SysVersForNSL - return the system version running on this machine
 *********************************************************************************/	
short SysVersForNSL(void)
{
 	static short sysVers = 0;
	long result;
		
	if (!sysVers) 
	{
		Gestalt(gestaltSystemVersion, &result);
		sysVers = LoWord(result);
	}
	
	return (sysVers);
}

/*********************************************************************************
 * IsNSBroken - return if this machine is running the 1.0 version of NS Library
 *********************************************************************************/	
Boolean IsNSBroken(void)
{				
	// Crap: This no longer works in 8.7b3c3.  OTCfgEncrypt can be called, but NS Lib is still busted.
	return (true);
					 
	// version 1.1 and beyond implements OTCfgEncrypt(), a routine to munge ARA passwords
	//return ((long)(&OTCfgEncrypt) == kUnresolvedCFragSymbolAddress);
}
	
/*********************************************************************************
 * InitNetworkSetup - initialize things so we can use the Network Setup Library
 *********************************************************************************/	
void InitNetworkSetup(void)
{
	WriteZero(&gRef, sizeof(MNSDatabadeRef));
	gNSLValid = true;
}

/*********************************************************************************
 * CloseNetworkSetup - we're finished with Network Setup.
 *********************************************************************************/
OSErr CloseNetworkSetup(void)
{
	OSStatus err = noErr;
	
	if (gNSLValid && MNSValidDatabase(&gRef)) err = MNSCloseDatabase(&gRef,false);
	gNSLValid = false;
	
	return (err);
}

/*********************************************************************************
 * ReEnableNSLibrary - enable the network setup extension.  Warn the user.
 *********************************************************************************/
OSErr ReEnableNSLibrary(void)
{
	OSErr err = noErr;
	short evRef, dvRef;
	long edirId, ddirId;
	FSSpec nsSpec, nsSpecNew;
	
	// locate the Extensions folder
	err = FindFolder(kOnSystemDisk, kExtensionFolderType, false, &evRef, &edirId);
	if (err == noErr)
	{
		// locate the disabled Extensions folder
		err = FindFolder(kOnSystemDisk, kExtensionDisabledFolderType, false, &dvRef, &ddirId);
		if (err == noErr)
		{
			// locate the Network Setup Extension within the disabled Extensions folder
			err = FindNSLibrary(dvRef, ddirId, &nsSpec);
			if (err == noErr)
			{
				// make sure there's nothing else with that name in the Extensions folder already
				err = FSMakeFSSpec(evRef, edirId, nsSpec.name, &nsSpecNew);
				if (err == noErr) err = dupFNErr;
				
				// Now move the file
				if (err == fnfErr) err = SpecMove(&nsSpec, &nsSpecNew);
			}
		}
	}
	
	if (err == noErr) ComposeStdAlert(Note, NS_LIB_ENABLED);
	else ComposeStdAlert(Caution, NS_LIB_NOT_ENABLED);
	
	return (err);
}

/*********************************************************************************
 * FindNSLibrary - Locate the NS Library.
 *********************************************************************************/
OSErr FindNSLibrary(short vRef, long dirId, FSSpecPtr spec)
{
	CInfoPBRec hfi;
	Str31 name;
	OSErr err = fnfErr;
		
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	
	while((err==fnfErr) && !DirIterate(vRef,dirId,&hfi))
	{
		if ((hfi.hFileInfo.ioFlFndrInfo.fdType == NS_LIBRARY_TYPE &&
			hfi.hFileInfo.ioFlFndrInfo.fdCreator == NS_LIBRARY_CREATOR))
		{
			// Found it.
			FSMakeFSSpec(vRef, dirId, name, spec);
			err = noErr;
		}
	}
	
	return (err);
}

static pascal void TypeAndClassIterator(const MNSDatabadeRef *ref, CfgSetsElement *thisElement, void *p)
	// This routine is used as a callback for MNSIterateSet.
	// It looks for the entity specified by the fType and fClass
	// fields of the param, and puts its entityRef into the
	// variable pointed to by the currentEntity field of param.
{
	TypeAndClassParam *param;
	
	param = (TypeAndClassParam *) p;

	MoreAssertQ(MNSValidDatabase(ref));
	MoreAssertQ(thisElement != nil);
	MoreAssertQ(param != nil);
	MoreAssertQ(param->currentEntity != nil);
	
	if (thisElement->fEntityInfo.fClass == param->fClass && thisElement->fEntityInfo.fType == param->fType) {
		MoreAssertQ( ! param->found );
		param->found = true;
		*(param->currentEntity) = thisElement->fEntityRef;
	}
}

static void UnpackIITF(Ptr *buffer, TCPiitfPref *unpackedIITF)
	// This routine unpacks an interface from an 'iitf' preference
	// into a TCPiitfPref.  *buffer must point to the beginning
	// of the interface, ie two bytes into the pref data if
	// if you're extracting the first interface.  *buffer
	// is updated to point to the byte after the last byte
	// parsed, so you can parse multiple interfaces by
	// repeatedly calling this routine.
{
	UInt8 *cursor;
	
	cursor = (UInt8 *) *buffer;
	
	unpackedIITF->fActive = *cursor;
	cursor += sizeof(UInt8);
	unpackedIITF->fIPAddress = *((InetHost *) cursor);
	cursor += sizeof(InetHost);
	unpackedIITF->fSubnetMask = *((InetHost *) cursor);
	cursor += sizeof(InetHost);
	BlockMoveData(cursor, unpackedIITF->fAppleTalkZone, *cursor + 1);
	cursor += (*cursor + 1);
	BlockMoveData(cursor, unpackedIITF->fPath, 36);
	cursor += 36;
	BlockMoveData(cursor, unpackedIITF->fModuleName, 32);
	cursor += 32;
	unpackedIITF->fFramingFlags = *((UInt32 *) cursor);
	cursor += sizeof(UInt32);

	*buffer = (Ptr) cursor;
}

/*********************************************************************************
 * GetConnectionModeFromDatabase - return the currently selected TCP/IP transport
 *********************************************************************************/
OSErr GetConnectionModeFromDatabase(unsigned long *connectionSelection)
{
	OSStatus err = noErr;
	char currentPortName[kMaxProviderNameSize];
	char currentLongPortName[kMaxProviderNameSize];
	Boolean enabled;
	
#ifdef	DEBUG
	ASSERT(connectionSelection);
#endif
	*connectionSelection = kOtherSelected;

	// Read the port name of the current TCP/IP transport
	err = GetCurrentPortNameFromDatabase(currentLongPortName, currentPortName, &enabled);
	if (err == noErr)
	{
		if (StringSame(currentLongPortName,PPP_NAME))	
			*connectionSelection = kPPPSelected;
	}

	return (err);
}

/*********************************************************************************
 * GetConnectionModeFromDatabase - return the currently selected TCP/IP transport
 *********************************************************************************/
OSErr GetCurrentPortNameFromDatabase(char *longPortName, char *portName, Boolean *enabled)
{
	OSStatus err = noErr;
	CfgEntityRef activeConn;
	short curResFile = CurResFile();

	// must have a place to put our results
	if (!longPortName || !portName || !enabled) return (paramErr);
	
	// initialize if we must
	if (!gNSLValid) InitNetworkSetup();
	
	// open the database if it is not yet opened
	if (!MNSValidDatabase(&gRef)) err = MNSOpenDatabase(&gRef,false);
	if (err == noErr) 
	{
		// find the current network connection configuration
		err = FindCurrentConnection(&gRef, kOTCfgTypeTCPv4, &activeConn);
		if (err == kCfgErrDatabaseChanged)
		{
			// The database has changed.  Close and re-open it.
			err = MNSCloseDatabase(&gRef,false);
			if (err == noErr)
			{
				err = MNSOpenDatabase(&gRef,false);
				if (err == noErr)
				{
					// re-try
					err = FindCurrentConnection(&gRef, kOTCfgTypeTCPv4, &activeConn);
				}
			}
		}
		
		// see what connection method the current configuration supports
		if (err == noErr)
		{
			err = GetPortNameFromTCPEntity(&gRef, &activeConn, longPortName, portName, enabled);
		}
		
		if (IsNSBroken()) 
		{
			// To work around a flaw in NSL for OS 8.6 and below
			CloseNetworkSetup();
		}
	}
	
	UseResFile(curResFile);	// to work around bug in older (8.6 and older) NSL that mucks with CurResFile().

	return (err);
}

/*********************************************************************************
 * GetPPPDialingInformationFromDatabase - return PPP specific preferences
 *********************************************************************************/
OSErr GetPPPDialingInformationFromDatabase(Boolean *redial, unsigned long *numRedials, unsigned long *delay)
{
	OSStatus err = noErr;
	CfgEntityRef activeConn;
	short curResFile = CurResFile();
	
#ifdef	DEBUG
	ASSERT(redial);
	ASSERT(numRedials);
	ASSERT(delay);
#endif
	
	*numRedials = *delay = 0;
	*redial = false;
	
	// initialize if we must
	if (!gNSLValid) InitNetworkSetup();
	
	// open the database if it is not yet opened.
	if (!MNSValidDatabase(&gRef)) err = MNSOpenDatabase(&gRef,false);
	if (err == noErr) 
	{
		// find the current network connection configuration
		err = FindCurrentConnection(&gRef, kOTCfgTypeRemote, &activeConn);
		if (err == kCfgErrDatabaseChanged)
		{
			// The database has changed.  Close and re-open it.
			err = MNSCloseDatabase(&gRef,false);
			if (err == noErr)
			{
				err = MNSOpenDatabase(&gRef,false);
				if (err == noErr)
				{
					// re-try
					err = FindCurrentConnection(&gRef, kOTCfgTypeRemote, &activeConn);
				}
			}
		}
		
		// read the dialing information from this configuration
		if (err == noErr)
		{
			err = GetDialingInfoFromRemoteEntity(&gRef, &activeConn, redial, numRedials, delay);
		}
		
		if (IsNSBroken()) 
		{
			// To work around a flaw in NSL for OS 8.6 and below
			CloseNetworkSetup();
		}
	}
	
	UseResFile(curResFile);	// to work around bug in older (8.6 and older) NSL that mucks with CurResFile().
	
	return (err);	
}

/*********************************************************************************
 * FindCurrentConnection - find the currently active TCP/IP configuration
 *********************************************************************************/		
static OSStatus FindCurrentConnection(const MNSDatabadeRef *ref, OSType protocol, CfgEntityRef *currentEntity)
	// This routine finds the current connection entity for the specified
	// protocol in the active set, returning it in currentEntity.
{
	OSStatus err;
	CfgEntityRef activeSet;
	TypeAndClassParam param;
	
	MoreAssertQ(MNSValidDatabase(ref));
	MoreAssertQ(currentEntity != nil);
	
	param.fClass = kOTCfgClassNetworkConnection;
	param.fType = protocol;
	param.found = false;
	param.currentEntity = currentEntity;
	
	err = MNSFindActiveSet(ref, &activeSet);
	if (err == noErr) {
		err = MNSIterateSet(ref, &activeSet, TypeAndClassIterator, &param, false);
	}
	if (err == noErr) {
		if (param.found) {
			// Set preferences contain entities from weird areas because
			// of the way the database is committed.  We can safely fix
			// that up here.  I discussed with the Network Setup engineer
			// and he reassured me that this was cool -- Quinn, 9 Nov 1998
			
			currentEntity->fLoc = ref->area;
		} else {
			err = -6;
		}
	}
	
	return err;
}

/*********************************************************************************
 * GetPortNameFromTCPEntity - return the name of the crrently selected transport
 *	Also return if it's enabled or not
 *********************************************************************************/	
static OSStatus GetPortNameFromTCPEntity(const MNSDatabadeRef *ref, const CfgEntityRef *entityID, unsigned char *longPortName, char *portName, Boolean *enabled)
{	
	OSStatus err;
	ByteCount prefSize;
	Ptr buffer = nil;
	SInt16 enabledInt;

	// must have parameters to return results in
	if (!portName || !enabled) return (paramErr);
	
	// initialize return parameters
	longPortName[0] = 0;
	portName[0] = 0;
	*enabled = false;
	
	//
	// figure out if TCP/IP is enabled
	//
	
	err = MNSGetFixedSizePref(ref, entityID, 'unld', &enabledInt, sizeof(SInt16));
	if (err == noErr) 
	{
		*enabled = (enabledInt != 3);
	}
	
	//
	// Now get the full name of the connection method
	//
	
	err = MNSGetPref(ref, entityID, kOTCfgTypeRemotePort, &buffer, &prefSize);
	if (err == noErr) 
	{
		BMD(buffer, longPortName, MIN(sizeof(Str255),prefSize));
	}
	
	// Clean up.
	if (buffer != nil)  
	{
		DisposePtr(buffer);
		buffer = nil;
	}
	
	//
	// Finally, read the name of the port.
	//
	
	err = MNSGetPref(ref, entityID, kOTCfgTypeTCPiitf, &buffer, &prefSize);
	if (err == noErr) 
	{
		err = GetPortNameFromTCPPrefs(buffer, prefSize, portName);
	}
	
	// Clean up.
	if (buffer != nil)  
	{
		DisposePtr(buffer);
		buffer = nil;
	}
	
	return (err);
}

/*********************************************************************************
 * GetDialingInfoFromRemoteEntity - find the current dialing preferences
 *********************************************************************************/	
static OSStatus GetDialingInfoFromRemoteEntity(const MNSDatabadeRef *ref, const CfgEntityRef *entityID, Boolean *redial, unsigned long *numRedials, unsigned long *delay)
{	
	OSStatus err;
	ByteCount prefSize;
	Ptr buffer = nil;
	
	err = MNSGetPref(ref, entityID, kOTCfgTypeRemoteDialing, &buffer, &prefSize);
	if (err == noErr) 
	{
		if (prefSize == sizeof(OTCfgRemoteDialing)
			&& RemoteVersionAcceptable(((UInt32 *)buffer)[0])
			&& ((UInt32 *)buffer)[1] == 'dial') 
		{
			OTCfgRemoteDialing *dialingPtr;

			dialingPtr = (OTCfgRemoteDialing *)buffer;
			*redial = dialingPtr->dialMode;
			*numRedials = dialingPtr->redialTries;
			*delay = dialingPtr->redialDelay;
		}
	}
	
	// Clean up.
	if (buffer != nil) 
	{
		DisposePtr(buffer);
	}
	
	return (err);
}

/*********************************************************************************
 * RemoteVersionAcceptable - return true if we cna understand these ARA preferences
 *********************************************************************************/	
Boolean RemoteVersionAcceptable(UInt32 version)
{
	return version == kRemoteDefaultVersion || version == kRemoteAcceptedVersion;
}
#endif	//USE_NETWORK_SETUP

/*********************************************************************************
 * GetCurrentPortNameFromFile - read the port name and enabled state of the 
 *	currently selected TCP/IP transport.
 *
 * 	- Look in Preferences folder for pref/ztcp
 *	- Open the resource fork
 *	- Fetch 'ccfg' id 1, which contains a short that is the id of the selected
 *		interface.  Call this "portid".
 *	- Fetch 'port' id portid, which contains a pascal string that is the name
 *		of the selected interface.
 *	- If this is "PPP", we're talking to OT/PPP
 *	- If this is "MacSLIP" (or whatever you use), we're talking to MacSLIP
 *
 * If any of this fails, we assume we don't have OT/PPP or MacSLIP.
 *
 * The spec locating the TCP/IP preference file is cached.  This way, we can check
 * the TCP/IP preference file periodically, and make sure the setting hasn't changed.
 *
 * This works up until OS 8.6.  After that point, Network Setup Library must be 
 * invoked.
 *********************************************************************************/
OSErr GetCurrentPortNameFromFile(unsigned char *longPortName,  char *portName, Boolean *enabled)
{
	OSErr err = noErr;
	short vRef = 0;
	long dirId = 0;
	Boolean foundIt = false;
	CInfoPBRec hfi;
	Str31 name;
	short refNum = 0;
	Handle probe = 0;
	short oldRes;	
	short config;
	
	oldRes = CurResFile();
				
	if (TCPprefFileSpec.name[0] == 0)	//have we located the TCP/IP pref file yet?
	{
		//
		// Locate the TCP/IP preferences file
		//
				
		//find the active Preferences folder
		err = FindFolder(kOnSystemDisk,kPreferencesFolderType,False,&vRef,&dirId);
		if (err == noErr)
		{
			hfi.hFileInfo.ioNamePtr = name;
			hfi.hFileInfo.ioFDirIndex = 0;
			while(!foundIt && !DirIterate(vRef,dirId,&hfi))
			{
				if ((hfi.hFileInfo.ioFlFndrInfo.fdType == TCP_PREF_FILE_TYPE &&
						hfi.hFileInfo.ioFlFndrInfo.fdCreator == TCP_PREF_FILE_CREATOR))
				{
					// Found it.  Cache it.
					foundIt = true;
					FSMakeFSSpec(vRef,dirId,name,&TCPprefFileSpec);
				}
			}
		}
	}
		
	if (TCPprefFileSpec.name[0] != 0)	//have we located the TCP/IP pref file yet?
	{
		refNum = FSpOpenResFile(&TCPprefFileSpec,fsRdPerm);
		err = ResError();
		if (err == noErr && refNum >= 0)
		{	
			probe = Get1Resource(CONFIG_RESOURCE,CONFIG_RESOURCE_ID);
			err = ResError();
			if (err == noErr && probe != 0)
			{
				// this is the current configuration
				config = *(short *)(*probe);
				
				// 
				// determine if TCP/IP is enabled
				//
				
				probe = Get1Resource(LOADED_RESOURCE, config);
				err = ResError();
				if ((err == noErr) && (probe != 0))
				{
					*enabled = (**((SInt16**)probe) != 3);
				} 
			
				//
				// find the full name of the active TCP/IP transport
				//
			
				probe = Get1Resource(PORT_RESOURCE, config);
				err = ResError();
				if ((err == noErr) && (probe != 0))
				{
					//assume the 'port' resource is just a P-String
					PStrCopy(longPortName,*probe,kMaxProviderNameSize);
				}
				
				//
				// find the short name of the current port
				//
						
				probe = Get1Resource(IITF_RESOURCE, config);
				err = ResError();
				if ((err == noErr) && (probe != 0))
				{
					SignedByte state = HGetState(probe);
					HLock(probe);
					err = GetPortNameFromTCPPrefs(*probe, GetHandleSize(probe), portName);
					HSetState(probe, state);
				}
			}
		}
		CloseResFile(refNum);
	}
		
	if ((err != noErr) || (TCPprefFileSpec.name[0] == 0)) err = errPPPPrefNotFound;
	
	UseResFile(oldRes);
	
	return (err);
}

/*********************************************************************************
 * TCPWillDial - will a TCP call dial the modem?
 *
 *	This routine requires Open Transport be used.  If OT is not present, or if the
 *	user has switched off OT cooperation, return that the next TCP
 * 	call won't dial the phone.  That way, Adware will work, although it will 
 *	interrupt to dial the modem occasionally.
 *
 *	01/25/00 - jdboyd
 *	Added the forceRead parameter.  Cache the results, so we can be called without
 *	preventing the machine from falling asleep.
 *********************************************************************************/	
Boolean TCPWillDial(Boolean forceRead)
{
	static Boolean result = false;	// assume we're online
	static Boolean readOnce = false;
	OSStatus err;
	UInt32 willDial;
	
	if (PrefIsSet(PREF_IGNORE_PPP)) return false;
	
	if (!forceRead && readOnce) 	// return value from last time, if there was one?
		return (result);
	
	// determine if a TCP call will dial the modem using pref files or NS Library.
	if (HaveTheDiseaseCalledOSX())
	{
		readOnce = true;
		result = PPPDown();
	}
	else if (gUseOT)
	{
		err = NSHTCPWillDial(&willDial);
		readOnce = true;
		
		if (err == noErr) 
		{
			switch (willDial) 
			{
				case kNSHTCPDialUnknown: 		// Unknown
				case kNSHTCPDialTCPDisabled:	// TCP/IP is disabled
				case kNSHTCPDialYes:			// TCP/IP will dial
					result = true;
					break;
				
				case kNSHTCPDialNo:				// TCP/IP will not dial
					result = false;
					break;
				
				default:
					MoreAssertQ(false);
					break;
			}
		} 
		// else 
			// Failed with error.  Assume we'll dial.
	}
		
	return (result);
}

// Assume a TCP/IP won't dial if the TCP/IP stack is currently loaded.
#define kUseInetInterfaceInfo true

/*********************************************************************************
 * NSHTCPWillDial - will a TCP call dial the modem?
 *	Usurped from MoreNetworkSetup1.0b2.
 *********************************************************************************/	
static OSStatus NSHTCPWillDial(UInt32 *willDial)
{
	OSStatus err = noErr;
	InetInterfaceInfo info;
	Boolean enabled;
	unsigned char pCurrentPortName[kMaxProviderNameSize];
	char portName[kMaxProviderNameSize];
	OTPortRecord portRecord;
	
	ASSERT(!PrefIsSet(PREF_IGNORE_PPP));
	
	// must have a parameter to return result in
	MoreAssertQ(willDial != nil);
	
	*willDial = kNSHTCPDialUnknown;
	
	if ( kUseInetInterfaceInfo && OTInetGetInterfaceInfo(&info, kDefaultInetInterface) == noErr) 
	{
		// With the current way TCP/IP is organized, the stack being loaded implies hat we're already dialled in.
		*willDial = kNSHTCPDialNo;	
	} 
	else if (gActiveConnections > 0)
	{
		// if there's an active connection in Eudora already, no dialing will happen
		*willDial = kNSHTCPDialNo;
	}
	else 
	{
		// Get the current TCP info ...
		err = GetTCPInfo(&enabled, pCurrentPortName, portName);
		if (err == noErr) 
		{		
			if (OTStrEqual(portName, kDDPName)) 
			{ 
				// A special case for MacIP, because "ddp" does not have an active port if AppleTalk is disabled.
				*willDial = kNSHTCPDialNo;	
			} 
			else if (OTFindPort(&portRecord, portName)) 
			{
				// We know the port.  Look at the device type to decide whether we might dial.
				short device = OTGetDeviceTypeFromPortRef(portRecord.fRef);
				
				switch (device) 
				{
					case kOTADEVDevice:
					case kOTIRTalkDevice:
					case kOTSMDSDevice:
						// Assert: TCP shouldn't be using this link type
						MoreAssertQ(false);
						*willDial = kNSHTCPDialNo;
						break;
						
					case kOTISDNDevice:
					case kOTATMDevice:
					case kOTSerialDevice:
					case kOTModemDevice:
						// Assert: TCP shouldn't be using this link type
						MoreAssertQ(false);
						*willDial = kNSHTCPDialYes;
						break;

					case kOTLocalTalkDevice:
					case kOTTokenRingDevice:
					case kOTEthernetDevice:
					case kOTFastEthernetDevice:
					case kOTFDDIDevice:
					case kOTIrDADevice:
					case kOTATMSNAPDevice:
					case kOTFibreChannelDevice:
					case kOTFireWireDevice:
						*willDial = kNSHTCPDialNo;
						break;

					case kOTMDEVDevice:
					case kOTSLIPDevice:
					case kOTPPPDevice:
						if ((device==kOTPPPDevice) && gHasOTPPP && PPPDown())
						{
							// we're connected with PPP, and PPP is down.  Will dial.
						 	*willDial = kNSHTCPDialYes;
						}
						else
						{
							// we don't know if we're going to dial.  Assume we're not, and dial if we have to.
							*willDial = kNSHTCPDialNo;
						}
						break;

					default:
						MoreAssertQ(*willDial == kNSHTCPDialUnknown);
						break;
				}
			} 
			else 
			{
				err = -1;
			}
		}
	}
	
	return err;
}

/*********************************************************************************
 * GetTCPInfo - get the current TCP/IP connection information from NS Library or
 *	preference files.
 *********************************************************************************/	
static OSStatus GetTCPInfo(Boolean *enabled, unsigned char *portNameLong, char *portName)
{
	OSStatus err;
	
#ifdef	USE_NETWORK_SETUP
	if (UseNetworkSetup()) 
	{
		// read the information from the NS Library ...
		err = GetCurrentPortNameFromDatabase(portNameLong, portName, enabled);
	} 
	else 
#endif //USE_NETWORK_SETUP
	{
		// read the information from the settings file ourselves ...
		err = GetCurrentPortNameFromFile(portNameLong, portName, enabled);
	}
	return err;
}

/*********************************************************************************
 * UnpackTCPPrefs - unpack an interface from an 'iitf' preference into the 
 *	relavent fields of a NSHTCPv4COnfigurationDigest.
 *********************************************************************************/	
static void UnpackTCPPrefs(Ptr *buffer, NSHTCPv4ConfigurationDigest *configurationDigest)
{
	UInt8 *cursor;
	
	cursor = (UInt8 *) *buffer;
	
	configurationDigest->fConfigMethod = *cursor;
	cursor += sizeof(UInt8);
	configurationDigest->fIPAddress = *((InetHost *) cursor);
	cursor += sizeof(InetHost);
	configurationDigest->fSubnetMask = *((InetHost *) cursor);
	cursor += sizeof(InetHost);

	// fAppleTalkZone is a Str32.  A longer string in the
	// 'iitf' is a bug in the person who wrote the code and
	// causes us to stop parsing.  The caller will notice that 
	// the cursor did not advance far enough and error out.
	
	if ( *cursor <= 32 ) 
	{
		BlockMoveData(cursor, configurationDigest->fAppleTalkZone, *cursor + 1);
		cursor += (*cursor + 1);
		BlockMoveData(cursor, configurationDigest->fHintPortName, 36);
		cursor += 36;
		BlockMoveData(cursor, configurationDigest->fHintDriverName, 32);
		cursor += 32;
		configurationDigest->fFraming = *((UInt32 *) cursor);
		cursor += sizeof(UInt32);
	}

	*buffer = (Ptr) cursor;
}

/*********************************************************************************
 * GetPortNameFromTCPPrefs - extract the port name from the first interface 
 *	described in the buffer.
 *********************************************************************************/	
static OSStatus GetPortNameFromTCPPrefs(Ptr buffer, SInt32 prefSize, char *portName)
{
	OSStatus err;
	UInt16 interfaceCount;
	Ptr cursor;
	NSHTCPv4ConfigurationDigest firstInterface;
	UInt8 portNameLength;
	
	// Get the count of interfaces, checking for possibly bogus
	// preference data.
	
	err = noErr;
	if (prefSize < sizeof(UInt16)) 
	{
		err = -1;
	}
	if (err == noErr) 
	{
		interfaceCount = *((UInt16 *)buffer);
		if (interfaceCount < 1) 
		{
			err = -1;
		}
	}
	
	// Unpack the first interface out of the 'iitf'.
	
	if (err == noErr) 
	{
		cursor = buffer + sizeof(UInt16);
		UnpackTCPPrefs(&cursor, &firstInterface);

		// Assert: Did not consume correct number of bytes
		MoreAssertQ( interfaceCount > 1 || (cursor == buffer + prefSize) );
	}
	
	// Copy the port name out of the unpacked interface.
	
	if (err == noErr) 
	{
		portNameLength = firstInterface.fHintPortName[0];
		if ( portNameLength > kMaxProviderNameLength) 
		{
			err = -1;
		} 
		else 
		{

			// Poor Man's C2PString avoids me having to figure
			// out which wacky library CodeWarrior wants me to link with
			// today!
			
			BlockMoveData(firstInterface.fHintPortName + 1, portName, portNameLength);
			portName[ portNameLength ] = 0;
		}
	}

	return err;
}
