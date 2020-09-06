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

#define FILE_NUM 19
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Main
void ListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle);
/************************************************************************
 * my list definition
 ************************************************************************/
void ListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle)
{
	Str63 myString;
	short junk=sizeof(myString);
	SAVE_STUFF;
	SET_COLORS;
		
	EraseRect(lRect);
	LGetCell(myString,&junk,lCell,lHandle);
	MoveTo(lRect->left+(*lHandle)->indent.h,lRect->top+(*lHandle)->indent.v);
	DrawString(myString+1);
	
	if (myString[0])
	{
		Point pt;
		PolyHandle pH;
		
		pt.h = lRect->right - 2;
		pt.v = (lRect->top + lRect->bottom)/2;
		MoveTo(pt.h,pt.v);
		PenNormal();
		if (pH=OpenPoly())
		{
			Line(-4,-4);
			Line(0,9);
			LineTo(pt.h,pt.v);
			ClosePoly();
			PaintPoly(pH);
			KillPoly(pH);
		}
	}
	if (lSelect)
	{
		HiInvertRect(lRect);
	}
	REST_STUFF;
}
