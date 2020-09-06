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

#ifndef MAILXFER_H
#define MAILXFER_H

#include "filters.h"

typedef struct XferFlags
{
	Boolean check;
	Boolean send;
	Boolean servFetch;
	Boolean servDel;
	Boolean nuke;
	Boolean nukeHard;
	Boolean stub;
	Boolean isAuto;
} XferFlags;

#ifdef	IMAP
typedef struct IMAPTransferRec_ IMAPTransferRec, *IMAPTransferPtr;
#endif

short XferMail(Boolean check, Boolean send, Boolean manual,Boolean ae,Boolean thread,short modifiers); 
short XferMailSetup(Boolean *check, Boolean *send, Boolean manual,Boolean ae,XferFlags *flags,short modifiers);
#ifdef IMAP
short XferMailRun(Boolean check, Boolean send, Boolean manual,Boolean ae, XferFlags flags, IMAPTransferPtr imapInfo);
#else
short XferMailRun(Boolean check, Boolean send, Boolean manual,Boolean ae, XferFlags flags);
#endif
void GrabSignature(uLong fid);
OSErr SigSpec(FSSpecPtr spec,long id);
OSErr TransmitMessageHi(TransStream stream,MessHandle messH,Boolean chatter,Boolean sendDataCmd);
void ShowBoxAt(TOCHandle tocH,short selectMe,WindowPtr behindWin);
short FumLub(TOCHandle tocH);
OSErr GoOnline(void);
#ifdef THREADING_ON
void FilterXferMessages (void);
void ResetCheckTime(Boolean force); 
#endif
#ifdef BATCH_DELIVERY_ON
void NotifyNewMail(short gotSome,Boolean noXfer,TOCHandle tocH, FilterPB *fpbDelivery);
void NotifyNewMailLo(short gotSome,Boolean noXfer,TOCHandle tocH, FilterPB *fpbDelivery, Boolean OpenIn);
#else
void NotifyNewMail(short gotSome,Boolean noXfer,TOCHandle tocH);
#endif
OSErr DoFcc(TOCHandle tocH,short sumNum,CSpecHandle list);	
void CompAttDel(MessHandle messH);
WindowPtr OpenBehindMePlease(void);
void ProcessReceivedRegFiles(void);
OSErr OutgoingMIDListSave(void);
OSErr OutgoingMIDListLoad(void);
void BadgeTheSupidDock(short count, PStr text, Boolean attentionColor);
long GlobalUnreadCount(void);
#define PrefBadgeDo() ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES)&0x1)==0)
#define PrefBadgeRecent()  ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES)&0x2)==0)
#define PrefBadgeOpenBoxes()  ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES)&0x4)==0)
#endif