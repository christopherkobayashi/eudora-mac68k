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

#ifndef APPEAR_UTIL_H
#define APPEAR_UTIL_H

#define	kInsideOutBevelButtonMargin			4
#define	kInsideOutBevelButtonTextMargin	2

void InitAppearanceMgr(void);
void RefreshAppearance(void);
OSErr SetBevelIcon(ControlHandle theControl, short id, OSType type,
		   OSType creator, Handle suite);
OSErr SetBevelIconIconRef(ControlHandle theControl, IconRef iconRef);
OSErr KillBevelIcon(ControlHandle theControl);
OSErr SetBevelFontSize(ControlHandle theControl, short font, short size);
OSErr SetBevelColor(ControlHandle theControl, RGBColor * color);
OSErr SetBevelStyle(ControlHandle theControl, Style style);
OSErr GetBevelStyle(ControlHandle theControl, Style * style);
OSErr SetBevelJust(ControlHandle theControl, SInt16 just);
OSErr SetBevelMode(ControlHandle theControl, short mode);
OSErr SetBevelMenu(ControlHandle theControl, short menuID,
		   MenuHandle mHandle);
OSErr SetBevelMenuValue(ControlHandle theControl, short value);
short GetBevelMenuValue(ControlHandle theControl);
MenuHandle GetBevelMenu(ControlHandle theControl);
OSErr SetBevelTextAlign(ControlHandle theControl, short value);
OSErr SetBevelTextOffset(ControlHandle theControl, short h);
OSErr SetBevelGraphicOffset(ControlHandle theControl, short h);
OSErr SetBevelGraphicAlign(ControlHandle theControl, short value);
OSErr SetBevelTextPlace(ControlHandle theControl, short value);
OSErr SetTextControlText(ControlHandle theControl, PStr textStr,
			 UHandle textHandle);
Boolean ControlIsUgly(ControlHandle cntl);
ControlHandle GetNewControlSmall(short id, WindowPtr win);
ControlHandle NewControlSmall(WindowPtr theWindow, Rect * boundsRect,
			      PStr title, Boolean visible, short value,
			      short min, short max, short procID,
			      long refCon);
void LetsGetSmall(ControlHandle cntl);
ControlHandle CreateControl(MyWindowPtr win, ControlHandle embed,
			    short strID, short procID, Boolean fit);
ControlHandle CreateMenuControl(MyWindowPtr win, ControlHandle embed,
				PStr title, short menuID, short variant,
				short value, Boolean autoCalcTitle);
MenuHandle GetPopupMenuHandle(ControlHandle cntl);
short ComposeStdAlert(AlertType type, short template, ...);
short AppearanceVersion(void);
ControlHandle CreateInsideOutBevelIconButtonUserPane(WindowPtr theWindow,
						     SInt16 iconID,
						     SInt16 textID,
						     Rect * bounds,
						     SInt16 iconSize,
						     SInt16 maxTextWidth,
						     UInt16 buttonID);
ControlHandle FindInsideOutBevelIconButtonControl(ControlHandle parentCntl,
						  ControlPartCode part);
#endif
