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

#ifndef LDAPUTILS_H
#define LDAPUTILS_H

/* MJN *//* new file */



#include "ldap.h"
#include "lber.h"
#include "dflsuppl.h"



/* FINISH *//* SET FINAL */
/* FINISH *//* should this go somewhere else?  mydefs.h?  sysdefs.c? */
#define LDAP_CAPABLE

#pragma mark typedefs

typedef struct {
	Boolean				sessionOpen;
	LDAP*					ldapRef;
	Boolean				opActive;
	int						opMsgID;
	Boolean				searchResultsValid;
	int						searchErr;
	LDAPMessage*	searchResults;
#ifdef DEBUG
	Str255				searchFilter;
#endif
	long					refCon;
} LDAPSessionRec, *LDAPSessionPtr, **LDAPSessionHdl;

typedef char** RawLDAPAttrList;

typedef struct {
	long		numLogicalAttrs;
	long		numPhysicalAttrs;
	Handle	attrList;
} LDAPAttrListRec, *LDAPAttrListPtr, **LDAPAttrListHdl;

typedef void (*LDAPResultsEntryFilterProcPtr)(LDAPSessionHdl ldapSession, Str255 userNameStr, Str255 emailAddressStr, long startOffset, long endOffset);



#pragma mark globals

extern Str255 LDAPDividerStr;



#pragma mark prototypes

#if GENERATING68KSEGLOAD
#pragma mpwc on
#endif

RawLDAPAttrList DerefLDAPAttrList(LDAPAttrListHdl attributesList);
void UnderefLDAPAttrList(LDAPAttrListHdl attributesList);
OSErr NewLDAPAttrList(LDAPAttrListHdl *attributesList);
void DisposeLDAPAttrList(LDAPAttrListHdl attributesList);
OSErr AppendLDAPAttrList(LDAPAttrListHdl attributesList, Str255 attrName);
OSErr CompactLDAPAttrList(LDAPAttrListHdl attributesList);
OSErr LDAPResultsToText(LDAPSessionHdl ldapSession, Handle *resultText, LDAPAttrListHdl attributesList, Boolean translateLabels, LDAPResultsEntryFilterProcPtr entryFilter);
OSErr ClearLDAPSearchResults(LDAPSessionHdl ldapSession);
OSErr LDAPSearch(LDAPSessionHdl ldapSession, Str255 searchStr, PStr forHost, Boolean useRawSearchStr, short searchScope, Str255 searchBaseObject, LDAPAttrListHdl attributesList);
OSErr OpenLDAPSession(LDAPSessionHdl ldapSession, Str255 ldapServer, int ldapPortNo);
OSErr CloseLDAPSession(LDAPSessionHdl ldapSession);
OSErr NewLDAPSession(LDAPSessionHdl *ldapSession);
OSErr DisposeLDAPSession(LDAPSessionHdl ldapSession);
OSErr GetLDAPServerList(Handle *serverList);
OSErr ParseLDAPURLQuery(Str255 urlQueryStr, Str255 baseObjectDN, LDAPAttrListHdl *attributesList, short *searchScope, Str255 searchFilter);
OSErr LDAPErrCodeToMsgStr(int errCode, Str255 errMsg, Boolean addErrNum);
short GetLDAPSpecialErrMsgIndex(OSErr theError);
OSErr LoadLDAPCode(void);
OSErr UnloadLDAPCode(void);
OSErr PurgeLDAPCode(void);
long MonitorLDAPCodeGrow(Boolean forcePurge);
void RefreshLDAPSettings(void);
Boolean LDAPSupportPresent(void);
OSErr LDAPSupportError(void);
OSErr InitLDAP(void);

#if GENERATING68KSEGLOAD
#pragma mpwc reset
#endif


#endif  //ifndef LDAPUTILS_H
