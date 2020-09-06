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

#include "makefilter.h"
#define FILE_NUM 101
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment Makefilter

// Real Name Queue Structure - to maintain a list of entire addresses.
typedef struct RNQ RealNameQ, *RealNameQPtr, **RealNameQHandle;
struct RNQ
{
	RealNameQHandle next;
	Str255 realName;
};

// RecPopup structure.  Makes it easier to maintain the list and popup in parallel
typedef struct RecPopup
{
	MenuHandle menu;
	RealNameQHandle listOfNames;
} RecPopup, *RecPopupPtr;

#define Toggle(b) {if (!b) b = true; else b = false;}
#define IGNORE_RECIPIENTS (RealNameQHandle)(-1)
			
OSErr DoMakeFilterDialog(Boolean *create, Boolean *details, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo);
OSErr SetUpFilterFromComp(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo);	
OSErr SetUpFilterFromMbox(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo);	
OSErr SetUpFilterFromMess(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo);	
OSErr GatherAddresses(MessHandle messH, HeaderEnum header, Handle *dest, Boolean wantComments);
OSErr CreateTheFilter(MakeFilterRecPtr mfRecPtr, Boolean details);
void SetProperDefaultFolder(MakeFilterRecPtr mfRecPtr, Boolean warnUser);
OSErr FileSpecByName(FSSpecPtr spec,PStr name);
OSErr FileSpecByNameInMenu(MenuHandle mh,FSSpecPtr spec,PStr name);
short FindFileByNameIn1Menu(MenuHandle mh, PStr name);
UPtr SuggestAMailboxName(MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo);
short AnyRPopup(DialogPtr dlog,short top,short left,MenuHandle mh);
OSErr ANDAddressesToListOfNames(RecPopupPtr popupInfo, UHandle addresses);
void FlushRNQ(RecPopupPtr popupInfo);
OSErr BuildAnyRPopupMenu(RecPopupPtr popupInfo);
void ChangeMenuForFolderSelect(short menu);
void TweakSubMenuForFolderSelect(short menu);
void SetDefaultLocation(DialogPtr dlog, Str255 defaultText);

/**********************************************************************
 * DoMakeFilter - do Make Filter.
 **********************************************************************/
void DoMakeFilter(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	OSErr err = noErr;
	Boolean create, details;
	MakeFilterRec mfRec;
	RecPopup anyRPopupMenuInfo;
	
#ifdef	IMAP
	// make sure the message is downloaded before doing the make filter ...
	if (GetWindowKind(winWP)==MESS_WIN)
	{
		MessHandle messH;

		// we're doing a make filter with a message window frontmost.
		if ((messH  = Win2MessH(win)) && ((*(*messH)->tocH)->imapTOC))
		{
			LDRef(messH);
			EnsureMsgDownloaded((*messH)->tocH, (*messH)->sumNum, false);
			UL(messH);
		}
	}
	else if (GetWindowKind(winWP)==MBOX_WIN) 
	{
		TOCHandle tocH;
	 	short sumNum;
	
		// we're doing a make filter with a mailbox frontmost.
		if ((tocH = (TOCHandle)GetMyWindowPrivateData(win)) && (*tocH)->imapTOC)
		{
			for (sumNum=0; sumNum < (*tocH)->count; sumNum++)
			{
				if ((*tocH)->sums[sumNum].selected)	
					EnsureMsgDownloaded(tocH, sumNum, false);	
			}
		}
	}
#endif
	
	// Initialize
	WriteZero(&mfRec,sizeof(MakeFilterRec));
	WriteZero (&anyRPopupMenuInfo,sizeof(RecPopup));
	
	//Determine the initial values for the make filter dialog
	if (GetWindowKind(winWP)==COMP_WIN) 
		err = SetUpFilterFromComp(win, &mfRec, &anyRPopupMenuInfo);
	else if ((GetWindowKind(winWP)==MBOX_WIN) || (GetWindowKind(winWP)==CBOX_WIN)) 
		err = SetUpFilterFromMbox(win, &mfRec, &anyRPopupMenuInfo);
	else if (GetWindowKind(winWP)==MESS_WIN) 
		err = SetUpFilterFromMess(win, &mfRec, &anyRPopupMenuInfo);
	
	if (err == noErr)
	{
		//build the anyR Popup menu from the anyRPopupMenuInfo gathered during initialization
		err = BuildAnyRPopupMenu(&anyRPopupMenuInfo);
	
		if (err == noErr)
		{
			//put the anyRPopupMenu into our list of menus, if it's been built.
			if (IsValidMenuHandle(anyRPopupMenuInfo.menu)) InsertMenu(anyRPopupMenuInfo.menu,-1);
				
			//Display the make filter dialog and allow the user to make changes
			err = DoMakeFilterDialog(&create, &details, &mfRec, &anyRPopupMenuInfo);
			
			//Create the filter.
			if (err == noErr && create) err = CreateTheFilter(&mfRec, details);
		}
	}
		
	//Cleanup
	if (IsValidMenuHandle(anyRPopupMenuInfo.menu)) 
	{
		DeleteMenu(MF_ANYR_POPUP_MENU);
		DisposeMenu(anyRPopupMenuInfo.menu);
	}
	if (anyRPopupMenuInfo.listOfNames) FlushRNQ(&anyRPopupMenuInfo);
}

/**********************************************************************
 * DoMakeFilterDialog - do the Make Filter dialog.
 **********************************************************************/
OSErr DoMakeFilterDialog(Boolean *create, Boolean *details, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo)
{
	OSErr err = noErr;
	PETEHandle pte;
	PETEDocInitInfo	pdi;
	MyWindowPtr	makeFilterDialogWin;
	DialogPtr makeFilterDialog;
	extern ModalFilterUPP DlgFilterUPP;
	short itemHit;
	Boolean done = false;
	ControlHandle transferButton;
	Str255 scratch;
	Boolean userDidTransfer = false;
	Boolean userTouchedNewMailboxName = false;	//set to true once the user touches the new mailbox name
	Handle anyRPopup;
	Rect anyRPopupRect;
	short anyRPopupType;
	Str255 noneChosen;
	short	textFont,textSize;
	SAVE_PORT;
	
	ASSERT(create && details && mfRecPtr);
	
	//Initialize various local things
	*create = *details = false;
	GetRString(noneChosen,MF_NONE_CHOSEN);
	 
	//get the makefilter dialog
	SetDialogFont(SmallSysFontID());
	makeFilterDialogWin = GetNewMyDialog(MAKEFILTER_DLOG,nil,nil,InFront);	
	SetDialogFont(0);

	makeFilterDialog = GetMyWindowDialogPtr (makeFilterDialogWin);
	if (!makeFilterDialog) return(WarnUser(MAKEFILTER_ERR,errMakeFilterOpen));	

	//fix the controls
	SetPort(GetDialogPort(makeFilterDialog));
	ConfigFontSetup(makeFilterDialogWin);
	ReplaceAllControls(makeFilterDialog);
			
	//Put the first item in the anyR popup in the anyRText field.
	if (IsValidMenuHandle(popupInfo->menu))
	{
	 	GetMenuItemText(popupInfo->menu, 1, mfRecPtr->anyRText);
		mfRecPtr->anyRSelection = 1;
	}
	else
	{ 
		mfRecPtr->anyRText[0] = 0;
		mfRecPtr->anyRSelection = 0;
	}

	// (jp) For setting up our PETE's.
	DefaultPII (makeFilterDialogWin, false, peNoStyledPaste | peClearAllReturns, &pdi);

	textFont = GetPortTextFont(GetQDGlobalsThePort());
	textSize = GetPortTextSize(GetQDGlobalsThePort());
	// (jp)	I've re-ordered things a bit...  Here, order of creation determines tabbing
	//			order in the world of user panes
	if (pte = CreatePeteUserPane (GetDItemCtl (makeFilterDialog, MF_FROM_EDIT), peNoStyledPaste | peClearAllReturns, &pdi, true)) {
		PeteFontAndSize (pte,textFont,textSize);
		SetNickScanning (pte, nickHighlight | nickComplete | nickSpaces, kNickScanAllAliasFiles, nil, nil, nil);
		makeFilterDialogWin->pte = pte;
		(void) SetKeyboardFocus (GetDialogWindow(makeFilterDialog), GetDItemCtl (makeFilterDialog, MF_FROM_EDIT), kControlEditTextPart);
	}
	if (pte = CreatePeteUserPane (GetDItemCtl (makeFilterDialog, MF_ANYR_EDIT), peNoStyledPaste | peClearAllReturns, &pdi, true)) {
		PeteFontAndSize (pte,textFont, textSize);
		SetNickScanning (pte, nickHighlight | nickComplete | nickSpaces, kNickScanAllAliasFiles, nil, nil, nil);
	}
	if (pte = CreatePeteUserPane (GetDItemCtl (makeFilterDialog, MF_SUBJECT_EDIT), peNoStyledPaste | peClearAllReturns, &pdi, true))
		PeteFontAndSize (pte,textFont, textSize);
	if (pte = CreatePeteUserPane (GetDItemCtl (makeFilterDialog, MF_TRANSFERNEW_EDIT), peNoStyledPaste | peClearAllReturns, &pdi, true))
		PeteFontAndSize (pte,textFont, textSize);

	CleanPII(&pdi);
	
	// (jp) Place the "Any recipient" text in a PETE user pane
	SetPeteDItemText (makeFilterDialogWin, MF_ANYR_EDIT, mfRecPtr->anyRText);

	// Set the new mailbox and new mailbox in location text.
	SetProperDefaultFolder(mfRecPtr,true);
	SuggestAMailboxName (mfRecPtr, popupInfo);
	SetDefaultLocation(makeFilterDialog, mfRecPtr->defaultText);
	SetPeteDItemText (makeFilterDialogWin, MF_TRANSFERNEW_EDIT, mfRecPtr->transferToNew.name);

	// Set the Existing Mailbox button to the same mailbox the messages are in, if they're not in IN or OUT:
	transferButton = GetDItemCtl(makeFilterDialog,MF_TRANSFEREXISTING_BUTTON);
	
	// If the message is in the In or Out box, NONE CHOSEN is the default.
	if (mfRecPtr->transferToExisting.name[0] == 0)
	{
		SetDICTitle(makeFilterDialog,MF_TRANSFEREXISTING_BUTTON,noneChosen);
	}
	else	//use the same mailbox the message is currently in
	{
		SetDICTitle(makeFilterDialog,MF_TRANSFEREXISTING_BUTTON,MailboxSpecAlias(&(mfRecPtr->transferToExisting),scratch));
		mfRecPtr->action = MF_TRANSFEREXISTING_RADIO;
		userDidTransfer = true;	//and pretend the user already did a transfer to existing
	}
					
	GetDialogItem(makeFilterDialog,MF_ANYR_POPUP,&anyRPopupType,&anyRPopup,&anyRPopupRect);

	//Enter the right values into the controls
	SetDItemState(makeFilterDialog,MF_INCOMING_CHECKBOX,mfRecPtr->incoming);
	SetDItemState(makeFilterDialog,MF_OUTGOING_CHECKBOX,mfRecPtr->outgoing);
	SetDItemState(makeFilterDialog,MF_MANUAL_CHECKBOX,mfRecPtr->manual);
	
	SetDItemState(makeFilterDialog,MF_SUBJECT_RADIO,mfRecPtr->match==MF_SUBJECT_RADIO);
	SetDItemState(makeFilterDialog,MF_FROM_RADIO,mfRecPtr->match==MF_FROM_RADIO);
	SetDItemState(makeFilterDialog,MF_ANYR_RADIO,mfRecPtr->match==MF_ANYR_RADIO);
	
	SetDItemState(makeFilterDialog,MF_DELETE_RADIO,mfRecPtr->action==MF_DELETE_RADIO);
	SetDItemState(makeFilterDialog,MF_TRANSFERNEW_RADIO,mfRecPtr->action==MF_TRANSFERNEW_RADIO);
	SetDItemState(makeFilterDialog,MF_TRANSFEREXISTING_RADIO,mfRecPtr->action==MF_TRANSFEREXISTING_RADIO);

	SetPeteDItemText (makeFilterDialogWin, MF_SUBJECT_EDIT, mfRecPtr->subjectText);
	SetPeteDItemText (makeFilterDialogWin, MF_FROM_EDIT, mfRecPtr->fromText);
	
	//Do makefilter
	SetMyCursor(arrowCursor);
	StartMovableModal(makeFilterDialog);
	ShowWindow(GetDialogWindow(makeFilterDialog));
	HiliteButtonOne(makeFilterDialog);
	
	do
	{
		do
		{
			*create = *details = false;
			MovableModalDialog(makeFilterDialog,DlgFilterUPP,&itemHit);
			switch(itemHit)
			{
				case MF_DETAILS_BUTTON:
					*details = true;
				case OK_BUTTON:
					*create = true;
				case MF_CANCEL_BUTTON:
					if (itemHit == MF_CANCEL_BUTTON) err = noErr;
				case CANCEL_ITEM:		//handle the cmd-. or esc key
					done = true;
					break;
				
				case MF_INCOMING_CHECKBOX:
					Toggle(mfRecPtr->incoming);
					SetDItemState(makeFilterDialog,MF_INCOMING_CHECKBOX,mfRecPtr->incoming);
					break;
					
				case MF_OUTGOING_CHECKBOX:
					Toggle(mfRecPtr->outgoing);
					SetDItemState(makeFilterDialog,MF_OUTGOING_CHECKBOX,mfRecPtr->outgoing);
					break;
					
				case MF_MANUAL_CHECKBOX:
					Toggle(mfRecPtr->manual);
					SetDItemState(makeFilterDialog,MF_MANUAL_CHECKBOX,mfRecPtr->manual);
					break;
					
				case MF_SUBJECT_RADIO:			
				case MF_FROM_RADIO:	
				case MF_ANYR_RADIO:	
					mfRecPtr->match = itemHit;	  	
					
					//if the user hasn't modified the suggested mailbox name yet, make a new suggestion.
					if (!userTouchedNewMailboxName)
						SetPeteDItemText (makeFilterDialogWin, MF_TRANSFERNEW_EDIT, SuggestAMailboxName (mfRecPtr, popupInfo));
					
					//also update the new folder location
					SetProperDefaultFolder(mfRecPtr,true);
					SetDefaultLocation(makeFilterDialog, mfRecPtr->defaultText);
					break;
				
				case MF_DELETE_RADIO:		
				case MF_TRANSFERNEW_RADIO:
				case MF_TRANSFEREXISTING_RADIO:
					mfRecPtr->action = itemHit;
					SetDItemState(makeFilterDialog,MF_DELETE_RADIO,mfRecPtr->action==MF_DELETE_RADIO);
					SetDItemState(makeFilterDialog,MF_TRANSFERNEW_RADIO,mfRecPtr->action==MF_TRANSFERNEW_RADIO);
					SetDItemState(makeFilterDialog,MF_TRANSFEREXISTING_RADIO,mfRecPtr->action==MF_TRANSFEREXISTING_RADIO);

					//if this is the first time the user has hit "Transfer to existing", fall through and select a mbox
					if (itemHit == MF_TRANSFEREXISTING_RADIO && !userDidTransfer);
					else break;
				
				case MF_TRANSFEREXISTING_BUTTON:
					if (ChooseMailbox(TRANSFER_MENU,CHOOSE_MBOX_TRANSFER,&(mfRecPtr->transferToExisting)))
					{
						//remember that the user hit transfer.
						userDidTransfer = true;
						
						//set the transfer existing button title
						SetDICTitle(makeFilterDialog,MF_TRANSFEREXISTING_BUTTON,MailboxSpecAlias(&(mfRecPtr->transferToExisting),scratch));
						mfRecPtr->action = MF_TRANSFEREXISTING_RADIO;
					}
					else if (HasCommandPeriod()) CommandPeriod = false;	//hack to keep MovableModalDialog from seeing ESC if pressed during ChooseMailbox.
					break;
					
				case MF_TRANSFERNEW_EDIT:
					//Once the user enters something into this field, stop suggesting names for mailboxes.
					mfRecPtr->action = MF_TRANSFERNEW_RADIO;
					userTouchedNewMailboxName = true;	//remember that the user has hit this field.
					break;
				
				case MF_ANYR_POPUP:
					if (IsValidMenuHandle(popupInfo->menu))
					{
						mfRecPtr->anyRSelection = AnyRPopup(makeFilterDialog, anyRPopupRect.top, anyRPopupRect.left, popupInfo->menu);
						GetMenuItemText(popupInfo->menu, mfRecPtr->anyRSelection, mfRecPtr->anyRText);
						mfRecPtr->match = MF_ANYR_RADIO;
						
						//if the user hasn't modified the suggested mailbox name yet, make a new suggestion.
						if (!userTouchedNewMailboxName)
							SetPeteDItemText (makeFilterDialogWin, MF_TRANSFERNEW_EDIT, SuggestAMailboxName (mfRecPtr, popupInfo));
						
						//also update the new folder location
						SetProperDefaultFolder(mfRecPtr,false);
						SetDefaultLocation(makeFilterDialog, mfRecPtr->defaultText);
					}
					break;
									
				case MF_SUBJECT_EDIT:
				case MF_FROM_EDIT:
				case MF_ANYR_EDIT:
					if (makeFilterDialogWin->pte == GetPeteDItem (makeFilterDialogWin, MF_SUBJECT_EDIT))
						mfRecPtr->match = MF_SUBJECT_RADIO;
					else
					if (makeFilterDialogWin->pte == GetPeteDItem (makeFilterDialogWin, MF_FROM_EDIT))
						mfRecPtr->match =  MF_FROM_RADIO;
					else
					if (makeFilterDialogWin->pte == GetPeteDItem (makeFilterDialogWin, MF_ANYR_EDIT))
						mfRecPtr->match = MF_ANYR_RADIO;
					else
						userTouchedNewMailboxName = true;	//remember that the user has hit this field.
					
					//suggest a new mailbox name
					if (!userTouchedNewMailboxName)
						SetPeteDItemText (makeFilterDialogWin, MF_TRANSFERNEW_EDIT, SuggestAMailboxName (mfRecPtr, popupInfo));
					
					//also update the new folder location
					SetProperDefaultFolder(mfRecPtr,false);
					SetDefaultLocation(makeFilterDialog, mfRecPtr->defaultText);
					break;
					
				case MF_DEFAULT:	
				default:
					break;
			}
			
			//set the radio buttons
			SetDItemState(makeFilterDialog,MF_SUBJECT_RADIO,mfRecPtr->match==MF_SUBJECT_RADIO);
			SetDItemState(makeFilterDialog,MF_FROM_RADIO,mfRecPtr->match==MF_FROM_RADIO);
			SetDItemState(makeFilterDialog,MF_ANYR_RADIO,mfRecPtr->match==MF_ANYR_RADIO);
			SetDItemState(makeFilterDialog,MF_DELETE_RADIO,mfRecPtr->action==MF_DELETE_RADIO);
			SetDItemState(makeFilterDialog,MF_TRANSFERNEW_RADIO,mfRecPtr->action==MF_TRANSFERNEW_RADIO);
			SetDItemState(makeFilterDialog,MF_TRANSFEREXISTING_RADIO,mfRecPtr->action==MF_TRANSFEREXISTING_RADIO);
						
		} while (!done);
	
		//Gather values out of the dialog before putting it away
		if (*create)
		{	
			GetPeteDItemText (makeFilterDialogWin, MF_SUBJECT_EDIT, mfRecPtr->subjectText);
			GetPeteDItemText (makeFilterDialogWin, MF_FROM_EDIT, mfRecPtr->fromText);
			GetPeteDItemText (makeFilterDialogWin, MF_ANYR_EDIT, mfRecPtr->anyRText);

			if (mfRecPtr->action==MF_DELETE_RADIO) GetRString(mfRecPtr->transferToExisting.name, TRASH);
			else if (mfRecPtr->action==MF_TRANSFERNEW_RADIO) GetPeteDItemText (makeFilterDialogWin, MF_TRANSFERNEW_EDIT, mfRecPtr->transferToNew.name);
			
			//create the new mailbox, if we ought to.
			if (mfRecPtr->action==MF_TRANSFERNEW_RADIO)
			{
#ifdef	IMAP
				// things are different for IMAP mailboxes
				if (IsIMAPVD(mfRecPtr->transferToNew.vRefNum, mfRecPtr->transferToNew.parID))
				{
					FSSpec spec;
					
					//First, see if the mailbox already exists.  If it does, just do a transfer to existing.
					if (FSMakeFSSpec(mfRecPtr->transferToNew.vRefNum, mfRecPtr->transferToNew.parID,mfRecPtr->transferToNew.name,&spec)==noErr)
					{
						spec.parID = SpecDirId(&spec);
						
						mfRecPtr->action = MF_TRANSFEREXISTING_RADIO;
						mfRecPtr->transferToExisting.vRefNum = spec.vRefNum;
						mfRecPtr->transferToExisting.parID = spec.parID;
						PCopy(mfRecPtr->transferToExisting.name,spec.name);
					}
					else	//otherwise, create the new mailbox.
					{	
						if (BadMailboxName(&mfRecPtr->transferToNew,false)) done = false;	//Creation failed.  
					}
				}
				else
				{
#endif
					//First, see if the mailbox already exists.  If it does, just do a transfer to existing.
					if (!BoxSpecByName(&mfRecPtr->transferToNew,mfRecPtr->transferToNew.name))
					{
						mfRecPtr->action = MF_TRANSFEREXISTING_RADIO;
						mfRecPtr->transferToExisting.vRefNum = mfRecPtr->transferToNew.vRefNum;
						mfRecPtr->transferToExisting.parID = mfRecPtr->transferToNew.parID;
						PCopy(mfRecPtr->transferToExisting.name,mfRecPtr->transferToNew.name);
					}
					else	//otherwise, create the new mailbox.
					{	
						if (BadMailboxName(&mfRecPtr->transferToNew,false)) 
							done = false;	//bad name.  Don't allow the user to continue.
						else
							AddBoxHigh(&mfRecPtr->transferToNew);
					}
#ifdef	IMAP
				}
#endif
			}
			else if (mfRecPtr->action==MF_TRANSFEREXISTING_RADIO) //grab the name of the mailbox to transfer to.
			{
				GetDICTitle(makeFilterDialog,MF_TRANSFEREXISTING_BUTTON,mfRecPtr->transferToExisting.name);
				
				//if the user hasn't selected a mailbox from the menus, continue with no mailbox specified.
				if (StringSame(mfRecPtr->transferToExisting.name,noneChosen)) WriteZero(&(mfRecPtr->transferToExisting),sizeof(FSSpec));
			}
		}
	} while (!done);
	
	//Cleanup
	DeleteMenu(MF_ANYR_POPUP_MENU);
	DisposePeteUserPaneItem (makeFilterDialogWin, MF_FROM_EDIT);
	DisposePeteUserPaneItem (makeFilterDialogWin, MF_ANYR_EDIT);
	DisposePeteUserPaneItem (makeFilterDialogWin, MF_SUBJECT_EDIT);
	DisposePeteUserPaneItem (makeFilterDialogWin, MF_TRANSFERNEW_EDIT);
	EndMovableModal(makeFilterDialog);
	DisposDialog_(makeFilterDialog);
	
	REST_PORT;
	
	return(err);
}
							
/**********************************************************************
 * SetUpFilterFromComp - set up the filter according to a comp window
 **********************************************************************/
OSErr SetUpFilterFromComp(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo)
{
	OSErr err = noErr;
	MessHandle messH  = Win2MessH(win);
	short heads[] = {TO_HEAD, CC_HEAD, BCC_HEAD};
	short h;
	UHandle addresses = 0;
	
	mfRecPtr->outgoing = true;
	mfRecPtr->manual = true;

	//Grab the addresses in the to:, cc:, or bcc: headers
	for (h=0;!err && h<sizeof(heads)/sizeof(short);h++)
		err = GatherAddresses(messH, heads[h], &addresses, true);
	
	//put together the list of recipients
	err = ANDAddressesToListOfNames(popupInfo, addresses);
	
	if (addresses) ZapHandle(addresses);
	
	//if there was an error building the recipient list, zero it out, but continue.
	if (err != noErr)	
		if (popupInfo->listOfNames && popupInfo->listOfNames != IGNORE_RECIPIENTS) 
			FlushRNQ(popupInfo);
	
	//Grab the address in the from: header
	err = GatherAddresses(messH, FROM_HEAD, &addresses, false);
	if (err || !addresses || !*addresses || !**addresses || (GetHandleSize(addresses)==0))
		mfRecPtr->fromText[0] = 0;
	else 
	{
		Str255 scratch;
		
		PCopy(mfRecPtr->fromText, *addresses);
		
		//grab the real name of the sender, while we're at it.
		if (addresses) ZapHandle(addresses);
		if (GatherAddresses(messH, FROM_HEAD, &addresses, true)==noErr)
		{
			if (addresses && !*addresses && (GetHandleSize(addresses)!=0))
			{
				PCopy(scratch, *addresses);
				BeautifyFrom(scratch);
				if (!StringSame(mfRecPtr->fromText, scratch)) PCopy(mfRecPtr->fromRealName, scratch);
			}
		}
	}
	
	if (addresses) ZapHandle(addresses);
	
	//Grab the subject
	SuckHeaderText(messH, mfRecPtr->subjectText, sizeof(mfRecPtr->subjectText), SUBJ_HEAD);
	TrimRe(mfRecPtr->subjectText, false);
	TrimInitialWhite(mfRecPtr->subjectText);
	TrimWhite(mfRecPtr->subjectText);
	
	//set the default match and action.
	if (mfRecPtr->fromText[0]) mfRecPtr->match = MF_FROM_RADIO;
	else if (mfRecPtr->anyRText[0]) mfRecPtr->match = MF_ANYR_RADIO;
	else mfRecPtr->match = MF_SUBJECT_RADIO;
	
	mfRecPtr->action = MF_TRANSFERNEW_RADIO;
	
	return (err);
}

/**********************************************************************
 * SetUpFilterFromMess - set up filter from at a message window
 **********************************************************************/
OSErr SetUpFilterFromMess(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo)
{
	OSErr err = noErr;
	MessHandle messH;
	short heads[] = {TO_HEAD, CC_HEAD, BCC_HEAD};
	short h;
	UHandle addresses = 0;
	FSSpec	spec;
		
	messH  = Win2MessH(win);
		
	// 11/7/97 - sdorner requested that Incoming and Manual be checked by default for all non-out messages.
	mfRecPtr->incoming = true;
	mfRecPtr->outgoing = false;
	mfRecPtr->manual = true;
		
	// Remember what mailbox this message is in, if it's not the in box.  
	// Will use this as the transfer to existing choice.
	if (((*(*messH)->tocH)->which!=IN) && ((*(*messH)->tocH)->which!=OUT))
	{
		spec = GetMailboxSpec((*messH)->tocH,-1);
		mfRecPtr->transferToExisting.vRefNum = spec.vRefNum;
		mfRecPtr->transferToExisting.parID = spec.parID;
		PCopy(mfRecPtr->transferToExisting.name,spec.name);
	}
	else
	{
		mfRecPtr->transferToExisting.vRefNum = mfRecPtr->transferToExisting.parID = mfRecPtr->transferToExisting.name[0] = 0;
	}
		
	//Grab the addresses in the to:, cc:, or bcc: headers
	for (h=0;!err && h<sizeof(heads)/sizeof(short);h++)
		err = GatherAddresses(messH, heads[h], &addresses, true);
	
	//put together the popup menu for the recipients
	ANDAddressesToListOfNames(popupInfo, addresses);
	
	if (addresses) ZapHandle(addresses);
	
	//if there was an error building the recipient list, zero it out, but continue.
	if (err != noErr)	
		if (popupInfo->listOfNames && popupInfo->listOfNames != IGNORE_RECIPIENTS) 
			FlushRNQ(popupInfo);
	
	//Grab the address in the from: header
	err = GatherAddresses(messH, FROM_HEAD, &addresses, false);
	if (err || !addresses || !*addresses || !**addresses || (GetHandleSize(addresses)==0))
		mfRecPtr->fromText[0] = 0;
	else 
	{
		Str255 scratch;
		
		PCopy(mfRecPtr->fromText, *addresses);
		
		//grab the real name of the sender, while we're at it.
		if (addresses) ZapHandle(addresses);
		if (GatherAddresses(messH, FROM_HEAD, &addresses, true)==noErr)
		{
			if (addresses && !*addresses && (GetHandleSize(addresses)!=0))
			{
				PCopy(scratch, *addresses);
				BeautifyFrom(scratch);
				if (!StringSame(mfRecPtr->fromText, scratch)) PCopy(mfRecPtr->fromRealName, scratch);
			}
		}
	}
	
	if (addresses) ZapHandle(addresses);


	//Grab the subject
	SuckHeaderText(messH, mfRecPtr->subjectText, sizeof(mfRecPtr->subjectText), SUBJ_HEAD);
	TrimRe(mfRecPtr->subjectText, true);
	TrimInitialWhite(mfRecPtr->subjectText);
	TrimWhite(mfRecPtr->subjectText);
	
	//set the default match and action.
	if (mfRecPtr->fromText[0]) mfRecPtr->match = MF_FROM_RADIO;
	else if (mfRecPtr->anyRText[0]) mfRecPtr->match = MF_ANYR_RADIO;
	else mfRecPtr->match = MF_SUBJECT_RADIO;
	
	mfRecPtr->action = MF_TRANSFERNEW_RADIO;
	
	return (err);
}

/**********************************************************************
 * SetUpFilterFromMbox - set up looking at the selected message(s)
 **********************************************************************/
OSErr SetUpFilterFromMbox(MyWindowPtr win, MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo)
{
	OSErr err = noErr;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	MyWindowPtr messWin;
 	short sumNum;
	short from = 0;
	short to = (*tocH)->count-1;
	MakeFilterRec mfRec;
	Boolean sawDifferentSubject = false;
	Boolean sawDifferentFrom = false;
	Boolean sawDifferentAnyR = false;
	
	//Initialize the make filter record
	WriteZero(&mfRec,sizeof(MakeFilterRec));
	 
	for (sumNum=from;!err && sumNum<=to;sumNum++)
	{
		MiniEvents(); if (CommandPeriod) break;
		if ((*tocH)->sums[sumNum].selected)
		{
			//Fill in the mfRec for each selected message, and save matching stuff in the mfRec we were passed.
			if (messWin = GetAMessage(tocH,sumNum,nil,nil,False))
			{
				WindowPtr	messWinWP = GetMyWindowWindowPtr (messWin);
				if ((**tocH).which==OUT) SetUpFilterFromComp(messWin, &mfRec, popupInfo);
				else SetUpFilterFromMess(messWin, &mfRec, popupInfo);
				
				//transfer pertinent information from this message to the mfRec we're building.
				
				//subject:
				if ((mfRecPtr->subjectText[0] == 0) && !sawDifferentSubject)//nothing in the from field yet.
					PCopy(mfRecPtr->subjectText, mfRec.subjectText);		//copy the from we see.
				else if (!sawDifferentSubject)								//something's already in the from field, but it's matched so far.
					if (!StringSame(mfRecPtr->subjectText, mfRec.subjectText)) 
					{
						sawDifferentSubject = true;
						mfRecPtr->subjectText[0] = 0;
					}
				
				//from:
				if ((mfRecPtr->fromText[0] == 0) && !sawDifferentFrom)		//nothing in the from field yet.
				{
					PCopy(mfRecPtr->fromText, mfRec.fromText);				//copy the from we see.
				}
				else if (!sawDifferentFrom)									//something's already in the from field, but it's matched so far.
				{
					//The from field always contains the short address.
					if (!StringSame(mfRecPtr->fromText, mfRec.fromText)) 
					{
						sawDifferentFrom = true;
						mfRecPtr->fromText[0] = 0;
					}
				}
				
				//copy over the from real name if we don't yet have one.
				if (!mfRecPtr->fromRealName[0] && mfRec.fromRealName[0]) PCopy(mfRecPtr->fromRealName,mfRec.fromRealName);
				
				//copy data about enclosing mailbox if we haven't yet.
				if ((mfRecPtr->transferToExisting.name[0] == 0) && ((**tocH).which!=IN) && ((**tocH).which!=OUT))
				{
					mfRecPtr->transferToExisting.vRefNum = mfRec.transferToExisting.vRefNum;
					mfRecPtr->transferToExisting.parID = mfRec.transferToExisting.parID;
					PCopy(mfRecPtr->transferToExisting.name,mfRec.transferToExisting.name);				
				}
				
				//Cleanup
				if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
			}
			else return (err=WarnUser(MF_SCAN_ERROR,errScanningMbox));
		}
	}
	
	//The message we're building a filter for will all be in the same mailbox.
	mfRecPtr->incoming = mfRec.incoming;
	mfRecPtr->outgoing = mfRec.outgoing;
	mfRecPtr->manual = mfRec.manual;

	//Depending on what field has data in it, choose the default match condition.
	mfRecPtr->action = mfRecPtr->transferToExisting.name[0] == 0 ? MF_TRANSFERNEW_RADIO : MF_TRANSFEREXISTING_RADIO;

	if (mfRecPtr->fromText[0]) mfRecPtr->match = MF_FROM_RADIO;
	else if (popupInfo->listOfNames && (popupInfo->listOfNames != IGNORE_RECIPIENTS)) mfRecPtr->match = MF_ANYR_RADIO;
	else if (mfRecPtr->subjectText[0]) mfRecPtr->match = MF_SUBJECT_RADIO;
	else err = WarnUser(MF_NO_COMMON_ERROR,errNoCommonElements);
		
	return (err);
}

/**********************************************************************
 * GatherAddresses - Grab the addresses out of a header.
 **********************************************************************/
OSErr GatherAddresses(MessHandle messH, HeaderEnum header, Handle *dest, Boolean wantComments)
{
	OSErr err=noErr;
	Uhandle text=nil;
	Handle littlelist;
	HeadSpec hs;
	
	if (!*dest) *dest = NuHandle(0);
	if (!*dest) err = MemError();
	
	if (!err && CompHeadFind(messH,header,&hs))
	{
		if (!(err=CompHeadGetText(TheBody,&hs,&text)))
		{
			err=SuckAddresses(&littlelist,text,wantComments,false,false,nil);
			if (littlelist)
			{
			 	if (**littlelist)
				{
					long size=GetHandleSize_(*dest);
					if (size) SetHandleBig_(*dest,size-1);	/* strip final terminator */
					LDRef(littlelist);
					err=HandAndHand(littlelist,*dest);
					UL(littlelist);
				}
				ZapHandle(littlelist);
				ZapHandle(text);
			}
		}
	}
	return (err);
}

/**********************************************************************
 * SetProperDefaultFolder - Set the mfRecPtr to point at the right
 *	default folder for new mailboxes.
 **********************************************************************/
void SetProperDefaultFolder(MakeFilterRecPtr mfRecPtr, Boolean warnUser)
{
	short defaultFolder;
		
	if (mfRecPtr->match==MF_SUBJECT_RADIO) defaultFolder = PREF_MF_SUBJECT_DEFAULT_FOLDER;
	else if (mfRecPtr->match==MF_FROM_RADIO) defaultFolder = PREF_MF_FROM_DEFAULT_FOLDER;
	else if (mfRecPtr->match==MF_ANYR_RADIO) defaultFolder = PREF_MF_ANYR_DEFAULT_FOLDER;
	
	GetPref(mfRecPtr->defaultText, defaultFolder);
	if (!mfRecPtr->defaultText[0])	//no default folder specified.  Use mail folder.
	{
		mfRecPtr->transferToNew.vRefNum = MailRoot.vRef;
		mfRecPtr->transferToNew.parID = MailRoot.dirId;
		GetRString(mfRecPtr->defaultText,MAIL_FOLDER_NAME);
	}
	else
	{
		//see if the default folder exists.
		if (!FileSpecByName(&mfRecPtr->transferToNew, mfRecPtr->defaultText));
		else	//it doesn't.  Set the default folder to the Mail Folder.
		{
			// warn the user the default folder doesn't exist, unless we were told to shut up about it.
			if (warnUser) WarnUser(MF_DEFAULT_FOLDER_NOT_FOUND,0);
			mfRecPtr->transferToNew.vRefNum = MailRoot.vRef;
			mfRecPtr->transferToNew.parID = MailRoot.dirId;
			GetRString(mfRecPtr->defaultText,MAIL_FOLDER_NAME);
		}
	}
}

/**********************************************************************
 * CreateTheFilter - Part of MakeFilter:
 *	actually create the filter, and display the filters window if the 
 *	user hit "Add Details". 
 **********************************************************************/
OSErr CreateTheFilter(MakeFilterRecPtr mfRecPtr, Boolean details)
{
	OSErr err = noErr;
	Str31 scratch,head;
	Str255 name;
	Str255 s;
	FActionHandle fa;
	FilterRecord fr;
	FActionProc *fap;
	Boolean folder = false;
				
	if (!Filters || !*Filters) err = RegenerateFilters();
	if (err == noErr)
	{
		FRInit(&fr);
		fr.fu.id = FilterNewId();
		fr.fu.lastMatch = GMTDateTime();
		
		// Set when this new filter is applied.
		fr.incoming = mfRecPtr->incoming;
		fr.outgoing = mfRecPtr->outgoing;
		fr.manual = mfRecPtr->manual;
		
		// Set up the match terms of the new filter.
		fr.terms[0].verb = mbmContains;
		if (mfRecPtr->match==MF_SUBJECT_RADIO)
		{
			GetRString(fr.terms[0].header,HEADER_LABEL_STRN+SUBJ_HEAD);
			PCopyTrim(fr.terms[0].value, mfRecPtr->subjectText, sizeof(fr.terms[0].value));
		}
		else if (mfRecPtr->match==MF_FROM_RADIO) 
		{
			GetRString(fr.terms[0].header,HEADER_LABEL_STRN+FROM_HEAD);
			PCopyTrim(fr.terms[0].value, mfRecPtr->fromText, sizeof(fr.terms[0].value));
		}
		else if (mfRecPtr->match==MF_ANYR_RADIO) 
		{
			GetRString(fr.terms[0].header, FILTER_ADDRESSEE);
			PCopyTrim(fr.terms[0].value, mfRecPtr->anyRText, sizeof(fr.terms[0].value));
		}
		
		// set up the action
		if (!(fa=NewZH(FAction)))
		{
			err = MemError();
		}
		else
		{
			(*fa)->action = flkTransfer;
			fap = FATable(flkTransfer);
			
			if (mfRecPtr->action == MF_TRANSFERNEW_RADIO)
			{
				if (Box2Path(&(mfRecPtr->transferToNew),s)==noErr) PCopy(mfRecPtr->transferToNew.name,s);
			 	err = (*fap)(faeRead,fa,nil,mfRecPtr->transferToNew.name);
			}
			else 
			{
				if (Box2Path(&(mfRecPtr->transferToExisting),s)==noErr) PCopy(mfRecPtr->transferToExisting.name,s);
				err = (*fap)(faeRead,fa,nil,mfRecPtr->transferToExisting.name);
			}
			
			if (!err)
			{
				LL_Queue(fr.actions,fa,(FActionHandle));
				fa = nil;
					
				// build the name
				if (*fr.terms[0].header || *fr.terms[0].value)
				{
					PSCopy(head,fr.terms[0].header);
					if (head[*head]==':') --*head;
					utl_PlugParams(GetRString(scratch,FILTER_NAME),name,
						head,fr.terms[0].value,nil,nil);
					PSCopy(fr.name,name);
				}
				else GetRString(fr.name,UNTITLED);
				
				Transmogrify(fr.terms[0].header,FiltMetaEnglishStrn,fr.terms[0].header,FiltMetaLocalStrn);
				
				/*
				 * save the actions
				 */
				
				// study it
				StudyFilter(&fr);

				// save it
				if (!(err = AppendFilter(&fr, Filters)))
					if (!(err = SaveFilters())) 
						RefreshFiltersWindow(fr, details);
			}
		}
	}
		
	return(err?WarnUser(MF_SAVE_FILTER_ERROR,errCantSaveFilter):noErr);
}


/************************************************************************
 * FileSpecByName - find a file/folder in the Mail Folder.
 ************************************************************************/
OSErr FileSpecByName(FSSpecPtr spec,PStr name)
{
	MenuHandle mh = GetMHandle(MAILBOX_MENU);
	OSErr err = noErr;
	
	//if this is the mail folder, no point in searching the mailbox tree.
	if (EqualStrRes(name, MAIL_FOLDER_NAME))
	{
		GetRString(spec->name, MAIL_FOLDER_NAME);
		spec->vRefNum = MailRoot.vRef;
		spec->parID = MailRoot.dirId;
	}
	else
	{
		err = FileSpecByNameInMenu(mh,spec,name);
	}
	
	return (err);
}

/**********************************************************************
 * FileSpecByNameInMenu - search a given menu for a file
 **********************************************************************/
OSErr FileSpecByNameInMenu(MenuHandle mh,FSSpecPtr spec,PStr name)
{
	short item;
	short n;
	short sub;
	MenuHandle subMH;
	OSErr err = fnfErr;
	
	if (item=FindFileByNameIn1Menu(mh,name))
	{
		//If this item has a submenu, return the vRefNum and parID of the submenu's folder
		if (HasSubmenu(mh,item))
		{
			MenuID2VD(SubmenuId(mh,item),&spec->vRefNum,&spec->parID);
		}
		else	//otherwise, return the vRefNum and parID of the folder it lives in
		{
			MenuID2VD(GetMenuID(mh),&spec->vRefNum,&spec->parID);
		}
		return(noErr);
	}
	
	n = CountMenuItems(mh);
	for (item=1;item<=n;item++)
		if (HasSubmenu(mh,item))
		{
			sub = SubmenuId(mh,item);
			if (subMH=GetMHandle(sub))
			{
				err = FileSpecByNameInMenu(subMH,spec,name);
				if (!err) break;
				if (err!=fnfErr) break;
			}
		}	
	return(err);
}

/**********************************************************************
 * FindFileByNameIn1Menu - find a file in a single menu
 **********************************************************************/
short FindFileByNameIn1Menu(MenuHandle mh, PStr name)
{
	Str63 itemTitle;
	short i;
	short n = CountMenuItems(mh);
	short res;
	Boolean root = GetMenuID(mh) == MAILBOX_MENU;
  
	for (i=(root?1:3);i<=n;i++)
		if (!root || i!=MAILBOX_BAR1_ITEM && i!=MAILBOX_NEW_ITEM && i!=MAILBOX_OTHER_ITEM)
		{
			if (root) MailboxMenuFile(MAILBOX_MENU,i,itemTitle);
			else MyGetItem(mh,i,itemTitle);
			res = StringComp(name,itemTitle);
			if (!res) return(i); // found it!
		}
	return(0);
}

/************************************************************************
 * ChooseMailboxFolder - get the the name of a folder from the mbox menus
 ************************************************************************/
Boolean ChooseMailboxFolder(short menu, short msg, UPtr name)
{
	short mId;
	WindowPtr theWindow;
	EventRecord event;
	MyWindowPtr	dgPtrWin;
	DialogPtr dgPtr;
	Boolean rslt = False;
	Str255 msgStr;
	MenuHandle mh;
	
	PushGWorld();
	
	/*
	 * put up message and disable all but transfer
	 */
	for (mId=APPLE_MENU;mId<MENU_LIMIT;mId++) DisableItem(GetMHandle(mId),0);
	DisableItem(GetMHandle(WINDOW_MENU),0);
	EnableItem(GetMHandle(menu),0);
	ChangeMenuForFolderSelect(menu);
	DrawMenuBar();
	ParamText(GetRString(msgStr,msg),"","","");
	dgPtrWin = GetNewMyDialog (XFER_MENU_DLOG,nil,nil,InFront);
	dgPtr = GetMyWindowDialogPtr (dgPtrWin);
//	dgPtr = GetNewDialog(XFER_MENU_DLOG,nil,InFront);
	DrawDialog(dgPtr);
	SetMyCursor(arrowCursor); SFWTC = True;
	
	/*
	 * now, let the user choose something:
	 */
	for (;;)
	{
		if (WNE(mDownMask|keyDownMask|updateMask,&event,REAL_BIG))
		{
			if (event.what==updateEvt) MiniMainLoop(&event);
			else break;
		}
	}
	if (event.what==mouseDown)
	{
		if (inMenuBar == FindWindow_(event.where,&theWindow))
		{
			long mSelect = MenuSelect(event.where);
			short mnu, itm;
			
			if (!mSelect) mSelect = MenuChoice();
			
			mnu = (mSelect>>16)&0xffff;
			itm = mSelect & 0xffff;
			if (mnu==menu || IsMailboxSubmenu(mnu))
			{
				rslt = false;
				mh = GetMHandle(mnu);
				if (mnu == menu && itm == MAILBOX_NEW_ITEM)
				{
				 	GetRString(name, MAIL_FOLDER_NAME);
				 	rslt = true;
				}
				else if (itm && !HasSubmenu(mh,itm) && IsEnabled(mnu,itm)) 
				{
					GetMenuTitle(mh,name);
					rslt = true;
				}
			}
			HiliteMenu(0);
		}
	}
	DisposDialog_(dgPtr);
	BuildBoxMenus();
	MBTickle(nil,nil);
	EnableMenus(FrontWindow_(),False);
	PopGWorld();
	return(rslt);
}

/************************************************************************
 * SuggestAMailboxName - suggest a mailbox name.
 *
 *	If the subject is being matched, suggest the first 31 chars of the subject.
 *	if the from field is matched, suggest the real name of the sender, if there is one.
 *	If anyR is being matched, suggest the recipient in the anyR field.
 ************************************************************************/
UPtr SuggestAMailboxName(MakeFilterRecPtr mfRecPtr, RecPopupPtr popupInfo)
{	
	UPtr spot;
	Str255 suggestedName;
	
	suggestedName[0] = 0;
	
	//the suggested name will be from the Subject, From, or AnyR fields.
	if (mfRecPtr->match==MF_SUBJECT_RADIO) 
	{
		PCopy(suggestedName, mfRecPtr->subjectText);	
	}
	else if (mfRecPtr->match==MF_FROM_RADIO)
	{
		if (mfRecPtr->fromRealName[0]) PCopy(suggestedName, mfRecPtr->fromRealName);	//use the real name, if available
		else PCopy(suggestedName, mfRecPtr->fromText);
	}
	else if (mfRecPtr->match==MF_ANYR_RADIO) 
	{
		short count;
		RealNameQHandle rnq = popupInfo->listOfNames;
		
		//grab the real name from the list of names described by the popup menu.
		if (rnq && rnq != IGNORE_RECIPIENTS && (*rnq)->realName)
		{
			for (count = 0; (count < (mfRecPtr->anyRSelection - 1)) && rnq; count++) rnq = (*rnq)->next;			
			if (rnq)
			{
				PCopy(suggestedName, (*rnq)->realName);
				BeautifyFrom(suggestedName);
			}
			else GetRString(suggestedName,UNTITLED_MAILBOX);			
		}		
	}
	
	//Clean up the suggested name.
	TrimRe(suggestedName, false);
	TrimInitialWhite(suggestedName);
	TrimWhite(suggestedName);;
	for (spot=suggestedName+1;spot<=suggestedName+suggestedName[0];spot++)
		if (*spot==':') *spot = '-';
	
	//And suggest the first 27 characters of the cleaned up name.
	PCopyTrim(mfRecPtr->transferToNew.name, suggestedName, 27);
	
	return(mfRecPtr->transferToNew.name);
}

/************************************************************************
 * AnyRPopup - pop up the menu for choosing a recipient.  Return the
 *	item number of what's chosen.
 ************************************************************************/
short AnyRPopup(DialogPtr dlog, short top, short left, MenuHandle mh)
{
	MyWindowPtr	dlogWin = GetDialogMyWindowPtr (dlog);
	Str255 value;
	short item;
	long sel;
	Point pt;
	GrafPtr origPort;
		
	pt.h = left; pt.v = top; 
	
	GetPort(&origPort);
	SetPort(GetDialogPort(dlog));
	LocalToGlobal(&pt);
	SetPort(origPort);

	value[0]=0;
	GetPeteDItemText (dlogWin, MF_ANYR_EDIT, value);
	
	if (*value && !(item=FindItemByName(mh,value)))
	{
		if (!(item=FindItemByName(mh,"\p-"))) 
		{
			AppendMenu(mh,"\p-");
			DisableItem(mh,CountMenuItems(mh));
			MyAppendMenu(mh,value);
		}
		else SetMenuItemText(mh, CountMenuItems(mh), value);

		item = CountMenuItems(mh);
	}
	else if (*value==0) item = 0;
	sel = AFPopUpMenuSelect(mh,pt.v,pt.h,item);
	if (sel&0xffff && sel&0xff && (sel&0xff)!=item)
	{
		MyGetItem(mh,item=sel&0xff,value);
		//make sure the anyR text field gets set to the actual address, not the real name.
		
		SetPeteDItemText (dlogWin, MF_ANYR_EDIT, value);
	}
	
	return(item);
}

/**********************************************************************
 * ANDAddressesToListOfNames - add the addresses from handle to the
 *	list of names we're maintaining.  We'll turn it into a popup
 *	menu later, and use the list for suggesting names.
 **********************************************************************/
OSErr ANDAddressesToListOfNames(RecPopupPtr popupInfo, UHandle addresses)
{
	OSErr err = noErr;
	unsigned char* scan = 0;
	long size = GetHandleSize(addresses);
	Str255 scratch, temp;
	RealNameQHandle nextRNQ, rnq = popupInfo->listOfNames;
	
	//if there are no addresses, kill the menu.  There are no common addresses.
	if (!addresses || !*addresses || !**addresses) err = errNoCommonAddresses;
	else
	{
		HLock(addresses);
		if (rnq==0)	//nothing in the list yet.  Add all the addresses.
		{
			scan = *addresses;
			while (scan < (*addresses + size))
			{
				if (rnq = NuHandleClear(sizeof(RealNameQ)))
				{
					PCopy((*rnq)->realName, scan);
					LL_Queue(popupInfo->listOfNames,rnq,(RealNameQHandle));
				}
				else
				{
					err = MemError();
					break;
				}	
				scan = scan + *scan + 1;
				while (*scan == 0 && (scan < (*addresses + size))) scan++;
			}	
		}
		else	// there are some address in the list already.  Delete the ones that aren't in addresses.
		{	
			rnq = popupInfo->listOfNames;
			while (rnq && rnq != IGNORE_RECIPIENTS)
			{			
				// compare the short addresses.
				LDRef(rnq);
				ShortAddr(temp, (*rnq)->realName);
				UL(rnq);
				
				nextRNQ = (*rnq)->next;
				
				scan = *addresses;
				while (scan < (*addresses + size))
				{				
					// the addresses are the same if the short addresses turn out to be equal
					ShortAddr(scratch, scan);
					// if the short address are equivalent, save the longer of the two addresses to the list.
					if (StringSame(temp, scratch)) 
					{
						if (*scan > *((*rnq)->realName))
							PCopy((*rnq)->realName, scan);
						break;
					}
					else
					{
						scan = scan + *scan + 1;
						while (*scan == 0 && (scan < (*addresses + size))) scan++;
					}
				}
				
				if (scan >= (*addresses + size))	// reached the end of the address handle, and didn't see it in our list?
				{
					LL_Remove(popupInfo->listOfNames,rnq,(RealNameQHandle));
					if (!popupInfo->listOfNames) popupInfo->listOfNames = IGNORE_RECIPIENTS;	//give up.
				}
				//check the next item in the real name list.
				rnq = nextRNQ;
			}
		}
			
		HUnlock(addresses);
	}
	
	return (err);
}


/**********************************************************************
 * BuildAnyRPopupMenu - build the Any Recipient popup.
 **********************************************************************/
OSErr BuildAnyRPopupMenu(RecPopupPtr popupInfo)
{
	OSErr err = noErr;
	RealNameQHandle rnq = popupInfo->listOfNames;
	Str255 scratch;
	
	if (rnq && rnq != IGNORE_RECIPIENTS)
	{
		popupInfo->menu = NewMenu(MF_ANYR_POPUP_MENU, "");
		if (popupInfo->menu)
		{
			while (rnq)
			{			
				LDRef(rnq);
				ShortAddr(scratch, (*rnq)->realName);
				UL(rnq);
				AppendMenu(popupInfo->menu, scratch);	
				rnq = (*rnq)->next;
			}
		}
	}	
	return (err);
}


/**********************************************************************
 * FlushRNQ - flush the real name queue.
 **********************************************************************/
void FlushRNQ(RecPopupPtr popupInfo)
{
	RealNameQHandle rnq, nextRNQ;
		
	if (rnq = popupInfo->listOfNames)
	{
		if (rnq == IGNORE_RECIPIENTS) rnq = 0;
		
		while (rnq)
		{			
			nextRNQ = (*rnq)->next;
			ZapHandle(rnq);
			rnq = nextRNQ;
		}
	}
	popupInfo->listOfNames = 0;
}

/**********************************************************************
 * ChangeMenuForFolderSelect - change New... to This Mailbox, and
 *	disable everything else .  BuildBoxMenus() reverses this process
 **********************************************************************/
void ChangeMenuForFolderSelect(short menu)
{
	short menuCount, item, subID;
	MenuHandle mh = GetMHandle(menu);
	
	//This only works on the mailbox and transfer menus
	if (mh && (menu == MAILBOX_MENU || menu == TRANSFER_MENU))
	{
		EnableIf(mh,MAILBOX_IN_ITEM,false);
		EnableIf(mh,MAILBOX_OUT_ITEM,false);
		EnableIf(mh,MAILBOX_JUNK_ITEM,false);
		EnableIf(mh,MAILBOX_TRASH_ITEM,false);
		
		SetItemR(mh, MAILBOX_NEW_ITEM, CHOOSE_MBOX_THIS_FOLDER);	//replace the New... item with "This Mailbox"
		SetMenuItemText(mh, MAILBOX_OTHER_ITEM, "\p-");						//Change Other ... into a divider line
		
		//And disable all mailbox entries in the menu
		menuCount = CountMenuItems(mh);
		for(item=MAILBOX_FIRST_USER_ITEM;item<=menuCount;item++)
		{
			// Do submenu with recursion
			if (subID = SubmenuId(mh,item)) TweakSubMenuForFolderSelect(subID);
			// otherwise disable the item.  It's a mailbox
			else EnableIf(mh,item,false);
		}
	}
}



/************************************************************************
 * TweakSubMenuForFolderSelect -
 ************************************************************************/
void TweakSubMenuForFolderSelect(short menu)
{
	MenuHandle mh;
	short	menuCount,subID,item;

	if (mh = GetMHandle(menu))
	{
		SetItemR(mh, 1, CHOOSE_MBOX_THIS_FOLDER);		//replace the New... item with "This Mailbox"
		SetMenuItemText(mh, 2, "\p-");							//Change Other ... into a divider line
		
		menuCount = CountMenuItems(mh);
		for(item=3;item<=menuCount;item++)
		{
			if (subID = SubmenuId(mh,item)) TweakSubMenuForFolderSelect(subID);
			else EnableIf(mh,item,false);
		}
	}
}


/************************************************************************
 * SetDefaultLocation - set the default location static text item
 ************************************************************************/
void SetDefaultLocation(DialogPtr dlog, Str255 defaultText)
{
	Str255 string;
	
	ComposeString(string, "\p%p.", defaultText);
	SetDIText(dlog, MF_DEFAULT, string);
}
