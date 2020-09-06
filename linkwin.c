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

#include "linkwin.h"

#define FILE_NUM 126
/* Copyright (c) 1999 by QUALCOMM Incorporated */

#pragma segment LinkWin

#define NUM_COLUMNS 3

#define THUMB_COLUMN 0
#define NAME_COLUMN 1
#define DATE_COLUMN 2

#define MINI_BANNER_WIDTH 70

// Link History window controls
enum {
	kctlColumns1 = 0,
	kctlColumns2,
	kctlColumns3,
	kctlView,
	kctlRemove
};

typedef struct {
	ViewList list;
	MyWindowPtr win;
	ControlHandle ctlColumns[NUM_COLUMNS], ctlView, ctlRemove;
	Boolean inited;
	Boolean needsSort;
	short columnLefts[NUM_COLUMNS];
	LinkSortTypeEnum sort;
} WinData;
static WinData gWin;

short gColumnHeaderControls[NUM_COLUMNS] =
    { LINK_THUMB_COL_CNTL, LINK_NAME_COL_CNTL, LINK_DATE_COL_CNTL };

/************************************************************************
 * prototypes
 ************************************************************************/
// Link window
static void DoDidResize(MyWindowPtr win, Rect * oldContR);
static void DoZoomSize(MyWindowPtr win, Rect * zoom);
static void DoUpdate(MyWindowPtr win);
static Boolean DoClose(MyWindowPtr win);
static void DoClick(MyWindowPtr win, EventRecord * event);
static void DoCursor(Point mouse);
static void DoActivate(MyWindowPtr win);
static Boolean DoKey(MyWindowPtr win, EventRecord * event);
static OSErr DoDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag);
static void DoShowHelp(MyWindowPtr win, Point mouse);
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item,
			    short modifiers);
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message,
			     long data);
static Boolean GetListData(VLNodeInfo * data, short selectedItem);
static void SetControls(void);
static void DoGrow(MyWindowPtr win, Point * newSize);
static Boolean LinkFind(MyWindowPtr win, PStr what);
static Boolean LinkDrawRowCallBack(ViewListPtr pView, short item,
				   Rect * pRect, CellRec * pCellData,
				   Boolean select, Boolean eraseName);
static Boolean LinkFillBlankCallback(ViewListPtr pView);
static void OpenSelectedLinks(void);
static void DeleteSelectedLinks(void);
static void GetCellRectsForLHWin(ViewListPtr pView, CellRec * pCellData,
				 Rect * pRect, Rect * pIcon, Rect * pName,
				 Rect * pDate, Point * pNamePt,
				 Point * pDatePt);
static Boolean LinkAddColumnHeaders(void);
static void LinkPositionColumnHeaders(MyWindowPtr win, short count,
				      ControlHandle * pCtlList, short left,
				      short right, short top,
				      short height);
static short ColumnRight(short column, short max);
static void DoColumnClick(MyWindowPtr win, EventRecord * event,
			  ControlHandle ctl);
static Boolean LinkGetCellRectsCallBack(ViewListPtr pView,
					CellRec * cellData,
					Rect * cellRect, Rect * iconRect,
					Rect * nameRect);
static Boolean LinkInterestingClickCallBack(ViewListPtr pView,
					    CellRec * cellData,
					    Rect * cellRect, Point pt);
static void SetLinkWinBackgroundColor(void);
static LinkSortTypeEnum LinkSavedSortOrder(void);
static void LinkSaveSortOrder(LinkSortTypeEnum sort);

// Offline Link Dialog
typedef enum {
	kOfflineLinkDlogNow = 1,
	kOfflineLinkDlogCancel = 2,
	kOfflineLinkDlogBookmark = 3,
	kOfflineLinkDlogRemind = 4,
	kOfflineLinkDlogRemember = 5,
	kOfflineLinkDlogRemindText = 8,
	kOfflineLinkDlogText = 10
} OfflineLinkEnum;

enum {
	kRemindDlogNow = 1,
	kRemindDlogLater,
	kRemindDlogForget
};

#define VaildOfflineActionPref(p) ((p == kOfflineLinkDlogNow) || (p == kOfflineLinkDlogBookmark) || (p == kOfflineLinkDlogRemind))

OfflineLinkEnum DoOfflineLinkDialog(void);
void DoRemindNag(void);
OSErr RemindDlogInit(DialogPtr theDialog, long refcon);
Boolean RemindDlogHit(EventRecord * event, DialogPtr theDialog,
		      short itemHit, long dialogRefcon);
void RemindSortLinkWin(void);
Boolean RemindUserNow(void);
Boolean CanRemindUser(void);

/************************************************************************
 * OpenLinkWin - open the link window
 ************************************************************************/
void OpenLinkWin(void)
{
	DrawDetailsStruct details;
	WindowPtr gWinWinPtr;

	if (!HasFeature(featureLink))
		return;

	UseFeature(featureLink);

	if (SelectOpenWazoo(LINK_WIN))
		return;		//      Already opened in a wazoo

	if (!gWin.inited) {
		short err = 0;
		Rect r;

		if (GenHistoriesList() != noErr)
			goto fail;

		// age the links if we haven't in a while ...
		AgeLinks();

		if (!
		    (gWin.win =
		     GetNewMyWindow(LINK_WIND, nil, nil, BehindModal,
				    false, false, LINK_WIN))) {
			err = MemError();
			goto fail;
		}
		gWinWinPtr = GetMyWindowWindowPtr(gWin.win);
		SetWinMinSize(gWin.win, 320, 12 * FontLead);
		SetPort_(GetMyWindowCGrafPtr(gWin.win));
		ConfigFontSetup(gWin.win);
		MySetThemeWindowBackground(gWin.win,
					   kThemeListViewBackgroundBrush,
					   False);

		/*
		 * list
		 */

		SetRect(&r, 0, 0, 20, 20);

		details.arrowLeft = 2;
		details.iconTop = 2;
		details.iconLeft = details.arrowLeft;	//      First icon level
		details.iconLevelWidth = 0;
		details.textBottom = 5;
		details.rowHt = 36;
		details.nameAddMargin = 5;	//      Add to name width when drawing
		details.maxNameWidth = 225;	//      Approximate maximum name width
		details.keyNavTicks = 60;	//      Delay accepted between navigation keys

		// callbacks for list drawing
		details.DrawRowCallback = LinkDrawRowCallBack;
		details.FillBlankCallback = LinkFillBlankCallback;
		details.GetCellRectsCallBack = LinkGetCellRectsCallBack;
		details.InterestingClickCallback =
		    LinkInterestingClickCallBack;

		gWin.sort = LinkSavedSortOrder();
		gWin.needsSort = true;

		if (LVNewWithDetails
		    (&gWin.list, gWin.win, &r, 1, ViewListCallBack,
		     flavorTypeText, &details)) {
			err = MemError();
			goto fail;
		}
		// controls
		if (!
		    (gWin.ctlView =
		     NewIconButton(LINK_NEW_CNTL, gWinWinPtr))
		    || !(gWin.ctlRemove =
			 NewIconButton(LINK_REMOVE_CNTL, gWinWinPtr))
		    || !(LinkAddColumnHeaders()))
			goto fail;

		// columns.  Do this in a more flexible way someday
		gWin.columnLefts[THUMB_COLUMN] = 0;
		gWin.columnLefts[NAME_COLUMN] = MINI_BANNER_WIDTH + 2;
		gWin.columnLefts[DATE_COLUMN] =
		    gWin.columnLefts[NAME_COLUMN] + details.maxNameWidth;

		gWin.win->didResize = DoDidResize;
		gWin.win->close = DoClose;
		gWin.win->update = DoUpdate;
		gWin.win->position = PositionPrefsTitle;
		gWin.win->click = DoClick;
		gWin.win->bgClick = DoClick;
		gWin.win->dontControl = true;
		gWin.win->cursor = DoCursor;
		gWin.win->activate = DoActivate;
		gWin.win->help = DoShowHelp;
		gWin.win->menu = DoMenuSelect;
		gWin.win->key = DoKey;
		gWin.win->app1 = DoKey;
		gWin.win->drag = DoDragHandler;
		gWin.win->zoomSize = DoZoomSize;
		gWin.win->grow = DoGrow;
		gWin.win->idle = nil;
		gWin.win->find = LinkFind;
		gWin.win->showsSponsorAd = true;

		MyWindowDidResize(gWin.win, &gWin.win->contR);
		ShowMyWindow(gWinWinPtr);
		gWin.inited = true;
		return;

	      fail:
		if (gWin.win)
			CloseMyWindow(GetMyWindowWindowPtr(gWin.win));
		if (err)
			WarnUser(COULDNT_WIN, err);
		return;
	}
	UserSelectWindow(GetMyWindowWindowPtr(gWin.win));
}

/************************************************************************
 * LinkAddColumnHeaders - get the column header controls for the LH win
 ************************************************************************/
static Boolean LinkAddColumnHeaders(void)
{
	WindowPtr gWinWinPtr = GetMyWindowWindowPtr(gWin.win);
	Boolean result = true;
	short i;

	for (i = 0; result && (i < NUM_COLUMNS); i++) {
		if (!
		    (gWin.ctlColumns[i] =
		     NewIconButton(gColumnHeaderControls[i], gWinWinPtr)))
			result = false;
	}

	return (result);
}

/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect * oldContR)
{
#define kListInset 10
#pragma unused(oldContR)
	ControlHandle ctlList[NUM_COLUMNS];
	Rect r;
	short htAdjustment;
	short column;
	short columnHeaderHeight = GROW_SIZE + 5;

	// column header controls
	for (column = 0; column < NUM_COLUMNS; column++)
		ctlList[column] = gWin.ctlColumns[column];
	LinkPositionColumnHeaders(win, NUM_COLUMNS, ctlList, kListInset,
				  gWin.win->contR.right - kListInset,
				  win->topMargin + kListInset,
				  columnHeaderHeight);

	//      buttons
	ctlList[0] = gWin.ctlView;
	ctlList[1] = gWin.ctlRemove;
	PositionBevelButtons(win, 2, ctlList, kListInset,
			     gWin.win->contR.bottom - INSET - kHtCtl,
			     kHtCtl, RectWi(win->contR));

	// list
	SetRect(&r, kListInset,
		win->topMargin + kListInset + columnHeaderHeight,
		gWin.win->contR.right - kListInset,
		gWin.win->contR.bottom - 2 * INSET - kHtCtl);
	if (gWin.win->sponsorAdExists
	    && r.bottom >=
	    gWin.win->sponsorAdRect.top - kSponsorBorderMargin)
		r.bottom =
		    gWin.win->sponsorAdRect.top - kSponsorBorderMargin;
	LVSize(&gWin.list, &r, &htAdjustment);

	//      enable/disble controls
	SetControls();

	// redraw
	InvalContent(win);
}

/************************************************************************
 * LinkPositionColumnHeaders - position and size the column headers
 ************************************************************************/
static void LinkPositionColumnHeaders(MyWindowPtr win, short count,
				      ControlHandle * pCtlList, short left,
				      short right, short top, short height)
{
	short width[64];
	short i;
	short max = right - left + (count - 1);	// rightmost part of last column header, relative to the list.
	short columnLeft = left;

	//      Get width of each column header
	for (i = 0; i < count; i++) {
		width[i] =
		    pCtlList[i] ? (ColumnRight(i, max) -
				   gWin.columnLefts[i]) : 0;
	}

	// move the column headers
	for (i = 0; i < count; i++) {
		if (pCtlList[i]) {
			MoveMyCntl(pCtlList[i], columnLeft - 1, top,
				   width[i], height);
			columnLeft += width[i];
		}
	}
}


/************************************************************************
 * SetControls - enable or disable the controls, based on current situation
 ************************************************************************/
static void SetControls(void)
{
	short i, count = LVCountSelection(&gWin.list);
	Boolean fSelect = count != 0;
	LinkSortTypeEnum sortedColumn = gWin.sort & kLinkSortTypeMask;

	// depress sorted column control
	for (i = 0; i < NUM_COLUMNS; i++)
		SetControlValue(gWin.ctlColumns[i], (sortedColumn == i));

	// enable/disable buttons
	SetGreyControl(gWin.ctlView, !fSelect);
	SetGreyControl(gWin.ctlRemove, !fSelect);
}

/************************************************************************
 * DoZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void DoZoomSize(MyWindowPtr win, Rect * zoom)
{
	short zoomHi = zoom->bottom - zoom->top;
	short zoomWi = zoom->right - zoom->left;
	short hi, wi;

	LVMaxSize(&gWin.list, &wi, &hi);
	wi += 2 * kListInset;
	hi += 2 * kListInset + INSET + kHtCtl;

	wi = MIN(wi, zoomWi);
	wi = MAX(wi, win->minSize.h);
	hi = MIN(hi, zoomHi);
	hi = MAX(hi, win->minSize.v);
	zoom->right = zoom->left + wi;
	zoom->bottom = zoom->top + hi;
}

/************************************************************************
 * DoGrow - adjust grow size
 ************************************************************************/
static void DoGrow(MyWindowPtr win, Point * newSize)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Rect r;
	short htControl = ControlHi(gWin.ctlView);
	short bottomMargin, sponsorMargin;

	//      Get list position
	bottomMargin = INSET * 2 + htControl;
	if (win->sponsorAdExists) {
		Rect rPort;

		GetPortBounds(GetWindowPort(winWP), &rPort);
		sponsorMargin =
		    rPort.bottom - win->sponsorAdRect.top +
		    kSponsorBorderMargin;
		if (sponsorMargin > bottomMargin)
			bottomMargin = sponsorMargin;
	}
	SetRect(&r, kListInset, win->topMargin + kListInset,
		newSize->h - kListInset, newSize->v - bottomMargin);
	LVCalcSize(&gWin.list, &r);

	//      Calculate new window height
	newSize->v = r.bottom + bottomMargin;
}

/************************************************************************
 * DoClose - close the window
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win)
{
#pragma unused(win)

	if (gWin.inited) {
		//      Dispose of list
		LVDispose(&gWin.list);
		gWin.inited = false;

		// clean up the history list, but don't throw it away.
		ZapHistoriesList(false);

		// Take care of the icon cache as well
		ZapPVICache();
	}
	return (True);
}

/************************************************************************
 * DoUpdate - draw the window
 ************************************************************************/
static void DoUpdate(MyWindowPtr win)
{
	CGrafPtr winPort = GetMyWindowCGrafPtr(win);
	Rect r, rCntl0, rCntlLast;

	// Tweak the link history window colors
	SetLinkWinBackgroundColor();

	// draw the list
	r = gWin.list.bounds;
	DrawThemeListBoxFrame(&r, kThemeStateActive);
	LVDraw(&gWin.list, MyGetPortVisibleRegion(winPort), true, false);

	// Draw a rect around the column controls
	GetControlBounds(gWin.ctlColumns[0], &rCntl0);
	GetControlBounds(gWin.ctlColumns[NUM_COLUMNS - 1], &rCntlLast);
	SetRect(&r, rCntl0.left - 1, rCntl0.top - 1, rCntlLast.right,
		rCntl0.bottom);
	FrameRect(&r);
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate(MyWindowPtr win)
{
#pragma unused(win)
	LVActivate(&gWin.list, gWin.win->isActive);
	SetControls();
}

/************************************************************************
 * DoKey - key stroke
 ************************************************************************/
static Boolean DoKey(MyWindowPtr win, EventRecord * event)
{
#pragma unused(win)
	short key = (event->message & 0xff);

	if (LVKey(&gWin.list, event)) {
		SetControls();
		return true;
	}

	return false;
}

/**********************************************************************
 * LinkFind - find in the window
 **********************************************************************/
static Boolean LinkFind(MyWindowPtr win, PStr what)
{
	return FindListView(win, &gWin.list, what);
}

/************************************************************************
 * DoClick - click in window
 ************************************************************************/
void DoClick(MyWindowPtr win, EventRecord * event)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Point pt;
	ControlHandle hCtl;

	SetPort(GetMyWindowCGrafPtr(win));

	if (!LVClick(&gWin.list, event)) {
		pt = event->where;
		GlobalToLocal(&pt);
		if (!win->isActive) {
			SelectWindow_(winWP);
			UpdateMyWindow(winWP);	//      Have to update manually since no events are processed
		}

		if (FindControl(pt, winWP, &hCtl)) {
			if (TrackControl(hCtl, pt, (void *) (-1))) {
				if (hCtl == gWin.ctlView) {
					OpenSelectedLinks();
					AuditHit((event->
						  modifiers & shiftKey) !=
						 0,
						 (event->
						  modifiers & controlKey)
						 != 0,
						 (event->
						  modifiers & optionKey) !=
						 0,
						 (event->
						  modifiers & cmdKey) != 0,
						 false,
						 GetWindowKind(winWP),
						 AUDITCONTROLID
						 (GetWindowKind(winWP),
						  kctlView), event->what);
				} else if (hCtl == gWin.ctlRemove) {
					DeleteSelectedLinks();
					AuditHit((event->
						  modifiers & shiftKey) !=
						 0,
						 (event->
						  modifiers & controlKey)
						 != 0,
						 (event->
						  modifiers & optionKey) !=
						 0,
						 (event->
						  modifiers & cmdKey) != 0,
						 false,
						 GetWindowKind(winWP),
						 AUDITCONTROLID
						 (GetWindowKind(winWP),
						  kctlRemove),
						 event->what);
				} else
					DoColumnClick(win, event, hCtl);
			}
		}
	}
	SetControls();
}

/************************************************************************
 * DoColumnClick - Handle a click on a column header, if indeed it is one
 ************************************************************************/
static void DoColumnClick(MyWindowPtr win, EventRecord * event,
			  ControlHandle ctl)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	short column = 0;
	LinkSortTypeEnum oldSort = gWin.sort;
	LinkSortTypeEnum newSort;

	while (column < NUM_COLUMNS) {
		if (ctl == gWin.ctlColumns[column]) {
			newSort = column;
			if (event->modifiers & optionKey)
				newSort |= kLinkReverseSort;

			if (gWin.sort != newSort) {
				gWin.needsSort = true;
				gWin.sort = newSort;
				LinkSaveSortOrder(gWin.sort);
				LinkTickle();
			}
			AuditHit((event->modifiers & shiftKey) != 0,
				 (event->modifiers & controlKey) != 0,
				 (event->modifiers & optionKey) != 0,
				 (event->modifiers & cmdKey) != 0, false,
				 GetWindowKind(winWP),
				 AUDITCONTROLID(GetWindowKind(winWP),
						column), mouseDown);
			break;
		}
		column++;
	}
}

/************************************************************************
 * NotifyLinkWin - notify the link window that we've been up to no good
 ************************************************************************/
void NotifyLinkWin(void)
{
	if (!HasFeature(featureLink))
		return;

	if (gWin.inited)
		InvalidListView(&gWin.list);	//      Regenerate list 
}

/************************************************************************
 * DoCursor - set the cursor properly for the window
 ************************************************************************/
static void DoCursor(Point mouse)
{
	if (!PeteCursorList(gWin.win->pteList, mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * DoShowHelp - provide help for the window
 ************************************************************************/
static void DoShowHelp(MyWindowPtr win, Point mouse)
{
	if (PtInRect(mouse, &gWin.list.bounds))
		MyBalloon(&gWin.list.bounds, 100, 0, LINK_HELP_STRN + 1, 0,
			  nil);
	else
		ShowControlHelp(mouse, LINK_HELP_STRN + 2, gWin.ctlView,
				gWin.ctlRemove,
				gWin.ctlColumns[THUMB_COLUMN],
				gWin.ctlColumns[NAME_COLUMN],
				gWin.ctlColumns[DATE_COLUMN], nil);
}

/************************************************************************
 * GetListData - get data for selected item
 ************************************************************************/
static Boolean GetListData(VLNodeInfo * data, short selectedItem)
{
	return LVGetItem(&gWin.list, selectedItem, data, true);
}

/************************************************************************
 * DoMenuSelect - menu choice in the window
 ************************************************************************/
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item,
			    short modifiers)
{
#pragma unused(win,modifiers)

	switch (menu) {
	case FILE_MENU:
		switch (item) {
		case FILE_OPENSEL_ITEM:	// this should open the selected link
//                                      MBListOpen();
			return (True);
			break;
		}
		break;

	case EDIT_MENU:
		if (item == EDIT_SELECT_ITEM) {
			if (LVSelectAll(&gWin.list)) {
				SetControls();
				return (true);
			}
		}
		break;
	}
	return (False);
}

/**********************************************************************
 * DoDragHandler - handle drags
 **********************************************************************/
static OSErr DoDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag)
{
#pragma unused(win)
	OSErr err = noErr;

//#warning  don't forget to handle drags from link history window!

	if (!DragIsInteresting(drag, MESS_FLAVOR, TOC_FLAVOR, nil))
		return (dragNotAcceptedErr);	//      Nothing here we want

	switch (which) {
	case kDragTrackingEnterWindow:
	case kDragTrackingLeaveWindow:
	case kDragTrackingInWindow:
		err = LVDrag(&gWin.list, which, drag);
		break;
	case 0xfff:
		break;
	}
	return err;
}

/************************************************************************
 * LinkFillBlankCallback - callback function for List View.  Fill unused 
 *	parts of the list.
 ************************************************************************/
static Boolean LinkFillBlankCallback(ViewListPtr pView)
{
	ListHandle hList = pView->hList;
	Rect rErase;
	GrafPtr savePort;
	short column;
	Rect columnRect;
	LinkSortTypeEnum sortedColumn = gWin.sort & kLinkSortTypeMask;

	if ((*hList)->visible.bottom > (*hList)->dataBounds.bottom) {
		SAVE_STUFF;

		SET_COLORS;
		GetPort(&savePort);
		SetPort(GetMyWindowCGrafPtr(pView->wPtr));
		Cell1Rect(hList, (*hList)->dataBounds.bottom + 1, &rErase);
		rErase.bottom = pView->bounds.bottom;

		if (UseListColors) {
			for (column = 0; column < NUM_COLUMNS; column++) {
				// what is the rect describing this column?
				SetRect(&columnRect,
					rErase.left +
					gWin.columnLefts[column],
					rErase.top,
					rErase.left + ColumnRight(column,
								  rErase.
								  right),
					rErase.bottom);

				// color this column
				SetThemeBackground(sortedColumn ==
						   column ?
						   kThemeBrushListViewSortColumnBackground
						   :
						   kThemeBrushListViewBackground,
						   RectDepth(&columnRect),
						   true);
				EraseRect(&columnRect);
			}
		}

		SetPort(savePort);
		REST_STUFF;
	}
	return true;		/* mtc sez - this value is never checked 11-18-03 */
}

/************************************************************************
 * ColumnRight - return the right side of a column
 ************************************************************************/
static short ColumnRight(short column, short max)
{
	short right = 0;

	if ((column >= 0) && (column < NUM_COLUMNS)) {
		if (column < NUM_COLUMNS - 1)
			right = gWin.columnLefts[column + 1];
		else
			right = max;
	}

	return (right);
}

/************************************************************************
 * LinkDrawRowCallBack - callback function for List View.  Draw a row.
 ************************************************************************/
static Boolean LinkDrawRowCallBack(ViewListPtr pView, short item,
				   Rect * pRect, CellRec * pCellData,
				   Boolean select, Boolean eraseName)
{
	Boolean result = true;
	Rect rIcon, rName, rDate;
	Point ptName, ptDate;
	Boolean hasColor = IsColorWin(GetMyWindowWindowPtr(pView->wPtr));
	Str255 dateStr;
	short column;
	Rect columnRect;
	Handle theIcon;
	LinkSortTypeEnum sortedColumn = gWin.sort & kLinkSortTypeMask;

	if (item && item <= (*pView->hList)->dataBounds.bottom) {
		SAVE_STUFF;

		//
		// Figure out where things are in this row
		//

		GetCellRectsForLHWin(pView, pCellData, pRect, &rIcon,
				     &rName, &rDate, &ptName, &ptDate);


		//
		//      Draw the list background color and seperator lines for this row
		//

		if (UseListColors) {
			for (column = 0; column < NUM_COLUMNS; column++) {
				// what is the rect describing this column?
				SetRect(&columnRect,
					pRect->left +
					gWin.columnLefts[column],
					pRect->top,
					pRect->left + ColumnRight(column,
								  pRect->
								  right),
					pRect->top +
					(*pView->hList)->cellSize.v - 1);

				// color this column
				if (sortedColumn == column
				    || ((sortedColumn == sSpecialRemind)
					&&
					(IsMarkedRemind
					 (pCellData->nodeID))))
					SetThemeBackground
					    (kThemeBrushListViewSortColumnBackground,
					     RectDepth(&columnRect), true);
				else
					SetThemeBackground
					    (kThemeBrushListViewBackground,
					     RectDepth(&columnRect), true);
				EraseRect(&columnRect);
			}

			SET_COLORS;
		}
		//
		//      Draw the contents of this item
		//

		SET_COLORS;

		//      Plot the icon
		if (pCellData->iconID) {
			switch (pCellData->iconID) {
			case kCustomIconResource:
				theIcon =
				    GetLHPreviewIcon(pCellData->nodeID);
				if (theIcon) {
					PlotIconSuite(&rIcon,
						      atNone +
						      atHorizontalCenter,
						      select ? ttSelected :
						      ttNone, theIcon);
					HPurge(theIcon);	// This icon can be purged when needed.
					break;
				} else {
					// this ad does not have a preview for some reason
					pCellData->iconID =
					    HTTP_LINK_TYPE_ICON;

					// fall through and plot the http icon
				}

			default:
				OffsetRect(&rIcon, 0, -1);
				PlotIconID(&rIcon,
					   atNone + atHorizontalCenter,
					   select ? ttSelected : ttNone,
					   pCellData->iconID);
				break;
			}
		}
		//      Draw the name
		MoveTo(ptName.h, ptName.v);
		if (eraseName)
			EraseRect(&rName);
		if (pCellData->misc.style)
			TextFace(pCellData->misc.style);

		TextFont(pView->font);
		TextSize(pView->fontSize);

		DrawString(pCellData->name);
		if (pCellData->misc.style)
			TextFace(0);

		if (select)
			InvertRect(&rName);

		// Draw the date
		if (GetDateString(pCellData->nodeID, dateStr)) {
			MoveTo(ptDate.h, ptDate.v);
			if (eraseName)
				EraseRect(&rDate);
			if (pCellData->misc.style)
				TextFace(pCellData->misc.style);

			TextFont(pView->font);
			TextSize(pView->fontSize);

			DrawString(dateStr);
			if (pCellData->misc.style)
				TextFace(0);
		}

		REST_STUFF;
	}

	return (result);
}

/**********************************************************************
 * GetCellRects - get triangle, icon, and name rects
 **********************************************************************/
static void GetCellRectsForLHWin(ViewListPtr pView, CellRec * pCellData,
				 Rect * pRect, Rect * pIcon, Rect * pName,
				 Rect * pDate, Point * pNamePt,
				 Point * pDatePt)
{
	short offset;
	Point pt;
	FontInfo fInfo;
	short column;

	SAVE_STUFF;

	for (column = 0; column < NUM_COLUMNS; column++) {
		switch (column) {
		case THUMB_COLUMN:
			//      Thumb or Icon
			if (pCellData->iconID) {
				// This entry has an icon.  Return rect of icon centered in thumb column.

				if (column < NUM_COLUMNS - 1) {
					// center of column, minus half of an icon
					offset =
					    (gWin.columnLefts[column + 1] -
					     gWin.columnLefts[column]) /
					    2 - 16;
				} else {
					// half an icon away from the leftmost part of the column
					offset =
					    gWin.columnLefts[column] + 48;
				}
				SetRect(pIcon, pRect->left + offset,
					pRect->top +
					(pView->details.iconTop),
					pRect->left + offset + 32,
					pRect->top +
					(pView->details.iconTop) + 32);
			} else {
				// This entry has a thumbnail.
				SetRect(pIcon,
					pRect->left +
					(pView->details.arrowLeft),
					pRect->top +
					(pView->details.iconTop),
					pRect->left +
					(pView->details.arrowLeft) + 62,
					pRect->top +
					(pView->details.iconTop) + 32);
			}
			break;

		case NAME_COLUMN:
		case DATE_COLUMN:
			offset =
			    gWin.columnLefts[column] +
			    (pView->details.nameAddMargin);
			TextFont(pView->font);
			TextSize(pView->fontSize);
			pt.h = pRect->left + offset;
			pt.v = pRect->top + ((*pView->hList)->cellSize.v / 2) + (pView->fontSize) / 2;	// center the text vertically
			GetFontInfo(&fInfo);

			if (column == NAME_COLUMN) {
				*pNamePt = pt;
				SetRect(pName, pt.h,
					pt.v - fInfo.ascent -
					fInfo.leading,
					pt.h +
					StringWidth(pCellData->name),
					pt.v + fInfo.descent);
			} else if (column == DATE_COLUMN) {
				*pDatePt = pt;
				SetRect(pDate, pt.h,
					pt.v - fInfo.ascent -
					fInfo.leading,
					pt.h +
					StringWidth(pCellData->name),
					pt.v + fInfo.descent);
			}
			break;

		default:
			break;
		}

	}



	REST_STUFF;
}

/************************************************************************
 * ViewListCallBack - callback function for List View
 ************************************************************************/
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message,
			     long data)
{
	VLNodeInfo *pInfo;
	OSErr err = noErr;
	Handle hUrl;
	SendDragDataInfo *pSendData;

	switch (message) {
	case kLVAddNodeItems:
		AddAllHistoryItems(pView, gWin.needsSort, gWin.sort);	// Add History items to the list.  Sort if necessary.
		break;

	case kLVOpenItem:
		OpenSelectedLinks();
		break;

	case kLVDeleteItem:
		DeleteSelectedLinks();
		break;

	case kLVRenameItem:
		break;

	case kLVQueryItem:
		pInfo = (VLNodeInfo *) data;
		switch (pInfo->query) {
		case kQuerySelect:
		case kQueryDrag:
		case kQueryDCOpens:
			return true;

		case kQueryDrop:
		case kQueryRename:
			return false;
		}
		break;

	case kLVSendDragData:
		pSendData = (SendDragDataInfo *) data;
		hUrl = GetLinkURL(pSendData->info);
		HLock(hUrl);
		err =
		    SetDragItemFlavorData(pSendData->drag,
					  pSendData->itemRef,
					  pSendData->flavor, *hUrl,
					  InlineGetHandleSize((Handle)
							      hUrl), 0L);
		UL(hUrl);
		break;
	}
	return err;
}

/************************************************************************
 * OpenSelectedLinks - open all selected links
 ************************************************************************/
static void OpenSelectedLinks(void)
{
	short i;
	VLNodeInfo info;

	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		GetListData(&info, i);
		OpenHistoryEntry(&info);
	}
}

/************************************************************************
 * DeleteSelectedLinks - delete all selected links
 ************************************************************************/
static void DeleteSelectedLinks(void)
{
	short i;
	VLNodeInfo info;

	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		GetListData(&info, i);
		DeleteHistoryEntry(&info);
	}

	// Save all history files that need it
	SaveAllHistoryFiles();

	// Tell the Link Window to redraw itself.
	LinkTickle();
}

/************************************************************************
 * LinkTickle - tickle the link window
 ************************************************************************/
void LinkTickle(void)
{
	if (gWin.inited) {
		InvalidListView(&gWin.list);
		UpdateMyWindow(GetMyWindowWindowPtr(gWin.win));
	}
}

/************************************************************************
 * LinkInterestingClickCallBack - return true if this point is interesting
 ************************************************************************/
static Boolean LinkInterestingClickCallBack(ViewListPtr pView,
					    CellRec * cellData,
					    Rect * cellRect, Point pt)
{
	Boolean result = false;
	Rect rIcon, rName, rDate;
	Point ptName, ptDate;

	GetCellRectsForLHWin(pView, cellData, cellRect, &rIcon, &rName,
			     &rDate, &ptName, &ptDate);
	if (PtInRect(pt, &rIcon) || PtInRect(pt, &rName)
	    || PtInRect(pt, &rDate))
		result = true;

	return (result);
}

/**********************************************************************
 * GetCellData - Get data for cell,item is 0-based
 **********************************************************************/
static short GetCellData(ViewListPtr pView, short item,
			 CellRec * pCellData)
{
	short dataLen;
	Cell c;

	dataLen = sizeof(CellRec);
	c.h = 0;
	c.v = item;
	LGetCell((Ptr) pCellData, &dataLen, c, pView->hList);
	return dataLen;
}

/************************************************************************
 * LinkGetCellRectsCallBack - return true if this we should drag
 ************************************************************************/
static Boolean LinkGetCellRectsCallBack(ViewListPtr pView,
					CellRec * cellData,
					Rect * cellRect, Rect * iconRect,
					Rect * nameRect)
{
	Rect rDate;
	Point ptName, ptDate;

	GetCellRectsForLHWin(pView, cellData, cellRect, iconRect, nameRect,
			     &rDate, &ptName, &ptDate);

	return (true);
}

/************************************************************************
 * SetLinkWinBackgroundColor - set the background color of the link win
 ************************************************************************/
static void SetLinkWinBackgroundColor(void)
{
	if (UseListColors) {
		// Using finder list color scheme.  Punt on background color.
		gWin.win->backColor.red = gWin.win->backColor.green =
		    gWin.win->backColor.blue = 0xffff;
	} else {
		// Using BACK_COLOR.
		GetRColor(&gWin.win->backColor, BACK_COLOR);
	}
}

/************************************************************************
 * LinkHasCustomIcons - return if we should draw preview icons
 ************************************************************************/
Boolean LinkHasCustomIcons(void)
{
	long flags = GetRLong(LINK_HISTORY_UI_FLAGS);

	return ((flags & kLinkHasNoCustomIcons) == 0);
}

/************************************************************************
 * LinkSavedSortOrder - return sort order saved from last time
 ************************************************************************/
static LinkSortTypeEnum LinkSavedSortOrder(void)
{
	long flags = GetRLong(LINK_HISTORY_UI_FLAGS);

	if (flags & kLinkHasNoCustomIcons)
		flags &= ~kLinkHasNoCustomIcons;

	return (flags);
}

/************************************************************************
 * LinkSaveSortOrder - remember the sort order
 ************************************************************************/
static void LinkSaveSortOrder(LinkSortTypeEnum sort)
{
	long newFlags = sort;

	if (!LinkHasCustomIcons())
		newFlags |= kLinkHasNoCustomIcons;

	SetPrefLong(LINK_HISTORY_UI_FLAGS, newFlags);
}

/************************************************************************
 * PreventOfflineLink - don't follow this link if we're offline
 ************************************************************************/
Boolean PreventOfflineLink(OSErr * err, Boolean justDoIt)
{
	Boolean preventConnection = false;
	Boolean goingToFail = false;
	long offlineAction = 0;

	*err = noErr;

	// Only makes sense to do this if the Link Window is present
	if (!HasFeature(featureLink))
		return (false);

	if (!justDoIt) {
		// don't follow this link if we're offline
		goingToFail = Offline;

		// this link will fail if we have to dial
		if (!goingToFail) {
			if (!CanCheckPPPState())
				return (false);	// if we can't tell if we're offline, pretend we're online
			goingToFail = TCPWillDial(true);
		}
	}
	// if the machine is running OT, we know for sure if the link is going to fail or not now.
	if (goingToFail || justDoIt) {
		// did the user tell us what to do last time?  If not, ask again.
		offlineAction = GetRLong(PREF_OFFLINE_LINK_ACTION);
		if (!VaildOfflineActionPref(offlineAction)
		    || PrefIsSet(PREF_LINK_OFFLINE_WARN))
			offlineAction = DoOfflineLinkDialog();

		// if the user cancels the dialog, prevent the connection.
		if (offlineAction == kOfflineLinkDlogCancel) {
			// we won't do anything
			*err = olaCancel;

			// prevent the connection ...
			preventConnection = true;
		}
		// connect now
		if (offlineAction == kOfflineLinkDlogNow) {
			// connect to the internet right now ...
			*err = OTConnectForLink();
			if (*err != noErr)
				preventConnection = true;
		}
		// bookmark
		if (offlineAction == kOfflineLinkDlogBookmark) {
			// we will mark the link in the Link History window ...
			*err = oldaBookmark;

			// and not connect right now ...
			preventConnection = true;
		}
		// remind later
		if (offlineAction == kOfflineLinkDlogRemind) {
			// we will remember this link ...
			*err = oldaRemind;

			// and not connect right now ...
			preventConnection = true;
		}
	}
	// else
	// no need to prevent the connection.

	return (preventConnection);
}

#ifdef	NOT_NEEDED
/************************************************************************
 * PingAdServer - can we see the ad server?
 *
 *	Do this before launching an ad or web link.  This will either cause
 * the OS to dial, which will happen anyway when the web browser comes
 * up, or produce some sort of error.
 *
 *	If there's an error, we know we're not online, and we can display the
 * Offline Link Dialog.
 ************************************************************************/
Boolean PingAdServer(void)
{
	Boolean result = false;
	TransStream stream = nil;
	extern char PlayListURL[];	// currently in adwin.c
	Str255 proto, host, query;
	unsigned char *p;
	long port = 80;		//default http: port
	OSErr err = NewTransStream(&stream);
	Boolean oldPref = PrefIsSet(PREF_IGNORE_PPP);

//#warning Ping the Adserver, whatever we're supposed to
	if (err == noErr) {
		// extract the host from the playlist url
		if ((err =
		     ParseURL(PlayListURL, proto, host, query)) == noErr) {
			if (host[0]) {
				// null terminate host ...
				host[MIN(host[0] + 1, 255)] = 0;

				// figure out the server and port from the hostname ...
				p = PIndex(host, ':');
				if (p) {
					host[0] = p - host - 1;	// host now contains the hostname only;
					p++;
					if (port = atoi(p));	// port is whatever's left over
					else
						port = 80;	// or the default http: port
				}
				// try to connect to the host or something.  See if it's there.
				SetPref(PREF_IGNORE_PPP, YesStr);
				err =
				    ConnectTrans(stream, host, port, true,
						 GetRLong
						 (OFFLINE_LINK_OPEN_TIMEOUT));
				SetPref(PREF_IGNORE_PPP,
					oldPref ? YesStr : NoStr);

				// That's all we wanted.  Cleanup.
				if (err == noErr) {
					DestroyTrans(stream);
					result = true;
				}
				ZapTransStream(&stream);

			}
		}
	}

	return (result);
}
#endif				//NOT_NEEDED

/************************************************************************
 * DoOfflineLinkDialog - Ask the user what to do with a link that can't
 *	be followed right now.
 ************************************************************************/
OfflineLinkEnum DoOfflineLinkDialog(void)
{
	MyWindowPtr offlineLinkDlogWin;
	DialogPtr offlineLinkDlog;
	extern ModalFilterUPP DlgFilterUPP;
	short itemHit;
	Boolean done;
	short itemType;
	Handle itemH;
	Rect rect;
	Boolean remember;

	SAVE_PORT;

	//get the Offline Link dialog
	SetDialogFont(SmallSysFontID());
	offlineLinkDlogWin =
	    GetNewMyDialog(OFFLINE_LINK_DLOG, nil, nil, InFront);
	SetDialogFont(0);

	offlineLinkDlog = GetMyWindowDialogPtr(offlineLinkDlogWin);

	if (!offlineLinkDlog) {
		WarnUser(GENERAL, MemError());
		return (kOfflineLinkDlogCancel);
	}

	//fix the controls
	SetPort(GetDialogPort(offlineLinkDlog));
	ConfigFontSetup(offlineLinkDlogWin);
//      ReplaceAllControls(offlineLinkDlog);                    // let's leave the checkbox alone ...

	// uncheck the Remember checkbox
	remember = false;
	GetDialogItem(offlineLinkDlog, kOfflineLinkDlogRemember, &itemType,
		      &itemH, &rect);
	SetDItemState(offlineLinkDlog, kOfflineLinkDlogRemember, remember);

	// hide the Remind stuff if we're too uninformed
	if (!CanRemindUser()) {
		HideDialogItem(offlineLinkDlog, kOfflineLinkDlogRemind);
		HideDialogItem(offlineLinkDlog,
			       kOfflineLinkDlogRemindText);
	}

	//Do the dialog
	SetMyCursor(arrowCursor);
	StartMovableModal(offlineLinkDlog);
	ShowWindow(GetMyWindowWindowPtr(offlineLinkDlogWin));

	done = false;
	do {
		MovableModalDialog(offlineLinkDlog, DlgFilterUPP,
				   &itemHit);
		switch (itemHit) {
		case kOfflineLinkDlogRemember:
			remember = !remember;
			GetDialogItem(offlineLinkDlog,
				      kOfflineLinkDlogRemember, &itemType,
				      &itemH, &rect);
			SetDItemState(offlineLinkDlog,
				      kOfflineLinkDlogRemember, remember);
			break;

		case kOfflineLinkDlogNow:
		case kOfflineLinkDlogBookmark:
		case kOfflineLinkDlogRemind:
		case kOfflineLinkDlogCancel:
			done = true;
			break;

		default:
			break;
		}
	} while (!done);

	// Set the remember pref properly if we were supposed to ...
	if (remember && VaildOfflineActionPref(itemHit)) {
		SetPrefLong(PREF_OFFLINE_LINK_ACTION, itemHit);
		SetPref(PREF_LINK_OFFLINE_WARN, NoStr);
	} else {
		SetPrefLong(PREF_OFFLINE_LINK_ACTION, 0);
		SetPref(PREF_LINK_OFFLINE_WARN, YesStr);
	}

	EndMovableModal(offlineLinkDlog);
	DisposDialog_(offlineLinkDlog);

	REST_PORT;

	return (itemHit);
}

/************************************************************************
 * RemindNag - Nag the user about links remembered in the Link Window
 ************************************************************************/
void RemindNag(void)
{
	// Are there any links to remind the user about?
	if (gNeedRemind) {
		// nothing to do if the Link Window isn't around.
		if (!HasFeature(featureLink))
			return;

		if (!CanCheckPPPState())
			return;

		// see if we're online.
		if (RemindUserNow()) {
			// Nag the user
			DoRemindNag();

			// don't need to do remind again anytime soon.
			gNeedRemind = false;
		}
	}
}

/************************************************************************
 * DoRemindNag - display the remind nag
 ************************************************************************/
void DoRemindNag(void)
{
	extern ModalFilterUPP DlgFilterUPP;

	Nag(NAG_LINK_REMIND_DLOG, RemindDlogInit, RemindDlogHit, nil,
	    false, 0);
}

/************************************************************************
 * RemindDlogInit - init the Remind nag
 ************************************************************************/
OSErr RemindDlogInit(DialogPtr theDialog, long refcon)
{
	OSErr theError = noErr;

	ConfigFontSetup(GetDialogMyWindowPtr(theDialog));

	return (theError);
}

/************************************************************************
 * RemindDlogHit - handle clicks in the remind dialog
 ************************************************************************/
Boolean RemindDlogHit(EventRecord * event, DialogPtr theDialog,
		      short itemHit, long dialogRefcon)
{
	switch (itemHit) {
		// remind the user about the sites NOW
	case kRemindDlogNow:
		CloseMyWindow(GetDialogWindow(theDialog));
		RemindSortLinkWin();
		UnRemindLinks(false);
		break;

		// make the dialog go away, but don't forget about the sites
	case kRemindDlogLater:
		CloseMyWindow(GetDialogWindow(theDialog));
		break;

		// forget about the sites completely
	case kRemindDlogForget:
		UnRemindLinks(true);
		CloseMyWindow(GetDialogWindow(theDialog));
		LinkTickle();
		break;
	}
	return (true);
}

/************************************************************************
 * RemindSortLinkWin - sort the link window so the Remind links filter
 *	to the top.
 ************************************************************************/
void RemindSortLinkWin(void)
{
	OpenLinkWin();

	if (gWin.inited) {
		gWin.needsSort = true;
		gWin.sort = sSpecialRemind;
		LinkSaveSortOrder(gWin.sort);
		ScrollTopListView(&(gWin.list));
		SetControls();
		LinkTickle();
	}
}

/************************************************************************
 * RemindUserNow - are we online enough to remind the user about links?
 *	This is a lot like AutoCheckOK().
 ************************************************************************/
Boolean RemindUserNow(void)
{
	uLong hourBits;
	DateTimeRec dtr;

	// if we're offline, forget about it
	if (Offline)
		return (False);

	// if we're on batteries, and we're not supposed to check while on batteries, don't fire up the network for this either.
	// go ahead and remind the user if the network is up, even if the machine is on batteries. -jdboyd 01/22/01
//      if (PrefIsSet(PREF_NO_BATT_CHECK) && OnBatteries()) return(False);

	// if PPP is down, forget it.
	if (PPPDown())
		return (False);

	// are we outside the range of time we're allowed to check?  Don't remind the user now, either
	hourBits = GetPrefLong(PREF_CHECK_HOURS);
	if (0 != (hourBits & kHourUseMask)
	    || 0 != (hourBits & kHourNeverWeekendMask)) {
		SecondsToDate(LocalDateTime(), &dtr);
		if (dtr.dayOfWeek == 1 || dtr.dayOfWeek == 7)	// weekends
		{
			if (hourBits & kHourAlwaysWeekendMask)
				return (True);
			if (hourBits & kHourNeverWeekendMask)
				return (False);
		}
		if (0 != (hourBits & kHourUseMask))	// non-weekends, or weekend same as weekday
		{
			return 0 != (hourBits & (1L << dtr.hour));
		}
	}
	return (True);
}

/************************************************************************
 * CanRemindUser - we can only reliably remind the user if OT/PPP or
 *	MacSLIP is being used, or if we're fully offline.
 ************************************************************************/
Boolean CanRemindUser(void)
{
	// we're offline.  We'll remind the user once we're back online.
	if (Offline)
		return true;

	// we're using OT/PPP or MacSLIP to connect
	if (CanCheckPPPState())
		return true;

	// otherwise, we're not going to be able to tell when we go back online.
	return (false);
}
