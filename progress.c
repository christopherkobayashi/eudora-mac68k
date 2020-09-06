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

#include "progress.h"

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * Progress monitoring software
 ************************************************************************/

#define FILE_NUM 33
#ifndef TASK_PROGRESS_ON
#define PROG_BAR_HI 12
#define PROG_BOX_HI 16
#endif
#pragma segment Progress

/************************************************************************
 * other stuff
 ************************************************************************/

Boolean ProgressOff;

/************************************************************************
 * Private functions
 ************************************************************************/
void DisposProgress(ProgressBHandle prbl);
void InvalProgress(ProgressRectEnum which);

void ProgressButton(MyWindowPtr win, ControlHandle button, long modifiers,
		    short part);
void InstallProgMessage(PStr string, ProgressRectEnum which);
Boolean ProgPosition(Boolean save, MyWindowPtr win);
Boolean ProgClose(MyWindowPtr win);

ProgressBlock **GetPrbl(MyWindowPtr win);
void SetPrbl(MyWindowPtr win, ProgressBlock ** prbl);

/**********************************************************************
 * ProgressR - progress, but with resource id's for title/subtitle
 **********************************************************************/
void ProgressR(short percent, short remaining, short titleId,
	       short subTitleId, PStr message)
{
	Str255 title, subTitle;

	*title = *subTitle = 0;

	if (titleId)
		GetRString(title, titleId);
	if (subTitleId)
		GetRString(subTitle, subTitleId);
	Progress(percent, remaining, titleId ? title : nil,
		 subTitleId ? subTitle : nil, message);
}


/**********************************************************************
 * ProgressMessage - put something in one section of progress
 **********************************************************************/
void ProgressMessage(short which, PStr message)
{
	UPtr msgs[kpTitle + 1];
	Str15 empty;
	ProgressRectEnum remaining;

	*empty = 0;
	remaining = which == kpMessage ? NoChange : NoBar;
	WriteZero(msgs, sizeof(msgs));
	msgs[which] = message;
	while (which)
		msgs[--which] = empty;
	Progress(remaining, remaining, msgs[kpTitle], msgs[kpSubTitle],
		 msgs[kpMessage]);
}

#pragma segment Progress


/************************************************************************
 * GetPrbl - get appropriate ProgressBlock
 ************************************************************************/
ProgressBlock **GetPrbl(MyWindowPtr win)
{
#ifndef TASK_PROGRESS_ON
	return ((ProgressBlock **) GetMyWindowPrivateData(ProgWindow);
#else
	ProgressBlock **prbl;

	if (win && (win != TaskProgressWindow))
		prbl =
		    ProgWindow ? (ProgressBlock **)
		    GetMyWindowPrivateData(ProgWindow) : nil;
	else
		prbl = GetCurrentThreadPrbl();
	return (prbl);
#endif
}

/************************************************************************
 * SetPrbl - set appropriate ProgressBlock
 ************************************************************************/
void SetPrbl(MyWindowPtr win, ProgressBlock ** prbl)
{
#ifndef TASK_PROGRESS_ON
	SetMyWindowPrivateData(win, (long) prbl);
#else
	if (win && (win != TaskProgressWindow)) {
		SetMyWindowPrivateData(win, (long) prbl);
	} else {
		threadDataHandle threadData = nil;

		GetCurrentThreadData(&threadData);
		if (threadData)
			(*threadData)->prbl = prbl;
	}
#endif
}


/**********************************************************************
 * ProgressMessageR - like progressmessage, but with resource id instead
 **********************************************************************/
void ProgressMessageR(short which, short messageId)
{
	Str255 message;

	ProgressMessage(which, GetRString(message, messageId));
}

/************************************************************************
 * GetProgressBytes - return the number of bytes transmitted so far
 ************************************************************************/
int GetProgressBytes(void)
{
	ProgressBlock **prbl;
	if (prbl = (ProgressBlock **) GetPrbl(ProgWindow))
		return ((*prbl)->on);
	return 0;
}

/************************************************************************
 * ByteProgressExcess - we under-estimated
 ************************************************************************/
void ByteProgressExcess(int excess)
{
	ProgressBlock **prbl;
	if (prbl = (ProgressBlock **) GetPrbl(ProgWindow))
		(*prbl)->excessOn += excess;
}

/************************************************************************
 * ByteProgress - keep track of the number of bytes transmitted so far
 ************************************************************************/
void ByteProgress(UPtr message, int onLine, int totLines)
{
	ProgressBlock **prbl;
	static long lastTicks;

	CycleBalls();

	if (prbl = (ProgressBlock **) GetPrbl(ProgWindow)) {
		if (onLine >= 0) {
			(*prbl)->on = onLine;
			(*prbl)->excessOn = 0;
			lastTicks = 0;
		} else {
			if ((*prbl)->excessOn > 0) {
				if (((*prbl)->excessOn -= onLine) < 0)
					onLine = (*prbl)->excessOn;
				else
					onLine = 0;
			}
			(*prbl)->on -= onLine;
		}
		if (totLines) {
			(*prbl)->excessOn = 0;
			(*prbl)->total = totLines;
			lastTicks = 0;
		}

		if (!(*prbl)->total)
			return;

		if (TickCount() - lastTicks > 10) {
			lastTicks = TickCount();
			ASSERT((*prbl)->total != 0);
			Progress((100 * (*prbl)->on) / (*prbl)->total,
				 NoChange, nil, nil, message);
		}
	}
}

/************************************************************************
 * OpenProgress - create the progress window
 ************************************************************************/
int OpenProgress(void)
{
	int err;
	ProgressBlock **prbl;
	MyWindowPtr win;
	WindowPtr winWP;
	ControlHandle stopH;
	short v;

#ifdef TASK_PROGRESS_ON
	if (InAThread()) {
		if (TaskProgressWindow
		    &&
		    GetWindowKind(GetMyWindowWindowPtr(TaskProgressWindow))
		    == TASKS_WIN)
			InvalContent(TaskProgressWindow);
		return (0);
	}
#endif
	if (ProgWindow || PrefIsSet(PREF_NO_PROGRESS))
		return (0);
	if ((win =
	     GetNewMyWindow(PROGRESS_WIND, nil, nil, InFront, False, False,
			    PROG_WIN)) == nil) {
		WarnUser(COULDNT_WIN, err = MemError());
		return (err);
	}

	winWP = GetMyWindowWindowPtr(win);
	MySetThemeWindowBackground(win,
				   kThemeActiveModelessDialogBackgroundBrush,
				   False);

	InsetRect(&win->contR, 6, 6);

	prbl = NewZH(ProgressBlock);
	stopH = GetNewControl(STOP_CNTL, winWP);
	if (!prbl || !stopH) {
		WarnUser(MEM_ERR, err = MemError());
		DisposeWindow_(winWP);
		return (err);
	}
	(*prbl)->stop = stopH;

	/*
	 * set rectangles
	 */
	LDRef(prbl);

	v = INSET;
	SetRect(&(*prbl)->rects[kpSubTitle], win->contR.left, v,
		win->contR.right - (3 * ControlWi(stopH)) / 2 - INSET,
		v + PROG_BOX_HI);

	SetRect(&(*prbl)->rects[kpRemaining],
		(*prbl)->rects[kpSubTitle].right + INSET, v,
		win->contR.right - INSET, v + PROG_BOX_HI);

	v += PROG_BOX_HI + INSET;
	SetRect(&(*prbl)->rects[kpMessage], win->contR.left, v,
		win->contR.right - INSET, v + PROG_BOX_HI);

	v += PROG_BOX_HI + INSET;
	SetRect(&(*prbl)->rects[kpBar], win->contR.left, v,
		win->contR.right - ControlWi(stopH) - INSET,
		v + PROG_BAR_HI);
	(*prbl)->bar =
	    NewControlSmall(winWP, &(*prbl)->rects[kpBar], "", false, 0, 0,
			    100, kControlProgressBarProc, 0);

	UL(prbl);

	/*
	 * move the stop button
	 */
	MoveMyCntl(stopH, (*prbl)->rects[kpBar].right + 6,
		   (*prbl)->rects[kpBar].top - (ControlHi(stopH) - 12) / 2,
		   ControlWi(stopH), ControlHi(stopH));

	/*
	 * and the rest
	 */
	TextFont(0);
	TextSize(0);
	SetMyWindowPrivateData(win, (long) prbl);
	win->update = ProgressUpdate;
	win->button = ProgressButton;
	win->position = ProgPosition;
	win->close = ProgClose;
	(*prbl)->percent = -1;
	SetPort_(GetWindowPort(winWP));
	ProgWindow = win;
	win->isRunt = True;
	ShowMyWindow(winWP);
	PushModalWindow(winWP);
	EnableMenus(winWP, False);
	return (noErr);
}

#pragma segment Progress

/**********************************************************************
 * ProgressButton - handle the stop button
 **********************************************************************/
void ProgressButton(MyWindowPtr win, ControlHandle button, long modifiers,
		    short part)
{
	short windowKind = GetWindowKind(GetMyWindowWindowPtr(win));

	if (part == kControlButtonPart) {
		CommandPeriod = True;
		AuditHit((modifiers & shiftKey) != 0,
			 (modifiers & controlKey) != 0,
			 (modifiers & optionKey) != 0,
			 (modifiers & cmdKey) != 0, false, windowKind,
			 AUDITCONTROLID(windowKind, kControlButtonPart),
			 mouseDown);
	}
}

/************************************************************************
 * CloseProgress - close the progress window
 ************************************************************************/
void CloseProgress(void)
{
#ifdef TASK_PROGRESS_ON
	if (ProgWindow && (ProgWindow != TaskProgressWindow))
		CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#else
	if (ProgWindow)
		CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#endif
}

/**********************************************************************
 * ProgClose - deal with closing the progress window
 **********************************************************************/
Boolean ProgClose(MyWindowPtr win)
{
	DisposProgress((ProgressBHandle) GetMyWindowPrivateData(win));
	SetMyWindowPrivateData(win, nil);
	ProgWindow = nil;
	if (!InBG)
		FlushEvents(keyDownMask, 0);
	return (True);
}

/************************************************************************
 * DisposProgress - get rid of the progress chain
 ************************************************************************/
void DisposProgress(ProgressBHandle prbl)
{
	if (prbl == nil)
		return;
	if ((*prbl)->bar) {
		SetControlVisibility((*prbl)->bar, false, false);
		DisposeControl((*prbl)->bar);
		(*prbl)->bar = nil;
	}
	if ((*prbl)->next)
		DisposProgress((*prbl)->next);
	ZapHandle(prbl);
}

/************************************************************************
 * Progress - record progress in the progress window
 ************************************************************************/
 // thread-aware version

void Progress(short percent, short remaining, PStr title, PStr subTitle,
	      PStr message)
{
	WindowPtr ProgWindowWP = GetMyWindowWindowPtr(ProgWindow);
	ProgressBlock **prbl;
	Str255 scratch;

	MiniEvents();

#ifdef TASK_PROGRESS_ON
	if (title)
		InstallProgMessage(title, kpTitle);
#endif

	if (!(prbl = (ProgressBlock **) GetPrbl(ProgWindow)))
		return;
	if (percent != NoChange && percent != (*prbl)->percent) {
		(*prbl)->percent = percent;
		if (percent == NoBar) {
			if ((*prbl)->bar)
				SetControlVisibility((*prbl)->bar, false,
						     false);
//                              HideControl((*prbl)->bar);
		} else if ((*prbl)->bar) {
			SetControlVisibility((*prbl)->bar, false, false);
			SetControlValue((*prbl)->bar, percent);
			SetControlVisibility((*prbl)->bar, true, false);
#ifdef TASK_PROGRESS_ON
			if (ProgWindow == TaskProgressWindow)
				DrawTaskProgressBar((*prbl)->bar);
			else
#endif
				ShowControl((*prbl)->bar);
		}
	}

	InstallProgMessage(message, kpMessage);
	InstallProgMessage(subTitle, kpSubTitle);
	if (remaining != NoChange) {
		if (remaining > 0)
			NumToString(remaining, scratch);
		else
			*scratch = 0;
		InstallProgMessage(scratch, kpRemaining);
	}

	if (!ProgWindow)
		return;

	if (title) {
#ifdef TASK_PROGRESS_ON
		if (ProgWindow == TaskProgressWindow)
			PSCopy((*prbl)->title, title);
		else {
#endif
			MyGetWTitle(ProgWindowWP, scratch);
			if (!EqualString(title, scratch, True, True))
				SetWTitle_(ProgWindowWP, title);
#ifdef TASK_PROGRESS_ON
		}
#endif
	}
#ifdef THREADING_ON
	if (!InAThread())
#endif
		if (FrontWindow_() != ProgWindowWP)
			UserSelectWindow(ProgWindowWP);
	UpdateMyWindow(ProgWindowWP);
}

/************************************************************************
 * ProgPosition - ph window position
 ************************************************************************/
Boolean ProgPosition(Boolean save, MyWindowPtr win)
{
	Str31 progress;

	SetWTitle_(GetMyWindowWindowPtr(win),
		   GetRString(progress, PROGRESS));
	return (PositionPrefsTitle(save, win));
}

/**********************************************************************
 * InstallProgressMessage - install a progress message
 **********************************************************************/
void InstallProgMessage(PStr string, ProgressRectEnum which)
{
	Str255 scratch;
	ProgressBlock **prbl;

	if (!(prbl = (ProgressBlock **) GetPrbl(ProgWindow)))
		return;

	if (string) {
		PCopyTrim(scratch, string, sizeof(scratch));
		LDRef(prbl);
		if (!StringSame(scratch, (*prbl)->messages[which])) {
			PCopy((*prbl)->messages[which], scratch);
			InvalProgress(which);
			if (which == kpMessage)
				Log(LOG_PROG, scratch);
		}
		UL(prbl);
	}
}

/************************************************************************
 * InvalProgress - invalidate the selected part of the progress window
 ************************************************************************/
void InvalProgress(ProgressRectEnum which)
{
	ProgressBlock **prbl;
	short start, stop;

#ifdef TASK_PROGRESS_ON
	if (!ProgWindow)
		return;
	if (ProgWindow == TaskProgressWindow
	    && GetWindowKind(GetMyWindowWindowPtr(TaskProgressWindow)) !=
	    TASKS_WIN)
		return;
#endif
	if (!(prbl = (ProgressBlock **) GetPrbl(ProgWindow)))
		return;
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(ProgWindow));
	if (which == kpTitle) {
		start = 0;
		stop = kpTitle;
	} else
		start = stop = which;
	for (; start <= stop; start++) {
		Rect textRect = (*prbl)->rects[start];
#ifdef TASK_PROGRESS_ON
		if (ProgWindow == TaskProgressWindow)
			InvalTPRect(&textRect);
		else
#endif
			InvalWindowRect(GetMyWindowWindowPtr(ProgWindow),
					&textRect);
	}
	PopGWorld();
}


/************************************************************************
 * ProgressUpdate - update the progress window
 ************************************************************************/
void ProgressUpdate(MyWindowPtr win)
{
	ProgressBlock **prbl;
	ProgressRectEnum which;

	if (!(prbl = (ProgressBlock **) GetPrbl(win)))
		return;

	ClipRect(&win->contR);
	EraseRect(&win->contR);

	SetThemeTextColor(win->
			  isActive ? kThemeActiveModelessDialogTextColor :
			  kThemeInactiveModelessDialogTextColor,
			  RectDepth(&win->contR), True);

	LDRef(prbl);
	for (which = 0; which < kpTitle; which++) {
		MoveTo((*prbl)->rects[which].left,
		       (*prbl)->rects[which].bottom - 4);
		if (which == kpMessage) {
			SetSmallSysFont();
		}
		ClipRect(&(*prbl)->rects[which]);
		DrawString((*prbl)->messages[which]);
		TextFont(0);
		TextSize(0);
	}
	ClipRect(&win->contR);
	UL(prbl);
}

/************************************************************************
 * PushProgress - stash a copy of the progress info
 ************************************************************************/
void PushProgress(void)
{
	ProgressBHandle prbl;
	ProgressBHandle pH;

	if (!ProgWindow)
		return;

	if (!(prbl = (ProgressBlock **) GetPrbl(ProgWindow)))
		return;

	UL(prbl);
	MyGetWTitle(GetMyWindowWindowPtr(ProgWindow), (*prbl)->title);
	UL(prbl);
	if (pH = NuHandle(sizeof(ProgressBlock))) {
		**pH = **prbl;
		(*pH)->next = prbl;
		SetPrbl(ProgWindow, pH);
	}
}

/************************************************************************
 * PopProgress - restore the progress info
 ************************************************************************/
void PopProgress(Boolean messageOnly)
{
	WindowPtr ProgWindowWP = GetMyWindowWindowPtr(ProgWindow);
	ProgressBHandle prbl;
	ProgressBHandle pNext;

	if (!ProgWindow)
		return;

	if (!(prbl = (ProgressBlock **) GetPrbl(ProgWindow)))
		return;

	if ((*prbl)->next) {
		pNext = (*prbl)->next;
		(*pNext)->percent = (*prbl)->percent;
		(*pNext)->on = (*prbl)->on;
		(*pNext)->total = (*prbl)->total;
		SetWTitle_(ProgWindowWP, LDRef(pNext)->title);
		UL(pNext);
		if ((*prbl)->bar) {
			SetControlValue((*prbl)->bar, (*prbl)->percent);
			if ((*prbl)->percent == NoBar)
				SetControlVisibility((*prbl)->bar, false,
						     true);
//                              HideControl((*prbl)->bar);
			else {
				SetControlVisibility((*prbl)->bar, true,
						     false);
#ifdef TASK_PROGRESS_ON
				if (ProgWindow == TaskProgressWindow)
					DrawTaskProgressBar((*prbl)->bar);
				else
#endif
					ShowControl((*prbl)->bar);
			}
		}

		SetPrbl(ProgWindow, pNext);
		InvalProgress(kpTitle);
		ZapHandle(prbl);
		UpdateMyWindow(ProgWindowWP);
	}
}

/**********************************************************************
 * PressStop - press the stop button
 **********************************************************************/
void PressStop(void)
{
	WindowPtr ProgWindowWP = GetMyWindowWindowPtr(ProgWindow);
	ProgressBlock **prbl = (ProgressBlock **) GetPrbl(ProgWindow);
	ControlRef stopH;

	if (ProgWindow && prbl && (stopH = (*prbl)->stop)) {
		HiliteControl(stopH, 1);
		Pause(20L);
		HiliteControl(stopH, 0);
	}
}

#pragma segment Main

/************************************************************************
 * ProgressIsOpen - is the progress window open?
 ************************************************************************/
Boolean ProgressIsOpen(void)
{
	return (ProgWindow != nil);
}

/************************************************************************
 * ProgressIsTop - is the progress window topmost?
 ************************************************************************/
Boolean ProgressIsTop(void)
{
	return (ProgWindow != nil
		&& GetMyWindowWindowPtr(ProgWindow) == FrontWindow_());
}
