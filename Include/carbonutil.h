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

#ifndef CARBONUTIL_H
#define CARBONUTIL_H

#include <PMApplication.h>
#include <MacWindows.h>

/**********************************************************************
	carbonutil.h	- accessor functions and macros for carbon compatibility

To access the following fields, use the indicated accessor functions:

****** QuickDraw ********
CGrafPort
	device;         
	portPixMap;     PixMapHandle GetPortPixMap(CGrafPtr port)
					Get pixel depth: short GetPortPixelDepth(CGrafPtr port)
					Get bitmap for copybits: BitMap *GetPortBitMapForCopyBits(CGrafPtr port)
	portVersion;    Boolean IsPortColor(CGrafPtr port)
	grafVars;       
	chExtra;        short GetPortChExtra(CGrafPtr port)
	pnLocHFrac;     short GetPortFracHPenLocation(CGrafPtr port) / void SetPortFracHPenLocation(CGrafPtr port,short pnLocHFrac)
	portRect;       Rect *GetPortBounds(CGrafPtr port, Rect *r / void SetPortBounds(CGrafPtr port,const Rect *r)
	visRgn;         RgnHandle GetPortVisibleRegion(CGrafPtr port,RgnHandle visRgn) / void SetPortVisibleRegion(CGrafPtr portRgnHandle visRgn)
					To avoid having to create a new region, use:
						RgnHandle MyGetPortVisibleRegion(CGrafPtr port)
						Important: returns region in a global variable, do not reuse while still in use! 
	clipRgn;        RgnHandle GetPortClipRegion(CGrafPtr port,RgnHandle clipRgn) / void SetPortClipRegion(CGrafPtr port,RgnHandle clipRgn)
					To save and restore clip region, use the following which allocate and deallocate temporary regions:
						RgnHandle SavePortClipRegion(CGrafPtr port);
						void RestorePortClipRegion(CGrafPtr port,RgnHandle rgn);
	bkPixPat;       PixPatHandle GetPortBackPixPat(CGrafPtr port,PixPatHandle backPattern) / void SetPortBackPixPat(CGrafPtr port,PixPatHandle backPattern)
	rgbFgColor;     RGBColor *GetPortForeColor(CGrafPtr port,RGBColor *foreColor) /  void RGBForeColor(const RGBColor *color)
	rgbBkColor;     RGBColor *GetPortBackColor(CGrafPtr port,RGBColor *backColor) /  void RGBBackColor(const RGBColor *color)
	pnLoc;          Point *GetPortPenLocation(CGrafPtr port,Point * penLocation)
	pnSize;         Point *GetPortPenSize(CGrafPtr port,Point *penSize) / void SetPortPenSize(CGrafPtr port,Point penSize)
	pnMode;         SInt32 GetPortPenMode(CGrafPtr port) / void SetPortPenMode(CGrafPtr port,SInt32 penMode)
	pnPixPat;       PixPatHandle GetPortPenPixPat(CGrafPtr port,PixPatHandle penPattern) / void SetPortPenPixPat(CGrafPtr port,PixPatHandle penPattern)
	fillPixPat;     PixPatHandle GetPortFillPixPat(CGrafPtr port,PixPatHandle fillPattern)
	pnVis;          short GetPortPenVisibility(CGrafPtr port) / void ShowPen(void) / void HidePen(void)
	txFont;         short GetPortTextFont(CGrafPtr port) / void TextFont(short font)
	txFace;         short GetPortTextFace(CGrafPtr port) / void TextFace(short face)
	              	StyleField occupies 16-bits, but only first 8-bits are used
	txMode;         short GetPortTextMode(CGrafPtr port) / void TextMode(short mode)
	txSize;         short GetPortTextSize(CGrafPtr port) / void TextSize(short size)
	spExtra;        Fixed GetPortSpExtra(CGrafPtr port) / void SpaceExtra(Fixed extra)
	fgColor;        
	bkColor;        
	colrBit;        
	patStretch;     
	picSave;        Boolean IsPortPictureBeingDefined(CGrafPtr port)
	rgnSave;        
	polySave;       
	grafProcs;      CQDProcsPtr GetPortGrafProcs(CGrafPtr port) / void SetPortGrafProcs(CGrafPtr port,CQDProcs *procs)
	
  Misc:
	Create new port:	void MyCreateNewPort(CGrafPtr *port)
	Dispose port:		DisposePort(CGrafPtr port)
	Region bounds:		Rect *GetRegionBounds(RgnHandle region,Rect *bounds)

QDGlobals
	privates[76];
	randSeed;       long GetQDGlobalsRandomSeed(void)
	screenBits;     BitMap *GetQDGlobalsScreenBits(BitMap *screenBits)
	arrow;          Cursor *GetQDGlobalsArrow(Cursor *arrow)
	dkGray;         Pattern *GetQDGlobalsDarkGray(Pattern *dkGray)
	ltGray;         Pattern *GetQDGlobalsLightGray(Pattern *ltGray)
	gray;           Pattern *GetQDGlobalsGray(Pattern *gray)
	black;          Pattern *GetQDGlobalsBlack(Pattern *black)
	white;          Pattern *GetQDGlobalsWhite(Pattern *white)
	thePort;        CGrafPtr GetQDGlobalsThePort(void)
	
  Misc:
  	Get screenbits bounds:	Rect *GetQDGlobalsScreenBitsBounds(Rect *bounds)


****** Window Manager ********
CWindowRecord
	port;           CGrafPtr GetWindowPort(WindowRef window)
	windowKind;     short GetWindowKind(WindowRef window) / void SetWindowKind(WindowRef window,short kind)
	visible;        Boolean IsWindowVisible(WindowRef w)
	hilited;        Boolean IsWindowHilited(WindowRef w)
	goAwayFlag;     Boolean GetWindowGoAwayFlag(WindowRef win)
	spareFlag;      ChangeWindowAttributes
	strucRgn;       OSStatus GetWindowStructureRgn(WindowRef window,RgnHandle rgn) !! Copies into existing region handle
						To get bounds:
							Rect *GetStructureRgnBounds (WindowPtr theWindow, Rect *bounds)
						To get strucRgn without making a copy:
							RgnHandle MyGetWindowStructureRegion(WindowRef window)
							Important: returns region in a global variable, do not reuse while still in use! 
	contRgn;        Rect *GetContentRgnBounds(WindowPtr theWindow,Rect *bounds)
						To get bounds:
							Rect *GetContentRgnBounds (WindowPtr theWindow, Rect *bounds)
						To get contRgn without making a copy:
							RgnHandle MyGetWindowContentRegion(WindowRef window)
							Important: returns region in a global variable, do not reuse while still in use! 
	updateRgn;      RgnHandle GetWindowUpdateRgn(WindowPtr theWindow, RgnHandle rgn)	// makes a copy
					Rect *GetUpdateRgnBounds(WindowPtr theWindow, Rect *bounds)
					Boolean	HasUpdateRgn(WindowPtr theWindow)
	windowDefProc;
	dataHandle;
	titleHandle;    void GetWTitle(WindowRef window,Str255 title) / void SetWTitle(WindowRef window,ConstStr255Param title)
	titleWidth;     short MyGetWindowTitleWidth(WindowPtr theWindow)
	controlList;    ControlHandle GetControlList (WindowPtr theWindow)
					ControlHandle GetNextControl(ControlHandle theControl)
	nextWindow;     WindowRef GetNextWindow(WindowRef w) 
	windowPic;      PicHandle GetWindowPic(WindowRef window) / void SetWindowPic(WindowRef window,PicHandle pic)
	refCon;         long GetWRefCon(WindowRef window) / void SetWRefCon(WindowRef window,long data)

  Misc:
  	InvalRect:		OSStatus InvalWindowRect(WindowRef window,const Rect *bounds)
  	Inval portRect:	OSStatus InvalWindowPort(WindowPtr theWindow)
  	Get portRect:	Rect *GetWindowPortBounds(WindowRef window,Rect * bounds)
  	StdState:		Rect *GetWindowStandardState(WindowRef window,Rect * rect)
  					void SetWindowStandardState(WindowRef window,const Rect *rect)
  	UserState:		Rect *GetWindowUserState(WindowRef window,Rect *rect)
  					void SetWindowUserState(WindowRef window,const Rect *rect)

****** Control Manager ********
ControlRecord
	nextControl;    ControlHandle GetNextControl(ControlHandle theControl)
					ControlHandle GetControlList(WindowPtr theWindow)
	contrlOwner;    WindowRef GetControlOwner(ControlRef control)
	contrlRect;     Rect *GetControlBounds(ControlRef control,Rect * bounds) / void SetControlBounds(ControlRef control,const Rect *bounds)
	contrlVis;      Boolean IsControlVisible(ControlRef inControl) / OSErr SetControlVisibility(ControlRef inControl,Boolean inIsVisible,Boolean inDoDraw)
	contrlHilite;   UInt16 GetControlHilite(ControlRef control)
	contrlValue;    SInt16 GetControlValue(ControlRef theControl) / void SetControlValue(ControlRef theControl,SInt16 newValue)
					SInt32 GetControl32BitValue(ControlRef theControl) / void SetControl32BitValue(ControlRef theControl,SInt32 newValue)
	contrlMin;      SInt16 GetControlMinimum(ControlRef theControl) / void SetControlMinimum(ControlRef theControl,SInt16 newMinimum)
					SInt32 GetControl32BitMinimum(ControlRef theControl) / void SetControl32BitMinimum(ControlRef theControl,SInt32 newMinimum)
	contrlMax;      SInt16 GetControlMaximum(ControlRef theControl) / void SetControlMaximum(ControlRef theControl,SInt16 newMaximum)
					SInt32 GetControl32BitMaximum(ControlRef theControl) / void SetControl32BitMaximum(ControlRef theControl,SInt32 newMaximum)
	contrlDefProc;
	contrlData;     Handle GetControlDataHandle(ControlRef control) / void SetControlDataHandle(ControlRef control,Handle dataHandle)
					ZapControlDataHandle(ControlRef control)
	contrlAction;   ControlActionUPP GetControlAction(ControlRef theControl) / void SetControlAction(ControlRef theControl,ControlActionUPP actionProc)
	contrlRfCon;    SInt32 GetControlReference(ControlRef theControl) / void SetControlReference(ControlRef theControl,SInt32 data)
	contrlTitle;    void GetControlTitle(ControlRef theControl,Str255 title) / void SetControlTitle(ControlRef theControl,ConstStr255Param title)

****** Menu Manager ********
MenuInfo
	menuID;         MenuID GetMenuID(MenuRef menu) / void SetMenuID(MenuRef menu,MenuID menuID)
	menuWidth;      SInt16 GetMenuWidth(MenuRef menu) / void SetMenuWidth(MenuRef menu,SInt16 width)
	menuHeight;     SInt16 GetMenuHeight(MenuRef menu) / void SetMenuHeight(MenuRef menu,SInt16 height)
	menuProc;
	enableFlags;    Boolean IsMenuItemEnabled(MenuRef menu,MenuItemIndex item) / EnableMenuItem / DisableMenuItem
	menuData;       StringPtr GetMenuTitle(MenuRef menu,Str255 title) / OSStatus SetMenuTitle(MenuRef menu,ConstStr255Param title)


 **********************************************************************/


//	QuickDraw
#define	MyCreateNewPort(port)							port = CreateNewPort()
#define	GetPortPixelDepth(port)						(GetPixDepth (GetPortPixMap (port)))
RgnHandle SavePortClipRegion(CGrafPtr port);
void RestorePortClipRegion(CGrafPtr port,RgnHandle rgn);

//	QuickDraw Globals
Rect	*GetQDGlobalsScreenBitsBounds (Rect *bounds);

//	Window Manager
#define	GetWindowVisRgn(aWindowPtr,aRgnHandle)	(GetPortVisibleRegion (GetWindowPort (aWindowPtr), (aRgnHandle)))
#define GetWindowUpdateRgn(theWindow,rgn)		(GetWindowRegion(theWindow,kWindowUpdateRgn,rgn),rgn)
RgnHandle MyGetWindowStructureRegion(WindowRef window);
RgnHandle MyGetWindowContentRegion(WindowRef window);
RgnHandle MyGetWindowUpdateRegion(WindowRef window);
#define GetWindowStructureBounds(win,rectPtr)	GetWindowBounds(win,kWindowStructureRgn,rectPtr)
#define GetWindowContentBounds(win,rectPtr)	GetWindowBounds(win,kWindowContentRgn,rectPtr)
// pascal Boolean GetWindowGoAwayFlag(WindowRef win); CK
void GetWMgrPort(GrafPtr *wPort);
void GetCWMgrPort(CGrafPtr *wPort);
#define GetWindowStructureRgn(win,rgn)			GetWindowRegion(win,kWindowStructureRgn,rgn)
#define IsWindowVisible(win) MyIsWindowVisible(win)
Boolean MyIsWindowVisible(WindowRef win);
OSStatus	InvalWindowPort (WindowPtr theWindow);
Rect	*GetUpdateRgnBounds (WindowPtr theWindow, Rect *bounds);
Rect	*GetVisibleRgnBounds (WindowPtr theWindow, Rect *bounds);
Rect	*GetContentRgnBounds (WindowPtr theWindow, Rect *bounds);
Rect	*GetStructureRgnBounds (WindowPtr theWindow, Rect *bounds);
short	MyGetWindowTitleWidth (WindowPtr theWindow);
Boolean	HasUpdateRgn (WindowPtr theWindow);

//	Control Manager
#define ZapControlDataHandle(cntl)								{ if (GetControlDataHandle (cntl)) { DisposeHandle ((void *)(GetControlDataHandle (cntl))); SetControlDataHandle ((cntl), nil); }}
ControlHandle GetControlList (WindowPtr theWindow);
ControlHandle GetNextControl (ControlHandle theControl);

//	Menu Manager
#define	EnableItem	MacEnableMenuItem
#define	DisableItem	DisableMenuItem
#define	IsValidMenuHandle(aMenuRef)						(aMenuRef)
#define MySetMenuTitle(menu,title) SetMenuTitle(menu,title)

//	Dialog Manager
#define	GetDialogPortBounds(dp,aRectPtr)		GetWindowPortBounds(GetDialogWindow(dp),aRectPtr)

//	Memory Manager
#define NewEmptyHandleSys NewEmptyHandle
#define NewHandleSysClear NewHandleClear
#define NewHandleSys NewHandle

OSErr AEGetDescDataHandle(AEDesc *theAEDesc,Handle *handle);
void AEDisposeDescDataHandle(Handle h);

//	Printing
//OSStatus PMGetPhysicalPaperSizeAsRect(PMPageFormat pageFormat,Rect *paperSize);	// PMGetPhysicalPaperSize returns rect as PMRect, not Rect
//OSStatus PMGetPhysicalPageSizeAsRect (PMPageFormat pageFormat,Rect *pageSize);	// PMGetPhysicalPageSize returns rect as PMRect, not Rect
OSStatus PMGetAdjustedPaperSizeAsRect(PMPageFormat pageFormat,Rect *paperSize);	// PMGetAdjustedPaperSize returns rect as PMRect, not Rect
OSStatus PMGetAdjustedPageSizeAsRect (PMPageFormat pageFormat,Rect *paperSize);	// PMGetAdjustedPaperSize returns rect as PMRect, not Rect

//	Help Manager
//		Ignore all balloon functions
#define HMRemoveBalloon()		noErr
#define	HMIsBalloon()			false
#define	HMSetBalloons(flag)		noErr
#define	HMGetBalloons()			false
#define HMShowBalloon(msg,tip,hotRect,tipProc,procID,var,method)	noErr

//	Scrap Manager
long GetScrap(Handle destination,ScrapFlavorType flavorType,SInt32 *offset);
OSStatus PutScrap(SInt32 sourceBufferByteCount,ScrapFlavorType flavorType,const void * sourceBuffer);
#define ZeroScrap() ClearCurrentScrap()
Boolean IsScrapFull(void);

RgnHandle MyGetPortVisibleRegion(CGrafPtr port);
void SetEmptyVisRgn(CGrafPtr port);
void SetEmptyClipRgn(CGrafPtr port);
Rect GetScreenBounds(void);

void InitCarbonUtil(void);
OSErr	MyAECreateDesc (DescType typeCode, void *dataPtr, Size dataSize, AEDesc *result);
OSStatus	InvalControl (ControlHandle theControl);
//ComponentResult	GraphicsImportSetClipFromPortClip (GraphicsImportComponent  ci, CGrafPtr port);
void	MySetDialogFont (short fontNum);
ListHandle			CreateNewList (ListDefUPP ldefUPP, short theProc, Rect *rView, ListBounds *dataBounds, Point cSize, WindowPtr theWindow, Boolean drawIt, Boolean hasGrow, Boolean scrollHoriz, Boolean scrollVert);
Boolean MyIsMenuItemEnabled(MenuRef  menu,MenuItemIndex item);


short			ControlHi (ControlHandle theControl);
short			ControlWi (ControlHandle theControl);
// OSStatus	NotSupportedInCarbon (...); CK
#define	InlineGetHandleSize	GetHandleSize


#endif

/************************************************************************
 * DECLARE_UPP - declare a UPP, use in function variable declarations
 * INIT_UPP - initialize UPP, use before UPP is accessed
 *		These macros replace SIMPLE_UPP
 ************************************************************************/
#define DECLARE_UPP(routine,what) static what##UPP routine##UPP
#define INIT_UPP(routine,what) if (!routine##UPP) routine##UPP = New##what##UPP(routine)
