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

#include "stationerywin.h"

#define FILE_NUM 88
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment StationeryWin

// Stationery Window controls
enum
{
	kctlNew=0,
	kctlEdit,
	kctlRemove
};

typedef struct
{
	ViewList list;
	MyWindowPtr win;
	ControlHandle ctlEdit,ctlNew,ctlRemove;
	Boolean	inited;
	Boolean dontOpenAfterAll; // Don't open; set when deleting stationery that may
														// just have been created
} WinData;
static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR);
static void DoZoomSize(MyWindowPtr win,Rect *zoom);
static void DoUpdate(MyWindowPtr win);
static Boolean DoClose(MyWindowPtr win);
static void DoClick(MyWindowPtr win,EventRecord *event);
static void DoCursor(Point mouse);
static void DoActivate(MyWindowPtr win);
static Boolean DoKey(MyWindowPtr win, EventRecord *event);
static void DoShowHelp(MyWindowPtr win,Point mouse);
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers);
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message, long data);
static void EditStationery(void);
static void SetControls(void);
static void DeleteStationery(void);
static Boolean GetListData(VLNodeInfo *data,short selectedItem);
static void GetStationerySpec(FSSpecPtr pSpec, StringPtr name);
static MyWindowPtr NewStationeryWindow(FSSpecPtr spec);
static void NewStationeryFile(void);
static OSErr DoDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
static void DoGrow(MyWindowPtr win,Point *newSize);
static void CloseStationeryWindow(FSSpecPtr spec);
static Boolean StnyFind(MyWindowPtr win,PStr what);

/************************************************************************
 * OpenStationeryWin - open the stationery window
 ************************************************************************/
void OpenStationeryWin(void)
{
	WindowPtr	gWinWinWP;
	
	if (!HasFeature (featureStationery))
		return;
		
	if (SelectOpenWazoo(STA_WIN)) return;	//	Already opened in a wazoo

	if (!gWin.inited)
	{
		short err=0;
		Rect r;
		
		if (!(gWin.win=GetNewMyWindow(STATIONERY_WIND,nil,nil,BehindModal,false,false,STA_WIN)))
			{err=MemError(); goto fail;}
		gWinWinWP = GetMyWindowWindowPtr (gWin.win);
		SetWinMinSize(gWin.win,-1,-1);
		SetPort_(GetWindowPort(gWinWinWP));
		ConfigFontSetup(gWin.win);	
		MySetThemeWindowBackground(gWin.win,kThemeListViewBackgroundBrush,False);

		// list
		SetRect(&r,0,0,20,20);
		if (LVNew(&gWin.list,gWin.win,&r,1,ViewListCallBack,kStationeryDragType))
			{err=MemError(); goto fail;}
		
		// controls
		if (!(gWin.ctlNew = NewIconButton(STNY_NEW_CNTL,gWinWinWP)) ||
			!(gWin.ctlRemove = NewIconButton(STNY_REMOVE_CNTL,gWinWinWP)) ||
			!(gWin.ctlEdit = NewIconButton(STNY_EDIT_CNTL,gWinWinWP)))
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
		gWin.win->zoomSize = DoZoomSize;
		gWin.win->drag = DoDragHandler;
		gWin.win->grow = DoGrow;
		gWin.win->find = StnyFind;
		gWin.win->showsSponsorAd = true;
		ShowMyWindow(gWinWinWP);
		MyWindowDidResize(gWin.win,&gWin.win->contR);
		gWin.inited = true;
		return;
		
		fail:
			if (gWin.win) CloseMyWindow(GetMyWindowWindowPtr(gWin.win));
			if (err) WarnUser(COULDNT_WIN,err);
			return;
	}
	UserSelectWindow(GetMyWindowWindowPtr(gWin.win));
}


/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR)
{
#define kListInset 10
#pragma unused(oldContR)
	Rect r;
	short	htAdjustment;
	ControlHandle	ctlList[3];
		
	//	buttons
	ctlList[0] = gWin.ctlNew;
	ctlList[1] = gWin.ctlRemove;
	ctlList[2] = gWin.ctlEdit;
	PositionBevelButtons(win,3,ctlList,kListInset,gWin.win->contR.bottom-INSET-kHtCtl,kHtCtl,RectWi(win->contR));

	// list
	SetRect(&r,kListInset,win->topMargin+kListInset,gWin.win->contR.right-kListInset,gWin.win->contR.bottom - 2*INSET - kHtCtl);
	if (gWin.win->sponsorAdExists && r.bottom >= gWin.win->sponsorAdRect.top - kSponsorBorderMargin)
		r.bottom = gWin.win->sponsorAdRect.top - kSponsorBorderMargin;
	LVSize(&gWin.list,&r,&htAdjustment);

	//	enable/disble controls
	SetControls();

	// redraw
	InvalContent(win);
}

/************************************************************************
 * DoZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void DoZoomSize(MyWindowPtr win,Rect *zoom)
{
	short zoomHi = zoom->bottom-zoom->top;
	short zoomWi = zoom->right-zoom->left;
	short hi, wi;
	
	LVMaxSize(&gWin.list, &wi, &hi);
	wi += 2*kListInset;
	hi += 2*kListInset + INSET + kHtCtl;

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;
}

/************************************************************************
 * DoGrow - adjust grow size
 ************************************************************************/
static void DoGrow(MyWindowPtr win,Point *newSize)
{
	Rect	r;
	short	htControl = ControlHi(gWin.ctlNew);
	short	bottomMargin,sponsorMargin;
	
	//	Get list position
	bottomMargin = INSET*2 + htControl;
	if (win->sponsorAdExists)
	{
		CGrafPtr	winPort = GetMyWindowCGrafPtr(win);
		Rect		rPort;
		GetPortBounds(winPort,&rPort);
		sponsorMargin = rPort.bottom - win->sponsorAdRect.top + kSponsorBorderMargin;
		if (sponsorMargin > bottomMargin) bottomMargin  = sponsorMargin;
	}
	SetRect(&r,kListInset,win->topMargin+kListInset,newSize->h-kListInset,newSize->v - bottomMargin);
	LVCalcSize(&gWin.list,&r);
	
	//	Calculate new window height
	newSize->v = r.bottom + bottomMargin;
}

/************************************************************************
 * DoClose - close the window
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win)
{
#pragma unused(win)
	
	if (gWin.inited)
	{
		//	Dispose of list
		LVDispose(&gWin.list);
		gWin.inited = false;
	}
	return(True);
}

/************************************************************************
 * DoUpdate - draw the window
 ************************************************************************/
static void DoUpdate(MyWindowPtr win)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr(win);
	Rect	r;
	
	r = gWin.list.bounds;
	DrawThemeListBoxFrame(&r,kThemeStateActive);
	LVDraw(&gWin.list,nil, true, false);
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
static Boolean DoKey(MyWindowPtr win, EventRecord *event)
{
#pragma unused(win)
	short key = (event->message & 0xff);
	Boolean	fResult;
	
	fResult = LVKey(&gWin.list,event);
	SetControls();
	return fResult;
}

/************************************************************************
 * DoClick - click in window
 ************************************************************************/
void DoClick(MyWindowPtr win,EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point pt;
	ControlHandle	hCtl;
	
	SetPort(GetWindowPort(winWP));
	pt = event->where;
	GlobalToLocal(&pt);
	
	if (PtInControl(pt,gWin.ctlRemove)) gWin.dontOpenAfterAll = True; // don't open file before we delete it

	if (!LVClick(&gWin.list,event))
	{
		if (!win->isActive)
		{
			SelectWindow_(winWP);
			UpdateMyWindow(winWP);	//	Have to update manually since no events are processed
		}
		
		if (FindControl(pt, winWP, &hCtl))
		{
			if (TrackControl(hCtl,pt,(void *)(-1)))
			{
				long controlId;
				
				if (hCtl == gWin.ctlEdit && !DateWarning(true))
				{
					EditStationery();
					controlId = kctlEdit;
				}
				else if (hCtl == gWin.ctlNew && !DateWarning(true))
				{
					NewStationeryFile();
					controlId = kctlNew;
				}
				else if (hCtl == gWin.ctlRemove)
				{
					DeleteStationery();
					controlId = kctlRemove;
				}
				else
				{
					ASSERT(0);
				}
				
				AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),controlId), event->what);
			}
		}
	}
	gWin.dontOpenAfterAll = False;
	SetControls();
}

/**********************************************************************
 * StnyFind - find in the window
 **********************************************************************/
static Boolean StnyFind(MyWindowPtr win,PStr what)
{
	return FindListView(win,&gWin.list,what);
}

/************************************************************************
 * DoCursor - set the cursor properly for the window
 ************************************************************************/
static void DoCursor(Point mouse)
{
	if (!PeteCursorList(gWin.win->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * DoShowHelp - provide help for the window
 ************************************************************************/
static void DoShowHelp(MyWindowPtr win,Point mouse)
{
	if (PtInRect(mouse,&gWin.list.bounds))
		MyBalloon(&gWin.list.bounds,100,0,STNRY_HELP_STRN+1,0,nil);
	else
		ShowControlHelp(mouse,STNRY_HELP_STRN+2,gWin.ctlNew,gWin.ctlRemove,gWin.ctlEdit,nil);
}

/************************************************************************
 * DoMenuSelect - menu choice in the window
 ************************************************************************/
static Boolean DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(win,modifiers)
	
	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_OPENSEL_ITEM:
					EditStationery();
					return(True);
					break;
			}
			break;

		case EDIT_MENU:
			switch(item)
			{
				case EDIT_SELECT_ITEM:
					if (LVSelectAll(&gWin.list))
					{
						SetControls();
						return(true);
					}
					break;
				case EDIT_COPY_ITEM:
					LVCopy(&gWin.list);
					return true;
			}
			break;
	}
	return(False);
}

/************************************************************************
 * GetListData - get data for selected item
 ************************************************************************/
static Boolean GetListData(VLNodeInfo *data,short selectedItem)
{
	return LVGetItem(&gWin.list,selectedItem,data,true);
}

/************************************************************************
 * SetControls - enable or disable the controls, based on current situation
 ************************************************************************/
static void SetControls(void)
{
	Boolean fSelect;
	
	fSelect = LVCountSelection(&gWin.list)!=0;
	gWin.win->hasSelection = fSelect;

	//	Determine if "Remove" and "Edit" button needs to be greyed
	SetGreyControl(gWin.ctlRemove,!fSelect);	
	SetGreyControl(gWin.ctlEdit,!fSelect);	
}

/**********************************************************************
 * DeleteStationery - delete stationery file(s)
 **********************************************************************/
static void DeleteStationery(void)
{	
	FSSpec	spec;
	short		i;
	VLNodeInfo info;
	Handle textH;
	MSumType sum;
	
	for (i=1;i<=LVCountSelection(&gWin.list);i++)
	{
		GetListData(&info,i);
		GetStationerySpec(&spec, info.name);
		CloseStationeryWindow(&spec);

		//	check for spool folder
		if (!Snarf(&spec,&textH,0))
		{
			if (!GetStationerySum(textH,&sum))
				RemSpoolFolder(sum.uidHash);
			ZapHandle(textH);
		}
		
		//	move stationery to trash
		FSpTrash(&spec);
	}
	BuildStationeryList();				
}

/**********************************************************************
 * CloseStationeryWindow - find and close any Stationery window associated with the file
 **********************************************************************/
static void CloseStationeryWindow(FSSpecPtr spec)
{
	WindowPtr		winWP;
	MyWindowPtr	win;
	
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
	{
		if (GetWindowKind(winWP) == COMP_WIN)
		{	
			MessHandle messH;
			
			win = GetWindowMyWindowPtr(winWP);
			messH = Win2MessH(win);			

			if ((*messH)->hStationerySpec)
			{			
				FSSpec winSpec = **(*messH)->hStationerySpec;
				if (SameSpec(spec,&winSpec))
				{
					//	Close the window. Don't save changes
					win->isDirty = False;
					PeteCleanList(win->pte);
					CloseMyWindow(winWP);
					break;
				}
			}
		}
	}
}

/************************************************************************
 * ViewListCallBack - callback function for List View
 ************************************************************************/
static long ViewListCallBack(ViewListPtr pView, VLCallbackMessage message, long data)
{
	VLNodeInfo	info,*pInfo;
	OSErr	err = noErr;
	short	i,count;
	FSSpec	spec;
	MenuHandle	mh;
	SendDragDataInfo	*pSendData;
	Boolean anyStationery = false;
	
	switch (message)
	{
		case kLVAddNodeItems:
			//	Add stationery names to list
			mh = GetMHandle(NEW_WITH_HIER_MENU);	//	Get names from stationery menu
			count = CountMenuItems(mh);
			if (count>1 || IsMenuItemEnabled(mh,1))
				for (i=1;i<=count;i++)
				{
					Zero(info);
					info.useLevelZero = true;
					info.iconID = STATIONERY_ICON;
					MyGetItem(mh, i, info.name);
					LVAdd(pView, &info);
					anyStationery = true;
				}
			if (anyStationery)
				UseFeature (featureStationery);
			break;
		
		case kLVOpenItem:
			if (!gWin.dontOpenAfterAll)
			{
				if (CurrentModifiers()&optionKey)
					//	Edit if option key down
						EditStationery();
				else
				{
					//	New message with stationery
					for (i=1;i<=LVCountSelection(&gWin.list);i++)
					{
						GetListData(&info,i);
						NewMessageWith(info.rowNum);
					}
				}
			}
			break;
			
		case kLVRenameItem:
		{
			Str31 newName;
			SanitizeFN(newName,(StringPtr)data,MAC_FN_BAD,MAC_FN_REP,false);
			GetListData(&info,1);
			GetStationerySpec(&spec, info.name);
			if (err = HRename(spec.vRefNum,spec.parID,spec.name,newName))
				FileSystemError(CANT_RENAME_ERR,spec.name,err);
			BuildStationeryList();				
			break;
		}
		
		case kLVQueryItem:
			pInfo = (	VLNodeInfo *)data;
			switch (pInfo->query)
			{
				case kQuerySelect:
				case kQueryRename:
				case kQueryDrop:
				case kQueryDrag:
				case kQueryDCOpens:
				return true;					
			}
			return false;
			break;
						
		case kLVDeleteItem:
			DeleteStationery();
			break;
		
		case kLVSendDragData:
			pSendData = (SendDragDataInfo *)data;
			GetStationerySpec(&spec,pSendData->info->name);
			err = SetDragItemFlavorData(pSendData->drag, pSendData->itemRef, pSendData->flavor,&spec, sizeof(spec), 0L);
			break;
	}
	return err;
}

/************************************************************************
 * GetStationerySpec - get the file spec for a stationery file
 ************************************************************************/
static void GetStationerySpec(FSSpecPtr pSpec, StringPtr name)
{
	SubFolderSpec(STATION_FOLDER,pSpec);
	PCopy(pSpec->name,name);
}

/************************************************************************
 * EditStationery - edit the selected stationery
 ************************************************************************/
static void EditStationery(void)
{
	//	Open selected stationery
	VLNodeInfo	info;
	short	i;
	
	for (i=1;i<=LVCountSelection(&gWin.list);i++)
	{
		MyWindowPtr newWin;
		FSSpec	spec;

		GetListData(&info,i);
		GetStationerySpec(&spec, info.name);
		if (newWin = NewStationeryWindow(&spec))
			ShowMyWindow(GetMyWindowWindowPtr(newWin));		
	}
}

/************************************************************************
 * BuildStationeryList - rebuild the entire stationery list
 ************************************************************************/
void BuildStationeryList(void)
{
	BuildStationMenu();
	if (gWin.inited)
		InvalidListView(&gWin.list);	//	Regenerate list	
}

/**********************************************************************
 * NewStationeryWindow - create a new stationery window
 **********************************************************************/
static MyWindowPtr NewStationeryWindow(FSSpecPtr spec)
{
	MyWindowPtr newWin;
	
	if (!HasFeature (featureStationery))
		return (nil);
	
	if (newWin=DoComposeNew(0))
	{
		MessHandle messH =(MessHandle)GetMyWindowPrivateData(newWin);

		if (spec)
		{
			FSSpecHandle	hSpec;
			
			if (hSpec = NuHandle(sizeof(FSSpec)))
			{
				BMD(spec,*hSpec,sizeof(FSSpec));
				(*messH)->hStationerySpec = hSpec;
			}
			ApplyStationeryLo(newWin,spec,True,True,true);
		}
		else
			ApplyDefaultStationery(newWin,True,True);
		UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
	}
	return newWin;	
}

/**********************************************************************
 * NewStationeryFile - create a new stationery file
 **********************************************************************/
static void NewStationeryFile(void)
{
	MyWindowPtr newWin;
	FSSpec spec,folderSpec;
	OSErr err;
		
	if (!HasFeature (featureStationery))
		return;
	
	UseFeature (featureStationery);
	
	SubFolderSpec(STATION_FOLDER,&folderSpec);

	//	Make a unique "untitled" name
	MakeUniqueUntitledSpec (folderSpec.vRefNum, folderSpec.parID, UNTITLED, &spec);

	if (err=FSpCreate(&spec,CREATOR,STATIONERY_TYPE,smSystemScript))
	{
		FileSystemError(CREATE_STA,&spec.name,err);
		return;
	}

	//	Write default info to file
	if (newWin = NewStationeryWindow(nil))
	{
		MessHandle messH =(MessHandle)GetMyWindowPrivateData(newWin);
		short	refN;
		
		if (!FSpOpenDF(&spec,fsRdWrPerm,&refN))
		{
			Stationery = True;
			err = SaveAsToOpenFile(refN,messH);
			Stationery = false;
			MyFSClose(refN);
		}
		CloseMyWindow(GetMyWindowWindowPtr(newWin));

		BuildStationeryList();				
		SelectWindow_(GetMyWindowWindowPtr(gWin.win));
		LVRename(&gWin.list,0,spec.name,false,false);	//	Allow user to rename the stationery file
	}
}

/**********************************************************************
 * DoDragHandler - handle drags
 **********************************************************************/
static OSErr DoDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{	
#pragma unused(win)
	OSErr err = noErr;
	VLNodeInfo targetInfo;
	
	if (!DragIsInteresting(drag,MESS_FLAVOR,TOC_FLAVOR,nil))
		return(dragNotAcceptedErr);	//	Nothing here we want
	
	switch (which)
	{
		case kDragTrackingEnterWindow:
		case kDragTrackingLeaveWindow:
		case kDragTrackingInWindow:
				err = LVDrag(&gWin.list,which,drag);
			break;
		case 0xfff:
			//	Drop
				if (LVDrop(&gWin.list,&targetInfo))
				{
					TOCHandle tocH;
					UHandle data=nil;
					
					if (!(err=MyGetDragItemData(drag,1,MESS_FLAVOR,(void*)&data)))
					{
						Boolean all, quote, self;
						MessHandle	messH;

						messH = **(MessHandle**)data;
						tocH = (*messH)->tocH;
						ReplyDefaults(0,&all,&self,&quote);
						DoReplyMessage((*messH)->win,all,self,quote,True,targetInfo.rowNum,True,True,True);
					}
					else if (!(err=MyGetDragItemData(drag,1,TOC_FLAVOR,(void*)&data)))
					{
						tocH = **(TOCHandle**)data;
						DoIterativeThingy(tocH,MESSAGE_REPLY_ITEM,0,(void*)targetInfo.rowNum);
					}
					ZapHandle(data);
					
					return(err);
				}
				else
					return dragNotAcceptedErr;		
				break;
	}
	return err;
}

