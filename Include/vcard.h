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

#ifndef VCARD_H
#define VCARD_H

/* Copyright (c) 2000 by QUALCOMM Incorporated */

typedef enum {
	vcKeyNone = 0,
	vcKeyOtherXDash = 1,	// An "X-" keyword that doesn't match any passed to the parse
	vcKeyString = 2,	// When present, the text of the keyword is stored in the 'strings' Handle of aparamter record
	vcKeyBegin = 3,		// DO NOT CHANGE THE ORDER OF THESE FROM HERE DOWN!!
	vcKeyVcard = 4,
	vcKeyEnd = 5,
	vcKeyAdr = 6,
	vcKeyOrg = 7,
	vcKeyN = 8,
	vcKeyAgent = 9,
	vcKeyType = 10,
	vcKeyValue = 11,
	vcKeyEncoding = 12,
	vcKeyCharset = 13,
	vcKeyLanguage = 14,
	vcKeyInline = 15,
	vcKeyURL = 16,
	vcKeyContentID = 17,
	vcKeyCID = 18,
	vcKey7Bit = 19,
	vcKey8Bit = 20,
	vcKeyQuotedPrintable = 21,
	vcKeyBase64 = 22,
	vcKeyLogo = 23,
	vcKeyPhoto = 24,
	vcKeyLabel = 25,
	vcKeyFN = 26,
	vcKeyTitle = 27,
	vcKeySound = 28,
	vcKeyVersion = 29,
	vcKeyLang = 30,
	vcKeyTel = 31,
	vcKeyEmail = 32,
	vcKeyTZ = 33,
	vcKeyGeo = 34,
	vcKeyNote = 35,
	vcKeyBday = 36,
	vcKeyRole = 37,
	vcKeyRev = 38,
	vcKeyUID = 39,
	vcKeyKey = 40,
	vcKeyMailer = 41,
	vcKeyDom = 42,
	vcKeyIntl = 43,
	vcKeyPostal = 44,
	vcKeyParcel = 45,
	vcKeyHome = 46,
	vcKeyWork = 47,
	vcKeyPref = 48,
	vcKeyVoice = 49,
	vcKeyFax = 50,
	vcKeyMSG = 51,
	vcKeyCell = 52,
	vcKeyPager = 53,
	vcKeyBBS = 54,
	vcKeyModem = 55,
	vcKeyCar = 56,
	vcKeyISDN = 57,
	vcKeyVideo = 58,
	vcKeyAOL = 59,
	vcKeyAppleLink = 60,
	vcKeyATTMail = 61,
	vcKeyCIS = 62,
	vcKeyEWorld = 63,
	vcKeyInternet = 64,
	vcKeyIBMMail = 65,
	vcKeyMCIMail = 66,
	vcKeyPowerShare = 67,
	vcKeyProdigy = 68,
	vcKeyTLX = 69,
	vcKeyX400 = 70,
	vcKeyGIF = 71,
	vcKeyCGM = 72,
	vcKeyWMF = 73,
	vcKeyBMP = 74,
	vcKeyMET = 75,
	vcKeyPMB = 76,
	vcKeyDIB = 77,
	vcKeyPICT = 78,
	vcKeyTIFF = 79,
	vcKeyPDF = 80,
	vcKeyPS = 81,
	vcKeyJPEG = 82,
	vcKeyQTime = 83,
	vcKeyWave = 84,
	vcKeyAIFF = 85,
	vcKeyPCM = 86,
	vcKeyX509 = 87,
	vcKeyPGP = 88,		// From here up MUST match those items in VCardKeywordStrn
	vcPOBox = 89,		// addressparts
	vcExtendedAddress = 90,	// addressparts
	vcStreet = 91,		// addressparts
	vcLocality = 92,	// addressparts
	vcRegion = 93,		// addressparts
	vcPostalCode = 94,	// addressparts
	vcCountry = 95,		// addressparts
	vcFamilyName = 96,	// nameparts
	vcGivenName = 97,	// nameparts
	vcMiddleName = 98,	// nameparts
	vcNamePrefix = 99,	// nameparts
	vcNameSuffix = 100,	// nameparts
	vcOrganizationName = 101,	// orgparts
	vcOrganizationUnits = 102	// orgparts
} vcKeywordType;

typedef enum {
	vcErrorNone = 0,
	vcErrorExpectingWhitespace,
	vcErrorExpectingPeriod,
	vcErrorExpectingColon,
	vcErrorExpectingSemicolon,
	vcErrorExpectingDelimiter,
	vcErrorExpectingCRLF,
	vcErrorExpectingKeyword,
	vcErrorExpectingItem,
	vcErrorExpectingValue,
	vcErrorUnknownParamter,
	vcMissingBegin,
	vcMissingEnd,
	vcMissingVcard
} VCardErrorType;

typedef enum {
	vcValueOtherEncoding = 0,
	vcValue7Bit,
	vcValue8Bit,
	vcValueQuotedPrintable,
	vcValueBase64,
} ValueEncodingType;

typedef enum {
	vcValueOtherCharset = 0,
	vcValueUSAscii,
	vcValueISO_8859_1
} ValueCharsetType;

typedef enum {
	vcPropFlagNone = 0x0000,
	vcPropFlagPreferred = 0x0001,
	vcPropFlagHome = 0x0002,
	vcPropFlagWork = 0x0004
} VCardPropertyFlagsType;

typedef UHandle XDashStringHandle;	// All the X- things the client cares about -- length-byte, text, null (ad nauseum...)

//
//      VCardParamRec
//
//              Parameter records contain a property keyword and a value keyword.  For example:
//
//                              ENCODING = QUOTED-PRINTABLE
//
//              In the above example, "ENCODING" is the property keyword and "QUOTED-PRINTABLE"
//              is the value keyword.  Because vCard is SO DARN POWERFUL (i.e. way overly complex
//              and verbose for a simple business card format), parameters may contain text that
//              can't possibly be described in a known keyword.  Take a look at this wonderfully
//              vCard-legal example:
//
//                              X-MUSEUM-OF-DEATH = GRUESOME
//
//              Golly gee Batman, no keywords there, huh!!  In this case we have to leave it up to
//              the client to tell us what to do.  When the vCard parse is first primed, the client
//              passes down a list of known "X-" types in a Handle (coded as null separated, concatenated
//              Pascal Strings), and we'll assign as our keyword the negative index of the found "X-"
//              in the passed Handle.  So, in the above case, "X-Museum-Of-Death" might be assigned -4
//              indicating that this matches the 4th "X-" string.
//
//              The second half of the above assignment is a little more complicated.  In the 'pValue'
//              field we'll assign 'vcKeyString' and store in 'offset' an offset into the 'strings'
//              field of the VCardItemRec pointing to the real text.
//

typedef struct {
	vcKeywordType pProperty;
	vcKeywordType pValue;
	long offset;
	long length;
} VCardParamRec, *VCardParamPtr, **VCardParamHandle;

//
//      VCardItemRec
//
//              Each item of a vCard MUST contain a value of some sort.  vCard, it turns out, is really
//              a value-oriented format, with each value assigned to 1 or more fields, modified in 0 or
//              more ways.  I guess this made sense to someone when it was being defined.  Personally,
//              I think they were on drugs.  Anyway, since our focus is going to be on values, 'value'
//              is a required field containing the value (duh) of this vCard line item.  We also have a
//              required property for each item, representing things like "ADR", "ORG", "N", or a valid
//              vCard field name.
//
//              In addition, we might optionally also find various parameters or group information.  Each
//              of these is stored in a Handle.  'params' is a collection of VCardParamRec records, while
//              'group' is a Handle containing the text of the whole group identifier.  'strings' is a
//              Handle containing any text needed by the parameters.
//

typedef struct {
	vcKeywordType property;
	ValueEncodingType encoding;
	ValueCharsetType charset;
	VCardPropertyFlagsType propertyFlags;
	Handle value;
	Handle group;
	Handle strings;
	VCardParamHandle params;
} VCardItemRec, *VCardItemPtr, **VCardItemHandle;

typedef OSErr(*VCardItemProc) (VCardItemPtr itemPtr, long refcon);
typedef Boolean(*VCardErrorProc) (Handle vCard, VCardItemPtr itemPtr,
				  long refcon, VCardErrorType error,
				  long offset);

Boolean IsVCardAvailable(void);
Handle MakeVCard(Handle addresses, Handle notes);
OSErr ParseVCard(Handle vCard, uLong * offset,
		 XDashStringHandle xDashStrings, VCardItemProc itemProc,
		 VCardErrorProc errorProc, long refcon);
Boolean IsVCardFile(FSSpecPtr spec);
PStr MakeVCardFileName(short ab, short nick, PStr filename);

#endif
