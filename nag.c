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

#include <conf.h>
#include <mydefs.h>

#ifdef NAG
#include "nag.h"
#define FILE_NUM 120

#pragma segment Nagging

#define	kUpdateNagScheduleDuringBeta		0x55555555		// Update checks every other day
#define	kUpdateNagIntervalDuringBeta		2

#define	kDelayBeforeSwitchingRepayUsersToSponsored	(60 * 60 * 60)	// 60 ticks x 60 seconds x 60 minutes
long	gTimeAtWhichRepayUsersBecomeSponsored = 0;

extern ModalFilterUPP DlgFilterUPP;
OSErr PutFeaturesInto (DialogPtr theDialog, short theItem);

#define NAG_INTRO_DLOG_OK	1
#define NAG_INTRO_INFO_BTN	3
#define NAG_INTRO_CODE_BTN	2

void CheckAdQT(void);

//
//	InitNagging
//
//		This is not a real function.  It exists right now mostly to initialize the user state
//		for our early betas.  It should be replaced by a function that erifies the user's
//		registration state.  For now, the user is a newUser if the 'NagS' resource is not
//		present.  If it is present, we automatically make them an Ad User.
//				

InitNagResultType InitNagging (void)

{
	InitNagResultType		result;
	UserStateType				newState;
	uLong								regDate;
	int									pnPolicyCode;
	int									regMonth;
	Boolean							foundRegFile,
											needsRegistration;
	
	needsRegistration	= false;
	
	// Grab the nag state information from the settings file, or create a new
	// resource if we didn't fint what we wanted
	
	nagState = GetResourceFromFile (NAG_STATE_TYPE, NAG_STATE_ID, SettingsRefN);
	if (!nagState)
		if (nagState = NuHandleClear (sizeof (NagStateRec))) {
			(*nagState)->version = NAG_STATE_VERS;
#ifdef I_HATE_THE_BOX
			(*nagState)->state	 = boxUser;
#else
			if (ValidRegCode(paidUser,&pnPolicyCode,&regMonth) && PolicyCheck (pnPolicyCode, regMonth))
				(*nagState)->state	 = paidUser;
			else
				(*nagState)->state	 = newUser;
#endif
			(*nagState)->regDate = 0;
			SettingsHandle (NAG_STATE_TYPE, nil, NAG_STATE_ID, nagState);
		}
		else
			DieWithError (MEM_ERR, MemError ());

#ifdef DEATH_BUILD
	newState = paidUser;
	result	 = noDialogPending;
#else
	// If we somehow happened upon an old EP 4 user, they will become adware users for 5.0
	if (IsEP4User ())
#ifdef I_HATE_THE_BOX
		(*nagState)->state = boxUser;
#else
		(*nagState)->state = adwareUser;
#endif
	

#ifdef I_HATE_THE_BOX
	if (!PrefIsSet (PREF_BOX_NO_LONGER_VIRGIN)) {
		(*nagState)->state = boxUser;
		SetPref(PREF_BOX_NO_LONGER_VIRGIN,"\py");
	}
#else
	// If someone ran as a box user, then switches to a non-box build, make them a paid user
	// (which we eventually reg validate)
	if ((*nagState)->state == boxUser)
		(*nagState)->state = paidUser;
#endif
#endif

	// Look for a RegCode file next to the Eudora application
	regDate = (*nagState)->regDate;
#ifndef DEATH_BUILD
	foundRegFile = CheckForRegCodeFile (&needsRegistration, &regDate, &pnPolicyCode);
	(*nagState)->regDate = regDate;
#endif

	ChangedResource ((Handle) nagState);
	MyUpdateResFile	(SettingsRefN);
	DetachResource ((Handle) nagState);
	
	// Choose the user's initial state from all we know of the world
#ifndef DEATH_BUILD
	newState = ChooseInitialUserState ((*nagState)->state, foundRegFile, needsRegistration, pnPolicyCode, &result);
#endif

	gCanPayMode = UserHasValidPaidModeRegcode();
	
	if (newState != (*nagState)->state)
		TransitionState (newState);

	CheckAdQT();
	return (result);
}

void DoPendingNagDialog (InitNagResultType pendingNagResult)

{
	switch (pendingNagResult) {
		case paymentRegPending:
			OpenPayWin ();
			break;
		case codeEntryPending:
			(void) CodeEntryForProduct (paidUser, invalidRegCodeEntryVariant);
			break;
		case repayPending:
		case gracelessRepayPending:
			RepayDialog (pendingNagResult);
			break;
	}
}


//
//	CheckNagging
//
//
//		It's time to nag if there is a nag day greater than the last nag, and
// 		less than or equal to the current day.  Nag days are stored in the 'Nag '
// 		resource as a bit mask corresponding to days 0 thru 31 of the nag
// 		schedule.  We can accomplish this by a strange and non-obvious formula
// 		(yet somehow occurred to me one evening while watching Stephen King's
//		"Storm Of The Century"):
//
//				[((1 << (currentDay + 1)) - 1) - ((1 << (lastDay + 1)) - 1)] & schedule
//
// 				(Note that lastDay MUST be <= currentDay...)
//
//		Here's how it works:
//
//				Since each day since the nagBase is represented by a single bit, we
//				can left shift a normalized day to represent that particular day.
//				Likewise, subtracting 1 from a single shifted bit turns on all bits
//				less than the set bit.  Subtracting one such one-subtracted bit value
//				from a second will result in setting all bits between one bit and another.
//
// 		For example:
//
//				NagBase  = 0 (always)
//				lastDay  = 6
//		    Current  = 11
//	  		Schedule = [0, 4, 9, 12, 3]    Mask is  0000 0000 0000 0000 0001 0010 0001 0001
//
//				Using the formula above...
//
//						currentDay + 1						:  12
//						left shift 1 by 12 bits		:  0000 0000 0000 0000 0001 0000 0000 0000
//			  		subtract 1								:  0000 0000 0000 0000 0000 1111 1111 1111   1st value
//
//						lastDay + 1							  :  7
//						left shift 1 by 7 bits    :  0000 0000 0000 0000 0000 0000 1000 0000
//						subtract 1								:	 0000 0000 0000 0000 0000 0000 0111 1111   2nd value
//
//						subtract the 2nd from 1st :  0000 0000 0000 0000 0000 1111 1000 0000
//
//						Which means that we'll nag if there is a nag scheduled for any of the
//						days set in the results, so... let's AND it with the schedule.
//
//																				 0000 0000 0000 0000 0000 1111 1000 0000
//																		AND  0000 0000 0000 0000 0001 0010 0001 0001
//																		--------------------------------------------
//																				 0000 0000 0000 0000 0000 0010 0000 0000  NAG!!!
//
//		Of course, the above only works during the days covered by the nag schedule itself.
//		Once we go past this date we have to look at the nag interval to make the determination
//		of whether or no we will nag.
//

void CheckNagging (UserStateType state)

{
	NagRec								nag;
	NagUsageHandle				nagUsage;
	NagDialogInitProcPtr	initProc;
	NagDialogHitProcPtr		hitProc;
	ModalFilterUPP				theFilterProc;
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
	AdFailureRec					adFailure;
#endif
	OSErr									theError;
	long									refcon;
	uLong									nagBaseSeconds,			// In seconds
												lastNagDay,					// In days
												currentDay,					// In days
												lastScheduledDay,		// In days
												nextNagDay,					// In days
												currentTime,				// In seconds
												currentDayMask,
												lastDayMask;
	short									dialogID,
												nagID,
												index;
	Boolean								nagMe,
												saveMe,
												ignoreDefaultItem;

	nagMe		 = false;
	saveMe	 = false;
	index		 = 1;
	theError = noErr;
	
	// Check to see if it's time to change repay users to sponsored mode
	if (IsRepayUser () && gTimeAtWhichRepayUsersBecomeSponsored)
		if (TickCount () >= gTimeAtWhichRepayUsersBecomeSponsored) {
			TransitionState (RegisteredUser (paidUser) ? regAdwareUser : adwareUser);
			gTimeAtWhichRepayUsersBecomeSponsored = 0;
			return;
		}
	
	// Check each of the possible nag scedules for this state -- until we find it's time to nag
	while (!theError && !GetIndNagState (&nag, &nagID, state, index++)) {
		// Get the nag usage record for the particular nag dialog we're thinking about (as saved in settings)
		nagUsage = GetResourceFromFile (NAG_USAGE_TYPE, nagID, SettingsRefN);
			
		// If the nag usage record is empty, create one and store it in settings
		currentTime = LocalDateTime ();
		if (!nagUsage)
			if (nagUsage = NuHandleClear (sizeof (NagUsageRec))) {
				(*nagUsage)->version = NAG_USAGE_VERS;
				(*nagUsage)->nagBase = currentTime;
				(*nagUsage)->lastNag = 0;
				(*nagUsage)->aux		 = 0;
				if (!SettingsHandle (NAG_USAGE_TYPE, nil, nagID, nagUsage))
					MyUpdateResFile	(SettingsRefN);
				else
					ZapHandle (nagUsage);
			}
#ifdef BETA_UPDATE_SCHEDULE
		if (nag.flags & nagFlagSilentUpdate) {
			nag.schedule = kUpdateNagScheduleDuringBeta;
			nag.interval = kUpdateNagIntervalDuringBeta;
		}
#endif

		if (nagUsage) {
			// Normalize the dates for the nag base, last nag and the current
			// day in order to calculate the days-since-nagBase
			nagBaseSeconds = SecondsToWholeDay ((*nagUsage)->nagBase);
			lastNagDay		 = (*nagUsage)->lastNag ? (SecondsToWholeDay ((*nagUsage)->lastNag) - nagBaseSeconds) / kSecondsPerDay : 0;
			currentDay		 = (SecondsToWholeDay (currentTime) - nagBaseSeconds) / kSecondsPerDay;

			// Figure out the last scheduled day (it's the most significant set bit of the schedule)
			lastScheduledDay = MostSignificantSetBit (nag.schedule);
			
			// The dialog that we _might_ nag with
			dialogID = nag.dialogID;
			
			// If the last nag day falls within the schedule, we can use our spiffy formula,
			// otherwise we'll have to more laboriously test to see if the nag day falls into
			// the correct range using the nag interval
			// (jp) 2-5-00  Previously, we were checking to see if the last day we nagged fell within the
			//							range of the nag schedule.  For example, in a schedule like 7,14,3 we'd be checking
			//							to see if the last day we nagged was less than or equal to 14 -- the last day in the
			//							schedule before the nag interval kicked in.  This was wrong, however, and caused
			//							us to never again check in the current day was past the final day of the range (as was
			//							the case for the ad failure check).  The correct algorithm is to check the current
			//							day against the range, use the bit mask if we're still running within the range, otherwise
			//							we take into account the nag interval.
//			if (lastNagDay <= lastScheduledDay) {
			if (currentDay <= lastScheduledDay) {
				currentDayMask = (1 << (currentDay + 1)) - 1;
				lastDayMask		 = (*nagUsage)->lastNag ? (1 << (lastNagDay + 1)) - 1 : 0;
				nagMe = (currentDayMask - lastDayMask) & nag.schedule ? true : false;
			}
			else {
				nextNagDay = lastScheduledDay + nag.interval * ((lastNagDay - lastScheduledDay) / nag.interval) + nag.interval;
				nagMe = nextNagDay > lastNagDay && nextNagDay <= currentDay;
			}
			// If it is time to nag the user in whichever manner is appropriate -- this might be a dialog, or
			// it might be a request to perform some operation.  We should also update the 'NagU' resource.
			if (nagMe)
				if (nag.flags & nagFlagSilentUpdate && !PrefIsSet (PREF_DISABLE_AUTO_UPDATE_CHECK) && !Offline) {
#ifndef DO_NOT_UPDATE_CHECK_IN_DEATH_BUILDS
					theError = UpdateCheck (true, false);
					(*nagUsage)->lastNag = currentTime;
					saveMe = true;
#endif
					nagMe  = false;
				} else
				if (nag.flags & nagFlagUpdate && !PrefIsSet (PREF_DISABLE_AUTO_UPDATE_CHECK) && !Offline) {
#ifndef DO_NOT_UPDATE_CHECK_IN_DEATH_BUILDS
					theError = UpdateCheck (false, false);
					(*nagUsage)->lastNag = currentTime;
					saveMe = true;
#endif
					nagMe	 = false;
				}
				else
				if (nag.flags & nagFlagAdFailureCheck) {
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
					// Check for ad failures and deadbeatness
					LDRef (nagUsage);
					adFailure.error = AdFailureCheck (nagUsage, currentTime, &nagMe, &dialogID, adFailure.text);
					UL (nagUsage);
					(*nagUsage)->lastNag = currentTime;
					saveMe = true;
#else
					nagMe = false;
#endif
				}

#ifdef DEATH_BUILD
			if (nagMe && dialogID == NAG_PLEASE_REGISTER_DLOG)
				nagMe = false;
#endif

			// If we're still nagging (no error and a flag didn't handle things), nag away!
			if (!theError && nagMe) {
				initProc			= nil;
				hitProc				= nil;
				theFilterProc	= nil;
				refcon				= 0;
				ignoreDefaultItem	= false;
				switch (dialogID) {
					case NAG_INTRO_DLOG:
						hitProc = IntroDlogHit;
						theFilterProc = DlgFilterUPP;
						break;
					case NAG_PLEASE_REGISTER_DLOG:
						hitProc = AdUserHit;
						break;
					case NAG_DOWNGRADE_DLOG:
						initProc = DowngradeDlogInit;
						hitProc	 = DowngradeDlogHit;
						break;
					case NAG_FEATURES_DLOG:
						initProc = FeaturesDlogInit;
						hitProc	 = FeaturesDlogHit;
						break;
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
					case NAG_NOT_GETTING_ADS_DLOG:
						initProc = NotGettingAdsDlogInit;
						hitProc	 = NotGettingAdsDlogHit;
						refcon	 = (long) &adFailure;
						break;
					case NAG_FRIGGING_HACKER_DLOG:
						hitProc						= NoAdsRevertToFreeDlogHit;
						theFilterProc			= DlgFilterUPP;
						ignoreDefaultItem	= true;
						break;
#endif
				}
				theError = Nag (dialogID, initProc, hitProc, theFilterProc, ignoreDefaultItem, refcon);
				if (!theError) {
					(*nagUsage)->lastNag = currentTime;
					saveMe = true;
				}
			}
			if (!theError || saveMe) {
				ChangedResource ((Handle) nagUsage);
				if (!ResError ()) 
					MyUpdateResFile	(SettingsRefN);
			}
			ReleaseResource (nagUsage);
		}
	}
}




//
//	Nag
//
//		Displays a Nag dialog to the user.  The parameters look like so:
//
//			dialogID					-		Resource ID of the 'DLOG' to be displayed
//			initProc					-		NagDialogInitProcPtr callback issued after the dialog is successfully
//														created.  This allows you to perform any dialog special initialization
//														before modeless dialog handling kicks in (for instance, so we can setup
//														the feature list in the downgrade dialog).
//			hitProc						-		NagDialogHitProcPtr that is called whenever an item in the dialog is hit.
//														The dialog hit proc accepts a pointer to an event, a dialog pointer, and
//														the item that was hit.
//			theFilterProc			-		An optional modal filter proc which -- if present -- turns this nag into
//														a movable modal dialog.  Pass in nil if you want the Nag to behave in a
//														modeless manner.
//			ignoreDefaultItem	-		'true' if you don't want item 1 to be a default item
//
//		Quick nag dialog note...  Whenever a new nag dialog is added to the resource fork
//		don't forget:
//
//				1. Add the DLOG resID to the 'hlist' in AddDlgx
//				2. Add a 'dftb' resource with the same id to Common.r
//
//		(There, it helps to write things down he said after spending an hour trying to figure
//		out why GetDialogItemAsControl had broken and all of the dialog text was in the system font).
//
//		Sort of, kind of important note.... Do not define a Nag with a dialog ID of zero!!
//
//stuvoid DebugDialogItems (DialogPtr theDialog);

OSErr Nag (short dialogID, NagDialogInitProcPtr initProc, NagDialogHitProcPtr hitProc, ModalFilterUPP theFilterProc, Boolean ignoreDefaultItem, long dialogRefcon)

{
	MyWindowPtr	dlogWin,
							win;
	DialogPtr		dlog;
	WindowPtr		theWindow;
	OSErr				theError;
	short				dItem;
	Boolean			done;

	if (!dialogID)
		return (noErr);
		
	// Is this nag already open?
	if (win = FindNag (dialogID)) {
		UserSelectWindow (GetMyWindowWindowPtr(win));
		return (noErr);
	}

	dlogWin = GetNewMyDialog (dialogID, nil, nil, InFront);
	theError = ResError ();

	if (dlogWin) {
		dlog = GetMyWindowDialogPtr (dlogWin);
		theWindow = GetDialogWindow(dlog);
		dlogWin->hit							 = hitProc;
		dlogWin->hideme						 = true;
		dlogWin->ignoreDefaultItem = ignoreDefaultItem;
		dlogWin->centerAsDefault	 = true;
		dlogWin->isNag						 = true;
		dlogWin->dialogRefcon			 = dialogRefcon;

		if (initProc)
			theError = (*initProc) (dlog, dialogRefcon);
		
		if (!theError) {
			HiliteButtonOne (dlog);
			theError = SubstituteLongStaticTextItems (dlog);
			if (AppearanceVersion() < 0x0110)
				;	// Eventually we need to write our own AutoSizeDialog hack for Appearance 1.0.3
			else {
				AutoSizeDialog(dlog);
				GrowGroupItems(theWindow,nil);
			}
//DebugDialogItems (theDialog);
		}
		
		if (!theError) {
			if (theFilterProc) {
				StartMovableModal (dlog);
				ShowWindow (theWindow);
				AuditWindowOpen(dlogWin->windex,dialogID,0);
				done = false;
				while (!done) {
					MovableModalDialog (dlog, theFilterProc, &dItem);
					if (dItem == CANCEL_ITEM)		// For some reason, CANCEL_ITEM is -1� huh?
						done = true;
					else
						if (hitProc)
							done = (*hitProc) (nil, dlog, dItem, dialogRefcon);
				}
				EndMovableModal (dlog);
				CloseMyWindow (theWindow);
				AuditWindowClose(dlogWin->windex);
			}
			else
				ShowMyWindow (theWindow);
		}
		else
			DisposeDialog_ (dlog);
	}
	return (theError);
}	


MyWindowPtr FindNag (short dialogID)

{
	WindowPtr	winWP;
	MyWindowPtr	win;
		
	for (winWP = GetWindowList (); winWP; winWP = GetNextWindow (winWP)) {
		win = GetWindowMyWindowPtr(winWP);
		if (IsKnownWindowMyWindow(winWP) && win->dialogID == dialogID)
			return (win);
	}
	return (nil);
}




void DownGradeDialog (void)

{
	(void) Nag (NAG_DOWNGRADE_DLOG, DowngradeDlogInit, DowngradeDlogHit, nil, true, nil);
}

void NotifyDownGradeDialog (void)

{
	FeatureCellHandle	newFeatureList;
	GrafPtr						oldPort;
	ControlHandle			theControl;
	ListHandle				theList;
	MyWindowPtr				theDialogWin;
	OSErr							theError;
	Rect							contrlRect;
	Size							actualSize;

	if (theDialogWin = FindNag (NAG_DOWNGRADE_DLOG)) {
		DialogPtr	theDialog = GetMyWindowDialogPtr (theDialogWin);
		theError = GetDialogItemAsControl (theDialog, NAG_DOWNGRADE_FEATURE_LIST, &theControl);
		if (!theError)
			theError = GetControlData (theControl, kControlEntireControl, kControlListBoxListHandleTag, sizeof (theList), (Ptr) &theList, &actualSize);
		if (!theError && theList)
			if (newFeatureList = BuildFeatureList (false)) {
				if (memcmp (LDRef (newFeatureList), LDRef((*theList)->userHandle), GetHandleSize (newFeatureList))) {
					GetControlBounds(theControl,&contrlRect);
					GetPort (&oldPort);
					SetPort (GetWindowPort(GetDialogWindow(theDialog)));
					InvalWindowRect(GetDialogWindow(theDialog),&contrlRect);
					SetPort (oldPort);
				}
				UL (newFeatureList);
				ZapHandle ((*theList)->userHandle);
				(*theList)->userHandle = newFeatureList;
			}
	}
}


Boolean RepayHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon);
Boolean JunkDownHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon);
extern Boolean PreRegHitProc (EventRecord *event, DialogPtr theDialog, short itemHit, long refcon);

void RepayDialog (InitNagResultType pendingNagResult)

{
	UserStateType	newState;
	short					itemHit;
	
	if (!Nag (REPAY_DLOG, nil, RepayHitProc, DlgFilterUPP, true, (long) &itemHit)) {
		switch (itemHit) {
			case REPAY_DITL_PAY_NOW:
				if (pendingNagResult != gracelessRepayPending)
					ComposeStdAlert (kAlertNoteAlert, GRACE_PERIOD_PAY_NOW_ALRT);
				OpenAdwareURL (paidUser, REG_SITE, actionPay, paymentQuery, nil);
				// If the user is trying to pay, we'll temporarily make them a 'repay' user.  This effectively
				// leaves them in paid mode for some length of time (currently an hour), or until they restart
				// Eudora.
				newState = repayUser;
				break;
			case REPAY_DITL_SPONSORED:
				// Profile deadbeat users are subject to the same profile requirements
				// as if they clicked 'Sponsored' in Payment & Registration
				if (IsProfileDeadbeatUser()) {
					// User must profile
					if (SendUserToProfile())
						if (kAlertStdAlertOKButton==ComposeStdAlert(Note,-PROFILING_NOW))
							// user has indicated he has profiled.  Make a playlist request
							ForcePlaylistRequest();
					return;
				}
				else
					newState = IsRegisteredUser () ? regAdwareUser : adwareUser;
				break;
		}
	}
	// If no grace time is allowed, shoo them immediately into sponsored mode
	if (pendingNagResult == gracelessRepayPending)
		newState = IsRegisteredUser () ? regAdwareUser : adwareUser;
	TransitionState (newState);
}

void JunkDownDialog (void)
{
	short					itemHit;
	
	SetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefDeadPluginsWarning);
	if (!Nag (JUNKDOWN_DLOG, nil, JunkDownHitProc, DlgFilterUPP, true, (long) &itemHit)) {
		switch (itemHit) {
			case JUNKDOWN_DITL_PAY_NOW:
				OpenAdwareURL (paidUser, REG_SITE, actionPay, paymentQuery, nil);
				break;
		}
	}
}

Boolean RepayHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon)
{
	short	*dItem;
	
	dItem = (short *) refcon;
	*dItem = itemHit;
	if (itemHit==REPAY_DITL_SHOW_VERSIONS)
	{
		OpenAdwareURL (paidUser, UPDATE_SITE, actionArchived, updateQuery, nil);
		return false;
	}
	return (true);
}

Boolean JunkDownHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon)

{
	short	*dItem;
	
	dItem = (short *) refcon;
	*dItem = itemHit;
	if (itemHit==JUNKDOWN_DITL_MORE)
	{
    OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionSupport, junkDownQuery, topicJunkDown);
		return false;
	}
	return (true);
}

Boolean AdUserHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)

{
	short	action;
	
	switch (itemHit) {
		case NAG_PLEASE_REGISTER_REGISTER_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
#ifdef I_HATE_THE_BOX
			action = IsAdwareMode () ? actionRegisterAd : (IsFreeMode () ? actionRegisterFree : actionRegister50box);
#else
			action = IsAdwareMode () ? actionRegisterAd : actionRegisterFree;
#endif
			OpenAdwareURL (GetNagState (), REG_SITE, action, registrationQuery, nil);
			break;
		case NAG_PLEASE_REGISTER_LATER_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			break;
	}
	return (true);
}

Boolean IntroDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)
{
        // Transition to Adware no matter what
        TransitionState (adwareUser);
        switch (itemHit) {
                case NAG_INTRO_DLOG_OK:
												CloseMyWindow (GetDialogWindow(theDialog));
                        break;
                case NAG_INTRO_INFO_BTN:
                				CloseMyWindow (GetDialogWindow(theDialog));
                        OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionIntro, introQuery, nil);
                        break;
                case NAG_INTRO_CODE_BTN:
                				CloseMyWindow(GetDialogWindow(theDialog));
                        CodeEntryDialog(nil);
                        break;
        }                
        return (true);
}


// Vacuuous hit proc
Boolean HitMeHitMeHitMe (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)
{
	return (true);
}



OSErr DowngradeDlogInit (DialogPtr theDialog, long refcon)

{
	ControlHandle			theControl;
	ListHandle				theList;
	OSErr							theError;
	Size							actualSize;
	
	theError = GetDialogItemAsControl (theDialog, NAG_DOWNGRADE_FEATURE_LIST, &theControl);
	if (!theError)
		theError = GetControlData (theControl, kControlEntireControl, kControlListBoxListHandleTag, sizeof (theList), (Ptr) &theList, &actualSize);
	if (!theError)
		if ((*theList)->userHandle = BuildFeatureList (false)) {
			LSetDrawingMode (false, theList);
			LAddRow (GetHandleSize ((*theList)->userHandle) / sizeof (FeatureCellRec), 0, theList);
			LSetDrawingMode (true, theList);
		}
		
	return (theError);
}


FeatureCellHandle BuildFeatureList (Boolean ignoreUsage)

{
	FeatureRecPtr			featurePtr,
										subFeaturePtr;
	FeatureCellHandle	featureCells;
	FeatureCellPtr		featureCellsPtr,
										primaryCellPtr;
	OSErr							theError;
	short							numFeatures,
										subCount,
										featureCount;
	FeatureID					id;
	
	featureCells = nil;
	
	// Figure out how many feature we actually need to display in the list
	numFeatures	 = GetHandleSize (gFeatureList) / sizeof (FeatureRec);
	featureCount = 0;
	featurePtr	 = LDRef (gFeatureList);
	for (id = 0; id < numFeatures; ++id) {
		if (IsFreeMode ()) {
			if (featurePtr->resID != featNoResource && featurePtr->resID != featSimultaneousDirService)
				++featureCount;
		}
		else
			if (featurePtr->resID != featNoResource && HasFeature (id))
				++featureCount;
		++featurePtr;
	}
			
	if (featureCount) {
		// Allocate space for the list data and store it in the useHandle (it will get released by the LDEF)
		featureCells = NuHandleClear (featureCount * sizeof (FeatureCellRec) * 2);
		theError = MemError ();
		if (!theError) {
			// Build the list data
			featureCount		= 0;
			featurePtr			= *gFeatureList;
			featureCellsPtr	= LDRef (featureCells);
			for (id = 0; !theError && id < numFeatures; ++id) {
				if ((IsFreeMode () && featurePtr->resID != featNoResource && featurePtr->resID != featSimultaneousDirService) ||
						(!IsFreeMode () && featurePtr->resID != featNoResource && HasFeature (id))) {
					// If the feature is NOT a sub feature, we'll add and process it
					if (featurePtr->sub == '????') {
						primaryCellPtr = featureCellsPtr;
						theError = SetFeatureCell (&featureCellsPtr, featurePtr, ignoreUsage);
						++featureCellsPtr;
						++featureCount;
						// Now, if this feature has sub features, look for them!
						if (featurePtr->hasSubFeatures) {
							subFeaturePtr = *gFeatureList;
							subCount			= numFeatures;
							while (!theError && subCount--) {
								if (subFeaturePtr->primary == featurePtr->primary && subFeaturePtr->sub != '????') {
									if (featureCellsPtr->used = ((subFeaturePtr->flags & featurePresent) && subFeaturePtr->lastUsed && !ignoreUsage))
										primaryCellPtr->used = true;
									theError = SetFeatureCell (&featureCellsPtr, subFeaturePtr, ignoreUsage);
									++featureCellsPtr;
									++featureCount;
								}
								++subFeaturePtr;
							}
						}
					}
				}
				++featurePtr;
			}
			UL (featureCells);
			if (!theError)
				SetHandleBig (featureCells, featureCount * sizeof (FeatureCellRec) * 2);
		}
	}
	UL (gFeatureList);
	if (theError && featureCells)
		ZapHandle (featureCells);
	return (featureCells);
}


//
//	SetFeatureCell
//
//		Set a particular feature cell to reflect a given feature
//

OSErr SetFeatureCell (FeatureCellPtr *featureCellsPtr, FeatureRecPtr featurePtr, Boolean ignoreUsage)

{
	FeatureResHandle	resFeature;
	char							*nameAndDesc;
	
	if (resFeature = GetResource (featureResourceType, (featurePtr)->resID)) {
		nameAndDesc = (char *) &(*resFeature)->nameAndDesc;
		// First, the name...
		BlockMoveData (&nameAndDesc[0], (*featureCellsPtr)->description, nameAndDesc[0] + 1);
		(*featureCellsPtr)->type = featurePtr->primary;
		(*featureCellsPtr)->isName = true;
		(*featureCellsPtr)->isSubFeature = (featurePtr->sub != '????');
		(*featureCellsPtr)->used = ((featurePtr->flags & featurePresent) && featurePtr->lastUsed && !ignoreUsage);
		
		// Next, the description...
		++(*featureCellsPtr);
		(*featureCellsPtr)->isSubFeature = (featurePtr->sub != '????');
		BlockMoveData (&nameAndDesc[nameAndDesc[0] + 1], (*featureCellsPtr)->description, nameAndDesc[nameAndDesc[0] + 1] + 1);
		ReleaseResource (resFeature);
	}
	return (ResError ());
}

Boolean DowngradeDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)

{
	switch (itemHit) {
		case NAG_DOWNGRADE_YES_I_MEAN_IT_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			TransitionState (ValidRegCode (regFreeUser, nil, nil) ? regFreeUser : freeUser);
			break;
		case NAG_DOWNGRADE_CANCEL_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			break;
	}
	return (true);
}

OSErr FeaturesDlogInit (DialogPtr theDialog, long refcon)
{
	return PutFeaturesInto(theDialog,NAG_FEATURES_FEATURE_LIST);
}

OSErr PutFeaturesInto (DialogPtr theDialog, short theItem)
{
	ControlHandle			theControl;
	ListHandle				theList;
	OSErr							theError;
	Size							actualSize;
	
	theError = GetDialogItemAsControl (theDialog, theItem, &theControl);
	if (!theError)
		theError = GetControlData (theControl, kControlEntireControl, kControlListBoxListHandleTag, sizeof (theList), (Ptr) &theList, &actualSize);
	if (!theError)
		if ((*theList)->userHandle = BuildFeatureList (true)) {
			LSetDrawingMode (false, theList);
			LAddRow (GetHandleSize ((*theList)->userHandle) / sizeof (FeatureCellRec), 0, theList);
			LSetDrawingMode (true, theList);
		}
		
	return (theError);
}


Boolean FeaturesDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)

{
	switch (itemHit) {
		case NAG_FEATURES_YES_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			TransitionState (adwareUser);
			break;
		case NAG_FEATURES_CANCEL_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			break;
	}
	return (true);
}


OSErr NotGettingAdsDlogInit (DialogPtr dlog, AdFailurePtr adFailurePtr)

{
	SetDIText (dlog, NAG_NOT_GETTING_ADS_ERROR_TEXT, adFailurePtr->text);
	return (noErr);
}


Boolean NotGettingAdsDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)

{
	switch (itemHit) {
		case NAG_NOT_GETTING_ADS_MORE_INFO_BTN:
			CloseMyWindow (GetDialogWindow(theDialog));
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionSupport, helpQuery, topicAdFailure);
			break;
	}
	return (true);
}


Boolean NoAdsRevertToFreeDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon)
{
	switch (itemHit) {
		case NAG_NO_ADS_AT_ALL_OK_BTN:
			TransitionState (deadbeatUser);
			return (true);
			break;
		case NAG_NO_ADS_AT_ALL_MORE_INFO_BTN:
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionSupport, helpQuery, topicAdFailure);
			return (false);
			break;
	}
	return (true);
}

/*
void DebugDialogItems (DialogPtr theDialog)

{
	ControlHandle	theControl;
	Str255				string;
	Rect					itemRect;
	short					count,
								itemNo,
								itemType;
	ComposeString (string,"\p[%d, %d, %d, %d]", theDialog->portRect.left, theDialog->portRect.top, theDialog->portRect.right, theDialog->portRect.bottom);
	DebugStr (string);
	count = CountDITL (theDialog);
	for (itemNo = 1; itemNo <= count; ++itemNo) {
		GetDialogItemAsControl (theDialog, itemNo, &theControl);
//		GetDialogItem (theDialog, itemNo, &itemType, (ControlHandle *) &theControl, &itemRect);
		ComposeString (string,"\p[%d, %d, %d, %d]", (*theControl)->contrlRect.left, (*theControl)->contrlRect.top, (*theControl)->contrlRect.right, (*theControl)->contrlRect.bottom);
		DebugStr (string);
	}
}
*/

//
//	SubstituteLongStaticTextItems
//

OSErr SubstituteLongStaticTextItems (DialogPtr theDialog)

{
	ControlHandle	theControl;
	Str255				text;
	Handle				textHandle;
	OSErr					theError;
	Rect					itemRect;
	long					resID;
	short					count,
								itemNo,
								itemType;
	
	theError = noErr;
	
	count = CountDITL (theDialog);
	for (itemNo = 1; !theError && itemNo <= count; ++itemNo) {
		GetDialogItem (theDialog, itemNo, &itemType, (ControlHandle *) &theControl, &itemRect);
		if (itemType & kStaticTextDialogItem) {
			GetDialogItemText (theControl, text);
			if (text[0] && text[1] == kTEXTResIDFollowsChar) {
				text[1] = text[0] - 1;
				StringToNum (&text[1], &resID);
				if (textHandle = GetResource ('TEXT', resID)) {
					theError = GetDialogItemAsControl (theDialog, itemNo, &theControl);
					if (!theError)
						theError = SetControlData (theControl, kControlEntireControl, kControlStaticTextTextTag, GetHandleSize (textHandle), LDRef (textHandle));
					UL (textHandle);
					ReleaseResource (textHandle);
				}
			}
		}
	}
	return (theError);
}

/************************************************************************
 * GrowGroupItems - grow the group boxes in a window to contain their subcontrols
 ************************************************************************/
short GrowGroupItems(WindowPtr win,ControlHandle parentCntl)
{
	ControlHandle cntl;
	short maxV = 0, maxSubV;
	short subI;
	Rect	rParent;

	// Start off with the root control
	if (!parentCntl)
	if (GetRootControl(win,&parentCntl) || !parentCntl)
		return 0;	// no embedding
	
	// First, figure out the size of the subcontrols
	for (CountSubControls(parentCntl,&subI);subI;subI--)
		if (!GetIndexedSubControl(parentCntl,subI,&cntl) && IsControlVisible(cntl))
		{
			maxSubV = GrowGroupItems(win,cntl);
			maxV = MAX(maxV,maxSubV);
		}
	
	// Do we need to change anything?
	GetControlBounds(parentCntl,&rParent);
	if (maxV+MAX_APPEAR_RIM > rParent.bottom)
	if (IsGroupControl(parentCntl))
	{
		SizeControl(parentCntl,ControlWi(parentCntl),maxV+MAX_APPEAR_RIM-rParent.top);
	}
	
	// Return final size
	GetControlBounds(parentCntl,&rParent);
	maxV = rParent.bottom;	

	return(maxV);
}

/************************************************************************
 * IsGroupControl - is a control a group control?
 ************************************************************************/
Boolean IsGroupControl(ControlHandle cntl)
{
	return GetControlReference(cntl)=='gpbx';
}

//
//	GetIndNagID
//
//		Grab an indexed nagID from Eudora itself.
//

OSErr GetIndNagID (short *nagID, short nagListID, short index)

{
	NagListHandle	nagList;
	OSErr					theError;

	theError = resNotFound;
#ifdef I_HATE_THE_BOX
	if (nagList = GetResource (NAG_LIST_TYPE, nagListID)) {
#else
	if (nagList = GetResourceFromFile (NAG_LIST_TYPE, nagListID, AppResFile)) {
#endif
		if (index > 0 && index <= (*nagList)->count) {
			*nagID = (*nagList)->nagID[index - 1];
			theError = noErr;
		}
		ReleaseResource (nagList);
	}
	return (theError);
}

//
//	GetIndNagState
//
//		Grab an indexed 'Nag ' resource from Eudora itself, depending on
//		a particular user state.
//

OSErr GetIndNagState (NagPtr theNag, short *nagID, UserStateType state, short index)

{
	NagHandle	nag;
	OSErr			theError;
	
	// First, get the indexed nag ID for the current state
	theError = GetIndNagID (nagID, (short) state, index);
	if (!theError) {
		theError = resNotFound;
#ifdef I_HATE_THE_BOX
		if (nag = GetResource (NAG_TYPE, *nagID)) {
#else
		if (nag = GetResourceFromFile (NAG_TYPE, *nagID, AppResFile)) {
#endif
			BlockMoveData (*nag, theNag, sizeof (NagRec));
			theError = noErr;
		}
		ReleaseResource (nag);
	}
	return (theError);
}



//
//	TransitionState
//

void TransitionState (UserStateType newState)

{
	WindowPtr		winWP;
	MyWindowPtr	win;
	UserStateType	oldState;
	short oldJunkCount = ETLCountTranslators(EMSF_JUNK_MAIL);
	
	if (ValidUser (newState)) {
		oldState = (*nagState)->state;
		(*nagState)->state = newState;
		if (!SettingsHandle (NAG_STATE_TYPE, nil, NAG_STATE_ID, nagState)) {
			MyUpdateResFile	(SettingsRefN);
			DetachResource ((Handle) nagState);
		}
		
		CheckAdQT();
		
		// Anything special we need to do when transitioning to a new state?
		if (RepayUser (newState))
			gTimeAtWhichRepayUsersBecomeSponsored = TickCount () + kDelayBeforeSwitchingRepayUsersToSponsored;
			
		if ((FreeUser (oldState) && !FreeUser (newState)) ||
				(!FreeUser (oldState) && FreeUser (newState))) {
			// We need to defer making the state switch until we have torn down the
			// current environment (in particular, closing windows have to know we
			// are closing in Light as opposed to Sponsored mode).  The state switch
			// itself will occur in OpenNewSettings
			(*nagState)->state = oldState;
			RelaunchEudora (newState);
			
			// Notify any plugins that care (Peanut) of our state change.
			ETLEudoraModeNotification(EMS_ModeChanged, GetCurrentPayMode ());
		}
		else {
			WindowPtr	nextWP;
			// 'OpenAdWindow' actually _does_ need to be called here regardless of
			// mode so that it can perform various ad switching stuff itself.
			// if (AdwareUser (newState))
			OpenAdWindow ();
			winWP = GetWindowList ();
			while (winWP) {
				nextWP = GetNextWindow (winWP);				// Just in case we close 'winWP' (like we do w/the ad window)
				win = GetWindowMyWindowPtr (winWP);
				if (IsKnownWindowMyWindow (winWP) && win->transition)
					(*win->transition) (win, oldState, newState);
				winWP = nextWP;
			}
			// Notify any plugins that care (Peanut, SpamWatch suite) of our state change.
			ETLEudoraModeNotification(EMS_ModeChanged, GetCurrentPayMode ());
		}
		
		// plug-ins may have become active
		if (JunkPrefNeedIntro()) JunkIntro();
		
		// on the other hand, they may not...
		if (oldJunkCount>ETLCountTranslators(EMSF_JUNK_MAIL)) JunkDownDialog();			
	}
	gCanPayMode = UserHasValidPaidModeRegcode();
}

/************************************************************************
 * UserHasValidPaidModeRegcode - does the user have a valid paid-mode code?
 ************************************************************************/
Boolean UserHasValidPaidModeRegcode(void)
{
	int policy, month;

	return ValidRegCode(paidUser,&policy,&month) && PolicyCheck(policy,month);
}

//
//	DaysSinceNagBase
//
//		Returns the number of days since the nag base for the specified nag dialog.
//		If the nag base is zero (indicating that we've never been nagged), 
//

uLong DaysSinceNagBase (short dialogID)

{
	NagUsageHandle	nagUsage;
	uLong						daysSince;
									
	daysSince = 0;
	nagUsage = GetResourceFromFile (NAG_USAGE_TYPE, dialogID, SettingsRefN);
	if (nagUsage)
		daysSince = (SecondsToWholeDay (LocalDateTime ()) - SecondsToWholeDay ((*nagUsage)->nagBase)) / kSecondsPerDay;
	return (daysSince);
}


uLong DurationOfSchedule (UserStateType state, short whichSchedule)

{
	NagRec	nag;
	uLong		duration;
	short		lastScheduledDay,
					nagID;

	duration = 0;
	if (!GetIndNagState (&nag, &nagID, state, whichSchedule))
		if ((lastScheduledDay = MostSignificantSetBit (nag.schedule)) >= 0)
			duration = lastScheduledDay + 1;
	return (duration);
}


//
//	SecondsToWholeDay
//
//		Takes a time expressed in seconds since 1/1/04 and returns the
//		time expressed as midnight on that day since 1/1/04 (if that
//		makes any sense whatsoever...).  The purpose of this routine is
//		normalize internal times to more easily perform date calculation
//		in absolute day increments without rounding problems.  For example,
//		the number of days between 11:59 PM on 3/3/99, and 12:00 AM on
//		3/4/99 would be 1.  This is much easier to accomplish across day,
//		month and year boundaries if we're always dealing with days timed
//		at midnight.
//

uLong SecondsToWholeDay (uLong secs)

{
	DateTimeRec	dateTime;

	SecondsToDate (secs, &dateTime);
	dateTime.hour		= 0;
	dateTime.minute	= 0;
	dateTime.second	= 0;
	DateToSeconds (&dateTime, &secs);
	return (secs);
}


//
//	MostSignificantSetBit
//
//		Find the most significant set bit of an unsigned 32 bit value
//		using an inefficient algorithm but oh well you've been warned
//		and you're not going to call this 10 million times in a tight
//		loop everytime through the main event loop anyway, right?
//
//		Returns -1 if no bits are set.
//

short MostSignificantSetBit (UInt32 value)

{
	short	bit;
	
	bit = 31;
	while (value)
		if (value & 0x80000000)
			return (bit);
		else {
			--bit;
			value <<= 1;
		}
	return (-1);
}




void RelaunchEudora (UserStateType newState)

{
	OpenNewSettings (&SettingsFileSpec, true, newState);
}


//
//	UpdateCheck
//
//		Contact eudora.com and check for application updates.
//		If there are updates write the results into a file
//		Open a window displaying this information.
//
//		silently - Indicates that this check was not initiated by the user
//

OSErr UpdateCheck (Boolean silently, Boolean archives)

{
	FSSpec	updateSpec;
	Handle	url;
	OSErr 	theError;
	long		reference;
	
	theError = noErr;
		
	// First, we need a file -- but make sure this is a temporary file because the server might be
	// handing us an empty update to signify "no change"
	NewTempSpec (Root.vRef, Root.dirId, nil, &updateSpec);

// (old way...)
//	theError = FSMakeFSSpec (Root.vRef, Root.dirId, GetRString (filename, UPDATE_FILE), &updateSpec);
//	if (theError == fnfErr)
//		theError = FSpCreate (&updateSpec, CREATOR, 'TEXT', smSystemScript);
	
	// Send a GET to the server to request updates
	if (url = GenerateAdwareURL (GetNagState (), UPDATE_SITE, archives ? actionArchived : actionUpdate, updateQuery, nil)) {
		theError = DownloadURL (LDRef (url), &updateSpec, silently, FinishedUpdateCheck, &reference, nil);			
		ZapHandle (url);
	}
	return (theError);
}


//
//	FinishedUpdateCheck
//
//		Completion routine for DownloadURL.  If 'silently' is true, the update check has been
//		performed without a user action.  If no error's were found we need to check to see if
//		the checksum on the file has changed.  If so, display the window.  If not, do
//		nothing.
//

void FinishedUpdateCheck (long silently, OSErr theError, DownloadInfo *info)

{
	MyWindowPtr 		win;
	NagUsageHandle	nagUsage;
	FSSpec					updateSpec;
	Str255					filename;
	uLong						hash;
	long						fileSize;
	short						oldResFile;
	Boolean					differentChecksum;

	differentChecksum = false;
	if (!silently)
		TellPayWindowTheUpdateCheckIsDone (theError);
	
	// Now that the update is complete, check to see if the server gave us anything.
	// If the update file is empty, no updates are available and we should not
	// automatically display the update page
	fileSize = theError ? 0 : FSpDFSize (&info->spec);

	// Make the real update file spec
	if (!theError) {
		theError = FSMakeFSSpec (Root.vRef, Root.dirId, GetRString (filename, UPDATE_FILE), &updateSpec);
		if (theError == fnfErr)
			theError = FSpCreate (&updateSpec, CREATOR, 'TEXT', smSystemScript);
	}
	
	// Exchange it with the new one -- but only if this file is not empty!
	if (!theError && fileSize) {
		theError = FSpExchangeFiles (&updateSpec, &info->spec);
		if (!theError)
			FSpDelete (&info->spec);
	}
	
	// Compare the checksum to what we've snuck in the update check's nag aux field but only if this is
	// NOT the first time we've performed an update check.  If the checksum has changed (or if this is
	// not a silent check) we'll display the update information to the user.  We also have to save the
	// checksum back to the settings file in the update nag's aux field.
	if (!theError && fileSize) {
		hash = HashFile (&updateSpec);
		oldResFile = CurResFile ();
		if (nagUsage = GetResourceFromFile (NAG_USAGE_TYPE, UPDATE_CHECK, SettingsRefN)) {
			differentChecksum = (*nagUsage)->aux && ((*nagUsage)->aux != hash);
			if (!(*nagUsage)->aux || differentChecksum) {
				(*nagUsage)->aux = hash;
				ChangedResource ((Handle) nagUsage);
				if (!ResError ()) 
					MyUpdateResFile	(SettingsRefN);
			}
			ReleaseResource (nagUsage);
		}
		UseResFile (oldResFile);
	}

	if (!theError && (!silently || differentChecksum))
		win = OpenText (&updateSpec, nil, nil, nil, true, nil, true, true);
}

#define KRHashPrime (2147483629) 

uLong HashFile (FSSpec *spec)

{
	Handle	text;
	Ptr			textPtr;
	Size		len;
	int 		Bit;
	uLong		sum; 
	
	sum = 0;
	if (!Snarf (spec, &text, 0)) {
		len = GetHandleSize (text);
		textPtr = *text;
		while (len--) { 
			for (Bit = 0x80; Bit != 0; Bit >>= 1) { 
				sum += sum; 
				if (sum >= KRHashPrime) sum -= KRHashPrime; 
				if ((*textPtr) & Bit) ++sum; 
				if (sum >= KRHashPrime) sum -= KRHashPrime; 
			}
			++textPtr;
		}
	} 
	return (sum + 1); 
}


/************************************************************************
 * CheckAdQT - if we're in adware mode, make sure QuickTime is installed
 ************************************************************************/
void CheckAdQT(void)
{
	if (IsAdwareMode() && !HaveQuickTime(0x0300))
	{
		//	No QuickTime. Must revert to freeware.
		switch (ComposeStdAlert(kAlertNoteAlert,CANT_AD)) {
			case 1:
				OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionSupport, helpQuery, topicNoQuickTime);
				EjectBuckaroo = true;
				break;
			case 2:
				TransitionState ((*nagState)->state==regAdwareUser ? regFreeUser : freeUser);
				break;
		}
	}
}


//
//	AdFailureCheck
//
//		Nag the user if they refuse to get ads (whether they want to or not).
//		We overload the nagBaseTime filed of the nagUsageRec to indicate the
//		last time we found that we checked for ad failure.
//
//		Note that we only look at the state of the 'NoAdsAuxRec' when we enter
//		into a new day.  At this point, the contents of the data structure
//		reflect the ad fetching state for the entirety of the previous day
//		which means we don't have to worry too much about mid day transitions.
//
//			If we did not get ads on the previous day we checked for ads...
//					...bump the deadbeat counter
//					...check to see if we're due for a nag
//
//			If we _did_ get ads...
//					...bump the consecutive day counter
//					...check to see if we should adjust the deadbeat counter and
//						 set the consecutive day counter back to zero
//
//			Regardless, at the beginning of a new day we have to initialize 'adsAreFailing'
//

OSErr AdFailureCheck (NagUsageHandle nagUsage, uLong currentTime, Boolean *nagMe, short *dialogID, PStr errString)

{
	OSErr				theError;

	theError = noErr;
	*nagMe	 = false;
	*errString = 0;
	
	// Are we checking on a new day?  If so, what happened yesterday?
	if (SecondsToWholeDay ((*nagUsage)->nagBase) != SecondsToWholeDay (currentTime)) {
		if (NoAdsRec.adsAreFailing) {
			if (NoAdsRec.deadbeatCounter >= kDaysUntilNoAdsNag) {
				*nagMe = true;
				if (NoAdsRec.deadbeatCounter >= kDeadbeatDays) {
					*dialogID = NAG_FRIGGING_HACKER_DLOG;
					NoAdsRec.deadbeatCounter = kDeadbeatDays-1;	// in case we ever get back to sponsored mode
				}
				else
					*dialogID = NAG_NOT_GETTING_ADS_DLOG;
			}
		}
	}
	
	// Keep track of the last time we checked for ad failures
	(*nagUsage)->nagBase = currentTime;
	
	return (theError);
}

#endif


#ifdef DEBUG
void DebugNagging (void)

{
	NagDialogInitProcPtr	initProc;
	NagDialogHitProcPtr		hitProc;
	ModalFilterUPP				theFilterProc;
	AdFailureRec					adFailure;
	UserStateType					oldState;
	MyWindowPtr						theDialogWin;
	DialogPtr							theDialog;
	Str255								string;
	Str255								sErr;
	short									dItem;
	long									dialogID;
	Boolean								ignoreDefaultItem;
	
	if (theDialogWin = GetNewMyDialog (NAG_DEBUG, nil, nil, InFront)) {
		theDialog = GetMyWindowDialogPtr (theDialogWin);
		oldState = GetNagState ();
		SetDItemState (theDialog, 8,  oldState == newUser);
		SetDItemState (theDialog, 9,  oldState == paidUser);
		SetDItemState (theDialog, 10, oldState == freeUser);
		SetDItemState (theDialog, 11, oldState == adwareUser);
		SetDItemState (theDialog, 12, oldState == regFreeUser);
		SetDItemState (theDialog, 13, oldState == regAdwareUser);
		SetDItemState (theDialog, 14, oldState == deadbeatUser);
		StartMovableModal (theDialog);
		ShowWindow (GetDialogWindow(theDialog));
		do {
			MovableModalDialog (theDialog, DlgFilterUPP, &dItem);
			if (dItem >= 6 && dItem <= 14) {
				SetDItemState (theDialog, 6,  dItem == 6);
				SetDItemState (theDialog, 7,  dItem == 7);
				SetDItemState (theDialog, 8,  dItem == 8);
				SetDItemState (theDialog, 9,  dItem == 9);
				SetDItemState (theDialog, 10, dItem == 10);
				SetDItemState (theDialog, 11, dItem == 11);
				SetDItemState (theDialog, 12, dItem == 12);
				SetDItemState (theDialog, 13, dItem == 13);
				SetDItemState (theDialog, 14, dItem == 14);
			}
		} while (dItem != ok && dItem != cancel);
		EndMovableModal (theDialog);
		if (dItem == ok) {
			GetDIText (theDialog, 4, string);
			dItem = ok;
		}
		DisposeDialog_ (theDialog);
		if (dItem == ok) {
				if (string[0]) {
					StringToNum (string, &dialogID);
					if (dialogID) {
						initProc					= nil;
						hitProc						= nil;
						theFilterProc			= nil;
						ignoreDefaultItem	= false;
						adFailure.text[0] = 0;
						adFailure.error		= noErr;
						switch (dialogID) {
							case NAG_INTRO_DLOG:
								hitProc = IntroDlogHit;
								theFilterProc = DlgFilterUPP;
								break;
							case JUNKDOWN_DLOG:
								hitProc = JunkDownHitProc;
								theFilterProc = DlgFilterUPP;
								break;
							case REPAY_DLOG:
								hitProc = RepayHitProc;
								theFilterProc = DlgFilterUPP;
								break;
							case NAG_PLEASE_REGISTER_DLOG:
								hitProc = AdUserHit;
								break;
							case NAG_DOWNGRADE_DLOG:
								initProc = DowngradeDlogInit;
								hitProc  = DowngradeDlogHit;
								break;
							case NAG_FEATURES_DLOG:
								initProc = FeaturesDlogInit;
								hitProc	 = FeaturesDlogHit;
								break;
							case NAG_NOT_GETTING_ADS_DLOG:
								initProc = NotGettingAdsDlogInit;
								hitProc	 = NotGettingAdsDlogHit;
								adFailure.error		= 503;
								GetRString(sErr, HTTP_ERR_STRN + 3);
								ComposeRString (adFailure.text,HTTP_ERR_FORMAT,sErr,adFailure.error);
								break;
							case NAG_FRIGGING_HACKER_DLOG:
								hitProc						= NoAdsRevertToFreeDlogHit;
								theFilterProc			= DlgFilterUPP;
								ignoreDefaultItem	= true;
								break;
						}
						Nag (dialogID, initProc, hitProc, theFilterProc, ignoreDefaultItem, (long) &adFailure.text);
					}
				}
		}
	}
}
#endif

void AutoSwitchClientMode(void)
{
	if (NewClientModePlusOne)
	{
		switch (NewClientModePlusOne-1)
		{
			case 0: TransitionState(adwareUser); break;
			case 1: 
			case 3: 
				// User is being beaten dead
				if (kAlertStdAlertOtherButton==ComposeStdAlert(Note,PROFILE_FAILURE))
				{
					if (SendUserToProfile())
					if (kAlertStdAlertOKButton==ComposeStdAlert(Note,-PROFILING_NOW))
					{
						// user has indicated he has profiled.  Make a playlist request
						// if he has profiled, it won't switch him to light after all
						// if he has failed to profile, it will switch him to light again
						ForcePlaylistRequest();
						break;
					}
				}
				// Hasta la vista, bebe's kids
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
				TransitionState(NewClientModePlusOne-1==3 ? profileDeadbeatUser : deadbeatUser);
#endif
				break;
			case 2: TransitionState(paidUser); break;
		}
		NewClientModePlusOne = 0;
	}
}

extern ModalFilterUPP DlgFilterUPP;
Boolean PlaylistNagHitProc (EventRecord *event, DialogPtr theDialog, short itemHit,long refcon);
OSErr PlaylistNagInitProc(DialogPtr theDialog,long refcon);
/************************************************************************
 * DoPlaylistNag - Ask the user if it's ok to send the audit stats
 ************************************************************************/
Boolean DoPlaylistNag(short nagID)
{
	Boolean retVal = userCanceledErr != Nag(nagID,PlaylistNagInitProc,PlaylistNagHitProc,nil,false, nagID);
	ZapResource(NAG_REQUEST_TYPE,nagID);
	return retVal;
}

#define kPlNagOK 2
#define kPlNagProfile 1
#define kPlNagExplain 3
#define kPlNagParam 5
#define kPlNagFeatures 8
/************************************************************************
 * PlaylistNagHitProc - handle item hits
 ************************************************************************/
Boolean PlaylistNagHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon)
{
	switch (itemHit) {
		case kPlNagExplain:
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionProfileidReqd, profileidReqdQuery , nil);
			CloseMyWindow(GetDialogWindow(theDialog));
			return true;
		case kPlNagProfile:
			SendUserToProfile();
			CloseMyWindow (GetDialogWindow(theDialog));
			return true;
			break;
		default:
			CloseMyWindow (GetDialogWindow(theDialog));
			return true;
			break;
	}
	return (false);
}

/************************************************************************
 * PlaylistNagInitProc - init dialog by filling in the fields
 ************************************************************************/
OSErr PlaylistNagInitProc(DialogPtr theDialog,long refcon)
{
	Handle text = GetResource(NAG_REQUEST_TYPE,refcon);
	Str255 s;
	
	if (text)
	{
		MakePStr(s,*text,GetHandleSize(text));
		SetDIText(theDialog,kPlNagParam,s);
	}
	
	// the feature list
	if (refcon==PLNAG_LEVEL1_DLOG)
		PutFeaturesInto(theDialog,kPlNagFeatures);
	
	return noErr;
}
