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

#include "find.h"
#define FILE_NUM 14
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Find

/************************************************************************
 * structure to keep track of what we're finding
 ************************************************************************/
typedef enum
{
	fcFind,
	fcWord,
//fcBackward,
	fcCase,
	fcFindTxt,
	fcOptionsTxt,
	fcLimit
} FindControlEnum;

typedef struct {
	Str63 what;									/* the string we're finding */
	MyWindowPtr win; 							/* our window, if any */
	Boolean findDone;							/* are we done? */
	short kind;										/* which kind of find? */
	ControlHandle controls[fcLimit];	/* our controls */
	PETEHandle queryPTE;					// te box for the string we're looking for
	Rect qRect;										// rectangle for query
	Boolean finding;							// we're atively finding
	Str255 whereStr;							// description of where we are
} FindVars, *FindPtr, **FindHandle;
FindHandle FG;

#define WhereMark	(*FG)->whereMark
#define WhereStr (*FG)->whereStr
#define Controls (*FG)->controls
#define QRect (*FG)->qRect
#define What (*FG)->what
#define Win (*FG)->win
#define FCurrMarker (*FG)->curr
#define FindDone (*FG)->findDone
#define FKind	(*FG)->kind
#define Finding (*FG)->finding
#define QueryPTE (*FG)->queryPTE
#define Points (*FG)->points

/************************************************************************
 * private functions
 ************************************************************************/
void CloseFindBox(void);
void FindOpen(void);
short InitFind(void);
Boolean FindClose(MyWindowPtr win);
void DoFindOK(void);
Boolean DoMessFind(MyWindowPtr win,PETEHandle pte,Boolean allowWrap);
void ReportFindFailure(void);
void FindCursor(Point mouse);
long FindByteOffset(UPtr sub,UPtr buffer);
void FindBGClick(MyWindowPtr win, EventRecord *event);
void FindButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
void FindSetStop(short where);
OSErr FindCreateControls(void);
void FindDidResize(MyWindowPtr win,Rect *oldContR);
void FindDisplayCurrent(void);
void FindIdle(MyWindowPtr win);
void FindGreyButtons(void);
Boolean FindKey(MyWindowPtr win, EventRecord *event);
void FindHelp(MyWindowPtr win,Point mouse);
static Boolean FindInCollapsed(VLNodeInfo *info,MyWindowPtr win,ViewListPtr pView,PStr what);
static void DoEnterSelection(void);
pascal OSErr FindStrChanged(PETEHandle pte,long start,long stop);
void DoWebFind(void);

#pragma segment Menu
/************************************************************************
 * EnableFindMenu - do the enabling for the find menu
 ************************************************************************/
void EnableFindMenu(Boolean all)
{
	WindowPtr	winWP = FindTopUserWindow();
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	MenuHandle mh = GetMHandle(FIND_HIER_MENU);
	short	winKind = winWP ? GetWindowKind(winWP) : 0;
	Boolean	hasMB = winKind==COMP_WIN || winKind==MESS_WIN || winKind==MBOX_WIN || winKind==CBOX_WIN;
	Boolean	exchangeFindSearch;

	if (all || !ModalWindow)
	{
		EnableItem(GetMHandle(SPECIAL_MENU),AdjustSpecialMenuItem(SPECIAL_FIND_ITEM));
		EnableItem(mh,0);
		EnableIf(mh,FIND_ENTER_ITEM,all||(win&&win->hasSelection));
		EnableIf(mh,FIND_AGAIN_ITEM,all||FG && *What);
		EnableIf(mh,FIND_SEARCH_BOX_ITEM,all||hasMB);
		EnableIf(mh,FIND_SEARCH_FOLDER_ITEM,all||hasMB);
		EnableIf(mh,FIND_SEARCH_WEB_ITEM,true);
	}
	else
	{
		DisableItem(GetMHandle(SPECIAL_MENU),AdjustSpecialMenuItem(SPECIAL_FIND_ITEM));
		DisableItem(mh,0);
	}
	
	exchangeFindSearch = PrefIsSet(PREF_EXCHANGE_FIND_SEARCH);
	SetMenuItemModifiers(mh,FIND_FIND_ITEM,exchangeFindSearch ? kMenuOptionModifier : 0);
	SetMenuItemModifiers(mh,FIND_SEARCH_ITEM,exchangeFindSearch ? 0 : kMenuOptionModifier);
}


/************************************************************************
 * FindTopUserWindow - find the topmost (non-find) user window
 ************************************************************************/
WindowPtr FindTopUserWindow(void)
{
	WindowPtr winWP;
	
	for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow (winWP))
		if (IsWindowVisible(winWP) && GetWindowKind(winWP)>userKind && GetWindowKind(winWP)!=FIND_WIN
				&& winWP!=ModalWindow && !IsFloating(winWP))
			return(winWP);
	return(nil);
}

#pragma segment Main


#pragma segment Find

/************************************************************************
 * DoEnterSelection - enter find selection from top window
 ************************************************************************/
static void DoEnterSelection(void)
{
	WindowPtr	winWP = FindTopUserWindow();
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	Str255 string;
	
	if (win && win->pte && win->hasSelection && SetFindString(string,win->pte))
		FindEnterSelection(string,true);
}

/************************************************************************
 * DoFind - handle the user choosing find
 ************************************************************************/
void DoFind(short item,short modifiers)
{
	if (InitFind()) return;
	
	if (item > FIND_MENU_LIMIT)
	{
		OpenSearchMenu(item);
		return;
	}
	
	switch (item)
	{
		case FIND_FIND_ITEM:
			if (modifiers & shiftKey)
				DoEnterSelection();
			FindOpen();
			return;
		case FIND_ENTER_ITEM:
			DoEnterSelection();
			return;
		case FIND_SEARCH_ITEM:
		case FIND_SEARCH_ALL_ITEM:
		case FIND_SEARCH_BOX_ITEM:
		case FIND_SEARCH_FOLDER_ITEM:
			if (modifiers & shiftKey)
				DoEnterSelection();
			SearchOpen(item);
			return;
		case FIND_SEARCH_WEB_ITEM:
			DoWebFind();
			return;
	}
	DoFindOK();
}

/************************************************************************
 * DoWebFindWarning - warn the user about what we're going to do
 ************************************************************************/
Boolean DoWebFindWarning(short menu,short item)
{
	if (menu==FIND_HIER_MENU && item==FIND_SEARCH_WEB_ITEM && !GetPrefBit(PREF_SEARCH_WEB_BITS,prefSearchWebSelectWarning))
	{
		WindowPtr	winWP = FindTopUserWindow();
		MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
		Str255 s;
		
		*s = 0;
		if (win && win->pte)
		{
			PeteSelectedString(s,win->pte);
			*s = MIN(*s,127);
		}
		
		if (*s)
		{
			short btn = ComposeStdAlert(Note,SEARCH_TEXT_WARNING);
			if (btn==kAlertStdAlertOtherButton) SetPrefBit(PREF_SEARCH_WEB_BITS,prefSearchWebSelectWarning);
			return kAlertStdAlertCancelButton!=btn;
		}
	}
	return true;
}

/************************************************************************
 * DoWebFind - search for something on the web, using the topmost window as input
 ************************************************************************/
void DoWebFind(void)
{
	WindowPtr	winWP = FindTopUserWindow();
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	Str255 s;
	
	*s = 0;
	if (win && win->pte)
	{
		PeteSelectedString(s,win->pte);
		CollapseLWSP(s);
		*s = MIN(*s,127);
	}
	
	DoWebFindStr(s);
}

/************************************************************************
 * DoWebFindStr - search for a string on the web
 ************************************************************************/
void DoWebFindStr(PStr s)
{
	OpenAdwareURL(GetNagState(),SEARCH_SITE,actionSearch,searchQuery,(long)s);
}


/**********************************************************************
 * FindStrChanged - the find string has changed
 **********************************************************************/
pascal OSErr FindStrChanged(PETEHandle pte,long start,long stop)
{
	Str63 what;
	
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(Win));
	PeteSString(what,pte);
	PSCopy(What,what);
	PopGWorld();
	return noErr;
}

/************************************************************************
 * FindOpen - open the find window
 ************************************************************************/
void FindOpen(void)
{
	WindowPtr	WinWP;
	Str255 what;
	OSErr err;
	
	if (InitFind()) return;
	if (!Win)
	{
		MyWindowPtr win;
		PETEHandle pte;
		win = GetNewMyWindow(FIND_WIND,nil,nil,BehindModal,False,False,FIND_WIN);
		if (!win)
			{WarnUser(MEM_ERR,MemError()); return;}
		if (!(err=PeteCreate(win,&pte,0,nil)))
		{
			PETESetCallback(PETE,pte,(void*)FindStrChanged,peDocChanged);
			SetPort(GetMyWindowCGrafPtr(win));
			ConfigFontSetup(win);
			MySetThemeWindowBackground(win,kThemeActiveModelessDialogBackgroundBrush,False);
			PeteFontAndSize(pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
			Win = win;
			QueryPTE = Win->pte = pte;
			(*PeteExtra(pte))->frame = True;
			if (!(err=FindCreateControls()))
				FindDidResize(Win,nil);
		}
		
		if (err)
		{
			CloseMyWindow(GetMyWindowWindowPtr(win));
			Win = nil;
			WarnUser(MEM_ERR,MemError());
			return;
		}
		Win->close = FindClose;
		Win->position = PositionPrefsTitle;
		Win->bgClick = FindBGClick;
		Win->button = FindButton;
		Win->isRunt = True;
		Win->idle = FindIdle;
		Win->key = FindKey;
		Win->help = FindHelp;
		Win->idleInterval = 30;
	}
	PCopy(what,What);

	PeteSetString(what,QueryPTE);
	SetControlValue(Controls[fcCase],PrefIsSet(PREF_SENSITIVE)?1:0);
	SetControlValue(Controls[fcWord],PrefIsSet(PREF_FIND_WORD)?1:0);
//	SetControlValue(Controls[fcBackward],PrefIsSet(PREF_FIND_BACK)?1:0);

	WinWP = GetMyWindowWindowPtr (Win);
	if (!IsWindowVisible (WinWP))
	{
		if (Get1NamedResource(SAVE_POS_TYPE,GetRString(what,FIND_FIND)))
			ShowMyWindow(WinWP);
		else
			ShowWindow(WinWP);
	}
	UserSelectWindow(WinWP);
	PeteSelect(Win,QueryPTE,0,REAL_BIG);
	SFWTC = True;
}

/**********************************************************************
 * FindHelp - balloon help for find
 **********************************************************************/
void FindHelp(MyWindowPtr win,Point mouse)
{
	short i;
	Rect r;
	
	for (i=0;i<fcLimit;i++)
	{
		GetControlBounds(Controls[i],&r);
		if (PtInRect(mouse,&r))
		{
			MyBalloon(&r,80,0,FindHelpStrn+i+1,0,nil);
			return;
		}
	}
	
	if (PtInPETE(mouse,Win->pte))
	{
		r = QRect;
		MyBalloon(&r,80,0,FindHelpStrn+fcLimit+1,0,nil);
	}
}

/**********************************************************************
 * FindKey - act on a find keystroke
 **********************************************************************/
Boolean FindKey(MyWindowPtr win, EventRecord *event)
{
	if (event->modifiers & cmdKey) {return(False);}				/* no command keys! */
	if ((event->message & charCodeMask)==returnChar ||
			(event->message & charCodeMask)==enterChar)
		FindButton(Win,Controls[fcFind],0,1);
	else PeteEdit(win,win->pte,peeEvent,event);
	return(True);
}

/**********************************************************************
 * FindCreateControls - create the controls for the find window
 **********************************************************************/
OSErr FindCreateControls(void)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	FindControlEnum i;
	ControlHandle cntl;
	Rect r;
	Str31 s;
	static short ids[] = {
		FIND_FIND,		 						  kControlUsesOwningWindowsFontVariant,
		FIND_WORD,		 kControlCheckBoxProc	| kControlUsesOwningWindowsFontVariant,
//		FIND_BACKWARD,	 kControlCheckBoxProc	| kControlUsesOwningWindowsFontVariant,
		FIND_CASE,		 kControlCheckBoxProc	| kControlUsesOwningWindowsFontVariant,
		FIND_FIND_LABEL, kControlStaticTextProc	| kControlUsesOwningWindowsFontVariant,
		FIND_OPTIONS, 	 kControlStaticTextProc	| kControlUsesOwningWindowsFontVariant,
	};
	
	
	for (i=0;i<fcLimit;i++)
	{
		SetRect(&r,-60,-60,-20,-20);
		cntl = NewControlSmall(WinWP,&r,GetRString(s,ids[2*i]),
											True,0,0,1,ids[2*i+1],i);
		if (!cntl) break;
		Controls[i] = cntl;
	}

	OutlineControl(Controls[fcFind],true);
	SetTextControlText(Controls[fcFindTxt],GetRString(s,FIND_FIND_LABEL),nil);
	SetTextControlText(Controls[fcOptionsTxt],GetRString(s,FIND_OPTIONS),nil);
	return(cntl ? noErr : -108);
}

/**********************************************************************
 * FindDidResize - Resize the find window
 **********************************************************************/
void FindDidResize(MyWindowPtr win,Rect *oldContR)
{
	Rect r;
	short hi,left,top,i;
	short notSoHi;
	short	wiWin = RectWi(Win->contR);
	Str255	s;
	
	hi = Win->vPitch+4;
	notSoHi = hi-2;
	
	for (i=0;i<fcLimit;i++)
	{
		if (i==fcFindTxt || i==fcOptionsTxt)
		{
			//	ButtonFit doesn't work with static text controls. Do it ourself.
			SizeControl(Controls[i],
				StringWidth(GetRString(s,i==fcFindTxt ? FIND_FIND_LABEL : FIND_OPTIONS))+win->hPitch,
				win->vPitch + 4);
		}
		else
			ButtonFit(Controls[i]);
	}

	//	Top row
	top = 2*INSET;
	MoveMyCntl(Controls[fcFindTxt],INSET,top,0,0);
	GetControlBounds(Controls[fcFind],&r);
	left = wiWin-INSET-RectWi(r);
	MoveMyCntl(Controls[fcFind],left,top-3,0,0);
	GetControlBounds(Controls[fcFindTxt],&r);
	SetRect(&r,r.right + INSET,top,left-INSET*2,top+ONE_LINE_HI(win));
	PeteDidResize(QueryPTE,&r);
	QRect = r;
	
	//	2nd row
	top = r.bottom+INSET*2;
	MoveMyCntl(Controls[fcOptionsTxt],INSET,top+3,0,0);
	GetControlBounds(Controls[fcOptionsTxt],&r);
	MoveMyCntl(Controls[fcWord],r.right+INSET,top,0,0);
	GetControlBounds(Controls[fcWord],&r);
	MoveMyCntl(Controls[fcCase],r.right+2*INSET,top,0,0);

//	MoveMyCntl(Controls[fcBackward],left,top+2*GROW_SIZE+2*INSET,0,0);
}

#pragma segment Find

/**********************************************************************
 * 
 **********************************************************************/
void FindButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	FindControlEnum item;
	
	SetMyCursor(watchCursor); SFWTC = True;
	
	for (item=0;item<fcLimit;item++)
	{
		if (Controls[item]==button)
		{
			switch(item)
			{
				case fcFind:
					DoFindOK();
					break;
		
				case fcCase:
					SetControlValue(button,TogglePref(PREF_SENSITIVE)?1:0);
					break;
		
//				case fcBackward:
//					SetControlValue(button,TogglePref(PREF_FIND_BACK)?1:0);
//					break;
		
				case fcWord:
					SetControlValue(button,TogglePref(PREF_FIND_WORD)?1:0);
					break;
			}
			break;
		}
	}
	
	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP), item), mouseDown);	
}

/**********************************************************************
 * 
 **********************************************************************/
void FindBGClick(MyWindowPtr win, EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point pt = event->where;

	SetPort(GetWindowPort(winWP));
	GlobalToLocal(&pt);
	if (!HandleControl(pt,win)) SelectWindow_(winWP);
}

/************************************************************************
 * DoFindOK - continue with our find
 ************************************************************************/
void DoFindOK(void)
{
	WindowPtr	winWP = FindTopUserWindow();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	Str255 s;
	
	if (!FG) return;
	
	if (Win)
	{
		PeteSString(s,QueryPTE);
		PSCopy(What,s);
	}
	
	if (!*What || !win) return; 									/* nothing to find */
	FindDone = False;		
	if (PrefIsSet(PREF_SENSITIVE)) Sensitive = True; else Sensitive = False;
	if (PrefIsSet(PREF_FIND_WORD)) WholeWord = True; else WholeWord = False;

	if (win->find)
	{
		CycleBalls();	
		FKind = GetWindowKind (winWP);
		PSCopy(s,What);
		if (!(*win->find)(win,s))
			ReportFindFailure();
	}
}

/************************************************************************
 * FindInCollapsed - if it's a collapsed list item, search inside
 ************************************************************************/
static Boolean FindInCollapsed(VLNodeInfo *info,MyWindowPtr win,ViewListPtr pView,PStr what)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);

	if (info->isParent && info->isCollapsed)
		if (GetWindowKind (winWP)==MB_WIN || IsSearchWindow(winWP))
			return MBFindInCollapsed(win,pView,what,MBGetFolderMenuID(info->nodeID,info->name));
	return false;
}

/************************************************************************
 * FindListView - find in a list
 ************************************************************************/
Boolean FindListView(MyWindowPtr win,ViewListPtr pView,PStr what)
{
	short	row,startRow;
	ListHandle	hList = pView->hList;
	Boolean	wrapped = false;
	VLNodeInfo info;
	Boolean	found = false;
	
	if (pView->selectCount)
	{
		startRow = (*pView->hSelectList)[pView->selectCount-1].row+1;
		if (LVGetItem(pView,startRow,&info,false) && FindInCollapsed(&info,win,pView,what))
			//	Started with a collapsed item which contained the string
			found = true;
	}
	else
		startRow = 0;	

	for(row = startRow;!found && (row != startRow || !wrapped);row++)
	{
		if (row>=(*hList)->dataBounds.bottom)
		{
			//	start from the top again
			row = 0;
			wrapped = true;
			if (!startRow) break;
		}
		
		if (LVGetItem(pView,row+1,&info,false))
		{		
			if (FindStrStr(what,info.name)>=0 && LVSelect(pView,info.nodeID,info.name,false))
				found = true;
			else
				found = FindInCollapsed(&info,win,pView,what);
		}
	}

	if (found)
	{
		WindowPtr	winWP = GetMyWindowWindowPtr (win);
		if (!IsWindowVisible (winWP))
			ShowMyWindow(winWP);
		SelectWindow_(winWP);
		UpdateMyWindow(winWP);
		SFWTC=True;
		return true;
	}
	
	return false;
}

/************************************************************************
 * FindInPTE - find in pete handle
 ************************************************************************/
Boolean FindInPTE(MyWindowPtr win,PETEHandle pte,PStr what)
{
	long	startHere;
	long offset;

	PeteGetTextAndSelection(pte,nil,nil,&startHere);

	offset = PeteFindString(what,startHere,pte);
	if (offset < 0 && startHere)
		//	Not found. Wrap around and search again
		offset = PeteFindString(what,0,pte);
	
	if (offset>=0)
	{
		WindowPtr	winWP = GetMyWindowWindowPtr (win);
		if (!IsWindowVisible (winWP))
			ShowMyWindow(winWP);
		UserSelectWindow(winWP);
		UpdateMyWindow(winWP);
		if (GetWindowKind(winWP)==MESS_WIN) MessFocus(Win2MessH(win),(*Win2MessH(win))->bodyPTE);
		else  PeteFocus(win,pte,True);
		
		if (AttIsSelected(win,pte,offset,offset+*what,0,nil,nil))
		{
			// we found part of the attachment name; we need
			// to select the attachment itself
			PETESelectGraphic(PETE,pte,offset);
		}
		else
			PeteSelect(nil,win->pte,offset,offset+*what);	// text selection instead

		PeteScroll(win->pte,pseNoScroll,pseCenterSelection);
		win->hasSelection = True;
		SFWTC=True;
		return(True);
	}
	else
		return(False);
}

/************************************************************************
 * SetFindString - set string used for find and search
 ************************************************************************/
Boolean SetFindString(PStr what, PETEHandle pte)
{
	//	Don't use text if too long or if contains a return
	long	selStart;
	long	selEnd;
	Handle	textH;
	
	if (!PeteGetTextAndSelection(pte,&textH,&selStart,&selEnd) && 
		selEnd-selStart < GetRLong(MAX_FIND_SELECTION) && 
		!memchr(*textH+selStart,'\r',selEnd-selStart))
	{
		PeteSelectedString(what,pte);
		return true;
	}
	else
	{
		*what = 0;
		return false;
	}
}

/************************************************************************
 * FindEnterSelection - enter the selection from the current txe in the
 * find window
 ************************************************************************/
void FindEnterSelection(PStr what,Boolean searchToo)
{
	if (!FG) InitFind();
	if (!FG) return;
	PSCopy(What,what);
	if (Win) PeteSetString(what,QueryPTE);
	if (searchToo)
		SearchNewFindString(what);
}

/************************************************************************
 * FindGetWhat - get what we're finding
 ************************************************************************/
Boolean GetFindString(PStr what)
{
	if (FG)
	{
		PCopy(what,What);
		return true;	
	}
	else
	{
		*what = 0;
		return false;
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void FindIdle(MyWindowPtr win)
{
	FindGreyButtons();
}

/**********************************************************************
 * 
 **********************************************************************/
void FindGreyButtons(void)
{
	WindowPtr	winWP = FindTopUserWindow();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
		
	if (InBG || !Win->isActive) return;
	
	ActivateMyControl(Controls[fcFind],*What && win && win->find);
}

/************************************************************************
 * InitFind - get the find stuff ready to go
 ************************************************************************/
short InitFind(void)
{
	if (FG) return(0);
	if ((FG=NewZH(FindVars))==nil) return(WarnUser(MEM_ERR,MemError()));
	return(0);
}

/************************************************************************
 * FindClose - close the find window
 ************************************************************************/
Boolean FindClose(MyWindowPtr win)
{
#pragma unused(win)
	Win=nil;
	return(True);
}

/************************************************************************
 * FindSub - find a substring in some text. 
 * Brute force.
 ************************************************************************/
long FindSub(UPtr sub,UHandle	text,long offset)
{
	long size = GetHandleSize_(text);
	long result = 0;
	
	if (PtrPlusHand_(sub+1,text,*sub+1))
		return(size);
	sub[*sub+1] = (*text)[size+*sub] = 0;
	result = FindByteOffset(sub+1,LDRef(text)+offset);
	UL(text);
	SetHandleBig_(text,size);
	return(result+offset);
}

/************************************************************************
 * FindByteOffset - find a substring in some text.
 * Brute force.  Sub *must* be in buffer!!!!!  Each string should be NULL
 * terminated.
 ************************************************************************/
long FindByteOffset(UPtr sub,UPtr buffer)
{
	UPtr anchor, spot, subSpot;
	long result;
	
	if (Sensitive)
	{
		for (anchor = buffer;;anchor++)
		{
			for (spot=anchor,subSpot=sub;
					 *spot==*subSpot && *subSpot;
					 spot++,subSpot++);
			if (!*subSpot)
			{
				result = anchor - buffer;
				break;
			}
		}
	}
	else
	{
		for (anchor = buffer;;anchor++)
		{
			if (!striscmp(sub,anchor))
			{
				result = anchor - buffer;
				break;
			}
		}
	}
	return(result);
}

/************************************************************************
 * ReportFindFailure - just like it says
 ************************************************************************/
void ReportFindFailure(void)
{
	Str255 what;
	if (CommandPeriod) return;
	PCopy(what,What);
	if (PrefIsSet(PREF_NO_NOT_FOUND_ALERT))
		SysBeep(1);
	else
		AlertStr(NOT_FOUND_ALRT,Note,what);
}
