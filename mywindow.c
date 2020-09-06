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

#include "mywindow.h"
#define FILE_NUM 28
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment MyWindow
#define W_INSET 1
#define W_MIN 72

void EraseAndSeparate(MyWindowPtr win);
void SizeTopMargin(MyWindowPtr win);
MyWindowPtr FindAdWin(void);
pascal void DrawWindowHeader(ControlHandle control, SInt16 part);
pascal void LabelDraw(ControlHandle cntl, SInt16 part);
static OSStatus WindowEvents(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData);

/**********************************************************************
 * GetNewMyWindow - create a new window from a template
 **********************************************************************/
MyWindowPtr GetNewMyWindow(short resId,void *wStorage,MyWindowPtr win,WindowPtr behind,Boolean hBar, Boolean vBar, short windowKind)
{
	return GetNewMyWindowWithClass(resId,wStorage,win,behind,hBar, vBar, windowKind, 0);
}

/**********************************************************************
 * GetNewMyWindowLo - create a new MyWindow
 **********************************************************************/
// (jp) We've grown a new parameter, win, to support memory preflighting for
//			a world of opaque data structures.  We formerly passed in a pointer to an
//			extended WindowRecord in 'wStorage'... which we won't be able to do under
//			Carbon.  In fact, under Carbon we can't preflight memory at all -- so this
//			will have to eventually change again.  For the time being, we're passing in
//			pre-allocated storage for both the WindowRecord and our own window structure.
MyWindowPtr GetNewMyWindowWithClass(short resId,void *wStorage,MyWindowPtr win,WindowPtr behind,Boolean hBar, Boolean vBar, short windowKind, WindowClass winClass)
{
	MyWindowPtr	wazooWin;
	WindowPtr	theWindow;
	Rect oldContR;
	Boolean	fIsWazoo;
	ControlHandle junk;
	DECLARE_UPP(ScrollAction,ControlAction);
	DECLARE_UPP(WindowEvents,EventHandler);
	EventTypeSpec winEventList[] = 
	{ 
		{kEventClassWindow, kEventWindowResizeStarted },
		{kEventClassWindow, kEventWindowBoundsChanged },
		{kEventClassWindow, kEventWindowResizeCompleted } 
	};

	wazooWin = GetNewWazoo(windowKind,&fIsWazoo);
	
	INIT_UPP(ScrollAction,ControlAction);
	INIT_UPP(WindowEvents,EventHandler);
	if (!wazooWin)
	{
		if (HandyMyWindow)
		{
			win = HandyMyWindow;
			HandyMyWindow = nil;
		}
		else if ((win=New(MyWindow))==nil)
			return (nil);
		WriteZero(win, sizeof(MyWindow));
			
		if (winClass)
		{
			Rect	rBounds;
			Str32	wTitle;
			Handle	hWIND;
			OSErr	err;
			
			if (hWIND = GetResource_('WIND', resId))
			{
				rBounds = *(Rect*)*hWIND;
				PStrCopy(wTitle,*hWIND+18,sizeof(wTitle));
			}
			else
			{
				SetRect(&rBounds,10,10,50,50);	//	We will resize later
				*wTitle = 0;
			}

			if (winClass == kToolbarWindowClass)
			{
				// For the toolbar, use a dialog def proc so we get a border around the window
				WindowDefSpec def;
				
				def.defType = kWindowDefProcID;
				def.u.procID = kWindowPlainDialogProc;
				err = CreateCustomWindow(&def,winClass,kWindowNoAttributes,&rBounds,&theWindow);
			}
//			else CK
//				err = CreateNewWindow(winClass,winClass==kDrawerWindowClass?kWindowCompositingAttribute+kWindowResizableAttribute:kWindowNoAttributes,&rBounds,&theWindow); CK

			if (err)
			{
				ZapPtr (win);
				return(nil);
			}
			SetWTitle(theWindow,wTitle);
		}
		else
			theWindow = (ThereIsColor?GetNewCWindow(resId, wStorage, behind):GetNewWindow(resId, wStorage, behind));

		if (theWindow==nil) { ZapPtr (win); return(nil); }
		
		SetWindowMyWindowPtr (theWindow, win);
		win->theWindow = theWindow;
		
		SetPort_(GetWindowPort(theWindow));
		win->isDialog = False;
		win->windowType = kStandard;
		UsingWindow(theWindow);
		win->windex = ++Windex;

		CreateRootControl(theWindow,&junk);
		SetWindowKind (theWindow, windowKind);		//	Needed for PromoteToWazoo
		if (fIsWazoo)
			PromoteToWazoo(win);
	}
	else {
		win = wazooWin;
		theWindow = GetMyWindowWindowPtr (wazooWin);
	}
	
	SetPort_(GetWindowPort(theWindow));
	SetWindowKind (theWindow, windowKind);

	if (ThereIsColor)
	{
		GetRColor(&win->textColor,TEXT_COLOR);
		GetRColor(&win->backColor,BACK_COLOR);
		RGBBackColor(&win->backColor);
	}
				
	/*
	 * add scroll bar(s)
	 */
	if (hBar)
	{
		if (!(win->hBar = GetNewControl_(PrefIsSet(PREF_LIVE_SCROLL) ? LIVE_SCROLL_CNTL : SCROLL_CNTL, theWindow)))
			WarnUser(NO_CONTROL,ResError());
		else
		{
			SetControlAction(win->hBar,ScrollActionUPP);
			SetControlMinimum(win->hBar,0);
			SetControlValue(win->hBar,0);
			SetControlMaximum(win->hBar,0);
		}
	}

	if (vBar)
	{
		if (!(win->vBar = GetNewControl_(PrefIsSet(PREF_LIVE_SCROLL) ? LIVE_SCROLL_CNTL : SCROLL_CNTL, theWindow)))
			WarnUser(NO_CONTROL,ResError());
		else
		{
			SetControlAction(win->vBar,ScrollActionUPP);
			SetControlMinimum(win->vBar,0);
			SetControlValue(win->vBar,0);
			SetControlMaximum(win->vBar,0);
		}
	}

	win->hPitch = FontWidth;
	win->vPitch = FontLead;
	win->minSize.h = win->minSize.v = W_MIN;
	TextFont(FontID);
	TextSize(FontSize);
	SetWindowKind (theWindow, windowKind);
	
	win->titleBarHi = win->uselessHi = win->leftRimWi = win->uselessWi = -1;
	
	/*
	 * do size-related computations
	 */
	WriteZero(&oldContR,sizeof(Rect));
	MyWindowDidResize(win,&oldContR);
	
	/*
	 * hooray!
	 */
	win->isActive = FALSE;		/* let the activation event handle this */
	
	// Add support for live resizing
	if (!PrefIsSet(PREF_NO_LIVE_RESIZE))
	{
		InstallWindowEventHandler(theWindow, WindowEventsUPP, GetEventTypeCount(winEventList), winEventList, win, NULL);
		ChangeWindowAttributes(theWindow, kWindowLiveResizeAttribute, kWindowNoAttributes);
	}
	
	return(win);
}

/************************************************************************
 * LabelDraw - draw the label color
 ************************************************************************/
pascal void LabelDraw(ControlHandle cntl, SInt16 part)
{
#pragma unused(part)
	Str255 s;
	RGBColor c;
	MyWindowPtr win = GetWindowMyWindowPtr(GetControlOwner(cntl));
	Rect r;
	
	if (win->label)
	{
		MyGetLabel(win->label,&c,s);
		if (!BlackWhite(&c))
		{
			SAVE_STUFF;
			if (!IsControlActive(cntl)) LightenColor(&c,50);
			RGBForeColor(&c);
			GetControlBounds(cntl,&r);
			PaintRect(&r);
			REST_STUFF;
		}
	}
}

/************************************************************************
 * SetTopMargin - set the top margin of a window
 ************************************************************************/
void SetTopMargin(MyWindowPtr win,short h)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	DECLARE_UPP(DrawWindowHeader,ControlUserPaneDraw);
	DECLARE_UPP(LabelDraw,ControlUserPaneDraw);
	
	if (h!=win->topMargin)
	{
		InvalTopMargin(win);
		win->topMargin = h;
		win->contR.top = h;
		if (h)
		{
			if (!win->topMarginCntl)
			{
				Zero(r);
				win->topMarginCntl = GetNewControlSmall(gGoodAppearance ? TOPMARGIN_CNTL:BAD_TOPMARGIN_CNTL,winWP);
				if (win->topMarginCntl)
				{
					if (!gGoodAppearance)
					{
						//	We're using a user pane to draw the window header because of some problems with window headers
						//	with Appearance 1.0.
						INIT_UPP(DrawWindowHeader,ControlUserPaneDraw);
						SetControlData(win->topMarginCntl,0,kControlUserPaneDrawProcTag,sizeof(DrawWindowHeaderUPP),(Ptr)&DrawWindowHeaderUPP);
					}
					win->labelCntl = NewControlSmall(winWP,&r,"\p",True,0,0,0,kControlUserPaneProc,0);
					if (win->labelCntl)
					{
						INIT_UPP(LabelDraw,ControlUserPaneDraw);
						SetControlData(win->labelCntl,0,kControlUserPaneDrawProcTag,sizeof(LabelDrawUPP),(void*)&LabelDrawUPP);
						EmbedControl(win->labelCntl,win->topMarginCntl);
					}
				}
			}
			SizeTopMargin(win);
		}
		else if (win->topMarginCntl)
		{
			DisposeControl(win->topMarginCntl);
			win->topMarginCntl = nil;
			win->labelCntl = nil;
		}
		InvalTopMargin(win);
	}
}

/************************************************************************
 * SetBotMargin - set the bottom margin of a window
 ************************************************************************/
void SetBotMargin(MyWindowPtr win,short h)
{
	if (h!=win->topMargin)
	{
		WindowPtr	winWP = GetMyWindowWindowPtr (win);
		Rect		rPort;
		GetWindowPortBounds(winWP,&rPort);
		InvalBotMargin(win);
		win->botMargin = h;
		win->contR.bottom = rPort.bottom-h;
		InvalBotMargin(win);
	}
}

/************************************************************************
 * SizeTopMargin - size the topmargin control
 ************************************************************************/
void SizeTopMargin(MyWindowPtr win)
{
	Rect r;
	
	if (win->topMargin && win->topMarginCntl)
	{
		WindowPtr	winWP = GetMyWindowWindowPtr (win);
		Rect		rPort;
		PushGWorld();
		SetPort_(GetWindowPort(winWP));
		GetWindowPortBounds(winWP,&rPort);
		MoveMyCntl(win->topMarginCntl,-1,-1,RectWi(rPort)+2,win->topMargin+1);
		if (win->labelCntl)
		{
			GetControlBounds(win->topMarginCntl,&r);
			InsetRect(&r,MAX_APPEAR_RIM,MAX_APPEAR_RIM);
			r.bottom = r.top + 4;
			MySetCntlRect(win->labelCntl,&r);
		}
		PopGWorld();
	}
}
	
/**********************************************************************
 * UpdateMyWindow - handle an update event for a window
 **********************************************************************/
void UpdateMyWindow(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	int maxH, maxV;
	Rect updRect;
	PETEHandle pte;
#if __profile__	 	
	static Boolean profileUpdates;
#endif

	// Defensive programming for the masses and a little debugging for ourselves
	ASSERT(winWP && win);
	if (!winWP || !win)
		return;

	//Dprintf("\pStart %x;g",win);
	if (win->noUpdates)
		return;
	
	if (win->updating)
		//	Recursive update. Ignore for now.
		return;
	
	if (!IsWindowVisible(winWP))
	{
		//	If window is invisible, can ignore updates
		Rect	r;
		SetRect(&r,-REAL_BIG,-REAL_BIG,REAL_BIG,REAL_BIG);
		ValidWindowRect(winWP,&r);
		return;
	}

	GetUpdateRgnBounds(winWP,&updRect);
	
	InfiniteClip(GetWindowPort(winWP));
	
	/*
	 * set the port
	 */
	PushGWorld();
	SetPort_(GetWindowPort(winWP));
	
	win->updating = true;
	
	/*
	 * let the window manager move regions for us
	 */
	BeginUpdate(winWP);
	
	if (IsMyWindow(winWP) && win && !(IsWindowVisible(winWP) && (updRect.bottom<=updRect.top || updRect.left>=updRect.right)))
	{
#if __profile__	 
	if (profileUpdates) ProfilerClear();
#endif
		/*
		 * make life normal
		 */
		//WinGreyBG(win);
#if TARGET_CPU_PPC
		if ((long)NormalizeThemeDrawingState != kUnresolvedCFragSymbolAddress)
		{
			short oldFont = GetPortTextFont(GetQDGlobalsThePort());
			short oldSize = GetPortTextSize(GetQDGlobalsThePort());
			if (!win->isDialog) NormalizeThemeDrawingState();
			TextFont(oldFont);
			TextSize(oldSize);
		}
		else
#endif
			PenNormal();
		
		EraseAndSeparate(win);
		
		//	See if we need to do any wazoo preliminaries
		WazooPreUpdate(win);

		/*
		 * redraw the contents
		 */
		maxH = win->hMax; maxV = win->vMax;
		if (win->update) (*win->update)(win);

		/*
		 * draw the controls, etc.
		 */
		if (!win->isDialog)
		{
		  if (!win->dontDrawControls) UpdateControls(winWP,MyGetPortVisibleRegion(GetWindowPort(winWP)));
		}
		else
		{
			if (!win->update)
				DrawDialog(GetDialogFromWindow(winWP));
		}

		RGBBackColor(&win->backColor);
		for (pte=win->pteList;pte;pte=PeteNext(pte))
			PeteUpdate(pte);

		DrawSponsorAd(win);
#if __profile__
	{
		Str255	sTitle,sFile;
		static short count;
		MyGetWTitle(winWP,sTitle);
		if (*sTitle > 15) *sTitle = 15;
		ComposeString(sFile,"\pEudora Update %p",sTitle);
		if (profileUpdates)
			ProfilerDump(sFile);
//		Dprintf("\pUpdate %d %d %d %d %p %d",updRect.top,updRect.left,updRect.bottom,updRect.right,sTitle,count++);
	}
#endif
	}
	
	/*
	 * put stuff back
	 */
	EndUpdate(winWP);
	
	/*
	 * the update routine may have necessitated a change in the scroll bars
	 */
	if (IsMyWindow(winWP) && win && win->isActive)
	{
		if  (win->hBar && maxH!=win->hMax) Draw1Control(win->hBar);
		if  (win->vBar && maxV!=win->vMax) Draw1Control(win->vBar);
	}
			
	/*
	 * reset the port
	 */
	PopGWorld();
	//Dprintf("\pEnd %x;g",win);

	win->updating = false;
}

/************************************************************************
 * EraseAndSeparate - Erase window, and write separators  
 ************************************************************************/
void EraseAndSeparate(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	ThemeBrush brush = (win->isActive && !InBG) ? kThemeActiveModelessDialogBackgroundBrush:kThemeInactiveModelessDialogBackgroundBrush;
	static short kWazooMargin = 28;
	
	GetWindowPortBounds(winWP,&r);
// 	r.top += win->topMargin;

//	if (!win->isDialog)
	{
		// set appropriate background
		if (win->backBrush)
			SetThemeWindowBackground(winWP,brush,False);
		else
			RGBBackColor(&win->backColor);
	}

	/*
	 * erase bottom
	 */
	if (!IsWazoo(winWP)) EraseRectExceptPete(win,&r);	// tab control will handle "erase" otherwise
	
	/*
	 * erase non-grey rect
	 */
	if (win->backBrush)
	{
		RGBBackColor(&win->backColor);
		EraseRect(&win->dontGreyOnMe);
		SetThemeWindowBackground(winWP,brush,False);
	}
}

/**********************************************************************
 * EffectiveBGColor - find the actual bg color for a possibly 3D-affected thing
 **********************************************************************/
RGBColor *EffectiveBGColor(RGBColor *color,short label,Rect *r,short depth)
{
	Boolean blackWhite;
	
	if (label) GetLabelColor(label,color);

	blackWhite = BlackWhite(color);
	
	if (!label || blackWhite) GetBackColor(color);
	
	if ((!label||blackWhite) && D3)
	{
		if (r) depth = RectDepth(r);
		if (depth==4) SetRGBGrey(color,k4Grey1);
		else SetRGBGrey(color,k8Grey1);
	}
	return(color);
}

/**********************************************************************
 * move a window
 **********************************************************************/
void DragMyWindow(WindowPtr winWP,Point thePt)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	Rect bounds;
	Rect oldPos;
	Boolean oldZoomed;
	
	/*
	 * calculate a bounding rectangle
	 */
	GetQDGlobalsScreenBitsBounds(&bounds);
	bounds.top += GetMBarHeight();
	InsetRect(&bounds,W_INSET,W_INSET);

	utl_SaveWindowPos(winWP,&oldPos,&oldZoomed);
	
	/*
	 * do it
	 */
#ifdef	FLOAT_WIN
	MyDragWindow(winWP,thePt,&bounds); 
#else
	DragWindow(winWP,thePt,&bounds);
	if (!(MainEvent.modifiers&cmdKey)) SelectWindow_(winWP);
#endif	//FLOAT_WIN
	
	if (HaveTheDiseaseCalledOSX() && WinRectUnderFloaters(winWP,nil,kWindowDragRgn))
	{
		MoveWindow(winWP,oldPos.left,oldPos.top,false);
		return;
	}
	
	/*
	 * toolbar
	 */
	if (IsFloating(winWP) && win && win->windowType == kDockable) ToolbarNudgeAll(true);
	
	if (win) win->saveSize = True;
	SFWTC = True;
}

/************************************************************************
 * FrontWindowNeedsUpdate - does the frontmost window need updating?
 ************************************************************************/
WindowPtr FrontWindowNeedsUpdate(WindowPtr ignoreThisWin)
{
	WindowPtr theWindow;
	
	for (theWindow=FrontWindow();theWindow;theWindow=GetNextWindow(theWindow))
	{
		if (IsKnownWindowMyWindow(theWindow) && IsWindowVisible(theWindow) && theWindow!=ignoreThisWin)
		{
			if (HasUpdateRgn(theWindow))
				return(theWindow);
			else
				return(nil);
		}
	}
	return(nil);
}

/**********************************************************************
 * grow a window
 **********************************************************************/
void GrowMyWindow(MyWindowPtr win,EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	GrafPtr oldPort;
	Rect bounds;
	Rect oldContR, newContR;
	
	/*
	 * save port
	 */
	GetPort(&oldPort);
	SetPort_(GetMyWindowCGrafPtr(win));

	/*
	 * calculate a bounding rectangle
	 */
	if (event->modifiers & cmdKey)
	{
		//	No minimum size
		bounds.top=bounds.left=0;
	}
	else
	{
		bounds.top=win->minSize.v;
		bounds.left=win->minSize.h;
	}
	bounds.bottom=win->maxSize.v ? win->maxSize.v : REAL_BIG;
	bounds.right=win->maxSize.h ? win->maxSize.h : REAL_BIG;
	
	/*
	 * grow it
	 */
	if (ResizeWindow(winWP, event->where, &bounds, &newContR))
	{
		Rect r;
		
		/*
		 * invalidate grow region
		 */
		GetWindowPortBounds(winWP,&r);
		r.left = r.right - GROW_SIZE;
		r.top = r.bottom - GROW_SIZE;
		InvalWindowRect(winWP,&r);
		
		//	Allow window to adjust to grid
		if (win->grow)
		{
      	Point newSize,adjustSize;
      	
      	newSize.h = RectWi(newContR);
      	newSize.v = RectHi(newContR);
      	adjustSize = newSize;
		   (*win->grow)(win,&adjustSize);
		   if (!EqualPt(newSize,adjustSize))
		      SizeWindow(winWP,adjustSize.h,adjustSize.v,true);
		}
				
		/*
		 * diddle it
		 */
		oldContR = win->contR;
		MyWindowDidResize(win,&oldContR);
	
		/*
		 * update it
		 */
		UpdateMyWindow(winWP);
		
		win->saveSize = True;
	}
	
	/*
	 * restore port
	 */
	SetPort_(oldPort);
	SFWTC = True;
}

/**********************************************************************
 * zoom a window
 **********************************************************************/
void ZoomMyWindow(WindowPtr winWP,Point thePt,int partCode)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	GrafPtr oldPort;
	GDHandle gd;
	Rect oldStd;
	Rect oldUsr;
	Rect curState;
	Rect newStd;
	Rect dummy1, dummy2;
	Boolean hasMB;
	Boolean wasZoomed;
	
	GetWindowStandardState(winWP,&oldStd);
	GetWindowUserState(winWP,&oldUsr);
	Zero(curState);
	curState = CurState(winWP);
	wasZoomed = AboutSameRect(&curState,&oldStd);

	/*
	 * save port
	 */
	GetPort(&oldPort);
	SetPort_(GetWindowPort(winWP));
	
	/*
	 * watch that mouse...
	 */
	if (thePt.h == -1 || TrackBox(winWP,thePt,partCode))
	{
		Rect r,rState;
		
		/*
		 * invalidate grow region
		 */
		GetWindowPortBounds(winWP,&r);
		r.left = r.right - GROW_SIZE;
		r.top = r.bottom - GROW_SIZE;
		InvalWindowRect(winWP,&r);
		
		/*
		 * erase the window (grumble)
		 */
		EraseRect(&r);
		
		/*
		 * figure new zoom rect
		 */
		FigureZoom(winWP);
		GetWindowStandardState(winWP,&newStd);
		
		/*
		 * figure out what WE think the part code should be
		 */
		partCode = (wasZoomed && AboutSameRect(&newStd,&oldStd)) ? inZoomIn : inZoomOut;


		/*
		 * zoom it
		 *
		 */
		ZoomWindow(winWP,partCode,FALSE);
		
		/*
		 * if the window was in the "zoomed" state before, preserve the old
		 * user state; ZoomWindow will have set it to the previous state, which
		 * may have been the old standard state, which is not a very useful thing to do.
		 */
		if (partCode==inZoomOut)
		{
			utl_GetWindGD(winWP,&gd,&dummy1,&dummy2,&hasMB);
			if( WinLocValid( &oldUsr, hasMB ) )
				SetWindowUserState(winWP,&oldUsr);
			else
			{
				GetWindowStandardState(winWP,&rState);
				SetWindowUserState(winWP,&rState);
			}
		}
		
		/*
		 * note size change
		 */
		if (IsMyWindow(winWP) && win) MyWindowDidResize(win,nil);
		
		/*
		 * update it all (grumble, grumble)
		 */
		InvalWindowPort(winWP);

		/*
		 * update it
		 */
		UpdateMyWindow(winWP);
	}
	
	/*
	 * restore
	 */
	SetPort_(oldPort);
	SFWTC = True;
}

/**********************************************************************
 * ReZoomMyWindow - rezoom a window
 **********************************************************************/
void ReZoomMyWindow(WindowPtr theWindow)
{
	Rect oldStd;
	Rect oldUsr;
	Rect curState = CurState(theWindow);
	Rect newStd;
	Point pt;
	
	GetWindowStandardState(theWindow,&oldStd);
	GetWindowUserState(theWindow,&oldUsr);
	if (AboutSameRect(&curState,&oldStd))
	{
		FigureZoom(theWindow);
		GetWindowStandardState(theWindow,&newStd);
		if (!AboutSameRect(&newStd,&oldStd))
		{
			SetWindowStandardState(theWindow,&oldStd);
			pt.h = -1;
			ZoomMyWindow(theWindow,pt,0);
		}
	}
}

/**********************************************************************
 * OffsetWindow - bump a window to its proper place
 **********************************************************************/
void OffsetWindow(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	short offset;
	Point corner;
	Rect oldContR,r;
	
	SetPort_(GetWindowPort(winWP));
	offset = TitleBarHeight(winWP);
	if (IsMyWindow(winWP))
	{
		if (IsWazoo(winWP))
		{
			PositionWazoo(win);
			return;
		}
		if (win->position)
		{
			oldContR = win->contR; 
			if ((*win->position)(False,win))
			{
				MyWindowDidResize(win,nil);
				return;
			}
		}
		if (PrefIsSet(PREF_ZOOM_OPEN) && !win->isRunt)
		{
			Point pt;
			pt.h = pt.v = -1;
			ZoomMyWindow(winWP,pt,0);
			return;
		}
	}
	GetWindowPortBounds(winWP,&r);
	if (win && win->centerAsDefault) {
		utl_CenterDlogRect (&r, true);
		MoveWindow (winWP, r.left, r.top, False);
	}
	else if (win && win->windowType==kStandard) {
		MyStaggerWindow(winWP,&r,&corner);
		MoveWindow(winWP,corner.h,corner.v,False);
	}
	else {
		utl_StaggerWindow(winWP,&r,0,offset,TitleBarHeight(winWP),LeftRimWidth(winWP),&corner,GetPrefLong(PREF_NW_DEV));
		MoveWindow(winWP,corner.h,corner.v,False);
	}
	if (IsMyWindow(winWP)) MyWindowDidResize(win,nil);
	SFWTC = True;
}

/**********************************************************************
 * MyGetMainDevice - get the main device, bearing Eudora's prefs in mind
 **********************************************************************/
GDHandle MyGetMainDevice(void)
{
	GDHandle gd = nil;
	
	short specificDevice = GetPrefLong(PREF_NW_DEV);
	
	//	If ad window, constrain to main screen
	if (specificDevice)
		utl_GetIndGD(specificDevice,&gd,nil,nil);
	else gd = GetMainDevice();

	return gd;
}

/**********************************************************************
 * UpdateAllWindows - update all windows
 **********************************************************************/
void UpdateAllWindows(void)
{
	WindowPtr theWindow;

	for (theWindow=FrontWindow();theWindow;theWindow=GetNextWindow(theWindow))
	{
		if (IsKnownWindowMyWindow(theWindow))
			UpdateMyWindow(theWindow);
		else
			PlugwindowUpdate(theWindow);
	}
}


/**********************************************************************
 * MyWindowDidResize - handle a window whose size has changed
 **********************************************************************/
void MyWindowDidResize(MyWindowPtr win,Rect *oldContR)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	Rect oldCont;
	Boolean	topMarginCtlVisible = false;
	Rect controlR;

	PushGWorld();
	SetPort_(GetWindowPort(winWP));
	
	if (!oldContR) SetRect(&oldCont,0,0,0,0);
	else oldCont = *oldContR;
	
	SetupWinSponsorInfo(win);

	/*
	 * make a couple of copies of the portRect
	 */
	GetWindowPortBounds(winWP,&r);
	r.top += win->topMargin;
	r.bottom -= win->botMargin;
	win->contR = r;
	
	/*
	 * header
	 */
	//	To avoid unnecessary drawing, hide the top margin control for a bit
	if (!IsWazoo(winWP) && win->topMarginCntl && IsWindowVisible(winWP))
	{
		topMarginCtlVisible = IsControlVisible(win->topMarginCntl);
		SetControlVisibility(win->topMarginCntl, false, false);
	}
	SizeTopMargin(win);
	InvalBotMargin(win);
	
	/*
	 * handle scroll bars
	 */
	if (win->hBar)
	{
		SetRect(&controlR,r.left-1,r.bottom-GROW_SIZE,r.right,r.bottom+1);
		controlR.right -= GROW_SIZE-1;
		MySetCntlRect(win->hBar,&controlR);
		win->contR.bottom -= GROW_SIZE;
	}
	if (win->vBar)
	{
		MoveMyCntl(win->vBar,r.right-GROW_SIZE,r.top-1,GROW_SIZE+1,r.bottom-r.top-GROW_SIZE+2);
		win->contR.right -= GROW_SIZE;
	}
									
	/*
	 * grow region
	 */
	r.left = r.right - GROW_SIZE;
	r.top = r.bottom - GROW_SIZE;
	InvalWindowRect(winWP,&r);
	
	/*
	 * window-specific adjustments
	 */
	if (win->didResize)
		(*win->didResize)(win,&oldCont);
	
	if (IsWazoo(winWP))
		DidResizeWazoo(win,&oldCont);
	
	/*
	 * now the scroll bars
	 */
	MyWindowMaxes(win,win->hMax,win->vMax);

	if (!IsWazoo(winWP) && topMarginCtlVisible)
	{
		RgnHandle	tempRgn = NewRgn();	
		if (tempRgn)
		{
			GetWindowUpdateRgn(winWP,tempRgn);	//	Making the top margin control visible messes up the update rgn
			SetControlVisibility(win->topMarginCntl, true, true);
			if (win->labelCntl) SetControlVisibility(win->labelCntl, true, true);
			LocalizeRgn(tempRgn);
			InvalWindowRgn(winWP,tempRgn);
			DisposeRgn(tempRgn);
		}
	}
	PopGWorld();
}

/**********************************************************************
 * 
 **********************************************************************/
void MySetCntlRect(ControlHandle cntl,Rect *r)
{
	MoveMyCntl(cntl,r->left,r->top,RectWi(*r),RectHi(*r));
}

/**********************************************************************
 * MoveMyCntl - move a control attached to a window
 **********************************************************************/
void MoveMyCntl(ControlHandle cntl,int h,int v,int w,int t)
{
	Rect r;
	Boolean hideAndShow;
	Rect oldR;
	
	//	If control is visible but window isn't, hide it while we move it.
	//	Things go a lot faster that way.
	hideAndShow = IsControlVisible(cntl) && !IsWindowVisible(GetControlOwner(cntl));

	if (hideAndShow) SetControlVisibility(cntl,false,false);
	
	GetControlBounds(cntl,&oldR);
	if (w && w!=RectWi(oldR) || t && t!=RectHi(oldR))
	{
		GetControlBounds(cntl,&r);
		if (!w) w = r.right-r.left;
		if (!t) t = r.bottom-r.top;
		SizeControl(cntl,w,t);
	}

	if (oldR.left != h || oldR.top!=v) MoveControl(cntl,h,v);
	
	if (hideAndShow) SetControlVisibility(cntl,true,false);
}

/************************************************************************
 * GetCtlNameWidth - get width of control title
 ************************************************************************/
short GetCtlNameWidth(ControlHandle hCtrl)
{
	Str255	s;		// Used to be Str32, but crashed for long control titles
	
	GetControlTitle(hCtrl,s);
	return StringWidth(s)+2;
}

/************************************************************************
 * SetupButton - set up a button
 ************************************************************************/
ControlHandle NewIconButton(short cntlId,WindowPtr theWindow)
{
	ControlHandle hCtl;	
	ControlButtonTextPlacement placement = kControlBevelButtonPlaceToRightOfGraphic;
	ControlButtonTextAlignment alignment = kControlBevelButtonAlignTextFlushLeft;
	ControlButtonGraphicAlignment gAlignment = kControlBevelButtonAlignLeft;
	
	if (hCtl = GetNewControlSmall_(cntlId,theWindow))
	{
		LetsGetSmall(hCtl);
		SetControlData(hCtl,0,kControlBevelButtonTextAlignTag,sizeof(alignment),(void*)&alignment);
		SetControlData(hCtl,0,kControlBevelButtonTextPlaceTag,sizeof(placement),(void*)&placement);
		SetControlData(hCtl,0,kControlBevelButtonGraphicAlignTag,sizeof(gAlignment),(void*)&gAlignment);
	}
	return hCtl;
}

/************************************************************************
 * NewIconButtonMenu - set up a button that displays a menu
 ************************************************************************/
ControlHandle NewIconButtonMenu(short menuId,short iconId,WindowPtr theWindow)
{
	Str255 title;
	Rect r;
	ControlHandle hCtl;	
	ControlButtonTextPlacement placement = kControlBevelButtonPlaceToRightOfGraphic;
	ControlButtonTextAlignment alignment = kControlBevelButtonAlignTextFlushLeft;
	ControlButtonGraphicAlignment gAlignment = kControlBevelButtonAlignLeft;
	
	Zero(title);
	GetMenuTitle(GetMenu(menuId),title);
	SetRect (&r, 0, 0, 16, 16);
	if ((hCtl = NewControlSmall(theWindow, &r, title, True, menuId, kControlContentIconRef+kControlBehaviorCommandMenu,0,kControlBevelButtonSmallBevelProc+kControlBevelButtonMenuOnRight,0)) != NULL)
	{
		SetBevelIcon(hCtl,iconId,0,0,nil);
		SetControlData(hCtl,0,kControlBevelButtonTextAlignTag,sizeof(alignment),(void*)&alignment);
		SetControlData(hCtl,0,kControlBevelButtonTextPlaceTag,sizeof(placement),(void*)&placement);
		SetControlData(hCtl,0,kControlBevelButtonGraphicAlignTag,sizeof(gAlignment),(void*)&gAlignment);
	}
	return hCtl;
}

/************************************************************************
 * GetMenuFromIconButton - get the menu from an icon button
 ************************************************************************/
MenuHandle GetMenuFromIconButton(ControlHandle cntl)
{
	MenuHandle mh = nil;
	return((GetControlData(cntl,kControlMenuPart,kControlBevelButtonMenuHandleTag,sizeof(mh),(Ptr)&mh,nil) == noErr) ? mh : nil);
}

/************************************************************************
 * PositionBevelButtons - position and size bevel buttons
 ************************************************************************/
void PositionBevelButtons(MyWindowPtr win,short count,ControlHandle *pCtlList,short left,short top,short height,short maxWidth)
{
#define kIconButtonWidth 26
	short	width[64];
	short	i,spacing,sumWidth;
	short	winWidth;
	Boolean	isSponsorAd = win->sponsorAdExists;

	//	Get width of each button (including names)
	sumWidth = 0;
	spacing = INSET;
	for (i=0;i<count;i++)
	{
		width[i] = pCtlList[i] ? GetCtlNameWidth(pCtlList[i]) + kIconButtonWidth + 6 : 0;
		
		// leave room for the triangle on menu buttons
		if (GetMenuFromIconButton(pCtlList[i]))
			width[i]+=6;
			
		sumWidth += width[i];
	}

	if (isSponsorAd)
	{
		//	Don't worry about sponsor ad unless at same vertical
		//	position as the buttons
		if (top+height < win->sponsorAdRect.top || top > win->sponsorAdRect.bottom)
			isSponsorAd = false;	//	No overlap. Don't need to worry about sponsor ad
	}

	winWidth = (isSponsorAd ? win->sponsorAdRect.left : win->contR.right) -  win->contR.left;		
	winWidth = MIN (maxWidth, winWidth);
	if (sumWidth + (count+1)*spacing > winWidth)
	{
		//	Oops, window not wide enough. Display buttons without names
		for (i=0;i<count;i++)
			width[i] = kIconButtonWidth;

		if (count*kIconButtonWidth + (count+1)*spacing > winWidth)
			//	Oops, window still not wide enough. Display buttons without spacing between
			spacing = 0;
	}
	
	// move the buttons
	for (i=0;i<count;i++)
	{
		if (pCtlList[i])
		{
			MoveMyCntl(pCtlList[i],left,top,width[i],height);
			left += width[i] + spacing;
		}
	}
	
	if (isSponsorAd && left-spacing > win->sponsorAdRect.left)
		//	Sponsor ad is in the way. Get rid of it.
		win->sponsorAdExists = false;
}

/************************************************************************
 * ShowControlHelp - display help for a control
 ************************************************************************/
Boolean ShowControlHelp(Point mouse,short strnID,ControlHandle cntl,...)
{
	va_list args;
	short	hnum;
	
	if (!cntl) return false;	
	va_start(args,cntl);
	do
	{
		Rect	r;
		GetControlBounds(cntl,&r);
		if (PtInRect(mouse,&r))
		{
			hnum = strnID;
			if (!ControlIsGrey(cntl)) hnum++;
			MyBalloon(&r,100,0,hnum,0,nil);
			return true;
		}
		strnID += 2;
	}
	while (cntl = va_arg(args,ControlHandle));
	va_end(args);
	return false;
}

/**********************************************************************
 * ScrollMyWindow - scroll a window
 **********************************************************************/
void ScrollMyWindow(MyWindowPtr win,int h,int v)
{
	RgnHandle updateRgn;
	Rect r;
	
	if (!h && !v) return;

	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(win));
	
	if (!win->scroll || (*win->scroll)(win,h,v))
	{
		/*
		 * create a region for ScrollRect to use
		 */
		updateRgn = NewRgn();
		
		ScrollRect(&win->contR,h*win->hPitch,v*win->vPitch,updateRgn);
		
		// add one more pixel to update region
		GetRegionBounds(updateRgn,&r);
		if (h<0)
		{
			r.left--;
			r.right = r.left+1;
		}
		else if (h)
		{
			r.right++;
			r.left = r.right-1;
		}
		if (v<0)
		{
			r.top--;
			r.bottom = r.top+1;
		}
		else if (v)
		{
			r.bottom++;
			r.top = r.bottom-1;
		}
		RgnPlusRect(updateRgn,&r);
		
		/*
		 * update region maintenance
		 */
		InvalWindowRgn(GetMyWindowWindowPtr(win),updateRgn);
		DisposeRgn(updateRgn);
	}
	PopGWorld();
}

/**********************************************************************
 * MyWindowMaxes	- set the max values for the scroll bars
 * takes the maximum number of units in each direction, and subtracts
 * the number of units represented by the window.
 **********************************************************************/
void MyWindowMaxes(MyWindowPtr win,int hMax,int vMax)
{
	int h=0,v=0;
	
	if (win->hBar)
	{
		win->hMax = hMax;
		h = BarMax(win->hBar,hMax,win->contR.right-win->contR.left,win->hPitch);
	}
	if (win->vBar)
	{
		win->vMax = vMax;
		v = BarMax(win->vBar,vMax,win->contR.bottom-win->contR.top,win->vPitch);
	}
	ScrollMyWindow(win,h,v);
}

/**********************************************************************
 * BarMax - set the max value for a scroll bar
 **********************************************************************/
int BarMax(ControlHandle cntl,int max,int winSize,int pitch)
{
		int old;
		
		old = GetControlValue(cntl);
		max -= winSize/pitch;
		if (max < 0) max = 0;
		SetControlMaximum(cntl,max);
#if TARGET_CPU_PPC
		if ((long)SetControlViewSize!=kUnresolvedCFragSymbolAddress)
			SetControlViewSize(cntl,winSize/pitch);
#endif
		return(old>max ? old-max : 0);
}

/**********************************************************************
 * ScrollAction - action procedure for scroll bar
 **********************************************************************/
pascal void ScrollAction(ControlHandle theCtl,short partCode)
{
	WindowPtr	winWP = GetControlOwner(theCtl);
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	short inc=0,page;
	Boolean isH;
	Rect	r;
	static uLong lastTicks;
	short threshId = (partCode==kControlUpButtonPart||partCode==kControlDownButtonPart) ?
		SCROLL_ARROW_THROTTLE : SCROLL_THROTTLE;
	short growSize;
	
	GetControlBounds(theCtl,&r);
	if (win) {
		growSize = win->isRunt || win->hBar&&win->vBar ? 0:GROW_SIZE;
	
		if (!partCode) {lastTicks = 0; return;}	// reset throttle
		if (TickCount()-lastTicks<GetRLong(threshId)) return;
		else lastTicks = TickCount();
	  isH = r.right-r.left > r.bottom-r.top;
		page = (isH ? (r.right-r.left+growSize)/win->hPitch
								 : (r.bottom-r.top+growSize)/win->vPitch)-1;

		switch(partCode)
		{
			case kControlUpButtonPart:
				inc = -1;
				break;
			case kControlDownButtonPart:
				inc = 1;
				break;
			case kControlPageUpPart:
				inc = -page;
				break;
			case kControlPageDownPart:
				inc = page;
				break;
		}
		
		if (inc) inc = -IncMyCntl(theCtl,inc);
		if (inc)
		{
			if (isH)
				ScrollMyWindow(win,inc,0);
			else
				ScrollMyWindow(win,0,inc);
			UpdateMyWindow(winWP);
		}
	}
}

/************************************************************************
 * ScrollIsH - is a scroll bar a horizontal one?
 ************************************************************************/
Boolean ScrollIsH(ControlHandle cntl)
{
	Rect r;

	GetControlBounds(cntl,&r);
	return(r.right-r.left > r.bottom-r.top);
}


/**********************************************************************
 * AMLiveScrollAction - action procedure for live scroll bar using Appearance Mgr
 **********************************************************************/
pascal void AMLiveScrollAction(ControlHandle cntl, SInt16 part)
{
#pragma unused(part)
	SInt32 oldA5 = SetCurrentA5();
	WindowPtr	winWP = GetControlOwner(cntl);
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	short delta, NewCtlValue = GetControlValue(cntl);

	if (delta = gLastCtlValue - NewCtlValue)
	{
		if (ScrollIsH(cntl))
			ScrollMyWindow(win,delta,0);
		else
			ScrollMyWindow(win,0,delta);
		UpdateMyWindow(winWP);
	}

	gLastCtlValue = NewCtlValue;

	SetA5(oldA5);
}


/**********************************************************************
 * ActivateMyWindow - handle an activate event for one of my windows
 **********************************************************************/
void ActivateMyWindow(WindowPtr winWP,Boolean active)
{
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	GrafPtr oldPort;
	ControlHandle cntl;
	PETEHandle pte;
	Boolean nickScanningRefreshed;
	
	if (!IsMyWindow(winWP) || !win) return;
	
	UsingAnyWindows = true;
	
	GetPort(&oldPort);
	SetPort_(GetWindowPort(winWP));
	
	//Dprintf("\pActivate %x %p %p %p",win,*win->qWindow.titleHandle,win->isActive?"\pis":"\pisn't",active?"\pshdbe":"\pshdn'tbe");
	
	win->isActive = active;
	
	if (!win->dontActivate && !GetRootControl(winWP,&cntl))
	{
		RgnHandle	tempRgn = NewRgn();
		
		if (tempRgn)
		{
			//	Save updateRgn as Control Manager may change it
			GetWindowUpdateRgn(winWP,tempRgn);
			if (active) ActivateControl(cntl);
			else DeactivateControl(cntl);
			LocalizeRgn(tempRgn);
			InvalWindowRgn(winWP,tempRgn);
			DisposeRgn(tempRgn);
		}
	}

	if (!win->isDialog)
	{
		if (win->backBrush)
		{
			if (win->dontActivate)
				SetThemeWindowBackground(winWP,kThemeActiveModelessDialogBackgroundBrush,False);
			else
			{
				RgnHandle invalRgn = NewRgn();
				if (invalRgn)
				{
					SetThemeTextColor(win->isActive ? kThemeActiveModelessDialogTextColor : kThemeInactiveModelessDialogTextColor,
						RectDepth(&win->contR),True);
					RectRgn(invalRgn,&win->contR);
					RgnMinusRect(invalRgn,&win->dontGreyOnMe);
					InvalWindowRgn(winWP,invalRgn);
					UpdateMyWindow(winWP);
					DisposeRgn(invalRgn);
				}
			}
		}
		else
			RGBBackColor(&win->backColor);
	}

	if (win->activate)
		(*win->activate)(win);
	
	if (win->pteList) {
		nickScanningRefreshed = false;
		for (pte=win->pteList;pte;pte=PeteNext(pte))
		{
			// (jp) Refresh the nickname globals if at least one field is nick scannable
			if (!nickScanningRefreshed)
				if (HasNickScanCapability (pte)) {
					RefreshNicknameWatcherGlobals (false);
					nickScanningRefreshed = true;
				}
					
			//Dprintf("\pActivate %x %d %d;g",pte,active&&win->pte==pte,active);
			PeteActivate(pte,active && win->pte==pte,active);
			if (win->pte==pte && (*PeteExtra(win->pte))->frame)
				PeteHotRect(win->pte,active);
		}
	}
	
	if (win->isDialog)
	{
		MenuHandle mh = GetMHandle(EDIT_MENU);
		if (mh)
		{
			EnableIf(mh,EDIT_COPY_ITEM,active);
			EnableIf(mh,EDIT_CUT_ITEM,active);
			EnableIf(mh,EDIT_PASTE_ITEM,active);
			EnableIf(mh,EDIT_CLEAR_ITEM,active);
			EnableIf(mh,EDIT_UNDO_ITEM,active);
		}
	}
	
	if (active) SFWTC=True;

#ifdef	FLOAT_WIN
	HiliteWindow(winWP,(active || IsFloating(winWP)));
#endif	//FLOAT_WIN

	SetPort_(oldPort);
}

/**********************************************************************
 * CloseMyWindow - close a window, asking for confirmation if necessary
 **********************************************************************/
Boolean CloseMyWindow(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	Boolean current = GetQDGlobalsThePort() == GetWindowPort(winWP);
	Boolean rmHelp = HasHelp && winWP==FrontWindow_();
	PETEHandle pte, nextPTE;
#ifdef FLOAT_WIN	
	WindowPtr floater = 0;
#endif	//FLOAT_WIN
		
	if (!win)	return (false);
	
	InsertWin = nil;
	
	if (IsPlugwindow(winWP)) return(PlugwindowClose(winWP));
	
	if (!GrowZoning && win->saveSize && win->position) (*win->position)(True,win);
	
#ifdef FLOAT_WIN	
	//This window is no longer interested in keystrokes.  Figure out who should get keys now.	
	if (keyFocusedFloater==win) SetKeyFocusedFloater(nil);
	
#ifdef FLOAT_MULTIPLE_KEYFOCUS
	//Give key presses to the frontmost, non-floating window.
	if ((floater = MyFrontNonFloatingWindow())==winWP)
		floater = GetNextWindow(floater);
		
	if (floater==0 || (GetWindowKind(floater)==TBAR_WIN))
	{
		//There are no non-floaters around to listen to keys.  Find the topmost floater.
		floater = FrontWindow();
		if (floater==winWP) floater = GetNextWindow(floater);
		if (floater && IsFloating(floater)) SetKeyFocusedFloater(GetWindowMyWindowPtr(floater));
	}
#endif
#endif	//FLOAT_WIN

	if (IsWindowVisible(winWP)) AuditWindowClose(win->windex);
	
	if (IsWazoo(winWP))
	{
		if (!CloseWazoo(win))
			return false;
	}
	else
		if (win->close && !(*win->close)(win)) return(False);

#ifdef PEE
	for (pte=win->pteList;pte;pte=nextPTE)
	{
		nextPTE = PeteNext(pte);
		PeteDispose(win,pte);
	}
#endif
	
	win->isDialog ? DisposDialog_(GetDialogFromWindow(winWP)) : DisposeWindow_(winWP);
	if (current) SetPort(InsurancePort);	/* just to be sure... */
	SFWTC = True;
	if (rmHelp) HMRemoveBalloon();
	if (winWP==ModalWindow)
	{
		PopModalWindow();
		EnableMenus(FrontWindow_(),False);
	}

#ifdef	FLOAT_WIN
	if (winWP == 	lastHilitedWinWP) 	lastHilitedWinWP = 0;
#endif	//FLOAT_WIN

	WashMe = true;

	return(True);
}

/**********************************************************************
 * RaisedWindowRect - put a raised rectangle around a window's content
 **********************************************************************/
void RaisedWindowRect(MyWindowPtr win)
{
	RGBColor color;
	
	if (D3 && RectDepth(&win->contR)>=4)
	{
		SetRGBGrey(&color,RectDepth(&win->contR)==4 ? k4Grey1:k8Grey1);
		Color3DRect(&win->contR,&color,e3DSlight,True);
	}
	else EraseRect(&win->contR);
}

/**********************************************************************
 * GoAwayMyWindow - track a click in the go away box
 **********************************************************************/
void GoAwayMyWindow(WindowPtr theWindow,Point pt)
{
	SetPort_(GetWindowPort(theWindow));
	if (TrackGoAway(theWindow,pt))
		CloseMyWindow(theWindow);
}

/**********************************************************************
 * ShowMyControl - show a control, but do it right
 **********************************************************************/
void ShowMyControl(ControlHandle cntrlH)
{
	Rect	r;
	ShowControl(cntrlH);
	GetControlBounds(cntrlH,&r);
	ValidWindowRect(GetControlOwner(cntrlH),&r);
}

/************************************************************************
 * ShowMyWindow - make a window visible
 ************************************************************************/
OSErr ShowMyWindow(WindowPtr theWindow)
{
	InfiniteClip(GetWindowPort(theWindow));
	return(ShowMyWindowBehind(theWindow,nil));
}

/************************************************************************
 * StashStructure - stash the structure rect of a window so that we can
 * still avoid it if it is hidden
 ************************************************************************/
void StashStructure(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);

	GetStructureRgnBounds(winWP,&win->strucR);
	if (RectHi(win->strucR)<RectHi(win->contR)) win->strucR.bottom = win->strucR.top + RectHi(win->contR);
	if (RectWi(win->strucR)<RectWi(win->contR)) win->strucR.right = win->strucR.left + RectWi(win->contR);
}

/************************************************************************
 * ShowMyWindowBehind - make a window visible
 ************************************************************************/
OSErr ShowMyWindowBehind(WindowPtr winWP, WindowPtr behindWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	short kind = GetWindowKind(winWP);
	OSErr err = noErr;
	Rect rBig;
	
	if (win) 
	{
		WashMe = true;
		win->noUpdates = false;
		if (!IsWindowVisible(winWP))
		{
#ifdef NEVER
			if (RunType!=Production)
			{
				Str255 title;
				Dprintf("\pShowMyWindowBehind %x %x;g",win,behindWP);
				if (winWP&&winWP!=InFront&&((long)winWP&1)==0) Dprintf("\p%x �%p�;g",winWP,MyGetWTitle(winWP,title));
				if (behindWP&&behindWP!=InFront&&((long)behindWP&1)==0) Dprintf("\p%x �%p�;g",behindWP,MyGetWTitle(winWP,title));
			}
#endif
			if (win->gonnaShow) err = (*win->gonnaShow)(win);
			if (!err)
			{
				OffsetWindow(winWP);
				AuditWindowOpen(win->windex,GetWindowKind(winWP),IsWazoo(winWP)?1:0);
				if (win->pte) PETEScroll(PETE,win->pte,0,pseSelection);
				if (!behindWP && ModalWindow) behindWP = ModalWindow;
				if (!behindWP)
				{
#ifdef	FLOAT_WIN
					if (!IsFloating(winWP)) SendBehind(winWP, GetMyWindowWindowPtr(FindLastFloater()));	// Send this window behind the last floater	
#endif	//FLOAT_WIN
					win->isActive = !InBG;
					ActivateMyWindow(winWP,!InBG);
					if (IsFloating(winWP))
					{
						ShowHide(winWP,true);
						StashStructure(win);
					}
					else
					{
						ShowWindow(winWP);
						SelectWindow_(winWP);
					}
				}
				else 
				{
					ASSERT(behindWP!=winWP);
					if (behindWP!=winWP&&behindWP!=(WindowPtr)1) SendBehind(winWP,behindWP);
					win->isActive = False;
					ActivateMyWindow(winWP,False);
					ShowWindow(winWP);
				}
				SetRect(&rBig,-REAL_BIG/2,-REAL_BIG/2,REAL_BIG/2,REAL_BIG/2);
				InvalWindowRect(winWP,&rBig);
				SFWTC = True;
			}
		}
		if (err == noErr) WindowIsVisibleClassic(winWP);
	}
	return(err);
}

/************************************************************************
 * ScrollIt - scroll a window, doing all the nasty things that have to
 * be done.
 ************************************************************************/
void ScrollIt(MyWindowPtr win,int deltaH,int deltaV)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	RgnHandle oldClip;
	int hMin,hMax,hVal;
	int vMin,vMax,vVal;
	
	SetPort_(GetWindowPort(winWP));
	if (deltaH && win->hBar)
	{
		hMin = GetControlMinimum (win->hBar);
		hMax = GetControlMaximum (win->hBar);
		hVal = GetControlValue   (win->hBar);
		if (deltaH<0)
		{
			if (deltaH<hVal-hMax) deltaH = hVal-hMax;
		}
		else
		{
			if (deltaH>hVal-hMin) deltaH = hVal-hMin;
		}
	}
	else deltaH = 0;
	if (deltaV && win->vBar)
	{
		vMin = GetControlMinimum (win->vBar);
		vMax = GetControlMaximum (win->vBar);
		vVal = GetControlValue   (win->vBar);
		if (deltaV<0)
		{
			if (deltaV<vVal-vMax) deltaV = vVal-vMax;
		}
		else
		{
			if (deltaV>vVal-vMin) deltaV = vVal-vMin;
		}
	}
	else deltaV = 0;
	
	if (deltaH || deltaV)
	{ 	
		oldClip = SavePortClipRegion(GetWindowPort(winWP));
		InfiniteClip(GetWindowPort(winWP));
		if (deltaH) SetControlValue(win->hBar,hVal-deltaH);
		if (deltaV) SetControlValue(win->vBar,vVal-deltaV);
		ScrollMyWindow(win,deltaH,deltaV);
		UpdateMyWindow(winWP);
		RestorePortClipRegion(GetWindowPort(winWP),oldClip);
	}
}

/************************************************************************
 * EraseUpdateRgn - erase a window's update region
 ************************************************************************/
void EraseUpdateRgn(WindowPtr theWindow)
{
	Point origin;
	RgnHandle rgn = NewRgn();
	
	if (rgn)
	{
		GetWindowUpdateRgn(theWindow,rgn);
		SetPort_(GetWindowPort(theWindow));
		origin.h = origin.v = 0;
		LocalToGlobal(&origin);
		OffsetRgn(rgn,-origin.h,-origin.v);
		EraseRgn(rgn);
		DisposeRgn(rgn);
	}
}

/************************************************************************
 * InvalContent - invalidate a window's content region
 ************************************************************************/
void InvalContent(MyWindowPtr win)
{
	InvalWindowRect(GetMyWindowWindowPtr(win),&win->contR);
}

/************************************************************************
 * InvalTopMargin - invalidate the top margin of a window
 ************************************************************************/
void InvalTopMargin(MyWindowPtr win)
{
	Rect r;
	
	SetRect(&r,-REAL_BIG/2,-REAL_BIG/2,REAL_BIG/2,win->topMargin);
	InvalWindowRect(GetMyWindowWindowPtr(win),&r);
}

/************************************************************************
 * InvalBotMargin - invalidate the bottom margin of a window
 ************************************************************************/
void InvalBotMargin(MyWindowPtr win)
{
	Rect r;
	
	SetRect(&r,-REAL_BIG/2,win->botMargin,REAL_BIG/2,REAL_BIG/2);
	InvalWindowRect(GetMyWindowWindowPtr(win),&r);
}

/************************************************************************
 * CalcCntlInc - do the necessary drudge work before changing a control
 ************************************************************************/
short CalcCntlInc(ControlHandle cntl,short tentativeInc)
{
	short current,max,min,new;
	
	if (!tentativeInc) return(0);
	new = current = GetControlValue(cntl);
	if (tentativeInc<0)
	{
		min = GetControlMinimum(cntl);
		if (current > min) new = MAX(min,current+tentativeInc);
	}
	else
	{
		max = GetControlMaximum(cntl);
		if (current < max) new = MIN(max,current+tentativeInc);
	}
	
	return(new-current);
}

/************************************************************************
 * IncMyCntl - change a control.  Returns the change in value.
 ************************************************************************/
short IncMyCntl(ControlHandle cntl,short inc)
{
	inc = CalcCntlInc(cntl,inc);
	if (inc) SetControlValue(cntl,GetControlValue(cntl)+inc);
	return(inc);
}

/************************************************************************
 * UsingWindow - mark a window as in use
 ************************************************************************/
void UsingWindow(WindowPtr winWP)
{
	MyWindowPtr	win;
	
	if (IsMyWindow(winWP))
		if (win = GetWindowMyWindowPtr (winWP))
			win->inUse = True;
	UsingAnyWindows = true;
}

/************************************************************************
 * NotUsingWindow - mark a window as not in use
 ************************************************************************/
void NotUsingWindow(WindowPtr winWP)
{
	MyWindowPtr	win;
	
	if (IsMyWindow(winWP))
		if (win = GetWindowMyWindowPtr (winWP))
			win->inUse = False;
}

/************************************************************************
 * IsAnyWindow - is a "window" on the window list?
 ************************************************************************/
Boolean IsAnyWindow(WindowPtr maybeWindow)
{
	WindowPtr theWindow = GetWindowList();

	while (theWindow)
	{
		if (theWindow==maybeWindow) return true;
		theWindow = GetNextWindow(theWindow);
	}
	return false;
}

/************************************************************************
 * FigureZoom - figure out the proper zoomed state for a window
 ************************************************************************/
void FigureZoom(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	GDHandle gd;
	Rect zoom,regular,staticZoom;
	Boolean hasMB;
	Point corner;
	Boolean isHideMe = win->hideme;
	short	titleBarHi;

	utl_GetWindGD(winWP,&gd,&zoom,&regular,&hasMB);
	if (gd)	//	If not on screen, don't zoom
	{
		GetDisplayRect(gd,&zoom);
		ComputeWinUseless(winWP,nil,nil,nil,nil);
		titleBarHi = TitleBarHeight(winWP);
		InsetRect(&zoom,3,3);	// leave a little room
		InsetRect(&zoom,UselessWidth(winWP)/2,0);
		zoom.top += titleBarHi;
		zoom.bottom -= (UselessHeight(winWP)-titleBarHi);
		if (IsMyWindow(winWP) && win->zoomSize) (*win->zoomSize)(win,&zoom);
		corner.h = corner.v = 0;
		
		win->hideme = true;	/* make staggerwindow ignore us */
		utl_StaggerWindow(winWP,&zoom,1,TitleBarHeight(winWP),TitleBarHeight(winWP),LeftRimWidth(winWP),&corner,GDIndexOf(gd));
		win->hideme = isHideMe;
		OffsetRect(&zoom,corner.h-zoom.left,corner.v-zoom.top);
		
		
		//if the window's original top-left corner location was alright, then don't move the window
		if( win && FigureStaticZoom( win, hasMB, &zoom, &staticZoom ) )
			SetWindowStandardState(winWP,&staticZoom);

		//otherwise, move it to the top-left corner of the valid screen area
		else
			SetWindowStandardState(winWP,&zoom);
	}
}

/************************************************************************
 * FigureStaticZoom - given the originally figured zoom rect for a window
 * (which will cause the window to jump to the upp-left corner of the
 * screen), figure out the zoom rect without jumping, and return whether or
 * not the original location for the window (based on top left corner) is
 * valid
 ************************************************************************/
Boolean FigureStaticZoom( MyWindowPtr win, Boolean hasMB, const Rect* origZoom, Rect* staticZoom  )
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point origTopLeft;
	Rect validateMe,rCont;
	
	//save the original top-left corner of the window.  Isn't there a nicer place to find this info?
	GetContentRgnBounds(winWP,&rCont);
	origTopLeft.h = rCont.left;
	origTopLeft.v = rCont.top;

	//get the previously figured zoom rect for the window
	*staticZoom = *origZoom;

	//compute the static zoom rect
	staticZoom->right += origTopLeft.h - origZoom->left;
	staticZoom->left = origTopLeft.h;

	staticZoom->bottom += origTopLeft.v - origZoom->top;
	staticZoom->top = origTopLeft.v;
	
  ToolbarNudgeRect(win,staticZoom,false);

	validateMe = *staticZoom;
	if (win->titleBarHi==-1) ComputeWinUseless(winWP,nil,nil,nil,nil);
	validateMe.top -= win->titleBarHi;
	validateMe.bottom += win->uselessHi-win->titleBarHi;
	validateMe.left -= win->leftRimWi;
	validateMe.right += win->uselessWi-win->leftRimWi;

	if( WinLocValid( &validateMe, hasMB ) )
		return( True );
	else
		return( False );
}

/************************************************************************
 * WinLocValid - determines whether the given window rect is in a valid
 * location
 ************************************************************************/
Boolean WinLocValid( Rect* winRect, Boolean hasMB )
{
	GDHandle tmpGD;
	Rect validScreenRect, tb, sect;
	Point corner;
	short i;

	//now loop through each screen until we find the one the window is on
	tmpGD = GetDeviceList();
	while( tmpGD )
	{
		//if the current device is an active screen
		if( TestDeviceAttribute( tmpGD, screenDevice ) && TestDeviceAttribute( tmpGD, screenActive ) )
		{
			//determine the valid area on the screen
			validScreenRect = (**tmpGD).gdRect;

			validScreenRect.left += GetRLong(DESK_LEFT_STRIP);
			validScreenRect.right -= GetRLong(DESK_RIGHT_STRIP);
			validScreenRect.bottom -= GetRLong(DESK_BOTTOM_STRIP);
			validScreenRect.top += GetRLong(DESK_TOP_STRIP);
			if( hasMB ) validScreenRect.top += utl_GetMBarHeight();
			
			corner.h = winRect->left;
			corner.v = winRect->top;
			//if we've found the screen that the top-left corner is on
			if( PtInRect( corner, &validScreenRect ) )
			{
				//then check to make sure the other three corners are also on the screen
				for( i=0; i<3; i++ )
				{
					switch( i )
					{
						case 0:
							corner.h = winRect->right;
							corner.v = winRect->top;
							break;
						case 1:
							corner.h = winRect->right;
							corner.v = winRect->bottom;
							break;
						case 2:
							corner.h = winRect->left;
							corner.v = winRect->bottom;
							break;
					}
					if( !PtInRect( corner, &validScreenRect ) )
						return( False );
				}
				//if the window's location is in the valid screen rectangle
				//and doesn't overlap the toolbar, then return true
				ToolbarRect(&tb);
				return( !SectRect( &tb, winRect, &sect ) );
			}
		}
		tmpGD = GetNextDevice( tmpGD );
	}

	//we never found the window rect in a valid location on any screen. Return false.
	return( False );
}

/************************************************************************
 * GDIndexOf - find the index of a given gd
 ************************************************************************/
short GDIndexOf(GDHandle gd)
{
	short i = 0;
	GDHandle candidate;
	Rect r;
	Boolean b;
	
	if (ThereIsColor)
	{
	  for (utl_GetIndGD(++i,&candidate,&r,&b);candidate;utl_GetIndGD(++i,&candidate,&r,&b))
		  if (candidate==gd) return(i);
	}
	return(0);
}

/************************************************************************
 * ZeroWinfuncs - remove all special handlers from a window
 ************************************************************************/
void ZeroWinFuncs(MyWindowPtr win)
{
	WriteZero(&win->update,(UPtr)&win->zoomSize-(UPtr)&win->update+sizeof(UPtr));
}

#pragma segment Main
/************************************************************************
 * NotUsingAllWindows - mark all windows as not in use
 ************************************************************************/
void NotUsingAllWindows(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	static WindowPtr oldFrontWin;
	WindowPtr newFrontWin = FrontWindow();
	
	// if frontmost window has changed, make sure all is right in the floating world
	if (oldFrontWin != newFrontWin ||
			oldFrontWin && newFrontWin && GetWindowKind(oldFrontWin)!=GetWindowKind(newFrontWin))
	{
		FloatingWinIdle();
		oldFrontWin = FrontWindow();
	}
	
	if (UsingAnyWindows) for (winWP=FrontWindow();winWP;winWP=GetNextWindow(winWP)) 
	{
		win = GetWindowMyWindowPtr (winWP);
		InfiniteClip(GetWindowPort(winWP));	//WHACK!
		
		// windowshade is evil
		if (IsMyWindow(winWP) && !HaveTheDiseaseCalledOSX())
		{
			Rect r;
			GetPortBounds(GetWindowPort(winWP),&r);
			if (r.top>r.bottom)
			{
				GetWindowUserState(winWP,&r);
				SizeWindow(winWP,RectWi(r),RectHi(r),true);
			}
		}

		NotUsingWindow(winWP);
		if (IsKnownWindowMyWindow(winWP))
		{
			if (IsFloating(winWP))
			{
				if (IsWindowVisible(winWP)) StashStructure(win);
				if (IsWindowVisible(winWP)==InBG) ShowHide(winWP,!InBG);
			}
			if ((IsDirtyWindow(win) && win->userSave) != IsWindowModified(winWP))
				SetWindowModified(winWP,win->userSave && IsDirtyWindow(win));
		}
		if (!CorrectlyActivated(winWP) && win)
			ActivateMyWindow(winWP,!win->isActive);
		//	Carbon has a tendency to set our windows to the wronte hilite state
		else if (win && win->isActive != IsWindowHilited(winWP))
			ActivateMyWindow(winWP,win->isActive);
		
		// Properly activate EMSAPI windows
		if (!InBG && (GetWindowKind(winWP)==EMS_PW_WINDOWKIND))
		{		
			// Deactive EMSAPI windows that aren't frontmost
			if (IsWindowHilited(winWP) && (winWP != FrontWindow_()))
			{
				PlugwindowActivate(winWP, false);
				
				// Protect against less friendly EMSAPI windows that
				// don't defer to Eudora's SelectWindow.
				#ifdef	FLOAT_WIN
					if (winWP == lastHilitedWinWP) 	lastHilitedWinWP = 0;
				#endif	//FLOAT_WIN
			}
			
			// Activate EMSAPI windows that are
			if (!IsWindowHilited(winWP) && (winWP == FrontWindow_())) 
				SelectWindow_(winWP);
		}
	}
	UsingAnyWindows = false;
}

/************************************************************************
 * InfiniteClip - set a window's clipping region to REAL_BIG
 ************************************************************************/
void InfiniteClip(CGrafPtr thePort)
{
	if (thePort) {
		Rect r;
		SetPort(thePort);
		SetRect(&r,-REAL_BIG,-REAL_BIG,REAL_BIG,REAL_BIG);
		ClipRect(&r);
	}
}

/************************************************************************
 * DoWindowPropMenu - process item from window properties menu
 ************************************************************************/
void DoWindowPropMenu(MyWindowPtr win,short item,short modifiers)
{
	switch (item)
	{		
		case wpmDockable:
//			ChangeWindowType(win,win->windowType == kDockable ? kStandard : kDockable);
			break;
		case wpmFloating:
//			ChangeWindowType(win,win->windowType == kFloating ? kStandard : kFloating);
			break;
		case wpmTabs:
			if (modifiers & optionKey)
				SetupDefaultWazoos();
			else if (IsWazoo(GetMyWindowWindowPtr(win)))
				DemoteWazoo(win);
			else
				PromoteToWazoo(win);
			break;
	}
}

/************************************************************************
 * SetWindowPropMenu - enable/check items in window properties menu
 ************************************************************************/
void SetWindowPropMenu(MyWindowPtr win,Boolean enable,Boolean all,char markChar,short modifiers)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MenuHandle	mh = GetMHandle(WINDOW_MENU);
	Boolean	doEnable = all||IsWazooable(winWP);
	Boolean option = (modifiers & optionKey) != 0;
	
	if (!mh) return;
	
#ifdef DEBUG
	if (RunType!=Production)
	{
		mh = GetMHandle(WIND_PROP_HIER_MENU);
		if (!mh) return;
		if (enable)
		{
			//	Enable/disable
			EnableIf(mh,wpmTabs,doEnable || option);
			EnableIf(mh,wpmFloating,doEnable);
			EnableIf(mh,wpmDockable,doEnable);
		}
		else
		{
			SetItemR(mh,wpmTabs,option ? REVERT_TO_DEFAULT_TABS : TABS_ITEXT);
			SetItemMark(mh,wpmTabs,IsWazoo(winWP) && !option ? markChar : noMark);
			SetItemMark(mh,wpmFloating,win && win->windowType == kFloating ? markChar : noMark);
			SetItemMark(mh,wpmDockable,win && win->windowType == kDockable ? markChar : noMark);
		}
	}
	else
#endif
	{
		if (enable) EnableIf(mh,WIN_PROPERTIES_ITEM,doEnable || option);
		else
		{
			SetItemR(mh,WIN_PROPERTIES_ITEM,option ? REVERT_TO_DEFAULT_TABS : TABS_ITEXT);
			if (option)
				EnableIf(mh,WIN_PROPERTIES_ITEM,true);
			SetItemMark(mh,WIN_PROPERTIES_ITEM,IsWazoo(winWP) && !option ? markChar : noMark);
		}
	}
}

/**********************************************************************
 * MyFrontWindow - returns top window. Can be floating window if it has focus.
 **********************************************************************/
WindowPtr MyFrontWindow(void)
{
#ifdef	FLOAT_WIN
	return keyFocusedFloater ? GetMyWindowWindowPtr(keyFocusedFloater) : MyFrontNonFloatingWindow();
#else		//FLOAT_WIN
	return FrontWindow_();
#endif	//FLOAT_WIN
}

/**********************************************************************
 * DrawWindowHeaderType - draw proc draws window header
 **********************************************************************/
pascal void DrawWindowHeader(ControlHandle control, SInt16 part)
{
#pragma unused(part)
	Rect	r;
	
	GetControlBounds(control,&r);
	DrawThemeWindowHeader(&r, IsControlActive(control) ? kThemeStateActive : kThemeStateDisabled);
}

#undef ActivateControl
#undef DeactivateControl
/************************************************************************
 * ActivateMyControl - activate or deactivate a control
 ************************************************************************/
void ActivateMyControl(ControlHandle cntl,Boolean active)
{
	if ((IsControlActive(cntl)==0) != (active==0))
		if (active)
			ActivateControl(cntl);
		else
			DeactivateControl(cntl);
}

/************************************************************************
 * MyGetOriginalWTitle - get "original" window title
 *				written to handle naming wazoos of windows that change title 
 *						(e.g. Task Progress Window)
 ************************************************************************/
void MyGetOriginalWTitle(MyWindowPtr win, Str255 title)
{
#ifdef TASK_PROGRESS_ON
	if (win==TaskProgressWindow)
		MyGetItem(GetMHandle(WINDOW_MENU),WIN_TASKS_ITEM,title);
	else
#endif
		GetWTitle(GetMyWindowWindowPtr(win), title);
}

/************************************************************************
 * RememberWindow - save window info for reopening
 ************************************************************************/
Boolean RememberWindow(WindowPtr winWP,DejaVu *dv)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	TOCHandle tocH;
	MessHandle messH;
	TextDHandle textH;
	extern FindClose();
	FSSpec	spec;

	Zero(*dv);
	dv->wType = GetWindowKind(winWP);
	if (win && (UPtr)win->close==(UPtr)FindClose) dv->wType = FIND_WIN;
	switch (dv->wType)
	{
		case dialogKind:
			return false;
		case MBOX_WIN:
		case CBOX_WIN:
			if (GetSearchWinSpec(winWP,&spec))
				dv->wType = SEARCH_WIN;
			else
			{
				tocH = (TOCHandle) GetMyWindowPrivateData(win);
				spec = GetMailboxSpec(tocH,-1);
			}
			PCopy(dv->name,spec.name);
			dv->dirId = spec.parID;
			GetMyVolName(spec.vRefNum,dv->volName);
			break;
		case MESS_WIN:
		case COMP_WIN:
			messH = (MessHandle) GetMyWindowPrivateData(win);
			if (SumOf(messH)->length==0) return false;	/* forget it */
			tocH = (TOCHandle) (*messH)->tocH;
			spec = GetMailboxSpec(tocH,-1);
			PCopy(dv->name,spec.name);
			dv->dirId = spec.parID;
			GetMyVolName(spec.vRefNum,dv->volName);
			dv->index = (*messH)->sumNum;
			dv->id = ValidHash(SumOf(messH)->uidHash) ? SumOf(messH)->uidHash : SumOf(messH)->msgIdHash;
			break;
		case TEXT_WIN:
			textH = (TextDHandle) GetMyWindowPrivateData(win);
			PCopy(dv->name,(*textH)->spec.name);
			if (!*dv->name) return false;
			MyGetWTitle(winWP,dv->alias);
			GetMyVolName((*textH)->spec.vRefNum,dv->volName);
			PCatC(dv->volName,':');
			dv->dirId = (*textH)->spec.parID;
			dv->id = (*textH)->textAttrbutes;
			break;
#ifdef TWO
		case PICT_WIN:
			MyGetWTitle(winWP,dv->name);
			break;
#endif
	}
	return true;
}

/************************************************************************
 * RecallWindow - reopen a window
 ************************************************************************/
void RecallWindow(DejaVu *dv)
{
	TOCHandle tocH;
	short vRef;
	FSSpec spec;
	long sumNum;
	
	switch (dv->wType)
	{
		case MBOX_WIN:
		case CBOX_WIN:
			vRef = GetMyVR(dv->volName);
			if (!FSMakeFSSpec(vRef,dv->dirId,dv->name,&spec) &&
#ifdef	IMAP
					((FindDirLevel(vRef,dv->dirId)>=0) || (IsIMAPMailboxFile(&spec))))
#else
					FindDirLevel(vRef,dv->dirId)>=0)	/* don't open boxes not in tree */
#endif
				(void) GetMailbox(&spec,True);
			break;
		case PREVIEW:
		case MESS_WIN:
		case COMP_WIN:
			vRef = GetMyVR(dv->volName);
			if (!FSMakeFSSpec(vRef,dv->dirId,dv->name,&spec) &&
					(IsIMAPMailboxFile(&spec) || (FindDirLevel(vRef,dv->dirId)>=0)) &&	/* don't open boxes not in tree, unless it's an IMAP mailbox */
				  (tocH = TOCBySpec(&spec)) && dv->index<(*tocH)->count)
			{
				sumNum = dv->index;
				if (ValidHash(dv->id))
					if (TOCFindMessByMID(dv->id,tocH,&sumNum))
						TOCFindMessByMsgID(dv->id,tocH,&sumNum);
				GetAMessage(tocH,sumNum,nil,nil,True);
			}
			break;
		case TEXT_WIN:
			vRef = GetMyVR(dv->volName);
			if (!FSMakeFSSpec(vRef,dv->dirId,dv->name,&spec))
				OpenText(&spec,nil,nil,nil,True,nil,dv->id & kTextAttributeReadOnly ? true : false,dv->id & kTextAttributeIsHTML ? true : false);
			break;
		case ALIAS_WIN:
			OpenABWin(nil);
			break;
		case MB_WIN:
			OpenMBWin();
			break;
		case SIG_WIN:
			OpenSignaturesWin();
			break;
		case PERS_WIN:
			if (HasFeature (featureMultiplePersonalities))
				OpenPersonalitiesWin();
			break;
		case STA_WIN:
			if (HasFeature (featureStationery))
				OpenStationeryWin();
			break;
		case LINK_WIN:
			if (HasFeature (featureLink))
				OpenLinkWin();
			break;
#ifdef ADWARE
		case PAY_WIN:
			OpenPayWin ();
			break;
#endif		
		case FIND_WIN:
			DoFind(FIND_FIND_ITEM,0);
			break;
		case PH_WIN:
			OpenPh(nil);
			break;
#ifdef TASK_PROGRESS_ON
		case TASKS_WIN:
			OpenTasksWin();
			TaskDontAutoClose = true;
			break;
#endif
#ifdef TWO
		case PICT_WIN:
			PICTResWin(dv->name);
			break;
		case FILT_WIN:
			OpenFiltersWindow();
			break;
		case SEARCH_WIN:
			vRef = GetMyVR(dv->volName);
			if (!FSMakeFSSpec(vRef,dv->dirId,dv->name,&spec))
				OpenSearchFile(&spec);
			break;
		case STAT_WIN:
			OpenStatWin();
			break;
#endif
	}
}


long	GetWindowPrivateData (WindowPtr theWindow)

{
	MyWindowPtr	win;
	long				privateData;
	
	privateData = nil;
	if (win = GetWindowMyWindowPtr (theWindow))
		privateData = win->privateData;
	return (privateData);
}

long	GetDialogPrivateData (DialogPtr theDialog)

{
	MyWindowPtr	win;
	long				privateData;

	privateData = nil;
	if (win = GetDialogMyWindowPtr (theDialog))
		privateData = win->privateData;
	return (privateData);
}

void	SetWindowPrivateData (WindowPtr theWindow, long privateData)

{
	MyWindowPtr	win;
	
	if (theWindow)
		if (win = GetWindowMyWindowPtr (theWindow))
			win->privateData = privateData;
}


void	SetDialogPrivateData (DialogPtr theDialog, long privateData)

{
	MyWindowPtr	win;
	
	if (theDialog)
		if (win = GetDialogMyWindowPtr (theDialog))
			win->privateData = privateData;
}

//#define IsMyWindow(wp) ((wp) && IsAnyWindow((MyWindowPtr)wp) && \
//												((GrafPtr)wp)!=InsurancePort && (((MyWindowPtr)wp)->qWindow.windowKind==dialogKind &&\
//												((MyWindowPtr)wp)->qWindow.refCon==CREATOR || \
//												((MyWindowPtr)wp)->qWindow.windowKind>=userKind && ((MyWindowPtr)wp)->qWindow.windowKind!=EMS_PW_WINDOWKIND))
//
//	We have to be very careful here since things like Standard File act stupidly... sigh.
#ifdef DEBUG
void WindowSanityCheck (WindowPtr winWP);
#endif
Boolean	IsMyWindowLo (WindowPtr winWP,Boolean checkWindowList)

{
	short		windowKind;
	Boolean	isIt; // well?
	
	isIt = false;
	// First, do the Window pointery things
	if (winWP && (!checkWindowList || IsAnyWindow (winWP)) && GetWindowPort (winWP) != InsurancePort) {
		windowKind = GetWindowKind (winWP);
		if (windowKind == dialogKind) {
			// Isn't the Standard File package nice??
			long	refcon = GetWRefCon(winWP);
			if (refcon)
				if (GetMyWindowPrivateData (winWP ? (MyWindowPtr) GetWRefCon (winWP) : nil) == CREATOR)
					isIt = true;
		}
		else
			if (windowKind >= OUR_WIN && windowKind < LIMIT_WIN && windowKind != EMS_PW_WINDOWKIND)
				isIt = true;
	}
#if defined(DEBUG) && !defined(__profile__)
if (isIt && RunType!=Production)
WindowSanityCheck (winWP);
#endif
	return (isIt);
}

#ifdef DEBUG
void WindowSanityCheck (WindowPtr winWP)
{
	MyWindowPtr win = (MyWindowPtr) GetWRefCon (winWP);

	ASSERT (win != 0);	
	if (win) {
//		ASSERT (win->theWindow == winWP);
		if (win->theWindow != winWP)
		{
			//	The simple ASSERT was not enough info
			Str255	sTitle;
			GetWTitle(winWP,sTitle);
			Dprintf("\passertion failed: win->theWindow(0x%x) != winWP(0x%x): \"%p\"",win->theWindow,winWP,sTitle);
		}
		ASSERT (win->isRunt == true || win->isRunt == false);
		ASSERT (win->isDialog == true || win->isDialog == false);
		ASSERT (win->isNag == true || win->isNag == false);
		ASSERT (win->isActive == true || win->isActive == false);
		ASSERT (win->isDirty == true || win->isDirty == false);
		ASSERT (win->hasSelection == true || win->hasSelection == false);
		ASSERT (win->butch == true || win->butch == false);
		ASSERT (win->inUse == true || win->inUse == false);
		ASSERT (win->noUpdates == true || win->noUpdates == false);
		ASSERT (win->ignoreDefaultItem == true || win->ignoreDefaultItem == false);
		ASSERT (win->centerAsDefault == true || win->centerAsDefault == false);
		ASSERT (win->hideme == true || win->hideme == false);
		ASSERT (win->dontActivate == true || win->dontActivate == false);
		ASSERT (win->showsSponsorAd == true || win->showsSponsorAd == false);
		ASSERT (win->sponsorAdExists == true || win->sponsorAdExists == false);
		ASSERT (win->classicVisible == true || win->classicVisible == false);
		ASSERT (IsAnyWindow(winWP));
	}
}
#endif

//#ifdef	FLOAT_WIN
//#define CorrectlyActivated(win) (!IsMyWindow(win) || (!InBG && ((win)==FrontWindow_() || IsFloating(win)))==(win)->isActive)
//#else		//FLOAT_WIN
//#define CorrectlyActivated(win) (!IsMyWindow(win) || (!InBG && ((win)==FrontWindow_()))==(win)->isActive)
//#endif	//FLOAT_WIN

Boolean	CorrectlyActivated(WindowPtr winWP)

{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
#ifdef	FLOAT_WIN
	return (!win || !IsMyWindow(winWP) || (!InBG && (winWP==FrontWindow_() || IsFloating(winWP)))==win->isActive);
#else
	return (!win || !IsMyWindow(winWP) || (!InBG && (winWP==FrontWindow_()))==win->isActive)
#endif
}


//	Previously, all of the storage we allocated when creating a dialog was disposed by the Dialog Manager,
//	simply by disposing of thedialog itself.  Since we can no longer use extended dialog records we have
//	to hang our own window structure off of the dialog's refcon field, so we have to manage disposal of
//	this data ourselves
void MyDisposeWindow (WindowPtr winWP)

{
	if (IsMyWindow (winWP)) {
		MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
		ZapPtr (win);
		SetWindowMyWindowPtr (winWP, win);
	}
	DisposeWindow (winWP);
}

/************************************************************************
 * IsWindowVisibleClassic - has a window been displayed to the user?
 ************************************************************************/
Boolean IsWindowVisibleClassic(WindowPtr winWP)
{
	Boolean visible = false;
	MyWindowPtr	win;
	
	if (IsMyWindow(winWP))
		if (win = GetWindowMyWindowPtr (winWP))
			visible = win->classicVisible;
	
	return (visible);
}

/************************************************************************
 * WindowIsInvisibleClassic - this window was just made invisible
 ************************************************************************/
void WindowIsInvisibleClassic(WindowPtr winWP)
{
	MyWindowPtr	win;
	
	if (IsMyWindow(winWP))
		if (win = GetWindowMyWindowPtr (winWP))
			win->classicVisible = false;
}

/************************************************************************
 * WindowIsVisibleClassic - this window has just been made visible
 ************************************************************************/
void WindowIsVisibleClassic(WindowPtr winWP)
{
	MyWindowPtr	win;
	
	if (IsMyWindow(winWP))
		if (win = GetWindowMyWindowPtr (winWP))
			win->classicVisible = true;
}

/************************************************************************
 * WindowEvents - handle carbon window events
 ************************************************************************/
static OSStatus WindowEvents(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
	MyWindowPtr win = (MyWindowPtr)userData;
	static MyWindowPtr resizeWin;
		
	switch (GetEventKind(theEvent))
	{
		case kEventWindowResizeStarted:
			// Indicate we are resizing this window
			resizeWin = win;
			break;
			
		case kEventWindowBoundsChanged:
			// Do live resizing
			if (win == resizeWin)
			{
				// Only handle this event while resizing. Don't handle while
				// moving the window
				MyWindowDidResize(win,nil);
				UpdateMyWindow(GetMyWindowWindowPtr(win));
			}
  			break;

		case kEventWindowResizeCompleted:
			resizeWin = nil;
			break;			
	}
	return CallNextEventHandler (nextHandler, theEvent);
}

/************************************************************************
 * RequestPeteFocus - somebody would like to change the focus in this window
 ************************************************************************/
Boolean RequestPeteFocus(MyWindowPtr win, PETEHandle pte)
{
	// does the window care?
	if (win->requestPeteFocus) return win->requestPeteFocus(win,pte);
	
	// window doesn't care, just do it
	PeteFocus(win,pte,true);
	return true;
}
