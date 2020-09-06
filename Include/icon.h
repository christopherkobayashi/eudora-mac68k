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

#ifndef ICON_H
#define ICON_H

void NamedIconCalc(Rect *inRect,PStr name,short hIndent,short vIndent,Rect *iconR,Point *textP);
OSErr DTGetIconInfo(short dtRef,OSType creator, short index,OSType *type);

typedef struct ICacheStruct *ICachePtr, **ICacheHandle;
typedef struct ICacheStruct
{
	long magic;
	OSType type;
	OSType creator;
	Handle cache;
	short dtRef;
	Boolean dtRefValid;
	ICacheHandle next;
} ICacheType;

OSErr PlotIconFromICache(ICacheHandle ich,IconTransformType transform,Rect *inRect);
OSErr ICacheToRgn(ICacheHandle ich,Rect *inRect,RgnHandle rgn);
ICacheHandle FindICache(OSType type,OSType creator);
ICacheHandle GetICache(OSType type,OSType creator);
long DisposeICache(ICacheHandle ich);
OSErr DupIconSuite(Handle fromSuite,Handle *toSuite,Boolean reuseSuite);
OSErr DupDeskIconSuite(OSType creator,OSType type,Handle *toSuite);
short Names2Icon(PStr baseName,PStr modifierNames);
void PlotTinyIconAtPenID(IconTransformType transform,short icon);
void PlotTinyULIconAtPenID(IconTransformType transform,short icon);
Handle FSpGetCustomIconSuite(FSSpecPtr spec,short dataSelector);
#ifdef LABEL_ICONS
OSErr	RGBColorToIconFamily( RGBColor* theColor, short iconID, ConstStr255Param name, Boolean overWrite, Handle maskSuite );
OSErr RefreshRGBIcon(short id,RGBColor *color,short template,Boolean *changed);
#define atAbsoluteCenter atNone
#endif
#ifdef DEBUG
#define PlotIconID MyPlotIconID
OSErr MyPlotIconID (Rect *theRect, IconAlignmentType align, IconTransformType transform, short theResID);
#endif
#endif

