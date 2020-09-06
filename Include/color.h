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

#ifndef COLOR_H
#define COLOR_H

typedef enum {
	e3DNone,		/* none of that! */
	e3DSlight,	/* slight 3-d effect */
	e3DOverBearing	/* overbearing 3-d effect */
} D3EffectEnum;

enum {
	wHiliteColorLight=	5,			/*lightest stripes in title bar */
															/* and lightest dimmed text*/
	wHiliteColorDark,			/*darkest stripes in title bar */
															/* and darkest dimmed text*/
	wTitleBarLight,			/*lightest parts of title bar background*/
	wTitleBarDark,			/*darkest parts of title bar background*/
	wDialogLight,			/*lightest element of dialog box frame*/
	wDialogDark,			/*darkest element of dialog box frame*/
	wTingeLight,			/*lightest window tinging*/
	wTingeDark			/*darkest window tinging*/
};
typedef struct MyWindowStruct *MyWindowPtr;

void SetMenuColor(MenuHandle menu, short item, RGBColor *color);

RGBColor *GetLabelColor(short index,RGBColor *color);
OSErr MyGetLabel(short labelNumber,RGBColor *color,PStr labelString);

void Color3DRect(Rect *r, RGBColor *color, D3EffectEnum howMuch, Boolean raised);
void TwoToneFrame(Rect *r, RGBColor *topLeft, RGBColor *botRight);
#define BlackWhite(c) (Black(c)||White(c))
Boolean Black(RGBColor *color);
Boolean White(RGBColor *color);
RGBColor *LightenColor(RGBColor *color,short percent);
RGBColor *DarkenColor(RGBColor *color,short percent);
RGBColor *LimitColorRange(RGBColor *color);
RGBColor *PastelColor(RGBColor *color);
RGBColor *SetRGBGrey(RGBColor *color,short greyValue);
void SetForeGrey(short greyValue);
void SetBGGrey(short greyValue);
RGBColor *SetRGBGrey(RGBColor *color,short greyValue);
short LightestGrey(Rect *r);
OSErr ColorParam(RGBColor *color,PStr text);
Boolean ColorIsLight(RGBColor *color);

Boolean ColCtlPicker(ControlHandle cntl);
void ColCtlSet(ControlHandle cntl, RGBColor *color);
RGBColor *ColCtlGet(ControlHandle cntl, RGBColor *color);
void DrawDivider(Rect *r,Boolean raised);
void WinGreyBG(MyWindowPtr win);

void Frame3DOrNot(Rect *r,RGBColor *baseColor,Boolean erase);

#define k8Grey1			61166
#define	k8Grey2			56797
#define	k8Grey3			52428
#define	k8Grey4			48059
#define	k8Grey5			43690
#define	k8Grey6			34952
#define	k8Grey7			30583
#define	k8Grey8			21845
#define	k8Grey9			17476
#define	k8Grey10			8738
#define	k8Grey11			4369

#define	k4Grey1			49152
#define	k4Grey2			32768
#define	k4Grey3			8192

#endif