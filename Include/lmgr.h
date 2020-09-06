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

#ifndef LMGR_H
#define LMGR_H

#include <Lists.h>

Boolean FirstSelected(Cell * c, ListHandle list);
short Next1Selected(short thisOne, ListHandle list);
short InWhich1Cell(Point mouse, ListHandle list);
Boolean Cell1Selected(short which, ListHandle list);
void SelectSingleCell(short which, ListHandle list);
void ResizeList(ListHandle lHand, Rect * r, Point c);
Boolean MyLClick(Point pt, short modifiers, ListHandle lHandle);
Boolean LClickWDrag(Point pt, short modifiers, ListHandle lHand,
		    Boolean * drag, void (*hook)(short cell,
						 ListHandle lHand));
RgnHandle MyLDragRgn(ListHandle lHand);
#define ldtInterstice 0x0001
#define ldtIgnoreSelection 0x0002
short MyLDragTracker(DragReference drag, Point pt, short flags,
		     ListHandle lHand);
Rect *Cell1Rect(ListHandle lHand, short cell, Rect * r);
Boolean ListApp1(EventRecord * event, ListHandle lHand);
void MyListHilite(Boolean lSelect, Rect * lRect, Cell lCell,
		  ListHandle lHandle);

// Glue to get backgrounds right
#define LAddColumn MyLAddColumn
#define LAddRow MyLAddRow
#define LDelColumn MyLDelColumn
#define LDelRow MyLDelRow
#define LSize MyLSize
#define LSetDrawingMode MyLSetDrawingMode
#define LScroll MyLScroll
#define LAutoScroll MyLAutoScroll
#define LUpdate MyLUpdate
#define LActivate MyLActivate
#define LCellSize MyLCellSize
#define LClick MyLClick
#define LAddToCell MyLAddToCell
#define LClrCell MyLClrCell
#define LSetCell MyLSetCell
#define LSetSelect MyLSetSelect
#define LDraw MyLDraw

short MyLAddColumn(short count, short colNum, ListRef lHandle);
short MyLAddRow(short count, short rowNum, ListRef lHandle);
void MyLDelColumn(short count, short colNum, ListRef lHandle);
void MyLDelRow(short count, short rowNum, ListRef lHandle);
void MyLSize(short listWidth, short listHeight, ListRef lHandle);
void MyLSetDrawingMode(Boolean drawIt, ListRef lHandle);
void MyLScroll(short dCols, short dRows, ListRef lHandle);
void MyLAutoScroll(ListRef lHandle);
void MyLUpdate(RgnHandle theRgn, ListRef lHandle);
void MyLActivate(Boolean act, ListRef lHandle);
void MyLCellSize(Point cSize, ListRef lHandle);
Boolean MyLClick(Point pt, short modifiers, ListRef lHandle);
void MyLAddToCell(const void *dataPtr, short dataLen, Cell theCell,
		  ListRef lHandle);
void MyLClrCell(Cell theCell, ListRef lHandle);
void MyLSetCell(const void *dataPtr, short dataLen, Cell theCell,
		ListRef lHandle);
void MyLSetSelect(Boolean setIt, Cell theCell, ListRef lHandle);
void MyLDraw(Cell theCell, ListRef lHandle);

#endif
