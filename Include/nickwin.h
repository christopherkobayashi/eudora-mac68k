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

#ifndef NICKWIN_H
#define NICKWIN_H

#include "listview.h"
#include "nickmng.h"
#include "mbwin.h"

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * handling the alias panel
 **********************************************************************/

#define	kNickDragType									'Nick'
#define	kNickTabType									'Nick'
#define	kAddressBookUndefined					(-1)
#define	kNickUndefined								(-1)
#define	kAdddressBookRootID						((kAddressBookUndefined << 16) | kNickUndefined)

#define	GetAddressBook(aVLNodeID)			((aVLNodeID) >> 16)
#define	GetNickname(aVLNodeID)				((aVLNodeID) & 0xFFFF)

#define	ValidAddressBook(aShort)			((aShort) != kAddressBookUndefined)
#define	ValidNickname(aShort)					((aShort) != kNickUndefined)

// Focus constants since the toolbox focus routines are really a sham
typedef enum {
	focusNoChange,
	focusNickList,
	focusNickField,
	focusTabControl,
	focusNickTabAdvance,
	focusNickTabReverse
} NickFocusType;


//	special node ID's
enum	{
	kNicknameFolder		 = -2,
	kPersonalNicknames = -3
};

// Help indices
typedef enum {
	noNickHelp = 0,															//	 0
	nickHelpList,																//	 1
	nickHelpVertDivider,												//	 2
	nickHelpViewByPopup,												//	 3
	nickHelpNewNicknameButton,									//	 4
	nickHelpNewNicknameButtonDimmed,						//	 5
	nickHelpNewAddressBookButton,								//	 6
	nickHelpRemoveButton,												//	 7
	nickHelpRemoveButtonDimmed,									//	 8
	nickHelpRemoveButtonDimmedReadOnly,					//	 9
	nickHelpRemoveButtonDimmedEudoraNicknames,	//	10
	nickHelpRemoveButtonDimmedHistoryList,			//	11
	nickHelpToButton,														//	12
	nickHelpToButtonAddToTopmost,								//	13
	nickHelpToButtonDimmed,											//	14
	nickHelpCcButton,														//	15
	nickHelpCcButtonAddToTopmost,								//	16
	nickHelpCcButtonDimmed,											//	17
	nickHelpBccButton,													//	18
	nickHelpBccButtonAddToTopmost,							//	19
	nickHelpBccButtonDimmed,										//	20
	nickHelpZoomHorizontalButtonCollapsed,			//	21
	nickHelpZoomHorizontalButtonExpanded,				//	22
	nickHelpRecipientCheckbox,									//	23
	nickHelpRecipientCheckboxDimmed,						//	24
	nickHelpNickField,													//	25
	nickHelpNickFieldDimmedNothingSelected,			//	26
	nickHelpNickFieldDimmedMultipleSelection,		//	27
	nickHelpTabs,																//	28
	nickHelpTabsDimmedNothingSelected,					//	29
	nickHelpTabsDimmedMultipleSelection					//	30
} NickHelpType;


typedef enum {
	lnfNickname,
	lnfFirstLast,
	lnfLastFirst,
	lnfOtherField
} ListNameFormatType;

typedef enum {
	mabeHitOK,
	mabeHitAddDetails,
	mabeHitOther
} MakeAddressBookEntryHitType;

typedef enum {
	nickFoundNothing		= 0,
	nickFoundNickname,
	nickFoundAddresses,
	nickFoundNotes
} NickFoundType;


#define IsRecip(name) 			(BinFindItemByName(GetMHandle(NEW_TO_HIER_MENU),name) ? true : false)
#define GetCurrentName(x,y) GetName(Selected-1,x,y)
#define PutCurrentName(x,y) PutName(Selected-1,x,y)

#define	MakeNodeID(aAddressBook,aNickname)	(((aAddressBook & 0x0000FFFF) << 16) | (aNickname & 0x0000FFFF))
#define	MakeABNodeID(aAddressBook)					MakeNodeID ((aAddressBook), (kNickUndefined))

typedef struct {
	BinAddrHandle addresses;
	Handle				notes;
	OSErr					theError;
} NicknameDataRec, *NicknameDataPtr, **NicknameDataHandle;

#define	avPairUnknown					0x0001				// An attribute tag we don't know about
#define	avPairKnown						0x0002				// An attribute tag that appears on one of our tabs
#define	avPairHidden					0x0004				// An attribute tag for a hidden value
#define	avPairUnsearchable		0x0008				// An attribute tag for a value we don't wish to search

#define	avNoPairs							0x0000
#define	avAllPairs						0xFFFF
#define	avAllButHiddenPairs		(avAllPairs & ~avPairHidden)
#define	avAllSearchablePairs	(avAllPairs & ~avPairUnsearchable)

typedef struct {
	long	type;
	long	attributeOffset;
	long	attributeLength;
	long	valueOffset;
	long	valueLength;
} AttributeValueRec, *AttributeValuePtr, **AttributeValueHandle;



void GetABNick (VLNodeID nodeID, short *ab, short *nick);
Boolean GetIndexedABNick (short index, short *ab, short *nick);
Boolean GetSelectedABNick (short index, short *ab, short *nick);
PStr GetSelectedName (short index, PStr name);
PStr GetSelectedABNickName (short index, short *ab, short *nick, PStr name);
PStr GetSelectedABNickNameData (short index, short *ab, short *nick, PStr name, VLNodeInfo *data);

void OpenABWin (PStr findStr);
Boolean CalcMinTabWidthProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, short *width);
void ABClean (void);
void ABDidResize (MyWindowPtr win, Rect *oldContR);
Boolean ABClose (MyWindowPtr win);
void ABUpdate (MyWindowPtr win);
void ABActivate (MyWindowPtr win);
Boolean ABFind (MyWindowPtr win, PStr what);
NickFoundType FindNextNicknameContainingString (MyWindowPtr win, PStr what);
NickFoundType FindStringInAddressBook (short ab, short startNick, short *endNick, PStr what, Boolean caseSens);
NickFoundType FindStringInNickName (short ab, short nick, PStr what, Boolean caseSens);
Boolean FindStringInSelectedNickname (PStr what, Boolean checkNickField);
Boolean ABDirty (MyWindowPtr win);
void ABMenuEnable (MyWindowPtr win);
void ABWazooSwitch (MyWindowPtr win, Boolean show);
void ABSetKeyboardFocus (NickFocusType nickFocus);
Boolean ABClearKeyboardFocus (void);
void ABAdvanceKeyboardFocus (void);
void ABReverseKeyboardFocus (void);
Boolean ABKey (MyWindowPtr win, EventRecord *event);
void ABClick (MyWindowPtr win, EventRecord *event);
void ABCursor (Point mouse);
void ABSetControls (Boolean isActive);
short ABDetermineSelectionConditions (Boolean *canAddNickname, Boolean *canAddAddressBook, Boolean *canRemove, Boolean *anyNicksSelected, Boolean *singleNickSelected);
void ABSetRecipientButton (void);
void ABSetSyncButton (void);
Boolean SetGreyTabProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, Boolean *shdBeGrey);
void ABSelectNickname (VLNodeID nodeID);
void ABUnselectNickname (VLNodeID nodeID);
void ABHit (MyWindowPtr win, EventRecord *event, short whichControl, short part);
void ABMessageTo (short strResID, long modifiers);
void ABHelp (MyWindowPtr win,Point mouse);
Boolean ABMenu (MyWindowPtr win, int menu, int item, short modifiers);
Boolean ABSave (void);
void ABTickle (void);
void ABTickleHardEnoughToMakeYouPuke(void);
void MakeNicknameFromSelectedNicknames (void);
void ABIdle (MyWindowPtr win);
OSErr ABDragHandler (MyWindowPtr win, DragTrackingMessage which, DragReference drag);
OSErr ABAddDragFlavors (long data);
OSErr ABDoSendDragData (long data);
void ABMove (VLNodeInfo *pToInfo, Boolean copy);
OSErr ABNickDrop (DragReference drag, VLNodeInfo *targetInfo);
#ifdef VCARD
OSErr ABFileDrop (DragReference drag, VLNodeInfo *targetInfo);

typedef struct {
	Handle					addresses;
	Handle					notes;
	short						numErrors;
	VCardErrorType 	error;
	long						offset;
} NickFromVCardItemRec, *NickFromVCardItemRecPtr, **NickFromVCardItemRecHandle;

OSErr VCardToNicknameItemProc (VCardItemPtr itemPtr, NickFromVCardItemRecPtr nickFromVCardItemPtr);
Boolean VCardToNicknameErrorProc (Handle data, VCardItemPtr itemPtr, NickFromVCardItemRecPtr nickFromVCardItemPtr, VCardErrorType error, long offset);
OSErr VCardToNicknameSetValue (Handle notes, short homeTagIndex, short workTagIndex, Boolean home, Boolean work, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored);
OSErr VCardToNicknameSetValueOrOther (Handle notes, short homeTagIndex, short workTagIndex, short otherTagIndex, Boolean home, Boolean work, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored);
#endif
OSErr GetNickFlavorDragData (Handle dragData, long *offset, short *ab, short *nick, PStr nickname, Handle *addresses, Handle *notes);
OSErr	BuildDragData (SendDragDataInfo	*pSendData, Handle *dragData);
OSErr BuildNicknameDragData (SendDragDataInfo	*pSendData, AccuPtr drag, short ab, short nick);
OSErr BuildAddressBookDragData (SendDragDataInfo	*pSendData, AccuPtr drag, short ab);
OSErr Build822FlavorDragData (AccuPtr drag, short ab, short nick);
OSErr BuildNickFlavorDragData (AccuPtr drag, short ab, short nick);
OSErr BuildTextFlavorDragData (AccuPtr drag, short ab, short nick);
#ifdef VCARD
OSErr BuildSpecFlavorDragData (SendDragDataInfo	*pSendData, AccuPtr drag, short ab, short nick);
#endif
Boolean DropNicknameOntoAddressBook (VLNodeID **movedList, VLNodeInfo *targetInfo, short ab, short nick, Boolean copy);
Boolean DropAddressBook (VLNodeID **movedList, VLNodeInfo *targetInfo, short ab, Boolean copy);
short MoveNickname (short ab, short nick, short targetAB, Boolean copy);
void DoViewByMenu (void);
void DoNewNickname (long modifiers);
void DoNewAddressBook (void);
void DoHorizontalZoom (MyWindowPtr win);
void DoRecipientList (ControlHandle theControl);
void DoDoNotSync (ControlHandle theControl);
Boolean ABNickLVSelect (VLNodeID nodeID, Boolean addToSelection);
Boolean ABRename (short ab, short nick, StringPtr newName);
void ABRemove(void);
Boolean ABMaybeRenameNickname (MyWindowPtr win);
Boolean ABDisplayNicknameInTab (short ab, short nick);
OSErr PopulateTabs (ControlHandle tabControl, short ab, short nick);
void ReplaceNicknameData (ControlHandle tabControl, short ab, short nick);
Boolean ReplaceNicknameDataProc (TabObjectPtr objectPtr, NicknameDataPtr nickDataPtr);
void ReplaceNicknameText (TabObjectPtr objectPtr, NicknameDataPtr nickDataPtr, Handle text);
void AddNicknameListItems (ViewListPtr pView, VLNodeID nodeID);
OSErr AddChildren (ViewListPtr pView, short ab);
PStr GetListNameBasedOnTag (PStr tag, short ab, short nick, Str31 name);
void SortAddressBook (short ab);
void GetSortTag (void);
int NickCompare (short *n1, short *n2);
void NickSwap (short *n1, short *n2);
int FieldCompare (short *n1, short *n2);
int EmailCompare (short *n1, short *n2);
void PositionPushButtons (MyWindowPtr win, short hTab1, short hTab2, short vTab5);
void BuildViewByMenu (ControlHandle tabControl, ControlHandle viewByControl);
long AddressBookLVCallBack (ViewListPtr pView, VLCallbackMessage message, long data);
void DoRenameCallback (PStr newName);
Boolean RemoveAddressBook (short ab);
Boolean RemoveNickname (short ab, short nick);
UPtr ParseAttributeValuePair (UPtr start, long length, UPtr *attribute, long *attributeLength, UPtr *value, long *valueLength);
UPtr ParseAttributeValuePairHandle (UPtr start, long length, Handle *hAttribute, Handle *hValue);
UPtr ParseAttributeValuePairStr (UPtr start, long length, PStr attributeStr, PStr valueStr);
OSErr AddAttributeValuePair (Handle notes, PStr attribute, Ptr value, long length);
OSErr AppendLeftoverText (AccuPtr a, UPtr from, UPtr to);
AttributeValueHandle ParseAllAttributeValuePairs (Handle notes, Handle *leftovers, long requestedTypes, long unrequestedTypesToBePutInLeftovers);
short ABSetIcon (short ab, short nick);
OSErr GetPhotoSpec (FSSpec *spec, short ab, short nick, Boolean *alreadyExists);

#ifdef VCARD
NickReplaceEnum AskNickname(Handle addresses, FSSpec *vcardSpec, short ab);
NickReplaceEnum DoNicknameDialog(Handle addresses, FSSpec *vcardSpec, short ab, PStr nickname, PStr fullName);
#else
NickReplaceEnum AskNickname(Handle addresses, short ab);
NickReplaceEnum DoNicknameDialog(Handle addresses, short ab, PStr nickname, PStr fullName);
#endif
NickReplaceEnum DoGroupNicknameDialog(Handle addresses, short ab, PStr nickname, PStr fullName);
Handle CreateSimpleNotes (PStr fullName, PStr firstName, PStr lastName);
OSErr CreatePeteUserPaneWithQDTextAndScanning (DialogPtr theDialog, short item, uLong flags, PETEDocInitInfoPtr initInfo, Boolean noWrap, NickScanType nickScan, short aliasIndex);
void PostMakeNicknameDialog (short ab, short nick, MakeAddressBookEntryHitType mabeHit);
#ifdef VCARD
void NewNick(Handle addresses,FSSpec *vcardSpec,short which);
#else
void NewNick(Handle addresses,short which);
#endif
short NewNickLow(Handle addresses,Handle notes,short which,Str63 name,Boolean makeRecip,NickReplaceEnum nre,Boolean doSave);
short AddNickname(Handle addresses,Handle notes,short which,Str63 name,Boolean makeRecip,NickReplaceEnum nre,Boolean doSave);
Boolean CanMakeNick(void);
Boolean	NickUserFieldExists(PStr theField);
Boolean	IsNicknameOnRecipList(short which,short index);
Boolean IsNicknamePrivate (short ab, short nick);
void AESaveCurrentAlias(short which,short index);
void	ForceSelectedAliasUpdate(short which,short index,Boolean didDirty);
OSErr NickGetDataFromField(UPtr theField,UPtr sViewData,short	which,short	nickNum,Boolean doingDrag,Boolean doingAE,Boolean *nickNameEmpty);
short NickMakeNewFile(Str63 name);
Boolean AERemoveNick(short index,short which);
Boolean NickWinIsOpen(void);
Boolean AliasWinIsOpen(void);
void RemoveNickFromAddressBookList (short which, short index, Boolean doSave);
void AliasWinRefresh(void);
void VanquishRecipientList (Boolean discardChanges);
void SaveExpandedAddressBookNames (void);
void NickSuggest(BinAddrHandle addresses, PStr name, PStr realName, Boolean uniq,short prefix);
void NickMemErrorAndQuit(int theStr,short theErr);
short NickRepAlert(PStr candidate);
void NickParseForInfo(Str255 input,long *height,long *width,Str255 fieldName);
void GetName (short i,short *nickNum,short *whichFile);
OSErr MyFSpMoveToTrash(const FSSpec *spec);
MyWindowPtr TopCompositionWindow(Boolean skipCurrent, Boolean dontCreate);
void AliasesFixFont(void);
short *GetNickFileArray (Boolean *needFile);
void BuildAddressBookMenu (DialogPtr theDialog, short addressBookMenuItem, short	*nickFileIndexArray, Boolean needFile, short *which);
void HitFileNickRadioButton (DialogPtr theDialog, short itemHit, short addressBookItem, short nicknameItem, short addressBookMenuItem, short recipientItem);
void HitAddressBookMenu (MyWindowPtr dgPtrWin, short addressBookMenuItem, short	*nickFileIndexArray, Boolean needFile);
NickReplaceEnum BadNickname(UPtr candidate,UPtr veteran,short which,short nick,Boolean justRename,Boolean collisionCheck,Boolean folder);
MyWindowPtr InsertTheAlias(short txeIndex,Boolean wantExpansion);
void NickWDragVDivider(Point pt);
Boolean AnyNicknamesDirty(short ab);
void ReformatClip(void);
Handle GetAliasExpansionFor822Flavor (short which, short index);
#ifdef VCARD
OSErr AutoGeneratePersonalInformation (short ab);
OSErr MakeVCardFile (FSSpec *spec, short ab, short nick);
#endif
#ifdef DEBUG
void PaintUpdateRgn (WindowPtr theWindow, short color);
#endif
void ABUnselectCurrentNickname(void);

void SortAddressBookList (void);
int abCompare (AliasDPtr adp1, AliasDPtr adp2);
void abSwap (AliasDPtr adp1, AliasDPtr adp2);
#endif

