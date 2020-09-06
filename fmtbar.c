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

#include "fmtbar.h"
#define FILE_NUM 80
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* MJN *//* new file *//* moved text formatting toolbar code from compact.c to here */

void TFBFontPopupPrepare(MyWindowPtr win, EventRecord* event);
void TFBSizePopupPrepare(MyWindowPtr win, EventRecord* event);
void TFBColorPopupPrepare(MyWindowPtr win, EventRecord* event);
void PlaceTxtFmtBarIcons(MyWindowPtr win,Boolean visible);
void GetCurEditorMarginsLo(PETEHandle pte, PSMPtr currentTxtMargin, long paraIndex);

#pragma segment CompMng
/************************************************************************
 * private functions
 ************************************************************************/
MenuHandle GetSizePopupMenuHdl(void); /* MJN *//* new routine */
void InitTextFormattingBarGlobals(void); /* MJN *//* new routine */
Rect *CompTextFormattingBarIconRect(Rect *r,MessHandle messH,short index); /* MJN *//* new routine */
ControlHandle GetNextCompWindowTFBIcon(MyWindowPtr win,ControlHandle prevControl,long iconTag); /* MJN *//* new routine */

#define NUM_TEXT_BAR_ICONS 15 /* MJN *//* text formatting toolbar */
#define FIRST_TEXT_BTN (ICON_BAR_NUM+10) /* MJN *//* text formatting toolbar */ //SD - moved 10 buttons higher
#define LAST_TEXT_BTN (FIRST_TEXT_BTN+NUM_TEXT_BAR_ICONS-1) /* MJN *//* text formatting toolbar */
#define NUM_TEXT_BAR_POPUPS 3
#define FIRST_TEXT_POPUP FIRST_TEXT_BTN+NUM_TEXT_BAR_ICONS
#define LAST_TEXT_POPUP (FIRST_TEXT_POPUP+NUM_TEXT_BAR_POPUPS-1)
	/* MJN *//* text formatting toolbar */

#define TXT_FMT_BAR_ICON_WIDTH 19
#define TXT_FMT_BAR_ICON_HEIGHT 19

#define TFB_IDLE_INTERVAL 15 /* minimum frequency, in ticks, that the text formatting bar should be checked and updated
																to match the text settings of the current text selection */

	static Boolean txtFmtBarGlobalsInitted = false; /* true if InitTextFormattingBarGlobals has been called; used to make sure the globals are only initialized once */
	static short tfbHeight; /* height (in pixels) of the text formatting bar */
	static Boolean tfbColorPopupEnabled; /* true if the Color popup should appear in the text formatting bar; normally set to false if Color QuickDraw is not present */
	static short tfbPopupFontNum; /* font family ID to use for drawing text in the formatting toolbar popups */
	static short tfbPopupFontSize; /* font size to use for drawing text in the formatting toolbar popups */
	static char tfbPopupItemMarkChar; /* character to use as the check mark character when poping a popup menu in the formatting toolbar; make sure it's a valid character in the font specified by tfbPopupFontNum */
	static Rect Rects[NUM_TEXT_BAR_POPUPS+1];
#define tfbFontPopupRect Rects[0] /* rect of the font popup in the formatting toolbar (in local coordinates) */
#define tfbSizePopupRect Rects[1] /* rect of the size popup in the formatting toolbar (in local coordinates) */
#define tfbColorPopupRect Rects[2] /* rect of the color popup in the formatting toolbar (in local coordinates) */
#define tfbOverallRect Rects[3] /* rect of the color popup in the formatting toolbar (in local coordinates) */
	static short MenuIDs[]={FONT_HIER_MENU,COLOR_HIER_MENU,COLOR_HIER_MENU};
	static short lastDrawnFontPopupItem; /* item number of the menu item of the font popup that was last drawn */
	static short lastDrawnSizePopupItem; /* item number of the menu item of the size popup that was last drawn */
	static short lastDrawnColorPopupItem; /* item number of the menu item of the color popup that was last drawn */
	static Boolean needTBFEnableCheck; /* true if we need to check the formatting toolbar's enabled state to see if it needs to change (see comments for RequestTFBEnableCheck() for more info) */
	static unsigned long lastTFBIdle; /* system tick count of the last time TextFormattingBarIdle() did idle-time processing */
	static Boolean needTFBIdleImmediately; /* true if we want to make TextFormattingBarIdle() do idle-time processing without waiting for the next idle interval */

	/* NOTE: The order of items in the array txtFmtIcons must be kept in synch with the enum which
						follows it, so that the icon resource IDs match with the button numbers.  Also, don't
						forget to keep the Balloon Help strings in 'STR#' ID 15400 in help.r in sync with
						this list. */
	static uLong txtFmtIcons[]={BOLD_TEXT_SICN,ITALIC_TEXT_SICN,UNDERLINE_TEXT_SICN,
															UNQUOTE_TEXT_SICN,QUOTE_TEXT_SICN,
															LEFT_JUST_TEXT_SICN,CENTER_JUST_TEXT_SICN,RIGHT_JUST_TEXT_SICN,
															TEXT_INDENT_OUT_SICN,TEXT_INDENT_IN_SICN,
															BULLET_ICON,RULE_ICON,LINK_ICON,
															CLEAR_TEXT_FORMATTING_SICN,EMOTICON_ICON};
	enum {BOLD_TEXT_BTN = FIRST_TEXT_BTN,ITALIC_TEXT_BTN,UNDERLINE_TEXT_BTN,
					UNQUOTE_TEXT_BTN,QUOTE_TEXT_BTN,
					LEFT_JUST_TEXT_BTN,CENTER_JUST_TEXT_BTN,RIGHT_JUST_TEXT_BTN,
					TEXT_INDENT_OUT_BTN,TEXT_INDENT_IN_BTN,
					BULLET_BTN,RULE_BTN,LINK_BTN,
					CLEAR_TEXT_FORMATTING_BTN,
					EMOTICON_BTN}; /* last item must be equal to LAST_TEXT_BTN */
	enum {FONT_TEXT_POPUP = FIRST_TEXT_POPUP,SIZE_TEXT_POPUP,COLOR_TEXT_POPUP}; /* last item must be equal to LAST_TEXT_POPUP */
	/* This array controls how the icons are grouped in the text formatting bar.  The sum of
			the members of the array must be exactly equal to NUM_TEXT_BAR_ICONS. */
	static uLong txtFmtIconSetCounts[]={3,2,3,3,2,1,1};
#define NUM_TXT_FMT_ICON_SETS 6
	/* The contents of the array txtFmtBarIconCtrlTags must be kept in sync with the prior enum and array */
	static long txtFmtBarIconCtrlTags[NUM_TEXT_BAR_ICONS] =
																{BOLD_TEXT_BTN,ITALIC_TEXT_BTN,UNDERLINE_TEXT_BTN,
																UNQUOTE_TEXT_BTN,QUOTE_TEXT_BTN,
																LEFT_JUST_TEXT_BTN,CENTER_JUST_TEXT_BTN,RIGHT_JUST_TEXT_BTN,
																TEXT_INDENT_OUT_BTN,TEXT_INDENT_IN_BTN,
																BULLET_BTN,RULE_BTN,LINK_BTN,
																CLEAR_TEXT_FORMATTING_BTN,EMOTICON_BTN};

#pragma segment Balloon
/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarHelp - provide help for composition windows
 ************************************************************************/
Boolean TextFormattingBarHelp(MyWindowPtr win,Point mouse)
{
	short tfbItemIndex;
	Rect tfbItemRect;
	short iconNum=0;
	ControlHandle ctrlHdl;
	short helpItemIndex;

	if (!TextFormattingBarVisible(win)) return false;
	
	tfbItemIndex = 0;
	if (PtInRect(mouse,&tfbFontPopupRect))
	{
		tfbItemIndex = 2;
		tfbItemRect = tfbFontPopupRect;
	}
	else if (PtInRect(mouse,&tfbSizePopupRect))
	{
		tfbItemIndex = 3;
		tfbItemRect = tfbSizePopupRect;
	}
	else if (tfbColorPopupEnabled && PtInRect(mouse,&tfbColorPopupRect))
	{
		tfbItemIndex = 4;
		tfbItemRect = tfbColorPopupRect;
	}
	else
	{
		for (iconNum=NUM_TEXT_BAR_ICONS-1,ctrlHdl=nil;!tfbItemIndex&&(iconNum>=0);iconNum--)
		{
			ctrlHdl = GetNextCompWindowTFBIcon(win,ctrlHdl,txtFmtBarIconCtrlTags[iconNum]);
			GetControlBounds(ctrlHdl,&tfbItemRect);
			if (ctrlHdl&&PtInRect(mouse,&tfbItemRect))
			{
				tfbItemIndex = 5+iconNum;
				break;
			}
		}
	}
	if (!tfbItemIndex) return false;
	helpItemIndex = (tfbItemIndex-1)*2+1;
	if (!TextFormattingBarEnabled(win) ||
			PrefIsSet(PREF_SEND_ENRICHED_NEW) && (iconNum==BULLET_BTN-FIRST_TEXT_BTN || iconNum==RULE_BTN-FIRST_TEXT_BTN || iconNum==LINK_BTN-FIRST_TEXT_BTN) ||
			iconNum==LINK_BTN-FIRST_TEXT_BTN && !win->hasSelection)
		helpItemIndex++;
	MyBalloon(&tfbItemRect,80,0,TXT_FMT_BAR_HELP_STRN+helpItemIndex,0,nil);
	return true;
}

/* MJN *//* new routine */
/************************************************************************
 * AddTextFormattingBarIcons - called from CompGonnaShow to add
 * formatting toolbar icons to the composition window
 ************************************************************************/
void AddTextFormattingBarIcons(MyWindowPtr win,Boolean visible,Boolean enabled)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle ctrlHdl;
	ControlHandle fmtbarCtl;
	short iconNum;
	Rect iconRect = tfbOverallRect;
	OSErr err;
	
	// create a user pane for all the controls
	fmtbarCtl = NewControlSmall(winWP,&iconRect,"\p",false,kControlSupportsEmbedding,0,0,kControlUserPaneProc,kControlUserPaneProc);
	err = EmbedControl(fmtbarCtl,win->topMarginCntl);
	ASSERT(!err);
	
	// make sure we have globals
	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	
	SetRect(&iconRect,1,1,1+TXT_FMT_BAR_ICON_WIDTH,1+TXT_FMT_BAR_ICON_HEIGHT);
	// add the icon buttons
	for (iconNum=0;iconNum<NUM_TEXT_BAR_ICONS;iconNum++)
	{
		Boolean emote = iconNum==EMOTICON_BTN-FIRST_TEXT_BTN;
		ctrlHdl = NewControlSmall(winWP,&iconRect,"\p",false,0,emote ? kControlBehaviorMultiValueMenu : 0,0,kControlBevelButtonSmallBevelProc,FIRST_TEXT_BTN+iconNum);
		if (ctrlHdl)
		{
			SetBevelIcon(ctrlHdl,txtFmtIcons[iconNum],0,0,nil);
			err = EmbedControl(ctrlHdl,fmtbarCtl);
			ASSERT(!err);
			if (emote)
			{
				SetBevelMenu(ctrlHdl,0,GetMHandle(EMOTICON_HIER_MENU));
				SetControlMaximum(ctrlHdl,CountMenuItems(GetMHandle(EMOTICON_HIER_MENU)));
			}
		}
	}
	
	// add the divider
	if (ctrlHdl = NewControlSmall(winWP,&iconRect,"\p",false,0,0,0,kControlSeparatorLineProc,kSeparatorRefCon))
	{
		err = EmbedControl(ctrlHdl,win->topMarginCntl);
		ASSERT(!err);
	}
		
	
	//add the menus
	for (iconNum=0;iconNum<NUM_TEXT_BAR_POPUPS;iconNum++)
	{
		ctrlHdl = GetNewControlSmall(TEXT_BAR_FONT_CNTL+iconNum,winWP);
		if (ctrlHdl)
		{
			SetControlReference(ctrlHdl,FIRST_TEXT_POPUP+iconNum);
			err = EmbedControl(ctrlHdl,fmtbarCtl);
			ASSERT(!err);
			SetBevelTextAlign(ctrlHdl,teFlushDefault);
			SetBevelTextOffset(ctrlHdl,MAX_APPEAR_RIM);
		}
	}
	PlaceTxtFmtBarIcons(win,visible);
	if (!enabled) DeactivateControl(fmtbarCtl);
	SetControlVisibility(fmtbarCtl,visible,false);
}

#ifdef NEVER
/* MJN *//* new routine */
/************************************************************************
* GetSizePopupMenuHdl - call this routine instead of GetMHandle to get
* a MenuHandle to the Size popup menu.  This routine will first call
* GetMHandle, and if it returns nil, it will re-load the menu and
* re-insert it into the menu list.  This is to handle any cases where
* ClearMenuBar or GetNewMBar has been called.
 ************************************************************************/
MenuHandle GetSizePopupMenuHdl(void)
{
	MenuHandle menuHdl;

	menuHdl = GetMHandle(TFB_SIZE_POPUP_MENU);
	if (menuHdl)
		return menuHdl;
	menuHdl = GetMenu(TFB_SIZE_POPUP_MENU);
	if (menuHdl) InsertMenu(menuHdl,-1);
	return menuHdl;
}
#endif

/* MJN *//* new routine */
/************************************************************************
* InitTextFormattingBarGlobals - sets up globals used by the text
* formatting toolbar.  The global Boolean txtFmtBarGlobalsInitted
* indicates whether or not this routine has already been called.
 ************************************************************************/
void InitTextFormattingBarGlobals(void)
{
	GrafPtr origPort,scratchPort;
	FontInfo popupFontInfo;
	short popupFontHeight, popupHeight;
	short popupTop, popupBottom;
	short popupWidth;
	Str255 scratchStr;

	if (txtFmtBarGlobalsInitted) return;

	tfbHeight = GetRLong(TXT_FMT_BAR_HEIGHT);
	tfbColorPopupEnabled = ThereIsColor;
	tfbPopupFontNum = SmallSysFontID();
	tfbPopupFontSize = SmallSysFontSize();
	GetRString(scratchStr,TXT_FMT_BAR_POPUP_CHECK_MARK);
	if (scratchStr[0]) tfbPopupItemMarkChar = scratchStr[1];
	else tfbPopupItemMarkChar = '>';

	GetPort(&origPort);
	MyCreateNewPort(scratchPort);
	TextFont(tfbPopupFontNum);
	TextSize(tfbPopupFontSize);
	GetFontInfo(&popupFontInfo);
	popupFontHeight = popupFontInfo.ascent + popupFontInfo.descent;
	DisposePort(scratchPort);
	SetPort(origPort);
	popupHeight = popupFontHeight + 4;

	popupTop = GetRLong(COMP_TOP_MARGIN) + ((tfbHeight - popupHeight) / 2);
	popupBottom = popupTop + popupHeight;

	popupWidth = GetRLong(TXT_FMT_BAR_FONT_POPUP_MAX_WIDTH);
	tfbFontPopupRect.top = popupTop;
	tfbFontPopupRect.left = INSET-2;
	tfbFontPopupRect.bottom = popupBottom;
	tfbFontPopupRect.right = tfbFontPopupRect.left + popupWidth;

	popupWidth = GetRLong(TXT_FMT_BAR_SIZE_POPUP_MAX_WIDTH);
	tfbSizePopupRect.top = popupTop;
	tfbSizePopupRect.left = tfbFontPopupRect.right;
	tfbSizePopupRect.bottom = popupBottom;
	tfbSizePopupRect.right = tfbSizePopupRect.left + popupWidth;

	if (tfbColorPopupEnabled)
	{
		popupWidth = GetRLong(TXT_FMT_BAR_COLOR_POPUP_MAX_WIDTH);
		tfbColorPopupRect.top = popupTop;
		tfbColorPopupRect.left = tfbSizePopupRect.right;
		tfbColorPopupRect.bottom = popupBottom;
		tfbColorPopupRect.right = tfbColorPopupRect.left + popupWidth;
	}
	else
	{
		tfbColorPopupRect = tfbSizePopupRect;
		tfbColorPopupRect.left = tfbColorPopupRect.right;
	}
	
	CompTextFormattingBarIconRect(&tfbOverallRect,0,NUM_TEXT_BAR_ICONS-1);
	SetRect(&tfbOverallRect,0,GetRLong(COMP_TOP_MARGIN)-2,tfbOverallRect.right+INSET,GetRLong(COMP_TOP_MARGIN)+tfbHeight);

	lastDrawnFontPopupItem = -1;
	lastDrawnSizePopupItem = -1;
	lastDrawnColorPopupItem = -1;

	needTBFEnableCheck = false;

	lastTFBIdle = 0;
	needTFBIdleImmediately = false;

	txtFmtBarGlobalsInitted = true;
}

/* MJN *//* new routine */
/************************************************************************
* GetTxtFmtBarHeight - returns the height for the text formatting toolbar
 ************************************************************************/
short GetTxtFmtBarHeight(void)
{
	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	return(tfbHeight);
}

/* MJN *//* new routine */
/************************************************************************
 * GetTxtFmtBarRect - get the Rect, in local coordinates of the
 * window win, of the text formatting toolbar.  If the toolbar is not
 * visible, the returned rectangle is set to empty.
 ************************************************************************/
void GetTxtFmtBarRect(MyWindowPtr win,Rect *txtFmtBarRect)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	long iconBarBottom;
	Rect compWinRect;

	if (!TextFormattingBarVisible(win))
	{
		SetRect(txtFmtBarRect,0,0,0,0);
		return;
	}
	iconBarBottom = GetRLong(COMP_TOP_MARGIN);
	GetWindowPortBounds(winWP,&compWinRect);
	txtFmtBarRect->top = iconBarBottom-1;
	txtFmtBarRect->left = compWinRect.left;
	txtFmtBarRect->bottom = win->topMargin;
	txtFmtBarRect->right = compWinRect.right;
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarVisible - returns true if text formatting bar should
 * be drawn in composition window
 ************************************************************************/
Boolean TextFormattingBarVisible(MyWindowPtr win)
{
	MessHandle messH;

	if (!win) return(false);
	if (!TextFormattingBarAllowed(win)) return(false);
	messH = (MessHandle)GetMyWindowPrivateData(win);
	return(MessOptIsSet(messH,OPT_COMP_TOOLBAR_VISIBLE));
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarEnabled - returns true if text formatting bar is
 *  enabled
 ************************************************************************/
Boolean TextFormattingBarEnabled(MyWindowPtr win)
{
	MessHandle messH;

	if (!win) return(false);
	if (!TextFormattingBarAllowed(win)) return(false);
	messH = (MessHandle)GetMyWindowPrivateData(win);
	return((*messH)->textFormatBarEnabled);
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarOK - returns true if text formatting bar should
 * be enabled (not necessesarily the same as whether or not the text
 * formatting bar is actually enabled)
 ************************************************************************/
Boolean TextFormattingBarOK(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);

	if (!win) return(false);
	if (InBG) return(false);
	if (!IsWindowVisible (winWP) || !IsWindowHilited (winWP)) return(false);
	if (winWP!=FrontWindow_()) return(false);
	if (!TextFormattingBarAllowed(win)) return(false);
	if (!win->isActive) return(false);
	if (!win->pte) return(false);
	if (PETESelectionLocked(PETE,win->pte,-1,-1)) return(false);
	if (CompHeadCurrent(win->pte)) return(false);
	return true;
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarAllowed - returns true if the window given in win is
 * of a type that is capable of including the text formatting bar
 ************************************************************************/
Boolean TextFormattingBarAllowed(MyWindowPtr win)
{
	if (!win) return(false);
	return IsCompWindow(GetMyWindowWindowPtr(win));
}

/* MJN *//* new routine */
/************************************************************************
 * TFBMenusAllowed - returns true if menus and menu items associated with
 * the text formatting bar should be enabled, false if they should be
 * disabled
 ************************************************************************/
Boolean TFBMenusAllowed(MyWindowPtr activeWindow)
{
	return TextFormattingBarAllowed(activeWindow);
}

/* MJN *//* new routine */
/************************************************************************
 * GetNextCompWindowTFBIcon - utility routine for getting a
 * ControlHandle to one of the icon button controls in the text
 * formatting toolbar.  The parameter iconTag contains an integer value
 * indicating which icon to look for, and will be compared against the
 * refCon of each control in the window's control list.  The values for
 * passing into iconTag are defined at the top of this file as enum
 * constants; search for the first occurance of BOLD_TEXT_BTN in this
 * file to find the enum.  The window referenced by win is the window
 * whos control list will be searched.  If you are looping through all
 * of the text formatting bar icons, you can pass in the ControlHandle
 * that was last returned by this routine in the parameter prevControl,
 * so that this routine doesn't have to search from the beginning of the
 * control list each time; pass in nil for prevControl the first time
 * you call this routine.  If the control cannot be found, or there is
 * an invalid input parameter, GetNextCompWindowTFBIcon returns nil.
 ************************************************************************/
ControlHandle GetNextCompWindowTFBIcon(MyWindowPtr win,ControlHandle prevControl,long iconTag)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle ctrl;

	if (!win) return(nil);
	if (!IsCompWindow(winWP)) return(nil);
	if (prevControl && (GetControlOwner(prevControl)!=winWP)) return(nil);
	ctrl = prevControl?prevControl:GetControlList(winWP);
	while (ctrl)
	{
		if (GetControlReference(ctrl) == iconTag) return(ctrl);
		ctrl = GetNextControl(ctrl);
	}
	if (!prevControl) return(nil);
	ctrl = GetControlList(winWP);
	while (ctrl)
	{
		if (GetControlReference(ctrl) == iconTag) return(ctrl);
		ctrl = GetNextControl(ctrl);
	}
	return(nil);
}

/* MJN *//* new routine */
/************************************************************************
 * CurFontPopupItem - returns the menu item number of the item in the
 * font popup menu which matches the currently active font
 ************************************************************************/
short CurFontPopupItem(MyWindowPtr win);
short CurFontPopupItem(MyWindowPtr win)
{
	PETETextStyle ps;
	PETEStyleEntry pse;
	Str255 fontName;
	MenuHandle menuHdl;

	PeteStyleAt(win->pte,kPETECurrentStyle,&pse);
	ps = pse.psStyle.textStyle;
	if (ps.tsFont==kPETEDefaultFont) ps.tsFont = FontID;
	else if (ps.tsFont==kPETEDefaultFixed) ps.tsFont = FixedID;
	menuHdl = GetMHandle(FONT_HIER_MENU);
	GetFontName(ps.tsFont,fontName);
	return(FindItemByName(menuHdl,fontName));
}

/* MJN *//* new routine */
/************************************************************************
 * CurSizePopupItem - returns the menu item number of the item in the
 * size popup menu which matches the currently active font
 ************************************************************************/
short CurSizePopupItem(MyWindowPtr win);
short CurSizePopupItem(MyWindowPtr win)
{
	PETETextStyle ps;
	PETEStyleEntry pse;

	PeteStyleAt(win->pte,kPETECurrentStyle,&pse);
	ps = pse.psStyle.textStyle;
	if (ps.tsSize==kPETEDefaultSize) ps.tsSize = FontSize;
#ifdef USERELATIVESIZES
	if (ps.tsSize < 0)
	{
		short tempSize;
		switch(tempSize = ps.tsSize - kPETERelativeSizeBase)
		{
			case 0 :
				return tfbSizeNormal;
			case 1 :
				return tfbSizeBig;
			case 2 :
				return tfbSizeBigger;
			case 3 :
		Biggest :
				return tfbSizeBiggest;
			default :
				if(tempSize > 0) goto Biggest;
			case -1 :
				return tfbSizeSmall;
		}
	}
#endif
	if (ps.tsSize < FontSize) return tfbSizeSmall;
	else if (ps.tsSize > IncrementTextSize(FontSize,2)) return tfbSizeBiggest;
	else if (ps.tsSize > IncrementTextSize(FontSize,1)) return tfbSizeBigger;
	else if (ps.tsSize > FontSize) return tfbSizeBig;
	else return tfbSizeNormal;
}

/* MJN *//* new routine */
/************************************************************************
 * CurColorPopupItem - returns the menu item number of the item in the
 * color popup menu which matches the currently active font
 ************************************************************************/
short CurColorPopupItem(MyWindowPtr win);
short CurColorPopupItem(MyWindowPtr win)
{
	PETETextStyle ps;
	PETEStyleEntry pse;
	RGBColor curColor;
	MenuHandle menuHdl;
	MCEntryPtr menuColorInfo;
	RGBColorPtr itemColor;
	short numItems,i;

	if (!tfbColorPopupEnabled) return(1);
	PeteStyleAt(win->pte,kPETECurrentStyle,&pse);
	ps = pse.psStyle.textStyle;
	curColor = ps.tsColor;
	menuHdl = GetMHandle(COLOR_HIER_MENU);
	numItems = CountMenuItems(menuHdl);
	for (i=1;i<=numItems;i++)
	{
		menuColorInfo = GetMCEntry(COLOR_HIER_MENU,i);
		if (!menuColorInfo) continue;
		itemColor = &menuColorInfo->mctRGB2;
		if ((curColor.red==itemColor->red)&&(curColor.green==itemColor->green)&&(curColor.blue==itemColor->blue)) return(i);
	}
	return(1);
}

/* MJN *//* new routine */
/************************************************************************
 * InvalTextFormattingBar - call InvalRect for the Rect containing the
 * text formatting bar in a window
 ************************************************************************/
void InvalTextFormattingBar(MyWindowPtr win)
{
	GrafPtr origPort;
	Rect txtFmtBarRect;

	GetTxtFmtBarRect(win,&txtFmtBarRect);
	if (EmptyRect(&txtFmtBarRect)) return;
	GetPort(&origPort);
	SetPort(GetMyWindowCGrafPtr(win));
	InvalWindowRect(GetMyWindowWindowPtr(win),&txtFmtBarRect);
	SetPort(origPort);
}

/* MJN *//* new routine */
long TFBPopUpMenuSelect(MenuHandle theMenu, MyWindowPtr win, short top, short left, short curItem);
long TFBPopUpMenuSelect(MenuHandle theMenu, MyWindowPtr win, short top, short left, short curItem)
{
	long result;
	GrafPtr origPort;
	SInt16 origSysFontFamID;
	SInt16 origSysFontSize;
	Point popLoc;
	short itemNum;
	short origItemMark;

	if (!theMenu)
		return 0;
	GetPort(&origPort);
	SetPort(GetMyWindowCGrafPtr(win));
	popLoc.v = top;
	popLoc.h = left;
	LocalToGlobal(&popLoc);
	SetPort(origPort);
	GetItemMark(theMenu,curItem,&origItemMark);
	for (itemNum=CountMenuItems(theMenu);itemNum;itemNum--)
		if (!HasSubmenu(theMenu,itemNum)) SetItemMark(theMenu,itemNum,noMark);
	SetItemMark(theMenu,curItem,tfbPopupItemMarkChar);
	origSysFontFamID = LMGetSysFontFam();
	origSysFontSize = LMGetSysFontSize();
	LMSetSysFontFam(tfbPopupFontNum);
	LMSetSysFontSize(tfbPopupFontSize);
	LMSetLastSPExtra(-1);
	result = PopUpMenuSelect(theMenu, popLoc.v, popLoc.h, curItem);
	LMSetSysFontFam(origSysFontFamID);
	LMSetSysFontSize(origSysFontSize);
	LMSetLastSPExtra(-1);
	SetItemMark(theMenu,curItem,origItemMark);
	return result;
}

/************************************************************************
 * TFBFontPopupPrepare - prepare the font popup
 ************************************************************************/
void TFBFontPopupPrepare(MyWindowPtr win, EventRecord* event)
{
	ControlHandle cntl = FindControlByRefCon(win,FONT_TEXT_POPUP);
	MenuHandle menuHdl = GetMHandle(FONT_HIER_MENU);

	EnableMenuItems(False);
	if (menuHdl) CheckNone(menuHdl);
	if (cntl) SetBevelMenuValue(cntl,CurFontPopupItem(win));
}

/************************************************************************
 * TFBSizePopupPrepare - prepare the font popup
 ************************************************************************/
void TFBSizePopupPrepare(MyWindowPtr win, EventRecord* event)
{
	ControlHandle cntl = FindControlByRefCon(win,SIZE_TEXT_POPUP);
	MenuHandle menuHdl = GetMHandle(TEXT_SIZE_HIER_MENU);
	short mark;
	short i;

	if (menuHdl && cntl)
	{
		SetMenuTexts(0,False);
		for (i=CountMenuItems(menuHdl);i;i--)
		{
			GetItemMark(menuHdl,i,&mark);
			if (mark!=noMark)
			{
				SetBevelMenuValue(cntl,i);
				break;
			}
		}
	}
}

/************************************************************************
 * TFBColorPopupPrepare - prepare the color popup
 ************************************************************************/
void TFBColorPopupPrepare(MyWindowPtr win, EventRecord* event)
{
	ControlHandle cntl = FindControlByRefCon(win,COLOR_TEXT_POPUP);
	MenuHandle menuHdl = GetMHandle(COLOR_HIER_MENU);

	EnableMenuItems(False);
	if (menuHdl) CheckNone(menuHdl);
	if (cntl) SetBevelMenuValue(cntl,CurColorPopupItem(win));
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarClick - tests for click in an area of the text
 * formatting tool bar other than one of the controls (i.e. the three
 * popups).  If the click is in one of these spots, this function does
 * the appropriate handling, and then returns true.  If the click is not
 * in any such area, then the function returns false, in which case, the
 * caller must do further processing on the click, including checking to
 * see if the click was in a control.
 ************************************************************************/
Boolean TextFormattingBarClick(MyWindowPtr win, EventRecord* event, Point clickLoc)
{
	Point pt;

	if (!TextFormattingBarVisible(win)) return(false);
	if (!TextFormattingBarOK(win)) return(false);
	if (!TextFormattingBarEnabled(win)) return(false);

	pt = event->where;
	GlobalToLocal(&pt);
	
	if (PtInRect(pt,&tfbFontPopupRect)) TFBFontPopupPrepare(win,event);
	else if (PtInRect(pt,&tfbSizePopupRect)) TFBSizePopupPrepare(win,event);
	else if (PtInRect(pt,&tfbColorPopupRect)) TFBColorPopupPrepare(win,event);

	CheckNone(GetMHandle(EMOTICON_HIER_MENU));
	
	return false;
}

/* MJN *//* new routine */
/*****************************************************************************
 * HideTxtFmtBar - hide the text formatting toolbar in the composition window
 *****************************************************************************/
void HideTxtFmtBar(MyWindowPtr win)
{
	MessHandle messH;
	ControlHandle ctrlHdl;

	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	if (!TextFormattingBarAllowed(win)) return;
	if (!TextFormattingBarVisible(win)) return;
	messH = (MessHandle)GetMyWindowPrivateData(win);
	ClearMessOpt(messH,OPT_COMP_TOOLBAR_VISIBLE);
	SetTopMargin(win,win->topMargin-GetTxtFmtBarHeight());
	if (ctrlHdl=FindControlByRefCon(win,kControlUserPaneProc))
		HideControl(ctrlHdl);
	if (ctrlHdl=FindControlByRefCon(win,kSeparatorRefCon))
		HideControl(ctrlHdl);
	ForceCompWindowRecalcAndRedraw(win);
}

/* MJN *//* new routine */
/*****************************************************************************
 * ShowTxtFmtBar - show the text formatting toolbar in the composition window
 *****************************************************************************/
void ShowTxtFmtBar(MyWindowPtr win)
{
	MessHandle messH;
	ControlHandle ctrlHdl;

	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	if (!TextFormattingBarAllowed(win)) return;
	if (TextFormattingBarVisible(win)) return;
	messH = (MessHandle)GetMyWindowPrivateData(win);
	SetMessOpt(messH,OPT_COMP_TOOLBAR_VISIBLE);
	SetTopMargin(win,win->topMargin+GetTxtFmtBarHeight());
	PlaceTxtFmtBarIcons(win,True);
	if (ctrlHdl=FindControlByRefCon(win,kControlUserPaneProc))
		ShowControl(ctrlHdl);
	if (ctrlHdl=FindControlByRefCon(win,kSeparatorRefCon))
		ShowControl(ctrlHdl);
	ForceCompWindowRecalcAndRedraw(win);
}

/************************************************************************
 * PlaceTxtFmtBarIcons - put the icons in the right place
 ************************************************************************/
void PlaceTxtFmtBarIcons(MyWindowPtr win,Boolean visible)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle ctrlHdl;
	Rect r;
	short i;
	Rect	rPort;
	
	// the embedding pane
	GetPortBounds(GetWindowPort(winWP),&rPort);
	tfbOverallRect.right = rPort.right - INSET;
	if (ctrlHdl=FindControlByRefCon(win,kControlUserPaneProc))
		MySetCntlRect(ctrlHdl,&tfbOverallRect);
	
	// the buttons
	for (i=NUM_TEXT_BAR_ICONS-1;i>=0;i--)
	{
		ctrlHdl = GetNextCompWindowTFBIcon(win,ctrlHdl,txtFmtBarIconCtrlTags[i]);
		CompTextFormattingBarIconRect(&r,0,i);
		MySetCntlRect(ctrlHdl,&r);
		SetControlVisibility(ctrlHdl,visible,false);
	}
	
	// the divider
	r = tfbOverallRect;
	r.bottom = r.top+3;
	r.left += INSET;
	if (ctrlHdl = FindControlByRefCon(win,kSeparatorRefCon))
	{
		MySetCntlRect(ctrlHdl,&r);
		SetControlVisibility(ctrlHdl,visible,false);
	}
	
	//the menus
	for (i=0;i<NUM_TEXT_BAR_POPUPS;i++)
	{
		ctrlHdl = FindControlByRefCon(win,FIRST_TEXT_POPUP+i);
		MySetCntlRect(ctrlHdl,&Rects[i]);
		SetControlVisibility(ctrlHdl,visible,false);
	}
}



/* MJN *//* new routine */
/*****************************************************************************
 * HideShowTxtFmtBar - set the visibility of the text formatting toolbar in
 * the composition window
 *****************************************************************************/
void HideShowTxtFmtBar(MyWindowPtr win,Boolean visible)
{
	if (visible) ShowTxtFmtBar(win);
	else HideTxtFmtBar(win);
}

/* MJN *//* new routine */
/*****************************************************************************
 * ToggleTxtFmtBarVisible - toggle the visibility of the text formatting
 * toolbar in the composition window
 *****************************************************************************/
void ToggleTxtFmtBarVisible(MyWindowPtr win)
{
	if (TextFormattingBarVisible(win)) HideTxtFmtBar(win);
	else ShowTxtFmtBar(win);
}

/* MJN *//* new routine */
/************************************************************************
 * HideShowAllTFB - calls HideShowTxtFmtBar for all composition windows
 * which are currently open
 ************************************************************************/
void HideShowAllTFB(Boolean visible)
{
	WindowPtr winWP;
	MyWindowPtr	win;

	winWP = FrontWindow_();
	while (winWP)
	{
		if (IsCompWindow(winWP)) /* ignore win->qWindow.visible */
		{
			win = GetWindowMyWindowPtr (winWP);
			HideShowTxtFmtBar(win, visible);
			CompSetFormatBarIcon(win,visible);
		}
		winWP = GetNextWindow(winWP);
	}
}

/* MJN *//* new routine */
/*****************************************************************************
 * EnableTxtFmtBar
 *****************************************************************************/
void EnableTxtFmtBar(MyWindowPtr win)
{
	MessHandle messH;
	ControlHandle ctrlHdl;
	Boolean html;

	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	if (!TextFormattingBarAllowed(win)) return;
	html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
	if (TextFormattingBarEnabled(win)) return;
	messH = (MessHandle)GetMyWindowPrivateData(win);
	(*messH)->textFormatBarEnabled = True;
	if (ctrlHdl=FindControlByRefCon(win,kControlUserPaneProc))
		ActivateControl(ctrlHdl);
	if (!html)
	{
		if (ctrlHdl=FindControlByRefCon(win,BULLET_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,LINK_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,RULE_BTN))
			DeactivateControl(ctrlHdl);
	}
	EnableItem(GetMHandle(FONT_HIER_MENU),0);
	
	// (jp) Stuff for Adware
	if (HasFeature (featureStyleColor))
		EnableItem(GetMHandle(COLOR_HIER_MENU),0);
	else
		if (ctrlHdl=FindControlByRefCon(win,COLOR_TEXT_POPUP))
			DeactivateControl(ctrlHdl);
		
	if (!HasFeature (featureStyleQuote)) {
		if (ctrlHdl=FindControlByRefCon(win,UNQUOTE_TEXT_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,QUOTE_TEXT_BTN))
			DeactivateControl(ctrlHdl);
	}
	if (!HasFeature (featureStyleJust)) {
		if (ctrlHdl=FindControlByRefCon(win,LEFT_JUST_TEXT_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,CENTER_JUST_TEXT_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,RIGHT_JUST_TEXT_BTN))
			DeactivateControl(ctrlHdl);
	}
	if (!HasFeature (featureStyleMargin)) {
		if (ctrlHdl=FindControlByRefCon(win,TEXT_INDENT_OUT_BTN))
			DeactivateControl(ctrlHdl);
		if (ctrlHdl=FindControlByRefCon(win,TEXT_INDENT_IN_BTN))
			DeactivateControl(ctrlHdl);
	}
	if (!HasFeature (featureStyleBullet))
		if (ctrlHdl=FindControlByRefCon(win,BULLET_BTN))
			DeactivateControl(ctrlHdl);
	if (!HasFeature (featureStyleHorzRule))
		if (ctrlHdl=FindControlByRefCon(win,RULE_BTN))
			DeactivateControl(ctrlHdl);
	if (!HasFeature (featureStyleLink))
		if (ctrlHdl=FindControlByRefCon(win,LINK_BTN))
			DeactivateControl(ctrlHdl);
}

/* MJN *//* new routine */
/*****************************************************************************
 * DisableTxtFmtBar
 *****************************************************************************/
void DisableTxtFmtBar(MyWindowPtr win)
{
	MessHandle messH;
	ControlHandle ctrlHdl;

	if (!txtFmtBarGlobalsInitted) InitTextFormattingBarGlobals();
	if (!TextFormattingBarAllowed(win)) return;
	if (!TextFormattingBarEnabled(win)) return;
	messH = (MessHandle)GetMyWindowPrivateData(win);
	(*messH)->textFormatBarEnabled = False;
	if (ctrlHdl=FindControlByRefCon(win,kControlUserPaneProc))
		DeactivateControl(ctrlHdl);
}

/* MJN *//* new routine */
/*****************************************************************************
 * SetTxtFmtBarEnabled
 *****************************************************************************/
void SetTxtFmtBarEnabled(MyWindowPtr win,Boolean enabled)
{
	if (enabled) EnableTxtFmtBar(win);
	else DisableTxtFmtBar(win);
}

/* MJN *//* new routine */
/*****************************************************************************
 * ToggleTxtFmtBarEnabled
 *****************************************************************************/
void ToggleTxtFmtBarEnabled(MyWindowPtr win)
{
	if (TextFormattingBarEnabled(win)) DisableTxtFmtBar(win);
	else EnableTxtFmtBar(win);
}

/* MJN *//* new routine */
/*****************************************************************************
 * EnableTxtFmtBarIfOK - sets the enabled state appropriately
 *****************************************************************************/
void EnableTxtFmtBarIfOK(MyWindowPtr win)
{
	SetTxtFmtBarEnabled(win,TextFormattingBarOK(win));
}

/* MJN *//* new routine */
/*****************************************************************************
 * RequestTFBEnableCheck - sets the global needTBFEnableCheck to true if the
 * window win is a composition window.  If needTBFEnableCheck gets set, then
 * the next time CompIdle gets called, we'll check to see if the formatting
 * toolbar needs to be enabled or disabled.  In effect, this is like having
 * a dirty bit for the formatting toolbar's enabled state. Currently, this
 * mechanism is only set up to deal with the front-most window, but in the
 * future, it might support marking specific windows as needing a check.
 *****************************************************************************/
void RequestTFBEnableCheck(MyWindowPtr win)
{
	if (!TextFormattingBarAllowed(win) || !TextFormattingBarVisible(win))
		return;
	needTBFEnableCheck = true;
	needTFBIdleImmediately = true;
}

/* MJN *//* new routine */
/************************************************************************
 * TFBRespondToSettingsChanges - to be called after any user settings
 * have been changed; this allows the formatting toolbar code to do
 * anything it needs to in response to a change in user settings that
 * affect the formatting toolbar.
 ************************************************************************/
void TFBRespondToSettingsChanges(void)
{
}

/* MJN *//* new routine */
/************************************************************************
 * IncreaseTextSizeOfCurSelection
 ************************************************************************/
void IncreaseTextSizeOfCurSelection(MyWindowPtr win, short amount);
void IncreaseTextSizeOfCurSelection(MyWindowPtr win, short amount)
{
	short curSizeItem;
	PETEHandle pte;
	PETEStyleEntry pse;
	PETETextStyle ps;
	long textValid;

	curSizeItem = CurSizePopupItem(win);
	if (curSizeItem>=tfbSizeBiggest) return;
	curSizeItem += amount;
	if (curSizeItem>tfbSizeBiggest) curSizeItem = tfbSizeBiggest;
	pte = win->pte;
	if (PeteIsValid(pte))
	{
		PeteSetDirty(pte);
		return;
	}
	textValid = 0;
	PeteStyleAt(pte,-1,&pse);
	ps = pse.psStyle.textStyle;
#ifdef USERELATIVESIZES
	ps.tsSize = kPETERelativeSizeBase + curSizeItem-tfbSizeNormal;
#else
	ps.tsSize = IncrementTextSize(FontSize,curSizeItem-tfbSizeNormal);
#endif
	textValid = peSizeValid;
	if (textValid) PETESetTextStyle(PETE,pte,-1,-1,&ps,textValid);
	PeteSetDirty(pte);
	return;
}

/* MJN *//* new routine */
void MatchTFBToCurSettings(MyWindowPtr win)
{
	ControlHandle ctrlHdl;

	if (!TextFormattingBarVisible(win)||!TextFormattingBarOK(win)) return;
	//if (CurFontPopupItem(win)!=lastDrawnFontPopupItem) DrawTFBFontPopup(win);
	//if (CurSizePopupItem(win)!=lastDrawnSizePopupItem) DrawTFBSizePopup(win);
	//if (CurColorPopupItem(win)!=lastDrawnColorPopupItem) DrawTFBColorPopup(win);
	if (!PrefIsSet(PREF_SEND_ENRICHED_NEW) && (ctrlHdl=FindControlByRefCon(win,LINK_BTN)))
		if (URLOkHere(win->pte) && HasFeature (featureStyleLink))
			ActivateControl(ctrlHdl);
		else
			DeactivateControl(ctrlHdl);
}

/* MJN *//* new routine */
void GetCurEditorMargins(PETEHandle pte, PSMPtr currentTxtMargin)
{
	GetCurEditorMarginsLo(pte, currentTxtMargin, -1L);
}

void GetCurEditorMarginsLo(PETEHandle pte, PSMPtr currentTxtMargin, long paraIndex)
{
	PETEParaInfo curTxtParaInfo;
	PETEParaInfo defaultParaInfo;
	long width;

	width=FontWidth*GetRLong(TAB_DISTANCE);
	curTxtParaInfo.tabHandle=nil;
	/* PETEGetParaInfo returns a result code of type ComponentResult.  The place that calls PETESetParaInfo
			ignores the result code, so I am assuming that it is "proper" to assume success. */
	PETEGetParaInfo(PETE,pte,paraIndex,&curTxtParaInfo);
	defaultParaInfo.tabHandle=nil;
	PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&defaultParaInfo);	
	/* Note: PeteConvert2Marg in peteglue.c doesn't seem to do the conversion correctly when first != second,
						so we'll just do it ourselves */
	if (curTxtParaInfo.indent>=0)
	{
		currentTxtMargin->second=FIX2HLFTAB(curTxtParaInfo.startMargin-defaultParaInfo.startMargin);
		currentTxtMargin->first=currentTxtMargin->second+FIX2HLFTAB(curTxtParaInfo.indent);
	}
	else
	{
		currentTxtMargin->first=FIX2HLFTAB(curTxtParaInfo.startMargin-defaultParaInfo.startMargin);
		currentTxtMargin->second=currentTxtMargin->first-FIX2HLFTAB(curTxtParaInfo.indent);
	}
	currentTxtMargin->right=FIX2HLFTAB(defaultParaInfo.endMargin-curTxtParaInfo.endMargin);
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part)
 * handle a click on a button in the text formatting toolbar.  win is the window,
 * buttonHandle is the ControlHandle to the button that was clicked, modifiers is the
 * modifiers field from the EventRecord of the click, part is the part code from FindControl,
 * and which is the ID of the control, pulled from the low byte of the control's refCon.
 * The function returns false if the button didn't belong to the text formatting
 * toolbar, in which case further processing should be done; it returns true
 * if it was a formatting toolbar button, in which case the appropriate processing
 * is done from within this routine.
 ************************************************************************/
Boolean TextFormattingBarButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part,short which)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	PETEHandle pte;
	PETETextStyle ps;
	PETEStyleEntry pse;
	PETEParaInfo pinfo;
	long pStart, pStop;
	Style txtAttrib;
	PeteSaneMargin txtMargin;
	Boolean changedTxtMargin;

	if ((which<FIRST_TEXT_BTN) || (which>LAST_TEXT_POPUP)) return false;
	if (!TextFormattingBarOK(win)) return true;
	pte = win->pte;
	if (!PeteIsValid(pte)) return true;	// bail if no pte

	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind (winWP), AUDITCONTROLID(GetWindowKind(winWP),which), mouseDown);

	PeteStyleAt(pte,-1,&pse);
	ps = pse.psStyle.textStyle;
	switch (which)
	{
		case FONT_TEXT_POPUP: DoMenu(winWP,(FONT_HIER_MENU<<16)|GetBevelMenuValue(buttonHandle),modifiers); return(True);
		case SIZE_TEXT_POPUP: DoMenu(winWP,(TEXT_SIZE_HIER_MENU<<16)|(GetBevelMenuValue(buttonHandle)),modifiers); return(True);
		case COLOR_TEXT_POPUP: DoMenu(winWP,(COLOR_HIER_MENU<<16)|GetBevelMenuValue(buttonHandle),modifiers); return(True);
		case EMOTICON_BTN: DoMenu(winWP,(EMOTICON_HIER_MENU<<16)|GetBevelMenuValue(buttonHandle),modifiers); return(True);
		case BULLET_BTN:	DoMenu(winWP,(MARGIN_HIER_MENU<<16)|CountMenuItems(GetMHandle(MARGIN_HIER_MENU)),modifiers); return(True);
		case RULE_BTN:	DoMenu2(winWP,TEXT_HIER_MENU,tmRule,modifiers); return(True);
		case LINK_BTN:	DoMenu2(winWP,TEXT_HIER_MENU,tmURL,modifiers); return(True);
		case CLEAR_TEXT_FORMATTING_BTN: DoMenu2(winWP,TEXT_HIER_MENU,tmPlain,modifiers|optionKey); return(True);
		case BOLD_TEXT_BTN:
		case ITALIC_TEXT_BTN:
		case UNDERLINE_TEXT_BTN:
			txtAttrib = 1<<(which-BOLD_TEXT_BTN);
			ps.tsFace ^= txtAttrib;
			PETESetTextStyle(PETE,pte,-1,-1,&ps,txtAttrib);
			break;
		case QUOTE_TEXT_BTN:	DoMenu2(winWP,TEXT_HIER_MENU,tmQuote,modifiers); return(True);
		case UNQUOTE_TEXT_BTN:	DoMenu2(winWP,TEXT_HIER_MENU,tmUnquote,modifiers); return(True);
		case LEFT_JUST_TEXT_BTN:
		case CENTER_JUST_TEXT_BTN:
		case RIGHT_JUST_TEXT_BTN:
 			UseFeature (featureStyleJust);
 			pStart = pStop = -1;	// use selection
			PeteParaRange(pte,&pStart,&pStop);
			PeteParaConvert(pte,pStart,pStop);
			switch (which)
			{
				case LEFT_JUST_TEXT_BTN:
					pinfo.justification = teFlushLeft;
					break;
				case CENTER_JUST_TEXT_BTN:
					pinfo.justification = teCenter;
					break;
				case RIGHT_JUST_TEXT_BTN:
					pinfo.justification = teFlushRight;
					break;
				default: /* unexpected */
					pinfo.justification = teFlushDefault;
					break;
			}
			PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);
			break;
		case TEXT_INDENT_IN_BTN:
		case TEXT_INDENT_OUT_BTN:
 			UseFeature (featureStyleMargin);
 			PetePrepareUndo(pte,peUndoStyleAndPara,-1,-1,nil,nil);
			pStart = pStop = -1;	// use selection
			PeteParaRange(pte,&pStart,&pStop);
			PeteParaConvert(pte,pStart,pStop);
			PeteGetTextAndSelection(pte, nil, &pStart, &pStop);
			pStart = PeteParaAt(pte, pStart);
			pStop = PeteParaAt(pte, pStop);
			while(pStart <= pStop) {
				GetCurEditorMarginsLo(pte, &txtMargin, pStart);
				changedTxtMargin=false;
				switch (which)
				{
					case TEXT_INDENT_IN_BTN:
						if (txtMargin.first>=4) break;
						txtMargin.first++;
						txtMargin.second=txtMargin.first;
						txtMargin.right=txtMargin.first;
						changedTxtMargin=true;
						break;
					case TEXT_INDENT_OUT_BTN:
						if (txtMargin.first<=0 || txtMargin.first==1 && PeteIsBullet(pte,pStart)) break;
						txtMargin.first--;
						txtMargin.second=txtMargin.first;
						txtMargin.right=txtMargin.first;
						changedTxtMargin=true;
						break;
				}
				if (changedTxtMargin)
				{
					PeteConvertMarg(pte,kPETEDefaultPara,&txtMargin,&pinfo);
					PETESetParaInfo(PETE,pte,pStart,&pinfo,peIndentValid | peStartMarginValid | peEndMarginValid);
				}
				++pStart;
			}
			PeteFinishUndo(pte,peUndoStyleAndPara,-1,-1);
			break;
		default:
			return false;
	}
	PeteSetDirty(pte);
	return true;
}

/* MJN *//* new routine */
/************************************************************************
 * TextFormattingBarIdle - periodically updates the text formatting bar
 * to reflect the current text selection
 ************************************************************************/
void TextFormattingBarIdle(MyWindowPtr win)
{
	unsigned long curTicks;

	if (!txtFmtBarGlobalsInitted) return;
	curTicks = TickCount();
	if (!needTFBIdleImmediately && (curTicks < (lastTFBIdle + TFB_IDLE_INTERVAL))) return;

	lastTFBIdle = curTicks;
	if (needTBFEnableCheck)
	{
		EnableTxtFmtBarIfOK(win);
		needTBFEnableCheck = false;
	}
	MatchTFBToCurSettings(win);
}

/* MJN *//* new routine */
/************************************************************************
 * CompTextFormattingBarIconRect - get the rect for a particular icon
 * in the text formatting toolbar
 ************************************************************************/
Rect *CompTextFormattingBarIconRect(Rect *r,MessHandle messH,short index)
{
	uLong priorIconSets;
	short n;

	n=index;
	priorIconSets=-1;
	while (n>=0)
		n-=txtFmtIconSetCounts[++priorIconSets];
	r->left = tfbColorPopupRect.right+INSET/2+(TXT_FMT_BAR_ICON_WIDTH)*index+(INSET/2)*priorIconSets;
	r->top = GetRLong(COMP_TOP_MARGIN) + ((tfbHeight - TXT_FMT_BAR_ICON_HEIGHT) / 2);
	r->bottom = r->top + TXT_FMT_BAR_ICON_HEIGHT;
	r->right = r->left + TXT_FMT_BAR_ICON_WIDTH;
	return r;
}

