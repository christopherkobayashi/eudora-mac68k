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

#include "mbwin.h"
#include "listview.h"
#define FILE_NUM 23
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * handling the mailbox window
 **********************************************************************/

//	First few items in list
enum	{ kItemEudoraFolder=1,kItemInBox,kItemOutBox,kItemJunkBox,kItemTrash,kItemMailBoxes };

// ViewListIcon - Icon constants for list view
enum 
{
	kFolderIcon=kGenericFolderIconResource,
	kTrashBoxIcon=kTrashIconResource,
	kMailBoxIcon=MAILBOX_ONLY_ICON,
	kIMAPMailboxIcon=IMAP_MAILBOX_FILE_ICON,
	kIMAPPolledMailboxIcon=RESYNC_ICON,
	kIMAPPolledFolderIcon=IMAP_POLLED_MAILBOX_FILE_ICON
};

#define dashChar ((unsigned char)'-')

Boolean	gfMessageDrag,gfBoxDrag;

// define this to include the Resync button.  Remove this all eventually if it's not missed.
//#define SHOW_RESYNC_BUTTON

#pragma segment MboxWin

/************************************************************************
 * the structure for the mailbox window
 ************************************************************************/
typedef enum 
{ 
	NewMBIndex,
	NewFolderIndex,
	RemoveIndex,
#ifdef SHOW_RESYNC_BUTTON
	ResyncIndex,
#endif
	OptionsMenuIndex,
	ControlCount 
} ControlEnum;
typedef struct
{
	ViewList list;
	MyWindowPtr win;
	ControlHandle controls[ControlCount];
	Boolean dontTickle;
	Boolean	inited;
	Boolean	IMAPButtonsVisible;
#ifdef SHOW_RESYNC_BUTTON
	Boolean resyncEnabled;
	Boolean imapMailboxSelected;
#endif
} MBType;
MBType MB;

#define Win MB.win
#define DontTickle MB.dontTickle
#define Controls MB.controls
#ifdef SHOW_RESYNC_BUTTON
#define cntMBControls 5
#else
#define cntMBControls 4
#endif
#define CtlRemove MB.controls[RemoveIndex]
#define CtlNewMB MB.controls[NewMBIndex]
#define CtlNewFolder MB.controls[NewFolderIndex]
#ifdef SHOW_RESYNC_BUTTON
#define CtlResync MB.controls[ResyncIndex]
#endif
#define CtlOptionsMenu MB.controls[OptionsMenuIndex]

/************************************************************************
 *	Structure for remembering which folders are expanded.
 *  Saved as a resource in the settings file
 ************************************************************************/
struct ExpandListRec
{
	short	count;
	long	dirID[];
};
typedef struct ExpandListRec ExpandListRec;

ExpandInfo	gExpandList;

typedef struct
{
	short	cmd;
	short	mark;
	Style	style;
} MenuProperties;

/************************************************************************
 * prototypes
 ************************************************************************/
Boolean MBMenu(MyWindowPtr win, int menu, int item, short modifiers);
void MBDidResize(MyWindowPtr win, Rect *oldContR);
void MBZoomSize(MyWindowPtr win,Rect *zoom);
void MBUpdate(MyWindowPtr win);
Boolean MBClose(MyWindowPtr win);
short MBFill(void);
short MBFillSide(MenuHandle mh);
void MBClick(MyWindowPtr win,EventRecord *event);
void MBCursor(Point mouse);
void MBActivate(MyWindowPtr win);
void MBSetControls(void);
void MBHit(short which, Boolean optionPressed);
static void MBIdle(MyWindowPtr win);
static short WhichTree(VLNodeInfo *pInfo);
static void EnableIMAPButtons(MyWindowPtr win);
static Boolean MBKey(MyWindowPtr win, EventRecord *event);
void MBListOpen(ViewListPtr pView);
void MBListClose(short whichSide);
static void MBMenuToFile(short menuID, short *pVRef, long *pDirID);
static void MBMove(VLNodeInfo *pTargetInfo);
static void MBRemove(void);
static OSErr MBDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
short FindItemBySubmenu(MenuHandle mh, short subID);
short MBRefill(UPtr andSelect);
static void DoNewMailbox(Boolean fFolder);
Boolean DoRemoveBox(MenuHandle mh,UPtr name,Boolean multiple,Boolean *andShutUp,Boolean *needRefill);
static short DeleteMailFolder(MenuHandle mh,short item);
void DoMoveMailbox(UPtr selectName,short si);
Boolean SameSelected(void);
Boolean MBGetData(VLNodeInfo *data,short item);
void MBHelp(MyWindowPtr win,Point mouse);
//	void MBFixIndices(void);
static void SetExpandListSize(ExpandInfoPtr pExpList);
static void GetMenuItemProperties(MenuHandle hMenu,short item,MenuProperties *pFormat);
static void AddMBMenuItem(MenuHandle hMenu,StringPtr sName,MenuProperties *pFormat);
static void MBGrow(MyWindowPtr win,Point *newSize);
static Boolean MBFind(MyWindowPtr win,PStr what);
static Boolean IsSpecialBox(ViewListPtr pView, VLNodeInfo *pInfo);
Boolean IsIMAPTrashBox(VLNodeInfo *data);
OSErr MBTellOthersRename(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn);
OSErr GetSelectedMailboxSpecs(Handle *mailboxes, Boolean optionPressed);
OSErr GetSelectedChildMailboxSpecs(Accumulator *b, MailboxNodeHandle toAdd, Boolean childrenToo);
void EnableOptionsMenu(MyWindowPtr win, Boolean optionPressed);
char IMAPOptionsCheckMarkChar(MailboxNeedsEnum needs);
long IMAPMailboxIcon(VLNodeInfo	*info, short menuID);
void MBRefreshIMAPMailboxes(Boolean optionPressed);
OSErr MBResyncOrExpungeIMAPMailboxes(Boolean optionPressed, TaskKindEnum task);
void MBMarkIMAPMailboxes(Boolean on, MailboxNeedsEnum needs);
void MBCompactMailboxes(void);

/************************************************************************
 * OpenMBWin - open the mailbox window
 ************************************************************************/
void OpenMBWin(void)
{
	WindowPtr	WinWP = nil;
	
	if (SelectOpenWazoo(MB_WIN)) return;	//	Already opened in a wazoo

	if (!MB.inited)
	{
		short err=0;
		Rect r;
		
		if (!(Win=GetNewMyWindow(MBWIN_WIND,nil,nil,BehindModal,False,False,MB_WIN)))
			{err=MemError(); goto fail;}
			
		WinWP = GetMyWindowWindowPtr (Win);
		
		SetWinMinSize(Win,-1,-1);
		SetPort_(GetMyWindowCGrafPtr(Win));
		ConfigFontSetup(Win);	
		MySetThemeWindowBackground(Win,kThemeListViewBackgroundBrush,False);

		/*
		 * controls
		 */
		if (!(CtlNewMB = NewIconButton(MB_NEWMB_CNTL,WinWP)) ||
			!(CtlNewFolder = NewIconButton(MB_NEWFOLDER_CNTL,WinWP)) ||
			!(CtlRemove = NewIconButton(MB_REMOVE_CNTL,WinWP)) ||
#ifdef SHOW_RESYNC_BUTTON
			!(CtlResync = NewIconButton(MB_RESYNC_CNTL,WinWP)) ||
#endif
			!(CtlOptionsMenu = NewIconButtonMenu(MAILBOX_OPTIONS_MENU,REFRESH_ICON,WinWP)))
				goto fail;

#ifdef SHOW_RESYNC_BUTTON		
		MB.resyncEnabled  = true;
#endif
		MB.IMAPButtonsVisible = true;
		MBIdle(Win);
		
		/*
		 * list
		 */
		gExpandList.resID = kMBExpandID;
		SetRect(&r,0,0,20,20);
		if (LVNew(&MB.list,Win,&r,0,MailboxesLVCallBack,kMBDragType))
			{err=MemError(); goto fail;}
		
//		if (MBFill()) goto fail;
		Win->didResize = MBDidResize;
		Win->close = MBClose;
		Win->update = MBUpdate;
		Win->position = PositionPrefsTitle;
		Win->click = MBClick;
		Win->bgClick = MBClick;
		Win->dontControl = True;
		Win->cursor = MBCursor;
		Win->activate = MBActivate;
		Win->help = MBHelp;
		Win->menu = MBMenu;
		Win->key = MBKey;
		Win->app1 = MBKey;
		Win->drag = MBDragHandler;
		Win->zoomSize = MBZoomSize;
		Win->grow = MBGrow;
		Win->find = MBFind;
		Win->idle = MBIdle;
		Win->showsSponsorAd = true;
		ShowMyWindow(WinWP);
		MyWindowDidResize(Win,&Win->contR);
		MB.inited = true;
		return;
		
		fail:
			if (WinWP) CloseMyWindow(WinWP);
			if (err) WarnUser(COULDNT_MBOX,err);
			return;
	}
	else
		WinWP = GetMyWindowWindowPtr (Win);
	UserSelectWindow(WinWP);
}
#pragma segment MboxWin
/************************************************************************
 * MBDidResize - resize the MB window
 ************************************************************************/
void MBDidResize(MyWindowPtr win, Rect *oldContR)
{
#define kListInset 10 
#pragma unused(oldContR)
	Rect r;
	short	htAdjustment;

	//	buttons
	PositionBevelButtons(win,cntMBControls,MB.controls,kListInset,Win->contR.bottom - INSET - kHtCtl,kHtCtl,RectWi(win->contR));
	
	// list
	SetRect(&r,kListInset,win->topMargin+kListInset,Win->contR.right-kListInset,Win->contR.bottom - 2*INSET - kHtCtl);
	if (win->sponsorAdExists && r.bottom >= win->sponsorAdRect.top - kSponsorBorderMargin)
		r.bottom = win->sponsorAdRect.top - kSponsorBorderMargin;
	LVSize(&MB.list,&r,&htAdjustment);
	
	/*
	 * redraw
	 */
	MBSetControls();
	InvalContent(win);
}

/************************************************************************
 * MBOpenFolder - open and selected the folder indicated by the menu ID
 ************************************************************************/
void MBOpenFolder(Handle hStringList,Boolean IsIMAP)
{
	StringPtr	pString,sSelect;
	VLNodeID	id;
	short			selectID = 0;	
	
	OpenMBWin();	//	Make sure the MB window is open and in front

	//	Expand each folder
	id = IsIMAP ? kIMAPFolder: kEudoraFolder;
	for(pString = LDRef(hStringList);*pString;pString += *pString+1)
	{
		LVExpand(&MB.list,id,pString,true);
		sSelect = pString;
		selectID = id;
		id = id==kEudoraFolder ? MAILBOX_MENU : MBGetFolderMenuID(id==kIMAPFolder?MAILBOX_MENU:id,pString);
	}
	
	if (selectID)
		LVSelect(&MB.list,selectID,sSelect,false);
}

/************************************************************************
 * MBZoomSize - zoom to only the maximum size of list
 ************************************************************************/
void MBZoomSize(MyWindowPtr win,Rect *zoom)
{
	short zoomHi = zoom->bottom-zoom->top;
	short zoomWi = zoom->right-zoom->left;
	short hi, wi;
	
	LVMaxSize(&MB.list, &wi, &hi);
	wi += 2*kListInset;
	hi += 2*kListInset + INSET + ControlHi(CtlNewMB);

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;
}

/************************************************************************
 * MBGrow - adjust grow size
 ************************************************************************/
static void MBGrow(MyWindowPtr win,Point *newSize)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect	r,rWin;
	short	htControl = ControlHi(CtlNewMB);
	short	bottomMargin,sponsorMargin;
	
	//	Get list position
	bottomMargin = INSET*2 + htControl;
	if (win->sponsorAdExists)
	{
		GetWindowPortBounds(winWP,&rWin);
		sponsorMargin = rWin.bottom - win->sponsorAdRect.top + kSponsorBorderMargin;
		if (sponsorMargin > bottomMargin) bottomMargin  = sponsorMargin;
	}
	SetRect(&r,kListInset,win->topMargin+kListInset,newSize->h-kListInset,newSize->v - bottomMargin);
	LVCalcSize(&MB.list,&r);
	
	//	Calculate new window height
	newSize->v = r.bottom + bottomMargin;
}

#pragma segment Main
/************************************************************************
 * MBClose - close the window
 ************************************************************************/
Boolean MBClose(MyWindowPtr win)
{
#pragma unused(win)
	
	if (MB.inited)
	{
		//	Save expanded folder list
		SaveExpandedFolderList(&gExpandList);

		//	Dispose of list
		LVDispose(&MB.list);
		MB.inited = false;
	}
	return(True);
}
#pragma segment MboxWin

/************************************************************************
 * MBUpdate - draw the window
 ************************************************************************/
void MBUpdate(MyWindowPtr win)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Rect	r;
	
	r = MB.list.bounds;
	DrawThemeListBoxFrame(&r,kThemeStateActive);
	LVDraw(&MB.list, MyGetPortVisibleRegion(winPort), true, false);
}

/************************************************************************
 * MBActivate - activate the window
 ************************************************************************/
void MBActivate(MyWindowPtr win)
{
#pragma unused(win)
	LVActivate(&MB.list, Win->isActive);
}

/**********************************************************************
 * MBFind - find in the Mailboxes window
 **********************************************************************/
static Boolean MBFind(MyWindowPtr win,PStr what)
{
	return FindListView(win,&MB.list,what);
}

/************************************************************************
 * MBRefill - refill current menus
 ************************************************************************/
short MBRefill(UPtr andSelect)
{
	short err;

	/*
			If andSelect specified, find and select it.
			Else If there was a selection before, select it
	*/

	err = MBFillSide(GetMHandle(MAILBOX_MENU));
	return(err);
}

/************************************************************************
 * MBFill - fill the windows
 ************************************************************************/
short MBFill(void)
{
	short err;
	MenuHandle mh=GetMHandle(MAILBOX_MENU);
	if (err=MBFillSide(mh)) return(err);
	return(noErr);
}

/************************************************************************
 * MBFillSide - fill the list from a menu
 ************************************************************************/
short MBFillSide(MenuHandle mh)
{
	InvalidListView(&MB.list);
	return noErr;
}

/************************************************************************
 * MBKey - key stroke
 ************************************************************************/
static Boolean MBKey(MyWindowPtr win, EventRecord *event)
{
#pragma unused(win)
	short key = (event->message & 0xff);

	if (LVKey(&MB.list,event))
	{
		MBSetControls();
		return true;
	}

	return false;
}

/************************************************************************
 * MBClick - click in mailboxes window
 ************************************************************************/
void MBClick(MyWindowPtr win,EventRecord *event)
{
//#pragma unused(win)
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	short i;
	Point pt;
	
	SetPort(GetMyWindowCGrafPtr(win));

	if (!LVClick(&MB.list,event))
	{
		pt = event->where;
		GlobalToLocal(&pt);
		if (!win->isActive)
		{
			SelectWindow_(winWP);
			UpdateMyWindow(winWP);	//	Have to update manually since no events are processed
		}
		for (i=0;i<ControlCount;i++)
		{
			GetControlBounds(Controls[i],&r);
			if (PtInRect(pt,&r))
			{
				// properly enable and disable the menu when it's selected
				if (Controls[i] == CtlOptionsMenu)
					EnableOptionsMenu(Win, ((event->modifiers&optionKey)!=0));
				
				if (!ControlIsGrey(Controls[i]) &&
						TrackControl(Controls[i],pt,(void *)(-1)))
					MBHit(i,(event->modifiers&optionKey) != 0);				
	
					AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),i), event->what);	
	
					break;
			}
		}
	}

	MBSetControls();

}

/************************************************************************
 * MBCursor - set the cursor properly for the mailboxes window
 ************************************************************************/
void MBCursor(Point mouse)
{
	if (!PeteCursorList(Win->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * MBSetControls - enable or disable the controls, based on current situation
 ************************************************************************/
void MBSetControls(void)
{
	Boolean fSelect,fRemove;
	Boolean fAdd = true;
	Boolean isIMAPBox;
	short	i;
	VLNodeInfo data;
		
	fSelect = LVCountSelection(&MB.list)!=0;
	Win->hasSelection = fSelect;

	//	Determine if "Remove" button needs to be greyed
	if (fRemove = fSelect)
	{
		//	Make sure that selection doesn't include any non-removeable items
		short	i;
		VLNodeInfo data;
		
		for (i=1;i<=LVCountSelection(&MB.list);i++)
		{
			MBGetData(&data,i);
			if ((IsSpecialBox(&MB.list,&data)) 
				|| data.nodeID == kIMAPFolder							// Can't delete IMAP personalities
				|| (IsIMAPBox(&data) && !CanModifyMailboxTrees()))		// Can't delete IMAP boxes while the tree is in use
			{
				//	Here's an item that can't be removed
				fRemove = false;
				break;
			}		
		}
	}
	SetGreyControl(CtlRemove,!fRemove);	
	
	// Determine if the "Add" buttons need to be greyed.
	
	// see what's selected
#ifdef SHOW_RESYNC_BUTTON
	MB.imapMailboxSelected = false;
#endif 
	for (i=1;i<=LVCountSelection(&MB.list);i++)
	{
		MBGetData(&data,i);
		isIMAPBox = IsIMAPBox(&data);
		
		if (isIMAPBox)
		{	
			// Can't add mailboxes when the tree is in use
			if (!CanModifyMailboxTrees())		
			{
				//	Here's an item that can't be removed
				fAdd = false;
				break;
			}

#ifdef SHOW_RESYNC_BUTTON
			// but we can enable the resync control
			MB.imapMailboxSelected = true;
#endif
		}		
	}
	// properly activate Refresh and Resync buttons
	EnableIMAPButtons(Win);
	
	SetGreyControl(CtlNewMB,!fAdd);	
	SetGreyControl(CtlNewFolder,!fAdd);
}

/************************************************************************
 * MBHit - a control item was hit
 ************************************************************************/
void MBHit(short which, Boolean optionPressed)
{
	Boolean resyncPlease = false;
	SInt16 value=0;
	MenuHandle menu = nil;
	short markChar=0;
						
	switch (which)
	{
		case RemoveIndex:
			MBRemove();
			break;

		case NewMBIndex:
			DoNewMailbox(false);
			break;

		case NewFolderIndex:
			DoNewMailbox(true);
			break;

		case OptionsMenuIndex:
			if (!GetControlData(CtlOptionsMenu,1,kControlBevelButtonMenuValueTag,sizeof(SInt16),(Ptr)&value,nil) && !GetControlData(CtlOptionsMenu,1,kControlBevelButtonMenuHandleTag,sizeof(menu),(Ptr)&menu,nil) && menu)
			{
				switch(value)
				{
					case OPTIONS_RESYNCSUB_ITEM:
						optionPressed = true;
					case OPTIONS_RESYNC_ITEM:
						resyncPlease = true;
						break;
						
					case OPTIONS_AUTOSYNC_ITEM:
						GetItemMark(menu,value,&markChar);
						MBMarkIMAPMailboxes((markChar==0), kNeedsPoll);
						break;
						
					case OPTIONS_REFRESH_ITEM:
						MBRefreshIMAPMailboxes(optionPressed);
						break;
						
					case OPTIONS_SHOWDELETED_ITEM:
						GetItemMark(menu,value,&markChar);
						MBMarkIMAPMailboxes((markChar==0), kShowDeleted);
						break;
						
					case OPTIONS_EXPUNGE_ITEM:
						MBResyncOrExpungeIMAPMailboxes(false, IMAPMultExpungeTask);
						break;
						
					case OPTIONS_COMPACT_ITEM:
						MBCompactMailboxes();
						break;
					
					default:
						break;	
				}
			}
			
			// fall through if resync was requested through the menu
			if (!resyncPlease)	
				break;
				
#ifdef	SHOW_RESYNC_BUTTON
		case ResyncIndex:
#endif
			MBResyncOrExpungeIMAPMailboxes(optionPressed, IMAPMultResyncTask);
			break;
	}
}

/************************************************************************
 * GetSelectedMailboxSpecs - build a list of selected mailboxes to process.
 ************************************************************************/
OSErr GetSelectedMailboxSpecs(Handle *mailboxes, Boolean optionPressed)
{
	OSErr err = noErr;
	Accumulator boxes;
	short count, i;
	VLNodeInfo data;
	FSSpec spec;
	MailboxNodeHandle mbox;
	PersHandle selectedPers = nil, pers;
			
	*mailboxes = NULL;
	
	if (count = LVCountSelection(&MB.list))
	{
		err = AccuInit(&boxes);
		if (err == noErr)
		{
			// collect all selected mailboxes
			for (i=1;i<=count;i++)
			{			
				//	get mailbox spec
				MBGetData(&data,i);
				MakeMBSpec(&data,&spec);
									
				// option-refresh on an IMAP account refreshes all mailboxes.
				if (optionPressed && (data.nodeID == kIMAPFolder))
				{
					selectedPers = FindPersByName(spec.name);
					GetSelectedChildMailboxSpecs(&boxes, (*selectedPers)->mailboxTree, true);
				}
					
				// is this an IMAP mailbox?
				if (IsIMAPBox(&data))
				{
					spec.parID = SpecDirId(&spec);
					LocateNodeBySpecInAllPersTrees(&spec, &mbox, &pers);
					
					// skip this mailbox if t belongs to the last IMAP account we added to the list.
					if (pers != selectedPers)
					{
						if (mbox)
						{
							GetSelectedChildMailboxSpecs(&boxes, mbox, optionPressed);
						}
					}
				}
			}	
					
			AccuTrim(&boxes);
			*mailboxes = boxes.data;
		}
	}
	// else
		// nothing was selected.  Return nothing.
		
	return (err);
}

/************************************************************************
 * GetSelectedChildMailboxSpecs - add the mailbox and the children, if
 *	appropriate, to the list.
 ************************************************************************/
OSErr GetSelectedChildMailboxSpecs(Accumulator *b, MailboxNodeHandle toAdd, Boolean childrenToo)
{
	OSErr err = noErr;
	FSSpec spec;
	MailboxNodeHandle scan;
	
	// check parameters
	if (toAdd && b)
	{
		// add this mailbox if it's really a mailbox
		if ((*toAdd)->mailboxName)
		{
			spec = ((*toAdd)->mailboxSpec);	
			err = AccuAddPtr(b, &spec, sizeof(FSSpec));
		
			// add the children if we ought
			if ((err == noErr) && childrenToo)
			{
				if (scan = (*toAdd)->childList)
				{
					while (scan && (err == noErr))
					{
						err = GetSelectedChildMailboxSpecs(b, scan, true);
						scan = (*scan)->next;
					}
				}
			}
		}
		else if (childrenToo)
		{
			// This is an IMAP account.  Add all of the mailboxes.
			scan = (*toAdd)->next;
			while (scan && (err == noErr))
			{
				err = GetSelectedChildMailboxSpecs(b, scan, true);
				scan = (*scan)->next;
			}
		}
	}
		
	return (err);
}
								
/************************************************************************
 * MBListOpen - open some stuff from a list handle
 ************************************************************************/
void MBListOpen(ViewListPtr pView)
{
	short	i;
	VLNodeInfo data;
	FSSpec spec;
	
	for (i=1;i<=LVCountSelection(pView);i++)
	{
		LVGetItem(pView,i,&data,true);
		if (!data.isParent || data.iconID == kIMAPMailboxIcon || IsIMAPTrashBox(&data))
		{
			MakeMBSpec(&data,&spec);
			(void) GetMailbox(&spec,True);
		}
	}
}

/************************************************************************
 * MBFixUnread - Update and "unread" status of a mailbox
 ************************************************************************/
void MBFixUnread(MenuHandle mh,short item,Boolean unread)
{
	if (MB.inited)
		MBFixUnreadLo(&MB.list,mh,item,unread,GetWindowKind(GetMyWindowWindowPtr(MB.win)) == MB_WIN);
	SearchFixUnread(mh,item,unread);
	MBDrawerFixUnread(mh,item,unread);
}

/************************************************************************
 * MBFixUnread - Update and "unread" status of a mailbox
 ************************************************************************/
void MBFixUnreadLo(ViewListPtr pView,MenuHandle mh,short item,Boolean unread,Boolean draw)
{
	Str63	name;
	Style	theStyle = unread ? UnreadStyle : 0;
	short	menuID;
	Boolean	mailboxMenu = GetMenuID(mh) == MAILBOX_MENU;
	
	if (mailboxMenu && !item)
	{
		//	Update "Eudora Folder"
		GetMBDirName(Root.vRef,Root.dirId,name);	//	Name of Mail Folder				
		LVUpdateStyle(pView,kEudoraFolder,name,theStyle,draw);		
	}
	else
	{
		MyGetItem(mh,item,name);
		if (mailboxMenu && IMAPExists() && (menuID = SubmenuId(mh,item)))
		{
			short	vRef;
			long	dirID;

			MBMenuToFile(menuID,&vRef,&dirID);
			if (IsIMAPVD(vRef,dirID))
			{
				LVUpdateStyle(pView,kIMAPFolder,name,theStyle,draw);
				return;
			}
		}
		LVUpdateStyle(pView,GetMenuID(mh),name,theStyle,draw);
	}
}

/************************************************************************
 * DoNewMailbox - make a new mailbox or mailbox folder
 ************************************************************************/
static void DoNewMailbox(Boolean folder)
{
	FSSpec spec;
	short	vRefNum;
	long	dirID;
	VLNodeInfo data;
	short	menuID;
	Boolean imapSuccess = false;
	MenuHandle	mh;
	short	level,subMenuID;
	Str63 name;
	

	//	Determine which folder to put it in
	vRefNum = MailRoot.vRef;	//	Assume main mailboxes folder
	dirID = MailRoot.dirId;
	menuID = MAILBOX_MENU;
	
	if (MBGetData(&data,1))
	{
		// if the currently select node is an IMAP personality, make sure we have a mailbox tree
		if (data.nodeID == kIMAPFolder)
		{
			PersHandle oldPers = CurPers;
			
			// the personality is collapsed.  Do nothing.
			if (data.isCollapsed) return;
			
			if (CurPers = FindPersByName(data.name))
			{
				if (PrefIsSet(PREF_IS_IMAP))
					if (!MailboxTreeGood(CurPers)) 
						if (CreateLocalCache() != noErr) 
						{
							CurPers = oldPers;
							return;
						}
			}		
			CurPers = oldPers;
		}
		//	Put it in parent of first selected item or self if a folder
		MBMenuToFile(data.nodeID, &vRefNum, &dirID);
		menuID = data.nodeID;

		if ((!data.isParent || !data.isCollapsed) && IsIMAPBox(&data))
		{
			PersHandle pers = nil;
			MailboxNodeHandle node = nil;
			
			FSMakeFSSpec(vRefNum,dirID,data.name,&spec);
			spec.parID = SpecDirId(&spec);
			
			LocateNodeBySpecInAllPersTrees(&spec, &node, &pers);
			if (node && pers && CanHaveChildren(node))
			{
				dirID = spec.parID;
				menuID = MBGetFolderMenuID(data.nodeID,data.name);
			}
		}
		else
		{
			if (data.isParent && !data.isCollapsed)
			{
				FSMakeFSSpec(vRefNum,dirID,data.name,&spec);
				dirID = SpecDirId(&spec);
				if (!dirID) dirID = spec.parID;
				menuID = MBGetFolderMenuID(data.nodeID,data.name);
			}
		}
	}	

	//	Warn user if we will be more than 5 levels deep
	subMenuID = menuID;
	mh=GetMenuHandle(subMenuID);
	for(level=0;mh;level++)
		mh=ParentMailboxMenu(mh,&subMenuID);
	
	if (level>5 && !PrefIsSet(PREF_NO_MAILBOX_LEVEL_WARNING) &&
		!Mom(CREATE,0,PREF_NO_MAILBOX_LEVEL_WARNING,R_FMT,MAILBOX_LEVEL_WARNING))
			return;	//	too deep

	//	Make a unique "untitled" name
	MakeUniqueUntitledSpec (vRefNum, dirID, folder ? UNTITLED_FOLDER : UNTITLED_MAILBOX, &spec);

	// try to add this mailbox as an IMAP mailbox first.
	if (IMAPAddMailbox(&spec, folder, &imapSuccess, false))
	{
		if (imapSuccess)
		{
			MBTickle(nil,nil);
			menuID = MBGetFolderMenuID(data.nodeID,data.name);
			LVRename(&MB.list,menuID,spec.name,false,false);	//	Allow user to rename the untitled mailbox/folder
		}
	}
	else
	{
		PSCopy(name,spec.name);	// BadMailboxName steps on the name if it's a folder
		if (!BadMailboxName(&spec,folder))	//	Creates the mailbox/folder
		{
			if (!folder)
			{
				AddBoxHigh(&spec);
			}
			else
			{
				BuildBoxMenus();
				MBTickle(nil,nil);
			}
			
			LVRename(&MB.list,menuID,name,false,false);	//	Allow user to rename the untitled mailbox/folder
		}
	}
}

/************************************************************************
 * MBRemove - delete selected mailbox(es)
 ************************************************************************/
static void MBRemove(void)
{
	short	i;
	Boolean needRefill=false;
	Boolean andShutUp=false;
	Boolean	fFolder=false;
	short n;

	for (i=1;i<=(n=LVCountSelection(&MB.list));i++)
	{
		VLNodeInfo data;
		
		MBGetData(&data,i);
		if ((data.nodeID != kIMAPFolder) && (!IsSpecialBox(&MB.list,&data)))
		{
			if (!DoRemoveBox(GetMHandle(data.nodeID),data.name,i<n,&andShutUp,&needRefill)) break;
			if (data.isParent)
				fFolder = true;
		}
	}

	if (fFolder)
		BuildBoxMenus();
	
	if (needRefill)
		MBTickle(nil,nil);
}

/************************************************************************
 * MBRename - rename selected mailbox
 ************************************************************************/
static void MBRename(StringPtr newName)
{
	VLNodeInfo data;
	short	vRef;
	long	dirID;
	FSSpec	spec,origSpec,newSpec;
	TOCHandle tocH;
	Boolean isFolder;
	OSErr	err=0;
	Str255 suffix;
	
	// complain if the mailbox name is too long
	if (*newName>31-*GetRString(suffix,TOC_SUFFIX))
	{
		TooLong(newName);
		goto Done;
	}
	
	if (MBGetData(&data,1) && *newName && !EqualString(newName,data.name,true,true))
	{
		MBMenuToFile(data.nodeID, &vRef, &dirID);
		FSMakeFSSpec(vRef,dirID,data.name,&spec);
		SimpleMakeFSSpec(vRef,dirID,newName,&newSpec);
		IsAlias(&spec,&origSpec);
		isFolder=FSpIsItAFolder(&origSpec);
		
		if (!isFolder && (!(tocH=TOCBySpec(&spec)) || (*tocH)->which))
		{
			err = WarnUser(MAYNT_RENAME_BOX,0);
			goto Done;
		}
		
		// complain if the mailbox name has some inappropriate characters
		if (BadMailboxNameChars(&newSpec))
			goto Done;
		
		// rename the IMAP mailbox
		if (IsIMAPCacheFolder(&spec)) 
		{
			MBTellOthersRename(&spec,&newSpec,True,True,false);
			IMAPRenameMailbox(&spec, newSpec.name);
			MBTellOthersRename(&spec,&newSpec,True,False,false);
			goto Done;
		}
		
		
//		if (!StringSame(spec.name,newName))
//			HDelete(spec.vRefNum,spec.parID,newName);

		if (isFolder)
		{			
			MBTellOthersRename(&spec,&newSpec,True,True,false);
			if (err = HRename(spec.vRefNum,spec.parID,spec.name,newName))
			{
				(FileSystemError(RENAMING_BOX,spec.name,err));
				goto Done;
			}
			BuildBoxMenus();
			MBTickle(nil,nil);
			MBTellOthersRename(&spec,&newSpec,True,False,false);
		}
		else
		{
			MBTellOthersRename(&spec,&newSpec,False,True,false);
			if (err = RenameMailbox(&spec,newName,isFolder)) goto Done;
			if (tocH)
			{
				TOCSetDirty(tocH,true);
				PCopy((*tocH)->mailbox.spec.name,newName);
				SetWTitle_(GetMyWindowWindowPtr((*tocH)->win),newName);
			}
			DontTickle = True;
			RemoveBoxHigh(&spec);
			DontTickle = False;
			AddBoxHigh(&newSpec);
			MBTellOthersRename(&spec,&newSpec,False,False,false);
			TellSearchMBRename(&spec,&newSpec);
		}
	}
 Done:
	if (err)
		InvalContent(Win);	//	Clean up window
}

/************************************************************************
 * ParentMailboxMenu - find the menu enclosing this one
 ************************************************************************/
MenuHandle ParentMailboxMenu(MenuHandle mh,short *itemPtr)
{
	short myID, curID;
	
	curID = myID = GetMenuID(mh);
	while (--curID && (mh = GetMHandle(curID)))
	{
		if (*itemPtr = FindItemBySubmenu(mh,myID)) return(mh);
	}
	
	mh = GetMHandle(MAILBOX_MENU);
	if (*itemPtr = FindItemBySubmenu(mh,myID)) return(mh);
	return(nil);
}

/************************************************************************
 * FindItemBySubmenu - find a menu item with a given submenu
 ************************************************************************/
short FindItemBySubmenu(MenuHandle mh, short subID)
{
	short item, newID;
	for (item=CountMenuItems(mh);item;item--)
	{
		if (HasSubmenu(mh,item))
		{
			newID = SubmenuId(mh,item);
			if (newID==subID) return(item);
		}
	}
	return(0);
}


/************************************************************************
 * DoRemoveBox - interact with the user before removing a mailbox
 ************************************************************************/
Boolean DoRemoveBox(MenuHandle mh,UPtr name,Boolean multiple,Boolean *andShutUp,Boolean *needRefill)
{
	Boolean isFolder=False;
	Boolean isEmpty;
	short clicked;
	FSSpec spec, newSpec;
	CInfoPBRec hfi;
	FInfo info;
	short res;
	Boolean trashChain=False;
	Boolean justTrash=False;
	Boolean isIMAPBox = false;

	GetTransferParams(GetMenuID(mh),FindItemByName(mh,name),&spec,nil);
	FSpGetFInfo(&spec,&info);
	isFolder = FSpIsItAFolder(&spec);	
	isIMAPBox = IsIMAPMailboxFile(&spec);

	/*
	 * is it an alias?
	 */
	if (!isFolder && info.fdFlags & kIsAlias)
	{
		if (!IsAlias(&spec,&newSpec))
		{
			/*
			 * ok, here we have an alias that points nowhere.  just trash it.
			 */
			isEmpty = True;
			trashChain = False;
			justTrash = True;
		}
		else
		{
			res = AlertStr(ALIAS_OR_REAL_ALRT,Note,spec.name);
			if (res==aorCancel) return(False);
			trashChain = (res==aorBoth);
			if (trashChain) isFolder = info.fdType==kContainerFolderAliasType;
			else justTrash = isEmpty = True;
		}
	}			

	if (!justTrash)
	{
		if (!isFolder)
		{
			/*
			 * is the box empty?
			 */
			if (AFSpGetHFileInfo(&spec,&hfi)) isEmpty = True;
			else isEmpty = hfi.hFileInfo.ioFlLgLen==0;
		}
		else isEmpty = FolderFileCount(&spec)<=0;

		// if this is an IMAP mailbox, make sure this mailbox really is empty
		if (isEmpty && isIMAPBox) isEmpty = IsIMAPMailboxEmpty(&spec);		
	}
	
	if (!justTrash && !*andShutUp)
	{
		clicked = AlertStr(isEmpty?
													(multiple ? DELETE_EMPTY_ALRT : ALRTStringsOnlyStrn+DELETE_EMPTY_SINGLE_ASTR)
												: (multiple ? (isIMAPBox?DELETE_NON_EMPTY_IMAP_ASTR+ALRTStringsOnlyStrn:DELETE_NON_EMPTY_ALRT) : ALRTStringsOnlyStrn+(isIMAPBox?DELETE_NON_EMPTY_SINGLE_IMAP_ASTR:DELETE_NON_EMPTY_SINGLE_ASTR)),
											 Caution,name);
		if (clicked==DELETE_REMOVE_ALL) *andShutUp = True;
		else if (clicked!=DELETE_REMOVE_IT) return(False);
	}

	// if this is an IMAP folder, then do the IMAP thing
	if (isIMAPBox) 
	{
		Boolean result = false;
		
		// close any child mailboxes ...
		IMAPCloseChildMailboxes(&spec);
		
		DontTickle = True;
		result = IMAPDeleteMailbox(&spec);
		DontTickle = False;
		*needRefill = True;
		return (result);
	}

	if (isFolder)
	{
		short item = FindItemByName(mh,name); 
		if (DeleteMailFolder(mh,item)) return(False);
		*needRefill = True;
	}
	else
	{
		RemoveMailbox(&spec,trashChain);
		DontTickle = True;
		RemoveBoxHigh(&spec);
		DontTickle = False;
		*needRefill = True;
	}
	return(True);
}


/************************************************************************
 * DeleteMailFolder - move a mail folder to the trash
 ************************************************************************/
static short DeleteMailFolder(MenuHandle mh,short item)
{
	short err=0;
	FSSpec spec, realSpec;
	
	GetTransferParams(GetMenuID(mh),item,&spec,nil);

	if (IsAlias(&spec,&realSpec))
			FSpTrash(&realSpec);	//	Was an alias, move real folder to trash, then alias
	err = FSpTrash(&spec);
	return(err);
}

/************************************************************************
 * MBMove - move selected mailbox(es) to target
 ************************************************************************/
static void MBMove(VLNodeInfo *pToInfo)
{
	short	i,n;
	short	toVRef;
	long	toDirID;
	Boolean	fChanged = false;
	Boolean	fFolder = false;
	Boolean	fMovedFolder;
	FSSpec	spec, newSpec;
	OSErr	moveErr=nil;
	short	fromVRef,flushVRef=0;
	long	fromDirID;
	VLNodeInfo	data,folderData;
	Boolean	ignoreFilters = false;
	Boolean	dontWarn = false;

	if (pToInfo->nodeID==kEudoraFolder)
	{
		//	Eudora Folder
		toVRef = MailRoot.vRef;
		toDirID = MailRoot.dirId;
	}
	else
	{
		MBMenuToFile(pToInfo->nodeID, &toVRef, &toDirID);
		FSMakeFSSpec(toVRef,toDirID,pToInfo->name,&spec);
		IsAlias(&spec,&spec);
		toDirID = SpecDirId(&spec);
		toVRef = spec.vRefNum;	//	In case it was alias to another volume
	}
	
	n = LVCountSelection(&MB.list);		
	fMovedFolder = false;
	for (i=1;i<=n && !moveErr;i++)
	{
		MBGetData(&data,i);
		if (IsSpecialBox(&MB.list,&data))
			//	Can't move this one
			continue;
		
		MBMenuToFile(data.nodeID, &fromVRef, &fromDirID);

		// If the mailbox to be moved is an IMAP box, allow it to be moved on the same server only
		if (IsIMAPBox(&data))
		{
			FSSpec fromSpec, toSpec;
			
			SimpleMakeFSSpec(fromVRef, fromDirID, data.name, &fromSpec);			
			SimpleMakeFSSpec(toVRef, toDirID, pToInfo->name, &toSpec);
			
			MBTellOthersRename(&spec,&newSpec,true,true,false);		
			if (!IMAPMoveMailbox(&fromSpec, &toSpec, i==n, i>n, &dontWarn)) return;
			MBTellOthersRename(&spec,&newSpec,true,false,dontWarn);
					
			continue;
		}
		
		// we don't allow moves of local mailboxes *to* an IMAP server
		if (pToInfo->nodeID == kIMAPFolder)
		{
			WarnUser(MOVE_MAILBOX,0);
			return;		
		}
		
		// and we don't allow moves of local mailboxes to an IMAP folder
		if (IsIMAPBox(pToInfo))
		{
			WarnUser(MOVE_MAILBOX,0);
			return;			
		}
		
		if (toVRef != fromVRef)
		{
			WarnUser(CANT_VOL_MOVE,0);
			return;
		}

		if (fMovedFolder && LVDescendant(&MB.list,&data,&folderData))
			//	We have already moved an ancestor of this file. Don't
			//	try to move it again
			continue;

		if (fMovedFolder = data.isParent)
		{
			//	Moved a folder
			folderData = data;
		}

		if (LVDescendant(&MB.list,&data,pToInfo)==1)
			//	Don't try to move a child into its parent--it's already there
			continue;
		
		//	Move file or directory
		if (data.isParent)
			fFolder = true;
		
		MBTellOthersRename(&spec,&newSpec,fFolder,true,false);

		if (!fFolder)
		{
			DontTickle = true;
			FSMakeFSSpec(fromVRef,fromDirID,data.name,&spec);
			RemoveBoxHigh(&spec);
			DontTickle = false;
		}
		
		moveErr=HMove(fromVRef,fromDirID,data.name,toDirID,nil);
		flushVRef = fromVRef;
		
		if (!fFolder)
		{
			//	If there was an error, we need to add the mailbox back in to its original location
			DontTickle = true;			
			if (!moveErr)
				FSMakeFSSpec(fromVRef,toDirID,data.name,&spec);
			AddBoxHigh(&spec);
			DontTickle = false;
		}

		if (moveErr)
		{
			FileSystemError(MOVE_MAILBOX,data.name,moveErr);
		}
		else
		{
				//	If file, move TOC also
			if (!data.isParent)
			{
				Str15 suffix;
				short err;
				TOCHandle tocH;
				FSSpec spec;
				
				PCat(data.name,GetRString(suffix,TOC_SUFFIX));
				if ((err=HMove(fromVRef,fromDirID,data.name,toDirID,nil)) &&
						err!=fnfErr && err!=paramErr && err!=bdNamErr)
					{FileSystemError(MOVE_MAILBOX,data.name,err); return;}
				*data.name -= *suffix;
				
				FSMakeFSSpec(fromVRef,fromDirID,data.name,&spec);
				if (tocH=FindTOC(&spec))
					(*tocH)->mailbox.spec.parID = toDirID;
			}
			fChanged = true;
			
			SimpleMakeFSSpec(fromVRef,fromDirID,data.name,&spec);
			SimpleMakeFSSpec(toVRef,toDirID,data.name,&newSpec);
			if (!ignoreFilters)
			{
				if (MBTellOthersRename(&spec,&newSpec,fFolder,false,dontWarn)==userCanceledErr)
					ignoreFilters = true;
				dontWarn = true;	//	one warning is enough
			}
			TellSearchMBRename(&spec,&newSpec);
		}
	}
	if (flushVRef)
		// Flush the volume so the directory is correct for
		// rebuilding the mailbox tree
		FlushVol(nil,flushVRef);
	if (fFolder)
		BuildBoxMenus();
	if (fChanged)
		MBTickle(nil,nil);
}

Boolean MBGetData(VLNodeInfo *data,short selectedItem)
{
	short junk=64;
	
	return LVGetItem(&MB.list,selectedItem,data,true);
}

void MBTickle(UPtr fromSelect,UPtr select)
{
	if (MB.inited  && !DontTickle)
	{
//		MBFixIndices();
		(void) MBRefill(select);
	}
	SearchMBUpdate();
	MBDrawerReload();
}

/************************************************************************
 * MBTellOthersRename - let the rest of the app know what we're up to
 ************************************************************************/
OSErr MBTellOthersRename(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn)
{
	
	if (MBRenameStack)
	{
		short n = (*MBRenameStack)->elCount;
		
		while (n--)
		{
			MBRenameProcPtr proc;
			OSErr err;
			
			StackItem(&proc,n,MBRenameStack);
			err = (*proc)(spec,newSpec,folder,will,dontWarn);
			if (err) return err;
		}
	}
	return noErr;
}
		

#pragma segment Balloon
/************************************************************************
 * MBHelp - provide help for the mailbox window
 ************************************************************************/
void MBHelp(MyWindowPtr win,Point mouse)
{
#pragma unused(win)
	if (PtInRect(mouse,&MB.list.bounds))
		MyBalloon(&MB.list.bounds,100,0,MBWIN_HELP_STRN+1,0,nil);
	else
		ShowControlHelp(mouse,MBWIN_HELP_STRN+2,CtlNewMB,CtlNewFolder,CtlRemove,CtlOptionsMenu,
#ifdef SHOW_RESYNC_BUTTON																		
																								CtlResync,
#endif
																										  nil);
}

#pragma segment MboxWin
/************************************************************************
 * MBMenu - menu choice in the mailbox window
 ************************************************************************/
Boolean MBMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(win,modifiers)
	
	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_OPENSEL_ITEM:
					MBListOpen(&MB.list);
					return(True);
					break;
			}
			break;

		case EDIT_MENU:
			switch(item)
			{
				case EDIT_SELECT_ITEM:
					if (LVSelectAll(&MB.list))
					{
						MBSetControls();
						return(true);
					}
					break;
				case EDIT_COPY_ITEM:
					LVCopy(&MB.list);
					return true;
			}
			break;
	}
	return(False);
}

/************************************************************************
 * MBMenuToFile - convert a menu id into a file reference
 ************************************************************************/
void MBMenuToFile(short menuID, short *pVRef, long *pDirID)
{
	if (menuID==kIMAPFolder)
	{
		*pVRef = IMAPMailRoot.vRef;
		*pDirID = IMAPMailRoot.dirId;
	}
	else
		MenuID2VD(menuID==kEudoraFolder?MAILBOX_MENU:menuID,pVRef,pDirID);
}

/**********************************************************************
 * MBDragHandler - handle drags
 *
 *  Can do internal dragging of mailboxes or external dragging and dropping
 *  of messages
 **********************************************************************/
static OSErr MBDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{	
#pragma unused(win)
	OSErr err = noErr;
	
	gfMessageDrag = DragIsInteresting(drag,MESS_FLAVOR,TOC_FLAVOR,nil);	//	Dragging messages
	gfBoxDrag = LVDragFlavor() == kMBDragType;	//	Dragging mailboxes
	
	if (!gfMessageDrag && !gfBoxDrag)
		return(dragNotAcceptedErr);	//	Nothing here we want
	
	switch (which)
	{
		case kDragTrackingEnterWindow:
		case kDragTrackingLeaveWindow:
		case kDragTrackingInWindow:
				err = LVDrag(&MB.list,which,drag);
			break;
		case 0xfff:
			//	Drop
			if (gfBoxDrag)
				//	Mailbox drag
				err = LVDrag(&MB.list,which,drag);
			else
			{
				//	Message drag
				VLNodeInfo targetInfo;
				
				if (LVDrop(&MB.list,&targetInfo))
					err = MBDragMessages(drag, &targetInfo);
				else
					return dragNotAcceptedErr;
				
			}
			break;
	}
	return err;
}

/**********************************************************************
 * MBDragMessages - handle message drags
  **********************************************************************/
OSErr MBDragMessages(DragReference drag, VLNodeInfo *targetInfo)
{
	FSSpec spec,toSpec;
	TOCHandle tocH;
	short sumNum;
	OSErr err;
	UHandle data=nil;
	
	//	Get spec of mailbox dropped on
	MakeMBSpec(targetInfo,&spec);
	
	if (!(err=MyGetDragItemData(drag,1,MESS_FLAVOR,(void*)&data)))
	{
		tocH = (***(MessHandle**)data)->tocH;
		sumNum = (***(MessHandle**)data)->sumNum;
	}
	else if (!(err=MyGetDragItemData(drag,1,TOC_FLAVOR,(void*)&data)))
	{
		tocH = **(TOCHandle**)data;
		sumNum = -1;
	}
	ZapHandle(data);
	
	//	Can't move to own mailbox
	toSpec = GetMailboxSpec(tocH,-1);
	if (toSpec.vRefNum==spec.vRefNum && toSpec.parID==spec.parID && StringSame(toSpec.name,spec.name))
		//	Dragging to own mailbox
		return dragNotAcceptedErr;
	
	if (!err)
	{
		if (sumNum==-1) MoveSelectedMessages(tocH,&spec,(DragOrMods(drag)&optionKey)!=0);
		else
		{
//								if (!(DragOrMods(drag)&optionKey)) EzOpen(tocH,sumNum,0,DragOrMods(drag),True,True);
			if (!(DragOrMods(drag)&optionKey)) {
#ifndef clarenceBug821
#ifdef TWO
				AddXfUndo(tocH,TOCBySpec(&spec),sumNum);
#endif
#endif
				EzOpen(tocH,sumNum,0,DragOrMods(drag),True,True);
			}
			MoveMessage(tocH,sumNum,&spec,(DragOrMods(drag)&optionKey)!=0);
		}
	}
	return(err);
}


/************************************************************************
 * MBGetExpList - return pointer to folder expansion list data
 ************************************************************************/
ExpandInfoPtr MBGetExpList(void)
{
	return &gExpandList;
}

/************************************************************************
 * MBGetList - return pointer to mailbox list
 ************************************************************************/
ViewListPtr MBGetList(void)
{
	return &MB.list;
}

/************************************************************************
 * FindExpandDirID - Find the indicated dirID in the expanded folder list
 ************************************************************************/
long *FindExpandDirID(long dirID, ExpandInfoPtr pExpList)
{
	short	i;
	long	*pDirID;

	if (!pExpList->hExpandList)
	{
		//	Get expanded folder list
		if (pExpList->resID && (pExpList->hExpandList = Get1Resource(kExpandListType,pExpList->resID)))
			DetachResource(pExpList->hExpandList);
		else
			//	Need to create a new one
			pExpList->hExpandList = NewHandleClear(2);

		pExpList->fExpandListChanged = false;
	}

	//	Find the dirID in the list
	for (i=0,pDirID=(*pExpList->hExpandList)->dirID;i<(*pExpList->hExpandList)->count;i++,pDirID++)
	{
		if (*pDirID == dirID)
			return pDirID;
	}
	return nil;
}

/************************************************************************
 * SaveExpandStatus - save status of expanded folders
 ************************************************************************/
void SaveExpandStatus(VLNodeInfo *data,ExpandInfoPtr pExpList)
{		
	long	*pDirID;


	pDirID = FindExpandDirID(data->refCon,pExpList);
	
	if (data->isCollapsed)
	{
		//	Remove from list
		if (pDirID)
		{
			(*pExpList->hExpandList)->count--;
			BMD(pDirID+1,pDirID,sizeof(long)*((*pExpList->hExpandList)->count-(pDirID-(*pExpList->hExpandList)->dirID)));
			SetExpandListSize(pExpList);
			pExpList->fExpandListChanged = true;
		}
	}
	else
	{
		//	Add to list
		if (!pDirID)
		{
			if (!PtrPlusHand_(&data->refCon,(Handle)pExpList->hExpandList,sizeof(long)))
			{
				(*pExpList->hExpandList)->count++;
				pExpList->fExpandListChanged = true;
			}
		}
	}
}

/************************************************************************
 * SetExpandListSize - set size based on count
 ************************************************************************/
static void SetExpandListSize(ExpandInfoPtr pExpList)
{
	SetHandleSize((Handle)pExpList->hExpandList,sizeof(short)+sizeof(long)*(*pExpList->hExpandList)->count);
}

/************************************************************************
 * MBGetFolderMenuID - get the menu ID for this folder
 ************************************************************************/
short MBGetFolderMenuID(short theID,StringPtr name)
{	
	short item,id;
	MenuHandle	mh;

	id = 0;
	if (theID==kEudoraFolder || theID==kIMAPFolder)
		theID = MAILBOX_MENU;
	if (mh = GetMenuHandle(theID))
	{
		item = FindItemByName(mh,name);
		id = SubmenuId(mh,item);
	}
	return(id);
}

/************************************************************************
 * SaveExpandedFolderList - save expanded folder list. Remove any folders
 * that no longer exist
 ************************************************************************/
void SaveExpandedFolderList(ExpandInfoPtr pExpList)
{
	if (pExpList->fExpandListChanged && pExpList->resID)
	{	
		short from,to;
		OSErr	err;

		//	Remove any directories from the list that no longer exist
		for(from=0,to=0;from<(*pExpList->hExpandList)->count;from++)
		{
			CInfoPBRec hfi;
			long	dirID;

			hfi.dirInfo.ioCompletion = nil;
			hfi.dirInfo.ioNamePtr = nil;
			hfi.dirInfo.ioVRefNum=MailRoot.vRef;
			hfi.dirInfo.ioDrDirID = dirID = (*pExpList->hExpandList)->dirID[from];
			hfi.dirInfo.ioFDirIndex=-1;
			if (!PBGetCatInfo((CInfoPBPtr)&hfi,false))
			{
				//	This one still exists. Use it
				(*pExpList->hExpandList)->dirID[to++] = dirID;
			}
		}

		if (from != to)
		{
			//	Looks like we deleted one or more directory ID's
			(*pExpList->hExpandList)->count = to;
			SetExpandListSize(pExpList);
		}

		ZapSettingsResource(kExpandListType,pExpList->resID);
		AddMyResource_((Handle)pExpList->hExpandList,kExpandListType,pExpList->resID,"");					
		if (!(err=ResError())) MyUpdateResFile(SettingsRefN);
		DetachResource((Handle)pExpList->hExpandList);
	}
	ZapHandle((Handle)pExpList->hExpandList);
}


/************************************************************************
 * AddMailboxListItems - add mailboxes to list
 ************************************************************************/
void AddMailboxListItems(ViewListPtr pView, short nodeId,ExpandInfoPtr pExpList)
{
	short i,n;
	Point c;
	short	menuID;
	VLNodeInfo	info;
	Str255	sTemp;
	MenuHandle	mh;
	Boolean	haveIMAP;
	Boolean	doingTopLvlIMAP=false;

	haveIMAP = IMAPExists();
	mh = GetMenuHandle(nodeId==0 || nodeId == kEudoraFolder ? MAILBOX_MENU : nodeId);
	n = CountMenuItems(mh);
	c.h = 0;
	SetPort_(GetMyWindowCGrafPtr(pView->wPtr));
	menuID = GetMenuID(mh);	
	i = (menuID==MAILBOX_MENU) ? 1: MAILBOX_FIRST_USER_ITEM-MAILBOX_BAR1_ITEM;

	if (nodeId==0)
	{
		//	First level. Add Eudora folder
		if (haveIMAP)
		{
			info.useLevelZero = false;
			info.isParent = true;
		}
		else
		{
			info.useLevelZero = true;
			info.isParent = false;
		}
		info.iconID = APP_ICON;
		info.nodeID = kEudoraFolder;
		info.isCollapsed = !FindExpandDirID(MailRoot.dirId,pExpList);
		info.style = 0;
		if (haveIMAP)
		{
			//	Any unread in the local mailboxes?
			short	count = CountMenuItems(mh);
			short	item;
			
			for(item=1;item<=count;item++)
			{
				Style	style;
				
				GetMenuItemText(mh, item, sTemp);
				if (!IsIMAPCacheName(sTemp))
				{
					GetItemStyle(mh,item,&style);
					if (style & UnreadStyle)
					{
						info.style = UnreadStyle;
						break;
					}
				}
			}
		}
		info.refCon = MailRoot.dirId;
					
		GetMBDirName(Root.vRef,Root.dirId,info.name);	//	Name of Mail Folder				

		LVAdd(pView, &info);

		//	If we have IMAP folders, add them at this level.
		//	Otherwise add all the POP stuff at this level
		if (haveIMAP)
		{
			for (i=MAILBOX_FIRST_USER_ITEM;i<=n;i++)
			{
					MyGetItem(mh,i,info.name);
					if (StringSame(info.name,"\p-")) break;	// stop at menu divider
			}
			i++;
			doingTopLvlIMAP = true;
		}
	}		

	info.useLevelZero = false;
	for (;i<=n;i++)
	{
		short	mailBoxIcon;
		Style	style;
		
		mailBoxIcon = kMailBoxIcon;
		if (menuID==MAILBOX_MENU)
		{
			switch (i)
			{
				case MAILBOX_IN_ITEM:	//	In box
					mailBoxIcon = IN_MB_ICON;
					break;
				case MAILBOX_OUT_ITEM:	//	Out box
					mailBoxIcon = OUT_MB_ICON;
					break;
				case MAILBOX_JUNK_ITEM:	//	Junk box
					//mailBoxIcon = JUNK_MB_ICON;
					break;
				case MAILBOX_TRASH_ITEM:	//	Trash
					mailBoxIcon = kTrashBoxIcon;
					break;
				case MAILBOX_BAR1_ITEM:
					i = MAILBOX_FIRST_USER_ITEM;
					break;
			}
		}
		
		if (i <= n)
		{
			short	vRef;
			long	dirID;

			MyGetItem(mh,i,info.name);
			if (menuID==MAILBOX_MENU && haveIMAP && StringSame(info.name,"\p-")) break;	// stop at menu divider
			info.nodeID = menuID;
			if (info.isParent = HasSubmenu(mh,i))
			{
				short	thisMenuID;

				thisMenuID = SubmenuId(mh,i);
				MBMenuToFile(thisMenuID, &vRef, &dirID);
				info.iconID = kFolderIcon;
				info.isCollapsed = info.isParent && !FindExpandDirID(dirID,pExpList);
				info.refCon = dirID;
			}
			else
			{
				info.iconID = mailBoxIcon;
				info.isCollapsed = false;
			}
			if (doingTopLvlIMAP)
			{
				info.nodeID = kIMAPFolder;
				info.iconID = APP_ICON;
			}
			else if (IsIMAPBox(&info))
				info.iconID = IMAPMailboxIcon(&info, menuID);
			
			if (info.iconID == kMailBoxIcon && !PrefIsSet(PREF_NO_CUSTOM_MB_ICONS))
			{
				Handle	resH;
				OSType junk;

				//	See if we have a custom icon
				SetResLoad(false);
				resH = GetNamedResource('ICN#',info.name);
				SetResLoad(true);
				if (resH)
				{
					short	resFile = HomeResFile(resH);
					
					//	Don't use icons from Eudora application or help file				
					if (resFile != AppResFile && resFile != HelpResFile)
						GetResInfo(resH,&info.iconID,&junk,sTemp);					
				}
			}

			GetItemStyle(mh,i,&style);
			info.style = style&UnreadStyle ? UnreadStyle : 0;
			LVAdd(pView, &info);
		}
	}
}

/************************************************************************
 * IMAPMailboxIcon - which icon should be used for this IMAP mailbox?
 ************************************************************************/
long IMAPMailboxIcon(VLNodeInfo	*info, short menuID)
{
	long id = kMailBoxIcon;
	short vRef;
	long dirID;
	FSSpec mailboxSpec;
	MailboxNodeHandle node = nil;
	PersHandle pers = nil;
	Str255 sTemp;
		
	MBMenuToFile(menuID,&vRef,&dirID);
	if (IsIMAPVD(vRef,dirID))
	{	
		SimpleMakeFSSpec(vRef, dirID, info->name, &mailboxSpec);
		mailboxSpec.parID = SpecDirId(&mailboxSpec);	
		LocateNodeBySpecInAllPersTrees(&mailboxSpec, &node, &pers);
		if (node)
		{					
			if (StringSame(info->name,GetRString(sTemp,IMAP_INBOX_NAME)) && !((*node)->childList))	// if this is the IMAP Inbox, and it can't have any children, use "In" icon
				id = IN_MB_ICON;
			else if (((*node)->attributes&LATT_TRASH) != 0) 										// if this is the trash mailbox, use the trash icon
				id = kTrashBoxIcon;
			else if (DoesIMAPMailboxNeed(node,kNeedsPoll))											// a polled mailbox
			{
				if ((*node)->childList)																
					id = kIMAPPolledFolderIcon;														// with children
				else
					id = kIMAPPolledMailboxIcon;													// without
			}
			else if (((*node)->attributes & LATT_NOSELECT))											// a strict folder
				id = kFolderIcon;
			else if ((*node)->childList)															// a regular IMAP mailbox folder
				id = kIMAPMailboxIcon;
		}
	}			
	return (id);
}

/************************************************************************
 * WhichTree - which tree (POP, IMAP account) does this file/folder belong to
 ************************************************************************/
static short WhichTree(VLNodeInfo *pInfo)
{
	MailboxNodeHandle	node;
	PersHandle	pers;
	FSSpec		spec;
	
	if (IMAPExists())
	{
		MakeMBSpec(pInfo,&spec);
		if (IsIMAPBox(pInfo))
		{
			spec.parID = SpecDirId(&spec);
			LocateNodeBySpecInAllPersTrees(&spec, &node, &pers);
			return (long)pers;
		}
	}

	//	POP mailbox
	return -1;
}

/************************************************************************
 * MailboxesLVCallBack - callback function for List View
 ************************************************************************/
long MailboxesLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data)
{
	OSErr	err = noErr;
	SendDragDataInfo	*pSendData;
	MBDragData	dragData;
	VLNodeInfo	*pInfo;
	MenuHandle	mh;
	short				nodeID;

	switch (message)
	{
		case kLVAddNodeItems:
			AddMailboxListItems(pView,data,&gExpandList);
			break;
		
		case kLVGetParentID:
			if (((VLNodeInfo *)data)->rowNum==1)
				return kEudoraFolder;
			nodeID = ((VLNodeInfo *)data)->nodeID;
			if (nodeID == kEudoraFolder || nodeID == kIMAPFolder) nodeID = MAILBOX_MENU;
			return MBGetFolderMenuID(nodeID,((VLNodeInfo *)data)->name);
			//break;
		
		case kLVOpenItem:
			MBListOpen(&MB.list);
			break;
			
		case kLVMoveItem:
			MBMove((VLNodeInfo*)data);
			break;

		case kLVDeleteItem:
			MBRemove();
			break;
		
		case kLVRenameItem:
			MBRename((StringPtr)data);
			break;
		
		case kLVQueryItem:
			pInfo = (	VLNodeInfo *)data;
			switch (pInfo->query)
			{
				case kQuerySelect:
					return true;;
				case kQueryDrag:
				{
					//	make sure we're not dragging items from different trees
					//	must be all POP or same IMAP tree
					long	thisTree = WhichTree(pInfo);
					
					if (!pView->dragGroup)
						pView->dragGroup = thisTree;	// this is the first item we've seen
					else if (pView->dragGroup != thisTree)
						return false;	// drag to different tree
				}
					//	fall through to common rename code
				case kQueryRename:
					if ((((VLNodeInfo*)data)->nodeID == kIMAPFolder)
						|| (IsIMAPBox(pInfo) && !CanModifyMailboxTrees())) return (false);
					return (!IsSpecialBox(pView,pInfo));
				case kQueryDrop:
				case kQueryDropParent:
					//	make sure we're not dragging items from different trees
					//	must be all POP or same IMAP tree
					if (gfBoxDrag && pView->dragGroup != WhichTree(pInfo))
						return false;	// drag to different tree

					// IMAP drop cases
					if (IsIMAPBox(pInfo))
					{
						IMAPMailboxAttributes att;
						FSSpec toSpec;
						
						// can't drop if we can't currently modify mailbox trees
						if (gfBoxDrag && !CanModifyMailboxTrees()) return (false);
						
						MBMenuToFile(pInfo->nodeID, &(toSpec.vRefNum), &(toSpec.parID));
						PCopy(toSpec.name, pInfo->name);
						toSpec.parID = SpecDirId(&toSpec);
							
						// can't drop if the destination can't have messages
						if (MailboxAttributes(&toSpec, &att))
						{
							if ((gfMessageDrag && att.noSelect) ||	//	Will MB accept messages?
								gfBoxDrag && att.noInferiors)	//	Will MB accept children?
									return false;
							else
								return true;
						}
					}
					if (pInfo->query==kQueryDrop)
					{
						if (pInfo->isParent || pInfo->rowNum == kItemEudoraFolder)
							//	Can accept mail box drags
							return gfBoxDrag;
						else
							//	Can accept message drags
							return gfMessageDrag;
					}
					else
						//	message==kQueryDropParent
						return gfBoxDrag;					
					break;
				case kQueryDragExpand:
						//	if we're dragging to a different tree, don't auto-expand
						//	folders
						if (gfBoxDrag && pView->dragGroup != WhichTree(pInfo))
							return false;	// drag to different tree
					return true;
				case kQueryDCOpens:
					//	If IMAP mailbox hybrid, double-click opens the mailbox
					//	instead of expanding/collapsing the folder
					return pInfo->iconID == kIMAPMailboxIcon || IsIMAPTrashBox(pInfo) || (!pInfo->isParent && pInfo->nodeID != kEudoraFolder);
			}
			break;
						
		case kLVCopyItem:
			break;

		case kLVExpandCollapseItem:
			if (((VLNodeInfo *)data)->rowNum==1 && (MainEvent.modifiers & optionKey))
			{
				FinderOpen(&SettingsSpec,true,false);
			}
			else
				SaveExpandStatus((VLNodeInfo *)data,&gExpandList);
			break;
		
		case kLVSendDragData:
			pSendData = (SendDragDataInfo *)data;
			PCopy(dragData.name,pSendData->info->name);
			dragData.menuID = pSendData->info->nodeID;
			if (mh = GetMenuHandle(dragData.menuID))
				dragData.menuItem = FindItemByName(mh,dragData.name);
				
			MakeMBSpec(pSendData->info,&dragData.spec);
			err = SetDragItemFlavorData(pSendData->drag, pSendData->itemRef, pSendData->flavor,&dragData, sizeof(dragData), 0L);
			break;

	}
	return err;
}

/************************************************************************
 * MakeMBSpec - make a file spec for a mailbox file
 ************************************************************************/
void MakeMBSpec(VLNodeInfo *pData,FSSpec *pSpec)
{
	short	vRef;
	long	dirID;
	Str32	name;

	MBMenuToFile(pData->nodeID, &vRef, &dirID);
	PCopy(name,pData->name);
	if (pData->nodeID == MAILBOX_MENU)
		switch(pData->rowNum)
		{
			//	Filename may be different than name in list if localized
			case kItemInBox: GetRString(name,IN); break;
			case kItemOutBox: GetRString(name,OUT); break;
			case kItemJunkBox: GetRString(name,JUNK); break;
			case kItemTrash: GetRString(name,TRASH); break;
		}	
	FSMakeFSSpec(vRef,dirID,name,pSpec);
}

/************************************************************************
 * IsIMAPBox - does this entry refer to an IMAP mailbox?
 ************************************************************************/
Boolean IsIMAPBox(VLNodeInfo *pData)
{
	short	vRef;
	long	dirID;

	if (pData->nodeID == kIMAPFolder)
		return true;
		
	MBMenuToFile(pData->nodeID, &vRef, &dirID);
	return IsIMAPVD(vRef,dirID);
}

/**********************************************************************
 * OpenMBFolder - open folders above this one in the mailbox list
 **********************************************************************/
void OpenMBFolder(ViewListPtr pView,short menuID,StringPtr s)
{
	MenuHandle	mh;
	
	if (menuID>0 && (mh = GetMenuHandle(menuID)))
	{
		short	item;
		
		if (menuID == MAILBOX_MENU)
		{
			if (IMAPExists())
			{
				//	Open Eudora Folder
				Str32		s;

				GetDirName(nil,Root.vRef,Root.dirId,s);			
				LVExpand(pView,kEudoraFolder,s,true);
			}
		}
		else
		{
			MenuHandle mhPar = ParentMailboxMenu(mh,&item);
			
			if (IMAPExists() && GetMenuID(mhPar) == MAILBOX_MENU)
			{
				short	vRef;
				long	dirID;

				MBMenuToFile(GetMenuID(mh), &vRef, &dirID);
				if (IsIMAPVD(vRef,dirID))
				{
					//	This is an IMAP account folder
					GetMenuTitle(mh,s);
					LVExpand(pView,kIMAPFolder,s,true);
					return;
				}			
			}			
			
			OpenMBFolder(pView,GetMenuID(mhPar),s);
			MyGetItem(mhPar,item,s);
			LVExpand(pView,GetMenuID(mhPar),s,true);
		}
	}
}

/**********************************************************************
 * MBIdle - MB idle function
 **********************************************************************/
static void MBIdle(MyWindowPtr win)
{
	EnableIMAPButtons(win);
}		

/**********************************************************************
 * MBIdle - see if we need to hide/show the IMAP refresh button
 **********************************************************************/
static void EnableIMAPButtons(MyWindowPtr win)
{
	Boolean	haveIMAP = IMAPExists();
	MenuHandle mh = nil;
	
	if (MB.IMAPButtonsVisible != haveIMAP)
	{
		SetControlVisibility(CtlOptionsMenu,haveIMAP,true);
#ifdef SHOW_RESYNC_BUTTON
		SetControlVisibility(CtlResync,haveIMAP,true);
#endif
		MB.IMAPButtonsVisible = haveIMAP;
	}
#ifdef SHOW_RESYNC_BUTTON
	if (MB.IMAPButtonsVisible)
	{
		// also grey out the Resync control if there's nothing selected
		if ( MB.imapMailboxSelected != MB.resyncEnabled)
		{
			MB.resyncEnabled = MB.imapMailboxSelected;
			SetGreyControl(CtlResync, !MB.imapMailboxSelected);
		}
	}
#endif
}

/**********************************************************************
 * EnableOptionsMenu - properly enable and disable items in the options
 *	menu based on what's selected in the mailboxes window
 **********************************************************************/
void EnableOptionsMenu(MyWindowPtr win, Boolean optionPressed)
{
	Boolean enabled;
	MenuHandle mh = nil;
	short count,i;
	VLNodeInfo data;
	Boolean imapMailboxSelected = false;
	Boolean imapPersSelected = false;
	Boolean popMailboxSelected = false;
	
	if (MB.IMAPButtonsVisible)
	{	
		// what is visible?
		if (count = LVCountSelection(&MB.list))
		{
			for (i=1;i<=count;i++)
			{			
				//	get mailbox spec
				MBGetData(&data,i);

				if (data.nodeID == kIMAPFolder)
					imapPersSelected = true;
				else if (IsIMAPBox(&data))
					imapMailboxSelected = true;
				else if (data.nodeID == kEudoraFolder)
					;
				else if (data.iconID == kFolderIcon)
					;
				else
					popMailboxSelected = true;	
			}
		}
		
		// properly enable the menu items
		if (!GetControlData(CtlOptionsMenu,kControlMenuPart,kControlBevelButtonMenuHandleTag,sizeof(mh),(Ptr)&mh,nil) && mh)
		{
			// prevent the user from doing a refresh if there's currently a background thread running.
			enabled = CanModifyMailboxTrees() && (imapMailboxSelected || imapPersSelected  || (CurrentModifiers()&optionKey));
			EnableIf(mh, OPTIONS_REFRESH_ITEM, enabled);
				
			// properly enable all IMAP-mailbox-specifc items while we're at it
			enabled = imapMailboxSelected;
			EnableIf(mh, OPTIONS_RESYNC_ITEM, enabled);
			EnableIf(mh, OPTIONS_RESYNCSUB_ITEM, enabled);
			EnableIf(mh, OPTIONS_AUTOSYNC_ITEM, enabled);
			EnableIf(mh, OPTIONS_SHOWDELETED_ITEM, enabled);
			EnableIf(mh, OPTIONS_EXPUNGE_ITEM, enabled);
			
			// set the check marks appropriately
			SetItemMark (mh, OPTIONS_AUTOSYNC_ITEM, IMAPOptionsCheckMarkChar(kNeedsPoll));
			SetItemMark (mh, OPTIONS_SHOWDELETED_ITEM, IMAPOptionsCheckMarkChar(kShowDeleted));
						
			// Compact menu item applies to POP mailboxes, too
			EnableIf(mh, OPTIONS_COMPACT_ITEM, (popMailboxSelected || imapMailboxSelected));
		}
	}
}

/**********************************************************************
 * IsSpecialBox - see if this is a special POP box (in, out, trash, Eudora folder)
 **********************************************************************/
static Boolean IsSpecialBox(ViewListPtr pView, VLNodeInfo *pInfo)
{
	VLNodeInfo	efInfo;
	if (pInfo->rowNum >= kItemMailBoxes) return false;
	
	//	Need to get status of Eudora folder
	LVGetItem(pView,1,&efInfo,false);
	if (efInfo.isParent && efInfo.isCollapsed)
		return pInfo->rowNum <= 1;
	else
		return (pInfo->rowNum < kItemMailBoxes);
}


/**********************************************************************
 * MBFindInCollapsed - search a collapsed folder
 **********************************************************************/
Boolean MBFindInCollapsed(MyWindowPtr win,ViewListPtr pView,PStr what,short menuID)
{
	CGrafPtr	winPort;
	MenuHandle	mh;
	short	item,count,id;
	Str255	s,name;

	if (mh = GetMenuHandle(menuID))
	{
		count = CountMenuItems(mh);
		for (item=1;item<=count;item++)
		{
			if (id = SubmenuId(mh,item))
			{
				//	has submenu. recurse
				if (MBFindInCollapsed(win,pView,what,id))
					return true;
			}
			MyGetItem(mh,item,name);
			if (FindStrStr(what,name)>=0)
			{
				// found
				// open collapsed folder(s) and select item
				OpenMBFolder(pView,menuID,s);
				LVSelect(pView,menuID,name,false);
				winPort = GetMyWindowCGrafPtr (win);
				LVDraw(pView,MyGetPortVisibleRegion(winPort),true,false);				
				return true;
			}
		}
	}
	return false;
}

/**********************************************************************
 * IsIMAPTrashBox - is this an IMAP trash mailbox?
 **********************************************************************/
Boolean IsIMAPTrashBox(VLNodeInfo *data)
{
	return (data->iconID == kTrashBoxIcon && IsIMAPBox(data));
}

/************************************************************************
 * IMAPOptionsCheckMarkChar - return the appropriate character to display 
 *	next to an IMAP mailboxes options menu item
 ************************************************************************/
char IMAPOptionsCheckMarkChar(MailboxNeedsEnum needs)
{
	char c = 0;
	short count, i, needsMark;
	VLNodeInfo data;
	FSSpec spec;
	MailboxNodeHandle mbox;
	PersHandle pers;
			
	
	if (count = LVCountSelection(&MB.list))
	{
		// count the number of mailboxes that need to be marked
		needsMark = 0;
		for (i=1;i<=count;i++)
		{			
			//	get mailbox spec
			MBGetData(&data,i);
			MakeMBSpec(&data,&spec);
				
			// is this an IMAP mailbox?
			if (IsIMAPBox(&data))
			{
				spec.parID = SpecDirId(&spec);
				LocateNodeBySpecInAllPersTrees(&spec, &mbox, &pers);
				
				if (pers && mbox && (DoesIMAPMailboxNeed(mbox,needs) || ((needs == kNeedsPoll) && (mbox == LocateInboxForPers(pers)))))
					needsMark++;	
			}
		}	
		
		// return the proper char
		if (needsMark)
		{
			if (count == needsMark)
				c = checkMark;
			else
				c = dashChar;
		}
	}
	return (c);
}

/************************************************************************
 * MBRefreshIMAPMailboxes - refresh IMAP mailbox trees.  That is, fetch
 *	the mailbox list from the server and adjust cache mailboxes as needed
 ************************************************************************/
void MBRefreshIMAPMailboxes(Boolean optionPressed)
{
	Boolean refresh = false;
	short count, i;
	VLNodeInfo data;
	FSSpec spec;
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
					
	// is there anything selected that can be refreshed?
	if (!optionPressed && (count = LVCountSelection(&MB.list)))
	{
		for (i=1;i<=count;i++)
		{
			//	get mailbox spec
			MBGetData(&data,i);
			MakeMBSpec(&data,&spec);
			
			// who does this mailbox belong to?
			if (data.nodeID == kIMAPFolder)
			{
				// an IMAP personality heading is selected.  Which personality does it refer to?
				pers = FindPersByName(spec.name);
			}
			else
			{
				// a mailbox is selected.  Who does it belong to?
				spec.parID = SpecDirId(&spec);
				LocateNodeBySpecInAllPersTrees(&spec, &node, &pers);
			}
			
			// if it belongs to an IMAP personality, we'll want to refresh this personality
			if (pers) 
			{
				(*pers)->imapRefresh = 1;
				refresh = true;
			}
		}		
		
		// now go refresh
		if (refresh) IMAPRefreshPersCaches();
	}
	else if (optionPressed)
	{
		// Shift was pressed.  Refresh ALL the IMAP personalities
		IMAPRefreshAllCaches();
	}				
}

/************************************************************************
 * MBResyncOrExpungeIMAPMailboxes - process the selected IMAP mailboxes.
 ************************************************************************/
OSErr MBResyncOrExpungeIMAPMailboxes(Boolean optionPressed, TaskKindEnum task)
{
	OSErr err;
	Handle mailboxes;

	// Build a handle of all of the selected mailboxes
	err = GetSelectedMailboxSpecs(&mailboxes, optionPressed);
	if ((err == noErr) && mailboxes)
	{
		// process the mailboxs
		err = IMAPProcessMailboxes(mailboxes, task);
	}
	if (err != noErr)
	{
		WarnUser(MEM_ERR, err);
		ZapHandle(mailboxes);
	}
	
	return (err);
}

/************************************************************************
 * MBMarkIMAPMailboxes - mark selected mailboxes
 ************************************************************************/
void MBMarkIMAPMailboxes(Boolean on, MailboxNeedsEnum needs)
{
	short count, i;
	VLNodeInfo data;
	FSSpec spec;
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
	
	if (count = LVCountSelection(&MB.list))
	{	
		for (i=1;i<=count;i++)
		{	
			//	get mailbox spec
			MBGetData(&data,i);
			MakeMBSpec(&data,&spec);
			
			if (data.nodeID != kIMAPFolder)
			{
				// a mailbox is selected.  Who does it belong to?
				spec.parID = SpecDirId(&spec);
				LocateNodeBySpecInAllPersTrees(&spec, &node, &pers);
				
				if (node)
				{
					// is this not the inbox?
					if ((needs != kNeedsPoll) || (node != LocateInboxForPers(pers)))
					{
						// set the property
						SetIMAPMailboxNeeds(node, needs, on);
						
						// are we hiding or showing deleted messages?
						if (needs == kShowDeleted)
							HideDeletedMessages(node, true, on);
							
						// update the cache mailbox now
						WriteIMAPMailboxInfo(&spec, node);
					}
				}
			}
		}		
		
		// update the list
		MBTickle(nil,nil);
	}
}

/************************************************************************
 * MBCompactMailboxes - compact selected mailboxes
 ************************************************************************/
void MBCompactMailboxes(void)
{
	short count, i;
	VLNodeInfo data;
	FSSpec spec;
	PersHandle pers = nil;
	MailboxNodeHandle node = nil;
	
	// properly set autocheck property
	if (count = LVCountSelection(&MB.list))
	{	
		for (i=1;(i<=count) && !CommandPeriod;i++)
		{	
			//	get mailbox spec
			MBGetData(&data,i);
			MakeMBSpec(&data,&spec);
				
			// compact the mailbox	
			if ((data.nodeID != kIMAPFolder) && (data.nodeID != kEudoraFolder))
			{
				// is this an IMAP mailbox?
				if (IsIMAPBox(&data))
					spec.parID = SpecDirId(&spec);
				// is this a POP folder?
				else if (FSpIsItAFolder(&spec))
					continue;
				
				CompactMailbox(&spec,false);
			}
		}		
		
		// update the list
		MBTickle(nil,nil);
	}
}