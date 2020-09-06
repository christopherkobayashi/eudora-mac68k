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

#define FILE_NUM 141

#ifdef VCARD

/* Copyright (c) 2000 by QUALCOMM Incorporated */

#define	isCR(aChar)				((aChar) == 13)
#define	isLF(aChar)				((aChar) == 10)
#define	isHTAB(aChar)			((aChar) == 9)
#define	isSPACE(aChar)		((aChar) == 32)
#define	isCOLON(aChar)		((aChar) == ':')
#define	isSEMI(aChar)			((aChar) == ';')
#define	isDASH(aChar)			((aChar) == '-')
#define	isCRLF(aChar)			(isCR (aChar) || isLF (aChar))
#define	isEQUAL(aChar)		((aChar) == '=')
#define	isEscape(aChar)		((aChar) == '\\')
	
typedef struct {
	Handle						data;						// Data containing the entire vCard
	Ptr								spot;						// Spot within the vCard where we're currently reading
	Ptr								end;						// Points to the final byte in the vCard
	VCardErrorType		error;					// Errors detected in the vCard source by the parser
	XDashStringHandle	xdash;					// Handle to all the vCard extensions the client knows about
	VCardItemRec			item;						// Information about the item currently being parsed
	VCardItemProc			itemProc;				// Callback for each parsed item in the vCard
	VCardErrorProc		errorProc;			// Callback for each parsing
	long							refcon;					// Private data owned by the caller, passed back during itemProcs
} VCardParserRec, *VCardParserPtr, **VCardParserHandle;


//
//	Static Prototypes
//

static OSErr					VCardAccuAddValue (AccuPtr a, Handle value, Boolean quotedPrintable);
static OSErr					VCardAccuAddValueQuotedPrintable (AccuPtr a, Handle value);
static OSErr					VCardAccuMaybeQuotedPrintableSoftline (AccuPtr a, Handle value, long *offset, long *count, long maxChar);
static Boolean				VCardValueContainsCRLFs (Handle value);
static OSErr					VCardAccuAddProperties (AccuPtr a, short count, short *vcProp);
static OSErr					VCardAccuAddPropertiesAndValue (AccuPtr a, short count, short *vcProp, Handle value, Boolean quotedPrintable);
static OSErr					VCardAccuAddValueList (AccuPtr a, Handle notes, short count, short *abTag, Boolean *anyValues);
static OSErr					VCardAccuAddAVPairStr (AccuPtr a, short attributeResIndex, PStr value);
static OSErr 					VCardAccuAddAVPairRes (AccuPtr a, short attributeResIndex, short valueResIndex);
static OSErr					VCardAccuAddAddress (AccuPtr a, Handle notes, Boolean home, Boolean preferred);
static OSErr					VCardAccuAddPhone (AccuPtr a, Handle notes, short propertyID, short propertyLocation, short tagID, Boolean preferred);
static OSErr					VCardAccuAddURL (AccuPtr a, Handle notes, short propertyLocation, short tagID, Boolean preferred);
static OSErr					VCardAccuAddOtherEmail (AccuPtr a, Handle notes);
static OSErr					VCardAccuAddOther (AccuPtr a, Handle notes, short propertyID, short tagID);
static OSErr					VCardAccuAddNotes (AccuPtr a, Handle notes);
static OSErr					VCardParserVCardFile (VCardParserPtr parserPtr);
static OSErr					VCardParserVCard (VCardParserPtr parserPtr);
static OSErr					VCardParserItems (VCardParserPtr parserPtr);
static OSErr					VCardParserItem (VCardParserPtr parserPtr);
static OSErr					VCardParserGroups (VCardParserPtr parserPtr, VCardItemPtr itemPtr, Str255 keyword);
static OSErr					VCardParserParameters (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static void						VCardParserParameterTypeValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr);
static void						VCardParserParameterValueValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr);
static void						VCardParserParameterEncodingValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr);
static void						VCardParserParameterCharsetValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr);
static void						VCardParserParameterLangaugeValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr);
static void						VCardParserProcessOrphanedEncoding (VCardItemPtr itemPtr, vcKeywordType encodingValue);
static void						VCardParserEqualSign (VCardParserPtr parserPtr);
static OSErr					VCardParserValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static OSErr					VCardParserAddress (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static OSErr					VCardParserOrganization (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static OSErr					VCardParserName (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static OSErr					VCardParserPropertPartList (VCardParserPtr parserPtr, VCardItemPtr itemPtr, vcKeywordType *partList, short parts);
static OSErr					VCardParser7BitValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr, Boolean valueList, Boolean quotedPrintable);
static OSErr					VCardParser8BitValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static OSErr					VCardParserQuotedPrintableValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr, char delimiter);
static OSErr					VCardParserBase64Value (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static void						VCardParserSkipToNextLine (VCardParserPtr parserPtr);
static Boolean				VCardParserWS (VCardParserPtr parserPtr, Boolean optional);
static Boolean				VCardParserWSLS (VCardParserPtr parserPtr, Boolean optional);
static OSErr					VCardParserStrnosemi (VCardParserPtr parserPtr, VCardItemPtr itemPtr);
static PStr						VCardParserKeyword (VCardParserPtr parserPtr, PStr keyword);
static OSErr					VCardParserWord (VCardParserPtr parserPtr, Handle value);
static Boolean				VCardParserCharacter (VCardParserPtr parserPtr, char c, Boolean optional);
static Boolean				VCardParseCRLFs (VCardParserPtr parserPtr, Boolean optional);
static OSErr					VCardAppendKeywordToGroup (VCardItemPtr itemPtr, Str255 keyword);
static OSErr					VCardParserAppendPropertyValueString (VCardParserPtr parserPtr, VCardParamPtr paramPtr, Handle strings);
static vcKeywordType	VCardProcessProperty (VCardParserPtr parserPtr, Str255 keyword);
static short					FindXDashString (XDashStringHandle xdash, PStr keyword);


Boolean IsVCardAvailable (void)

{
	return (HasFeature (featureVCard) && PrefIsSet (PREF_VCARD));
}



//
//	MakeVCard
//
//		Accepts standard nickname address and notes handle and returns a handle
//		containing the text of a vCard.  For now, our email expansion will be
//		mapped in full to vcard's single address field.  Dunno if this will
//		actually be acceptable to other vcard savvy apps... we'll see...
//
//		For now, we're just going to built vCards by brute force, but it would be nice
//		if we could build a more portable means of building vCards based on templates.
//
//		We are also ingnoring several Eudora nickname fields when exporting as a vCard:
//				� Other Email
//				� Other Phone
//				� Other Web Pages
//				� Notes (though we'll probably add this later)
//
//		We also need to encoded fields with values containing 
//

#define	kVCardLineMaxValues		10

Handle MakeVCard (Handle addresses, Handle notes)

{
	Accumulator					vCard;
	PrimaryLocationType	preferred;
	Str255							scratch;
	OSErr								theError;
	long								oldOffset;
	short								abTag[kVCardLineMaxValues],				// More than enough room for now
											vcProp[kVCardLineMaxValues],			// More than enough room for now
											count;
	Boolean							anyValues,
											quotedPrintable;
	
	if (!IsVCardAvailable ())
		return (nil);
		
	UseFeature (featureVCard);
	
	preferred = GetPrimaryLocation (notes);
	
	// Initialize an accumulator for the vCard data
	theError = AccuInit (&vCard);
	
	// BEGIN:VCARD
	theError = VCardAccuAddAVPairRes (&vCard, vcBegin, vcVCard);
	
	// VERSION:2.1
	if (!theError)
		theError = VCardAccuAddAVPairStr (&vCard, vcVersion, "\p2.1");

	// Add the body of the vCard
	
	// Full name
	oldOffset = vCard.offset;
	if (!theError)
		theError = AccuAddStr (&vCard, GetRString (scratch, VCardKeywordStrn + vcFN));
	if (!theError) {
		abTag[0] = abTagName;
		theError = VCardAccuAddValueList (&vCard, notes, 1, abTag, &anyValues);
	}
	if (!anyValues)
		vCard.offset = oldOffset;

	// Name
	oldOffset = vCard.offset;
	if (!theError)
		theError = AccuAddStr (&vCard, GetRString (scratch, VCardKeywordStrn + vcN));
	if (!theError) {
		abTag[0] = abTagLast;
		abTag[1] = abTagFirst;
		abTag[2] = abTag[3] = abTag[4] = 0;
		theError = VCardAccuAddValueList (&vCard, notes, 5, abTag, &anyValues);
	}
	if (!anyValues)
		vCard.offset = oldOffset;

	// Email
	if (!theError && addresses) {
		quotedPrintable = VCardValueContainsCRLFs (addresses);
		vcProp[0] = vcEmail;
		vcProp[1] = vcInternet;
		vcProp[2] = vcPref;
		count = 3;
		if (quotedPrintable = VCardValueContainsCRLFs (addresses))
			vcProp[count++] = vcQuotedPrintable;

		theError = VCardAccuAddPropertiesAndValue (&vCard, count, vcProp, addresses, quotedPrintable);
	}
	
	// Home Address
	if (!theError)
		theError = VCardAccuAddAddress (&vCard, notes, true, preferred == homePrimary);
	
	// Work Address
	if (!theError)
		theError = VCardAccuAddAddress (&vCard, notes, false, preferred == workPrimary);
	
	// Home voice phone
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcVoice, vcHome, abTagPhone, preferred == homePrimary);
	
	// Home fax
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcFax, vcHome, abTagFax, preferred == homePrimary);
	
	// Home cell phone
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcCell, vcHome, abTagMobile, preferred == homePrimary);
	
	// Work voice phone
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcVoice, vcWork, abTagPhone2, preferred == workPrimary);
	
	// Work fax
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcFax, vcWork, abTagFax2, preferred == workPrimary);
	
	// Work cell phone
	if (!theError)
		theError = VCardAccuAddPhone (&vCard, notes, vcCell, vcWork, abTagMobile2, preferred == workPrimary);
	
	// Title
	oldOffset = vCard.offset;
	if (!theError)
		theError = AccuAddStr (&vCard, GetRString (scratch, VCardKeywordStrn + vcTitle));
	if (!theError) {
		abTag[0] = abTagTitle;
		theError = VCardAccuAddValueList (&vCard, notes, 1, abTag, &anyValues);
	}
	if (!anyValues)
		vCard.offset = oldOffset;
	
	// Company
	oldOffset = vCard.offset;
	if (!theError)
		theError = AccuAddStr (&vCard, GetRString (scratch, VCardKeywordStrn + vcOrg));
	if (!theError) {
		abTag[0] = abTagCompany;
		abTag[1] = 0;
		theError = VCardAccuAddValueList (&vCard, notes, 2, abTag, &anyValues);
	}
	if (!anyValues)
		vCard.offset = oldOffset;

	// Home URL	
	if (!theError)
		theError = VCardAccuAddURL (&vCard, notes, vcHome, abTagWeb, preferred == homePrimary);

	// Work URL	
	if (!theError)
		theError = VCardAccuAddURL (&vCard, notes, vcWork, abTagWeb2, preferred == workPrimary);

	// Other Email
	if (!theError)
		theError = VCardAccuAddOtherEmail (&vCard, notes);
		
	// Other Phone
	if (!theError)
		theError = VCardAccuAddOther (&vCard, notes, vcTel, abTagOtherPhone);
	
	// Other URL
	if (!theError)
		theError = VCardAccuAddOther (&vCard, notes, vcURL, abTagOtherWeb);

	// Notes
	if (!theError)
		theError = VCardAccuAddNotes (&vCard, notes);
	
	// END:VCARD
	if (!theError)
		theError = VCardAccuAddAVPairRes (&vCard, vcEnd, vcVCard);
	
	if (!theError)
		AccuTrim (&vCard);
	else
		AccuZap (vCard);
	return (vCard.data);
}


static OSErr VCardAccuAddValue (AccuPtr a, Handle value, Boolean quotedPrintable)

{
	Ptr		spot,
				end;
	OSErr	theError;
	long	offset;
	
	theError = noErr;
	if (value) {
		if (quotedPrintable)
			return (VCardAccuAddValueQuotedPrintable (a, value));
			
		offset	 = 0;
		spot		 = LDRef (value);
		end			 = spot + GetHandleSize (value);
		
		while (spot < end && !theError) {
			if (isSEMI (*spot)) {
				theError = AccuAddFromHandle (a, value, offset, spot - *value - offset);
				if (!theError)
					theError = AccuAddChar (a, '\\');
				offset = spot - *value;
			}
			++spot;
		}
		if (!theError)
			theError = AccuAddFromHandle (a, value, offset, end - *value - offset);
		UL (value);
	}
	return (theError);
}


static OSErr VCardAccuAddValueQuotedPrintable (AccuPtr a, Handle value)

{
	Ptr		spot,
				end;
	OSErr	theError;
	long	offset,
				count;
	
	theError = noErr;
	offset	 = 0;
	count		 = 0;
	spot		 = LDRef (value);
	end			 = spot + GetHandleSize (value);
	
	while (spot < end && !theError) {
		theError = VCardAccuMaybeQuotedPrintableSoftline (a, value, &offset, &count, 74);				// Room for "="
		if (!theError)
			switch (*spot) {
				case ';':
					theError = VCardAccuMaybeQuotedPrintableSoftline (a, value, &offset, &count, 72);	// Room for "\;="
					if (!theError)
						theError = AccuAddFromHandle (a, value, offset, spot - *value - offset);
					if (!theError)
						theError = AccuAddChar (a, '\\');
					offset = spot - *value;
					++count;
					break;
				case '\r':
				case '\n':
					theError = VCardAccuMaybeQuotedPrintableSoftline (a, value, &offset, &count, 68);	// Room for "=0D=0A="
					if (!theError)
						theError = AccuAddFromHandle (a, value, offset, spot - *value - offset);
					if (!theError)
						theError = AccuAddStr (a, "\p=0D=0A");
					if (!theError && spot + 1 < end)
						theError = AccuAddStr (a, "\p=\r\n");
					offset = spot - *value + 1;
					count = 0;
					break;
			}
		++spot;
		++count;
	}
	if (!theError)
		theError = AccuAddFromHandle (a, value, offset, end - *value - offset);
	UL (value);
	return (theError);
}


static OSErr VCardAccuMaybeQuotedPrintableSoftline (AccuPtr a, Handle value, long *offset, long *count, long maxChar)

{
	OSErr	theError;
	
	theError = noErr;
	if (*count >= maxChar) {
		theError = AccuAddFromHandle (a, value, *offset, maxChar);
		if (!theError)
			theError = AccuAddStr (a, "\p=\r\n");
		*offset += maxChar;
		*count = 0;
	}
	return (theError);
}


static Boolean VCardValueContainsCRLFs (Handle value)

{
	Ptr 	spot;
	Size	length;
	
	if (value) {
		length = GetHandleSize (value);
		for (spot = *value; spot < *value + length; spot++)
			if (*spot == '\r' || *spot == '\n')
				return (true);
	}
	return (false);
}


static OSErr VCardAccuAddProperties (AccuPtr a, short count, short *vcProp)

{
	Str255	property;
	OSErr		theError;
	short		i;

	theError = noErr;
	for (i = 0; i < count && !theError; ++i) {
		theError = AccuAddStr (a, GetRString (property, VCardKeywordStrn + vcProp[i]));
		if (!theError && i + 1 < count)
			theError = AccuAddChar (a, ';');
	}
	return (theError);
}

static OSErr VCardAccuAddPropertiesAndValue (AccuPtr a, short count, short *vcProp, Handle value, Boolean quotedPrintable)

{
	OSErr	theError;
	
	if (value) {
		theError = VCardAccuAddProperties (a, count, vcProp);
		if (!theError)
			theError = AccuAddChar (a, ':');
		if (!theError)
			theError = VCardAccuAddValue (a, value, quotedPrintable);
		if (!theError)
			theError = AccuAddStr (a, "\p\r\n");
	}
	return (theError);
}


//
//	VCardAccuAddValueList
//
//		Add a value list to the vCard data accumulator.
//

static OSErr VCardAccuAddValueList (AccuPtr a, Handle notes, short count, short *abTag, Boolean *anyValues)

{
	Str255	property,
					tag;
	Handle	**values,
					*valuePtr;
	OSErr		theError;
	short		i;
	Boolean	weHaveValue,
					quotedPrintable;

	weHaveValue			= false;
	quotedPrintable	= false;
	
	// Allocate space for one Handle per requested value
	values = NuHandleClear (count * sizeof (Handle));
	theError = MemError ();
	
	// Get the values
	valuePtr = LDRef (values);
	for (i = 0; i < count && !theError; ++i, ++valuePtr) {
		if (abTag[i])
			if (*valuePtr = GetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + abTag[i]))) {
				weHaveValue = true;
				if (!quotedPrintable)
					quotedPrintable = VCardValueContainsCRLFs (*valuePtr);
			}
		theError = MemError ();
	}
	
	// Add the formatted values to the accumulator (values still locked)
	if (weHaveValue) {
		if (quotedPrintable) {
			theError = AccuAddChar (a, ';');
			if (!theError)
				theError = AccuAddStr (a, GetRString (property, VCardKeywordStrn + vcQuotedPrintable));
		}
		if (!theError)
			theError = AccuAddChar (a, ':');
		valuePtr = *values;
		for (i = 0; i < count && !theError; ++i, ++valuePtr) {
			theError = VCardAccuAddValue (a, *valuePtr, quotedPrintable);
			if (!theError && i + 1 < count)
				theError = AccuAddChar (a, ';');
		}
		if (!theError)
			theError = AccuAddStr (a, "\p\r\n");
	}

	// Dispose of the memory we allocated
	if (values) {
		valuePtr = *values;
		for (i = 0; i < count; ++i)
			ZapHandle (*valuePtr);
		ZapHandle (values);
	}
	
	if (anyValues)
		*anyValues = weHaveValue;
	return (theError);
}


static OSErr VCardAccuAddAVPairStr (AccuPtr a, short attributeResIndex, PStr value)

{
	Str255	scratch;
	OSErr		theError;
	
	theError = AccuAddStr (a, GetRString (scratch, VCardKeywordStrn + attributeResIndex));
	if (!theError)
		theError = AccuAddChar (a, ':');
	if (!theError)
		theError = AccuAddStr (a, value);
	if (!theError)
		theError = AccuAddStr (a, "\p\r\n");
	return (theError);
}


static OSErr VCardAccuAddAVPairRes (AccuPtr a, short attributeResIndex, short valueResIndex)

{
	Str255	scratch;
	
	return (VCardAccuAddAVPairStr (a, attributeResIndex, GetRString (scratch, VCardKeywordStrn + valueResIndex)));
}


static OSErr VCardAccuAddAddress (AccuPtr a, Handle notes, Boolean home, Boolean preferred)

{
	OSErr		theError;
	long		oldOffset;
	short		abTag[kVCardLineMaxValues],				// More than enough room for now
					vcProp[kVCardLineMaxValues],			// More than enough room for now
					count;
	Boolean	anyValues;
		
	// Home Address
	oldOffset = a->offset;
	vcProp[0] = vcAdr;
	vcProp[1] = home ? vcHome : vcWork;
	count = 2;
	if (preferred)
		vcProp[count++] = vcPref;
	theError = VCardAccuAddProperties (a, count, vcProp);
	if (!theError) {
		abTag[0] = abTag[1] = 0;
		if (home) {
			abTag[2] = abTagAddress;
			abTag[3] = abTagCity;
			abTag[4] = abTagState;
			abTag[5] = abTagZip;
			abTag[6] = abTagCountry;
		}
		else {
			abTag[2] = abTagAddress2;
			abTag[3] = abTagCity2;
			abTag[4] = abTagState2;
			abTag[5] = abTagZip2;
			abTag[6] = abTagCountry2;
		}
		theError = VCardAccuAddValueList (a, notes, 7, abTag, &anyValues);
	}
	if (!anyValues)
		a->offset = oldOffset;
	return (theError);
}

static OSErr VCardAccuAddPhone (AccuPtr a, Handle notes, short propertyID, short propertyLocation, short tagID, Boolean preferred)

{
	Str255	tag;
	Handle	value;
	OSErr		theError;
	short		vcProp[kVCardLineMaxValues],			// More than enough room for now
					count;
	Boolean	quotedPrintable;
	
	theError = noErr;
	if (value = GetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + tagID))) {
		quotedPrintable = VCardValueContainsCRLFs (value);
		vcProp[0] = vcTel;
		vcProp[1] = propertyID;
		vcProp[2] = propertyLocation;
		count = 3;
		if (preferred)
			vcProp[count++] = vcPref;
		if (quotedPrintable)
			vcProp[count++] = vcQuotedPrintable;
		theError = VCardAccuAddPropertiesAndValue (a, count, vcProp, value, quotedPrintable);
		ZapHandle (value);
	}
	return (theError);
}

static OSErr VCardAccuAddURL (AccuPtr a, Handle notes, short propertyLocation, short tagID, Boolean preferred)

{
	Str255	tag;
	Handle	value;
	OSErr		theError;
	short		vcProp[kVCardLineMaxValues],			// More than enough room for now
					count;
	Boolean	quotedPrintable;
	
	theError = noErr;
	if (value = GetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + tagID))) {
		quotedPrintable = VCardValueContainsCRLFs (value);
		vcProp[0] = vcURL;
		vcProp[1] = propertyLocation;
		count = 2;
		if (preferred)
			vcProp[count++] = vcPref;
		if (quotedPrintable)
			vcProp[count++] = vcQuotedPrintable;
		theError = VCardAccuAddPropertiesAndValue (a, count, vcProp, value, quotedPrintable);
		ZapHandle (value);
	}
	return (theError);
}


static OSErr VCardAccuAddOtherEmail (AccuPtr a, Handle notes)

{
	BinAddrHandle addresses;
	Str255				scratch;
	Handle				value;
	Ptr						addrPtr;
	OSErr					theError;

	theError = noErr;
	if (value = GetTaggedFieldValueInNotes (notes, GetRString (scratch, ABReservedTagsStrn + abTagOtherEmail))) {
		if (!SuckAddresses (&addresses, value, false, true, false, nil)) {
			// Create an 'EMAIL' line for each address we suck from the other email field
			for (addrPtr = LDRef (addresses); !theError && *addrPtr; addrPtr += *addrPtr + 2)
				theError = VCardAccuAddAVPairStr (a, vcEmail, addrPtr);
		}
		ZapHandle (addresses);
		ZapHandle (value);
	}
	return (theError);
}


static OSErr VCardAccuAddOther (AccuPtr a, Handle notes, short propertyID, short tagID)

{
	Str255	tag;
	Handle	value;
	OSErr		theError;
	short		vcProp[kVCardLineMaxValues],			// More than enough room for now
					count;
	Boolean	quotedPrintable;
	
	theError = noErr;
	if (value = GetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + tagID))) {
		quotedPrintable = VCardValueContainsCRLFs (value);
		vcProp[0] = propertyID;
		count = 1;
		if (quotedPrintable)
			vcProp[count++] = vcQuotedPrintable;
		theError = VCardAccuAddPropertiesAndValue (a, count, vcProp, value, quotedPrintable);
		ZapHandle (value);
	}
	return (theError);
}


static OSErr VCardAccuAddNotes (AccuPtr a, Handle notes)

{
	AttributeValueHandle	avPairs;
	Str255								tag;
	Handle								value,
												leftovers;
	OSErr									theError;
	short									vcProp[kVCardLineMaxValues],			// More than enough room for now
												count;
	Boolean								quotedPrintable;
	
	theError	= noErr;
	leftovers = nil;
	value = GetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + abTagNote));
	avPairs = ParseAllAttributeValuePairs (notes, &leftovers, avAllButHiddenPairs, avPairUnknown);
	if (leftovers) {
		if (!value) {
			value = NuHandle (0);
			theError = MemError ();
		}
		else
			theError = PtrPlusHand ("\r\n", value, 2);
		if (!theError)
			theError = HandPlusHand (leftovers, value);
	}
	if (value && !theError) {
		quotedPrintable = VCardValueContainsCRLFs (value);
		vcProp[0] = vcNote;
		count = 1;
		if (quotedPrintable)
			vcProp[count++] = vcQuotedPrintable;
		theError = VCardAccuAddPropertiesAndValue (a, count, vcProp, value, quotedPrintable);
	}
	ZapHandle (value);
	ZapHandle (leftovers);
	ZapHandle (avPairs);
	return (theError);
}


//
//	ParseVCard
//
//		Accepts the text of a vCard and attempts to create a Eudora nickname, returning (hopefully)
//		a suggested nickname, addresses and notes.
//
//		If data remains after we've sucked out a vCard, we'll put the offset to that data in offset,
//		and they can call us again to suck out the rest
//

OSErr ParseVCard (Handle data, uLong *offset, XDashStringHandle xDashStrings, VCardItemProc itemProc, VCardErrorProc errorProc, long refcon)
{
	VCardParserRec	parser;
	OSErr					theError;

	if (!IsVCardAvailable ())
		return (noErr);
		
	UseFeature (featureVCard);
	

	// My hero, zero
	Zero (parser);

	parser.data			 = data;
	parser.spot			 = LDRef (data) + *offset;
	parser.end			 = *data + GetHandleSize (data) - 1;
	parser.error		 = vcErrorNone;
	parser.xdash		 = xDashStrings;
	parser.itemProc	 = itemProc;
	parser.errorProc = errorProc;
	parser.refcon		 = refcon;
	theError = VCardParserVCardFile (&parser);		// ...and, they're off!

	if (parser.errorProc && parser.error)
		(*parser.errorProc) (data, &parser.item, refcon, parser.error, parser.spot - *data);
	
	if (parser.spot>=parser.end) *offset = 0;
	else *offset = parser.spot - *data;

	UL (data);
	
	return (theError);
}


//
//	VCardParserVCardFile
//
//		vcardfile = [wsls] vcard [wsls]
//

static OSErr VCardParserVCardFile (VCardParserPtr parserPtr)

{
	OSErr	theError;
	
	// Check for optional whitespace and linefeeds
	VCardParserWSLS (parserPtr, true);
	
	// Process vcard data
	theError = VCardParserVCard (parserPtr);
	
	// Check for more optional whitespace and linefeeds
	if (!theError && parserPtr->error == vcErrorNone)
		VCardParserWSLS (parserPtr, true);

	return (theError);
}


//
//	VCardParserVCard
//
//		vcard = "BEGIN:VCARD" [ws 7bit] 1*CRLF items *CRLF "END:VCARD"
//

static OSErr VCardParserVCard (VCardParserPtr parserPtr)

{
	Str255	keyword,
					scratch;
	OSErr		theError;
	
	// Initialize the handles we'll be needing
	parserPtr->item.value		 = NuHandle (0);
	parserPtr->item.group		 = NuHandle (0);
	parserPtr->item.params	 = NuHandle (0);
	parserPtr->item.strings	 = NuHandle (0);
	theError = MemError ();

	// Check for "BEGIN:VCARD"
	if (!StringSame (VCardParserKeyword (parserPtr, keyword), GetRString (scratch, VCardKeywordStrn + vcBegin)))
		parserPtr->error = vcMissingBegin;
	if (parserPtr->error == vcErrorNone)
		if (VCardParserCharacter (parserPtr, ':', false))
			if (!StringSame (VCardParserKeyword (parserPtr, keyword), GetRString (scratch, VCardKeywordStrn + vcVCard)))
				parserPtr->error = vcMissingVcard;

	// Check for the optional [ws 7bit] portion
	if (!theError && parserPtr->error == vcErrorNone)
		if (VCardParserWS (parserPtr, true))
			theError = VCardParser7BitValue (parserPtr, nil, false, false);
	
	// Skip over 1 or more CRLF's
	if (!theError && parserPtr->error == vcErrorNone)
		VCardParseCRLFs (parserPtr, false);
		
	// Process the 'items' in the vCard
	if (!theError && parserPtr->error == vcErrorNone)
		theError = VCardParserItems (parserPtr);
		
	// Check for "END:VCARD"	(actually, check for ":VCARD" since we already parsed the keyword as an item)
	if (!theError) {
		if (parserPtr->error == vcErrorNone)
			if (parserPtr->item.property != vcKeyEnd)
				parserPtr->error = vcMissingEnd;
		if (parserPtr->error == vcErrorNone)
			if (VCardParserCharacter (parserPtr, ':', false))
				if (!StringSame (VCardParserKeyword (parserPtr, keyword), GetRString (scratch, VCardKeywordStrn + vcVCard)))
					parserPtr->error = vcMissingVcard;
	}
	
	// Release our memory
	ZapHandle (parserPtr->item.value);
	ZapHandle (parserPtr->item.group);
	ZapHandle (parserPtr->item.params);
	ZapHandle (parserPtr->item.strings);

	return (theError);
}


//
//	VCardParserItems
//
//		items = items *CRLF item
//					/ item
//
//		This effectively means 1 or more items.  We'll stop once we've
//		parsed "END" as the item property.
//

static OSErr VCardParserItems (VCardParserPtr parserPtr)

{
	OSErr	theError;
	short	numItems;
	
	theError = noErr;
	numItems = 0;
	do {
		theError = VCardParserItem (parserPtr);

		// Error handling
		if (parserPtr->error) {
			// Callback to the client and quit if the client found the error Super Serious
		 	if (parserPtr->errorProc)
				if ((*parserPtr->errorProc) (parserPtr->data, &parserPtr->item, parserPtr->refcon, parserPtr->error, parserPtr->spot - *parserPtr->data))
					break;

			// Clear the error for the next item and skip to the next line in the data
			parserPtr->error = vcErrorNone;
		}
		else
			if (!theError)
				++numItems;

	} while (!theError && parserPtr->item.property != vcKeyEnd && parserPtr->spot <= parserPtr->end);
	
	// Odd case, but we need it
	if (numItems && parserPtr->error == vcErrorExpectingItem)
		parserPtr->error = vcErrorNone;

	return (theError);
}


//
//	VCardParserItem
//
//		item  = [ws] [groups "."] name		[params] ":" value				CRLF
//					/ [ws] [groups "."] "ADR"		[params] ":" addressparts CRLF
//					/ [ws] [groups "."] "ORG"		[params] ":" orgparts			CRLF
//					/ [ws] [groups "."] "N"			[params] ":" nameparts		CRLF
//					/ [ws] [groups "."] "AGENT"	[params] ":" vcard				CRLF
//
//		In parsing the item information we're going to make a couple of assumptions about
//		what constitutes an "item".  As we parse the text of a line, we might be looking
//		at a group, or a name, or a known type.  Each case shares a certain format symetry.
//
//						text [paramList] delimiter value CRLF
//
//		Notice that a 'group' is nothing more than a series of word separated by periods --
//		and notice that 'params' is a list of parameters, each preceded by a semicolon.
//		Therefore, we can safely (ha!) assume that we can determine the meaning of the text
//		based on the type of delimiter we find.
//
//		As we successfully parse the line items of a vCard, we build a VCardItemRec
//		which gets passed back to the client in a callback.  The client can then
//		pick values, fields, or whatever from this structure in neat, tidy keywords.
//

static OSErr VCardParserItem (VCardParserPtr parserPtr)

{
	Str255	keyword;
	OSErr		theError;
	
	// Initial values for the item record
	parserPtr->item.property = vcKeyNone;
	parserPtr->item.encoding = vcValue7Bit;
	parserPtr->item.charset	 = vcValueUSAscii;
	parserPtr->item.propertyFlags = vcPropFlagNone;
	SetHandleBig (parserPtr->item.value, 0);
	SetHandleBig (parserPtr->item.group, 0);
	SetHandleBig (parserPtr->item.params, 0);
	SetHandleBig (parserPtr->item.strings, 0);
	
	// Check for optional whitespace
	VCardParserWS (parserPtr, true);
	
	// Check for the presence of grouping information
	theError = VCardParserGroups (parserPtr, &parserPtr->item, keyword);
	
	// At this point, 'keyword' contains primary property (the last parsed keyword prior to the delimiter)
	if (!theError && !parserPtr->error)
		parserPtr->item.property = VCardProcessProperty (parserPtr, keyword);
	
	// We're done if we've found the "END" property (and it's not some stupidly ill-formed group syntax)
	if (!theError && !parserPtr->error && parserPtr->item.property == vcKeyEnd)
		return (theError);

	// Do we have a parameter list to parse?
	if (!theError && !parserPtr->error && VCardParserCharacter (parserPtr, ';', true))
		theError = VCardParserParameters (parserPtr, &parserPtr->item);

	// Do we have a value to parse?
	if (!theError && !parserPtr->error && VCardParserCharacter (parserPtr, ':', false))
		theError = VCardParserValue (parserPtr, &parserPtr->item);
	
	// No errors?  Let the client process the results of this item
	if (!theError && parserPtr->item.property != vcKeyNone && parserPtr->itemProc)
		theError = (*parserPtr->itemProc) (&parserPtr->item, parserPtr->refcon);

	// Skip over exactly 1 CRLF or to the beginning of the next line if an error was detected
	if (!theError && parserPtr->error == vcErrorNone)
		VCardParseCRLFs (parserPtr, false);
	else
		VCardParserSkipToNextLine (parserPtr);
	return (theError);
}


//
//	VCardParserGroups
//
//		groups = groups "." word
//					 / word
//
//		Loop looking for keywords that precede a "." and append these to the item's group handle.
//		The function concludes when there's no more text to read, or when we've hit a delimiter
//		that is NOT a period.
//
//		Upon leaving the function, 'spot' points to the terminating delimiter (so it should be
//		either a ':' or a ';' if the vCard is valid -- and if we cared since right now we're not
//		exactly a validating parser.
//

static OSErr VCardParserGroups (VCardParserPtr parserPtr, VCardItemPtr itemPtr, Str255 keyword)

{
	OSErr		theError;
	Boolean	done;
	
	theError = noErr;
	
	done = false;
	while (parserPtr->spot <= parserPtr->end && !done && !theError && !parserPtr->error) {
		// Grab a keyword
		VCardParserKeyword (parserPtr, keyword);

		// If the delimiter is a '.' then the preceding keyword was part of a grouping... append it!
		if (*parserPtr->spot == '.')
			theError = VCardAppendKeywordToGroup (itemPtr, keyword);
		done = !VCardParserCharacter (parserPtr, '.', true);
	}
	return (theError);
}


//
//	VCardParserParameters
//
//		paramlist = paramlist [ws] ";" [ws] param
//							/ param
//
//		This effectively means 1 param, or a sequence of params separated by semicolons.
//
//		param =		"TYPE"			[ws] "=" [ws] ptypeval
//					/		"VALUE"			[ws] "=" [ws] pvalueval
//					/		"ENCODING"	[ws] "=" [ws] pencodingval
//					/		"CHARSET"		[ws] "=" [ws] charsetval
//					/		"LANGUAGE"	[ws] "=" [ws] langval
//					/		"X-" word		[ws] "=" [ws] word
//					/		knowntype
//
//		Upon entry, 'spot' should be pointing to the first character in the parameter list.
//		On exit, 'spot' should point to the first character following the parameter list.
//

static OSErr VCardParserParameters (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	VCardParamRec	param;
	Str255				keyword;
	OSErr					theError;
	Boolean				done;
	
	theError = noErr;
	SetHandleBig (itemPtr->strings, 0);

	// Loop-ity-loop until we are done.
	done = false;
	while (parserPtr->spot <= parserPtr->end && !theError && !done && !parserPtr->error) {
		// My hero, zero
		Zero (param);

		// Parse a property as a keyword and examine it closely
		switch (param.pProperty = VCardProcessProperty (parserPtr, VCardParserKeyword (parserPtr, keyword))) {
			case vcKeyNone:
				parserPtr->error = vcErrorUnknownParamter;
				done = true;
				break;
			case vcKeyType:
				VCardParserEqualSign (parserPtr);
				if (parserPtr->error == vcErrorNone)
					VCardParserParameterTypeValue (parserPtr, &param);
				break;
			case vcKeyValue:
				VCardParserEqualSign (parserPtr);
				if (parserPtr->error == vcErrorNone)
					VCardParserParameterValueValue (parserPtr, &param);
				break;
			case vcKeyEncoding:
				VCardParserEqualSign (parserPtr);
				if (parserPtr->error == vcErrorNone)
					VCardParserParameterEncodingValue (parserPtr, &param);
				break;
			case vcKeyCharset:
				VCardParserEqualSign (parserPtr);
				if (parserPtr->error == vcErrorNone)
					VCardParserParameterCharsetValue (parserPtr, &param);
				break;
			case vcKeyLanguage:
				VCardParserEqualSign (parserPtr);
				if (parserPtr->error == vcErrorNone)
					VCardParserParameterLangaugeValue (parserPtr, &param);
				break;
			case vcKey7Bit:
			case vcKey8Bit:
			case vcKeyQuotedPrintable:
			case vcKeyBase64:
				// Annoyingly, we have to do this for vCards that don't follow the spec.
				VCardParserProcessOrphanedEncoding (itemPtr, param.pProperty);
				break;
			case vcKeyHome:
				itemPtr->propertyFlags |= vcPropFlagHome;
				break;
			case vcKeyWork:
				itemPtr->propertyFlags |= vcPropFlagWork;
				break;
			case vcKeyPref:
				itemPtr->propertyFlags |= vcPropFlagPreferred;
				break;
			default:
				// Was an "X-" found as the parameter property?
				if (param.pProperty < 0) {
					VCardParserEqualSign (parserPtr);
					if (parserPtr->error == vcErrorNone)
						theError = VCardParserAppendPropertyValueString (parserPtr, &param, itemPtr->strings);
				}
				// else the pProperty was a knowntype
				break;
		}
		
		// Append the parameter record onto our Big Block of Parameters
		if (!theError && !parserPtr->error)
			theError = PtrPlusHand (&param, itemPtr->params, sizeof (param));
		
		done = !VCardParserCharacter (parserPtr, ';', true);
	}
	
	// Done looping... we SHOULD be pointing at a colon (which means a value is upcoming -- oooo! ahhh!)
	return (theError);
}


//
//	VCardParserParameterTypeValue
//
//		ptypeval = knowntype
//						 / "X-" word
//

static void VCardParserParameterTypeValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr)

{
	Str255	keyword;
	
	paramPtr->pValue = VCardProcessProperty (parserPtr, VCardParserKeyword (parserPtr, keyword));
}


//
//	VCardParserParameterValueValue
//
//		pvalueval = "INLINE"
//							/ "URL"
//							/ "CONTENT-ID"
//							/ "X-" word
//
//		Functionality is the same as above for now, but these are broken out into separate
//		functions just in case we want the parser to do some keyword validation down the road.
//

static void VCardParserParameterValueValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr)

{
	Str255	keyword;
	
	paramPtr->pValue = VCardProcessProperty (parserPtr, VCardParserKeyword (parserPtr, keyword));
}


//
//	VCardParserParameterEncodingValue
//
//		pencodingval = "7BIT"
//								 / "8BIT"
//								 / "QUOTED-PRINTABLE"
//								 / "BASE64"
//								 / "X-" word
//

static void VCardParserParameterEncodingValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr)

{
	Str255	keyword;
	
	VCardParserProcessOrphanedEncoding (&parserPtr->item, paramPtr->pValue = VCardProcessProperty (parserPtr, VCardParserKeyword (parserPtr, keyword)));
}


//
//	VCardParserParameterCharsetValue
//
//		charsetval = <a character set string as defined in section 7.1 of RFC 1521>
//
//		We'll parse a keyword and see if it matches a known charset value.
//

static void VCardParserParameterCharsetValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr)

{
	Str255	keyword,
					scratch;
	short		value,
					i;
	
	VCardParserKeyword (parserPtr, keyword);

	// US-ASCII?
	if (StringSame (VCardParserKeyword (parserPtr, keyword), GetRString (scratch, VCardKeywordStrn + vcVCard)))
		parserPtr->item.charset = vcValueUSAscii;
	else
		// How about ISO-8859?
		if (BeginsWith (keyword, GetRString (scratch, VCardKeywordStrn + vcISO_8859))) {
			value = 0;
			for (i = scratch[0] + 1; i <= keyword[0]; ++i)
				value = value * 10 + (keyword[i] - '0');
			parserPtr->item.charset = vcValueISO_8859_1 + value - 1;
		}
		else
			parserPtr->item.charset = vcValueOtherCharset;
}


//
//	VCardParserParameterLangaugeValue
//
//		langval = <a language string as defined in RFC 1766>
//						= 1*8ALPHA *( "-" 1*8ALPHA )
//
//		Were going to be extremely lax here and just parse this as a regular value, leaving it up to
//		the caller to make sense out of the string.  In most cases, this is going to be a legitimate
//		2 character language code, but we may sometimes see subtags.  Oh well.
//

static void VCardParserParameterLangaugeValue (VCardParserPtr parserPtr, VCardParamPtr paramPtr)

{
	(void) VCardParserAppendPropertyValueString (parserPtr, paramPtr, parserPtr->item.strings);
}


//
//	VCardParserProcessOrphanedEncoding
//
//		The only reason why this function exists is because of existing vCard writers that do not
//		propery follow the spec.  Encodings are not supposed to appear as standalone properties,
//		a la ...;WORK;QUOTED-PRINTABLE:...    They are supposed to be matched with the ENCODING
//		keyword a la ...;WORK;ENCODING=QUOTED-PRINTABLE:...
//
//		I understand the rationale, but c'mon... follow the spec.
//

static void VCardParserProcessOrphanedEncoding (VCardItemPtr itemPtr, vcKeywordType encodingValue)

{
	switch (encodingValue) {
		case vcKey7Bit:
			itemPtr->encoding = vcValue7Bit;
			break;
		case vcKey8Bit:
			itemPtr->encoding = vcValue8Bit;
			break;
		case vcKeyQuotedPrintable:
			itemPtr->encoding = vcValueQuotedPrintable;
			break;
		case vcKeyBase64:
			itemPtr->encoding = vcValueBase64;
			break;
		default:
			// Must handle the "X-" case somehow
			itemPtr->encoding = vcValueOtherEncoding;
			break;
	}
}

//
//	VCardParserEqualSign
//
//		[ws] "=" [ws]
//
//		Used often enough to be broken out into a function.
//

static void VCardParserEqualSign (VCardParserPtr parserPtr)

{
	// Check for optional whitespace...
	VCardParserWS (parserPtr, true);

	// ...and the required '=' sign
	VCardParserCharacter (parserPtr, '=', false);

	// Check for optional whitespace...
	if (parserPtr->error == vcErrorNone)
		VCardParserWS (parserPtr, true);
}



//
//	VCardParserValue
//
//		At this point, 'spot' should be pointing to the first character of an item's
//		value.  This function attempts to act on the item's stored property, like so:
//
//				"ADR"    --  Parse the address parts
//				"ORG"		 --  Parse the organization parts
//				"N"			 --  Parse the name parts
//				"AGENT"  --  An abomination!
//				other		 --  Just stores all following characters in the item's 'value' handle.
//
//

static OSErr VCardParserValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	OSErr	theError;
	
	theError = noErr;
	
	// Do something cool depending on the property
	switch (itemPtr->property) {
		case vcKeyAdr:
			theError = VCardParserAddress (parserPtr, itemPtr);
			break;
		case vcKeyOrg:
			theError = VCardParserOrganization (parserPtr, itemPtr);
			break;
		case vcKeyN:
			theError = VCardParserName (parserPtr, itemPtr);
			break;
		case vcKeyAgent:
			// The value will contain an entire vCard... do we parse this, or just pass it back???
			// We certainly could parse it, of course, but we would need to tell the caller that
			// this data coming back will be different than that we are currently parsing
			break;
		default:
			switch (itemPtr->encoding) {
				case vcValue7Bit:
					theError = VCardParser7BitValue (parserPtr, itemPtr, false, false);
					break;
				case vcValue8Bit:
					theError = VCardParser8BitValue (parserPtr, itemPtr);
					break;
				case vcValueQuotedPrintable:
					// Parse as quoted printable
					theError = VCardParserQuotedPrintableValue (parserPtr, itemPtr, 0);
					break;
				case vcValueBase64:
					// Parse as base 64
					theError = VCardParserBase64Value (parserPtr, itemPtr);
					break;
				case vcValueOtherEncoding:
					// Probably execute a callback to allow the client to interpret the value?
				default:
					// Just skip it...
					theError = VCardParser7BitValue (parserPtr, nil, false, false);
					break;
			}
			break;
	}
	return (theError);
}


//
//	VCardParserAddress
//
//		addressparts = 0*6 (strnosemi ";") strnosemi
//
//				PO Box, Extended Addr, Street, Locality, Region, Postal Code, Country Name
//
//		For each part of the address, we want to parse until we reach a semicolon, put
//		the found text into 'value', set the proper property keyword, then callback the
//		results to the client.
//

static OSErr VCardParserAddress (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	vcKeywordType	addrParts[7] = {	vcPOBox, 	vcExtendedAddress,	vcStreet, vcLocality,
																	vcRegion,	vcPostalCode,				vcCountry							};

	return (VCardParserPropertPartList (parserPtr, itemPtr, addrParts, 7));
}


//
//	VCardParserOrganization
//
//		orgparts = * (strnosemi ";") strnosemi
//
//				Organization Name, Organization Units
//
//		For each part of the organization, we want to parse until we reach a semicolon, put
//		the found text into 'value', set the proper property keyword, then callback the
//		results to the client.
//

static OSErr VCardParserOrganization (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	vcKeywordType	orgParts[2] = {	vcOrganizationName, vcOrganizationUnits };

	return (VCardParserPropertPartList (parserPtr, itemPtr, orgParts, 2));
}


//
//	VCardParserName
//
//		nameparts = 0*4 (strnosemi ";") strnosemi
//
//				Family, Given, Middle, Prefix, Suffix
//
//		For each part of the name, we want to parse until we reach a semicolon, put
//		the found text into 'value', set the proper property keyword, then callback the
//		results to the client.
//

static OSErr VCardParserName (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	vcKeywordType	nameParts[5] = {	vcFamilyName, vcGivenName, vcMiddleName, vcNamePrefix, vcNameSuffix };

	return (VCardParserPropertPartList (parserPtr, itemPtr, nameParts, 5));
}


//
//	VCardParserPropertPartList
//
//		Parse a series of semicolon delimited values from a vcard.  We pass in a pointer to the
//		list of parts (for example, family name, given name, etc) and the number of parts in the
//		list.  Each part represents a positional value, thus missing parts are still represented
//		by a semicolon.
//
//		This function parses until we reach a semicolon, puts the found text into item 'value',
//		sets the proper property keyword (based on the part list), then calls back to the client
//		to hand off the results.
//

static OSErr VCardParserPropertPartList (VCardParserPtr parserPtr, VCardItemPtr itemPtr, vcKeywordType *partList, short parts)

{
	OSErr	theError;
	short	index;

	theError = noErr;
	// Parse for semicolon separated strings
	index = 0;
	do {
		// Make sure the 'value' is cleared for each part of the address
		SetHandleBig (itemPtr->value, 0);
		itemPtr->property = *partList++;
		theError = VCardParserStrnosemi (parserPtr, itemPtr);
		if (!theError && parserPtr->itemProc)
			theError = (*parserPtr->itemProc)(itemPtr, parserPtr->refcon);
		if (!theError)
			VCardParserCharacter (parserPtr, ';', true);
	} while (!theError && ++index < parts);
	
	// Since we handled callbacks ourself, clear the property before returning
	itemPtr->property = vcKeyNone;
	return (theError);
}


//
//	VCardParser7BitValue
//
//		7bit = <7bit us-ascii printable chars, excluding CR LF>
//
//		Parse 7bit vCard data for a value terminated by either a semicolon (in the case of
//		a property value list) or a CRLF.  We also handle soft returns in quote-printable text
//		by passing true for 'quotedPrintable'.
//
//		Pass nil for the 'value' if you don't care to actually save the value
//

static OSErr VCardParser7BitValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr, Boolean valueList, Boolean quotedPrintable)

{
	Ptr			oldSpot;
	OSErr		theError;
	Boolean	done;

	theError = noErr;

	oldSpot = parserPtr->spot;
	done = false;
	while (!done && !theError && parserPtr->spot <= parserPtr->end) {
		// Take a look at interesting characters
		switch (*parserPtr->spot) {
			case '\\':
				// Escape semicolons
				if (parserPtr->spot + 1 <= parserPtr->end && isSEMI (*(parserPtr->spot + 1))) {
					// Copy the value up to (but not including) the '\'
					if (itemPtr && itemPtr->value)
						theError = PtrPlusHand (oldSpot, itemPtr->value, parserPtr->spot - oldSpot);
					// Move the pointer to the semicolon and save this spot
					oldSpot = ++parserPtr->spot;
				}
				break;
				
			case ';':
				// Semicolon terminates each member of a value list
				if (valueList)
					done = true;
				break;
				
			case '=':
				// Is it just an equal sign, or a quoted-printable soft return?
				if (parserPtr->spot + 1 <= parserPtr->end && isCRLF (*(parserPtr->spot + 1)))
					++parserPtr->spot;						// Move the pointer to the CRLF
				break;
			
			default:
				// Control characters terminate the parse (which includes CRLF's)
				if (iscntrl (*parserPtr->spot))
					done = true;
				break;
		}
		++parserPtr->spot;
	}

	// Copy the value -- which does NOT include the terminating delimiter (so we give it back)
	if (!theError)
		if (parserPtr->spot > oldSpot) {
			--parserPtr->spot;
			if (itemPtr && itemPtr->value)
				theError = PtrPlusHand (oldSpot, itemPtr->value, (long) (parserPtr->spot - oldSpot));
		}
		else
			if (!valueList)
				parserPtr->error = vcErrorExpectingValue;
	return (theError);
}


//
//	VCardParser8BitValue
//
//		8bit = <MIME RFC 1521 8-bit text>
//
//		Pass nil for the 'itemPtr' if you don't care to actually save the value
//

static OSErr VCardParser8BitValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	Ptr		oldSpot;
	OSErr	theError;
	
	theError = noErr;
	
	oldSpot = parserPtr->spot;

	// Skip past printable characters, terminating with a CR or LF
	while (parserPtr->spot <= parserPtr->end && !isCR (*parserPtr->spot) && !isLF (*parserPtr->spot))
		++parserPtr->spot;

	// If the pointer moved, there is value to what we're doing
	if (itemPtr)
		if (parserPtr->spot > oldSpot)
			theError = PtrPlusHand (oldSpot, itemPtr->value, (long) (parserPtr->spot - oldSpot));
		else
			parserPtr->error = vcErrorExpectingValue;

	return (theError);
}


//
//	VCardParserQuotedPrintableValue
//
//		quoted-printable = <MIME RFC 1521 quoted-printable text>
//
//		Overly simplified algorithm for gathering the quoted printable text
//			� Scan to the passed delimiter, or the end of the line if no delimiter is specified
//			� If we hit and end of line and the last character is an '=' we've hit a soft return and keep scanning
//

static OSErr VCardParserQuotedPrintableValue (VCardParserPtr parserPtr, VCardItemPtr itemPtr, char delimiter)

{
	return (VCardParser7BitValue (parserPtr, itemPtr, false, true));
}


//
//	VCardParserBase64Value
//
//		base64 = <MIME RFC 1521 base64 text>
//			; the end of the text is marked with two CRLF sequences
//
//		We're going to be real simpletons and just look for the CRLF CRLF
//		sequence, passing back everything that precedes as the value.  This
//		is our best guess of legal Base64.  The client should be better
//		equipped to validate the value when it attempts to decode the value.
//

static OSErr VCardParserBase64Value (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	Ptr			oldSpot;
	OSErr		theError;
	Boolean	done;
	
	theError = noErr;
	
	oldSpot = parserPtr->spot;

	// Scanning the data until we're 'done'.
	done = false;
	while (!done && parserPtr->spot <= parserPtr->end) {
		if (isCR (*parserPtr->spot) || isLF (*parserPtr->spot))
			done = (parserPtr->spot > oldSpot && (isCR (*(parserPtr->spot - 1)) || isLF (*(parserPtr->spot - 1))));
		++parserPtr->spot;
	}
					
	// Copy the value -- which does NOT include either terminating CRLF (and we give one of them back)
	if (parserPtr->spot - 1 > oldSpot) {
		theError = PtrPlusHand (oldSpot, itemPtr->value, (long) (parserPtr->spot - oldSpot - 2));
		--parserPtr->spot;
	}
	else
		parserPtr->error = vcErrorExpectingValue;

	return (theError);
}


//
//	VCardParserSkipToNextLine
//
//		Skip past all characters on this line, leaving the 'spot' pointing to the first
//		character on the next line.
//

static void VCardParserSkipToNextLine (VCardParserPtr parserPtr)

{
	// Skip past all characters, skipping CR's and LF's
	while (parserPtr->spot <= parserPtr->end && !isCR (*parserPtr->spot) && !isLF (*parserPtr->spot))
		++parserPtr->spot;
	VCardParseCRLFs (parserPtr, true);
}


//
//	VCardParserWS
//
//		ws = 1*(SPACE / HTAB)
//			; "whitespace," one or more spaces or tabs
//
//		Return 'true' if we actually did find white space, 'false' if not
//

static Boolean VCardParserWS (VCardParserPtr parserPtr, Boolean optional)

{
	Ptr			oldSpot;
	Boolean	wsPresent;
	
	oldSpot = parserPtr->spot;

	// Skip spaces and tabs
	while (parserPtr->spot <= parserPtr->end && (isSPACE (*parserPtr->spot) || isHTAB (*parserPtr->spot)))
		++parserPtr->spot;

	if (wsPresent = (oldSpot == parserPtr->spot))
		if (!optional)
			parserPtr->error = vcErrorExpectingWhitespace;
	return (wsPresent);
}



//
//	VCardParserWSLS
//
//		wsls = 1*(SPACE / HTAB / CRLF)
//			; whitespace with line separators
//
//		Return 'true' if we actually did find white space, 'false' if not
//

static Boolean VCardParserWSLS (VCardParserPtr parserPtr, Boolean optional)

{
	Ptr			oldSpot;
	Boolean	wsPresent;

	oldSpot = parserPtr->spot;

	// Skip spaces, tabs and CRLF's
	while (parserPtr->spot <= parserPtr->end && (isSPACE (*parserPtr->spot) || isHTAB (*parserPtr->spot) || isCRLF (*parserPtr->spot)))
		++parserPtr->spot;

	if (wsPresent = (oldSpot == parserPtr->spot))
		if (!optional)
			parserPtr->error = vcErrorExpectingWhitespace;
	return (wsPresent);
}


//
//	VCardParserStrnosemi
//
//		strnosemi = *(*nosemi ("\;" / "\" CRLF)) *nosemi
//
//		A string of 0 or more non-control ASCII characters, terminated by a semicolon.
//
//		  ';' or CRLF may appear in the string if escaped with a '\'
//
//		At the conclusion of this function, 'spot' points to the first non-strnosemi character.
//

static OSErr VCardParserStrnosemi (VCardParserPtr parserPtr, VCardItemPtr itemPtr)

{
	return (VCardParser7BitValue (parserPtr, itemPtr, true, itemPtr->encoding == vcValueQuotedPrintable));
}



//
//	VCardParserKeyword
//
//		Parse the vCard data for a keyword.  Defined here to be any string of one or more
//		of the upper or lowercase characters A - Z, plus the digits 0 - 9.  We'll also let
//		in '-' characters so that we'll pickup things like X-EUDORA-SHOE-SIZE.
//
//		The parsed string is placed in a Pascal style string, and the parser 'spot' is left
//		pointing to the first character that follows the keyword.
//

static PStr VCardParserKeyword (VCardParserPtr parserPtr, PStr keyword)

{
	*keyword = 0;
	
	// copy alpha characters into the keyword
	while (*keyword < sizeof (Str255) && parserPtr->spot <= parserPtr->end && (isalpha (*parserPtr->spot) || isdigit (*parserPtr->spot) || isDASH (*parserPtr->spot)))
		keyword[++keyword[0]] = *parserPtr->spot++;
	return (keyword);
}


//
//	VCardParserWord
//
//		Parse the vCard data for a word.  Defined here to be any string of one or more
//		7Bit us-ascii printable characted, excepting a couple of punctuation characters.
//		The data is appended onto the end of the handle passed as a parameter.
//
//		There is a potential bug here depending on how you choose to interpret the definition
//		above.  If a CR or a LF is found in the text -- but not a CRLF -- is it merely ignored
//		with subsequent characters continueing to be scanned?  Or does the first occurence of
//		either terminate the scan?  For now, we're doing the latter.
//

static OSErr VCardParserWord (VCardParserPtr parserPtr, Handle value)

{
	Ptr		oldSpot;
	OSErr	theError;
	
	theError = noErr;
	
	oldSpot = parserPtr->spot;

	// Skip past printable characters, skipping CR's and LF's
	while (parserPtr->spot <= parserPtr->end && isprint (*parserPtr->spot) && !strchr ("[]=:., ", *parserPtr->spot) && !isCR (*parserPtr->spot) && !isLF (*parserPtr->spot))
		++parserPtr->spot;

	// If the pointer moved, there is value to what we're doing
	if (parserPtr->spot > oldSpot)
		theError = PtrPlusHand (oldSpot, value, (long) (parserPtr->spot - oldSpot));
	else
		parserPtr->error = vcErrorExpectingValue;

	return (theError);
}


//
//	VCardParserCharacter
//
//		Parse a single specified character from the data stream, returning 'true' if the character
//		was found, 'false' if not
//

static Boolean VCardParserCharacter (VCardParserPtr parserPtr, char c, Boolean optional)

{
	Boolean	found;
	
	if (*parserPtr->spot == c) {
		++parserPtr->spot;
		found = true;
	}
	else {
		if (!optional)
			switch (c) {
				case '.':		parserPtr->error = vcErrorExpectingPeriod;		break;
				case ':':		parserPtr->error = vcErrorExpectingColon;			break;
				case ';':		parserPtr->error = vcErrorExpectingSemicolon;	break;
				default:		parserPtr->error = vcErrorExpectingDelimiter;	break;
			}
		found = false;
	}
	return (found);
}


//
//	VCardParseCRLFs
//
//		Parse carriage returns and linefeeds from the data stream.
//
//		Return 'true' if we actually did find a CRLF, 'false' if not.
//		At the conclusion of the function, 'spot' points to the first non-
//		CRLF.
//

static Boolean VCardParseCRLFs (VCardParserPtr parserPtr, Boolean optional)

{
	Ptr			oldSpot;
	Boolean	crlfPresent;
	
	oldSpot = parserPtr->spot;

	// Skip CRLF's
	while (parserPtr->spot <= parserPtr->end && isCRLF (*parserPtr->spot))
		++parserPtr->spot;

	if (crlfPresent = (oldSpot == parserPtr->spot))
		if (!optional)
			parserPtr->error = vcErrorExpectingCRLF;
	
	return (crlfPresent);
}


static OSErr VCardAppendKeywordToGroup (VCardItemPtr itemPtr, Str255 keyword)

{
	OSErr	theError;
	
	theError = noErr;
	if (GetHandleSize (itemPtr->group))
		theError = PtrPlusHand (".", itemPtr->group, 1);
	if (!theError)
		theError = PtrPlusHand (&keyword[1], itemPtr->group, *keyword);
	return (theError);
}


static OSErr VCardParserAppendPropertyValueString (VCardParserPtr parserPtr, VCardParamPtr paramPtr, Handle strings)

{
	OSErr	theError;
	
	paramPtr->pValue = vcKeyString;
	paramPtr->offset = GetHandleSize (strings);
	theError = VCardParserWord (parserPtr, strings);
	if (!theError)
		paramPtr->length = GetHandleSize (strings) - paramPtr->offset;
	return (theError);
}


//
//	VCardProcessProperty
//
//		Check to see if a parsed keyword matches one of our known keywords, returning
//		the ID for that keyword.  If the keywword is unknown, return 'vcKeyNone'.
//

static vcKeywordType VCardProcessProperty (VCardParserPtr parserPtr, Str255 keyword)

{
	vcKeywordType	keywordID;
	Str15					xDash;
	short					index;
	
	keywordID = vcKeyNone;
	// Does it match one of our known keywords?
	if (index = FindSTRNIndex (VCardKeywordStrn, keyword))
		keywordID = index + vcKeyBegin - 1;
	else {
		// Is it an "X-"?
		if (BeginsWith (keyword, GetRString (xDash, VCardKeywordStrn + vcXDash)))
			// One we know about, or one we don't
			keywordID = (index = FindXDashString (parserPtr->xdash, keyword)) != -1 ? -(index + 1) : vcKeyOtherXDash;
		else
			parserPtr->error = vcErrorExpectingKeyword;
	}
	return (keywordID);
}


//
//	FindXDashString
//
//		Find an "X-" keyword in a list of "X-" strings, returning the index of the
//		found string, or -1 if the keyword was not present.
//

static short FindXDashString (XDashStringHandle xdash, PStr keyword)

{
  UPtr		spot,
  				end;
  short		index;
  Boolean	found;
	
	found = false;
	index = 0;
	if (xdash) {
		spot	= LDRef (xdash);
		end	= spot + GetHandleSize (xdash);
		while (spot < end && !found) {
			if (StringSame (spot, keyword))
				found = true;
			else {
				++index;
				spot += (*spot + 2);
			}
		}
		UL (xdash);
	}
	return (found ? index : -1);
}


Boolean IsVCardFile (FSSpecPtr spec)

{
	Str32		extension,
					vcardExtension;
	short		i;
	Boolean	result;

	if (!IsVCardAvailable ())
		return (false);
	
	result = false;

	//	Resolve if alias
	IsAlias (spec, spec);
	
	// Is this a Eudora vCard?
	result = (FileTypeOf (spec) == VCARD_TYPE);
	
	// How about an 8.3 vCard?
	if (!result) {
		for (i = spec->name[0]; i; --i)
			if (spec->name[i] == '.')
				break;
	
		if (i) {
			MakePStr (extension, &spec->name[i + 1], spec->name[0] - i);
			
			result = striscmp (extension, GetRString (vcardExtension, VCARD_FILE_EXTENSION)) == 0;
		}
	}
	
	// Hmmmm... maybe just a TEXT file (not created by Eudora)?
	if (!result)
		result = (FileTypeOf (spec) == 'TEXT' && FileCreatorOf (spec) != CREATOR);
	
	// We could go an extra step here and peek into the file, checking for "BEGIN:VCARD"... nah.
	if (result)
		;
		
	return (result);
}


//
//	MakeVCardFileName
//
//		Make an appropriate name for a vCard.  For now, this is simply the nickname
//		itself with ".vcf" appended.
//

PStr MakeVCardFileName (short ab,short nick, PStr filename)

{
	Str31	extension;
	
	GetNicknameNamePStr (ab, nick, filename);
	filename[++filename[0]] = '.';
	PCat (filename, GetRString (extension, VCARD_FILE_EXTENSION));
	*filename = MIN (*filename, sizeof (Str31) - 1);
	return (filename);
}


#endif
