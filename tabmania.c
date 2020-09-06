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

#include "tabmania.h"
#define FILE_NUM 139

pascal Boolean SFChooseGraphicFilter(CInfoPBPtr pb, InsertHookData * ihdp);


//
//      CreateTabControl
//
//              Simple convenience routine for creating a tab control.
//

ControlHandle CreateTabControl(MyWindowPtr win, Rect * boundsRect,
			       OSType applOwner, OSType tabOwner,
			       Boolean currentResFileOnly)
{
	ControlHandle tabControl;
	TabHandle allTabs;
	Str255 tabResourceName;
	Handle newTab;
	OSErr theError;
	short tabIndex, index;

	theError = noErr;
	if (tabControl =
	    NewControl(GetMyWindowWindowPtr(win), boundsRect, "\p", false,
		       gBetterAppearance ? 0 : ADDRESS_BOOK_TABS, 0, 0,
		       kControlTabSmallProc, nil)) {
		LetsGetSmall(tabControl);

		// Allocate some space for the tab data
		allTabs = NuHandle(0);

		// Place the TabHandle into the control's refcon field
		SetControlReference(tabControl, (long) allTabs);

		// We'll first populate the control with tabs of the user's choosing
		index = 1;
		do {
			if (newTab =
			    GetNamedResource(TAB_TYPE,
					     GetRString(tabResourceName,
							ABTabOrderStrn +
							index++))) {
				theError =
				    AddTab(tabControl, allTabs, newTab,
					   applOwner, tabOwner);
				ReleaseResource(newTab);
			}
		} while (newTab && !theError && !currentResFileOnly);

		// Scanning the globe for any other 'eTab' resources
		tabIndex = 1;
		do {
			if (newTab = GetIndResource(TAB_TYPE, tabIndex++)) {
				theError =
				    AddTab(tabControl, allTabs, newTab,
					   applOwner, tabOwner);
				ReleaseResource(newTab);
			}
		} while (newTab && !theError && !currentResFileOnly);

		// Clean up if we didn't find any tabs
		if (theError || !CountTabs(allTabs)) {
			DisposeTabControl(tabControl, true);
			DisposeControl(tabControl);
			tabControl = nil;
		}
	}
	return (tabControl);
}


void DisposeTabControl(ControlHandle tabControl, Boolean disposeControls)
{
	TabHandle allTabs;
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	short objectIndex, numTabs;

	allTabs = GetTabHandle(tabControl);
	tabPtr = *allTabs;
	for (numTabs = CountTabs(allTabs); numTabs; --numTabs, ++tabPtr) {
		objectPtr = LDRef(tabPtr->objects);
		for (objectIndex = tabPtr->numObjects; objectIndex;
		     --objectIndex, ++objectPtr) {
			switch (objectPtr->type) {
			case fieldObject:
				DisposeLabelField(objectPtr->control);
				break;
			case pictureObject:
				DisposePictureObject(objectPtr);
				break;
			}
			ZapHandle(objectPtr->behavior);
		}
		ZapHandle(tabPtr->objects);
	}
	ZapHandle(allTabs);
	if (disposeControls)
		DisposeControl(tabControl);	// Takes care of all the embedded controls as well
}

void DisposePictureObject(TabObjectPtr objectPtr)
{
	Handle hRef;
	ImageWellRemovePicture(objectPtr->control);
	hRef = (Handle) GetControlReference(objectPtr->control);
	SetControlReference(objectPtr->control, 0);
	ZapHandle(hRef);
}


//
//      AddTab
//
//      Take the data out of an 'eTab' resource and put it into a structure that makes
//      sense to our tab management stuff.  Each of the panes we'll create for the tab
//      will store in the refcon field its 'tabIndex'.  So, for instance, if we have a
//      tab control with three panes, the first pane will store 0, the second stores 1
//      and the third stores 2.  The indexes prove useful when we need to go back and
//      grab the TabRec for this pane.
//

OSErr AddTab(ControlHandle tabControl, TabHandle tabs, Handle newTab,
	     OSType applOwner, OSType tabOwner)
{
	TabRec newTabRec;
	TabResHeaderRec tabResHeader;
	TabObjectPtr objectPtr;
	ControlTabInfoRec tabInfo;
	ControlHandle tabPane;
	Rect tabContentBounds;
	Ptr from;
	OSErr theError;
	short tabNumber, objectIndex;

	// Be defensive
	if (!tabs || !newTab)
		return (noErr);

	// Parse the tab header
	from = *newTab;
	from = ParseTabHeader(from, &tabResHeader);

	// Bail if this tab doesn't look particularly interesting
	if (!InterestingTab(tabs, &tabResHeader, applOwner, tabOwner))
		return (noErr);

	// Add this tab to the control
	if (gBetterAppearance) {
		tabInfo.version = 0;
		tabInfo.iconSuiteID = tabResHeader.iconSuiteID;
		PCopy(tabInfo.name, tabResHeader.title);
		SetControlMaximum(tabControl, tabNumber =
				  GetTabPaneCount(tabControl) + 1);
		SetControlData(tabControl, tabNumber, kControlTabInfoTag,
			       sizeof(ControlTabInfoRec), (Ptr) & tabInfo);
	}
	// We're going to create a blank user pane as an embedder for each tab we create.  We can then peform nifty
	// tab switching operations by simply hiding or showing the user pane.
	GetTabContentRect(tabControl, &tabContentBounds);
	tabPane =
	    NewControl(GetControlOwner(tabControl), &tabContentBounds,
		       tabResHeader.title, !gBetterAppearance,
		       kControlSupportsEmbedding, 0, 0,
		       kControlUserPaneProc, nil);
	theError = MemError();
	if (!theError)
		theError = EmbedControl(tabPane, tabControl);
	// Initialize a tab record and concatenate it to the other tabs in this control
	if (!theError) {
		newTabRec.tabSignature = tabResHeader.tabSignature;
		newTabRec.defaultHorzMargin =
		    tabResHeader.defaultHorzMargin;
		newTabRec.defaultVertMargin =
		    tabResHeader.defaultVertMargin;
		newTabRec.numObjects = tabResHeader.numObjects;
		newTabRec.focusObject = kNoTabObjectFocus;
		newTabRec.objects =
		    NuHandleClear(newTabRec.numObjects *
				  sizeof(TabObjectRec));
		theError = MemError();
	}
	if (!theError) {
		// Store the index of this pane in the refcon
		SetControlReference(tabPane, CountTabs(tabs));
		theError =
		    PtrPlusHand_(&newTabRec, tabs, sizeof(newTabRec));
	}
	// Fetch all of the objects on this tab
	objectPtr = LDRef(newTabRec.objects);
	for (objectIndex = 0;
	     !theError && objectIndex < newTabRec.numObjects;
	     ++objectIndex)
		from = ParseObject(tabPane, from, objectPtr++);
	UL(newTabRec.objects);
	// Always hide the tabs we add
	if (!theError)
		HideTab(tabPane);
	return (theError);
}


Boolean InterestingTab(TabHandle tabs, TabResHeaderPtr tabResHeader,
		       OSType applOwner, OSType tabOwner)
{
	TabPtr tabPtr;
	short numTabs, tabIndex;

	// Bail if this tab doesn't look particularly interesting
	if ((applOwner != tabResHeader->applOwner
	     && applOwner != kTabTypeWildCard)
	    || (tabOwner != tabResHeader->tabOwner
		&& tabOwner != kTabTypeWildCard))
		return (false);

	// We also bail if we already have a tab group with this owner
	numTabs = CountTabs(tabs);
	for (tabIndex = 1, tabPtr = *tabs; tabIndex <= numTabs;
	     ++tabIndex, ++tabPtr)
		if (tabResHeader->tabSignature == tabPtr->tabSignature)
			return (false);
	return (true);
}


void TabActivate(ControlHandle tabControl, Boolean isActive)
{
	ControlHandle tabPane;

	if (tabControl) {
		tabPane = GetActiveTabUserPane(tabControl);
		IterateTabPaneObjectsUL(tabPane, TabActivateProc,
					&isActive);
	}
}


Boolean TabActivateProc(TabObjectPtr objectPtr, Boolean * isActive)
{
	switch (objectPtr->type) {
	case pictureObject:
		ImageWellDrawFocus(objectPtr->control, *isActive
				   && objectPtr->hasFocus);
		break;
	}
	return (false);
}


void TabMenuEnable(ControlHandle tabControl, Boolean ro)
{
	TabObjectPtr objectPtr;
	MenuHandle mh;
	Boolean selection;

	if (objectPtr = FindObjectWithFocus(tabControl)) {
		if (objectPtr->type == pictureObject) {
			selection = TabHasSelection(tabControl);
			mh = GetMHandle(EDIT_MENU);
			EnableIf(mh, EDIT_CUT_ITEM, selection && !ro);
			EnableIf(mh, EDIT_COPY_ITEM, selection);
			EnableIf(mh, EDIT_CLEAR_ITEM, selection && !ro);
			EnableIf(mh, EDIT_PASTE_ITEM, IsScrapFull()
				 && !ro);
		}
		UnlockObject(objectPtr);
	}
}


Boolean TabMenu(ControlHandle tabControl, int menu, int item,
		short modifiers)
{
	TabObjectPtr objectPtr;
	Boolean menuHandled;

	menuHandled = false;
	if (objectPtr = FindObjectWithFocus(tabControl)) {
		if (objectPtr->type == pictureObject)
			switch (menu) {
			case EDIT_MENU:
				switch (item) {
				case EDIT_CUT_ITEM:
				case EDIT_COPY_ITEM:
					if (CopyPictureFromImageWell
					    (objectPtr->control)
					    && item == EDIT_CUT_ITEM)
						if (SetPictureObjectValue
						    (tabControl, objectPtr,
						     nil, 0))
							PeepingTom
							    (tabControl);
					menuHandled = true;
					break;
				case EDIT_CLEAR_ITEM:
					if (SetPictureObjectValue
					    (tabControl, objectPtr, nil,
					     0))
						PeepingTom(tabControl);
					menuHandled = true;
					break;
				case EDIT_PASTE_ITEM:
					PastePictureIntoImageWell
					    (tabControl, objectPtr);
					break;
				}
				break;
			}
		UnlockObject(objectPtr);
	}
	return (menuHandled);
}


Boolean TabKey(ControlHandle tabControl, EventRecord * event)
{
	TabObjectPtr objectPtr;
	short key;
	Boolean keyHandled;

	keyHandled = false;
	key = (event->message & 0xff);
	if (objectPtr = FindObjectWithFocus(tabControl))
		if (objectPtr->type == pictureObject)
			if (key == backSpace || key == delChar)
				if (SetPictureObjectValue
				    (tabControl, objectPtr, nil, 0)) {
					PeepingTom(tabControl);
					keyHandled = true;
				}
	return (keyHandled);
}


Boolean TabDragIsInteresting(ControlHandle tabControl, DragReference drag)
{

	ControlHandle tabPane;
	TabObjectPtr objectPtr;
	Boolean isInteresting;

	isInteresting = false;
	if (tabControl) {
		tabPane = GetActiveTabUserPane(tabControl);
		if (objectPtr =
		    IterateTabPaneObjects(tabPane,
					  TabDragIsInterestingProc,
					  drag)) {
			isInteresting = true;
			UnlockObject(objectPtr);
		}
	}
	return (isInteresting);
}

Boolean TabDragIsInterestingProc(TabObjectPtr objectPtr,
				 DragReference drag)
{
	switch (objectPtr->type) {
	case pictureObject:
		if (DragIsImage(drag)
		    || DragIsInteresting(drag, objectPtr->dragFlavor, nil))
			return (true);
		break;
	}
	return (false);
}


OSErr TabDrag(ControlHandle tabControl, DragTrackingMessage which,
	      DragReference drag)
{
	TabObjectPtr objectPtr;
	ControlHandle tabPane;
	Point mouse, pinnedMouse;
	OSErr theError;

	SetPort_(GetWindowPort(GetControlOwner(tabControl)));
	theError = GetDragMouse(drag, &mouse, &pinnedMouse);
	if (!theError) {
		tabPane = GetActiveTabUserPane(tabControl);
		GlobalToLocal(&mouse);
		objectPtr = PtInTabObject(tabPane, mouse);
		switch (which) {
		case kDragTrackingInWindow:
			if (!theError && objectPtr) {
				if (DragIsInteresting
				    (drag, objectPtr->dragFlavor, nil))
					switch (objectPtr->type) {
					case pictureObject:
						HiliteImageWell(objectPtr->
								control,
								drag,
								true);
						return (theError);
						break;
					}
			}
			HiliteImageWell(nil, drag, false);
			break;
		case 0xfff:
			//      Plop
			theError = dragNotAcceptedErr;
			HiliteImageWell(nil, drag, false);
			if (objectPtr)
				switch (objectPtr->type) {
				case pictureObject:
					theError =
					    ImageWellDrop(tabControl,
							  objectPtr, drag);
					break;

				}
			break;
		}
		UnlockObject(objectPtr);
	}
	return (theError);
}


OSErr ImageWellDrop(ControlHandle tabControl, TabObjectPtr objectPtr,
		    DragReference drag)
{
	ControlHandle tabPane;
	TabPtr tabPtr;
	TabObjectPtr focusObjectPtr;
	PicHandle picture;
	PromiseHFSFlavor **promise;
	HFSFlavor **data;
	AliasHandle alias;
	AEDesc dropLocation;
	FSSpec spec, tempSpec;
	OSErr theError;
	SignedByte oldTabHState, oldObjectHState;
	long tempDirId;
	short tempVRef;

	data = nil;
	promise = nil;
	if (!
	    (theError =
	     MyGetDragItemData(drag, 1, 'PICT', (void *) &picture)))
		theError =
		    ReplaceImageWellPicture(tabControl, objectPtr,
					    picture);
	else {
		// Funky stuff from CompDrop to accomodate braindeadness of older Finders and Find File.
		// first, see if the "promised" file is really there already
		// we only do this if the promised flavor is one that we know is ok, ie
		// from an app that handles promiseHFS in the particular way we need
		if (theError =
		    MyGetDragItemData(drag, 1, flavorTypeHFS,
				      (void *) &data))
			if (!
			    (theError =
			     MyGetDragItemData(drag, 1,
					       flavorTypePromiseHFS,
					       (void *) &promise))) {
				if (TypeIsOnList
				    ((*promise)->promisedFlavor,
				     PRE_PROMISE_OK))
					theError =
					    MyGetDragItemData(drag, 1,
							      (*promise)->
							      promisedFlavor,
							      (void *)
							      &data);
				else if (TypeIsOnList
					 ((*promise)->promisedFlavor,
					  PROMISE_OK)) {
					NullADList(&dropLocation, nil);

					theError =
					    FindTemporaryFolder(Root.vRef,
								Root.dirId,
								&tempDirId,
								&tempVRef);
					if (!theError)
						theError =
						    FSMakeFSSpec(tempVRef,
								 tempDirId,
								 "\p",
								 &spec);
					if (!theError)
						if (!
						    (theError =
						     NewAlias(nil, &spec,
							      &alias))) {
							//set the droplocation to it
							AECreateDesc
							    (typeAlias,
							     LDRef(alias),
							     GetHandleSize_
							     (alias),
							     &dropLocation);
							ZapHandle(alias);
							SetDropLocation
							    (drag,
							     &dropLocation);
							// now find the file
							theError =
							    MyGetDragItemData
							    (drag, 1,
							     (*promise)->
							     promisedFlavor,
							     (void *)
							     &data);
							DisposeADList
							    (&dropLocation,
							     nil);
						}
				}
			}
		if (!theError) {
			spec = (*data)->fileSpec;
			theError =
			    NewTempSpec(Root.vRef, Root.dirId, nil,
					&tempSpec);
		}
		if (!theError)
			theError =
			    FSpDupFile(&tempSpec, &spec, false, false);
		if (!theError)
			SetImageWellFSSpec(tabControl, objectPtr,
					   &tempSpec);
	}
	ZapHandle(data);
	ZapHandle(promise);

	if (!theError) {
		tabPane = GetActiveTabUserPane(tabControl);
		tabPtr =
		    LockTabs(tabControl, GetTabPaneIndex(tabPane),
			     &oldTabHState);
		focusObjectPtr =
		    tabPtr->focusObject !=
		    kNoTabObjectFocus ? LockTabObjects(tabPtr,
						       tabPtr->focusObject,
						       &oldObjectHState) :
		    nil;
		// If the focus is somewhere besides the object that was hit, remove it
		ABSetKeyboardFocus(focusTabControl);
		if (objectPtr != focusObjectPtr) {
			if (focusObjectPtr)
				TabObjectSetKeyboardFocus(tabPtr,
							  focusObjectPtr,
							  false);
			TabObjectSetKeyboardFocus(tabPtr, objectPtr, true);
		}
		if (focusObjectPtr)
			UnlockTabObjects(tabPtr, oldObjectHState);
		UnlockTabs(tabControl, oldTabHState);
	}
	return (theError);
}

void HiliteImageWell(ControlHandle theControl, DragReference drag,
		     Boolean hilite)
{
	RGBColor backColor, saveColor;
	Rect bounds;

	GetBackColor(&saveColor);
	backColor.red = 0xffff;
	backColor.green = 0xffff;
	backColor.blue = 0xffff;
	RGBBackColor(&backColor);
	if (hilite) {
		GetControlBounds(theControl, &bounds);
		InsetRect(&bounds, 2, 2);
		ShowDragRectHilite(drag, &bounds, true);
	} else
		HideDragHilite(drag);
	RGBBackColor(&saveColor);
}


/**********************************************************************
 * CopyPictureFromImageWell - copy the content of the image well onto the clipboard
 **********************************************************************/
Boolean CopyPictureFromImageWell(ControlHandle theControl)
{
	PicHandle picture;
	OSErr theError;

	theError = noErr;
	if (theControl)
		if (picture = GetImageWellPicture(theControl)) {
			ZeroScrap();
			theError =
			    PutScrap(GetHandleSize(picture), 'PICT',
				     LDRef(picture));
			UL(picture);
			if (!theError)
				return (true);
		}
	SysBeep(1);
	return (false);
}


void PastePictureIntoImageWell(ControlHandle tabControl,
			       TabObjectPtr objectPtr)
{
	Handle picture;
	OSErr theError;
	long scrapResult, offset;

	picture = NuHandle(0);
	theError = MemError();
	if (!theError) {
		scrapResult = GetScrap(picture, kPictureScrap, &offset);
		if (scrapResult < 0)
			theError = scrapResult;
	}
	if (!theError)
		theError =
		    ReplaceImageWellPicture(tabControl, objectPtr,
					    picture);
	if (theError)
		SysBeep(1);
}


OSErr ReplaceImageWellPicture(ControlHandle tabControl,
			      TabObjectPtr objectPtr, PicHandle picture)
{
	FSSpec tempSpec;
	FInfo info;
	Str31 nickname;
	PicHandle oldPicture;
	Ptr header;
	OSErr theError;
	short iconRes, oldResFile;

	if (oldPicture = GetImageWellPicture(objectPtr->control))
		KillPicture(oldPicture);

	theError = SetImageWellPicture(objectPtr->control, picture);

	if (!theError)
		theError =
		    NewTempSpec(Root.vRef, Root.dirId, nil, &tempSpec);
	if (!theError)
		theError =
		    FSpCreate(&tempSpec, CREATOR, 'PICT', smSystemScript);

	if (!theError) {
		header = NuPtrClear(512);
		theError = MemError();
		if (!theError)
			theError = BlatPtr(&tempSpec, header, 512, false);
		if (!theError)
			theError = Blat(&tempSpec, picture, true);
	}
	// Place an icon suite of the picture into the file
	if (!theError) {
		oldResFile = CurResFile();
		FSpCreateResFile(&tempSpec, CREATOR, 'PICT',
				 smSystemScript);
		theError = ResError();
		if (!theError) {
			iconRes = FSpOpenResFile(&tempSpec, fsRdWrPerm);
			theError = ResError();
			if (!theError && iconRes != -1) {
				theError =
				    FSpMakeIconSuite(&tempSpec, picture,
						     GetSelectedName(1,
								     nickname));
				CloseResFile(iconRes);
				// set the icon of the new file to the custom icon
				if (!theError) {
					FSpGetFInfo(&tempSpec, &info);
					info.fdFlags |= kHasCustomIcon;
					FSpSetFInfo(&tempSpec, &info);
				}
			}
		}
		UseResFile(oldResFile);
	}
	if (!theError)
		SetImageWellFSSpec(tabControl, objectPtr, &tempSpec);
	return (theError);
}



OSErr FSpMakeIconSuite(FSSpec * tempSpec, PicHandle picture, PStr name)
{
	GraphicsImportComponent importer;
	MatrixRecord whatIsTheMatrix;
	GWorldPtr gWorld;
	RGBColor color, *transparentColor;
	Rect iconRect, srcRect;
	OSErr theError;
	short width, height, scale;

	SetRect(&iconRect, 0, 0, 32, 32);

	// Create an offscreen GWorld
	theError = NewGWorld(&gWorld, 8, &iconRect, nil, nil, useTempMem);	//      Try temp memory first
	if (theError)
		theError = NewGWorld(&gWorld, 8, &iconRect, nil, nil, nil);	//      Failed, use application heap
	if (!theError) {
		PushGWorld();
		SetGWorld(gWorld, nil);
		EraseRect(&iconRect);
		// figure out which importer can open the picture file ...
		theError = GetGraphicsImporterForFile(tempSpec, &importer);

		// Tell the importer where to put it ...
		if (!theError) {
			theError =
			    GraphicsImportSetGWorld(importer, gWorld, nil);

			// Tell the importer how to scale the picture when drawn ...
			if (!theError)
				theError =
				    GraphicsImportGetBoundsRect(importer,
								&srcRect);

			if (!theError) {
				width = RectWi(srcRect);
				height = RectHi(srcRect);
				if (height > width) {
					scale = 32 * width / height;
					SetRect(&iconRect,
						(32 - scale) / 2, 0,
						(32 - scale) / 2 + scale,
						32);
				} else {
					scale = 32 * height / width;
					SetRect(&iconRect, 0,
						(32 - scale) / 2, 32,
						(32 - scale) / 2 + scale);
				}
				RectMatrix(&whatIsTheMatrix, &srcRect,
					   &iconRect);
				theError =
				    GraphicsImportSetMatrix(importer,
							    &whatIsTheMatrix);
			}
			if (!theError)
				theError = GraphicsImportDraw(importer);

			if (!theError) {
				transparentColor = nil;
				if (GetPNGTransColor
				    (importer, tempSpec, &color))
					transparentColor = &color;
				else {
					GetBackColor(&color);
					transparentColor = &color;
				}
				FrameRect(&iconRect);
				SetRect(&iconRect, 0, 0, 32, 32);
				theError =
				    MakeIconSuite(gWorld, &iconRect,
						  transparentColor, name);
			}
			// Cleanup
			CloseComponent(importer);
		}
		PopGWorld();
	}
	// Cleanup
	if (gWorld)
		DisposeGWorld(gWorld);
	return (theError);
}



Boolean TabHasSelection(ControlHandle tabControl)
{
	TabObjectPtr objectPtr;
	PETEHandle pte;
	PicHandle picture;
	UHandle text;
	long start, stop;
	Boolean hasSelection;

	hasSelection = false;

	if (objectPtr = FindObjectWithFocus(tabControl)) {
		switch (objectPtr->type) {
		case fieldObject:
			if (pte = GetLabelFieldPete(objectPtr->control))
				if (!PeteGetTextAndSelection
				    (pte, &text, &start, &stop))
					hasSelection = (stop != start);
			break;
		case pictureObject:
			if (objectPtr->control)
				if (picture =
				    GetImageWellPicture(objectPtr->
							control))
					hasSelection =
					    GetHandleSize(picture) > 0;
			break;
		}
		UnlockObject(objectPtr);
	}
	return (hasSelection);
}



//
//      ClickTab
//
//              Process clicks in the _content_ of the tab
//

Boolean ClickTab(ControlHandle tabControl, EventRecord * event, Point pt)
{
	WindowPtr winWP = GetControlOwner(tabControl);
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	TabPtr tabPtr;
	TabObjectPtr objectPtr, focusObjectPtr;
	ControlHandle tabPane;
	ControlHandle theControl;
	short part;
	SignedByte oldTabHState, oldObjectHState;
	Boolean accepted;

	accepted = false;
	tabPane = GetActiveTabUserPane(tabControl);
	tabPtr =
	    LockTabs(tabControl, GetTabPaneIndex(tabPane), &oldTabHState);
	focusObjectPtr =
	    tabPtr->focusObject !=
	    kNoTabObjectFocus ? LockTabObjects(tabPtr, tabPtr->focusObject,
					       &oldObjectHState) : nil;
	// Give all of the objects on the tab a chance
	if (objectPtr =
	    PtInTabObject(GetActiveTabUserPane(tabControl), pt)) {
		// If the focus is somewhere besides the object that was hit, remove it
		if (objectPtr != focusObjectPtr) {
			if (focusObjectPtr)
				TabObjectSetKeyboardFocus(tabPtr,
							  focusObjectPtr,
							  false);
			TabObjectSetKeyboardFocus(tabPtr, objectPtr, true);
		}
		switch (objectPtr->type) {
		case fieldObject:
			if (part =
			    FindControl(pt, GetControlOwner(tabControl),
					&theControl))
				if (part != kControlLabelPart) {
					PeteEdit(win,
						 GetLabelFieldPete
						 (objectPtr->control),
						 peeEvent, (void *) event);
					accepted = true;
				}
			break;
		case controlObject:
		case controlResourceObject:
			// track the control and handle any special behavior
			if (TrackControl
			    (objectPtr->control, pt, (void *) (-1)))
				if (objectPtr->behavior)
					HandleTabObjectBehavior(tabControl,
								objectPtr);
			break;
		case pictureObject:
			ImageWellDrawFocus(objectPtr->control,
					   objectPtr->hasFocus);
			ImageWellDrag(objectPtr->control, event);
			accepted = true;
			break;
		}
		UnlockObject(objectPtr);
	}
	if (focusObjectPtr)
		UnlockTabObjects(tabPtr, oldObjectHState);
	UnlockTabs(tabControl, oldTabHState);
	return (accepted);
}

//
//      TabSetKeyboardFocus
//
//              Give (or give up) focus involving the Tab control.
//

Boolean TabSetKeyboardFocus(ControlHandle tabControl,
			    TabFocusType tabFocus, Boolean getFocus)
{
	TabPtr tabPtr;
	TabObjectPtr objectPtr, foundObjectPtr;
	ControlHandle tabPane;
	SignedByte oldTabHState, oldObjectHState;

	tabPane = GetActiveTabUserPane(tabControl);
	tabPtr =
	    LockTabs(tabControl, GetTabPaneIndex(tabPane), &oldTabHState);
	if (getFocus) {
		foundObjectPtr = nil;
		IterateTabPaneObjects(tabPane,
				      tabFocus ==
				      focusTabControlReverse ?
				      LastFocusObjectProc :
				      FirstFocusObjectProc,
				      &foundObjectPtr);
		if (foundObjectPtr) {
			TabObjectSetKeyboardFocus(tabPtr, foundObjectPtr,
						  true);
			UnlockObject(foundObjectPtr);
		}
	} else if (tabPtr->focusObject != kNoTabObjectFocus) {
		objectPtr =
		    LockTabObjects(tabPtr, tabPtr->focusObject,
				   &oldObjectHState);
		TabObjectSetKeyboardFocus(tabPtr, objectPtr, false);
		UnlockTabObjects(tabPtr, oldObjectHState);
	}
	UnlockTabs(tabControl, oldTabHState);
	return (true);
}


Boolean FirstFocusObjectProc(TabObjectPtr objectPtr,
			     TabObjectPtr * foundObjectPtr)
{
	if (objectPtr->flags & objectFlagAcceptsFocus) {
		*foundObjectPtr = objectPtr;
		return (true);
	}
	return (false);
}


Boolean LastFocusObjectProc(TabObjectPtr objectPtr,
			    TabObjectPtr * foundObjectPtr)
{
	if (objectPtr->flags & objectFlagAcceptsFocus)
		*foundObjectPtr = objectPtr;
	return (false);
}


Boolean TabObjectSetKeyboardFocus(TabPtr tabPtr, TabObjectPtr objectPtr,
				  Boolean getFocus)
{
	MyWindowPtr win =
	    GetWindowMyWindowPtr(GetControlOwner(objectPtr->control));
	PETEHandle pte;
	if (win && win->pte)
		PeteFocus(win, win->pte, false);

	if (objectPtr->flags & objectFlagAcceptsFocus)
		if (getFocus) {
			switch (objectPtr->type) {
			case fieldObject:
				SetKeyboardFocus(GetControlOwner
						 (objectPtr->control),
						 objectPtr->control,
						 kControlEditTextPart);
				if (win
				    && (pte =
					GetLabelFieldPete(objectPtr->
							  control)))
					PeteFocus(win, pte, true);
				break;
			case pictureObject:
				ImageWellDrawFocus(objectPtr->control,
						   true);
				break;
			}
			tabPtr->focusObject = objectPtr - *tabPtr->objects;
			objectPtr->hasFocus = true;
		} else {
			switch (objectPtr->type) {
			case fieldObject:
				SetKeyboardFocus(GetControlOwner
						 (objectPtr->control),
						 objectPtr->control,
						 kControlFocusNoPart);
				break;
			case pictureObject:
				ImageWellDrawFocus(objectPtr->control,
						   false);
				break;
			}
			tabPtr->focusObject = kNoTabObjectFocus;
			objectPtr->hasFocus = false;
		}
	return (objectPtr->flags & objectFlagAcceptsFocus ? true : false);
}



Boolean TabAdvanceKeyboardFocus(ControlHandle tabControl)
{
	TabPtr tabPtr;
	TabObjectPtr focusObjectPtr, objectPtr;
	ControlHandle tabPane;
	SignedByte oldTabHState, oldObjectHState;
	Boolean advanceOutOfTab;

	advanceOutOfTab = false;
	tabPane = GetActiveTabUserPane(tabControl);
	tabPtr =
	    LockTabs(tabControl, GetTabPaneIndex(tabPane), &oldTabHState);
	focusObjectPtr =
	    LockTabObjects(tabPtr, tabPtr->focusObject, &oldObjectHState);
	objectPtr = focusObjectPtr;
	while (++tabPtr->focusObject < tabPtr->numObjects) {
		++objectPtr;
		if (objectPtr->flags & objectFlagAcceptsFocus) {
			TabObjectSetKeyboardFocus(tabPtr, focusObjectPtr,
						  false);
			TabObjectSetKeyboardFocus(tabPtr, objectPtr, true);
			break;
		}
	}
	if (tabPtr->focusObject >= tabPtr->numObjects) {
		TabObjectSetKeyboardFocus(tabPtr, focusObjectPtr, false);
		tabPtr->focusObject = kNoTabObjectFocus;
		advanceOutOfTab = true;
	}
	UnlockTabObjects(tabPtr, oldObjectHState);
	UnlockTabs(tabControl, oldTabHState);
	return (advanceOutOfTab);
}

Boolean TabReverseKeyboardFocus(ControlHandle tabControl)
{
	TabPtr tabPtr;
	TabObjectPtr focusObjectPtr, objectPtr;
	ControlHandle tabPane;
	SignedByte oldTabHState, oldObjectHState;
	Boolean reverseOutOfTab;

	reverseOutOfTab = false;
	tabPane = GetActiveTabUserPane(tabControl);
	tabPtr =
	    LockTabs(tabControl, GetTabPaneIndex(tabPane), &oldTabHState);
	focusObjectPtr =
	    LockTabObjects(tabPtr, tabPtr->focusObject, &oldObjectHState);
	objectPtr = focusObjectPtr;
	while (--tabPtr->focusObject > kNoTabObjectFocus) {
		--objectPtr;
		if (objectPtr->flags & objectFlagAcceptsFocus) {
			TabObjectSetKeyboardFocus(tabPtr, focusObjectPtr,
						  false);
			TabObjectSetKeyboardFocus(tabPtr, objectPtr, true);
			break;
		}
	}
	if (tabPtr->focusObject <= kNoTabObjectFocus) {
		TabObjectSetKeyboardFocus(tabPtr, focusObjectPtr, false);
		tabPtr->focusObject = kNoTabObjectFocus;
		reverseOutOfTab = true;
	}
	UnlockTabObjects(tabPtr, oldObjectHState);
	UnlockTabs(tabControl, oldTabHState);
	return (reverseOutOfTab);
}


//
//      TabFindString
//
//              Find the search string in a tab field, starting from the field following the
//              current tab focus and searching forward to the end of all the tab panes.  This
//              search does NOT wrap, instead we'll return false which is a signal to the caller
//              that the search has failed.  (The caller, presumably the address book, then knows
//              to move on to it's next object.  In the case of the address book this will be the
//              next nickname in the nick list.)
//

Boolean TabFindString(ControlHandle tabControl, Boolean hasFocus,
		      PStr what)
{
	TabFindStringProcParamRec params;
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	PETEHandle pte;
	MyWindowPtr win =
	    GetWindowMyWindowPtr(GetControlOwner(tabControl));
	ControlHandle tabPane, objectPane;
	short oldTabIndex;
	SignedByte oldTabHState;
	Boolean found;

	found = false;

	params.what = what;
	params.focusFound = false;
	params.hasFocus = hasFocus;
	if (objectPtr =
	    IterateTabObjects(tabControl, TabFindStringProc, &params)) {
		// If we found an object containing the search string, select the pane, field and text

		// First, we need to clear the old focus.  Eventually this should be in a callback
		ABClearKeyboardFocus();

		// If the object we found is not on the same pane as the focus, we need to switch panes
		GetSuperControl(objectPtr->control, &objectPane);
		if (objectPane
		    && objectPane != GetActiveTabUserPane(tabControl)) {
			oldTabIndex = GetActiveTabIndex(tabControl);
			SetControlValue(tabControl,
					GetTabPaneIndex(objectPane) + 1);
			SwitchTabs(tabControl, oldTabIndex,
				   GetActiveTabIndex(tabControl),
				   PREF_LAST_NICK_TAB);
		}
		// Select the new focus
		tabPane = GetActiveTabUserPane(tabControl);
		tabPtr =
		    LockTabs(tabControl, GetTabPaneIndex(tabPane),
			     &oldTabHState);
		TabObjectSetKeyboardFocus(tabPtr, objectPtr, true);
		// Select the found text
		if (pte = GetLabelFieldPete(objectPtr->control)) {
			PeteSelect(win, pte, params.offset,
				   params.offset + *what);
			PeteScroll(pte, pseCenterSelection,
				   pseCenterSelection);
		}
		UnlockTabs(tabControl, oldTabHState);

		UnlockObject(objectPtr);
		found = true;
	}
	return (found);
}



//
//      TabFindStringProc
//
//              Returns true when we've reached the object containing the search string AFTER the object that
//              actually possesses focus -- unless no object has the focus, in which case it returns the first matching object
Boolean TabFindStringProc(TabObjectPtr objectPtr,
			  TabFindStringProcParamPtr paramsPtr)
{
	PETEHandle pte;

	// We only care about field objects (because they have searchable text)
	if (objectPtr->type == fieldObject) {
		// If we've previously found the focus object, we can look at the text
		if (paramsPtr->focusFound || !paramsPtr->hasFocus) {
			if (pte = GetLabelFieldPete(objectPtr->control)) {
				// If the text is found, we're done!
				if ((paramsPtr->offset =
				     PeteFindString(paramsPtr->what, 0,
						    pte)) >= 0)
					return (true);
			}
			return (false);
		}
		// If this object has the focus, we can start checking future field objects for the search text
		if (objectPtr->hasFocus)
			paramsPtr->focusFound = true;
	}
	return (false);
}



//
//      HandleTabObjectBehavior
//
//              At some hypothetical point in time this routine could handle all kinds of wacky object
//              behaviors.  For now though, it is setup to handle only behaviors we expect in our
//              default tabs.  This is robust enough, however, to move objects from tab to tab and have
//              the control behaviors still work.
//

void HandleTabObjectBehavior(ControlHandle tabControl,
			     TabObjectPtr objectPtr)
{
	ControlHandle tabPaneOne, tabPaneTwo;
	TabObjectPtr objectOne, objectTwo;

	// checkbox type behavior
	if ((*objectPtr->behavior)->flags & behaveCheckBox) {
		SetControlValue(objectPtr->control,
				1 - GetControlValue(objectPtr->control));
		DirtyActiveTabPane(tabControl);
		if ((*objectPtr->behavior)->
		    flags & behaveMutuallyExclusive)
			if ((*objectPtr->behavior)->flags & behaveAllTabs)
				IterateTabObjectsUL(tabControl,
						    MutuallyExclusiveCheckboxProc,
						    objectPtr);
	} else
		// "swapper" type behavior
	if ((*objectPtr->behavior)->flags & behaveSwapper) {

		// swap two fields (like first and last name)
		if ((*objectPtr->behavior)->flags & behaveFields) {
			objectOne =
			    FindObjectWithTag(tabControl,
					      (*objectPtr->behavior)->
					      stringOne);
			objectTwo =
			    FindObjectWithTag(tabControl,
					      (*objectPtr->behavior)->
					      stringTwo);
			SwapObjectValues(objectOne, objectTwo);
			DirtyActiveTabPane(tabControl);
			UnlockObject(objectOne);
			UnlockObject(objectTwo);
		} else
			// swap two tabs (like home and work)
		if ((*objectPtr->behavior)->flags & behaveTwoTabs) {
			tabPaneOne =
			    FindTabPaneWithTitle(tabControl,
						 (*objectPtr->behavior)->
						 stringOne);
			tabPaneTwo =
			    FindTabPaneWithTitle(tabControl,
						 (*objectPtr->behavior)->
						 stringTwo);
			// Iterate over each of the objects on the first pane
			IterateTabPaneObjectsUL(tabPaneOne,
						SwapTabObjectsProc,
						tabPaneTwo);
			DirtyActiveTabPane(tabControl);
		}
	} else
		// Nav Services
	if ((*objectPtr->behavior)->flags & behaveNavigation) {
		FSSpec spec;
		Str255 prompt;
		Boolean result;

		// Ask the user to choose a file to be placed in the image well
		if (UseNavServices())
			GetFileNav(nil, CHOOSE_PICTURE_TITLE,
				   GetRString(prompt,
					      CHOOSE_NICK_PICTURE_PROMPT),
				   SELECT, false, &spec, &result,
				   chooseGraphicObjectFilterProc);

		// If a file was selected, compose a file URL, copy that graphic into a temporary file
		// store the URL in the refcon of the image well and display the image.  We'll move the
		// temp file into the Photo Album on an address book save -- just in case the user changes
		// his or her mind.
		if (result) {
			FSSpec tempSpec;
			TabPtr pictObjectPtr;

			if (!NewTempSpec
			    (Root.vRef, Root.dirId, nil, &tempSpec))
				if (!FSpDupFile
				    (&tempSpec, &spec, false, false))
					if (pictObjectPtr =
					    FindObjectWithTag(tabControl,
							      (*objectPtr->
							       behavior)->
							      stringOne)) {
						SetImageWellFSSpec
						    (tabControl,
						     pictObjectPtr,
						     &tempSpec);
						UnlockObject
						    (pictObjectPtr);
					}
		}
	}
}

pascal Boolean chooseGraphicObjectFilterProc(AEDesc * theItem, void *info,
					     SFODocPtr optionsPtr,
					     NavFilterModes filterMode)
{
	NavFileOrFolderInfo *theInfo;
	Boolean display;
	FSSpec spec;

	display = true;
	theInfo = (NavFileOrFolderInfo *) info;
	if (theItem->descriptorType == typeFSS && !theInfo->isFolder) {
		AEGetDescData(theItem, &spec, sizeof(spec));
		display = IsGraphicFile(&spec);
	}
	return (display);
}


void SetImageWellFSSpec(ControlHandle tabControl, TabObjectPtr objectPtr,
			FSSpec * spec)
{
	Str255 url;

	if (objectPtr) {
		MakeFileURL(url, spec, 0);
		if (SetPictureObjectValue
		    (tabControl, objectPtr, &url[1], *url))
			PeepingTom(tabControl);
	}
}

Boolean MutuallyExclusiveCheckboxProc(TabObjectPtr objectPtr,
				      TabObjectPtr hitObjectPtr)
{
	if (objectPtr != hitObjectPtr)
		if (((*hitObjectPtr->behavior)->flags & behaveMatchTag)
		    && StringSame(hitObjectPtr->tag, objectPtr->tag))
			SetControlValue(objectPtr->control,
					1 -
					GetControlValue(hitObjectPtr->
							control));
	return (false);
}


Boolean SwapTabObjectsProc(TabObjectPtr tabOneObjectPtr,
			   ControlHandle tabPaneTwo)
{
	TabObjectPtr tabTwoObjectPtr;
	Str255 contrlTitle;

	if (tabOneObjectPtr->control) {
		GetControlTitle(tabOneObjectPtr->control, contrlTitle);
		if (tabTwoObjectPtr =
		    FindObjectWithTitleOnTab(tabPaneTwo, contrlTitle)) {
			SwapObjectValues(tabOneObjectPtr, tabTwoObjectPtr);
			UnlockObject(tabTwoObjectPtr);
		}
	}
	return (false);
}


void ClearTab(ControlHandle tabControl)
{
	IterateTabObjectsUL(tabControl, ClearTabProc, tabControl);
}

Boolean ClearTabProc(TabObjectPtr objectPtr, ControlHandle tabControl)
{
	switch (objectPtr->type) {
	case fieldObject:
		PeteSetTextPtr(GetLabelFieldPete(objectPtr->control), "",
			       0);
		break;
	case controlObject:
	case controlResourceObject:
		SetControlValue(objectPtr->control, 0);
		break;
	case pictureObject:
		SetPictureObjectValue(tabControl, objectPtr, nil, 0);
		break;
	}
	return (false);
}

TabFieldHandle GetTabFieldStrings(ControlHandle tabControl,
				  TagFieldStringType type)
{
	GetTabFieldsRec gtf;

	gtf.type = type;
	if (gtf.fields = NuHandle(0)) {
		IterateTabObjectsUL(tabControl, GetTabFieldStringsProc,
				    &gtf);
		if (!GetHandleSize(gtf.fields))
			ZapHandle(gtf.fields);
	}
	return (gtf.fields);
}

Boolean GetTabFieldStringsProc(TabObjectPtr objectPtr,
			       GetTabFieldsPtr gtfp)
{
	Str255 name;
	OSErr theError;

	*name = 0;
	switch (gtfp->type) {
	case tabTagName:
		PSCopy(name, objectPtr->tag);
		break;
	case tagFieldName:
		if (objectPtr->control)
			GetControlTitle(objectPtr->control, name);
		break;
	case tagShortName:
		PSCopy(name, objectPtr->shortName);
		break;
	}
	theError = PtrPlusHand(name, gtfp->fields, *name + 1);
	if (!theError)
		theError = PtrPlusHand("", gtfp->fields, 1);
	if (theError) {
		ZapHandle(gtfp->fields);
		return (true);
	}
	return (false);
}


TabFieldHandle GetExportableTabFieldStrings(ControlHandle tabControl)
{
	TabFieldHandle fields;

	if (fields = NuHandle(0)) {
		IterateTabObjectsUL(tabControl,
				    GetExportableTabFieldStringsProc,
				    &fields);
		if (!GetHandleSize(fields))
			ZapHandle(fields);
	}
	return (fields);
}

Boolean GetExportableTabFieldStringsProc(TabObjectPtr objectPtr,
					 TabFieldHandle * fields)
{
	Str255 name;
	OSErr theError;

	if (!(objectPtr->flags & objectFlagExportable))
		return (false);

	*name = 0;
	PSCopy(name, objectPtr->tag);
	theError = PtrPlusHand(name, *fields, *name + 1);
	if (!theError)
		theError = PtrPlusHand("", *fields, 1);
	if (theError) {
		ZapHandle(*fields);
		return (true);
	}
	return (false);
}

//
//      GetTabFieldsFromResources
//
//              Brute force method of getting all the tab field data we could ever want
//              when the address book is not open.
//
//              Note:  Don't pass in tagFieldName... you won't get nothin'
//

TabFieldHandle GetTabFieldsFromResources(TagFieldStringType type,
					 OSType applOwner, OSType tabOwner)
{
	TabResHeaderRec tabResHeader;
	GetTabFieldsRec gtf;
	TabHandle allTabs;
	TabRec newTabRec;
	TabObjectRec object;
	Handle newTab;
	Ptr from;
	OSErr theError;
	short tabIndex, objectIndex;

	theError = noErr;
	gtf.type = type;
	if (gtf.fields = NuHandle(0)) {
		// Allocate some space for the tab data
		allTabs = NuHandle(0);
		if (!allTabs)
			return (gtf.fields);

		// Scanning the globe for 'eTab' resources
		tabIndex = 1;
		do {
			if (newTab = GetIndResource(TAB_TYPE, tabIndex++)) {
				// Parse the tab header
				from = LDRef(newTab);
				from = ParseTabHeader(from, &tabResHeader);

				// Only look at interesting tabs
				if (InterestingTab
				    (allTabs, &tabResHeader, applOwner,
				     tabOwner)) {
					newTabRec.tabSignature =
					    tabResHeader.tabSignature;
					newTabRec.defaultHorzMargin =
					    tabResHeader.defaultHorzMargin;
					newTabRec.defaultVertMargin =
					    tabResHeader.defaultVertMargin;
					newTabRec.numObjects =
					    tabResHeader.numObjects;
					newTabRec.objects = nil;
					newTabRec.focusObject =
					    kNoTabObjectFocus;
					theError =
					    PtrPlusHand_(&newTabRec,
							 allTabs,
							 sizeof
							 (newTabRec));

					// Fetch all of the objects on this tab
					for (objectIndex = 0;
					     objectIndex <
					     tabResHeader.numObjects
					     && !theError; ++objectIndex) {
						object.control = nil;
						object.behavior = nil;
						from =
						    ParseObject(nil, from,
								&object);
						GetTabFieldStringsProc
						    (&object, &gtf);
					}
				}
				UL(newTab);
				ReleaseResource(newTab);
			}
		} while (newTab && !theError);
		ZapHandle(allTabs);
		if (!GetHandleSize(gtf.fields))
			ZapHandle(gtf.fields);
	}
	return (gtf.fields);
}

Boolean FindFieldString(TabFieldHandle fields, PStr key)
{
	UPtr spot, end;

	spot = *fields;
	end = spot + GetHandleSize(fields);

	while (spot < end) {
		if (StringSame(spot, key))
			return (true);
		spot += (*spot + 2);
	}
	return (false);
}


void CleanTab(ControlHandle tabControl)
{
	ControlHandle tabPane;
	TabPtr tabPtr;
	short tabIndex, numTabs;

	numTabs = CountTabs(GetTabHandle(tabControl));
	for (tabIndex = 1; tabIndex <= numTabs; ++tabIndex) {
		tabPtr = GetTabPtr(tabControl, tabIndex - 1);
		tabPtr->dirty = false;
		GetIndexedSubControl(tabControl, tabIndex, &tabPane);
		IterateTabPaneObjectsUL(tabPane, CleanTabPaneProc, nil);
	}
}


Boolean CleanTabPaneProc(TabObjectPtr objectPtr, void *private)
{
	if (objectPtr->type == fieldObject)
		PETEMarkDocDirty(PETE,
				 GetLabelFieldPete(objectPtr->control),
				 false);
	return (false);
}


Boolean IsTabDirty(ControlHandle tabControl)
{
	ControlHandle tabPane;
	TabPtr tabPtr;
	short tabIndex, numTabs;
	Boolean dirty;

	dirty = false;
	if (tabControl) {
		numTabs = CountTabs(GetTabHandle(tabControl));
		for (tabIndex = 1; !dirty && tabIndex <= numTabs;
		     ++tabIndex) {
			tabPtr = GetTabPtr(tabControl, tabIndex - 1);
			if (!(dirty = tabPtr->dirty)) {
				GetIndexedSubControl(tabControl, tabIndex,
						     &tabPane);
				IterateTabPaneObjectsUL(tabPane,
							DirtyTabPaneProc,
							&dirty);
			}
		}
	}
	return (dirty);
}


Boolean DirtyTabPaneProc(TabObjectPtr objectPtr, Boolean * dirty)
{
	if (objectPtr->type == fieldObject)
		if (*dirty =
		    PeteIsDirty(GetLabelFieldPete(objectPtr->control)) ?
		    true : false)
			return (true);
	return (false);
}




void HideTab(ControlHandle tabPane)
{
	Rect tabRect;
	Boolean showIt;

	// Hide the user pane (or not if the Appearance Manager is just stupid)
	if (gBetterAppearance) {
		Rect rClipZero;

		HideControl(tabPane);

		//      clip to nothing so we don't get controls leaving behind residues when they move
		SetPort(GetWindowPort(GetControlOwner(tabPane)));
		SetRect(&rClipZero, 0, 0, 0, 0);
		ClipRect(&rClipZero);
	}
	// Move all of the embedded objects off screen
	GetControlBounds(tabPane, &tabRect);
	MoveTabPane(tabPane, -REAL_BIG, -REAL_BIG, RectWi(tabRect),
		    RectHi(tabRect));

	// Allow objects to do their own hiding
	showIt = false;
	IterateTabPaneObjectsUL(tabPane, ShowHideProc, &showIt);

	if (gBetterAppearance)
		InfiniteClip(GetWindowPort(GetControlOwner(tabPane)));	//      restore clip region
}


void ShowTab(ControlHandle tabPane)
{
	ControlHandle tabControl;
	Rect tabRect;
	Boolean showIt;

	GetSuperControl(tabPane, &tabControl);

	if (tabControl) {
		GetTabContentRect(tabControl, &tabRect);

		// Move all of the embedded objects back onto the screen
		MoveTabPane(tabPane, tabRect.left, tabRect.top,
			    RectWi(tabRect), RectHi(tabRect));

		// Show the user pane after everything has been shifted around
		ShowControl(tabPane);

		// Allow objects to show themselves on their own
		showIt = true;
		IterateTabPaneObjectsUL(tabPane, ShowHideProc, &showIt);
	}
}


//
//      ShowHideProc  --  (ObjectIterateProc)
//

Boolean ShowHideProc(TabObjectPtr objectPtr, Boolean * showIt)
{
	PETEHandle pte;

	switch (objectPtr->type) {
	case fieldObject:
		if (objectPtr->control)
			if (pte = GetLabelFieldPete(objectPtr->control))
				if ((*PeteExtra(pte))->isInactive =
				    !*showIt)
					DeactivateControl(objectPtr->
							  control);
				else
					ActivateControl(objectPtr->
							control);
		break;
	case pictureObject:
		if (*showIt)
			DisplayPictureObject(objectPtr);
		break;
	}
	return (false);
}


void MoveTab(ControlHandle tabControl, int h, int v, int w, int t)
{
	short tabHeight;

//      if (GetThemeMetric (kThemeMetricSmallTabHeight, &tabHeight)) // Do this for Carbon eventually...
	tabHeight = kThemeSmallTabHeightMax;

	// First, resize the tab itself (this might be causing the extra update we see...)
	MoveMyCntl(tabControl, h, v, w, t);

	// Then all the soft and squishy stuff inside (effectively inset into the tab itself)
	MoveTabPane(GetActiveTabUserPane(tabControl), h + 1,
		    v + 1 + tabHeight, w - 2, t - 2 - tabHeight);
}



void MoveTabPane(ControlHandle tabPane, short int h, int v, int w, int t)
{
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	ControlHandle tabControl;
	Rect relevantObjectBounds, bounds;
	short objectIndex, lineHeight, dh, dv;
	SignedByte oldTabHState, oldObjectHState;
	Rect rCntl;

	if (!tabPane)
		return;

	GetSuperControl(tabPane, &tabControl);

	// Resize the user pane itself
	MoveMyCntl(tabPane, h, v, w, t);

	// We might need this, so calculate it once
	lineHeight =
	    ONE_LINE_HI(GetWindowMyWindowPtr(GetControlOwner(tabControl)));

	// Okay... Lets position everything inside this tab (yikes...)
	tabPtr =
	    LockTabs(tabControl, GetTabPaneIndex(tabPane), &oldTabHState);
	for (objectIndex = 0; objectIndex < tabPtr->numObjects;
	     ++objectIndex) {

		// Point to the object
		objectPtr =
		    LockTabObjects(tabPtr, objectIndex, &oldObjectHState);

		// Calculate each of the relevant coordinates for this object
		bounds.left =
		    CalcRelevantCoordinate(tabPane, tabPtr,
					   &objectPtr->position.left);
		bounds.top =
		    CalcRelevantCoordinate(tabPane, tabPtr,
					   &objectPtr->position.top);
		bounds.right =
		    CalcRelevantCoordinate(tabPane, tabPtr,
					   &objectPtr->position.right);
		bounds.bottom =
		    CalcRelevantCoordinate(tabPane, tabPtr,
					   &objectPtr->position.bottom);

		// Some coordinates might still be undefined, in which case we rely upon height or width
		if (bounds.left == kUndefinedCoordinate
		    && bounds.right != kUndefinedCoordinate)
			bounds.left = bounds.right - WidthHint(objectPtr);
		if (bounds.top == kUndefinedCoordinate
		    && bounds.bottom != kUndefinedCoordinate)
			bounds.top =
			    bounds.bottom - HeightHint(objectPtr,
						       lineHeight);
		if (bounds.right == kUndefinedCoordinate
		    && bounds.left != kUndefinedCoordinate)
			bounds.right = bounds.left + WidthHint(objectPtr);
		if (bounds.bottom == kUndefinedCoordinate
		    && bounds.top != kUndefinedCoordinate)
			bounds.bottom =
			    bounds.top + HeightHint(objectPtr, lineHeight);

		// If we are STILL missing some coordinates and we have a control, just use the control bounds
		if (objectPtr->control) {
			GetControlBounds(objectPtr->control, &rCntl);
			if (bounds.left == kUndefinedCoordinate
			    || bounds.right == kUndefinedCoordinate) {
				bounds.left = rCntl.left;
				bounds.right = rCntl.right;
			}
			if (bounds.top == kUndefinedCoordinate
			    || bounds.bottom == kUndefinedCoordinate) {
				bounds.top = rCntl.top;
				bounds.bottom = rCntl.bottom;
			}
		}
		// Any special alignment?
		dh = 0;
		dv = 0;
		if (objectPtr->position.horizontal.isRelevant) {
			GetRelevantObjectBounds(tabControl, tabPtr,
						objectPtr->position.
						horizontal.object,
						objectPtr->position.
						horizontal.flags,
						&relevantObjectBounds);
			switch (objectPtr->position.horizontal.align) {
			case alignLeft:
				dh = relevantObjectBounds.left -
				    bounds.left;
				break;
			case alignCenter:
				dh = relevantObjectBounds.left +
				    (RectWi(relevantObjectBounds) -
				     RectWi(bounds)) / 2 - bounds.left;
				break;
			case alignRight:
				dh = relevantObjectBounds.right -
				    RectWi(bounds) - bounds.left;
				break;
			}
		}
		if (objectPtr->position.vertical.isRelevant) {
			GetRelevantObjectBounds(tabControl, tabPtr,
						objectPtr->position.
						vertical.object,
						objectPtr->position.
						vertical.flags,
						&relevantObjectBounds);
			switch (objectPtr->position.vertical.align) {
			case alignTop:
				dv = relevantObjectBounds.top - bounds.top;
				break;
			case alignCenter:
				dv = relevantObjectBounds.top +
				    (RectHi(relevantObjectBounds) -
				     RectHi(bounds)) / 2 - bounds.top;
				break;
			case alignBottom:
				dv = relevantObjectBounds.bottom -
				    RectWi(bounds) - bounds.bottom;
				break;
			}
		}
		if (dh || dv)
			OffsetRect(&bounds, dh, dv);

		UnlockTabObjects(tabPtr, oldObjectHState);

		// Move the object based on type and it's new bounding rectangle
		switch (objectPtr->type) {
		case fieldObject:
			MoveLabelField(objectPtr->control, bounds.left,
				       bounds.top,
				       bounds.right - bounds.left,
				       bounds.bottom - bounds.top);
			GetControlBounds(objectPtr->control, &bounds);
			break;
		case controlObject:
		case controlResourceObject:
		case pictureObject:
			MoveMyCntl(objectPtr->control, bounds.left,
				   bounds.top, bounds.right - bounds.left,
				   bounds.bottom - bounds.top);
			break;
		}

		// Save it for later... (The English Beat)
		objectPtr->bounds = bounds;
	}
	UnlockTabs(tabControl, oldTabHState);
}


//
//      GetActiveTabUserPane
//
//              Get the user pane for the active tab.  This is the embedder for all the objects contained
//              within a single tab pane.

ControlHandle GetActiveTabUserPane(ControlHandle tabControl)
{
	ControlHandle userPane;

	if (GetIndexedSubControl
	    (tabControl, GetActiveTabIndex(tabControl), &userPane))
		userPane = nil;
	return (userPane);
}


//
//      SwitchTab
//
//              Switch from one tab to another in a tabbed control
//

short SwitchTabs(ControlHandle tabControl, short currentTabIndex,
		 short newTabIndex, short tabPrefResID)
{
	ControlHandle currentUserPane, newUserPane;
	Str255 name;

	currentUserPane = nil;
	newUserPane = nil;
	if (newTabIndex != currentTabIndex) {
		if (currentTabIndex)
			GetIndexedSubControl(tabControl, currentTabIndex,
					     &currentUserPane);
		GetIndexedSubControl(tabControl, newTabIndex,
				     &newUserPane);

		if (currentUserPane)
			HideTab(currentUserPane);
		if (newUserPane)
			ShowTab(newUserPane);

		GetControlTitle(newUserPane, name);
		SetPref(tabPrefResID, name);
	}
	return (newTabIndex);
}

//
//      CalcRelevantCoordinate
//
//              Calculate a coordinate based on some relevancy to aanother object's coorinate.
//              For example, 24 pixels from the right coordinate of some previous object.
//

short CalcRelevantCoordinate(ControlHandle tabPane, TabPtr tabPtr,
			     RelativeToPtr relative)
{
	Rect relevantObjectBounds;
	short coordinate;

	coordinate = kUndefinedCoordinate;

	if (relative->isRelevant) {
		// Get the bounds of the object relevant to this coordinate
		GetRelevantObjectBounds(tabPane, tabPtr, relative->object,
					relative->flags,
					&relevantObjectBounds);
		switch (relative->relevantTo) {
		case relToLeft:
			coordinate =
			    relevantObjectBounds.left +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultHorzMargin);
			break;
		case relToTop:
			coordinate =
			    relevantObjectBounds.top +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultVertMargin);
			break;
		case relToRight:
			coordinate =
			    relevantObjectBounds.right +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultHorzMargin);
			break;
		case relToBottom:
			coordinate =
			    relevantObjectBounds.bottom +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultVertMargin);
			break;
		case relToHorzCenter:
			coordinate =
			    relevantObjectBounds.left +
			    (relevantObjectBounds.right -
			     relevantObjectBounds.left) / 2 +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultHorzMargin);
			break;
		case relToVertCenter:
			coordinate =
			    relevantObjectBounds.top +
			    (relevantObjectBounds.bottom -
			     relevantObjectBounds.top) / 2 +
			    GetRelativeToDelta(relative,
					       tabPtr->defaultVertMargin);
			break;
		case relToWidth:
			coordinate =
			    relevantObjectBounds.left +
			    (relevantObjectBounds.right -
			     relevantObjectBounds.left) * relative->delta /
			    100;
			break;
		case relToHeight:
			coordinate =
			    relevantObjectBounds.top +
			    (relevantObjectBounds.bottom -
			     relevantObjectBounds.top) * relative->delta /
			    100;
			break;
		}
	}
	return (coordinate);
}


short GetRelativeToDelta(RelativeToPtr relative, short defaultMargin)
{
	return (relative->
		flags & rfDefaultMargin ? (relative->
					   flags & rfNegativeMargin ?
					   -defaultMargin : defaultMargin)
		: relative->delta);
}


//
//      GetRelevantObjectBounds
//
//              Get the bounding rectangle of another object or, in the case of the tab itself, the user pane
//              into which a tab's objects are embedded.
//

Rect *GetRelevantObjectBounds(ControlHandle tabPane, TabPtr tabPtr,
			      short object, RelativeFlagsType flags,
			      Rect * boundsRect)
{
	TabObjectPtr objectPtr;
	SignedByte oldObjectHState;

	if (object == kTabObject)
		GetControlBounds(tabPane, boundsRect);
	else {
		objectPtr =
		    LockTabObjects(tabPtr, object, &oldObjectHState);
		*boundsRect = objectPtr->bounds;
		if (objectPtr->type == fieldObject
		    && (flags & (rfPETEPart | rfLabelPart)))
			GetRelevantLabelFieldBounds(objectPtr->control,
						    flags, boundsRect);
		UnlockTabObjects(tabPtr, oldObjectHState);
	}
	return (boundsRect);
}


//
//      ParseTabHeader
//
//              Parse the tab header information out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                        2                                     Version
//                                              4                                       Application owner signature
//                                              4                                       Tab Group signature
//                                              4                                       Tab Resource signature
//                                      PStr                            Tab name
//                                              2                                       Icon suite ID
//                                              2                                       Default horizontal margin
//                                              2                                       Default vertical margin
//                                              2                                       Number of objects on this tab
//

Ptr ParseTabHeader(Ptr from, TabResHeaderPtr tabHeader)
{
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->version,
				sizeof(tabHeader->version));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->applOwner,
				sizeof(tabHeader->applOwner));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->tabOwner,
				sizeof(tabHeader->tabOwner));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->tabSignature,
				sizeof(tabHeader->tabSignature));
	from = CopyBytesAndMovePtr(from, &tabHeader->title, *from + 1);
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->iconSuiteID,
				sizeof(tabHeader->iconSuiteID));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->defaultHorzMargin,
				sizeof(tabHeader->defaultHorzMargin));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->defaultVertMargin,
				sizeof(tabHeader->defaultVertMargin));
	from =
	    CopyBytesAndMovePtr(from, &tabHeader->numObjects,
				sizeof(tabHeader->numObjects));
	return (from);
}


//
//      ParseObject
//
//              Parse object information out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                        2                                     Object type
//                                        2                                     Tabbing order index for this tab object (1 based)
//                                        :                                     Each object
//                                              2                                       Number of relative positions described for this object
//                                        n                                     Each position
//

Ptr ParseObject(ControlHandle tabControl, Ptr from, TabObjectPtr objectPtr)
{
	short numPositions, positionIndex;

	from =
	    CopyBytesAndMovePtr(from, &objectPtr->type,
				sizeof(objectPtr->type));
	from =
	    CopyBytesAndMovePtr(from, &objectPtr->flags,
				sizeof(objectPtr->flags));
	switch (objectPtr->type) {
	case fieldObject:
		from = ParseFieldObject(tabControl, from, objectPtr);
		break;
	case controlObject:
		from = ParseControlObject(tabControl, from, objectPtr);
		break;
	case controlResourceObject:
		from =
		    ParseControlResourceObject(tabControl, from,
					       objectPtr);
		break;
	case pictureObject:
		from = ParsePictureObject(tabControl, from, objectPtr);
		break;
	default:
		// Uh oh... We better at least be smart enough here to move the pointer to the
		// end of this object...
		break;
	}
	// Should be pointing at the position descriptions now
	from =
	    CopyBytesAndMovePtr(from, &numPositions, sizeof(numPositions));
	// Fetch all of the positions descriptions for this object
	for (positionIndex = 0; positionIndex < numPositions;
	     ++positionIndex)
		from = ParsePosition(from, &objectPtr->position);
	return (from);
}

//
//      ParsePosition
//
//              Parse object information out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                        2                                     Coordinate type
//      left, top
//      right, bottom
//                                        2                                     Delta
//                                              2                                       relative object
//                                        2                                     relative position
//                                              4                                       Relativity flags
//      height, width
//                                              2                                       value
//      horizontal, vertical
//                                              2                                       align enum
//                                              2                                       relative object
//                                              4                                       Relativity flags
//

Ptr ParsePosition(Ptr from, ObjectPositionPtr positionPtr)
{
	short coordinate;

	from = CopyBytesAndMovePtr(from, &coordinate, sizeof(coordinate));
	switch (coordinate) {
	case left:
		from = ParseRelativeInformation(from, &positionPtr->left);
		break;
	case top:
		from = ParseRelativeInformation(from, &positionPtr->top);
		break;
	case right:
		from = ParseRelativeInformation(from, &positionPtr->right);
		break;
	case bottom:
		from =
		    ParseRelativeInformation(from, &positionPtr->bottom);
		break;
	case height:
		from =
		    CopyBytesAndMovePtr(from, &positionPtr->height,
					sizeof(positionPtr->height));
		break;
	case width:
		from =
		    CopyBytesAndMovePtr(from, &positionPtr->width,
					sizeof(positionPtr->width));
		break;
	case horizontal:
		from =
		    ParseAlignmentInformation(from,
					      &positionPtr->horizontal);
		break;
	case vertical:
		from =
		    ParseAlignmentInformation(from,
					      &positionPtr->vertical);
		break;
	default:
		// Uh oh... We better at least be smart enough here to move the pointer to the
		// end of this object...
		break;
	}
	return (from);
}


//
//      ParseRelativeInformation
//
//              Parse object relation information out of an 'eTAB' resource.
//

Ptr ParseRelativeInformation(Ptr from, RelativeToPtr relativePtr)
{
	relativePtr->isRelevant = true;
	from =
	    CopyBytesAndMovePtr(from, &relativePtr->delta,
				sizeof(relativePtr->delta));
	from =
	    CopyBytesAndMovePtr(from, &relativePtr->object,
				sizeof(relativePtr->object));
	from =
	    CopyBytesAndMovePtr(from, &relativePtr->relevantTo,
				sizeof(relativePtr->relevantTo));
	from =
	    CopyBytesAndMovePtr(from, &relativePtr->flags, sizeof(uLong));
	return (from);
}


//
//      ParseAlignmentInformation
//
//              Parse object relation information out of an 'eTAB' resource.
//

Ptr ParseAlignmentInformation(Ptr from, AlignToptr alignPtr)
{
	alignPtr->isRelevant = true;
	from =
	    CopyBytesAndMovePtr(from, &alignPtr->align,
				sizeof(alignPtr->align));
	from =
	    CopyBytesAndMovePtr(from, &alignPtr->object,
				sizeof(alignPtr->object));
	from = CopyBytesAndMovePtr(from, &alignPtr->flags, sizeof(uLong));
	return from;
}

//
//      ParseFieldObject
//
//              Parse information about a Labeled Field out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                      PStr                            Tag used to identify this object
//                                      PStr                            Label for this field
//                                      PStr                            Short name for this field
//                                              2                                       Label width
//                                              2                                       Label justification
//                                              4                                       Label flags
//                                              4                                       PETE flags
//

Ptr ParseFieldObject(ControlHandle tabControl, Ptr from,
		     TabObjectPtr objectPtr)
{
	MyWindowPtr win;
	PETEDocInitInfo pdi;
	Str255 tag, title, shortName;
	uLong pteFlags, labelFlags;
	short labelWidth, labelJustification;

	from = CopyBytesAndMovePtr(from, tag, *from + 1);
	from = CopyBytesAndMovePtr(from, title, *from + 1);
	from = CopyBytesAndMovePtr(from, shortName, *from + 1);
	from = CopyBytesAndMovePtr(from, &labelWidth, sizeof(labelWidth));
	from =
	    CopyBytesAndMovePtr(from, &labelJustification,
				sizeof(labelJustification));
	from = CopyBytesAndMovePtr(from, &labelFlags, sizeof(labelFlags));
	from = CopyBytesAndMovePtr(from, &pteFlags, sizeof(pteFlags));

	PSCopy(objectPtr->tag, tag);
	PSCopy(objectPtr->shortName, shortName);

	if (tabControl) {
		win = GetWindowMyWindowPtr(GetControlOwner(tabControl));
		if (labelFlags & labelDisplayAboveField
		    && labelWidth <= kUseOneLineHeight)
			labelWidth = -labelWidth * ONE_LINE_HI(win);

		DefaultPII(win, false, pteFlags, &pdi);
		if (objectPtr->control =
		    CreateLabelField(win, nil, title, labelWidth,
				     labelJustification, labelFlags, &pdi,
				     pteFlags))
			EmbedControl(objectPtr->control, tabControl);
		CleanPII(&pdi);
	}
	return (from);
}

//
//      ParseControlObject
//
//              Parse information about a control out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                      PStr                            Tag used to identify this object
//                                      PStr                            Title for the control
//                                      PStr                            Short name for this field
//                                              2                                       Initial value
//                                              2                                       Initial minimum
//                                              2                                       Initial maximum
//                                        2                                     procID we'll use to create the object
//                                              4                                       refcon
//                                              4                                       control flags
//                                              4                                       behavior flags
//                                      PStr                            name of one of the objects acted upon by this behavior
//                                      PStr                            name of the other object acted upon by this behavior
//

Ptr ParseControlObject(ControlHandle tabControl, Ptr from,
		       TabObjectPtr objectPtr)
{
	ControlObjectFlagsType controlFlags;
	BehaviorRec behave;
	Str255 tag, title, shortName;
	Rect siberia;
	long refcon;
	short value, min, max, procID;

	SetRect(&siberia, -20, -20, -10, -10);

	from = CopyBytesAndMovePtr(from, tag, *from + 1);
	from = CopyBytesAndMovePtr(from, title, *from + 1);
	from = CopyBytesAndMovePtr(from, shortName, *from + 1);
	from = CopyBytesAndMovePtr(from, &value, sizeof(value));
	from = CopyBytesAndMovePtr(from, &min, sizeof(min));
	from = CopyBytesAndMovePtr(from, &max, sizeof(max));
	from = CopyBytesAndMovePtr(from, &procID, sizeof(procID));
	from = CopyBytesAndMovePtr(from, &refcon, sizeof(refcon));
	from =
	    CopyBytesAndMovePtr(from, &controlFlags, sizeof(controlFlags));
	from =
	    CopyBytesAndMovePtr(from, &behave.flags, sizeof(behave.flags));
	from = CopyBytesAndMovePtr(from, behave.stringOne, *from + 1);
	from = CopyBytesAndMovePtr(from, behave.stringTwo, *from + 1);

	PSCopy(objectPtr->tag, tag);
	PSCopy(objectPtr->shortName, shortName);

	if (tabControl)
		if (objectPtr->control =
		    NewControlSmall(GetControlOwner(tabControl), &siberia,
				    title, true, value, min, max, procID,
				    refcon)) {
			EmbedControl(objectPtr->control, tabControl);
			if (controlFlags & coFit)
				ButtonFit(objectPtr->control);
			if (behave.flags)
				PtrToHand(&behave, &objectPtr->behavior,
					  sizeof(behave));
		}
	return (from);
}


//
//      ParseControlResourceObject
//
//              Parse information about a 'CNTL' resource out of an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                      PStr                            Tag used to identify this object
//                                      PStr                            Short name for this field
//                                        2                                     'CNTL' resource ID
//                                              4                                       behavior flags
//                                      PStr                            name of one of the objects acted upon by this behavior
//                                      PStr                            name of the other object acted upon by this behavior
//

Ptr ParseControlResourceObject(ControlHandle tabControl, Ptr from,
			       TabObjectPtr objectPtr)
{
	BehaviorRec behave;
	Str255 tag, shortName;
	short controlID;

	from = CopyBytesAndMovePtr(from, tag, *from + 1);
	from = CopyBytesAndMovePtr(from, shortName, *from + 1);
	from = CopyBytesAndMovePtr(from, &controlID, sizeof(controlID));
	from =
	    CopyBytesAndMovePtr(from, &behave.flags, sizeof(behave.flags));
	from = CopyBytesAndMovePtr(from, behave.stringOne, *from + 1);
	from = CopyBytesAndMovePtr(from, behave.stringTwo, *from + 1);

	PSCopy(objectPtr->tag, tag);
	PSCopy(objectPtr->shortName, shortName);

	if (tabControl)
		if (objectPtr->control =
		    GetNewControl(controlID,
				  GetControlOwner(tabControl))) {
			EmbedControl(objectPtr->control, tabControl);
			if (behave.flags)
				PtrToHand(&behave, &objectPtr->behavior,
					  sizeof(behave));
		}
	return (from);
}


//
//      ParsePictureObject
//
//              Parse information about a picture an 'eTAB' resource.
//
//                               Bytes                           Contents
//                              -------                         ----------
//                                      PStr                            Tag used to identify this object
//                                      PStr                            Short name for this field
//                                      FlavorType      Drag flavor
//
//              Picture objects store a StringHandle in the refcon field of the object control.  This
//              StringHandle contains a URL to the picture to be displayed.  For now, this is a simple
//              file: URL pointing into the Photo Album folder.
//

Ptr ParsePictureObject(ControlHandle tabControl, Ptr from,
		       TabObjectPtr objectPtr)
{
	FlavorType dragFlavor;
	StringHandle urlString;
	Str255 tag, shortName;
	Rect siberia;

	SetRect(&siberia, -20, -20, -10, -10);

	from = CopyBytesAndMovePtr(from, tag, *from + 1);
	from = CopyBytesAndMovePtr(from, shortName, *from + 1);
	from = CopyBytesAndMovePtr(from, &dragFlavor, sizeof(dragFlavor));

	PSCopy(objectPtr->tag, tag);
	PSCopy(objectPtr->shortName, shortName);
	objectPtr->dragFlavor = dragFlavor;

	if (tabControl)
		if (urlString = NuHandle(0))
			if (objectPtr->control =
			    NewControlSmall(GetControlOwner(tabControl),
					    &siberia, "\p", true, 0,
					    kControlContentPictHandle, 0,
					    kControlImageWellProc,
					    (long) urlString))
				EmbedControl(objectPtr->control,
					     tabControl);
	return (from);
}


void *CopyBytesAndMovePtr(void *from, void *to, short len)
{
	BMD(from, to, len);
	return ((Ptr) from + len);
}


short WidthHint(TabObjectPtr objectPtr)
{
	short width;

	if (objectPtr->position.width)
		width = objectPtr->position.width;
	else
		width =
		    objectPtr->control ? ControlWi(objectPtr->
						   control) : 80;
	return (width);
}

short HeightHint(TabObjectPtr objectPtr, short lineHeight)
{
	short height;

	height =
	    GetRequestedHeight(objectPtr->position.height, lineHeight);
	if (!height)
		height =
		    objectPtr->control ? ControlHi(objectPtr->
						   control) : lineHeight;
	return (height);
}



ControlHandle FindTabPaneWithTitle(ControlHandle tabControl, PStr title)
{
	return (IterateTabPanes
		(tabControl, FindTabPaneWithTitleProc, title));
}


Boolean FindTabPaneWithTitleProc(ControlHandle tabControl,
				 ControlHandle tabPane, short tabIndex,
				 PStr title)
{
	ControlTabInfoRec tabInfo;
	Size actualSize;

	tabInfo.version = 0;
	if (!GetControlData
	    (tabControl, tabIndex, kControlTabInfoTag,
	     sizeof(ControlTabInfoRec), &tabInfo, &actualSize))
		if (StringSame(title, tabInfo.name))
			return (true);
	return (false);
}


//
//      FindObjectWithTag
//
//              Find an object with a particular tag on any pane of a tab control.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.
//

TabObjectPtr FindObjectWithTag(ControlHandle tabControl, PStr tag)
{
	return (IterateTabObjects(tabControl, FindObjectWithTagProc, tag));
}


Boolean FindObjectWithTagProc(TabObjectPtr objectPtr, PStr tag)
{
	return (StringSame(tag, objectPtr->tag));
}

PStr FindTitleWithTag(ControlHandle tabControl, PStr tag, PStr title)
{
	TabObjectPtr objectPtr;

	*title = 0;
	if (objectPtr = FindObjectWithTag(tabControl, tag)) {
		if (objectPtr->type == fieldObject)
			GetControlTitle(objectPtr->control, title);
		UnlockObject(objectPtr);
	}
	return (title);
}

PStr FindShortNameWithTag(ControlHandle tabControl, PStr tag,
			  PStr shortName)
{
	TabObjectPtr objectPtr;

	*shortName = 0;
	if (objectPtr = FindObjectWithTag(tabControl, tag)) {
		if (objectPtr->type == fieldObject)
			PCopy(shortName, objectPtr->shortName);
		UnlockObject(objectPtr);
	}
	return (shortName);
}

//
//      FindObjectWithTitle
//
//              Find a control object with a particular title on any pane of a tab control.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.
//

TabObjectPtr FindObjectWithTitle(ControlHandle tabControl, PStr title)
{
	return (IterateTabObjects
		(tabControl, FindObjectWithTitleProc, title));
}


Boolean FindObjectWithTitleProc(TabObjectPtr objectPtr, PStr title)
{
	Str255 contrlTitle;

	if (objectPtr->control) {
		GetControlTitle(objectPtr->control, contrlTitle);	// To avoid potential locking problems while iterating
		return (StringSame(title, contrlTitle));
	}
	return (false);
}

//
//      FindObjectWithShortName
//
//              Find a control object with a particular short name on any pane of a tab control.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.
//

TabObjectPtr FindObjectWithShortName(ControlHandle tabControl,
				     PStr shortName)
{
	return (IterateTabObjects
		(tabControl, FindObjectWithShortNameProc, shortName));
}


Boolean FindObjectWithShortNameProc(TabObjectPtr objectPtr, PStr shortName)
{
	return (StringSame(shortName, objectPtr->shortName));
}


//
//      FindObjectWithTagOnTab
//
//              Find an object with a particular tag on a particular pane of a tab control.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.
//

TabObjectPtr FindObjectWithTagOnTab(ControlHandle tabPane, PStr tag)
{
	return (IterateTabPaneObjects
		(tabPane, FindObjectWithTagProc, tag));
}


//
//      FindObjectWithTitleOnTab
//
//              Find an object with a particular title on a particular pane of a tab control.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.
//

TabObjectPtr FindObjectWithTitleOnTab(ControlHandle tabPane, PStr title)
{
	return (IterateTabPaneObjects
		(tabPane, FindObjectWithTitleProc, title));
}


//
//      FindFieldOnTab
//
//              Find a particular field on a tab pane.  Pass in the PETE your looking for, or
//              nil to find the first PETE on that pane.
//
//              Returns the PETE we found, or nil if no such PETE was found.
//

PETEHandle FindFieldOnTab(ControlHandle tabControl, short tabIndex,
			  PETEHandle pte)
{
	ControlHandle tabPane;
	TabObjectPtr objectPtr;

	GetIndexedSubControl(tabControl, tabIndex, &tabPane);
	if (objectPtr =
	    IterateTabPaneObjects(tabPane, FindFieldOnTabProc, &pte))
		UnlockObject(objectPtr);
	else
		pte = nil;
	return (pte);
}



//
//      FindFieldOnTabProc  --  (ObjectIterateProc)
//

Boolean FindFieldOnTabProc(TabObjectPtr objectPtr, PETEHandle * pteTarget)
{
	PETEHandle pte;

	if (objectPtr->type == fieldObject)
		if (objectPtr->control)
			if (pte = GetLabelFieldPete(objectPtr->control))
				if (pte == *pteTarget)
					return (true);	// We're looking for a particular field, and... Found it!
				else if (!*pteTarget) {	// If we're looking for the first PETE, we'll return it
					*pteTarget = pte;
					return (true);
				}
	return (false);
}


//
//      FindObjectWithFocus
//
//              Returns the object with the current focus, or 'nil' if no object has focus
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.

TabObjectPtr FindObjectWithFocus(ControlHandle tabControl)
{
	TabPtr tabPtr;
	ControlHandle tabPane;
	TabObjectPtr objectPtr;
	SignedByte oldTabHState, oldObjectHState;

	if (tabPane = GetActiveTabUserPane(tabControl)) {
		if (tabPtr =
		    LockTabs(tabControl, GetTabPaneIndex(tabPane),
			     &oldTabHState)) {
			if (objectPtr =
			    tabPtr->focusObject !=
			    kNoTabObjectFocus ? LockTabObjects(tabPtr,
							       tabPtr->
							       focusObject,
							       &oldObjectHState)
			    : nil) {
				objectPtr->lockedObjects = tabPtr->objects;
				objectPtr->lockedTabs =
				    GetTabHandle(tabControl);
				objectPtr->objectsHState = oldObjectHState;
				objectPtr->tabsHState = oldTabHState;
				return (objectPtr);
			}
			if (objectPtr)
				UnlockTabObjects(tabPtr, oldObjectHState);
		}
		UnlockTabs(tabControl, oldTabHState);
	}
	return (nil);
}


//
//      PtInTabObject and PtInTabObjectProc
//
//              Seems kind of silly to have two consecutive 1 line functions, doesn't it?
//              The first should probably be a macro, but the second is a proc to determine
//              whether or not a point appears in a particular object.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine.

TabObjectPtr PtInTabObject(ControlHandle tabPane, Point pt)
{
	return (IterateTabPaneObjects(tabPane, PtInTabObjectProc, &pt));
}

Boolean PtInTabObjectProc(TabObjectPtr objectPtr, Point * pt)
{
	return (PtInRect(*pt, &objectPtr->bounds));
}


TabObjectPtr SetObjectValueWithTag(ControlHandle tabControl, PStr tag,
				   UPtr value, long valueLength)
{
	TabObjectPtr objectPtr;
	Str255 valueStr;

	TrLo(value, valueLength, "\002", ">");
	if (objectPtr = FindObjectWithTag(tabControl, tag)) {
		switch (objectPtr->type) {
		case fieldObject:
			SetLabelFieldText(objectPtr->control, value,
					  valueLength);
			break;
		case controlObject:
			// We need to handle things like the 'primary' checkbox here
			if (objectPtr->behavior) {
				if ((*objectPtr->behavior)->
				    flags & behaveStringOneIsTrue) {
					MakePPtr(valueStr, value,
						 valueLength);
					SetControlValue(objectPtr->control,
							StringSame
							(valueStr,
							 (*objectPtr->
							  behavior)->
							 stringOne));
				}
				if ((*objectPtr->behavior)->
				    flags & behaveStringTwoIsFalse) {
					MakePPtr(valueStr, value,
						 valueLength);
					SetControlValue(objectPtr->control,
							StringSame
							(valueStr,
							 (*objectPtr->
							  behavior)->
							 stringTwo) ? 0 :
							1);
				}
			}
			break;
		case pictureObject:
			SetPictureObjectValue(tabControl, objectPtr, value,
					      valueLength);
			break;
		}
		UnlockObject(objectPtr);
	}
	TrLo(value, valueLength, ">", "\002");
	return (objectPtr);
}


//
//      SetPictureObjectValue
//
//              Picture objects keep a StringHandle containing the URL for the photo in the refcon
//              field of the object control.
//

Boolean SetPictureObjectValue(ControlHandle tabControl,
			      TabObjectPtr objectPtr, UPtr value,
			      long valueLength)
{
	ControlHandle tabPane;
	StringHandle urlString;
	OSErr theError;

	theError = ImageWellRemovePicture(objectPtr->control);
	if (urlString =
	    (StringHandle) GetControlReference(objectPtr->control)) {
		SetHandleBig(urlString, 0);
		theError = PtrPlusHand(value, urlString, valueLength);
	} else {
		theError = PtrToHand(value, &urlString, valueLength);
		if (!theError)
			SetControlReference(objectPtr->control,
					    (long) urlString);
	}

	// If the image well is on the active tab, go ahead and load'n'display the picture
	if (!theError)
		theError = GetSuperControl(objectPtr->control, &tabPane);
	if (!theError)
		if (GetTabPaneIndex(tabPane) ==
		    GetActiveTabIndex(tabControl) - 1)
			DisplayPictureObject(objectPtr);
	return (theError ? false : true);
}


void DisplayPictureObject(TabObjectPtr objectPtr)
{
	GraphicsImportComponent importer;
	ControlButtonContentInfo contentInfo;
	StringHandle urlString;
	FSSpec spec;
	OSErr theError;
	short oldResFile;

	contentInfo.contentType = kControlContentPictHandle;
	contentInfo.u.picture = nil;

	theError = noErr;
	urlString = (StringHandle) GetControlReference(objectPtr->control);
	if (urlString && GetHandleSize(urlString)) {
		// Make an FSSpec out of the URL stored with the picture object
		theError = URLStringToSpec(urlString, &spec);
		if (!theError) {
			oldResFile = CurResFile();
			// Give the graphic importers a first crack at making sense out of the file
			theError =
			    GetGraphicsImporterForFile(&spec, &importer);
			if (!theError) {
				SetupPNGTransparency(importer, &spec);
				theError =
				    GraphicsImportGetAsPicture(importer,
							       &contentInfo.
							       u.picture);
				CloseComponent(importer);
			}
			UseResFile(oldResFile);
		}
		// If the graphic importers failed, see if we can grab a 'PICT' resource from the file
		if (theError)
			if (contentInfo.u.picture = SpecResPicture(&spec))
				theError = noErr;

		// If we've gotten any error, but we are expecting to display a picture, use the broken picture icon
		if (theError) {
			contentInfo.contentType =
			    kControlContentIconSuiteRes;
			contentInfo.u.resID = MISSING_IMAGE_ICON;
			theError = noErr;
		}
	}
	// Place the image into the image well  
	if (!theError)
		theError =
		    SetImageWellContentInfo(objectPtr->control,
					    &contentInfo);

	// Force it to draw (since, for some reason, the Control Manager doesn't do this for us)
	if (!theError)
		Draw1Control(objectPtr->control);
}


OSErr ImageWellRemovePicture(ControlHandle theControl)
{
	PicHandle picture;
	OSErr theError;

	theError = noErr;
	if (theControl)
		if (picture = GetImageWellPicture(theControl)) {
			KillPicture(picture);
			theError = SetImageWellPicture(theControl, nil);
		}
	return (theError);
}

void ImageWellDrawFocus(ControlHandle theControl, Boolean hasFocus)
{
	MyWindowPtr win;
	Rect r;

	win = GetWindowMyWindowPtr(GetControlOwner(theControl));
	if (!win->isActive)
		hasFocus = false;
	GetControlBounds(theControl, &r);
	DrawThemeFocusRect(&r, hasFocus);
}


void ImageWellDrag(ControlHandle theControl, EventRecord * event)
{
	WindowPtr winWP = GetControlOwner(theControl);
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	GWorldPtr gworld;
	PicHandle picture;
	DragReference drag;
	RgnHandle rgn, drawRgn;
	Rect pictRect, picFrame;
	OSErr theError;
	Rect rCntl;

	if (MyWaitMouseMoved(event->where, true))
		if (!MyNewDrag(win, &drag)) {
			if (picture = GetImageWellPicture(theControl)) {
				theError =
				    AddDragItemFlavor(drag, 1,
						      kScrapFlavorTypePicture,
						      LDRef(picture),
						      GetHandleSize
						      (picture), 0);
				UL(picture);
			}
			if (!theError) {
				rgn = nil;
				gworld = nil;
				drawRgn = nil;
				picFrame = (*picture)->picFrame;
				GetControlBounds(theControl, &rCntl);
				pictRect.left =
				    rCntl.left + (ControlWi(theControl) -
						  RectWi(picFrame)) / 2;
				pictRect.top =
				    rCntl.top + (ControlHi(theControl) -
						 RectHi(picFrame)) / 2;
				pictRect.right =
				    pictRect.left + RectWi(picFrame);
				pictRect.bottom =
				    pictRect.top + RectHi(picFrame);
				if (GetPictureDragRgn
				    (win, picture, &pictRect, &rgn,
				     &gworld, &drawRgn, drag))
					MyTrackDrag(drag, event, rgn);
				if (rgn)
					DisposeRgn(rgn);
				if (gworld)
					DisposeGWorld(gworld);
				if (drawRgn)
					DisposeRgn(drawRgn);
			}
			MyDisposeDrag(drag);
		}
}

PicHandle GetImageWellPicture(ControlHandle theControl)
{
	ControlButtonContentInfo contentInfo;
	PicHandle picture;

	picture = nil;
	if (!GetImageWellContentInfo(theControl, &contentInfo))
		if (contentInfo.contentType == kControlContentPictHandle)
			picture = contentInfo.u.picture;
	return (picture);
}


OSErr SetImageWellPicture(ControlHandle theControl, PicHandle picture)
{
	ControlButtonContentInfo contentInfo;

	contentInfo.contentType = kControlContentPictHandle;
	contentInfo.u.picture = picture;
	return (SetImageWellContentInfo(theControl, &contentInfo));
}

Boolean GetPictureDragRgn(MyWindowPtr win, PicHandle picture,
			  Rect * pictRect, RgnHandle * phRgn,
			  GWorldPtr * pgworld, RgnHandle * phDrawRgn,
			  DragReference drag)
{
	CGrafPtr savePort;
	GDHandle saveDevice;
	GWorldPtr gworld;
	PixMapHandle imagePixMap;
	RgnHandle rgn, drawRgn;
	Rect picFrame;
	Point offsetPt;
	OSErr theError;
	long response;

	picFrame = (*picture)->picFrame;
	rgn = NewRgn();
	theError = MemError();
	if (!theError) {
		RectRgn(rgn, pictRect);
		if (!Gestalt(gestaltDragMgrAttr, &response)
		    && (response & (1L << gestaltDragMgrHasImageSupport)))
		{
			GetGWorld(&savePort, &saveDevice);
			theError =
			    NewGWorld(&gworld, 8, pictRect, nil, nil,
				      useTempMem);
			if (theError)
				theError =
				    NewGWorld(&gworld, 8, pictRect, nil,
					      nil, 0);
			if (!theError) {
				Rect rGWorld;
				imagePixMap = GetGWorldPixMap(gworld);

				//  draw the item into the GWorld
				SetGWorld(gworld, nil);
				LockPixels(imagePixMap);
				EraseRect(GetPortBounds(gworld, &rGWorld));
				DrawPicture(picture, pictRect);
				UnlockPixels(imagePixMap);
				SetGWorld(savePort, saveDevice);

				//  attach the image to the drag
				if (drawRgn = NewRgn()) {
					RectRgn(drawRgn, pictRect);
					SetPt(&offsetPt, 0, 0);
					SetPort(GetMyWindowCGrafPtr(win));
					LocalToGlobal(&offsetPt);
					theError =
					    SetDragImage(drag, imagePixMap,
							 drawRgn, offsetPt,
							 kDragDarkTranslucency
							 +
							 kDragRegionAndImage);
				}
			}
			SetGWorld(savePort, saveDevice);
		}
		OutlineRgn(rgn, 1);
		GlobalizeRgn(rgn);
	}
	*phRgn = rgn;
	*pgworld = gworld;
	*phDrawRgn = drawRgn;
	return (theError ? false : true);
}



//
//      IterateTabPanes
//
//              Iterate over all of the panes of a tab control, returning either ther ControlHandle of the
//              last pane we touched, or nil if we have exhausted all panes.
//

ControlHandle IterateTabPanes(ControlHandle tabControl,
			      TabPaneIterateProc tpiProc, void *private)
{
	ControlHandle tabPane;
	short tabIndex;

	tabIndex = 1;
	while (!GetIndexedSubControl(tabControl, tabIndex, &tabPane)) {
		if ((*tpiProc) (tabControl, tabPane, tabIndex, private))
			return (tabPane);
		++tabIndex;
	}
	return (nil);
}


//
//      IterateTabObjects
//
//              Iterate over all of the objects on a tab control, returning either a pointer to the last
//              object we touched, or nil if we have exhausted all objects.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine, unless we _know_ that all objects are being
//                                       processed!

TabObjectPtr IterateTabObjects(ControlHandle tabControl,
			       ObjectIterateProc oiProc, void *private)
{
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	short objectIndex, tabIndex, numTabs;
	SignedByte oldTabHState, oldObjectHState;

	numTabs = CountTabs(GetTabHandle(tabControl));
	for (tabIndex = 0; tabIndex < numTabs; ++tabIndex) {
		tabPtr = LockTabs(tabControl, tabIndex, &oldTabHState);
		for (objectIndex = 0; objectIndex < tabPtr->numObjects;
		     ++objectIndex) {
			objectPtr =
			    LockTabObjects(tabPtr, objectIndex,
					   &oldObjectHState);
			if ((*oiProc) (objectPtr, private)) {
				objectPtr->lockedObjects = tabPtr->objects;
				objectPtr->lockedTabs =
				    GetTabHandle(tabControl);
				objectPtr->objectsHState = oldObjectHState;
				objectPtr->tabsHState = oldTabHState;
				return (objectPtr);
			}
			UnlockTabObjects(tabPtr, oldObjectHState);
		}
		UnlockTabs(tabControl, oldTabHState);
	}
	return (nil);
}


//
//      IterateTabPaneObjects
//
//              Iterate over all of the objects on a tab pane, returning either a pointer to the last
//              object we touched, or nil if we have exhausted all objects.
//
//              Note:  This function leaves the tabs and found object locked!  Remeber to unlock
//                                       things in the calling routine, unless we _know_ that all objects are being
//                                       processed!

TabObjectPtr IterateTabPaneObjects(ControlHandle tabPane,
				   ObjectIterateProc oiProc, void *private)
{
	ControlHandle tabControl;
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	short objectIndex;
	SignedByte oldTabHState, oldObjectHState;

	GetSuperControl(tabPane, &tabControl);
	if (tabControl) {
		tabPtr =
		    LockTabs(tabControl, GetTabPaneIndex(tabPane),
			     &oldTabHState);
		for (objectIndex = 0; objectIndex < tabPtr->numObjects;
		     ++objectIndex) {
			objectPtr =
			    LockTabObjects(tabPtr, objectIndex,
					   &oldObjectHState);
			if ((*oiProc) (objectPtr, private)) {
				objectPtr->lockedObjects = tabPtr->objects;
				objectPtr->lockedTabs =
				    GetTabHandle(tabControl);
				objectPtr->objectsHState = oldObjectHState;
				objectPtr->tabsHState = oldTabHState;
				return (objectPtr);
			}
			UnlockTabObjects(tabPtr, oldObjectHState);
		}
		UnlockTabs(tabControl, oldTabHState);
	}
	return (nil);
}


void UnlockObject(TabObjectPtr objectPtr)
{
	if (objectPtr) {
		if (objectPtr->lockedObjects) {	// if the block was not previously locked
			HSetState(objectPtr->lockedObjects,
				  objectPtr->objectsHState);
			if ((objectPtr->objectsHState & 0x80) == 0)
				objectPtr->lockedObjects = nil;
		}
		if (objectPtr->lockedTabs) {	// if the block was not previously locked
			HSetState(objectPtr->lockedTabs,
				  objectPtr->tabsHState);
			if ((objectPtr->tabsHState & 0x80) == 0)
				objectPtr->lockedTabs = nil;
		}
	}
}


//
//      SwapObjectValues
//
//              For now we'll only handle label fields
//

void SwapObjectValues(TabObjectPtr objectOne, TabObjectPtr objectTwo)
{
	Str255 stringOne, stringTwo;

	if (objectOne->type == fieldObject
	    && objectTwo->type == fieldObject) {
		GetLabelFieldString(objectOne->control, stringOne);
		SetLabelFieldString(objectOne->control,
				    GetLabelFieldString(objectTwo->control,
							stringTwo));
		SetLabelFieldString(objectTwo->control, stringOne);
	}
}


void DirtyActiveTabPane(ControlHandle tabControl)
{
	TabPtr tabPtr;

	tabPtr = GetTabPtr(tabControl, GetActiveTabIndex(tabControl) - 1);
	tabPtr->dirty = true;
}


//
//      PeepingTom
//
//              Make a nickname picture dirty by staring at with tremendous lust.
//

void PeepingTom(ControlHandle tabControl)
{
	short ab, nick;

	if (GetSelectedABNick(1, &ab, &nick)) {
		DirtyActiveTabPane(tabControl);
		(*((*Aliases)[ab].theData))[nick].pornography = true;
	}
}

TabPtr LockTabs(ControlHandle tabControl, short tabIndex,
		SInt8 * oldHState)
{
	TabHandle allTabs;
	TabPtr tabPtr;

	tabPtr = nil;
	if (allTabs = GetTabHandle(tabControl)) {
		*oldHState = HGetState(allTabs);
		LDRef(allTabs);
		tabPtr = GetTabPtr(tabControl, tabIndex);
	}
	return (tabPtr);
}


void UnlockTabs(ControlHandle tabControl, SInt8 oldHState)
{
	TabHandle allTabs;

	if (allTabs = GetTabHandle(tabControl))
		HSetState(allTabs, oldHState);
}


TabObjectPtr LockTabObjects(TabPtr tabPtr, short objectIndex,
			    SInt8 * oldHState)
{
	TabObjectPtr objectPtr;
	Handle objects;

	objectPtr = nil;
	if (objects = GetTabObjectHandle(tabPtr)) {
		*oldHState = HGetState(objects);
		LDRef(objects);
		objectPtr = GetTabObjectPtr(tabPtr, objectIndex);
	}
	return (objectPtr);
}


void UnlockTabObjects(TabPtr tabPtr, SInt8 oldHState)
{
	Handle objects;

	if (objects = GetTabObjectHandle(tabPtr))
		HSetState(objects, oldHState);
}

#ifdef DEBUG
void CheckAllObjects(ControlHandle tabControl);
void CheckAllObjects(ControlHandle tabControl)
{
	TabPtr tabPtr;
	TabObjectPtr objectPtr;
	short objectIndex, tabIndex, numTabs;

	numTabs = CountTabs(GetTabHandle(tabControl));
	for (tabIndex = 0; tabIndex < numTabs; ++tabIndex) {
		tabPtr = GetTabPtr(tabControl, tabIndex);
		for (objectIndex = 0; objectIndex < tabPtr->numObjects;
		     ++objectIndex) {
			objectPtr = GetTabObjectPtr(tabPtr, objectIndex);
			if (objectPtr->lockedObjects
			    && (HGetState(objectPtr->lockedObjects) &
				0x80))
				DebugStr("\pObject still locked!!!");
			else if (objectPtr->lockedTabs
				 && (HGetState(objectPtr->lockedTabs) &
				     0x80))
				DebugStr("\pObject still locked!!!");
		}
	}
}
#endif
