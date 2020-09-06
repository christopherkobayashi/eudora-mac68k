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

#ifndef MYWINDOW_H
#define MYWINDOW_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * my own window package
 **********************************************************************/

//
//	(jp) Under Carbon you can't use extended Toolbox data structures because everything stored within
//			 a WindowRecord, DialogRecord, ControlRecord, etc is supposedly stored in some sort of vague
//			 amorphous we'll-move-things-around-if-we-feel-like-it fashion.
//
//			 We have a couple of options here:
//					1. Store a WindowPtr in our MyWindowStruct, then continue to pass around pointers to our
//						 own window structures.
//					2. Pass around WindowPtr's and store a reference to our MyWindowStruct in the refcon field.
//					   This would also necessitate that we move the current contents of the refcon field into
//						 a refcon field within MyWindowStruct.
//					3. Use  Window properties.
//
//			 Okay, of these three choices, number (3) is clearly the most Apple-sanctioned way to do this.
//			 It's also likely the most un-backwards compatible way to deal with our source code.  Plus...
//       after my experiences using Control properties while implementing Nav Services, I don't trust
//			 properties, no matter how cool they are supposed to be.
//
//			 With (3) out of the picture, let's look at number (1)...  Number (1) is probably the method that
//			 requires the least amount of change to the existing code base -- but (2) is the bestest and
//			 purest "object model".  (2), however, is going to be a pain since we'd have to make fairly
//			 significant changes throughout the code.  (1) is a pain too, since we'd have to do backflips to
//			 access back to our own window structure if all we have is a WindowPtr.
//
//			 For now... let's do both (double the pain and twice the fun!)
//
//			      MyWindowStruct
//			 ________________________
//			|                        |
//			|                        |
//			|                        |
//			|------------------------|                                             MessType (or whatever)
//			|      Ptr to normal     |                                            ________________________
//			|      refcon stuff  --------------------------------------------->  |                        |
//			|------------------------|                                           |                        |
//			|                        |                                           |                        |
//			|                        |                                           |                        |
//			|                        |                                           |                        |
//			|                        |                                           |________________________|
//			|                        |                  WindowRecord
//			|------------------------|            ________________________
//			|         WindowPtr -------------->  |                        |
//			|________________________|           |                        |
//                   �                       |                        |
//                   |                       |                        |
//                   |                       |                        |
//                   |                       |                        |
//                   |                       |------------------------|
//                    -------------------------------- refcon         |
//                                           |________________________|
//
//
//			 What we're basically showing in the above diagram is a schem in which we allocate our own
//			 structure and point to this structure from the refcon field of a WindowRecord.  We also store
//			 a pointer back to the window itself in our structure.  The former refcon field has also been
//			 moved into our structure.
//
//			 A bunch of accessor routines have been created to turn WindowPtr's into MyWindowPtr's and
//			 vice versa, as follows:
//
//					MyWindowPtr GetWindowMyWindowPtr (WindowPtr theWindow);
//					MyWindowPtr GetDialogMyWindowPtr (DialogPtr theDialog);
//					WindowPtr   GetMyWindowWindowPtr (MyWindowPtr win);
//					DialogPtr   GetMyWindowDialogPtr (MyWindowPtr win);
//					long				GetWindowPrivateData (WindowPtr theWindow);
//					long				GetDialogPrivateData (DialogPtr theDialog);
//
//			 Use these accessors instead of peeking into the structures -- and don't forget to check for
//			 nil!
//
//			 Yeah, the names seem clumsy, but follow naming conventions used in the new Universal
//			 headers for casting (?) between window/dialog/grafports.
//
//			 It's also important to note that a DialogPtr is not a WIndowPtr is not a GrafPtr, so we're no
//			 longer unionizing the window and dialogs.  Instead, the pointer in our window structure is
//			 ALWAYS a WindowPtr and we derive a DiaogPtr from the WindowPtr if and only if we know we are
//			 dealing with a Dialog.  Likewise, GrafPtr's should be derived when needed.
//
//			 As you might guess, danger abounds!!  Especially since we have to be extremely cautious about
//			 passing the right type of Window/Dialog/GrafPort pointer to toolbox functions which expect
//			 such things.  Casting is very bad -- casting to void * is even worse.  Live, learn and let's
//			 all change our evil ways.
//

typedef struct MyWindowStruct *MyWindowPtr;
typedef struct TableData **TableHandle;
typedef struct ViewListRec *ViewListPtr;
#ifdef USECMM
	typedef struct CMenuInfo **CMenuInfoHndl;
#endif
typedef struct MyWindowStruct
{
	WindowPtr	theWindow;			/* points to the window that references this structure */
	long			privateData;		/* private storage for this window structure (what was formerly in the window refcon)	*/
	long 			dialogRefcon;		/* Private storage for our dialogs (different than above since we stuff CREATOR in privateData for bwrd compatibility */
	Boolean isRunt; 					/* should not grow */
	Boolean isDialog; 				/* is this a dialog? */
	Boolean isNag;						/* is this a nag? */
	short dialogID;						/* really? what's its ID? */
	ControlHandle vBar; 			/* vertical scroll bar, if any */
	ControlHandle hBar; 			/* horizontal scroll bar, if any */
	int 			vMax;
	int 			hMax;
	Byte			hPitch;
	Byte			vPitch;
	short topMargin;					/* distance from top of window to top of contR */
	Rect			contR;					/* real content rectangle */
	Rect			strucR;					/* structure region bounding box */
	Boolean 		isActive; 		/* the window is topmost */
	Boolean 		isDirty;			/* the contents of the window are dirty */
	Boolean 		hasSelection; /* the window contains a selection */
	Boolean	butch;		// don't call pete drag handlers in PantyTrack
	void (*update)(MyWindowPtr win); 				/* handle update events */
	void (*click)(MyWindowPtr win,EventRecord *event);					/* handle clicks in the content region */
	void (*proxy)(MyWindowPtr win,EventRecord *event);					/* handle clicks in the content region */
	void (*bgClick)(MyWindowPtr win,EventRecord *event);					/* handle clicks in the content region in the background */
	void (*activate)(MyWindowPtr win); 			/* handle activation/deactivation */
	Boolean (*scroll)(MyWindowPtr win,short h,short v);			/* handle scrolling in lieu of normal mechanism */
	void (*didResize)(MyWindowPtr win, Rect *oldContR);			/* act after a window has been resized */
	Boolean (*menu)(MyWindowPtr win,int menu,int item,short modifiers);				/* handle a menu selection */
	Boolean (*close)(MyWindowPtr win); 			/* close the window */
	Boolean (*key)(MyWindowPtr win, EventRecord *event); 				/* we saw a keystroke */
	Boolean (*position)(Boolean save,MyWindowPtr win);		/* save/restore window position */
	Boolean (*filter)(MyWindowPtr win, EventRecord *event);					/* filter events (modeless dialogs) */
	Boolean (*selection)(MyWindowPtr win);					/* does the window have a selection? */
	void (*cursor)(Point mouse); 				/* set the cursor  and mouse region */
	void (*button)(MyWindowPtr win,ControlHandle button,long modifiers,short part); 				/* a button was hit */
	void (*showInsert)(); 		/* show the insertion point */
	void (*help)(MyWindowPtr win, Point mouse); 					/* show help balloon for the window */
	OSErr (*gonnaShow)(MyWindowPtr win);			/* get ready to become visible */
	void (*textChanged)(MyWindowPtr win, TEHandle teh, short oldNl, short newNl, Boolean scroll);		/* called after a text has changed */
	Boolean (*app1)(MyWindowPtr win,EventRecord *event);				/* handle an app1 (page keys) */
	Boolean (*hit)(EventRecord *event,DialogPtr dlog,short item,long dialogRefcon); 				/* for dialog windows */
	OSErr (*drag)(MyWindowPtr win,DragTrackingMessage message, DragReference drag);	/* drag tracking */
	void (*zoomSize)(MyWindowPtr win,Rect *zoom);				/* size the zoom rectangle */
	void (*grow)(MyWindowPtr win,Point *newSize);	// user has resized the window. Can adjust the size to a grid
	void (*idle)(MyWindowPtr win);	// routine to call every once in a while
	Boolean (*find)(MyWindowPtr win,PStr what);
	void (*transition)(MyWindowPtr win, UserStateType oldState, UserStateType newState);	// routine to call when we make a user state transition
	Boolean	(*dirty)(MyWindowPtr win);		// routine allowing windows to determine when they are or are not dirty
	void (*menuEnable)(MyWindowPtr win);	// routine allowing windows to enable menu items that override the norm
	void (*wazooSwitch)(MyWindowPtr win, Boolean show);	// routine allowing windows to do extra processing during a wazoo switch
	Boolean (*scrollWheel)(MyWindowPtr win,Point pt,short h,short v);	/* handle scroll wheel events for non-standard controls */
	PStr (*curAddr)(MyWindowPtr win,PStr addr);	// return the current address, if customized
#ifdef USECMM
	OSStatus (*buildCMenu)(MyWindowPtr win, EventRecord* contextEvent, MenuHandle contextMenu);	/*build a list of context-specific contextual menu items*/
#endif
	Boolean (*requestPeteFocus)(MyWindowPtr win, PETEHandle pte);	// somebody would like the focus here, probably the CMM
	Boolean (*drawerUseless)(MyWindowPtr win, short *left, short *right);	// how much useless space does the drawer add?
	uLong lastIdle;						// # of ticks since idle was last called
	uLong idleInterval;				// call idle routine no oftener than this # of ticks
	//TEHandle txe; 						/* handle to textedit, if any */
	PETEHandle pte;						/* handle to focussed pete TE field, if any */
	PETEHandle pteList;				/* list of all PTE's in this window */
	Handle	wazooData;				/* wazoo data if window is wazoo */
	Boolean userSave;					// the user saves this window
	Boolean ro; 							/* current txe is ro */
	Boolean dontControl;			/* leave control handling to the click routine */
	Boolean saveSize; 				/* save the size when the window is closed */
	Boolean dontDrawControls;	/* tell the update proc not to draw controls for the window */
	Point minSize;						/* how small to allow the window to be */
	Point maxSize;						/* how large to allow the window to be */
	Boolean inUse;						/* is the window in use? */
	Boolean	noUpdates;					/* ignore update events for a while */
	Boolean ignoreDefaultItem;	/* ignore any default item if this is a control (i.e. no focus ring) */
	Boolean centerAsDefault;		/* Places this window in the center of the main screen when it is first displayed */
	short label;							/* finder label color */
	short windex;							/* permanent window index */
	Boolean hideme;						// pretend window isn't there when calcing other window positions
#ifdef THREADING_ON
	threadDataHandle threadData;	
#endif
	TableHandle	table;				//	Handle to table data if editing a table
	short windowType;					/* standard, floating, or dockable */
	RGBColor textColor;				// default text color
	RGBColor backColor;				// default background color
	ThemeBrush backBrush;			// standard background brush to use
	ControlHandle topMarginCntl;	// top Margin of window
	ControlHandle labelCntl;	// window label indicator
	Boolean dontActivate;			// don't do activation stuff with controls
	short titleBarHi;
	short uselessHi;
	short leftRimWi;
	short uselessWi;
	Rect dontGreyOnMe;				// extra rectangle to punch out of grey background
	short botMargin;					// bottom margin of the window
	Boolean showsSponsorAd;		// this window can display the sponsor ad
	Boolean sponsorAdExists;		// there is a sponsor ad to display, window can turn this off if insufficient room to display
	Rect		sponsorAdRect;		// sponsor ad goes here
	Handle		hControlList;	//	list of ControlRefs
	Boolean classicVisible;			//	this window is visible in the classic sense.  That is, it has been displayed to the user.
	ViewListPtr	pView;
	Boolean	updating;		// Stop recursive updates
	Boolean noEditMenu;	// beep on edit menu commands
} MyWindow;

// Window Types
enum
{
	kStandard=0,
	kFloating,
	kDockable,
};

#define kHtCtl 22 
#define kSponsorBorderMargin 2

typedef struct
{
	short wType;					/* window type */
	long dirId; 					/* if any */
	short index;					/* if any */
	Str31 volName;				/* if any */
	Str31 name; 					/* if any */
	Str31 alias;					/* if any */
	uLong id;
} DejaVu, **DejaVuPtr, **DejaVuHandle;
#define DEJA_VU_TYPE	'dJvU'

/**********************************************************************
 * prototypes
 **********************************************************************/
MyWindowPtr GetNewMyWindow(short resId,void *wStorage,MyWindowPtr win,WindowPtr behind,Boolean hBar, Boolean vBar, short windowKind);
MyWindowPtr GetNewMyWindowWithClass(short resId,void *wStorage,MyWindowPtr win,WindowPtr behind,Boolean hBar, Boolean vBar, short windowKind, WindowClass winClass);
RGBColor *EffectiveBGColor(RGBColor *color,short label,Rect *r,short depth);
void UpdateMyWindow(WindowPtr winWP);
WindowPtr FrontWindowNeedsUpdate(WindowPtr ignoreThisWin);
void ScrollIt(MyWindowPtr win,int deltaH,int deltaV);
void EraseUpdateRgn(WindowPtr theWindow);
void DragMyWindow(WindowPtr winWP,Point thePt);
void GrowMyWindow(MyWindowPtr win,EventRecord *event);
void ZoomMyWindow(WindowPtr winWP,Point thePt,int partCode);
void ReZoomMyWindow(WindowPtr theWindow);
void OffsetWindow(WindowPtr winWP);
void MyWindowDidResize(MyWindowPtr win,Rect *oldContR);
void MoveMyCntl(ControlHandle cntl,int h,int v,int w,int t);
void MySetCntlRect(ControlHandle cntl,Rect *r);
short IncMyCntl(ControlHandle cntl,short inc);
void ScrollMyWindow(MyWindowPtr win,int h,int v);
void MyWindowMaxes(MyWindowPtr win,int hMax,int vMax);
int BarMax(ControlHandle cntl,int max,int winSize,int pitch);
void ActivateMyWindow(WindowPtr winWP,Boolean active);
Boolean CloseMyWindow(WindowPtr winWP);
void GoAwayMyWindow(WindowPtr theWindow,Point pt);
void ShowMyControl(ControlHandle cntrlH);
// (jp) We'll really need to be careful when window testing!  So... no casts, make it a function
//#define IsMyWindow(wp) ((wp) && IsAnyWindow((MyWindowPtr)wp) && \
//												((GrafPtr)wp)!=InsurancePort && (((MyWindowPtr)wp)->qWindow.windowKind==dialogKind &&\
//												((MyWindowPtr)wp)->qWindow.refCon==CREATOR || \
//												((MyWindowPtr)wp)->qWindow.windowKind>=userKind && ((MyWindowPtr)wp)->qWindow.windowKind!=EMS_PW_WINDOWKIND))
Boolean	IsMyWindowLo(WindowPtr winWP,Boolean checkWindowList);
#define IsMyWindow(w) IsMyWindowLo(w,true)
#define IsKnownWindowMyWindow(w)	IsMyWindowLo(w,false)
Boolean IsAnyWindow(WindowPtr maybeWindow);
#define	IsDirtyWindow(aMyWindowPtr)	((aMyWindowPtr)->dirty ? (*(aMyWindowPtr)->dirty)((aMyWindowPtr)) : (aMyWindowPtr)->isDirty)
void InvalContent(MyWindowPtr win);
void InvalTopMargin(MyWindowPtr win);
void InvalBotMargin(MyWindowPtr win);
OSErr ShowMyWindow(WindowPtr theWindow);
OSErr ShowMyWindowBehind(WindowPtr winWP, WindowPtr behindWP);
void InfiniteClip(CGrafPtr thePort);
pascal Boolean MyClikLoop(void);
short CalcCntlInc(ControlHandle cntl,short tentativeInc);
Boolean ScrollIsH(ControlHandle cntl);
pascal void AMLiveScrollAction(ControlHandle cntl, SInt16 part);
void UsingWindow(WindowPtr winWP);
void NotUsingWindow(WindowPtr winWP);
void NotUsingAllWindows(void);
void FigureZoom(WindowPtr winWP);
Boolean FigureStaticZoom( MyWindowPtr win, Boolean hasMB,
									const Rect* origZoom, Rect* staticZoom  );
Boolean WinLocValid( Rect* winRect, Boolean hasMB );
short GDIndexOf(GDHandle gd);
void ZeroWinFuncs(MyWindowPtr win);
//#define NextWindow(win) ((MyWindowPtr)(win->qWindow.nextWindow))
void RaisedWindowRect(MyWindowPtr win);
void UpdateAllWindows(void);
short GetCtlNameWidth(ControlHandle hCtrl);
ControlHandle NewIconButton(short cntlId,WindowPtr theWindow);
ControlHandle NewIconButtonMenu(short menuId,short iconId,WindowPtr theWindow);
MenuHandle GetMenuFromIconButton(ControlHandle cntl);
void ChangeWindowType(MyWindowPtr win,short newType);
void DoWindowPropMenu(MyWindowPtr win,short item,short modifiers);
void SetWindowPropMenu(MyWindowPtr win,Boolean enable,Boolean all,char markChar,short modifiers);
WindowPtr MyFrontWindow(void);
void SetTopMargin(MyWindowPtr win,short h);
void SetBotMargin(MyWindowPtr win,short h);
void PositionBevelButtons(MyWindowPtr win,short count,ControlHandle *pCtlList,short left,short top,short height,short maxWidth);
#define ActivateControl(c)	ActivateMyControl(c,true)
#define DeactivateControl(c)	ActivateMyControl(c,false)
void ActivateMyControl(ControlHandle cntl,Boolean active);
Boolean ShowControlHelp(Point mouse,short strnID,ControlHandle cntl,...);
// (jp) This macro has to be a little different in a world without extended WindowRecords
//#ifdef	FLOAT_WIN
//#define CorrectlyActivated(win) (!IsMyWindow(win) || (!InBG && ((win)==FrontWindow_() || IsFloating(win)))==(win)->isActive)
//#else		//FLOAT_WIN
//#define CorrectlyActivated(win) (!IsMyWindow(win) || (!InBG && ((win)==FrontWindow_()))==(win)->isActive)
//#endif	//FLOAT_WIN
Boolean	CorrectlyActivated(WindowPtr winWP);
void MyGetOriginalWTitle(MyWindowPtr win, Str255 title);
void RecallWindow(DejaVu *dv);
Boolean RememberWindow(WindowPtr winWP,DejaVu *dv);
void StashStructure(MyWindowPtr win);
GDHandle MyGetMainDevice(void);
#ifdef	FLOAT_WIN
#define UserSelectWindow(aWindowPtr) do{MySelectWindow(aWindowPtr);if (IsWindowCollapsed(aWindowPtr)) CollapseWindow(aWindowPtr,false);}while(0)
#else	//FLOAT_WIN
#define UserSelectWindow(aWindowPtr) do{SelectWindow(aWindowPtr);if (IsWindowCollapsed(aWindowPtr)) CollapseWindow(aWindowPtr,false);}while(0)
#endif	//FLOAT_WIN
Boolean IsWindowVisibleClassic(WindowPtr winWP);
void WindowIsInvisibleClassic(WindowPtr winWP);
void WindowIsVisibleClassic(WindowPtr winWP);

Boolean RequestPeteFocus(MyWindowPtr win, PETEHandle pte);


//	Casting functions to swap between Windows, Dialogs, Ports and our own window structure
#define	GetWindowMyWindowPtr(aWindowPtr)		(IsKnownWindowMyWindow(aWindowPtr) ? (MyWindowPtr) GetWRefCon (aWindowPtr) : nil)
#define	GetDialogMyWindowPtr(aDialogPtr)		((aDialogPtr) ? GetWindowMyWindowPtr (GetDialogWindow (aDialogPtr)) : nil)
#define	GetPortMyWindowPtr(aCGrafPtr)				((aCGrafPtr) ? GetWindowMyWindowPtr(GetWindowFromPort (aCGrafPtr)) : nil)

#define	GetMyWindowCGrafPtr(aMyWindowPtr)		((aMyWindowPtr) ? GetWindowPort (GetMyWindowWindowPtr(aMyWindowPtr)) : nil)
#define	GetMyWindowWindowPtr(aMyWindowPtr)	((aMyWindowPtr) ? (aMyWindowPtr)->theWindow : nil)
#define	GetMyWindowDialogPtr(aMyWindowPtr)	((aMyWindowPtr) ? GetDialogFromWindow(GetMyWindowWindowPtr(aMyWindowPtr)) : nil)

#define	SetWindowMyWindowPtr(aWindowPtr,aMyWindowPtr)	do { if (aWindowPtr) SetWRefCon ((aWindowPtr),(long)(aMyWindowPtr)); } while (0)
#define	SetDialogMyWindowPtr(aDialogPtr,aMyWindowPtr)	do { if (aDialogPtr) SetWRefCon (GetDialogWindow(aDialogPtr),(long)(aMyWindowPtr)); } while (0)

long	GetWindowPrivateData (WindowPtr theWindow);
long	GetDialogPrivateData (DialogPtr theDialog);
void	SetWindowPrivateData (WindowPtr theWindow, long privateData);
void	SetDialogPrivateData (DialogPtr theDialog, long privateData);
#define	GetMyWindowPrivateData(aMyWindowPtr)				((aMyWindowPtr) ? (aMyWindowPtr)->privateData : 0)
#define	SetMyWindowPrivateData(aMyWindowPtr,aLong)	do { if (aMyWindowPtr) (aMyWindowPtr)->privateData = (aLong); } while (0)

void MyDisposeWindow (WindowPtr winWP);
pascal void ScrollAction(ControlHandle theCtl,short partCode);

#define ZapControl(c) do {if (c) {DisposeControl(c); c = nil;}} while (0)

// Though Appearance 1.1 fixes many of the problems with the tab control, it is clueless when it comes
// to moving embedded controls.  Or maybe it's the OS that's screwy.  In whichever, it's preferable to
// move our controls offscreen into Negativland (sic), but this does not work on 8.5.1 with Appearance 1.1
// so we need to move things into Positiveland -- which is dangerous since the window could conceivably be
// that big (though highly unlikely).
#define CNTL_OUT_OF_VIEW	(gBestAppearance?-REAL_BIG/2 : REAL_BIG/2)

#define WinWPWindowsMenuEligible(winWP) ( \
			(IsKnownWindowMyWindow(winWP)) &&\
			(GetWindowKind(winWP)!=TBAR_WIN) &&\
			(GetWindowKind(winWP)!=DRAWER_WIN) &&\
			(!IsWazooable(winWP)) &&\
			(!IsFloating(winWP)) &&\
			(IsWindowVisible(winWP))\
			)
#endif