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

#ifndef JUNK__H
#define JUNK__H

#define bJunkPrefBoxHold	(1<<0)
#define bJunkPrefBoxArchive	(1<<1)
#define bJunkPrefBoxArchiveWarning (1<<2)
#define bJunkPrefBoxAutoFilter (1<<3)
#define bJunkPrefBoxNoUnread (1<<4)
#define bJunkPrefBoxExistWarning (1<<5)
#define bJunkPrefNeedIntro (1<<6)
#define bJunkPrefServerDel (1<<7)
#define bJunkPrefWhiteList (1<<8)
#define bJunkPrefNotJunkWhite	(1<<9)
#define bJunkPrefDoHeaders	(1<<10)
#define bJunkPrefDoBayes	(1<<11)
#define bJunkPrefSwitchCmdJ	(1<<12)
#define bJunkPrefSwitchCmdJAsked	(1<<13)
#define bJunkPrefBelieveDate	(1<<14)
#define bJunkPrefAlwaysEnable	(1<<15)
#define bJunkPrefIMAPNoRunPlugins	(1<<16)
#define bJunkPrefHasIMAPSupport	(1<<17)
#define bJunkPrefNoIMAPSupportWarning	(1<<18)
#define bJunkPrefNewIMAPSupportWarning	(1<<19)
#define bJunkPrefDeadPluginsWarning	(1<<20)
#define bJunkPrefCantCreateJunk	(1<<21)
#define bJunkPrefNoWhitelistReplies	(1<<22)
#define JunkPrefBoxHold() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxHold))
#define JunkPrefBoxArchive() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxArchive))
#define JunkPrefBoxArchiveWarning() (JunkPrefBoxArchive() && (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxArchiveWarning)))
#define JunkPrefBoxAutoFilter() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxAutoFilter))
#define JunkPrefBoxNoUnread() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxNoUnread))
#define JunkPrefBoxExistWarning() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBoxExistWarning))
#define JunkPrefNeedIntro() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefNeedIntro))
#define JunkPrefServerDel() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefServerDel))
#define JunkPrefWhiteList() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefWhiteList))
#define JunkPrefNotJunkWhite() (JunkPrefWhiteList()&&(!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefNotJunkWhite)))
#define JunkPrefDoHeaders() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefDoHeaders))
#define JunkPrefDoBayes() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefDoBayes))
#define JunkPrefSwitchCmdJ() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefSwitchCmdJ))
#define JunkPrefSwitchCmdJAsked() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefSwitchCmdJAsked))
#define JunkPrefBelieveDate() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefBelieveDate))
#define JunkPrefAlwaysEnable() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefAlwaysEnable))
#define JunkPrefIMAPNoRunPlugins() (GetPrefBitNoDominant(PREF_JUNK_MAILBOX,bJunkPrefIMAPNoRunPlugins))
#define JunkPrefHasIMAPSupport() (GetPrefBitNoDominant(PREF_JUNK_MAILBOX,bJunkPrefHasIMAPSupport))
#define JunkPrefNoIMAPSupportWarning() (GetPrefBitNoDominant(PREF_JUNK_MAILBOX,bJunkPrefNoIMAPSupportWarning))
#define JunkPrefNewIMAPSupportWarning() (GetPrefBitNoDominant(PREF_JUNK_MAILBOX,bJunkPrefNewIMAPSupportWarning))
#define JunkPrefDeadPluginsWarning() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefDeadPluginsWarning))
#define JunkPrefCantCreateJunk() (GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefCantCreateJunk))
#define JunkPrefWhiteListReplies() (!GetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefNoWhitelistReplies))

// Minimum and maximim scores
#define JUNK_MAX_SCORE	100
#define JUNK_MIN_SCORE	0

// Reasons things might be junk
#define JUNK_BECAUSE_XFER	1
#define JUNK_BECAUSE_USER 2
#define JUNK_BECAUSE_PLUG 3
#define JUNK_BECAUSE_WHITE	4
#define JUNK_BECAUSE_IMAP_SUCKS	5

OSErr FilterJunk(TOCHandle fromTocH);
#define GetJunkTOC()	GetSpecialTOC(JUNK)

void JunkTOCCleanse(TOCHandle tocH);
OSErr ArchiveJunk(TOCHandle tocH);
Boolean SpecIsJunkSpec(FSSpecPtr spec);
Boolean BoxIsJunkBox(TOCHandle tocH);
void PreexistingJunkWarning(FSSpecPtr spec);
short JunkRescanBox (TOCHandle tocH);
short JunkRescanJunkMailbox ();
OSErr Junk(TOCHandle tocH,short sumNum,Boolean isJunk,Boolean ezOpen);
OSErr JunkSetScore(TOCHandle tocH,short sumNum,short because,short score);
short JunkIntro(void);
Boolean JunkTrimOK(void);
Boolean JunkItemsEnable(MyWindowPtr win,Boolean not);
void JunkReassignKeys(Boolean switchem);

Boolean	CanScoreJunk ();
void JunkScoreBox ( TOCHandle tocH, short first, short last, Boolean rescore );
void JunkScoreIMAPBox ( TOCHandle tocH, short first, short last, Boolean unfiltered );
void JunkScoreSelected ( TOCHandle tocH );
OSErr MoveToJunk ( TOCHandle source, short spamThresh, FilterPB *fpb );
OSErr MoveToIMAPJunk ( TOCHandle source, short sumNum, short spamThresh, FilterPB *fpb );
#endif