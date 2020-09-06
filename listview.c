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

#include <Folders.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

/**********************************************************************
 * Implements a hierarchical list similar to Finder's list view
 **********************************************************************/

#include "listview.h"
#define FILE_NUM 83

//      Don't include auto bring-to-front feature. There is a bug when returning the window
//      to it's previous position--it sometimes gets lost. This bug is not reproduceable and
//      I can't determine what's causing it. ALB 1/24/97
//      Well, let's try it again! ALB 7/11/97
#define AUTOFRONT true

#pragma segment ListView
static short gAddCell, gAddLevel;
static OSType gDragFlavor;

#define ArrowLeft pView->details.arrowLeft
#define IconTop pView->details.iconTop
#define IconLeft pView->details.iconLeft
#define IconLevelWidth pView->details.iconLevelWidth
#define TextBottom pView->details.textBottom
#define RowHt pView->details.rowHt
#define NameAddMargin pView->details.nameAddMargin
#define MaxNameWidth pView->details.maxNameWidth
#define KeyNavTicks pView->details.keyNavTicks

#define	COLLAPSED_ICON 3060
#define EXPANDED_ICON 3061

//      Drag info needed for list clickloop
typedef struct {
	ViewListPtr pView;
	EventRecord *event;
	Point pt;
	short clickLoopCount;
	Boolean dragHilited;
	DragReference drag;
} DragInfo;

//      Prototypes
static void AdjustPeteSize(ViewListPtr pView, Rect * rErase);
static void BringWinToFront(ViewListPtr pView);
static long CallBack(ViewListPtr pView, short message, long data);
static void CheckFillList(ViewListPtr pView);
static void CollapseDragExpanded(ViewListPtr pView, short descendent,
				 DragReference drag);
static void CopyCellInfo(VLNodeInfo * pInfo, CellRec * pCellData,
			 short rowNum);
static void CopyNodeInfo(CellRec * pCellData, VLNodeInfo * pInfo);
static short CountChildren(ViewListPtr pView, short item);
static void DelayTicks(long tickCount);
static void DisposeDragRgn(RgnHandle rgn, GWorldPtr gworld,
			   RgnHandle hDragRgn);
static void DoDraw(ViewListPtr pView);
static void DontDraw(ViewListPtr pView);
static void DragHilite(ViewListPtr pView, short row, Boolean hilite);
static void DragSelect(ViewListPtr pView, Point initPt);
static void DrawItem(ViewListPtr pView, short row, Rect * pRect,
		     CellRec * pCellData, Boolean select,
		     Boolean eraseName);
static void DrawPete(ViewListPtr pView);
static void DrawRow(ViewListPtr pView, short row, Boolean hilite);
static CellRec *FindByName(ViewListPtr pView, VLNodeID id, StringPtr sName,
			   short *pRow);
static CellRec *FindByNodeID(ViewListPtr pView, VLNodeID id, short *pRow);
static CellRec *FindCellData(ViewListPtr pView, short item);
static void ClipMax(void);
static void DrawList(ViewListPtr pView, Rect * pRect);
static void FinishRename(ViewListPtr pView, Boolean fRename);
static short GetCellData(ViewListPtr pView, short item,
			 CellRec * pCellData);
static void AdjustSubsequentCells(ListHandle hList,
				  long followingThisOffset, long delta);
static void GetCellRects(ViewListPtr pView, CellRec * pCellData,
			 Rect * pRect, Rect * pTriangle, Rect * pIcon,
			 Rect * pName, Point * pText);
static Boolean GetDragRgn(ViewListPtr pView, RgnHandle * rgn,
			  GWorldPtr * gworld, RgnHandle * hDrawRgn,
			  DragReference drag, short row);
static short GetParent(ViewListPtr pView, short rowNum);
static void InitRename(ViewListPtr pView, Rect * pRect, UPtr name,
		       short rowNum);
static Boolean IsDragExpanded(short rowNum);
static short InWhichCell(Point pt, ListHandle hList);
static void KeyNavigate(ViewListPtr pView, StringPtr sKeyNavigation);
static Boolean ListDrag(ViewListPtr pView, EventRecord * event, Point pt);
static pascal Boolean MyListClickLoop(void);
static void PlotTriangle(Rect * pRect, short index);
static void PlotTheIcon(short iconID, Rect * pRect, Boolean selected,
			IconAlignmentType align);
static Boolean QueryItem(ViewListPtr pView, short query, short item);
#if AUTOFRONT
static void RestoreToLayer(ViewListPtr pView, Boolean fUpdate);
#endif
static Boolean SelectOneItem(ViewListPtr pView, short row);
static void SelectItem(ViewListPtr pView, short item, Boolean fSelect,
		       Boolean dontUpdate);
static OSErr SetCellData(ViewListPtr pView, short item,
			 CellRec * pCellData);
static void SetCollapseStatus(ViewListPtr pView, CellRec * pCellData,
			      short rowNum, Boolean fCollapse);
static pascal OSErr ListViewSend(FlavorType flavor, void *dragSendRefCon,
				 ItemReference theItemRef,
				 DragReference drag);
static void SetWaitExpandRow(ViewListPtr pView, short rowNum);
static void ShowListDragHilite(ViewListPtr pView, DragReference drag);
static void SizePete(ViewListPtr pView, Rect * pRect);
static Boolean TrackTriangle(Rect * prTriangle, Boolean collapsed);
static void UpdateSelection(ViewListPtr pView);
static SignedByte LockCells(ViewListPtr pView);
static SignedByte LockUserHandle(ViewListPtr pView);
static void RestoreCells(ViewListPtr pView, SignedByte state);
static void RestoreUserHandle(ViewListPtr pView, SignedByte state);
static void CheckAutoSelect(ViewListPtr pView, short row);

static Boolean DrawRowCallback(ViewListPtr pView, short item, Rect * pRect,
			       CellRec * pCellData, Boolean select,
			       Boolean eraseName);
static Boolean FillBlankCallback(ViewListPtr pView);
static Boolean GetCellRectsCallBack(ViewListPtr pView, CellRec * cellData,
				    Rect * cellRect, Rect * iconRect,
				    Rect * nameRect);
static Boolean InterestingClickCallback(ViewListPtr pView,
					CellRec * cellData,
					Rect * cellRect, Point pt);

//      Globals
static Boolean gDragFromMe, gDontDragHilite;
static DragInfo gDragInfo;
static short gDragExpandLevel;
static short gWaitExpandRow;	//      This row is a folder about to auto expand
static Boolean gDidDrag;	//      Did we do a drag on the click?
#if AUTOFRONT
static WindowPtr gwSendBehind;
#endif
static Boolean gAutoSelect;

static Handle ghDrawExpandedRows;	//      RowNum (1-based) of drag-expanded row at each level
static Boolean gOpenAfterRename;

/**********************************************************************
 * LVNew - Open a new list view, use standard list view sizes.
 **********************************************************************/
OSErr LVNew(ViewListPtr pView, MyWindowPtr wPtr, Rect * pBounds,
	    VLNodeID rootNodeID, ViewListCallBackPtr pCallBack,
	    OSType dragFlavor)
{
	DrawDetailsStruct details;

	// Use the standard sizes for this list view ...
	LVInitDetails(&details);
	return (LVNewWithDetails
		(pView, wPtr, pBounds, rootNodeID, pCallBack, dragFlavor,
		 &details));
}

/**********************************************************************
 * LVNewWithDetails - Open a new list view
 **********************************************************************/
OSErr LVNewWithDetails(ViewListPtr pView, MyWindowPtr wPtr, Rect * pBounds,
		       VLNodeID rootNodeID, ViewListCallBackPtr pCallBack,
		       OSType dragFlavor, DrawDetailsPtr details)
{
	Rect rDataBounds, rBounds;
	Cell cSize;
	ListHandle hList;
	DECLARE_UPP(ListViewListDef, ListDef);

	INIT_UPP(ListViewListDef, ListDef);
	WriteZero(pView, sizeof(ViewList));
	pView->wPtr = wPtr;
	pView->bounds = rBounds = *pBounds;
	rBounds.right -= GROW_SIZE;	//      Don't include scroll bar in parameter to LNew
	pView->rootNodeID = rootNodeID;
	pView->pCallBack = pCallBack;
	pView->details = *details;
	SetRect(&rDataBounds, 0, 0, 1, 0);
	pView->dragFlavor = dragFlavor;
	pView->font = SmallSysFontID();
	pView->fontSize = SmallSysFontSize();
	pView->listFocus = kFocusNone;
	cSize.v = RowHt;
	cSize.h = 0;
	if (pView->hList = hList =
	    CreateNewList(ListViewListDefUPP, LISTVIEW_LDEF, pBounds,
			  &rDataBounds, cSize, GetMyWindowWindowPtr(wPtr),
			  False, False, False, True)) {
		(*hList)->indent.v = 4;
		(*hList)->refCon = (long) pView;
		pView->hSelectList = NuHTempOK(0);
		pView->flags = CallBack(pView, kLVGetFlags, 0);

		if (pView->flags & kfSupportsMondoBigList)
			(*hList)->userHandle = NuHTempOK(0);

		if (pView->flags & kfSingleSelection)
			(*hList)->selFlags |= lOnlyOne;

		wPtr->pView = pView;

		//      Add the first level
		InvalidListView(pView);
	}

	return hList == 0;
}

/**********************************************************************
 * LVInitDetails - initialize details fields
 **********************************************************************/
void LVInitDetails(DrawDetailsPtr details)
{
	Zero(*details);
	details->arrowLeft = 3;
	details->iconTop = 1;
	details->iconLeft = details->arrowLeft;	//      First icon level
	details->iconLevelWidth = 20;
	details->textBottom = 5;
	details->rowHt = 18;
	details->nameAddMargin = 2;	//      Add to name width when drawing
	details->maxNameWidth = 160;	//      Approximate maximum name width
	details->keyNavTicks = 60;	//      Delay accepted between navigation keys
}

/**********************************************************************
 * LVDispose - Dispose of list view
 **********************************************************************/
void LVDispose(ViewListPtr pView)
{
	if (pView->pte)
		FinishRename(pView, true);

	if (pView->hList) {
		ZapHandle((*pView->hList)->userHandle);
		LDispose(pView->hList);
		pView->hList = nil;
	}
	if (pView->hSelectList) {
		ZapHandle(pView->hSelectList);
		pView->hSelectList = 0;
		pView->selectCount = 0;
	}
}

/**********************************************************************
 * LVAdd - Add a list view item
 * 	�� This should only be called in response to a kLVAddNodeItems request ��
 **********************************************************************/
OSErr LVAdd(ViewListPtr pView, VLNodeInfo * pInfo)
{
	CellRec cellData;
	OSErr theError;
	short theRow;

	theError = noErr;

	//      Add the list item
	if (!(pView->flags & kfManualRowAddsExpected))
		DontDraw(pView);
	if (pView->flags & kfManualRowAddsExpected)
		theRow = gAddCell;
	else
		theRow = LAddRow(1, gAddCell, pView->hList);

	//      Save the cell data
	cellData.iconID = pInfo->iconID;
	cellData.nodeID = pInfo->nodeID;
	cellData.misc.fParent = pInfo->isParent;
	cellData.misc.fCollapsed = pInfo->isCollapsed;
	cellData.misc.fHaveChildren = false;
	cellData.misc.level = pInfo->useLevelZero ? 0 : gAddLevel;
	cellData.misc.style = pInfo->style;
	cellData.refCon = pInfo->refCon;
	PCopy(cellData.name, pInfo->name);

	theError = SetCellData(pView, theRow, &cellData);

	gAddCell++;
	if (gAutoSelect)
		SelectItem(pView, theRow + 1, true, false);
	if (!(pView->flags & kfManualRowAddsExpected))
		DoDraw(pView);
	return (theError);
}

/**********************************************************************
 * InvalidListView - Regenerate the list
 **********************************************************************/
void InvalidListView(ViewListPtr pView)
{
	short saveTop;
	ListHandle hList = pView->hList;
	GrafPtr savePort;
//              Rect    rDraw;

	GetPort(&savePort);
	SetPort(GetMyWindowCGrafPtr(pView->wPtr));
	gAddCell = 0;
	gAddLevel = 1;
	saveTop = (*hList)->visible.top;
	DontDraw(pView);

	LDelRow(REAL_BIG, 0, hList);
	if (pView->flags & kfSupportsMondoBigList)
		SetHandleBig((*hList)->userHandle, 0);

	gAutoSelect = pView->flags & kfAutoSelectAll != 0;
	CallBack(pView, kLVAddNodeItems, pView->rootNodeID);
	CheckFillList(pView);
	gAutoSelect = false;
	if (saveTop) {
		LScroll(0, saveTop, hList);
	}
	UpdateSelection(pView);
	InvalWindowRect(GetMyWindowWindowPtr(pView->wPtr), &pView->bounds);
	SetPort(savePort);
	DoDraw(pView);
}

/**********************************************************************
 * AdjustPeteSize - Adjust the size of the edit region based on the size
 *	of the string and the width of the window
 **********************************************************************/
static void AdjustPeteSize(ViewListPtr pView, Rect * rErase)
{
	Str63 s;
	Rect r, rCell;
	SAVE_STUFF;
	SET_COLORS;

	PeteStringLo(s, sizeof(s), pView->pte);
	if (*s > 31)
		*s = 31;
	PeteRect(pView->pte, &r);
	r.right = r.left + StringWidth(s) + 8;
	SizePete(pView, &r);
	if (rErase) {
		rErase->left = r.right + 1;
		EraseRect(rErase);
	} else {
		Cell1Rect(pView->hList, pView->renameRow, &rCell);
		rCell.left = r.right + 1;
		EraseRect(&rCell);	//      Erase if Pete shrinks
	}
	DrawPete(pView);
	REST_STUFF;
}

/**********************************************************************
 * LVKey - Key for list view
 **********************************************************************/
Boolean LVKey(ViewListPtr pView, EventRecord * event)
{
	Boolean fResult = true;
	Boolean fUpdate = false;
	short key = (event->message & 0xff);
	ListHandle hList = pView->hList;
	Str63 s;
	short row;
	static Str32 sKeyNavigation;
	static unsigned long lastKeyTicks;
	unsigned long ticks = 0;

	if (pView->pte) {
		//      Renaming a file
		Boolean isDirtyKey;
		long start, stop;

		if (event->modifiers & cmdKey) {
			long select;
			if ((select = MyMenuKey(event))
			    && ((select >> 16) & 0xffff) == EDIT_MENU) {
				//      May have done paste or undo which
				//      require resizing the edit field
				DoMenu(GetMyWindowWindowPtr(pView->wPtr),
				       select, event->modifiers);
				AdjustPeteSize(pView, nil);
				return true;
			}
			return (false);
		}

		switch (key) {
		case returnChar:
		case enterChar:
		case tabChar:
			FinishRename(pView, true);
			break;

		case escChar:
			FinishRename(pView, false);
			break;

		default:
			PeteStringLo(s, 33, pView->pte);
			isDirtyKey = DirtyKey(event->message);
			//      Don't try to insert another character if the name is
			//      already at 31 characters.
			PeteGetTextAndSelection(pView->pte, nil, &start,
						&stop);
			if (*s >= 31 && isDirtyKey && start == stop
			    && key != backSpace && key != delChar) {
				//      Oops, file name too long
			} else {
				Rect rTemp, rErase;
				if (isDirtyKey) {
					//      Make sure field is wide enough to insert a wide character
					PeteRect(pView->pte, &rTemp);
					SetRect(&rErase, rTemp.left,
						rTemp.top - 1,
						rTemp.right + 1,
						rTemp.bottom + 1);
					rTemp.right += 12;
					SizePete(pView, &rTemp);
				}
				PeteEdit(pView->wPtr, pView->pte, peeEvent,
					 event);
				if (isDirtyKey) {
					PeteRect(pView->pte, &rTemp);
					AdjustPeteSize(pView, &rErase);
					rTemp.right += 20;
					ValidWindowRect
					    (GetMyWindowWindowPtr
					     (pView->wPtr), &rTemp);
				}
			}
			break;
		}
		return true;
	}

	if (ListApp1(event, hList))	//      Check for page commands
		fUpdate = true;
	else {
		SelectedListHandle hSelectList;
		SelectedListPtr pSelectList;
		short i, count, bottom;
		CellRec cellData;
		Boolean fCollapse;

		hSelectList = pView->hSelectList;
		pSelectList = *hSelectList;
		count = pView->selectCount;
		bottom = (*hList)->dataBounds.bottom;
		if (event->modifiers & cmdKey) {
			//      Command key down
			switch (key) {
			case leftArrowChar:
			case rightArrowChar:
				//      Expand or collapse selected items
				fCollapse = key == leftArrowChar;
				for (i = 0; i < pView->selectCount; i++) {
					row = (*hSelectList)[i].row;
					GetCellData(pView, row, &cellData);
					if (cellData.misc.fParent
					    && (cellData.misc.fCollapsed !=
						fCollapse)) {
						SetCollapseStatus(pView,
								  &cellData,
								  row + 1,
								  fCollapse);
						CheckFillList(pView);
						UpdateSelection(pView);
					}
				}
				break;

			case upArrowChar:
				row = count ? pSelectList[0].row - 1 : 0;
				for (; row >= 0; --row) {
					GetCellData(pView, row, &cellData);
					if (cellData.misc.fParent) {
						SelectOneItem(pView, row);
						break;
					}
				}
				break;
			case downArrowChar:
				row =
				    count ? pSelectList[count - 1].row +
				    1 : 0;
				for (; row < bottom; ++row) {
					GetCellData(pView, row, &cellData);
					if (cellData.misc.fParent) {
						SelectOneItem(pView, row);
						break;
					}
				}
				break;

			default:
				fResult = false;
				break;
			}
		} else {
			switch (key) {
			case upArrowChar:
				row = count ? pSelectList[0].row : 1;
				if (row >= 1)
					row--;
			      SetSelection:
				SelectOneItem(pView, row);;
				break;

			case downArrowChar:
				row =
				    count ? pSelectList[count -
							1].row : bottom -
				    1;
				if (row < bottom - 1)
					row++;
				goto SetSelection;

			case returnChar:
			case enterChar:
				CallBack(pView, kLVOpenItem, 0);
				break;

			case backSpace:
			case delChar:
				CallBack(pView, kLVDeleteItem,
					 dataDeleteFromKeyboard);
				break;

			default:
				//      Do key navigation
				if (key >= ' ') {
					ticks = TickCount();
					if (ticks >
					    lastKeyTicks + KeyNavTicks)
						//      Start over
						*sKeyNavigation = 0;

					if (*sKeyNavigation < 31) {
						sKeyNavigation
						    [++*sKeyNavigation] =
						    key;
						KeyNavigate(pView,
							    sKeyNavigation);
					}
				} else
					fResult = false;
				break;
			}
		}
	}

	lastKeyTicks = ticks;
	if (fResult)
		UpdateSelection(pView);
	return fResult;
}

/**********************************************************************
 * LVClick - Click on list view
 **********************************************************************/
Boolean LVClick(ViewListPtr pView, EventRecord * event)
{
	Boolean fDblClick;
	short rowNum;
	ListHandle hList;
	Point pt;
	Boolean fResult = false;
	GrafPtr savePort;
	ListFocusType newFocus = pView->listFocus;
	SAVE_STUFF;
	SET_COLORS;

	GetPort(&savePort);
	SetPort(GetMyWindowCGrafPtr(pView->wPtr));
	pt = event->where;
	GlobalToLocal(&pt);
	gDidDrag = false;

	if (pView->pte && PtInPETE(pt, pView->pte)) {
		PeteEdit(pView->wPtr, pView->pte, peeEvent, event);
		fResult = true;
	} else {
		ControlRef vScroll;
		Rect rScroll;

		if (pView->pte)
			FinishRename(pView, true);

		hList = pView->hList;
		vScroll = (*hList)->vScroll;
		GetControlBounds(vScroll, &rScroll);
		if (PtInRect(pt, &rScroll)) {
			//      Click in scroll bar
			BringWinToFront(pView);
			MyLClick(pt, event->modifiers, pView->hList);
			UpdateSelection(pView);
			fResult = true;
		} else if (PtInRect(pt, &pView->bounds)) {
			rowNum = InWhichCell(pt, hList);

			newFocus = kFocusList;
			if (rowNum) {
				Point ptName;
				CellRec cellData;
				Rect rTriangle, rIcon, rName, rCell;

				Cell1Rect(hList, rowNum, &rCell);
				GetCellData(pView, rowNum - 1, &cellData);
				GetCellRects(pView, &cellData, &rCell,
					     &rTriangle, &rIcon, &rName,
					     &ptName);

				//       Check for click in triangle
				if (cellData.misc.fParent &&
				    PtInRect(pt, &rTriangle)) {
					if (TrackTriangle
					    (&rTriangle,
					     cellData.misc.fCollapsed)) {
						//      Toggle triangle
						SetCollapseStatus(pView,
								  &cellData,
								  rowNum,
								  !cellData.
								  misc.
								  fCollapsed);
					}
				}
				//      Check for click in icon or name
				else if (((*pView->details.
					   InterestingClickCallback)
					  &&
					  (InterestingClickCallback
					   (pView, &cellData, &rCell, pt))
					  || PtInRect(pt, &rIcon)
					  || PtInRect(pt, &rName))
					 && QueryItem(pView, kQuerySelect,
						      rowNum)) {
					ListClickLoopUPP saveClickLoop;
					DECLARE_UPP(MyListClickLoop,
						    ListClickLoop);

					INIT_UPP(MyListClickLoop,
						 ListClickLoop);
					//      Set up a clickloop to check for drags. Don't leave cliploop function installed
					//      because we don't want it when doing LClick for the scroll bar
					saveClickLoop =
					    (*hList)->lClickLoop;
					(*hList)->lClickLoop =
					    MyListClickLoopUPP;

					//      Set up data needed for clickloop since cliploop receives no parameters
					gDragInfo.pView = pView;
					gDragInfo.event = event;
					gDragInfo.pt = pt;
					gDragInfo.clickLoopCount = 0;
					gDragInfo.dragHilited = true;
					gDragInfo.drag = nil;
					fDblClick =
					    MyLClick(pt, event->modifiers,
						     pView->hList);
					(*hList)->lClickLoop =
					    saveClickLoop;
					if (!gDidDrag) {
						Cell lastCell =
						    LLastClick(hList);
						CheckAutoSelect(pView,
								lastCell.
								v);
					}
					UpdateSelection(pView);
					if (!gDidDrag) {
						BringWinToFront(pView);

						if (fDblClick) {
							//      Double click. 
							if (QueryItem
							    (pView,
							     kQueryDCOpens,
							     rowNum))
								//      Open the selected item(s)
								CallBack
								    (pView,
								     kLVOpenItem,
								     0);
							else if (cellData.
								 misc.
								 fParent)
								//      Folder. Toggle collapse status
								SetCollapseStatus
								    (pView,
								     &cellData,
								     rowNum,
								     !cellData.
								     misc.
								     fCollapsed);
						} else
						    if (PtInRect
							(pt, &rName)
							&& pView->
							selectCount == 1
							&& QueryItem(pView,
								     kQueryRename,
								     rowNum))
						{
							//      Rename this item
							long startTicks;
							Boolean
							    fAbortRename =
							    false;
							//      Let's wait around for the time of a double-click to see if the
							//      user is going to double click
							for (startTicks =
							     TickCount();
							     TickCount() <
							     startTicks +
							     GetDblTime();)
							{
								EventRecord
								    theEvent;

								if (EventAvail(mDownMask + keyDownMask, &theEvent)) {
									//      Mouse or key event. We don't want to rename
									fAbortRename
									    =
									    true;
									break;
								}
							}
							if (!fAbortRename) {
								InitRename
								    (pView,
								     &rName,
								     cellData.
								     name,
								     rowNum);
								newFocus =
								    kFocusEdit;
							}
						}
					}
				} else {
					//      Click on nothing.
					if (!
					    (event->
					     modifiers & (shiftKey +
							  cmdKey))) {
						//      Select nothing if no shift or command key key
						SelectOneItem(pView, -1);
					}

					BringWinToFront(pView);

					//      Check for drag selection
					if (!
					    (pView->
					     flags & kfSingleSelection))
						if (MyWaitMouseMoved
						    (event->where, True)) {
							DragSelect(pView,
								   pt);
						}
				}
			} else
				BringWinToFront(pView);

			CheckFillList(pView);
			fResult = true;
		}
	}

	LVChangeFocus(pView, newFocus);

	SetPort(savePort);
	REST_STUFF;

	return fResult;
}

/**********************************************************************
 * LVDraw - Draw list view
 **********************************************************************/
void LVDraw(ViewListPtr pView, RgnHandle hRgn, Boolean checkList,
	    Boolean fDrawFrame)
{
	WindowPtr pViewWPtrWP = GetMyWindowWindowPtr(pView->wPtr);
	Rect r;

	//      Make sure we have the data on all the cells currently being displayed
	if (checkList)
		CheckFillList(pView);

	//      Draw the list
	LUpdate(hRgn ? hRgn :
		MyGetPortVisibleRegion(GetWindowPort(pViewWPtrWP)),
		pView->hList);

	DrawPete(pView);

	//      Draw frame
	if (fDrawFrame) {
		r = pView->bounds;
		InsetRect(&r, -1, -1);
		FrameRect(&r);
	}

	LVDrawFocus(pView);

	// Fill the unused portion of the list with something
	if (pView->details.FillBlankCallback)
		FillBlankCallback(pView);

}

/**********************************************************************
 * LVActivate - Activate list view
 **********************************************************************/
void LVActivate(ViewListPtr pView, Boolean activate)
{
#pragma unused(activate)
	if (pView->pte)
		FinishRename(pView, true);
	LActivate(activate, pView->hList);
	LVDrawFocus(pView);
}

/**********************************************************************
 * LVSize - Size list view
 **********************************************************************/
void LVSize(ViewListPtr pView, Rect * pRect, short *pHeightAdjustment)
{
	WindowPtr pViewWPtrWP = GetMyWindowWindowPtr(pView->wPtr);
	Point cSize;
	Rect r, rPort;
	ControlHandle cntl;

	if (pView->pte)
		FinishRename(pView, true);

	r = *pRect;		//      May get modified. Make a copy.
	cSize.v = RowHt;
	cSize.h = r.right - r.left;

	//      Make height a multiple of the cell height
	if (pHeightAdjustment) {
		r.bottom = r.top + ((r.bottom - r.top) / RowHt) * RowHt;
		*pHeightAdjustment = r.bottom - pRect->bottom;
	}
	pView->bounds = r;
	r.right -= GROW_SIZE;	//      Don't include scroll bar in this measurement
	InsetRect(&(*pView->hList)->rView, 2, 2);
	ResizeList(pView->hList, &r, cSize);

	cntl = (*pView->hList)->vScroll;
	GetControlBounds(cntl, &r);
	GetPortBounds(GetWindowPort(pViewWPtrWP), &rPort);
	if (r.bottom > rPort.bottom - 14) {
		// scroll bar is covering up grow box. move it up
		r.bottom = rPort.bottom - 14;
		MySetCntlRect(cntl, &r);
	}

	if (pView->pte)
		AdjustPeteSize(pView, nil);
}

/**********************************************************************
 * LVCalcSize - Calculate the adjust size the list will be
 **********************************************************************/
void LVCalcSize(ViewListPtr pView, Rect * r)
{
#pragma unused(pView)

	//      Make height a multiple of the cell height
	r->bottom =
	    r->top + ((r->bottom - r->top + RowHt / 2) / RowHt) * RowHt;
}

/**********************************************************************
 * FindByName - find an item by nodeID and name, row is 1-based
 **********************************************************************/
static CellRec *FindByName(ViewListPtr pView, VLNodeID id, StringPtr sName,
			   short *pRow)
{
	short i;
	ListHandle hList = pView->hList;
	CellRec *pCellData = nil;
	Boolean found = false;

	// StringSame may move memory for international languages, so 'pCellData'
	// might be pointing to nowheresville when we return.  We'll need to make
	// sure the cells (and userhandle) are locked.
	SignedByte cellsState = LockCells(pView);
	SignedByte userHandleState = LockUserHandle(pView);
	for (i = 0; i < (*hList)->dataBounds.bottom && !found; i++) {
		pCellData = FindCellData(pView, i);
		if (id == pCellData->nodeID
		    && StringSame(sName, pCellData->name)) {
			*pRow = i + 1;
			found = true;
		}
	}
	RestoreCells(pView, cellsState);
	RestoreUserHandle(pView, userHandleState);
	return (found ? pCellData : nil);
}

/**********************************************************************
 * FindByNodeID - find an item by nodeID only, row is 1-based
 **********************************************************************/
static CellRec *FindByNodeID(ViewListPtr pView, VLNodeID id, short *pRow)
{
	short i;
	ListHandle hList = pView->hList;

	for (i = 0; i < (*hList)->dataBounds.bottom; i++) {
		CellRec *pCellData;

		pCellData = FindCellData(pView, i);
		if (id == pCellData->nodeID) {
			*pRow = i + 1;
			return pCellData;
		}
	}
	return nil;
}

/**********************************************************************
 * LVRename - allow renaming of an item
 **********************************************************************/
void LVRename(ViewListPtr pView, VLNodeID id, StringPtr sName,
	      Boolean openAfterRename, Boolean hahaOnlyKidding)
{
	CellRec *pCellData, cellData;
	short row;

	SignedByte cellsState = LockCells(pView);
	SignedByte userHandleState = LockUserHandle(pView);
	if (pCellData = FindByName(pView, id, sName, &row)) {
		Rect rCell, rTriangle, rIcon, rName;
		Point ptName;

		SelectOneItem(pView, row - 1);
		LAutoScroll(pView->hList);
		Cell1Rect(pView->hList, row, &rCell);
		if (rCell.bottom > pView->bounds.bottom) {
			LScroll(0, 1, pView->hList);
			Cell1Rect(pView->hList, row, &rCell);
		}
		if (!hahaOnlyKidding) {
			cellData = *pCellData;	// make a copy that can't move
			GetCellRects(pView, &cellData, &rCell, &rTriangle,
				     &rIcon, &rName, &ptName);
			InitRename(pView, &rName, sName, row);
		}
		gOpenAfterRename = openAfterRename;
	}
	RestoreCells(pView, cellsState);
	RestoreUserHandle(pView, userHandleState);
}

/**********************************************************************
 * LVSelect - select a list view item
 **********************************************************************/
Boolean LVSelect(ViewListPtr pView, VLNodeID id, StringPtr sName,
		 Boolean addToSelection)
{
	short row;
	Boolean result = false;

	if (FindByName(pView, id, sName, &row)) {
		if (addToSelection)
			SelectItem(pView, row, true, true);
		else
			result = SelectOneItem(pView, row - 1);
		UpdateSelection(pView);
	}
	return result;		//      Indicate if we selected anything
}

/**********************************************************************
 * LVUnselect - unselect a list view item (should be combined with LVSelect,
 *							but this is less code impact FTTB.
 **********************************************************************/
Boolean LVUnselect(ViewListPtr pView, VLNodeID id, StringPtr sName,
		   Boolean removeFromSelection)
{
	short row;
	Boolean result = false;

	if (FindByName(pView, id, sName, &row)) {
		if (removeFromSelection)
			SelectItem(pView, row, false, true);
		else
			result = SelectOneItem(pView, -1);
		UpdateSelection(pView);
	}
	return result;		//      Indicate if we selected anything
}

/**********************************************************************
 * LVExpand - expand/collapse a list view item
 **********************************************************************/
void LVExpand(ViewListPtr pView, VLNodeID id, StringPtr sName,
	      Boolean expand)
{
	short row;
	CellRec *pCellData;

	if (pCellData = FindByName(pView, id, sName, &row)) {
		DataHandle cellsHandle = (*pView->hList)->cells;
		Handle userHandle = (*pView->hList)->userHandle;

		DEC_STATE(cellsHandle)
		    DEC_STATE(userHandle)
		    L_STATE(cellsHandle);
		if (pView->flags & kfSupportsMondoBigList)
			L_STATE(userHandle);

		SetCollapseStatus(pView, pCellData, row, !expand);
		U_STATE(cellsHandle);
		if (pView->flags & kfSupportsMondoBigList)
			U_STATE(userHandle);

		if (expand) {
//                      DontDraw(pView);
			CheckFillList(pView);
//                      DoDraw(pView);
		}
	}
}

/**********************************************************************
 * LVUpdateStyle - update style for an item
 **********************************************************************/
void LVUpdateStyle(ViewListPtr pView, VLNodeID id, StringPtr sName,
		   Style theStyle, Boolean fDraw)
{
	short row;
	ListHandle hList = pView->hList;
	CellRec *pCellData;

	if (pCellData = FindByName(pView, id, sName, &row)) {
		//      Found it. Don't redraw unless it's different
		if (pCellData->misc.style != theStyle) {
			Cell c;

			pCellData->misc.style = theStyle;
			c.h = 0;
			c.v = row - 1;
			if (fDraw) {
				GrafPtr savePort;

				GetPort(&savePort);
				SetPort(GetMyWindowCGrafPtr(pView->wPtr));
				LDraw(c, hList);
				SetPort(savePort);
			}
		}
	}
}

/**********************************************************************
 * LVCountSelection - return number of items selected
 **********************************************************************/
short LVCountSelection(ViewListPtr pView)
{
	return pView->selectCount;
}

/**********************************************************************
 * GetListViewSelection - return the cell data for the indicated item
 *		item is 1-based from the list or from the selection
 **********************************************************************/
Boolean LVGetItem(ViewListPtr pView, short item, VLNodeInfo * pInfo,
		  Boolean fFromSelection)
{
	Boolean fResult;
	CellRec *pCellData;
	short row;

	fResult = false;
	if (fFromSelection) {
		//      Get a selected item
		if (item > 0 && item <= pView->selectCount
		    && pView->hSelectList)
			row = (*pView->hSelectList)[item - 1].row;
		else
			//      Invalid item
			return false;
	} else {
		//      Get the specified item
		if (item > 0 && item <= (*pView->hList)->dataBounds.bottom)
			row = item - 1;
		else
			//      Invalid item
			return false;
	}

	if (pCellData = FindCellData(pView, row)) {
		CopyCellInfo(pInfo, pCellData,
			     (*pView->hSelectList)[item - 1].row + 1);
		return true;
	}
	return false;
}

Boolean LVGetItemWithNodeID(ViewListPtr pView, VLNodeID nodeID,
			    VLNodeInfo * pInfo)
{
	CellRec *pCellData;
	short row;

	if (pCellData = FindByNodeID(pView, nodeID, &row)) {
		CopyCellInfo(pInfo, pCellData, row);
		return true;
	}
	return (false);
}

Boolean LVSetItemWithNodeID(ViewListPtr pView, VLNodeID nodeID,
			    VLNodeInfo * pInfo, Boolean update)
{
	CellRec *pCellData;
	Rect cellRect;
	short row;

	if (pCellData = FindByNodeID(pView, nodeID, &row)) {
		CopyNodeInfo(pCellData, pInfo);
		if (update)
			InvalWindowRect(GetMyWindowWindowPtr(pView->wPtr),
					Cell1Rect(pView->hList, row,
						  &cellRect));
		return true;
	}
	return (false);
}


#if AUTOFRONT
/************************************************************************
 * RestoreToLayer - restore the window to its proper layer if it was brought
 *		to the front.
 ************************************************************************/
static void RestoreToLayer(ViewListPtr pView, Boolean fUpdate)
{
	WindowPtr pViewWPtrWP = GetMyWindowWindowPtr(pView->wPtr);

	if (gwSendBehind) {
		//      Send the window back where it came from         
		//      Make sure the window we need to go behind is still in the window list. It
		//      make have closed by now.
		WindowPtr theWindow;

		for (theWindow = FrontWindow(); theWindow;
		     theWindow = GetNextWindow(theWindow)) {
			if (gwSendBehind == theWindow) {
				SendBehind(pViewWPtrWP, gwSendBehind);
				if (fUpdate) {
					EventRecord theEvent;

					while (CheckUpdate(&theEvent)
					       && theEvent.what ==
					       updateEvt) {
						UpdateMyWindow((WindowPtr) theEvent.message);	//      Have to update manually since no events are processed
					}
				}
				break;
			}
		}
	}
}
#endif

/************************************************************************
 * LVDrag - called by drag tracking and drag drop handler
 ************************************************************************/
OSErr LVDrag(ViewListPtr pView, DragTrackingMessage which,
	     DragReference drag)
{
	short rowNum;
	Point pt, pinnedPt;
	OSErr result;
	static short lastRow, lastExpanded;
	static long lastTicks, LastScrollTicks;
	static short lastScroll;
#if AUTOFRONT
	static long enterTicks;
#endif
	short expandingRow, scroll;
	Boolean fTimesUp;
	ListHandle hList;
	short dragExpandTicks = GetRLong(DRAG_EXPAND_TICKS);

	result = noErr;
	hList = pView->hList;

	switch (which) {
	case kDragTrackingEnterWindow:
		if (pView->pte)
			FinishRename(pView, true);
		pView->dragHiliteRow = 0;
		lastScroll = 0;
		gWaitExpandRow = 0;
		//              SelectOneItem(pView,-1);        //      Remove any selections
		if (!gDontDragHilite) {
			ShowListDragHilite(pView, drag);
		}
#if AUTOFRONT
		enterTicks = TickCount();
		gwSendBehind = nil;
#endif
		if (!gDragFromMe)
			SelectOneItem(pView, -1);	//      Get rid of any selection if dragging from outside
		break;

	case kDragTrackingLeaveWindow:
		DragHilite(pView, pView->dragHiliteRow, false);
		pView->dragHiliteRow = 0;
		SetWaitExpandRow(pView, 0);
		if (!gDontDragHilite)
			HideDragHilite(drag);
		CollapseDragExpanded(pView, 0, nil);
		gDontDragHilite = false;	//      Make sure we hilite next time coming into this window
#if AUTOFRONT
		RestoreToLayer(pView, true);
#endif
		break;

	case kDragTrackingInWindow:
		{
			Point ptLocal;
			Rect rContent;

			GetDragMouse(drag, &pt, &pinnedPt);
			GlobalToLocal(&pt);

			do {
				rowNum = InWhichCell(pt, hList);

				if (lastRow != rowNum) {
					lastTicks = TickCount();
					lastRow = rowNum;
				}
				//      Check for drag collapse
				if (rowNum)
					CollapseDragExpanded(pView, rowNum,
							     drag);

				if (rowNum) {
					CellRec *pCellData = nil;
					SignedByte cellsState =
					    LockCells(pView);
					SignedByte userHandleState =
					    LockUserHandle(pView);
					Boolean canDragExpand, fParent;

					//      Don't hilite unless over icon, name or triangle                 
					if (pCellData =
					    FindCellData(pView,
							 rowNum - 1)) {
						Rect rCell, rTriangle,
						    rIcon, rName;
						Point ptName;

						Cell1Rect(hList, rowNum,
							  &rCell);
						GetCellRects(pView,
							     pCellData,
							     &rCell,
							     &rTriangle,
							     &rIcon,
							     &rName,
							     &ptName);
						if (!
						    (PtInRect
						     (pt, &rTriangle)
						     || PtInRect(pt,
								 &rIcon)
						     || PtInRect(pt,
								 &rName)))
							rowNum = 0;
					}
					//      Can't drag onto something that's already selected if we're dragging from ourself
					if (rowNum && gDragFromMe
					    && Cell1Selected(rowNum,
							     hList))
						rowNum = 0;

					expandingRow = gWaitExpandRow;

					//      If it's a folder, see if we can do drag expand
					fParent = pCellData->misc.fParent;
					canDragExpand = rowNum && fParent
					    && pCellData->misc.fCollapsed
					    && !IsDragExpanded(rowNum)
					    && QueryItem(pView,
							 kQueryDragExpand,
							 rowNum);

					if (canDragExpand) {
						if (TickCount() < lastTicks + dragExpandTicks)	//      Short delay before expanding
						{
							//      Don't expand yet. Rotate the triangle 45 degrees.
							expandingRow =
							    rowNum;
							GetMouse(&pt);
						} else {
							//      Time to expand it
							if (!ghDrawExpandedRows)
								ghDrawExpandedRows
								    =
								    NuHandle
								    (0);
							if (ghDrawExpandedRows) {
								HideDragHilite
								    (drag);
								SetCollapseStatus
								    (pView,
								     pCellData,
								     rowNum,
								     false);
								PtrAndHand
								    (&rowNum,
								     ghDrawExpandedRows,
								     sizeof
								     (rowNum));

								// The userhandle MUST be unlocked prior to calling CheckFillList during a drag
								// expand because we'll be adding cells
								if (pView->
								    flags &
								    kfSupportsMondoBigList)
									RestoreUserHandle
									    (pView,
									     userHandleState);
								CheckFillList
								    (pView);
								ShowListDragHilite
								    (pView,
								     drag);
							}
							//                                              rowNum = 0;
							pView->
							    dragHiliteRow =
							    0;
							expandingRow = 0;
						}
					}
					RestoreCells(pView, cellsState);
					RestoreUserHandle(pView,
							  userHandleState);

					if (expandingRow
					    && expandingRow != rowNum)
						expandingRow = 0;

					SetWaitExpandRow(pView,
							 expandingRow);

					//      See if we can drag onto this
					if (rowNum
					    && !QueryItem(pView,
							  kQueryDrop,
							  rowNum)) {
						//      Can't drop on this. Can we drop on the parent?
						if (!fParent
						    && QueryItem(pView,
								 kQueryDropParent,
								 rowNum)) {
							//      Drop on the parent
							rowNum =
							    GetParent
							    (pView,
							     rowNum);
						} else
							//      Can't drop on this one
							rowNum = 0;
					}
				}

				if (pView->dragHiliteRow != rowNum) {
					DragHilite(pView,
						   pView->dragHiliteRow,
						   false);
					DragHilite(pView, rowNum, true);
					pView->dragHiliteRow = rowNum;
				}
				//      See if we need to auto scroll
				fTimesUp =
				    TickCount() - LastScrollTicks >= 12;
				scroll = 0;
				if (((pt.v < pView->bounds.top && fTimesUp) || pt.v < pView->bounds.top - 16) && (*hList)->visible.top)	//      Scroll only if we're not already at the top
					scroll = -1;
				else if (((pt.v > pView->bounds.bottom && fTimesUp) || pt.v > pView->bounds.bottom + 16) && (*hList)->visible.bottom < (*hList)->dataBounds.bottom)	//      Scroll only if we're not already at the bottom                  
					scroll = 1;

				if (scroll) {
					if (lastScroll
					    && lastScroll != scroll) {
						//      There appears to be a bug in DragPostScroll if you
						//      switch directions on the scroll. Hiding and showing
						//      the drag hilite clears up the problem
						HideDragHilite(drag);
						LScroll(0, scroll, hList);
						ShowListDragHilite(pView,
								   drag);
					} else {
						DragPreScroll(drag, 0,
							      -RowHt *
							      scroll);
						LScroll(0, scroll, hList);
						DragPostScroll(drag);
					}
					LastScrollTicks = TickCount();
					lastScroll = scroll;
				}
				// The dragTrackingInWindow message is sent only while the mouse is moving.
				// To support auto scrolling, drag expand, we need to keep looping here as long as the mouse
				// is inside the window and not in the list
				GetMouse(&ptLocal);
				GetWindowBounds(GetMyWindowWindowPtr
						(pView->wPtr),
						kWindowContentRgn,
						&rContent);
			} while (EqualPt(ptLocal, pt) && StillDown());

#if AUTOFRONT
			//      Check if we need to bring the mailboxes window to the front
			if (!gwSendBehind
			    && (TickCount() - enterTicks >= 60)
			    && RunType == Debugging) {
				WindowPtr pViewWPtrWP =
				    GetMyWindowWindowPtr(pView->wPtr);
				gwSendBehind = nil;
				if ((FrontWindow()) != pViewWPtrWP) {
					//      Find the window in front of us  
					for (gwSendBehind = FrontWindow();
					     gwSendBehind;
					     gwSendBehind =
					     GetNextWindow(gwSendBehind)) {
						if (GetNextWindow
						    (gwSendBehind) ==
						    pViewWPtrWP) {
							HideDragHilite
							    (drag);
							BringToFront
							    (pViewWPtrWP);
							UpdateMyWindow(pViewWPtrWP);	//      Have to update manually since no events are processed
							ShowListDragHilite
							    (pView, drag);
							break;
						}
					}
				}
				enterTicks = positiveInfinity;
			}
#endif
		}
		break;

	case 0xfff:
		//      Drop
#if AUTOFRONT
		RestoreToLayer(pView, false);
#endif
		if (pView->dragHiliteRow || (pView->flags & kfDropRoot)) {
			CellRec *pCellData;
			VLNodeInfo info;

			if (pView->dragHiliteRow) {
				if (pCellData =
				    FindCellData(pView,
						 pView->dragHiliteRow - 1))
					CopyCellInfo(&info, pCellData,
						     pView->dragHiliteRow);
				else
					goto DropFail;
			} else {
				// Drag to root level
				Zero(info);
				info.nodeID = pView->rootNodeID;
				info.isParent = true;
			}

			if (gDragFromMe) {
				//      Internal drag/drop
				short modifiers, mouseDownModifiers,
				    mouseUpModifiers;

				GetDragModifiers(drag, &modifiers,
						 &mouseDownModifiers,
						 &mouseUpModifiers);
				CallBack(pView,
					 (modifiers | mouseDownModifiers |
					  mouseUpModifiers) & optionKey ?
					 kLVCopyItem : kLVMoveItem,
					 (long) &info);
			} else {
				//      Drop from external source
				SendDragDataInfo dragInfo;

				dragInfo.drag = drag;
				dragInfo.info = &info;
				CallBack(pView, kLVDragDrop,
					 (long) &dragInfo);
			}
		      DropFail:
			if (pView->dragHiliteRow) {
				DragHilite(pView, pView->dragHiliteRow,
					   false);
				pView->dragHiliteRow = 0;
			}
		} else
			result = dragNotAcceptedErr;
		CollapseDragExpanded(pView, 0, nil);
		SetWaitExpandRow(pView, 0);
		break;
	}

	return result;
}

/************************************************************************
 * LVDrop - finish up drag and return item dropped on
 ************************************************************************/
Boolean LVDrop(ViewListPtr pView, VLNodeInfo * pInfo)
{
#if AUTOFRONT
	RestoreToLayer(pView, false);
#endif
	if (pView->dragHiliteRow) {
		CellRec *pCellData;

		if (pCellData =
		    FindCellData(pView, pView->dragHiliteRow - 1)) {
			CopyCellInfo(pInfo, pCellData,
				     pView->dragHiliteRow);
		}
		DragHilite(pView, pView->dragHiliteRow, false);
		pView->dragHiliteRow = 0;
		return true;
	}
	return false;
}

/************************************************************************
 * LVDescendant - see if "check" is a descendent of "in".
 *		Return 0 if not or generation if it is
 ************************************************************************/
short LVDescendant(ViewListPtr pView, VLNodeInfo * checkInfo,
		   VLNodeInfo * inInfo)
{
	CellRec *pInCellData;
	short row;

	if (pInCellData =
	    FindByName(pView, inInfo->nodeID, inInfo->name, &row)) {
		short level = pInCellData->misc.level;
		ListHandle hList = pView->hList;
		short i;

		for (i = row; i < (*hList)->dataBounds.bottom; i++) {
			CellRec *pCellData;

			if (pCellData = FindCellData(pView, i)) {
				short descendLevel;

				descendLevel =
				    pCellData->misc.level - level;
				if (descendLevel <= 0)
					return 0;	//      Not a descendent
				if (pCellData->nodeID == checkInfo->nodeID
				    && StringSame(pCellData->name,
						  checkInfo->name))
					return descendLevel;
			}
		}
	}
	return 0;
}

/************************************************************************
 * LVMaxSize - get the maximum size of the list
 ************************************************************************/
void LVMaxSize(ViewListPtr pView, short *pWd, short *pHt)
{
	*pWd = pView->maxNameWidth + GROW_SIZE;
	*pHt =
	    (*pView->hList)->cellSize.v *
	    (*pView->hList)->dataBounds.bottom;
}

/************************************************************************
 * LVDragFlavor - return the drag flavor if we are dragging
 ************************************************************************/
OSType LVDragFlavor(void)
{
	return gDragFlavor;
}

/**********************************************************************
 * LVIsSelected - check if a row is in the list of selected items (row is 1-based)
 **********************************************************************/
Boolean LVIsSelected(ViewListPtr pView, short row)
{
	SelectedListPtr pSelectedList;
	short i;

	for (i = 0, pSelectedList = *pView->hSelectList;
	     i < pView->selectCount; i++, pSelectedList++)
		if (pSelectedList->row == row - 1)
			return (true);
	return false;
}

/**********************************************************************
 * LVSelectAll - select all items in list (at least all those that allow selection)
 **********************************************************************/
Boolean LVSelectAll(ViewListPtr pView)
{
	short row;
	short bottom = (*pView->hList)->dataBounds.bottom;

	if (pView->pte)
		return false;	// select all while renaming selects all text in item

	for (row = 1; row <= bottom; row++)
		SelectItem(pView, row, true, true);
	UpdateSelection(pView);
	return true;
}


/***************************************************************
 *
 * To add an LDEF do:
 * - add "data 'LDEF'" in Common.r
 * - add enum to LDEFEnum in MyRes.h
 * - add doInitDEF and extern declaration in "ends.c"
 *
 ***************************************************************/
/************************************************************************
 * list definition proc
 ************************************************************************/
pascal void ListViewListDef(short message, Boolean select, Rect * pRect,
			    Cell cell, short dataOffset, short dataLen,
			    ListHandle hList)
{
	ViewListPtr pView;
	CellRec cellData;
	WindowPtr theWindow = GetWindowFromPort(GetQDGlobalsThePort());

	switch (message) {
	case lDrawMsg:
	case lHiliteMsg:
		if (dataLen) {
			SAVE_STUFF;
			SET_COLORS;
			pView = (ViewListPtr) (*hList)->refCon;
			if (pView->flags & kfSupportsMondoBigList) {
				long userHandleOffset;
				short len;

				BlockMoveData(*(*hList)->cells +
					      dataOffset,
					      &userHandleOffset, dataLen);
				len = offsetof(CellRec, name) + 1;	// Includes the length byte of the name
				BlockMoveData(*(*hList)->userHandle +
					      userHandleOffset, &cellData,
					      len);
				BlockMoveData(*(*hList)->userHandle +
					      userHandleOffset + len,
					      &cellData.name[1],
					      cellData.name[0]);
			} else
				BlockMoveData(*(*hList)->cells +
					      dataOffset, &cellData,
					      dataLen);
			EraseRect(pRect);
			DrawItem((ViewListPtr) (*hList)->refCon,
				 cell.v + 1, pRect, &cellData, select,
				 false);
			REST_STUFF;
		}
		break;
	}
}


/**********************************************************************
 * PlotTheIcon - Plot a color icon
 **********************************************************************/
static void PlotTheIcon(short iconID, Rect * pRect, Boolean hilite,
			IconAlignmentType align)
{
	short realSettingsRef = CurResFile();
	Boolean plotted = false;
	typedef struct IconTypeMap {
		short id;
		OSType type;
	} IconTypeMap;
	static IconTypeMap iconTable[] = {
		kGenericFolderIconResource, kGenericFolderIcon,
		kTrashIconResource, kTrashIcon,
		0, 0
	};
	static Boolean checkedIconServices, haveIconServices;
	IconTypeMap *pType;
	SInt32 gestaltResult;

	if (!checkedIconServices)
		haveIconServices =
		    !Gestalt(gestaltIconUtilitiesAttr, &gestaltResult)
		    && (gestaltResult &
			gestaltIconUtilitiesHasIconServices)
		    && GetIconRefFromFile;
	if (iconID < 0 && haveIconServices) {
		for (pType = iconTable; pType->type; pType++) {
			if (pType->id == iconID) {
				IconRef iconRef;

				if (!GetIconRef
				    (kOnSystemDisk, kSystemIconsCreator,
				     pType->type, &iconRef)) {
					PlotIconRef(pRect, align,
						    hilite ? ttSelected :
						    ttNone,
						    kIconServicesNormalUsageFlag,
						    iconRef);
					ReleaseIconRef(iconRef);
					plotted = true;
				}
				break;
			}
		}
	}
	if (!plotted) {
		UseResFile(GetMainGlobalSettingsRefN());
		PlotIconID(pRect, align, hilite ? ttSelected : ttNone,
			   iconID);
		UseResFile(realSettingsRef);
	}
}

/**********************************************************************
 * CallBack - Send a message back to the caller
 **********************************************************************/
static long CallBack(ViewListPtr pView, short message, long data)
{
	return (*pView->pCallBack) (pView, message, data);
}

/**********************************************************************
 * DrawRowCallback - Draw a single row via the callback.  Returns true
 *	if no more drawing is required.
 **********************************************************************/
static Boolean DrawRowCallback(ViewListPtr pView, short item, Rect * pRect,
			       CellRec * pCellData, Boolean select,
			       Boolean eraseName)
{
	return (*pView->details.DrawRowCallback) (pView, item, pRect,
						  pCellData, select,
						  eraseName);
}

/**********************************************************************
 * FillBlankCallback - Draw a single row via the callback.  Returns true
 *	if no more drawing is required.
 **********************************************************************/
static Boolean FillBlankCallback(ViewListPtr pView)
{
	return (*pView->details.FillBlankCallback) (pView);
}

/**********************************************************************
 * GetCellRectsCallBack - Draw a single row via the callback.  Returns true
 *	if no more drawing is required.
 **********************************************************************/
static Boolean GetCellRectsCallBack(ViewListPtr pView, CellRec * cellData,
				    Rect * cellRect, Rect * iconRect,
				    Rect * nameRect)
{
	return (*pView->details.GetCellRectsCallBack) (pView, cellData,
						       cellRect, iconRect,
						       nameRect);
}

/**********************************************************************
 * InterestingClickCallback - see if a point is interesting
 **********************************************************************/
static Boolean InterestingClickCallback(ViewListPtr pView,
					CellRec * cellData,
					Rect * cellRect, Point pt)
{
	return (*pView->details.InterestingClickCallback) (pView, cellData,
							   cellRect, pt);
}

/**********************************************************************
 * DrawItem - Draw a list item, item is 1-based
 **********************************************************************/
static void DrawItem(ViewListPtr pView, short item, Rect * pRect,
		     CellRec * pCellData, Boolean select,
		     Boolean eraseName)
{
	Rect rTriangle, rIcon, rName;
	Point ptText;
	Boolean hasColor = IsColorWin(GetMyWindowWindowPtr(pView->wPtr));
	Boolean didCallback;

	if (item && item <= (*pView->hList)->dataBounds.bottom) {
		SAVE_STUFF;

		SET_COLORS;
		GetCellRects(pView, pCellData, pRect, &rTriangle, &rIcon,
			     &rName, &ptText);

		// Do the drawing via the callback, if one is provided.
		didCallback = *pView->details.DrawRowCallback
		    && DrawRowCallback(pView, item, pRect, pCellData,
				       select, eraseName);

		//      Draw triangle even for callback drawing
		if (pCellData->misc.fParent) {
			//      Parent, draw triangle (expanded, collapsed, or in transition
			EraseRect(&rTriangle);
			PlotTheIcon(item ==
				    gWaitExpandRow ?
				    TRIANGLE_TRANSITION_ICON : (pCellData->
								misc.
								fCollapsed
								?
								COLLAPSED_ICON
								:
								EXPANDED_ICON),
				    &rTriangle, false, atNone);
		}
		// Do the drawing via the callback, if one is provided.
		if (!didCallback) {
			//      Plot the icon, if any
			if (pCellData->iconID) {
				OffsetRect(&rIcon, 0, -1);
				PlotTheIcon(pCellData->iconID, &rIcon,
					    select,
					    atNone + atHorizontalCenter);
			}
			//      Draw the name (if not renaming)
			if (pView->renameRow != item) {

				MoveTo(ptText.h, ptText.v);
				InsetRect(&rName, -NameAddMargin, 0);
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

			}
		}
		REST_STUFF;
	}
}

/**********************************************************************
 * GetCellData - Get data for cell,item is 0-based
 **********************************************************************/
static short GetCellData(ViewListPtr pView, short item,
			 CellRec * pCellData)
{
	Size len;
	long offset;
	short dataLen;
	Cell c;

	c.h = 0;
	c.v = item;
	if (pView->flags & kfSupportsMondoBigList) {
		dataLen = sizeof(offset);
		LGetCell(&offset, &dataLen, c, pView->hList);
		if (dataLen) {
			len = offsetof(CellRec, name) + 1;	// Includes the length byte of the name
			BlockMoveData(*(*pView->hList)->userHandle +
				      offset, pCellData, len);
			BlockMoveData(*(*pView->hList)->userHandle +
				      offset + len, &pCellData->name[1],
				      pCellData->name[0]);
			dataLen = len + pCellData->name[0];
		}
	} else {
		dataLen = sizeof(CellRec);
		LGetCell((Ptr) pCellData, &dataLen, c, pView->hList);
	}
	return dataLen;
}

/**********************************************************************
 * FindCellData - Find data for cell, item is zero-based
 **********************************************************************/
static CellRec *FindCellData(ViewListPtr pView, short item)
{
	Cell c;
	long cellOffset;
	short offset, len;

	c.h = 0;
	c.v = item;
	if (pView->flags & kfSupportsMondoBigList) {
		if (item >= (*pView->hList)->dataBounds.bottom || item < 0)
			return (nil);
		len = sizeof(cellOffset);
		LGetCell(&cellOffset, &len, c, pView->hList);
		if (len > 0)
			return (CellRec *) (*(*pView->hList)->userHandle +
					    cellOffset);
		else
			return nil;
	} else {
		LGetCellDataLocation(&offset, &len, c, pView->hList);
		if (len > 0)
			return (CellRec *) (*(*pView->hList)->cells +
					    offset);
		else
			return nil;
	}
}


/**********************************************************************
 * SetCellData - Set data for cell, item is 0-based
 **********************************************************************/
static OSErr SetCellData(ViewListPtr pView, short item,
			 CellRec * pCellData)
{
	CellRec existingCell;
	Cell c;
	long offset;
	short dataLen;
	short len;
	OSErr theError;

	c.h = 0;
	c.v = item;
	theError = noErr;
	if (pView->flags & kfSupportsMondoBigList) {
		len = offsetof(CellRec, name) + pCellData->name[0] + 1;
		// Is the item within the bounds of the list?
		if (item < (*pView->hList)->dataBounds.bottom) {
			dataLen = sizeof(offset);
			LGetCell(&offset, &dataLen, c, pView->hList);
			// If the cell containing the offset into the userHandle is zero,
			// append the cell data onto the end of the big list and set the
			// list cell to point to this new record.
			if (!dataLen) {
				offset =
				    GetHandleSize((*pView->hList)->
						  userHandle);
				theError =
				    PtrPlusHand(pCellData,
						(*pView->hList)->
						userHandle, len);
				if (!theError)
					LSetCell(&offset, sizeof(offset),
						 c, pView->hList);
			}
			// For existing items, we'll have to size the existing record to the new
			// cell data, then adjust any indices that have moved
			else {
				dataLen =
				    GetCellData(pView, item,
						&existingCell);
				Munger((*pView->hList)->userHandle, offset,
				       nil, dataLen, pCellData, len);
				theError = MemError();
				if (!theError && len != dataLen)
					AdjustSubsequentCells(pView->hList,
							      offset,
							      len -
							      dataLen);
			}
		}
	} else {
		len =
		    (offsetof(CellRec, name) + pCellData->name[0] + 1 +
		     1) & -2;
		LSetCell((Ptr) pCellData, len, c, pView->hList);
	}
	return (theError);
}


/**********************************************************************
 * AdjustSubsequentCells - When using really big lists we have to recalculate
 * 												 offsets stored in the list's 'cells' array
 *												 whenever we remove cells or change the size of
 *												 any one cell.  Since the data is stored out of
 *												 order we have to pass through the entire list
 *												 adjusting only those items that originally fell
 *												 after the cell offset that was changed.
 **********************************************************************/
static void AdjustSubsequentCells(ListHandle hList,
				  long followingThisOffset, long delta)
{
	Cell c;
	long dataOffset;
	short dataLen;

	c.h = 0;
	dataLen = sizeof(dataOffset);
	for (c.v = 0; c.v < (*hList)->dataBounds.bottom; ++c.v) {
		LGetCell(&dataOffset, &dataLen, c, hList);
		if (dataOffset > followingThisOffset) {
			dataOffset += delta;
			LSetCell(&dataOffset, sizeof(dataOffset), c,
				 hList);
		}
	}
}


/**********************************************************************
 * LVUpdateSelection - update the selection data
 **********************************************************************/
void LVUpdateSelection(ViewListPtr pView)
{
	UpdateSelection(pView);
}

/**********************************************************************
 * UpdateSelection - update the selection data
 **********************************************************************/
static void UpdateSelection(ViewListPtr pView)
{
	SelectedListHandle hSelectList, oldList;
	CellRec cellData;
	short count = 0, oldCount;

	if (pView->flags & kfSupportsSelectCallbacks) {
		oldCount = pView->selectCount;
		oldList = pView->hSelectList;
		MyHandToHand(&oldList);
	}

	if (hSelectList = pView->hSelectList) {
		Point c;

		c.h = 0;
		c.v = 0;
		while (LGetSelect(True, &c, pView->hList)) {
			if (count >= pView->selectCount) {
				//      Need to expand handle size
				SetHandleSize((Handle) hSelectList,
					      (count +
					       1) *
					      sizeof(SelectedListRec));
				if (MemError())
					return;
			}
			GetCellData(pView, c.v, &cellData);
			(*hSelectList)[count].nodeID = cellData.nodeID;
			(*hSelectList)[count++].row = c.v++;
		}
	}
	pView->selectCount = count;
	SetHandleSize((Handle) hSelectList,
		      (count) * sizeof(SelectedListRec));

	// Compare the old list to the new list, things missing from the new list will be send back
	// to the caller in an unselect callback, while things missing from the old list will be sent
	// to the caller in a select callback.
	if (pView->flags & kfSupportsSelectCallbacks) {
		SelectedListPtr pItem, oldItem;
		short i, j;

		// First, scan the old list for items no longer selected in the new list
		LDRef(oldList);
		LDRef(hSelectList);
		for (i = 0, oldItem = *oldList; i < oldCount; i++) {
			for (j = 0, pItem = *hSelectList;
			     j < count && (pItem->nodeID != oldItem->nodeID
					   || pItem->row != oldItem->row);
			     j++)
				++pItem;
			if (j == count)
				CallBack(pView, kLVUnselectItem,
					 (long) oldItem->nodeID);
			++oldItem;
		}
		// Next, scan the new list for items not in the old list
		for (i = 0, pItem = *hSelectList; i < count; i++) {
			for (j = 0, oldItem = *oldList;
			     j < oldCount
			     && (pItem->nodeID != oldItem->nodeID
				 || pItem->row != oldItem->row); j++)
				++oldItem;
			if (j == oldCount)
				CallBack(pView, kLVSelectItem,
					 (long) pItem->nodeID);
			++pItem;
		}
		UL(hSelectList);
		ZapHandle(oldList);
	}
}

/**********************************************************************
 * CheckFillList - see if we need to add more sub entries to the list
 **********************************************************************/
static void CheckFillList(ViewListPtr pView)
{
	short item;
	ListHandle hList;
	Boolean fUpdate, fCalcNameWidth;
	Rect rUpdate;

	//      Make sure we have the data on all the cells currently being displayed
	hList = pView->hList;
	fUpdate = false;
	pView->maxLevel = 0;

	//      If we've got a lot of names, don't check their width. Just use MaxNameWidth
	fCalcNameWidth = (*hList)->dataBounds.bottom < 250;
	pView->maxNameWidth = 0;

	for (item = (*hList)->dataBounds.top;
	     item < (*hList)->dataBounds.bottom; item++) {
		CellRec cellData;

		if (GetCellData(pView, item, &cellData)) {
			if (cellData.misc.fParent &&
			    !cellData.misc.fCollapsed &&
			    !cellData.misc.fHaveChildren) {
				//      Add children for this parent
				VLNodeID nodeID;
				VLNodeInfo nodeInfo;
				Boolean saveAutoSelect;

				CopyCellInfo(&nodeInfo, &cellData,
					     item + 1);
				nodeID =
				    CallBack(pView, kLVGetParentID,
					     (long) &nodeInfo);
				gAddCell = item + 1;
				gAddLevel = cellData.misc.level + 1;
				if (gAddLevel > pView->maxLevel)
					pView->maxLevel = gAddLevel;
				saveAutoSelect = gAutoSelect;
				gAutoSelect =
				    pView->flags & kfAutoSelectChild
				    && LVIsSelected(pView, item + 1);
				CallBack(pView, kLVAddNodeItems, nodeID);
				gAutoSelect = saveAutoSelect;
				cellData.misc.fHaveChildren = true;
				SetCellData(pView, item, &cellData);	//      Save updated info

				if (!fUpdate && CountChildren(pView, item)) {
					//      Update from this row on down
					Cell1Rect(hList, item + 1,
						  &rUpdate);
					rUpdate.bottom =
					    pView->bounds.bottom;
					fUpdate = true;
				}
			}
			if (cellData.misc.level > pView->maxLevel)
				pView->maxLevel = cellData.misc.level;

			if (fCalcNameWidth) {
				//      Check max name width
				short wd;

				wd = IconLeft + (pView->maxLevel +
						 1) * IconLevelWidth +
				    StringWidth(cellData.name) + 3;
				if (wd > pView->maxNameWidth)
					pView->maxNameWidth = wd;
			}
		}
	}

	if (!fCalcNameWidth)
		pView->maxNameWidth =
		    IconLeft + (pView->maxLevel + 1) * IconLevelWidth +
		    MaxNameWidth + 3;

	if (fUpdate) {
		UpdateSelection(pView);
		DrawList(pView, &rUpdate);
	}
}

/**********************************************************************
 * GetCellRects - get triangle, icon, and name rects
 **********************************************************************/
static void GetCellRects(ViewListPtr pView, CellRec * pCellData,
			 Rect * pRect, Rect * pTriangle, Rect * pIcon,
			 Rect * pName, Point * pText)
{
	short offset;
	Point pt;
	FontInfo fInfo;

	SAVE_STUFF;

	//      Triangle
	if (pCellData->misc.fParent && pCellData->misc.level)
		SetRect(pTriangle, pRect->left + ArrowLeft,
			pRect->top + IconTop, pRect->left + ArrowLeft + 16,
			pRect->top + IconTop + 16);
	else
		SetRect(pTriangle, 0, 0, 0, 0);

	//      Icon
	offset = IconLeft + pCellData->misc.level * IconLevelWidth;
	if (pCellData->iconID) {
		SetRect(pIcon, pRect->left + offset, pRect->top + IconTop,
			pRect->left + offset + 16,
			pRect->top + IconTop + 16);
		offset += IconLevelWidth;
	} else
		SetRect(pIcon, 0, 0, 0, 0);

	//      Name
	TextFont(pView->font);
	TextSize(pView->fontSize);
	TextFace(pCellData->misc.style);
	pt.h = pRect->left + offset;
	pt.v = pRect->top + (*pView->hList)->cellSize.v - TextBottom;
	*pText = pt;
	GetFontInfo(&fInfo);
	SetRect(pName, pt.h, pt.v - fInfo.ascent - fInfo.leading,
		pt.h +
		StringWidth(pCellData->name) /* + 2*CharWidth(' ') */ ,
		pt.v + fInfo.descent);
	REST_STUFF;
}

/**********************************************************************
 * SelectOneItem - Select a single cell, row is 0-based
 **********************************************************************/
static Boolean SelectOneItem(ViewListPtr pView, short row)
{
	if (!QueryItem(pView, kQuerySelect, row + 1))
		row = -1;	//      Can't select this one
	SelectSingleCell(row, pView->hList);
	CheckAutoSelect(pView, row);
	UpdateSelection(pView);
	return row != -1;	//      return false if nothing selected
}

/**********************************************************************
 * PlotTriangle - plot a small B&W icon
 **********************************************************************/
static void PlotTriangle(Rect * pRect, short index)
{
	Handle hSICN;

	EraseRect(pRect);
	if (hSICN = GetResource('SICN', TRIANGLE_SICN))
		PlotSICN(pRect, (SICNHand) hSICN, index);
}

/**********************************************************************
 * TrackTriangle - track mouse over triangle
 **********************************************************************/
static Boolean TrackTriangle(Rect * prTriangle, Boolean collapsed)
{
	Boolean fInside = true;
	Boolean fLastInside = false;

	while (WaitMouseUp()) {
		Point pt;

		GetMouse(&pt);
		fInside = PtInRect(pt, prTriangle);
		if (fInside != fLastInside) {
			PlotTriangle(prTriangle,
				     fInside ? (collapsed ? kRightSICN :
						kDownSICN) : (collapsed ?
							      kRightOutlineSICN
							      :
							      kDownOutlineSICN));
			fLastInside = fInside;
		}
	}

	if (fInside) {
		//      Do intermediate triangles
		PlotTriangle(prTriangle, kDiagonalSICN);
		DelayTicks(2);
		PlotTriangle(prTriangle,
			     collapsed ? kDownSICN : kRightSICN);
		DelayTicks(2);
	}

	//      Plot the final color icon
	EraseRect(prTriangle);
	PlotTheIcon(fInside ? (collapsed ? EXPANDED_ICON : COLLAPSED_ICON)
		    : (collapsed ? COLLAPSED_ICON : EXPANDED_ICON),
		    prTriangle, false, atNone);

	return fInside;
}

/**********************************************************************
 * DelayTicks - delay a few ticks
 **********************************************************************/
static void DelayTicks(long tickCount)
{
	long ticks;

	ticks = TickCount() + tickCount;
	while (TickCount() <= tickCount);
}

/**********************************************************************
 * SetCollapseStatus - expand or collapse a folder, rownum is 1-based
 **********************************************************************/
static void SetCollapseStatus(ViewListPtr pView, CellRec * pCellData,
			      short rowNum, Boolean fCollapse)
{
	Boolean fUpdateOnDown = false;
	VLNodeInfo info;

	if (pCellData->misc.fCollapsed = fCollapse) {
		//      Collapsing
		short numChildren;
		if (numChildren = CountChildren(pView, rowNum - 1)) {
			//      DontDraw(pView);
			// We'll need to remove these records from our private userhandle
			if (pView->flags & kfSupportsMondoBigList) {
				CellRec existingCell;
				Cell c;
				long firstOffset,
				    lastOffset, bytesToRemove;
				short dataLen;
				char dummy;

				c.h = 0;
				c.v = rowNum;
				dataLen = sizeof(firstOffset);
				LGetCell(&firstOffset, &dataLen, c,
					 pView->hList);
				c.v = rowNum + numChildren - 1;
				LGetCell(&lastOffset, &dataLen, c,
					 pView->hList);

				// We need to remove all the data between the first and last offset, plus the length of the last item
				if (dataLen) {
					bytesToRemove =
					    lastOffset - firstOffset +
					    GetCellData(pView, c.v,
							&existingCell);
					Munger((*pView->hList)->userHandle,
					       firstOffset, nil,
					       bytesToRemove, &dummy, 0);
					AdjustSubsequentCells(pView->hList,
							      lastOffset,
							      -bytesToRemove);
				}
			}
			LDelRow(numChildren, rowNum, pView->hList);
			//      DoDraw(pView);
			fUpdateOnDown = true;
		}
		pCellData->misc.fHaveChildren = false;
	}
//      DontDraw(pView);
	SetCellData(pView, rowNum - 1, pCellData);
	// Need to refresh the parent ourselves when using big lists since we bypass the List Manager
	if (pCellData->misc.fCollapsed
	    && (pView->flags & kfSupportsMondoBigList))
		DrawRow(pView, rowNum, LVIsSelected(pView, rowNum));

//      DoDraw(pView);

	//      Update from this row on down
#if 0
	if (fCollapse) {
		Cell1Rect(pView->hList, rowNum, &rUpdate);
		if (fUpdateOnDown) {
			rUpdate.bottom = pView->bounds.bottom;
			UpdateSelection(pView);
		}
		DrawList(pView, &rUpdate);
	}
#endif

	UpdateSelection(pView);

	//      Notify caller
	CopyCellInfo(&info, pCellData, rowNum);
	CallBack(pView, kLVExpandCollapseItem, (long) &info);
}

/**********************************************************************
 * ListViewSend - provide promised drag flavor
 **********************************************************************/
static pascal OSErr ListViewSend(FlavorType flavor, void *dragSendRefCon,
				 ItemReference item, DragReference drag)
{
	OSErr err = cantGetFlavorErr;
	ViewListPtr pView = (ViewListPtr) dragSendRefCon;
	SendDragDataInfo sendData;

	if (flavor == pView->dragFlavor
	    || pView->dragFlavor == kLVDoOwnDragAdd) {
		VLNodeInfo info;
		CellRec *pCellData;
		short row;

		sendData.drag = drag;
		sendData.itemRef = item;
		sendData.flavor = flavor;
		sendData.info = &info;
		row = (*pView->hSelectList)[item - 1].row;
		if (pCellData = FindCellData(pView, row)) {
			CopyCellInfo(&info, pCellData, row + 1);
			if (!CallBack
			    (pView, kLVSendDragData, (long) &sendData))
				err = noErr;
		}
	}
	return (err);
}


/**********************************************************************
 * ListDrag - check for drag and then handle it if it is a drag
 **********************************************************************/
static Boolean ListDrag(ViewListPtr pView, EventRecord * event, Point pt)
{
	ListHandle hList = pView->hList;
	short s = InWhichCell(pt, hList);
	DragReference drag;
	RgnHandle rgn, hDrawRgn;
	GWorldPtr gworld;
	OSErr err = noErr;
	long item;
	DECLARE_UPP(ListViewSend, DragSendData);

	pView->dragGroup = 0;
	for (item = 1; item <= pView->selectCount; item++) {
		//      For every selected item, see if there is one we can't drag
		if (!QueryItem
		    (pView, kQueryDrag,
		     (*pView->hSelectList)[item - 1].row + 1))
			//      Can't drag this item
			return false;
	}

	if (!(err = MyNewDrag(pView->wPtr, &drag))) {
		if (!Cell1Selected(s, hList)) {
			//      This one's not selected. Select it.
			SelectOneItem(pView, s - 1);
		}

		if (!(err = FinderDragVoodoo(drag)))	// bad finder.  bad.
			if (pView->dragFlavor) {
				CellRec *pCellData;

				//      Promise data for each item
				for (item = 1; item <= pView->selectCount;
				     item++)
					if ((pCellData =
					     FindCellData(pView,
							  (*pView->
							   hSelectList)
							  [item - 1].row))
					    && (!pCellData->misc.fParent
						|| pView->dragFlavor ==
						kLVDoOwnDragAdd)) {
						if (pView->dragFlavor ==
						    kLVDoOwnDragAdd) {
							//      The caller wants to add the items itself
							SendDragDataInfo
							    dragInfo;
							VLNodeInfo
							    nodeInfo;

							dragInfo.drag =
							    drag;
							dragInfo.itemRef =
							    item;
							CopyCellInfo
							    (&nodeInfo,
							     pCellData,
							     (*pView->
							      hSelectList)
							     [item -
							      1].row + 1);
							dragInfo.info =
							    &nodeInfo;
							CallBack(pView,
								 kLVAddDragItem,
								 (long)
								 &dragInfo);
						} else
							AddDragItemFlavor
							    (drag, item,
							     pView->
							     dragFlavor,
							     nil, 0L,
							     flavorSenderOnly
							     |
							     flavorNotSaved);
					}
				INIT_UPP(ListViewSend, DragSendData);
				SetDragSendProc(drag, ListViewSendUPP,
						pView);
			}

		// build the region
		if (GetDragRgn(pView, &rgn, &gworld, &hDrawRgn, drag, s)) {
			gDragFlavor = pView->dragFlavor;
			gDragFromMe = gDontDragHilite = true;
			MyTrackDrag(drag, event, rgn);
			gDragFromMe = gDontDragHilite = false;
			gDragFlavor = nil;
			DisposeDragRgn(rgn, gworld, hDrawRgn);
			if (DragTargetWasTrash(drag))
				CallBack(pView, kLVDeleteItem,
					 dataDeleteFromDrag);
			gDidDrag = true;
		}
		MyDisposeDrag(drag);
		return (True);
	      cleanup:
		MyDisposeDrag(drag);
		if (err)
			WarnUser(COULDNT_DRAG, err);
	}

	return (False);
}


/**********************************************************************
 * CopyCellInfo - copy cell info into rec for caller
 **********************************************************************/
static void CopyCellInfo(VLNodeInfo * pInfo, CellRec * pCellData,
			 short rowNum)
{
	pInfo->iconID = pCellData->iconID;
	pInfo->nodeID = pCellData->nodeID;
	pInfo->isParent = pCellData->misc.fParent;
	pInfo->isCollapsed = pCellData->misc.fCollapsed;
	pInfo->style = pCellData->misc.style;
	pInfo->refCon = pCellData->refCon;
	pInfo->rowNum = rowNum;
	PCopy(pInfo->name, pCellData->name);
}


/**********************************************************************
 * CopyNodeInfo - copy node info into a cell for caller
 **********************************************************************/
static void CopyNodeInfo(CellRec * pCellData, VLNodeInfo * pInfo)
{
	pCellData->iconID = pInfo->iconID;
	pCellData->nodeID = pInfo->nodeID;
	pCellData->misc.fParent = pInfo->isParent;
	pCellData->misc.fCollapsed = pInfo->isCollapsed;
	pCellData->misc.style = pInfo->style;
	pCellData->refCon = pInfo->refCon;
	PCopy(pCellData->name, pInfo->name);
}

/**********************************************************************
 * CountChildren - return the number of children this folder has
 **********************************************************************/
static short CountChildren(ViewListPtr pView, short item)
{
	CellRec *pCellData;
	short children = 0;

	if ((pCellData = FindCellData(pView, item))
	    && pCellData->misc.fParent) {
		short level, i;

		level = pCellData->misc.level;
		for (i = item + 1; pCellData = FindCellData(pView, i); i++) {
			if (level < pCellData->misc.level)
				children++;
			else
				break;
		}
	}
	return children;
}

/**********************************************************************
 * InitRename - setup for renaming
 **********************************************************************/
static void InitRename(ViewListPtr pView, Rect * pRect, UPtr name,
		       short rowNum)
{
	PETEHandle pte;
	OSErr err;
	PETEDocInitInfo pdi;
	short saveRight;
	GrafPtr savePort;
	PETEDefaultFontEntry defaultFont;

	GetPort(&savePort);
	SetPort(GetMyWindowCGrafPtr(pView->wPtr));
	pRect->left -= 4;
	pRect->top += 1;
	pRect->bottom += 1;
	pRect->right += 4;
	saveRight = pRect->right;

	DefaultPII(pView->wPtr, false, 0, &pdi);
	pdi.inRect = *pRect;
	pdi.docWidth = 4000;
	//pdi.inParaInfo.startMargin = 2;
	if (err =
	    PeteCreate(pView->wPtr, &pte,
		       peClearAllReturns | peNoStyledPaste, &pdi)) {
		WarnUser(PETE_ERR, err);
		return;
	}
	CleanPII(&pdi);
	Zero(defaultFont);
	defaultFont.pdFont = SmallSysFontID();
	defaultFont.pdSize = SmallSysFontSize();
	err = PETESetDefaultFont(PETE, pte, &defaultFont);
	ASSERT(!err);

	pView->pte = pte;
	SizePete(pView, pRect);
	PeteSetString(name, pte);
	PETEMarkDocDirty(PETE, pte, false);
	PeteSelectAll(pView->wPtr, pte);
	PeteFocus(pView->wPtr, pte, true);
	//      Erase any leftover if we resized
	if (saveRight != pRect->right) {
		Rect rErase;
		SAVE_STUFF;

		SET_COLORS;

		SetRect(&rErase, pRect->right, pRect->top - 1,
			pView->bounds.right - GROW_SIZE, pRect->bottom);
		EraseRect(&rErase);
		REST_STUFF;
	}
	DrawPete(pView);
	pView->renameRow = rowNum;
	SetPort(savePort);
}

/**********************************************************************
 * FinishRename - finish renaming
 **********************************************************************/
static void FinishRename(ViewListPtr pView, Boolean fRename)
{
	Str255 s;
	Rect rCell;
	GrafPtr savePort;
	VLNodeInfo info;
	VLNodeID id;

	GetPort(&savePort);
	SetPort(GetMyWindowCGrafPtr(pView->wPtr));
	PeteStringLo(s, 33, pView->pte);	//      To get a 31-char string, we need to pass 33
	PeteDispose(pView->wPtr, pView->pte);
	pView->pte = nil;

	if (LVGetItem(pView, 1, &info, true))
		id = info.nodeID;

	if (fRename) {
		//      Go ahead and rename the item            
		CallBack(pView, kLVRenameItem, (long) s);
	}

	Cell1Rect(pView->hList, pView->renameRow, &rCell);
	InvalWindowRect(GetMyWindowWindowPtr(pView->wPtr), &rCell);
	pView->renameRow = 0;

	if (fRename) {
		LVSelect(pView, id, s, false);
		LAutoScroll(pView->hList);
		if (gOpenAfterRename)
			CallBack(pView, kLVOpenItem, 0);	//      Open the renamed file
	}

	SetPort(savePort);
	gOpenAfterRename = false;

	LVChangeFocus(pView, kFocusList);
}

/**********************************************************************
 * DontDraw - Don't draw list
 **********************************************************************/
static void DontDraw(ViewListPtr pView)
{
	if (0 == pView->listStatus++)
		LSetDrawingMode(false, pView->hList);
}

/**********************************************************************
 * DoDraw - Do draw list
 **********************************************************************/
static void DoDraw(ViewListPtr pView)
{
	if (pView->listStatus == 0 || --pView->listStatus == 0)
		LSetDrawingMode(true, pView->hList);
}

/**********************************************************************
 * QueryItem - query the caller about a specific item (item is 1-based)
 **********************************************************************/
static Boolean QueryItem(ViewListPtr pView, short query, short item)
{
	VLNodeInfo info;
	CellRec *pCellData;
	long result = 0;

	if (pCellData = FindCellData(pView, item - 1)) {
		CopyCellInfo(&info, pCellData, item);
		info.query = query;
		result = CallBack(pView, kLVQueryItem, (long) &info);
	}

	return result;
}

/**********************************************************************
 * KeyNavigate - navigate through list by keys
 **********************************************************************/
static void KeyNavigate(ViewListPtr pView, StringPtr sKeyNavigation)
{
	ListHandle hList = pView->hList;
	short i, row;
	Str32 s;
	Boolean fCloseMatch = false;

	*s = 0;
	row = 0;
	for (i = 0; i < (*hList)->dataBounds.bottom; i++) {
		CellRec *pCellData;
		if (pCellData = FindCellData(pView, i)) {
			short compare1, compare2;
			unsigned char saveLen;

			saveLen = *pCellData->name;
			if (*sKeyNavigation < saveLen)
				*pCellData->name = *sKeyNavigation;	//      Make the list name the same length as the search name
			compare1 =
			    RelString(pCellData->name, sKeyNavigation,
				      false, true);
			*pCellData->name = saveLen;
			compare2 =
			    RelString(pCellData->name, s, false, true);
			if (compare1 == 0) {
				if (saveLen == *sKeyNavigation) {
					//      Found exact match
					row = i;
					break;
				} else if (!fCloseMatch || compare2 < 0)	//      No match so far or closer one
				{
					fCloseMatch = true;
					goto BestMatch;
				}
			} else if (!fCloseMatch &&
				   compare1 > 0 &&
				   ((*s == 0) || compare2 < 0))
				//      Greater than or equal to the search item. If it's less than the one we have, use it
			{
			      BestMatch:
				PCopy(s, pCellData->name);
				row = i;
			}
		}
	}

	//      Only change selection if it's not the right one
	if (pView->selectCount != 1 || (**pView->hSelectList).row != row)
		SelectOneItem(pView, row);
}

/**********************************************************************
 * GreyRect - do EOR of rect
 **********************************************************************/
static void GreyRect(Rect * pRect)
{
	Pattern grayPat;

	PenMode(patXor);
	PenPat(GetQDGlobalsGray(&grayPat));
	FrameRect(pRect);
	PenPat(GetQDGlobalsBlack(&grayPat));
	PenMode(patCopy);
}

/**********************************************************************
 * DragSelect - drag selection box around items
 **********************************************************************/
static void DragSelect(ViewListPtr pView, Point initPt)
{
	Rect r, lastRect;
	ListHandle hList;

	pView->dragSelect = true;
	SetRect(&lastRect, initPt.h, initPt.v, initPt.h, initPt.v);
	r = pView->bounds;
//      r.right -= GROW_SIZE;   /* I don't know why this was here. ALB 8/4/00 */
	ClipRect(&r);
	GreyRect(&lastRect);
	hList = pView->hList;
	while (WaitMouseUp()) {
		Point pt;
		short scroll;

		GetMouse(&pt);

		if (pt.h < initPt.h) {
			r.left = pt.h;
			r.right = initPt.h;
		} else {
			r.left = initPt.h;
			r.right = pt.h;
		}

		if (pt.v < initPt.v) {
			r.top = pt.v;
			r.bottom = initPt.v;
		} else {
			r.top = initPt.v;
			r.bottom = pt.v;
		}

		if (!EqualRect(&r, &lastRect)) {
			Boolean fChangeSelection;
			short row;
			CellRec *pCellData;

			GreyRect(&lastRect);
			GreyRect(&r);

			//      For every item, check to see if it is within the rect
			fChangeSelection = false;
			for (row = 0; row < (*hList)->dataBounds.bottom;
			     row++) {
				Rect rCell, rTemp;

				Cell1Rect(hList, row + 1, &rCell);
				//      This cell is interesting only if it is within the current rect or with the last one
				if (SectRect(&rCell, &r, &rTemp)
				    || SectRect(&rCell, &lastRect,
						&rTemp)) {
					if (pCellData =
					    FindCellData(pView, row)) {
						Rect rTriangle, rIcon,
						    rName;
						Point ptName;
						Boolean fSelect,
						    fLastSelect;

						// Figure out the name and icon rects to make the drag region
						if (*pView->details.
						    GetCellRectsCallBack)
							GetCellRectsCallBack
							    (pView,
							     pCellData,
							     &rCell,
							     &rIcon,
							     &rName);
						else
							GetCellRects(pView,
								     pCellData,
								     &rCell,
								     &rTriangle,
								     &rIcon,
								     &rName,
								     &ptName);

						fSelect =
						    SectRect(&r,
							     &rTriangle,
							     &rTemp)
						    || SectRect(&r, &rIcon,
								&rTemp)
						    || SectRect(&r, &rName,
								&rTemp);
						fLastSelect =
						    SectRect(&lastRect,
							     &rTriangle,
							     &rTemp)
						    || SectRect(&lastRect,
								&rIcon,
								&rTemp)
						    || SectRect(&lastRect,
								&rName,
								&rTemp);
						if (fSelect ==
						    !fLastSelect) {
							//      Need to change selection
							Cell c;

							c.h = 0;
							c.v = row;
							GreyRect(&r);	//      To avoid screen fragments just hide selection rectangle
							SelectItem(pView,
								   row + 1,
								   !LVIsSelected
								   (pView,
								    row +
								    1),
								   false);
							GreyRect(&r);
							fChangeSelection =
							    true;
						}
					}
				}
			}
			if (fChangeSelection)
				UpdateSelection(pView);
			lastRect = r;
		}

		//      See if we need to scroll
		scroll = 0;
		if (pt.v < pView->bounds.top)
			scroll = -1;
		else if (pt.v > pView->bounds.bottom)
			scroll = 1;
		if ((scroll < 0 && (*hList)->visible.top) || (scroll > 0 &&
							      (*hList)->
							      visible.
							      bottom <
							      (*hList)->
							      dataBounds.
							      bottom)) {
			short top;

			top = (*hList)->visible.top;	//      Save top
			GreyRect(&lastRect);
			LScroll(0, scroll, pView->hList);
			initPt.v += (top - (*hList)->visible.top) * RowHt;
			OffsetRect(&lastRect, 0,
				   -scroll * (*pView->hList)->cellSize.v);
			GreyRect(&lastRect);
		}
	}
	GreyRect(&lastRect);
	ClipMax();
	pView->dragSelect = false;
}


/**********************************************************************
 * LVDragSelectInProgress - Chance for the client to find out if a drag
 * selection is in progress.
 **********************************************************************/
Boolean LVDragSelectInProgress(ViewListPtr pView)
{
	return (pView->dragSelect);
}

/**********************************************************************
 * SelectItem - select (or unselect) an item, item is 1-based
 **********************************************************************/
static void SelectItem(ViewListPtr pView, short item, Boolean fSelect,
		       Boolean dontUpdate)
{
	Cell c;

	if (fSelect && !QueryItem(pView, kQuerySelect, item))
		return;		//      Can't select this one
	c.h = 0;
	c.v = item - 1;
	LSetSelect(fSelect, c, pView->hList);
	CheckAutoSelect(pView, item - 1);
	if (!dontUpdate)
		UpdateSelection(pView);
}

/**********************************************************************
 * DragHilite - Hilite (or unhilite) a list item (1-based). This DOESN'T select.
 **********************************************************************/
static void DragHilite(ViewListPtr pView, short row, Boolean hilite)
{
	if (!row)
		return;		//      Not within list

	if (!hilite && Cell1Selected(row, pView->hList))
		//      Is selected. Don't unhilite.
		return;

	DrawRow(pView, row, hilite);
}

/************************************************************************
 * DrawRow - draw the specified row (1-based)
 ************************************************************************/
static void DrawRow(ViewListPtr pView, short row, Boolean hilite)
{
	ListHandle hList = pView->hList;
	Rect rCell, rClip, r;
	CellRec cellData;

	if (row) {
		Cell1Rect(hList, row, &rCell);
		if (SectRect(&rCell, &pView->bounds, &r))	//      Make sure we're inside the list
		{
			GetCellData(pView, row - 1, &cellData);
			rClip = rCell;
			rClip.right -= 2;	//      Avoid drag hilight on right
			ClipRect(&rClip);
			DrawItem(pView, row, &rCell, &cellData, hilite,
				 true);
			ClipMax();
		}
	}
}

/************************************************************************
 * list click loop
 ************************************************************************/
static pascal Boolean MyListClickLoop(void)
{
	Point mouse;

	//      Don't check for drag the very first time because we haven't returned a true value yet
	//      so LClick hasn't selected our item yet
	if (gDragInfo.clickLoopCount++) {
		if (gDragInfo.clickLoopCount == 2) {
			//      LClick has updated the selection.
			UpdateSelection(gDragInfo.pView);
		}
		GetMouse(&mouse);
		// (jp) 12-11-00  Added 'StillDown' to the if statement so that we don't mistakenly think a
		//                                                              drag is underway when, in fact, 'UpdateSelection' has merely taken a long
		//                                                              time to complete on a single click.
		if (StillDown()
		    && abs(mouse.h - gDragInfo.pt.h) + abs(mouse.v -
							   gDragInfo.pt.
							   v) > 3)
			//      Moved, do drag
			if (ListDrag
			    (gDragInfo.pView, gDragInfo.event,
			     gDragInfo.pt))
				//      Did drag, abort LClick
				return false;
	}

	//      Check for drag outside of list area
	if (gDragInfo.drag && gDontDragHilite) {
		GetMouse(&mouse);
		if (PtInRect(mouse, &gDragInfo.pView->bounds)) {
			//      Make sure draghilite is visible
			if (!gDragInfo.dragHilited) {
				ShowListDragHilite(gDragInfo.pView,
						   gDragInfo.drag);
				gDragInfo.dragHilited = true;
			}
		} else {
			//      Make sure draghilite is hidden
			if (gDragInfo.dragHilited) {
				HideDragHilite(gDragInfo.drag);
				gDragInfo.dragHilited = false;
			}
		}
	}

	return true;
}


/************************************************************************
 * DrawList - draw all or part of the list
 ************************************************************************/
static void DrawList(ViewListPtr pView, Rect * pRect)
{
	RgnHandle hRgn;
	ListHandle hList = pView->hList;

	if (hRgn = NewRgn()) {
		RectRgn(hRgn, pRect);
		LUpdate(hRgn, hList);
		DisposeRgn(hRgn);

		//      Erase any blank rows at the bottom
		if ((*hList)->visible.bottom > (*hList)->dataBounds.bottom) {
			Rect rErase;
			GrafPtr savePort;
			SAVE_STUFF;

			SET_COLORS;
			GetPort(&savePort);
			SetPort(GetMyWindowCGrafPtr(pView->wPtr));
			Cell1Rect(hList, (*hList)->dataBounds.bottom + 1,
				  &rErase);
			rErase.bottom = pView->bounds.bottom;
			EraseRect(&rErase);
			SetPort(savePort);
			REST_STUFF;
		}
	}
}

/************************************************************************
 * IsDragExpanded - is the indicated item already drag-expanded
 ************************************************************************/
static Boolean IsDragExpanded(short rowNum)
{
	short i;

	if (ghDrawExpandedRows) {
		short *pRow;

		pRow = (short *) *ghDrawExpandedRows;
		for (i =
		     (GetHandleSize(ghDrawExpandedRows) / sizeof(short)) -
		     1; i >= 0; i--) {
			if (pRow[i] == rowNum)
				return true;
		}
	}
	return false;
}

/************************************************************************
 * CollapseDragExpanded - collapse any folders that were drag expanded
 *	that do not included the indicated descendent (1-based)
 ************************************************************************/
static void CollapseDragExpanded(ViewListPtr pView, short theRow,
				 DragReference drag)
{
	short i;

	if (ghDrawExpandedRows) {
		short *pRow;
		short count;

		HLock(ghDrawExpandedRows);
		pRow = (short *) *ghDrawExpandedRows;
		count =
		    (GetHandleSize(ghDrawExpandedRows) / sizeof(short));
		for (i = count; i > 0; i--) {
			short checkRow;

			checkRow = pRow[i - 1];
			if (theRow < checkRow) {
				SignedByte cellsState = LockCells(pView);
				SignedByte userHandleState =
				    LockUserHandle(pView);
				if (drag)
					HideDragHilite(drag);
				SetCollapseStatus(pView,
						  FindCellData(pView,
							       checkRow -
							       1),
						  checkRow, true);
				RestoreCells(pView, cellsState);
				RestoreUserHandle(pView, userHandleState);
				if (drag)
					ShowListDragHilite(pView, drag);
			} else
				break;
		}

		HUnlock(ghDrawExpandedRows);

		if (i <= 0 || theRow == 0) {
			//      Collapsed them all
			DisposeHandle(ghDrawExpandedRows);
			ghDrawExpandedRows = 0;
		} else if (i < count) {
			SetHandleSize(ghDrawExpandedRows,
				      i * sizeof(short));
		}
	}
}

/************************************************************************
 * InWhichCell - in what cell (1-based) is the point?
 ************************************************************************/
static short InWhichCell(Point pt, ListHandle hList)
{
	short row;

	if (row = InWhich1Cell(pt, hList)) {
		if (row > (*hList)->dataBounds.bottom)
			row = 0;
	}
	return row;
}

/************************************************************************
 * ClipMax - set clipping to maximum
 ************************************************************************/
static void ClipMax(void)
{
	Rect rFull;

	SetRect(&rFull, -32766, -32766, 32766, 32766);
	ClipRect(&rFull);
}

/************************************************************************
 * ShowListDragHilite - Show the hilite for a drag
 ************************************************************************/
static void ShowListDragHilite(ViewListPtr pView, DragReference drag)
{
	Rect r;

	if (!gDragFromMe) {
		r = pView->bounds;
		r.right -= GROW_SIZE;
		//      InsetRect(&r,-1,-1);
		ShowDragRectHilite(drag, &r, true);
	}
}

/************************************************************************
 * SetWaitExpandRow - set the row with the intermediate triangle to the indicated row number
 ************************************************************************/
static void SetWaitExpandRow(ViewListPtr pView, short rowNum)
{
	if (gWaitExpandRow != rowNum) {
		short saveRow;

		saveRow = gWaitExpandRow;
		gWaitExpandRow = rowNum;
		DrawRow(pView, saveRow, false);
		DrawRow(pView, gWaitExpandRow, false);
	}
}

/************************************************************************
 * DrawPete - draw and frame the edit field
 ************************************************************************/
static void DrawPete(ViewListPtr pView)
{
	Rect r;
	PETEHandle pte;

	if (pte = pView->pte)
		if (PeteIsValid(pte)) {
			PeteRect(pte, &r);
//              if (r.right >= pView->bounds.right-GROW_SIZE)
//              {
			//      Need to clip
//                      r.right = pView->bounds.right-GROW_SIZE-1;
//                      ClipRect(&r);
//              }
			PeteUpdate(pte);
			InsetRect(&r, -1, -1);
			FrameRect(&r);
		}
}

/************************************************************************
 * SizePete - resize edit field
 ************************************************************************/
static void SizePete(ViewListPtr pView, Rect * pRect)
{
	short rtMax;

	rtMax = pView->bounds.right - GROW_SIZE - 4;
	if (pRect->right > rtMax)
		pRect->right = rtMax;
	if (pView->pte)
		PETESizeDoc(PETE, pView->pte, RectWi(*pRect),
			    RectHi(*pRect));
}

/************************************************************************
 * GetParent - find the parent of the item (rowNum is 1-based)
 ************************************************************************/
static short GetParent(ViewListPtr pView, short rowNum)
{
	CellRec *pCellData;
	short parent = 0;

	if (rowNum > 1 && (pCellData = FindCellData(pView, rowNum - 1))) {
		short level, i;

		level = pCellData->misc.level;
		for (i = rowNum - 1;
		     i && (pCellData = FindCellData(pView, i - 1)); i--) {
			if (level > pCellData->misc.level) {
				parent = i;
				break;
			}
		}
	}
	return parent;
}

/**********************************************************************
 * DisposeDragRgn - dispose of drag region
 **********************************************************************/
static void DisposeDragRgn(RgnHandle rgn, GWorldPtr gworld,
			   RgnHandle hDrawRgn)
{
	if (rgn)
		DisposeRgn(rgn);
	if (gworld)
		DisposeGWorld(gworld);
	if (hDrawRgn)
		DisposeRgn(hDrawRgn);
}

/**********************************************************************
 * BringWinToFront - bring our window to the front
 **********************************************************************/
static void BringWinToFront(ViewListPtr pView)
{
	MyWindowPtr pViewWPtr = pView->wPtr;
	if (!pViewWPtr->isActive) {
		WindowPtr pViewWPtrWP = GetMyWindowWindowPtr(pViewWPtr);
		SelectWindow_(pViewWPtrWP);
		UpdateMyWindow(pViewWPtrWP);	//      Have to update manually since no events are processed
	}
}

/**********************************************************************
 * GetDragRgn - make a drag region out of the selected cells (clickRow is 1-based)
 **********************************************************************/
static Boolean GetDragRgn(ViewListPtr pView, RgnHandle * phRgn,
			  GWorldPtr * pgworld, RgnHandle * phDrawRgn,
			  DragReference drag, short clickRow)
{
	Boolean result = false;
	RgnHandle hRgn = nil;
	RgnHandle hDrawRgn = nil;
	GWorldPtr gworld = nil;
	ListHandle hList = pView->hList;

	if (hRgn = NewRgn()) {
		short i;
		RgnHandle hTempRgn;

		if (hTempRgn = NewRgn()) {
			for (i = 1; i <= pView->selectCount; i++) {
				CellRec cellData;
				short row;

				row = (*pView->hSelectList)[i - 1].row;
				if (GetCellData(pView, row, &cellData)) {
					Rect rCell, rTriangle, rIcon,
					    rName;
					Point ptName;
					Boolean fTranslucent = false;

					Cell1Rect(hList, row + 1, &rCell);

					// Figure out the name and icon rects to make the drag region
					if (*pView->details.
					    GetCellRectsCallBack) {
						// do the drag region callback if there is one.
						GetCellRectsCallBack(pView,
								     &cellData,
								     &rCell,
								     &rIcon,
								     &rName);
					} else {
						GetCellRects(pView,
							     &cellData,
							     &rCell,
							     &rTriangle,
							     &rIcon,
							     &rName,
							     &ptName);
						InsetRect(&rName,
							  -NameAddMargin,
							  0);
					}

					//      Get region of icon
					if (IconIDToRgn
					    (hTempRgn, &rIcon,
					     atNone + atHorizontalCenter,
					     cellData.iconID))
						//      Icon unailable, just use icon rect
						RectRgn(hTempRgn, &rIcon);

					if (row == clickRow - 1) {
						//      See if we can do translucent dragging on this one
						long response;
						OSErr err;

						//  check if the Drag Manager supports image dragging�
						if (!Gestalt
						    (gestaltDragMgrAttr,
						     &response)
						    && (response &
							(1L <<
							 gestaltDragMgrHasImageSupport)))
						{
							//  allocate a GWorld to hold the image; it is
							//  okay if the pixels are in the app heap or
							//  in temp memory
							WindowPtr
							    pViewWPtrWP =
							    GetMyWindowWindowPtr
							    (pView->wPtr);
							CGrafPtr savePort;
							GDHandle
							    saveDevice;
							Rect imageRect;
							Point offsetPt;

							GetGWorld
							    (&savePort,
							     &saveDevice);
							UnionRect(&rIcon,
								  &rName,
								  &imageRect);

							if (err =
							    NewGWorld
							    (&gworld, 8,
							     &imageRect,
							     nil, nil,
							     useTempMem))
								err =
								    NewGWorld
								    (&gworld,
								     8,
								     &imageRect,
								     nil,
								     nil,
								     0);

							if (!err) {
								PixMapHandle
								    imagePixMap;
								Rect rGW;

								imagePixMap
								    =
								    GetGWorldPixMap
								    (gworld);

								//  draw the item into the GWorld
								SetGWorld
								    (gworld,
								     nil);
								LockPixels
								    (imagePixMap);
								EraseRect
								    (GetPortBounds
								     (gworld,
								      &rGW));
								TextFont
								    (GetPortTextFont
								     (GetWindowPort
								      (pViewWPtrWP)));
								TextSize
								    (GetPortTextSize
								     (GetWindowPort
								      (pViewWPtrWP)));
								DrawItem
								    (pView,
								     clickRow,
								     &rCell,
								     &cellData,
								     true,
								     true);
								UnlockPixels
								    (imagePixMap);
								SetGWorld
								    (savePort,
								     saveDevice);

								//  We've already got the icon region. Just add the name region
								if (hDrawRgn = NewRgn()) {
									RectRgn(hDrawRgn, &rName);	//      Name region
									UnionRgn
									    (hDrawRgn,
									     hTempRgn,
									     hDrawRgn);

									//      RectRgn(hDrawRgn,&rIcon);

									//  attach the image to the drag
									SetPt
									    (&offsetPt,
									     0,
									     0);
									SetPort
									    (GetMyWindowCGrafPtr
									     (pView->
									      wPtr));
									LocalToGlobal
									    (&offsetPt);
									if (!SetDragImage(drag, imagePixMap, hDrawRgn, offsetPt, kDragDarkTranslucency + kDragRegionAndImage))
										fTranslucent
										    =
										    true;
								}
							}
							SetGWorld(savePort,
								  saveDevice);
						}
					}

					if (!fTranslucent) {
						//      Add regions
						UnionRgn(hRgn, hTempRgn, hRgn);	//      Icon region
						RectRgn(hTempRgn, &rName);	//      Name region
						UnionRgn(hRgn, hTempRgn,
							 hRgn);
					}
				}
			}
			DisposeRgn(hTempRgn);
			OutlineRgn(hRgn, 1);
			GlobalizeRgn(hRgn);
			result = true;
		}
	}

	*phRgn = hRgn;
	*pgworld = gworld;
	*phDrawRgn = hDrawRgn;
	return result;
}

/**********************************************************************
 * LockCells - lock cells handle in ListRec
 **********************************************************************/
static SignedByte LockCells(ViewListPtr pView)
{
	Handle hCells = (*pView->hList)->cells;
	SignedByte cellsState = HGetState(hCells);
	HLock(hCells);
	return cellsState;
}

/**********************************************************************
 * LockUserHandle - lock the big list data handle in our pView
 **********************************************************************/
static SignedByte LockUserHandle(ViewListPtr pView)
{
	Handle userHandle = (*pView->hList)->userHandle;
	SignedByte userHandleState = HGetState(userHandle);
	HLock(userHandle);
	return userHandleState;
}

/**********************************************************************
 * RestoreCells - restore cells handle state
 **********************************************************************/
static void RestoreCells(ViewListPtr pView, SignedByte state)
{
	HSetState((*pView->hList)->cells, state);
}

/**********************************************************************
 * RestoreUserHandle - restore the big list data handle state
 **********************************************************************/
static void RestoreUserHandle(ViewListPtr pView, SignedByte state)
{
	HSetState((*pView->hList)->userHandle, state);
}

/**********************************************************************
 * CheckAutoSelect - see if we need to auto select some children, row is 0-based
 **********************************************************************/
static void CheckAutoSelect(ViewListPtr pView, short row)
{
	CellRec cellData, *pCellData;
	CellRec nextCellData;

	if (pView->flags & kfAutoSelectChild && GetCellData(pView, row, &cellData) && ((cellData.misc.fParent && cellData.misc.fHaveChildren) ||	// current cell has natural children
										       (cellData.misc.level == 0 && GetCellData(pView, row + 1, &nextCellData) && nextCellData.misc.level == 1)))	// current cell has adopted children
	{
		// auto select children
		Boolean select = Cell1Selected(row + 1, pView->hList);
		ListHandle hList = pView->hList;
		for (row++; row < (*hList)->dataBounds.bottom; row++) {
			if ((pCellData = FindCellData(pView, row))
			    && pCellData->misc.level > cellData.misc.level)
				SelectItem(pView, row + 1, select, false);
			else
				break;	// done
		}
	}
}

/**********************************************************************
 * ScrollTopListView - scroll the list to the top.
 **********************************************************************/
void ScrollTopListView(ViewListPtr pView)
{
	ListHandle hList = pView->hList;
	GrafPtr savePort;

	// this seems strange and scary to me ...
	(*hList)->visible.bottom -= (*hList)->visible.top;
	(*hList)->visible.top = 0;

	// redraw the list
	UpdateSelection(pView);
	GetPort(&savePort);
	SetPort(GetMyWindowCGrafPtr(pView->wPtr));
	InvalWindowRect(GetMyWindowWindowPtr(pView->wPtr), &pView->bounds);
	SetPort(savePort);
	DoDraw(pView);
}

/**********************************************************************
 * LVScroll - scroll the list up or down
 **********************************************************************/
void LVScroll(ViewListPtr pView, SInt32 lines)
{
	LScroll(0, -lines, pView->hList);
}

/**********************************************************************
 * LVCopy - copy selected item text to clipboard
 **********************************************************************/
void LVCopy(ViewListPtr pView)
{
	OSErr err = noErr;
	Str255 s;

	if (pView->pte) {
		//      Renaming an item. Copy it's name
		err =
		    PeteEdit(pView->wPtr, pView->pte, peeCopy, &MainEvent);
	} else {
		Handle copyText = NuHandle(0);

		if (!copyText)
			err = MemError();
		else {
			short i;
			VLNodeInfo data;

			for (i = 1; i <= pView->selectCount; i++) {
				LVGetItem(pView, i, &data, true);
				PCopy(s, data.name);
				if (i < pView->selectCount)
					PCatC(s, '\015');
				if (err =
				    PtrPlusHand_(s + 1, copyText, *s))
					break;
			}

			if (!err) {
				ScrapRef scrap;
				ClearCurrentScrap();
				GetCurrentScrap(&scrap);
				err =
				    PutScrapFlavor(scrap, 'TEXT',
						   kScrapFlavorMaskNone,
						   GetHandleSize(copyText),
						   LDRef(copyText));
			}
			ZapHandle(copyText);
		}
	}

	if (err)
		WarnUser(COPY_FAILED, err);
}

/**********************************************************************
 * LVChangeFocus - Change the focus
 **********************************************************************/
void LVChangeFocus(ViewListPtr pView, ListFocusType newFocus)
{
	if (pView->flags & kfListSupportsFocus)
		if (pView->listFocus != newFocus) {
			pView->listFocus = newFocus;
			LVDrawFocus(pView);
		}
}


/**********************************************************************
 * LVFocus - Give (or remove) the listview focus
 **********************************************************************/
Boolean LVSetKeyboardFocus(ViewListPtr pView, Boolean getFocus)
{
	LVChangeFocus(pView, getFocus ? kFocusList : kFocusNone);
	return (true);
}

/**********************************************************************
 * LVAdvanceKeyboardFocus - Silly, we always give up the focus
 **********************************************************************/
Boolean LVAdvanceKeyboardFocus(ViewListPtr pView)
{
	return (true);
}

/**********************************************************************
 * LVReverseKeyboardFocus - Silly, we always give up the focus
 **********************************************************************/
Boolean LVReverseKeyboardFocus(ViewListPtr pView)
{
	return (true);
}

/**********************************************************************
 * LVDrawFocus - Draw the focus
 **********************************************************************/
void LVDrawFocus(ViewListPtr pView)
{
	Boolean hasFocus;

	if (pView->flags & kfListSupportsFocus) {
		hasFocus = pView->wPtr->isActive
		    && pView->listFocus == kFocusList;
		DrawThemeFocusRect(&pView->bounds, hasFocus);
		// Have to redraw the frame on an erase because Inside Mac tells us to
		if (!hasFocus)
			DrawThemeListBoxFrame(&pView->bounds,
					      kThemeStateActive);
	}
}


/**********************************************************************
 * LVGetLastAddedRow - Get the number of the last added row -- 1 based
 **********************************************************************/
short LVGetNextRowToAdd(void)
{
	return (gAddCell);
}
