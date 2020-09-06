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

#define FILE_NUM 61
/* Copyright (c) 1994 by QUALCOMM Incorporated */
#include <Controls.h>

#include <conf.h>
#include <mydefs.h>

#include "appcdef.h"
#pragma segment Appcdef

typedef long CdefFunc(short varCode, ControlHandle theControl, short message, long param);

CdefFunc AppCdefDraw, AppCdefTest, AppCdefCalc, AppCdefInit,
				 AppCdefDisp, AppCdefTrack;

CdefFunc *AppCdefFuncs[] =
{
	AppCdefDraw,
	AppCdefTest,
	AppCdefCalc,
	AppCdefInit,
	AppCdefDisp,
	nil, /* pos */
	nil, /* thumb */
	nil, /* drag */
	AppCdefTrack, /* track */
	nil, /* mystery */
	AppCdefCalc,
	AppCdefCalc
};

#define kMyCntlDataTag 'MySf'

void AppCdefFillData(short varCode,ControlHandle theControl);
void AppCdefEmptyData(short varCode,ControlHandle theControl);
OSType AppCdefCreator(ControlHandle theControl);
void AppCdefCalcRgnLocal(short varCode,ControlHandle theControl);
Boolean SameIconStuff(MyIconStuff *one, MyIconStuff *two);
void AppCdefBGColor(Rect *r, IconTransformType transform,short depth);
void PlotClippedIconID(Rect *theRect, IconAlignmentType align,
	IconTransformType transform, short theResID);
void PlotClippedIconFromICache(ICacheHandle ich,IconTransformType transform,Rect *inRect);
void PlotClippedIconSuite(Rect *theRect, IconAlignmentType align,
	IconTransformType transform, Handle suite);
void PrepareClip(Rect *theRect,RgnHandle *clip);

/**********************************************************************
 * AppCdef - CDEF for a control that lets the user choose an application
 *
 *	Data - handle to AppData
 **********************************************************************/
pascal long AppCdef(short varCode, ControlHandle theControl, short message, long param)
{
	if (message<(sizeof(AppCdefFuncs)/sizeof(CdefFunc*)) && AppCdefFuncs[message])
		return((*AppCdefFuncs[message])(varCode,theControl,message,param));
	else return(0);
}

/**********************************************************************
 * SameIconStuff - is an icon the same?
 **********************************************************************/
Boolean SameIconStuff(MyIconStuff *one, MyIconStuff *two)
{
	return(	one->id == two->id &&
					one->type == two->type &&
					one->suite == two->suite &&
					one->creator == two->creator);
}

/**********************************************************************
 * AppCdefSetIcon - set the icon for a control
 **********************************************************************/
void AppCdefSetIcon(ControlHandle theControl, short id, OSType type, OSType creator, Handle suite)
{
	MyIconStuff new;
	AppData data;
	Rect r;
	
	AppCdefGetData(theControl,&data);
	
	if (!GetControlDataHandle(theControl)) return;
	
	new.id = id;
	new.type = type;
	new.creator = creator;
	new.suite = suite;
	
	if (!SameIconStuff(&new,&data.cur))
	{
		data.cur = new;
		if (data.shadowRgn) {DisposeRgn(data.shadowRgn);data.shadowRgn=nil;}
		**(AppDataHandle)GetControlDataHandle(theControl) = data;
		r = *GetControlBounds(theControl,&r);
		InvalWindowRect(GetControlOwner(theControl),&r);
	}
}

/**********************************************************************
 * AppCdefDraw - draw the control
 **********************************************************************/
long AppCdefDraw(short varCode, ControlHandle theControl, short message, long param)
{
	Rect r = *GetControlBounds(theControl,&r);
	AppData data;
	Str255 name;
	short transform;
	short depth = RectDepth(&r);
	Boolean noFrame = 0!=(varCode&appCdefNoFrameVC);
	PortSaveStuff stuff;
	Rect r2;
	Boolean d3 = D3?depth:1;
	
	if (!GetControlDataHandle(theControl)) return(-108);
	
	SavePortStuff(&stuff);
	PenNormal();

	/* MJN */
/* 	if ((*theControl)->contrlVis) */
	if (IsControlVisible(theControl) && IsWindowVisible(GetControlOwner(theControl)))
	{
		AppCdefGetData(theControl,&data);
		TextFace(data.style);
		if (!SameIconStuff(&data.old,&data.cur))
		{
			AppCdefCalcRgnLocal(varCode,theControl);
			AppCdefGetData(theControl,&data);
		}
				
		switch (GetControlHilite(theControl))
		{
			case 0: transform=ttNone; break;
			case 255: transform=ttDisabled; break;
			default: transform=ttSelected; break;
		}

		if (noFrame)
		{
			SAVE_STUFF;
			if (depth>=4) RGBBackColor(&data.bgColor);
			EraseRect(&r);
			REST_STUFF;
		}
		else
		{
			if (depth>=4) AppCdefBGColor(&r,transform,depth);
			InsetRect(&r,1,1); EraseRect(&r); InsetRect(&r,-1,-1);
			Frame3DRectDepth(&r,transform,d3);
		}
		
		if (depth>=4 && data.textP.h && data.textP.v)
		{
			short d = transform==ttSelected ? 1 : -1;
			OffsetRect(&data.iconR,d,d);
			data.textP.h += d;
			data.textP.v += d;
		}

		if (PtInRect(data.textP,&r) && data.textP.h && data.textP.v)
		{
			SAVE_STUFF;
			
			MoveTo(data.textP.h,data.textP.v);
#ifdef NEVER
			if (RunType==Debugging)
			{
				ComposeString(name,"\p%d:",(*theControl)->contrlRfCon);
				DrawString(name);
			}
#endif
			GetControlTitle(theControl,name);
			/*EraseRect(&data.textR);*/
			if (StringWidth(name)>RectWi(data.textR)) data.style |= condense;
			TextFace(data.style);
			TruncString(RectWi(data.textR),name,smTruncMiddle);
			if (ThereIsColor && depth>=4) RGBForeColor(&data.color);
			if (transform==ttDisabled) TextMode(grayishTextOr);
			DrawString(name);
			REST_STUFF;
		}
		
		if (depth<4 && GetControlHilite(theControl) && GetControlHilite(theControl)!=255 &&
				!(varCode&appCdefNoFrameVC))
			HiInvertRect(&r);

		/* checkbox? */
		if ((varCode & appCdefCheckBoxVC) && RectWi(data.iconR) && GetControlValue(theControl))
		{
			SAVE_STUFF;
			
			TextFont(0);
			TextSize(0);
			if (ThereIsColor) RGBForeColor(&data.color);
			
			MoveTo(data.iconR.left-CharWidth(checkMark),data.iconR.top+3*RectHi(data.iconR)/4);
			DrawChar(checkMark);
			
			REST_STUFF;
		}

		if (RectWi(data.iconR))
		{
			if (d3>=4 && (varCode&appCdefNoFrameVC) && !GetControlHilite(theControl))
			{
				if (data.cur.id)
				{
					if (!data.shadowRgn && (data.shadowRgn=NewRgn()))
					{
						r2 = data.iconR;
						OffsetRect(&r2,-r2.left,-r2.top);
						if (data.cur.suite) IconSuiteToRgn(data.shadowRgn,&r2,atAbsoluteCenter,data.cur.suite);
						else IconIDToRgn(data.shadowRgn,&r2,atAbsoluteCenter,data.cur.id);
						if (data.shadowRgn) OffsetRgn(data.shadowRgn,2,2);
						**(AppDataHandle)GetControlDataHandle(theControl) = data;
					}
				}
				if (data.shadowRgn)
				{
					RGBColor shadow = data.bgColor;
					RGBColor oldColor;
					
					GetForeColor(&oldColor);
					DarkenColor(&shadow,30);
					RGBForeColor(&shadow);
					OffsetRgn(data.shadowRgn,data.iconR.left,data.iconR.top);
					PaintRgn(data.shadowRgn);
					OffsetRgn(data.shadowRgn,-data.iconR.left,-data.iconR.top);
					RGBForeColor(&oldColor);
				}
			}
			if (data.cur.suite)
				PlotClippedIconSuite(&data.iconR,atAbsoluteCenter,transform,
									 data.cur.suite);
			else if (data.cur.id)
				PlotClippedIconID(&data.iconR,atAbsoluteCenter,transform,
									 data.cur.id);
			else if (data.cur.type)
				PlotClippedIconFromICache(GetICache(data.cur.type,data.cur.creator),
										transform,
										&data.iconR);
			//else if (depth<4 && (*theControl)->contrlHilite  && (*theControl)->contrlHilite!=255)
				//HiInvertRect(&data.iconR);
		}
		
	}
	SetPortStuff(&stuff);
	DisposePortStuff(&stuff);
	return(0);
}

/**********************************************************************
 * PlotClippedIconID - plot an icon, clipped to the rectangle if need be
 **********************************************************************/
void PlotClippedIconID(Rect *theRect, IconAlignmentType align,
	IconTransformType transform, short theResID)
{
	RgnHandle clip=nil;
	RgnHandle oldClip;
	Rect plotRect;
	
	plotRect = *theRect;
	
	oldClip = SavePortClipRegion(GetQDGlobalsThePort());
	
	PrepareClip(&plotRect,&clip);
	
	PlotIconID(&plotRect,align,transform,theResID);
	
	RestorePortClipRegion(GetQDGlobalsThePort(),oldClip);
	
	if (clip) DisposeRgn(clip);
}

/**********************************************************************
 * PlotClippedIconSuite - plot an icon, clipped to the rectangle if need be
 **********************************************************************/
void PlotClippedIconSuite(Rect *theRect, IconAlignmentType align,
	IconTransformType transform, Handle suite)
{
	RgnHandle clip=nil;
	RgnHandle oldClip;
	Rect plotRect;
	
	plotRect = *theRect;
	
	oldClip = SavePortClipRegion(GetQDGlobalsThePort());
	
	PrepareClip(&plotRect,&clip);
	
	PlotIconSuite(&plotRect,align,transform,suite);
	
	RestorePortClipRegion(GetQDGlobalsThePort(),oldClip);
	
	if (clip) DisposeRgn(clip);
}

/**********************************************************************
 * PlotClippedIconID - plot an icon, clipped to the rectangle if need be
 **********************************************************************/
void PlotClippedIconFromICache(ICacheHandle ich,IconTransformType transform,Rect *inRect)
{
	RgnHandle clip=nil;
	RgnHandle oldClip;
	Rect plotRect;
	
	plotRect = *inRect;
	
	oldClip = SavePortClipRegion(GetQDGlobalsThePort());
	
	PrepareClip(&plotRect,&clip);
	
	PlotIconFromICache(ich,transform,&plotRect);
	
	RestorePortClipRegion(GetQDGlobalsThePort(),oldClip);

	if (clip) DisposeRgn(clip);
}
	
/**********************************************************************
 * PrepareClip - prepare a window's clipping region for drawing a clipped icon
 **********************************************************************/
void PrepareClip(Rect *theRect,RgnHandle *clip)
{
	if (RectWi(*theRect)<16 && (*clip=NewRgn()))
	{
		RectRgn(*clip,theRect);
		SetPortClipRegion(GetQDGlobalsThePort(),*clip);
		theRect->bottom = theRect->top+16;
		theRect->right = theRect->left+16;
	}
}

/**********************************************************************
 * AppCdefBGColor - set the background color for the control
 **********************************************************************/
void AppCdefBGColor(Rect *r, IconTransformType transform,short depth)
{
	short grey;
	
	if (depth<4) return;
	
 	if (depth==4)
		switch(transform)
		{
			case ttNone: grey = k4Grey1; break;
			case ttSelected: grey = k4Grey2; break;
			case ttDisabled: grey = k4Grey1; break;
		}
	else
		switch(transform)
		{
			case ttNone: grey = k8Grey2; break;
			case ttSelected: grey = k8Grey5; break;
			case ttDisabled: grey = k8Grey1; break;
		}
	SetBGGrey(grey);
}


/**********************************************************************
 * AppCdefTest - did the mouse come down in the control?
 **********************************************************************/
long AppCdefTest(short varCode, ControlHandle theControl, short message, long param)
{
	Point mouse;
	
	mouse.v = (param>>16)&0xffff;
	mouse.h = param&0xffff;

	return(PtInControl(mouse,theControl));
}

/**********************************************************************
 * AppCdefCalcRgnLocal - calculate the icon & text region
 **********************************************************************/
void AppCdefCalcRgnLocal(short varCode,ControlHandle theControl)
{
	AppData data;
	Rect r = *GetControlBounds(theControl,&r);
	Str255 name;
	short w;
	
	if (!AppCdefGetData(theControl,&data))
	{
		if (varCode & appCdefCheckBoxVC) r.left += 12;	/* leave room for the checkmark */
		
		if (varCode & appCdefForceIcon)
		{
			w = MIN(RectWi(r),RectHi(r)) - ((varCode&appCdefNoFrameVC)?1:3);
			SetRect(&data.iconR,0,0,w,w);
			CenterRectIn(&data.iconR,&r);
			data.textP.h = data.textP.v = 0;
			SetRect(&data.textR,0,0,0,0);
		}
		else
		{
			GetControlTitle(theControl,name);
			NamedIconCalc(&r,name,4,4,&data.iconR,&data.textP);
			data.textP.h = MAX(data.textP.h,r.left+6);
			data.textR.left = data.textP.h;
			data.textR.bottom = data.textP.v+4/*fudge*/;
			data.textR.top = data.textR.bottom-4/*fudge*/-10/*fudge*/;
			data.textR.right = data.textR.left+StringWidth(name);
			data.textR.right = MIN(r.right-INSET/2,data.textR.right);
		}
		**(AppDataHandle)GetControlDataHandle(theControl) = data;
	}
}


/**********************************************************************
 * AppCdefGetData - extract the private data from the CDEF
 **********************************************************************/
OSErr AppCdefGetData(ControlHandle theControl,AppDataPtr data)
{
	if ((AppDataHandle)GetControlDataHandle(theControl))
	{
		*data = **(AppDataHandle)GetControlDataHandle(theControl);
		return(noErr);
	}
	return(fnfErr);
}

/**********************************************************************
 * AppCdefCalc - calculate control regions
 **********************************************************************/
long AppCdefCalc(short varCode, ControlHandle theControl, short message, long param)
{
	//AppData data;
	Rect r = *GetControlBounds(theControl,&r);
	
	if (message==calcCRgns) param &= 0x00ffffff;
	AppCdefCalcRgnLocal(varCode,theControl);
#ifdef NEVER
	if (!AppCdefGetData(theControl,&data))
	{
		RectRgn((RgnHandle)param,&data.iconR);
		RgnPlusRect((RgnHandle)param,&data.textR);
	}
	else
#endif
		RectRgn((RgnHandle)param,&r);
	return(0);
}

/**********************************************************************
 * AppCdefSetColor - set the color for the control
 **********************************************************************/
void AppCdefSetColor(ControlHandle theControl, RGBColor *color)
{
	Rect r;

	if (!GetControlDataHandle(theControl)) return;
	
	(*(AppDataHandle)GetControlDataHandle(theControl))->color = *color;
	r = *GetControlBounds(theControl,&r);
	InvalWindowRect(GetControlOwner(theControl),&r);
}

/**********************************************************************
 * AppCdefSetStyle - set the style for the control
 **********************************************************************/
void AppCdefSetStyle(ControlHandle theControl, Style style)
{
	Rect r;
	AppDataHandle	hAppData;

	if (!(hAppData = (AppDataHandle)GetControlDataHandle(theControl))) return;
	
	if ((*hAppData)->style != style &&
			(*hAppData)->textP.h &&
			(*hAppData)->textP.v)
	{
		(*hAppData)->style = style;
		r = *GetControlBounds(theControl,&r);
		InvalWindowRect(GetControlOwner(theControl),&r);
	}
}

/**********************************************************************
 * AppCdefSetBGColor - set the background color for the control
 **********************************************************************/
void AppCdefSetBGColor(ControlHandle theControl, RGBColor *color)
{
	Rect r;
	
	if (!GetControlDataHandle(theControl)) return;
	(*(AppDataHandle)GetControlDataHandle(theControl))->bgColor = *color;
	r = *GetControlBounds(theControl,&r);
	InvalWindowRect(GetControlOwner(theControl),&r);
}

/**********************************************************************
 * AppCdefInit - initialize stuff for the CDEF
 **********************************************************************/
long AppCdefInit(short varCode, ControlHandle theControl, short message, long param)
{
	AppDataHandle data = NewZH(AppData);
	long	theTag = kMyCntlDataTag;
	
	if (!data) return(MemError());
	SetControlAction(theControl,(void*)-1);
	SetControlDataHandle(theControl,(Handle)data);	/* a place to store my stuff */
	if (ThereIsColor)
	{
		GetBackColor(&LDRef(data)->bgColor);
		UL(data);
	}
	
	SetControlData (theControl,0,kMyCntlDataTag,sizeof(theTag),(void*)&theTag);
	return(0);
}

/**********************************************************************
 * AppCdefDisp - dispose the control
 **********************************************************************/
long AppCdefDisp(short varCode, ControlHandle theControl, short message, long param)
{
	AppData data;
	
	if (!AppCdefGetData(theControl,&data))
	{
		if (data.shadowRgn) DisposeRgn(data.shadowRgn);
		if (data.cur.suite) DisposeIconSuite(data.cur.suite,True);
	}
	
	DisposeHandle(GetControlDataHandle(theControl));
	return(0);
}



/**********************************************************************
 * AppCdefBGChange - change background color for top margin controls
 **********************************************************************/
void AppCdefBGChange(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl;
	RGBColor color;
	Rect r;
	long	theTag;
	Size	theSize;
	
	if (ThereIsColor)
		for (cntl=GetControlList(winWP);cntl;cntl=GetNextControl(cntl))
		{
			if (!GetControlData(cntl,0,kMyCntlDataTag,sizeof(theTag),(void*)&theTag,&theSize)
				&& theTag == kMyCntlDataTag)
			{
				r = *GetControlBounds(cntl,&r);
				AppCdefSetBGColor(cntl,EffectiveBGColor(&color,win->label,&r,RectDepth(&r)));
			}
		}
}


/**********************************************************************
 * 
 **********************************************************************/
long AppCdefTrack(short varCode, ControlHandle theControl, short message, long param)
{
	return(0);
}

