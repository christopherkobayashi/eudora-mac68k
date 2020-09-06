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

#include "utl.h"
#define FILE_NUM 42
/*______________________________________________________________________

	 utl.c - Utilities.
	
	 Copyright � 1988, 1989, 1990 Northwestern University.	Permission is granted
	 to use this code in your own projects, provided you give credit to both
	 John Norstad and Northwestern University in your about box or document.
	
	 This module exports miscellaneous reusable utility routines.
_____________________________________________________________________*/




#pragma segment utl

/*______________________________________________________________________

	 Global Variables.
_____________________________________________________________________*/


static SysEnvRec TheWorld;	/* system environment record */
static Boolean GotSysEnviron = false;
																		/* true if sys environ has been
																		   gotten */


static CursHandle *CursArray;	/* ptr to array of cursor handles */
static short NumCurs;		/* number of cursors to rotate */
static short TickInterval;	/* number of ticks between rotations */
static short CurCurs;		/* index of current cursor */
static short LastTick;		/* tick count at loast rotation */

static ModalFilterProcPtr Filter;	/* dialog filter proc */
static short CancelItem;	/* item number of cancel button */

/*______________________________________________________________________

	 utl_CenterDlogRect - Center a dialog rectangle.
	
	 Entry: 	rect = rectangle.
						centerMain = true to center on main (menu bar) screen.
						centerMain = false to center on the screen containing
							 the maximum intersection with the frontmost window.
	
	 Exit:		rect = rectangle offset so that it is centered on
							 the specified screen, with twice as much space below
							 the rect as above.
							
	 See HIN 6.
_____________________________________________________________________*/


void utl_CenterDlogRect(Rect * rect, Boolean centerMain)
{
	Rect screenRect;	/* screen rectangle */
	short mBHeight;		/* menu bar height */
	GDHandle gd;		/* gdevice */
	Rect windRect;		/* window rectangle */
	Boolean hasMB;		/* true if screen contains menu bar */

	mBHeight = utl_GetMBarHeight();
	if (centerMain) {
		screenRect = GetScreenBounds();
	} else {
		utl_GetWindGD(FrontWindow(), &gd, &screenRect, &windRect,
			      &hasMB);
		if (!hasMB)
			mBHeight = 0;
	};
	OffsetRect(rect,
		   (screenRect.right + screenRect.left - rect->right -
		    rect->left) >> 1,
		   (screenRect.bottom +
		    ((screenRect.top + mBHeight - rect->top) << 1) -
		    rect->bottom + 7) / 3);
}

/*______________________________________________________________________

	 utl_CenterRect - Center a rectangle on the main screen.
	
	 Entry: 	rect = rectangle.
	
	 Exit:		rect = rectangle offset so that it is centered on
							 the main screen.
_____________________________________________________________________*/


void utl_CenterRect(Rect * rect)
{
	Rect screenRect;	/* main screen rectangle */
	short mBHeight;		/* menu bar height */

	mBHeight = utl_GetMBarHeight();
	screenRect = GetScreenBounds();
	OffsetRect(rect,
		   (screenRect.right + screenRect.left - rect->right -
		    rect->left) >> 1,
		   (screenRect.bottom - screenRect.top + mBHeight -
		    rect->bottom - rect->top) >> 1);
}

#ifdef NEVER
/*______________________________________________________________________

	 utl_CheckPack - Check to see if a package exists.
	
	 Entry: 	packNum = package number.
						preload = true to preload package.
							
	 Exit:		function result = true if package exists.
_____________________________________________________________________*/


Boolean utl_CheckPack(short packNum, Boolean preload)
{
	short trapNum;		/* trap number */
	Handle h;		/* handle to PACK resource */

	/* Check to make sure the trap exists, by comparing its trap address to
	   the trap address of the unimplemented trap. */

	trapNum = packNum + 0x1e7;
	if (NGetTrapAddress(trapNum & 0x3ff, ToolTrap) ==
	    NGetTrapAddress(_Unimplemented & 0x3ff, ToolTrap))
		return false;

	/* Check to make sure the package exists on the System file or in
	   ROM.        If it's not in ROM make it nonpurgeable, if requested. */

	if (preload) {
		if (utl_Rom64()) {
			h = GetResource_('PACK', packNum);
			if (!h)
				return false;
			HNoPurge_(h);
		} else {
			*(unsigned short *) RomMapInsert = 0xFF00;
			h = GetResource_('PACK', packNum);
			if (!h)
				return false;
			if ((*(unsigned long *) h & 0x00FFFFFF) <
			    *(unsigned long *) ROMBase) {
				h = GetResource_('PACK', packNum);
				HNoPurge_(h);
			};
		};
		return true;
	} else {
		SetResLoad(false);
		if (!utl_Rom64())
			*(unsigned short *) RomMapInsert = 0xFF00;
		h = GetResource_('PACK', packNum);
		SetResLoad(true);
		if (h)
			return true;
		else
			return false;
	};
}
#endif

/*______________________________________________________________________

	 utl_CopyPString - Copy Pascal String.
	
	 Entry: 	dest = destination string.
						source = source string.
_____________________________________________________________________*/


void utl_CopyPString(Str255 dest, Str255 source)
{
	BMD(source, dest, *source + 1);
}

/*______________________________________________________________________

	 utl_CouldDrag - Determine if a window could be dragged to a location.
	
	 Entry: 	windRect = window rectangle, in global coords.
						offset = pixel offset used in DragRect calls.
	
	 Exit:		function result = true if the window could have been
							 dragged to the specified position.
							
	 This routine is used when restoring windows to saved positions.	According
	 to HIN 6, we must check to see if the window "could have been dragged to
	 the saved position."
	
	 The "offset" parameter is usually 4.  When initializing the boundary rectangle
	 for DragWindow calls, normally the boundary rectangle of the desktop gray
	 region is inset by 4 pixels.  If some value other than 4 is used, it should
	 be passed to CouldDrag as the "offset" parameter.
	
	 The algorithm used is the following:  The routine computes the four squares
	 at the corners of the title bar.  "true" is returned if and only if at least one 
	 of these four squares is completely contained within the desktop gray region.
	
	 Three pixels are added to the offset to err on the side of requiring a larger
	 portion of the drag bar to be visible. 
_____________________________________________________________________*/


Boolean utl_CouldDrag(WindowPtr theWindow, Rect * windRect, short offset,
		      short titleBarHeight, short leftRimWidth)
{
	RgnHandle rgn = nil;	/* scratch region handle */
	Boolean could;		/* function result */
	short corner;		/* which corner */
	Rect r;			/* corner rectangle */
	RgnHandle grayRgn;

	rgn = NewRgn();
	grayRgn = NewRgn();
	if (grayRgn) {
		CopyRgn(GetGrayRgn(), grayRgn);
		DockedWinRemove(grayRgn, theWindow);
	}
	{
		could = false;
		offset += 3;
		for (corner = 1; corner <= 4; corner++) {
			switch (corner) {
			case 1:
				r.top = windRect->top - titleBarHeight;
				r.left = windRect->left;
				break;
			case 2:
				r.top = windRect->top - offset;
				r.left = windRect->left;
				break;
			case 3:
				r.top = windRect->top - titleBarHeight;
				r.left = windRect->right - offset;
				break;
			case 4:
				r.top = windRect->top - offset;
				r.left = windRect->right - offset;
				break;
			};
			r.bottom = r.top + offset;
			r.right = r.left + offset;
			RectRgn(rgn, &r);
			DiffRgn(rgn, grayRgn, rgn);
			if (EmptyRgn(rgn)) {
				could = true;
				break;
			};
		};
	}
	if (rgn)
		DisposeRgn(rgn);
	if (grayRgn)
		DisposeRgn(grayRgn);
	return could;
}

#ifdef NEVER
/*______________________________________________________________________

	 utl_FixStdFile - Fix Standard File Pacakge.
	
	 This routine should be called before calling the Standard File 
	 package if there's any chance that SFSaveDisk might specify
	 a volume that has been umounted.  Standard File gets confused if this
	 happens and presents an alert telling the user that a "system error"
	 has occurred.
	
	 This routine checks to make sure that SFSaveDisk specifies a volume
	 that is still mounted.  If not, it sets it to the first mounted
	 volume, and it sets CurDirStore to the root directory on that volume.
_____________________________________________________________________*/


void utl_FixStdFile(void)
{
	ParamBlockRec vBlock;	/* vol info param block */

	vBlock.volumeParam.ioNamePtr = nil;
	vBlock.volumeParam.ioVRefNum = -*(short *) SFSaveDisk;
	vBlock.volumeParam.ioVolIndex = 0;
	if (PBGetVInfo(&vBlock, false)) {
		vBlock.volumeParam.ioVolIndex = 1;
		(void) PBGetVInfo(&vBlock, false);
		*(short *) SFSaveDisk = -vBlock.volumeParam.ioVRefNum;
		if (*(short *) FSFCBLen > 0)
			*(long *) CurDirStore = fsRtDirID;
	};
}
#endif

/*______________________________________________________________________

	 utl_FlashButton - Flash Dialog Button.
	
	 Entry: 	theDialog = pointer to dialog.
						itemNo = item number of button to flash.
						
	 The push button is inverted for 8 ticks.  See HIN #10.
_____________________________________________________________________*/


void utl_FlashButton(DialogPtr theDialog, short itemNo)
{
	short itemType;		/* item type */
	Handle item;		/* item handle */
	Rect box;		/* item rectangle */
	short roundFactor;	/* rounded corner factor for InvertRoundRect */
	long tickEnd;		/* tick count to end flash */

	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetPort(GetDialogPort(theDialog));
	roundFactor = (box.bottom - box.top) >> 1;
	InvertRoundRect(&box, roundFactor, roundFactor);
	tickEnd = TickCount() + 8;
	while (TickCount() < tickEnd);
	InvertRoundRect(&box, roundFactor, roundFactor);
}

/*______________________________________________________________________

	 utl_FrameItem
	
	 Entry: 	theWindow = pointer to dialog window.
						itemNo = dialog item number.
	
	 Exit:		dialog item rectangle framed.
						
	 This function is for use as a Dialog Manager user item procedure.
	 It is particularly useful for drawing rules (straight lines).	Simply
	 define the user item rectangle with bottom=top+1 or right=left+1.
_____________________________________________________________________*/


pascal void utl_FrameItem(DialogPtr theDialog, short itemNo)
{
	short itemType;		/* item type */
	Handle item;		/* item handle */
	Rect box;		/* item rectangle */

	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	FrameRect(&box);
}

#ifdef NEVER
/*______________________________________________________________________

	 utl_GetApplVol - Get the volume reference number of the application volume.
	
	 Exit:		function result = volume reference number of the volume 
							 containing the current application.
_____________________________________________________________________*/


short utl_GetApplVol(void)
{
	short vRefNum;		/* vol ref num */

	GetVRefNum(*(short *) (CurMap), &vRefNum);
	return vRefNum;
}
#endif

/*______________________________________________________________________

	 utl_GetFontNumber - Get Font Number.
	
	 Entry: 	fontName = font name.
	
	 Exit:		function result = true if font exists.
						fontNum = font number.
						
	 Copied from TN 191.
_____________________________________________________________________*/


Boolean utl_GetFontNumber(Str255 fontName, short *fontNum)
{
	Str255 systemFontName;

	GetFNum(fontName, fontNum);
	if (*fontNum) {
		return true;
	} else {
		GetFontName(0, systemFontName);
		return StringSame(fontName, systemFontName);
	};
}

/*______________________________________________________________________

	 utl_GetMBarHeight - Get Menu Bar Height
	
	 Exit:		function result = menu bar height.
	
	 See TN 117.
_____________________________________________________________________*/


short utl_GetMBarHeight(void)
{
	static short mBHeight = 0;

	if (!mBHeight) {
		mBHeight = utl_Rom64()? 20 : GetMBarHeight();
	};
	return mBHeight;
}

/*______________________________________________________________________

	 utl_GetNewControl_ - Get New Control.
	
	 Entry: 	controlID = resource id of CNTL resource.
						theWindow = pointer to window record.
						
	 Exit:		function result = handle to new control record.
						
	 This routine is identical to the Control Manager routine GetNewControl_,
	 except it does not release or make purgeable the CNTL resource.
_____________________________________________________________________*/


ControlHandle utl_GetNewControl(short controlID, WindowPtr theWindow)
{
	Rect boundsRect;	/* boundary rectangle */
	short value;		/* initial control value */
	Boolean visible;	/* true if visible */
	short max;		/* max control value */
	short min;		/* min control value */
	short procID;		/* window proc id */
	long refCon;		/* refCon field for window record */
	Str255 title;		/* window title */
	Handle theRez;		/* handle to CNTL resource */

	theRez = GetResource_('CNTL', controlID);
	boundsRect = *(Rect *) (*theRez);
	value = *(short *) (*theRez + 8);
	visible = *(Boolean *) (*theRez + 10);
	max = *(short *) (*theRez + 12);
	min = *(short *) (*theRez + 14);
	procID = *(short *) (*theRez + 16);
	refCon = *(long *) (*theRez + 18);
	utl_CopyPString(title, *theRez + 22);
	return NewControl(theWindow, &boundsRect, title, visible, value,
			  min, max, procID, refCon);
}

/*______________________________________________________________________

	 utl_GetNewDialog - Get New Dialog.
	
	 Entry: 	dialogID = resource id of DLOG resource.
						dStorage = pointer to dialog record.
						behind = window to insert in back of.
						
	 Exit:		function result = pointer to new dialog record.
						
	 This routine is identical to the Dialog Manager routine GetNewDialog,
	 except it does not release or make purgeable the DLOG resource.
_____________________________________________________________________*/


DialogPtr utl_GetNewDialog(short dialogID, Ptr dStorage, WindowPtr behind)
{
	Rect boundsRect;	/* boundary rectangle */
	short procID;		/* window proc id */
	Boolean visible;	/* true if visible */
	Boolean goAwayFlag;	/* true if window has go away box */
	long refCon;		/* refCon field for window record */
	Str255 title;		/* window title */
	Handle theRez;		/* handle to DLOG resource */
	short itemID;		/* rsrc id of item list */
	Handle items;		/* handle to item list */

	theRez = GetResource_('DLOG', dialogID);
	boundsRect = *(Rect *) (*theRez);
	procID = *(short *) (*theRez + 8);
	visible = *(Boolean *) (*theRez + 10);
	goAwayFlag = *(Boolean *) (*theRez + 12);
	refCon = *(long *) (*theRez + 14);
	itemID = *(short *) (*theRez + 18);
	utl_CopyPString(title, *theRez + 20);
	items = GetResource_('DITL', itemID);
	return NewDialog(dStorage, &boundsRect, title, visible, procID,
			 behind, goAwayFlag, refCon, items);
}

/*______________________________________________________________________

	 utl_GetNewWindow - Get New Window.
	
	 Entry: 	windowID = resource id of WIND resource.
						wStorage = pointer to window record.
						behind = window to insert in back of.
						
	 Exit:		function result = pointer to new window record.
						
	 This routine is identical to the Window Manager routine GetNewWindow,
	 except it does not release or make purgeable the WIND resource.
_____________________________________________________________________*/


WindowPtr utl_GetNewWindow(short windowID, Ptr wStorage, WindowPtr behind)
{
	Rect boundsRect;	/* boundary rectangle */
	short procID;		/* window proc id */
	Boolean visible;	/* true if visible */
	Boolean goAwayFlag;	/* true if window has go away box */
	long refCon;		/* refCon field for window record */
	Str255 title;		/* window title */
	Handle theRez;		/* handle to WIND resource */

	theRez = GetResource_('WIND', windowID);
	boundsRect = *(Rect *) (*theRez);
	procID = *(short *) (*theRez + 8);
	visible = *(Boolean *) (*theRez + 10);
	goAwayFlag = *(Boolean *) (*theRez + 12);
	refCon = *(long *) (*theRez + 14);
	utl_CopyPString(title, *theRez + 18);
	return NewWindow(wStorage, &boundsRect, title, visible, procID,
			 behind, goAwayFlag, refCon);
}

#ifdef NEVER
/*______________________________________________________________________

	 utl_GetSysVol - Get the volume reference number of the system volume.
	
	 Exit:		function result = volume reference number of the volume 
							 containing the currently active system file.
_____________________________________________________________________*/


short utl_GetSysVol(void)
{

	short vRefNum;		/* vol ref num */

	GetVRefNum(*(short *) (SysMap), &vRefNum);
	return vRefNum;
}
#endif

/*______________________________________________________________________

	 utl_GetWindGD - Get the GDevice containing a window.
	
	 Entry: 	theWindow = pointer to window.
	
	 Exit:		gd = handle to GDevice, or nil if no color QD.
						screenRect = bounding rectangle of GDevice, or
							 qd.screenBits.bounds if no color QD.
						windRect = content rectangle of window, in global
							 coords.
						hasMB = true if this screen contains the menu bar.
						
	 The routine determines the GDevice (screen) containing the maximum
	 intersection with a window.	See TN 79.
_____________________________________________________________________*/


void utl_GetWindGD(WindowPtr theWindow, GDHandle * gd,
		   Rect * screenRect, Rect * windRect, Boolean * hasMB)
{
	GrafPtr savePort;	/* saved grafport */

	GetPortBounds(GetWindowPort(theWindow), windRect);
	GetPort(&savePort);
	SetPort(GetWindowPort(theWindow));
	LocalToGlobal((Point *) & windRect->top);
	LocalToGlobal((Point *) & windRect->bottom);
	utl_GetRectGDStuff(gd, screenRect, windRect,
			   TitleBarHeight(theWindow),
			   LeftRimWidth(theWindow), hasMB);
	SetPort_(savePort);
}

void utl_GetRectGDStuff(GDHandle * gd,
			Rect * screenRect, Rect * windRect,
			short titleBarHeight, short leftRimWidth,
			Boolean * hasMB)
{
	Rect sectRect;		/* intersection rect */
	GDHandle curDevice;	/* current GDevice */
	GDHandle dominantDevice;	/* dominant GDevice */
	long sectArea;		/* intersection area */
	long maxArea;		/* max intersection area */

	if (utl_HaveColor()) {
		windRect->top -= titleBarHeight;
		windRect->left -= leftRimWidth;
		curDevice = GetDeviceList();
		maxArea = 0;
		dominantDevice = nil;
		while (curDevice) {
			if (TestDeviceAttribute(curDevice, screenDevice) &&
			    TestDeviceAttribute(curDevice, screenActive)) {
				SectRect(windRect, &(**curDevice).gdRect,
					 &sectRect);
				sectArea =
				    (long) (sectRect.right -
					    sectRect.left) *
				    (long) (sectRect.bottom -
					    sectRect.top);
				if (sectArea > maxArea) {
					maxArea = sectArea;
					dominantDevice = curDevice;
				};
			};
			curDevice = GetNextDevice(curDevice);
		};
		windRect->top += titleBarHeight;
		windRect->left += leftRimWidth;
		if (dominantDevice) {
			*gd = dominantDevice;
			*screenRect = (**dominantDevice).gdRect;
			*hasMB = dominantDevice == GetMainDevice();
		} else {
			*gd = nil;
			*screenRect = GetScreenBounds();
			*hasMB = true;
		};
	} else {
		*gd = nil;
		*screenRect = GetScreenBounds();
		*hasMB = true;
	};
}

/*______________________________________________________________________

	 utl_GetRectGD - Get the GDevice containing a rect.
	
	 Entry: 	windRect - pointer to the rectangle
	
	 Exit:		gd = handle to GDevice, or nil if no color QD.
						
	 The routine determines the GDevice (screen) containing the maximum
	 intersection with a rect.	See TN 79.
_____________________________________________________________________*/


void utl_GetRectGD(Rect * windRect, GDHandle * gd)
{
	Rect sectRect;		/* intersection rect */
	GDHandle curDevice;	/* current GDevice */
	GDHandle dominantDevice;	/* dominant GDevice */
	long sectArea;		/* intersection area */
	long maxArea;		/* max intersection area */

	if (utl_HaveColor()) {
		curDevice = GetDeviceList();
		maxArea = 0;
		dominantDevice = nil;
		while (curDevice) {
			if (TestDeviceAttribute(curDevice, screenDevice) &&
			    TestDeviceAttribute(curDevice, screenActive)) {
				SectRect(windRect, &(**curDevice).gdRect,
					 &sectRect);
				sectArea =
				    (long) (sectRect.right -
					    sectRect.left) *
				    (long) (sectRect.bottom -
					    sectRect.top);
				if (sectArea > maxArea) {
					maxArea = sectArea;
					dominantDevice = curDevice;
				};
			};
			curDevice = GetNextDevice(curDevice);
		};
		if (dominantDevice) {
			*gd = dominantDevice;
		} else {
			*gd = nil;
		};
	} else {
		*gd = nil;
	};
}

/*______________________________________________________________________

	 utl_GetIndGD - Get the nth GDevice.
	
	 Entry: 	n = device index.
	
	 Exit:		gd = handle to GDevice, or nil if no color QD.
						screenRect = bounding rectangle of GDevice, or
							 qd.screenBits.bounds if no color QD.
						hasMB = true if this screen contains the menu bar.

_____________________________________________________________________*/


void utl_GetIndGD(short n, GDHandle * gd, Rect * screenRect,
		  Boolean * hasMB)
{
	GDHandle curDevice;	/* current GDevice */
	GDHandle dominantDevice;	/* dominant GDevice */

	if (utl_HaveColor()) {
		dominantDevice = nil;
		curDevice = GetDeviceList();
		while (curDevice && n--) {
			if (TestDeviceAttribute(curDevice, screenDevice) &&
			    TestDeviceAttribute(curDevice, screenActive))
				dominantDevice = curDevice;
			curDevice = GetNextDevice(curDevice);
		}
		if (dominantDevice) {
			if (gd)
				*gd = dominantDevice;
			if (screenRect)
				*screenRect = (**dominantDevice).gdRect;
			if (hasMB)
				*hasMB = dominantDevice == GetMainDevice();
		} else {
			if (gd)
				*gd = nil;
			if (screenRect)
				*screenRect = GetScreenBounds();
			if (hasMB)
				*hasMB = true;
		};
	} else {
		if (gd)
			*gd = nil;
		if (screenRect)
			*screenRect = GetScreenBounds();
		if (hasMB)
			*hasMB = true;
	};
}

/*______________________________________________________________________

	 utl_GetVolFilCnt - Get the number of files on a volume.
	
	 Entry: 	volRefNum = volume reference number of volume.
	
	 Exit:		function result = number of files on volume.
	
	 For serdver volumes this function always returns 0.
_____________________________________________________________________*/


long utl_GetVolFilCnt(short volRefNum)
{
	HParamBlockRec pBlock;	/* param block for PHBGetVInfo */

	pBlock.volumeParam.ioNamePtr = nil;
	pBlock.volumeParam.ioVRefNum = volRefNum;
	pBlock.volumeParam.ioVolIndex = 0;
	(void) PBHGetVInfo(&pBlock, false);
	return pBlock.volumeParam.ioVFilCnt;
}

/*______________________________________________________________________

	 utl_HaveColor - Determine if system has color QuickDraw.

	 Exit:		function result = true if we have color QD.
_____________________________________________________________________*/


Boolean utl_HaveColor(void)
{
#if 0
	SInt32 gestaltResult;

	if (!Gestalt(gestaltQuickdrawFeatures, &gestaltResult))
		return (gestaltResult & gestaltHasColor) != 0;
	return false;		//      shouldn't happen
#endif
	return true;
}

/*______________________________________________________________________

	 utl_InitSpinCursor - Initialize animated cursor.
	
	 Entry: 	cursArray = array of handles to cursors.
						numCurs = number of cursors to rotate.
						tickInterval = interval between cursor rotations.
_____________________________________________________________________*/


void utl_InitSpinCursor(CursHandle * cursArray, short numCurs,
			short tickInterval)
{
	CursHandle h;

	CursArray = cursArray;
	CurCurs = 0;
	NumCurs = numCurs;
	TickInterval = tickInterval;
	LastTick = TickCount();
	h = *cursArray;
	SetCursor(*h);
}

/*______________________________________________________________________

	 utl_InvalGrow - Invalidate Grow Icon.
	
	 Entry: 				theWindow = pointer to window.
	
	 This routine should be called before and after calling SizeWindow
	 for windows with grow icons.
_____________________________________________________________________*/


void utl_InvalGrow(WindowPtr theWindow)
{
	Rect r;			/* rect to be invalidated */

	GetPortBounds(GetWindowPort(theWindow), &r);
	r.top = r.bottom - 15;
	r.left = r.right - 15;
	InvalWindowRect(theWindow, &r);
}

/*______________________________________________________________________

	 utl_IsDAWindow - Check to see if a window is a DA.
	
	 Entry: 	theWindow = pointer to dialog window.
						
	
	 Exit:		function result = true if DA window.
_____________________________________________________________________*/


Boolean utl_IsDAWindow(WindowPtr theWindow)
{
	return GetWindowKind(theWindow) < 0;
}

/*______________________________________________________________________

	 utl_IsLaser - Check Printer for LaserWriter.
	
	 Entry: 	hPrint = handle to print record.
	
	 Exit:		function result = true if LaserWriter.
	
_____________________________________________________________________*/

#if 0
Boolean utl_IsLaser(THPrint hPrint)
{
	unsigned char wDev;	/* printer device */

	wDev = (**hPrint).prStl.wDev >> 8;
	return wDev == 3 || wDev == 4;
}
#endif

/*______________________________________________________________________

	 utl_PlugParams - Plug parameters into message.
	
	 Entry: 	line1 = input line.
						p0, p1, p2, p3 = parameters.
							
	 Exit:		line2 = output line.
							
	 This routine works just like the toolbox routine ParamText.
	 The input line may contain place-holders ^0, ^1, ^2, and ^3, 
	 which are replaced by the parameters p0-p3.	The input line
	 must not contain any other ^ characters.  The input and output lines
	 may not be the same string.	Pass nil for parameters which don't
	 occur in line1.	If the output line exceeds 255 characters it's
	 truncated.
_____________________________________________________________________*/


void utl_PlugParams(Str255 line1, Str255 line2, Str255 p0,
		    Str255 p1, Str255 p2, Str255 p3)
{
	/* MJN *//* changed type declaration for in, out, inEnd, outEnd, and param from char* to unsigned char*,
	   so that this routine will work properly with strings over 127 characters in length */
	unsigned char *in;	/* pointer to cur pos in input line */
	unsigned char *out;	/* pointer to cur pos in output line */
	unsigned char *inEnd;	/* pointer to end of input line */
	unsigned char *outEnd;	/* pointer to end of output line */
	unsigned char *param;	/* pointer to param to be plugged */
	short len;		/* length of param */

	in = line1 + 1;
	out = line2 + 1;
	inEnd = line1 + 1 + *line1;
	outEnd = line2 + 256;
	while (in < inEnd) {
		if (*in == '^') {
			in++;
			if (in >= inEnd)
				break;
			switch (*in++) {
			case '0':
				param = p0;
				break;
			case '1':
				param = p1;
				break;
			case '2':
				param = p2;
				break;
			case '3':
				param = p3;
				break;
			default:
				continue;
			};
			if (!param)
				continue;
			len = *param;
			if (out + len > outEnd)
				len = outEnd - out;
			BMD(param + 1, out, len);
			out += len;
		} else {
			if (out >= outEnd)
				break;
			*out++ = *in++;
		};
	};
	*line2 = out - (line2 + 1);
}

/*______________________________________________________________________

	 utl_RestoreWindowPos - Restore Window Position.
	
	 Entry: 	theWindow = pointer to window.
						userState = saved user state rectangle for the window.
						zoomed = true if window in zoomed state when saved.
						offset = pixel offset used in DragRect calls.
						computeStdState = pointer to function to compute standard state.
						computeDefState = pointer to function to compute default state.
	
	 Exit:		window position, size, and zoom state restored.
						userState = new user state.
						
	 See HIN 6: "When reopening a movable window, check its saved 
	 position.	If the window is in a position to which the user could
	 have dragged it, then leave it there.	If the window can be zoomed
	 and was in the zoomed state when it was last closed, put it in the
	 zoomed state again.	(Note that the current and previous zoomed states
	 are not necessarily the same, since the window may be reopened on a
	 different monitor.)	If the window is not in a position to which the
	 user could have dragged it, then it must be relocated, so use the
	 default location.	However, do not automatically use the default size
	 when using the default location; if the entire window would be visible
	 using the default location and stored size, then use the stored size."
	
	 The "offset" parameter is usually 4.  When initializing the boundary rectangle
	 for DragWindow calls, normally the boundary rectangle of the desktop gray
	 region is inset by 4 pixels.  If some value other than 4 is used, it should
	 be passed to RestoreWindowPos as the "offset" parameter.
	
	 The computeStdState function is passed a pointer to the window.	Given
	 the userState in the window zoom info, it must compute the standard
	 (zoom) state in the window zoom info.	This is an application-dependent
	 function.
	
	 The computeDefState function must determine the default position and
	 size of a new window, and return the result as a rectangle.	This may
	 involve invocation of a staggering algorithm or some other algorithm.
	 This is an application-dependent function.
	
	 Changed to keep window smaller than the desktop's bounding box.
	 (s-dorner@uiuc.edu, 12/16/91)
_____________________________________________________________________*/


void utl_RestoreWindowPos(WindowPtr theWindow, Rect * userState,
			  Boolean zoomed, short offset,
			  short titleBarHeight, short leftRimWidth,
			  utl_ComputeStdStatePtr computeStdState,
			  utl_ComputeDefStatePtr computeDefState)
{
	MyWindowPtr win;	/* pointer to one of our own window structures */
	WindowPtr winWP;
	short userHeight;	/* height of userState */
	short userWidth;	/* width of userState */
	Rect r;			/* scratch rectangle */
	RgnHandle rgn;		/* scratch region */
	Rect stdState;		/* standard state */
	short windHeight;	/* window height */
	short windWidth;	/* window width */

	win = GetWindowMyWindowPtr(theWindow);
	if (!win)
		return;

	winWP = GetMyWindowWindowPtr(win);
	SanitizeSize(win, userState);
	if (titleBarHeight > 0
	    && !utl_CouldDrag(theWindow, userState, offset, titleBarHeight,
			      leftRimWidth)) {
		userHeight = userState->bottom - userState->top;
		userWidth = userState->right - userState->left;
		(*computeDefState) (theWindow, userState);
		if (!zoomed) {
			r = *userState;
			r.bottom = r.top + userHeight;
			r.right = r.left + userWidth;
			r.top -= titleBarHeight;
			r.left -= leftRimWidth;
			InsetRect(&r, -1, -1);
			rgn = NewRgn();
			RectRgn(rgn, &r);
			DiffRgn(rgn, GetGrayRgn(), rgn);
			if (EmptyRgn(rgn)) {
				userState->bottom =
				    userState->top + userHeight;
				userState->right =
				    userState->left + userWidth;
			};
			DisposeRgn(rgn);
		};
	};
	MoveWindow(theWindow, userState->left, userState->top, false);
	windHeight = userState->bottom - userState->top;
	windWidth = userState->right - userState->left;
	if (!win->isRunt)
		SizeWindow(theWindow, windWidth, windHeight, true);
	SetWindowUserState(winWP, userState);
	(*computeStdState) (theWindow);
	if (zoomed) {
		GetWindowStandardState(winWP, &stdState);
		MoveWindow(theWindow, stdState.left, stdState.top, false);
		windHeight = stdState.bottom - stdState.top;
		windWidth = stdState.right - stdState.left;
		if (!win->isRunt)
			SizeWindow(theWindow, windWidth, windHeight, true);
	};
}

/*______________________________________________________________________

	 utl_Rom64 - Check to see if we have the old 64K ROM.
	
	 Exit:		function result = true if 64K ROM.
_____________________________________________________________________*/


Boolean utl_Rom64(void)
{
	return (False);
#ifdef NEVER
	return *(short *) ROM85 < 0;
#endif
}

/*______________________________________________________________________

	 utl_RFSanity - Check a Resource File's Sanity.
	
	 Entry: 	*fName = file name.
	
	 Exit:		*sane = true if resource fork is sane.
						function result = error code.
						
	 This routine checks the sanity of a resource file.  The Resource Manager
	 does not do error checking, and can bomb or hang if you use it to open
	 a damaged resource file.  This routine can be called first to precheck
	 the file.
	
	 The routine checks the entire resource map.
	
	 It is the caller's responsibility to set the proper default volume and
	 directory.
_____________________________________________________________________*/


OSErr utl_RFSanity(FSSpecPtr spec, Boolean * sane)
{
	short refNum;		/* file refnum */
	long count;		/* number of bytes to read */
	long logEOF;		/* logical EOF */
	Ptr map;		/* pointer to resource map */
	unsigned long dataLWA;	/* offset in file of data end */
	unsigned long mapLWA;	/* offset in file of map end */
	unsigned short typeFWA;	/* offset from map begin to type list */
	unsigned short nameFWA;	/* offset from map begin to name list */
	unsigned char *pType;	/* pointer into type list */
	unsigned char *psName;	/* pointer to start of name list */
	unsigned char *pMapEnd;	/* pointer to end of map */
	short nType;		/* number of resource types in map */
	unsigned char *pTypeEnd;	/* pointer to end of type list */
	short nRes;		/* number of resources of given type */
	unsigned short refFWA;	/* offset from type list to ref list */
	unsigned char *pRef;	/* pointer into reference list */
	unsigned char *pRefEnd;	/* pointer to end of reference list */
	unsigned short resNameFWA;	/* offset from name list to resource name */
	unsigned char *pResName;	/* pointer to resource name */
	unsigned long resDataFWA;	/* offset from data begin to resource data */
	Boolean mapOK;		/* true if map is sane */
	OSErr rCode;		/* error code */

	struct {
		unsigned long dataFWA;	/* offset in file of data */
		unsigned long mapFWA;	/* offset in file of map */
		unsigned long dataLen;	/* data area length */
		unsigned long mapLen;	/* map area length */
	} header;

	/* Open the resource file. */
	if (rCode = FSpOpenRF(spec, fsRdPerm, &refNum)) {
		if (rCode == fnfErr || rCode == permErr) {
			*sane = true;
			return noErr;
		} else {
			return rCode;
		};
	};

	/* Get the logical eof of the file. */

	if (rCode = GetEOF(refNum, &logEOF))
		return rCode;
	if (!logEOF) {
		*sane = true;
		if (rCode = MyFSClose(refNum))
			return rCode;
		return noErr;
	};

	/* Read and validate the resource header. */

	count = 16;
	if (rCode = ARead(refNum, &count, (Ptr) & header)) {
		MyFSClose(refNum);
		return rCode;
	};
	dataLWA = header.dataFWA + header.dataLen;
	mapLWA = header.mapFWA + header.mapLen;
	mapOK = count == 16;
	mapOK = mapOK && header.mapLen > 28;
	mapOK = mapOK && header.dataFWA < 0x01000000;
	mapOK = mapOK && header.mapFWA < 0x01000000;
	mapOK = mapOK && dataLWA <= logEOF;
	mapOK = mapOK && mapLWA <= logEOF;
	mapOK = mapOK && (dataLWA <= header.mapFWA
			  || mapLWA <= header.dataFWA);

	/* Read the resource map. */

	map = nil;
	if (mapOK) {
		map = NuPtr(header.mapLen);
		if (!map)
			rCode = MemError();
		else if (!
			 (rCode =
			  SetFPos(refNum, fsFromStart, header.mapFWA))) {
			count = header.mapLen;
			rCode = ARead(refNum, &count, map);
		};
	};

	/* Verify the type list and name list offsets. */

	if (mapOK && !rCode) {
		typeFWA = *(unsigned short *) (map + 24);
		nameFWA = *(unsigned short *) (map + 26);
		mapOK = typeFWA == 28 && nameFWA >= typeFWA
		    && nameFWA <= header.mapLen && !(typeFWA & 1)
		    && !(nameFWA & 1);
	};

	/* Verify the type list, reference lists, and name list. */

	if (mapOK && !rCode) {
		pType = map + typeFWA;
		psName = map + nameFWA;
		pMapEnd = map + header.mapLen;
		nType = *(short *) pType + 1;
		pType += 2;
		pTypeEnd = pType + (nType << 3);
		if (mapOK = pTypeEnd <= pMapEnd) {
			while (pType < pTypeEnd) {
				nRes = *(short *) (pType + 4) + 1;
				refFWA = *(unsigned short *) (pType + 6);
				pRef = map + typeFWA + refFWA;
				pRefEnd = pRef + 12 * nRes;
				if (!
				    (mapOK = pRef >= pTypeEnd
				     && pRef < psName && !(refFWA & 1)))
					break;
				while (pRef < pRefEnd) {
					resNameFWA =
					    *(unsigned short *) (pRef + 2);
					if (resNameFWA != 0xFFFF) {
						pResName =
						    psName + resNameFWA;
						if (!
						    (mapOK =
						     pResName + *pResName <
						     pMapEnd))
							break;
					};
					resDataFWA =
					    *(unsigned long *) (pRef +
								4) &
					    0x00FFFFFF;
					if (!
					    (mapOK =
					     header.dataFWA + resDataFWA <
					     dataLWA))
						break;
					pRef += 12;
				};
				if (!mapOK)
					break;
				pType += 8;
			};
		};
	};

	/* Dispose of the resource map, close the file and return. */

	if (map)
		ZapPtr(map);
	if (!rCode) {
		rCode = MyFSClose(refNum);
	} else {
		(void) MyFSClose(refNum);
	};
	*sane = mapOK;
	return rCode;
}

/*______________________________________________________________________

	 utl_SaveWindowPos - Save Window Position.
	
	 Entry: 	theWindow = window pointer.
	
	 Exit:		userState = user state rectangle of the window.
						zoomed = true if window zoomed.
						
	 See HIN 6: "Before closing a movable window, check to see if its
	 location or size have changed.  If so, save the new location and
	 size.	If the window can be zoomed, save the user state and also 
	 save whether or not the window is in the zoomed (standard) sate."
	
	 We assume in this routine that the caller has already kept track
	 of the fact that this window's location or size has changed, and that
	 we do indeed need to save the location and size.
	
	 This routine only works if the window's origin has not been offset.
_____________________________________________________________________*/


void utl_SaveWindowPos(WindowPtr theWindow, Rect * userState,
		       Boolean * zoomed)
{
	GrafPtr savedPort;	/* saved grafport */
	Rect curState;		/* current window rect, global coords */
	Rect stdState;		/* standard (zoom) rect, global coords */
	Point p;		/* scratch point */
	Rect r;			/* scratch rect */

	GetPort(&savedPort);
	SetPort_(GetWindowPort(theWindow));
	SetPt(&p, 0, 0);
	LocalToGlobal(&p);
	GetPortBounds(GetWindowPort(theWindow), &curState);
	OffsetRect(&curState, p.h, p.v);
	if (IsKnownWindowMyWindow(theWindow)) {
		MyWindowPtr win = GetWindowMyWindowPtr(theWindow);
		if (!RectHi(curState) || RectHi(curState) < win->minSize.v)
			curState.bottom =
			    curState.top + MAX(win->minSize.v,
					       RectHi(win->contR));
	}

	/* Determine if window is zoomed.  The criteria is that both the
	   top left and bottom right corners of the current window rectangle
	   and the standard (zoom) rectangle must be within 7 pixels of each
	   other.  This is the same algorithm as the one used by the standard
	   system window definition function. */
	*zoomed = false;
	GetWindowStandardState(theWindow, &stdState);
	SetPt(&p, curState.left, curState.top);
	SetRect(&r, stdState.left, stdState.top, stdState.left,
		stdState.top);
	InsetRect(&r, -7, -7);
	if (PtInRect(p, &r)) {
		SetPt(&p, curState.right, curState.bottom);
		SetRect(&r, stdState.right, stdState.bottom,
			stdState.right, stdState.bottom);
		InsetRect(&r, -7, -7);
		*zoomed = PtInRect(p, &r);
	};
	if (*zoomed) {
		GetWindowUserState(theWindow, userState);
	} else {
		*userState = curState;
	};
	SetPort_(savedPort);
}

/*______________________________________________________________________

	 utl_ScaleFontSize - Scale Font Size
	
	 Entry: 	fontNum = font number.
						fontSize = nominal font size.
						percent = percent change in size.
						laser = true if laserwriter.
	
	 Exit:		function result = scaled font size.
						
	 The nominal font size is multiplied by the percentage,
	 then truncated.	For non-laserwriters it is then rounded down
	 to the nearest font size which is available in that true size,
	 without font manager scaling, or which can be generated by doubling
	 an existing font size.
_____________________________________________________________________*/


short utl_ScaleFontSize(short fontNum, short fontSize, short percent,
			Boolean laser)
{
	short nSize;		/* new size */
	short x;		/* new test size */

	nSize = fontSize * percent / 100;
	if (!laser) {
		x = nSize;
		while (x > 0) {
			if (RealFont(fontNum, x))
				break;
			if (!(x & 1) && RealFont(fontNum, x >> 1))
				break;
			x--;
		};
		if (x)
			nSize = x;
	};
	return nSize;
}

/*______________________________________________________________________

	 utl_SpinCursor - Animate cursor.
	
	 After calling InitSpinCursor to initialize an animated cursor, call
	 SpinCursor periodically to make the cursor animate.
_____________________________________________________________________*/


void utl_SpinCursor(void)
{
	CursHandle h;		/* handle to cursor */
	long ticksNow;		/* current tick count */

	ticksNow = TickCount();
	if (ticksNow < LastTick + TickInterval)
		return;
	LastTick = ticksNow;
	CurCurs++;
	if (CurCurs >= NumCurs)
		CurCurs = 0;
	h = CursArray[CurCurs];
	SetCursor(*h);
}

/*______________________________________________________________________

	 utl_StaggerWindow - Stagger a New Window
	
	 Entry: 	windRect = window portrect.
						initialOffset = intitial pixel offset of window from corner.
						offset = offset for subsequent staggered windows.
	
	 Exit:		pos = window position.
	
	 According to HIN 6, a new window should be positioned as follows:
	 "The first document window should be positioned in the upper-left corner
	 of the gray area of the main screen (the screen with the menu bar).
	 Each additional independent window should be staggered from the upper-left
	 corner of the screen that contains the largest portion of the
	 frontmost window."  Also, "When a window is used or closed, its original
	 position becomes available again.	The next window opened should use this
	 position.	Similarly, if a window is moved onto a previously available
	 position, that position becomes unavailable again."
	
	 This routine implements these rules.  A position is considered to be
	 "unavailable" if some other window is within (offset+1)/2 pixels of the
	 position in both the vertical and horizontal directions.
	
	 If all slots are occupied, the routine attempts to locate the first one
	 which is occupied by only one other window.	If this attempt fails, it
	 tries to locate one which is occupied by only two other windows, etc.
	 Thus, if the screen is filled with staggered windows, subsequent windows
	 will be staggered on top of the existing ones, forming a second "layer."
	 If this layer fills up, a third layer is started, etc.
	
	 Steve Dorner, UIUC Computing Services Office 9/27/90
	 Invisible windows are no longer considered by the function.
	 Horizontal offset is 2 pixels on small screens
	 Steve Dorner, UIUC Computing Services Office 3/92
	 Leaving 2 pixel margin at screen bottom to allow for autoscroll
	 Added optional insets
	 Steve Dorner, UIUC Computing Services Office 4/92
	 Added parameter for specifying a particular device
_____________________________________________________________________*/

#define SIZE_9	540 		/* a 9" (or so) screen */               /* sd */
#define OFF_9 	3 			/* horizontal offset for 9" screen */   /* sd */
void utl_StaggerWindow(WindowPtr theWindow, Rect * windRect,
		       short initialOffset, short offset,
		       short titleBarHeight, short leftRimWidth,
		       Point * pos, short specificDevice)
{
	MyWindowPtr win;	/* pointer to one of our own window structures */
	GrafPtr savedPort;	/* saved grafport */
	short offsetDiv2;	/* offset/2 */
	short windHeight;	/* window height */
	short windWidth;	/* window width */
	WindowPtr frontWind;	/* pointer to front window */
	GDHandle gd;		/* GDevice */
	Rect screenRect;	/* screen rectangle */
	Rect junkRect;		/* window rectangle */
	Boolean hasMB;		/* true if screen has menu bar */
	Point initPos;		/* initial staggered window position */
	Point curPos;		/* current staggered window position */
	WindowPtr curWind;	/* pointer to current window */
	MyWindowPtr curWindWin;	/* the MyWindow version of the current window */
	Point windPos;		/* current window position */
	short deltaH;		/* horizontal distance */
	short deltaV;		/* vertical distance */
	short layer;		/* layer number */
	short nOccupied;	/* number windows occupying cur pos */
	Boolean noPos;		/* true if no position is on screen */
	Boolean is9;		/* is this a 9" screen? *//* sd */

	win = GetWindowMyWindowPtr(theWindow);
	GetPort(&savedPort);
	offsetDiv2 = (offset + 1) >> 1;
	windHeight = windRect->bottom - windRect->top;
	windWidth = windRect->right - windRect->left;
	frontWind =
	    UglyHackFrontWindow ? UglyHackFrontWindow :
	    FindTopUserWindow();
	gd = nil;
	if (specificDevice) {
		utl_GetIndGD(specificDevice, &gd, &screenRect, &hasMB);
	} else if (frontWind) {
		utl_GetWindGD(frontWind, &gd, &screenRect, &junkRect,
			      &hasMB);
	}
	if (!gd) {
		gd = GetMainDevice();
		hasMB = true;
		screenRect = (*gd)->gdRect;
	};

	if (!IsMyWindow(theWindow)
	    || (win && win->windowType != kDockable)) {
		GetDisplayRect(gd, &screenRect);	// Normal window; get normal rectangle
		InsetRect(&screenRect, 3, 3);
	} else if (hasMB)
		screenRect.top += utl_GetMBarHeight();	// docked - just avoid menu bar
	SetPt(&initPos, screenRect.left + initialOffset + leftRimWidth,
	      screenRect.top + initialOffset + titleBarHeight);
	is9 = screenRect.right - screenRect.left <= SIZE_9;	/* sd */
	layer = 1;

	while (true) {
		/* Test each layer number "layer", starting with 1 and incrementing by 1.
		   Break out of the loop when we find a position curPos which
		   is "occupied" by fewer than "layer" other windows. */
		curPos = initPos;
		noPos = true;
		while (true) {
			/* Test each possible position curPos.  Break out of the loop
			   when we have exhaused the possible positions, or when we
			   have located one which has < layer "occupants". */
			curWind = frontWind;
#ifndef NEWWAY
			if (curPos.v + windHeight >= screenRect.bottom ||
			    curPos.h + windWidth >= screenRect.right) {
				break;
			};
#else
			if (curPos.h + windWidth >= screenRect.right)
				break;
			if (curPos.v + windHeight >= screenRect.bottom)
				curPos.v = initPos.v;
#endif
			noPos = false;
			nOccupied = 0;
			while (curWind) {
				Boolean hideme;

				/* Scan the window list and count up how many of them "occupy"
				   the current location curPos.  Break out of the loop when
				   we reach the end of the list, or when the count is >=
				   layer. */
				curWindWin = GetWindowMyWindowPtr(curWind);
				hideme =
				    curWindWin ? curWindWin->
				    hideme : false;
				if (IsWindowVisible(curWind) && !(IsKnownWindowMyWindow(curWind) && hideme)) {	/* sd */
					SetPt(&windPos, 0, 0);
					SetPort_(GetWindowPort(curWind));
					LocalToGlobal(&windPos);
					deltaH = curPos.h - windPos.h;
					deltaV = curPos.v - windPos.v;
					if (deltaH < 0)
						deltaH = -deltaH;
					if (deltaV < 0)
						deltaV = -deltaV;
					if (deltaH <= offsetDiv2
					    && deltaV <= offsetDiv2) {
						nOccupied++;
						if (nOccupied >= layer)
							break;
					};
				};	/* sd */
				curWind = GetNextWindow(curWind);
			};
			if (!curWind)
				break;
			if (initialOffset)
				curPos.h += is9 ? OFF_9 : offset;	// docked windows we don't nudge to the right.
			curPos.v += offset;
		};
		if (!curWind || noPos)
			break;
		layer++;
	};
	SetPort_(savedPort);

	//   Make sure we're OK with docked windows
	SetRect(&junkRect, curPos.h, curPos.v, curPos.h + windWidth,
		curPos.v + windHeight);
	DockedWinReduce(theWindow, &junkRect, &screenRect);
	SetPt(&initPos, screenRect.left + initialOffset + leftRimWidth,
	      screenRect.top + initialOffset + titleBarHeight);
	if (initPos.h > curPos.h)
		curPos.h = initPos.h;
	if (initPos.v > curPos.v)
		curPos.v = initPos.v;

	*pos = curPos;
}

#if !(TARGET_RT_MAC_CFM)

/*______________________________________________________________________

	 CancelFilter - Command Period Dialog Filter Proc.
	
	 Entry: 	theDialog = pointer to dialog record.
						theEvent = pointer to event record.
	
	 Exit:		if command-period or escape typed:
	
						function result = true.
						itemHit = item number of cancel button.
						
						else if user specified a filterProc on the call to
						utl_StopAlert:
						
						user's filterProc called, function result and itemHit
						returned by user's filterProc.
						
						else:
						
						function result = false.
						itemHit = undefined.
_____________________________________________________________________*/


static pascal Boolean CancelFilter(DialogPtr theDialog,
				   EventRecord * theEvent, short *itemHit)
{
	short key;		/* ascii code of key pressed */

	if (theEvent->what != keyDown && theEvent->what != autoKey) {
		if (Filter) {
			return (*Filter) (theDialog, theEvent, itemHit);
		} else {
			return false;
		};
	} else {
		key = theEvent->message & charCodeMask;
		if (key == escapeKey) {
			*itemHit = CancelItem;
			utl_FlashButton(theDialog, CancelItem);
			return true;
		} else if ((theEvent->modifiers & cmdKey) && (key == '.')) {
			*itemHit = CancelItem;
			utl_FlashButton(theDialog, CancelItem);
			return true;
		} else if (Filter) {
			return (*Filter) (theDialog, theEvent, itemHit);
		} else if (key == returnKey || key == enterKey) {
			*itemHit = 1;
			utl_FlashButton(theDialog, 1);
			return true;
		} else {
			return false;
		};
	};
}

/*______________________________________________________________________

	 utl_StopAlert - Present Stop Alert
	
	 Entry: 	alertID = resource id of alert.
						filterProc = pointer to filter proc.
						cancelItem = item number of cancel button, or 0 if
							 none.
	
	 Exit:		function result = item number
						
	 This routine is identical to the Dialog Manager routine StopAlert,
	 except that it centers the alert on the main window, and if requested,
	 command/period or the Escape key is treated the same as a click on the 
	 Cancel button.
_____________________________________________________________________*/


short utl_StopAlert(short alertID, ModalFilterProcPtr filterProc,
		    short cancelItem)
{
	Handle h;		/* handle to alert resource */
	short result;		/* function result */

	h = GetResource_('ALRT', alertID);
	HLock(h);
	utl_CenterDlogRect(*(Rect **) h, false);
	if (cancelItem) {
		Filter = filterProc;
		CancelItem = cancelItem;
		result = StopAlert(alertID, CancelFilter);
	} else {
		result = StopAlert(alertID, filterProc);
	};
	HUnlock(h);
	return result;
}
#endif

/*______________________________________________________________________

	 utl_VolIsMFS - Test for MFS volume.
	
	 Entry: 	vRefNum = volume reference number.
	
	 Exit:		function result = true if volume is MFS.
_____________________________________________________________________*/


Boolean utl_VolIsMFS(short vRefNum)
{
	HParamBlockRec vBlock;	/* vol info param block */

	vBlock.volumeParam.ioNamePtr = nil;
	vBlock.volumeParam.ioVolIndex = 0;
	vBlock.volumeParam.ioVRefNum = vRefNum;
	vBlock.volumeParam.ioVSigWord = 0xd2d7;	/* in case we don't have HFS */
	(void) PBHGetVInfo(&vBlock, false);
	return vBlock.volumeParam.ioVSigWord == 0xd2d7;
}

/*----------------------------------------------------------------------------
	GetDropLocationDirectory
	
	Given an 'alis' drop location AEDesc, return the volume reference
	number and directory ID of the directory.
	
	Entry:	dropLocation = pointer to 'alis' AEDesc record.
	
	Exit:	function result = error code.
			*volumeID = volume reference number.
			*directoryID = directory ID.
			
	From Apple "HFS Drag Sample" sample code.
----------------------------------------------------------------------------*/

OSErr GetDropLocationDirectory(AEDesc * dropLocation, short *volumeID,
			       long *directoryID)
{
	OSErr err = noErr;
	AEDesc targetDescriptor;
	FSSpec targetLocation;
	CInfoPBRec getTargetInfo;

	err = AECoerceDesc(dropLocation, typeFSS, &targetDescriptor);
	if (err != noErr)
		return err;

	AEGetDescData(&targetDescriptor, &targetLocation, sizeof(FSSpec));

	err = AEDisposeDesc(&targetDescriptor);
	if (err != noErr)
		return err;

	IsAlias(&targetLocation, &targetLocation);

	getTargetInfo.dirInfo.ioNamePtr = targetLocation.name;
	getTargetInfo.dirInfo.ioVRefNum = targetLocation.vRefNum;
	getTargetInfo.dirInfo.ioFDirIndex = 0;
	getTargetInfo.dirInfo.ioDrDirID = targetLocation.parID;

	err = PBGetCatInfoSync(&getTargetInfo);
	if (err != noErr)
		return err;

	*directoryID = getTargetInfo.dirInfo.ioDrDirID;
	*volumeID = targetLocation.vRefNum;
	return noErr;
}



/*----------------------------------------------------------------------------
	DragTargetWasTrash
	
	Check to see if the target of a drag and drop was the Finder trashcan.
	
	Entry:	dragRef = drag reference.
	
	Exit:	function result = true if target was trash.
----------------------------------------------------------------------------*/

Boolean DragTargetWasTrash(DragReference dragRef)
{
	AEDesc dropLocation;
	OSErr err = noErr;
	long dropDirID, trashDirID;
	short dropVRefNum, trashVRefNum;

	err = GetDropLocation(dragRef, &dropLocation);
	if (err != noErr)
		return false;

	if (dropLocation.descriptorType == typeNull)
		goto exit;

	err =
	    GetDropLocationDirectory(&dropLocation, &dropVRefNum,
				     &dropDirID);
	if (err != noErr)
		goto exit;

	err =
	    FindFolder(dropVRefNum, kTrashFolderType, false, &trashVRefNum,
		       &trashDirID);
	if (err != noErr)
		goto exit;

	if (dropVRefNum != trashVRefNum || dropDirID != trashDirID)
		goto exit;

	AEDisposeDesc(&dropLocation);
	return true;

      exit:

	AEDisposeDesc(&dropLocation);
	return false;
}
