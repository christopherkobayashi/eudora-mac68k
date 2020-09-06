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

#ifndef NAG_H
#define NAG_H
#ifdef NAG

#include <MyRes.h>

#define	kTEXTResIDFollowsChar	'~'
#define	kSecondsPerDay				 (24*60*60)
#define	kDeadbeatDays					14
#define	kAdGracePeriod				 2
#define	kDaysUntilNoAdsNag		 3

typedef enum {
	noDialogPending				= 0,
	paymentRegPending			= 1,
	codeEntryPending			= 2,
	repayPending					= 3,
	gracelessRepayPending	= 4
} InitNagResultType;

//
//	User states
//
//		These states should have the same values as the 'Nag#' resources stored in
//		the application.
//
//		Removed all references to defunct user states (like ep4User, etc...)
//

typedef enum {
	ep4User				 = NAG_EP4_USER,
	ep4RegUser		 = NAG_EP4_REG_USER,
	newUser				 = NAG_NEW_USER,					// User trying Eudora for the first time
	paidUser			 = NAG_PAID_USER,					// User who has paid, entered his registration code, and is using the entitled version of Eudora
	freeUser			 = NAG_FREE_USER,					// User who has chosen to use Freeware, but who has not entered a Freeware registration number
	adwareUser		 = NAG_AD_USER,						// User who is using the version that displays ads
	regFreeUser		 = NAG_REG_FREE_USER,			// Free user who has entered a Freeware registration code
	regAdwareUser	 = NAG_REG_ADD_USER,			// Ad user who has entered a Freeware registration code
	deadbeatUser	 = NAG_DEADBEAT_USER,			// A dead beat
	boxUser				 = NAG_BOX_USER,					// User who got their reg code from an installer
	profileDeadbeatUser	= NAG_PROFILE_DEADBEAT_USER,						// User who got their reg code from an installer
	unpaidUser		 = NAG_UNPAID_USER,				// User who is using a version of Eudora for which they are not entitled
	repayUser			 = NAG_REPAY_USER,				// User who is subject to repaying and who has clicked on "Pay Now".  They will turn to sponsored in an hour or at their next restart
	userStateLimit
} UserStateType;

// Added paranoia for the death build
#ifdef DEATH_BUILD
#define	GetNagState()							paidUser
#define	ValidUser(state)					true
#define	EP4User(state)						false
#define	NewUser(state)						false
#define	FreeUser(state)						false
#define	PayUser(state)						true
#define	RepayUser(state)					false
#define	AdwareUser(state)					false
#define	RegisteredUser(state)			true
#define	DeadbeatUser(state)				false
#define	ProfileDeadbeatUser(state)				false
#define	UnpaidUser(state)					false
#define	BoxUser(state)						true
#define	PayMode(state)						true
#define	AdwareMode(state)					false
#define	FreeMode(state)						false

#define	IsValidUser()							true
#define	IsEP4User()								false
#define	IsNewUser()								false
#define	IsFreeUser()							false
#define	IsPayUser()								true
#define	IsRepayUser()							false
#define	IsAdwareUser()						false
#define	IsRegisteredUser()				true
#define	IsDeadbeatUser()					false
#define	IsProfileDeadbeatUser()		false
#define	IsUnpaidUser()						false
#define IsBoxUser()								true
#define	IsPayMode()								true
#define	IsAdwareMode()						false
#define	IsFreeMode()							false

#define IsProfiledUser()					true
#else
#define	GetNagState()							(nagState ? (*nagState)->state : adwareUser)
#define	ValidUser(state)					((state) == ep4User || (state) == ep4RegUser || (state) == newUser || (state) == paidUser || (state) == freeUser || (state) == adwareUser || (state) == regFreeUser || (state) == regAdwareUser || (state) == deadbeatUser || (state) == profileDeadbeatUser || (state) == boxUser || (state) == profileDeadbeatUser || (state) == unpaidUser || (state) == repayUser)
#define	EP4User(state)						((state) == ep4User || (state) == ep4RegUser)
#define	NewUser(state)						((state) == newUser)
#define	FreeUser(state)						((state) == freeUser || (state) == regFreeUser || (state) == deadbeatUser || (state) == profileDeadbeatUser)
#define	PayUser(state)						((state) == paidUser || (state) == repayUser)
#define	RepayUser(state)					((state) == repayUser)
#define	AdwareUser(state)					((state) == adwareUser || (state) == regAdwareUser)
#define	RegisteredUser(state)			((state) == paidUser || (state) == regFreeUser || (state) == regAdwareUser)
#define	DeadbeatUser(state)				((state) == deadbeatUser || (state) == profileDeadbeatUser)
#define	ProfileDeadbeatUser(state)				((state) == profileDeadbeatUser)
#define	UnpaidUser(state)					((state) == unpaidUser)
#define	BoxUser(state)						((state) == boxUser)
#define	PayMode(state)						(PayUser (state) || BoxUser (state))
#define	AdwareMode(state)					(AdwareUser (state) || NewUser (state) || UnpaidUser (state))
#define	FreeMode(state)						(!(PayMode (state) || AdwareMode (state)))

#define	IsValidUser()							(nagState ? ValidUser((*nagState)->state) : false)
#define	IsEP4User()								(nagState ? EP4User((*nagState)->state) : false)
#define	IsNewUser()								(nagState ? NewUser((*nagState)->state) : false)
#define	IsNewUser()								(nagState ? NewUser((*nagState)->state) : false)
#define	IsFreeUser()							(nagState ? FreeUser((*nagState)->state) : false)
#define	IsPayUser()								(nagState ? PayUser((*nagState)->state) : false)
#define	IsRepayUser()							(nagState ? RepayUser((*nagState)->state) : false)
#define	IsAdwareUser()						(nagState ? AdwareUser((*nagState)->state) : true)
#define	IsRegisteredUser()				(nagState ? RegisteredUser((*nagState)->state) : false)
#define	IsDeadbeatUser()					(nagState ? DeadbeatUser((*nagState)->state) : false)
#define	IsProfileDeadbeatUser()		(nagState ? ProfileDeadbeatUser((*nagState)->state) : false)
#define	IsUnpaidUser()						(nagState ? UnpaidUser((*nagState)->state) : false)
#define IsBoxUser()								(nagState ? BoxUser((*nagState)->state) : false)
#define	IsPayMode()								(nagState ? PayMode((*nagState)->state) : false)
#define	IsAdwareMode()						(nagState ? AdwareMode((*nagState)->state) : true)
#define	IsFreeMode()							(nagState ? FreeMode((*nagState)->state) : false)
#define CanPayMode()							(gCanPayMode || IsPayMode())

#define IsProfiledUser()	(*GetProfileID(GlobalTemp)>1)
Boolean UserHasValidPaidModeRegcode(void);
#endif

//
//	NagUsageRec
//
//		A record of this type exists for each of the nag dialogs and will
//		be stored in the settings file ('NagU') to keep track of which nags
//		have been displayed and when.
//

#define NAG_USAGE_VERS		0

typedef struct {
	short		version;					// Version of the NagUsageRec
	uLong		nagBase;					// Base from which all nags are determined (0 means not yet nagged)
	uLong		lastNag;					// Date/time of the last nag
	UInt32	aux;							// auxilliary field for use by any nag (but specifically by the no ads nags)
} NagUsageRec, *NagUsagePtr, **NagUsageHandle;

// Must be 32 bits
typedef struct {
	UInt8 unused:4;
	UInt8 checkedMailToday:1;	// did we check mail today?
	UInt8 checkedMail:1;			// did we check mail 'yesterday'?
	UInt8	adsAreSucceeding:1;	// we got significant facetime and ad facetime
	UInt8	adsAreFailing:1;		// we got significant facetime, but insufficient ad facetime
														// They will neither succeed nor fail if there is only a tiny amount of facetime
														// on a given day.
	UInt8		consecutiveDays;	// Number of consecutive days (up to 'kAdGracePeriod') we've received ads
	UInt16	deadbeatCounter;	// Running counter used to determine when someone not getting ads should become a deadbeat
} NoAdsAuxRec, *NoAdsAuxPtr, **NoAdsAuxHandle;

//
//	NagStateRec
//
//		Maintains the current nagging state between launches of Eudora.  The
//		content of this structure is stored in a resource of type 'NagS' in
//		the settings file.  Currently, there's not much in here... but it
//		could grow, so it's a struct.
//

#define NAG_STATE_VERS 0

typedef struct {
	short					version;						// Version of the NagStateRec
	UserStateType	state;							// Current state of nagging for this user
	uLong					regDate;						// The date/time of the most recent RegCode file
} NagStateRec, *NagStatePtr, **NagStateHandle;

	
//
//	Nag Records
//
//		Nag records are store in resources of type 'Nag '.  This resource
//		contains information about the schedules and dialogs to be used for each
//		nag state.
//
//		The format of the 'Nag ' resource is as follows:
//
//			UInt32	schedule;
//			UInt16	interval;
//			short		dialogID;
//			UInt32	flags;
//
//		Where each bit of the 'schedule' represents whether or not we're due to nag on that
//		particular day following the nag base.  The nag base is therefore bit 0, the
//		following day is bit 1, and so on.  A set bit indicates that we will nag on that
//		particular day following the nag base.
//
//		The repeat interval is the frequency with which we'll nag after the initial month
//		of nags has been exhausted.
//
//		The dialogID is the resourceID of the 'DLOG' resource used to display this nag.
//
//		'flag' is intended to be used to define various special attributes of a nag.  So
//		far, it doesn't do a whole lot, but could be useful in the future to override
//		certain nagging behaviors (get it?  nagging behaviors?  bad habits?  Oh nevermind).
//
//				0x00000001  Check for Eudora application updates with the next mail check
//

#define	nagFlagSilentUpdate					0x00000001
#define	nagFlagUpdate								0x00000002
#define	nagFlagAdFailureCheck				0x00000004

typedef struct {
	UInt32	schedule;
	UInt16	interval;
	short		dialogID;
	UInt32	flags;
} NagRec, *NagPtr, **NagHandle;

typedef struct {
	short		count;
	short		nagID[];
} NagListRec, *NagListPtr, **NagListHandle;

typedef	OSErr (*NagDialogInitProcPtr) (DialogPtr dlog, long refcon);
typedef	Boolean	(*NagDialogHitProcPtr) (EventRecord *event, DialogPtr dlog, short item, long refcon);

typedef struct {
	Str255	text;
	OSErr		error;
} AdFailureRec, *AdFailurePtr, **AdFailureHandle;

InitNagResultType	InitNagging (void);
void							DoPendingNagDialog (InitNagResultType pendingNagResult);
void							CheckNagging (UserStateType state);
OSErr							Nag (short dialogID, NagDialogInitProcPtr initProc, NagDialogHitProcPtr hitProc, ModalFilterUPP theFilterProc, Boolean ignoreDefaultItem, long refcon);
MyWindowPtr				FindNag (short dialogID);
Boolean						IntroDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
Boolean						AdUserHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
Boolean						DowngradeDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
OSErr							DowngradeDlogInit (DialogPtr dlog, long refcon);
OSErr							FeaturesDlogInit (DialogPtr dlog, long refcon);
Boolean						FeaturesDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
OSErr							NotGettingAdsDlogInit (DialogPtr dlog, AdFailurePtr adFailurePtr);
Boolean						NotGettingAdsDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
Boolean						NoAdsRevertToFreeDlogHit (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);
FeatureCellHandle BuildFeatureList (Boolean ignoreUsage);
OSErr							SetFeatureCell (FeatureCellPtr *featureCellsPtr, FeatureRecPtr featurePtr, Boolean ignoreUsage);
void							DownGradeDialog (void);
void							NotifyDownGradeDialog (void);
void							RepayDialog (InitNagResultType pendingNagResult);
OSErr							SubstituteLongStaticTextItems (DialogPtr theDialog);
OSErr							GetIndNagID (short *nagID, short nagListID, short index);
OSErr							GetIndNagState (NagPtr theNag, short *nagID, UserStateType state, short index);
void							TransitionState (UserStateType newState);
uLong							DaysSinceNagBase (short dialogID);
uLong							DurationOfSchedule (UserStateType state, short whichSchedule);
uLong							SecondsToWholeDay (uLong secs);
short							MostSignificantSetBit (UInt32 value);

void							RelaunchEudora (UserStateType newState);
OSErr							UpdateCheck (Boolean silently, Boolean archives);
void							FinishedUpdateCheck (long silently, OSErr theError, DownloadInfo *info);
uLong							HashFile (FSSpec *spec);
OSErr							AdFailureCheck (NagUsageHandle nagUsage, uLong currentTime, Boolean *nagMe, short *dialogID, PStr errString);
Boolean						HitMeHitMeHitMe (EventRecord *event, DialogPtr theDialog, short itemHit, long dialogRefcon);	// just returns true
short							GrowGroupItems(WindowPtr win,ControlHandle parentCntl);
Boolean						IsGroupControl(ControlHandle cntl);

void							AutoSwitchClientMode(void);
Boolean						DoPlaylistNag(short nagID);
void 							JunkDownDialog (void);

#ifdef DEBUG
void					 		DebugNagging (void);
#endif

#endif
#endif
