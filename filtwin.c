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

#include "filtwin.h"
/************************************************************************
 * Filters window - Copyright (C) 1994 QUALCOMM Incorporated
 ************************************************************************/
#define FILE_NUM 67
#ifdef TWO
#pragma segment FilterWin

#define MIN_ACTION_WI	(35*Win->hPitch)
#define MIN_MATCH_HI	(13*Win->vPitch)
#define MAX_MATCH_HI	(15*Win->vPitch)
#define MIN_ACTION_HI	(GROW_SIZE + 3*INSET + 5*MIN_ACT1_HI)
#define MIN_ACT1_HI		(24)
#define MAX_ACT1_HI		(3*Win->vPitch)
#define MAX_MATCHPOP_WI	(25*Win->hPitch)
#define BUTTON_REASONABLE	(24*Win->hPitch)
#define POP_V_ADJUST	(-3)
#define POP_H_ADJUST	INSET

#define FILT_DRAG_DATA_TYPE	'Eufl'

/************************************************************************
 * Filter globals
 ************************************************************************/
typedef enum
{
	flNewButton=0,
	flRemoveButton,
	flIncoming,
	flOutgoing,
	flManual,
	flMatchOnePop,
	flMatchTwoPop,
	flConjunction,
	flAct1,
	flAct2,
	flAct3,
	flAct4,
	flAct5,
	flAccel1,
	flAccel2,
	flNickFilePop1,
	flNickFilePop2,
	flActionGroup,
	flMatchGroup,
	flLimit
} FilterControlEnum;
#define fl1stLimit flAct1
#define fl2ndLimit flActionGroup

typedef enum
{
	flrVBar,
	flrAccel1,
	flrAccel2,
	flrMatch,
	flrAction,
	flrList,
	flrMatch1,
	flrMatch2,
	flrAct1,
	flrAct2,
	flrAct3,
	flrAct4,
	flrAct5,
	flrDate,
	flrVal1,
	flrVal2,
	flrLimit
} FilterRectEnum;

typedef struct
{
	PETEHandle headerPTE;
	PETEHandle valuePTE;
	Point labelPoint;
} MatchBlock, *MBlockPtr, **MBlockHandle;


typedef struct
{
	ControlHandle controls[flLimit];
	Rect rects[flrLimit];
	short actSpace;
	short actHi;
	MatchBlock blocks[2];
	short selected;
	MyWindowPtr win;
	ListHandle lHand;
	Boolean multiple;
	Boolean hasTwo;
	Boolean currentDirty;
	RgnHandle actRgn;
	MenuHandle actionMenu;
	DragReference oldDrag;
	Boolean dragInteresting;
	Boolean dragFromMe;
} FilterGlobalRec;

FilterGlobalRec **FLG;

#define DragInteresting (*FLG)->dragInteresting
#define DragFromMe (*FLG)->dragFromMe
#define OldDrag (*FLG)->oldDrag
#define Controls (*FLG)->controls
#define Rects (*FLG)->rects
#define Blocks (*FLG)->blocks
#define ActSpace (*FLG)->actSpace
#define ActHi (*FLG)->actHi
#define Selected (*FLG)->selected	
#define Win (*FLG)->win
#define LHand (*FLG)->lHand
#define Points (*FLG)->points
#define Multiple (*FLG)->multiple
#define CurrentDirty (*FLG)->currentDirty
#define CurrentLabel (*FLG)->currentLabel

#define RemoveButton Controls[flRemoveButton]
#define NewButton Controls[flNewButton]
#define MatchGroup Controls[flMatchGroup]
#define ActionGroup Controls[flActionGroup]
#define Block0 (*FLG)->blocks[0]
#define Block1 (*FLG)->blocks[1]
#define HasTwo (*FLG)->hasTwo
#define ActRgn (*FLG)->actRgn
#define ActionMenu (*FLG)->actionMenu
#define STEList Win->steList

/**********************************************************************
 * Prototypes
 **********************************************************************/
void FilterHelp(MyWindowPtr win,Point mouse);
OSErr FiltDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
void FilterClick(MyWindowPtr win, EventRecord *event);
short FilterListWidth(short total);
short FilterMatchHeight(short total);
void FilterDidResize(MyWindowPtr win, Rect *oldContR);
short MatchBlockHeight(short total);
void FiltersSetGreys(void);
Boolean FiltersClose(MyWindowPtr win);
OSErr FillFilters(short startingFrom);
void FiltersUpdate(MyWindowPtr win);
void SetFASpacings(short total);
void FAPopup(Rect *r);
void FAPopupValueFit(Rect *r,ControlHandle popup);
void AccelPopUp(PETEHandle pte,ControlHandle verbCntl,short top,short left);
void ActOnFilterSelect(void);
void DisplaySelectedFilter(void);
void InvalMatch2(void);
void FActionPopUp(Point pt,ControlHandle cntl);
void AdjustActionMenu (MenuHandle theMenu);
void NewAction(short n,FActionHandle fa,FilterKeywordEnum action);
void FASingleRect(Rect *inRect, Rect *r,short hi);
void FilterButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
void FilterSwap(short i, short j);
void RemoveFilter(short which);
void AddFilter(short n);
Boolean FilterActionButton(ControlHandle button,short modifiers);
void FilterSelect(short which);
void FilterTextChanged(MyWindowPtr win, TEHandle teh, short oldNl, short newNl, Boolean scroll);
OSErr FiltSetAndShowNickPop(FilterControlEnum pop,PStr value,Rect *r);
void InvalAct(short i);
short ActOf(FActionHandle fa);
Boolean FilterKey(MyWindowPtr win, EventRecord *event);
void FilterActivate(MyWindowPtr win);
Boolean FilterMenu(MyWindowPtr win, int menu, int item, short modifiers);
Boolean FilterApp1(MyWindowPtr win,EventRecord *event);
Boolean AppearInFilter(PStr what,short which,Boolean caseSens);
Boolean AppearInTerm(PStr what,MTPtr term);
void FAMultRect(Rect *inRect, Rect *r,short *spacing,short hi,short n);
Boolean AppearInActions(PStr what,FActionHandle fa);
Boolean AppearInAction(PStr what,FActionHandle fa);
void FiltDragDivider(Point where);
void FilterCursor(Point mouse);
OSErr PrintFilters(Boolean selected,Boolean now);
void FiltRedoList(short startSel,short nSel);
OSErr FiltDrop(short stice,DragReference drag);
OSErr FiltDropFile(short stice,DragReference drag);
OSErr MoveSelectedFilters(short stice);
#ifdef DEBUG
#ifdef GRIDLINES
void ControlGridlines(ControlHandle cntl);
#endif
#endif
FActionProc FAflkNone, FAflkPrint, FAflkStop, FAflkCopy, FAflkSubject, FAflkLabel,
	FAflkSound, FAflkOpenMessage, FAflkOpenMailbox, FAflkPriority, FAflkForward,
	FAflkRedirect, FAflkReply, FAflkNotifyApp, FAflkNotifyUser, FAflkServerOpts,
	FAflkTransfer, FAflkAttachments, FAflkTranslit, FAflkStatus, FAflkPersonality,FAflkJunk,
//#ifdef SPEECH_ENABLED
	FAflkSpeak,
//#endif
	FAflkAddHistory,
	FAflkMoveAttach;
OSErr PrintAFilter(short i, Rect *uRect, short *v, short *page,PMPrintContext printContext);
short FiltPrintLines(short which,Rect *uRect);
void FiltPrintTerm(MTPtr term);
void FAPopActRect(Rect *inRect, Rect *r);
OSErr FiltDoDrag(EventRecord *event);
void FiltClickHook(short cell,ListHandle lHand);
void RemoveSelectedFilters(void);
void RemoveFilterLo(short which);
void DrawFilterDate(void);
#ifdef	GX_PRINTING
OSErr GXPrintFilters(Boolean selected, Boolean now);
OSErr GXPrintAFilter(CGrafPtr GXPortPtr, short i, Rect *uRect, gxRectangle *GXuRect, short *v, short *page, long firstPage, long numPages);
short FilterToPrintFirst(long firstPage, Rect *uRect);
#endif	//GX_PRINTING
short SelectFiltersBasedOnWindow(WindowPtr winWP);
Boolean FilterFind(MyWindowPtr win,PStr what);
pascal OSErr FilterChanged(PETEHandle pte,long start,long stop);
pascal OSErr FiltDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag);
void FiltDragFilename(PStr name);
void FiltEnableVerb(ControlHandle popUpCntl,PStr head);
void FiltEnableVerbAll(void);
Boolean FilterScrollWheel(MyWindowPtr win,Point pt,short h,short v);

/**********************************************************************
 * FilterChanged - set the dirty bit on the window when an edit region changes
 **********************************************************************/
pascal OSErr FilterChanged(PETEHandle pte,long start,long stop)
{
#pragma unused(pte,start,stop)
	CurrentDirty = True;
	return noErr;
}

/************************************************************************
 * OpenFiltersWindow - open the window
 ************************************************************************/
OSErr OpenFiltersWindow(void)
{
	WindowPtr	winWP=nil;
	short err = noErr;
	void *grumble;
	MyWindowPtr win=nil;
	Rect r;
	Point cSize;
	Rect bounds;
	short i;
	Str255 not;		
	DECLARE_UPP(FiltListDef,ListDef);
	
	INIT_UPP(FiltListDef,ListDef);
	CycleBalls();

	NotifyHelpers(0,eFilterWin,nil);
	
	if (PrefIsSet(PREF_MA)) return(noErr);
	
	/*
	 * open already?
	 */
	if (SelectOpenWazoo(FILT_WIN))
		;
	else if (FLG)
		UserSelectWindow(GetMyWindowWindowPtr(Win));
	else
	{

		/*
		 * allocate space
		 */
		if (!(FLG = NewZH(FilterGlobalRec))) {WarnUser(MEM_ERR,err=MemError()); goto fail;}
		
		
		/*
		 * make the window
		 */
		if (!(win=GetNewMyWindow(FILT_WIND,nil,nil,BehindModal,False,False,FILT_WIN)))
		{
			WarnUser(MEM_ERR,err=MemError());
			goto fail;
		}
		Win = win;
		winWP = GetMyWindowWindowPtr(win);
		
		ConfigFontSetup(Win);
		//MySetThemeWindowBackground(win,kThemeListViewBackgroundBrush,False);
		
		/*
		 * regenerate the list of filters
		 */
		if (err=RegenerateFilters()) goto fail;
		
		/*
		 * and all the controls
		 */
		for (i=0;i<fl1stLimit;i++)
		{
			if (!(grumble=GetNewControlSmall_(i+FILT_CNTL_BASE,winWP)))
				{WarnUser(MEM_ERR,err=MemError()); goto fail;}
			Controls[i] = grumble;
		}
		ButtonFit(RemoveButton);
		ButtonFit(NewButton);
		
		for (i=fl1stLimit;i<fl2ndLimit;i++)
		{
			if (!(grumble=GetNewControlSmall_(i-fl1stLimit+FILT_CNTL_2ND_BASE,winWP)))
				{WarnUser(MEM_ERR,err=MemError()); goto fail;}
			Controls[i] = grumble;
		}
		
		//
		// group controls
		//
		if (!(grumble = GetNewControlSmall_(FILTER_MATCH_GROUP_CNTL,winWP)))
				{WarnUser(MEM_ERR,err=MemError()); goto fail;}
		ActionGroup = grumble;
		if (!(grumble = GetNewControlSmall_(FILTER_ACTION_GROUP_CNTL,winWP)))
				{WarnUser(MEM_ERR,err=MemError()); goto fail;}
		MatchGroup = grumble;
		
		
		/*
		 * make the fixed STE's
		 */
		SetRect(&r,0,0,20,20);
		Zero(not);
		not['\015'] = 1;

		/* header 1 */
		if (err = PeteCreate(Win,&grumble,0,nil)) {WarnUser(MEM_ERR,err); goto fail;}
		PeteFontAndSize(grumble,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		(*PeteExtra(grumble))->frame = True;
		(*PeteExtra(grumble))->infinitelyWide = True;
		Block0.headerPTE = grumble;
		PETESetCallback(PETE,Block0.headerPTE,(void*)FilterChanged,peDocChanged);
		BMD(not,(*PeteExtra(Block0.headerPTE))->not,sizeof(not));

		/* value 1 */
		if (err = PeteCreate(Win,&grumble,0,nil)) {WarnUser(MEM_ERR,err); goto fail;}
		PeteFontAndSize(grumble,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		(*PeteExtra(grumble))->frame = True;
		(*PeteExtra(grumble))->infinitelyWide = True;
		Block0.valuePTE = grumble;
		PETESetCallback(PETE,Block0.valuePTE,(void*)FilterChanged,peDocChanged);
		
		/* header 2 */
		if (err = PeteCreate(Win,&grumble,0,nil)) {WarnUser(MEM_ERR,err); goto fail;}
		(*PeteExtra(grumble))->frame = True;
		(*PeteExtra(grumble))->infinitelyWide = True;
		PeteFontAndSize(grumble,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		Block1.headerPTE = grumble;
		PETESetCallback(PETE,Block1.headerPTE,(void*)FilterChanged,peDocChanged);
		
		/* value 2 */
		if (err = PeteCreate(Win,&grumble,0,nil)) {WarnUser(MEM_ERR,err); goto fail;}
		PeteFontAndSize(grumble,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		(*PeteExtra(grumble))->frame = True;
		(*PeteExtra(grumble))->infinitelyWide = True;
		Block1.valuePTE = grumble;
		PETESetCallback(PETE,Block1.valuePTE,(void*)FilterChanged,peDocChanged);
		
		
		/*
		 * list handle
		 */
		SetRect(&bounds,0,0,1,0);
		cSize.h = cSize.v = 1;
	#ifdef FANCY_FILT_LDEF
		grumble = (void *)CreateNewList(FiltListDefUPP,FILT_LDEF,&r,&bounds,cSize,winWP,True,False,False,True);
	#else
		grumble = (void *)LNew(&r,&bounds,cSize,nil,winWP,True,False,False,True);
	#endif
		if (!grumble) {WarnUser(MEM_ERR,err=MemError()); goto fail;}
		LHand = (ListHandle)grumble;
		(*LHand)->indent.h = INSET;
		//(*LHand)->selFlags = lOnlyOne;
		
		/*
		 * install handlers
		 */
		Win->help = FilterHelp;
		Win->cursor = FilterCursor;
		Win->activate = FilterActivate;
		Win->app1 = FilterApp1;
		Win->menu = FilterMenu;
		Win->key = FilterKey;
		Win->textChanged = FilterTextChanged;
		Win->button = FilterButton;
		Win->click = FilterClick;
		Win->close = FiltersClose;
		Win->position = PositionPrefsTitle;
		Win->update = FiltersUpdate;
		Win->didResize = FilterDidResize;
		Win->dontControl = True;
		Win->drag = FiltDragHandler;
		Win->find = FilterFind;
		Win->scrollWheel = FilterScrollWheel;
		Win->userSave = true;
		FiltersSetGreys();
		grumble = (Handle)NewRgn();
		if (!grumble) {err=WarnUser(MEM_ERR,MemError()); goto fail;}
		ActRgn = (RgnHandle)grumble;
		
		grumble = (Handle)GetMenu(FLA_HI_MENU);
		if (!grumble) {err=WarnUser(MEM_ERR,MemError()); goto fail;}
		ActionMenu = (MenuHandle)grumble;
		AdjustActionMenu (ActionMenu);
		InsertMenu(ActionMenu,-1);
		
		/*
		 * fill the list
		 */
		if (err = FillFilters(0)) goto fail;		
		
		MyWindowDidResize(Win,&Win->contR);
		ShowMyWindow(winWP);
	}
	
	if (MainEvent.modifiers & shiftKey)
	{
		for (winWP = GetNextWindow (winWP); winWP && !IsWindowVisible (winWP); winWP = GetNextWindow (winWP));
		if (winWP) SelectFiltersBasedOnWindow(winWP);
	}
	
	//WinGreyBG(Win);
	
	return(noErr);

fail:
	FiltersClose(win);
	if (winWP) CloseMyWindow(winWP);
	return(err);
}


#pragma segment FilterWin

/************************************************************************
 * SelectFiltersBasedOnWindow - select the filters that match a window
 ************************************************************************/
short SelectFiltersBasedOnWindow(WindowPtr winWP)
{
	MyWindowPtr	win = GetWindowMyWindowPtr(winWP);
	short count = 0;
	TOCHandle tocH = nil;
	short sumNum;
	Point c;
	
	if (winWP)
	{
		switch (GetWindowKind(winWP))
		{
			case MESS_WIN:
			case COMP_WIN:
				tocH = (*Win2MessH(win))->tocH;
				sumNum = (*Win2MessH(win))->sumNum;
				break;
				
			case MBOX_WIN:
			case CBOX_WIN:
				tocH = (TOCHandle) GetWindowPrivateData (winWP);
				sumNum = FirstMsgSelected(tocH);
				if (sumNum<0) tocH = nil;
				break;
		}
		if (tocH)
		{
			c.h = 0;
			for (c.v=0;c.v<NFilters;c.v++)
			{
				if (FilterMatchHi(c.v,tocH,sumNum))
				{
					count++;
					LSetSelect(true,c,LHand);
				}
				else
					LSetSelect(false,c,LHand);
			}
			ActOnFilterSelect();
		}
	}
#ifdef	IMAP
	IMAPStopFiltering(true);	// FilterMatchHi may have started IMAP filtering
#endif
	return(count);
}

/************************************************************************
 * FilterCursor - set the cursor properly
 ************************************************************************/
void FilterCursor(Point mouse)
{
	Rect r;
	short cursor = arrowCursor;
	short i;
	FActionHandle fa;
	
	r = Rects[flrList]; r.right -= GROW_SIZE;
	if (PtInRect(mouse,&r))
		cursor = plusCursor;
	else if (PtInSlopRect(mouse,Rects[flrVBar],1))
		cursor = DIVIDER_CURS;
	else if (Selected && !Multiple)
	{
		if (PeteCursorList(Win->pteList,mouse))
			return;
		else
			for (i=0,fa=FR[Selected-1].actions;fa;fa=(*fa)->next,i++)
			{
				r = Rects[flrAct1+i];
				if (PtInSlopRect(mouse,Rects[flrAct1+i],2))
				{
					((*(FActionProc*)FATable((*fa)->action)))(faeCursor,fa,(Rect*)&mouse,&cursor);
					break;
				}
			}

	}
	SetMyCursor(cursor);
}

/**********************************************************************
 * FilterFind - find in the filters window
 **********************************************************************/
Boolean FilterFind(MyWindowPtr win,PStr what)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr (Win);
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Boolean found = False;
	short spot;
	short stop;
	Boolean wrapped;
	
	SelectWindow_(WinWP);
	SetPort_(GetWindowPort(WinWP));
	LActivate(True,LHand);
	
	wrapped = False;
	spot = -1;
	stop = 0;
	if (Selected) spot = stop = Selected-1;

	//	Don't let any drawing occur during find
	SetEmptyClipRgn(winPort);
	win->noUpdates = true;
	do
	{
		MiniEvents(); if (CommandPeriod || EjectBuckaroo) break;
		spot++;
		if (spot>=NFilters) {spot=0;wrapped = True;}
		FilterSelect(spot);
		if (spot==stop && wrapped) break;
		found = AppearInFilter(what,spot,PrefIsSet(PREF_SENSITIVE));
	}
	while (!found);
	win->noUpdates = false;
	InfiniteClip(winPort);	//	Restore clipping region
	InvalContent(win);
	
	return found;
}

/**********************************************************************
 * FilterScrollWheel - handle scroll wheel events for filter list
 **********************************************************************/
Boolean FilterScrollWheel(MyWindowPtr win,Point pt,short h,short v)
{
	Rect  rList;

	GetListViewBounds (LHand, &rList);
	if (PtInRect(pt,&rList))
	{
		LScroll(0,-v,LHand);
		return true;
	}
	return false;
}

/**********************************************************************
 * MatchInFilter - does a string appear in a filter?
 **********************************************************************/
Boolean AppearInFilter(PStr what,short which,Boolean caseSens)
{
#pragma unused(caseSens)
	Boolean found;
	Str255 scratch;
	
	LDRef(Filters);
	
	found = nil!=PFindSub(what,FR[which].name);
	found = found || AppearInTerm(what,&FR[which].terms[0]);
	found = found || FR[which].conjunction!=cjIgnore && AppearInTerm(what,&FR[which].terms[1]);
	found = found || PFindSub(what,GetRString(scratch,CONJ_STRN+FR[which].conjunction));
	found = found || AppearInActions(what,FR[which].actions);
		
	UL(Filters);
	return(found);
}

/**********************************************************************
 * AppearInActions - does a string appear in any of the actions?
 **********************************************************************/
Boolean AppearInActions(PStr what,FActionHandle fa)
{
	for (;fa;fa = (*fa)->next) if (AppearInAction(what,fa)) return(True);
	return(False);
}

/**********************************************************************
 * AppearInAction - does a string appear in an action?
 **********************************************************************/
Boolean AppearInAction(PStr what,FActionHandle fa)
{
	Str255 scratch;
	
	GetMenuItemText(ActionMenu,(*fa)->action,scratch);
	
	if (PFindSub(what,scratch)) return(True);
	return(CallAction(faeFind,fa,nil,what));
}

/**********************************************************************
 * AppearInTerm - does a string appear in a term?
 **********************************************************************/
Boolean AppearInTerm(PStr what,MTPtr term)
{
	Str255 verb;
	
	return (
		PFindSub(what,term->header) ||
		PFindSub(what,term->value) ||
		PFindSub(what,GetRString(verb,VERB_STRN+term->verb)) ||
		0
		);
}

/************************************************************************
 * FilterKey - handle keystrokes in the filter window
 ************************************************************************/
Boolean FilterKey(MyWindowPtr win, EventRecord *event)
{
#pragma unused(win)
	short key = event->message&0xff;
	
	if (event->modifiers & cmdKey) {return(False);}				/* no command keys! */
	
	switch(key)
	{
		case upArrowChar:
			FilterSelect(MAX(0,Selected-2));
			return(True);
		case downArrowChar:
			FilterSelect(MIN(Selected,NFilters-1));
			return(True);
	}
		
	if (!Win->pte) return(False);
	
	if (key=='\t' || key=='\015')
	{
		if (event->modifiers&shiftKey) PeteFocusPrevious(Win);
		else PeteFocusNext(Win);
		PeteSelectAll(Win,Win->pte);
		FiltEnableVerbAll();
	}
	else if (key==enterChar)
	{
		PeteSelectAll(Win,Win->pte);
		SaveCurrentFilter();
		FiltEnableVerbAll();
	}
	else
	{
		return(False);
	}
	return(True);
}

/************************************************************************
 * FilterApp1 - handle App1 event for filter
 ************************************************************************/
Boolean FilterApp1(MyWindowPtr win,EventRecord *event)
{
#pragma unused(win)
	return(ListApp1(event,LHand));
}

/************************************************************************
 * FilterActivate - handle activation and deactivation
 ************************************************************************/
void FilterActivate(MyWindowPtr win)
{
#pragma unused(win)
	LActivate(Win->isActive,LHand);
	if (!Win->isActive) SaveCurrentFilter();
}

/**********************************************************************
 * FiltersUpdate - update the filters window
 **********************************************************************/
void FiltersUpdate(MyWindowPtr win)
{
	CGrafPtr	WinPort = GetMyWindowCGrafPtr (Win);
	Rect r;
	Str63 s;
	short i;
	FActionHandle fa;
	short wi;
	
	//RaisedWindowRect(win);
	
	ConfigFontSetup(win);
	
	r = Rects[flrList];
	InsetRect(&r,1,1);
	DrawThemeListBoxFrame(&r,kThemeStateActive);

	for (i=flrMatch1;i<=flrAct5;i++)
	{
		r = Rects[i];
		DrawThemeSecondaryGroup(&r,kThemeStateActive);
	}

	TextMode(srcOr);
	
	GetRString(s,HEADER_LABEL);

	if (!Selected || Multiple)
		TextMode(grayishTextOr);
	
	MoveTo(Block0.labelPoint.h,Block0.labelPoint.v);
	DrawString(s);
	
	if (!HasTwo)
		TextMode(grayishTextOr);
	MoveTo(Block1.labelPoint.h,Block1.labelPoint.v); DrawString(s);

	TextMode(srcOr);
	TextFace(0);
	DrawFilterDate();
				
	r = Rects[flrVBar]; DrawDivider(&r,True);
	
	LUpdate(MyGetPortVisibleRegion(WinPort),LHand);

	wi = ControlWi(Controls[flMatchOnePop]);
	
	if (Selected && !Multiple)
	{
		for (i=0,fa=FR[Selected-1].actions;fa && i<MAX_ACTIONS;i++,fa=(*fa)->next)
		{
			r = Rects[flrAct1+i];
			(*(FActionProc*)FATable((*fa)->action))(faeDraw,fa,&r,nil);
		}
	}
}

/**********************************************************************
 * DrawFilterDate - draw the last use date of a filter
 **********************************************************************/
void DrawFilterDate(void)
{
	Str255 string;
	Str63 date;
	Rect r;
	long secs;
	
	r = Rects[flrDate];
	EraseRect(&r);
	if (Selected && !Multiple)
	{
		if (secs = FilterLastMatchHi(Selected-1))
		{
			DateString(secs+ZoneSecs(),shortDate,date,nil);
			ComposeRString(string,FILT_DATE_LABEL,date);
			MoveTo(r.right-StringWidth(string),r.bottom);
			DrawString(string);
		}
	}
}

/************************************************************************
 * FiltersMenu - handle our paltry menu item
 ************************************************************************/
Boolean FilterMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_SAVE_ITEM:
					if (win->isDirty) SaveFilters();
					return(True);
					break;
				case FILE_PRINT_ITEM:
				case FILE_PRINT_ONE_ITEM:
#ifdef	GX_PRINTING
					if (gGXIsPresent)
						GXPrintFilters((modifiers&shiftKey)!=0, item==FILE_PRINT_ONE_ITEM);
					else
#endif	//GX_PRINTING
						PrintFilters((modifiers&shiftKey)!=0, item==FILE_PRINT_ONE_ITEM);
					return(True);
					break;
			}
			break;
	}
	return(False);
}

/**********************************************************************
 * ActOf - get the number of an action
 **********************************************************************/
short ActOf(FActionHandle action)
{
	short i;
	FActionHandle fa;
	
	for (i=0,fa=FR[Selected-1].actions;fa;fa=(*fa)->next,i++)
		if (fa==action) break;
	return(i);
}

/**********************************************************************
 * FAPopup - find the popup for a given FAction rectangle
 **********************************************************************/
void FAPopup(Rect *r)
{
	short v = r->top;
	short h = r->left;
	short diff;
	
	GetControlBounds(Controls[flMatchOnePop],r);
	OffsetRect(r,h+INSET-r->left,v+INSET-r->top-1);
	
	// how much space is there?
	diff = ActHi-RectHi(*r);
	
	if (r->top>v+diff/2) // if space is tight, center
		OffsetRect(r,0,v+diff/2-r->top);
}

/************************************************************************
 * FAPopupValueFit - fit a popup control for a value in a filter action
 ************************************************************************/
void FAPopupValueFit(Rect *r,ControlHandle popup)
{
	Rect controlR;
	short over;
	
	// figure out how big it wants to be
	ButtonFit(popup);
	
	// figure out how big we think it should be
	FAPopActRect(r,&controlR);
	
	// if it's too tall, move it up a bit
	over = controlR.top + ControlHi(popup) - r->bottom;
	if (over>0) OffsetRect(&controlR,0,-over-2);
	
	// move the control into place
	MoveMyCntl(popup,controlR.left,controlR.top,MIN(ControlWi(popup),RectWi(controlR)),0);
}

/************************************************************************
 * FillFilters - fill the list box
 ************************************************************************/
OSErr FillFilters(short startingFrom)
{
	Str255 name;
	short n = NFilters;
	Point c;

	c.h = 0;
	
	LSetDrawingMode(False,LHand);
	LAddRow(n-startingFrom,n,LHand);
	
	for (c.v=startingFrom;c.v<n;c.v++)
	{
		PCopy(name,FR[c.v].name);
		LSetCell(name+1,*name,c,LHand);
	}
	LSetDrawingMode(True,LHand);
	return(noErr);
}

/**********************************************************************
 * FilterDidResize - resize the filters window
 **********************************************************************/
void FilterDidResize(MyWindowPtr win, Rect *oldContR)
{
#pragma unused(oldContR)
	short mblHeight;
	short x, y;
	Rect r,r2,hiddenRect;
	Str255 s;
	Point cSize;
	Rect listR, matchR, actionR, dateR;
	short wi;
	short i;
	short lineH;
	short	popupValue;
	Rect	rCntl;

	ConfigFontSetup(Win);
	
	lineH = Win->vPitch+2;
	SetWinMinSize(Win,120+55*Win->hPitch,MIN_MATCH_HI+MIN_ACTION_HI+2*INSET+Win->vPitch);

	/* Size the buttons early on */
	ButtonFit(NewButton);
	ButtonFit(RemoveButton);
	
	/*
	 * begin by figuring out the big rectangles
	 */
	listR = win->contR;
	InsetRect(&listR,GROW_SIZE,INSET);	/* leave a decent margin */
	matchR = actionR = listR;
	
	/* list rectangle needs room for a row of buttons along the bottom */
	listR.bottom -= (2*INSET+ControlHi(NewButton));
	/* list rectangle width */
	listR.right = listR.left + FilterListWidth(RectWi(listR));
	
	/* match & action offset from list */
	matchR.left = actionR.left = listR.right + GROW_SIZE;
	/* bit of room off the top */
	matchR.top += Win->vPitch/2+4;
	/* figure out relative heights of the two areas */
	matchR.bottom = matchR.top + FilterMatchHeight(RectHi(matchR));
	actionR.top = matchR.bottom + Win->vPitch + 2 + 4;
	/* leave room near grow box if possible */
	if (actionR.bottom-actionR.top>MIN_ACTION_HI+(GROW_SIZE-INSET))
		actionR.bottom -= GROW_SIZE-INSET;
	
	Rects[flrMatch] = matchR;
	Rects[flrAction] = actionR;
	Rects[flrList] = listR;
	
	r = actionR; r.top -= GROW_SIZE;
	MySetCntlRect(ActionGroup,&r);
	r = matchR; r.top -= GROW_SIZE;
	MySetCntlRect(MatchGroup,&r);
	
	/*
	 * new and remove buttons along the bottom
	 */ 
	MoveMyCntl(NewButton,listR.left,listR.bottom+INSET,0,0);
	MoveMyCntl(RemoveButton,listR.right-ControlWi(RemoveButton),listR.bottom+INSET,0,0);
	
	/*
	 * set up the rectangle for the vertical separator bar
	 */
	SET_RECT(&Rects[flrVBar],listR.right+INSET,win->contR.top+INSET,listR.right+INSET+2,actionR.bottom);

	/*
	 * the match rectangles
	 */
	mblHeight = MatchBlockHeight(RectHi(matchR));
	SET_RECT(&Rects[flrMatch1],matchR.left+GROW_SIZE,matchR.top+INSET+(3*GROW_SIZE)/2,
														 matchR.right-GROW_SIZE,matchR.top+INSET+mblHeight);
	Rects[flrMatch2] = Rects[flrMatch1];
	OFFSET_RECT(&Rects[flrMatch2],0,matchR.bottom - INSET - Rects[flrMatch1].bottom);
		
	dateR = actionR;
	dateR.bottom = dateR.top-4;
	dateR.top = dateR.bottom - (SmallSysFontSize()*12)/10;
	dateR.right = Rects[flrMatch1].right;
	dateR.left = (dateR.left+dateR.right)/2;
	Rects[flrDate] = dateR;

	/*
	 * place match controls
	 */
	x = (3*RectWi(Rects[flrMatch1]))/8;
	x = MIN(x,MAX_MATCHPOP_WI);
	x += Rects[flrMatch1].left;
	y = RectHi(Rects[flrMatch1])/2 + Rects[flrMatch1].top;
	
	MoveMyCntl(Controls[flMatchOnePop],Rects[flrMatch1].left+INSET,y+MAX_APPEAR_RIM,
																		 x-Rects[flrMatch1].left-INSET-1,
																		 lineH+2*TE_VMARGIN);
	GetControlBounds(Controls[flMatchOnePop],&rCntl);
	OFFSET_RECT(&rCntl,0,
		Rects[flrMatch2].bottom-Rects[flrMatch1].bottom);
	SetControlBounds(Controls[flMatchTwoPop],&rCntl);
	
	SetControlBounds(Controls[flConjunction],&rCntl);
	MoveControl(Controls[flConjunction],x-RectWi(rCntl)/2,
							(Rects[flrMatch2].top+Rects[flrMatch1].bottom-RectHi(rCntl))/2);
	
	/*
	 * Place match STE's
	 */
	 
	SetRect(&hiddenRect,-100,-100,-100,-100);
	 
	/* value 1 */
	GetControlBounds(Controls[flMatchOnePop],&r);
	r.left = r.right+MIN_APPEAR_SPACE;
	r.right = Rects[flrMatch1].right-INSET;
	r.top += ((r.bottom-r.top)-ONE_LINE_HI(Win))/2;
	r.bottom = r.top+ONE_LINE_HI(Win);
	Rects[flrVal1] = r;

	popupValue = GetControlValue(Controls[flMatchOnePop]);

	if (popupValue == mbmAppears || popupValue == mbmNotAppears ||
	    popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
	{
		PeteDidResize(Block0.valuePTE,&hiddenRect);
		if (popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
			FiltSetAndShowNickPop(flNickFilePop1,nil,&r);
		else
			HideControl(Controls[flNickFilePop1]);
	}
	else
	{
		HideControl(Controls[flNickFilePop1]);
		PeteDidResize(Block0.valuePTE,&r);
	}
	
	/* value 2 */
	
	popupValue = GetControlValue(Controls[flMatchTwoPop]);

	OffsetRect(&r,0,Rects[flrMatch2].bottom-Rects[flrMatch1].bottom);
	if (popupValue == mbmAppears || popupValue == mbmNotAppears ||
	    popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
	{
		PeteDidResize(Block1.valuePTE,&hiddenRect);
		if (popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
			FiltSetAndShowNickPop(flNickFilePop2,nil,&r);
		else
			HideControl(Controls[flNickFilePop1]);
	}
	else
	{
		HideControl(Controls[flNickFilePop2]);
		PeteDidResize(Block1.valuePTE,&r);
	}
	Rects[flrVal2] = r;
	
	/* header 2 */
	OffsetRect(&r,0,y-Rects[flrMatch1].bottom+3);
	r.right = r.left + (2*(r.right-r.left))/3;
	PeteDidResize(Block1.headerPTE,&r);
	Block1.labelPoint.h = x-StringWidth(GetRString(s,HEADER_LABEL))-2;
	Block1.labelPoint.v = r.top+SmallSysFontSize()+TE_VMARGIN;
	
	/* header 1 */
	OffsetRect(&r,0,Rects[flrMatch1].bottom-Rects[flrMatch2].bottom);
	PeteDidResize(Block0.headerPTE,&r);
	Block0.labelPoint = Block1.labelPoint;
	Block0.labelPoint.v += Rects[flrMatch1].bottom-Rects[flrMatch2].bottom;
	
	/* accel 1 */
	r.left = r.right+4;
	r.right = r.left+GROW_SIZE;
	r.bottom = r.top+GROW_SIZE;
	InsetRect(&r,0,4);
	Rects[flrAccel1] = r;
	MySetCntlRect(Controls[flAccel1],&r);
	//ShowControl(Controls[flAccel1]);
	
	/* accel 2 */
	OffsetRect(&r,0,Rects[flrMatch2].bottom-Rects[flrMatch1].bottom);
	Rects[flrAccel2] = r;
	MySetCntlRect(Controls[flAccel2],&r);
	//ShowControl(Controls[flAccel2]);
	
	/*
	 * incoming, outgoing, and manual buttons
	 */
	x = Rects[flrMatch1].left;
	wi = (Rects[flrMatch1].right-x-2*INSET)/3;
	y = matchR.top+INSET;
	MoveMyCntl(Controls[flIncoming],x,y,wi,lineH);
	MoveMyCntl(Controls[flOutgoing],x+wi+INSET,y,wi,lineH);
	MoveMyCntl(Controls[flManual],x+2*(wi+INSET),y,wi,lineH);
	
	/*
	 * list itself
	 */
	r = Rects[flrList];
	r.right -= GROW_SIZE;
	InsetRect(&r,1,1);
	cSize.h = r.right-r.left;
	cSize.v = lineH;
	r.bottom = r.top + (RectHi(r)/lineH)*lineH;
	ResizeList(LHand,&r,cSize);
	r.right += GROW_SIZE;
	InsetRect(&r,-1,-1);
	Rects[flrList] = r;
	
	
	/*
	 * action rectangles
	 */
	r = Rects[flrMatch1];
	SetFASpacings(Rects[flrAction].bottom-Rects[flrAction].top);
	r.top = Rects[flrAction].top + ActSpace;
	r.bottom = r.top + ActHi;
	for (i=flrAct1;i<flrAct1+MAX_ACTIONS;i++)
	{
		Rects[i] = r2 = r;
		OffsetRect(&r,0,ActSpace+ActHi);

//		FAPopup(&r2);
		{
			short v = r2.top;
			short h = r2.left;
			short diff;
			
			GetControlBounds(Controls[flAct1+i-flrAct1],&rCntl);
			r2.top = rCntl.top;
			r2.bottom = rCntl.bottom;
			GetControlBounds(Controls[flMatchOnePop],&rCntl);
			r2.left = rCntl.left;
			r2.right = rCntl.right;
			OffsetRect (&r2,h+INSET-r2.left,v+INSET-r2.top-1);
			
			// how much space is there?
			diff = ActHi-RectHi(r2);
			
			if (r2.top>v+diff/2) // if space is tight, center
				OffsetRect(&r2,0,v+diff/2-r2.top);
		}

		MySetCntlRect(Controls[flAct1+i-flrAct1],&r2);
	}
	
	if (ActRgn)
	{
		/* big box */
		r = Rects[flrAct1];
		InsetRect(&r,1,1);
		RectRgn(ActRgn,&r);
		
		/* outside of little box */
		r = Rects[flrAct1];
		FAPopup(&r);
		InsetRect(&r,-1,-1);
		r.bottom += 1;
		RgnMinusRect(ActRgn,&r);
		
		/* inside of little box */
		InsetRect(&r,2,3);
		r.top -= 1;
		RgnPlusRect(ActRgn,&r);
	}
	
	ForAllActionsDo(faeResize,nil);
	
	/*
	 * wipe it
	 */
	for (i=0;i<flLimit;i++) if (Controls[i]) SetControlVisibility(Controls[i],true,false);
	InvalContent(win);
}

/**********************************************************************
 * SetFASpacings - set the spacings for filter actions
 **********************************************************************/
void SetFASpacings(short total)
{
	short hi;
	short space;
	
	/*
	 * this code looks weird.  this code is weird.  ha ha!  joke's on you.
	 */
	space = 2*INSET/3;
	hi = (total-(MAX_ACTIONS+1)*space)/MAX_ACTIONS;
	hi = MIN(hi,MAX_ACT1_HI);
	hi = MAX(hi,MIN_ACT1_HI);
	space = (total-MAX_ACTIONS*hi)/(MAX_ACTIONS+1);
	space = MIN(space,(4*INSET)/3);
	hi = (total-(MAX_ACTIONS+1)*space)/MAX_ACTIONS;
	
	ActSpace = space;
	ActHi = hi;
}

/**********************************************************************
 * FilterListWidth - figure out the width of the filter list box
 **********************************************************************/
short FilterListWidth(short total)
{
	short width = GetPrefLong(PREF_F_LIST_WIDE);
	if (width==0) width = (GetRLong(FILTER_LIST_PERCENT)*total)/100;
	if (total-width<MIN_ACTION_WI) width = total-MIN_ACTION_WI;
	return(width);
}

/**********************************************************************
 * FilterMatchHeight - find height of the match region
 **********************************************************************/
short FilterMatchHeight(short total)
{
	short extra;
	short hi;
	extra = total - MIN_MATCH_HI - MIN_ACTION_HI - GROW_SIZE;
	hi = MIN_MATCH_HI + (2*extra)/5;
	hi = MIN(hi,MAX_MATCH_HI);
	return(hi);
}

/**********************************************************************
 * MatchBlockHeight - return the height of a single match block
 **********************************************************************/
short MatchBlockHeight(short total)
{
	short extra = total-MIN_MATCH_HI;
	extra = MAX(extra,0);
	return((MIN_MATCH_HI-2*INSET-(3*GROW_SIZE)/2)/2 + 2*extra/5);
}

/************************************************************************
 * FiltersSetGreys - set the greys for controls
 ************************************************************************/
void FiltersSetGreys(void)
{
	FilterControlEnum fl;

	if (!Selected) // Grey all controls except new button
		for(fl=0;fl<flLimit;fl++) SetGreyControl(Controls[fl],fl!=flNewButton);
	else if (Multiple)
	{
		// Grey controls from the incoming button to the end of the list
		for(fl=0;fl<flIncoming;fl++) SetGreyControl(Controls[fl],False);
		for(fl=flIncoming;fl<flLimit;fl++) SetGreyControl(Controls[fl],True);
	}
	else
	{
		if (HasTwo)
			for (fl=0;fl<flLimit;fl++) SetGreyControl(Controls[fl],False);
		else
			for (fl=0;fl<flLimit;fl++) SetGreyControl(Controls[fl],fl==flMatchTwoPop);
	}
}

#pragma segment Main
/************************************************************************
 * FiltersClose - close the window
 ************************************************************************/
Boolean FiltersClose(MyWindowPtr win)
{
#pragma unused(win)
	if (FLG && *FLG)
	{
		int which=WANNA_SAVE_SAVE;
		
		if (!GrowZoning)
		{
			SaveCurrentFilter();
			if (Win && Win->isDirty)
			{
				which = WannaSave(Win);
				if (which==CANCEL_ITEM || which == WANNA_SAVE_SAVE && SaveFilters()) return(False);
			}
		
			if (which==WANNA_SAVE_DISCARD)
			{
				FiltersRefCount = 0;
				ZapFilters();
			}
		}
		
		if (ActionMenu)
		{
			DeleteMenu(FLA_MENU);
			ReleaseResource_(ActionMenu);
		}

		LDRef(FLG);
		if (LHand) LDispose(LHand);
		FiltersDecRef();
		if (ActRgn) DisposeRgn(ActRgn);
		ZapHandle(FLG);
	}
	return(True);
}
		

/**********************************************************************
 * FilterWindowClean - set the clean of the fitler window
 **********************************************************************/
void FilterWindowClean(Boolean clean)
{
	if (FLG && Win) Win->isDirty = !clean;
}

#pragma segment FilterWin

/**********************************************************************
 * 
 **********************************************************************/
Boolean FiltWinOpen(void)
{
	return(FLG && Win);
}

/**********************************************************************
 * FilterIsSelected - is a filter selected?
 **********************************************************************/
Boolean FilterIsSelected(short which)
{
	return FLG && Cell1Selected(which+1,LHand);
}

/**********************************************************************
 * 
 **********************************************************************/
void FiltClickHook(short cell,ListHandle lHand)
{
	Point c;
	short oldCell;
	
	if (cell && cell<=(*lHand)->dataBounds.bottom)
	{
		if (MainEvent.modifiers&cmdKey)
		{
			c.h = 0;
			c.v = cell-1;
			LSetSelect(!Cell1Selected(cell,lHand),c,lHand);
			ActOnFilterSelect();
		}
		else if (!Cell1Selected(cell,lHand))
		{
			if (MainEvent.modifiers&shiftKey)
			{
				oldCell = Next1Selected(0,lHand);
				if (oldCell && oldCell<cell)
					oldCell--;	// Next1Selected is 1-based
				else if (oldCell>cell)
				{
					c.v = cell-1;
					cell = oldCell-1;
					oldCell = c.v;
				}
				else oldCell = --cell;
				
				c.h = 0;
				for (c.v=oldCell;c.v<cell;c.v++)	LSetSelect(True,c,lHand);
				ActOnFilterSelect();
			}
			else
				FilterSelect(cell-1);
		}
	}
	else
		FilterSelect(-1);
}

/************************************************************************
 * FilterClick - handle a click in the filter window
 ************************************************************************/
void FilterClick(MyWindowPtr win, EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point pt = event->where;
	Rect r;
	PETEHandle pte = nil;
	FActionHandle fa;
	short i;
	Boolean drag;
	ControlHandle cntl;
	long controlId = -1;
	
	GlobalToLocal(&pt);
	
	r = Rects[flrAction];
	if (PtInRect(pt,&r))
	{
		ForAllActionsDo(faeMouseGoingDown,nil);
	}
	r = Rects[flrList];
	if (PtInRect(pt,&r))
	{
		LClickWDrag(pt,event->modifiers,LHand,&drag,FiltClickHook);
		if (drag) FiltDoDrag(event);
		ActOnFilterSelect();
	}
	else if ((r = Rects[flrVBar]),PtInSloppyRect(pt,&r,1))
	{
		FiltDragDivider(pt);
	}
	else if (FindControl(pt,winWP,&cntl) && (GetControlReference(cntl)&0xffffff00)=='act\0')
	{
		FActionPopUp(pt,cntl);
		controlId = GetControlReference(cntl);
	}
	else if (HandleControl(pt,win))
		;
	else if (Selected && !Multiple)
	{
		if ((r=Rects[flrAccel1]),PtInRect(pt,&r))
		{
			AccelPopUp(Block0.headerPTE,Controls[flMatchOnePop],r.top,r.left);
			controlId = AUDITCONTROLID(GetWindowKind(winWP),flAccel1);
		}
		else if (HasTwo && ((r=Rects[flrAccel2]),PtInRect(pt,&r)))
		{
			AccelPopUp(Block1.headerPTE,Controls[flMatchTwoPop],r.top,r.left);
			controlId = AUDITCONTROLID(GetWindowKind(winWP),flAccel2);
		}
		else if (PtInPETE(pt,Block0.headerPTE)) pte = Block0.headerPTE;
		else if (PtInPETE(pt,Block0.valuePTE)) pte = Block0.valuePTE;
		else if (HasTwo && PtInPETE(pt,Block1.headerPTE)) pte = Block1.headerPTE;
		else if (HasTwo && PtInPETE(pt,Block1.valuePTE)) pte = Block1.valuePTE;
		else
			for (i=0,fa=FR[Selected-1].actions;fa;fa=(*fa)->next,i++)
			{
				r = Rects[flrAct1+i];
				if (PtInRect(pt,&r))
				{
					((*(FActionProc*)FATable((*fa)->action)))(faeClick,fa,(Rect*)&pt,event);
					break;
				}
			}
		if (pte) PeteEditWFocus(Win,pte,peeEvent,(void*)event,nil);
	}
	
	FiltEnableVerbAll();
	
	if (controlId >= 0)
		AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(winWP), controlId, event->what);
}

/**********************************************************************
 * FiltDoDrag - drag some filters
 **********************************************************************/
OSErr FiltDoDrag(EventRecord *event)
{
	Point p = event->where;
	short theCell;
	short n = NFilters;
	DragReference drag;
	OSErr err = noErr;
	Handle data;
	RgnHandle rgn;
	PromiseHFSFlavor promise;
	FSSpec spec;	// we may write here
	DECLARE_UPP(FiltDragSend,DragSendData);

	INIT_UPP(FiltDragSend,DragSendData);
	
	GlobalToLocal(&p);
	theCell = InWhich1Cell(p,LHand);
	
	if (theCell>n)
		LClick(p,event->modifiers,LHand);
	else
	{
		if (!Cell1Selected(theCell,LHand))
			FilterSelect(theCell-1);
		if (!(err=MyNewDrag(Win,&drag)))
		{
			if (!(data=NuHTempBetter(0)))
				err = MemError();
			else if (!(rgn=MyLDragRgn(LHand)))
				err = MemError();
			else
			{
				for (theCell=0;theCell=Next1Selected(theCell,LHand);)
					if (err=PtrPlusHand(&theCell,data,sizeof(theCell))) break;

				// Setup the promiseHFS
				Zero(promise);
				promise.fileType = FILTER_FTYPE;
				promise.fileCreator = CREATOR;
				promise.promisedFlavor = SPEC_FLAVOR;
				Zero(spec);

				if (!(err=FinderDragVoodoo(drag)))	// bad finder.  bad.
				if (!err)
				{
					// add the cell data for internal drags
					err = AddDragItemFlavor(drag,1,FILT_DRAG_DATA_TYPE,
																			 LDRef(data),GetHandleSize(data),
																			 flavorSenderOnly|flavorNotSaved);
					ZapHandle(data);
					if (!err)
					// add the promiseHFS
					if (!(err=AddDragItemFlavor(drag,1,flavorTypePromiseHFS,&promise,sizeof(promise),0)))
					// add an flavor to hold the spec we're supposed to write to
					if (!(err=AddDragItemFlavor(drag,1,SPEC_FLAVOR,nil,0,0)))
					// now the proc that really does the work if somebody takes us up on our promise
					if (!(err=SetDragSendProc(drag,FiltDragSendUPP,(void*)&spec)))
					{
						// finally actually track the drag
						if (!(err=MyTrackDrag(drag,event,rgn)))
						{
							if (*spec.name)
							{
								// We need to write selected filters to this file
								err = SaveFiltersLo(&spec,false,false);
							}
						}
					}
					DisposeRgn(rgn);
				}
			}
			MyDisposeDrag(drag);
		}
	}
	return(err);
}

/**********************************************************************
 * FiltDragSend - drag data proc for filters
 **********************************************************************/
pascal OSErr FiltDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag)
{
	AEDesc dropLocation;
	OSErr err=cantGetFlavorErr;
	FSSpecPtr spec = (FSSpecPtr)dragSendRefCon;
		
	/*
	 * return the filename
	 */
	if (flavor==SPEC_FLAVOR)
	{
		NullADList(&dropLocation,nil);
		if (!(err=GetDropLocation(drag,&dropLocation)))
		if (!(err=GetDropLocationDirectory(&dropLocation,&spec->vRefNum,&spec->parID)))
		{
			FiltDragFilename(spec->name);
			UniqueSpec(spec,31);
			FSpCreateResFile(spec,CREATOR,FILTER_FTYPE,smSystemScript);
			WhackFinder(spec);
			if (!(err = ResError()))
				err = MySetDragItemFlavorData(drag,1,SPEC_FLAVOR,spec,sizeof(FSSpec));
		}
		DisposeADList(&dropLocation,nil);
	}
	return(err);
}

/**********************************************************************
 * FiltDragFilename - setup a filename for the target of a filter drag
 **********************************************************************/
void FiltDragFilename(PStr name)
{
#ifdef TO_DO
#pragma warning Need to write better naming function for filter drags
#endif
	GetRString(name,FILTERS_NAME);
}

/**********************************************************************
 * FiltDragHandler - drag handler for filters window 
 **********************************************************************/
OSErr FiltDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{
	OSErr err=noErr;
	Point mouse;
	Rect r;
	short stice;
	static short flags;
	
	if (drag!=OldDrag)
	{
		Boolean interesting, me, draggingFile;
		interesting = DragIsInteresting(drag,FILT_DRAG_DATA_TYPE,nil);
		if (!interesting) draggingFile = interesting = DragIsInterestingFileType(drag,nil,FILTER_FTYPE,nil);
		else draggingFile = false;
		flags = ldtInterstice;
		if (draggingFile) flags |= ldtIgnoreSelection;
		me = DragSource==win;
		DragInteresting = interesting;
		OldDrag = drag;
		DragFromMe = me;
	}
	
	if (!DragInteresting) return(Selected ? dragNotAcceptedErr:1);
	
	if (which==0xfff) HideDragHilite(drag);
	
	SetPort(GetMyWindowCGrafPtr(Win));
	GetDragMouse(drag,&mouse,nil);
	GlobalToLocal(&mouse);
	
	switch (which)
	{
		case 0xfff:
		case kDragTrackingInWindow:
			if (DragInteresting)
			{
				r = Rects[flrList];
				r.right -= GROW_SIZE;
				InsetRect(&r,0,-GROW_SIZE);
				if (PtInRect(mouse,&r))
				{
					stice = MyLDragTracker(drag,mouse,flags,LHand);
					if (stice==-1) err = dragNotAcceptedErr;
					else if (which==0xfff)
					{
						HideDragHilite(drag);
						if (DragInteresting) err = FiltDrop(stice,drag);
						else err == dragNotAcceptedErr;
					}
				}
			}
			break;
			
		case kDragTrackingLeaveWindow:
			HideDragHilite(drag);
			break;
		}
	
	return(err);
}

/**********************************************************************
 * FiltDrop - handle a drop onto filters
 **********************************************************************/
OSErr FiltDrop(short stice,DragReference drag)
{
	short n = NFilters;
	short **data;
	FRHandle fr = nil;
	OSErr err;
	
	ASSERT(MyCountDragItems(drag)==1);
	
	SaveCurrentFilter();
	
	if (!(err=MyGetDragItemData(drag,1,FILT_DRAG_DATA_TYPE,(Handle*)&data)))
	{
		ZapHandle(data);
		err = MoveSelectedFilters(stice);
	}
	else
		err = FiltDropFile(stice,drag);

	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr FiltDropFile(short stice,DragReference drag)
{
	short item;
	OSErr err=noErr;
	HFSFlavor **data;
	FSSpec spec;
	short n = MyCountDragItems(drag);
	short oldN = NFilters;
	
	// Add the filters to the filter structure
	for (item=1;item<=n && !err;item++)	// loop through each item
	{
		if (!(err=MyGetDragItemData(drag,item,flavorTypeHFS,(void*)&data)))
		{
			spec = (*data)->fileSpec;
			ZapHandle(data);
			err = ReadFilters(Filters,spec.vRefNum,spec.parID,spec.name);
		}
	}

	// Add the filters to the list in the window
	n = NFilters - oldN;
	if (n>0)
	{
		FillFilters(oldN);
		FiltRedoList(oldN,n);
		MoveSelectedFilters(stice);
	}
	
	return(err);
}

/**********************************************************************
 * FiltSetAndShowNickPop - populate the nickname file popup and move it into place
 **********************************************************************/
OSErr FiltSetAndShowNickPop(FilterControlEnum pop,PStr value,Rect *r)
{
	// if we're passed with a string, set the contents
	// of the menu and its value
	if (value)
	{
		MenuHandle mh;
		short ab;
		Str63 name;
		short val = 1;
		
		if (mh = GetControlPopupMenuHandle(Controls[pop]))
		{
			TrashMenu(mh,1);
			// The address book menu will consist of only the following:
			//		Eudora Nicknames
			//		Regular Address Books
			//		History List
			//		�Any�
			GetRString(name,ANY_ALIAS_FILE);
			MyAppendMenu(mh,name);
			for (ab = 0; ab < NAliases; ++ab)
				if (IsEudoraAddressBook (ab) || IsRegularAddressBook (ab) || IsHistoryAddressBook (ab))
				{
					PCopy(name,(*Aliases)[ab].spec.name);
					MyAppendMenu (mh,name);
					if (StringSame(name,value)) val = CountMenuItems(mh);
				}
			
			SetControlMaximum (Controls[pop], CountMenuItems (mh));
			SetControlMinimum (Controls[pop], 1);
			SetControlValue   (Controls[pop], val);
		}
	}
	
	MySetCntlRect(Controls[pop],r);
	ShowControl(Controls[pop]);
	return noErr;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr MoveSelectedFilters(short stice)
{
	short fNum;
	short nSelected=0;
	FRHandle fr=nil;
	short i;
	short spot;
	short goodSpot;
	short n = NFilters;
	
	for (fNum=0;fNum<n;fNum++)
	if (Cell1Selected(fNum+1,LHand))
	{
		FR[fNum].kill = true;
		nSelected++;
	}
	
	if (fr=NuHTempBetter(sizeof(FilterRecord)*nSelected))
	{
		/*
		 * copy out the filters in question
		 */
		i=0;
		for (fNum=0;fNum<n;fNum++)
			if (FR[fNum].kill)
			{
				(*fr)[i] = FR[fNum];
				(*fr)[i].kill = false;
				i++;
			}
		
		/*
		 * where will the new spot be?
		 */
		spot = 0;
		for (fNum=0;fNum<stice;fNum++) if (!FR[fNum].kill) spot++;
		
		/*
		 * move the ones that belong before
		 */
		for (fNum=goodSpot=0;goodSpot<spot && fNum<n;fNum++)
		{
			if (FR[fNum].kill) continue;	//will be overwritten
			if (fNum!=goodSpot) FR[goodSpot]=FR[fNum];	//filter ok, copy into place
			goodSpot++;	//found another good one
		}
		
		/*
		 * move the ones that belong after
		 */
		for (goodSpot=fNum=n-1;goodSpot>=spot+nSelected;fNum--)
		{
			if (FR[fNum].kill) continue;	//will be overwritten
			if (fNum!=goodSpot) FR[goodSpot] = FR[fNum];	//filter ok, copy into place
			goodSpot--;	//found another good one
		}
		
		/*
		 * now, move them into place
		 */
		BMD(*fr,&FR[spot],nSelected*sizeof(FilterRecord));
		ZapHandle(fr);
		
		/*
		 * cleanup
		 */
		if (nSelected==1 && Selected && !Multiple) Selected = spot+1;
		FiltRedoList(spot,nSelected);
		
		return noErr;
	}
	return MemError();
}

/**********************************************************************
 * 
 **********************************************************************/
void FiltRedoList(short startSel,short nSel)
{
	Point c;
	short n = NFilters;
	Str255 title;
	
	c.h = 0;
	
	/*
	 * retitle every cell
	 */
	for (c.v=0;c.v<n;c.v++)
	{
		PCopy(title,FR[c.v].name);
		LSetCell(title+1,*title,c,LHand);
		LSetSelect(False,c,LHand);
	}
	
	/*
	 * select the newly selected ones
	 */
	for (c.v=startSel;c.v<startSel+nSel;c.v++)
		LSetSelect(True,c,LHand);
	
	/*
	 * neaten
	 */
	ActOnFilterSelect();

	/*
	 * redraw them on update
	 */
	Win->isDirty = True;
}

/**********************************************************************
 * 
 **********************************************************************/
void FiltDragDivider(Point pt)
{
	Rect div, bounds;
	Point newPt;
	
	div = Rects[flrVBar];
	bounds = Win->contR;
	bounds.right = RectWi(Win->contR)-MIN_ACTION_WI;
	bounds.left = (GetRLong(FILT_LIST_MIN)*Win->minSize.h)/100;
	
	newPt = DragDivider(pt,&div,&bounds,Win);
	
	/*
	 * resize?
	 */
	if (newPt.h)
	{
		SetPrefLong(PREF_F_LIST_WIDE,newPt.h-GROW_SIZE-INSET);
		MyWindowDidResize(Win,nil);
	}
}

/************************************************************************
 * FilterButton - handle hits on the buttons.
 ************************************************************************/
void FilterButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
#pragma unused(modifiers,part)
	FilterControlEnum which;
	Rect	r,hiddenRect;
	short	popupValue;
	Boolean wasFilterActionButton = false;
			
	SetRect(&hiddenRect,-100,-100,-100,ONE_LINE_HI(Win)-100);
	
	for (which=flNewButton;which<flLimit;which++)
		if (button==Controls[which]) break;
	
	switch(which)
	{
		case flRemoveButton:
			RemoveSelectedFilters();
			break;

		case flNewButton:
			AddFilter(Selected?Selected:NFilters);
			break;

		case flIncoming:
		case flOutgoing:
		case flManual:
			MySetCtlValue(Controls[which],!GetControlValue(Controls[which]));
			Win->isDirty = CurrentDirty = True;
			break;

		case flMatchOnePop:
			Win->isDirty = CurrentDirty = True;
			/* value 1 */
			r = Rects[flrVal1];
		
			popupValue = GetControlValue(Controls[flMatchOnePop]);
		
			if (popupValue == mbmAppears || popupValue == mbmNotAppears ||
			    popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
			{
				PeteDidResize(Block0.valuePTE,&hiddenRect);
				if (popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
					FiltSetAndShowNickPop(flNickFilePop1,"",&r);
				else
					HideControl(Controls[flNickFilePop1]);
			}
			else
			{
				HideControl(Controls[flNickFilePop1]);
				PeteDidResize(Block0.valuePTE,&r);
			}
			InsetRect(&r,-3,-3);
			InvalWindowRect(GetMyWindowWindowPtr(win),&r);
			break;

		case flMatchTwoPop:
			Win->isDirty = CurrentDirty = True;
			/* value 2 */
			popupValue = GetControlValue(Controls[flMatchTwoPop]);
			r = Rects[flrVal2];

		  if (popupValue == mbmAppears || popupValue == mbmNotAppears ||
		     popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
		  {
				PeteDidResize(Block1.valuePTE,&hiddenRect);
				if (popupValue == mbmIntersectsFile || popupValue == mbmNotIntersectsFile)
					FiltSetAndShowNickPop(flNickFilePop2,"",&r);
				else
					HideControl(Controls[flNickFilePop2]);
			}
			else
			{
				HideControl(Controls[flNickFilePop2]);
				PeteDidResize(Block1.valuePTE,&r);
			}
			InsetRect(&r,-3,-3);
			InvalWindowRect(GetMyWindowWindowPtr(win),&r);

			break;
		
		case flNickFilePop1:
		case flNickFilePop2:
			Win->isDirty = CurrentDirty = True;
			break;

		case flConjunction:
			if (HasTwo != (GetControlValue(Controls[flConjunction])!=cjIgnore))
			{
				InvalMatch2();
				HasTwo = !HasTwo;
				FiltersSetGreys();
				if (!HasTwo && (Win->pte==Block1.headerPTE || Win->pte==Block1.valuePTE))
				{
					PeteFocus(Win,Block0.headerPTE,True);
					PeteSelectAll(Win,Win->pte);
					Win->hasSelection = MyWinHasSelection(Win);
				}
			}
			Win->isDirty = CurrentDirty = True;
			break;
			
		default:
			if (FilterActionButton(button,modifiers))
				Win->isDirty = CurrentDirty = True;
			wasFilterActionButton = true;
			break;
	}
	
	// Audit the non-filter action button controls.
	if (!wasFilterActionButton)
		AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind (GetMyWindowWindowPtr (win)), AUDITCONTROLID(GetWindowKind (GetMyWindowWindowPtr (win)),which), mouseDown);
}

/**********************************************************************
 * RemoveSelectedFilters - remove the current set of filters
 **********************************************************************/
void RemoveSelectedFilters(void)
{
	short cell;
	short selectMe=0;
	Rect	r;
	
	cell=1;
	while (cell=Next1Selected(cell-1,LHand))
	{
		RemoveFilterLo(cell-1);
		if (!selectMe) selectMe = cell;
	}
	if (NFilters)
		FilterSelect(selectMe-1);
	else
		{
			FilterSelect(0);
			DisplaySelectedFilter();
			FiltersSetGreys();
			r = Win->contR;
			InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
		}

}

/**********************************************************************
 * FilterActionButton - is the given click in one of an action's buttons?
 **********************************************************************/
Boolean FilterActionButton(ControlHandle button,short modifiers)
{
	Boolean retVal = False;
	FActionHandle fa;
	short i;
	Point pt;
	Rect r;
	
	GetControlBounds(button,&r);
	pt.h = (r.left+r.right)/2;
	pt.v = (r.top+r.bottom)/2;
	
	if (Selected && !Multiple)
	{
		for (i=0,fa=FR[Selected-1].actions;fa && i<MAX_ACTIONS;fa=(*fa)->next,i++)
		{
			r = Rects[flrAct1+i];
			if (PtInRect(pt,&r))
			{
				(*(FActionProc*)FATable((*fa)->action))(faeButton,fa,(void*)&modifiers,button);
				return(True);
			}
		}
	}
	return retVal;
}

/************************************************************************
 * FilterSwap - swap two filters
 ************************************************************************/
void FilterSwap(short i, short j)
{
	FilterRecord fi,fj;
	Point pi, pj;
	Boolean iSelect, jSelect;
	
	/*
	 * make local copies
	 */
	pi.h = pj.h = 0;
	pi.v = i;
	pj.v = j;
	fi = FR[i];
	fj = FR[j];
  iSelect = LGetSelect(False,&pi,LHand);
	jSelect = LGetSelect(False,&pj,LHand);
	
	/*
	 * swap data in the handle
	 */
	FR[j] = fi;
	FR[i] = fj;
	
	/*
	 * swap data in the list
	 */
	LSetCell(fj.name+1,*fj.name,pi,LHand);
	LSetSelect(jSelect,pi,LHand);
	LSetCell(fi.name+1,*fi.name,pj,LHand);
	LSetSelect(iSelect,pj,LHand);
	Win->isDirty = True;
}

/************************************************************************
 * FilterTextChanged - the te routines did something to our text
 ************************************************************************/
void FilterTextChanged(MyWindowPtr win, TEHandle teh, short oldNl, short newNl, Boolean scroll)
{
	CurrentDirty = True;
}

/************************************************************************
 * RemoveFilter - get rid of a filter
 ************************************************************************/
void RemoveFilter(short which)
{
	short n = NFilters;
	
	RemoveFilterLo(which);
	if (FLG && *FLG && Win)
	{
		if (which==Selected-1)
		{
			Selected = 0;
			if (NFilters)
				FilterSelect(which);
			else
			{
				DisplaySelectedFilter();
				FiltersSetGreys();
			}
		}
	}
}

void RemoveFilterLo(short which)
{
	short n = NFilters;
	
	if (FLG && *FLG && Win && which==Selected-1 && !Multiple)
		ForAllActionsDo(faeClose,nil);
	
	Selected = Multiple = 0;
	
	if (which<--n)
		BMD(&FR[which+1],&FR[which],(n-which)*sizeof(FilterRecord));
	SetHandleBig_(Filters,n*sizeof(FilterRecord));

	if (FLG && *FLG && Win)
	{
		LDelRow(1,which,LHand);
		Win->isDirty = CurrentDirty = True;
	}
}

/************************************************************************
 * AddFilter - add a filter.  n is filter should be
 ************************************************************************/
void AddFilter(short n)
{
	Point c;
	FilterRecord fr;
	short i;
	FActionHandle fa;
	
	FRInit(&fr);
	fr.incoming = True;
	fr.fu.id = FilterNewId();
	GetRString(fr.name,UNTITLED);
	
	SetHandleBig_(Filters,(NFilters+1)*sizeof(fr));
	if (MemError()) {WarnUser(MEM_ERR,MemError()); return;}
	
	for (i=MAX_ACTIONS;i;i--)
	{
		fa = NewZH(FAction);
		if (!fa) {WarnUser(MEM_ERR,MemError()); break;}
		(*fa)->action = flkNone;
		LL_Push(fr.actions,fa);
	}
	
	if (i) SetHandleBig_(Filters,NFilters*sizeof(fr));

	if (n<NFilters-1) BMD(FR+n,FR+n+1,(NFilters-n-1)*sizeof(fr));
	FR[n] = fr;
	
	FilterNoteMatch(n,GMTDateTime());
	
	if (FLG && *FLG && Win)
	{
		PushGWorld();
		SetPort_(GetMyWindowCGrafPtr(Win));
		LAddRow(1,n,LHand);
		c.h = 0; c.v = n;
		LSetCell(fr.name+1,*fr.name,c,LHand);
		FilterSelect(n);
		Win->isDirty = CurrentDirty = True;
		PopGWorld();
	}
}

/************************************************************************
 * FilterSelect - select a given cell
 ************************************************************************/
void FilterSelect(short which)
{
	if (which>0) which = MIN(which,NFilters-1);
	SaveCurrentFilter();
	SelectSingleCell(which,LHand);
	LAutoScroll(LHand);		
	ActOnFilterSelect();
}

/************************************************************************
 * ActOnFilterSelect - we have selected a filter; do something about it
 ************************************************************************/
void ActOnFilterSelect(void)
{
	short which, next;
	Rect r;
	short i;
	FActionHandle fa;
	
	which = Next1Selected(0,LHand);
	next = Next1Selected(which,LHand);
	
	if (Multiple && next) return;	/* still a multiple selection */
	if (which==Selected && !next) return;	/* same single selection */
	SaveCurrentFilter();
	
	if (Multiple)
		for (i=0;i<MAX_ACTIONS;i++) InvalAct(i);
	else if (Selected)
		for (i=0,fa=FR[Selected-1].actions;fa && i<MAX_ACTIONS;i++,fa=(*fa)->next)
			if ((*fa)->action!=flkNone) InvalAct(i);

	ForAllActionsDo(faeClose,nil);

	if ((Multiple==0)!=(next==0) || (Selected==0)!=(which==0))
	{
		r = Win->contR;
		r.left = Rects[flrVBar].right+1;
		InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
	}
	else if (which && Selected && (FR[which-1].conjunction==cjIgnore)!=HasTwo)
		InvalMatch2();
	
	r = Rects[flrDate];
	InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
	Selected = which;
	Multiple = next;

	if (Multiple)
		for (i=0;i<MAX_ACTIONS;i++) InvalAct(i);
	else if (Selected)
		for (i=0,fa=FR[Selected-1].actions;fa && i<MAX_ACTIONS;i++,fa=(*fa)->next)
			if ((*fa)->action!=flkNone) InvalAct(i);
	
	if (!Selected || Multiple)
		for (i=0;i<MAX_ACTIONS;i++) HideControl(Controls[flAct1+i]);		

	DisplaySelectedFilter();
	FiltersSetGreys();
	CurrentDirty = False;
	if (Selected && !Multiple) LAutoScroll(LHand);
}

/************************************************************************
 * InvalMatch2 - invalidate the second match rectangle
 ************************************************************************/
void InvalMatch2(void)
{
	Rect r = Rects[flrMatch2];
	InsetRect(&r,1,1);
	InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
}

/************************************************************************
 * FActionPopUp - pop up a menu for choosing a faction
 ************************************************************************/
void FActionPopUp(Point pt,ControlHandle cntl)
{
	short item;
	Str63 s;
	FActionHandle other;
	FActionHandle fa;
	short i;
	short n = (GetControlReference(cntl)&0xff) - '0';
	Boolean enable;
	
	/*
	 * find the action
	 */
	for (i=n,fa=FR[Selected-1].actions;i;i--) fa=(*fa)->next;
	
	/*
	 * enable the items
	 */
	CheckNone(ActionMenu);
	item = GetControlValue(cntl);
	for (i=CountMenuItems(ActionMenu);i;i--)
	{
		GetMenuItemText(ActionMenu,i,s);
		if (*s>1)
		{
			//Enhanced Filters - disable and mark the Pro-only filter feature set (already done at this point)
			if (enable = (HasFeature (featureEudoraPro) || !FAProOnly (i)))
				if (!FAMultiple(i) && i!=(*fa)->action)
				{
					for (other=FR[Selected-1].actions;other;other=(*other)->next)
						if (other!=fa && (*other)->action==i)
						{
							DisableItem (ActionMenu,i);
							goto loop;
						}
				}
			EnableIf (ActionMenu, i, enable);
			if (item==i) SetItemMark (ActionMenu, item, diamondChar);
		}
loop:;
	}
	
	if (TrackControl(cntl,pt,nil))
	{
		short newItem;
		
		newItem = GetControlValue(cntl);
			
		if (newItem != item)
			NewAction(n,fa,newItem);
	}
}

/************************************************************************
 * AdjustActionMenu - enable/disable items in the action menu
 ************************************************************************/
void AdjustActionMenu (MenuHandle theMenu)

{
	// Disable any non-pro features (lamer than it should be...)
	SetMenuItemBasedOnFeature (theMenu, flkLabel, featureFilterLabel, true);
	SetMenuItemBasedOnFeature (theMenu, flkPersonality, featureFilterPersonality, true);
	SetMenuItemBasedOnFeature (theMenu, flkSound, featureFilterSound, true);
	SetMenuItemBasedOnFeature (theMenu, flkSpeak, featureFilterSpeak, SpeechAvailable ());
	SetMenuItemBasedOnFeature (theMenu, flkOpenMessage, featureFilterOpen, true);
	SetMenuItemBasedOnFeature (theMenu, flkPrint, featureFilterPrint, true);
	SetMenuItemBasedOnFeature (theMenu, flkAddHistory, featureFilterAddHistory, true);
	SetMenuItemBasedOnFeature (theMenu, flkForward, featureFilterForward, true);
	SetMenuItemBasedOnFeature (theMenu, flkRedirect, featureFilterRedirect, true);
	SetMenuItemBasedOnFeature (theMenu, flkReply, featureFilterReply, true);
	SetMenuItemBasedOnFeature (theMenu, flkServerOpts, featureFilterServerOptions, true);
	SetMenuItemBasedOnFeature (theMenu, flkJunk, featureJunk, true);
}

/************************************************************************
 * AccelPopUp - pop up a menu for choosing a header quickly
 ************************************************************************/
void AccelPopUp(PETEHandle pte,ControlHandle verbCntl,short top,short left)
{
	MenuHandle mh;
	Str255 value;
	short item;
	long sel;
	Point pt;
	
	pt.h = left; pt.v = top; LocalToGlobal(&pt);
	
	if (mh=GetMenu(HEAD_ACCEL_MENU))
	{
		if (PersCount()>1) MyAppendMenu(mh,GetRString(value,FiltMetaLocalStrn+fmlPersonality));
		
		GetRString(value,FiltMetaLocalStrn+fmlJunk);
		if (*value && (item=FindItemByName(mh,value)))
			SetMenuItemBasedOnFeature(mh,item,featureJunk,true);

		item = 0;
		
		PeteSString(value,pte);
		if (*value && !(item=FindItemByName(mh,value)))
		{
			AppendMenu(mh,"\p-");
			DisableItem(mh,CountMenuItems(mh));
			MyAppendMenu(mh,value);
			item = CountMenuItems(mh);
		}
		
		InsertMenu(mh,-1);
		sel = AFPopUpMenuSelect(mh,pt.v,pt.h,item);
		if (sel&0xffff && sel&0xff && (sel&0xff)!=item)
		{
			MyGetItem(mh,sel&0xff,value);
			PeteSetTextPtr(pte,value+1,*value);
			Win->isDirty = CurrentDirty = True;
			FiltEnableVerb(verbCntl,value);
		}
		DeleteMenu(HEAD_ACCEL_MENU);
		ReleaseResource_(mh);
	}
}

/**********************************************************************
 * NewAction - change an action
 **********************************************************************/
void NewAction(short n,FActionHandle fa,FilterKeywordEnum action)
{
	FActionProc *old, *new;
	Rect r;
	
	old = (FActionProc *)FATable((*fa)->action);
	new = (FActionProc *)FATable(action);
	if (old)
	{
		(*old)(faeDispose,fa,nil,nil);
		if ((*fa)->data) ZapHandle((*fa)->data);
	}
	Win->isDirty = CurrentDirty = True;
	(*fa)->action = action;
	r = Rects[flrAct1+n];
	InvalAct(n);
	if (new)
		if (!(*new)(faeRead,fa,nil,nil))
			if (!(*new)(faeInit,fa,&r,nil));
				(*new)(faeNew,fa,&r,nil);
}

/************************************************************************
 * DisplaySelectedFilter - display a filter
 ************************************************************************/
void DisplaySelectedFilter(void)
{
	FilterRecord fr;
	Rect	r,hiddenRect;
	Str255 s;
	short i;
	FActionHandle fa;

	SetRect(&hiddenRect,-100,-100,-100,ONE_LINE_HI(Win)-100);
	
	if (Selected && !Multiple)
		fr = FR[Selected-1];
	else
		FRInit(&fr);
	
	MySetCtlValue(Controls[flIncoming],fr.incoming);
	MySetCtlValue(Controls[flOutgoing],fr.outgoing);
	MySetCtlValue(Controls[flManual],fr.manual);
#ifdef NEVER
	CurrentLabel = fr.label;
	DrawLabel();
	DrawFilterDate();
#endif
	MySetCtlValue(Controls[flConjunction],fr.conjunction);
	MySetCtlValue(Controls[flMatchOnePop],fr.terms[0].verb);
	MySetCtlValue(Controls[flMatchTwoPop],fr.terms[1].verb);

	/* value 1 */
	r = Rects[flrVal1];

	FiltEnableVerb(Controls[flMatchOnePop],fr.terms[0].header);
	
  if (fr.terms[0].verb == mbmAppears || fr.terms[0].verb == mbmNotAppears ||
      fr.terms[0].verb == mbmIntersectsFile || fr.terms[0].verb == mbmNotIntersectsFile)
  {
		PeteDidResize(Block0.valuePTE,&hiddenRect);
		if (fr.terms[0].verb == mbmIntersectsFile || fr.terms[0].verb == mbmNotIntersectsFile)
			FiltSetAndShowNickPop(flNickFilePop1,fr.terms[0].value,&r);
		else
			HideControl(Controls[flNickFilePop1]);
	}
  else
  {
		HideControl(Controls[flNickFilePop1]);
		PeteDidResize(Block0.valuePTE,&r);
	}
	
	/* value 2 */
	
	r = Rects[flrVal2];
	
	FiltEnableVerb(Controls[flMatchTwoPop],fr.terms[1].header);

  if (fr.terms[1].verb == mbmAppears || fr.terms[1].verb == mbmNotAppears ||
      fr.terms[1].verb == mbmIntersectsFile || fr.terms[1].verb == mbmNotIntersectsFile)
	{
		PeteDidResize(Block1.valuePTE,&hiddenRect);
		if (fr.terms[1].verb == mbmIntersectsFile || fr.terms[1].verb == mbmNotIntersectsFile)
			FiltSetAndShowNickPop(flNickFilePop2,fr.terms[1].value,&r);
		else
			HideControl(Controls[flNickFilePop1]);
	}
	else
	{
		HideControl(Controls[flNickFilePop2]);
		PeteDidResize(Block1.valuePTE,&r);
	}

	PeteSetString(Transmogrify(s,FiltMetaLocalStrn,fr.terms[0].header,FiltMetaEnglishStrn),Block0.headerPTE);
	PeteSetString(fr.terms[0].value,Block0.valuePTE);
	PeteSetString(Transmogrify(s,FiltMetaLocalStrn,fr.terms[1].header,FiltMetaEnglishStrn),Block1.headerPTE);
	PeteSetString(fr.terms[1].value,Block1.valuePTE);
	if (HasTwo != (fr.conjunction!=cjIgnore))
	{
		HasTwo = !HasTwo;
		InvalMatch2();
	}
	
	ForAllActionsDo(faeInit,nil);
	
	if (Selected && !Multiple)
	{
		PeteFocus(Win,Block0.headerPTE,True);
		PeteSelectAll(Win,Win->pte);
		for (i=0,fa=FR[Selected-1].actions;i<MAX_ACTIONS;i++,fa=(*fa)->next)
		{
			SetControlValue(Controls[flAct1+i],fa?(*fa)->action:flkNone);
			ShowControl(Controls[flAct1+i]);
		}
	}
	else PeteFocus(Win,nil,True);
	
	PeteCleanList(Win->pteList);
}

/**********************************************************************
 * FiltEnableVerbAll - enable all the verb popups in the filters window
 **********************************************************************/
void FiltEnableVerbAll(void)
{
	Str63 s;
	PeteSString(s,Block0.headerPTE);
	FiltEnableVerb(Controls[flMatchOnePop],s);
	PeteSString(s,Block1.headerPTE);
	FiltEnableVerb(Controls[flMatchTwoPop],s);
}

/**********************************************************************
 * FiltEnableVerb - enable one of the verb popups
 **********************************************************************/
void FiltEnableVerb(ControlHandle popUpCntl,PStr head)
{
	Boolean junkScore = EqualStrRes(head,FiltMetaEnglishStrn+fmeJunk) || EqualStrRes(head,FiltMetaLocalStrn+fmlJunk);
	short i;
	MenuHandle mh = PopUpMenuH(popUpCntl);
	
	// enable only a few things if we're searching on junk score
	for (i=mbmContains;i<mbmLimit;i++)
		EnableIf(mh,i,((i==mbmJunkLess||i==mbmJunkMore)==junkScore)||i==mbmIs||i==mbmIsnt);
	
	// make sure what is selected isn't dimmed
	if (!IsMenuItemEnabled(mh,GetControlValue(popUpCntl)))
		SetControlValue(popUpCntl,junkScore?mbmJunkMore:mbmContains);
}


/**********************************************************************
 * InvalAct - invalidate an action
 **********************************************************************/
void InvalAct(short i)
{
	Rect	rRgn;
	
	GetRegionBounds(ActRgn,&rRgn);
	OffsetRgn(ActRgn,0,Rects[flrAct1+i].bottom-rRgn.bottom-1);
	InvalWindowRgn(GetMyWindowWindowPtr(Win),ActRgn);
}

/**********************************************************************
 * ForAllActionsDo - do something for the current actions
 **********************************************************************/
short ForAllActionsDo(FACallEnum callType,void *dataPtr)
{
	short i;
	FActionHandle fa;
	Rect r;
	OSErr err, anyErr;
	
	err = anyErr = noErr;
	
	if (Selected && !Multiple)
	{
		for (i=0,fa=FR[Selected-1].actions;fa && i<MAX_ACTIONS;i++,fa=(*fa)->next)
		{
			r = Rects[flrAct1+i];
			err = (*(FActionProc*)FATable((*fa)->action))(callType,fa,&r,dataPtr);
			if (err) anyErr = err;
		}
	}
	return(anyErr);
}



/************************************************************************
 * SaveCurrentFilter - save the current filter
 ************************************************************************/
void SaveCurrentFilter(void)
{
	Str31 scratch;
	Str255 name;
	Str31 head;
	Boolean dirty = FLG && Win && PeteIsDirtyList(Win->pteList);
	
	if (FLG) CurrentDirty = dirty || CurrentDirty;

	if (FLG && Selected && !Multiple && CurrentDirty)
	{
		FilterRecord fr;
		FilterUse fu = FR[Selected-1].fu;
		Str31 idStr;
		
		FRInit(&fr);
		fr.fu = fu;
		fr.actions = FR[Selected-1].actions;
				
		fr.incoming = GetControlValue(Controls[flIncoming]);
		fr.outgoing = GetControlValue(Controls[flOutgoing]);
		fr.manual = GetControlValue(Controls[flManual]);
		fr.conjunction = GetControlValue(Controls[flConjunction]);
		fr.terms[0].verb = GetControlValue(Controls[flMatchOnePop]);
		PeteSString(fr.terms[0].header,Block0.headerPTE);
		if (fr.terms[0].verb==mbmIntersectsFile || fr.terms[0].verb==mbmNotIntersectsFile)
			MyGetItem(PopUpMenuH (Controls[flNickFilePop1]),GetControlValue(Controls[flNickFilePop1]),fr.terms[0].value);
		else if (fr.terms[0].verb==mbmAppears || fr.terms[0].verb==mbmNotAppears)
			*fr.terms[0].value = 0;
		else
			PeteSString(fr.terms[0].value,Block0.valuePTE);
			
		if (HasTwo)
		{
			fr.terms[1].verb = GetControlValue(Controls[flMatchTwoPop]);
			PeteSString(fr.terms[1].header,Block1.headerPTE);
			if (fr.terms[1].verb==mbmIntersectsFile || fr.terms[1].verb==mbmNotIntersectsFile)
				MyGetItem(PopUpMenuH (Controls[flNickFilePop2]),GetControlValue(Controls[flNickFilePop2]),fr.terms[1].value);
			else if (fr.terms[1].verb==mbmAppears || fr.terms[1].verb==mbmNotAppears)
				*fr.terms[1].value = 0;
			else
				PeteSString(fr.terms[1].value,Block1.valuePTE);
		}
		TrimWhite(fr.terms[0].header);
		TrimWhite(fr.terms[1].header);
		TrimInitialWhite(fr.terms[0].header);
		TrimInitialWhite(fr.terms[1].header);
		
		/*
		 * build the name BEFORE transmogrification
		 */
		if (*fr.terms[0].header || *fr.terms[0].value)
		{
			NumToString(fr.fu.id,idStr);
			PSCopy(head,fr.terms[0].header);
			if (head[*head]==':') --*head;
			utl_PlugParams(GetRString(scratch,FILTER_NAME),name,
				head,fr.terms[0].value,idStr,nil);
			PSCopy(fr.name,name);
		}
		else GetRString(fr.name,UNTITLED);
		
		Transmogrify(fr.terms[0].header,FiltMetaEnglishStrn,fr.terms[0].header,FiltMetaLocalStrn);
		Transmogrify(fr.terms[1].header,FiltMetaEnglishStrn,fr.terms[1].header,FiltMetaLocalStrn);
		
		/*
		 * save the actions
		 */
		ForAllActionsDo(faeSave,nil);
		
		/*
		 * study it
		 */
		StudyFilter(&fr);
		
		/*
		 * save it
		 */
		LDRef(Filters);
		{
			Point c;
			c.h = 0; c.v = Selected-1;
			FR[Selected-1] = fr;
			Win->isDirty = True;
			LSetCell(fr.name+1,*fr.name,c,LHand);
		}
		UL(Filters);
		CurrentDirty = False;
		PeteCleanList(Win->pteList);
	}
}

#pragma segment Print
/**********************************************************************
 * PrintFilters - print the filter list
 **********************************************************************/
OSErr PrintFilters(Boolean selected,Boolean now)
{
	OSErr err;
	PMPrintContext printContext;
	Rect uRect;
	short page = 0;
	short n = NFilters;
	short i;
	short v;

	PushGWorld();
	
	if (!(err = PrintPreamble(&printContext,&uRect,now)))
	{
		for (i=0;i<NFilters && !err;i++)
			err = PrintAFilter(i,&uRect,&v,&page,printContext);

		/*
		 * finish final page
		 */
		if (page)
		{
			PrintBottomHeader(page,&uRect);
			PMEndPage(printContext);
		}
		
		PrintCleanup(printContext,err);
	}
	PopGWorld();

	return(err);
}

/**********************************************************************
 * PrintAFilter - print a single filter
 **********************************************************************/
OSErr PrintAFilter(short which, Rect *uRect, short *v, short *page, PMPrintContext printContext)
{
	Str255 scratch, string;
	OSErr err = noErr;
	short h = uRect->left + (GetRLong(NICK_PRINT_NICK_PER)*RectWi(*uRect))/100;
	FActionHandle fa;
	Point	penLoc;
	
	FilterRecord fr = FR[which];
	
	/*
	 * need new page?
	 */
	if (*page==0 || *v+FiltPrintLines(which,uRect)*Win->vPitch>uRect->bottom-GetRLong(PRINT_H_MAR))
	{
		GetWTitle(GetMyWindowWindowPtr(Win),scratch);
		if (*page!=0)
		{
			PrintBottomHeader(*page,uRect);
			PMEndPage(printContext);
		}
		PMBeginPage(printContext,nil);
		*v = uRect->top+GetRLong(PRINT_H_MAR);
		PrintMessageHeader(scratch,++*page,GetRLong(PRINT_H_MAR),*v,uRect->left,uRect->right);
	}
	
	/*
	 * print the name & types
	 */
	*v += Win->vPitch;
	MoveTo(uRect->left,*v);
	BoldString(fr.name);
	if (GetPortPenLocation(GetQDGlobalsThePort(),&penLoc)->h<h-10) MoveTo(h,*v);
	else Move(10,0);
	
	*scratch = 0;
	if (fr.incoming) GetRString(scratch,FiltKindPrintStrn+fkpIn);
	if (fr.outgoing)
	{
		if (*scratch) PSCat(scratch,"\p, ");
		PCatR(scratch,FiltKindPrintStrn+fkpOut);
	}
	if (fr.manual)
	{
		if (*scratch) PSCat(scratch,"\p, ");
		PCatR(scratch,FiltKindPrintStrn+fkpMan);
	}
	if (!*scratch) GetRString(scratch,FiltKindPrintStrn+fkpNone);
	ComposeRString(string,PAREN_STRING,scratch);
	DrawString(string);
	*v += Win->vPitch;
	
	/*
	 * print the terms
	 */
	MoveTo(h,*v);
	FiltPrintTerm(fr.terms);
	if (fr.conjunction!=cjIgnore)
	{
		*v += Win->vPitch;
		MoveTo(h,*v);
		DrawChar(' ');
		BoldRString(FiltConjPrintStrn+fr.conjunction);
		DrawChar(' ');
		FiltPrintTerm(fr.terms+1);
	}
	
	MoveTo(h,*v+(Win->vPitch*2)/3);
	Hairline(True); Line(72,0); Hairline(False);
	*v += Win->vPitch;
	
	/*
	 * print the actions
	 */
	for (fa=fr.actions;fa;fa=(*fa)->next)
	{
		if ((*fa)->action==flkNone) continue;
		*v += Win->vPitch;
		
		MoveTo(h,*v);
		GetMenuItemText(ActionMenu,(*fa)->action,scratch);
		BoldString(scratch);
		DrawChar(' ');
		CallAction(faePrint,fa,nil,nil);
	}
	
	/*
	 * and leave a blank line
	 */
	*v += Win->vPitch;
	
	return(err);
}

#ifdef	GX_PRINTING
/**********************************************************************
 * GXPrintFilters - print the filter list, Quickdraw GX version
 **********************************************************************/
OSErr GXPrintFilters(Boolean selected, Boolean now)
{
	OSErr err = noErr;
	gxRectangle GXuRect;
	Rect uRect;
	short i = 0, v = 0, page = 0;
	Str255 scratch;
	CGrafPort GXPort;
	long firstPage, numPages;
	GrafPtr oldPort;
				
	GetWTitle(GetMyWindowWindowPtr(Win), scratch);
	if ((err = GXPrintPreamble(&GXuRect, scratch, &firstPage, &numPages, now)) == noErr)
	{				
		GetPort(&oldPort);
		OpenCPort(&GXPort);
		SetPort((GrafPtr)&GXPort);
		
		GXRectToRect(&GXuRect, &uRect);
		for (i = FilterToPrintFirst(firstPage, &uRect); i < NFilters && !err; i++)
			err = GXPrintAFilter(&GXPort, i, &uRect, &GXuRect, &v, &page, firstPage, numPages);

		//finish final page
		if ((page) && (err==noErr))
		{
			MyGXInstallQDTranslator(&GXPort, &GXuRect, &gGXPrintViewPort, true);	
			PrintBottomHeader(page, &uRect);
			MyGXRemoveQDTranslator(&GXPort);
			GXFinishPage(GXPageSetup);
		}

#ifdef	DEBUG
		{
			GrafPtr	test;
			
			GetPort(&test);
			ASSERT(test == (GrafPtr)&GXPort);
		}
#endif	//DEBUG
	
		CloseCPort(&GXPort);
		SetPort(oldPort);
		
		err = GXPrintCleanup();
	}
	
	return(err);
}

/**********************************************************************
 * FilterToPrintFirst - return which filter to print first.  Return the
 *	first filter that shows up on the first page in the page range.
 **********************************************************************/
short FilterToPrintFirst(long firstPage, Rect *uRect)
{
	short filtNum = 0;
	short pageNum = 1;
	FActionHandle fa;
	FilterRecord fr;
	short v = 0;
		
	while (pageNum < firstPage)
	{
		fr = FR[filtNum];
		
		//"draw" the filter
		v += 4*(Win->vPitch);
		if (fr.conjunction!=cjIgnore)
		{
			v += Win->vPitch;
		}
		for (fa=fr.actions;fa;fa=(*fa)->next)
		{
			if ((*fa)->action==flkNone) continue;
			v += Win->vPitch;
		}	
		
		//we skipped this filter.
		filtNum++;
		
		//have we "filled" a page?
		if (v+FiltPrintLines(filtNum,uRect)*Win->vPitch>uRect->bottom-GetRLong(PRINT_H_MAR))
		{
			pageNum++;
			v = uRect->top+GetRLong(PRINT_H_MAR);
		}
	}	
	
	return (filtNum);
}

/**********************************************************************
 * GXPrintAFilter - print a single filter, GX version.  This sets the
 * port to GXPortPtr.
 **********************************************************************/
OSErr GXPrintAFilter(CGrafPtr GXPortPtr, short which, Rect *uRect, gxRectangle *GXuRect, short *v, short *page, long firstPage, long numPages)
{
	Str255 scratch, string;
	OSErr err = noErr;
	short h = uRect->left + (GetRLong(NICK_PRINT_NICK_PER)*RectWi(*uRect))/100;
	FActionHandle fa;
	FilterRecord fr = FR[which];
	gxFormat pageFormat = GXGetJobFormat(GXPageSetup, 1);
	Point	penLoc;
	
	//This will allow cmd-. to stop printing.
	GXIdleJob(GXPageSetup); 
	MiniEvents(); 
	if (CommandPeriod) return(errEndOfDocument);
	
	SetPort((GrafPtr)GXPortPtr);		//all printing will be done to this port ...

	/*
	 * need new page?
	 */
	if (*page == 0 || *v+FiltPrintLines(which,uRect)*Win->vPitch>uRect->bottom-GetRLong(PRINT_H_MAR))
	{
		GetWTitle(GetMyWindowWindowPtr(Win), scratch);
		//finish current page
		if (*page != 0)
		{
			MyGXInstallQDTranslator(GXPortPtr, GXuRect, &gGXPrintViewPort, true);	
			PrintBottomHeader(*page + firstPage - 1, uRect);
			MyGXRemoveQDTranslator(GXPortPtr);
			GXFinishPage(GXPageSetup);
		}
		
		//return if the next page is past the last page in the page range
		if ((numPages < INT_MAX) && (*page + firstPage > firstPage + numPages - 1))
			return (errEndOfDocument);
		
		//start new page
		GXStartPage(GXPageSetup, (*page)+firstPage, pageFormat, 1, &gGXPrintViewPort);
		err = GXGetJobError(GXPageSetup);
		if (err != noErr)	return (err);
		(*page)++;
		*v = uRect->top+GetRLong(PRINT_H_MAR);
		MyGXInstallQDTranslator(GXPortPtr, GXuRect, &gGXPrintViewPort, true);	
		PrintMessageHeader(scratch, *page + firstPage - 1, GetRLong(PRINT_H_MAR), *v, uRect->left, uRect->right);
		MyGXRemoveQDTranslator(GXPortPtr);
	}
	
	/*
	 * print the name & types
	 */
	
	MyGXInstallQDTranslator(GXPortPtr, GXuRect, &gGXPrintViewPort, true);	
		
	*v += Win->vPitch;
	MoveTo(uRect->left,*v);
	BoldString(fr.name);
	if (GetPortPenLocation(GetQDGlobalsThePort(),&penLoc)->h<h-10) MoveTo(h,*v);
	else Move(10,0);
	
	*scratch = 0;
	if (fr.incoming) GetRString(scratch,FiltKindPrintStrn+fkpIn);
	if (fr.outgoing)
	{
		if (*scratch) PSCat(scratch,"\p, ");
		PCatR(scratch,FiltKindPrintStrn+fkpOut);
	}
	if (fr.manual)
	{
		if (*scratch) PSCat(scratch,"\p, ");
		PCatR(scratch,FiltKindPrintStrn+fkpMan);
	}
	if (!*scratch) GetRString(scratch,FiltKindPrintStrn+fkpNone);
	ComposeRString(string,PAREN_STRING,scratch);
	DrawString(string);
	*v += Win->vPitch;
	
	/*
	 * print the terms
	 */
	MoveTo(h,*v);
	FiltPrintTerm(fr.terms);
	if (fr.conjunction!=cjIgnore)
	{
		*v += Win->vPitch;
		MoveTo(h,*v);
		DrawChar(' ');
		BoldRString(FiltConjPrintStrn+fr.conjunction);
		DrawChar(' ');
		FiltPrintTerm(fr.terms+1);
	}
	
	MoveTo(h,*v+(Win->vPitch*2)/3);
	Hairline(True); Line(72,0); Hairline(False);
	*v += Win->vPitch;
	
	/*
	 * print the actions
	 */
	for (fa=fr.actions;fa;fa=(*fa)->next)
	{
		if ((*fa)->action==flkNone) continue;
		*v += Win->vPitch;
		
		MoveTo(h,*v);
		GetMenuItemText(ActionMenu,(*fa)->action,scratch);
		BoldString(scratch);
		DrawChar(' ');
		CallAction(faePrint,fa,nil,nil);
	}
	
	/*
	 * and leave a blank line
	 */
	*v += Win->vPitch;

	MyGXRemoveQDTranslator(GXPortPtr);

	return(err);
}
#endif	//GX_PRINTING

/**********************************************************************
 * 
 **********************************************************************/
void FiltPrintTerm(MTPtr term)
{	
	if (!EqualStrRes(term->header,FILTER_ANY) &&
			!EqualStrRes(term->header,FILTER_ADDRESSEE) &&
			!EqualStrRes(term->header,FILTER_BODY))
	{
		BoldRString(JUST_PLAIN_HEADER);
		DrawChar(' ');
	}
	
	DrawString(term->header);
	
	DrawChar(' ');
	BoldRString(FiltVerbPrintStrn+term->verb);
	DrawChar(' ');
	
	if (term->verb!=mbmAppears && term->verb!=mbmNotAppears)
		DrawString(term->value);
	
}

/**********************************************************************
 * FiltPrintLines - how many lines will a filter take?
 **********************************************************************/
short FiltPrintLines(short which,Rect *uRect)
{
	short lines = 3;	// filter name & condition
	FActionHandle fa;
	Str255 name;
	
	PCopy(name,FR[which].name);
	if (FR[which].conjunction!=cjIgnore) lines++;

	for (fa=FR[which].actions;fa;fa=(*fa)->next) if ((*fa)->action!=flkNone) lines++;
	
	return(lines);
}
	
#pragma segment Balloon

typedef enum
{
	fhFirstTwo=1,		/* descriptions of the first four controls; up, down, new, remove */
	fhFilterTypes=3,	/* in, out, manual */
	fhList=9,					/* list box */
	fhHeader,					/* header text boxes */
	fhValue,					/* value text boxes */
	fhGreyNoSel,			/* something is grey because no selection */
	fhGreyOne,				/* something is grey because only one term in use */
	fhMatchType,			/* match type help base */
	fhConjunction=fhMatchType+mbmLimit-1,	/* help for the conjunction */
	fhDate=fhConjunction+cjLimit-1,						/* date rectangle */
	fhNone,
	fhAction,
} FilterHelpEnum;
	
/************************************************************************
 * FilterHelp - balloon help for the filter window
 ************************************************************************/
void FilterHelp(MyWindowPtr win,Point mouse)
{
#pragma unused(win)
	Rect r;
	FilterHelpEnum fh=0;
	FilterHelpEnum greyFH=0;
	short part;
	ControlHandle cntl;
	short cNum;
	Str255 message;
	Str255 grey;
	FActionHandle fa;
	short i;
	short percent = 80;
		
	*message = 0;
	greyFH = Selected ? fhGreyOne : fhGreyNoSel;
	
	/*
	 * help on controls
	 */
	if (part=FindControl(mouse,GetMyWindowWindowPtr(win),&cntl))
	{
		for (cNum=0;cNum<flLimit;cNum++)
			if (Controls[cNum] == cntl)
			{
				GetControlBounds(cntl,&r);
				if (cNum<=flRemoveButton)
					fh = fhFirstTwo+cNum;
				else if (cNum<=flManual)
					fh = fhFilterTypes + 2*(cNum-flIncoming) + (GetControlValue(cntl) ? 0 : 1);
				else switch(cNum)
				{
					case flMatchOnePop:
					case flMatchTwoPop:
						fh = fhMatchType + GetControlValue(cntl) - 1;
						break;
					case flConjunction:
						fh = fhConjunction + GetControlValue(cntl) - 1;
						break;
				}
				if (!ControlIsGrey(cntl)) greyFH = 0;
				break;
			}
	}
	if (!fh)
	{
		greyFH = Selected ? 0 : fhGreyNoSel;
		if (PtInPETE(mouse,Block0.headerPTE))	{PeteRect(Block0.headerPTE,&r); fh = fhHeader; goto display;}
		if (PtInPETE(mouse,Block0.valuePTE))	{PeteRect(Block0.valuePTE,&r); fh = fhValue; goto display;}
		greyFH = Selected ? (HasTwo?0:fhGreyOne) : fhGreyNoSel;
		if (PtInPETE(mouse,Block1.headerPTE))	{PeteRect(Block1.headerPTE,&r); fh = fhHeader+greyFH; goto display;}
		if (PtInPETE(mouse,Block1.valuePTE))	{PeteRect(Block1.valuePTE,&r); fh = fhValue+greyFH; goto display;}
		greyFH = 0;
		r = Rects[flrList]; if (PtInRect(mouse,&r))	{fh = fhList; goto display;}
		r = Rects[flrDate]; if (PtInRect(mouse,&r))	{fh = fhDate; goto display;}
		
		if (Selected && !Multiple)
		{
			for (i=0,fa=FR[Selected-1].actions;fa;i++,fa=(*fa)->next)
			{
				r = Rects[flrAct1+i];
				if (PtInRect(mouse,&r))
				{
					if ((*fa)->action==flkNone) fh = fhNone;
					else
					{
						*message = 0;
						CallAction(faeHelp,fa,&r,message);
						fh = fhAction+1;
						if (!*message) {percent=20; GetIndString(message,526,(*fa)->action);}
						//Dprintf("\p1 %d.%d %d.%d <%d> �%p�;g",r.left,r.top,r.right,r.bottom,fh,message);
					}
					goto display;
				}
			}
		}
		return;
	}
		
display:
	if (fh==fhList)
	{
		MyBalloon(&r,90,0,0,FILT_LIST_HELP_PICT,nil);
	}
	else if (fh)
	{
		if (!*message)
		{
			GetRString(message,FILT_HELP_STRN+fh);
			if (greyFH)
			{
				GetRString(grey,FILT_HELP_STRN+greyFH);
				PSCat(message,grey);
			}
		}
		//Dprintf("\p2 %d.%d %d.%d <%d> �%p�;g",r.left,r.top,r.right,r.bottom,fh,message);
		MyBalloon(&r,percent,0,0,0,message);
	}
}

/**********************************************************************
 * FAflkNone - no action
 **********************************************************************/
short FAflkNone(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(noErr);
}

/**********************************************************************
 * FAflkStop - stop filtering now
 **********************************************************************/
short FAflkStop(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	switch (callType)
	{
		case faeInit:
		case faeResize:
		case faeDraw:
		case faeClose:
		case faeDispose:
		case faeRead:
			break;
		case faeWrite:
			return(FWriteBool(*(short*)dataPtr,(*action)->action,True));
		case faeButton:
		case faeClick:
		case faeSave:
			break;
#ifdef FANCY_FILT_LDEF
		case faeListDraw:
		{
			Point old;
			GetPortPenLocation(GetQDGlobalsThePort(),&old);
			r->right -= 10;
			MoveTo(r->right+2,old.v);
			PlotTinyIconAtPenID(ttNone,STOP_ICON);
			MoveTo(old.h,old.v);
			break;
		}
#endif
		case faeDo:
			return(euFilterStop);
			break;
	}
	return(noErr);
}

/**********************************************************************
 * FAflkJunk - junk the current message
 **********************************************************************/
short FAflkJunk(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	OSErr err = noErr;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	TOCHandle junkTOC = NULL;
				
	switch (callType)
	{
		case faeInit:
		case faeResize:
		case faeDraw:
		case faeClose:
		case faeDispose:
		case faeRead:
			break;
		case faeWrite:
			return(FWriteBool(*(short*)dataPtr,(*action)->action,True));
		case faeButton:
		case faeClick:
		case faeSave:
			break;
#ifdef FANCY_FILT_LDEF
		case faeListDraw:
		{
			Point old;
			GetPortPenLocation(GetQDGlobalsThePort(),&old);
			r->right -= 10;
			MoveTo(r->right+2,old.v);
			PlotTinyIconAtPenID(ttNone,STOP_ICON);
			MoveTo(old.h,old.v);
			break;
		}
#endif
		case faeDo:
			if (HasFeature (featureJunk))
			{
				// locate proper Junk mailbox
				if ((*fpb->tocH)->imapTOC)
					junkTOC = LocateIMAPJunkToc(fpb->tocH, true,true);
				else
					junkTOC = GetSpecialTOC(JUNK);
			
				if (junkTOC)
				{
					UseFeature (featureJunk);
					err = Junk(fpb->tocH,fpb->sumNum,true,false);
					if (!err && ((fpb->tocH)!=junkTOC))
					{
						if (junkTOC)
						{
							fpb->spec = GetMailboxSpec(junkTOC,-1);
							fpb->xferred = True;
							fpb->dontUser = true;
						}
						err = euFilterXfered;
					}
				}
				if (err && err!=euFilterXfered) err = euFilterStop;	// continue filtering rest of messages in spite of error
			}
			return(err);
			break;
	}
	return(noErr);
}

/**********************************************************************
 * FAflkPrint - print one
 **********************************************************************/
short FAflkPrint(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;

	switch (callType)
	{
		case faeInit:
		case faeResize:
		case faeDraw:
		case faeClose:
		case faeDispose:
		case faeRead:
			break;
		case faeWrite:
			return(FWriteBool(*(short*)dataPtr,(*action)->action,True));
		case faeButton:
		case faeClick:
		case faeSave:
			break;
		case faeDo:
			//Enhanced Filters	- the print filter action is not supported in Light
			if (HasFeature (featureFilterPrint)) {
				UseFeature (featureFilterPrint);
#ifdef	IMAP
				// if this is an IMAP message, make sure it's been downloaded.
				if ((*fpb->tocH)->imapTOC)
				{
					Boolean filteringUnderway = IMAPFilteringUnderway();
					
					if (filteringUnderway) IMAPStopFiltering(false);
					EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
					if (filteringUnderway) IMAPStartFiltering(fpb->tocH, true);
				}
#endif
				fpb->print = True;
			}
			break;
	}
	return(noErr);
}

/**********************************************************************
 * FAFlkTransfer - transfer message to another mailbox
 **********************************************************************/
typedef struct
{
	ControlHandle button;
	FSSpec spec;
	Boolean brandNew;
} FDflkTransfer, *FDflkTransferPtr, **FDflkTransferHandle;

short FAflkTransfer(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkTransfer f;
	OSErr err = noErr;
	Str255 s;
	FDflkTransferHandle data = (FDflkTransferHandle)(*action)->data;
	Rect tempR;
	Boolean copy = (*action)->action==flkCopy;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	FSSpec curSpec;
	TOCHandle toTocH;
						
	switch (callType)
	{
		/*
		 * initialize for the window
		 */
		case faeInit:
			f.button = GetNewControlSmall(FILT_CNTL_BASE+flNewButton,GetMyWindowWindowPtr(Win));
			if (!f.button) { err = ResError(); break; }
			(*data)->button = f.button;
			f.spec = (*data)->spec;
			MailboxSpecAlias(&f.spec,s);
			SetControlTitle(f.button,s);
			EmbedControl(f.button,ActionGroup);
			/* fall-through */
		case faeResize:
			f.button = (*data)->button;
			ButtonFit(f.button);
			FASingleRect(r,&tempR,ControlHi(f.button));
			if (RectWi(tempR)>BUTTON_REASONABLE) tempR.right = tempR.left+BUTTON_REASONABLE;
			MoveMyCntl(f.button,tempR.left,tempR.top,RectWi(tempR),0);
			ShowControl(f.button);
			if ((*data)->brandNew) goto buttonHit;
			break;
		
		/*
		 * close for window
		 */
		case faeDispose:
		case faeClose:
			if (data)
			{
				if ((*data)->button) DisposeControl((*data)->button);
				(*data)->button = nil;
			}
			break;
		
		/*
		 * do we care that a mailbox was renamed?
		 */
		case faeMBWillRename:
			curSpec = (*data)->spec;
			if (SameSpec(&curSpec,&((MBRenamePBPtr)dataPtr)->oldSpec)) err = 1;
			break;
			
		case faeMBDidRename:
			curSpec = (*data)->spec;
			if (SameSpec(&curSpec,&((MBRenamePBPtr)dataPtr)->oldSpec))
			{
				(*data)->spec = ((MBRenamePBPtr)dataPtr)->newSpec;
				if (FLG && Win && (*data)->button) SetControlTitle((*data)->button,((MBRenamePBPtr)dataPtr)->newSpec.name);
			}
			break;
		
		/*
		 * read in data
		 */
		case faeRead:
			Zero(f);
			if (dataPtr)
			{
				BoxSpecByName(&f.spec,dataPtr);
				f.brandNew = False;
			}
			else f.brandNew = True;
			data = NewZH(FDflkTransfer);
			if (!data) err = MemError();
			else
			{
				**data = f;
				(*action)->data = (Handle)data;
			}
			break;
		
		/*
		 * find
		 */
		case faeFind:
			PCopy(s,(*data)->spec.name);
			err = PFindSub(dataPtr,s)!=nil;
			break;
		
#ifdef FANCY_FILT_LDEF
		/*
		 * draw the item
		 */
		case faeListDraw:
		{
			Point old;
			GetPortPenLocation(GetQDGlobalsThePort(),&old);
			r->right -= 10;
			MoveTo(r->right+2,old.v);
			PlotTinyIconAtPenID(ttNone,copy?COPY_ICON:TRANSFER_ICON);
			if ((*action)->action==flkTransfer) FAflkStop(callType,action,r,dataPtr);
			MoveTo(old.h,old.v);
			break;
		}
#endif

		/*
		 * print
		 */
		case faePrint:
			PCopy(s,(*data)->spec.name);
			DrawString(s);
			break;
			
		/*
		 * write it out
		 */
		case faeWrite:
			f.spec = (*data)->spec;
			if (Box2Path(&f.spec,s)) PCopy(s,(*data)->spec.name);
			if (*s) err = FWriteKey(*(short*)dataPtr,(*action)->action,s);
			break;
		
		/*
		 * button
		 */
		case faeButton:
			f.button = (ControlHandle)dataPtr;
			if (f.button==(*data)->button)
			{
				buttonHit:
				if ((*data)->brandNew) UpdateMyWindow(GetMyWindowWindowPtr(Win));
				(*data)->brandNew = False;
				if (ChooseMailbox(TRANSFER_MENU,CHOOSE_MBOX_TRANSFER,&f.spec))
				{
					SetControlTitle(f.button,MailboxSpecAlias(&f.spec,s));
					(*data)->spec = f.spec;
					Win->isDirty = CurrentDirty = True;
				}
				return(True);
			}
			break;
		
		case faeHelp:
			if (MouseInControl((*data)->button))
			{
				GetControlBounds((*data)->button,r);
				GetIndString(dataPtr,526,((*action)->action==flkTransfer ? flkhTransfer:flkhCopy));
			}
			break;

		case faeDo:
			if (!(*data)->spec.name) return(0);	/* failure */
			f = **data;
			curSpec = GetMailboxSpec(fpb->tocH,-1);
			if (SameSpec(&curSpec,&f.spec)) return(euFilterStop);
			if (!copy && fpb->openMessage)
				(*fpb->tocH)->sums[fpb->sumNum].opts |= OPT_OPEN;
			if (!copy && fpb->print)
			{
				short oldstat = (*fpb->tocH)->sums[fpb->sumNum].state;
#ifdef IMAP
				// if this is an IMAP message, make sure it's been downloaded before we try to print it
				if ((*fpb->tocH)->imapTOC)
				{
					Boolean filteringUnderway = IMAPFilteringUnderway();
				
					if (filteringUnderway) IMAPStopFiltering(false);
					EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
					if (filteringUnderway) IMAPStartFiltering(fpb->tocH, true);
				}
#endif
				PrintClosedMessage(fpb->tocH,fpb->sumNum,True);
				SetState(fpb->tocH,fpb->sumNum,oldstat);
			}

			(*fpb->tocH)->sums[fpb->sumNum].flags |= FLAG_SKIPWARN;

			// if this is a transfer to or from an IMAP mailbox, do the transfer in the foreground.
			toTocH = TOCBySpec(&f.spec);
			
			if (((*fpb->tocH)->imapTOC) || (toTocH && (*toTocH)->imapTOC))
			{
				err = IMAPMoveMessageDuringFiltering(fpb->tocH, fpb->sumNum, toTocH, (*action)->action==flkCopy, fpb);
			}
			else
				err = MoveMessageLo(fpb->tocH,fpb->sumNum,&f.spec,(*action)->action==flkCopy,false,true);
			
			if (!err)
			{
				if (!copy)
				{
					fpb->xferred = True;
#ifdef	IMAP
					if ((*fpb->tocH)->imapTOC) fpb->xferredFromIMAP = True;
#endif
					fpb->spec = f.spec;
					err = euFilterXfered;
				}
				else if ((fpb->doReport || PrefIsSet(PREF_REPORT)) && !fpb->dontReport)
					AddSpecToList(&f.spec,fpb->report);
			}
			if (err && err!=euFilterXfered) err = euFilterStop;	// continue filtering rest of messages in spite of error
			break;
	}
			
	return(err);
}
/**********************************************************************
 * FAflkMoveAttach - move attachments
 **********************************************************************/
typedef struct
{
	ControlHandle button;
	FSSpec spec;
	Boolean brandNew;
} FDflkMVA, *FDflkMVAPtr, **FDflkMVAHandle;

short FAflkMoveAttach(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkMVA f;
	OSErr err = noErr;
	Str255 s;
	FDflkMVAHandle data = (FDflkMVAHandle)(*action)->data;
	Rect tempR;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	
	switch (callType)
	{
		/*
		 * initialize for the window
		 */
		case faeInit:
			f.button = GetNewControlSmall(FILT_CNTL_BASE+flNewButton,GetMyWindowWindowPtr(Win));
			if (!f.button) { err = ResError(); break; }
			(*data)->button = f.button;
			f.spec = (*data)->spec;
			if ((*data)->brandNew || GetDirName(nil,f.spec.vRefNum,f.spec.parID,s))
				*s = 0;
			SetControlTitle(f.button,s);
			EmbedControl(f.button,ActionGroup);
			/* fall-through */
		case faeResize:
			f.button = (*data)->button;
			ButtonFit(f.button);
			FASingleRect(r,&tempR,ControlHi(f.button));
			if (RectWi(tempR)>BUTTON_REASONABLE) tempR.right = tempR.left+BUTTON_REASONABLE;
			MoveMyCntl(f.button,tempR.left,tempR.top,RectWi(tempR),0);
			ShowControl(f.button);
			if ((*data)->brandNew) goto buttonHit;
			break;
		
		/*
		 * close for window
		 */
		case faeDispose:
		case faeClose:
			if (data)
			{
				if ((*data)->button) DisposeControl((*data)->button);
				(*data)->button = nil;
			}
			break;
		
		/*
		 * do we care that a mailbox was renamed?
		 */
		case faeMBWillRename:
			break;
			
		case faeMBDidRename:
			break;
		
		/*
		 * read in data
		 */
		case faeRead:
			Zero(f);
			f.spec = AttFolderSpec;
			if (dataPtr)
			{
				StringToNum(dataPtr,&f.spec.parID);
				f.brandNew = False;
				if (GetDirName(nil,f.spec.vRefNum,f.spec.parID,s)) f.spec = AttFolderSpec;
			}
			else
				f.brandNew = True;
			data = NewZH(FDflkMVA);
			if (!data) err = MemError();
			else
			{
				**data = f;
				(*action)->data = (Handle)data;
			}
			break;
		
		/*
		 * find
		 */
		case faeFind:
			PCopy(s,(*data)->spec.name);
			err = PFindSub(dataPtr,s)!=nil;
			break;
		
#ifdef FANCY_FILT_LDEF
		/*
		 * draw the item
		 */
		case faeListDraw:
			break;
#endif

		/*
		 * print
		 */
		case faePrint:
			PCopy(s,(*data)->spec.name);
			DrawString(s);
			break;
			
		/*
		 * write it out
		 */
		case faeWrite:
			f.spec = (*data)->spec;
			NumToString(f.spec.parID,s);
			if (*s) err = FWriteKey(*(short*)dataPtr,(*action)->action,s);
			break;
		
		/*
		 * button
		 */
		case faeButton:
			f.button = (ControlHandle)dataPtr;
			if (f.button==(*data)->button)
			{
				buttonHit:
				(*data)->brandNew = False;
				if (GetFolder(s,&f.spec.vRefNum,&f.spec.parID,True,True,True,True))
				{
					if (f.spec.vRefNum!=AttFolderSpec.vRefNum)
					{
						WarnUser(SAME_VOLUME_DUMMY,0);
						goto buttonHit;
					}
					*f.spec.name = 0;
					GetDirName(nil,f.spec.vRefNum,f.spec.parID,s);
					SetControlTitle(f.button,s);
					(*data)->spec = f.spec;
					Win->isDirty = CurrentDirty = True;
				}
				return(True);
			}
			break;
		
		case faeHelp:
			if (MouseInControl((*data)->button))
			{
				GetControlBounds((*data)->button,r);
				GetIndString(dataPtr,526,((*action)->action==flkTransfer ? flkhTransfer:flkhCopy));
			}
			break;

		case faeDo:
			f = **data;
			if ((*fpb->tocH)->sums[fpb->sumNum].flags&FLAG_HAS_ATT)
#ifdef	IMAP
			{
				if ((*fpb->tocH)->imapTOC)
				{
					Boolean filteringUnderway = IMAPFilteringUnderway();
					
					// deselect the current message.  It may or may not go away, and it will just confuse the filtering process to keep it highlighted.
					(*fpb->tocH)->sums[fpb->sumNum].selected = false;
					InvalSum(fpb->tocH,fpb->sumNum);
									
					// Filtering takes place in the foreground.  Free up the connection for the upcoming transfer
					if (filteringUnderway) IMAPStopFiltering(false);
					
					EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
					if (FetchAllIMAPAttachments(fpb->tocH, fpb->sumNum, true))
					{
						MovingAttachmentsLo(fpb->tocH,fpb->sumNum,True,False,False,false,nil,&f.spec);
					}
					
					// go back to filtering
					if (filteringUnderway) IMAPStartFiltering(fpb->tocH, true);
				}
				else
#endif
					MovingAttachmentsLo(fpb->tocH,fpb->sumNum,True,False,False,false,nil,&f.spec);
#ifdef	IMAP
			}
#endif
			break;
	}
			
	return(err);
}

/************************************************************************
 * SetNewAndOther - enable/disable "New" and/or "Other" mailbox menu items
 ************************************************************************/
static void SetNewAndOther(short menu,short newItem,short firstUserItem,Boolean disableNew,Boolean disableOther)
{
	MenuHandle	mh;
	short				menuCount,subID,item;
	
	if (mh = GetMHandle(menu))
	{
		EnableIf(mh,newItem,!disableNew);
		EnableIf(mh,newItem+1,!disableOther);	//	Other item
		menuCount = CountMenuItems(mh);
		for(item=firstUserItem;item<=menuCount;item++)
			if (subID = SubmenuId(mh,item))
				//	Do submenu with recursion
				SetNewAndOther(subID,1,3,disableNew,disableOther);
	}
}

/************************************************************************
 * DisableRemoteMailboxes - disable IMAP mailbox menu items
 ************************************************************************/
static void DisableRemoteMailboxes(short menu, Boolean restore)
{
	MenuHandle mh;
	short c;
	PersHandle p;
	Boolean disable;
	Str255 itemText;
	short vRef;
	long dirId;
	short it;
	
	if (mh = GetMHandle(menu))
	{
		// the caller has asked for a local mailbox.  Disable IMAP malboxes in this menu.
		for (c = 1; c <= CountMenuItems(mh); c++)
		{
			MyGetItem(mh,c,itemText);
			
			disable = false;
			for (p = PersList; p && !disable; p = (*p)->next)
			{
				if (StringSame((*p)->name,itemText) && IsIMAPPers(p))
				{
					// This item has the same name as an IMAP personality
					it = SubmenuId(mh,c);
					if (it)
					{
						MenuID2VD(it,&vRef,&dirId);
						if (IsIMAPVD(vRef, dirId))
						{
							// this item *is* an IMAP personality menu
							disable = true;
						}
					}
				}
			}
			
			if (disable) EnableIf(mh, c, restore);
		}	
	}
}
		
/************************************************************************
 * ChooseMailboxLo - select a mailbox from Mailboxes or Transfer menu
 *	Allow disabling of new and other mailboxes
 ************************************************************************/
Boolean ChooseMailboxLo(short menu, PStr msgStr, FSSpecPtr spec, Boolean disableNew, Boolean disableOther, Boolean disableRemote)
{
	short mId;
	WindowPtr	theWindow;
	EventRecord event;
	DialogPtr dgPtr;
	MyWindowPtr	dgPtrWin;
	Boolean rslt = False;
	
	PushGWorld();
	
	/*
	 * put up message and disable all but transfer
	 */
	for (mId=APPLE_MENU;mId<MENU_LIMIT;mId++) DisableItem(GetMHandle(mId),0);
	DisableItem(GetMHandle(WINDOW_MENU),0);
	EnableItem(GetMHandle(menu),0);
	DrawMenuBar();
	ParamText(msgStr,"","","");

	dgPtrWin = GetNewMyDialog (XFER_MENU_DLOG,nil,nil,InFront);
	dgPtr = GetMyWindowDialogPtr (dgPtrWin);
//	dgPtr = GetNewDialog(XFER_MENU_DLOG,nil,InFront);
//	DrawDialog(dgPtr);
	SetMyCursor(arrowCursor); SFWTC = True;
	
	if (disableNew || disableOther)
		//	Disable "New" and/or "Other" menu items
		SetNewAndOther(menu,MAILBOX_NEW_ITEM,MAILBOX_FIRST_USER_ITEM,disableNew,disableOther);
		
	if (disableRemote)
		// The caller has asked for local mailboxes only.
		DisableRemoteMailboxes(menu, false);

	/*
	 * now, let the user choose something:
	 */
	PushModalWindow(GetDialogWindow(dgPtr));
	DirtyHackForChooseMailbox = true;
	for (;;)
	{
		if (WNE(mDownMask|keyDownMask|updateMask,&event,REAL_BIG))
		{
			if (event.what==updateEvt) MiniMainLoop(&event);
			//	Check for mouseDown and keyDown events. WNE in OS X
			//	is currently return more than what's in the event mask. 
			else if (event.what==mouseDown || event.what==keyDown) break;
		}
	}
	PopModalWindow();
	if (event.what==mouseDown)
	{
		if (inMenuBar == FindWindow_(event.where,&theWindow))
		{
			long mSelect = MenuSelect(event.where);
			short mnu, itm;
			
			mnu = (mSelect>>16)&0xffff;
			itm = mSelect & 0xffff;
			if (mnu==menu || IsMailboxSubmenu(mnu))
			{
				rslt=GetTransferParams(mnu,itm,spec,nil);
			}
			HiliteMenu(0);
		}
	}
	DisposDialog_(dgPtr);
	EnableMenus(FrontWindow_(),False);
	DirtyHackForChooseMailbox = false;

	if (disableNew || disableOther)
		//	Restore "New" and "Other" mailbox menu items
		SetNewAndOther(menu,MAILBOX_NEW_ITEM,MAILBOX_FIRST_USER_ITEM,false,false);
	
	if (disableRemote)
		// The caller has asked for local mailboxes only.
		DisableRemoteMailboxes(menu, true);

	PopGWorld();
	return(rslt);
}

/************************************************************************
 * ChooseMailbox - select a mailbox from Mailboxes or Transfer menu
 ************************************************************************/
Boolean ChooseMailbox(short menu, short msg, FSSpecPtr spec)
{
	Str255 msgStr;
	return ChooseMailboxLo(menu,GetRString(msgStr,msg),spec,false,false,false);
}

/**********************************************************************
 * FASingleRect - size a single rectangle in an action area
 **********************************************************************/
void FASingleRect(Rect *inRect, Rect *r,short hi)
{
	Rect popup = *inRect;
	short lines;
	
	FAPopup(&popup);
	lines = (RectHi(*inRect)-3*INSET)/Win->vPitch;
	
	if (lines<=1 || hi)
	{
		*r = popup;
		r->left = popup.right+2*INSET;
		r->right = inRect->right - 2*INSET;
		if (hi) r->bottom = r->top+hi;
	}
	else
	{
		hi = Win->vPitch*((RectHi(*inRect)-Win->vPitch-2*INSET)/Win->vPitch) + 2*TE_VMARGIN;
		r->top = popup.bottom + (inRect->bottom-popup.bottom-hi)/2;
		SetRect(r,popup.left+GROW_SIZE,r->top,inRect->right-GROW_SIZE,r->top+hi);
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void FAPopActRect(Rect *inRect, Rect *r)
{
	short wi;
	
	*r = *inRect;
	FAPopup(r);
	wi = RectWi(*r);
	r->left = r->right + 2*INSET;
	r->right = inRect->right - 2*INSET;
}

/**********************************************************************
 * FAMultRect - size multiple rectangles in an action area
 **********************************************************************/
void FAMultRect(Rect *inRect, Rect *r,short *spacing,short hi,short n)
{
	Rect overall;
	
	FASingleRect(inRect,&overall,0);
	*spacing = (RectHi(overall)>2*Win->vPitch) ? 2*INSET : INSET;

	if (hi)
	{
		OffsetRect(&overall,0,(RectHi(overall)-hi)/2);
		overall.bottom = overall.top + hi;
	}
	overall.right = overall.left + (RectWi(overall)-(n-1)*(*spacing))/n;
	*r = overall;
}

#pragma segment FiltActions
/**********************************************************************
 * 
 **********************************************************************/
short FAflkCopy(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(FAflkTransfer(callType,action,r,dataPtr));
}

/**********************************************************************
 * 
 **********************************************************************/
short FAflkTranslit(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(noErr);
}

typedef struct
{
	Rect r;
	long status;
	ControlHandle popup;
} FDflkStatus, **FDflkStatusHandle;

/**********************************************************************
 * 
 **********************************************************************/
short FAflkStatus(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkStatusHandle data;
	FDflkStatus f;
	Str255 s;
	OSErr err = noErr;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	short item;
	Rect controlR;
	
	data = (void*)(*action)->data;
		
	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_STATUS_POP,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkStatus(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			item = Status2Item(f.status);
			
			MySetCtlValue(f.popup,item);
			EmbedControl(f.popup,ActionGroup);
			**data = f;
			// fall-through
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;
		
		case faeHelp:
			FAPopActRect(r,&controlR);
			if (MouseInRect(&controlR))
			{
				*r = controlR;
				GetIndString(dataPtr,526,flkhStatus);
			}
			break;
			
		case faeDraw:
			f = **data;
			if (f.popup)
			{
				MenuHandle mh = GetMHandle(STATE_HIER_MENU);
				CGrafPtr	WinPort = GetMyWindowCGrafPtr (Win);
				EnableItem(mh,0);
				Update1Control(f.popup,MyGetPortVisibleRegion(WinPort));
#ifdef GRIDLINES
				ControlGridlines(f.popup);
#endif
			}
			break;
			
		case faeRead:
			data = NuHandleClear(sizeof(FDflkStatus));	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr)
			{
				StringToNum(dataPtr,&f.status);
				(*data)->status = f.status;
			}
			else (*data)->status = UNREAD;	// gotta be something
			break;
			
		case faePrint:
			MyGetItem(GetMHandle(STATE_HIER_MENU),Status2Item((*data)->status),s);
			DrawString(s);
			break;
			
		case faeWrite:
			NumToString((*data)->status,s);
			err = FWriteKey(*(short*)dataPtr,flkStatus,s);
			break;
		
		case faeButton:
			f = **data;
			item = GetControlValue(f.popup);
			item = Item2Status(item);
			if (item!=f.status)
			{
				f.status = item;
				Win->isDirty = CurrentDirty = True;
				**data = f;
			}
			break;

		case faeFind:
			MyGetItem(GetMHandle(STATE_HIER_MENU),(*data)->status,s);
			err = PFindSub(dataPtr,s)!=nil;
			break;
			
		case faeClick:
			break;

		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;

		case faeDo:
			SetState(fpb->tocH,fpb->sumNum,(*data)->status);
			break;
	}
	return(err);
}

/**********************************************************************
 * FAflkSubject - change a subject
 **********************************************************************/
typedef struct FDflkSubject **FDflkSubjectHandle;
typedef struct FDflkSubject
{
	PETEHandle pte;
	Str255 subject;
}	FDflkSubject;

short FAflkSubject(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkSubjectHandle data = (FDflkSubjectHandle)(*action)->data;
	FDflkSubject f;
	Rect textR;
	OSErr err = noErr;
	Str255 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;

	switch (callType)
	{
		case faeInit:
			SetRect(&textR,0,0,0,0);
			if (err=PeteCreate(Win,&f.pte,0,nil)) return(WarnUser(PETE_ERR,err));
			PeteFontAndSize(f.pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
			(*PeteExtra(f.pte))->frame = True;
			(*PeteExtra(f.pte))->infinitelyWide = True;
			PCopy(s,(*data)->subject);
			PeteSetString(s,f.pte);
			(*data)->pte = f.pte;
			PETESetCallback(PETE,f.pte,(void*)FilterChanged,peDocChanged);
			/* fall through */
		case faeResize:
			FASingleRect(r,&textR,ONE_LINE_HI(Win));
			PeteDidResize((*data)->pte,&textR);
			break;
		
		case faeNew:
			PeteFocus(Win,(*data)->pte,true);
			break;
			
		case faeDraw:
			break;
			
		case faeFind:
			err = PeteFindString(dataPtr,0,(*data)->pte)>=0;
			break;
		
		case faePrint:
			PCopy(s,(*data)->subject);
			DrawString(s);
			break;
		
		case faeCursor:
			if (PtInPETE(*(Point*)r,(*data)->pte))
				*(short*)dataPtr = iBeamCursor;
			else
				*(short*)dataPtr = arrowCursor;
			break;

		case faeDispose:
		case faeClose:
			if (data)
			{
				PeteDispose(Win,(*data)->pte);
				(*data)->pte = nil;
			}
			break;
						
		case faeRead:
			data = NewZH(FDflkSubject);
			if (!data) return(WarnUser(MEM_ERR,MemError()));
			else
			{
				(*action)->data = (void*)data;
				if (dataPtr) PCopy((*data)->subject,(UPtr)dataPtr);
			}
			break;
			
		case faeWrite:
			PCopy(s,(*data)->subject);
			if (*s) err = FWriteKey(*(short*)dataPtr,flkSubject,s);
			break;
		
		case faeHelp:
			if (MouseInPTE((*data)->pte))
			{
				PeteRect((*data)->pte,r);
				GetIndString(dataPtr,526,flkhSubject);
			}
			break;
			
		case faeClick:
			if (PtInPETE(*(Point*)r,(*data)->pte))
			{
				PeteEditWFocus(Win,(*data)->pte,peeEvent,dataPtr,nil);
				err = True;	/* actually, not an error, but... */
			}
			break;
	
		case faeSave:
			PeteSString(s,(*data)->pte);
			PCopy((*data)->subject,s);
			break;
		
		case faeDo:
			PCopy(s,(*data)->subject);
			NonSequitur(s,fpb->tocH,fpb->sumNum);
			break;

	}
	return(err);
}

/**********************************************************************
 * FAflkLabel - set the label for a message
 **********************************************************************/
typedef struct FDflkLabel **FDflkLabelHandle;
typedef struct FDflkLabel
{
	long color;
	Rect r;
	ControlHandle popup;
} FDflkLabel;

short FAflkLabel(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkLabelHandle data;
	FDflkLabel f;
	RGBColor oldColor,color;
	Str255 s;
	OSErr err = noErr;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	short item;
	
	data = (void*)(*action)->data;
		
	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_LABEL_POP,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkStatus(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			**data = f;
			FAflkLabel(faeMouseGoingDown,action,r,dataPtr);
			MySetCtlValue(f.popup,Label2Menu(f.color));
			EmbedControl(f.popup,ActionGroup);
			// fall-through
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;

		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;
			
		case faeButton:
			f = **data;
			item = GetControlValue(f.popup);
			item = Menu2Label(item);
			if (item!=f.color)
			{
				f.color = item;
				Win->isDirty = CurrentDirty = True;
				**data = f;
			}
			break;
		
		case faeMouseGoingDown:
		{
			MenuHandle mh;
			f = **data;
			if (f.popup && (mh = GetControlPopupMenuHandle(f.popup)))
			{
				TrashMenu(mh,3);
				AddFinderItems(mh);
				SetItemMark(mh,1,noMark);
				SetItemMark(mh,GetControlValue(f.popup),diamondChar);
				EnableItem(mh,0);
				SetControlMaximum(f.popup,CountMenuItems(mh));
			}
			break;
		}
		
		case faeHelp:
			FAPopActRect(r,&f.r);
			if (MouseInRect(&f.r))
			{
				*r = f.r;
				GetIndString(dataPtr,526,flkhLabel);
			}
			break;
		case faeDraw:
			f = **data;
			if (f.popup)
			{
				CGrafPtr	WinPort = GetMyWindowCGrafPtr (Win);
				MenuHandle mh = GetMHandle(LABEL_HIER_MENU);
				EnableItem(mh,0);
				Update1Control(f.popup,MyGetPortVisibleRegion(WinPort));
#ifdef GRIDLINES
				ControlGridlines(f.popup);
#endif
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkLabel);	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr)
			{
				StringToNum(dataPtr,&f.color);
				(*data)->color = f.color;
			}
			break;
			
		case faePrint:
			//No labels in Light, no label menu
			if (HasFeature (featureFilterLabel)) {
				MyGetLabel((*data)->color,&color,s);
				if (ThereIsColor)
				{
					GetForeColor(&oldColor);
					RGBForeColor(&color);
				}
				DrawString(s);
				if (ThereIsColor) RGBForeColor(&oldColor);
			}
			break;
			
		case faeWrite:
			NumToString((*data)->color,s);
			err = FWriteKey(*(short*)dataPtr,flkLabel,s);
			break;
		
		case faeFind:
			//No labels in Light, no label menu
			if (HasFeature (featureFilterLabel)) {
				MyGetLabel((*data)->color,&color,s);
				err = PFindSub(dataPtr,s)!=nil;
			}
			break;

#ifdef FANCY_FILT_LDEF
		case faeListDraw:
			//No labels in Light, no label menu
			if (HasFeature (featureFilterLabel)) {
				MyGetLabel((*data)->color,&color,s);
				if (ThereIsColor && !BlackWhite(&color)) RGBForeColor(DarkenColor(&color,GetRLong(TEXT_DARKER)));
			}
			break;
#endif
			
		case faeDo:
			//No labels in Light, no label menu
			if (HasFeature (featureFilterLabel)) {
				UseFeature (featureFilterLabel);
				SetSumColor(fpb->tocH,fpb->sumNum,(*data)->color);
			}
			break;
	}
	return(err);
}


/**********************************************************************
 * FAflkPersonality - set the label for a message
 **********************************************************************/
typedef struct FDflkPers **FDflkPersHandle;
typedef struct FDflkPers
{
	long persId;
	Rect r;
	ControlHandle popup;
} FDflkPers;
short FAflkPersonality(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	OSErr err = noErr;
	Str255 s;
	FilterPBPtr fpb;
	PersHandle pers;
	uLong persId;
	Rect localR;
	FDflkPersHandle data;
	FDflkPers f;
	short item;
	
	if (!HasFeature (featureFilterPersonality))
		return (err);
	
	fpb = (FilterPBPtr)dataPtr;
	data = (FDflkPersHandle)(*action)->data;
	
	if (data)
	{
		persId = (uLong)(*data)->persId;
		pers = PERS_FORCE(FindPersById(persId));
		f = **data;
	}
		
	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_PERS_POP,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkPersonality(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			**data = f;
			FAflkPersonality(faeMouseGoingDown,action,r,dataPtr);
			item = Pers2Index(pers);
			if (item>0) item+=2;
			else item+=1;
			MySetCtlValue(f.popup,item);
			EmbedControl(f.popup,ActionGroup);
			// fall-through
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;
			
		case faeDraw:
			f = **data;
			if (f.popup)
			{
				MenuHandle mh = GetMHandle(PERS_HIER_MENU);
				CGrafPtr	WinPort = GetMyWindowCGrafPtr (Win);
				EnableItem(mh,0);
				Update1Control(f.popup,MyGetPortVisibleRegion(WinPort));
#ifdef GRIDLINES
				ControlGridlines(f.popup);
#endif
			}
			break;

		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;
			
		case faeButton:
			f = **data;
			item = Pers2Index(pers);
			if (item) item += 2;
			else item++;
			if (item!=GetControlValue(f.popup))
			{
				item = GetControlValue(f.popup);
				item--;
				if (item) item--;
				f.persId = (*Index2Pers(item))->persId;
				Win->isDirty = CurrentDirty = True;
				**data = f;
			}
			break;
		
		case faeMouseGoingDown:
		{
			MenuHandle mh;
			Str255 string;
			f = **data;
			if (f.popup && (mh = GetControlPopupMenuHandle(f.popup)))
			{
				TrashMenu(mh,3);
				for (pers=(*PersList)->next;pers;pers=(*pers)->next)
					MyAppendMenu(mh,PCopy(string,(*pers)->name));
				SetItemMark(mh,1,noMark);
				SetItemMark(mh,GetControlValue(f.popup),diamondChar);
				EnableItem(mh,0);
				SetControlMaximum(f.popup,CountMenuItems(mh));
			}
		}
			
		
		case faeHelp:
			FAPopActRect(r,&localR);
			if (MouseInRect(&localR))
			{
				*r = localR;
				GetIndString(dataPtr,526,flkhPersonality);
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkPers);	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr && *(UPtr)dataPtr) persId = (*PERS_FORCE(FindPersByName(dataPtr)))->persId;
			else persId = 0;
			(*data)->persId = persId;
			break;
			
		case faePrint:
			PSCopy(s,(*pers)->name);
			DrawString(s);
			break;
			
		case faeWrite:
			if (pers==PersList) *s = 0;
			else PSCopy(s,(*pers)->name);
			if (*s)
				err = FWriteKey(*(short*)dataPtr,flkPersonality,s);
			else
				err = FWriteBool(*(short*)dataPtr,flkPersonality,True);
			break;
		
		case faeFind:
			PSCopy(s,(*pers)->name);
			err = PFindSub(dataPtr,s)!=nil;
			break;
			
		case faeDo:
			//Enhanced Filters	- the personalities action is not supported in Light
			if (HasFeature (featureMultiplePersonalities)) {
				if (pers != PersList)
					UseFeature (featureFilterPersonality);
				SetPers(fpb->tocH,fpb->sumNum,pers,False);
			}
			break;
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkSound **FDflkSoundHandle;
typedef struct FDflkSound
{
	ControlHandle popup;
	Str255 name;
} FDflkSound;
short FAflkSound(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkSoundHandle data;
	FDflkSound f;
	Str255 s;
	OSErr err = noErr;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	short item;
	MenuHandle	mh;
	
	data = (void*)(*action)->data;
		
	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_SOUND_CNTL,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkSound(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}

			mh = (MenuHandle)GetControlPopupMenuHandle(f.popup);
			AddSoundsToMenu(mh);
			SetControlMaximum(f.popup,CountMenuItems(mh));
			item = FindItemByName(mh,f.name);
			if (!item) {
				item = 1;
				GetMenuItemText(mh,item,f.name);
			}

			MySetCtlValue(f.popup,item);
			EmbedControl(f.popup,ActionGroup);
			**data = f;
			/* fall-through */
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;
			

		case faeHelp:
			if (MouseInControl((*data)->popup))
			{
				GetControlBounds((*data)->popup,r);
				GetIndString(dataPtr,526,flkhSound);
			}
			break;

		case faeDraw:
			break;
			
		case faeRead:
			data = NewZH(FDflkSound);	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr) PCopy((*data)->name,(UPtr)dataPtr);
			break;
			
		case faeWrite:
			PCopy(f.name,(*data)->name);
			if (*f.name) err = FWriteKey(*(short*)dataPtr,flkSound,f.name);
			break;
		
		case faeFind:
			PCopy(f.name,(*data)->name);
			err = PFindSub(dataPtr,f.name)!=nil;
			break;
			
		case faeButton:
			f = **data;
			item = GetControlValue(f.popup);
			GetMenuItemText((MenuHandle)GetControlPopupMenuHandle(f.popup),item,s);
			if (*s) PlayNamedSound(s);
			if (!StringSame(f.name,s))
			{
				Win->isDirty = CurrentDirty = True;
				PCopy(f.name,s);
				**data = f;
			}
			break;

		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;
		
		case faePrint:
			f = **data;
			DrawString(f.name);
			break;

		case faeDo:
			//Enhanced Filters	- the sound filter action is not supported in Light
			if (HasFeature (featureFilterSound)) {
				UseFeature (featureFilterSound);
				f = **data;
				if (*f.name) PlayNamedSound(f.name);
			}
			break;
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkOpen **FDflkOpenHandle;
typedef struct FDflkOpen
{
	long flags;
	ControlHandle message;
	ControlHandle mailbox;
} FDflkOpen;
short FAflkOpenMessage(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkOpenHandle data = (FDflkOpenHandle)(*action)->data;
	FDflkOpen f;
	Rect controlR;
	short spacing;
	short val;
	WindowPtr	WinWP;
	ControlHandle button = (ControlHandle)dataPtr;
	Str63 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	long flag;
	
	switch (callType)
	{
		case faeInit:
			WinWP = GetMyWindowWindowPtr (Win);
			f.mailbox = GetNewControlSmall(OPEN_BOX_CNTL,WinWP);
			f.message = GetNewControlSmall(OPEN_MESSAGE_CNTL,WinWP);
			if (!f.mailbox || !f.message)
			{
				FAflkOpenMessage(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			f.flags = (*data)->flags;
			**data = f;
			MySetCtlValue(f.mailbox,0!=(f.flags&afbOpenMailbox));
			MySetCtlValue(f.message,0!=(f.flags&afbOpenMessage));
			EmbedControl(f.mailbox,ActionGroup);
			EmbedControl(f.message,ActionGroup);
			/* fall-through */
		case faeResize:
			f = **data;
			ButtonFit(f.mailbox);
			ButtonFit(f.message);
			FAMultRect(r,&controlR,&spacing,ControlHi(f.mailbox),2);
			MoveMyCntl(f.mailbox,controlR.left,controlR.top,RectWi(controlR),0);
			OffsetRect(&controlR,RectWi(controlR)+spacing,0);
			MoveMyCntl(f.message,controlR.left,controlR.top,RectWi(controlR),0);
			ShowControl(f.message);
			ShowControl(f.mailbox);
			break;
			
		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.message) DisposeControl(f.message);
				if (f.mailbox) DisposeControl(f.mailbox);
				f.mailbox = f.message = nil;
				**data = f;
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkOpen);	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr) StringToNum(dataPtr,&f.flags);
			else f.flags = 0;
			(*data)->flags = f.flags;
			break;
		
		case faePrint:
			if ((*data)->flags&afbOpenMailbox)
			{
				DrawRString(MAILBOX_PRINT);
				if ((*data)->flags&afbOpenMessage)
				{
					DrawRString(AND_PRINT);
					DrawRString(MESSAGE_PRINT);
				}
			}
			else if ((*data)->flags&afbOpenMessage)
			{
				DrawRString(MESSAGE_PRINT);
			}
			break;
			
		case faeWrite:
			NumToString((*data)->flags,s);
			return(FWriteKey(*(short*)dataPtr,(*action)->action,s));
			break;
		
		case faeButton:
			f = **data;
			val = !GetControlValue(button);
			MySetCtlValue(button,val);
			flag = (button==f.mailbox) ? afbOpenMailbox : afbOpenMessage;
			if (val)
				(*data)->flags |= flag;
			else
				(*data)->flags &= ~flag;
			break;
			
		case faeHelp:
			f = **data;
			if (MouseInControl(f.mailbox))
			{
				GetIndString(dataPtr,526,flkhOpenBox);
				GetControlBounds(f.mailbox,r);
			}
			if (MouseInControl(f.message))
			{
				GetIndString(dataPtr,526,flkhOpenMess);
				GetControlBounds(f.message,r);
			}
			break;

		case faeDo:
			//Enhanced Filters	- the open message filter action is not supported in Light
			if (HasFeature (featureFilterOpen)) {
				UseFeature (featureFilterOpen);
				fpb->openMailbox = 0!=((*data)->flags&afbOpenMailbox);
				fpb->openMessage = 0!=((*data)->flags&afbOpenMessage);
				ComposeLogS(LOG_FILT,nil,"\pOpen %d %d",fpb->openMailbox,fpb->openMessage);
			}
			break;
	}
	return(noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkPrior **FDflkPriorHandle;
typedef struct FDflkPrior
{
	long prior;
	Rect r;
	ControlHandle popup;
} FDflkPrior;

short FAflkPriority(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkPriorHandle data;
	FDflkPrior f;
	Str255 s;
	OSErr err = noErr;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	Rect ir;
	short item;
	Point	penLoc;
	
	data = (void*)(*action)->data;
		
	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_PRIORITY_POP,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkStatus(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			item = f.prior ? f.prior : 3;
			
			MySetCtlValue(f.popup,item);
			**data = f;
			EmbedControl(f.popup,ActionGroup);
			// fall-through
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;

		case faeHelp:
			f.r = (*data)->r;
			if (MouseInRect(&f.r))
			{
				*r = f.r;
				GetIndString(dataPtr,526,flkhPriority);
			}
			break;
						
		case faeDraw:
			f = **data;
			if (f.popup)
			{
				MenuHandle mh = GetMHandle(PRIOR_HIER_MENU);
				CGrafPtr	WinPort = GetMyWindowCGrafPtr (Win);
				EnableItem(mh,0);
				Update1Control(f.popup,MyGetPortVisibleRegion(WinPort));
#ifdef GRIDLINES
				ControlGridlines(f.popup);
#endif
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkPrior);	/* if we can't find a dozen bytes, we're dead */
			if (!data) return(MemError());
			(*action)->data = (void*)data;
			if (dataPtr)
			{
				StringToNum(dataPtr,&f.prior);
				(*data)->prior = f.prior;
			}
			break;
		
		case faeFind:
			GetRString(s,PRIOR_STRN+(*data)->prior);
			err = PFindSub(dataPtr,s)!=nil;
			break;
			
		case faeWrite:
			NumToString((*data)->prior,s);
			err = FWriteKey(*(short*)dataPtr,flkPriority,s);
			break;
	
		case faeButton:
			f = **data;
			item = GetControlValue(f.popup);
			if (item!=f.prior)
			{
				f.prior = item;
				Win->isDirty = CurrentDirty = True;
				**data = f;
			}
			break;
		
		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;

		case faePrint:
			f = **data;
			GetPortPenLocation(GetQDGlobalsThePort(),&penLoc);
			ir.top = penLoc.v-10;
			ir.left = penLoc.h;
			ir.bottom = ir.top +16;
			ir.right = ir.left+16;
			PlotIconID(&ir,atAbsoluteCenter, ttNone,PRIOR_SICN_BASE+f.prior-1);
			break;
			
		case faeDo:
			SetPriority(fpb->tocH,fpb->sumNum,NewPrior((*data)->prior,
									(*fpb->tocH)->sums[fpb->sumNum].priority));
			break;
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkForward **FDflkForwardHandle;
typedef struct FDflkForward
{
	PETEHandle pte;
	Str255 addresses;
}	FDflkForward;

short FAflkForward(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkForwardHandle data = (FDflkForwardHandle)(*action)->data;
	FDflkForward f;
	Rect textR;
	OSErr err = noErr;
	Str255 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;

	switch (callType)
	{
		case faeInit:
			if (err=PeteCreate(Win,&f.pte,0,nil)) return(WarnUser(PETE_ERR,err));
			(*PeteExtra(f.pte))->frame = True;
			(*PeteExtra(f.pte))->infinitelyWide = True;
			PeteFontAndSize(f.pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
			PCopy(s,(*data)->addresses);
			PeteSetString(s,f.pte);
			(*data)->pte = f.pte;
			PETESetCallback(PETE,f.pte,(void*)FilterChanged,peDocChanged);
			/* fall through */
		case faeResize:
			FASingleRect(r,&textR,ONE_LINE_HI(Win));
			PeteDidResize((*data)->pte,&textR);
			break;

		case faeNew:
			PeteFocus(Win,(*data)->pte,true);
			break;
			
		case faeDraw:
			break;
			
		case faeDispose:
		case faeClose:
			FAflkSubject(callType,action,r,dataPtr);
			break;
						
		case faeFind:
			err = PeteFindString(dataPtr,0,(*data)->pte)>=0;
			break;

		case faeCursor:
			if (PtInPETE(*(Point*)r,(*data)->pte))
				*(short*)dataPtr = iBeamCursor;
			else
				*(short*)dataPtr = arrowCursor;
			break;

		case faeRead:
			data = NewZH(FDflkForward);
			if (!data) return(WarnUser(MEM_ERR,MemError()));
			else
			{
				(*action)->data = (void*)data;
				if (dataPtr) PCopy((*data)->addresses,(UPtr)dataPtr);
			}
			break;
			
		case faeWrite:
			PCopy(s,(*data)->addresses);
			if (*s) err = FWriteKey(*(short*)dataPtr,(*action)->action,s);
			break;

		case faePrint:
			PCopy(s,(*data)->addresses);
			DrawString(s);
			break;
			
		case faeClick:
			if (PtInPETE(*(Point*)r,(*data)->pte))
			{
				PeteEditWFocus(Win,(*data)->pte,peeEvent,dataPtr,nil);
				err = True;	/* actually, not an error, but... */
			}
			break;
			break;
	
		case faeSave:
				PeteSString(s,(*data)->pte);
				PCopy((*data)->addresses,s);
			break;
		
		case faeHelp:
			if (MouseInPTE((*data)->pte))
			{
				PeteRect((*data)->pte,r);
				GetIndString(dataPtr,526,((*action)->action==flkForward?flkhForward:flkhRedirect));
			}
			break;
		
		case faeDo:
			//Enhanced Filters	- the forward filter action is not supported in Light
			//if ((*fpb->tocH)->which==OUT) return(noErr);	/* don't do for out */
			if (HasFeature (featureFilterForward)) {
				if (((*fpb->tocH)->sums[fpb->sumNum].opts&OPT_BULK)&&!PrefIsSet(PREF_BOMBS_AWAY)) return(noErr); /* don't do for bulk mail */
				PCopy(s,(*data)->addresses);
				DoFordirectMessage(fpb->tocH,fpb->sumNum,(*action)->action,s,True);
				UseFeature (featureFilterForward);
			}
			break;

	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
short FAflkRedirect(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(FAflkForward(callType,action,r,dataPtr));
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkReply **FDflkReplyHandle;
typedef struct FDflkReply
{
	Str31 name;
	Rect r;
	ControlHandle popup;
}	FDflkReply;
short FAflkReply(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	OSErr err = noErr;
	Str255 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	FDflkReplyHandle data = (FDflkReplyHandle)(*action)->data;
	short item;
	MenuHandle mh;
	FDflkReply f;

	switch (callType)
	{
		case faeInit:
			f = **data;
			f.popup = GetNewControlSmall(FILT_REPLY_POP,GetMyWindowWindowPtr(Win));
			if (!f.popup)
			{
				FAflkReply(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			**data = f;
			FAflkReply(faeMouseGoingDown,action,r,dataPtr);
			mh = GetControlPopupMenuHandle(f.popup);
			item = FindItemByName(mh,f.name);
			if (!item)
			{
				item = 1;
				MyGetItem(mh,item,f.name);
				PCopy((*data)->name,f.name);
			}
			MySetCtlValue(f.popup,item);
			EmbedControl(f.popup,ActionGroup);
			// fall-through
			
		case faeResize:
			f = **data;
			FAPopupValueFit(r,f.popup);
			ShowControl(f.popup);
			break;

		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.popup) DisposeControl(f.popup);
				f.popup = nil;
				**data = f;
			}
			break;
			
		case faeButton:
			f = **data;
			mh = GetMHandle(REPLY_WITH_HIER_MENU);
			item = FindItemByName(mh,f.name);
			if (item!=GetControlValue(f.popup))
			{
				item = GetControlValue(f.popup);
				MyGetItem(mh,item,f.name);
				Win->isDirty = CurrentDirty = True;
				**data = f;
			}
			break;
		
		case faeMouseGoingDown:
		{
			Str255 string;
			f = **data;
			if (f.popup && (mh = GetControlPopupMenuHandle(f.popup)))
			{
				MenuHandle otherMH = GetMHandle(NEW_WITH_HIER_MENU);
				short n;
				
				if (otherMH)
				{
					TrashMenu(mh,1);
					n = CountMenuItems(otherMH);
					for (item=1;item<=n;item++)
					{
						MyGetItem(otherMH,item,string);
						MyAppendMenu(mh,string);
					}
					SetItemMark(mh,GetControlValue(f.popup),diamondChar);
					EnableItem(mh,0);
					SetControlMaximum(f.popup,CountMenuItems(mh));
				}
			}
			break;
		}
			
		case faeFind:
			PCopy(s,(*data)->name);
			err = PFindSub(dataPtr,s)!=nil;
			break;

		case faeRead:
			data = NewZH(FDflkReply);
			if (!data) return(WarnUser(MEM_ERR,MemError()));
			else
			{
				(*action)->data = (void*)data;
				if (dataPtr) PSCopy((*data)->name,(UPtr)dataPtr);
			}
			break;
			
		case faeWrite:
			PCopy(s,(*data)->name);
			if (*s) err = FWriteKey(*(short*)dataPtr,(*action)->action,s);
			break;

		case faePrint:
			PCopy(s,(*data)->name);
			DrawString(s);
			break;
				
		case faeDo:
			//We don't have a reply with menu, so we can't do this action in LIGHT
			if (HasFeature (featureFilterReply)) {
				short i;
				MenuHandle mh = GetMHandle(REPLY_WITH_HIER_MENU);
				
				//if ((*fpb->tocH)->which==OUT) return(noErr);	/* don't do for out */
				if (((*fpb->tocH)->sums[fpb->sumNum].opts&OPT_BULK)&&!PrefIsSet(PREF_BOMBS_AWAY)) return(noErr); /* don't do for bulk mail */
				PCopy(s,*(*action)->data);
				if (mh && (i=FindItemByName(mh,s)))
				{
					UseFeature (featureFilterReply);
					DoReplyClosed(fpb->tocH,fpb->sumNum,False,False,False,True,i,True,True);
				}
			}
			break;

	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
short FAflkNotifyApp(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
typedef struct FDflkNUser **FDflkNUserHandle;
typedef struct FDflkNUser
{
	long flags;
	ControlHandle user;
	ControlHandle report;
} FDflkNUser;
short FAflkNotifyUser(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkNUserHandle data = (FDflkNUserHandle)(*action)->data;
	FDflkNUser f;
	Rect controlR;
	short spacing;
	short val;
	WindowPtr	WinWP;
	ControlHandle button = (ControlHandle)dataPtr;
	Str63 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	long flag;
	
	switch (callType)
	{
		case faeInit:
			WinWP = GetMyWindowWindowPtr (Win);
			f.report = GetNewControlSmall(DO_FILTER_CNTL,WinWP);
			f.user = GetNewControlSmall(DO_USER_CNTL,WinWP);
			if (!f.user || !f.report)
			{
				FAflkNotifyUser(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			f.flags = (*data)->flags;
			**data = f;
			MySetCtlValue(f.user,0!=(f.flags&afbUser));
			MySetCtlValue(f.report,0!=(f.flags&afbReport));
			EmbedControl(f.report,ActionGroup);
			EmbedControl(f.user,ActionGroup);
			/* fall-through */
		case faeResize:
			f = **data;
			ButtonFit(f.user);
			ButtonFit(f.report);
			FAMultRect(r,&controlR,&spacing,ControlHi(f.user),2);
			MoveMyCntl(f.user,controlR.left,controlR.top,RectWi(controlR),0);
			OffsetRect(&controlR,RectWi(controlR)+spacing,0);
			MoveMyCntl(f.report,controlR.left,controlR.top,RectWi(controlR),0);
			ShowControl(f.user);
			ShowControl(f.report);
			break;
			
		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.user) DisposeControl(f.user);
				if (f.report) DisposeControl(f.report);
				f.user = f.report = nil;
				**data = f;
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkNUser);	/* if we can't find a dozen bytes, we're dead */
			(*action)->data = (void*)data;
			if (dataPtr) StringToNum(dataPtr,&f.flags);
			else f.flags = afbUser | (PrefIsSet(PREF_REPORT)?afbReport:0);
			(*data)->flags = f.flags;
			break;
			
		case faeWrite:
			NumToString((*data)->flags,s);
			return(FWriteKey(*(short*)dataPtr,(*action)->action,s));
			break;
		
		case faeButton:
			f = **data;
			val = !GetControlValue(button);
			MySetCtlValue(button,val);
			Win->isDirty = CurrentDirty = True;
			flag = (button==f.user) ? afbUser : afbReport;
			if (val)
				(*data)->flags |= flag;
			else
				(*data)->flags &= ~flag;
			break;
		
		case faePrint:
			if ((*data)->flags&afbUser)
			{
				DrawRString(NOTIFY_NORM);
				if ((*data)->flags&afbReport)
				{
					DrawRString(AND_PRINT);
					DrawRString(NOTIFY_REPORT);
				}
			}
			else if ((*data)->flags&afbReport)
			{
				DrawRString(NOTIFY_REPORT);
			}
			break;
			
			
		case faeDo:
			if (0==((*data)->flags&afbUser))
				fpb->dontUser = True;
			fpb->doReport = 0!=((*data)->flags&afbReport);
			fpb->dontReport = !fpb->doReport;
			break;
		
		case faeHelp:
			f = **data;
			if (MouseInControl(f.user))
			{
				GetIndString(dataPtr,526,flkhNUser);
				GetControlBounds(f.user,r);
			}
			if (MouseInControl(f.report))
			{
				GetIndString(dataPtr,526,flkhNReport);
				GetControlBounds(f.report,r);
			}
			break;
	}
	return(noErr);
}

/**********************************************************************
 * FAflkServerOpts - set server options
 **********************************************************************/
typedef struct FDflkSOpt **FDflkSOptHandle;
typedef struct FDflkSOpt
{
	long flags;
	ControlHandle fetch;
	ControlHandle trash;
} FDflkSOpt;
short FAflkServerOpts(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	FDflkSOptHandle data = (FDflkSOptHandle)(*action)->data;
	FDflkSOpt f;
	Rect controlR;
	short spacing;
	short val;
	WindowPtr	WinWP;
	ControlHandle button = (ControlHandle)dataPtr;
	Str63 s;
	FilterPBPtr fpb = (FilterPBPtr)dataPtr;
	long flag;
	long uidHash;
	PersHandle pers;
	Boolean lmos;
	
	switch (callType)
	{
		case faeInit:
			WinWP = GetMyWindowWindowPtr (Win);
			f.fetch = GetNewControlSmall(DO_FETCH_CNTL,WinWP);
			f.trash = GetNewControlSmall(DO_TRASH_CNTL,WinWP);
			if (!f.fetch || !f.trash)
			{
				FAflkServerOpts(faeDispose,action,nil,nil);
				return(MEM_ERR);
			}
			f.flags = (*data)->flags;
			**data = f;
			MySetCtlValue(f.trash,0!=(f.flags&afbTrash));
			MySetCtlValue(f.fetch,0!=(f.flags&afbFetch));
			lmos = PrefIsSet(PREF_LMOS);
			if (!lmos&&(f.flags&afbFetch))
			{
				HiliteControl(f.trash,255);
				(*data)->flags |= afbTrash;
				MySetCtlValue(f.trash,1);
			}
			else HiliteControl(f.trash,0);
			EmbedControl(f.trash,ActionGroup);
			EmbedControl(f.fetch,ActionGroup);
			/* fall-through */
		case faeResize:
			f = **data;
			ButtonFit(f.fetch);
			ButtonFit(f.trash);
			FAMultRect(r,&controlR,&spacing,ControlHi(f.fetch),2);
			MoveMyCntl(f.fetch,controlR.left,controlR.top,RectWi(controlR),0);
			OffsetRect(&controlR,RectWi(controlR)+spacing,0);
			MoveMyCntl(f.trash,controlR.left,controlR.top,RectWi(controlR),0);
			ShowControl(f.fetch);
			ShowControl(f.trash);
			break;
			
		case faeClose:
		case faeDispose:
			if (data)
			{
				f = **data;
				if (f.trash) DisposeControl(f.trash);
				if (f.fetch) DisposeControl(f.fetch);
				f.fetch = f.trash = nil;
				**data = f;
			}
			break;
			
		case faeRead:
			data = NewZH(FDflkSOpt);	/* if we can't find a dozen bytes, we're dead */
			(*action)->data = (void*)data;
			if (dataPtr) StringToNum(dataPtr,&f.flags);
			else f.flags = 0;
			(*data)->flags = f.flags;
			break;
			
		case faeWrite:
			NumToString((*data)->flags,s);
			return(FWriteKey(*(short*)dataPtr,(*action)->action,s));
			break;
		
		case faeButton:
			f = **data;
			val = !GetControlValue(button);
			MySetCtlValue(button,val);
			Win->isDirty = CurrentDirty = True;
			flag = (button==f.trash) ? afbTrash : afbFetch;
			if (val)
				f.flags |= flag;
			else
				f.flags &= ~flag;
			**data = f;
			lmos = PrefIsSet(PREF_LMOS);
			if (!lmos&&(f.flags&afbFetch))
			{
				HiliteControl(f.trash,255);
				(*data)->flags |= afbTrash;
				MySetCtlValue(f.trash,1);
			}
			else HiliteControl(f.trash,0);
			break;
			
		case faeHelp:
			f = **data;
			if (MouseInControl(f.fetch))
			{
				GetIndString(dataPtr,526,flkhFetch);
				GetControlBounds(f.fetch,r);
			}
			else if (MouseInControl(f.trash))
			{
				GetIndString(dataPtr,526,flkhDelete);
				GetControlBounds(f.trash,r);
			}
			break;

		case faeDo:
			//Enhanced Filters	- the Server Options filter actions is not supported in Light
			if (HasFeature (featureFilterServerOptions)) {
				UseFeature (featureFilterServerOptions);
				flag = (*data)->flags;
				uidHash = (*fpb->tocH)->sums[fpb->sumNum].uidHash;
				pers = TS_TO_PPERS(fpb->tocH,fpb->sumNum);
#ifdef	IMAP
				if ((*fpb->tocH)->imapTOC)
				{
					// This is an IMAP mailbox.
					
					Boolean filteringUnderway = IMAPFilteringUnderway();
					
					// Make sure progress is displayed while doing the next bit ...
					if (filteringUnderway) IMAPStopFiltering(false);
					
					if (flag&afbFetch)
					{
						// Server Options -> Fetch.  Download this message if it's not already present.	
						if (!IMAPMessageDownloaded(fpb->tocH, fpb->sumNum) && !IMAPMessageBeingDownloaded(fpb->tocH, fpb->sumNum))
							UIDDownloadMessage(fpb->tocH, uidHash, true, true);
					}
					else if (flag&afbTrash)
					{
						// Server Options -> Delete.  Delete this message from the IMAP server.
						IMAPDeleteMessageDuringFiltering(fpb->tocH, pers, (*fpb->tocH)->sums[fpb->sumNum].uidHash);
					}
					
					// go back to filtering
					if (filteringUnderway) IMAPStartFiltering(fpb->tocH, true);
				}
				else
				{
					// This is a regular mailbox.
#endif
					//if (IdIsOnPOPD(PERS_POPD_TYPE(pers),POPD_ID,uidHash)) // because of incremental filtering, we may not know it's on the server when we do this
					{
						Boolean skipped = ((*fpb->tocH)->sums[fpb->sumNum].flags & FLAG_SKIPPED)!=0;
						if ((flag&afbFetch) && skipped) AddIdToPOPD(PERS_POPD_TYPE(pers),FETCH_ID,uidHash,False);
						if (flag&afbTrash)
						{
							AddIdToPOPD(PERS_POPD_TYPE(pers),DELETE_ID,uidHash,False);
							TOCSetDirty(fpb->tocH,true);
						}
						if ((*fpb->tocH)->win) Fix1MessServerArea((*fpb->tocH)->win);
						if ((*fpb->tocH)->sums[fpb->sumNum].messH)
							Fix1MessServerArea((*(*fpb->tocH)->sums[fpb->sumNum].messH)->win);
					}
#ifdef	IMAP
				}
#endif
			}
			break;

		case faePrint:
			if ((*data)->flags&afbFetch)
			{
				DrawRString(FETCH_PRINT);
				if ((*data)->flags&afbTrash)
				{
					DrawRString(AND_PRINT);
					DrawRString(DELETE_PRINT);
				}
			}
			else if ((*data)->flags&afbTrash)
			{
				DrawRString(DELETE_PRINT);
			}
			break;

	}
	return(noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
short FAflkAttachments(FACallEnum callType,FActionHandle action,Rect *r,void *dataPtr)
{
	return(noErr);
}


#ifndef SPEECH_ENABLED
short FAflkSpeak (FACallEnum callType, FActionHandle action, Rect *r, void *dataPtr)
{
//	return noSynthFound;
	return noErr;
}
#else
typedef struct FDflkSpeak **FDflkSpeakHandle;
typedef struct FDflkSpeak
{
	SpeakableParts	speak;
	ControlHandle		popup;
	ControlHandle		sender;
	ControlHandle 	subject;
	VoiceSpec				voice;
} FDflkSpeak;
/**********************************************************************
 * FAflkSpeak - speak the sender and/or subject
 **********************************************************************/
short FAflkSpeak (FACallEnum callType, FActionHandle action, Rect *r, void *dataPtr)

{
	FDflkSpeakHandle	data;
	FDflkSpeak				filter;
	FilterPBPtr				fpb;
	VoiceSpec					voice;
	WindowPtr					WinWP;
	ControlHandle			theControl;
	Str255						sample;
	Str63							string;
	Rect							controlR;
	UPtr 							spot;
	OSType						creator,
										id;
	long							speak;
	short							spacing,
										value;
	Rect				rCntl;
	SpeakableParts		iHaveSpoken;
	
	data = (FDflkSpeakHandle) (*action)->data;

	switch (callType) {
		case faeInit:
			WinWP = GetMyWindowWindowPtr (Win);
			filter = **data;
			filter.speak = (*data)->speak;
			filter.sender	 = GetNewControlSmall (FILT_SPEAK_SENDER_CNTL, WinWP);
			filter.subject = GetNewControlSmall (FILT_SPEAK_SUBJECT_CNTL, WinWP);
			filter.popup	 = GetNewVoicePopupSmall (FILT_SPEAK_POPUP_CNTL, WinWP, &filter.voice);
			**data = filter;
			if (!filter.sender || !filter.subject) {
				FAflkSpeak (faeDispose, action, nil, nil);
				return (MEM_ERR);
			}
			MySetCtlValue (filter.sender, (filter.speak & speakSender) != 0);
			MySetCtlValue (filter.subject, (filter.speak & speakSubject) != 0);
			EmbedControl(filter.sender,ActionGroup);
			EmbedControl(filter.subject,ActionGroup);
			EmbedControl(filter.popup,ActionGroup);

			/* fall-through */
		case faeResize:
			filter = **data;
			ButtonFit (filter.sender);
			ButtonFit (filter.subject);
			FAMultRect (r, &controlR, &spacing, ControlHi (filter.sender), 2);
			if (filter.popup) {
				ButtonFit (filter.popup);
				GetControlBounds(filter.popup,&rCntl);
				MoveMyCntl (filter.popup, controlR.left, controlR.top, MIN (RectWi (rCntl), RectWi (controlR)), 0);
				OffsetRect (&controlR, RectWi (controlR) + spacing, 0);
				controlR.right = controlR.left + RectWi (controlR) / 2;
			}
			MoveMyCntl (filter.sender, controlR.left, controlR.top, RectWi (controlR), 0);
			OffsetRect (&controlR, RectWi (controlR) + spacing, 0);
			MoveMyCntl (filter.subject, controlR.left, controlR.top, RectWi (controlR), 0);
			if (filter.popup)
				ShowControl (filter.popup);
			ShowControl (filter.sender);
			ShowControl (filter.subject);
			break;

		case faeClose:
		case faeDispose:
			if (data) {
				filter = **data;
				if (filter.sender)
					DisposeControl (filter.sender);
				if (filter.subject)
					DisposeControl (filter.subject);
				if (filter.popup)
					DisposeControl (filter.popup);
				filter.sender = filter.subject = filter.popup = nil;
				**data = filter;
			}
			break;
			
		case faeRead:
			// Stored as <speakable parts> <voice creator> <voice id>
			data = NewZH (FDflkSpeak);	/* if we can't find a dozen or so bytes, we're dead */
			(*action)->data = (void *) data;
			if (dataPtr) {
				spot = (UPtr) dataPtr + 1;
				if (PToken (dataPtr, string, &spot, " ")) MyStringToNum (string,&speak);
				if (PToken (dataPtr, string, &spot, " ")) MyStringToNum (string,&creator);
				if (PToken (dataPtr, string, &spot, " ")) MyStringToNum (string,&id);
			}
			else {
				speak		= speakNothing;
				creator = 0;
				id			= 0;
			}
			(*data)->speak = speak;
			if (SpeechAvailable ())
				(void) MakeVoiceSpec (creator, id, &(*data)->voice);
			break;
			
		case faeWrite:
			// Written as <speakable parts> <voice creator> <voice id>
			NumToString ((*data)->speak, string);
			PLCat (string, (*data)->voice.creator);
			PLCat (string, (*data)->voice.id);
			return (FWriteKey (*(short *) dataPtr, (*action)->action, string));
			break;
			
		case faeButton:
			filter = **data;
			theControl = (ControlHandle) dataPtr;
			value = GetControlValue (theControl);
			
			Win->isDirty = CurrentDirty = True;
			if (theControl == filter.popup) {
				GetMenuItemText (GetControlPopupMenuHandle(filter.popup), value, string);
				if (*string && !FindVoiceFromName (string, &voice)) {
					(*data)->voice = voice;
					GetRString (sample, SPEAK_SAMPLE_TEXT);
					(void) Speak (&voice, sample + 1, *sample);
				}
			}
			else {
				MySetCtlValue (theControl, !value);
				speak = (theControl == filter.sender) ? speakSender : speakSubject;
				if (!value)
					(*data)->speak |= speak;
				else
					(*data)->speak &= ~speak;
			}
			break;
		
		case faePrint:
			filter = **data;
			if (filter.speak) {
				iHaveSpoken = false;
				if (filter.speak & speakSender) {
					DrawRString (SPEAK_SENDERS_NAME);
					iHaveSpoken = true;
				}
				if (filter.speak & speakSubject) {
					if (iHaveSpoken)
						DrawRString(AND_PRINT);
					DrawRString (SPEAK_SUBJECT);
					iHaveSpoken = true;
				}
			}
			else
				DrawRString (SPEAK_NOTHING);
			DrawRString (SPEAK_USING);
			if (GetVoiceName (&filter.voice, string))
				DrawRString (SPEAK_DEFAULT_VOICE);
			else
				DrawString (string);
			break;
			
		case faeDo:
			//Enhanced Filters	- the speak filter action is not supported in Light
			if (HasFeature (featureFilterSpeak)) {
				UseFeature (featureFilterSpeak);
				filter = **data;
				fpb = (FilterPBPtr) dataPtr;

				//	Speak now or forever hold your peace
				if (filter.speak)
					(void) SpeakMessage (&filter.voice, fpb->tocH, fpb->sumNum, filter.speak | speakSummary, false);
			}
			break;
		
		case faeHelp:
			filter = **data;
			if (MouseInControl (filter.popup)) {
				GetIndString (dataPtr, 526, flkhSpeak);
				GetControlBounds(filter.popup,r);
			}
			else
			if (MouseInControl (filter.sender)) {
				GetIndString (dataPtr, 526, flkhSpeakSender);
				GetControlBounds(filter.sender,r);
			}
			else
			if (MouseInControl (filter.subject))
			{
				GetIndString (dataPtr, 526, flkhSpeakSubject);
				GetControlBounds(filter.subject,r);
			}
			break;
	}
	return (noErr);
}
#endif

short FAflkAddHistory (FACallEnum callType, FActionHandle action, Rect *r, void *dataPtr)

{
	FilterPBPtr				fpb;
	
	switch (callType) {
		case faeInit:
		case faeResize:
		case faeClose:
		case faeDispose:
			break;
			
		case faeWrite:
			return(FWriteBool(*(short*)dataPtr,(*action)->action,True));
			break;
			
		case faeButton:
		case faePrint:
			break;
			
		case faeDo:
			//Enhanced Filters	- Add the sender to the nickname cache
			if (HasFeature (featureFilterAddHistory))
				if (fpb = (FilterPBPtr) dataPtr) {
					TextAddrHandle	addresses = nil;
					UseFeature (featureFilterAddHistory);
					if (!GatherBoxAddresses (fpb->tocH, 0, fpb->sumNum, fpb->sumNum, &addresses, false)) {
						CacheRecentNickname (LDRef (addresses));
						UL (addresses);
					}
				}
			break;
		
		case faeHelp:
			break;
	}
	return (noErr);
}

#ifdef GRIDLINES
/************************************************************************
 * ControlGridlines - draw gridlines where a control is
 ************************************************************************/
void ControlGridlines(ControlHandle cntl)
{
	Rect r;
	
	GetControlBounds(cntl,&r);
	MoveTo(r.left-10,r.top); Line(10,0); Line(0,-10);
	MoveTo(r.right+10,r.top); Line(-10,0); Line(0,-10);
	MoveTo(r.left-10,r.bottom); Line(10,0); Line(0,10);
	MoveTo(r.right+10,r.bottom); Line(-10,0); Line(0,10);
}
#endif


/**********************************************************************
 * RefreshFiltersWindow - bring it to the front and select the last
 *	filter if details is true.
 **********************************************************************/
void RefreshFiltersWindow(FilterRecord newFilter, Boolean details)
{
	Point c;
	short n = NFilters - 1;
		
	if (FLG && *FLG && Win)
	{
		PushGWorld();
		SetPort_(GetMyWindowCGrafPtr(Win));
		LAddRow(1,n,LHand);
		c.h = 0; c.v = n;
		LSetCell(newFilter.name+1,*newFilter.name,c,LHand);
		PopGWorld();
	}

	//open the filters window if the user hit "add details"
	if (details)
	{
		OpenFiltersWindow();
		PushGWorld();
		SetPort_(GetMyWindowCGrafPtr(Win));
		FilterSelect(n);
		PopGWorld();
	}
}

#endif