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

#define FILE_NUM 60
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#pragma segment Main
short SettingsIconId(PStr name);
void SetListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle);
void SetListHilite(Rect *lRect,Cell lCell,ListHandle lHandle);
void SetListCalc(ListHandle lHandle,short resId,Rect *lRect,PStr name,Rect *iconR,Point *textP, RgnHandle iRgn);
PStr GetResNameAndStripFeature(PStr into,ResType type,short id,Boolean *hasFeature);
pascal void SettingsListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell,
	short lDataOffset, short lDataLen, ListHandle lHandle)
{
#pragma unused(lDataOffset,lDataLen)
	Rect fullRect;
	if (lMessage==lDrawMsg || lMessage==lHiliteMsg)
	{
		fullRect = *lRect;
		fullRect.bottom = fullRect.top + (*lHandle)->cellSize.v;
	}
	switch (lMessage)
	{
		case lDrawMsg:
			SetListDraw(lSelect,&fullRect,lCell,lHandle);
			break;
		case lHiliteMsg:
			SetListHilite(&fullRect,lCell,lHandle);
			break;
	}
}

void SetListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle)
{
	short resId;
	short junk=sizeof(short);
	Str255 name;
	Rect iconR;
	Point textP;
	Boolean hasFeature;
	SAVE_STUFF;
	SET_COLORS;
	
	/*
	 * erase the rectangle first
	 */
	EraseRect(lRect);
	SetSmallSysFont();
	
	/*
	 * fetch the resource id and name
	 */
	LGetCell((Ptr)&resId,&junk,lCell,lHandle);
	GetResNameAndStripFeature(name,'DITL',resId,&hasFeature);
		
	/*
	 * calculate boxes and regions and stuff
	 */
	SetListCalc(lHandle,resId,lRect,name,&iconR,&textP,nil);
	
	/*
	 * draw the text
	 */
	if (!hasFeature)
		TextMode (grayishTextOr);
	MoveTo(textP.h,textP.v); DrawString(name);
	
	/*
	 * and the icon
	 */
	if (iconR.bottom)
	{
		short iconId = SettingsIconId(name);
		if (iconId) PlotIconID(&iconR, atAbsoluteCenter, hasFeature ? ttNone : ttDisabled, iconId);
		if (!hasFeature) {
			TextMode (srcCopy);
			iconR.left  -= 20;
			iconR.right  = iconR.left + 16;
			iconR.top	  +=  4;
			iconR.bottom = iconR.top + 16;
			PlotIconID (&iconR, atAbsoluteCenter, ttNone, PRO_ONLY_ICON);
		}
	}
	if (lSelect) SetListHilite(lRect,lCell,lHandle);
	
	REST_STUFF;
}

short SettingsIconId(PStr name)
{
	short id = Names2Icon(name,"");
	return(id ? id : 2000);
}

void SetListHilite(Rect *lRect,Cell lCell,ListHandle lHandle)
{
	short resId;
	short junk=sizeof(short);
	Rect iconR;
	Point textP;
	RgnHandle iRgn = NewRgn();
	Str31 name;
	Boolean hasFeature;
	SAVE_STUFF;
	SET_COLORS;
	
	LGetCell((Ptr)&resId,&junk,lCell,lHandle);
	if (iRgn)
	{		SetListCalc(lHandle,resId,lRect,GetResNameAndStripFeature(name,'DITL',resId,&hasFeature),&iconR,&textP,iRgn);
		HiInvertRgn(iRgn);
		DisposeRgn(iRgn);
	}
	else
	{
		HiInvertRect(lRect);
	}
	REST_STUFF;
}

void SetListCalc(ListHandle lHandle,short resId,Rect *lRect,PStr name,Rect *iconR,Point *textP, RgnHandle iRgn)
{
	RgnHandle iconRgn = nil;

	NamedIconCalc(lRect,name,(*lHandle)->indent.h,(*lHandle)->indent.v,iconR,textP);

	/*
	 * invert region begins as whole rectangle
	 */
	if (iRgn)
	{
		RectRgn(iRgn,lRect);
	
		/*
		 * punch icon mask out of invert region
		 */
		if (lRect->bottom-lRect->top>18 && (iconRgn = NewRgn()))
		{
			short iconId = SettingsIconId(name);
			IconIDToRgn(iconRgn,iconR,atAbsoluteCenter,iconId);
			DiffRgn(iRgn,iconRgn,iRgn);
			DisposeRgn(iconRgn);
		}
	}
}


PStr GetResNameAndStripFeature(PStr into,ResType type,short id,Boolean *hasFeature)

{
	FeatureType	feature;
	UPtr				spot;

	GetResName(into, type, id);
	
	*hasFeature = true;
	into[*into + 1] = 0;
	if (spot = PPtrFindSub("\p�", into + 1, *into)) {
		feature = Atoi(spot+1);
		*hasFeature = HasFeature (feature);
		into[0] = spot - into - 1;
	}
	return (into);
}
