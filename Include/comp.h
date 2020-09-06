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

#ifndef COMP_H
#define COMP_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
Boolean SaveComp(MyWindowPtr win);
MyWindowPtr OpenComp(TOCHandle tocH, int sumNum, WindowPtr winWP,
		     MyWindowPtr win, Boolean showIt, Boolean new);
int QueueSelectedMessages(TOCHandle tocH, short toState, uLong when);
short CreateMessageBody(UPtr buffer, uLong * hashPtr);
long CountCompBytes(MessHandle messH);
void UpdateSum(MessHandle messH, long offset, long length);
PStr NewMessageId(PStr id);
long BodyOffset(Handle text);

Boolean IsMe(PStr address);
OSErr WriteTranslators(short refN, TransInfoHandle translators);
int AddTranslatorsFromPtr(MessHandle messH, UPtr text, long len);
HSPtr CompHeadFind(MessHandle messH, short index, HSPtr hSpec);
HSPtr CompHeadFindStr(MessHandle messH, PStr name, HSPtr hSpec);
HSPtr HandleHeadFindStr(UHandle text, PStr name, HSPtr hSpec);
HSPtr HandleHeadFindReply(UHandle text, HSPtr hSpec);
OSErr HandleHeadGetAddrs(UHandle text, HSPtr hs, BinAddrHandle * addrs);
OSErr HandleHeadCopyAddrs(UHandle text, HSPtr hs, MessHandle messH,
			  short headerID, AccuPtr addrAcc,
			  Boolean cacheThem);
OSErr CompHeadAppendAddrStr(MessHandle messH, HSPtr targetHS, PStr addr);
OSErr AddSelfAddrHashes(AccuPtr addrAcc);
OSErr CompHeadActivate(PETEHandle pte, HSPtr hSpec);
OSErr CompHeadSet(PETEHandle pte, HSPtr hSpec, Handle text);
OSErr CompHeadAppend(PETEHandle pte, HSPtr hSpec, Handle text);
OSErr CompHeadSetPtr(PETEHandle pte, HSPtr hSpec, UPtr text, long size);
#define CompHeadSetStr(messH,hSpec,str)	CompHeadSetPtr((messH),(hSpec),(str)+1,*(str))
OSErr CompHeadSetIndexPtr(PETEHandle pte, short index, UPtr text,
			  long size);
#define CompHeadSetIndexStr(messH,index,str)	CompHeadSetIndexPtr((messH),(index),(str)+1,*(str))
OSErr CompHeadPrependPtr(PETEHandle pte, HSPtr hSpec, UPtr text,
			 long size);
OSErr CompHeadAppendPtr(PETEHandle pte, HSPtr hSpec, UPtr text, long size);
#define CompHeadAppendStr(messH,hSpec,str)	CompHeadAppendPtr((messH),(hSpec),(str)+1,*(str))
OSErr HandleHeadGetText(UHandle textIn, HSPtr hSpec, Handle * text);
OSErr HandleHeadGetIdText(UHandle textIn, short id, Handle * text);
OSErr CompHeadGetText(PETEHandle pte, HSPtr hSpec, Handle * text);
OSErr CompHeadGetTextPtr(PETEHandle pte, HSPtr hSpec, long offset,
			 UPtr text, long textSize, long *bytes);
PStr HandleHeadGetPStr(Handle text, short head, PStr val);
OSErr GetRHeaderAnywhere(MessHandle messH, short header, Handle * text);
OSErr GetRHeaderAnywherePtr(MessHandle messH, short header, UPtr text,
			    long textSize, long *bytes);
OSErr GetHeaderAnywhere(MessHandle messH, PStr header, Handle * text);
OSErr GetHeaderAnywherePtr(MessHandle messH, PStr header, UPtr text,
			   long textSize, long *bytes);
short CompHeadCurrent(PETEHandle pte);
OSErr CompHeadGetStrLo(MessHandle messH, short index, PStr string,
		       short size);
OSErr CompAddExtraHeaderDangerDangerLookOutWillRobinson(MessHandle messH,
							PStr headName,
							Handle
							headContents);
void SuckHeaderText(MessHandle messH, UPtr string, long size, short index);
PStr CompGetMID(MessHandle messH, PStr mid);
#define CompHeadGetStr(p,i,s)	CompHeadGetStrLo(p,i,s,sizeof(s))
	 /* MJN *//* new macro */
/* IsCompWindow(WindowPtr win) - this macro evaluates to a Boolean result indicating
 * whether the window pointed to by win is a composition window */
#define IsCompWindow(aWindowPtr) (((aWindowPtr) && GetWindowKind (aWindowPtr) == COMP_WIN) ? true : false)

Boolean IsHeaderNickField(PETEHandle pte);
OSErr HiliteCompHeader(PETEHandle pte, Boolean hilite);
Boolean GetCompNickFieldRange(PETEHandle pte, long *start, long *end);
OSErr CompGatherRecipientAddresses(MessHandle messH, Boolean wantComments);
Boolean IsAllLWSPMess(MessHandle messH);
uLong GetSigByName(PStr name);
OSErr AddInlineSig(MessHandle messH);
OSErr RemoveInlineSig(MessHandle messH);
void PersonalizeSubject(MessHandle messH);
void SerializeSubject(MessHandle messH);
void CompSelectSecondUnquoted(MessHandle messH);
#define HSIsEmpty(hs) ((hs)->stop<=(hs)->value+1)
#endif
