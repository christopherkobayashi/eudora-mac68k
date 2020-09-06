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

#ifndef COMPACT_H
#define COMPACT_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for functions relating to composition window actions
 ************************************************************************/

/* NOTE: the define of ICON_BAR_NUM here must always be the same as the
					number of elements in the arrays fBits[] and icons[], which are
					declared in compact.c */
#define ICON_BAR_NUM 6
void CompSetFormatBarIcon(MyWindowPtr win,Boolean visible);
OSErr AddPriorityPopup(MessHandle messH);
void CompDelAttachment(MessHandle messH,HSPtr hs);
void CompActivateAppropriate(MessHandle messH);
OSErr CompLeaving(MessHandle messH,short head);
Boolean CompClose(MyWindowPtr win);
void CompDidResize(MyWindowPtr win, Rect *oldContR);
void CompClick(MyWindowPtr win, EventRecord *event);
Boolean CompMenu(MyWindowPtr win, int menu, int item, short modifiers);
Boolean CompKey(MyWindowPtr win, EventRecord *event);
OSErr CompGonnaShow(MyWindowPtr win);
OSErr AddMessTranslator(MessHandle messH,long which,Handle properties);
OSErr RemoveMessTranslator(MessHandle messH,long wich);
OSErr SetSig(TOCHandle tocH,short sumNum,long sigId);
void CompUnattach(MyWindowPtr win);
void AttachSelect(MessHandle messH);
void CompButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part);
void CompIdle(MyWindowPtr win); /* MJN *//* new routine */
void CompHelp(MyWindowPtr win,Point mouse);
void CompAttach(MyWindowPtr win,Boolean insertDefault);
void CompAttachStd(MyWindowPtr win,Boolean insertDefault);
Boolean ModifyQueue(short *state,uLong *when,Boolean swap);
void WarpQueue(uLong secs);
void CompZoomSize(MyWindowPtr win,Rect *zoom);
short CountAttachments(MessHandle messH);
Boolean InTranslator(TransInfoHandle hTranslators,long id);
void PlotFlag(Rect *r,Boolean on,short which);
void CompAttachSpec(MyWindowPtr win, FSSpecPtr spec);
#define AttachOptNumber(flags) (((flags & (FLAG_ATYPE_LO|FLAG_ATYPE_HI))>>6)&0x3)
#define SetAOptNumber(flags,num)\
	do{(flags) &= ~(FLAG_ATYPE_LO|FLAG_ATYPE_HI); (flags) |= (num)<<6;}while(0)
void ForceCompWindowRecalcAndRedraw(MyWindowPtr win); /* MJN *//* new routine */
void SetAttachmentType(TOCHandle tocH, short sumNum, short type);
OSErr GetStationerySum(Handle textH,MSumPtr pSum);
#ifdef TWO
uLong ApproxMessageSize(MessHandle messH);
OSErr AttachDoc(MyWindowPtr win,FSSpecPtr spec);
OSErr SaveStationeryStuff(short refN,MessHandle messH);
void ApplyStationery(MyWindowPtr win,FSSpecPtr spec,Boolean dontCleanse,Boolean personality);
void ApplyStationeryLo(MyWindowPtr win,FSSpecPtr spec,Boolean dontCleanse,Boolean personality,Boolean editStationery);
void ApplyStationeryHandle(MyWindowPtr win,Handle textH,Boolean dontCleanse,Boolean personality,Boolean editStationery);
void CompIBarUpdate(MessHandle messH);
#endif
void DrawPopIBox(Rect *r, short sicnId);
void DrawShadowBox(Rect *r);
typedef enum {kEuSendNow, kEuSendNext, kEuSendLater, kEuSendNever} SendTypeEnum;
OSErr QueueMessage(TOCHandle tocH,short sumNum,SendTypeEnum st,long secs, Boolean noSpell, Boolean noAnalDelay);
OSErr CompDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
OSErr NickExpandAndCacheHead(MessHandle messH,short head,Boolean cacheOnly);
void CompReallyPreferBody(MyWindowPtr win);
#ifdef THREADING_ON
#define SENT_OR_SENDING(state)	((state)==SENT || (state)==BUSY_SENDING)
#else
#define SENT_OR_SENDING(state) ((state)==SENT)
#endif
OSErr GatherRecipientAddresses (MessHandle messH, Handle *dest, Boolean wantComments);
pascal OSErr PersGraphic(PETEHandle pte,PETEGraphicInfoHandle graphic,long offset,PETEGraphicMessage message,void *data);
#endif