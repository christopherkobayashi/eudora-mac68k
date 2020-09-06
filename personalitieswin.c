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

#include "personalitieswin.h"

#define FILE_NUM 87
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment PersonWin

// Personality Window controls
enum {
	kctlNew = 0,
	kctlRemove,
	kctlCheck,
	kctlEdit
};

typedef struct {
	ViewList list;
	MyWindowPtr win;
	ControlHandle ctlNew, ctlRemove, ctlCheck, ctlEdit;
	Boolean inited;
	PersHandle newPers;	//      New personality being renamed
} WinData;
static WinData gWin;
static Boolean gMyCheck;

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
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item,
			    short modifiers);
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message,
			     long data);
static Boolean GetListData(VLNodeInfo * data, short selectedItem);
static void SetControls(void);
static void DeletePersonality(void);
static void NewPersonality(void);
static void RenamePersonality(StringPtr newName);
static void ReplyWithPers(MessHandle messH, PersHandle pers,
			  short modifiers);
static void DoGrow(MyWindowPtr win, Point * newSize);
static void ProcessPersPassword(Boolean change);
static void PersIdle(MyWindowPtr win);
static Boolean PersFind(MyWindowPtr win, PStr what);
static void OpenSelectedPers(void);
extern void CopyExtraSigs(MenuHandle mh);

typedef enum {
	kOK = 1,
	kCancel,
	kPersName,
	kPopAccount,
	kRealName,
	kReturnAddress,
	kAutoCheck,
	kCheckInverval,
	kCheckManual,
	kLeaveMail,
	kPasswords,
	kKerberos,
	kAPOP,
	kDomain2Add,
	kSMTPServer,
	kStationeryMenu,
	kSignatureMenu,
	kSendManual
} EditPersDialogEnum;

/************************************************************************
 * OpenPersonalitiesWin - open the stationery window
 ************************************************************************/
void OpenPersonalitiesWin(void)
{
	WindowPtr gWinWinWP;

	if (!HasFeature(featureMultiplePersonalities))
		return;

	if (SelectOpenWazoo(PERS_WIN))
		return;		//      Already opened in a wazoo

	if (!gWin.inited) {
		short err = 0;
		Rect r;

		if (!
		    (gWin.win =
		     GetNewMyWindow(PERSONALITIES_WIND, nil, nil,
				    BehindModal, false, false,
				    PERS_WIN))) {
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

		/*
		 * list
		 */
		SetRect(&r, 0, 0, 20, 20);
		if (LVNew
		    (&gWin.list, gWin.win, &r, 1, ViewListCallBack,
		     kPersonalityDragType)) {
			err = MemError();
			goto fail;
		}
		// controls
		if (!
		    (gWin.ctlNew = NewIconButton(PERS_NEW_CNTL, gWinWinWP))
		    || !(gWin.ctlRemove =
			 NewIconButton(PERS_REMOVE_CNTL, gWinWinWP))
		    || !(gWin.ctlCheck =
			 NewIconButton(PERS_CHECK_CNTL, gWinWinWP))
		    || !(gWin.ctlEdit =
			 NewIconButton(PERS_EDIT_CNTL, gWinWinWP)))
			goto fail;

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
		gWin.win->idle = PersIdle;
		gWin.win->find = PersFind;
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

/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect * oldContR)
{
#define kListInset 10
#pragma unused(oldContR)
	ControlHandle ctlList[4];
	Rect r;
	short htAdjustment;

	//      buttons
	ctlList[0] = gWin.ctlNew;
	ctlList[1] = gWin.ctlRemove;
	ctlList[2] = gWin.ctlCheck;
	ctlList[3] = gWin.ctlEdit;
	PositionBevelButtons(win, 4, ctlList, kListInset,
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
 * SetControls - enable or disable the controls, based on current situation
 ************************************************************************/
static void SetControls(void)
{
	VLNodeInfo info;
	short count = LVCountSelection(&gWin.list);
	Boolean fSelect = count != 0;


	gWin.win->hasSelection = fSelect;

	SetGreyControl(gWin.ctlEdit, count != 1);
	SetGreyControl(gWin.ctlCheck, !fSelect || CheckThreadRunning);

	if (fSelect) {
		GetListData(&info, 1);
		if (info.rowNum == 1)
			fSelect = false;	//      Can't remove the dominant personality
	}

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
	Rect r, rWin;
	short htControl = ControlHi(gWin.ctlNew);
	short bottomMargin, sponsorMargin;

	//      Get list position
	bottomMargin = INSET * 2 + htControl;
	if (win->sponsorAdExists) {
		WindowPtr winWP = GetMyWindowWindowPtr(win);
		GetWindowPortBounds(winWP, &rWin);
		sponsorMargin =
		    rWin.bottom - win->sponsorAdRect.top +
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
	CGrafPtr winPort = GetMyWindowCGrafPtr(win);
	Rect r;

	r = gWin.list.bounds;
	DrawThemeListBoxFrame(&r, kThemeStateActive);
	LVDraw(&gWin.list, MyGetPortVisibleRegion(winPort), true, false);
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate(MyWindowPtr win)
{
#pragma unused(win)
	LVActivate(&gWin.list, gWin.win->isActive);
	gWin.newPers = nil;
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
 * PersFind - find in the window
 **********************************************************************/
static Boolean PersFind(MyWindowPtr win, PStr what)
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
				long controlId;

				if (hCtl == gWin.ctlEdit) {
					VLNodeInfo info;

					GetListData(&info, 1);
					DoPersSettings(Index2Pers
						       (info.rowNum - 1));
					controlId = kctlEdit;
				} else if (hCtl == gWin.ctlNew) {
					NewPersonality();
					controlId = kctlNew;
				} else if (hCtl == gWin.ctlRemove) {
					DeletePersonality();
					controlId = kctlRemove;
				} else if (hCtl == gWin.ctlCheck) {
					gMyCheck = true;
					XferMail(true, false, true, false,
						 true, 0);
					gMyCheck = false;
					controlId = kctlCheck;
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

/************************************************************************
 * NotifyPersonalitiesWin - notify the personalities window that we've
 *  been up to no good
 ************************************************************************/
void NotifyPersonalitiesWin(void)
{
	if (!HasFeature(featureMultiplePersonalities))
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
		MyBalloon(&gWin.list.bounds, 100, 0, PERS_HELP_STRN + 1, 0,
			  nil);
	else
		ShowControlHelp(mouse, PERS_HELP_STRN + 2, gWin.ctlNew,
				gWin.ctlRemove, gWin.ctlCheck,
				gWin.ctlEdit, nil);
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
		case FILE_OPENSEL_ITEM:
			OpenSelectedPers();
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

/**********************************************************************
 * ReplayWithPers - generate a reply message with indicated personality
 **********************************************************************/
static void ReplyWithPers(MessHandle messH, PersHandle pers,
			  short modifiers)
{
	Boolean all, quote, self;
	MyWindowPtr newWin;

	if (!HasFeature(featureMultiplePersonalities))
		return;

	UseFeature(featureMultiplePersonalities);

	ReplyDefaults(modifiers, &all, &self, &quote);
	if (newWin =
	    DoReplyMessage((*messH)->win, all, self, quote, true, 0, false,
			   false, true)) {
		WindowPtr newWinWP = GetMyWindowWindowPtr(newWin);
		messH = (MessHandle) GetMyWindowPrivateData(newWin);
		SetPers((*messH)->tocH, (*messH)->sumNum, pers, false);
		ShowMyWindow(newWinWP);
		UpdateMyWindow(newWinWP);
	}
}

/**********************************************************************
 * DoDragHandler - handle drags
 **********************************************************************/
static OSErr DoDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag)
{
#pragma unused(win)
	OSErr err = noErr;
	VLNodeInfo targetInfo;

	if (!DragIsInteresting(drag, MESS_FLAVOR, TOC_FLAVOR, nil))
		return (dragNotAcceptedErr);	//      Nothing here we want

	switch (which) {
	case kDragTrackingEnterWindow:
	case kDragTrackingLeaveWindow:
	case kDragTrackingInWindow:
		err = LVDrag(&gWin.list, which, drag);
		break;
	case 0xfff:
		//      Drop
		if (LVDrop(&gWin.list, &targetInfo)) {
			PersHandle pers =
			    Index2Pers(targetInfo.rowNum - 1);
			UHandle data = nil;
			short modifiers = DragOrMods(drag);

			if (!
			    (err =
			     MyGetDragItemData(drag, 1, MESS_FLAVOR,
					       (void *) &data))) {
				ReplyWithPers(**(MessHandle **) data, pers,
					      modifiers);
			} else
			    if (!
				(err =
				 MyGetDragItemData(drag, 1, TOC_FLAVOR,
						   (void *) &data))) {
				short sumNum;
				TOCHandle tocH;

				tocH = **(TOCHandle **) data;
				for (sumNum = 0; sumNum < (*tocH)->count;
				     sumNum++) {
					if ((*tocH)->sums[sumNum].selected) {
						MyWindowPtr tempWin;

						CycleBalls();
						MiniEvents();
						if (EjectBuckaroo
						    || CommandPeriod)
							break;

						if (tempWin =
						    GetAMessage(tocH,
								sumNum,
								nil, nil,
								false)) {
							WindowPtr tempWinWP
							    =
							    GetMyWindowWindowPtr
							    (tempWin);
							ReplyWithPers((MessHandle) GetMyWindowPrivateData(tempWin), pers, modifiers);
							if (!IsWindowVisible(tempWinWP))
								CloseMyWindow
								    (tempWinWP);
							else
								NotUsingWindow
								    (tempWinWP);
						}
					}
				}
			}

			ZapHandle(data);

			return (err);
		} else
			return dragNotAcceptedErr;
		break;
	}
	return err;
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
	PersHandle pers;
	SendDragDataInfo *pSendData;

	switch (message) {
	case kLVAddNodeItems:
		//      Add personality names to list
		count = PersCount();
		if (count > 1)
			UseFeature(featureMultiplePersonalities);

		for (i = 1; i <= count; i++) {
			Zero(info);
			if (pers = Index2Pers(i - 1)) {
				PCopy(info.name, (*pers)->name);
				info.useLevelZero = true;
				info.iconID = PERSONALITIES_ICON;
			}
			LVAdd(pView, &info);
		}
		break;

	case kLVOpenItem:
		//      New message with personality
		OpenSelectedPers();
		break;

	case kLVDeleteItem:
		DeletePersonality();
		break;

	case kLVRenameItem:
		{
			Str31 newName;
			SanitizeFN(newName, (StringPtr) data, MAC_FN_BAD,
				   MAC_FN_REP, false);
			RenamePersonality(newName);
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
			return pInfo->rowNum > 1;
		}
		break;

	case kLVSendDragData:
		pSendData = (SendDragDataInfo *) data;
		pers = Index2Pers(pSendData->info->rowNum - 1);
		err =
		    SetDragItemFlavorData(pSendData->drag,
					  pSendData->itemRef,
					  pSendData->flavor, &pers,
					  sizeof(pers), 0L);
	}
	return err;
}

/************************************************************************
 * RenamePersonality - rename the selected personality
 ************************************************************************/
static void RenamePersonality(StringPtr newName)
{
	VLNodeInfo info;
	PersHandle pers;
	Str255 scratch;

	if (!*newName) {
		WarnUser(PERS_MUST_HAVE_NAME, 0);
		return;
	}

	GetListData(&info, 1);
	if ((pers = Index2Pers(info.rowNum - 1))
	    &&
	    (!EqualString
	     (newName, PCopy(scratch, (*pers)->name), true, true))) {
		if (gWin.newPers == pers
		    || Aprintf(YES_CANCEL_ALRT, Stop, PERS_RENAME,
			       scratch) == 1) {
#ifdef IMAP
			if (IsIMAPPers(pers))
				if (!RenameIMAPPers(pers, newName)) {
					if (gWin.newPers == pers)
						DoPersSettings(pers);
					goto Done;
				}
#endif
			if (!PersSetName(pers, newName)) {
				InvalidListView(&gWin.list);	//      Regenerate list
				if (gWin.newPers == pers)
					DoPersSettings(pers);
			}
		}
	      Done:
		gWin.newPers = nil;
	}
}

/************************************************************************
 * DeletePersonality - delete the selected personality
 ************************************************************************/
static void DeletePersonality(void)
{
	short i;
	VLNodeInfo info;
	Str255 scratch;
	PersHandle pers;
	Boolean fDeleted = false;

	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		GetListData(&info, i);
		if (info.rowNum == 1)
			continue;	//      Can't delete dominant personality
		if (pers = FindPersByName(info.name)) {
#ifdef IMAP
			if (IsIMAPPers(pers)) {
				if (ComposeStdAlert
				    (Stop, IMAP_DELETE_CACHE,
				     PCopy(scratch, (*pers)->name)) == 1) {
					DeleteIMAPPers(pers, false);
					PersDelete(pers);
					fDeleted = true;
				} else
					break;
			} else {
#endif
				if (Aprintf
				    (YES_CANCEL_ALRT, Stop, PERS_DELETE,
				     PCopy(scratch, (*pers)->name)) == 1) {
					PersDelete(pers);
					fDeleted = true;
				} else
					break;
			}
		}
	}
	if (fDeleted) {
		InvalidListView(&gWin.list);	//      Regenerate list
		UpdateMyWindow(GetMyWindowWindowPtr(gWin.win));
	}
}

/************************************************************************
 * NewPersonality - new personality
 ************************************************************************/
static void NewPersonality(void)
{
	PersHandle pers;

	if (HasFeature(featureMultiplePersonalities) && (pers = PersNew())) {
		UseFeature(featureMultiplePersonalities);
		InvalidListView(&gWin.list);	//      Regenerate list
		gWin.newPers = pers;
		LVRename(&gWin.list, 0, (*pers)->name, false, false);	//      Allow user to rename the personality
	}
}

/************************************************************************
 * SelectXferMailPers - select which personalities to check mail/send queued messages
 ************************************************************************/
Boolean SelectXferMailPers(Boolean check, Boolean send, Boolean manual)
{
	//      Specify personalities to check/send if Personalities window is active
	//      and there are one or more personalities selected
	if (gMyCheck && check && manual && gWin.inited) {
		short i;
		PersHandle pers;

		//      Mark the personalities that need to be checked/sent
		for (i = 1, pers = PersList; pers;
		     pers = (*pers)->next, i++) {
			Boolean selected = LVIsSelected(&gWin.list, i);
			PersPtr pPers = *pers;

			pPers->doMeNow = check || send;
			pPers->checkMeNow = selected && check;
			pPers->sendMeNow = selected && send;
			pPers->checked = false;
		}
		return true;
	}
	return false;
}

#if 0
/************************************************************************
 * EditPersonality - edit the selected personality
 ************************************************************************/
static void EditPersonality(void)
{
	MyWindowPtr dlogWin;
	DialogPtr dlog;
	WindowPtr dlogWP;
	short res;
	extern ModalFilterUPP DlgFilterUPP;
	Str255 scratch;
	long sigId;
	VLNodeInfo info;
	PersHandle pers = nil;
	ControlHandle hSigCntl, hStnyCntl;
	MenuHandle hSigMenu, hStnyMenu, hTempMenu;
	short value, count, i;
	Style style;
	typedef struct {
		short item, pref;
	} DlgPrefData;
	static DlgPrefData DlgPrefTable[] = {
		//      Negative item #'s for check boxes
		//      Negative pref #'s for opposite values
		kPopAccount, PREF_POP,
		kRealName, PREF_REALNAME,
		kReturnAddress, PREF_RETURN,
		-kAutoCheck, PREF_AUTO_CHECK,
		kCheckInverval, PREF_INTERVAL,
		-kCheckManual, -PREF_JUST_SAY_NO,
		-kLeaveMail, PREF_LMOS,
		-kPasswords, -PREF_DONT_PASS,
		-kKerberos, PREF_KERBEROS,
		-kAPOP, PREF_APOP,
		kDomain2Add, PREF_AUTOQUAL,
		kSMTPServer, PREF_SMTP,
		-kSendManual, -PREF_PERS_NO_SEND,
		0, 0
	};
	DlgPrefData *pDlgPref;
	PersHandle oldCur = CurPers;


	GetListData(&info, 1);
	pers = Index2Pers(info.rowNum - 1);
	if (!pers)
		return;		//      Shouldn't happen

	SetDAFont(SmallSysFont());
	dlogWin = GetNewMyDialog(EDIT_PERSONALITY_DLOG, nil, nil, InFront);
	SetDAFont(0);
	if (!dlog) {
		(WarnUser(MEM_ERR, ResError()));
		return;
	}
	dlog = GetMyWindowDialogPtr(dlogWin);
	dlogWP = GetDialogWindow(dlog);
	SetPort_(GetWindowPort(dlogWP));
	ConfigFontSetup(dlogWin);

	// fix the dialog controls
	ReplaceAllControls(dlog);

	SetDIText(dlog, kPersName, (*pers)->name);

	//      Set dialog items from preferences
	CurPers = pers;		//      Use prefs for indicated personality
	for (pDlgPref = DlgPrefTable; pDlgPref->item; pDlgPref++) {
		if (pDlgPref->item < 0)
			//      Checkbox item
			SetDItemState(dlog, -pDlgPref->item,
				      pDlgPref->pref <
				      0 ? !PrefIsSet(-pDlgPref->
						     pref) :
				      PrefIsSet(pDlgPref->pref));
		else
			//      Text item
			SetDIText(dlog, pDlgPref->item,
				  GetPref(scratch, pDlgPref->pref));
	}

	//      Setup signature menu
	hSigCntl = GetDItemCtl(dlog, kSignatureMenu);
	hSigMenu = (MenuHandle) GetControlPopupMenuHandle(hSigCntl);
	CopyExtraSigs(hSigMenu);
	SetCtlMax(hSigCntl, CountMItems(hSigMenu));
	sigId = SigValidate(GetPrefLong(PREF_SIGFILE));
	switch (sigId) {
	case SIG_NONE:
		value = sigmNone;
		break;
	case SIG_STANDARD:
		value = sigmStandard;
		break;
	case SIG_ALTERNATE:
		value = sigmAlternate;
		break;
	default:
		for (value = CountMItems(hSigMenu); value > sigmAlternate;
		     value--) {
			MyGetItem(hSigMenu, value, scratch);
			MyLowerStr(scratch);
			if (Hash(scratch) == sigId)
				break;
		}
		break;
	}
	SetCtlValue(hSigCntl, value);

	//      Setup stationery menu
	hStnyCntl = GetDItemCtl(dlog, kStationeryMenu);
	GetPref(scratch, STATIONERY);
	hStnyMenu = (MenuHandle) GetControlPopupMenuHandle(hStnyCntl);
	hTempMenu = GetMHandle(REPLY_WITH_HIER_MENU);
	count = CountMItems(hTempMenu);
	SetCtlMax(hStnyCntl, count + 2);
	GetItemStyle(hTempMenu, 1, &style);
	value = 1;
	if (count > 1 || !style)
		for (i = 1; i <= count; i++) {
			MyGetItem(hTempMenu, i, scratch);
			MyAppendMenu(hStnyMenu, scratch);
			if (EqualStrRes(scratch, STATIONERY))
				value = i + 2;
		}
	SetCtlValue(hStnyCntl, value);

	CurPers = oldCur;	//      Restore the current personality

	// put up the alert
	StartMovableModal(dlog);
	ShowWindow(dlogWP);
	SetMyCursor(arrowCursor);

	do {
		MovableModalDialog(dlog, DlgFilterUPP, &res);
		if (!DialogRadioButtons(dlogWin, res))
			DialogCheckbox(dlogWin, res);
		if (res > kCancel)
			(dlogWin)->isDirty = true;
	}
	while (res > kCancel);

	if (dlogWin->isDirty && res == kOK) {
		//      Save settings
		CurPers = pers;	//      Use prefs for indicated personality
		for (pDlgPref = DlgPrefTable; pDlgPref->item; pDlgPref++) {
			short value, pref;
			if (pDlgPref->item < 0) {
				//      Checkbox item
				value =
				    GetDItemState(dlog, -pDlgPref->item);
				pref = pDlgPref->pref;
				if (pref < 0) {
					pref = -pref;
					value = !value;
				}
				SetPref(pref, value ? YesStr : NoStr);
			} else {
				//      Text item
				GetDIText(dlog, pDlgPref->item, scratch);
				SetPref(pDlgPref->pref, scratch);
			}
		}

		//      Save signature
		value = GetCtlValue(hSigCntl);
		switch (value) {
		case sigmNone:
			sigId = SIG_NONE;
			break;
		case sigmStandard:
			sigId = SIG_STANDARD;
			break;
		case sigmAlternate:
			sigId = SIG_ALTERNATE;
			break;
		default:
			MyGetItem(hSigMenu, value, scratch);
			MyLowerStr(scratch);
			sigId = Hash(scratch);
			break;
		}
		NumToString(sigId, scratch);
		SetPref(PREF_SIGFILE, scratch);

		//      Save stationery
		value = GetCtlValue(hStnyCntl);
		if (value == 1)
			PCopy(scratch, NoStr);
		else
			GetItem(hStnyMenu, value, scratch);
		SetPref(STATIONERY, scratch);

		CurPers = oldCur;	//      Restore the current personality
	}
	//      close
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
	CurPers = oldCur;
}
#endif				//      EditPersonality

/************************************************************************
 * ProcessPassword - change or forget password for selected personalities
 ************************************************************************/
static void ProcessPersPassword(Boolean change)
{
	short i;
	PersHandle pers;
	PersHandle oldCur = CurPers;

	if (!HasFeature(featureMultiplePersonalities))
		return;

	for (i = 1, pers = PersList; pers; pers = (*pers)->next, i++) {
		if (LVIsSelected(&gWin.list, i)) {
			CurPers = pers;
			if (change)
				ChangePassword();
			else
				InvalidatePasswords(false, false, false);
		}
	}
	CurPers = oldCur;
}

/************************************************************************
 * ChangePersPassword - change password for selected personalities
 ************************************************************************/
void ChangePersPassword(void)
{
	ProcessPersPassword(true);
}

/************************************************************************
 * ForgetPersPassword - forget password for selected personalities 
 ************************************************************************/
void ForgetPersPassword(void)
{
	ProcessPersPassword(false);
}

#ifdef	IMAP
/************************************************************************
 * EmptyIMAPPersTrash - empty trash for selected IMAP personalities 
 ************************************************************************/
void EmptyIMAPPersTrash(void)
{
	short i;
	PersHandle pers;
	Boolean warnMe = !PrefIsSet(PREF_NO_TRASH_WARN);

	PushPers(nil);

	for (i = 1, pers = PersList; pers; pers = (*pers)->next, i++) {
		if (LVIsSelected(&gWin.list, i)) {
			CurPers = pers;
			if (PrefIsSet(PREF_IS_IMAP)) {
				// Warn the user once what's about to happen
				if (warnMe) {
					warnMe = false;
					if (!Mom
					    (EMPTY_BUTTON, 0,
					     PREF_NO_TRASH_WARN,
					     IMAP_EMPTY_TRASH_WARN))
						break;
				}

				IMAPEmptyPersTrash();
			}
		}
	}

	PopPers();
}
#endif

/************************************************************************
 * DoWeUseSelectedPersonalities - do we use selected personalities? 
 ************************************************************************/
//      Use only if front window is Personalities window and items are selected
Boolean DoWeUseSelectedPersonalities(void)
{
	WindowPtr theWindow;

	if (!HasFeature(featureMultiplePersonalities))
		return (false);

	return (theWindow = FrontWindow_()) &&
	    GetWindowKind(theWindow) == PERS_WIN &&
	    LVCountSelection(&gWin.list);
}

/************************************************************************
 * DoWeUseSelectedIMAPPersonalities - do we use selected personalities? 
 ************************************************************************/
Boolean DoWeUseSelectedIMAPPersonalities(void)
{
	Boolean imapSelected = false;
	WindowPtr theWindow;
	short i;
	VLNodeInfo info;
	if (!HasFeature(featureMultiplePersonalities))
		return (false);

	if ((theWindow = FrontWindow_())
	    && (GetWindowKind(theWindow) == PERS_WIN)) {
		for (i = 1;
		     (i <= LVCountSelection(&gWin.list)) && !imapSelected;
		     i++) {
			GetListData(&info, i);
			imapSelected =
			    IsIMAPPers(Index2Pers(info.rowNum - 1));
		}
	}

	return (imapSelected);
}

/************************************************************************
 * PersIdle - make sure check personality button is in sync with background checking
 ************************************************************************/
static void PersIdle(MyWindowPtr win)
{
	static Boolean lastCheck;

	if (gWin.inited && CheckThreadRunning != lastCheck) {
		SetControls();
		lastCheck = CheckThreadRunning;
	}
}

/************************************************************************
 * OpenSelectedPers - make new message the selected personalities
 ************************************************************************/
static void OpenSelectedPers(void)
{
	short i;
	VLNodeInfo info;

	for (i = 1; i <= LVCountSelection(&gWin.list); i++) {
		MyWindowPtr win;

		GetListData(&info, i);

		if (win = DoComposeNew(nil)) {
			MessHandle messH =
			    (MessHandle) GetMyWindowPrivateData(win);

			if (info.rowNum == 1)
				//      Dominant personality
				ApplyDefaultStationery(win, True, True);
			else
				SetPers((*messH)->tocH, (*messH)->sumNum,
					Index2Pers(info.rowNum - 1), true);
			ShowMyWindow(GetMyWindowWindowPtr(win));
		}
	}
}

#ifdef IMAP
/************************************************************************
 * IsIMAPPers - is this personality IMAP?
 ************************************************************************/
Boolean IsIMAPPers(PersHandle pers)
{
	PersHandle oldPers = CurPers;
	Boolean result;

	CurPers = pers;
	result = PrefIsSet(PREF_IS_IMAP);
	CurPers = oldPers;
	return result;
}

/************************************************************************
 * RenameIMAPPers - rename an IMAP personality's cache
 ************************************************************************/
Boolean RenameIMAPPers(PersHandle pers, StringPtr newName)
{
	//      Need to rename IMAP mail cache
	FSSpec spec, newSpec;
	Str32 cacheName;
	OSErr err;

	if (!HasFeature(featureMultiplePersonalities))
		return (false);

	PersNameToCacheName(pers, cacheName);
	FSMakeFSSpec(IMAPMailRoot.vRef, IMAPMailRoot.dirId, cacheName,
		     &spec);
	SimpleMakeFSSpec(IMAPMailRoot.vRef, IMAPMailRoot.dirId, newName,
			 &newSpec);
	TellFiltMBRename(&spec, &newSpec, True, True, false);
	if ((err = HRename(spec.vRefNum, spec.parID, spec.name, newName)) && err != fnfErr)	//      Don't report error if local IMAP cache file not created yet
	{
		(FileSystemError(RENAMING_BOX, spec.name, err));
		return false;
	}
	BuildBoxMenus();
	MBTickle(nil, nil);
	TellFiltMBRename(&spec, &newSpec, True, False, false);
	return true;
}

/************************************************************************
 * DeleteIMAPPers - delete an IMAP personality's cache
 ************************************************************************/
void DeleteIMAPPers(PersHandle pers, Boolean bCacheOnly)
{
	FSSpec spec;
	Str32 cacheName;
	PersHandle oldPers = CurPers;

	// go through the personality's mailboxes, making sure they're all closed
	ClosePersMailboxes(pers);

	// remove the IMAP personality's cache
	PersNameToCacheName(pers, cacheName);
	if (FSMakeFSSpec
	    (IMAPMailRoot.vRef, IMAPMailRoot.dirId, cacheName,
	     &spec) == noErr)
		RemoveIMAPCacheDir(spec);
	LDRef(pers);
	DisposeMailboxTree(&((*pers)->mailboxTree));
	UL(pers);

	if (!bCacheOnly) {
		// this is no longer an IMAP personality
		CurPers = pers;
		SetPref(PREF_IS_IMAP, "");
		CurPers = oldPers;
	}

	// rebuild the mailbox menus
	BuildBoxMenus();
	MBTickle(nil, nil);
}

/************************************************************************
 * IMAPPersActive - is this personality active?
 ************************************************************************/
Boolean IMAPPersActive(PersHandle pers)
{
	Boolean result = false;
	TOCHandle tocH = nil;
	PersHandle p = nil;
	MailboxNodeHandle n = nil;

	// this persoanlity is "active" if there's a mailbox currently visible for it.
	for (tocH = TOCList; tocH && !result; tocH = (*tocH)->next) {
		// look at each open, visible TOC
		if ((*tocH)->imapTOC && (*tocH)->win
		    && IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win)))
		{
			n = TOCToMbox(tocH);
			p = TOCToPers(tocH);

			// does this toc refers to a mailbox owned by pers?
			if (n && p && (p == pers))
				result = true;
		}
	}

	return (result);
}

#endif
