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

#include "floatingwin.h"
#define FILE_NUM 89
// Copyright � 1997 by QUALCOMM Incorporated
/**********************************************************************
 * code to support floating windows
 **********************************************************************/

#ifdef	FLOAT_WIN

#pragma segment MyWindow

void DockedDragProc(Point *pt,RgnHandle winRgn,long refCon);
void GetScreenRect(WindowPtr theWindow, Rect *rWin, Rect *rScreen);
MyWindowPtr FindLastFloaterLo(short *count,Boolean *layerProblem);

/**********************************************************************
 * MySelectWindow - patch for SelectWindow that makes sure floaters
 * remain on top, and non floaters remain underneath them.
 **********************************************************************/
void MySelectWindow(WindowPtr winWP)
{
	SelectWindow(winWP);
}

/**********************************************************************
 * FindLastFloater - returns a pointer to the last floater
 **********************************************************************/
MyWindowPtr FindLastFloater(void)
{
	short	count;
	Boolean	layerProblem;
	
	return FindLastFloaterLo(&count,&layerProblem);
}

/**********************************************************************
 * FindLastFloaterLo - returns a pointer to the last floater and counts
 *		number of floaters
 **********************************************************************/
MyWindowPtr FindLastFloaterLo(short *count,Boolean *layerProblem)
{
	WindowPtr	wp,
						lastFloater = nil;
	Boolean	nonFloaterFound = false;
	
	*count = 0;
	*layerProblem = false;
	
	for (wp = GetWindowList (); wp; wp = GetNextWindow (wp))
		if (IsFloating(wp))
		{
			lastFloater=wp;
			(*count)++;
			if (nonFloaterFound) *layerProblem = true;
		}
		else if ((GetWindowKind(wp)!=dialogKind && !IsModalPlugwindow(wp)) || IsModelessDialog (wp)) nonFloaterFound = true;
		
	return (GetWindowMyWindowPtr(lastFloater));
}

/**********************************************************************
 * MyDragWindow - patch for DragWindow to properly draw drag outlines.
 **********************************************************************/
void MyDragWindow (WindowPtr winWP, Point start, Rect *boundsRect)
{
	DragWindow(winWP, start, boundsRect);
	PositionDockedWindow(winWP);
}

/**********************************************************************
 * DockedDragProc - callback for dragging docked window around edge of screen
 **********************************************************************/
void DockedDragProc(Point *pt,RgnHandle winRgn,long refCon)
{
	short	dx,dy,dx1,dy1,dx2,dy2;
	Rect	rWin,rScreen;
	WindowPtr	theWindow = (WindowPtr) refCon;
	
	//	Get bounds of window at new location
	GetRegionBounds(winRgn,&rWin);
	OffsetRect(&rWin,pt->h-rWin.left,pt->v-rWin.top);

	GetScreenRect(theWindow,&rWin,&rScreen);
	
	dx1 = rWin.left - rScreen.left;
	dy1 = rWin.top - rScreen.top;
	dx2 = rScreen.right - rWin.right;
	dy2 = rScreen.bottom - rWin.bottom;	
	dx = dy = 0;

	//	Make sure we're on screen
	if (dx1<0) dx = -dx1;
	else if (dx2 < 0) dx = dx2;
	if (dy1<0) dy = -dy1;
	else if (dy2 < 0) dy = dy2;
	
	if (!dx && !dy)
	{
		//	Move to edge. Find the edge we are closest to
		short	dTop,dLeft,dBottom,dRight;

		dTop = rWin.top - rScreen.top;
		dLeft = rWin.left - rScreen.left;
		dBottom = rScreen.bottom - rWin.bottom;
		dRight = rScreen.right - rWin.right;
		if (dLeft < dTop && dLeft < dRight && dLeft < dBottom)
			dx = -dx1;	//	Move to left edge
		else if (dTop < dRight && dTop < dBottom)
			dy = -dy1;	//	Move to top edge
		else if (dRight < dBottom)
			dx = dx2;	//	Move to right edge
		else if (dRight == dBottom)
		{
			dy = dy2;	//	Move to bottom/right corner
			dx = dx2;
		}
		else
			dy = dy2;	//	Move to bottom
	}
	
	//	Stay away from any other docked windows
	if (theWindow)
	{
		WindowPtr	winWP;
		MyWindowPtr	win;
		OffsetRect(&rWin,dx,dy);
		
		for (winWP = FrontWindow (); winWP; winWP = GetNextWindow (winWP))
		{
 			win = GetWindowMyWindowPtr (winWP);
 			if (winWP!=theWindow && IsWindowVisible (winWP) && IsFloating(winWP) && win->windowType==kDockable)
			{
				Rect rSect,rDocked;

				GetWindowStructureBounds(winWP,&rDocked);
				if (SectRect(&rWin,&rDocked,&rSect))
				{
					//	Overlap with this docked window
					Boolean	roomAbove = rDocked.top-rScreen.top >= RectHi(rWin);
					Boolean	roomBelow = rScreen.bottom-rDocked.bottom > RectHi(rWin);
					Boolean	roomLeft = rDocked.left-rScreen.left > RectWi(rWin);
					Boolean roomRight = rScreen.right-rDocked.right > RectWi(rWin);
					Boolean moveLeft = (rWin.left+rWin.right)/2 < (rDocked.left+rDocked.right)/2;
					Boolean moveAbove = (rWin.top+rWin.bottom)/2 < (rDocked.top+rDocked.bottom)/2;
					Boolean moveVertical = RectHi(rSect) < RectWi(rSect);

					if (moveVertical)
					{
						if (!roomAbove && !roomBelow)
							moveVertical = false;
					}
					else if (!roomLeft && !roomRight)
						moveVertical = true;
						
					if (moveVertical)
					{
						//	Move above or below
						if (moveAbove)
						{
							if (!roomAbove) moveAbove = false;
						}
						else
							if (!roomBelow) moveAbove = true;
						
						if (moveAbove)
							dy -= rWin.bottom-rDocked.top;	//	Move above
						else
							dy += rDocked.bottom-rWin.top;	//	Move below
					}
					else
					{
						//	Move left or right
						if (moveLeft)
						{
							if (!roomLeft) moveLeft = false;
						}
						else
							if (!roomRight) moveLeft = true;
						
						if (moveLeft)												
								dx -= rWin.right-rDocked.left;	//	Move left
						else
							dx += rDocked.right-rWin.left;	//	Move right
					}						
				}
			}
		}
	}
	
	pt->h += dx;
	pt->v += dy;
}

/**********************************************************************
 * GetScreenRect - get rect of screen containing specified window
 **********************************************************************/
void GetScreenRect(WindowPtr theWindow, Rect *rWin, Rect *rScreen)
{
	GDHandle	gd;

	if (theWindow && GetWindowKind(theWindow)==AD_WIN)
		//	If ad window, constrain to main screen
		gd = MyGetMainDevice();
	else
	{
		utl_GetRectGD(rWin,&gd);
		if (!gd)
			//	Not on any screen! Use main screen.
			gd = MyGetMainDevice();
	}
	
	// Get screen rect minus menu bar, dock, etc.
	GetAvailableWindowPositioningBounds(gd,rScreen);
}

/**********************************************************************
 * PositionDockedWindow - make sure docked window is positioned correctly
 **********************************************************************/
void PositionDockedWindow(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	Point	pt;
	RgnHandle	rgn = nil;
	Rect	rStruc,rCont;
	
	if (!win || !IsMyWindow(winWP) || win->windowType != kDockable || !IsWindowVisible (winWP)) return;
	
	GetWindowStructureBounds(winWP,&rStruc);
	pt.v = rStruc.top;
	pt.h = rStruc.left;
	DockedDragProc(&pt,rgn?rgn:MyGetWindowStructureRegion(winWP),(long)winWP);
	if (rgn) DisposeRgn(rgn);
	if (pt.h!=rStruc.left || pt.v!=rStruc.top)
	{
		//	pt is location of strucRgn. Move region needs location of contRgn
		GetWindowContentBounds(winWP,&rCont);
		MoveWindow(winWP,pt.h+rCont.left-rStruc.left,pt.v+rCont.top-rStruc.top,false);
	}
}

/**********************************************************************
 * PositionDockedWindows - make sure docked windows are positioned correctly
 **********************************************************************/
void PositionDockedWindows(void)
{
	WindowPtr	theWindow;
	
	for (theWindow = GetWindowList(); theWindow; theWindow = GetNextWindow (theWindow))
		PositionDockedWindow(theWindow);
}

/**********************************************************************
 * DockWinReduce - reduce a rect to not include docked windows
 **********************************************************************/
void DockedWinReduce(WindowPtr checkWinWP, Rect *winRect, Rect *r)
{
	WindowPtr	winWP;
	MyWindowPtr	win;
	Rect	rWin;
	
	if (winRect) rWin = *winRect;
	else if (checkWinWP) rWin = CurState(checkWinWP);
	
	for (winWP = GetWindowList (); winWP; winWP = GetNextWindow (winWP))
	{
		win = GetWindowMyWindowPtr (winWP);
		if (IsKnownWindowMyWindow(winWP) && win && (win)->windowType == kDockable && IsWindowVisible (winWP))
		{
			Rect	rDocked,rSect;
			
			GetWindowStructureBounds(winWP,&rDocked);
			if (SectRect(&rDocked,checkWinWP?&rWin:r,&rSect))
			{
				//	The window intersects a docked window
				Rect	rScreen;
				enum { kNone,kTop,kLeft,kBottom,kRight } where;
				short	minDelta = REAL_BIG;
				
				GetScreenRect(checkWinWP,&rDocked,&rScreen);
				
				//	Determine which screen edge we need to move away from
				//	Use the one that's the shortest distance
				if (rDocked.bottom - rScreen.top < minDelta)
				{
					minDelta = rDocked.bottom - rScreen.top;
					where = kTop;
				}
				if (rDocked.right - rScreen.left < minDelta)
				{
					minDelta = rDocked.right - rScreen.left;
					where = kLeft;
				}
				if (rScreen.bottom - rDocked.top < minDelta)
				{
					minDelta = rScreen.bottom - rDocked.top;
					where = kBottom;
				}
				if (rScreen.right - rDocked.left < minDelta)
				{
					minDelta = rScreen.right - rDocked.left;
					where = kRight;
				}

				switch (where)
				{
					case kTop:
						if (r->top < rDocked.bottom)
						{
							r->top = rDocked.bottom + 2;
							OffsetRect(&rWin,0,rDocked.bottom+2-rWin.top);
						}
						break;
					case kLeft:
						if (r->left < rDocked.right)
						{
							r->left = rDocked.right + 2;
							OffsetRect(&rWin,rDocked.right+2-rWin.left,0);
						}
						break;
					case kBottom:
						if (r->bottom > rDocked.top) r->bottom = rDocked.top - 2;
						break;
					case kRight:
						if (r->right > rDocked.left) r->right = rDocked.left - 2;
						break;
				}
			}
		}
	}
}

/**********************************************************************
 * DockedWinRemove - subtract docked windows from a region
 **********************************************************************/
void DockedWinRemove(RgnHandle rgn,WindowPtr ignoreWinWP)
{
	WindowPtr	winWP;
	MyWindowPtr	win;
	
	for (winWP = GetWindowList (); winWP; winWP = GetNextWindow (winWP))
	{
		win = GetWindowMyWindowPtr (winWP);
		if (winWP!=ignoreWinWP && IsKnownWindowMyWindow(winWP) && win && (win)->windowType == kDockable)
		{
			Rect	rDocked;
			
			GetWindowStructureBounds(winWP,&rDocked);
			RgnMinusRect(rgn,&rDocked);
		}
	}
}

/**********************************************************************
 * Returns true if this is the frontmost, non floating window
 **********************************************************************/
Boolean IsTopNonFloater(WindowPtr theWindow)
{
	return (theWindow == MyFrontNonFloatingWindow());
}
/**********************************************************************
 * Return a pointer to the frontmost, non floating, visible window.
 *
 * In OS X, invisible windows can be at the head of the window list.
 * Eudora opens a lot of invisible windows, and assumes they're not
 * in the list at all.  This routine exists to help work around this
 * development, and return the topmost, VISIBLE window.  It should
 * replace calls to FrontWindow_() when we're looking for the frontmost
 * visible window.
 *
 * Changing MyFrontNonFloatingWindow() to return the topmost, visible, 
 * non-floating window should be perfectly safe.  In the classic
 * version, MyFrontNonFloatingWindow NEVER returns an invisible window.
 **********************************************************************/
WindowPtr MyFrontNonFloatingWindow(void)
{
	return FrontNonFloatingWindow();
}


/**********************************************************************
 * Return a pointer to the window that is topmost on this part of the
 * screen.  Return 0 if there are no windows here, or those that are
 * aren't floating windows.
 **********************************************************************/
MyWindowPtr FloaterAtPoint(Point mouse)
{
	WindowPtr aFloater = NULL;
	Rect r;
	
	LocalToGlobal(&mouse);
	
	if (IsFloating(FrontWindow()))
	{
		aFloater = FrontWindow();
		
		while (aFloater && IsFloating(aFloater))
		{
			GetWindowContentBounds(aFloater,&r);
			if (PtInRect(mouse,&r)) return (GetWindowMyWindowPtr(aFloater));
			aFloater = GetNextWindow(aFloater);
		}
	}
	
	return (nil);
}

/**********************************************************************
 * Hide a window, but activate the next non-floating window.
 **********************************************************************/
void MyHideWindow(WindowPtr theWindow)
{

	HideWindow(theWindow);
	WindowIsInvisibleClassic(theWindow);
	if (IsFloating(theWindow)) SetKeyFocusedFloater(nil);
	if (!IsFloating(theWindow) && GetNextWindow(theWindow)) ActivateMyWindow(theWindow, !InBG);
}

/**********************************************************************
 * IsFloating - returns true if this window floats above others.  
 * Currently this includes kFloating and kDockable windows.
 **********************************************************************/
Boolean IsFloating(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);

	return (IsKnownWindowMyWindow(winWP) && win && ((win->windowType == kFloating) || (win->windowType == kDockable)));
} 

/**********************************************************************
 * SetKeyFocusFloater - sets floating window to receive key focus if window supports keys
 **********************************************************************/
void SetKeyFocusedFloater(MyWindowPtr win)
{
	ASSERT(!keyFocusedFloater || win && keyFocusedFloater==win);
	//	Don't allow ad window to receive focus although it has a Pete handle
	keyFocusedFloater = win && win->pte && GetWindowKind (GetMyWindowWindowPtr (win)) != AD_WIN ? win : nil;
}

/**********************************************************************
 * FloatingWinIdle - periodically make sure no non-floaters are on top
 *   the only windows I'm aware of that can do this are plug-in windows
 **********************************************************************/
void FloatingWinIdle(void)
{
}


#endif	//FLOAT_WIN
