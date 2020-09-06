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

#include <Types.h>
#include "PrefDefs.h"
#include "StringDefs.h"
PersHandle SettingsPers(void);
#define ICCRAP	SettingsPers()!=PersList||!ReadIC||WriteIC
Boolean PrefEnabled(short prefN)
{
#ifdef LIGHT
	Boolean Light = true;
#else
	Boolean Light = false;
#endif

	switch (ABS(prefN))
	{
		case PREF_PPP_DISC: return(CanChangePPPState());
		case PREF_WARN_RICH: return(HasFeature(featureExtendedStyles));
		case PREF_RRT: return(HasFeature(featureReturnReceiptTo));
		case PREF_AUTO_FCC: return(HasFeature (featureFcc));
		case PREF_PPP_NOAUTO: return(CanCheckPPPState());
		case PREF_SEND_STYLE: return(HasFeature (featureExtendedStyles));
		case PREF_IGNORE_MX: return(gUseOT == true && GetOSVersion () < 0x1030);
		case PREF_NICK_EXP_TYPE_AHEAD: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_WATCH_IMMED: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_WATCH_WAIT_KEYTHRESH: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_WATCH_WAIT_NO_KEYDOWN: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_POPUP_ON_MULTMATCH: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_POPUP_ON_DEFOCUS: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_HILITING: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_TYPE_AHEAD_HILITING: return(HasFeature (featureNicknameWatching));
		case PREF_NO_WINTERTREE: return(HasFeature(featureSpellChecking)&&HaveSpeller());
		case PREF_WINTERTREE_OPTS: return(HasFeature(featureSpellChecking)&&HaveSpeller());
		case PREF_SPEAK_NO_NICE_ADDRESSES: return(HasFeature (featureSpeak));
		case PREF_KEYCHAIN: return(KeychainAvailable());
		case PREF_SPEAK_BODY_GYMNASTICS_BITS: return(HasFeature (featureSpeak));
		case PREF_NO_SPOKEN_WARNINGS: return(HasFeature (featureSpeak));
		case PREF_NICK_CACHE: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_CACHE_NOT_VISIBLE: return(HasFeature (featureNicknameWatching));
		case PREF_NICK_CACHE_NOT_ADD_REPLY_TO: return(HasFeature (featureNicknameWatching));
		case PREF_NO_LINK_HISTORY: return(HasFeature (featureLink));
		case PREF_NICK_CACHE_NOT_ADD_TYPING: return(HasFeature (featureNicknameWatching));
		case PREF_NO_COURIC: return(HasFeature(featureAnal));
		case PREF_NO_INLINE_SIG: return(HasFeature (featureInlineSig)&&OkSigIntro());
		case PREF_VCARD_QUIT_ON_ERROR: return(HasFeature (featureVCard));
		case PREF_HOME_IS_NICER_THAN_WORK: return(HasFeature (featureVCard));
		case PREF_PERSONAL_NICKNAMES_NOT_VISIBLE: return(HasFeature (featureVCard));
		case PREF_SSL_POP_SETTING: return(CanDoSSL());
		case PREF_HIDE_VCARD_BUTTON: return(HasFeature (featureVCard));
		case PREF_SSL_SMTP_SETTING: return(CanDoSSL());
		case PREF_SSL_IMAP_SETTING: return(CanDoSSL());
		case PREF_VCARD: return(HasFeature (featureVCard));
		case PREF_USE_OWN_DOC_HELPERS: return(HaveTheDiseaseCalledOSX());
		case PREF_USE_OWN_URL_HELPERS: return(HaveTheDiseaseCalledOSX());
		case PREF_PROXY: return(Proxies!=nil);
		case PREF_RELAY_PERSONALITY: return(HasFeature(featureMultiplePersonalities)&&PersCount()>1);
		case PREF_NO_RELAY_PARTICIPATE: return(HasFeature(featureMultiplePersonalities)&&PersCount()>1);
		case PREF_SYNC_OSXAB: return((GetOSVersion () >= 0x1020));
		case CONCON_PREVIEW_PROFILE: return(HasFeature(featureConCon));
		case CONCON_MULTI_PREVIEW_PROFILE: return(HasFeature(featureConCon));
		case CONCON_MESSAGE_PROFILE: return(0);
		default: return(True);
	}
}