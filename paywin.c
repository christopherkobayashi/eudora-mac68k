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

#include "paywin.h"
#include "regcode_v2.h"

#define FILE_NUM 127
/* Copyright (c) 1999 by QUALCOMM Incorporated */

#pragma segment PayWin

#define MemResError()	(MemError () ? MemError ()	: ResError ())

typedef struct {
	MyWindowPtr		win;
	// Buttons we care about...
	ControlHandle whichEudoraGroup;
	ControlHandle keepCurrentGroup;
	ControlHandle regInfoGroup;
	ControlHandle paywareButton;
	ControlHandle adwareButton;
	ControlHandle freeButton;
	ControlHandle registerButton;
	ControlHandle customizeButton;
	ControlHandle	updateButton;
	ControlHandle	moreInfoButton;
	ControlHandle	nameText;
	ControlHandle	regcodeText;
	ControlHandle	changeCodeButton;
	Boolean				inited;
	Boolean				updateInProgress;
	OSErr					updateError;
} WinData;
static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win);
static void DoButton (MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part);
static void DoActivate (MyWindowPtr win);
static void DoTransition (MyWindowPtr win, UserStateType oldState, UserStateType newState);
static void OpenRefundWindow (void);
static Boolean RefundDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit,long dialogRefcon);
Boolean PreRegHitProc (EventRecord *event, DialogPtr theDialog, short itemHit, long refcon);
static Boolean MatchAnyPersonality (Str255 address);
static OSErr ProfileReceivedDlogInit (DialogPtr dlog, PStr profileID);
static Boolean ProfileReceivedDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, StringHandle dialogRefcon);
static Boolean AnyRegInfoAtAll (UserStateType	state);
void PayWinIdle(MyWindowPtr win);


extern ModalFilterUPP DlgFilterUPP;

/************************************************************************
 * OpenPayWin - open the pay window
 ************************************************************************/

void OpenPayWin (void)

{
	WindowPtr			gWinWinWP;
	Rect					controlRect,
								titleRect,
								insideOutButtonRect;
	OSErr					theError;
	Size					actualSize;
	short					groupWi;
	Rect				rPort;

	if (gWin.inited) {
		UserSelectWindow (GetMyWindowWindowPtr(gWin.win));
		return;
	}
	
	gWin.win = GetNewMyWindow (PAY_WIND, nil, nil, BehindModal, false, false, PAY_WIN);
	theError = MemResError ();
	
	if (!theError) {
		gWinWinWP = GetMyWindowWindowPtr (gWin.win);
		
		SetPort_(GetWindowPort(gWinWinWP));
		ConfigFontSetup(gWin.win);	
		MySetThemeWindowBackground(gWin.win,kThemeActiveModelessDialogBackgroundBrush,False);

		// Controls
		if (!(gWin.whichEudoraGroup = GetNewControlSmall_ (PAY_WHICH_EUDORA_TITLE, gWinWinWP)))
			theError = MemResError ();
		if (!theError && !(gWin.keepCurrentGroup = GetNewControlSmall_ (PAY_KEEP_CURRENT_TITLE, gWinWinWP)))
			theError = MemResError ();
		if (!theError && !(gWin.regInfoGroup = GetNewControlSmall_ (PAY_REG_INFO_TITLE, gWinWinWP)))
			theError = MemResError ();
	}

	GetPortBounds(GetQDGlobalsThePort(),&rPort);
	if (!theError)
	{
		groupWi = RectWi(rPort) - 4*INSET;
		theError = SetBevelFontSize (gWin.whichEudoraGroup, kControlFontSmallBoldSystemFont, 0);
		SizeControl(gWin.whichEudoraGroup,groupWi,ControlHi(gWin.whichEudoraGroup));
	}
	if (!theError)
	{
		theError = SetBevelFontSize (gWin.keepCurrentGroup, kControlFontSmallBoldSystemFont, 0);
		SizeControl(gWin.keepCurrentGroup,groupWi,ControlHi(gWin.keepCurrentGroup));
	}
	if (!theError)
	{
		theError = SetBevelFontSize (gWin.regInfoGroup, kControlFontSmallBoldSystemFont, 0);
		SizeControl(gWin.regInfoGroup,groupWi,ControlHi(gWin.regInfoGroup));
	}

	// Group control - Which Eudora is for you?
	if (!theError) {
		GetControlBounds(gWin.whichEudoraGroup,&controlRect);
		if (AppearanceVersion() < 0x0110) {
			ControlFontStyleRec	fontStyle;
			titleRect = controlRect;
			if (!(theError = GetControlData (gWin.whichEudoraGroup, 0, kControlGroupBoxFontStyleTag, sizeof (fontStyle), (void *) &fontStyle, &actualSize)))
				titleRect.bottom = controlRect.top + GetAscent (fontStyle.font, fontStyle.size);
		}
		else
			theError = GetControlData (gWin.whichEudoraGroup, 0, kControlGroupBoxTitleRectTag, sizeof (titleRect), (void *) &titleRect, &actualSize);
	}
	if (!theError) {
		controlRect.top = titleRect.bottom;
		InsetRect (&controlRect, 4, 4);
		insideOutButtonRect.top		 = controlRect.top + kPayWindowInsideOutButtonTopMargin;
		insideOutButtonRect.bottom = controlRect.bottom;
		insideOutButtonRect.left	 = controlRect.left;
		insideOutButtonRect.right	 = insideOutButtonRect.left + (controlRect.right - controlRect.left) / 3;
		// Adware!
		gWin.adwareButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, AD_VERSION_ICON, ADWARE_VERSION_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth, payWinAdwareButton);
		if (IsAdwareMode ())
			SetControlValue (FindInsideOutBevelIconButtonControl (gWin.adwareButton, kControlButtonPart), 1);
		
		// Payware!
		insideOutButtonRect.left	= insideOutButtonRect.right + 1;
		insideOutButtonRect.right	= insideOutButtonRect.left + (controlRect.right - controlRect.left) / 3;
		gWin.paywareButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, PAY_VERSION_ICON, PAY_VERSION_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth, payWinPaywareButton);
		if (IsPayMode ())
			SetControlValue (FindInsideOutBevelIconButtonControl (gWin.paywareButton, kControlButtonPart), 1);

		// Freeware!
		insideOutButtonRect.left	= insideOutButtonRect.right + 1;
		insideOutButtonRect.right	= controlRect.right;
		gWin.freeButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, FREE_VERSION_ICON, FREE_VERSION_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth, payWinFreewareButton);
		if (IsFreeMode ())
			SetControlValue (FindInsideOutBevelIconButtonControl (gWin.freeButton, kControlButtonPart), 1);
	
		EmbedControl (gWin.adwareButton,gWin.whichEudoraGroup);
		EmbedControl (gWin.paywareButton,gWin.whichEudoraGroup);
		EmbedControl (gWin.freeButton,gWin.whichEudoraGroup);
	}
	
	// Group control - Keeping Current
	if (!theError) {
		GetControlBounds(gWin.keepCurrentGroup,&controlRect);
		if (AppearanceVersion() < 0x0110) {
			ControlFontStyleRec	fontStyle;
			titleRect = controlRect;
			if (!(theError = GetControlData (gWin.whichEudoraGroup, 0, kControlGroupBoxFontStyleTag, sizeof (fontStyle), (void *) &fontStyle, &actualSize)))
				titleRect.bottom = controlRect.top + GetAscent (fontStyle.font, fontStyle.size);
		}
		else
			theError = GetControlData (gWin.keepCurrentGroup, 0, kControlGroupBoxTitleRectTag, sizeof (titleRect), (void *) &titleRect, &actualSize);
	}
	if (!theError) {
		controlRect.top = titleRect.bottom;
		InsetRect (&controlRect, 4, 4);
		insideOutButtonRect.top		 = controlRect.top + kPayWindowInsideOutButtonTopMargin;
		insideOutButtonRect.bottom = controlRect.bottom;
		insideOutButtonRect.left	 = controlRect.left;
		insideOutButtonRect.right	 = insideOutButtonRect.left + (controlRect.right - controlRect.left) / 3;
		// Register with us
		gWin.registerButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, REGISTER_ICON, IsRegisteredUser () ? UPDATE_REGISTER_BUTTON_TITLE : REGISTER_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth-18, payWinRegisterButton);
		// Customize the Ads You See
		insideOutButtonRect.left	= insideOutButtonRect.right + 1;
		insideOutButtonRect.right	= insideOutButtonRect.left + (controlRect.right - controlRect.left) / 3;
		gWin.customizeButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, CUSTOMIZE_ADS_ICON, IsProfiledUser() ? UPDATE_PROFILE_BUTTON_TITLE : CUSTOMIZE_ADS_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth - 36, payWinCustomizeAdsButton);
		// Find the Latest Versions
		insideOutButtonRect.left	= insideOutButtonRect.right + 1;
		insideOutButtonRect.right	= controlRect.right;
		gWin.updateButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, UPDATES_ICON, UPDATES_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth - 18, payWinUpdatesButton);

		EmbedControl(gWin.registerButton,gWin.keepCurrentGroup);
		EmbedControl(gWin.customizeButton,gWin.keepCurrentGroup);
		EmbedControl(gWin.updateButton,gWin.keepCurrentGroup);
	}
	
	// Group control - Your Registration Information
	if (!theError) {
		GetControlBounds(gWin.regInfoGroup,&controlRect);
		if (AppearanceVersion() < 0x0110) {
			ControlFontStyleRec	fontStyle;
			titleRect = controlRect;
			if (!(theError = GetControlData (gWin.whichEudoraGroup, 0, kControlGroupBoxFontStyleTag, sizeof (fontStyle), (void *) &fontStyle, &actualSize)))
				titleRect.bottom = controlRect.top + GetAscent (fontStyle.font, fontStyle.size);
		}
		else
			theError = GetControlData (gWin.regInfoGroup, 0, kControlGroupBoxTitleRectTag, sizeof (titleRect), (void *) &titleRect, &actualSize);
	}
	if (!theError) {
		controlRect.top = titleRect.bottom;
		InsetRect (&controlRect, 4, 4);
		insideOutButtonRect.top		 = controlRect.top + kPayWindowInsideOutButtonTopMargin;
		insideOutButtonRect.bottom = controlRect.bottom;
		gWin.changeCodeButton = CreateInsideOutBevelIconButtonUserPane (gWinWinWP, CHANGE_REG_ICON, AnyRegInfoAtAll (GetNagState ()) ? CHANGE_REGISTRATION_BUTTON_TITLE : ENTER_REGISTRATION_BUTTON_TITLE, &insideOutButtonRect, kPayWinIconSize, kPayWinMaxTextWidth - 18, payWinChangeRegistrationButton);

		// Create text controls for the registration name and code
		GetControlBounds(gWin.regInfoGroup,&controlRect);
		controlRect.left  += kPayWindowRegInfoLeftMargin;
		controlRect.right  = insideOutButtonRect.left - kPayWindowRegInfoRightMargin;
		controlRect.top    = controlRect.top + (controlRect.bottom - controlRect.top - (gWin.win->vPitch * 2 + 12)) / 2;
		controlRect.bottom = controlRect.top + gWin.win->vPitch + 4;
		gWin.nameText = NewControl (gWinWinWP, &controlRect, "\p", true, 0, 0, 1, kControlStaticTextProc, 0);
		controlRect.top    = controlRect.bottom + 4;
		controlRect.bottom = controlRect.top + gWin.win->vPitch + 4;
		gWin.regcodeText = NewControl (gWinWinWP, &controlRect, "\p", true, 0, 0, 1, kControlStaticTextProc, 0);
		LetsGetSmall(gWin.regcodeText);
		
		// More info button
		if (gWin.moreInfoButton = NewIconButton (MORE_INFO_CNTL, gWinWinWP)) {
			short width = GetCtlNameWidth (gWin.moreInfoButton) + 22 + 6;
			GetControlBounds(gWin.regInfoGroup,&controlRect);
			MoveMyCntl (gWin.moreInfoButton, rPort.left + ((rPort.right - rPort.left - width) >> 1), controlRect.bottom + ((rPort.bottom - controlRect.bottom - kHtCtl) >> 1), width, kHtCtl);
			SetControlReference (gWin.moreInfoButton, payWinMoreInfoButton);
		}
	
		gWin.win->close						= DoClose;
		gWin.win->button					= DoButton;
		gWin.win->activate				= DoActivate;
		gWin.win->transition			= DoTransition;
		gWin.win->idle						= PayWinIdle;
		gWin.win->isRunt					= true;
		gWin.win->centerAsDefault	= true;

		ShowMyWindow (gWinWinWP);
		gWin.inited = true;
		theError = UpdateRegInfoText (GetNagState ());

		EmbedControl(gWin.changeCodeButton,gWin.regInfoGroup);
	}
	if (theError) {
		if (gWinWinWP)
			CloseMyWindow (gWinWinWP);
		WarnUser (COULDNT_WIN, theError);
	}
}


OSErr UpdateRegInfoText (UserStateType state)

{
	GrafPtr	oldPort;
	Str255	regName,
					string;
	OSErr		theError;
	
	theError = noErr;
	if (gWin.win && gWin.inited) {
		GetPort (&oldPort);
		SetPort (GetMyWindowCGrafPtr(gWin.win));

		*regName = 0;
#ifdef I_HATE_THE_BOX
		if (!BoxUser (state))
#endif
			GetRegFirst (state, regName);
		if (regName[0])
			PCatC (regName, ' ');
		
		*string = 0;
#ifdef I_HATE_THE_BOX
		if (!BoxUser (state))
#endif
			PCat (regName, GetRegLast (state, string));
		if (!regName[0])
			GetRString (regName, NO_REG_NAME_TEXT);
		if (gWin.nameText)
			theError = SetTextControlText (gWin.nameText, regName, nil);
		if (!theError) {
			*string = 0;
#ifdef I_HATE_THE_BOX
			if (!BoxUser (state))
#endif
				GetRegCode (state, string);
			if (!string[0])
				GetRString (string, NO_REG_CODE_TEXT);
			{
				// insert date information
				int pnPolicy, pnMonth;
				
				if (ValidRegCode(state,&pnPolicy,&pnMonth))
				{
					ComposeRString(regName,REGCODE_PLUS_MONTH,string,MONTH_STRN+1+pnMonth%12,1999+pnMonth/12);
					PCopy(string,regName);
				}
			}
			if (gWin.regcodeText)
				theError = SetTextControlText (gWin.regcodeText, string, nil);
		}
		SetPort (oldPort);
	}
	return (theError);
}

Boolean	AnyRegInfoAtAll (UserStateType	state)

{
	Str255	scratch;
	
	GetRegFirst (state, scratch);
	if (scratch[0])	return (true);
	GetRegLast (state, scratch);
	if (scratch[0])	return (true);
	GetRegCode (state, scratch);
	if (scratch[0])	return (true);
	return (false);
}


void TellPayWindowTheUpdateCheckIsDone (OSErr err)

{
	GrafPtr	oldPort;
	
	if (gWin.inited) {
		gWin.updateInProgress = false;
		gWin.updateError = err;
		GetPort (&oldPort);
		SetPort (GetMyWindowCGrafPtr(gWin.win));
		SetControlValue (FindInsideOutBevelIconButtonControl (gWin.updateButton, kControlButtonPart), 0);
		SetPort (oldPort);
	}
}


/************************************************************************
 * DoClose - close the window
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win)
{
#pragma unused(win)
	
	gWin.inited = false;
	return (true);
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate (MyWindowPtr win)

{
	GrafPtr	oldPort;
	
	if (win->isActive) {
#ifdef I_HATE_THE_BOX
		NagRec	nag;
		short		nagID,
						index = 1;
		Boolean	canRegister;
		canRegister = false;
		while (!canRegister && !GetIndNagState (&nag, &nagID, NAG_BOX_USER, index++))
			if (nagID == REGISTRATION_NAG)
				canRegister = true;
#endif
		GetPort (&oldPort);
		SetPort (GetMyWindowCGrafPtr(gWin.win));
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
		if (IsDeadbeatUser () && !IsProfileDeadbeatUser())
			DeactivateControl (gWin.adwareButton);
		else
#endif
			if (IsPayMode ())
				DeactivateControl (gWin.customizeButton);
//		if (IsRegisteredUser ())
//			DeactivateControl (gWin.registerButton);
#ifdef I_HATE_THE_BOX
		if (!canRegister) {
			DeactivateControl (gWin.registerButton);
			DeactivateControl (gWin.customizeButton);
			DeactivateControl (gWin.changeCodeButton);
		}
#endif
		SetPort (oldPort);
	}
}


/************************************************************************
 * DoTransition - transition the window between user states
 ************************************************************************/
static void DoTransition (MyWindowPtr win, UserStateType oldState, UserStateType newState)

{
	CGrafPtr	oldPort;

	// When transitioning the payment and registration window we may need to change button states
	GetPort (&oldPort);
	SetPort (GetMyWindowCGrafPtr(win));
	SetControlValue (FindInsideOutBevelIconButtonControl (gWin.paywareButton, kControlButtonPart), PayMode(newState));
	SetControlValue (FindInsideOutBevelIconButtonControl (gWin.adwareButton, kControlButtonPart), AdwareMode(newState));
	SetControlValue (FindInsideOutBevelIconButtonControl (gWin.freeButton, kControlButtonPart), FreeMode(newState));
	SetPort (oldPort);
	
	DoActivate (win);
}


/************************************************************************
 * DoButton - handle a hit in a button
 ************************************************************************/
void DoButton (MyWindowPtr win, ControlHandle buttonHandle, long modifiers, short part)

{
	WindowPtr	gWinWinWP = GetMyWindowWindowPtr (gWin.win);
#ifndef I_HATE_THE_BOX
	Str255		regCode;
#endif
	Handle		regInfo;
	UInt16		buttonID;
	int				pnPolicyCode,
						regMonth;
	short			action,
						itemHit;

	switch (buttonID = GetControlReference (buttonHandle) & 0x0000FFFF) {
		case payWinAdwareButton:
			if (modifiers&shiftKey) {
				if (ComposeStdAlert (Caution, SWITCH_TO_SPONSORED) == ok) {
					// Blow away the registration code as soon as they're committed to switch
					// Don't forget that other parts of the code expect 3 Pascal strings, so
					// seed the empty preference with three consecutive nulls
					if (regInfo = NuHandleClear (3)) {
						if (!SettingsHandle (REG_PREF_TYPE, nil, REG_PREF_PAID, regInfo)) {
							MyUpdateResFile	(SettingsRefN);
							ReleaseResource (regInfo);
						}
					}
					CloseMyWindow (gWinWinWP);
					TransitionState (adwareUser);
					OpenRefundWindow ();
				}
			}
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
			else if (IsProfileDeadbeatUser()) {
				// User must profile
				if (SendUserToProfile())
				if (kAlertStdAlertOKButton==ComposeStdAlert(Note,-PROFILING_NOW))
				{
					// user has indicated he has profiled.  Make a playlist request
					ForcePlaylistRequest();
					break;
				}
			}
#endif
			else {
				CloseMyWindow (gWinWinWP);
				TransitionState (ValidRegCode (regAdwareUser, nil, nil) ? regAdwareUser : adwareUser);
			}
			break;
		case payWinPaywareButton:
			// Check to see if we have some inkling that this user has a paid reg code...  If so, just switch 'em
#ifdef I_HATE_THE_BOX
			TransitionState (ValidRegCode (paidUser, &pnPolicyCode, &regMonth) ? paidUser : boxUser);
#else
			if (ValidRegCode (paidUser, &pnPolicyCode, &regMonth)) {
				if (!PolicyCheck (pnPolicyCode, regMonth))
					RepayDialog (repayPending);
				else
					TransitionState (paidUser);
			}
			else {
				GetRegCode (paidUser, regCode);
				if (regCode[0])
					(void) CodeEntryForProduct (paidUser, invalidRegCodeEntryVariant);
				else
					StartPaymentProcess();
			}
#endif
			break;
		case payWinFreewareButton:
			DownGradeDialog ();
			break;
		case payWinRegisterButton:
#ifdef I_HATE_THE_BOX
			action = IsAdwareMode () ? actionRegisterAd : (IsFreeMode () ? actionRegisterFree : actionRegister50box);
#else
			action = IsAdwareMode () ? actionRegisterAd : actionRegisterFree;
#endif
				if (!Nag (PRE_REGISTRATION_DLOG, nil, PreRegHitProc, DlgFilterUPP, false, (long) &itemHit))
					if (itemHit == ok)
						OpenAdwareURL (GetNagState (), REG_SITE, action, registrationQuery, nil);
			break;
		case payWinCustomizeAdsButton:
			SendUserToProfile();
			break;
		case payWinUpdatesButton:
			if (!Offline || !GoOnline()) {
				SetControlValue (FindInsideOutBevelIconButtonControl (gWin.updateButton, kControlButtonPart), 1);
				gWin.updateInProgress = true;
				UpdateCheck (false, true);
			}
			break;
		case payWinChangeRegistrationButton:
			CodeEntryDialog (nil);
			break;
		case payWinMoreInfoButton:
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionPayReg, payRegQuery, nil);
			break;
	}
	
	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(gWinWinWP), AUDITCONTROLID(GetWindowKind(gWinWinWP),buttonID), mouseDown);
}

void StartPaymentProcess(void)
{
	short itemHit;
	
	if (!Nag (PRE_PAYMENT_DLOG, nil, PreRegHitProc, DlgFilterUPP, false, (long) &itemHit))
		if (itemHit == ok)
			OpenAdwareURL (GetNagState (), REG_SITE, actionPay, paymentQuery, nil);
}

Boolean SendUserToProfile(void)
{
	Str63 profileID;
	short itemHit;
	
	GetProfileID (profileID);
	if (profileID[0]) {
		if (ComposeStdAlert (Note, -PRE_PROFILE_UPDATE_NOTE) == ok)
		{
			OpenAdwareURL (GetNagState (), REG_SITE, actionProfile, profileQuery, nil);
			return true;
		}
	}
	else
		if (!Nag (PRE_PROFILING_DLOG, nil, PreRegHitProc, DlgFilterUPP, false, (long) &itemHit))
			if (itemHit == ok)
			{
				OpenAdwareURL (GetNagState (), REG_SITE, actionProfile, profileQuery, nil);
				return true;
			}
	return false;
}

Boolean PreRegHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon)

{
	short	*dItem;
	
	dItem = (short *) refcon;
	*dItem = itemHit;
	return (true);
}


OSErr CodeEntryInit(DialogPtr codeEntry, long dialogRefcon);
Boolean CodeEntryHit(EventRecord *event, DialogPtr codeEntry, short itemNo, long dialogRefcon);

OSErr CodeEntryForProduct (UserStateType state, CodeEntryVariantType variant)

{
	StringHandle	regInfo;
	Str255				regCode;
	Str255				firstName;
	Str255				lastName;
	OSErr					theError;
	
	GetRegFirst (state, firstName);
	GetRegLast (state, lastName);
	GetRegCode (state, regCode);
	if (regInfo = (StringHandle) NuHTempBetter (1)) {
		**regInfo = variant;
		firstName[++firstName[0]] = 0;
		lastName[++lastName[0]] = 0;
		regCode[++regCode[0]] = 0;
		theError = PtrPlusHand (&firstName[1], regInfo, firstName[0]);
		if (!theError)
			theError = PtrPlusHand (&lastName[1], regInfo, lastName[0]);
		if (!theError)
			theError = PtrPlusHand (&regCode[1], regInfo, regCode[0]);
		if (!theError)
			theError = CodeEntryDialog (regInfo);
		ZapHandle (regInfo);
	}
	return (theError);
}

// The first byte in the 'regInfo' handle is now a variation coe for tailoring the
// static string items in the dialog. (But only, of course, if 'regInfo' is not nil)
OSErr CodeEntryDialog(StringHandle regInfo)
{
	OSErr err;
	
	err = Nag(20100, CodeEntryInit, CodeEntryHit, DlgFilterUPP, false, (long)regInfo);
	return err;
}

OSErr CodeEntryInit(DialogPtr codeEntry, long dialogRefcon)
{
	OSErr err;
	ControlHandle firstCtl, lastCtl, regCtl, promptCtl, titleCtl;
	Handle regInfo;
	int len;
	
	err = GetDialogItemAsControl(codeEntry, 4, &titleCtl);
	if(err) return err;
	err = GetDialogItemAsControl(codeEntry, 5, &promptCtl);
	if(err) return err;
	err = GetDialogItemAsControl(codeEntry, 6, &firstCtl);
	if(err) return err;
	err = GetDialogItemAsControl(codeEntry, 7, &lastCtl);
	if(err) return err;
	err = GetDialogItemAsControl(codeEntry, 8, &regCtl);
	if(err) return err;
	if((regInfo = (Handle)dialogRefcon) != nil)
	{
		switch (**regInfo) {
			case fromButtonCodeEntryVariant:
				GetRString(GlobalTemp, REG_THANK_YOU_PROMPT);
				SetControlTitle(titleCtl, GlobalTemp);
				GetRString(GlobalTemp, REG_FROM_BUTTON_DESC);
				SetDialogItemText(promptCtl, GlobalTemp);
				break;
			case fromFileCodeEntryVariant:
				GetRString(GlobalTemp, REG_THANK_YOU_PROMPT);
				SetControlTitle(titleCtl, GlobalTemp);
				GetRString(GlobalTemp, REG_FROM_FILE_DESC);
				SetDialogItemText(promptCtl, GlobalTemp);
				break;
			case invalidRegCodeEntryVariant:
				GetRString(GlobalTemp, REG_INVALID_PROMPT);
				SetControlTitle(titleCtl, GlobalTemp);
				GetRString(GlobalTemp, REG_FROM_INVALID_CODE);
				SetDialogItemText(promptCtl, GlobalTemp);
				break;
		}
		CtoPPtrCpy(GlobalTemp, *regInfo + 1);
		SetDialogItemText(firstCtl, GlobalTemp);
		len = strlen(*regInfo + 1);
		CtoPPtrCpy(GlobalTemp, *regInfo + len + 2);
		SetDialogItemText(lastCtl, GlobalTemp);
		len += strlen(*regInfo + len + 2);
		CtoPPtrCpy(GlobalTemp, *regInfo + len + 3);
		SetDialogItemText(regCtl, GlobalTemp);
	}
	else
	{
		short state = GetNagState ();

		GetRString(GlobalTemp, REG_FROM_BUTTON_DESC);
		SetDialogItemText(promptCtl, GlobalTemp);
		GetRegFirst(state, GlobalTemp);
		SetDialogItemText(firstCtl, GlobalTemp);
		GetRegLast(state, GlobalTemp);
		SetDialogItemText(lastCtl, GlobalTemp);
		GetRegCode(state, GlobalTemp);
		SetDialogItemText(regCtl, GlobalTemp);
	}
	SelectDialogItemText (codeEntry, 6, 0, REAL_BIG);
	return noErr;
}

Boolean CodeEntryHit (EventRecord *event, DialogPtr codeEntry, short itemNo, long dialogRefcon)

{
	UserStateType	newState;
	ControlHandle	theControl;
	Str255				firstName,
								lastName,
								fullName,
								regCode;
	Handle				regInfo;
	OSErr					theError;
	int						pnProduct,
								regMonth;
	short					resID;
	Boolean				results,
								codeVerifies,
								policyChecksOut;

	results = true;

	regInfo = NuHTempBetter (0);
	theError = MemError ();
	
	if (!theError)
		switch (itemNo) {
			case kStdOkItemIndex:
				// get the text from the three edit fields
				theError = GetDialogItemAsControl (codeEntry, 6, &theControl);
				if (!theError && theControl) {
					GetDialogItemText (theControl, firstName);
					theError = GetDialogItemAsControl (codeEntry, 7, &theControl);
				}
				if (!theError && theControl) {
					GetDialogItemText (theControl, lastName);
					theError = GetDialogItemAsControl (codeEntry, 8, &theControl);
				}
				if (!theError && theControl)
					GetDialogItemText (theControl, regCode);
				
				// In preparation for verifying the code... 
				if (!theError) {
					PCopy (fullName, firstName);
					PCat (fullName, "\p ");
					PCat (fullName, lastName);
					if (*firstName < sizeof (firstName)) firstName[*firstName + 1] = 0; else firstName[sizeof (firstName)] = 0;
					if (*lastName < sizeof (lastName)) lastName[*lastName + 1] = 0; else lastName[sizeof (lastName)] = 0;
					if (*fullName < sizeof (fullName)) fullName[*fullName + 1] = 0; else fullName[sizeof (fullName)] = 0;
					if (*regCode < sizeof (regCode)) regCode[*regCode + 1] = 0; else regCode[sizeof (regCode)] = 0;

					// Verify the code and check out the policy
					codeVerifies = RegCodeVerifies (&regCode[1], &fullName[1], &pnProduct, &regMonth);
					policyChecksOut = PolicyCheck (pnProduct, regMonth);

					// If the code verifies, we'll save the regCode (unless, of course, the policy did not check out)
					if (codeVerifies) {
						switch (pnProduct) {
							case REG_EUD_AD_WARE:
								resID		 = REG_PREF_SPONSORED;
								newState = regAdwareUser;
								policyChecksOut = true;	// save it, even though it's not valid PAID code, since it is a valid adware code
								break;
							case REG_EUD_LIGHT:
								resID		 = REG_PREF_LIGHT;
								newState = regFreeUser;
								policyChecksOut = true;	// save it, even though it's not valid PAID code, since it is a valid light code
								break;
							case REG_EUD_PAID:
							case REG_EUD_50_PAID_TRIMODE:
							case REG_EUD_50_PAID_BOX_ESD:
							case REG_EUD_50_PAID_37_RSRV:
							case REG_EUD_50_PAID_38_RSRV:
							case REG_EUD_50_PAID_39_RSRV:
							case REG_EUD_50_PAID_40_RSRV:
							case REG_EUD_50_PAID_EN_ONLY:
							case REG_EUD_50_PAID_EN_NOT_X1:
								if (policyChecksOut) {
									resID		 = REG_PREF_PAID;
									newState = paidUser;
								}
#ifndef I_HATE_THE_BOX
								else
									RepayDialog (repayPending);
#endif
								break;
							default :
								ComposeStdAlert (kAlertStopAlert, REG_INVALID);
								results = false;
								break;
						}
					}
					else {
						ComposeStdAlert (kAlertStopAlert, REG_INVALID);
						results = false;
					}
					if (policyChecksOut) {
						regInfo = NuHTempBetter (0);
						theError = MemError ();
						if (!theError)
							theError = PtrPlusHand (&firstName[1], regInfo, *firstName + 1);
						if (!theError)
							theError = PtrPlusHand (&lastName[1], regInfo, *lastName + 1);
						if (!theError)
							theError = PtrPlusHand (&regCode[1], regInfo, *regCode + 1);
						if (!theError) {
							theError = SettingsHandle (REG_PREF_TYPE, nil, resID, regInfo);
							if (!theError) {
								MyUpdateResFile	(SettingsRefN);
								ReleaseResource (regInfo);
								regInfo = nil;
							}
						}
						ZapHandle (regInfo);
					}
				}
				if(!theError && gWin.win && policyChecksOut)
					theError = UpdateRegInfoText (newState);
				if (!theError)
					TransitionState (newState);
				break;
			case kStdCancelItemIndex :
				break;
			case 3 :
				OpenAdwareURL (GetNagState (), REG_SITE, actionLostCode, lostCodeQuery, nil);
				break;
			default:
				results = false;
				break;
		}
	if (theError)
		ComposeStdAlert (kAlertStopAlert, CANT_REGISTER);
	return (results);
}
/*
Boolean CodeEntryHit(EventRecord *event, DialogPtr codeEntry, short itemNo, long dialogRefcon)
{
	UserStateType	newState;
	Handle regInfo;
	Boolean noRegInfo,
					codeVerifies;
	int pnProduct,regMonth;
	unsigned long firstSize, lastSize, regSize;
	ControlHandle firstCtl, lastCtl, regCtl;
	OSErr err = noErr;
	
	regInfo = (Handle)dialogRefcon;
	switch(itemNo)
	{
		default :
			return false;
		case kStdOkItemIndex :

			if(noRegInfo = (regInfo == nil))
			{
				if(err = GetDialogItemAsControl(codeEntry, 6, &firstCtl)) break;
				if(err = GetDialogItemAsControl(codeEntry, 7, &lastCtl)) break;
				if(err = GetDialogItemAsControl(codeEntry, 8, &regCtl)) break;
				if(err = GetControlDataSize(firstCtl, kControlEntireControl, kControlEditTextTextTag, &firstSize)) break;
				if(err = GetControlDataSize(lastCtl, kControlEntireControl, kControlEditTextTextTag, &lastSize)) break;
				if(err = GetControlDataSize(regCtl, kControlEntireControl, kControlEditTextTextTag, &regSize)) break;
				regInfo = NuHTempBetter(firstSize + lastSize + regSize + 3);
				if(err = MemError()) break;
			}
			else
			{
				// Remove the man-made variation code...
				Munger (regInfo, 0, nil, 1, nil, 0);
				firstSize = strlen(*regInfo);
				lastSize = strlen(*regInfo + firstSize + 1);
				regSize = strlen(*regInfo + firstSize + lastSize + 2);
			}
			HLock(regInfo);
			if(!noRegInfo ||
			   (((err = GetControlData(firstCtl, kControlEntireControl, kControlEditTextTextTag, firstSize, *regInfo, &firstSize)) == noErr) &&
			    ((err = GetControlData(lastCtl, kControlEntireControl, kControlEditTextTextTag, lastSize, *regInfo + firstSize + 1, &lastSize)) == noErr) &&
			    ((err = GetControlData(regCtl, kControlEntireControl, kControlEditTextTextTag, regSize, *regInfo + firstSize + lastSize + 2, &regSize)) == noErr))) {

				(*regInfo)[firstSize] = ' ';
				(*regInfo)[firstSize + lastSize + 1] = 0;
				(*regInfo)[firstSize + lastSize + regSize + 2] = 0;
				codeVerifies = RegCodeVerifies(*regInfo + firstSize + lastSize + 2, *regInfo, &pnProduct, &regMonth);
				(*regInfo)[firstSize] = 0;
			}
			HUnlock(regInfo);
//			CloseMyWindow(GetDialogWindow(codeEntry));
			if(!err) {
				short	resID;
				Boolean	policyChecksOut = PolicyCheck (pnProduct, regMonth);				
				switch(pnProduct) {
					case REG_EUD_AD_WARE:
						if (codeVerifies) {
							resID = REG_PREF_SPONSORED;
							newState = regAdwareUser;
							policyChecksOut = true;
						}
						break;
					case REG_EUD_LIGHT:
						if (codeVerifies) {
							resID = REG_PREF_LIGHT;
							newState = regFreeUser;
							policyChecksOut = true;
						}
						break;
					case REG_EUD_PAID:
					case REG_EUD_50_PAID_TRIMODE:
					case REG_EUD_50_PAID_BOX_ESD:
					case REG_EUD_50_PAID_37_RSRV:
					case REG_EUD_50_PAID_38_RSRV:
					case REG_EUD_50_PAID_39_RSRV:
					case REG_EUD_50_PAID_40_RSRV:
					case REG_EUD_50_PAID_EN_ONLY:
					case REG_EUD_50_PAID_EN_NOT_X1:
						if (policyChecksOut) {
							resID = REG_PREF_PAID;
							newState = paidUser;
						}
						else
							RepayDialog (repayPending);
						break;
					default :
						ComposeStdAlert(kAlertStopAlert, REG_INVALID);
						return false;
				}
				if (policyChecksOut)
					err = SettingsHandle(REG_PREF_TYPE, nil, resID, regInfo);
			}
			if(!err && gWin.win)
				err = UpdateRegInfoText (newState);
			if (!err) {
				regInfo = nil;
				TransitionState(newState);
			}
			break;
		case kStdCancelItemIndex :
//			CloseMyWindow(GetDialogWindow(codeEntry));
			break;
		case 3 :
			OpenAdwareURL (GetNagState (), REG_SITE, actionLostCode, lostCodeQuery, nil);
	}
	ZapHandle(regInfo);
	if(err) ComposeStdAlert(kAlertStopAlert, CANT_REGISTER);
	return true;
}
*/

OSErr ParseRegCodeType (parseNoiseType parseNoise, HeaderDHandle hdh, StringHandle h, Boolean *needsRegistration, int *pnPolicyCode);
OSErr ParseProfileType (HeaderDHandle hdh);

OSErr ParseRegFile(FSSpecPtr spec, parseNoiseType parseNoise, Boolean *needsRegistration, int *pnPolicyCode)
{
	OSErr err;
	StringHandle h;
	HeaderDHandle hdh;
	
	*needsRegistration = false;
	
	err = Snarf(spec, &h, 0L);
	if(err) return err;
	
	err = ParseAHeaderLo(h, &hdh, RegCodeHeadStrn, RegCodeHeadLimit);
	if(err) return err;
	
	*GlobalTemp = 0;
	err = AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hFileType, GlobalTemp);
	if(!err) {
		TrimWhite(GlobalTemp);
		TrimInitialWhite(GlobalTemp);
		switch(FindSTRNIndex(RegCodeFileTypeStrn, GlobalTemp)) {
			case typeRegCode :
				err = ParseRegCodeType (parseNoise, hdh, h, needsRegistration, pnPolicyCode);
				if(err || parseNoise != parseDoDialog) DisposeHandle(h);
				break;
			case typeProfile :
				DisposeHandle(h);
				err = ParseProfileType (hdh);
				break;
			default :
				DisposeHandle(h);
		}
		DisposeHeaderDesc(hdh);
		MyUpdateResFile	(SettingsRefN);
	}
	
	return err;
}


OSErr ParseRegCodeType (parseNoiseType parseNoise, HeaderDHandle hdh, StringHandle h, Boolean *needsRegistration, int *pnPolicyCode)

{
	Str255	firstName,
					lastName,
					regCode;
	OSErr		err;
	unsigned long firstSize, lastSize;
	char variant;

	*firstName = 0;
	if((err = AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hRegFirst, firstName)) == noErr)
	{
		TrimWhite(firstName);
		TrimInitialWhite(firstName);
		firstSize = firstName[0];
		firstName[++(firstName[0])] = 0;
		TransLitRes(&firstName[1], firstSize, TRANS_IN_TABL);
		/* re-use h for regInfo */
		if((err = PtrToXHand(&firstName[1], h, firstName[0])) == noErr)
		{
			*lastName = 0;
			if((err = AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hRegLast, lastName)) == noErr)
			{
				TrimWhite(lastName);
				TrimInitialWhite(lastName);
				lastSize = lastName[0];
				lastName[++(lastName[0])] = 0;
				TransLitRes(&lastName[1], lastSize, TRANS_IN_TABL);
				if((err = PtrAndHand(&lastName[1], h, lastName[0])) == noErr)
				{
					*regCode = 0;
					if((err = AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hRegCode, regCode)) == noErr)
					{
						TrimWhite(regCode);
						TrimInitialWhite(regCode);
						regCode[++(regCode[0])] = 0;
						if ((err = PtrAndHand(&regCode[1], h, regCode[0])) == noErr)
						{
							*GlobalTemp = 0;
							if(!AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hRegNeed, GlobalTemp)) {
								TrimWhite(GlobalTemp);
								TrimInitialWhite(GlobalTemp);
								*needsRegistration = (GlobalTemp[0] && GlobalTemp[1] == 'Y');
							}
						}
					}
				}
			}
		}
	}
		
	if(!err)
	{
		(*h)[firstSize] = ' ';
		HLock(h);
		err = !RegCodeVerifies(*h + firstSize + lastSize + 2, *h, pnPolicyCode, nil);
		HUnlock(h);
		if (parseNoise == parseMaybeSilent) {
			UserStateType state = PolicyCodeToRegisteredNagState (*pnPolicyCode);
			parseNoise = parseDoDialog;
			if (StringSame (firstName, GetRegFirst (state, GlobalTemp)))
				if (StringSame (lastName, GetRegLast (state, GlobalTemp)))
					if (StringSame (regCode, GetRegCode (state, GlobalTemp)))
						parseNoise = parseSilent;
		}
		if (parseNoise == parseDoDialog) {
			(*h)[firstSize] = 0;
			if(!err) {
				variant = (char) fromFileCodeEntryVariant;
				Munger (h, 0, nil, 0, &variant, 1);
			}
			if (!err) err = CodeEntryDialog(h);
		}
		else
			if (!err) {
				(*h)[firstSize] = 0;

				switch(*pnPolicyCode) {
					case REG_EUD_AD_WARE:
						err = SettingsHandle(REG_PREF_TYPE, nil, REG_PREF_SPONSORED, h);
						break;
					case REG_EUD_LIGHT:
						err = SettingsHandle(REG_PREF_TYPE, nil, REG_PREF_LIGHT, h);
						break;
					case REG_EUD_PAID:
					case REG_EUD_50_PAID_TRIMODE:
					case REG_EUD_50_PAID_BOX_ESD:
					case REG_EUD_50_PAID_37_RSRV:
					case REG_EUD_50_PAID_38_RSRV:
					case REG_EUD_50_PAID_39_RSRV:
					case REG_EUD_50_PAID_40_RSRV:
					case REG_EUD_50_PAID_EN_ONLY:
					case REG_EUD_50_PAID_EN_NOT_X1:
						err = SettingsHandle(REG_PREF_TYPE, nil, REG_PREF_PAID, h);
						break;
				}
				MyUpdateResFile	(SettingsRefN);
				DetachResource (h);
			}
	}
	return (err);
}

typedef enum {
	ignoreProfile,
	silentlyAcceptProfile,
	silentlyDeleteProfile,
	confirmProfile
} ProfileActionType;
	
OSErr ParseProfileType (HeaderDHandle hdh)

{
	ProfileActionType	profileAction;
	Str255						profileID,
										address,
										scratch;
	OSErr							err;
	Boolean						hasProfileID,
										profileMatch,
										addressMatch,
										deleteMe;
	
	err						= noErr;
	profileAction	= ignoreProfile;
	hasProfileID	= false;
	profileMatch	= false;
	addressMatch	= false;
	deleteMe			= false;
	profileID[0]	= 0;
	address[0]		= 0;
	
	// Parse the profile ID from the headers
	(void) AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hProfile, profileID);
	TrimWhite (profileID);
	TrimInitialWhite (profileID);
	
	// Compare the profile ID to any we might already possess
	GetProfileID (scratch);
	hasProfileID = scratch[0] ? true : false;
	profileMatch = StringSame (scratch, profileID);
	
	// Parse the Mailed-To header from the headers
	(void) AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hMailedTo, address);
	TrimWhite (address);
	TrimInitialWhite (address);

	// Compare it to the personalities and 'me' nickname
	addressMatch = (MatchAnyPersonality (address) || IsMe (address));
	
	// Parse for a Delete command in the header
	(void) AAFetchResData((*hdh)->funFields, RegCodeHeadStrn+hDelete, scratch);
	deleteMe = (scratch[0] && (scratch[1] == 'y' || scratch[1] == 'Y'));

	// Okay, what are we going to do with all these booleans we've generated?
	if (addressMatch && !hasProfileID)
		profileAction = silentlyAcceptProfile; else
	if (deleteMe && addressMatch && profileMatch)
		profileAction = silentlyDeleteProfile; else
	if (addressMatch && hasProfileID)
		profileAction = confirmProfile;		// No else this time
	// One last chance to bail with no action
	if (!addressMatch || (deleteMe && hasProfileID && !profileMatch) || (!deleteMe && hasProfileID && profileMatch))
		profileAction = ignoreProfile;
	
	// Actferchristsake
	switch (profileAction) {
		case silentlyAcceptProfile:
			if (IsAdwareMode () || IsFreeMode())
				err = SetProfileID(profileID);
			break;
		case silentlyDeleteProfile:
			err = SetProfileID("");
			break;
		case confirmProfile:
			if (IsAdwareMode ()) {
				ParamText (address,"\p","\p","\p");
				Nag (PROFILE_RECEIVED_DLOG, ProfileReceivedDlogInit, ProfileReceivedDlogHit, nil, false, (long) profileID);
			}
//			if (IsAdwareMode () && ComposeStdAlert (Note, PROFILE_RECEIVED_WARNING) != 1)
//				err = SetProfileID(profileID);
			break;
	}
	return (err);
}

OSErr ProfileReceivedDlogInit (DialogPtr dlog, PStr profileID)

{
	SetDIText (dlog, PROFILE_RCVD_DITL_PROFILEID, profileID);
	return (noErr);
}

Boolean ProfileReceivedDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, StringHandle profileString)

{
	Str255	profileID;
	
	switch (itemHit) {
		case PROFILE_RCVD_DITL_REPLACE:
			GetDIText (theDialog, PROFILE_RCVD_DITL_PROFILEID, profileID);
			SetProfileID(profileID);
			CloseMyWindow (GetDialogWindow(theDialog));
			return (true);
			break;
		case PROFILE_RCVD_DITL_CANCEL:
			CloseMyWindow (GetDialogWindow(theDialog));
			return (true);
			break;
		case PROFILE_RCVD_DITL_FAQ:
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionProfileidFAQ, profileidFAQQuery, nil);
			break;
	}
	return (false);
}

Boolean MatchAnyPersonality (Str255 address)

{
	Str255	returnAddress;
	Boolean	personalityMatch;
	
	personalityMatch = false;
	PushPers (PersList);
	for (CurPers = PersList; !personalityMatch && CurPers; CurPers = (*CurPers)->next)
		personalityMatch = StringSame (address, GetReturnAddr (returnAddress, false));
	PopPers();
	return (personalityMatch);
}


void OpenRefundWindow (void)

{
	DialogPtr			theDialog;
	MyWindowPtr		theDialogWin;
	ControlHandle	refundCodeCtl;
	OSErr					theError;
	Str15					hex;
	
	theDialogWin = GetNewMyDialog (REFUND_CODE_DLOG, nil, nil, InFront);
	theError = ResError ();

	theDialog = GetMyWindowDialogPtr (theDialogWin);
	if (!theError) {
		theDialogWin->hit							= RefundDlogHit;
		theDialogWin->centerAsDefault	= true;
		
		theError = GetDialogItemAsControl (theDialog, REFUND_DITL_CODE_TEXT, &refundCodeCtl);
	}
	if (!theError) {
		SetDialogItemText (refundCodeCtl, Long2Hex (hex, (Random () << 16) | Random ()));
		
		HiliteButtonOne (theDialog);
		AutoSizeDialog (theDialog);
		ShowMyWindow (GetDialogWindow(theDialog));
	}
}


Boolean RefundDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit,long dialogRefcon)

{
	if (itemHit == REFUND_DITL_OK)
		CloseMyWindow (GetDialogWindow(theDialog));
	return (true);
}

/************************************************************************
 * PayWinIdle - idle proc for payment window
 ************************************************************************/
void PayWinIdle(MyWindowPtr win)
{
	if (gWin.inited && gWin.updateError)
	{
		WarnUser(UPDATE_FAILED,gWin.updateError);
		gWin.updateError = noErr;
	}
}