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

#include <Quickdraw.h>

#include <conf.h>
#include <mydefs.h>

#include "cursor.h"
#define FILE_NUM 10
#pragma segment Cursor
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * routines to handle the cursor
 ************************************************************************/

#define MAX_CURSOR 8
short CStack[MAX_CURSOR+1];
int CSTop = 0;
/************************************************************************
 * CycleBalls - spin the beach ball
 ************************************************************************/
void CycleBalls(void)
{
	static int currentBall= -1;
	static long ticks;
	extern short gNumBackgroundThreads;
	
	//ASSERT(!gNumBackgroundThreads || !InAThread() || CurThreadGlobals != &ThreadGlobals);
	
	if (TickCount()>ticks+10)
	{
		ticks = TickCount();

#ifdef THREADING_ON
	if (InAThread () || DFWTC)
	{
		MiniEvents();
		return;
	}
#endif
	
		currentBall = (currentBall+1) % 4;
		if (CStack[CSTop]<BALL_CURS || CStack[CSTop]>BALL_CURS+3)
			PushCursor(BALL_CURS+currentBall);
		else
			SetTopCursor(BALL_CURS+currentBall);
		SFWTC = True;
		MiniEventsLo(0,nil);	// let other threads run, but handle no events
	}
}

/************************************************************************
 * CyclePendulum - swing the pendulum
 ************************************************************************/
void CyclePendulum(void)
{
	static int currentPend= -1;
	static int currentDir= 1;
	
#ifdef THREADING_ON
	if (InAThread ()) {
		return;
	}
#endif
	
	if (currentDir > 0)
	{
		currentPend++;
		if (CStack[CSTop]<PENDULUM_CURS ||
								 CStack[CSTop]>PENDULUM_CURS+3)
			PushCursor(PENDULUM_CURS+currentPend);
		else
			SetTopCursor(PENDULUM_CURS+currentPend);
		if (currentPend == 3)
		{
			currentPend++;
			currentDir = -1;
		}
	}
	else
	{
		currentPend--;
		if (CStack[CSTop]<PENDULUM_CURS ||
				CStack[CSTop]>PENDULUM_CURS+3)
			PushCursor(PENDULUM_CURS+currentPend);
		else
			SetTopCursor(PENDULUM_CURS+currentPend);

		if (currentPend == 0)
		{
			currentPend--;
			currentDir = 1;
		}
	}
	SFWTC = True;
}

/************************************************************************
 * SetCursorByLocation - make the cursor fit the window
 ************************************************************************/
void SetCursorByLocation(void)
{
	WindowPtr winWP;
	Point mouse;
	Rect r;
	MyWindowPtr topFloater;
	Rect	rPort;
#ifdef URL_PROTECTION
	Boolean hideTag = true;
#endif //URL_PROTECTION

	if (InBG) return;
	
	PushGWorld();
	
	winWP = FrontWindow_();
	if (winWP) SetPort(GetWindowPort(winWP));
	GetMouse(&mouse);

	if (!MousePen) return;
	SetRectRgn(MousePen,mouse.h,mouse.v,mouse.h+1,mouse.v+1);
	GlobalizeRgn(MousePen);
	
	//If there's a floating window covering this spot, then we need to call
	//the floater's cursor function	
	if (topFloater = FloaterAtPoint(mouse))
	{
		SetPort(GetMyWindowCGrafPtr(topFloater));
		GetMouse(&mouse);
		if (topFloater->cursor)
		  (*topFloater->cursor)(mouse);
	 	else if (PeteCursorList(topFloater->pteList,mouse))
			;
		else
			SetMyCursor(arrowCursor);
	}
	else
	{
		//	Not floating window
		if (!winWP || IsWindowCollapsed(winWP))
		{
			SetMyCursor(arrowCursor);
		}
		else
		{
			GetPortBounds(GetWindowPort(winWP),&rPort);
			if (!PtInRect(mouse,&rPort))
			{
				if (!ToolbarShowing())
				{
					SetMyCursor(arrowCursor);
				}
				else 	ToolbarCursor(mouse);
			}
			else if (IsMyWindow(winWP))
			{
				MyWindowPtr	win;
				
				if (win = GetWindowMyWindowPtr (winWP)) {
					SetPort(GetWindowPort(winWP));
					r = win->contR;
					r.top = 0;
					
#ifdef URL_PROTECTION
					hideTag = !URLHelpTagList(win->pteList,mouse);
#endif //URL_PROTECTION
			
					if (win->cursor)
						(*win->cursor)(mouse);
					else if (PeteCursorList(win->pteList,mouse))
						;
					else
					{
						SetMyCursor(arrowCursor);
					}
				}
			}
			else if (IsPlugwindow(winWP))
			{
				//	Send the plug window a fake mouse moved event in case that's
				//	how it handles cursor setting (e.g. ESP and MLM).
				LocalToGlobal(&mouse);
				PlugwindowSendFakeEvent(winWP, mouseMovedMessage << 24, osEvt, 0, &mouse);
			}
		}
	}
	
#ifdef URL_PROTECTION
	if (hideTag) MyHMHideTag();
#endif //URL_PROTECTION
	
#ifdef NEVER
	{
		GrafPtr wMgrPort;
		
		PushGWorld();
		GetWMgrPort(&wMgrPort);
		SetPort_(wMgrPort);
		InvertRgn(MousePen);
		Pause(10L);
		InvertRgn(MousePen);
		PopGWorld();
	}
#endif
	
	PopGWorld();
	SFWTC = False;
}

/************************************************************************
 * SetMyCursor - clear the cursor stack, and set the cursor
 ************************************************************************/
void SetMyCursor(int cursor)
{
	CSTop = 1;
	SetTopCursor(cursor);
}

/************************************************************************
 * PushCursor - push a cursor on the stack
 ************************************************************************/
void PushCursor(int cursor)
{
	if (CSTop<MAX_CURSOR) CSTop++;
	SetTopCursor(cursor);
	SFWTC = True;
}

/************************************************************************
 * PopCursor - restore to the last cursor on the stack
 ************************************************************************/
void PopCursor(void)
{
	if (CSTop) CSTop--;
	SetTopCursor(CStack[CSTop]);
}

/**********************************************************************
 * change cursors
 **********************************************************************/
void SetTopCursor(int cursor)
{
	Cursor	arrowCurs;
	
	CStack[CSTop] = cursor;
	if (cursor == arrowCursor)
		SetCursor(GetQDGlobalsArrow(&arrowCurs));
	else
	{
		CursHandle curse = GetCursor(cursor);
		short state = HGetState((Handle)curse);
		SetCursor(LDRef(curse));
		HSetState((Handle)curse,state);
	}
}

