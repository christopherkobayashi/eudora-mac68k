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

#include "stickypopup.h"

#define FILE_NUM 94
#pragma segment StickyPopup


/* MJN *//* new file */
/*

		stickypopup.c


		stickypopup.c contains code to do a "sticky" popup menu.  It is basically identical to
		a normal popup menu, except that it doesn't require the mouse button to stay down in
		order to stay open, and it allows keyboard navigation to select items.

		Use NewStickyPopup to create a new, empty sticky popup.  Use AddEntriesToStickyPopup to
		add items to the popup, and RemoveEntriesFromStickyPopup to remove items.  Call
		StickyPopupSelect to bring up the sticky popup and allow the user to select a choice
		from it; this function is comperable to PopUpMenuSelect.  Use DisposeStickyPopup to
		dispose of the sticky popup when you are through with it.

		WARNING: The sticky popup menu routines are not compatible with the Menu Manager routines
		for working with menus and standard popup menus.  The data types (i.e. StickyPopupHdl
		and MenuHandle) are not interchangeable.

*/



#pragma mark Constants & defines

#define STICKY_POPUP_MEM_TEST_SIZE 16384 /* 16 K */
#define MAX_LIST_DATA_SIZE 32000 /* safely less than 32 K */
#define CELL_WIDTH_EXTRA 4 /* number of extra pixels to leave on right hand side in list */
#define STICKY_SELECT_EVENT_MASK (mDownMask + mUpMask + keyDownMask + autoKeyMask + keyUpMask)

#define RETURN_CHAR_CODE 0x0D /* ASCII code for Carriage Return */
#define ENTER_CHAR_CODE 0x03 /* ASCII code for Enter */
#define PERIOD_CHAR_CODE 0x2E /* ASCII code for Period */
#define ESCAPE_KEY_CODE 0x35 /* virtual key code for Escape */
#define BACKSPACE_CHAR_CODE 0x08 /* ASCII code for Backspace/Delete */
#define CLEAR_KEY_CODE 0x47 /* virtual key code for Clear key on numeric keypad */
#define FWD_DEL_CHAR_CODE 0x7F /* ASCII code for Forward Delete key on extended keyboard */
#define UP_ARROW_CHAR_CODE 0x1E /* ASCII code for Up Arrow */
#define DOWN_ARROW_CHAR_CODE 0x1F /* ASCII code for Down Arrow */
#define LEFT_ARROW_CHAR_CODE 0x1C /* ASCII code for Left Arrow */
#define RIGHT_ARROW_CHAR_CODE 0x1D /* ASCII code for Right Arrow */
#define HOME_CHAR_CODE 0x01 /* ASCII code for Home key on extended keyboard */
#define END_CHAR_CODE 0x04 /* ASCII code for End key on extended keyboard */
#define SPACE_CHAR_CODE 0x20 /* ASCII code for space character */
#define DASH_CHAR_CODE 0x2D /* ASCII code for dash character */
#define STICKY_SPACE_CHAR_CODE 0xCA /* ASCII code for sticky (non-breaking) space (em-space) character */
#define STICKY_DASH_CHAR_CODE 0xD0 /* ASCII code for sticky (non-breaking) dash (en-dash) character */



#pragma mark Globals

static StickyPopupHdl CurStickyPopup = nil; /* sticky popup currently being "run" by StickyPopupSelect */
static long StickyPopupKeyUpdateDelay; /* time, in ticks, to wait after a key stroke before updating the list selection in response to typing */
static Str255 StickyPopupTypingStr; /* string for accumulating the text that the user is typing for purpose of selection in list */
static long StickyPopupLastKeyTicks; /* system ticks (TickCount) of last key stroke being used for purpose of selection in list */
static pascal Boolean StickyPopupClickLoop(void);


/* FINISH *//* move to stringutil.c (and its prototype, too) */
/*********************************************************************************************
 *	StripStickyCharacters - strips out sticky spaces and sticky dashes from the Pascal
 *	string passed in theString (sticky meaning non-breaking).
 *
 *	Note that the usage of the word "sticky" in this routine's name has nothing to do with
 *	sticky popups.
 *
 *	This routine will not move or purge memory.
 *********************************************************************************************/

void StripStickyCharacters(Str255 theString)

{
	long							numChars;
	long							i;
	unsigned char*		curChar;


	numChars = theString[0];
	for (i = 0, curChar = theString + 1; i < numChars; i++, curChar++)
		switch (*curChar)
		{
			case STICKY_SPACE_CHAR_CODE:
				*curChar = SPACE_CHAR_CODE;
				break;
			case STICKY_DASH_CHAR_CODE:
				*curChar = DASH_CHAR_CODE;
				break;
		}
}


/*********************************************************************************************
 *	StickyPopupNormalizeString - normalize a string for purposes of string comparison.  This
 *	routine is provided so that you can set up a sticky popup menu in alpha-numeric-sorted
 *	order, so that typing to select an item will work; since the sticky popup routines use
 *	this routine also, you are guaranteed to be sorting using the same methods as the sticky
 *	popup code.
 *
 *	Currently, normalization does the following:
 *
 *		-	Converts all lowercase characters to uppercase
 *		-	Strips out all diacritical markings
 *		-	Converts all sticky spaces to normal spaces, and converts all sticky dashes to
 *			normal dashes (via StripStickyCharacters)
 *********************************************************************************************/

void StickyPopupNormalizeString(Str255 theString)

{
	/* FINISH *//* support scripts other than smRoman */
	if (FurrinSort) UppercaseStripDiacritics(theString + 1, theString[0], smRoman);
	else MyUpperText(theString+1,theString[0]);
	StripStickyCharacters(theString);
}


/*********************************************************************************************
 *	StickyPopupCompareString - string comparison routine.  Return value depends on the
 *	comparison of the two strings:
 *
 *			Relationship				Return value
 *
 *			stringA < stringB		-1
 *			stringA = stringB		 0
 *			stringA > stringB		 1
 *
 *	The return value is identical to that returned by both RelString and CompareString.
 *	This routine is provided so that you can set up a sticky popup menu in
 *	alpha-numeric-sorted order, so that typing to select an item will work; since the sticky
 *	popup routines use this routine also, you are guaranteed to be sorting using the same
 *	methods as the sticky popup code.
 *
 *	If you pass in true for needNormalize, this routine will make local copies of the two
 *	strings, and call StickyPopupNormalizeString on the local copies before doing the string
 *	compare; the original strings you pass in are left unmodified.  If you pass in false for
 *	needNormalize, it is assumed that you have already called StickyPopupNormalizeString on
 *	the strings yourself, or that you have some reason to do the comparison without first
 *	normalizing the strings.
 *********************************************************************************************/

short StickyPopupCompareString(Str255 stringA, Str255 stringB, Boolean needNormalize)

{
	Str255		stringA_;
	Str255		stringB_;


	if (!needNormalize)
		return StringComp(stringA,stringB);	// calls CompareString or RelString depending on FurrinSort SD
		
	BlockMoveData(stringA, stringA_, stringA[0] + 1);
	BlockMoveData(stringB, stringB_, stringB[0] + 1);
	StickyPopupNormalizeString(stringA_);
	StickyPopupNormalizeString(stringB_);
	return StringComp(stringA_, stringB_);	// calls CompareString or RelString depending on FurrinSort SD
}


/*********************************************************************************************
 *	MouseLocInList - returns true if mouse location passed in curMouseLoc is inside
 *	the boundaries of the sticky popup menu specified by stickyPopup; the popup
 *	should be currently popped (isPopped = true).  curMouseLoc is in global coordinates.
 *********************************************************************************************/

static Boolean MouseLocInList(Point curMouseLoc, StickyPopupHdl stickyPopup)

{
	return PtInRgn(curMouseLoc,MyGetWindowContentRegion((**stickyPopup).theWindow));
}


/*********************************************************************************************
 *	StickyPopupClickLoop - lClikLoop routine for the list used in sticky popups.
 *	It always returns true (i.e. it never canceles LClick).  It's only purpose
 *	is to unhilite cells while the mouse is outside of the list's boundaries.
 *********************************************************************************************/

static pascal Boolean StickyPopupClickLoop(void)

{
	ListHandle	theList;
	Point				curMouseLoc;
	Cell				curSel;
	Boolean			selectionExists;


	theList = (**CurStickyPopup).theList;
	GetMouse(&curMouseLoc);
	LocalToGlobal(&curMouseLoc);
	if (MouseLocInList(curMouseLoc, CurStickyPopup))
	{
		curSel.v = curSel.h = 0;
		selectionExists = LGetSelect(true, &curSel, theList);
		if (!selectionExists)
		{
			curSel.v = (**theList).mouseLoc.v / (**theList).cellSize.v; /* WARNING *//* assumes cellSize.v has already been set to a valid non-zero value */
			curSel.h = 0;
			LSetDrawingMode(true, theList);
			LSetSelect(true, curSel, theList);
		}
		return true;
	}
	curSel.v = curSel.h = 0;
	selectionExists = LGetSelect(true, &curSel, theList);
	if (selectionExists)
	{
		LSetSelect(false, curSel, theList);
		LSetDrawingMode(false, theList);
	}
	return true;
}

/*********************************************************************************************
 *	NewStickyPopup - create a new sticky popup menu.  Pass in the font family ID
 *	and font size to use in fontNum and fontSize.  Pass a pointer to a StickyPopupHdl
 *	in stickyPopup; the created sticky popup will be stored in this handle.  The
 *	function returns noErr if it succeeds, or a non-zero error code if it fails.
 *********************************************************************************************/

OSErr NewStickyPopup(short fontNum, short fontSize, StickyPopupHdl *stickyPopup)

{
	OSErr						err;
	StickyPopupHdl	popupHdl;
	StickyPopupPtr	popupPtr;
	GrafPtr					origPort;
	Ptr							p;
	WindowPtr				win;
	Rect						r;
	ListBounds			listDataBounds;
	Point						listCellSize;
	ListHandle			listHdl;
	Str255					scratchStr;
	DECLARE_UPP(StickyPopupClickLoop,ListClickLoop);

	*stickyPopup = nil;

	GetFontName(fontNum, &scratchStr);
	if (!scratchStr[0])
		return paramErr;
	if (fontSize < 4)
		return paramErr;

	p = NewPtr(STICKY_POPUP_MEM_TEST_SIZE);
	if (!p)
		return memFullErr;
	DisposePtr(p);

	GetPort(&origPort);

	popupHdl = (StickyPopupHdl)NewHandleClear(sizeof(StickyPopupRec));
	if (!popupHdl)
		return memFullErr;

	r.top = r.left = 0;
	r.bottom = r.right = 1;
	win = NewWindow(nil, &r, "\p", false, HaveOSX () ? dBoxProc : altDBoxProc, (WindowPtr)-1, false, 0);
	if (!win)
	{
		err = memFullErr;
		goto BadExit1;
	}
	SetWindowKind (win, dialogKind); /* extra precaution to prevent full-context switch-out */

	SetPort(GetWindowPort(win));
	TextFont(fontNum);
	TextSize(fontSize);
	
	listDataBounds.top = listDataBounds.left = listDataBounds.bottom = 0;
	listDataBounds.right = 1;
	listCellSize.v = listCellSize.h = 0;
	listHdl = LNew(&r, &listDataBounds, listCellSize, 0, win, false, false, false, false);
	SetPort(origPort);
	if (!listHdl)
	{
		err = MemError();
		if (!err)
			err = memFullErr;
		goto BadExit2;
	}
	(**listHdl).selFlags = lOnlyOne;
	INIT_UPP(StickyPopupClickLoop,ListClickLoop);
	(**listHdl).lClickLoop = StickyPopupClickLoopUPP;

	popupPtr = *popupHdl;
	popupPtr->theWindow = win;
	popupPtr->fontNum = fontNum;
	popupPtr->fontSize = fontSize;
	popupPtr->theList = listHdl;
	popupPtr->maxVisItems = 0;
	popupPtr->isPopped = false;
	popupPtr->refCon = 0;

	*stickyPopup = popupHdl;
	return noErr;


BadExit2:
	DisposeWindow_(win);
BadExit1:
	DisposeHandle((Handle)popupHdl);
BadExit0:
	return err;
}


/*********************************************************************************************
 *	DisposeStickyPopup - disposes of the sticky popup menu passed in stickyPopup.
 *********************************************************************************************/

void DisposeStickyPopup(StickyPopupHdl stickyPopup)

{
	StickyPopupPtr	popupPtr;


	if (!stickyPopup)
		return;

	HLock((Handle)stickyPopup);
	popupPtr = *stickyPopup;

	if (popupPtr->theList)
		LDispose(popupPtr->theList);
	if (popupPtr->theWindow)
		DisposeWindow_(popupPtr->theWindow);
	DisposeHandle((Handle)stickyPopup);
}


/*********************************************************************************************
 *	AddEntriesToStickyPopup - add entries to the sticky popup menu passed in stickyPopup.
 *	The routine can be used for adding multiple entries.  dataPtr points to the start of
 *	the data.  entryOffset is the offset, in bytes, between entries.  entryCount is the
 *	number of entries to add to the popup.  beforeIndex is the index (1-based) of the entry
 *	to insert the new entries in front of; pass -1 for beforeIndex to append to the end.
 *
 *	The data being added is always a Pascal-style string.  If you just wanted to add one
 *	entry, dataPtr would point to the start of the Pascal string (length byte), entryOffset
 *	would be ignored (set it to zero), and entryCount would be 1.  If you had an array of
 *	16 Str255's to add, you would set dataPtr to the start of the first Pascal string in
 *	the array, entryOffset would be 256, and entryCount would be 16.
 *
 *	This function doesn't attempt to do any kind of sorting of the items you're adding.
 *	Any special sorting, such as alphabetizing, must be handled by the caller.  The items
 *	in the sticky popup menu must be alphabetized in order for selection by typing to
 *	work (i.e. typing G to select the first item in the popup that starts with a G).
 *
 *	Unlike AddMenu, this function ignores menu metacharacters.
 *
 *	You cannot add entries to a sticky popup menu while the menu is popped (isPopped = true).
 *
 *	The function returns noErr if it succeeds, or a non-zero error code if it fails.
 *********************************************************************************************/

OSErr AddEntriesToStickyPopup(StickyPopupHdl stickyPopup, StringPtr dataPtr, long entryOffset, short entryCount, short beforeIndex)

{
	OSErr				err;
	ListHandle	theList;
	ListBounds	listDataBounds;
	Handle			listCellDataHdl;
	Cell				curCell;
	short				endIndex;
	StringPtr		p;
	short				origCount, newCount;
	Size				origSize, newSize;
	short				scratch;


	if (!stickyPopup)
		return paramErr;
	if ((**stickyPopup).isPopped)
		return paramErr;
	if (!entryCount || ((entryCount > 1) && !entryOffset) || !dataPtr)
		return paramErr;

	p = NewPtr(STICKY_POPUP_MEM_TEST_SIZE);
	if (!p)
		return memFullErr;
	DisposePtr(p);

	theList = (**stickyPopup).theList;
	listCellDataHdl = (Handle)(**theList).cells;
	listDataBounds = (**theList).dataBounds;
	if ((beforeIndex < 1) || (beforeIndex > listDataBounds.bottom))
		beforeIndex = listDataBounds.bottom;
	else
		beforeIndex -= 1;

	origCount = listDataBounds.bottom;
	scratch = LAddRow(entryCount, beforeIndex, theList);
	listDataBounds = (**theList).dataBounds;
	newCount = listDataBounds.bottom;
	if (newCount != (origCount + entryCount))
		return memFullErr;

	curCell.h = 0;
	endIndex = beforeIndex + entryCount;
	err = noErr;
	for (p = dataPtr, curCell.v = beforeIndex; !err && (curCell.v < endIndex); p += entryOffset, curCell.v++)
	{
		origSize = GetHandleSize(listCellDataHdl);
		if ((origSize + p[0] + 1) > MAX_LIST_DATA_SIZE)
		{
			err = memFullErr;
			continue;
		}
		LSetCell(p + 1, p[0], curCell, theList);
		newSize = GetHandleSize(listCellDataHdl);
		if (newSize == origSize)
		{
			err = memFullErr;
			continue;
		}
	}

	return err;
}


/*********************************************************************************************
 *	RemoveEntriesFromStickyPopup - remove entries from the sticky popup menu passed in
 *	stickyPopup.  The index (1-based) of the first entry to remove is passed in index
 *	and the number of entries removed is passed in count.
 *
 *	You cannot remove entries from a sticky popup menu while the menu is popped
 *	(isPopped = true).
 *
 *	The function returns noErr if it succeeds, or a non-zero error code if it fails.
 *********************************************************************************************/

OSErr RemoveEntriesFromStickyPopup(StickyPopupHdl stickyPopup, short index, short count)

{
	if (!stickyPopup)
		return paramErr;
	if ((**stickyPopup).isPopped)
		return paramErr;
	if (!count)
		return noErr;
	LDelRow(count, index - 1, (**stickyPopup).theList);
	return noErr;
}


/*********************************************************************************************
 *	ClearListSelection - deselect all cells in the popup's list
 *********************************************************************************************/


static void ClearListSelection(ListHandle lHandle)

{
	ListBounds	dataBounds;
	short				maxV, maxH;
	Cell				theCell;


	dataBounds = (**lHandle).dataBounds;
	maxV = dataBounds.bottom;
	maxH = dataBounds.right;
	for (theCell.h = dataBounds.left; theCell.h < maxH; theCell.h++)
		for (theCell.v = dataBounds.top; theCell.v < maxV; theCell.v++)
			LSetSelect(false, theCell, lHandle);
}


/*********************************************************************************************
 *	SelectListItem - move the selection in a sticky popup's list to the specific
 *	item specified by itemNo (one-based).  You can pass in -1 for itemNo to specify
 *	the last item in the list.  The popup is specified in stickyPopup; the popup
 *	should be currently popped (isPopped = true).
 *********************************************************************************************/

static void SelectListItem(StickyPopupHdl stickyPopup, short itemNo)

{
	ListHandle	theList;
	Cell				theCell;
	Boolean			selectionExists;
	ListBounds	dataBounds;


	theList = (**stickyPopup).theList;
	dataBounds = (**theList).dataBounds;

	if (itemNo == -1)
		itemNo = dataBounds.bottom - 1;
	else
		itemNo--;

	theCell.v = theCell.h = 0;
	selectionExists = LGetSelect(true, &theCell, theList);
	if (selectionExists && (theCell.v == itemNo))
		return;
	if ((itemNo < dataBounds.top) || (itemNo >= dataBounds.bottom))
		return;
	if (itemNo >= (**stickyPopup).maxVisItems)
		return;

	if (selectionExists)
		LSetSelect(false, theCell, theList);
	theCell.v = itemNo;
	LSetSelect(true, theCell, theList);
}


/*********************************************************************************************
 *	AdjustListSelection - move the selection in a sticky popup's list.  The
 *	popup is specified in stickyPopup; the popup should be currently popped
 *	(isPopped = true).
 *********************************************************************************************/

static void AdjustListSelection(StickyPopupHdl stickyPopup, long adjustFactor)

{
	ListHandle	theList;
	Cell				theCell;
	Boolean			selectionExists;
	short				newCellV;
	ListBounds	dataBounds;


	if (!adjustFactor)
		return;

	theList = (**stickyPopup).theList;
	dataBounds = (**theList).dataBounds;

	theCell.v = theCell.h = 0;
	selectionExists = LGetSelect(true, &theCell, theList);
	newCellV = theCell.v + adjustFactor;
	if ((newCellV < dataBounds.top) || (newCellV >= dataBounds.bottom))
		return;
	if (newCellV >= (**stickyPopup).maxVisItems)
		return;

	if (selectionExists)
		LSetSelect(false, theCell, theList);
	theCell.v = newCellV;
	LSetSelect(true, theCell, theList);
}


/*********************************************************************************************
 *	SelectItemByString - select an item in the popup stickyPopup based on the
 *	string passed in searchStr; the popup should be currently popped
 *	(isPopped = true).  The string compare used in the search/selection
 *	process is insensitive to case and diacritical marks.
 *
 *	In order for this to be useful, the items in the sticky popup menu must
 *	be in alphabetical order.
 *
 *	This routine selects an item, given that you already have the string to
 *	search for and select.  The actual handling of the user's typing is done
 *	in HandleStickyPopupKeyDown and HandleStickyPopupIdle.
 *********************************************************************************************/

static void SelectItemByString(StickyPopupHdl stickyPopup, Str255 searchStr)

{
	ListHandle	theList;
	Str255			typingStr;
	Cell				theCell;
	Cell				curSelected;
	Boolean			selectionExists;
	ListBounds	dataBounds;
	Boolean			found;
	Str255			cellStr;
	short				cellDataLen;


	theList = (**stickyPopup).theList;
	BlockMoveData(searchStr, typingStr, searchStr[0] + 1);
	StickyPopupNormalizeString(typingStr);
	dataBounds = (**theList).dataBounds;
	curSelected.v = curSelected.h = 0;
	selectionExists = LGetSelect(true, &curSelected, theList);

	theCell.v = theCell.h = 0;
	found = false;
	while (!found && (theCell.v < dataBounds.bottom))
	{
		cellDataLen = sizeof(cellStr) - 1;
		LGetCell(cellStr + 1, &cellDataLen, theCell, theList);
		cellStr[0] = cellDataLen;
		StickyPopupNormalizeString(cellStr);
		if (StickyPopupCompareString(typingStr, cellStr, false) <= 0)
			found = true;
		else
			theCell.v++;
	}
	if (!found || (theCell.v == curSelected.v))
		return;
	if (theCell.v >= (**stickyPopup).maxVisItems)
		return;

	if (selectionExists)
		LSetSelect(false, curSelected, theList);
	LSetSelect(true, theCell, theList);
}


/*********************************************************************************************
 *	ResetStickyPopupTyping - resets the global variables used to track the user's typing
 *	when the user is typing to select an item in a sticky popup menu.  This routine
 *	should be called any time we need to "start over" with determining what the user
 *	is trying to type.
 *
 *	LMGetKeyThresh() (word at $18E) returns the delay, in ticks, to wait before
 *	initiating auto-repeat typing; it is set in the "Delay Until Repeat" section of
 *	the Keyboard control panel.  Apple recommends using this value for this kind of
 *	update delay.  We re-read the value from low memory each time this function is
 *	called because the user can change this setting in between calls to
 *	StickyPopupSelect.  We could set this value just once at the start of each call
 *	to StickyPopupSelect, but I do it here so that all the typing-related globals
 *	are reset in the same spot.
 *********************************************************************************************/

static void ResetStickyPopupTyping(void)

{
	StickyPopupKeyUpdateDelay = LMGetKeyThresh();
	StickyPopupTypingStr[0] = 0;
	StickyPopupLastKeyTicks = 0;
}


/*********************************************************************************************
 *	GetStickyPopupMonitorRect - calculates the Rect, in global coordinates, of the monitor
 *	on which the sticky popup will be displayed.  If the monitor contains the menu bar, that
 *	area is subtracted from the Rect before it is returned.  The Rect returned in
 *	*listMonitorRect contains the global Rect within which it is okay to dispaly.  Pass in
 *	the coordinates (global) of the top left corner of the popup in top and left; this point
 *	is used to decide which monitor to target.
 *********************************************************************************************/

static void GetStickyPopupMonitorRect(short top, short left, Rect *listMonitorRect)

{
	Point			topLeft;
	Rect			screenRect;
	short			menuBarHeight;
	Boolean		hasCQD;
	GDHandle	gd;
	Boolean		found;
	long			response;


	menuBarHeight = GetMBarHeight();
	hasCQD = !Gestalt(gestaltQuickdrawVersion, &response) && (response >= gestalt8BitQD);
	if (hasCQD)
	{
		topLeft.v = top;
		topLeft.h = left;
		gd = GetDeviceList();
		found = false;
		while (!found && gd)
		{
			if (TestDeviceAttribute(gd, screenDevice) && PtInRect(topLeft, &(**gd).gdRect))
				found = true;
			else
				gd = GetNextDevice(gd);
		}
		if (!found)
			gd = GetMainDevice();
		screenRect = (**gd).gdRect;
		if (TestDeviceAttribute(gd, mainScreen))
			screenRect.top += menuBarHeight;
	}
	else
	{
		GetQDGlobalsScreenBitsBounds(&screenRect);
		screenRect.top += menuBarHeight;
	}
	*listMonitorRect = screenRect;
}


/*********************************************************************************************
 *	CalcStickyPopupSize - for the popup specified in stickyPopup, makes the following
 *	calculations: calculates the height and width for the popup, based on the number
 *	of entries and the width of each entry; adjusts the location of the top-left corner
 *	of the popup as necessary to insure that the popup stays entirely on the monitor, and
 *	also to adjust the popup so that the item specified in curItem shows up in the top left
 *	location; sets (**stickyPopup).maxVisItems to the number of items which will fit in the
 *	popup without making it go off the bottom of the screen.  Pass in the initial values for
 *	the top-left corner of the popup in top and left (global coordinates); these values
 *	may get adjusted.  The height and width of the popup are returned in popupHeight and
 *	popupWidth.
 *
 *	(jp) Added code to support a "teflon" rect.   This is a rectangle of the screen that the
 *			 sticky popup should never intersect.  For example, the already-typed portion of a
 *			 nickname during nickname completion.
 *********************************************************************************************/

static void CalcStickyPopupSize(StickyPopupHdl stickyPopup, short curItem, Rect *teflonRect, short *top, short *left, short *popupHeight, short *popupWidth)

{
	GrafPtr			origPort;
	ListHandle	theList;
	ListPtr			listPtr;
	short				height;
	short				width;
	short				maxVisItems;
	short				cellCount;
	short				cellHeight;
	short				cellIndent;
	Cell				curCell;
	short				curCellWidth, largestCellWidth;
	Str255			cellStr;
	short				cellDataLen;
	Rect				listMonitorRect,
							stickyPopupRect,
							dstRect;
	short				listBottom;


	theList = (**stickyPopup).theList;
	listPtr = *theList;
	cellCount = listPtr->dataBounds.bottom;
	cellHeight = listPtr->cellSize.v;
	cellIndent = listPtr->indent.h;

	GetPort(&origPort);
	SetPort(GetWindowPort((**stickyPopup).theWindow));
	largestCellWidth = 1;
	curCell.h = 0;
	for (curCell.v = 0; curCell.v < cellCount; curCell.v++)
	{
		cellDataLen = sizeof(cellStr) - 1;
		LGetCell(cellStr + 1, &cellDataLen, curCell, theList);
		cellStr[0] = cellDataLen;
		curCellWidth = cellIndent + StringWidth(cellStr);
		if (curCellWidth > largestCellWidth)
			largestCellWidth = curCellWidth;
	}
	SetPort(origPort);

	height = cellCount * cellHeight;
	width = largestCellWidth + CELL_WIDTH_EXTRA;
	maxVisItems = cellCount;

	if (curItem != 1)
		*top -= cellHeight * (curItem - 1);

	GetStickyPopupMonitorRect(*top, *left, &listMonitorRect);
	listMonitorRect.top += 2;
	listMonitorRect.left += 2;
	listMonitorRect.bottom -= 4;
	listMonitorRect.right -= 4;

/* (jp) Replaced this with code to allow the popup to "drift" past the teflon rect
	if (*top < listMonitorRect.top)
		*top = listMonitorRect.top;
	if (*left < listMonitorRect.left)
		*left = listMonitorRect.left;
	if ((*left + width) > listMonitorRect.right)
		*left = listMonitorRect.left;
	
	listBottom = *top + height;
	if (listBottom > listMonitorRect.bottom)
	{
		*top -= (listBottom - listMonitorRect.bottom);
		if (*top < listMonitorRect.top)
		{
			*top = listMonitorRect.top;
			maxVisItems = (listMonitorRect.bottom - listMonitorRect.top) / cellHeight;
			height = maxVisItems * cellHeight;
		}
	}
*/

	// (jp) Here's the new stuff, featuring Teflon (tm).  All we're really doing is
	//			"drifting" the popup in one direction or another if it intersects with
	//			the teflon rect.  This is certainly not fool-proof and takes advantage
	//			of the way _we_ use sticky popups (as opposed to the way they could
	//			conceivably be used as a general UI element).
	SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
	if (*top < listMonitorRect.top) {
		*top = listMonitorRect.top;
		SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		if (SectRect (&stickyPopupRect, teflonRect, &dstRect)) {
			*top = teflonRect->bottom + 2;
			SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		}
	}

	if (*left < listMonitorRect.left) {
		*left = listMonitorRect.left;
		SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		if (SectRect (&stickyPopupRect, teflonRect, &dstRect)) {
			*left = teflonRect->right + 2;
			SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		}
	}

	if ((*left + width) > listMonitorRect.right) {
		*left = listMonitorRect.right - width;
		SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		if (SectRect (&stickyPopupRect, teflonRect, &dstRect)) {
			*left = teflonRect->left - 2 - width;
			SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		}
	}
	
	listBottom = *top + height;
	if (listBottom > listMonitorRect.bottom)
	{
		*top -= (listBottom - listMonitorRect.bottom);
		if (*top < listMonitorRect.top)
		{
			*top = listMonitorRect.top;
			maxVisItems = (listMonitorRect.bottom - listMonitorRect.top) / cellHeight;
			height = maxVisItems * cellHeight;
		}
		SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		if (SectRect (&stickyPopupRect, teflonRect, &dstRect)) {
			*top = teflonRect->top - 2 - height;
			SetRect (&stickyPopupRect, *left, *top, *left + width, *top + height);
		}
	}
	// (jp) End of replacement stuff

	if ((*left + width) > listMonitorRect.right)
		width = listMonitorRect.right - *left;

	*popupHeight = height;
	*popupWidth = width;
	(**stickyPopup).maxVisItems = maxVisItems;
}


/*********************************************************************************************
 *	HandleStickyPopupMouseDown - handle a mouseDown event in our internal event loop
 *	for sticky popup menus.  stickyPopup is the popup being run; the popup should be
 *	currently popped (isPopped = true).  Pass a pointer to the EventRecord in
 *	popupEvent.
 *
 *	A Boolean value is returned via validSel.  This indicates whether the currently
 *	selected cell in the popup's list should be considered to be the user's selection.
 *
 *	The function returns true if StickyPopupSelect should terminate its event loop
 *	and return a result to the caller, or false if it should keep going and wait for
 *	the user to do more actions.  If the function returns true and *validSel is set
 *	to true, then the currently selected cell in the popup's list is the user's choice.
 *********************************************************************************************/

static Boolean HandleStickyPopupMouseDown(StickyPopupHdl stickyPopup, EventRecord* popupEvent, Boolean *validSel)

{
	GrafPtr			origPort;
	ListHandle	theList;
	Point				clickLoc;
	Point				endMouseLoc;
	WindowPtr		theWindow;
	Cell				origSel;
	Cell				newSel;
	Boolean			selectionExists;


	*validSel = false;
	ResetStickyPopupTyping();

	theList = (**stickyPopup).theList;
	theWindow = (**stickyPopup).theWindow;
	clickLoc = popupEvent->where;
	if (!MouseLocInList(clickLoc, stickyPopup))
		return true;
	origSel.v = origSel.h = 0;
	selectionExists = LGetSelect(true, &origSel, theList);
	GetPort(&origPort);
	SetPort(GetWindowPort(theWindow));
	GlobalToLocal(&clickLoc);
	SetPort(origPort);
	LClick(clickLoc, popupEvent->modifiers, theList);
	LSetDrawingMode(true, theList);
	GetMouse(&endMouseLoc);
	LocalToGlobal(&endMouseLoc);
	newSel.v = newSel.h = 0;
	selectionExists = LGetSelect(true, &newSel, theList);
	if (!MouseLocInList(endMouseLoc, stickyPopup))
	{
		if (selectionExists)
			LSetSelect(false, newSel, theList);
		LSetSelect(true, origSel, theList);
		return false;
	}
	*validSel = selectionExists;
	return true;
}


/*********************************************************************************************
 *	HandleStickyPopupKeyDown - handle a keyDown event in our internal event loop
 *	for sticky popup menus.  stickyPopup is the popup being run; the popup should be
 *	currently popped (isPopped = true).  Pass a pointer to the EventRecord in
 *	popupEvent.
 *
 *	A Boolean value is returned via validSel.  This indicates whether the currently
 *	selected cell in the popup's list should be considered to be the user's selection.
 *
 *	The Boolean parameter allowTyping indicates whether or not the user is allowed to
 *	select entries in the popup's list by typing characters (i.e. typing a G to select
 *	the first item in the list that begins with a G).  In order for this to work, the
 *	items in the popup's list must be in alphabetical order.  If you pass in false for
 *	allowTyping, the user can still use the arrow keys to move the selection up and
 *	down, and use Return or Enter to accept the current selection, or Command-Period or
 *	Escape to cancel the popup.
 *
 *	The function returns true if StickyPopupSelect should terminate its event loop
 *	and return a result to the caller, or false if it should keep going and wait for
 *	the user to do more actions.  If the function returns true and *validSel is set
 *	to true, then the currently selected cell in the popup's list is the user's choice.
 *********************************************************************************************/

static Boolean HandleStickyPopupKeyDown(StickyPopupHdl stickyPopup, EventRecord* popupEvent, Boolean *validSel, Boolean allowTyping)

{
	unsigned char		keyChar; /* ASCII of keystroke */
	unsigned char		keyCode; /* virtual key code of keystroke */
	short						adjust;


	*validSel = false;

	keyChar = popupEvent->message & charCodeMask;
	keyCode = (popupEvent->message & keyCodeMask) >> 8;

	/* if the user pressed Command-Period, Escape, Backspace/Delete, the Clear key on the numeric
			keypad, or the Forward Delete key on the extended keyboard, then cancel the popup */
	if (
						((popupEvent->modifiers & cmdKey) && (keyChar == PERIOD_CHAR_CODE))
				||	(keyCode == ESCAPE_KEY_CODE)
				||	(keyChar == BACKSPACE_CHAR_CODE)
				||	(keyCode == CLEAR_KEY_CODE)
				||	(keyChar == FWD_DEL_CHAR_CODE)
		)
	{
		ResetStickyPopupTyping();
		return true;
	}

	/* if the uesr pressed Return or Enter, then exit the popup with the current selection */
	if ((keyChar == RETURN_CHAR_CODE) || (keyChar == ENTER_CHAR_CODE))
	{
		*validSel = true;
		ResetStickyPopupTyping();
		return true;
	}

	/* ignore Left Arrow and Right Arrow, and reset the typing globals */
	if ((keyChar == LEFT_ARROW_CHAR_CODE) || (keyChar == RIGHT_ARROW_CHAR_CODE))
	{
		ResetStickyPopupTyping();
		return false;
	}

	/* if the user pressed Up Arrow or Down Arrow, use them to navigate up and down the list */
	if ((keyChar == UP_ARROW_CHAR_CODE) || (keyChar == DOWN_ARROW_CHAR_CODE))
	{
		if (keyChar == UP_ARROW_CHAR_CODE)
			adjust = -1;
		else
			adjust = 1;
		AdjustListSelection(stickyPopup, adjust);
		ResetStickyPopupTyping();
		return false;
	}

	/* if the user pressed the Home or End keys, move the selection to the top or bottom of the list, respectively */
	if (keyChar == HOME_CHAR_CODE)
	{
		SelectListItem(stickyPopup, 1);
		ResetStickyPopupTyping();
		return false;
	}
	if (keyChar == END_CHAR_CODE)
	{
		SelectListItem(stickyPopup, -1);
		ResetStickyPopupTyping();
		return false;
	}

	if (!allowTyping)
		return false;

	/* if the user tried a Command key while the popup was running, beep and ignore it, and reset the typing globals */
	if (popupEvent->modifiers & cmdKey)
	{
		SysBeep(5);
		ResetStickyPopupTyping();
		return false;
	}

	/* Accumulate keystrokes for purposes of selecting an item in the popup via typing.  The actual selection
			of the item is performed by HandleStickyPopupIdle. */
	StickyPopupLastKeyTicks = popupEvent->when;
	if (StickyPopupTypingStr[0] < 255)
		StickyPopupTypingStr[++StickyPopupTypingStr[0]] = keyChar;

	return false;
}


/*********************************************************************************************
 *	HandleStickyPopupIdle - handle a null event in our internal event loop
 *	for sticky popup menus.  stickyPopup is the popup being run; the popup should be
 *	currently popped (isPopped = true).
 *
 *	This is where we check to see if we should adjust the currently selected
 *	cell in the popup's list based on what the user has been typing.
 *
 *	If a MenuHook proc is present (long at $A30), it does not get called.
 *********************************************************************************************/

static void HandleStickyPopupIdle(StickyPopupHdl stickyPopup)

{
	if (!StickyPopupTypingStr[0])
		return;
	if (TickCount() < (StickyPopupLastKeyTicks + StickyPopupKeyUpdateDelay))
		return;
	SelectItemByString(stickyPopup, StickyPopupTypingStr);
	ResetStickyPopupTyping();
}


/*********************************************************************************************
 *	StickyPopupSelect - display a sticky popup menu and allow the user to make a
 *	selection.  This routine is comparable to the Mac toolbox routine PopUpMenuSelect.
 *
 *	stickyPopup is a StickyPopupHdl for a sticky popup you have already created.  Before
 *	calling this routine, call NewStickyPopup to create the popup, and
 *	AddEntriesToStickyPopup to add items to the popup.
 *
 *	The top-left corner of the popup is placed in the location specified by top and
 *	left (global coordinates).  The popup comes up with the item whose index (1-based) is
 *	specified in curItem as the currently selected item.
 *
 *	The Boolean parameter allowTyping indicates whether or not the user is allowed to
 *	select entries in the popup's list by typing characters (i.e. typing a G to select
 *	the first item in the list that begins with a G).  In order for this to work, the
 *	items in the popup's list must be in alphabetical order.  If you pass in false for
 *	allowTyping, the user can still use the arrow keys to move the selection up and
 *	down, and use Return or Enter to accept the current selection, or Command-Period or
 *	Escape to cancel the popup.
 *
 *	The function returns a longword to indicate the user's selection.  The high word of
 *	this value is always zero.  The low word is zero if the user made no selection (i.e.
 *	canceled the popup or let go of the mouse outside of the popup's boundaries).  If the
 *	user made a valid selection, the low word contains the index (1-based) of the item
 *	which the user selected.
 *
 *	This function is designed to work the same as PopUpMenuSelect (and to handle the
 *	parameters the same way), except that this function has an additional parameter for
 *	allowTyping, and it doesn't return a menu ID in the high word of the function result.
 *
 *	If a MenuHook proc is present (ProcPtr at $A30), it does not get called.
 *********************************************************************************************/

long StickyPopupSelect(StickyPopupHdl stickyPopup, short top, short left, short curItem, Boolean allowTyping, Rect *teflonRect)

{
	long						result;
	StickyPopupPtr	popupPtr;
	SignedByte			origState;
	WindowPtr				popupWindow;
	ListHandle			theList;
	Point						cellSize;
	Cell						theCell;
	short						popupHeight, popupWidth;
	Boolean					done, validSel;
	EventRecord			popupEvent;


	if (!stickyPopup)
		return 0;
	if ((**stickyPopup).isPopped || CurStickyPopup)
		return 0;

	origState = HGetState((Handle)stickyPopup);
	MoveHHi((Handle)stickyPopup); /* ignore failure */
	HLock((Handle)stickyPopup);
	popupPtr = *stickyPopup;
	popupWindow = popupPtr->theWindow;
	theList = popupPtr->theList;

	popupPtr->isPopped = true;
	CurStickyPopup = stickyPopup;
	ResetStickyPopupTyping();

	CalcStickyPopupSize(stickyPopup, curItem, teflonRect, &top, &left, &popupHeight, &popupWidth);
	MoveWindow(popupWindow, left, top, false);
	SizeWindow(popupWindow, popupWidth, popupHeight, false);
	LSize(popupWidth, popupHeight, theList);
	cellSize.v = (**theList).cellSize.v;
	cellSize.h = popupWidth;
	LCellSize(cellSize, theList);
	ClearListSelection(theList);
	theCell.v = curItem - 1;
	theCell.h = 0;
	LSetSelect(true, theCell, theList);
	BringToFront(popupWindow);
	ShowHide(popupWindow, true);
	LSetDrawingMode(true, theList);
	LUpdate(MyGetPortVisibleRegion(GetWindowPort(popupWindow)), theList);

	done = false;
	validSel = false;
	while (!done)
	{
		if (!GetNextEvent(STICKY_SELECT_EVENT_MASK, &popupEvent))
			HandleStickyPopupIdle(stickyPopup);
		else switch (popupEvent.what)
		{
			case mouseDown:
				if (HandleStickyPopupMouseDown(stickyPopup, &popupEvent, &validSel))
					done = true;
				break;
			case keyDown:
			case autoKey:
				if (HandleStickyPopupKeyDown(stickyPopup, &popupEvent, &validSel, allowTyping))
					done = true;
				break;
		}
	}
	if (validSel)
	{
		theCell.v = theCell.h = 0;
		LGetSelect(true, &theCell, theList);
		result = theCell.v + 1;
	}
	else
		result = 0;

	LSetDrawingMode(false, theList);
	ShowHide(popupWindow, false);

	popupPtr->maxVisItems = 0;
	popupPtr->isPopped = false;
	CurStickyPopup = nil;
	HSetState((Handle)stickyPopup, origState);

	return result;
}