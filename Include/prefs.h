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

#ifndef PREFS_H
#define PREFS_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
PStr PrefHelpString(short pref, PStr string,Boolean geeky);
void DoAboutUIUCmail(void);
Boolean PrefIsSet(short pref);
Boolean PrefEnabled(short pref);
short SettingsPtr(ResType type,UPtr name,short id,UPtr dataPtr, long dataSize);
short SettingsHandle(ResType type,UPtr name,short id,UHandle	dataHandle);
#define GetPOPInfo(u,h) GetPOPInfoLo(u,h,nil)
#define GetSMTPInfo(h) GetSMTPInfoLo(h,nil)
OSErr GetPOPInfoLo(UPtr user, UPtr host,long *port);
UPtr GetSMTPInfoLo(UPtr host,long *port);
UPtr GetReturnAddr(UPtr addr,Boolean comments);
UPtr GetShortReturnAddr(UPtr addr);
Boolean GetVolpath(UPtr volpath,UPtr volName,long *dirId);
void SaveBoxLines(void);
OSErr GetAttFoldSpec(FSSpecPtr spec);
Boolean GetHexPath(UPtr folder,UPtr volpath);
OSErr SetPref(short pref,PStr value);
void SetPrefLong(short prefNum,long val);
OSErr SetStrOverride(short pref, PStr value);
OSType FetchOSType(short index);
OSType DefaultCreator(void);
#define GetAttFolderSpec(spec) (*(spec)=AttFolderSpec,noErr)
#define GetCurrentAttFolderSpec(spec) (*(spec)=CurrentAttFolderSpec,noErr)
OSErr GetAttFolderSpecLo(FSSpecPtr spec);
#define PrefIsSetOrNot(pref,mods,rev)	\
	((mods&rev) ? !PrefIsSet(pref) : PrefIsSet(pref))
PStr GetPref(PStr string, short prefNum);
PStr GetDefPref(PStr string, short prefNum);
long GetPrefLong(short prefNum);
Boolean TogglePref(short pref);
Boolean AutoCheckOK(void);
Boolean Mom(short yesId,short noId,short prefId,short fmt,...);
Boolean Wife(short buttonID, short prefId, short fmt, ...);
PStr GetPrefNoDominant(PStr string, short prefNum);
Boolean GetPrefBitNoDominant(short prefNum, long mask);
PStr GetRealname(PStr name);
PStr GetPOPPref(PStr popAcct);
void DoPersSettings(PersHandle pers);
void SetStupidStuff(void);
void HesiodKill(void);
long GetSMTPPort(void);
Boolean ShouldSMTPAuth(void);

long SigValidate(long sigID);
#define GetPrefBit(pref,mask)	((GetPrefLong(pref)&mask)!=0)
#define SetPrefBit(pref,mask)	SetPrefLong(pref,GetPrefLong(pref)|mask)
#define ClearPrefBit(pref,mask)	SetPrefLong(pref,GetPrefLong(pref)&~mask)

void 	     CheckForDefaultMailClient ();

// (jp) Gee guys, thanks for changing the name of KCReleaseItemRef in Universal Headers 3.4...
#define	KCReleaseItemRef	KCReleaseItem
#ifdef HAVE_KEYCHAIN
#define KeychainAvailable() (KeychainManagerAvailable() && KCAddInternetPassword && KCFindInternetPassword && KCDeleteItem && KCReleaseItemRef)
OSErr UpdatePersKCPassword(PersHandle pers);
OSErr FindPersKCPassword(PersHandle pers,PStr password,uLong passStringLen);
OSErr DeletePersKCPassword(PersHandle pers);
OSErr AddPersKCPassword(PersHandle pers,PStr password);
#else //HAVE_KEYCHAIN
#define KeychainAvailable() (false)
#endif //HAVE_KEYCHAIN

typedef enum
{
	setDITLBase = 2000,
	setDITLBasic = 2001,
	setDITLLimit = 3000,
	setDITLProOnly = 3000,
} SettingsDITLEnum;

typedef enum
{
	popRUIDL=1,
	popRStatus,
	popRLast
} POPSettingEnum;

typedef enum
{
	ssBloat,	// send multipart/alternative
	ssStrip,	// send plaintext
	ssStyle,	// send just style
	ssLimit
} SendStyleEnum;

void DoSettings(SettingsDITLEnum startingWith);
OSErr ZapSettingsResource(OSType type, short id);

// Preview stuff
#define PreviewReadTimer (!(GetPrefLong(PREF_NO_PREVIEW_READ)&1))
#define PreviewReadFocus (!(GetPrefLong(PREF_NO_PREVIEW_READ)&2))
#define PreviewReadNext (!(GetPrefLong(PREF_NO_PREVIEW_READ)&4))
#define PreviewReadDelete (!(GetPrefLong(PREF_NO_PREVIEW_READ)&8))


#define PERS_PREF	100
#define PERS_POPUP	1
#define PERS_NEW		2
#define PERS_REMOVE	3
#define PERS_NAME		4
#define	PERS_VALIDATE	5

#define SETTINGSLISTKEY

#define kHourUseMask			0x80000000	// pay attention to the check hours pref
#define kHourAlwaysWeekendMask	0x40000000	// ignore on weekends
#define kHourNeverWeekendMask	0x20000000	// never check on weekends
#define kHourUseWeekendMask	0x10000000	// check hours pref on weekends, too

#define kFlowMask	1L	// don't bother with flow at all
#define kFlowInMask	2L	// don't interpret flow
#define kFlowOutMask	4L	// don't send flow at all
#define	kFlowInExcerptMask	8L	// don't change flowed quotes into excerpt bars
#define kFlowOutExcerptMask	16L	// don't change messages with excerpt only into flow

#define UseFlow	(((M_T1=GetPrefLong(PREF_FLOWED_BITS))&kFlowMask)==0)
#define UseFlowIn	(UseFlow && (M_T1&kFlowInMask)==0)
#define UseFlowOut	(UseFlow && (M_T1&kFlowOutMask)==0)
#define UseFlowInExcerpt	(UseFlowIn && (M_T1&kFlowInExcerptMask)==0 && !(GetPrefLong(PREF_INTERPRET_PARA)&peQuoteLevelValid))
#define UseFlowOutExcerpt	(UseFlowOut && (M_T1&kFlowOutExcerptMask)==0 && !(GetPrefLong(PREF_INTERPRET_PARA)&peQuoteLevelValid))

#define DragPrefSumImmediate	((GetPrefLong(PREF_DRAG_OPTIONS)&1)!=0)

//MJD #define UseListColors (PrefIsSet(PREF_USE_LIST_COLORS))
#define UseListColors (false)

#define	kSpeakEmailRulesMask								1L		// Don't apply email specifc rules to speaking (trust the Speech Manager to mangle things)
#define	kSpeakEliminateTheUnspeakableMask		2L		// Don't eliminate strings of unspeakable characters (like *****, -----, etc -- which the Speech Manager is very happy to speak)
#define	kSpeakMixedCaseMask									4L		// Don't separate mixed case and alpha/numeric strings with a breaking space
#define	kSpeakFindQuoteMask									8L		// Don't attempt to find and eliminate quoting prefixes (let the Speech Manager struggle through the >'s)
#define	kSpeakQuotePhraseMask							 16L		// Don't speak a phrase at the start and end of quoted blocks
#define	kSpeakNewMessagePhraseMask				 32L		// Don't speak a phrase between messages read from a mailbox
#define	kSpeakTruncUrlMask								 64L		// Don't truncate spoken URL's to its minimal path
#define	kSpeakAttemptUrlMask							128L		// Don't even attempt to recognize the presence of a URL
#define	kSpeakDottyMask										256L		// Don't attempt to intelligently distinquish between period, "dot" and "point"
#define	kSpeakJohnnyCantReadMask					512L		// Don't correct the Speech Manager's embarrassing attempt at pronouncing "email"
#define	kSpeakQuotesMask								 1024L		// Don't bother reading quoted text at all
#define	kSpeakModifyQuoteVoiceMask			 2048L		// Don't modify the pitch of the voice when speaking quoted passages
#define	kSpeakEmphasisMask							 4096L		// Don't attempt to detect emphasis, either in styled text or creative use of ASCII
#define	kSpeakAcronymsMask							 8192L		// Don't translate common acronyms to their spoken equivalents
#define	kSpeakUpperToLowerWordBreakMask	16384L		// Break words at strict upper-to-lowercase transitions instead of assuming the last cap is the first character of a new word
#define	kSpeakShorthandMask							32768L		// Don't attempt to translate specific forms of shorthand like "w/regards"
#define	kSpeakShortLinesMask						65536L		// Don't place a pause (via an inserted period) at the end of short lines

#define	UseSpeakEmailRules							(((M_T1=GetPrefLong(PREF_SPEAK_BODY_GYMNASTICS_BITS))&kSpeakEmailRulesMask)==0)
#define UseSpeakEliminateTheUnspeakable	(UseSpeakEmailRules && (M_T1&kSpeakEliminateTheUnspeakableMask)==0)
#define UseSpeakMixedCase								(UseSpeakEmailRules && (M_T1&kSpeakMixedCaseMask)==0)
#define UseSpeakFindQuote								(UseSpeakEmailRules && (M_T1&kSpeakFindQuoteMask)==0)
#define UseSpeakNewMessagePhrase				(UseSpeakEmailRules && (M_T1&kSpeakNewMessagePhraseMask)==0)
#define UseSpeakTruncUrl								(UseSpeakEmailRules && (M_T1&kSpeakTruncUrlMask)==0)
#define UseSpeakAttemptUrl							(UseSpeakEmailRules && (M_T1&kSpeakAttemptUrlMask)==0)
#define UseSpeakDotty										(UseSpeakEmailRules && (M_T1&kSpeakDottyMask)==0)
#define UseSpeakJohnnyCantRead					(UseSpeakEmailRules && (M_T1&kSpeakJohnnyCantReadMask)==0)
#define	UseSpeakQuotes									(UseSpeakEmailRules && (M_T1&kSpeakQuotesMask)==0 && UseSpeakFindQuote)
#define	UseSpeakQuotePhrase							(UseSpeakEmailRules && (M_T1&kSpeakQuotePhraseMask)==0 && UseSpeakQuotes)
#define	UseSpeakModifyQuoteVoice				(UseSpeakEmailRules && (M_T1&kSpeakModifyQuoteVoiceMask)==0 && UseSpeakQuotes)
#define UseSpeakEmphasis								(UseSpeakEmailRules && (M_T1&kSpeakEmphasisMask)==0)
#define UseSpeakAcronyms								(UseSpeakEmailRules && (M_T1&kSpeakAcronymsMask)==0)
#define UseSpeakStrictUpperToLower			(UseSpeakEmailRules && (M_T1&kSpeakUpperToLowerWordBreakMask)==0 && UseSpeakMixedCase)
#define UseSpeakShorthand								(UseSpeakEmailRules && (M_T1&kSpeakShorthandMask)==0)
#define UseSpeakShortLines							(UseSpeakEmailRules && (M_T1&kSpeakShortLinesMask)==0)

#define UseInlineSig										(!PrefIsSet(PREF_NO_INLINE_SIG) && OkSigIntro())
Boolean OkSigIntro(void);

#define kAMOAvoidInvisible	0
#define kAMOAvoidNone				1
#define kAMOAvoidAll				2

#define prefSearchWebTBButtonInstalled 1
#define prefSearchWebSelectWarning 2

#endif
