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

#ifndef NETWORKSETUPLIBRARY_H
#define NETWORKSETUPLIBRARY_H
/* Copyright (c) 1999 by Qualcomm, Inc. */
/************************************************************************
 * Functions to use Apple's Network Setup Library included with Mac OS
 * 8.5 and beyond.  The library is used to determine the TCP/IP connection
 * method, as well as read in ARA PPP dialing preferences.
 ************************************************************************/

#if TARGET_CPU_PPC
//
// define USE_NETWORK_SETUP to compile in Network Setup Library support.
//
// The Network Setup Library is included with MacOS 8.5 and beyond.  Since
// 8.5 requires a PPC processor, there's no sense in including this in the
// 68K version
//
// Further, Network Setup is not carbonized.  There's no need to include it
// in the Carbon build of Eudora, either.
//

#define USE_NETWORK_SETUP
#endif //TARGET_CPU_PPC


#define CONFIG_RESOURCE 	'ccfg'		// indicates which configuration is currently selected
#define CONFIG_RESOURCE_ID	1
#define LOADED_RESOURCE 	'unld'		// == 3 when TCP/IP is not loaded
#define PORT_RESOURCE		'port'		// resource where the individual configs are stored
#define IITF_RESOURCE		'iitf'		// current TCP info resouce
#define	PPP_NAME			"\pPPP"		// 'port' resource contains this when PPP is selected
#define TCP_PREF_FILE_TYPE 'pref'
#define TCP_PREF_FILE_CREATOR 'ztcp'
#define PPP_PREF_FILE_TYPE 'lzcn'
#define PPP_PREF_FILE_CREATOR 'rmot'
#define DIAL_RESOURCE 'cdia'
#define DIAL_RESOURCE_ID 128
#define NS_LIBRARY_TYPE 'shlb'
#define NS_LIBRARY_CREATOR 'ntex'

struct TypeAndClassParam {
	OSType  fType;
	OSType  fClass;
	Boolean found;
	CfgEntityRef *currentEntity;
};
typedef struct TypeAndClassParam TypeAndClassParam;

struct TCPiitfPref {
	UInt8    fActive;
	InetHost fIPAddress;
	InetHost fSubnetMask;
	Str255   fAppleTalkZone;
	UInt8    fPath[36];			// Pascal string
	UInt8    fModuleName[31];	// Pascal string
	UInt32   fFramingFlags;
};
typedef struct TCPiitfPref TCPiitfPref;

typedef UInt8 TCPv4ConfigMethod;
enum {
	kTCPv4ManualConfig = 0,
	kTCPv4RARPConfig = 1,
	kTCPv4BOOTPConfig = 2,
	kTCPv4DHCPConfig = 3,
	kTCPv4MacIPConfig = 4
};

typedef UInt16 TCPv4UnloadAttr;
enum {
	kTCPv4ActiveLoadedOnDemand = 1,
	kTCPv4ActiveAlwaysLoaded = 2,
	kTCPv4Inactive = 3
};

struct NSHTCPv4ConfigurationDigest {
	OSType				fProtocol;			// always kOTTCPv4NetworkConnection for NSHTCPv4ConfigurationDigest
	Str255 				fConfigName;		// user-visible name of the configuration
	OTPortRef			fPortRef;			// OT port for the config (lookup �ddp1� for MacIP)
											// "Connect via" in the UI
	TCPv4ConfigMethod	fConfigMethod;		// one of the constants given above
											// "Configure:" in the UI
	InetHost			fIPAddress;			// IP address
	InetHost			fSubnetMask;		// IP subnet mask
	Handle				fRouterList;		// array of InetHost
	Handle				fDNSServerList;		// list of DNS servers, array of InetHost
	Str255				fLocalDomain;		// the local domain for this machine
											// "Implicit Search Path: Starting domain name:" in the UI
	Str255				fAdminDomain;		// "Implicit Search Path: Ending domain name:" in the UI
	Handle				fSearchDomains;		// STR# format
	Str32				fAppleTalkZone;		// for MacIP only, specify empty string otherwise
	UInt32				fFraming;			// framing attributes for this port
											// "Use 802.3" in the UI
											// Only for port with device type kOTEthernetDevcie
											// - use kOTFramingEthernet by default
											// - use kOTFraming8022 if the 802.3 checkbox is set
	TCPv4UnloadAttr		fUnloadAttr;		// one of the constants given above
	Str255				fHintUserVisiblePortName;	// Hints to find the port name if fPortRef is kOTInvalidPortRef
	Str63				fHintPortName;
	Str63				fHintDriverName;
	UInt16				fHintDeviceType;
};
typedef struct NSHTCPv4ConfigurationDigest NSHTCPv4ConfigurationDigest;

// Many Remote Access preferences have a version number field,
// and different versions of Remote Access set that field to
// different values, even though the preference format hasn't
// changed.  The following are the two most common values.

enum {
	kRemoteDefaultVersion  = 0x00020003,
	kRemoteAcceptedVersion = 0x00010000
};

//
// General
//
Boolean UseNetworkSetup(void);
void InitNetworkSetup(void);
OSErr CloseNetworkSetup(void);

//
// OT/PPP and MacSLIP cooperation
//

OSErr GetConnectionModeFromDatabase(unsigned long *connectionSelection);
OSErr GetPPPDialingInformationFromDatabase(Boolean *redial, unsigned long *numRedials, unsigned long *delay);
OSErr GetCurrentPortNameFromFile(unsigned char *longPortName,  char *portName, Boolean *enabled);

//
// TCP/IP Will Dial
//

Boolean TCPWillDial(Boolean forceRead);


#endif
