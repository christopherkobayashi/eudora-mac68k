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

#include <mailbox.h>
#include <mywindow.h>

#define FILE_NUM 71
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#pragma segment Main

typedef long CdefFunc(short varCode, ControlHandle theControl, short message, long param);

CdefFunc ColCdefDraw, ColCdefCalc, ColCdefTest, ColCdefInit;

CdefFunc *ColCdefFuncs[] =
{
	ColCdefDraw,
	ColCdefTest,
	ColCdefCalc,
	ColCdefInit,
	nil,
	nil, /* pos */
	nil, /* thumb */
	nil, /* drag */
	nil, /* track */
	nil, /* mystery */
	ColCdefCalc,
	ColCdefCalc
};

/**********************************************************************
 * ColCdef - CDEF for a control that lets the user choose an application
 *
 *	Data - handle to AppData
 **********************************************************************/
pascal long ColCdef(short varCode, ControlHandle theControl, short message, long param)
{
	if (message<(sizeof(ColCdefFuncs)/sizeof(CdefFunc*)) && ColCdefFuncs[message])
		return((*ColCdefFuncs[message])(varCode,theControl,message,param));
	else return(0);
}

#pragma segment ColorCdef

/**********************************************************************
 * ColCdefDraw - draw the control
 **********************************************************************/
long ColCdefDraw(short varCode, ControlHandle theControl, short message, long param)
{
	Rect r = *GetControlBounds(theControl,&r);
	RGBColor color, oldColor;
	short hilite = GetControlHilite(theControl);
	Pattern	thePattern;
	
	FrameRect(&r);


	if (hilite && hilite!=255) PenPat(GetQDGlobalsBlack(&thePattern));
	else PenPat(GetQDGlobalsWhite(&thePattern));
	
	InsetRect(&r,1,1);
	FrameRect(&r);
	InsetRect(&r,2,2);
	
	PenNormal();
	
	ColCtlGet(theControl,&color);
		
	if (ThereIsColor)
	{
		GetForeColor(&oldColor);
		RGBForeColor(&color);
	}
	
	PaintRect(&r);
	
	if (ThereIsColor) RGBForeColor(&oldColor);

	return(0);
}

/**********************************************************************
 * ColCdefTest - did the mouse come down in the control?
 **********************************************************************/
long ColCdefTest(short varCode, ControlHandle theControl, short message, long param)
{
	Point mouse;
	Rect r;
	
	mouse.v = (param>>16)&0xffff;
	mouse.h = param&0xffff;

	r = *GetControlBounds(theControl,&r);
	return(PtInRect(mouse,&r));
}


/**********************************************************************
 * ColCdefCalc - calculate control regions
 **********************************************************************/
long ColCdefCalc(short varCode, ControlHandle theControl, short message, long param)
{
	Rect r = *GetControlBounds(theControl,&r);
	
	if (message==calcCRgns) param &= 0x00ffffff;
	RectRgn((RgnHandle)param,&r);
	return(0);
}

/**********************************************************************
 * ColCdefInit - initialize stuff for the CDEF
 **********************************************************************/
long ColCdefInit(short varCode, ControlHandle theControl, short message, long param)
{
	SetControlAction(theControl,(void*)-1);
	return(0);
}

