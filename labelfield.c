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

#include "labelfield.h"
#define FILE_NUM 138

#pragma segment Util

#define	kLabelFieldHorizontalMargin		4
#define	kLabelFieldVerticalMargin			2

typedef	struct {
	short	left;				// Left coordinate 
	short	top;				// Top coordinate
	short	width;			// Width (duh) of the label field
	short	height;			// Height of the label field -- though this may be a fantasy we need to recalc
} LabelGeometryRec, *LabelGeometryPtr, **LabelGeometryHandle;

static void SetLabelGeometry (LabelGeometryPtr geometry, short left, short top, short width, short height);
static void LabelFieldGeometry (ControlHandle theControl,  LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect);
static void LabelFieldGeometryVertical (ControlHandle theControl,  LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect);
static void LabelFieldGeometryHorizontal (ControlHandle theControl,  LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect);
static pascal void LabelFieldIdle (ControlHandle theControl);
static pascal void LabelFieldDraw (ControlHandle theControl, SInt16 part);
static pascal ControlPartCode LabelFieldKeyDown (ControlHandle control, SInt16 keyCode, SInt16 charCode, SInt16 modifiers);
static pascal ControlPartCode LabelFieldHitTest (ControlHandle theControl, Point where);
static pascal ControlPartCode LabelFieldFocus (ControlHandle theControl, ControlFocusPart action);
static pascal void LabelFieldBackground(ControlHandle theControl, ControlBackgroundPtr info);

extern void DoKeyDown(WindowPtr topWin,EventRecord *event);

//
//	CreateLabelField
//
//		Label fields are controls that contain both a PETE field and a nice labels in front
//		of the editng field.  Why something like this isn't in the toolbox by default, I'll
//		never know...
//
//		We overload a few of the ControlRecord fields like so:
//
//						min  =  Upper 16 bits: label justification
//								 =  Lower 16 bits: if labelDisplayAboveField is clear -> label width
//																	 if labelDisplayAboveField is set   -> pete height (now, that's clear, isn't it?)
//						max	 =  flags
//				 refcon  =  PETEHandle
//

ControlHandle CreateLabelField (MyWindowPtr win, Rect *boundsRect, Str255 title, short labelWidth, short labelJustification, LabelFieldFlagType flags, PETEDocInitInfoPtr initInfo, uLong pteFlags)

{
//	PETEParaInfo			pinfo;
	PETEHandle				pte;
	LabelGeometryRec	geometry;
	ControlHandle			theControl;
	Rect							contrlRect,
										pteRect;
	OSErr							theError;
	DECLARE_UPP(LabelFieldIdle,ControlUserPaneIdle);
	DECLARE_UPP(LabelFieldDraw, ControlUserPaneDraw);
	DECLARE_UPP(LabelFieldKeyDown, ControlUserPaneKeyDown);
	DECLARE_UPP(LabelFieldHitTest, ControlUserPaneHitTest);
	DECLARE_UPP(LabelFieldFocus, ControlUserPaneFocus);
	DECLARE_UPP(LabelFieldBackground, ControlUserPaneBackground);

	theError = noErr;
	pte			 = nil;
	
	if (boundsRect)
		contrlRect = *boundsRect;
	else
		SetRect (&contrlRect, -50, -50, -20, -20);
		
	// Set up the control
	theControl = NewControlSmall (GetMyWindowWindowPtr(win),
																&contrlRect,
																title,
																true,
																kControlWantsIdle | kControlSupportsFocus | kControlGetsFocusOnClick | kControlSupportsEmbedding | kControlHasSpecialBackground,
																0,
																0,
																kControlUserPaneProc,
																nil);

	if (theControl) {
		if (gBetterAppearance) {
			SetControl32BitMinimum (theControl, labelJustification << 16 | labelWidth);
			SetControl32BitMaximum (theControl, flags);
		}
		else {
			SetControlMinimum (theControl, labelJustification << 8 | (labelWidth & 0x00FF));
			SetControlMaximum (theControl, (short) (flags & 0x0000FFFF));
		}
		// Setup procs to handle an assortment of events
		INIT_UPP(LabelFieldIdle,ControlUserPaneIdle);
		INIT_UPP(LabelFieldDraw, ControlUserPaneDraw);
		INIT_UPP(LabelFieldKeyDown, ControlUserPaneKeyDown);
		INIT_UPP(LabelFieldHitTest, ControlUserPaneHitTest);
		INIT_UPP(LabelFieldFocus, ControlUserPaneFocus);
		INIT_UPP(LabelFieldBackground, ControlUserPaneBackground);

		theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneIdleProcTag, sizeof (ControlUserPaneIdleUPP), (void*) &LabelFieldIdleUPP);
		if (!theError)
			theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneKeyDownProcTag, sizeof (ControlUserPaneKeyDownUPP), (void*) &LabelFieldKeyDownUPP);
		if (!theError)
			theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneHitTestProcTag, sizeof (ControlUserPaneHitTestUPP), (void*) &LabelFieldHitTestUPP);
		if (!theError)
			theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneDrawProcTag, sizeof (ControlUserPaneDrawUPP), (void*) &LabelFieldDrawUPP);
		if (!theError)
			theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneFocusProcTag, sizeof (ControlUserPaneFocusUPP), (void*) &LabelFieldFocusUPP);
		if (!theError)
			theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneBackgroundProcTag, sizeof (ControlUserPaneBackgroundUPP), (void*) &LabelFieldBackgroundUPP);

		// Create a PETEHandle that will be put into the user pane and assign it a 'pete' property
		if (!theError) {
			if (!(flags & labelWrapField))
				initInfo->docWidth = REAL_BIG;
			initInfo->containerControl = theControl;	//	embed pte scroll bars in this control
			theError = PeteCreate (win, &pte, pteFlags, initInfo);
		}
		if (!theError) {
			if (!(flags & labelWrapField))
				(*PeteExtra(pte))->infinitelyWide = true;
			theError = PeteFontAndSize (pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		}
//		if (!theError && !(flags & labelWrapField)) {
//			pinfo.endMargin = REAL_BIG;
//			theError = PETESetParaInfo (PETE, pte, -1, &pinfo, peEndMarginValid);
//		}
		if (!theError) {	
			// Place the PETEHandle into the control's refcon field, or cleanup if disaster has struck
			SetControlReference (theControl, (long) pte);

			// Size up the situation
			SetLabelGeometry (&geometry, contrlRect.left, contrlRect.top, RectWi (contrlRect), RectHi (contrlRect));
			LabelFieldGeometry (theControl, &geometry, nil, &pteRect);
			PeteDidResize (pte, &pteRect);
			(*PeteExtra (pte))->frame = true;
		}
		else
			if (pte) {
				PeteDispose (win, pte);
				DisposeControl (theControl);
				theControl = nil;
			}
	}
	return (theControl);
}

static void SetLabelGeometry (LabelGeometryPtr geometry, short left, short top, short width, short height)

{
	geometry->left	 = left;
	geometry->top		 = top;
	geometry->width	 = width;
	geometry->height = height;
}


static void LabelFieldGeometry (ControlHandle theControl, LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect)

{
	if (theControl) {
		if (GetLabelFieldFlags (theControl) & labelDisplayAboveField)
			LabelFieldGeometryVertical (theControl, geometry, labelRect, pteRect);
		else
			LabelFieldGeometryHorizontal (theControl, geometry, labelRect, pteRect);
	}
}

static void LabelFieldGeometryHorizontal (ControlHandle theControl, LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect)

{
	short	labelWidth,
				right,
				bottom;
	Str255	s;
	
	labelWidth = GetLabelFieldLabelWidth (theControl);
	right			 = geometry->left + geometry->width;
	bottom		 = geometry->top + geometry->height;
	if (GetLabelFieldFlags (theControl) & labelAutoSize)
	{
		GetControlTitle(theControl,s);
		labelWidth = StringWidth(s);
	}
	if (labelRect)
		SetRect (labelRect, geometry->left, geometry->top, geometry->left + labelWidth, bottom);
	if (pteRect)
		SetRect (pteRect, geometry->left + labelWidth + (labelWidth ? kLabelFieldHorizontalMargin : 0), geometry->top, right, bottom);
}

static void LabelFieldGeometryVertical (ControlHandle theControl, LabelGeometryPtr geometry, Rect *labelRect, Rect *pteRect)

{
	FontInfo	info;
	short			peteTop,
						peteHeight,
						labelHeight,
						right;
	Boolean		autoSize = GetLabelFieldFlags (theControl) & labelAutoSizeFieldHeight ? true : false;
	Str255	s;
	
	GetFontInfo (&info);
	right				= geometry->left + geometry->width;
	labelHeight	= info.ascent + info.descent + info.leading;
	peteHeight	= autoSize ? geometry->height - labelHeight - kLabelFieldVerticalMargin : GetLabelFieldPeteHeight (theControl);
	peteTop			= geometry->top + labelHeight + kLabelFieldVerticalMargin;
	if (labelRect)
	{
		GetControlTitle(theControl,s);
		SetRect (labelRect, geometry->left, geometry->top, geometry->left + StringWidth(s), geometry->top + labelHeight);
	}
	if (pteRect)
		SetRect (pteRect, geometry->left, peteTop, right, peteTop + peteHeight);
}


//
//	DisposeLabelField
//
//		Disposes of the PETEHandle inside of a Label Field -- but does NOT dispose of
//		the ControlHandle itself!
//

void DisposeLabelField (ControlHandle theControl)

{
	PETEHandle	pte;
	
	if (theControl)
		if (pte = GetLabelFieldPete (theControl)) {
			PeteDispose (GetWindowMyWindowPtr (GetControlOwner(theControl)), pte);
			SetControlReference (theControl, (long) nil);
		}
}


//
//	MoveLabelField
//
//		Moves the label field and its PETEHandle.
//

void MoveLabelField (ControlHandle theControl, int h, int v, int w, int t)

{
	LabelGeometryRec	geometry;
	PETEHandle				pte;
	Rect							contrlRect,
										labelRect,
										pteRect;
	
	SetLabelGeometry (&geometry, h, v, w, t);
	if (theControl)
		if (pte = GetLabelFieldPete (theControl)) {
			// Get the rectangles for both the label and the PETE
			LabelFieldGeometry (theControl, &geometry, &labelRect, &pteRect);
			
			// Move the control to its new location.  Note that the new location is defined
			// by the union of the label and PETE rects.
			UnionRect (&labelRect, &pteRect, &contrlRect);
			MoveMyCntl (theControl, contrlRect.left, contrlRect.top, RectWi (contrlRect), RectHi (contrlRect));
			PeteDidResize (pte, &pteRect);
		}
}


//
//	LabelFieldIdle
//
//		The idleProc for a Label Field.  Our idle handling amounts to nothing more
//		than giving time to the nickname watcher if the PETE supports nickname
//		scanning.
//
//		We only want to give idle time to the PETE that currently has the focus.  So,
//		once we've grabbed the PETEHandle we verify that this handle is the current
//		PETE for this window.
//
//		Note: In the future there might be more interesting things we'll want
//					to do at idle time.
// 

static pascal void LabelFieldIdle (ControlHandle theControl)

{
	MyWindowPtr	win;
	PETEHandle	pte;

	if (pte = GetLabelFieldPete (theControl))
		if (win = GetWindowMyWindowPtr (GetControlOwner(theControl)))
			if ((*PeteExtra(pte))->win->pte == pte)
				if (HasNickScanCapability (pte))
					NicknameWatcherIdle (pte);
}


//
//	LabelFieldKeyDown
//
//		Key down handling for the label field.
//

static pascal ControlPartCode LabelFieldKeyDown (ControlHandle control, SInt16 keyCode, SInt16 charCode, SInt16 modifiers)

{
	EventRecord		fakeEvent;

	fakeEvent.what			= keyDown;
	fakeEvent.message		= (charCode & charCodeMask) | (keyCode << 8);
	fakeEvent.modifiers	= modifiers;
	fakeEvent.when			= TickCount ();
	GetMouse (&fakeEvent.where);
	LocalToGlobal (&fakeEvent.where);
	DoKeyDown (GetControlOwner(control), &fakeEvent);
	return (kControlEditTextPart);
}


//
//	LabelFieldDraw
//
//		The drawProc for a Label Field.  We want to position and draw the label, as well
//		as update the PETE.
//

static pascal void LabelFieldDraw (ControlHandle theControl, SInt16 part)

{
#pragma unused(part)
	LabelGeometryRec	geometry;
	PETEHandle				pte;
	FontInfo					theInfo;
	Rect							labelRect,rCntl;
	short							labelJustification;
	Str255				sTitle;
	MyWindowPtr			win;

	if (win = GetWindowMyWindowPtr(GetControlOwner(theControl)))
		ConfigFontSetup(win);
	
	GetFontInfo (&theInfo);
	GetControlBounds(theControl,&rCntl);
	SetLabelGeometry (&geometry,rCntl.left,rCntl.top, RectWi (rCntl), RectHi(rCntl));
	LabelFieldGeometry (theControl, &geometry, &labelRect, nil);
	labelJustification = GetLabelFieldLabelJustification (theControl);
	GetControlTitle(theControl,sTitle);
	if (labelJustification == teFlushDefault)
		labelJustification = GetSysDirection () ? teFlushRight : teFlushLeft;
	switch (labelJustification) {
		case teFlushLeft:
			MoveTo (labelRect.left, labelRect.top + theInfo.ascent);
			break;
		case teFlushRight:
			MoveTo (labelRect.right - StringWidth (sTitle), labelRect.top + theInfo.ascent);
			break;
		case teCenter:
			MoveTo (labelRect.left + (RectWi (labelRect) - StringWidth (sTitle)) >> 1, labelRect.top + theInfo.ascent);
	}
	
	if (!HaveTheDiseaseCalledOSX())
	if (win = GetWindowMyWindowPtr(GetControlOwner(theControl)))
		RGBBackColor(&win->backColor);

	if (pte = GetLabelFieldPete (theControl)) {
		if ((*PeteExtra(pte))->isInactive = !IsControlActive (theControl))
			TextMode (grayishTextOr);
		if (GetLabelFieldFlags (theControl) & labelWrapLabel) {
			TETextBox (&sTitle[1], sTitle[0], &labelRect, labelJustification);
		}
		else
			DrawString (sTitle);
		PeteUpdate (pte);
	}
}


//
//	LabelFieldHitTest
//

static pascal ControlPartCode LabelFieldHitTest (ControlHandle theControl, Point where)

{
	LabelGeometryRec	geometry;
	ControlPartCode		part;
	Rect							labelRect,rCntl;

	part = kControlNoPart;
	if (PtInRect (where,GetControlBounds(theControl,&rCntl)) && IsControlActive (theControl)) {
		SetLabelGeometry (&geometry,rCntl.left,rCntl.top, RectWi (rCntl), RectHi (rCntl));
		LabelFieldGeometry (theControl, &geometry, &labelRect, nil);
		if (PtInRect (where, &labelRect))
			part = kControlLabelPart;
		else
			if (PtInPETEView (where, GetLabelFieldPete (theControl)))
				part = kControlEditTextPart;
	}
	return (part);
}


static pascal ControlPartCode LabelFieldFocus (ControlHandle theControl, ControlFocusPart action)

{
	ControlPartCode	partCode;
	PETEHandle			pte;
	MyWindowPtr			win;
	
	partCode = kControlNoPart;
	if (pte = GetLabelFieldPete (theControl))
		if (win = GetWindowMyWindowPtr(GetControlOwner(theControl)))
			switch (action) {
				case kControlFocusNoPart:
					PeteSelect (win, pte, 0, 0);
					PeteFocus (win, nil, true);
					break;
				case kControlEditTextPart:
					if (!(*PeteExtra(pte))->isInactive) {
						PeteFocus (win, pte, true);
						partCode = kControlEditTextPart;
					}
					break;
			}
	return (partCode);
}

static pascal void LabelFieldBackground(ControlHandle theControl, ControlBackgroundPtr info)
{
	MyWindowPtr			win;
	
	if (win = GetWindowMyWindowPtr(GetControlOwner(theControl)))
		RGBBackColor(&win->backColor);		
}

void SetLabelFieldText (ControlHandle theControl, UPtr text, long len)

{
	PETEHandle	pte;
	
	if (pte = GetLabelFieldPete (theControl))
		PeteSetTextPtr (pte, text, len);
}

Handle GetLabelFieldText (ControlHandle theControl)

{
	PETEHandle	pte;
	UHandle			text;
	
	text = nil;
	if (pte = GetLabelFieldPete (theControl))
		PeteGetRawText (pte, &text);
	return (text);
}

PStr GetLabelFieldString (ControlHandle theControl, PStr string)

{
	PETEHandle	pte;

	if (pte = GetLabelFieldPete (theControl))
		PeteString (string, pte);
	return (string);
}


Rect *GetRelevantLabelFieldBounds (ControlHandle theControl, RelativeFlagsType flags, Rect *boundsRect)

{
	LabelGeometryRec	geometry;
	Rect				rCntl;

	GetControlBounds(theControl,&rCntl);
	SetLabelGeometry (&geometry,rCntl.left,rCntl.top,RectWi(rCntl), RectHi(rCntl));
	if (flags & rfPETEPart)
		LabelFieldGeometry (theControl, &geometry, nil, boundsRect);
	else
		if (flags & rfLabelPart)
			LabelFieldGeometry (theControl, &geometry, boundsRect, nil);		
	return  (boundsRect);
}
