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

#include "carbonutil.h"
#define FILE_NUM 122

#pragma segment carbonutil

void GetControlListLo(Handle controlList, ControlHandle theControl);
static void PMRectToRect(PMRect * pmRect, Rect * r);

RgnHandle gAccessorRgn, gEmptyRgn;

/**********************************************************************
 * InitCarbonUtil - initialize data for Carbon utilities
 **********************************************************************/
void InitCarbonUtil(void)
{
	gAccessorRgn = NewRgn();
	gEmptyRgn = NewRgn();
	if (!gAccessorRgn || !gEmptyRgn)
		DieWithError(MEM_ERR, MemError());
}


/**********************************************************************
 * Accesor functions for regions.
 * !!! Important. They all use and return the gAccessorRgn region
 * Be careful not to save this value or call any of these functions more than
 * once in the same statement.
 **********************************************************************/
// MyGetPortVisibleRegion - get port's visRgn
RgnHandle MyGetPortVisibleRegion(CGrafPtr port)
{
	return GetPortVisibleRegion(port, gAccessorRgn);
}

// MyGetWindowStructureRegion - get window's strucRgn
RgnHandle MyGetWindowStructureRegion(WindowRef window)
{
	GetWindowRegion(window, kWindowStructureRgn, gAccessorRgn);
	return gAccessorRgn;
}

// MyGetWindowContentRegion - get window's contentRgn
RgnHandle MyGetWindowContentRegion(WindowRef window)
{
	GetWindowRegion(window, kWindowContentRgn, gAccessorRgn);
	return gAccessorRgn;
}

// MyGetWindowUpdateRegion - get window's UpdateRgn
RgnHandle MyGetWindowUpdateRegion(WindowRef window)
{
	GetWindowRegion(window, kWindowUpdateRgn, gAccessorRgn);
	return gAccessorRgn;
}

// SetEmptyVisRgn - set port's visRgn to nil region
void SetEmptyVisRgn(CGrafPtr port)
{
	SetPortVisibleRegion(port, gEmptyRgn);
}

// SetEmptyClipRgn - set port's clipRgn to nil region
void SetEmptyClipRgn(CGrafPtr port)
{
	SetPortClipRegion(port, gEmptyRgn);
}


/**********************************************************************
 * GetScreenBounds - get screen bounds
 **********************************************************************/
Rect GetScreenBounds(void)
{
	BitMap screenBits;

	screenBits = *GetQDGlobalsScreenBits(&screenBits);
	return screenBits.bounds;
}

/**********************************************************************
 * GetCWMgrPort - make a wmgr port
 **********************************************************************/
void GetCWMgrPort(CGrafPtr * wPort)
{
	static CGrafPtr wMgrPort;
	static Rect rWMgrPort;

	if (!wMgrPort)
		MyCreateNewPort(wMgrPort);
	if (wMgrPort) {
		//      Set up size and position of port
		Rect gdRect;
		GDHandle gd;

		gd = GetMainDevice();
		if (gd) {
			gdRect = (*gd)->gdRect;
			if (!EqualRect(&gdRect, &rWMgrPort)) {
				GrafPtr savePort;

				GetPort(&savePort);
				SetPort(wMgrPort);
				MovePortTo(gdRect.left, gdRect.top);
				PortSize(RectWi(gdRect), RectHi(gdRect));
				SetPort(savePort);
				rWMgrPort = gdRect;
			}
		}
	}
	*wPort = wMgrPort;
}

void GetWMgrPort(GrafPtr * wPort)
{
	GetCWMgrPort((CGrafPtr *) wPort);
}

/**********************************************************************
 * GetWindowGoAwayFlag - does window have a close box?
 **********************************************************************/
pascal Boolean GetWindowGoAwayFlag(WindowRef win)
{
	WindowAttributes attr;
	GetWindowAttributes(win, &attr);
	return (attr & kWindowCloseBoxAttribute) != 0;
}

/**********************************************************************
 * GetScrap - old Scrap Manager replacement function
 **********************************************************************/
long GetScrap(Handle h, ScrapFlavorType flavorType, SInt32 * offset)
{
	ScrapRef scrap;
	Size byteCount = 0;
	OSErr err;

	GetCurrentScrap(&scrap);
	GetScrapFlavorSize(scrap, flavorType, &byteCount);
	if (h) {
		SetHandleSize(h, byteCount);
		if (!(err = MemError())) {
			GetScrapFlavorData(scrap, flavorType, &byteCount,
					   LDRef(h));
			UL(h);
		}
	}
	*offset = 0;
	if (err == cantGetFlavorErr)
		err = noTypeErr;
	return err ? err : byteCount;
}

/**********************************************************************
 * PutScrap - old Scrap Manager replacement function
 **********************************************************************/
OSStatus PutScrap(SInt32 byteCount, ScrapFlavorType flavorType,
		  const void *source)
{
	ScrapRef scrap;

	GetCurrentScrap(&scrap);
	return PutScrapFlavor(scrap, flavorType, kScrapFlavorMaskNone,
			      byteCount, source);
}

/**********************************************************************
 * The functions PMGetPhysicalPaperSize and PMGetPhysicalPageSize return
 * the sizes as PMRect, which uses a double for each field. We would rather
 * use rect.
 **********************************************************************/
#ifdef NEVER
OSStatus PMGetPhysicalPaperSizeAsRect(PMPageFormat pageFormat,
				      Rect * paperSize)
{
	PMRect pmRect;
	PMGetPhysicalPaperSize(pageFormat, &pmRect);
	PMRectToRect(&pmRect, paperSize);
}

OSStatus PMGetPhysicalPageSizeAsRect(PMPageFormat pageFormat,
				     Rect * pageSize)
{
	PMRect pmRect;
	PMGetPhysicalPageSize(pageFormat, &pmRect);
	PMRectToRect(&pmRect, pageSize);
}
#endif
OSStatus PMGetAdjustedPaperSizeAsRect(PMPageFormat pageFormat,
				      Rect * paperSize)
{
	OSStatus err;
	PMRect pmRect;
	err = PMGetAdjustedPaperRect(pageFormat, &pmRect);
	PMRectToRect(&pmRect, paperSize);
	return err;
}

OSStatus PMGetAdjustedPageSizeAsRect(PMPageFormat pageFormat,
				     Rect * pageSize)
{
	OSStatus err;
	PMRect pmRect;
	err = PMGetAdjustedPageRect(pageFormat, &pmRect);
	PMRectToRect(&pmRect, pageSize);
	return err;
}

static void PMRectToRect(PMRect * pmRect, Rect * r)
{
	r->top = pmRect->top;
	r->left = pmRect->left;
	r->bottom = pmRect->bottom;
	r->right = pmRect->right;
}


/**********************************************************************
 * SavePortClipRegion - save copy of port clipRgn
 **********************************************************************/
RgnHandle SavePortClipRegion(CGrafPtr port)
{
	RgnHandle rgn;

	if (rgn = NewRgn())
		GetPortClipRegion(port, rgn);
	return rgn;
}

/**********************************************************************
 * RestorePortClipRegion - restore port clipRgn and dispose copy
 **********************************************************************/
void RestorePortClipRegion(CGrafPtr port, RgnHandle rgn)
{
	if (rgn) {
		SetPortClipRegion(port, rgn);
		DisposeRgn(rgn);
	}
}

//
//      AppleEvent help
//

/**********************************************************************
 * AEGetDescDataHandle - return AEDesc as copy of handle
 **********************************************************************/
OSErr AEGetDescDataHandle(AEDesc * theAEDesc, Handle * handle)
{
	Handle dataHandle = nil;
	OSErr theError = noErr;
	Size size = AEGetDescDataSize(theAEDesc);

	dataHandle = NuHandle(size);
	theError = MemError();
	if (!theError) {
		theError =
		    AEGetDescData(theAEDesc, LDRef(dataHandle), size);
		UL(dataHandle);
	}
	if (theError)
		ZapHandle(dataHandle);
	*handle = dataHandle;
	return (theError);
}

/**********************************************************************
 * AEDisposeDescDataHandle - dispose of AEDesc copy handle, only for Carbon
 **********************************************************************/
void AEDisposeDescDataHandle(Handle h)
{
	DisposeHandle(h);
}

OSErr MyAECreateDesc(DescType typeCode, void *dataPtr, Size dataSize,
		     AEDesc * result)
{
	return (AECreateDesc(typeCode, dataPtr, dataSize, result));
}


//
//      QuickDraw Utilities
//

OSStatus InvalWindowPort(WindowPtr theWindow)
{
	Rect theRect;

	return (InvalWindowRect
		(theWindow, GetWindowPortBounds(theWindow, &theRect)));
}


Rect *GetQDGlobalsScreenBitsBounds(Rect * bounds)
{
	BitMap screenBits;

	screenBits = *GetQDGlobalsScreenBits(&screenBits);
	*bounds = screenBits.bounds;
	return (bounds);
}

Rect *GetUpdateRgnBounds(WindowPtr theWindow, Rect * bounds)
{
#ifdef TARGET_CARBON_CANT_YET_GET_UPDATE_BOUNDS_DIRECTLY
	RgnHandle updateRgn;

	if (updateRgn = NewRgn()) {
		if (!GetWindowRegion
		    (theWindow, kWindowUpdateRgn, updateRgn))
			GetRegionBounds(updateRgn, bounds);
		DisposeRgn(updateRgn);
	}
#else

	GetWindowBounds(theWindow, kWindowUpdateRgn, bounds);
#endif
	return (bounds);
}

Rect *GetVisibleRgnBounds(WindowPtr theWindow, Rect * bounds)
{
	RgnHandle visRgn;

	if (visRgn = NewRgn()) {
		GetRegionBounds(GetWindowVisRgn(theWindow, visRgn),
				bounds);
		DisposeRgn(visRgn);
	}
	return (bounds);
}

Rect *GetContentRgnBounds(WindowPtr theWindow, Rect * bounds)
{
	GetWindowBounds(theWindow, kWindowContentRgn, bounds);
	return (bounds);
}

Rect *GetStructureRgnBounds(WindowPtr theWindow, Rect * bounds)
{
	GetWindowBounds(theWindow, kWindowStructureRgn, bounds);
	return (bounds);
}

Boolean HasUpdateRgn(WindowPtr theWindow)
{
	RgnHandle updateRgn;
	Boolean hasUpdate;

	hasUpdate = false;
	if (updateRgn = NewRgn()) {
		if (!GetWindowRegion
		    (theWindow, kWindowUpdateRgn, updateRgn))
			hasUpdate = !EmptyRgn(updateRgn);
		DisposeHandle(updateRgn);
	}
	return (hasUpdate);
}

short MyGetWindowTitleWidth(WindowPtr theWindow)
{
	RgnHandle textRgn;
	Rect bounds;
	short width;

	width = 0;
	if (textRgn = NewRgn()) {
		GetWindowRegion(theWindow, kWindowTitleTextRgn, textRgn);
		GetRegionBounds(textRgn, &bounds);
		width = bounds.right - bounds.left;
		DisposeRgn(textRgn);
	}
	return (width);
}


void MySetDialogFont(short fontNum)
{
#ifndef TARGET_CARBON_GOING_AWAY_BUT_CURRENTLY_IN_HEADERS
	SetDialogFont(fontNum);
#endif
}

//
//      Control utilities
//



//
//      MyGetControlList
//
//              Builds a control list which is an array of ControlHandles
//              we can walk ourselves.  This is stored in a Handle within
//              the window structure under carbon.  While we certainly
//              could cache this information, we don't currently so as to
//              avoid rewriting all of the dialog manager calls that fiddle
//              with the control list.  Instead, we always destroy the old
//              list and create a new one.  Not very efficient, but...
//
ControlHandle GetControlList(WindowPtr theWindow)
{
	MyWindowPtr win;
	ControlHandle rootControl;

	rootControl = nil;
	if (win = GetWindowMyWindowPtr(theWindow)) {
		ZapHandle(win->hControlList);

		if (win->hControlList = NuHandle(0))
			if (!GetRootControl(theWindow, &rootControl))
				GetControlListLo(win->hControlList,
						 rootControl);
	}
	return (rootControl);
}

void GetControlListLo(Handle controlList, ControlHandle theControl)
{
	ControlHandle subControl;
	UInt16 count, index;

	count = 0;
	if (theControl)
		if (!PtrPlusHand
		    (&theControl, controlList, sizeof(ControlHandle)))
			if (!CountSubControls(theControl, &count))
				for (index = 1; index <= count; ++index) {
					if (!GetIndexedSubControl
					    (theControl, index,
					     &subControl))
						GetControlListLo
						    (controlList,
						     subControl);
				}
}

//
//      MyGetNextControl
//
ControlHandle GetNextControl(ControlHandle theControl)
{
	MyWindowPtr win;
	ControlHandle *controlPtr;
	UInt16 count, i;

	if (theControl)
		if (win =
		    GetWindowMyWindowPtr(GetControlOwner(theControl)))
			if (win->hControlList) {
				controlPtr =
				    (ControlHandle *) *
				    (win->hControlList);
				count =
				    (GetHandleSize(win->hControlList) /
				     sizeof(ControlHandle)) - 1;
				for (i = 0; i < count; ++i, ++controlPtr)
					if (*controlPtr == theControl) {
						++controlPtr;
						return (*controlPtr);
					}
			}
	return (nil);
}



OSStatus InvalControl(ControlHandle theControl)
{
	Rect theRect;

	return (InvalWindowRect
		(GetControlOwner(theControl),
		 GetControlBounds(theControl, &theRect)));
}

short ControlHi(ControlHandle theControl)
{
	Rect controlRect;

	GetControlBounds(theControl, &controlRect);
	return (RectHi(controlRect));
}

short ControlWi(ControlHandle theControl)
{
	Rect controlRect;

	GetControlBounds(theControl, &controlRect);
	return (RectWi(controlRect));
}


//
//      List Manager Helpers
//
ListHandle CreateNewList(ListDefUPP ldefUPP,	// For Carbon
			 short theProc,	// For non-Carbon
			 Rect * rView,
			 ListBounds * dataBounds,
			 Point cSize,
			 WindowPtr theWindow,
			 Boolean drawIt,
			 Boolean hasGrow,
			 Boolean scrollHoriz, Boolean scrollVert)
{
	ListDefSpec ldefSpec;
	ListHandle theList = nil;

	ldefSpec.defType = kListDefUserProcType;
	ldefSpec.u.userProc = ldefUPP;
	return (CreateCustomList
		(rView, dataBounds, cSize, &ldefSpec, theWindow, drawIt,
		 hasGrow, scrollHoriz, scrollVert,
		 &theList) ? nil : theList);
}

//
//      Scrap Manager Helpers
//
/**********************************************************************
 * IsScrapFull - anything on the scrap?
 **********************************************************************/
Boolean IsScrapFull(void)
{
	ScrapRef scrap;
	UInt32 scrapFlavorCount;

	GetCurrentScrap(&scrap);
	GetScrapFlavorCount(scrap, &scrapFlavorCount);
	return scrapFlavorCount > 0;
}


//
//      Not supported in carbon
//
OSStatus NotSupportedInCarbon(...)
{
	return (noErr);
}

/************************************************************************
 * MyIsWindowVisible - is window visible even if application is hidden
 *  
 *  Has same functionality as IsWindowVisible under classic
 ************************************************************************/
Boolean MyIsWindowVisible(WindowRef win)
{
#undef IsWindowVisible
	WindowLatentVisibility latentVisible;

	return IsWindowLatentVisible ?
	    (IsWindowLatentVisible(win, &latentVisible) || latentVisible) :
	    IsWindowVisible(win);
}
