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

#include "wazoo.h"

#define FILE_NUM 92
/* Copyright (c) 1997 by QUALCOMM Incorporated */

#pragma segment Wazoo

/*
 *	constants
 */
#define kEmbedderId		'Embr'
#define kTabsId		'Tabs'
#define kMaxTabs 16

//      Tab drawing specs
#define kTabLeftMargin 3	// Start first tab here from left
#define kTabCtlTop (GetRLong(WAZOO_TOPMARGIN)-GetRLong(WAZOO_TABHEIGHT)-INSET)

//      Constants used in saving contents of a MyWindow structure minus the WindowRecord
enum { kSaveDataOffset = 0, kSaveDataSize = sizeof(MyWindow) };
//enum { kSaveDataOffset = sizeof(DialogRecord), kSaveDataSize = sizeof(MyWindow)-kSaveDataOffset };

/*
 *	globals
 */
//      List of wazooable windows, menu items, functions that open them
typedef struct {
	short kind;		//      window kind
	short windRsrc;		//      resource ID of WIND resource
	void (*openFunction)(void);	//      function to open this window
	FeatureType dependentFeature;	// feature this wazoo depends on (if any)
	short temp;		//      used to remember which window are currently open when restoring wazoos
} WazooableItem;

static WazooableItem gWazooKinds[] = {
#ifdef TASK_PROGRESS_ON
	TASKS_WIN, TASKS_WIND, OpenTasksWin, featureNone, 0,
#endif
	MB_WIN, MBWIN_WIND, OpenMBWin, featureNone, 0,
	PH_WIN, PH_WIND, nil, featureNone, 0,	//      The open function needs a parameter
	ALIAS_WIN, ALIAS_WIND, OpenABWin, featureNone, 0,
	FILT_WIN, FILT_WIND, nil, featureNone, 0,
	PERS_WIN, PERSONALITIES_WIND, OpenPersonalitiesWin,
	    featureMultiplePersonalities, 0,
	STA_WIN, STATIONERY_WIND, OpenStationeryWin, featureStationery, 0,
	SIG_WIN, SIGNATURES_WIND, OpenSignaturesWin, featureNone, 0,
	LINK_WIN, LINK_WIND, OpenLinkWin, featureLink, 0,
	STAT_WIN, STAT_WIND, OpenStatWin, featureNone, 0
};

typedef struct {
	long winRefCon;
	ControlHandle ctlEmbedder;
	Byte winData[kSaveDataSize];
} SaveWazooData, *SaveWazooPtr, **SaveWazooHandle;

//      Info saved in resource for each item in a wazoo
typedef struct {
	short kind;
	SaveWazooHandle hSaveInfo;
	Point minSize;
	Str32 name;
} WazooItem;

//      Wazoo list stored in resource of type kWazooListResType
typedef struct {
	short count;		//      Number of windows in list
	Boolean zoomed;
	Rect position;
	WazooItem kindList[];	//      List of window types in wazoo
} WazooRes, *WazooResPtr, **WazooResHandle;

//      Wazoo list
typedef struct WazooDataStruct *WazooDataPtr, **WazooDataHandle;
enum { kIconsAndNames, kCondensedNames, kIconsOnly };
typedef struct WazooDataStruct {
	short current;		//      Which one is currently active? (0 - N-1)
	WazooDataHandle next;
	MyWindowPtr win;
	WazooResHandle list;	//      Wazoo list resource
	ControlHandle hTabCtrl;
	ControlHandle hTabColorCtrl;
	short tabDisplayMode;	//      Show names, condensed, tabs only
	Rect tabRect[kMaxTabs + 1];	//      Rect of each tab
} WazooData;

//      In debug builds, we check this somewhere else
#ifndef DEBUG
static
#endif
WazooDataHandle gWazooListHead;

//      Info for dragging a wazoo tab
typedef struct {
	MyWindowPtr winSrc, winDest;
	short tabIdx;
	WazooItem tabData;
} DragData;

typedef enum { kDragInWazoo, kDragOutsideWazoo, kDragDone,
	    kDragHid } DragDrawMode;
typedef struct {
	long a5;
	RgnHandle rgnWindow;
	Point lastMouse;
	DragDrawMode mode, lastDrawMode;
	RgnHandle tempRgn, saveClip;
} DragOutlineInfo;

static DragData gDragData;
static Boolean gDontDragHilite;
static Boolean gForceNewWazoo;	//      Make the next new window a wazoo?
static short gForceHPos, gForceVPos;
static DragOutlineInfo *gpDragOutline;

/*
 *	prototypes
 */
static void CheckTabResize(MyWindowPtr win, WazooDataHandle hWazooData,
			   WazooResHandle hWazooRes, short tabDisplayMode);
static Boolean CloseAWazoo(MyWindowPtr win, WazooDataHandle hWazooData,
			   WazooResHandle hWazooRes, short index);
static Boolean CloseAllButCurrent(MyWindowPtr win,
				  WazooDataHandle hWazooData,
				  WazooResHandle hWazooRes);
static void DisposeDragRgn(RgnHandle rgn);
static void DisposeWazoo(WazooDataHandle hWazooData,
			 WazooResHandle hWazooRes);
static void DoDragHilite(MyWindowPtr win, DragReference drag,
			 Rect * rHilite, Boolean fShow);
static void DragWazoo(MyWindowPtr win, Rect * rTab, Boolean * dontActivate,
		      short tab);
static Boolean GetWazooData(MyWindowPtr win, WazooDataHandle * hWazooData,
			    WazooResHandle * hWazooRes);
static Boolean IsKindWazooable(short thisKind);
static RgnHandle MakeDragRegion(Rect * rTab, short tab,
				WazooDataHandle hWazooData);
static void OpenWazoo(short kind);
static short PtInTab(MyWindowPtr win, Point pt);
static void RemoveTab(MyWindowPtr win, short tabIdx);
static Boolean RestoreWazooWindow(MyWindowPtr win,
				  WazooDataHandle hWazooData, short idx,
				  Boolean quietly);
static Boolean SaveCurrentWazoo(MyWindowPtr win,
				WazooDataHandle hWazooData,
				WazooResHandle hWazooRes,
				Boolean switching);
static void SelectWazoo(MyWindowPtr win, short idx);
static void SetWinFields(MyWindowPtr win, Ptr winData);
static WazooDataHandle FindWazoo(short windowKind, short *idx);
static ControlHandle EmbedTheControls(MyWindowPtr win,
				      ControlHandle ctlEmbedder);
static void CalcWazooMinSize(MyWindowPtr win);
static void SetWazooMinSize(short kind, short h, short v);
static void CheckMinWinSize(MyWindowPtr win, Point minSize);
static Boolean MustSaveWazoo(MyWindowPtr win);
static void SetupTabs(WazooDataHandle hWazooData, WazooResHandle hWazooRes,
		      Boolean draw);
static void SetCurrentTab(ControlHandle ctl, short tab);
static ControlHandle GetEmbedder(WazooDataHandle hWazooData, short idx,
				 Boolean create);
static void SetupCustomTabs(MyWindowPtr win, WazooDataHandle hWazooData,
			    WazooResHandle hWazooRes);
static RgnHandle GetTabRgn(Rect * r);
static void CalcTabRects(WazooDataHandle hWazooData,
			 WazooResHandle hWazooRes);

#define SelectWazooWithUpdate(aMyWindowPtr,i) do{UpdateMyWindow(GetMyWindowWindowPtr(aMyWindowPtr));SelectWazoo(aMyWindowPtr,i);}while(0)

static pascal OSErr DrawDrag(DragRegionMessage message,
			     RgnHandle showRegion, Point showOrigin,
			     RgnHandle hideRegion, Point hideOrigin,
			     void *dragDrawingRefCon, DragReference drag);
static pascal void SetupTabBackground(ControlHandle control,
				      ControlBackgroundPtr info);

/*
//	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	
//	Translucent dragging stuff
//
//	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

enum
{
    _DragDispatch = 0xABED
};

enum
{
    gestaltDragMgrHasImageSupport = 3
};

typedef unsigned long DragImageFlags;

enum 
{
    dragStandardImage   = 0x00000000,
    dragDarkImage       = 0x00000001,
    dragDarkerImage     = 0x00000002,
    dragOpaqueImage     = 0x00000003,
    dragRegionAndImage  = 0x00000010
};

pascal OSErr SetDragImage ( DragReference   theDragRef,
                            PixMapHandle    imagePixMap,
                            RgnHandle       imageRgn,
                            Point           imageOffsetPt,
                            DragImageFlags  theImageFlags   );
*/


/************************************************************************
 * GetNewWazoo - a new window is being opened, is it a wazoo?
 ************************************************************************/
MyWindowPtr GetNewWazoo(short windowKind, Boolean * fIsWazoo)
{
	MyWindowPtr win = nil;
	short idx;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	Str32 title;

	if (hWazooData = FindWazoo(windowKind, &idx)) {
		//      It's a wazoo
		*fIsWazoo = true;
		hWazooRes = (*hWazooData)->list;
		if (win = (*hWazooData)->win) {
			WindowPtr winWP = GetMyWindowWindowPtr(win);
			//      Wazoo window is already open, just switch
			if (SaveCurrentWazoo
			    (win, hWazooData, (*hWazooData)->list, true)) {
				SetMyWindowPrivateData(win, 0);
				win->pteList = nil;
				PeteFocus(win, nil, true);
				//      Init everything to zero
				SetWinFields(win, nil);
				//      Window has a new name
				PCopy(title,
				      (*hWazooRes)->kindList[idx].name);
				SetWTitle(winWP, title);
				UserSelectWindow(winWP);

				//      Don't let drawing take place until we're ready
				//      to display it
				SetEmptyClipRgn(GetWindowPort(winWP));
				win->noUpdates = true;
			}
		}
		(*hWazooData)->current = idx;
	} else if (IsKindWazooable(windowKind))
		*fIsWazoo = gForceNewWazoo;	//      Make this new window a wazoo?
	else
		*fIsWazoo = false;
	return win;
}

/************************************************************************
 * PositionWazoo - position the wazoo window
 ************************************************************************/
void PositionWazoo(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	Rect rPosition;

	if (gForceNewWazoo && IsWazoo(winWP)) {
		//      Move to specified location
		if (win->position) {
			(*win->position) (False, win);
			MoveWindow(winWP, gForceHPos, gForceVPos, false);
			MyWindowDidResize(win, nil);
		}
	} else if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		rPosition = (*hWazooRes)->position;
		utl_RestoreWindowPos(winWP, &rPosition,
				     (*hWazooRes)->zoomed, 1,
				     TitleBarHeight(winWP),
				     LeftRimWidth(winWP),
				     (void *) FigureZoom,
				     (void *) DefPosition);
		MyWindowDidResize(win, nil);
	}
}


/************************************************************************
 * SelectOpenWazoo - see if we can switch to a previous-opened wazoo
 ************************************************************************/
Boolean SelectOpenWazoo(short windowKind)
{
	MyWindowPtr win;
	short idx;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if ((hWazooData = FindWazoo(windowKind, &idx)) &&
	    (win = (*hWazooData)->win) &&
	    (hWazooRes = (*hWazooData)->list) &&
	    (idx == (*hWazooData)->current
	     || (*hWazooRes)->kindList[idx].hSaveInfo)) {
		SelectWazooWithUpdate(win, idx);
		UserSelectWindow(GetMyWindowWindowPtr(win));
		return true;
	}
	return false;
}

/************************************************************************
 * FindOpenWazoo - return window ptr of wazoo based on window kind
 ************************************************************************/
MyWindowPtr FindOpenWazoo(short windowKind)
{
	short idx;
	WazooDataHandle hWazooData = FindWazoo(windowKind, &idx);

	return hWazooData ? (*hWazooData)->win : nil;
}

/************************************************************************
 * CloseWazoo - close wazoo window
 ************************************************************************/
Boolean CloseWazoo(MyWindowPtr win)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		short current;
		Rect rWindow;
		Boolean zoomed;

		//      Close the other ones first
		current = (*hWazooData)->current;
		if (!CloseAllButCurrent(win, hWazooData, hWazooRes))
			return false;

		//      Now close the current one
		if (!CloseAWazoo(win, hWazooData, hWazooRes, current))
			return false;

		utl_SaveWindowPos(GetMyWindowWindowPtr(win), &rWindow,
				  &zoomed);
		if (!EqualRect(&rWindow, &(*hWazooRes)->position)
		    || zoomed != (*hWazooRes)->zoomed) {
			(*hWazooRes)->position = rWindow;
			(*hWazooRes)->zoomed = zoomed;
			ChangedResource((Handle) hWazooRes);
		}

		(*hWazooData)->win = nil;
	}
	return true;
}

/************************************************************************
 * PromoteToWazoo - convert a normal window to a wazoo window
 ************************************************************************/
void PromoteToWazoo(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	Str255 sTitle;
	Rect r;
	Boolean zoomed;
	OSErr err;
	ControlHandle ctl, ctlColor;
	short windowKind;
	ResType resType =
	    IsFreeMode()? kFreeWazooListResType : kWazooListResType;;

	windowKind = GetWindowKind(winWP);
	//      Verify that this window kind is wazooable. Shouldn't get here if it isn't (but
	//      apparently it has happened).
	if (!IsWazooable(winWP))
		return;

	if (!(hWazooData = FindWazoo(windowKind, nil))) {
		//      New wazoo
		if (!(hWazooData = NuHandleClear(sizeof(WazooData))))
			return;	//      Err
		if (!
		    (hWazooRes =
		     NuHandleClear(sizeof(WazooRes) + sizeof(WazooItem))))
			return;	//      Err
		//      Do wazoo list resource
		(*hWazooRes)->count = 1;
		(*hWazooRes)->kindList[0].kind = windowKind;
		MyGetOriginalWTitle(win, sTitle);
		if (*sTitle > 31)
			*sTitle = 31;	//      Make sure name isn't too long
		PCopy((*hWazooRes)->kindList[0].name, sTitle);
		(*hWazooRes)->kindList[0].minSize = win->minSize;
		win->minSize.v += GetRLong(WAZOO_TOPMARGIN);

		//      Get window position
		utl_SaveWindowPos(winWP, &r, &zoomed);
		(*hWazooRes)->position = r;
		(*hWazooRes)->zoomed = zoomed;
		AddResource_(hWazooRes, resType, Unique1ID(resType), "");

		//      Do wazoo data for win
		LL_Push(gWazooListHead, hWazooData);
		(*hWazooData)->list = hWazooRes;

	}

	SetTopMargin(win, GetRLong(WAZOO_TOPMARGIN));

	//      Do win stuff
	(*hWazooData)->win = win;
	win->wazooData = (Handle) hWazooData;
	CalcWazooMinSize(win);

	//      Set up a tab control
	GetPortBounds(GetWindowPort(winWP), &r);
	r.bottom = GetRLong(WAZOO_TOPMARGIN);

	r.top = kTabCtlTop;
	r.left--;
	r.right++;
	ctl =
	    NewControl(winWP, &r, "", false, 0, 0, 0,
		       kControlTabLargeNorthProc, kTabsId);

	if (ctl) {
		err = EmbedControl(ctl, win->topMarginCntl);
//              ASSERT(!err);
		(*hWazooData)->hTabCtrl = ctl;
		SetupTabs(hWazooData, (*hWazooData)->list, false);
		SetControlVisibility(ctl, true, false);

		//      Embed a control within the tab control so we can get it's background color
		SetRect(&r, 10, GetRLong(WAZOO_TABHEIGHT) + 10, 15,
			GetRLong(WAZOO_TABHEIGHT) + 15);
		if (ctlColor =
		    NewControl(winWP, &r, "", false, 0, 0, 0,
			       kControlUserPaneProc, 0)) {
			err = EmbedControl(ctlColor, ctl);
			ASSERT(!err);
			(*hWazooData)->hTabColorCtrl = ctlColor;
		}
	}

	//      Make sure window isn't too small with the tabs
	CheckMinWinSize(win, win->minSize);

	MyWindowDidResize(win, nil);
}

/************************************************************************
 * DemoteWazoo - demote a wazoo to a normal window
 ************************************************************************/
void DemoteWazoo(MyWindowPtr win)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		Point minSize;
		short i;

		minSize =
		    (*hWazooRes)->kindList[(*hWazooData)->current].minSize;
		if (!CloseAllButCurrent(win, hWazooData, hWazooRes))
			return;
		DisposeControl((*hWazooData)->hTabCtrl);
		DisposeControl((*hWazooData)->hTabColorCtrl);

		//      dispose of saved data for each window in wazoo
		for (i = 0; i < (*hWazooRes)->count; i++) {
			SaveWazooHandle hSaveInfo;

			if (hSaveInfo =
			    (*hWazooRes)->kindList[i].hSaveInfo) {
				ControlHandle ctlEmbedder;

				if (ctlEmbedder =
				    (*hSaveInfo)->ctlEmbedder) {
					//      Un-embed all controls before disposing of embedder control
					UInt16 count;
					ControlHandle ctl, ctlRoot;
					WindowPtr winWP =
					    GetMyWindowWindowPtr(win);

					GetRootControl(winWP, &ctlRoot);
					if (!CountSubControls
					    (ctlEmbedder, &count))
						for (; count; count--)
							if (!GetIndexedSubControl(ctlEmbedder, count, &ctl))
								EmbedControl
								    (ctl,
								     ctlRoot);
					DisposeControl(ctlEmbedder);
				}
				ZapHandle(hSaveInfo);
				(*hWazooRes)->kindList[i].hSaveInfo = nil;
			}
		}

		DisposeWazoo(hWazooData, hWazooRes);
		SetTopMargin(win, 0);
		win->wazooData = nil;
		win->minSize = minSize;
		MyWindowDidResize(win, nil);
	}
}


/************************************************************************
 * WazooPreUpdate - going to update a window. Handle tab control updates
 ************************************************************************/
void WazooPreUpdate(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	CGrafPtr winPort = GetWindowPort(winWP);
	WazooDataHandle hWazooData;
	Rect rTab;

	if (IsWindowVisible(winWP) && GetWazooData(win, &hWazooData, nil)) {
		//      Draw tab control first so it's in the background.
		//      Need to temporarily resize embedder control so entire
		//      tab control will draw
		ControlHandle hTabCtrl = (*hWazooData)->hTabCtrl;
		RgnHandle clipRgn;

		// draw the top margin control first to clean up old tabs
		Draw1Control(win->topMarginCntl);

		if (GetWindowKind(GetMyWindowWindowPtr(win)) != STAT_WIN) {
			//      if statistics window, only redraw top part of tabs to avoid flicker
			Rect rPort, rCtl;
			GetPortBounds(winPort, &rPort);
			GetControlBounds(hTabCtrl, &rCtl);
			rCtl.bottom = rPort.bottom + 1;
			SetControlBounds(hTabCtrl, &rCtl);
		}
		//      Don't draw in the dontGreyOnMe area
		clipRgn = SavePortClipRegion(winPort);
		RgnMumbleRect(clipRgn, &win->dontGreyOnMe, false);
		RestorePortClipRegion(winPort, clipRgn);
		GetControlBounds(hTabCtrl, &rTab);
		rTab.top = kTabCtlTop;
		SetControlBounds(hTabCtrl, &rTab);
		Draw1Control(hTabCtrl);
		InfiniteClip(winPort);	//      Restore clipping region
		rTab.bottom = GetRLong(WAZOO_TOPMARGIN);
		SetControlBounds(hTabCtrl, &rTab);
		SetTabBackColor(win);
//              SetControlVisibility((*hWazooData)->hTabCtrl,false,false);      //      Don't want tab control to draw again

		//      Don't need to draw tabs anymore on this update event
		ValidWindowRect(winWP, &rTab);

		//      Make sure controls are embedded
		GetEmbedder(hWazooData, -1, true);
	}
}

/************************************************************************
 * SetupTabBackground - setup background color for tab control
 ************************************************************************/
static pascal void SetupTabBackground(ControlHandle control,
				      ControlBackgroundPtr info)
{
	SetTabBackColor(GetWindowMyWindowPtr(GetControlOwner(control)));
}

/************************************************************************
 * IsKindWazooable - is this window kind wazooable?
 ************************************************************************/
static Boolean IsKindWazooable(short thisKind)
{
	short kind;
	for (kind = 0; kind < sizeof(gWazooKinds) / sizeof(WazooableItem);
	     kind++) {
		if (thisKind == gWazooKinds[kind].kind)
			if (gWazooKinds[kind].dependentFeature
			    && !HasFeature(gWazooKinds[kind].
					   dependentFeature))
				continue;
			else
				return true;
	}
	return false;
}

/************************************************************************
 * IsWazooable - is this window wazooable?
 ************************************************************************/
Boolean IsWazooable(WindowPtr winWP)
{
	if (winWP)
		return IsKindWazooable(GetWindowKind(winWP));
	return false;
}

/************************************************************************
 * IsWazoo - is this window a wazoo?
 ************************************************************************/
Boolean IsWazoo(WindowPtr winWP)
{
	if (IsMyWindow(winWP)) {
		MyWindowPtr win = GetWindowMyWindowPtr(winWP);
		return (win->wazooData != 0);
	}
	return (false);
}

/************************************************************************
 * IsLonelyWazoo - is this window in a wazoo, all by itself?
 ************************************************************************/
Boolean IsLonelyWazoo(WindowPtr winWP)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (IsWazoo(winWP))
		if (GetWazooData
		    (GetWindowMyWindowPtr(winWP), &hWazooData, &hWazooRes))
			return ((*hWazooRes)->count == 1);
	return (false);
}

/************************************************************************
 * IsKindWazoo - is this kind in a wazoo?
 ************************************************************************/
Boolean IsKindWazoo(short windowKind)
{
	WazooDataHandle hWazooData;
	short idx;

	hWazooData = FindWazoo(windowKind, &idx);
	return (hWazooData != nil);
}

/************************************************************************
 * ClickWazoo - check for and process clicks in a wazoo
 ************************************************************************/
Boolean ClickWazoo(MyWindowPtr win, EventRecord * event, Point pt,
		   Boolean * dontActivate)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	short tab;

	if (dontActivate)
		*dontActivate = False;
/*
	if (event->modifiers & controlKey && IsWazooable(win) && !PrefIsSet(PREF_CONTROL_DRAG))
	{
		//	Pop up contextual menu for window
		Point	globalPt = pt;
		short	menuItem;
		MenuHandle mh = GetMHandle(WIND_PROP_HIER_MENU);
		
		SetWindowPropMenu(win,true,False,diamondChar,event->modifiers);
		SetWindowPropMenu(win,false,False,diamondChar,event->modifiers);
		LocalToGlobal(&globalPt);
		menuItem = AFPopUpMenuSelect(mh,globalPt.v,globalPt.h,-1);
		DoWindowPropMenu(win,menuItem,event->modifiers);
		return true;
	}
	
*/
	if ((tab = PtInTab(win, pt)) >= 0) {
		WazooDataHandle hWazooData;
		Boolean moved;
		Rect rTab;

		if (!win->isActive) {
			SelectWindow_(winWP);
			ActivateMyWindow(winWP, true);
		}

		GetWazooData(win, &hWazooData, nil);

		rTab = (*hWazooData)->tabRect[tab];
		if (tab != (*hWazooData)->current) {
			SetCurrentTab((*hWazooData)->hTabCtrl, tab);
			SelectWazoo(win, tab);
		}
		moved = MyWaitMouseMoved(event->where, false);
		if (moved) {
			//      Started a drag
			DragWazoo(win, &rTab, dontActivate, tab);
		}

		return true;
	}
	return false;
}

/************************************************************************
 * DidResizeWazoo - wazoo was resized
 ************************************************************************/
void DidResizeWazoo(MyWindowPtr win, Rect * oldCont)
{
#pragma unused(oldCont)

	//      If tab width has changed, redraw the tabs
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		Rect r;

		GetPortBounds(GetMyWindowCGrafPtr(win), &r);
		r.bottom = GetRLong(WAZOO_TOPMARGIN);
		r.top = kTabCtlTop;
		r.left--;
		r.right++;

		//      Don't draw tab control when resizing
		SetControlVisibility((*hWazooData)->hTabCtrl, false,
				     false);
		SizeControl((*hWazooData)->hTabCtrl, RectWi(r), RectHi(r));
		SetControlVisibility((*hWazooData)->hTabCtrl, true, true);

		SetupTabs(hWazooData, hWazooRes, false);
		CheckTabResize(win, hWazooData, hWazooRes,
			       (*hWazooData)->tabDisplayMode);
	}
	CalcWazooMinSize(win);
}

/************************************************************************
 * KillWazoos - get rid of the list of wazoos
 ************************************************************************/
void KillWazoos(void)
{
	WazooDataHandle hWazooData;

	if (hWazooData = gWazooListHead) {
		//      Dispose of current wazoo list. Probably switching settings files
		while (hWazooData) {
			WazooDataHandle hNextWazooData;

			hNextWazooData = (*hWazooData)->next;
			ZapHandle(hWazooData);
			hWazooData = hNextWazooData;
		}
		gWazooListHead = nil;
	}
}

/************************************************************************
 * WazooHelp - display wazoo help balloon
 ************************************************************************/
Boolean WazooHelp(MyWindowPtr win, Point mouse)
{
	Boolean result = false;
	short tab;
	Str255 sHelp;
	Str32 title;

	if ((tab = PtInTab(win, mouse)) >= 0) {
		WazooDataHandle hWazooData;
		WazooResHandle hWazooRes;
		Rect rTab;

		GetWazooData(win, &hWazooData, &hWazooRes);
		rTab = (*hWazooData)->tabRect[tab];
		PCopy(title, (*hWazooRes)->kindList[tab].name);

		ComposeRString(sHelp, WAZOO_TAB_HELP, title, title);
		MyBalloon(&rTab, 100, 0, 0, 0, sHelp);

		result = true;
	}
	return result;
}

/************************************************************************
 * InitWazoos - set up list of wazoos
 ************************************************************************/
void InitWazoos(void)
{
	short saveFile = CurResFile();
	short count, idx;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	ResType resType =
	    IsFreeMode()? kFreeWazooListResType : kWazooListResType;;

	KillWazoos();		// just in case...

	UseResFile(SettingsRefN);
	count = Count1Resources(resType);
	for (idx = 1; idx <= count; idx++) {
		if (hWazooRes =
		    (WazooResHandle) Get1IndResource(resType, idx)) {
			short i;
			WazooResPtr pWazooRes;

			//      Zero out hSaveInfo handle which may have been saved with the resource
			pWazooRes = *hWazooRes;
			for (i = 0; i < pWazooRes->count; i++)
				pWazooRes->kindList[i].hSaveInfo = nil;

			if (hWazooData = NuHandleClear(sizeof(WazooData))) {
				(*hWazooData)->list = hWazooRes;
				LL_Push(gWazooListHead, hWazooData);
			}
		}
	}
	UseResFile(saveFile);
}

/**********************************************************************
 * SetupDefaultWazoos - revert to default wazoos
 **********************************************************************/
void SetupDefaultWazoos(void)
{
	short i;
	WindowPtr win, next;
	ResType resType =
	    IsFreeMode()? kFreeWazooListResType : kWazooListResType;
	Boolean abort = false;

	//      Clear out temp field for each wazoo type
	for (i = 0; i < sizeof(gWazooKinds) / sizeof(WazooableItem); i++)
		gWazooKinds[i].temp = false;

	//      Close every wazoo that is open and remember what was open
	for (win = FrontWindow(); win; win = next) {
		next = GetNextWindow(win);	//      We might close this window
		if (IsWindowVisible(win) && IsWazooable(win)) {
			for (i = 0;
			     i <
			     sizeof(gWazooKinds) / sizeof(WazooableItem);
			     i++)
				if (gWazooKinds[i].kind ==
				    GetWindowKind(win)) {
					if (!CloseMyWindow(win)) {
						// couldn't close window. abort
						abort = true;
						next = nil;
					}
					gWazooKinds[i].temp = true;
					break;
				}
		}
	}

	if (!abort) {
		//      Get rid of all wazoo resources and copy over default ones
		UseResFile(SettingsRefN);
		for (i = Count1Resources(resType); i; i--) {
			Handle resH;

			SetResLoad(false);
			resH = Get1IndResource(resType, i);
			SetResLoad(true);
			if (resH) {
				RemoveResource(resH);
				ZapHandle(resH);
			}
		}
		ResourceCpy(SettingsRefN, AppResFile, resType, kWazooRes1);
		ResourceCpy(SettingsRefN, AppResFile, resType, kWazooRes2);
		InitWazoos();
	}

	//      Reopen wazoo windows that were previously open
	for (i = 0; i < sizeof(gWazooKinds) / sizeof(WazooableItem); i++)
		if (gWazooKinds[i].temp) {
			gWazooKinds[i].temp = false;
			OpenWazoo(gWazooKinds[i].kind);
		}
}

/************************************************************************
 * DisposeWazoo - dispose of wazoo info for window
 ************************************************************************/
static void DisposeWazoo(WazooDataHandle hWazooData,
			 WazooResHandle hWazooRes)
{
	SaveWazooHandle hSaveInfo;
	short current = (*hWazooData)->current;

	LL_Remove(gWazooListHead, hWazooData, (WazooDataHandle));
	RemoveResource((Handle) hWazooRes);
	if (current >= 0)
		if (hSaveInfo = (*hWazooRes)->kindList[current].hSaveInfo)
			ZapHandle(hSaveInfo);
	ZapHandle(hWazooRes);
	ZapHandle(hWazooData);

}


/************************************************************************
 * SetTabBackColor - get tab background color
 ************************************************************************/
void SetTabBackColor(MyWindowPtr win)
{
	WazooDataHandle hWazooData;

	if (GetWazooData(win, &hWazooData, nil)
	    && (*hWazooData)->hTabColorCtrl)
		SetUpControlBackground((*hWazooData)->hTabColorCtrl,
				       RectDepth(&win->contR),
				       ThereIsColor);
}

/************************************************************************
 * FindWazoo - search wazoo lists for the indicated type
 ************************************************************************/
static WazooDataHandle FindWazoo(short windowKind, short *idx)
{
	WazooDataHandle hWazooData = gWazooListHead;

	while (hWazooData) {
		WazooResPtr pWazooRes = *(*hWazooData)->list;
		short i;

		for (i = 0; i < pWazooRes->count; i++) {
			if (pWazooRes->kindList[i].kind == windowKind) {
				if (idx)
					*idx = i;
				return hWazooData;	//      Found it!
			}
		}
		hWazooData = (*hWazooData)->next;
	}
	return nil;		//      Not found
}

/************************************************************************
 * SetWazooMinSize - set min size for this window and update wazoo's min size
 ************************************************************************/
static void SetWazooMinSize(short kind, short h, short v)
{
	WazooDataHandle hWazooData;
	short idx;

	if (hWazooData = FindWazoo(kind, &idx)) {
		//      Make sure minimum size is set up correctly
		Point minSize;
		WazooResHandle hWazooRes;
		MyWindowPtr win;

		hWazooRes = (*hWazooData)->list;
		if (win = (*hWazooData)->win) {
			Point newSize;

			minSize = (*hWazooRes)->kindList[idx].minSize;
			if (h && v) {
				newSize.h = h;
				newSize.v = v;
			} else {
				//      Need to get initial window size from WIND resource if we don't already have it
				short kind;

				if (minSize.h || minSize.v) {
					//      Already have size
					newSize = minSize;
				} else {
					newSize = minSize;
					for (kind = 0;
					     kind <
					     sizeof(gWazooKinds) /
					     sizeof(WazooableItem); kind++)
						if (GetWindowKind
						    (GetMyWindowWindowPtr
						     (win)) ==
						    gWazooKinds[kind].
						    kind) {
							Handle hWIND;

							if (hWIND =
							    GetResource
							    ('WIND',
							     gWazooKinds
							     [kind].
							     windRsrc)) {
								Rect r =
								    **(Rect
								       **)
								    hWIND;
								newSize.h =
								    r.
								    right -
								    r.left;
								newSize.v =
								    r.
								    bottom
								    -
								    r.top;
								break;
							}
						}
				}
			}

			if (minSize.h != newSize.h
			    || minSize.v != newSize.v) {
				(*hWazooRes)->kindList[idx].minSize =
				    newSize;
				ChangedResource((Handle) hWazooRes);
			}
			CalcWazooMinSize(win);
		}
	}
}

/************************************************************************
 * OpenWindow - open a window based on the windowKind
 ************************************************************************/
static void OpenWazoo(short kind)
{

	if (kind == PH_WIN)
		OpenPh(nil);	//      Special case, requires a parameter
	else if (kind == FILT_WIN)
		OpenFiltersWindow();	//      Special case, returns a value
	else {
		short i;
		for (i = 0;
		     i < sizeof(gWazooKinds) / sizeof(WazooableItem);
		     i++) {
			// (jp) 12-16-99 Check to see if the kind is wazooable, just in case we're
			//                                                       attempting to wazoo a window that isn't available in Light
			if (kind == gWazooKinds[i].kind
			    && IsKindWazooable(kind)) {

				//      Open the window
				(*gWazooKinds[i].openFunction) ();
				break;
			}
		}
	}

	SetWazooMinSize(kind, 0, 0);
}

/************************************************************************
 * GetWazooData - get wazoo data
 ************************************************************************/
static Boolean GetWazooData(MyWindowPtr win, WazooDataHandle * hWazooData,
			    WazooResHandle * hWazooRes)
{
	if (win && (*hWazooData = (WazooDataHandle) win->wazooData)) {
		if (hWazooRes)
			*hWazooRes = (**hWazooData)->list;
		return true;
	}
	return false;
}

/************************************************************************
 * SelectWazoo - select the window
 ************************************************************************/
static void SelectWazoo(MyWindowPtr win, short idx)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)
	    && idx != (*hWazooData)->current) {
		if (MustSaveWazoo(win) && GetRLong(AUTOSAVE_INTERVAL))
			//      Auto save this wazoo window before switching out
			DoMenu2(winWP, FILE_MENU, FILE_SAVE_ITEM, 0);

		if (SaveCurrentWazoo(win, hWazooData, hWazooRes, true)) {
			if (!RestoreWazooWindow
			    (win, hWazooData, idx, false)) {
				//      First time in this window. Open it.
				//      Set clip rgn to nil so the window doesn't
				//      get any garbage drawn to it while the window
				//      is being set up. Most of the window initialization
				//      is done with the assumption that the window is invisible.
				SetEmptyClipRgn(GetWindowPort(winWP));
				win->noUpdates = true;
				OpenWazoo((*hWazooRes)->kindList[idx].
					  kind);
				win->noUpdates = false;
				InfiniteClip(GetWindowPort(winWP));	//      Restore clipping region
			}
			InvalContent(win);
			InvalTopMargin(win);
			CheckMinWinSize(win, win->minSize);
		}
	}
}

/************************************************************************
 * PtInTab - see if point is in a tab (returns 0-based index), -1 if not found
 ************************************************************************/
static short PtInTab(MyWindowPtr win, Point pt)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	short tab;

	if (GetWazooData(win, &hWazooData, &hWazooRes))
		for (tab = 0; tab < (*hWazooRes)->count; tab++) {
			Rect rTab = (*hWazooData)->tabRect[tab];
			if (PtInRect(pt, &rTab))
				return tab;
		}

	return -1;
}

/************************************************************************
 * EmbedTheControls - make sure every non-embedder control in the window is embedded within an embedder
 ************************************************************************/
static ControlHandle EmbedTheControls(MyWindowPtr win,
				      ControlHandle ctlEmbedder)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	ControlHandle ctlRoot;
	ControlHandle ctlTabs = nil;
	ControlHandle ctlTemp, ctl;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes))
		ctlTabs = (*hWazooData)->hTabCtrl;

	if (!ctlEmbedder) {
		//      Create an embedder control
		Rect r;
		DECLARE_UPP(SetupTabBackground, ControlUserPaneBackground);

		INIT_UPP(SetupTabBackground, ControlUserPaneBackground);
		SetRect(&r, -REAL_BIG / 2, -REAL_BIG / 2, REAL_BIG / 2,
			REAL_BIG / 2);
		ctlEmbedder =
		    NewControl(winWP, &r, "", true,
			       kControlSupportsEmbedding +
			       kControlWantsActivate +
			       kControlHasSpecialBackground, 0, 0,
			       kControlUserPaneProc, kEmbedderId);
		SetControlData(ctlEmbedder, 0,
			       kControlUserPaneBackgroundProcTag,
			       sizeof(SetupTabBackgroundUPP),
			       (Ptr) & SetupTabBackgroundUPP);
	}

	//      Make sure every non-embedder control in the window is embedded within this embedder
	if (ctlEmbedder) {
		GetRootControl(winWP, &ctlRoot);

		//      Need to inspect every control embedded in the root control.
		//      Unfortunately, CountSubControls and GetIndexedSubControl don't appear
		//      to work correctly with the root control under Appearance Manager 1.0.
		//      We will therefore walk thru the control list checking for the root control
		//      as the super control
		for (ctl = GetControlList(winWP); ctl;
		     ctl = GetNextControl(ctl)) {
			if (!GetSuperControl(ctl, &ctlTemp) &&	//      Get super control
			    ctlTemp == ctlRoot &&	//      Only want controls embedded within root control
			    GetControlReference(ctl) != kEmbedderId &&	//      Ignore embedders
			    ctl != win->topMarginCntl &&	//      Ignore top margin control
			    ctl != ctlTabs)	//      Ignore tabs control
				//      Embed it in the embedder.
				EmbedControl(ctl, ctlEmbedder);
		}
	}

	return ctlEmbedder;
}


/************************************************************************
 * SaveCurrentWazoo - save the current wazoo window data
 ************************************************************************/
static Boolean SaveCurrentWazoo(MyWindowPtr win,
				WazooDataHandle hWazooData,
				WazooResHandle hWazooRes,
				Boolean switching)
{
	SaveWazooHandle hSave;
	short current;
	ControlHandle embedder;

	current = (*hWazooData)->current;
	if (current < 0)
		return true;	//      Nothing to save
	hSave = (*hWazooRes)->kindList[current].hSaveInfo;
	if (!hSave) {
		hSave = NuHandleClear(sizeof(SaveWazooData));
		(*hWazooRes)->kindList[current].hSaveInfo = hSave;
	}
	if (hSave) {
		//      Make sure every non-embedder control in the window is embedded within an embedder
		EmbedTheControls(win, embedder =
				 GetEmbedder(hWazooData, current, true));
		if (switching) {
			//      Switching out, hide controls
			SetControlVisibility(embedder, false, false);
			if (win->wazooSwitch)
				(*win->wazooSwitch) (win, false);
		}
		(*hSave)->winRefCon = GetMyWindowPrivateData(win);
		BMD(((Ptr) win) + kSaveDataOffset, (*hSave)->winData,
		    kSaveDataSize);
	}
	return hSave != nil;
}

/************************************************************************
 * RestoreWazooWindow - restore the wazoo window data
 ************************************************************************/
static Boolean RestoreWazooWindow(MyWindowPtr win,
				  WazooDataHandle hWazooData, short idx,
				  Boolean quietly)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	SaveWazooHandle hSave;
	Str32 title;
	WazooResHandle hWazooRes = (*hWazooData)->list;
	ControlHandle embedder;
	short current;

	if (win->pte)
		//      Deactivate any Pete text
		PeteActivate(win->pte, false, false);
	//      Hide former controls
	current = (*hWazooData)->current;
	if (current >= 0)
		if ((hSave = (*hWazooRes)->kindList[current].hSaveInfo) &&
		    (embedder = GetEmbedder(hWazooData, -1, true))) {
			SetControlVisibility(embedder, false, false);
			if (win->wazooSwitch)
				(*win->wazooSwitch) (win, false);
		}

	if (hSave = (*hWazooRes)->kindList[idx].hSaveInfo) {
		SaveWazooPtr pSave = *hSave;
		Boolean wasActive = win->isActive;

		SetMyWindowPrivateData(win, pSave->winRefCon);
		SetWindowKind(winWP, (*hWazooRes)->kindList[idx].kind);
		SetWinFields(win, pSave->winData);
		(*hWazooData)->current = idx;
		if (embedder = GetEmbedder(hWazooData, idx, true)) {
			//      Make these controls visible
			SetControlVisibility(embedder, true, false);
			if (win->wazooSwitch)
				(*win->wazooSwitch) (win, true);
		}
		if (!quietly) {
			PCopy(title, (*hWazooRes)->kindList[idx].name);
			SetWTitle(winWP, title);
			if (win->pte)
				//      Reactivate any Pete text
				PeteActivate(win->pte, true, true);
			SetEmptyClipRgn(GetWindowPort(winWP));
			MyWindowDidResize(win, nil);
			if (wasActive != win->isActive)
				//      Make sure wazoo is activated/deactivated
				ActivateMyWindow(winWP, wasActive);
			InfiniteClip(GetWindowPort(winWP));	//      Restore clipping region
		}
		return true;
	} else
		return false;
}

/************************************************************************
 * CloseAllButCurrent - close all wazoo windows except one
 ************************************************************************/
static Boolean CloseAllButCurrent(MyWindowPtr win,
				  WazooDataHandle hWazooData,
				  WazooResHandle hWazooRes)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	short i, current;
	Boolean fSelect = false;

	current = (*hWazooData)->current;
	SaveCurrentWazoo(win, hWazooData, hWazooRes, false);
	for (i = 0; i < (*hWazooRes)->count; i++) {
		if (i != current) {
			if (RestoreWazooWindow(win, hWazooData, i, true)) {
				if (MustSaveWazoo(win)) {
					//      These windows will put up a WannaSave dialog.
					//      Need to switch to them
					RestoreWazooWindow(win, hWazooData,
							   i, false);
					UpdateMyWindow(winWP);
					fSelect = true;
				}

				if (!CloseAWazoo
				    (win, hWazooData, hWazooRes, i))
					return false;
			}
		}
	}

	//      Need to go back to the current
	RestoreWazooWindow(win, hWazooData, current, !fSelect);
	if (fSelect)
		UpdateMyWindow(winWP);

	return true;
}

/************************************************************************
 * CloseAWazoo - close a wazoo window
 ************************************************************************/
static Boolean CloseAWazoo(MyWindowPtr win, WazooDataHandle hWazooData,
			   WazooResHandle hWazooRes, short index)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	SaveWazooHandle hSaveInfo;
	PETEHandle pte, nextPTE;
	ControlHandle embedder;

	SetEmptyClipRgn(GetWindowPort(winWP));
	if (win->close && !(*win->close) (win))
		return (false);
	for (pte = win->pteList; pte; pte = nextPTE) {
		nextPTE = PeteNext(pte);
		PeteDispose(win, pte);
	}

	hSaveInfo = (*hWazooRes)->kindList[index].hSaveInfo;
	if (embedder = GetEmbedder(hWazooData, -1, false))
		DisposeControl(embedder);
	if (hSaveInfo) {

		ZapHandle(hSaveInfo);
		(*hWazooRes)->kindList[index].hSaveInfo = nil;
	}
	(*hWazooData)->current = -1;
	InfiniteClip(GetWindowPort(winWP));	//      Restore clipping region
	return true;
}

/************************************************************************
 * MakeDragRegion - make drag region for wazoo tab
 ************************************************************************/
static RgnHandle MakeDragRegion(Rect * rTab, short tab,
				WazooDataHandle hWazooData)
{
	RgnHandle dragRgn;

	if (dragRgn = NewRgn()) {
		GetControlRegion((*hWazooData)->hTabCtrl, tab + 1,
				 dragRgn);
		GlobalizeRgn(dragRgn);
		OutlineRgn(dragRgn, 1);
	}
	return dragRgn;
}

/************************************************************************
 * DragWazoo - drag a wazoo tab to: same wazoo, different wazoo, new wazoo
 ************************************************************************/
static void DragWazoo(MyWindowPtr win, Rect * rTab, Boolean * dontActivate,
		      short tab)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	RgnHandle dragRgn;
	DragReference drag;
	Str255 sTitle;
//      OSErr err;
//      PromiseHFSFlavor promise;

	SetMyCursor(arrowCursor);
	SFWTC = True;

	if (!MyNewDrag(win, &drag)) {
		if (!FinderDragVoodoo(drag))	// pacify the evil gods in finder-land
			if (!
			    (AddDragItemFlavor
			     (drag, 1, kWazooDragType, nil, 0,
			      flavorSenderOnly | flavorNotSaved))) {
				WazooDataHandle hWazooData;
				WazooResHandle hWazooRes;

				if (GetWazooData
				    (win, &hWazooData, &hWazooRes)
				    && SaveCurrentWazoo(win, hWazooData,
							hWazooRes,
							false)) {
					Rect rWind, rPosition;
					Boolean zoomed;
					DECLARE_UPP(DrawDrag, DragDrawing);
					DragOutlineInfo dragOutline;

					INIT_UPP(DrawDrag, DragDrawing);
					gDragData.winSrc = win;
					gDragData.winDest = nil;
					gDragData.tabIdx =
					    (*hWazooData)->current;
					gDragData.tabData =
					    (*hWazooRes)->
					    kindList[gDragData.tabIdx];
					gDragData.tabData.hSaveInfo = nil;	//      Don't want saved wazoo info to transfer to another window

					//      Set up a drawing proc for drawing the drag outline
					dragRgn =
					    MakeDragRegion(rTab, tab,
							   hWazooData);
					gpDragOutline = &dragOutline;
					dragOutline.a5 = SetCurrentA5();

					//      Get outline of window region
					dragOutline.rgnWindow = NewRgn();
					GetStructureRgnBounds(winWP,
							      &rWind);
					GetWTitle(winWP, sTitle);
					if (RestorePosPrefs
					    (sTitle, &rPosition,
					     &zoomed)) {
						rWind.right =
						    rWind.left +
						    (rPosition.right -
						     rPosition.left);
						rWind.bottom =
						    rWind.top +
						    (rPosition.bottom -
						     rPosition.top);
					}
					RectRgn(dragOutline.rgnWindow,
						&rWind);
					OutlineRgn(dragOutline.rgnWindow,
						   1);
					GetMouse(&dragOutline.lastMouse);
					LocalToGlobal(&dragOutline.
						      lastMouse);
					OffsetRgn(dragOutline.rgnWindow, rTab->left - kTabLeftMargin, 0);	//      Normalize region so tab appears in 1st position
					dragOutline.mode = kDragInWazoo;
					if (!HaveOSX())
						// Apple engineering: "Drag drawing proc is not supported on X."
						SetDragDrawingProc(drag,
								   DrawDragUPP,
								   (void *)
								   &dragOutline);

					//      Do drag
					gDontDragHilite = true;
					MyTrackDrag(drag, &MainEvent,
						    dragRgn);
					gDontDragHilite = false;
					DisposeRgn(dragOutline.rgnWindow);

					if (gDragData.winDest !=
					    gDragData.winSrc) {
						short fromIdx =
						    (*hWazooData)->current;

						if (dontActivate)
							*dontActivate =
							    True;

						//      Moved from original window
						if (gDragData.winDest) {
							//      Drag to a different window
							//      Do a soft close in old window
							if (!CloseAWazoo
							    (win,
							     hWazooData,
							     hWazooRes,
							     fromIdx)) {
								//      The window won't close. Leave the tab in the source
								//      and remove it from the destination
								WazooDataHandle
								    hWazooDataDest;
								WazooResHandle
								    hWazooResDest;

								RemoveTab
								    (gDragData.
								     winDest,
								     gDragData.
								     tabIdx);
								GetWazooData
								    (gDragData.
								     winDest,
								     &hWazooDataDest,
								     &hWazooResDest);
								SetupTabs
								    (hWazooDataDest,
								     hWazooResDest,
								     true);
							} else {
								//      Remove from old window
								if ((*hWazooRes)->count < 2) {
									//      No tabs left in window. Get rid of the window
									DisposeWazoo
									    (hWazooData,
									     hWazooRes);
									win->close = nil;	//      Don't call close proc
									win->
									    position
									    =
									    nil;
									win->
									    wazooData
									    =
									    nil;
									CloseMyWindow
									    (winWP);
								} else {
									SelectWazoo(win, fromIdx ? 0 : 1);	//      Select first tab in source window
									RemoveTab
									    (win,
									     fromIdx);
									SetupTabs
									    (hWazooData,
									     hWazooRes,
									     true);
									UpdateMyWindow
									    (winWP);
								}

								//      Select tab in new window
								SelectWazooWithUpdate
								    (gDragData.
								     winDest,
								     gDragData.
								     tabIdx);
							}

							CheckMinWinSize
							    (gDragData.
							     winDest,
							     gDragData.
							     tabData.
							     minSize);
						} else {
							Point mouse;
							Rect rMenuBar;

							//      Drag to nowhere.
							GetDragMouse(drag,
								     &mouse,
								     nil);

							//      Don't move if drag to menu bar
							rMenuBar =
							    (*GetMainDevice
							     ())->gdRect;
							rMenuBar.bottom =
							    rMenuBar.top +
							    GetMBarHeight
							    ();
							if (!PtInRect
							    (mouse,
							     &rMenuBar)) {

								gForceHPos = mouse.h - 24;	//      Position new window
								gForceVPos
								    =
								    mouse.
								    v - 14;
								if ((*hWazooRes)->count > 1) {
									//      Still got at least one left.
									//      Drag out to new window
									//      Need to soft close first
									short
									    kind;

									kind = (*hWazooRes)->kindList[fromIdx].kind;
									if (CloseAWazoo(win, hWazooData, hWazooRes, fromIdx)) {
										SelectWazoo(win, fromIdx ? 0 : 1);	//      Select first tab in source window
										RemoveTab(win, fromIdx);	//      Remove tab from source window
										SetupTabs
										    (hWazooData,
										     hWazooRes,
										     true);
										UpdateMyWindow
										    (winWP);

										//      Create new window
										gForceNewWazoo
										    =
										    true;
										OpenWazoo
										    (kind);
										gForceNewWazoo
										    =
										    false;
									}
								} else {
									//      Drag single tab out of any window. Just move this window
									Rect oldContR = win->contR;
									Rect r;
									long sysVers;

									GetPortBounds
									    (GetWindowPort
									     (winWP),
									     &r);
									r.right = r.right - r.left + gForceHPos;
									r.bottom = r.bottom - r.top + gForceVPos;
									r.left = gForceHPos;
									r.top = gForceVPos;

									if ((!Gestalt(gestaltSystemVersion, &sysVers) && sysVers < 0x0850) ||
									    //      utl_CouldDrag doesn't work correctly with visible windows with OS < 8.5
									    utl_CouldDrag
									    (winWP,
									     &r,
									     4,
									     TitleBarHeight
									     (winWP),
									     LeftRimWidth
									     (winWP)))
										MoveWindow
										    (winWP,
										     r.
										     left,
										     r.
										     top,
										     False);
								}
							}
						}
					}

					gDragData.winSrc = nil;
					DisposeDragRgn(dragRgn);
				}
			}
	}
	if (drag)
		DisposeDrag(drag);
}

/************************************************************************
 * RemoveTab - remove a tab from a window
 ************************************************************************/
static void RemoveTab(MyWindowPtr win, short tabIdx)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		WazooItem *pKindList;
		long len;
		short count;
		Boolean tabDisplayMode = (*hWazooData)->tabDisplayMode;

		pKindList = (*hWazooRes)->kindList;
		count = (*hWazooRes)->count - 1;
		if (len = sizeof(WazooItem) * (count - tabIdx))
			//      Move items after it
			BMD(&pKindList[tabIdx + 1], &pKindList[tabIdx],
			    len);
		SetHandleSize((Handle) hWazooRes,
			      sizeof(WazooRes) +
			      sizeof(WazooItem) * count);
		(*hWazooRes)->count = count;
		ChangedResource((Handle) hWazooRes);
		if (len) {
			if (tabIdx <= (*hWazooData)->current)
				(*hWazooData)->current--;
		}
		CheckTabResize(win, hWazooData, hWazooRes, tabDisplayMode);
		CalcWazooMinSize(win);

		//      Redraw
		Draw1Control((*hWazooData)->hTabCtrl);
	}
}

/************************************************************************
 * DraggingWazoo - drag/receive handler if we are dragging a wazoo
 ************************************************************************/
Boolean DraggingWazoo(MyWindowPtr win, DragTrackingMessage message,
		      DragReference drag, OSErr * err)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Boolean result = false;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	Boolean isWazooWindow;
	Point pt;
	short tab, lastTab;

	*err = dragNotAcceptedErr;
	isWazooWindow = GetWazooData(win, &hWazooData, &hWazooRes);
	if (!gDragData.winSrc) {
		//      Not dragging a wazoo tab
		if (isWazooWindow) {
			//      If we are over a wazoo tab, do a wazoo switch
			SetPort_(GetWindowPort(winWP));
			GetMouse(&pt);
			if ((tab = PtInTab(win, pt)) >= 0
			    && tab != (*hWazooData)->current) {
				unsigned long waitTicks =
				    TickCount() +
				    GetRLong(DRAG_EXPAND_TICKS);

				//      Highlight tab then wait here a while
				SetCurrentTab((*hWazooData)->hTabCtrl,
					      tab);
				lastTab = tab;
				while (WaitMouseUp() && tab == lastTab
				       && waitTicks > TickCount()) {
					GetMouse(&pt);
					lastTab = PtInTab(win, pt);
				}
				if (tab == lastTab) {
					SetupTabs(hWazooData, hWazooRes,
						  true);
					SelectWazoo(win, tab);
					UpdateMyWindow(winWP);
				}
			}
		}
	} else {
		//      Dragging a wazoo tab
		if (isWazooWindow) {
			//      Dragging a wazoo tab within a wazoo window
			Rect rHilite;
			WazooItem *pKindList;
			short count;
			GrafPtr savePort;
			Boolean tabDisplayMode =
			    (*hWazooData)->tabDisplayMode;

			SetPort_(GetWindowPort(winWP));

			switch (message) {
			case kDragTrackingEnterWindow:
				rHilite = win->contR;
				rHilite.top = 0;
				if (!gDontDragHilite)
					DoDragHilite(win, drag, &rHilite,
						     true);

				if (win == gDragData.winSrc) {
					//      Going back into original window
					gDragData.tabIdx =
					    (*hWazooData)->current;
				} else {
					//      Add the tab to the end if not original window
					count = (*hWazooRes)->count + 1;
					SetHandleSize((Handle) hWazooRes,
						      sizeof(WazooRes) +
						      sizeof(WazooItem) *
						      (count));
					if (!MemError()) {
						(*hWazooRes)->count =
						    count;
						gDragData.tabIdx =
						    count - 1;
						(*hWazooRes)->
						    kindList[gDragData.
							     tabIdx] =
						    gDragData.tabData;
						ChangedResource((Handle)
								hWazooRes);
						CalcWazooMinSize(win);
						GetPort(&savePort);
						SetPort_(GetWindowPort
							 (winWP));
						CheckTabResize(win,
							       hWazooData,
							       hWazooRes,
							       tabDisplayMode);
						SetupTabs(hWazooData,
							  hWazooRes, true);
						SetPort(savePort);
					}
				}

				gpDragOutline->mode = kDragInWazoo;
				break;

			case kDragTrackingLeaveWindow:
				if (!gDontDragHilite)
					if (win)
						DoDragHilite(win, drag,
							     nil, false);
					else
						gDontDragHilite = false;	//      Make sure we hilite next time coming into this window

				//      Remove the tab if not the original window
				//      Check for destination window since the drag manager does
				//      a dragTrackingLeaveWindow on the drop also
				if (win && win != gDragData.winSrc
				    && !gDragData.winDest) {
					RemoveTab(win, gDragData.tabIdx);
					SetupTabs(hWazooData, hWazooRes,
						  true);
				}
				gpDragOutline->mode = kDragOutsideWazoo;
				break;

			case kDragTrackingInWindow:
				GetDragMouse(drag, &pt, nil);
				GlobalToLocal(&pt);
				if ((tab = PtInTab(win, pt)) >= 0) {
					if (tab != gDragData.tabIdx) {
						//      Make sure that we are far enough into the other tab that after
						//      we switch, we won't immediately switch back
						Rect rFrom, rTo;
						short wdTo;

						rFrom =
						    (*hWazooData)->
						    tabRect[gDragData.
							    tabIdx];
						rTo =
						    (*hWazooData)->
						    tabRect[tab];
						wdTo =
						    rTo.right - rTo.left;

						if (gDragData.tabIdx < tab)
							rFrom.right =
							    rFrom.left +
							    wdTo;
						else
							rFrom.left =
							    rFrom.right -
							    wdTo;

						if (!PtInRect(pt, &rFrom)) {

							//      Reposition item in tabs         
							WazooItem
							    tempWazooItem;

							pKindList =
							    (*hWazooRes)->
							    kindList;
							tempWazooItem =
							    pKindList
							    [gDragData.
							     tabIdx];
							pKindList
							    [gDragData.
							     tabIdx] =
							    pKindList[tab];
							pKindList[tab] =
							    tempWazooItem;
							if (gDragData.
							    winSrc == win)
								(*hWazooData)->current = tab;
							else {
								//      Make sure current tab stays the same
								if (tab ==
								    (*hWazooData)->
								    current)
									(*hWazooData)->current = gDragData.tabIdx;
							}
							SetupTabs
							    (hWazooData,
							     hWazooRes,
							     true);
							ChangedResource((Handle) hWazooRes);
							gDragData.tabIdx =
							    tab;
						}
					}
				}
				break;

			case 0xfff:
				//      Drop
				gDragData.winDest = win;
				break;
			}
		}
		result = true;
		*err = noErr;
	}

	return result;
}

/************************************************************************
 * DoDragHilite - show/hide drag hilite
 ************************************************************************/
 //     Need to change the background color of the window to match the
 //     color that is displayed
static void DoDragHilite(MyWindowPtr win, DragReference drag,
			 Rect * rHilite, Boolean fShow)
{
	RGBColor saveColor;

	GetBackColor(&saveColor);
	SetTabBackColor(win);
	if (fShow)
		ShowDragRectHilite(drag, rHilite, true);
	else
		HideDragHilite(drag);
	RGBBackColor(&saveColor);
}

/************************************************************************
 * CheckTabResize - see if the tabs need to be redrawn because of size change
 ************************************************************************/
static void CheckTabResize(MyWindowPtr win, WazooDataHandle hWazooData,
			   WazooResHandle hWazooRes, short tabDisplayMode)
{
	if (tabDisplayMode != (*hWazooData)->tabDisplayMode) {
		//      Tab drawing mode has changed. Redraw.
		Draw1Control((*hWazooData)->hTabCtrl);
	}
}

/************************************************************************
 * SetWinFields - change or initialize window fields
 ************************************************************************/
static void SetWinFields(MyWindowPtr win, Ptr winData)
{
	struct MyWindowStruct oldWin;

	//      Save fields
	oldWin = *win;

	if (winData)
		BMD(winData, ((Ptr) win) + kSaveDataOffset, kSaveDataSize);
	else
		//      Init everything to zero
		WriteZero(((Ptr) win) + kSaveDataOffset, kSaveDataSize);

	//Restore a few fields
	win->theWindow = oldWin.theWindow;
	win->privateData = oldWin.privateData;
	win->dialogRefcon = oldWin.dialogRefcon;
	win->windowType = oldWin.windowType;
	win->wazooData = oldWin.wazooData;
	win->noUpdates = oldWin.noUpdates;
	win->topMarginCntl = oldWin.topMarginCntl;
	win->topMargin = oldWin.topMargin;
	win->windex = oldWin.windex;
	win->uselessHi = oldWin.uselessHi;
	win->uselessWi = oldWin.uselessWi;
	win->titleBarHi = oldWin.titleBarHi;
	win->leftRimWi = oldWin.leftRimWi;
	win->minSize = oldWin.minSize;
//      win->isActive = oldWin.isActive;
}

/**********************************************************************
 * DisposeDragRgn - dispose of drag region
 **********************************************************************/
static void DisposeDragRgn(RgnHandle rgn)
{
	if (rgn)
		DisposeRgn(rgn);
#if 0
	if (gworld)
		DisposeGWorld(gworld);
	if (hDrawRgn)
		DisposeRgn(hDrawRgn);
#endif
}

/**********************************************************************
 * DrawDrag - drag drag region proc
 **********************************************************************/
static pascal OSErr DrawDrag(DragRegionMessage message,
			     RgnHandle showRegion, Point showOrigin,
			     RgnHandle hideRegion, Point hideOrigin,
			     void *dragDrawingRefCon, DragReference drag)
{
#pragma unused (showOrigin, hideOrigin)
	Rect rClip;
	DragOutlineInfo *pInfo;
	RgnHandle paintRgn, tempRgn;
	OSErr err = noErr;
	Pattern gray;

	pInfo = dragDrawingRefCon;
	tempRgn = pInfo->tempRgn;

	switch (message) {
	case kDragRegionBegin:
		pInfo->tempRgn = NewRgn();
		if (pInfo->saveClip = NewRgn())
			GetClip(pInfo->saveClip);	//      Save wmgr clip region
		pInfo->lastDrawMode = kDragInWazoo;
		break;

	case kDragRegionEnd:
		DisposeRgn(tempRgn);
		if (pInfo->saveClip) {
			SetClip(pInfo->saveClip);
			DisposeRgn(pInfo->saveClip);
		}
		break;

	case kDragRegionDraw:
	case kDragRegionHide:
		if (!tempRgn)
			break;
		SetRect(&rClip, -REAL_BIG, -REAL_BIG, REAL_BIG, REAL_BIG);
		ClipRect(&rClip);
		PenPat(GetQDGlobalsDarkGray(&gray));	//      Need our gray pattern for XOR'ing
		PenMode(notPatXor);
		paintRgn = nil;
		if (message == kDragRegionDraw) {
			if (pInfo->lastDrawMode == kDragOutsideWazoo) {
				//      Hide window region
				CopyRgn(pInfo->rgnWindow, tempRgn);
				hideRegion = tempRgn;
			}
			if (pInfo->mode == kDragOutsideWazoo) {
				//      Show window region
				Point mouse;

				GetDragMouse(drag, &mouse, nil);
				OffsetRgn(pInfo->rgnWindow,
					  mouse.h - pInfo->lastMouse.h,
					  mouse.v - pInfo->lastMouse.v);
				showRegion = pInfo->rgnWindow;
				pInfo->lastMouse = mouse;
			}
			if (pInfo->lastDrawMode == kDragHid)
				paintRgn = showRegion;	//      Nothing to hide
			else {
				XorRgn(showRegion, hideRegion, tempRgn);
				paintRgn = tempRgn;
			}
			pInfo->lastDrawMode = pInfo->mode;
		} else {
			//      Hide region
			if (pInfo->lastDrawMode == kDragInWazoo) {
				paintRgn = hideRegion;
			} else if (pInfo->lastDrawMode ==
				   kDragOutsideWazoo) {
				paintRgn = pInfo->rgnWindow;
				pInfo->lastDrawMode = kDragHid;
			}
		}
		if (paintRgn)
			PaintRgn(paintRgn);
		PenNormal();
		break;

	}
	return err;
}

/************************************************************************
 * SafeWazooControl - make sure this control will be properly hidden in a wazoo
 ************************************************************************/
void SafeWazooControl(MyWindowPtr win, ControlHandle ctl, short windowKind)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWindowKind(GetMyWindowWindowPtr(win)) == windowKind)
		return;		//      This window is the current wazoo window. It's already safe.

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		short idx;

		FindWazoo(windowKind, &idx);
		//      Embed the control in the embedder control
		EmbedControl(ctl, GetEmbedder(hWazooData, idx, true));
	}
}


/************************************************************************
 * DirtyWazoo - make this wazooable window dirty
 ************************************************************************/
void DirtyWazoo(MyWindowPtr win, short windowKind)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	struct MyWindowStruct tempWin;

	if (GetWindowKind(GetMyWindowWindowPtr(win)) == windowKind)
		win->isDirty = true;	//      This window is the current wazoo window. It's easy
	else if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		//      Need to change isDirty flag in saved wazoo data
		short idx;
		SaveWazooHandle hSave;

		FindWazoo(windowKind, &idx);
		if (hSave = (*hWazooRes)->kindList[idx].hSaveInfo) {
			//      get saved window data
			BMD((*hSave)->winData,
			    ((Ptr) & tempWin) + kSaveDataOffset,
			    kSaveDataSize);
			tempWin.isDirty = true;
			//      put save window data back
			BMD(((Ptr) & tempWin) + kSaveDataOffset,
			    (*hSave)->winData, kSaveDataSize);
		}
	}
}


/************************************************************************
 * CalcWazooMinSize - set wazoo minimum size
 ************************************************************************/
static void CalcWazooMinSize(MyWindowPtr win)
{
	short count;
	Point minSize;
	WazooItem *pItem;
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;

	if (GetWazooData(win, &hWazooData, &hWazooRes)) {
		minSize.h = minSize.v = 0;
		pItem = (*hWazooRes)->kindList;
		for (count = (*hWazooRes)->count; count--;) {
			//      Get maximum minimum size
			if (minSize.h < pItem->minSize.h)
				minSize.h = pItem->minSize.h;
			if (minSize.v <
			    pItem->minSize.v + GetRLong(WAZOO_TOPMARGIN))
				minSize.v =
				    pItem->minSize.v +
				    GetRLong(WAZOO_TOPMARGIN);
			pItem++;
		}
		win->minSize = minSize;
	}
}

/************************************************************************
 * SetWinMinSize - set window minimum size
 ************************************************************************/
void SetWinMinSize(MyWindowPtr win, short h, short v)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Boolean initialSize = h <= 0 || v <= 0;

	if (IsWazoo(winWP)) {
		if (initialSize)
			h = v = 0;

		//      Set min size for this window and update wazoo's min size
		SetWazooMinSize(GetWindowKind(winWP), h, v);
	} else {
		//      Not a wazoo
		if (initialSize) {
			//      Use current window size
			Rect r;
			GetPortBounds(GetWindowPort(winWP), &r);
			h = r.right - r.left;
			v = r.bottom - r.top;
		}
		win->minSize.h = h;
		win->minSize.v = v;
	}
}

/************************************************************************
 * CheckMinWinSize - make sure the window isn't too small
 ************************************************************************/
static void CheckMinWinSize(MyWindowPtr win, Point minSize)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Rect rWinSize;
	Point winSize, origSize;

	GetPortBounds(GetWindowPort(winWP), &rWinSize);
	winSize.h = rWinSize.right - rWinSize.left;
	winSize.v = rWinSize.bottom - rWinSize.top;
	origSize = winSize;
	if (minSize.h > winSize.h)
		winSize.h = minSize.h;
	if (minSize.v > winSize.v)
		winSize.v = minSize.v;
	if (winSize.h != origSize.h || winSize.v != origSize.v) {
		Rect oldContR;

		oldContR = win->contR;
		SizeWindow(winWP, winSize.h, winSize.v, TRUE);
		MyWindowDidResize(win, &oldContR);
		UpdateMyWindow(winWP);
		win->saveSize = True;
	}
}

/************************************************************************
 * MustSaveWazoo - does this window need to be saved?
 ************************************************************************/
static Boolean MustSaveWazoo(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	return ((win->isDirty && GetWindowKind(winWP) == FILT_WIN)
		|| (ABDirty(win) && GetWindowKind(winWP) == ALIAS_WIN));
}


/************************************************************************
 * IsWazooEmbedderCntl - is this control a wazoo embedder?
 ************************************************************************/
Boolean IsWazooEmbedderCntl(ControlHandle cntl)
{
	return GetControlReference(cntl) == kEmbedderId;
}

/************************************************************************
 * IsWazooTabCntl - is this control the tab control?
 ************************************************************************/
Boolean IsWazooTabCntl(ControlHandle cntl)
{
	return GetControlReference(cntl) == kTabsId;
}

/************************************************************************
 * SetCurrentTab - set current tab in control
 ************************************************************************/
static void SetCurrentTab(ControlHandle ctl, short tab)
{
	if (GetControlValue(ctl) != tab + 1) {
		RgnHandle rgn = NewRgn();

		if (rgn) {
			Rect r;
			short saveBottom;

			GetClip(rgn);
			GetControlBounds(ctl, &r);
			ClipRect(&r);
			saveBottom = r.bottom;
			r.bottom = 1000;	//      Avoid drawing line at bottom of tab
			SetControlBounds(ctl, &r);
			SetControlValue(ctl, tab + 1);
			r.bottom = saveBottom;
			SetControlBounds(ctl, &r);
			SetClip(rgn);
			DisposeRgn(rgn);
		}
	}
}

/************************************************************************
 * SetupTabs - setup tab icons and names
 ************************************************************************/
static void SetupTabs(WazooDataHandle hWazooData, WazooResHandle hWazooRes,
		      Boolean drawIt)
{
	ControlHandle ctl = (*hWazooData)->hTabCtrl;
	ControlTabInfoRec tabInfo;
	short tabIdx;
	ControlFontStyleRec fontStyle;
	Rect rCtl;
	CGrafPtr port = GetMyWindowCGrafPtr((*hWazooData)->win);
	RgnHandle clipRgn = SavePortClipRegion(port);
	Rect rPort;

	GetControlBounds(ctl, &rCtl);
	SetPort(port);
	ClipRect(&rCtl);	//      Don't want to redraw the whole window
//      SetControlMaximum(ctl,(*hWazooRes)->count>1?(*hWazooRes)->count:2);
	SetControlMaximum(ctl, (*hWazooRes)->count);
	RestorePortClipRegion(port, clipRgn);
	tabInfo.version = 0;

	fontStyle.flags =
	    kControlUseFontMask + kControlUseSizeMask +
	    kControlUseFaceMask;
	fontStyle.font = SmallSysFontID();
	fontStyle.size = SmallSysFontSize();
	fontStyle.style = bold;
	SetControlData(ctl, 0, kControlFontStyleTag, sizeof(fontStyle),
		       (Ptr) & fontStyle);

	//      Put names on tabs
	for (tabIdx = 0; tabIdx < (*hWazooRes)->count; tabIdx++) {
		PCopy(tabInfo.name, (*hWazooRes)->kindList[tabIdx].name);
		tabInfo.iconSuiteID = Names2Icon(tabInfo.name, "\p");
		SetControlData(ctl, tabIdx + 1, kControlTabInfoTag,
			       sizeof(ControlTabInfoRec), (Ptr) & tabInfo);
	}


	//      Determine which tab mode fits in the window
	CalcTabRects(hWazooData, hWazooRes);

	(*hWazooData)->tabDisplayMode = kIconsAndNames;
	GetPortBounds(port, &rPort);

	//      Too wide?
	if ((*hWazooData)->tabRect[(*hWazooRes)->count - 1].right >
	    rPort.right) {
		//      Try condensed style
		fontStyle.style = condense;
		SetControlData(ctl, 0, kControlFontStyleTag,
			       sizeof(fontStyle), (Ptr) & fontStyle);
		(*hWazooData)->tabDisplayMode = kCondensedNames;

		CalcTabRects(hWazooData, hWazooRes);

		//      Still too wide?
		if ((*hWazooData)->tabRect[(*hWazooRes)->count - 1].right >
		    rPort.right) {
			//      Take off names
			for (tabIdx = 0; tabIdx < (*hWazooRes)->count;
			     tabIdx++) {
				PCopy(tabInfo.name,
				      (*hWazooRes)->kindList[tabIdx].name);
				tabInfo.iconSuiteID =
				    Names2Icon(tabInfo.name, "\p");
				*tabInfo.name = 0;
				SetControlData(ctl, tabIdx + 1,
					       kControlTabInfoTag,
					       sizeof(ControlTabInfoRec),
					       (Ptr) & tabInfo);
			}
			(*hWazooData)->tabDisplayMode = kIconsOnly;

			CalcTabRects(hWazooData, hWazooRes);
		}
	}

	SetCurrentTab(ctl, (*hWazooData)->current);

	if (HaveTheDiseaseCalledOSX()) {
		// bugs in the Jaguar control manager, if you ask me...
		(*hWazooData)->win->topMargin += 20;
		InvalTopMargin((*hWazooData)->win);
		(*hWazooData)->win->topMargin -= 20;
	} else if (drawIt) {
		Draw1Control((*hWazooData)->win->topMarginCntl);
		Draw1Control((*hWazooData)->hTabCtrl);
	} else {
		Rect r;
		GetControlBounds((*hWazooData)->hTabCtrl, &r);
		InvalWindowRect(GetMyWindowWindowPtr((*hWazooData)->win),
				&r);
	}
}

/************************************************************************
 * CalcTabRects - get bounds of each tab
 ************************************************************************/
static void CalcTabRects(WazooDataHandle hWazooData,
			 WazooResHandle hWazooRes)
{
	RgnHandle rgn;
	short tab;
	Rect rTab;

	rgn = NewRgn();
	for (tab = 0; tab < (*hWazooRes)->count; tab++) {
		GetControlRegion((*hWazooData)->hTabCtrl, tab + 1, rgn);
		GetRegionBounds(rgn, &rTab);
		(*hWazooData)->tabRect[tab] = rTab;
	}
	DisposeRgn(rgn);
}

/************************************************************************
 * GetEmbedder - get embedder control for a wazoo
 ************************************************************************/
static ControlHandle GetEmbedder(WazooDataHandle hWazooData, short idx,
				 Boolean create)
{
	ControlHandle ctlEmbedder = nil;
	WazooResHandle hWazooRes;
	SaveWazooHandle hSaveInfo;

	if (idx < 0)
		idx = (*hWazooData)->current;
	hWazooRes = (*hWazooData)->list;
	if (hSaveInfo = (*hWazooRes)->kindList[idx].hSaveInfo)
		ctlEmbedder = (*hSaveInfo)->ctlEmbedder;
	if (!ctlEmbedder && create) {
		ctlEmbedder = EmbedTheControls((*hWazooData)->win, nil);
		if (!hSaveInfo) {
			hSaveInfo = NuHandleClear(sizeof(SaveWazooData));
			(*hWazooRes)->kindList[idx].hSaveInfo = hSaveInfo;
		}
		if (hSaveInfo)
			(*hSaveInfo)->ctlEmbedder = ctlEmbedder;
	}
	return ctlEmbedder;
}


/************************************************************************
 * EmbedInWazoo - embed control in wazoo embedder.
 *		Need to do this for OS X, otherwise may not be able to click
 *		on control
 ************************************************************************/
void EmbedInWazoo(ControlRef cntl, WindowPtr winWP)
{
	WazooDataHandle hWazooData;
	WazooResHandle hWazooRes;
	SaveWazooHandle hSaveInfo;
	ControlHandle ctlEmbedder;
	MyWindowPtr win;

	if (win = GetWindowMyWindowPtr(winWP))
		if (GetWazooData(win, &hWazooData, &hWazooRes))
			if (hSaveInfo =
			    (*hWazooRes)->kindList[(*hWazooData)->current].
			    hSaveInfo)
				if (ctlEmbedder =
				    (*hSaveInfo)->ctlEmbedder)
					EmbedControl(cntl, ctlEmbedder);
}
