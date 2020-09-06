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

#ifndef FILTERS_H
#define FILTERS_H

#ifdef TWO
#include "FiltDefs.h"

typedef struct CountedSpecStruct {
	FSSpec spec;
	short count;
} CSpec, *CSpecPtr, **CSpecHandle;

typedef struct FAction *FActionPtr, **FActionHandle;
typedef struct FAction {
	Handle data;
	FActionHandle next;
	FilterKeywordEnum action;
} FAction;

typedef enum {
	faeInit,
	faeDraw,
	faeClose,
	faeDispose,
	faeResize,
	faeRead,
	faeWrite,
	faeButton,
	faeClick,
	faeSave,
	faeDo,
	faeFind,
	faeCursor,
	faePrint,
	faeHelp,
	faeMBWillRename,
	faeMBDidRename,
	faeMouseGoingDown,
	faeNew,
#ifdef FANCY_FILT_LDEF
	faeListDraw,
#endif
	faeLimit
} FACallEnum;

typedef short FActionProc(FACallEnum callType, FActionHandle action,
			  Rect * r, void *dataPtr);
#define CallAction(callType,act,r,dataPtr)\
	(*(FActionProc *)FATable((*act)->action))(callType,act,r,dataPtr)

#define MAX_ACTIONS	5

typedef enum {
	mbmContains = 1,
	mbmNotContains,
	mbmIs,
	mbmIsnt,
	mbmStarts,
	mbmEnds,
	mbmAppears,
	mbmNotAppears,
	mbmIntersects,
	mbmNotIntersects,
	mbmIntersectsFile,
	mbmNotIntersectsFile,
	mbmRegEx,
	mbmJunkLess,
	mbmJunkMore,
	mbmLimit
} MatchEnum;

typedef enum {
	cjIgnore = 1,
	cjAnd,
	cjOr,
	cjUnless,
	cjLimit
} ConjunctionEnum;

typedef struct {
	Str63 header;
	short headerID;		//0 or FILTER_BODY|FILTER_ADDRESSEE|FILTER_ANY
	Str127 value;
	MatchEnum verb;
	UHandle nickExpanded;
	UHandle nickAddresses;
	RegexpHandle regex;
} MatchTerm, *MTPtr, **MTHandle;


typedef struct {
	long id;
	uLong lastMatch;
} FilterUse, *FUPtr, **FUHandle;

typedef struct {
	Str31 name;
	FSSpec transferSpec;
	Boolean incoming;
	Boolean outgoing;
	Boolean manual;
	MatchTerm terms[2];
	ConjunctionEnum conjunction;
	FActionHandle actions;
	FilterUse fu;
	Boolean kill;
} FilterRecord, *FRPtr, **FRHandle;

#define NFilters (Filters ? GetHandleSize_(Filters)/sizeof(FilterRecord) : 0)
#define FR (*(FRHandle)Filters)

typedef struct {
	TOCHandle tocH;
	short sumNum;
	Boolean openMailbox;
	Boolean openMessage;
	Boolean doReport;
	Boolean dontReport;
	Boolean dontUser;
	Boolean xferred;
	Boolean xferredFromIMAP;
	Boolean print;
	short doNotifyThing;
	FSSpec spec;
	CSpecHandle report;
	CSpecHandle mailbox;
	CSpecHandle message;
	short **sounds;
	short notify;
	Str15 to;
	Str15 cc;
	Str15 bcc;
	BinAddrHandle toAddresses;
	BinAddrHandle ccAddresses;
	BinAddrHandle bccAddresses;
} FilterPB, *FilterPBPtr, **FilterPBHandle;

#define afbOpenMailbox 1
#define afbOpenMessage 2
#define afbUser 1
#define afbReport 2
#define afbTrash 1
#define afbFetch 2

#endif
#endif
