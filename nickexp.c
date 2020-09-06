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

#include "nickexp.h"
#define FILE_NUM 6
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * expansion of nicknames
 **********************************************************************/

#pragma segment NickExp

#pragma mark ========== Declarations ==========


#pragma mark constants & defines

#define NICK_TABLE_ALLOC_BLOCK_COUNT 20 /* number of NicknameTableEntry elements to add when we grow a NicknameTable */

#undef NE_PARAM_VALIDATION /* DEBUG *//* if defined, some extra stuff gets validated for debugging purposes (requires that MacsBug be installed) *//* FINISH *//* REMOVE */
#undef NTA_PREVENT_EXACT_MATCH /* if defined, nickname type-ahead won't complete to nicknames matching the prefix exactly */


#pragma mark globals

//
//	These are global representations of the current Preference settings
//

Boolean gNicknameTypeAheadEnabled = false;			// true if nickname type-ahead is turned on
Boolean gNicknameWatcherImmediate = false;			// true if nickname type-ahead should expand the nickname on keyDown, as opposed to during idle time
Boolean gNicknameWatcherWaitKeyThresh = false; 	// true if nickname type-ahead should wait for KeyThresh ticks before doing the expansion
Boolean gNicknameWatcherWaitNoKeyDown = false; 	// true if nickname type-ahead should wait until there are no keyDown events in the Event Queue before doing the expansion
Boolean gNicknameHilitingEnabled = false;				// true if nickname hilighting is turned on
Boolean	gNicknameAutoExpandEnabled = false;			// true if nickname expansion is turned on
Boolean	gNicknameCacheEnabled = false;					// true if nickname caching is turned on

//
//	A few more globals... Semi-preference related, but not field related either
//

Str255	gRecipientDelimiterList;
long		gTypeAheadIdleDelay = 15;			// delay, in ticks, that we wait from the last keystroke before we do nickname type-ahead processing
long		gHilitingIdleDelay	= 15;			// delay, in ticks, that we wait from the last keystroke before we do nickname type-ahead processing

//
//	Globals we _may_ want to move into the PeteExtra record...
//

Str255						gTypeAheadPrefix;							// prefix of current nickname type-ahead selection; this is the text the user actually typed, and isn't included in the selection
Str255						gTypeAheadSuffix;							// suffix of current nickname type-ahead selection; this is the part that we inserted and selected ourselves
NicknameTableHdl	gTypeAheadNicknameTable;			// alphabetized NicknameTableHdl of all nicknames which match the current prefix
long							gTypeAheadNicknameTableIndex;	// index of the entry in gTypeAheadNicknameTable of the nickname used for the current nickname type-ahead selection


//
//	Constants that could conceivably become preferences, but are now constants to eliminate evil globals
//

#define	kMinCharsForTypeAhead		1


/************************************************************************
 * 7/17/96	ALB
 *		Performance update for very large nickname files. 
 *		- Each nickname is no longer stored in a separate handle. They
 *			are all stored in one handle with an offset to each one 
 *		- Addresses, notes, and view data are no longer kept in a separate handle
 ************************************************************************/



#pragma mark ========== Utility routines ==========
Boolean AppearsInWhichAliasFile(uLong addrHash,short which);


#ifdef NE_PARAM_VALIDATION
/* DEBUG *//* REMOVE */
void NEValidateWin(MyWindowPtr win)

{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	OSErr err;
	PETEHandle pte;
	MessHandle messH;
	PeteExtraHandle peteExtra;
	UHandle textHdl;

	if (!win)
	{
		DebugStr("\pnil win");
		return;
	}
	pte = NickPeteToWatch (win);
	if (!PeteIsValid(pte))
	{
		DebugStr("\pnil pte");
		return;
	}
	if (GetWindowKind(winWP)==COMP_WIN || GetWindowKind(winWP)==MESS_WIN) {
		messH = Win2MessH(win);
		if (!messH)
		{
			DebugStr("\pnil messH");
			return;
		}
	}
	peteExtra = (PeteExtraHandle)PETEGetRefCon(PETE, pte);
	if (!peteExtra)
	{
		DebugStr("\pnil peteExtra");
		return;
	}
	err = PeteGetRawText(pte, &textHdl);
	if (err)
	{
		lDebugLong("\perr on PeteGetRawText = #", err);
		return;
	}
	if (!textHdl)
	{
		DebugStr("\pnil textHdl");
		return;
	}
}

void NEValidatePte(PETEHandle pte)

{
	if (!PeteIsValid(pte))
	{
		DebugStr("\pnil pte");
		return;
	}
	NEValidateWin ((*PeteExtra(pte))->win);
}
	
#endif


/* MJN *//* new routine */
/* FINISH *//* move to stringutil.c? */
/************************************************************************
 * NicknameRawEqualString
 ************************************************************************/
Boolean NicknameRawEqualString(ConstStr255Param str1, ConstStr255Param str2)

{
	/* FINISH *//* as per Mr. Cannon's useful response, rewrite this to not use any Apple routines! */

	if (str1[0] != str2[0])
		return false;
	return (memcmp(str1 + 1, str2 + 1, str1[0]) ? false : true); /* FINISH *//* use something else? */
}


/* MJN *//* new routine */
/* FINISH *//* move to stringutil.c? */
/************************************************************************
 * NicknameNormalizeString
 ************************************************************************/
void NicknameNormalizeString(Str255 theString, Boolean caseSens, Boolean diacSens, Boolean stickySens)

{
	/* FINISH *//* as per Mr. Cannon's useful response, rewrite this to not use any Apple routines! */
	/* nothing funny, Martin.  FurrinSort controls whether the script manager should be used for this
	   sort of thing.  Big performance gains if not.  SD */

	if (!caseSens && !diacSens)
		UppercaseStripDiacritics(theString + 1, theString[0], smRoman); /* FINISH *//* support scripts other than smRoman */
	else if (!caseSens)
		MyUpperText(theString + 1, theString[0]);	// uses script manager or not, depending on FurrinSort
	else if (!diacSens)
		StripDiacritics(theString + 1, theString[0], smRoman); /* FINISH *//* support scripts other than smRoman */
	if (!stickySens)
		StripStickyCharacters(theString);
}


/* MJN *//* new routine */
/* FINISH *//* move to stringutil.c? */
/************************************************************************
 * NicknameEqualString
 ************************************************************************/
Boolean NicknameEqualString(ConstStr255Param str1, ConstStr255Param str2, Boolean caseSens, Boolean diacSens, Boolean stickySens)

{
	Str255 str1_, str2_;


	if (caseSens && diacSens && stickySens)
		return NicknameRawEqualString(str1, str2);

	BlockMoveData(str1, str1_, str1[0] + 1);
	BlockMoveData(str2, str2_, str2[0] + 1);
	NicknameNormalizeString(str1_, caseSens, diacSens, stickySens);
	NicknameNormalizeString(str2_, caseSens, diacSens, stickySens);
	return NicknameRawEqualString(str1_, str2_);
}


/* MJN *//* new routine */
/* FINISH *//* move to stringutil.c? */
/************************************************************************
 * EqualStringPrefix - returns true if the string passed in prefixStr
 * is equal to the beginning of otherStr.  If caseSens is false,
 * differences in case will be ignored.  If diacSens is false,
 * differences in diacritical markings (�, �, �, etc.) will be ignored.
 * If stickySens is false, differences in sticky (non-breaking)
 * vs. non-sticky spaces and dashes will be ignored.
 *
 * This routine will not move or purge memory, although it will drool if
 * it is exposed to doughnuts for prolonged periods of time.
 ************************************************************************/
 /* FINISH *//* we may now be allocating memory, so adjust the comment */
Boolean EqualStringPrefix(Str255 prefixStr, Str255 otherStr, Boolean caseSens, Boolean diacSens, Boolean stickySens)

{
	Str255 otherStr_;


	if (prefixStr[0] > otherStr[0])
		return false;
	BlockMoveData(otherStr, otherStr_, otherStr[0] + 1);
	otherStr_[0] = prefixStr[0];
	return NicknameEqualString(prefixStr, otherStr_, caseSens, diacSens, stickySens);
}


/* MJN *//* new routine */
/* FINISH *//* move to stringutil.c? */
/************************************************************************
 * CharIsTypingChar
 ************************************************************************/
Boolean CharIsTypingChar(unsigned char keyChar, unsigned char keyCode)

{
#pragma unused (keyChar)

	if ((keyCode >= 0x60) && (keyCode <= 0x65))
		return false;
	if ((keyCode >= 0x71) && (keyCode <= 0x7A))
		return false;
	switch (keyCode)
	{
		case 0x35:
		case 0x67:
		case 0x69:
		case 0x6B:
		case 0x6D:
		case 0x6F:
			return false;
	}
	return true;
}


#pragma mark ========== Nickname data routines ==========


/* MJN *//* new routine */
/* FINISH *//* move to nickmng.c */
/************************************************************************
 * GetNicknameEmailAddressPStr - this routine fetches the Pascal string
 * of the e-mail address for a nickname.  The nickname is specified by
 * aliasIndex and nicknameIndex; aliasIndex is the index of the alias
 * (i.e. (*Aliases)[aliasIndex]), and nicknameIndex is the index of the
 * nickname within the alias referenced by aliasIndex.  If there is
 * more than one address stored with that nickname, all of the addresses
 * are all returned, delimited by commas.  If the address field for that
 * nickname contains nicknames, no attempt is made to expand those
 * nicknames (if you are looking to do this, try using ExpandAliases).
 * The resulting string is returned in emailAddressStr, and a pointer
 * to this string is also returned as the function result.
 *
 * WARNING: The caller is responsible for making sure that
 * RegenerateAllAliases was called prior to calling this routine.
 ************************************************************************/
PStr GetNicknameEmailAddressPStr(short aliasIndex, short nicknameIndex, PStr emailAddressStr)
{
	Handle emailAddress;
	unsigned long emailAddressSize;


	emailAddressStr[0] = 0;

	emailAddress = GetNicknameData(aliasIndex, nicknameIndex, true, true);
	if (!emailAddress)
		return emailAddressStr;
	emailAddressSize = GetHandleSize(emailAddress);
	if (!emailAddressSize)
		return emailAddressStr;
	if ((emailAddressSize == 1) && (**emailAddress == ',')) /* this is what GetNicknameData returns if the address field is empty */
		return emailAddressStr;
	if (emailAddressSize > 255)
		emailAddressSize = 255;
	BlockMoveData(*emailAddress, emailAddressStr + 1, emailAddressSize);
	emailAddressStr[0] = emailAddressSize;

	return emailAddressStr;
}

/************************************************************************
 * GetNicknamePhrasePStr - given an alias, get the according-to-hoyle name/phrase
 ************************************************************************/
PStr GetNicknamePhrasePStr(short aliasIndex, short nicknameIndex, PStr nameStr)
{
	// is there an explicit name field?
	GetNicknameTrueNamePStr(aliasIndex,nicknameIndex,nameStr,false);
	
	// No name field.  Combine first and last names.
	if (!*nameStr)
	{
		Str255 firstName,lastName;
		Str31 tag;
								
		GetTaggedFieldValueStr (aliasIndex,nicknameIndex, GetRString (tag, ABReservedTagsStrn + abTagFirst), firstName);
		GetTaggedFieldValueStr (aliasIndex,nicknameIndex, GetRString (tag, ABReservedTagsStrn + abTagLast), lastName);

		JoinFirstLast (nameStr, firstName, lastName);
	}							
	
	// always return it, even if it's not meaningful
	return nameStr;
}

/* MJN *//* new routine */
/* FINISH *//* move to nickmng.c */
/************************************************************************
 * GetNicknameTrueNamePStr - this routine fetches the Pascal string
 * of the name field for a nickname.  The nickname is specified by
 * aliasIndex and nicknameIndex; aliasIndex is the index of the alias
 * (i.e. (*Aliases)[aliasIndex]), and nicknameIndex is the index of the
 * nickname within the alias referenced by aliasIndex.  If the name field
 * for the specified nickname is empty, the function will return the
 * nickname, surrounded by international quotes (� and �).  The resulting
 * string is returned in trueNameStr, and a pointer to this string is
 * also returned as the function result.
 *
 * WARNING: The caller is responsible for making sure that
 * RegenerateAllAliases was called prior to calling this routine.
 ************************************************************************/
PStr GetNicknameTrueNamePStr(short aliasIndex, short nicknameIndex, PStr trueNameStr, Boolean okToMakeItUp)
{
	OSErr err;
	Str255 nameStr;
	Str255 nameFieldTag; /* the "notes" section of nickname data contains the true name; this string is used to tag that data */
	Boolean nicknameNameEmpty;


	trueNameStr[0] = 0;

	nameStr[0] = 0;
	GetRString(nameFieldTag, NickManKeyStrn + 1); /* get tag for Name field in nickname data */
	err = NickGetDataFromField(nameFieldTag, nameStr, aliasIndex, nicknameIndex, !okToMakeItUp, false, &nicknameNameEmpty);
	if (err)
		return trueNameStr;
	BlockMoveData(nameStr, trueNameStr, nameStr[0] + 1);

	return trueNameStr;
}


/* MJN *//* new routine */
/* FINISH *//* move to nickmng.c */
/************************************************************************
 * GetExpandedNicknamePStr - this routine extracts the name field and
 * e-mail address from a nickname, and builds them into a string.  If
 * both are present, the string is built into the format of
 * True Name <addr@domain.xxx>, and if one of those fields is empty, then
 * this function simply returns the non-empty field.  The returned output
 * should be suitable for simple display of this info in the user
 * interface, but is not appropriate for generating actual text to go
 * into the recipient fields of a message header.
 *
 * The nickname is specified by aliasIndex and nicknameIndex; aliasIndex
 * is the index of the alias (i.e. (*Aliases)[aliasIndex]), and
 * nicknameIndex is the index of the nickname within the alias referenced
 * by aliasIndex.  The resulting string is returned in
 * expandedNicknameStr, and a pointer to this string is also returned as
 * the function result.
 *
 * This function gets the strings for the name field and e-mail address
 * by calling GetNicknameTrueNamePStr and GetNicknameEmailAddressPStr.
 * For more details, see the comments for those routines.
 *
 * WARNING: The caller is responsible for making sure that
 * RegenerateAllAliases was called prior to calling this routine.
 ************************************************************************/
PStr GetExpandedNicknamePStr(short aliasIndex, short nicknameIndex, PStr expandedNicknameStr)
{
	Str255 nameStr;
	Str255 emailAddressStr;
	Str255 formatInfo;
	StringPtr onlyString;


	expandedNicknameStr[0] = 0;

	GetNicknameTrueNamePStr(aliasIndex, nicknameIndex, nameStr, true);
	GetNicknameEmailAddressPStr(aliasIndex, nicknameIndex, emailAddressStr);

	if (nameStr[0] && emailAddressStr[0])
	{
		GetRString(formatInfo, ADD_REALNAME);
		utl_PlugParams(formatInfo, expandedNicknameStr, emailAddressStr, nameStr, nil, nil);
	}
	else
	{
		onlyString = nameStr[0] ? nameStr : emailAddressStr;
		BlockMoveData(onlyString, expandedNicknameStr, onlyString[0] + 1);
	}

	return expandedNicknameStr;
}


/* MJN *//* new routine */
/* FINISH *//* move to nickmng.c */
/************************************************************************
 * GetNicknameRecipientText - this routine allocates a handle and fills
 * it with the expansion of the specified nickname.  The returned output
 * is suitable for inserting into a recipient field in a message header.
 * If there are embedded nicknames, they will be recursively expanded.
 * If there are nicknames which have more than one address, they will
 * be expanded using group syntax (GroupName: addr1, addr2, ... addrN;)
 *
 * The nickname is specified by aliasIndex and nicknameIndex; aliasIndex
 * is the index of the alias (i.e. (*Aliases)[aliasIndex]), and
 * nicknameIndex is the index of the nickname within the alias referenced
 * by aliasIndex.  The resulting handle is returned in
 * *recipientText; the handle size is set to the exact size of the text.
 *
 * The function returns noErr if it succeeded, or a non-zero error code
 * if an error occurred.
 *
 * WARNING: The caller is responsible for making sure that
 * RegenerateAllAliases was called prior to calling this routine.
 ************************************************************************/
OSErr GetNicknameRecipientText(PETEHandle pte, short aliasIndex, short nicknameIndex, UHandle *recipientText)
{
	OSErr err;
	Str255 nicknameStr;
	long nicknameLen;
	UHandle nicknameTxtHdl;
	UHandle resultTxtHdl;


	*recipientText = nil;

	GetNicknameNamePStr(aliasIndex, nicknameIndex, nicknameStr);
	if (!nicknameStr[0])
		return paramErr;
	nicknameLen = nicknameStr[0] + 1;
	nicknameTxtHdl = NewHandleClear(nicknameLen + 5); /* FINISH *//* why +5? */
	if (!nicknameTxtHdl)
		return MemError();
	BlockMoveData(nicknameStr, *nicknameTxtHdl, nicknameLen);

	err = PeteExpandAliases(pte, &resultTxtHdl, nicknameTxtHdl, 0, true);
	DisposeHandle(nicknameTxtHdl);
	if (err)
		return err;

	CommaList(resultTxtHdl);

	*recipientText = resultTxtHdl;
	return noErr;
}


#pragma mark ========== Nickname search routines ==========


/* MJN *//* new routine */
/* FINISH *//* rename back to FindNickname */
/* WARNING *//* There is another routine called FindNickname in nickae.c.  These two
								routines are unrelated.  The other FindNickname was implemented first,
								but I chose to use this routine name anyways.  This routine MUST
								be declared as static in order for it to work properly. */
/************************************************************************
 * NEFindNickname
 ************************************************************************/
Boolean NEFindNickname(Str255 nicknameSearchStr, Boolean caseSens, Boolean diacSens, Boolean stickySens, short *foundAliasIndex, short *foundNicknameIndex)
{
	Str255 nicknameSearchStr_;
	Boolean found;
	long numAliases;
	long numNicknames;
	short aliasIndex;
	short nicknameIndex;
	NickStructHandle curAliasData;
	Str255 curNicknameStr;
	Str255 curNicknameStr_;


	if (foundAliasIndex)
		*foundAliasIndex = -1;
	if (foundNicknameIndex)
		*foundNicknameIndex = -1;

	numAliases = NAliases;

	BlockMoveData(nicknameSearchStr, nicknameSearchStr_, nicknameSearchStr[0] + 1);
	NicknameNormalizeString(nicknameSearchStr_, caseSens, diacSens, stickySens);

	found = false;
	aliasIndex = 0;
	nicknameIndex = 0;
	while (!found && (aliasIndex < numAliases))
	{
		if (curAliasData = (*Aliases)[aliasIndex].theData)
		{
			numNicknames = GetHandleSize_(curAliasData)/sizeof(NickStruct);
			while (!found && (nicknameIndex < numNicknames))
			{
				if (!(*curAliasData)[nicknameIndex].deleted)
				{
					GetNicknameNamePStr(aliasIndex, nicknameIndex, curNicknameStr);
					if (curNicknameStr[0])
					{
						BlockMoveData(curNicknameStr, curNicknameStr_, curNicknameStr[0] + 1);
						NicknameNormalizeString(curNicknameStr_, caseSens, diacSens, stickySens);
						if (NicknameRawEqualString(nicknameSearchStr_, curNicknameStr_))
							found = true;
					}
				}
				if (!found)
					nicknameIndex++;
			}
		}
		if (!found)
		{
			nicknameIndex = 0;
			aliasIndex++;
		}
	}

	if (found)
	{
		if (foundAliasIndex)
			*foundAliasIndex = aliasIndex;
		if (foundAliasIndex)
			*foundNicknameIndex= nicknameIndex;
	}
	return found;
}


/* MJN *//* new routine */
/* FINISH *//* for this and its similar routines, pre-normalize prefixStr_, so that we don't re-do it each time we call EqualStringPrefix */
/************************************************************************
 * FindNextNicknamePrefix - used to search through the aliases for a
 * nickname which starts with the prefix passed in prefixStr.  It stops
 * and returns at each match found.  The string compare is insensitive
 * to case, diacritical markings, and special non-breaking characters.
 *
 * If a match was found, the function returns true, and returns the
 * indices of the alias and nickname that identify the match in
 * *foundAliasIndex and *foundNicknameIndex.  If there are no more matches,
 * or if startAliasIndex is out of range, the function returns false,
 * and returns -1 in *foundAliasIndex and *foundNicknameIndex.
 *
 * The search begins with the alias specified by startAliasIndex and
 * the nickname specified by startNicknameIndex.  When you first start
 * a search, pass in zero for both of these parameters.  If
 * startNicknameIndex specifies an index beyond the number of nicknames
 * in the alias specified by startAliasIndex, that's okay; in this case,
 * the search will be continued in the next alias.  This means that you
 * can cycle through all nicknames beginning with prefixStr by initially
 * passing in zero for startAliasIndex and startNicknameIndex, and then
 * making subsequent calls, passing in the previously returned value of
 * *foundAliasIndex for startAliasIndex, and passing in 1 + the value
 * returned in *foundNicknameIndex for startNicknameIndex, and repeating
 * this until the function returns false.
 *
 * If you pass in false for allowExactMatch, this routine won't return
 * nicknames which are exactly the same as prefixStr (i.e. if prefixStr
 * is "Frank" and the nickname is "Frank").  If you pass true for this
 * parameter, then all matches are returned, including an exact match.
 *
 * WARNING: The caller is responsible for making sure that
 * RegenerateAllAliases was called prior to calling this routine.
 ************************************************************************/
Boolean FindNextNicknamePrefix(Str255 prefixStr, short startAliasIndex, short startNicknameIndex, short *foundAliasIndex, short *foundNicknameIndex, Boolean allowExactMatch, Boolean findInAllFiles)
{
	long prefixLen;
	Str255 prefixStr_;
	Boolean found;
	long numAliases;
	long numNicknames;
	long noAddressHashValue;
	short aliasIndex;
	short nicknameIndex;
	NickStructHandle curAliasData;
	Str255 nicknameStr;

	*foundAliasIndex = -1;
	*foundNicknameIndex = -1;

	noAddressHashValue = NickHashString ("\p");			// The hash value for an empty expansion

	numAliases = NAliases;

	if ((startAliasIndex < 0) || (startAliasIndex >= numAliases) || (startNicknameIndex < 0))
		return false;

	/* make a local copy of prefixStr, and if the last two characters are ':;', remove them from the string */
	prefixLen = prefixStr[0];
	BlockMoveData(prefixStr, prefixStr_, prefixLen + 1);
	if ((prefixStr_[prefixLen - 1] == ':') && (prefixStr_[prefixLen] == ';'))
		prefixStr_[0] -= 2;

	found = false;
	aliasIndex = startAliasIndex;
	nicknameIndex = startNicknameIndex;
	while (!found && (aliasIndex < numAliases))
	{
		curAliasData = (*Aliases)[aliasIndex].theData;
		numNicknames = curAliasData ? GetHandleSize_(curAliasData)/sizeof(NickStruct) : 0;
		while (!found && (nicknameIndex < numNicknames))
		{
			if (!(*curAliasData)[nicknameIndex].deleted)
			{
				GetNicknameNamePStr(aliasIndex, nicknameIndex, nicknameStr);
				if (nicknameStr[0] && EqualStringPrefix(prefixStr_, nicknameStr, false, true, false) && (allowExactMatch || (prefixStr_[0] != nicknameStr[0]))) {
					if (!PrefIsSet (PREF_ALLOW_EMPTY_NICK_EXPANSIONS)) {
						if ((*curAliasData)[nicknameIndex].addressOffset != -1 && (*curAliasData)[nicknameIndex].hashAddress != noAddressHashValue)
							found = true;
					}
					else
						found = true;
				}
			}
			if (!found)
				nicknameIndex++;
		}
		if (!found)
		{
			nicknameIndex = 0;
			if (findInAllFiles)
				aliasIndex++;
			else
				aliasIndex = numAliases;
		}
	}

	if (found)
	{
		*foundAliasIndex = aliasIndex;
		*foundNicknameIndex= nicknameIndex;
	}
	return found;
}


/* MJN *//* new routine */
/************************************************************************
 * FindNextNicknameTablePrefix - used to search through the nicknames
 * in a NicknameTableHdl for a nickname which starts with prefixStr.
 * It stops and returns at each match found.  The string compare is
 * insensitive to case, diacritical markings, and special non-breaking
 * characters.  This routine is comperable to FindNextNicknamePrefix,
 * except that it searches a nickname table instead of the master
 * nickname data.
 *
 * If a match was found, the function returns true, and returns the
 * index of the matching table entry in *foundEntryIndex.  If there are
 * no more matches, or if startEntryIndex is out of range, the function
 * returns false, and returns -1 in *foundEntryIndex.
 *
 * The search begins with the entry specified by startEntryIndex.  When
 * you first start a search, pass in zero for this parameter.  To search
 * the entire nickname table, keep making subsequent calls, passing in
 * 1 + the value returned in *foundEntryIndex for startEntryIndex, and
 * repeating this until the function returns false.
 *
 * If you pass in false for allowExactMatch, this routine won't return
 * nicknames which are exactly the same as prefixStr (i.e. if prefixStr
 * is "Frank" and the nickname is "Frank").  If you pass true for this
 * parameter, then all matches are returned, including an exact match.
 ************************************************************************/
Boolean FindNextNicknameTablePrefix(Str255 prefixStr, NicknameTableHdl nicknameTable, long startEntryIndex, long *foundEntryIndex, Boolean allowExactMatch)
{
	long prefixLen;
	Str255 prefixStr_;
	Boolean found;
	long numEntries;
	long entryIndex;
	NicknameTableEntryPtr entryPtr;
	StringPtr nicknameStrPtr;
	SInt8 origState;


	*foundEntryIndex = -1;

	numEntries = (**nicknameTable).numEntries;

	if ((startEntryIndex < 0) || (startEntryIndex >= numEntries))
		return false;

	/* make a local copy of prefixStr, and if the last two characters are ':;', remove them from the string */
	prefixLen = prefixStr[0];
	BlockMoveData(prefixStr, prefixStr_, prefixLen + 1);
	if ((prefixStr_[prefixLen - 1] == ':') && (prefixStr_[prefixLen] == ';'))
		prefixStr_[0] -= 2;

	origState = HGetState((Handle)nicknameTable);
	HLock((Handle)nicknameTable);

	found = false;
	entryIndex = startEntryIndex;
	entryPtr = &(**nicknameTable).table[entryIndex];
	while (!found && (entryIndex < numEntries))
	{
		nicknameStrPtr = entryPtr->entryStr;
		if (nicknameStrPtr[0] && EqualStringPrefix(prefixStr_, nicknameStrPtr, false, true, false) && (allowExactMatch || (prefixStr_[0] != nicknameStrPtr[0])))
			found = true;
		else
		{
			entryIndex++;
			entryPtr++;
		}
	}

	HSetState((Handle)nicknameTable, origState);

	if (found)
		*foundEntryIndex = entryIndex;
	return found;
}




/* MJN *//* new routine */
/************************************************************************
 * NicknamePrefixOccursNTimes - returns true if there are at least nTimes
 * nicknames beginning with the string prefixStr
 ************************************************************************/
Boolean NicknamePrefixOccursNTimes(Str255 prefixStr, long nTimes, short aliasFileToScan)
{
	OSErr err;
	short aliasIndex, nicknameIndex;
	long count;
	Boolean found, finished;


	err = RegenerateAllAliases(false); /* FINISH *//* shouldn't we be passing in true? */
	if (err)
		return false;

	aliasIndex = aliasFileToScan == kNickScanAllAliasFiles ? 0 : aliasFileToScan;
	nicknameIndex = 0;
	count = 0;
	finished = false;

	while (!finished && (count < nTimes))
	{
		found = FindNextNicknamePrefix(prefixStr, aliasIndex, nicknameIndex, &aliasIndex, &nicknameIndex, true, aliasFileToScan == kNickScanAllAliasFiles);
		if (found)
		{
			count++;
			nicknameIndex++;
		}
		else
			finished = true;
	}

	return count >= nTimes;
}


/* MJN *//* new routine */
/************************************************************************
 * FindNicknameTableIndex - this routine returns the entry index
 * (zero-based) of the entry in the nickname table passed in
 * nicknameTable which has an alias index of aliasIndex and a
 * nickname index of nicknameIndex.  If the specific combination of
 * aliasIndex/nicknameIndex is not found in the table, the function
 * returns -1.
 ************************************************************************/
long FindNicknameTableIndex(NicknameTableHdl nicknameTable, short aliasIndex, short nicknameIndex)
{
	NicknameTablePtr tablePtr;
	long numEntries;
	long entryIndex;
	NicknameTableEntryPtr entryPtr;


	tablePtr = *nicknameTable;
	numEntries = tablePtr->numEntries;
	entryIndex = 0;
	entryPtr = &tablePtr->table[0];
	while (entryIndex < numEntries)
	{
		if ((aliasIndex == entryPtr->aliasIndex) && (nicknameIndex == entryPtr->nicknameIndex))
			return entryIndex;
		else
		{
			entryIndex++;
			entryPtr++;
		}
	}
	return -1;
}

long FindFirstNonHistoryNicknameTableIndex (NicknameTableHdl nicknameTable)

{
	NicknameTablePtr			tablePtr;
	NicknameTableEntryPtr	entryPtr;
	long									tableIndex,
												numEntries;
	
	tablePtr	 = *nicknameTable;
	entryPtr	 = &tablePtr->table[0];
	numEntries = tablePtr->numEntries;
	for (tableIndex = 0; tableIndex < numEntries; ++tableIndex) {
		if (!IsHistoryAddressBook (entryPtr->aliasIndex))
			return (tableIndex);
		entryPtr++;
	}
	return (0);
}

#pragma mark ========== Nickname table routines ==========


/* MJN *//* new routine */
/************************************************************************
 * GetNicknameTableEntry - given a nickname table passsed in
 * nicknameTable, this function returns in *aliasIndex and *nicknameIndex
 * the alias index and nickname index of the table entry whose index is
 * passed in entryIndex.  If entryIndex is out of range, the function
 * returns -1 in *aliasIndex and *nicknameIndex.
 ************************************************************************/
void GetNicknameTableEntry(NicknameTableHdl nicknameTable, long entryIndex, short *aliasIndex, short *nicknameIndex)
{
	NicknameTablePtr tablePtr;
	NicknameTableEntryPtr entryPtr;


	tablePtr = *nicknameTable;
	if ((entryIndex < 0) || (entryIndex >= tablePtr->numEntries))
	{
		*aliasIndex = -1;
		*nicknameIndex = -1;
		return;
	}

	entryPtr = &tablePtr->table[entryIndex];
	*aliasIndex = entryPtr->aliasIndex;
	*nicknameIndex = entryPtr->nicknameIndex;
}


/* MJN *//* new routine */
/************************************************************************
 * NewNicknameTable - create a new empty nickname table, and return it
 * in *nicknameTable.  A nickname table is a convenient way to build a
 * list of nicknames for temporary internal usage.  Currently, it's
 * being used for building the list of nicknames to display in the
 * nicknames sticky popup menu.  The function returns noErr if it
 * succeeds, or a non-zero error code if it fails.
 ************************************************************************/
OSErr NewNicknameTable(NicknameTableHdl *nicknameTable)
{
	NicknameTableHdl newTable;
	NicknameTablePtr newTablePtr;

	newTable = (NicknameTableHdl)NewHandleClear(sizeof(NicknameTable));
	if (!newTable)
	{
		*nicknameTable = nil;
		return MemError();
	}
	newTablePtr = *newTable;
	newTablePtr->logicalSize = newTablePtr->physicalSize = sizeof(NicknameTable);
	*nicknameTable = newTable;
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * DisposeNicknameTable - dispose of the nickname table specified
 * in nicknameTable
 ************************************************************************/
void DisposeNicknameTable(NicknameTableHdl nicknameTable)
{
	DisposeHandle((Handle)nicknameTable);
}


/* MJN *//* new routine */
/************************************************************************
 * CompactNicknameTable - compacts the handle containing the nickname
 * table passed in nicknameTable, so that it uses no more memory than
 * is needed.  Returns noErr if it succeeds, or a non-zero error code
 * if an error occurs.
 ************************************************************************/
OSErr CompactNicknameTable(NicknameTableHdl nicknameTable)
{
	OSErr err;
	long logicalSize;


	logicalSize = (**nicknameTable).logicalSize;
	if ((**nicknameTable).physicalSize == logicalSize)
		return noErr;
	SetHandleSize((Handle)nicknameTable, logicalSize);
	err = MemError();
	if (err)
		return err;
	(**nicknameTable).physicalSize = logicalSize;
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * GrowNicknameTable - routine used by AddNicknameTableEntry to manage
 * the handle containing a nickname table.  Rather than calling
 * SetHandleSize to increase the handle's size each time we add an
 * entry, we grow the handle in chunks.  The nickname table is specified
 * in nicknameTable.  entryCount indicates the number of entries the
 * caller is planning to add; if used carefully, you could pass in a
 * negative number for this parameter, but you really shouldn't do that.
 * The routine will adjust the NicknameTableHdl so that entryCount
 * entries, can be added, and grow the handle if needed.
 *
 * The function returns noErr if it succeeds, or a non-zero error code
 * if an error occurred.
 ************************************************************************/
OSErr GrowNicknameTable(NicknameTableHdl nicknameTable, long entryCount)
{
	OSErr err;
	NicknameTablePtr tablePtr;
	long neededSize;
	long newPhysicalSize;


	tablePtr = *nicknameTable;
	neededSize = tablePtr->logicalSize + (entryCount * sizeof(NicknameTableEntry));
	if (tablePtr->physicalSize >= neededSize)
	{
		tablePtr->logicalSize = neededSize;
		return noErr;
	}

	newPhysicalSize = tablePtr->physicalSize;
	while (newPhysicalSize < neededSize)
		newPhysicalSize += NICK_TABLE_ALLOC_BLOCK_COUNT * sizeof(NicknameTableEntry);
	SetHandleSize((Handle)nicknameTable, newPhysicalSize);
	err = MemError();
	if (err)
		return err;

	tablePtr = *nicknameTable;
	tablePtr->logicalSize = neededSize;
	tablePtr->physicalSize = newPhysicalSize;
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * AddNicknameTableEntry - add a nickname to the nickname table passed
 * in nicknameTable.  aliasIndex is the index of the alias
 * (i.e. (*Aliases)[aliasIndex]), and nicknameIndex is the index of the
 * nickname within the alias referenced by aliasIndex.  newEntryStr is
 * the string that will be entered in this entry for the specified
 * nickname; it is not necessarily the nickname itself or its expansion,
 * it's for whatever string you want to associate with that nickname.
 *
 * If you pass true for sortedInsert, the position where the entry is
 * inserted is based on an alpha-numeric sort using newEntryStr.  If you
 * pass false for sortedInsert, the entry is just appended to the end of
 * the table.
 *
 * If the function succeeds, it returns noErr.  If it fails, it will
 * return a non-zero error code, and the nickname table will remain
 * unchanged.
 ************************************************************************/
OSErr AddNicknameTableEntry(NicknameTableHdl nicknameTable, short aliasIndex, short nicknameIndex, Str255 newEntryStr, Boolean sortedInsert)
{
	OSErr err;
	Str255 newEntryStr_normalized;
	Str255 curEntryStr_normalized;
	NicknameTablePtr tablePtr;
	NicknameTableEntryPtr entryPtr;
	long newEntryIndex;
	Boolean foundInsertionPoint;
	long entryIndex;
	SInt8 origState;
	long first, last;
	short result;


	err = GrowNicknameTable(nicknameTable, 1);
	if (err)
		return err;

	origState = HGetState((Handle)nicknameTable);
	HLock((Handle)nicknameTable);
	tablePtr = *nicknameTable;
	tablePtr->numEntries++;
	newEntryIndex = tablePtr->numEntries - 1;

	if (sortedInsert)
	{
		/* we do the search for the proper insertion point backwards, so that the search will be optimized
				in the event that the caller is adding items in alpha-numeric sorted order */

		BlockMoveData(newEntryStr, newEntryStr_normalized, newEntryStr[0] + 1);
		StickyPopupNormalizeString(newEntryStr_normalized);
		foundInsertionPoint = false;
		first = 0;
		last = newEntryIndex-1;
		entryIndex = 0;
		entryPtr = tablePtr->table;
		result = -1;
		while (first<=last)
		{
			entryIndex = (first+last)/2;
			entryPtr = &tablePtr->table[entryIndex];
			BlockMoveData(entryPtr->entryStr, curEntryStr_normalized, entryPtr->entryStr[0] + 1);
			StickyPopupNormalizeString(curEntryStr_normalized);
			result = StickyPopupCompareString(newEntryStr_normalized, curEntryStr_normalized, false);
			if (!result) break;	// found exact match
			else if (result<0) last = entryIndex-1;
			else first = entryIndex+1;
		}
		if (result<0) {entryIndex--;entryPtr--;}	// last one we looked at was greater than we are, so insert before
		entryIndex++;
		entryPtr++;
		if (entryIndex < newEntryIndex)
			BlockMoveData(entryPtr, entryPtr + 1, (newEntryIndex - entryIndex) * sizeof(NicknameTableEntry));
	}
	else
	{
		entryIndex = newEntryIndex;
		entryPtr = &tablePtr->table[newEntryIndex];
	}

	entryPtr->aliasIndex = aliasIndex;
	entryPtr->nicknameIndex = nicknameIndex;
	BlockMoveData(newEntryStr, entryPtr->entryStr, newEntryStr[0] + 1);

	HSetState((Handle)nicknameTable, origState);
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * BuildNicknameTable - builds a sorted nickname table of all nicknames
 * beginning with a specific prefix string.  prefixStr is the prefix
 * string to search for.  If showExpansions is set to true, then the
 * entryStr in the table (and thus the popup) will include the expansion
 * of the nicknames.  If allowExactMatch is set to true, then the table
 * will include nicknames which match prefixStr exactly (this parameter
 * is passed in as the allowExactMatch parameter for calls to
 * FindNextNicknamePrefix; see the comments for that routine for more
 * details).  The resulting table is returned via *builtNicknameTable.
 * The table is sorted using the default sorting method of
 * AddNicknameTableEntry.  The function returns noErr if it succeeds,
 * or a non-zero error code if it fails.
 *
 * You can limit the table to contain only those nicknames from a
 * particular alias file by passing an alias index in 'inAliasFile', or
 * -1 if you wish to include nicknames appearing in all alias files.
 ************************************************************************/
OSErr BuildNicknameTable(Str255 prefixStr, short inAliasFile, Boolean showExpansions, Boolean allowExactMatch, NicknameTableHdl *builtNicknameTable)
{
	OSErr err;
	NicknameTableHdl nicknameTable;
	short aliasIndex;
	short nicknameIndex;
	Str255 nicknameStr;
	Boolean finished;

	*builtNicknameTable = nil;

	err = NewNicknameTable(&nicknameTable);
	if (err)
		return err;

	err = RegenerateAllAliases(false); /* FINISH *//* shouldn't we be passing in true? */
	if (err)
	{
		DisposeNicknameTable(nicknameTable);
		return err;
	}

	finished = false;
	aliasIndex = inAliasFile == kNickScanAllAliasFiles ? 0 : inAliasFile;
	nicknameIndex = 0;
	while (!finished)
	{
		finished = !FindNextNicknamePrefix (prefixStr, aliasIndex, nicknameIndex, &aliasIndex, &nicknameIndex, allowExactMatch, inAliasFile == kNickScanAllAliasFiles);
		if (!finished)
		{
			if (showExpansions)
				GetExpandedNicknamePStr(aliasIndex, nicknameIndex, nicknameStr);
			else
				GetNicknameNamePStr(aliasIndex, nicknameIndex, nicknameStr);
			err = AddNicknameTableEntry(nicknameTable, aliasIndex, nicknameIndex, nicknameStr, true);
			if (err)
			{
				DisposeNicknameTable(nicknameTable);
				return err;
			}
			nicknameIndex++;
		}
		if (EventPending()) {err = userCanceledErr; break;} 	// bail if something else to do
	}


	if (!err) err = CompactNicknameTable(nicknameTable);
	if (err)
	{
		DisposeNicknameTable(nicknameTable);
		return err;
	}

	BlockMoveData(prefixStr, (**nicknameTable).prefixStr, prefixStr[0] + 1);

	*builtNicknameTable = nicknameTable;
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * BuildNicknameTableSubset - builds a sorted nickname table of all
 * nicknames in an already existing nickname table beginning with a
 * specific prefix string.  This routine is useful when you need to build
 * a nickname table, and there is an existing table which contains
 * nicknames of a prefix that's the same as the beginning of the new
 * prefix; this way, instead of conducting a full search of all nicknames,
 * the data to search is already significantly narrowed down.  Aside from
 * the fact that this routine searches for matches in a nickname table as
 * opposed to the master nickname data, this routine is essentially the
 * same as BuildNicknameTable.
 *
 * prefixStr is the prefix string to search for.  srcNicknameTable is the
 * nickname table to search in.  Unlike BuildNicknameTable, this routine
 * does not have a showExpansions parameter, and it is assumed that
 * srcNicknameTable was built with showExpansions set to false.  If
 * allowExactMatch is set to true, then the table will include nicknames
 * which match prefixStr exactly (this parameter is passed in as the
 * allowExactMatch parameter for calls to FindNextNicknameTablePrefix;
 * see the comments for that routine for more details).  The resulting
 * table is returned via *builtNicknameTable.  The table is sorted using
 * the default sorting method of AddNicknameTableEntry.  The function
 * returns noErr if it succeeds, or a non-zero error code if it fails.
 ************************************************************************/
OSErr BuildNicknameTableSubset(Str255 prefixStr, NicknameTableHdl srcNicknameTable, Boolean allowExactMatch, NicknameTableHdl *builtNicknameTable)
{
	OSErr err;
	NicknameTableHdl nicknameTable;
	NicknameTablePtr srcNicknameTablePtr;
	NicknameTableEntryPtr entryPtr;
	long srcEntryIndex;
	Boolean finished;
	SInt8 origState;


	*builtNicknameTable = nil;

	err = NewNicknameTable(&nicknameTable);
	if (err)
		return err;

	origState = HGetState((Handle)srcNicknameTable);
	HLock((Handle)srcNicknameTable);
	srcNicknameTablePtr = *srcNicknameTable;

	finished = false;
	srcEntryIndex = 0;
	while (!finished)
	{
		finished = !FindNextNicknameTablePrefix(prefixStr, srcNicknameTable, srcEntryIndex, &srcEntryIndex, allowExactMatch);
		if (!finished)
		{
			entryPtr = &srcNicknameTablePtr->table[srcEntryIndex];
			err = AddNicknameTableEntry(nicknameTable, entryPtr->aliasIndex, entryPtr->nicknameIndex, entryPtr->entryStr, true);
			if (err)
			{
				HSetState((Handle)srcNicknameTable, origState);
				DisposeNicknameTable(nicknameTable);
				return err;
			}
			srcEntryIndex++;
		}
	}

	HSetState((Handle)srcNicknameTable, origState);

	err = CompactNicknameTable(nicknameTable);
	if (err)
	{
		DisposeNicknameTable(nicknameTable);
		return err;
	}

	BlockMoveData(prefixStr, (**nicknameTable).prefixStr, prefixStr[0] + 1);

	*builtNicknameTable = nicknameTable;
	return noErr;
}


#pragma mark ========== Nickname popup routines ==========


/* MJN *//* new routine */
/************************************************************************
 * TruncateNicknamePopupItems - this routine is used by
 * DoNicknameStickyPopup to insure that the pixel width of the entries
 * in the popup won't exceed a certain length.  If the item name needs
 * to be truncated, this routine will truncate just the minimum amount
 * needed, and add an ellipses character (�) to the end of the item name.
 * The routine truncates so that the popup will, at a maximum, be just
 * a little wider than the window from which it is being evoked.  This
 * routine currently only supports Roman scripts.
 *
 * nicknameTable is a table of nicknames already built by
 * DoNicknameStickyPopup, containing the list of items to be displayed
 * in the popup.  fontNum and fontSize specify the font family ID and
 * font point size in which the popup will be displayed.  win specifies
 * the window with which the popup is associated.
 ************************************************************************/
void TruncateNicknamePopupItems (NicknameTableHdl nicknameTable, short fontNum, short fontSize, MyWindowPtr win)

{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect winRect;
	long maxWidth;
	NicknameTablePtr tablePtr;
	long numItems;
	long curItem;
	NicknameTableEntryPtr curItemPtr;
	StringPtr curItemStrPtr;
	long curItemWidth;
	char truncateChar;
	long truncateCharWidth;
	long neededGain, currentGain;
	GrafPtr origPort;
	CGrafPtr tempPort;
	Ptr p;
	SInt8 origState;

	/* FINISH *//* support scripts other than smRoman */

	/* make sure we aren't desperately low on memory */
	p = NewPtr(4096);
	if (!p)
		return;
	DisposePtr(p);

	GetWindowPortBounds(winWP,&winRect);
	maxWidth = winRect.right - winRect.left; /* this will actually allow the popup to be slightly wider than the window */

	origState = HGetState((Handle)nicknameTable);
	HLock((Handle)nicknameTable);
	tablePtr = *nicknameTable;

	GetPort(&origPort);
	MyCreateNewPort(tempPort);
	TextFont(fontNum);
	TextSize(fontSize);

	truncateChar = ellipsesChar;
	truncateCharWidth = CharWidth(truncateChar);

	numItems = tablePtr->numEntries;
	for (curItem = 0, curItemPtr = &tablePtr->table[0]; curItem < numItems; curItem++, curItemPtr++)
	{
		curItemStrPtr = curItemPtr->entryStr;
		curItemWidth = StringWidth(curItemStrPtr);
		if (curItemWidth <= maxWidth)
			continue;
		neededGain = (curItemWidth + truncateCharWidth) - maxWidth;
		currentGain = 0;
		while ((currentGain < neededGain) && (curItemStrPtr[0] > 1))
		{
			currentGain += CharWidth(curItemStrPtr[curItemStrPtr[0]]);
			curItemStrPtr[0]--;
		}
		curItemStrPtr[++curItemStrPtr[0]] = truncateChar;
	}

	HSetState((Handle)nicknameTable, origState);
	DisposePort(tempPort);
	SetPort(origPort);
}


/* MJN *//* new routine */
/************************************************************************
 * CalcNicknamePopupLoc - calculates where the top-left corner of the
 * nicknames sticky popup should be located.  win is the window in which
 * the user is typing; the popup will be positioned so that its top-left
 * corner is just next to the top-right corner of the current text
 * selection or text insertion point for the window passed in win, unless
 * the current selection was created by nickname type-ahead, in which
 * case the popup will be positioned with its top-left corner over the
 * top-left corner of the text selection.  The location is returned
 * in *popupLoc, and is in global coordinates.
 *
 * (jp) Added a bit of code to return a 'teflon' rectangle.  This is a
 *			rectangle we want to avoid obstructing when displaying the sticky
 *			popup.
 ************************************************************************/
void CalcNicknamePopupLoc (PETEHandle pte, Point *popupLoc, Rect *teflon)

{
	ComponentResult result;
	GrafPtr origPort;
	Boolean hasTypeAheadSel;
	PETEDocInfo info;
	Point position,
				startPos;
	LHElement	lineHeight;

	if (!PeteIsValid(pte)) {
		position.v = position.h = 50; /* return something that will be mostly harmless */
		return;
	}
	
	hasTypeAheadSel = CurSelectionIsTypeAheadText (pte);

	result = PETEGetDocInfo(PETE, pte, &info);
	if (!result)
		result = PETEOffsetToPosition(PETE, pte, (hasTypeAheadSel ? info.selStart : info.selStop), &position, nil);
	
	// (jp) additions for teflon rect
	if (!result)
		result = PETEOffsetToPosition (PETE, pte, 0, &startPos, &lineHeight);
	if (!result)
		SetRect (teflon, startPos.h - 2, startPos.v, position.h, startPos.v + lineHeight.lhHeight);
	
	if (result)
		position.v = position.h = 0;
	GetPort(&origPort);
	SetPort (GetMyWindowCGrafPtr((*PeteExtra(pte))->win));
	LocalToGlobal(&position);
	LocalToGlobal((Point*)teflon);
	LocalToGlobal((Point*)teflon + 1);
	SetPort(origPort);
	*popupLoc = position;
}


/* MJN *//* new routine */
/************************************************************************
 * DoNicknameStickyPopup - run the nicknames sticky popup menu.  The menu
 * is built to have a list of all nicknames which begin with the prefix
 * passed in prefixStr.  If you pass in true for showExpansions, the
 * menu items will contain the expansions of the nicknames.  win is the
 * window with which the popup is associated; you should pass it the
 * window that the user is currently typing in.  If the user selects a
 * nickname, the alias index and nickname index are returned in
 * *resultAliasIndex and *resultNicknameIndex, respectively.  The
 * function returns true if the user selected a valid nickname, and it
 * returns false if the user did not select a nickname (i.e. cancel the
 * popup by clicking outside of the popup or pressing Command-Period).
 ************************************************************************/
Boolean DoNicknameStickyPopup(PETEHandle pte, Boolean showExpansions, Str255 prefixStr, short *resultAliasIndex, short *resultNicknameIndex)
{
	OSErr err;
	NicknameTableHdl nicknameTable;
	NicknameTablePtr tablePtr;
	NicknameTableEntryPtr entryPtr;
	short fontNum, fontSize;
	StickyPopupHdl popup;
	Rect teflon;
	Point popupLoc;
	long result;
	Str255 scratchStr;

	if (!PeteIsValid(pte))
		return (false);

	*resultAliasIndex		 = -1;
	*resultNicknameIndex = 0;

	err = noErr;
	result = 0;

	/* this is what prefixStr looks like if we get called from FinishAlias with no prefix */
	if ((prefixStr[0] == 1) && (prefixStr[1] == ' '))
		prefixStr[0] = 0;

	err = BuildNicknameTable (prefixStr, GetAliasFileToScan (pte), showExpansions, true, &nicknameTable);
	if (err)
		goto Exit0;

	fontNum = FontID;
	fontSize = FontSize;
	GetFontName(fontNum, &scratchStr);
	if (!scratchStr[0])
		fontNum = kFontIDGeneva;
	if (fontSize < 4)
		fontSize = 9;

	TruncateNicknamePopupItems (nicknameTable, fontNum, fontSize, (*PeteExtra(pte))->win);

	err = NewStickyPopup(fontNum, fontSize, &popup);
	if (err)
		goto Exit1;

	HLock((Handle)nicknameTable);
	tablePtr = *nicknameTable;
	err = AddEntriesToStickyPopup(popup, &tablePtr->table[0].entryStr, sizeof(NicknameTableEntry), tablePtr->numEntries, -1);
	if (err)
		goto Exit2;
	HUnlock((Handle)nicknameTable);

	CalcNicknamePopupLoc (pte, &popupLoc, &teflon);
	PushCursor(arrowCursor);
	result = StickyPopupSelect(popup, popupLoc.v, popupLoc.h, 1, true, &teflon);
	PopCursor();
	if (result)
	{
		entryPtr = &((**nicknameTable).table[result - 1]);
		*resultAliasIndex = entryPtr->aliasIndex;
		*resultNicknameIndex = entryPtr->nicknameIndex;
	}


Exit2:
	DisposeStickyPopup(popup);
Exit1:
	DisposeNicknameTable(nicknameTable);
Exit0:
	if (err)
	{
		SysBeep(5);
		return false;
	}
	return result ? true : false;
}


#pragma mark ========== Nickname expansion routines ==========


/************************************************************************
 * ExpandAliases - take an address list (as from SuckAddresses), and
 * expand it using the alias list
 ************************************************************************/
OSErr ExpandAliases(BinAddrHandle *addresses,BinAddrHandle fromH,short depth,Boolean wantComments)
{
	Str255 autoQual;
	EAL_VARS_DECL;
	
	GetPref(autoQual,PREF_AUTOQUAL);
	return(ExpandAliasesLow(addresses,fromH,depth,wantComments,autoQual,EAL_VARS));
}

	
OSErr ExpandAliasesLow(BinAddrHandle *addresses,BinAddrHandle fromH,short depth,Boolean wantComments,PStr autoQual,EAL_VARS_FORM)
{
	int err=0;
	BinAddrHandle toH,spewHandle;
	long offset;
	long	theHandleSize = fromH ? GetHandleSize_(fromH):0;
	Handle	theExpansion = nil;
	Boolean group;
	short which,index;
	Boolean nameEmpty = false;
	Boolean isGroup = false;
	short	tempSize;
	
	if (++depth > MAX_DEPTH)
	{
		GetRString(tempStr,ALIA_LOOP);
		PCopy(junk,*fromH);
		MyParamText(tempStr,junk,"","");
		(void) ReallyDoAnAlert(OK_ALRT,Stop);
		if (addresses != nil)
			*addresses = nil;
		return(1);
	}

	if (!(err=RegenerateAllAliases(false)))
	{
		toH = NuHTempBetter(0L);
		if (!toH)
			err=MemError();
		else
		{
			for (offset=0;!err && offset < theHandleSize && (*fromH)[offset]; offset += (*fromH)[offset]+2)
			{
				/*
				 * get rid of the extraneous stuff
				 */
				if (err=SuckPtrAddresses(&spewHandle,LDRef(fromH)+offset+1,(*fromH)[offset],False,True,False,nil)) break;
				UL(fromH);
				if (spewHandle)
				{
					LDRef(spewHandle);
					if (group = (*spewHandle)[**spewHandle]==':')
					{
						err=PtrPlusHand_((*fromH)+offset,toH,(*fromH)[offset]+2);
						if (err) break;
						theExpansion = nil;
					}
					else
					{
						theExpansion = FindNickExpansionFor(*spewHandle,&which,&index);
						
						if (CompareRawToExpansion (spewHandle, theExpansion))
							theExpansion = nil;
								
						//	Don't expand if group syntax, ALB 10/8/96
						if (theExpansion && GetHandleSize_(theExpansion) == **spewHandle)
						{
							Handle tempExpansionHandle = nil,tempSpewHandle = nil;
							long	expansionSize = GetHandleSize_(theExpansion);
							long	spewSize = **spewHandle;
							Byte theEndChar = '\0';
							tempExpansionHandle = NuHandle(expansionSize);
							tempSpewHandle = NuHandle(spewSize );
							if (!tempExpansionHandle || ! tempSpewHandle)
								goto breakTheLoop;
							BlockMoveData(*theExpansion,*tempExpansionHandle,expansionSize);
							BlockMoveData(*spewHandle + 1,*tempSpewHandle,spewSize);
							if (err=PtrPlusHand(&theEndChar,tempExpansionHandle,1)) 						
							{
									ZapHandle(tempExpansionHandle);
									ZapHandle(tempSpewHandle);
									goto breakTheLoop;
							}
							if (err=PtrPlusHand(&theEndChar,tempSpewHandle,1)) 
							{
									ZapHandle(tempExpansionHandle);
									ZapHandle(tempSpewHandle);
									goto breakTheLoop;
							}


							if (striscmp(*tempExpansionHandle,*tempSpewHandle) == 0)
							{
								err=PtrPlusHand_((*fromH)+offset,toH,(*fromH)[offset]+2);
								ZapHandle(spewHandle);
								UL(theExpansion);
								ZapHandle(tempExpansionHandle);
								ZapHandle(tempSpewHandle);
								goto breakTheLoop;
								}

								ZapHandle(tempExpansionHandle);
								ZapHandle(tempSpewHandle);
						}
					}
				}
				ZapHandle(spewHandle);
				if (theExpansion)
				{
					BinAddrHandle lookup = nil,result = nil;
					err = SuckPtrAddresses(&lookup,LDRef(theExpansion),
										 GetHandleSize_(theExpansion),wantComments,True,False,nil);
					UL(theExpansion);

					if (lookup)
					{
						/*
						* now, expand the addresses in the expansion
						*/
						if (err=ExpandAliasesLow(&result,lookup,depth,wantComments,autoQual,EAL_VARS)) break;
						isGroup = HandleEndsWithR(fromH,GROUP_DONT_HIDE);
						ZapHandle(lookup);
						
						/*
						 * add the expanded aliases to the new list
						 */
						if (!err && result)
						{
							SetHandleBig_(result,GetHandleSize_(result)-1);
							HLock(result);
							err = MemError();
							tempSize = GetHandleSize_(result);
							if (err)
								break;
							
							// Check to see if we have a group of nicknames or just a single one.
							if (!isGroup && tempSize > *result[0] + 2)
								isGroup = true;

							GetRString(junk,NickManKeyStrn+1); // Read in tag for Name field

							// Get data from "name" field.
							NickGetDataFromField(junk,tempStr,which,index,false,false,&nameEmpty); // Go snag data from the notes field
							
							// If the name field was empty, join the first and last names to form the name
							if (nameEmpty) {
								Str255	firstName,
												lastName,
												tag;
								
								GetTaggedFieldValueStr (which, index, GetRString (tag, ABReservedTagsStrn + abTagFirst), firstName);
								GetTaggedFieldValueStr (which, index, GetRString (tag, ABReservedTagsStrn + abTagLast), lastName);

								JoinFirstLast (tempStr, firstName, lastName);
								if (*tempStr)
									nameEmpty = false;
							}
							
							// Only add the name field if we have a name and want comments.
							if (*tempStr && !isGroup && !nameEmpty && wantComments) // We just have a single name
							{
								ShortAddr(shortAddress,LDRef(result));
								UL(result);
								CanonAddr(buffer,shortAddress,tempStr);
								err = PtrPlusHand(buffer,toH,*buffer + 1);
								if (err)
									break;
								tempStr[0] = 0; // Tack on a null
								err = PtrPlusHand(tempStr,toH,1);
								if (err)
									break;
							}
							
							// Only add the name field if we're at the highest expansion level and we have a name
							// and we want comments.
							// I don't believe that group syntax within group syntax is valid
							else if (*tempStr && isGroup && !nameEmpty && depth == 1 && wantComments) // We have a list of names
							// Put into Name: addresses; format (group syntax)
							{
								Quote822(junk,tempStr,true);
								PCopy(tempStr,junk);

								PCat(tempStr,"\p:");
								err = PtrPlusHand(tempStr,toH,*tempStr + 1);
								if (err)
									break;
								*tempStr = 0;
								PtrPlusHand(tempStr,toH,1);
								PCopy(tempStr,"\p;");
								err = HandAndHand(result,toH);
								if (err)
									break;
								err = PtrPlusHand(tempStr,toH,*tempStr + 1);
								if (err)
									break;
								*tempStr = 0;
								err = PtrPlusHand(tempStr,toH,1); // Tack on null;
								if (err)
									break;								
							}
							else
									{
										err = HandAndHand(result,toH);
										if (err)
											break;
									}

							
						}
						else if (!err)
							{
								err = MemError();
								if (err)
									break;
							}
					}
							UL(result);
							ZapHandle(result);

				}
				else if (!group)
				{
					//Str255 temp;

					/*
					 * original was NOT an alias; just copy it
					 */
					/* if it is "me" then the user has not defined me as a nickname.
						 just put in the return address. */
					LDRef(fromH);
					if (EqualStrRes((*fromH)+offset,ME))
					{
						GetReturnAddr(tempStr, wantComments);
						if (!wantComments)	// strip <>'s
						{
							if (*tempStr && tempStr[1]=='<')
							{
								--*tempStr;
								BMD(tempStr+2,tempStr+1,*tempStr+1);
							}
							if (*tempStr && tempStr[*tempStr]=='>')
							{
								tempStr[*tempStr] = 0;
								--*tempStr;
							}
						}
						err = PtrPlusHand_(tempStr, toH, tempStr[0]+2);
					}
					else if (*autoQual)
					{
						BinAddrHandle qualified;
						if (err = SuckPtrAddresses(&qualified,(*fromH)+offset+1,(*fromH)[offset],True,False,True,nil)) break;
						err = PtrPlusHand_(LDRef(qualified),toH,**qualified+2);
						ZapHandle(qualified);
					}
					else
						err=PtrPlusHand_((*fromH)+offset,toH,(*fromH)[offset]+2);
					if (err)
						break;
					HUnlock(fromH);
				}
				breakTheLoop: ;
			}
		}
	}
	if (!err)
	{
		SetHandleBig_(toH,GetHandleSize_(toH)+1);
		if (!(err=MemError())) (*toH)[GetHandleSize_(toH)-1] = 0;
	}
	if (err)
	{
		ZapHandle(toH);
		if (err!=1) WarnUser(ALLO_EXPAND,err);
	}
	if (addresses != nil)
		*addresses = toH;
	return(err);
}


#pragma mark ========== Message parsing routines ==========


/* MJN *//* new routine */
/* FINISH *//* add comments, make static */
/* WARNING *//* may move or purge memory *//* FINISH *//* really? */
/* FINISH *//* all callers need to check to see if we hit the end of the text and/or field */
long FindNextRecipientStart(Handle textHdl, long startOffset)
{
	long textLen;
	Str255 delimiterList;
	UPtr curDelim;
	long delimCount;
	Boolean hitDelimiter;
	UPtr curChar;
	long curOffset;


	BlockMoveData("\p,:;", delimiterList, 4); /* FINISH *//* use resource */
	delimiterList[++delimiterList[0]] = returnChar;

	textLen = GetHandleSize(textHdl); /* FINISH *//* just make this the end of that recipient field, or use PETEGetTextLen */

	/* Go forward until we hit a non-space character.  If that character is a recipient delimiter, then go on to the next
			non-space character following that delimiter (which may be another delimiter).  If at any point we hit the end
			of the text block, stop and return the end offset. */
	curOffset = startOffset;
	curChar = *textHdl + curOffset;

	/* FINISH *//* in all spots, change this to check curOffset first, so that we don't read bytes past the end of a block */
	while ((*curChar == ' ') && (curOffset < textLen))
	{
		curChar++;
		curOffset++;
	}
	/* FINISH *//* make sure we do this check anywhere in the code where this comes up */
	if (curOffset >= textLen)
		return curOffset;

	hitDelimiter = false;
	delimCount = delimiterList[0];
	curDelim = delimiterList + 1;
	while (!hitDelimiter && delimCount--)
	{
		if (*curChar == *curDelim)
			hitDelimiter = true;
		else
			curDelim++;
	}
	if (!hitDelimiter)
		return curOffset;

	curChar++;
	curOffset++;

	while ((*curChar == ' ') && (curOffset < textLen))
	{
		curChar++;
		curOffset++;
	}

	return curOffset;
}


/* MJN *//* new routine */
/* FINISH *//* add comments, make static */
/* WARNING *//* may move or purge memory *//* FINISH *//* really? */
/* FINISH *//* all callers need to check to see if we hit the end of the text and/or field */
/* FINISH *//* we must return startOffset if errors occur */
long FindRecipientStartInText(Handle textHdl, long startOffset, Boolean skipLeadingSpaces)
{
	UPtr textPtr;
	Str255 delimiterList;
	UPtr curDelim;
	long delimCount;
	Boolean hitDelimiter;
	UPtr curChar;
	long curOffset;


	BlockMoveData("\p,:;", delimiterList, 4); /* FINISH *//* use resource */
	delimiterList[++delimiterList[0]] = returnChar;

	textPtr = *textHdl;

	/* Back up until we hit a character that's a recipient delimiter, or we hit the first character in the text block.
			If we've hit a colon, it may be the colon following the label of a recipient field (i.e. the colon in "To:"), or it
			may be the colon in a group syntax entry (i.e. "GroupName:addr1,addr2,addr3;"). */
	curOffset = startOffset - 1;
	curChar = textPtr + curOffset;
	hitDelimiter = false;
	while (!hitDelimiter && (curOffset >= 0))
	{
		delimCount = delimiterList[0];
		curDelim = delimiterList + 1;
		while (!hitDelimiter && delimCount--)
		{
			if (*curChar == *curDelim) /* FINISH *//* spots like this do continous dereferencing of *curChar */
				hitDelimiter = true;
			else
				curDelim++;
		}
		if (hitDelimiter)
			continue;

		curChar--;
		curOffset--;
	}
	curChar++;
	curOffset++;

	if (skipLeadingSpaces)
		while ((*curChar == ' ') && (curOffset < (startOffset - 1)))
		{
			curChar++;
			curOffset++;
		}

	return curOffset;
}


/* MJN *//* new routine */
/* FINISH *//* add comments, make static */
/* WARNING *//* may move or purge memory *//* FINISH *//* really? */
/* FINISH *//* all callers need to check to see if we hit the end of the text and/or field */
/* FINISH *//* we must return startOffset if errors occur */
long FindRecipientEndInText(Handle textHdl, long startOffset, Boolean skipTrailingSpaces)
{
	long textLen;
	UPtr textPtr;
	Str255 delimiterList;
	Str15 prefix;
	UPtr curDelim;
	long delimCount;
	Boolean hitDelimiter;
	Boolean	isFCC;
	UPtr curChar;
	long curOffset;

	GetRString(prefix,FCC_PREFIX);
	
	BlockMoveData("\p,:;", delimiterList, 4); /* FINISH *//* use resource */
	delimiterList[++delimiterList[0]] = returnChar;

	textLen = GetHandleSize(textHdl); /* FINISH *//* just make this the end of that recipient field, or use PETEGetTextLen */
	textPtr = *textHdl;

	/* Go forward until we hit a character that's a recipient delimiter, or we hit the last character in the text block. */
	curOffset = startOffset;
	curChar = textPtr + curOffset;
	hitDelimiter = false;
	isFCC = false;
	while (!hitDelimiter && (curOffset < textLen))
	{
		delimCount = delimiterList[0];
		curDelim = delimiterList + 1;
		while (!hitDelimiter && delimCount--)
		{
			// jp - Check to see if we're processing an Fcc -- denoted by "�:  (actually, using the FCC_PREFIX)
			if (!isFCC && *curChar == 0x22)
				if (curOffset + prefix[0] + 1 < textLen)
					if (!memcmp (curChar + 1, &prefix[1], prefix[0]) && *(curChar + prefix[0] + 1) == ':')
						isFCC = true;

			// jp - Ignore colons as long as we are processing an Fcc
			if (*curChar == *curDelim) {
				if (!isFCC || (isFCC && *curChar != ':'))
					hitDelimiter = true;
			}
			else
				curDelim++;
		}
		if (hitDelimiter)
			continue;

		curChar++;
		curOffset++;
	}
	curChar--;
	curOffset--;

	if (skipTrailingSpaces)
		while ((*curChar == ' ') && (curOffset > startOffset))
		{
			curChar--;
			curOffset--;
		}

	curOffset++;

	// For safety...
	if (curOffset > textLen)
		curOffset = textLen;

	return curOffset;
}


/* MJN *//* new routine */
/* FINISH *//* add comments, make static */
/* WARNING *//* may move or purge memory */
/* FINISH *//* all callers need to check to see if we hit the end of the text and/or field */
long FindNicknameStartInText(Handle textHdl, long startOffset, Boolean recipientField)
{
	UPtr textPtr;
	Str255 delimiterList;
	UPtr curDelim;
	long delimCount;
	Boolean hitDelimiter;
	UPtr curChar;
	long curOffset;
	Size textHdlSize;

	if (!textHdl)
		return (0);
		
	textHdlSize = GetHandleSize (textHdl);
	if (!textHdlSize)
		return (0);
	
	GetRString(delimiterList, ALIAS_VERBOTEN); /* characters forbidden in a nickname */
	/* WARNING *//* assumes delimiterList[0] <= 253 */
	delimiterList[++delimiterList[0]] = returnChar;
	if (!recipientField)
		delimiterList[++delimiterList[0]] = ' '; /* don't allow spaces in nicknames if we're not in a recipient field */

	textPtr = *textHdl;

	/* Back up until we hit a character that's not allowable in a nickname, or we hit the first character in the text block.
			If we've hit a colon, it may be the colon following the label of a recipient field (i.e. the colon in "To:"), or it
			may be the colon in a group syntax entry (i.e. "GroupName:addr1,addr2,addr3;"). */
	curOffset = startOffset - 1;
	curChar = textPtr + curOffset;
	hitDelimiter = false;
	while (!hitDelimiter && (curOffset >= 0))
	{
		delimCount = delimiterList[0];
		curDelim = delimiterList + 1;
		while (!hitDelimiter && delimCount--)
		{
			if (*curChar == *curDelim)
				hitDelimiter = true;
			else
				curDelim++;
		}
		if (hitDelimiter)
			continue;

		curChar--;
		curOffset--;
	}
	curChar++;
	curOffset++;

	if (recipientField)
		while ((*curChar == ' ') && (curOffset < (startOffset - 1) && curOffset < textHdlSize))
		{
			curChar++;
			curOffset++;
		}

	return curOffset;
}



/* MJN *//* new routine */
/* FINISH *//* finish comments */
/************************************************************************
 * GetNicknamePrefixFromField - given the window passed in win, this
 * function gets the characters typed so far in the current PETE field,
 * and returns them in prefixStr.  This returned string is appropriate
 * for doing nickname completion.
 ************************************************************************/
OSErr GetNicknamePrefixFromField (PETEHandle pte, Str255 prefixStr, Boolean ignoreSelection, Boolean useNicknameTypeAhead)
{
	OSErr err;
	Handle textHdl;
	long selStart, selEnd;
	long startOffset;
	long prefixLen;
	Boolean recipientField;

	/* FINISH *//* need special handling if selection was created by type-ahead */
	/* FINISH *//* if ignoreSelection is false, then it should return prefix + selection, and we can remove the param useNicknameTypeAhead */
	/* FINISH *//* is anyone even calling this with useNicknameTypeAhead set to true?  How about ignoreSelection set to false? */

	prefixStr[0] = 0;

	if (!PeteIsValid(pte))
		return paramErr;

	if (useNicknameTypeAhead && HasNickCompletion (pte) && CurSelectionIsTypeAheadText (pte))
	{
		BlockMoveData (gTypeAheadPrefix, prefixStr, gTypeAheadPrefix[0] + 1);
		BlockMoveData (gTypeAheadSuffix, prefixStr + prefixStr[0] + 1, gTypeAheadSuffix[0]);
		prefixStr[0] += gTypeAheadSuffix[0];
		return noErr;
	}

	err = PeteGetTextAndSelection(pte, &textHdl, &selStart, &selEnd);
	if (err)
		return err;
	if (ignoreSelection)
		selEnd = selStart;

	if (selStart != selEnd) /* if a range of text is selected */
	{
		prefixLen = selEnd - selStart;
		if (prefixLen > 255)
			prefixLen = 255;
		BlockMoveData(*textHdl + selStart, prefixStr + 1, prefixLen);
		prefixStr[0] = prefixLen;
		return noErr;
	}

	recipientField = HasNickSpaces (pte);
	if (IsCompWindow(GetMyWindowWindowPtr((*PeteExtra(pte))->win)) && IsAddressHead(CompHeadCurrent(pte)))
		recipientField = true;
	startOffset = FindNicknameStartInText(textHdl, selStart, recipientField);

	prefixLen = selStart - startOffset;
	if (!prefixLen)
		return noErr;
	if (prefixLen > 255)
		prefixLen = 255;
	BlockMoveData(*textHdl + startOffset, prefixStr + 1, prefixLen);
	prefixStr[0] = prefixLen;

	return noErr;
}


#pragma mark ========== Finish nickname routines ==========


/* MJN *//* new routine */
/************************************************************************
 * FinishAliasUsingTypeAhead - complete a partially typed nickname; for
 * use in cases where nickname type-ahead has already done some work.
 *
 * Pass the window which contains the text to be finished in win; this
 * must be a composition window, and the current selection must be in
 * one of the recipient fields.
 *
 * If you pass true for wantExpansion, then rather than putting in the
 * nickname, this routine will enter the full expansion of the nickname.
 * If you pass true for allowPopup and there are two or more nicknames
 * ending with the current nickname prefix, then the nickname sticky
 * popup menu will come up and allow the user to specify which nickname
 * they want to use.  If you pass true for insertComma, then a comma
 * will be inserted following the inserted text if it is appropriate
 * to do so.
 *
 * This function is specifically for completing a nickname when a
 * nickname type-ahead selection exists in the specified window
 * (i.e. CurSelectionIsTypeAheadText(win) returns true).  For general
 * nickname completion, you should call FinishAlias.
 *
 * The function returns noErr if it succeeds, or a non-zero error code
 * if it fails.
 ************************************************************************/

OSErr FinishAliasUsingTypeAhead (PETEHandle pte, Boolean wantExpansion, Boolean allowPopup, Boolean insertComma)

{
	MyWindowPtr	win;
	WindowPtr		winWP;
	Str255			nicknameStr;
	UHandle 		recipientText;
	Handle			text;
	Ptr					insertTextPtr;
	OSErr				theError;
	long				insertTextLen,
							replaceStart,
							replaceEnd;
	short				aliasIndex,
							nicknameIndex;

	if (!PeteIsValid(pte))
		return (paramErr);

	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return (false);
	
	if (!CurSelectionIsTypeAheadText (pte))
		return (paramErr);

	aliasIndex		= GetTypeAheadAliasIndex (pte);
	nicknameIndex = GetTypeAheadNicknameIndex (pte);

	if (allowPopup && NicknamePrefixOccursNTimes (gTypeAheadPrefix, 2, GetAliasFileToScan (pte)))
		if (!DoNicknameStickyPopup (pte, wantExpansion, gTypeAheadPrefix, &aliasIndex, &nicknameIndex))
			return (noErr);

	if (theError = RegenerateAllAliases (false)) /* FINISH *//* shouldn't we be passing in true? */
		return (theError);

	//	This code used to be executed well down in the code.  Trouble is, gTypeAheadPrefix is cleared
	//	whenever a nickname file is saved... for instance, when nicknames are cached.  As a result, the
	//	insertion target for nicknames that were cached and tthe completed was incorrect.
	replaceStart = GetTypeAheadSelStart (pte) - gTypeAheadPrefix[0];
	replaceEnd	 = GetTypeAheadSelEnd (pte);

	recipientText = nil;
	if (wantExpansion) {
		if (theError = GetNicknameRecipientText (pte, aliasIndex, nicknameIndex, &recipientText))
		return (theError);

		insertTextLen = GetHandleSize (recipientText);
		
		// If there is no expansion present, we're done...  Unless!!  We're also watching for either completion or hiliting...
		if (!insertTextLen) {
			if ((gNicknameTypeAheadEnabled && HasNickCompletion (pte)) || (gNicknameHilitingEnabled && HasNickHiliting (pte)))
				wantExpansion = false;
			else
				{ ZapHandle(recipientText); return (noErr); }
		}
	}
	// (jp) Note that this is the same case as above, but required since we might
	//			turn off 'wantExpansion' if there was no expansion
	if (wantExpansion) {
		if (insertComma) {
			SetHandleSize (recipientText, ++insertTextLen);
			if (theError = MemError ()) {
				DisposeHandle ((Handle) recipientText);
				return (theError);
			}
			*(*recipientText + insertTextLen - 1) = ',';
		}

		MoveHHi ((Handle) recipientText);
		HLock ((Handle) recipientText);
		insertTextPtr = *recipientText;
	}
	else {
		GetNicknameNamePStr (aliasIndex, nicknameIndex, nicknameStr);
		insertTextPtr = nicknameStr + 1;
		insertTextLen = nicknameStr[0];

		if (insertComma) {
			if (insertTextLen < 255) {
				nicknameStr[++nicknameStr[0]] = ',';
				++insertTextLen;
			}
			else {
				ZapHandle(recipientText);
				recipientText = NewHandle (++insertTextLen);
				if (!recipientText)
					return (MemError ());
				BlockMoveData (insertTextPtr, *recipientText, insertTextLen - 1);
				*(*recipientText + insertTextLen - 1) = ',';
				MoveHHi ((Handle) recipientText);
				HLock ((Handle) recipientText);
				insertTextPtr = *recipientText;
			}
		}
	}

	/* need to grab these values before calling ResetNicknameTypeAhead */
//
//	We are now grabbing these values ealier in the code... By this point
//	the Type Ahead Prefix has been cleared by the nickname caching code.
//
//	replaceStart = GetTypeAheadSelStart (pte) - gTypeAheadPrefix[0];
//	replaceEnd	 = GetTypeAheadSelEnd (pte);

	theError = PETEClearUndo (PETE, pte);

	// (jp) Turn off the highlighting before substituting text
	//			...but only if the text being finished is an expansion
	if (!theError && wantExpansion && insertTextLen)
		theError = SetNicknameHiliting (pte, replaceStart, replaceEnd, false);

	win = (*PeteExtra(pte))->win;
	winWP = GetMyWindowWindowPtr (win);
	if (!theError && gNicknameTypeAheadEnabled && HasNickCompletion (pte))
		PeteSelect (win, pte, replaceStart, replaceEnd);
	
	if (!theError)
		(void) PeteInsertPtr (pte, kPETECurrentSelection, insertTextPtr, insertTextLen);
	
	ResetNicknameTypeAhead (pte);
	NicknameWatcherModifiedField (pte);

	if (wantExpansion && gNicknameCacheEnabled)
		if ((GetWindowKind(winWP) == COMP_WIN || GetWindowKind(winWP) == MESS_WIN))
			CompGatherRecipientAddresses (Win2MessH (win), true);
		else
			if (!PeteGetTextAndSelection (pte, &text, nil, nil))
				SetNickCacheAddresses (pte, text);

	ZapHandle (recipientText);

	return (theError);
}

/* MJN *//* FINISH *//* should indices be short or long? */

/************************************************************************
 * FinishAlias - finish a partially completed alias
 ************************************************************************/
void FinishAlias (PETEHandle pte, Boolean wantExpansion, Boolean allowPopup, Boolean dontUndo)
{
	Str63		lcd,
					tempStr,
					word;
	Str31		nameStr;
	Handle	text;
	UPtr		spot,
					begin,
					end;
	OSErr		theError;
	long		offset,
					index,
					start,
					stop,
					pStart;
	short		found,
					foundIndex,
					whichAlias,
					foundAlias;

	if (!PeteIsValid(pte))
		return;
	
	found			 = 0;
	foundIndex = 0;
	foundAlias = -1;
	
	/* MJN *//* we need to do special-handling if the selection was created by nickname type-ahead */
	if (CurSelectionIsTypeAheadText (pte)) {
		(void) FinishAliasUsingTypeAhead (pte, wantExpansion, allowPopup, false);
		return;
	}

	if (PeteGetTextAndSelection (pte, &text, &start, &stop))
		return;

	begin = *text;
	// Is there a selection?
	if (start != stop) {
		spot = begin + start;
		end	 = begin + stop - 1;
	}
	else
		// Is it an address field?
		if ((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)) {
			spot = end = begin + start - 1;
			while (spot >= begin && *spot != ',' && *spot != '(' && *spot != ':')
				spot--;
			spot++;
			while (*spot == ' ' && spot < end)
				spot++;
			if (spot > end)
				return;
		}
		else {
		// plain old
			spot = end = begin + start - 1;
			while (spot >= begin && *spot > ' ' && *spot != ',' && *spot != '(')
				spot--;
			spot++;
			if (spot > end)
				return;
		}
	*word = MIN (sizeof (word) - 1, end - spot + 1);
	BlockMoveData (spot, word + 1, *word);
	TrimAllWhite (word);
	if (!*word)
		return;
	
	if (RegenerateAllAliases (false))
		return;
	
	found = 0;
	for (whichAlias = 0; whichAlias < NAliases; whichAlias++) {
		offset = 0;
		while ((index = FindAPrefix (whichAlias, word, offset)) >= 0) {
			if (!found) {
				foundAlias = whichAlias;
				GetNicknameNamePStr (whichAlias, index, lcd);
				if (theError = MemError ()) {
					WarnUser (MEM_ERR, theError);
					return;
				}
				foundIndex = index;
			}
#ifdef TWO
			else {
				GetNicknameNamePStr (whichAlias, index, tempStr);
				LCD (lcd, tempStr);
			}
			/* MJN *//* FINISH *//* I think this next bit was supposed to follow the call to GetNicknameNamePStr */
			if (theError = MemError()) {
				WarnUser (MEM_ERR, theError);
				return;
			}
#endif
			/* MJN *//* FINISH *//* what should this do if we're bringing up the nickname sticky popup? */
			if (found == 1 && !allowPopup)	 // More than one matched.
				SysBeep (20);
			++found;
			offset = index + 1;
			if (EqualString (lcd, word, False, True))
				goto out;
		}
	}

out:

/* WARNING */ /* by this time, text (a Handle) may have been relocated, so the actual values
								 of begin, end, and spot may not point to the proper address; however, it is
								 safe to use them for the purposes of calculating offsets, as is done below. */

	// Only one match
	if (found == 1) {
		if (!dontUndo)
			PetePrepareUndo (pte, peUndoPaste, spot - begin, end - begin + 1, &pStart, nil);
		PeteSelect (nil, pte, spot - begin, end - begin + 1);
		GetNicknameNamePStr (foundAlias, foundIndex, nameStr);
		InsertAlias (pte, nil, nameStr, wantExpansion, spot - begin, dontUndo);
	}
#ifdef TWO
	/* MJN *//* run nickname sticky popup menu */
	else
		if (found > 1 && allowPopup) {
			if (DoNicknameStickyPopup (pte, wantExpansion, word, &foundAlias, &foundIndex)) {
				if (!dontUndo)
					PetePrepareUndo (pte, peUndoPaste, spot - begin, end - begin + 1, &pStart, nil);
				PeteSelect (nil, pte, spot - begin, end - begin + 1);
				GetNicknameNamePStr (foundAlias, foundIndex, nameStr);
				InsertAlias (pte, nil, nameStr, wantExpansion, spot - begin, dontUndo);
			}
		}
		else
			if (found > 1 && *lcd) {
				if (!dontUndo)
					PetePrepareUndo (pte, peUndoPaste, spot - begin, end - begin + 1, &pStart, nil);
				PeteSelect (nil, pte, spot - begin, end - begin + 1);
				PeteInsertPtr (pte, -1, lcd + 1, *lcd);
				if (!dontUndo)
					PeteFinishUndo (pte, peUndoPaste, pStart, -1);

				NicknameWatcherModifiedField (pte); /* MJN */
			}
#endif
}

/************************************************************************
 * FindAPrefix - find a prefix in a list of aliases
 ************************************************************************/
long FindAPrefix(short which,PStr word,long startIndex)
{
	Str63 local,tempStr;	//	ALB 9/10/96, can't use Str31 because LCD adds a null to the end
	long stop;
	long i;

	if (!(*Aliases)[which].theData)
		return (-1);
		
	stop = (GetHandleSize_((*Aliases)[which].theData)/sizeof(NickStruct));
	if (startIndex >= stop)
		return (-1);

	PSCopy(local,word);
	if (local[*local]==';' && local[*local-1]==':') *local -= 2;
	

	for (i=startIndex;i<stop;i++)
	{
			if (!(*((*Aliases)[which].theData))[i].deleted)
			{
				GetNicknameNamePStr(which,i,tempStr);
				if (*tempStr && EqualString(LCD(tempStr,local),local,false,false) && !(*((*Aliases)[which].theData))[i].deleted)
					return (i);
			}
	}

	return(-1);

}

/************************************************************************
 * InsertAlias - insert an alias in a window
 ************************************************************************/
void InsertAlias (PETEHandle pte, HSPtr hs, PStr nameStr, Boolean wantExpansion, long pStart, Boolean dontUndo)

{
	PETEStyleEntry	pse;
	MyWindowPtr			win;
	WindowPtr				winWP;
	PersHandle			pers;
	BinAddrHandle		wordH,
									list;
	Handle					text;
	long						spot,
									tempSize,
									addressSize,
									selStart,
									selEnd;
	short						len;

	if (!PeteIsValid(pte))
		return;

	win	 = (*PeteExtra(pte))->win;
	winWP = GetMyWindowWindowPtr (win);
	spot = hs ? hs->stop : -1;
	
	// If we don't want the nickname expansion, just copy in the alias name
	if (!wantExpansion) { 
		len = *nameStr;
		
		if (HasNickHiliting (pte) && gNicknameHilitingEnabled)
			if (!PeteGetTextAndSelection (pte, nil, &selStart, &selEnd))
				(void) SetNicknameHiliting (pte, selStart, selEnd, true);
		
		// (jp) 1/24/00	This is a temporary fix for the problem that "turns off" spell checking
		//							following expansion of nicknames that contain a space.  The spell scanner
		//							keys on the space as the end of spell checking -- though the syle is not
		//							yet complete.  For now I'm commenting out the call to PeteInsertPtr and
		//							doing it normally while turning off the spell bit.
		// (void) PeteInsertPtr (pte, spot, nameStr + 1, len);
		PeteStyleAt (pte, spot, &pse);
		pse.psStyle.textStyle.tsLock	= 0;
		pse.psStartChar								= 0;
		pse.psStyle.textStyle.tsLabel	&= ~pSpellLabel;
		**Pslh = pse;
		(void) PETEInsertTextPtr(PETE,pte,spot,nameStr + 1,len,Pslh);
		// End of hack.
		
		if (!dontUndo)
			(void) PeteFinishUndo (pte, peUndoPaste, pStart, spot == -1 ? kPETECurrentSelection : spot + len);

		if (hs)
			hs->stop += len;
	}
	else {
		addressSize = *nameStr + 1;
		wordH = NuHandle (addressSize + 5);
		if (!wordH) {
			WarnUser (MEM_ERR, MemError ());
			return;
		}
		else {
			BlockMoveData (nameStr, *wordH, nameStr[0] + 1);
			(*wordH)[**wordH + 1] = (*wordH)[**wordH + 2] = 0;

			pers = PersList;
			if ((GetWindowKind(winWP)==COMP_WIN ||
					 GetWindowKind(winWP)==MESS_WIN))
				pers = PERS_FORCE (MESS_TO_PERS (Win2MessH (win)));
			PushPers (pers);

			list = nil;
			PeteExpandAliases (pte, &list, wordH, 0, true);
			PopPers ();
			ZapHandle(wordH);
			
			// (jp) Bad ju-ju ahead... if the list is zero length, we correctly do not replace the existing typing
			//			with nothing (what would be the point?), but we incorrectly lose the caret.
			if (list) {
				tempSize = GetHandleSize_(list);
				if (tempSize == 1 && strlen (LDRef (list)) == 0) {
					ZapHandle(list);
					// ... and here's how we're fixing the ju-ju.  Basically, set the selection to the end and finish the undo.
					(void) PeteGetTextAndSelection (pte, nil, nil, &selEnd);
					PeteSelect (nil, pte, selEnd, selEnd);
					if (!dontUndo)
						PeteFinishUndo (pte, peUndoPaste, pStart, spot == -1 ? kPETECurrentSelection : spot);
				}
				else
					UL(list);
			}
			
			if (list) {
				CommaList (list);
				
				if (wantExpansion)
					if (!PeteGetTextAndSelection (pte, nil, &selStart, &selEnd))
						(void) SetNicknameHiliting (pte, selStart, selEnd, false);
				
				len = GetHandleSize (list);
				PeteInsert (pte, spot, list);
				if (!dontUndo)
					PeteFinishUndo (pte, peUndoPaste, pStart, spot == -1 ? kPETECurrentSelection : spot + len);
				if (hs)
					hs->stop += len;
				ZapHandle (list);
			}
			if (gNicknameCacheEnabled)
				if ((GetWindowKind(winWP) == COMP_WIN || GetWindowKind(winWP) == MESS_WIN))
					CompGatherRecipientAddresses (Win2MessH (win), true);
				else
					if (!PeteGetTextAndSelection (pte, &text, nil, nil))
						SetNickCacheAddresses (pte, text);
		}
	}
	// (jp) We only worry about this if we are given a HeaderSpec... And we're only given a
	//			HeaderSpec when we're inserting an address into a composition window.  So...
	//			we don't worry about other types of windows here.  (Still, let's check for safety)
	if (hs && GetWindowKind(winWP) == COMP_WIN)
		if (CompHeadCurrent (pte) == hs->index)
			PeteSelect (win, pte, hs->stop, hs->stop);
	NicknameWatcherModifiedField (pte); /* MJN */
}

/**********************************************************************
 * IsNickname - is something a nickname?
 **********************************************************************/
Boolean IsNickname(PStr name,short which)
{
	Boolean result = false;
	
	if (!(*Aliases)[which].theData) return false;

	LDRef((Handle)(*Aliases)[which].theData);
	if (NickMatchFound((*Aliases)[which].theData,NickHash(name),name,which)>=0)
		result = true;
	else
		result = false;

	UL((Handle)(*Aliases)[which].theData);
		
	return result;
}

/************************************************************************
 * FindNickExpansionLo - find the first expansion that matches with plugin specification
 ************************************************************************/
short FindNickExpansionForLo(UPtr name,long hash,short *theWhich,Boolean pluginNicks)
{
	short index;
	short	which;
	
	for (which=0;which<NAliases;which++)
	{
		if (pluginNicks == IsPluginAddressBook (which))
		{
			index = NickMatchFound(((*Aliases)[which].theData),hash,name,which);
			if (index >= 0)
			{
				*theWhich = which;
				return index;
			}
		}
	}
	return -1;	//	Not found
}

/************************************************************************
 * FindNickExpansionFor - find the first expansion that matches
 ************************************************************************/
Handle FindNickExpansionFor(UPtr name,short *theWhich,short *theIndex)
{
	short index = -1;
	long	hash;
	
	*theWhich = -1;
	*theIndex = -1;
	
	if (!name || !*name || *name > sizeof(Str31) - 1)
		return (nil);
	
	hash = NickHash(name);
	index = FindNickExpansionForLo(name,hash,theWhich,true);	//	Search plug-in nicknames first
	if (index < 0)
		//	Not found, search standard nicknames
		index = FindNickExpansionForLo(name,hash,theWhich,false);

	*theIndex = index;
	
	if ( index>= 0) // If it is a nickname, then return the address information
		return (GetNicknameData(*theWhich,index,true,true));
	else
		return (nil);

}


#pragma mark ========== Nickname type-ahead routines ==========


/* MJN *//* new routine */
/************************************************************************
 * ResetNicknameTypeAhead - reset all of the global variables used to
 * handle nickname type-ahead
 *
 * WARNING: this routine can get called when gNicknameTypeAheadEnabled is
 * set to false
 ************************************************************************/
void ResetNicknameTypeAhead (PETEHandle pte)

{
	if (gTypeAheadNicknameTable) {
		DisposeNicknameTable (gTypeAheadNicknameTable); /* FINISH *//* make sure this won't hurt any caching that we do */
		gTypeAheadNicknameTable = nil;
	}
	gTypeAheadNicknameTableIndex = -1;

	gTypeAheadPrefix[0] = 0;
	gTypeAheadSuffix[0] = 0;

	gTypeAheadIdleDelay = gNicknameWatcherWaitKeyThresh ? LMGetKeyThresh() : 0;

	if (!PeteIsValid(pte))
		return;
		
	if (gNicknameHilitingEnabled && CurSelectionIsTypeAheadText (pte) && HasNickHiliting (pte))
		HilitingRangeCheckInclude (pte, GetTypeAheadSelStart (pte) - gTypeAheadPrefix[0], GetTypeAheadSelEnd (pte));

	/* FINISH *//* these two globals really only need to get reset every CompOpen */

	ClearTypeAhead (pte);
	SetTypeAheadKeyTicks (pte, 0); /* this resets idling, do we really want to do this? */

	SetTypeAheadSelStart (pte, -1);
	SetTypeAheadSelEnd (pte, -1);
	
	/* NOTE: TypeAheadPrevSelEnd doesn't get reset here, because SetNicknameTypeAheadText calls this routine, and we need to
						preserve TypeAheadPrevSelEnd across calls to SetNicknameTypeAheadText */
	SetTypeAheadAliasIndex (pte, -1);
	SetTypeAheadNicknameIndex (pte, -1);
}


/* MJN *//* new routine */
/************************************************************************
 * CurSelectionIsTypeAheadText - returns true if the current text
 * selection in the window passed in win was created by nickname
 * type-ahead.  To return true, the following conditions have to be met:
 *
 *			-	the window must be a composition window
 *			-	the MyWindowPtr and windex for win must match that in
 *				TypeAheadWin and TypeAheadWindex
 *			-	selection must be in a recipient field
 *			-	selection range must match that in TypeAheadSelStart and
 *				TypeAheadSelEnd
 *			-	text of current selection must match that in gTypeAheadSuffix
 *			-	text preeceeding current selection must match that in
 *				gTypeAheadPrefix
 *
 * The function will also return false in the unlikely event that
 * PETE returns an error.
 *
 * This function ignores any pref settings which have to do with
 * nickname type-ahead.
 ************************************************************************/

Boolean CurSelectionIsTypeAheadText (PETEHandle pte)

{
	Str255	prefixStr;
	Str255	suffixStr;
	Handle	textHdl;
	long		suffixLen,
					selStart,
					selEnd;

	if (!PeteIsValid(pte))
		return (false);
	
	// Verfify that this field accepts nickname completion
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return (false);

	// Get the current selection and compare it against our own typing.  We do this because the
	// user could have changed our typeahead selection via the mouse manual text selection.
	if (PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd))
		return (false);

	if ((selStart != GetTypeAheadSelStart (pte)) || (selEnd != GetTypeAheadSelEnd (pte)))
		return (false);

	if (suffixLen = selEnd - selStart) {
		BlockMoveData (*textHdl + selStart, suffixStr + 1, suffixLen);
		suffixStr[0] = suffixLen;
		if (!NicknameEqualString (suffixStr, gTypeAheadSuffix, true, true, true))
			return (false);
	}
	else
		if (gTypeAheadSuffix[0])
			return (false);

	(void) GetNicknamePrefixFromField (pte, prefixStr, true, false);
	if (!prefixStr[0]) /* will be true if GetNicknamePrefixFromField returns an error code */
		return (false);
	if (!NicknameRawEqualString (prefixStr, gTypeAheadPrefix))
		return (false);

	return (true);
}


/* MJN *//* new routine */
/* FINISH *//* add comments */
/************************************************************************
 * OKToInitiateTypeAhead
 ************************************************************************/
Boolean OKToInitiateTypeAhead (PETEHandle pte)
{
	Boolean charIsDelimiter;
	UPtr curDelim;
	long delimCount;
	Handle textHdl;
	long selStart, selEnd;
	long curOffset;
	long start,
			 end;
	UPtr curChar;
	unsigned char testChar;

	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return (false);
	
	if (!GetNickFieldRange (pte, &start, &end))
		return (false);

	if (PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd))
		return (false);

	curOffset = selStart;
	curChar = *textHdl + curOffset;
	charIsDelimiter = false;
	while (!charIsDelimiter && (curOffset > start))
	{
		curChar--;
		curOffset--;
		testChar = *curChar;
		if (testChar == '@')
			return false;
		delimCount = gRecipientDelimiterList[0];
		curDelim = gRecipientDelimiterList + 1;
		while (!charIsDelimiter && delimCount--)
		{
			if (testChar == *curDelim)
				charIsDelimiter = true;
			else
				curDelim++;
		}
	}

	curOffset = selEnd;
	curChar = *textHdl + curOffset;
	while ((curOffset < end) && (*curChar == ' '))
	{
		curChar++;
		curOffset++;
	}
	if (curOffset >= end)
		return true;

	testChar = *curChar;
	charIsDelimiter = false;
	delimCount = gRecipientDelimiterList[0];
	curDelim = gRecipientDelimiterList + 1;
	while (!charIsDelimiter && delimCount--)
	{
		if (testChar == *curDelim)
			charIsDelimiter = true;
		else
			curDelim++;
	}
	return charIsDelimiter;
}


/* MJN *//* new routine */
/************************************************************************
 * SetNicknameTypeAheadText - routine for doing nickname type-ahead
 * (automatic nickname completion while typing).  Before calling this
 * routine, you need to determine what characters the user has typed
 * so far, and what nickname it matches.  Pass the user's typing into
 * prefixStr, and use aliasIndex and nicknameIndex to specify the
 * nickname.  Pass in the MyWindowPtr into which the user is typing in
 * win.  If you have pre-built a nickname table to be used with
 * type-ahead, pass it in tableHdl, and pass in the associated entry
 * index in tableIndex.  If you pass in nil for tableHdl, this routine
 * will build a new table, if necessary.  If you pass in -1 for tableHdl,
 * it will force a new table to be built, even if it normally wouldn't
 * be necessary.
 *
 * This routine will get the nickname specified by aliasIndex and
 * nicknameIndex, subtract the text in prefixStr from the beginning of
 * it, insert the remaining characters at the text insertion point, and
 * then select the newly inserted text.
 *
 * Only one window can have a nickname type-ahead selection at a time.
 * If you call this routine for a second window when a first window
 * already has a nickname type-ahead selection, the text and selection
 * range in the first window will remain unaltered, but will no longer
 * be considered a nickname type-ahead selection.
 *
 * This function ignores any pref settings which have to do with
 * nickname type-ahead.
 *
 * This routine will intentionally fail if the current text insertion
 * point is not in a recipient field, or if a range of text is selected.
 * The one exception to this is that if a range of text is selected, but
 * it was created by a prior call to SetNicknameTypeAheadText, the
 * selected text will be deleted, and replaced with new text.
 *
 * Calling this routine sets up all of the nickname type-ahead globals,
 * so that we can later test to see if the current selection came from
 * calling this routine.  To test this, call CurSelectionIsTypeAheadText.
 *
 * The function returns noErr if it succeeds, or a non-zero error code
 * if it fails.  If it fails, the nickname type-ahead globals will all
 * be reset, even if the prior selection was created by calling this
 * routine and is still intact after the failed attempt to change it.
 * If the error occurred from calling PETE, the text in the window may
 * have been altered, and you are not guaranteed as to what state it is
 * in; this should not present a major problem, as the typical loss to
 * the user would be that the undo buffer will be clear or the selection
 * might not get recognized as nickname type-ahead text for future
 * keyboard operations.
 ************************************************************************/
OSErr SetNicknameTypeAheadText (PETEHandle pte, Str255 prefixStr, short aliasIndex, short nicknameIndex, NicknameTableHdl tableHdl, long tableIndex)

{
	NicknameTableHdl	prevTypeAheadNicknameTable;
	Str255						nicknameStr,
										prefixStr_,
										suffixStr;
	Handle						textHdl;
	OSErr 						theError,
										scratchErr;
	long							suffixLen,
										selStart,
										selEnd;
	Boolean						hadTypeAheadSel;

	if (!PeteIsValid(pte))
		return (paramErr);
	
	// Copy the prefix string
	BlockMoveData (prefixStr, prefixStr_, prefixStr[0] + 1);

	// If the current selection contains type ahead text and the nickname has not changed, just return
	hadTypeAheadSel = CurSelectionIsTypeAheadText (pte);
	if (hadTypeAheadSel && (aliasIndex == GetTypeAheadAliasIndex (pte)) && (nicknameIndex == GetTypeAheadNicknameIndex (pte)) && NicknameRawEqualString(prefixStr, gTypeAheadPrefix))
		return (noErr);
	
	// Make a copy of the nick table handle (but not the contents), then make the global 
	// nil so that the table is not disposed when resetting the type ahead globals
	prevTypeAheadNicknameTable = gTypeAheadNicknameTable;
	gTypeAheadNicknameTable = nil; /* to prevent ResetNicknameTypeAhead from disposing of it */
	ResetNicknameTypeAhead (pte);

	if (!hadTypeAheadSel && !OKToInitiateTypeAhead (pte))
		return (paramErr);

	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return (paramErr);
		
	if (theError = PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd))
		return (theError);
		
	if ((selStart != selEnd) && !hadTypeAheadSel) /* FINISH *//* should we do this?  what's the harm?  maybe this should be removed. */
		return (paramErr);

	/* FINISH *//* just dig it out of the table! */
	if (theError = RegenerateAllAliases (false)) /* FINISH *//* shouldn't we be passing in true? */
		return (theError);

	GetNicknameNamePStr (aliasIndex, nicknameIndex, nicknameStr);

#ifdef NTA_PREVENT_EXACT_MATCH
	/* never set type-ahead text if the prefix is an exact match with the specified nickname */
	if (prefixStr_[0] == nicknameStr[0])
		return (noErr);
#endif

	suffixLen = nicknameStr[0] - prefixStr_[0];
#ifdef NTA_PREVENT_EXACT_MATCH
	if (suffixLen <= 0)
		return (noErr);
#else
	if (suffixLen < 0) /* unexpected */
		return (noErr);
#endif

	if (suffixLen)
		BlockMoveData (nicknameStr + 1 + nicknameStr[0] - suffixLen, suffixStr + 1, suffixLen);
	suffixStr[0] = suffixLen;

	if ((prefixStr_[0] + suffixStr[0]) > 255) /* unlikely, but we'll check for this, just in case */
		return (noErr);

	/* FINISH *//* HUH???  How can this ever be true, if prefixStr+suffixStr is nicknameStr, which we know is always <= 255 chars?  Enquiring minds want to know! */

	/* FINISH */
	// make sure we don't already have a special selection from us

	theError = PETESetRecalcState (PETE, pte, false);
	if (!theError)
		theError = PETEClearUndo (PETE, pte);
	if (!theError)
		theError = PeteInsertPtr (pte, kPETECurrentSelection, suffixStr + 1, suffixLen);

	/* FINISH *//* is this the right thing to do? */
	if (!theError && suffixLen)
		PeteSelect ((*PeteExtra(pte))->win, pte, selStart, selStart + suffixLen);

	scratchErr = PETESetRecalcState(PETE, pte, true);
	if (!theError)
		theError = scratchErr;
	if (!theError)
		theError = PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd);
	if (theError)
		return (theError);

	SetTypeAheadSelStart (pte, selStart);
	SetTypeAheadSelEnd (pte, selEnd);
	BlockMoveData(prefixStr_, gTypeAheadPrefix, prefixStr_[0] + 1);
	BlockMoveData(suffixStr, gTypeAheadSuffix, suffixStr[0] + 1);
	SetTypeAheadAliasIndex (pte, aliasIndex);
	SetTypeAheadNicknameIndex (pte, nicknameIndex);

	// If we need to, fore a rebuild of the nickname table
	if (tableHdl == (NicknameTableHdl) (-1)) {
		tableHdl = nil;
		if (prevTypeAheadNicknameTable) {
			DisposeNicknameTable(prevTypeAheadNicknameTable);
			prevTypeAheadNicknameTable = nil;
		}
	}
	if (tableHdl) {
		if (prevTypeAheadNicknameTable && (tableHdl != prevTypeAheadNicknameTable))
			DisposeNicknameTable(prevTypeAheadNicknameTable);
		gTypeAheadNicknameTable = tableHdl;
		gTypeAheadNicknameTableIndex = tableIndex;
	}
	else
		if (prevTypeAheadNicknameTable && NicknameRawEqualString(prefixStr, (**prevTypeAheadNicknameTable).prefixStr)) {
			gTypeAheadNicknameTable = prevTypeAheadNicknameTable;
			gTypeAheadNicknameTableIndex = FindNicknameTableIndex(gTypeAheadNicknameTable, aliasIndex, nicknameIndex);
		}
		else {
			if (prevTypeAheadNicknameTable && EqualStringPrefix((**prevTypeAheadNicknameTable).prefixStr, prefixStr, true, true, true)) {
#ifdef NTA_PREVENT_EXACT_MATCH
				theError = BuildNicknameTableSubset (prefixStr, prevTypeAheadNicknameTable, false, &gTypeAheadNicknameTable);
#else
				theError = BuildNicknameTableSubset (prefixStr, prevTypeAheadNicknameTable, true, &gTypeAheadNicknameTable);
#endif
				DisposeNicknameTable (prevTypeAheadNicknameTable);
			}
			else {
				if (prevTypeAheadNicknameTable)
					DisposeNicknameTable(prevTypeAheadNicknameTable);
#ifdef NTA_PREVENT_EXACT_MATCH
				theError = BuildNicknameTable (prefixStr, GetAliasFileToScan (pte), false, false, &gTypeAheadNicknameTable);
#else
				theError = BuildNicknameTable (prefixStr, GetAliasFileToScan (pte), false, true, &gTypeAheadNicknameTable);
#endif
			}
		if (theError) {
			ResetNicknameTypeAhead (pte);
			return (theError);
		}
	
		gTypeAheadNicknameTableIndex = FindNicknameTableIndex (gTypeAheadNicknameTable, aliasIndex, nicknameIndex);
	}
	if ((gTypeAheadNicknameTableIndex < 0) || (gTypeAheadNicknameTableIndex >= (**gTypeAheadNicknameTable).numEntries))
		gTypeAheadNicknameTableIndex = 0;

	return (noErr);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameTypeAheadKey - process a keystroke from the user's typing in
 * a composition window, and do the appropriate handling for nickname
 * type-ahead (automatic nickname completion while typing).  Pass in the
 * window in which the user is typing in win, and a pointer to the
 * EventRecord for the keystroke in event.  win must point to a
 * composition window.
 *
 * The function returns true if no further processing of the keystroke
 * is needed, and false if the caller should proceed and handle the
 * keystroke on its own.  The function may do some processing, and still
 * return false.  The function may also modify the event record before
 * returning false.  In any case, if the function returns false, you must
 * continue with processing, and if you copied any data out of the
 * EventRecord prior to calling this routine, you may need to re-extract
 * those fields.
 *
 * This function has no effect if the text insertion point is not in a
 * recipient field, or if the pref for nickname type-ahead is turned off.
 * It is assumed that this routine will only be called from CompKey, and
 * therefore, this function will not check to validate that the window
 * passed in win is a composition window.
 *
 * Calling this routine will most likely do a reset of the nickname
 * type-ahead globals.
 *
 * This routine doesn't actually do the automatic insertion of the
 * nickname completion; that is done by NicknameTypeAheadIdle or
 * NicknameTypeAheadKeyFollowup.  Calling this routine will set a flag,
 * if appropriate, to let NicknameTypeAheadIdle or
 * NicknameTypeAheadKeyFollowup know that it needs to process the user's
 * new keystroke.
 ************************************************************************/

Boolean NicknameTypeAheadKey (PETEHandle pte, EventRecord* event)

{
	OSErr					theError;
	long					tableIndex;
	short					aliasIndex,
								nicknameIndex;
	unsigned char keyChar; /* ASCII of keystroke */
	unsigned char keyCode; /* virtual key code of keystroke */
	Boolean				hasTypeAheadSel;

	// Verify that this field can accept type ahead right now
	if (!NicknameTypeAheadValid (pte))
		return (false);
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return (false);
	
	// Check to see whether or not the current selection was created by type ahead,
	// or if it is okay to start type ahead typing.
	hasTypeAheadSel = CurSelectionIsTypeAheadText (pte);
	if (!hasTypeAheadSel)
		if (!OKToInitiateTypeAhead (pte))
			return (false);

	keyChar = event->message & charCodeMask;
	keyCode = (event->message & keyCodeMask) >> 8;

	// If a command-key combination aside from cmd-period is typed,
	// pass it back to the caller.
	if ((event->modifiers & cmdKey) && keyChar != periodChar)
		return (false);

	// Make some adjustments to help our switch statement find it's way...
	if (keyCode == clearKey)
		keyChar = delChar;
	else
		if (keyCode == escKey)
			keyChar = escChar;
			
	switch (keyChar) {
		case backSpace:
		case delChar:
			// On backspace, delete, clear (or forward delete), we reset the type ahead variables
			// then pass back false so that the keystroke will be processed by PETE, thereby
			// deleting the current selection
			ResetNicknameTypeAhead (pte);
			return (false);
			break;
		case tabKey:	// (jp) Added tab support here in b73 to change case immediately
		case returnChar:
		case enterChar:
			// On the enter or return keys we auto-complete the nickname, then reset the type
			// ahead globals.
			if (hasTypeAheadSel) {
				(void) FinishAliasUsingTypeAhead (pte, CanWeExpand (pte, event), false, false);
				ResetNicknameTypeAhead (pte);
			}
			return (keyChar == tabKey ? false : hasTypeAheadSel);
			break;
		case periodChar:
			// If there exists a selection and the user hits cmd-period, pass the event back
			// and handle it as though the user has pressed delete or backspace.
			if (hasTypeAheadSel && (event->modifiers & cmdKey)) {
				event->modifiers &= ~cmdKey;
				event->message	 &= ~(keyCodeMask | charCodeMask);
				event->message	 |= (clearKey << 8) | clearChar;
				ResetNicknameTypeAhead (pte);
			}
			else {
				NeedTypeAhead (pte);
				SetTypeAheadKeyTicks (pte, TickCount ());
			}
			return (false);
			break;
		case escChar:
			if (hasTypeAheadSel) {
				event->modifiers &= ~cmdKey;
				event->message	 &= ~(keyCodeMask | charCodeMask);
				event->message	 |= (clearKey << 8) | clearChar;
				ResetNicknameTypeAhead (pte);
				return (false);
			}
			break;
		case commaChar:
		case optionCommaChar:
			// Complete the nickname on a comma or opt-comma, then reset the type ahead variables
			if (hasTypeAheadSel) {
				(void) FinishAliasUsingTypeAhead (pte, CanWeExpand (pte, event), false, true);
				ResetNicknameTypeAhead (pte);
			}
			return (hasTypeAheadSel);
			break;
		case upArrowChar:
		case downArrowChar:
			// On an up or down arrow we cycle through the matching nicknames (in the proper direction)
			// if there is a current type ahead selection
			if (hasTypeAheadSel) {
				SetTypeAheadPrevSelEnd (pte, -1);
				if (gTypeAheadNicknameTable) {
					tableIndex = gTypeAheadNicknameTableIndex;
					if (keyChar == upArrowChar) {
						if (--tableIndex < 0)
							tableIndex = (**gTypeAheadNicknameTable).numEntries - 1;
					}
					else
						if (keyChar == downArrowChar)
							if (++tableIndex >= (**gTypeAheadNicknameTable).numEntries)
								tableIndex = 0;
				
					GetNicknameTableEntry (gTypeAheadNicknameTable, tableIndex, &aliasIndex, &nicknameIndex);
					if (!((aliasIndex == GetTypeAheadAliasIndex (pte)) && (nicknameIndex == GetTypeAheadNicknameIndex (pte)))) {
						SetTypeAheadPrevSelEnd (pte, GetTypeAheadSelEnd (pte));
						theError = SetNicknameTypeAheadText (pte, gTypeAheadPrefix, aliasIndex, nicknameIndex, gTypeAheadNicknameTable, tableIndex);
					}
					else
						SysBeep (5);
				}
				else
					SysBeep (5);
				// Once we've changed selections, we're no longer checking for type ahead (apparently)
				ClearTypeAhead (pte);
				return (true);
			}
			else
				ResetNicknameTypeAhead (pte);
			return (hasTypeAheadSel);
			break;
		case leftArrow:
		case rightArrow:
			// On left or right arrow we just reset typeahead and let PETE handle the keystroke
			ResetNicknameTypeAhead (pte);
			return (false);
			break;
		default:
			// As long as we didn't attempt to enter a control key, we'll handle it
			if (keyChar >= ' ') {
				NeedTypeAhead (pte);
				SetTypeAheadKeyTicks (pte, TickCount ());
				return (false);
			}
			break;
	}
	return (false);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameTypeAheadKeyFollowup - do followup work for processing a
 * keystroke from the user's typing in a composition window, and do the
 * appropriate handling for nickname type-ahead (automatic nickname
 * completion while typing).  Pass in the window in which the user is
 * typing in win.  win must point to a composition window.
 *
 * This function is used to handle nickname type-ahead when immediate
 * completion is turned on, via the global Boolean
 * gNicknameWatcherImmediate.  When this flag is on, we want to do the
 * nickname completion immediately after the user's keystroke, instead
 * of waiting until the next time NicknameTypeAheadIdle gets called.
 * This routine just calls NicknameTypeAheadIdle itself, and temporarily
 * sets the global gNicknameWatcherWaitKeyThresh to force
 * NicknameTypeAheadIdle to do its processing.
 *
 * This function has no effect if the text insertion point is not in a
 * recipient field, or if the pref for nickname type-ahead is turned off.
 * It is assumed that this routine will only be called from CompKey, and
 * therefore, this function will not check to validate that the window
 * passed in win is a composition window.
 *
 * Calling this routine will most likely do a reset of the nickname
 * type-ahead globals.
 ************************************************************************/
void NicknameTypeAheadKeyFollowup (PETEHandle pte)
{
	Boolean origNicknameWatcherWaitKeyThresh;

	if (!PeteIsValid(pte))
		return;
		
	if (!gNicknameTypeAheadEnabled || !gNicknameWatcherImmediate || !HasNickCompletion (pte))
		return;
		
	origNicknameWatcherWaitKeyThresh = gNicknameWatcherWaitKeyThresh;
	gNicknameWatcherWaitKeyThresh = false;
	
	// Give idle time immediately
	NicknameTypeAheadIdle (pte);
	
	gNicknameWatcherWaitKeyThresh = origNicknameWatcherWaitKeyThresh;
}


//
//	NicknameTypeAheadValid
//
//		Verify that...
//		  - we got a PETE
//			- that the setting for type ahead is set
//			-- whether or not we're in a field that handles type ahead

Boolean NicknameTypeAheadValid (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return (false);
	if (!gNicknameTypeAheadEnabled)
		return (false);
	if (!HasNickCompletion (pte))
		return (false);
	return (true);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameTypeAheadIdle - do idle-time nickname type-ahead processing
 * for the window passed in win.  Pass in the window the user has been
 * typing in in win; this must be a composition window.
 *
 * This routine handles a user's typing after they have finished typing
 * a few characters from the beginning of a nickname.  The actual
 * processing of keystrokes is handled by NicknameTypeAheadKey.
 *
 * Since the user may be typing several characters, this routine will
 * not process the user's typing until there are no more keyDown events
 * in the event queue and at least gTypeAheadIdleDelay ticks have passed
 * since the last keystroke.  Once this occurs, and if the user has typed
 * a valid nickname prefix, this routine calls SetNicknameTypeAheadText
 * to do the actual insertion of text into the window.
 *
 * This function has no effect if the text insertion point is not in a
 * recipient field, or if the pref for nickname type-ahead is turned off.
 * It is assumed that this routine will only be called from CompIdle, and
 * therefore, this function will not check to validate that the window
 * passed in win is a composition window.
 ************************************************************************/
void NicknameTypeAheadIdle (PETEHandle pte)

{
	EventRecord				scratchEvent;
	NicknameTableHdl	newTable;
	Str255						prefixStr;
	OSErr							theError;
	long							curTicks,
										tableIndex,
										oldLastTypeAheadKeyTicks,	// SD
										selStart,
										selEnd;
	short							aliasIndex,
										nicknameIndex;

	// Verify that this field can accept type ahead right now
	if (!NicknameTypeAheadValid (pte))
		return;
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickCompletion (pte)))
		return;
	
 	// Has type ahead started?  If not, why bother, y'know?
	if (!TypeAhead (pte))
		return;

	// Is it time for a type ahead check?
	curTicks = TickCount();
	if (gNicknameWatcherWaitKeyThresh && (curTicks < (GetTypeAheadKeyTicks (pte) + gTypeAheadIdleDelay)))
		return;

	// But wait... What if the user is still typing?
	if (gNicknameWatcherWaitNoKeyDown && EventAvail (keyDownMask, &scratchEvent))
		return;

	// If there is a selection -- reset the type ahead variables and bail
	if (!PeteGetTextAndSelection (pte, nil, &selStart, &selEnd))
		if (selStart != selEnd) {
			ResetNicknameTypeAhead (pte);
			return;
		}
	
	UseFeature (featureNicknameWatching);

	oldLastTypeAheadKeyTicks = GetTypeAheadKeyTicks (pte);	// SD -- in case we need to put it back
	SetTypeAheadKeyTicks (pte, curTicks);	// Why?
	ClearTypeAhead (pte);

	// Get the character typed so far into the field
	theError = GetNicknamePrefixFromField (pte, prefixStr, true, false);
	if (theError || !prefixStr[0] || (prefixStr[0] < kMinCharsForTypeAhead))
		return;

	// If the prefix matches only one nickname, we're done!  Yippee!!
	if (!NicknamePrefixOccursNTimes (prefixStr, 1, GetAliasFileToScan (pte)))
		return;

	// When the prefix matches multiple nicknames we'll need to build a table of matches (remember,
	// this is being done at idle time) that will be used by the sticky popup or the up/down arrow
	// keys to cycle through nicknames that match the typed prefix.
	if (gTypeAheadNicknameTable && EqualStringPrefix ((**gTypeAheadNicknameTable).prefixStr, prefixStr, true, true, true))
#ifdef NTA_PREVENT_EXACT_MATCH
		theError = BuildNicknameTableSubset (prefixStr, gTypeAheadNicknameTable, false, &newTable);
#else
		theError = BuildNicknameTableSubset (prefixStr, gTypeAheadNicknameTable, true, &newTable);
#endif
	else
#ifdef NTA_PREVENT_EXACT_MATCH
		theError = BuildNicknameTable (prefixStr, GetAliasFileToScan (pte), false, false, &newTable);
#else
		theError = BuildNicknameTable (prefixStr, GetAliasFileToScan (pte), false, true, &newTable);

	// if the table build is cancelled because the user does something, try again later SD
	// by changing the type ahead variable back to "need checking" status and setting the
	// type ahead key ticks back to it's original value
	if (theError == userCanceledErr) {
		NeedTypeAhead (pte);
		SetTypeAheadKeyTicks (pte, oldLastTypeAheadKeyTicks);
	}
	
	if (!theError) {
		// (jp) 12-2-99  To default the table to an item not in the history list
		tableIndex = FindFirstNonHistoryNicknameTableIndex (newTable);
//		tableIndex = 0;
		GetNicknameTableEntry (newTable, tableIndex, &aliasIndex, &nicknameIndex);
		(void) SetNicknameTypeAheadText (pte, prefixStr, aliasIndex, nicknameIndex, newTable, tableIndex);
	}
#endif
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameTypeAheadFinish - if a nickname type-ahead selection exists,
 * finish it; this is the equivalent as if the user had manually pressed
 * Return or Enter while the window was active.
 *
 * Pass the window you want the call to operate on in win.  If you pass
 * in nil for win, and a nickname type-ahead selection exists, this
 * function will operate on whatever window contains the type-ahead
 * selection.
 *
 * Since a nickname type-ahead selection is essentially a mode, and since
 * there is no visual feedback to indicate that it exists, and since it's
 * a pretty complex concept, the type-ahead selection should be finished
 * whenever the user takes their attention away from that particular
 * recipient field.  This includes actions like switching between fields
 * in the message, making non-textual changes to the message,
 * de-activating the message's window, etc.  Although it shouldn't hurt
 * anything to leave a type-ahead selection unfinished in such a
 * situation, it is best for the sake of UI to do so, anyways.
 *
 * This function ignores any pref settings which have to do with
 * nickname type-ahead.
 ************************************************************************/
void NicknameTypeAheadFinish (PETEHandle pte)
{
	if (!PeteIsValid(pte))
		return;
	
	if (CurSelectionIsTypeAheadText (pte))
		(void) FinishAliasUsingTypeAhead (pte, CanWeExpand (pte, nil), false, false);
}


/* MJN *//* new routine */
/************************************************************************
 * ToggleNicknameTypeAhead - toggle the state of the global
 * gNicknameTypeAheadEnabled, and if we've just turned it on, do an
 * expansion of the current recipient, if appropriate.
 *
 * Calling this routine does not actually change the pref setting itself
 * (PREF_NICK_EXP_TYPE_AHEAD), but rather just changes the global.  The
 * global will get reset each time the composition window receives an
 * activate/deactivate event, so this is just a temporary mechanism,
 * intended for toggling the state while typing in a recipient field.
 ************************************************************************/
void ToggleNicknameTypeAhead (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return;
		
	gNicknameTypeAheadEnabled = !gNicknameTypeAheadEnabled;
	ResetNicknameTypeAhead (pte);
	if (gNicknameTypeAheadEnabled) {
		NeedTypeAhead (pte);
		if (gNicknameWatcherImmediate)
			NicknameTypeAheadKeyFollowup (pte);
	}
}


#pragma mark ========== Nickname hiliting routines ==========


/* MJN *//* new routine */
/************************************************************************
 * ResetNicknameHiliting
 ************************************************************************/
void ResetNicknameHiliting (PETEHandle pte)

{
	gHilitingIdleDelay = gNicknameWatcherWaitKeyThresh ? LMGetKeyThresh() : 0;
	if (!PeteIsValid(pte))
		return;
		
	ClearHilite (pte);
	ClearRefresh (pte);
	SetHilitingKeyTicks (pte, 0);
	SetHilitingStart (pte, -1);
	SetHilitingEnd (pte, -1);
}


/* MJN *//* new routine */
/************************************************************************
 * SetNicknameHiliting
 ************************************************************************/
OSErr SetNicknameHiliting (PETEHandle pte, long startOffset, long endOffset, Boolean hilite)

{
	PETEDocInfo	peteInfo;
	OSErr				theError;
	short				peteDirtyWas;
	Boolean			winDirtyWas;

	if (!PeteIsValid(pte))
		return (paramErr);

	theError = noErr;
	
	// save the dirty bits
	peteDirtyWas = PeteIsDirty (pte);
	winDirtyWas	 = (*PeteExtra(pte))->win->isDirty;

	if (hilite && ((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickHiliting (pte)))
		theError = PeteLabel (pte, startOffset, endOffset, pNickHiliteLabel, pNickHiliteLabel);
	else
		theError = PeteNoLabel (pte, startOffset, endOffset, pNickHiliteLabel);

	if (!theError)
		theError = PETEGetDocInfo (PETE, pte, &peteInfo);
	if (!theError)
		if (endOffset == peteInfo.selStart)
			if (hilite)
				theError = PeteNoLabel (pte, endOffset, endOffset, pNickHiliteLabel);
	
	// restore the dirty bits
	if (!theError) {
		PETEMarkDocDirty(PETE,pte,peteDirtyWas);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
	}
	return (theError);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingUpdateRange
 ************************************************************************/
OSErr NicknameHilitingUpdateRange (PETEHandle pte, long startOffset, long endOffset)
{
	Str255	recipientTestStr;
	Handle	textHdl;
	OSErr		theError;
	long 		curOffset,
					curRecipientStartOffset,
					curRecipientEndOffset,
					curRecipientLen,
					foundOffset;
	Boolean	recipientIsNickname,
					hasTypeAheadSel;

	if (!PeteIsValid(pte))
		return paramErr;
		
	theError = PeteGetRawText (pte, &textHdl);

	if (!theError) {
		hasTypeAheadSel = CurSelectionIsTypeAheadText (pte);

		curOffset = startOffset;
		while (!theError && curOffset < endOffset) {
			foundOffset = FindNextRecipientStart (textHdl, curOffset);
			if (foundOffset > curOffset)
				(void) SetNicknameHiliting (pte, curOffset, foundOffset, false); /* ignore errors */
			if (foundOffset >= endOffset)
				return (noErr);

			curOffset		= foundOffset;
			foundOffset	= FindRecipientEndInText (textHdl, curOffset, true);
			curRecipientStartOffset	= curOffset;
			curRecipientEndOffset		= foundOffset;
			curRecipientLen					= curRecipientEndOffset - curRecipientStartOffset;
			
			if (curRecipientLen <= 0)
				continue;

			/* NOTE *//* normal nickname completion is sensitive to diacritical marks and sticky characters, but type-ahead is not, so we have to special-case type-ahead */
			if (hasTypeAheadSel && (curRecipientEndOffset == GetTypeAheadSelEnd (pte)) && (curRecipientStartOffset == (GetTypeAheadSelStart (pte) - gTypeAheadPrefix[0])))
				recipientIsNickname = true;
			else {
				MakePStr (recipientTestStr, *textHdl + curRecipientStartOffset, curRecipientLen);
				recipientIsNickname = NEFindNickname (recipientTestStr, false, true, true, nil, nil);
			}
			
			theError = SetNicknameHiliting (pte, curRecipientStartOffset, curRecipientEndOffset, recipientIsNickname); /* ignore errors */

			curOffset = curRecipientEndOffset;
		}
	}
	return (noErr); 	// Ignore errors
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingUpdateField
 ************************************************************************/
OSErr NicknameHilitingUpdateField (PETEHandle pte)

{
	Handle	textHdl;
	OSErr		theError,
					scratchErr;
	long		selStart,
					selEnd;

	if (!PeteIsValid(pte))
		return (paramErr);
		
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickHiliting (pte)))
		return (paramErr);
		
	ResetNicknameHiliting (pte);

	if (!gNicknameHilitingEnabled)
		return (NicknameHilitingUnHiliteField (pte));

	if (theError = PETESetRecalcState (PETE, pte, false))
		return (theError);

	if ((*PeteExtra(pte))->nick.fieldHilite)
		(*(*PeteExtra(pte))->nick.fieldHilite) (pte, true);
	else {
		theError = PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd);
		if (!theError)
			theError = NicknameHilitingUpdateRange (pte, 0, GetHandleSize (textHdl));
	}

	scratchErr = PETESetRecalcState (PETE, pte, true);
	if (!theError)
		theError = scratchErr;
	
	PeteWhompHiliteRegionBecausePeteWontFixIt (pte);

	return (theError);
}



/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingUnHiliteField
 ************************************************************************/
OSErr NicknameHilitingUnHiliteField (PETEHandle pte)

{
	Handle	textHdl;
	OSErr		theError,	
					scratchErr;
	long		selStart,
					selEnd;
	
	if (!PeteIsValid(pte))
		return (paramErr);
		
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickHiliting (pte)))
		return (paramErr);
		
	ResetNicknameHiliting (pte);

	if (theError = PETESetRecalcState(PETE, pte, false))
		return (theError);

	if ((*PeteExtra(pte))->nick.fieldHilite)
		(*(*PeteExtra(pte))->nick.fieldHilite) (pte, false);
	else {
		theError = PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd);
		if (!theError)
			theError = PeteNoLabel (pte, 0, GetHandleSize (textHdl), pNickHiliteLabel);
	}

	scratchErr = PETESetRecalcState(PETE, pte, true);
	if (!theError)
		theError = scratchErr;
	
	PeteWhompHiliteRegionBecausePeteWontFixIt (pte);
	return (theError);
}


/************************************************************************
 * NicknameHilitingUpdateAllFields
 ************************************************************************/
void NicknameHilitingUpdateAllFields(void)

{
	WindowPtr		winWP;
	MyWindowPtr	win;
	PETEHandle	pte;

	winWP = FrontWindow_();
	
	// Loop through all the windows and fields, updating the hilite
	while (winWP) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsKnownWindowMyWindow (winWP))
			if (IsWindowVisible(winWP))
				for (pte = win->pteList; pte; pte = PeteNext (pte))
					if (HasNickHiliting (pte))
						(void) NicknameHilitingUpdateField (pte);
		winWP = GetNextWindow (winWP);
	}
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingUnHiliteAllFields
 ************************************************************************/
void NicknameHilitingUnHiliteAllFields (void)

{
	WindowPtr 	winWP;
	MyWindowPtr	win;
	PETEHandle	pte;

	winWP = FrontWindow_();

	// Loop through all the windows and fields, updating the hilite
	while (winWP) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsMyWindow (winWP))
			if (IsWindowVisible(winWP))
				for (pte = win->pteList; pte; pte = PeteNext (pte))
					if (HasNickHiliting (pte))
						(void) NicknameHilitingUnHiliteField (pte);
		winWP = GetNextWindow(winWP);
	}
}


/* MJN *//* new routine */
/************************************************************************
 * HilitingRangeCheckInsert
 ************************************************************************/
void HilitingRangeCheckInsert (PETEHandle pte, long startOffset, long insertSize)

{
	long	headerStartOffset,
				headerEndOffset;

	if (!PeteIsValid(pte))
		return;
	
	if (RefreshPending (pte))
		return;

	NeedHilite (pte);

	if (GetHilitingStart (pte) == -1) {
		SetHilitingStart (pte, startOffset);
		SetHilitingEnd (pte, startOffset + insertSize);
		return;
	}

	if (startOffset >= GetHilitingEnd (pte))
		SetHilitingEnd (pte, startOffset + insertSize);
	else {
		SetHilitingEnd (pte, GetHilitingEnd (pte) + insertSize);
		if (startOffset < GetHilitingStart (pte))
			SetHilitingStart (pte, startOffset);
		}

	if (GetNickFieldRange (pte, &headerStartOffset, &headerEndOffset)) {
		headerEndOffset += insertSize;
		if ((GetHilitingStart (pte) < headerStartOffset) || (GetHilitingEnd (pte) > headerEndOffset))
			NeedRefresh (pte);
	}
}


/* MJN *//* new routine */
/************************************************************************
 * HilitingRangeCheckDelete
 ************************************************************************/
void HilitingRangeCheckDelete(PETEHandle pte, long startOffset, long endOffset)
{
	long	headerStartOffset,
				headerEndOffset,
				swap;

	if (!PeteIsValid(pte))
		return;
		
	if (RefreshPending (pte))
		return;

	if (endOffset < startOffset) {
		swap				= startOffset;
		startOffset = endOffset;
		endOffset		= swap;
	}

	NeedHilite (pte);

	if (GetHilitingStart (pte) == -1) {
		SetHilitingStart (pte, startOffset);
		SetHilitingEnd (pte, startOffset);
		return;
	}

	if (startOffset < GetHilitingStart (pte))
		SetHilitingStart (pte, startOffset);
	if (endOffset > GetHilitingEnd (pte))
		SetHilitingEnd (pte, startOffset);
	else
		SetHilitingEnd (pte, startOffset + GetHilitingEnd (pte) - endOffset);

	/* FINISH *//* do this sort of stuff in keyDown instead of here */
	if (GetNickFieldRange (pte, &headerStartOffset, &headerEndOffset)) {
		headerEndOffset -= endOffset - startOffset;
		if ((GetHilitingStart (pte) < headerStartOffset) || (GetHilitingEnd (pte) > headerEndOffset))
			NeedRefresh (pte);
	}
}


/* MJN *//* new routine */
/************************************************************************
 * HilitingRangeCheckInclude
 ************************************************************************/
void HilitingRangeCheckInclude (PETEHandle pte, long startOffset, long endOffset)
{
	long	headerStartOffset,
				headerEndOffset;

	if (!PeteIsValid(pte))
		return;

	if (RefreshPending (pte))
		return;

	NeedHilite (pte);

	if (GetHilitingStart (pte) == -1) {
		SetHilitingStart (pte, startOffset);
		SetHilitingEnd (pte, endOffset);
		return;
	}

	if (startOffset < GetHilitingStart (pte))
		SetHilitingStart (pte, startOffset);
	if (endOffset > GetHilitingEnd (pte))
		SetHilitingEnd (pte, endOffset);

	if (GetNickFieldRange (pte, &headerStartOffset, &headerEndOffset))
		if ((GetHilitingStart (pte) < headerStartOffset) || (GetHilitingEnd (pte) > headerEndOffset))
			NeedRefresh (pte);
}	


/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingKey
 ************************************************************************/
void NicknameHilitingKey (PETEHandle pte, EventRecord* event)

{
	PETEDocInfo		peteInfo;
	UPtr					curDelim;
	long					typeAheadStartOffset,
								delimCount;
	unsigned char keyChar;
	unsigned char	keyCode;
	Boolean				charIsDelimiter;

	// Verify that this field can accept hiliting right now
	if (!NicknameHilitingValid (pte))
		return;

	// Reset the tick mark for hiliting keystrokes
	SetHilitingKeyTicks (pte, TickCount ());

	// If a hiliting check or refresh is pending, bail.
	if (HilitePending (pte) && RefreshPending (pte))
		return;

	// All command keys get passed on
	if (event->modifiers & cmdKey)
		return;

	if (PETEGetDocInfo (PETE, pte, &peteInfo))
		return;
		
	keyChar = event->message & charCodeMask;
	keyCode = (event->message & keyCodeMask) >> 8;

	switch (keyChar) {
		case leftArrow:
		case rightArrow:
			break;
		case upArrowChar:
		case downArrowChar:
			if (CurSelectionIsTypeAheadText (pte)) {
				if (GetTypeAheadPrevSelEnd (pte) == -1) {
					NeedHilite (pte);
					NeedRefresh (pte);
				}
				else {
					typeAheadStartOffset = GetTypeAheadSelStart (pte) - gTypeAheadPrefix[0];

					if (HilitePending (pte) && ((GetHilitingStart (pte) == typeAheadStartOffset) && (GetHilitingEnd (pte) == GetTypeAheadPrevSelEnd (pte))))
						SetHilitingEnd (pte, GetTypeAheadSelEnd (pte));
					else {
						if (GetTypeAheadPrevSelEnd (pte) < GetTypeAheadSelEnd (pte))
							HilitingRangeCheckInsert (pte, GetTypeAheadPrevSelEnd (pte), GetTypeAheadSelEnd (pte) - GetTypeAheadPrevSelEnd (pte));
						else if (GetTypeAheadPrevSelEnd (pte) > GetTypeAheadSelEnd (pte))
							HilitingRangeCheckDelete (pte, GetTypeAheadSelEnd (pte), GetTypeAheadPrevSelEnd (pte));
						HilitingRangeCheckInclude (pte, typeAheadStartOffset, GetTypeAheadSelEnd (pte));
					}
				}
			}
			break;
		case tabKey:
		case returnChar:
		case enterChar:
			if (HilitePending (pte))
				NeedRefresh (pte);
			break;
		case backSpace:
		case clearKey:
		case delChar:
			if (peteInfo.selStart != peteInfo.selStop)
				HilitingRangeCheckDelete (pte, peteInfo.selStart, peteInfo.selStop);
			else if (keyChar == delChar)
				HilitingRangeCheckDelete (pte, peteInfo.selStart, peteInfo.selStart + 1);
			else
				HilitingRangeCheckDelete (pte, peteInfo.selStart - 1, peteInfo.selStart);
			break;
		default:
			if (CharIsTypingChar (keyChar, keyCode)) {
				if (peteInfo.selStart != peteInfo.selStop)
					HilitingRangeCheckDelete (pte, peteInfo.selStart, peteInfo.selStop);

				delimCount			= gRecipientDelimiterList[0];
				curDelim				= gRecipientDelimiterList + 1;
				while (!charIsDelimiter && delimCount--)
					if (keyChar == *curDelim)
						charIsDelimiter = true;
					else
						curDelim++;

				if (charIsDelimiter) {
					HilitingRangeCheckInsert (pte, peteInfo.selStart, 1);
					HilitingRangeCheckInclude (pte, peteInfo.selStart - 1, peteInfo.selStart + 2);
				}
				else
					HilitingRangeCheckInsert (pte, peteInfo.selStart, 1);
			}
			break;
	}
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingKeyFollowup
 ************************************************************************/
void NicknameHilitingKeyFollowup (PETEHandle pte)
{
	Boolean origNicknameWatcherWaitKeyThresh;

	if (!NicknameHilitingValid (pte))
		return;
		
	if (!gNicknameWatcherImmediate)
		return;

	origNicknameWatcherWaitKeyThresh = gNicknameWatcherWaitKeyThresh;
	gNicknameWatcherWaitKeyThresh = false;
	NicknameHilitingIdle (pte);
	gNicknameWatcherWaitKeyThresh = origNicknameWatcherWaitKeyThresh;
}


//
//	NicknameHilitingValid
//
//		Verify that...
//		  -- we got a PETE
//			-- that the setting for hiliting is set
//			-- whether or not we're in a field that handles hiliting

Boolean NicknameHilitingValid (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return (false);
	if (!gNicknameHilitingEnabled)
		return (false);
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickHiliting (pte)))
		return (false);
	return (true);
}

/* MJN *//* new routine */
/************************************************************************
 * NicknameHilitingIdle
 ************************************************************************/
void NicknameHilitingIdle (PETEHandle pte)

{
	EventRecord	scratchEvent;
	Handle			textHdl;
	long				curTicks,
							startOffset,
							endOffset;

	// Verify that this field can hilite right now
	if (!NicknameHilitingValid (pte))
		return;
		
 	// Has anything happened that would require a hilite check?
	if (!HilitePending (pte))
		return;

	// Is it even time for a hilite check?
	curTicks = TickCount ();
	if (gNicknameWatcherWaitKeyThresh && (curTicks < (GetHilitingKeyTicks (pte) + gHilitingIdleDelay)))
		return;

	// But wait... What if the user is still typing?
	if (gNicknameWatcherWaitNoKeyDown && EventAvail(keyDownMask, &scratchEvent))
		return;

	SetHilitingKeyTicks (pte, curTicks);

	UseFeature (featureNicknameWatching);

	// If a full field refresh is pending, update the field and reset the hiliting globals
	if (RefreshPending (pte)) {
		(void) NicknameHilitingUpdateField (pte);
		ResetNicknameHiliting (pte); /* FINISH *//* where's the right place to call this? */
		return;
	}

	// When a refresh is not pending we'll refresh only the highlighted recipient
	if (PeteGetRawText (pte, &textHdl))
		return;

	if ((*PeteExtra(pte))->nick.fieldHilite)
		(*(*PeteExtra(pte))->nick.fieldHilite) (pte, true);
	else {
		startOffset = FindRecipientStartInText (textHdl, GetHilitingStart (pte), true);
		endOffset		= FindRecipientEndInText (textHdl, GetHilitingEnd (pte), true);
		(void) NicknameHilitingUpdateRange (pte, startOffset, endOffset);
	}

//	(void) NicknameHilitingUpdateRange (pte, startOffset, endOffset); /* ignore errors */
	ResetNicknameHiliting (pte); /* FINISH *//* where's the right place to call this? */
}


/* MJN *//* new routine */
/************************************************************************
 * InitNicknameHiliting
 ************************************************************************/

void InitNicknameHiliting (void)

{
	PETELabelStyleEntry	pls;
	long								styleCode;

	Zero (pls);
	
	// Label for nickname highlighting
	pls.plValidBits = peColorValid | peFaceValid;
	pls.plLabel			= pNickHiliteLabel;

	styleCode = GetRLong (NICK_STYLE);
	if (styleCode & 0x00010000)
		GetRColor (&pls.plColor, NICK_COLOR);
	pls.plFace = (long) styleCode & (long) 0x00000000FF;
	PETESetLabelStyle (PETE, nil, &pls);

	ResetNicknameHiliting (nil);
}


#pragma mark ========== Nickname Watcher routines ==========


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherKey
 ************************************************************************/
Boolean NicknameWatcherKey (PETEHandle pte, EventRecord* event)

{
	Boolean typeAheadResult;

	if (!PeteIsValid(pte))
		return (false);

	typeAheadResult = NicknameTypeAheadKey (pte, event);
	NicknameHilitingKey (pte, event);
	return (typeAheadResult);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherKeyFollowup
 ************************************************************************/
void NicknameWatcherKeyFollowup (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return;
		
	NicknameTypeAheadKeyFollowup (pte);
	NicknameHilitingKeyFollowup (pte);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherIdle
 ************************************************************************/
void NicknameWatcherIdle (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return;
		
	NicknameTypeAheadIdle (pte);
	NicknameHilitingIdle (pte);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherFocusChange
 ************************************************************************/
void NicknameWatcherFocusChange (PETEHandle pte)

{
	if (NicknameTypeAheadValid (pte))
		NicknameTypeAheadFinish (pte);

	if (NicknameHilitingValid (pte))
		if (HilitePending (pte))
			NeedRefresh (pte);
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherModifiedField
 ************************************************************************/
void NicknameWatcherModifiedField (PETEHandle pte)

{
	if (NicknameHilitingValid (pte)) {
		NeedHilite (pte);
		NeedRefresh (pte);
	}
}


/* MJN *//* new routine */
/************************************************************************
 * NicknameWatcherMaybeModifiedField
 ************************************************************************/
void NicknameWatcherMaybeModifiedField (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return;
	
	if (!((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickScanCapability (pte)))
		return;
		
	if ((!gNicknameTypeAheadEnabled && (!gNicknameHilitingEnabled || !HasNickHiliting (pte))) || !HasNickCompletion (pte))
		return;

	NicknameWatcherModifiedField (pte);
}



/* MJN *//* new routine */
/************************************************************************
 * InitNicknameWatcher
 ************************************************************************/
OSErr InitNicknameWatcher (void)

{
	// Initialize the globals
	gTypeAheadPrefix[0] = 0;
	gTypeAheadSuffix[0] = 0;
	
	gTypeAheadNicknameTable			= nil;
	gTypeAheadNicknameTableIndex = -1;

	BlockMoveData("\p,:;", gRecipientDelimiterList, 4); /* FINISH *//* use resource */
	gRecipientDelimiterList[++gRecipientDelimiterList[0]] = returnChar;

	// Make sure we reset type ahead and hiliting
	ResetNicknameTypeAhead (nil);
	RefreshNicknameWatcherGlobals (false);
	InitNicknameHiliting ();
	return noErr;
}


/* MJN *//* new routine */
/************************************************************************
 * SetDefaultNicknameWatcherPrefs
 ************************************************************************/
void SetDefaultNicknameWatcherPrefs(void)

{
	/* we ignore errors here, since an error probably means something is really screwed up,
			and failure just means the pref isn't set to its idea setting */
	(void) SetPref(PREF_NICK_EXP_TYPE_AHEAD, YesStr);
	(void) SetPref(PREF_NICK_HILITING, YesStr);
	(void) SetPref(PREF_NICK_CACHE, NoStr);
	(void) SetPref(PREF_NICK_WATCH_IMMED, YesStr);
	(void) SetPref(PREF_NICK_WATCH_WAIT_KEYTHRESH, NoStr);
	(void) SetPref(PREF_NICK_WATCH_WAIT_NO_KEYDOWN, YesStr);
}

#pragma mark ========== Nickname Cache routines ==========


Boolean NicknameCachingValid (PETEHandle pte)

{
	if (!PeteIsValid(pte))
		return (false);
	if (!gNicknameCacheEnabled || !HasNickCaching (pte))
		return (false);
	return (true);
}


//
//	NicknameCachingScan
//
//		Compares a new cache of addresses against the existing cache.
//		Differences become candidates for inclusion in the nickname cache.
//

void NicknameCachingScan (PETEHandle pte, Handle newAddresses)

{
	PStr	string,
				end;
	long	len;
	
	if (!NicknameCachingValid (pte))
		return;
	
	if (PrefIsSet (PREF_NICK_CACHE_NOT_ADD_TYPING))
		return;

	if (!(*PeteExtra(pte))->nick.addresses)
		return;

	string = LDRef (newAddresses);
	LDRef ((*PeteExtra(pte))->nick.addresses);
	end = string + GetHandleSize (newAddresses);
	len = GetHandleSize ((*PeteExtra(pte))->nick.addresses);
	while (string < end && *string) {
		if (!FindPStr (string, *((*PeteExtra(pte))->nick.addresses), len))
			CacheRecentNickname (string);
		string += *string + 2;
	}
	UL ((*PeteExtra(pte))->nick.addresses);
	UL (newAddresses);
}

//
//	FindPStr
//
//		Finds Pascal strings in a block of memory that itself MUST be an
//		array of null-terminated Pascal strings (pretty specialized, huh).
//

UPtr FindPStr (PStr string, UPtr spot, Size len)

{
	UPtr	end;
	
	end = spot + len;
	while (spot < end) {
		if (StringSame (string, spot))
			return (spot);
		spot += *spot + 2;
	}
	return (nil);
}


#pragma mark ========== Nickname refresh routines ==========


/* MJN *//* new routine */
/* FINISH *//* rename to watcher */
/************************************************************************
 * RefreshNicknameWatcherGlobals - reset global variables for nickname
 * expansion from preferences
 ************************************************************************/

void RefreshNicknameWatcherGlobals (Boolean refreshFields)

{
	Boolean prevNicknameHilitingEnabled,
					nicknameWatchingSupported;

	nicknameWatchingSupported = HasFeature (featureNicknameWatching);

	prevNicknameHilitingEnabled = gNicknameHilitingEnabled;

	gNicknameTypeAheadEnabled			= nicknameWatchingSupported && PrefIsSet (PREF_NICK_EXP_TYPE_AHEAD);
	gNicknameWatcherImmediate			= nicknameWatchingSupported && PrefIsSet (PREF_NICK_WATCH_IMMED);
	gNicknameWatcherWaitKeyThresh = nicknameWatchingSupported && PrefIsSet (PREF_NICK_WATCH_WAIT_KEYTHRESH);
	gNicknameWatcherWaitNoKeyDown	= nicknameWatchingSupported && PrefIsSet (PREF_NICK_WATCH_WAIT_NO_KEYDOWN);
	gNicknameHilitingEnabled			= nicknameWatchingSupported && PrefIsSet (PREF_NICK_HILITING);
	gNicknameAutoExpandEnabled		= PrefIsSet (PREF_NICK_AUTO_EXPAND);
	gNicknameCacheEnabled					= nicknameWatchingSupported && !PrefIsSet (PREF_NICK_CACHE);

	gTypeAheadIdleDelay	= gNicknameWatcherWaitKeyThresh ? LMGetKeyThresh() : 0;
	gHilitingIdleDelay	= gNicknameWatcherWaitKeyThresh ? LMGetKeyThresh() : 0;
	
	if (!gNicknameTypeAheadEnabled)
		ResetNicknameTypeAhead (nil);

	if (!gNicknameHilitingEnabled)
		ResetNicknameHiliting (nil);

	// Check to see if the hiliting has changed and refresh all fields if it has
	if (refreshFields && gNicknameHilitingEnabled != prevNicknameHilitingEnabled)
		if (gNicknameHilitingEnabled)
			NicknameHilitingUpdateAllFields ();
		else
			NicknameHilitingUnHiliteAllFields ();
}


/* MJN *//* new routine */
/************************************************************************
 * InvalCachedNicknameData
 ************************************************************************/
void InvalCachedNicknameData(void)

{
	ResetNicknameTypeAhead (nil);
	if (gNicknameHilitingEnabled)
		NicknameHilitingUpdateAllFields (); /* ignore errors */
}


#pragma mark ========== Final comments ==========


/* MJN *//* FINISH *//* if hiliting is on, call refreshheader on activate for compwin */
/* MJN *//* FINISH *//* set final color and style for nickname hiliting */

//
//	GetNickFieldRange
//
//		Returns false if the current field (for example, in the case of a composisiton
//		window header) cannot _really_ accept nickname stuff.

Boolean GetNickFieldRange (PETEHandle pte, long *start, long *end)

{
	Handle	textHdl;
	long		selStart,
					selEnd;

	*start	 = 0;
	*end		 = 0;

	if (!HasNickScanCapability (pte))
		return (false);
			
	if ((*PeteExtra(pte))->nick.fieldRange)
		return ((*(*PeteExtra(pte))->nick.fieldRange) (pte, start, end));
	else
		if (!PeteGetTextAndSelection (pte, &textHdl, &selStart, &selEnd))
			*end = GetHandleSize (textHdl);
	return (true);
}


//
//	SetNickScanning
//
//		Call this routine to setup a PETE field to support various flavors of nickname scanning.
//				nickHighlight  :  perform nickname highlighting in this field
//				nickComplete	 :	do nickname auto-completion in this field
//				nickExpand		 :  nickname expansion is supported by this field
//				nickCache			 :  nickname caching is supported by this field
//

void SetNickScanning (PETEHandle pte, NickScanType nickScan, short aliasIndex, NickFieldCheckProcPtr nickFieldCheck, NickFieldHiliteProcPtr nickFieldHilite, NickFieldRangeProcPtr nickFieldRange)

{
	if (!PeteIsValid(pte))
		return;
	
	if (HasFeature (featureNicknameWatching)) {
		(*PeteExtra(pte))->nick.scan				= nickScan;
		(*PeteExtra(pte))->nick.aliasIndex	= aliasIndex;
		(*PeteExtra(pte))->nick.fieldCheck	= nickFieldCheck;
		(*PeteExtra(pte))->nick.fieldHilite	= nickFieldHilite;
		(*PeteExtra(pte))->nick.fieldRange	= nickFieldRange;
	}
}


void SetNickCacheAddresses (PETEHandle pte, Handle addresses)

{
	if (NicknameCachingValid (pte) && addresses) {
		ZapHandle ((*PeteExtra(pte))->nick.addresses);
		(*PeteExtra(pte))->nick.addresses = addresses;
	}
}


//
//	CanWeExpand
//
//		Checks to see whether or not we're currently able to expand a nickname.
//		Takes into account the nick scanning attributes of the field, the
//		nick scanning preferences, and modifier keys.
//

Boolean CanWeExpand (PETEHandle pte, EventRecord* event)

{
	Boolean	yesWeCan;
	
	if (yesWeCan = HasNickExpansion (pte)) {
		yesWeCan = gNicknameAutoExpandEnabled;
		if (event)
			if (event->modifiers & optionKey)
				yesWeCan = !yesWeCan;
	}
	return (yesWeCan);
}


//
//	CacheRecentNickname
//
//		The loose logic...
//
// 		Check to see if this nickname is already in the cache file
// 				True:  Update it's cache date and we're done
// 				False: Check to see if there is room in the cache
//									True:		Add this nickname
//									False:	Trim the cache to one less than the max
//													Add this nickname


void CacheRecentNickname (PStr possibleNickname)

{
	NickStructHandle	aliases;
	BinAddrHandle			addresses,
										shortAddress;
	Handle						notes;
	Str255						firstName,
										lastName;
	Str63							suggestedNickname,
										realName;
	long							hashName,
										hashAddress,
										index;
	short							foundAliasIndex,
										foundNicknameIndex,
										historyAB,
										which;
	Boolean						wannaSave,
										cacheWasDirty;;
	
	if (!HasFeature (featureNicknameWatching))
		return;
								
	UseFeature (featureNicknameWatching);

	historyAB = FindAddressBookType (historyAddressBook);
	if (*possibleNickname && historyAB >= 0) {
		cacheWasDirty = (*Aliases)[historyAB].dirty;
		// First, determine the suggested nickname from the drivel we've been passed
		addresses = nil;
		if (!SuckPtrAddresses (&addresses, &possibleNickname[1], *possibleNickname, true, true, false, nil))
			if (addresses) {
				if (!PrefIsSet (PREF_CACHE_ATLESS_NICKS) && !PIndex (*addresses, '@'))
					return;
				NickSuggest (addresses, suggestedNickname, realName,false,HIST_NICK_FMT);
				// Once we have a safe'n'sane nickname, go to work...
				if (suggestedNickname[0]) {
					// Check to see if the suggested nickname is already present in a nickname file besides the cache file
					foundAliasIndex = -1;
					// If we did NOT find the nickname in any address book, or if it was found in the history list... maybe cache it.
					if (!NEFindNickname (suggestedNickname, false, true, true, &foundAliasIndex, &foundNicknameIndex) || IsHistoryAddressBook (foundAliasIndex))
						// Check to see if this nickname is already in the cache file
						if (aliases = (*Aliases)[historyAB].theData) {
							hashName = NickHash (suggestedNickname);
							wannaSave = false;
							if ((index = NickMatchFound (aliases, hashName, suggestedNickname, historyAB)) >= 0) {
								// We have to pull the addresses into memory, otherwise we think they've been purged since the nickname is dirty
								if (GetNicknameData (historyAB, index, true, true)) {
									// Update it's cache date since we found it hanging around the cache file
									(*aliases)[index].cacheDate			 = LocalDateTime ();
									(*aliases)[index].addressesDirty = true;
									SetAliasDirty(historyAB);
									wannaSave = true;
								}
							}
							else {
								shortAddress = nil;
								// As a last check... see if the address is already present in a nickname file
								// Or... if we're dealing with Adobe-esque bogus address books, try to find the
								//			 shortAddress in the Nickname field.
								if (!SuckPtrAddresses (&shortAddress, LDRef (addresses) + 1, **addresses, false, true, false, nil)) {
#ifdef DEBUG
									short testWhich;
									short testIndex;
									NickExpFindNickFromAddr(LDRef(shortAddress),&testWhich,&testIndex);
#endif
									hashAddress = NickHashString (LDRef (shortAddress));
									index				= -1;
									for (which = 0; which < NAliases && index < 0; ++which) {
										index = NickAddressMatchFound ((*Aliases)[which].theData, hashAddress, *shortAddress, which);
										if ((*Aliases)[which].containsBogusNicks && index == -1)
											index = NickMatchFound ((*Aliases)[which].theData, hashAddress, *shortAddress, which);
									}

									ASSERT(testIndex==index);
									UL (shortAddress);
									
									if (index == -1) {
										// Trim any excess off of the end of the cache
										TrimNicknameCache (aliases, 1);
										// Add this nickname
										// (jp) 4/12/00  This is NOT the most elegant solution to bug 3041 (save changes alert when nothing in
										//							 the address book has changed except history).  I tried fixing this the right way by
										//							 passing 'true' for doSave, which of course did nothing.  I tried a couple of other
										//							 things as well to correctly save changes inside of AddNickname, but no matter what
										//							 you do the various dirty bits for nicknames, address books or the address book window
										//							 are out of sync, I think due to a bug in SaveCurrentAlias -- but I'm a little scared
										//							 make sweeping changes (or even subtle changes) in there.
										//
										//							 Instead, I'm taking the coward's way out and I'm simulating a manual address book save
										//							 as long as the address book is open and the nickname was successfully added.
										ParseFirstLast (realName, firstName, lastName);
										notes = CreateSimpleNotes (realName, firstName, lastName);
										if (NewNickLow (shortAddress, notes, historyAB, suggestedNickname, false, nrAdd, false) >= 0) {
											if (!(*Aliases)[historyAB].collapsed)
												ABTickleHardEnoughToMakeYouPuke ();
											SetAliasDirty(historyAB);
											wannaSave = true;
										}
									}
								}
								ZapHandle (shortAddress);
							}
							if (wannaSave && !cacheWasDirty)
								if (SaveIndNickFile (historyAB, true))
									ABClean ();
						}
				}
				ZapHandle (addresses);
			}
	}
}


/**********************************************************************
 * NickExpFindNickFromAddr - given an address, find the nickname it belongs
 * to, if possible
 **********************************************************************/
OSErr NickExpFindNickFromAddr(PStr addrStr,short *outWhich,short *outIndex)
{
	uLong hashAddress;
	short index = -1;
	short which;
	
	hashAddress = NickHashString(addrStr);
	index				= -1;
	for (which = 0; which < NAliases; ++which) {
		index = NickAddressMatchFound ((*Aliases)[which].theData, hashAddress, addrStr, which);
		if ((*Aliases)[which].containsBogusNicks && index == -1)
			index = NickMatchFound ((*Aliases)[which].theData, hashAddress, addrStr, which);
		if (index>=0) break;
	}
	
	*outWhich = which;
	*outIndex = index;
	return index==-1 ? fnfErr : noErr;
}

void TrimNicknameCache (NickStructHandle aliases, short spaceNeeded)

{
	long	cacheSize,
				threshold,
				index;
	short	historyAB;
				
	if (!HasFeature (featureNicknameWatching))
		return;

	historyAB = FindAddressBookType (historyAddressBook);
	cacheSize	= HandleCount (aliases);
	threshold	= GetRLong (NICK_CACHE_SIZE) - spaceNeeded;
	
	while (cacheSize-- > threshold) {
		index = FindOldestCacheMember (aliases);
		(*aliases)[index].addressesDirty		= true;
		(*aliases)[index].deleted	= true;
		RemoveNickFromAddressBookList (historyAB, index, false);
		SetAliasDirty(historyAB);
	}
}

short FindOldestCacheMember (NickStructHandle aliases)

{
	register NickStructPtr	nickPtr;
	register uLong					oldestTimestamp;
	register short					oldestTimeStampIndex,
													index,
													numAliases;
	
	oldestTimestamp			 = 0xFFFFFFFF;		// Way of in the future...
	oldestTimeStampIndex = 0;
	
	numAliases = HandleCount (aliases);
	nickPtr = *aliases;
	for (index = 0; index < numAliases; ++index, ++nickPtr)
		if (!nickPtr->deleted && nickPtr->cacheDate < oldestTimestamp) {
			oldestTimestamp = nickPtr->cacheDate;
			oldestTimeStampIndex = index;
		}
	return (oldestTimeStampIndex);
}


//
//	CompareRawToExpansion
//
//		Compares a raw address string (Pascal-style, stored in a handle, as you might
//		get back from SuckPtrAddresses) to a Handle of a text that has not yet been
//		massaged for extra spaces.
//
//		Keep your fingers crossed...
//

Boolean CompareRawToExpansion (StringHandle raw, Handle expansion)

{
	Str255	temp;
	Size		expSize;
	Boolean	same;

	same = false;
	if (raw && expansion) {
		expSize = GetHandleSize_ (expansion);
		if (**raw <= expSize) {
			MakePStr (temp, *expansion, GetHandleSize_ (expansion));
			if ((*temp = RemoveChar (' ', temp + 1, *temp)) == **raw) {
				same = StringSame (temp, LDRef (raw));
				UL (raw);
			}
		}
	}
	return (same);
}

/************************************************************************
 * AppearsInAliasFile - does a nickname appear in an address book?
 ************************************************************************/
Boolean AppearsInAliasFile(PStr address,PStr file)
{
	Str255 shortAddr;
	uLong addrHash;
	
	ShortAddr(shortAddr,address);
	MyLowerStr(shortAddr);
	addrHash = Hash(shortAddr);
	
	return HashAppearsInAliasFile(addrHash,file);
}

Boolean HashAppearsInAliasFile(uLong addrHash,PStr file)
{
	short which;
	FSSpec spec;
	
	for (which=NAliases;which;which--)
	{
		spec = (*Aliases)[which-1].spec;
		if (file==nil || StringSame(file,spec.name))
			if (AppearsInWhichAliasFile(addrHash,which-1)) return true;
	}
	
	return false;
}

/************************************************************************
 * AppearsInWhichAliasFile - does a nickname appear in an address book, but hashed and found
 ************************************************************************/
Boolean AppearsInWhichAliasFile(uLong addrHash,short which)
{
	uLong *hash, *end;
	
	if (!(*Aliases)[which].addressHashes.data) BuildAddressHashes(which);
	if (!(*Aliases)[which].addressHashes.data) return false;
	
	hash = (uLong*)(*(*Aliases)[which].addressHashes.data);
	end = (uLong*)(*(*Aliases)[which].addressHashes.data + (*Aliases)[which].addressHashes.offset);
	
	for (;hash<end;hash++)
		if (*hash==addrHash) return true;
	
	return false;
}