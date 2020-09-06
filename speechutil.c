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
#include "speechutil.h"
#define FILE_NUM 116

#define G4_PROBLEM	1

#pragma segment speech

pascal void TextDoneCallback (SpeechChannel chan, SpeakQueueHandle SpeakQueueHandle, const void **nextBuf, unsigned long *byteLen, long *controlFlags);
pascal void SpeechDoneCallback (SpeechChannel chan, SpeakQueueHandle SpeakQueueHandle);
pascal Boolean TalkingAlertFilter (DialogPtr dgPtr, EventRecord *event, short *item);

SpeakQueueHandle				gSpeakQueue = nil;					// Queue of buffers submitted to be spoken
StringPtr								gTalkError = nil;						// The error string we will speak
StringPtr								gTalkExplain = nil;					// The explanation string we will speak
StringPtr								gTalkPhrase = nil;					// The alert phrase we will speak
long										gTalkingAlertTicks;					// Ticks at which to speak alerts
short										gNextPhrase = 1;						// Index to the next alert phrase to be spoken
Boolean									gTalkingAlertPresent = false;

extern UHandle GetMessText(MessHandle messH);

//
//	SpeechAvailable
//
//		Checks to see if the Speech Manager is avilable

Boolean SpeechAvailable (void)

{
#ifdef SPEECH_ENABLED
	long		result;
	Boolean	present;
	
	if (!HasFeature (featureSpeak))
		return (false);
		
	if (present = (Gestalt (gestaltSpeechAttr, &result) == noErr))
		present = (result & (1 << gestaltSpeechMgrPresent)) ? true : false;
#if TARGET_RT_MAC_CFM
	if (present)
		present = ((long) MakeVoiceSpec != kUnresolvedCFragSymbolAddress);
#endif
	return (present);
#else
	return (false);
#endif
}



//
//	Speak
//
//		This is a low level routine for speaking a buffer of text over an open speech channel.
//		It converts the text into speech and plays the speech asynchronously.
//
//		This routine accepts:
//					; a speech channel that describes how the text is to be spoken
//					; a pointer to text that is to be spoken
//					; the length of the text to be spoken
//		And returns:
//					; noErr if the text is spoken successfully (or the Speech Manager is not present)
//					; Speech Manager errors
//

OSErr Speak (VoiceSpec *voice, Ptr textBuf, long textBytes)

{
	SpeakQueueHandle	element;
	VoiceDescription	info;
 	OSErr							theError;
	short							oldResFile;

	if (!SpeechAvailable ())
		return (noErr);

	theError = noErr;
	
	// Create a queue element
	if (element = NuHTempBetter (sizeof (SpeakQueueRec))) {
	 	LDRef (element);
		(*element)->validChannel = false;
		(*element)->textDone		 = false;
		(*element)->speechDone	 = false;
		(*element)->next				 = nil;

		oldResFile = CurResFile ();
		if (voice)
			(*element)->voice = *voice;
		else
			theError = GetVoiceDescription (voice, &info, sizeof (info));
		UseResFile (oldResFile);
		
		if (!theError) {
			// Add it to our speaking queue
			LL_Queue (gSpeakQueue, element, (SpeakQueueHandle));
		
			// Make a copy of the text so we can manage our own buffer
			(*element)->buffer = NuDHTempBetter (textBuf, textBytes);
			theError = MemError ();
		}
		if (!theError) {
			// Check to see if we or another app are already speaking (or making sound).
			// If not, go ahead and start speaking.  Otherwise, we'll have to wait for
			// a pause in the conversation.
			if (!SpeechBusySystemWide () && element == gSpeakQueue)
				theError = SpeakOnNewChannel (gSpeakQueue);
		}

		// Clean up after any errors
		if (theError)
			ZapSpeakElement (element);
	}
	return (theError);
}

//
//	SpeechIdle
//
//		Always checks the head element in the queue, returning true if the Speech
//		Manager is pending an event of some sort, false if we have nothing to do.
//
Boolean SpeechIdle (void)

{
	SpeakQueueHandle	element;

	if (gSpeakQueue) {
	
		if (!SpeechAvailable ())
			return (false);
			
		if ((*gSpeakQueue)->textDone && (*gSpeakQueue)->buffer)
				ZapHandle ((*gSpeakQueue)->buffer);
		if ((*gSpeakQueue)->speechDone && (*gSpeakQueue)->validChannel) {
			DisposeSpeechChannel ((*gSpeakQueue)->channel);
			
			element = gSpeakQueue;
			LL_Remove (gSpeakQueue, element, (SpeakQueueHandle));
			ZapHandle (element);

			if (gSpeakQueue && !SpeechBusySystemWide ())
				if (SpeakOnNewChannel (gSpeakQueue))
					while (element = gSpeakQueue)
						ZapSpeakElement (element);
		}
	}
	return (gSpeakQueue ? true : false);
}


//
//	CancelSpeech
//
//		Should be called anytime the user presses cmd-period to cancel
//		speaking.
//
//		Returns true if we intercepted the cmd-period and canceled the
//		speech, false if we were not speaking at the time -- indicating
//		that the cmd-period was targeted at another operation.
//

Boolean CancelSpeech (void)

{
	Boolean	chatty;

	chatty = false;
	if (SpeechAvailable ())
		if (SpeechBusy ()) {
			SpeechShutup ();
			chatty = gTalkingAlertPresent ? false : true;
		}
	return (chatty);
}


void SpeechShutup (void)

{
	SpeakQueueHandle element;

	if (!SpeechAvailable ())
		return;

	// Walk through the speech queue getting rid of everything
	while (element = gSpeakQueue) {
		if ((*element)->validChannel)
			StopSpeech ((*element)->channel);
		ZapSpeakElement (element);
	}
}


OSErr SpeakOnNewChannel (SpeakQueueHandle element)

{
	OSErr	theError;

	MightSwitch ();

	// Grab a speech channel upon which we'll talk
	if (theError = NewSpeechChannel (&(*element)->voice, &(*element)->channel))
		theError = NewSpeechChannel (nil, &(*element)->channel);

	if (!theError)
		(void) InstallPronunciationDictionary ((*element)->channel, SPEAK_DICTIONARY);
	
	// Setup callbacks for text done and speech done
	if (!theError) {
		(*element)->validChannel = true;
		theError = SetupSpeechCallbacks ((*element)->channel, (long) element);
	}
	
	// Volume
	{
		Fixed volume = GetRLong(SPEECH_VOLUME)*6553;
		SetSpeechInfo((*element)->channel, soVolume, (Ptr)&volume);
	}
	
#ifdef NOT_YET_THIS_WILL_BE_SUPPORTED_AT_SOME_POINT_IN_THE_FUTURE
	if (!theError)
		theError = InstallPureVoiceOutputComponent ((*element)->channel);
#endif
	
	// Check to see if we or another app are already speaking (or making sound).
	// If not, go ahead and start speaking.  Otherwise, we'll have to wait for
	// a pause in the conversation.
	if (!theError) {
		MoveHHi ((*element)->buffer);
		LDRef ((*element)->buffer);
		theError = SpeakText ((*element)->channel, *(*element)->buffer, GetHandleSize ((*element)->buffer));
	}

	AfterSwitch ();

	return (theError);
}


pascal void TextDoneCallback (SpeechChannel chan, SpeakQueueHandle SpeakQueueHandle, const void **nextBuf, unsigned long *byteLen, long *controlFlags)

{
	(*SpeakQueueHandle)->textDone = true;

	*nextBuf = nil;
	*byteLen = 0;
}


pascal void SpeechDoneCallback (SpeechChannel chan, SpeakQueueHandle SpeakQueueHandle)

{
	(*SpeakQueueHandle)->speechDone = true;
}

OSErr SetupSpeechCallbacks (SpeechChannel chan, long refcon)

{
	OSErr	theError;
	DECLARE_UPP(TextDoneCallback,SpeechTextDone);
	DECLARE_UPP(SpeechDoneCallback,SpeechDone);
	
	INIT_UPP(TextDoneCallback,SpeechTextDone);
	INIT_UPP(SpeechDoneCallback,SpeechDone);
		theError = SetSpeechInfo (chan, soRefCon, (Ptr) refcon);
	if (!theError)
		theError = SetSpeechInfo (chan, soTextDoneCallBack, TextDoneCallbackUPP);
	if (!theError)
		theError = SetSpeechInfo (chan, soSpeechDoneCallBack, SpeechDoneCallbackUPP);
	return (theError);
}


void ZapSpeakElement (SpeakQueueHandle element)

{
	if (element) {
		if ((*element)->validChannel)
			DisposeSpeechChannel ((*element)->channel);
		ZapHandle ((*element)->buffer);
		LL_Remove (gSpeakQueue, element, (SpeakQueueHandle));
		ZapHandle (element);
	}
}


//
//	GetVoiceDescriptionDefaultOkay
//
//		Attempts to get the voice description of a particular voice, retrieving information
//		about the default voice if the requested voice cannot be found.
//
//		This routine accepts:
//					; a pointer to the voice we want a description of
//					; a pointer to a voice desription record into which we'll return information about
//						the requested voice, or the default voice if we could not find the requested voice
//					; the length of the information we're requesting
//					; a pointer to a boolean variable into which we'll store 'true' if we were forced to
//						return info about the default voice, or 'false' if the info is for the voice we
//						requested
//		And returns:
//					; noErr if the text is spoken successfully (or the Speech Manager is not present)
//					; userCanceledErr if the user halts the speech
//					; Speech Manager errors
//

OSErr GetVoiceDescriptionDefaultOkay (VoiceSpec *voice, VoiceDescription *info, long infoLength, Boolean *defaultVoice)

{
	OSErr	theError;
	short	resFile;
	
	if (!SpeechAvailable ())
		return (noErr);
	
	resFile = CurResFile ();
	theError = GetVoiceDescription (voice, info, infoLength);
	if (!theError)
		*defaultVoice = false;
	else
		*defaultVoice = (theError = GetVoiceDescription (nil, info, infoLength)) ? false : true;
	UseResFile (resFile);
	return (theError);
}


//
//	BuildVoiceMenu
//
//		Fills a menu handle with the names of the voices currently available
//

OSErr BuildVoiceMenu (MenuHandle theMenu)

{
	VoiceDescription	info;
	VoiceSpec					voice;
	OSErr							theError;
	short							numVoices,
										resFile,
										index;
	
	if (!theMenu || !SpeechAvailable ())
		return (noErr);

	resFile = CurResFile ();
	theError = CountVoices (&numVoices);
	for (index = numVoices; !theError && index > 0; --index) {
		theError = GetIndVoice (index, &voice);
		if (!theError)
			theError = GetVoiceDescription (&voice, &info, sizeof (VoiceDescription));
		if (!theError)
			MyAppendMenu (theMenu, info.name);
	}
	UseResFile (resFile);
	return (theError);
}


//
//	GetNewVoicePopupSmall
//
//		Gets a new voice popup menu control listing all available voices and sets the popup
//		selection to the current voice (or the default if the passed voice can't be found
//
//		Returns nil if the Speech Manager is not present or the control could not be created
//

ControlHandle GetNewVoicePopupSmall (short id, WindowPtr win, VoiceSpec *currentVoice)

{
	ControlHandle			theControl;
	MenuHandle				theMenu;
	VoiceDescription	info;
	OSErr							theError;
	short							resFile,
										item;
	Boolean						defaultVoice;

	theControl = nil;
	if (SpeechAvailable ()) {
		resFile = CurResFile ();
		if (theControl = GetNewControlSmall (id, win))
			if (theMenu = GetControlPopupMenuHandle(theControl)) {
				TrashMenu (theMenu, 1);
				if (!BuildVoiceMenu (theMenu)) {
					theError = GetVoiceDescriptionDefaultOkay (currentVoice, &info, sizeof (info), &defaultVoice);
					if (!theError && defaultVoice)
						*currentVoice = info.voice;
					item = FindItemByName (theMenu, info.name);
					SetControlMaximum (theControl, CountMenuItems (theMenu));
					MySetCtlValue (theControl, item ? item : 1);
				}
			}
		UseResFile (resFile);
	}
	return (theControl);
}


//
//	GetVoiceName
//
//		Retrieves the name of a given voice
//

OSErr GetVoiceName (VoiceSpec *voice, Str63 name)

{
	VoiceDescription	info;
	OSErr							theError;
	short							resFile;
	
	resFile = CurResFile ();
	theError = GetVoiceDescription (voice, &info, sizeof (VoiceDescription));
	UseResFile (resFile);
	if (!theError)
		PSCopy (name, info.name);
	else
		name[0] = 0;
	return (theError);
}

//
//	FindVoiceFromName
//
//		Iterate over all voices looking for a voice with a given name
//		Returns the default voice if none can be found
//

OSErr FindVoiceFromName (Str63 name, VoiceSpec *foundVoice)

{
	VoiceDescription	info;
	VoiceSpec					voice;
	OSErr							theError;
	short							numVoices,
										resFile,
										index;
	Boolean						found;

	if (!SpeechAvailable ())
		return (noErr);

	resFile = CurResFile ();
		
	found = false;
	theError = CountVoices (&numVoices);
	for (index = 1; !theError && !found && index <= numVoices; ++index) {
		theError = GetIndVoice (index, &voice);
		if (!theError)
			theError = GetVoiceDescription (&voice, &info, sizeof (VoiceDescription));
		if (!theError)
			if (found = StringSame (name, info.name))
				*foundVoice = info.voice;
	}
	if (!theError && !found)
		if (!(theError = GetVoiceDescription (nil, &info, sizeof (VoiceDescription))))
			*foundVoice = info.voice;

	UseResFile (resFile);
		
	return (theError);
}

//
//	SpeakMessage
//
//		Speaks one or more "parts" of a message using a particular voice (or the default voice
//		if a nil VoiceSpecPtr is passed).  The message parts can be spoken individually (by
//		passing a single selector) or collectively (by passing multiple selectors).  Currently,
//		the order that the parts will be spoken is hardcoded - which works well enough since we
//		currently support only a couple of selectors.
//
//		This routine accepts:
//					; a pointer to the voice to be used when speaking the message, or 'nil' if the
//						text is to be spoken using the default voice
//					; a handle specifying the TOC containing the message summary
//					; an index into the specified TOC identifying the summary or the message to be spoken
//					; selectors identifying the parts of the message to be spoken.  Currently, only
//						a handful of selectors are defined:  
//																								speakNothing  : mute!
//																								speakSender		: speaks the From line
//																								speakSubject	: speaks the subject line
//																								speakBody			: speaks the body of a message
//		And returns:
//					; noErr if the text is spoken successfully (or the Speech Manager is not present)
//					; Speech Manager errors
//

OSErr SpeakMessage (VoiceSpec *voice, TOCHandle tocH, short sumNum, SpeakableParts speak, Boolean nextMessage)

{
	QuoteTableHandle	quoteTable;
	Accumulator				textAccumulator;
	MyWindowPtr				messWin;
	WindowPtr					messWinWP;
	MessHandle				messH;
	Str255						scratch;
	UHandle						text;
	OSErr							theError;
	uLong							offset;
	Boolean						inUse;
	
	if (!SpeechAvailable ())
		return (noErr);

	UseFeature (featureSpeak);
	
	theError = noErr;
	if (speak) {
		inUse = false;
		if (messH = (*tocH)->sums[sumNum].messH)
			if ((*messH)->win)
				inUse = (*messH)->win->inUse;
		
		if(!(speak & speakSummary) || (speak & ~(speakSubject|speakSender|speakSummary)))
		{
			if (messWin = GetAMessage (tocH, sumNum, nil, nil, false))
				messH = Win2MessH (messWin);
		} else {
			messWin = nil;
		}

		// First, let's build the text we wish to speak.  We'll use an accumulator
		theError = AccuInit (&textAccumulator);

		// If we're speaking multiple messages, pause, and speak some sort of a "next message" phrase
		if (nextMessage) {
			if (!theError)
				theError = AccuComposeR (&textAccumulator, SPEAK_SILENCE_COMMAND, SPEAK_MESSAGE_PAUSE_DURATION);
			if (!theError && UseSpeakNewMessagePhrase) {
				(void) AccuAddRes (&textAccumulator, SPEAK_NEXT_MESSAGE);
				(void) AccuAddChar (&textAccumulator, ' ');
			}
		}
		
		// Add the 'To:' line to the text to be spoken
		if (speak & speakTo)
			theError = AccuAddAddressHeader (&textAccumulator, messH, nil, TO_HEAD, SPEAK_TO_PREFIX);
		
		// Add the 'From:' line to the text to be spoken 
		if (speak & speakSender) {
			LDRef (tocH);
			theError = AccuAddAddressHeader (&textAccumulator, messH, (*tocH)->sums[sumNum].from, FROM_HEAD, SPEAK_SENDER_PREFIX);
			UL (tocH);
		}
		
		// Add the 'Subject:' line to the text to be spoken
		if (!theError && (speak & speakSubject)) {
			if (speak & speakSender)
				theError = AccuAddStr (&textAccumulator, "\p  ");
			if (!theError) {
				scratch[0] = 0;
				if (messH)
					SuckHeaderText (messH, scratch, sizeof (scratch), SUBJ_HEAD);
				if (!scratch[0])
					PSCopy (scratch, (*tocH)->sums[sumNum].subj);
				TrimRe (scratch, true);
				TrimInitialWhite (scratch);
				TrimWhite (scratch);
				theError = AccuAddSpeakableText (&textAccumulator, &scratch[1], scratch[0], nil, false);
			}
		}
	
		// Add the body of the message
		if (!theError && (speak & speakBody)) {
			if (speak & (speakSender | speakSubject))
				theError = AccuAddStr (&textAccumulator, "\p.  ");
			
			// Add the raw text to the speech buffer
			if (!theError)
				if (text = MessVisibleText (messH)) {
					offset = BodyOffset (text);
					if (UseSpeakEmailRules) {
						quoteTable = BuildQuoteLevelTable (TheBody, offset);
						theError = AccuAddSpeakableText (&textAccumulator, LDRef (text) + offset, GetHandleSize (text) - offset, quoteTable, true);
						ZapHandle (quoteTable);
					}
					else
						theError = AccuAddPtr (&textAccumulator, LDRef (text) + offset, GetHandleSize (text) - offset);
					UL (text);
				}
		}

		// Start flapping our electronic lips
		if (!theError) {
			AccuTrim (&textAccumulator);
			MoveHHi (textAccumulator.data);
			theError = Speak (voice, LDRef (textAccumulator.data), textAccumulator.size);
		}
		AccuZap (textAccumulator);
		
		// Close the window, unless it is in the out mailbox
		if (messWin && !inUse) {
			messWinWP = GetMyWindowWindowPtr (messWin);
			if (!IsWindowVisible(messWinWP))
				CloseMyWindow (messWinWP);
		}
	}
	return (theError);
}


OSErr SpeakSelectedText (VoiceSpec *voice, PETEHandle pte)

{
	Accumulator	textAccumulator;
	Handle			text;
	OSErr				theError = noErr;
	long				selStart,
							selEnd;
					
	if (!SpeechAvailable ())
		return (noErr);

	// First, let's build the text we wish to speak.  We'll use an accumulator
	theError = AccuInit (&textAccumulator);
	
	if (!theError)
		theError = PeteGetTextAndSelection (pte, &text, &selStart, &selEnd);

	if (!theError)
		if (UseSpeakEmailRules)
			theError = AccuAddSpeakableText (&textAccumulator, LDRef (text) + selStart, selEnd - selStart, nil, false);
		else
			theError = AccuAddPtr (&textAccumulator, LDRef (text) + selStart, selEnd - selStart);

		// Start flapping our electronic lips
	if (!theError) {
		AccuTrim (&textAccumulator);
		MoveHHi (textAccumulator.data);
		theError = Speak (voice, LDRef (textAccumulator.data), textAccumulator.size);
	}
	AccuZap (textAccumulator);
	return theError;
}



pascal Boolean TalkingAlertFilter (DialogPtr dgPtr, EventRecord *event, short *item)

{
	Boolean retVal = false;
#ifdef THREADING_ON	
	if (NEED_YIELD)
		MyYieldToAnyThread ();
#endif

	if (event->what == mouseDown || event->what == keyDown) {
		gTalkingAlertTicks = 0;
	}

#ifdef CTB
	if (CnH)
		CMIdle (CnH);
#endif
	if (MiniMainLoop (event) || HasCommandPeriod ()) {
		MyStdFilterProc (dgPtr,event,item);
		return (true);
	}
	else if (event->what != nullEvent) 
		retVal = MyStdFilterProc (dgPtr, event, item);
	else {
			SpeechIdle ();
			if (gTalkingAlertTicks && gTalkingAlertTicks < TickCount ()) {
				(void) SpeakAlert (gTalkPhrase, gTalkError, gTalkExplain);
				gTalkingAlertTicks = 0;
			}
		}
	return retVal;
}


//
//	TalkingAlert
//
//		Called instead of StandardAlert to produce either a Talking Alert (an alert that
//		speak's it's text) or a Spoken Warning (passive speech without a visible alert).
//		If anything goes haywire and we had intended to speak a Talking Alert, we just
//		back off and produce a StandardAlert.
//
//		Note:  This routine dips into the Speech Preference file to retrieve various
//					 talking alert preferences.
//

void TalkingAlert (Boolean spokenWarning, AlertType alertType, StringPtr error, StringPtr explanation, AlertStdAlertParamPtr alertParam, short *item)

{
	TalkingAlertPrefHandle	talkingAlertPrefs;
	Str255									speakPhrase,
													displayError,
													speakError,
													displayExplanation,
													speakExplanation;
	Handle									phraseList;
	Ptr											p;
	OSErr										theError;
	short										oldResFile,
													refnum,
													delayTicks,
													numPhrases,
													index;
	Boolean									speechAvail;
	DECLARE_UPP (TalkingAlertFilter,ModalFilter);

	refnum						 = -1;
	gTalkError				 = nil;
	gTalkExplain			 = nil;
	gTalkPhrase				 = nil;
	gTalkingAlertTicks = 0;
	
	speechAvail = SpeechAvailable ();

	*displayError				= 0;
	*displayExplanation	= 0;
	*speakError					= 0;
	*speakExplanation		= 0;
	
	// Make sense of all these dang meta characters to build strings for both display and speech
	ParseTalkingAlertString (error, displayError, speakError);
	ParseTalkingAlertString (explanation, displayExplanation, speakExplanation);

	// If we were not able to parse a speakable error (like, for memory errors), don't talk!
	if (!*speakError)
		speechAvail = false;

	// Perform a last second memory check.  How we doin'?
	if (speechAvail)
		if (MonitorGrow (false) > 0)
			speechAvail = false;
		
	if (speechAvail) {
		// Open the Speech Preferences file
		oldResFile = CurResFile ();
		refnum = FSpOpenResFile (&SpeechPrefFileSpec, fsRdPerm);
		theError = ResError();
		if (!theError && refnum >= 0) {
			// Grab the Talking Alert preference from Speech Preferences
			if (talkingAlertPrefs = Get1Resource (rTalkingAlertResType, rTalkingAlertPrefResID)) {
				// Save the amount of time we should wait before speaking the alert
				delayTicks = (*talkingAlertPrefs)->delayTicks;
				
				// Setup the error and explanation strings, handle meta characters to build a
				// speakable string for each.
				if ((*talkingAlertPrefs)->speakAlertText) {
					gTalkError	 = speakError;
					gTalkExplain = speakExplanation;
				}
				
				// Get the alert phrase if the user's into that kind of thing
				if ((*talkingAlertPrefs)->speakPhrase)
					if (phraseList = Get1Resource (rTalkingAlertResType, rTalkingAlertPhraseListResID)) {
						BlockMoveData (*phraseList, &numPhrases, sizeof (short));
						switch ((*talkingAlertPrefs)->phraseModifier) {
							case kSpeakTheNextPhrase:
								index = gNextPhrase;
								if (++gNextPhrase > numPhrases)
									gNextPhrase = 1;
								break;
							case kSpeakARandomPhrase:
								index = (TickCount () % numPhrases) + 1;
								break;
							default:
								index = (*talkingAlertPrefs)->phraseIndex;
								break;
						}
						// Point to the first available phrase
						p = *phraseList + sizeof (short);
						// find the phrase we want
						while (--index)
							p = p + (*p + 1);
						PCopy (speakPhrase, p);
						gTalkPhrase = speakPhrase;
					}
					else
						speechAvail = false;
			}
			else
				speechAvail = false;
			CloseResFile (refnum);
		}
		else
			speechAvail = false;
		UseResFile (oldResFile);
	}
	
	if (speechAvail && (gTalkPhrase ||gTalkError || gTalkExplain)) {
		if (spokenWarning)
			SpeakAlert (nil, gTalkError, gTalkExplain);
		else {
			if (gTalkingAlertTicks < TickCount ())
				gTalkingAlertTicks = TickCount () + delayTicks;
			
			INIT_UPP (TalkingAlertFilter,ModalFilter);
			alertParam->filterProc = TalkingAlertFilterUPP;
			gTalkingAlertPresent = true;
			MyStandardAlert (alertType, displayError, displayExplanation, alertParam, item);
			gTalkingAlertPresent = false;
			SpeechShutup ();
		}
	}
	else
		if (!spokenWarning)
			MyStandardAlert (alertType, displayError, displayExplanation, alertParam, item);
}


//
//	ParseTalkingAlertString
//
//		Parse an alert string for meta characters, returning the text to be displayed in
//		the alert and the text to be spoken.  An "alert string" is a portion of a larger
//		formatted alert template; these being the "error" or "explanation" pieces.  The
//		format of an "alert string" is:
//
//				�displayText�spokenText
//
//		where both the '�' (option-l) and '�' (option-w) act as metacharacters to
//		control speakable aspects of the string.
//
//			'�'		...indicates that the text which follows is not to be read
//			'�'		...indicates that the text which follows is to be read, but not displayed
//
//		An example is knocking at the door... let's listen in!
//
//			�Something really complex happened that I can't explain�Just hit OK, trust me
//
//			Displays:  Something really complex happened that I can't explain
//
//			Speaks:		 Just hit OK, trust me
//

void ParseTalkingAlertString (StringPtr alertString, StringPtr displayError, StringPtr speakError)

{
	Accumulator	textAccumulator;
	Str255			scratch;
	UPtr				text;
	Byte				meta[2];
	
	meta[0] = 0xB7;		// '�'
	meta[1] = 0;
	
	if (*alertString) {
		text = alertString + (alertString[1] == 0xC2 ? 2 : 1);	// '�'
		PToken (alertString, displayError, &text, meta);
		if (alertString[1] != 0xC2)
			PCat (speakError, displayError);
		PToken (alertString, scratch, &text, meta);
		if (scratch[0]) {
			speakError[++speakError[0]] = ' ';
			PCat (speakError, scratch);
		}
	}
	if (!AccuInit (&textAccumulator)) {
		if (!AccuAddSpeakableText (&textAccumulator, &speakError[1], speakError[0], nil, false)) {
			AccuTrim (&textAccumulator);
			if (textAccumulator.size < sizeof (Str255)) {
				speakError[0] = textAccumulator.size;
				BlockMoveData (*(textAccumulator.data), &speakError[1], speakError[0]);
			}
		}
		AccuZap (textAccumulator);
	}
}

//
//	SpeakAlert
//
//		Speaks an alert text using the Speech control panel's Talking Alert setting.
//		If the 'spokenWarning' flag is set this is an alert spoken without an
//		actual alert.  As such, we should disregard the talking delay and speak
//		the alert immediately.
//

OSErr SpeakAlert (StringPtr phrase, StringPtr error, StringPtr explanation)

{
	OSErr	theError;
	
	theError = noErr;

	if (phrase)
		if (*phrase)
			theError = Speak (nil, &phrase[1], *phrase);
	if (error)
		if (*error)
			theError = Speak (nil, &error[1], *error);
	if (!theError && explanation)
		if (*explanation)
			theError = Speak (nil, &explanation[1], *explanation);
	gTalkingAlertTicks = 0;
	return (theError);
}


OSErr SpeakSelectedMessages (TOCHandle tocH)

{
	SpeakableParts	speak;
	OSErr						theError;
	short						sumNum;
	Boolean					nextMessage;
	
	
	theError		= noErr;
	nextMessage = false;
	if (tocH)
		for (sumNum = 0; !theError && sumNum < (*tocH)->count; ++sumNum)
			if ((*tocH)->sums[sumNum].selected) {
				speak = speakSubject | speakBody;
				speak |= (((*tocH)->which==OUT || (*tocH)->sums[sumNum].state == SENT || (*tocH)->sums[sumNum].state == UNSENT || (*tocH)->sums[sumNum].flags & FLAG_OUT) ? speakTo : speakSender);
				theError = SpeakMessage (nil, tocH, sumNum, speak, nextMessage);
				nextMessage = true;
			}
	return (theError);
}


#define	spek	true

//	Map the alphanumerics, spaces and tabs to be immediately speakable
#define	isspeakable(c)	(speakableMap[c])

//
//	AccuAddSpeakableText
//
//		Attempt to make a speech buffer a bit more readable...
//

OSErr AccuAddSpeakableText (AccuPtr a, UPtr textPtr, long len, QuoteTableHandle quoteTable, Boolean lookForQuotes)

{
	unsigned char	speakableMap[256] = {
	//  -0    -1    -2    -3    -4    -5    -6    -7    -8    -9    -A    -B    -C    -D    -E    -F
		   0,    0,    0,    0,    0,    0,    0,    0,    0, spek, spek,    0,    0, spek,    0,    0, // 0-
		   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 1-
		spek,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 2-
		spek, spek, spek, spek, spek, spek, spek, spek, spek, spek,    0,    0,    0,    0,    0,    0, // 3-
		   0, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, // 4-
		spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek,    0,    0,    0,    0,    0, // 5-
		   0, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, // 6-
		spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek,    0,    0,    0,    0,    0, // 7-
		spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, // 8-
		spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, // 9-
		   0,    0,    0,    0,    0,    0,    0, spek,    0,    0,    0,    0,    0,    0, spek, spek, // A-
		   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, spek, spek, // B-
		   0,    0,    0,    0,    0,    0,    0,    0,    0,    0, spek, spek, spek, spek, spek, spek, // C-
		   0,    0,    0,    0,    0,    0,    0,    0, spek, spek,    0,    0,    0,    0, spek, spek, // D-
		   0,    0,    0,    0,    0, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, spek, // E-
		   0, spek, spek, spek, spek,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0  // F-
	};

	Str31					prefix;
	Str8					tla;
	UPtr					p,
#ifdef G4_PROBLEM
								before_p,
#endif
								after_p,
								base,
								oldBase,
								endPtr,
								digitPtr,
								alphaPtr,
								upperPtr,
								lowerPtr,
								beginingOfLine;
	OSErr					theError;
	short					patternCount,
								quoteLevel,
								quoteLevelThisLine,
								numParagraphs,
								paragraphIndex;
	Boolean				processingSpeakableText,
								mightBeURL,
								wasTranslated;

	theError = noErr;
	
	base										= textPtr;
	beginingOfLine					= textPtr;
	patternCount						= 0;
	digitPtr								= nil;
	alphaPtr								= nil;
	upperPtr								= nil;
	lowerPtr								= nil;
	processingSpeakableText	= false;
	mightBeURL							= false;
	quoteLevel							= 0;
	paragraphIndex					= 0;
	
	numParagraphs = HandleCount (quoteTable);
	GetRString (prefix, QUOTE_PREFIX);
	
	p = textPtr;
	endPtr = textPtr + len;
	while (!theError && p < endPtr) {

		// If we're at the beginning of a line...
		if (lookForQuotes)
			if (p == beginingOfLine && (UseSpeakFindQuote || !UseSpeakQuotes)) {

				// Calculate the quote level for this line: from the quote table, by looking for quote characters... or, both!
				quoteLevelThisLine = GetQuoteLevel (p, endPtr, textPtr, quoteTable, quoteLevel, numParagraphs, prefix, &paragraphIndex, &p);
			
				// Save off the text between the base and the begining of the line, and move the base
				// to point to the first character beyond the conclusion of the quoting character.
				theError = AccuAddPtr (a, base, beginingOfLine - base);
				base = p;

				// If we're not planning on speaking quotes, skip all of the text until the next return,
				// setting the base to point to the character following the return.  Note that we leave
				// 'p' pointing to the return...  This will be caught below during our character 'switch',
				// with the new beginingOfLine set there.
				if (!theError)
					if (!UseSpeakQuotes) {
						while (p < endPtr && *p != returnChar)
							++p;
						base = p + 1;		// Note 
					}
					else {
						// We're increasing the quote level...
						if (quoteLevelThisLine > quoteLevel) {
							// Add the quote phrase
							if (UseSpeakQuotePhrase)
								(void) AccuAddQuoteStr (a, SPEAK_QUOTE);
							// Use a slightly modified voice for the quoted text
							if (UseSpeakModifyQuoteVoice)
								(void) AccuAddRes (a, SPEAK_QUOTE_MODIFY_VOICE_COMMAND);
						}

						// We're decreasing the quote level...
						if (quoteLevelThisLine < quoteLevel) {
							// Reset the voice characteristics once the quote level returns to normal
							if (UseSpeakModifyQuoteVoice)
								(void) AccuAddRes (a, SPEAK_UNQUOTE_MODIFY_VOICE_COMMAND);
							// Add the unquote phrase
							if (UseSpeakQuotePhrase)
								(void) AccuAddQuoteStr (a, SPEAK_UNQUOTE);
						}
					}
				quoteLevel = quoteLevelThisLine;
			}
			
		// Get fancy only once we've moved past the beginning of a new run of text (the base)
		if (!theError && p > base) {
			// If we are currently proccessing speakable text...
			if (processingSpeakableText) {
				oldBase = base;
				// When we are processing a string of digits...
				if (digitPtr) {
					// Check to see if we're making an numeric-to-alpha transition "123abc"
					if (UseSpeakMixedCase && isalpha (*p) && !NumberShorthand (p))
						theError = InsertAndMoveBase (a, p, " ", 1, &base);
				}
				else
				
				// When we are processing a string of alphas...
				if (alphaPtr) {
					// Check to see if we're making an alpha-to-numeric transition "abc123"
					if (isdigit (*p)) {
						if (UseSpeakMixedCase)
							theError = InsertAndMoveBase (a, p, " ", 1, &base);
					}
					else

					// When we are processing a string of uppers...
					if (upperPtr) {
						// Check to see if we are transitioning from an uppercase string
						// Possibilities include:
						//		1. John			- (lower follows) Simple transition to lowercase, ignore it
						//		2. URLs			- (lower follows) Plural!  We shouldn't do anything.
						//		3. ATTNet		- (lower follows) Tricky! Should be translated to ATT Net
						//		4. THXsound	- (lower follows) Tricky! Should be translated to THX sound
						//		5. AFAIK		- (non-alpha, non-digit) might be an acronym that needs to be replaced
						//		6. URL's		- (apostrophe) should be translated to URLs -- but might be an acronym!
						//		and, probably... more!
						if (!isupper (*p)) {
							// Handle lowercase transitions (cases 1, 2, 3 and 4)
							if (islower (*p)) {
								// If the uppercase string is more than one character (cases 2, 3 and 4) 
								if (p - upperPtr > 1)
									// If what follows is NOT just pluralization (cases 3 and 4)
									if (!(*p == 's' && (p == endPtr - 1 || (p + 1 < endPtr && !isalnum (*(p + 1)))))) {
										if (UseSpeakStrictUpperToLower)
											// Case 4
											theError = InsertAndMoveBase (a, p, " ", 1, &base);
										else
											if (UseSpeakMixedCase) {
												// Case 3
												theError = AccuAddPtr (a, base, p - base - 1);
												if (!theError)
													theError = AccuAddChar (a, ' ');
												base = p - 1;
											}
									}
							}
							else
							
							// Handle an apostrophe (case 6)
							// Strip the apostrophe if it is a pluralized all caps word
							if (*p == '\'' || *p == '�') {
								if (p + 1 < endPtr && (*(p + 1) == 's' || *(p + 1) == 'S') && (p + 1 == endPtr - 1 || (p + 2 < endPtr && !isalnum (*(p + 2))))) {
									theError = AccuAddPtr (a, base, p - base);
									base = p + 1;
								}
							}
							else {
								// Handle everything else (case 5)
								// Flush everything up to the first character in the uppercase string
								theError = AccuAddPtr (a, base, upperPtr - base);
								oldBase = base = upperPtr;
								
								// Check to see if we have an acronym and replace it 
								if (UseSpeakAcronyms && !theError) {
									MakePStr (tla, upperPtr, p - upperPtr);
									theError = AccuAddAcronym (a, tla, &wasTranslated);
									if (wasTranslated)
										base = p;
								}
							}
						}
					}
					else
					
					// When we are processing a string of lowers...
					if (lowerPtr) {
						// Check to see if we're making a lower-to-upper transition "johnPurlia"
						if (UseSpeakMixedCase && isupper (*p))
							theError = InsertAndMoveBase (a, p, " ", 1, &base);
					}
				}
				
				// Flush our buffer if we've hit something unspeakable and we haven't already handled
				// one of the above cases
				if (!isspeakable (*p) && oldBase == base) {
					theError = AccuAddPtr (a, base, p - base);
					base = p;
				}
			}
			
			// Looks like we've been processing a run of unspeakable text
			// Replace runs of 3 or more non-speakable characters with a period and a space.
			// Otherwise, keep what we found.
			else
				if (UseSpeakEliminateTheUnspeakable && isspeakable (*p)) {
					if (p - base >= 3)
						theError = AccuAddStr (a, "\p. ");
					else
						theError = AccuAddPtr (a, base, p - base);
					base = p;
				}
		}
		
		// Handle certain characters with a little care...
		if (!theError)
			switch (*p) {
				case returnChar:
					// If we've hit the end of a line that did not terminate with a period, see if
					// we might want to insert a pause of our own.
					if (UseSpeakShortLines)
						if ((p - beginingOfLine < 48 || (p - beginingOfLine < 64 && (p + 1 < endPtr && isupper (*(p + 1)))))
							  && p > textPtr && *(p - 1) != returnChar && !ispunct (*(p - 1)))
							theError = InsertAndMoveBase (a, p, ". ", 2, &base);

					// Mark the next character as the beginning of a new line
					beginingOfLine = p + 1;
					break;
				case periodChar:
					// If the period breaks between two words, insert the word "dot".  (qualcomm.com)
					// Anything that is ".edu" should be changed to ".EDU"
					// Likewise, ".fourorlesschars." should be changed to ".uppercase."
					// If the period breaks between a word and a number, insert the word "dot".  (john316.com)
					// If the period breaks between two numbers, things get interesting...
					// 		We might have to deal with (john316.98degrees.com, 129.46.138.16, 21.2, $19.99, 8.5.1)
#ifdef G4_PROBLEM
					if (UseSpeakDotty)
						if (p > textPtr && p + 1 < endPtr)
							if ((isalpha (*(p - 1)) && isalnum (*(p + 1))) ||
									(isalnum (*(p - 1)) && isalpha (*(p + 1)))) {
								mightBeURL = true;
								theError = AccuAddRes (a, SPEAK_DOT);
								base = p + 1;
								if (p + 4 <= endPtr && !memcmp (p + 1, "edu", 3) && (p + 4 == endPtr || !isalnum (*(p + 4)))) {
									theError = InsertAndMoveBase (a, p, " EDU", 4, &base);
									base = p + 4;
									p += 3;
								}
								else
									// Weird logic to determine whether or not a segment in a dodted address _might_ be
									// an acronym (for example, "www.ucsd.edu").  We want the Speech Manager to speak
									// such things as letters -- "w w w dot U C S D dot E D U"
									if (p + 5 < endPtr && (*(p + 4) == periodChar || *(p + 5) == periodChar))
										if (memcmp (p + 1, "com", 3) && memcmp (p + 1, "org", 3) &&
												memcmp (p + 1, "net", 3) && memcmp (p + 1, "mil", 3) &&
												memcmp (p + 1, "gov", 3)) {
											for (++p; !theError && *p != periodChar; ++p)
												theError = AccuAddChar (a, islower (*p) ? toupper (*p) : *p);
											base = p;
											--p;
										}
							}
							else
								if (isdigit (*(p - 1)) && isdigit (*(p + 1))) {
									// Scan backwards and forwards until we hit things that aren't digits
									before_p = p - 1;
									while (before_p >= textPtr && isdigit (*before_p))
										--before_p;
									after_p = p + 1;
									while (after_p < endPtr && isdigit (*after_p))
										++after_p;
									// If we hit a letter or another period -- in either direction, let's just dot it!
									if (isalpha (*before_p) || *before_p == periodChar || isalpha (*after_p) || *after_p == periodChar) {
										theError = AccuAddRes (a, SPEAK_DOT);
										base = p + 1;
									}
								}
#endif
					break;
				case '/':
					// If we hit a slash in a URL, skip the rest of the URL
					if (UseSpeakTruncUrl && mightBeURL) {
						theError = AccuAddPtr (a, base, p - base);
						if (!theError) {
							while (p < endPtr && !isspace (*p))
								++p;
							base = p;
							--p;	// Because it will be incremented at the end of the loop
						}
					}
					break;
				case ':':
					// If a colon is the last character on a line, replace it with a period (full stop)
					if (p + 1 < endPtr && *(p + 1) == returnChar && !mightBeURL) {
						theError = AccuAddPtr (a, base, p - base);
						if (!theError)
							theError = AccuAddChar (a, periodChar);
						base = p + 1;
					}
						
					// If we might be working on a URL, save what we have and skip past any '/' characters
					if (mightBeURL) {
						theError = AccuAddPtr (a, base, p - base + 1);
						if (!theError) { 
							++p;
							while (p < endPtr && *p == '/')
								++p;
							base = p;
							--p;	// Because it will be incremented at the end of the loop
						}
					}
					break;
				case '<':
					if (UseSpeakAttemptUrl) {
						after_p = p + 1;
						while (after_p < endPtr && (*after_p != ':' && !isspace (*after_p)))
							++after_p;
						if (mightBeURL = (*after_p == ':')) {
							theError = AccuAddPtr (a, base, p - base);
							base = p + 1;
						}
					}
					break;
				case '>':
					if (UseSpeakAttemptUrl && mightBeURL) {
						theError = AccuAddPtr (a, base, p - base);
						base = p + 1;
					}
					break;
				case 'E':
				case 'e':
					// How embarrassing... "email" is generally spoken incorrectly.  Let's fix that.
					if (UseSpeakJohnnyCantRead)
						if (p >= textPtr && p + 4 < endPtr)
							if (!memcmp (p + 1, "mail", 4))
								theError = InsertAndMoveBase (a, p + 1, " ", 1, &base);
					break;
				case '*':
				case '_':
					// Look for creative use of ASCII emphasis... You are a _total_ moron.  If we find something
					// like this, let's tell the speech manager to emphasize the word.  (We may also want to do
					// this for words in all caps, but I suspect that would be problematic.)
					if (UseSpeakEmphasis && p > textPtr && isspace (*(p - 1))) {
						after_p = p + 1;
						while (after_p < endPtr && isalpha (*after_p))
							++after_p;
						if (*after_p == *p) {
							theError = AccuAddPtr (a, base, p - base);
							if (!theError)
								theError = AccuAddChar (a, ' ');
							if (!theError)
								theError = AccuAddStr (a, "\p[[emph +]]");
							++p;
							while (!theError && p < endPtr && isalpha (*p)) {
								theError = AccuAddChar (a, isupper (*p) ? tolower (*p) : *p);
								++p;
							}
							if (!theError)
								theError = AccuAddChar (a, ' ');
							base = p + 1;
						}
					}
					break;
				case 'w':
				case 'W':
					// Look for the shorthand use of 'w' to mean 'with'
					if (!UseSpeakShorthand)
						if (p > textPtr && isspace (*(p - 1)) && (p + 1 < endPtr && *(p + 1) == '/') && (p + 2 < endPtr && isalnum(*(p + 2)))) {
							theError = AccuAddPtr (a, base, p - base);
							if (!theError)
								theError = AccuAddChar (a, ' ');
							if (!theError)
								theError = AccuAddRes (a, SPEAK_WITH);
							if (!theError)
								theError = AccuAddChar (a, ' ');
							base = (p += 2);
						}
					break;
			}
		processingSpeakableText = isspeakable (*p);

		digitPtr = !isdigit (*p) ? nil : (digitPtr ? digitPtr : p);
		alphaPtr = !isalpha (*p) ? nil : (alphaPtr ? alphaPtr : p);
		upperPtr = !isupper (*p) ? nil : (upperPtr ? upperPtr : p);
		lowerPtr = !islower (*p) ? nil : (lowerPtr ? lowerPtr : p);
		
		if (isspace (*p))
			mightBeURL = false;
		++p;
	}
	// Finish up by appending the remainder of the line if we were working on speakable text
	if (!theError && processingSpeakableText) {
		// Special case if we're doing uppercase processing at the end (might have an acronym)
		if (UseSpeakAcronyms && upperPtr) {
			theError = AccuAddPtr (a, base, upperPtr - base);
			base = upperPtr;
			
			// Check to see if we have an acronym and replace it 
			if (UseSpeakAcronyms && !theError) {
				MakePStr (tla, upperPtr, p - upperPtr);
				theError = AccuAddAcronym (a, tla, &wasTranslated);
				if (wasTranslated)
					base = p;
			}
		}
		if (!theError)
			theError = AccuAddPtr (a, base, p - base);
	}
	if (!theError && lookForQuotes && quoteLevel && UseSpeakQuotePhrase) {
		theError = AccuAddStr (a, "\p[[rset]]");
		if (!theError)
			theError = AccuAddQuoteStr (a, SPEAK_UNQUOTE);
	}
	return (theError);
}


OSErr AccuAddQuoteStr (AccuPtr a, short resID)

{
	OSErr	theError;
	
	theError = AccuComposeR (a, SPEAK_SILENCE_COMMAND, SPEAK_QUOTE_PAUSE_DURATION);
	if (!theError)
		theError = AccuAddRes (a, resID);
	if (!theError)
		theError = AccuAddStr (a, "\p.  ");
	return (theError);
}


//
//	NumberShorthand
//
//		Hard coded to only work for English... sorry.
//

Boolean NumberShorthand (UPtr p)

{
	if (memcmp (p, "st", 2))					// 1st
		if (memcmp (p, "nd", 2))				// 2nd
			if (memcmp (p, "rd", 2))			// 3rd
				if (memcmp (p, "th", 2))		// 4th
					return (false);
	return (!isalnum(*(p + 2)));
}


QuoteTableHandle BuildQuoteLevelTable (PETEHandle pte, uLong offset)

{
	PETEParaInfo	pinfo;
	Accumulator		table;
	QuoteTableRec	quoteInfo;
	OSErr					theError;
	short					paragraphIndex;

	theError = AccuInit (&table);
	for (paragraphIndex = 1; !theError; paragraphIndex++) {
		Zero (pinfo);
		if (PETEGetParaInfo (PETE, pte, paragraphIndex, &pinfo))
			break;

		if (pinfo.paraOffset >= offset) {
			Zero (quoteInfo);
			quoteInfo.quoteLevel = pinfo.quoteLevel;
			quoteInfo.paraOffset = pinfo.paraOffset - offset;
			theError = AccuAddPtr (&table, &quoteInfo, sizeof (quoteInfo));
		}
	}
	if (theError)
		AccuZap (table);
	else
		AccuTrim (&table);
	return ((QuoteTableHandle) table.data);
}


//
//	GetQuoteLevel
//

short GetQuoteLevel (UPtr p, UPtr endPtr, UPtr textPtr, QuoteTableHandle quoteTable, short quoteLevel, short numParagraphs, Str31 prefix, short *paragraphIndex, UPtr *speakAbleText)

{
	QuoteTablePtr	qtPtr;
	UPtr					q;
	short					quoteLevelThisLine;
	
	// First, see if the current pointer matches that in the quote table, and use that -- otherwise, the level has not changed
	if (quoteTable && numParagraphs) {
		qtPtr = *quoteTable;
		if (*paragraphIndex < numParagraphs && textPtr + qtPtr[*paragraphIndex].paraOffset == p)
			quoteLevelThisLine = qtPtr[(*paragraphIndex)++].quoteLevel;
		else
			quoteLevelThisLine = *paragraphIndex < numParagraphs ? quoteLevel : 0;
	}
	else
		quoteLevelThisLine = 0;
	
	// Next, let's check to see if the message contains quote prefixes (which _could_ be separated by white space)
	if (prefix[0]) {
		q = p;
		while (q < endPtr - prefix[0] && !memcmp (q, &prefix[1], prefix[0])) {
			++quoteLevelThisLine;
			p = (q += prefix[0]);
			// Skip any spaces or tabs
			while (q < endPtr && *q == ' ' || *q == tabChar)
				++q;
		}
		*speakAbleText = p;
	}
	return (quoteLevelThisLine);
}


//
//	InsertAndMoveBase
//
//		Flushes those charcters from the base to the pointer, inserts text, then
//		moves the base to the pointer location.
//

OSErr InsertAndMoveBase (AccuPtr a, UPtr p, UPtr text, long length, UPtr *base)

{
	OSErr	theError;
		
	theError = AccuAddPtr (a, *base, p - *base);
	if (!theError && text)
		theError = AccuAddPtr (a, text, length);
	*base = p;
	return (theError);
}


//
//	SmartAddressSpeaking
//

PStr SmartAddressSpeaking (PStr scratch)

{
	Str63	dot;
	UPtr	atPtr;
	short	i;
	
	atPtr = PIndex (scratch, '@');
	for (i = 2; i < scratch[0]; ++i) {
		if (((isdigit (scratch[i - 1]) && !isdigit (scratch[i])) ||
				(!isdigit (scratch[i - 1]) && isdigit (scratch[i]))) && scratch[i] != periodChar && scratch[i] != '@')
			PInsert (scratch, sizeof (scratch), "\p ", &scratch[i++]);
		else
			if (scratch[i] == periodChar)
				if (&scratch[i] < atPtr)
						scratch[i] = ' ';
	}
	PReplace (scratch, "\p.", GetRString (dot, SPEAK_DOT));
	return (scratch);
}


//
//	AccuAddAddressHeader
//
//		Added so that we can speak either To or From fields.
//

OSErr AccuAddAddressHeader (AccuPtr a, MessHandle messH, PStr substitute, short headerIndex, short headerResID)

{
	Str255	scratch;
	OSErr		theError;
	
	theError = noErr;
	
	// Grab the header text from the message, clean it up a bit and (maybe) substitute other text
	scratch[0] = 0;
	if (messH) {
		SuckHeaderText (messH, scratch, sizeof (scratch), headerIndex);
		BeautifyFrom (scratch);
	}
	if (!scratch[0] && substitute)
		PSCopy (scratch, substitute);

	if (scratch[0]) {
		theError = AccuAddRes (a, headerResID);

		// Be tricky and make addresses almost speakable!
		//		� Place a space around strings of numbers "john316" beccomes "john 316"
		//		� Replace periods with a spoken "dot" so that we get "devseed at apple dot com"
		//		  However, if the period is before the @, put in a space instead.  Thus,
		//		  "john.purlia@qualcomm.com" becomes "john purlia at qualcomm dot com"
		if (!theError) {
			if (!PrefIsSet (PREF_SPEAK_NO_NICE_ADDRESSES))
				SmartAddressSpeaking (scratch);
			theError = AccuAddStr (a, scratch);
		}
		if (!theError)
			theError = AccuAddChar (a, periodChar);
	}
	return (theError);
}


//
//	AccuAddAcronym
//
//		Lookup an acronym in the resource fork and add its spoken equivalent
//

OSErr AccuAddAcronym (AccuPtr a, PStr tla, Boolean *wasTranslated)

{
	AcronymHandle	acronym;
	OSErr					theError;
	
	theError = noErr;
	if (acronym = GetNamedResource (rAcronymResType, tla)) {
		LDRef (acronym);
		theError = AccuAddStr (a, (*acronym)->spokenAs);
		ReleaseResource (acronym);
		*wasTranslated = true;
	}
	else
		*wasTranslated = false;
	return (theError);
}


//
//	InstallPronunciationDictionary
//
//		Installs a user Pronunciation Dictionary on the speech channel.
//		Pass in the string ID of the dictionary to be used (which must be
//		installed in the Eudora Stuff folder).
//

OSErr InstallPronunciationDictionary (SpeechChannel channel, short resID)

{
	Handle	dictionary;
	FSSpec	dictionarySpec;
	Str31		filename;
	OSErr		theError;

	dictionary = nil;
	theError = FindUserPronunciationDictionary (GetRString (filename, resID), &dictionarySpec);
	if (!theError)
		theError = CreateSpeechDictionary (&dictionarySpec, &dictionary);
	if (!theError)
		theError = UseDictionary (channel, dictionary);
	if (dictionary)
		DisposeHandle (dictionary);
	return (theError);
}	


OSErr FindUserPronunciationDictionary (Str31 filename, FSSpec *dictionarySpec)

{
	Str31	folderName;
	OSErr	theError;
	
	theError = GetFileByRef (AppResFile, dictionarySpec);
	if (!theError)
		theError = FSMakeFSSpec (dictionarySpec->vRefNum, dictionarySpec->parID, GetRString (folderName, STUFF_FOLDER), dictionarySpec);
	if (!theError) {
		IsAlias (dictionarySpec, dictionarySpec);
		theError = FSMakeFSSpec (dictionarySpec->vRefNum, SpecDirId (dictionarySpec), filename, dictionarySpec);
	}
	return (theError);
}


OSErr CreateSpeechDictionary (FSSpec *dictionarySpec, Handle *dictionary)

{
	DictHeaderRec						dictHeader,
													*dictHeaderPtr;
	DictEntryHeaderRec			dictEntryHeader;
	Accumulator							dictAccumulator;
	LineIOD									lid;
	Str255									format,
													word,
													phoneme;
	OSErr										theError;
	UInt32									numEntries;
	long										length;
	
	Zero (dictAccumulator);
	numEntries = 0;
	
	// Open the pronunciation dictionary
	theError = OpenLine (dictionarySpec->vRefNum, dictionarySpec->parID, dictionarySpec->name, fsRdWrPerm, &lid);
	
	// Read the first line (which should contain the file format -- which I currently ignore)
	if (!theError)
		(void) GetLine (format + 1, sizeof (format) - 2, &length, &lid);

	if (!theError)
		theError = AccuInit (&dictAccumulator);
	
	// Add a speech dictionary header
	if (!theError) {
		Zero (dictHeader);
		dictHeader.atom			= kDictionaryAtom;
		dictHeader.version	= kDictionaryAtomVersion;
		dictHeader.script		= smRoman;
		dictHeader.language	= langEnglish;
		dictHeader.region		= verUS;
		GetDateTime (&dictHeader.modDate);
		theError = AccuAddPtr (&dictAccumulator, &dictHeader, sizeof (dictHeader));
	}
	
	while (!theError) {
		theError = GetSpeechDictionaryLine (&lid, word, phoneme);
		// Add a dictionary entry header
		if (!theError) {
			++numEntries;
			dictEntryHeader.length		= sizeof (dictEntryHeader) + 2 * sizeof (DictEntryFieldHeaderRec) + word[0] + phoneme[0] + (word[0] % 2) + (phoneme[0] % 2);
			dictEntryHeader.type			= pronunciationEntry;
			dictEntryHeader.numFields	= 2;
			theError = AccuAddPtr (&dictAccumulator, &dictEntryHeader, sizeof (dictEntryHeader));
		}
		// Add a the 'word' as a field entry
		if (!theError)
			theError = AccuAddDictionaryEntryField (&dictAccumulator, word, textEntryField);
		// Add a the 'phoneme' as a field entry
		if (!theError)
			theError = AccuAddDictionaryEntryField (&dictAccumulator, phoneme, phonEntryField);
	}
	if (theError == eofErr)
		theError = noErr;
		
	// Done!!  trim the accumulator and go back and fill in the 'DictHeaderRec'
	// length and count fields
	if (!theError) {
		AccuTrim (&dictAccumulator);
		dictHeaderPtr = *dictAccumulator.data;
		dictHeaderPtr->length			= dictAccumulator.size;
		dictHeaderPtr->numEntries = numEntries;
	}
	
	CloseLine (&lid);
	*dictionary = dictAccumulator.data;
	
	return (theError);
}


OSErr AccuAddDictionaryEntryField (AccuPtr a, PStr string, SInt16 type)

{
	DictEntryFieldHeaderRec	header;
	OSErr										theError;
	
	// Add a dictionary entry field header
	header.length = sizeof (header) + string[0];
	header.type		= type;
	theError = AccuAddPtr (a, &header, sizeof (header));
	if (!theError)
		theError = AccuAddPtr (a, &string[1], string[0]);
	if (!theError && (string[0] % 2))
		theError = AccuAddChar (a, 0);
	return (theError);
}

OSErr GetSpeechDictionaryLine (LineIOP lip, PStr word, PStr phoneme)
{
	Str255 	line;
	UPtr		spot;
	OSErr		theError;
	long		length;
	int			code;
	char		delims[2];
	
	theError = noErr;
	do {
		code = GetLine (line + 1, sizeof (line) - 2, &length, lip);
		if (code < 0)
			theError = code;
		if (!code || !length)
			theError = eofErr;
		if (!theError)
			if (line[line[0] = length] != returnChar)
				theError = SPEAK_DICTIONARY_BAD;

	} while (!theError && line[1] == '#');
	
	if (!theError) {
		delims[0] = ' ';
		delims[1] = 0;
		spot = line + 1;	
		PToken (line, word, &spot, delims);
		delims[0] = returnChar;
		PToken (line, phoneme, &spot, delims);
		if (!word[0] || !phoneme[0])
			theError = SPEAK_DICTIONARY_BAD;
	}
	return (theError);
}


#ifdef NOT_YET_THIS_WILL_BE_SUPPORTED_AT_SOME_POINT_IN_THE_FUTURE
OSErr InstallPureVoiceOutputComponent (SpeechChannel chan)

{
	ComponentDescription	outputDevice;
	Component							theComponent;
	OSErr									theError;
	
	// First, find the output component in the Eudora Folder
	
	theError = noErr;
	outputDevice.componentType = kSoundOutputDeviceType;
	outputDevice.componentSubType = 'XPVC';
	outputDevice.componentManufacturer = 'QCOM';
	outputDevice.componentFlags = 0;
	outputDevice.componentFlagsMask = 0;
	
	if (theComponent = FindNextComponent (0, &outputDevice))
		theError = SetSpeechInfo (chan, soSoundOutput, &theComponent);
	return (theError);
}
#endif



#endif