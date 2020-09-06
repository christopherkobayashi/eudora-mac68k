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

#include <AEObjects.h>
#include <Folders.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

#include "navUtils.h"
#define FILE_NUM 118

#pragma segment navUtils

extern	Boolean DoUpdate(WindowPtr topWin,EventRecord *event);
extern	OSErr		InsertGraphicFile(MyWindowPtr win,FSSpecPtr spec);
extern	OSErr		InsertTextFile(MyWindowPtr win, FSSpecPtr spec, Boolean rich, Boolean delSP);

typedef struct {
	ZoneAndResFileRec			zoneAndResFile;
//	DialogPtr					theDialog;
//	short						defaultItem;
	Boolean						customControlsCreated;		// Safety for Nav 1.0
	Boolean						insertDefault;				//	Insert button is the default
//	Boolean						insertHit;					//	The insert button was hit
//	Boolean						defaultButtonIsInsert;
} attachInsertRec, *attachInsertRecPtr, **attachInsertRecHandle;


pascal void attachNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, attachInsertRecPtr attachInsertPtr);
pascal void updateNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, ZoneAndResFilePtr zoneAndResFile);
// OSErr HandleNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, attachInsertRecPtr attachInsertPtr);
void NegotiateCustomRectSize (Rect *customRect, short minWidth, short minHeight);
OSErr SelectNavEntry (DialogPtr navDialog, NavDialogRef context, AEDescList *fileList, attachInsertRecPtr attachInsertPtr);
OSErr AddDialogItemsToNav (short ditlResID, NavDialogRef context);
short GetCustomNavItem (NavDialogRef context, short customNavItem);
pascal Boolean InsertNavFilterProc ( AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode );

void SeeIfWeShouldPreloadNav (void)

{
	OSErr	theError;
	
	if (UseNavServices () && PrefIsSet (PREF_PRELOAD_NAV))
		if (theError = NavLoad ())
			DieWithError (NAV_COULD_NOT_LOAD_ERR, theError);	
}


Boolean UseNavServices (void)

{
	UInt32	version;
	Boolean	useNav;

	if (HaveOSX ())
		return (true);

	if (useNav = NavServicesAvailable ()) {
		version = NavLibraryVersion ();
		useNav = (version >= 0x01100000);
	}
	
	return (useNav && version);
}


void GetZoneAndResFile (ZoneAndResFilePtr zoneAndResFile)

{
	zoneAndResFile->resFile	 = CurResFile ();
}


void SetZoneAndResFile (ZoneAndResFilePtr zoneAndResFile)

{
	UseResFile (zoneAndResFile->resFile);
}


pascal Boolean InsertNavFilterProc ( AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode ) {
	OSErr err = noErr;
	NavFileOrFolderInfo *theInfo;
	FSSpec	spec;
	Boolean	wasAlias;
	FInfo	fInfo;
	
//	if it's not a file/folder, we don't want to know about it
	if ( theItem->descriptorType != 'fss ' )
		return false;

	theInfo = (NavFileOrFolderInfo *) info;
	
	if ( theInfo->isFolder )
		return true;
	fInfo.fdType = 0;
	
	err = AEGetDescData ( theItem, &spec, sizeof ( spec ));
	if ( noErr == err )
		err = ResolveAliasNoMount ( &spec, &spec, &wasAlias );
	if ( noErr == err )
		err = AFSpGetFInfo ( &spec, &spec, &fInfo );
	
	if ( err != noErr )
		return false;
		
//	We can always insert text
	if ( fInfo.fdType == 'TEXT' || fInfo.fdType == 'ttro' )
		return true;
		
	if ( HasFeature (featureStyleGraphics) && !PrefIsSet ( PREF_SEND_ENRICHED_NEW ) && IsGraphicFile ( &spec ))
		return true;
	
	return false;		// don't show the file
	}


void CompAttachNav (MyWindowPtr win, Boolean insertDefault)

{
	attachInsertRec			attachInsert;
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavEventUPP					eventUPP;
	NavObjectFilterUPP		filterUPP;
	FSSpec							spec;
	OSErr								theError,
											disposeError;
	long								count,
											i;

	Zero (theReply);
	Zero (attachInsert);
	
	attachInsert.insertDefault = insertDefault;
	
	eventUPP = NewNavEventUPP (attachNavProc);
	filterUPP = NewNavObjectFilterUPP ( InsertNavFilterProc );
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		if (HaveOSX ())
			dialogOptions.dialogOptionFlags |= kNavSupportPackages;
		GetRString (dialogOptions.windowTitle, insertDefault ? INSERT_DOCUMENT_NAV_TITLE : ATTACH_DOCUMENT_NAV_TITLE);
		GetRString (dialogOptions.actionButtonLabel, insertDefault ? INSERT_DOCUMENT_ACTION_BUTTON : ATTACH_DOCUMENT_ACTION_BUTTON);
		GetRString (dialogOptions.message, insertDefault ? INSERT_DOCUMENT_INSTRUCTIONS : ATTACH_DOCUMENT_INSTRUCTIONS);
	}
	if (!theError) {
		GetZoneAndResFile (&attachInsert.zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFile (nil, &theReply, &dialogOptions, eventUPP, nil, insertDefault ? filterUPP : nil, nil, &attachInsert);
		PopCursor();
		SetZoneAndResFile (&attachInsert.zoneAndResFile);
		if (theError)
			if (theError == fnfErr)
				WarnUser (ALIAS_TO_NOWHERE, theError);
			else if (theError != userCanceledErr)
				WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError && theReply.validRecord) {
			theError = AECountItems (&theReply.selection, &count);
			for (i = 1; !theError && i <= count; ++i) {
				theError = GetSpecFromNthDesc (&theReply.selection, i, &spec);
				if (!theError) {
				//	if (attachInsert.insertHit || attachInsert.defaultButtonIsInsert) 
					if ( insertDefault )
					{
						if (IsGraphicFile (&spec))
							InsertGraphicFile (win, &spec);
						else
							InsertTextFile (win, &spec, false, false);
					}
					else
						CompAttachSpec (win, &spec);
				}
			}
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
		
	if (filterUPP)
		DisposeNavObjectFilterUPP (filterUPP);
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);

}


//
//	Very annoying note...
//
//	Selectors are passed in the following order:
//		1. kNavCBCustomize
//		2. kNavCBCustomize
//		3. kNavCBCustomize
//		4. kNavCBSelectEntry
//		5. kNavCBSelectEntry
//		6. kNavCBStart
//
//	This is not helpful because our custom control is not created until kNavCBStart.  Therefore, we
//	can't activate our custom controls when we get the initial kNavCBSelectEntry selectors. (grr... grr...)
//
//	On top of this, the kNavCBSelectEntry passes a nil dialog pointer!!  Yikes!  C'mon Nav Service guys...
//

pascal void attachNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, attachInsertRecPtr attachInsertPtr)

{
	AEDescList				fileList;
	ZoneAndResFileRec	navZoneAndResFile;
	DialogPtr					theDialog;
	OSErr							theError;
	
	theError = noErr;
	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (&attachInsertPtr->zoneAndResFile);
	theDialog = callBackParms->window ? GetDialogFromWindow (callBackParms->window) : nil;
	switch (callBackSelector) {
		case kNavCBEvent:
		//	theError = HandleNavEvent (theDialog, callBackParms->eventData.eventDataParms.event, callBackParms->context, attachInsertPtr);
			break;

		case kNavCBCustomize :
		//	NegotiateCustomRectSize (&callBackParms->customRect, 400, 40);
			break;

		case kNavCBStart :
	//		attachInsertPtr->defaultItem = GetDialogDefaultItem (theDialog);
	//		theError = AddDialogItemsToNav (ATTACH_NAV_DITL, callBackParms->context);
			// Hack!! - kNavCBSelectEntry selector comes in before we start and Nav 1.0 is really unfriendly about this
	//		if (!theError) {
				attachInsertPtr->customControlsCreated = true;
				theError = NavCustomControl (callBackParms->context, kNavCtlGetSelection, &fileList);
				if (!theError)
					theError = SelectNavEntry (theDialog, callBackParms->context, &fileList, attachInsertPtr);
	//		}
			break;

		case kNavCBSelectEntry:
			//  !@#$%^ Nav Services... 	The window field of the NavCBRecPtr is nil!!!
//		theError = SelectNavEntry (callBackParms->window, callBackParms->context, (AEDescList *) callBackParms->eventData.eventDataParms.param, attachInsertPtr);
			if (attachInsertPtr->customControlsCreated)
				theError = SelectNavEntry (GetDialogFromWindow(FrontWindow()), callBackParms->context, (AEDescList *) callBackParms->eventData.eventDataParms.param, attachInsertPtr);
			break;
	}
	SetZoneAndResFile (&navZoneAndResFile);
}


pascal void updateNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, ZoneAndResFilePtr zoneAndResFile)

{
	ZoneAndResFileRec	navZoneAndResFile;
	WindowPtr			win;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (zoneAndResFile);
	switch (callBackSelector) {
		case kNavCBEvent:
			switch (callBackParms->eventData.eventDataParms.event->what) {
				case updateEvt:
					win = (WindowPtr)callBackParms->eventData.eventDataParms.event->message;
					if (IsMyWindow(win) && (GetWindowKind(win)==dialogKind))
						//	Nav Services attempts to udpate dialogs itself by redrawing all the
						//	controls which zaps the udpate rgn. Not good enough. We need to
						//	do some drawing ourself. This is a problem mainly with the
						//	Settings dialog.
						InvalContent(GetWindowMyWindowPtr(win));
					DoUpdate (win, (EventRecord *) callBackParms->eventData.eventDataParms.event);
					break;
				case nullEvent:
//#ifdef THREADING_ON	
//					if (NEED_YIELD)
//						MyYieldToAnyThread();
//#endif
					break;
			}
			break;
	}
	SetZoneAndResFile (&navZoneAndResFile);
}


#if 0
OSErr HandleNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, attachInsertRecPtr attachInsertPtr)

{
	DialogItemIndexZeroBased	foundItem,
														firstItem;
	GrafPtr										savedPort;
	WindowPtr									win;
	ControlHandle							theControl;
	Point											mouseLoc;
	OSErr											theError;
	short											partCode;
	
	theError = noErr;
	switch (event->what) {
		case mouseDown:
			partCode = FindWindow (event->where, &win);
			if (partCode == inContent && navDialog == GetDialogFromWindow (win)) {
				GetPort (&savedPort);
				SetPort (GetDialogPort(navDialog));	// Hack for Nav Services 1.0
				GetMouse (&mouseLoc);
				SetPort (savedPort);
				
				foundItem = FindDialogItem (navDialog, mouseLoc);
				if (foundItem >= 0)
					if ((theControl = GetDItemCtl (navDialog, foundItem + 1)) && !GetControlHilite(theControl))
						if (!NavCustomControl (context, kNavCtlGetFirstControlID, &firstItem))
							if (foundItem >= firstItem) {
								attachInsertPtr->insertHit = true;
								NavCustomControl (context, kNavCtlAccept, nil);
							}
			}
			break;
		case updateEvt:
			DoUpdate ((WindowPtr) event->message, event);
			break;
	}
	return (theError);
}
#endif

void NegotiateCustomRectSize (Rect *customRect, short minWidth, short minHeight)

{
	if (!customRect->bottom)
		customRect->bottom = customRect->top + minHeight;
	if (!customRect->right)
		customRect->right = customRect->left + minWidth;
}


OSErr SelectNavEntry (DialogPtr navDialog, NavDialogRef context, AEDescList *fileList, attachInsertRecPtr attachInsertPtr)

{
//	ControlHandle	theControl;
	AEKeyword			keyword;
	DescType			typeCode;
	FInfo 				info;
	FSSpec				spec;
	OSErr					theError;
	long					actualSize,
								count,
								i;
	Boolean				dimInsertButton,
								wasAlias;

	dimInsertButton = false;
	theError = AECountItems (fileList, &count);
	for (i = 1; !theError && !dimInsertButton && i <= count; ++i) {
		theError = AEGetNthPtr (fileList, i, typeFSS, &keyword, &typeCode,(Ptr) &spec, sizeof(FSSpec), &actualSize);		
		if (!theError)
			theError = ResolveAliasNoMount (&spec, &spec, &wasAlias);
		if (!theError)
			theError = AFSpGetFInfo (&spec, &spec, &info);
		if (!theError)
			if (!(info.fdType == 'TEXT' || info.fdType == 'ttro' || (HasFeature (featureStyleGraphics) && !PrefIsSet(PREF_SEND_ENRICHED_NEW) && IsGraphicFile (&spec))))
				dimInsertButton = true;
	}
	if (theError)
		dimInsertButton = true;
		
//	if (theControl = GetDItemCtl (navDialog, GetCustomNavItem (context, 1))) {
//		attachInsertPtr->defaultButtonIsInsert = attachInsertPtr->insertDefault && !dimInsertButton;
//		HiliteControl (theControl, dimInsertButton ? 255 : 0);
//		SetDialogDefaultItem (navDialog, attachInsertPtr->defaultButtonIsInsert ? GetCustomNavItem (context, 1) : attachInsertPtr->defaultItem);
//	}
	return (theError);
}


OSErr AddDialogItemsToNav (short ditlResID, NavDialogRef context)

{
	Handle	ditl;
	OSErr		theError;
	
	theError = noErr;
	ditl = GetResource ('DITL', ditlResID);

	if (!ditl) {
		if (!(theError = ResError ()))
			theError = resNotFound;
	}
	else {
		theError = NavCustomControl (context, kNavCtlAddControlList, ditl);
		ReleaseResource (ditl);
	}

	return (theError);
}


short GetCustomNavItem (NavDialogRef context, short customNavItem)

{
	DialogItemIndexZeroBased	item;

	item = 0;
	if (!NavCustomControl (context, kNavCtlGetFirstControlID, &item))
		item += customNavItem;
	return (item);
}


void DoSFOpenNav (short modifiers)

{
	static OSType				types[] = {kContainerFolderAliasType,'TEXT','ttro',kFakeAppType,STATIONERY_TYPE,'ttro','PREF','eAp1','eAp2','eAp3','eAp4','eAp5','eAp6','eAp7','eAp8','eAp9','eApA','eApB','eApC','eApD','eApE','eApF',SEARCH_FILE_TYPE,REG_FILE_TYPE,0};
	ZoneAndResFileRec		zoneAndResFile;
	ProcessSerialNumber psn;
	NavTypeListHandle		typeList;
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavEventUPP					eventUPP;
	AEDescList					fileList;
	FSSpec							spec;
	FInfo								info;
	OSErr								theError,
											disposeError;
	short								numTypes;

	Zero (theReply);
	
	theError = noErr;

	NullADList (&fileList, nil);

	numTypes = CountOSTypes (types, 0);

	typeList = NewNavTypeList (CREATOR, numTypes, types);
	
	eventUPP = NewNavEventUPP (updateNavProc);
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		// We get one hell of a nasty popup...  let's not, for now.
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags &= ~kNavAllowMultipleFiles;
		dialogOptions.dialogOptionFlags |= kNavSelectAllReadableItem;
		BlockMoveData (LMGetCurApName(), dialogOptions.clientName, /* LMGetCurApName()[0]*/ ((StringPtr) 0x0910)[0] + 1);
	}
	if (!theError) {
		GetZoneAndResFile (&zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavGetFile (nil, &theReply, &dialogOptions, eventUPP, nil, nil, typeList, &zoneAndResFile);
		PopCursor();
		SetZoneAndResFile (&zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError && theReply.validRecord) {
			theError = GetSpecFromNthDesc (&theReply.selection, 1, &spec);
			if (!theError)
				theError = AFSpGetFInfo (&spec, &spec, &info);
			if (!theError)
				if (info.fdType == SETTINGS_TYPE)
					OpenNewSettings (&spec, false, GetNagState());
				else
					SimpleAESend (CurrentPSN (&psn),
									 kCoreEventClass,kAEOpenDocuments,nil,nil /*kEAEWhenever CK*/,
									 keyDirectObject,keyEuDesc,SimpleAEList (&fileList, typeFSS, &spec, sizeof (spec), nil),
									 nil,nil);
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	ZapHandle (typeList);
	DisposeADList (&fileList, nil);
}


NavTypeListHandle NewNavTypeList (OSType applicationSignature, short numTypes, OSType typeList[])

{
	NavTypeListHandle	navTypeList;
	
	navTypeList = nil;
	
	if (numTypes > 0) {
		navTypeList = (NavTypeListHandle) NuHandle (sizeof (NavTypeList) + numTypes * sizeof (OSType));
		if (navTypeList) {
			(*navTypeList)->componentSignature = applicationSignature;
			(*navTypeList)->osTypeCount				 = numTypes;
			BlockMoveData (typeList, (*navTypeList)->osType, numTypes * sizeof(OSType));
		}
	}
	return (navTypeList);
}


OSErr GetSpecFromNthDesc (AEDescList *list, short n, FSSpec *spec)

{
	AEDesc		firstDesc;
	AEKeyword	ignoreKeyword;
	OSErr			theError,
						disposeError;
		
	theError = AEGetNthDesc (list, n, typeFSS, &ignoreKeyword, &firstDesc);
	if (!theError)
		AEGetDescData(&firstDesc,spec,sizeof(*spec));
	disposeError = AEDisposeDesc (&firstDesc);
	return (theError ? theError : disposeError);
}


Boolean GetFolderNav (char *name, short *volume, long *folder)

{
	ZoneAndResFileRec		zoneAndResFile;
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavEventUPP					eventUPP;
	NavObjectFilterUPP	objectFilterUPP;
	AEDesc							defaultFolderDesc,
											*defaultFolderDescPtr;
	FSSpec							spec;
	OSErr								theError,
											disposeError;
	Boolean					result = false;

	Zero (theReply);
	
	eventUPP				= NewNavEventUPP (updateNavProc);
	objectFilterUPP = NewNavObjectFilterUPP (getFolderObjectFilterProc);
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;
		dialogOptions.dialogOptionFlags |= kNavAllowInvisibleFiles;
		GetRString (dialogOptions.message, NAV_CHOOSE_FOLDER_MESSAGE);
		// Try to make a spec... some routines pass us bogus vRef's though, so we'll try to
		// make a daring (and, probably, temporary) recovery.
		if (theError = FSMakeFSSpec (*volume, *folder, "\p", &spec)) {
			HGetVol (name, volume, folder);
			theError = FSMakeFSSpec (*volume, *folder, "\p", &spec);
		}
		if (!theError)
			theError = AECreateDesc (typeFSS, &spec, sizeof (spec), &defaultFolderDesc);
		defaultFolderDescPtr = theError ? nil : &defaultFolderDesc;
		theError = noErr;
	}
	if (!theError) {
		GetZoneAndResFile (&zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFolder (defaultFolderDescPtr, &theReply, &dialogOptions, eventUPP, objectFilterUPP, &zoneAndResFile);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		SetZoneAndResFile (&zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError && theReply.validRecord) {
			theError = GetSpecFromNthDesc (&theReply.selection, 1, &spec);
			if (!theError) {
				*volume = spec.vRefNum;
				*folder = spec.parID;
				GetMyVolName (*volume, name);
			}
			result = true;
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
		AEDisposeDesc (&defaultFolderDesc);
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	if (objectFilterUPP)
		DisposeNavObjectFilterUPP (objectFilterUPP);
	return (result);
}

pascal Boolean getFolderObjectFilterProc (AEDesc *theItem, void *info, ZoneAndResFilePtr zoneAndResFile, NavFilterModes filterMode)

{
	AEDesc							specDescriptor;
	FSSpec							spec;
	CInfoPBRec 					hfi;
	ZoneAndResFileRec		navZoneAndResFile;
	NavFileOrFolderInfo	*theInfo;
	long								trashDirID;
	short								trashVRefNum;
	Boolean							display;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (zoneAndResFile);
	
	display = false;
	theInfo = (NavFileOrFolderInfo*) info;
	if (theItem->descriptorType == typeFSS)
		if (display = theInfo->isFolder)
			if (!theInfo->visible)
				if (!AECoerceDesc (theItem, typeFSS, &specDescriptor)) {
					AEGetDescData(&specDescriptor, &spec, sizeof(FSSpec));
					if (!FindFolder (spec.vRefNum, kTrashFolderType, false, &trashVRefNum, &trashDirID)) {
						hfi.hFileInfo.ioCompletion	= nil;
						hfi.hFileInfo.ioNamePtr			= spec.name;
						hfi.hFileInfo.ioVRefNum			= spec.vRefNum;
						hfi.hFileInfo.ioDirID				= spec.parID;
						hfi.hFileInfo.ioFDirIndex		= 0;
						if (!PBGetCatInfoSync ((CInfoPBPtr) &hfi))
							display = (hfi.dirInfo.ioDrDirID == trashDirID);
					}
				}
	SetZoneAndResFile (&navZoneAndResFile);
	return (display);
}


Boolean MakeUserFindSettingsNav (FSSpecPtr theSpec)

{
	static OSType			types[] = {'PREF',0};
	ZoneAndResFileRec	zoneAndResFile;
	NavTypeListHandle	typeList;
	NavReplyRecord		theReply;
	NavDialogOptions	dialogOptions;
	NavEventUPP				eventUPP;
	FSSpec						spec;
	OSErr							theError,
										disposeError;
	short							numTypes;
	Boolean						result = true;

	Zero (theReply);

	numTypes = CountOSTypes (types, 0);

	typeList = NewNavTypeList (CREATOR, numTypes, types);

	eventUPP = NewNavEventUPP (updateNavProc);
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		dialogOptions.dialogOptionFlags &= ~kNavAllowMultipleFiles;
		GetRString (dialogOptions.message, NAV_CHOOSE_SETTINGS);
		BlockMoveData (LMGetCurApName(), dialogOptions.clientName, /* LMGetCurApName()*/ ((StringPtr) 0x0910)[0] + 1);
	}
	if (!theError) {
		GetZoneAndResFile (&zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFile (nil, &theReply, &dialogOptions, eventUPP, nil, nil, typeList, &zoneAndResFile);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		SetZoneAndResFile (&zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError && theReply.validRecord) {
			theError = GetSpecFromNthDesc (&theReply.selection, 1, &spec);
			if (!theError) {
				Root.vRef = spec.vRefNum;
				Root.dirId = spec.parID;
				*theSpec = spec;
				result = false;
			}
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	ZapHandle (typeList);
	return (result);
}


OSErr SFPutOpenNav (FSSpecPtr spec, long creator, long type, short *refN, short ditlID, ScriptCode *script, FSSpecPtr defaultSpec,PStr windowTitle, PStr message)

{
	ZoneAndResFileRec	zoneAndResFile;
	NavReplyRecord		theReply;
	NavDialogOptions	dialogOptions;
	NavEventUPP				eventUPP;
	OSErr							theError,
										disposeError;
	AEDesc						defaultFolderDesc,
										*defaultFolderDescPtr;
	
	Zero (theReply);
	
	switch (ditlID) {
		case SAVEAS_NAV_DITL:
			eventUPP = NewNavEventUPP (saveAsNavProc);
			break;
		default:
			eventUPP = NewNavEventUPP (updateNavProc);
			break;
	}
			
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);
	if (!theError) {
		dialogOptions.dialogOptionFlags	= kNavNoTypePopup | kNavDontAddTranslateItems;
		PCopy (dialogOptions.savedFileName, spec->name);
		if (windowTitle)
			PCopy (dialogOptions.windowTitle, windowTitle);
		if (message)
			PCopy (dialogOptions.message, message);
		GetZoneAndResFile (&zoneAndResFile);
		//	Set up default folder (if any)
		if (defaultSpec)
			defaultFolderDescPtr = AECreateDesc (typeFSS, defaultSpec, sizeof (FSSpec), &defaultFolderDesc) ? nil : &defaultFolderDesc;
		else
			defaultFolderDescPtr = nil;
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavPutFile (defaultFolderDescPtr, &theReply, &dialogOptions, eventUPP, type, creator, &zoneAndResFile);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		if (defaultFolderDescPtr)
			AEDisposeDesc(defaultFolderDescPtr);	
		SetZoneAndResFile (&zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		else {
			if (theReply.validRecord) {
				*script = GetScriptManagerVariable (smKeyScript);
				theError = GetSpecFromNthDesc (&theReply.selection, 1, spec);
			}
			else
				theError = 1;
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	return (theError);
}


pascal void saveAsNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, ZoneAndResFilePtr	zoneAndResFile)

{
	ZoneAndResFileRec	navZoneAndResFile;
	DialogPtr					theDialog;
	OSErr							theError;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (zoneAndResFile);
	theDialog = callBackParms->window ? GetDialogFromWindow (callBackParms->window) : nil;
	
	theError = noErr;
	switch (callBackSelector) {
		case kNavCBEvent:
			theError = HandleSaveAsNavEvent (theDialog, callBackParms->eventData.eventDataParms.event, callBackParms->context);
			break;
		case kNavCBCustomize :
			NegotiateCustomRectSize (&callBackParms->customRect, 400, 50);
			break;
		case kNavCBStart :
			theError = AddDialogItemsToNav (SAVEAS_NAV_DITL, callBackParms->context);
			if (!theError) {
				SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavSaveAsGuessParagraphsItem), PrefIsSet(PREF_PARAGRAPHS));
				SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavSaveAsIncludeHeadersItem), !PrefIsSet(PREF_EXCLUDE_HEADERS));
#ifdef TWO
				if (Stationery == 2) {
					HideDialogItem (theDialog, GetCustomNavItem (callBackParms->context, rNavSaveAsStationeryItem));
					HideDialogItem (theDialog, GetCustomNavItem (callBackParms->context, rNavSaveAsGoToStationeryItem));
					Stationery = 0;		// (jp) Since we're hiding the items, we better make sure we don't save as stationery
				}
				else
					SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavSaveAsStationeryItem), Stationery);

				saveAsCustomHilite (theDialog, callBackParms->context);
#endif
			}
			break;
	}
	SetZoneAndResFile (&navZoneAndResFile);
}

void saveAsCustomHilite (DialogPtr navDialog, NavDialogRef context)

{
	HiliteControl (GetDItemCtl (navDialog, GetCustomNavItem (context, rNavSaveAsIncludeHeadersItem)), Stationery == 1 ? 255 : 0);
	HiliteControl (GetDItemCtl (navDialog, GetCustomNavItem (context, rNavSaveAsGuessParagraphsItem)), Stationery == 1 ? 255 : 0);
	HiliteControl (GetDItemCtl (navDialog, GetCustomNavItem (context, rNavSaveAsGoToStationeryItem)), Stationery == 1 ? 0 : 255);
}


OSErr HandleSaveAsNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context)

{
	GrafPtr			savedPort;
	WindowPtr		win;
	AEDesc			location;
	FSSpec			spec;
	Point				mouseLoc;
	OSErr				theError;
	short				partCode,
							firstItem,
							foundItem;
	
	switch (event->what) {
		case mouseDown:
			partCode = FindWindow (event->where, &win);
			if (partCode == inContent && navDialog == GetDialogFromWindow (win)) {
				GetPort (&savedPort);
				SetPort (GetDialogPort(navDialog));	// Hack for Nav Services 1.0
				GetMouse (&mouseLoc);
				SetPort (savedPort);
				
				foundItem = FindDialogItem (navDialog, mouseLoc);
				if (foundItem >= 0) {
					firstItem = GetCustomNavItem (context, 1);
					switch (foundItem - firstItem + 2) {
						case rNavSaveAsGuessParagraphsItem:
							SetDItemState (navDialog, foundItem + 1, !GetDItemState (navDialog, foundItem + 1));
							TogglePref(PREF_PARAGRAPHS);
							break;
						case rNavSaveAsIncludeHeadersItem:
							SetDItemState (navDialog, foundItem + 1, !GetDItemState (navDialog, foundItem + 1));
							TogglePref(PREF_EXCLUDE_HEADERS);
							break;
						case rNavSaveAsStationeryItem:
							Stationery = !GetDItemState (navDialog, foundItem + 1);
							SetDItemState (navDialog, foundItem + 1, Stationery);
							saveAsCustomHilite (navDialog, context);
							break;
						case rNavSaveAsGoToStationeryItem:
							theError = FSMakeFSSpec (StationVRef, StationDirId, "\p", &spec);
							if (!theError)
								theError = AECreateDesc (typeFSS, &spec, sizeof (spec), &location);
							if (!theError) {
								theError = NavCustomControl (context, kNavCtlSetLocation, &location);
								AEDisposeDesc (&location);
							}
							break;
					}
				}
			}
			break;
		case updateEvt:
			DoUpdate ((WindowPtr) event->message, event);
			break;
	}
	return (theError);
}

short CountOSTypes (OSType *types, short limit)

{
	OSType	*tPtr;
	short		numTypes;
	
	tPtr = types;
	numTypes = 0;
	while (*tPtr++)
		if (++numTypes == limit)
			return (limit);
	return (numTypes);
}


OSErr GetFileNav (OSType *types, short titleResID, Str255 prompt, short actionButtonLabelResID, Boolean allowPackages, FSSpec *spec, Boolean *good, ProcPtr objectFilterProc)

{
	ZoneAndResFileRec		zoneAndResFile;
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavTypeListHandle		typeList;
	NavEventUPP					eventUPP;
	NavObjectFilterUPP	objectFilterUPP;
	OSErr								theError,
											disposeError;
	short								numTypes;

	*good = false;
	Zero (theReply);

	numTypes = 0;
	typeList = nil;
	if (types) {
		numTypes = CountOSTypes (types, 0);
		typeList = NewNavTypeList (CREATOR, numTypes, types);
	}

	eventUPP = NewNavEventUPP (updateNavProc);
	objectFilterUPP = objectFilterProc ? NewNavObjectFilterUPP (objectFilterProc) : nil;

	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		if (titleResID)
			GetRString (dialogOptions.windowTitle, titleResID);
		if (actionButtonLabelResID)
			GetRString (dialogOptions.actionButtonLabel, actionButtonLabelResID);
		if (allowPackages && HaveOSX ())
			dialogOptions.dialogOptionFlags |= kNavSupportPackages;
		PCopy (dialogOptions.message, prompt);
		GetZoneAndResFile (&zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFile (nil, &theReply, &dialogOptions, eventUPP, nil, objectFilterUPP, typeList, &zoneAndResFile);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		SetZoneAndResFile (&zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError) {
			if (*good = theReply.validRecord)
				theError = GetSpecFromNthDesc (&theReply.selection, 1, spec);
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	if (objectFilterUPP)
		DisposeNavObjectFilterUPP (objectFilterUPP);
	ZapHandle (typeList);
	return (theError);
}


pascal Boolean mailboxFileFilterNavProc (AEDesc *theItem, void *info, ZoneAndResFilePtr zoneAndResFile, NavFilterModes filterMode)

{
	ZoneAndResFileRec		navZoneAndResFile;
	NavFileOrFolderInfo	*theInfo;
	FSSpec							spec;
	Boolean							display;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (zoneAndResFile);

	display = true;
	theInfo = (NavFileOrFolderInfo*) info;
	if (theItem->descriptorType == typeFSS)
		if (!theInfo->isFolder) {
			AEGetDescData(theItem, &spec, sizeof(FSSpec));
			display = IsMailbox (&spec);
		}
	SetZoneAndResFile (&navZoneAndResFile);
	return (display);
}


OSErr SFODocNav (FSSpecPtr doc, SFTypeList types, Str255 prompt, FSSpec *spec, SFODocPtr optionsPtr, Boolean *good)

{
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavTypeListHandle		typeList;
	NavEventUPP					eventUPP;
	NavObjectFilterUPP	objectFilterUPP;
	OSErr								theError,
											disposeError;
	short								numTypes;
	
	*good = false;
	Zero (theReply);

	numTypes = CountOSTypes (types, 0);
	typeList = NewNavTypeList (CREATOR, numTypes, types);
	
	eventUPP				= NewNavEventUPP (sfODocNavProc);
	objectFilterUPP = NewNavObjectFilterUPP (sfOdocObjectFilterProc);
	theError				= NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		if (HaveOSX ())
			dialogOptions.dialogOptionFlags |= kNavSupportPackages;
		PCopy (dialogOptions.message, prompt);
		GetZoneAndResFile (&optionsPtr->zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFile (nil, &theReply, &dialogOptions, eventUPP, nil, objectFilterUPP, typeList, optionsPtr);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		SetZoneAndResFile (&optionsPtr->zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError) {
			if (*good = theReply.validRecord)
				theError = GetSpecFromNthDesc (&theReply.selection, 1, spec);
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	if (objectFilterUPP)
		DisposeNavObjectFilterUPP (objectFilterUPP);
	ZapHandle (typeList);
	return (theError);
}

pascal void sfODocNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, SFODocPtr optionsPtr)

{
	ZoneAndResFileRec	navZoneAndResFile;
	DialogPtr					theDialog;
	OSErr							theError;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (&optionsPtr->zoneAndResFile);
	
	theDialog = callBackParms->window ? GetDialogFromWindow (callBackParms->window) : nil;
	theError = noErr;
	switch (callBackSelector) {
		case kNavCBEvent:
			theError = HandleODocNavEvent (theDialog, callBackParms->eventData.eventDataParms.event, callBackParms->context, optionsPtr);
			break;

		case kNavCBCustomize :
			NegotiateCustomRectSize (&callBackParms->customRect, 400, 40);
			break;

		case kNavCBStart :
			theError = AddDialogItemsToNav (SFODOC_DITL, callBackParms->context);
			if (!theError)
				if (TypeIsOnList (optionsPtr->type, FINDER_LIST_TYPE) == kTOLOther) {
					optionsPtr->permanently = False;
					SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavOdocAlwaysUseThisApp), false);
					HiliteControl (GetDItemCtl (theDialog, GetCustomNavItem (callBackParms->context, rNavOdocAlwaysUseThisApp)), 255);
				}
				else
					SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavOdocAlwaysUseThisApp), optionsPtr->permanently);
			break;
	}
	SetZoneAndResFile (&navZoneAndResFile);
}


OSErr HandleODocNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, SFODocPtr optionsPtr)

{
	GrafPtr			savedPort;
	WindowPtr		win;
	Point				mouseLoc;
	OSErr				theError;
	short				partCode,
							firstItem,
							foundItem;
	
	theError = noErr;
	switch (event->what) {
		case mouseDown:
			partCode = FindWindow (event->where, &win);
			if (partCode == inContent && navDialog == GetDialogFromWindow (win)) {
				GetPort (&savedPort);
				SetPort (GetDialogPort(navDialog));	// Hack for Nav Services 1.0
				GetMouse (&mouseLoc);
				SetPort (savedPort);
				
				foundItem = FindDialogItem (navDialog, mouseLoc);
				if (foundItem >= 0) {
					firstItem = GetCustomNavItem (context, 1);
					switch (foundItem - firstItem + 2) {
						case rNavOdocUseFinder:
							optionsPtr->finder = True;
							NavCustomControl (context, kNavCtlCancel, nil);	// Can't return 'accept' since 'Choose' might be inactive
							break;
						case rNavOdocAlwaysUseThisApp:
							SetDItemState (navDialog, GetCustomNavItem (context, rNavOdocAlwaysUseThisApp), optionsPtr->permanently = !optionsPtr->permanently);
							break;
					}
				}
			}
			break;
		case updateEvt:
			DoUpdate ((WindowPtr) event->message, event);
			break;
	}
	return (theError);
}


pascal Boolean sfOdocObjectFilterProc (AEDesc *theItem, void *info, SFODocPtr optionsPtr, NavFilterModes filterMode)

{
	ZoneAndResFileRec		navZoneAndResFile;
	NavFileOrFolderInfo	*theInfo;
	FSSpec							spec;
	Str31								name;
	OSErr								theError;
	long								dirId;
	short								vRef;
	Boolean							display;


	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (&optionsPtr->zoneAndResFile);
	
	display = true;
	theInfo = (NavFileOrFolderInfo*) info;
	if (theItem->descriptorType == typeFSS)
		if (!theInfo->isFolder) {
			AEGetDescData(theItem, &spec, sizeof(FSSpec));
			if (theInfo->fileAndFolder.fileInfo.finderInfo.fdType == 'adrp') {
				PCopy (name, spec.name);
				vRef	= spec.vRefNum;
				dirId = spec.parID;
				if (theError = MyResolveAlias (&vRef, &dirId, name, nil))
					return (false);
				return (CanOpen (TypeToOpen, vRef, dirId, name));
			}
			return (CanOpen (TypeToOpen, spec.vRefNum, spec.parID, spec.name));
		}
	SetZoneAndResFile (&navZoneAndResFile);
	return (display);
}


OSErr SFURLAppNav (PStr proto, AliasHandle *alias, SFTypeList types, Str255 prompt, FSSpec *spec, URLHookOptionsPtr optionsPtr, Boolean *good)

{
	NavReplyRecord			theReply;
	NavDialogOptions		dialogOptions;
	NavTypeListHandle		typeList;
	NavEventUPP					eventUPP;
	OSErr								theError,
											disposeError;
	short								numTypes;
	
	*good = false;
	Zero (theReply);

	numTypes = CountOSTypes (types, 0);
	typeList = NewNavTypeList (CREATOR, numTypes, types);
	
	eventUPP = NewNavEventUPP (SFURLAppNavProc);
	theError = NavGetDefaultDialogOptions (&dialogOptions);
	if (theError)
		WarnUser (NAV_GENERAL_ERR, theError);

	if (!theError) {
		if (HaveOSX ())
			dialogOptions.dialogOptionFlags |= kNavSupportPackages;
		PCopy (dialogOptions.message, prompt);
		GetZoneAndResFile (&optionsPtr->zoneAndResFile);
		PushCursor(arrowCursor);	// Have to do this to clear the watch cursor for OS X
		theError = NavChooseFile (nil, &theReply, &dialogOptions, eventUPP, nil, nil, typeList, optionsPtr);
		PopCursor();		// Have to do this to clear the watch cursor for OS X
		SetZoneAndResFile (&optionsPtr->zoneAndResFile);
		if (theError && theError != userCanceledErr)
			WarnUser (NAV_GENERAL_ERR, theError);
		if (!theError) {
			if (*good = theReply.validRecord)
				theError = GetSpecFromNthDesc (&theReply.selection, 1, spec);
		}
		disposeError = NavDisposeReply (&theReply);
		if (!theError)
			theError = disposeError;
	}
	if (eventUPP)
		DisposeNavEventUPP (eventUPP);
	ZapHandle (typeList);
	return (theError);
}


pascal void SFURLAppNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, URLHookOptionsPtr optionsPtr)

{
	ZoneAndResFileRec	navZoneAndResFile;
	DialogPtr					theDialog;
	OSErr							theError;

	GetZoneAndResFile (&navZoneAndResFile);
	SetZoneAndResFile (&optionsPtr->zoneAndResFile);
	theDialog = callBackParms->window ? GetDialogFromWindow (callBackParms->window) : nil;

	theError = noErr;
	switch (callBackSelector) {
		case kNavCBEvent:
			theError = HandleURLAppNavEvent (theDialog, callBackParms->eventData.eventDataParms.event, callBackParms->context, optionsPtr);
			break;

		case kNavCBCustomize :
			NegotiateCustomRectSize (&callBackParms->customRect, 400, 40);
			break;

		case kNavCBStart :
			theError = AddDialogItemsToNav (URL_DITL, callBackParms->context);
			if (!theError)
				if (optionsPtr->protocol) {
					optionsPtr->permanently = False;
					SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavURLAppAlwaysUseThisApp), false);
					HiliteControl (GetDItemCtl (theDialog, GetCustomNavItem (callBackParms->context, rNavURLAppAlwaysUseThisApp)), 255);
				}
				else
					SetDItemState (theDialog, GetCustomNavItem (callBackParms->context, rNavURLAppAlwaysUseThisApp), optionsPtr->permanently);
			break;
	}
	SetZoneAndResFile (&navZoneAndResFile);
}


OSErr HandleURLAppNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, URLHookOptionsPtr optionsPtr)

{
	GrafPtr			savedPort;
	WindowPtr		win;
	Point				mouseLoc;
	OSErr				theError;
	short				partCode,
							firstItem,
							foundItem;
	
	theError = noErr;
	switch (event->what) {
		case mouseDown:
			partCode = FindWindow (event->where, &win);
			if (partCode == inContent && navDialog == GetDialogFromWindow (win)) {
				GetPort (&savedPort);
				SetPort (GetDialogPort(navDialog));	// Hack for Nav Services 1.0
				GetMouse (&mouseLoc);
				SetPort (savedPort);
				
				foundItem = FindDialogItem (navDialog, mouseLoc);
				if (foundItem >= 0) {
					firstItem = GetCustomNavItem (context, 1);
					if (foundItem - firstItem + 2 == rNavURLAppAlwaysUseThisApp)
						SetDItemState (navDialog, GetCustomNavItem (context, rNavURLAppAlwaysUseThisApp), optionsPtr->permanently = !optionsPtr->permanently);
				}
			}
			break;
		case updateEvt:
			DoUpdate ((WindowPtr) event->message, event);
			break;
	}
	return (theError);
}
