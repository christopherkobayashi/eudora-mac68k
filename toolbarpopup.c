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

#define FILE_NUM 140
/* Copyright (c) 2000 by QUALCOMM Incorporated */

#pragma segment ToolbarPopup

typedef struct {
	Str255	nickname;				// Name of the nickname that is the target of this popup
	short		menuID;
	short item;
} ToolbarPopupData, *ToolbarPopupDataPtr, **ToolbarPopupDataHandle;

typedef struct {
	MyWindowPtr		win;
	ToolbarVEnum	varCode;				// Variation code of the button from which this toolbar popup extends
	Rect					windowRect;			// Rectangle of the toolbar popup window in global coordinates
	Rect					buttonRect;			// Rectangle of the button from which this toolbar popup extends (in global coordinates)
	short					numButtons;			// Number of buttons on this toolbar popup
	Boolean				vertical;
	Boolean				inToolbarPopup;	// We're in the toolbar popup!
} ToolbarPopupGlobals, *ToolbarPopupGlobalsPtr, **ToolbarPopupGlobalsHandle;

ToolbarPopupGlobals	TBPopupGlobals;

#define Win									TBPopupGlobals.win
#define	VarCode							TBPopupGlobals.varCode
#define	ButtonRect					TBPopupGlobals.buttonRect
#define	WindowRect					TBPopupGlobals.windowRect
#define	NumButtons					TBPopupGlobals.numButtons
#define	Vertical						TBPopupGlobals.vertical
#define	InToolbarPopup			TBPopupGlobals.inToolbarPopup

Boolean ToolbarPopupClose (MyWindowPtr win);
Boolean ToolbarPopupPosition (Boolean save, MyWindowPtr win);
OSErr		ToolbarPopupDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
OSErr		ToolbarPopupDrop (DragReference drag, WindowPtr theWindow, Point mouse, short which);

/**********************************************************************
 * OpenToolbarPopup - open a toolbar popup window
 **********************************************************************/
Boolean OpenToolbarPopup (Rect *buttonRect, ToolbarVEnum varCode, short numButtons, Boolean vertical)
{
	WindowPtr	winWP;

	if (Win)
		return (false);
		
	Win = GetNewMyWindow (TOOLBAR_POPUP_WIND, nil, nil, BehindModal, false, false, TOOLBAR_POPUP_WIN);
	winWP = GetMyWindowWindowPtr (Win);
	if (Win) {
		VarCode		 = varCode;
		ButtonRect = *buttonRect;
		NumButtons = numButtons;
		Vertical	 = vertical;
		
		Win->windowType	= kFloating;
		Win->isRunt			= true;
		Win->close			= ToolbarPopupClose;
		Win->drag				= ToolbarPopupDragHandler;
		Win->position		= ToolbarPopupPosition;
		
		MySetThemeWindowBackground (Win, kThemeActiveUtilityWindowBackgroundBrush, false);
		SetThemeTextColor (kThemeActiveWindowHeaderTextColor, RectDepth (&Win->contR), true);
		ShowMyWindow (winWP);
	}
	return (Win ? true : false);
}


Boolean ToolbarPopupClose (MyWindowPtr win)

{
	WindowPtr			theWindow;
	ControlHandle	theControl;
	
	if (theWindow = GetMyWindowWindowPtr (win))
		for (theControl = GetControlList(theWindow); theControl;theControl = GetNextControl(theControl))
			DisposeHandle((Handle)GetControlReference(theControl));
	return (true);
}

void CloseToolbarPopup (void)

{
	CGrafPtr	oldPort;
	
	CloseMyWindow (GetMyWindowWindowPtr (Win));
	Win = nil;
	InToolbarPopup = false;
	
	GetPort (&oldPort);
	UpdateAllWindows ();	// Need do do this to force background windows to redraw when drag handling
	SetPort (oldPort);
}


/**********************************************************************
 * AddMenuToToolbarPopup - Add a menu item to a toolbar popup window
 **********************************************************************/
OSErr AddMenuToToolbarPopup (short index, short menuID, short item, PStr nickname)

{
	ToolbarPopupData							toolbarData;
	ToolbarPopupDataHandle				toolbarDataH;
	ControlHandle									theControl;
	ControlButtonTextPlacement		placement;
	ControlButtonTextAlignment		alignment;
	ControlButtonGraphicAlignment	gAlignment;
	Str255												menuName;
	Rect													bounds;
	Point													gOffset;
	OSErr													theError;
	short													textOffset;

	toolbarDataH = nil;
	
	PCopy (toolbarData.nickname, nickname);
	toolbarData.menuID = menuID;
	toolbarData.item	 = item;
	theError = PtrToHand (&toolbarData, &toolbarDataH, sizeof (toolbarData));

	if (!theError) {
		SetRect (&bounds, 0, 0, RectWi (ButtonRect), RectHi (ButtonRect));
		OffsetRect (&bounds, 1, 1);

		if (Vertical)
			OffsetRect (&bounds, 0, index * RectHi (bounds));
		else
			OffsetRect (&bounds, index * RectWi (bounds), 0);

		MyGetItem (GetMHandle (menuID), item, menuName);
		if (theControl = NewControl (GetMyWindowWindowPtr(Win), &bounds, VarHasName(VarCode) ? menuName : "\p", true, 0, 0, 1, kControlBevelButtonSmallBevelProc, (long) toolbarDataH)) {
			GetButtonAlignment (VarCode, &alignment, &placement, &textOffset, &gAlignment, &gOffset);
		 	if (VarHasName(VarCode)) {
			 	SetControlData (theControl,0,kControlBevelButtonTextAlignTag,sizeof(alignment),(void*)&alignment);
			 	SetControlData (theControl,0,kControlBevelButtonTextPlaceTag,sizeof(placement),(void*)&placement);
		 		SetControlData (theControl,0,kControlBevelButtonTextOffsetTag,sizeof(textOffset),(void*)&textOffset);
			}
			if (VarHasIcon(VarCode)) {
				SetBevelIcon (theControl, Names2Icon (menuName, "\p"), nil, nil, nil);
		 		SetControlData (theControl,0,kControlBevelButtonGraphicAlignTag,sizeof(gAlignment),(void*)&gAlignment);
		 		SetControlData (theControl,0,kControlBevelButtonGraphicOffsetTag,sizeof(gOffset),(void*)&gOffset);
	 		}
		}
	}
	if (theError)
		ZapHandle (toolbarDataH);
	return (theError);
}


Boolean ToolbarPopupPosition (Boolean save, MyWindowPtr win)

{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect			windowRect;
	
	if (Vertical)
		SetRect (&windowRect, ButtonRect.left - 1, ButtonRect.bottom, ButtonRect.right + 1, ButtonRect.bottom + NumButtons * RectHi (ButtonRect) + 2);
	else
		SetRect (&windowRect, ButtonRect.right, ButtonRect.top - 1, ButtonRect.right + NumButtons * RectWi (ButtonRect) + 2, ButtonRect.bottom + 1);
	
	SizeWindow (winWP, RectWi (windowRect), RectHi (windowRect), false);
	MoveWindow (winWP, windowRect.left, windowRect.top, true);
	WindowRect = windowRect;
	return (true);
}


OSErr ToolbarPopupDragHandler(MyWindowPtr win, DragTrackingMessage which, DragReference drag)

{
	CGrafPtr			oldPort;
	WindowPtr			theWindow;
	ControlHandle	selectedControl,
								theControl;
	OSErr					theError;
	Point					mouse;
	
	theError	= noErr;
	theWindow = GetMyWindowWindowPtr (win);
	
	GetPort (&oldPort);
	SetPort (GetWindowPort (theWindow));
	GetDragMouse (drag, &mouse, nil);
	switch (which) {
		case kDragTrackingEnterWindow:
			InToolbarPopup = true;
			break;
		case kDragTrackingInWindow:
			// While we're in the toolbar popup, handle highlighting of our buttons in a menu-like fashion.
			GlobalToLocal (&mouse);
			if (FindControl (mouse, theWindow, &selectedControl))
				for (theControl = GetControlList(theWindow); theControl;theControl = GetNextControl(theControl))
					SetControlValue (theControl, theControl == selectedControl ? 1 : 0);
			break;
		case kDragTrackingLeaveWindow:
			if (!PtInRect (mouse, &ButtonRect)) {
				CloseToolbarPopup ();
				theError = dragNotAcceptedErr;
			}
			else
				for (theControl = GetControlList(theWindow); theControl;theControl = GetNextControl(theControl))
					SetControlValue (theControl, 0);
			break;
		case 0xfff:
			theError = ToolbarPopupDrop (drag, theWindow, mouse, which);
			break;
	}
	SetPort (oldPort);
	return (theError);
}


OSErr ToolbarPopupDrop (DragReference drag, WindowPtr theWindow, Point mouse, short which)

{
	ToolbarPopupDataHandle	toolbarPopupH;
	ToolbarPopupData				toolbarPopup;
	TOCHandle								tocH;
	TextAddrHandle					toWhom;
	UHandle									data;
	ControlHandle						theControl;
	OSErr										theError;
	
	theError = dragNotAcceptedErr;
	GlobalToLocal (&mouse);
	if (FindControl (mouse, theWindow, &theControl)) {
		if (toolbarPopupH = (ToolbarPopupDataHandle) GetControlReference(theControl)) {
			toolbarPopup = **toolbarPopupH;
			tocH = nil;
			data = nil;
			if (toWhom = NuHandle (toolbarPopup.nickname[0]))
				BlockMoveData (&toolbarPopup.nickname[1], *toWhom, toolbarPopup.nickname[0]);
			if (!(theError = MyGetDragItemData (drag, 1, MESS_FLAVOR, (void*) &data)))
				tocH = (***(MessHandle**)data)->tocH;
			else
				if (!(theError = MyGetDragItemData (drag, 1, TOC_FLAVOR, (void*) &data)))
					tocH = **(TOCHandle**) data;
			CloseToolbarPopup ();
			if (tocH)
				DoIterativeThingy (tocH, toolbarPopup.item, CurrentModifiers (), toWhom);
			ZapHandle (data);
			ZapHandle (toWhom);
		}
	}
	return (theError);
}


Boolean MouseInToolbarPopup (Point mouse, Boolean checkButton)

{
	Boolean	inWindowRect,
					inButtonRect;
	
	inWindowRect = PtInRect (mouse, &WindowRect);
	inButtonRect = checkButton ? PtInRect (mouse, &ButtonRect) : false;
	return (inWindowRect || inButtonRect);
}