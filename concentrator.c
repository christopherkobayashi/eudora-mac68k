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

#include "concentrator.h"
#define FILE_NUM 146

ConConProH ConConProfiles;

/************************************************************************
 * Local functions
 ************************************************************************/
OSErr ConConMessParse(ConConMessPrsPtr context);
OSErr ConConMessOutput(MessHandle messH, ConConElmH cceh,
		       StackHandle stack, short baseQuote, PETEHandle pte,
		       ConConOutFilterProcPtr filter, void *userData);
void ConConElmSetHeader(ConConElmH cceh, PStr headerName);
Boolean ConConParaAdvance(ConConMessPrsPtr context);
uLong HandleFindChar(UHandle h, uLong offset, uLong terminal,
		     Byte cToFind);
ConConElmH ConConElmFind(ConConMessElmPtr me, ConConElmH cceh);
short ConConElmMatch(ConConMessElmPtr me, ConConElmH cceh);
OSErr ConConElmOutput(ConConMessElmPtr me, MessHandle messH,
		      ConConElmH cceh, PETEHandle pte, StackHandle stack,
		      ConConOutFilterProcPtr filter, void *userData);
void ConConFixTrimOffsets(UHandle text, long *start, long *stop);
OSErr ConConCopy(PETEHandle fromPTE, PETEHandle toPTE, long start,
		 long stop, Boolean flatten);
Boolean ConConFixTruncOffsets(UHandle text, long start, long goalSize,
			      long *stop, PStr ellipsis,
			      ConConMessElmPtr oldME, StackHandle stack);
OSErr PeteParaInfoCopy(PETEHandle fromPTE, uLong fromOffset,
		       PETEHandle toPTE, uLong toOffset,
		       Boolean breakBefore);
PStr ConConTextSnipLo(PStr s, short sizeOfS, UHandle h, uLong start,
		      uLong stop);
#define ConConTextSnip(s,h,start,stop) ConConTextSnipLo(s,sizeof(s),h,start,stop)
OSErr ConConInsertCRIfNeeded(ConConMessElmPtr lastME, ConConMessElmPtr me,
			     ConConElmH cceh, PETEHandle fromPTE,
			     PETEHandle toPTE);
#ifdef DEBUG
void PeteUpdate14(PETEHandle pte);
void PeteUpdate14(PETEHandle pte)
{
	if (BUG14)
		PeteUpdate(pte);
}
#else
#define PeteUpdate14(pte)
#endif
Boolean ConConFixTruncByWord(UPtr start, UPtr * spotP, UPtr end,
			     short *quoteChars);
Boolean ConConFixTruncByPara(UPtr start, UPtr * spotP, UPtr end);
Boolean ConConIsParaAttribution(ConConMessPrsPtr context);
Boolean ConConIsParaForwardOn(ConConMessPrsPtr context);
Boolean ConConIsParaForwardOff(ConConMessPrsPtr context);
Boolean ConConIsParaQuoteOn(ConConMessPrsPtr context);
Boolean ConConIsParaQuoteOff(ConConMessPrsPtr context);
Boolean ConConIsParaSigIntro(ConConMessPrsPtr context);
Boolean ConConIsAttachment(ConConMessPrsPtr context);
Boolean ConConMightAttribution(PStr s);
Boolean ConConMightForwardOn(PStr s, ConConMessPrsPtr context);
Boolean ConConMightForwardOff(PStr s, ConConMessPrsPtr context);
Boolean ConConMightQuoteOn(PStr s);
Boolean ConConMightQuoteOff(PStr s);
Boolean ConConDigestSeparator(PStr s);
Boolean ConConMightSigIntro(PStr s);
Boolean ConConMightAttachment(PStr s);
Boolean ConConAllWhite(PStr s);
#define ConConOrdinaryishType(t) ((t)==conConParaTypeOrdinary || (t)==conConParaTypeBlank || (t)==conConParaTypeWhite)
Boolean TSIsForward(TOCHandle tocH, short sumNum);
Boolean TSIsDigest(TOCHandle tocH, short sumNum);
void ConConDisposePro(ConConProH ccph);
void ConConDisposeElm(ConConElmH cceh);
ConConProH ConConXML2Profile(XMLBinPtr xmlbP);
ConConElmH ConConXML2Elm(XMLBinPtr xmlbP);
PStr ConConXMLContentShortValue(XMLBinPtr xmlbP, PStr string);
short ConConXMLContentID(XMLBinPtr xmlbP);
OSErr ConConXML2Output(XMLBinPtr xmlbP, ConConElmH cceh);

/************************************************************************
 * ConConInit - initialize the content concentrator
 ************************************************************************/
OSErr ConConInit(void)
{
	UHandle profileH = GetResource('TEXT', CONCON_PROFILES_TEXT);
	StackHandle conConStack = nil;
	OSErr err;

	if (!profileH)
		return fnfErr;

	err = StackInit(sizeof(XMLBinary), &conConStack);
	if (!(err = XMLParse(profileH, ConConKeyStrn, nil, conConStack))) {
		XMLBinary xmlb;
		ConConProH ccph;
#ifdef DEBUG
		Accumulator a;

		Zero(a);
		XMLBinStackPrint(&a, conConStack);
		LineLog(LOG_MORE, CONCON_DEBUG_FMT, LDRef(a.data),
			a.offset);
		AccuZap(a);
#endif

		while (!StackPop(&xmlb, conConStack)) {
			if (ccph = ConConXML2Profile(&xmlb))
				LL_Queue(ConConProfiles, ccph,
					 (ConConProH));
			XMLBinDispose(&xmlb);
		}

	}
	XMLBinStackDispose(conConStack);

	return noErr;
}

/************************************************************************
 * ConConXML2Profile - turn an xml binary into a concentrator profile
 ************************************************************************/
ConConProH ConConXML2Profile(XMLBinPtr xmlbP)
{
	ConConProH ccph = NewZH(ConConProfile);
	ConConElmH cceh;
	XMLNVP nvp;
	short i;
	XMLBinary innerXMLB;

	if (ccph) {
		// search for the name attribute
		while (!StackPop(&nvp, xmlbP->attribs)) {
			if (nvp.type == conConKeyName) {
				uLong hash = Hash(nvp.shortValue);
				PSCopy((*ccph)->name, nvp.shortValue);
				(*ccph)->hash = hash;
			} else
				ASSERT(0);
			XMLNVPDispose(&nvp);
		}
		ASSERT(*(*ccph)->name);

		// Now grab the elements
		for (i = 0; !StackItem(&innerXMLB, i, xmlbP->contents);
		     i++) {
			ASSERT(innerXMLB.self.type == kElementTag);
			ASSERT(innerXMLB.self.id == conConKeyElement);
			if (cceh = ConConXML2Elm(&innerXMLB))
				LL_Queue((*ccph)->cceh, cceh,
					 (ConConElmH));
			else
				ASSERT(0);
		}
	}

	return ccph;
}

/************************************************************************
 * ConConXML2Elm - turn an xml binary into a concentrator profile element
 ************************************************************************/
ConConElmH ConConXML2Elm(XMLBinPtr xmlbP)
{
	ConConElmH cceh = NewZH(ConConElement);
	XMLBinary innerXMLB;
	long num;
	short i;

	// we don't do attributes here
	ASSERT(!xmlbP->attribs);

	// we do do contents
	for (i = 0; !StackItem(&innerXMLB, i, xmlbP->contents); i++)
		if (innerXMLB.self.type == kContent) {
			ASSERT(!(*cceh)->type);
			(*cceh)->type = innerXMLB.self.id;
			ASSERT((*cceh)->type >= conConTypeAny
			       && (*cceh)->type < conConTypeComplete);
		} else if (innerXMLB.self.type == kElementTag) {
			switch (innerXMLB.self.id) {
			case conConKeyName:
				ASSERT((*cceh)->type == conConTypeHeader);
				ConConXMLContentShortValue(&innerXMLB,
							   GlobalTemp);
				PSCopy((*cceh)->headerName, GlobalTemp);
				num =
				    FindSTRNIndex(InterestHeadStrn,
						  GlobalTemp);
				(*cceh)->headerID = num;
				break;
			case conConKeyOutput:
				ConConXML2Output(&innerXMLB, cceh);
				break;
			case conConKeyElement:
				ASSERT(0);	// hierarchy goes here ... someday
				break;
			default:
				ASSERT(0);
				break;
			}
		} else
			ASSERT(0);

	return cceh;
}

/************************************************************************
 * ConConXMLContentShortValue - grab the short name of the content inside a tag
 ************************************************************************/
PStr ConConXMLContentShortValue(XMLBinPtr xmlbP, PStr string)
{
	XMLBinary innerXMLB;

	*string = 0;

	while (!StackPop(&innerXMLB, xmlbP->contents)) {
		ASSERT(innerXMLB.self.type == kContent);
		if (innerXMLB.self.type == kContent) {
			ASSERT(*innerXMLB.self.shortValue);
			ASSERT(!*string);
			PCopy(string, innerXMLB.self.shortValue);
		}
		XMLBinDispose(&innerXMLB);
	}

	return string;
}

/************************************************************************
 * ConConXMLContentID - grab the id of the content inside a tag
 ************************************************************************/
short ConConXMLContentID(XMLBinPtr xmlbP)
{
	XMLBinary innerXMLB;
	short id = -1;

	while (!StackPop(&innerXMLB, xmlbP->contents)) {
		ASSERT(innerXMLB.self.type == kContent);
		if (innerXMLB.self.type == kContent)
			id = innerXMLB.self.id;
		else
			ASSERT(0);
		XMLBinDispose(&innerXMLB);
	}

	return id;
}

/************************************************************************
 * ConConXML2Output - turn some binary xml into an output descriptor
 ************************************************************************/
OSErr ConConXML2Output(XMLBinPtr xmlbP, ConConElmH cceh)
{
	XMLBinary innerXMLB;
	long num;

	while (!StackPop(&innerXMLB, xmlbP->contents)) {
		if (innerXMLB.self.type == kElementTag
		    || innerXMLB.self.type == kEmptyElementTag) {
			switch (innerXMLB.self.id) {
			case conConOutModTrim:
				(*cceh)->outDesc.trim = true;
				break;
			case conConOutModFlatten:
				(*cceh)->outDesc.flatten = true;
				break;
			case conConQualBytes:
				ConConXMLContentShortValue(&innerXMLB,
							   GlobalTemp);
				StringToNum(&GlobalTemp, &num);
				(*cceh)->outDesc.bytes = num;
				break;
			case conConOutModQuoteIncrement:
				ConConXMLContentShortValue(&innerXMLB,
							   GlobalTemp);
				StringToNum(&GlobalTemp, &num);
				(*cceh)->outDesc.quoteIncrement =
				    num ? num : 1;
				break;
			default:
				ASSERT(0);
				break;
			}
		} else if (innerXMLB.self.type == kContent) {
			(*cceh)->outDesc.type = innerXMLB.self.id;
			ASSERT(innerXMLB.self.id);
		} else
			ASSERT(0);

		XMLBinDispose(&innerXMLB);
	}

	return noErr;
}

/************************************************************************
 * ConConDispose - get rid of the content concentrator profiles
 ************************************************************************/
void ConConDispose(void)
{
	ConConProH ccph;

	while (ccph = ConConProfiles) {
		LL_Remove(ConConProfiles, ccph, (ConConProH));
		ConConDisposePro(ccph);
	}
}

/************************************************************************
 * ConConDisposePro - get rid of a content concentrator profile
 ************************************************************************/
void ConConDisposePro(ConConProH ccph)
{
	ConConElmH cceh;

	while (cceh = (*ccph)->cceh) {
		LL_Remove((*ccph)->cceh, cceh, (ConConElmH));
		ConConDisposeElm(cceh);
	}
}

/************************************************************************
 * ConConDisposeElm - get rid of a content concentrator profile
 ************************************************************************/
void ConConDisposeElm(ConConElmH cceh)
{
	ZapHandle((*cceh)->outDesc.text);
	ZapHandle(cceh);
}

/************************************************************************
 * ConConMessR - concentrate a message into (the insertion point of)
 *  a PETEHandle, using a resource to find the concentrator profile
 ************************************************************************/
OSErr ConConMessR(MessHandle messH, PETEHandle pte, short conConID,
		  ConConOutFilterProcPtr filter, void *userData)
{
	Str63 profile;
	return ConConMess(messH, pte, GetRString(profile, conConID),
			  filter, userData);
}

/************************************************************************
 * ConConTS - concentrate a message into (the insertion point of)
 *  a PETEHandle
 ************************************************************************/
OSErr ConConMess(MessHandle messH, PETEHandle pte, PStr profileStr,
		 ConConOutFilterProcPtr filter, void *userData)
{
	if (HasFeature(featureConCon)) {
		ConConProH ccph = ConConProFind(profileStr);
		StackHandle messStack;
		OSErr err;
		ConConMessParseContext context;

		if (!ccph)
			return fnfErr;

		if (err = StackInit(sizeof(ConConMessElm), &messStack))
			return err;

		// Make sure the message is ready to go
		HiliteOddReply(messH);

		// Boring context setup
		Zero(context);
		context.messH = messH;
		context.messStack = messStack;
		context.body = (*messH)->bodyPTE;
		context.end = PeteLen(context.body);
		context.presumeForward =
		    TSIsForward((*messH)->tocH, (*messH)->sumNum);
		context.isDigest =
		    TSIsDigest((*messH)->tocH, (*messH)->sumNum);
		context.state = conConTypeHeader;
		PeteGetTextAndSelection(context.body, &context.text, nil,
					nil);
		ConConParaAdvance(&context);	// Load up the first paragraph

		// Do the parsing
		err = ConConMessParse(&context);

		// Do the output
		if (!err) {
			ConConElmOutput(nil, nil, nil, nil, nil, nil, nil);
			err =
			    ConConMessOutput(messH, (*ccph)->cceh,
					     messStack, 0, pte, filter,
					     userData);
		}

		ZapHandle(messStack);

		return err;
	} else
		return fnfErr;
}

/************************************************************************
 * ConConMessParse - parse a message to get ready for the content concentrator
 ************************************************************************/
OSErr ConConMessParse(ConConMessPrsPtr context)
{
	long spot;
	Str255 s;
	Boolean needRescan;
	Boolean needQueue;

#ifdef DEBUG
	if (RunType == Debugging)
		ComposeLogS(LOG_MORE, nil, "\pConcentrating %p",
			    MyGetWTitle(GetMyWindowWindowPtr
					((*context->messH)->win), s));
#endif

	do {
		if (!ConConParaAdvance(context))
			break;	// grab the next paragraph

	      rescan:
		Zero(context->thisElm);
		needRescan = false;
		needQueue = true;

		if (context->quoteLevel > context->thisPara.quoteLevel)
			break;	// if the quote level is now
		// less than when we started
		// we've come to the end of the
		// quote we have been processing

		// Assume this element is this paragraph
		context->thisElm.start = context->thisPara.start;
		context->thisElm.stop = context->thisPara.stop;
		context->thisElm.quoteLevel = context->thisPara.quoteLevel;

		// Adjust the element
		switch (context->state) {
			////////////////////////
		case conConTypeHeader:
			////////////////////////
			{
				if (context->thisElm.stop ==
				    context->thisElm.start + 1) {
					// message separator found
					context->thisElm.type =
					    conConTypeSeparator;
					context->state =
					    conConTypeBodyInitial;
					break;
				} else {
					// We're in the headers.  The next paragraph may belong with
					// us, since the headers may be folded.  Deal.
					while (context->nextPara.stop > context->nextPara.start &&	// there is a next paragraph that's not empty
					       IsWhite((*context->text)[context->nextPara.start]))	// the first character is whitespace
						ConConParaAdvance(context);	// include it!

					// Point the end of the element at the end of this paragraph
					context->thisElm.stop =
					    context->thisPara.stop;

#ifdef DEBUG
					MakePStr(s,
						 (*context->text) +
						 context->thisElm.start,
						 context->thisElm.stop -
						 context->thisElm.start);
#endif

					// Ok, header gathered.  Let's see what we have
					context->thisElm.type =
					    conConTypeHeader;
					spot =
					    HandleFindChar(context->text,
							   context->
							   thisElm.start,
							   context->
							   thisElm.stop,
							   ':');
					if (spot < context->thisElm.stop) {
						// we have a header name!
						MakePStr(s,
							 (*context->text) +
							 context->thisElm.
							 start,
							 spot -
							 context->thisElm.
							 start);
						TrimAllWhite(s);
						PSCopy(context->thisElm.
						       headerName, s);
						context->thisElm.headerID =
						    FindSTRNIndex
						    (InterestHeadStrn,
						     context->thisElm.
						     headerName);
					} else {
						// hmmmmmmmmmm.  No colon found, so this isn't really a header.
						// We should do something clever here to figure out what this is,
						// like look ahead, behind, or over our right shoulder.  For now,
						// however, we're just going to drool on ourselves
						GetRString(context->
							   thisElm.
							   headerName,
							   UNTITLED);
					}
					break;
				}
			}

			////////////////////////
		case conConTypeBodyInitial:
			////////////////////////
			{
				if (context->thisElm.quoteLevel)
					context->state =
					    context->
					    presumeForward ?
					    conConTypeForward :
					    conConTypeQuote;
				else if (ConConIsParaAttribution(context))
					context->state =
					    conConTypeAttribution;
				else if (ConConIsParaForwardOn(context))
					context->state =
					    conConTypeForwardOn;
				else if (ConConIsParaQuoteOn(context))
					context->state = conConTypeQuoteOn;
				else if (ConConIsParaSigIntro(context))
					context->state =
					    conConTypeSigIntro;
				else
					context->state = conConTypeBody;
				needRescan = true;
				needQueue = false;
				break;
			}

			////////////////////////
		case conConTypePlainForward:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypePlainForward;

				// We're in a plaintext quote; collect until we're not
				do {
					if (ConConIsParaForwardOff
					    (context)) {
						context->state =
						    conConTypeForwardOff;
						needRescan = true;
						break;
					}
				}
				while (ConConParaAdvance(context));

				if (context->state != conConTypeForward)
					context->thisElm.stop =
					    context->lastPara.stop;
				else {
					context->thisElm.stop =
					    context->lastPara.stop;
					context->state =
					    conConTypeComplete;
				}

				break;
			}

			////////////////////////
		case conConTypeOutlookQuote:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypeOutlookQuote;

				// We're in a plaintext quote; collect until we're not
				do {
					if (ConConIsParaQuoteOff(context)) {
						context->state =
						    conConTypeQuoteOff;
						needRescan = true;
						break;
					}
				}
				while (ConConParaAdvance(context));

				if (context->state !=
				    conConTypeOutlookQuote)
					context->thisElm.stop =
					    context->lastPara.stop;
				else {
					context->thisElm.stop =
					    context->lastPara.stop;
					context->state =
					    conConTypeComplete;
				}

				break;
			}

			////////////////////////
		case conConTypeForward:
			////////////////////////
			{
				context->thisElm.type = conConTypeForward;

				// We're in a plaintext quote; collect until we're not
				while (context->nextPara.quoteLevel >=
				       context->thisElm.quoteLevel
				       && ConConParaAdvance(context));

				if (context->thisPara.type ==
				    conConParaTypeComplete)
					context->thisElm.stop =
					    context->lastPara.stop;
				else {
					context->thisElm.stop =
					    context->thisPara.stop;
					context->state = conConTypeBody;
				}

				break;
			}


			////////////////////////
		case conConTypeSignature:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypeSignature;

				// We're in a signature; collect until something happens
				while (ConConOrdinaryishType
				       (context->nextPara.type)
				       && !context->nextPara.quoteLevel
				       && ConConParaAdvance(context));

				if (context->thisPara.type !=
				    conConParaTypeComplete) {
					context->thisElm.stop =
					    context->thisPara.stop;
					context->state = conConTypeQuote;
				} else {
					context->thisElm.stop =
					    context->lastPara.stop;
					context->state =
					    conConTypeComplete;
				}

				break;
			}

			////////////////////////
		case conConTypeBody:
			////////////////////////
			{
				context->thisElm.type = conConTypeBody;

				// We're in the body, in the original text.  Collect original text forever
				do {
					if (ConConIsParaForwardOn(context)) {
						context->state =
						    conConTypeForwardOn;
						break;
					}
					if (ConConIsParaQuoteOn(context)) {
						context->state =
						    conConTypeQuoteOn;
						break;
					} else
					    if (ConConIsParaSigIntro
						(context)) {
						context->state =
						    conConTypeSignature;
						break;
					} else
					    if (ConConIsParaAttribution
						(context)) {
						context->state =
						    conConTypeAttribution;
						break;
					}
				}
				while (!context->nextPara.quoteLevel
				       && ConConParaAdvance(context));

				if (context->state == conConTypeBody) {
					if (context->nextPara.quoteLevel) {
						context->thisElm.stop =
						    context->thisPara.stop;
						context->state =
						    context->
						    presumeForward ?
						    conConTypeForward :
						    conConTypeQuote;
					} else {
						context->thisElm.stop =
						    context->lastPara.stop;
						context->state =
						    conConTypeComplete;
					}
				} else {
					context->thisElm.stop =
					    context->lastPara.stop;
					needRescan = true;
				}
				break;
			}

			////////////////////////
		case conConTypeAttribution:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypeAttribution;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeQuote;
				break;
			}

			////////////////////////
		case conConTypeForwardOn:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypeForwardOn;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypePlainForward;
				break;
			}

			////////////////////////
		case conConTypeQuoteOn:
			////////////////////////
			{
				context->thisElm.type = conConTypeQuoteOn;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeOutlookQuote;
				break;
			}

			////////////////////////
		case conConTypeQuoteOff:
			////////////////////////
			{
				context->thisElm.type = conConTypeQuoteOff;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeBodyInitial;
				break;
			}

			////////////////////////
		case conConTypeForwardOff:
			////////////////////////
			{
				context->thisElm.type =
				    conConTypeForwardOff;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeBodyInitial;
				break;
			}

			////////////////////////
		case conConTypeSigIntro:
			////////////////////////
			{
				context->thisElm.type = conConTypeSigIntro;
				context->thisElm.start =
				    context->thisPara.start;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeSignature;
				break;
			}

			////////////////////////
		case conConTypeQuote:
			////////////////////////
			{
				while (context->nextPara.quoteLevel
				       && ConConParaAdvance(context));

				context->thisElm.type = conConTypeQuote;
				context->thisElm.stop =
				    context->thisPara.stop;
				context->state = conConTypeBodyInitial;
				break;
			}

			////////////////////////
		default:
			////////////////////////
			{
				// we weren't supposed to get here
				context->err = unimpErr;
				break;
			}
		}

		// Post-process the element
		if (!context->err) {

#ifdef DEBUG
			// put the first part of the element in the "headerName"
			if (!(*context->thisElm.headerName))
				ConConTextSnip(context->thisElm.headerName,
					       context->text,
					       context->thisElm.start,
					       context->thisElm.stop);
#endif

			// Put our element on the stack
			if (needQueue
			    && context->thisElm.start !=
			    context->thisElm.stop) {
				context->lastElm = context->thisElm;
				context->err =
				    StackQueue(&context->thisElm,
					       context->messStack);
#ifdef DEBUG
				if (RunType == Debugging)
					ComposeLogS(LOG_MORE, nil,
						    "\pElement %r q%d %d-%d �%e�",
						    ConConKeyStrn +
						    context->thisElm.type,
						    context->thisElm.
						    quoteLevel,
						    context->thisElm.start,
						    context->thisElm.stop,
						    context->thisElm.
						    headerName);
#endif
			}
			// rescan the current element?
			if (needRescan)
				goto rescan;
		}
	}
	while (!context->err);

	return context->err;
}

/************************************************************************
 * The ConConIsPara... routines determine if the current paragraph is a
 * particular interesting element.  They differ from the ConConMight...
 * routines because they take into account the context of the parser,
 * (ie, grammar)  whereas the Might... routines look only at the syntax
 * of the paragraph itself
 ************************************************************************/

Boolean ConConIsParaAttribution(ConConMessPrsPtr context)
{
	// Must be less quoted than what follows
	if (context->thisPara.quoteLevel >= context->nextPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeAttr;
}

Boolean ConConIsParaForwardOn(ConConMessPrsPtr context)
{
	// Must not be quoting here
	if (context->thisPara.quoteLevel || context->nextPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeForwardOn;
}

Boolean ConConIsParaForwardOff(ConConMessPrsPtr context)
{
	// Must not be quoting here
	if (context->thisPara.quoteLevel || context->nextPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeForwardOff;
}

Boolean ConConIsParaQuoteOn(ConConMessPrsPtr context)
{
	// Must not be quoting here
	if (context->thisPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeQuoteOn;
}

Boolean ConConIsParaQuoteOff(ConConMessPrsPtr context)
{
	// Must not be quoting here
	if (context->thisPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeQuoteOff;
}

Boolean ConConIsParaSigIntro(ConConMessPrsPtr context)
{
	// Quoting should not vary here
	if (context->thisPara.quoteLevel != context->nextPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeSigIntro;
}

Boolean ConConIsAttachment(ConConMessPrsPtr context)
{
	// no quoting
	if (context->thisPara.quoteLevel)
		return false;

	// And must have parsed as such
	return context->thisPara.type == conConParaTypeAttachment;
}


/************************************************************************
 * The ConConMight... routines use paragraph syntax to tentatively
 * identify message elements
 ************************************************************************/

Boolean ConConAllWhite(PStr s)
{
	short i;

	if (s[0] > 254 || s[0] == 1)
		return false;
	for (i = s[0]; i > 0; i++)
		if (!IsAnySP(s[i]))
			return false;

	return true;
}

Boolean ConConMightForwardOn(PStr s, ConConMessPrsPtr context)
{
	// is it too long or too short?
	if (*s < 10 || *s > 40)
		return false;

	// -----Original Message-----
	// --- begin forwarded text
	if (s[1] == '-' && s[2] == '-' && s[3] == '-' &&
	    ItemFromResAppearsInStr(CONCON_FORWARD_ON, s, ","))
		return !ConConMightQuoteOn(s) || context->presumeForward;

	return false;
}

Boolean ConConMightForwardOff(PStr s, ConConMessPrsPtr context)
{
	// is it too long or too short?
	if (*s < 10 || *s > 40)
		return false;

	// -----End Original Message-----
	// --- end forwarded text
	if (s[1] == '-' && s[2] == '-' && s[3] == '-' &&
	    ItemFromResAppearsInStr(CONCON_FORWARD_OFF, s, ","))
		return !ConConMightQuoteOn(s) || context->presumeForward;

	return false;
}

Boolean ConConMightQuoteOn(PStr s)
{
	// is it too long or too short?
	if (*s < 10 || *s > 40)
		return false;

	// -----Original Message-----
	// --- begin forwarded text
	if (s[1] == '-' && s[2] == '-' && s[3] == '-' &&
	    ItemFromResAppearsInStr(CONCON_QUOTE_ON, s, ","))
		return true;

	return false;
}

Boolean ConConMightQuoteOff(PStr s)
{
	// is it too long or too short?
	if (*s < 10 || *s > 40)
		return false;

	// -----End Original Message-----
	// --- end forwarded text
	if (s[1] == '-' && s[2] == '-' && s[3] == '-' &&
	    ItemFromResAppearsInStr(CONCON_QUOTE_OFF, s, ","))
		return true;

	return false;
}

Boolean ConConDigestSeparator(PStr s)
{
	// is it too long or too short?
	if (*s < 4 || *s > 40)
		return false;

	if (s[1] == '-' && s[2] == '-' && s[3] == '-' && s[4] == '-')
		return true;

	return false;
}

Boolean ConConMightAttribution(PStr s)
{
	// Must be a reasonable length
	if (*s < 4 || *s > 127)
		return false;

	// Must end with : and return
	if (s[*s] != '\015' || s[*s - 1] != ':')
		return false;

	// Must begin with a word char
	if (!IsWordChar[s[1]])
		return false;

	// well, that's as smart as I am
	return true;
}

Boolean ConConMightSigIntro(PStr s)
{
	return s[0] == 4 && s[1] == '-' && s[2] == '-' && s[3] == ' '
	    && s[4] == '\015';
}

Boolean ConConMightAttachment(PStr s)
{
	return !AttLine2Spec(s, nil, false);
}

/************************************************************************
 * ConConTextSnipLo - put a snip of text in a string
 ************************************************************************/
PStr ConConTextSnipLo(PStr s, short sizeOfS, UHandle h, uLong start,
		      uLong stop)
{
	Str255 localStr;
	short bite;

	if (stop - start <= sizeOfS - 1) {
		MakePStr(localStr, *h + start, stop - start);
		PCopy(s, localStr);
	} else {
		bite = (sizeOfS - 1) / 2;
		MakePStr(localStr, *h + start, bite);
		PCopy(s, localStr);

		MakePStr(localStr, *h + stop - bite, bite);
		PCatC(s, '�');
		PCat(s, localStr);
	}

	return s;
}

/************************************************************************
 * ConConParaAdvance - advance paragraph records on the context
 *  returns false if this para is not real
 ************************************************************************/
Boolean ConConParaAdvance(ConConMessPrsPtr context)
{
	Str15 quoteStr;
	long spot;

	CycleBalls();

	GetRString(quoteStr, QUOTE_PREFIX);

	// shuffle the records
	context->lastPara = context->thisPara;
	context->thisPara = context->nextPara;

	// parse for the next paragraph
	context->spot = context->thisPara.start;
	context->nextPara.start = context->thisPara.stop;
	context->nextPara.stop =
	    HandleFindChar(context->text, context->nextPara.start,
			   context->end, '\015');
	if (context->nextPara.stop < context->end)
		context->nextPara.stop++;	// include the terminal return
#ifdef DEBUG
	Zero(context->nextPara.paraStr);
	ConConTextSnip(context->nextPara.paraStr, context->text,
		       context->nextPara.start, context->nextPara.stop);
#endif
	context->nextPara.quoteChars = 0;

	// quote levels?
	if (context->nextPara.start < context->end) {
		context->nextPara.quoteLevel =
		    PeteIsExcerptAt(context->body,
				    context->nextPara.start);
		// ok, now here is an ugliness; we may have quote characters on top of excerpt bars
		// These may simply be characters the user has chosen to put in, or they may
		// indicate that excerpt was not used consistently.  Bleah.
		// in fullness of time, we may pay attention to the "lay of the land"
		// in terms of whether or not this number of quote chars is used consistently
		// from one para to the next.  For now, we're keeping it simple.
		for (spot = context->nextPara.start;
		     spot < context->nextPara.stop; spot++) {
			if ((*context->text)[spot] == quoteStr[1])
				context->nextPara.quoteChars++;
			else
				break;
		}
		context->nextPara.quoteLevel +=
		    context->nextPara.quoteChars;
	}
	// set the type
	if (context->nextPara.start >= context->end)
		context->nextPara.type = conConParaTypeComplete;
	else {
		Str255 s;

		// Make it a string for convenience
		MakePStr(s, *context->text + context->nextPara.start,
			 context->nextPara.stop - context->nextPara.start);

		if (s[0] == 1 && s[1] == '\015')
			context->nextPara.type = conConParaTypeBlank;
		else if (ConConAllWhite(s))
			context->nextPara.type = conConParaTypeWhite;
		else if (ConConMightAttribution(s))
			context->nextPara.type = conConParaTypeAttr;
		else if (ConConMightForwardOn(s, context))
			context->nextPara.type = conConParaTypeForwardOn;
		else if (ConConMightForwardOff(s, context))
			context->nextPara.type = conConParaTypeForwardOff;
		else if (ConConMightQuoteOn(s))
			context->nextPara.type = conConParaTypeQuoteOn;
		else if (ConConMightQuoteOff(s) || context->isDigest
			 && ConConDigestSeparator(s))
			context->nextPara.type = conConParaTypeQuoteOff;
		else if (ConConMightSigIntro(s))
			context->nextPara.type = conConParaTypeSigIntro;
		else if (ConConMightAttachment(s))
			context->nextPara.type = conConParaTypeAttachment;
		else
			context->nextPara.type = conConParaTypeOrdinary;
	}

	// if we have data in "thisPara", return true
	return context->thisPara.start < context->end;
}

/************************************************************************
 * ConConMessOutput - given a parsed message, output it to a PETEHandle
 ************************************************************************/
OSErr ConConMessOutput(MessHandle messH, ConConElmH cceh,
		       StackHandle stack, short baseQuote, PETEHandle pte,
		       ConConOutFilterProcPtr filter, void *userData)
{
	ConConElmH thisEH;
	ConConMessElm thisME;
	OSErr err = noErr;

	while (!StackPop(&thisME, stack)) {
		// if we're less quoted than our base, bail
		if (thisME.quoteLevel < baseQuote) {
			// put it back!
			StackPush(&thisME, stack);
			return noErr;
		}
		// find the profile element that matches us
		if (thisEH = ConConElmFind(&thisME, cceh))
			err =
			    ConConElmOutput(&thisME, messH, thisEH, pte,
					    stack, filter, userData);
		else
			err = fnfErr;	// ack!  no instructions on what to do with this!
		// perhaps the right thing to do is just display
		// here, but we'll worry about that later
	}

	return err;
}

/************************************************************************
 * ConConElmFind - find a message element in a list of profile elements
 ************************************************************************/
ConConElmH ConConElmFind(ConConMessElmPtr me, ConConElmH cceh)
{
	ConConElmH foundEH = nil;
	short match;

	CycleBalls();

	// We'll take all the matches
	for (; cceh; cceh = (*cceh)->next) {
		match = ConConElmMatch(me, cceh);

		if (match == 0)
			foundEH = cceh;
		if (match == 1)
			break;
		// match==-1 means we didn't match but should continue looking
	}

	return foundEH;
}

/************************************************************************
 * ConConElmMatch - does a message element match a profile element?
 *  -1 - element doesn't match, but matches may be found later
 *   0 - element matches
 *   1 - element doesn't match, and no more elements are allowed to match it
 ************************************************************************/
short ConConElmMatch(ConConMessElmPtr me, ConConElmH cceh)
{
	if ((*cceh)->type == conConTypeAny)
		return 0;	// wildcard

	if (me->type != (*cceh)->type)
		return -1;	// haven't gotten to a meaty enough element

	// element types are the same.  Now must qualify
	if (me->type == conConTypeHeader) {
		if (*me->headerName && *(*cceh)->headerName) {
			Str63 name;
			PSCopy(name, (*cceh)->headerName);
			if (!StringSame(name, me->headerName))
				return -1;	// keep looking; names don't match
		}
	}
	// do qualifiers here... someday
	return 0;		// it matches!
}

/************************************************************************
 * ConConElmOutput - output a concentrator element
 ************************************************************************/
OSErr ConConElmOutput(ConConMessElmPtr me, MessHandle messH,
		      ConConElmH cceh, PETEHandle pte, StackHandle stack,
		      ConConOutFilterProcPtr filter, void *userData)
{
	OSErr err = unimpErr;
	long trimStart, trimStop;
	UHandle text;
	Boolean muckedWith;
	Str63 ellipsis;
	static ConConMessElm lastME;
	long newTextStart, newTextEnd;
	ConConOutDesc outDesc;

	// reinit?
	if (!me) {
		Zero(lastME);
		return noErr;
	}
	// filter?
	outDesc = (*cceh)->outDesc;
	if (filter)
		(*filter) (&outDesc, me, messH, cceh, stack, pte,
			   userData);

	trimStart = me->start;
	trimStop = me->stop;

	PeteGetTextAndSelection(TheBody, &text, nil, nil);
	PeteGetTextAndSelection(pte, nil, &newTextStart, nil);

	// trim?
	if (outDesc.trim)
		ConConFixTrimOffsets(text, &trimStart, &trimStop);

	switch (outDesc.type) {
		// The easy one; do nothing
	case conConOutTypeRemove:
		err = noErr;
		break;

		// Just copy it into the output
	case conConOutTypeDisplay:
		if (!
		    (err =
		     ConConInsertCRIfNeeded(&lastME, me, cceh, TheBody,
					    pte)))
			err =
			    ConConCopy(TheBody, pte, trimStart, trimStop,
				       outDesc.flatten);
		lastME = *me;
		break;

		// Truncate after a certain size
	case conConOutTypeTruncate:
		muckedWith =
		    ConConFixTruncOffsets(text, trimStart, outDesc.bytes,
					  &trimStop, ellipsis, me, stack);
		if (!
		    (err =
		     ConConInsertCRIfNeeded(&lastME, me, cceh, TheBody,
					    pte)))
			err =
			    ConConCopy(TheBody, pte, trimStart, trimStop,
				       outDesc.flatten);
		if (!err && muckedWith) {
			long oldSel = PeteBumpSelection(pte, 0);

			if (outDesc.text) {
				err = PeteInsert(pte, -1, outDesc.text);
				if (PeteCharAt(pte, oldSel - 1) == '\015')
					PeteEnsureBreak(pte, oldSel);
				PeteBumpSelection(pte,
						  GetHandleSize(outDesc.
								text));
			} else {
				err =
				    PeteInsertPtr(pte, -1, ellipsis + 1,
						  *ellipsis);
				if (PeteCharAt(pte, oldSel - 1) == '\015')
					PeteEnsureBreak(pte, oldSel);
				PeteBumpSelection(pte, *ellipsis);
			}
		}
		lastME = *me;
		break;
	}

	if (outDesc.quoteIncrement) {
		short i;

		newTextEnd = PeteBumpSelection(pte, 0);
		for (i = 0; i < outDesc.quoteIncrement; i++)
			PeteExcerpt(pte, newTextStart, newTextEnd);
	}

	PeteUpdate14(pte);

	return err;
}

/************************************************************************
 * ConConInsertCRIfNeeded - in certain situations, we need to add returns
 *   while outputting elements
 ************************************************************************/
OSErr ConConInsertCRIfNeeded(ConConMessElmPtr lastME, ConConMessElmPtr me,
			     ConConElmH cceh, PETEHandle fromPTE,
			     PETEHandle toPTE)
{
	long offset;
	OSErr err = noErr;

	PeteGetTextAndSelection(toPTE, nil, &offset, nil);

	// are we inserting plain(er?) text after a quote?
	if (lastME->quoteLevel > me->quoteLevel) {
		err = PeteInsertChar(toPTE, offset, '\015', nil);
		if (!err)
			err = PeteEnsureBreak(toPTE, offset);
		if (!err)
			err =
			    PeteSetExcerptLevelAt(toPTE, offset,
						  me->quoteLevel);
		if (!err)
			PeteBumpSelection(toPTE, 1);
	}
	// are we inserting a quote after non-attributional plain(er) text?
	else if (lastME->type != conConTypeSeparator
		 && lastME->quoteLevel < me->quoteLevel
		 && lastME->type != conConTypeAttribution) {
		err = PeteInsertChar(toPTE, offset, '\015', nil);
		if (!err)
			err = PeteEnsureBreak(toPTE, offset);
		if (!err)
			err =
			    PeteSetExcerptLevelAt(toPTE, offset,
						  lastME->quoteLevel);
		if (!err)
			PeteBumpSelection(toPTE, 1);
	} else if ( /*offset==PeteLen(toPTE) && */
		   PeteCharAt(toPTE, offset - 1) == '\015')
		PeteEnsureBreak(toPTE, offset);

	PeteUpdate14(toPTE);

	return err;
}

/************************************************************************
 * ConConCopy - copy from one pte to another, playing the proper games for
 *   the Content Concentrator
 ************************************************************************/
OSErr ConConCopy(PETEHandle fromPTE, PETEHandle toPTE, long start,
		 long stop, Boolean flatten)
{
	short oldID;
	OSErr err;
	uLong selStart, selEnd;

	// we're going to need this later...
	PeteGetTextAndSelection(toPTE, nil, &selStart, nil);
	selEnd = PeteLen(toPTE);

	// Set the copy mask to include the reply-to label for now
	PETESetLabelCopyMask(PETE, LABEL_COPY_MASK | pReplyLabel);

	// do the copy, but fool the graphics into thinking it's the same Pete record
	oldID = (*PeteExtra(fromPTE))->id;
	(*PeteExtra(toPTE))->id = (*PeteExtra(fromPTE))->id;
	// Ick.  If we copy something with para styles, we need to not trample on the para
	// styles of what we're copying just after.  This is voodoo, so far as I'm concerned, but
	// it's going to have to do
	if (PeteCrAt(toPTE, PeteBumpSelection(toPTE, 0) - 1)
	    && PeteBumpSelection(toPTE, 0) == PeteLen(toPTE))
		PeteInsertPlainParaAtEnd(toPTE);
	err = PeteCopy(fromPTE, toPTE, start, stop, -1, nil, flatten);
	(*PeteExtra(toPTE))->id = oldID;

	// Reset the copy mask
	PETESetLabelCopyMask(PETE, LABEL_COPY_MASK);

	if (!err) {
		PeteUpdate14(toPTE);

		// ok, now we have a messy business.  We've got to make the para styles
		// of the beginning and end of what we've done match properly.
		selEnd = selStart + PeteLen(toPTE) - selEnd;

		if (PeteCrAt(toPTE, selStart - 1)) {
			err =
			    PeteParaInfoCopy(fromPTE, start, toPTE,
					     selStart, true);
			PeteUpdate14(toPTE);
		}

		if (!err) {
			err =
			    PeteParaInfoCopy(fromPTE, stop - 1, toPTE,
					     selEnd - 1, false);
			PeteUpdate14(toPTE);
		}
		// finally, let's update the selection
		PeteBumpSelection(toPTE, stop - start);
	}

	return err;
}

/************************************************************************
 * PeteParaInfoCopy - copy para info from one place to another
 ************************************************************************/
OSErr PeteParaInfoCopy(PETEHandle fromPTE, uLong fromOffset,
		       PETEHandle toPTE, uLong toOffset,
		       Boolean breakBefore)
{
	OSErr err;
	long fromPara, toPara;
	PETEParaInfo pInfo;

	// get old para #
	fromPara = PeteParaAt(fromPTE, fromOffset);

	// insert para break if wanted & needed
	if (breakBefore)
		PeteEnsureBreak(toPTE, toOffset);

	// get new para number
	toPara = PeteParaAt(toPTE, toOffset);

	// get/set info
	Zero(pInfo);
	if (!(err = PETEGetParaInfo(PETE, fromPTE, fromPara, &pInfo)))
		err =
		    PETESetParaInfo(PETE, toPTE, toPara, &pInfo,
				    peAllParaValidButTabs);

	return err;
}

/************************************************************************
 * ConConFixTrimOffsets - reduce offsets that include all-white lines,
 *   until we wind up with either no all-white lines or the whole text
 *   is collapsed to a single all-white line
 ************************************************************************/
void ConConFixTrimOffsets(UHandle text, long *start, long *stop)
{
	UPtr startP, stopP, endP;

	startP = *text + *start;
	endP = *text + *stop;

	// find the first non-white character
	for (stopP = startP; stopP < endP; stopP++)
		if (!IsAnySP(*stopP))
			break;

	// Back up to just after the last return
	while (stopP > startP && stopP[-1] != '\015')
		stopP--;

	// Set it!
	startP = stopP;
	*start = startP - *text;

	// find the last non-white character
	stopP = endP;
	while (stopP > startP && IsAnySP(stopP[-1]))
		stopP--;

	// extend to next return
	while (stopP < endP && *stopP++ != '\015');

	// Set it!
	*stop = stopP - *text;
}

/************************************************************************
 * ConConFixWordOffset - adjust an offset so that we don't break off
 *  in the middle of a word, and p
 ************************************************************************/
Boolean ConConFixTruncOffsets(UHandle text, long start, long goalSize,
			      long *stop, PStr ellipsis,
			      ConConMessElmPtr oldME, StackHandle stack)
{
	UPtr startP, endP, trunc1P, trunc2P;
	uLong len = *stop - start;
	uLong truncSize = len - goalSize;
	ConConMessElm newME;
	uLong cr;
	Boolean para1, para2;
	short quoteChars = 0;

	// A little setup
	startP = *text + start;
	endP = *text + *stop;

	// Don't truncate if we're within 20% of the end of our element
	if (goalSize > (len * 8) / 10)
		return false;

	// We want to truncate from the middle, so let's setup offsets to
	// the beginning and end of the truncation
	trunc1P = startP + goalSize / 2;
	trunc2P = endP - goalSize / 2;

	// For grins, let's see where the paragraph boundaries are,
	// since we like to trim at paragraph boundaries if possible
	if (!(para1 = ConConFixTruncByPara(startP, &trunc1P, endP)))
		// nope, do words instead
		ConConFixTruncByWord(startP, &trunc1P, endP, nil);

	// And the second offset?
	if (!(para2 = ConConFixTruncByPara(startP, &trunc2P, endP)))
		// nope, do words instead
		ConConFixTruncByWord(startP, &trunc2P, endP, &quoteChars);

	// Another sanity check; are we taking out enough to matter?
	if (trunc1P - startP + endP - trunc2P > (len * 8) / 10)
		return false;

	// Ok, it's a go
	*stop = trunc1P - *text;

	newME = *oldME;
	newME.start = trunc2P - *text;
	StackPush(&newME, stack);

	// Now, figure out the ellipsis

	// If we're in the body, check to see if there any cr's in what we removed?
	cr = oldME->type != conConTypeHeader
	    && HandleFindChar(text, *stop, newME.start, '\015');
	*ellipsis = 0;

	// if we removed a cr and truncated the first paragraph,
	// put an ellipsis and return on the first paragraph
	if (cr && !para1) {
		PCatR(ellipsis, CONCON_ELLIPSIS_TRAIL);
		PCatC(ellipsis, '\015');
	}
	// the main ellipsis
	PCatR(ellipsis, CONCON_ELLIPSIS_CENTER);

	// if we removed a cr, we need a trailing cr,
	// and possibly an ellipsis at the start of the next
	// paragraph, if we didn't cut cleanly there
	if (cr)
		if (!para2) {
			PCatC(ellipsis, '\015');
			if (quoteChars) {
				Str15 quoteStr;
				GetRString(quoteStr, QUOTE_PREFIX);
				quoteChars = MIN(quoteChars, 10);
				while (quoteChars--)
					PCatC(ellipsis, quoteStr[1]);
			}
			PCatR(ellipsis, CONCON_ELLIPSIS_LEAD);
		} else
			PCatC(ellipsis, '\015');

	return true;
}

/************************************************************************
 * ConConFixTruncByWord - adjust a truncation spot to end on a word
 *   boundary.  Returns true if it was adjusted
 ************************************************************************/
Boolean ConConFixTruncByWord(UPtr start, UPtr * spotP, UPtr end,
			     short *quoteChars)
{
	UPtr spot = *spotP;
	Boolean reverse = *spotP - start > end - *spotP;
	Str15 quoteStr;

	ASSERT(start <= end);
	ASSERT(*spotP >= start);
	ASSERT(*spotP <= end);

	if (quoteChars) {
		UPtr cr;
		GetRString(quoteStr, QUOTE_PREFIX);
		cr = *spotP;
		while (cr > start && cr[-1] != '\-015')
			cr--;
		while (cr < end && *cr++ == quoteStr[1])
			++ * quoteChars;
	}
	// Are we in a word?
	if (IsWordChar[*spot]) {
		if (reverse) {
			// back up until a non-word char is before us
			while (spot > start && IsWordChar[spot[-1]])
				spot--;
		} else {
			// continue until we point at a non-word char
			while (++spot < end)
				if (!IsWordChar[*spot])
					break;
		}
	} else {
		// not in a word.
		if (reverse) {
			// move forward until a word char is found
			while (++spot < end)
				if (IsWordChar[*spot])
					break;
		} else {
			// back up until a word char is before us
			while (spot > start && !IsWordChar[spot[-1]])
				spot--;
		}
	}

	// did we do all that work just to end up where we started?
	if (spot == *spotP)
		return false;

	// These would be silly...
	if (spot == start || spot == end)
		return false;

	// Set it!
	*spotP = spot;

	return true;		// we mucked
}

/************************************************************************
 * ConConFixTruncByPara - adjust a truncation spot to end on a paragaph
 *   boundary.  Returns true if it succeeded in putting it on a proper
 *   boundary.  Returns false if it seems not to make sense to adjust
 *   the truncation paragraphwise
 ************************************************************************/
Boolean ConConFixTruncByPara(UPtr start, UPtr * spotP, UPtr end)
{
	uLong len = end - start;
	Boolean reverse = *spotP - start > end - *spotP;
	long wantToRemove =
	    len - 2 * (reverse ? end - *spotP : *spotP - start);
	long removing;
	UPtr side1, side2, theNominee;
	uLong diff;

	// point at cr's before and after us
	for (side1 = *spotP; side1 > start; side1--)
		if (*side1 == '\015')
			if (side1 == start + 1 || side1[-1] != '\015')
				break;	// find the first one of a run of cr's
	if (side1 == start)
		side1 = nil;

	for (side2 = *spotP; side2 < end; side2++)
		if (*side2 == '\015')
			if (side2 == end - 1 || side2[2] != '\015')
				break;	// find the last one of a run of cr's
	if (side2 == end)
		side2 = nil;

	// did we get any?
	if (!side1 && !side2)
		return false;	// no cr's in sight; bail
	else if (!side1)
		theNominee = side2;
	else if (!side2)
		theNominee = side1;
	else
		// pick the closest one
		theNominee =
		    *spotP - side1 < side2 - *spotP ? side1 : side2;

	// Actually, truncate AFTER the cr
	theNominee++;

	// Now, let's see how far off we are
	removing =
	    len - 2 * (reverse ? end - theNominee : theNominee - start);

	// figure out how far off we are from the removal we want
	diff = ABS(wantToRemove - removing);

	// if the variance is more than 20%, forget it
	if (diff * 5 > wantToRemove)
		return false;

	// hey... we found a good one!
	*spotP = theNominee;

	return true;
}

/************************************************************************
 * ConConMultipleAppropriate - is is appropriate to concentrate all
 *   these messages?
 ************************************************************************/
Boolean ConConMultipleAppropriate(TOCHandle tocH)
{
	short sumNum;
	short count = 0;
	Str63 sub1, sub2;
	Str63 who1, who2;
	short maxCount = GetRLong(CONCON_MULTI_MAX);
	Boolean needSame = ConConPrefSameSubj();

	if (!HasFeature(featureConCon))
		return false;

	(*tocH)->conConMultiScan = false;

	for (sumNum = (*tocH)->count - 1; sumNum >= 0; sumNum--)
		if ((*tocH)->sums[sumNum].selected) {
			if (++count > maxCount)
				return false;
			if (needSame) {
				if (count == 1) {
					PSCopy(sub1,
					       (*tocH)->sums[sumNum].subj);
					PSCopy(who1,
					       (*tocH)->sums[sumNum].from);
				} else {
					PSCopy(sub2,
					       (*tocH)->sums[sumNum].subj);
					PSCopy(who2,
					       (*tocH)->sums[sumNum].from);
					if (!StringSame(who1, who2)
					    && SubjCompare(sub1, sub2))
						return false;
				}
			}
		}

	return count > 1;
}

/************************************************************************
 * ConConMultipleR - concentrate the selected messages in a toc into
 *   (the insertion point of) a PETEHandle, using a resource to find
 *   the concentrator profile
 ************************************************************************/
OSErr ConConMultipleR(TOCHandle tocH, PETEHandle pte, short conConID,
		      short separatorType, ConConOutFilterProcPtr filter,
		      void *userData)
{
	Str63 profile;
	return ConConMultiple(tocH, pte, GetRString(profile, conConID),
			      separatorType, filter, userData);
}

/************************************************************************
 * ConConMultiple - concentrate the selected messages in a toc into
 *   (the insertion point of) a PETEHandle
 ************************************************************************/
OSErr ConConMultiple(TOCHandle tocH, PETEHandle pte, PStr profileStr,
		     short separatorType, ConConOutFilterProcPtr filter,
		     void *userData)
{
	if (HasFeature(featureConCon)) {
		OSErr err = noErr;
		short sumNum;
		short oldLen;
		Boolean opened;
		MessHandle messH;
		Str255 scratch;
		Boolean first = true;

		PeteCalcOff(pte);
		PETEAllowUndo(PETE, pte, false, true);
		UseFeature(featureConCon);

		// any junk?
		(*PeteExtra(pte))->containsJunkMail = false;
		for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
			if ((*tocH)->sums[sumNum].selected
			    && (*tocH)->sums[sumNum].selected >=
			    GetRLong(JUNK_MAILBOX_THRESHHOLD)) {
				(*PeteExtra(pte))->containsJunkMail = true;
				break;
			}
		// now, concentrate!
		for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
			if ((*tocH)->sums[sumNum].selected) {
				opened = false;
				oldLen = PeteLen(pte);
				messH = (*tocH)->sums[sumNum].messH;
				if (messH == nil) {
					TOCHandle realTocH;
					short realSumNum;

					if (realTocH =
					    GetRealTOC(tocH, sumNum,
						       &realSumNum)) {
						EnsureMsgDownloaded(realTocH, realSumNum, false);	// FSOIMAP
						GetAMessage(realTocH,
							    realSumNum,
							    nil, nil,
							    false);
						messH =
						    (*realTocH)->
						    sums[realSumNum].messH;
						opened = messH != nil;
					}
				}
				if (!messH) {
					err = fnfErr;
					break;
				}
				if (oldLen) {
					if (separatorType ==
					    conConOutSeparatorRule) {
						PeteInsertPlainParaAtEnd
						    (pte);
						if (err =
						    PeteInsertRule(pte, -1,
								   0, 0,
								   true,
								   true,
								   true))
							break;
						PeteBumpSelection(pte,
								  PeteLen
								  (pte) -
								  oldLen);
					} else if (separatorType ==
						   conConTypeAttribution) {
						PeteInsertPlainParaAtEnd
						    (pte);
						GrabAttribution
						    (ATTRIBUTION,
						     (*messH)->win,
						     scratch);
						PCatC(scratch, '\015');
						if (!first)
							PInsertC(scratch,
								 sizeof
								 (scratch),
								 '\015',
								 scratch +
								 1);
						if (err =
						    PeteInsertStr(pte, -1,
								  scratch))
							break;
						PeteBumpSelection(pte,
								  PeteLen
								  (pte) -
								  oldLen);
					}
				}
				if (err =
				    ConConMess(messH, pte, profileStr,
					       filter, userData))
					break;
				if (opened)
					CloseMyWindow(GetMyWindowWindowPtr
						      ((*messH)->win));
				first = false;
			}

		if (err)
			PeteSetTextPtr(pte, "", 0);

		PeteCalcOn(pte);

		return err;
	} else
		return fnfErr;
}

/************************************************************************
 * ConConAddItems - add the current set of concentrator profiles to a menu
 ************************************************************************/
OSErr ConConAddItems(MenuHandle mh)
{
	ConConProH ccph;
	Str63 name;

	if (!ConConProfiles)
		return fnfErr;

	for (ccph = ConConProfiles; ccph; ccph = (*ccph)->next) {
		PSCopy(name, (*ccph)->name);
		MyAppendMenu(mh, name);
	}

	return noErr;
}

/************************************************************************
 * ConConProFind - find a concentrator profile
 ************************************************************************/
ConConProH ConConProFind(PStr profileStr)
{
	ConConProH ccph;
	Str63 name;

	for (ccph = ConConProfiles; ccph; ccph = (*ccph)->next) {
		PCopy(name, (*ccph)->name);
		if (StringSame(name, profileStr))
			return ccph;
	}

	return nil;
}

/************************************************************************
 * ConConProFindHash - find a concentrator profile by hash
 ************************************************************************/
ConConProH ConConProFindHash(uLong profileHash)
{
	ConConProH ccph;

	for (ccph = ConConProfiles; ccph; ccph = (*ccph)->next) {
		if ((*ccph)->hash == profileHash)
			return ccph;
	}

	return nil;
}

/************************************************************************
 * ConConElmSetHeader - copy a header into an element, setting the id, too
 ************************************************************************/
void ConConElmSetHeader(ConConElmH cceh, PStr headerName)
{
	short id = FindSTRNIndex(InterestHeadStrn, headerName);

	(*cceh)->type = conConTypeHeader;
	(*cceh)->headerID = id;
	PSCopy((*cceh)->headerName, headerName);
}

/************************************************************************
 * TSIsForward - is a message a forwarded message?
 ************************************************************************/
Boolean TSIsForward(TOCHandle tocH, short sumNum)
{
	Str15 subjStart;
	PSCopy(subjStart, (*tocH)->sums[sumNum].subj);

	if (*subjStart < *Fwd)
		return false;
	*subjStart = *Fwd;
	return StringSame(subjStart, Fwd);
}

/************************************************************************
 * TSIsDigest - is a message a digest message?
 ************************************************************************/
Boolean TSIsDigest(TOCHandle tocH, short sumNum)
{
	Str63 subj;
	PSCopy(subj, (*tocH)->sums[sumNum].subj);

	// does it look like a digest?
	return ItemFromResAppearsInStr(CONCON_DIGEST_ITEMS, subj, ",");
}


/************************************************************************
 * HandleFindChar - find a character in a handle, between an offset
 * and a terminal value.  If not found, return terminal value
 ************************************************************************/
uLong HandleFindChar(UHandle h, uLong offset, uLong terminal, Byte cToFind)
{
	UPtr spot;
	UPtr end = *h + terminal;

	for (spot = *h + offset; spot < end; spot++)
		if (*spot == cToFind)
			break;

	return spot - *h;
}
