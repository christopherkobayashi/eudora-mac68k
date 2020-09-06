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

#ifndef MESSACT_H
#define MESSACT_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * prototypes
 **********************************************************************/
Boolean MessClose(MyWindowPtr win);
Boolean MessMenu(MyWindowPtr win,int menu,int item,short modifiers);
void MessZoomSize(MyWindowPtr win,Rect *zoom);
void HiliteOddReply(MessHandle messH);
//void AlignHeaders(MessHandle messH);
void SaveMessageAs(MessHandle messH);
Boolean MessKey(MyWindowPtr win, EventRecord *event);
short SaveAsToOpenFile(short refN,MessHandle messH);
pascal Boolean SaveAsFilter(DialogPtr dgPtr,EventRecord *event,short *item,Ptr userData);
pascal short SaveAsHook(short item,DialogPtr dgPtr, Ptr userData);
void NextMess(TOCHandle tocH,MessHandle messH,short whichWay,long modifiers,Boolean ezOpen);
Boolean MessFind(MyWindowPtr win,PStr what);
void MessClick(MyWindowPtr win,EventRecord *event);
short NewPrior(short item,short prior);
OSErr MessGonnaShow(MyWindowPtr win);
OSErr MessMakeEditable(MyWindowPtr win, Boolean value);
void MessDidResize(MyWindowPtr win, Rect *oldContR);
void MessFocus(MessHandle messH,PETEHandle pte);
void MessCursor(Point mouse);
void PriorMenuHelp(MyWindowPtr win,Rect *priorRect);
Boolean GetPriorityRect(MyWindowPtr win,Rect *pr);
void DrawPriority(Rect *pr,short p);
short PriorityMenu(MyWindowPtr win);
void ShowMessageSeparator(PETEHandle pte,Boolean center);
int UnwrapSave(UPtr text, long length, long offset, short refN);
Boolean MessApp1(MyWindowPtr win,EventRecord *event);
void SetSubject(TOCHandle tocH,short sumNum,PStr sub);
void SetSender(TOCHandle tocH,short sumNum,PStr sender);
void SetFlag(TOCHandle tocH,short sumNum,long flag,Boolean on);
void SetOpt(TOCHandle tocH,short sumNum,long flag,Boolean on);
void MessIBarUpdate(MessHandle messH);
Boolean CheckAddNotifyControls(MyWindowPtr win, MessHandle messH);
#define attColor	1
#define attSelect	2
#define attOpen		4
#define	attFinder	8
#define	attAll		7
#define attPrint	16
Boolean SaveMess(MyWindowPtr win);
OSErr MessSaveSub(MessHandle messH);
void AddMessErrNote(MessHandle messH);
void PlaceMessErrNote(MessHandle messH);
Boolean SaveMessHi(MyWindowPtr win,Boolean closing);
Boolean AttIsSelected(MyWindowPtr win,PETEHandle pte,long startWith,long endWith,short what,long *start,long *stop);
Boolean TransferMenuChoice(short menu,short item,TOCHandle tocH,short sumNum,long modifiers,Boolean fcc);
OSErr AttLine2Spec(PStr line,FSSpecPtr spec,Boolean wantToOpen);
OSErr RelLine2Spec(PStr line,FSSpecPtr spec,uLong *cid, uLong *relURL, uLong *absURL);
short AddXlateTables(Boolean isOut,short nowId,Boolean ph,MenuHandle pmh);
void SetMessTable(TOCHandle tocH, short sumNum, short tableId);
RgnHandle MessBuildDragRgn(MessHandle messH);
Boolean Menu2TableId(TOCHandle tocH,MenuHandle pmh,short item, short *tableId);
void SetMessTable(TOCHandle tocH, short sumNum, short newId);
OSErr ExportHTMLSum(TOCHandle tocH,short sumNum);
OSErr ExportHTML(MessHandle messH);
#ifdef TWO
Boolean GetServerRect(MyWindowPtr win, short which, Rect *r);
#endif
short EzOpenFind(TOCHandle tocH, short origSum);
void EzOpen(TOCHandle tocH, short sumNum,uLong uidHash,long modifiers,Boolean hideFront,Boolean willDelete);
void Fcc(MessHandle messH,FSSpecPtr box);
short MessWi(MyWindowPtr win);
#define IsArrowSwitch(m)	(((m)&(shiftKey|optionKey|cmdKey|alphaLock|controlKey))==GetPrefLong(PREF_SWITCH_MODIFIERS))
#endif
Boolean GetMesgErrorsRect(MyWindowPtr win, Rect *r);
short GetMesgErrorsHeight(MyWindowPtr win);
#define MESG_ERR_WIDTH 32
OSErr IncrementQuoteLevel(PETEHandle pte,long startSel,long endSel,short increment);
pascal short SaveAsHook(short item,DialogPtr dgPtr, Ptr userData);
pascal void PetePaneDraw(ControlHandle cntl, SInt16 part);