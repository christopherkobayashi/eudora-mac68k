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

#include "signaturewin.h"

#define FILE_NUM 86
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment SignaturesWin

// Signature Window controls
enum {
	kctlNew = 0,
	kctlRemove,
	kctlEdit
};

typedef enum { kNewSig, kRemoveSig, ControlCount } ControlEnum;
typedef struct {
	ViewList list;
	MyWindowPtr win;
	ControlHandle ctlNew, ctlRemove;
	ControlHandle ctlEdit;
	Boolean inited;
} WinData;
static WinData gWin;

Handle ghSigList;

/************************************************************************
 * prototypes
 ************************************************************************/
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
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message,
			     long data);
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item,
			    short modifiers);
static void OpenSig(short item);
static Boolean GetListData(VLNodeInfo * data, short selectedItem);
static void NewSigFile(void);
static void DeleteSig(void);
static void GetSigSpec(short index, FSSpecPtr pSpec, StringPtr name);
static void OpenSelectedSigs(void);
static void DoGrow(MyWindowPtr win, Point * newSize);
static void CloseTextWindow(FSSpecPtr spec);
static void RenameTextWindow(FSSpecPtr spec, StringPtr newName);
static Boolean SigFind(MyWindowPtr win, PStr what);
static void SetControls(void);


/************************************************************************
 * OpenSignaturesWin - open the stationery window
 ************************************************************************/
void OpenSignaturesWin(void)
{
	WindowPtr gWinWinWP;
	if (SelectOpenWazoo(SIG_WIN))
		return;		//      Already opened in a wazoo

	if (!gWin.inited) {
		short err = 0;
		Rect r;

		if (!
		    (gWin.win =
		     GetNewMyWindow(SIGNATURES_WIND, nil, nil, BehindModal,
				    false, false, SIG_WIN))) {
			err = MemError();
			goto fail;
		}
		gWinWinWP = GetMyWindowWindowPtr(gWin.win);
		SetWinMinSize(gWin.win, -1, -1);
		SetPort_(GetWindowPort(gWinWinWP));
		ConfigFontSetup(gWin.win);
		MySetThemeWindowBackground(gWin.win,
					   kThemeListViewBackgroundBrush,
					   False);

		// controls
		if (HasFeature(featureSignatures))
			if (!
			    (gWin.ctlNew =
			     NewIconButton(SIG_NEW_CNTL, gWinWinWP)))
				goto fail;
		if (!
		    (gWin.ctlRemove =
		     NewIconButton(SIG_REMOVE_CNTL, gWinWinWP)))
			goto fail;
		if (!
		    (gWin.ctlEdit =
		     NewIconButton(SIG_EDIT_CNTL, gWinWinWP)))
			goto fail;


		// list
		SetRect(&r, 0, 0, 20, 20);
		if (LVNew
		    (&gWin.list, gWin.win, &r, 1, ViewListCallBack,
		     'TEXT')) {
			err = MemError();
			goto fail;
		}

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
		gWin.win->grow = DoGrow;
		gWin.win->find = SigFind;

//              gWin.win->drag = DoDragHandler;
		gWin.win->zoomSize = DoZoomSize;
		gWin.win->showsSponsorAd = true;
		ShowMyWindow(gWinWinWP);
		MyWindowDidResize(gWin.win, &gWin.win->contR);
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

/**********************************************************************
 * SignatureCount - return number of signatures
 **********************************************************************/
short SignatureCount(void)
{
	if (!ghSigList)
		BuildSigList();
	if (ghSigList)
		return *(short *) *ghSigList;
	else
		return 0;
}

/**********************************************************************
 * GetSignatureName - return name of indicated signature
 **********************************************************************/
void GetSignatureName(short item, StringPtr s)
{
	short count;

	if ((count = SignatureCount()) && item <= count) {
		StringPtr pName;
		short i;

		pName = *ghSigList + sizeof(short);
		for (i = 1; i < item; i++, pName += *pName + 1);
		PCopy(s, pName);
	} else
		*s = 0;
}

/**********************************************************************
 * GetSignatureIcon - return icon resID for menu of indicated signature
 **********************************************************************/
short GetSignatureIcon(short item)
{
	switch (item) {
	case 1:
		return SIG_SICN;
	case 2:
		return ALT_SIG_SICN;
	default:
		return (HasFeature(featureSignatures) ? N_SIG_SICN :
			PRO_ONLY_ICON);
	}
}


/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect * oldContR)
{
#define kListInset 10
#pragma unused(oldContR)
	Rect r;
	short htAdjustment;
	short i = 0;
	ControlHandle ctlList[3];

	//      buttons
	if (HasFeature(featureSignatures))
		ctlList[i++] = gWin.ctlNew;
	ctlList[i++] = gWin.ctlRemove;
	ctlList[i++] = gWin.ctlEdit;
	PositionBevelButtons(win, i, ctlList, kListInset,
			     gWin.win->contR.bottom - INSET - kHtCtl,
			     kHtCtl, RectWi(win->contR));

	// list
	SetRect(&r, kListInset, win->topMargin + kListInset,
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
	CGrafPtr winPort = GetMyWindowCGrafPtr(win);
	Rect r, rPort;
	short htControl = ControlHi(gWin.ctlNew);
	short bottomMargin, sponsorMargin;

	//      Get list position
	bottomMargin = INSET * 2 + htControl;
	if (win->sponsorAdExists) {
		GetPortBounds(winPort, &rPort);
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
	}
	return (True);
}

/************************************************************************
 * DoUpdate - draw the window
 ************************************************************************/
static void DoUpdate(MyWindowPtr win)
{
	CGrafPtr winPort;
	Rect r;

	winPort = GetMyWindowCGrafPtr(win);

	r = gWin.list.bounds;
	DrawThemeListBoxFrame(&r, kThemeStateActive);
	LVDraw(&gWin.list, nil, true, false);
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate(MyWindowPtr win)
{
#pragma unused(win)
	LVActivate(&gWin.list, gWin.win->isActive);
}

/************************************************************************
 * DoKey - key stroke
 ************************************************************************/
static Boolean DoKey(MyWindowPtr win, EventRecord * event)
{
#pragma unused(win)
	short key = (event->message & 0xff);
	Boolean fResult;

	fResult = LVKey(&gWin.list, event);
	SetControls();
	return fResult;
}

/************************************************************************
 * DoClick - click in window
 ************************************************************************/
void DoClick(MyWindowPtr win, EventRecord * event)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Point pt;
	ControlHandle hCtl;

	SetPort(GetWindowPort(winWP));

	if (!LVClick(&gWin.list, event)) {
		pt = event->where;
		GlobalToLocal(&pt);
		if (!win->isActive) {
			SelectWindow_(winWP);
			UpdateMyWindow(winWP);	//      Have to update manually since no events are processed
		}

		if (FindControl(pt, winWP, &hCtl)) {
			if (TrackControl(hCtl, pt, (void *) (-1))) {
				long controlId;

				if (HasFeature(featureSignatures)) {
					if (hCtl == gWin.ctlNew) {
						NewSigFile();
						controlId = kctlNew;
					} else if (hCtl == gWin.ctlRemove) {
						DeleteSig();
						controlId = kctlRemove;
					} else if (hCtl == gWin.ctlEdit) {
						OpenSelectedSigs();
						controlId = kctlEdit;
					}
				} else {
					if (hCtl == gWin.ctlEdit) {
						OpenSelectedSigs();
						controlId = kctlEdit;
					} else if (hCtl == gWin.ctlRemove) {
						DeleteSig();
						controlId = kctlRemove;
					}
				}
				AuditHit((event->modifiers & shiftKey) !=
					 0,
					 (event->modifiers & controlKey) !=
					 0,
					 (event->modifiers & optionKey) !=
					 0,
					 (event->modifiers & cmdKey) != 0,
					 false, GetWindowKind(winWP),
					 AUDITCONTROLID(GetWindowKind
							(winWP),
							controlId),
					 event->what);
			}
		}
	}
	SetControls();
}

/**********************************************************************
 * SigFind - find in the window
 **********************************************************************/
static Boolean SigFind(MyWindowPtr win, PStr what)
{
	return FindListView(win, &gWin.list, what);
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
		MyBalloon(&gWin.list.bounds, 100, 0, SIG_HELP_STRN + 1, 0,
			  nil);
	else if (HasFeature(featureSignatures))
		ShowControlHelp(mouse, SIG_HELP_STRN + 2, gWin.ctlNew,
				gWin.ctlRemove, gWin.ctlEdit, nil);
	else
		ShowControlHelp(mouse, SIG_HELP_STRN + 3, gWin.ctlRemove,
				gWin.ctlEdit, nil);
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
		case FILE_OPENSEL_ITEM:
			OpenSelectedSigs();
			return (True);
			break;
		}
		break;

	case EDIT_MENU:
		switch (item) {
		case EDIT_SELECT_ITEM:
			if (LVSelectAll(&gWin.list)) {
				SetControls();
				return (true);
			}
			break;
		case EDIT_COPY_ITEM:
			LVCopy(&gWin.list);
			return true;
		}
		break;
	}
	return (False);
}

#if 0
/**********************************************************************
 * DoDragHandler - handle drags
 *
 *  Can do internal dragging of mailboxes or external dragging and dropping
 *  of messages
 **********************************************************************/
static OSErr DoDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag)
{
	OSErr err = noErr;

//      gfMessageDrag = DragIsInteresting(drag,MESS_FLAVOR,TOC_FLAVOR,nil);     //      Dragging messages
//      gfBoxDrag = LVDragFlavor() == kMBDragType;      //      Dragging mailboxes

//      if (!gfMessageDrag && !gfBoxDrag)
//              return(dragNotAcceptedErr);     //      Nothing here we want

	switch (which) {
	case dragTrackingEnterWindow:
	case dragTrackingLeaveWindow:
	case dragTrackingInWindow:
		err = LVDrag(&gWin.list, which, drag);
		break;
	case 0xfff:
		//      Drop
		if (gfBoxDrag)
			//      Mailbox drag
			err = LVDrag(&gWin.list, which, drag);
		else {
			//      Message drag
			VLNodeInfo targetInfo;
			if (LVDrop(&gWin.list, &targetInfo)) {
				FSSpec spec, *pTOCSpec;
				TOCHandle tocH;
				short sumNum;
				OSErr err;
				UHandle data = nil;
				short vRef;
				long dirID;

				//      Get spec of mailbox dropped on
				MBMenuFile(targetInfo.nodeID, &vRef,
					   &dirID);
				FSMakeFSSpec(vRef, dirID, targetInfo.name,
					     &spec);

				if (!
				    (err =
				     MyGetDragItemData(drag, 1,
						       MESS_FLAVOR,
						       (void *) &data))) {
					tocH =
					    (***(MessHandle **) data)->
					    tocH;
					sumNum =
					    (***(MessHandle **) data)->
					    sumNum;
				} else
				    if (!
					(err =
					 MyGetDragItemData(drag, 1,
							   TOC_FLAVOR,
							   (void *)
							   &data))) {
					tocH = **(TOCHandle **) data;
					sumNum = -1;
				}
				ZapHandle(data);

				//      Can't move to own mailbox
				pTOCSpec = &(*tocH)->spec;
				if (pTOCSpec->vRefNum == spec.vRefNum
				    && pTOCSpec->parID == spec.parID
				    && StringSame(pTOCSpec->name,
						  spec.name))
					//      Dragging to own mailbox
					return dragNotAcceptedErr;

				if (!err) {
					if (sumNum == -1)
						MoveSelectedMessages(tocH,
								     &spec,
								     (DragOrMods
								      (drag)
								      &
								      optionKey)
								     != 0);
					else {
//                                                              if (!(DragOrMods(drag)&optionKey)) EzOpen(tocH,sumNum,0,DragOrMods(drag),True,True);
						if (!
						    (DragOrMods(drag) &
						     optionKey)) {
#ifndef clarenceBug821
#ifdef TWO
							AddXfUndo(tocH,
								  TOCBySpec
								  (&spec),
								  sumNum);
#endif
#endif
							EzOpen(tocH,
							       sumNum, 0,
							       DragOrMods
							       (drag),
							       True, True);
						}
						MoveMessage(tocH, sumNum,
							    &spec,
							    (DragOrMods
							     (drag) &
							     optionKey) !=
							    0);
					}
				}
				return (err);
			} else
				return dragNotAcceptedErr;

		}
		break;
	}
	return err;
}
#endif

/************************************************************************
 * GetListData - get data for selected item
 ************************************************************************/
Boolean GetListData(VLNodeInfo * data, short selectedItem)
{
	return LVGetItem(&gWin.list, selectedItem, data, true);
}

/**********************************************************************
 * BuildSigList - build the signature list
 **********************************************************************/
void BuildSigList(void)
{
	Str255 string;
	FSSpec folderSpec;
	CInfoPBRec hfi;
	long newDirId;
	short count;
	Boolean extraSignatures = false;

	// create the folder
	DirCreate(Root.vRef, Root.dirId, GetRString(string, SIG_FOLDER),
		  &newDirId);

	// is there a folder?
	if (!SubFolderSpec(SIG_FOLDER, &folderSpec)) {
		// setup list handle
		if (ghSigList)
			SetHandleSize(ghSigList, sizeof(short));
		else {
			ghSigList = NuHandle(2);
			if (!ghSigList)
				return;
		}

		GetRString(string, FILE_ALIAS_STANDARD);	//      Standard signature
		PtrAndHand(string, ghSigList, *string + 1);
		GetRString(string, FILE_ALIAS_ALTERNATE);	//      Alternate signature
		PtrAndHand(string, ghSigList, *string + 1);
		count = 2;

		/*
		 * iterate through all the files, adding them to the list
		 */
		//Multiple Signatures - allow only the standard and alternate sigs in light     
		hfi.hFileInfo.ioNamePtr = string;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate
		       (folderSpec.vRefNum, folderSpec.parID, &hfi)) {
			if (!(hfi.hFileInfo.ioFlAttrib & 0x10)
			    && !EqualStrRes(string, SIGNATURE)
			    && !EqualStrRes(string, ALT_SIG)
			    && (hfi.hFileInfo.ioFlFndrInfo.fdType ==
				'TEXT')) {
				extraSignatures = true;
				PtrAndHand(string, ghSigList, *string + 1);
				count++;
			}
		}
		if (extraSignatures)
			UseFeature(featureSignatures);

		*(short *) *ghSigList = count;
	}

	if (gWin.inited)
		InvalidListView(&gWin.list);	//      Regenerate list 
}

/************************************************************************
 * SetControls - enable or disable the controls, based on current situation
 ************************************************************************/
static void SetControls(void)
{
	Boolean fSelect, fRemove;

	fSelect = LVCountSelection(&gWin.list) != 0;
	gWin.win->hasSelection = fSelect;

	//      Determine if "Remove" and "Edit" buttons need to be greyed
	if (fRemove = fSelect) {
		//      Make sure that selection doesn't include any non-removeable items
		short i;
		VLNodeInfo data;

		for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
			GetListData(&data, i);
			if (data.rowNum <= 2) {
				//      Here's an item that can't be removed
				fRemove = false;
			}
			if (!HasFeature(featureSignatures)
			    && data.rowNum > 2)
				fSelect = false;
		}
	}
	SetGreyControl(gWin.ctlRemove, !fRemove);
	SetGreyControl(gWin.ctlEdit, !fSelect);
}

/**********************************************************************
 * NewSigFile - create a new signature file
 **********************************************************************/
static void NewSigFile(void)
{
	FSSpec spec, folderSpec;
	OSErr err;

	//Multiple Signatures - allow only the standard and alternate sigs in light     
	if (!HasFeature(featureSignatures))
		return;

	UseFeature(featureSignatures);

	if (SubFolderSpec(SIG_FOLDER, &folderSpec))
		return;

	//      Make a unique "untitled" name
	MakeUniqueUntitledSpec(folderSpec.vRefNum, folderSpec.parID,
			       UNTITLED, &spec);

	if (err = FSpCreate(&spec, CREATOR, 'TEXT', smSystemScript)) {
		FileSystemError(CREATE_SIG, &spec.name, err);
		return;
	}

	BuildSigList();

	LVRename(&gWin.list, 0, spec.name, true, false);	//      Allow user to rename the signature file
}

/**********************************************************************
 * DeleteSig - delete signature file(s)
 **********************************************************************/
static void DeleteSig(void)
{
	FSSpec spec;
	short i;
	VLNodeInfo info;

	//Multiple Signatures - allow only the standard and alternate sigs in light     
	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		GetListData(&info, i);
		if (info.rowNum > 2) {
			GetSigSpec(info.rowNum, &spec, info.name);
			CloseTextWindow(&spec);
			FSpTrash(&spec);
		}
	}

	BuildSigList();
}

/**********************************************************************
 * CloseTextWindow - find and close any text window associated with the file
 **********************************************************************/
static void CloseTextWindow(FSSpecPtr spec)
{
	WindowPtr winWP;
	MyWindowPtr win;

	//Multiple Signatures - allow only the standard and alternate sigs in light     
	for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr(winWP);
		if (GetWindowKind(winWP) == TEXT_WIN) {
			FSSpec winSpec =
			    (*(TextDHandle) GetMyWindowPrivateData(win))->
			    spec;
			if (SameSpec(spec, &winSpec)) {
				//      Close the window. Don't save changes
				win->isDirty = False;
				PeteCleanList(win->pte);
				CloseMyWindow(winWP);
				break;
			}
		}
	}
}

/**********************************************************************
 * RenameTextWindow - find and rename any text window associated with the file
 **********************************************************************/
static void RenameTextWindow(FSSpecPtr spec, StringPtr newName)
{
	WindowPtr winWP;
	MyWindowPtr win;

	//Multiple Signatures - allow only the standard and alternate sigs in light     
	for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr(winWP);
		if (GetWindowKind(winWP) == TEXT_WIN) {
			FSSpec winSpec =
			    (*(TextDHandle) GetMyWindowPrivateData(win))->
			    spec;
			if (SameSpec(spec, &winSpec)) {
				PCopy((*(TextDHandle) GetMyWindowPrivateData(win))->spec.name, newName);	//      Adjust file spec
				SetWTitle_(winWP, newName);
				break;
			}
		}
	}
}


/************************************************************************
 * ViewListCallBack - callback function for List View
 ************************************************************************/
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message,
			     long data)
{
	VLNodeInfo info, *pInfo;
	OSErr err = noErr;
	short i, count;
	FSSpec spec;
	SendDragDataInfo *pSendData;
	Handle hData;

	count = SignatureCount();
	switch (message) {
	case kLVAddNodeItems:
		//      Add signature names to list
		for (i = 1; i <= count; i++) {
			Zero(info);
			info.useLevelZero = true;
			info.iconID = GetSignatureIcon(i);
			GetSignatureName(i, info.name);
			LVAdd(pView, &info);
		}
		break;

	case kLVOpenItem:
		OpenSelectedSigs();
		break;

	case kLVRenameItem:
		{
			Str31 newName;
			SanitizeFN(newName, (StringPtr) data, MAC_FN_BAD,
				   MAC_FN_REP, false);
			if (EqualStrRes(newName, FILE_ALIAS_STANDARD)
			    || EqualStrRes(newName, FILE_ALIAS_ALTERNATE))
				ComposeStdAlert(Caution, NAME_IN_USE);
			else {
				GetListData(&info, 1);
				if (info.rowNum > 2)
					UseFeature(featureSignatures);
				GetSigSpec(info.rowNum, &spec, info.name);
				if (err =
				    HRename(spec.vRefNum, spec.parID,
					    spec.name, newName))
					FileSystemError(CANT_RENAME_ERR,
							spec.name, err);
				else
					RenameTextWindow(&spec, newName);
			}
			BuildSigList();
			break;
		}

	case kLVQueryItem:
		pInfo = (VLNodeInfo *) data;
		switch (pInfo->query) {
		case kQuerySelect:
		case kQueryDrag:
		case kQueryDrop:
		case kQueryDCOpens:
			return true;
		case kQueryRename:
			return pInfo->rowNum >= 3;	//      Can't rename standard or alternate
		}
		return false;
		break;

	case kLVDeleteItem:
		DeleteSig();
		break;

	case kLVSendDragData:
		//      Get context of sig file for drag
		pSendData = (SendDragDataInfo *) data;
		GetSigSpec(pSendData->info->rowNum, &spec,
			   pSendData->info->name);
		if (!Snarf(&spec, &hData, nil)) {
			HLock(hData);
			SetDragItemFlavorData(pSendData->drag,
					      pSendData->itemRef,
					      pSendData->flavor, *hData,
					      GetHandleSize(hData), 0L);
			DisposeHandle(hData);
		}
		break;
	}
	return err;
}

/************************************************************************
 * GetSigSpec - get the file spec for a signature
 ************************************************************************/
static void GetSigSpec(short index, FSSpecPtr pSpec, StringPtr name)
{
	SubFolderSpec(SIG_FOLDER, pSpec);
	if (index <= 2)
		GetRString(pSpec->name, index == 2 ? ALT_SIG : SIGNATURE);
	else
		PCopy(pSpec->name, name);	//      Use file name
}

/************************************************************************
 * OpenSelectedSigs - open the selected signature(s)
 ************************************************************************/
static void OpenSelectedSigs(void)
{
	//      Open selected signatures
	VLNodeInfo info;
	short i;
	FSSpec spec;

	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		GetListData(&info, i);
		if (info.rowNum <= 2 || HasFeature(featureSignatures)) {
			GetSigSpec(info.rowNum, &spec, info.name);
			OpenText(&spec, nil, nil, nil, true, nil, false,
				 false);
		}
	}
}
