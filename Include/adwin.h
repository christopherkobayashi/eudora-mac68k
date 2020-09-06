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

#ifndef ADWIN_H
#define ADWIN_H

//	Ad id
//	Note: must update linkmng.c if this changes ...
typedef struct
{	long	server;		//	Server ID
	long	ad;			//	Individual ad ID
} AdId;

typedef struct
{
	AdId	adId;
	Str32	title;
	Handle	iconSuite;
	Boolean	deleted;
} TBAdInfo, **TBAdHandle;

//	Prototypes
void OpenAdWindow(void);
void CloseAdWindow(void);
Boolean PeteIsInAdWindow(PETEHandle pte);
Boolean IsAdInPlaylist(AdId adId);
void AdWinGotImage(PETEHandle pte,FSSpecPtr spec);
void AdShutdown(void);
#ifdef DEBUG
void ToggleAdWindow(Boolean resetPlayState);
void DebugAdMenu(short item,short modifiers);
void CheckCurrentAd(void);
#endif
void AdWinIdle(void);
void AreAdsFailing(StringPtr errString, OSErr *errCode,Boolean *failing,Boolean *succeeding);
void AdCheckingMail(void);
Boolean AdWinNeedsNetwork(void);
Boolean SameAd(AdId *pAd1,AdId *pAd2);
void AdUserClick(AdId adId);
Handle AdGetTBIcon(AdId adId);
void AdDeleteButton(AdId adId);
void SizeAdWin(void);
OSErr ValidAdImage(PETEHandle pte,Handle hData);
void SetupSponsorAd(void);
void DrawSponsorAd(MyWindowPtr win);
Boolean ClickSponsorAd(MyWindowPtr win,EventRecord *event,Point pt);
void SetupWinSponsorInfo(MyWindowPtr win);
UPtr GetLanguageCode(UPtr s);
void ForcePlaylistRequest(void);
void GetSponsorAdTitle(UPtr sTitle);
OSErr AdFailureChecks(NoAdsAuxPtr noAds);
#endif