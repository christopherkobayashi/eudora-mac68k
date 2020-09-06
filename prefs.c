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

#include <Keychain.h>
#include <InternetConfig.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

#include "prefs.h"
#include "myssl.h"	// 	for "esslUseAltPort"

#define FILE_NUM 31
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#include <LaunchServices.h>
#include <CFPreferences.h>

#pragma segment SetPrefs

void NewCreator(short item);
void FontHasChanged(void);
int BadPrefItem(DialogPtr dialog);
void GetPrefParms(short pref, short *id, short *num);
OSErr SetPrefWithError(short pref, PStr value);
OSErr CheckPrefSpecial(short pref, PStr value);
void GetIndHString(PStr string,short id,short index);
void PrefHelpAlert(short pref);
void SettingsHelp(MyWindowPtr dlogWin);
OSErr SetStrnFromDlog(MyWindowPtr dlog,short item,short resId);
OSErr FillWithStrn(MyWindowPtr dlogWin,short item,short resId);
void SetPOPAccount(PStr user,PStr host);
OSErr SavePersItem(short which,short item,Handle itemH);
OSErr BuildPersItem(short which,short item,Handle itemH,Boolean *enabled);
OSErr HitPersItem(short which,short item);
#ifdef SETTINGSLISTKEY
Boolean SettingsKey (MyWindowPtr dlogWin, EventRecord *event);	
void SettingsActivate(MyWindowPtr win);
void SettingsTEActivate(MyWindowPtr dlogWin,Boolean activate);
Boolean SettingsListKey (MyWindowPtr win, EventRecord *event);	
pascal short SettingsSearchProc (Ptr cellDataPtr, Ptr searchDataPtr,short cellDataLen, short searchDataLen);
#endif SETTINGSLISTKEY
OSErr HesiodPOPInfo(PStr user,PStr host);
void ResetSettings(void);
OSErr SaveSacredSettings(AccuPtr a);
OSErr RestoreSacredSettings(AccuPtr a);
#define NEED_TO_SAVE	(Dlog->isDirty || (Pers && Pers!=PersList))
#define PrefIsOverride(pref)	(pref>399)
short PrefNormalize(short pref);
// Here's a sordid tale.  When building the settings dialogs,
// we have to set any pref we get, just to make sure that
// settings that appear in dialogs are reflected in the settings
// file and not just inherited from dominant, which makes
// them tricky to change.  Sigh.
PStr GetSetPref(PStr val,short pref);
long GetSetPrefLong(short pref);
Boolean PrefIsSetSet(short pref);
extern PStr GetResNameAndStripFeature(PStr into,ResType type,short id,Boolean *hasFeature);
pascal void NullUserDraw(DialogPtr dlog, short item);
long GetSubmissionPort(void);
int ShowSimpleModalDialog ( int resID );

OSErr SetDefaultMailClient ( ICInstance icp, OSType creator, ConstStr255Param theName );
OSErr GetDefaultMailClient ( ICInstance icp, OSType *creator, StringPtr theName );
Boolean   IsEudoraTheDefaultMailClient ( ICInstance icp );
OSErr  SetEudoraAsTheDefaultMailClient ( ICInstance icp );

/**********************************************************************
 * PrefNormalize - take a pref number with resource specified (incorrectly)
 * and map into normal pref range
 **********************************************************************/
short PrefNormalize(short pref)
{
	if (PREF_STRN<pref && pref<PREF_STRN+100) pref -= PREF_STRN;
	else if (PREF2_STRN<pref && pref<PREF2_STRN+100) pref -= PREF2_STRN-100;
	else if (PREF3_STRN<pref && pref<PREF3_STRN+100) pref -= PREF3_STRN-200;
	else if (PREF4_STRN<pref && pref<PREF4_STRN+100) pref -= PREF4_STRN-300;
	return pref;
}

/**********************************************************************
 * SetPrefWithError - set a preference, letting user know if there is an error
 **********************************************************************/
OSErr SetPrefWithError(short pref, PStr value)
{
	long min,max;
	long iVal=-1;
	UPtr spot;
	OSErr err = noErr;

	pref = PrefNormalize(pref);
		
	if (PrefBounds(pref,&min,&max) && *value && !(*value==1 && value[1]=='0'))
	{
		for (spot=value+1;spot<=value+*value;spot++)
		  if (!PIndex("\p-0123456789",*spot)) {err=1; break;}
		if (!err)
		{
			StringToNum(value,&iVal);
			err = iVal<min || iVal>max;
		}
	}
	
	if (!err) err = CheckPrefSpecial(pref,value);
	
	if (pref==PREF_INTERVAL && iVal==0) err = 0;

	if (err) {if (err!=userCanceledErr) PrefHelpAlert(pref);}
	else SetPref(pref,value);
	
	return(err);
}

/**********************************************************************
 * Mom - nag somebody..  Returns true if the user wishes to continue
 **********************************************************************/
Boolean Mom(short yesId,short noId,short prefId,short fmt,...)
{
	MyWindowPtr	dlogWin = GetNewMyDialog(MOM_ALRT,nil,nil,InFront);
	DialogPtr dlog = GetMyWindowDialogPtr(dlogWin);
	Str255 scratch;
	Str255 fmtStr;
	short res;
	extern ModalFilterUPP DlgFilterUPP;
	va_list args;
	va_start(args,fmt);
	(void) VaComposeString(scratch,GetRString(fmtStr,fmt),args);
	va_end(args);
	
	if (!dlog) return(False);

	/*
	 * set the appropriate strings
	 */
	ParamText(scratch,"","","");
	if (yesId)
	{
		SetDICTitle(dlog,1,GetRString(scratch,yesId));
		SetDICTitle(dlog,2,ComposeRString(scratch,DONT,yesId));
		SetDICTitle(dlog,3,ComposeRString(scratch,AND_DONT_WARN,yesId));
	}
		
	if (noId) SetDICTitle(dlog,2,GetRString(scratch,noId));
	
	SetDialogDefaultItem(dlog,1);	// button 1 is default

	/*
	 * put up the alert
	 */
	StartMovableModal(dlog);
	AutoSizeDialog(dlog);
	ShowWindow(GetDialogWindow(dlog));

	// Under Mac OS X the caution, note and warning icons are replaced by the application
	// icon, so we should hide the application icon we've for years placed in the lower left corner.
	if (HaveOSX ())
		HideDialogItem (dlog, 5);
		
	SysBeep(20L);
	MovableModalDialog(dlog,DlgFilterUPP,&res);
	
	/*
	 * close
	 */
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
	
	/*
	 * act
	 */
	if (res==3 && prefId)
	{
		TogglePref(prefId);
		res = 1;
	}
	
	return(res==1);
}

//Wife is only needed in Light at this point.
/**********************************************************************
 * Wife - like Mom, except she only gives you one choice.
 **********************************************************************/
Boolean Wife(short buttonID, short prefId, short fmt, ...)
{
	GrafPtr oldPort;
	MyWindowPtr	dlogWin;
	DialogPtr dlog;
	WindowPtr	dlogWP;
	Str255 scratch;
	Str255 fmtStr;
	short res;
	extern ModalFilterUPP DlgFilterUPP;
	va_list args;
	va_start(args,fmt);
	(void) VaComposeString(scratch,GetRString(fmtStr,fmt),args);
	va_end(args);
	
	GetPort (&oldPort);
	dlogWin = GetNewMyDialog(WIFE_ALRT,nil,nil,InFront);
	dlog = GetMyWindowDialogPtr(dlogWin);
	dlogWP = GetDialogWindow(dlog);
	if (!dlog) return(False);
	SetPort (GetWindowPort(dlogWP));

	/*
	 * set the appropriate strings
	 */
	ParamText(scratch,"","","");
	if (buttonID)
	{
		SetDICTitle(dlog,1,GetRString(scratch,buttonID));
		SetDICTitle(dlog,2,ComposeRString(scratch,AND_DONT_WARN,buttonID));
	}

	SetDialogDefaultItem(dlog,1);	// button 1 is default
	
	/*
	 * put up the alert
	 */
	StartMovableModal(dlog);
	ShowWindow(dlogWP);

	// Under Mac OS X the caution, note and warning icons are replaced by the application
	// icon, so we should hide the application icon we've for years placed in the lower left corner.
	if (HaveOSX ())
		HideDialogItem (dlog, 4);
		
	SysBeep(20L);
	MovableModalDialog(dlog,DlgFilterUPP,&res);
	
	/*
	 * close
	 */
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
	
	/*
	 * act
	 */
	if (res==2 && prefId)
	{
		TogglePref(prefId);
		res = 1;
	}
	SetPort (oldPort);
	return(res==1);
}


#ifdef TWO
/**********************************************************************
 * SetStrnFromDlog - suck values back out of a dialog and into an STRN resource
 **********************************************************************/
OSErr SetStrnFromDlog(MyWindowPtr dlog,short item,short resId)
{
	Handle text;
	short type;
	Rect box;
	UPtr spot, end, newline;
	Str255 string;
	Handle dataH = NuHandle(sizeof(short));
	short count = 0;
	OSErr err = noErr;
	
	GetDialogItem(GetMyWindowDialogPtr(dlog),item,&type,&text,&box);
	
	if (!text || !dataH) return(MemError());
	
	spot = LDRef(text);
	end = spot + GetHandleSize(text);
	
	/*
	 * build a handle to the data
	 */
	for (;spot<end;spot=newline+1)
	{
		count++;
		for (newline=spot;newline<end && *newline!='\015';newline++);
		MakePStr(string,spot,newline-spot);
		CaptureHex(string,string);
		if (err=PtrPlusHand_(string,dataH,*string+1)) goto done;
	}
	*(short*)(*dataH) = count;
	
	/*
	 * is the resource in the current settings file?
	 */
	ZapSettingsResource('STR#',resId);
	
	/*
	 * add it
	 */
	AddMyResource_(dataH,'STR#',resId,"");
	MyUpdateResFile(SettingsRefN);
	
	ZapHandle(StringCache);
	
done:
	if (err) ZapHandle(dataH);
	if (text) UL(text);		
	return(err);
}

/**********************************************************************
 * FillWithStrn - fill a dialog item with an STRN resource
 **********************************************************************/
OSErr FillWithStrn(MyWindowPtr dlogWin,short item,short resId)
{
	DialogPtr	theDialog = GetMyWindowDialogPtr (dlogWin);
	short count;
	UHandle text;
	Str255 string;
	short i;
	OSErr err = noErr;
	short type;
	Rect box;
	
	GetDialogItem(theDialog,item,&type,&text,&box);
	if (!text) return(MemError());
	SetHandleBig(text,0);
	
	count = CountStrn(resId);
	for (i=1;i<=count;i++)
	{
		GetRString(string,resId+i);
		EscapeInHex(string,string);
		PCatC(string,'\015');
		if (err=PtrPlusHand_(string+1,text,*string))
			break;
	}
	
	if (!err) SetDialogItem(theDialog,item,type,text,&box);
	else ZapHandle(text);
	return(err);
}

#endif

/**********************************************************************
 * PrefHelpString - get the help string for a preference
 **********************************************************************/
PStr PrefHelpString(short pref, PStr string,Boolean geeky)
{
	Str255 scratch;
	Str127 range;
	short id;
	short num;
	long min, max;
	UHandle geekH = nil;
	
	if (pref<400)
	{
		GetPrefParms(pref,&id,&num);
		GetIndHString(scratch,id,num);
		if (geeky) geekH = GetResource('GSTR',id+num);
	}
	else
	{
		GetIndHString(scratch,100*(pref/100),pref%100);
		if (geeky) geekH = GetResource('GSTR',pref);
	}
	
	if (PrefBounds(pref,&min,&max))
	{
		ComposeRString(range,PREFLIMIT_EXPLAIN,min,max);
		PSCat(scratch,range);
	}
	
	if (geekH)
	{
		PSCat(scratch,*geekH);
		HPurge(geekH);
	}
	
	PCopy(string,scratch);
	return(string);
}

/**********************************************************************
 * GetIndHString - get a help string
 **********************************************************************/
void GetIndHString(PStr string,short id,short index)
{
	UHandle resH;
	UPtr spot;
	short count;
	
	*string = 0;
	if (resH=GetResource('STH#',id))
	{
		count = 256*(*resH)[0]+(*resH)[1];
		if (index<=count)
		{
			spot = *resH + 2;
			for (index--;index;index--) spot += *spot+1;
			PCopy(string,spot);
		}
	}
}

/**********************************************************************
 * PrefHelpAlert - put up an alert with the pref help in it
 **********************************************************************/
void PrefHelpAlert(short pref)
{
	Str255 help;
	Str255 intro;
	
	GetRString(intro,PREFERROR_INTRO);
	PrefHelpString(pref,help,false);
	MyParamText(intro,help,"","");
	ReallyDoAnAlert(BIG_OK_ALRT,Stop);
}

/**********************************************************************
 * CheckSpecialPref - check a pref for special stuff
 **********************************************************************/
OSErr CheckPrefSpecial(short pref, PStr value)
{
	switch(pref)
	{
		case PREF_STUPID_HOST:
			if (PIndex(value,'@')) return 1;
			break;
		case JUNK_MAILBOX_THRESHHOLD:
		{
			long num;
			StringToNum(value,&num);
			if (num<GetRLong(JUNK_MIN_REASONABLE))
			{
				short button = ComposeStdAlert(Caution,JUNK_UNREASONABLE_WARNING,FILE_ALIAS_JUNK,JUNK_MIN_REASONABLE,FILE_ALIAS_JUNK,JUNK_MIN_REASONABLE);
				if (CommandPeriod || button==kAlertStdAlertCancelButton) return userCanceledErr;
				if (button==kAlertStdAlertOKButton) GetRString(value,JUNK_MIN_REASONABLE);
			}
			break;
		}
	}
	return(noErr);
}


/************************************************************************
 * SetPref - set a preference, and take any necessary action
 ************************************************************************/
OSErr SetPref(short pref,PStr value)
{
	OSErr err;
	short id, num;
	Str255 oldValue;
	Str255 s;
	long longVal;
	Boolean neg;
	
	if (!CurPers) CurPers = PersList;
	
	if (neg=(pref<0)) pref *= -1;
		
	if (EqualString(GetPrefNoDominant(oldValue,pref),value,True,True)) return(noErr);
	
	pref = PrefNormalize(pref);
	
	if (PrefIsOverride(pref))
	{
		err = SetStrOverride(pref,value);
	}
	else
	{
		GetPrefParms(pref,&id,&num);
		
		if (CurPers!=PersList)	// non-dominant personalities always use overrides
		  err = SetStrOverride(id+num,value);
		else
		{
			/*
			 * change the preference
			 */
			SetStrOverride(id+num,"");	// kill override!
			ChangeStrn(id,num,value);
			if (err=ResError()) return(err);
		}
	}
	
	if (err) return(err);

#ifdef THREADING_ON
	// note thread changes to preferences
	if (InAThread ()) 
		PushThreadPrefChange (pref);
#endif
	
	/*
	 * take action
	 */
	switch (pref)
	{
		case PREF_JUNK_MAILBOX:
		{
			long oldLongVal;
			
			StringToNum(oldValue,&oldLongVal);
			StringToNum(value,&longVal);
			if ((oldLongVal&bJunkPrefSwitchCmdJ) != (longVal&bJunkPrefSwitchCmdJ))
				JunkReassignKeys(0==(longVal&bJunkPrefSwitchCmdJ));
			break;
		}
		
		case PREF_AUTOHEX_VOLPATH:
			GetAttFolderSpecLo(&AttFolderSpec);
			ZapHandle(AttFolderStack);	// this is no longer valid. -jdboyd 12/18/03
			break;
		
		case PREF_POP_SIGH:
			InvalidatePasswords(False,True,False);
			(*CurPers)->noUIDL = False;
			PersSetAutoCheck();
			break;
		
		case PREF_INTERVAL:
			StringToNum(value,&longVal);
			if (longVal) (*CurPers)->checkTicks = TickCount();
			else (*CurPers)->checkTicks = 0;
			PersSetAutoCheck();
			break;
		
		case PREF_PROXY:
		{
			ProxyHandle proxy = ProxyFind(value);
			(*CurPers)->proxy = proxy;
			break;
		}
		
		case PREF_AUTO_CHECK:
			if (PrefIsSet(PREF_AUTO_CHECK))
				(*CurPers)->autoCheck = True;
			else
				(*CurPers)->autoCheck = False;
			(*CurPers)->checkTicks = 0;
			PersSetAutoCheck();
			break;

		case PREF_KEYCHAIN:
			if (!KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
				SetPref(PREF_KEYCHAIN,NoStr);
#ifdef HAVE_KEYCHAIN
			else if (PrefIsSet(PREF_KEYCHAIN))
				KeychainConvert();
#endif //HAVE_KEYCHAIN
			break;
		
	//MJD	case PREF_USE_LIST_COLORS:
		case PREF_NO_LINES:
		case PREF_BOX_HIDDEN:
		case SEP_LINE_DARK_COLOR:
		case SEP_LINE_LIGHT_COLOR:
		case LIGHT_COLOR:
		case PREF_NO_COURIC:
			{
				WindowPtr	tocWinWP;
				TOCHandle tocH;
				Rect	rWinPort;
				PushGWorld();
				for (tocH=TOCList;tocH;tocH=(*tocH)->next)
				{
					tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
					SetPort_(GetWindowPort(tocWinWP));
					InvalWindowRect(tocWinWP,GetWindowPortBounds(tocWinWP,&rWinPort));
					BoxPlaceBevelButtons((*tocH)->win);
				}
				PopGWorld();
				LinkTickle();
			}
			break;
			
		case PREF_AUXUSR:
			InvalidatePasswords(True,False,False);
			break;
		
		case PREF_NO_CHECK_MEM:
			NoPreflight = PrefIsSet(PREF_NO_CHECK_MEM);
			break;
			
		case PREF_SMTP:
			PersSetAutoCheck();
			NewPhHost();
			SetPref(PREF_SMTP_DOES_AUTH,YesStr);
			SetPref(PREF_SMTP_GAVE_530,NoStr);
			break;
			
		case PREF_OFFLINE:
			Offline = PrefIsSet(PREF_OFFLINE);
			/* fall-through */
#ifdef CTB
		case PREF_TRANS_METHOD:
			UseCTB = PrefIsSet(PREF_TRANS_METHOD);
			CurTrans = GetTCPTrans();
			GetRString(NewLine,UseCTB ? CTB_NEWLINE : NEWLINE);
			if (UseCTB) EnableItem (GetMHandle(SPECIAL_MENU),AdjustSpecialMenuItem(SPECIAL_CTB_ITEM));
			else        DisableItem(GetMHandle(SPECIAL_MENU),AdjustSpecialMenuItem(SPECIAL_CTB_ITEM));
			break;
#endif
			
		case PREF_DUMB_PASTE:
			PETEAllowIntelligentEdit(PETE,!PrefIsSet(PREF_DUMB_PASTE));
			break;
		
		case PREF_ANCHORED_SELECTION:
			PETEAnchoredSelection(PETE,PrefIsSet(PREF_ANCHORED_SELECTION));
			break;
		
		case PREF_PH:
		case PREF_FINGER_HOST:
			NewPhHost();
			break;
		
		case PREF_3D:
			if (D3 != PrefIsSet(PREF_3D) && ThereIsColor)
			{
				D3 = !D3;
				TBarHasChanged = True;
				RedrawAllWindows();
			}
			break;
		
		case PrivColorsStrn+1:
		case PrivColorsStrn+2:
		case PrivColorsStrn+3:
		case PrivColorsStrn+4:
		case PrivColorsStrn+5:
		case PrivColorsStrn+6:
		case PrivColorsStrn+7:
		case PrivColorsStrn+8:
		case PrivLabelsStrn+1:
		case PrivLabelsStrn+2:
		case PrivLabelsStrn+3:
		case PrivLabelsStrn+4:
		case PrivLabelsStrn+5:
		case PrivLabelsStrn+6:
		case PrivLabelsStrn+7:
		case PrivLabelsStrn+8:
			RefreshLabelsMenu();
			// fall-through on a porpoise...
		case REPLY_COLOR:
		case URL_COLOR:
		case TEXT_COLOR:
		case BACK_COLOR:
		case QUOTE_COLOR:
		case QUOTE_BLEND:
		case SPELL_COLOR:
		case LINK_COLOR:
		case NICK_COLOR:
		case MOOD_H_COLOR:
		case MOOD_COLOR:
			if (PERS_FORCE(CurPers)==PersList) RedrawAllWindows();
			break;

		case STAT_CURRENT_COLOR:
		case STAT_PREVIOUS_COLOR:
		case STAT_AVERAGE_COLOR:
		case STAT_CURRENT_TYPE:
		case STAT_PREVIOUS_TYPE:
		case STAT_AVERAGE_TYPE:
		case STAT_GRAPH_HEIGHT:
		case STAT_GRAPH_WIDTH:
		case STAT_LEGEND_WIDTH:
			RedisplayStats();
			break;		
			
		case PREF_LIVE_SCROLL :
			PETELiveScroll(PETE, PrefIsSet(PREF_LIVE_SCROLL));
			break;
			
		case PREF_PRINT_FONT:
		case PREF_PRINT_FONT_SIZE:
		case PREF_FONT_NAME:
		case PREF_FONT_SIZE:
		case PREF_TEXT_FONT:
		case PREF_TEXT_SIZE:
#ifdef USEFIXEDDEFAULTFONT
		case FIXED_FONT:
		case DEF_FIXED_SIZE:
#endif
			FontHasChanged();
			break;
		
		case PREF_REPLY_ALL:
		case PREF_TOOLBAR:
		case PREF_TB_VARIATION:
		case PREF_TB_VERT:
		case PREF_NW_DEV:
		case PREF_SHOW_FKEYS:
		case PREF_TB_FKEYS:
		case TOOLBAR_EXTRA_PIXELS:
		case TOOLBAR_EXTRA_COUNT:
		case TOOLBAR_SEP_PIXELS:
		case ToolbarSizesStrn+1:
		case ToolbarSizesStrn+2:
		case ToolbarSizesStrn+3:
		case ToolbarSizesStrn+4:
		case ToolbarSizesStrn+5:
		case ToolbarSizesStrn+6:
		case ToolbarSizesStrn+7:
		case ToolbarSizesStrn+8:
		case PREF_NO_TB_SEARCH:
			TBarHasChanged = True;
			break;
			
		case PREF_PASS_TEXT:
			Zero((*CurPers)->password);
			PSCopy((*CurPers)->password,value);
			(*CurPers)->popSecure = False;
			break;
		
		case PREF_AUXPW:
			Zero((*CurPers)->password);
			PSCopy((*CurPers)->secondPass,value);
			break;
		
		case PREF_LOG:
			StringToNum(value,&longVal);
			LogLevel = longVal;
			break;
		
		case PREF_FIXED_DATES:
		case PREF_LOCAL_DATE:
			RedoAllTOCs();
			LinkTickle();
			break;
		
		case PREF_SYNC_IO:
			SyncRW = PrefIsSet(PREF_SYNC_IO);
			break;
		
		case PREF_SAVE_PASSWORD:
			(*CurPers)->dirty = true;
			(*CurPers)->popSecure = false;
			Zero((*CurPers)->password);
			SetPref(PREF_PASS_TEXT,"");
			SetPref(PREF_AUXPW,"");
			SetPref(PREF_ACAP_PASS,"");
			break;
		
		case PREF_STUPID_USER:
			if (!neg) SetPOPAccount(value,GetPref(s,PREF_STUPID_HOST));
			if (IsIMAPPers(CurPers)) 
			{
				// close all connections to the old server
				IMAPInvalidatePerConnections(CurPers);
				
				// reset SpamWatch prefs
				IMAPResetSpamSupportPrefs();
				
				// zap local cache
				DeleteIMAPPers(CurPers, true);
			}
		  break;
		  
		case PREF_STUPID_HOST:
			if (!neg) SetPOPAccount(GetPref(s,PREF_STUPID_USER),value);
			if (IsIMAPPers(CurPers)) 
			{
				// close all connections to the old server
				IMAPInvalidatePerConnections(CurPers);
				
				// reset SpamWatch prefs
				IMAPResetSpamSupportPrefs();
				
				// zap local cache
				if (!PrefIsSet(PREF_SAVE_IMAP_CACHE_SERVER))
					DeleteIMAPPers(CurPers, true);
			}
			break;

		case PREF_NICK_EXP_TYPE_AHEAD:
		case PREF_NICK_WATCH_IMMED:
		case PREF_NICK_WATCH_WAIT_KEYTHRESH:
		case PREF_NICK_WATCH_WAIT_NO_KEYDOWN:
		case PREF_NICK_POPUP_ON_MULTMATCH:
		case PREF_NICK_POPUP_ON_DEFOCUS:
		case PREF_NICK_HILITING:
		case PREF_NICK_TYPE_AHEAD_HILITING:
		case PREF_NICK_CACHE:
			RefreshNicknameWatcherGlobals (true);
			break;

		case PREF_SEND_STYLE:
			TFBRespondToSettingsChanges();
			break;

		case BG_YIELD_INTERVAL:
			StringToNum(value,&BgYieldInterval);
			break;

		case FG_YIELD_INTERVAL:
			StringToNum(value,&FgYieldInterval);
			break;
			
		case VICOM_FACTOR:
			StringToNum(value,&VicomFactor);
			break;
		
#ifdef WINTERTREE
		case PREF_NO_WINTERTREE:
			if (HasFeature (featureSpellChecking) && PrefIsSet(pref)) SpellClose(true);
			break;
		
		case PREF_WINTERTREE_OPTS:
			WinterTreeOptions = GetPrefLong(PREF_WINTERTREE_OPTS);
			SpellAgain();
			break;

#endif
		case PREF_NO_ADWIN_DRAG_BAR:
			SizeAdWin();
			break;
			
		case PREF_IMAP_NO_FANCY_TRASH:
			ResetTrashMailbox(CurPers);
			break;

#ifdef VCARD
		case PREF_PERSONAL_NICKNAMES_NOT_VISIBLE:
			ABTickleHardEnoughToMakeYouPuke ();
			break;
		case PREF_VCARD:
			// If we're turning on vCard awareness...
			if (PrefIsSet (PREF_VCARD)) {
				FSSpec	spec;
				Str255	name;
			
				// Create the personal address book
				GetRString (name, PERSONAL_ALIAS_FILE);
				if (FSMakeFSSpec (Root.vRef, Root.dirId, name, &spec))
					if (err = MakeResFile (name, Root.vRef, Root.dirId, CREATOR, 'TEXT')) {
						WarnUser (CREATING_PERSONAL_ALIAS, err);
						SetPref (PREF_VCARD, NoStr);
					}
				if (!err)
					RegenerateAliases (FindAddressBookType (personalAddressBook), true);
			}
			ABTickleHardEnoughToMakeYouPuke ();
			break;
#endif

		case PREF_ALTERNATE_CHECK_MAIL_CMD:
		case PREF_NO_ALTERNATE_ATTACH_CMD:
		case COMMAND_KEY_CHECKMAIL_REPLACEMENT:
		case COMMAND_KEY_ATTACH_REPLACEMENT:
			AdjustStupidCommandKeys();
			break;
			
		case PREF_NO_USE_UIDPLUS:
			// SpamWatch currently requires UIDPLUS.  Turn off Scoring plugins if we disable UIDPLUS.
			SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefIMAPNoRunPlugins);
			break;

		case PREF_MAKE_EUDORA_DEFAULT_MAILER:
			if ( value[1] == 'y' ) {
				ICInstance icp;

				err = ICStart ( &icp, '\?\?\?\?' );
				ASSERT ( noErr == err );
				err = SetEudoraAsTheDefaultMailClient ( icp );
				err = ICStop ( icp );
				ASSERT ( noErr == err );
				}
			break;
			
		case PREF_SYNC_OSXAB:
			if ( value[1] != 'y' )
				OSXAddressBookSyncCleanup(true);
			else
				OSXAddressBookSync();
			
		case PREF_BITE_ME_EMO:
			StringToNum(value,&longVal);
			if (longVal & emoPrefOffBit)
				EmoSearchAndDestroy();
			break;
		
		case PREF_QD_TEXT_ROUTINES:
			SwapQDTextFlags(PrefIsSet(PREF_QD_TEXT_ROUTINES) ? kQDUseTrueTypeScalerGlyphs : kQDUseCGTextRendering|kQDUseCGTextMetrics);
			FontHasChanged();
			break;

	}
	
	return(noErr);
}

/************************************************************************
 * GetPrefNoDominant - get a pref, do not take from dominant
 ************************************************************************/
PStr GetPrefNoDominant(PStr string, short prefNum)
{
	PStr val;
	Boolean oldNoDom = NoDominant;
	Boolean oldNoProx = NoProxify;
	
	NoDominant = NoProxify = True;
	
	val = GetPref(string,prefNum);
	
	NoDominant = oldNoDom;
	NoProxify = oldNoProx;
	
	return(val);
}

/************************************************************************
 * GetPrefNoDominant - get a pref, do not take from dominant
 ************************************************************************/
Boolean GetPrefBitNoDominant(short prefNum, long mask)
{
	Boolean val;
	Boolean oldNoDom = NoDominant;
	Boolean oldNoProx = NoProxify;
	
	NoDominant = NoProxify = True;
	
	val = GetPrefBit(prefNum,mask);
	
	NoDominant = oldNoDom;
	NoProxify = oldNoProx;
	
	return(val);
}

/************************************************************************
 * SetStupidStuff - set the stupid user and host name from the pop account
 ************************************************************************/
void SetStupidStuff(void)
{
	Str255 user, host, scratch;
	UPtr at;
	
	GetPref(scratch,PREF_POP_SIGH);
	scratch[*scratch+1] = 0;
	at = strrchr(scratch+1,'@');
	
	if (at)
	{
		MakePStr(user,scratch+1,at-scratch-1);
		MakePStr(host,at+1,strlen(at)-1);
	}
	else
	{
		PCopy(user,scratch);
		*host = 0;
	}
	SetPref(-PREF_STUPID_USER,user);
	SetPref(-PREF_STUPID_HOST,host);
}

/************************************************************************
 * SetPOPAccount - set the pop account from the stupid stuff
 ************************************************************************/
void SetPOPAccount(PStr user,PStr host)
{
	Str255 localUser, localHost;
	
	GetPOPInfo(localUser,localHost);
	if (user) PCopy(localUser,user);
	if (!host) host = localHost;
	if (*host)
	{
		PCatC(localUser,'@');
		PCat(localUser,host);
	}
	SetPref(PREF_POP_SIGH,localUser);
}
	

/************************************************************************
 * SetStrOverride - set a string override
 ************************************************************************/
OSErr SetStrOverride(short pref, PStr value)
{
	OSErr err;
	OSType curType = CUR_STR_TYPE;
	Handle res = GetResource(curType,pref);
	
	SCClear(pref);
	
	if (!res)
	{
		if (*value || curType!='STR ')
		{
			res = NuHandle(*value+1);
			if (!res) return(MemError());
			PCopy(*res,value);
			AddResource(res,curType,pref,"");
			err = ResError();
		}
	}
	else if (HomeResFile(res)==SettingsRefN)
	{
		if (*value || curType!='STR ')
		{
			HNoPurge(res);
			SetHandleBig(res,*value+1);
			if (!(err=MemError()))
			{
				PCopy(*res,value);
				ChangedResource(res);
			}
		}
		else
		{
			RemoveResource(res);
			DisposeHandle(res);
			err = noErr;
		}
	}
	return(err);
}

/**********************************************************************
 * SetPrefLong - stuff a long into a pref
 **********************************************************************/
void SetPrefLong(short prefNum,long val)
{
	Str63 string;
	NumToString(val,string);
	SetPref(prefNum,string);
}
	
/************************************************************************
 * FontHasChanged - the user changed the font on us on the fly.  Fix up
 * windows.
 ************************************************************************/
void FontHasChanged(void)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	GrafPtr oldPort;
	short kind;
	
	GetPort(&oldPort);
	
	FigureOutFont(True);
	
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsKnownWindowMyWindow(winWP))
		{
			win = GetWindowMyWindowPtr (winWP);
			SetPort_(GetWindowPort(winWP));
			kind = GetWindowKind(winWP);
			
			if (!CONFIG_KIND(kind))
			{
				TextFont(FontID);
				TextSize(FontSize);
				win->vPitch = FontLead;
				win->hPitch = FontWidth;
			}
			switch (kind)
			{
				case MBOX_WIN:
				case CBOX_WIN:
					win->vPitch = FontLead+FontDescent;
#ifdef NEVER
					CalcAllSumLengths((TOCHandle)GetMyWindowPrivateData(win));
#endif
					break;
				case PH_WIN:
					PhFixFont();
					break;
				case ALIAS_WIN:
					AliasesFixFont();
					break;
			}
			SetPort_(GetWindowPort(winWP));
			MyWindowDidResize(win,&win->contR);
		}
	RedrawAllWindows();
	SetPort_(oldPort);
}

/************************************************************************
 * SettingsPtr - put some data into the Settings file
 ************************************************************************/
short SettingsPtr(ResType type,UPtr name,short id,UPtr dataPtr, long dataSize)
{
	Handle rH = NuHandle(dataSize);
	short err;
	
	if (!rH) return(MemError());
	BMD(dataPtr,*rH,dataSize);
	if (err=SettingsHandle(type,name,id,rH))
		ZapHandle(rH);
	return(err);
}

/************************************************************************
 * SettingsHandle - put some data into the Settings file
 ************************************************************************/
short SettingsHandle(ResType type,UPtr name,short id,UHandle	dataHandle)
{
	short oldRefN = CurResFile();
	short err;
	Handle oldDataHandle;
	
	UseResFile(SettingsRefN);
	oldDataHandle = Get1Resource(type,id);
	if (oldDataHandle!=nil)
	{
		RemoveResource(oldDataHandle);
		ZapHandle(oldDataHandle);
	}
	
	AddResource_(dataHandle,type,id,name?name:"");
	err = ResError();
	UseResFile(oldRefN);
	return(err);
}


/************************************************************************
 * SaveBoxLines - save the current setting of BoxLines
 ************************************************************************/
void SaveBoxLines(void)
{
	Handle bLinesH = Get1Resource('STR#',BoxLinesStrn);
	Str31 num;
	short last=0,offset = 0;
	short i;
	
	if (!bLinesH)
	{
		if (!(bLinesH=NuHandleClear(2))) return;
		*num = 0;
		AddMyResource_(bLinesH,'STR#',BoxLinesStrn,num);
	}
	
	for (i=1;i<BoxLinesLimit;i++)
	{
		NumToString((*BoxWidths)[i-1],num);
		ChangeStrn(BoxLinesStrn,i,num);
	}
	MyUpdateResFile(SettingsRefN);
}

#pragma segment GetPrefs

/************************************************************************
 * PrefIsSet - is an on/off preference on?
 ************************************************************************/
Boolean PrefIsSet(short pref)
{
	Str255 scratch;
	
	GetPref(scratch,pref);
	return(*scratch && scratch[1]=='y');
}

/************************************************************************
 * PrefIsSetSet - is an on/off preference on?, and set it right away
 ************************************************************************/
Boolean PrefIsSetSet(short pref)
{
	Str255 scratch;
	
	GetSetPref(scratch,pref);
	return(*scratch && scratch[1]=='y');
}

/************************************************************************
 * GetPref - get a preference
 ************************************************************************/
PStr GetPref(PStr string, short prefNum)
{
	short num,id;
	
	prefNum = PrefNormalize(prefNum);
	
	if (!PrefIsOverride(prefNum))
	{
		GetPrefParms(prefNum,&id,&num);
		return(GetRString(string,id+num));
	}
	else return(GetRString(string,prefNum));
}

/************************************************************************
 * GetSetPref - get a preference, then immediately set it
 ************************************************************************/
PStr GetSetPref(PStr string, short prefNum)
{
	GetPref(string,prefNum);
	SetPref(prefNum,string);
	return(string);
}

/************************************************************************
 * GetDefPref - get default value of a preference
 ************************************************************************/
PStr GetDefPref(PStr string, short prefNum)
{
	short realSettingsRef = SettingsRefN;
	
	SettingsRefN = AppResFile;
	NoDominant++;
	GetPref(string,prefNum);
	NoDominant--;
	SettingsRefN = realSettingsRef;
	
	return(string);
}

/************************************************************************
 * GetPrefLong - get a long out of a pref
 ************************************************************************/
long GetPrefLong(short prefNum)
{
	short num,id;
	GetPrefParms(prefNum,&id,&num);
	return(GetRLong(id+num));
}

/************************************************************************
 * GetSetPrefLong - get a long out of a pref
 ************************************************************************/
long GetSetPrefLong(short prefNum)
{
	short num,id;
	Str255 s;
	
	GetSetPref(s,prefNum);
	GetPrefParms(prefNum,&id,&num);
	return(GetRLong(id+num));
}

/************************************************************************
 * GetPrefParms - figure out resource and number from pref number
 ************************************************************************/
void GetPrefParms(short pref, short *id, short *num)
{
	if (pref>300)
	{
		*id = PREF4_STRN;
		*num = pref-300;
	}
	else if (pref>200)
	{
		*id = PREF3_STRN;
		*num = pref-200;
	}
	else if (pref>100)
	{
		*id = PREF2_STRN;
		*num = pref-100;
	}
	else
	{
		*id = PREF_STRN;
		*num = pref;
	}
}

/**********************************************************************
 * GetRealname - get the user's real name
 **********************************************************************/
PStr GetRealname(PStr name)
{
	Str255 user, scratch;
	OSErr err;
	UPtr spot;
	
	GetPref(name,PREF_REALNAME);
#if 0 // CK hesiod
	if (EqualStrRes(name,HESIOD))
	{
		*name = 0;
		GetPOPInfo(user,scratch);
		if (*user)
		{
			char **argv;

			// ask for the passwd string
			GetRString(scratch,HESIOD_PASSWD);
			user[*user+1] = 0;
			scratch[*scratch+1] = 0;
			if (hes_resolve)	//	Is Hesiod library installed?
			{
				if (argv=hes_resolve(user+1,scratch+1))
				{
					err = 0;
					if (*argv[0])
					{
						CtoPCpy(scratch,argv[0]);
						spot = scratch+1;
						if (PToken(scratch,name,&spot,":") &&
								PToken(scratch,name,&spot,":") &&
								PToken(scratch,name,&spot,":") &&
								PToken(scratch,name,&spot,":") &&
								PToken(scratch,name,&spot,":"))
							SetPref(PREF_REALNAME,name);
					}
				}
				else if ((err=hes_error()) && err!=HES_ER_NOTFOUND)
				{
					Aprintf(OK_ALRT,Stop,HESIOD_ERR,user,scratch,err);
				}
			}
		}
	}	
#endif
	return(name);
}

/************************************************************************
 * DefaultCreator - return the default creator for text files
 ************************************************************************/
OSType DefaultCreator(void)
{
	OSType creator;
	Str31 scratch;
	
	GetPref(scratch,PREF_CREATOR);
	if (*scratch!=4) GetRString(scratch,TEXT_CREATOR);
	BMD(scratch+1,&creator,4);
	return(creator);
}

/************************************************************************
 * FetchOSType - fetch an OSType from a resource
 ************************************************************************/
OSType FetchOSType(short index)
{
	OSType creator;
	Str31 scratch;
	
	GetRString(scratch,index);
	BMD(scratch+1,&creator,4);
	return(creator);
}


/************************************************************************
 * GetAttFolderSpec - get the FSSpec for the attachments folder
 ************************************************************************/
OSErr GetAttFolderSpecLo(FSSpecPtr spec)
{
	Str63 volPath,volName;
	
	if (!GetVolpath(GetPref(volPath,PREF_AUTOHEX_VOLPATH),volName,&spec->parID))
		ForceAttachFolder(volName,&spec->parID);
		
	spec->vRefNum = GetMyVR(volName);
	*spec->name = 0;
	return(noErr);
}

/************************************************************************
 * GetHexPath - get the auto-binhex path
 ************************************************************************/
Boolean GetHexPath(UPtr folder,UPtr volpath)
{
	Str31 volName;
	long dirId;
	short vRef;
	Boolean result;
	UPtr rev;
	
	if (!GetVolpath(volpath,volName,&dirId))
	{
		HGetVol(volName,&vRef,&dirId);
	}
	if (result=GetFolder(volName,&vRef,&dirId,True,True,True,True))
	{
		PCopy(volpath,volName);
		PLCat(volpath,dirId);
		for (rev=volpath+*volpath;*rev!=' ';rev--);
		*rev = ':';
		(void) GetDirName(volName,vRef,dirId,folder);
	}
	return(result);
}

/************************************************************************
 * GetPOPInfo - get pop user and account
 ************************************************************************/
OSErr GetPOPInfoLo(UPtr user, UPtr host, long *port)
{
	Str255 scratch;
	UPtr atSign;
	OSErr err = noErr;
	Str255 local;
	long localPort;
	
	if (!user) user = local;
	if (!host) host = local;
	
	*user = *host = 0;
	GetPOPPref(scratch);
	if (*scratch)
	{
		if (atSign=PRIndex(scratch,'@'))
		{
			*atSign = *scratch - (atSign-scratch);
			*scratch = atSign-scratch-1;
			PCopy(user,scratch);
			PCopy(host,atSign);
			user[*user+1] = host[*host+1] = 0;
		
#ifdef TWO
			if (EqualStrRes(host,HESIOD))
			{
				err = HesiodPOPInfo(user,host);
			}
#endif
		}
		else
			PCopy(user,scratch);
	}
	user[*user+1] = host[*host+1] = 0;
	
	if (SplitPort(host,&localPort) && port) *port = localPort;
	
	if (!*user) err = 1;
	return(err);
}

/************************************************************************
 * GetPOPPref - get the pop account pref.  This used to be so simple :-(
 ************************************************************************/
PStr GetPOPPref(PStr popAcct)
{
	Str127 host;
	uLong curPersId = CurThreadGlobals ? (CurPers ? (*CurPers)->persId : 0) : 0;
	
	GetPref(popAcct,PREF_STUPID_USER);
	GetPref(host,PREF_STUPID_HOST);
	if (*host && *popAcct) PCatC(popAcct,'@');
	if (host) PCat(popAcct,host);
	return popAcct;
}

#if 0 // CK hesiod?

static Str127 hes_host, hes_user;

/************************************************************************
 * HesiodKill - kill the hesiod stuff
 ************************************************************************/
void HesiodKill(void)
{
	*hes_host = *hes_user = 0;
}

/************************************************************************
 * HesiodPOPInfo - get pop info from hesiod server
 ************************************************************************/
OSErr HesiodPOPInfo(PStr user,PStr host)
{
	char **argv;
	Str127 hes_serv;
	OSErr err = noErr;
	UPtr spot;
	Str255 scratch;
	Str255 pobox;
	
	if (HesOK)
	{
		*hes_user = *hes_host = 0;
		
		if (PrefIsSet(PREF_KERBEROS) && (err=KerbUsername(user))) return(err);
		user[*user+1] = 0;	//null terminate
		
		ProgressMessage(kpSubTitle,
			ComposeRString(scratch,HESIOD_LOOKUP,user,GetRString(pobox,HESIOD_POBOX)));
		
		/*
		 * first, let's try the sophisticated form
		 */
		if (hes_resolve)	//	Is Hesiod library installed?
		{
			if (argv=hes_resolve(user+1,pobox+1))
			{
				err = 0;
				if (*argv[0])
				{
					CtoPCpy(scratch,argv[0]);
					spot = scratch+1;
					if (PToken(scratch,hes_serv,&spot," ")
							&& EqualStrRes(hes_serv,HESIOD_POP3)
							&& PToken(scratch,hes_host,&spot," ")
							&& PToken(scratch,hes_user,&spot," "))
					{
						if (!PrefIsSet(PREF_NO_LOWER_HESIOD))
						{
							MyLowerStr(hes_host);
							MyLowerStr(hes_serv);	// rad, man
						}
						HesOK = True;
					}
					else *hes_host = *hes_serv = 0;
				}
			}
			else if ((err=hes_error()) && err!=HES_ER_NOTFOUND)
			{
				Aprintf(OK_ALRT,Stop,HESIOD_ERR,user,pobox,err);
				return(err);
			}
		}
						
		/*
		 * if that failed, we'll try the stupid one
		 */
		if (!(*hes_host && *hes_user))
		{
			ProgressMessage(kpSubTitle,
				ComposeRString(scratch,HESIOD_LOOKUP,
					GetRString(hes_serv,HESIOD_POP3),
					GetRString(hes_host,HESIOD_SLOC)));
			if (argv=hes_resolve(hes_serv+1, hes_host+1))
			{
				if (*argv[0])
				{
					CtoPCpy(hes_host,argv[0]);
					PCopy(hes_user,user);	//user the username we were already given
				}
				err = 0;
				HesOK = True;
			}
			else
			{
				if ((err=hes_error()) && err!=HES_ER_NOTFOUND)
				{
					Aprintf(OK_ALRT,Stop,HESIOD_ERR,hes_serv,hes_host,err);
					return(err);
				}
				else return(WarnUser(HESIOD_NOTFOUND,err));
			}
		}
	}
		
	/*
	 * did we get a response from Hesiod?
	 */
	if (*hes_user && *hes_host)
	{
		PCopy(user,hes_user);
		PCopy(host,hes_host);
	}
	else
	{
		*user = *host = 0;
		if (!err) err = 1;
	}
	return(err);
}
#endif

/************************************************************************
 * GetSMTPInfo - get SMTP host (fallback to POP host if necessary)
 ************************************************************************/
UPtr GetSMTPInfoLo(UPtr host,long *port)
{
	Str255 user;
	long localPort;
	
	GetPref(host,PREF_SMTP);
	if (!*host)
		GetPOPInfo(user,host);
	else if (host[1]!='!')
	{
		if (SplitPort(host,&localPort) && port) *port = localPort;
	}
		
	// fall back to dominant; we want an SMTP server very badly...
	if (!*host && CurPers!=PersList)
	{
		PushPers(PersList);
		GetSMTPInfoLo(host,port);
		PopPers();
	}
	
	return(host);
}

/************************************************************************
 * GetShortReturnAddr - get return address, short form
 ************************************************************************/
UPtr GetShortReturnAddr(UPtr addr)
{
	Str255 longAddr;
	return ShortAddr(addr,GetReturnAddr(longAddr,false));
}

/************************************************************************
 * GetReturnAddr - get return address (fallback to POP account if necessary)
 ************************************************************************/
UPtr GetReturnAddr(UPtr addr,Boolean comments)
{
	UHandle canon;
	Str255 realname, scratch;
	Str255 host;
	
	/*
	 * is pref set?
	 */
	GetPref(addr,PREF_RETURN);
	
	/*
	 * use POP account.
	 * if host is ip address, use domain literal
	 */
	if (!*addr)
	{
		GetPOPInfo(addr,host);
		PCatC(addr,'@');
		if (DotToNum(host,(long*)realname))
		{
			/* ip address; turn into domain literal */
			PCatC(addr,'[');
			PCatC(host,']');
		}
		PCat(addr,host);
	}
	
	/*
	 * validate
	 */
	if (!SuckPtrAddresses(&canon,addr+1,*addr,comments,True,False,nil))
	{
		*addr = 0;
		if (!comments && **canon!='<') PCatC(addr,'<');
		PCat(addr,*canon);
		if (!comments && addr[*addr]!='>') PCatC(addr,'>');
		ZapHandle(canon);
	}
	if (comments)
	{
		GetRealname(scratch);
		if (*scratch)
		{
			Str255 fmt,buffer;
			GetRString(fmt,ADD_REALNAME);
			ComposeRString(realname,RNAME_QUOTE,scratch);
			utl_PlugParams(fmt,buffer,addr,realname,nil,nil);
			PCopy(addr,buffer);
		}
	}
	// null terminate
	addr[*addr+1] = 0;
	return(addr);
}

/************************************************************************
 * OkSigIntro - make sure the user hasn't effed with the sig intro
 ************************************************************************/
Boolean OkSigIntro(void)
{
	Str255 intro;
	
	GetRString(intro,SIG_INTRO);
	if (*intro<2) return false;
	if (intro[*intro]!='\015') return false;
	return true;
}

/************************************************************************
 * SigValidate - make sure the sig really exists and return one that does
 ************************************************************************/
long SigValidate(long sigID)
{
	FSSpec spec;
	if (sigID==SIG_NONE) return sigID;
	if (!SigSpec(&spec,sigID)) return sigID;
	if (!SigSpec(&spec,SIG_STANDARD)) return SIG_STANDARD;
	return SIG_NONE;
}

/************************************************************************
 * GetVolpath - check the auto-binhex path
 ************************************************************************/
Boolean GetVolpath(UPtr volpath,UPtr volName,long *dirId)
{
	Str31 maybeVol,maybeFolder;
	UPtr colon;
	long maybeId,oldDirID,newDirID;
	short oldVol,newVol;
	short err;
	
	/*
	 * volpath is of the form "volname:dirId"; find that colon.
	 */
	if (!*volpath) return(False);
	for (colon=volpath+1;colon<volpath+*volpath;colon++) if (*colon==':') break;
	if (*colon!=':') return(False);
	
	/*
	 * volume name
	 */
	BMD(volpath,maybeVol,colon-volpath+1);
	*maybeVol = colon-volpath;
	
	/*
	 * dirId
	 */
	maybeId=0;
	for (colon++;colon<=volpath+*volpath;colon++)
		{maybeId *= 10; maybeId += (*colon)-'0';}

	/*
	 * now check it
	 */
	HGetVol(nil,&oldVol,&oldDirID);
	if (!(err=HSetVol(maybeVol,0,0)))
	{
		HGetVol(nil,&newVol,&newDirID);
		if (!(err = GetDirName(maybeVol,0,maybeId,maybeFolder)))
		{
			*dirId = maybeId;
			PCopy(volName,maybeVol);
		}
	}
	HSetVol(nil,oldVol,oldDirID);
	return(!err);
}

/**********************************************************************
 * AutoCheckOK - is it ok to make an automatic check?
 **********************************************************************/
Boolean AutoCheckOK(void)
{
	uLong hourBits;
	DateTimeRec dtr;
	
	if (Offline) return(False);
#ifndef ONE
	if (PrefIsSet(PREF_NO_BATT_CHECK) && OnBatteries()) return(False);
	if (!PrefIsSet(PREF_IGNORE_PPP))
	{	
		if (PrefIsSet(PREF_PPP_NOAUTO) && PPPDown()) return(False);
	}
#endif
	hourBits = GetPrefLong(PREF_CHECK_HOURS);
	if (0!=(hourBits&kHourUseMask) || 0!=(hourBits&kHourNeverWeekendMask))
	{
		SecondsToDate(LocalDateTime(),&dtr);
		if (dtr.dayOfWeek==1||dtr.dayOfWeek==7)	// weekends
		{
			if (hourBits&kHourAlwaysWeekendMask) return(True);
			if (hourBits&kHourNeverWeekendMask) return(False);
		}
		if (0!=(hourBits&kHourUseMask))	// non-weekends, or weekend same as weekday
		{
			return 0!=(hourBits&(1L<<dtr.hour));
		}
	}
	return(True);
}

#pragma segment SettingDialog

typedef struct SettingsGlobals
{
#ifdef SETTINGSLISTKEY
	Boolean listFocus;	
	short firstTextItem, lastTextItem;
#endif
	MyWindowPtr dlog;		/* our dialog */
	ListHandle list;				/* our List Manager list */
	short **ditlList;		/* list of DITL id's */
	short **prefList;		/* list of preference #'s for current dialog */
	short **bitList;		/* list of bit #'s for current dialog */
	Str127 **valList;		/* list of values for current dialog */
	short currentScreen;	/* current screen */
	Str63 currentName;		// name of same
	Rect subRect;					/* rectangle for the sub screens */
	PersHandle pers;		// the personality for this settings screen */
	PersHandle lastPers;	// Last selected personality
	StackHandle groupStack;	// stack of dynmically-created group boxes
} SettingsGlobals, *SGPtr, **SGHandle;

SGHandle SG;

#define Dlog (*SG)->dlog
#define List (*SG)->list
#define DitlList (*SG)->ditlList
#define PrefList (*SG)->prefList
#define BitList (*SG)->bitList
#define ValList (*SG)->valList
#define CurrentScreen (*SG)->currentScreen
#define CurrentName (*SG)->currentName
#define SubRect (*SG)->subRect
#define Pers	(*SG)->pers
#define LastPers	(*SG)->lastPers
#define GroupStack	(*SG)->groupStack

typedef enum {setOKItem=1,setCancelItem,setListItem,setHiddenText,setHelp,setBaseLimit} SettingsItem;
	Boolean BuildSettings(void);
	Boolean BuildSettingsList(void);
	void BuildPrefList(Boolean disabled);
	void SettingsUpdate(MyWindowPtr win);
	Boolean SettingsFilter(MyWindowPtr win, EventRecord *event);
	void SettingsScreen(short startingWith,Boolean opening);
	Boolean SettingsHit(EventRecord *event,DialogPtr dlog,short item,long dialogRefcon);
	void CloseSettings(void);
	Boolean CloseSettingsWindow(MyWindowPtr win);
	void SettingsSelectAct(void);
	OSErr SaveSettingsScreen(void);
	void SettingsWebHelp(void);
	Point SettingsDITLToCell(short ditlId);
	short SettingsCellToDITL(Point cell);
	void BuildSubRect(void);
	OSErr SetPrefFromControl(short pref,ControlHandle cntl);
	void SetControlFromPrefLo(ControlHandle cntl,short pref,Boolean set);
#define SetControlFromPref(c,p) SetControlFromPrefLo(c,p,false)
	PersHandle SettingsPers(void);
	void SettingsListFocus(Boolean how);
	OSErr HitIMAPItem(short which,short item);
	short SwapDITL(short ditl, short pref);
	Boolean SettingsScrollWheel(MyWindowPtr win,Point pt,short h,short v);

/**********************************************************************
 * DoSettings - set settings the new way
 **********************************************************************/
void DoSettings(SettingsDITLEnum startingWith)
{
	if ((MainEvent.modifiers==(optionKey|controlKey|shiftKey|cmdKey)) &&
			ComposeStdAlert(Caution,RESET_PREFS_WARN)==kAlertStdAlertOKButton)
		{ResetSettings();return;}
	if (BuildSettings())
		SettingsScreen(startingWith,True);
	else
		CloseSettings();
}

/************************************************************************
 * ResetSettings - set all but a handful of settings back to basics
 ************************************************************************/
void ResetSettings(void)
{
	PersHandle oldCur = CurPers;
	Accumulator a;
	static short ids[] = {PREF_STRN,PREF2_STRN,PREF3_STRN,PREF4_STRN,0};
	short *id;
	
	
	// first, do dominant
	CurPers = PersList;
	SaveSacredSettings(&a);
	PersZapResources('STR ',0);
	for (id=ids;*id;id++)
	{
		ZapResourceLo('STR#',*id,true);
		ResourceCpy(SettingsRefN,AppResFile,'STR#',*id);
		SCClear(-1);	// kill all the cached values
	}
	RestoreSacredSettings(&a);
	
	// apres nous, le deluge
	for (CurPers=(*CurPers)->next;CurPers;CurPers=(*CurPers)->next)
	{
		SaveSacredSettings(&a);
		PersZapResources('STR ',(*CurPers)->resEnd);
		RestoreSacredSettings(&a);
	}
	
	// and now, quit
	InvalidatePasswords(false,false,true);
	UpdateResFile(SettingsRefN);
	CurPers = oldCur;
	Done = EjectBuckaroo = true;
}

/************************************************************************
 * SaveSacredSettings - save sacred settings into an accumular
 ************************************************************************/
OSErr SaveSacredSettings(AccuPtr a)
{
	Str255 list;
	Str31 one;
	UPtr spot;
	long id;
	Str255 val;
	OSErr err = noErr;
		
	AccuInit(a);
	GetRString(list,SACRED_PREFS);

	NoDominant++;	// be sure to do this AFTER fetching the sacred prefs
								// lest bug 2925 return from the dead.

	spot = list+1;
	while (!err && PToken(list,one,&spot,","))
	{
		StringToNum(one,&id);
		if (*GetPref(val,id))
		if (!(err=AccuAddPtr(a,one,*one+1)))
			err = AccuAddPtr(a,val,*val+1);
	}
	
	NoDominant--;
	AccuTrim(a);
	return(err);
}	

/************************************************************************
 * RestoreSacredSettings - put settings back on the sacred list
 ************************************************************************/
OSErr RestoreSacredSettings(AccuPtr a)
{
	long offset;
	Str255 val;
	long id;
	
	for (offset=0;offset<a->offset;)
	{
		PCopy(val,(*a->data)+offset);
		offset += *val+1;
		StringToNum(val,&id);
		
		PCopy(val,(*a->data)+offset);
		offset += *val+1;
		
		SCClear(id);
		SetPref(id,val);
	}
	AccuZap(*a);
	return(noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
PersHandle SettingsPers(void)
{
	return(Pers);
}

/************************************************************************
 * SettingsTEActivate - activate a text field, carefully
 ************************************************************************/
void SettingsTEActivate(MyWindowPtr dlogWin,Boolean activate)
{
	DialogPtr	theDialog = GetMyWindowDialogPtr(dlogWin);
	SAVE_STUFF;

	SetBGGrey(0xffff);
	if (dlogWin->isActive && activate)
		TEActivate(GetDialogTextEditHandle(theDialog));
	else
		TEDeactivate(GetDialogTextEditHandle(theDialog));
	REST_STUFF;
}


#ifdef SETTINGSLISTKEY
/************************************************************************
 * SettingsListFocus - draw the focus ring for settings
 ************************************************************************/
void SettingsListFocus(Boolean how)
{
	Rect r;

	r = (*List)->rView;
	r.right += GROW_SIZE;
	DrawThemeFocusRect(&r,how);
}

pascal short SettingsSearchProc (Ptr cellDataPtr, Ptr searchDataPtr,short cellDataLen, short searchDataLen)
{
	short *resId;
	Str255 name;
		
	resId = (short *) cellDataPtr;
	GetResName(name,'DITL',*resId);
	return (IdenticalText(name + 1, searchDataPtr, searchDataLen, searchDataLen, nil));
}


Boolean SettingsKey (MyWindowPtr dlogWin, EventRecord *event)	
{
	DialogPtr	theDialog = GetMyWindowDialogPtr(dlogWin);
	short c = event->message & charCodeMask;

	if (c == tabChar)
	{
		Boolean listFocus = (*SG)->listFocus,
						shift;

		shift = (event->modifiers & shiftKey) != 0;
		
		if (listFocus)
		{
			SettingsListFocus((*SG)->listFocus=false);
		}
		else
		{
		// if shifting to list
			if ((!shift && GetDialogKeyboardFocusItem(theDialog) == (*SG)->lastTextItem) ||
					(shift && GetDialogKeyboardFocusItem(theDialog) == (*SG)->firstTextItem))
			{
				SettingsListFocus((*SG)->listFocus=true);
				SelectDialogItemText(GetMyWindowDialogPtr(Dlog),setHiddenText,0,0);
				return(true);
			}
		}
	}
	return false;
}


Boolean SettingsListKey (MyWindowPtr win, EventRecord *event)	
{
	short c = event->message & charCodeMask;
	DECLARE_UPP(SettingsSearchProc,ListSearch);

	if ((*SG)->listFocus && c!=tabChar)
	{
		Cell theSelection,
					lastCell,
					searchCell;
		Boolean cmd = false;
		short page = RoundDiv((*List)->rView.bottom-(*List)->rView.top,(*List)->cellSize.v)-1;

		cmd = (event->modifiers & cmdKey) != 0;
				
		lastCell.v = lastCell.h = 0;
		while (LNextCell(false, true, &lastCell, List));
		theSelection.v = SettingsDITLToCell(CurrentScreen).v;
		theSelection.h = 0;
		
		switch (c) {
			case leftArrowChar:
			case upArrowChar:
				if (cmd)
				{
					if (theSelection.v != (*List)->visible.top)
						theSelection.v = (*List)->visible.top;
					else
						theSelection.v -= page;
				}
				else
					theSelection.v--;
				if (theSelection.v < 0)
					theSelection.v = 0;
				break;
			case rightArrowChar:
			case downArrowChar:
				if (cmd)
				{
					if (theSelection.v != ((*List)->visible.bottom - 1))
						theSelection.v = (*List)->visible.bottom - 1;
					else
						theSelection.v += page;
				}
				else
					theSelection.v++;
				if (theSelection.v > lastCell.v)
					theSelection.v = lastCell.v;
				break;
			default:
				if (cmd) return(false);	// don't do type-2-select for cmd keys
				searchCell.v = searchCell.h = 0;
				INIT_UPP(SettingsSearchProc,ListSearch);
				if (LSearch(&Type2SelString[1], Type2SelString[0], SettingsSearchProcUPP, &searchCell, List))
					theSelection.v = searchCell.v;
				break;
		}
		SelectSingleCell(theSelection.v,List);
		if (FirstSelected(&theSelection,List))
			SettingsSelectAct();
		else
			SelectSingleCell(SettingsDITLToCell(CurrentScreen).v,List);
		event->what = nullEvent;
//		SetCursorByLocation();	// dunno why the stupid cursor is the watch
		SetMyCursor(arrowCursor);
		return (true);
	}
	return (false);
}


void SettingsActivate(MyWindowPtr win)
{
	SettingsListFocus(win->isActive && (*SG)->listFocus);
	LActivate(win->isActive,List);
	
	// For whatever reason, OSUX fails to do the right thing
	// when activating the list after the window was
	// obscured.  Force the issue.
	if (win->isActive && HaveTheDiseaseCalledOSX())
	{
		Rect rView;
		RgnHandle myRgn = NewRgn();
		
		if (myRgn)
		{
			GetListViewBounds(List,&rView);
			RectRgn(myRgn,&rView);
			LUpdate(myRgn,List);
			DisposeRgn(myRgn);
		}
	}
}
#endif

/**********************************************************************
 * BuildSettings - build the settings dialog
 **********************************************************************/
Boolean BuildSettings(void)
{
	DialogPtr	DlogDP;
	Handle grumble;
	
	if (SG=NewZH(SettingsGlobals))									/* allocated globals */
	{
		if (grumble = NuHandle(0))  /* allocate preflist */
		{
			PrefList = (void*)grumble;
			if (grumble = NuHandle(0))
			{
				BitList = (void*)grumble;
				if (grumble = NuHandle(0))
				{
					ValList = (void*)grumble;
					SetDialogFont(SmallSysFontID());
					grumble = (void*)GetNewMyDialog(setDITLBase,nil,nil,InFront);
					SetDialogFont(0);
					if (grumble)  /* allocate dialog */
					{
						Dlog = (void*)grumble;
						DlogDP = GetMyWindowDialogPtr (Dlog);
						ConfigFontSetup(Dlog);
						if (!StackInit(sizeof(ControlHandle),&grumble)) GroupStack = grumble;
						if (BuildSettingsList())
						{
							WindowPtr	dlogWP = GetDialogWindow(DlogDP);
							SetPort_(GetWindowPort(dlogWP));
#ifdef SETTINGSLISTKEY							
							Dlog->key = SettingsKey;	
							(*SG)->listFocus = true;	
#endif
							Dlog->hit = SettingsHit;
							Dlog->close = CloseSettingsWindow;
							Dlog->position = PositionPrefsTitle;
							Dlog->cursor = DlgCursor;
							Dlog->help = SettingsHelp;

							ShowMyWindow(dlogWP);
#ifdef SETTINGSLISTKEY							
							Dlog->activate = SettingsActivate;
#endif
							Dlog->update = SettingsUpdate;
							Dlog->filter = SettingsFilter;
							Dlog->scrollWheel = SettingsScrollWheel;
#ifdef NEVER
{
	Rect r;
	short type;
	Handle h;
	
	GetDialogItem(DlogDP,setOKItem,&type,&h,&r);
	NewControl(GetDialogWindow(Dlog), &r, "", true, 0,0,0, DEBUG_CDEF<<4, 0);
}
#endif
							
							PushModalWindow(dlogWP);
							EnableMenus(dlogWP,False);
							BuildSubRect();
							Pers = PersList;
							return(True);
						}
					}
				}
			}
		}
	}
	WarnUser(COULDNT_PREF,MemError());
	return(False);
}
#pragma segment SettingDialog

/**********************************************************************
 * BuildSubRect - build the rectangle that holds the subscreens.
 **********************************************************************/
void BuildSubRect(void)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr (Dlog);
	short type;
	Handle resH;
	Rect r;
	
	SubRect.top = 0;
	GetDItem_(DlogDP,setListItem,&type,&resH,&r);
	SubRect.left = r.right+6;
	SubRect.right = Dlog->contR.right;
	GetDItem_(DlogDP,setOKItem,&type,&resH,&r);
	SubRect.bottom = r.top -4;
}

/**********************************************************************
 * SettingsFilter - filter events for the settings window
 **********************************************************************/
Boolean SettingsFilter(MyWindowPtr win, EventRecord *event)
{
	Point pt;
	Rect rView;
#ifdef SETTINGSLISTKEY
	Boolean listFocus = (*SG)->listFocus;	
#endif

	rView = (*List)->rView;
	rView.right += GROW_SIZE;
	
	switch(event->what)
	{
#ifdef SETTINGSLISTKEY
#if 0
		case activateEvt:	
			SettingsActivate(win);
			event->what = nullEvent;
			break;
#endif
		case keyDown:
		case autoKey:
			return(SettingsListKey(win,event));
			break;
		case app1Evt:	// pageup, pagedown, home, end
			if ((*SG)->listFocus)
				ListApp1(event, List);
			break;
#endif
		case updateEvt:
			UpdateMyWindow(GetMyWindowWindowPtr(win));
			break;
		case mouseDown:
			pt = event->where;
			GlobalToLocal(&pt);
			if (PtInRect(pt,&rView))
			{
				SettingsListFocus((*SG)->listFocus=true);
				SelectDialogItemText(GetMyWindowDialogPtr(win),setHiddenText,0,0);
				LClick(pt,event->modifiers,List);
				if (FirstSelected(&pt,List))
					SettingsSelectAct();
				else
					SelectSingleCell(SettingsDITLToCell(CurrentScreen).v,List);
				event->what = nullEvent;
			}
#ifdef SETTINGSLISTKEY
			else
				SettingsListFocus((*SG)->listFocus=false);
#endif
			break;
	}
#ifdef SETTINGSLISTKEY
	return ((*SG)->listFocus);
#else
	return (false);
#endif
}

/**********************************************************************
 * SettingsHit - hit a dialog item in a settings window
 **********************************************************************/
Boolean SettingsHit(EventRecord *event,DialogPtr dlog,short item,long dialogRefcon)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr(Dlog);
	short pref = 0;
	Str255 name;
	Str63 volpath;
	short type;
	Rect r;
	short screen;
	ControlHandle cntl;
	MyWindowPtr dlogWin = GetDialogMyWindowPtr(dlog);
	long dlogId = dlogWin->dialogID;
	
	if (item==1)
	{
#ifdef TWO
		if (event->what==keyDown && (event->message&charCodeMask)==returnChar)
		{
			pref = (*PrefList)[GetDialogKeyboardFocusItem(dlog)-setBaseLimit];
			if (pref && !(pref%100) && pref!=PERS_PREF)
			{
				TEKey(returnChar,GetDialogTextEditHandle(dlog));
				return(False);
			}
		}
#endif
		if (SaveSettingsScreen()) return(True);
	}
	
	if (item==setHelp)
		SettingsWebHelp();
	else if (item<=2) CloseSettings();
	else
	{
		if (!DialogRadioButtons(dlogWin,item))
			DialogCheckbox(dlogWin,item);
		if (item>=setBaseLimit)
		{
			pref = (*PrefList)[item-setBaseLimit];
			switch (pref)
			{
				case PERS_PREF:
					if (HasFeature (featureMultiplePersonalities))
						HitPersItem((*BitList)[item-setBaseLimit],item);
					break;
				case PREF_ERROR_SOUND:
				case PREF_NEWMAIL_SOUND:
					GetDIPopup(DlogDP,item,name);
					PlayNamedSound(name);
					break;
				case PREF_CREATOR:
					NewCreator(item);
					break;
				case PREF_AUTOHEX_NAME:
					GetPref(volpath,PREF_AUTOHEX_VOLPATH);
					if (GetHexPath(name,volpath))
					{
						GetDItem_(DlogDP,item,&type,&cntl,&r);
						SetControlTitle(cntl,name);
						SetPref(PREF_AUTOHEX_NAME,name);
						SetPref(PREF_AUTOHEX_VOLPATH,volpath);
					}
					break;				
				case PrivColorsStrn+1:
				case PrivColorsStrn+2:
				case PrivColorsStrn+3:
				case PrivColorsStrn+4:
				case PrivColorsStrn+5:
				case PrivColorsStrn+6:
				case PrivColorsStrn+7:
				case PrivColorsStrn+8:
				case TEXT_COLOR:
				case BACK_COLOR:
				case QUOTE_COLOR:
				case SPELL_COLOR:
				case LINK_COLOR:
				case NICK_COLOR:
				case URL_COLOR:
				case STAT_CURRENT_COLOR:
				case STAT_PREVIOUS_COLOR:
				case STAT_AVERAGE_COLOR:
				case MOOD_H_COLOR:
				case MOOD_COLOR:
					GetDItem_(DlogDP,item,&type,&cntl,&r);
					dlogWin->isDirty = ColCtlPicker(cntl) || dlogWin->isDirty;
					break;
				case PREF_MF_SUBJECT_DEFAULT_FOLDER:
					GetPref(volpath,PREF_MF_SUBJECT_DEFAULT_FOLDER);
					if (ChooseMailboxFolder(MAILBOX_MENU,CHOOSE_MBOX_FOLDER,name))
					{
						GetDItem_(DlogDP,item,&type,&cntl,&r);
						SetControlTitle(cntl,name);
						SetPref(PREF_MF_SUBJECT_DEFAULT_FOLDER,name);
					}
					break;
				case PREF_MF_FROM_DEFAULT_FOLDER:
					GetPref(volpath,PREF_MF_FROM_DEFAULT_FOLDER);
					if (ChooseMailboxFolder(MAILBOX_MENU,CHOOSE_MBOX_FOLDER,name))
					{
						GetDItem_(DlogDP,item,&type,&cntl,&r);
						SetControlTitle(cntl,name);
						SetPref(PREF_MF_FROM_DEFAULT_FOLDER,name);
					}
					break;
				case PREF_MF_ANYR_DEFAULT_FOLDER:	
					GetPref(volpath,PREF_MF_ANYR_DEFAULT_FOLDER);
					if (ChooseMailboxFolder(MAILBOX_MENU,CHOOSE_MBOX_FOLDER,name))
					{
						GetDItem_(DlogDP,item,&type,&cntl,&r);
						SetControlTitle(cntl,name);
						SetPref(PREF_MF_ANYR_DEFAULT_FOLDER,name);
					}
					break;
				case LOAD_SETTINGS_NOW:
					SaveSettingsScreen();
					if (!ACAPLoad(False))
					{
						screen = CurrentScreen;
						CurrentScreen = 0;
						SettingsScreen(screen,False);
					}
					SFWTC = True;
					break;
				case RESET_STATS_NOW:
					if (AlertStr(RESET_STATS_ALRT,Caution,nil)==1)
						ResetStatistics();
					break;
					
				case PREF_IS_IMAP:
				case -PREF_IS_IMAP:
					HitIMAPItem(pref,item);
					break;
			}
		}
		dlogWin->isDirty = True;
	}			
	
	// Audit pref hits, but skip key presses.  Too much of a good thing, IMO.
	if (event && event->what!=keyDown)
		AuditHit((event->modifiers&shiftKey)!=0, (event->modifiers&controlKey)!=0, (event->modifiers&optionKey)!=0, (event->modifiers&cmdKey)!=0, false, dlogId, AUDITCONTROLID(dlogId,ABS(pref)), event->what);
			
	return(True);	
}

/**********************************************************************
 * SettomgsScrollWheel - handle scroll wheel events for settings panels list
 **********************************************************************/
Boolean SettingsScrollWheel(MyWindowPtr win,Point pt,short h,short v)
{
	Rect  rList;

	GetListViewBounds (List, &rList);
	if (PtInRect(pt,&rList))
	{
		LScroll(0,-v,List);
		return true;
	}
	return false;
}

/**********************************************************************
 * CloseSettings - dispose of the settings block
 **********************************************************************/
void CloseSettings(void)
{
	if (SG)
	{
		SetPrefLong(PREF_LAST_SCREEN,CurrentScreen);
		if (Dlog)
		{
			CloseMyWindow(GetMyWindowWindowPtr(Dlog));
			RefreshAppearance();
		}
		else
		{
			if (List) LDispose(List);
			ZapHandle(DitlList);
			ZapHandle(PrefList);
			ZapHandle(BitList);
			ZapHandle(ValList);
			ZapHandle(GroupStack);
			ZapHandle(SG);
		}
#ifdef REFRESH_LABELS_MENU
		RefreshLabelsMenu();
#endif // REFRESH_LABELS_MENU
	}
}

/**********************************************************************
 * CloseSettingsWindow - close the window
 **********************************************************************/
Boolean CloseSettingsWindow(MyWindowPtr win)
{
	// if at least one IMAP personality exists, make sure there's a cache folder
	if (IMAPExists()) CreateNewPersCaches();

	Dlog = nil;
	CloseSettings();
	return(True);
}

/**********************************************************************
 * NullUserDraw - draw a useritem (not really; just a stub)
 **********************************************************************/
pascal void NullUserDraw(DialogPtr dlog, short item)
{
}

/**********************************************************************
 * BuildSettingsList - build the list of settings possibilities
 **********************************************************************/
Boolean BuildSettingsList(void)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr (Dlog);
	Str255 resName;
	short resId;
	short resIndex;
	Handle resH;
	ResType resType;
	short count = 0;
	Handle h;
	Rect bounds, view;
	Point c;
	short type;
	short standardEnds;
	DECLARE_UPP(SettingsListDef,ListDef);
	DECLARE_UPP(NullUserDraw,UserItem);
	
	INIT_UPP(SettingsListDef,ListDef);
	INIT_UPP(NullUserDraw,UserItem);
	/*
	 * allocate list for id's
	 */
	if (!(h = NuHandle(0))) return(False);
	DitlList = (void*)h;
	
	/*
	 * look at the list item to figure out how big the list should be
	 */
	GetDItem_(DlogDP,setListItem,&type,&resH,&view);
	SetDialogItem(DlogDP,setListItem,type,(Handle)NullUserDrawUPP,&view);
	
	/*
	 * allocate the list handle
	 */
	SetRect(&bounds,0,0,1,0);
	view.right -= GROW_SIZE;
	c.v = GetRLong(SETTINGS_CELL_SIZE);
	view.bottom = view.top + c.v*((view.bottom-view.top)/c.v);
	c.h = 0;
	SetRect(&bounds,0,0,1,0);


	if (!(h = (void*)CreateNewList(SettingsListDefUPP,ICON_LDEF,&view,&bounds,c,GetDialogWindow(DlogDP),True,False,False,True)))
		return(False);

	List = (void*)h;
	(*List)->selFlags = lOnlyOne|lNoNilHilite;
	(*List)->indent.v = 4;
	(*List)->indent.h = 4;
	
	/*
	 * look for appropriate resources
	 */
	 
	/*
	 * first, let's do the standard ones
	 */
	c.v = c.h = 0;
	for (standardEnds=setDITLBase+1;;standardEnds++)
	{
		SetResLoad(False);
		resH = GetResource('DITL',standardEnds);
		SetResLoad(True);
		if (!resH) break;

		GetResInfo(resH,&resId,&resType,resName);

		/* add it to our list of id's */
		PtrPlusHand_(&resId,DitlList,sizeof(short));
		
		/* add it to our scrolling list of names */
		LAddRow(1,c.v,List);
		LSetCell((Ptr)&resId,sizeof(short),c,List);
		c.v++;
	}
		
	for (resIndex=1;;resIndex++)
	{
		/*
		 * get the next resource
		 */
		SetResLoad(False);
		resH = GetIndResource('DITL',resIndex);
		SetResLoad(True);
		if (!resH) break;
		
		/*
		 * is it in the right range?
		 */
		GetResInfo(resH,&resId,&resType,resName);
		if (standardEnds<=resId && resId<setDITLLimit)
		{
			/* add it to our list of id's */
			PtrPlusHand_(&resId,DitlList,sizeof(short));
			
			/* add it to our scrolling list of names */
			LAddRow(1,c.v,List);
			LSetCell((Ptr)&resId,sizeof(short),c,List);
			c.v++;
		}
	}
	return(True);
}

/**********************************************************************
 * SettingsUpdate - update the settings window
 **********************************************************************/
void SettingsUpdate(MyWindowPtr win)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Rect r;
	
	SetPort_(winPort);
	SetSmallSysFont();
	LUpdate(MyGetPortVisibleRegion(winPort),List);
	r = (*List)->rView;
	InsetRect(&r,-1,-1);
	FrameRect(&r);
	InsetRect(&r,1,1);
	r.right += GROW_SIZE;
	SetSmallSysFont();
	UpdateDialog(GetMyWindowDialogPtr(win),winPort->visRgn);
#ifdef SETTINGSLISTKEY	
	SettingsListFocus((*SG)->listFocus);
#endif
}

/**********************************************************************
 * SettingsScreen - display a particular screen
 **********************************************************************/
void SettingsScreen(short startingWith,Boolean opening)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr(Dlog);
	CGrafPtr	DlogDPPort = GetWindowPort(GetDialogWindow(DlogDP));
	short current;
	short ditlToSwap = SwapDITL(startingWith, PREF_IS_IMAP);
	Str255 name;
	Boolean hasFeature;
	
	if (!startingWith || 0>SettingsDITLToCell(startingWith).v) startingWith = setDITLBasic;
	
	/*
	 * save current
	 */
	if (CurrentScreen == startingWith) return;
	
	SetMyCursor(watchCursor); SFWTC = True;
	if (CurrentScreen && NEED_TO_SAVE && SaveSettingsScreen())
	{
		SelectSingleCell(SettingsDITLToCell(CurrentScreen).v,List);
		return;
	}

	CurrentScreen = 0;
	Dlog->isDirty = False;
	CurPers = Pers = PersList;
	
	/*
	 * turn off the drawing
	 */
	SetEmptyClipRgn(DlogDPPort);
	
	/*
	 * out with the old
	 */
	current = CountDITL(DlogDP);
	if (current>=setBaseLimit) ShortenDITL(DlogDP,current-setBaseLimit+1);
	SetHandleBig_(PrefList,0);
	SetHandleBig_(BitList,0);
	SetHandleBig_(ValList,0);
	
	// Clear old group boxes
	{
		ControlHandle cntl;
		
		while (!StackPop(&cntl,GroupStack)) DisposeControl(cntl);
	}
		
	/*
	 * in with the new
	 */
	SetDialogFont(SmallSysFontID());
	AppendDITLId(DlogDP,ditlToSwap?ditlToSwap:startingWith,overlayDITL);
	ChangeTEFont(GetDialogTextEditHandle(DlogDP),SmallSysFontID(),SmallSysFontSize());
	SetDialogFont(0);

	/*
	 * build pref list
	 */
	GetResName(name,'DITL',ditlToSwap?ditlToSwap:startingWith);
	PCopy(CurrentName,name);
	GetResNameAndStripFeature(name,'DITL',ditlToSwap?ditlToSwap:startingWith,&hasFeature);
	BuildPrefList(!hasFeature);
	SettingsHelp(Dlog);

	/*
	 * draw
	 */
	InfiniteClip(DlogDPPort);
	InvalWindowRect(GetDialogWindow(DlogDP),&SubRect);
	
	CurrentScreen = startingWith;
	SelectSingleCell((SettingsDITLToCell(startingWith)).v,List);
#ifdef SETTINGSLISTKEY
	if ((*SG)->listFocus)
	{
		Str255 s;

		SelectDialogItemText(DlogDP,setHiddenText,0,0);
		UpdateMyWindow(GetDialogWindow(DlogDP));
		if (setDITLBasic==startingWith && opening && !*GetPref(s,PREF_REALNAME))
		{
			SettingsListFocus((*SG)->listFocus=false);
			SelectDialogItemText(DlogDP,setBaseLimit,0,32000);
		}
	}
#endif // SETTINGSLISTKEY
}

/**********************************************************************
 * SettingsSelectAct - Act on a selection in the list
 **********************************************************************/
void SettingsSelectAct(void)
{
	Point c;
	
	if (FirstSelected(&c,List))
		SettingsScreen(SettingsCellToDITL(c),false);
}

/**********************************************************************
 * SaveSettingsScreen - save the current settings screen
 **********************************************************************/
OSErr SaveSettingsScreen(void)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr(Dlog);
	short item;
	short type;
	Handle itemH;
	short n = CountDITL(DlogDP);
	short pref;
	Rect r;
	short value;
	Str255 text;
	OSErr err=noErr;
	short bit;
	Str255 val;
	uLong current;
	PersHandle oldCur = CurPers;
	
	CurPers = Pers;
	NoProxify = true;
		
	for (item=setBaseLimit;!err && item<=n;item++)
	{
		pref = (*PrefList)[item-setBaseLimit];
		bit = (*BitList)[item-setBaseLimit];
		PCopy(val,(*ValList)[item-setBaseLimit]);
		if (pref)
		{
			GetDialogItem(DlogDP,item,&type,&itemH,&r);
			if (HasFeature (featureMultiplePersonalities) && pref==PERS_PREF) err = SavePersItem(bit,item,itemH);
			else 
			switch (type)
			{
				case editText:
					GetDIText(DlogDP,item,text);
					if (pref%100)
					{
						TrimWhite(text);
						TrimInitialWhite(text);
						CaptureHex(text,text);
						if (item<n && pref==PREF_STUPID_USER && (*PrefList)[item-setBaseLimit+1]==PREF_STUPID_HOST)
						{
							pref = 3;	// set account instead
							item++;		// skip host
							// Get the next value
							GetDIText(DlogDP,item,val);
							TrimWhite(val);
							TrimInitialWhite(val);
							CaptureHex(val,val);
							PCatC(text,'@');
							PSCat(text,val);
						}
						err = SetPrefWithError(pref,text);
					}
#ifdef TWO
					else if (pref)
						err = SetStrnFromDlog(Dlog,item,pref);
#endif
					if (err) SelectDialogItemText(DlogDP,item,0,REAL_BIG);
					break;
				case ctrlItem+btnCtrl:
					GetControlTitle((ControlHandle)itemH,text);
					err = SetPrefWithError(pref,text);
					break;
				case ctrlItem+chkCtrl:
				case ctrlItem+radCtrl:
					value = GetControlValue((ControlHandle)itemH);
					if (pref<0) {pref = -pref; value = 1-value;}
					if (bit>=0)
					{
						current = GetPrefLong(pref);
						if (value) current |= (1<<bit);
						else current &= ~(1<<bit);
						NumToString(current,text);
						err = SetPrefWithError(pref,text);
					}
					else if (*val)
					{
						if (value)
						{
							err = SetPrefWithError(pref,val);
						}
					}
					else
						err = SetPrefWithError(pref,value?YesStr:NoStr);
					break;
				case ctrlItem+resCtrl:
					err = SetPrefFromControl(pref,(ControlHandle)itemH);
					break;
			}
		}
	}
	
	Pers = CurPers;
	CurPers = oldCur;
	NoProxify = false;
	
	return(err);
}

/**********************************************************************
 * SettingsDITLToCell - find the list cell that contains a given DITL
 **********************************************************************/
Point SettingsDITLToCell(short ditlId)
{
	Point c;
	
	c.h = 0;
	c.v = (*List)->dataBounds.bottom;
	while (c.v--)
		if ((*DitlList)[c.v]==ditlId) break;
	return(c);
}

/**********************************************************************
 * SettingsCellToDITL - find the DITL id for a given cell
 **********************************************************************/
short SettingsCellToDITL(Point cell)
{
	return((*DitlList)[cell.v]);
}

/**********************************************************************
 * BuildPrefList - build the list of pref #'s for the current dialog
 **********************************************************************/
void BuildPrefList(Boolean disabled)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr (Dlog);
	short item;
	short type;
	short bit;
	Str127 val;
	Handle itemH;
	short n = CountDITL(DlogDP);
	short pref;
	Rect r;
	UPtr spot;
	Str255 text;
	Boolean setSelect = True;
	long value;
	UPtr colon;
	Boolean enabled;
	PersHandle oldCur = CurPers;
	extern void CopyExtraSigs(MenuHandle mh);
	ControlHandle cntl;
	ControlHandle outerGroupCntl=nil;
	
	CurPers = Pers;	// set current personality to settings personality
#ifdef SETTINGSLISTKEY
	(*SG)->lastTextItem = -1; 
	(*SG)->firstTextItem = -1; 
#endif // SETTINGSLISTKEY

	ConfigFontSetup(Dlog);
		
	NoProxify = true;
	
	for (item=setBaseLimit;item<=n;item++)
	{
		if (!GetDialogItemAsControl(DlogDP,item,&cntl))
			LetsGetSmall(cntl);
		GetDialogItem(DlogDP,item,&type,&itemH,&r);
		pref = 0;
		*val = 0;
		switch (type)
		{
			case editText:
				GetDIText(DlogDP,item,text);
				text[*text+1]=0;
				if (spot=PPtrFindSub("\p��",text+1,*text))
				{
					pref = Atoi(spot+2);
					enabled = disabled?false:PrefEnabled(pref);
					if (!enabled) SetDialogItem(DlogDP,item,statText,itemH,&r);
					text[0] = spot-text-1;
					if (pref%100 && pref!=PERS_PREF)
					{
						GetSetPref(text,pref);
						EscapeInHex(text,text);
						SetDIText(DlogDP,item,text);
					}
					else if (HasFeature (featureMultiplePersonalities) && pref==PERS_PREF)
					{
						colon = strchr(spot+2,':');
						bit = colon?Atoi(colon+1):-1;
						BuildPersItem(bit,item,itemH,&enabled);
						enabled=disabled?false:enabled;
					}
					else if (pref)
						FillWithStrn(Dlog,item,pref);
				}
				if (enabled && setSelect)
				{
					SelectDialogItemText(DlogDP,item,0,REAL_BIG);
					setSelect = False;
				}
#ifdef SETTINGSLISTKEY
				if (enabled)
				{
					if ((*SG)->firstTextItem == -1) 
						(*SG)->firstTextItem = item;
					(*SG)->lastTextItem = item;	
				}
#endif // SETTINGSLISTKEY
				if (disabled||!enabled) DeactivateControl(cntl);
				break;
			case ctrlItem+btnCtrl:
			case ctrlItem+chkCtrl:
			case ctrlItem+radCtrl:
				ReplaceControl(DlogDP,item,type,(ControlHandle*)&itemH,&r);
				/* fall through */
			case ctrlItem+resCtrl:
				GetControlTitle((ControlHandle)itemH,text);
				text[*text+1]=0;
				if (spot=PPtrFindSub("\p��",text+1,*text))
				{
					pref = Atoi(spot+2);
					enabled = disabled?false:PrefEnabled(pref);
					if (!enabled) HiliteControl((ControlHandle)itemH,255);
					colon = strchr(spot+2,':');
					bit = colon?Atoi(colon+1):-1;
					colon = strchr(spot+2,'=');
					if (colon) MakePStr(val,colon+1,strlen(colon+1));
					text[0] = spot-text-1;
					SetControlTitle((ControlHandle)itemH,text);
					if (HasFeature (featureMultiplePersonalities) && pref==PERS_PREF) 
					{
						BuildPersItem(bit,item,itemH,&enabled);
						enabled=disabled?false:enabled;
					}
					else
					{
						if (pref==PREF_SIGFILE)
						{
							CopyExtraSigs((MenuHandle)GetControlPopupMenuHandle((ControlHandle)itemH));
							SetControlMaximum((ControlHandle)itemH,CountMenuItems((MenuHandle)GetControlPopupMenuHandle((ControlHandle)itemH)));
						}
						if (type==ctrlItem+resCtrl)
							SetControlFromPrefLo((ControlHandle)itemH,pref,true);
						else if (type==ctrlItem+btnCtrl)
							SetControlTitle((ControlHandle)itemH,GetSetPref(text,pref));
						else
						{
							if (bit>=0)
							{
								value = pref<0?GetSetPrefLong(-pref):GetSetPrefLong(pref);
								value = (value&(1<<bit)) ? 1:0;
								if (pref<0) value = 1-value;
							}
							else if (*val)
							{
								value = StringSame(val,GetSetPref(text,pref));
								if (!value && val[0]==1 && val[1]=='0' && !*text) value = 1;
							}
							else
								value = pref<0?!PrefIsSetSet(-pref):PrefIsSetSet(pref);
							SetControlValue((ControlHandle)itemH,value);
						}
					}
				}
				break;
		}
		PtrPlusHand_(&pref,PrefList,sizeof(short));
		PtrPlusHand_(&bit,BitList,sizeof(short));
		PtrPlusHand_(val,ValList,sizeof(val));
	}
	NoProxify = false;
	CurPers = oldCur;
	if (setSelect) 	SelectDialogItemText(DlogDP,0,0,0);	// found no edit fields; make sure dialog mgr understands that

	// Do the group box thang
	for (item=setBaseLimit;item<=n;item++)
	{
		GetDialogItem(DlogDP,item,&type,&itemH,&r);
		if ((type&~kItemDisableBit) == statText)
		{
			GetDIText(DlogDP,item,text);
			text[*text+1]=0;
			if (spot=PPtrFindSub("\p��",text+1,*text))
			{
				short endItem  = Atoi(spot+2);
				Rect endRect;
				Handle endItemH=nil;
				short endType;
				ControlHandle groupBoxCntl;
				short rightSide = 0;
				ControlHandle itemCntl=nil;
				short embedItem;
				Boolean small;
				
				// if endItem is large, it means to use a secondary box
				small = endItem > 1000;
				if (small) endItem -= 1000;
				
				// if endItem is negative, it means to go to the right side of the dialog
				if (endItem<0)
				{
					endItem *= -1;
					rightSide = Dlog->contR.right - 2*INSET;
				}
				endItem += setBaseLimit - 1;
				
				// trim off the item indicator
				text[0] = spot-text-1;
				
				// get the last item we should enclose
				GetDialogItem(DlogDP,endItem,&endType,&endItemH,&endRect);
				ASSERT(endItemH);
				if (!endItemH) continue; // oops!
								
				// Adjust the rectangle
				if (endItem != item)
				{
					r.bottom = endRect.bottom + (small ? INSET/2 : 3*INSET/2);
					r.right = rightSide ? rightSide : endRect.right + 3*INSET/2;
				}
				
				// Make a group control
				groupBoxCntl = NewControl(GetMyWindowWindowPtr(Dlog), &r, text, true, 0,0,0, small ? kControlGroupBoxSecondaryTextTitleProc+kControlUsesOwningWindowsFontVariant:kControlGroupBoxTextTitleProc, 0);
				if (!groupBoxCntl) continue; // oops!
				
				// Is this an outer group box?
				if (!small) outerGroupCntl = groupBoxCntl;
				else if (outerGroupCntl) EmbedControl(groupBoxCntl,outerGroupCntl);
				
				// Ok, believe me, this is gonna work.  Oh yeah.
				
				// grab the old control
				GetDialogItemAsControl(DlogDP,item,(ControlHandle*)&itemCntl);
				
				// Embed all the subcontrols
				for (embedItem=item+1;embedItem<=endItem;embedItem++)
				{
					endItemH = nil;
					GetDialogItemAsControl(DlogDP,embedItem,(ControlHandle*)&endItemH);
					if (endItemH) EmbedControl((ControlHandle)endItemH,groupBoxCntl);
				}
				
				// Empty the old item, but don't destroy it, because the dialog
				// manager gets really schizo if you do that
				SetDIText(DlogDP,item,"");
								
				// Remember!
				StackPush(&groupBoxCntl,GroupStack);
			}
		}
	}
}

/**********************************************************************
 * SavePersItem - save an item for a personality pref item
 **********************************************************************/
OSErr SavePersItem(short which,short item,Handle itemH)
{
	Str255 old, new;

	//No personalities settings screen in light
	if (HasFeature (featureMultiplePersonalities))	
		switch(which)
		{
			case PERS_NAME:
				if (Pers!=PersList)	// don't do for dominant
				{
					PSCopy(old,(*Pers)->name);
					GetDIText(GetMyWindowDialogPtr(Dlog),item,new);
					if (EqualString(old,new,true,true)) return noErr;

					PushPers(Pers);
					if (Aprintf(YES_CANCEL_ALRT,Stop,PERS_RENAME,(*Pers)->name)!=1)
					{
						SetDIText(GetMyWindowDialogPtr(Dlog),item,(*Pers)->name);
						PopPers();
						return userCanceledErr;
					}
					PopPers();

					if (IsIMAPPers(Pers))
						if (!StringSame(old, new))
							if (!RenameIMAPPers(Pers,new))
								break;

					PersSetName(Pers,new);
					BuildPersMenu();
					NotifyPersonalitiesWin();
				}
				break;
		}
	return(noErr);
}


int ShowSimpleModalDialog ( int resID )
{
	int retVal = kStdOkItemIndex;
	MyWindowPtr	dlogWin;
	DialogPtr	dlog;
	WindowPtr	dlogWP;
	short res;
	extern ModalFilterUPP DlgFilterUPP;
	
	dlogWin = GetNewMyDialog ( resID ,nil,nil,InFront );
	if ( NULL != dlogWin )
		{
		dlog = GetMyWindowDialogPtr (dlogWin);
		dlogWP = GetDialogWindow (dlog);
		SetPort_(GetWindowPort(dlogWP));
		StartMovableModal(dlog);
		ShowWindow(dlogWP);
		SetMyCursor(arrowCursor);

		do
		{
			MovableModalDialog ( dlog, DlgFilterUPP, &res );
		}
		while ( res > kStdCancelItemIndex );
	}

	EndMovableModal(dlog);
	DisposeDialog_(dlog);

	return retVal;
}
/**********************************************************************
 * HitPersItem - handle a hit on a personality item
 **********************************************************************/
extern pascal Boolean StdAlertFilter(DialogPtr dgPtr,EventRecord *event,short *item);
OSErr HitPersItem(short which,short item)
{
	PersHandle pers;
	Str255 scratch;
	short screen = CurrentScreen;
	
	//No personalities settings screen in light
	if (HasFeature (featureMultiplePersonalities))	
		switch(which)
		{
			case PERS_POPUP:
				GetDIPopup(GetMyWindowDialogPtr(Dlog),item,scratch);
				pers = EqualStrRes(scratch,DOMINANT) ? PersList : FindPersByName(scratch);
				if (pers==Pers) return(noErr);
				if (NEED_TO_SAVE) SaveSettingsScreen();
				if (!pers) LastPers=PersList;
				else LastPers = pers;
				CurrentScreen = 0;
				SettingsScreen(screen,false);
				break;
			
			case PERS_NEW:
				if (NEED_TO_SAVE) SaveSettingsScreen();
				if (HasFeature(featureMultiplePersonalities) && (pers=PersNew()))
				{
					LastPers = pers;
					CurrentScreen = 0;
					SettingsScreen(screen,false);
					NotifyPersonalitiesWin();
				}
				break;
			
			case PERS_REMOVE:
				if (IsIMAPPers(Pers))
				{
					if (ComposeStdAlert(Stop,IMAP_DELETE_CACHE,PCopy(scratch,(*Pers)->name))==1)
					{
						Dlog->isDirty = False;
						DeleteIMAPPers(Pers, false);
						PersDelete(Pers);
						LastPers = PersList;
						CurrentScreen = 0;
						SettingsScreen(screen,false);
						NotifyPersonalitiesWin();
					}
				}
				else
				{
		    	 	if (Aprintf(YES_CANCEL_ALRT,Caution,PERS_DELETE,PCopy(scratch,(*Pers)->name))==1)
		     		{
						Dlog->isDirty = False;
						PersDelete(Pers);
						LastPers = PersList;
						CurrentScreen = 0;
						SettingsScreen(screen,false);
						NotifyPersonalitiesWin();
					}
				}
				break;
			case PERS_VALIDATE:
				if (NEED_TO_SAVE) SaveSettingsScreen();
				{
				PersHandle oldPers = CurPers;
				Str255	host1, host2, user;
				long	port1, port2;
				OSErr	err1, err2;
				
			//	Set the current personality
				CurPers = Pers;
				
			//	Ok, here we are going to grab the POP (or IMAP) settings for the personality,
				if (IsIMAPPers(CurPers)) 
				{
					port1 = GetRLong(IMAP_PORT);
					if ( GetPrefLong(PREF_SSL_IMAP_SETTING) & esslUseAltPort )
						port1 = GetRLong(IMAP_SSL_PORT);
					GetPOPInfoLo ( user, host1, &port1 );
				}
				else
				{
					port1 = PrefIsSet(PREF_KERBEROS) ? GetRLong(KERB_POP_PORT) : GetRLong(POP_PORT);
					if ( GetPrefLong ( PREF_SSL_POP_SETTING ) & esslUseAltPort )
						port1 = GetRLong ( POP_SSL_PORT );
					GetPOPInfoLo ( user, host1, &port1 );
				}
						
			//	and attempt to connect to the server named in the settings.
				err1 = CheckConnectionSettings ( host1, port1, P2 );
				if ( noErr == err1 )
					GetPref ( P2, CHECK_CONNECTION_SUCCESS );
					
			//	Then we'll repeat with the SMTP settings
				port2 = GetSMTPPort ();
				if ( GetPrefLong ( PREF_SSL_SMTP_SETTING ) & esslUseAltPort )
					port2 = GetRLong ( SMTP_SSL_PORT );
				
				GetSMTPInfoLo ( host2, &port2 );
				err2 = CheckConnectionSettings ( host2, port2, P4 );
				if ( noErr == err2 )
					GetPref ( P4, CHECK_CONNECTION_SUCCESS );

			//	Show the results to the user
				ComposeRString ( P1, CHECK_CONNECTION_FORMAT, host1, port1 );
				ComposeRString ( P3, CHECK_CONNECTION_FORMAT, host2, port2 );
			//	ParamText ( summary1, err1Str, summary2, err2Str );
				
				ParamText ( P1, P2, P3, P4 );
				ShowSimpleModalDialog  ( CHECK_CONNECTION_DLOG );
			//	NoteAlert ( CHECK_CONNECTION_DLOG, nil );
				
			//	Restore original settings
				CurPers = oldPers;
				}
				break;

			default:
				break;			
			
		}
	return(noErr);
}

/**********************************************************************
 * BuildPersItem - build a pref for a personality pref item
 **********************************************************************/
OSErr BuildPersItem(short which,short item,Handle itemH,Boolean *enabled)
{
	DialogPtr	DlogDP = GetMyWindowDialogPtr (Dlog);
	MenuHandle mh;
	PersHandle pers;
	Str255 scratch;
	ControlHandle cntl;
	
	//No personalities settings screen in light
	if (HasFeature (featureMultiplePersonalities))	
		switch (which)
		{
			case PERS_POPUP:
				if (0<=Pers2Index(LastPers)) CurPers = Pers = LastPers;
				SetControlMaximum((ControlHandle)itemH,PersCount()+1);
				mh = (MenuHandle)GetControlPopupMenuHandle((ControlHandle)itemH);
				for (pers=(*PersList)->next;pers;pers=(*pers)->next)
				{
					MyAppendMenu(mh,PCopy(scratch,(*pers)->name));
					if (Pers==pers)
						SetControlValue((ControlHandle)itemH,CountMenuItems(mh));
				}
				break;
				
			case PERS_NEW:	break;
			
			case PERS_NAME:
				if (Pers==PersList)
				{
					SetDIText(DlogDP,item,GetRString(scratch,DOMINANT));
					GetDialogItemAsControl(DlogDP,item,&cntl);
					DeactivateControl(cntl);
					*enabled = False;
				}
				else
				{
					SetDIText(DlogDP,item,PCopy(scratch,(*Pers)->name));
					GetDialogItemAsControl(DlogDP,item,&cntl);
					ActivateControl(cntl);
				}
				break;
				
			case PERS_REMOVE:
				HiliteControl((ControlHandle)itemH,Pers==PersList ? 255:0);
				break;
		}
	return(noErr);
}

/**********************************************************************
 * SetPrefFromControl - read the value of a control, and set the pref
 * accordingly.
 **********************************************************************/
OSErr SetPrefFromControl(short pref,ControlHandle cntl)
{
	Str255 scratch;
	long value;
	AppData data;
	RGBColor color;
	uLong sigId;

	value = GetControlValue(cntl);
	
	switch (pref)
	{
		case PREF_FONT_NAME:
		case PREF_PRINT_FONT:
#ifdef USEFIXEDDEFAULTFONT
		case FIXED_FONT:
#endif
		case PREF_NEWMAIL_SOUND:
		case PREF_ERROR_SOUND:
		case PREF_PROXY:
			MyGetItem((MenuHandle)GetControlPopupMenuHandle(cntl),value,scratch);
			if (pref==PREF_PROXY && GetControlValue(cntl)<2) *scratch = 0;
			break;
		case PREF_SIGFILE:
			switch (value)
			{
				case sigmNone: sigId = SIG_NONE; break;
				case sigmStandard: sigId = SIG_STANDARD; break;
				case sigmAlternate: sigId = SIG_ALTERNATE; break;
				default:
					MyGetItem((MenuHandle)GetControlPopupMenuHandle(cntl),value,scratch);
					MyLowerStr(scratch);
					sigId = Hash(scratch);
					break;
			}
			NumToString(sigId,scratch);
			SetPrefWithError(pref,scratch);
			break;
		case PREF_RELAY_PERSONALITY:
		case CONCON_PREVIEW_PROFILE:
		case CONCON_MULTI_PREVIEW_PROFILE:
		case CONCON_MESSAGE_PROFILE:
		case STATIONERY:
			if (value==1)
				PCopy(scratch,NoStr);
			else
				MyGetItem((MenuHandle)GetControlPopupMenuHandle(cntl),value,scratch);
			SetPrefWithError(pref,scratch);
			break;
		case PREF_CREATOR:
			AppCdefGetData(cntl,&data);
			BMD(&data.cur.creator,scratch+1,4);
			*scratch = 4;
			break;
		case PrivColorsStrn+1:
		case PrivColorsStrn+2:
		case PrivColorsStrn+3:
		case PrivColorsStrn+4:
		case PrivColorsStrn+5:
		case PrivColorsStrn+6:
		case PrivColorsStrn+7:
		case PrivColorsStrn+8:
		case TEXT_COLOR:
		case BACK_COLOR:
		case QUOTE_COLOR:
		case SPELL_COLOR:
		case LINK_COLOR:
		case NICK_COLOR:
		case URL_COLOR:
		case STAT_CURRENT_COLOR:
		case STAT_PREVIOUS_COLOR:
		case STAT_AVERAGE_COLOR:
		case MOOD_H_COLOR:
		case MOOD_COLOR:
			Color2String(scratch,ColCtlGet(cntl,&color));
			break;
		default:
			NumToString(value,scratch);
			break;
	}
	
	return(SetPrefWithError(pref,scratch));
}

/**********************************************************************
 * TogglePref - toggle a pref; returns new value
 **********************************************************************/
Boolean TogglePref(short pref)
{
	Boolean result = !PrefIsSet(pref);
	
	if (result)
		SetPref(pref,YesStr);
	else
		SetPref(pref,"");
	
	return(result);
}


/**********************************************************************
 * SetControlFromPrefLo - set the value of a control from a pref
 **********************************************************************/
void SetControlFromPrefLo(ControlHandle cntl,short pref,Boolean set)
{
	Str255 scratch;
	long value;
	RGBColor color;
	uLong sigId;
	MenuHandle mh, mh2;
	long count;
	short i;
	Style style;

	NoProxify = true;
	GetPref(scratch,pref);
	NoProxify = false;
	if (set) SetPref(pref,scratch);
	
	switch (pref)
	{
		case PREF_FONT_NAME:
		case PREF_PRINT_FONT:
#ifdef USEFIXEDDEFAULTFONT
		case FIXED_FONT:
#endif
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			SetControlMaximum(cntl,CountMenuItems(mh));
			value = FindItemByName(mh,scratch);
			break;
		case PREF_NEWMAIL_SOUND:
		case PREF_ERROR_SOUND:
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			AddSoundsToMenu(mh);
			SetControlMaximum(cntl,CountMenuItems(mh));
			value = FindItemByName(mh,scratch);
			break;
		case PREF_PROXY:
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			ProxyMenu(mh);
			SetControlMaximum(cntl,CountMenuItems(mh));
			value = FindItemByName(mh,scratch);
			if (!value) value = 1;
			break;
		case PREF_SIGFILE:
			StringToNum(scratch,&sigId);
			switch (sigId)
			{
				case SIG_NONE: value = sigmNone; break;
				case SIG_STANDARD: value = sigmStandard; break;
				case SIG_ALTERNATE:	value = sigmAlternate; break;
				default:
					mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
					for (value=CountMenuItems(mh);value>sigmAlternate;value--)
					{
						MyGetItem(mh,value,scratch);
						MyLowerStr(scratch);
						if (Hash(scratch)==sigId) break;
					}
					break;
			}
			break;
		case PREF_RELAY_PERSONALITY:
		{
			PersHandle pers;
			Str63 persName;
			
			GetPref(persName,pref);
			
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			SetControlMaximum(cntl,PersCount()+2);
			MyAppendMenu(mh,GetRString(scratch,DOMINANT));
			i = CountMenuItems(mh);
			if (PrefIsSet(PREF_SMTP_AUTH_NOTOK)) DisableItem(mh,i);
			for (pers=(*PersList)->next;pers;pers=(*pers)->next)
			{
				PushPers(pers);
				MyAppendMenu(mh,PCopy(scratch,(*pers)->name));
				EnableIf(mh,++i,!PrefIsSet(PREF_SMTP_AUTH_NOTOK));
				PopPers();
			}
			count = CountMenuItems(mh);
			SetControlMaximum(cntl,count);
			if (mh)
				GetItemStyle(mh,1,&style);
			value = 1;
			if (count>1 || !style)
				for (i=1;i<=count;i++)
				{
					MyGetItem(mh,i,scratch);
					if (StringSame(scratch,persName))
						value = i;
				}
			break;
		}

		case CONCON_PREVIEW_PROFILE:
		case CONCON_MULTI_PREVIEW_PROFILE:
		case CONCON_MESSAGE_PROFILE:
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			ConConAddItems(mh);
			count = CountMenuItems(mh);
			SetControlMaximum(cntl,count+2);
			if (mh)
				GetItemStyle(mh,1,&style);
			value = 1;
			if (count>1 || !style)
				for (i=1;i<=count;i++)
				{
					MyGetItem(mh,i,scratch);
					if (EqualStrRes(scratch,pref))
						value = i;
				}
			break;
		case STATIONERY:
			mh = (MenuHandle)GetControlPopupMenuHandle(cntl);
			mh2 = GetMHandle(REPLY_WITH_HIER_MENU);
			count = mh2 ? CountMenuItems(mh2) : 0;
			SetControlMaximum(cntl,count+2);
			if (mh2)
				GetItemStyle(mh2,1,&style);
			value = 1;
			if (count>1 || !style)
				for (i=1;mh2 && i<=count;i++)
				{
					MyGetItem(mh2,i,scratch);
					MyAppendMenu(mh,scratch);
					if (EqualStrRes(scratch,STATIONERY)) value = i+2;
				}
			break;
		case PREF_CREATOR:
			BMD(scratch+1,&value,4);
			AppCdefSetIcon(cntl,0,'APPL',value,nil);
			SetControlTitle(cntl,GetPref(scratch,PREF_CREATOR_NAME));
			break;
		case PrivColorsStrn+1:
		case PrivColorsStrn+2:
		case PrivColorsStrn+3:
		case PrivColorsStrn+4:
		case PrivColorsStrn+5:
		case PrivColorsStrn+6:
		case PrivColorsStrn+7:
		case PrivColorsStrn+8:
		case TEXT_COLOR:
		case BACK_COLOR:
		case QUOTE_COLOR:
		case SPELL_COLOR:
		case LINK_COLOR:
		case NICK_COLOR:
		case URL_COLOR:
		case STAT_CURRENT_COLOR:
		case STAT_PREVIOUS_COLOR:
		case STAT_AVERAGE_COLOR:
		case MOOD_H_COLOR:
		case MOOD_COLOR:
			GetRColor(&color,pref);
			ColCtlSet(cntl,&color);
			return;
			break;
		default:
			StringToNum(scratch,&value);
			break;
	}
	
	SetControlValue(cntl,value);
}

/************************************************************************
 * NewCreator - allow the user to pick a new application
 ************************************************************************/
void NewCreator(short item)
{
	ControlHandle	cntl;
	SFTypeList		types;
	FInfo					fi;
	OSType				creator;
	FSSpec				spec;
	Str255				prompt;
	Str63					junk;
	OSErr					theError;
	Rect					r;
	Boolean				good;

	types[0] = 'APPL';
	types[1] = 'adrp';
	types[2] = 'appe';
	types[3] = 0;
	
	GetRString (prompt, PICK_CREATOR);
		theError = GetFileNav (types, 0, prompt, 0, true, &spec, &good, nil);
	if (good) {
		if (!(theError = FSpGetFInfo (&spec, &fi)))
			creator = fi.fdCreator;
		else
			// If FSpGetFInfo fails, chances are we chose a Mac OS X 
			if (HaveOSX ()) {
				LSItemInfoRecord	lsItem;
				FSRef							fRef;
				
				theError = FSpMakeFSRef (&spec, &fRef);
				if (!theError)
					theError = FSGetCatalogInfo (&fRef, kFSCatInfoNone, nil, nil, &spec, nil);
				if (!theError)
					theError = LSCopyItemInfoForRef (&fRef, kLSRequestTypeCreator, &lsItem);
				if (!theError)
					creator = lsItem.creator;
			}
		if (!theError) {
			GetDItem_ (GetMyWindowDialogPtr(Dlog), item, &theError, &cntl, &r);
			SetControlTitle (cntl, spec.name);
			SetPref (PREF_CREATOR_NAME, spec.name);
			BMD (&creator, junk + 1, 4);
			*junk = 4;
			SetPref (PREF_CREATOR, junk);
			AppCdefSetIcon (cntl, 0, 'APPL', creator, nil);
		}
		else
			FileSystemError (NO_FINFO, spec.name, theError);
	}
}


/************************************************************************
 * ZapSettingsResource - remove a resource from the settings file
 ************************************************************************/
OSErr ZapSettingsResource(OSType type, short id)
{
	short oldRes = CurResFile();
	OSErr err = resNotFound;
	
 	if (SettingsRefN)
 	{
 		UseResFile(SettingsRefN);
 		err = Zap1Resource(type,id);
 	}
 	UseResFile(oldRes);
 	return(err);
}

/**********************************************************************
 * SettingsHelp - do balloon help for the settings window
 **********************************************************************/
void SettingsHelp(MyWindowPtr dlogWin)
{
	short itemN;
	HMHelpContentRec hmr;
	DialogPtr dlog = GetMyWindowDialogPtr(dlogWin);
	short pref;
	Str255 help;
	ControlHandle cntl;
	OSErr err;

	hmr.version = kMacHelpVersion; 
	hmr.tagSide = kHMDefaultSide;
	hmr.content[kHMMinimumContentIndex].contentType = kHMPascalStrContent;
	hmr.content[kHMMaximumContentIndex].contentType = kHMNoContent; 
	Zero(hmr.absHotRect);
	
	for (itemN=1;;itemN++)
	{
		cntl = nil;
		GetDialogItemAsControl(dlog,itemN,&cntl);
		if (!cntl) break;
		if (itemN>=setBaseLimit)
		{
			if (pref = (*PrefList)[itemN-setBaseLimit])
			{
				pref = ABS(pref);
				PrefHelpString(pref,help,false);
				if (*help)
				{
					PSCopy(hmr.content[kHMMinimumContentIndex].u.tagString,help);
					err = HMSetControlHelpContent(cntl,&hmr);
					ASSERT(!err);
				}
			}
		}
	}
}

/**********************************************************************
 * DoPersSettings - open Personalities settings
 **********************************************************************/
void DoPersSettings(PersHandle pers)
{
	short	persResId=0;
	Handle	resH;
	Str255 scratch;
	
	if (BuildSettings())
	{
		//	Use the specified personality when opening the settings panel
		LastPers = pers;
		
		//	Get resource ID for personalities settings panel
		GetRString(scratch,PERSONALITIES_SETTING);
		SetResLoad(false);
		resH = GetNamedResource('DITL',scratch);
		SetResLoad(true);
		if (resH)
		{
			ResType	resType;
			
			GetResInfo(resH,&persResId,&resType,scratch);
			ReleaseResource(resH);
		}
		SettingsScreen(persResId,false);
	}
	else
		CloseSettings();	
}

#define	kMailHelper	"\pHelper�mailto"


OSErr SetDefaultMailClient ( ICInstance icp, OSType creator, ConstStr255Param theName ) {
	OSErr 		err = noErr;
	ICAppSpec	appSpec;
	ICAttr		attr = kICAttrNoChange;
	long sz = sizeof ( appSpec );
	
//	Copy from the app spec
	appSpec.fCreator = creator;
	BlockMoveData ( theName, appSpec.name, sizeof ( appSpec.name ));
	err = ICSetPref ( icp, (ConstStr255Param) kMailHelper, attr, (void *) &appSpec, sz );

	return err;
	}


OSErr GetDefaultMailClient ( ICInstance icp, OSType *creator, StringPtr theName ) {
	OSErr 		err = noErr;
	ICAppSpec	appSpec;
	ICAttr		attr;
	long sz = sizeof ( appSpec );
	
	err = ICGetPref ( icp, (ConstStr255Param) kMailHelper, &attr, (void *) &appSpec, &sz );

//	Copy from the app spec
	*creator = appSpec.fCreator;
	if ( theName != NULL )
		BlockMoveData ( appSpec.name, theName, appSpec.name [ 0 ] + 1 );
	
	return err;
	}

Boolean IsEudoraTheDefaultMailClient ( ICInstance icp ) {
	OSType creator;
	OSErr err = GetDefaultMailClient ( icp, &creator, NULL );
	return err == noErr && creator == CREATOR;
	}

OSErr SetEudoraAsTheDefaultMailClient ( ICInstance icp ) {
	return SetDefaultMailClient ( icp, CREATOR, "\pEudora" );
	}

/*
	Check to see if we are the default client.
	If we are, do nothing - we're happy.
	If we are not, and the user has set the preference "Make Eudora the default client",
		make it so.
	If we are not, and the user hasn't set that goodly preference, then ask them if
		they want that preference set - but only ask them once, ever.
*/
void CheckForDefaultMailClient () {
	ICInstance icp;
	OSErr err = noErr;
	
	err = ICStart ( &icp, '\?\?\?\?' );
	ASSERT ( noErr == err );
	
	if ( !IsEudoraTheDefaultMailClient ( icp )) {
		if ( PrefIsSet ( PREF_MAKE_EUDORA_DEFAULT_MAILER ))
			SetEudoraAsTheDefaultMailClient ( icp );
		else if ( !PrefIsSet ( PREF_HAVE_WE_ASKED_ABOUT_DEFAULT_MAILER )) {
		//	Ask the user about making us the default mail client	
			if ( kStdOkItemIndex == ComposeStdAlert(kAlertNoteAlert, DEFAULT_MAILER_Q )) {
				SetEudoraAsTheDefaultMailClient ( icp );
				SetPref ( PREF_MAKE_EUDORA_DEFAULT_MAILER, YesStr );
				}
		//	Remember that we asked, so that we don't ask again
			SetPref ( PREF_HAVE_WE_ASKED_ABOUT_DEFAULT_MAILER, YesStr );						
			}
		}

	err = ICStop ( icp );
	ASSERT ( noErr == err );
	}

/**********************************************************************
 * HitPersItem - handle a hit on a personality item
 **********************************************************************/
OSErr HitIMAPItem(short which,short item)
{
	short screen = CurrentScreen;
	Boolean	isIMAP = IsIMAPPers(Pers);
	Str255	scratch;
	PersHandle oldPers;
	
	switch (which)
	{
		case PREF_IS_IMAP:
			if (isIMAP) return noErr;	//	no change
			
			//	Converting POP to IMAP. Create local cache
			oldPers = CurPers;
			CurPers = Pers;
			TogglePref(PREF_IS_IMAP);
			CurPers = oldPers;
			break;
			
		case -PREF_IS_IMAP:
			if (!isIMAP) return noErr;	//	no change
			
			//	Converting IMAP to POP. Warn that cache will be deleted, then do it
			if (ComposeStdAlert(Stop,IMAP_TO_POP,PCopy(scratch,(*Pers)->name))==2)
			{			
				CurrentScreen = 0;
				SettingsScreen(screen,false);
				return noErr;	//	user aborted
			}
			
			// actually switch the pref now
			oldPers = CurPers;
			CurPers = Pers;
			TogglePref(PREF_IS_IMAP);
			// reset SpamWatch prefs
			IMAPResetSpamSupportPrefs();
			CurPers = oldPers;
			
			DeleteIMAPPers(Pers, false);
			break;
	}
	
	Dlog->isDirty = true;
	SaveSettingsScreen();
	CurrentScreen = 0;
	SettingsScreen(screen,false);
	
	// #4194 - force an update to remove any traces of the old screen -jdboyd 12/19/03
	UpdateMyWindow(GetDialogWindow(GetMyWindowDialogPtr(Dlog)));
	
	return(noErr);
}

/**********************************************************************
 * SwapDITL - return the DITL id of the panel we should display if
 *	pref is set, else return 0
 **********************************************************************/
short SwapDITL(short ditl, short pref)
{
	short swap = 0;
	Str255 resName, swapResName;
	Handle resH = 0;
	short resId;
	ResType resType;
	PersHandle oldPers = CurPers;
	
	Zero(resName);Zero(swapResName);
	
	// interested in the currently displayed personality
	CurPers = LastPers;
	
	// if the preference is set ...
	if (PrefIsSet(pref))
	{
		// what is the current DITL's name?
		SetResLoad(False);
		resH = GetResource('DITL',ditl);
		SetResLoad(True);
		
		if (resH && ResError() == noErr)
		{
			GetResInfo(resH,&resId,&resType,resName);
			ReleaseResource(resH);
			
			if (ResError() == noErr)
			{
				// find the 'DITL' with the same name + �pref
				ComposeString(swapResName,"\p%s�%d",resName+1,pref);
				SetResLoad(False);
				resH = GetNamedResource('DITL', swapResName);
				SetResLoad(True);
		
				if (resH && ResError() == noErr) 
				{
					GetResInfo(resH,&swap,&resType,resName);
					if (!swap || ResError() != noErr) swap = 0;
					ReleaseResource(resH);
				}
			}
		}
	}
	
	CurPers = oldPers;
	
	// and substitute it in the preference panel
	return (swap);
}	

/************************************************************************
 * SettingsWebHelp - send the user to the web for help
 ************************************************************************/
void SettingsWebHelp(void)
{
	Handle url = GenerateAdwareURL(GetNagState (), TECH_SUPPORT_SITE, actionHelpText, helpQuery, topicQuery);
	Accumulator a;
	Str63 name;
	Str31 keyword;
	
	if (url)
	{
		a.data = url;
		a.size = a.offset = GetHandleSize(url);
		a.offset--;	// get rid of null added by GenerateAdwareURL
		
		PSCopy(name,CurrentName);
		TransLitRes(name+1,*name,ktFlatten);
		AccuAddChar(&a,'&');
		AccuAttributeValuePair(&a,GetRString(keyword,SETTINGS_PANEL_KEYWORD),name);
		a.offset--;	// aavp adds one at the end
		
		AccuTrim(&a);
		if (!ParseProtocolFromURLPtr (LDRef (url), a.offset, keyword))
			OpenOtherURLPtr (keyword, *url, a.offset);
		ZapHandle(url);
	}	
}

/************************************************************************
 * GetSubmissionPort - return the submission port, but make sure it's not
 * something stupid, like "y" or "n"
 ************************************************************************/
long GetSubmissionPort(void)
{
	long port = GetRLong(SUBMISSION_PORT);
	if (!port) port = 587;	// per RFC
	return port;
}

/************************************************************************
 * GetSMTPPort - return the proper port for SMTP; maybe 25, maybe 587
 ************************************************************************/
long GetSMTPPort(void)
{
	// Note that there are other ways to get the port
	// If you say "alternate port SSL", that wins, not this.
	// If you use ":portnumber" on the end of the smtp host, that wins, not this.
	// But as you can see, this wins over just setting the SMTP port in general.
	return PrefIsSet(PREF_SUBMISSION_PORT) ? GetSubmissionPort(): GetRLong(SMTP_PORT);
}

/************************************************************************
 * ShouldSMTPAuth - should we attempt SMTP auth?
 ************************************************************************/
Boolean ShouldSMTPAuth(void)
{
	return PrefIsSet(PREF_SMTP_GAVE_530) || PrefIsSet(PREF_SUBMISSION_PORT);
}

#ifdef HAVE_KEYCHAIN
/************************************************************************
 * UpdatePersKCPassword - update the password for a personality in the keychain if needed
 ************************************************************************/
OSErr UpdatePersKCPassword(PersHandle pers)
{
	KCItemRef kcItem;
	Str63 user;
	Str255 host;
	Str255 password;
	OSErr err;
	uLong len;
	
	if (!KeychainAvailable()) return(userCanceledErr);
	
	PushPers(pers);
	GetPOPInfo(user,host);
	PopPers();
	MightSwitch();

	err = KCFindInternetPassword(host,nil,user,kAnyPort,CREATOR,kAnyAuthType,0,nil,&len,&kcItem);
	if (err == errKCItemNotFound)
	{
		PSCopy(password,(*pers)->password);
		err = KCAddInternetPassword(host,nil,user,kAnyPort,CREATOR,kAnyAuthType,*password,password+1,nil);
	}
	else if (err == noErr)
	{
		Boolean update = true;
		
		if (len <= sizeof(password)-1)
		{
			err = KCGetData(kcItem, sizeof(password)-1, password+1, &len);
			if (err == noErr)
			{
				unsigned char *s1, *s2;
				
				s1 = password+1;
				s2 = (*pers)->password;
				
				if (len == *s2++)
				{
					while(len && *s1++ == *s2++)
					{
						--len;
					}
					if (len == 0)
					{
						update = false;
					}
				}
				else
				{
					update = true;
				}
			}
			else
			{
				update = false;
			}
		}
		if (update)
		{
			PSCopy(password,(*pers)->password);
			err = KCSetData(kcItem, *password, password + 1);
			if (!err) err = KCUpdateItem(kcItem);
		}
		KCReleaseItemRef(&kcItem);
	}

	AfterSwitch();
	return(err);
}

/************************************************************************
 * FindPersKCPassword - find the password for a personality in the keychain
 ************************************************************************/
OSErr FindPersKCPassword(PersHandle pers,PStr password,uLong passStringLen)
{
	Str63 user;
	Str255 host;
	OSErr err;
	uLong len;
	
	if (!KeychainAvailable()) return(fnfErr);
	
	PushPers(pers);
	GetPOPInfo(user,host);
	PopPers();
	MightSwitch();
	err = KCFindInternetPassword(host,nil,user,kAnyPort,CREATOR,kAnyAuthType,passStringLen-1,password+1,&len,nil);
	AfterSwitch();
	if (!err) *password = len;
	else *password = 0;
	return(err);
}

/************************************************************************
 * DeletePersKCPassword - delete the password for a personality from the keychain
 ************************************************************************/
OSErr DeletePersKCPassword(PersHandle pers)
{
	KCItemRef kcItem;
	Str63 user;
	Str255 host;
	OSErr err;
	Str255 password;
	uLong len;
	
	if (!KeychainAvailable()) return(userCanceledErr);
	
	PushPers(pers);
	GetPOPInfo(user,host);
	PopPers();
	MightSwitch();
	if (*user && *host && !(err = KCFindInternetPassword(host,nil,user,kAnyPort,CREATOR,kAnyAuthType,sizeof(password)-1,password+1,&len,&kcItem)))
	{
		err = KCDeleteItem(kcItem);
		KCReleaseItemRef(&kcItem);
	}
	AfterSwitch();
	return(err);
}

/************************************************************************
 * AddPersKCPassword - add the password for a personality to the keychain
 ************************************************************************/
OSErr AddPersKCPassword(PersHandle pers,PStr password)
{
	Str63 user;
	Str255 host;
	OSErr err;
	
	if (!KeychainAvailable()) return(userCanceledErr);
	
	PushPers(pers);
	GetPOPInfo(user,host);
	PopPers();
	MightSwitch();
	err = KCAddInternetPassword(host,nil,user,kAnyPort,CREATOR,kAnyAuthType,*password,password+1,nil);
	AfterSwitch();
	return(err);
}
#endif //HAVE_KEYCHAIN
