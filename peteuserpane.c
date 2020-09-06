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

#include "peteuserpane.h"
#define FILE_NUM 119

#pragma segment Util

static pascal void PeteUserPaneIdle (ControlHandle theControl);
static pascal void PeteUserPaneDraw (ControlHandle theControl, SInt16 part);
static pascal ControlPartCode PeteUserPaneKeyDown (ControlHandle control, SInt16 keyCode, SInt16 charCode, SInt16 modifiers);
static pascal ControlPartCode PeteUserPaneFocus (ControlHandle control, ControlFocusPart action);

extern void DoKeyDown(WindowPtr topWin,EventRecord *event);

//
//	CreatePeteUserPane
//
//		Creates a user pane that contains a PETE field.  This allows fields in dialogs to act
//		like PETE fields instead of TE fields.  PETE user panes are given a control property
//		with the tag 'pete' (and the boolean value, true)  so that we can easily determine that
//		this is a PETE control.  Ordinarily, this might not be all that important since (for
//		instance) a window might be able to easily specify which of it's controls are or are
//		not of a specific type.  In a dialog, however, this is not quite so easy since most
//		dialog handling code is bottlenecked to handle most of the dialog processing in an
//		application.  By assigning a unique property to these controls the bottle neck routines
//		are able to look at a ControlHandle and determine whether or not it is a PETE user pane.
//
//		We also set up a number of user pane procs to handle a the processing of a PETE control
//		in response to idle, keyDown and draw events.
//
//		Lastly, the PETEHandle itself is stuffed into the control's refcon field.
//
//		If we are unable to create the PETE user pane, we dispose of the PETE and return nil.
//
//		Note:	In order to use a PETE in a user pane of a dialog, you'll need to do the following
//					to your dialog and control:
//						1. Create a 'CNTL' resource with a procID of 256
//						2. Your dialog must have a root control.  This means that the dialog
//							 must have the "use control hierarchy" bit set, so you'll have to
//							 add your dialog to the 'hlist' variable in AddDlgx.
//						3. After creating your dialog (with GetNewMyDialog), call CreatePeteUserPane
//							 for any controls that are to contain PETE's.
//						4. Initialize the win->pte pointer for your dialog to point to the PETEHandle
//							 returned from CreatePeteUserPane.
//
//		To get to the PETEHandle itself, use GetPeteDItem.
//
//		Don't forget to PeteDispose the PETEHandle _before_ you dispose of your dialog!!
//

PETEHandle CreatePeteUserPane (ControlHandle theControl, uLong flags, PETEDocInitInfoPtr initInfo, Boolean noWrap)

{
	MyWindowPtr		win;
	PETEParaInfo	pinfo;
	PETEHandle		pte;
	OSErr					theError;
//	Boolean				isPete;
	Rect			rCntl;
	DECLARE_UPP(PeteUserPaneIdle,ControlUserPaneIdle);
	DECLARE_UPP(PeteUserPaneDraw, ControlUserPaneDraw);
	DECLARE_UPP(PeteUserPaneKeyDown, ControlUserPaneKeyDown);
	DECLARE_UPP(PeteUserPaneFocus, ControlUserPaneFocus);

	if (!theControl)
		return (nil);
	
	win = GetWindowMyWindowPtr (GetControlOwner(theControl));
	pte = nil;
	
	SetControlValue (theControl, kControlWantsIdle | kControlSupportsFocus | kControlGetsFocusOnClick);
	
	// Create a PETEHandle that will be put into the user pane and assign it a 'pete' property
	theError = PeteCreate (win, &pte, flags, initInfo);
	if (!theError)
		if (noWrap) {
			pinfo.endMargin = REAL_BIG;
			theError = PETESetParaInfo (PETE, pte, -1, &pinfo, peEndMarginValid);
		}
	if (!theError) {
		(*PeteExtra (pte))->frame = true;
		PeteDidResize (pte,GetControlBounds(theControl,&rCntl));

//  Commented out for now because control properties are not supported on 68K
//		isPete = true;
//		theError = SetControlProperty (theControl, CREATOR, kPETEComponentSubType, sizeof (isPete), (void*) &isPete);
//
//	Instead, because 'GetControlData' is BROKEN for early versions of appearance, we're going to brand PETE controls
//	with some "pete" in the contrlTitle field (grrrr...)
		SetControlTitle (theControl, kPeteUserPaneTitleIDString);
	}
	
	// Setup procs to handle an assortment of events
	INIT_UPP(PeteUserPaneIdle,ControlUserPaneIdle);
	INIT_UPP(PeteUserPaneDraw, ControlUserPaneDraw);
	INIT_UPP(PeteUserPaneKeyDown, ControlUserPaneKeyDown);
	INIT_UPP(PeteUserPaneFocus, ControlUserPaneFocus);
	if (!theError)	
		theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneIdleProcTag, sizeof (ControlUserPaneIdleUPP), (void*) &PeteUserPaneIdleUPP);
	if (!theError)
		theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneKeyDownProcTag, sizeof (ControlUserPaneKeyDownUPP), (void*) &PeteUserPaneKeyDownUPP);
	if (!theError)
		theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneDrawProcTag, sizeof (ControlUserPaneDrawUPP), (void*) &PeteUserPaneDrawUPP);
	if (!theError)
		theError = SetControlData (theControl, kControlEditTextPart, kControlUserPaneFocusProcTag, sizeof (ControlUserPaneFocusUPP), (void*) &PeteUserPaneFocusUPP);

	// Place the PETEHandle into the control's refcon field, or cleanup if disaster has struck
	if (!theError)
		SetControlReference (theControl, (long) pte);
	else 
		if (pte) {
			PeteDispose (win, pte);
			pte = nil;
		}
	return (pte);
}


//
//	DisposePeteUserPaneItem
//
//		Disposes of a PETE user pane item by disposing of its associated PETEHandle.
//

void DisposePeteUserPaneItem (MyWindowPtr dgPtr, short item)

{
	PETEHandle	pte;
	
	if (pte = GetPeteDItem (dgPtr, item))
		PeteDispose (dgPtr, pte);
}


//
//	IsPeteControl
//
//		Checks to see if a given control is a PETE user pane by retrieving the 'pete'
//		property (if this control even has such a property) and verifying that this
//		boolean value is true.
//

Boolean IsPeteControl (ControlHandle theControl)

{
	Str255	title;
	Boolean	isPete;

	isPete = false;

//  Commented out for now because control properties are not supported on 68K
//	if (theControl)
//		(void) GetControlProperty (theControl, CREATOR, kPETEComponentSubType, sizeof (isPete), &actualSize, (void*) &isPete);

// Instead, we're going to check to see if the control gas a keyDown proc
// and check to see if it is the same as the PeteUserPaneKeyDown proc.  :(
//	if (theControl) {
//		if (!GetControlData (theControl, 0,kControlUserPaneKeyDownProcTag, sizeof (ControlUserPaneKeyDownUPP), (void*) &currentKeyDownUPP, &actualSize))
//			isPete = currentKeyDownUPP == PeteUserPaneKeyDownUPP;
//	}

// Strike Two... GetControlData is broken for versions of Appearance earlier than 1.1 (great).
	if (theControl) {
		GetControlTitle (theControl, title);
		isPete = StringSame (title, kPeteUserPaneTitleIDString);
	}
	return (isPete);
}


//
//	PeteUserPaneIdle
//
//		The idleProc for a PETE user pane.  Our idle handling amounts to nothing more
//		than giving time to the nickname watcher if this PETE supports nickname
//		scanning.
//
//		We only want to give idle time to the PETE that currently has the focus.  So,
//		once we've grabbed the PETEHandle we verify that this handle is the current
//		PETE for this window.
//
//		Note: In the future there might be more interesting things we'll want
//					to do at idle time.
// 

static pascal void PeteUserPaneIdle (ControlHandle theControl)
{
	MyWindowPtr	win;
	PETEHandle	pte;

	if (pte = (PETEHandle) GetControlReference (theControl))
		if (win = GetWindowMyWindowPtr(GetControlOwner(theControl)))
			if ((*PeteExtra(pte))->win->pte == pte)
				if (HasNickScanCapability (pte))
					NicknameWatcherIdle (pte);
}


//
//	PeteUserPaneDraw
//
//		The drawProc for a PETE user pane.  All we need to do is update the PETE
//		if we have a valid PETEHandle.
//

static pascal void PeteUserPaneDraw (ControlHandle theControl, SInt16 part)
{
#pragma unused(part)
	PETEHandle pte;

	if (pte = (PETEHandle) GetControlReference (theControl))
		PeteUpdate (pte);
}


//
//	PeteUserPaneKeyDown
//
//		Key down handling for the PETE user pane.  We need to handle tabbing ourselves
//		if the user pane is in a dialog, so we do.  Everything else gets passed to the
//		normal key down handler in a scary fake event.
//

static pascal ControlPartCode PeteUserPaneKeyDown (ControlHandle control, SInt16 keyCode, SInt16 charCode, SInt16 modifiers)

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


static pascal ControlPartCode PeteUserPaneFocus (ControlHandle control, ControlFocusPart action)

{
	MyWindowPtr	win;

	win = GetWindowMyWindowPtr(GetControlOwner(control));
	if (!win)
		return (0);
		
	PeteSelect (win, win->pte, 0, 0);

	switch (action) {
		case kControlFocusNextPart:
			PeteFocusNext (win);
			break;
		case kControlFocusPrevPart:
			PeteFocusPrevious (win);
			break;
	}
	PeteSelectAll (win, win->pte);
	return (kControlEditTextPart);
}

//
//	GetPeteDItem
//
//		Retrieves the PETEHandle from a dialog item.  This sure as baked potatos
//		better be a PETE user pane, or all bets are off.  (Well, actually, we'll
//		return 'nil)
//

PETEHandle GetPeteDItem (MyWindowPtr dPtr, short itemNo)

{
	PETEHandle		pte;
	ControlHandle	theControl;
	
	pte = nil;
	if (theControl = GetDItemCtl (GetMyWindowDialogPtr(dPtr), itemNo))
		if (IsPeteControl (theControl))
			pte = (PETEHandle) GetControlReference (theControl);
	if (!PeteIsValid(pte)) pte = nil;
	return (pte);
}


//
//	GetPeteDItemTextH
//
//		Grabs the text from a PETE user pane item, returning it in a Handle
//

Handle GetPeteDItemTextH (MyWindowPtr dPtr, int item)

{
	PETEHandle	pte;
	Handle			hText,
							textH;
	
	textH = nil;
	if (pte = GetPeteDItem (dPtr, item)) {
		PeteGetRawText (pte, &hText);
		if (hText) {
			textH = hText;
			HandToHand (&textH);
		}
	}
	return (textH);
}


//
//	GetPeteDItemText
//
//		Grabs the text from a PETE user pane item, returning it in a PStr
//

void GetPeteDItemText (MyWindowPtr dPtr, int item, PStr text)

{
	PETEHandle	pte;
	Handle			hText;
	
	*text = 0;
	if (pte = GetPeteDItem (dPtr, item)) {
		PeteGetRawText (pte, &hText);
		if (hText) {
			*text = MIN (GetHandleSize (hText),255);
			BlockMoveData (*hText, text + 1, *text);
		}
	}
}


//
//	SetPeteDItemText
//
//		Sets the text of a PETE user pane item to a given string.
//

void SetPeteDItemText (MyWindowPtr dPtr, int item, PStr text)

{
	PETEHandle	pte;

	if (pte = GetPeteDItem (dPtr, item))
		PeteSetString (text, pte);
}


//
//	GetControlFromPete
//
//		Given a PETEHandle, find its user pane control, or 'nil' if no such PETE
//		can be found
//

ControlHandle GetControlFromPete (PETEHandle pte)

{
	WindowPtr			pteWinWP = GetMyWindowWindowPtr ((*PeteExtra(pte))->win);
	ControlHandle	theControl;
	
	for (theControl = GetControlList(pteWinWP); theControl; theControl = GetNextControl(theControl))
		if (IsPeteControl (theControl))
			if ((PETEHandle) GetControlReference (theControl) == pte)
				return (theControl);
	return (theControl);
}


//
//	AllWeAreSayingIsGivePeteAChance
//
//				Everybody's talkin' 'bout mouse-ism, key-ism, null-ism,
//				modifiers, message masks, concurrent tasks...
//
//				All we are saaaaaaaying... is give Pete a chance... (repeat)
//
//		Actually, what we're doing here is giving any PETE user panes a shot
//		at an event that occurs inside of the dialog manager.  We do this here
//		(where we are called by MovableModalDialog) so that we don't have to
//		put PETE user pane handling code in lots of different filter procs.
//
//		Note: Could this function completely go away by just allowing the
//					standard filter proc to hand off events to us via the user
//					pane procs?	
//
//		For now, all we do is check for a click in a PETE user pane, set the
//		item to the item hit, and return true.

Boolean AllWeAreSayingIsGivePeteAChance (MyWindowPtr win, EventRecord *event, short *peteItemHit)

{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Point			where;
	short			item;
	Boolean		hitPETEUserPane;
	
	hitPETEUserPane = false;

	// Make sure this is one of our windows (not a plain old dialog) and check
	// to see whether or not a mouse down event occurred in a PETE field.
	if (IsMyWindow (winWP))
		if (event->what == mouseDown) {
			SetPort (GetWindowPort(winWP));
			where = event->where;
			GlobalToLocal (&where);
			if (item = FindDialogItem (GetMyWindowDialogPtr(win), where) + 1)
				if (GetPeteDItem (win, item)) {
					*peteItemHit = item;
					hitPETEUserPane = true;
				}
		}
	return (hitPETEUserPane);
}
