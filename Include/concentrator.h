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

#ifndef CONCENTRATOR_H
#define CONCENTRATOR_H
#define bConConPrefNoSameSubj	(1)
#define ConConPrefSameSubj() (0==(GetPrefLong(PREF_CONCON_BITS)&bConConPrefNoSameSubj))

/************************************************************************
 * Profiles & profile components
 ************************************************************************/
typedef struct {
	long lines;
	long bytes;
	long height;
	long width;
} ConConQualifier, *ConConQualPtr, *ConConQualH;

typedef struct {
	ConConKeyEnum type;
	long lines;
	long bytes;
	Boolean trim;
	Boolean flatten;
	short quoteIncrement;
	UHandle text;
	short summaryLevel;
} ConConOutDesc, *ConConOutDescPtr, **ConConOutDescH;

typedef struct ConConElementStruct {
	long elementID;
	ConConKeyEnum type;
	InterestHeadEnum headerID;	// calculated; id of header from InterestingHeadStrn
	Str63 headerName;
	ConConQualifier qual;
	ConConOutDesc outDesc;
	struct ConConElementStruct **next;
	struct ConConElementStruct **inner;
} ConConElement, *ConConElmPtr, **ConConElmH;

typedef struct ConConProfileStruct {
	Str31 name;
	uLong hash;
	ConConElmH cceh;
	struct ConConProfileStruct **next;
} ConConProfile, *ConConProPtr, **ConConProH;

/************************************************************************
 * Message parsing data types
 ************************************************************************/
typedef struct {
	long start;		// offset of beginning of structure
	long stop;		// offset of end (plus one) of structure
	short type;		// type of structure found
	short headerID;		// id of header (from InterestingHeadStrn)
	Str63 headerName;	// name of header, not including colon
	short quoteLevel;	// the quote level of this element
} ConConMessElm, *ConConMessElmPtr, **ConConMessElmH;

typedef enum {
	conConParaTypeOrdinary,
	conConParaTypeBlank,
	conConParaTypeWhite,
	conConParaTypeAttr,
	conConParaTypeForwardOn,
	conConParaTypeForwardOff,
	conConParaTypeQuoteOn,
	conConParaTypeQuoteOff,
	conConParaTypeSigIntro,
	conConParaTypeAttachment,
	conConParaTypeComplete
} ConConParaType, *ConConParaTypePtr, **ConConParaTypeH;

typedef struct {
	long start;		// offset of start of paragraph
	long stop;		// offset of end (plus one) of paragraph
	short quoteLevel;	// quoting level
	short quoteChars;	// # of quote chars found (in addition to quote level, if any)
	ConConParaType type;	// paragraph type
#ifdef DEBUG
	Str63 paraStr;		// for debugging, an idea of what's in the paragraph
#endif
} ConConPara, *ConConParaPtr, **ConConParaH;

typedef struct {
	OSErr err;		// error value
	ConConKeyEnum state;	// current state of the parser
	long spot;		// current spot we're looking at
	MessHandle messH;	// message we're parsing
	StackHandle messStack;	// stack we're putting stuff into
	ConConMessElm thisElm;	// element currently being parsed
	ConConPara lastPara;	// the last paragraph we parsed
	ConConPara thisPara;	// the para we're looking at
	ConConPara nextPara;	// the para after us
	short quoteLevel;	// the quote level where we began
	Boolean presumeForward;	// are first-level quotes forwards?
	Boolean isDigest;	// is the message a digest?
	ConConMessElm lastElm;	// the element directory before us (convenience)
	long end;		// end of the text (convenience)
	UHandle text;		// text (convenience)
	PETEHandle body;	// message body (convenience)
} ConConMessParseContext, *ConConMessPrsPtr, **ConConMessPrsH;


typedef void (*ConConOutFilterProcPtr)(ConConOutDescPtr thisODP,
				       ConConMessElmPtr thisMEP,
				       MessHandle messH, ConConElmH cceh,
				       StackHandle stack, PETEHandle pte,
				       void *userData);

OSErr ConConMessR(MessHandle messH, PETEHandle pte, short conConID,
		  ConConOutFilterProcPtr filter, void *userData);
OSErr ConConMess(MessHandle messH, PETEHandle pte, PStr profileStr,
		 ConConOutFilterProcPtr filter, void *userData);
OSErr ConConMultipleR(TOCHandle tocH, PETEHandle pte, short conConID,
		      short separatorType, ConConOutFilterProcPtr filter,
		      void *userData);
OSErr ConConMultiple(TOCHandle tocH, PETEHandle pte, PStr profileStr,
		     short separatorType, ConConOutFilterProcPtr filter,
		     void *userData);
OSErr ConConAddItems(MenuHandle mh);
OSErr ConConInit(void);
void ConConDispose(void);
ConConProH ConConProFind(PStr profileStr);
ConConProH ConConProFindHash(uLong profileHash);
Boolean ConConMultipleAppropriate(TOCHandle tocH);
#endif				// CONCENTRATOR_H
