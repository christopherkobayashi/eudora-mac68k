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

#include "nickmng.h"
#define FILE_NUM 2
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * routines to manage the nickname list
 ************************************************************************/
#pragma segment NickMng

/************************************************************************
 * 7/17/96	ALB
 *		Performance update for very large nickname files. 
 *		- Each nickname is no longer stored in a separate handle. They
 *			are all stored in one handle with an offset to each one 
 *		- Addresses, notes, and view data are no longer kept in a separate handle
 ************************************************************************/

/************************************************************************
 * The aliases list has the following structure:
 * <length byte>name-of-alias<2 length bytes>expansion-of-alias...
 * The aliases file contains lines of the form:
 * "alias" name-of-alias expansion of alias<newline>
 * Newlines may be escaped with "\".
 * Lines not beginning with "alias" will be ignored
 ************************************************************************/

/************************************************************************
 * private functions
 ************************************************************************/


typedef struct
{
	Str31 name;
	long offset;
} AliasSortType, *AliasSortPtr, **AliasSortHandle;

typedef struct
{
	long hashName;
	long hashAddress;
//	long	createDate;		(eventually)
//	long	modDate;			(eventually)
//	long	usageDate;		(eventually)
	long	cacheDate;
	long addressOffset;
	long	notesOffset;
	NicknameFlagsType	flags;					// New for 5.0
} NickTOCStruct;

typedef struct
{
	long	offset;
	short	nickIndex;
} NickOffSetSortType;

	Boolean NeatenLine(UPtr line, long *len);
	void CheckForNicknameBogosity (short which);
	Boolean SaveIndNickFile(short which, Boolean saveChangeBits);
	Boolean	SaveFileFast (short which, Boolean saveChangeBits);
	int NickOffsetCompare(NickOffSetSortType *n1, NickOffSetSortType *n2);
	void NickOffsetSwap(NickOffSetSortType *n1, NickOffSetSortType *n2);
	PStr StripQualifiersAndHonorifics (PStr name, PStr strippedName);
OSErr AddHandleToAddressHashes(TextAddrHandle sourceHandle,AccuPtr a);
int SortAddrNameCompare(UPtr *s1, UPtr *s2);

#define This	(*Aliases)[which]

short	TotalNumOfNicks;

OSErr ReadNicknames(short which);
Boolean SplitNicknames (short ab);
OSErr KillNickTOC(FSSpecPtr spec);
OSErr ReadNickTOC(short which);
OSErr WriteNickTOC(short which);
Boolean NickFileOkForFastSave(short which);
void SaveDirtyPictures (short ab);
long	NickMatchFoundLo(NickStructHandle theNicknames,long theNicknamesLen,long hashName,PStr theName,short which);

/************************************************************************
 * ReadNicknames - read the list of aliases
 *  Can be called either to read the nicknames initially or simply to make
 *  sure they're all in memory
 ************************************************************************/
OSErr ReadNicknames(short which)
{
	BinAddrHandle	shortAddress;
	FSSpec spec;
	OSErr err = noErr;
	Str255 line;
	short type;
	Boolean exLine=False;
	long expOffset=0;
	long len;
	Str31 aliasCmd, noteCmd;
	Str31	currentCmd,tempName;
	LineIOD lid;
	long	i,count;
	long	currNickIndex;
	long	hashName;
	Boolean	doingAlias,doingNote;
	OSErr	theMemErr = noErr;
	long	nameOffset = 0;
	Accumulator nameAcc,dataAcc,lineAcc;
	char lookingFor;
	long firstOffset;
	long lineOffset;
	long curOffset;
	Boolean firstRead = This.hNames==nil;	// are we reading for the first time?  (or filling in)
	Boolean lastExLine;
	
	/*
	 * Setup section
	 */
	Zero(nameAcc);
	Zero(dataAcc);
	Zero(lineAcc);
	// put existing handles into accumulators
	nameAcc.data = This.hNames;
	if (nameAcc.data) nameAcc.size = nameAcc.offset = GetHandleSize(nameAcc.data);
	dataAcc.data = (void*)This.theData;
	if (dataAcc.data) dataAcc.size = dataAcc.offset = GetHandleSize(dataAcc.data);
	
	//Find the file, open it for reading, get the command names
	spec = This.spec;
	This.collapsed = FindSTRNIndex(NickFileCollapseStrn,spec.name) > 0;
	
	GetRString(aliasCmd,ALIAS_CMD);
	GetRString(noteCmd,NOTE_CMD);
	if (err=FSpOpenLine(&spec,fsRdPerm,&lid))
		return(err==fnfErr ? noErr : FileSystemError(OPEN_ALIAS,spec.name,err));

	// accumulator to hold the line we're working on
	if (theMemErr = AccuInit(&lineAcc))
		goto hitMemError;
	
	/*
	 * the main reading loop; read lines until no more
	 */
	curOffset = firstOffset = lastExLine = 0;
	while ((type=GetLine(line,sizeof(line),&len,&lid))>0)
	{
		lastExLine = exLine;
		lineOffset = curOffset;	// offset of beginning of line
		curOffset += len;	// this is the offset of the next line
		exLine = line[len-1]!='\015';
		
		// Record the offset where we first start collecting data
		if (lineAcc.offset==0) firstOffset = lineOffset;
		
		// if the last line was escaped or we're looking at a partial line, just accumulate
		if (lastExLine || exLine) // If the line was escaped or we're in the middle of a line
		{
			// first, fix up the line
			exLine = NeatenLine(line,&len) || exLine;
			
			// if the previous line was escaped and this line doesn't begin
			// with a space, add one now
			if (lastExLine && type!=LINE_MIDDLE && !issep(*line))
				if (theMemErr=AccuAddChar(&lineAcc,' ')) goto hitMemError;
			
			// if this line is further extended, append it now
			// if it's not, it will get appended inside the next if
			if (exLine && (theMemErr=AccuAddPtr(&lineAcc,line,len)))
				goto hitMemError;
		}
		
		if (!exLine) 
		{
			// Previous line was not escaped.  Is this one?
			exLine = NeatenLine(line,&len) || exLine;
			
			// and append the current line to the line accumulator
			if (theMemErr=AccuAddPtr(&lineAcc,line,len)) 	goto hitMemError;

			// if the line was escaped, just go round again; we want to deal with
			// full lines only
			if (exLine) continue;
			
			// ok, now we know that we have a full line			
			// if not empty, do parsing and add nickname here
			if (lineAcc.offset)
			{
				// first string of non-space characters is the command name
				for (i=0;i<sizeof(currentCmd) - 1;i++)
					if ((*lineAcc.data)[i] == ' ') break;
				MakePStr(currentCmd,*lineAcc.data,i);

				/*
				 * find the nickname
				 */
				// first, skip over spaces
				while (i<lineAcc.offset && (*lineAcc.data)[i]==' ') i++;
				
				// if the character we found is not a quote, then back up one,
				// because we want i to be the position just BEFORE the nickname
				if ((*lineAcc.data)[i]!='"') i--;
				
				// If we found a quote, we'll be looking for a quote to finish the nickname
				lookingFor = (*lineAcc.data)[i];
				for (count=i+1;count<lineAcc.offset;count++)
				{
					if ((*lineAcc.data)[count] == lookingFor)
						break;
				}
				// Eureka!  Copy the name into a pascal string and find its hash
				MakePStr(tempName,(*lineAcc.data)+i+1,count-i-1);
				SanitizeFN(tempName,tempName,NICK_STORED_BAD_CHAR,NICK_STORED_REP_CHAR,false);
				hashName = NickHash(tempName);
				if (lookingFor=='"') count++;	// skip the quote later when looking
																			// at the body of the line
				
				/*
				 * process the contents
				 */
				// Is this an alias command or a note command?
				doingAlias = StringSame(aliasCmd,currentCmd);
				doingNote = doingAlias ? false : StringSame(noteCmd,currentCmd);
				
				if ((doingAlias || doingNote) && *tempName) // We have a valid command and a valid nickname
				{
					// Add to toc if necessary					
					if ((currNickIndex = NickMatchFoundLo(This.theData,dataAcc.offset,hashName,tempName,which)) < 0 ) // Scan list for nickname
					{
						ASSERT(firstRead);
						if (firstRead)
						{
							// Nickname doesn't exist...create new one and clear it out
							// If it doesn't exist, add it
							NickStruct	nickInfo;
		
							// Append the name
							if (theMemErr = AccuAddPtr(&nameAcc,&tempName,*tempName + 1))
								goto hitMemError;
							
							// Set nickname info
							nickInfo.hashName = hashName;
							nickInfo.hashAddress = -1;
							nickInfo.addressesDirty = false;
							nickInfo.notesDirty = false;
							nickInfo.pornography = false;
							nickInfo.deleted = false;
							nickInfo.group = false;
							nickInfo.theAddresses = nil;
							nickInfo.theNotes = nil;
							nickInfo.nameTOCOffset = nameOffset;
							nameOffset += *tempName+1;								
		
							if (doingAlias) // Command is alias
							{
								// Put offset of address in file into nickname
								nickInfo.addressOffset = firstOffset + 1;
								// Initialize notes offset to something not valid
								nickInfo.notesOffset = -1;
								
								// Hash the addresses in semi-complicated fashion
								shortAddress = nil;
								// As a last check... see if the address is already present in a nickname file
								if (!SuckPtrAddresses (&shortAddress, LDRef (lineAcc.data) + count + 1, lineAcc.offset - count - 1, false, true, false, nil))
									nickInfo.hashAddress = NickHashString (LDRef (shortAddress));

								nickInfo.group = ContainsMultipleAddresses (shortAddress);
								UL (lineAcc.data);
								
								ZapHandle (shortAddress);
							}
							if (doingNote) // Command is note
							{
								// Put offset of notes in file into nickname
								nickInfo.notesOffset = firstOffset + 1;
								// Initialize address offset to something not valid
								nickInfo.addressOffset = -1;
							}

							//	Add nickname info to array handle
							if (theMemErr = AccuAddPtr(&dataAcc,(void*)&nickInfo,sizeof(NickStruct)))
								goto hitMemError;
							currNickIndex = dataAcc.offset/sizeof(NickStruct) - 1;
						}
					}
					
					// fill in data if necessary
					if (This.theData) {
						if (doingAlias && (*(This.theData))[currNickIndex].addressOffset==-1)
						{
							// Put offset of address in file into nickname
							(*(This.theData))[currNickIndex].addressOffset = firstOffset + 1;
							
							// trim off the command and alias
							BMD(*lineAcc.data+count+1,*lineAcc.data,lineAcc.offset-count-1);
							lineAcc.offset -= count + 1;
							AccuTrim(&lineAcc);
							
							// And fill in the data
							(*(This.theData))[currNickIndex].theAddresses = lineAcc.data;

							Zero(lineAcc);
						}
						if (doingNote && (*(This.theData))[currNickIndex].notesOffset==-1)
						{
							// Put offset of notes in file into nickname
							(*(This.theData))[currNickIndex].notesOffset = firstOffset + 1;
							
							// trim off the command and alias
							BMD(*lineAcc.data+count+1,*lineAcc.data,lineAcc.offset-count-1);
							lineAcc.offset -= count + 1;
							AccuTrim(&lineAcc);
							
							// And fill in the data
							(*(This.theData))[currNickIndex].theNotes = lineAcc.data;
							Zero(lineAcc);
						}
					}
				}
				lineAcc.offset = 0;
			}
		}
	}
	
	/*
	 * cleanup
	 */
	
	// close out a little stuff
	AccuZap(lineAcc);
	CloseLine(&lid);

	// report any errors
	if (type	|| err)
	{
		AccuZap(dataAcc);
		AccuZap(nameAcc);
		return(err ? WarnUser(ALLO_ALIAS,err) :
											FileSystemError(READ_ALIAS,spec.name,type));
	}
	
	// Success!! install the toc and names handles
	if (firstRead)
	{
		AccuTrim(&dataAcc);
		AccuTrim(&nameAcc);
		This.hNames = nameAcc.data;
		This.theData = (void*)dataAcc.data;
		CheckForNicknameBogosity (which);
	}

	return(noErr);
	
hitMemError:
	err = WarnUser(ALLO_ALIAS,theMemErr);
	CloseLine(&lid);
	AccuZap(lineAcc);
	// kill these only on first read; on reread leave existing handles alone!
	if (firstRead)
	{
		AccuZap(dataAcc);
		AccuZap(nameAcc);
	}

	return (err);
}

void CheckForNicknameBogosity (short which)

{
	Str255	beautifulName;
	short		count,
					index;
	Boolean	bogus;
	
	bogus = false;
	count = This.theData ? (GetHandleSize_ (This.theData) / sizeof (NickStruct)) : 0;
	for (index = 0; !bogus && index < count; ++index) {
		BeautifyFrom (GetNicknameNamePStr (which, index, beautifulName));
		if (!(*(This.theData))[index].deleted && (*(This.theData))[index].hashName != NickHash (beautifulName))
			bogus = true;
	}		
	This.containsBogusNicks = bogus;
}

/************************************************************************
 * NickMatchFound - find a given nickname in a nickname structure,
 * i.e. a nickname file
 * //SD - I rearranged things a bit for efficiency
 ************************************************************************/
long	NickMatchFound(NickStructHandle theNicknames,long hashName,PStr theName,short which)
{
	return (NickMatchFoundLo(theNicknames,GetHandleSize_(theNicknames),hashName,theName,which));	
}

/************************************************************************
 * NickMatchFoundLo - find a given nickname in a nickname structure,
 * i.e. a nickname file
 * //SD - I rearranged things a bit for efficiency
 ************************************************************************/
long	NickMatchFoundLo(NickStructHandle theNicknames,long theNicknamesLen,long hashName,PStr theName,short which)
{
	long i;
	Str31	tempStr, tempName;
	long stop;
	Boolean	needStringMatch = false;
	long matched = -1;
	NickStruct *theStruct, *endStruct;
	
	if (!theNicknames)
		return (-1);
		
	if (*theName > sizeof(Str31) - 1)
		return (-1);
	
	stop = (theNicknamesLen/sizeof(NickStruct));
	endStruct = (*theNicknames)+stop;
	for (theStruct=*theNicknames; theStruct<endStruct; theStruct++)
		if (theStruct->hashName == hashName && !theStruct->deleted)
			if (matched==-1) matched = theStruct-*theNicknames;		
			else {needStringMatch = True; break;}

	if (!needStringMatch) return(matched);
	
	PSCopy(tempName,theName);
	*tempName = RemoveChar(' ',tempName+1,*tempName);
	for (i=0;i<stop;i++)
	{
		if ((*theNicknames)[i].hashName == hashName && !(*theNicknames)[i].deleted)
		{
			GetNicknameNamePStr(which,i,tempStr);
			*tempStr = RemoveChar(' ',tempStr+1,*tempStr);
			if (StringSame(tempName,tempStr))
				return (i);
		}
	}

	return (-1);	

}

/************************************************************************
 * NickAddressMatchFound - find a given address in a nickname structure,
 * i.e. a nickname file
 ************************************************************************/
long	NickAddressMatchFound (NickStructHandle theNicknames, long hashAddress, PStr theAddress, short which)

{
	BinAddrHandle addresses;
	NickStruct		*theStruct,
								*endStruct;
	Handle				theAddresses;
	Str255				tempStr,
								tempAddress;
	long					stop,
								matched,
								i;
	Boolean				needStringMatch;
	
	if (!theNicknames)
		return (-1);
		
	matched					= -1;
	needStringMatch	= false;
	stop						= GetHandleSize_ (theNicknames) / sizeof (NickStruct);
	endStruct				= (*theNicknames) + stop;
	
	for (theStruct = *theNicknames; theStruct < endStruct; theStruct++)
		if (theStruct->hashAddress == hashAddress && !theStruct->deleted)
			if (matched == -1)
				matched = theStruct - *theNicknames;		
			else {
				needStringMatch = true;
				break;
			}

	if (!needStringMatch)
		return (matched);
	
	PSCopy(tempAddress, theAddress);
	*tempAddress = RemoveChar (' ', tempAddress + 1, *tempAddress);
	for (i = 0; i < stop; i++)
		if ((*theNicknames)[i].hashAddress == hashAddress && !(*theNicknames)[i].deleted)
			if (theAddresses = GetNicknameData(which,i,true,true)) {
				SuckPtrAddresses (&addresses, LDRef (theAddresses), GetHandleSize (theAddresses), False, False, False, nil);
				UL (theAddresses);
				if (addresses) {
					PCopy (tempStr, *addresses);
					*tempStr = RemoveChar (' ', tempStr + 1, *tempStr);
					ZapHandle (addresses);
					if (StringSame (tempAddress, tempStr))
						return (i);
				}
			}

	return (-1);	
}


/************************************************************************
 * RegenerateAliases - make sure the alias list is in memory
 ************************************************************************/
OSErr RegenerateAliases(short which,Boolean rebuild)
{
	OSErr err = noErr;
	UHandle hand;
	
	
	if (rebuild) ZapAliasFile(which);
	
		if (!This.theData) // If handle for data doesn't exist, create it
		{
			hand = NuHTempOK(0L);
			if (!hand) err = WarnUser(ALLO_ALIAS,MemError());
			This.theData = (NickStructHandle)hand;
			HNoPurge_(This.theData);
		}
		else // Handle exists
		{
			if (!rebuild)
				return (noErr);
		}
	
	if (!err) 
		{
			// Check to see if the TOC exists in the resource. If so, read it into the structure.
			// If not, read in the nicknames and then write out the TOC
			
			if (!rebuild)
				err = ReadNickTOC(which);
			if (err != noErr || rebuild)
				{
					ZapHandle(This.theData);
					hand = NuHTempOK(0L);
					if (!hand) err = WarnUser(ALLO_ALIAS,MemError());
					This.theData = (NickStructHandle)hand;
					HNoPurge_(This.theData);
					err = ReadNicknames(which);
					if (!err && !This.ro) {
						WriteNickTOC(which);
#ifdef VCARD
						if (!IsPluginAddressBook (which) && !IsPersonalAddressBook (which) && SplitNicknames (which))
#else
						if (!IsPluginAddressBook (which) && SplitNicknames (which))
#endif
						{
							SetAliasDirty(which);
							SaveIndNickFile (which, true);
						}
					}
				}
		}

	if (err)
		ZapHandle(This.theData);

	return(err);
}


Boolean SplitNicknames (short ab)

{
	Str255	name;
	Handle	notes;
	short		nick,
					totalNicks;
	SInt8		oldHState;
	Boolean	notesChanged;

	notesChanged = false;
	
	totalNicks = (*Aliases)[ab].theData ? (GetHandleSize_((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	for (nick = 0; nick < totalNicks; ++nick) {
		if (notes = GetNicknameData (ab, nick, false, true)) {
			oldHState = HGetState (notes);
			if (MaybeApplySplittingAlgorithm (notes) && !(*Aliases)[ab].ro) {	
				ReplaceNicknameNotes (ab, GetNicknameNamePStr (ab, nick, name), notes);
				notesChanged = true;
			}
			else
				// If we did not split the name, restore the note's previous purge state
				HSetState (notes, oldHState);
		}
	}
	return (notesChanged);
}

//
//	MaybeApplySplittingAlgorithm
//
//		Apply our name splitting algorithm to the notes if we have a full name,
//		and we have no first or last name.  This grows the notes handle and returns
//		true when a split has taken place.  Likewise, if we have a first and/or
//		last name, but no full name, employ our joining algorithm to build the
//		full name.
//
Boolean MaybeApplySplittingAlgorithm (Handle notes)

{
	Str255	nameTag,
					firstTag,
					lastTag,
					realName,
					firstName,
					lastName;
	UPtr		notesPtr,
					newPtr,
					attribute,
					value;
	Size		notesSize;
	long		attributeLength,
					valueLength;
	Boolean	notesChanged;

	notesChanged = false;
	
	*realName	 = 0;
	*firstName = 0;
	*lastName	 = 0;
	
	GetRString (nameTag, ABReservedTagsStrn + abTagName);
	GetRString (firstTag, ABReservedTagsStrn + abTagFirst);
	GetRString (lastTag, ABReservedTagsStrn + abTagLast);
	
	// Walk through the 'notes', looking for attribute/value pairs we care about.
	notesSize	= GetHandleSize (notes);
	notesPtr	= LDRef (notes);
	while ((!*realName || !*firstName || !*lastName) && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, &attributeLength, &value, &valueLength))) {
		// Check to see if we hit a tag we care about
		if (*nameTag == attributeLength && !memcmp (&nameTag[1], attribute, attributeLength))
			MakePPtr (realName, value, valueLength);
		else
		if (*firstTag == attributeLength && !memcmp (&firstTag[1], attribute, attributeLength))
			MakePPtr (firstName, value, valueLength);
		else
		if (*lastTag == attributeLength && !memcmp (&lastTag[1], attribute, attributeLength))
			MakePPtr (lastName, value, valueLength);
		notesPtr = newPtr;
	}
	UL (notes);

	// If we have a real name, but neither a first or last name, apply our splitting algorithm
	if (*realName && !*firstName && !*lastName) {
		ParseFirstLast (realName, firstName, lastName);
		// Append the first and last names to the end of our notes
		if (*firstName)
			if (!AddAttributeValuePair (notes, firstTag, &firstName[1], *firstName))
				notesChanged = true;
		if (*lastName)
			if (!AddAttributeValuePair (notes, lastTag, &lastName[1], *lastName))
				notesChanged = true;
	}
	else
	
		// If we have a first and/or last name, but no real name, create one
		if (!*realName && (*firstName || *lastName)) {
			// Apply joining algorithm
			JoinFirstLast (realName, firstName, lastName);
			if (*realName)
				if (!AddAttributeValuePair (notes, nameTag, &realName[1], *realName))
					notesChanged = true;
		}
	return (notesChanged);
}


//
//	GetTaggedFieldValue
//
//		Pull a tagged value out of the notes portion of a nickname
//
Handle GetTaggedFieldValue (short ab, short nick, PStr tag)

{
	return (GetTaggedFieldValueInNotes (GetNicknameData (ab, nick, false, true), tag));
}


Handle GetTaggedFieldValueInNotes (Handle notes, PStr tag)

{
	Handle	hValue;
	UPtr		attribute,
					value,
					notesPtr,
					newPtr;
	Size		notesSize;
	long		attributeLength,
					valueLength;
	Boolean	found;
	
	hValue = nil;
	if (notes) {
		// Walk through the 'notes', looking for attribute/value pairs.  We'll set the value of any object
		// we find
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		found			= false;
		while (!found && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, &attributeLength, &value, &valueLength))) {
			// Is this the tag we're looking for?
			if (*tag == attributeLength && !memcmp (&tag[1], attribute, attributeLength))
				found = true;
			else
				notesPtr = newPtr;
		}
		if (found)
		{
			PtrToHand (value, &hValue, valueLength);
			Tr(hValue,"\002",">");
		}
		UL (notes);
	}
	return (hValue);
}


PStr GetTaggedFieldValueStr (short ab, short nick, PStr tag, PStr value)

{
	return (GetTaggedFieldValueStrInNotes (GetNicknameData (ab, nick, false, true), tag, value));
}


PStr GetTaggedFieldValueStrInNotes (Handle notes, PStr tag, PStr value)

{
	Str255	attribute;
	UPtr		notesPtr,
					newPtr;
	Size		notesSize;
	Boolean	found;

	*value = 0;
	if (notes) {
		// Walk through the 'notes', looking for attribute/value pairs.
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		found			= false;
		while (!found && (newPtr = ParseAttributeValuePairStr (notesPtr, notesSize - (notesPtr - *notes), attribute, value))) {
			// Is this the tag we're looking for?
			if (StringSame (tag, attribute))
				found = true;
			else
				notesPtr = newPtr;
		}
		if (!found)
			*value = 0;
		else
			TrLo(value+1,*value,"\002",">");
		UL (notes);
	}
	return (value);
}


OSErr SetTaggedFieldValue (short ab, short nick, PStr tag, PStr value, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored)

{
	Handle	notes;
	OSErr		theError;

	theError = noErr;
	if (notes = GetNicknameData (ab, nick, false, true))
		theError = SetTaggedFieldValueInNotes (notes, tag, &value[1], *value, setValue, separatorIndex, ignored);
	
	return (theError);
}


OSErr SetTaggedFieldValueInNotes (Handle notes, PStr tag, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored)

{
	Str255	concatString;
	UPtr		attribute,
					originalValue,
					notesPtr,
					newPtr;
	OSErr		theError;
	Size		notesSize;
	long		attributeLength,
					originalValueLength;
	Boolean	found;
	
	TrLo(value,length,">","\002");

	theError = noErr;
	if (notes) {
		// Walk through the 'notes', looking for attribute/value pairs.
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		found			= false;
		while (!found && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, &attributeLength, &originalValue, &originalValueLength))) {
			// Is this the tag we're looking for?
			if ((*tag == attributeLength) && !memcmp (&tag[1], attribute, attributeLength))
				found = true;
			else
				notesPtr = newPtr;
		}
		UL (notes);		

		if (found) {
			switch (setValue) {
				case nickFieldReplaceExisting:
					Munger (notes, originalValue - *notes, nil, originalValueLength, value, length);
					break;
				case nickFieldAppendExisting:
					GetRString (concatString, separatorIndex);
					if (*concatString) {
						Munger (notes, originalValue - *notes + originalValueLength, nil, 0, &concatString[1], *concatString);
						originalValueLength += *concatString;
					}
					Munger (notes, originalValue - *notes + originalValueLength, nil, 0, value, length);
					break;
				case nickFieldIgnoreExisting:
					if (ignored)
						*ignored = true;
			}
			theError = MemError ();
		}
		else
			theError = AddAttributeValuePair (notes, tag, value, length);
	}

	TrLo(value,length,"\002",">");

	return (theError);
}


OSErr SetNicknameChangeBit (Handle notes, ChangeBitType changeBits, Boolean clearFirst)

{
	Str255	changeTag,
					value;
	OSErr		theError;
	long		num;
	
	theError = noErr;
	if (notes) {
		GetRString (changeTag, ABHiddenTagsStrn + abTagChangeBits);
		num = clearFirst ? 0 : GetNicknameChangeBits (notes);
		num |= (long) changeBits;
		NumToString (num, value);
		theError = SetTaggedFieldValueInNotes (notes, changeTag, &value[1], *value, nickFieldReplaceExisting, 0, nil);
	}
	return (theError);
}


long GetNicknameChangeBits (Handle notes)

{
	Str255	changeTag,
					value;
	long		num;
	
	num = 0;
	if (notes) {
		GetRString (changeTag, ABHiddenTagsStrn + abTagChangeBits);
		// Retrieve the current value
		GetTaggedFieldValueStrInNotes (notes, changeTag, value);
		// Convert it to a number
		StringToNum (value, &num);
	}
	return (num);
}



//
//	FindTaggedFieldValue
//
//		Find a particular tagged field in a nickname, returning offsets to the attribute and value.
//
Boolean FindTaggedFieldValueOffsets (short ab, short nick, PStr tag, long *attributeOffset, long *attributeLength, long *valueOffset, long *valueLength)

{
	Handle	notes;
	UPtr		notesPtr,
					newPtr,
					attribute,
					value;
	Size		notesSize;
	Boolean	found;

	found = false;
	if (notes = GetNicknameData (ab, nick, false, true)) {
		// Walk through the 'notes', looking for attribute/value pairs.
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		while (!found && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, attributeLength, &value, valueLength)))
			// Is this the tag we're looking for?
			if ((*tag == *attributeLength) && !memcmp (&tag[1], attribute, *attributeLength)) {
				*attributeOffset = attribute - *notes;
				*valueOffset		 = value - *notes;
				found = true;
			}
			else
				notesPtr = newPtr;
		UL (notes);
	}
	return (found);
}


/************************************************************************
 * RegnerateAllAliases - regnerate aliases from all files
 ************************************************************************/
OSErr RegenerateAllAliases(Boolean rebuild)
{
	short which = NAliases;
	short err = 0;
	Str255 scratch,name;
	short i,n;

	while (which--)
		if (err = RegenerateAliases(which,rebuild))
		{		
			if (which)
			{
				if (err != -108)
				{
					PCopy(name,This.spec.name);
					ComposeRString(scratch,NICK_FILE_GONE,name);
					AlertStr(OK_ALRT,Caution,scratch);
				}
				n = NAliases;
				for (i=which;i<n-1;i++) (*Aliases)[i] = (*Aliases)[i+1];
				HUnlock((Handle) Aliases);
				SetHandleBig((Handle)Aliases,(n-1)*sizeof(**Aliases));
				break;
			}
			else DieWithError(NO_MAIN_NICK,err);
		}
	
	if (!err) AliasRefCount++;
	
	ASSERT(NAliases);
	
	return(err);
}

/************************************************************************
 * ZapAliasFile - release memory for all aliases in a file
 ************************************************************************/
void ZapAliasFile(short which)
{
	short	i;
	NickStructHandle aliases = Aliases ? This.theData : nil;

	if (aliases)
	{
		for (i=0;i<NNicknames;i++)
		{
			ZapHandle((*aliases)[i].theAddresses);	// (jp) 7/31/99 Big ol' hanging leak fixed
			ZapHandle((*aliases)[i].theNotes);			// (jp) 7/31/99 Big ol' hanging leak fixed
		}
		ZapHandle(This.theData);
	}
	if (Aliases) {
		ZapHandle(This.hNames);
		This.dirty = false;
	}
}

/************************************************************************
 * ZapAliases - release memory for all aliases
 ************************************************************************/
void ZapAliases(void)
{
	short which = NAliases;

	while (which--)
		ZapAliasFile(which);
}

/************************************************************************
 * ZapAliases - release all the alias hashes
 ************************************************************************/
void ZapAliasHash(short which)
{
	AccuZap(This.addressHashes);
}

/************************************************************************
 * ZapPluginAliases - release memory for all plugin aliases
 ************************************************************************/
void ZapPluginAliases(void)
{
	short which = NAliases;

	//	All of the plugin aliases should be at the end of the list
	while (which-- && IsPluginAddressBook (which))
		ZapAliasFile(which);
	SetHandleBig_(Aliases,(which+1) * sizeof(AliasDesc));
}

/************************************************************************
 * NickHash - return a hash value on the lowercase of the name
 ************************************************************************/
long NickHash(UPtr newName)
{
	if (*newName > sizeof(Str31) - 1)
		return (-1);
	return (NickHashString (newName));
}

// Redundancy ensues... (clean all this up later)
long NickHashString (PStr string)

{
	Str255	tempStr;
	
	PCopy (tempStr, string);
	*tempStr = RemoveChar (' ', tempStr + 1, *tempStr);
	*tempStr = RemoveChar (optSpace, tempStr + 1, *tempStr);
	MyLowercaseText (tempStr + 1, *tempStr);
	return (Hash (tempStr));
}

long NickHashHandle (Handle h)

{
	Str255	tempStr;
	Size		len;
	
	*tempStr = 0;
	if (h) {
	len = GetHandleSize (h);
		tempStr[0] = MIN (len, (sizeof (tempStr) - 1));
		BlockMoveData (*h, &tempStr[1], tempStr[0]);
		*tempStr = RemoveChar (' ', tempStr + 1, *tempStr);
		MyLowercaseText (tempStr + 1, *tempStr);
	}
	return (Hash (tempStr));
}


long NickHashRawAddresses (Handle addresses, Boolean *group)

{
	BinAddrHandle	shortAddress;
	long					hashValue;

	hashValue = -1;
	
	if (addresses) {
		// Hash the addresses in semi-complicated fashion
		shortAddress = nil;
		// As a last check... see if the address is already present in a nickname file
		if (!SuckPtrAddresses (&shortAddress, LDRef (addresses), GetHandleSize (addresses), false, false, false, nil))
			hashValue = NickHashString (LDRef (shortAddress));
		UL (addresses);
		*group = ContainsMultipleAddresses (shortAddress);
		ZapHandle (shortAddress);
	}
	return (hashValue);
}


/************************************************************************
 * NickGenerateUniqueID - generate a unique ID for a nickname
 *
 *		The ID will put a counter in the upper 16 bits, and Random
 *		in the lower 16.  The counter is, effectively, unique until
 *		the user resets the 'next id' preference in settings.  But
 *		even with the sequence starting over again, each is matched
 *		with a random number, so we're in pretty good shape.
 ************************************************************************/
long NickGenerateUniqueID(void)
{
	long id;

	id = GetPrefLong (PREF_NEXT_NICK_UNIQUE_ID);
	SetPrefLong (PREF_NEXT_NICK_UNIQUE_ID, id + 1);

	id <<= 16;
	id |= ((long) Random () & 0x0000FFFF);
	
	return (id);
}


OSErr PrepAllAddressBooksForSync (void)

{
	OSErr	theError;
	short	totalABs,
				ab;
	
	theError = noErr;
	totalABs = NAliases;
	for (ab = 0; !theError && ab < totalABs; ++ab)
		theError = PrepAddressBookForSync (ab);
	return (theError);
}

OSErr PrepAddressBookForSync (short ab)

{
	Str255	idTag,
					changeBitsTag;
	OSErr		theError;
	short		totalNicks,
					nick;

	theError = noErr;
	GetRString (idTag, ABHiddenTagsStrn + abTagUniqueID);
	GetRString (changeBitsTag, ABHiddenTagsStrn + abTagChangeBits);
	totalNicks = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	for (nick = 0; !theError && nick < totalNicks; ++nick)
		theError = PrepNicknameForSync (ab, nick, idTag, changeBitsTag);
	return (theError);
}

OSErr PrepNicknameForSync (short ab, short nick, Str255 idTag, Str255 changeBitsTag)

{
	Handle	notes;
	Str255	idString,
					scratch,
					name;
	OSErr		theError;
	Boolean	notesExist,
					replaceNotes;
	
	theError = noErr;
	notes = GetNicknameData (ab, nick, false, true);
	if (!(notesExist = notes ? true : false))
		notes = NuHandle (0);
	if (notes) {
		replaceNotes = false;
		GetTaggedFieldValueStrInNotes (notes, idTag, scratch);
		if (!*scratch) {
			replaceNotes = true;
			NumToString (NickGenerateUniqueID (), idString);
			theError = SetTaggedFieldValueInNotes (notes, idTag, &idString[1], *idString, nickFieldReplaceExisting, 0, nil);
		}
		GetTaggedFieldValueStrInNotes (notes, changeBitsTag, scratch);
		if (!theError && !*scratch) {
			replaceNotes = true;
			theError = SetNicknameChangeBit (notes, changeBitAdded, false);
		}
		if (!theError && replaceNotes)
			ReplaceNicknameNotes (ab, GetNicknameNamePStr (ab, nick, name), notes);
	}
	
	if (!notesExist)
		ZapHandle (notes);
	return (theError);
}

//
//	ClearAllAddressBookChangeBits
//
//		Clears change bits as specified by the mask.
//

OSErr ClearAllAddressBookChangeBits (long mask)

{
	OSErr	theError;
	short	totalABs,
				ab;
	
	theError = noErr;
	totalABs = NAliases;
	for (ab = 0; !theError && ab < totalABs; ++ab)
		theError = ClearAddressBookChangeBits (ab, mask);
	return (theError);
}

OSErr ClearAddressBookChangeBits (short ab, long mask)

{
	OSErr	theError;
	short	totalNicks,
				nick;

	theError = noErr;
	totalNicks = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	for (nick = 0; !theError && nick < totalNicks; ++nick)
		theError = ClearNicknameChangeBits (ab, nick, mask);
	return (theError);
}

OSErr ClearNicknameChangeBits (short ab, short nick, long mask)

{
	Handle	notes;
	Str255	name;
	OSErr		theError = noErr;
	long		value;
	
	theError = noErr;
	if (notes = GetNicknameData (ab, nick, false, true)) {
		value = GetNicknameChangeBits (notes);
		if (value != (value & ~mask))
		{
			value &= ~mask;
			theError = SetNicknameChangeBit (notes, value, true);
			if (!theError)
				ReplaceNicknameNotes (ab, GetNicknameNamePStr (ab, nick, name), notes);
		}
	}
	return (theError);
}


/**********************************************************************
 * KillNickTOC - kill the toc for a file
 **********************************************************************/
OSErr KillNickTOC(FSSpecPtr spec)
{
	OSErr err = noErr;
	
	/*
	 * if the resource fork is bad or we can't open it, just remove it
	 */
	err = FSpKillRFork(spec);
	return (err);

}

/**********************************************************************
 * ReadNickTOC - read the toc for a file
 **********************************************************************/
OSErr ReadNickTOC(short which)
{
	uLong	fileModDate = AFSpGetMod(&This.spec);
	uLong	TOCModDate;
	FSSpec lSpec = This.spec;
	Boolean sane;
	OSErr	err = noErr;
	Handle structR = nil,nameR = nil,r1 = nil;
	long	structSize;
	short	numberOfNicks = 0;
	long	currentStructPos = 0,currentNamePos = 0,currNickCount;
	short refN=0;
	long	theSize = 0;
	long eof;	
	NickStructHandle	theData;
	NickTOCStruct	*pTOCInfo,*pTOCEnd;
	NickStruct		*pNick;
	short oldResF = CurResFile();
	short oldId;
	
	theData = (*Aliases)[which].theData;

	IsAlias(&This.spec,&lSpec);
	This.collapsed = FindSTRNIndex(NickFileCollapseStrn,This.spec.name) > 0;
	
	if (err=FSpRFSane(&lSpec,&sane)) return(-1);

	if (!sane)
	{
		FSpKillRFork(&lSpec);
		return (-1);
	}
	else if (-1!=(refN=FSpOpenResFile(&lSpec,fsRdPerm)))
	{
		Handle	hOldTOC;
		
		SetResLoad(False);	//	Don't load now so we can use temporary memory
		if (hOldTOC = Get1Resource(OLD_NICK_TYPE,OLD_NICK_RESID1))
		{
			//	Remove old-style TOC resources
			RemoveResource(hOldTOC);
			if (hOldTOC = Get1Resource(OLD_NICK_TYPE,OLD_NICK_RESID2))
				RemoveResource(hOldTOC);
		}
		
		for (oldId=NICK_BASE_RESID;oldId<NICK_RESID;oldId++)
		{
			if (r1=Get1Resource(NICK_TOC_TYPE,oldId)) RemoveResource(r1);
			if (r1=Get1Resource(NICK_NAMES_TYPE,oldId)) RemoveResource(r1);
		}
		
		r1 = Get1Resource(NICK_TOC_TYPE,NICK_RESID);
		SetResLoad(True);
		nameR = Get1Resource(NICK_NAMES_TYPE,NICK_RESID);
		ZapHandle(This.hNames);
		if (r1 && nameR)
		{
			DetachResource(nameR);	//	Need to keep the names in memory after res file closes
			This.hNames = nameR;	//	Save handle to nicknames
			structSize = GetResourceSizeOnDisk(r1);
			if (err = ResError())
				goto theExit;
			else
			{
				structR = NuHTempOK(structSize);
				if (!structR)
					goto theExit;
				else
				{
					ReadPartialResource(r1,0,LDRef(structR),structSize);
					if (err = ResError())
						goto theExit;

					// See if file has been modified since TOC last modified
					BlockMoveData(LDRef(structR) + currentStructPos,&TOCModDate,sizeof(TOCModDate));
					if (TOCModDate != fileModDate) // Make sure the TOC is pretty much in synch
					{
						err = 1;
						goto theExit;
					}
					currentStructPos += sizeof(TOCModDate);
					BlockMoveData(*structR + currentStructPos,&numberOfNicks, sizeof(numberOfNicks));
					currentStructPos += sizeof(numberOfNicks);
	
					// Verify the size of the resource read in is actually the size of the data we need
					theSize = numberOfNicks * sizeof(NickTOCStruct);
					if ((numberOfNicks * sizeof(NickTOCStruct) +
						sizeof(numberOfNicks) + sizeof(TOCModDate)) != structSize)
					{
						err = 1;
						goto theExit;
					}

					// Verify that the file must be empty if the toc is
					if (!numberOfNicks && FSpDFSize(&lSpec))
					{
						err = 1;
						goto theExit;
					}

					//	Load up the nick array
					SetHandleBig((Handle)theData,numberOfNicks * sizeof(NickStruct));
					if (MemError()) goto theExit;
					WriteZero(*theData,numberOfNicks * sizeof(NickStruct));
					eof = FSpDFSize(&lSpec);
					pTOCInfo = (NickTOCStruct	*)(*structR + currentStructPos);
					pTOCEnd = (NickTOCStruct	*)(*structR + structSize);
					CycleBalls();
					pNick = *theData;

					//	Fill in TOC data
					for (currNickCount=0;
							pTOCInfo < pTOCEnd && currNickCount < numberOfNicks;
							currNickCount++,pTOCInfo++,pNick++)
					{												
						pNick->hashName		 = pTOCInfo->hashName;
						pNick->hashAddress = pTOCInfo->hashAddress;
						pNick->group			 = pTOCInfo->flags & nfMultipleAddresses ? true : false;
						if ((pNick->addressOffset = pTOCInfo->addressOffset) > eof) goto theExit;	//	Bad data
						if ((pNick->notesOffset = pTOCInfo->notesOffset) > eof) goto theExit;	//	Bad data
						pNick->nameTOCOffset = currentNamePos;

						//	Advance to next name	
						currentNamePos += *(*nameR + currentNamePos) + 1;

					}
				}
			}
			CheckForNicknameBogosity (which);
		}
		else
			goto theExit;
	}

	if (numberOfNicks == 0)
	theExit:
		numberOfNicks = 0;	//	Error
		
	if (refN) CloseResFile(refN);
	if (nameR) UL(nameR);
	if (err) ZapHandle(This.hNames);
	ZapHandle(structR);
	UseResFile (oldResF);
	if (numberOfNicks == 0)
		return (-1);
	return(noErr);
}

/**********************************************************************
 * WriteNickTOC - write the toc for a file
 **********************************************************************/
OSErr WriteNickTOC(short which)
{
	uLong	fileModDate;
	FSSpec	spec = This.spec;
	OSErr	err = noErr;
	short refN;
	NickTOCStruct tempStruct;
	short	i,totalNicks = NNicknames,realCount = 0;
	Handle	structR = nil,nameR = nil,tempHandle = nil;
	Accumulator structAcc,nameAcc;
	NickStruct	*pNickInfo;
	NickStructHandle	theData;
	Boolean	fSaved = false;
	short oldResF = CurResFile();
	
	theData = (*Aliases)[which].theData;
	IsAlias(&spec,&spec);
	KillNickTOC(&spec);
	FSpCreateResFile(&spec,CREATOR,'TEXT',smSystemScript);
	fileModDate = AFSpGetMod(&spec);

	structR = NuHTempOK(0);
	nameR = NuHTempOK(0);
	
	Zero(structAcc);
	Zero(nameAcc);
	structAcc.data = (Handle)structR;
	nameAcc.data = (Handle)nameR;
	
	if (!structR || !nameR)	//SD and drop whichever one might not be nil
	{
		UseResFile (oldResF);
		return (-1);
	}
	
	realCount = 0;
	for (i=0,pNickInfo=(*theData);i<totalNicks;i++,pNickInfo++)
	{
		if (!pNickInfo->deleted)
			realCount++;
	}
	
	err = AccuAddPtr(&structAcc,(void*)&fileModDate,sizeof(fileModDate));
	if (err)
		goto exit;
	err = AccuAddPtr(&structAcc,(void*)&realCount,sizeof(realCount));
	if (err)
		goto exit;

	//	Build nick TOC struct
	for (i=0;i<totalNicks;i++)
	{
			pNickInfo=&(*theData)[i];
			if (!pNickInfo->deleted)
			{
				Str31	sName;

				//	Add nick info
				tempStruct.hashName			 = pNickInfo->hashName;
				tempStruct.hashAddress	 = pNickInfo->hashAddress;
				tempStruct.addressOffset = pNickInfo->addressOffset;
				if (pNickInfo->group)
					tempStruct.flags |= nfMultipleAddresses;
				else
					tempStruct.flags &= ~nfMultipleAddresses;
				tempStruct.notesOffset = pNickInfo->notesOffset;				
				if (err=AccuAddPtr(&structAcc,(void*)&tempStruct,sizeof(tempStruct))) goto exit;

				//	Add nick name	
				GetNicknameNamePStr(which,i,sName);
				if (err=AccuAddPtr(&nameAcc,sName,*sName+1)) goto exit;		
			}
	}
	
	AccuTrim((void*)&nameAcc);
	AccuTrim((void*)&structAcc);

	if (-1!=(refN=FSpOpenResFile(&spec,fsRdWrPerm)))
	{
		AddResource(structR,NICK_TOC_TYPE,NICK_RESID,"");
		if (!ResError())
				AddResource(nameR,NICK_NAMES_TYPE,NICK_RESID,"");

		err = ResError();
		if (!err)
		{
			UpdateResFile(refN);
			err = ResError();
			fSaved = true;
		}
		//	ALB DetachResource(structR);
		//	ALB DetachResource(nameR);
		CloseResFile(refN);
	}
	else err = ResError();
	
	
	if (!err)
		err = AFSpSetMod(&spec,fileModDate);
	
	exit:
		if (!fSaved)
		{
			//	Failed to save, get rid of the temporary handles
			ZapHandle(structR);
			ZapHandle(nameR);
		}	
	UseResFile (oldResF);
	return (err);

}		

/************************************************************************
 * NeatenLine - strip the newline from a line; if it was escaped, strip
 * the backslash, and return True
 ************************************************************************/
Boolean NeatenLine(UPtr line, long *len)
{
	if (line[*len-1]=='\015') line[--*len] = 0;
	if (line[*len-1]=='\\')
	{
		line[--*len] = 0;
		return(True);
	}
	return(False);
}


/************************************************************************
 * AliasExpansion - return pointer to expansion of an alias
 ************************************************************************/
UPtr AliasExpansion(UPtr data, long offset)
{
	UPtr ptr = data+offset;
	ptr += *ptr+3;
	return(ptr);
}


/************************************************************************
 * AddNickToTOC - add nickname to TOC
 *
 *		Note!!  This routine makes a copy of the data handle (notes or addresses)
 *		which is assigned to the TOC.  Therefore, there is no need to hang onto
 *		the handle passed in hData.
 ************************************************************************/
static short AddNickToTOC(short which,UPtr name,Handle hData,Boolean fFromAddress)
{
	long	currNickCount;
	NickStructHandle aliases = This.theData;
	OSErr	err = noErr;
	NickStruct	nickInfo;
	Str255 beautifulName;
	long	nameOffset;
	Boolean group;

	// Nickname doesn't exist...create new one and clear it out
	// If it doesn't exist, add it
	HUnlock((Handle)aliases);
	currNickCount = NNicknames; // Get nickname count
	if (currNickCount > 0)
		{
			SetHandleBig_((Handle)aliases,(currNickCount + 1)*sizeof(NickStruct)); // Add a nickname
			if (err = MemError())
				return(WarnUser(ALIAS_NEW_NICK_ERR,err));

		}
	else
		{
			ZapHandle(This.theData);	// (jp) 7/31/99 Operate on This.theData instead of aliases
			aliases = NuHTempOK(sizeof(NickStruct));
			if (!aliases)
				return(WarnUser(ALIAS_NEW_NICK_ERR,MemError()));
			else
				This.theData = aliases;	// (jp) 7/31/99 The newly created data handle
		}

	nameOffset = GetHandleSize(This.hNames);
	err = PtrPlusHand_(name,This.hNames,*name+1);
	if (err)
				return(WarnUser(ALIAS_NEW_NICK_ERR,err));

	// Fill in the basic data
	WriteZero(&nickInfo,sizeof(nickInfo));
	nickInfo.hashName = NickHash(name);
	nickInfo.hashAddress = 0;
	nickInfo.addressOffset = (-1L);
	nickInfo.notesOffset = (-1L);
	nickInfo.addressesDirty = true;
	nickInfo.notesDirty = true;
	nickInfo.pornography = false;
	nickInfo.group = false;
	nickInfo.nameTOCOffset = nameOffset;
	
	if (!This.containsBogusNicks) {
		PCopy (beautifulName, name);
		BeautifyFrom (beautifulName);
		if (nickInfo.hashName != NickHash (beautifulName))
			This.containsBogusNicks = true;
	}
	
	if (hData) // Put the address information into the nickname
		{
			Handle	hNewData;
			
			hNewData = NuHTempOK(0);
			if (!hNewData)
							return(WarnUser(ALIAS_NEW_NICK_ERR,MemError()));
			err = PtrPlusHand(*hData,hNewData,GetHandleSize_(hData));
			if (err)
						return(WarnUser(ALIAS_NEW_NICK_ERR,err));
			
			if (fFromAddress) {
				//	Data is addresses
				nickInfo.theAddresses = hNewData;
				nickInfo.hashAddress  = NickHashRawAddresses (hNewData, &group);
				nickInfo.group = group;
			}
			else
				//	Data is notes
				nickInfo.theNotes = hNewData;
		}

	//	Put nick info in nick array
	(*(aliases))[currNickCount] = nickInfo;
	
	// This data CANNOT be purged because it hasn't been written to disk
	HNoPurge((Handle)aliases);
	UL(aliases);

	return(0);
}

/************************************************************************
 * AddNickToTOCfromName - add nickname to TOC
 ************************************************************************/
short AddNickToTOCfromName(short which,UPtr name,Handle addresses)
{
	return AddNickToTOC(which,name,addresses,true);
}

/************************************************************************
 * AddNickToTOCfromNotes - add nickname to TOC from notes
 ************************************************************************/
short AddNickToTOCfromNotes(short which,UPtr name,Handle notes)
{
//	return AddNickToTOC(which,name,notes,true);	// (jp) This is wrong!!  We're setting notes, not addresses
	return AddNickToTOC(which,name,notes,false);
}


/************************************************************************
 * RemoveNamedNickname - remove the named nickname
 * The nickname CANNOT be completely removed from memory in case the user
 * discards the changes from the window.
 ************************************************************************/
void RemoveNamedNickname(short which,UPtr name)
{
	NickStructHandle aliases = This.theData;
	long	hashName = NickHash(name);
	long index = NickMatchFound(aliases,hashName,name,which); // returns index of match

	if (index >= 0 ) // the nickname exists
	{
		(*((*Aliases)[which].theData))[index].addressesDirty = true;
		(*((*Aliases)[which].theData))[index].notesDirty = true;
		(*((*Aliases)[which].theData))[index].deleted = true;
	}
}


/************************************************************************
 * NickUniq - uniquify a nicknames list
 ************************************************************************/
OSErr NickUniq(TextAddrHandle addresses,PStr sep,Boolean wantErrors)
{
	TextAddrHandle cmntOrig=nil, unOrig=nil, cmntNew=nil, unNew=nil;
	short err = noErr;
	UPtr spot;
	UPtr newSpot;
	UPtr end;
	long size;
	Boolean group, groupWas=False;
	
	err = SuckAddresses(&cmntOrig,addresses,True,wantErrors,False,nil);
	cmntNew = NuHTempOK(0);
	unNew = NuHTempOK(0);
	
	if (!err && cmntNew && unNew && cmntOrig)
	{
		for (spot = LDRef(cmntOrig);*spot && !err;spot+=*spot+2)
		{
			err = SuckPtrAddresses(&unOrig,spot+1,*spot,False,wantErrors,False,nil);
			if (!err && unOrig && **unOrig)
			{
				LDRef(unOrig);
				group = (*unOrig)[**unOrig]==':';
				for (newSpot=LDRef(unNew),end=newSpot+GetHandleSize_(unNew);
						 newSpot<end && *newSpot;
						 newSpot+=*newSpot+2)
					if (StringSame(newSpot,*unOrig)) break;
				UL(unNew);
				if (newSpot>=end || !*newSpot)
				{
					/* did NOT find it */
					if (!groupWas && GetHandleSize_(cmntNew)) PtrPlusHand_(sep+1,cmntNew,*sep);
					err = PtrPlusHand_(spot+1,cmntNew,*spot);
					if (err) break;
					err = PtrPlusHand_(*unOrig,unNew,**unOrig+2);
					if (err) break;
				}
				ZapHandle(unOrig);
				groupWas = group;
			}
		}
	}
	
	if (!err)
	{
		size = GetHandleSize_(cmntNew);
		HUnlock(addresses);
		SetHandleBig_(addresses,size);
		if (!(err=MemError()))
			BlockMoveData(*cmntNew,*addresses,size);
	}
	
	if (err<0 && wantErrors) WarnUser(MEM_ERR,err);
	
	ZapHandle(cmntOrig);
	ZapHandle(unOrig);
	ZapHandle(cmntNew);
	ZapHandle(unNew);
	return(err);
}

/************************************************************************
 * ReplaceNicknameInfo - replace nickname address or notes
 *
 *		Note!!  This routine makes a copy of the data handle (notes or addresses)
 *		which is assigned to the TOC.  Therefore, there is no need to hang onto
 *		the handle passed in hData.
 ************************************************************************/
static short ReplaceNicknameInfo(short which,UPtr theName,TextAddrHandle text,Boolean fAddresses)
{
	long	hashName = NickHash(theName);
	long 	index;
	NickStructHandle aliases = This.theData;
	OSErr	err = noErr;
	NickStruct	tempNick;
	long	textSize;
	Handle	hTemp;
	Boolean	group;

	if (theName && *theName && aliases)
	{
		index = NickMatchFound(aliases,hashName,theName,which);
		if (index<0)
			//	Not found, add nickname
			if (fAddresses)
				return (AddNickToTOCfromName(which,theName,text));
			else
				return (AddNickToTOCfromNotes(which,theName,text));
		
		//	Get nick info
		tempNick = (*aliases)[index];
				
		if (text)
		{
			//	Make a copy of addresses
			textSize = GetHandleSize(text);
			if (hTemp = NuHTempOK(textSize))
				BlockMoveData(*text,*hTemp,textSize);
			else
				//	Memory error
					return (WarnUser(ALIAS_REPLACE_NICK_ERR,MemError()));
		}
		else
			hTemp = nil;

		if (fAddresses)
		{
			//	Replace the addresses
			if (tempNick.theAddresses)
				ZapHandle(tempNick.theAddresses);	//	Dispose of former addresses
				
			// Don't let the user put crap in here; bug 4519
			if (hTemp) TransLitRes(*hTemp,GetHandleSize(hTemp),ktFlatten);

			tempNick.theAddresses = hTemp;
			tempNick.hashAddress = NickHashRawAddresses (tempNick.theAddresses, &group);
			tempNick.group = group;
			tempNick.theNotes = GetNicknameData(which,index,false,true);	//	Make sure notes are loaded
		}
		else
		{
			//	Replace notes
			if (tempNick.theNotes)
				ZapHandle(tempNick.theNotes);	//	Dispose of former notes
			tempNick.theNotes = hTemp;
			tempNick.theAddresses = GetNicknameData(which,index,true,true);	//	Make sure addresses are loaded
			tempNick.hashAddress = NickHashRawAddresses (tempNick.theAddresses, &group);
			tempNick.group = group;
		}
		if (tempNick.theAddresses)
			HNoPurge(tempNick.theAddresses);
		if (tempNick.theNotes)
			HNoPurge(tempNick.theNotes);

		UL(text);
		tempNick.addressesDirty = true;
		tempNick.notesDirty = true;
				
		//	Save temp nick info
		(*aliases)[index] = tempNick;
		SetAliasDirty(which);
	}
	return noErr;
}


/************************************************************************
 * ReplaceNicknameAddresses - replace one nickname definition with another
 ************************************************************************/
short ReplaceNicknameAddresses(short which,UPtr theName,TextAddrHandle text)
{
	return ReplaceNicknameInfo(which,theName,text,true);
}

/************************************************************************
 * ChangeNameOfNick - change the nickname
 ************************************************************************/
short ChangeNameOfNick(short which,UPtr oldName,UPtr newName)
{
	NickStructHandle	aliases = This.theData;
	long	hashName = NickHash(oldName);
	long 	index=-1;

	if (oldName && *oldName) index = NickMatchFound(aliases,hashName,oldName,which);
	if (index<0) return(-1);

	SetNickname (which, index, newName);
	
	return(0);
}


void SetNickname (short ab, short nick, UPtr name)

{
	NickStructHandle	aliases;
	NickStruct				*pNick;
	Str32							oldName;
	long							oldOffset,
										adjustment,
										nickCount,
										i;

	GetNicknameNamePStr (ab, nick, oldName);

	if (aliases = (*Aliases)[ab].theData) {
		oldOffset = (*aliases)[nick].nameTOCOffset;
	
		//	Replace the name
		Munger ((*Aliases)[ab].hNames, oldOffset, nil, *oldName + 1, name, *name + 1);

		//	New hash value
		(*aliases)[nick].hashName = NickHash (name);

		//	Calculate new name offsets if the new name is a different length
		//	Only need to adjust those that are past the one we changed
		if (adjustment = *name - *oldName) {
			
			nickCount = GetHandleSize_(aliases) / sizeof (NickStruct);
			for (i = 0, pNick = *aliases; i < nickCount; i++, pNick++)
				if (pNick->nameTOCOffset > oldOffset)
					pNick->nameTOCOffset += adjustment;
		}
		// Mark the nickname as dirty so that the alias and note entries in the nickname file get rewritten
// (9/11/00) We don't really want to do this because the addresses and note might not be in memory, in which
//					 case we'd falsely save them as empty.
//		(*aliases)[nick].addressesDirty = true;
//		(*aliases)[nick].notesDirty = true;
	}
}


/**********************************************************************
 * IsAnyNickname - is the name a nickname?
 **********************************************************************/
Boolean IsAnyNickname(PStr name)
{
	short which;
	for (which=NAliases;which--;) if (IsNickname(name,which)) return(True);
	return(False);
}

/************************************************************************
 * ReplaceNicknameNotes - replace nickname notes with another
 ************************************************************************/
short ReplaceNicknameNotes(short which,UPtr theName,TextAddrHandle text)
{
	return ReplaceNicknameInfo(which,theName,text,false);
}

/*
 *	Get the addresses or notes of a given nickname; if not in memory, it will read from disk
 */
Handle	GetNicknameData(short which,short index,Boolean wantAddresses,Boolean readFromDisk)
{
	Handle	tempHandle;
	NickStructHandle aliases = This.theData;
	FSSpec	spec;
	Boolean finished = false;
	int err;
	Str255 line;
	short type;
	Boolean exLine=False;
	long len;
	Handle	dataHandle;
	LineIOD lid;
	long	theOffset,count;
	Str31	theCmd;
	Byte lookingFor;
	Boolean group;
	
	spec = This.spec;

	
	if (!aliases || index < 0 || which < 0 || index >= NNicknames || (*aliases)[index].deleted)
		return (nil);

	
	if (wantAddresses)
		tempHandle = (*aliases)[index].theAddresses;
	else
		tempHandle = (*aliases)[index].theNotes;

	if ((*aliases)[index].addressesDirty)
		{
			if (tempHandle) HNoPurge(tempHandle);
			return (tempHandle);
		}
		
	if (tempHandle !=nil && *tempHandle !=nil)
		return (tempHandle);
		
	if (tempHandle != nil)
		ZapHandle(tempHandle);
		
	if (!readFromDisk) return(nil);	//SD - Scott, is this ok?

	if (wantAddresses)
		{
			theOffset = (*aliases)[index].addressOffset;
			GetRString(theCmd,ALIAS_CMD);
		}
	else
		{
			theOffset = (*aliases)[index].notesOffset;
			GetRString(theCmd,NOTE_CMD);
		}

	if ( theOffset >= 0)
			{
					if (err=FSpOpenLine(&spec,fsRdPerm,&lid))
					{
						if (err !=fnfErr)
							FileSystemError(OPEN_ALIAS,spec.name,err);
						return (nil);
					}
				dataHandle = NuHTempOK(0L);
				if (!dataHandle)
					{
						WarnUser(ALIAS_GET_NICK_DATA_ERR,MemError());
						return (nil);

					}
				/*
				 * Offset is from the beginning of the line; add in length of the command and length of name
				 * and two spaces
				 */
				theOffset += *theCmd + 1;
				SeekLine(theOffset - 1,&lid);
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
				
#ifdef NEVER
				while (!finished && (type=GetLine(line,sizeof(line),&len,&lid))>0)
				{
					if (type==LINE_MIDDLE && len < sizeof(line)-1) // We're not really in the middle of a line...we're at the end
																							// so we need to handle the completion of the nickname
								type = 1;
								
					if (exLine || (type==LINE_MIDDLE)) // If the line was escaped or we're in the middle of a line
					{
						len = strlen(line);
						exLine = NeatenLine(line,&len);
						if (exLine && !issep(*line)) // If line was escaped and the first character isn't a space, add one
						{
							if (err=PtrPlusHand_(" ",dataHandle,1)) break;
						}
						if (err=PtrPlusHand_(line,dataHandle,len)) break;
					}
					else
						{
							if (*line)
								if (err=PtrPlusHand_(line,dataHandle,len)) break;
							if (len < (sizeof(line) - 1 ) && len > 0) // Got less than a full line of text; therefore we are done
								{
									finished = true;
									break;
								}
						}
				}
#endif
				CloseLine(&lid);
				if (err)
					{
						WarnUser(ALIAS_GET_NICK_DATA_ERR,err);
						ZapHandle(dataHandle);
						return (nil);
					}
				HLock(dataHandle);
				len = GetHandleSize_(dataHandle);
				for (count=0;count<len;count++) if ((*dataHandle)[count]!=' ') break;
				if ((*dataHandle)[count]=='"') lookingFor = '"';
				else lookingFor = ' ';
				for (count++;count<len;count++) // Scan for space to find end of alias name
				{
					if ((*dataHandle)[count] == lookingFor)
						break;
				}
				if (lookingFor=='"') count++;
				BlockMoveData(*dataHandle + count + 1,*dataHandle,len-count-1);
				SetHandleBig_(dataHandle,len-count-1);
				len = len - count - 1;
				while (len && (*dataHandle)[len-1]=='\015')
					SetHandleBig_(dataHandle,--len);

				if (wantAddresses) {
					(*aliases)[index].theAddresses = dataHandle;
					(*aliases)[index].hashAddress = NickHashRawAddresses (dataHandle, &group);
					(*aliases)[index].group				= group;
				}
				else
					(*aliases)[index].theNotes = dataHandle;
				UL(dataHandle);
				return (dataHandle);
			}
	
	else	return (nil);
	
}

/*
 *	Get the name of a given nickname
 */
PStr	GetNicknameNamePStr(short which,short index,PStr theName)
{
	NickStructHandle aliases = This.theData;
	Handle	hNames;

	*theName = 0;	//	Initialize to null string
	if (!aliases || index < 0 || which < 0 || index >= NNicknames || (*aliases)[index].deleted)
		return (theName);

	hNames = This.hNames;
	if (!hNames || !*hNames)
		return theName;

	PCopy(theName,*(hNames)+(*aliases)[index].nameTOCOffset);
	return (theName);
		
}


/************************************************************************
 * MakeCompNick - make a nickname out of a comp window
 ************************************************************************/
#ifdef VCARD
void MakeCompNick(MyWindowPtr win, FSSpec *vcardSpec)
#else
void MakeCompNick(MyWindowPtr win)
#endif
{
	TextAddrHandle biglist;
	OSErr err = noErr;
	
	biglist = NuHTempOK(0);
	if (!biglist)
	{
		WarnUser(ALIAS_NEW_NICK_ERR,MemError());
		return;

	}
	if (err=GatherCompAddresses(win,biglist))
	{
		if (err!=paramErr) WarnUser(MEM_ERR,err);
		return;
	}

	/*
	 * and make the nickname...
	 */
	if (GetHandleSize (biglist) && **biglist)
#ifdef VCARD
		NewNick(biglist,vcardSpec,0);
#else
		NewNick(biglist,0);
#endif
	else NoteUser(NO_ADDRESSES,0);
	
	ZapHandle(biglist);
}

/**********************************************************************
 * MakeNickFromSelection - make a nickname from the selected addresses
 **********************************************************************/
void MakeNickFromSelection(MyWindowPtr win)
{
	Handle list = nil;
	Handle text;
	Handle textCopy;
	long selStart, selEnd;
		
	if (PeteGetTextAndSelection(win->pte,&text,&selStart,&selEnd)
			|| selStart>=selEnd)
		NoteUser(NO_ADDRESSES,0);
	else
	{
//	ALB 9/5/96, replaced this with SuckPtrAddresses for bug 602
//		selEnd = MIN(selEnd,selStart+250);
//		list = ZeroHandle(NuHTempBetter(selEnd-selStart+3));
//		if (!list)
//			{
//				WarnUser(ALLO_ALIAS,MemError());
//				return;
//			}
//		**list = len = selEnd-selStart;
//		BMD(*text+selStart,*list+1,len);
		// (jp) 12/12/00  Translate carriage returns to commas so we can discern individual addresses
		textCopy = text;
		if (!HandToHand (&textCopy)) {
			Tr (textCopy, "\015", ",");
		
			HLock(textCopy);
			if (!SuckPtrAddresses(&list,*textCopy+selStart,selEnd-selStart,True,True,False,nil))
			{
				if (!list || !*list || !**list)
					WarnUser(NO_ADDRESSES,0);
				else
#ifdef VCARD
					NewNick(list,nil,0);
#else
					NewNick(list,0);
#endif
				ZapHandle(list);
			}
			HUnlock(textCopy);
		}
		ZapHandle (textCopy);
	}
}

/************************************************************************
 * GatherCompAddresses - gather the addresses from a window
 ************************************************************************/
short GatherCompAddresses(MyWindowPtr win,Handle biglist)
{
	Handle littlelist;
	MessHandle messH;
	static short heads[] = {TO_HEAD,CC_HEAD,BCC_HEAD};
	short err=0;
	short h;
	Uhandle text=nil;
	HeadSpec hs;
	
	/*
	 * I vaant to suck your addresses...
	 */
	messH = Win2MessH(win);
	for (h=0;!err && h<sizeof(heads)/sizeof(short);h++)
	{
		if (CompHeadFind(messH,heads[h],&hs))
		{
			if (err=CompHeadGetText(TheBody,&hs,&text)) break;
			err=SuckAddresses(&littlelist,text,True,True,False,nil);
			if (littlelist)
			{
			  if (**littlelist)
				{
					long size=GetHandleSize_(biglist);
					if (size) SetHandleBig_(biglist,size-1);	/* strip final terminator */
					if (err=HandAndHand(littlelist,biglist)) break;
				}
				ZapHandle(littlelist);
				ZapHandle(text);
			}
		}
	}
	return(err);
}

/************************************************************************
 * MakeMessNick - make a nickname out of a message window
 ************************************************************************/
void MakeMessNick(MyWindowPtr win,short modifiers)
{
#ifdef VCARD
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	TOCHandle tocH = (*messH)->tocH;
	int sumNum = (*messH)->sumNum;
	FSSpec attSpec;
	Handle text;
	long offset;
	Boolean	foundVCard;
#endif
	TOCHandle out=GetOutTOC();
	Boolean all, quote, self;

#ifdef VCARD
	// Is a vCard attached to this message?
	foundVCard = false;
	if (IsVCardAvailable ()) {
		CacheMessage (tocH, sumNum);
		if (!(text = (*tocH)->sums[sumNum].cache))
			return;
		HNoPurge (text);
		offset = (*tocH)->sums[sumNum].bodyOffset-1;
		while (!foundVCard && (0<=(offset = FindAnAttachment(text,offset+1,&attSpec,true,nil,nil,nil)))) {
			foundVCard = IsVCardFile (&attSpec);
		}
	}
#endif
	
	ReplyDefaults(modifiers,&all,&self,&quote);

	if (out)
	{
		Boolean wasDirty = (*out)->win->isDirty;
		win = DoReplyMessage(win,all,self,False,False,0,False,False,False);
		if (!win) return;
#ifdef VCARD
		MakeCompNick(win, foundVCard ? &attSpec : nil);
#else
		MakeCompNick(win);
#endif
		CloseMyWindow(GetMyWindowWindowPtr(win));
		(*out)->win->isDirty = wasDirty;
	}
}

/************************************************************************
 * MakeMboxNick - make a nickname out of the selected messages in an mbox
 ************************************************************************/
void MakeMboxNick(MyWindowPtr win,short modifiers)
{
	TextAddrHandle addresses=nil;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	OSErr err = GatherBoxAddresses(tocH,modifiers,-1,-1,&addresses,false);
	
	if (!err)
	{
		if (**addresses)
#ifdef VCARD
			NewNick(addresses,nil,0);
#else
			NewNick(addresses,0);
#endif
		else WarnUser(NO_ADDRESSES,0);
	}
	
	ZapHandle(addresses);
}

/************************************************************************
 * GatherBoxAddresses - gather addresses from the selected messages in an mbox
 ************************************************************************/
OSErr GatherBoxAddresses(TOCHandle tocH,short modifiers,short from, short to, UHandle *addresses, Boolean caching)
{
	MyWindowPtr messWin,compWin;
  short sumNum;
	short err = 0;
	Boolean all, quote, self;
	Boolean selected = from==-1;
	
	if (selected) {from=0;to=(*tocH)->count-1;}

	ReplyDefaults(modifiers,&all,&self,&quote);
	
	if (!(*addresses=NuHTempOK(0))) return(MemError());
	for (sumNum=from;!err && sumNum<=to;sumNum++)
	{
		MiniEvents(); if (CommandPeriod) break;
		if (!selected || (*tocH)->sums[sumNum].selected)
		{
#ifdef	IMAP
			// if this is an IMAP message, make sure it's been fully fetched.
			if (tocH && (*tocH)->imapTOC && !EnsureMsgDownloaded(tocH,sumNum,false)) return (err = 1);
#endif
			if (messWin = GetAMessage(tocH,sumNum,nil,nil,False))
			{
				compWin = MessFlagIsSet(Win2MessH(messWin),FLAG_OUT) ? messWin :
													DoReplyMessage(messWin,all,self,False,False,0,False,False,caching);
				if (compWin)
				{
					err=GatherCompAddresses(compWin,*addresses);
					if (compWin != messWin) CloseMyWindow(GetMyWindowWindowPtr(compWin));
				}
				if (!IsWindowVisible(GetMyWindowWindowPtr(messWin))) CloseMyWindow(GetMyWindowWindowPtr(messWin));
			}
			else err = 1;
		}
	}
	
	if (CommandPeriod) err = userCancelled;
	
	if (err) ZapHandle(*addresses);
	return(err);
}

/************************************************************************
 * MakeCboxNick - make a nickname out of the selected messages in Out
 ************************************************************************/
void MakeCboxNick(MyWindowPtr win)
{
	Handle addresses=NuHTempOK(0);
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	MyWindowPtr compWin;
  short sumNum;
	short err = 0;
	
	if (!addresses) return;
	for (sumNum=0;!err && sumNum<(*tocH)->count;sumNum++)
	{
		MiniEvents(); if (CommandPeriod) break;
		if ((*tocH)->sums[sumNum].selected)
			if (compWin=GetAMessage(tocH,sumNum,nil,nil,False))
			{
					WindowPtr	compWinWP = GetMyWindowWindowPtr (compWin);
					err=GatherCompAddresses(compWin,addresses);
					if (!IsWindowVisible(compWinWP)) CloseMyWindow(compWinWP);
			}
			else err = 1;
	}
	
	if (!err && !CommandPeriod)
	{
		if (**addresses)
#ifdef VCARD
			NewNick(addresses,nil,0);
#else
			NewNick(addresses,0);
#endif
		else WarnUser(NO_ADDRESSES,0);
	}
	
	ZapHandle(addresses);
}

/************************************************************************
 * FlattenListWith - make an address list one to a line
 ************************************************************************/
void FlattenListWith(Handle h,Byte c)
{
  UPtr from, to;
  Boolean colon;
	
	from = to = *h;
	while (*from)
	{
		if (from[1]==';' && to!=*h && to[-1]!=':') to--; //backup over separator
	  while (*++from) *to++ = *from;	/* skip length byte, copy string */
	  colon = from[-1]==':';
		from++;													/* skip terminator */
		if (!colon) *to++ = c;					/* and add a separator */
	}
	if (to > *h) to--;
	SetHandleBig_(h,to-*h);
}

/************************************************************************
 * CommaList - make an address list have commas
 ************************************************************************/
void CommaList(Handle h)
{
  UPtr from, to;
  Boolean colon;
	
	from = to = *h;
	while (*from)
	{
		if (from[1]==';' && to!=*h && to[-1]!=':') to-=2; //backup over separator
	  while (*++from) *to++ = *from;	/* skip length byte, copy string */
	  colon = from[-1]==':';
		from++;													/* skip terminator */
		if (!colon)
		{
			*to++ = ',';					/* and add a separator */
			*to++ = ' ';					/* and add a separator */
		}
	}
	if (to > *h) to -= 2;
	SetHandleBig_(h,to-*h);
}

/************************************************************************
 * SaveAliases - save the edited aliases (if necessary)
 * returns False if the operation failed
 ************************************************************************/
Boolean SaveAliases(Boolean saveChangeBits)
{
	short 	ab;
	Boolean fResult;

	// Save each address book
	ab = NAliases;
	fResult = true;
	while (ab-- && fResult)
		fResult = SaveIndNickFile (ab, saveChangeBits);
	
	// If the address books were successfully saved, tell the Adddres Book window
	if (fResult)
		ABClean ();

	return (fResult);
}

/************************************************************************
 * SetAliasDirty - set the dirty bit for an address book, and kill the
 *   address hashes
 ************************************************************************/
void SetAliasDirty(short which)
{
	This.dirty = true;
	ZapAliasHash(which);
}

/************************************************************************
 * SaveIndNickFile - save an individual alias file
 ************************************************************************/
Boolean SaveIndNickFile(short which, Boolean saveChangeBits)
{
	Str31 aliasCmd;
	Str255 scratch;
	int err;
	long bytes,offset;
	short refN=0;
	long i, count;
	FSSpec spec,tmpSpec;
	Boolean junk;
	Handle	tempHandle;
	short		numInMemory;
	
	/*
	 * do we need to save it?
	 */
	if (!This.dirty) return(True);


	SaveDirtyPictures (which);

	/*
	 * notify the nickname completion stuff that the world is changing
	 */
	InvalCachedNicknameData();

	/*
	 * make a backup
	 */
	if (PrefIsSet(PREF_NICK_BACKUP)) NickBackup(&(This.spec));

	/*
	 * If fast save allowed, do one.  If it returns false, must do complete save
	 */
	if (!PrefIsSet(PREF_NO_NICK_FAST_SAVE) && SaveFileFast(which, saveChangeBits))
		return(true);
		
	/*
	 * find the file
	 */
	spec = This.spec;
	if (err=FSpMyResolve(&spec,&junk))
	{
		FileSystemError(SAVE_ALIAS,spec.name,err);
		return(False);
	}

	
	/*
	 * regnerate the aliases, if need be
	 */
	if (!This.theData || !*This.theData) {WarnUser(SAVE_ALIAS,0);return(False);}
	count = NNicknames; // Get nickname count
	numInMemory = 0;

	// This is going to give us a ROUGH count on the number of nicknames in memory.
	// It is possible to have the address info in memory, but not the notes...we'll just count that as
	// being in memory.
	for (i=0;i<count;i++)
	{
		// If we actually have some address info and it is in memory, then increase the count
		if ((*(This.theData))[i].addressOffset >=0 && (*(This.theData))[i].theAddresses != nil)
			numInMemory++;
		// If we actually have some notes info and it is in memory, then increase the count
		else if ((*(This.theData))[i].notesOffset >=0 && (*(This.theData))[i].theNotes != nil)
			numInMemory++;
	}
	
	// If we have more than 75% of the nicknames in memory, don't reread them.
	if (numInMemory < (count * 3/4))
	{
		err = ReadNicknames(which);
		if (err) return (false);
	}
	
	/*
	 * make && open a temp file
	 */
	//tmpSpec = spec;
	//PCat(tmpSpec.name,GetRString(scratch,TEMP_SUFFIX));
	if (err=NewTempSpec(spec.vRefNum,spec.parID,nil,&tmpSpec))
		goto done;
	if (err=FSpCreate(&tmpSpec,CREATOR,MAILBOX_TYPE,smSystemScript))
		{FileSystemError(SAVE_ALIAS,tmpSpec.name,err); goto done;}
	if (err=FSpOpenDF(&tmpSpec,fsRdWrPerm,&refN))
		{FileSystemError(OPEN_ALIAS,tmpSpec.name,err); goto done;}
	
		
	count = NNicknames; // Get nickname count

	GetRString(aliasCmd,ALIAS_CMD);
	PCatC(aliasCmd,' ');
	HLock((Handle)This.theData);
	for (i=0;!err && i<count;i++)
	{
		CycleBalls();
		if ((*(This.theData))[i].addressesDirty)		// Nickname has been modified...use in memory copy
			tempHandle = GetNicknameData(which,i,true,false);
		else // Nickname hasn't been modified, use on disk copy if necessary
			tempHandle = GetNicknameData(which,i,true,true);
		
		HLock(tempHandle);
		bytes = (tempHandle && *tempHandle && **tempHandle) ? GetHandleSize_(tempHandle) : 0;		

//		PCopy(scratch,*((*(This.theData))[i].theName));
		GetNicknameNamePStr(which,i,scratch);
		
		
		if (bytes >0 && !(*(This.theData))[i].deleted)
		{	
			GetFPos(refN,&offset);
			(*(This.theData))[i].addressOffset = offset;
		}
		else {
				(*(This.theData))[i].addressOffset = -1;
				(*(This.theData))[i].group = false;
		}
		if ((!(*(This.theData))[i].deleted) && bytes > 0 && !(err = FSWriteP(refN,aliasCmd)))
		{
			if (PIndex(scratch,' '))
			{
				PInsert(scratch,sizeof(scratch),"\p\"",scratch+1);
				PCatC(scratch,'"');
			}
			PCatC(scratch,' ');
			if (!(err=FSWriteP(refN,scratch)))
			{
				if (!(err = AWrite(refN,&bytes,*tempHandle)))
				{
					bytes = 1;
					err = AWrite(refN,&bytes,"\015");
				}
			}
			HPurge(tempHandle);
		}
		HUnlock(tempHandle);
	}

	GetRString(aliasCmd,NOTE_CMD);
	PCatC(aliasCmd,' ');
	for (i=0;!err && i<count;i++)
	{
		if ((*(This.theData))[i].addressesDirty) {	// Nickname has been modified...use in memory copy
			tempHandle = GetNicknameData(which,i,false,false);
			// Make sure the 'modified' change bit is set
			if (saveChangeBits && tempHandle && PrefIsSet (PREF_CHANGE_BITS_FOR_CONDUIT))
				SetNicknameChangeBit (tempHandle, changeBitModified, false);
		}
		else
			tempHandle = GetNicknameData(which,i,false,true);
		
		//if (!(*(This.theData))[i].deleted)
			(*(This.theData))[i].addressesDirty = false;
			(*(This.theData))[i].notesDirty = false;
		
		HLock(tempHandle);
		bytes = (tempHandle && *tempHandle && **tempHandle) ? GetHandleSize_(tempHandle) : 0;		
//		PCopy(scratch,*((*(This.theData))[i].theName));
		GetNicknameNamePStr(which,i,scratch);
		
		if (bytes >0 && !(*(This.theData))[i].deleted)
		{
			GetFPos(refN,&offset);
			(*(This.theData))[i].notesOffset = offset;
		}
		else
			(*(This.theData))[i].notesOffset = -1;

		if ((!(*(This.theData))[i].deleted) && bytes >0 && !(err = FSWriteP(refN,aliasCmd)))
		{
			if (PIndex(scratch,' '))
			{
				PInsert(scratch,sizeof(scratch),"\p\"",scratch+1);
				PCatC(scratch,'"');
			}
			PCatC(scratch,' ');
			if (!(err=FSWriteP(refN,scratch)))
			{
				if (!(err = AWrite(refN,&bytes,*tempHandle)))
				{
					bytes = 1;
					err = AWrite(refN,&bytes,"\015");
				}
			}
			HPurge(tempHandle);
		}
		HUnlock(tempHandle);
		if (err) {FileSystemError(SAVE_ALIAS,tmpSpec.name,err); goto done;}
	}

	UL(This.theData);

	GetFPos(refN,&bytes);
	SetEOF(refN,bytes);
	MyFSClose(refN); refN = 0;

	/* do the deed */
	if (!err) err = ExchangeAndDel(&tmpSpec,&spec);
	if (!err) This.dirty = False;

	WriteNickTOC(which);

done:
	if (This.theData) UL(This.theData);
	if (refN) MyFSClose(refN);
	if (err) FSpDelete(&tmpSpec);
	return(err==noErr);

}


/************************************************************************
 * SaveFileFast - incremental save of the nickname file
 * Currently assumes that all notes commands follow all alias commands.
 * Fast save must not be done if this isn't the case
 ************************************************************************/
Boolean SaveFileFast (short which, Boolean saveChangeBits)
{
	NickOffSetSortType **addressOffsetHandle;
	NickOffSetSortType **notesOffsetHandle;
	NickOffSetSortType	dummyValue;
	short	numOfNicks = NNicknames;
	long	theSize;
	short	count,tempCount;
	FSSpec spec,tmpSpec;
	OSErr	err = fnfErr;
	long bytes;
	short tempRefN=0,nickRefN = 0;
	Str31 aliasCmd;
	long	cleanStartOffset,cleanStopOffset,cleanStartIndex,cleanStopIndex;
	Boolean junk;
	Str255 scratch;
	Handle	tempHandle = nil;
	Boolean	dirty;
	Boolean	deleted;
	long	offset;
	short	theIndex,i;
	long	firstNoteStart;
	long	bytesToShift;
	
	char	theChar;
	
	/*
	 * first make sure the file is ordered properly
	 */
	if (!NickFileOkForFastSave(which)) return(false);
	
	addressOffsetHandle=NuHTempOK(sizeof(NickOffSetSortType) * (numOfNicks + 1));
	notesOffsetHandle=NuHTempOK(sizeof(NickOffSetSortType) * (numOfNicks + 1));

	if (addressOffsetHandle == nil || notesOffsetHandle == nil)
	{
		err = fnfErr;
		goto done;
	}

	TotalNumOfNicks = numOfNicks;

	HLock((Handle)addressOffsetHandle);
	HLock((Handle)notesOffsetHandle);
	for (count = 0; count < numOfNicks; count++)
		{
			(*addressOffsetHandle)[count].offset = (*(This.theData))[count].addressOffset;
			(*addressOffsetHandle)[count].nickIndex = count;
			(*notesOffsetHandle)[count].offset = (*(This.theData))[count].notesOffset;
			(*notesOffsetHandle)[count].nickIndex = count;
		}


	dummyValue.offset = -1000;
	dummyValue.nickIndex = numOfNicks + 1000;
	theSize = GetHandleSize_(addressOffsetHandle) - sizeof(NickOffSetSortType);
	BlockMoveData(&dummyValue,*addressOffsetHandle + (theSize/sizeof(NickOffSetSortType)),sizeof(NickOffSetSortType));
	BlockMoveData(&dummyValue,*notesOffsetHandle + (theSize/sizeof(NickOffSetSortType)),sizeof(NickOffSetSortType));

	QuickSort((void *)(*addressOffsetHandle),sizeof(NickOffSetSortType),0,numOfNicks,(void*)NickOffsetCompare,(void*)NickOffsetSwap);
	SetHandleBig_((Handle)addressOffsetHandle,theSize);
	
	QuickSort((void *)(*notesOffsetHandle),sizeof(NickOffSetSortType),0,numOfNicks,(void*)NickOffsetCompare,(void*)NickOffsetSwap);
	SetHandleBig_((Handle)notesOffsetHandle,theSize);

	HLock((Handle)addressOffsetHandle);
	HLock((Handle)notesOffsetHandle);

	firstNoteStart = -1;


	// Find the offset of the first non-blank note
	for (count=0;count<numOfNicks;count++)
	{
			theIndex = (*notesOffsetHandle)[count].nickIndex;
			firstNoteStart = (*(This.theData))[theIndex].notesOffset;
			
			if (firstNoteStart >= 0)
				break;
	}
	/*
	 * find the file
	 */
	spec = This.spec;
	if (err=FSpMyResolve(&spec,&junk))
		return(False);



	/*
	 * make && open a temp file
	 */
//	tmpSpec = spec;
//	PCat(tmpSpec.name,GetRString(scratch,TEMP_SUFFIX));
	if (err=NewTempSpec(spec.vRefNum,spec.parID,nil,&tmpSpec))
		goto done;
	if (err=FSpCreate(&tmpSpec,CREATOR,MAILBOX_TYPE,smSystemScript))
		goto done;
	if (err=FSpOpenDF(&tmpSpec,fsRdWrPerm,&tempRefN))
		goto done;

	if (err=FSpOpenDF(&spec,fsRdPerm,&nickRefN))
		goto done;

	cleanStartOffset = -1;
	cleanStopOffset = -1;
	cleanStartIndex = -1;
	cleanStopIndex = -1;

	tempHandle = NuHTempOK(0);
	if (tempHandle == nil)
	{
		err = fnfErr;
		goto done;
	}
	HLock((Handle)This.theData);

	count = 0;
	while (count <numOfNicks)
	
	{
		cleanStartIndex = count - 1;
		do
		{
			cleanStartIndex++;
			if (cleanStartIndex==numOfNicks) break;
			theIndex = (*addressOffsetHandle)[cleanStartIndex].nickIndex;
			dirty = (*(This.theData))[theIndex].addressesDirty;
			deleted = (*(This.theData))[theIndex].deleted;
			offset = (*(This.theData))[theIndex].addressOffset;
		}
		while ((dirty || deleted || offset < 0) && cleanStartIndex<numOfNicks);

		if (cleanStartIndex == numOfNicks)
			break;

		theIndex = (*addressOffsetHandle)[cleanStartIndex].nickIndex;
		cleanStartOffset = (*(This.theData))[theIndex].addressOffset;

		cleanStopIndex = cleanStartIndex - 1;
		
		do
		{
			cleanStopIndex++;
			if (cleanStopIndex >= numOfNicks)
				break;
			theIndex = (*addressOffsetHandle)[cleanStopIndex].nickIndex;
			dirty = (*(This.theData))[theIndex].addressesDirty;
			deleted = (*(This.theData))[theIndex].deleted;
			offset = (*(This.theData))[theIndex].addressOffset;
		}
		while ((!dirty && !deleted && offset >= 0) && cleanStopIndex<numOfNicks);


		if (cleanStopIndex >= numOfNicks && !(*((*Aliases)[which].theData))[(*addressOffsetHandle)[numOfNicks - 1].nickIndex].addressesDirty && !(*((*Aliases)[which].theData))[(*addressOffsetHandle)[numOfNicks - 1].nickIndex].deleted)
			{
				 if (firstNoteStart >= 0)
				 		cleanStopOffset = firstNoteStart;
				 else
				 		GetEOF(nickRefN,&cleanStopOffset);
				 	cleanStopIndex = numOfNicks;

			}
		else
		{
				theIndex = (*addressOffsetHandle)[cleanStopIndex].nickIndex;
				cleanStopOffset = (*(This.theData))[theIndex].addressOffset;
		}

		if (cleanStartOffset <= 0 )
			cleanStartOffset = 1;

		if (err = SetFPos(nickRefN,fsFromStart,cleanStartOffset - 1))
			goto done;
		GetFPos(tempRefN,&bytes);
		bytesToShift = cleanStartOffset - bytes - 1;
		bytes = cleanStopOffset-cleanStartOffset;
		if (cleanStartOffset >=0 && bytes > 0)
		{
			long	readBytes;

			SetHandleBig_(tempHandle,bytes);
			if (err = MemError())
				goto done;
			HLock(tempHandle);
			readBytes = bytes;
			if (err = ARead(nickRefN,&readBytes,*tempHandle))
				goto done;
			if (readBytes != bytes)
			{
				err = readErr;
				goto done;
			}
			if (err = AWrite(tempRefN,&bytes,*tempHandle))
				goto done;
			HUnlock(tempHandle);

			for (tempCount = cleanStartIndex; tempCount < cleanStopIndex; tempCount++)
			{
				theIndex = (*addressOffsetHandle)[tempCount].nickIndex;
				if ((*(This.theData))[theIndex].addressOffset >=0 && !(*(This.theData))[theIndex].deleted)
					(*(This.theData))[theIndex].addressOffset-=bytesToShift;
			}
		}
		count = cleanStopIndex + 1;
	}
	
	ZapHandle(tempHandle); // leftover from last loop.  SD 4/16
	
	SetFPos(tempRefN,fsFromLEOF,-1);
	bytes = 1;
	ARead(tempRefN,&bytes,&theChar);
	GetEOF(tempRefN,&bytes);
	SetFPos(tempRefN,fsFromStart,bytes);
	if (theChar != '\015' && bytes >0)
	{
		bytes = 1;
		err = AWrite(tempRefN,&bytes,"\015");
	}

	GetRString(aliasCmd,ALIAS_CMD);
	PCatC(aliasCmd,' ');
	HLock((Handle)This.theData);
	for (i=0;!err && i<numOfNicks;i++)
	{
		CycleBalls();
		if ((*(This.theData))[i].addressesDirty && !(*(This.theData))[i].deleted) // Nickname has been modified...use in memory copy
		{
			tempHandle = GetNicknameData(which,i,true,false);
		
			HLock(tempHandle);
			if (tempHandle != nil && *tempHandle != nil && ** tempHandle != nil)
				bytes = GetHandleSize_(tempHandle);		
			else
				bytes = 0;
	
			GetNicknameNamePStr(which,i,scratch);
		
			
			if (bytes >0 && !(*(This.theData))[i].deleted)
			{	
				GetFPos(tempRefN,&offset);
				(*(This.theData))[i].addressOffset = offset;
			}
			else {
					(*(This.theData))[i].addressOffset = -1;
					(*(This.theData))[i].group = false;
			}
			if ((!(*(This.theData))[i].deleted) && bytes > 0 && !(err = FSWriteP(tempRefN,aliasCmd)))
			{
				if (PIndex(scratch,' '))
				{
					PInsert(scratch,sizeof(scratch),"\p\"",scratch+1);
					PCatC(scratch,'"');
				}
				PCatC(scratch,' ');
				if (!(err=FSWriteP(tempRefN,scratch)))
				{
					if (!(err = AWrite(tempRefN,&bytes,*tempHandle)))
					{
						bytes = 1;
						err = AWrite(tempRefN,&bytes,"\015");
					}
				}
				HPurge(tempHandle);
			}
			HUnlock(tempHandle);
		}
		else if ((*(This.theData))[i].deleted) {
			(*(This.theData))[i].addressOffset = -1;
			(*(This.theData))[i].group = false;
		}

	}



	tempHandle = NuHTempOK(0);
	if (tempHandle == nil)
		goto done;

	// Do the notes

	count = 0;
	while (count <numOfNicks)
		{
		cleanStartIndex = count - 1;
		do
		{
			cleanStartIndex++;
			if (cleanStartIndex==numOfNicks) break;
			theIndex = (*notesOffsetHandle)[cleanStartIndex].nickIndex;
			dirty = (*(This.theData))[theIndex].addressesDirty;
			deleted = (*(This.theData))[theIndex].deleted;
			offset = (*(This.theData))[theIndex].notesOffset;
		}
		while ((dirty || deleted || offset < 0) && cleanStartIndex<numOfNicks);

		if (cleanStartIndex == numOfNicks)
			break;

		theIndex = (*notesOffsetHandle)[cleanStartIndex].nickIndex;
		cleanStartOffset = (*(This.theData))[theIndex].notesOffset;

		cleanStopIndex = cleanStartIndex - 1;
		
		do
		{
			cleanStopIndex++;
			if (cleanStopIndex >= numOfNicks)
				break;
			theIndex = (*notesOffsetHandle)[cleanStopIndex].nickIndex;
			dirty = (*(This.theData))[theIndex].addressesDirty;
			deleted = (*(This.theData))[theIndex].deleted;
			offset = (*(This.theData))[theIndex].notesOffset;
		}
		while ((!dirty && !deleted && offset >= 0) && cleanStopIndex<numOfNicks);


		if (cleanStopIndex >= numOfNicks && !(*((*Aliases)[which].theData))[(*notesOffsetHandle)[numOfNicks - 1].nickIndex].addressesDirty && !(*((*Aliases)[which].theData))[(*notesOffsetHandle)[numOfNicks - 1].nickIndex].deleted)
			{
				 	GetEOF(nickRefN,&cleanStopOffset);
				 	cleanStopIndex = numOfNicks;
			}
		else
		{
				theIndex = (*notesOffsetHandle)[cleanStopIndex].nickIndex;
				cleanStopOffset = (*(This.theData))[theIndex].notesOffset;
		}

		if (cleanStartOffset > 0)
		{
			if (err = SetFPos(nickRefN,fsFromStart,cleanStartOffset - 1))
				goto done;
			GetFPos(tempRefN,&bytes);
			bytesToShift = cleanStartOffset - bytes - 1;
			bytes = cleanStopOffset-cleanStartOffset;
			if (cleanStartOffset >=0 && bytes >= 0)
			{
				long	readBytes;

				SetHandleBig_(tempHandle,bytes);
				if (err=MemError())
					goto done;
				HLock(tempHandle);
				readBytes = bytes;
				if (err = ARead(nickRefN,&readBytes,*tempHandle))
					goto done;
				if (readBytes != bytes)
				{
					err = readErr;
					goto done;
				}
				if (err = AWrite(tempRefN,&bytes,*tempHandle))
					goto done;
				HUnlock(tempHandle);
				for (tempCount = cleanStartIndex; tempCount < cleanStopIndex; tempCount++)
				{
					theIndex = (*notesOffsetHandle)[tempCount].nickIndex;
					if ((*(This.theData))[theIndex].notesOffset >= 0 && !(*(This.theData))[theIndex].deleted)
						(*(This.theData))[theIndex].notesOffset-=bytesToShift;
				}
			}
		}
	
		count = cleanStopIndex + 1;
	}

	ZapHandle(tempHandle);

	SetFPos(tempRefN,fsFromLEOF,-1);
	bytes = 1;
	ARead(tempRefN,&bytes,&theChar);
	GetEOF(tempRefN,&bytes);
	SetFPos(tempRefN,fsFromStart,bytes);
	if (theChar != '\015' && bytes > 0)
	{
		bytes = 1;
		err = AWrite(tempRefN,&bytes,"\015");
	}


	GetRString(aliasCmd,NOTE_CMD);
	PCatC(aliasCmd,' ');
	if (This.theData)
	{
		HLock((Handle)This.theData);
		for (i=0;!err && i<numOfNicks;i++)
		{
			CycleBalls();
			if ((*(This.theData))[i].addressesDirty)
			{
				tempHandle = GetNicknameData(which,i,false,false);
				
				// Make sure the 'modified' change bit is set
				if (saveChangeBits && tempHandle && PrefIsSet (PREF_CHANGE_BITS_FOR_CONDUIT))
					SetNicknameChangeBit (tempHandle, changeBitModified, false);
			
				(*(This.theData))[i].addressesDirty = false;
				(*(This.theData))[i].notesDirty = false;	// ...for now
			
				HLock(tempHandle);
				if (tempHandle != nil && *tempHandle != nil && **tempHandle != nil)
					bytes = GetHandleSize_(tempHandle);		
				else
					bytes = 0;
				
				if (!(*(This.theData))[i].deleted)
					GetNicknameNamePStr(which,i,scratch);
			
				
				if (bytes >0 && !(*(This.theData))[i].deleted)
				{	
					GetFPos(tempRefN,&offset);
					(*(This.theData))[i].notesOffset = offset;
				}
				else
						(*(This.theData))[i].notesOffset = -1;
					
				if ((!(*(This.theData))[i].deleted) && bytes > 0 && !(err = FSWriteP(tempRefN,aliasCmd)))
				{
					if (PIndex(scratch,' '))
					{
						PInsert(scratch,sizeof(scratch),"\p\"",scratch+1);
						PCatC(scratch,'"');
					}
					PCatC(scratch,' ');
					if (!(err=FSWriteP(tempRefN,scratch)))
					{
						if (!(err = AWrite(tempRefN,&bytes,*tempHandle)))
						{
							bytes = 1;
							err = AWrite(tempRefN,&bytes,"\015");
						}
					}
				}
				if (tempHandle)
				{
					HPurge(tempHandle);
					HUnlock(tempHandle);
				}
			}
			
		}
		UL(This.theData);
	}

	tempHandle = nil;


	GetFPos(tempRefN,&bytes);
	SetEOF(tempRefN,bytes);
	MyFSClose(tempRefN); tempRefN = 0;
	MyFSClose(nickRefN); nickRefN = 0;

	/* do the deed */
	if (!err) err = ExchangeAndDel(&tmpSpec,&spec);
	if (!err) This.dirty = False;

	if (!err)
		WriteNickTOC(which);

done:
	if (This.theData) UL(This.theData);
	if (tempRefN) MyFSClose(tempRefN);
	if (nickRefN) MyFSClose(nickRefN);
	if (err) FSpDelete(&tmpSpec);
	UL(addressOffsetHandle);
	UL(notesOffsetHandle);
	ZapHandle(addressOffsetHandle);
	ZapHandle(notesOffsetHandle);
	UL(This.theData);
	return(err==noErr);


}


//
//	Nickname photos are initially saved into the temporary items folder.
//	When we save the address book, we need to move these files into the
//	photo album and modify tthe nickname 'picture' tag to point to the
//	new location.
//

void SaveDirtyPictures (short ab)

{
	NickStructHandle	theData;
	NickStructPtr			pData;
	FInfo							fInfo;
	FSSpec						photoSpec,
										urlSpec;
	Str255						pictureTag,
										nickname,
										url;
	Handle						urlString;
	OSErr							theError;
	short							nick,
										numNicks;
	Boolean						alreadyExists;
	
	GetRString (pictureTag, ABReservedTagsStrn + abTagPicture);
	
	theError = noErr;
	if (theData = (*Aliases)[ab].theData) {
		numNicks = GetHandleSize_ (theData) / sizeof (NickStruct);
		for (nick = 0, pData = *theData; nick < numNicks; nick++, pData++)
			// Does this nickname have a dirty picture?
			if (!pData->deleted)
				if (pData->pornography) {
					// Grab the current 'file' URL from the notes, and turn it into an FSSpec
					if (urlString = GetTaggedFieldValue (ab, nick, pictureTag)) {
							theError = URLStringToSpec (urlString, &urlSpec);
						// Make an FSSpec for the photo in the Photo Album
						if (!theError)
							theError = GetPhotoSpec (&photoSpec, ab, nick, &alreadyExists);
						if (!theError && !alreadyExists) {
							theError = FSpGetFInfo (&urlSpec, &fInfo);
							if (!theError)
								theError = FSpCreate (&photoSpec, fInfo.fdCreator, fInfo.fdType, smSystemScript);
							if (!theError)
								theError = FSpSetFInfo (&photoSpec, &fInfo);
						}
						// Swap specs and delete the temporary
						if (!theError)
							theError = ExchangeAndDel (&urlSpec, &photoSpec);
						// Make a url for the file's permanent location
						if (!theError) {
							SetTaggedFieldValue (ab, nick, pictureTag, MakeFileURL (url, &photoSpec, 0), nickFieldReplaceExisting, 0, nil);
							pData->pornography = false;
						}
						else
							ComposeStdAlert (Caution, NICK_PHOTO_COULD_NOT_SAVE, GetNicknameNamePStr (ab, nick, nickname), theError);
					}
				}
	}
}


//
//	URLStringToSpec
//
//		Take a URL and turn it into an FSSpec
//

OSErr URLStringToSpec (StringHandle urlString, FSSpec *spec)

{
	Str255	fullPath,
					proto,
					host;
	Ptr			query;
	long		urlSize,
					origQueryLen,
					queryLen;
	OSErr		theError;
	
	theError = ParseURLPtr (LDRef (urlString), urlSize = GetHandleSize (urlString), proto, host, &query, &queryLen);
	origQueryLen = queryLen;
	if (!theError)
		switch (FindSTRNIndex (ProtocolStrn, proto)) {
			case proFile:
				TrLo (query, queryLen, "/", ":");
				FixURLPtr (query, &queryLen);
				MakePPtr (fullPath, query, queryLen);
				theError = FSMakeFSSpec (0, 0, fullPath, spec);
				break;
		}
	UL (urlString);
	if (!theError) {
		SetHandleBig (urlString, urlSize + (queryLen - origQueryLen));
		IsAlias (spec, spec);
	}
	return (theError);
}


/************************************************************************
 * NickFileOkForFastSave - see if all addresses come before all notes
 ************************************************************************/
Boolean NickFileOkForFastSave(short which)
{
	short n = NNicknames;
	long minNote = 0x7fffffff;
	long maxAlias = 0;
	long note, alias;
	
	while (n--)
	{
		note = (*(This.theData))[n].notesOffset;
		alias = (*(This.theData))[n].addressOffset;
		if (note>=0) minNote = MIN(minNote,note);
		if (alias>=0) maxAlias = MAX(maxAlias,alias);
		if (minNote <= maxAlias) return(false);	// at least one note comes before at least one alias
	}
	return(true);	// all aliases come after all notes
}


/************************************************************************
 * NickOffsetCompare - compare two names
 ************************************************************************/
int NickOffsetCompare(NickOffSetSortType *n1, NickOffSetSortType *n2)
{
	int result;

	if (n1->nickIndex>=TotalNumOfNicks) return(1);
	if (n2->nickIndex>=TotalNumOfNicks) return(-1);

	if (n1->offset == n2->offset)
		result = 0;
	else
		result = n1->offset > n2->offset?1:-1;


	return(result);
}


/************************************************************************
 * NickNameSwap - swap two names
 ************************************************************************/
void NickOffsetSwap(NickOffSetSortType *n1, NickOffSetSortType *n2)
{
	NickOffSetSortType temp;

	BlockMoveData(n2,&temp,sizeof(NickOffSetSortType));
	BlockMoveData(n1,n2,sizeof(NickOffSetSortType));
	BlockMoveData(&temp,n1,sizeof(NickOffSetSortType));
}


/************************************************************************
 * AppendTextToNick - append (comma separated) addresses to a nickname
 ************************************************************************/
OSErr AddTextToNick(short which, PStr name, Handle text,Boolean append)
{
	long	hashName = NickHash(name);
	long 	index;
	NickStructHandle aliases = This.theData;
	OSErr err = noErr;
	Handle tempHandle;
	
//	AliasWinGonnaSave();
	index = NickMatchFound(aliases,hashName,name,which);
	
	if (append)
	{
		tempHandle = GetNicknameData(which,index,true,true);
		err = PtrPlusHand_(",",text,1);
			if (!err) err = PtrPlusHand_(LDRef(tempHandle),text,GetHandleSize_(GetNicknameData(which,index,true,true)));
		UL(tempHandle);
		if (err) {WarnUser(MEM_ERR,err); return(err);}
		UL(This.theData);
	}
	
	NickUniq(text,"\p,",True);
	ReplaceNicknameAddresses(which,name,text);
	SetAliasDirty(which);
	if (AliasWinIsOpen())
		AliasWinRefresh();
	else
		SaveAliases(true);
	return err;
}


/************************************************************************
 * ReadNickFileList - get list of nickfiles
 ************************************************************************/
void ReadNickFileList(FSSpec *pSpec, AddressBookType type, Boolean reread)
{
	Str31 name;
	CInfoPBRec hfi;
	AliasDesc ad;
	Boolean multipleNickFiles;

	multipleNickFiles = false;
	Zero(ad);	
	ad.type = type;
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(pSpec->vRefNum,pSpec->parID,&hfi))
		if (hfi.hFileInfo.ioFlFndrInfo.fdType=='TEXT' || hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			multipleNickFiles = true;
			SimpleMakeFSSpec(pSpec->vRefNum,pSpec->parID,name,&ad.spec);
			if (!CanWrite(&ad.spec,&ad.ro))
			{
				ad.ro = !ad.ro;	/* opposite sense! */
				if (PtrPlusHand_(&ad,Aliases,sizeof(ad))) DieWithError(MEM_ERR,MemError());
				if (type == pluginAddressBook && reread) FSpKillRFork(&ad.spec);
			}
			else
			{
				Str255 scratch;
				ComposeRString(scratch,NICK_FILE_GONE,ad.spec.name);
				AlertStr(OK_ALRT,Caution,scratch);
			}
		}
	if (multipleNickFiles)
		UseFeature (featureMultipleNicknameFiles);
}

/************************************************************************
 * BuildAddressHashes - build a hash table for all the expanded addresses
 ************************************************************************/
OSErr BuildAddressHashes(short which)
{
	Accumulator a;
	NickStructHandle nicks = (*Aliases)[which].theData;
	short n = NNicknames;
	TextAddrHandle tempHandle=nil;
	Str31 s;
	OSErr err = noErr;
	
	Zero(a);
	
	for (n--;!err && n>=0;n--)
	{
		CycleBalls();
		
		if (!(*This.theData)[n].deleted)
		{
			if ((*(This.theData))[n].addressesDirty)		// Nickname has been modified...use in memory copy
				tempHandle = GetNicknameData(which,n,true,false);
			else // Nickname hasn't been modified, use on disk copy if necessary
				tempHandle = GetNicknameData(which,n,true,true);
			
			// main addresses
			if (tempHandle)
			{
				err = AddHandleToAddressHashes(tempHandle,&a);
				if (!(*(This.theData))[n].addressesDirty) HPurge(tempHandle);
			}
			
			// other addresses
			if (!err && (tempHandle = GetTaggedFieldValue(which,n,GetRString(s,ABReservedTagsStrn+abTagOtherEmail))))
			{
				err = AddHandleToAddressHashes(tempHandle,&a);
				ZapHandle(tempHandle);
			}
		}
	}
	
	// Did we win?
	if (!err)
	{
		AccuTrim(&a);
		(*Aliases)[which].addressHashes = a;
	}
	else
		AccuZap(a);
	
	return err;	
}

/************************************************************************
 * AddHandleToAddressHashes - add addresses from a handle to a hash accumulator
 ************************************************************************/
OSErr AddHandleToAddressHashes(TextAddrHandle sourceHandle,AccuPtr a)
{
	BinAddrHandle rawHandle=nil;
	BinAddrHandle expandedHandle=nil;
	Str255 oneAddr;
	long spot, size;
	uLong hash;
	OSErr err = noErr;
	
	if (sourceHandle)
	if (!SuckAddresses(&rawHandle,sourceHandle,false,false,true,nil))
	if (!ExpandAliases(&expandedHandle,rawHandle,0,false))
	{
		size = GetHandleSize(expandedHandle)-1;
		for (spot=0;!err && spot<size;spot += (*expandedHandle)[spot]+2)
		{
			PCopy(oneAddr,(*expandedHandle)+spot);
			MyLowerStr(oneAddr);
			hash = Hash(oneAddr);
			err = AccuAddPtr(a,&hash,sizeof(hash));
		}
	}
	
	ZapHandle(expandedHandle);
	ZapHandle(rawHandle);
	return err;
}


/************************************************************************
 * ReadPluginNickFiles - get list of plugin nickfiles
 ************************************************************************/
void ReadPluginNickFiles(Boolean reread)
{
	FSSpec folderSpec;

	ETLGetPluginFolderSpec(&folderSpec,PLUGIN_NICKNAMES);
	folderSpec.parID = SpecDirId(&folderSpec);
	*folderSpec.name = 0;
	ReadNickFileList(&folderSpec,pluginAddressBook,reread);
	if (reread) RegenerateAllAliases(false);
}

//
//	ParseFirstLast
//
//	Algorithm
//
//		1. Set the name pointer to point to the first name (or last if asian)
//		2. Read a token
//					� No more!  Go to step 10
//		3. Is the token an Honorific?
//					� Yes!  Append it onto the honorific string
//					� Go to step 2
//		4. Is the token a Qualifier?
//					� Yes!  Append it onto the qualifier string
//					� Go to step 2
//		6. If we've now reached the first comma
//					� Set the name pointer to point to the last name
//		7. Append the token onto the name pointer
//		8. Switch the name pointer to the "other" name
//		9. Go to step 2
//	 10. Did we find any commas?
//					� No!  Make the "other" name
//

PStr ParseFirstLast (PStr realName, PStr firstName, PStr lastName)

{
	Str255	qualifiers,
					honorifics,
					name1,
					name2,
					token,
					tokenCopy;
	PStr		name;
	UPtr		spot;
	short		numQualifiers,
					numHonorifics,
					numCommas,
					numName1,
					numName2,
					*numPtr;
	char		lastTokenSeperator,
					lastDelimiter;
					
				
	*name1				= 0;
	*name2				= 0;
	*qualifiers		= 0;
	*honorifics		= 0;
	numQualifiers	= 0;
	numHonorifics	= 0;
	numCommas			= 0;
	numName1			= 0;
	numName2			= 0;
	lastTokenSeperator	= 0;
	
	// Some very lazy people fail to use spaces when representing their name (J.P.Morgan)
	// so we'll do the work for them and insert spaces where appropriate
	ScanNameForSpaces (realName);
	
	name = name1;
	numPtr = &numName1;

	// Loop through all the tokens in the name
	spot = realName + 1;
	while (PToken (realName, token, &spot, " ,")) {
		PCopy (tokenCopy, token);
		
		// Cleanup the token a bit before giving the qualifiers and honorifics a shot
		PStripChar (tokenCopy, '.');
		
		// Is the token either a name qualifier or a high falutin' honorific?
		if (FindSTRNIndex (NameQualifiersStrn, tokenCopy)) {
			if (numQualifiers && lastTokenSeperator)
				PCatC (qualifiers, lastTokenSeperator);
			PCat (qualifiers, token);
			++numQualifiers;
		}
		else
			if (FindSTRNIndex (NameHonorificsStrn, tokenCopy)) {
				if (numHonorifics && lastTokenSeperator)
					PCatC (honorifics, lastTokenSeperator);
				PCat (honorifics, token);
				++numHonorifics;
			}
			else {
				// The token is just plain ol' unsophisticated text, put it in the name
				if (*numPtr && lastTokenSeperator)
					PCatC (name, lastDelimiter = lastTokenSeperator);
				PCat (name, token);
				++(*numPtr);

				// If we just processed the first comma, switch to our "other" name
				if (*(spot - 1) == ',') {
					if (!numCommas) {
						name = (name == name1) ? name2 : name1;
						numPtr = (numPtr = &numName1) ? &numName2 : &numName1;
					}
					++numCommas;
				}
			}
		if (spot > realName + 1)
			lastTokenSeperator = *(spot - 1);
		// Skip white space before the next token
		while (spot <= &realName[realName[0]] && *spot == ' ' || *spot == tabChar)
				++spot;
	}
	
	// Okay, at this point we have stuff in 4 places:  name1, name2, qualifiers and honorifics
	// We can now go about building the actual first and last names based on whether or not
	// we've found a comma and whether or not we're working with eastern vs western style names.
	name = name1;
	numPtr = &numName1;
	
	// If we found no commas, or the _opposite_ name is empty (indicating that the entire name is
	// stuffed into the primary (first) name fiels, then we'll make the opposite (last) name the
	// final token appearing in the primary name field (whew!)
	if (!numCommas || !((name == name1) ? *name2 : *name1)) {
		if (*numPtr > 1) {
			short	name2len;
			
			spot = PRIndex (name, lastDelimiter);
			name2len = *name - (spot - name);
			BMD (spot, (name == name1) ? name2 : name1, name2len + 1);
			*name2 = name2len;
			*name -= (name2len + 1);
		}
		if ((GetPrefLong(PREF_NICK_GEN_OPTIONS)&kNickGenOptAsian) || IsAllUpper(name1) && !IsAllUpper(name2)) {
			PCopy (firstName, name2);
			PCopy (lastName, name1);
		}
		else {
			PCopy (firstName, name1);
			PCopy (lastName, name2);
		}
	}
	else
		if ((GetPrefLong(PREF_NICK_GEN_OPTIONS)&kNickGenOptAsian) || IsAllUpper(name2) && !IsAllUpper(name1)) {
			PCopy (firstName, name1);
			PCopy (lastName, name2);
		}
		else {
			PCopy (firstName, name2);
			PCopy (lastName, name1);
		}
	
	// Append qualifiers to the first name
	if (numQualifiers) {
		PCatC (firstName, ' ');
		PCat (firstName, qualifiers);
	}
	return (realName);
}

PStr JoinFirstLast (PStr fullName, PStr firstName, PStr lastName)

{
	Str255	first,
					last;

	*fullName = 0;
	*first		= 0;
	*last			= 0;

	// Strip Qualifiers and Honorifics from the first and last names
	StripQualifiersAndHonorifics (firstName, first);
	StripQualifiersAndHonorifics (lastName, last);

	// Join them based on our "international joining preferences"
	if ((GetPrefLong(PREF_NICK_GEN_OPTIONS)&kNickGenOptAsian) || IsAllUpper(last) && !IsAllUpper(first)) {
		if (*last) PCat (fullName, last);
		if (*first) {
			if (*last) PCatC (fullName, ' ');
			PCat (fullName, first);
		}
	}
	else {
		if (*first) PCat (fullName, first);
		if (*last) {
			if (*first) PCatC (fullName, ' ');
			PCat (fullName, last);
		}
	}
	return (fullName);
}


PStr StripQualifiersAndHonorifics (PStr name, PStr strippedName)

{
	Str255	token,
					tokenCopy;
	UPtr		spot;
	char		lastTokenSeperator;

	*strippedName			 = 0;
	lastTokenSeperator = 0;
	
	// Loop through all the tokens in the first name
	spot = name + 1;
	while (PToken (name, token, &spot, " ,")) {
		PCopy (tokenCopy, token);
		
		// Cleanup the token a bit before giving the qualifiers and honorifics a shot
		PStripChar (tokenCopy, '.');
		
		// If the token neither a name qualifier or honorific, keep what we found
		if (!FindSTRNIndex (NameQualifiersStrn, tokenCopy) && !FindSTRNIndex (NameHonorificsStrn, tokenCopy)) {
			if (lastTokenSeperator)
				PCatC (strippedName, lastTokenSeperator);
			PCat (strippedName, token);
		}
		if (spot > name + 1)
			lastTokenSeperator = *(spot - 1);
		// Skip white space before the next token
		while (spot <= &name[name[0]] && *spot == ' ' || *spot == tabChar)
				++spot;
	}

	return (strippedName);
}

//	Walk the text of the name backwards, inserting spaces as we find periods that
//  follow runs of two or more characters

PStr ScanNameForSpaces (PStr name)

{
	Ptr		spot;
	short	chars;
	
	// don't insert spaces if name contains an @, since it's probably an address
	if (PIndex(name,'@')) return name;
	
	chars = 0;
	for (spot = name + *name; spot > name; spot--)
		switch (*spot) {
			case '.':
				if (chars > 1) {
					PInsertC (name, sizeof (Str255), ' ', spot + 1);
				}
			case ' ':
				chars = 0;
				break;
			default:
				++chars;
		}
	return (name);
}


/************************************************************************
 * MakeUniqueNickname - Make a unique "untitled" nickname
 ************************************************************************/
void MakeUniqueNickname (short ab, Str31 nickname)

{
	NickStructHandle	aliases;
	Str31							s;
	long							hashName,
										suffix;
	Byte							saveLen;

	if (!*nickname)
		GetRString (nickname, UNTITLED_NICKNAME);
	if (aliases = (*Aliases)[ab].theData) {
		hashName = NickHash (nickname);
		suffix = 2;
		saveLen = *nickname;
		while (NickMatchFound (aliases, hashName, nickname, ab) >= 0) {
			*nickname = saveLen;
			s[*s = 1] = ' ';
			PLCat (s, suffix++);
			if (*nickname + *s + 1 < sizeof (Str31) - 1)
				PCat (nickname, s);
			else {
				BlockMoveData (&s[1], &nickname[*nickname - *s + 1], *s);
				*nickname = sizeof (Str31) - 1;
			}
			hashName = NickHash (nickname);
		}
	}
}

/************************************************************************
 * NickBackup - Make a backup of a nickname file
 ************************************************************************/
OSErr NickBackup(FSSpecPtr spec)
{
	OSErr err = noErr;
	FSSpec spoolSpec;
	DateTimeRec dtr;
	Str31 name;
	
	if ((err=SubFolderSpec(SPOOL_FOLDER,&spoolSpec))==noErr)
	{
		GetTime(&dtr);
		FSMakeFSSpec(spoolSpec.vRefNum,spoolSpec.parID,ComposeString(name,"\p%p.%d.%d.%d.%d",spec->name,dtr.day,dtr.hour,dtr.minute,dtr.second),&spoolSpec);
		err = FSpDupFile(&spoolSpec,spec,False,False);
	}
	
	return (err);
}

//
//	GetNicknameTagMap
//
//		Retrieve a Nickname Tag Map from a 'TGMP' resource, placing the contents
//		into a NicknameTagMapRec.  In searching for the proper resource we look
//		for a 'TGMP' with a resource name that matches:
//				1. The requested server
//				2. The requested service (if no server match was found)
//

OSErr GetNicknameTagMap (PStr service, PStr server, NicknameTagMapRecPtr tagMapPtr)

{
	Handle	resource;
	Ptr			from;
	Str255	scratch;
	OSErr		theError;
	short		i;
	
	// First, find a 'TGMP' resource
	resource = nil;
	if (*server)
		resource = GetNamedResource (TAG_MAP_TYPE, server);
	if (!resource && *service)
		resource = GetNamedResource (TAG_MAP_TYPE, service);
	if (!resource)
		return (ResError ());
	
	// Create handles for service and nickname tags
	tagMapPtr->serviceTags	= NuHandle (0);
	tagMapPtr->nicknameTags	= NuHandle (0);
	theError = MemError ();
	
	if (!theError) {
		// Read the information out of the 'TGMP' resource
		from = LDRef (resource);
	
		// Service
		from = CopyBytesAndMovePtr (from, &tagMapPtr->service, *from + 1);
		// Server
		from = CopyBytesAndMovePtr (from, &tagMapPtr->server, *from + 1);
		// Count
		from = CopyBytesAndMovePtr (from, &tagMapPtr->count, sizeof (tagMapPtr->count));
		// The tags
		for (i = 0; i < tagMapPtr->count && !theError; ++i) {
			from = CopyBytesAndMovePtr (from, scratch, *from + 1);
			theError = PtrPlusHand (scratch, tagMapPtr->serviceTags, *scratch + 1);
			if (!theError) {
				from = CopyBytesAndMovePtr (from, scratch, *from + 1);
				theError = PtrPlusHand (scratch, tagMapPtr->nicknameTags, *scratch + 1);
			}
		}
		
		UL (resource);
	}
	
	ReleaseResource (resource);

	// Get rid of any memory we've allocated if trouble is brewing
	if (theError)
		DisposeNicknameTagMap (tagMapPtr);

	return (theError);
}

void	DisposeNicknameTagMap (NicknameTagMapRecPtr tagMapPtr)

{
	if (tagMapPtr) {
		ZapHandle (tagMapPtr->serviceTags);
		ZapHandle (tagMapPtr->nicknameTags);
	}
}


//
//	NicknameTag2ServiceTag
//
//		Given a pointer to a Nickname Tag Map and a Eudora nickname tag, return the corresponding
//		tag for a given service.
//

PStr NicknameTag2ServiceTag (NicknameTagMapRecPtr tagMapPtr, PStr nicknameTag, PStr serviceTag)

{
	Ptr		serviceTagsPtr,
				nicknameTagsPtr;
	short	i;
	
	*serviceTag = 0;
	nicknameTagsPtr	= LDRef (tagMapPtr->nicknameTags);
	serviceTagsPtr = LDRef (tagMapPtr->serviceTags);
	for (i = 0; i < tagMapPtr->count && !*serviceTag; ++i)
		if (StringSame (nicknameTagsPtr, nicknameTag))
			PCopy (serviceTag, serviceTagsPtr);
		else {
			nicknameTagsPtr += (*nicknameTagsPtr + 1);
			serviceTagsPtr += (*serviceTagsPtr + 1);
		}
	UL (tagMapPtr->serviceTags);
	UL (tagMapPtr->nicknameTags);
	return (serviceTag);
}


//
//	ServiceTag2NicknameTag
//
//		Given a pointer to e Nickname Tag Map and a service tag, return the corresponding
//		tag for a Eudora nickname
//

PStr ServiceTag2NicknameTag (NicknameTagMapRecPtr tagMapPtr, PStr serviceTag, PStr nicknameTag)

{
	Ptr		serviceTagsPtr,
				nicknameTagsPtr;
	short	i;
	
	*nicknameTag = 0;
	serviceTagsPtr	= LDRef (tagMapPtr->serviceTags);
	nicknameTagsPtr = LDRef (tagMapPtr->nicknameTags);
	for (i = 0; i < tagMapPtr->count && !*nicknameTag; ++i)
		if (StringSame (serviceTagsPtr, serviceTag))
			PCopy (nicknameTag, nicknameTagsPtr);
		else {
			serviceTagsPtr += (*serviceTagsPtr + 1);
			nicknameTagsPtr += (*nicknameTagsPtr + 1);
		}
	UL (tagMapPtr->nicknameTags);
	UL (tagMapPtr->serviceTags);
	return (nicknameTag);
}


PStr GetIndNicknameTag (NicknameTagMapRecPtr tagMapPtr, short index, PStr nicknameTag)

{
	Ptr		nicknameTagsPtr;
	short	i;
	
	*nicknameTag = 0;
	nicknameTagsPtr = *tagMapPtr->nicknameTags;
	if (index < tagMapPtr->count) {
		for (i = 0; i < index; ++i)
			nicknameTagsPtr += (*nicknameTagsPtr + 1);
		PCopy (nicknameTag, nicknameTagsPtr);
	}
	return (nicknameTag);
}

short FindServiceTagIndex (NicknameTagMapRecPtr tagMapPtr, PStr serviceTag)

{
	Ptr			serviceTagsPtr;
	short		index,
					i;
	
	serviceTagsPtr = LDRef (tagMapPtr->serviceTags);
	index = 0;
	for (i = 1; !index && i <= tagMapPtr->count; ++i)
		if (StringSame (serviceTagsPtr, serviceTag))
			index = i;
		else
			serviceTagsPtr += (*serviceTagsPtr + 1);
	UL (tagMapPtr->serviceTags);
	return (index);
}


PrimaryLocationType GetPrimaryLocation (Handle notes)

{
	PrimaryLocationType	location;
	Str255							tag,
											value;
	
	location = noPrimary;
	GetTaggedFieldValueStrInNotes (notes, GetRString (tag, ABReservedTagsStrn + abTagPrimary), value);
	if (StringSame (value, GetRString (tag, VCardKeywordStrn + vcHome)))
		location = homePrimary;
	else if (StringSame (value, GetRString (tag, VCardKeywordStrn + vcWork)))
		location = workPrimary;
	return (location);
}


short FindAddressBookType (AddressBookType type)

{
	short	addressBooks,
				ab;
	
	addressBooks = NAliases;
	for (ab = 0; ab < addressBooks; ++ab)
		if ((*Aliases)[ab].type == type)
			return (ab);
	return (-1);
}

#ifdef VCARD
Boolean AnyPersonalNicknames (void)

{
	short		ab,
					nick;
	Boolean	anyNicks;
	
	anyNicks = false;
	ab = FindAddressBookType (personalAddressBook);
	if (ValidAddressBook (ab))
		if ((*Aliases)[ab].theData) {
			nick = GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct);
			while (!anyNicks && nick--)
				if (!(*((*Aliases)[ab].theData))[nick].deleted)
					anyNicks = true;
		}
	return (anyNicks);
}

/************************************************************************
 * WhiteListTS - add a message's sender to the whitelist
 ************************************************************************/
OSErr WhiteListTS(TOCHandle tocH,short sumNum)
{
	Str255 scratch;
	HeadSpec hs;
	TextAddrHandle addr=nil;
	Accumulator a;
	uLong hash;
		
	if (sumNum<0)
	{
		Zero(a);
		
		// Examine all the selected messages
		for (sumNum=(*tocH)->count-1;sumNum>=0;sumNum--)
		{
			if ((*tocH)->sums[sumNum].selected)
			{
				WhiteListTS(tocH,sumNum);
				EnsureFromHash(tocH,sumNum);
				hash = (*tocH)->sums[sumNum].fromHash;
				if (ValidHash(hash) && 0>AccuFindPtr(&a,&hash,sizeof(hash)))
					AccuAddPtr(&a,&hash,sizeof(hash));
			}
		}

		// select everything with the same hash
		for (sumNum=(*tocH)->count;sumNum--;)
			if (!(*tocH)->sums[sumNum].selected)
			{
				EnsureFromHash(tocH,sumNum);
				hash = (*tocH)->sums[sumNum].fromHash;
				if (ValidHash(hash) && 0<=AccuFindPtr(&a,&hash,sizeof(hash)))
					BoxSetSummarySelected(tocH,sumNum,true);
			}
		
		AccuZap(a);
	}
	else
	{
		if (!CacheMessage(tocH,sumNum))
		{
			HNoPurge((*tocH)->sums[sumNum].cache);
			
			HeaderName(FROM_HEAD); // weird--goes into scratch
			TrimWhite(scratch);
			if (HandleHeadFindStr((*tocH)->sums[sumNum].cache,scratch,&hs))
			{
				HandleHeadGetText((*tocH)->sums[sumNum].cache,&hs,&addr);
				if (addr) WhiteListAddr(addr);
				ZapHandle(addr);
			}
			
			HPurge((*tocH)->sums[sumNum].cache);
			
			// Now that we've whitelisted the message, bop its junk score
			JunkSetScore(tocH,sumNum,JUNK_BECAUSE_WHITE,0);
		}
	}
		
	return noErr; // la la la la la la la la la la la la la la
}

/************************************************************************
 * WhiteListAddr - add a an address to the whitelist
 ************************************************************************/
OSErr WhiteListAddr(TextAddrHandle addr)
{
	Str255 scratch;
	Str31 abName;
	Str63 nick;
	Str255 first, last;
	short which=0;
	BinAddrHandle binAddr, justAddr;
	OSErr err = fnfErr;
	
	// is it there already?
	MakePStr(scratch,*addr,GetHandleSize(addr));
	if (AppearsInAliasFile(scratch,nil)) return dupFNErr;	// already there

	// Nope.  Add it.
	
	// find which book
	if (*GetRString(scratch,WHITELIST_ADDRBOOK))
	{
		for (which = NAliases-1; which>0; which--)
		{
			PCopy(abName,(*Aliases)[which].spec.name);
			if (StringSame(abName,scratch)) break;
		}
	}
	else which = FindAddressBookType(eudoraAddressBook);
	
	// reformat the address
	if (!SuckAddresses(&binAddr,addr,true,false,false,nil))
	{
		// get a nickname suggestion
		NickSuggest(binAddr,nick,scratch,true,JUNK_NICK_FMT);
		if (*nick)
		if (!SuckAddresses(&justAddr,addr,false,false,false,nil))
		{
			// fancy names are good
			ParseFirstLast(scratch,first,last);
			
			// finally add the darn thing
			if (0<=NewNickLow(justAddr, CreateSimpleNotes(scratch,first,last), which, nick, false, nrNone, !NickWinIsOpen()))
				ABTickleHardEnoughToMakeYouPuke();
			
			// the rest is gravy...
			ZapHandle(justAddr);
		}
		ZapHandle(binAddr);
	}
	
	return err;
}

#endif

/************************************************************************
 * UniqBinAddr - make a BinAddrHandle contain only one of each address
 ************************************************************************/
BinAddrHandle UniqBinAddr(BinAddrHandle addresses)
{
	Accumulator hashAcc, addrAcc;
	Str255 addr;
	long offset = 0;
	long len = GetHandleSize(addresses);
	
	Zero(hashAcc);
	Zero(addrAcc);
	
	// loop through all the addresses in the bin addr list
	for (offset = 0; offset<len-1; offset += (*addresses)[offset]+2)
	{
		// copy to a string
		PCopy(addr,*addresses + offset);
		
		ASSERT(*addr);	// shouldn't happen...
		
		// add hash to list of hashes if not there already
		if (AddAddressHashUniq(addr,&hashAcc))
		{
			// wasn't there already, add to output list
			PTerminate(addr);
			AccuAddPtr(&addrAcc,addr,*addr+2);
		}
	}
	
	// terminating BinAddrHandle nil
	AccuAddChar(&addrAcc,0);
	
	// copy into original handle
	SetHandleBig(addresses,addrAcc.offset);
	if (!MemError())
		BMD(*addrAcc.data,*addresses,addrAcc.offset);
	
	// cleanup
	AccuZap(addrAcc);
	AccuZap(hashAcc);
	
	return addresses;
}

/************************************************************************
 * SortBinAddr - make a BinAddrHandle be in order
 ************************************************************************/
BinAddrHandle SortBinAddr(BinAddrHandle addresses)
{
	short count = CountAddresses(addresses,0);
	short i;
	Handle addressVector = NuHandle(count*sizeof(UPtr));
	BinAddrHandle newAddresses = DupHandle(addresses);
	UPtr spot;
	UPtr *vSpot;
	
	// fill in a vector of pointers to each address
	if (addressVector && newAddresses)
	{
		vSpot = (UPtr*)*addressVector;
		
		for (spot = LDRef(addresses); *spot; spot += *spot+2)
			*vSpot++ = spot;
		
		ASSERT(((long*)*addressVector)[count-1]);
	
		// now sort the vector
		QuickSort(LDRef(addressVector),sizeof(UPtr),0,count-1,(void*)SortAddrNameCompare,(void*)PtrSwap);
		
		// copy the list...
		spot = *newAddresses;
		vSpot = (UPtr*)*addressVector;
		for (i=0;i<count;i++)
		{
			BMD(*vSpot,spot,**vSpot+2);
			spot += **vSpot+2;
			vSpot++;
		}
		
		// Shoot it home
		BMD(*newAddresses,*addresses,GetHandleSize(addresses));
	}

	// cleanup
	ZapHandle(newAddresses);
	ZapHandle(addressVector);
	
	return addresses;
}

/************************************************************************
 * SortAddrNameCompare - compare two addresses, going by last name then first name.
 *  This is an insanely expensive comparison, but we don't usually sort huge
 *  lists of addresses and cpu is cheap
 ************************************************************************/
int SortAddrNameCompare(UPtr *s1, UPtr *s2)
{
	Str127 first, last, n1, n2;
	
	PSCopy(n1,*s1);
	PSCopy(n2,*s2);
	BeautifyFrom(n1);
	BeautifyFrom(n2);
	ParseFirstLast(n1,first,last);
	PCopy(n1,last);
	PCat(n1,first);
	ParseFirstLast(n2,first,last);
	PCopy(n2,last);
	PCat(n2,first);
	return StringComp(n1,n2);
}
