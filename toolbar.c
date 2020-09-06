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

#define FILE_NUM 70
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#include "toolbar.h"
#include "toolbarpopup.h"
#pragma segment Toolbar

typedef struct ButtonData
{
	TBButtonEnum kind;
	long select;
	long modifiers;
	Handle data;
	ControlHandle cntl;
	Str255 title;
	Style style;
	Boolean reinitBox;
	short slots;
} ButtonData, *BDPtr, **BDHandle;

typedef struct TBGlobals
{
	ToolbarVEnum varCode;
	Boolean vertical;
	short wi;
	short hi;
	short exCount;
	short exPixels;
	short space;
	BDHandle bd;
	//ControlHandle controls[2];
	Rect strucRect;
	uLong downTicks;
	uLong	dragTicks;
	Boolean ignore;
	DragReference oldDrag;
	ControlHandle beingDragged;
	Boolean droppedOnMe;
	Boolean dragInteresting;
	ControlHandle dragHilite;
	ControlHandle lastCntl;
	Boolean messageBeingDragged;
	Boolean deleted;
	Boolean forceEnable;
	short n;
	Boolean fKeys;
	Rect tip;
	ControlHandle tipCntl;
	PortSaveStuff stuff;
	uLong idleTicks;
	uLong lastIdleRunTicks;
	ControlRef taskProgButton;
	ControlHandle searchUserPane;
	ControlHandle searchGo;
	ControlHandle searchEdit;
	ControlHandle searchPopup;
	PETEHandle searchPTE;
	short searchItem;
	StackHandle searchStack;
	Rect screenRect;
} TBG, **TBGHandle;

typedef struct TBMenu
{
	Boolean doingButton;
	Boolean	doingMenuSelect;
	short	menuId;
	short	item;
} TBMenu;

#ifdef DEBUG
TBGHandle TBGDB;
#endif

#define ForceEnable (*(TBGHandle)GetMyWindowPrivateData(Win))->forceEnable
#define VarCode (*(TBGHandle)GetMyWindowPrivateData(Win))->varCode
#define Deleted (*(TBGHandle)GetMyWindowPrivateData(Win))->deleted
#define Vertical (*(TBGHandle)GetMyWindowPrivateData(Win))->vertical
#define Controls (*(TBGHandle)GetMyWindowPrivateData(Win))->controls
#define Hi (*(TBGHandle)GetMyWindowPrivateData(Win))->hi
#define Space (*(TBGHandle)GetMyWindowPrivateData(Win))->space
#define Wi (*(TBGHandle)GetMyWindowPrivateData(Win))->wi
#define ExCount (*(TBGHandle)GetMyWindowPrivateData(Win))->exCount
#define ExPixels (*(TBGHandle)GetMyWindowPrivateData(Win))->exPixels
#define BD (*(TBGHandle)GetMyWindowPrivateData(Win))->bd
#define Dirty Win->isDirty
#define DroppedOnMe (*(TBGHandle)GetMyWindowPrivateData(Win))->droppedOnMe
#define StrucRect (*(TBGHandle)GetMyWindowPrivateData(Win))->strucRect
#define N (*(TBGHandle)GetMyWindowPrivateData(Win))->n
#define DownTicks (*(TBGHandle)GetMyWindowPrivateData(Win))->downTicks
#define DragTicks (*(TBGHandle)GetMyWindowPrivateData(Win))->dragTicks
#define Ignore (*(TBGHandle)GetMyWindowPrivateData(Win))->ignore
#define OldDrag (*(TBGHandle)GetMyWindowPrivateData(Win))->oldDrag
#define DragInteresting (*(TBGHandle)GetMyWindowPrivateData(Win))->dragInteresting
#define BeingDragged (*(TBGHandle)GetMyWindowPrivateData(Win))->beingDragged
#define DragHilite (*(TBGHandle)GetMyWindowPrivateData(Win))->dragHilite
#define LastCntl (*(TBGHandle)GetMyWindowPrivateData(Win))->lastCntl
#define TipCntl (*(TBGHandle)GetMyWindowPrivateData(Win))->tipCntl
#define FKeys (*(TBGHandle)GetMyWindowPrivateData(Win))->fKeys
#define MessageBeingDragged (*(TBGHandle)GetMyWindowPrivateData(Win))->messageBeingDragged
#define Tip (*(TBGHandle)GetMyWindowPrivateData(Win))->tip
#define ForceEnable (*(TBGHandle)GetMyWindowPrivateData(Win))->forceEnable
#define Stuff (*(TBGHandle)GetMyWindowPrivateData(Win))->stuff
#define IdleTicks (*(TBGHandle)GetMyWindowPrivateData(Win))->idleTicks
#define LastIdleRunTicks (*(TBGHandle)GetMyWindowPrivateData(Win))->lastIdleRunTicks
#define TaskProgButton (*(TBGHandle)GetMyWindowPrivateData(Win))->taskProgButton
#define SearchUserPane (*(TBGHandle)GetMyWindowPrivateData(Win))->searchUserPane
#define SearchGo (*(TBGHandle)GetMyWindowPrivateData(Win))->searchGo
#define SearchEdit (*(TBGHandle)GetMyWindowPrivateData(Win))->searchEdit
#define SearchPopup (*(TBGHandle)GetMyWindowPrivateData(Win))->searchPopup
#define SearchPTE (*(TBGHandle)GetMyWindowPrivateData(Win))->searchPTE
#define SearchItem (*(TBGHandle)GetMyWindowPrivateData(Win))->searchItem
#define SearchStack (*(TBGHandle)GetMyWindowPrivateData(Win))->searchStack
#define ScreenRect (*(TBGHandle)GetMyWindowPrivateData(Win))->screenRect

void TBMonitorMonitor(void);
Boolean TBButtonInterested(ControlHandle theCtl);
long TBExtractMenu(BDPtr bd,short *mnu,short *item);
OSErr TBSetButtonType(ControlHandle button);

#define IsRecipMenu(m)	(NEW_TO_HIER_MENU<=(m) && (m)<=INSERT_TO_HIER_MENU)
#define IsStationMenu(m)	((m)==REPLY_WITH_HIER_MENU || (m)==NEW_WITH_HIER_MENU)

#define	ToolbarStrnID	(HasFeature (featureCustomizeToolbar) ? ToolbarProStrn : ToolbarLightStrn)

#define TBAR_DRAG_FLAVOR	'TBAR'
#define TB_ALIAS_TYPE	'EuTA'

typedef struct 
{
	short id;
	OSType type;
	OSType creator;
} TBAliasData, *TBAliasPtr, **TBAliasHandle;

MyWindowPtr Win;
TBMenu	gTBMenu;

#define HideControl(c)	SetControlVisibility((c),false,false)
#define ShowControl(c)	SetControlVisibility((c),true,false)

/**********************************************************************
 * Prototypes
 **********************************************************************/
void TBPlaceControls(Rect *screenRect,short *windowWide, short *windowHi);
OSErr TBNewButton(short which);
void ToolbarButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
OSErr LoadToolbarButton(short i);
void SizeToolbar(short max);
short Menu2Icon(short menu,short item,short modifiers);
short Key2Icon(short key,short modifiers);
Boolean ConfigureTBButton(short which);
void ToolClick(MyWindowPtr win, EventRecord *event);
void InstallButtonMenu(short i, short menu, short item,PStr itemText,short modifiers,void *data,long size);
void InstallButtonFile(short i, FSSpecPtr spec);
void InstallButtonKey(short i, long key,short modifiers);
Boolean ToolbarClose(MyWindowPtr win);
void SaveToolbar(void);
void FlipToolbar(void);
void ToolbarUpdate(MyWindowPtr win);
PStr ModifierNames(PStr string,short modifiers);
Boolean TBInterStice(Point pt,short *which);
void TBBalloon(ControlHandle cntl);
Boolean TBInstSpecialMenus(short menu,short item,short modifiers,PStr itemStr,PStr titleStr);
Boolean TBSpecialStr(short menu,PStr itemStr,short *item,short *modifiers);
Boolean TBMovedMenu(short *menu,PStr itemStr,short *item);
Boolean ToolControl(Point pt, MyWindowPtr win);
void TBButton2String(ButtonData *bd, PStr s);
void TBDelButton(ControlHandle cntl);
Boolean ToolDrag(ControlHandle theCtl, EventRecord *event);
OSErr TBDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
void TBDragHilite(DragReference drag,ControlHandle cntl);
OSErr TBDrop(DragReference drag, short which, Boolean replacing);
void InstallButtonSpec(short which, FSSpecPtr spec);
void TBZapButtonData(short which);
void InstallButtonAliasId(short which,short id,OSType type, OSType creator,PStr name);
void TBFileButton(BDPtr bdp);
OSErr LoadButtonString(short i,PStr s);
OSErr ToolbarMailbox(short which,FSSpecPtr spec);
OSErr TBMessageDrop(DragReference drag,short which);
Boolean TBIsMboxButton(ControlHandle theCtl);
Boolean TBIsNicknameButton(ControlHandle theCtl);
void TBTipLo(ControlHandle cntl);
void TBDrawTip(void);
Boolean ToolbarMenu(MyWindowPtr win, int menu, int item, short modifiers);
OSErr SetPrinterIcon(ControlHandle button);
OSErr SetNicknameIcon(ControlHandle button, PStr nickname);
#define TBTip(x)	do{if (Win && TipCntl!=(x)) TBTipLo(x);}while(0)
void TBButtonClose(short which);
PStr TBGetCTitle(ControlHandle cntl,PStr title);
void TBSetCTitle(ControlHandle theCtl,PStr title);
void TBIdle(MyWindowPtr win);
void InstallButtonAd(short i,TBAdInfo *pAd);
void InstallButtonNickname (short i, PStr nickname);
Boolean TBRename (TBButtonEnum kind, PStr oldName, PStr newName);
short TBBoxishMenu(short which,FSSpecPtr spec,PStr s,short mnu,short itm);
OSErr TBReinitBox(short which);
void DeletingButton(BDPtr pBD);
void MessageDragPopup (ControlHandle button, Boolean vertical);
static void TaskProgAnimate(ControlRef control);
#define TB_RESTORE_STUFF	do{LDRef((TBGHandle)GetMyWindowPrivateData(Win));SetPortStuff(&Stuff);UL((TBGHandle)GetMyWindowPrivateData(Win));}while(0);
#define TB_SAVE_STUFF	do{LDRef((TBGHandle)GetMyWindowPrivateData(Win));DisposePortStuff(&Stuff);SavePortStuff(&Stuff);UL((TBGHandle)GetMyWindowPrivateData(Win));}while(0);
ControlHandle TBNewSearchEdit(WindowPtr winWP);
void TBSearchStart(PStr searchMe);
void TBSearchEmpty(void);
void TBSearchPopup(Point pt);
void TBSearchFocus(Boolean focus);
void TBDragPreProcess(PETEHandle pte,DragTrackingMessage message,DragReference drag);
void TBDragPostProcess(PETEHandle pte,DragTrackingMessage message,DragReference drag);

#pragma segment Toolbar
/**********************************************************************
 * OpenToolbar - open the toolbar
 **********************************************************************/
void OpenToolbar(void)
{
	MyWindowPtr win;
	WindowPtr	winWP;
	short i,n;
	
	if (Win) {CloseMyWindow(GetMyWindowWindowPtr(Win)); Win=nil; return;}

	win = GetNewMyWindowWithClass(TBAR_WIND,nil,nil,BehindModal,False,False,TBAR_WIN,PrefIsSet(PREF_TB_FLOATING) ? kFloatingWindowClass: kToolbarWindowClass);
	winWP = GetMyWindowWindowPtr (win);
	if (win)
	{
		Win = win;
		SetMyWindowPrivateData(Win,(long)NewZH(TBG));
#ifdef DEBUG
		TBGDB = (TBGHandle)GetMyWindowPrivateData(Win);
#endif
		i = GetPrefLong(PREF_TB_VARIATION)+1;
		VarCode = i;
		if (PrefIsSet(PREF_TB_VERT)) Vertical = True; else Vertical = False;
		SetPort_(GetWindowPort(winWP));
		TextFont(SmallSysFontID());
		TextSize(9);
		MySetThemeWindowBackground(win,kThemeActiveUtilityWindowBackgroundBrush,False);
		SetThemeTextColor(kThemeActiveWindowHeaderTextColor,RectDepth(&Win->contR),True);
		TB_SAVE_STUFF;
		n = CountStrn(ToolbarStrnID);
		SizeToolbar(n);
		for (i=1;i<=n;i++)
		{
			if (LoadToolbarButton(i)) break;
		}
		win->button = ToolbarButton;
		win->click = ToolClick;
		win->isRunt = True;
		win->bgClick = ToolClick;
		win->close = ToolbarClose;
		win->update = ToolbarUpdate;
		win->cursor = ToolbarCursor;
		win->menu = ToolbarMenu;
		win->dontControl = True;
		win->dontActivate = True;
		win->idle = TBIdle;
		win->idleInterval = 60;
		win->drag = HasFeature (featureCustomizeToolbar) ? TBDragHandler : nil;
		win->position = PositionPrefsTitle;
		win->dontDrawControls = True;
		win->windowType = PrefIsSet(PREF_TB_FLOATING) ? kFloating : kDockable;
		ShowMyWindow(winWP);
		PositionDockedWindow(winWP);
		
		// Because of the mini-dialog, we no longer do this
		// Add web search
		// if (!GetPrefBit(PREF_SEARCH_WEB_BITS,prefSearchWebTBButtonInstalled))
		//	TBAddMenuButton(FIND_HIER_MENU,FIND_SEARCH_WEB_ITEM,ComposeRString(GlobalTemp,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB));

		//	Add any in-your-face plug-ins to the toolbar
		ETLAddToToolbar();
		
		GetStructureRgnBounds(winWP,&StrucRect);
		ToolbarNudgeAll(true);
		PositionDockedWindows();
	}
}

/**********************************************************************
 * TBIdle - idle loop for the toolbar
 * Will ordinarily defer to other events, but if called with nil does not defer
 **********************************************************************/
void TBIdle(MyWindowPtr win)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr(Win);	// Yeah, seems like these are the same... or are they?
	WindowPtr	winWP = GetMyWindowWindowPtr(win);	// Safer for now to just treat them as different
	ButtonData bd;
	short mnu, item;
	short i;
	short lite;
	Boolean fiddle = False;
	Style style;
	MenuHandle mh;
	static uLong styleTicks;
	Boolean reStyle;
	ControlHandle cntl;

	if (win && (FrontWindowNeedsUpdate(WinWP) || !IsWindowVisible(winWP)))
	{
		return;	// if there's anything else to do or toolbar is invisible, go away
	}
		
	if (LastIdleRunTicks >= NonNullTicks) return;	// nothing has happened lately; go away
	
	LastIdleRunTicks = NonNullTicks;
	
	if (!InBG)
	{
		SetPort_(GetMyWindowCGrafPtr(Win));
		TB_RESTORE_STUFF;
		TBMonitorMonitor();
		if (reStyle = (TickCount()-styleTicks>90)) styleTicks = TickCount();
		GetRootControl(WinWP,&cntl);
		ActivateControl(cntl);
		EnableMenuItems(False);
		
		// clear labels
		if (keyFocusedFloater==Win && Win->pte) PeteNoLabel(Win->pte,0,0xffffffff,0xff);
		
		// Update buttons
		for (i=0;i<N;i++)
		{
			if ((*BD)[i].reinitBox) TBReinitBox(i);
			
			bd = (*BD)[i];
			
			if (bd.kind==tbkMenu)
			{
				if (TBExtractMenu(&bd,&mnu,&item))
				{
					if (reStyle && (mh=GetMHandle(mnu)))
					{
						GetItemStyle(mh,item,&style);
						if (bd.style != style)
						{
							(*BD)[i].style = style;
							SetBevelStyle(bd.cntl,style);
						}
						if (mnu==LABEL_HIER_MENU)
						{
							Str255 s;
							MyGetItem(mh,item,s);
							TBSetCTitle(bd.cntl,s);
						}
#ifdef NEVER // unfortunately, doesn't work
						GetItemColor(mnu,item,&color);
						SetBevelColor(bd.cntl,&color);
#endif
					}
					if (mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) item=1;	//avoid looking at the separator bar, dummy.
					mnu = MailboxKindaMenu(mnu,item,nil,nil);
					if (mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) item=1;	//avoid looking at the separator bar, dummy.
					if (mnu==FILE_MENU && item==FILE_QUIT_ITEM && HaveTheDiseaseCalledOSX())
					{
						// enable Quit anytime In is enabled
						mnu=MAILBOX_MENU;
						item=1;
					}
					lite = !ModalWindow&&(ForceEnable||IsEnabled(mnu,item))?0:255;
				}
			}
			else
				lite = ModalWindow||NoMenus ? 255 : 0;
			if (GetControlHilite(bd.cntl)!=lite)
			{
				HiliteControl(bd.cntl,lite);
				if (Space<0 && lite==255)
				{
					fiddle = True;
					if (i>0 && GetControlHilite((*BD)[i-1].cntl)!=255)
						Draw1Control((*BD)[i-1].cntl);
				}
				else fiddle = False;
			}
			else
			{
				if (fiddle && GetControlHilite(bd.cntl)!=255)
					Draw1Control(bd.cntl);
				fiddle = False;
			}
		}
	}
}

/**********************************************************************
 * TBMonitorMonitor - watch the monitor and see if it changes
 **********************************************************************/
void TBMonitorMonitor(void)
{
	Rect screenRect;
	GDHandle junk;
	short i = GetPrefLong(PREF_NW_DEV);
	Boolean hasMB;
	
	if (i)
	{
		utl_GetIndGD(i,&junk,&screenRect,&hasMB);
		if (hasMB) screenRect.top += GetMBarHeight();
	}
	else
	{
		GetQDGlobalsScreenBitsBounds(&screenRect);
		screenRect.top += GetMBarHeight();
	}
	
	if (!AboutSameRect(&screenRect,&ScreenRect)) SizeToolbar(N);
}

/**********************************************************************
 * TBReinitBox - read the mailbox back out of a toolbar button
 **********************************************************************/
OSErr TBReinitBox(short which)
{
	// Yowza.  We be changin' it
	short mnu = (((*BD)[which].select)>>16)&0xffff;
	short item;
	Str63 s;
	FSSpec spec;
	Boolean xfer = mnu==TRANSFER_MENU;
	
	if (!ToolbarMailbox(which+1,&spec))
	if (!Spec2Menu(&spec,xfer,&mnu,&item))
	{
		*s = 0;
		if (xfer) GetRString(s,TRANSFER_PREFIX);
		PSCat(s,spec.name);
		
		mnu = TBBoxishMenu(which,&spec,s,mnu,item);
		InstallButtonMenu(GetControlReference((*BD)[which].cntl),mnu,0,s,0,&spec,sizeof(FSSpec));
		Dirty = true;
	}
	(*BD)[which].reinitBox = false;
	return noErr;
}

/**********************************************************************
 * TBForceIdle - force the toolbar through an idle loop
 **********************************************************************/
void TBForceIdle(void)
{
	if (Win) LastIdleRunTicks = 0;
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean TBButtonInterested(ControlHandle theCtl)
{
	Str255	sTitle;
	
	GetControlTitle(theCtl,sTitle);
	return (!MessageBeingDragged && !*sTitle ||
					 MessageBeingDragged && (TBIsMboxButton(theCtl)||TBIsNicknameButton(theCtl)));	//	Don't drag onto ads
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr TBDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{
#pragma unused(win)
	WindowPtr	WinWP = GetMyWindowWindowPtr(Win);
	OSErr err=noErr;
	Point mouse;
	short stice;
	Rect r;
	RgnHandle rgn;
	ControlHandle theCtl=nil;
	
	if (drag!=OldDrag)
	{
		Boolean interesting = DragIsInteresting(drag,TBAR_DRAG_FLAVOR,flavorTypeHFS,flavorTypeDirectory,TOC_FLAVOR,MESS_FLAVOR,kMBDragType,kNickDragType,nil);
		OldDrag = drag;
		DragInteresting = interesting;
		
		if (interesting && DragIsInteresting(drag,TOC_FLAVOR,MESS_FLAVOR,nil))
			MessageBeingDragged = True;
		else MessageBeingDragged = False;
	}
	
	if (!DragInteresting) return(dragNotAcceptedErr);
	
	SetPort_(GetMyWindowCGrafPtr(Win));
	TB_RESTORE_STUFF;
	GetDragMouse(drag,&mouse,nil);
	GlobalToLocal(&mouse);
	
	switch (which)
	{
		case kDragTrackingInWindow:
			if (!MessageBeingDragged && TBInterStice(mouse,&stice) && (rgn=NewRgn()))
			{
				if (DragHilite) TBDragHilite(drag,nil);
				SetMyCursor(Vertical ? SPREAD_CURS_V : SPREAD_CURS);
				if (Vertical)
					InsetRect(&r,-10,3);
				else
					InsetRect(&r,3,-10);
				RectRgn(rgn,&r);
				ShowDragHilite(drag,rgn,True);
				DisposeRgn(rgn);
			}
			else if (FindControl(mouse,WinWP,&theCtl) && theCtl!=TaskProgButton)
			{
				SetMyCursor(arrowCursor);
				if (!TBButtonInterested(theCtl))
					TBDragHilite(drag,nil);
				else
					TBDragHilite(drag,theCtl);
			}
			else TBDragHilite(drag,nil);
			err = noErr;
			break;
		case kDragTrackingLeaveWindow:
			SetMyCursor(arrowCursor);
			TBDragHilite(drag,nil);
			err = noErr;
			break;
		case 0xfff:
			DroppedOnMe = True;
			if (!DragInteresting) return(dragNotAcceptedErr);
			if (TBInterStice(mouse,&stice))
			{
				err = TBDrop(drag,stice,False);
			}
			else if (FindControl(mouse,WinWP,&theCtl))
			{
				if (!TBButtonInterested(theCtl)) return(dragNotAcceptedErr);
				err = TBDrop(drag,GetControlReference(theCtl),True);
			}
			else err = dragNotAcceptedErr;
			break;
	}
	
	TBTip(theCtl);
	
	return(err);
}

/**********************************************************************
 * TBDrop - handle something dropped on the toolbar
 **********************************************************************/
OSErr TBDrop(DragReference drag, short which, Boolean replacing)
{
	short i, n;
	OSErr err = noErr;
	Boolean dirty = False;
	Handle data = nil;
	FSSpec spec;
	Str255 string;

	n = MyCountDragItems(drag);
	
	TBDragHilite(drag,nil);
	
	if (MessageBeingDragged)
	{
		TBMessageDrop(drag,which);
	}
	else
	{
		for (i=1;i<=n && !err; i++, which++)
		{
			if (!(err=MyGetDragItemData(drag,i,TBAR_DRAG_FLAVOR,&data)))
			{
				if (BeingDragged)
				{
					if (!replacing) {TBNewButton(which); which++;}
					
					if (replacing && GetControlReference(BeingDragged)==which)
					{
						err = dragNotAcceptedErr;
					}
					else
					{
						DeletingButton(&(*BD)[which-1]);
						PCopy(string,*data);
						LoadButtonString(which,string);
					}
				}
				else
				{
					if (!replacing) {TBNewButton(which); which++;}
					else DeletingButton(&(*BD)[which-1]);
					PCopy(string,*data);
					LoadButtonString(which,string);
				}
				if (!err) Dirty = True;
			}
			else if (!(err=MyGetDragItemData(drag,i,flavorTypeHFS,&data)))
			{
				//	File drag from Finder
				if (!replacing) {TBNewButton(which);}
				else { replacing = False; DeletingButton(&(*BD)[which-1]); which--;}
				
				spec = (*(struct HFSFlavor**)data)->fileSpec;
				
				InstallButtonSpec(which+1,&spec);
				Dirty = True;
			}
			else if (!(err=MyGetDragItemData(drag,i,kMBDragType,&data)))
			{
				//	Mailbox drag from Mailboxes window
				MBDragPtr	pDragData;
				
				if (!replacing) {TBNewButton(which);}
				else { replacing = False; DeletingButton(&(*BD)[which-1]); which--;}
				
				pDragData = (MBDragPtr)*data;
				InstallButtonMenu(which+1, MailboxKindaMenu(pDragData->menuID,pDragData->menuItem,pDragData->name,&pDragData->spec),0,pDragData->name,nil,&pDragData->spec,sizeof(FSSpec));
				Dirty = True;
			}
			else if (!(err=MyGetDragItemData(drag,i,kNickDragType,&data)))
			{
				Str31		nickname;
				Handle	notes;
				long		offset;
				short		ab,
								nick;
				
				notes = nil;
				offset = 0;
				if (!GetNickFlavorDragData (data, &offset, &ab, &nick, nickname, nil, &notes)) {
					if (!replacing) {TBNewButton(which);}
					else { replacing = False; DeletingButton(&(*BD)[which-1]); which--;}

					// Make the toolbar button link to this recipient in the "New Message To" menu
					InstallButtonNickname (which + 1, nickname);
					Dirty = True;
				}
				ZapHandle (notes);
			}
			ZapHandle(data);
		}
	}
	return(err);
}


/**********************************************************************
 * TBMessageDrop - Drop a message (or group of messages from a TOC) onto
 *								 a toolbar button.
 *
 *								 If the drop is on a nickname button, the message(s)
 *								 will be sent to the target nickname.  Without any
 *								 modifiers accompanying the drop, the action will be
 *								 "forward to".  By holding down some as of yet defined
 *								 modifier the action is changed to "redirect to".
 **********************************************************************/
OSErr TBMessageDrop(DragReference drag,short which)
{
	FSSpec spec;
	OSErr err;
	FSSpec ***data;
	
	err = noErr;
	// Are we dropping the message onto a nickname?
	if ((*BD)[which-1].kind == tbkNick) {
		UHandle data=nil;
		TOCHandle	tocH = nil;
		
		TextAddrHandle	toWhom;

		if (toWhom = NuHandle ((*BD)[which-1].title[0]))
			BlockMoveData (&(*BD)[which-1].title[1], *toWhom, (*BD)[which-1].title[0]);
			
		if (!(err=MyGetDragItemData(drag,1,MESS_FLAVOR,(void*)&data)))
		{
			tocH = (***(MessHandle**)data)->tocH;
		}
		else if (!(err=MyGetDragItemData(drag,1,TOC_FLAVOR,(void*)&data)))
			tocH = **(TOCHandle**) data;
		// Make sure the toolbar popup is closed if we're tracking
		CloseToolbarPopup ();
		if (tocH)
			DoIterativeThingy (tocH, PrefIsSet (PREF_REDIRECT_MSG_DROPS_ON_TB_NICKS) ? MESSAGE_REDISTRIBUTE_ITEM :  MESSAGE_FORWARD_ITEM, CurrentModifiers (), toWhom);
		ZapHandle(data);
		ZapHandle(toWhom);
	}
	else {
		// tell the dragger where the destination is and let it do the work
		if (!(err=ToolbarMailbox(which,&spec)))
		if (!(err=MyGetDragItemData(drag,1,'euXX',(void*)&data)))
		{
			***data = spec;
		}
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean TBIsMboxButton(ControlHandle theCtl)
{
	ButtonData bd = (*BD)[GetControlReference(theCtl)-1];
	long mnu;
	
	if (bd.kind!=tbkMenu) return(False);
	mnu = (bd.select>>16)&0xffff;
	return ((mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) && bd.data);
}

/**********************************************************************
 * TellToolMBRename - make sure we know that a mailbox is being renamed
 **********************************************************************/
OSErr TellToolMBRename(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn)
{
	short i;
	Boolean isIMAPMailbox = IsIMAPCacheFolder(newSpec);

	if (ToolbarShowing() && !will)
	{
		for (i=0;i<N;i++)
		{
			if (TBIsMboxButton((*BD)[i].cntl))
			{
				FSSpec buttonSpec;
				
				if (!ToolbarMailbox(i+1,&buttonSpec))
				{
					if (isIMAPMailbox)
					{
						FSSpec scratch;
						
						// IMAP mailbox cache _files_ are stored in the toolbar buttonSpec.
						Zero(scratch);
						ParentSpec(&buttonSpec, &scratch);
						buttonSpec.parID = scratch.parID;
					}
					
					if (SameSpec(&buttonSpec,spec))
					{
						(*BD)[i].reinitBox = true;
						**(FSSpecPtr*)(*BD)[i].data = *newSpec;
					}
				}
			}
		}
	}
	return noErr;
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean TBIsNicknameButton(ControlHandle theCtl)
{
	return ((*BD)[GetControlReference(theCtl)-1].kind==tbkNick);
}


/**********************************************************************
 * 
 **********************************************************************/
void TBSetCTitle(ControlHandle theCtl,PStr title)
{
	PCopy((*BD)[GetControlReference(theCtl)-1].title,title);
	if (VarHasName(VarCode)) MySetControlTitle(theCtl,title);
}

/**********************************************************************
 * 
 **********************************************************************/
void InstallButtonSpec(short which, FSSpecPtr spec)
{
	ButtonData bd;
	AliasHandle alias;
	FSSpec settings;
	short id;
	FInfo info;
	
	TBZapButtonData(which);
	bd = (*BD)[which-1];
	
	bd.kind = tbkFile;
	TBSetCTitle(bd.cntl,spec->name);
	PCopy(bd.title,spec->name);
	
	GetFileByRef(SettingsRefN,&settings);
	
	if (NewAlias(&settings,spec,&alias)) return;
	
	UseResFile(SettingsRefN);
	id = MyUniqueID(TB_ALIAS_TYPE);
	
	AddMyResource_(alias,TB_ALIAS_TYPE,id,"");
	if (ResError())
	{
		ZapHandle(alias);
		return;
	}
	
	MyUpdateResFile(SettingsRefN);
	
	if (FSpIsItAFolder(spec)) info.fdType = 'fdrp';
	else AFSpGetFInfo(spec,spec,&info);

	if (bd.data = NuHandle(sizeof(TBAliasData)))
	{
		(*(TBAliasHandle)bd.data)->id = id;
		(*(TBAliasHandle)bd.data)->type = info.fdType;
		(*(TBAliasHandle)bd.data)->creator = info.fdCreator;
	}
	
	if (VarHasIcon(VarCode))
		if (info.fdType=='fdrp')
			SetBevelIcon(bd.cntl,kGenericFolderIconResource,nil,nil,nil);
		else
			SetBevelIcon(bd.cntl,0,info.fdType,info.fdCreator,nil);
	
	(*BD)[which-1] = bd;
	if (TaskProgButton==bd.cntl) TaskProgButton = nil;
}

/**********************************************************************
 * 
 **********************************************************************/
void TBZapButtonData(short which)
{
	RGBColor color;
	Zero(color);
	//if ((*BD)[which-1].kind == tbkFile && (*BD)[which-1].data)
		//ZapSettingsResource(TB_ALIAS_TYPE,(*(TBAliasHandle)((*BD)[which-1].data))->id);
	ZapHandle((*BD)[which-1].data);
	if ((*BD)[which-1].cntl) SetBevelColor((*BD)[which-1].cntl,&color);
}

/**********************************************************************
 * 
 **********************************************************************/
void TBButtonClose(short which)
{
	if ((*BD)[which-1].cntl) KillBevelIcon((*BD)[which-1].cntl);
	ZapHandle((*BD)[which-1].data);
}

/**********************************************************************
 * 
 **********************************************************************/
void TBDragHilite(DragReference drag,ControlHandle cntl)
{
	ControlHandle	oldDragHilite = DragHilite;
	Point					mouse;
	
	HideDragHilite(drag);
	
	if (DragHilite && DragHilite!=BeingDragged && DragHilite!=cntl) {
		SetControlValue(DragHilite,0);
		// Don't close a toolbar popup if the mouse isin the window
		GetDragMouse (drag, &mouse, nil);
		if (!MouseInToolbarPopup (mouse, cntl ? false : true))
			CloseToolbarPopup ();
	}
	if (cntl) SetControlValue(cntl,1);
	DragHilite = cntl;

	if (MessageBeingDragged)
		if (cntl && (*BD)[GetControlReference(cntl) - 1].kind == tbkNick)
			if (oldDragHilite != DragHilite && (!oldDragHilite || (oldDragHilite && (*BD)[GetControlReference(oldDragHilite) - 1].kind != tbkNick)))
				DragTicks = TickCount ();
			else
				if (TickCount () > DragTicks + GetRLong (NICK_BUTTON_DRAG_TICKS))
					MessageDragPopup (cntl, !Vertical);
}


/**********************************************************************
 * ToolbarNudgeAll - nudge all windows
 **********************************************************************/
void ToolbarNudgeAll(Boolean gently)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (win!=Win) ToolbarNudge(win,gently);
	}
}

/**********************************************************************
 * ToolbarUpdate - update the toolbar; really just sets the bg color
 **********************************************************************/
void ToolbarUpdate(MyWindowPtr win)
{
	WindowPtr	WinWP;
	short i;
	Str15 numStr;
	ControlHandle cntl;
	ControlHandle root;
	Rect r, rr;
	SAVE_STUFF;

	TextFont(SmallSysFontID());
	TextSize(9);
	
	if (FKeys && PrefIsSet(PREF_SHOW_FKEYS))
	{
		short h,v;
		short bh, bv;
		
		bh = bv = 0;
		
		TextSize(9);
		TextFace(condense|italic);
		if (ThereIsColor) SetForeGrey(k8Grey7);
		for (i=0;i<N && i<15;i++)
		{
			ComposeRString(numStr,FKEY_FMT,i+1);
			GetControlBounds((*BD)[i].cntl,&r);
			rr = r;
			if (Vertical)
			{
				h = r.right+INSET;
				v = r.bottom-INSET;
			}
			else
			{
				h = r.left+INSET;
				v = r.bottom+10;
			}
			MoveTo(h,v);
			DrawString(numStr);
		}
	}
	REST_STUFF;
	PenNormal();
	
	WinWP = GetMyWindowWindowPtr(Win);
	GetRootControl(GetMyWindowWindowPtr(win),&root);
	// Draw dimmed cntls first
	for (cntl=GetControlList(WinWP);cntl;cntl=GetNextControl(cntl))
		if (cntl!=root && !IsControlActive(cntl)) Draw1Control(cntl);
	// Undimmed ones next
	for (cntl=GetControlList(WinWP);cntl;cntl=GetNextControl(cntl))
		if (cntl!=root && IsControlActive(cntl)) Draw1Control(cntl);
	
	TBDrawTip();
}

/**********************************************************************
 * 
 **********************************************************************/
void TBDrawTip(void)
{
	Rect r=Tip;
	if (TipCntl)
	{
		PortSaveStuff old;
		Str255 title;
		//Style style;

		PushGWorld();
		SetPort_(GetMyWindowCGrafPtr(Win));
		SavePortStuff(&old);
		TextSize(9);
		TextFace(0);
		EraseRect(&r);
		TBGetCTitle(TipCntl,title);
		MoveTo(Tip.left,Tip.bottom-2);
		//if (!GetBevelStyle(TipCntl,&style)) TextFace(style); //seems to be bug in GetControlData for bevels; this isn't working correctly
		DrawString(title);
		SetPortStuff(&old);
		DisposePortStuff(&old);
		PopGWorld();
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void TBTipLo(ControlHandle cntl)
{
	Rect r = Tip,rCntl;
	Str255 title;
	
	// anything to do?
	if (TipCntl==cntl) return;
	if (r.bottom==0) return;
	
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(Win));
	TB_RESTORE_STUFF;
	
	// clear the old one
	if (r.left!=r.right)
	{
		EraseRect(&r);
		InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
		r.left = r.right = 0;
	}
	
	// show the new one
	TipCntl = nil;
	if (cntl)
	{
		long controlRef = GetControlReference(cntl)-1;
		if (0<=controlRef && controlRef<N)
		{
			TBGetCTitle(cntl,title);
			if (*title)
			{
				GetControlBounds(cntl,&rCntl);
				r.left = rCntl.left + INSET;
				r.right = r.left + StringWidth(title) + 2;
				r.top = rCntl.bottom;
				r.bottom = r.top+12;
				if (r.right>Win->contR.right) OffsetRect(&r,Win->contR.right-2-r.right,0);
				r.right += 6;
				Tip = r;
				InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
			}
			TipCntl = cntl;
		}
	}
	
	UpdateMyWindow(GetMyWindowWindowPtr(Win));
	PopGWorld();
}

/**********************************************************************
 * 
 **********************************************************************/
PStr TBGetCTitle(ControlHandle cntl,PStr title)
{
	Str31 menuStr;
	MenuHandle mh;
	ButtonData bd = (*BD)[GetControlReference(cntl)-1];
	short menu;
	
	PCopy(title,bd.title);
	if (bd.kind==tbkMenu)
	{
		menu = bd.select>>16;
		if ((IsRecipMenu(menu) || IsStationMenu(menu)) && (mh=GetMHandle(bd.select>>16)))
		{
			GetMenuTitle(mh,menuStr);
			PCatC(menuStr,' ');
			PInsert(title,256,menuStr,title+1);
		}
	}
	return(title);
}

/**********************************************************************
 * TBInterStice - is a point (local coords) in a toolbar interstice?
 **********************************************************************/
Boolean TBInterStice(Point pt,short *which)
{
	short i;
	Boolean found = false;
	short slop = MAX(4,ExPixels/2);
	short hSlop = Vertical ? 0 : slop;
	short vSlop = Vertical ? slop : 0;
	Rect rOuter,rInner;
	
	for (i=0;i<N;i++)
	{
		GetControlBounds((*BD)[i].cntl,&rInner);
		rOuter = rInner;
		InsetRect(&rOuter,-hSlop,-vSlop);
		InsetRect(&rInner,hSlop,vSlop);
		if (PtInRect(pt,&rOuter) && !PtInRect(pt,&rInner))
		{
			if (Vertical && pt.v > (rOuter.bottom+rOuter.top)/2) i++;
			if (!Vertical && pt.h > (rOuter.right+rOuter.left)/2) i++;
			*which = i;
			return true;
		}
	}

	return(false);
}

/**********************************************************************
 * ToolbarClose - deallocate memory
 **********************************************************************/
Boolean ToolbarClose(MyWindowPtr win)
{
#pragma unused(win)
	short i, n;
	
	if (Dirty) SaveToolbar();

	if (BD)
	{
		n = HandleCount(BD);
		for (i=1;i<=n;i++) TBButtonClose(i);
	}
	DisposeHandle((Handle)GetMyWindowPrivateData(Win));
	SetMyWindowPrivateData(Win,0);
	Win = nil;
	return(True);
}

/**********************************************************************
 * 
 **********************************************************************/
void SaveToolbar(void)
{
	Handle r = nil;
	ButtonData bd;
	Str255 s;
	short i;
	short n = HandleCount(BD);
	
	UseResFile(SettingsRefN);

	ZapSettingsResource('STR#',ToolbarStrnID);
	
	if (r = NuHTempOK(2))
	{
		**(short**)r = 0;
		AddMyResource(r,'STR#',ToolbarStrnID,"");
	}
	else return;
	
	for (i=0;i<n;i++)
	{
		bd = (*BD)[i];
		TBButton2String(&bd,s);
		ChangeStrn(ToolbarStrnID,i+1,s);
	}
	
	Dirty = False;
}

/**********************************************************************
 * 
 **********************************************************************/
void TBButton2String(ButtonData *bd, PStr s)
{
	MenuHandle mh;
	Str255 menuStr, itemStr;
	TBAliasData tbd;
	Str63 mods;
	short menu, item;
	short cookedMods;
	FSSpec spec, junk;
	Str255 title;
	
	*s = 0;
	switch(bd->kind)
	{
		case tbkMenu:
			PCopy(title,bd->title);

			// Save the entire pathname for mailboxes
			TBExtractMenu(bd,&menu,&item);
			if (IsMailboxChoice(menu,item) && menu!=MAILBOX_MENU && menu!=TRANSFER_MENU)
			{
				GetTransferParams(menu,item,&spec,nil);
				// I don't understand why the following was done:
				// if (IsIMAPMailboxFile(&spec)) spec.parID = SpecDirId(&spec);
				// sd 3/10/03
				
				// grab its full path name, unless we're dealing with a grafted mailbox.
				if (!IsAlias(&spec,&junk)) Box2Path(&spec, title);	
			}

			menu = (bd->select>>16)&0xffff;
			item = (bd->select)&0xffff;
			if (mh=GetMHandle(menu))
			{
				if (menu==LABEL_HIER_MENU)
					NumToString(Menu2Label(item),itemStr);
				else
					PCopy(itemStr,title);
				cookedMods = bd->modifiers;
				if (menu==MESSAGE_MENU && item==MESSAGE_REPLY_ITEM) cookedMods &= ~optionKey;
				ModifierNames(mods,cookedMods);
				if (*itemStr > *mods && !CompareText(mods+1,itemStr+1,*mods,*mods,nil))
				{
					*itemStr -= *mods;
					BMD(itemStr+1+*mods,itemStr+1,*itemStr);
				}
				GetMenuTitle(mh,menuStr);
				ComposeRString(s,TB_FMT,bd->modifiers,TBButtonStrn+bd->kind,menuStr,itemStr);
			}
			break;
			
		case tbkKey:
			NumToString(bd->select,itemStr);
			ComposeRString(s,TB_FMT,bd->modifiers,TBButtonStrn+bd->kind,itemStr,"");
			break;

		case tbkFile:
			tbd = **(TBAliasHandle)bd->data;
			NumToString(tbd.id,menuStr);
			PCopy(itemStr,bd->title);
			PCatC(itemStr,':');
			*itemStr += 8;
			BMD(&tbd.type,itemStr+*itemStr-7,4);
			BMD(&tbd.creator,itemStr+*itemStr-3,4);
			ComposeRString(s,TB_FMT,bd->modifiers,TBButtonStrn+bd->kind,menuStr,itemStr);
			break;
			
		case tbkAd:
			NumToString(bd->select,itemStr);
			ComposeRString(s,TB_FMT,bd->modifiers,TBButtonStrn+bd->kind,itemStr,bd->title);
			break;
			
		case tbkNick:
			ComposeRString(s,TB_FMT,bd->modifiers,TBButtonStrn+bd->kind,bd->title,"");
			break;
	}
}

/**********************************************************************
 * ToolClick - handle a click in the toolbar
 **********************************************************************/
void ToolClick(MyWindowPtr win, EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Point pt = event->where;
	short which;
	Boolean did;
	
	// Don't allow command clicks in light
	if (!HasFeature (featureCustomizeToolbar) && event->modifiers&cmdKey)
		return;
	
	UseFeature (featureCustomizeToolbar);
	
	SetPort_(GetMyWindowCGrafPtr(Win));
	TB_RESTORE_STUFF;
	GlobalToLocal(&pt);

	LastIdleRunTicks = 0;
	TBIdle(nil);
	if (event->modifiers&cmdKey && TBInterStice(pt,&which))
	{
		while (WaitMouseUp()) {WNE(0,nil,0);}
		GetMouse(&pt);
		if (TBInterStice(pt,&which))
		{
			TBNewButton(which);
			Deleted = false;
			did = ConfigureTBButton(which+1);
			if (!Deleted && !did)
				TBDelButton((*BD)[which].cntl);
				
			AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),which), event->what);
		}
	}
	else if ((event->modifiers&(cmdKey+optionKey))==cmdKey+optionKey)
	{
		//	Drag toolbar
		DragMyWindow(winWP,event->where);
	}
	else if (SearchPTE && PtInPETE(pt,SearchPTE))
	{
		// Focus in the search field if we haven't already
		if (!Win->pte)
		{
			TBSearchFocus(true);
		}
		else
			PeteEditWFocus (Win, SearchPTE, peeEvent, (void *) event, nil);

	}
	else
	{
		if (SearchPopup && PtInControl(pt,SearchPopup))
		{
			if (!(event->modifiers&cmdKey)) TBSearchPopup(pt);
		}
		else if (SearchGo && PtInControl(pt,SearchGo) && (event->modifiers&cmdKey))
			;// ignore
		else
		{
			if (event->modifiers&cmdKey)
			{
				ForceEnable = True;
				LastIdleRunTicks = 0;
				TBIdle(nil);
				ForceEnable = False;
			}
			ToolControl(pt,Win);
		}
	}
}

Boolean ToolDrag(ControlHandle theCtl, EventRecord *event)
{
	DragReference drag;
	ButtonData bd;
	Str255 s;
	RgnHandle rgn;
	Rect r;
	Boolean result = False;
	
	if (!Ignore && MyWaitMouseMoved(event->where,True))
	{
		Ignore = True;
		
		/*
		 * allow the item to be dragged
		 */
		if (MyNewDrag(Win,&drag)) return(False);
		
		/*
		 * the flavor is toolbar button
		 */
		bd = (*BD)[GetControlReference(theCtl)-1];
		TBButton2String(&bd,s);
		AddDragItemFlavor(drag, 1, TBAR_DRAG_FLAVOR, s, *s+1, 0);

		BeingDragged = theCtl;
		DroppedOnMe = False;
		/*
		 * build the region
		 */
		if (rgn = NewRgn())
		{
			GetControlBounds(theCtl,&r);
			RectRgn(rgn,&r);
			InsetRect(&r,1,1);
			RgnMinusRect(rgn,&r);
			GlobalizeRgn(rgn);
			GetRegionBounds(rgn,&r); SetDragItemBounds(drag, 1, &r);
			if (!MyTrackDrag(drag,event,rgn))
			{
				/*
				 * drag accepted
				 */
				if (!(MainEvent.modifiers&optionKey)  && !(CurrentModifiers()&optionKey))
					result = (DroppedOnMe || DragTargetWasTrash(drag));
			}
			DisposeRgn(rgn);
		}
		MyDisposeDrag(drag);
		BeingDragged = nil;
	}
	return(result);
}

/**********************************************************************
 * ToolControl - handle controls in the toolbar
 **********************************************************************/
Boolean ToolControl(Point pt, MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl;
	short part;
	EventRecord event;

	Deleted = False;
	if (part = FindControl(pt,winWP,&cntl))
	{
		DownTicks = TickCount();
		Ignore = False;
		
		event = MainEvent;
		HiliteControl(cntl,1);
		if (HasFeature (featureCustomizeToolbar) && event.modifiers&cmdKey && ToolDrag(cntl,&event))
		{
			UseFeature (featureCustomizeToolbar);
			TBDelButton(cntl);
		}
		else
		{
			part = TrackControl(cntl,pt,nil);
			HiliteControl(cntl,1);
			if (!Ignore && part) (*win->button)(win,cntl,event.modifiers,part);
		}
		if (!Deleted) HiliteControl(cntl,0);
	
		AuditHit((event.modifiers&shiftKey)!=0, (event.modifiers&controlKey)!=0, (event.modifiers&optionKey)!=0, (event.modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),GetControlReference(cntl)), event.what);
	 	return(True);
	}
	return(False);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr TBNewButton(short which)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	ButtonData bd;
	OSErr err;
	short n = HandleCount(BD); 
	ControlHandle button = GetNewControl(TB_CNTL,WinWP);
	short i;

	
	Zero(bd);
	if (!button) err = MemError();
	else err=PtrPlusHand_(&bd,BD,sizeof(bd));
		
	if (err) WarnUser(MEM_ERR,err);
	else
	{
		TBSetButtonType(button);
		for (i=n;i>which;i--) (*BD)[i] = (*BD)[i-1];
		bd.cntl = button;
		(*BD)[which] = bd;
		Dirty = True;
		if (button)
		{
			(*BD)[which].cntl = button;
			for (i=0;i<=n;i++)
				if ((*BD)[i].cntl) SetControlReference((*BD)[i].cntl,i+1);
			SizeToolbar(n+1);
			UpdateMyWindow(WinWP);
		}
	}
	return(err);
}

/************************************************************************
 * TBSetButtonType - set the appropriate uglification button type
 ************************************************************************/
OSErr TBSetButtonType(ControlHandle button)
{
	ControlButtonContentInfo ci;
	OSErr err;
	
	// set font and size while we're at it
SetBevelFontSize(button,SmallSysFontID(),GetRLong(TB_BUTTON_FONT_SIZE));

	// now set the type
	Zero(ci);
	ci.contentType = !VarHasIcon(VarCode) ? kControlContentTextOnly : kControlContentIconSuiteRes;
	err = SetControlData(button,0,kControlBevelButtonContentTag,sizeof(ci),(void*)&ci);
	
	ShowControl(button);
	return(err);
}


/**********************************************************************
 * 
 **********************************************************************/
void FlipToolbar(void)
{
	VarCode++;
	if (VarCode==tbvLimit)
	{
		VarCode = 1;
		Vertical = !Vertical;
	}
	SizeToolbar(CountStrn(ToolbarStrnID));
	UpdateMyWindow(GetMyWindowWindowPtr(Win));
	ToolbarNudgeAll(true);
}

/**********************************************************************
 * ToolbarRect - Rect of the toolbar
 **********************************************************************/
void ToolbarRect(Rect *r)
{
	if (Win)
	{
		*r = StrucRect;
		if(!EmptyRect(r)) InsetRect(r,-1,-1);
	}
	else SetRect(r,-REAL_BIG,-REAL_BIG,-REAL_BIG,-REAL_BIG);
}

/**********************************************************************
 * ToolbarNudge - nudge a window out of the way of any docked windows
 **********************************************************************/
Boolean ToolbarNudge(MyWindowPtr win,Boolean gently)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect newRect;
	Point corner;
	Boolean intersects;

#ifdef	FLOAT_WIN
	if (IsFloating(winWP)) return (false);
#endif	//FLOAT_WIN

	if (intersects=ToolbarNudgeRect(win, &newRect, gently))
	{
		SetPort_(GetWindowPort(winWP));
		corner.h=newRect.left;
		corner.v=newRect.top;
		LocalToGlobal(&corner);
		MoveWindow(winWP,corner.h,corner.v,False);
	}
	return(intersects);
}


/**********************************************************************
 * ToolbarNudgeRect - nudge a rect out of the way of any docked windows
 **********************************************************************/
Boolean ToolbarNudgeRect(MyWindowPtr win, Rect *newRect, Boolean gently)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Rect bounds;
	Rect intersect;
	Rect winRect;
	Point corner;
	MyWindowPtr	dockWin;
	WindowPtr	dockWinWP;
	Boolean	nudged = false;
	Rect testRect;

	GetStructureRgnBounds(winWP,&winRect);
	
	// "Gently" means we'll only move windows whose title bars are completely covered
	if (gently)
	{
		RgnHandle rgn = NewRgn();
		if (rgn)
		{
			if (GetWindowRegion)
			if (!GetWindowRegion(winWP,kWindowTitleBarRgn,rgn))
			if (!EmptyRgn(rgn))
				GetRegionBounds(rgn,&testRect);
			DisposeRgn(rgn);
		}
	}
	 	

	GetPortBounds(GetWindowPort(winWP),newRect);
	for (dockWinWP=FrontWindow();dockWinWP;dockWinWP=GetNextWindow(dockWinWP))
	{
		dockWin = GetWindowMyWindowPtr (dockWinWP);
		if (IsKnownWindowMyWindow(dockWinWP) && dockWin->windowType == kDockable && IsWindowVisible(dockWinWP))
		{
			GetStructureRgnBounds(dockWinWP,&bounds);
			
			// For gentle motion, check for only completely covered title bars
			if (gently)
			{
				SectRect(&bounds,&testRect,&intersect);
				if (!AboutSameRect(&intersect,&testRect)) continue; // ignore
			}
			
			// Either title bar is covered, or we're being rough
			if (SectRect(&bounds,&winRect,&intersect))
			{
				short overlapTop, overlapBottom, overlapLeft, overlapRight, overlapHi, overlapWi;
				corner.h = corner.v = 0;
				
				// The first order of business is to figure out exactly how we're overlapping
				overlapBottom = intersect.top == bounds.top ? RectHi(intersect) : 0;
				overlapTop = intersect.bottom == bounds.bottom ? RectHi(intersect) : 0;
				overlapRight = intersect.left == bounds.left ? RectWi(intersect) : 0;
				overlapLeft = intersect.right == bounds.right ? RectWi(intersect) : 0;
				overlapHi = MAX(overlapTop,overlapBottom);
				overlapWi = MAX(overlapLeft,overlapRight);
				
				// We'll move the window the least we can
				if (overlapHi && (!overlapWi || overlapHi < overlapWi))
				{
					if (overlapTop) corner.v = overlapTop;
					else corner.v = - overlapBottom;
				}
				else if (overlapWi)
				{
					if (overlapLeft) corner.h = overlapLeft;
					else corner.h = -overlapRight;
				}
				
				OffsetRect(newRect, corner.h, corner.v);
				nudged = true;
			}
		}
	}
	return nudged;
}

/**********************************************************************
 * ToolbarSect - does a rectangle overlap the toolbar?
 **********************************************************************/
Boolean ToolbarSect(Rect *r)
{
	Rect tb, sect;
	if (Win)
	{
		ToolbarRect(&tb);
		return (SectRect(&tb,r,&sect));
	}
	else return(False);
}

/**********************************************************************
 * ToolbarReduce - reduce a rectangle to fit the toolbar
 **********************************************************************/
void ToolbarReduce(Rect *r)
{
	if (Win)
	{
		Rect tb;
		ToolbarRect(&tb);
		if (Vertical) r->left += RectWi(tb);
		else r->top += RectHi(tb);
	}
}

#pragma segment ToolbarMain

#ifdef TASK_PROGRESS_ON
/**********************************************************************
 * ToolbarIdleControls 
 **********************************************************************/
void ToolbarIdleControls(void)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	
	if (WinWP && TaskProgButton && IsWindowVisible(WinWP) && TickCount()-IdleTicks>20)
	{
		PushGWorld();
		SetPort_(GetWindowPort(WinWP));
		TaskProgAnimate(TaskProgButton);
		PopGWorld();
		IdleTicks=TickCount();
	}
}

/**********************************************************************
 * ToolbarUserPaneIdle 
 **********************************************************************/
#define NUM_TP_ICONS (TASK_PROGRESS5_ICON - TASK_PROGRESS_ICON + 1)

static void TaskProgAnimate(ControlRef control)
{
	short id;
	static short frame=0;
	static unsigned long lastTick=0;
	
	if (NewError)
	{
		if (TickCount() - lastTick < 60)
			return;
		id = frame ? TASK_PROGRESS_ERROR_ICON : TASK_PROGRESS_ICON;
		if (frame++)
			frame=0;
	}
	else
	{
//		if (TickCount() - lastTick < 12)
//			return;
		if (!(CheckThreadRunning || SendThreadRunning))
		{
			if (frame == 1)
				return;
		}
		id = TASK_PROGRESS_ICON + frame;
		if (++frame >= (NUM_TP_ICONS - 1))
			frame = 0;
	}
	lastTick = TickCount();

 	SetBevelIcon(control,id,0,0,nil);
}

/**********************************************************************
 * IsTPIdleControlVisible 
 **********************************************************************/
Boolean IsTPIdleControlVisible(void)
{
	return Win && VarHasIcon(VarCode) && TaskProgButton != nil;
}
#endif

/**********************************************************************
 * ToolbarCursor - set the cursor for the toolbar
 **********************************************************************/
void ToolbarCursor(Point mouse)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	ControlHandle cntl;
	Rect r;
	short which;
	static long firstTicks;
	
	LocalToGlobal(&mouse);
	
	if (PtInRgn(mouse,MyGetWindowContentRegion(WinWP)))
	{
		PushGWorld();
		SetPort_(GetWindowPort(WinWP));
		TBIdle(nil);
		GlobalToLocal(&mouse);
		if (HasFeature (featureCustomizeToolbar) && CurrentModifiers()&cmdKey && TBInterStice(mouse,&which))
		{
			TBTip(nil);
			SetMyCursor(Vertical ? SPREAD_CURS_V : SPREAD_CURS);
		}
		else if (PeteCursorList(Win->pteList,mouse))
			;
		else if (FindControl(mouse,WinWP,&cntl))
		{
			SetMyCursor(arrowCursor);
			GetControlBounds(cntl,&r);
			TBTip(cntl);
		}
		else
		{
			TBTip(nil);
			SetRect(&r,mouse.h,mouse.v,mouse.h+1,mouse.v+1);
			SetMyCursor(arrowCursor);
		}
		PopGWorld();
	}
	else
	{
		SetMyCursor(arrowCursor);
		PushGWorld();
		SetPort_(GetWindowPort(WinWP));
		TBTip(nil);
		PopGWorld();
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void ShowToolbar(void)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);

	if (WinWP && !IsWindowVisible(WinWP))
	{
		//DebugStr("\pShowToolbar;sc");
		TBIdle(nil);
		ShowHide(WinWP,true);
		StashStructure(Win);
		GetStructureRgnBounds(WinWP,&StrucRect);
		TBTip(nil);
	}
}

void HideToolbar(void)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);

	if (WinWP && IsWindowVisible(WinWP))
	{
		//DebugStr("\pHideToolbar;sc");
		StashStructure(Win);
		ShowHide(WinWP,false);
		//TBTip(nil);
	}
}
void ToolbarBack(void)
{
//	if (Win && GetNextWindow(GetMyWindowWindowPtr(Win)))
//		SendBehind(GetMyWindowWindowPtr(Win),nil);
}

Boolean ToolbarShowing(void) {return(Win!=nil);}
/**********************************************************************
 * TBEventFilter - filter events to see if the toolbar is interested
 **********************************************************************/
Boolean TBEventFilter(EventRecord *event,Boolean oldResult)
{	
	if (Win)
	{
		OldDrag = (DragReference)-1;
		if (!InBG)
		if (event->what==activateEvt)
			TBSearchFocus(false);
		else if (event->what==mouseDown)
		{
			WindowPtr winWP;
			short wPart = FindWindow_(event->where,&winWP);
			MyWindowPtr win = GetWindowMyWindowPtr (winWP);
			if (win && win!=Win) TBSearchFocus(false);
		}
		else
		{
			if (event->what==keyDown)
			if (!(event->modifiers&cmdKey))
			if (!ModalWindow)
			if (!DragFxxkOff)
			{
				short b;
				Byte k;
				Byte c = event->message&charCodeMask;
				
				if ((c==returnChar || c==enterChar) && Win->pte)
				{
					// hit return with focus in the search field
					// start search
					TBSearchStart(nil);
					event->what = nullEvent;
				}
				else if (FKeys)
				{
					k = (event->message&keyCodeMask)>>8;
					for (b=0;b<N && b<15;b++)
						if (FunctionKeys[b]==k)
						{
							TBIdle(nil);
							if (IsControlActive((*BD)[b].cntl))
							{
								HiliteControl((*BD)[b].cntl,1);
								ToolbarButton(Win,(*BD)[b].cntl,event->modifiers,1);
								HiliteControl((*BD)[b].cntl,0);
							}
							else SysBeep(20L);
							event->what = nullEvent;
							return(False);
						}
				}
			}
		}
	}
	return(oldResult);
}

/************************************************************************
 * TBDisable - turn off the toolbar completely
 ************************************************************************/
void TBDisable(void)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	ControlHandle cntl;

	if (ToolbarShowing())
	{
		PushGWorld();
		SetPort_(GetWindowPort(WinWP));
		GetRootControl(WinWP,&cntl);
		DeactivateControl(cntl);
		PopGWorld();
	}
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean ToolbarMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(win,modifiers)
	if (menu==FILE_MENU && item==FILE_SAVE_ITEM)
	{
		if (Dirty) SaveToolbar();
		return(True);
	}
	else return(False);
}
#pragma segment Toolbar

/**********************************************************************
 * 
 **********************************************************************/
void TBDelButton(ControlHandle cntl)
{
	short which = GetControlReference(cntl);
	short n = HandleCount(BD);
	short i;
	
	TBZapButtonData(which);
	if (which<n)
	{
		BMD(&(*BD)[which],&(*BD)[which-1],sizeof(**BD)*(n-which));
		for (i=which;i<n;i++) SetControlReference((*BD)[i-1].cntl,i);
	}
	SetHandleSize((Handle)BD,GetHandleSize((Handle)BD)-sizeof(**BD));
	Dirty = True;
	Deleted = True;
	DisposeControl(cntl);
	if (cntl == TaskProgButton)
		TaskProgButton = nil;
	SizeToolbar(n-1);
	TipCntl = LastCntl = nil;
}

/**********************************************************************
 * TBRemoveDefunctMenuButton - remove a button referring to a menu item
 *  we no longer have
 **********************************************************************/
OSErr TBRemoveDefunctMenuButton(short menu,short item)
{
	short i;
	ButtonData bd;
	
	for (i=0;i<N;i++)
	{
		bd = (*BD)[i];
		if (bd.kind == tbkMenu && (bd.select>>16L)==menu && ((unsigned short)bd.select)==item)
		{
			TBDelButton(bd.cntl);
			i--;	//reexamine buttons
		}
	}
	return noErr;	/* nothing to return here */
}

/**********************************************************************
 * TBRemoveDefunctNicknameButton - remove a button referring to a nickname
 *  we no longer have
 **********************************************************************/
void TBRemoveDefunctNicknameButton(PStr nickname)
{
	ButtonData	bd;
	short				i;
	
	if (Win)
		for (i = 0;i < N; i++) {
			bd = (*BD)[i];
			if (bd.kind == tbkNick && StringSame (bd.title, nickname)) {
				TBDelButton (bd.cntl);
				i--;	//reexamine buttons
			}
		}
}

/**********************************************************************
 * LoadToolbarButton - load buttons in the toolbar
 **********************************************************************/
OSErr LoadToolbarButton(short i)
{
	Str255 s;
	return(LoadButtonString(i,GetRString(s,ToolbarStrnID+i)));
}

/**********************************************************************
 * LoadButtonString - load a string into a button
 **********************************************************************/
OSErr LoadButtonString(short i,PStr s)
{
	Str255 menuStr, itemStr;
	Str31 pfix;
	short menu, item;
	UPtr spot;
	FSSpec spec;
	void *dataPtr = nil;
	long size;
	Boolean wasXfer;
	short modifiers;
	long key;
	Boolean mailbox;
	ControlHandle button = (*BD)[i-1].cntl;
	OSType type, creator;
	Str255 scratch;
	
	if (button)
	{
		TBSetCTitle(button,"");
		spot = s+1;
		if (PToken(s,menuStr,&spot,":"))
		{
			StringToNum(menuStr,&key);
			modifiers = key;
			if (PToken(s,menuStr,&spot,":"))
			{
				TBButtonEnum buttonKind = FindSTRNIndex(TBButtonStrn,menuStr);
				if (buttonKind==tbkMenu)
				{
					if (PToken(s,menuStr,&spot,":") &&
							PToken(s,itemStr,&spot,"\377"))
					{
						if (menu=FindMenuByName(menuStr))
						{
							if (menu==FILE_MENU && HaveTheDiseaseCalledOSX() && EqualStrRes(itemStr,FILE_QUIT_ITEM_STR))
							{
								item = FILE_QUIT_ITEM;
							}
							else
							{
								SetMenuTexts(modifiers,True);
								item = 0;
								
								PCopy(scratch,itemStr);
								PSCat(scratch,"\p<I");
								
								if (mailbox = ((menu==MAILBOX_MENU||menu==TRANSFER_MENU) &&
															 !EqualStrRes(scratch,NEW_ITEM_TEXT) &&
															 !EqualStrRes(scratch,OTHER_ITEM_TEXT)))
								{
									if (wasXfer = (menu==TRANSFER_MENU))
									{
										menu==MAILBOX_MENU;
										TrimPrefix(itemStr,GetRString(pfix,TRANSFER_PREFIX));
									}
					
									if (EqualStrRes(itemStr,FILE_ALIAS_IN)) GetRString(scratch,IN);
									else if (EqualStrRes(itemStr,FILE_ALIAS_OUT)) GetRString(scratch,OUT);
									else if (EqualStrRes(itemStr,FILE_ALIAS_JUNK)) GetRString(scratch,JUNK);
									else if (EqualStrRes(itemStr,FILE_ALIAS_TRASH)) GetRString(scratch,TRASH);
									else PCopy(scratch,itemStr);
					
									if (!BoxSpecByName(&spec,scratch))
									{
										dataPtr = &spec;
										size = sizeof(FSSpec);

										// Display only the mailbox name in the toolbar.
										Zero(scratch);
										BMD(itemStr+1, scratch, itemStr[0]);
										PathToMailboxName (scratch, itemStr, ':');

										PCopy(scratch,itemStr);
										if (wasXfer) PCopy(itemStr,pfix);
										else *itemStr = 0;
										PCat(itemStr,scratch);
									}
									else menu=0;
								}
							}
							if (TBSpecialStr(menu,itemStr,&item,&modifiers) ||
									TBMovedMenu(&menu,itemStr,&item) ||
									GetMHandle(menu) && (mailbox || (item=FindItemByName(GetMHandle(menu),itemStr))))
							{
								InstallButtonMenu(i,menu,item,itemStr,modifiers,dataPtr,size);
							}
						}
					}
				}
				else if (buttonKind==tbkKey)
				{
					if (PToken(s,menuStr,&spot,":"))
					{
						StringToNum(menuStr,&key);
						InstallButtonKey(i,key,modifiers);
					}
				}
				else if (buttonKind==tbkFile)
				{
					if (PToken(s,menuStr,&spot,":"))
					if (PToken(s,itemStr,&spot,":"))
					{
						StringToNum(menuStr,&key);
						BMD(spot,&type,4);
						BMD(spot+4,&creator,4);
						InstallButtonAliasId(i,key,type,creator,itemStr);
					}
				}
				else if (buttonKind==tbkAd)
				{
					TBAdInfo	adInfo;
					
					if (PToken(s,menuStr,&spot,":"))
					if (PToken(s,adInfo.title,&spot,":"))
					{					
						adInfo.adId.server = modifiers;
						StringToNum(menuStr,&adInfo.adId.ad);
						adInfo.deleted = false;
						adInfo.iconSuite = AdGetTBIcon(adInfo.adId);
						InstallButtonAd(i,&adInfo);
					}
				}
				else if (buttonKind==tbkNick)
				{
					if (PToken(s,menuStr,&spot,":"))
					InstallButtonNickname(i, menuStr);
				}
			}
		}
		HiliteControl(button,255);
	}
	return(0);
}

/**********************************************************************
 * 
 **********************************************************************/
void InstallButtonAliasId(short which,short id,OSType type,OSType creator,PStr name)
{
	ButtonData bd;
	TBAliasData tbd;
	
	TBZapButtonData(which);
	bd = (*BD)[which-1];
	bd.kind = tbkFile;
	bd.modifiers = 0;
	tbd.id = id;
	tbd.type = type;
	tbd.creator = creator;
	if (bd.data = NuHandle(sizeof(tbd)))
		**(TBAliasHandle)bd.data = tbd;
	if (VarHasIcon(VarCode))
		if (type=='fdrp')
			SetBevelIcon(bd.cntl,kGenericFolderIconResource,nil,nil,nil);
		else
			SetBevelIcon(bd.cntl,nil,type,creator,nil);
	(*BD)[which-1] = bd;
	TBSetCTitle(bd.cntl,name);
	if (TaskProgButton==bd.cntl) TaskProgButton = nil;
}

/**********************************************************************
 * TBSpecialStr - fix up some menu items that vary with preferences
 **********************************************************************/
Boolean TBSpecialStr(short menu,PStr itemStr,short *item,short *modifiers)
{
	Str63 searchWebStr;
	
	if (menu==FILE_MENU && EqualStrRes(itemStr,CHECK_MAIL))
		*item = FILE_CHECK_ITEM;
	else if (menu==EDIT_MENU && EqualStrRes(itemStr,UNDO))
		*item = EDIT_UNDO_ITEM;
	else if (menu==FIND_HIER_MENU && StringSame(itemStr,ComposeRString(searchWebStr,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB)))
		*item = FIND_SEARCH_WEB_ITEM;
	else if (menu==MESSAGE_MENU && (EqualStrRes(itemStr,REPLY)||EqualStrRes(itemStr,REPLY_ALL)))
	{
		*item = MESSAGE_REPLY_ITEM;
		if ((0==EqualStrRes(itemStr,REPLY_ALL)) != (0==PrefIsSet(PREF_REPLY_ALL)))
			*modifiers |= optionKey;
		else
			*modifiers &= ~optionKey;
	}
	else if (menu==MESSAGE_MENU &&
		(EqualStrRes(itemStr,QUEUE_M_ITEM)||EqualStrRes(itemStr,SEND_M_ITEM)||
		 EqualStrRes(itemStr,OLD_QUEUE_M_ITEM)||EqualStrRes(itemStr,OLD_SEND_M_ITEM)))
	{
		*item = MESSAGE_QUEUE_ITEM;
		if ((0==EqualStrRes(itemStr,SEND_M_ITEM)) != (0==PrefIsSet(PREF_AUTO_SEND)))
			*modifiers |= optionKey;
		else
			*modifiers &= ~optionKey;
	}
	else if (menu==LABEL_HIER_MENU && AllDigits(itemStr+1,*itemStr) && *itemStr<3)
	{
		long label;
		StringToNum(itemStr,&label);
		*item = Label2Menu(label);
		MyGetItem(GetMHandle(menu),*item,itemStr);
		return true;
	}
#ifdef	IMAP
	else if (menu==SERVER_HIER_MENU)
	{
		*item = FindSTRNIndex(ServerMenuStrnStrn,itemStr);
		if (*item > ksvmLimit) *item -= ksvmLimit;
		return (true);	// leave  itemStr set to be used on the button
	}
#endif
	else if (menu==TABLE_HIER_MENU)
		;// no fixup needed
	else return(False);
	
	*itemStr = 0;
	return(True);
}

/**********************************************************************
 * TBMovedMenu - fix up some menu items that have moved since the last version
 **********************************************************************/
Boolean TBMovedMenu(short *menu,PStr itemStr,short *item)
{
	short	winItems[] = { WIN_ALIASES_ITEM,WIN_PH_ITEM,WIN_FILTERS_ITEM,WIN_MAILBOX_ITEM };
	short	i;
	Str32	s;
	
	if (*menu==SPECIAL_MENU)
	{
		//	Tool windows moved from Speical menu to Window menu
		MenuHandle	windowMenu = GetMHandle(WINDOW_MENU);
		
		for (i=0;i<sizeof(winItems)/sizeof(short);i++)
		{			
			if (StringSame(itemStr,MyGetItem(windowMenu,winItems[i],s)))
			{
				*menu = WINDOW_MENU;
				*item = winItems[i];
				return true;
			}
		}
	}
	return false;
}

/**********************************************************************
 * ToolbarAddMenuButton - add a menu item to the end of the toolbar
 **********************************************************************/
Boolean TBAddMenuButton(short menu,short item,PStr itemText)
{
	short	i;
	short	n;
	ButtonData bd;
	
	if (!Win) return false;	//	No toolbar!
	
	//	Make sure the item isn't already in the toolbar
	n = N;
	for (i=0;i<n;i++)
	{
		bd = (*BD)[i];
		if (bd.kind == tbkMenu && (bd.select>>16L)==menu && ((unsigned short)bd.select)==item)
		{
			return false;	//	It's already there
		}
	}
	
	//	Create new button
	if (!TBNewButton(n))
		InstallButtonMenu(n+1,menu,item,itemText,0,nil,0);
	
	return true;	//	Successfully added
}

/**********************************************************************
 * TBAddAdButton - add an ad to the toolbar
 **********************************************************************/
void TBAddAdButtons(TBAdHandle hTBAds)
{
	short	numAds,buttonIdx,i;
	short	n;
	
	if (!Win) return;	//	No toolbar!

	//	Delete any ad buttons no longer in use
	numAds = HandleCount(hTBAds);
	for(buttonIdx=N-1;buttonIdx>=0;buttonIdx--)
	{
		BDPtr	pBD = &(*BD)[buttonIdx];
		
		if (pBD->kind == tbkAd)
		{
			Boolean	found = false;
			AdId	adId;
			
			adId.ad = pBD->select;
			adId.server = pBD->modifiers;
			for(i=0;i<numAds;i++)
			{
				if (SameAd(&(*hTBAds)[i].adId,&adId))
				{
					if (!(*hTBAds)[i].deleted)
					{
						found = true;
						(*hTBAds)[i].deleted = true;	//	We don't want to ad it again
					}
					break;
				}
			}
			if (!found)
				TBDelButton(pBD->cntl);
		}
	}
	
	//	Install ad buttons
	LDRef(hTBAds);
	for(i=0;i<numAds;i++)
	{		
		//	Create new button
		if (!(*hTBAds)[i].deleted && !TBNewButton(n=N))
			InstallButtonAd(n+1,&(*hTBAds)[i]);
	}
	UL(hTBAds);
}

/**********************************************************************
 * InstallButtonAd - put an ad in a button
 **********************************************************************/
void InstallButtonAd(short i,TBAdInfo *pAd)
{
	BDPtr	pBD = &(*BD)[i-1];
	ControlHandle button = (*BD)[i-1].cntl;
	
	pBD->kind = tbkAd;
	pBD->select = pAd->adId.ad;
	pBD->modifiers = pAd->adId.server;
	PCopy(pBD->title,pAd->title);
	if (VarHasIcon(VarCode)) SetBevelIcon(pBD->cntl,0,0,0,pAd->iconSuite);
	TBSetCTitle(button,pAd->title);
}

/**********************************************************************
 * TBUpdateAdButtonIcon - need to redraw this button ad
 **********************************************************************/
void TBUpdateAdButtonIcon(AdId setAdId,Handle iconSuite)
{
	short i;
	AdId	adId;			
	
	if (Win)
	{
		for (i=0;i<N;i++)
		{
			BDPtr	pBD = &(*BD)[i];

			if (pBD->kind == tbkAd)
			{
				adId.ad = pBD->select;
				adId.server = pBD->modifiers;
				if (SameAd(&setAdId,&adId))
				{
					if (VarHasIcon(VarCode)) SetBevelIcon(pBD->cntl,0,0,0,iconSuite);
					break;
				}
			}
		}
	}
}


			
/**********************************************************************
 * InstallButtonNickname - put a nickname in a button
 **********************************************************************/
void InstallButtonNickname (short i, PStr nickname)
{
	ButtonData		bd;
	ControlHandle	button = (*BD)[i-1].cntl;

	button = (*BD)[i - 1].cntl;
	Zero (bd);
	bd.cntl = button;
	bd.kind = tbkNick;
	PCopy (bd.title, nickname);
	(*BD)[i - 1] = bd;
	if (VarHasIcon(VarCode))
		SetNicknameIcon (button, nickname);
	TBSetCTitle (button, nickname);
	if (TaskProgButton==button) TaskProgButton = nil;
}


/**********************************************************************
 * TBRename - rename a toolbar button of a particular kind
 **********************************************************************/
Boolean TBRename (TBButtonEnum kind, PStr oldName, PStr newName)

{
	ButtonData	bd;
	short				i;
	
	//	Find the button with the old name
	if (Win)
		for (i = 0; i < N; ++i) {
			bd = (*BD)[i];
			if (bd.kind == kind && StringSame (bd.title, oldName)) {
				PCopy (bd.title, newName);
				(*BD)[i] = bd;
				TBSetCTitle (bd.cntl, newName);
				Dirty = True;
				return (true);
			}
		}
	return (false);
}

Boolean TBRenameNickButton (PStr oldName, PStr newName)

{
	return (TBRename (tbkNick, oldName, newName));
}


/**********************************************************************
 * InstallButtonMenu - put a menu choice in a button
 **********************************************************************/
void InstallButtonMenu(short i, short menu, short item,PStr itemText,short modifiers,void *data,long size)
{
	Handle dataH;
	ControlHandle button = (*BD)[i-1].cntl;
	Str255 itemStr;
	Str255 titleStr;
	ButtonData bd;
	short id;
	Boolean special=False;
	MenuHandle mh;
	Handle suite;
	Byte type;
	
	Zero(bd);
	bd.select = (menu<<16L)|item;
	bd.modifiers = modifiers;
	bd.kind = tbkMenu;
	bd.cntl = button;
	TBZapButtonData(i);
	
	if (menu==FIND_HIER_MENU && item==FIND_SEARCH_WEB_ITEM) SetPrefBit(PREF_SEARCH_WEB_BITS,prefSearchWebTBButtonInstalled);
	
	if (data)
	{
		if (dataH = NuHandle(size)) BMD(data,*dataH,size);
		bd.data = dataH;
	}
	if (mh=GetMHandle(menu))
	{
		if (!itemText  || !*itemText)
		{
			if (!(special=TBInstSpecialMenus(menu,item,modifiers,itemStr,titleStr)))
				MyGetItem(GetMHandle(menu),item,itemStr);
		}
		else PCopy(itemStr,itemText);
	}
	else *itemStr = 0;
	if (IsRecipMenu(menu) || IsStationMenu(menu))
		bd.data = NuDHTempOK(itemStr,*itemStr+1);
	(*BD)[i-1] = bd;
	if (!special) ModifierNames(titleStr,modifiers);
	PSCat(titleStr,itemStr);
	if (VarHasIcon(VarCode))
	{
#ifdef TASK_PROGRESS_ON	// 
		if (menu==WINDOW_MENU && item==WIN_TASKS_ITEM)
		{
			TaskProgButton = button;
		}
		else if (TaskProgButton==button)
		{
			TaskProgButton = nil;
		}
#endif
		if (menu==FILE_MENU && (item==FILE_PRINT_ITEM||item==FILE_PRINT_ONE_ITEM) && !SetPrinterIcon(button))
			;
		else if (menu==SCRIPTS_MENU)
		{
			FSSpec spec;
			SInt32	gestaltResult;
			IconRef iconRef;
			SInt16	theLabel;
			Boolean	custIcon = false;

			// Check for a custom icon. We need Icon Services to do this			
			if (!GetScriptFolderSpec(&spec))
			if (!Gestalt(gestaltIconUtilitiesAttr,&gestaltResult) && 
				(gestaltResult&gestaltIconUtilitiesHasIconServices) && 
				GetIconRefFromFile)
			{
				GetMenuItemText(GetMHandle(menu),item,spec.name);
				if (!GetIconRefFromFile(&spec,&iconRef,&theLabel))
				{
					SetBevelIconIconRef(bd.cntl,iconRef);
					custIcon = true;
				}
			}
			if (!custIcon)
				SetBevelIcon(bd.cntl,nil,'osas','ToyS',nil);
		}
		/* I don't believe we really want to do this anymore since we now have true nickname buttons (jp)...
		else if (menu == NEW_TO_HIER_MENU && !SetNicknameIcon(button, itemText))
			;  */
		else if (suite = ETLMenu2Icon(menu,item))
		{
			SetBevelIcon(button,0,0,0,suite);
		}
		else if (menu==MESSAGE_MENU && item==MESSAGE_JUNK_ITEM)
		{
			SetBevelIcon(button,MAKE_JUNK_ICON,0,0,nil);
		}
		else if (menu==EMOTICON_HIER_MENU && !GetMenuItemIconHandle(GetMHandle(menu),item,&type,&suite) && suite)
		{
			Handle dupSuite;
			DupIconSuite(suite,&dupSuite,false);
			SetBevelIcon(button,0,0,0,dupSuite);
		}
		else
		{
			if (!(id = Names2Icon(itemStr,titleStr))) id = Menu2Icon(menu,item,modifiers);
			SetBevelIcon(button,id,0,0,nil);
		}
	}
	TBSetCTitle(button,titleStr);
}

/**********************************************************************
 * SetPrinterIcon - put the current printer in a toolbar button icon
 **********************************************************************/
OSErr SetPrinterIcon(ControlHandle button)
{
	AliasHandle res = GetResource_('alis',-8192);
	FSSpec spec;
	Boolean changed;
	OSErr err;
	FInfo info;
	
	if (!res) return(resNotFound);
	Zero(spec);
	if (!(err=ResolveAlias(nil,res,&spec,&changed)))
	if (!(err=FSpGetFInfo(&spec,&info)))
	{
		KillBevelIcon(button);
		SetBevelIcon(button,0,info.fdType,info.fdCreator,nil);
	}
	return(err);
}


/**********************************************************************
 * SetNicknameIcon - put a nickname photo into a toolbar button icon
 **********************************************************************/
OSErr SetNicknameIcon(ControlHandle button, PStr nickname)

{
	FSSpec	urlSpec;
	Str255	tag;
	Handle	urlString;
	OSErr		theError;
	IconRef iconRef;
	SInt32	gestaltResult;
	SInt16	theLabel;
	long		hashName;
	short		ab,
					nick;
	
	theError	= resNotFound;
	urlString	= nil;
	if (nickname) {
		hashName	= NickHash (nickname);
		for (ab = 0; ab < NAliases; ++ab) {
			nick = NickMatchFound(((*Aliases)[ab].theData), hashName, nickname, ab);
			if (ValidNickname (nick))
				break;
		}
		if (ValidNickname (nick))
			if (urlString = GetTaggedFieldValue (ab, nick, GetRString (tag, ABReservedTagsStrn + abTagPicture))) {
				theError = URLStringToSpec (urlString, &urlSpec);
				if (!theError)
					if (!Gestalt(gestaltIconUtilitiesAttr,&gestaltResult) && (gestaltResult&gestaltIconUtilitiesHasIconServices) && GetIconRefFromFile)
						if (!GetIconRefFromFile (&urlSpec, &iconRef, &theLabel)) {
							KillBevelIcon(button);
							theError = SetBevelIconIconRef (button, iconRef);
						}
			}
	}
	// If an error occurs -- or if we're actually intending to assign a generic icon,
	// assign a generic nickname icon
	if (theError || !urlString)
		SetBevelIcon (button, ValidNickname (nick) ? ABSetIcon (ab, nick) : NICKNAME_ICON, 0, 0, nil);
	ZapHandle (urlString);
	return (theError);
}



/**********************************************************************
 * TBInstSpecialMenus - install special menus
 **********************************************************************/
Boolean TBInstSpecialMenus(short menu,short item,short modifiers,PStr itemStr,PStr titleStr)
{
	ModifierNames(titleStr,modifiers);
	if (menu==FILE_MENU && item==FILE_CHECK_ITEM)
	{
		if (!(modifiers&optionKey)) GetRString(itemStr,CHECK_MAIL);
		else return(false);
	}
	else if (menu==FILE_MENU && item==FILE_QUIT_ITEM && HaveTheDiseaseCalledOSX())
	{
		GetRString(itemStr,FILE_QUIT_ITEM_STR);
		return true;
	}
	else if (menu==FIND_HIER_MENU && item==FIND_SEARCH_WEB_ITEM)
	{
		ComposeRString(itemStr,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB);
		return true;
	}
	else if (menu==EDIT_MENU && item==EDIT_UNDO_ITEM) GetRString(itemStr,UNDO);
//#ifdef NEVER
	else if (menu==MESSAGE_MENU && item==MESSAGE_REPLY_ITEM)
	{
		ModifierNames(titleStr,modifiers&~optionKey);
		if (modifiers & optionKey)
			GetRString(itemStr,PrefIsSet(PREF_REPLY_ALL)?REPLY:REPLY_ALL);
		else
			GetRString(itemStr,PrefIsSet(PREF_REPLY_ALL)?REPLY_ALL:REPLY);
	}
	else if (menu==MESSAGE_MENU && item==MESSAGE_QUEUE_ITEM)
	{
		ModifierNames(titleStr,modifiers&~optionKey);
		if (modifiers & optionKey)
			GetRString(itemStr,PrefIsSet(PREF_AUTO_SEND)?QUEUE_M_ITEM:SEND_M_ITEM);
		else
			GetRString(itemStr,PrefIsSet(PREF_AUTO_SEND)?SEND_M_ITEM:QUEUE_M_ITEM);
	}
//#endif
	else return(False);
	return(True);
}

/**********************************************************************
 * ModifierNames - write out the names of the modifier keys
 **********************************************************************/
PStr ModifierNames(PStr string,short modifiers)
{
	*string = 0;
	if (modifiers & shiftKey) PCatR(string,SHIFT_PLUS);
	if (modifiers & optionKey) PCatR(string,OPTION_PLUS);
	if (modifiers & cmdKey && HasFeature (featureCustomizeToolbar)) PCatR(string,COMMAND_PLUS);
	if (modifiers & controlKey) PCatR(string,CONTROL_PLUS);
	return(string);
}

/**********************************************************************
 * InstallButtonKey - put a keystroke in a button
 **********************************************************************/
void InstallButtonKey(short i, long key,short modifiers)
{
	short id = Key2Icon(key,modifiers);
	ControlHandle button = (*BD)[i-1].cntl;
	Str255 itemStr;
	Str63 modStr;
	Str31 keyName;
	short fmt = SPECIAL_KEY_FMT;
	uShort smallKey;
	
	(*BD)[i-1].select = key;
	(*BD)[i-1].modifiers = modifiers;
	(*BD)[i-1].kind = tbkKey;
	TBZapButtonData(i);
	if (VarHasIcon(VarCode)) SetBevelIcon(button,id,0,0,nil);
	
	smallKey = key & 0xff;
	
	if (smallKey<=' ')
	{
		GetRString(keyName,KeyNameStrn+(smallKey+1));
		fmt = SPECIAL_KEY_FMT;
	}
	else
	{
		keyName[0] = 1;
		keyName[1] = (UnadornKey(key,modifiers))&0xff;
		fmt = KEY_FMT;
	}
	ComposeRString(itemStr,fmt,ModifierNames(modStr,modifiers),keyName);
	TBSetCTitle(button,itemStr);
	if (TaskProgButton==button) TaskProgButton = nil;
}

/**********************************************************************
 * Key2Icon - find the right icon for a keystroke
 **********************************************************************/
short Key2Icon(short key, short modifiers)
{
#pragma unused(key,modifiers)
	return(KEY_ICON);
}

/**********************************************************************
 * Menu2Icon - get the icon for a menu item
 **********************************************************************/
short Menu2Icon(short menu,short item,short modifiers)
{
	Handle r;
	Str255 menuName;
	Str63 modName;
	MenuHandle mh;
	short id;
	RGBColor color;
	
	if (menu==kHMHelpMenuID) menu = HELP_NOTMENU;
	
	if (mh=GetMHandle(menu)) GetMenuTitle(mh,menuName);
	
	if (id=Names2Icon(menuName,ModifierNames(modName,modifiers))) return(id);
	
	SetResLoad(false);
	r = GetResource('ICN#',menu*20+item);
	SetResLoad(true);
	if (r) return(menu*20+item);
	
	SetResLoad(false);
	r = GetResource('ICN#',menu*20);
	SetResLoad(true);
	if (r) return(menu*20);

	switch(menu)
	{
		case MAILBOX_MENU:
			return(MAILBOX_ICON);
			break;
		
		case HELP_NOTMENU: return(-16490);
		
#ifdef LABEL_ICONS
		case LABEL_HIER_MENU:
			if (item<=2) return(LABEL_SICN_TEMPLATE);
			else if (item<=9) return(LABEL_SICN_BASE+item-3);
			else return(LABEL_SICN_BASE+item-4);
			break;

		case COLOR_HIER_MENU:
			id = TOOLBAR_COLOR_SICN_BASE+item;
			if (item<=2) DEFAULT_COLOR(color);
			else GetItemColor(menu,item,&color);
			RefreshRGBIcon(id,&color,TOOLBAR_COLOR_SICN_TEMPLATE,nil);
			return(id);
			break;
#endif
				
		default: return(MENU_ICON);
	}
}

/**********************************************************************
 * ToolbarMailbox - return the mailbox represented by a toolbar button
 **********************************************************************/
OSErr ToolbarMailbox(short which,FSSpecPtr spec)
{
	ButtonData bd;
	short mnu;
	
	bd = (*BD)[which-1];
	if (bd.kind!=tbkMenu) return(fnfErr);
	mnu = (bd.select>>16)&0xffff;
	if ((mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) && bd.data)
	{
		if (spec) *spec = **(FSSpecHandle)bd.data;
		return(noErr);
	}
	return(fnfErr);
}

/**********************************************************************
 * ToolbarButton - execute a button press in the toolbar
 **********************************************************************/
void ToolbarButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
#pragma unused(win,part)
	WindowPtr topWin = FrontWindow_();
	short mnu, item;
	long select;
	EventRecord event;
	short controlRef = GetControlReference(button)-1;
	ButtonData bd;
	Boolean hasBD;
	Boolean swap;
	AdId	adId;
	
	SetPort_(GetMyWindowCGrafPtr(Win));
	TB_RESTORE_STUFF;
	
	if (hasBD = (0<=controlRef && controlRef<N)) bd = (*BD)[controlRef];
	
	if (!hasBD && modifiers&cmdKey) return;	// punt if we try to customize something we can't
	
	if (HasFeature (featureCustomizeToolbar) && modifiers&cmdKey)
	{
		UseFeature (featureCustomizeToolbar);
		ConfigureTBButton(GetControlReference(button));
	}
	else if (button==SearchGo)
	{
		if (!(modifiers&cmdKey)) TBSearchStart(nil);
	}
	else if (hasBD)
	{
		modifiers |= bd.modifiers;
		// hmmmmmmmmmmmmm.  A few things check this rather than the 
		// modifiers passed in.  I'm not sure this is a good idea,
		// but I'm going to try it for now.  SD 3/19/03
		MainEvent.modifiers = modifiers;
		switch(bd.kind)
		{
			case tbkMenu:
				if (select = bd.select)
				{
					EnableMenuItems(False);
					select = TBExtractMenu(&bd,&mnu,&item);
					if (mnu==FILE_MENU && item==FILE_QUIT_ITEM)
						PleaseQuit = true;
					else if (IsEnabled(mnu,item))
					{
						if (DoWebFindWarning(mnu,item))
						{
							swap = (mnu==MESSAGE_MENU && item==MESSAGE_QUEUE_ITEM && (modifiers&optionKey));
							if (swap) {TogglePref(PREF_AUTO_SEND);modifiers&=~optionKey;}
							DoMenu(topWin,select,modifiers);
							if (swap) TogglePref(PREF_AUTO_SEND);
						}
					}
					else
						SysBeep(20);
				}
				break;
				
			case tbkKey:
				Zero(event);
				event.what = keyDown;
				event.modifiers = modifiers;
				event.message = bd.select;
				event.when = TickCount();
				HandleEvent(&event);
				break;
			
			case tbkFile:
				TBFileButton(&bd);
				break;
			
			case tbkAd:
				adId.ad = bd.select;
				adId.server = bd.modifiers;
				AdUserClick(adId);
				break;
			
			case tbkNick:
				// A click on a nickname toolbar button should behave like a click in the recipient buttons of the address book
				{
					MyWindowPtr	win = TopCompositionWindow (false, false);
					GrafPtr			oldPort;
					MessHandle	messH;
					HeadSpec		hs;
					long 				start;
					Boolean			wantExpansion;
					
					if (win && CompHeadFind (messH = Win2MessH(win), TO_HEAD, &hs)) {
						wantExpansion = ((modifiers & optionKey) != 0);
						if (HasFeature (featureNicknameWatching) && PrefIsSet (PREF_NICK_AUTO_EXPAND))
							wantExpansion = true;
						GetPort (&oldPort);
						SetPort_ (GetMyWindowCGrafPtr (win));
						PeteSelect (win, win->pte, hs.stop, hs.stop);
						PetePrepareUndo (win->pte, peUndoPaste, -1, -1, &start, nil);
						InsertCommaIfNeedBe (TheBody, &hs);
						InsertAlias (win->pte, &hs, bd.title, wantExpansion, start, false);
						ShowMyWindow (GetMyWindowWindowPtr (win));
						UpdateSum (messH, SumOf(messH)->offset, SumOf(messH)->length);
						SetPort_ (oldPort);
					}
				}
				break;
				
			default:
				ConfigureTBButton(GetControlReference(button));
				break;
		}
	}
}


/**********************************************************************
 * TBExtractMenu - get menu stuff from a button
 **********************************************************************/
long TBExtractMenu(BDPtr bd,short *mnu,short *item)
{
	long select = bd->select;
	Str255 itemStr;
	FSSpec spec;
	
	*mnu = (select>>16)&0xffff;
	*item = select&0xffff;
	if (IsRecipMenu(*mnu) || IsStationMenu(*mnu))
	{
		if (bd->data)
		{
			PCopy(itemStr,*bd->data);
			if (*item=FindItemByName(GetMHandle(*mnu),itemStr))
				select = (*mnu)<<16L | *item;
		}
	}
	else if ((*mnu==MAILBOX_MENU || *mnu==TRANSFER_MENU) && *item!=MAILBOX_NEW_ITEM && *item!=MAILBOX_OTHER_ITEM)
	{
		spec = **(FSSpecHandle)bd->data;
		Spec2Menu(&spec,*mnu==TRANSFER_MENU,mnu,item);
		select = *mnu<<16L | *item;
	}
	return(select);
}

/**********************************************************************
 * 
 **********************************************************************/
void TBFileButton(BDPtr bdp)
{
	short id = (*(TBAliasHandle)bdp->data)->id;
	AliasHandle res = GetResource_(TB_ALIAS_TYPE,id);
	FSSpec settings, spec;
	Boolean changed;
	OSErr err;
	Str255 name;
	
	GetControlTitle(bdp->cntl,name);

	if (res)
	{
		GetFileByRef(SettingsRefN,&settings);
		err = ResolveAlias(&settings,res,&spec,&changed);
		if (!err)
		{
			if (changed) ChangedResource((Handle)res);
			OpenOtherDoc(&spec,False,false,nil);
		}
		else FileSystemError(TB_NO_FILE,name,err);
	}
	else
	{
		if (AlertStr(TBAR_REM_ALRT,Note,name)==OK)
			TBDelButton(bdp->cntl);
	}
}

/**********************************************************************
 * ConfigureTBButton - configure a toolbar button
 **********************************************************************/
Boolean ConfigureTBButton(short which)
{
	WindowPtr theWindow;
	DialogPtr dgPtr;
	MyWindowPtr	dgPtrWin;
	EventRecord event;
	Str255 s;
	FSSpec spec;
	ControlHandle button = (*BD)[which-1].cntl;
	Boolean val = False;

	TBTip(button);
	TBTipLo(nil);

	HiliteControl(button,1);
	
	EnableMenus(nil,True);
	EnableMenuItems(True);
	
	DrawMenuBar();

	SetMyCursor(arrowCursor); SFWTC = True;

//	dgPtr = GetNewDialog(TBAR_MENU_DLOG,nil,InFront);
//	DrawDialog(dgPtr);
	dgPtrWin = GetNewMyDialog (TBAR_MENU_DLOG,nil,nil,InFront);
	dgPtr = GetMyWindowDialogPtr (dgPtrWin);
	if (!dgPtrWin || !dgPtr)
		return (false);
		
	SetPort_(GetWindowPort(GetDialogWindow(dgPtr)));
	ActivateMyWindow(GetDialogWindow(dgPtr),true);
	PushModalWindow(GetDialogWindow(dgPtr));
	
	Zero(gTBMenu);
	gTBMenu.doingButton = true;
	
	/*
	 * now, let the user choose something:
	 */
again:
	FlushEvents(mDownMask|mUpMask|keyDownMask,0);
	for (;!InBG;)
	{
		if (WNE(mDownMask|keyDownMask|updateMask,&event,REAL_BIG))
		{
			SetPort_(GetWindowPort(GetDialogWindow(dgPtr)));
			if (event.what==updateEvt) MiniMainLoop(&event);
			else if (gTBMenu.menuId)
				break;
			else if (event.what==mouseDown)
			{
				ControlHandle cntl;
				short wPart = FindWindow_(event.where,&theWindow);
				
				if (wPart==inMenuBar) break;
				else if (wPart==inContent)
				{
					Point pt = event.where;
					GlobalToLocal(&pt);
					if (FindControl(pt,theWindow,&cntl)) break;
				}
			}
			else if (event.what==keyDown)
				break;
		}
	}
	SetPort_(GetMyWindowCGrafPtr(Win));
	if (gTBMenu.menuId) goto stupidApplicationMenuCrap;

	if (((event.message&charCodeMask)=='.') && (event.modifiers&cmdKey) ||
			((event.message&charCodeMask)==escChar) &&
		 (((event.message&keyCodeMask)>>8)==escKey)) event.what=nullEvent;

	//Dprintf("\p%d %d.%d",event.what,event.where.h,event.where.v);
	if (event.what==mouseDown)
	{
		switch(FindWindow_(event.where,&theWindow))
		{
			case inContent:
			{
				Point pt = event.where;
				ControlHandle cntl, remove;
				Rect r;
				short junk;
				
				SetPort_(GetWindowPort(GetDialogWindow(dgPtr)));
				GlobalToLocal(&pt);
				if (FindControl(pt,theWindow,&cntl))
				{
					if (!TrackControl(cntl,pt,nil))
						if (!HaveTheDiseaseCalledOSX()) goto again;	// Under Jaguar, this TrackControl
																												// fails the first time, for reasons
																												// I do not understand and do not currently
																												// have the patience to discover.  The user who
																												// pulls the mouse of the button will just
																												// have to suffer  SD 9/4/02
					GetDialogItem(dgPtr,2,&junk,(void*)&remove,&r);
					if (cntl==remove)	//remove button
					{
						SetPort_(GetMyWindowCGrafPtr(Win));
						DeletingButton(&(*BD)[GetControlReference(button)-1]);
						TBDelButton(button);
						Deleted = True;
					}
				}
				break;
			}
				
			case inMenuBar:
			{
				long mSelect;
				short mnu, itm;
				
				SetMenuTexts(event.modifiers,True);
				gTBMenu.doingMenuSelect = true;
				gTBMenu.menuId = gTBMenu.item = 0;
				mSelect = MenuSelect(event.where);
				gTBMenu.doingMenuSelect = false;
				stupidApplicationMenuCrap: if (gTBMenu.menuId)
				{
					// We received an AppleEvent indicating which
					// menu item
				
					mnu = gTBMenu.menuId;
					itm = gTBMenu.item;
					event.modifiers = CurrentModifiers()&~cmdKey;
				}
				else if (PleaseQuit)
				{
					mnu = FILE_MENU;
					itm = FILE_QUIT_ITEM;
					PleaseQuit = false;
				}
				else
				{
					mnu = (mSelect>>16)&0xffff;
					itm = mSelect & 0xffff;
				}
				HiliteMenu(0);
				if (!itm)
				{
					short	menuItem;
					
					//	No item selected. See if it's an IMAP hybrid mailbox. Since
					//	it has a submenu, we have to call MenuChoice.
					mSelect = MenuChoice();
					mnu = (mSelect>>16)&0xffff;
					menuItem = mSelect & 0xffff;
					if (IsMailboxChoice(mnu,menuItem) && mnu!=MAILBOX_MENU && mnu!=TRANSFER_MENU)
					{
						short	vRef;
						long	dirID;
						
						MenuID2VD(mnu,&vRef,&dirID);
						if (IsIMAPVD(vRef,dirID))
							itm = menuItem;	//	IMAP hybrid mailbox
					}
				}
				*spec.name = 0;
				if (mnu&&itm)
				{
					MyGetItem(GetMHandle(mnu),itm,s);
					if (IsMailboxChoice(mnu,itm))
						mnu = TBBoxishMenu(which,&spec,s,mnu,itm);
					if (mnu)
					{
						if (*spec.name)
							InstallButtonMenu(GetControlReference(button),mnu,0,s,event.modifiers,&spec,sizeof(FSSpec));
						else
							InstallButtonMenu(GetControlReference(button),mnu,itm,nil,event.modifiers,nil,0);
						Dirty = True;
						val = True;
					}
				}
				break;
			}
		}
	}
	else if (event.what==keyDown)
	{
		SetPort_(GetMyWindowCGrafPtr(Win));
		InstallButtonKey(GetControlReference(button),event.message,event.modifiers);
		Dirty = True;
		val = True;
	}

	PopModalWindow();
	TBTip(nil);
	DisposeDialog_(dgPtr);
	if (!Deleted) HiliteControl(button,0);
	SFWTC = True;
	Zero(gTBMenu);
	return(val);
}

short TBBoxishMenu(short which,FSSpecPtr spec,PStr s,short mnu,short itm)
{
	if ((mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) && (itm==MAILBOX_NEW_ITEM ||itm==MAILBOX_OTHER_ITEM))
		;
	else if (!(mnu==MAILBOX_MENU || mnu==TRANSFER_MENU) &&
						(itm==TRANSFER_NEW_ITEM-TRANSFER_BAR1_ITEM || itm==TRANSFER_OTHER_ITEM-TRANSFER_BAR1_ITEM))
	{
		Boolean	doWarn = true;
		
		if (itm==TRANSFER_OTHER_ITEM-TRANSFER_BAR1_ITEM)
		{
			//	If IMAP mailbox, this is actually "This Mailbox", not "Other..."
			short	vRefNum;
			long	dirID;
			
			MenuID2VD(mnu,&vRefNum,&dirID);
			if (IsIMAPVD(vRefNum,dirID))
			{
				//	IMAP. It's "This Mailbox"
				mnu = MailboxKindaMenu(mnu,itm,s,spec);
				if (*spec->name) PCopy(s,spec->name);
				doWarn = false;
			}
		}
		if (doWarn)
		{
			mnu = 0;
			WarnUser(CANT_ADD_NESTED,0);
		}
	}
	else
		mnu = MailboxKindaMenu(mnu,itm,s,spec);

	return mnu;
}


/**********************************************************************
 * SizeToolbar - how big should the toobar be?
 **********************************************************************/
void SizeToolbar(short max)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	Str63 s;
	Str31 token;
	UPtr spot;
	long h, v;
	short totalHi, totalWi;
	ControlHandle button;
	short i;
	Boolean big = VarCode==tbvBig||VarCode==tbvBigName;
	Rect screenRect;
	GDHandle junk;
	Boolean hasMB;
	long exPixels, exCount, space;
	
	i = GetPrefLong(PREF_NW_DEV);
	if (i)
	{
		utl_GetIndGD(i,&junk,&screenRect,&hasMB);
		if (hasMB) screenRect.top += GetMBarHeight();
	}
	else
	{
		GetQDGlobalsScreenBitsBounds(&screenRect);
		screenRect.top += GetMBarHeight();
	}
	
	ScreenRect = screenRect;
	
	/*
	 * convenient place to check this
	 */
	if (PrefIsSet(PREF_TB_FKEYS))
		FKeys = True;
	else
		FKeys = False;
		
	if (!max) max = 1;

	/*
	 * anchored in UL corner
	 */
	if (!IsWindowVisible(WinWP)) MoveWindow(WinWP,screenRect.left+1,screenRect.top,False);
	
	/*
	 * make sure we have enough selectors
	 */
	if (!BD)
	{
		button = (void*)NuHandleClear(max*sizeof(ButtonData));
		BD = (void*)button;
	}
	else if ((i=HandleCount(BD))<max)
	{
		SetHandleBig((Handle)BD,max*sizeof(ButtonData));
		if (!MemError())
		{
			LDRef(BD);
			WriteZero(*BD+i,(max-i)*sizeof(ButtonData));
			UL(BD);
		}
	}

	/*
	 * make sure we have enough controls
	 */
	for (i=0;i<max;i++)
	{
		if (!(*BD)[i].cntl)
		{
			button = GetNewControl(TB_CNTL,WinWP);
			if (button)
			{
				SetControlReference(button,i+1);
				TBSetButtonType(button);
			}
			(*BD)[i].cntl = button;
		}
	}
	
	/*
	 * Search control?
	 */
	if (!PrefIsSet(PREF_NO_TB_SEARCH))
	{
		if (!SearchUserPane)
		{
			StackHandle stack;
			
			// make 'em
			button = GetNewControl(TB_SEARCH_USER_CNTL,WinWP);
			SearchUserPane = button;
			button = GetNewControl(TB_SEARCH_GO_CNTL,WinWP);
			SearchGo = button;
			button = GetNewControl(TB_SEARCH_POPUP,WinWP);
			SearchPopup = button;
			button = TBNewSearchEdit(WinWP);
			SearchEdit = button;
			StackInit(sizeof(Str255),&stack);
			if (!(SearchPopup && SearchGo && SearchEdit && stack))
			{
				// if we didn't get'em, kill 'em
				ZapControl(SearchPopup);
				ZapControl(SearchEdit);
				ZapControl(SearchGo);
				ZapControl(SearchUserPane);
				ZapHandle(stack);
			}
			else
			{
				short id = SEARCH_WEB_ICON;
				IconAlignmentType align = kAlignAbsoluteCenter;
				
				// Panes and alignment and other rote crap
				EmbedControl(SearchGo,SearchUserPane);
				SetControlData(SearchGo,0,kControlIconResourceIDTag,sizeof(id),(void*)&id);
				SetControlData(SearchGo,0,kControlIconAlignmentTag,sizeof(align),(void*)&align);				
				EmbedControl(SearchPopup,SearchUserPane);
				TBSearchEmpty();
				SearchStack = stack;
			}
		}
	}
	else if (SearchUserPane)
	{
		ZapControl(SearchUserPane);
		TipCntl = nil;
	}
	
	/*
	 * figure cell sizes based on variation code
	 */
	GetRString(s,ToolbarSizesStrn+VarCode);
	spot = s+1;
	StringToNum(PToken(s,token,&spot,","),&h);
	StringToNum(PToken(s,token,&spot,","),&v);
	exCount = GetRLong(TOOLBAR_EXTRA_COUNT);	if (!exCount) exCount = 1000;
	exPixels = GetRLong(TOOLBAR_EXTRA_PIXELS);
	space = GetRLong(TOOLBAR_SEP_PIXELS);
	Hi = v;
	Wi = h;
	ExPixels = exPixels;
	ExCount = exCount;
	Space = space;
		
	/*
	 * place them
	 */
	N = max;
	TBPlaceControls(&screenRect, &totalWi, &totalHi);
		
	if (!PrefIsSet(PREF_NO_TB_BALL) && !Vertical && (VarCode==tbvBig||VarCode==tbvSmall))
	{
		Tip.bottom = totalHi-1;
	}
	else Tip.bottom = 0;
	Tip.top = Tip.bottom-10;
	
	SizeWindow(WinWP,totalWi,totalHi,False);
	GetPortBounds(GetWindowPort(WinWP),&Win->contR);
	
	InvalContent(Win);
}

/**********************************************************************
 * 
 **********************************************************************/
void GetButtonAlignment(ToolbarVEnum varCode, ControlButtonTextAlignment *alignment, ControlButtonTextPlacement *placement, short *textOffset, ControlButtonGraphicAlignment *gAlignment, Point *gOffset)
{
	*textOffset = 0;
	gOffset->h = gOffset->v = 0;
	
	switch(varCode)
	{
		case tbvBigName:	// big icons & names
			*placement = kControlBevelButtonPlaceBelowGraphic;
			*alignment = kControlBevelButtonAlignTextCenter;
			*gAlignment = kControlBevelButtonAlignTop;
			gOffset->v = 2;
			break;
		case tbvBig:	// big icons
			*placement = kControlBevelButtonPlaceToRightOfGraphic;
			*alignment = kControlBevelButtonAlignTextFlushRight;
			*gAlignment = kControlBevelButtonAlignCenter;
			*textOffset = 500;
			gOffset->v = 2;
			break;
		case tbvSmallName:	// small icons & names
			*placement = kControlBevelButtonPlaceToRightOfGraphic;
			*alignment = kControlBevelButtonAlignTextFlushLeft;
			*gAlignment = kControlBevelButtonAlignLeft;
			gOffset->h = 2;
			break;
		case tbvSmall:	// small icons only
			*placement = kControlBevelButtonPlaceToRightOfGraphic;
			*alignment = kControlBevelButtonAlignTextFlushRight;
			*gAlignment = kControlBevelButtonAlignCenter;
			*textOffset = 500;
			break;
		case tbvName:	// text only
			*placement = kControlBevelButtonPlaceBelowGraphic;
			*alignment = kControlBevelButtonAlignTextCenter;
			*gAlignment = kControlBevelButtonAlignTop;
			break;
	}
}
/**********************************************************************
 * 
 **********************************************************************/
void TBPlaceControls(Rect *screenRect,short *windowWide, short *windowHi)
{
	short i;
	ControlHandle button;
	ControlButtonTextPlacement placement;
	ControlButtonTextAlignment alignment;
	ControlButtonGraphicAlignment gAlignment;
	short textOffset = 0;
	Point gOffset = {0,0};
	short h, v;
	short wi, hi;
	short vFactor = Vertical ? 1 : 0;
	short hFactor = 1-vFactor;
	short maxH = 0;
	short maxV = 0;
	short extraCount = 0;
	short hWrap = RectWi(*screenRect)-GetRLong(TB_H_DESK_MARGIN);
	short vWrap = RectHi(*screenRect)-GetRLong(TB_V_DESK_MARGIN);
	short nextSlots;
	short fKeyHi = 0;
	short fKeyWi = 0;
	short fKeyWiSave;
	short tipHi = 0;
	Rect r;
	short searchPaneHi = 0;
	short searchPaneWi = 0;
	short wraps = 0;
	
	// allow room for search button
	if (SearchUserPane)
	{
		searchPaneWi = ControlWi(SearchUserPane);
		searchPaneHi = ControlHi(SearchUserPane);
		hWrap -= ExPixels + searchPaneWi;
		vWrap -= ExPixels + searchPaneHi;
	}
	
	// allow room for fkey labels
	if (FKeys && PrefIsSet(PREF_SHOW_FKEYS))
	{
		fKeyWi += vFactor*GetRLong(TB_FKEY_LABEL_WIDE);	// yes, vFactor
		fKeyHi += hFactor*10;	// yes, hFactor
	}
	if (!PrefIsSet(PREF_NO_TB_BALL) && !Vertical && (VarCode==tbvBig||VarCode==tbvSmall))
		tipHi = 12;
	fKeyHi = MAX(fKeyHi,tipHi);
	fKeyWiSave = fKeyWi;
	
	// setup alignment stuff
	GetButtonAlignment (VarCode, &alignment, &placement, &textOffset, &gAlignment, &gOffset);

	h = v = ExPixels;

	for (i=0;i<N;i++)
	{
		if (i>14)
		{
			fKeyHi = tipHi;
			fKeyWi = 0;
		}
		
		button=(*BD)[i].cntl;
		
 		SetControlData(button,0,kControlBevelButtonTextAlignTag,sizeof(alignment),(void*)&alignment);
 		SetControlData(button,0,kControlBevelButtonTextPlaceTag,sizeof(placement),(void*)&placement);
 		SetControlData(button,0,kControlBevelButtonTextOffsetTag,sizeof(textOffset),(void*)&textOffset);
 		SetControlData(button,0,kControlBevelButtonGraphicAlignTag,sizeof(gAlignment),(void*)&gAlignment);
 		SetControlData(button,0,kControlBevelButtonGraphicOffsetTag,sizeof(gOffset),(void*)&gOffset);

		wi = Wi*(1 + hFactor*(*BD)[i].slots);
		hi = Hi*(1 + vFactor*(*BD)[i].slots);
		
		MoveMyCntl(button,h,v,wi,hi);
 		
 		maxH = MAX(maxH,h+wi+fKeyWi);
		maxV = MAX(maxV,v+hi+fKeyHi);

 		extraCount++;
 		
		v += vFactor*(hi + Space);
		if (!(extraCount%ExCount)) v += vFactor*ExPixels;
		h += hFactor*(wi + Space);
		if (!(extraCount%ExCount)) h += hFactor*ExPixels;

		
		if (i<N-1)
		{
			nextSlots = i<N-1 ? (*BD)[i+1].slots : 0;
			if (v+vFactor*(1+nextSlots)*Hi>vWrap)
			{
				v = ExPixels;
				h = maxH+ExPixels;
				extraCount = 0;
				wraps++;
			}
			else if (h+hFactor*(1+nextSlots)*Wi>hWrap)
			{
				h = ExPixels;
				v = maxV+ExPixels;
				extraCount = 0;
				wraps++;
			}
		}
	}

	// basic sizing
	if (windowWide)
	{
		*windowWide = maxH+ExPixels;
		if (!Vertical && SearchUserPane) *windowWide += ControlWi(SearchUserPane)+ExPixels;
	}
	if (windowHi)
	{
		*windowHi = maxV+ExPixels;
		if (Vertical && SearchUserPane) *windowHi += ControlHi(SearchUserPane)+ExPixels;
	}
	
	// The search button
	if (SearchUserPane)
	{
		// If the search button is forcing the window higher or wider, make sure the buttons
		// grow to where they need to be
		if (Vertical)
		{
			if (fKeyWiSave+Wi < searchPaneWi && !wraps)
			{
				// we're going to have to make the buttons wider to fill in the space
				Wi = searchPaneWi-fKeyWiSave;
				for (i=0;i<N;i++)
					SizeControl((*BD)[i].cntl,Wi,Hi);
			}
		}
		
		hi = ControlHi(SearchUserPane);
		if (Vertical)
		{
			if (*windowWide) *windowWide = MAX(*windowWide,ControlWi(SearchUserPane)+2*ExPixels);
			MoveMyCntl(SearchUserPane,ExPixels,maxV+ExPixels,0,hi);
		}
		else
		{
			if (*windowHi) *windowHi = MAX(*windowHi,ControlHi(SearchUserPane)+2*ExPixels);
			MoveMyCntl(SearchUserPane,maxH+ExPixels,ExPixels+(Hi-hi)/2,0,hi);
		}
		// Now, move the embedded controls to the right places
		GetControlBounds(SearchUserPane,&r);
		MoveMyCntl(SearchPopup,r.left+1,r.top+(RectHi(r)-ControlHi(SearchPopup))/2,0,0);
		MoveMyCntl(SearchGo,r.right-ControlWi(SearchGo),r.top+(RectHi(r)-ControlHi(SearchGo))/2,0,0);
		MoveMyCntl(SearchEdit,r.left+MAX_APPEAR_RIM+ControlWi(SearchPopup)+1,r.top+(RectHi(r)-ControlHi(SearchGo))/2,
							 ControlWi(SearchUserPane)-2-2*MAX_APPEAR_RIM-ControlWi(SearchPopup)-ControlWi(SearchGo),
							 RectHi(r)-2*MAX_APPEAR_RIM);
		GetControlBounds(SearchEdit,&r);
		PeteDidResize(SearchPTE,&r);
	}
}

/************************************************************************
 * AssignCmdKey - (re)assign command keys
 ************************************************************************/
void AssignCmdKey(short modifiers)
{
	
}

/************************************************************************
 * ChangeCmdKeys - go through the cmd key string and change cmd keys
 ************************************************************************/
void ChangeCmdKeys(void)
{
	Str255 s;
	UPtr spot;
	Str31 token;
	long num;
	EventRecord event;
	MenuHandle mh;
	short newMods;
	Str31 menuStr;
	Str31 itemStr;
	short menu;
	short item;
	short i;
	
	if (*GetRString(s,CmdKeyStrn+1))
	{
		Zero(event);
		event.what = keyDown;
		EnableMenuItems(true);
		
		for (i=1;;i++)
		{
			// grab the string
			GetRString(s,CmdKeyStrn+i);
			if (!*s) break;
			spot = s+1;
			// first word is modifiers
			PToken(s,token,&spot," ");
			StringToNum(token,&num);
			event.modifiers = num;
			// next word is message
			PToken(s,token,&spot," ");
			StringToNum(token,&num);
			event.message = num;
			// next word are new modifiers
			PToken(s,token,&spot," ");
			StringToNum(token,&num);
			newMods = num;
			// next the menu name
			PToken(s,menuStr,&spot,":");
			menu = FindMenuByName(menuStr);
			// and the item name
			PToken(s,itemStr,&spot,"\377");
			if (menu) item = FindItemByName(GetMHandle(menu),itemStr);
			
			if (menu && item && event.message)
			{
				// is it assigned now?
				if ((num=MyMenuKeyLo(&event,false))&0xffff0000)
					if (mh=GetMHandle((num>>16)&0xffff))
						SetItemCmd(mh,num&0xffff,0);
				
				// and assign
				mh = GetMHandle(menu);
				SetItemCmd(mh,item,UnadornMessage(&event));
				SetMenuItemModifiers(mh,item,
					(event.modifiers&cmdKey ? 0:kMenuNoCommandModifier)|
					(event.modifiers&optionKey ? kMenuOptionModifier:0)|
					(event.modifiers&shiftKey ? kMenuShiftModifier:0)|
					(event.modifiers&controlKey ? kMenuControlModifier:0));
			}
		}
		EnableMenuItems(false);
		HiliteMenu(0);
	}
}


/************************************************************************
 * TBNewSearchEdit - create the search button & edit region
 ************************************************************************/
ControlHandle TBNewSearchEdit(WindowPtr winWP)
{
	MyWindowPtr win=GetWindowMyWindowPtr(winWP);
	GrafPtr oldPort;
	OSErr err;
	PETEDocInitInfo pi;
	PETEHandle pte;
	ControlHandle cntl = nil;
	DECLARE_UPP(PetePaneDraw,ControlUserPaneDraw);
	
	GetPort(&oldPort);
	SetPort_(GetMyWindowCGrafPtr(win));
	
	DefaultPII(win,False,0,&pi);
	SetRect(&pi.inRect,0,0,20,20);
	pi.docWidth = 80;
	
	if (!(err=PeteCreate(win,&pte,peClearAllReturns|peNoStyledPaste,&pi)))
	{
		PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->frame = true;
		PeteFontAndSize(pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		(*PeteExtra(pte))->infinitelyWide = True;
		(*PeteExtra(pte))->dragPreProcess = TBDragPreProcess;
		(*PeteExtra(pte))->dragPostProcess = TBDragPostProcess;
		SearchPTE = pte;
		
		if (cntl=NewControl(winWP,&pi.inRect,"",True,0,0,0,kControlUserPaneProc,(uLong)pte))
		{
			INIT_UPP(PetePaneDraw,ControlUserPaneDraw);
			SetControlData(cntl,0,kControlUserPaneDrawProcTag,sizeof(PetePaneDrawUPP),(void*)&PetePaneDrawUPP);
		}
	}
	CleanPII(&pi);
	if (err) WarnUser(PETE_ERR,err);
	
	SetPort_(oldPort);
	return(cntl);
}

/************************************************************************
 * DeletingButton - process deleting ad button
 ************************************************************************/
void DeletingButton(BDPtr pBD)
{
	if (pBD->kind == tbkAd)
	{
		AdId	adId;
		
		adId.ad = pBD->select;
		adId.server = pBD->modifiers;
		AdDeleteButton(adId);	
	}
}


void MessageDragPopup (ControlHandle button, Boolean vertical)
{
	CGrafPtr	oldPort;
	Rect			buttonRect;
	Point			offsetPt;

	GetControlBounds(button,&buttonRect);
	
	// Put the button rectangle into global coordinates
	offsetPt.h = offsetPt.v = 0;
	LocalToGlobal (&offsetPt);
	OffsetRect (&buttonRect, offsetPt.h, offsetPt.v);

	GetPort (&oldPort);
	if (OpenToolbarPopup (&buttonRect, VarCode, 2, vertical)) {
		AddMenuToToolbarPopup (0, MESSAGE_MENU, MESSAGE_FORWARD_ITEM,(*BD)[GetControlReference(button)-1].title);
		AddMenuToToolbarPopup (1, MESSAGE_MENU, MESSAGE_REDISTRIBUTE_ITEM, (*BD)[GetControlReference(button)-1].title);
	}
	SetPort (oldPort);
}

/************************************************************************
 * ConfiguringToolbar - do indicated menu item if configuring the toolbar
 ************************************************************************/
Boolean ConfiguringToolbarMenuItem(short menuId,short item)
{
	if (gTBMenu.doingButton)
	{
		gTBMenu.menuId = menuId;
		gTBMenu.item = item;
		return true;
	}
	return false;
}

/************************************************************************
 * TBSearchStart - start a search from the mini-dialog
 ************************************************************************/
void TBSearchStart(PStr searchMe)
{
	Str255 searchFor;
	
	if (!searchMe)
	{
		searchMe = searchFor;
		if (Win->pte)
		{
			PeteSString(searchFor,SearchPTE);
			if (IsAllLWSP(searchFor)) *searchFor = 0;
		}
		else *searchFor = 0;
	}
	
	// Add to recent searches
	if (0>StackStringFind(searchMe,SearchStack))
	{
		if ((*SearchStack)->elCount>=GetRLong(RECENT_SEARCH_LIMIT))
			StackPop(nil,SearchStack);
		StackQueue(searchMe,SearchStack);
	}
	
	// Now go do it
	if (SearchItem>1)
	{
		short findItem = FIND_SEARCH_ITEM;
		MyWindowPtr win = nil;		
		
		switch (SearchItem)
		{
			case 2: findItem=FIND_SEARCH_ITEM; break;
			case 3: findItem=FIND_SEARCH_ALL_ITEM; break;
			case 4: findItem=FIND_SEARCH_BOX_ITEM; break;
			case 5: findItem=FIND_SEARCH_FOLDER_ITEM; break;
			default: ASSERT(0);
		}
		
		win = SearchOpen(findItem);
		
		if (win)
		{
			SearchNewFindStringLo(searchMe,true);
			StartSearch(win);
		}
	}
	else
		DoWebFindStr(searchMe);

	TBSearchFocus(false);
}

/************************************************************************
 * TBSearchEmpty - empty the search field and put in fake text
 ************************************************************************/
void TBSearchEmpty(void)
{
	Str255 s;
	PETETextStyle ps;
	
	switch (SearchItem)
	{
		case 0:
		case 1:
			ComposeRString(s,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB);
			break;
		case 2:
			ComposeRString(s,SEARCH_NOTHING_FMT,SEARCH_FOR_EMAIL);
			break;
		case 3:
		case 4:
		case 5:
			MyGetItem(GetMHandle(FIND_HIER_MENU),FIND_SEARCH_ALL_ITEM-3+SearchItem,s);
			break;
		default:
			ASSERT(0);
			*s = 0;
			break;
	}
	
	PeteSetString(s,SearchPTE);
	GetRColor(&ps.tsColor,FAKE_CONTENT_COLOR);
	PETESetTextStyle(PETE,SearchPTE,0,0xffff,&ps,peColorValid);
}


/************************************************************************
 * TBSearchPopup - let the user choose to make us money or be productive
 ************************************************************************/
void TBSearchPopup(Point pt)
{
	MenuHandle mh = NewMenu(CONTEXT_MENU,"");
	Str255 s;
	long sel;
	short mnu, itm;
	
	// Fill the menu with recent searches
	if ((*SearchStack)->elCount)
	{
		for (itm=0;itm<(*SearchStack)->elCount;itm++)
		{
			StackItem(s,itm,SearchStack);
			MyAppendMenu(mh,s);
		}
		AppendMenu(mh,"\p-");
	}
	
	if (!SearchItem) SearchItem = 1;
		
	// The search-where items
	AppendMenu(mh,ComposeRString(s,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB));
	AppendMenu(mh,ComposeRString(s,SEARCH_NOTHING_FMT,SEARCH_FOR_EMAIL));
	CopyMenuItem(GetMHandle(FIND_HIER_MENU),FIND_SEARCH_ALL_ITEM,mh,REAL_BIG);
	CopyMenuItem(GetMHandle(FIND_HIER_MENU),FIND_SEARCH_BOX_ITEM,mh,REAL_BIG);
	CopyMenuItem(GetMHandle(FIND_HIER_MENU),FIND_SEARCH_FOLDER_ITEM,mh,REAL_BIG);
	SetItemMark(mh,CountMenuItems(mh)-SearchItem+5,diamondChar);
	LocalToGlobal(&pt);

	sel = AFPopUpMenuSelect(mh,pt.v,pt.h,-1);
	mnu = (sel>>16)&0xffff;
	itm = sel&0xffff;
	
	// Did the user choose anything valid?
	if (mnu&&itm)
	{
		// focus in the search window
		TBSearchFocus(true);
		
		if ((*SearchStack)->elCount && itm<=(*SearchStack)->elCount)
		{
			// recent item
			StackItem(s,itm-1,SearchStack);
			PeteSetString(s,SearchPTE);
		}
		else // search where?
		{
			SearchItem = itm - CountMenuItems(mh) + 5;
			if (*PeteString(s,SearchPTE))
				TBSearchStart(s);
		}
	}
}

/************************************************************************
 * TBSearchFocus - focus or unfocus on the search field
 ************************************************************************/
void TBSearchFocus(Boolean focus)
{
	if (SearchPTE)
	{
		if (focus)
		{
			if (!Win->pte)
			{
				PeteSetTextPtr(SearchPTE,nil,0);
				PetePlain(SearchPTE,0,0xffff,peAllValid);
				PetePlain(SearchPTE,kPETECurrentStyle,kPETECurrentStyle,peAllValid);
				PeteFocus(Win,SearchPTE,true);
				PeteActiveLock(SearchPTE);
			}
		}
		else if (Win->pte)
		{
			PeteFocus(Win,nil,true);
			PeteActiveLock(nil);
			TBSearchEmpty();
		}
	}
}

/************************************************************************
 * TBDragPreProcess - clear out fake text on drag
 ************************************************************************/
void TBDragPreProcess(PETEHandle pte,DragTrackingMessage message,DragReference drag)
{
	if (message==kDragTrackingEnterWindow)
		TBSearchFocus(true);
}

/************************************************************************
 * TBDragPostProcess - launch search after drop
 ************************************************************************/
void TBDragPostProcess(PETEHandle pte,DragTrackingMessage message,DragReference drag)
{
	if (message==kDragTrackingLeaveWindow && !PeteLen(SearchPTE))
		TBSearchFocus(false);
	else if (message==0x0fff) 
	{
		Str255 s;
		PeteString(s,SearchPTE);
		if (!IsAllLWSP(s)) TBSearchStart(s);
		TBSearchEmpty();
	}
}
