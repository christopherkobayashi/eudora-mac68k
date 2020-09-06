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

#ifndef ANAL_H
#define ANAL_H
// #include "TAE.h" CK

#define MAX_ANAL_WHITE	50	// allow up to 50 words to be whitelisted

void AnalScan(void);
void AnalScanPete(PETEHandle pte,Boolean toCompletion,Boolean toSpeak);
short AnalScanHandle(UHandle text,long offset,long len,Boolean *inHeader);
#define AnalIcon(score) (ANAL_ICON_BASE+score-1)
#define AnalDisabled()	(NoAnalDictionary || (GetPrefLong(PREF_NO_COURIC)&1))
#define BeingAnal()	(!AnalDisabled()&&HasFeature(featureAnal)&&TAEStartSession)
#define AnalDoIncoming()	(BeingAnal()&&!(GetPrefLong(PREF_NO_COURIC)&2))
#define AnalDoExisting()	(BeingAnal()&&!(GetPrefLong(PREF_NO_COURIC)&4))
#define AnalMarkPhrases()	(BeingAnal()&&!(GetPrefLong(PREF_NO_COURIC)&8))
#define AnalCoverPhrases()	(BeingAnal()&&(GetPrefLong(PREF_NO_COURIC)&16))
#define AnalSpeakPhrases()	(BeingAnal()&&(GetPrefLong(PREF_NO_COURIC)&32))
#define AnalDelayOutgoing()	(BeingAnal()&&!(GetPrefLong(PREF_NO_COURIC)&64)&&GetRLong(ANAL_DELAY_LEVEL))
#define AnalScanQuotes()	(BeingAnal()&&!(GetPrefLong(PREF_NO_COURIC)&128))
#define AnalWarnOutgoing()	(BeingAnal()&&GetRLong(ANAL_WARNING_LEVEL))
Boolean AnalWarning(MessHandle messH);
OSErr AnalFindMine(void);
#endif
