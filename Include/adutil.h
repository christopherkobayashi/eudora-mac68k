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

#ifndef ADUTIL_H
#define ADUTIL_H

#include <nag.h>

/* Copyright (c) 1999 by QUALCOMM Incorporated */

#define	regInvalidPolicyCode	0

typedef enum {
	realNameQueryPart				= 0x00000001,
	regFirstQueryPart				= 0x00000002,
	regLastQueryPart				= 0x00000004,
	regCodeQueryPart				= 0x00000008,
	oldRegQueryPart					= 0x00000010,
	emailQueryPart					= 0x00000020,
	profileQueryPart				= 0x00000040,
	destinationQueryPart		= 0x00000080,
	adIDQueryPart						= 0x00000100,
	platformQueryPart				= 0x00000200,
	productQueryPart				= 0x00000400,
	versionQueryPart				= 0x00000800,
	distributorIDQueryPart	= 0x00001000,
	actionQueryPart					= 0x00002000,
	topicQueryPart					= 0x00004000,
	modeQueryPart						= 0x00008000,
	regLevelQueryPart				= 0x00010000,
	langQueryPart						= 0x00020000,
	queryQueryPart					= 0x00040000
} urlQueryParts;

#define	standardQueryPart					(langQueryPart | platformQueryPart | productQueryPart | versionQueryPart | distributorIDQueryPart)
#define	registrationQueryPart			(langQueryPart | regFirstQueryPart | regLastQueryPart | regCodeQueryPart)
#define	identityQueryPart					(langQueryPart | realNameQueryPart | emailQueryPart)
#define	adQueryPart								(langQueryPart | destinationQueryPart | adIDQueryPart)
#define	standardRegIdentityQuery	(langQueryPart | standardQueryPart | identityQueryPart | registrationQueryPart)

#define	fullQuery									(realNameQueryPart			| regFirstQueryPart	| regLastQueryPart	| regCodeQueryPart			|		\
																	 oldRegQueryPart				| emailQueryPart		| profileQueryPart	| destinationQueryPart	|		\
																	 adIDQueryPart					| platformQueryPart	| productQueryPart	| versionQueryPart			|		\
																	 distributorIDQueryPart	| actionQueryPart		| topicQueryPart		| modeQueryPart					|		\
																	 langQueryPart)
																	 
#define	paymentQuery							(actionQueryPart | standardRegIdentityQuery | modeQueryPart | oldRegQueryPart)
#define	junkDownQuery							helpQuery
#define	registrationQuery					(actionQueryPart | standardRegIdentityQuery | modeQueryPart)
#define	lostCodeQuery							(actionQueryPart | standardRegIdentityQuery | modeQueryPart | oldRegQueryPart)
#define	updateQuery								(actionQueryPart | standardQueryPart | regLevelQueryPart | modeQueryPart )
#define	archivedQuery							(actionQueryPart | standardQueryPart | regLevelQueryPart | modeQueryPart)
#define	profileQuery							(actionQueryPart | standardQueryPart | identityQueryPart | modeQueryPart | profileQueryPart)
#define	clickThruQuery						(standardQueryPart | profileQueryPart | adQueryPart)
#define	helpQuery									(actionQueryPart | standardQueryPart | topicQueryPart)
#define	supportQuery							(actionQueryPart | standardRegIdentityQuery | modeQueryPart | oldRegQueryPart)
#define siteQuery									(supportQuery)
#define	introQuery								(langQueryPart | actionQueryPart)
#define	payRegQuery								(langQueryPart | actionQueryPart)
#define	profileidFAQQuery					(actionQueryPart | standardQueryPart)
#define	profileidReqdQuery				(langQueryPart | profileQuery)
#define searchQuery								(actionQueryPart | standardQueryPart | queryQueryPart)

typedef struct {
	PStr	destination;
	PStr	adID;
} AdURLStringsRec, *AdURLStringsPtr, **AdURLStringsHandle;

typedef	PStr	(*GetSomeHunkOfRegProcPtr) (UserStateType state, PStr string);

void								OpenAdwareURL (UserStateType state, short hostResID, short action, UInt32 query, long refcon);
OSErr 							OpenAdwareURLStr (UserStateType state, StringPtr host, short action, UInt32 query, long refcon);
Handle							GenerateAdwareURL (UserStateType state, short hostResID, short action, UInt32 query, long refcon);
OSErr								AccuAttributeValuePair (AccuPtr a, PStr attribute, PStr value);
OSErr								AccuAttributeValuePairHandle (AccuPtr a, PStr attribute, Handle value);
UserStateType				ChooseInitialUserState (UserStateType state, Boolean foundRegFile, Boolean needsRegistration, int pnPolicyCode, InitNagResultType *result);
Boolean							PolicyCheck (int pnPolicyCode, int regMonth);
Boolean							CheckForRegCodeFile (Boolean *needsRegistration, uLong *regDate, int *pnPolicyCode);
UserStateType				PolicyCodeToRegisteredNagState (int policyCode);
UserStateType				PolicyCodeToUnregisteredNagState (int policyCode);
PStr								GetDominantPref (short prefNumber, PStr string);
PStr								GetSomeHunkOfReg (UserStateType state, PStr string, short action, GetSomeHunkOfRegProcPtr getRegProc);
PStr								GetRegFirst (UserStateType state, PStr string);
PStr								GetRegLast (UserStateType state, PStr string);
PStr								GetRegCode (UserStateType state, PStr string);
PStr								GetDespisedBoxRegCode (UserStateType state, PStr string);
PStr								GetRegLevel (UserStateType state, PStr string);
PStr								GetOldRegCode (UserStateType state, PStr string);
PStr								GetVersionString (PStr string);
PStr								GetModeString (PStr string);
Handle							GetDistributorID (void);
PStr								GetProfileID (PStr string);
OSErr								SetProfileID(PStr profile);
Handle							GetProfileData (void);
Handle							GetTextFromEudoraFolderFile (short fileResID);
Handle							GetTextFromAppFolderFile (short folderResID, short fileResID);
Handle							GetResourceFromFile (ResType theType, short theID, short refNum);
Boolean							ValidRegCode (UserStateType state, int *policyCode, int *regMonth);
Boolean							RegCodeVerifies(const char *szCode, const char *szName, int *outProduct, int *outMonth);

#endif
