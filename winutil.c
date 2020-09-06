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

#include "winutil.h"
#define FILE_NUM 43
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1999 by QUALCOMM Incorporated */

/**********************************************************************
 * various useful functions with at least a tenuous connection to windows
 **********************************************************************/

#define topLeft(r)	(((Point *) &(r))[0])
#define botRight(r)	(((Point *) &(r))[1])

#pragma segment WinUtil
Boolean SafeShow(MyWindowPtr win, Point *pt);
void SafeHide(MyWindowPtr win, Point pt);
int WinLeftCompare(WindowPtr *win1, WindowPtr *win2);
pascal Boolean DlgFilter(DialogPtr dgPtr,EventRecord *event,short *item);

/************************************************************************
 * WinRectUnderFloaters - is a window's rectangle completely under floating windows?
 ************************************************************************/
Boolean WinRectUnderFloaters(WindowPtr winWP,Rect *rect,WindowRegionCode rgnCode)
{
	Rect floatRect;
	RgnHandle rgnLeft = NewRgn();
	WindowPtr floatWP;
	Boolean result = false;
	Rect rectLeft;
	
	if (rgnLeft)
	{
		if (rect)
			RectRgn(rgnLeft,rect);
		else
			GetWindowRegion(winWP,rgnCode,rgnLeft);
		
		InsetRgn(rgnLeft,2,2);
		
		if (EmptyRgn(rgnLeft)) return false; // no drag region?
		
		for (floatWP = FrontWindow(); floatWP && IsFloating(floatWP); floatWP=GetNextWindow(floatWP))
			if (floatWP!=winWP)
			{
				GetWindowBounds(floatWP,kWindowGlobalPortRgn,&floatRect);
				RgnMinusRect(rgnLeft,&floatRect);
				GetRegionBounds(rgnLeft,&rectLeft);
				if ((RectWi(rectLeft)*RectHi(rectLeft)) < 200)
				{
					result = true;
					break;
				}
			}
		
		DisposeRgn(rgnLeft);
	}
	return result;
}


/************************************************************************
 * WinLeftCompare - compare two windows to see which is leftmost or topmost
 ************************************************************************/
int WinLeftCompare(WindowPtr *win1, WindowPtr *win2)
{
	short val;
	Rect rWin1,rWin2;
	
	if (!*win1) return 1;
	if (!*win2) return -1;
	GetStructureRgnBounds(*win1,&rWin1);
	GetStructureRgnBounds(*win2,&rWin2);
	val = rWin1.left - rWin2.left;
	if (!val) val = rWin1.top - rWin2.top;
	return val;
}

/************************************************************************
 * PtrSwap - swap two pointers
 ************************************************************************/
void PtrSwap(Ptr *win1, Ptr *win2)
{
	Ptr temp = *win2;
	*win2 = *win1;
	*win1 = temp;
}

/**********************************************************************
 * GetDisplayRect - Get the maximum size of a window
 **********************************************************************/
typedef struct
{
	Rect r;
	Boolean left;
	Boolean right;
	Boolean top;
	Boolean bottom;
	Boolean vertical;
	Boolean squarish;
	Boolean narrow;
} DockInfo, *DockInfoPtr, **DockInfoHandle;
#define MAX_FLOATS 15 	// We will work with 16 floating windows max
void GetDisplayRect(GDHandle gd, Rect *displayRect)
{
	Rect r;
	MyWindowPtr win;
	Rect baseRect;
	short closeEnoughX, closeEnoughY;
	short verySmallX, verySmallY;
	short bigEnoughX, bigEnoughY;
	short narrowX, narrowY;
	WindowPtr	winWP;
	WindowPtr wins[MAX_FLOATS];
	WindowPtr *nextWin;
	
	if (!gd) gd = GetMainDevice();

	if (GetAvailableWindowPositioningBounds)	//	Available in 10.0 and later
		GetAvailableWindowPositioningBounds(gd,&r);
	else
	{
		r = (*gd)->gdRect;
		
		// menu bar
		if (gd==GetMainDevice()) r.top += GetMBarHeight();
	}
	
	// desktop margins
	r.left += GetRLong(DESK_LEFT_STRIP);
	r.right -= GetRLong(DESK_RIGHT_STRIP);
	r.bottom -= GetRLong(DESK_BOTTOM_STRIP);
	r.top += GetRLong(DESK_TOP_STRIP);
	
	// Ok, remember this.  It's our fallback
	baseRect = r;
	
	// figure out how far from the window margins things can be and
	// still be considered "docked"
	closeEnoughX = (RectWi(r)*GetRLong(CLOSE_ENOUGH_PERCENT))/100;
	closeEnoughY = (RectHi(r)*GetRLong(CLOSE_ENOUGH_PERCENT))/100;

	// figure out how small a thing we will just strip off and forget about
	verySmallX = (RectWi(r)*GetRLong(VERY_SMALL_PERCENT))/100;
	verySmallY = (RectHi(r)*GetRLong(VERY_SMALL_PERCENT))/100;
	
	// figure out what we mean by "narrow"
	narrowX = (RectWi(r)*GetRLong(NARROW_PERCENT))/100;
	narrowY = (RectHi(r)*GetRLong(NARROW_PERCENT))/100;
	
	// How big is big enough?
	bigEnoughX = GetPrefLong(PREF_MWIDTH);
	if (!bigEnoughX) bigEnoughX = GetRLong(DEF_MWIDTH);
	bigEnoughX *= FontWidth;
	bigEnoughX += MAX_APPEAR_RIM * 8;
	bigEnoughY = GetRLong(MIN_WIN_HI) * FontLead;

	// Build list of floating windows, sorted by leftmostness
	nextWin = wins;
	WriteZero(wins,sizeof(wins));
	for (winWP = GetWindowList(); winWP; winWP = GetNextWindow (winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsKnownWindowMyWindow(winWP) && ((win)->windowType==kDockable||(win)->windowType==kFloating))
			if (nextWin<&wins[MAX_FLOATS-1]) *nextWin++ = winWP;
	}
	QuickSort(wins,sizeof(WindowPtr),0,nextWin-wins-1,(void*)WinLeftCompare,(void*)PtrSwap);

	// Do them
	for (nextWin=wins;winWP=*nextWin;nextWin++)
	{
		DockInfo dock;
		
		win = GetWindowMyWindowPtr (winWP);
		dock.r = win->strucR;
		if (SectRect(&r,&dock.r,&dock.r))
		{
			// Figure out
			if (dock.top = dock.r.top-r.top < closeEnoughY) dock.r.top = r.top;
			if (dock.bottom = r.bottom-dock.r.bottom < closeEnoughY) dock.r.bottom = r.bottom;
			if (dock.left = dock.r.left - r.left < closeEnoughX) dock.r.left = r.left;
			if (dock.right = r.right - dock.r.right < closeEnoughX) dock.r.right = r.right;
			
			// If not docked, bite me
			if (!(dock.top || dock.bottom || dock.left || dock.right)) continue;
			
			// Figure out basic geometry
			dock.vertical = RectWi(dock.r) < RectHi(dock.r);
			dock.squarish = 15 < (dock.vertical ? (10*RectHi(dock.r))/RectWi(dock.r) : (10*RectWi(dock.r))/RectWi(dock.r));
			dock.narrow = dock.vertical ? (dock.left || dock.right) && RectWi(dock.r)<narrowX : (dock.top || dock.bottom) && RectHi(dock.r)<narrowY;
			
			// If the overlap is very small, just reduce the rectangle
			if (dock.narrow && dock.vertical)
			{
				if (RectWi(dock.r)<verySmallX)
				{
					if (dock.left) r.left = dock.r.right;
					else r.right = dock.r.left;
					continue;
				}
			}
			else if (dock.narrow && !dock.vertical)
			{
				if (RectHi(dock.r)<verySmallY)
				{
					if (dock.top) r.top = dock.r.bottom;
					else r.bottom = dock.r.top;
					continue;
				}
			}
			
			// So much for the easy stuff
			
			// Special-case for normal-form toolbar
			if (dock.top && dock.left && !dock.squarish && dock.narrow)
			{
				if (dock.vertical) r.left = dock.r.right;
				else r.top = dock.r.bottom;
			}
			// If there is room to the left of us, we'll use it
			else if (dock.r.left - r.left > bigEnoughX)
			{
				r.right = dock.r.left;
			}
			// Well shee-ite.  I'm running out of ideas here.
			// If the window is narrow, we'll just reduce our rectangle
			else if (dock.narrow)
			{
				if (dock.vertical && dock.left) r.left = dock.r.right;
				else if (dock.vertical && dock.right) r.right = dock.r.left;
				else if (dock.top) r.top = dock.r.bottom;
				else if (dock.bottom) r.bottom = dock.r.top;
			}
			// ok, I effing give up
			// from the top, unless the window is vertical and also docked to the side
			else if (dock.top && !((dock.left||dock.right)&&dock.vertical)) r.top = dock.r.bottom;
			// from the bottom, unless the window is vertical and also docked to the side
			else if (dock.bottom && !((dock.left||dock.right)&&dock.vertical)) r.bottom = dock.r.top;
			else if (dock.left) r.left = dock.r.right;
			else if (dock.right) r.right = dock.r.left;
		}
	}
	
	// One final sanity check - if our fancy rect is too small, eff it and go back to the base
	*displayRect = (RectWi(r)<bigEnoughX||RectHi(r)<bigEnoughY) ? baseRect : r;
}

/**********************************************************************
 * force all windows to be redrawn
 **********************************************************************/
void RedrawAllWindows(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	Rect r;
	
	PushGWorld();
	
	/*
	 * invalidate all windows
	 */
	SetRect(&r,-REAL_BIG,-REAL_BIG,REAL_BIG,REAL_BIG);
	for (winWP = FrontWindow_();
		 winWP != nil ;
		 winWP = GetNextWindow(winWP))
	{
		SetPort_(GetWindowPort(winWP));
		InvalWindowRect(winWP,&r);
		if (ThereIsColor && IsKnownWindowMyWindow(winWP) && GetWindowKind(winWP)!=dialogKind)
		{
			win = GetWindowMyWindowPtr(winWP);
			GetRColor(&win->backColor,BACK_COLOR);
			GetRColor(&win->textColor,TEXT_COLOR);
			if (win->backBrush)
				MySetThemeWindowBackground(win,kThemeActiveModelessDialogBackgroundBrush,False);
			else
				RGBBackColor(&win->backColor);

			AppCdefBGChange(win);
		}
	}
	PeteSetupTextColors(nil,false);
	
	PopGWorld();
}

/**********************************************************************
 * MyFindControl - Find a control, possibly disabled
 **********************************************************************/
ControlHandle MyFindControl(Point pt, MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl;
	ControlHandle root;
	ControlHandle bestCntl = nil;
	long area;
	long smallestArea = REAL_BIG;
	
	GetRootControl(winWP,&root);

	for (cntl=GetControlList(winWP);cntl;cntl=GetNextControl(cntl))
		if (cntl!=root && cntl!=win->topMarginCntl && PtInControl(pt,cntl) && 
			IsControlVisible(cntl) && !IsWazooEmbedderCntl(cntl) && !IsWazooTabCntl(cntl))
		{
			area = ControlWi(cntl)*ControlHi(cntl);
			if (area<smallestArea)
			{
				smallestArea = area;
				bestCntl = cntl;
			}
		}
	return(bestCntl);
}

/**********************************************************************
 * PtInControl - is a point (local coords) in a control
 **********************************************************************/
Boolean PtInControlLo(Point pt, ControlHandle cntl,Boolean invisibleOK)
{
	Rect r;
	if (!invisibleOK && !IsControlVisible(cntl)) return false;
	GetControlBounds(cntl,&r);
	return(PtInRect(pt,&r));
}

/**********************************************************************
 * MouseInControl - is the mouse in a control
 **********************************************************************/
Boolean MouseInControl(ControlHandle cntl)
{
	Point pt;
	
	GetMouse(&pt);
	return(PtInControl(pt,cntl));
}

/**********************************************************************
 * MouseInRect - is the mouse in a rect
 **********************************************************************/
Boolean MouseInRect(Rect *r)
{
	Point pt;
	
	GetMouse(&pt);
	return(PtInRect(pt,r));
}

/**********************************************************************
 * GetResName - get a resource's name
 **********************************************************************/
PStr GetResName(PStr into,ResType type,short id)
{
	Handle resH;
	
	*into = 0;
	SetResLoad(False);
	if (resH = GetResource(type,id))
		GetResInfo(resH,&id,&type,into);
	SetResLoad(True);
	return(into);
}

/**********************************************************************
 * SetDIText - set the text of a particular dialog item
 **********************************************************************/
void SetDIText(DialogPtr dialog,int item,UPtr text)
{
	short type;
	Handle itemH;
	Rect box;
	
	if (!GetDialogItemAsControl(dialog,item,(void*)&itemH))	// voas says to do this
		SetDialogItemText(itemH,text);
	else
	{
		GetDialogItem(dialog,item,&type,&itemH,&box);
		if (itemH!=nil)
			SetDialogItemText(itemH,text);
	}
}

/**********************************************************************
 * GetDIText - get the text of a particular dialog item
 **********************************************************************/
void GetDIText(DialogPtr dialog,int item,UPtr text)
{
	short type;
	Handle itemH;
	Rect box;
	
	GetDialogItem(dialog,item,&type,&itemH,&box);
	if (itemH!=nil)
		GetDialogItemText(itemH,text);
}

/**********************************************************************
 * DialogCheckbox - is a hit in dialog checkboxes?
 **********************************************************************/
Boolean DialogCheckbox(MyWindowPtr win, short item)
{
	short type;
	ControlHandle itemH;
	Rect r;
	short value;
	
	GetDialogItem(GetMyWindowDialogPtr(win),item,&type,(Handle*)&itemH,&r);
	if (type==ctrlItem+chkCtrl)
	{
		value = GetControlValue(itemH);
		SetControlValue(itemH,1-value);
		return(True);
	}
	return(False);
}

/**********************************************************************
 * DialogRadioButtons - is a hit in dialog radio buttons?
 **********************************************************************/
Boolean DialogRadioButtons(MyWindowPtr dlogWin, short item)
{
	DialogPtr	dlog;
	short type;
	short otherItem;
	ControlHandle itemH;
	Rect r;
	short n;
	
	dlog = GetMyWindowDialogPtr(dlogWin);
	GetDialogItem(dlog,item,&type,(Handle*)&itemH,&r);
	if (type==ctrlItem+radCtrl)
	{
		if (!GetControlValue(itemH))
		{
			SetControlValue(itemH,1);
			for (otherItem=item-1;otherItem;otherItem--)
			{
				GetDialogItem(dlog,otherItem,&type,(Handle*)&itemH,&r);
				if (type!=ctrlItem+radCtrl) break;
				SetControlValue(itemH,0);
			}
			n=CountDITL(dlog);
			for (otherItem=item+1;otherItem<=n;otherItem++)
			{
				GetDialogItem(dlog,otherItem,&type,(Handle*)&itemH,&r);
				if (type!=ctrlItem+radCtrl) break;
				SetControlValue(itemH,0);
			}
		}
		return(True);
	}
	return(False);
}

/************************************************************************
 * GetDIPopup - get the string a popup control is looking at
 ************************************************************************/
UPtr GetDIPopup(DialogPtr pd,short item,UPtr whatName)
{
	ControlHandle ch;
	short itemType;
	Rect itemRect;

	GetDItem_(pd,item,&itemType,&ch,&itemRect);
	GetMenuItemText(GetControlPopupMenuHandle(ch),GetControlValue(ch),whatName);
	return(whatName);
}
	
/************************************************************************
 * SetDItemState - set the state of an item in a dialog
 ************************************************************************/
short SetDItemState(DialogPtr pd,short dItem,short on)
{
	SetControlValue(GetDItemCtl(pd,dItem),on);
	return(on);
}

/**********************************************************************
 * EnableDItemIf - enable a dialog item or not
 **********************************************************************/
Boolean EnableDItemIf(DialogPtr pd,short dItem,Boolean on)
{
	ControlHandle cntl=GetDItemCtl(pd,dItem);
	
	if (cntl && IsControlActive(cntl)!=on) 
	{
		ActivateMyControl(cntl,on);
		return true;
	}
	return false;
}

/************************************************************************
 * GetDItemCtl - get the controlhandle an item in a dialog
 ************************************************************************/
ControlHandle GetDItemCtl(DialogPtr pd,short dItem)
{
	ControlHandle ch;
	// (jp) 7-19-00  This is a little safer than what we used to do
	//							 We had been relying on the Handle returned from GetDialogItem
	return (!GetDialogItemAsControl (pd, dItem, &ch) ? ch : nil);
}

/************************************************************************
 * GetDItemState - get the state of an item in a dialog
 ************************************************************************/
short GetDItemState(DialogPtr pd,short dItem)
{
	return(GetControlValue(GetDItemCtl(pd,dItem)));
}

/************************************************************************
 * DlgFilter - filter for normal dialogs
 ************************************************************************/
pascal Boolean DlgFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	Boolean oldCmdPeriod=CommandPeriod;
	
#ifdef THREADING_ON	
	if (NEED_YIELD)
		MyYieldToAnyThread();
#endif

#ifdef CTB
	if (CnH) CMIdle(CnH);
#endif
	if (MiniMainLoop(event) || HasCommandPeriod())
	{
		*item = CANCEL_ITEM;
		event->what=nullEvent;
		return(True);
	}
	if (event->what==keyDown || event->what==autoKey) SpecialKeys(event);
	if (event->what==keyDown || event->what==autoKey)
	{
		short key = event->message & charCodeMask;
		if (!(event->modifiers&cmdKey))
			switch (key)
			{
				case enterChar:
				case returnChar:
					*item = GetDialogDefaultItem(dgPtr);
					return(True);
					break;
				case tabChar:
					// Don't attempt any Back Tab magic if the focus is not in an editText
					// item (as is the case when PETE user panes are present)
					{
						Rect itemR;
						short type;
						Handle itemH;
						GetDialogItem (dgPtr, GetDialogKeyboardFocusItem (dgPtr),&type,&itemH,&itemR);
						if ((type==editText) && (event->modifiers&shiftKey))
						{
							BackTab(dgPtr);
							event->what=nullEvent;
						}
					}
					break;
				case escChar:
				case delChar:
					event->what=nullEvent;
					break;
			}
		else
			TEFromScrap();
	}
	else if (event->what==nullEvent)
	{
		if (ToolbarShowing()) TBDisable();
	}
	
	if (!PtInRgn(event->where,MousePen)) DlgCursor(event->where);

	return(MyStdFilterProc(dgPtr,event,item));
}

/**********************************************************************
 * MySetControlTitle - set the title of a control, if different
 **********************************************************************/
void MySetControlTitle(ControlHandle cntl,PStr title)
{
	Str255 oldTitle;
	Boolean hideAndShow;
	
	GetControlTitle(cntl,oldTitle);
	if (StringSame(title,oldTitle)) return;
	
	//	If control is visible but window isn't, hide it while we set it.
	//	Things go a lot faster that way.
	hideAndShow = IsControlVisible(cntl) && !IsWindowVisible(GetControlOwner(cntl));

	if (hideAndShow) SetControlVisibility(cntl,false,false);
	SetControlTitle(cntl,title);
	if (hideAndShow) SetControlVisibility(cntl,true,false);
}

/**********************************************************************
 * MySetCtlValue - set the value of a control, if different
 **********************************************************************/
void MySetCtlValue(ControlHandle cntl,short value)
{
	if (value!=GetControlValue(cntl))
	{
		Boolean hideAndShow;
		
		//	If control is visible but window isn't, hide it while we set it.
		//	Things go a lot faster that way.
		hideAndShow = IsControlVisible(cntl) && !IsWindowVisible(GetControlOwner(cntl));

		if (hideAndShow) SetControlVisibility(cntl,false,false);	
		SetControlValue(cntl,value);
		if (hideAndShow) SetControlVisibility(cntl,true,false);
	}
}

/************************************************************************
 * Update1Control - update a single control
 ************************************************************************/
void Update1Control(ControlHandle cntl,RgnHandle rgn)
{
	Rect r1;
	Rect r2;
	
	GetControlBounds(cntl,&r1);
	GetRegionBounds(rgn,&r2);
	if (SectRect(&r1,&r2,&r2)) Draw1Control(cntl);
}


#pragma segment Main
/**********************************************************************
 * DlgCursor - set the cursor for a dialog
 **********************************************************************/
void DlgCursor(Point mouse)
{
	short itemN;
	short type;
	Handle itemH=nil;
	Rect r;
	DialogPtr dgPtr;
	
	if (dgPtr = GetDialogFromWindow(FrontWindow_())) {
		PushGWorld();
		SetPort(GetDialogPort(dgPtr));
		GetMouse(&mouse);
		
		for (itemN=CountDITL(dgPtr);itemN;itemN--)
		{
			itemH = nil;
			GetDialogItem(dgPtr,itemN,&type,&itemH,&r);
			if (PtInRect(mouse,&r))
			{
				// (jp) PETE user panes do not have an editText type, so we
				//			have to check to see if the control is a PETE user pane.
				if (type&editText || ((type & ctrlItem) && IsPeteControl ((ControlHandle) itemH)))
					SetTopCursor(iBeamCursor);
				else 
					SetTopCursor(arrowCursor);
				break;
			}
		}
		
		if (!itemH)
		{
			SetTopCursor(arrowCursor);
			SetRect(&r,mouse.h,mouse.v,mouse.h+1,mouse.v+1);
		}
		
		PopGWorld();
	}
}



#pragma segment Util

/************************************************************************
 * GetAnswer - make the user answer us
 ************************************************************************/
short GetAnswer(PStr prompt,PStr answer)
{
	MyWindowPtr	dgPtrWin;
	DialogPtr dgPtr;
	short item;
	DECLARE_UPP(DlgFilter,ModalFilter);
	
	INIT_UPP(DlgFilter,ModalFilter);
	if (!MommyMommy(ATTENTION,nil)) return(adlCancel);
	MyParamText(prompt,"","","");
	if ((dgPtrWin = GetNewMyDialog(ANSWER_DLOG,nil,nil,InFront))==nil)
	{
		WarnUser(GENERAL,MemError());
		return(adlCancel);
	}
	
	dgPtr = GetMyWindowDialogPtr(dgPtrWin);
	
	StartMovableModal(dgPtr);
	ShowWindow(GetDialogWindow(dgPtr));
	HiliteButtonOne(dgPtr);
	SetDIText(dgPtr,adlAnswer,"");
	SelectDialogItemText(dgPtr,adlAnswer,0,REAL_BIG);
	PushCursor(arrowCursor);
	MovableModalDialog(dgPtr,DlgFilterUPP,&item);
	PopCursor();
	GetDIText(dgPtr,adlAnswer,answer);
	EndMovableModal(dgPtr);
	DisposDialog_(dgPtr);
	InBG = False; 
	return(item);
}

/**********************************************************************
 * EraseRectExceptPete - erase everything except Pete regions
 **********************************************************************/
void EraseRectExceptPete(MyWindowPtr win,Rect *r)
{
	RgnHandle rgn = NewRgn();
	
	if (!rgn || !win->pteList && EmptyRect(&win->dontGreyOnMe)) EraseRect(r);
	else
	{
		RectRgn(rgn,r);
		PeteRemoveFromRgn(rgn,win->pteList);
		if (win->backBrush) RgnMinusRect(rgn,&win->dontGreyOnMe);
		EraseRgn(rgn);
	}
	if (rgn) DisposeRgn(rgn);
}

/**********************************************************************
 * InvalRectExceptPete - invalidate everything except Pete regions
 **********************************************************************/
void InvalRectExceptPete(MyWindowPtr win,Rect *r)
{
	RgnHandle rgn = NewRgn();
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	
	if (!rgn || !win->pteList) InvalWindowRect(winWP,r);
	else
	{
		RectRgn(rgn,r);
		PeteRemoveFromRgn(rgn,win->pteList);
		InvalWindowRgn(winWP,rgn);
	}
	if (rgn) DisposeRgn(rgn);
}

/************************************************************************
 * GlobalizeRgn - offset a region to global coords
 ************************************************************************/
void GlobalizeRgn(RgnHandle rgn)
{
	Point o;
	
	o.h = o.v = 0;
	LocalToGlobal(&o);
	OffsetRgn(rgn,o.h,o.v);
}

/************************************************************************
 * LocalizeRgn - offset a region to local coords
 ************************************************************************/
void LocalizeRgn(RgnHandle rgn)
{
	Point o;
	
	o.h = o.v = 0;
	GlobalToLocal(&o);
	OffsetRgn(rgn,o.h,o.v);
}

/************************************************************************
 * HiliteButtonOne - hilite the default button in a dialog
 ************************************************************************/
void HiliteButtonOne(DialogPtr dgPtr)
{
	MyWindowPtr	dgPtrWin = GetDialogMyWindowPtr (dgPtr);
	short type;
	ControlHandle itemH;
	Rect r;
	short item = GetDialogDefaultItem(dgPtr) ? GetDialogDefaultItem(dgPtr) : 1;

	if (!dgPtrWin->ignoreDefaultItem) {
		GetDItem_(dgPtr,item,&type,&itemH,&r);
		if (type==btnCtrl+ctrlItem)
			SetDialogDefaultItem(dgPtr,item);
	}
}

/************************************************************************
 * DrawRJust - draw a string, right justified
 ************************************************************************/
void DrawRJust(PStr s,short h, short v)
{
	MoveTo(h-StringWidth(s),v);
	DrawString(s);
}

/************************************************************************
 * Control2Menu - get the menu of a popup control
 ************************************************************************/
MenuHandle Control2Menu(ControlHandle cntl)
{
	return GetControlPopupMenuHandle(cntl);
}

/************************************************************************
 * PlotFullSICNRes - plot a sicn, making sure it's 16x16
 ************************************************************************/
void PlotFullSICNRes(Rect *theRect, short resId, long theIndex)
{
	Rect r = *theRect;
	
	r.bottom = r.top+16;
	r.right = r.left+16;
	PlotSICNRes(&r,resId,theIndex);
}

/************************************************************************
 * PlotSICNRes - plot a sicn from a resource
 ************************************************************************/
void PlotSICNRes(Rect *theRect, short resId, long theIndex)
{
	SICNHand resH = GetResource_('SICN',resId);
	if (resH) PlotSICN(theRect,resH,theIndex);
}

/************************************************************************
 * PlotSICN, courtesy of Apple DTS
 * changed not to scale sicn
 ************************************************************************/
void PlotSICN(Rect *theRect, SICNHand theSICN, long theIndex) {
			 auto char	 state; 	/*saves original flags of 'SICN' handle*/
			 auto BitMap srcBits; /*built up around 'SICN' data so we can
_CopyBits*/
			 Rect localRect = *theRect;
			 
			 SetRect(&localRect,0,0,16,16);
			 CenterRectIn(&localRect,theRect);

			 /* check the index for a valid value */
			 if ((GetHandleSize_(theSICN) / sizeof(SICN)) > theIndex) {

					 /* store the resource's current locked/unlocked condition */
					 state = HGetState((Handle)theSICN);

					 /* lock the resource so it won't move during the _CopyBits call
*/
					 HLock((Handle)theSICN);

					 /* set up the small icon's bitmap */
					 srcBits.baseAddr = (Ptr) (*theSICN)[theIndex];
					 srcBits.rowBytes = 2;
					 SetRect(&srcBits.bounds, 0, 0, 16, 16);

					 /* draw the small icon in the current grafport */
					 CopyBits(&srcBits,GetPortBitMapForCopyBits(GetQDGlobalsThePort()),&srcBits.bounds,
										&localRect,srcOr,nil);

					 /* restore the resource's locked/unlocked condition */
					 HSetState((Handle)theSICN, state);
			 }
}

/************************************************************************
 *
 ************************************************************************/
void SavePosPrefs(UPtr name,Rect *r, Boolean zoomed)
{
	PositionHandle rez;
	
	if (!*name) return;	// can't retrieve without a name, so don't save
	rez=(void*)Get1NamedResource(SAVE_POS_TYPE,name);

	// Handle invalid resources here
	if (rez && !*rez || GetHandleSize(rez)!=sizeof(PositionType))
	{
		RemoveResource(rez);
		ZapHandle(rez);
	}

	if (!rez)	
	{
		rez = NewH(PositionType);
		if (!rez) return;
		AddResource_(rez,SAVE_POS_TYPE,Unique1ID(SAVE_POS_TYPE),name);
		if (ResError()) {ZapHandle(rez); return;}
	}
	(*rez)->r = *r;
	(*rez)->zoomed = zoomed;
	ChangedResource((Handle)rez);
}

/**********************************************************************
 * FindControlByRefCon - find a control by its refcon
 **********************************************************************/
ControlHandle FindControlByRefCon(MyWindowPtr win,long refCon)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl;
	
	for (cntl=GetControlList(winWP);cntl;cntl = GetNextControl(cntl))
		if (GetControlReference(cntl) == refCon) break;
	
	return(cntl);
}

/************************************************************************
 *
 ************************************************************************/
void SavePosFork(short vRef,long dirId,UPtr name,Rect *r, Boolean zoomed)
{
	short refN;
	Str31 scratch;
	CInfoPBRec info;
	long oldmdDat;
	short err;
	short oldResF = CurResFile();
	
	if (AHGetFileInfo(vRef,dirId,name,&info)) return;
	oldmdDat = info.hFileInfo.ioFlMdDat;
	
	refN=HOpenResFile(vRef,dirId,name,fsRdWrPerm);
	if (refN<0)
	{
		HCreateResFile(vRef,dirId,name);
		if (err=ResError()) return;
		refN=HOpenResFile(vRef,dirId,name,fsRdWrPerm);
		err=ResError();
	}
	if (refN>=0)
	{
		SavePosPrefs(GetRString(scratch,POSITION_NAME),r,zoomed);
		if (refN != SettingsRefN)
		{
			CloseResFile(refN);
			if (!AHGetFileInfo(vRef,dirId,name,&info))
			{
				info.hFileInfo.ioFlMdDat = oldmdDat;
				AHSetFileInfo(vRef,dirId,name,&info);
			}
		}
	}
	UseResFile (oldResF);
}

/************************************************************************
 *
 ************************************************************************/
Boolean RestorePosPrefs(UPtr name,Rect *r, Boolean *zoomed)
{
	PositionHandle rez;
	
	if (rez=(void*)Get1NamedResource(SAVE_POS_TYPE,name))
	{
		*r = (*rez)->r;
		*zoomed = (*rez)->zoomed;
		return(True);
	}
	return(False);
}

/************************************************************************
 *
 ************************************************************************/
Boolean RestorePosFork(short vRef,long dirId,UPtr name,Rect *r, Boolean *zoomed)
{
	Str31 scratch;
	short refN;
	Boolean done;
	short oldResF = CurResFile();
	
	if ((refN=HOpenResFile(vRef,dirId,name,fsRdPerm))>=0)
	{
		done = RestorePosPrefs(GetRString(scratch,POSITION_NAME),r,zoomed);
		if (refN != SettingsRefN) CloseResFile(refN);
		UseResFile (oldResF);
		return(done);
	}
	UseResFile (oldResF);
	return(False);
}

/************************************************************************
 * Save/restore window position by name
 ************************************************************************/
Boolean PositionPrefsByName(Boolean save,MyWindowPtr win,StringPtr title)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	Boolean zoomed;
	
	if (save)
	{
		utl_SaveWindowPos(winWP,&r,&zoomed);
		SavePosPrefs(title,&r,zoomed);
	}
	else
	{
		if (!RestorePosPrefs(title,&r,&zoomed))
			{return(False);}
		if (win->isRunt)
		{
			r.right = r.left + WindowWi(winWP);
			r.bottom = r.top + WindowHi(winWP);
		}
		
		//	Make sure saved window size isn't smaller than window's minSize
		if (RectWi(r)<win->minSize.h) r.right = r.left + win->minSize.h;
		if (RectHi(r)<win->minSize.v) r.bottom = r.top + win->minSize.v;
		
		utl_RestoreWindowPos(winWP,&r,zoomed,1,TitleBarHeight(winWP),LeftRimWidth(winWP),(void*)FigureZoom,(void*)DefPosition);

		PositionDockedWindow(winWP);	//	Make sure positioned correctly if docked
	}
	return(True);
}

/************************************************************************
 *
 ************************************************************************/
Boolean PositionPrefsTitle(Boolean save,MyWindowPtr win)
{
	Str255 title;
	
	MyGetWTitle(GetMyWindowWindowPtr(win),title);
	return PositionPrefsByName(save,win,title);
}

/**********************************************************************
 * SetDICTitle - Set the title of a control in a dialog
 **********************************************************************/
void SetDICTitle(DialogPtr dlog,short item,PStr title)
{
	Str255 now;
	ControlHandle cntl=nil;
	short type;
	Rect r;
	
	GetDICTitle(dlog,item,now);
	if (!EqualString(title,now,True,True))
	{
		GetDialogItem(dlog,item,&type,(void*)&cntl,&r);
		if (type&ctrlItem) SetControlTitle(cntl,title);
	}
}

/**********************************************************************
 * GetDICTitle - get the title of a control in a dialog
 **********************************************************************/
PStr GetDICTitle(DialogPtr dlog,short item,PStr title)
{
	ControlHandle cntl=nil;
	short type;
	Rect r;

	GetDialogItem(dlog,item,&type,(void*)&cntl,&r);
	if (type&ctrlItem) GetControlTitle(cntl,title);
	else *title = 0;
	
	return(title);
}

/**********************************************************************
 * HiliteDIControl - hilite a control that's an item in a dialog
 **********************************************************************/
void HiliteDIControl(DialogPtr dlog,short item,short hilite)
{
	ControlHandle cntl=nil;
	short type;
	Rect r;

	GetDialogItem(dlog,item,&type,(void*)&cntl,&r);
	if (type&ctrlItem) HiliteControl(cntl,hilite);
}

/************************************************************************
 * WindowWi - how wide is a window
 ************************************************************************/
short WindowWi(WindowPtr theWindow)
{
	CGrafPtr gp = GetWindowPort (theWindow);
	Rect	rPort;
	GetPortBounds(gp,&rPort);
	return(rPort.right-rPort.left);
}

/************************************************************************
 * WindowHi - how wide is a window
 ************************************************************************/
short WindowHi(WindowPtr theWindow)
{
	CGrafPtr gp = GetWindowPort (theWindow);
	Rect	rPort;
	GetPortBounds(gp,&rPort);
	return(rPort.bottom-rPort.top);
}

/************************************************************************
 * PopUpMenuH - get the handle of a pop-up menu control
 ************************************************************************/
MenuHandle PopUpMenuH(ControlHandle ctl)
{
	return GetControlPopupMenuHandle(ctl);
}

/************************************************************************
 * DefPosition - find the default position for a window
 ************************************************************************/
void DefPosition(WindowPtr theWindow,Rect *r)
{
	Point corner;

	GetPortBounds(GetWindowPort(theWindow),r);
	MyStaggerWindow(theWindow,r,&corner);
	OffsetRect(r,corner.h-r->left,corner.v-r->top);
}

/************************************************************************
 * GreyOutRoundRect - grey a rectangle
 ************************************************************************/
void GreyOutRoundRect(Rect *r,short r1, short r2)
{
	PenState oldState;
	Pattern	gray;
	
	GetPenState(&oldState);
	PenMode(patBic);
	PenPat(GetQDGlobalsGray(&gray));
	PaintRoundRect(r,r1,r2);
	SetPenState(&oldState);
}

/**********************************************************************
 * ConfigFontSetup - set up a window with the config font
 **********************************************************************/
void ConfigFontSetup(MyWindowPtr win)
{
	DialogPtr	dlog=nil;
	WindowPtr	wp;
	GrafPtr		port;
	GrafPtr 	oldPort;
	
	if (win->isDialog)
	{
		dlog = GetMyWindowDialogPtr (win);
		wp = GetDialogWindow (dlog);
	}
	else
	{
		dlog = nil;
		wp = GetMyWindowWindowPtr(win);
	}
	port = GetWindowPort(wp);
	
	GetPort(&oldPort);
	SetPort(port);
	SetSmallSysFont();
	win->vPitch = GetLeading(GetPortTextFont(port),GetPortTextSize(port));
	win->hPitch = GetWidth(GetPortTextFont(port),GetPortTextSize(port));
	if (win->isDialog && dlog && GetDialogTextEditHandle(dlog))
		ChangeTEFont(GetDialogTextEditHandle(dlog),GetPortTextFont(port),GetPortTextSize(port));
	SetPort(oldPort);
}

/************************************************************************
 * SetSmallSysFont - set font & size of current port to small system font
 ************************************************************************/
void SetSmallSysFont(void)
{
	TextFont(SmallSysFontID());
	TextSize(SmallSysFontSize());
}

/************************************************************************
 * SmallSysFontID - return id of small sys font
 ************************************************************************/
short SmallSysFontID(void)
{
	Str63 font;
	
	if (*GetRString(font,SMALL_SYS_FONT_NAME))
	{
		if (EqualStrRes(font,APPL_FONT)) return 1;
		else return GetFontID(font);
	}	
	return(ScriptVar(smScriptSmallSysFondSize)>>16);
}

/************************************************************************
 * SmallSysFontSize - return size of small sys font
 ************************************************************************/
short SmallSysFontSize(void)
{
	short size = GetRLong(SMALL_SYS_FONT_SIZE);
	
	if (size) return size;
	return(ScriptVar(smScriptSmallSysFondSize)&0xffff);
}
	
/************************************************************************
 * SanitizeSize - make sure a rect is small enough to fit onscreen
 ************************************************************************/
void SanitizeSize(MyWindowPtr win,Rect *r)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect gray;
	short maxWi, maxHi;
	GDHandle gd;
	Rect localR,rWin;
	Boolean hasMB;
	
	localR = *r;
	if (GetAvailableWindowPositioningBounds)	//	Available in 10.0 and later
	{
		utl_GetWindGD (GetMyWindowWindowPtr(win), &gd, &localR, &rWin, &hasMB);
		hasMB = false;
		GetAvailableWindowPositioningBounds(gd,&gray);
	}
	else
	{
		GetRegionBounds(GetGrayRgn(),&gray);
		utl_GetRectGDStuff(&gd,&gray,&localR,TitleBarHeight(winWP),LeftRimWidth(winWP),&hasMB);
	}
	DockedWinReduce(winWP,nil,&gray);
	maxWi = RectWi(gray) - 2;
	maxHi = RectHi(gray) - 8;
	if (hasMB) maxHi -= GetMBarHeight();
	if (winWP) maxHi -= UselessHeight(winWP);
	
	if (RectHi(*r)<win->minSize.v)
		r->bottom = r->top+win->minSize.v;

	if (RectWi(*r)<win->minSize.h)
		r->right = r->left+win->minSize.h;

	if (RectWi(*r)>maxWi) r->right = r->left+maxWi;
	if (RectHi(*r)>maxHi) r->bottom = r->top+maxHi;
}

/************************************************************************
 * LeftRimWidth - figure out how wide the left rim of a window is
 ************************************************************************/
short LeftRimWidth(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	short wi;
	
	if (!IsMyWindow(winWP) || !win || win->leftRimWi==-1)
		ComputeWinUseless(winWP,nil,&wi,nil,nil);
	else
		wi = win->leftRimWi;
	return(wi);
}

/************************************************************************
 * UselessWidth - figure out how much of a window's width is useless
 ************************************************************************/
short UselessWidth(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	short wi;
	
	if (!IsMyWindow(winWP) || !win || win->uselessWi==-1)
		ComputeWinUseless(winWP,nil,nil,nil,&wi);
	else
		wi = win->uselessWi;
	return(wi);
}

/************************************************************************
 * ComputeWinUseless - how wide are the useless parts of a window?
 ************************************************************************/
void ComputeWinUseless(WindowPtr winWP,short *titleHi,short *leftRimWi,short *uselessHi,short *uselessWi)
{
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	short title, left, hi, wi;
	Rect	rWinStruc,rWinCont,rTitle;
	
	// compute the values
	GetWindowBounds(winWP,kWindowStructureRgn,&rWinStruc);
	GetWindowBounds(winWP,kWindowContentRgn,&rWinCont);
	hi = RectHi(rWinStruc) - RectHi(rWinCont);
	wi = RectWi(rWinStruc) - RectWi(rWinCont);
	left = rWinCont.left - rWinStruc.left;
	GetWindowBounds(winWP,kWindowTitleBarRgn,&rTitle);
	title = RectHi(rTitle);
	
	// stash the values
	if (IsMyWindow(winWP))
	{
		if (win->drawerUseless)
		{
			short dLeft, dRight;
			win->drawerUseless(win,&dLeft,&dRight);
			wi += dLeft + dRight;
			left += dLeft;
		}
		win->titleBarHi = title;
		win->leftRimWi = left;
		win->uselessHi = hi;
		win->uselessWi = wi;
	}
	
	// return the values
	if (titleHi) *titleHi = title;
	if (uselessHi) *uselessHi = hi;
	if (leftRimWi) *leftRimWi = left;
	if (uselessWi) *uselessWi = wi;
}


/************************************************************************
 * TitleBarHeight - how high is the title bar?
 ************************************************************************/
short TitleBarHeight(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	short hi;
	
	if (!IsMyWindow(winWP) || !win || win->titleBarHi==-1)
		ComputeWinUseless(winWP,&hi,nil,nil,nil);
	else
		hi = win->titleBarHi;
	return(hi);
}

/************************************************************************
 * UselessHeight - how high is the useless part of a window?
 ************************************************************************/
short UselessHeight(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	short hi;
	
	if (!IsMyWindow(winWP) || !win || win->uselessHi==-1)
		ComputeWinUseless(winWP,nil,nil,&hi,nil);
	else
		hi = win->uselessHi;
	return(hi);
}

/************************************************************************
 * SafeShow - show a window, making sure it's offscreen first
 ************************************************************************/
Boolean SafeShow(MyWindowPtr win, Point *pt)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);

	if (IsWindowVisible(winWP)) return(False);
	SetPort_(GetWindowPort(winWP));
	Zero(*pt);
	LocalToGlobal(pt);
	MoveWindow(winWP,5000,5000,False);
	ShowHide(winWP,True);
	WindowIsVisibleClassic(winWP);
	return(True);
}  

/************************************************************************
 * SafeHide - hide a window that was shown offscreen
 ************************************************************************/
void SafeHide(MyWindowPtr win, Point pt)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);

	ShowHide(winWP,False);
	MoveWindow(winWP,pt.h,pt.v,False);
	WindowIsInvisibleClassic(winWP);
}  

/**********************************************************************
 * DragDivider - drag a dividing line, given some bounds
 **********************************************************************/
Point DragDivider(Point pt, Rect *divider, Rect *bounds, MyWindowPtr win)
{
	RgnHandle grey = NewRgn();
	Rect r;
	long dvdh;
	Point ret;
	Rect slop;
	Boolean vert = RectWi(*divider)>RectHi(*divider);
	
	Zero(ret);	

	if (grey)
	{
		RectRgn(grey,divider);
		
		r = *bounds;
		
		if (r.top == r.bottom) r.top = r.bottom = pt.v;
		if (r.left == r.right) r.left = r.right = pt.h;
		
		slop = r;
		InsetRect(&slop,-8,-8);
		ret = pt;
		
		dvdh = DragGrayRgn(grey,pt,&r,&slop,vert?vAxisOnly:hAxisOnly,nil);
		if (vert) ret.v = pt.v + (short)((dvdh>>16)&0xffff);
		else ret.h = pt.h + (short)(dvdh&0xffff);
		
		if (PtInRect(ret,&slop))
		{
			ret.h = MAX(bounds->left,ret.h);
			ret.h = MIN(bounds->right,ret.h);
			ret.v = MAX(bounds->top,ret.v);
			ret.v = MIN(bounds->bottom,ret.v);
		}

		/*
		 * I don't understand why I have to do this; I guess I don't understand
		 * DragGrayRgn.
		 */
		if (!PtInRect(ret,bounds)) Zero(ret);
		
		DisposeRgn(grey);
	}
	return(ret);
}

/**********************************************************************
 * PtInSloppyRect - see if a point is in a rectangle, but with slop
 **********************************************************************/
Boolean PtInSloppyRect(Point pt, Rect *r, short slop)
{
	Rect slopR = *r;
	
	InsetRect(&slopR,-slop, -slop);
	return(PtInRect(pt,&slopR));
}

/************************************************************************
 * MyWinHasSelection - is there a selection in one of the te's?
 ************************************************************************/
Boolean MyWinHasSelection(MyWindowPtr win)
{
	long start,stop;
	UHandle text;
	
	if (win->selection)
		return((*win->selection)(win));
	else if (win->pte && !PeteGetTextAndSelection(win->pte,&text,&start,&stop))
		return(stop!=start);
	else
		return(False);
}

/**********************************************************************
 * BoldString - draw a bold string
 **********************************************************************/
void BoldString(PStr string)
{
	TextFace(bold);
	DrawString(string);
	TextFace(0);
}

/**********************************************************************
 * 
 **********************************************************************/
void BoldRString(short id)
{
	Str255 s;
	BoldString(GetRString(s,id));
}

#define SetLineWidth 182
/**********************************************************************
 * 
 **********************************************************************/
void Hairline(Boolean on)
{
	Point **h = (void*)NewHandle(sizeof(Point));
	if (!h) return;
	if (on)
	{
		(*h)->v = 1;
		(*h)->h = 5;
		PicComment(SetLineWidth,sizeof(Point),(void*)h);
	}
	else
	{
		(*h)->v = 5;
		(*h)->h = 1;
		PicComment(SetLineWidth,sizeof(Point),(void*)h);
		(*h)->v = 1;
		(*h)->h = 1;
		PicComment(SetLineWidth,sizeof(Point),(void*)h);
	}
	ZapHandle(h);
}

/************************************************************************
 * HotRect - SFPutFile-style frame
 ************************************************************************/
void HotRect(Rect *r,Boolean on)
{
	DrawThemeFocusRect(r,on);
}

/************************************************************************
 * MySetThemeWindowBackground - set the window bg and remember
 ************************************************************************/
OSStatus MySetThemeWindowBackground(MyWindowPtr win, ThemeBrush brush, Boolean update)
{
	win->backBrush = brush;
	return SetThemeWindowBackground(GetMyWindowWindowPtr(win),brush,update);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean PtInSlopRect(Point pt,Rect r,short slop)
{
	InsetRect(&r,-slop,-slop);
	return(PtInRect(pt,&r));
}

/************************************************************************
 * RgnMumbleRect - add or subtract a rectangle from a rgn
 ************************************************************************/
void RgnMumbleRect(RgnHandle rgn, Rect *r,Boolean add)
{
	RgnHandle punchRgn;
		
	if (punchRgn=NewRgn())
	{
		RectRgn(punchRgn,r);
		if (add) UnionRgn(rgn,punchRgn,rgn);
		else DiffRgn(rgn,punchRgn,rgn);
		DisposeRgn(punchRgn);
	}
}

/**********************************************************************
 * DrawRString - draw a string direct from a resource
 **********************************************************************/
void DrawRString(short id)
{
	Str255 s;
	DrawString(GetRString(s,id));
}

/**********************************************************************
 * RStringWidth - measure a string direct from a resource
 **********************************************************************/
short RStringWidth(short id)
{
	Str255 s;
	return(StringWidth(GetRString(s,id)));
}

/************************************************************************
 * MaxSizeZoom - size the zoom rect of a window, using the maxes provided
 ************************************************************************/
void MaxSizeZoom(MyWindowPtr win,Rect *zoom)
{
	long zoomHi = zoom->bottom-zoom->top;
	long zoomWi = zoom->right-zoom->left;
	long hi, wi;
	long fixedHi = win->topMargin+win->botMargin+(win->hBar ? GROW_SIZE : 0);
	
	if (win->hMax<0) UpdateMyWindow(GetMyWindowWindowPtr(win));
	hi = win->vMax ?
		win->vMax*win->vPitch+fixedHi :
		(win->vBar ? win->minSize.v : zoomHi);
	wi = win->hMax ?
		win->hMax*win->hPitch+(win->vBar ? GROW_SIZE : 0) :
		(win->hBar ? win->minSize.h : zoomWi);

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	hi = ((hi-fixedHi)/win->vPitch)*win->vPitch+fixedHi;
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;
}

/************************************************************************
 * CurState - return a windows current state
 ************************************************************************/
Rect CurState(WindowPtr theWindow)
{
	Point pt;
	Rect curState;
	GrafPtr oldPort;
	
	pt.h = pt.v = 0;
	GetPort(&oldPort);
	SetPort_(GetWindowPort(theWindow));
	LocalToGlobal(&pt);
	SetPort(oldPort);
	GetPortBounds(GetWindowPort(theWindow),&curState);
	OffsetRect(&curState, pt.h, pt.v);
	return(curState);
}

/************************************************************************
 * AboutSameRect - are two rects "about equal"
 ************************************************************************/
Boolean AboutSameRect(Rect *r1,Rect *r2)
{
	short *s1 = (short*)r1;
	short *s2 = (short*)r2;
	short i=4;
	
	for (;i--;s1++,s2++) if (ABS(*s1-*s2)>7) return(False);
	return(True);
}

/************************************************************************
 * OutlineControl - outline the default button
 ************************************************************************/
void OutlineControl(ControlHandle cntl,Boolean blackOrWhite)
{
	SetControlData( cntl, 0,
		   kControlPushButtonDefaultTag, sizeof( blackOrWhite ),
		   (Ptr)&blackOrWhite );
	if (blackOrWhite)
		Draw1Control(cntl);
	else
	{
		//	Draw1Control won't remove the ring
		Rect	rUpdate;
		
		GetControlBounds(cntl,&rUpdate);
		InsetRect(&rUpdate,-5,-5);		
		InvalWindowRect(GetControlOwner(cntl),&rUpdate);
	}
}


/**********************************************************************
 * MyDisposeDrag - dispose of a drag & also the globals that go with it
 **********************************************************************/
void MyDisposeDrag(DragReference drag)
{
	DisposeDrag(drag);
	DragSource = nil;
	DragFxxkOff = True;
	DragSourceKind = 0;
}

/**********************************************************************
 * MyNewDrag - create a drag, and keep track of some stuff
 **********************************************************************/
OSErr MyNewDrag(MyWindowPtr win,DragReference *drag)
{
	OSErr err = NewDrag(drag);
	if (!err)
	{
		DragSource = win;
		DragFxxkOff = False;
		DragSourceKind = GetWindowKind(GetMyWindowWindowPtr(win)); // Who started it?
	}
	return(err);
}

/**********************************************************************
 * AppendDITLId - append to a ditl, by id
 **********************************************************************/
void AppendDITLId(DialogPtr dlog,short id,DITLMethod method)
{
	Handle res;

	if (res = GetResource('DITL',id))
		AppendDITL(dlog,res,method);
}

/************************************************************************
 * MyGetWTitle - get a window's title
 ************************************************************************/
PStr MyGetWTitle(WindowPtr theWindow,PStr title)
{
	*title = 0;
	GetWTitle(theWindow,title);
	return(title);
}

/************************************************************************
 * GetNthWindow - find the nth window
 ************************************************************************/
MyWindowPtr GetNthWindow(short n)
{
	WindowPtr		winWP;
	MyWindowPtr win;
	
	for (winWP=FrontWindow_();winWP;winWP = GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (win && win->windex == n) break;
	}

	return(GetWindowMyWindowPtr (winWP));
}

/************************************************************************
 * TrackRect - track the mouse in a rectangle
 ************************************************************************/
Boolean TrackRect(Rect *r)
{
	Point pt;
	Boolean in = False;
	
	do
	{
		GetMouse(&pt);
		if (in!=PtInRect(pt,r))
		{
			in = !in;
			InvertRect(r);
		}
	}
	while (StillDown());
	return(in);
}

/************************************************************************
 * DragMyGreyRgn - drag a grey region around
 * dragRgn - the region; local coords
 * pt - the original mouse point; local coords
 * DragProc - proc to be called repeatedly, with point, refCon
 *   refCon is 0 if the mouse button has gone up and cleanup should be done
 ************************************************************************/
long DragMyGreyRgn(RgnHandle dragRgn,Point pt,void(*DragProc)(Point*,RgnHandle,long),long refCon)
{
	Point newMouse,ptOffset;
	GrafPtr port;
	long	result;
	Rect	rStart,r,rPort;
	Point	newPos;
	Pattern	gray;
				
	PushGWorld();
	
	/*
	 * move to global coords and current mouseloc
	 */
//	GlobalizeRgn(dragRgn);
//	GetMouse(&newMouse);
//	OffsetRgn(dragRgn,newMouse.h-pt.h,newMouse.v-pt.v);
//	LocalToGlobal(&pt);
	
	OutlineRgn(dragRgn,2);

	/*
	 * open a new port to draw in
	 */
	MyCreateNewPort(port);
	SetPortVisibleRegion(port,GetGrayRgn());
	GetRegionBounds(MyGetPortVisibleRegion(port),&rPort);
	SetPortBounds(port,&rPort);
	SetPort(port);
	
	/*
	 * the grey part of all this
	 */
	PenPat(GetQDGlobalsGray(&gray));
	PenMode(patXor);
	
	/*
	 * draw the region
	 */
	PaintRgn(dragRgn);
	
	GetRegionBounds(dragRgn,&rStart);
	newPos = topLeft(rStart);
	ptOffset = newPos;
	SubPt(pt,&ptOffset);

	while (StillDown())
	{
		GetMouse(&newMouse);
		/*
		 * update stuff if the mouse has moved
		 */
		if (!EqualPt(newMouse,pt))
		{
			//	Erase old region
			PaintRgn(dragRgn);

			//	Where do we want to move the region to?			
			newPos = newMouse;
			AddPt(ptOffset,&newPos);

			//	Allow callback to modify new position and/or drag region
			if (DragProc) (*DragProc)(&newPos,dragRgn,refCon);
			
			//	Move the region and redraw
			GetRegionBounds(dragRgn,&r);
			OffsetRgn(dragRgn,newPos.h-r.left,newPos.v-r.top);
			PaintRgn(dragRgn);
			
			pt = newMouse;
		}
	}
	
	/*
	 * erase region and let callback know it's done
	 */
	PaintRgn(dragRgn);
	if (DragProc) (*DragProc)(&newPos,dragRgn,0);

	/*
	 * cleanup
	 */
	DisposePort(port);
	PopGWorld();
	
	//	return result (how far region moved)
	GetRegionBounds(dragRgn,&r);
	result = DeltaPoint(topLeft(r),topLeft(rStart));
	if (!result) result = 0x80008000;
	return result;
}

/************************************************************************
 * FrameXorContent - frame the content region with xor
 ************************************************************************/
void FrameXorContent(MyWindowPtr win)
{
	Rect r;
	PenState oldPen;
	short n;
	Pattern gray;

	PushGWorld();
	
	SetPort_(GetMyWindowCGrafPtr(win));
	GetPenState(&oldPen);
	
	r = win->contR;
	PenPat(GetQDGlobalsGray(&gray));
	PenMode(patXor);
	for (n=0;n<3;n++)
	{
		FrameRect(&r);
		InsetRect(&r,1,1);
	}
	SetPenState(&oldPen);
	
	PopGWorld();
}

/**********************************************************************
 * Frame3DRectDepth - frame a 3d rectangle, given a screen depth
 **********************************************************************/
void Frame3DRectDepth(Rect *bigRect, IconTransformType transform,short depth)
{
	short selector;
	static short greys[6][4]={
		/* 4 bit, normal */
		0xffff,	k4Grey1,	k4Grey2,	k4Grey3,
		/* 4 bit, selected */
		k4Grey3,	k4Grey2, k4Grey1, 0xffff,	
		/* 4 bit, dimmed */
		k4Grey1,	k4Grey1,	k4Grey1, k4Grey1,
		/* 8 bit, normal */
		0xffff,	k8Grey1,	k8Grey5, k8Grey7,
		/* 8 bit, selected */
		k8Grey8, k8Grey7, k8Grey2, k8Grey7,
		/* 8 bit, dimmed */
		k8Grey1, k8Grey1, k8Grey1, k8Grey1
	};
	Rect r;

	if (transform==ttDisabled && depth>=4)
		SetForeGrey(depth==4 ? k4Grey2 : k8Grey4);
	FrameRect(bigRect);
	SetForeGrey(0);
	
	if (depth<4 || !D3) return;
		
	selector = (depth==4) ? 0 : 3;
	if (transform==ttSelected) selector++;
	else if (transform==ttDisabled) selector += 2;
	
	r = *bigRect;
	InsetRect(&r,1,1);
	FrameGreys(&r,greys[selector]);
	SetForeGrey(0);
}

/**********************************************************************
 * FrameGreys - draw a rectangle, using certain grey values
 **********************************************************************/
void FrameGreys(Rect *r, short *greys)
{
	/*
	 * left/top, outer
	 */
	SetForeGrey(greys[0]);
	MoveTo(r->left,r->bottom-2); LineTo(r->left,r->top); LineTo(r->right-2,r->top);
	
	/*
	 * left/top, inner
	 */
	SetForeGrey(greys[1]);
	MoveTo(r->left+1,r->bottom-3); LineTo(r->left+1,r->top+1); LineTo(r->right-3,r->top+1);

	/*
	 * bottom/right, inner
	 */
	SetForeGrey(greys[2]);
	MoveTo(r->left+1,r->bottom-2); LineTo(r->right-2,r->bottom-2); LineTo(r->right-2,r->top+2);
	
	/*
	 * bottom/right, outer
	 */
	SetForeGrey(greys[3]);
	MoveTo(r->left,r->bottom-1); LineTo(r->right-1,r->bottom-1); LineTo(r->right-1,r->top);
}

#if 0
/**********************************************************************
>>>> ORIGINAL winutil.c#71
 * LopCornerRect - Draw a rectangle without corners
 **********************************************************************/
Rect LopCornerRect(Rect *r)
{
	MoveTo(r->left+1,r->top); LineTo(r->right-2,r->top);
	MoveTo(r->left+1,r->bottom-1); LineTo(r->right-2,r->bottom-1);
	MoveTo(r->left,r->top+1); LineTo(r->left,r->bottom-2);
	MoveTo(r->right-1,r->top+1); LineTo(r->right-1,r->bottom-2);
}

/**********************************************************************
==== THEIRS winutil.c#72
==== YOURS winutil.c
 * LopCornerRect - Draw a rectangle without corners
 **********************************************************************/
Rect LopCornerRect(Rect *r)
{
	MoveTo(r->left+1,r->top); LineTo(r->right-2,r->top);
	MoveTo(r->left+1,r->bottom-1); LineTo(r->right-2,r->bottom-1);
	MoveTo(r->left,r->top+1); LineTo(r->left,r->bottom-2);
	MoveTo(r->right-1,r->top+1); LineTo(r->right-1,r->bottom-2);
}
#endif

/**********************************************************************
<<<<
 * SetPortMyWinColors - Function replacement for SET_COLORS macro that
 *											does the window stuff very carefully
 **********************************************************************/
void SetPortMyWinColors (void)

{
	WindowPtr		qdPortWP;
	MyWindowPtr qdPortWin;
	
	qdPortWP = GetWindowFromPort(GetQDGlobalsThePort());
	if (IsMyWindow (qdPortWP))
		if (qdPortWin = GetWindowMyWindowPtr (qdPortWP)) {
			RGBForeColor (&qdPortWin->textColor);
			RGBBackColor (&qdPortWin->backColor);
		}
}


/**********************************************************************
 * SavePortStuff - save stuff for the current port
 **********************************************************************/
void SavePortStuff(PortSaveStuffPtr stuff)
{
	// Yes, this can actually be nil...
	if (!GetQDGlobalsThePort())
		return;

	// Check to see where qd.thePort lives in memory.  New control manager definitions
	// appear to do their drawing offscreen and (supposedly) manage port safety themselves.
	// If we see that the current port is not in our heap -- or in the system heap -- then
	// it's likely that the OS has allocated a backing store in some other zone, indicating
	// (perhaps) that "all the right stuff will happen anyway".
	//
	// So, we'll turn off SpotLight when this is the case
		
	//if the current is a MyWindowPtr, then we need to save its pitches, too
	if(IsMyWindow(GetWindowFromPort(GetQDGlobalsThePort())))
	{
		MyWindowPtr	win = GetPortMyWindowPtr (GetQDGlobalsThePort());

		stuff->hPitch = win->hPitch;
		stuff->vPitch = win->vPitch;
	}

#if TARGET_CPU_PPC
	if ((long)GetThemeDrawingState != kUnresolvedCFragSymbolAddress)
	{
		GetThemeDrawingState(&stuff->themeState);
		stuff->txFace = GetPortTextFace(GetQDGlobalsThePort());	// ThemeDrawingState misses this.
	}
	else
#endif
	{
		stuff->themeState = nil;
		stuff->txFont = GetPortTextFont(GetQDGlobalsThePort());
		stuff->txFace = GetPortTextFace(GetQDGlobalsThePort());
		stuff->txSize = GetPortTextSize(GetQDGlobalsThePort());
		stuff->txMode = GetPortTextMode(GetQDGlobalsThePort());
		GetPenState(&stuff->pnState);
		if (ThereIsColor)
		{
			GetForeColor(&stuff->fore);
			GetBackColor(&stuff->back);
		}
	}
	SLEnable ();
}

/**********************************************************************
 * SetPortStuff - set stuff for a port
 **********************************************************************/
void SetPortStuff(PortSaveStuffPtr stuff)
{
	GrafPtr thePort;
	MyWindowPtr	win;

	GetPort (&thePort);
	if (!thePort)
		return;
		
	// See SpotLight comments in SavePortStuff, above
	//if it's a MyWindowPtr then we need to restore its pitches also
	if(IsMyWindow(GetWindowFromPort(thePort)))
	{
		win = GetPortMyWindowPtr (thePort);
		win->hPitch = stuff->hPitch;
		win->vPitch = stuff->vPitch;
	}

#if TARGET_CPU_PPC
	if ((long)SetThemeDrawingState != kUnresolvedCFragSymbolAddress)
	{
		SetThemeDrawingState(stuff->themeState,false);
		TextFace(stuff->txFace);	// ThemeDrawingState misses this.
	}
	else
#endif
	{
		TextFont(stuff->txFont);
		TextSize(stuff->txSize);
		TextFace(stuff->txFace);
		TextMode(stuff->txMode);
		if (ThereIsColor)
		{
			SetPenState(&stuff->pnState);
			RGBForeColor(&stuff->fore);
			RGBBackColor(&stuff->back);
		}
	}
	SLEnable ();
}

/**********************************************************************
 * DisposePortStuff - dispose stupid appearance 1.1 theme drawing stuff
 **********************************************************************/
void DisposePortStuff(PortSaveStuffPtr stuff)
{
#if TARGET_CPU_PPC
	if (stuff->themeState && (long)DisposeThemeDrawingState != kUnresolvedCFragSymbolAddress)
		DisposeThemeDrawingState(stuff->themeState);
#endif
	stuff->themeState = nil;
}

struct GWStuff
{
	CGrafPtr gp;
	GDHandle gd;
};

/**********************************************************************
 * 
 **********************************************************************/
void PushGWorld(void)
{
	struct GWStuff stuff;
	
	if (GWStack || !StackInit(sizeof(struct GWStuff),&GWStack))
	{
		GetGWorld(&stuff.gp,&stuff.gd);
		StackPush(&stuff,GWStack);
	}
}

void PopGWorld(void)
{
	struct GWStuff stuff;
	
	if (GWStack && !StackPop(&stuff,GWStack))
		SetGWorld(stuff.gp,stuff.gd);
}

/**********************************************************************
 * 
 **********************************************************************/
void Frame3DRect(Rect *r, IconTransformType transform)
{
	Frame3DRectDepth(r,transform,RectDepth(r));
}

/**********************************************************************
 * RectDepth - get the depth of the screen a rect is mostly on
 **********************************************************************/
short RectDepth(Rect *localRect)
{
	Rect globalRect;
	GDHandle gd;
	
	if (!ThereIsColor) return(1);
	
	globalRect = *localRect;
	LocalToGlobal((Point*)&globalRect.top);
	LocalToGlobal((Point*)&globalRect.bottom);
	utl_GetRectGD(&globalRect,&gd);
	if (!gd) return(1);
	
	return((*(*gd)->gdPMap)->pixelSize);
}

/**********************************************************************
 * ReplaceAllControls - replace all controls with ones using the window font
 **********************************************************************/
void ReplaceAllControls(DialogPtr dlog)
{
	short n = CountDITL(dlog);
	Rect r;
	Handle itemH;
	short type;
	short item;
		
	for (item=1;item<=n;item++)
	{
		GetDialogItem(dlog,item,&type,&itemH,&r);
		if (type==(ctrlItem+chkCtrl)||type==(ctrlItem+radCtrl))
			ReplaceControl(dlog,item,type,(ControlHandle*)&itemH,&r);
	}
}

void OffsetDItem(DialogPtr dlog,short item,short dh,short dv)
{
	short type;
	Handle itemH=nil;
	Rect r;
	
	GetDialogItem(dlog,item,&type,(void*)&itemH,&r);
	if (itemH)
	{
		OffsetRect(&r,dh,dv);
		SetDialogItem(dlog,item,type,(void*)itemH,&r);
		
		if ((type&ctrlItem) == ctrlItem)
			MySetCntlRect((ControlHandle)itemH,&r);
	}
}

/**********************************************************************
 * ReplaceControl - replace a control with one using the right font
 **********************************************************************/
void ReplaceControl(DialogPtr dlog,short item,short type,ControlHandle *itemH,Rect *r)
{
	SetBevelFontSize(*itemH,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
}

/************************************************************************
 * ChangeTEFont - set the font of a TERec
 ************************************************************************/
void ChangeTEFont(TEHandle teh,short font,short size)
{
	if (teh)
	{
		(*teh)->txFont = font;
		(*teh)->txSize = size;
		(*teh)->lineHeight = GetLeading(font,size);
		(*teh)->fontAscent = GetAscent(font,size);
	}
}

/************************************************************************
 * MouseStill - is the user holding the mouse still?
 ************************************************************************/
Boolean MouseStill(uLong forHowManyTicks)
{
	Point orig,now;
	uLong origTicks = TickCount();
	short tolerance = GetRLong(DOUBLE_TOLERANCE);
	
	GetMouse(&orig);
	
	while (origTicks+forHowManyTicks>TickCount())
	{
		if (!StillDown())
			return(False);
			
		GetMouse(&now);
		if (ABS(orig.h-now.h)>tolerance || ABS(orig.v-now.v)>tolerance)
			return(False);
	}
	return(True);
}

/**********************************************************************
 * HiInvertRect - invertrect with hilite color
 **********************************************************************/
void HiInvertRect(Rect *r)
{
	SetHiliteMode();
	InvertRect(r);
}	

/**********************************************************************
 * HiInvertRect - invertrect with hilite color
 **********************************************************************/
void HiInvertRgn(RgnHandle r)
{
	SetHiliteMode();
	InvertRgn(r);
}

/**********************************************************************
 * OutlineRgn - outline a region
 **********************************************************************/
OSErr OutlineRgn(RgnHandle rgn,short inset)
{
	RgnHandle inner = NewRgn();
	
	if (!inner) return(MemError());
	
	CopyRgn(rgn,inner);
	InsetRgn(inner,inset,inset);
	DiffRgn(rgn,inner,rgn);
	DisposeRgn(inner);
	return(noErr);
}

/**********************************************************************
 * DragIsInteresting - decide if a drag is interesting
 **********************************************************************/
Boolean DragIsInteresting(DragReference drag,...)
{
	Boolean interesting=False;
	short item, flavor;
	OSType type;
	OSType wantType;
	FlavorFlags flags;
	va_list args;
	short modifiers, junk;
	HFSFlavor **data;
	
	
	for (item=MyCountDragItems(drag);item;item--)	// loop through each item
	{
		interesting = False;	// each item must have at least on interesting flavor
		
		for (flavor=MyCountDragItemFlavors(drag,item);flavor;flavor--)	// loop through each flavor
		{
			type = MyGetDragItemFlavorType(drag,item,flavor);
			flags = MyGetDragItemFlavorFlags(drag,item,flavor);
			
			if (!(flags & flavorSenderOnly) || DragSource)
			{
				va_start(args,drag);	// loop through the list of acceptable types
				for (wantType=va_arg(args,OSType);!interesting && wantType;wantType=va_arg(args,OSType))
				{
					interesting = (type==wantType);
					if (wantType==flavorTypeHFS &&
						  !GetDragModifiers(drag,&junk,&modifiers,&junk) &&
						  !(modifiers&cmdKey))
					{
						if (!MyGetDragItemData(drag,item,flavorTypeHFS,(void*)&data))
						{
							interesting = !(*(short*)&(*data)->fileType=='il' && (*data)->fileCreator=='drag' ||
														 (*data)->fileType=='clpt' || (*data)->fileType=='clpp' && !PrefIsSet(PREF_SEND_ENRICHED_NEW));
							ZapHandle(data);
						}
					}
				}
				va_end(args);
				if (interesting) break;	// once we find at least on interesting flavor, we're done with this item
			}
		}
		if (!interesting) break;	// if we find an item with no interesting flavors, it's over
	}
	
	return(interesting);
}

/************************************************************************
 * DragIsImage - is a drag an image?
 ************************************************************************/
Boolean DragIsImage(DragReference drag)
{
	return DragIsInterestingFileType(drag,IsGraphicFile,nil);
}

/************************************************************************
 * DragIsInterestingFileType - is a drag an image?
 ************************************************************************/
Boolean DragIsInterestingFileType(DragReference drag,Boolean (*testFunc)(FSSpecPtr),...)
{
	static Boolean isInteresting;
	static uLong sequence;
	short item, flavor;
	Boolean interesting;
	HFSFlavor **data;
	FlavorFlags flags;
	OSType type;
	FSSpec spec;
	
	// let's not do this over and over
	if (sequence==DragSequence) return(isInteresting);
	sequence = DragSequence;
	
	// gotta do it.	
	for (item=MyCountDragItems(drag);item;item--)	// loop through each item
	{
		interesting = False;	// each item must have at least one image
		for (flavor=MyCountDragItemFlavors(drag,item);flavor;flavor--)	// loop through each flavor
		{
			type = MyGetDragItemFlavorType(drag,item,flavor);
			flags = MyGetDragItemFlavorFlags(drag,item,flavor);
			
			if (type==flavorTypeHFS && (!(flags & flavorSenderOnly) || DragSource))
			{
				if (!MyGetDragItemData(drag,item,flavorTypeHFS,(void*)&data))
				{
					spec = (*data)->fileSpec;
					
					// does the caller want to do his own testing?
					if (testFunc)
						interesting = (*testFunc)(&spec);
					// nope, we need to test against a list of types
					else
					{
						OSType wantType;
						OSType haveType = FileTypeOf(&spec);
						va_list args;
						
						va_start(args,testFunc);
						for (wantType=va_arg(args,OSType);!interesting && wantType;wantType=va_arg(args,OSType))
							interesting = haveType==wantType;
						va_end(args);
					}
					ZapHandle(data);
				}
				if (interesting) break;	// once we find at least one interesting flavor, we're done with this item
			}
		}
		if (!interesting) break;	// if we find an item with no interesting flavors, it's over
	}
	
	return(isInteresting = interesting);
}

#ifdef DEBUG
/************************************************************************
 * DebugCdef - a control for debugging purposes
 ************************************************************************/
pascal long DebugCdef(short varCode, ControlHandle theControl, short message, long param)
{
	Str255 title;
	Rect r;
	GetControlTitle(theControl,title);
	Dprintf("\p%p (%d): m%d v%d p%x;g",title,GetControlReference(theControl),message,varCode,param);
	
	GetControlBounds(theControl,&r);
	switch(message)
	{
		case drawCntl:
			FrameRect(&r);
			MoveTo(r.left,r.top); LineTo(r.right,r.bottom);
			MoveTo(r.left,r.bottom); LineTo(r.right,r.top);
			break;
			
		case calcCRgns:
		case calcCntlRgn:
			if (message==calcCRgns) param &= 0x00ffffff;
			RectRgn((RgnHandle)param,&r);
			break;
	}
	return(0);
}
	
/************************************************************************
 * ShowGlobalRgn - show a region in global coords
 ************************************************************************/
void ShowGlobalRgn(RgnHandle globalRgn,UPtr string)
{
	GrafPtr wmgr;
	PushGWorld();
	GetWMgrPort(&wmgr);
	SetPort(wmgr);
	InvertRgn(globalRgn);
	if (EmptyRgn(globalRgn)) DebugStr("\pempty!;g");
	DebugStr(string);
	InvertRgn(globalRgn);
	PopGWorld();
}


/************************************************************************
 * 
 ************************************************************************/
void ShowLocalRgn(RgnHandle localRgn,UPtr string)
{
	GlobalizeRgn(localRgn);
	ShowGlobalRgn(localRgn,string);
	LocalizeRgn(localRgn);
}
#endif

#undef DrawDialog
#undef UpdateDialog
/************************************************************************
 * MyDrawDialog - update a dialog window
 ************************************************************************/
void MyDrawDialog(DialogPtr dgPtr)
{
	short oldResFile = CurResFile();

	DrawDialog(dgPtr);
	UseResFile(oldResFile);
}

/************************************************************************
 * DlgUpdate - update a dialog window
 ************************************************************************/
void DlgUpdate(MyWindowPtr win)
{
	short oldResFile = CurResFile();

	UpdateDialog(GetMyWindowDialogPtr(win),MyGetPortVisibleRegion(GetMyWindowCGrafPtr(win)));
	UseResFile(oldResFile);
}

#undef RGBBackColor
#undef RGBForeColor
/************************************************************************
 * MyRGBBackColor - set the background color and pattern
 ************************************************************************/
void MyRGBBackColor(RGBColor *color)
{
	Pattern	white;
	RGBBackColor(color);
	BackPat(GetQDGlobalsWhite(&white));
}

/************************************************************************
 * MyRGBForeColor - set the foreground color and pattern
 ************************************************************************/
void MyRGBForeColor(RGBColor *color)
{
	Pattern	black;
	RGBForeColor(color);
	PenPat(GetQDGlobalsBlack(&black));
}

/************************************************************************
 * ScreenChange - screen resolution has changed, make sure all windows are on screen
 ************************************************************************/
void ScreenChange(void)
{
	WindowPtr		winWP;
	MyWindowPtr	win;

	//	Docked windows first
	PositionDockedWindows();
	
	for (winWP=GetWindowList();winWP;winWP=GetNextWindow(winWP))
	{
		win = GetWindowMyWindowPtr (winWP);
		if (!IsKnownWindowMyWindow(winWP) || (win)->windowType != kDockable || !IsWindowVisible(winWP))	//	Don't do docked windows
		if (IsWindowVisible(winWP))
		{
			Rect r;
			Boolean zoomed;
			GDHandle gd;
			Boolean	moveIt = false;

			utl_SaveWindowPos(winWP,&r,&zoomed);
			utl_GetRectGD(&r,&gd);
			if (gd)
			{
				//	Make sure title bar is on screen
				Rect	rTitle,rTemp,rCont;
				
				GetStructureRgnBounds(winWP,&rTitle);
				GetContentRgnBounds(winWP,&rCont);
				rTitle.bottom = rCont.top;
				if (!SectRect(&rTitle,&(*gd)->gdRect,&rTemp))
					moveIt = true;
			}
			else
				moveIt = true;
			
			if (moveIt)
			{
				//	Put window back on screen.
				SanitizeSize(win,&r);
				utl_RestoreWindowPos(winWP,&r,zoomed,1,TitleBarHeight(winWP),LeftRimWidth(winWP),(void*)FigureZoom,(void*)DefPosition);
			}
		}
	}
}
 
/************************************************************************
 * MyStdFilterProc - support cmd-key equivalents for dialog buttons
 ************************************************************************/
Boolean MyStdFilterProc (DialogRef dgPtr,EventRecord *event,DialogItemIndex *item)
{
	Str255	itemText;
	
	if (event->what==keyDown && event->modifiers&cmdKey && !MenuKey(UnadornMessage(event)))
	{
		char key = event->message & charCodeMask;
		short button = 0;
		short i;

		if (islower(key)) key = toupper(key);
		for (i=CountDITL(dgPtr);i;i--)
		{
			DialogItemType itemType;
			Handle	itemCtrl;
			Rect	rItem;
			
			GetDialogItem(dgPtr,i,&itemType,&itemCtrl,&rItem);
			if (itemType >= btnCtrl && itemType <= ctrlItem)
			{
				GetControlTitle((ControlRef)itemCtrl,itemText);
				if (*itemText && key==(islower(itemText[1]) ? toupper(itemText[1]) : itemText[1]))
				{
					//	Found a button starting with this cmd-key
					if (button)
					{
						//	This is a duplicate. Just beep and return since we
						//	don't know which one the user wants
						SysBeep(10);
						return false;
					}
					else
						button = i;
				}
			}
		}
		if (button)
		{
			*item = button;
			return true;
		}
	}
	return StdFilterProc(dgPtr,event,item);
}