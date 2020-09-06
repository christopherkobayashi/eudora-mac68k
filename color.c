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

#include "color.h"
#define FILE_NUM 59
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#ifdef TWO
#pragma segment Color

	uShort ChangeByPercent(uShort value,short percent);

/************************************************************************
 * SetMenuColor - set the color of a menu item
 ************************************************************************/
void SetMenuColor(MenuHandle menu, short item, RGBColor *color)
{
	MCEntry mcEntry;
	MCEntry *defaultMC;
	
	defaultMC = GetMCEntry(0,0);
	if (defaultMC)
		mcEntry.mctRGB4 = defaultMC->mctRGB4;
	else
		mcEntry.mctRGB4.red = mcEntry.mctRGB4.blue = mcEntry.mctRGB4.green = 0xffff;
	mcEntry.mctID = GetMenuID(menu);
	mcEntry.mctItem = item;
	mcEntry.mctRGB1 = mcEntry.mctRGB2 = mcEntry.mctRGB3 = *color;
	SetMCEntries(1,&mcEntry);
}

/************************************************************************
 * GetLabelColor - return the color of a label
 ************************************************************************/
RGBColor *GetLabelColor(short index,RGBColor *color)
{
	Str255 label;
	
	MyGetLabel(index,color,label);
	return(color);
}

/************************************************************************
 * MyGetLabel - get a label description from the Finder
 ************************************************************************/
OSErr MyGetLabel(short index,RGBColor *color,PStr label)
{
	OSErr err;
	
	if (index<8)
	{
		short fIndex = index ? 8-index:0;
		err = GetLabel(fIndex,color,label);
//		ComposeLogS(-1,nil,"\pindex %d RGB %d.%d.%d Name %p",
//			fIndex, color->red/256, color->green/256, color->blue/256, label);
	}
	else
	{
		GetRString(label,PrivLabelsStrn+index-7);
		GetRColor(color,PrivColorsStrn+index-7);
		err = noErr;
	}
	return err;
}

/************************************************************************
 * LightenColor - make a color lighter
 ************************************************************************/
RGBColor *LightenColor(RGBColor *color,short percent)
{
	HSVColor hsv;
	
	RGB2HSV(color,&hsv);
	hsv.value = ChangeByPercent(hsv.value,percent);
	hsv.saturation = ChangeByPercent(hsv.saturation,-percent);
	HSV2RGB(&hsv,color);
	return(color);
}

/************************************************************************
 * ColorIsLight - is a color light?
 ************************************************************************/
Boolean ColorIsLight(RGBColor *color)
{
	long light = GetRLong(LIGHT_COLOR);
	
	return(color->red>light && color->blue>light && color->green>light);
}


/**********************************************************************
 * Color3DRect - color a 3D rectangle, based on some color
 **********************************************************************/
void Color3DRect(Rect *inRect, RGBColor *color, D3EffectEnum howMuch, Boolean raised)
{
	RGBColor colors[2];
	RGBColor *light, *dark;
	Rect r = *inRect;
	
	if (raised)
	{
		light = &colors[0];
		dark = &colors[1];
	}
	else
	{
		light = &colors[1];
		dark = &colors[0];
	}
	*light = *dark = *color;
	
	switch (howMuch)
	{
		case e3DNone:	break;	/* none of that crap */
		
		case e3DSlight:
			LightenColor(&colors[0],30); DarkenColor(&colors[1],60);
			TwoToneFrame(&r,light,dark);
			InsetRect(&r,1,1);
			break;
		
		case e3DOverBearing:
			LightenColor(&colors[0],40); DarkenColor(&colors[1],60);
			TwoToneFrame(&r,light,dark);
			InsetRect(&r,1,1);
			*light = *dark = *color;
			LightenColor(&colors[0],20); DarkenColor(&colors[1],20);
			TwoToneFrame(&r,light,dark);
			InsetRect(&r,1,1);
			break;
	}
	
	if (raised)
	{
		RGBColor oldColor;
		
		GetForeColor(&oldColor);
		RGBForeColor(color);
		PaintRect(&r);
		RGBForeColor(&oldColor);
	}
}

/**********************************************************************
 * Frame3DOrNot - frame something, possibly in 3D
 **********************************************************************/
void Frame3DOrNot(Rect *r, RGBColor *baseColor,Boolean erase)
{
	short depth;
	RGBColor color;
	
	InsetRect(r,-2,-2);

	if (D3 && (depth=RectDepth(r))>=4)
	{
		if (baseColor) color = *baseColor;
		else SetRGBGrey(&color,depth==4?k4Grey1:k8Grey1);
		Color3DRect(r,&color,e3DSlight,False);
	}

	InsetRect(r,1,1);
	FrameRect(r);
	
	if (erase)
	{
		InsetRect(r,1,1);
		EraseRect(r);
	}
}

/**********************************************************************
 * WinGreyBG - set the right grey for a window's background
 **********************************************************************/
void WinGreyBG(MyWindowPtr win)
{
	short depth;
	RGBColor color;
	
	if (D3 && (depth=RectDepth(&win->contR))>=4)
		RGBBackColor(SetRGBGrey(&color,depth>4?k8Grey1:k4Grey1));
	else if (ThereIsColor)
		RGBBackColor(SetRGBGrey(&color,0xffff));
}

/**********************************************************************
 * DrawDivider - draw a divider, possibly 3D
 **********************************************************************/
void DrawDivider(Rect *r,Boolean raised)
{
	WindowPtr	portWP = GetQDGlobalsThePort();
	MyWindowPtr win = GetWindowMyWindowPtr(portWP);
	Boolean isActive = IsMyWindow(portWP) && win && win->isActive && !InBG;
	
	DrawThemeSeparator(r,isActive?kThemeStateActive:kThemeStateInactive);
}

/**********************************************************************
 * ColorParam - return a color
 **********************************************************************/
OSErr ColorParam(RGBColor *color,PStr text)
{
	MCEntryPtr mc;
	short item;
	Str255 red, green, blue;
	UPtr spot;
	
	if (item=FindItemByName(GetMHandle(COLOR_HIER_MENU),text))
	{
		if (item==1 || !(mc = GetMCEntry(COLOR_HIER_MENU,item)))
			GetForeColor(color);
		else
			*color = mc->mctRGB2;
	}
	else
	{
		spot = text+1;
		if (PToken(text,red,&spot,",") && PToken(text,green,&spot,",") &&
				PToken(text,blue,&spot,","))
		{
			item = 1;
			Hex2Bytes(red+1,4,(void*)&color->red);
			Hex2Bytes(green+1,4,(void*)&color->green);
			Hex2Bytes(blue+1,4,(void*)&color->blue);
		}
	}
	return(item?noErr:fnfErr);
}

/**********************************************************************
 * LightestGrey - the lightest appropriate grey
 **********************************************************************/
short LightestGrey(Rect *r)
{
	short depth = RectDepth(r);
	if (depth<4) return(0xffff);
	else if (depth==4) return(k4Grey1);
	else return(k8Grey1);
}

/**********************************************************************
 * TwoToneFrame - a two-tone frame for a rectangle
 **********************************************************************/
void TwoToneFrame(Rect *r, RGBColor *topLeft, RGBColor *botRight)
{
	RGBColor oldColor;
	GetForeColor(&oldColor);
	
	//top
	RGBForeColor(topLeft);
	MoveTo(r->left,r->top);
	LineTo(r->right-1,r->top);

	//left
	MoveTo(r->left,r->top);
	LineTo(r->left,r->bottom-1);
	
	//right
	RGBForeColor(botRight);
	MoveTo(r->right-1,r->bottom-1);
	LineTo(r->right-1,r->top+1);

	//bottom
	MoveTo(r->right-1,r->bottom-1);
	LineTo(r->left+1,r->bottom-1);
	
	RGBForeColor(&oldColor);
}

/**********************************************************************
 * LimitColorRange - make sure a color is not too light or too dark
 **********************************************************************/
RGBColor *LimitColorRange(RGBColor *color)
{
	HSVColor hsv;
	uShort min, max;
	
	max = GetRLong(MAX_COLOR_LIGHT)*(0xffff/100);
	min = GetRLong(MIN_COLOR_LIGHT)*(0xffff/100);

	RGB2HSV(color,&hsv);
	hsv.value = MIN(hsv.value,max);
	hsv.value = MAX(hsv.value,min);
	HSV2RGB(&hsv,color);
	return color;
}	

/************************************************************************
 * PastelColor - make a color pastel
 ************************************************************************/
RGBColor *PastelColor(RGBColor *color)
{
	HSVColor hsv;
	long val;
	
	RGB2HSV(color,&hsv);
	if (val=GetRLong(PASTEL_LIGHT)) hsv.value = val;
	if (val=GetRLong(PASTEL_SATUR8)) hsv.saturation = val;
	HSV2RGB(&hsv,color);
	return(color);
}

/************************************************************************
 * DarkenColor - make a color darker
 ************************************************************************/
RGBColor *DarkenColor(RGBColor *color,short percent)
{
	return(LightenColor(color,-percent));
}

/************************************************************************
 * ChangeByPercent - change a vlue by a percentage
 ************************************************************************/
uShort ChangeByPercent(uShort value,short percent)
{
	long newValue = value;
	
	newValue += (newValue*percent)/100;
	if (newValue<0) newValue = 0;
	if (newValue>0xffff) newValue = 0xffff;
	return(newValue);
}

/**********************************************************************
 * SetRGBGrey - set a grey color
 **********************************************************************/
RGBColor *SetRGBGrey(RGBColor *color,short greyValue)
{
	color->red = color->green = color->blue = greyValue;
	return(color);
}

/**********************************************************************
 * BlackWhite - is a color pure black or pure white?
 **********************************************************************/
#define CTOL 100
Boolean Black(RGBColor *color)
{
	if (color->red < CTOL && color->green < CTOL && color->blue < CTOL) return(True);
	return(False);
}
Boolean White(RGBColor *color)
{
	if (color->red > 0xffff-CTOL && color->green > 0xffff-CTOL && color->blue > 0xffff-CTOL) return(True);
	return(False);
}

/**********************************************************************
 * SetForeGrey - set the foreground grey
 **********************************************************************/
void SetForeGrey(short greyValue)
{
	RGBColor color;
	RGBForeColor(SetRGBGrey(&color,greyValue));
}

/**********************************************************************
 * SetBGGrey - set the background grey
 **********************************************************************/
void SetBGGrey(short greyValue)
{
	RGBColor color;
	RGBBackColor(SetRGBGrey(&color,greyValue));
}

/**********************************************************************
 * ColCtlPicker - call the color picker
 **********************************************************************/
Boolean ColCtlPicker(ControlHandle cntl)
{
	RGBColor oldC, newC;
	Point where;
	Boolean result;
	
	if (ThereIsColor)
	{
		ColCtlGet(cntl,&oldC);
		where.h = where.v = -1;
		
		result = GetColor(where,"",&oldC,&newC);
		SetPort(GetWindowPort(GetControlOwner(cntl)));

		if (result)
		{
			ColCtlSet(cntl,&newC);
			return(True);
		}
	}
	
	return(False);
}

/**********************************************************************
 * ColCtlSet - set color in the color cdef
 **********************************************************************/
void ColCtlSet(ControlHandle cntl, RGBColor *color)
{
	SetControl32BitMinimum(cntl,-70000);
	SetControl32BitMaximum(cntl,70000);
	SetControl32BitValue(cntl,color->red);
	SetControlReference(cntl,((uLong)color->green<<16) | (uLong)color->blue);
	Draw1Control(cntl);
}

/**********************************************************************
 * ColCtlGet - get the color from the color cdef
 **********************************************************************/
RGBColor *ColCtlGet(ControlHandle cntl, RGBColor *color)
{
	color->red = GetControl32BitValue(cntl);
	color->green = (short) 0xffff&(GetControlReference(cntl)>>16);
	color->blue = (short) 0xffff&GetControlReference(cntl);
	return(color);
}

#endif