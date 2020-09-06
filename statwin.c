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

#include <conf.h>
#include <mydefs.h>

#include "statwin.h"
#define FILE_NUM 132
/* Copyright (c) 2000 by Qualcomm, Inc */

enum
{
	kHeaderHeight = 30
};

typedef struct
{
	MyWindowPtr win;
	Boolean	inited;
	ControlHandle	ctlHeader,ctlTimePeriod,ctlMore;
	PETEHandle	pte;
	short	timePeriod,more;
} WinData;
static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static Boolean StatClose(MyWindowPtr win);
static void StatDidResize(MyWindowPtr win, Rect *oldContR);
static void StatClick(MyWindowPtr win,EventRecord *event);
static Boolean StatMenu(MyWindowPtr win, int menu, int item, short modifiers);
static void StatZoomSize(MyWindowPtr win,Rect *zoom);
static Boolean StatKey(MyWindowPtr win, EventRecord *event);

/************************************************************************
 * OpenStatWin - open statistics window
 ************************************************************************/
void OpenStatWin(void)
{
	WindowPtr	gWinWinWP;
	
	if (!HasFeature(featureStatistics))
		return;
		
	if (SelectOpenWazoo(STAT_WIN)) return;	//	Already opened in a wazoo

	if (!gWin.inited)
	{
		short err=0;
		PETEDocInitInfo pi;
		
		if (!(gWin.win=GetNewMyWindow(STAT_WIND,nil,nil,BehindModal,false,false,STAT_WIN)))
			{err=MemError(); goto fail;}
		gWinWinWP = GetMyWindowWindowPtr(gWin.win);
		SetWinMinSize(gWin.win,-1,-1);
		SetPort_(GetWindowPort(gWinWinWP));
		ConfigFontSetup(gWin.win);	
		MySetThemeWindowBackground(gWin.win,kThemeListViewBackgroundBrush,False);

		// controls
		if (!(gWin.ctlHeader = GetNewControlSmall_(PLACARD_CNTL,gWinWinWP)) ||
			!(gWin.ctlTimePeriod = GetNewControlSmall_(STAT_TIME_MENU_CNTL,gWinWinWP)) ||
			!(gWin.ctlMore = GetNewControlSmall_(STAT_MORE_CNTL,gWinWinWP)))
				goto fail;
		
		EmbedControl(gWin.ctlTimePeriod,gWin.ctlHeader);
		EmbedControl(gWin.ctlMore,gWin.ctlHeader);
		SetControlValue(gWin.ctlTimePeriod,gWin.timePeriod = GetPrefLong(PREF_STAT_TIME_PERIOD));
		if (gWin.timePeriod)
			gWin.timePeriod--;
		SetControlValue(gWin.ctlMore,gWin.more = GetPrefLong(PREF_STAT_MORE));

		DefaultPII(gWin.win,true,peVScroll|peGrowBox,&pi);
		if (err=PeteCreate(gWin.win,&gWin.pte,0,&pi))
			goto fail;
		CleanPII(&pi);
		PeteLock(gWin.pte,0,0x7fffffff,peModLock);
		PeteFocus(gWin.win,gWin.pte,true);

		gWin.win->dontControl = true;
		gWin.win->position = PositionPrefsTitle;
		gWin.win->close = StatClose;
		gWin.win->didResize = StatDidResize;
		gWin.win->click = StatClick;
		gWin.win->bgClick = StatClick;
		gWin.win->menu = StatMenu;
		gWin.win->zoomSize = StatZoomSize;
		gWin.win->key = StatKey;
#if 0
		gWin.win->update = DoUpdate;
		gWin.win->activate = DoActivate;
		gWin.win->help = DoShowHelp;
		gWin.win->key = DoKey;
		gWin.win->app1 = DoKey;
		gWin.win->grow = DoGrow;
		gWin.win->find = SigFind;
		gWin.win->drag = DoDragHandler;
		gWin.win->zoomSize = DoZoomSize;
#endif

		ShowMyWindow(gWinWinWP);
		MyWindowDidResize(gWin.win,&gWin.win->contR);
		gWin.inited = true;
		RedisplayStats();
		UseFeature(featureStatistics);
		return;
		
		fail:
			if (gWin.win) CloseMyWindow(GetMyWindowWindowPtr(gWin.win));
			if (err) WarnUser(COULDNT_WIN,err);
			return;
	}
	UserSelectWindow(GetMyWindowWindowPtr(gWin.win));	
}

/************************************************************************
 * RedisplayStats - redisplay the stats
 ************************************************************************/
void RedisplayStats(void)
{
	Handle	textH;
	long hOff = 0L, iOff = 0L;
	OSErr	err;
	
	if (!gWin.inited) return;	//	Statistics window not open
	
	if (GetWindowKind(GetMyWindowWindowPtr(gWin.win))!=STAT_WIN) return;	// Is in a wazoo, but not currently selected
	
	if (textH = GetStatsAsText(gWin.timePeriod,gWin.more))
	{
		PeteCalcOff(gWin.pte);
		PeteDelete(gWin.pte,0,0x7fffffff);
		err = InsertHTML(textH,&hOff,GetHandleSize(textH),&iOff,gWin.pte,kDontEnsureCR);
		PeteCalcOn(gWin.pte);
		PeteLock(gWin.pte,0,0x7fffffff,peModLock);
		if (!err)
		{
			PETEMarkDocDirty(PETE,gWin.pte,false);
			gWin.win->isDirty = false;
		}
		PushGWorld();
		PETEDraw(PETE,gWin.pte);
		PopGWorld();
	}
}

/************************************************************************
 * GetStatWin - return the stat window
 ************************************************************************/
MyWindowPtr GetStatWin(void)
{
	return gWin.win;
}

/************************************************************************
 * StatClose - close the window
 ************************************************************************/
static Boolean StatClose(MyWindowPtr win)
{
#pragma unused(win)
	
	if (gWin.inited)
	{
		gWin.inited = false;
	}
	return(True);
}

/************************************************************************
 * StatDidResize - resize the window
 ************************************************************************/
static void StatDidResize(MyWindowPtr win, Rect *oldContR)
{
#pragma unused(oldContR)
	Rect r,rMore;

	r = gWin.win->contR;
	MoveMyCntl(gWin.ctlHeader,-1,win->topMargin-1,RectWi(gWin.win->contR)+2,kHeaderHeight);
	MoveMyCntl(gWin.ctlTimePeriod,5,win->topMargin+5,0,0);
	GetControlBounds(gWin.ctlMore,&rMore);
	MoveMyCntl(gWin.ctlMore,gWin.win->contR.right-RectWi(rMore),win->topMargin+5,0,0);

	r.top = win->topMargin+kHeaderHeight;
	PeteDidResize(gWin.pte,&r);
}

/************************************************************************
 * StatClick - handle a click in the stat window
 ************************************************************************/
static void StatClick(MyWindowPtr win,EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point pt;
	
	SetPort(GetWindowPort(winWP));
	pt = event->where;
	GlobalToLocal(&pt);

	if (!win->isActive)
	{
		SelectWindow_(winWP);
		UpdateMyWindow(winWP);	//	Have to update manually since no events are processed
	}

	if (PtInPETE(pt,gWin.pte))
		PeteEditWFocus(gWin.win,gWin.pte,peeEvent,event,nil);
	else
	{
		ControlHandle	hCtl;

		if (FindControl(pt,winWP,&hCtl))
		if (TrackControl(hCtl,pt,(void *)(-1)))
		{
			short	timePeriod,more;

			if (hCtl == gWin.ctlTimePeriod)
				SetPrefLong(PREF_STAT_TIME_PERIOD,GetControlValue(hCtl));
			else if (hCtl == gWin.ctlMore)
			{
				//	Toggle checkbox
				SetControlValue(hCtl,1-GetControlValue(hCtl));
				SetPrefLong(PREF_STAT_MORE,GetControlValue(hCtl));
			}

			timePeriod = GetControlValue(gWin.ctlTimePeriod)-1;
			more = GetControlValue(gWin.ctlMore);
			if (timePeriod != gWin.timePeriod || more != gWin.more)
			{
				if (more != gWin.more)
					PeteScroll(gWin.pte,0,0);
				gWin.timePeriod = timePeriod;
				gWin.more = more;
				RedisplayStats();
			}
		}
	}
}

/************************************************************************
 * StatKey - handle key stroke for stat window
 ************************************************************************/
static Boolean StatKey(MyWindowPtr win, EventRecord *event)
{
	if (!(event->modifiers&cmdKey))
	{
		switch (event->message&charCodeMask)
		{
			case ' ':			PeteScroll(gWin.pte,0,psePageDn); return true;
		}
	}
	return false;
}

/************************************************************************
 * StatMenu - handle menu choices for stat window
 ************************************************************************/
static Boolean StatMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(modifiers)
	switch (menu)
	{
		case FILE_MENU:
			switch (item)
			{
				case FILE_PRINT_ITEM:
				case FILE_PRINT_ONE_ITEM:
					PeteFocus(gWin.win,gWin.pte,true);					
					PrintOneMessage(win,modifiers&shiftKey,item==FILE_PRINT_ONE_ITEM);
					return true;
			}
	}
	return false;
}

/************************************************************************
 * StatZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void StatZoomSize(MyWindowPtr win,Rect *zoom)
{
	short zoomHi = zoom->bottom-zoom->top;
	short zoomWi = zoom->right-zoom->left;
	short hi, wi;
	PETEDocInfo info;
	
	wi = GetRLong(STAT_GRAPH_WIDTH) + 24;
	PETEGetDocInfo(PETE,gWin.pte,&info);
	hi = info.docHeight+kHeaderHeight;

	wi = MIN(wi,zoomWi); wi = MAX(wi,win->minSize.h);
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+wi;
	zoom->bottom = zoom->top+hi;
}

