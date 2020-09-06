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

//
//	carboncompat.h
//
//	Macros for selectively dealing with TARGET_CARBON style window records and
//	pre TARGET_CARBON windows
//

#ifndef CARBONCOMPAT_H
#define CARBONCOMPAT_H

//
//	QuickDraw Helpers
//
#define	MyGetWMgrPort(aGrafPtrPtr)										(*(aGrafPtrPtr) = FakeWMgrPort)
#define	MyCreateNewPort()															(CreateNewPort ())
#define	MyGetPortBounds(aCGrafPtr,aRectPtr)						(GetPortBounds ((aCGrafPtr), (aRectPtr)))
#define	MyGetPortTextFont(aCGrafPtr)									(GetPortTextFont (aCGrafPtr))
#define	MyGetPortTextFace(aCGrafPtr)									(GetPortTextFace (aCGrafPtr))
#define	MyGetPortTextSize(aCGrafPtr)									(GetPortTextSize (aCGrafPtr))
#define	MyGetPortTextMode(aCGrafPtr)									(GetPortTextMode (aCGrafPtr))
#define	SavePortClipRegion(aCGrafPtr,aRgnHandle)			do { if (aRgnHandle = NewRgn ()) GetPortClipRegion ((aCGrafPtr), aRgnHandle); } while (0)
#define	RestorePortClipRegion(aCGrafPtr,aRgnHandle)		do { if (aRgnHandle) { SetPortClipRegion ((aCGrafPtr), aRgnHandle); DisposeRgn (aRgnHandle); } } while (0)
#define	MyGetPortClipRegion(aCGrafPtr,aRgnHandle)			(GetPortClipRegion ((aCGrafPtr), (aRgnHandle)))
#define	MySetPortClipRegion(aCGrafPtr,aRgnHandle)			SetPortClipRegion ((aCGrafPtr), (aRgnHandle))
#define	GetPortPixelDepth(aGrafPtr)										(GetPixDepth (GetPortPixMap (port)))
#define	MyGetRegionBounds(aRgnHandle,aRectPtr)				(GetRegionBounds ((aRgnHandle),(aRectPtr)))
#define	MyGetPortPenLocation(aCGrafPtr,aPointPtr)			(GetPortPenLocation((aCGrafPtr),(aPointPtr)))


//
//	QuickDraw Globals Helpers
//
#define	QDGlobalsThePort											(GetQDGlobalsThePort ())
#define MyGetQDGlobalsGray(aPatternPtr)				(GetQDGlobalsGray (aPatternPtr))
#define MyGetQDGlobalsBlack(aPatternPtr)			(GetQDGlobalsBlack (aPatternPtr))
#define MyGetQDGlobalsWhite(aPatternPtr)			(GetQDGlobalsWhite (aPatternPtr))
#define	MyGetQDGlobalsArrow(aCursorPtr)				(GetQDGlobalsArrow (aCursorPtr))
#define	MyGetQDGlobalsScreenBits(aBitMapPtr)	(GetQDGlobalsScreenBits (aBitMapPtr))



//
//	Window Manager Helpers
//

#define	MyCloseWindow(aWindowPtr)								MyDisposeWindow (aWindowPtr)
#define	MyGetWindowList													(GetWindowList ())
#define	MyGetWindowFromPort(aCGrafPtr)					(GetWindowFromPort(aCGrafPtr))
#define	MyGetWindowKind(wp)											(GetWindowKind (wp))
#define	MyIsWindowVisible(wp)										(IsWindowVisible (wp))
#define	MyIsWindowHilited(wp)										(IsWindowHilited (wp))
#define	MyGetWindowPortBounds(wp,aRectPtr)			(GetWindowPortBounds ((wp), (aRectPtr)))
#define	MyGetWindowStandardState(wp,aRectPtr)		(GetWindowStandardState((wp), (aRectPtr)))
#define	MyGetWindowUserState(wp,aRectPtr)				(GetWindowUserState((wp), (aRectPtr)))
#define	MyGetWindowSpareFlag(wp)								(GetWindowSpareFlag(wp))
#define	MyInvalWindowRect(aWindowPtr,aRectPtr)	InvalWindowRect ((aWindowPtr), (aRectPtr))
#define	MyInvalWindowRgn(aWindowPtr,aRgnHandle)	InvalWindowRgn ((aWindowPtr), (aRgnHandle))
#define	MyValidWindowRect(aWindowPtr,aRectPtr)	ValidWindowRect ((aWindowPtr), (aRectPtr))
#define	MyValidWindowRgn(aWindowPtr,aRgnHandle)	ValidWindowRgn ((aWindowPtr), (aRgnHandle))
#define	MySetWindowKind(aWindowPtr,aShort)			SetWindowKind ((aWindowPtr), (aShort))
#define	MySetWindowStandardState(wp,aRectPtr)		SetWindowStandardState ((wp), (aRectPtr))
#define	MySetWindowUserState(wp,aRectPtr)				SetWindowUserState ((wp), (aRectPtr))
#define	GetWindowVisRgn(aWindowPtr,aRgnHandle)	(GetPortVisibleRegion (GetWindowPort (aWindowPtr), (aRgnHandle)))

//
//	Control Manager Helpers
//

#define	MyGetControlDataHandle(cntl)							(GetControlDataHandle (cntl))
#define	MyGetControlBounds(cntl,aRectPtr)					(GetControlBounds ((cntl),(aRectPtr)))
#define	MyIsControlVisible(cntl)									(IsControlVisible (cntl))
#define	MyGetControlOwner(cntl)										(GetControlOwner (cntl))
#define	MyGetControlHilite(cntl)									(GetControlHilite (cntl))
#define	MySetControlBounds(cntl,aRectPtr)					SetControlBounds ((cntl),(aRectPtr))
#define	MySetControlDataHandle(cntl,aHandle)			SetControlDataHandle ((cntl), (aHandle))
#define	MyGetControlPopupMenuHandle(cntl)					(GetControlPopupMenuHandle(cntl))
#define	MySetControlVisibility(cntl,aBool,aBool2)	SetControlVisibility ((cntl), (aBool), (aBool2))
#define ZapControlDataHandle(cntl)								do{ if (GetControlDataHandle (cntl)) { DisposeHandle ((void *)(GetControlDataHandle (cntl))); SetControlDataHandle ((cntl), nil); }} while (0)


//
//	Menu Manager Helpers
//

#define	EnableItem	MacEnableMenuItem
#define	DisableItem	DisableMenuItem
#define	IsValidMenuHandle(aMenuRef)						(aMenuRef)
#define	MyIsValidMenuHandle(aMenuRef)					IsValidMenuHandle (aMenuRef)
#define	MyGetMenuID(aMenuRef)									(GetMenuID (aMenuRef))
#define	MyGetMenuTitle(aMenuRef,aStr255)			(GetMenuTitle ((aMenuRef), (aStr255)))
#define	MyGetMenuWidth(aMenuRef)							(GetMenuWidth (aMenuRef))
#define	MySetMenuID(aMenuRef,aMenuID)					SetMenuID ((aMenuRef), (aMenuID))


//
//	Dialog Manager Helpers
//

#define	MyGetDialogPortBounds(dp,aRectPtr)		(GetWindowPortBounds (GetDialogPort (dp), (aRectPtr)))
#define	MyGetDialogKeyboardFocusItem(dp)			(GetDialogKeyboardFocusItem (dp))
#define	MyGetDialogTextEditHandle(dp)					(GetDialogTextEditHandle (dp))

//
//	AppleEvent Helpers
//

#define	MyAEGetDescData(aDescPtr,aDescType,aVoidPtr,aSize)		AEGetDescData ((aDescPtr), (aDescType), (aVoidPtr), (aSize))
#define	MyAEGetDescDataSize(aDescPtr)													AEGetDescDataSize(aDescPtr)


//
//	OpenTransport Helpers
//

#define	MyOTAsyncOpenInternetServices(aOTConfigurationPtr,aOTOpenFlags,aOTNotifyUPP,aVoidPtr,aOTClientContextPtr)							\
					OTAsyncOpenInternetServices((aOTConfigurationPtr),(aOTOpenFlags),(aOTNotifyUPP),(aVoidPtr),(aOTClientContextPtr))	
#define	MyOTAsyncOpenEndpoint(aOTConfigurationPtr,aOTOpenFlags,aTEndpointInfoPtr,aOTNotifyUPP,aVoidPtr,aOTClientContextPtr)		\
					OTAsyncOpenEndpoint((aOTConfigurationPtr),(aOTOpenFlags),(aTEndpointInfoPtr),(aOTNotifyUPP),(aVoidPtr),(aOTClientContextPtr))
#define	MyOTOpenEndpoint(aOTConfigurationPtr,aOTOpenFlags,aTEndpointInfoPtr,aOSStatusPtr,aOTClientContextPtr)									\
					OTOpenEndpoint((aOTConfigurationPtr),(aOTOpenFlags),(aTEndpointInfoPtr),(aOSStatusPtr),(aOTClientContextPtr))


//
//	Help Manager Helpers (redundant redundant)
//
#ifdef TARGET_CARBON_NO_BALLON_HELP
#define MyHMRemoveBalloon				NotSupportedInCarbon
#define	MyHMIsBalloon						NotSupportedInCarbon
#define	MyHMSetBalloons					NotSupportedInCarbon
#define	MyHMGetBalloons					NotSupportedInCarbon
#define	HMGetHelpMenuHandle
#else
#define MyHMRemoveBalloon				HMRemoveBalloon
#define	MyHMIsBalloon						HMIsBalloon
#define	MyHMSetBalloons					HMSetBalloons
#define	MyHMGetBalloons					HMGetBalloons
#endif

#endif
