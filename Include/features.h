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

#ifndef FEATURES_H
#define FEATURES_H

#define	featureMatchAny		nil

enum {
	featureResourceType = FOUR_CHAR_CODE('FEAT'),
	featureUsageResourceType = FOUR_CHAR_CODE('F U ')
};

#define	rFeatureUsageResID		128

typedef OSType FeatureType;
typedef SInt16 FeatureID;

#define	featNoResource							0
#define	featSimultaneousDirService	1008

//
//      Indexes into the FeatureRecHandle of all features known
//      to this version of Eudora
//
//                      Do not change the order of these!!
//                      Always add new items to the bottom of the list, along with
//                      an entry into the 'FEAT' resource
//
//                      Items should not be deleted from this list unless the
//                      corresponding 'FEAT' resource is also removed.
//

#define	rFeatureBaseResID				1000

enum {
	featureNone = 0,	// Essentially, a dummy record.  Nothing is something
	featureEudoraPro,	// Another placeholder
	featureJunk,		// 'FEAT' (1000)
	featureSpellChecking,	// 'FEAT' (1001)
	featureMultiplePersonalities,	// 'FEAT' (1002)
	featureStationery,	// 'FEAT' (1003)
	featureSignatures,	// 'FEAT' (1004)
	featureFilters,		// 'FEAT' (1005)
	featureExtendedStyles,	// 'FEAT' (1006)
	featureCustomizeToolbar,	// 'FEAT' (1007)
	featureSimultaneousDirService,	// 'FEAT' (1008)
	featureMultipleNicknameFiles,	// 'FEAT' (1009)
	featureNicknameWatching,	// 'FEAT' (1010)
	featureFcc,		// 'FEAT' (1011)
	featureFilterPersonality,	// 'FEAT' (1012)
	featureFilterSound,	// 'FEAT' (1013)
	featureFilterOpen,	// 'FEAT' (1014)
	featureFilterPrint,	// 'FEAT' (1015)
	featureFilterForward,	// 'FEAT' (1016)
	featureFilterReply,	// 'FEAT' (1017)
	featureFilterRedirect,	// 'FEAT' (1018)
	featureFilterServerOptions,	// 'FEAT' (1019)
	featureFilterLabel,	// 'FEAT' (1020)
	featureFilterSpeak,	// 'FEAT' (1021)
	featureFilterAddHistory,	// 'FEAT' (1022)
	featureStyleColor,	// 'FEAT' (1023)
	featureStyleQuote,	// 'FEAT' (1024)
	featureStyleJust,	// 'FEAT' (1025)
	featureStyleMargin,	// 'FEAT' (1026)
	featureStyleBullet,	// 'FEAT' (1027)
	featureStyleHorzRule,	// 'FEAT' (1028)
	featureStyleLink,	// 'FEAT' (1029)
	featureStyleGraphics,	// 'FEAT' (1030)
	featureLink,		// 'FEAT' (1031)
	featureSpeak,		// 'FEAT' (1032)
	featureReturnReceiptTo,	// 'FEAT' (1033)
	featureSearch,		// 'FEAT' (1034)
	featureAnal,		// 'FEAT' (1035)
	featureStatistics,	// 'FEAT' (1036)
	featureScriptsMenu,	// 'FEAT' (1037)
	featureInlineSig,	// 'FEAT' (1038)
	featureConCon,		// 'FEAT' (1039)
	featureClip2Phone,	// 'FEAT' (1040)
#ifdef VCARD
	featureVCard,		// 'FEAT' (1041)
#endif
	featureContextualFiling = 44,	// 'FEAT' (1042)
	featureMBDrawer,	// 'FEAT' (1043)
	featureEmo,		// 'FEAT' (1044)
	featureLimit
};


//
//      Primary Feature Types
//
//              Do not change the order of these!!

enum {
	featureNoneType = 0,
	featureEudoraProType = FOUR_CHAR_CODE('pro '),
	featureTechSupportType = FOUR_CHAR_CODE('tech'),
	featureSpellCheckingType = FOUR_CHAR_CODE('spel'),
	featureMultiplePersonalitiesType = FOUR_CHAR_CODE('mPrs'),
	featureStationeryType = FOUR_CHAR_CODE('stat'),
	featureSignaturesType = FOUR_CHAR_CODE('mSig'),
	featureLinkType = FOUR_CHAR_CODE('link'),
	featureFiltersType = FOUR_CHAR_CODE('filt'),
	featureExtendedStylesType = FOUR_CHAR_CODE('xSty'),
	featureCustomizeToolbarType = FOUR_CHAR_CODE('cTbr'),
	featureSimultaneousDirServiceType = FOUR_CHAR_CODE('sDir'),
	featureMultipleNicknameFilesType = FOUR_CHAR_CODE('mNic'),
	featureNicknameWatchingType = FOUR_CHAR_CODE('nWch'),
	featureFccType = FOUR_CHAR_CODE('Fcc '),
	featureSpeakType = FOUR_CHAR_CODE('spek'),
	featureReturnReceiptToType = FOUR_CHAR_CODE('rrto'),
	featureSearchType = FOUR_CHAR_CODE('srch'),
	featureAnalType = FOUR_CHAR_CODE('anal'),
	featureStatisticsType = FOUR_CHAR_CODE('stcs'),
	featureScriptsMenuType = FOUR_CHAR_CODE('scpt'),
	featureInlineSigType = FOUR_CHAR_CODE('isig'),
#ifdef VCARD
	featureVCardType = FOUR_CHAR_CODE('vcrd'),
#endif
	featureConConType = FOUR_CHAR_CODE('conc'),
	featureContextualFilingType = FOUR_CHAR_CODE('cxfl'),
	featureMBDrawerType = FOUR_CHAR_CODE('mbdr'),
	featureTypeLimit
};


//
//      Filter ('filt') Sub Feature Types
//

enum {
	featureFilterPersonalityType = FOUR_CHAR_CODE('fPrs'),
	featureFilterSoundType = FOUR_CHAR_CODE('fSnd'),
	featureFilterOpenType = FOUR_CHAR_CODE('fOpn'),
	featureFilterPrintType = FOUR_CHAR_CODE('fPrt'),
	featureFilterForwardType = FOUR_CHAR_CODE('fFor'),
	featureFilterReplyType = FOUR_CHAR_CODE('fRpy'),
	featureFilterRedirectType = FOUR_CHAR_CODE('fRdr'),
	featureFilterServerOptionsType = FOUR_CHAR_CODE('fSvr'),
	featureFilterLabelType = FOUR_CHAR_CODE('fLbl'),
	featureFilterSpeakType = FOUR_CHAR_CODE('fSpk'),
	featureFilterAddHistoryType = FOUR_CHAR_CODE('fHst')
};

//
//      Extended Styles ('xSty') Sub Feature Types
//

enum {
	featureStyleColorType = FOUR_CHAR_CODE('sCol'),
	featureStyleQuoteType = FOUR_CHAR_CODE('sQuo'),
	featureStyleJustType = FOUR_CHAR_CODE('sJst'),
	featureStyleMarginType = FOUR_CHAR_CODE('sMgn'),
	featureStyleBulletType = FOUR_CHAR_CODE('sBlt'),
	featureStyleHorzRuleType = FOUR_CHAR_CODE('sHrz'),
	featureStyleLinkType = FOUR_CHAR_CODE('sLnk'),
	featureStyleGraphicsType = FOUR_CHAR_CODE('sGra')
};

enum {
	featurePresent = 0x0001,
	featureEnabled = 0x0002,
	featureUpdate = 0x0004
};


// Feature Resources ('FEAT')

typedef struct {
	FeatureType primary;	// The primary feature being supported
	FeatureType sub;	// Any secondary feature being supported within the primary feature
	Boolean hasSubFeatures;	// 'true' if this feature has associated sub features
	char *nameAndDesc;	// The name given to this feature (i.e. Jello Disposal), followed by
	// the description of what this feature does (i.e. Eliminates all Jello
	// in a painful unspoken manner).  Each are stored as Pascal styled strings,
	// one after the other.
} FeatureRes, *FeatureResPtr, **FeatureResHandle;

// Features in memory

typedef struct {
	FeatureType primary;	// The primary feature being supported
	FeatureType sub;	// Any secondary feature being supported within the primary feature
	UInt32 flags;		// The status of this feature (is it available, disabled, is an update available?)
	UInt32 lastUsed;	// The date and time when this feature was last used (zero of it's a virginal feature)
	short resID;		// Resource ID of the 'FEAT' representing this feature, or 'featNoResource' if there is no resource describing this feature
	Boolean hasSubFeatures;	// 'true' if this feature has associated sub features
} FeatureRec, *FeatureRecPtr, **FeatureRecHandle;

typedef struct {
	FeatureType type;	// The feature we're talking about
	UInt32 lastUsed;	// The date and time when this feature was last used (zero of it's a virginal feature)
} FeatureUsageRec, *FeatureUsagePtr, **FeatureUsageHandle;

#ifdef DEATH_BUILD
#define RegisterProFeatures()
#define RegisterFeature(aFeatureType,anotherFeatureType,aShort,aBoolean)
#define UpdateFeatureUsage(aFeatureRecHandle)
#define SaveFeaturesWithExtemeProfanity(aFeatureRecHandle)
#define HasFeature(aFeatureID)	true
#define DisableFeature(aFeatureID)
#define EnableFeature(aFeatureID)
#define UseFeature(aFeatureID)
#define UseFeatureType(aFeatureType)
#define FindFeatureType(aFeatureRecHandle,aFeatureType,anotherFeatureType)	nil
#define FindFeatureID(aFeatureRecHandle,aFeatureID)	nil
#define DisableMenuItemWithProOnlyIcon(aMenuHandle,aShort)
#define SetMenuItemBasedOnFeature(aMenuHandle,aShort,aFeatureType,anotherShort)
#else
void RegisterProFeatures(void);
void RegisterFeature(FeatureType primary, FeatureType sub, short resID,
		     Boolean hasSubFeatures);
void UpdateFeatureUsage(FeatureRecHandle featureSet);
void SaveFeaturesWithExtemeProfanity(FeatureRecHandle featureSet);
Boolean HasFeature(FeatureID feature);
void DisableFeature(FeatureID feature);
void EnableFeature(FeatureID feature);
void UseFeature(FeatureID feature);
void UseFeatureType(FeatureType feature);
FeatureRecPtr FindFeatureType(FeatureRecHandle featureSet,
			      FeatureType primary, FeatureType sub);
FeatureRecPtr FindFeatureID(FeatureRecHandle featureSet,
			    FeatureID feature);
void DisableMenuItemWithProOnlyIcon(MenuHandle theMenu, short item);
void SetMenuItemBasedOnFeature(MenuHandle theMenu, short item,
			       FeatureType feature, short enable);
#endif

#endif
