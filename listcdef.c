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

// Copyright � 1996 by QUALCOMM Incorporated
#define FILE_NUM 84
#include "listcdef.h"

#pragma segment ListCdef

typedef long CdefFunc(short varCode, ControlHandle theControl, short message, long param);

CdefFunc ListCdefDraw, ListCdefTest, ListCdefCalc, ListCdefInit,
				 ListCdefDisp, ListCdefTrack;

CdefFunc *ListCdefFuncs[] =
{
	ListCdefDraw,
	ListCdefTest,
	ListCdefCalc,
	ListCdefInit,
	ListCdefDisp,
	nil, /* pos */
	nil, /* thumb */
	nil, /* drag */
	ListCdefTrack, /* track */
	nil, /* mystery */
	ListCdefCalc,
	ListCdefCalc
};

static callingLClick;

/**********************************************************************
 * LCGetList - get the list out of the control
 **********************************************************************/
ListHandle LCGetList(ControlHandle cntl)
{
	return((ListHandle)GetControlDataHandle(cntl));
}

/**********************************************************************
 * LCSetList - set the list for a control
 **********************************************************************/
void LCSetList(ControlHandle cntl,ListHandle list)
{
	if (LCGetList(cntl)) LDispose(LCGetList(cntl));
	SetControlDataHandle(cntl,(void*)list);
}

/**********************************************************************
 * ListCdef - CDEF for a control that gives a list manager box
 **********************************************************************/
pascal long ListCdef(short varCode, ControlHandle theControl, short message, long param)
{
	if (message<(sizeof(ListCdefFuncs)/sizeof(CdefFunc*)) && ListCdefFuncs[message])
		return((*ListCdefFuncs[message])(varCode,theControl,message,param));
	else return(0);
}

/**********************************************************************
 * ListCdefDraw - draw the list cdef
 **********************************************************************/
long ListCdefDraw(short varCode, ControlHandle theControl, short message, long param)
{
	Rect r;
	ListHandle list = LCGetList(theControl);

	GetControlBounds(theControl,&r);
	if (list) LUpdate(MyGetPortVisibleRegion(GetWindowPort(GetControlOwner(theControl))),list);
	FrameRect(&r);
	return(0);
}

/**********************************************************************
 * ListCdefTest - test a point to see if it's in the CDEF
 **********************************************************************/
long ListCdefTest(short varCode, ControlHandle theControl, short message, long param)
{
	Point mouse;
	Rect r;
	
	GetControlBounds(theControl,&r);
	mouse.v = (param>>16)&0xffff;
	mouse.h = param&0xffff;

	/*
	 * make sure that LClick sees its scroll bar rather
	 * than this control
	 */
	if (callingLClick) r.right -= GROW_SIZE;
	
	return(PtInRect(mouse,&r));
}

/**********************************************************************
 * ListCdefCalc - calculate region for cdef
 **********************************************************************/
long ListCdefCalc(short varCode, ControlHandle theControl, short message, long param)
{
	Rect r;
	
	GetControlBounds(theControl,&r);
	if (message==calcCRgns) param &= 0x00ffffff;
	RectRgn((RgnHandle)param,&r);
	return(0);
}

/**********************************************************************
 * ListCdefInit - initialize
 **********************************************************************/
long ListCdefInit(short varCode, ControlHandle theControl, short message, long param)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (GetControlOwner(theControl));
	ListHandle list;
	Point cSize;
	Rect bounds;
	Rect r;
	OSErr err = noErr;
	
	// Calculate sizes
	GetControlBounds(theControl,&r);
	InsetRect(&r,1,1);
	r.right -= GROW_SIZE;
	SetRect(&bounds,0,0,1,0);
	cSize.h = RectWi(r);
	cSize.v = win ? win->vPitch + 2 : 0;
	if (list = LNew(&r,&bounds,cSize,nil,GetControlOwner(theControl),True,False,False,True))
	{
		LCSetList(theControl,list);
		if (win) (*list)->indent.v = win->vPitch-2;
	}
	else err = MemError();
	SetControlAction(theControl,(void*)-1);
	
	return(err);
}

/**********************************************************************
 * ListCdefDisp - dispose of the list cdef
 **********************************************************************/
long ListCdefDisp(short varCode, ControlHandle theControl, short message, long param)
{
	ListHandle list = LCGetList(theControl);
	if (list)
	{
		(*list)->hScroll = (*list)->vScroll = nil;
		LDispose(list);
	}
	return(0);
}

/**********************************************************************
 * ListCdefTrack - track the mouse
 **********************************************************************/
long ListCdefTrack(short varCode, ControlHandle theControl, short message, long param)
{
	Point pt = MainEvent.where;
	ListHandle list = LCGetList(theControl);
	callingLClick = True;	// make sure that LClick sees its scroll bar rather
										// than this control
	GlobalToLocal(&pt);
	LClick(pt, MainEvent.modifiers, list);
	callingLClick = False;
	return(1);
}
