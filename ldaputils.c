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

#include <conf.h>
#include <mydefs.h>

#include "ldap.h"
#include "lber.h"
#include "dflutils.h"
#include "dflsuppl.h"
#include "ldaplibglue.h"
#include "ldaputils.h"

#define FILE_NUM 107
#pragma segment LDAPUtils


/* MJN *//* new file */



#pragma mark defines

#define ASYNC_LDAP_CALLS 1

#define LDAP_TEXT_ACCUM_CHUNK_SIZE 1024
#define LDAP_ATTR_LIST_GROW_SIZE 10

#define LDAP_ATTR_TRANSLATION_TABLE_ID 32767
#define LDAP_SERVER_LIST_ID 32767


#pragma mark enum constants

enum {
	kLDAPOpenPreflightMemSize = 65536, /* 64K */
	kLDAPSearchPreflightMemSize = 32768 /* 32K */
};
enum {
	kLDAPErrTableRsrcType = 'LECT',
	kLDAPErrTableRsrcID = 128
};
enum {
	kMonitorLDAPCodeGrowMinSize = 131072 /* 128K */
};
enum {
	kLDAPUserCancelCheckDelay = 15 /* in ticks */
};


#pragma mark typedefs

/* WARNING *//* the numbering of these items must match the ordering of items in the LDAP Atrribute lookup table resource */
typedef enum {
	invalidAttr = 0,
	commonNameAttr,
	commonNameAttr2,
	cityAttr,
	stateAttr,
	organizationAttr,
	organizationalUnitAttr,
	countryAttr,
	streetAttr,
	emailAttr,
	emailAlt1Attr,
	emailAlt2Attr,
	lastNameAttr,
	givenNameAttr,
	webLookupURLAttr,
	telephoneAttr
} LDAPAttrType;

typedef struct {
	Handle	dataHdl;
	long		physicalSize;
	long		logicalSize;
} LDAPTextAccumRec, *LDAPTextAccumPtr, **LDAPTextAccumHdl;

/* defined as struct timeval in ":ldap-3.3:libraries:macintosh:tcp:tcp.h";
		the definition in "utime.mac.h" in the Metrowerks Standard Library headers is incompatible */
typedef struct LDAPTimeValRec {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
} LDAPTimeValRec, *LDAPTimeValPtr;


#pragma mark globals

static Boolean LDAPCodeLoaded = false;
static Boolean LDAPCodePurgeable = false;
static Boolean LDAPCanUseDFL = false;
static OSErr LDAPCanUseDFLErr = noErr;
static OSErr LDAPLibLoadErr = noErr;

static Str255 LDAPResultCountStr;
static Str255 LDAPResultErrStr;
static Str255 LDAPUserCanceledMsgStr;
Str255 LDAPDividerStr;
static long MinLDAPLabelLen;


/* FINISH *//* does this need to be here? */
#if GENERATING68KSEGLOAD
#pragma mpwc on
#endif

#ifdef LDAP_ENABLED
static void AppendPStr(Str255 targetStr, Str255 appendStr)

{
	long		len;


	len = appendStr[0];
	if (targetStr[0] + len > 255)
		len = 255 - targetStr[0];
	BlockMoveData(appendStr + 1, targetStr + targetStr[0] + 1, len);
	targetStr[0] += len;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static void AppendCStr(Str255 targetStr, char* appendStr)

{
	long		len;
	char*		c;


	len = 0;
	c = appendStr;
	while (*c++)
		len++;
	if (len > 255)
		len = 255;
	if (targetStr[0] + len > 255)
		len = 255 - targetStr[0];
	BlockMoveData(appendStr, targetStr + targetStr[0] + 1, len);
	targetStr[0] += len;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr NewLDAPTextAccum(LDAPTextAccumHdl *textAccum)

{
	LDAPTextAccumHdl	accumHdl;
	LDAPTextAccumPtr	accumPtr;
	Handle						h;


	*textAccum = nil;

	accumHdl = (LDAPTextAccumHdl)NewHandle(sizeof(LDAPTextAccumRec));
	if (!accumHdl)
		return MemError();
	h = NewHandle(LDAP_TEXT_ACCUM_CHUNK_SIZE);
	if (!h)
	{
		DisposeHandle((Handle)accumHdl);
		return MemError();
	}

	accumPtr = *accumHdl;
	accumPtr->dataHdl = h;
	accumPtr->physicalSize = LDAP_TEXT_ACCUM_CHUNK_SIZE;
	accumPtr->logicalSize = 0;

	*textAccum = accumHdl;
	return noErr;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static void DisposeLDAPTextAccum(LDAPTextAccumHdl textAccum)

{
	if (textAccum)
	{
		if ((**textAccum).dataHdl) DisposeHandle((**textAccum).dataHdl);
		DisposeHandle((Handle)textAccum);
	}
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr CompactLDAPTextAccum(LDAPTextAccumHdl textAccum)

{
	LDAPTextAccumPtr	accumPtr;


	accumPtr = *textAccum;
	SetHandleSize(accumPtr->dataHdl, accumPtr->logicalSize);
	return MemError(); /* no error expected */
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendLDAPTextAccum(LDAPTextAccumHdl textAccum, unsigned char* dataPtr, long length)

{
	OSErr							err;
	LDAPTextAccumPtr	accumPtr;
	long							neededSize, newPhysicalSize;
	Handle						dataHdl;


	accumPtr = *textAccum;
	neededSize = accumPtr->logicalSize + length;
	if (neededSize > accumPtr->physicalSize)
	{
		newPhysicalSize = accumPtr->physicalSize;
		while (newPhysicalSize < neededSize)
			newPhysicalSize += LDAP_TEXT_ACCUM_CHUNK_SIZE;
		SetHandleSize(accumPtr->dataHdl, newPhysicalSize);
		err = MemError();
		if (err)
			return err;
		(**textAccum).physicalSize = newPhysicalSize;
	}
	accumPtr = *textAccum;
	dataHdl = accumPtr->dataHdl;
	BlockMoveData(dataPtr, *dataHdl + accumPtr->logicalSize, length);
	accumPtr->logicalSize = neededSize;

	return noErr;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendPStrLDAPTextAccum(LDAPTextAccumHdl textAccum, Str255 theString)

{
	return AppendLDAPTextAccum(textAccum, theString + 1, theString[0]);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendCStrLDAPTextAccum(LDAPTextAccumHdl textAccum, char* theString)

{
	long		len;
	char*		c;


	len = 0;
	c = theString;
	while (*c++)
		len++;
	return AppendLDAPTextAccum(textAccum, (unsigned char*)theString, len);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendCRLDAPTextAccum(LDAPTextAccumHdl textAccum)

{
	unsigned char		c;


	c = 13;
	return AppendLDAPTextAccum(textAccum, &c, 1);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendDividerLDAPTextAccum(LDAPTextAccumHdl textAccum)

{
	OSErr						err;


	err = AppendPStrLDAPTextAccum(textAccum, LDAPDividerStr);
	if (!err)
		err = AppendCRLDAPTextAccum(textAccum);
	return err;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr AppendNumLDAPTextAccum(LDAPTextAccumHdl textAccum, long theNum)

{
	Str255	s;


	NumToString(theNum, s);
	return AppendPStrLDAPTextAccum(textAccum, s);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr LDAPTextAccumToHandle(LDAPTextAccumHdl textAccum, Handle *dataHdl)

{
	OSErr		err;


	*dataHdl = nil;

	err = CompactLDAPTextAccum(textAccum); /* no error expected */
	if (!err)
	{
		*dataHdl = (**textAccum).dataHdl;
		(**textAccum).dataHdl = nil;
	}
	DisposeLDAPTextAccum(textAccum);
	return err;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
RawLDAPAttrList DerefLDAPAttrList(LDAPAttrListHdl attributesList)

{
	if (!attributesList)
		return nil;
	HLock((**attributesList).attrList);
	return (RawLDAPAttrList)(*(**attributesList).attrList);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
void UnderefLDAPAttrList(LDAPAttrListHdl attributesList)

{
	if (!attributesList)
		return;
	HUnlock((**attributesList).attrList);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
OSErr NewLDAPAttrList(LDAPAttrListHdl *attributesList)

{
	OSErr							err;
	LDAPAttrListHdl		attrs;
	Handle						attrData;


	*attributesList = nil;

	attrs = (LDAPAttrListHdl)NewHandle(sizeof(LDAPAttrListRec));
	if (!attrs)
		return MemError();
	attrData = NewHandleClear(sizeof(char*));
	if (!attrData)
	{
		err = MemError();
		DisposeHandle((Handle)attrs);
		return err;
	}

	(**attrs).numLogicalAttrs = 0;
	(**attrs).numPhysicalAttrs = 0;
	(**attrs).attrList = attrData;

	*attributesList = attrs;
	return noErr;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
void DisposeLDAPAttrList(LDAPAttrListHdl attributesList)

{
	RawLDAPAttrList	rawList;
	long						numAttr;
	long						i;


	if (!attributesList)
		return;

	rawList = DerefLDAPAttrList(attributesList);
	numAttr = (**attributesList).numLogicalAttrs;
	for (i = 0; i < numAttr; i++)
	{
		DisposePtr(rawList[i]);
		rawList[i] = nil;
	}
	UnderefLDAPAttrList(attributesList);
	DisposeHandle((**attributesList).attrList);
	DisposeHandle((Handle)attributesList);
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static OSErr GrowLDAPAttrList(LDAPAttrListHdl attributesList, long growSize)

{
	OSErr	err;
	long	newCount;
	long	curSize, newSize;


	if (!attributesList)
		return paramErr;

	if (!growSize)
		return noErr;

	curSize = GetHandleSize((**attributesList).attrList);
	newCount = (**attributesList).numPhysicalAttrs + growSize;
	newSize = sizeof(char*) * (newCount + 1);
	SetHandleSize((**attributesList).attrList, newSize);
	err = MemError();
	if (!err)
		(**attributesList).numPhysicalAttrs = newCount;
	return err;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
OSErr AppendLDAPAttrList(LDAPAttrListHdl attributesList, Str255 attrName)

{
	OSErr						err;
	long						newLogCount;
	RawLDAPAttrList	rawList;
	Ptr							p;


	if (!attributesList)
		return paramErr;

	newLogCount = (**attributesList).numLogicalAttrs + 1;
	if (newLogCount > (**attributesList).numPhysicalAttrs)
	{
		err = GrowLDAPAttrList(attributesList, LDAP_ATTR_LIST_GROW_SIZE);
		if (err)
			return err;
	}
	(**attributesList).numLogicalAttrs = newLogCount;

	p = NewPtr(attrName[0] + 1);
	if (!p)
		return MemError();
	BlockMoveData(attrName + 1, p, attrName[0]);
	p[attrName[0]] = 0;
	rawList = DerefLDAPAttrList(attributesList);
	rawList[newLogCount - 1] = p;
	rawList[newLogCount] = nil;
	UnderefLDAPAttrList(attributesList);

	return noErr;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
OSErr CompactLDAPAttrList(LDAPAttrListHdl attributesList)

{
	OSErr	err;
	long	curSize, newSize;


	if (!attributesList)
		return paramErr;

	curSize = GetHandleSize((**attributesList).attrList);
	newSize = sizeof(char*) * ((**attributesList).numLogicalAttrs + 1);
	if (newSize == curSize)
		return noErr;
	if (newSize > curSize)
		return paramErr;
	SetHandleSize((**attributesList).attrList, newSize);
	err = MemError();
	if (!err)
		(**attributesList).numPhysicalAttrs = (**attributesList).numLogicalAttrs;
	return err;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static LDAPAttrType LDAPAttrNameToIndex(char* attrName)

{
	long			len;
	char*			c;
	Str255		attrNamePStr;
	Handle		attrTableHdl;
	long			numAttrNames;
	long			i;
	Boolean		found;
	Str255		attrCode;


	len = 0;
	c = attrName;
	while (*c++)
		len++;
	if (len > 255)
		len = 255;
	BlockMoveData(attrName, attrNamePStr + 1, len);
	attrNamePStr[0] = len;

	attrTableHdl = GetResource('STR#', LDAP_ATTR_TRANSLATION_TABLE_ID);
	if (!attrTableHdl || (GetHandleSize(attrTableHdl) < 2))
		return invalidAttr;

	numAttrNames = (**(short**)attrTableHdl);
	i = 0;
	while (i <= numAttrNames)
	{
		GetIndString(attrCode, LDAP_ATTR_TRANSLATION_TABLE_ID, i + 1);
		found = EqualString(attrNamePStr, attrCode, false, false);
		if (found)
			return (i / 2) + 1;
		i += 2;
	}

	return invalidAttr;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static Boolean LDAPAttrIsEmailAddr(char* attrName)

{
	LDAPAttrType	attrType;


	attrType = LDAPAttrNameToIndex(attrName);
	switch (attrType)
	{
		case emailAttr:
		case emailAlt1Attr:
		case emailAlt2Attr:
			return true;
			break;
	}
	return false;
}
#endif  //ifdef LDAP_ENABLED


#ifdef LDAP_ENABLED
static void LDAPAttrNameToLabel(char* attrName, Str255 label)

{
	long			len;
	char*			c;
	Handle		attrTableHdl;
	long			numAttrNames;
	long			i;
	Boolean		found;
	Str255		attrCode;
	Str255		attrLabel;
	long			adjustSize;


	len = 0;
	c = attrName;
	while (*c++)
		len++;
	if (len > 255)
		len = 255;
	BlockMoveData(attrName, label + 1, len);
	label[0] = len;

	attrTableHdl = GetResource('STR#', LDAP_ATTR_TRANSLATION_TABLE_ID);
	if (!attrTableHdl || (GetHandleSize(attrTableHdl) < 2))
		goto Align;
	numAttrNames = (**(short**)attrTableHdl);
	i = 0;
	while (i <= numAttrNames)
	{
		GetIndString(attrCode, LDAP_ATTR_TRANSLATION_TABLE_ID, i + 1);
		found = EqualString(label, attrCode, false, true);
		if (found)
		{
			GetIndString(attrLabel, LDAP_ATTR_TRANSLATION_TABLE_ID, i + 2);
			BlockMoveData(attrLabel, label, attrLabel[0] + 1);
			goto Align;
		}
		i += 2;
	}


Align:
	if (!label[0] || (label[0] >= MinLDAPLabelLen))
		return;
	adjustSize = MinLDAPLabelLen - label[0];
	BlockMoveData(label + 1, label + 1 + adjustSize, label[0]);
	for (i = 0, c = (char*)label + 1; i < adjustSize; i++, c++)
		*c = ' ';
	label[0] = MinLDAPLabelLen;
}
#endif  //ifdef LDAP_ENABLED


OSErr LDAPResultsToText(LDAPSessionHdl ldapSession, Handle *resultText, LDAPAttrListHdl attrList, Boolean translateLabels, LDAPResultsEntryFilterProcPtr entryFilter)

{
#ifdef LDAP_ENABLED
	OSErr 		err, scratchErr;
	Str255		errMsgStr;
	LDAP*			ldapRef;
	LDAPMessage*			searchResults;
	LDAPTextAccumHdl	textAccum;
	LDAPMessage*			curMsg;
/* FINISH *//* REMOVE */
#if 0
	char*			entryName;
#endif
	char*			attr;
	void*			dataPtr;
	char*			*vals;
	long			i;
	long			entryCount;
	Str255		attrLabel;
	long			numEntries;
	char*			tempCharPtr = nil; /* FINISH *//* REMOVE */
	Str255		userNameStr;
	Boolean		needUserNameStr;
	Boolean		foundUserNameStr;
	Str255		emailAddressStr;
	Boolean		needEmailAddressStr;
	Boolean		foundEmailAddressStr;
	long			startOffset;
	long			endOffset;
	short			a;


	*resultText = nil;

	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;

	ldapRef = (**ldapSession).ldapRef;
	searchResults = (**ldapSession).searchResults;

	err = NewLDAPTextAccum(&textAccum);
	if (err)
		return err;

	if ((**ldapSession).searchErr == LDAP_USER_CANCELLED)
	{
		err = AppendPStrLDAPTextAccum(textAccum, LDAPUserCanceledMsgStr);
		if (!err)
			err = AppendCRLDAPTextAccum(textAccum);

		if (err)
			goto Error;
		err = LDAPTextAccumToHandle(textAccum, resultText);
		return err;
	}
	
#ifdef DEBUG
	if (RunType!=Production)
	{
		AppendPStrLDAPTextAccum(textAccum,PCopy(userNameStr,(**ldapSession).searchFilter));
		AppendPStrLDAPTextAccum(textAccum,Cr);
	}
#endif

	if (!(**ldapSession).searchResultsValid || (**ldapSession).searchErr)
	{
		err = AppendPStrLDAPTextAccum(textAccum, LDAPResultErrStr);
		if (!err)
		{
			scratchErr = LDAPErrCodeToMsgStr((**ldapSession).searchErr, errMsgStr, true);
			if (!scratchErr)
				err = AppendPStrLDAPTextAccum(textAccum, errMsgStr);
			else
				err = AppendNumLDAPTextAccum(textAccum, (**ldapSession).searchErr);
		}
		if (!err)
			err = AppendCRLDAPTextAccum(textAccum);

		/* FINISH *//* use ldapRef->ld_error */
		if (tempCharPtr /* ldapRef->ld_error */)
		{
			if (!err)
				err = AppendCStrLDAPTextAccum(textAccum, tempCharPtr /* ldapRef->ld_error */);
			if (!err)
				err = AppendCRLDAPTextAccum(textAccum);
		}

		if (err)
			goto Error;
		err = LDAPTextAccumToHandle(textAccum, resultText);
		return err;
	}

	numEntries = ldap_count_entries(ldapRef, searchResults);
	err = AppendPStrLDAPTextAccum(textAccum, LDAPResultCountStr);
	if (!err)
		err = AppendNumLDAPTextAccum(textAccum, numEntries);
	if (!err)
		err = AppendCRLDAPTextAccum(textAccum);

	if (!numEntries)
	{
		err = LDAPTextAccumToHandle(textAccum, resultText);
		return err;
	}
	else PhRememberServer();

	for (curMsg = ldap_first_entry(ldapRef, searchResults), entryCount = 1; curMsg != nil; curMsg = ldap_next_entry(ldapRef, curMsg), entryCount++)
	{
		CycleBalls();
		if (CommandPeriod) break;
		err = AppendDividerLDAPTextAccum(textAccum);
		if (err)
			goto Error;

		startOffset = (**textAccum).logicalSize;
		if (entryFilter)
		{
			userNameStr[0] = 0;
			needUserNameStr = true;
			foundUserNameStr = false;
			emailAddressStr[0] = 0;
			needEmailAddressStr = true;
			foundEmailAddressStr = false;
		}

/* FINISH *//* REMOVE */
#if 0
		entryName = ldap_get_dn(ldapRef, curMsg);
		err = AppendPStrLDAPTextAccum(textAccum, "\pname = ");
		if (!err)
			err = AppendCStrLDAPTextAccum(textAccum, entryName);
		if (!err)
			err = AppendCRLDAPTextAccum(textAccum);
		if (err)
			goto Error;
		// free(entryName);
#endif

		if (attrList)
			for (a=0;a<(*attrList)->numLogicalAttrs;a++)
			{
				attr = ((char**)(*(*attrList)->attrList))[a];
				vals = ldap_get_values(ldapRef, curMsg, attr);
				if (vals && vals[0])
				{
					
					if (needUserNameStr)
					{
						int attrIndex = LDAPAttrNameToIndex(attr);
						foundUserNameStr =  attrIndex == commonNameAttr || attrIndex == commonNameAttr2;
					}
					if (needEmailAddressStr)
						foundEmailAddressStr = LDAPAttrIsEmailAddr(attr);

					if (translateLabels)
					{
						LDAPAttrNameToLabel(attr, attrLabel);
						err = AppendPStrLDAPTextAccum(textAccum, attrLabel);
					}
					else
						err = AppendCStrLDAPTextAccum(textAccum, attr);
					if (!err)
						err = AppendPStrLDAPTextAccum(textAccum, "\p: ");
					if (err)
						goto Error;

					err = noErr;
					for (i = 0; !err && (vals[i] != nil); i++)
					{
						if (needUserNameStr && foundUserNameStr)
						{
							needUserNameStr = false;
							AppendCStr(userNameStr, vals[i]);
						}
						if (needEmailAddressStr && foundEmailAddressStr)
						{
							needEmailAddressStr = false;
							AppendCStr(emailAddressStr, vals[i]);
						}

						/* FINISH *//* instead, just put each value on its own line, and repeat the label */
						if (i > 0)
							err = AppendPStrLDAPTextAccum(textAccum, "\p | ");
						if (!err)
							err = AppendCStrLDAPTextAccum(textAccum, vals[i]);
					}
					ldap_value_free(vals);
					err = AppendCRLDAPTextAccum(textAccum);
					if (err)
						goto Error;
				}
			}
		else
			for (attr = ldap_first_attribute(ldapRef, curMsg, &dataPtr); attr != nil; attr = ldap_next_attribute(ldapRef, curMsg, dataPtr))
			{
				if (needUserNameStr)
				{
					int attrIndex = LDAPAttrNameToIndex(attr);
					foundUserNameStr =  attrIndex == commonNameAttr || attrIndex == commonNameAttr2;
				}
				if (needEmailAddressStr)
					foundEmailAddressStr = LDAPAttrIsEmailAddr(attr);

				if (translateLabels)
				{
					LDAPAttrNameToLabel(attr, attrLabel);
					err = AppendPStrLDAPTextAccum(textAccum, attrLabel);
				}
				else
					err = AppendCStrLDAPTextAccum(textAccum, attr);
				if (!err)
					err = AppendPStrLDAPTextAccum(textAccum, "\p: ");
				if (err)
					goto Error;

				vals = ldap_get_values(ldapRef, curMsg, attr);
				err = noErr;
				for (i = 0; !err && (vals[i] != nil); i++)
				{
					if (needUserNameStr && foundUserNameStr)
					{
						needUserNameStr = false;
						AppendCStr(userNameStr, vals[i]);
					}
					if (needEmailAddressStr && foundEmailAddressStr)
					{
						needEmailAddressStr = false;
						AppendCStr(emailAddressStr, vals[i]);
					}

					/* FINISH *//* instead, just put each value on its own line, and repeat the label */
					if (i > 0)
						err = AppendPStrLDAPTextAccum(textAccum, "\p | ");
					if (!err)
						err = AppendCStrLDAPTextAccum(textAccum, vals[i]);
				}
				ldap_value_free(vals);
				err = AppendCRLDAPTextAccum(textAccum);
				if (err)
					goto Error;
			}


		endOffset = (**textAccum).logicalSize;

		if (entryFilter)
			(*entryFilter)(ldapSession, userNameStr, emailAddressStr, startOffset, endOffset);
	}
	err = AppendDividerLDAPTextAccum(textAccum);
	if (err)
		goto Error;

	err = LDAPTextAccumToHandle(textAccum, resultText);
	return err;


Error:
	DisposeLDAPTextAccum(textAccum);
	return err;

#else  //ifdef LDAP_ENABLED
	*resultText = nil;
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


OSErr ClearLDAPSearchResults(LDAPSessionHdl ldapSession)

{
	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;
#ifdef LDAP_ENABLED
	ldap_msgfree((**ldapSession).searchResults);
#endif  //ifdef LDAP_ENABLED
	(**ldapSession).searchErr = noErr;
	(**ldapSession).searchResults = nil;
	(**ldapSession).searchResultsValid = false;
	return noErr;
}

#if ASYNC_LDAP_CALLS
static int LDAPWaitForCompletion(LDAPSessionHdl ldapSession, LDAPMessage* *resultData)

{
	int				result;
	LDAP*			ldapRef;
	int				opMsgID;
	LDAPMessage*			localResultData;
	LDAPTimeValRec		timeoutVal;
	long			lastCancelCheckTicks;
	long			curTicks;
	Boolean		done;


	if (resultData)
		*resultData = nil;
	localResultData = nil;

	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;

	if (!(**ldapSession).opActive)
		return LDAP_SUCCESS;

	ldapRef = (**ldapSession).ldapRef;
	opMsgID = (**ldapSession).opMsgID;

	if (!ldapRef)
		return LDAP_PARAM_ERROR;
	if (opMsgID == -1)
		return LDAP_PARAM_ERROR;

	timeoutVal.tv_sec = 0;
	timeoutVal.tv_usec = 0;
	lastCancelCheckTicks = 0;
	done = false;
	while (!done)
	{
		curTicks = TickCount();
		if ((curTicks - lastCancelCheckTicks) > kLDAPUserCancelCheckDelay)
		{
			lastCancelCheckTicks = curTicks;
			if (MiniEvents())
			{
				result = ldap_abandon(ldapRef, opMsgID);
				if (!result)
				{
					done = true;
					(**ldapSession).opActive = false;
					if (localResultData)
						result = ldap_msgfree(localResultData);
					return LDAP_USER_CANCELLED;
				}
			}
		}

		result = ldap_result(ldapRef, opMsgID, 1, (struct timeval*)&timeoutVal, &localResultData);
		if (result)
		{
			done = true;
			break;
		}
	}

	(**ldapSession).opActive = false;

	if (!result) /* should never occur */
	{
		if (localResultData)
			result = ldap_msgfree(localResultData);
		return LDAP_TIMEOUT;
	}

	if (result > 0)
	{
		if (resultData)
		{
			*resultData = localResultData;
			return ldap_result2error(ldapRef, localResultData, 0);
		}
		else
			return ldap_result2error(ldapRef, localResultData, 1);
	}

	return ldap_result2error(ldapRef, localResultData, 1);
}
#endif  // ASYNC_LDAP_CALLS

OSErr BuildLDAPSearchFilter(PStr searchStr,PStr searchFilter,PStr forHost);
OSErr AddToLDAPSearchFilter(PStr termWord,PStr searchFilter, Byte op, PStr template);
PStr GetLDAPTemplate(PStr template,short id,PStr forHost);

OSErr LDAPSearch(LDAPSessionHdl ldapSession, Str255 searchStr, PStr forHost, Boolean useRawSearchStr, short searchScope, Str255 searchBaseObject, LDAPAttrListHdl attributesList)

{
#ifdef LDAP_ENABLED
	OSErr					err;
	int						result;
#if ASYNC_LDAP_CALLS
	int						opMsgID;
#endif
	LDAPMessage*	searchResults;
	Str255				searchFilter;
	Str255				baseObjectDN;
	RawLDAPAttrList		rawAttrList;
	Ptr						p;


	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;

	if ((**ldapSession).searchResultsValid)
	{
		err = ClearLDAPSearchResults(ldapSession);
		if (err)
			return err;
	}

	if (!searchStr[0])
		return noErr;

	p = NewPtr(kLDAPSearchPreflightMemSize);
	if (!p)
		return memFullErr;
	DisposePtr(p);

	if (useRawSearchStr)
		BlockMoveData(searchStr, searchFilter, searchStr[0] + 1);
	else
	{
		if (searchStr[0] > 250)
			return paramErr;
		BuildLDAPSearchFilter(searchStr,searchFilter,forHost);
	}
#ifdef DEBUG
	PCopy((**ldapSession).searchFilter,searchFilter);
#endif
	p2cstr(searchFilter);

	if (searchBaseObject)
	{
		BlockMoveData(searchBaseObject, baseObjectDN, searchBaseObject[0] + 1);
		p2cstr(baseObjectDN);
	}
	else
		baseObjectDN[0] = 0;

	if (attributesList)
		rawAttrList = DerefLDAPAttrList(attributesList);
	else
		rawAttrList = nil;

	/* FINISH *//* set time and size limits */
#if ASYNC_LDAP_CALLS
	opMsgID = ldap_search((**ldapSession).ldapRef, baseObjectDN, searchScope, (const char*)searchFilter, rawAttrList, 0);
	(**ldapSession).opActive = true;
	(**ldapSession).opMsgID = opMsgID;
	result = LDAPWaitForCompletion(ldapSession, &searchResults);
#else
	result = ldap_search_s((**ldapSession).ldapRef, baseObjectDN, searchScope, (const char*)searchFilter, rawAttrList, 0, &searchResults);
#endif

	if (attributesList)
		UnderefLDAPAttrList(attributesList);

	(**ldapSession).searchErr = result;
	(**ldapSession).searchResults = searchResults;
	if (searchResults)
		(**ldapSession).searchResultsValid = true;

	return result;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}

/************************************************************************
 * BuildLDAPSearchFilter - build the search filter
 ************************************************************************/
OSErr BuildLDAPSearchFilter(PStr searchStr,PStr searchFilter,PStr forHost)
{
	Str255 termWord;
	Str255 template;
	Str63 delims;
	UPtr spot;
	short terms=0;
	
	*searchFilter = 0;
	GetRString(delims,LDAP_SEARCH_TOKEN_DELIMS);
	spot = searchStr+1;
	
	GetLDAPTemplate(template,LDAP_SEARCH_FILTER,forHost);
	
	if (*template)
		// add words to the filter one at a time
		while (PToken(searchStr,termWord,&spot,delims+1))
		{
			terms++;
			AddToLDAPSearchFilter(termWord,searchFilter,'&',template);
		}
	
	// if more than one term or no wordwise search, add whole list, too
	if (!*template || terms>1) AddToLDAPSearchFilter(searchStr,searchFilter,'|',GetLDAPTemplate(template,LDAP_CN_SEARCH_FILTER,forHost));
	
	return(noErr);
}

/************************************************************************
 * AddToLDAPSearchFilter - add a single word to the search filter
 ************************************************************************/
OSErr AddToLDAPSearchFilter(PStr termWord,PStr searchFilter, Byte op, PStr template)
{
	UPtr spot, end;
	short roomLeft = 250 - *searchFilter;
	short maxWordLen;
	short combLen;
	Str255 termFilter;
	Str255 localSearchFilter;
	short numSub=0;
	
	// figure out how much room the combining will take
	combLen = *searchFilter ? *GetRString(localSearchFilter,LDAP_TERM_COMBINER) - 5	// take out the two %p's and one char from %c
														: 0;	// no combining needed

	// count the substitutions in the term template
	if (!*template) return(noErr); // if the template is empty, forget it
	end = template+*template;
	for (spot=template+1;spot<end;spot++)
		if (spot[0]=='^' && spot[1]=='0') numSub++;
	
	// Subtract the fixed stuff
	roomLeft -= combLen + *template - numSub*2;	// add in 2 for each ^0
	
	// calculate the max word that would fit
	maxWordLen = roomLeft/numSub;
	
	if (maxWordLen<2) return(errAEIllegalIndex);	// forget it; doesn't fit
	
	// do we need to trim the word?
	if (*termWord > maxWordLen)
	{
		*termWord = maxWordLen;
		termWord[maxWordLen] = '*';	//becomes a substring match
	}
	
	// Ok, that was fun.  Now we build the term
	utl_PlugParams(template,termFilter,termWord,nil,nil,nil);
	
	// Replace whitespace with *
	end = termFilter+*termFilter;
	for (spot=termFilter+1;spot<end;spot++) if (IsWhite(*spot)) *spot = '*';
	
	// comb the term for ** and reduce to *
	for (spot=termFilter+1;spot<end;spot++)
		if (spot[0]=='*' && spot[1]=='*')
		{
			BMD(spot+1,spot,end-spot);
			end--;
			spot--; // rescan to catch triples, etc
			--*termFilter;
		}
	
	// do we need to combine?
	if (!*searchFilter)
		PCopy(searchFilter,termFilter);	// nope; just copy
	else
	{
		ComposeRString(localSearchFilter,LDAP_TERM_COMBINER,op,termFilter,searchFilter);
		PCopy(searchFilter,localSearchFilter);
	}
	
	// we win
	return(noErr);
}

/************************************************************************
 * GetLDAPTemplate - get a template for an LDAP search
 ************************************************************************/
PStr GetLDAPTemplate(PStr template,short id,PStr forHost)
{
	Handle h;
	OSType type = id==LDAP_SEARCH_FILTER ? LDAP_FILTER_TYPE : LDAP_CN_FILTER_TYPE;
	
	if (h=GetNamedResource(type,forHost))
		PCopy(template,*h);
	else
		GetRString(template,id);
	return(template);
}
	

OSErr OpenLDAPSession(LDAPSessionHdl ldapSession, Str255 ldapServer, int ldapPortNo)

{
#ifdef LDAP_ENABLED
	LDAP*		ldapRef;
	int			result;
#if ASYNC_LDAP_CALLS
	int			opMsgID;
#endif
	Str255	ldapServer_CStr;
	Ptr			p;
	short		oldRes;


	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;
	if ((**ldapSession).sessionOpen)
		return paramErr;

	p = NewPtr(kLDAPOpenPreflightMemSize);
	if (!p)
		return memFullErr;
	DisposePtr(p);

	BlockMoveData(ldapServer, ldapServer_CStr, ldapServer[0] + 1);
	p2cstr(ldapServer_CStr);
	oldRes = CurResFile();
	ldapRef = ldap_open((const char*)ldapServer_CStr, ldapPortNo);
	UseResFile(oldRes);
	if (!ldapRef)
		return LDAP_SERVER_DOWN; /* FINISH *//* any way to get a real result code here? */

	(**ldapSession).ldapRef = ldapRef;

#if ASYNC_LDAP_CALLS
	opMsgID = ldap_simple_bind(ldapRef, nil, nil);
	(**ldapSession).opActive = true;
	(**ldapSession).opMsgID = opMsgID;
	result = LDAPWaitForCompletion(ldapSession, nil);
#else
	result = ldap_simple_bind_s(ldapRef, nil, nil);
#endif
	if (result != LDAP_SUCCESS)
		return result; /* FINISH *//* probably leaks memory from ldap_open */

	(**ldapSession).sessionOpen = true;
	return noErr;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


OSErr CloseLDAPSession(LDAPSessionHdl ldapSession)

{
#ifdef LDAP_ENABLED
	OSErr		err;
	int			result;


	if (!LDAPCodeLoaded)
		return kDFLLibCodeNotLoadedErr;
	if (!(**ldapSession).sessionOpen)
		return noErr;

	if ((**ldapSession).searchResultsValid)
	{
		err = ClearLDAPSearchResults(ldapSession);
		if (err)
			return err;
	}

	result = ldap_unbind((**ldapSession).ldapRef);
	if (result != LDAP_SUCCESS)
		return result;

	(**ldapSession).sessionOpen = false;
	(**ldapSession).ldapRef = nil;
	return noErr;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


OSErr NewLDAPSession(LDAPSessionHdl *ldapSession)

{
	LDAPSessionHdl	sessionHdl;


	sessionHdl = (LDAPSessionHdl)NewHandleClear(sizeof(LDAPSessionRec));
	if (!sessionHdl)
		return MemError();
	(**sessionHdl).sessionOpen = false;
	(**sessionHdl).opActive = false;
	(**sessionHdl).searchResultsValid = false;
	*ldapSession = sessionHdl;
	return noErr;
}


OSErr DisposeLDAPSession(LDAPSessionHdl ldapSession)

{
	OSErr		err;


	if ((**ldapSession).sessionOpen)
	{
		err = CloseLDAPSession(ldapSession);
		if (err)
			return err;
	}
	DisposeHandle((Handle)ldapSession);
	return noErr;
}


OSErr GetLDAPServerList(Handle *serverList)

{
	OSErr			err;
	Handle		serverListRsrc;
	long			dataSize;
	Handle		h;


	*serverList = nil;

	serverListRsrc = GetResource('TEXT', LDAP_SERVER_LIST_ID);
	err = ResError();
	if (!serverListRsrc && !err)
		err = resNotFound;
	if (err)
		return err;
	HNoPurge(serverListRsrc);

	dataSize = GetHandleSize(serverListRsrc);
	h = NewHandle(dataSize);
	if (!h)
	{
		HPurge(serverListRsrc);
		return MemError();
	}
	BlockMoveData(*serverListRsrc, *h, dataSize);
	HPurge(serverListRsrc);

	*serverList = h;
	return noErr;
}


static OSErr GetLDAPAttrsFromStr(Str255 attrListStr, LDAPAttrListHdl *attributesList)

{
	OSErr							err;
	LDAPAttrListHdl		attrs;
	UPtr							curSpot;
	Str255						curAttrStr;


	*attributesList = nil;

	err = NewLDAPAttrList(&attrs);
	if (err)
		return err;

	curSpot = attrListStr + 1;

	while (PToken(attrListStr, curAttrStr, &curSpot, ","))
		if (curAttrStr[0])
		{
			err = AppendLDAPAttrList(attrs, curAttrStr);
			if (err)
			{
				DisposeLDAPAttrList(attrs);
				return err;
			}
		}

	err = CompactLDAPAttrList(attrs);
	if (err)
	{
		DisposeLDAPAttrList(attrs);
		return err;
	}

	*attributesList = attrs;
	return noErr;
}


static short SearchScopeStrToVal(Str255 searchScopeStr)

{
	Str255		curTestStr;


	GetRString(curTestStr, LDAP_SCOPE_BASE_STR);
	if (EqualString(searchScopeStr, curTestStr, false, true))
		return LDAP_SCOPE_BASE;
	GetRString(curTestStr, LDAP_SCOPE_ONELEVEL_STR);
	if (EqualString(searchScopeStr, curTestStr, false, true))
		return LDAP_SCOPE_ONELEVEL;
	GetRString(curTestStr, LDAP_SCOPE_SUBTREE_STR);
	if (EqualString(searchScopeStr, curTestStr, false, true))
		return LDAP_SCOPE_SUBTREE;
	return LDAP_SCOPE_SUBTREE;	// spec says default is base, I say screw spec.  SD
}


/* FINISH *//* REMOVE */
static Boolean LDAPURLQueryContainsSearchFilter(Str255 urlQueryStr)

{
	Str255						localQueryStr;
	Str255						curURLPart;
	UPtr							curSpot;


	if (!urlQueryStr)
		return false;
	if (!*urlQueryStr)
		return false;

	BlockMoveData(urlQueryStr, localQueryStr, urlQueryStr[0] + 1);
	FixURLString(localQueryStr);

	curSpot = localQueryStr + 1;

	if (!(PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0]))
		return false;
	if (!(PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0]))
		return false;
	if (!(PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0]))
		return false;
	if (!(PToken(localQueryStr, curURLPart, &curSpot, "/") && curURLPart[0]))
		return false;

	return true;
}


OSErr ParseLDAPURLQuery(Str255 urlQueryStr, Str255 baseObjectDN, LDAPAttrListHdl *attributesList, short *searchScope, Str255 searchFilter)

{
	OSErr							err;
	Str255						localQueryStr;
	Str255						curURLPart;
	UPtr							curSpot;
	LDAPAttrListHdl		attrs;


	if (baseObjectDN)
		*baseObjectDN = 0;
	if (attributesList)
		*attributesList = nil;
	if (searchScope)
		*searchScope = LDAP_SCOPE_SUBTREE;	// spec says default is base, I say screw spec.  SD
	if (searchFilter)
		*searchFilter = 0;

	if (!urlQueryStr)
		return paramErr;
	if (!*urlQueryStr)
		return noErr;

	BlockMoveData(urlQueryStr, localQueryStr, urlQueryStr[0] + 1);
	FixURLString(localQueryStr);

	curSpot = localQueryStr + 1;

	if (PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0])
		if (baseObjectDN)
			BlockMoveData(curURLPart, baseObjectDN, curURLPart[0] + 1);

	if (PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0])
		if (attributesList)
		{
			err = GetLDAPAttrsFromStr(curURLPart, &attrs);
			if (err)
				return err;
			*attributesList = attrs;
		}

	if (PToken(localQueryStr, curURLPart, &curSpot, "?") && curURLPart[0])
		if (searchScope)
			*searchScope = SearchScopeStrToVal(curURLPart);

	if (PToken(localQueryStr, curURLPart, &curSpot, "/") && curURLPart[0])
		if (searchFilter)
			BlockMoveData(curURLPart, searchFilter, curURLPart[0] + 1);

	return noErr;
}


OSErr LDAPErrCodeToMsgStr(int errCode, Str255 errMsg, Boolean addErrNum)

{
	OSErr			err;
	Handle		errTableHdl;
	long			numEntries; /* minus one */
	long			curIndex;
	Boolean		found;
	Ptr				entryPtr;
	short*		entryCodePtr;
	StringPtr	entryMsgPtr;
	long			entryMsgLen;


	errMsg[0] = 0;
	if (!errCode)
		return noErr;

	if (addErrNum)
		NumToString(errCode, errMsg);

	errTableHdl = GetResource(kLDAPErrTableRsrcType, kLDAPErrTableRsrcID);
	err = ResError();
	if (!errTableHdl && !err)
		err = resNotFound;
	if (err == resNotFound)
		return noErr;
	if (err)
		return err;

	numEntries = **(short**)errTableHdl;
	if (numEntries < 0)
		return noErr;

	curIndex = 0;
	entryPtr = *errTableHdl + 2;
	found = false;
	while (curIndex < numEntries)
	{
		entryCodePtr = (short*)entryPtr;
		entryMsgPtr = (StringPtr)(entryPtr + 2);
		if (*entryCodePtr == errCode)
		{
			found = true;
			break;
		}
		curIndex++;
		entryPtr += 2 + *entryMsgPtr + 1;
		if ((long)entryPtr & 1)
			entryPtr++; /* make it word-aligned */
	}
	if (!found)
		return noErr;

	if (addErrNum)
	{
		errMsg[++errMsg[0]] = ' ';
		errMsg[++errMsg[0]] = ' ';
		entryMsgLen = *entryMsgPtr;
		if ((entryMsgLen + errMsg[0]) > 255)
			entryMsgLen = 255 - errMsg[0];
		BlockMoveData(entryMsgPtr + 1, errMsg + errMsg[0] + 1, entryMsgLen);
		errMsg[0] += entryMsgLen;
	}
	else
		BlockMoveData(entryMsgPtr, errMsg, *entryMsgPtr + 1);

	HPurge(errTableHdl);
	return noErr;
}


short GetLDAPSpecialErrMsgIndex(OSErr theError)

{
	switch (theError)
	{
		case kDFLNeedMixedModeMgrErr:
		case kDFLNeedCFMErr:
			return LDAP_NEEDS_CFM68K_MSG;
		case kDFLBadCFM68KVersErr:
			return LDAP_NEEDS_NEWER_CFM68K_MSG;
		case kDFLBadLibVersErr:
			return LDAP_LIB_VERS_BAD_MSG;
		case cfragNoLibraryErr:
			return LDAP_NOT_LOADED_MSG;
		case memFullErr:
			return LDAP_LIB_OPEN_MEM_ERR_MSG;
		default:
			return 0;
	}
}


/* FINISH *//* put this proc back in, but make it work for a 68K true-link to the .o lib file */
#if 0
static Boolean LDAPSharedLibPresent(void)

{
#ifdef LDAP_ENABLED
	OSErr					err, scratchErr;
	long					response;
	Str255				ldapLibName;
	Str255				errStr;
	ConnectionID	connID;
	Ptr						mainAddr;
	Ptr						symbolAddr;
	SymClass			symbolClass;


	if (Gestalt(gestaltCFMAttr, &response) || !(response & (1 << gestaltCFMPresent)))
		return false;
	if ((long)ldap_open == kUnresolvedCFragSymbolAddress)
		return false;
	GetRString(ldapLibName, LDAP_SHARED_LIB_TRUE_NAME);
	if (!ldapLibName[0])
		return false;
	err = GetSharedLibrary(ldapLibName, kCurrentCFragArch, kFindLib, &connID, &mainAddr, errStr);
	if (err)
		return false;
	err = FindSymbol(connID, "\pldap_open", &symbolAddr, &symbolClass);
	scratchErr = CloseConnection(&connID);
	if (err)
		return false;
	if ((long)symbolAddr == kUnresolvedCFragSymbolAddress)
		return false;
	return true;

#else  //ifdef LDAP_ENABLED
	return false;
#endif  //ifdef LDAP_ENABLED
}
#endif


OSErr LoadLDAPCode(void)

{
#ifdef LDAP_ENABLED
	OSErr		err;


	if (LDAPCodeLoaded)
		return noErr;
	if (PrefIsSet(PREF_LDAP_OVERRIDE_DFL_STUB))
		useDFLStubLib = false;
	err = LoadLDAPSharedLib();
	LDAPLibLoadErr = err;
	if (err)
		return err;
	LDAPCodeLoaded = true;
	LDAPCodePurgeable = false;
	return noErr;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


OSErr UnloadLDAPCode(void)

{
#ifdef LDAP_ENABLED

	if (!LDAPCodeLoaded)
		return noErr;

	LDAPCodePurgeable = true;

	return noErr;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


#pragma segment Ph

#if GENERATING68KSEGLOAD
static void UnloadLDAPUtilsSeg(void)

{
	UnloadSeg(LoadLDAPCode);
}
#endif


/* WARNING: It is probably not be safe to call PurgeLDAPCode from a GrowZoneProc.
		This function will generate a call to CFM's CloseConnection routine, which may
		attempt to allocate or move memory, and may move, purge, or dispose GZSaveHnd(). */

#pragma segment Main

OSErr PurgeLDAPCode(void)

{
#ifdef LDAP_ENABLED

	OSErr		err;


	if (!LDAPCodeLoaded)
		return noErr;
	if (!LDAPCodePurgeable)
		return memPurErr;

	err = UnloadLDAPSharedLib();
	if (err)
		return err;

#if GENERATING68KSEGLOAD
	UnloadLDAPUtilsSeg();
#endif

	LDAPCodeLoaded = false;
	LDAPCodePurgeable = false;

	return noErr;

#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


#ifdef LDAP_ENABLED

#pragma segment Main

static long MonitorLDAPCodeGrow_FreeMem(void)

{
	long	maxContigAfterCompact;
	long	maxContigAfterPurge;
	long	scratch;


	maxContigAfterCompact = MaxBlock();
	PurgeSpace(&scratch, &maxContigAfterPurge);
	return (maxContigAfterCompact > maxContigAfterPurge) ? maxContigAfterCompact : maxContigAfterPurge;
}
#endif  //ifdef LDAP_ENABLED


#pragma segment Main

long MonitorLDAPCodeGrow(Boolean forcePurge)

{
#ifdef LDAP_ENABLED
	OSErr						err;
	unsigned long		origFree, newFree;


	if (!LDAPCodeLoaded)
		return 0;
	if (!LDAPCodePurgeable)
		return 0;

	if (!forcePurge && (MonitorLDAPCodeGrow_FreeMem() >= kMonitorLDAPCodeGrowMinSize))
		return 0;

	origFree = FreeMem();
	err = PurgeLDAPCode();
	if (err)
		return 0;
	newFree = FreeMem();
	return newFree - origFree;
#else  //ifdef LDAP_ENABLED
	return 0;
#endif  //ifdef LDAP_ENABLED
}


#pragma segment Ph

void RefreshLDAPSettings(void)

{
	if (!LDAPSupportPresent() && (GetCurDirSvcType() == ldapLookup))
		SetCurDirSvcType(phLookup);
}


#pragma segment Ph

Boolean LDAPSupportPresent(void)

{
#ifdef LDAP_ENABLED
	return LDAPCanUseDFL;
#else  //ifdef LDAP_ENABLED
	return false;
#endif  //ifdef LDAP_ENABLED
}


#pragma segment Ph

OSErr LDAPSupportError(void)

{
#ifdef LDAP_ENABLED
	return LDAPCanUseDFLErr;
#else  //ifdef LDAP_ENABLED
	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


#pragma segment Ph

OSErr InitLDAP(void)

{
#ifdef LDAP_ENABLED

#ifndef LDAP_USE_STD_LINK_MECHANISM
	OSType	libArchType;
#endif


#ifndef LDAP_USE_STD_LINK_MECHANISM
	libArchType = GetROSType(LDAP_SHARED_LIB_ARCH_TYPE);
	if (!(long)libArchType)
		return paramErr;
	LDAPCanUseDFLErr = SystemSupportsDFRLibraries(libArchType);
	LDAPCanUseDFL = !LDAPCanUseDFLErr;
#else
	LDAPCanUseDFLErr = SystemSupportsDFRLibraries(kAnyCFragArch);
	LDAPCanUseDFL = !LDAPCanUseDFLErr;
#endif

	GetRString(LDAPResultCountStr, LDAP_RESULT_COUNT);
	GetRString(LDAPResultErrStr, LDAP_RESULT_ERR);
	GetRString(LDAPUserCanceledMsgStr, LDAP_SEARCH_USER_CANCELED_MSG);
	GetRString(LDAPDividerStr, LDAP_SEPARATOR);
	MinLDAPLabelLen = GetRLong(MIN_LDAP_LABEL_LEN);

	return noErr;

#else  //ifdef LDAP_ENABLED

	LDAPCanUseDFLErr = paramErr;
	LDAPCanUseDFL = false;
	return noErr;

#endif  //ifdef LDAP_ENABLED
}


#if GENERATING68KSEGLOAD
#pragma mpwc reset
#endif
