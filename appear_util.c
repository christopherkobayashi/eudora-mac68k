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

#include "appear_util.h"
/* Appearance Manager Utilities */

#define FILE_NUM 99
#pragma segment AppearanceUtil

static void RefreshWinAppearance ( void );
static void RefreshMenuAppearance ( void );
static void ReplaceScrollbars ( void );
OSErr SetBevelIconLo(ControlHandle theControl, short id, OSType type, OSType creator, Handle suite, IconRef iconRef);
pascal Boolean StdAlertFilter(DialogPtr dgPtr,EventRecord *event,short *item);

/**********************************************************************
 * InitAppearanceMgr - register Appearance client if specified in preferences
 **********************************************************************/

void InitAppearanceMgr ( void )
{
	if (gUseAppearance = !PrefIsSet(PREF_NO_APPEARANCE))
		RegisterAppearanceClient();
	gUseLiveScroll = PrefIsSet(PREF_LIVE_SCROLL);
}


/**********************************************************************
 * RefreshAppearance - toggle Appearance on/off depending on preferences
 **********************************************************************/

void RefreshAppearance (void)
{
	if (gUseAppearance != !PrefIsSet(PREF_NO_APPEARANCE))
	{
		gUseAppearance = !PrefIsSet(PREF_NO_APPEARANCE);
		if (gUseAppearance)
			RegisterAppearanceClient();
		else 
			UnregisterAppearanceClient();
		if (gUseLiveScroll != PrefIsSet(PREF_LIVE_SCROLL))
			gUseLiveScroll = PrefIsSet(PREF_LIVE_SCROLL);
		RefreshMenuAppearance();
		RefreshWinAppearance();
		return;
	}
	if (gUseLiveScroll != PrefIsSet(PREF_LIVE_SCROLL))
	{
		gUseLiveScroll = PrefIsSet(PREF_LIVE_SCROLL);
		ReplaceScrollbars();
	}
	
}

/**********************************************************************
 * AppearanceVersion - return appearance number in BCD: 0x0101 = 1.0.1
 **********************************************************************/
short AppearanceVersion(void)
{
	static short	theVersion;
	SInt32			gestaltResult;
	
	if (!theVersion)
		//	This gestalt doesn't exist for version 1.0
		theVersion = Gestalt(gestaltAppearanceVersion,&gestaltResult) ? 0x0100 : gestaltResult;
	return theVersion;
}

/**********************************************************************
 * RefreshWinAppearance - refresh appearance on menus and all windows
 **********************************************************************/

void RefreshWinAppearance (void)
{
	SaveAll();
	RememberOpenWindows();
	CloseAll(true);
	RecallOpenWindows();
}


/**********************************************************************
 * RefreshWinMenuAppearance - refresh appearance on menus and all windows
 **********************************************************************/

void RefreshMenuAppearance (void)
{
	MenuEnum mInt;
	HierMenuEnum hmInt;
	MenuHandle menu;
	
	for (mInt=APPLE_MENU;mInt<MENU_LIMIT2;mInt++)
	{
		if (menu = GetMenuHandle(mInt))
			CalcMenuSize(menu);
	}

	for (hmInt=FIND_HIER_MENU;hmInt<HIER_MENU_LIMIT;hmInt++)
	{
		if (menu = GetMenuHandle(hmInt))
			CalcMenuSize(menu);
	}

	DrawMenuBar();	
}


/**********************************************************************
 * ReplaceScrollbars - replace live scrollbars w/ plain scrollbars or vice versa
 **********************************************************************/

void ReplaceScrollbars (void)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	ControlHandle oldScroll, newScroll;
	Rect controlRect;
	DECLARE_UPP(ScrollAction,ControlAction);
		
	INIT_UPP(ScrollAction,ControlAction);
	for (winWP = FrontWindow (); winWP; winWP = GetNextWindow (winWP))
		if (IsKnownWindowMyWindow(winWP) && GetWindowKind(winWP)==MBOX_WIN)
		{
			win = GetWindowMyWindowPtr (winWP);
			if(win->vBar)
			{
				oldScroll = win->vBar;
				controlRect = *GetControlBounds(oldScroll,&controlRect);
				newScroll = NewControl(GetControlOwner(oldScroll), &controlRect, nil, IsControlVisible(oldScroll), GetControlValue(oldScroll), GetControlMinimum(oldScroll), GetControlMaximum(oldScroll), gUseLiveScroll ? kControlScrollBarLiveProc : kControlScrollBarProc, GetControlReference(oldScroll));
				win->vBar = newScroll;
				SetControlAction(win->vBar,ScrollActionUPP);
				DisposeControl(oldScroll);
				Draw1Control(win->vBar);
			}
			if(win->hBar)
			{
				oldScroll = win->hBar;
				controlRect = *GetControlBounds(oldScroll,&controlRect);
				newScroll = NewControl(GetControlOwner(oldScroll), &controlRect, nil, IsControlVisible(oldScroll), GetControlValue(oldScroll), GetControlMinimum(oldScroll), GetControlMaximum(oldScroll), gUseLiveScroll ? kControlScrollBarLiveProc : kControlScrollBarProc, GetControlReference(oldScroll));
				win->hBar = newScroll;
				SetControlAction(win->hBar,ScrollActionUPP);
				DisposeControl(oldScroll);
				Draw1Control(win->hBar);
			}
		}
}


/************************************************************************
 * SetBevelIcon - set the icon for a bevel control
 ************************************************************************/
OSErr SetBevelIcon(ControlHandle theControl, short id, OSType type, OSType creator, Handle suite)
{
	IconRef ref = nil;
	IconFamilyHandle iconFamilyH = nil;
	
	if (id && (iconFamilyH=GetResource('icns',id)))
	{
		RegisterIconRefFromIconFamily('CSOm',('IC'<<16) | id,iconFamilyH,&ref);
		id = 0;
	}
	return SetBevelIconLo(theControl, id, type, creator, suite, ref);
}

/************************************************************************
 * SetBevelIconLo - set the icon for a bevel control
 ************************************************************************/
OSErr SetBevelIconLo(ControlHandle theControl, short id, OSType type, OSType creator, Handle suite, IconRef iconRef)
{
	long junk;
	ControlButtonContentInfo ci;
	OSErr err = GetControlData(theControl,0,kControlBevelButtonContentTag,sizeof(ci),(void*)&ci,&junk);

	if (SettingsRefN)
		UseResFile(SettingsRefN);
		
	if (!err)
	{
		// make sure it's not the same icon
		if (id && ci.contentType==kControlContentIconSuiteRes && ci.u.resID==id) return(noErr);
		if (suite && ci.contentType==kControlContentIconSuiteHandle && ci.u.iconSuite==suite) return(noErr);
		if (iconRef && ci.contentType==kControlContentIconRef && ci.u.iconRef==iconRef) return(noErr);
		
		// work to do
		KillBevelIcon(theControl);	// destroy the old icon
		
		// fill out info for new icon
 		Zero(ci);
 		if (id)
 		{
	 		ci.u.resID = id;
	 		ci.contentType = kControlContentIconSuiteRes;
	 	}
	 	else if (suite)
	 	{
	 		ci.u.iconSuite = suite;
	 		ci.contentType = kControlContentIconSuiteHandle;
	 	}
	 	else if (type)
	 	{
	 		DupDeskIconSuite(type,creator,&ci.u.iconSuite);
	 		ci.contentType = kControlContentIconSuiteHandle;
	 	}
	 	else if (iconRef)
	 	{
	 		ci.u.iconRef = iconRef;
	 		ci.contentType = kControlContentIconRef;
	 	}
	 	
	 	// and set it
 		err = SetControlData(theControl,0,kControlBevelButtonContentTag,sizeof(ci),(void*)&ci);
 		Draw1Control(theControl);
 	}
 	return(err);
}

/************************************************************************
 * SetBevelIconIconRef - set the icon for a bevel control using IconRef
 ************************************************************************/
OSErr SetBevelIconIconRef(ControlHandle theControl, IconRef iconRef)
{
	return SetBevelIconLo(theControl,0,nil,nil,nil,iconRef);
}

/************************************************************************
 * KillBevelIcon - remove an icon from a bevel control
 ************************************************************************/
OSErr KillBevelIcon(ControlHandle theControl)
{
	long junk;
	ControlButtonContentInfo ci;
	OSErr err = GetControlData(theControl,0,kControlBevelButtonContentTag,sizeof(ci),(void*)&ci,&junk);
	
	if (!err)
	{
		// Dispose of suite if need be
		if (ci.contentType==kControlContentIconSuiteHandle && ci.u.iconSuite)
		{
			if ((*(ICacheHandle)ci.u.iconSuite)->magic!='iCaC')
				DisposeIconSuite(ci.u.iconSuite,True);
		}
		else if (ci.contentType==kControlContentIconRef && ci.u.iconRef)
			ReleaseIconRef(ci.u.iconRef);

		// icon id of zero to clear
		Zero(ci);
 		ci.contentType = kControlContentIconSuiteRes;
 		err = SetControlData(theControl,0,kControlBevelButtonContentTag,sizeof(ci),(void*)&ci);
 	}
 	return(err);
}

/************************************************************************
 * SetBevelColor - set the fg color for a bevel button
 ************************************************************************/
OSErr SetBevelColor(ControlHandle theControl, RGBColor *color)
{	
	ControlFontStyleRec fs;
	long junk;
	OSErr err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);
	
	if (err) return(err);
	if (SAME_COLOR(fs.foreColor,*color)) return(noErr);
	fs.foreColor = *color;
	fs.flags |= kControlUseForeColorMask;
	return (SetControlFontStyle(theControl,&fs));
}

/************************************************************************
 * SetBevelMode - set the text mode for a button
 ************************************************************************/
OSErr SetBevelMode(ControlHandle theControl, short mode)
{	
	ControlFontStyleRec fs;
	long junk;
	OSErr err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);

	fs.mode = mode;
	fs.flags |= kControlUseModeMask;
	return (SetControlFontStyle(theControl,&fs));
}

/************************************************************************
 * SetBevelStyle - set the text style for a bevel button
 ************************************************************************/
OSErr SetBevelStyle(ControlHandle theControl, Style style)
{	
	ControlFontStyleRec fs;
	long junk;
	OSErr err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);

	if (!err && fs.style != style)
	{
		fs.style = style;
		fs.flags |= kControlUseFaceMask;
		err = SetControlFontStyle(theControl,&fs);
		Draw1Control(theControl);
	}
	return(err);
}

/************************************************************************
 * GetBevelStyle - get the text style from a bevel button
 ************************************************************************/
OSErr GetBevelStyle(ControlHandle theControl, Style *style)
{	
	ControlFontStyleRec fs;
	long junk;
	OSErr err;
	
	Zero(fs);
	err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);
	
	if (!err) *style = fs.style;
	else *style = 0;
	return(err);
}


/************************************************************************
 * SetBevelJust - set the text justification for a bevel button
 ************************************************************************/
OSErr SetBevelJust(ControlHandle theControl, SInt16 just)
{	
	ControlFontStyleRec fs;
	long junk;
	OSErr err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);

	if (!err && fs.just != just)
	{
		fs.just = just;
		fs.flags |= kControlUseJustMask;
		err = SetControlFontStyle(theControl,&fs);
		Draw1Control(theControl);
	}
	return(err);
}

/************************************************************************
 * SetBevelFontSize - set font & size for a bevel button
 ************************************************************************/
OSErr SetBevelFontSize(ControlHandle theControl,short font,short size)
{
	ControlFontStyleRec fs;
	long junk;
	OSErr err = GetControlData(theControl,0,kControlFontStyleTag,sizeof(fs),(void*)&fs,&junk);
	
	fs.font = font;
	fs.size = size;
	fs.flags |= kControlUseFontMask | kControlUseSizeMask;
	return (SetControlFontStyle(theControl,&fs));
}

/************************************************************************
 * SetBevelMenu - install a menu
 ************************************************************************/
OSErr SetBevelMenu(ControlHandle theControl, short menuID, MenuHandle mHandle)
{
	if (!mHandle) mHandle = GetMHandle(menuID);
	if (!mHandle) return(fnfErr);
	return(SetControlData(theControl,0,kControlBevelButtonMenuHandleTag,sizeof(mHandle),(void*)&mHandle));
}

/************************************************************************
 * SetBevelMenuValue - set the value of a bevel menu
 ************************************************************************/
OSErr SetBevelMenuValue(ControlHandle theControl,short value)
{
	return(SetControlData(theControl,0,kControlBevelButtonMenuValueTag,sizeof(value),(void*)&value));
}

/************************************************************************
 * SetBevelTextOffset - set the text offset of a bevel menu
 ************************************************************************/
OSErr SetBevelTextOffset(ControlHandle theControl,short h)
{
	return(SetControlData(theControl,0,kControlBevelButtonTextOffsetTag,sizeof(h),(void*)&h));
}

/************************************************************************
 * SetBevelGraphicOffset - set the text offset of a bevel menu
 ************************************************************************/
OSErr SetBevelGraphicOffset(ControlHandle theControl,short h)
{
	Point pt;
	pt.h = h;
	pt.v = 0;
	return(SetControlData(theControl,0,kControlBevelButtonGraphicOffsetTag,sizeof(pt),(void*)&pt));
}

/************************************************************************
 * SetBevelTextAlign - set the text alignment of a bevel menu
 ************************************************************************/
OSErr SetBevelTextAlign(ControlHandle theControl,short value)
{
	return(SetControlData(theControl,0,kControlBevelButtonTextAlignTag,sizeof(value),(void*)&value));
}

/************************************************************************
 * SetBevelGraphicAlign - set the graphic alignment of a bevel menu
 ************************************************************************/
OSErr SetBevelGraphicAlign(ControlHandle theControl,short value)
{
	return(SetControlData(theControl,0,kControlBevelButtonGraphicAlignTag,sizeof(value),(void*)&value));
}

/************************************************************************
 * SetBevelTextPlace - set the text placement of a bevel menu
 ************************************************************************/
OSErr SetBevelTextPlace(ControlHandle theControl,short value)
{
	return(SetControlData(theControl,0,kControlBevelButtonTextPlaceTag,sizeof(value),(void*)&value));
}

/************************************************************************
 * GetBevelMenu - get the menu handle of a bevel menu
 ************************************************************************/
MenuHandle GetBevelMenu(ControlHandle theControl)
{
	long junk;
	MenuHandle mHandle;
	if (GetControlData(theControl,0,kControlBevelButtonMenuHandleTag,sizeof(mHandle),(void*)&mHandle,&junk)) return(nil);
	else return(mHandle);
}

/************************************************************************
 * GetBevelMenuValue - get the value of a bevel menu
 ************************************************************************/
short GetBevelMenuValue(ControlHandle theControl)
{
	long junk;
	short value;
	if (GetControlData(theControl,0,kControlBevelButtonMenuValueTag,sizeof(value),(void*)&value,&junk)) return(0);
	else return(value);
}

/************************************************************************
 * SetControlText - set the text of a control, if it has changed
 ************************************************************************/
OSErr SetTextControlText(ControlHandle theControl,PStr textStr,UHandle textHandle)
{
	Str255 oldText;
	Str255 newText;
	long oldCount;
	long newCount;
	UPtr textPtr;
	OSErr err = GetControlData(theControl,0,kControlStaticTextTextTag,sizeof(oldText)-2,oldText+1,&oldCount);
	
	if (!err)
	{
		*oldText = oldCount;
		if (textStr)
			PCopy(newText,textStr);
		else
		{
			newCount = GetHandleSize(textHandle);
			MakePStr(newText,*textHandle,newCount);
		}
		
		if (EqualString(newText,oldText,True,True)) return(noErr);	// no difference
		
		if (textStr)
		{
			textPtr = textStr+1;
			newCount = *textStr;
		}
		else
			textPtr = LDRef(textHandle);

		err = SetControlData(theControl,0,kControlStaticTextTextTag,newCount,textPtr);
		Draw1Control(theControl);
		
		if (textHandle) LDRef(textHandle);
	}
	return(err);
}


/************************************************************************
 * ControlIsUgly
 ************************************************************************/
Boolean ControlIsUgly(ControlHandle cntl)
{
	return (cntl && kControlSupportsNewMessages==SendControlMessage(cntl,kControlMsgTestNewMsgSupport,nil));
}

/************************************************************************
 * GetNewControlSmall - get a control from a resource, then make it small
 ************************************************************************/
ControlHandle GetNewControlSmall(short id,WindowPtr win)
{
	ControlHandle cntl = GetNewControl(id,win);
	LetsGetSmall(cntl);
	EmbedInWazoo(cntl,win);
	return(cntl);
}

/************************************************************************
 * NewControlSmall - make a control, then make it small
 ************************************************************************/
ControlHandle NewControlSmall(WindowPtr theWindow, Rect *boundsRect, PStr title, Boolean visible, short value, short min, short max, short procID, long refCon)
{
	ControlHandle cntl = NewControl(theWindow, boundsRect, title, visible,  value,  min,  max,  procID,  refCon);
	LetsGetSmall(cntl);
	return(cntl);
}

/************************************************************************
 * LetsGetSmall - make a control small
 ************************************************************************/
void LetsGetSmall(ControlHandle cntl)
{
	if (cntl) SetBevelFontSize(cntl,kControlFontSmallSystemFont,0);
}

/**********************************************************************
 * CreateControl - create a control for the find window
 **********************************************************************/
ControlHandle CreateControl(MyWindowPtr win,ControlHandle embed,short strID,short procID,Boolean fit)
{
	ControlHandle cntl;
	Rect r;
	Str255 s;
	
	SetRect(&r,-50,-50,-20,-20);
	cntl = NewControlSmall(GetMyWindowWindowPtr(win),&r,GetRString(s,strID),
											True,0,0,1,procID,0);	
	if (embed)
		EmbedControl(cntl,embed);
	if (fit) ButtonFit(cntl);
	return cntl;
}

/**********************************************************************
 * CreateMenuControl - create a menu control
 **********************************************************************/
ControlHandle CreateMenuControl(MyWindowPtr win,ControlHandle embed,PStr title,short menuID,short variant,short value,Boolean autoCalcTitle)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl;
	Rect r;
	ControlCalcSizeRec	calcSize;
	MenuHandle	mh;

	if (!menuID) return NULL;
	
	SetRect(&r,-4000,-4000,-4000+20,-4000+20);
	cntl = NewControl(winWP,&r,title,true,0,menuID,
		autoCalcTitle ? -1 : 0, kControlPopupButtonProc + variant,0);
	LetsGetSmall(cntl);
	
	SetControlValue(cntl,value);

	// set optimal size
	SendControlMessage(cntl,kControlMsgCalcBestRect,(void*)&calcSize);
	SizeControl(cntl,calcSize.width,calcSize.height);	
	
	// make sure it's enabled
	if (mh = GetPopupMenuHandle(cntl))
		EnableItem(mh,0);

	if (embed)
		EmbedControl(cntl,embed);
	return cntl;
}

/**********************************************************************
 * GetPopupMenuHandle - get menu handle for a popup control
 **********************************************************************/
MenuHandle GetPopupMenuHandle(ControlHandle cntl)
{
	Size	junk;
	MenuHandle	mh = nil;

	GetControlData(cntl,0,kControlPopupButtonMenuHandleTag,sizeof(mh),(void*)&mh,&junk);
	return mh;
}




/************************************************************************
 * StdAlertFilter - filter for standard alerts
 ************************************************************************/
pascal Boolean StdAlertFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	Boolean oldCmdPeriod=CommandPeriod;
	Boolean retVal = false;
	
#ifdef THREADING_ON	
	if (NEED_YIELD)
		MyYieldToAnyThread();
#endif

#ifdef CTB
	if (CnH) CMIdle(CnH);
#endif

//	Handle command-period first
	if (MiniMainLoop(event) || HasCommandPeriod())
	{
		*item = kAlertStdAlertCancelButton;
		MyStdFilterProc(dgPtr,event,item);
		return(True);
	}
	
	if (event->what != nullEvent)
		retVal = MyStdFilterProc(dgPtr,event,item); 
	return retVal;
}

/************************************************************************
 * ComposeStdAlert - Compose an alert using a string and StdAlert
 *	Format of string is:
 *	error str�explanation�otherbutton�cancelbutton�okbutton
 *	The default button is set by adding a bullet to the end of its text
 *	The cancel button is set by adding a dash to the end of its text
 *  The error and explanation will be sent through VAComposeStringDouble with the
 *  remaining arguments.
 *
 *  If the entire format string is preceded by a � (option-s) the alert will be
 *	spoken instead of displayed in a dialog with the default item automatically
 *	returned.  (PREF_NO_SPOKEN_WARNINGS)
 *
 *	If either of the error or explanation strings is preceded by a � (option-l)
 *	that string will not be spoken.
 *
 *	However, a � (option-w) appearing within a string designates the following
 *	text to be spoken without also being displayed within the alert.
 *
 *	An example:
 *
 *		The pie went bad��Because you left the fridge open�You moron��Ick!�
 *
 *	Will display:    The pie went bad
 *									 Because you left the fridge open     [[ Ick! ]]
 *
 *	And will speak:	 The pie went bad you moron
 *
 *	IMPORTANT:  See the code for ReallyStandardAlert and TalkingAlert to
 *							see how/where these strings are parsed for speaking meta
 *							characters.
 *
 ************************************************************************/
short ComposeStdAlert(AlertType type,short template,...)
{
	Str255 error;
	Str255 explanation;
	Str255 fmtdError;
	Str255 fmtdExplanation;
	Str31 buttons[3];
	AlertStdAlertParamRec param;
	Byte integral[2];
	UPtr spot;
	short i;
	va_list args;
	short defWhich = kAlertStdAlertOKButton;
	short canWhich = 0;
#ifdef THREADING_ON
	Boolean inAThread = InAThread();
	Boolean silent = false;
	DECLARE_UPP(StdAlertFilter,ModalFilter);
	
	INIT_UPP(StdAlertFilter,ModalFilter);
	if (silent = (template < 0))	template *= -1;
	
	/*
		Note:
		STR# entries for ALERTS with task progress translations belong in strndefs with ALRTStringsStrn.
		There should be two per alert:
		1) ALERT dialog format string
		2) Task Progress format string 				
	*/
	ASSERT(inAThread ? (template > ALRTStringsStrn && template < ALRTStringsStrn+LIMIT_ASTR) : 1);
	template+=inAThread;
#endif
	
	integral[0] = '�';
	integral[1] = 0;
	
	// suck out the strings
	GetRString(fmtdError,template);
	spot = fmtdError+1;
	PToken(fmtdError,error,&spot,integral);
	PToken(fmtdError,explanation,&spot,integral);
	
#ifdef THREADING_ON
	// suck out default button
	if (inAThread)
	{
		buttons[0][0]=0;
		PToken(fmtdError,buttons[0],&spot,integral);
		if (buttons[0][0]==1)
			defWhich = buttons[0][1]-'0';
	}
	else
#endif
	// suck out the button texts
	for (i=0;i<3;i++)
	{
		PToken(fmtdError,buttons[i],&spot,integral);
		if (buttons[i][*buttons[i]] == bulletChar)
		{
			defWhich = 3-i;
			--*buttons[i];
		}
		if (buttons[i][*buttons[i]] == '-')
		{
			canWhich = 3-i;
			--*buttons[i];
		}
	}
	
	// compose the explanation
	va_start(args,template);
	(void) VaComposeStringDouble(fmtdError,sizeof(fmtdError)-1,error,args,fmtdExplanation,sizeof(fmtdExplanation)-1,explanation);
	va_end(args);

#ifdef THREADING_ON
	// if in a thread, add the error to the tp window
	if (inAThread)
	{
		if (fmtdError[0] || fmtdExplanation[0])
		 	AddTaskErrorsS(fmtdError,fmtdExplanation,GetCurrentTaskKind(),(*CurPersSafe)->persId);
	 	return(defWhich);
 	}
#endif

	// fill in the param block
	param.movable = True;
	param.helpButton = False;
	param.filterProc = StdAlertFilterUPP;
	param.defaultText = *buttons[2] ? buttons[2]:nil;
	param.cancelText = *buttons[1] ? buttons[1]:nil;
	param.otherText = *buttons[0] ? buttons[0]:nil;
	param.defaultButton = defWhich;
	param.cancelButton = canWhich;
	param.position = kWindowDefaultPosition;

	// Beep if we're not doing a spoken warning
	if (!silent && !(*fmtdError && fmtdError[1] == 0xA7 && !PrefIsSet (PREF_NO_SPOKEN_WARNINGS)))
		SysBeep (1);

	// do it
	return (ReallyStandardAlert(type,fmtdError,fmtdExplanation,&param));
}

//
//	The refcon is overloaded to contain an identifier and part ID.  Ick.
//

ControlHandle CreateInsideOutBevelIconButtonUserPane (WindowPtr theWindow, SInt16 iconID, SInt16 textID, Rect *bounds, SInt16 iconSize, SInt16 maxTextWidth, UInt16 buttonID)

{
	ControlHandle	userPaneControl,
								buttonControl,
								textControl;
	Str255				text; 
	Rect					buttonRect,
								textRect;
	OSErr					theError;
	SInt16				center;

	// Figure out where the bevel button should be placed (it's centered and anchored to the top of the user pane)
	center = (bounds->right - bounds->left) >> 1;
	buttonRect.left		= bounds->left + center - (iconSize >> 1) - kInsideOutBevelButtonMargin;
	buttonRect.top		= bounds->top;
	buttonRect.right	= buttonRect.left + iconSize + (kInsideOutBevelButtonMargin << 1);
	buttonRect.bottom	= buttonRect.top + iconSize + (kInsideOutBevelButtonMargin << 1);

	// Figure out where we'll put the text
	textRect.top = buttonRect.bottom + kInsideOutBevelButtonTextMargin;
	textRect.bottom = bounds->bottom;
	if (maxTextWidth>0) {
		textRect.left  = bounds->left + center - (maxTextWidth >> 1);
		textRect.right = textRect.left + maxTextWidth;
	}
	else {
		textRect.left  = bounds->left;
		textRect.right = bounds->right;
		if (maxTextWidth<0) InsetRect(&textRect,-maxTextWidth/2,0);
	}

	// Create away!
	if (userPaneControl = NewControl (theWindow, bounds, "\p", true, kControlSupportsEmbedding, 0, 0, kControlUserPaneProc, buttonID)) {
		if (buttonControl = NewControl (theWindow, &buttonRect, "\p", true, 0, ((kControlBehaviorPushbutton | kControlBehaviorToggles) << 8) | kControlContentIconSuiteRes, iconID, kControlBevelButtonNormalBevelProc, (kControlButtonPart << 16) | buttonID))
			if (textControl = NewControlSmall (theWindow, &textRect, "\p", true, 0, 0, 0, kControlStaticTextProc, (kControlLabelPart << 16) | buttonID)) {
				theError = EmbedControl (buttonControl, userPaneControl);
				if (!theError)
					theError = EmbedControl (textControl, userPaneControl);
				if (!theError)
					theError = SetTextControlText (textControl, GetRString (text, textID), nil);
				if (!theError)
					theError = SetBevelJust (textControl, teCenter);
				if (!theError)
					return (userPaneControl);
			}
		DisposeControl (userPaneControl);
		userPaneControl = nil;
	}
	return (userPaneControl);
}

//
//	GetInsideOutBevelIconButtonControl
//
//		Returns the control handle of the button part of a InsideOutBevelIconButton
//

ControlHandle FindInsideOutBevelIconButtonControl (ControlHandle parentCntl, ControlPartCode part)

{
	ControlHandle	buttonControl;
	UInt16				count;
	
	if (!CountSubControls (parentCntl, &count))
	for (; count; count--)
		if (!GetIndexedSubControl (parentCntl, count, &buttonControl))
			if ((GetControlReference (buttonControl) >> 16) == part)
				return (buttonControl);
	return (nil);
}