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

#ifndef AEUTIL_H
#define AEUTIL_H

#define kEAEImmediate (kAEWaitReply|kAECanInteract|kAECanSwitchLayer)
#define kEAEWhenever (kAEQueueReply|kAECanInteract|kAECanSwitchLayer)
Boolean HaveScriptableFinder(void);
OSErr SimpleAESend(ProcessSerialNumber *psn,DescType eClass, DescType eId, AppleEvent *reply,AESendMode mode,...);
AEDescList *SimpleAEList(AEDescList *list,...);
void NullADList(AEDescPtr first,...);
void DisposeADList(AEDescPtr first,...);
OSErr GetAEError(AppleEventPtr ae);
#define NullDesc(desc) do{(desc)->descriptorType = typeNull; (desc)->dataHandle = nil;}while(0)
OSErr AEPutEnum(AppleEvent *event,DescType key,long theLong);
OSErr AEPutLong(AppleEvent *event,DescType key,long theLong);
OSErr AEPutPStr(AppleEvent *event,DescType key,PStr str);
OSErr AEPutBool(AppleEvent *event,DescType key,Boolean b);
OSErr AEPutRect(AppleEvent *event,DescType key,Rect *r);
OSErr AEPutLongDate(AppleEvent *event,DescType key,uLong secs);
PStr GetAEPStr(PStr s,AEDescPtr desc);
Handle GetAEText(AEDescPtr desc,AEDesc *coerced,Boolean *wasCoerced);
long GetAELong(AEDescPtr desc);
Boolean GetAEBool(AEDescPtr desc);
OSErr GetAEFSSpec(AEDescPtr desc,FSSpecPtr spec);
OSErr GetAERect(AEDescPtr desc,Rect *r);
OSErr GetAEPoint(AEDescPtr desc,Point *pt);
OSErr GotAERequired(AppleEvent *event);
OSErr MyAEPutAlias(AppleEvent *event, AEKeyword keyword, FSSpecPtr spec);
Boolean GetAEDefaultBool(AppleEvent *ae,DescType param,Boolean deflt);
OSErr MyAESend (const AppleEvent *theAppleEvent, AppleEvent *reply,
	 AESendMode sendMode, AESendPriority sendPriority,
	 long timeOutInTicks);
OSErr WhackFinder(FSSpecPtr spec);
OSErr CopyReply (const AppleEvent *sourceReplyEvent, AppleEvent *targetReplyEvent);
OSErr	ReflectAppleEvent (const AppleEvent *appleEvent, AppleEvent *replyEvent);

#endif