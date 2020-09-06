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

#include <conf.h>
#include <mydefs.h>

#include "lmgr.h"
#define FILE_NUM 49
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1992 by Qualcomm, Inc. */

#pragma segment LMgr

/************************************************************************
 * FirstSelected - find the first selection (if any)
 ************************************************************************/
Boolean FirstSelected(Cell *c,ListHandle list)
{
	c->h = c->v = 0;
	return(LGetSelect(True,c,list));
}

/************************************************************************
 * Next1Selected - get the index (plus 1) of the next selected cell
 *  in a 1-dimensional list.
 *  returns 0 if no (more) cells selected
 ************************************************************************/
short Next1Selected(short thisOne,ListHandle list)
{
	Point c;
	c.h = 0;
	c.v = thisOne;
	if (LGetSelect(True,&c,list)) return(c.v+1);
	else return(0);
}

/************************************************************************
 * InWhich1Cell - over which cell (in a 1dim list) is the point?
 ************************************************************************/
short InWhich1Cell(Point mouse,ListHandle list)
{
	Rect rView = (*list)->rView;
	short cellHi = (*list)->cellSize.v;
	short scroll = (*list)->visible.top;
	short cell;
	
	if (!PtInRect(mouse,&rView)) return(0);
	
	cell = scroll + (mouse.v-rView.top)/cellHi + 1;
	return(cell);
}


/************************************************************************
 * Cell1Selected - is a cell in a 1dim list selected?
 ************************************************************************/
Boolean Cell1Selected(short which,ListHandle list)
{
	Point pt;
	
	pt.h = 0; pt.v = which-1;
	return(LGetSelect(False,&pt,list));
}

/************************************************************************
 * SelectSingleCell - select just once cell in a list
 ************************************************************************/
void SelectSingleCell(short which,ListHandle list)
{
	Point c;
	
	c.h = c.v = 0;
	while (LGetSelect(True,&c,list)) LSetSelect(False,c,list);
	if (which>=0)
	{
		c.v = which;
		LSetSelect(True,c,list);
		LAutoScroll(list);
	}
}


/**********************************************************************
 * MyLDragRgn - make a drag region out of the selected cells
 **********************************************************************/
RgnHandle MyLDragRgn(ListHandle lHand)
{
	RgnHandle rgn;
	short s;
	Rect first,r;
	
	if (rgn = NewRgn())
	{
		s = 0;
		first = (*lHand)->rView;
		first.bottom = first.top + (*lHand)->cellSize.v;
		OffsetRect(&first,0,-(*lHand)->visible.top*(*lHand)->cellSize.v);
		
		while (s=Next1Selected(s,lHand))
		{
			r = first;
			OffsetRect(&r,0,(s-1)*(*lHand)->cellSize.v);
			RgnPlusRect(rgn,&r);
		}
		OutlineRgn(rgn,1);
		GlobalizeRgn(rgn);
	}
	return(rgn);
}
/**********************************************************************
 * MyLDragTracker - track a drag in a list rect
 **********************************************************************/
short MyLDragTracker(DragReference drag,Point pt,short flags,ListHandle lHand)
{
	Rect r = (*lHand)->rView;
	short scroll = 0;
	short theCell = 0;
	SAVE_STUFF;
	SET_COLORS;
	
	ASSERT(flags&ldtInterstice);
	
	if (pt.v<r.top) scroll = -1;
	else if (pt.v>r.bottom) scroll = 1;
	
	if (scroll)
	{
		HideDragHilite(drag);
		LScroll(0,scroll*(*lHand)->cellSize.v,lHand);
	}
	
	if (flags&ldtInterstice)
	{
		if (pt.v<r.top)
			theCell = 1;
		else if (pt.v>r.bottom)
			theCell = (*lHand)->dataBounds.bottom;
		else
			theCell = InWhich1Cell(pt,lHand);
			
		if (theCell>(*lHand)->dataBounds.bottom)
			theCell = (*lHand)->dataBounds.bottom;
		
		Cell1Rect(lHand,theCell,&r);
		
		if (pt.v>(r.top+r.bottom)/2)
		{
			r.top = r.bottom-1;
			r.bottom++;
		}
		else
		{
			theCell--;
			r.top--;
			r.bottom = r.top+2;
		}
		InsetRect(&r,-8,0);
		
		if (!(flags&ldtIgnoreSelection) &&
				(theCell==0 && Cell1Selected(1,lHand) ||
			  Cell1Selected(theCell,lHand) ||
			  theCell<(*lHand)->dataBounds.bottom && Cell1Selected(theCell+1,lHand)))
		{	
			HideDragHilite(drag);
			return(-1);
		}
		ShowDragRectHilite(drag,&r,True);
	}
	REST_STUFF;
	return(theCell);
}

/**********************************************************************
 * Cell1Rect - get the rectangle for a given (1-based, 1-dim) cell
 **********************************************************************/
Rect *Cell1Rect(ListHandle lHand,short cell,Rect *r)
{
	*r = (*lHand)->rView;
	r->bottom = r->top + (*lHand)->cellSize.v;
	OffsetRect(r,0,(*lHand)->cellSize.v * (cell-(*lHand)->visible.top-1));
	return(r);
}

/************************************************************************
 * ListApp1 - handle an App1 event for a list
 *  returns True if handled
 ************************************************************************/
Boolean ListApp1(EventRecord *event, ListHandle lHand)
{
	short page = RoundDiv((*lHand)->rView.bottom-(*lHand)->rView.top,(*lHand)->cellSize.v)-1;
	switch(event->message & charCodeMask)
	{
		case homeChar:
			page = -REAL_BIG/2;
			break;
		case endChar:
			page = REAL_BIG/2;
			break;
		case pageUpChar:
			page *= -1;
			break;
		case pageDownChar:
			break;
		default:
			return(False);
	}
	LScroll(0,page,lHand);
	return(True);
}

/**********************************************************************
 * LClickWDrag - handle a list click, but allow dragging
 **********************************************************************/
Boolean LClickWDrag(Point pt, short modifiers, ListHandle lHand, Boolean *drag,void (*hook)(short cell,ListHandle lHand))
{
	Point glob;
	short cell;
	
	*drag = False;
	glob = pt;
	LocalToGlobal(&glob);
	
	if ((*lHand)->rView.right-pt.h<GROW_SIZE) return(LClick(pt,modifiers,lHand));
	else
	{
		cell = InWhich1Cell(pt,lHand);
		if (hook) (*hook)(cell,lHand);
		if (*drag = MyWaitMouseMoved(glob,True)) return(False);
		else if (!hook) return (LClick(pt,modifiers,lHand));
	}
	return(False);
}

/**********************************************************************
 * ResizeList - resize a list
 **********************************************************************/
void ResizeList(ListHandle lHand,Rect *r,Point c)
{
	Rect oldR = (*lHand)->rView;
	WindowPtr	winWP = (*lHand)->port;
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	
	LSetDrawingMode(False,lHand);
	(*lHand)->rView = *r;
	(*lHand)->cellSize = c;
	LSize(r->right-r->left,r->bottom-r->top,lHand);
	LSetDrawingMode(True,lHand);
	InvalWindowRect(winWP,&oldR);
	InvalWindowRect(winWP,r);
	if (IsMyWindow(winWP)) win->dontGreyOnMe = *r;
}

/************************************************************************
 * MyListHilite - general hilite function for ldef's to use
 ************************************************************************/
void MyListHilite(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle)
{
	MyWindowPtr win = GetPortMyWindowPtr(GetQDGlobalsThePort());
	SAVE_STUFF;
	
	if (win) {
		RGBBackColor(&win->backColor);
		HiInvertRect(lRect);
	}
	REST_STUFF;
}

// List manager glue stuff
#undef LAddColumn
#undef LAddRow
#undef LDelColumn
#undef LDelRow
#undef LSize
#undef LSetDrawingMode
#undef LScroll
#undef LAutoScroll
#undef LUpdate
#undef LActivate
#undef LCellSize
#undef LClick
#undef LAddToCell
#undef LClrCell
#undef LSetCell
#undef LSetSelect
#undef LDraw

short MyLAddColumn(short count, short colNum, ListRef lHandle) {short val;SAVE_STUFF;SET_COLORS;val=LAddColumn(count, colNum, lHandle);REST_STUFF;return(val);}
short MyLAddRow(short count, short rowNum, ListRef lHandle) {short val;SAVE_STUFF;SET_COLORS;val=LAddRow(count, rowNum, lHandle);REST_STUFF;return(val);}
void MyLDelColumn(short count, short colNum, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LDelColumn(count,  colNum,  lHandle);REST_STUFF;}
void MyLDelRow(short count, short rowNum, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LDelRow( count,  rowNum,  lHandle);REST_STUFF;}
void MyLSize(short listWidth, short listHeight, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LSize( listWidth,  listHeight,  lHandle);REST_STUFF;}
void MyLSetDrawingMode(Boolean drawIt, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LSetDrawingMode( drawIt,  lHandle);REST_STUFF;}
void MyLScroll(short dCols, short dRows, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LScroll( dCols,  dRows,  lHandle);REST_STUFF;}
void MyLAutoScroll(ListRef lHandle) {SAVE_STUFF;SET_COLORS;LAutoScroll( lHandle);REST_STUFF;}
void MyLActivate(Boolean act, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LActivate( act,  lHandle);REST_STUFF;}
void MyLCellSize(Point cSize, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LCellSize( cSize,  lHandle);REST_STUFF;}
Boolean MyLClick(Point pt, short modifiers, ListRef lHandle) {Boolean val;SAVE_STUFF;SET_COLORS;val=LClick( pt,  modifiers,  lHandle)&&!(modifiers&cmdKey);REST_STUFF;return(val);}
void MyLAddToCell(const void *dataPtr, short dataLen, Cell theCell, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LAddToCell( dataPtr,  dataLen,  theCell,  lHandle);REST_STUFF;}
void MyLClrCell(Cell theCell, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LClrCell( theCell,  lHandle);REST_STUFF;}
void MyLSetSelect(Boolean setIt, Cell theCell, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LSetSelect( setIt,  theCell,  lHandle);REST_STUFF;}
void MyLDraw(Cell theCell, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LDraw( theCell,  lHandle);REST_STUFF;}
void MyLSetCell(const void *dataPtr, short dataLen, Cell theCell, ListRef lHandle) {SAVE_STUFF;SET_COLORS;LSetCell(dataPtr,  dataLen,  theCell,  lHandle);REST_STUFF;}
void MyLUpdate(RgnHandle theRgn, ListRef lHandle)
{
	GrafPtr origPort;
	Rect r=(*lHandle)->rView;
	RgnHandle sectRgn = NewRgn();
	SAVE_STUFF;
	SET_COLORS;
	if (sectRgn)
	{
		if (theRgn)
		{
			RectRgn(sectRgn,&r);
			SectRgn(sectRgn,theRgn,sectRgn);
			GetPort(&origPort);
			SetPort((**lHandle).port);
			EraseRgn(sectRgn);
			SetPort(origPort);
		}
		DisposeRgn(sectRgn);
	}
	LUpdate( theRgn,  lHandle);
	REST_STUFF;
}
