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

#ifndef BOXACT_H
#define BOXACT_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#define kBoxSizeRefCon 'boxs'
#define kDrawerSwitch 'dwrs'
#define kConConProfRefCon 'ccpp'

/**********************************************************************
 * function prototypes
 **********************************************************************/
void BoxUpdate(MyWindowPtr win);
void BoxClick(MyWindowPtr win,EventRecord *event);
void BoxActivate(MyWindowPtr win);
Boolean BoxMenu(MyWindowPtr win,int menu,int item,short modifiers);
Boolean BoxClose(MyWindowPtr win);
Boolean BoxFind(MyWindowPtr win,PStr what);
void BoxOpen(MyWindowPtr win);
Boolean BoxKey(MyWindowPtr win, EventRecord *event);
void SelectBoxRange(TOCHandle tocH,int start,int end,Boolean cmd,int eStart,int eEnd);
void BoxSetSummarySelected(TOCHandle tocH,short sumNum,Boolean selected);
void BoxCenterSelection(MyWindowPtr win);
void BoxSelectAfter(MyWindowPtr win, short mNum);
Boolean BoxPosition(Boolean save,MyWindowPtr win);
void BoxSelectSame(TOCHandle tocH,short item,short clickedSum);
void BoxLabelMenu(TOCHandle tocH,short mNum,MessHandle messH,Point pt);
void MakeMessFileName(TOCHandle tocH,short sumNum, UPtr name);
void BoxHelp(MyWindowPtr win, Point mouse);
void BoxCursor(Point mouse);
void BoxDidResize(MyWindowPtr win, Rect *oldContR);
void BoxGrowSize(MyWindowPtr win, Point *newSize);
void BoxPlaceBevelButtons(MyWindowPtr win);
void BoxListFocus(TOCHandle tocH,Boolean focus);
#define SortedDescending(tocH) (((*(tocH))->sorts[blDate-1]&3)==SORT_DESCEND)
OSErr BoxGonnaShow(MyWindowPtr win);
UPtr PriorityString(UPtr string,Byte priority);
void SetPriority(TOCHandle tocH,short sumNum,short priority);
#define Prior2Display(p) ((p)?RoundDiv(MIN((p),200),40):3)
#define Display2Prior(p) ((p)*40)
void SetTAEScore(TOCHandle tocH,short sumNum,short score);
short Item2Status(short item);
short Status2Item(short status);
void InvalTocBox(TOCHandle tocH,short sumNum,short box);
Boolean RedoTOC(TOCHandle tocH);
short BoxLimits(BoxLinesEnum which,short *left,short *right,TOCHandle tocH);
void DragXfer(Point pt, TOCHandle tocH,MessHandle messH);
void RedoAllTOCs(void);
void SelectOtherBox(Point pt,TOCHandle tocH);
void MBResort(TOCHandle tocH);
OSErr FinishBoxDrag(void);
void CheckSortItems(MyWindowPtr win);
OSErr InterpretSortString(PStr s,Boolean *group,Byte *sorts,PStr menuItem);
TextAddrHandle MenuItem2Handle(short menu, short item);
short StatusMenu(MyWindowPtr win,short origStatus,Point where);
void ShowBoxSizes(MyWindowPtr win);
#define InvalBoxSizeBox ShowBoxSizes
short EnableServerItems(MyWindowPtr win,short selectedSum,Boolean all,Boolean shiftPressed);
void ServerMenuChoice(TOCHandle tocH,short sumNum,short item,Boolean shiftPressed);
void BeenThereDoneThat(TOCHandle tocH,short sumNum);
void BoxAddBevelButtons(MyWindowPtr win);
void BoxButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
Boolean BoxScroll(MyWindowPtr win,short h,short v);
Boolean BoxHasSelection(MyWindowPtr win);
OSErr BoxDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
void BoxIdle(MyWindowPtr win);
PStr StationMenu(PStr name,Point where);
void BoxInitialSelection(TOCHandle tocH);
void AnalBox(TOCHandle tocH,short first, short last);
void BoxInversionSetup(void);
Boolean BoxColumnShowing(BoxLinesEnum which,TOCHandle tocH);
int SubjCompare(PStr s1, PStr s2);
void InvalTocBoxLo(TOCHandle tocH,short sumNum,short box);
void SetFileView(MyWindowPtr win,TOCHandle tocH, Boolean fileView);
Boolean MultiMessageOpOK(short fmt,short count);
void BoxSetBevelButtonValues(MyWindowPtr win);
PStr BoxPreviewProfile(PStr profileName,TOCHandle tocH,short previewTypeID);
PStr Sort2String(PStr s,TOCHandle tocH);
void ApplySortString(TOCHandle tocH,PStr s);
#endif