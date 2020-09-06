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

#ifdef SPEECH_ENABLED
#ifdef TWO
#ifndef SPEECHUTIL_H
#define SPEECHUTIL_H

#include <Speech.h>

//
//	'dict' resource (for which Apple hasn't created a data structure)
//

enum {
	kDictionaryAtom					= FOUR_CHAR_CODE('dict'),
	kDictionaryAtomVersion	= 1
};

enum {
	nullEntryField = 0x0000,
	textEntryField = 0x0021,
	phonEntryField = 0x0022
};

enum {
	pronunciationEntry = 0x0021,
	abbreviationEntry	 = 0x0022
};

typedef struct {
	UInt16	length;
	SInt16	type;
} DictEntryFieldHeaderRec, *DictEntryFieldHeaderPtr, **DictEntryFieldHeaderHandle;

typedef struct {
	UInt16	length;
	SInt16	type;
	UInt16	numFields;
} DictEntryHeaderRec, *DictEntryHeaderPtr, **DictEntryHeaderHandle;

typedef struct {
	UInt32				length;
	FourCharCode	atom;
	SInt32				version;
	ScriptCode		script;
	LangCode			language;
	RegionCode		region;
	UInt32				modDate;
	UInt8					reserved[16];
	UInt32				numEntries;
} DictHeaderRec, *DictHeaderPtr, **DictHeaderHandle;
	

//
//	'ACRY' resource (our own invention)
//
typedef unsigned char	Str8[9];

typedef struct {
	Str8		letters;
	Str255	spokenAs;
} AcronymRec, *AcronymPtr, **AcronymHandle;

#define	rAcronymResType		'ACRY'

typedef struct {
	long	paraOffset;
	Byte	quoteLevel;
} QuoteTableRec, *QuoteTablePtr, **QuoteTableHandle;

#define SPEECH_PREF_FILE_TYPE			'pref'
#define	SPEECH_PREF_FILE_CREATOR	'pltk'

//
//	Format of the Speech control panel's Talking Alerts preference
//
//		(near as I can figure...)
//

#define rTalkingAlertResType					'SADt'	// Talking Alerts pref resource type
#define rTalkingAlertPrefResID				-4033		// Talking Alerts pref resource ID
#define rTalkingAlertPhraseListResID	-4034		// Talking Alerts speak phrases resource ID

#define	kSpeakTheNextPhrase				0x0100
#define	kSpeakARandomPhrase				0x0200

//
//	Format of the 'SADt' (-4033) resource
//
typedef struct {
	short		unknown1;				// ...perhaps the preference version?
	char		unknown2;				// ...search me
	Boolean	speakPhrase;		// State of the "Speak phrase" checkbox
	Boolean	speakAlertText;	// State of the "Speak alert text" checkbox
	char		unknown3;				// ...dunno
	short		delayTicks;			// Number of ticks to wait before speaking the alert
	short		phraseModifier;	// Speak a phrase other then the default phrase
	short		phraseIndex;		// Index of the alert phrase in the 'SADt' (-4034) resource (1 based)
} TalkingAlertPrefRec, *TalkingAlertPrefPtr, **TalkingAlertPrefHandle;

//
//	Format   of the 'SADt' (-4034) resource
//
//		short		;	number of phrases
//						; followed by a concatenation of all defined phrases
//

//
//	Parts of a message we wish to speak (as brought to you by SpeakMessage)
//
typedef enum {
	speakNothing	= 0x00000000,
	speakSender		= 0x00000001,
	speakSubject	= 0x00000002,
	speakBody			= 0x00000004,
	speakTo				= 0x00000008,
	speakSummary	= 0x00000010
} SpeakableParts;


//
//	A single speech queue element
//

typedef struct {
	VoiceSpec				voice;					// The voice!
	SpeechChannel		channel;				// The channel upon which we will speak
	Handle					buffer;					// The text buffer to be spoken
	Boolean					validChannel;		// True if we successfully generated a new speech channel
	Boolean					textDone;				// Set to true once the speech manager has processed all bytes in  our buffer
	Boolean					speechDone;			// Set to true once the speech manager has finished speaking our buffer
	Handle					next;						// Handle to the next Speech record in the queue
} SpeakQueueRec, *SpeakQueuePtr, **SpeakQueueHandle;

Boolean						SpeechAvailable (void);
OSErr							Speak (VoiceSpec *voice, Ptr textBuf, long textBytes);
Boolean						SpeechIdle (void);
Boolean						CancelSpeech (void);
void							SpeechShutup (void);
OSErr							SpeakOnNewChannel (SpeakQueueHandle element);
OSErr							SetupSpeechCallbacks (SpeechChannel chan, long refcon);
void							ZapSpeakElement (SpeakQueueHandle element);
	
OSErr							GetVoiceDescriptionDefaultOkay (VoiceSpec *voice, VoiceDescription *info, long infoLength, Boolean *defaultVoice);
OSErr							BuildVoiceMenu (MenuHandle theMenu);
ControlHandle			GetNewVoicePopupSmall (short id, WindowPtr win, VoiceSpec *currentVoice);
OSErr							GetVoiceName (VoiceSpec *voice, Str63 name);
OSErr							FindVoiceFromName (Str63 name, VoiceSpec *foundVoice);
OSErr							SpeakMessage (VoiceSpec *voice, TOCHandle tocH, short sumNum, SpeakableParts speak, Boolean nextMessage);
OSErr							SpeakSelectedText (VoiceSpec *voice, PETEHandle pte);
void							TalkingAlert (Boolean spokenWarning, AlertType alertType, StringPtr error, StringPtr explanation, AlertStdAlertParamPtr alertParam, short *item);
void							ParseTalkingAlertString (StringPtr error, StringPtr displayError, StringPtr speakError);
OSErr 						SpeakAlert (StringPtr phrase, StringPtr error, StringPtr explanation);

OSErr							SpeakSelectedMessages (TOCHandle tocH);
OSErr							AccuAddSpeakableText (AccuPtr a, UPtr textPtr, long len, QuoteTableHandle quoteTable, Boolean lookForQuotes);
OSErr							AccuAddQuoteStr (AccuPtr a, short resID);
Boolean						NumberShorthand (UPtr p);
QuoteTableHandle	BuildQuoteLevelTable (PETEHandle pte, uLong offset);
OSErr							AccuTranslateEmoticon (AccuPtr a);
short							GetQuoteLevel (UPtr p, UPtr endPtr, UPtr textPtr, QuoteTableHandle quoteTable, short quoteLevel, short numParagraphs, Str31 prefix, short *paragraphIndex, UPtr *speakAbleText);
OSErr							InsertAndMoveBase (AccuPtr a, UPtr p, UPtr text, long length, UPtr *base);
PStr							SmartAddressSpeaking (PStr scratch);
OSErr							AccuAddAddressHeader (AccuPtr a, MessHandle messH, PStr substitute, short headerIndex, short headerResID);
OSErr							AccuAddAcronym (AccuPtr a, PStr tla, Boolean *wasTranslated);
OSErr							InstallPronunciationDictionary (SpeechChannel channel, short resID);
OSErr 						FindUserPronunciationDictionary (Str31 filename, FSSpec *dictionarySpec);
OSErr 						CreateSpeechDictionary (FSSpec *dictionarySpec, Handle *dictionary);
OSErr 						AccuAddDictionaryEntryField (AccuPtr a, PStr string, SInt16 type);
OSErr 						GetSpeechDictionaryLine (LineIOP lip, PStr word, PStr phoneme);

#ifdef NOT_YET_THIS_WILL_BE_SUPPORTED_AT_SOME_POINT_IN_THE_FUTURE
OSErr							InstallPureVoiceOutputComponent (SpeechChannel chan);
#endif
#endif
#endif
#endif