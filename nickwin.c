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

#include "nickwin.h"
#include "listview.h"
#define FILE_NUM 3
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1992 by Qualcomm, Inc. */
/**********************************************************************
 * handling the alias panel
 **********************************************************************/

// Some basic geometry
#define	kVertDividerWidth				 2
#define	kHorzZoomWidth					16
#define	kApparentTabTitleSlop			12

#define	kMinListWidth					(180)
#define kAquaHeightAdjustment			7						// slight height adjustment for Aqua

#define	kMinNickWinHeight			 	340+2*kAquaHeightAdjustment
#define	kTabWidthSlop					32							// Extra pixels to accomodate a min width for State and Zip fields
	
// Tag delimeters
#define	kFieldTagBegin			'<'
#define	kFieldTagEnd				'>'
#define	kFieldTagSeparator	':'
#define kDragDataDelim			'\r'


//	First few items in the nickname list
enum {
	kItemPersonalNicknames = 1,		// The nickname for the user of this machine
	kItemEudoraNicknamesAB,				// The Eudora Nicknames address book
	kItemNicknames								// ev'r thang else
};

// Icon constants for list view
enum {
	kAddressBookIcon	= kGenericFolderIconResource,
	kNicknameItem			= MAILBOX_ONLY_ICON,
	kGroupItem				= MAILBOX_ONLY_ICON
};

// Control constants
typedef enum {
	abViewByPopup = 0,
	abNewNicknameButton,
	abNewAddressBookButton,
	abRemoveButton,
	abToButton,
	abCcButton,
	abBccButton,
	abChatButton,
	abZoomHorizontalButton,
	abRecipientCheckbox,
	abNickField,
	abTabs,
	abDoNotSyncCheckbox,
	abControlCount
} ABControlType;

#pragma segment NickWin

// Address book windows structure
typedef struct {
	MyWindowPtr		win;
	ViewList			list;
	ControlHandle	controls[abControlCount];
	Handle		oldRecipientMenu;
	Rect					vertDivider;
	Rect					listRect;
	Rect					tabRect;
	Rect					nickFieldRect;
	Str255				sortTag;
	NickFocusType	nickFocus;
	long					listFlags;
	short					minTabWidth;
	short					minListWidth;
	short					collapsed;
	short					currentTab;
	short					abDisplayed;
	short					nickDisplayed;
	Boolean				tickleMe;
	Boolean				inited;
} ABType, *ABPtr, **ABHandle;

ABType	AB;
short		gSortAB;			// Address book being sorted - used by our sort routines
Boolean gPageOpened;

// Some convenient shorthand
#define	Win									AB.win
#define	NickList						AB.list
#define	Controls						AB.controls
#define	CtlViewBy						AB.controls[abViewByPopup]
#define	CtlNewNickname			AB.controls[abNewNicknameButton]
#define	CtlNewAddressBook		AB.controls[abNewAddressBookButton]
#define	CtlRemove						AB.controls[abRemoveButton]
#define	CtlTo								AB.controls[abToButton]
#define	CtlCc								AB.controls[abCcButton]
#define	CtlBcc							AB.controls[abBccButton]
#define	CtlChat							AB.controls[abChatButton]
#define	CtlZoomHorizontal		AB.controls[abZoomHorizontalButton]
#define	CtlRecipient				AB.controls[abRecipientCheckbox]
#define	CtlDoNotSync				AB.controls[abDoNotSyncCheckbox]
#define	OldRecipientMenu		AB.oldRecipientMenu
#define	NickField						AB.controls[abNickField]
#define	Tabs								AB.controls[abTabs]
#define	VertDivider					AB.vertDivider
#define	ListRect						AB.listRect
#define	TabRect							AB.tabRect
#define	NickFieldRect				AB.nickFieldRect
#define	SortTag							AB.sortTag
#define	NickFocus						AB.nickFocus
#define	ListFlags						AB.listFlags
#define	MinTabWidth					AB.minTabWidth
#define	MinListWidth				AB.minListWidth
#define	CurrentTab					AB.currentTab
#define	Collapsed						AB.collapsed
#define	TickleMe						AB.tickleMe

typedef struct
{
	PETEHandle pte;
	PETEParaInfo pi1;
	PETEParaInfo pi2;
	PETEParaInfo pi3;
	short ab;
	short nick;
	TextAddrHandle addresses;
	Str31 name;
	Boolean foundOne;
	ControlHandle tabPane;
} NickPrintStuff, *NickPrintStuffPtr;

typedef struct {
	Accumulator		a;
	short					refNum;
	OSErr					theError;
	short					ab;
	short					nick;
	Str255				eol;
	Str255				comma;
	Str255				commaReplace;
	Str255				crReplace;
} NickExportRec, *NickExportPtr, **NickExportHandle;


Boolean ABHasSelection (MyWindowPtr win);

Boolean ABIsChattable(void);
OSErr ABChat(short modifiers);
Boolean ABPrintPaneProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, NickPrintStuffPtr myStuffP);
Boolean ABPrintObjectProc (TabObjectPtr objectPtr, NickPrintStuffPtr myStuffP);
OSErr PrintAddressBookWindow(Boolean select,Boolean now);
OSErr PrintNickFile (short which, Boolean select, PETEHandle pte);
void PrintNickname (NickPrintStuffPtr myStuffP, short ab, short nick);
OSErr NickPage(short *pageNum,PStr title,Rect *uRect,short *v);
void ABPrintMakeTitle(PStr title, Boolean select);
void BuildNickPrintTitle(short which,Boolean select,PStr title);
OSErr NickPrintName(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name);
OSErr NickPrintNameAndValue(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name, PStr value);
OSErr PrintANick(NickPrintStuffPtr myStuffP);
OSErr NickPrintAddresses(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name, UHandle value);
Boolean TheWorldAccordingToMarthaStewart(MyWindowPtr win);
void DoAddressBookExport (Boolean select);
OSErr ExportNickFile (NickExportPtr nickExport, short ab, Boolean select);
OSErr ExportNickname (NickExportPtr nickExport, short ab, short nick);
Boolean ExportNicknameProc (TabObjectPtr objectPtr, NickExportPtr nickExport);
pascal OSErr NickAddrSetDragContents(PETEHandle pte,DragReference drag);
pascal OSErr NickGetDragContents(PETEHandle pte,UHandle *theText, PETEStyleListHandle *theStyles, PETEParaScrapHandle *theParas, DragReference drag, long dropLocation);

// Stuff to eventually go away
/* Information about user defined fields */
typedef struct
{
	long	height; 								// How many lines high?
	long width; 									// 1 = normal width, 2 = half width
	Str255	name; 								// Text to be displayed next to field
	Str255	tag;	// Tag in the notes field
	Rect	theRect;								// Current field rectangle
} fieldInfo;

void GetABNick (VLNodeID nodeID, short *ab, short *nick)

{
	*ab		= GetAddressBook (nodeID);
	*nick	= GetNickname (nodeID);
}

Boolean GetIndexedABNick (short index, short *ab, short *nick)

{
	VLNodeInfo	data;

	if (LVGetItem (&NickList, index, &data, false)) {
		GetABNick (data.nodeID, ab, nick);
		return (true);
	}
	*ab		= kAddressBookUndefined;
	*nick	= kNickUndefined;
	return (false);
}

Boolean GetSelectedABNick (short index, short *ab, short *nick)

{
	VLNodeInfo	data;

	if (LVGetItem (&NickList, index, &data, true)) {
		GetABNick (data.nodeID, ab, nick);
		return (true);
	}
	*ab		= kAddressBookUndefined;
	*nick	= kNickUndefined;
	return (false);
}


PStr GetSelectedName (short index, PStr name)

{
	VLNodeInfo	data;

	*name = 0;
	if (LVGetItem (&NickList, index, &data, true))
		PCopy (name, data.name);
	return (name);
}


PStr GetSelectedABNickName (short index, short *ab, short *nick, PStr name)

{
	VLNodeInfo	data;

	*name = 0;
	if (LVGetItem (&NickList, index, &data, true)) {
		PCopy (name, data.name);
		GetABNick (data.nodeID, ab, nick);
	}
	return (name);
}


PStr GetSelectedABNickNameData (short index, short *ab, short *nick, PStr name, VLNodeInfo *data)

{
	*name = 0;
	if (LVGetItem (&NickList, index, data, true)) {
		PCopy (name, data->name);
		GetABNick (data->nodeID, ab, nick);
	}
	return (name);
}

void OpenABWin (PStr findStr)

{
	PETEDocInitInfo	pdi;
	PETEHandle			pte;
	TabObjectPtr		objectPtr;
	WindowPtr				WinWP;
	ControlHandle		tabPane;
	Str255					title,
									tag;
	Rect						r;
	OSErr						theError;
	short						tabIndex;
	
	WinWP		 = nil;
	theError = noErr;

	if (SelectOpenWazoo (ALIAS_WIN))
	{
		// Do an initial find, if need be
		if (findStr)
			if (!FindNextNicknameContainingString(Win,findStr)) SysBeep(20L);
		return;	//	Already opened in a wazoo
	}

	// make sure the alias list is in memory
	if (RegenerateAllAliases ((MainEvent.modifiers & optionKey) != 0))
		goto fail;
	
	// Clean up the recipient menu if we've been asked
	if (MainEvent.modifiers & optionKey)
		PruneRecipMenu ();

	if (AB.inited)
		UserSelectWindow (GetMyWindowWindowPtr (Win));
	else {
		if (!(Win = GetNewMyWindow (ALIAS_WIND,nil,nil,BehindModal,False,False,ALIAS_WIN))) {
			theError = MemError ();
			goto fail;
		}

		// This needs to be initialized early so our close routine gets called if anything else fails
		Win->close = ABClose;

		// Initialize our private data
		AB.abDisplayed	 = kAddressBookUndefined;
		AB.nickDisplayed = kNickUndefined;
		AB.collapsed		 = GetPrefLong (PREF_NICK_COLLAPSED);

		WinWP = GetMyWindowWindowPtr (Win);

		// If anybody is wazooed with us, do not allow collapsing
		if (IsWazoo(WinWP) && !IsLonelyWazoo(WinWP)) AB.collapsed = false;
		
		SetPort_ (GetMyWindowCGrafPtr (Win));
		ConfigFontSetup (Win);	
		MySetThemeWindowBackground (Win, kThemeListViewBackgroundBrush, False);

		// controls
		if (!(CtlViewBy					= CreateMenuControl(Win, nil, GetRString (title, VIEW_BY_LABEL), NICK_VIEW_BY_MENU, kControlPopupUseWFontVariant + kControlPopupVariableWidthVariant, 0, true))																			||		// View by menu
			  !(CtlNewNickname		= NewIconButton (AB_NEW_NICKNAME_CNTL, WinWP)) 																						||		// New Nickname button
			  !(CtlNewAddressBook	= NewIconButton (AB_NEW_ADDRESSBOOK_CNTL, WinWP)) 																				||		// New Address Book button
			  !(CtlRemove					= NewIconButton (AB_REMOVE_CNTL, WinWP)) 																									||		// Remove button
			  !(CtlTo							= CreateControl (Win, nil, HEADER_LABEL_STRN + TO_HEAD, kControlPushButtonProc, true))		||		// To button
			  !(CtlCc							= CreateControl (Win, nil, HEADER_LABEL_STRN + CC_HEAD, kControlPushButtonProc, true))		||		// Cc button
			  !(CtlBcc						= CreateControl (Win, nil, HEADER_LABEL_STRN + BCC_HEAD, kControlPushButtonProc, true))		||		// Bcc button
			  !(CtlChat						= CreateControl (Win, nil, ICHAT_BTN_TEXT, kControlPushButtonProc, true))									||		// Chat button
			  !(CtlRecipient			= CreateControl (Win, nil, ALIAS_ON_RECIPIENT_LIST, kControlCheckBoxProc, true))					||		// Recipient checkbox
			  !(CtlDoNotSync			= CreateControl (Win, nil, ALIAS_DO_NOT_SYNC, kControlCheckBoxProc, true))								||		// Do not sync checkbox
				!(CtlZoomHorizontal	= GetNewControlSmall (NICK_HZOOM_CNTL, WinWP))) {																								// Horizontal zoom button
					theError = MemError ();
					goto fail;
		}
		SetControlMaximum (CtlRecipient, 2);
		SetControlMaximum (CtlDoNotSync, 2);
		
		OutlineControl (CtlTo, true);
		
		// (jp) For setting up our PETE's.
		DefaultPII (Win, false, peNoStyledPaste | peClearAllReturns, &pdi);

		// Label Fields
		NickField = CreateLabelField (Win, nil, GetRString (title, ALIAS_A_LABEL), 0, teForceLeft, labelAutoSize, &pdi, peNoStyledPaste | peClearAllReturns);

		CleanPII(&pdi);

		// Tabs (we don't know the min tab width at this point)
		SetRect (&r, 0, 0, 800, kMinNickWinHeight);
		if (Tabs = CreateTabControl (Win, &r, CREATOR, kNickTabType, false)) {
			tabIndex = 1;
			if (tabPane = FindTabPaneWithTitle (Tabs, GetPref (title, PREF_LAST_NICK_TAB)))
				tabIndex = GetTabPaneIndex (tabPane) + 1;
			SetControlValue (Tabs, tabIndex);
			CurrentTab = SwitchTabs (Tabs, 0, tabIndex, PREF_LAST_NICK_TAB);
			SetControlVisibility(Tabs,true,false);
		}
		
		// View By menu
		BuildViewByMenu (Tabs, CtlViewBy);

		// Nickname list
		ListFlags = kfListSupportsFocus | kfSupportsSelectCallbacks | kfSupportsMondoBigList | kfManualRowAddsExpected;
		SetRect (&r, -20, -20, 0, 0);
		if (LVNew (&NickList, Win, &r, kAdddressBookRootID, AddressBookLVCallBack, kLVDoOwnDragAdd)) {
			theError = MemError();
			goto fail;
		}

		// The alias expansion field has special editing and drag capabilities, as does "other email" (dragging only for now)
		if (objectPtr = FindObjectWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagEmail))) {
			if (pte = GetLabelFieldPete (objectPtr->control)) {
				SetNickScanning (pte, nickHighlight | nickComplete | nickSpaces, kNickScanAllAliasFiles, nil, nil, nil);
				PETESetCallback (PETE, pte, (void *) NickAddrSetDragContents, peSetDragContents);
				PETESetCallback (PETE, pte, (void *) NickGetDragContents, peGetDragContents);
			}
			UnlockObject (objectPtr);
		}
		if (objectPtr = FindObjectWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagOtherEmail))) {
			if (pte = GetLabelFieldPete (objectPtr->control)) {
				PETESetCallback (PETE, pte, (void *) NickAddrSetDragContents, peSetDragContents);
				PETESetCallback (PETE, pte, (void *) NickGetDragContents, peGetDragContents);
			}
			UnlockObject (objectPtr);
		}

		// Set the windows minimum sizes once everything has been created
		MinListWidth = 2 * (2 * INSET) + kMinListWidth;
		MinTabWidth	 = 2 * (2 * INSET) + 8 + kTabWidthSlop;
		IterateTabPanes (Tabs, CalcMinTabWidthProc, &MinTabWidth);
		
		if (Collapsed)
			SetWinMinSize (Win, MinListWidth, kMinNickWinHeight);
		else
			SetWinMinSize (Win, MinListWidth + MinTabWidth, kMinNickWinHeight);
//		SetWinMinSize (Win, MinListWidth + MinTabWidth, kMinNickWinHeight);
		
		// Address book functions
		Win->didResize	 = ABDidResize;
		Win->update			 = ABUpdate;
		Win->position		 = PositionPrefsTitle;
		Win->click			 = ABClick;
		Win->bgClick		 = ABClick;
		Win->cursor			 = ABCursor;
		Win->activate		 = ABActivate;
		Win->help				 = ABHelp;
		Win->menu				 = ABMenu;
		Win->key				 = ABKey;
		Win->app1				 = ABKey;
		Win->drag				 = ABDragHandler;
		Win->idle				 = ABIdle;
		Win->find				 = ABFind;
		Win->dirty			 = ABDirty;
		Win->selection	 = ABHasSelection;
		Win->menuEnable	 = ABMenuEnable;

		// Other initialization
		Win->dontControl = true;
		Win->userSave = true;

		// Save a copy of the recipient menu in case we make changes, then decide to discard
		if (OldRecipientMenu = GetResource('STR#',RECIPIENT_STRN))
			MyHandToHand (&OldRecipientMenu);
		
		AB.inited = true;
		// clean all the dirty stuff so we're ready for user input
		ABClean ();
		
		// If we open in the collapsed state, hide the things on the right side
		if (Collapsed) {
			MoveLabelField (NickField, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (NickField), ControlHi (NickField));
			MoveMyCntl (CtlRecipient, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (CtlRecipient), ControlHi (CtlRecipient));
			MoveMyCntl (CtlDoNotSync, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (CtlDoNotSync), ControlHi (CtlDoNotSync));
			MoveTab (Tabs, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (Tabs), ControlHi (Tabs));
		}

		NickFocus = focusNoChange;
		ABSetKeyboardFocus (focusNickList);
		
		// Do an initial find, if need be
		if (findStr)
			if (!FindNextNicknameContainingString(Win,findStr)) SysBeep(20L);
		
		// Finally, show and size the window
		ShowMyWindow (WinWP);
		MyWindowDidResize (Win,&Win->contR);
		return;
	}	

fail:
	if (WinWP)
		CloseMyWindow (WinWP);
	if (theError)
		WarnUser (COULDNT_WIN, theError);
}


Boolean CalcMinTabWidthProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, short *width)

{
	MyWindowPtr				win;
	ControlTabInfoRec tabInfo;
	Size							actualSize;
	
	win = GetWindowMyWindowPtr(GetControlOwner(tabControl));
	tabInfo.version = 0;
	if (!GetControlData (tabControl, tabIndex, kControlTabInfoTag, sizeof (ControlTabInfoRec), &tabInfo, &actualSize))
		MinTabWidth	 += (2 * kApparentTabTitleSlop + win->hPitch * tabInfo.name[0]);
	return (false);
}



//
//	ABClean
//
//		Make sure all the fields and UI dirty bits are ready for user input
//
void ABClean (void)

{
	if (AB.inited) {
		PETEMarkDocDirty (PETE, GetLabelFieldPete (NickField), false);
		CleanTab (Tabs);
		Win->isDirty = false;
	}
}

/************************************************************************
 * ABDidResize - resize the Address Book window
 ************************************************************************/
void ABDidResize (MyWindowPtr win, Rect *oldContR)

{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Rect			rClipZero,
						r;
	short			htAdjustment,
						buttonHeight,
						hTab1,						// left side of list
						hTab2,						// right side of list
						hTab3,						// left side of vertical drag bar
						hTab4,						// left side of content tab
						hTab5,						// right side of content tab
						vTab1,						// top of view by menu
						vTab2,						// top of list
						vTab3,						// bottom of list
						vTab4,						// top of bevel buttons
						vTab5,						// top of recipient buttons
						vTab6,						// bottom of content tab
						vTab7,						// top of the content tab
						winWidth,
						listWidth,
						nickPrct;

	//	clip to nothing so we don't get controls leaving behind residues when they move
	SetPort (winPort);
	SetRect (&rClipZero, 0, 0, 0, 0);
	ClipRect (&rClipZero);

	// Figure out some basic geometry for the window
	buttonHeight = ControlHi (CtlCc);
	winWidth		 = RectWi (Win->contR);

	nickPrct = GetRLong (ALIAS_NICK_LIST_PRCT);

	if (Collapsed) {
		listWidth = winWidth - 4 * INSET;
		hTab1 = 2 * INSET;
		hTab2 = hTab1 + listWidth;
		hTab3 = Win->contR.right;
		hTab4 = hTab3;
		hTab5	= hTab3;
	}
	else {
		if (winWidth < win->minSize.h)
			winWidth = win->minSize.h;
		listWidth = winWidth * nickPrct / 100;
		if (listWidth < MinListWidth)
			listWidth = MinListWidth;
		if (winWidth - listWidth < MinTabWidth)
			listWidth = winWidth - MinTabWidth;
			
		hTab1 = 2 * INSET;
		hTab2 = hTab1 + listWidth;
		hTab3 = hTab2 + 2 * INSET;
		hTab4 = hTab3 + 2 * INSET;
		hTab5 = Win->contR.right - 2 * INSET;
	}
	vTab1 = Win->contR.top + INSET * 2;
	vTab2 = vTab1 + buttonHeight + INSET;
	vTab5 = Win->contR.bottom - 2 * INSET - buttonHeight;
	vTab3 = vTab5 - 2 * INSET - kHtCtl;
	vTab7 = vTab2;
	vTab6 = Win->contR.bottom - ControlHi (CtlDoNotSync) - 2 * INSET;
	
	// menu
	ButtonFit (CtlViewBy);
	MoveMyCntl (CtlViewBy, hTab1, vTab1, MIN (ControlWi (CtlViewBy), hTab2 - hTab1), 0);

	// list (we do this before the buttons because the list size may change
	SetRect (&r, hTab1, vTab2, hTab2, vTab3);
	LVSize (&NickList, &r, &htAdjustment);
	ListRect = NickList.bounds;
	
	vTab3 += htAdjustment;
	vTab4 = vTab3 + (vTab5 - vTab3 - kHtCtl) / 2;
	
	// buttons
	PositionBevelButtons (win, 3, &CtlNewNickname, hTab1, vTab4, kHtCtl, hTab3);
	PositionPushButtons (win, hTab1, hTab2, vTab5);

	// No need to reposition these when the window is horizontally collapsed
	if (!Collapsed) {
		// the vertical divider
		SetRect (&VertDivider, hTab3, vTab1, hTab3 + kVertDividerWidth, Win->contR.bottom - vTab1);
		
		// recipient checkbox
		MoveMyCntl (CtlRecipient, hTab5 - ControlWi (CtlRecipient), vTab1, 0, ONE_LINE_HI(win) );
		
		// do not sync checkbox
		MoveMyCntl (CtlDoNotSync, hTab4, vTab6 + INSET, 0, ControlHi (CtlDoNotSync));
		
		// the nickname field
		MoveLabelField (NickField, hTab4, vTab1, hTab5 - ControlWi (CtlRecipient) - 2 * INSET - hTab4, ONE_LINE_HI (win));
		GetControlBounds (NickField, &NickFieldRect);
		
		// content pane
		MoveTab (Tabs, hTab4, vTab7, hTab5 - hTab4, vTab6 - vTab7 - kAquaHeightAdjustment);
		GetControlBounds (Tabs, &TabRect);
	}

	// Need to do this here to catch wazoo promotions
	SetGreyControl (CtlZoomHorizontal, (IsWazoo (GetMyWindowWindowPtr (Win)) && !IsLonelyWazoo (GetMyWindowWindowPtr (Win))) || !win->isActive);

	//	restore clip region
	InfiniteClip (winPort);
	
	// redraw
	InvalContent (win);
}


/************************************************************************
 * ABClose - close the window
 ************************************************************************/
Boolean ABClose (MyWindowPtr win)

{
	NickStructHandle	theData;
	NickStructPtr			pData;
	short							ab,
										nick,
										numABs,
										numNicks;
	Boolean						discardChanges;
	
	discardChanges = false;
	if (AB.inited) {
		// Check to see if there's stuff we need to save
		if (IsDirtyWindow (win)) {
			// Engage in conversation with the user...
			switch (WannaSave (win)) {
				case WANNA_SAVE_CANCEL:
				case CANCEL_ITEM:
					return (false);;
					break;
				case WANNA_SAVE_DISCARD:
					discardChanges = true;
					break;
				case WANNA_SAVE_SAVE:
					if (!ABSave ())
						return (false);
					break;
			}
		}

		// Old crap, modified some...  Play with the deletion and dirty flags
		numABs = NAliases;
		for (ab = 0; ab < numABs; ab++) {
			if (theData = (*Aliases)[ab].theData) {
				numNicks = GetHandleSize_ (theData) / sizeof (NickStruct);
				for (nick = 0, pData = *theData; nick < numNicks; nick++, pData++)
					if (pData->addressesDirty) {
						pData->addressesDirty = false;
						pData->notesDirty			= false;
						pData->pornography		= false;
						if (pData->deleted && discardChanges)
							pData->deleted = false;
					}
			}
			// Get rid of the sort data for each address book
			ZapHandle ((*Aliases)[ab].sortData);
		}

		// Need to restore the recipient list if we're discarding changes
		VanquishRecipientList (discardChanges);
		
		// Save the list of expanded address books
		SaveExpandedAddressBookNames ();


		// Get rid of in-memory nickname info if we're discarding changes
		if (discardChanges) {
			ZapAliases ();
			RegenerateAllAliases (false);
		}
		
		// Dispose of list
		LVDispose (&NickList);
		// Dispose of any label fields
		DisposeLabelField (NickField);
		// Vanquish the tab control to memory hell
		DisposeTabControl (Tabs, false);
		
		AB.win		= nil;
		AB.inited = false;
	}
	return (true);
}


/************************************************************************
 * ABUpdate - draw the window
 ************************************************************************/
void ABUpdate (MyWindowPtr win)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	
	// list
	DrawThemeListBoxFrame (&NickList.bounds, kThemeStateActive);
	LVDraw (&NickList,MyGetPortVisibleRegion(winPort), false, false);

	// vertical divider
	if (!Collapsed)
		DrawDivider (&VertDivider, true);
}


/************************************************************************
 * ABActivate - activate the window
 ************************************************************************/
void ABActivate (MyWindowPtr win)

{
	short	ab,
				nick;
						
	// save the info on the tab whenever the address book goes inactive (which forces an update to the
	// current nickname if, for instance, we switch to a comp window and expect to do auto expand).
	if (!win->isActive)
		if (GetSelectedABNick (1, &ab, &nick))
			ReplaceNicknameData (Tabs, ab, nick);
	LVActivate (&NickList, win->isActive);
	TabActivate (Tabs, win->isActive);
	ABSetControls (win->isActive);
}


/**********************************************************************
 * ABFind - find in the Address Book window
 *
 *	� First, check to see if the text appears in the current edit field
 *	� If so, select it and we're done
 *	� If not, search the rest of the fields for this nickname starting
 *		with the next tab-able edit field.
 *	� When the text is found, select it.  If it is not found....
 *  � Step through the nickname list, testing each nickname to
 *		see if the search text appears in either the nickname, addresses
 *		or notes.
 *	� If a match is made, select that nickname in the list, then search
 *		for the text as above.
 **********************************************************************/
Boolean ABFind (MyWindowPtr win, PStr what)

{
	WindowPtr			winWP = GetMyWindowWindowPtr (win);
	PETEHandle		pte;
	PETEDocInfo		info;
	NickFoundType	found;
	long					offset;

	found = false;
	
	UserSelectWindow (winWP);
	SetPort (GetWindowPort (winWP));
	
	if (!win->isActive)
		ActivateMyWindow (winWP, true);

	// We're done if the text is found in the current PETE, so look for it!
	if (pte = win->pte) {
		PETEGetDocInfo (PETE, pte, &info);
		if ((offset = PeteFindString (what, info.selStop, pte)) >= 0) {
			PeteSelect (win, pte, offset, offset + *what);
			PeteScroll (pte, pseCenterSelection, pseCenterSelection);
			found = true;
		}
	}

	// darn!  The text is not in the current field.  Give the other fields associated with the selected nickname a chance
	if (!found)
		found = FindStringInSelectedNickname (what, false);
	
	// Okay, see if the search string is present in some other nickname
	if (!found)
		if (FindNextNicknameContainingString (win, what))
			found = FindStringInSelectedNickname (what, true);
	return (found);
}	


//
//	FindNextNicknameContainingString
//
//		Find (and select) the next nickname containing the search string.

NickFoundType FindNextNicknameContainingString (MyWindowPtr win, PStr what)

{
	VLNodeInfo		data;
	NickFoundType	found;
	short					ab,
								nick,
								endNick,
								abStop,
								nickStop,
								index;
	Boolean				wrapped,
								done,
								caseSens;

	// Start searching from the current selection
	if (LVGetItem (&NickList, 1, &data, true)) {
		// Make sure the current tab information is saved
		GetABNick (data.nodeID, &ab, &nick);
		ReplaceNicknameData (Tabs, ab, nick);
		index = data.rowNum + 1;
	}
	else
		index = 1;
	GetIndexedABNick (index, &ab, &nick);

	abStop		= ab;
	nickStop	= nick;
	endNick		= -1;			// Means scan to the end of the address book
	wrapped		= false;
	found			= nickFoundNothing;
	done			= false;
	caseSens	= PrefIsSet(PREF_SENSITIVE);

	LSetDrawingMode (false, NickList.hList);
	while (!found && !done) {
		if (CommandPeriod || EjectBuckaroo)
			break;
		if (!IsPluginAddressBook (ab))
			found = FindStringInAddressBook (ab, nick, &endNick, what, caseSens);
		if (!found)
			// Are we back where it all started?  If so, we be done.
			if (wrapped && ab == abStop && (endNick == nickStop || endNick == -1))
				done = true;
			else {
				// Advance to the next address book, maybe wrapping around back to the first one
				if (++ab == NAliases) {
					ab = 0;
					wrapped = true;
				}
				// Start at the first nickname and end... either with the last in the address book or the last after the wrap
				nick = -1;
				endNick = ab == abStop ? nickStop : -1;
			}
	}
	LSetDrawingMode (true, NickList.hList);

	// If we found something, display it (whoopee!), maybe expanding the address book
	if (found) {
		if ((*Aliases)[ab].collapsed) {
			LVGetItemWithNodeID (&NickList, MakeABNodeID (ab), &data);
			LVExpand (&NickList, MakeABNodeID (ab), data.name, true);
		}
		ABNickLVSelect (MakeNodeID (ab, endNick), false);
	}
	
	return (found);
}


//
//	FindStringInAddressBook
//
//		Fings a search string in an address book, starting from a particular nickname,
//		and returning the found nickname in the end index.
//
//		When -1 is passed for 'startNick' it means to scan the whole address book.
//

NickFoundType FindStringInAddressBook (short ab, short startNick, short *endNick, PStr what, Boolean caseSens)

{
	NickFoundType	found;
	short					*sort,
								count;
	Boolean				foundStartNick;

	found					 = nickFoundNothing;
	foundStartNick = startNick == -1;

	SortAddressBook (ab);
	if ((*Aliases)[ab].sortData && (*Aliases)[ab].theData) {
		count = GetHandleSize ((*Aliases)[ab].sortData) / sizeof (short);
		sort = LDRef ((*Aliases)[ab].sortData);
		while (count-- && !found && *sort != *endNick) {
			CycleBalls();
			if (CommandPeriod || EjectBuckaroo)
				break;
			if (*sort == startNick)
				foundStartNick = true;
			if (foundStartNick && !(*((*Aliases)[ab].theData))[*sort].deleted)
				if (found = FindStringInNickName (ab, *sort, what, caseSens))
					*endNick = *sort;
			++sort;
		}
		UL ((*Aliases)[ab].sortData);
	}
	return (found);
}


NickFoundType FindStringInNickName (short ab, short nick, PStr what, Boolean caseSens)

{
	AttributeValueHandle	avPairs;
	AttributeValuePtr			avPairPtr;
	NickFoundType					found;
	Str255								fieldValue;
	Handle								notes,
												addresses,
												leftovers;
	short									count,
												i;
	
	found			= nickFoundNothing;
	avPairs		= nil;
	leftovers = nil;
	GetNicknameNamePStr (ab, nick, fieldValue);
	// See if the search string is in the name
	if (SearchPtrPtr (what + 1, *what, fieldValue + 1, 0, *fieldValue, caseSens, false, nil) >=0)
		found = nickFoundNickname;
	else		// Didn't match on name, continue checking notes
		if (notes = GetNicknameData (ab, nick, false, true))
			if (SearchPtrHandle (what + 1, *what, notes, 0, caseSens, false, nil) >=0)
				if (avPairs = ParseAllAttributeValuePairs (notes, &leftovers, avAllSearchablePairs, avPairUnknown)) {
					avPairPtr = LDRef (avPairs);
					LDRef (notes);
					count = GetHandleSize (avPairs) / sizeof (AttributeValueRec);
					for (i = 0; i < count && !found; ++i, ++avPairPtr)
						if (SearchPtrPtr (what + 1, *what, *notes + avPairPtr->valueOffset, 0, avPairPtr->valueLength, caseSens, false, nil) >=0)
							found = nickFoundNotes;		// Found it in an attribute value pair (that is not hidden) -- so we can select this text in the proper tagged field
					UL (notes);
				}
	
	// Maybe the text is in the leftovers
	if (!found && leftovers)
		if (SearchPtrHandle (what + 1, *what, leftovers, 0, caseSens, false, nil) >=0)
			found = nickFoundNotes;
	
	// See if the search string is present in the addresses
	if (!found)
		if (addresses = GetNicknameData (ab, nick, true, true))
			if (SearchPtrHandle (what + 1, *what, addresses, 0, caseSens, false, nil) >=0)
				found = nickFoundAddresses;

	ZapHandle (avPairs);
	ZapHandle (leftovers);

	return (found);
}

//
//	FindStringInSelectedNickname
//
//		Find the next occurence of the search sting in the selected nickname.
//

Boolean FindStringInSelectedNickname (PStr what, Boolean checkNickField)

{
	PETEHandle	pte;
	long				offset;
	short				ab,
							nick,
							oldTabIndex;
	Boolean			found;
	
	found = false;			
	if (GetSelectedABNick (1, &ab, &nick)) {
		if (checkNickField) {
			// Remove the current focus
			ABClearKeyboardFocus ();
			if (pte = GetLabelFieldPete (NickField)) {
				if ((offset = PeteFindString (what, 0, pte)) >= 0) {
					// Switch to the first tab
					oldTabIndex = GetActiveTabIndex (Tabs);
					SetControlValue (Tabs, 1);
					CurrentTab = SwitchTabs (Tabs, oldTabIndex, GetActiveTabIndex (Tabs), PREF_LAST_NICK_TAB);
					// Move the focus to the nickname field
					ABSetKeyboardFocus (focusNickField);
					// Select the search string in the field
					PeteSelect (Win, pte, offset, offset + *what);
					PeteScroll (pte, pseCenterSelection, pseCenterSelection);
					found = true;
				}
			}
		}
		// Look for it in the tab
		if (!found)
			if (found = TabFindString (Tabs, NickFocus == focusTabControl, what))
				NickFocus = focusTabControl;
	}
	return (found);
}


#if 0
/**********************************************************************
 * ABFindInCollapsed - search a collapsed folder
 **********************************************************************/
Boolean ABFindInCollapsed (MyWindowPtr win, ViewListPtr pView, PStr what, VLNodeID nodeID)

{
	VLNodeInfo	data;
	CGrafPtr		winPort;
	Str31				name;
	short				totalNicks,
							nick,
							ab;
	
	ab = GetAddressBook (nodeID);
	totalNicks = (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct));
	for (nick = 0; nick < totalNicks; nick++)
		if (FindStrStr (what, GetNicknameNamePStr (ab, nick, name)) >= 0) {
			LVGetItemWithNodeID (&NickList, MakeABNodeID (ab), &data);
			LVExpand (&NickList, MakeABNodeID (ab), data.name, true);
			LVSelect (pView, MakeNodeID (ab, nick), name, false);
			winPort = GetMyWindowCGrafPtr (win);
			LVDraw(pView,winPort->visRgn,true,false);				
			return true;
		}
	return false;
}
#endif

Boolean ABDirty (MyWindowPtr win)

{
	return (win->isDirty || AnyNicknamesDirty (kAddressBookUndefined) || (Tabs && IsTabDirty (Tabs)));
}


Boolean ABHasSelection (MyWindowPtr win)

{
	UHandle text;
	long		start,
					stop;
	
	if (LVCountSelection (&NickList))
		return (true);
	if (ValidNickname (AB.nickDisplayed) && TabHasSelection (Tabs))
		return (true);
	if (win->pte && !PeteGetTextAndSelection (win->pte, &text, &start, &stop))
		return (stop!=start);
	return (false);
}


void ABMenuEnable (MyWindowPtr win)

{
	if (NickFocus == focusTabControl && ValidNickname (AB.nickDisplayed))
		TabMenuEnable (Tabs, (*Aliases)[AB.abDisplayed].ro);
}


void ABSetKeyboardFocus (NickFocusType nickFocus)

{
	// If the focus is not changing, why bother, ya know?
	if (nickFocus == NickFocus)
		return;
	
	// First, clear the old focus
	if (!ABClearKeyboardFocus ())
		return;

	// Next, set the new focus
	switch (NickFocus = nickFocus) {
		case focusNickList:
			LVSetKeyboardFocus (&NickList, true);
			break;
		case focusNickField:
			SetKeyboardFocus (GetMyWindowWindowPtr (Win), NickField, kControlEditTextPart);
			PeteFocus(Win,GetLabelFieldPete(NickField),true);
			break;
		case focusTabControl:
			TabSetKeyboardFocus (Tabs, focusTabControlDirect, true);
			break;
		case focusNickTabAdvance:
			TabSetKeyboardFocus (Tabs, focusTabControlAdvance, true);
			NickFocus = focusTabControl;
			break;
		case focusNickTabReverse:
			TabSetKeyboardFocus (Tabs, focusTabControlReverse, true);
			NickFocus = focusTabControl;
			break;
	}
}

Boolean ABClearKeyboardFocus (void)

{
	switch (NickFocus) {
		case focusNickList:
			LVSetKeyboardFocus (&NickList, false);
			break;
		case focusNickField:
			if (!ABMaybeRenameNickname (Win))
				return (false);
			SetKeyboardFocus (GetMyWindowWindowPtr (Win), NickField, kControlFocusNoPart);
			PeteFocus(Win,GetLabelFieldPete(NickField),false);
			break;
		case focusTabControl:
			TabSetKeyboardFocus (Tabs, focusTabControlDirect, false);
			break;
	}
	NickFocus = focusNoChange;
	
	if (Win->pte) PeteFocus(Win,Win->pte,false);
	
	return (true);
}

void ABAdvanceKeyboardFocus (void)

{
	NickFocusType newFocus;

	// If the window is collapsed, there's nowhere for the focus to go
	if (Collapsed)
		return;
	
	newFocus = focusNoChange;
	switch (NickFocus) {
		case focusNickList:
			// Advance the focus within the nickname list, if we don't want the focus anymore, give it to the nickname field
			if (ValidNickname (AB.nickDisplayed))
				if (LVAdvanceKeyboardFocus (&NickList))
					newFocus = focusNickField;
			break;
		case focusNickField:
			// Advance the focus to the Tab control
			newFocus = focusNickTabAdvance;
			break;
		case focusTabControl:
			// Advance the focus within the Tab control, if we don't want the focus anymore, give it to the nickname list
			if (TabAdvanceKeyboardFocus (Tabs))
				newFocus = focusNickList;
			break;
	}
	// If the focus is changing, see if the new item is willing to accept the focus, if not we advance again
	if (newFocus != focusNoChange)
		ABSetKeyboardFocus (newFocus);
}

void ABReverseKeyboardFocus (void)

{
	NickFocusType newFocus;
	
	newFocus = focusNoChange;
	switch (NickFocus) {
		case focusNickList:
			// Reverse the focus within the nickname list, if we don't want the focus anymore, give it to the Tab control
			if (ValidNickname (AB.nickDisplayed))
				if (LVReverseKeyboardFocus (&NickList))
					newFocus = focusNickTabReverse;
			break;
		case focusNickField:
			// Reverse the focus to the nickname list
			newFocus = focusNickList;
			break;
		case focusTabControl:
			// Reverse the focus within the Tab control, if we don't want the focus anymore, give it to the nickname field
			if (TabReverseKeyboardFocus (Tabs))
				newFocus = focusNickField;
			break;
	}
	// If the focus is changing, see if the new item is willing to accept the focus, if not we reverse again
	if (newFocus != focusNoChange)
		ABSetKeyboardFocus (newFocus);
}


/************************************************************************
 * ABKey - key stroke
 ************************************************************************/
Boolean ABKey (MyWindowPtr win, EventRecord *event)
{
	short		key = (event->message & 0xff),
					newTabIndex;
	Boolean	result;

	result = false;
	// Where is the focus?
	switch (NickFocus) {
		case focusNickList:
			result = LVKey (&NickList, event);
			break;
		case focusTabControl:
			result = TabKey (Tabs, event);
			break;
	}
	if (!result || key == tabChar)
		switch (key) {
			case tabChar:
				if (!Collapsed) {
					if (event->modifiers & optionKey) {
						newTabIndex = GetActiveTabIndex (Tabs) + (event->modifiers & shiftKey ? -1 : 1);
						newTabIndex = newTabIndex ? (newTabIndex > GetTabPaneCount (Tabs) ? 1 : newTabIndex) : (GetTabPaneCount (Tabs));
						SetControlValue (Tabs, newTabIndex);
						// Pretend we clicked on this tab to do all the right switching of fields
						ABHit (win, event, abTabs, 0);
					}
					else {
						if (event->modifiers & shiftKey)
							ABReverseKeyboardFocus ();
						else
							ABAdvanceKeyboardFocus ();
						if (win->pte)
							PeteSelectAll (win, win->pte);
					}
					result = true;
				}
				break;
			default:
				if (!(event->modifiers & cmdKey)) {
					if (DirtyKey (key) && ValidAddressBook (AB.abDisplayed) && (*Aliases)[AB.abDisplayed].ro)
						AlertStr (READ_ONLY_ALRT, Stop, nil);
					else {
						event->what = keyDown;
						(void) PeteEdit (win, win->pte, peeEvent, (void*) event); // (jp) For nickname completion
					}
					result = true;
				}
		}
	return (result);
}

/************************************************************************
 * ABClick - click in nickname window
 ************************************************************************/
void ABClick (MyWindowPtr win, EventRecord *event)

{
	NickFocusType	oldNickFocus;
	WindowPtr			WinWP = GetMyWindowWindowPtr (Win);
	Point					pt;
	short					part,
								i;
	Rect					tabRect;
	
	SetPort (GetMyWindowCGrafPtr (win));
	pt = event->where;
	GlobalToLocal (&pt);

	// First, check to see if the click was in the list while the PETE was not.  When this is the
	// case we need to remove the focus before handing the click to the list view.  We'll also check
	// to see if we should rename the current nickname (as the ListView is kinda picky about the
	// selection and sort states).
	if (PtInRect (pt, &NickList.bounds)) {
		if (!ABMaybeRenameNickname (win))
			return;
		ABSetKeyboardFocus (focusNickList);
	}
	
	// Now, let's give the list view a shot at the click
	if (!LVClick (&NickList, event)) {
		if (!win->isActive) {
			SelectWindow_ (WinWP);
			UpdateMyWindow (WinWP);	//	Have to update manually since no events are processed
		}

		// tab content (since the toolbox is not nice enough to tell about clicks in the content... sheesh)
		GetTabContentRect (Tabs, &tabRect);
		if (PtInRect (pt, &tabRect) && ValidNickname (AB.nickDisplayed)) {
			oldNickFocus = NickFocus;
			if (NickFocus != focusTabControl)
				if (!ABClearKeyboardFocus ())
					return;
			if (ClickTab (Tabs, event, pt))
				NickFocus = focusTabControl;
			else
				ABSetKeyboardFocus (oldNickFocus);
		}
		else {
			// Let's look at the controls we know about
			for (i = 0; i < abControlCount; i++)
				if (part = TestControl (Controls[i], pt)) {
					// For now we're going to intercept mouse downs in label fields and pass them down to our hit routine.
					// Eventually it would be cool (and maybe easy) to have the label field user pane respond to tracking messages
					// We also have to qluify the part test against 'kControlEditTextPart' to not be a hit in a tab control
					// because the tab control returns the index og the hit tab in  the part... and since we have 5 tabs, clicks
					// in the Notes field were being swallowed by the part==kControlEditTextPart test.
					if (!ControlIsGrey (Controls[i]))
						if ((part == kControlEditTextPart && Controls[i] != Tabs) || TrackControl (Controls[i], pt, (void *)(-1))) {
							ABHit (win, event, i, part);
							AuditHit ((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, GetWindowKind(WinWP), AUDITCONTROLID(GetWindowKind(WinWP),i), event->what);	
							break;
						}
					}
			
			// If we did not hit a control
			if (i == abControlCount) {
				// The vertical divider, by chance?
				if (!Collapsed && PtInSloppyRect (pt, &VertDivider, 1))
					NickWDragVDivider (pt);
			}
		}
	}
	ABSetControls (win->isActive);
}


/************************************************************************
 * ABCursor - set the cursor properly for the address book window
 ************************************************************************/
void ABCursor (Point mouse)

{
	int	cursor;
		
	if (!PeteCursorList (Win->pteList, mouse)) {
		cursor = arrowCursor;
		if (PtInSlopRect (mouse, VertDivider, 1))
			cursor = DIVIDER_CURS;
		SetMyCursor (cursor);
	}
}


/************************************************************************
 * ABSetControls - Set the controls based on the state of the selected
 * information in the list.
 ************************************************************************/
void ABSetControls (Boolean isActive)

{
	short		ab,
					nick,
					count;
	Boolean	canAddNickname,
					canAddAddressBook,
					canRemove,
					anyNicksSelected,
					singleNickSelected,
					greyTab;

	// Here's a neato oddity of the toolbox...  the active state of a control becomes
	// hosed if your activate or deactivate a control while the mouse button is down...
	// even though the mouse might not be down IN that particulat control.  Who knew?
	if (StillDown ())
		return;
	
	count = ABDetermineSelectionConditions (&canAddNickname, &canAddAddressBook, &canRemove, &anyNicksSelected, &singleNickSelected);

	SetGreyControl (CtlNewNickname, !canAddNickname || !isActive);	
	SetGreyControl (CtlNewAddressBook, !canAddAddressBook || !isActive);
	SetGreyControl (CtlRemove, !canRemove || !isActive);	
	SetGreyControl (CtlTo, !anyNicksSelected || !isActive);	
	SetGreyControl (CtlCc, !anyNicksSelected || !isActive);	
	SetGreyControl (CtlBcc, !anyNicksSelected || !isActive);
	SetGreyControl (CtlChat, !singleNickSelected || !isActive || !ABIsChattable());
	SetGreyControl (CtlRecipient, !anyNicksSelected || !isActive);
	SetGreyControl (CtlZoomHorizontal, !isActive);
	
	if (IsWazoo (GetMyWindowWindowPtr (Win)) && !IsLonelyWazoo (GetMyWindowWindowPtr (Win)))
		HideControl(CtlZoomHorizontal);
	else
		ShowControl(CtlZoomHorizontal);

	// nickname field and tabs
	if (count == 1)
		GetSelectedABNick (1, &ab, &nick);
	else {
		ab	 = kAddressBookUndefined;
		nick = kNickUndefined;
	}
	SetGreyControl (CtlDoNotSync, !anyNicksSelected || (ab != kAddressBookUndefined && (*Aliases)[ab].ro) || !isActive );
	SetGreyControl (NickField, 	!singleNickSelected || (ab != kAddressBookUndefined && (*Aliases)[ab].ro) || !isActive);
	ABSetRecipientButton ();
	ABSetSyncButton ();

	greyTab = !singleNickSelected || (ab != kAddressBookUndefined && (*Aliases)[ab].ro) || !isActive;
	IterateTabPanes (Tabs, SetGreyTabProc, &greyTab);
	Win->hasSelection = anyNicksSelected || MyWinHasSelection (Win);
}


//
//	ABDetermineSelectionConditions
//
//		Takes a peek at various nickname selection criteria so that we can know which buttons need to be
//		deactivated -- and maybe why.
//
//		Returns the number of items currently selected.
//
short ABDetermineSelectionConditions (Boolean *canAddNickname, Boolean *canAddAddressBook, Boolean *canRemove, Boolean *anyNicksSelected, Boolean *singleNickSelected)

{
	short	count,
				ab,
				nick,
				i;
	
	*canAddNickname			= true;
	*canAddAddressBook	= HasFeature (featureMultipleNicknameFiles);
	*canRemove					= LVCountSelection (&NickList) ? true : false;
	*anyNicksSelected		= false;
	*singleNickSelected = false;
	count = LVCountSelection (&NickList);
	// Take a trip through all the selections looking for relevant conditions
	for (i = 1; i <= count; ++i)
		if (GetSelectedABNick (i, &ab, &nick))
			if (ab >= 0) {
				if ((*Aliases)[ab].ro) {
					*canRemove			 = false;
					*canAddNickname = false;
				}
				if (nick == kNickUndefined) {
#ifdef VCARD
					if (IsEudoraAddressBook (ab) || IsHistoryAddressBook (ab) || IsPersonalAddressBook (ab))
#else
					if (IsEudoraAddressBook (ab) || IsHistoryAddressBook (ab))
#endif
						*canRemove = false;
				}
				else {
					*anyNicksSelected = true;
					if (count == 1)
						*singleNickSelected = true;
				}
			}
	return (count);
}


Boolean SetGreyTabProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, Boolean *shdBeGrey)

{
	SetGreyControl (tabPane, *shdBeGrey);
	return (false);
}


void ABSelectNickname (VLNodeID nodeID)

{
	// On a selection during a drag, set the nick selection to be nothing
	if (LVDragSelectInProgress (&NickList))
		nodeID = MakeNodeID (kAddressBookUndefined, kNickUndefined);

	if (ABDisplayNicknameInTab (GetAddressBook (nodeID), GetNickname (nodeID)))
		ABSetControls (Win->isActive);
}


void ABUnselectNickname (VLNodeID nodeID)

{
	short	ab,
				nick,
				count;
				
	ab	 = GetAddressBook (nodeID);
	nick = GetNickname (nodeID);

	// If we are unselecting the _displayed_ nickname, replace its data
	ReplaceNicknameData (Tabs, ab, nick);

	// When individual nicknames are unselected, check to see if any are selected at all.
	// If not, we'll deactivate controls.  (Note:  we might want to instead use this:
	count = LVCountSelection (&NickList);
	if (!count) {
		if (ABDisplayNicknameInTab (kAddressBookUndefined, kNickUndefined))
			ABSetControls (false);
	}
	else
		// If by unselecting a name we now have exactly one name selected.... display it
		if (count == 1) {
			GetSelectedABNick (1, &ab, &nick);
			if (ABDisplayNicknameInTab (ab, nick))
				ABSetControls (true);
		}
}


void ABSetRecipientButton (void)

{
	short		ab,
					nick,
					count,
					value,
					i;
	Boolean	isRecipient;
	
	value = -1;
	// We're going to loop through the selected list items to see whether or not we have a
	// mix of recipient list settings (so that we can set the control to its "mixed" setting).
	count = LVCountSelection (&NickList);
	for (i = 1; i <= count && value != kControlCheckBoxMixedValue; ++i) {
		GetSelectedABNick (i, &ab, &nick);
		if (nick != kNickUndefined) {
			isRecipient = IsNicknameOnRecipList (ab, nick);
			if (value == -1)
				value = isRecipient ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue;
			else
				if ((value == kControlCheckBoxCheckedValue && !isRecipient) || (value == kControlCheckBoxUncheckedValue && isRecipient))
					value = kControlCheckBoxMixedValue;
		}
	}
	SetControlValue (CtlRecipient, value);
}


void ABSetSyncButton (void)

{
	short		ab,
					nick,
					count,
					value,
					i;
	Boolean	isPrivate;
	
	value = -1;
	// We're going to loop through the selected list items to see whether or not we have a
	// mix of recipient list settings (so that we can set the control to its "mixed" setting).
	count = LVCountSelection (&NickList);
	for (i = 1; i <= count && value != kControlCheckBoxMixedValue; ++i) {
		GetSelectedABNick (i, &ab, &nick);
		if (nick != kNickUndefined) {
			isPrivate = IsNicknamePrivate (ab, nick);
			if (value == -1)
				value = isPrivate ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue;
			else
				if ((value == kControlCheckBoxCheckedValue && !isPrivate) || (value == kControlCheckBoxUncheckedValue && isPrivate))
					value = kControlCheckBoxMixedValue;
		}
	}
	SetControlValue (CtlDoNotSync, value);
}


/************************************************************************
 * ABHit - a control item was hit
 ************************************************************************/

void ABHit (MyWindowPtr win, EventRecord *event, short whichControl, short part)

{
	// Certain control hits should be preflighted with a possible nickname field save
	if (whichControl == abViewByPopup						||
			whichControl == abNewNicknameButton 		||
			whichControl == abNewAddressBookButton	||
			whichControl == abToButton							||
			whichControl == abCcButton							||
			whichControl == abBccButton							||
			whichControl == abChatButton						||
			whichControl == abRecipientCheckbox)
		if (!ABMaybeRenameNickname (win))
			return;
		
	switch (whichControl) {
		case abViewByPopup:
			DoViewByMenu ();
			break;

		case abNewNicknameButton:
			DoNewNickname (event->modifiers);
			break;
			
		case abNewAddressBookButton:
			DoNewAddressBook ();
			break;

		case abRemoveButton:
			ABRemove ();
			break;
		
		case abToButton:
			ABMessageTo (TO_HEAD, event->modifiers);
			break;
			
		case abCcButton:
			ABMessageTo (CC_HEAD, event->modifiers);
			break;
			
		case abChatButton:
			ABChat (event->modifiers);
			break;
			
		case abBccButton:
			ABMessageTo (BCC_HEAD, event->modifiers);
			break;
			
		case abZoomHorizontalButton:
			DoHorizontalZoom (win);
			break;
		
		case abRecipientCheckbox:
			DoRecipientList (CtlRecipient);
			break;
			
		case abDoNotSyncCheckbox:
			DoDoNotSync (CtlDoNotSync);
			break;
			
		case abNickField:
			if (part == kControlEditTextPart) {
				ABSetKeyboardFocus (focusNickField);
				PeteEdit (win, GetLabelFieldPete (NickField), peeEvent, (void*) event);
			}
			break;
			
		case abTabs:
			// Remove the focus from the current tab...
			if (NickFocus == focusTabControl)
				TabSetKeyboardFocus (Tabs, focusTabControlDirect, false);

			CurrentTab = SwitchTabs (Tabs, CurrentTab, GetActiveTabIndex (Tabs), PREF_LAST_NICK_TAB);
			
			/// ...and put it back onto the first field of the new tab
			if (NickFocus == focusTabControl)
				TabSetKeyboardFocus (Tabs, focusTabControlDirect, ValidNickname (AB.nickDisplayed));
			break;
	}
}


void ABMessageTo (short strResID, long modifiers)

{
	WindowPtr	WinWP = GetMyWindowWindowPtr(Win);
	short			ab,
						nick;
	MyWindowPtr insertedWin;
						
	// save the info on the tab whenever a recipient button has been clicked
	if (GetSelectedABNick (1, &ab, &nick))
		ReplaceNicknameData (Tabs, ab, nick);

	insertedWin = InsertTheAlias (strResID, (modifiers & optionKey) != 0);
	if ((modifiers & (shiftKey | cmdKey))) {
		if (FrontWindow_ () != WinWP)
			UserSelectWindow (WinWP);
	}
	else
		if (FrontWindow_() == WinWP) {
			UserSelectWindow(GetMyWindowWindowPtr(insertedWin));
			UpdateMyWindow (WinWP);
		}
}


/************************************************************************
 * ABHelp - help for the alias window
 ************************************************************************/
void ABHelp (MyWindowPtr win,Point mouse)
{
	NickHelpType	help;
	Rect					tipRect;
	short					i,
								ab,
								nick;
	Boolean				canAddNickname,
								canAddAddressBook,
								canRemove,
								anyNicksSelected,
								singleNickSelected;
	Rect			cntlRect;
	
	ABDetermineSelectionConditions (&canAddNickname, &canAddAddressBook, &canRemove, &anyNicksSelected, &singleNickSelected);

	help = noNickHelp;
	if (PtInRect (mouse, &NickList.bounds)) {
		tipRect = NickList.bounds;
		help = nickHelpList;
	}
	else
	if (PtInSloppyRect (mouse, &VertDivider, 1)) {
		tipRect = VertDivider;
		help = nickHelpVertDivider;
	}
	else
		for (i = 0; help == noNickHelp && i < abControlCount; i++)
			if (PtInRect (mouse, GetControlBounds(Controls[i],&cntlRect))) {
				GetControlBounds(Controls[i],&tipRect);
				switch (i) {
					case abViewByPopup:
						help = nickHelpViewByPopup;
						break;
					case abNewNicknameButton:
						help = canAddAddressBook ? nickHelpNewNicknameButton : nickHelpNewNicknameButtonDimmed;
						break;
					case abNewAddressBookButton:
						help = nickHelpNewAddressBookButton;
						break;
					case abRemoveButton:
						if (canRemove)
							help = nickHelpRemoveButton;
						else {
							help = nickHelpRemoveButtonDimmed;
							if (GetSelectedABNick (i, &ab, &nick))
								if (ab >= 0) {
									if ((*Aliases)[ab].ro)
										help = nickHelpRemoveButtonDimmedReadOnly;
									else
										if (nick == kNickUndefined)
											if (IsEudoraAddressBook (ab))
												help = nickHelpRemoveButtonDimmedEudoraNicknames;
											else
												if (IsHistoryAddressBook (ab))
													help = nickHelpRemoveButtonDimmedHistoryList;
								}
						}
						break;
					case abToButton:
						help = anyNicksSelected ? (TopCompositionWindow (true, true) ? nickHelpToButtonAddToTopmost : nickHelpToButton) : nickHelpToButtonDimmed;
						break;
					case abCcButton:
						help = anyNicksSelected ? (TopCompositionWindow (true, true) ? nickHelpCcButtonAddToTopmost : nickHelpCcButton) : nickHelpCcButtonDimmed;
						break;
					case abBccButton:
						help = anyNicksSelected ? (TopCompositionWindow (true, true) ? nickHelpBccButtonAddToTopmost : nickHelpBccButton) : nickHelpBccButtonDimmed;
						break;
					case abZoomHorizontalButton:
						help = Collapsed ? nickHelpZoomHorizontalButtonCollapsed : nickHelpZoomHorizontalButtonExpanded;
						break;
					case abRecipientCheckbox:
						help = anyNicksSelected ? nickHelpRecipientCheckbox : nickHelpRecipientCheckboxDimmed;
						break;
					case abNickField:
						help = anyNicksSelected ? (singleNickSelected ? nickHelpNickField : nickHelpNickFieldDimmedMultipleSelection) : nickHelpNickFieldDimmedNothingSelected;
						break;
					case abTabs:
						help = anyNicksSelected ? (singleNickSelected ? nickHelpTabs : nickHelpTabsDimmedMultipleSelection) : nickHelpTabsDimmedNothingSelected;
						break;
				}
			}
				
	if (help)
		MyBalloon (&tipRect, 100, 0, NICK_HELP_STRN + help, 0, nil);
}


/************************************************************************
 * ABMenu - menu choice in the alias window
 ************************************************************************/
Boolean ABMenu (MyWindowPtr win, int menu, int item, short modifiers)
{
	TabObjectPtr			objectPtr;
	SendDragDataInfo	sendDragData;
	Str255						tag,
										name;
	Handle						copyText;
	OSErr							theError;
	short							ab,
										nick;
	ScrapRef	scrap;
	
	if (!TabMenu (Tabs, menu, item, modifiers))
		switch (menu)	{
			case FILE_MENU:
				switch (item) {
					case FILE_SAVE_ITEM:
						if (IsDirtyWindow (win))
							ABSave ();
						return (True);
						break;
					case FILE_SAVE_AS_ITEM:
						DoAddressBookExport ((modifiers&shiftKey)!=0);
						break;
					case FILE_PRINT_ITEM:
					case FILE_PRINT_ONE_ITEM:
							PrintAddressBookWindow((modifiers&shiftKey)!=0, item==FILE_PRINT_ONE_ITEM);
						break;
				}
				break;
			case EDIT_MENU:
				if (win->pte) {
					PETEHandle	pteEmail = nil,
											pteOtherEmail = nil;
					
					DoIndependentMenu (menu, item, modifiers);
					win->hasSelection = MyWinHasSelection (win);
					if (objectPtr = FindObjectWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagEmail))) {
						pteEmail = GetLabelFieldPete (objectPtr->control);
						UnlockObject (objectPtr);
					}
					if (objectPtr = FindObjectWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagOtherEmail))) {
						pteOtherEmail = GetLabelFieldPete (objectPtr->control);
						UnlockObject (objectPtr);
					}
					if ((item == EDIT_COPY_ITEM || item == EDIT_CUT_ITEM) && (win->pte == pteEmail || win->pte == pteOtherEmail))
						ReformatClip();
					return (true);
				}
				else
					switch (item) {
						case EDIT_SELECT_ITEM:
							if (NickFocus == focusNickList)
								LVSelectAll (&NickList);
							break;
						case EDIT_CLEAR_ITEM:
							if (NickFocus == focusNickList)
								ABRemove();
							break;
						case EDIT_COPY_ITEM:
							Zero (sendDragData);
							sendDragData.flavor = A822_FLAVOR;
							theError = BuildDragData (&sendDragData, &copyText);
							if (!theError) {
								ClearCurrentScrap();
								GetCurrentScrap(&scrap);
								theError = PutScrapFlavor(scrap,'TEXT',kScrapFlavorMaskNone,GetHandleSize (copyText),LDRef (copyText));
							}
							ZapHandle (copyText);
							break;
					}
				break;
			case SPECIAL_MENU:
				switch (AdjustSpecialMenuSelection(item)) {
					case SPECIAL_MAKE_NICK_ITEM:
						if (modifiers&shiftKey)
							MakeNickFromSelection(win);
						else
							MakeNicknameFromSelectedNicknames ();
						break;
				}
				break;
			case WINDOW_MENU:
				if (item == WIN_PH_ITEM && (modifiers & shiftKey) && LVCountSelection (&NickList) == 1) {
					if (GetSelectedABNick (1, &ab, &nick)) {
						GetTaggedFieldValueStr (ab, nick, GetRString (tag, ABReservedTagsStrn + abTagName), name);
						if (*name) {
							OpenPh (name);
							return (true);
						}
					}
				}
				break;
		}
	return (false);
}

Boolean ABSave (void)

{
	short		ab,
					nick;
	Boolean	bornAgain;
	
	// Does the nickname field need to be saved before we save?		
	if (!ABMaybeRenameNickname (Win))
		return (false);
		
	// Make sure the current tab information is saved
	if (GetSelectedABNick (1, &ab, &nick))
		ReplaceNicknameData (Tabs, ab, nick);

	if (bornAgain = SaveAliases (true))
		ABClean ();

	return (bornAgain);
}


void ABTickle (void)

{
	short	abDisplayed,
				nickDisplayed;
	if (AB.inited) {
		abDisplayed		= AB.abDisplayed;
		nickDisplayed	= AB.nickDisplayed;
		InvalidListView (&NickList);
		// Reselect the previous items
		if (!(abDisplayed == kAddressBookUndefined && nickDisplayed == kNickUndefined))
			ABNickLVSelect (MakeNodeID (abDisplayed, nickDisplayed), true);
		TickleMe = false;
	}
}

void ABTickleHardEnoughToMakeYouPuke(void)
{
	if (AliasWinIsOpen())
	{
		short which;
		
		for (which=0;which<NAliases;which++)
		{
			ZapHandle((*Aliases)[which].sortData);
			SortAddressBook(which);
		}
		ABTickle();
	}
}

void MakeNicknameFromSelectedNicknames (void)

{
	Handle	addresses;
	Str32		name;						// Note, this is one extra character to accomodate a null at the end (as is done in old nickwin.c)
	OSErr		theError;
	short		ab,
					nick,
					count,
					i;
	
	addresses = NuHandle (0);
	theError = MemError ();
	count = LVCountSelection (&NickList);
	for (i = 1; i <= count && !theError; ++i) {
		GetSelectedABNick (i, &ab, &nick);
		if (nick != kNickUndefined) {
			GetNicknameNamePStr (ab, nick, name);
			name[name[0]+1] = 0;
			theError = PtrPlusHand_ (name, addresses, *name + 2);
		}
	}
	if (!theError)
		theError = PtrPlusHand_ ("", addresses, 1);
	if (!theError)
#ifdef VCARD
		NewNick (addresses, nil, 0);
#else
		NewNick (addresses, 0);
#endif
	ZapHandle(addresses);

	if (theError)
		WarnUser (COULDNT_MOD_ALIAS, theError);
}


//
//	ABIdle
//
//		We need an idle handler for the address book in order for the label field controls
//		to get time for nickname highlighting
//

void ABIdle (MyWindowPtr win)

{
	TabObjectPtr	objectPtr;
	PETEHandle		pte;
	Str255				tag;

	if (win)
	{
		if (TickleMe) ABTickle();
		if (objectPtr = FindObjectWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagEmail))) {
			pte = GetLabelFieldPete (objectPtr->control);
			if (win->pte == pte)
				IdleControls (GetMyWindowWindowPtr (win));
			UnlockObject (objectPtr);
		}
		if (NickFocus==focusNickField)
		{
			Boolean singleNickSelected,ratSass;
			
			ABDetermineSelectionConditions(&ratSass, &ratSass, &ratSass, &ratSass, &singleNickSelected);
			if (!singleNickSelected) ABClearKeyboardFocus();
		}
	}
}


/**********************************************************************
 * ABDragHandler - handle drags
 *
 *  Can do internal dragging of nicknames or external dragging and dropping
 *  of messages
 **********************************************************************/
OSErr ABDragHandler (MyWindowPtr win, DragTrackingMessage which, DragReference drag)

{
	VLNodeInfo	targetInfo;
	OSErr				theError;
	Point				initialMouse,
							currentMouse;
	short				ab,
							nick;
#ifdef VCARD
	short				modifiers,
							mouseDownModifiers,
							mouseUpModifiers;
	Boolean			isVCardDrag,
							isSpecDrag;
#endif
	Boolean			isListItemDrag,
							isDragInList,
							is822Drag,
							isTextDrag,
							isGraphicFileDrag,
							isNickDrag,
							isHFSDrag,
							isTabDrag;

	theError = dragNotAcceptedErr;

	is822Drag	 = DragIsInteresting (drag, A822_FLAVOR, nil);
	isTextDrag = DragIsInteresting (drag, 'TEXT', nil);
	isNickDrag = DragIsInteresting (drag, kNickDragType, nil);
	isHFSDrag	 = DragIsInteresting (drag, flavorTypeHFS, nil);
#ifdef VCARD
	isSpecDrag = DragIsInteresting (drag, SPEC_FLAVOR, nil) && IsVCardAvailable ();
#endif
	isTabDrag	 = TabDragIsInteresting (Tabs, drag);

	isListItemDrag = false;
	// Only check the drag origin if the drag started with this window
	if (DragSourceKind == GetWindowKind(GetMyWindowWindowPtr(win)))
		if (!GetDragOrigin (drag, &initialMouse)) {
			GlobalToLocal (&initialMouse);
			isListItemDrag	= PtInRect (initialMouse, &NickList.bounds);
		}

	isDragInList = false;
	if (!GetDragMouse (drag, &currentMouse, nil)) {
		GlobalToLocal (&currentMouse);
		isDragInList	= PtInRect (currentMouse, &NickList.bounds);
	}

	if (isTabDrag = (isTabDrag && LVCountSelection (&NickList) == 1)) {
		GetSelectedABNick (1, &ab, &nick);
		isTabDrag = ValidNickname (nick);
	}

	// Calling IsGraphicFile is ssssssssssllllllloooooooowwwwwww.
	// We could speed it up with a global.
	isGraphicFileDrag = false;
#ifdef VCARD
	isVCardDrag				= false;
#endif
	if (isHFSDrag) {
		FSSpec			spec;
		HFSFlavor		**dragData;
		
		dragData = nil;
		if (!MyGetDragItemData (drag, 1, flavorTypeHFS, &dragData)) {
			spec = (*dragData)->fileSpec;
			isGraphicFileDrag = IsGraphicFile (&spec);
#ifdef VCARD
			if (IsVCardAvailable ())
				isVCardDrag = IsVCardFile (&spec);
#endif
		}
		ZapHandle (dragData);
	}

#ifdef VCARD
	if (!is822Drag && !isTextDrag && !isNickDrag && !isHFSDrag && !isListItemDrag && !isTabDrag && !isSpecDrag)
		return (dragNotAcceptedErr);	//	Nothing here we want
#else
	if (!is822Drag && !isTextDrag && !isNickDrag && !isHFSDrag && !isListItemDrag && !isTabDrag)
		return (dragNotAcceptedErr);	//	Nothing here we want
#endif

	// If we're dragging TEXT that is not also a drag that originated in the list itself, check
	// to see if there is exactly one nickname selected in the list.  If so, the addres book can
	// accept the drag and we return 'dragNotAcceptedErr' (yes, that's right... NOT accepted) so
	// that PETE handles the drag.  If there is no list selection, or there are multiple selections
	// then the nickname fields are disabled and we return 'noErr' to make PETE think we were smart
	// enough to handle the drag ourselves.
	if (isTextDrag && !isListItemDrag && !isDragInList) {
		if (LVCountSelection (&NickList) == 1) {
			GetSelectedABNick (1, &ab, &nick);
			if (ValidNickname (nick))
				return (dragNotAcceptedErr);
		}
		return (noErr);
	}
	
#ifdef VCARD
	// If we are dragging a SPEC_FLAVOR that originated with ourselves, we're dragging a personal nickname
	// as a vCard.  We'll allow this ONLY if a copy is taking place.
	GetDragModifiers (drag, &modifiers, &mouseDownModifiers, &mouseUpModifiers);
	if (isSpecDrag && isListItemDrag && !((modifiers|mouseDownModifiers|mouseUpModifiers)&optionKey))
		return (dragNotAcceptedErr);
#endif
		
	switch (which) {
		case kDragTrackingEnterWindow:
		case kDragTrackingLeaveWindow:
		case kDragTrackingInWindow:
			if (isNickDrag || (isHFSDrag && !isGraphicFileDrag) || (isListItemDrag && !isTextDrag))
				theError = LVDrag (&NickList, which, drag);
			else
				if (isTabDrag)
					theError = TabDrag (Tabs, which, drag);
			break;
		case 0xfff:
			//	Plop
			if (isTabDrag)
				theError = TabDrag (Tabs, which, drag);
			else
			if (isListItemDrag)
				theError = LVDrag (&NickList, which, drag);
			else
			if (LVDrop (&NickList, &targetInfo)) {
#ifdef VCARD
				if (isHFSDrag && IsVCardAvailable ())
					theError = ABFileDrop (drag, &targetInfo);
				else
#endif
					theError = ABNickDrop (drag, &targetInfo);
			}
			else
				theError = noErr;		// Dropped, just not very interesting
			break;
	}
	return (theError);
}


OSErr ABAddDragFlavors (long data)

{
	SendDragDataInfo	*pSendData;
	OSErr							theError;
#ifdef VCARD
	PromiseHFSFlavor	promise;
	short							count,
										index,
										ab,
										nick;
	Boolean						anyNonVCards;
#endif

	theError	= noErr;
	pSendData = (SendDragDataInfo *) data;

#ifdef VCARD
	// We need to see if the selected items exclusively consist of those in the
	// Personal Nicknames address book.  If so, we will promise an HFS in the
	// drag and eventually deliver a vCard.  If not, we treat the Personal Nickname
	// like any other nickname.  We do this because we DON'T want to deliver
	// regular old nicknames as vCards (because these aren't OUR vCards).
	if (IsVCardAvailable ()) {
		anyNonVCards = false;
		count = LVCountSelection (&NickList);
		for (index = 1; index <= count && !anyNonVCards; ++index)
			if (GetSelectedABNick (index, &ab, &nick))
				if (!IsPersonalAddressBook (ab) || !ValidNickname (nick))
					anyNonVCards = true;
		
		if (!anyNonVCards) {
			Zero (promise);
			promise.fileType = VCARD_TYPE;
			promise.fileCreator = CREATOR;
			promise.promisedFlavor = SPEC_FLAVOR;
			theError = AddDragItemFlavor (pSendData->drag, 1L, flavorTypePromiseHFS, &promise, sizeof(promise), flavorNotSaved);
			if (!theError)
				theError = AddDragItemFlavor (pSendData->drag, 1L, SPEC_FLAVOR, nil, 0L, flavorNotSaved);
			UseFeature (featureVCard);
		}
	}
#endif
	if (!theError)
		theError = AddDragItemFlavor (pSendData->drag, 1L, 'TEXT', nil, 0L, 0);
	if (!theError)
		theError = AddDragItemFlavor (pSendData->drag, 1L, kNickDragType, nil, 0L, 0);
	if (!theError)
		theError = AddDragItemFlavor (pSendData->drag, 1L, A822_FLAVOR, nil, 0L, 0);
	return (theError);
}


//
//	ABDoSendDragData
//
//		Build a data handle from the selected nicknames.  Based somewhat on the old NickDragToData
//		but using the new list view schtuff.
//
OSErr ABDoSendDragData (long data)

{
	SendDragDataInfo	*pSendData;
	Handle						dragData;
	OSErr							theError;
	Size							dataSize;
	short							ab,
										nick;

	// Save the current nickname before committing its data to a drag
	if (GetSelectedABNick (1, &ab, &nick))
		ReplaceNicknameData (Tabs, ab, nick);

	pSendData	= (SendDragDataInfo *) data;
	dragData	= nil;
	theError	= BuildDragData (pSendData, &dragData);
	if (!theError)
		if (dataSize = GetHandleSize (dragData)) {
			theError = SetDragItemFlavorData (pSendData->drag, pSendData->itemRef, pSendData->flavor, LDRef (dragData), dataSize, 0L);
			UL (dragData);
		}
		else
			theError = dragNotAcceptedErr;
	
	ZapHandle (dragData);
	return (theError);
}


void ABMove (VLNodeInfo *pToInfo, Boolean copy)

{
	VLNodeID	**movedList,
						*pItem;
	short			selectCount,
						ab,
						nick,
						lastMovedAB,
						i;
	Boolean		anyChanges;	// well?

	// Save the current nickname before committing its data to a move
	if (GetSelectedABNick (1, &ab, &nick))
		ReplaceNicknameData (Tabs, ab, nick);

	anyChanges	= false;
	selectCount	= LVCountSelection (&NickList);
	lastMovedAB	= kAddressBookUndefined;
	movedList		= (VLNodeID **) NuHandle (0);
	for (i = 1; i <= selectCount; i++) {
		if (GetSelectedABNick (i, &ab, &nick)) {
			// Is it just a nickname, or is it an entire address book?
			if (lastMovedAB != ab && ValidNickname (nick)) {
				if (DropNicknameOntoAddressBook (movedList, pToInfo, ab, nick, copy))
					anyChanges = true;
			}
			else
				if (DropAddressBook (movedList, pToInfo, ab, copy)) {
					lastMovedAB = ab;
					anyChanges = true;
				}
		}
	}
	// Build a new list -- and mark any of the moved or copied items as selected
	if (anyChanges) {
		InvalidListView (&NickList);
		selectCount = GetHandleSize (movedList) / sizeof (VLNodeID);
		for (i = 0, pItem = *movedList; i < selectCount; ++i, ++pItem)
			ABNickLVSelect (*pItem, true);
	}
}


//
//	ABNickDrop
//
//		Handle drops in the nickname list
//

OSErr ABNickDrop (DragReference drag, VLNodeInfo *targetInfo)

{
	BinAddrHandle	mungedAddresses;
	Str31					name;
	OSErr					theError;
	Handle				dragData,
								addresses,
								notes;
	long					offset,
								dragDataSize;
	short					count,
								item,
								newNick,
								ab,
								nick;

	theError = noErr;

	count = MyCountDragItems (drag);
	for (item = 1; !theError && item <= count; ++item) {
		dragData = nil;
		theError = MyGetDragItemData (drag, item, kNickDragType, &dragData);
		if (!theError) {
			offset					= 0;
			dragDataSize		= GetHandleSize (dragData);
			while (offset < dragDataSize && !theError) {
				mungedAddresses	= nil;
				addresses				= nil;
				notes						= nil;
				theError = GetNickFlavorDragData (dragData, &offset, &ab, &nick, name, &addresses, &notes);
		
				if (!theError)
					MaybeApplySplittingAlgorithm (notes);
				if (!theError)
					theError = SuckAddresses (&mungedAddresses, addresses, true, true, false, nil);
				if (!theError)
					if (!ValidNickname (newNick = AddNickname (mungedAddresses, notes, GetAddressBook (targetInfo->nodeID), name, false, nrDifferent, false)))
						theError = userCanceledErr;
				ZapHandle (mungedAddresses);
				ZapHandle (addresses);
				ZapHandle (notes);
			}
		}
		ZapHandle (dragData);
	}
	InvalidListView (&NickList);
	return (theError);
}

#ifdef VCARD

void DecodeQuotedPrintable (Handle value);

OSErr ABFileDrop (DragReference drag, VLNodeInfo *targetInfo)

{
	NickFromVCardItemRec	nickFromVCard;
	BinAddrHandle					addresses;
	FSSpec								spec;
	HFSFlavor							**dragData;
	Handle								vcardData;
	Str255								nickname,
												fullName,
												firstName,
												lastName,
												tag;
	OSErr									theError = noErr;
	short									count,
												item,
												ab,
												nick;
	Boolean								gotOne;	// we got at least one nickname out of the thing
	long									offset;	// offset to data in the vcard that's been left unparsed

	UseFeature (featureVCard);

	ab = GetAddressBook (targetInfo->nodeID);
	
	Zero (nickFromVCard);
	nickFromVCard.addresses	= NuHandle (0);
	nickFromVCard.notes			= NuHandle (0);
	theError = MemError ();
	
	count = MyCountDragItems (drag);
	for (item = 1; !theError && item <= count; ++item) {
		vcardData = nil;
		theError = MyGetDragItemData (drag, item, flavorTypeHFS, &dragData);
		if (!theError) {
			spec = (*dragData)->fileSpec;
			theError = SnarfRoman (&spec, &vcardData, 0);
			offset = 0;
		}
		do
		{
			if (!theError) {
				SetHandleBig (nickFromVCard.addresses, 0);
				SetHandleBig (nickFromVCard.notes, 0);
				nickFromVCard.numErrors = 0;
				theError = ParseVCard (vcardData, &offset, nil, VCardToNicknameItemProc, VCardToNicknameErrorProc, (long) &nickFromVCard);
				if (!theError && nickFromVCard.error)
					theError = dragNotAcceptedErr;
			}
			if (!offset) ZapHandle (vcardData);
			ZapHandle (dragData);
		
			// We should now have a nickname described as the addresses and notes.
			// We'll prep this data for its emergence as an official EUDORA NICKNAME!!
			
			// The addresses we've parsed are just raw text.  Convert them into the BinAddr format we like.
			addresses = nil;
			if (!theError)
				theError = SuckAddresses (&addresses, nickFromVCard.addresses, true, true, false, nil);

			// Apply our name splitting algorithm and clean up the notes 
			if (!theError) {
				MaybeApplySplittingAlgorithm (nickFromVCard.notes);
				Tr (nickFromVCard.notes, "\015", "\003");

				// Derive the nickname by first looking joining the first and last names
				*nickname = 0;
				GetTaggedFieldValueStrInNotes (nickFromVCard.notes, GetRString (tag, ABReservedTagsStrn + abTagFirst), firstName);
				GetTaggedFieldValueStrInNotes (nickFromVCard.notes, GetRString (tag, ABReservedTagsStrn + abTagLast), lastName);
				if (*firstName && *lastName)
					JoinFirstLast (nickname, firstName, lastName);
				
				// If that didn't work, try the full name field
				if (!*nickname)
					GetTaggedFieldValueStrInNotes (nickFromVCard.notes, GetRString (tag, ABReservedTagsStrn + abTagName), nickname);
				
				// Last gasp... make a nickname out of the address
				if (!*nickname) {
					if (addresses && *addresses && **addresses)
						NickSuggest (addresses, nickname, fullName,true,0);
				}
				else {
					BeautifyFrom (nickname);
					SanitizeFN (nickname, nickname, NICK_BAD_CHAR, NICK_REP_CHAR, false);
					*nickname = MIN (*nickname, sizeof (Str31) - 1);
				}
				
				// Put the nickname into the address book (and cross our fingers)
				if (*nickname) {
					if (!ValidNickname (nick = AddNickname (addresses, nickFromVCard.notes, ab, nickname, false, nrDifferent, false)))
						theError = userCanceledErr;
					else
						gotOne = true;
				}
			}
			ZapHandle (addresses);
		} while (!theError && offset);
	}
	
	if (gotOne) {
		InvalidListView (&NickList);
		if (count == 1) {
			if ((*Aliases)[ab].collapsed) {
				VLNodeInfo	data;
				LVGetItemWithNodeID (&NickList, MakeABNodeID (ab), &data);
				LVExpand (&NickList, MakeABNodeID (ab), data.name, true);
			}
			ABNickLVSelect (MakeNodeID (ab, nick), false);
			LVDraw (&NickList, nil, false, false);
		}
	}

	ZapHandle (nickFromVCard.addresses);
	ZapHandle (nickFromVCard.notes);
	return theError;
}


#ifdef NEVER
//
//	NicknameFromVCardFile
//
//		Create the nickname from a vCard file, returning handles containing
//		the addresses and notes of a nickname.
//

OSErr NicknameFromVCardFile (FSSpec *spec, Handle *addresses, Handle *notes)

{
	NickFromVCardItemRec	nickFromVCard;
	Handle								vcardData;
	OSErr									theError;

	theError = noErr;
	Zero (nickFromVCard);
	if (spec) {
		vcardData = nil;
		// Allocate a pair of empty handles for the notes and addresses
		nickFromVCard.addresses	= NuHandle (0);
		nickFromVCard.notes			= NuHandle (0);
		theError = MemError ();
		
		// Suck in all of the vCard data
		if (!theError)
			theError = Snarf (spec, &vcardData, 0);
		
		// Parse the vCard data using the VCardToNicknameItemProc to to the vCard-to-nickname transation
		if (!theError)
			theError = ParseVCard (vcardData, nil, VCardToNicknameItemProc, VCardToNicknameErrorProc, (long) &nickFromVCard);
		
		// We no longer need the vCard data
		ZapHandle (vcardData);
	}
	
	// If any errors occurred, zap the storage we allocated
	if (theError) {
		ZapHandle (nickFromVCard.addresses);
		ZapHandle (nickFromVCard.notes);
	}
	
	// Pass back the results
	*addresses = nickFromVCard.addresses;
	*notes		 = nickFromVCard.notes;
	return (theError);
}
#endif


//
//	VCardToNicknameItemProc
//
//		vCard item proc that builds a nickname from a vCard (wow!)
//
//		We actually don't care about much of the stuff included in a vCard (though, we someday might).
//		This function can grow to some greater level of detail, but for now we make a simple address
//		book entry.
//
//		Still, vCards can return very frustrating results to us.  Take, for instance, the following pair
//		of items:
//
//			TEL;HOME;WORK;VOICE;FAX:(858) 658-4063
//			TEL;VOICE;FAX;HOME;WORK:(858) 658-4063
//
//		In both cases we have a fax and voice number used for home and work.  So in our Eudora nickname
//		format we need to represent each of these lines as four unique attribute/value pairs (spread
//		across a pair of tabs in the Address Book).  Because the order of the parameters is very
//		different in each example, we can't simply take each parameter one-by-one and easily determine
//		the appropriate attribute to be assigned.
//
//		Instead, with each parsed item we'll build a list of home/work tag indices from which we'll
//		build each necessary attribute/value pair.  In the above example, this list will look like so:
//
//				abTagPhone	abTagPhone2
//				abTagFax		abTagFax2
//
//		When we process this list we'll process BOTH the 'home' and 'work' tags for each.  Had only one
//		of these tags (HOME or WORK) been present, we'd only process the appropriate index.
//
//		vCard is capable of all kinds of screwy combinations -- some, as the spec admits, impossible to
//		interpret (mostly, these are encoding oddities).  Because there is no formal schema, there's no
//		way to absolutely capture the spirit of a given vCard in our nickname format.  Take a look at
//		the following:
//
//			FN;HOME;FAX:John Purlia
//
//		Huh?  My "home" full name is a fax machine named "John Purlia"??  Or, this simpler combo:
//
//			FN;HOME:John Purlia
//			FN;WORK;PREF:The Mad Scientist of VCard
//			FN;WORK;PREF:Super Toy Man, Master of all things mechanical
//
//		So at work and at home I have different names.  In fact, at work I have TWO names and both of
//		then are apparently the preferred way for people to communicate my name.  This seems unreasonable
//		(not to mention incredibly pompous), but it's allowed by the vCard specification.  So, since we
//		don't have a concept of multiple full names, we have to choose one.  While we _could_ apply some
//		kind of logic to this problem and say, well, we'll use the first occurrence, or maybe we'll keep
//		those we don't use in notes as "AKA" fields, or maybe we'll see which (if any) is the preferred
//		name.  Uh, no, sorry.  There are too many fields and too many combinations.  Instead, we'll use
//		the first one, ignoring all others.  Tough.  This also goes for non name fields that repeat in
//		a vCard.  For example:
//
//			TEL;WORK:(858) 658-4063
//			TEL;WORK:(619) 890-6158
//
//		Ooooo... big man... you have two phones at work.  Well, fine.  We'll only assign the first as the
//		"Work Phone" in Eudora -- the second will end up in "Other Phone Numbers", and only because this
//		is a field that has a reasonable other place to put the data.  If that's not the case, for example:
//
//			TITLE:Director, Engineering
//			TITLE:Mac Programmer Guy
//
//		For now we'll just ignore the second title (though I suppose it could be dumped into notes).
//
//		Fields that have no reasonable nickname equivalent (GEO, TZ, etc) will for now be ignored.  At some
//		future time we might dump this information into notes, but for now that will onyl server to clutter
//		the notes field.
//
	
OSErr VCardToNicknameItemProc (VCardItemPtr itemPtr, NickFromVCardItemRecPtr nickFromVCardItemPtr)

{
	VCardParamPtr	paramPtr;
	OSErr					theError;
	Size					valSize,
								addrSize;
	short					numParams,
								i;
	Boolean				home,
								work,
								ignored,
								tel,
								preferred;
					
	// Some initialization
	theError	= noErr;
	ignored		= false;
	tel				= false;
	numParams	= GetHandleSize (itemPtr->params) / sizeof (VCardParamRec);
	home			= itemPtr->propertyFlags & vcPropFlagHome ? true : false;
	work			= itemPtr->propertyFlags & vcPropFlagWork ? true : false;
	preferred	= itemPtr->propertyFlags & vcPropFlagPreferred ? true : false;

	if (!home && !work)
		if (PrefIsSet (PREF_HOME_IS_NICER_THAN_WORK))
			home = true;
		else
			work = true;
	
	// Any special decoding for the value?
	switch (itemPtr->encoding) {
		case 	vcValue7Bit:
		case vcValue8Bit:
			break;
		case vcValueQuotedPrintable:
			DecodeQuotedPrintable (itemPtr->value);
			break;
		case vcValueBase64:
			break;
		default:
			break;
	}

	valSize = GetHandleSize (itemPtr->value);
	if (!valSize)
		return (noErr);

	LDRef (itemPtr->value);
	// Properties we care about
	switch (itemPtr->property) {
		case vcKeyFN:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagName, 0, true, false, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcKeyTitle:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagTitle, 0, true, false, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcKeyTel:
			// Telephone numbers are special (or, at the very least, difficult).  A parameter indicating the specific type
			// of phone might be present, so we handle those cases below.  Or, a parameter might not be present -- so we have
			// to just note that we're working on a TEL property and handle it later if there are no specific phone properties.
			tel = true;
			break;
		case vcKeyEmail:
			// The preferred email address should go into the address expansion field -- everything else gets dumped into "other email".
			// If no preferred address is specified, we assume that the first one received is the preferred address.
			// Couple of things to note...  When we find "the" preferred email address (at least, we hope only one is preferred) we take
			// anything previously assumed to be the preferred email address and dump it into the "other email" field, placing the new
			// data into the address expansion.  If we find a second preferred email address, we repeat the swap, so it's possible that
			// duplicate addresses could end up in "other email", but this is a vCard problem (which is the lazy way of saying that we're
			// not going to save state just for the sake of badly formed vCard).
			if (preferred) {
				if (addrSize = GetHandleSize (nickFromVCardItemPtr->addresses)) {
					theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagOtherEmail, 0, true, false, LDRef (nickFromVCardItemPtr->addresses), addrSize, nickFieldAppendExisting, NICK_CONCAT_NEWLINE, nil);
					UL (nickFromVCardItemPtr->addresses);
					SetHandleBig (nickFromVCardItemPtr->addresses, 0);
				}
				if (!theError)
					theError = HandPlusHand (itemPtr->value, nickFromVCardItemPtr->addresses);
			}
			else
				// When we have an unpreferred address, check to see if the addresses expansion handle is empty.  If it is, place it there.
				// If it is not, place it into "other email".
				if (!GetHandleSize (nickFromVCardItemPtr->addresses))
					theError = HandPlusHand (itemPtr->value, nickFromVCardItemPtr->addresses);
				else
					theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagOtherEmail, 0, true, false, *itemPtr->value, valSize, nickFieldAppendExisting, NICK_CONCAT_NEWLINE, nil);
			break;
		case vcKeyURL:
			theError = VCardToNicknameSetValueOrOther (nickFromVCardItemPtr->notes, abTagWeb, abTagWeb2, abTagOtherWeb, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, NICK_CONCAT_NEWLINE, &ignored);
			break;
		case vcKeyNote:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagNote, 0, true, false, *itemPtr->value, valSize, nickFieldAppendExisting, NICK_CONCAT_NEWLINE, nil);
			break;
		case vcPOBox:
		case vcExtendedAddress:
		case vcStreet:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagAddress, abTagAddress2, home, work, *itemPtr->value, valSize, nickFieldAppendExisting, NICK_CONCAT_NEWLINE, nil);
			break;
		case vcLocality:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagCity, abTagCity2, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcRegion:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagState, abTagState2, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcPostalCode:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagZip, abTagZip2, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcCountry:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagCountry, abTagCountry2, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcFamilyName:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagLast, 0, true, false, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcGivenName:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, abTagFirst, 0, true, false, *itemPtr->value, valSize, nickFieldIgnoreExisting, 0, nil);
			break;
		case vcOrganizationName:
		case vcOrganizationUnits:
			theError = VCardToNicknameSetValue (nickFromVCardItemPtr->notes, 0, abTagCompany, false, true, *itemPtr->value, valSize, nickFieldAppendExisting, NICK_CONCAT_SEPARATOR, nil);
			break;
	}
	
	if (numParams) {
		paramPtr = *itemPtr->params;
		for (i = 0; i < numParams; ++i, ++ paramPtr) {
			switch (paramPtr->pProperty) {
				case vcKeyVoice:
					if (itemPtr->property == vcKeyTel) {
						tel = false;
						theError = VCardToNicknameSetValueOrOther (nickFromVCardItemPtr->notes, abTagPhone, abTagPhone2, abTagOtherPhone, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, NICK_CONCAT_NEWLINE, &ignored);
					}
					break;
				case vcKeyFax:
					if (itemPtr->property == vcKeyTel) {
						tel = false;
						theError = VCardToNicknameSetValueOrOther (nickFromVCardItemPtr->notes, abTagFax, abTagFax2, abTagOtherPhone, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, NICK_CONCAT_NEWLINE, &ignored);
					}
					break;
				case vcKeyMSG:
					// We basically ignore this one so that we don't accidentally repeat a number in the "other phone" field.  We treat it like TEL
					break;
				case vcKeyCell:
					if (itemPtr->property == vcKeyTel) {
						tel = false;
						theError = VCardToNicknameSetValueOrOther (nickFromVCardItemPtr->notes, abTagMobile, abTagMobile2, abTagOtherPhone, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, NICK_CONCAT_NEWLINE, &ignored);
					}
					break;
			}
		}
	}
	
	// If we're working on a telephone property that was not assigned by an existing parameter, assume it is a regular phone
	if (!theError && tel)
		theError = VCardToNicknameSetValueOrOther (nickFromVCardItemPtr->notes, abTagPhone, abTagPhone2, abTagOtherPhone, home, work, *itemPtr->value, valSize, nickFieldIgnoreExisting, NICK_CONCAT_NEWLINE, &ignored);
	
	UL (itemPtr->value);

	return (theError);
}


Boolean VCardToNicknameErrorProc (Handle data, VCardItemPtr itemPtr, NickFromVCardItemRecPtr nickFromVCardItemPtr, VCardErrorType error, long offset)

{
	Boolean	shouldWeQuit;
	
	// We'll be extra tough on data that does not contain a 'BEGIN' keyword or
	// any discerable vCard data (but leaving out 'END' is dandy.
	++nickFromVCardItemPtr->numErrors;
	if (shouldWeQuit = (error == vcMissingBegin || error == vcMissingVcard)) {
		nickFromVCardItemPtr->error = error;
		nickFromVCardItemPtr->offset = offset;
	}
	return (PrefIsSet (PREF_VCARD_QUIT_ON_ERROR) || shouldWeQuit);
}


OSErr VCardToNicknameSetValue (Handle notes, short homeTagIndex, short workTagIndex, Boolean home, Boolean work, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored)

{
	Str255	tag;
	OSErr		theError;

	theError = noErr;	
	if (home)
		theError = SetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + homeTagIndex), value, length, setValue, separatorIndex, ignored);
	if (work)
		theError = SetTaggedFieldValueInNotes (notes, GetRString (tag, ABReservedTagsStrn + workTagIndex), value, length, setValue, separatorIndex, ignored);
	return (theError);
}


OSErr VCardToNicknameSetValueOrOther (Handle notes, short homeTagIndex, short workTagIndex, short otherTagIndex, Boolean home, Boolean work, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored)

{
	OSErr	theError;
	
	theError = VCardToNicknameSetValue (notes, homeTagIndex, workTagIndex, home, work, value, length, setValue, 0, ignored);
	if (*ignored && !theError)
		theError = VCardToNicknameSetValue (notes, otherTagIndex, 0, true, false, value, length, setValue, separatorIndex, nil);
	return (theError);
}


//
//	DecodeQuotedPrintable
//
//		Remove occurences of "=\r"
//		Replace occurences of "=0D" and "=0A" with '\r'
//
void DecodeQuotedPrintable (Handle value)

{
	Ptr		spot,
				end,
				copySpot;

	spot		 = *value;
	end			 = spot + GetHandleSize (value);
	copySpot = spot;

	while (spot < end) {
		if (*spot == '=') {
			if (spot + 1 < end && !memcmp (spot, "=\r", 2))
				spot += 2;
			else
			if (spot + 5 < end && !memcmp (spot, "=0D=0A", 6)) {
				spot += 6;
				*copySpot++ = '\r';
			}
			else
			if (spot + 2 < end && !memcmp (spot, "=0D", 3)) {
				spot += 3;
				*copySpot++ = '\r';
			}
			else
			if (spot + 2 < end && !memcmp (spot, "=0A", 3)) {
				spot += 3;
				*copySpot++ = '\r';
			}
			else
				*copySpot++ = *spot++;
		}
		else
			*copySpot++ = *spot++;
	}
	SetHandleBig (value, copySpot - *value);

}
#endif


OSErr GetNickFlavorDragData (Handle dragData, long *offset, short *ab, short *nick, PStr nickname, Handle *addresses, Handle *notes)

{
	Ptr		dragDataPtr;
	OSErr	theError;
	long	dataSize;
	
	theError = noErr;
	
	dragDataPtr = LDRef (dragData) + *offset;
	
	// The address book and nickname
	BlockMoveData (dragDataPtr, ab, sizeof (*ab));
	dragDataPtr += sizeof (*ab);
	BlockMoveData (dragDataPtr, nick, sizeof (*nick));
	dragDataPtr += sizeof (*nick);

	// Make sure these are valid indices -- otherwise we might be dealing with garbage
// Commented out for now because this does not seem to be used
//	if (*ab < 0 || *ab >= NAliases)
//		return (dragNotAcceptedErr);
//	if (*nick < 0 || *nick >= (GetHandleSize_ ((*Aliases)[*ab].theData) / sizeof (NickStruct)))
//		return (dragNotAcceptedErr);
	
	// The nickname itself
	PCopy (nickname, dragDataPtr);
	dragDataPtr += *nickname + 1;

	// Addresses
	BlockMoveData (dragDataPtr, &dataSize, sizeof (dataSize));
	dragDataPtr += sizeof (dataSize);
	if (dataSize && addresses)
		theError = PtrToHand (dragDataPtr, addresses, dataSize);
	dragDataPtr += dataSize;

	// Notes
	if (!theError) {
		BlockMoveData (dragDataPtr, &dataSize, sizeof(dataSize));
		dragDataPtr += sizeof (dataSize);
		if (dataSize && notes)
			theError = PtrToHand (dragDataPtr, notes, dataSize);
		dragDataPtr += dataSize;
	}
	UL (dragData);
	*offset = dragDataPtr - *dragData;
	return (theError);
}

//
//	BuildDragData
//
//		Build a data handle from the selected nicknames.  Based somewhat on the old NickDragToData
//		but using the new list view and expanded nickname schtuff.
//

OSErr	BuildDragData (SendDragDataInfo	*pSendData, Handle *dragData)

{
	Accumulator	drag;
	OSErr				theError;
	short				ab,
							nick,
							lastAB,
							count,
							index;

	theError = AccuInit (&drag);
	if (!theError) {
		count = LVCountSelection (&NickList);
		// Take a trip through all the selections
		lastAB = kAddressBookUndefined;
		for (index = 1; index <= count && !theError && !drag.err; ++index)
			if (GetSelectedABNick (index, &ab, &nick)) {
				CycleBalls();
				if (lastAB != ab && ValidNickname (nick))
					theError = BuildNicknameDragData (pSendData, &drag, ab, nick);
				else 
					theError = BuildAddressBookDragData (pSendData, &drag, lastAB = ab);
			}
	}
	if (!theError) {
		AccuTrim (&drag);
		*dragData = drag.data;
		drag.data = nil;
	}
	AccuZap (drag);
	return (theError);
}


OSErr BuildNicknameDragData (SendDragDataInfo	*pSendData, AccuPtr drag, short ab, short nick)

{
	OSErr	theError;

	theError = noErr;
	switch (pSendData->flavor) {
		case A822_FLAVOR:
			theError = Build822FlavorDragData (drag, ab, nick);
			break;
		case kNickDragType:
			theError = BuildNickFlavorDragData (drag, ab, nick);
			break;
		case 'TEXT':
			theError = BuildTextFlavorDragData (drag, ab, nick);
			break;
#ifdef VCARD
		case SPEC_FLAVOR:
			if (IsVCardAvailable ())
				theError = BuildSpecFlavorDragData (pSendData, drag, ab, nick);
			break;
#endif
	}
	return (theError);
}


OSErr BuildAddressBookDragData (SendDragDataInfo *pSendData, AccuPtr drag, short ab)

{
	OSErr	theError;
	short	totalNicks,
				nick;

	theError = noErr;
	totalNicks = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	for (nick = 0; nick < totalNicks && !theError; nick++)
		theError = BuildNicknameDragData (pSendData, drag, ab, nick);
	return (theError);
}


OSErr Build822FlavorDragData (AccuPtr drag, short ab, short nick)

{
	Handle	data;
	OSErr		theError;
	
	theError = noErr;
	if (data = GetAliasExpansionFor822Flavor (ab, nick)) {
		if (drag->offset)
			theError = AccuAddChar (drag, ',');
		if (!theError)
			theError = AccuAddHandle (drag, data);
		ZapHandle (data);
	}
	return (theError);
}


OSErr BuildNickFlavorDragData (AccuPtr drag, short ab, short nick)

{
	Str31		name;
	Handle	data;
	OSErr		theError;
	long		dataSize;
	
	// The address book
	theError = AccuAddPtr (drag, (void *) &ab, sizeof (ab));
	
	// The nickname
	if (!theError)
		theError = AccuAddPtr (drag, (void *) &nick, sizeof (nick));
		
	// name
	if (!theError) {
		GetNicknameNamePStr (ab, nick, name);
		theError = AccuAddPtr (drag, name, *name + 1);
	}

	// addresses
	if (!theError) {
		data = GetNicknameData (ab, nick, true, true);
		dataSize = data ? GetHandleSize (data) : 0;
		theError = AccuAddPtr (drag, (void *) &dataSize, sizeof (dataSize));
	}
	if (data && !theError)
		theError = AccuAddHandle (drag, data);
		
		// notes
	if (!theError) {
		data = GetNicknameData (ab, nick, false, true);
		dataSize = data ? GetHandleSize (data) : 0;
		theError = AccuAddPtr (drag, (void *) &dataSize, sizeof (dataSize));
	}
	if (data && !theError)
		theError = AccuAddHandle (drag, data);

	return (theError);
}

//
//	BuildTextFlavorDragData
//

OSErr BuildTextFlavorDragData (AccuPtr drag, short ab, short nick)

{
	AttributeValueHandle	avPairs;
	AttributeValuePtr			avPairPtr;
	TabFieldHandle				fields;
	Str31									name,
												tag,
												shortName;
	BinAddrHandle					expansion;
	Handle								addresses,
												notes,
												leftovers;
	OSErr									theError;
	long									unrequestedTypesToBePutInLeftovers;
	short									count,
												i;

	// Data we will find incredibly useful later
	leftovers = nil;
	addresses	= nil;
	expansion	= nil;
	fields		= Tabs && AB.inited ? GetTabFieldStrings (Tabs, tabTagName) : GetTabFieldsFromResources (tabTagName, CREATOR, kNickTabType);
	notes			= GetNicknameData (ab, nick, false, true);
	
	unrequestedTypesToBePutInLeftovers = PrefIsSet (PREF_INCLUDE_HIDDEN_NICK_FIELDS_IN_DRAGS) ? (avPairUnknown | avPairHidden) : avPairUnknown;
	avPairs		= notes ? ParseAllAttributeValuePairs (notes, &leftovers, avPairKnown, unrequestedTypesToBePutInLeftovers) : nil;

	theError = nil;

	// Put in a blank line in between nicknames
	if (drag->offset)
		theError = AccuAddChar (drag, kDragDataDelim);

	// The nickname itself
	if (!theError)
		theError = AccuAddRes (drag, ALIAS_A_LABEL);
	if (!theError)
		theError = AccuAddChar (drag, ' ');
	if (!theError)
		theError = AccuAddStr (drag, GetNicknameNamePStr (ab, nick, name));
	if (!theError)
		theError = AccuAddChar (drag, kDragDataDelim);

	// All the email addresses -- which is the expansion of the nickname itself.
	if (!theError)
		theError = PtrToHand (name, &addresses, *name + 1);
	if (!theError)
		theError = PtrPlusHand ("", addresses, 1);
	if (!theError)
		theError = ExpandAliases (&expansion, addresses, 0, true);
	if (!theError)
		CommaList (expansion);
	if (expansion) {
		if (!theError)
			theError = AccuAddStr (drag, FindShortNameWithTag (Tabs, GetRString (tag, ABReservedTagsStrn + abTagEmail), shortName));
		if (!theError)
			theError = AccuAddChar (drag, ' ');
		if (!theError)
			theError = AccuAddHandle (drag, expansion);
		if (!theError)
			theError = AccuAddChar (drag, kDragDataDelim);
	}
	
	if (!theError && fields && notes && avPairs) {
		// Everything else including the kitchen sink
		if (!theError) {
			avPairPtr = LDRef (avPairs);
			LDRef (notes);
			count = GetHandleSize (avPairs) / sizeof (AttributeValueRec);
			for (i = 0; i < count && !theError; ++i, ++avPairPtr) {
				MakePStr (tag, *notes + avPairPtr->attributeOffset, avPairPtr->attributeLength);
				FindShortNameWithTag (Tabs, tag, name);
				if (*name) {
					theError = AccuAddStr (drag, name);
					if (!theError)
						theError = AccuAddChar (drag, ' ');
					if (!theError)
						theError = AccuAddTrPtr (drag, *notes + avPairPtr->valueOffset, avPairPtr->valueLength,"\002\003",">\015");
					if (!theError)
						theError = AccuAddChar (drag, kDragDataDelim);
				}
			}
			UL (notes);
		}
		
		// Leftovers!  Mmmm, mmm, good!
		if (leftovers && GetHandleSize (leftovers)) {
			if (!theError)
				theError = AccuAddRes (drag, ALIAS_N_LABEL);
			if (!theError)
				theError = AccuAddChar (drag, ' ');
			if (!theError)
				theError = AccuAddTrHandle (drag, leftovers,"\002",">");
			if (!theError)
				theError = AccuAddChar (drag, kDragDataDelim);
		}
	}
	// Cleanup
	ZapHandle (fields);
	ZapHandle (avPairs);
	ZapHandle (leftovers);
	ZapHandle (expansion);
	ZapHandle (addresses);
	return (theError);
}


#ifdef VCARD
//
//	BuildSpecFlavorDragData
//

OSErr BuildSpecFlavorDragData (SendDragDataInfo	*pSendData, AccuPtr drag, short ab, short nick)

{
	AEDesc	dropLocation;
	FSSpec	spec;
	Handle	vcardData,
					notes;
	OSErr		theError;

	theError = noErr;
	// Make a vCard from this nickname
	if (notes = GetNicknameData (ab, nick, false, true))
		Tr (notes, "\003", "\015");
	if (vcardData = MakeVCard (GetNicknameData (ab, nick, true, true), notes)) {
		NullADList (&dropLocation, nil);
		theError = GetDropLocation (pSendData->drag, &dropLocation);
		if (!theError)
			theError = GetDropLocationDirectory (&dropLocation, &spec.vRefNum, &spec.parID);

		if (!theError) {
			MakeVCardFileName (ab, nick, spec.name);
			theError = UniqueSpec (&spec, 31);
		}
		if (!theError) {
			FSpCreateResFile (&spec, CREATOR, VCARD_TYPE, smSystemScript);
			WhackFinder (&spec);
			theError = ResError ();
		}
		// Write the vCard data into the file
		if (!theError)
			theError = Blat (&spec, vcardData, false);

		if (!theError)
			theError = AccuAddPtr (drag, &spec, sizeof (FSSpec));
		DisposeADList (&dropLocation,nil);
		ZapHandle (vcardData);
	}
	else
		theError = MemError ();
	return (theError);
}
#endif

//
//	DropNicknameOntoAddressBook
//
//		Handle nickname drops onto address books.  Returns true if the drop successfully
//		completed, false if not.
//
Boolean DropNicknameOntoAddressBook (VLNodeID **movedList, VLNodeInfo *targetInfo, short ab, short nick, Boolean copy)

{
	VLNodeID			nodeID;
	BinAddrHandle	mungedAddresses;
	short					targetAB,
								targetNick,
								newNick;
	Boolean				lookingGood;
	
	lookingGood			= false;		// We are pessimists
	mungedAddresses = nil;

	GetABNick (targetInfo->nodeID, &targetAB, &targetNick);
	if (ab != targetAB && ValidAddressBook (targetAB) && !ValidNickname (targetNick))
		if (lookingGood = ValidNickname (newNick = MoveNickname (ab, nick, targetAB, copy))) {
			nodeID = MakeNodeID (targetAB, newNick);
			PtrPlusHand (&nodeID, movedList, sizeof (nodeID));
		}
	return (lookingGood);
} 

//
//	DropAddressBook
//
//		Handle address book drops onto other address books.  Returns true if the drop successfully
//		completed, false if not.
//
Boolean DropAddressBook (VLNodeID **movedList, VLNodeInfo *targetInfo, short ab, Boolean copy)

{
	VLNodeID	nodeID;
	short			targetAB,
						targetNick,
						abNick,
						totalNicks,
						newNick;
	Boolean		lookingGood;
	
	lookingGood = false;		// We are pessimists
	GetABNick (targetInfo->nodeID, &targetAB, &targetNick);
	if (ab != targetAB) {
		GetABNick (targetInfo->nodeID, &targetAB, &targetNick);
		if (ValidAddressBook (targetAB) && !ValidNickname (targetNick)) {
			totalNicks = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
			for (abNick = 0; abNick < totalNicks; abNick++)
				if (lookingGood = ValidNickname (newNick = MoveNickname (ab, abNick, targetAB, copy))) {
					nodeID = MakeNodeID (targetAB, newNick);
					PtrPlusHand (&nodeID, movedList, sizeof (nodeID));
				}
		}
	}
	return (lookingGood);
}


//
//	MoveNickname
//
//		Returns the index of the nickname as it appears in it's new address book, or
//		kNickUndefined if the move failed.
//

short MoveNickname (short ab, short nick, short targetAB, Boolean copy)

{
	BinAddrHandle	mungedAddresses;
	Handle				addresses,
								notes;
	Str31					name;
	short					newNick;
	Boolean				lookingGood;

	if (!ValidAddressBook (targetAB))
		return (kNickUndefined);
	
	lookingGood = true;
	mungedAddresses = nil;
	notes = GetNicknameData (ab, nick, false, true);
	if (addresses = GetNicknameData (ab, nick, true, true))
		if (SuckAddresses (&mungedAddresses, addresses, true, true, false, nil))
			lookingGood = false;

	if (lookingGood) {
		// Add the nickname into the target
		lookingGood = ValidNickname (newNick = AddNickname (mungedAddresses, notes, targetAB, GetNicknameNamePStr (ab, nick, name), false, nrDifferent, false));
		
		// Now, if that worked out okay, mark the nickname from its original address book as deleted (unless we were performing a copy)
		if (lookingGood && !copy && !(*Aliases)[ab].ro && (*Aliases)[ab].theData) {
			(*((*Aliases)[ab].theData))[nick].addressesDirty = true;
			(*((*Aliases)[ab].theData))[nick].deleted				 = true;
			SetAliasDirty(ab);
		}
	}
	ZapHandle (mungedAddresses);
	return (lookingGood ? newNick : kNickUndefined);
}

/************************************************************************
 * DoViewByMenu - change the way the list is sorted
 ************************************************************************/

void DoViewByMenu (void)

{
	Str255	oldViewByString,
					newViewByString;
	short		numABs,
					ab,
					abDisplayed,
					nickDisplayed;

	GetPref (oldViewByString, PREF_NICK_VIEW_BY);
	GetMenuItemText (PopUpMenuH (CtlViewBy), GetControlValue (CtlViewBy), newViewByString);
	if (!StringSame (oldViewByString, newViewByString)) {
		abDisplayed		= AB.abDisplayed;
		nickDisplayed	= AB.nickDisplayed;
		numABs = NAliases;
		for (ab = 0; ab < numABs; ab++)
			ZapHandle ((*Aliases)[ab].sortData);
		InvalidListView (&NickList);
		// Reselect the previous items
		if (!(abDisplayed == kAddressBookUndefined && nickDisplayed == kNickUndefined))
			ABNickLVSelect (MakeNodeID (abDisplayed, nickDisplayed), true);
		SetPref (PREF_NICK_VIEW_BY, newViewByString);
	}
}

/************************************************************************
 * DoNewNickname - make a new nickname
 ************************************************************************/

void DoNewNickname (long modifiers)

{
	VLNodeInfo	data;
	Str31				name,
							massagedName;
	Handle			addresses,
							notes,
							shortAddresses;
	short				ab,
							nick;
	
	// Put the nickname in the first selected address book...
	//		... or in the default address book
	if (LVGetItem (&NickList, 1, &data, true))
		GetABNick (data.nodeID, &ab, &nick);
	else
		if (!LVGetItemWithNodeID (&NickList, MakeABNodeID (ab = FindAddressBookType (eudoraAddressBook)), &data))
			return;

	// If a nickname (by itself) is selected, and the option key is being held down, we'll clone that nickname
	*name					 = 0;
	addresses			 = nil;
	notes					 = nil;
	shortAddresses = nil;
	if ((modifiers & optionKey) && LVCountSelection (&NickList) == 1 && ValidNickname (nick)) {
		PCopy (name, data.name);
		if (addresses = GetNicknameData (ab, nick, true, true))
			SuckPtrAddresses (&shortAddresses, LDRef (addresses), GetHandleSize (addresses), true, true, false, nil);
		notes = GetNicknameData (ab, nick, false, true);
		// Make copies of the existing notes to be passed to NewNickLow
		if (notes)
			HandToHand (&notes);

	}
	MakeUniqueNickname (ab, name);
	if ((nick = NewNickLow (shortAddresses, notes, ab, name, false, nrAdd, false)) != kNickUndefined) {
		// If this address book is collapsed, expand it
		if ((*Aliases)[ab].collapsed)
			LVExpand (&NickList, MakeABNodeID (ab), data.name, true);
		InvalidListView (&NickList);
		// Massage the item name so it can be found in the list view
		GetListNameBasedOnTag (SortTag, ab, nick, massagedName);
		// Rename it
		ABSetKeyboardFocus (focusNickList);
		LVRename (&NickList, MakeNodeID (ab, nick), massagedName, false, !PrefIsSet (PREF_ALLOW_NICK_RENAME_IN_LIST));	//	Allow user to rename the untitled nickname
		if (!PrefIsSet (PREF_ALLOW_NICK_RENAME_IN_LIST)) {
			ABSetKeyboardFocus (focusNickField);
			PeteSelectAll (Win, Win->pte);
		}
	}
	ZapHandle (shortAddresses);
}


/************************************************************************
 * DoNewNickname - make a new nickname
 ************************************************************************/

void DoNewAddressBook (void)

{
	FSSpec	spec,
					folderSpec;
	short		ab;

	SubFolderSpec (NICK_FOLDER,&folderSpec);

	//	Make a unique "untitled" name
	MakeUniqueUntitledSpec (folderSpec.vRefNum, folderSpec.parID, UNTITLED_ADDRESS_BOOK, &spec);

	// Make the new file
	ab = NickMakeNewFile (spec.name);

	if (ValidAddressBook (ab)) {
		InvalidListView (&NickList);
		LVRename (&NickList, MakeABNodeID (ab), spec.name, false, false);	//	Allow user to rename the untitled address book
	}
}


void DoHorizontalZoom (MyWindowPtr win)

{
	WindowPtr	winWP;
	Rect			contR;
	
	winWP = GetMyWindowWindowPtr (win);
	win->saveSize = true;
	utl_InvalGrow (winWP);

	if (Collapsed) {
		SetWinMinSize (Win, MinListWidth + MinTabWidth, kMinNickWinHeight);
		SetPrefLong (PREF_NICK_COLLAPSED, 0);
		contR = win->contR;
		contR.right += Collapsed - 2;
		SanitizeSize (win, &contR);
		
		// Show the things on the right side (note that the tabs, since not really hidden, move into place when we resize
		ShowControl (NickField);
		ShowControl (CtlRecipient);
		ShowControl (CtlDoNotSync);
		
		SizeWindow (winWP, contR.right, contR.bottom, false);
		Collapsed = 0;
	}
	else {
		SetWinMinSize (Win, MinListWidth, kMinNickWinHeight);
		SetPrefLong (PREF_NICK_COLLAPSED, Collapsed = win->contR.right - VertDivider.left + 1);
		
		// Hide the things on the right side
		MoveLabelField (NickField, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (NickField), ControlHi (NickField));
		MoveMyCntl (CtlRecipient, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (CtlRecipient), ControlHi (CtlRecipient));
		MoveMyCntl (CtlDoNotSync, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (CtlDoNotSync), ControlHi (CtlDoNotSync));
		MoveTab (Tabs, CNTL_OUT_OF_VIEW, CNTL_OUT_OF_VIEW, ControlWi (Tabs), ControlHi (Tabs));
		
		// Move the focus to the list
		ABSetKeyboardFocus (focusNickList);
		
		SizeWindow (winWP, VertDivider.left, win->contR.bottom, false);
	}
	MyWindowDidResize (win, nil);
}

void DoRecipientList (ControlHandle theControl)

{
	MyWindowPtr	win = GetWindowMyWindowPtr(GetControlOwner(theControl));
	VLNodeInfo	data;
	Str31				name;
	short				value,
							count,
							i,
							ab,
							nick;
	
	// Flip the checkbox itself
	value = GetControlValue (theControl);
	SetControlValue (theControl, value = (value == kControlCheckBoxUncheckedValue || value == kControlCheckBoxMixedValue ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue));

	// Walk through all the selected list items
	count = LVCountSelection (&NickList);
	// Take a trip through all the selections looking for relevant conditions
	for (i = 1; i <= count; ++i) {
		GetSelectedABNickNameData (i, &ab, &nick, name, &data);
		if (nick != kNickUndefined) {
			GetNicknameNamePStr (ab, nick, name);
			if (value == kControlCheckBoxCheckedValue)
				AddStringAsTo (name);
			else				
				RemoveFromTo (name);
		}
		data.iconID = ABSetIcon (ab, nick);
		LVSetItemWithNodeID (&NickList, MakeNodeID (ab, nick), &data, true);
	}
	win->isDirty = true;
}

void DoDoNotSync (ControlHandle theControl)

{
	MyWindowPtr	win = GetWindowMyWindowPtr(GetControlOwner(theControl));
	VLNodeInfo	data;
	Str31				name;
	Handle			notes;
	short				value,
							count,
							i,
							ab,
							nick;
	Boolean			notesExist;
	
	// Flip the checkbox itself
	value = GetControlValue (theControl);
	SetControlValue (theControl, value = (value == kControlCheckBoxUncheckedValue || value == kControlCheckBoxMixedValue ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue));

	// Walk through all the selected list items
	count = LVCountSelection (&NickList);
	// Take a trip through all the selections looking for relevant conditions
	for (i = 1; i <= count; ++i) {
		GetSelectedABNickNameData (i, &ab, &nick, name, &data);
		if (nick != kNickUndefined) {
			GetNicknameNamePStr (ab, nick, name);
			if (value == kControlCheckBoxCheckedValue) {
				notes = GetNicknameData (ab, nick, false, true);
				if (!(notesExist = notes ? true : false))
					notes = NuHandle (0);
				if (notes)
					if (!SetNicknameChangeBit (notes, changeBitPrivate, false))
						ReplaceNicknameNotes (ab, name, notes);
				if (!notesExist)
					ZapHandle (notes);
			}
			else				
				ClearNicknameChangeBits (ab, nick, changeBitPrivate);
		}
	}
	win->isDirty = true;
}


//
//	ABNickLVSelect
//
//		Wrapper around list view's LVSelect taking into account the current View By sort order
//

Boolean ABNickLVSelect (VLNodeID nodeID, Boolean addToSelection)

{
	Str31	massagedName;
	
	return (LVSelect (&NickList, nodeID, GetListNameBasedOnTag (SortTag, GetAddressBook (nodeID), GetNickname (nodeID), massagedName), addToSelection));
}




//
//	ABRename
//
//		We're always dealing with nicknames here, even though the list might be sorted
//		by some other means
//
//		WARNING - after calling this to rename a nickname file, the index (ab) may no longer
//		be valid.  SortAddressBookList() is called here which may reorder the Aliases list.
//		Don't trust ab after you call this routine on a nickname file! - jdboyd 12/18/03

Boolean ABRename (short ab, short nick, StringPtr newName)

{
	VLNodeInfo	data;
	VLNodeID		nodeID;
	Str31				oldName;
	Handle			notes;
	OSErr				theError;
	Boolean			notesExist;
	Str63 		abName;

	// If we haven't been given a valid nickname, bail on outta here
	nodeID = MakeNodeID (ab, nick);
	if (!LVGetItemWithNodeID (&NickList, nodeID, &data))
		return (false);
		
	*newName = MIN (*newName, sizeof (Str31) - 1);
	if (*newName && !ExactlyEqualString (newName, GetNicknameNamePStr (ab, nick, oldName))) {
		PCopy (data.name, newName);
		// Is it a nickname file?
		if (nick == kNickUndefined) {
			Str31 cleanName;
			SanitizeFN(cleanName,newName,MAC_FN_BAD,MAC_FN_REP,false);
			if (EqualStrRes(cleanName,ALIAS_FILE) || EqualStrRes(cleanName,USA_EUDORA_NICKNAMES) || EqualStrRes(cleanName,FILE_ALIAS_NICKNAMES))
				theError = dupFNErr;
			else
				theError = FSpRename (&(*Aliases)[ab].spec, cleanName);
			if (!theError) {
				PCopy ((*Aliases)[ab].spec.name, cleanName);
				SortAddressBookList ();
				
				// sorting the list may have moved the address book.  Find it again. jdboyd 12/18/03
				for (ab = NAliases-1; ab>=0; ab--)
				{
					PCopy(abName,(*Aliases)[ab].spec.name);
					if (StringSame(abName,cleanName)) break;
				}
	
				// We'll need to rename the nickname's photo folder in the photo album as well
			}
			else {
				FileSystemError (CANT_RENAME_ERR, (*Aliases)[ab].spec.name, theError);
				return (false);
			}
		}
		else {
			// Check to see whether or not we already have a nickname with this name
			if (BadNickname (newName, oldName, ab, nick, true, true, false) != nrOk)
				return (false);
			
			// Make sure the state of the recipient list is maintained
			if (IsNicknameOnRecipList (ab, nick)) {
				RemoveFromTo (oldName);
				AddStringAsTo (newName);
			}
			
			// Rename this in the toolbar as well
			TBRenameNickButton (oldName, newName);
			
			// Let's rename a nickname
			SetNickname (ab, nick, newName);
			LVSetItemWithNodeID (&NickList, nodeID, &data, false);

			// Update the nickname field
			SetLabelFieldString (NickField, newName);
		}

		// Need to resort the list
		InvalidListView (&NickList);

		// Massage the item name so it can be found in the list view
		ABNickLVSelect (data.nodeID, true);
		SetAliasDirty(ab);
		
		// 1/25/01 (jp)  We have a problem in the conduit when nicknames are changed
		// and only the 'NNam' resource is changed on disk.  The 'alias' left in the
		// data fork reflects the old nickname.  To fix this, we're going to read in
		// the nickname's notes and mark them dirty, which seems to force the 'alias'
		// lines to be written to disk when we save.
		if (ValidNickname (nick)) {
			notes = GetNicknameData (ab, nick, false, true);
			if (!(notesExist = notes ? true : false))
				notes = NuHandle (0);
			if (notes)
				ReplaceNicknameNotes (ab, newName, notes);
			if (!notesExist)
				ZapHandle (notes);
		}
	}
	return (true);
}



/************************************************************************
 * ABRemove - delete selected nicknames and address books
 ************************************************************************/

void ABRemove(void)

{
	VLNodeInfo	data;
	Str31				name;
	short				ab,
							nick,
							i;
	Boolean			weDid;	// well?

	weDid = false;

	ABSetKeyboardFocus (focusNickList);

	// Save info about the first selected row so we can reselect 
	// the item now in this row after the remove has been completed.
	LVGetItem (&NickList, 1, &data, true);
	
	// Check for ro
	for (i = LVCountSelection (&NickList); i; --i)
		if (GetSelectedABNick (i, &ab, &nick) && (*Aliases)[ab].ro)
		{
			WarnUser(ITS_LOCKED_DUMMY,0);
			return;
		}
	
	// Remove items in reverse list order 	
	for (i = LVCountSelection (&NickList); i; --i) {
		if (GetSelectedABNick (i, &ab, &nick)) {
			// Is it a nickname file?
			if (nick == kNickUndefined)
				weDid = RemoveAddressBook (ab);
			else {
				GetNicknameNamePStr (ab, nick, name);
				if (IsNicknameOnRecipList (ab, nick))
					RemoveFromTo (name);
				if (weDid = RemoveNickname (ab, nick))
					TBRemoveDefunctNicknameButton (name);
				SetAliasDirty(ab);
 			}
		}
	}
	if (weDid) {
		InvalidListView (&NickList);
		if (LVGetItem (&NickList, data.rowNum, &data, false))
			if (!data.isParent)
				LVSelect (&NickList, data.nodeID, data.name, true);
	}
}


//
//	ABMaybeRenameNickname
//

Boolean ABMaybeRenameNickname (MyWindowPtr win)

{
	Str255	nickname;
	Boolean	fResults;
	
	// Patch up our focus problem...
	if (AB.abDisplayed<0 || AB.nickDisplayed<0) return true;
	
	fResults = true;
	// If the focus is in the nickname field, check to see if we need to rename the nickname itself
	if (NickFocus == focusNickField)
		if (!ABRename (AB.abDisplayed, AB.nickDisplayed, GetLabelFieldString (NickField, nickname))) {
			GetNicknameNamePStr (AB.abDisplayed, AB.nickDisplayed, nickname);
			SetLabelFieldString (NickField, nickname);
			PeteSelectAll (win, win->pte);
			fResults = false;
		}
	return (fResults);
}

Boolean ABDisplayNicknameInTab (short ab, short nick)

{
	Str31		nickname,
					massagedName;
	OSErr		theError;
	Boolean	validNickname,
					changedDisplay;

	theError = noErr;
	changedDisplay = false;
	if (LVCountSelection (&NickList) != 1) {
		ab	 = kAddressBookUndefined;
		nick = kNickUndefined;
	}
	
	if (ab != AB.abDisplayed || nick != AB.nickDisplayed) {	
		validNickname = (ab != kAddressBookUndefined && nick != kNickUndefined);
		
		// Nickname field
		nickname[0] = 0;
		if (validNickname)
			GetNicknameNamePStr (ab, nick, nickname);

		SetLabelFieldString (NickField, nickname);
		
		// Let the tab ready itself for a new nickname
		ClearTab (Tabs);

		// Now we fill in the cleared fields with any tagged data
		if (validNickname)
			theError = PopulateTabs (Tabs, ab, nick);

		// clean all the dirty stuff so we're ready for user input
		ABClean ();

		AB.abDisplayed	 = ab;
		AB.nickDisplayed = nick;
		changedDisplay	 = true;
	}
	
	// Uh oh... if we weren't able to populate the tabs, unselect the item and tell the user
	if (theError)
		if (validNickname) {
			// Massage the item name so it can be found in the list view
			GetListNameBasedOnTag (SortTag, ab, nick, massagedName);
			LVUnselect (&NickList, MakeNodeID (ab, nick), massagedName, true);
		}
	return (changedDisplay);
}


//
//	PopulateTabs
//
//		� Find the relevant nickname data in notes
//		� Parse tags from the nickname data
//		� For each found tag, look for this tag in an object within each tab pane
//		� Set the value of the object to the value associated with the tag in the data
//		� Set any other object values to be empty
//

OSErr PopulateTabs (ControlHandle tabControl, short ab, short nick)

{
	TabObjectPtr	objectPtr;
	Accumulator 	leftovers;		// Untagged or unrecognized tagged information we find
	PETEHandle		pte;
	Handle				temp,
								addresses,
								notes;
	Str255				tag;
	UPtr					notesPtr,
								newPtr,
								attribute,
								value;
	OSErr					theError;
	Size					notesSize;
	long					attributeLength,
								valueLength;

	addresses = nil;
	notes			= nil;
	theError	= AccuInit (&leftovers);
	
	// The addresses field.  We'll try to suck addresses from the handle we've retrieved.  If we're not able
	// to get addresses we'll just use the content of the handle in it's entirety
	if (!theError) {
		GetRString (tag, ABReservedTagsStrn + abTagEmail);
		if (temp = GetNicknameData (ab, nick, true, true)) {
			theError = SuckAddresses (&addresses, temp, true, true, false, nil);
			if (!theError)
				if (addresses) {
					if (!PrefIsSet(PREF_NICK_NOSORT)) SortBinAddr(addresses);
					FlattenListWith (addresses, '\015');
					SetObjectValueWithTag (tabControl, tag, LDRef (addresses), GetHandleSize (addresses));
					UL (addresses);
				}
		}
		if (!theError && !addresses && temp) {
			SetObjectValueWithTag (tabControl, tag, LDRef (temp), GetHandleSize (temp));
			UL (temp);
		}
	}
	ZapHandle (addresses);
	
	// Find the relevant nickname data in notes
	if (!theError) {
		if (temp = GetNicknameData (ab, nick, false, true)) {
			notes = temp;
			theError = MyHandToHand (&notes);
		}
	}
	if (!theError && notes) {
		// Clean things up a bit
		Tr (notes, "\003", "\015");
		
		// Walk through the 'notes', looking for attribute/value pairs.
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		// Go fish...  Dangerous -- but we are being very, very careful to not use any pointer into the notes after we have unlocked the handle
		while (!theError && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, &attributeLength, &value, &valueLength))) {
			// I got one!
			MakePPtr (tag, attribute, attributeLength);
			
			// If anything appears between this tag and the previous tag, put it in leftovers
			theError = AppendLeftoverText (&leftovers, notesPtr, attribute - 2);
			
			// Try to map this tag to one of our fields
			if (!theError && !SetObjectValueWithTag (tabControl, tag, value, valueLength)) {
				// If there wasn't a matching tag, and this is not one of our hidden tags,
				// save this information in our 'leftovers'
				if (!FindSTRNIndex (ABHiddenTagsStrn, tag)) {
					if (leftovers.offset)
						theError = AccuAddChar (&leftovers, '\015');
					if (!theError)
						theError = AccuAddPtr (&leftovers, attribute - 1, value - attribute + valueLength + 2);
				}
			}
			notesPtr = newPtr;
		}
		// Any leftovers at the end of the note?
		if (!theError)
			theError = AppendLeftoverText (&leftovers, notesPtr, *notes + notesSize - 1);
	}
	// Once we're done setting up all the found tab fields, append the leftovers to the notes field
	if (!theError && leftovers.offset) {
		AccuTrim (&leftovers);
		if (objectPtr = FindObjectWithTag (tabControl, GetRString (tag, ABReservedTagsStrn + abTagNote)))
			if (pte = GetLabelFieldPete (objectPtr->control))
				theError = PeteAppendText (LDRef (leftovers.data), GetHandleSize (leftovers.data), pte);
		UnlockObject (objectPtr);
	}
	ZapHandle (notes);
	AccuZap (leftovers);
	return (theError);
}

//
//	ReplaceNicknameData
//
//		Replace an existing nickname with new (new! new! new!) minty fresh data culled from changes in the address book
//

void ReplaceNicknameData (ControlHandle tabControl, short ab, short nick)

{
	NicknameDataRec				nickData;
	AttributeValueHandle	avPairs;
	AttributeValuePtr			avPairPtr;
	TabObjectPtr					objectPtr;
	VLNodeInfo						data;
	VLNodeID							nodeID;
	Str255								name,
												tag;
	Handle								addresses,
												notes,
												temp2Handle;
	short									count,
												i;
	Boolean								noAddresses,
												oldGroup;

	// Doesn't make much sense to replace the data for an address bookn does it?
	if (nick == kNickUndefined)
		return;
		
	// We will _only_ replace the nickname data if this is _really_ the displayed nickname
	if (ab == AB.abDisplayed || nick == AB.nickDisplayed) {
		// Funky code to see if the email expansion field is empty.  If it is, we lie and say it is
		// dirty (even if it is not) so that we have a chance to initialize the contents of the address
		// handle to a comma to maintain compatibility with the old code which requires something in
		// addresses in order for a nickname to be written out into the nickname file (grumble, grumble...)
		
		// Don't do this on locked address book files.  Who cares if the old code can't write the nickname 
		// back out to the file?  This address book is read-only and we're not going to be saving changes.
		// jdboyd 3/24/04
		
		noAddresses = false;
		if (!(*Aliases)[ab].ro) {
			if (objectPtr = FindObjectWithTag (tabControl, GetRString (tag, ABReservedTagsStrn + abTagEmail))) {
				if (noAddresses = GetHandleSize (GetLabelFieldText (objectPtr->control)) == 0)
					if (addresses = GetNicknameData (ab, nick, true, true))
						noAddresses = !(GetHandleSize (addresses) == 1 && **addresses == ',');
				UnlockObject (objectPtr);
			}
		}
		
		if (tabControl && (IsTabDirty (tabControl) || noAddresses)) {
			TickleMe = GetControlValue(CtlViewBy)!=1;
			oldGroup = (*((*Aliases)[ab].theData))[nick].group;
			Zero (nickData);
			nickData.notes = NuHandle (0);
			nickData.theError = MemError ();
			
			// Copy any existing hidden notes into our working notes handle
			if (!nickData.theError) {
				temp2Handle = nil;
				if (notes = GetNicknameData (ab, nick, false, true))
					if (avPairs = ParseAllAttributeValuePairs (notes, &temp2Handle, avPairHidden, avNoPairs)) {
						avPairPtr = LDRef (avPairs);
						count = GetHandleSize (avPairs) / sizeof (AttributeValueRec);
						for (i = 0; i < count && !nickData.theError; ++i, ++avPairPtr) {
							MakePStr (tag, *notes + avPairPtr->attributeOffset, avPairPtr->attributeLength);
							nickData.theError = AddAttributeValuePair (nickData.notes, tag, *notes + avPairPtr->valueOffset, avPairPtr->valueLength);
						}
					}
				ZapHandle (temp2Handle);
			}
			
			// Iterate all of the tab objects so we can build the new notes
			IterateTabObjectsUL (tabControl, ReplaceNicknameDataProc, &nickData);
			if (!nickData.theError) {
				ReplaceNicknameAddresses (ab, GetNicknameNamePStr (ab, nick, name), nickData.addresses);
				Tr (nickData.notes, "\015", "\003");
				ReplaceNicknameNotes (ab, GetNicknameNamePStr (ab, nick, name), nickData.notes);
				// If the nickname's "group status" has changed, update the icon in the list
				if (oldGroup != (*((*Aliases)[ab].theData))[nick].group)
					if (LVGetItemWithNodeID (&NickList, nodeID = MakeNodeID (ab, nick), &data)) {
						data.iconID = ABSetIcon (ab, nick);
						LVSetItemWithNodeID (&NickList, nodeID, &data, true);
					}
			}
			if (nickData.theError) {
				ZapHandle (nickData.addresses);
				ZapHandle (nickData.notes);
				WarnUser (COULDNT_MOD_ALIAS, nickData.theError);
			}
		}
	}
}


Boolean ReplaceNicknameDataProc (TabObjectPtr objectPtr, NicknameDataPtr nickDataPtr)

{
	Handle	text;

	if (nickDataPtr->theError)
		return (true);
		
	// handle a field object
	switch (objectPtr->type) {
		case fieldObject:
		// grab the text
			if (text = GetLabelFieldText (objectPtr->control))
				ReplaceNicknameText (objectPtr, nickDataPtr, text);
			break;
			
		case controlObject:
		case controlResourceObject:
			if (objectPtr->behavior) {
				if (((*objectPtr->behavior)->flags & behaveStringOneIsTrue) && GetControlValue (objectPtr->control))
					nickDataPtr->theError = AddAttributeValuePair (nickDataPtr->notes, objectPtr->tag, &(*objectPtr->behavior)->stringOne[1], *(*objectPtr->behavior)->stringOne);
				if (((*objectPtr->behavior)->flags & behaveStringTwoIsFalse) && !GetControlValue (objectPtr->control))
					nickDataPtr->theError = AddAttributeValuePair (nickDataPtr->notes, objectPtr->tag, &(*objectPtr->behavior)->stringTwo[1], *(*objectPtr->behavior)->stringTwo);
			}
			break;
		
		case pictureObject:
			// Take the URL out of the control refcon and put it in the notes
			ReplaceNicknameText (objectPtr, nickDataPtr, (Handle) GetControlReference (objectPtr->control));
			break;
	}
	return (false);
}


void ReplaceNicknameText (TabObjectPtr objectPtr, NicknameDataPtr nickDataPtr, Handle text)

{
	BinAddrHandle addresses;
	Str255				tag;
	long					length;
	Boolean				textZapNeeded;
	
	textZapNeeded = false;
	length = GetHandleSize (text);
	// could this be the expansion field?
	if (StringSame (objectPtr->tag, GetRString (tag, ABReservedTagsStrn + abTagEmail))) {
		// We need to make a copy of the text before massaging it, otherwise the text in the PETE becomes confused
		// for subsequent editing operations.
		nickDataPtr->theError = MyHandToHand (&text);
		if (!nickDataPtr->theError) {
			textZapNeeded = true;
			if (!GetHandleSize_ (text))
				nickDataPtr->theError = PtrPlusHand_ ("\015", text, 1);
			if (!nickDataPtr->theError) {
				Tr (text, "\015", ",");
				nickDataPtr->addresses = text;	
				nickDataPtr->theError = SuckAddresses (&addresses, text, false, true, false, nil);
				ZapHandle (addresses);
				textZapNeeded = false;  // Note that we don't want to zap the copy of the text since it's now stored in addresses
			}
		}
	}
	// Just a plain ol' field, so the text will be slapped into an attribute value pair and put into notes
	else
		if (length) {
			// The 'note' field might already contain attribute value pairs, so just put this text
			// onto the end of our notes handle (it will be parsed correctly when we next open the Notes tab)
			if (StringSame (objectPtr->tag, GetRString (tag, ABReservedTagsStrn + abTagNote)))
				nickDataPtr->theError = HandAndHand (text, nickDataPtr->notes);
			else {
				// Massage the other email field, sorta like the regular email field
				if (StringSame (objectPtr->tag, GetRString (tag, ABReservedTagsStrn + abTagOtherEmail))) {
					nickDataPtr->theError = MyHandToHand (&text);
					if (!nickDataPtr->theError) {
						textZapNeeded = true;
						Tr (text, "\015", ",");
						nickDataPtr->theError = SuckAddresses (&addresses, text, false, true, false, nil);
						ZapHandle (addresses);
					}
				}
				if (!nickDataPtr->theError) {
					Tr(text,">","\002");
					nickDataPtr->theError = AddAttributeValuePair (nickDataPtr->notes, objectPtr->tag, LDRef (text), length);
					UL (text);
					Tr(text,"\002",">");
				}
			}
		}
	if (textZapNeeded)
		ZapHandle (text);
}




/************************************************************************
 * AddNicknameListItems - add nicknames and address books to list
 ************************************************************************/
void AddNicknameListItems (ViewListPtr pView, VLNodeID nodeID)

{
	VLNodeInfo	info;
	OSErr				theError;
	short				addressBooks,
							nick,
							ab;;

	theError = noErr;
	
	GetABNick (nodeID, &ab, &nick);

	SetPort_ (GetMyWindowCGrafPtr (pView->wPtr));
	
	LSetDrawingMode (false, pView->hList);
	// Check for the top level condition, in which case we add all the address books
	if (nodeID == kAdddressBookRootID) {
		info.isParent			= true;
		info.useLevelZero	= false;
		info.style				= 0;
		info.refCon				= 0;

		addressBooks = NAliases;
		for (ab = 0; ab < addressBooks && !theError; ++ab) {
			//	Don't display plug-in nicknames
			if (IsPluginAddressBook (ab))
				continue;

#ifdef VCARD
			// Auto-expand personal nickname folders (i.e. my vCards) without the parent folder
			if (IsPersonalAddressBook (ab)) {
				// Don't display personal nicknames unless the user wishes to do so
				if (PrefIsSet (PREF_PERSONAL_NICKNAMES_NOT_VISIBLE) || !IsVCardAvailable ())
					continue;
					
				// If the Personal Nicknames file is empty, create something reasonable
				if (!(*Aliases)[ab].theData || !GetHandleSize ((*Aliases)[ab].theData))
					theError = AutoGeneratePersonalInformation (ab);

				if (!theError)
					theError = AddChildren (pView, ab);
				continue;
			}
#endif
			// Don't display the cache file unless the user wishes to do so
			if (HasFeature (featureNicknameWatching) && IsHistoryAddressBook (ab) && PrefIsSet (PREF_NICK_CACHE_NOT_VISIBLE))
				continue;

			// Add the address book itself
			info.iconID				= (*Aliases)[ab].ro ? LOCKED_ADDRESS_BOOK_ICON : ADDRESS_BOOK_ICON;
			info.nodeID				= MakeABNodeID (ab);
			info.isCollapsed	= (*Aliases)[ab].collapsed;
			PCopy (info.name, (*Aliases)[ab].spec.name);
			
			// Add List Manager rows myself for (hopefully) better speed
			if (ListFlags & kfManualRowAddsExpected)
				(void) LAddRow (1, LVGetNextRowToAdd (), pView->hList);

			LVAdd (pView, &info);
		}
	}
	else {

		// Add the items for a specific parent
		GetABNick (nodeID, &ab, &nick);
		theError = AddChildren (pView, ab);
	}
	LSetDrawingMode (true, pView->hList);
	if (theError)
		WarnUser (ALIAS_DISPLAY_ERR, theError);
}


OSErr AddChildren (ViewListPtr pView, short ab)

{
	VLNodeInfo	info;
	OSErr				theError;
	short				nicknames,
							oldRows,
							count,
							nick,
							index,
							*sort;
	
	theError = noErr;
	
	// Add the items for a specific parent
	info.isParent			= false;
	info.isCollapsed	= false;
	info.useLevelZero	= false;
	info.style				= 0;
	info.refCon				= 0;

	SortAddressBook (ab);
	sort = LDRef ((*Aliases)[ab].sortData);

	// Add List Manager rows myself for (hopefully) better speed -- but subtract out deleted rows!
	count = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	if (ListFlags & kfManualRowAddsExpected) {
		nicknames = count;
		for (nick = 0; nick < count; ++nick)
			if ((*((*Aliases)[ab].theData))[nick].deleted)
				--nicknames;
		oldRows = (*pView->hList)->dataBounds.bottom;
		(void) LAddRow (nicknames, LVGetNextRowToAdd (), pView->hList);
		if ((*pView->hList)->dataBounds.bottom != oldRows + nicknames) {
			if ((*pView->hList)->dataBounds.bottom != oldRows)
				LDelRow ((*pView->hList)->dataBounds.bottom - oldRows, LVGetNextRowToAdd (), pView->hList);
			theError = memFullErr;
		}
	}
	
	for (index = 0; index < count && !theError; ++index) {
		nick = sort ? *sort++ : nick;
		if (!(*((*Aliases)[ab].theData))[nick].deleted) {
			info.nodeID = MakeNodeID (ab, nick);
			info.iconID = ABSetIcon (ab, nick);
			GetListNameBasedOnTag (SortTag, ab, nick, info.name);
			LVAdd (pView, &info);
		}
	}
	UL ((*Aliases)[ab].sortData);
	return (theError);
}

//
//	GetListNameBasedOnTag
//
//		List items are formatted in specific ways, depending on the current "View By" setting:
//
//				nickname			-- Sorting by nickname
//				first last		-- Sorting by first name
//				last, first		-- Sorting by last name
//				field					-- Sorting by anything else
//
//		When sorting by anything other than nickname, we may also append the nickname at the end -- unless
//		we are sorting one of the special "ABDeferNickInListTags" fields or if the field value is nil.
//
//		We further suppress nickname appending if the user has set PREF_SUPPRESS_NICKS_IN_VIEW_BY -- but,
//		again, only if there is indeed text available to display.
//

PStr GetListNameBasedOnTag (PStr tag, short ab, short nick, Str31 name)

{
	ListNameFormatType	listNameFormat;
	Str255							missingValue,
											scratch,
											nickname;
	Handle							value1,
											value2;
	
	// Sorting by nickname
	if (!*tag)
		return (GetNicknameNamePStr (ab, nick, name));
	
	listNameFormat = lnfOtherField;
	
	// Sorting by email is a weird thing to do, so let's make it weird here, too
	if (StringSame (tag, GetRString (scratch, ABReservedTagsStrn + abTagEmail))) {
		if (value1 = GetNicknameData (ab, nick, true, true))
			MyHandToHand (&value1);
	}
	else
		// Sorting by some other field -- get the value
		value1 = GetTaggedFieldValue (ab, nick, tag);
	
	// Certain fields are combined with the key field (first/last), in which case we get another value
	if (StringSame (tag, GetRString (scratch, ABReservedTagsStrn + abTagFirst))) {
		listNameFormat = lnfFirstLast;
		value2 = GetTaggedFieldValue (ab, nick, GetRString (scratch, ABReservedTagsStrn + abTagLast));
	}
	else
	if (StringSame (tag, GetRString (scratch, ABReservedTagsStrn + abTagLast))) {
		listNameFormat = lnfLastFirst;
		value2 = GetTaggedFieldValue (ab, nick, GetRString (scratch, ABReservedTagsStrn + abTagFirst));
	}
	else
		value2 = nil;
	
	// We still might need the nickname, let's see if we do...
	if (!value1 || (!FindSTRNIndex (ABSuppressNickInListTagsStrn, tag) && !PrefIsSet (PREF_SUPPRESS_NICKS_IN_VIEW_BY)))
		GetNicknameNamePStr (ab, nick, nickname);
	else
		*nickname = 0;
	
	//
	// We should have everything we need (I hope), so let's crank out a name
	//
	
	// First, deal with the initial field value
	if (value1)
		MakePPtr (name, *value1, MIN (sizeof (Str31) - 1, GetHandleSize (value1)));
	else
		PCopy (name, GetRString (missingValue, NICKLIST_MISSING_FIELD_VALUE));
	
	// And the secondary value
	if (value2)
		MakePPtr (scratch, *value2, GetHandleSize (value2));
	else
		*scratch = 0;
		
	// Handle the rest depending on the format to be built
	switch (listNameFormat) {
		case lnfFirstLast:
			if (value2) {
				PMaxCatC (name, sizeof (Str31) - 1, ' ');
				PSCat_C (name, scratch, sizeof (Str31) - 1);
			}
			break;
		case lnfLastFirst:
			if (value2) {
				PSCat_C (name, "\p, ", sizeof (Str31) - 1);
				PSCat_C (name, scratch, sizeof (Str31) - 1);
			}
			break;
		case lnfOtherField:
			break;
	}

	// Append on the nickname (if any)
	if (*nickname) {
		GetRString (scratch, NICKLIST_PAREN);
		PMaxCatC (name, sizeof (Str31) - 1, ' ');
		if (*scratch)
			PMaxCatC (name, sizeof (Str31) - 1, scratch[1]);
		PSCat_C (name, nickname, sizeof (Str31) - 1);
		if (*scratch > 1)
			PMaxCatC (name, sizeof (Str31) - 1, scratch[2]);
	}
	ZapHandle (value1);
	ZapHandle (value2);
	return (name);
}


void SortAddressBook (short ab)

{
	NickStructPtr	nickPtr;
	Str255				scratch;
	long					attributeOffset,
								attributeLength,
								valueOffset,
								valueLength;
	int						(*compare)();
	short					*sortPtr,
								nicknames,
								nick;
	
	// No need to sort if nothing is dirty and we already have a sort array
	if (!AnyNicknamesDirty (ab) && (*Aliases)[ab].sortData && (*Aliases)[ab].theData)
		return;
		
	nicknames = (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct));

	// Allocate some space for our sort array
	ZapHandle ((*Aliases)[ab].sortData);
	(*Aliases)[ab].sortData = NuHandle ((nicknames+1) * sizeof (short));
	if (!(*Aliases)[ab].sortData)
		return;
	
	GetSortTag ();
	
	// Fill the array with one-to-one indexes to the items in the address book and setup the sort data in our toc
	LDRef (Aliases);
	sortPtr = LDRef ((*Aliases)[ab].sortData);
	nickPtr = LDRef ((*Aliases)[ab].theData);
	for (nick = 0; nick < nicknames; ++nick) {
		if (*SortTag) {
			if (FindTaggedFieldValueOffsets (ab, nick, SortTag, &attributeOffset, &attributeLength, &valueOffset, &valueLength)) {
				nickPtr->valueOffset = valueOffset;
				nickPtr->valueLength = valueLength;
			}
			else {
				nickPtr->valueOffset = -1;
				nickPtr->valueLength = -1;
			}
			++nickPtr;
		}
		*sortPtr++ = nick;
	}
	*sortPtr = -1;	//	Quicksort needs a sentinel at the end

	UL ((*Aliases)[ab].theData);
	UL ((*Aliases)[ab].sortData);
	UL (Aliases);

	// If there is no tag, then we sort by nickname
	if (!*SortTag)
		compare = NickCompare;
	else
		compare = StringSame (SortTag, GetRString (scratch, ABReservedTagsStrn + abTagEmail)) ? EmailCompare : FieldCompare;
	gSortAB = ab;
	LDRef ((*Aliases)[ab].sortData);
	QuickSort (*(*Aliases)[ab].sortData, sizeof (short), 0, nicknames - 1, (void*) compare,(void*) NickSwap);
	UL ((*Aliases)[ab].sortData);
}



//
//	GetSortTag
//
//		Make the sort tag by playing around with the current View By menu selection
//
void GetSortTag (void)

{
	TabObjectPtr	objectPtr;
	Str255				shortName;

	// Figure out how the user prefers sorting
	*SortTag = 0;
	GetMenuItemText (PopUpMenuH (CtlViewBy), GetControlValue (CtlViewBy), shortName);
	shortName[++shortName[0]] = ':';
	if (objectPtr = FindObjectWithShortName (Tabs, shortName)) {
		PCopy (SortTag, objectPtr->tag);
		UnlockObject (objectPtr);
	}
}
/************************************************************************
 * NickCompare - compare two nicknames
 ************************************************************************/
int NickCompare (short *n1, short *n2)
{
	Str255	name1,name2;
	PStr	s1,s2;

	// The sentinel might get passed into the sort routine.  If so, return as needed
	if (*n1 == -1)
		return (*n2 == -1 ? 0 : 1);
	if (*n2 == -1)
		return (-1);
	
	s1 = GetNicknameNamePStr(gSortAB, *n1, name1);
	s2 = GetNicknameNamePStr(gSortAB, *n2, name2);
	
	// The following doesn't work.  's1' and 's2' should always have some value
	// since GetNicknameNamePStr returns 'name1' or 'name2'.  Instead, let's
	// just let StringCompare do its thing
	//	if (s1) return s2 ? StringComp(s1,s2) : -1;
	//	else return s2 ? 1 : 0;
	return (StringComp(s1,s2));
}


/************************************************************************
 * NickSwap - swap two nicknames
 ************************************************************************/
void NickSwap (short *n1, short *n2)
{
	short temp = *n2;
	*n2 = *n1;
	*n1 = temp;
}

/************************************************************************
 * FieldCompare - compare two nicknames using tagged fields as the data
 ************************************************************************/
int FieldCompare (short *n1, short *n2)

{
	NickStructPtr	nickPtr1,
								nickPtr2;
	Handle				notes1,
								notes2;

	// The sentinel might get passed into the sort routine.  If so, return as needed
	if (*n1 == -1)
		return (*n2 == -1 ? 0 : 1);
	if (*n2 == -1)
		return (-1);

	notes1 = GetNicknameData (gSortAB, *n1, false, true);
	notes2 = GetNicknameData (gSortAB, *n2, false, true);

	nickPtr1 = &(*((*Aliases)[gSortAB].theData))[*n1];
	nickPtr2 = &(*((*Aliases)[gSortAB].theData))[*n2];
	if (notes1 && nickPtr1->valueOffset >= 0) {
		if (notes2 && nickPtr2->valueOffset >= 0)
			return (strincmp (*notes1 + nickPtr1->valueOffset, *notes2 + nickPtr2->valueOffset, MIN (nickPtr1->valueLength, nickPtr2->valueLength)));
		else
			return (-1);
	}
	else {
		if (notes2 && nickPtr2->valueOffset >= 0)
			return (1);
		else {
			return NickCompare (n1, n2);
		}
	}
}



/************************************************************************
 * EmailCompare - compare two nicknames using email addresses as the data
 ************************************************************************/
int EmailCompare (short *n1, short *n2)

{
	Handle	value1,
					value2;
	Size		value1Size,
					value2Size;

	// The sentinel might get passed into the sort routine.  If so, return as needed
	if (*n1 == -1)
		return (*n2 == -1 ? 0 : 1);
	if (*n2 == -1)
		return (-1);
	
	if (value1 = GetNicknameData (gSortAB, *n1, true, true))
		value1Size = GetHandleSize (value1);
	if (value2 = GetNicknameData (gSortAB, *n2, true, true))
		value2Size = GetHandleSize (value2);
	
	if (value1) {
		if (value2)
			return (strincmp (*value1, *value2, MIN (value1Size, value2Size)));
		else
			return (-1);
	}
	else {
		if (value2 && value2Size)
			return (1);
		else
			return NickCompare (n1, n2);
	}
}


void PositionPushButtons (MyWindowPtr win, short hTab1, short hTab2, short vTab5)

{
	short tabWidth,
				buttonHeight,
				buttonWidth;
	
	short iChatButtonWidth;
	
	ButtonFit(CtlChat);
	iChatButtonWidth = ControlWi(CtlChat) - 4*INSET;	// darn giant aqua button can bite me
	
	tabWidth		 = hTab2 - hTab1;
	buttonHeight = ControlHi (CtlCc);
	buttonWidth	 = (tabWidth - 2 * INSET - 2 * INSET - kHorzZoomWidth) / 4;
	
	if (buttonWidth < iChatButtonWidth)
		buttonWidth = (tabWidth - 2 * INSET - 2 * INSET - kHorzZoomWidth - iChatButtonWidth) / 3;
	else
		iChatButtonWidth = buttonWidth;
	
	// horizontal zoom button
	MoveMyCntl (CtlZoomHorizontal, hTab2 - kHorzZoomWidth, vTab5 + (buttonHeight - kHorzZoomWidth) / 2, kHorzZoomWidth, kHorzZoomWidth);

	// To, Cc, Bcc, iChat
	MoveMyCntl (CtlTo, hTab1, vTab5, buttonWidth, buttonHeight);
	MoveMyCntl (CtlCc, hTab1 + buttonWidth + INSET, vTab5, buttonWidth, buttonHeight);
	MoveMyCntl (CtlBcc, hTab1 + 2 * (buttonWidth + INSET), vTab5, buttonWidth, buttonHeight);
	MoveMyCntl (CtlChat, hTab1 + 3 * (buttonWidth + INSET), vTab5, iChatButtonWidth, buttonHeight);
}


void BuildViewByMenu (ControlHandle tabControl, ControlHandle viewByControl)

{
	MenuHandle	viewByMenu;
	Str255			tag,
							viewByString,
							title;
	short				index;

	if (tabControl && viewByControl)
		if (viewByMenu = PopUpMenuH (viewByControl)) {
			SetControlValue (viewByControl, 1);
			// The current view by label
			GetPref (viewByString, PREF_NICK_VIEW_BY);
			index = 1;
			do {
				GetRString (tag, ABViewByTagsStrn + index++);
				if (*tag)
					if (tag[1] == '-') {
						SetControlMaximum (viewByControl, GetControlMaximum (viewByControl) + 1);
						AppendMenu (viewByMenu, "\p(-");
					}
					else {
						FindShortNameWithTag (tabControl, tag, title);
						if (*title) {
							SetControlMaximum (viewByControl, GetControlMaximum (viewByControl) + 1);
							// Remove the ':' following the field label
							if (title[title[0]] == ':')
								--title[0];
							MyAppendMenu (viewByMenu, title);
							if (StringSame (viewByString, title))
								SetControlValue (viewByControl, index);
						}
					}
			} while (*tag);
		}
}


/************************************************************************
 * AddressBookLVCallBack - callback function for list View
 ************************************************************************/
long AddressBookLVCallBack (ViewListPtr pView, VLCallbackMessage message, long data)

{
	VLNodeInfo	*pInfo;
	long				fResults;
	short				ab,
							nick;

	fResults = 0;
	switch (message) {
		case kLVAddNodeItems:
			AddNicknameListItems (pView, (VLNodeID) data);
			break;
		
		case kLVGetParentID:
			fResults = ((VLNodeInfo *) data)->nodeID;
			break;
			
		case kLVOpenItem:
			ABMessageTo (TO_HEAD, CurrentModifiers ());
			break;

		case kLVMoveItem:
		case kLVCopyItem:
			ABMove ((VLNodeInfo*) data, message == kLVCopyItem);
			break;

		case kLVDeleteItem:
// For now we won't respond to requests to delete nicknames via the Delete key.  Eventually we
// need to support a protected method for deleting listview items that probably involves command-Delete
// or something.
//
// Ah!  Except, this is the mechanism we use to remove items from the list view when the user has dragged an item
// to the trash.  What to do, what to do...  Why, overload the 'data' parameter, of course.
//
			if (data == dataDeleteFromDrag || (data == dataDeleteFromKeyboard && PrefIsSet (PREF_REMOVE_NICKS_WITH_DELETE_KEY)))
				ABRemove ();
			break;
		
		case kLVRenameItem:
			DoRenameCallback ((StringPtr) data);
			break;

		case kLVGetFlags:
			fResults = ListFlags;
			break;

		case kLVQueryItem:
			pInfo = (VLNodeInfo *)data;
			switch (pInfo->query) {
				case kQuerySelect:
					fResults = true;
					break;
				case kQueryDrag:
					fResults = true;
					break;
				case kQueryDrop:
					fResults = false;
					GetABNick (pInfo->nodeID, &ab, &nick);
					if (ValidAddressBook (ab) && !(*Aliases)[ab].ro) {
						fResults = !ValidNickname (nick);
						if (ValidNickname (nick) && (*((*Aliases)[ab].theData))[nick].group && PrefIsSet (PREF_NICK_DROP_ON_GROUPS))
							fResults = true;
					}
					break;
				case kQueryDropParent:
					GetABNick (pInfo->nodeID, &ab, &nick);
					fResults = ValidNickname (nick) && ValidAddressBook (ab) && !(*Aliases)[ab].ro;
					break;
				case kQueryDragExpand:
					fResults = PrefIsSet (PREF_NICK_EXPAND_DRAGS);
					break;
				case kQueryRename:
					// Only allow in list renames when the current view is By Nickname and the user pref is set -- or if the node is an address book
					GetABNick (pInfo->nodeID, &ab, &nick);
					if (*SortTag || (!PrefIsSet (PREF_ALLOW_NICK_RENAME_IN_LIST) && ValidNickname (nick)))
						fResults = false;
					else {
						if (ab >= 0 && nick == kNickUndefined)
							fResults = (!((*Aliases)[ab].ro || IsEudoraAddressBook (ab) || IsHistoryAddressBook (ab)));
						else
							fResults = !(*Aliases)[ab].ro;
					}
					break;
					
				case kQueryDCOpens:
					fResults = !pInfo->isParent;
					break;
			}
			break;

		case kLVAddDragItem:
			fResults = ABAddDragFlavors (data);
			break;
			
		case kLVSendDragData:
			fResults = ABDoSendDragData (data);
			break;

		case kLVExpandCollapseItem:
			pInfo = (VLNodeInfo *)data;
			(*Aliases)[GetAddressBook (pInfo->nodeID)].collapsed = pInfo->isCollapsed;
			break;
		
		case kLVSelectItem:
			ABSelectNickname ((VLNodeID) data);
			break;
			
		case kLVUnselectItem:
			ABUnselectNickname ((VLNodeID) data);
			break;
	}
	return (fResults);
}


void DoRenameCallback (PStr newName)

{
	VLNodeInfo	data;
	short				ab,
							nick;
							
	if (LVGetItem (&NickList, 1, &data, true)) {
		GetABNick (data.nodeID, &ab, &nick);
		ABRename (ab, nick, newName);
	}
}


Boolean RemoveAddressBook (short ab)

{
	CellRec			*pCellData;
	VLNodeInfo	data;
	Cell				c;
	Str31				name;
	OSErr				theError;
	short				numAdressBooks,
							abIndex,
							nick,
							count,
							item,
							offset,
							len;
	
	if (ComposeStdAlert (Stop, ALIAS_REMOVE_FILE_ALERT, (*Aliases)[ab].spec.name) != 1)
		return (false);

	// With this address book going away, we should also remove any nicknames in the list from the recipient list
	// (This code was formerly misplaced _after_ the nickname file had been zapped... BOOM!)
	count = (*Aliases)[ab].theData ? (GetHandleSize_ ((*Aliases)[ab].theData) / sizeof (NickStruct)) : 0;
	for (nick = 0; nick < count; nick++) {
		GetNicknameNamePStr (ab, nick, name);
		if (IsNicknameOnRecipList (ab, nick))
			RemoveFromTo (name);
		TBRemoveDefunctNicknameButton (name);
	}

	// De-allocate underlying structures
	ZapAliasFile (ab);

	// Move file to trash
	LDRef (Aliases);
	theError = MyFSpMoveToTrash (&(*Aliases)[ab].spec);
	if (theError)
		theError = FSpDelete (&(*Aliases)[ab].spec);
	UL (Aliases);

	if (!theError) {
		// Get rid of the item in our list
		if (LVGetItemWithNodeID (&NickList, MakeABNodeID (ab), &data)) {
			item = data.rowNum;
			// This is going to get slightly nasty... We're going to run through all the remaining items that
			// follow in the list view and reset all of the nodeID's to reflect an "adjusted" address book
			// index.  It is important that we do this since the remaining list entries now all point to one
			// address book beyond...
			c.h = 0;
			for (--item; item < (*NickList.hList)->dataBounds.bottom; item++) {
				c.v = item;
				LGetCellDataLocation (&offset, &len, c, NickList.hList);
				if (pCellData = len > 0 ? (CellRec *) (*(*NickList.hList)->cells + offset) : nil)
					pCellData->nodeID = MakeNodeID (GetAddressBook (pCellData->nodeID) - 1, GetNickname (pCellData->nodeID));
			}
		}
		
		// Shift around the data in the big hangin' nickname structure
		numAdressBooks = NAliases;
		if (ab != numAdressBooks - 1)
			for (abIndex = ab + 1; abIndex < numAdressBooks; abIndex++)
				(*Aliases)[abIndex - 1] = (*Aliases)[abIndex];
		SetHandleBig_ (Aliases, (numAdressBooks - 1) * sizeof (AliasDesc));
				
		// We may have to move the location of the History List file, too
// No longer needed since we mark the history list file
//		if (HasFeature (featureNicknameWatching))
//			if (gEudoraNicknamesCacheFileIndex >= 0)
//				if (ab == gEudoraNicknamesCacheFileIndex)
//					gEudoraNicknamesCacheFileIndex = kAddressBookUndefined;
//				else
//					if (ab < gEudoraNicknamesCacheFileIndex)
//						--gEudoraNicknamesCacheFileIndex;
		return (true);
	}
	else
		WarnUser (ALIAS_REMOVE_NICK_FILE_ERR, theError);
	return (false);
}


//
//	RemoveNickname
//
//		The logic within this function is sort of backwards from what
Boolean RemoveNickname (short ab, short nick)

{
	VLNodeInfo	data;
	Boolean			result;
	
	if (result = (ab >= 0 && !(*Aliases)[ab].ro && nick >= 0))
		// Just mark it deleted
		if (result = LVGetItemWithNodeID (&NickList, MakeNodeID (ab, nick), &data)) {
			(*((*Aliases)[ab].theData))[nick].addressesDirty		= true;
			(*((*Aliases)[ab].theData))[nick].deleted = true;
		}
	return (result);
}


//
//	ParseAttributeValue
//
//		Parses a range of text for an attribute/value pair, returning a pointer
//		to the character following the pair (the character after the '>'), or 'nil'
//		if no pair was parsed.
//
//		Tagged fields have the absolute form:
//
//			<tag: someSpanOfCharacters >
//
//		This function does not make any attempt to validate either the tag or
//		the value � all it cares about is finding brackets and colons, making
//		no attempt to handle nesting or escaping or anything else.  Therefore,
//		tags can contain breaking spaces and other heinous characters.
//
//		The algorithm goes something like this:
//			1. Scan until we find a '<' and save this spot
//			2. Scan until we find a ':' and save this one, too.
//			3. Scan until we find a '>' and save this spot once again
//
//		IMPORTANT NOTE:
//		
//		It is also important to note that the pointers returned in 'attribute' and 'value' are
//		generally pointers into the notes handle itself.  If you dereference notes and call
//		this routine to parse the data, make sure the data actually has a staic place to live!!
//
//		Much safer is ParseAttributeValuePairHandle, which returns allocates its own destination
//		storage, returning the data in a pair of Handles.
//

UPtr ParseAttributeValuePair (UPtr start, long length, UPtr *attribute, long *attributeLength, UPtr *value, long *valueLength)

{
	UPtr		end,
					spot;
	short		bc;				// bracket count
	Boolean	found;
	
	found = false;
	bc		= 0;
	spot	= start;
	end		= start + length - 1;
	do {
		if (*spot == kFieldTagBegin) {
			++bc;
			*attribute = ++spot;
			do {
				if (*spot == kFieldTagSeparator) {
					*attributeLength = spot - *attribute;
					*value = ++spot;
					do {
						if (*spot == kFieldTagEnd)
							if (--bc == 0) {
								*valueLength = spot - *value;
								found = true;
							}
					} while (!found && ++spot <= end);
				}
			} while (!found && ++spot <= end);
		}
	} while (!found && ++spot <= end);
	return (found ? spot + 1 : nil);
}

//
//	ParseAttributeValuePairHandle
//
//		ParseAttributeValuePair, except putting the output into handles we allocate.  Don't forget
//		to dispose of the results when you're done.
//
//		It's also okay to pass in nil for either the attribute or value if we don't care about
//		those fields.
//
UPtr ParseAttributeValuePairHandle (UPtr start, long length, Handle *hAttribute, Handle *hValue)

{
	UPtr		attribute,
					value,
					spot;
	OSErr		theError;
	long		attributeLength,
					valueLength;
	
	if (hAttribute)
		*hAttribute = nil;
	if (hValue)
		*hValue = nil;

	if (spot = ParseAttributeValuePair (start, length, &attribute, &attributeLength, &value, &valueLength)) {
		theError = noErr;
		if (hAttribute)
			theError = PtrToHand (attribute, hAttribute, attributeLength);
		if (!theError && hValue)
			theError = PtrToHand (value, hValue, valueLength);
		if (theError) {
			if (hAttribute)
				ZapHandle (*hAttribute);
			if (hValue)
				ZapHandle (*hValue);
		}
	}
	return (spot);
}

//
//	ParseAttributeValuePairStr
//
//		ParseAttributeValuePair, except putting the output into pascal strings
//
//		It's also okay to pass in nil for either the attribute or value if we don't care about
//		those fields.
//
UPtr ParseAttributeValuePairStr (UPtr start, long length, PStr attributeStr, PStr valueStr)

{
	UPtr		attribute,
					value,
					spot;
	long		attributeLength,
					valueLength;
	
	if (attributeStr)
		*attributeStr = nil;
	if (valueStr)
		*valueStr = nil;

	if (spot = ParseAttributeValuePair (start, length, &attribute, &attributeLength, &value, &valueLength)) {
		if (attributeStr)
			MakePPtr (attributeStr, attribute, attributeLength);
		if (valueStr)
			MakePPtr (valueStr, value, valueLength);
	}
	return (spot);
}


//
//	AddAttributeValuePair
//
//		Create an attribute value pair and append it onto a notes handle.
//

OSErr AddAttributeValuePair (Handle notes, PStr attribute, Ptr value, long length)

{
	OSErr	theError;
	UPtr	notesPtr;
	Size	notesSize;
	
	theError = noErr;
	
	// Grow the notes field to accomodate a structured <attribute:value> pair
	notesSize = GetHandleSize (notes);
	SetHandleBig (notes, notesSize + attribute[0] + length + 3);
	theError = MemError ();
	if (!theError) {
		notesPtr = *notes + notesSize;
		*notesPtr++ = kFieldTagBegin;
		BlockMoveData (&attribute[1], notesPtr, attribute[0]);
		notesPtr += attribute[0];
		*notesPtr++ = kFieldTagSeparator;
		BlockMoveData (value, notesPtr, length);
		notesPtr += length;
		*notesPtr++ = kFieldTagEnd;
	}
	return (theError);
}

OSErr AppendLeftoverText (AccuPtr a, UPtr from, UPtr to)

{
	OSErr	theError;
	
	theError = noErr;
	if (from <= to) {
		while (IsSpace(*from))		// skip leading white space
			++from;
		if (from <= to) {
			if (a->offset)
				theError = AccuAddChar (a, '\015');
			if (!theError)
				theError = AccuAddPtr (a, from, to - from + 1);
		}
	}
	return (theError);
}


//
//	ParseAllAttributeValuePairs
//
//		Parses a text handle for all attribute/value pairs, returning a handle to
//		a structure containing the offset and lengths for each.  We also create
//		and return in 'leftovers' a handle to any text that was _not_ parsed as
//		an attribute value pair.
//

AttributeValueHandle ParseAllAttributeValuePairs (Handle notes, Handle *leftovers, long requestedTypes, long unrequestedTypesToBePutInLeftovers)

{
	TabFieldHandle				fields;
	AttributeValueHandle	avPairs;
	AttributeValueRec			av;
	Accumulator						leftoversAccumulator;
	OSErr									theError;								
	Str31									tag;							
	UPtr									notesPtr,
												newPtr,
												attribute,
												value;
	Size									notesSize;

	avPairs = nil;
	fields	= nil;

	theError	= AccuInit (&leftoversAccumulator);
	if (!theError) {
		avPairs = NuHandle (0);
		theError = MemError ();
	}
	if (!theError)
		fields = Tabs && AB.inited ? GetTabFieldStrings (Tabs, tabTagName) : GetTabFieldsFromResources (tabTagName, CREATOR, kNickTabType);

	// Walk through the 'notes', looking for attribute/value pairs.
	if (notes && avPairs && fields) {
		notesSize	= GetHandleSize (notes);
		notesPtr	= LDRef (notes);
		// Go fish...
		while (!theError && (newPtr = ParseAttributeValuePair (notesPtr, notesSize - (notesPtr - *notes), &attribute, &av.attributeLength, &value, &av.valueLength))) {
			// I got one!
			av.attributeOffset = attribute - *notes;
			av.valueOffset		 = value - *notes;
			
			// If anything appears between this tag and the previous tag, put it in leftovers
			if (!theError)
				theError = AppendLeftoverText (&leftoversAccumulator, notesPtr, attribute - 2);

			// Make the attribute tag
			MakePStr (tag, attribute, av.attributeLength);
			
			// What type of tag did we find?
			av.type = 0;
			if (FindSTRNIndex (ABHiddenTagsStrn, tag))
				av.type |= avPairHidden;
			if (FindSTRNIndex (ABUnseachableTagsStrn, tag))
				av.type = avPairUnsearchable;
			if (!av.type)
				av.type = FindFieldString (fields, tag) ? avPairKnown : avPairUnknown;

			// Add only requested pairs to our list
			if (av.type & requestedTypes)
				theError = PtrPlusHand (&av, avPairs, sizeof (av));
			else
				// And if we've been asked, save unrequested types in leftovers (otherwise, they are ignored completely)
				if (av.type & unrequestedTypesToBePutInLeftovers) {
					theError = AccuAddChar (&leftoversAccumulator, '\015');
					if (!theError)
						theError = AccuAddPtr (&leftoversAccumulator, attribute - 1, value - attribute + av.valueLength + 2);
				}
				
			notesPtr = newPtr;
		}
		// Any leftovers at the end of the note?
		if (!theError)
			theError = AppendLeftoverText (&leftoversAccumulator, notesPtr, *notes + notesSize - 1);
		UL (notes);
	}
	
	// Clean up
	if (!theError) {
		AccuTrim (&leftoversAccumulator);
		*leftovers = leftoversAccumulator.data;
		leftoversAccumulator.data = nil;
	}
	else
		ZapHandle (avPairs);
	ZapHandle (fields);
	AccuZap (leftoversAccumulator);
	return (avPairs);	
}


short ABSetIcon (short ab, short nick)

{
	Boolean isRecip;

#ifdef VCARD	
	if (IsPersonalAddressBook (ab) && IsVCardAvailable ())
		return (PERSONAL_NICKNAME_ICON);
#endif
	isRecip = IsNicknameOnRecipList (ab, nick);
	if ((*((*Aliases)[ab].theData))[nick].group)
		return (isRecip ? RECIPIENT_GROUP_ICON : GROUP_ICON);
	return (isRecip ? RECIPIENT_NICKNAME_ICON : NICKNAME_ICON);
}

OSErr GetPhotoSpec (FSSpec *spec, short ab, short nick, Boolean *alreadyExists)

{
	FSSpec	nickFolder,
					photoFolder,
					abPhotoFolder;
	Str255	photoFolderName,
					nickname;
	OSErr		theError;
	long		dirID;
	
	*alreadyExists = false;
	
	// Find (or create) the Nicknames Folder
	if (theError = SubFolderSpec (NICK_FOLDER, &nickFolder)) {
		SimpleMakeFSSpec (Root.vRef, Root.dirId, GetRString (nickFolder.name, NICK_FOLDER), &nickFolder);
		theError = FSpDirCreate (&nickFolder, smSystemScript, &dirID);
	}

	if (!theError) {
		IsAlias (&nickFolder, &nickFolder);
		theError = FSMakeFSSpec (nickFolder.vRefNum, SpecDirId (&nickFolder), GetRString (photoFolderName, PHOTO_FOLDER), &photoFolder);
	}
	
	// Create the Photo Album Folder
	if (theError == fnfErr) 
		theError = FSpDirCreate (&photoFolder, smSystemScript, &dirID);
	if (!theError) {
		IsAlias (&photoFolder, &photoFolder);
		
		// Create a folder inside the Photo Album for the current address book
		theError = FSMakeFSSpec (photoFolder.vRefNum, SpecDirId (&photoFolder), (*Aliases)[ab].spec.name, &abPhotoFolder);
		if (theError == fnfErr) 
			theError = FSpDirCreate (&abPhotoFolder, smSystemScript, &dirID);
	}
	
	// Make an FSSpec for the nickname itself
	if (!theError) {
		IsAlias (&abPhotoFolder, &abPhotoFolder);
		theError = FSMakeFSSpec (abPhotoFolder.vRefNum, SpecDirId (&abPhotoFolder), GetNicknameNamePStr (ab, nick, nickname), spec);
		if (theError == fnfErr)
			theError = noErr;
		else {
			*alreadyExists = true;
			IsAlias (spec, spec);
		}
	}
	return (theError);
}


//
//	Old scary crufty crap appears below... (not all that related to the address book window)
//

/************************************************************************
 * NewNick - create a new alias for the user
 ************************************************************************/
#ifdef VCARD
void NewNick(Handle addresses,FSSpec *vcardSpec,short which)
{
	AskNickname (addresses, vcardSpec, which);			// Wow... this routine sure did get stripped down...
}
#else
void NewNick(Handle addresses,short which)
{
	AskNickname (addresses, which);			// Wow... this routine sure did get stripped down...
}
#endif

/************************************************************************
 * NewNickLow - Acually create a new alias for the user
 ************************************************************************/
short NewNickLow(Handle addresses,Handle notes,short which,Str63 name,Boolean makeRecip,NickReplaceEnum nre,Boolean doSave)
{
	return AddNickname(addresses,notes,which,name,makeRecip,nre,doSave);
}

/************************************************************************
 * AddNickname - Create a new alias for the user
 *
 *	Note!!  No doubt there are still a lot of Gruby-ized conditions in this routine, but -- in general --
 *					The 'addresses' and 'notes' passed to this routine are COPIED into new handles which are then
 *					placed into the TOC.  Therefore, it is safe for calling routines to zap the handles they pass.
 *
 ************************************************************************/
short AddNickname(Handle addresses,Handle notes,short which,Str63 name,Boolean makeRecip,NickReplaceEnum nre,Boolean doSave)
{
	short index;
	long addrSize;
	Handle	tempHandle = nil,temp2Handle = nil;
	long	totalNicks;
	short	realCount,i;
	short nickCollision;
	Boolean	replacing = false;
	Byte comma = ',';
	OSErr	err = noErr;
	Boolean	notesExist;

	RegenerateAllAliases(false);	

	if ((*Aliases)[which].ro) which = FindAddressBookType (eudoraAddressBook);

	totalNicks = (*Aliases)[which].theData ? (GetHandleSize_((*Aliases)[which].theData)/sizeof(NickStruct)) : 0;
	realCount = 0;
	for (i=0;i<totalNicks;i++)
		{
			if (!(*((*Aliases)[which].theData))[i].deleted)
				realCount++;
		}
		
	//addrSize = incomingAddresses ? GetHandleSize_(incomingAddresses) : 0;
	//if (addrSize)
	//		SuckAddresses(&addresses,incomingAddresses,True,True,False,nil);
	addrSize = addresses ? GetHandleSize_(addresses) : 0;
	if (addrSize) FlattenListWith(addresses,',');
	
	*name = MIN (*name, sizeof (Str31) - 1);
	index = NickMatchFound((*Aliases)[which].theData,NickHash(name),name,which);

	if (index >= 0)
	{
		if (nre == nrDifferent) {
			nickCollision = NickRepAlert(name);
			// If it still thinks it is different, cancel was really hit
			if (nickCollision == nrDifferent)
				return(-1);
		}
		else
			nickCollision = (short) nre;

		// 2/2/01  (jp)  � If we are replacing or adding to an existing nickname that is currently selected, unselect before changing anything.
		if (AB.inited)
			if (which == AB.abDisplayed && index == AB.nickDisplayed && (nickCollision == nrReplace || nickCollision == nrAdd)) {
				Str31 massagedName;
				GetListNameBasedOnTag (SortTag, which, index, massagedName);
				LVUnselect (&NickList, MakeNodeID (which, index), massagedName, true);
			}


		switch (nickCollision)
		{
			case nrDifferent:
#ifdef VCARD
					nre = AskNickname(nil, nil, which);
#else
					nre = AskNickname(nil, which);
#endif
					if (nre == nrCancel)
						return(-1);
				break;
			case nrReplace:
				replacing = true;
				break;
			case nrAdd:
				replacing = true;
				// Get addresses
				// Append addresses
					tempHandle = GetNicknameData(which,index,true,true);
					if (tempHandle)
					{
						temp2Handle = NuHandle(0);
						if (!temp2Handle)
							{WarnUser(ALIAS_NEW_NICK_ERR,MemError());return(-1);}

						if (err = HandPlusHand(tempHandle,temp2Handle)) break;
						if (err=PtrPlusHand(&comma,temp2Handle,1)) break;
						if (err=HandPlusHand(addresses,temp2Handle)) break;
						addresses = temp2Handle;
						temp2Handle = nil;
					}
					if (notes)
						ZapHandle(notes);
				// Get notes
				// Copy notes
					temp2Handle = GetNicknameData(which,index,false,true);
					if (temp2Handle)
						{
							notes = NuHandle(0);
							if (!notes)
								{WarnUser(ALIAS_NEW_NICK_ERR,MemError());return(-1);}
							if (err = HandPlusHand(temp2Handle,notes)) break;
						}
				
				
				break;
		}
	}
	
	if (addresses) NickUniq(addresses,"\p,",True);
	if (makeRecip) AddStringAsTo(name);
	SetAliasDirty(which);
	
	// Both of these routines will make a copy of the addresses	
	if (replacing)
		ReplaceNicknameAddresses(which,name,addresses);
	else
		AddNickToTOCfromName(which,name,addresses);
	
	// When adding a new nickname, we'll add the 'added' change bit to the notes
	notesExist = notes ? true : false;
	if (!replacing && PrefIsSet (PREF_CHANGE_BITS_FOR_CONDUIT)) {
		if (!notes)
			notes = NuHandle (0);
		if (notes) {
			Str255	scratch,
							idTag;
			
			SetNicknameChangeBit (notes, changeBitAdded, true);
		
			// Generate a unique ID for this nickname and add it to the notes
			NumToString (NickGenerateUniqueID(), scratch);
			SetTaggedFieldValueInNotes (notes, GetRString (idTag, ABHiddenTagsStrn + abTagUniqueID), &scratch[1], *scratch, nickFieldReplaceExisting, 0, nil);
		}
	}

	// This routine will make a copy of the notes	
	if (notes)
		ReplaceNicknameNotes(which,name,notes);

	if (!notesExist)
		ZapHandle (notes);

	if (doSave)
		SaveAliases(true);

	return (index >= 0 ? index : NickMatchFound((*Aliases)[which].theData,NickHash(name),name,which));
}


/************************************************************************
 * CanMakeNick - should we enable the "make nickname" item?
 ************************************************************************/
Boolean CanMakeNick(void)
{
  return(AB.inited&&((LVCountSelection(&NickList)>1&&FrontWindow_()==GetMyWindowWindowPtr(Win))||HasNickScanCapability (Win->pte)) || PhCanMakeNick());
}

Boolean	NickUserFieldExists(PStr theField)
{
	short	numberOfUserFields;
	short i;
	Str31 tempField;
	fieldInfo	**userFieldsInfo;
	OSErr	err = noErr;
	short	reqFields,optFields;
	long	height,width;
	Str255 scratch;
	Str31 fieldName;

	 /*
	 * Determine number of extra fields
	 */
	#ifdef ONE
		reqFields = 1;
		optFields = 0;
	#else
		reqFields = CountStrn(NickManLabelStrn);
  	optFields = CountStrn(NickOptLabelStrn);
	#endif
	numberOfUserFields = reqFields + optFields;

	userFieldsInfo = NuHandle(sizeof(fieldInfo) * (reqFields + optFields));
	if (!userFieldsInfo)
	{
		err = MemError();
		WarnUser(MEM_ERR,err);
		return (err);
	}

	for (i=0;i<numberOfUserFields;i++)
		{

			if (i < reqFields)
				// Read the resource
				GetRString(scratch,NickManLabelStrn+i+1);
			else
				// Read the resource
				GetRString(scratch,NickOptLabelStrn + (i - reqFields) + 1);
	


			// Parse resource for field name, height, and width
			NickParseForInfo(scratch,&height,&width,fieldName);
			 (*userFieldsInfo)[i].width = width;
			 (*userFieldsInfo)[i].height = height;
			 PCopy((*userFieldsInfo)[i].name,fieldName);

			if (i < reqFields)
				// Read in tags;
				GetRString(scratch,NickManKeyStrn+i+1);
			else
				// Read in tags;
				GetRString(scratch,NickOptKeyStrn + (i - reqFields) + 1);
			PCopy((*userFieldsInfo)[i].tag,scratch);
		}

	for (i=0;i<numberOfUserFields;i++)
	{
		PCopy(tempField,(*userFieldsInfo)[i].name);
		
		tempField[0]--;
			if (StringSame(tempField,theField))
				{
					if (userFieldsInfo)
						ZapHandle(userFieldsInfo);
					return (true);
				}
		}
	
	if (userFieldsInfo)
		ZapHandle(userFieldsInfo);
	return (false);
	
}


Boolean	IsNicknameOnRecipList(short which,short index)
{
	Str63	alias;
//	PCopy(alias,*((*((*Aliases)[which].theData))[index].theName));
	GetNicknameNamePStr(which,index,alias);
	return (IsRecip(alias));
}


Boolean IsNicknamePrivate (short ab, short nick)

{
	return (GetNicknameChangeBits (GetNicknameData (ab, nick, false, true)) & changeBitPrivate ? true : false);
}

void AESaveCurrentAlias(short which,short index)
{
	short	ab,
				nick;
							
	if (!AB.inited)
		return;

	GetSelectedABNick (1, &ab, &nick);
	if (nick == index && ab == which)
		ReplaceNicknameData (Tabs, ab, nick);
}


void	ForceSelectedAliasUpdate(short which,short index,Boolean didDirty)
{
	SetAliasDirty(which);

	if (!AB.inited)
		SaveAliases(true);
}

/************************************************************************
 * NickGetDataFromField - Get some data from a field in the notes
 * If doingDrag, returns true to indicate that there is no data (i.e. don't copy into clipping)
 * otherwise, the nickname in �� is returned in theData
 ************************************************************************/
OSErr NickGetDataFromField(UPtr theField,UPtr sViewData,short	which,short	nickNum,Boolean doingDrag,Boolean doingAE,Boolean *nickNameEmpty)
{
	Str63		tempStr, anotherTemp;
	char		CFieldName[31],tempFieldName[31];
	Handle	theNotes,temp2Handle = nil;
	char		*tagPtr = nil;
	char		*dataPtr = nil;
	char		*endPtr = nil;
	short		i;
	Boolean	returnValue = false;
	short	length;
	long 	offset;
	long	count;
	short	bracketCount = 0;
	Str255 scratch;
	
	*sViewData = 0;
	*tempStr = 0;
	BlockMoveData(theField + 1,CFieldName,*theField);
	CFieldName[*theField] = 0;
	strcpy(tempFieldName,"<");
	strcat(tempFieldName,CFieldName);
	strcpy(CFieldName,tempFieldName);

	*nickNameEmpty = false;

	theNotes = GetNicknameData(which,nickNum,false,true);
	if (theNotes && *theNotes)
		{
			offset = SearchPtrPtr(CFieldName,strlen(CFieldName),LDRef(theNotes),0,GetHandleSize_(theNotes),false,false,nil); // Find field tag in notes
			if (offset >= 0)
				tagPtr = LDRef(theNotes) + offset;
			if (tagPtr)
				dataPtr = tagPtr + *theField + 1;
			if ( dataPtr && *dataPtr == ':' ) dataPtr++;
			if (dataPtr)
				endPtr = strchr(dataPtr,'>');
			length = endPtr  - dataPtr;
			for (count = 0;count<length;count++)
			{
				if (dataPtr[count] == '<')
					bracketCount++;
			}
			for (;bracketCount;bracketCount--)
			{
				endPtr = strchr(endPtr + 1,'>');
			}
		}
		else 
			returnValue = true;
			
	if ((tagPtr && dataPtr && endPtr) || doingDrag)
	{ // Copy from end of tag to end of data
		if (doingDrag)
		{
			if (theNotes == nil || *theNotes == nil || tagPtr == nil)
				{
					returnValue = true;
					UL(theNotes);
					*nickNameEmpty = true;
					return (noErr);
				}
			
			length = endPtr - dataPtr;
			if (length > 63)
				length = 63;
			BlockMoveData(dataPtr,sViewData+1,length);
			*sViewData = length;

			UL(theNotes);
			return(noErr);
		}
		else
		{
			length = endPtr - dataPtr;
			MakePStr(tempStr,dataPtr,length);
			
			// if field is not name, add nickname
			if (!EqualStrRes(theField,NickManKeyStrn+1) && !doingAE)
			{
				GetNicknameNamePStr(which,nickNum,anotherTemp);
				ComposeRString(scratch,NICK_VIEW_ANNOTATE,tempStr,anotherTemp);
				PSCopy(tempStr,scratch);
			}
		}
	}
	else if (!doingDrag && !doingAE)
		{
			PCopy(tempStr,"\p�");
//			PCat(tempStr,*((*((*Aliases)[which].theData))[nickNum].theName));
			GetNicknameNamePStr(which,nickNum,anotherTemp);
			PSCat(tempStr,anotherTemp);
			PSCatC(tempStr,'�');
			returnValue = true;
		}
	
	// (jp) 12-19-99  When we're getting field data from an AppleEvent we're going to
	//								replace the ETX characters with CR's, otherwise replace with spaces
	//								as before
	for (i=1;i<=*tempStr;i++) // Strip out illegal characters
		if (tempStr[i]=='\003')
			tempStr[i] = doingAE ? '\015' : ' ';
	
	PCopy(sViewData,tempStr);
	
	if (theNotes) UL(theNotes);
	*nickNameEmpty = returnValue;
	return (noErr);
}

/************************************************************************
 * Makes a new nickname file placing it in the correct spot in the nickname list
 ************************************************************************/

short NickMakeNewFile (Str63 name)

{
	AliasDesc	ad;
	FSSpec		folderSpec;
	OSErr			theError;
	long			dirID;
	short			ab,
						i,
						totalAB;
	Str63 		abName;

	// Does it already exist?
	for (ab = NAliases-1; ab>=0; ab--)
	{
		PCopy(abName,(*Aliases)[ab].spec.name);
		if (StringSame(abName,name)) return ab;
	}
	
	ab = kAddressBookUndefined;
	
	// Create an address book record
	Zero (ad);
	ad.type		 = regularAddressBook;
	ad.hNames	 = NuHandle (0);
	ad.theData = NuHandle (0);
	theError = MemError ();
	if (!theError) {
		HNoPurge (ad.theData);
		theError = MemError ();
	}

	// Create the nickname file
	if (!theError) {
		if (!SubFolderSpec (NICK_FOLDER, &folderSpec))
			SimpleMakeFSSpec (folderSpec.vRefNum, folderSpec.parID, name, &ad.spec);
		else {	// If not, create one and then create our file spec
			SimpleMakeFSSpec (Root.vRef, Root.dirId, GetRString (folderSpec.name, NICK_FOLDER),&folderSpec);
			theError = FSpDirCreate (&folderSpec, smSystemScript, &dirID);
			if (!theError) {
				SubFolderSpec (NICK_FOLDER, &folderSpec);
				SimpleMakeFSSpec (folderSpec.vRefNum, folderSpec.parID, name, &ad.spec);
			}
		}
		if (!theError)
			theError = FSpCreate (&ad.spec, CREATOR, MAILBOX_TYPE, smSystemScript);
		if (theError)
			FileSystemError (SAVE_ALIAS, ad.spec.name, theError);
	}
	
	// Find its position in the nickname file list.
	// This position should be after the Personal, Eudora and History
	// address books, but before the Plugin address books:
	//			Personal Nicknames
	//			Eudora Nicknames
	//			Regular Nickname Files
	//			History List
	//			Plugin Nickname Files
	if (!theError) {
		totalAB = NAliases;
		i = 0;
#ifdef VCARD
		// Skip the personal address book
		if (IsPersonalAddressBook (i))
			++i;
#endif
		// Skip the eudora address book
		if (IsEudoraAddressBook (i))
			++i;

		// Skip to the end of the regular address books
		while (i < totalAB && IsRegularAddressBook (i))
			++i;
	
		// 'i' should now be pointing to where in the list we wish to insert the nick file record
		Munger (Aliases, i * sizeof (AliasDesc), nil, 0, &ad, sizeof (AliasDesc));
		theError = MemError ();
		if (theError)
			WarnUser (ALIAS_NEW_NICK_FILE_ERR, theError);
		else
		{
			ab = i;
			ABTickleHardEnoughToMakeYouPuke();
		}

	}
	
	// Clean up...
	if (theError) {
		ZapHandle (ad.hNames);
		ZapHandle (ad.theData);
	}
	return (ab);
}

	
	
/************************************************************************
 * AERemoveNick - get rid of a nickname via AppleEvents
 ************************************************************************/
Boolean AERemoveNick(short index,short which)
{
	Str63 name;
	short numOfAliasFiles = NAliases;
	OSErr	theErr = noErr;

	if (index < 0) // Can't remove a nickname file
		{
#ifdef VCARD
			if (IsEudoraAddressBook(which) || IsPersonalAddressBook(which))
#else
			if (IsEudoraAddressBook(which))
#endif
				return (false);
		
	
			// De-allocate underlying structures
			ZapAliasFile(which);
	
		
			// Move file to trash
			theErr = MyFSpMoveToTrash(&(*Aliases)[which].spec);
			if (theErr)
				FSpDelete(&(*Aliases)[which].spec);
	
			return (true);
		}

//		PCopy(name,*((*((*Aliases)[which].theData))[index].theName));
		GetNicknameNamePStr(which,index,name);
		if (IsNicknameOnRecipList(which,index)) RemoveFromTo(name);
		RemoveNamedNickname(which,name);
		TBRemoveDefunctNicknameButton (name);
		SetAliasDirty(which);

	RemoveNickFromAddressBookList (which, index, true);
	return (true);

}

/************************************************************************
 * NickWinIsOpen - is the Address Book window open?
 ************************************************************************/
Boolean NickWinIsOpen(void)
{
	return AB.inited && GetWindowKind(GetMyWindowWindowPtr(Win)) == ALIAS_WIN &&
					IsWindowVisible(GetMyWindowWindowPtr(Win));
}

/************************************************************************
 * AliasWinIsOpen - is the alias window open?
 ************************************************************************/
Boolean AliasWinIsOpen(void)
{
	return(AB.inited!=nil);
}


void RemoveNickFromAddressBookList (short which, short index, Boolean doSave)

{
	if (AB.inited)
		RemoveNickname (which, index);
	else
		if (doSave)
			SaveAliases(true);
}


/**********************************************************************
 * AliasWinRefresh - refresh the current nickname
 **********************************************************************/
void AliasWinRefresh(void)
{

}		

//
//	VanquishRecipientList
//
//		This code used to be in Alias close.  I put it here instead and
//		rearranged things a little
//
void VanquishRecipientList (Boolean discardChanges)
{
	if (OldRecipientMenu)
	{
		if (discardChanges)
		{
			//	Need to restore recipient list
			SetupRecipMenusWith(OldRecipientMenu);
			ToMenusChanged();
		}
		//	Dispose of copy of recipient list
		ZapHandle (OldRecipientMenu);
	}
}


//
//	SaveExpandedAddressBookNames
//
//		This code used to be in Alias close.  I put it here instead and
//		rearranged things a little.  We could make this routine a lot
//		simpler by just storing a state boolean in the resource fork of
//		the address book itself -- but that would be trouble for anyone
//		planning to store their address book files on a shared server.
//
//		Also, there used to be logic to default empty address books to
//		closed, even if the user left the address book open.  Seems
//		valid for the user to choose to do this, so I'm axing this code.
//
//		We also used to do a big handle compare to see if the list had
//		changed.  Honestly, though, these days of faster CPUs and fast
//		disk access makes this check unnecessary -- so we'll just copy
//		out the list regardless.
//

void SaveExpandedAddressBookNames (void)

{
	Handle	dataH;
	OSErr		theError;
	short		numABs,
					ab,
					count;

	count = 0;
	numABs = NAliases;
	
	// Make space for the count, which we store in a short
	dataH = NuHandle (sizeof (short));
	theError = MemError ();
	
	for (ab = 0; !theError && ab < numABs; ab++)
		if ((*Aliases)[ab].collapsed) {
			++count;
			theError = PtrPlusHand_ ((*Aliases)[ab].spec.name, dataH, *(*Aliases)[ab].spec.name + 1);
		}
	if (!theError) {
		*(short*)(*dataH) = count;
		ZapSettingsResource ('STR#', NickFileCollapseStrn);
		AddMyResource_ (dataH,'STR#', NickFileCollapseStrn, "");
		theError = ResError ();
	}
	if (!theError)
		MyUpdateResFile (SettingsRefN);
	if (theError)
		ZapHandle (dataH);
}


/************************************************************************
 * NickRepAlert - nickname replacement alert, with vetted cancel return
 ************************************************************************/
short NickRepAlert(PStr candidate)
{
	short err = AlertStr(NICK_REP_ALRT,Note,candidate);
	if (err==userCanceledErr || err==CANCEL_ITEM) err = nrDifferent;
	return(err);
}

/*
 * Takes a string of the form: Field Title:Height,Width and parses out the info.
 */
void NickParseForInfo(Str255 input,long *height,long *width,Str255 fieldName)
{
	Str31 token;
	UPtr spot;
	short	i;

	spot = input+1;
	PToken(input,token,&spot,":");
	StringToNum(PToken(input,token,&spot,","),height); // Get num of lines for field

 	/*
 	 * Get width for field: 1 for full width, 2 for half width (currently not used)
 	 */
 	 
 	StringToNum(PToken(input,token,&spot,","),width);
	for (i=1;i<= *input;i++)	
	{
		if (input[i]==':')
			break;
	}
	PCopy(fieldName,input);
	fieldName[0]=i;
}



/************************************************************************
 * GetCurrentName - get the currently selected nickname
 ************************************************************************/
void GetName (short i,short *nickNum,short *whichFile)
{
	if (i<0)
		{
			*nickNum = -1;
			*whichFile = -1;
			return;
		}
	else
		if (GetSelectedABNick (i, whichFile, nickNum))
			;
}


OSErr MyFSpMoveToTrash(const FSSpec *spec)
{
	short	theVRef;
	long	theDirID;
	FSSpec	trashSpec,tempSpec,uniqueSpec;
	OSErr		theErr = noErr;
	
	// Move file to trash
	FindFolder(spec->vRefNum,kTrashFolderType,false,&theVRef,&theDirID);
	FSMakeFSSpec(theVRef,theDirID,"\p",&trashSpec);
	theErr = FSpCatMove(spec,&trashSpec);

	// If we don't have a problem, just leave.
	if (!theErr)
		return (noErr);
		
	// If the file already exists in the trash, load up our tempSpec
	if (theErr == dupFNErr)
			FSMakeFSSpec(theVRef,theDirID,spec->name,&tempSpec);

	uniqueSpec = tempSpec;
	theErr = UniqueSpec(&uniqueSpec,27);

	if (!theErr)
		theErr = FSpRename(&tempSpec,uniqueSpec.name);
	
	// We've successfully renamed the file in the trash, now we have to really trash our file
	if (theErr == noErr)
	{
		FindFolder(spec->vRefNum,kTrashFolderType,false,&theVRef,&theDirID);
		FSMakeFSSpec(theVRef,theDirID,"\p",&trashSpec);
		theErr = FSpCatMove(spec,&trashSpec);

	}
	
	return (theErr);
}



/**********************************************************************
 * TopCompositionWindow - topmost composition window, not counting current window
 **********************************************************************/
MyWindowPtr TopCompositionWindow(Boolean skipCurrent, Boolean dontCreate)
{
	WindowPtr winWP = FrontWindow_();
	MyWindowPtr	win;
	
	if (skipCurrent) {
		while (winWP && (IsFloating(winWP) || !IsWindowVisible(winWP) || GetWindowKind(winWP)==FIND_WIN)) winWP = GetNextWindow(winWP);
		winWP = GetNextWindow(winWP);
	}
	while (winWP && (IsFloating(winWP) || !IsWindowVisible(winWP) || GetWindowKind(winWP)==FIND_WIN)) winWP = GetNextWindow(winWP);
	win = GetWindowMyWindowPtr(winWP);
	if (!IsKnownWindowMyWindow(winWP) || GetWindowKind(winWP)!=COMP_WIN ||
	  SumOf(Win2MessH(win))->state==SENT || !IsWindowVisible(winWP) ||
	  SumOf(Win2MessH(win))->state==BUSY_SENDING)
	{
		if (dontCreate)
			return (nil);
		win = DoComposeNew(0);
#ifdef TWO
		if (win)
		{
			ApplyDefaultStationery(win,True,True);
			UpdateSum(Win2MessH(win),SumOf(Win2MessH(win))->offset,SumOf(Win2MessH(win))->length);
		}
#endif
	}

	return(win);
}

/************************************************************************
 *
 ************************************************************************/
void AliasesFixFont(void)
{
#ifdef BETA
	BIG UGLY ERROR
#endif
}


/************************************************************************
 * AskNickname - ask the user for a new nickname.
 ************************************************************************/
#ifdef VCARD
NickReplaceEnum AskNickname (Handle addresses, FSSpec *vcardSpec, short ab)
#else
NickReplaceEnum AskNickname (Handle addresses, short ab)
#endif
{
	NickReplaceEnum	nre;
	Str255					nickname,
									fullName;
	
	SAVE_PORT;	

	nre = nrNone;

	// Figure out the default names from the addresses
	*nickname = 0;
	*fullName = 0;
	if (addresses)
	{
		UniqBinAddr(addresses);
		SortBinAddr(addresses);
		NickSuggest (addresses, nickname, fullName, false, 0);
	}
	
	if (!*nickname)
		MakeUniqueNickname (ab, nickname);

	// Truncate the size of the nickname if it is too beefy
	*nickname = MIN (*nickname, sizeof (Str31) - 1);
	
	// Display either the name or group dialog boxes depending on whether or not we have a real name
	if (CountAddresses (addresses, 2) < 2)
#ifdef VCARD
		nre = DoNicknameDialog (addresses, vcardSpec, ab, nickname, fullName);
#else
		nre = DoNicknameDialog (addresses, ab, nickname, fullName);
#endif
	else
		nre = DoGroupNicknameDialog (addresses, ab, nickname, fullName);
	REST_PORT;
	return (nre);
}


#ifdef VCARD
NickReplaceEnum DoNicknameDialog (Handle addresses, FSSpec *vcardSpec, short ab, PStr nickname, PStr fullName)
#else
NickReplaceEnum DoNicknameDialog (Handle addresses, short ab, PStr nickname, PStr fullName)
#endif

{
	DialogPtr										dgPtr;
	MyWindowPtr 								dgPtrWin;
	NickReplaceEnum							nre;
	PETEDocInitInfo							pdi;
	MakeAddressBookEntryHitType	mabeHit;
	Handle											shortAddress,
															expansion,
															notes;
	Str255											workingName,
															firstName,
															lastName;
	OSErr												theError;
	short												*nickFileIndexArray,
															nick,
															item;
	Boolean											needFile,
															makeRecip,
															done;
	extern ModalFilterUPP				DlgFilterUPP;
	
	theError = noErr;
	nre			 = nrAdd;

	ParseFirstLast (fullName, firstName, lastName);

	needFile = true;
	nickFileIndexArray = GetNickFileArray (&needFile);

	SetDialogFont (SmallSysFontID ());
	dgPtrWin = GetNewMyDialog (NEW_NICK_DLOG, nil, nil, InFront);
	dgPtr		 = GetMyWindowDialogPtr (dgPtrWin);
	SetDialogFont (0);
	if (!dgPtrWin || !dgPtr)
		theError = userCanceledErr;

	//fix the controls and setup all the PETE user panes
	if (!theError) {
		SetPort_ (GetWindowPort (GetDialogWindow (dgPtr)));
		ConfigFontSetup (dgPtrWin);
		ReplaceAllControls (dgPtr);
		DefaultPII (dgPtrWin, false, peNoStyledPaste | peClearAllReturns, &pdi);
	
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_NICK_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickHighlight | nickComplete | nickSpaces, 0);
	}
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_NICK_FULL_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickNoScan, 0);
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_NICK_FIRST_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickNoScan, 0);
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_NICK_LAST_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickNoScan, 0);
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_NICK_ADDRESS, peNoStyledPaste | peClearAllReturns, &pdi, true, nickHighlight | nickComplete | nickSpaces, kNickScanAllAliasFiles);

	// Set the initial text for each PETE user pane
	if (!theError) {
		dgPtrWin->pte = GetPeteDItem (dgPtrWin, NEW_NICK_NAME);
		(void) SetKeyboardFocus (GetDialogWindow (dgPtr), GetDItemCtl (dgPtr, NEW_NICK_NAME), kControlEditTextPart);

		SetPeteDItemText (dgPtrWin, NEW_NICK_NAME, nickname);
		SetPeteDItemText (dgPtrWin, NEW_NICK_FULL_NAME, fullName);
		SetPeteDItemText (dgPtrWin, NEW_NICK_FIRST_NAME, firstName);
		SetPeteDItemText (dgPtrWin, NEW_NICK_LAST_NAME, lastName);

		if (addresses) {
			shortAddress = nil;
			SuckPtrAddresses (&shortAddress, LDRef (addresses) + 1, **addresses, false, True, False, nil);
			UL (addresses);
			if (shortAddress && *shortAddress)
				PeteSetString (*shortAddress, GetPeteDItem (dgPtrWin, NEW_NICK_ADDRESS));
			ZapHandle(shortAddress);
		}

		// Fiddle with the rest of the controls
		SetDItemState (dgPtr, NEW_NICK_RECIP, false);

		if (!HasFeature (featureMultipleNicknameFiles)) {
			HideDialogItem (dgPtr, NEW_NICK_IN_ADDRESS);
			needFile = false;
		}
		
		BuildAddressBookMenu (dgPtr, NEW_NICK_FILE, nickFileIndexArray, needFile, &ab);

		SetAliasFileToScan (GetPeteDItem (dgPtrWin, NEW_NICK_NAME), ab);
		SetAliasFileToScan (GetPeteDItem (dgPtrWin, NEW_NICK_ADDRESS), ab);

		HiliteButtonOne (dgPtr);
		
		PeteSelectAll (dgPtrWin, dgPtrWin->pte);
	}	
	
	// Run the dialog
	if (!theError) {
		StartMovableModal (dgPtr);
		ShowWindow (GetDialogWindow (dgPtr));

		done = false;
		while (!done) {
			SetGreyControl (GetDItemCtl (dgPtr, NEW_NICK_ADD_DETAILS), !PeteLen (GetPeteDItem (dgPtrWin, NEW_NICK_NAME)) || !PeteLen (GetPeteDItem (dgPtrWin, NEW_NICK_ADDRESS)));
			SetGreyControl (GetDItemCtl (dgPtr, NEW_NICK_OK), !PeteLen (GetPeteDItem (dgPtrWin, NEW_NICK_NAME)) || !PeteLen (GetPeteDItem (dgPtrWin, NEW_NICK_ADDRESS)));
			CommandPeriod = false;
			MovableModalDialog (dgPtr, DlgFilterUPP, &item);
		
			switch (item) {
				case NEW_NICK_RECIP:
					SetDItemState (dgPtr, NEW_NICK_RECIP, !GetDItemState (dgPtr, NEW_NICK_RECIP));
					break;
				case NEW_NICK_FILE:
					HitAddressBookMenu (dgPtrWin, NEW_NICK_FILE, nickFileIndexArray, needFile);
					break;
				case NEW_NICK_SWAP_NAMES:
					GetPeteDItemText (dgPtrWin, NEW_NICK_FIRST_NAME, firstName);
					GetPeteDItemText (dgPtrWin, NEW_NICK_LAST_NAME, lastName);
					SetPeteDItemText (dgPtrWin, NEW_NICK_FIRST_NAME, lastName);
					SetPeteDItemText (dgPtrWin, NEW_NICK_LAST_NAME, firstName);
					break;
			}
			GetPeteDItemText (dgPtrWin, NEW_NICK_NAME, workingName);

			ab = HasFeature (featureMultipleNicknameFiles) ? nickFileIndexArray[GetDItemState(dgPtr, NEW_NICK_FILE) - 1] : 0;
			if (item == CANCEL_ITEM || item == NEW_NICK_CANCEL || item == NEW_NICK_OK || item == NEW_NICK_ADD_DETAILS)
				done = true;
			if ((item == NEW_NICK_OK || item == NEW_NICK_ADD_DETAILS) && (nre = BadNickname (workingName, nickname, ab, -1, addresses==nil, true, false)) == nrDifferent) {
				PeteSelectAll (dgPtrWin, dgPtrWin->pte);
				done = false;
			}
		}
		
		mabeHit = mabeHitOther;
		if (item == NEW_NICK_OK || item == NEW_NICK_ADD_DETAILS) {
			Handle	typedAddresses;
			
			mabeHit = item == NEW_NICK_OK ? mabeHitOK : mabeHitAddDetails;
			
			// Grab the info from the typed fields
			makeRecip = GetDItemState(dgPtr,NEW_NICK_RECIP);
			GetPeteDItemText (dgPtrWin, NEW_NICK_NAME, nickname);
			GetPeteDItemText (dgPtrWin, NEW_NICK_FULL_NAME, fullName);
			GetPeteDItemText (dgPtrWin, NEW_NICK_FIRST_NAME, firstName);
			GetPeteDItemText (dgPtrWin, NEW_NICK_LAST_NAME, lastName);

			// Grab the typed addresses
			typedAddresses = GetPeteDItemTextH (dgPtrWin, NEW_NICK_ADDRESS);
			if (typedAddresses) {
				Tr (typedAddresses, "\015", ",");
				theError = SuckPtrAddresses (&expansion, LDRef(typedAddresses),GetHandleSize (typedAddresses),true,true,false,nil);
			}
			ZapHandle (typedAddresses);
			
			// Make a notes handle
#ifdef VCARD
			if (!theError)
/*				if (vcardSpec && HasFeature (featureVCard)) {
					ZapHandle (expansion);
					theError = NicknameFromVCardFile (vcardSpec, &expansion, &notes);
				}
				else */
					notes = CreateSimpleNotes (fullName, firstName, lastName);
			if (!theError)
				nick = NewNickLow (expansion, notes, ab, nickname, makeRecip, nre, AB.inited ? false : true);
#else
			if (!theError) {
				notes = CreateSimpleNotes (fullName, firstName, lastName);
				nick = NewNickLow (expansion, notes, ab, nickname, makeRecip, nre, AB.inited ? false : true);
			}
#endif
		}
	
		DisposePeteUserPaneItem (dgPtrWin, NEW_NICK_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_NICK_FULL_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_NICK_FIRST_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_NICK_LAST_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_NICK_ADDRESS);

		EndMovableModal(dgPtr);
		DisposDialog_(dgPtr);
		if (!theError)
		{
			if (AB.inited)
				UpdateMyWindow(GetMyWindowWindowPtr(Win));	//	redisplay AB in case dialog was on top
			PostMakeNicknameDialog (ab, nick, mabeHit);
		}
	}

	ZapPtr (nickFileIndexArray);
	if (theError)
		WarnUser (ALIAS_NEW_NICK_ERR, theError);
	return (item == NEW_NICK_OK || item == NEW_NICK_ADD_DETAILS ? nre : nrCancel);
}


//
//	CreateSimpleNotes
//
//		Create a simple notes handle containing only a full, first and last name.
//

Handle CreateSimpleNotes (PStr fullName, PStr firstName, PStr lastName)

{
	Str255	tag;
	Handle	notes;
	OSErr		theError;
	
	notes = NuHandle (0);
	theError = MemError ();
	if (!theError && *fullName)
		theError = AddAttributeValuePair (notes, GetRString (tag, ABReservedTagsStrn + abTagName), &fullName[1], *fullName);
	if (!theError && *firstName)
		theError = AddAttributeValuePair (notes, GetRString (tag, ABReservedTagsStrn + abTagFirst), &firstName[1], *firstName);
	if (!theError && *lastName)
		theError = AddAttributeValuePair (notes, GetRString (tag, ABReservedTagsStrn + abTagLast), &lastName[1], *lastName);
	if (theError)
		ZapHandle (notes);
	return (notes);
}

NickReplaceEnum DoGroupNicknameDialog (Handle addresses, short ab, PStr nickname, PStr fullName)

{
	DialogPtr										dgPtr;
	MyWindowPtr 								dgPtrWin;
	NickReplaceEnum							nre;
	PETEDocInitInfo							pdi;
	MakeAddressBookEntryHitType	mabeHit;
	Handle											expansion,
															notes;
	Str255											workingName,
															tag;
	OSErr												theError;
	short												*nickFileIndexArray,
															nick,
															item;
	Boolean											needFile,
															makeRecip,
															done;
	extern ModalFilterUPP				DlgFilterUPP;
	
	theError = noErr;
	nre			 = nrAdd;

	needFile = true;
	nickFileIndexArray = GetNickFileArray (&needFile);

	SetDialogFont (SmallSysFontID ());
	dgPtrWin = GetNewMyDialog (NEW_GROUP_DLOG, nil, nil, InFront);
	dgPtr		 = GetMyWindowDialogPtr (dgPtrWin);
	SetDialogFont (0);
	if (!dgPtrWin || !dgPtr)
		theError = userCanceledErr;

	//fix the controls and setup all the PETE user panes
	if (!theError) {
		SetPort_ (GetWindowPort (GetDialogWindow (dgPtr)));
		ConfigFontSetup (dgPtrWin);
		ReplaceAllControls (dgPtr);
		DefaultPII (dgPtrWin, false, peNoStyledPaste | peClearAllReturns, &pdi);

		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_GROUP_NICK_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickHighlight | nickComplete | nickSpaces, 0);
	}
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_GROUP_GROUP_NAME, peNoStyledPaste | peClearAllReturns, &pdi, true, nickNoScan, 0);
	if (!theError)
		theError = CreatePeteUserPaneWithQDTextAndScanning (dgPtr, NEW_GROUP_ADDRESSES, peNoStyledPaste | peVScroll | peUseHLineWidth, &pdi, false,nickHighlight | nickComplete | nickSpaces, kNickScanAllAliasFiles);

	// Set the initial text for each PETE user pane
	if (!theError) {
		dgPtrWin->pte = GetPeteDItem (dgPtrWin, NEW_GROUP_NICK_NAME);
		(void) SetKeyboardFocus (GetDialogWindow (dgPtr), GetDItemCtl (dgPtr, NEW_GROUP_NICK_NAME), kControlEditTextPart);

		SetPeteDItemText (dgPtrWin, NEW_GROUP_NICK_NAME, nickname);
		SetPeteDItemText (dgPtrWin, NEW_GROUP_GROUP_NAME, fullName);

		if (addresses) {
			FlattenListWith (addresses, '\015');
			PeteSetTextPtr (GetPeteDItem (dgPtrWin, NEW_GROUP_ADDRESSES), LDRef (addresses), GetHandleSize (addresses));
			UL (addresses);
		}

		// Fiddle with the rest of the controls
		SetDItemState (dgPtr, NEW_GROUP_RECIP, false);

		if (!HasFeature (featureMultipleNicknameFiles)) {
			HideDialogItem (dgPtr, NEW_GROUP_IN_ADDRESSES);
			needFile = false;
		}
		
		BuildAddressBookMenu (dgPtr, NEW_GROUP_FILE, nickFileIndexArray, needFile, &ab);

		SetAliasFileToScan (GetPeteDItem (dgPtrWin, NEW_GROUP_NICK_NAME), ab);
		SetAliasFileToScan (GetPeteDItem (dgPtrWin, NEW_GROUP_ADDRESSES), ab);

		HiliteButtonOne (dgPtr);

		PeteSelectAll (dgPtrWin, dgPtrWin->pte);
	}
	
	// Run the dialog
	if (!theError) {
		StartMovableModal (dgPtr);
		ShowWindow (GetDialogWindow (dgPtr));

		done = false;
		while (!done) {
			SetGreyControl (GetDItemCtl (dgPtr, NEW_GROUP_ADD_DETAILS), !PeteLen (GetPeteDItem (dgPtrWin, NEW_GROUP_NICK_NAME)) || !PeteLen (GetPeteDItem (dgPtrWin, NEW_GROUP_ADDRESSES)));
			SetGreyControl (GetDItemCtl (dgPtr, NEW_GROUP_OK), !PeteLen (GetPeteDItem (dgPtrWin, NEW_GROUP_NICK_NAME)) || !PeteLen (GetPeteDItem (dgPtrWin, NEW_GROUP_ADDRESSES)));
			CommandPeriod = false;
			MovableModalDialog (dgPtr, DlgFilterUPP, &item);
		
			switch (item) {
				case NEW_GROUP_RECIP:
					SetDItemState (dgPtr, NEW_GROUP_RECIP, !GetDItemState (dgPtr, NEW_GROUP_RECIP));
					break;
				case NEW_GROUP_FILE:
					HitAddressBookMenu (dgPtrWin, NEW_GROUP_FILE, nickFileIndexArray, needFile);
					break;
			}
			GetPeteDItemText (dgPtrWin, NEW_GROUP_NICK_NAME, workingName);

			ab = HasFeature (featureMultipleNicknameFiles) ? nickFileIndexArray[GetDItemState(dgPtr, NEW_GROUP_FILE) - 1] : 0;
			if (item == CANCEL_ITEM || item == NEW_GROUP_CANCEL || item == NEW_GROUP_OK || item == NEW_GROUP_ADD_DETAILS)
				done = true;
			if ((item == NEW_GROUP_OK || item == NEW_GROUP_ADD_DETAILS) && (nre = BadNickname (workingName, fullName, ab, -1, addresses==nil,true,false)) == nrDifferent) {
				PeteSelectAll (dgPtrWin, dgPtrWin->pte);
				done = false;
			}
		}
		mabeHit = mabeHitOther;
		if (item==NEW_GROUP_OK || item == NEW_GROUP_ADD_DETAILS) {
			Handle	typedAddresses;
			
			mabeHit = item == NEW_GROUP_OK ? mabeHitOK : mabeHitAddDetails;

			makeRecip = GetDItemState(dgPtr,NEW_GROUP_RECIP);
			GetPeteDItemText (dgPtrWin, NEW_GROUP_NICK_NAME, nickname);
			GetPeteDItemText (dgPtrWin, NEW_GROUP_GROUP_NAME, fullName);

			// Grab the typed addresses
			typedAddresses = GetPeteDItemTextH (dgPtrWin, NEW_GROUP_ADDRESSES);
			if (typedAddresses) {
				Tr (typedAddresses, "\015", ",");
				theError = SuckPtrAddresses (&expansion, LDRef(typedAddresses),GetHandleSize (typedAddresses),true,true,false,nil);
			}
			ZapHandle (addresses);

			// Make a notes handle
			if (!theError) {
				notes = NuHandle (0);
				theError = MemError ();
			}
			if (!theError && *fullName)
					theError = AddAttributeValuePair (notes, GetRString (tag, ABReservedTagsStrn + abTagName), &fullName[1], *fullName);
			if (!theError)
				nick = NewNickLow (expansion, notes, ab, nickname, makeRecip, nre, AB.inited ? false : true);
		}
		DisposePeteUserPaneItem (dgPtrWin, NEW_GROUP_NICK_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_GROUP_GROUP_NAME);
		DisposePeteUserPaneItem (dgPtrWin, NEW_GROUP_ADDRESSES);

		EndMovableModal(dgPtr);
		DisposDialog_(dgPtr);
		if (!theError)
		{
			if (AB.inited)
				UpdateMyWindow(GetMyWindowWindowPtr(Win));	//	redisplay AB in case dialog was on top
			PostMakeNicknameDialog (ab, nick, mabeHit);
		}
	}

	ZapPtr (nickFileIndexArray);
	if (theError)
		WarnUser (ALIAS_NEW_NICK_ERR, theError);
	return (item==NEW_GROUP_OK || item == NEW_GROUP_ADD_DETAILS ? nre : nrCancel);
}


//
//	CreatePeteUserPaneWithQDTextAndScanning
//
//		Abusively long, but it saves us some typing and code size in the long run.
//

OSErr CreatePeteUserPaneWithQDTextAndScanning (DialogPtr theDialog, short item, uLong flags, PETEDocInitInfoPtr initInfo, Boolean noWrap, NickScanType nickScan, short aliasIndex)

{
	PETEHandle	pte;
	
	if (pte = CreatePeteUserPane (GetDItemCtl (theDialog, item), flags, initInfo, noWrap)) {
		PeteFontAndSize (pte,GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort()));
		if (nickScan != nickNoScan)
			SetNickScanning (pte, nickScan, aliasIndex, nil, nil, nil);
	}
	return (pte ? noErr : userCanceledErr);
}


void PostMakeNicknameDialog (short ab, short nick, MakeAddressBookEntryHitType mabeHit)

{
	VLNodeInfo	data;
	VLNodeID		nodeID;
	
	nodeID = MakeNodeID (ab, nick);
	switch (mabeHit) {
		case mabeHitOK:
			if (AB.inited) {
				ABTickle ();
//				InvalidListView (&NickList);
//				ABNickLVSelect (nodeID, true);
//				LVDraw (&NickList,nil,false, false);
			}
			break;
		case mabeHitAddDetails:
			if (AB.inited) InvalidListView (&NickList);
			OpenABWin (nil);
			// Massage the item name so it can be found in the list view
			if (AB.inited) {
				if ((*Aliases)[ab].collapsed) {
					LVGetItemWithNodeID (&NickList, MakeABNodeID (ab), &data);
					LVExpand (&NickList, MakeABNodeID (ab), data.name, true);
				}
				ABNickLVSelect (nodeID, false);
				LVDraw (&NickList,nil,false,false);
			}
			break;
	}
}


/**********************************************************************
 * NickSuggest - suggest a name for an address and extract the real name
 **********************************************************************/
void NickSuggest(BinAddrHandle addresses, PStr name,PStr realName,Boolean uniq,short fmt)
{
	Str255 scratch;
	Str255 firstAddress;
	Str63 first, last;
	short i;
	
	*name = 0;
	*realName = 0;
	
	/*
	 * emptiness
	 */
	if (!addresses || !*addresses || !**addresses) return;
	
	/*
	 * more than one address?
	 */
	if ((*addresses)[**addresses+2]) return;
		
	/*
	 * only one address!
	 */
	PSCopy(scratch,*addresses);
	
	/*
	 * extract name portion
	 */
	BeautifyFrom(scratch);
	if (!*scratch || *scratch > 63) return;
	
	PCopy(realName,scratch);
	PCopy(firstAddress,*addresses);
	
	PCopy(name,realName);
	
	// if name is same as address, it's not really
	// a name, so don't bother trying to process it
	// as though it were
	if (StringSame(realName,firstAddress))
		*realName = 0;
	else
	{
		ParseFirstLast(name,first,last);
		if (GetPrefLong(PREF_NICK_GEN_OPTIONS)&kNickGenOptLastFirst)
		{
			PCopy(name,last);
			if (*name) PCatC(name,' ');
			PCat(name,first);
		}
		else
			JoinFirstLast(name,first,last);
	}
	
	// compose using the format, if any
	if (fmt)
	{
		ComposeRString(scratch,fmt,name);
		PCopy(name,scratch);
	}
	
	/*
	 * Sanitize
	 */
	SanitizeFN(name,name,NICK_BAD_CHAR,NICK_REP_CHAR,false);
	*name = MIN(*name,30);
	
	// make it unique
	if (uniq)
	{
		PSCopy(last,name);
		for (i=1;IsAnyNickname(last);i++)
		{
			NumToString(i,first);
			PCopy(scratch,name);
			*scratch = MIN(30-*first,*scratch);
			PCat(scratch,first);
			PCopy(last,scratch);
		}
		PCopy(name,last);
	}
}

short *GetNickFileArray (Boolean *needFile)

{
	short	*nickFileIndexArray,
				item;;
	
	nickFileIndexArray = nil;
	
	//	we only need the file menu if another writeable file exists
	//Enhanced Address Book	- prevent users from creating a new nickname FILE.
	if (HasFeature (featureMultipleNicknameFiles)) {
		if (*needFile) {
			*needFile = False;
			for (item = 1; item < NAliases; item++)
				if (!(*Aliases)[item].ro) {
					*needFile = True;
					break;
				}
		}
		// We're going to build a small array that matches each element of the file popup.  In
		// each array element we'll store the "real" 'which' value for that menu item to be used
		// when the user has specified a particular nickname file.  We're doing this to prevent
		// an offset problem when the user has plug-in nickname files.
		if (*needFile)
			nickFileIndexArray = NuPtr (sizeof (short) * NAliases);
	}
	else
		*needFile = false;
	return (nickFileIndexArray);
}


void BuildAddressBookMenu (DialogPtr theDialog, short addressBookMenuItem, short	*nickFileIndexArray, Boolean needFile, short *which)

{
	ControlHandle	theControl;
	MenuHandle		mh;
	short					*nickFileIndexArrayPtr,
								ab,
								totalAliases;

	if (theControl = GetDItemCtl (theDialog, addressBookMenuItem))
		if (mh = GetControlPopupMenuHandle (theControl))
			if (!needFile)
				HideDialogItem (theDialog, addressBookMenuItem);
			else {
				nickFileIndexArrayPtr = nickFileIndexArray;
				
				// The address book menu will consist of only the following:
				//		Eudora Nicknames
				//		Regular Address Books
				//		History List
				totalAliases = NAliases;
				for (ab = 0; ab < totalAliases; ++ab)
					if (IsEudoraAddressBook (ab) || IsRegularAddressBook (ab) || (IsHistoryAddressBook (ab) && !PrefIsSet (PREF_NICK_CACHE_NOT_VISIBLE))) {
						MyAppendMenu (mh, (*Aliases)[ab].spec.name);
						if ((*Aliases)[ab].ro)
							DisableItem (mh, CountMenuItems (mh));
						*nickFileIndexArrayPtr++ = ab;
					}
				
				SetControlMaximum (theControl, CountMenuItems (mh));
				SetControlMinimum (theControl, 1);
				SetControlValue   (theControl, *which + 1);
			}
}

void HitFileNickRadioButton (DialogPtr theDialog, short itemHit, short addressBookItem, short nicknameItem, short addressBookMenuItem, short recipientItem)

{
	short	newFileCheck;
	
	newFileCheck = GetDItemState (theDialog, addressBookItem);
	if ((newFileCheck && itemHit  == nicknameItem) || (!newFileCheck && itemHit == addressBookItem)) { // Currently checked, need to undim items
		if (newFileCheck) {
			SetGreyControl (GetDItemCtl (theDialog, addressBookMenuItem), false);
			SetGreyControl (GetDItemCtl (theDialog, recipientItem), false);
		}
		else // Currently unchecked, need to dim items.
			{
				SetGreyControl (GetDItemCtl (theDialog, addressBookMenuItem), true);
				SetGreyControl (GetDItemCtl (theDialog, recipientItem), true);
			}
		SetDItemState (theDialog, addressBookItem, !newFileCheck);
		SetDItemState (theDialog, nicknameItem, newFileCheck);
	}
}

void HitAddressBookMenu (MyWindowPtr dgPtrWin, short addressBookMenuItem, short	*nickFileIndexArray, Boolean needFile)

{
	long	selStart,
				selEnd;	

	if (needFile) {
		if (CurSelectionIsTypeAheadText (dgPtrWin->pte)) {
			ResetNicknameTypeAhead (dgPtrWin->pte);
			ResetNicknameHiliting (dgPtrWin->pte);
			if (!PeteGetTextAndSelection (dgPtrWin->pte, nil, &selStart, &selEnd))
				if (selStart != selEnd) {
					PeteDelete (dgPtrWin->pte, selStart, selEnd);
					NicknameHilitingUnHiliteField (dgPtrWin->pte);
				}
		}
		SetAliasFileToScan (dgPtrWin->pte, nickFileIndexArray[GetDItemState(GetMyWindowDialogPtr (dgPtrWin), addressBookMenuItem) - 1]);
	}
}

/************************************************************************
 * BadNickname - is the nickname defective?
 ************************************************************************/
NickReplaceEnum BadNickname(UPtr candidate,UPtr veteran,short which,short nick,Boolean justRename,Boolean collisionCheck,Boolean folder)
{
	Str31 verboten;
	UPtr spot,end,key;
	Str31 noWhiteCand, noWhiteVet;
	short ab,
				foundNick,
				i;

	if (!*candidate) return(nrDifferent);

	// If the strings hasn't changed on a rename, don't test it
	if (justRename && StringSame (candidate, veteran))
		return (nrOk);
		
	// folder?
	if (folder)
	{
		if (*candidate>31)
		{
			WarnUser(NICKFILE_TOO_LONG,0);
			return(nrDifferent);
		}
		if (PIndex(candidate,':'))
		{
			WarnUser(NICKFILE_BADCHAR,0);
			return(nrDifferent);
		}
		for (i=NAliases-1;i>=0;i--)
		{
			PSCopy(verboten,(*Aliases)[i].spec.name);
			if (StringSame(verboten,candidate))
			{
				WarnUser(NICKFILE_DUPLICATE,0);
				return(nrDifferent);
			}
		}
		if (EqualStrRes(candidate,ALIAS_FILE) || EqualStrRes(candidate,USA_EUDORA_NICKNAMES) || EqualStrRes(candidate,FILE_ALIAS_NICKNAMES))
		{
			WarnUser(NICKFILE_DUPLICATE,0);
			return(nrDifferent);
		}
		return(nrOk);
	}

	// nickname
	if (*candidate>MAX_NICKNAME)
	{
		ComposeStdAlert (Note, ALIAS_TOO_LONG, MAX_NICKNAME);
		return(nrDifferent);
	}
	GetRString(verboten,ALIAS_VERBOTEN);
	end = verboten+*verboten+1;
	for (key=candidate+1;key<=candidate+*candidate;key++)
		for (spot=verboten+1;spot<end;spot++)
			if (*spot==*key)
			{
				WarnUser(WARN_VERBOTEN,0);
				return(nrDifferent);
			}
	if (*veteran && StringSame(candidate,veteran) && !collisionCheck)
		return(!EqualString(candidate,veteran,True,True));
	if (*veteran && justRename && !collisionCheck)
	{
		PSCopy(noWhiteCand,candidate);
		PSCopy(noWhiteVet,veteran);
		*noWhiteCand = RemoveChar(' ',noWhiteCand+1,*noWhiteCand);
		*noWhiteVet = RemoveChar(' ',noWhiteVet+1,*noWhiteVet);
		if (StringSame(noWhiteCand,noWhiteVet)) return(!EqualString(candidate,veteran,True,True));
	}
	// Check to see if the nickname already exists in this nickname file
	foundNick = NickMatchFound ((*Aliases)[which].theData, NickHash (candidate), candidate, which);
	// If it does exist -- and, in fact, it is the same nick as that we are looking for during a rename, just return
	if (ValidNickname (nick) && foundNick == nick && justRename)
		return (nrOk);
		
	if (ValidNickname (foundNick)) {
		if (justRename) {
			ComposeStdAlert (Stop, NICK_IN_USE, candidate, (*Aliases)[which].spec.name);
			return (nrDifferent);
		}
		else	return (NickRepAlert(candidate));
	}
	
	// How about any other nickname file?
	for (ab = 0; ab < NAliases; ++ab)
		if (ab != which) {
			foundNick = NickMatchFound ((*Aliases)[ab].theData, NickHash (candidate), candidate, ab);
			if (ValidNickname (foundNick)) {
				if (justRename) {
					switch (ComposeStdAlert (Stop, DUP_NICKNAME_WARNING, candidate)) {
						case kAlertStdAlertOKButton:
							return (nrDifferent);
							break;
						case kAlertStdAlertOtherButton:
							return (nrOk);
							break;
					}
				}
			}
		}
	return(nrOk);
}


/************************************************************************
 * InsertTheAlias - insert the given alias in the window below us
 ************************************************************************/
MyWindowPtr InsertTheAlias (short txeIndex, Boolean wantExpansion)

{
	MessHandle	messH;
	GrafPtr			oldPort;
	MyWindowPtr win;
	HeadSpec		hs;
	Str31				nameStr;
	long 				start;
	short				ab,
							nick,
							index;
	
	if (HasFeature (featureNicknameWatching) && PrefIsSet (PREF_NICK_AUTO_EXPAND))
		wantExpansion = true;

	win = TopCompositionWindow(true, false);
	if (win && CompHeadFind (messH = Win2MessH(win), txeIndex, &hs)) {
		GetPort (&oldPort);
		SetPort_ (GetMyWindowCGrafPtr (win));
		PeteSelect (win, win->pte, hs.stop, hs.stop);
		PetePrepareUndo (win->pte, peUndoPaste, -1, -1, &start, nil);
		
		index = 1;
		while (GetSelectedABNick (index++, &ab, &nick))
			if (nick >= 0) {
				InsertCommaIfNeedBe (TheBody, &hs);
				GetNicknameNamePStr (ab, nick, nameStr);
				InsertAlias (win->pte, &hs, nameStr, wantExpansion, start, false);
			}
		ShowMyWindow (GetMyWindowWindowPtr (win));
		UpdateSum (messH, SumOf(messH)->offset, SumOf(messH)->length);
		SetPort_ (oldPort);
	}
	return win;
}

/**********************************************************************
 * NickWDragVDivider - 	Handle dragging of the divider between the list
 *											and the data
 **********************************************************************/
void NickWDragVDivider(Point pt)
{
	Rect div, bounds;
	Point newPt;
	long	tempValue;
	
	div = VertDivider;
	
	bounds = Win->contR;
	bounds.right = RectWi(Win->contR) - MinTabWidth;
	bounds.left = MinListWidth;
	
	if (bounds.left > bounds.right)
		{
			bounds.left = div.left;
			bounds.right = div.right;
		}
		
	if (div.right > bounds.right)
		bounds.right = div.right;
	
	newPt = DragDivider(pt,&div,&bounds,Win);
	
	// resize?
	if (newPt.h)
	{
		tempValue = (100 * (newPt.h-2*INSET-INSET/2))/(Win->contR.right - Win->contR.left);
		SetPrefLong(ALIAS_NICK_LIST_PRCT,MIN(tempValue,750));
		MyWindowDidResize(Win,nil);
	}
}


// Are any nicknames dirty?
Boolean AnyNicknamesDirty (short ab)

{
	short whichFile;
	short	totalAliases = NAliases;
	short	totalNicks;
	short	firstFile = 0;
	short i;

	if (ab != kAddressBookUndefined) {
		firstFile = ab;
		totalAliases = ab + 1;
	}
	
	for (whichFile=firstFile;whichFile<totalAliases;whichFile++) // Loop through all the files
	{
		if ((*Aliases)[whichFile].dirty)
			return (true);
		totalNicks = (*Aliases)[whichFile].theData ? (GetHandleSize_((*Aliases)[whichFile].theData)/sizeof(NickStruct)) : 0;
		for (i=0;i<totalNicks;i++)
			{
				if ((*((*Aliases)[whichFile].theData))[i].addressesDirty || (*((*Aliases)[whichFile].theData))[i].notesDirty || (*((*Aliases)[whichFile].theData))[i].pornography)
					return (true);
			}
	}
	return (false);

}

/************************************************************************
 * ReformatClip - change returns to comma-space on the clipboard
 ************************************************************************/
void ReformatClip(void)
{
	Handle text = NuHandle(0);
	long size,junk;
	UPtr spot,copy, end;
	short newlines = 0;
 	
	if (text)
	{
		if (size = GetScrap(text,'TEXT',&junk))
		{
			end = *text+size;
			for (spot=*text;spot<end;spot++) if (*spot=='\015') newlines++;
			if (newlines)
			{
				SetHandleBig_(text,newlines+size);
				if (!MemError())
				{
					copy = *text+size+newlines;
					for (spot=*text+size-1;spot>=*text;spot--)
					{
						if (*spot=='\015')
						{
							*--copy = ' ';
							*--copy = ',';
						}
						else *--copy = *spot;
					}
					ZeroScrap();
					PutScrap(size+newlines,'TEXT',LDRef(text));
				}
			}
		}
		ZapHandle(text);
	}
}


/**********************************************************************
 * PrintAddressBookWindow - print nicknames from the nicknames window
 * Brand-new strategy here: Let Pete do it!
 **********************************************************************/
OSErr PrintAddressBookWindow (Boolean select, Boolean now)
{
	OSErr err = noErr;
	MyWindowPtr win;
	WindowPtr	winWP;
	short which;
	
	// make the title for the window, this will go in the print header
	ABPrintMakeTitle(GlobalTemp,select);
	
	// Make a text window to dump the stuff to
	if (!(win=OpenText(nil,nil,nil,nil,false,GlobalTemp,true,false)))
		WarnUser(PRINT_FAILED,0);
	else
	{
		winWP = GetMyWindowWindowPtr (win);
		(*PeteExtra(win->pte))->spelled = sprNeverSpell;
		PeteCalcOff(win->pte);
		PETEAllowUndo(PETE,win->pte,false,false);
    SetNickScanning (win->pte, nickHighlight, kNickScanAllAliasFiles, nil, nil, nil);
		
		for (which=0;which<NAliases;which++)
			if (!IsPluginAddressBook (which)) PrintNickFile(which,select,win->pte);
		
		// Print the text window
		win->dirty = TheWorldAccordingToMarthaStewart;
#ifdef DEBUG
		if (RunType!=Production && !(CurrentModifiers()&optionKey)) 
		{
			PeteCalcOn(win->pte);
			ShowMyWindow(winWP);
			return noErr;
		}
		else
#endif
		err = PrintOneMessage(win,false,now);

		// Close the text window
		NoSaves = true;
		win->isDirty = false;
		PeteCleanList (win->pte);
		CloseMyWindow(winWP);
		NoSaves = false;
		
		if (err) WarnUser(PART_PRINT_FAIL,err);
	}
	
	return err;
}

/**********************************************************************
 * ABPrintMakeTitle - make up a title for printing
 **********************************************************************/
void ABPrintMakeTitle(PStr title, Boolean select)
{
	WindowPtr winWP = GetMyWindowWindowPtr(Win);
	GetWTitle(winWP,title);
}
 
/**********************************************************************
 * PrintNickFile - print nicknames from one file
 **********************************************************************/
OSErr PrintNickFile (short which, Boolean select, PETEHandle pte)

{
	Str255	title;
	short		total,
					ab,
					nick,
					count,
					firstIndex,		// Index of the first nickname in the chosen address book
					i,
					myCount = 0;
	NickPrintStuff myStuff;
	short **sort;
	
	// prepare para info
	Res2PInfo(NICK_PRINT_MARG1,pte,nil,&myStuff.pi1);
	Res2PInfo(NICK_PRINT_MARG2,pte,nil,&myStuff.pi2);
	Res2PInfo(NICK_PRINT_MARG3,pte,nil,&myStuff.pi3);
	myStuff.pte = pte;
	
	total = 0;
	if (select)
	{
		count	= LVCountSelection (&NickList);
		for (i = 1; i <= count; ++i) {
			GetSelectedABNick (i, &ab, &nick);
			if (ab == which) {
				if (nick != kNickUndefined) {
					if (++myCount == 1) firstIndex = i;
				}
				else {
					select = false;
					break;
				}
			}
		}
	}
	
	if (myCount == 0 && select)
		return (noErr);
		
	BuildNickPrintTitle (which, select, title);
	if (PeteLen(pte)) PInsertC(title,sizeof(title),'\f',title+1);
	PeteAppendText(title+1,*title,pte);
	PeteAppendText(Cr+1,1,pte);
	PeteInsertRule(pte,kPETELastPara,0,0,false,true,false);

	count = (*Aliases)[which].theData ? (GetHandleSize_ ((*Aliases)[which].theData) / sizeof (NickStruct)) : 0;
	total = 0;
	for (nick = 0; nick < count; nick++)
		if (!(*((*Aliases)[which].theData))[nick].deleted)
			++total;

	// The list will always have the file title as the first element and therefore the
	// count will be the total nickname count + 1;
	// However the first element (the title) is not printed.
	
	if (!total) // If nickname file is blank, DON'T print it!
		return (noErr);

	if (select) {
		for (i = firstIndex; !CommandPeriod && myCount; ++i) {
			GetSelectedABNick (i, &ab, &nick);
			PrintNickname ( &myStuff, ab, nick );
			myCount--;
		}
	}
	else {
		// Make sure the address book is sorted
		SortAddressBook(which);
		sort = (*Aliases)[which].sortData;
		if (!sort) return memFullErr;
		count = (*Aliases)[which].theData ? (GetHandleSize_ ((*Aliases)[which].theData) / sizeof (NickStruct)) : 0;
		for (nick = 0; !CommandPeriod && nick < count; ++nick)
			if (!(*((*Aliases)[which].theData))[(*sort)[nick]].deleted)
				PrintNickname ( &myStuff, which, (*sort)[nick] );
	}
	
	return (noErr);			// yes, we should probably do more with error checking, but the old
}											// version of this routine could not generate anything but 'noErr'



//
//	PrintNickname
//

void PrintNickname (NickPrintStuffPtr myStuffP, short ab, short nick)

{
	Handle				tempHandle;

	GetNicknameNamePStr (ab, nick, myStuffP->name);

	myStuffP->addresses = nil;
	if (tempHandle = GetNicknameData (ab, nick, true, true))
		SuckAddresses (&myStuffP->addresses, tempHandle, true, true, false, nil);
	myStuffP->ab = ab;
	myStuffP->nick = nick;
	
	PrintANick (myStuffP);

	ZapHandle (myStuffP->addresses);
}

/**********************************************************************
 * PrintANick - print a single nickname
 **********************************************************************/
OSErr PrintANick(NickPrintStuffPtr myStuffP)
{
	OSErr	err = noErr;

	/*
	 * print the nickname
	 */
	if (err=NickPrintName(myStuffP->pte, &myStuffP->pi1, myStuffP->name)) return err;
	
	/*
	 * print things in tab order
	 */
	IterateTabPanes (Tabs, ABPrintPaneProc, myStuffP);
	
	// and a blank line, just for fun
	if (err = PETEInsertParaPtr(PETE,myStuffP->pte,kPETELastPara,nil,Cr+1,*Cr,nil)) return err;
	err = PetePlainParaAt(myStuffP->pte,PeteLen(myStuffP->pte)-1,PeteLen(myStuffP->pte));

	return err;
}

/**********************************************************************
 * ABPrintPaneProc - interator over tabs.  Print the tab name, then iterate
 *  over the fields
 **********************************************************************/
Boolean ABPrintPaneProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, NickPrintStuffPtr myStuffP)
{
	myStuffP->foundOne = false;
	myStuffP->tabPane = tabPane;
	IterateTabPaneObjectsUL(tabPane,ABPrintObjectProc,myStuffP);
	return false;
}

/**********************************************************************
 * ABPrintObjectProc - iterate over objects in the tab
 **********************************************************************/
Boolean ABPrintObjectProc (TabObjectPtr objectPtr, NickPrintStuffPtr myStuffP)
{
	Str255 value;
	Str255 s;
	
	if (objectPtr->type == fieldObject)
	{
		if (EqualStrRes(objectPtr->tag,ABReservedTagsStrn+abTagEmail))
		{
			if (myStuffP->addresses && **myStuffP->addresses)
			{
				// print tab name if we haven't already
				if (!myStuffP->foundOne)
				{
					GetControlTitle(myStuffP->tabPane,s);
					NickPrintName(myStuffP->pte, &myStuffP->pi2, s);
					myStuffP->foundOne = true;
				}

				NickPrintAddresses(myStuffP->pte,&myStuffP->pi3,objectPtr->shortName,myStuffP->addresses);
			}
		}
		else
		{
			// does it have a value?
			GetTaggedFieldValueStr (myStuffP->ab, myStuffP->nick, objectPtr->tag, value);

			if (*value)
			{
				// print tab name if we haven't already
				if (!myStuffP->foundOne)
				{
					Str255	paneTitle;
					
					GetControlTitle(myStuffP->tabPane,paneTitle);
					NickPrintName(myStuffP->pte, &myStuffP->pi2, paneTitle);
					myStuffP->foundOne = true;
				}
							
				// Now print name & value
				NickPrintNameAndValue(myStuffP->pte, &myStuffP->pi3, objectPtr->shortName, value);
			}
		}
	}
	return false;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr NickPrintName(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name)
{
	Str255 s;
	OSErr err;
	
	PCopy(s,name);
	PCat(s,Cr);
	
	err = PETEInsertParaPtr(PETE,pte,kPETELastPara,pinfop,s+1,*s,nil);
	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr NickPrintAddresses(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name, UHandle addresses)
{
	OSErr err;
	short oldOffset = PeteLen(pte);
	
	err = PETEInsertParaPtr(PETE,pte,kPETELastPara,pinfop,name+1,*name,nil);
	if (!err) err = PeteInsertChar(pte,kPETELastPara,'\t',nil);
	FlattenListWith(addresses,'\015');
	if (!err) err = PETEInsertTextHandle(PETE,pte,kPETELastPara,addresses,GetHandleSize(addresses),0,nil);
	if (!err) err = PeteInsertChar(pte,kPETELastPara,'\r',nil);
	//NicknameHilitingUpdateRange (pte, oldOffset, PeteLen(pte));

	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr NickPrintNameAndValue(PETEHandle pte, PETEParaInfoPtr pinfop, PStr name, PStr value)
{
	OSErr err;
	
	err = PETEInsertParaPtr(PETE,pte,kPETELastPara,pinfop,name+1,*name,nil);
	if (!err) err = PeteInsertChar(pte,kPETELastPara,'\t',nil);
	if (!err) err = PETEInsertTextPtr(PETE,pte,kPETELastPara,value+1,*value,nil);
	if (!err) err = PeteInsertChar(pte,kPETELastPara,'\r',nil);

	return err;
}

/**********************************************************************
 * BuildNickPrintTitle - print title for a nickname
 **********************************************************************/
void BuildNickPrintTitle(short which,Boolean select,PStr title)
{
	Str63 fn;
	Str63 selectStr;
	
	if (select) GetRString(selectStr,SELECTED);
	else *selectStr = 0;
	
	PCopy(fn,(*Aliases)[which].spec.name);
	ComposeRString(title,NICK_HEAD_FMT,selectStr,fn);
}

//
//	GetAliasExpansionFor822Flavor
//
//		This is derived from InsertAlias. Originally, this code was buried within the InsertAlias
//		function in 'nickexp.c', special-cased because there was some commonality to what that
//		function did.  It made my head hurt so I moved it here.
//

Handle GetAliasExpansionFor822Flavor (short which, short index)

{
	BinAddrHandle	wordH,
								list;
	Str31					nameStr;
	long					tempSize,
								addressSize;;

	GetNicknameNamePStr (which, index, nameStr);
	addressSize = *nameStr + 1;
	wordH = NuHandle (addressSize + 5);
	if (!wordH) {
		WarnUser (MEM_ERR, MemError ());
		return (nil);
	}
	
	BlockMoveData (nameStr, *wordH, nameStr[0] + 1);
	(*wordH)[**wordH + 1] = (*wordH)[**wordH + 2] = 0;
	ExpandAliases (&list, wordH, 0, true);
	ZapHandle (wordH);
	
	// (jp) Bad ju-ju ahead... if the list is zero length, we correctly do not replace the existing typing
	//			with nothing (what would be the point?), but we incorrectly lose the caret.
	if (list) {
		tempSize = GetHandleSize_ (list);
		if (tempSize == 1 && strlen (LDRef (list)) == 0)
			ZapHandle (list);
		else
			UL (list);
	}
	
	if (list)
		CommaList (list);

	return (list);
}

/**********************************************************************
 * NickAddrSetDragContents - add the right stuff to a drag
 **********************************************************************/
pascal OSErr NickAddrSetDragContents(PETEHandle pte,DragReference drag)
{
	OSErr err = noErr;
	Handle	text = nil,commaText=nil;
	long	selStart,selEnd;
	
	err = PeteGetTextAndSelection(pte,&text,&selStart,&selEnd);
	if (err) return(err);
	commaText = NuHandle(0);
	if (!commaText)
		return(MemError());
	err = HandPlusHand(text,commaText);
	if(err)
		{ZapHandle(commaText);return(err);}
	Tr(commaText,"\015",",");

	err = AddDragItemFlavor(drag, 1L, 'TEXT', LDRef(text) + selStart, selEnd - selStart, 0);
	if (!err)
		err = AddDragItemFlavor(drag, 1L, A822_FLAVOR, LDRef(commaText) + selStart, selEnd - selStart, 0);
	
	UL(commaText);
//	ZapHandle(text);
	ZapHandle(commaText);
	
	return(err);
}

/**********************************************************************
 * NickGetDragContents - get the right stuff from a drag
 **********************************************************************/
OSErr SuckDragAddresses(DragReference drag,Handle *addresses,Boolean leadingComma,Boolean trailingComma);
pascal OSErr NickGetDragContents(PETEHandle pte,UHandle *theText, PETEStyleListHandle *theStyles, PETEParaScrapHandle *theParas, DragReference drag, long dropLocation)
{
	OSErr err = handlerNotFoundErr;
	long theLen = PeteLen(pte);
		*theText = nil;
		err = SuckDragAddresses(drag,theText,dropLocation > 0 && theLen != 0,dropLocation < theLen);
	return(err);
}

/**********************************************************************
 * TheWorldAccordingToMarthaStewart - is a window dirty?
 **********************************************************************/
Boolean TheWorldAccordingToMarthaStewart(MyWindowPtr win)
{
	return false;
}



void DoAddressBookExport (Boolean select)

{
	TabFieldHandle		fields;
	NickExportRec			nickExport;
	FSSpec						spec;
	Str255						windowTitle,
										tag;
	Str15							ctext;
	UPtr							spot,
										end;
	OSErr							theError;
	long							creator;
	short							ab;
	ModalFilterYDUPP filter=nil;
	
	GetRString (spec.name, UNTITLED_CSV);
	GetRString (windowTitle, NICK_SAVE_AS_TITLE);
	GetPref (ctext, PREF_CREATOR);
	if (*ctext != 4) GetRString (ctext, TEXT_CREATOR);
	BMD (ctext + 1, &creator, 4);
	if (theError = SFPutOpen (&spec, creator, 'TEXT', &nickExport.refNum, nil, filter, 0, nil, windowTitle, nil))
		return;

	OpenProgress();
	ProgressR (NoBar, 0, EXPORTING_NICKNAMES, EXPORTING_SUBTITLE, nil);

	nickExport.theError = noErr;
	
	// Stringy stuff
	GetRString (nickExport.eol, NICK_EXPORT_EOL);
	GetRString (nickExport.comma, NICK_EXPORT_COMMA);
	GetRString (nickExport.commaReplace, NICK_COMMA_REPLACE_CHAR);
	GetRString (nickExport.crReplace, NICK_CR_REPLACE_CHAR);
	
	// Initialize the accumulators
	theError = AccuInit (&nickExport.a);
		
	// Place the field names into the accumulator -- this is the first line written to the file.  The nickname
	// field is always the first field, followed by the rest in tab field order
	if (!theError)
		theError = AccuAddStr (&nickExport.a, GetRString (tag, ABReservedTagsStrn + abNickname));
	if (!theError)
		if (fields = GetExportableTabFieldStrings (Tabs)) {
			spot = LDRef (fields);
			end	 = spot + GetHandleSize (fields);
			while (spot < end && !theError) {
				if (*spot) {
					theError = AccuAddStr (&nickExport.a, nickExport.comma);
					if (!theError)
						theError = AccuAddStr (&nickExport.a, spot);
				}
				spot += (*spot + 2);
			}
			if (!theError)
				theError = AccuAddStr (&nickExport.a, nickExport.eol);
			ZapHandle (fields);
		}
		
	// If no address books are selected, we'll save them all.  Otherwise, export only those that are selected.
	// In either case, field values are added in the same order as the field names -- nickname followed by tab order.
	for (ab = 0; ab < NAliases && !theError; ++ab)
		if (!IsPluginAddressBook (ab))
			theError = ExportNickFile (&nickExport, ab, select);

	// Anything left to write to the file?
	if (!theError)
		theError = AccuWrite (&nickExport.a, nickExport.refNum);
	if (!theError)
		theError = TruncAtMark (nickExport.refNum);

	(void) MyFSClose (nickExport.refNum);

	AccuZap (nickExport.a);
	
	CloseProgress ();
		
	if (theError)
		WarnUser (NICK_EXPORT_FAIL, theError);
}


OSErr ExportNickFile (NickExportPtr nickExport, short which, Boolean select)

{
	OSErr	theError;
	short	**sort,
				ab,
				nick,
				count,
				myCount,
				firstIndex,
				i;
	
	
	theError = noErr;
	myCount	 = 0;
	if (select) {
		count	= LVCountSelection (&NickList);
		for (i = 1; i <= count; ++i) {
			GetSelectedABNick (i, &ab, &nick);
			if (ab == which) {
				if (ValidNickname (nick)) {
					if (++myCount == 1)
						firstIndex = i;
				}
				else {
					select = false;
					break;
				}
			}
		}
	}
	
	if (myCount == 0 && select)
		return (noErr);
		
	if (select) {
		for (i = firstIndex; !theError && !CommandPeriod && myCount; ++i) {
			GetSelectedABNick (i, &ab, &nick);
			theError = ExportNickname (nickExport, ab, nick);
			myCount--;
		}
	}
	else {
		// We'll sort before exporting, just to be nice
		SortAddressBook (which);
		sort = (*Aliases)[which].sortData;
		if (!sort)
			return (memFullErr);
		count = (*Aliases)[which].theData ? (GetHandleSize_ ((*Aliases)[which].theData) / sizeof (NickStruct)) : 0;
		for (nick = 0; !theError && !CommandPeriod && nick < count; ++nick)
			if (!(*((*Aliases)[which].theData))[(*sort)[nick]].deleted)
				theError = ExportNickname (nickExport, which, (*sort)[nick]);
	}

	return (theError);
}


OSErr ExportNickname (NickExportPtr nickExport, short ab, short nick)

{
	Str255	nickname;
	OSErr		theError;

	ProgressMessage (kpMessage, GetNicknameNamePStr (ab, nick, nickname));
	CycleBalls ();

	// The nickname itself is always first
	theError = AccuAddStr (&nickExport->a, nickname);
	
	// Following the nickname are all the fields, in tab order
	if (!theError) {
		nickExport->ab	 = ab;
		nickExport->nick = nick;
		IterateTabObjectsUL (Tabs, ExportNicknameProc, nickExport);
		theError = nickExport->theError;
	}
	
	// Line termination
	if (!theError)
		theError = AccuAddStr (&nickExport->a, nickExport->eol);
	// Maybe write out what we've accumulated so far
	if (!theError && nickExport->a.offset > 16 K) {
		theError = AccuWrite (&nickExport->a, nickExport->refNum);
		nickExport->a.offset = 0;
	}
	return (theError);
}


Boolean ExportNicknameProc (TabObjectPtr objectPtr, NickExportPtr nickExport)

{
	TextAddrHandle	addresses;
	Str255					value;
	Handle					tempHandle;
	OSErr						theError;
	
	theError = noErr;
	// add the value
	if (objectPtr->flags & objectFlagExportable) {
		// lead off with a comma before each field value
		theError = AccuAddStr (&nickExport->a, nickExport->comma);

		if (EqualStrRes (objectPtr->tag, ABReservedTagsStrn + abTagEmail)) {
			addresses = nil;
			if (tempHandle = GetNicknameData (nickExport->ab, nickExport->nick, true, true)) {
				theError = SuckAddresses (&addresses, tempHandle, true, true, false, nil);
				if (!theError && addresses && **addresses) {
					CommaList (addresses);
					Tr (addresses, ",", nickExport->commaReplace);
					theError = AccuAddHandle (&nickExport->a, addresses);
				}
			}
			ZapHandle (addresses);
		}
		else {
			GetTaggedFieldValueStr (nickExport->ab, nickExport->nick, objectPtr->tag, value);
			if (*value) {
				TrLo (&value[1], *value, ",", nickExport->commaReplace);
				TrLo (&value[1], *value, "\015", nickExport->crReplace);
				TrLo (&value[1], *value, "\003", nickExport->crReplace);
			}
			theError = AccuAddStr (&nickExport->a, value);
		}
	}
	nickExport->theError = theError;
	return (false);
}

#ifdef VCARD

//
//	AutoGeneratePersonalInformation
//
//		Auto-generate personal information and add it into a given address book
//		as a nickname.
//

OSErr AutoGeneratePersonalInformation (short ab)

{
	StringHandle	hAddress;
	Handle				expansion;
	Str255				returnAddress;
	Str31					realName,
								firstName,
								lastName,
								nickname;
	OSErr					theError;
	
	theError = noErr;
			
	*returnAddress = 0;
	*firstName		 = 0;
	*lastName			 = 0;
	expansion			 = nil;

	// Grab some important vCard-ish information from the dominant personality
	GetDominantPref (PREF_REALNAME, realName);
	if (hAddress = NewString (GetShortReturnAddr (returnAddress))) {
		theError = SuckPtrAddresses (&expansion, LDRef (hAddress), GetHandleSize (hAddress), true, true, false, nil);
		ZapHandle (hAddress);
	}

	// Make it into a nickname
	if (!theError && *realName) {
		ParseFirstLast (realName, firstName, lastName);
		PSCopy (nickname, realName);
		BeautifyFrom (nickname);
		SanitizeFN (nickname, nickname, NICK_BAD_CHAR, NICK_REP_CHAR, false);
		*nickname = MIN (*nickname, sizeof (Str31) - 1);
		NewNickLow (expansion, CreateSimpleNotes (realName, firstName, lastName), 0, nickname, false, nrNone, true);
	}
	return (theError);
}


OSErr MakeVCardFile (FSSpec *spec, short ab, short nick)

{
	FSSpec	nickFolder,
					vcardFolder;
	Str255	vcardFolderName,
					nickname;
	Handle	vcardData,
					notes;
	OSErr		theError;
	long		dirID;

	UseFeature (featureVCard);

	theError = noErr;
	// Make a vCard from this nickname
	if (notes = GetNicknameData (ab, nick, false, true))
		Tr (notes, "\003", "\015");
	if (vcardData = MakeVCard (GetNicknameData (ab, nick, true, true), notes)) {
		// Find the Nicknames Folder
		if (theError = SubFolderSpec (NICK_FOLDER, &nickFolder)) {
			SimpleMakeFSSpec (Root.vRef, Root.dirId, GetRString (nickFolder.name, NICK_FOLDER), &nickFolder);
			theError = FSpDirCreate (&nickFolder, smSystemScript, &dirID);
		}
		if (!theError) {
			IsAlias (&nickFolder, &nickFolder);
			theError = FSMakeFSSpec (nickFolder.vRefNum, SpecDirId (&nickFolder), GetRString (vcardFolderName, VCARD_FOLDER), &vcardFolder);
			// Create the vCards Folder if needed
			if (theError == fnfErr) 
				theError = FSpDirCreate (&vcardFolder, smSystemScript, &dirID);
		}
		
		// Make an FSSpec for the vCard itself
		if (!theError) {
			IsAlias (&vcardFolder, &vcardFolder);
			theError = FSMakeFSSpec (vcardFolder.vRefNum, SpecDirId (&vcardFolder), MakeVCardFileName (ab, nick, nickname), spec);
			if (theError == fnfErr) {
				FSpCreateResFile (spec, CREATOR, VCARD_TYPE, smSystemScript);
				theError = ResError ();
			}
		}
	
		// Write the vCard data into the file
		if (!theError) {
			IsAlias (spec, spec);
			theError = Blat (spec, vcardData, false);
		}

		ZapHandle (vcardData);
	}
	else
		theError = MemError ();
	return (theError);
}
#endif

#ifdef DEBUG
void PaintUpdateRgn (WindowPtr theWindow, short color)

{
	GrafPtr		savePort;
	RGBColor	oldColor;
	RgnHandle	rgn;
	Point			offsetPt;

	GetPort (&savePort);
	SetPort (GetWindowPort (theWindow));
	rgn = MyGetWindowUpdateRegion(theWindow);
	GetForeColor (&oldColor);
	ForeColor (color);
	offsetPt.h = offsetPt.v = 0;
	GlobalToLocal (&offsetPt);
	OffsetRgn (rgn, offsetPt.h, offsetPt.v);
	Debugger ();
	PaintRgn (rgn);
	RGBForeColor (&oldColor);
	OffsetRgn(rgn, -offsetPt.h, -offsetPt.v);
	SetPort (savePort);
}
#endif

/************************************************************************
 * ABUnselectCurrentNickname - unselect the currently selected nickname
 ************************************************************************/
void ABUnselectCurrentNickname(void)
{
	short ab, nick;
	Str31 massagedName;
	
	if (LVCountSelection (&NickList) == 1)
	{
		ab = AB.abDisplayed;
		nick = AB.nickDisplayed;
		
		if (ab != kAddressBookUndefined && nick != kNickUndefined)
		{
			GetListNameBasedOnTag (SortTag, ab, nick, massagedName);
			LVUnselect (&NickList, MakeNodeID (ab, nick), massagedName, true);
		}
	}
}


//
//	SortAddressBooks
//
//		Sort regular address books.  It is currently assumed that
//		these are in order.
//

void SortAddressBookList (void)

{
	short	totalAB,
				ab,
				firstRegular,
				lastRegular;
	
	firstRegular = kAddressBookUndefined;
	lastRegular	 = kAddressBookUndefined;
	totalAB = NAliases;
	ab = 0;
	while (ab < totalAB && firstRegular == kAddressBookUndefined) {
		if (IsRegularAddressBook (ab))
			firstRegular = ab;
		++ab;
	}
	ab = totalAB - 1;
	while (ab >= 0 && lastRegular == kAddressBookUndefined) {
		if (IsRegularAddressBook (ab))
			lastRegular = ab;
		--ab;
	}
	if (ValidAddressBook (firstRegular) && ValidAddressBook (lastRegular) && firstRegular != lastRegular) {
		QuickSort (LDRef (Aliases), sizeof (AliasDesc), firstRegular, lastRegular, (void*) abCompare,(void*) abSwap);
		UL (Aliases);
	}
}

int abCompare (AliasDPtr adp1, AliasDPtr adp2)

{
	return (StringComp (adp1->spec.name, adp2->spec.name));
}


void abSwap (AliasDPtr adp1, AliasDPtr adp2)

{
	AliasDesc temp = *adp2;
	*adp2 = *adp1;
	*adp1 = temp;
}

Boolean ABIsChattable(void)
{
	short index = 1;
	short ab;
	short nick;
	Boolean retVal = false;
	Str31 aim, aim2;
	Str255 value;
	
	if (GetOSVersion() < 0x1030) return false;
	
	GetRString(aim,ABReservedTagsStrn+abTagAIM);
	GetRString(aim2,ABReservedTagsStrn+abTagAIM2);
	
	while (GetSelectedABNick (index++, &ab, &nick))
		if (nick >= 0) {
			if (!(*GetTaggedFieldValueStr(ab, nick, aim, value) || *GetTaggedFieldValueStr(ab, nick, aim2, value))) return false;
			else retVal = true;
		}
	return retVal;
}

OSErr ABChat(short modifiers)
{
	short ab;
	short nick;
	Boolean retVal = false;
	Str31 aim, aim2;
	Str255 value, url;
	
	GetRString(aim,ABReservedTagsStrn+abTagAIM);
	GetRString(aim2,ABReservedTagsStrn+abTagAIM2);
	
	if (GetSelectedABNick (1, &ab, &nick))
	{
		if (*GetTaggedFieldValueStr(ab, nick, aim, value) || *GetTaggedFieldValueStr(ab, nick, aim2, value))
		{
			ComposeRString(url,AIM_URL_FMT,URLEscape(value));
			return OpenOtherURLPtr(GetRString(aim,AIM_PROTO),url+1,*url);
		}
	}
	return fnfErr;
}