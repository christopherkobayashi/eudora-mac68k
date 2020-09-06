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

#ifndef TABGEOMETRY_H
#define TABGEOMETRY_H

#define	kTabTypeWildCard	'****'
#define	kNoTabObjectFocus	(-1)

// Focus constants for tab controls
typedef enum {
	focusTabControlDirect,
	focusTabControlAdvance,
	focusTabControlReverse	
} TabFocusType;


//
//		This is cool stuff.  Cooler once I have time to explain how it works.
//

enum {
	kUndefinedTabObject		= -1,
	kTabObject						= -1,
	kUseOneLineHeight			= -1,
	kUseTwoLineHeight			= -2,
	kUseThreeLineHeight		= -3,
	kUseFourLineHeight		= -4,
	kUndefinedCoordinate	= -1
};

typedef enum {
	fieldObject							= 1,
	controlObject,
	controlResourceObject,
	pictureObject,
	invalidObect						= 0xFFFF			// Exists only to force the size to a short
} TabObjectType;

typedef enum {
	objectFlagNone					= 0x00000000,
	objectFlagAcceptsFocus	= 0x00000001,
	objectFlagExportable		= 0x00000002,
	objectFlagAll						= 0xFFFFFFFF
} ObjectFlagsType;

typedef enum {
	left = 1,
	top,
	right,
	bottom,
	height,
	width,
	horizontal,
	vertical
} CoordinateType;

//
//	RelativeToType
//
//	Values between 0 - 100 represent a percentage of width or height

typedef enum {
	relToNothing		= 0xFFFF,				// We don't really care -- forces sizeof to be a short
	relToLeft				= 1,						// Position relative to the left edge of another object
	relToTop				= 2,						// Position relative to the right edge of another object
	relToRight			= 3,						// Position relative to the top of another object
	relToBottom			= 4,						// Position relative to the bottom of another object
	relToHorzCenter	= 5,						// Position relative to the horizontal center of another object
	relToVertCenter	= 6,						// Position relative to the vertical center of another object
	relToWidth			= 7,						// Position relative to the width of another object (Delta contains relative percentage)
	relToHeight			= 8							// Position relative to the height of another object (Delta contains relative percentage)
} RelativeToType;

typedef enum {
	rfNone						= 0x00000000,
	rfDefaultMargin		= 0x00000001,		// Use the default margin as the delta
	rfNegativeMargin	= 0x00000002,		// The default margin is in the negative direction
	rfPETEPart				= 0x00000004,		// Relative to the PETE part of a label field
	rfLabelPart				= 0x00000008,		// Relative to the label part of a label field
	rAllRelativeFlags	= 0xFFFFFFFF		// Forces sizeof to a long
} RelativeFlagsType;

typedef enum {
	alignNothing	= 0xFFFF,				// We don't really care -- forces sizeof to be a short
	alignLeft			= 0,
	alignCenter		= 1,
	alignRight		= 2,
	alignTop			= 0,
	alignBottom		= 2
} ObjectAlignType;

typedef enum {
	coNone				= 0x00000000,
	coFit					= 0x00000001,
	coAllFlags		= 0xFFFFFFFF
} ControlObjectFlagsType;


typedef enum {
	noBehavior							= 0x00000000,
	behaveCheckBox					= 0x00000001,			// Act like a checkbox, setting the object value
	behaveSwapper						= 0x00000002,			// Swap values of some other objects, specified by Tabs or Fields flag
	behaveThisTab						= 0x00000004,			// Apply action to similar objects on this tab
	behaveTwoTabs						= 0x00000008,			// Apply action to similar objects on all tabs
	behaveAllTabs						= 0x00000010,			// Apply action to similar objects across two tabs (specified by tab name)
	behaveFields						=	0x00000020,			// Apply action to specified fields (specified by tag name)
	behaveMutuallyExclusive	= 0x00000040,			// Flip the state any similarly tagged or names objects (as you'd do a radio button)
	behaveMatchTitle				= 0x00000080,			// Use the object's title to gage what is and is not "similar"
	behaveMatchTag					= 0x00000100,			// Use the object's tag to gage what is and is not "similar"
	behaveStringOneIsTrue		= 0x00000200,			// Use "string one" of the BehaviorRec as the tag value when the control value is true
	behaveStringTwoIsFalse	= 0x00000400,			// Use "string two" of the BehaviorRec as the tag value when the control value is false
	behaveNavigation				= 0x00000800,			// Display a Nav services dialog
	behaveColorPicker				= 0x00001000,			// Display the color picker
	behaveAll								= 0xFFFFFFFF
} BehaviorFlagsType;

typedef enum {
	tabTagName,
	tagFieldName,
	tagShortName
} TagFieldStringType;

//
//	Tab Geometry Unleashed
//
//
//				ControlHandle (tab)
//						:
//						:
//						# of tabs (in the maximum field)
//						refcon ---> Tab 1 Description
//														:
//														:
//														# of objects
//														objects --->	Object 1 Description
//																							Object Type
//																							ControlHandle
//																							ObjectPositionRec
//																				
//																					Object 2 Description
//																							Object Type
//																							ControlHandle
//																							ObjectPositionRec
//												
//												Tab 2 Description
//														:
//														:
//														# of objects
//														objects --->
//

typedef struct {
	Boolean						isRelevant;	// Is this actually a relevant component?
	RelativeFlagsType	flags;			// Special stuff
	short							delta;			// Distance from the other object
	short							object;			// ID of the object we're relative to 
	RelativeToType		relevantTo;	// Relative to what coordinate of the other object
} RelativeToRec, *RelativeToPtr, **RelativeToHandle;

typedef struct {
	Boolean						isRelevant;	// Is this actually a relevant component?
	ObjectAlignType		align;			// Align this object with another
	RelativeFlagsType	flags;			// Special stuff
	short							object;			// ID of the object we're relative to 
} AlignToRec, *AlignToptr, **AlignToHandle;

typedef struct {
	RelativeToRec	left;
	RelativeToRec	top;
	RelativeToRec	right;
	RelativeToRec	bottom;
	short					height;
	short					width;
	AlignToRec		horizontal;
	AlignToRec		vertical;
} ObjectPositionRec, *ObjectPositionPtr, **ObjectPositionHandle;


typedef struct {
	BehaviorFlagsType	flags;
	Str255						stringOne;
	Str255						stringTwo;
} BehaviorRec, *BehaviorPtr, **BehaviorHandle;


typedef struct {
	TabObjectType			type;						// Object type
	ObjectFlagsType		flags;					// Object flags
	Str31							tag;						// Tag for the object
	Str31							shortName;			// Short name of the object, used for printing and exporting data
	ControlHandle			control;				// The object's ControlHandle (if, in fact, it _is_ a control...)
	BehaviorHandle		behavior;				// How this object behaves (if at all...)
	Rect							bounds;					// The bounding rectangle of this object
	ObjectPositionRec	position;				// How the object should be positioned within the tab
	FlavorType				dragFlavor;			// Drag flavor for this object
	Boolean						hasFocus;				// I have the focus!
	Handle						lockedObjects;	// Don't touch!  When dereferencing an object stored as a block of TabObjectRec's,
																		//							 we store the locked TabObjectHandle here so that it can be easily unlocked later.
	Handle						lockedTabs;			// Don't touch!  When dereferencing an object stored in a block of TabRec's,
																		//							 we store the locked TabHandle here so that it can be easily unlocked later.
	SignedByte				objectsHState;	// Don't touch!  When dereferencing an object stored as a block of TabObjectRec's,
																		//							 we store the state of the TabObjectHandle prior to locking so that it can be restored later.
	SignedByte				tabsHState;			// Don't touch!  When dereferencing an object stored as a block of TabRec's,
																		//							 we store the state of the TabHandle prior to locking so that it can be restored later.
} TabObjectRec, *TabObjectPtr, **TabObjectHandle;

typedef struct {
	OSType					tabSignature;					// Tab signature
	short						defaultHorzMargin;		// value to be used when 'kUseHorzMargin' is set as a position delta
	short						defaultVertMargin;		// value to be used when 'kUseVertMargin' is set as a position delta
	short						numObjects;						// Number of objects appearing on this tab
	short						focusObject;					// Index of the object that has focus (0-based... -1 means no focus)
	Boolean					dirty;								// Has anything on this pane been changed?
	TabObjectHandle	objects;							// Link to the tab objects
} TabRec, *TabPtr, **TabHandle;

typedef struct {
	short		version;
	OSType	applOwner;
	OSType	tabOwner;
	OSType	tabSignature;
	Str255	title;
	short		iconSuiteID;
	short		defaultHorzMargin;
	short		defaultVertMargin;
	short		numObjects;
} TabResHeaderRec, *TabResHeaderPtr, **TabResHeaderHandle;

typedef struct {
	TabFieldHandle			fields;
	TagFieldStringType	type;
} GetTabFieldsRec, *GetTabFieldsPtr, **GetTabFieldsHandle;

typedef struct {
	PStr		what;
	long		offset;
	Boolean	focusFound;
	Boolean	hasFocus;
} TabFindStringProcParamRec, *TabFindStringProcParamPtr, **TabFindStringProcParamHandle;


typedef Boolean	(*TabPaneIterateProc) (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, void *private);
typedef	Boolean	(*ObjectIterateProc) (TabObjectPtr objectPtr, void *private);


//	macros

#define	CountTabs(aTabHandle)							(GetHandleSize (aTabHandle) / sizeof (TabRec))
#define	GetTabHandle(aControlHandle)			(TabHandle) GetControlReference (aControlHandle)
#define	GetTabObjectHandle(aTabPtr)				((aTabPtr)->objects)
#define	GetTabPtr(aControlHandle,aShort)	(*(GetTabHandle (aControlHandle)) + (aShort))
#define	GetTabObjectPtr(aTabPtr,aShort)		(*(GetTabObjectHandle(aTabPtr)) + (aShort))

#define	GetRequestedHeight(aShortHeight,aShortLineheight)	((aShortHeight) <= kUseOneLineHeight ? -(aShortHeight) * (aShortLineheight) : (aShortHeight))
#define	GetActiveTabIndex(aControlHandle)	GetControlValue (aControlHandle)
#define	GetTabPaneCount(aControlHandle)		GetControlMaximum (aControlHandle)
#define	GetTabPaneIndex(aControlHandle)		GetControlReference (aControlHandle)

//	prototypes

ControlHandle 	CreateTabControl (MyWindowPtr win, Rect *boundsRect, OSType applOwner, OSType tabOwner, Boolean currentResFileOnly);
void						DisposeTabControl (ControlHandle tabControl, Boolean disposeControls);
void						DisposePictureObject (TabObjectPtr objectPtr);
OSErr						AddTab (ControlHandle tabControl, TabHandle tabs, Handle newTab, OSType applOwner, OSType tabOwner);
Boolean					InterestingTab (TabHandle tabs, TabResHeaderPtr tabResHeader, OSType applOwner, OSType tabOwner);
void						TabActivate (ControlHandle tabControl, Boolean isActive);
Boolean					TabActivateProc (TabObjectPtr objectPtr, Boolean *isActive);
void						TabMenuEnable (ControlHandle tabControl, Boolean ro);
Boolean					TabMenu (ControlHandle tabControl, int menu, int item, short modifiers);
Boolean					TabKey (ControlHandle tabControl, EventRecord *event);
Boolean					TabDragIsInteresting (ControlHandle tabControl, DragReference drag);
Boolean 				TabDragIsInterestingProc (TabObjectPtr objectPtr, DragReference drag);
OSErr						TabDrag (ControlHandle tabControl, DragTrackingMessage which, DragReference drag);
OSErr						ImageWellDrop (ControlHandle tabControl, TabObjectPtr objectPtr, DragReference drag);
void						HiliteImageWell (ControlHandle theControl, DragReference drag, Boolean hilite);
Boolean					CopyPictureFromImageWell (ControlHandle theControl);
void						PastePictureIntoImageWell (ControlHandle tabControl, TabObjectPtr objectPtr);
OSErr						ReplaceImageWellPicture (ControlHandle tabControl, TabObjectPtr objectPtr, PicHandle picture);
OSErr						FSpMakeIconSuite (FSSpec *tempSpec, PicHandle picture, PStr name);
Boolean					TabHasSelection (ControlHandle tabControl);
Boolean					ClickTab (ControlHandle tabControl, EventRecord *event, Point pt);
Boolean					TabSetKeyboardFocus (ControlHandle tabControl, TabFocusType tabFocus, Boolean getFocus);
Boolean					FirstFocusObjectProc (TabObjectPtr objectPtr, TabObjectPtr *foundObjectPtr);
Boolean					LastFocusObjectProc (TabObjectPtr objectPtr, TabObjectPtr *foundObjectPtr);
Boolean					TabObjectSetKeyboardFocus (TabPtr tabPtr, TabObjectPtr objectPtr, Boolean getFocus);
Boolean					TabAdvanceKeyboardFocus (ControlHandle tabControl);
Boolean					TabReverseKeyboardFocus (ControlHandle tabControl);
Boolean					TabFindString (ControlHandle tabControl, Boolean hasFocus, PStr what);
Boolean					TabFindStringProc (TabObjectPtr objectPtr, TabFindStringProcParamPtr paramsPtr);
void						HandleTabObjectBehavior (ControlHandle tabControl, TabObjectPtr objectPtr);
Boolean					MutuallyExclusiveCheckboxProc (TabObjectPtr objectPtr, TabObjectPtr hitObjectPtr);
Boolean					SwapTabObjectsProc (TabObjectPtr objectPtr, ControlHandle tabPaneTwo);
void						ClearTab (ControlHandle tabControl);
Boolean					ClearTabProc (TabObjectPtr objectPtr, ControlHandle tabControl);
TabFieldHandle	GetTabFieldStrings (ControlHandle tabControl, TagFieldStringType type);
Boolean					GetTabFieldStringsProc (TabObjectPtr objectPtr, GetTabFieldsPtr gtfp);
TabFieldHandle	GetExportableTabFieldStrings (ControlHandle tabControl);
Boolean					GetExportableTabFieldStringsProc (TabObjectPtr objectPtr, TabFieldHandle *fields);
TabFieldHandle	GetTabFieldsFromResources (TagFieldStringType type, OSType applOwner, OSType tabOwner);
Boolean					FindFieldString (TabFieldHandle fields, PStr key);
void						CleanTab (ControlHandle tabControl);
Boolean					CleanTabPaneProc (TabObjectPtr objectPtr, void *private);
Boolean					IsTabDirty (ControlHandle tabControl);
Boolean					DirtyTabPaneProc (TabObjectPtr objectPtr, Boolean *dirty);
void						HideTab (ControlHandle tabPane);
void						ShowTab (ControlHandle tabPane);
void						ShowHideTabObjects (ControlHandle tabPane, Boolean showIt);
Boolean					ShowHideProc (TabObjectPtr objectPtr, Boolean *showIt);
void						MoveTab (ControlHandle tabControl, int h, int v, int w, int t);
void						MoveTabPane (ControlHandle tabPane, short int h, int v, int w, int t);
ControlHandle 	GetActiveTabUserPane (ControlHandle tabControl);
short						SwitchTabs (ControlHandle tabControl, short currentTabIndex, short newTabIndex, short tabPrefResID);
void						SwitchLabelFieldsOnThisTab (ControlHandle tabControl, short tabIndex, Boolean showIt);
short						CalcRelevantCoordinate (ControlHandle tabPane, TabPtr tabPtr, RelativeToPtr relative);
short						GetRelativeToDelta (RelativeToPtr relative, short defaultMargin);
Rect 						*GetRelevantObjectBounds (ControlHandle tabPane, TabPtr tabPtr, short object, RelativeFlagsType flags, Rect *boundsRect);
Ptr							ParseTabHeader (Ptr from, TabResHeaderPtr tabHeader);
Ptr							ParseObject (ControlHandle tabControl, Ptr from, TabObjectPtr object);
Ptr							ParseRelativeInformation (Ptr from, RelativeToPtr relativePtr);
Ptr							ParseAlignmentInformation (Ptr from, AlignToptr alignPtr);
Ptr							ParseFieldObject (ControlHandle tabControl, Ptr from, TabObjectPtr object);
Ptr							ParseControlObject (ControlHandle tabControl, Ptr from, TabObjectPtr objectPtr);
Ptr							ParseControlResourceObject (ControlHandle tabControl, Ptr from, TabObjectPtr objectPtr);
Ptr							ParsePictureObject (ControlHandle tabControl, Ptr from, TabObjectPtr objectPtr);
Ptr							ParsePosition (Ptr from, ObjectPositionPtr positionPtr);
Ptr							ParseRelativeInformation (Ptr from, RelativeToPtr relativePtr);
void *						CopyBytesAndMovePtr (void *from, void *to, short len);

short						WidthHint (TabObjectPtr objectPtr);
short 					HeightHint (TabObjectPtr objectPtr, short lineHeight);

ControlHandle		FindTabPaneWithTitle (ControlHandle tabControl, PStr title);
Boolean					FindTabPaneWithTitleProc (ControlHandle tabControl, ControlHandle tabPane, short tabIndex, PStr title);
TabObjectPtr		FindObjectWithTag (ControlHandle tabControl, PStr tag);
Boolean					FindObjectWithTagProc (TabObjectPtr objectPtr, PStr tag);
PStr						FindTitleWithTag (ControlHandle tabControl, PStr tag, PStr title);
PStr						FindShortNameWithTag (ControlHandle tabControl, PStr tag, PStr shortName);
TabObjectPtr		FindObjectWithTitle (ControlHandle tabControl, PStr title);
Boolean					FindObjectWithTitleProc (TabObjectPtr objectPtr, PStr title);
TabObjectPtr		FindObjectWithShortName (ControlHandle tabControl, PStr shortName);
Boolean					FindObjectWithShortNameProc (TabObjectPtr objectPtr, PStr shortName);
TabObjectPtr		FindObjectWithTagOnTab (ControlHandle tabPane, PStr tag);
TabObjectPtr		FindObjectWithTitleOnTab (ControlHandle tabPane, PStr title);
PETEHandle			FindFieldOnTab (ControlHandle tabControl, short tabIndex, PETEHandle pte);
Boolean					FindFieldOnTabProc (TabObjectPtr objectPtr, PETEHandle *pteTarget);
TabObjectPtr		FindObjectWithFocus (ControlHandle tabControl);
TabObjectPtr		PtInTabObject (ControlHandle tabPane, Point pt);
Boolean					PtInTabObjectProc (TabObjectPtr objectPtr, Point *pt);

TabObjectPtr		SetObjectValueWithTag (ControlHandle tabControl, PStr tag, UPtr value, long valueLength);
Boolean					SetPictureObjectValue (ControlHandle tabControl, TabObjectPtr objectPtr, UPtr value, long valueLength);
void						DisplayPictureObject (TabObjectPtr objectPtr);
OSErr						ImageWellRemovePicture (ControlHandle theControl);
void						ImageWellDrawFocus (ControlHandle theControl, Boolean hasFocus);
void						ImageWellDrag (ControlHandle theControl, EventRecord *event);
PicHandle				GetImageWellPicture (ControlHandle theControl);
OSErr						SetImageWellPicture (ControlHandle theControl, PicHandle picture);
Boolean					GetPictureDragRgn (MyWindowPtr win, PicHandle picture, Rect *pictRect, RgnHandle *phRgn, GWorldPtr *pgworld, RgnHandle *phDrawRgn,DragReference drag);

ControlHandle		IterateTabPanes (ControlHandle tabControl, TabPaneIterateProc tpiProc, void *private);
TabObjectPtr		IterateTabObjects (ControlHandle tabControl, ObjectIterateProc oiProc, void *private);
TabObjectPtr		IterateTabPaneObjects (ControlHandle tabPane, ObjectIterateProc oiProc, void *private);
void						UnlockObject (TabObjectPtr objectPtr);
void						SwapObjectValues (TabObjectPtr objectOne, TabObjectPtr objectTwo);
void						DirtyActiveTabPane (ControlHandle tabControl);
void						PeepingTom (ControlHandle tabControl);

Boolean					ChooseGraphicStd (FSSpec *spec);
pascal Boolean	chooseGraphicObjectFilterProc (AEDesc *theItem, void *info, SFODocPtr optionsPtr, NavFilterModes filterMode);
void						SetImageWellFSSpec (ControlHandle tabControl, TabObjectPtr objectPtr, FSSpec *spec);

TabPtr					LockTabs (ControlHandle tabControl, short tabIndex, SignedByte *oldHState);
void						UnlockTabs (ControlHandle tabControl, SignedByte oldHState);

TabObjectPtr		LockTabObjects (TabPtr tabPtr, short objectIndex, SignedByte *oldHState);
void						UnlockTabObjects (TabPtr tabPtr, SignedByte oldHState);

#define	IterateTabObjectsUL(aControlHandle,aObjectIterateProc,aVoidPtr)	UnlockObject(IterateTabObjects(aControlHandle,aObjectIterateProc,aVoidPtr))
#define	IterateTabPaneObjectsUL(aControlHandle,aObjectIterateProc,aVoidPtr)	UnlockObject(IterateTabPaneObjects(aControlHandle,aObjectIterateProc,aVoidPtr))

#endif