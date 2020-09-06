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

#include "filters.h"
/************************************************************************
 * Filters window - Copyright (C) 1993 QUALCOMM Incorporated
 ************************************************************************/
#define FILE_NUM 55

#ifdef TWO
#ifdef OLDFILTER


OSErr FilterTokenToContent(FilterTokenPtr ft, FRPtr fp);
short FilterToken2Num(FilterTokenPtr ft);
OSErr FilterObjSpecifier(long id, AEDescPtr filter);
OSErr SetFilterPropertyLo(FRPtr fr, DescType property, AEDescPtr data);
OSErr StoreFilter(FilterTokenPtr fp, FRPtr fr);
OSErr SetFilterPropertyLo(FRPtr fr, DescType property, AEDescPtr data);
OSErr SetTermPropertyLo(FRPtr fr, short term, DescType property,
			AEDescPtr data);

/**********************************************************************
 * FilterToken2Num - given a filter token, return the filter index
 **********************************************************************/
short FilterToken2Num(FilterTokenPtr ft)
{
	return (ft->form ==
		formAbsolutePosition ? ft->selector -
		1 : FindFilterById(ft->selector));
}

/**********************************************************************
 * FilterTokenToContent - grab a filter for a token
 **********************************************************************/
OSErr FilterTokenToContent(FilterTokenPtr ft, FRPtr fp)
{
	short filter;

	if (!FilterExists(ft->form, ft->selector))
		return (errAENoSuchObject);

	filter = FilterToken2Num(ft);
	FilterLastMatch(filter);
	*fp = FR[filter];
	return (noErr);
}

/**********************************************************************
 * AECreateFilter - create a filter with AE's
 **********************************************************************/
OSErr AECreateFilter(DescType theClass, AEDescPtr insertLoc,
		     AppleEvent * event, AppleEvent * reply)
{
	AEDesc filter;
	OSErr err = RegenerateFilters();
	long id;
	DescType where, junk;
	short n;


	if (!err && !(err = GotAERequired(event))) {
		/*
		 * what's the next filter id?
		 */
		id = FilterNewId();

		/*
		 * what position?
		 */
		if (AEGetKeyPtr
		    (insertLoc, keyAEPosition, typeEnumeration,
		     (void *) &junk, &where, sizeof(where),
		     (void *) &junk))
			n = NFilters;
		else
			switch (where) {
			case kAEEnd:
				n = NFilters;
				break;
			case kAEBeginning:
				n = 0;
				break;
			default:
				n = NFilters;
				break;
			}

		/*
		 * create it
		 */
		AddFilter(n);

		/*
		 * find it
		 */
		if (FilterExists(formUniqueID, id)) {
			NullADList(&filter, nil);
			if (!(err = FilterObjSpecifier(id, &filter)))
				err =
				    AEPutParamDesc(reply, keyAEResult,
						   &filter);
			DisposeADList(&filter, nil);
		}

		/*
		 * is the window open?
		 */
		if (FLG && *FLG && Win)
			/* work is done */ ;
		else {
			/*
			 * sigh.  have to save
			 */
			SaveFilters();
		}
	}
	FiltersDecRef();
	return (err);
}

/**********************************************************************
 * FilterObjSpecifier - make an object specifier for a filter
 **********************************************************************/
OSErr FilterObjSpecifier(long id, AEDescPtr filter)
{
	AEDesc root, idDesc;
	OSErr err;

	NullADList(&root, &idDesc, nil);
	if (!
	    (err =
	     AECreateDesc(typeLongInteger, &id, sizeof(id), &idDesc)))
		err =
		    CreateObjSpecifier(cEuFilter, &root, formUniqueID,
				       &idDesc, False, filter);
	DisposeADList(&root, &idDesc, nil);
}

/**********************************************************************
 * SetFilterProperty - set property for a filter, with fetch & store
 **********************************************************************/
OSErr SetFilterProperty(AEDescPtr token, AEDescPtr data)
{
	DescType property =
	    (*(PropTokenHandle) token->dataHandle)->propertyId;
	FilterToken fToken =
	    *(FilterTokenPtr) ((*token->dataHandle) + sizeof(PropToken));
	OSErr err = RegenerateFilters();
	FilterRecord fr;

	if (!err)
		if (!(err = FilterTokenToContent(&fToken, &fr)))
			err = SetFilterPropertyLo(&fr, property, data);

	/*
	 * store it back
	 */
	if (!err)
		err = StoreFilter(&fToken, &fr);

	FiltersDecRef();
	return (err);
}

/**********************************************************************
 * SetTermProperty - set a property for a term of a filter, with fetch & store
 **********************************************************************/
OSErr SetTermProperty(AEDescPtr token, AEDescPtr data)
{
	OSErr err = RegenerateFilters();
	DescType property =
	    (*(PropTokenHandle) token->dataHandle)->propertyId;
	TermToken tToken =
	    *(TermTokenPtr) ((*token->dataHandle) + sizeof(PropToken));
	FilterRecord fr;

	if (!err)
		if (!(err = FilterTokenToContent(&tToken.filter, &fr)))
			err =
			    SetTermPropertyLo(&fr, tToken.term, property,
					      data);

	/*
	 * store it back
	 */
	if (!err)
		err = StoreFilter(&tToken.filter, &fr);

	FiltersDecRef();
	return (err);
}

/**********************************************************************
 * SetFilterPropertyLo - set a property for a filter, without fetch & store
 **********************************************************************/
OSErr SetFilterPropertyLo(FRPtr fr, DescType property, AEDescPtr data)
{
	Str255 string;
	long value;
	Boolean onOrOff;
	OSErr err = noErr;

	value = GetAELong(data);
	GetAEPStr(string, data);
	onOrOff = GetAEBool(data);

	/*
	 * change our local copy
	 */
	switch (property) {
	case pName:
	case formUniqueID:
		err = errAEEventNotHandled;
		break;
	case pEuFilterUse:
		fr->fu.lastMatch = value;
		break;
	case pEuManual:
		fr->manual = onOrOff;
		break;
	case pEuOutgoing:
		fr->outgoing = onOrOff;
		break;
	case IN:
		fr->incoming = onOrOff;
		break;
	case pEuConjunction:
		fr->conjunction = value & 0xff;
		break;
	case pEuSubject:
		PSCopy(fr->subject, string);
		break;
	case pEuLabel:
		fr->label = value;
		break;
	case pEuPriority:
		fr->raise = value > 0;
		fr->lower = value < 0;
		break;
	case cEuMailbox:
		err = BoxSpecByName(&fr->transferSpec, string);
		break;
	case pEuCopy:
		fr->copyInstead = onOrOff;
		break;
	default:
		err = errAENoSuchObject;
		break;
	}

	return (err);
}

/**********************************************************************
 * SetTermPropertyLo - set a property for a filter term, without fetch & store
 **********************************************************************/
OSErr SetTermPropertyLo(FRPtr fr, short term, DescType property,
			AEDescPtr data)
{
	Str255 string;
	long value;
	OSErr err = noErr;

	value = GetAELong(data);
	GetAEPStr(string, data);

	/*
	 * change our local copy
	 */
	switch (property) {
	case pEuFilterHeader:
		PSCopy(fr->terms[term].header, string);
		break;
	case pEuFilterVerb:
		fr->terms[term].verb = value & 0xff;
		break;
	case pEuFilterValue:
		PSCopy(fr->terms[term].value, string);
		break;
	default:
		err = errAENoSuchObject;
		break;
	}

	return (err);
}

/************************************************************************
 * GetFilterProperty - get a property of a filter
 ************************************************************************/
OSErr GetFilterProperty(AEDescPtr token, AppleEvent * reply, long refCon)
{
#pragma unused(refCon);
	DescType property =
	    (*(PropTokenHandle) token->dataHandle)->propertyId;
	FilterToken fToken =
	    *(FilterTokenPtr) ((*token->dataHandle) + sizeof(PropToken));
	OSErr err = RegenerateFilters();
	FilterRecord fr;

	if (!err)
		if (!(err = FilterTokenToContent(&fToken, &fr))) {
			switch (property) {
			case pName:
				err =
				    AEPutPStr(reply, keyAEResult, fr.name);
				break;
			case formUniqueID:
				err =
				    AEPutLong(reply, keyAEResult,
					      fr.fu.id);
				break;
			case pEuFilterUse:
				err =
				    AEPutLong(reply, keyAEResult,
					      fr.fu.lastMatch);
				break;
			case pEuManual:
				err =
				    AEPutBool(reply, keyAEResult,
					      fr.manual);
				break;
			case pEuOutgoing:
				err =
				    AEPutBool(reply, keyAEResult,
					      fr.outgoing);
				break;
			case IN:
				err =
				    AEPutBool(reply, keyAEResult,
					      fr.incoming);
				break;
			case pEuConjunction:
				err =
				    AEPutEnum(reply, keyAEResult,
					      fr.conjunction | 'cj-\000');
				break;
			case pEuSubject:
				err =
				    AEPutPStr(reply, keyAEResult,
					      fr.subject);
				break;
			case pEuLabel:
				err =
				    AEPutLong(reply, keyAEResult,
					      fr.label);
				break;
			case pEuPriority:
				err =
				    AEPutLong(reply, keyAEResult,
					      fr.raise ? 1 : (fr.
							      lower ? -1 :
							      0));
				break;
			case cEuMailbox:
				err =
				    AEPutPStr(reply, keyAEResult,
					      fr.transferSpec.name);
				break;
			case pEuCopy:
				err =
				    AEPutBool(reply, keyAEResult,
					      fr.copyInstead);
				break;
			default:
				err = errAENoSuchObject;
				break;
			}
		}
	FiltersDecRef();
	return (err);
}

/************************************************************************
 * GetTermProperty - get a property of a term
 ************************************************************************/
OSErr GetTermProperty(AEDescPtr token, AppleEvent * reply, long refCon)
{
#pragma unused(refCon);
	DescType property =
	    (*(PropTokenHandle) token->dataHandle)->propertyId;
	TermToken tToken =
	    *(TermTokenPtr) ((*token->dataHandle) + sizeof(PropToken));
	OSErr err = RegenerateFilters();
	FilterRecord fr;

	if (!err)
		if (!(err = FilterTokenToContent(&tToken.filter, &fr))) {
			switch (property) {
			case pEuFilterHeader:
				err =
				    AEPutPStr(reply, keyAEResult,
					      fr.terms[tToken.term].
					      header);
				break;
			case pEuFilterValue:
				err =
				    AEPutPStr(reply, keyAEResult,
					      fr.terms[tToken.term].value);
				break;
			case pEuFilterVerb:
				err =
				    AEPutEnum(reply, keyAEResult,
					      fr.terms[tToken.term].
					      verb | 'vb-\000');
				break;
			default:
				err = errAENoSuchObject;
				break;
			}
		}
	FiltersDecRef();
	return (err);
}

/**********************************************************************
 * StoreTokenFilter - store a filter to a spot indicated by the token
 **********************************************************************/
OSErr StoreFilter(FilterTokenPtr fp, FRPtr fr)
{
	short filter;
	Point c;
	OSErr err = noErr;

	/*
	 * store it
	 */
	filter = FilterToken2Num(fp);
	FR[filter] = *fr;
	FilterNoteMatch(filter, fr->fu.lastMatch);

	/*
	 * is the filter window open?
	 */
	if (FLG && Win) {
		PushGWorld();
		SetPort(GetMyWindowCGrafPtr(Win));

		/*
		 * are we the selected filter?
		 */
		if (Selected - 1 == filter)
			DisplaySelectedFilter();

		/*
		 * Change our name
		 */
		c.h = 0;
		c.v = filter;
		LSetCell(fr->name + 1, *fr->name, c, LHand);

		/*
		 * mark the window as dirty, let the user save
		 */
		Win->isDirty = True;
		PopGWorld();
	} else {
		/*
		 * window not open.  Must save filters
		 */
		err = SaveFilters();
	}
	return (err);
}

/**********************************************************************
 * CountFilters - how many filters are there?
 **********************************************************************/
OSErr CountFilters(long *howMany)
{
	OSErr err = RegenerateFilters();

	if (!err)
		*howMany = NFilters;

	FiltersDecRef();
	return (err);
}

/**********************************************************************
 * FilterExists - does a filter exist?
 **********************************************************************/
Boolean FilterExists(DescType form, long selector)
{
	Boolean exists = False;
	if (RegenerateFilters())
		return (False);

	switch (form) {
	case formAbsolutePosition:
		exists = 0 < selector && selector <= NFilters;
		break;
	case formUniqueID:
		exists = FindFilterById(selector) >= 0;
		break;
	}
	FiltersDecRef();
	return (exists);
}



/************************************************************************
 * FiltersUpdate - draw the window
 ************************************************************************/
void FiltersUpdate(MyWindowPtr win)
{
#pragma unused(win)
	Rect r;
	Str63 s;
	Handle pattern = GetResource_('PAT ', OFFSET_GREY);

	PenPat(pattern ? (ConstPatternParam) * pattern : &qd.gray);
	HUnlock(pattern);
	r = Rects[flrMatch1];
	FrameRect(&r);
	r = Rects[flrMatch2];
	FrameRect(&r);

	PenNormal();


	if (!Selected || Multiple) {
		r = Win->contR;
		r.left = Rects[flrHBar].left - 1;
		GreyOutRoundRect(&r, 0, 0);
	} else if (!HasTwo) {
		r = Rects[flrMatch2];
		InsetRect(&r, 1, 1);
		GreyOutRoundRect(&r, 0, 0);
	}

	DrawFilterDate();
}



/**********************************************************************
 * AEDeleteFilter - delete a filter from AE's
 **********************************************************************/
OSErr AEDeleteFilter(FilterTokenPtr fp)
{
	OSErr err = RegenerateFilters();

	if (!err) {
		RemoveFilter(FilterToken2Num(fp));
		if (FLG && *FLG && Win)
			/* work done */ ;
		else
			err = SaveFilters();
	}
	FiltersDecRef();
	return (err);
}


#pragma segment Filters

#endif
#endif
