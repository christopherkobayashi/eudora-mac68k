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

#define FILE_NUM 131
/* Copyright (c) 1999 by QUALCOMM Incorporated */

#include "regcode_v2.h"
#include "regcode_charsets.h"

//      Prototypes
void MakeJumpPath(StringPtr s, short hostResID);
Handle GenerateAdwareURLStr(UserStateType state, StringPtr host,
			    short action, UInt32 query, long refcon);

extern ModalFilterUPP DlgFilterUPP;

//
//      OpenAdwareURL
//
//      
//              Host is resource
void OpenAdwareURL(UserStateType state, short hostResID, short action,
		   UInt32 query, long refcon)
{
	Str255 host;
	MakeJumpPath(host, hostResID);
	OpenAdwareURLStr(state, host, action, query, refcon);
}

//              Host is string
OSErr OpenAdwareURLStr(UserStateType state, StringPtr host, short action,
		       UInt32 query, long refcon)
{
	Handle url;
	Str255 protocol;
	short length;
	OSErr err;

	if (url = GenerateAdwareURLStr(state, host, action, query, refcon)) {
		length = GetHandleSize(url);
		if (length && (*url)[length] == 0)
			length--;
		if (!ParseProtocolFromURLPtr(LDRef(url), length, protocol))
			err = OpenOtherURLPtr(protocol, *url, length);
		ZapHandle(url);
	}
	return err;
}

//
//      GenerateAdwareURL
//
//              'state'                 -       The state to be used when generating the URL (used by registration)
//              'hostResID'     - The host stuff... www.eudora.com
//              'action'                - The action to be taken.  Pass in one of the constants from URLAction in StrnDefs
//              'query'                 - A bit mask identifying the fields to be included in the query.  Use the 'urlQueryParts' constants in adutil.h
//              'refcon'                - Paramater for special use of certain actions and query types.  This mechanism is not entirely
//                                                                      swift since multiple query parts _could_ each require different refcons.  This would break.  Badly.
//                                                                      Don't do this.
//
//              Host is resource
Handle GenerateAdwareURL(UserStateType state, short hostResID,
			 short action, UInt32 query, long refcon)
{
	Str255 host;
	MakeJumpPath(host, hostResID);
	return GenerateAdwareURLStr(state, host, action, query, refcon);
}

//              Host is string
Handle GenerateAdwareURLStr(UserStateType state, StringPtr host,
			    short action, UInt32 query, long refcon)
{
	Accumulator urlAccumulator;
	AdURLStringsPtr urlStringPtrs;
	Str255 attribute, value;
	OSErr theError;

	// Initialize the accumulator into which we'll build the URL
	theError = AccuInit(&urlAccumulator);

	// Add the URL path, normally "http://www.eudora.com/jump.cgi"
	if (!theError)
		theError = AccuAddStr(&urlAccumulator, host);

	// Gimme a '?'
	if (!theError)
		theError = AccuAddChar(&urlAccumulator, '?');

	// The query is built from the bit mask passed in 'query'.  Each bit represents a attribute/value
	// pair that will be included in the query portion of the URL.  See 'urlQueryParts' in adutil.h
	// for those parts that have been defined.  See also 'URLQueryPartStrn' for the
	// attribute names

	if (!theError && (query & actionQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryAction),
					   GetRString(value,
						      URLActionStrn +
						      action));

	if (!theError && (query & realNameQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRealname),
					   GetDominantPref(PREF_REALNAME,
							   value));

#ifdef I_HATE_THE_BOX
	if (!theError && (query & regFirstQueryPart) && !BoxUser(state))
#else
	if (!theError && (query & regFirstQueryPart))
#endif
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegFirst),
					   GetSomeHunkOfReg(state, value,
							    action,
							    GetRegFirst));

#ifdef I_HATE_THE_BOX
	if (!theError && (query & regLastQueryPart) && !BoxUser(state))
#else
	if (!theError && (query & regLastQueryPart))
#endif
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegLast),
					   GetSomeHunkOfReg(state, value,
							    action,
							    GetRegLast));

#ifdef I_HATE_THE_BOX
	if (!theError && (query & regCodeQueryPart) && BoxUser(state)
	    && action == actionRegister50box)
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegCode),
					   GetDespisedBoxRegCode(state,
								 value));
	else
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegCode),
					   GetSomeHunkOfReg(state, value,
							    action,
							    GetRegCode));
#else
	if (!theError && (query & regCodeQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegCode),
					   GetSomeHunkOfReg(state, value,
							    action,
							    GetRegCode));
#endif

	if (!theError && (query & regLevelQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryRegLevel),
					   GetSomeHunkOfReg(state, value,
							    action,
							    GetRegLevel));

	if (!theError && (query & oldRegQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryOldReg),
					   GetOldRegCode(state, value));

	if (!theError && (query & emailQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryEmail),
					   GetShortReturnAddr(value));

	if (!theError && (query & profileQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryProfile),
					   GetProfileID(value));

	if (!theError && (query & destinationQueryPart))
		if (urlStringPtrs = (AdURLStringsPtr) refcon)
			if (urlStringPtrs->destination)
				theError =
				    AccuAttributeValuePair(&urlAccumulator,
							   GetRString
							   (attribute,
							    URLQueryPartStrn
							    +
							    queryDestination),
							   urlStringPtrs->
							   destination);

	if (!theError && (query & adIDQueryPart))
		if (urlStringPtrs = (AdURLStringsPtr) refcon)
			if (urlStringPtrs->adID)
				theError =
				    AccuAttributeValuePair(&urlAccumulator,
							   GetRString
							   (attribute,
							    URLQueryPartStrn
							    + queryAdID),
							   urlStringPtrs->
							   adID);

	if (!theError && (query & platformQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryPlatform),
					   GetRString(value,
						      REG_PLATFORM));

	if (!theError && (query & productQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryProduct),
					   GetRString(value, REG_PRODUCT));

	if (!theError && (query & versionQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryVersion),
					   GetVersionString(value));

	if (!theError && (query & distributorIDQueryPart))
		theError =
		    AccuAttributeValuePairHandle(&urlAccumulator,
						 GetRString(attribute,
							    URLQueryPartStrn
							    +
							    queryDistributorID),
						 GetDistributorID());

	if (!theError && (query & modeQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryMode),
					   GetModeString(value));

	if (!theError && (query & topicQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryTopic),
					   GetRString(value,
						      URLSupportTopicStrn +
						      refcon));

	if (!theError && (query & langQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryLang),
					   GetRString(value,
						      LOCALIZED_VERSION_LANG));

	if (!theError && (query & queryQueryPart))
		theError =
		    AccuAttributeValuePair(&urlAccumulator,
					   GetRString(attribute,
						      URLQueryPartStrn +
						      queryQuery),
					   (PStr) refcon);

	if (!theError) {
		// If any query part was present, strip the '& off the end of the accumulator
		if (query & fullQuery)
			AccuStrip(&urlAccumulator, 1);
		// Tack a null at the end of the handle since DownloadURL expects a C string
		theError = AccuAddChar(&urlAccumulator, 0);
	}
	if (!theError)
		AccuTrim(&urlAccumulator);
	else
		AccuZap(urlAccumulator);
	return (urlAccumulator.data);
}

//      MakeJumpPath - make path for jump URL
void MakeJumpPath(StringPtr s, short hostResID)
{
	// Get host
	GetRString(s, hostResID);

	// Slide in a slash '/'
	PCatC(s, '/');

	// Tack on the command portion  "jump.cgi"
	PCatR(s, URL_JUMP_COMMAND);
}


OSErr AccuAttributeValuePair(AccuPtr a, PStr attribute, PStr value)
{
	OSErr theError;
	Str255 localValue;

	PCopy(localValue, value);
	TransLitRes(localValue + 1, *localValue, ktFlatten);
	TransLitRes(localValue + 1, *localValue, ktMacISO);
	URLQueryEscape(localValue);

	theError = noErr;
	if (attribute[0] && value[0]) {
		theError = AccuAddStr(a, attribute);
		if (!theError)
			theError = AccuAddChar(a, '=');
		if (!theError && value[0])
			theError = AccuAddStr(a, localValue);
		if (!theError)
			theError = AccuAddChar(a, '&');
	}
	return (theError);
}


OSErr AccuAttributeValuePairHandle(AccuPtr a, PStr attribute, Handle value)
{
	Str255 scratch;
	OSErr theError;
	UPtr from, end;

	theError = noErr;
	if (attribute[0] && value) {
		theError = AccuAddStr(a, attribute);
		if (!theError)
			theError = AccuAddChar(a, '=');
		if (!theError && value) {
// Old unescaped way...
//                      theError = AccuAddHandle (a, value);
			end = *value + GetHandleSize(value);
			for (from = *value; !theError && from < end;
			     from++)
				switch (*from) {
				case '%':
				case '&':
				case '?':
				case ':':
				case ' ':
					theError = AccuAddChar(a, '%');
					if (!theError)
						theError =
						    AccuAddPtr(a,
							       Bytes2Hex
							       (from, 1,
								scratch),
							       1);
					break;
				default:
					if (*from & 0x80) {
						theError =
						    AccuAddChar(a, '%');
						if (!theError)
							theError =
							    AccuAddPtr(a,
								       Bytes2Hex
								       (from,
									1,
									scratch),
								       1);
					} else
						theError =
						    AccuAddChar(a, *from);
					break;
				}
		}
		if (!theError)
			theError = AccuAddChar(a, '&');
	}
	return (theError);
}


//
//      ChooseInitialUserState
//
//              Choose the initial state for a user very, very carefully...  We need to be
//              able to recognize the difference between a new user, an old user and a box
//              user.
//

UserStateType ChooseInitialUserState(UserStateType state,
				     Boolean foundRegFile,
				     Boolean needsRegistration,
				     int pnPolicyCode,
				     InitNagResultType * result)
{
	int regMonth;
	Boolean regValid;

	*result = noDialogPending;

	// If we've found a more recent RegCode file, set the state to that indicated by the policy code
	if (foundRegFile)
		state = PolicyCodeToRegisteredNagState(pnPolicyCode);

	// Don't trust anything or anyone... Now that we think we have a state, make darn sure
	if (!ValidUser(state)) {
		ComposeStdAlert(Note, STATE_INVALID_AT_STARTUP);
		*result = paymentRegPending;
		state = adwareUser;
	}
	// Do we suddenly have an invalid registration code?
	regValid = ValidRegCode(state, &pnPolicyCode, &regMonth);
	if (RegisteredUser(state) && !regValid) {
		if (PayUser(state) && foundRegFile) {
			ComposeStdAlert(Note, REG_INVALID_AT_STARTUP);
			*result = codeEntryPending;
		}
		state = PolicyCodeToUnregisteredNagState(pnPolicyCode);
	}
#ifdef I_HATE_THE_BOX
	// If the user has a valid regcode and is in Paid mode -- check the policy code
	// to verify that this is actually a regcode that applies to a box build
	if (regValid && PayUser(state)
	    && !PolicyCheck(pnPolicyCode, regMonth))
		state = boxUser;
#else
	// If the user had previously been changed to a repay user, we make them sponsored here because they have relaunched
	if (RepayUser(state)) {
		*result = gracelessRepayPending;
		return (state =
			RegisteredUser(paidUser) ? regAdwareUser :
			adwareUser);
	}
	// If the registration code is valid, but the user is not entitled to this update, give them the Repay dialog
	if (IsPayUser() && *result == noDialogPending && regValid
	    && !PolicyCheck(pnPolicyCode, regMonth))
		*result = repayPending;
#endif

	return (state);
}

Boolean PolicyCheck(int pnPolicyCode, int regMonth)
{
	Boolean entitledToFreeUpgrade;

	entitledToFreeUpgrade = false;
	switch (pnPolicyCode) {
	case REG_EUD_AD_WARE:
		break;
	case REG_EUD_LIGHT:
		break;
	case REG_EUD_PAID:
#ifndef I_HATE_THE_BOX
		// Never honor REG_EUD_PAID when using the box.  The user is expected to
		// re-register and recieve back a policy code of REG_EUD_50_PAID_BOX_ESD (or higher)
		// when using the 5.0 box.
		entitledToFreeUpgrade =
		    (regMonth < REG_EUD_CLIENT_34_DEFUNCT_MONTH)
		    && (regMonth + 12 >= CLIENT_BUILD_MONTH);
#endif
		break;
	case REG_EUD_50_PAID_TRIMODE:
	case REG_EUD_50_PAID_BOX_ESD:
	case REG_EUD_50_PAID_37_RSRV:
	case REG_EUD_50_PAID_38_RSRV:
	case REG_EUD_50_PAID_39_RSRV:
	case REG_EUD_50_PAID_40_RSRV:
	case REG_EUD_50_PAID_EN_ONLY:
	case REG_EUD_50_PAID_EN_NOT_X1:
#ifdef DONT_CHECK_REGMONTH
		return true;
#endif
		entitledToFreeUpgrade =
		    (regMonth + 12 >= CLIENT_BUILD_MONTH);
		break;
	}

	return (entitledToFreeUpgrade);
}

//
//      CheckForRegCodeFile
//
//              Checks to see if there is a RegCode file (with valid data, hopefully), returning
//              true or false.  We pass in the date of the last known use of the RegCode file and
//              reparse the file if the current RegCode file is newer than the old one.
//

Boolean CheckForRegCodeFile(Boolean * needsRegistration, uLong * regDate,
			    int *pnPolicyCode)
{
	FSSpec spec;
	Str31 folderName;
	uLong curRegDate;

	*pnPolicyCode = regInvalidPolicyCode;
	if (!GetFileByRef(AppResFile, &spec))
		if (!FSMakeFSSpec
		    (spec.vRefNum, spec.parID,
		     GetRString(folderName, STUFF_FOLDER), &spec)) {
			IsAlias(&spec, &spec);
			spec.parID = SpecDirId(&spec);
			GetRString(spec.name, REG_CODE_FILE);
			if ((curRegDate = FSpModDate(&spec)) > *regDate)
				if (!ParseRegFile
				    (&spec, parseSilent, needsRegistration,
				     pnPolicyCode)) {
					*regDate = curRegDate;
					return (true);
				}
		}
	return (false);
}


UserStateType PolicyCodeToRegisteredNagState(int policyCode)
{
	switch (policyCode) {
	case REG_EUD_PAID:
	case REG_EUD_50_PAID_TRIMODE:
	case REG_EUD_50_PAID_BOX_ESD:
	case REG_EUD_50_PAID_37_RSRV:
	case REG_EUD_50_PAID_38_RSRV:
	case REG_EUD_50_PAID_39_RSRV:
	case REG_EUD_50_PAID_40_RSRV:
	case REG_EUD_50_PAID_EN_ONLY:
	case REG_EUD_50_PAID_EN_NOT_X1:
		return (paidUser);
		break;
	case REG_EUD_LIGHT:
		return (regFreeUser);
		break;
	case REG_EUD_AD_WARE:
		return (regAdwareUser);
		break;
	}
	return (regAdwareUser);
}


UserStateType PolicyCodeToUnregisteredNagState(int policyCode)
{
	switch (policyCode) {
	case REG_EUD_PAID:
	case REG_EUD_50_PAID_TRIMODE:
	case REG_EUD_50_PAID_BOX_ESD:
	case REG_EUD_50_PAID_37_RSRV:
	case REG_EUD_50_PAID_38_RSRV:
	case REG_EUD_50_PAID_39_RSRV:
	case REG_EUD_50_PAID_40_RSRV:
	case REG_EUD_50_PAID_EN_ONLY:
	case REG_EUD_50_PAID_EN_NOT_X1:
		return (adwareUser);
		break;
	case REG_EUD_LIGHT:
		return (freeUser);
		break;
	case REG_EUD_AD_WARE:
		return (adwareUser);
		break;
	}
	return (adwareUser);
}

PStr GetDominantPref(short prefNumber, PStr string)
{
	PushPers(PersList);
	GetPref(string, prefNumber);
	PopPers();
	return (string);
}


PStr GetSomeHunkOfReg(UserStateType state, PStr string, short action,
		      GetSomeHunkOfRegProcPtr getRegProc)
{
	if (action == actionUpdate || action == actionArchived
	    || action == actionPay) {
		(*getRegProc) (paidUser, string);
		if (!*string && state != paidUser)
			(*getRegProc) (state, string);
	} else
		(*getRegProc) (state, string);
	return (string);
}


PStr GetRegFirst(UserStateType state, PStr string)
{
	Handle regInfo;
	short resID;

	resID =
	    PayMode(state) ? REG_PREF_PAID : (AdwareMode(state) ?
					      REG_PREF_SPONSORED :
					      REG_PREF_LIGHT);
	regInfo = GetResourceFromFile(REG_PREF_TYPE, resID, SettingsRefN);
	if (regInfo == nil || GetHandleSize(regInfo) < 3)
		string[0] = 0;
	else
		CtoPPtrCpy(string, *regInfo);
	if (regInfo)
		ReleaseResource(regInfo);
	return (string);
}



PStr GetRegLast(UserStateType state, PStr string)
{
	Handle regInfo;
	short resID;

	resID =
	    PayMode(state) ? REG_PREF_PAID : (AdwareMode(state) ?
					      REG_PREF_SPONSORED :
					      REG_PREF_LIGHT);
	regInfo = GetResourceFromFile(REG_PREF_TYPE, resID, SettingsRefN);
	if (regInfo == nil || GetHandleSize(regInfo) < 3)
		string[0] = 0;
	else {
		Ptr c = *regInfo;

		c += strlen(c) + 1;
		CtoPPtrCpy(string, c);
	}
	if (regInfo)
		ReleaseResource(regInfo);
	return (string);
}


PStr GetRegCode(UserStateType state, PStr string)
{
	Handle regInfo;
	short resID;

	resID =
	    PayMode(state) ? REG_PREF_PAID : (AdwareMode(state) ?
					      REG_PREF_SPONSORED :
					      REG_PREF_LIGHT);
	regInfo = GetResourceFromFile(REG_PREF_TYPE, resID, SettingsRefN);
	if (regInfo == nil || GetHandleSize(regInfo) < 3)
		string[0] = 0;
	else {
		Ptr c = *regInfo;

		c += strlen(c) + 1;
		c += strlen(c) + 1;
		CtoPPtrCpy(string, c);
	}
	if (regInfo)
		ReleaseResource(regInfo);
	return (string);
}


#ifdef I_HATE_THE_BOX
//
//      GetDespisedBoxRegCode
//
//              Generate a registration code for box users, using:
//
//                      PolicyCode = 41
//                                       Month = current minute
//                                        Name = Ihate Thebox
//
PStr GetDespisedBoxRegCode(UserStateType state, PStr string)
{
	DateTimeRec dateTime;
	Str255 szName, szResult;

	GetRString(szName, THE_DAVE_AND_CHUCK_LOVE_CONNECTION);
	szName[++szName[0]] = 0;

	SecondsToDate(GMTDateTime(), &dateTime);

	*string = 0;
	if (!regcode_v2_encode
	    (&szName[1], dateTime.minute, REG_EUD_50_TEMP_BOX_ESD,
	     Random(), szResult))
		CtoPPtrCpy(string, szResult);
	return (string);
}
#endif


PStr GetRegLevel(UserStateType state, PStr string)
{
	Str255 regName, regCode;
	int policyCode, regMonth;

	// grab name
	GetRegFirst(state, regName);
	GetRegLast(state, regCode);
	// reformat
	PSCatC(regName, ' ');
	PSCat(regName, regCode);

	// grab code
	GetRegCode(state, regCode);

	*regName = MIN(*regName, 254);
	*regCode = MIN(*regCode, 254);
	regName[*regName + 1] = 0;
	regCode[*regCode + 1] = 0;

	// Grab month by verifying code
	if (RegCodeVerifies
	    (regCode + 1, regName + 1, &policyCode, &regMonth))
		NumToString(19 *
			    (((((Random() & 0xff) << 8) | policyCode) << 8)
			     | regMonth), string);
	else
#ifdef I_HATE_THE_BOX
	{
		DateTimeRec dateTime;
		SecondsToDate(GMTDateTime(), &dateTime);
		NumToString(19 *
			    (((((Random() & 0xff) << 8) |
			       REG_EUD_50_TEMP_BOX_ESD) << 8) | dateTime.
			     minute), string);
	}
#else
		*string = 0;
#endif
	return string;
}



PStr GetOldRegCode(UserStateType state, PStr string)
{
	return (GetPref(string, PREF_SITE));
}


PStr GetVersionString(PStr string)
{
	VersRecHndl versInfo;

	string[0] = 0;
	if (versInfo =
	    (VersRecHndl) GetResourceFromFile('vers', 1, AppResFile)) {
		ComposeRString(string, REG_VERSION_FORMAT,
			       (*versInfo)->numericVersion.majorRev,
			       ((*versInfo)->numericVersion.
				minorAndBugRev & 0xF0) >> 4,
			       (*versInfo)->numericVersion.
			       minorAndBugRev & 0x0F,
			       (*versInfo)->numericVersion.nonRelRev);
		ReleaseResource(versInfo);
	}
	return (string);
}


PStr GetModeString(PStr string)
{
	return (GetRString
		(string,
		 URLModeStrn +
		 (IsPayMode()? modePaid
		  : (IsAdwareMode()? modeAd : modeFree))));
}


Handle GetDistributorID(void)
{
	return (GetTextFromAppFolderFile(STUFF_FOLDER, DIST_ID));
}


PStr GetProfileID(PStr string)
{
	Handle profileID;

	string[0] = 0;
	if (profileID = GetProfileData()) {
		string[0] = MIN(255, GetHandleSize(profileID));
		BMD(*profileID, &string[1], string[0]);
		ReleaseResource(profileID);
	}
	return (string);
}


Handle GetProfileData(void)
{
	Handle theData =
	    GetResourceFromFile(USER_PROFILE_TYPE, USER_PROFILE_ID,
				SettingsRefN);

	if (!theData) {
		TOCType inTOC, outTOC;
		FSSpec spec;
		Str63 s;

		// Try to fetch from In/Out toc's
		if (!FSMakeFSSpec
		    (MailRoot.vRef, MailRoot.dirId, GetRString(s, IN),
		     &spec))
			if (!PeekTOC(&spec, &inTOC))
				if (!FSMakeFSSpec
				    (MailRoot.vRef, MailRoot.dirId,
				     GetRString(s, OUT), &spec))
					if (!PeekTOC(&spec, &outTOC))
						if (outTOC.profile[0]
						    || outTOC.profile[1]
						    || inTOC.profile[0]
						    || inTOC.profile[1]) {
							// Got something.  Format & save
							ComposeString(s,
								      "\p%8x%8x%8x%8x",
								      inTOC.
								      profile
								      [0],
								      inTOC.
								      profile
								      [1],
								      outTOC.
								      profile
								      [0],
								      outTOC.
								      profile
								      [1]);
							SettingsPtr
							    (USER_PROFILE_TYPE,
							     nil,
							     USER_PROFILE_ID,
							     s + 1, *s);
							// refetch
							theData =
							    GetResourceFromFile
							    (USER_PROFILE_TYPE,
							     USER_PROFILE_ID,
							     SettingsRefN);
						}
	}
	return theData;
}



OSErr SetProfileID(PStr profile)
{
	TOCHandle inTOCH, outTOCH;
	long profileAsLongs[4];
	OSErr err = noErr;

	if (*profile)
		err =
		    SettingsPtr(USER_PROFILE_TYPE, nil, USER_PROFILE_ID,
				profile + 1, *profile);
	else
		ZapResource(USER_PROFILE_TYPE, USER_PROFILE_ID);

	ASSERT(*profile == 0 || *profile == 32);
	if (*profile)
		Hex2Bytes(profile + 1, 32, (Ptr) profileAsLongs);
	else
		WriteZero(profileAsLongs, 16);

	if (!err) {
		err = memFullErr;
		if (inTOCH = GetSpecialTOC(IN))
			if (outTOCH = GetSpecialTOC(OUT)) {
				(*inTOCH)->profile[0] = profileAsLongs[0];
				(*inTOCH)->profile[1] = profileAsLongs[1];
				(*outTOCH)->profile[0] = profileAsLongs[2];
				(*outTOCH)->profile[1] = profileAsLongs[3];
				(*inTOCH)->reallyDirty = true;
				(*outTOCH)->reallyDirty = true;
				err = noErr;
			}
	}
	return err;
}

Handle GetTextFromEudoraFolderFile(short fileResID)
{
	FSSpec spec;
	Str255 fileName;
	Handle text;
	OSErr theError;

	text = nil;
	theError =
	    FSMakeFSSpec(Root.vRef, Root.dirId,
			 GetRString(fileName, fileResID), &spec);
	if (!theError) {
		IsAlias(&spec, &spec);
		theError = Snarf(&spec, &text, 0);
	}
	if (theError)
		ZapHandle(text);
	return (text);
}


Handle GetTextFromAppFolderFile(short folderResID, short fileResID)
{
	FSSpec spec;
	Str255 folderName;
	Handle text;
	OSErr theError;

	text = nil;
	theError = GetFileByRef(AppResFile, &spec);
	if (!theError)
		theError =
		    FSMakeFSSpec(spec.vRefNum, spec.parID,
				 GetRString(folderName, folderResID),
				 &spec);
	if (!theError) {
		IsAlias(&spec, &spec);
		spec.parID = SpecDirId(&spec);
		GetRString(spec.name, fileResID);
		theError = Snarf(&spec, &text, 0);
	}
	if (theError)
		ZapHandle(text);
	return (text);
}


//
//      GetResourceFromFile
//
//              Get a resource, but make sure we're doing so only from a specifc
//              file.  We do this for the registration resources because we want
//              to make sure that certain information comes from the app and
//              certain information comes from the settings file -- without
//              interference from resource plug-ins
//

Handle GetResourceFromFile(ResType theType, short theID, short refNum)
{
	Handle resHandle;
	OSErr theError;
	short oldResFile;

	oldResFile = CurResFile();
	UseResFile(refNum);
	resHandle = Get1Resource(theType, theID);
	theError = ResError();
	UseResFile(oldResFile);
	if (theError == resNotFound)	// Uh oh... a resource isn't where it was supposed to be.  :(
		;
	return (resHandle);
}


//
//      ValidRegCode
//

Boolean ValidRegCode(UserStateType state, int *policyCode, int *regMonth)
{
	Str255 regCode, regName;
	short len;

	GetRegCode(state, regCode);
	*regCode = MIN(*regCode, 254);
	regCode[regCode[0] + 1] = 0;

	GetRegFirst(state, regName);
	GetRegLast(state, &regName[regName[0] + 1]);
	len = regName[regName[0] + 1];
	regName[regName[0] + 1] = ' ';
	regName[0] += (len + 1);
	*regName = MIN(*regName, 254);
	regName[regName[0] + 1] = 0;
	return RegCodeVerifies(&regCode[1], &regName[1], policyCode,
			       regMonth);
}

Boolean RegCodeVerifies(const char *szCode, const char *szName,
			int *outProduct, int *outMonth)
{
	int pnMonth;
	int pnProduct;
	Boolean verified = true;
//char          mapName[REG_CODE_MAX_NAME];
// So... University of California, Berkeley is one pretty darn long name...  Reg codes have been generated
// on the server and from the installer without imposing the 20 character limit originally intended for these
// things.  When validating, we're overflowing the buffer and crunching the stack when a really long name
// is input.  Truncation won't help since we'd be validating a different key. 
	Str255 mapName;

	// So, some idiot thinks that the Code Entry dialog is a "security risk" because he's able to type 255 characters
	// into the first name field and crash the reg code verifier.  He further believes that this is a "risk" because
	// Eudora has the capability of automatically processing registration information -- never mind that the only
	// regcodes we'll be automatically processing are those we've generated ourselves...
	//
	// We'll combat such stupidity by not even calling the verifier and making sure that the policy code fails.
	if (strlen(szCode) > REG_CODE_LEN - 1
	    || strlen(szName) > sizeof(mapName) - 1) {
		if (outProduct)
			*outProduct = regInvalidPolicyCode;
		return (false);
	}

	regcode_map_macroman(szName, mapName);
	verified =
	    !regcode_v2_verify(szCode, mapName, &pnMonth, &pnProduct);
	if (outProduct) {
		*outProduct = verified ? pnProduct : regInvalidPolicyCode;
	}
	if (outMonth) {
		*outMonth = verified ? pnMonth : 0;
	}
	return verified;
}
