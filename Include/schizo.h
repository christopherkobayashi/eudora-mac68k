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

#ifndef SCHIZO_H
#define SCHIZO_H

#define PERS_VERS 0

#ifdef	IMAP
typedef struct MailboxNode MailboxNode, *MailboxNodePtr, **MailboxNodeHandle;
#endif

typedef struct Personality Personality, *PersPtr, **PersHandle;
struct Personality
{
	uLong persId;
	short version;
	short resId;
	short resEnd;
	Str31 name;
	Str31 password;
	Str31 secondPass;
	Boolean popSecure;
	Boolean dirty;
	PersHandle next;
	long doMeNow:1;
	long checked:1;
	long checkMeNow:1;
	long sendMeNow:1;
	long autoCheck:1;
	long noUIDL:1;
	long uupcIn:1;
	long uupcOut:1;
#ifdef	IMAP
	long imapRefresh:1;
	long otherFlags:23;
#else
	long otherFlags:24;
#endif
	long sendQueue;
	uLong checkTicks;
	uLong ivalTicks;
#ifdef	IMAP
	MailboxNodeHandle mailboxTree;
#endif
	ProxyHandle proxy;
};

#define PERS_RTYPE	'Pers'

void InitPersonalities(void);
void DisposePersonalities(void);
uLong PersCheckTicks(void);
void PersSkipNextCheck(void);
Boolean PersAnyPasswords(void);
OSErr PersSaveAll(void);
OSErr PersSave(PersHandle pers);
OSErr PersSavePw(PersHandle pers);
OSErr PersFillPw(PersHandle pers,uLong whichOnes);
#define kFillRegularPw 1
#define kFillSecondPw 2
PersHandle PersNew(void);
PersHandle FindPersById(uLong persId);
PersHandle FindPersByName(PStr name);
OSType PersType(OSType theType,PersHandle pers);
OSErr SetPersProperty(AEDescPtr token,AEDescPtr descP);
OSErr GetPersProperty(AEDescPtr token,AppleEvent *reply,long refCon);
OSErr AECreatePersonality(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply);
OSErr AEPersObj(PersHandle pers,AEDescPtr descP);
OSErr AESetPers(TOCHandle tocH,short sumNum,AEDescPtr descP);
OSErr PersDelete(PersHandle pers);
long PersCount(void);
OSErr PersSetName(PersHandle pers,PStr name);
void PushPers(PersHandle newCur);
void PopPers(void);
OSErr SetPers(TOCHandle tocH,short sumNum,PersHandle pers,Boolean stationery);
void CheckPers(MyWindowPtr win,Boolean all);
short Pers2Index(PersHandle goalPers);
PersHandle Index2Pers(short n);
void PersSetAutoCheck(void);
PersHandle PersChoose(StringPtr prompt);
void PersZapResources(OSType type,short resEnd);
void SetBGColorsByPers(MessHandle messH);

#define PERS_TYPE(t,r)	((r) ? (((t)&0xffff0000) | (r)) : (t))
#define	PERS_POPD_TYPE(p)	(((p)&&((p)!=PersList)) ? (PERS_TYPE(OLD_POPD_TYPE,(*(p))->resEnd)) : OLD_POPD_TYPE)
#define CUR_POPD_TYPE	PERS_POPD_TYPE(CurPers)
#define CUR_STR_TYPE	((CurPers&&(*CurPers)->persId)?PERS_TYPE('STR ',(*CurPers)->resEnd):'STR ')
#endif
