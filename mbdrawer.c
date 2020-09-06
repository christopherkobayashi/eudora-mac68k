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

#include "mbdrawer.h"
#include "listview.h"

#define FILE_NUM 143
/* Copyright (c) 2002-2003 by QUALCOMM Incorporated */


#if TARGET_RT_MAC_CFM
//      constants
enum {
	kEventWindowDrawerOpening = 220,
	kEventWindowDrawerOpened = 221,
	kEventWindowDrawerClosing = 222,
	kEventWindowDrawerClosed = 223
};

// These items aren't in the carbon headers yet
typedef UInt32 WindowDrawerState;
enum { kWindowDrawerOpening = 1, kWindowDrawerOpen =
	    2, kWindowDrawerClosing = 3, kWindowDrawerClosed = 4 };
WindowDrawerState GetDrawerState(WindowRef inDrawerWindow);
#endif


//      structs
typedef struct {
	ViewListPtr list;	// mailboxes list
	ExpandInfo expandList;
	MyWindowPtr parentWin;
	TOCHandle tocH;
	ControlRef listCtl;
	Boolean hasFocus;
} MBDrawerVars, **MBDrawerHandle;

//      functions
static MBDrawerHandle GetMBDrawerInfo(MyWindowPtr win);
static void MBDrawerClick(MyWindowPtr win, EventRecord * event);
static void MBDrawerDidResize(MyWindowPtr win, Rect * oldContR);
static void MBDrawerGetListRect(WindowRef win, Rect * r);
static Boolean MBDrawerKey(MyWindowPtr win, EventRecord * event);
static long MBDrawerLVCallBack(ViewListPtr pView,
			       VLCallbackMessage message, long data);
static void MBDrawerUpdate(MyWindowPtr win);
static OSErr MBDrawerDragHandler(MyWindowPtr win,
				 DragTrackingMessage which,
				 DragReference drag);
static OSStatus ControlEvents(EventHandlerCallRef inHandlerCallRef,
			      EventRef theEvent, void *userData);
static OSStatus ListControlEvents(EventHandlerCallRef nextHandler,
				  EventRef theEvent, void *userData);
static void OpenMyDrawer(MyWindowPtr winDrawer);
static void SetDrawerIcon(MyWindowPtr winParent, Boolean open);
void MBDrawerSetState(MyWindowPtr winParent, Boolean open);

//      globals

/************************************************************************
 * MBDrawerOpen - open a mailbox drawer
 ************************************************************************/
MyWindowPtr MBDrawerOpen(MyWindowPtr winParent)
{
	TOCHandle tocH = (TOCHandle) GetMyWindowPrivateData(winParent);
	WindowRef winWP, winParentWP;
	MyWindowPtr win;
	OSErr err;
	CGrafPtr winPort;
	Rect r;
	GrafPtr oldPort;
	MBDrawerVars vars;
	Handle hMBDrawerInfo = nil;
	short width = GetPrefLong(PREF_DRAWER_WIDTH);
	EventTypeSpec ctlEventList[] = {
		{ kEventClassControl, kEventControlHitTest }
	};
	EventTypeSpec listCtlEventList[] = {
		{ kEventClassControl, kEventControlDraw }
	};
	DECLARE_UPP(ControlEvents, EventHandler);
	DECLARE_UPP(ListControlEvents, EventHandler);

	INIT_UPP(ControlEvents, EventHandler);
	INIT_UPP(ListControlEvents, EventHandler);


	if (!MBDrawerSupported(winParent))
		// Drawers aren't supported
		return NULL;

	UseFeature(featureMBDrawer);

	Zero(vars);
	vars.parentWin = winParent;
	vars.tocH = tocH;
	win =
	    GetNewMyWindowWithClass(0, nil, nil, BehindModal, False, False,
				    DRAWER_WIN, kDrawerWindowClass);
	if (!win) {
		err = MemError();
		goto fail;
	}

	winWP = GetMyWindowWindowPtr(win);
	winParentWP = GetMyWindowWindowPtr(winParent);
	SetDrawerParent(winWP, winParentWP);

	SetThemeWindowBackground(winWP, kThemeBrushDrawerBackground,
				 false);

	ConfigFontSetup(win);
	win->hPitch = FontWidth;
	win->vPitch = FontLead + FontDescent;

	// Add mailboxes
	vars.list = NuPtr(sizeof(ViewList));
	vars.expandList.resID = kMBDrawerExpandID;
	if (err = PtrToHand(&vars, &hMBDrawerInfo, sizeof(vars)))
		goto fail;
	SetMyWindowPrivateData(win, (long) hMBDrawerInfo);

	// Set size
	if (!width)
		width = kDefaultMBDrawerWidth;
	GetWindowBounds(winParentWP, kWindowContentRgn, &r);
	SizeWindow(winWP, width, r.bottom - r.top, false);

	if (vars.list) {
		ControlRef vScroll;

		winPort = GetMyWindowCGrafPtr(win);
		GetPort(&oldPort);
		SetPort_(winPort);
		SetRect(&r, 0, 0, 1, 1);
		LVNew(vars.list, win, &r, 0, MBDrawerLVCallBack, 0);
		SetListFlags(vars.list->hList, lOnlyOne);	// Don't support multiple selection
		SetPort_(oldPort);
		vScroll = (*vars.list->hList)->vScroll;
		InstallControlEventHandler(vScroll, ControlEventsUPP,
					   GetEventTypeCount(ctlEventList),
					   ctlEventList, vScroll, NULL);
		if (vars.listCtl =
		    NewControl(winWP, &r, "\p", true,
			       kControlSupportsEmbedding, 0, 0,
			       kControlUserPaneProc, NULL))
			InstallControlEventHandler(vars.listCtl,
						   ListControlEventsUPP,
						   GetEventTypeCount
						   (listCtlEventList),
						   listCtlEventList, win,
						   NULL);
	}

	win->close = MBDrawerClose;
	win->update = MBDrawerUpdate;
	win->click = MBDrawerClick;
	win->bgClick = MBDrawerClick;
	win->didResize = MBDrawerDidResize;
	win->key = MBDrawerKey;
	win->drag = MBDrawerDragHandler;

/*
	win->position = MBDrawerPosition;
	win->help = MBDrawerHelp;
	win->menu = MBDrawerMenuSelect;
	win->app1 = MBDrawerApp1;
	win->button = MBDrawerButton;
	win->scroll = MBDrawerScroll;
	win->selection = MBDrawerHasSelection;
	win->cursor = MBDrawerCursor;
	win->idle = MBDrawerIdle;
	win->dontControl = true;
	win->zoomSize = MaxSizeZoom;
	win->find = MBDrawerFind;
	win->activate = MBDrawerActivate;
*/
	win->isActive = true;
	win->dontActivate = true;

	if (tocH)
		(*tocH)->drawerWin = win;

	// Open drawer
	MBDrawerSetState(winParent, true);
	LVActivate(vars.list, true);
	MBDrawerDidResize(win, nil);
	MBDrawerUpdate(win);

      fail:
	return win;
}


/************************************************************************
 * MBDrawerToggle - toggle mailbox widow drawer
 ************************************************************************/
void MBDrawerToggle(MyWindowPtr winParent)
{
	TOCHandle tocH;

	if (!winParent)
		return;

	tocH = (TOCHandle) GetMyWindowPrivateData(winParent);

	MBDrawerSetState(winParent, !(*tocH)->drawer);
}

/************************************************************************
 * MBDrawerSetState - set state of drawer (open or closed)
 ************************************************************************/
void MBDrawerSetState(MyWindowPtr winParent, Boolean open)
{
	TOCHandle tocH;
	WindowPtr drawerWP = NULL;

	if (!winParent)
		return;

	tocH = (TOCHandle) GetMyWindowPrivateData(winParent);

	if ((*tocH)->drawerWin)
		drawerWP = GetMyWindowWindowPtr((*tocH)->drawerWin);

	if (open) {
		// Open drawer
		if (drawerWP) {
			// Already have drawer. Toggle it open
			OpenMyDrawer((*tocH)->drawerWin);
			MBDrawerUpdate((*tocH)->drawerWin);
		} else
			// Create drawer
			MBDrawerOpen(winParent);

		if ((*tocH)->drawerWin)
			(*tocH)->drawer = true;
	} else {
		// Close drawer
		if (drawerWP)
			CloseDrawer(drawerWP, true);
		(*tocH)->drawer = false;
		SetDrawerIcon(winParent, false);
	}
	TOCSetDirty(tocH, true);
}

/**********************************************************************
 * SetDrawerIcon - set state of drawer button icon
 **********************************************************************/
static void SetDrawerIcon(MyWindowPtr winParent, Boolean open)
{
	ControlRef cntl = FindControlByRefCon(winParent, kDrawerSwitch);

	if (cntl)
		SetControlValue(cntl, open ? 1 : 0);
}

/**********************************************************************
 * GetSearchInfo - get search info from window
 **********************************************************************/
static MBDrawerHandle GetMBDrawerInfo(MyWindowPtr win)
{
	return (MBDrawerHandle) GetMyWindowPrivateData(win);
}

/**********************************************************************
 * MBDrawerClose - close this drawer window. Called only if parent is closing
 **********************************************************************/
Boolean MBDrawerClose(MyWindowPtr win)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	SaveExpandedFolderList(&(*dh)->expandList);

	if ((*dh)->list) {
		LVDispose((*dh)->list);
		ZapPtr((*dh)->list);
	}

	ASSERT((*dh)->tocH);
	if ((*dh)->tocH)
		(*(*dh)->tocH)->drawerWin = nil;

	ZapHandle(dh);

	return true;
}

/**********************************************************************
 * MBDrawerUpdate - update drawer window
 **********************************************************************/
static void MBDrawerUpdate(MyWindowPtr win)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	SetPort(GetMyWindowCGrafPtr(win));
	SetOrigin(0, 0);
	LVDraw((*dh)->list, nil, true, false);
}

/**********************************************************************
 * MBDrawerClick - handle click in drawer window
 **********************************************************************/
static void MBDrawerClick(MyWindowPtr win, EventRecord * event)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	if (dh) {
		// Make sure mailbox window is active
		if (!(*dh)->parentWin->isActive)
			SelectWindow_(GetMyWindowWindowPtr
				      ((*dh)->parentWin));

		// Make sure focus is set
		MBDrawerSetFocus(win, true);
		BoxListFocus((*dh)->tocH, false);

		LVClick((*dh)->list, event);
	}
}

/************************************************************************
 * MBDrawerLVCallBack - callback function for List View
 ************************************************************************/
static long MBDrawerLVCallBack(ViewListPtr pView,
			       VLCallbackMessage message, long data)
{
	VLNodeInfo *pInfo;
	OSErr err = noErr;
	MBDrawerHandle dh = GetMBDrawerInfo(pView->wPtr);
	VLNodeInfo info;
	FSSpec spec;
	DEC_STATE(dh);

	switch (message) {
	case kLVAddNodeItems:
	case kLVExpandCollapseItem:
		L_STATE(dh);
		if (message == kLVAddNodeItems)
			AddMailboxListItems(pView, data,
					    &(*dh)->expandList);
		else
			SaveExpandStatus((VLNodeInfo *) data,
					 &(*dh)->expandList);
		U_STATE(dh);
		break;

	case kLVGetParentID:
		err = MailboxesLVCallBack(pView, message, data);
		break;

	case kLVOpenItem:
		//MBListOpen((*dh)->list);
		break;

	case kLVQueryItem:
		pInfo = (VLNodeInfo *) data;
		switch (pInfo->query) {
		case kQueryDrop:
			return !pInfo->isParent;
		case kQuerySelect:
		case kQueryDragExpand:
			return true;
		case kQueryDrag:
		case kQueryRename:
		case kQueryDropParent:
			return false;
		default:
			err = MailboxesLVCallBack(pView, message, data);
		}
		break;

	case kLVGetFlags:
		return kfSupportsSelectCallbacks + kfSingleSelection;

	case kLVSelectItem:
		if (LVGetItem(pView, 1, &info, true)) {
			if (!info.isParent
			    || info.iconID == IMAP_MAILBOX_FILE_ICON
			    || (info.iconID == kTrashIconResource
				&& IsIMAPBox(&info))) {
				// Switch to this mailbox
				MyWindowPtr boxWin = (*dh)->parentWin;
				MyWindowPtr hideWin = NULL;
				TOCHandle oldToc, newToc;
				ControlRef cntl;
				short i;
				Rect oldContR;
				PETEHandle pteTemp;
				Boolean saveNoUpdates;
				Boolean oldHasTab, newHasTab;

				MakeMBSpec(&info, &spec);
				// if this is an IMAP folder we're going to open, adjust the spec so it points to the mailbox inside
				if (IsIMAPCacheFolder(&spec))
					spec.parID = SpecDirId(&spec);

				oldToc =
				    (TOCHandle)
				    GetMyWindowPrivateData(boxWin);
				if (newToc = FindTOC(&spec)) {
					if (newToc == oldToc)
						return noErr;	// Selected the one we already have!                                            
					hideWin = (*newToc)->win;
					HideWindow_(GetMyWindowWindowPtr
						    (hideWin));
				} else {
					// Need to open this mailbox without showing
					if (GetMailbox(&spec, false))
						return noErr;	// value is going to be ignored here - bail
					newToc = FindTOC(&spec);
					if (!newToc)
						return noErr;	// value is going to be ignored here - bail
					hideWin = (*newToc)->win;
				}

				if ((*oldToc)->fileView)
					// Fileview needs to be turned off before swapping. It messes
					// with a lot of controls.                              
					SetFileView(boxWin, oldToc, false);

				// Let's not do any drawing until we've completely transitioned the window
				saveNoUpdates = boxWin->noUpdates;
				boxWin->noUpdates = true;
				SetEmptyClipRgn(GetMyWindowCGrafPtr
						(boxWin));

				// Setup swapped window data for both windows
				InitMailboxWin(boxWin, newToc, true);
				InitMailboxWin(hideWin, oldToc, false);

				// Swap preview ptes
				pteTemp = (*newToc)->previewPTE;
				(*newToc)->previewPTE =
				    (*oldToc)->previewPTE;
				(*oldToc)->previewPTE = pteTemp;

				// Cleanup some plugin stuff
				for (i = 1;
				     cntl =
				     FindControlByRefCon(boxWin,
							 'plug' + i); i++)
					DisposeControl(cntl);	// Cleanup peanut buttons
				FVRemoveButtons(boxWin);	// Cleanup any fileview buttons

				cntl = FindControlByRefCon(boxWin, 'tabs');
				oldHasTab = cntl != nil;
				newHasTab =
				    ETLHasMBoxContextFolder(boxWin);
				if (oldHasTab != newHasTab) {
					if (oldHasTab)
						DisposeControl(cntl);	// Remove fileview tab
					else {
						// We will be adding a tab control
						// Need to remove existing header buttons so they are
						// added after the tab control
						for (i = 1;
						     i < BoxLinesLimit;
						     i++)
							if (cntl =
							    FindControlByRefCon
							    (boxWin,
							     'wide' + i))
								DisposeControl
								    (cntl);
						// Drawer switch, too
						cntl =
						    FindControlByRefCon
						    (boxWin,
						     kDrawerSwitch);
						if (cntl)
							DisposeControl
							    (cntl);
					}
				}
				// If new one already has a drawer, close it.
				if ((*newToc)->drawerWin)
					MBDrawerClose((*newToc)->
						      drawerWin);

				// Use this drawer
				(*newToc)->drawerWin =
				    (*oldToc)->drawerWin;
				(*oldToc)->drawerWin = NULL;

				// Keep same preview height
				(*newToc)->previewHi =
				    (*oldToc)->previewHi;

				// Make sure this TOC has drawer flag set
				if (!(*newToc)->drawer) {
					(*newToc)->drawer = true;
					TOCSetDirty(newToc, true);
				}

				(*dh)->tocH = newToc;
				MyWindowMaxes(boxWin, boxWin->hMax,
					      (*newToc)->count);

				// Update preview pane
				(*newToc)->previewID = -1;

				BoxGonnaShow(boxWin);
				oldContR = boxWin->contR;
				BoxSetBevelButtonValues(boxWin);	//       Update bevel button (sort) values
				MyWindowDidResize(boxWin, &oldContR);

				// Now draw the entire window at once
				boxWin->noUpdates = saveNoUpdates;
				InfiniteClip(GetMyWindowCGrafPtr(boxWin));
				InvalWindowRect(GetMyWindowWindowPtr
						(boxWin), &boxWin->contR);
				UpdateMyWindow(GetMyWindowWindowPtr
					       (boxWin));

				// listFocus is changed by BoxGonnaShow. Need to set it here.
				BoxListFocus(newToc, false);

				// Close old (hidden) mailbox window
				CloseMyWindow(GetMyWindowWindowPtr
					      (hideWin));

				// If IMAP, resync
				if ((*newToc)->imapTOC)
					FlagForResync(newToc);
			}
		}
		break;
	}
	return err;
}

/**********************************************************************
 * MBDrawerGetListRect - get list bounds
 **********************************************************************/
static void MBDrawerGetListRect(WindowRef winWP, Rect * r)
{
	GetWindowBounds(winWP, kWindowContentRgn, r);
	OffsetRect(r, -r->left, -r->top);
	r->bottom -= 5;
}

/**********************************************************************
 * MBDrawerSupported - are drawers supported
 **********************************************************************/
Boolean MBDrawerSupported(MyWindowPtr win)
{
	// Drawers require Jaguar (Mac OS 10.2)
	if (GetOSVersion() >= 0x1002)
		// Don't put drawers on search windows
		if (!IsSearchWindow(GetMyWindowWindowPtr(win)))
			// gotta have the feature
			if (HasFeature(featureMBDrawer))
				return true;
	return false;
}

/************************************************************************
 * MBDrawerDidResize - resize the MB window
 ************************************************************************/
static void MBDrawerDidResize(MyWindowPtr win, Rect * oldContR)
{
#pragma unused(oldContR)
	MBDrawerHandle dh = GetMBDrawerInfo(win);
	short htAdjustment;
	WindowRef winWP = GetMyWindowWindowPtr(win);
	Rect r;
	Boolean drawerOpen = GetDrawerState(winWP) != kWindowDrawerClosed;

	// set list bounds
	MBDrawerGetListRect(winWP, &r);
	LVSize((*dh)->list, &r, &htAdjustment);

	// save drawer width preference
	GetWindowBounds(winWP, kWindowContentRgn, &r);
	SetPrefLong(PREF_DRAWER_WIDTH, RectWi(r));

	InvalContent(win);

	if (drawerOpen != (*(*dh)->tocH)->drawer)
		// User has changed state by resizing
		MBDrawerSetState((*dh)->parentWin, drawerOpen);
}

/************************************************************************
 * MBDrawerKey - key stroke
 ************************************************************************/
static Boolean MBDrawerKey(MyWindowPtr win, EventRecord * event)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	if (dh)
		return LVKey((*dh)->list, event);
	return false;
}

/**********************************************************************
 * MBDrawerActivate - perform processing for activate/deactivate
 **********************************************************************/
void MBDrawerActivate(MyWindowPtr win, Boolean activate)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	if (dh)
		LVActivate((*dh)->list, (*dh)->hasFocus && activate);
}

/**********************************************************************
 * MBDrawerSetFocus - set focus
 **********************************************************************/
void MBDrawerSetFocus(MyWindowPtr win, Boolean focus)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	if (dh) {
		if (focus != (*dh)->hasFocus) {
			(*dh)->hasFocus = focus;
			LVActivate((*dh)->list, focus);
		}
	}
}

/************************************************************************
 * MBDrawerFixUnread - update mailbox list unread status
 ************************************************************************/
void MBDrawerFixUnread(MenuHandle mh, short item, Boolean unread)
{
	MBDrawerHandle dh;
	TOCHandle tocH;

	//      Find mailbox drawers
	for (tocH = TOCList; tocH; tocH = (*tocH)->next) {
		if ((*tocH)->drawerWin)
			if (dh = GetMBDrawerInfo((*tocH)->drawerWin))
				MBFixUnreadLo((*dh)->list, mh, item,
					      unread, true);
	}
}

/************************************************************************
 * MBDrawerReload - mailbox list has changed, reload
 ************************************************************************/
void MBDrawerReload(void)
{
	MBDrawerHandle dh;
	TOCHandle tocH;

	//      Find mailbox drawers
	for (tocH = TOCList; tocH; tocH = (*tocH)->next) {
		if ((*tocH)->drawerWin)
			if (dh = GetMBDrawerInfo((*tocH)->drawerWin))
				InvalidListView((*dh)->list);
	}
}



/**********************************************************************
 * MBDrawerDragHandler - handle drags
 *
 *  Supports only dropping of messages
 **********************************************************************/
static OSErr MBDrawerDragHandler(MyWindowPtr win,
				 DragTrackingMessage which,
				 DragReference drag)
{
	MBDrawerHandle dh = GetMBDrawerInfo(win);
	OSErr err = noErr;
	VLNodeInfo targetInfo;

	if (!dh)
		return noErr;

	if (!DragIsInteresting(drag, MESS_FLAVOR, TOC_FLAVOR, nil))	//      Dragging messages
		return (dragNotAcceptedErr);	//      Nothing here we want

	switch (which) {
	case kDragTrackingEnterWindow:
	case kDragTrackingLeaveWindow:
	case kDragTrackingInWindow:
		err = LVDrag((*dh)->list, which, drag);
		break;
	case 0xfff:
		//      Message drag
		if (LVDrop((*dh)->list, &targetInfo))
			err = MBDragMessages(drag, &targetInfo);
		else
			return dragNotAcceptedErr;
		break;
	}
	return err;
}

/************************************************************************
 * ControlEvents - handle carbon control events
 ************************************************************************/
static OSStatus ControlEvents(EventHandlerCallRef nextHandler,
			      EventRef theEvent, void *userData)
{
	ControlRef vScroll = (ControlRef) userData;
	Rect rScroll;
	Point pt, topleft;

	switch (GetEventKind(theEvent)) {
	case kEventControlHitTest:
		// Fix bug in List Manager
		GetControlBounds(vScroll, &rScroll);
		GetEventParameter(theEvent, kEventParamMouseLocation,
				  typeQDPoint, NULL, sizeof(Point), NULL,
				  &pt);
		if (PtInRect(pt, &rScroll)) {
			SetPt(&topleft, rScroll.left, rScroll.top);
			SubPt(topleft, &pt);
			SetEventParameter(theEvent,
					  kEventParamMouseLocation,
					  typeQDPoint, sizeof(Point), &pt);
		}
		break;
	}
	return CallNextEventHandler(nextHandler, theEvent);
}

/************************************************************************
 * ListControlEvents - handle carbon control events for list control
 ************************************************************************/
static OSStatus ListControlEvents(EventHandlerCallRef nextHandler,
				  EventRef theEvent, void *userData)
{
	static Boolean drawing;
	MyWindowPtr win = (ControlRef) userData;
	MBDrawerHandle dh = GetMBDrawerInfo(win);

	switch (GetEventKind(theEvent)) {
	case kEventControlDraw:
		// Time to draw mailbox list
		if (drawing)
			//  Avoid recursion
			break;
		drawing = true;
		LVDraw((*dh)->list, nil, true, false);
		drawing = false;
		break;
	}
	return CallNextEventHandler(nextHandler, theEvent);
}

/************************************************************************
 * OpenMyDrawer - open drawer watching out for toolbar
 ************************************************************************/
static void OpenMyDrawer(MyWindowPtr winDrawer)
{
	OptionBits inEdge;
	Rect rDrawer;
	short width;
	MBDrawerHandle dh = GetMBDrawerInfo(winDrawer);

	switch (GetPrefLong(PREF_DRAWER_EDGE)) {
	case 1:
		inEdge =
		    MainEvent.
		    modifiers & optionKey ? kWindowEdgeRight :
		    kWindowEdgeLeft;
		break;
	case 2:
		inEdge =
		    MainEvent.
		    modifiers & optionKey ? kWindowEdgeLeft :
		    kWindowEdgeRight;
		break;
	default:
		// Make sure there is room on the left without going under toolbar
		width = GetPrefLong(PREF_DRAWER_WIDTH);
		if (!width)
			width = kDefaultMBDrawerWidth;
		GetContentRgnBounds(GetMyWindowWindowPtr((*dh)->parentWin),
				    &rDrawer);
		rDrawer.right = rDrawer.left;
		rDrawer.left -= width;

		if (ToolbarSect(&rDrawer))
			inEdge = kWindowEdgeRight;
		else
			inEdge = kWindowEdgeDefault;
		break;
	}
	OpenDrawer(GetMyWindowWindowPtr(winDrawer), inEdge, true);
	SetDrawerIcon((*dh)->parentWin, true);
}
