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

#define FILE_NUM 85
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment TlateCalc
void TlateListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle);
void TlateListHilite(Rect *lRect,Cell lCell,ListHandle lHandle);
void TlateListCalc(ListHandle lHandle,Handle suite,Rect *lRect,PStr name,Rect *iconR,Point *textP, RgnHandle iRgn);
pascal void TlateListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell,
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
			TlateListDraw(lSelect,&fullRect,lCell,lHandle);
			break;
		case lHiliteMsg:
			TlateListHilite(&fullRect,lCell,lHandle);
			break;
	}
}

void TlateListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle)
{
	short junk=sizeof(short);
	Str255 name;
	Rect iconR;
	Point textP;
	Handle suite;
	SAVE_STUFF;
	SET_COLORS;
	
	/*
	 * erase the rectangle first
	 */
	EraseRect(lRect);
	SetSmallSysFont();
	
	/*
	 * fetch the suite and name
	 */
	ETLNameAndIcon(lCell.v,name,&suite);
	
	/*
	 * calculate boxes and regions and stuff
	 */
	TlateListCalc(lHandle,suite,lRect,name,&iconR,&textP,nil);
	
	/*
	 * draw the text
	 */
	MoveTo(textP.h,textP.v); DrawString(name);
	
	/*
	 * and the icon
	 */
	if (iconR.bottom)
	{
		if (suite) PlotIconSuite(&iconR, atAbsoluteCenter, ttNone, suite);
	}
	if (lSelect) TlateListHilite(lRect,lCell,lHandle);
	
	/*
	 * restore
	 */
	REST_STUFF;
}

void TlateListHilite(Rect *lRect,Cell lCell,ListHandle lHandle)
{
	short junk=sizeof(short);
	Rect iconR;
	Point textP;
	RgnHandle iRgn = NewRgn();
	Str255 name;
	Handle suite;
	SAVE_STUFF;
	SET_COLORS;
	
	/*
	 * fetch the suite and name
	 */
	ETLNameAndIcon(lCell.v,name,&suite);

	if (iRgn)
	{
		TlateListCalc(lHandle,suite,lRect,name,&iconR,&textP,iRgn);
		HiInvertRgn(iRgn);
		DisposeRgn(iRgn);
	}
	else
	{
		HiInvertRect(lRect);
	}
	REST_STUFF;
}

void TlateListCalc(ListHandle lHandle,Handle suite,Rect *lRect,PStr name,Rect *iconR,Point *textP, RgnHandle iRgn)
{
	RgnHandle iconRgn = NewRgn();

	NamedIconCalc(lRect,name,(*lHandle)->indent.h,(*lHandle)->indent.v,iconR,textP);
	SetRect(iconR,lRect->left,lRect->top,lRect->left+32,lRect->top+32);
	OffsetRect(iconR,(*lHandle)->indent.h,(*lHandle)->indent.v+(RectHi(*lRect)-32)/2);
	textP->h = iconR->right+2;
	textP->v = iconR->bottom-8;
	if (iconRgn && suite)
	{
		IconSuiteToRgn(iconRgn,iconR,atAbsoluteCenter,suite);
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
				IconSuiteToRgn(iconRgn,iconR,atAbsoluteCenter,suite);
				DiffRgn(iRgn,iconRgn,iRgn);
			}
		}
	}
	DisposeRgn(iconRgn);
}
