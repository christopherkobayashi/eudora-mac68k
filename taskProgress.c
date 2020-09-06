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

/*
	File: taskProgress.c
	Author: Clarence Wong <cwong@qualcomm.com>
	Date: Augutst 1997 - ...
	Copyright (c) 1997 by QUALCOMM Incorporated 

	Comments:
		Much of this code is borrowed from Alan Bird's stationerywin.c and Steve Dorner's progress.c
*/

#include "taskProgress.h"
#include "task_ldef.h"

// Task Progress controls
enum
{
	kNetworkPopup=0,
	kFilterButton,
	kStopButton,
	kHelpButton,
};

#define FILE_NUM 104		

#pragma segment TaskProgressWindow

static ControlHandle FilterButton = nil;

/************************************************************************
 * prototypes
 ************************************************************************/
static void TPDoDidResize(MyWindowPtr win, Rect *oldContR);
static void TPDoZoomSize(MyWindowPtr win,Rect *zoom);
static void TPDoUpdate(MyWindowPtr win);
static void TPDoActivate(MyWindowPtr win);
static Boolean TPDoClose(MyWindowPtr win);
static void TPDoButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
static void TPDoClick(MyWindowPtr win,EventRecord *event);
static void TPDoCursor(Point mouse);
static Boolean TPDoKey(MyWindowPtr win, EventRecord *event);
static void TPDoShowHelp(MyWindowPtr win,Point mouse);
static Boolean TPPosition(Boolean save, MyWindowPtr win);
static void TPIdle(MyWindowPtr win);

UPtr GetNextCheckTime (UPtr item);
UPtr GetLastCheckTime (UPtr item);
static void DrawTPProgress(MyWindowPtr win, Rect *lRect,Handle data);
static void DrawTPFilter(MyWindowPtr win, Rect *lRect,Handle data);
static void DrawTPError(MyWindowPtr win, Rect *lRect,Handle data);
static void PositionAllCells(void);
static void SetTPWTitle(void);
static pascal void FooterUserPaneBackground (ControlHandle control, ControlBackgroundPtr info);
static pascal void SetWindowBackground (ControlHandle control, ControlBackgroundPtr info);
OSErr TPAddHelpButton(taskErrHandle taskErrs);

#define kListInset 10
#define kTextInset 10
#define kCellIndent 4
#define PROG_BAR_WI 100
#define PROG_TOTAL_HI ((PROG_BOX_HI * 3) + INSET + (2*kCellIndent))
#define kRemainWidth 40
#define kLastMailCheckWidth	140
#define kNextMailCheckWidth 145
#define kNextTimeRefCon 'tpnx'
#define kLastTimeRefCon 'tpls'
#define kFooterRefCon	'tpft'
#define kNetworkRefCon 'tpnc'
#define kListPaneRefCon 'tplp'
#define kNetworkWidth 24
#define kNetworkHeight	20
#define PROG_WI (GetRLong(PROG_CONT_WIDTH) + (2*kListInset))	

ListHandle TaskListHandle = nil;
taskErrHandle TaskErrorList = nil;

/************************************************************************
 * InitPrbl
 ************************************************************************/
void InitPrbl(ProgressBlock **prbl,short vert,ControlHandle *stopButton)
{
	if (TaskProgressWindow)
	{
		short stopWidth = 0, 
					stopHeight = 0;
#ifdef DEBUG
		short mid;
#endif
		MyWindowPtr win = TaskProgressWindow;
		WindowPtr	winWP = GetMyWindowWindowPtr(win);
		Rect contR;
		ControlHandle listPaneCntl=FindControlByRefCon(TaskProgressWindow,kListPaneRefCon);
		
		ASSERT(listPaneCntl);
		LDRef(prbl);
		
		if (!(*stopButton))
		{
			if (*stopButton = GetNewControlSmall(TASK_PROGRESS_STOP_CNTL,winWP))
				if (listPaneCntl) EmbedControl(*stopButton,listPaneCntl);
//	    	SafeWazooControl(win,*stopButton,TASKS_WIN);
    }
		
		if (*stopButton)
		{
			ButtonFit(*stopButton);
			stopWidth = ControlWi(*stopButton);
			stopHeight = ControlHi(*stopButton);
		}
		
		contR = win->contR;
		InsetRect(&contR,kTextInset,kTextInset);
		
		// title && progbar
		SetRect(&(*prbl)->rects[kpTitle],contR.left+kListInset,vert,PROG_WI-PROG_BAR_WI-INSET-kListInset-GROW_SIZE,vert+PROG_BOX_HI);
		SetRect(&(*prbl)->rects[kpBar],PROG_WI-PROG_BAR_WI-INSET-GROW_SIZE-kListInset,vert,PROG_WI-INSET-GROW_SIZE-kListInset,vert+PROG_BAR_HI);
		if (!(*prbl)->bar)
		{
			if ((*prbl)->bar = NewControl(winWP,&(*prbl)->rects[kpBar], "", false, 0,0,100, kControlProgressBarProc, 0 ))
				if (listPaneCntl) EmbedControl((*prbl)->bar,listPaneCntl);	
//	    	SafeWazooControl(win,(*prbl)->bar,TASKS_WIN);
		}
		else
			MoveMyCntl((*prbl)->bar,(*prbl)->rects[kpBar].left,(*prbl)->rects[kpBar].top,0,0);
		vert += PROG_BOX_HI;

		// subtitle and remaining counter
		SetRect(&(*prbl)->rects[kpSubTitle],contR.left+INSET+kListInset,vert,PROG_WI-stopWidth-GROW_SIZE-INSET-kRemainWidth-kListInset,vert+PROG_BOX_HI);		
		SetRect(&(*prbl)->rects[kpRemaining],(*prbl)->rects[kpSubTitle].right+4,vert,PROG_WI-stopWidth-GROW_SIZE-INSET-kListInset,vert+PROG_BOX_HI);
		vert += PROG_BOX_HI;
		
		// message
		SetRect(&(*prbl)->rects[kpMessage],contR.left+INSET+kListInset,vert,PROG_WI-stopWidth-GROW_SIZE-INSET-kListInset,vert+PROG_BOX_HI);
#ifdef DEBUG
		mid = ((*prbl)->rects[kpTitle].top + (*prbl)->rects[kpTitle].bottom)/2;
		SetRect(&(*prbl)->rects[kpBeat], 2+kListInset, mid-1, (*prbl)->rects[kpTitle].left-2,mid+1);
#endif
		
		/*
		 * move the stop button
		 */
		if (*stopButton)
			MoveMyCntl(*stopButton,PROG_WI-stopWidth-GROW_SIZE-INSET-kListInset,(*prbl)->rects[kpSubTitle].top,stopWidth,stopHeight);
		UL(prbl);
	}
}

/************************************************************************
 * InvalTPRect - 
 ************************************************************************/
void InvalTPRect(Rect *invalRect)
{
	Rect newInvalRect=(*TaskListHandle)->rView;
	if (SectRect(&newInvalRect, invalRect, &newInvalRect))
		InvalWindowRect(GetMyWindowWindowPtr(TaskProgressWindow),&newInvalRect);
}

/************************************************************************
 * DrawTaskProgressBar - 
 ************************************************************************/
#undef LUpdate
void DrawTaskProgressBar(ControlHandle bar)
{
	if (TaskProgressWindow)
	{
		RgnHandle rgn = NewRgn();
		Rect r;
		CGrafPtr	port = GetWindowPort(GetControlOwner(bar));
		RgnHandle oldClip = SavePortClipRegion(port);
		
		PositionAllCells();
		if (rgn)
		{
			r = (*TaskListHandle)->rView;
			RectRgn(rgn,&r);
			SectRgn(MyGetPortVisibleRegion(port),rgn,rgn);
			SectRgn(oldClip,rgn,rgn);
			SetPortClipRegion(port,rgn);
			Update1Control(bar,rgn);
			DisposeRgn(rgn);
			RestorePortClipRegion(port,oldClip);
		}
	}
}

/************************************************************************
 * DrawTP - 
 ************************************************************************/
static void DrawTPProgress(MyWindowPtr win, Rect *lRect,Handle data)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	ProgressBlock **prbl;	
	ProgressRectEnum which;
	threadDataHandle threadData;
#ifdef NEVER
	ThreadID threadID;
	Rect beatRect;
#endif	
	Rect clipRect;
	RgnHandle rgn,
						newRgn;
	Str255 mesg;

	threadData=(threadDataHandle)data;
	ASSERT(threadData);
	if (!threadData)
		return;
	rgn = NewRgn();
	newRgn = NewRgn();
	if (rgn && newRgn && (prbl = (*threadData)->prbl))
	{
			GetClip(rgn);
		LDRef(threadData);
		InitPrbl(prbl,lRect->top,&(*threadData)->stopButton);
		UL(threadData);
		LDRef(prbl);

		for (which=0;which<=kpTitle;which++)
		{
			MoveTo((*prbl)->rects[which].left,(*prbl)->rects[which].bottom-4);
			if (which==kpTitle)
				TextFace(bold);
			else
				TextFace(0);
			if (newRgn && rgn)
			{
				RectRgn(newRgn, &(*prbl)->rects[which]);
				SectRgn(newRgn, rgn, newRgn);
				SetClip(newRgn);
			}
			PSCopy(mesg,(*prbl)->messages[which]);
			TruncString(RectWi((*prbl)->rects[which]),mesg,truncEnd);
			DrawString(mesg);
		}
		clipRect = (*TaskListHandle)->rView;
		ClipRect(&clipRect);
		
		RectRgn(rgn,lRect);
		SectRgn(rgn,MyGetPortVisibleRegion(GetWindowPort(winWP)),rgn);
		
		if ((*threadData)->stopButton)
			if (!IsControlVisible((*threadData)->stopButton))
				ShowControl((*threadData)->stopButton);
			else
				Update1Control((*threadData)->stopButton,rgn);
		if ((*prbl)->bar)
			Update1Control((*prbl)->bar,rgn);
		UL(prbl);
	}
	if (rgn)
		DisposeRgn(rgn);
	if (newRgn)
		DisposeRgn(newRgn);
}

#ifndef BATCH_DELIVERY_ON
/************************************************************************
 * DrawTPFilter - 
 ************************************************************************/
static void DrawTPFilter(MyWindowPtr win, Rect *lRect,Handle data)
{
	short vert = lRect->top;
	Str255 num;

	if ((*FilterButton)->contrlRect.top != vert+PROG_BOX_HI)
		MoveMyCntl(FilterButton,PROG_WI-ControlWi(FilterButton)-GROW_SIZE-INSET-kListInset,vert+PROG_BOX_HI,ControlWi(FilterButton),ControlHi(FilterButton));	
//	SetGreyControl(FilterButton,CheckThreadRunning);	// pane not present during checking
	if (!IsControlVisible(FilterButton))
	{
		HiliteControl(FilterButton,0);
		ShowControl(FilterButton);
	}
	else Draw1Control(FilterButton);

	vert += PROG_BOX_HI;
	MoveTo(win->contR.left+kTextInset+kListInset, vert-4);
	TextFace(bold);
	DrawRString(TP_WAITING_TO_DELIVER);

	vert += PROG_BOX_HI;
	MoveTo(win->contR.left+kTextInset+INSET+kListInset, vert-4);	
	TextFace(0);
	DrawRString(TP_LEFT_TO_DELIVER);
	NumToString(TempInCount, num);	
	MoveTo((*FilterButton)->contrlRect.left-kRemainWidth, vert-4);	
	DrawString(num);

	vert += PROG_BOX_HI;
	MoveTo(win->contR.left+kTextInset+INSET+kListInset, vert-4);	
	DrawRString(TP_CLICK_TO_DELIVER);
}
#endif

/************************************************************************
 * DrawTPError - 
 ************************************************************************/
static void DrawTPError(MyWindowPtr win, Rect *lRect,Handle data)
{
	taskErrHandle taskErr = (taskErrHandle) data;
	short vert = lRect->top;
	Str255 text;
	Rect box,
			rView;
	
	if (taskErr)
	{
		TEHandle teh;
		vert += (win->vPitch - FontDescent);
		MoveTo(win->contR.left+kTextInset+kListInset, vert);
		TextFace(bold);
		rView = (*TaskListHandle)->rView;
		ClipRect(&rView);
		DrawString(PCopy(text,(*taskErr)->taskDesc));

		vert+=FontDescent;
		TextFace(0);
	
		SetRect(&box,win->contR.left+kTextInset+INSET+kListInset,vert,PROG_WI-kListInset,lRect->bottom);
		ClipRect(&rView);
		// strip all initial and trailing white space
		// TETextBox Sucks!!!
		if (teh = TEStyleNew(&box, &box))
		{
			TextStyle txStyle;
			Point aPoint;
			
			PCopy(text,(*taskErr)->errMess);
			//TrimAllWhite(text);	JDB 980218
			TESetText(&text[1],text[0],teh);
			if ((*taskErr)->errExplanation[0])
			{
				TESetSelect(32767,32767,teh);
				PCopy(text,(*taskErr)->errExplanation);
				//TrimAllWhite(text);	JDB 980218
				TEInsert(&text[1],text[0],teh); 
			}
			// if it doesn't fit, try condensing
			aPoint = TEGetPoint((*teh)->teLength, teh);
			if (!PtInRect(aPoint,&(*teh)->viewRect))
			{
				TESetSelect(0,(*teh)->teLength,teh);
				txStyle.tsFace=condense;
				TESetStyle(doFace,&txStyle,true, teh);	
			}
			TEUpdate(&box, teh);
			TEDispose(teh);	
		}
		
		// the help button
		{
			ControlHandle helpButton = (*taskErr)->helpButton;
			RgnHandle rgn = NewRgn();
			WindowPtr	winWP = GetMyWindowWindowPtr(TaskProgressWindow);
			if (rgn)
			{
				RectRgn(rgn,lRect);
				SectRgn(rgn,MyGetPortVisibleRegion(GetWindowPort(winWP)),rgn);
				MoveMyCntl(helpButton,lRect->right-ControlWi(helpButton)-GROW_SIZE-INSET-kListInset,lRect->bottom-ControlHi(helpButton)-INSET,0,0);
				if (!IsControlVisible(helpButton)) ShowControl(helpButton);
				else Update1Control(helpButton,rgn);
			}
		}
	}
}

/************************************************************************
 * PositionAllCells - really just draw invisible cells, since our 
 *										draw functions reposition controls
 ************************************************************************/
static void PositionAllCells(void)
{
	WindowPtr	TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
	Cell theCell;
	short bounds;
	Rect cellRect,rWin,
				emptyRect,
				visibleRect;
	Boolean next=true;
	RgnHandle updateRgn=NewRgn();
	
	SAVE_STUFF;
	
	bounds=(*TaskListHandle)->dataBounds.bottom;
	visibleRect=(*TaskListHandle)->visible;
	SetRect(&emptyRect,0,0,0,0);
	SetPort_(GetWindowPort(TaskProgressWindowWP));
	GetWindowUpdateRgn(TaskProgressWindowWP,updateRgn);
	// don't want to add movecontrol regions to update region, so save and restore updateRgn
	for(theCell.h=theCell.v=0;next;next=LNextCell(false,true,&theCell,TaskListHandle))
	{
		LRect(&cellRect,theCell,TaskListHandle);
		if (!EqualRect(&emptyRect,&cellRect) && !PtInRect(theCell,&visibleRect))
			ListItemDraw(false,&cellRect,theCell,TaskListHandle);
	}
	GetWindowStructureBounds(TaskProgressWindowWP,&rWin);
	ValidWindowRect(TaskProgressWindowWP,&rWin);
	InvalWindowRgn(TaskProgressWindowWP,updateRgn);
	DisposeRgn(updateRgn);
	REST_STUFF;
}


/************************************************************************
 * FooterUserPaneBackground - 
 ************************************************************************/
pascal void FooterUserPaneBackground (ControlHandle control, ControlBackgroundPtr info)
{
	if (TaskProgressWindow)
	{
		if (TaskProgressWindow->backBrush)
			SetThemeWindowBackground(GetMyWindowWindowPtr(TaskProgressWindow),kThemeActiveModelessDialogBackgroundBrush,False);
		else
			RGBBackColor(&TaskProgressWindow->backColor);
		SetUpControlBackground(control,RectDepth(&TaskProgressWindow->contR),ThereIsColor);
	}
}

/************************************************************************
 * SetWindowBackground - 
 ************************************************************************/
pascal void SetWindowBackground (ControlHandle control, ControlBackgroundPtr info)
{
	if (TaskProgressWindow)
		RGBBackColor(&TaskProgressWindow->backColor);
}

/************************************************************************
 * OpenTaskWin - 
 ************************************************************************/
 void OpenTasksWin(void)
{
	OpenTasksWinBehind(BehindModal);
	NewError = false;
}

/************************************************************************
 * OpenTaskWinBehind - 
 * 				behind parameter:
 *						InFront = -1 = frontmost
 *						nil = lastmost 
 *						ModalWindow	= frontmost nonmodal
 ************************************************************************/
void OpenTasksWinBehind(void* behind)
{
	OSErr err=0;
	MyWindowPtr win;
	WindowPtr	frontWin;
	WindowPtr	winWP;
	threadDataHandle threadData;
	ControlHandle cntl,
								footerCntl=nil;
	Rect 	r,
				bounds,
				rView;
	Point cSize;
	taskErrHandle taskErrs;
	DECLARE_UPP(ListItemDef,ListDef);
	DECLARE_UPP(FooterUserPaneBackground,ControlUserPaneBackground);
	DECLARE_UPP(SetWindowBackground,ControlUserPaneBackground);
	
	INIT_UPP(ListItemDef,ListDef);
	INIT_UPP(FooterUserPaneBackground,ControlUserPaneBackground);
	INIT_UPP(SetWindowBackground,ControlUserPaneBackground);
	if (ModalWindow)
		frontWin = ModalWindow;
	else
	{
		// need to also check for modal file dialog... if it doesn't belong to us, leave it frontmost.
		if (frontWin = FrontWindow())
			if ((GetWindowKind(frontWin) >= MBOX_WIN) && (GetWindowKind(frontWin) < LIMIT_WIN))
				frontWin = nil;
	}
	if (TaskProgressWindow)
	{ 
		WindowPtr	TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
		if (frontWin)
			SendBehind(TaskProgressWindowWP,frontWin);
		else
			UserSelectWindow(TaskProgressWindowWP);
		SelectOpenWazoo(TASKS_WIN);	//	Select if in a wazoo
		return;
	}
	if (behind==InFront)
		behind=frontWin ? frontWin : InFront;
	
	// if behind==nil, open window behind everyone else except toolbar	
	if (behind==nil)
	{
		WindowPtr newBehind=FrontWindow_();
		if (newBehind)
		{
			if (GetWindowKind(newBehind)==TBAR_WIN)
				newBehind = InFront;
			else
			for (newBehind;GetNextWindow(newBehind);newBehind=GetNextWindow(newBehind))
				if (GetWindowKind(GetNextWindow(newBehind))==TBAR_WIN) break;
		}
		behind = (void *)(newBehind ? newBehind : InFront);
	}
	
	if (!(win=GetNewMyWindow(TASKS_WIND,nil,nil,behind,false,false,TASKS_WIN)))
		{err=MemError(); goto fail;}

	winWP = GetMyWindowWindowPtr (win);
	SetWinMinSize(win,-1,-1);

	TaskProgressWindow = win;	
	if (InAThread())
		ProgWindow = TaskProgressWindow;		

	SetPort_(GetWindowPort(winWP));
	ConfigFontSetup(win);
	MySetThemeWindowBackground(win,kThemeListViewBackgroundBrush,False);

	if (!(FilterButton = GetNewControlSmall(TASK_PROGRESS_FILTER_CNTL,winWP)))
		{goto fail;}
	ButtonFit(FilterButton);	
	HideControl(FilterButton);
	OutlineControl(FilterButton,true);	
	
	/*
	 * allocate the list handle
	 */
	SetRect(&rView,win->contR.left+INSET,win->topMargin+INSET,win->contR.right,win->contR.bottom);
	cSize.v = PROG_TOTAL_HI; cSize.h = 0;
	SetRect(&bounds,0,0,1,0);
	if (!(TaskListHandle = CreateNewList(ListItemDefUPP,TASK_LDEF,&rView,&bounds,cSize,GetMyWindowWindowPtr(TaskProgressWindow),True,False,False,True)))
		goto fail;
	(*TaskListHandle)->selFlags = lOnlyOne|lNoNilHilite;
	(*TaskListHandle)->indent.v = kCellIndent;
	(*TaskListHandle)->indent.h = kCellIndent;

	SetRect(&r,-REAL_BIG,-REAL_BIG,REAL_BIG,REAL_BIG);
	if (cntl = NewControl(winWP, &r, "",True,kControlSupportsEmbedding+kControlWantsActivate+kControlHasSpecialBackground,0,0, kControlUserPaneProc, kListPaneRefCon))
	{
		SetControlData(cntl,0,kControlUserPaneBackgroundProcTag,sizeof(ControlUserPaneBackgroundUPP),(void*)&SetWindowBackgroundUPP);
		EmbedControl(FilterButton,cntl);
	}
	else
		goto fail;

	if (footerCntl = NewControl(winWP,&r,"\p",True,kControlHasSpecialBackground|kControlSupportsEmbedding,0,0,kControlUserPaneProc,kFooterRefCon))
		SetControlData(footerCntl,0,kControlUserPaneBackgroundProcTag,sizeof(ControlUserPaneBackgroundUPP),(void*)&FooterUserPaneBackgroundUPP);

	if (cntl = NewControl(winWP, &r, "",True, 0,0,0, kControlStaticTextProc, kNextTimeRefCon))
	{
		SetBevelFontSize(cntl,applFont,10);
		SetBevelTextAlign(cntl,teFlushRight);
		if (footerCntl)
			EmbedControl(cntl,footerCntl);
	}
	if (cntl = NewControl(winWP, &r, "",True, 0,0,0, kControlStaticTextProc, kLastTimeRefCon))
	{
		SetBevelFontSize(cntl,applFont,10);
		SetBevelTextAlign(cntl,teFlushLeft);
		if (footerCntl)
			EmbedControl(cntl,footerCntl);
	}
	// (jp) Don't allow icon buttons to be created REAL_BIG... Exhausts memory under alternate themes.
	SetRect (&r, 0, 0, 16, 16);
	if (cntl = NewControlSmall(winWP,&r,"\p",True,TP_NETWORK_MENU,kControlContentIconRef+kControlBehaviorMultiValueMenu,
											0,kControlBevelButtonSmallBevelProc+kControlBevelButtonMenuOnRight,kNetworkRefCon))
	{
		SetBevelIcon(cntl,TP_NETWORK_ICON,0,0,nil);
		//Gives 82a2 fits.  SD //SetBevelFontSize(cntl,applFont,10);
		if (footerCntl)
			EmbedControl(cntl,footerCntl);		
	}

	InsetRect(&win->contR,6,6);						

	win->maxSize.h = 0;

	win->didResize = TPDoDidResize;
	win->close = TPDoClose;
	win->update = TPDoUpdate;
	win->activate = TPDoActivate;
	win->position = TPPosition;	
	win->button = TPDoButton;		
	win->click = TPDoClick;	
	win->idle = TPIdle;
	win->cursor = TPDoCursor;
	win->help = TPDoShowHelp;
	win->key = TPDoKey;
	win->app1 = TPDoKey;
	win->zoomSize = TPDoZoomSize;

	win->dontControl = True;	
	win->dontDrawControls = True;
	win->dontActivate = True;

	ShowMyWindowBehind(winWP,(behind==InFront) ? nil : behind);
	SetTPWTitle();
	MyWindowDidResize(win,&win->contR);

	// add data to task list 
	for(threadData=gThreadData;threadData && !err;threadData=(*threadData)->next)
		err = AddListItemEntry((*TaskListHandle)->dataBounds.bottom, DrawTPProgress, (Handle)threadData,TaskListHandle);
#ifndef BATCH_DELIVERY_ON
	if (!err && !CheckThreadRunning)
		err = AddFilterTask();
#endif
	for(taskErrs=TaskErrorList;taskErrs && !err;taskErrs=(*taskErrs)->next)	
	{
		err = AddListItemEntry((*TaskListHandle)->dataBounds.bottom, DrawTPError, (Handle)taskErrs,TaskListHandle);
		if (!err) TPAddHelpButton(taskErrs);
	}
	if (err)
		goto fail;
	TaskDontAutoClose = !PrefIsSet(PREF_TASK_PROGRESS_AUTO);
	if (IsWazoo(winWP))
		TaskDontAutoClose = true;
	UpdateMyWindow(winWP);
	gTaskProgressInitied = true;
	return;
	
	fail:
		if (TaskProgressWindow) CloseMyWindow(GetMyWindowWindowPtr(TaskProgressWindow));
		WarnUser(COULDNT_WIN,err);
	return;
}

/************************************************************************
 * TPAddHelpButton - add the help button to an error string
 ************************************************************************/
OSErr TPAddHelpButton(taskErrHandle taskErrs)
{
	ControlHandle helpButton;
	ControlHandle listPaneCntl=FindControlByRefCon(TaskProgressWindow,kListPaneRefCon);
			WindowPtr	winWP = GetMyWindowWindowPtr(TaskProgressWindow);
	if (helpButton = GetNewControlSmall(TP_HELP_BUTTON,winWP))
	{
		if (listPaneCntl) EmbedControl(helpButton,listPaneCntl);
		ButtonFit(helpButton);
		(*taskErrs)->helpButton = helpButton;
		return noErr;
	}
	return MemError();
}

/************************************************************************
 * OpenTaskWinErrors - 
 ************************************************************************/
void OpenTasksWinErrors(void)
{
	Point pt;
	pt.h = pt.v = -1;

	OpenTasksWin();
	if (TaskProgressWindow)
	{
		WindowPtr	TaskProgressWindowWP = GetMyWindowWindowPtr (TaskProgressWindow);
		CollapseWindow(TaskProgressWindowWP, false);
		UpdateMyWindow(TaskProgressWindowWP);
	}
	if (!InBG)
		SysBeep(1);
	// else nm???	
//	NewError = false;
}			

/************************************************************************
 * GetNextCheckTime - 
 ************************************************************************/
UPtr GetNextCheckTime (UPtr item)
{
	uLong checkTicks = PersCheckTicks();
	
	item[0] = 0;
	if (checkTicks)	
	{
		if (!AutoCheckOK())
			GetRString(item,TP_NEVER);
		else
		{
			checkTicks = MAX(checkTicks,TickCount()+3600);
			TimeString((LocalDateTime()-TickCount()/60)+checkTicks/60,False,item,nil);
		}
	}
	else
		GetRString(item,TP_NOT_SCHEDULED);
	return(item);
}


/************************************************************************
 * GetLastCheckTime - 
 ************************************************************************/
UPtr GetLastCheckTime (UPtr item)
{
	item[0] = 0;
	if (CheckThreadRunning)
		GetRString(item,TP_CHECKING_NOW);
	else
	if (LastCheckTime)
		TimeString(LastCheckTime,False,item,nil);
	else
		GetRString(item,TP_NEVER);
	return(item);
}

/************************************************************************
 * TPDoUpdate - draw the window
 ************************************************************************/
static void TPDoUpdate(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Rect rView;
	ControlHandle cntl,
								listPaneCntl;
	RgnHandle rgn = NewRgn();
	ASSERT(win && (win==TaskProgressWindow));
	
	// list box frame
	InfiniteClip(GetWindowPort(winWP));
	rView = (*TaskListHandle)->rView;
	rView.right+=GROW_SIZE;
	DrawThemeListBoxFrame(&rView,kThemeStateActive);

	// update all controls outside of listpane
	TaskProgressRefresh();	
	listPaneCntl=FindControlByRefCon(TaskProgressWindow,kListPaneRefCon);
	ASSERT(listPaneCntl);
	if (!listPaneCntl)
		return;
	SetControlVisibility(listPaneCntl,false,false);
	if (!GetRootControl(winWP,&cntl))
		Update1Control(cntl,MyGetPortVisibleRegion(GetWindowPort(winWP)));
			
	// now update all controls inside listpane
	PositionAllCells();
	SetControlVisibility(listPaneCntl,true,false);
	rView = (*TaskListHandle)->rView;
	SET_COLORS;
	EraseRect(&rView);
	RectRgn(rgn,&rView);
	SectRgn(rgn,MyGetPortVisibleRegion(GetWindowPort(winWP)),rgn);
	rView.right+=GROW_SIZE;
	ClipRect(&rView);
	LUpdate(rgn,TaskListHandle);
	if (rgn)
		DisposeRgn(rgn);
}

/************************************************************************
 * TPDoActivate - activate/deactivate the window
 ************************************************************************/
static void TPDoActivate(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle listPaneCntl, rootCntl,
								cntl, parent, tabCntl=nil;
	Rect rView = (*TaskListHandle)->rView;

	if (win->isActive)
		NewError = false;
	
	InfiniteClip(GetWindowPort(winWP));
	listPaneCntl=FindControlByRefCon(TaskProgressWindow,kListPaneRefCon);
	ASSERT(listPaneCntl);
	if (!listPaneCntl)
		return;
	// activate all controls outside of listpane
	SetControlVisibility(listPaneCntl,false,false);
	if (!GetRootControl(winWP,&rootCntl))
	{
		for (cntl=GetControlList(winWP);cntl;cntl=GetNextControl(cntl))
			if (cntl!=listPaneCntl && !IsWazooEmbedderCntl(cntl))
			{
				if (IsWazooTabCntl(cntl))
					tabCntl = cntl;
				else
				{
					GetSuperControl(cntl,&parent);
					if (parent==rootCntl)
					{
						ActivateMyControl(cntl,win->isActive);
						Draw1Control(cntl);
					}
				}
			}
		if (tabCntl)
		{
			ActivateMyControl(tabCntl,win->isActive);
			Draw1Control(tabCntl);
		}
	}
	// activate all controls inside listpane
	SetControlVisibility(listPaneCntl,true,false);
	ClipRect(&rView);
//	if (win->isActive) ActivateControl(listPaneCntl);
//		else DeactivateControl(listPaneCntl);
	for (cntl=GetControlList(winWP);cntl;cntl=GetNextControl(cntl))
	{
		GetSuperControl(cntl,&parent);
		if (parent==listPaneCntl)
		{
			ActivateMyControl(cntl,win->isActive);
			Draw1Control(cntl);
		}
	}
	LActivate(win->isActive, TaskListHandle);
}

/************************************************************************
 * TaskProgressRefresh - update the task progress window top margin
 ************************************************************************/
void TaskProgressRefresh(void)
{
	Str255 scratch;
	Str63 timeString;
	ControlHandle cntl;

	if (TaskProgressWindow)
	{
		if (cntl=FindControlByRefCon(TaskProgressWindow,kNextTimeRefCon))
		{
			SetTextControlText(cntl,ComposeRString(scratch,TP_NEXT_CHECK,GetNextCheckTime(timeString)),nil);
		}
		if (cntl=FindControlByRefCon(TaskProgressWindow,kLastTimeRefCon))
		{
#if 0
			if (NewError)
			{
				RGBColor oldColor, newColor;
				GetForeColor(&oldColor);
				ForeColor(redColor);
				GetForeColor(&newColor);
				SetBevelColor(cntl,&newColor);
				RGBForeColor(&oldColor);
			}
#endif				
			SetTextControlText(cntl,ComposeRString(scratch,TP_LAST_CHECK,GetLastCheckTime(timeString)),nil);
		}
		if (cntl=FindControlByRefCon(TaskProgressWindow,kNetworkRefCon))
		{
			MenuHandle menu = nil;
			if (!GetControlData(cntl,kControlMenuPart,kControlBevelButtonMenuHandleTag,sizeof(menu),(Ptr)&menu,nil) && menu)
			{
				SetItemMark(menu,NETWORK_NO_CONNECTION,PrefIsSet(PREF_PPP_NOAUTO) ? checkMark : noMark);
				SetItemMark(menu,NETWORK_NO_BATTERIES,PrefIsSet(PREF_NO_BATT_CHECK) ? checkMark : noMark);
				SetItemMark(menu,NETWORK_GO_OFFLINE,PrefIsSet(PREF_OFFLINE) ? checkMark : noMark);
			}
		}
	}
}

/************************************************************************
 * TPDoDidResize - resize the window
 ************************************************************************/
static void TPDoDidResize(MyWindowPtr win, Rect *oldContR)
{
	ControlHandle cntl;
	Rect rView = win->contR;
	Point cSize;
	short listBottom = win->contR.bottom-GROW_SIZE-kListInset;
	short wi, midPoint;

	if (cntl=FindControlByRefCon(win,kListPaneRefCon))
		MoveMyCntl(cntl,0,0,win->contR.right,listBottom + 2);
	if (cntl=FindControlByRefCon(win,kFooterRefCon))
		MoveMyCntl(cntl,0,listBottom + 2,win->contR.right,win->contR.bottom - listBottom - 2);

	if (cntl=FindControlByRefCon(win,kNetworkRefCon))
		MoveMyCntl(cntl,kListInset,listBottom+(win->contR.bottom-listBottom-kNetworkHeight)/2+1,kNetworkWidth,kNetworkHeight);

	// list
	if (TaskListHandle)
	{
		cSize.h = win->contR.right - GROW_SIZE - 1;
		cSize.v = PROG_TOTAL_HI;
		SetRect(&rView,win->contR.left+kListInset,win->topMargin+kListInset,win->contR.right-GROW_SIZE-kListInset,listBottom);
		ResizeList(TaskListHandle,&rView,cSize);
	}
	
	midPoint = (rView.left + rView.right)/2 + kNetworkWidth;
	wi = (rView.right - rView.left - kNetworkWidth)/2 - INSET;
	if (cntl=FindControlByRefCon(win,kLastTimeRefCon))
		MoveMyCntl(cntl,kListInset+kNetworkWidth+1,listBottom+8,wi,ONE_LINE_HI(win));		
	if (cntl=FindControlByRefCon(win,kNextTimeRefCon))
		MoveMyCntl(cntl,midPoint,listBottom+8,wi-INSET,ONE_LINE_HI(win));
	
	InvalContent(win);
}

/************************************************************************
 * TPDoZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void TPDoZoomSize(MyWindowPtr win,Rect *zoom)
{
	int numCells;	// = (*TaskListHandle)->dataBounds.bottom;  <-- doesn't work when auto-opening tp
	taskErrHandle taskErrs;
	short zoomHi = zoom->bottom-zoom->top;
	short zoomWi = zoom->right-zoom->left;
	short hi, wi;

#ifdef BATCH_DELIVERY_ON
	numCells = CheckThreadRunning + SendThreadRunning;
#else
	numCells = CheckThreadRunning + SendThreadRunning + NeedToFilterIn;
#endif
	for(taskErrs=TaskErrorList;taskErrs;taskErrs=(*taskErrs)->next)	
		numCells++;
	numCells = MAX(numCells,1);

	hi = win->topMargin + GROW_SIZE + kListInset + (numCells * (*TaskListHandle)->cellSize.v);
	if (numCells)
		hi += kListInset;
	wi = PROG_WI;

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;	
}


/************************************************************************
 * TPDoClose - close the window
 ************************************************************************/
static Boolean TPDoClose(MyWindowPtr win)
{
#pragma unused(win)
	threadDataHandle threadData;
	ProgressBHandle prbl;
	taskErrHandle taskErrs;
	
	FilterButton = nil;
	for(threadData=gThreadData;threadData;threadData=(*threadData)->next)
	{
		(*threadData)->stopButton=nil;	
		for(prbl=(*threadData)->prbl;prbl;prbl=(*prbl)->next)
			(*prbl)->bar=nil;
	}

	for(taskErrs=TaskErrorList;taskErrs;taskErrs=(*taskErrs)->next)	
	{
		if ((*taskErrs)->helpButton)
		{
			DisposeControl((*taskErrs)->helpButton);
			(*taskErrs)->helpButton = nil;
		}
	}

	LDispose(TaskListHandle);
	TaskListHandle = nil;
	if (TaskProgressWindow==ProgWindow) ProgWindow = nil;	// only nil if same win
	TaskProgressWindow = nil;
	gTaskProgressInitied = false;
	return(True);
}

/************************************************************************
 * TPDoKey - key stroke
 ************************************************************************/
static Boolean TPDoKey(MyWindowPtr win, EventRecord *event)
{
	Byte c = event->message&charCodeMask;

	if ((event->modifiers&cmdKey) && IsCmdChar(event,'.') ||
			(c==escChar) &&
			 (((event->message&keyCodeMask)>>8)==escKey))
	{
		threadDataHandle threadData;
		Rect rView;
	
		if (!gThreadData)
			return (false);

#ifdef SPEECH_ENABLED
		if (CancelSpeech ())
			return (false);
#endif
			
		rView = (*TaskListHandle)->rView;
		ClipRect(&rView);
		for(threadData=gThreadData;threadData;threadData=(*threadData)->next)
			if ((*threadData)->stopButton)
//				HiliteControl((*threadData)->stopButton,1);
					ActivateControl((*threadData)->stopButton);	// am doc says to do this
		Pause(20L);
		for(threadData=gThreadData;threadData;threadData=(*threadData)->next)
		{
			ClipRect(&rView);	
			if ((*threadData)->stopButton)
				HiliteControl((*threadData)->stopButton,0);
			SetThreadGlobalCommandPeriod ((*threadData)->threadID, true);
		}
		ClipRect(&win->contR);
		return (true);
	}
	else if ((c==returnKey || c==enterKey) && IsControlVisible(FilterButton) && IsControlActive(FilterButton))
	{
		HiliteControl(FilterButton,1);
		TPDoButton(win,FilterButton,event->modifiers,1);
	}
	else if (!(event->modifiers&cmdKey))
		SysBeep(1);
	return (false);
}


/************************************************************************
 * TPPosition - 
 ************************************************************************/
static Boolean TPPosition(Boolean save, MyWindowPtr win)
{
	Str31 title;
	
	MyGetItem(GetMHandle(WINDOW_MENU),WIN_TASKS_ITEM,title);
	SetWTitle_(GetMyWindowWindowPtr(win),title);
	return(PositionPrefsTitle(save,win));
}

/************************************************************************
 * TPDoButton
 ************************************************************************/

static void TPDoButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
#pragma unused(win,modifiers)	
	threadDataHandle threadData;
	long controlId;
	taskErrHandle taskErrs;
	
	if (button==FindControlByRefCon(win,kNetworkRefCon))
	{
		SInt16 value=0;
		MenuHandle menu = nil;
		short markChar=0;
		
		if (!GetControlData(button,part,kControlBevelButtonMenuValueTag,sizeof(SInt16),(Ptr)&value,nil) && !GetControlData(button,part,kControlBevelButtonMenuHandleTag,sizeof(menu),(Ptr)&menu,nil) && menu)
		{
			controlId = kNetworkPopup;
			GetItemMark(menu,value,&markChar);
			switch(value)
			{
				case NETWORK_NO_CONNECTION:
					SetPref(PREF_PPP_NOAUTO,(markChar==checkMark) ? YesStr : NoStr);
					break;
				case NETWORK_NO_BATTERIES:
					SetPref(PREF_NO_BATT_CHECK,(markChar==checkMark) ? YesStr : NoStr);
					break;
				case NETWORK_GO_OFFLINE:
					SetPref(PREF_OFFLINE,(markChar==checkMark) ? YesStr : NoStr);
					break;
			}
			InvalContent(win);
		}
	}
	else
	if (button==FilterButton)
	{
		controlId = kFilterButton;
		FilterXferMessages();
		goto done;
	}
	for(threadData=gThreadData;threadData;threadData=(*threadData)->next)
		if (button == (*threadData)->stopButton)
		{
			controlId = kStopButton;
			SetThreadGlobalCommandPeriod ((*threadData)->threadID, true);
			goto done;
		}

	for(taskErrs=TaskErrorList;taskErrs;taskErrs=(*taskErrs)->next)	
		if (button == (*taskErrs)->helpButton)
		{
			Str255 err, expl;
			GoGetHelp(PCopy(err,(*taskErrs)->errMess),PCopy(expl,(*taskErrs)->errExplanation));
			controlId = kHelpButton;
			goto done;
		}	

done:	
	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(GetMyWindowWindowPtr(win)),AUDITCONTROLID(GetWindowKind(GetMyWindowWindowPtr(win)),controlId),mouseDown);
}

/************************************************************************
 * TPDoClick
 ************************************************************************/
static void TPDoClick(MyWindowPtr win,EventRecord *event)
{
	Point pt = event->where;
	Rect	rCntl;
	
	GlobalToLocal(&pt);
	if (PtInRect(pt,GetControlBounds((*TaskListHandle)->vScroll,&rCntl)))
		LClick(pt, CurrentModifiers(), TaskListHandle);
	else
	{
		Rect rView = (*TaskListHandle)->rView;
		if (PtInRect(pt,&rView))
		{
			ClipRect(&rView);
			HandleControl(pt,win);
			ClipRect(&win->contR);
		}
		else
			HandleControl(pt,win);	// <--- what if we click on a part of the cntrl that's sticking out of rView????
	}
}

/************************************************************************
 * TPDoCursor - set the cursor properly for the window
 ************************************************************************/
static void TPDoCursor(Point mouse)
{
	if (!PeteCursorList(TaskProgressWindow->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * TPDoShowHelp - provide help for the window
 ************************************************************************/
static void TPDoShowHelp(MyWindowPtr win,Point mouse)
{
#pragma unused(win,mouse)
}

/************************************************************************
 * AddProgressTask - 
 ************************************************************************/
OSErr AddProgressTask(threadDataHandle threadData)
{
	OSErr err=noErr;
	if (TaskProgressWindow)
	{
		err = AddListItemEntry(0, DrawTPProgress, (Handle)threadData,TaskListHandle);
		SetTPWTitle();
	}
	return err;
}

/************************************************************************
 * RemoveProgressTask - 
 ************************************************************************/
void RemoveProgressTask(threadDataHandle threadData)
{
	if (TaskProgressWindow)
	{
		RemoveListItemEntry((Handle)threadData,TaskListHandle);
		SetTPWTitle();
	}
}

#ifndef BATCH_DELIVERY_ON
/************************************************************************
 * AddFilterTask - 
 ************************************************************************/
OSErr AddFilterTask(void)
{
	if (TaskProgressWindow && NeedToFilterIn)
	{
		RemoveListItemEntry((Handle)FilterButton,TaskListHandle);	// just in case
		return(AddListItemEntry(SendThreadRunning, DrawTPFilter, (Handle)FilterButton,TaskListHandle));
	}
	return noErr;
}

/************************************************************************
 * RemoveFilterTask - 
 ************************************************************************/
void RemoveFilterTask(void)
{
	if (TaskProgressWindow)
	{
		RemoveListItemEntry((Handle)FilterButton,TaskListHandle);
		HideControl(FilterButton);
	}		
}
#endif

/************************************************************************
 * AddTaskErrorsString
 ************************************************************************/
OSErr AddTaskErrorsS(StringPtr error, StringPtr explanation, TaskKindEnum taskKind, long persId)
{
	OSErr err = noErr;
	taskErrHandle taskErrs;
	Str255 name;
	PersHandle pers;
	
	RemoveTaskErrors(taskKind,persId);
	ComposeLogS(LOG_ALRT,nil,"\p�%p� �%p�",error,explanation);
	NewError = true;
	if (!(taskErrs = NuHandleClear(sizeof(taskErrData))))
	{
		err = MemError();
		CommandPeriod = true;
		if (taskKind==CheckingTask) 
			CheckThreadError = err;
		else
		if (taskKind==SendingTask) 
			SendThreadError = err;
		return err;		
	}
	(*taskErrs)->taskKind = taskKind;
	(*taskErrs)->persId = persId;

	PSCopy((*taskErrs)->errMess,error);
	if ((*taskErrs)->errMess[0]==255)
		(*taskErrs)->errMess[255]='�';

	PSCopy((*taskErrs)->errExplanation,explanation);
	if ((*taskErrs)->errExplanation[0]==255)
		(*taskErrs)->errExplanation[255]='�';
	
	for(pers=PersList;pers;pers=(*pers)->next)
		if ((*pers)->persId==persId)
			break;
	if (pers)
		PSCopy(name,(*pers)->name);
	else
		GetRString(name,TP_UNKNOWN_PERS);		
	LDRef(taskErrs);
	switch(taskKind)
	{
		case CheckingTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_CHECKING_MAIL,name);
		break;
		case SendingTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_SENDING_MAIL,name);
		break;
		case IMAPResyncTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_RESYNCING,name);		
		break;
		case IMAPFetchingTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_FETCHING,name);		
		break;
		case IMAPDeleteTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_DELETING,name);		
		break;
		case IMAPUndeleteTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_UNDELETING,name);		
		break;
		case IMAPTransferTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_TRANSFERRING,name);		
		break;
		case IMAPExpungeTask:
		case IMAPMultExpungeTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_EXPUNGING,name);
		break;
		case IMAPAttachmentFetch:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_ATTACHMENT_FETCH,name);		
		break;
		case IMAPSearchTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_SEARCH,name);
		break;
		case IMAPMultResyncTask:
		case IMAPPollingTask:
			GetRString((*taskErrs)->taskDesc,ERR_MULT_RESYNCING);
		break;
		case IMAPUploadTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_TRANSFERRING,name);		
		break;
		case IMAPAlertTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_IMAP_ALERT,name);		
		break;
		case IMAPFilterTask:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_FILTERING,name);
		break;
		default:
			ComposeRString((*taskErrs)->taskDesc,ERR_PERS_UNKNOWN_TASK,name);
		break;
	}

	UL(taskErrs);
	LL_Queue(TaskErrorList,taskErrs,(taskErrHandle));
	if (TaskProgressWindow)
	{
		TPAddHelpButton(taskErrs);
		if (err = AddListItemEntry((*TaskListHandle)->dataBounds.bottom, DrawTPError, (Handle)taskErrs,TaskListHandle))
		{
			CommandPeriod=true;
			if (taskKind==CheckingTask) 
				CheckThreadError = err;
			else
			if (taskKind==SendingTask) 
				SendThreadError = err;
		}
	}
	return (err);
}

/************************************************************************
 * SetTPWTitle
 ************************************************************************/
static void SetTPWTitle(void)
{
	WindowPtr	TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
	Str31 title;
	Str255 winTitle;
	
	if (TaskProgressWindow && GetWindowKind(TaskProgressWindowWP)==TASKS_WIN)
	{
		MyGetItem(GetMHandle(WINDOW_MENU),WIN_TASKS_ITEM,title);
		PCopy(winTitle,title);
		if (SendThreadRunning || CheckThreadRunning)
		{
			if (CheckThreadRunning)
			{
				if (SendThreadRunning)
					ComposeRString(winTitle,TP_DUAL_FMT,title,TP_CHECKING,TP_SENDING);
				else
					ComposeRString(winTitle,TP_SINGLE_FMT,title,TP_CHECKING);
			}
			else if (SendThreadRunning)
				ComposeRString(winTitle,TP_SINGLE_FMT,title,TP_SENDING);
		}
		SetWTitle_(TaskProgressWindowWP,winTitle);
	}
}

/************************************************************************
 * RemoveTaskErrors
 ************************************************************************/
void RemoveTaskErrors(TaskKindEnum taskKind,long persId)
{
	taskErrHandle current, last, temp;

	for(last=current=TaskErrorList;current;)
	{
		temp=current;
		current=(*current)->next;
		if (((*temp)->taskKind==taskKind) && (((*temp)->persId==persId) || (persId==-1)))
		{
			if(temp==TaskErrorList)
				TaskErrorList=(*temp)->next;
			else
				(*last)->next=(*temp)->next;
			if (TaskProgressWindow)
			{
				RemoveListItemEntry((Handle)temp,TaskListHandle);
				if ((*temp)->helpButton)
				{
					DisposeControl((*temp)->helpButton);
					(*temp)->helpButton = nil;
				}				
			}
			DisposeHandle((Handle)temp);
		}
		else
			last=temp;
	}
	if (!TaskErrorList)
		NewError=false;
}

/************************************************************************
 * TPIdle - idle time handler for the Task Progress window
 ************************************************************************/
void TPIdle(MyWindowPtr win)
{	
	//	Update the TP window if the connection method has changed.
	//	We're watching the connection method in ActiveUserIdleTasks().
	
	if (NeedToUpdateTP()) InvalContent(win);
}


/*
Task Progress:

To do/bug list:

-tp control for toolbar

-hitting cancel and cmd-. sometimes doesn't cancel stdalert
-insert errors at top of error list
-composestdalert should deactivate frontmost window
-append � to button to mean hit this button when alert times out 
-Aborting mail send-- can't edit open comp window without closing it first
-next mail check time sometimes off a minute
-"One of the addresses is too long..." should be a per-message error
-"There was an error opening an attachment..." should be a per-message error

done:
-don't auto-hide tp window until filtered
-inset tp list for wazooability
-offline control 
-decouple in/out filtering
-don't put up filter pane for outgoing mail, change menu
-don't show filter pane if can't be done
-flickering scroll bar
-if cell is scrolled out of view and cell in view goes away, out of view cell not accessible until window resized
-report error from main thread if error reporting error
-add #of messages to filter pane

*/