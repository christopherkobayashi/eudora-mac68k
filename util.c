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

#include <AEObjects.h>
#include <KeychainHI.h>
#include <Gestalt.h>

#include <conf.h>
#include <mydefs.h>

#include "util.h"
#define FILE_NUM 41
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * various useful functions
 **********************************************************************/

typedef struct SoundEntryStruct SoundEntry, *SoundEntryPtr;

struct SoundEntryStruct {
	SoundEntryPtr next;
	Str255 name;
};

#pragma segment Util
#ifdef	KERBEROS
#include				<krb.h>
#endif

char BitTable[] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

pascal Boolean PasswordFilter(DialogPtr dgPtr, EventRecord * event,
			      short *item);
void CopyPassword(UPtr password);
int ResetPassword(void);
UHandle PwChars = nil;
void NukeMenuItem(MenuHandle mh, short item);
void CompactTempZone(void);
short FindMenuByName(UPtr name);
UPtr GetRStringLo(PStr theString, int theIndex, PersHandle forPers);
void *TempNewHandleGlue(long size, OSErr * err);
void QueueSound(PStr name);
void PlaySystemSound(PStr name);
void AddSoundsToMenuFrom(MenuHandle mh, short vRef, long dirID);
OSErr FindSystemSound(OSType disk, OSType folder, PStr name,
		      FSSpecPtr spec);

//      Globals
Movie gSoundMovie;
SoundEntryPtr gSystemSoundList;
GWorldPtr gGWorld;

#if 0
/**********************************************************************
 * NeedYield
 **********************************************************************/
Boolean NeedYield(void)
{
	short numThreads = GetNumBackgroundThreads();
	long elapsedTicks = TickCount() - ThreadYieldTicks;
	long vicomFactor =
	    (InBG ? 1 : (!VicomIs ? 1 : (GetRLong(VICOM_FACTOR))));
	long yieldInterval =
	    (InBG ? GetRLong(BG_YIELD_INTERVAL) :
	     GetRLong(FG_YIELD_INTERVAL)) /
	    ((numThreads ? numThreads : 1) * vicomFactor);

//      ASSERT(InBG ? (elapsedTicks < 120) : 1); // are we hogging time somewhere?
	if (EventPending() || (elapsedTicks > yieldInterval))
		return true;
	return false;
}
#endif

/**********************************************************************
 * write zeroes over an area of memory
 **********************************************************************/
void WriteZero(void *pointer, long size)
{
	UPtr u = (UPtr) pointer;

	while (size--)
		*u++ = 0;
}

#ifdef DEBUG
/**********************************************************************
 * SetBalloons - help the debugger a bit
 **********************************************************************/
void SetBalloons(Boolean on)
{
	HMSetBalloons(on);
}
#endif

/**********************************************************************
 * initialize all the mac managers
 **********************************************************************/
void MacInitialize(int masterCount, long ensureStack)
{
	OSErr err;
	while (masterCount--)
		MoreMasters();
	FlushEvents(everyEvent - diskMask, 0);
	InitCursor();
#ifdef THREADING_ON
	if (ThreadsAvailable())
		MyInitThreads();
#endif
	if (err = AEObjectInit())
		DieWithError(INSTALL_AE, err);
	EventPending();
	EventPending();
	EventPending();
}

/**********************************************************************
 * turn a font name into a font id; if the name is not found, use ApplFont
 **********************************************************************/
short GetFontID(UPtr theName)
{
	short theID;
	Str255 systemName;

	GetFNum(theName, &theID);
	if (!theID) {
		GetFontName(0, systemName);
		return (StringSame(theName, systemName) ? 0 : applFont);
	} else
		return (theID);
}

/**********************************************************************
 * Check or uncheck a font size in the font menu
 **********************************************************************/
void CheckFontSize(int menu, int size, Boolean check)
{
	Str255 aString;
	Str255 itemString;
	MenuHandle mHandle;
	short item;

	/*
	 * turn the font size into a string
	 */
	NumToString((long) size, aString);

	/*
	 * get a copy of the menu handle
	 */
	mHandle = GetMHandle(menu);

	/*
	 * look for the proper item
	 */
	for (item = 1;; item++) {
		GetMenuItemText(mHandle, item, itemString);
		if (itemString[1] == '-')
			break;
		if (StringSame(aString, itemString)) {
			CheckMenuItem(mHandle, item, check);
			break;
		}
	}
}

/**********************************************************************
 * check (or uncheck) a font name in a menu.	If check, also outline sizes
 **********************************************************************/
void CheckFont(int menu, int fontID, Boolean check)
{
	Str255 aString;
	Str255 itemString;
	MenuHandle mHandle;
	short item;

	/*
	 * turn the font id into a font name
	 */
	GetFontName(fontID, aString);

	/*
	 * get a copy of the menu handle
	 */
	mHandle = GetMHandle(menu);

	/*
	 * look for the proper item
	 */
	for (item = 1;; item++) {
		GetMenuItemText(mHandle, item, itemString);
		if (itemString[1] == '-')
			break;	/* skip the sizes */
	}
	for (item++;; item++) {
		GetMenuItemText(mHandle, item, itemString);
		if (StringSame(aString, itemString)) {
			CheckMenuItem(mHandle, item, check);
			if (check)
				OutlineFontSizes(menu, fontID);
			break;
		}
	}
}

/**********************************************************************
 * outline sizes of a font in a menu
 **********************************************************************/
void OutlineFontSizes(int menu, int fontID)
{
	Str255 aString;
	MenuHandle mHandle;
	short item;
	long aSize;

	/*
	 * get a copy of the menu handle
	 */
	mHandle = GetMHandle(menu);

	/*
	 * outline'em
	 */
	for (item = 1;; item++) {
		GetMenuItemText(mHandle, item, aString);
		if (aString[1] == '-')
			break;	/* end of sizes? */
		/*
		 * turn text into size
		 */
		StringToNum(aString, &aSize);

		/*
		 * does it exist?
		 */
		if (RealFont(fontID, (short) aSize))
			SetItemStyle(mHandle, item, outline);
		else
			SetItemStyle(mHandle, item, nil);
	}
}

/**********************************************************************
 * figure out the appropriate leading for a font
 **********************************************************************/
int GetLeading(int fontID, int fontSize)
{
	FMInput fInInfo;
	FMOutput *fOutInfo;

	/*
	 * set up the font input struct
	 */
	fInInfo.family = fontID;
	fInInfo.size = fontSize;
	fInInfo.face = 0;
	fInInfo.needBits = FALSE;
	fInInfo.device = 0;
	fInInfo.numer.h = fInInfo.numer.v = 1;
	fInInfo.denom.h = fInInfo.denom.v = 1;


	/*
	 * get the actual info
	 */
	fOutInfo = FMSwapFont(&fInInfo);

	/*
	 * yokey-dokey
	 */
	return (((fOutInfo->leading + fOutInfo->ascent +
		  fOutInfo->descent) * fOutInfo->numer.v) /
		fOutInfo->denom.v);
}

/**********************************************************************
 * find width of largest char in font
 **********************************************************************/
int GetWidth(int fontID, int fontSize)
{
#ifdef FONTMGR
	FMInput fInInfo;
	FMOutput *fOutInfo;

	/*
	 * set up the font input struct
	 */
	fInInfo.family = fontID;
	fInInfo.size = fontSize;
	fInInfo.face = 0;
	fInInfo.needBits = FALSE;
	fInInfo.device = 0;
	fInInfo.numer.h = fInInfo.numer.v = 1;
	fInInfo.denom.h = fInInfo.denom.v = 1;


	/*
	 * get the actual info
	 */
	fOutInfo = FMSwapFont(&fInInfo);

	/*
	 * yokey-dokey
	 */
	return ((fOutInfo->widMax * fOutInfo->numer.h) /
		fOutInfo->denom.h);
#else

	GrafPtr oldPort;
	GrafPtr aPort;
	int width;

	GetPort(&oldPort);

	MyCreateNewPort(aPort);
	TextFont(fontID);
	TextSize(fontSize);
	width = CharWidth('0');

	DisposePort(aPort);
	SetPort_(oldPort);

	return (width);
#endif
}

/**********************************************************************
 * find descent font
 **********************************************************************/
int GetDescent(int fontID, int fontSize)
{
	FMInput fInInfo;
	FMOutput *fOutInfo;

	/*
	 * set up the font input struct
	 */
	fInInfo.family = fontID;
	fInInfo.size = fontSize;
	fInInfo.face = 0;
	fInInfo.needBits = FALSE;
	fInInfo.device = 0;
	fInInfo.numer.h = fInInfo.numer.v = 1;
	fInInfo.denom.h = fInInfo.denom.v = 1;


	/*
	 * get the actual info
	 */
	fOutInfo = FMSwapFont(&fInInfo);

	/*
	 * yokey-dokey
	 */
	return ((fOutInfo->descent * fOutInfo->numer.v) /
		fOutInfo->denom.v);
}

/**********************************************************************
 * find ascent font
 **********************************************************************/
int GetAscent(int fontID, int fontSize)
{
	FMInput fInInfo;
	FMOutput *fOutInfo;

	/*
	 * set up the font input struct
	 */
	fInInfo.family = fontID;
	fInInfo.size = fontSize;
	fInInfo.face = 0;
	fInInfo.needBits = FALSE;
	fInInfo.device = 0;
	fInInfo.numer.h = fInInfo.numer.v = 1;
	fInInfo.denom.h = fInInfo.denom.v = 1;


	/*
	 * get the actual info
	 */
	fOutInfo = FMSwapFont(&fInInfo);

	/*
	 * yokey-dokey
	 */
	return ((fOutInfo->ascent * fOutInfo->numer.v) /
		fOutInfo->denom.v);
}

/**********************************************************************
 * find fixed-width-ness of font
 **********************************************************************/
Boolean IsFixed(int fontID, int fontSize)
{
	return (CharWidthInFont('M', fontID, fontSize) ==
		CharWidthInFont('i', fontID, fontSize));
}

/**********************************************************************
 * wait for the user to strike a modifier key
 **********************************************************************/
void AwaitKey(void)
{
	KeyMap kMap;
	register long *k;
	register long *kEnd;

	while (1) {
		GetKeys(&kMap);
		for (k = &kMap, kEnd = k + sizeof(KeyMap) / sizeof(long);
		     k < kEnd; k++)
			if (*k)
				return;
	}
}

/**********************************************************************
 * change or add some data to the current resource file
 **********************************************************************/
void ChangePResource(UPtr theData, int theLength, long theType, int theID,
		     UPtr theName)
{
	/*
	 * does the resource exist and reside in the topmost res file?
	 */
	Zap1Resource(theType, (short) theID);

	AddPResource(theData, theLength, theType, theID, theName);
}

/**********************************************************************
 * add some data to the current resource file
 **********************************************************************/
void AddPResource(UPtr theData, int theLength, long theType, int theID,
		  UPtr theName)
{
	Handle aHandle;

	/*
	 * allocate the handle
	 */
	aHandle = NuHandle((long) theLength);
	if (aHandle == nil)
		return;

	/*
	 * copy the data
	 */
	BMD(theData, *aHandle, (long) theLength);

	/*
	 * add it
	 */
	AddResource_(aHandle, theType, theID, theName);
}

/************************************************************************
 * ZapResourceLo - get rid of a resource.
 ************************************************************************/
OSErr ZapResourceLo(OSType type, short id, Boolean one)
{
	Handle resH;
	OSErr err = resNotFound;
	short refN;

	SetResLoad(False);
	resH = one ? Get1Resource(type, id) : GetResource_(type, id);
	SetResLoad(True);
	if (resH) {
		refN = HomeResFile(resH);
		RemoveResource(resH);
		ZapHandle(resH);
		err = ResError();
		if (!err && one)
			ZapResourceLo(type, id, one);
		MyUpdateResFile(refN);	/* this is very conservative, but it seems necessary
					 * to avoid duplicate resources on some PPC macs */
	}
	return (err);
}

/**********************************************************************
 * Atoi - replacement for standard C routine
 **********************************************************************/
long Atoi(UPtr s)
{
	long mul = 1;
	long n = 0;

	while (IsWhite(*s))
		s++;
	if (*s == '-') {
		mul = -1;
		s++;
	}
	if (*s == '+')
		s++;
	while (isdigit(*s)) {
		n = n * 10 + (*s - '0');
		s++;
	}
	return (n * mul);
}

/**********************************************************************
 * AToOSType - ASCII to OSType
 **********************************************************************/
OSType AToOSType(UPtr s)
{
	long n = 0;
	short i;

	for (i = 0; i < 4; ++i) {
		n <<= 8;
		n += *s++;
	}
	return (n);
}

/**********************************************************************
 * ResourceCpy - copy a resource from one resource file to the other
 **********************************************************************/
int ResourceCpy(short toRef, short fromRef, long type, int id)
{
	int oldRef;
	Handle resource;
	Str255 name;
	short attrs;

	oldRef = CurResFile();
	UseResFile(toRef);
	SetResLoad(false);
	resource = GetResource_(type, id);	/* no sense loading it if it's wrong */
	if (resource != nil) {
		if (HomeResFile(resource) == toRef) {
			/*
			 * delete the old one first
			 */
			SetResLoad(True);
			RemoveResource(resource);
			SetResLoad(False);
			ZapHandle(resource);
			resource = nil;
		} else if (HomeResFile(resource) != fromRef) {
			ReleaseResource_(resource);	/* wrong one */
			resource = nil;
		}
	}

	if (resource == nil) {
		/*
		 * fetch from the proper file
		 */
		UseResFile(fromRef);
		resource = Get1Resource(type, id);
	}
	SetResLoad(True);	/* turn that goodie back on */

	if (resource != nil) {
		attrs = GetResAttrs(resource);	/* save attributes */
		GetResInfo((Handle) resource, (short *) &id, (ResType *) & type, name);	/* get the name */
		LoadResource(resource);	/* load it in  */
		HNoPurge_(resource);	/* keep it right here */
		if (ResError())
			goto rErr;	/* did that work? */
		DetachResource(resource);	/* break cnxn with old file */
		UseResFile(toRef);	/* point at to resource file */
		AddResource_(resource, type, id, name);	/* stick into new file */
		if (ResError())
			goto rErr;	/* did that work? */
		SetResAttrs(resource, attrs);	/* restore attributes */
		ChangedResource(resource);	/* except it's been changed */
		HNoPurge(resource);	/* don't let anyone purge it */
	}
	UseResFile(oldRef);	/* restore old res file */

	if (resource == nil)
		return (resNotFound);

	return (noErr);
      rErr:
	UseResFile(oldRef);
	return (ResError());
}

/**********************************************************************
 * DrawTruncString - truncate and draw a string; restores string when done
 **********************************************************************/
void DrawTruncString(UPtr string, int len)
{
	int sLen = *string;

	if (sLen > len) {
		Byte save = string[len];
		string[len] = '�';
		*string = len;
		DrawString(string);
		*string = sLen;
		string[len] = save;
	} else
		DrawString(string);
}

/**********************************************************************
 * CalcTrunc - figure out how much of a string we can print to fit
 * in a given width
 **********************************************************************/
int CalcTextTrunc(UPtr string, short length, short width, GrafPtr port)
{
	short tLen;
	int cWidth;
	GrafPtr oldPort;

	if (width <= 0)
		return (0);

	GetPort(&oldPort);
	SetPort_(port);

	/*
	 * make an initial estimate
	 */
	tLen = MIN(width / CharWidth(' '), length);

	/*
	 * if the string fits, ...
	 */
	cWidth = TextWidth(string, 0, tLen);
	if (cWidth < width && tLen == length) {
		SetPort_(oldPort);
		return (length);
	}

	/*
	 * if it's too short
	 */
	if (cWidth <= width) {
		if (tLen < length) {
			tLen++;
			while (TextWidth(string, 0, tLen) < width
			       && tLen < length)
				tLen++;
			tLen--;
		}
	}
	/*
	 * too long...
	 * (we do pretend there is always room for 1 character)
	 */
	else
		do {
			tLen--;
			cWidth = TextWidth(string, 0, tLen);
		}
		while (tLen && cWidth > width);

	SetPort_(oldPort);
	return (tLen ? tLen : 1);
}

/**********************************************************************
 * WhiteRect - draw white rectangle with a black border
 **********************************************************************/
void WhiteRect(Rect * r)
{
	Rect myR = *r;

	FrameRect(&myR);
	InsetRect(&myR, 1, 1);
	EraseRect(&myR);
}

/************************************************************************
 * WannaSave - find out of the user wants to save the contents of a window
 ************************************************************************/
int WannaSave(MyWindowPtr win)
{
	Str255 title;
	short res;

	MyGetWTitle(GetMyWindowWindowPtr(win), title);
	res =
	    ComposeStdAlert(kAlertCautionAlert,
			    SAVE_CHANGES_ASTR + ALRTStringsOnlyStrn,
			    title);
	if (res == WANNA_SAVE_CANCEL)
		res = CANCEL_ITEM;
	return (res);
}

/************************************************************************
 * GetPassword - read a user's password
 ************************************************************************/
int GetPassword(PStr personality, PStr userName, PStr serverName,
		UPtr word, int size, short prompt)
{
	Str255 string;
	MyWindowPtr dgPtrWin;
	DialogPtr dgPtr;
	short item;
	DECLARE_UPP(PasswordFilter, ModalFilter);

	INIT_UPP(PasswordFilter, ModalFilter);
	if (!MommyMommy(ATTENTION, nil))
		return (PASSWORD_CANCEL);
	GetRString(string, prompt);
	MyParamText(personality, userName, serverName, string);
	if ((dgPtrWin =
	     GetNewMyDialog(PASSWORD_DLOG, nil, nil, InFront)) == nil) {
		WarnUser(GENERAL, MemError());
		ComposeLogR(LOG_ALRT, nil, ALERT_DISMISSED_ITEM,
			    PASSWORD_CANCEL);
		return (PASSWORD_CANCEL);
	}

	dgPtrWin->noEditMenu = true;	// sorry, no copy/paste

	dgPtr = GetMyWindowDialogPtr(dgPtrWin);
	if (!(CurrentModifiers() & alphaLock))
		HideDialogItem(dgPtr, PASSWORD_WARNING);
	AutoSizeDialog(dgPtr);

	StartMovableModal(dgPtr);
	ShowWindow(GetDialogWindow(dgPtr));
	HiliteButtonOne(dgPtr);
	SetDItemState(dgPtr, PASSWORD_SAVE, PrefIsSet(PREF_SAVE_PASSWORD));
	do {
		SetDIText(dgPtr, PASSWORD_WORD, "");
		SelectDialogItemText(dgPtr, PASSWORD_WORD, 0, REAL_BIG);
		if (ResetPassword()) {
			item = PASSWORD_CANCEL;
			break;
		}
		PushCursor(arrowCursor);

		do {
			MovableModalDialog(dgPtr, PasswordFilterUPP,
					   &item);
			if (item == PASSWORD_SAVE)
				SetDItemState(dgPtr, item,
					      !GetDItemState(dgPtr, item));
		}
		while (item == PASSWORD_SAVE);

		PopCursor();
		CopyPassword(string);
	}
	while (item == PASSWORD_OK && !*string);
	if (item == PASSWORD_OK)
		SetPref(PREF_SAVE_PASSWORD,
			GetDItemState(dgPtr,
				      PASSWORD_SAVE) ? YesStr : NoStr);
	ComposeLogR(LOG_ALRT, nil, ALERT_DISMISSED_ITEM, item);
	EndMovableModal(dgPtr);
	DisposDialog_(dgPtr);
	InBG = False;
	if (item == PASSWORD_OK) {
		if (*string > size - 1)
			*string = size - 1;
		PCopy(word, string);
	}
	return (item != PASSWORD_OK);
}

/************************************************************************
 * GetPassStuff - collect the stuff we need to get a password
 ************************************************************************/
void GetPassStuff(PStr persName, PStr uName, PStr hName)
{
	GetPref(uName, PREF_STUPID_USER);
	GetPref(hName, PREF_STUPID_HOST);
	PCopy(persName, (*CurPersSafe)->name);
	if (!*persName)
		GetRString(persName, DOMINANT);
}

/************************************************************************
 * ResetPassword - get the password routines ready
 ************************************************************************/
int ResetPassword(void)
{
	if (!PwChars)
		PwChars = NuHandle(256L);
	else if (!*PwChars)
		ReallocateHandle(PwChars, 256L);
	if (!PwChars || !*PwChars)
		return (MemError());
	HNoPurge_(PwChars);
	**PwChars = 0;
	return (0);
}

/************************************************************************
 * InvalidatePasswords - wipe out our memory of passwords
 ************************************************************************/
void InvalidatePasswords(Boolean pwGood, Boolean auxpwGood, Boolean all)
{
	PersHandle old;

	if (all) {
		old = CurPers;
		for (CurPers = PersList; CurPers;
		     CurPers = (*CurPers)->next)
			InvalidateCurrentPasswords(pwGood, auxpwGood);
		CurPers = old;
	} else
		InvalidateCurrentPasswords(pwGood, auxpwGood);
}

/**********************************************************************
 * InvalidatePasswords - wipe out our memory of passwords for the current personality
 **********************************************************************/
void InvalidateCurrentPasswords(Boolean pwGood, Boolean auxpwGood)
{
	if (!pwGood) {
		Zero((*CurPers)->password);
		(*CurPers)->popSecure = 0;
		if (PrefIsSet(PREF_SAVE_PASSWORD) && CurPers == PersList)
			SetPref(PREF_PASS_TEXT, "");
		SetPrefLong(PREF_POP_LAST_AUTH, 0);
	}
	if (!auxpwGood) {
		Zero((*CurPers)->secondPass);
		if (PrefIsSet(PREF_SAVE_PASSWORD) && CurPers == PersList)
			SetPref(PREF_AUXPW, "");
	}
#ifdef HAVE_KEYCHAIN
	if (!pwGood && KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
		DeletePersKCPassword(CurPers);
#endif				//HAVE_KEYCHAIN

	// force IMAP to reconnect
	if (!pwGood && PrefIsSet(PREF_IS_IMAP))
		IMAPInvalidatePerConnections(CurPers);

	if (!pwGood || !auxpwGood)
		(*CurPers)->dirty = true;
}

/************************************************************************
 * PasswordFilter - a ModalDialog filter for getting passwords
 ************************************************************************/
pascal Boolean PasswordFilter(DialogPtr dgPtr, EventRecord * event,
			      short *item)
{
	if (MiniMainLoop(event)) {
		*item = CANCEL_ITEM;
		return (True);
	}
	if (event->what == keyDown || event->what == autoKey) {
		if (event->modifiers & cmdKey) {
			SysBeep(20L);
			event->what = nullEvent;
			return false;
		} else {
			char key = event->message & charCodeMask;
			switch (key) {
			case enterChar:
			case returnChar:
				*item = 1;
				return (True);
				break;
			case backSpace:
				if (**PwChars)
					-- ** PwChars;
				return (False);
				break;
			case '.':
				if (event->modifiers & cmdKey) {
					*item = 2;
					return (True);
					break;
				}
				/* fall through */
			default:
				if (**PwChars < 255 && key != tabChar) {
					PCatC(*PwChars, key);
					event->message =
					    ((event->
					      message >> 8) << 8) |
					    bulletChar;
				} else {
					SysBeep(20);
					event->what = nullEvent;
				}
				return (False);
				break;
			}
		}
	} else if (event->what == updateEvt) {
		if (GetDialogFromWindow((WindowPtr) event->message) ==
		    dgPtr)
			HiliteButtonOne(dgPtr);
		else
			UpdateMyWindow((WindowPtr) event->message);
	} else {
		if (TickCount() % 120 < 100
		    && CurrentModifiers() & alphaLock)
			ShowDialogItem(dgPtr, PASSWORD_WARNING);
		else
			HideDialogItem(dgPtr, PASSWORD_WARNING);
	}

	return (False);
}

/************************************************************************
 * CopyPassword - retrieve the password
 ************************************************************************/
void CopyPassword(UPtr password)
{
	BMD(*PwChars, password, **PwChars + 1);
	HPurge(PwChars);
}

/************************************************************************
 * MyAppendMenu - see that a menu item gets appended to a menu.  Avoids
 * menu manager meta-characters.
 ************************************************************************/
void MyAppendMenu(MenuHandle menu, UPtr name)
{
	MyInsMenuItem(menu, name, CountMenuItems(menu));
}

/************************************************************************
 * GetItemColor - get the color of a menu item
 ************************************************************************/
RGBColor *GetItemColor(short menu, short item, RGBColor * color)
{
	MCEntryPtr mc;

	if (!(mc = GetMCEntry(menu, item)))
		DEFAULT_COLOR(*color);
	else
		*color = mc->mctRGB2;
	return (color);
}

/************************************************************************
 * MyInsMenuItem - see that a menu item gets appended to a menu.	Avoids
 * menu manager meta-characters.
 ************************************************************************/
void MyInsMenuItem(MenuHandle menu, UPtr name, short afterItem)
{
	Str255 fixName;

	PCopy(fixName, "\px");
	InsertMenuItem(menu, fixName, afterItem);
	if (!IsWordChar[name[1]] && (name[1] < '0' || name[1] > '9')) {
		PCat(fixName, name);
		fixName[1] = 0;
	} else
		PCopy(fixName, name);
	SetMenuItemText(menu, afterItem + 1, fixName);
}

/**********************************************************************
 * 
 **********************************************************************/
void SetItemR(MenuHandle menu, short item, short id)
{
	Str255 itemStr;
	GetRString(itemStr, id);
	MySetItem(menu, item, itemStr);
}

/**********************************************************************
 * MySetItem - set a menu item, avoiding metas
 **********************************************************************/
void MySetItem(MenuHandle menu, short item, PStr itemStr)
{
	if (!IsWordChar[itemStr[1]]
	    && (itemStr[1] < '0' || itemStr[1] > '9'))
		PInsert(itemStr, 255, "\p\000", itemStr + 1);
	SetMenuItemText(menu, item, itemStr);
}

/************************************************************************
 * MyGetItem - get the text of a menu item.  Strip leading NULL, if any
 ************************************************************************/
PStr MyGetItem(MenuHandle menu, short item, PStr name)
{
	GetMenuItemText(menu, item, name);
	if (*name && !name[1]) {
		BMD(name + 2, name + 1, name[0] - 1);
		name[0]--;
	}
	return (name);
}

/************************************************************************
 * CopyMenuItem - copy a menu item from one menu to another
 ************************************************************************/
OSErr CopyMenuItem(MenuHandle fromMenu, short fromItem, MenuHandle toMenu,
		   short toItem)
{
	Str255 s;
	short shortWhatever;
	long longWhatever;
	Byte byteWhatever;
	Handle handleWhatever;
	TextEncoding enc;
	short n = CountMenuItems(toMenu);

	// add the item
	InsertMenuItem(toMenu, "\p ", toItem);
	if (CountMenuItems(toMenu) != n + 1)
		return (MemError());

	// adjust toItem to be real
	if (toItem == 0)
		toItem = 1;
	else if (toItem > n)
		toItem = n + 1;
	else
		toItem++;

	// now, copy the attributes one at a time
	MyGetItem(fromMenu, fromItem, s);
	MySetItem(toMenu, toItem, s);

	GetItemMark(fromMenu, fromItem, &shortWhatever);
	SetItemMark(toMenu, toItem, shortWhatever);

	GetItemIcon(fromMenu, fromItem, &shortWhatever);
	SetItemIcon(toMenu, toItem, shortWhatever);

	GetItemMark(fromMenu, fromItem, &shortWhatever);
	SetItemMark(toMenu, toItem, shortWhatever);

	GetItemCmd(fromMenu, fromItem, &shortWhatever);
	SetItemCmd(toMenu, toItem, shortWhatever);

	GetItemStyle(fromMenu, fromItem, &byteWhatever);
	SetItemStyle(toMenu, toItem, byteWhatever);

	if (!GetMenuItemCommandID(fromMenu, fromItem, &longWhatever))
		SetMenuItemCommandID(toMenu, toItem, longWhatever);

	if (!GetMenuItemModifiers(fromMenu, fromItem, &byteWhatever))
		SetMenuItemModifiers(toMenu, toItem, byteWhatever);

	if (!GetMenuItemIconHandle
	    (fromMenu, fromItem, &byteWhatever, &handleWhatever))
		SetMenuItemIconHandle(toMenu, toItem, byteWhatever,
				      handleWhatever);

	if (!GetMenuItemTextEncoding(fromMenu, fromItem, &enc))
		SetMenuItemTextEncoding(toMenu, toItem, enc);

	if (!GetMenuItemHierarchicalID(fromMenu, fromItem, &shortWhatever))
		SetMenuItemHierarchicalID(toMenu, toItem, shortWhatever);

	if (!GetMenuItemFontID(fromMenu, fromItem, &shortWhatever))
		SetMenuItemFontID(toMenu, toItem, shortWhatever);

	if (!GetMenuItemRefCon(fromMenu, fromItem, &longWhatever))
		SetMenuItemRefCon(toMenu, toItem, longWhatever);

	if (!GetMenuItemKeyGlyph(fromMenu, fromItem, &shortWhatever))
		SetMenuItemKeyGlyph(toMenu, toItem, shortWhatever);

	EnableIf(toMenu, toItem, IsEnabled(GetMenuID(fromMenu), fromItem));

	return (noErr);
}

/**********************************************************************
 * PStr2Handle - copy a PString to a handle
 **********************************************************************/
UHandle PStr2Handle(PStr string)
{
	UHandle h = NuHandle(*string);

	if (h)
		BMD(string + 1, *h, *string);

	return (h);
}

/************************************************************************
 * SetItemReducedIcon - set a reduced icon for a menu item
 ************************************************************************/
void SetItemReducedIcon(MenuHandle menu, short item, short iconid)
{
	SetItemIcon(menu, item, iconid - 256);
	SetItemCmd(menu, item, 0x1D);
}

/************************************************************************
 * FindItemByName - find a named menu item
 ************************************************************************/
short FindItemByName(MenuHandle menu, UPtr name)
{
	short item;
	Str255 itemTitle;

	for (item = CountMenuItems(menu); item; item--) {
		MyGetItem(menu, item, itemTitle);
		if (StringSame(name, itemTitle))
			break;
	}

	return (item);
}

/************************************************************************
 * FindMenuByName - find a named menu item
 ************************************************************************/
short FindMenuByName(UPtr name)
{
	Str255 menuTitle;
	short i;
	MenuHandle m;
	Handle h;
	static short specialsPro[] =
	    { TABLE_HIER_MENU, REPLY_WITH_HIER_MENU, NEW_WITH_HIER_MENU,
FORWARD_TO_HIER_MENU,
		NEW_TO_HIER_MENU, REDIST_TO_HIER_MENU, INSERT_TO_HIER_MENU,
		    EMOTICON_HIER_MENU
	};
	static short specialsLight[] =
	    { TABLE_HIER_MENU, FORWARD_TO_HIER_MENU, NEW_TO_HIER_MENU,
REDIST_TO_HIER_MENU, INSERT_TO_HIER_MENU };
	short num, *specials;

	if (HasFeature(featureStationery)) {
		num = sizeof(specialsPro);
		specials = specialsPro;
	} else {
		num = sizeof(specialsLight);
		specials = specialsLight;
	}

	for (i = 0; i < num / sizeof(short); i++)
		if (m = GetMHandle(*specials++)) {
			GetMenuTitle(m, menuTitle);
			if (StringSame(name, menuTitle))
				return (GetMenuID(m));
		}



	for (i = 1; h = GetIndResource('MENU', i); i++) {
		PCopy(menuTitle, (*h) + 14);
		if (StringSame(name, menuTitle))
			return (*(short *) *h);
	}

	return (0);
}


/************************************************************************
 * MyUniqueID - get a unique id in the app range
 ************************************************************************/
short MyUniqueID(ResType type)
{
	short id;

	for (id = UniqueID(type); id < 128; id = UniqueID(type));
	return (id);
}

#pragma segment Util2

/**********************************************************************
 * OnBatteries - are we running on batteries?
 **********************************************************************/
static Boolean OnBatteries9(void)
{
	long value;
	Byte status, power;

	if (noErr != Gestalt(gestaltPowerMgrAttr, &value))
		return false;

	if (value & (1 << gestaltPMgrExists)) {
		if (noErr != BatteryStatus(&status, &power))
			return false;

		return (status & 1) == 0;
	}

	return false;
}

//      In Wrappers.cp
extern Boolean OnBatteriesX(void);

Boolean OnBatteries(void)
{
	static uLong ticks = 0;
	static Boolean val;

//      Don't check too often
	if (TickCount() - ticks < 60)
		return val;
	ticks = TickCount();

	return val = HaveOSX()? OnBatteriesX() : OnBatteries9();
}

/**********************************************************************
 * GestaltBits - return the bits for a Gestalt selector.  Note that this
 * routine cannot distinguish between an error and no bits, so if that's
 * important to you, use Gestalt directly.
 **********************************************************************/
uLong GestaltBits(OSType selector)
{
	uLong value;

	if (Gestalt(selector, &value))
		value = 0;

	return (value);
}

/************************************************************************
 * BinFindItemByName - find a named menu item, using binary search
 ************************************************************************/
short BinFindItemByName(MenuHandle menu, UPtr name)
{
	short item;
	Str255 itemTitle;
	short first, last;
	short cmp;

	first = 1;
	last = CountMenuItems(menu);
	for (item = (first + last) / 2; first <= last;
	     item = (first + last) / 2) {
		MyGetItem(menu, item, itemTitle);
		cmp = StringComp(name, itemTitle);
		switch (cmp) {
		case -1:
			last = item - 1;
			break;
		case 1:
			first = item + 1;
			break;
		case 0:
			return (item);
		}
	}

	return (0);
}

/************************************************************************
 * SpecialKeys - mutate events involving particular keys
 ************************************************************************/
void SpecialKeys(EventRecord * event)
{
	uShort menu, item;
	MenuRef mh;

	switch (event->message & charCodeMask) {
	case helpChar:
		HMSetBalloons(!HMGetBalloons());
		event->what = nullEvent;
		break;
	case homeChar:
	case endChar:
	case pageUpChar:
	case pageDownChar:
		event->what = app1Evt;
		break;
	default:
		switch ((event->message & keyCodeMask) >> 8) {
		case undoKey:
			menu = EDIT_MENU;
			item = EDIT_UNDO_ITEM;
			break;
		case cutKey:
			menu = EDIT_MENU;
			item = EDIT_CUT_ITEM;
			break;
		case copyKey:
			menu = EDIT_MENU;
			item = EDIT_COPY_ITEM;
			break;
		case pasteKey:
			menu = EDIT_MENU;
			item = EDIT_PASTE_ITEM;
			break;
		case clearKey:
			menu = EDIT_MENU;
			item = EDIT_CLEAR_ITEM;
			break;
		default:
			return;
		}
		EnableMenuItems(False);
		Type2SelTicks = 0;	/* make sure string gets cleared */
		mh = GetMenuHandle(menu);
		if (IsMenuItemEnabled(mh, 0)
		    && IsMenuItemEnabled(mh, item)) {
			short oldMenu = LMGetTheMenu();
			HiliteMenu(menu);
			DoMenu(FrontWindow_(), (menu << 16) | item,
			       event->modifiers);
			if (oldMenu)
				HiliteMenu(oldMenu);
		}
		event->what = nullEvent;
		break;
	}
}

/************************************************************************
 * Event2Window - grab a window from an event
 ************************************************************************/
WindowPtr Event2Window(EventRecord * event)
{
	WindowPtr maybe = (WindowPtr) event->message;
	WindowPtr is;

#ifdef	FLOAT_WIN
	for (is = FrontWindow(); is; is = GetNextWindow(is))
#else				//FLOAT_WIN
	for (is = FrontWindow_(); is; is = GetNextWindow(is))
#endif				//FLOAT_WIN
		if (is == maybe)
			break;

	return (is);
}

/**********************************************************************
 * FixURLString - undo escaped stuff in URL's
 **********************************************************************/
void FixURLString(PStr url)
{
	UPtr spot, end;

	spot = url + 1;
	end = url + *url + 1;

	for (; spot < end; spot++)
		if (*spot == '%')
			*spot = lowerDelta;

	CaptureHex(url, url);
}

/************************************************************************
 * CurrentModifiers - return the current state of the modifers
 ************************************************************************/
short CurrentModifiers(void)
{
	EventRecord theEvent;

	OSEventAvail(nil, &theEvent);
	return (theEvent.modifiers);
}


/************************************************************************
 * AttachHierMenu - attach a hierarchical menu to a menu item
 ************************************************************************/
void AttachHierMenu(short menu, short item, short hierId)
{
	MenuHandle mh = GetMHandle(menu);
	if (g16bitSubMenuIDs && hierId > 255)
		SetMenuItemHierarchicalID(mh, item, hierId);
	else {
		SetItemCmd(mh, item, 0x1b);
		SetItemMark(mh, item, hierId);
	}
}

/************************************************************************
 * DirtyKey - does a keystroke cause a window to become dirty?
 ************************************************************************/
Boolean DirtyKey(long keyAndChar)
{
	short charCode = keyAndChar & charCodeMask;
	short keyCode = (keyAndChar & keyCodeMask) >> 8;
	static short safeChars[] = {
		homeChar, endChar, pageUpChar, pageDownChar,
		leftArrowChar, rightArrowChar, upArrowChar, downArrowChar
	};
	short *which;

	for (which = safeChars;
	     which < safeChars + sizeof(safeChars) / sizeof(short);
	     which++)
		if (charCode == *which)
			return (False);

	return (True);
}

/************************************************************************
 * LocalDateTimeStr - return a ctime format date and time,
 * but as a pascal string
 * <length>Sun Sep 16 01:03:52 1973\015\0
 * This is quite purposefully not internationally blessed
 ************************************************************************/
UPtr LocalDateTimeStr(UPtr string)
{
	DateTimeRec dtr;

	GetTime(&dtr);
	return (ComposeRString(string, DATE_STRING_FMT,
			       WEEKDAY_STRN + dtr.dayOfWeek,
			       MONTH_STRN + dtr.month,
			       dtr.day / 10, dtr.day % 10,
			       dtr.hour / 10, dtr.hour % 10,
			       dtr.minute / 10, dtr.minute % 10,
			       dtr.second / 10, dtr.second % 10,
			       dtr.year));
}

/************************************************************************
 * WeekDay - get the name of a weekday
 ************************************************************************/
PStr WeekDay(PStr string, long secs)
{
	DateTimeRec dtr;
	short itlId;
	Str15 **weekNames;

	SecondsToDate(secs, &dtr);
	itlId = GetScriptVariable(smSystemScript, smScriptDate);
	weekNames = GetResource_('itl1', itlId);
	if (weekNames)
		return (PCopy(string, (*weekNames)[dtr.dayOfWeek - 1]));
	else
		return (GetRString(string, WEEKDAY_STRN + dtr.dayOfWeek));
}

/************************************************************************
 * GMTDateTime - return the current seconds
 ************************************************************************/
uLong GMTDateTime(void)
{
	static uLong secs;
	static uLong ticks;

	if (TickCount() - ticks > 30) {
		ticks = TickCount();
		GetDateTime(&secs);
		secs -= ZoneSecs();
	}
	return (secs);
}

/************************************************************************
 * LocalDateTime - return the current seconds
 ************************************************************************/
uLong LocalDateTime(void)
{
	uLong secs;
	GetDateTime(&secs);
	return (secs);
}

/************************************************************************
 * LocalDateTimeShortStr - return the date/time in a short representation
 ************************************************************************/
PStr LocalDateTimeShortStr(PStr s)
{
	DateTimeRec dtr;

	GetTime(&dtr);

	ComposeString(s, "\p%d%d%d%d%d%d%d%d",
		      dtr.month / 10, dtr.month % 10,
		      dtr.day / 10, dtr.day % 10,
		      dtr.hour / 10, dtr.hour % 10,
		      dtr.minute / 10, dtr.minute % 10);
	return (s);
}

/**********************************************************************
 * MenuWidth - return the width of a menu
 **********************************************************************/
short MenuWidth(MenuHandle mh)
{
	CalcMenuSize(mh);
	return (GetMenuWidth(mh));
}

/**********************************************************************
 * MyTrackDrag - track a drag
 **********************************************************************/
OSErr MyTrackDrag(DragReference drag, EventRecord * event, RgnHandle rgn)
{
	OSErr err;
	PushGWorld();

	DragFxxkOff = False;
	MightSwitch();
	err = TrackDrag(drag, event, rgn);
	AfterSwitch();
	DragFxxkOff = True;

	PopGWorld();
	return (err);
}

/**********************************************************************
 * HasDragManager - is the pestilent drag manager installed?
 **********************************************************************/
Boolean HasDragManager(void)
{
	long fxxkingGestalt;

	return (!
		(Gestalt('drag', &fxxkingGestalt)
		 || !(fxxkingGestalt & 1)))
#if TARGET_RT_MAC_CFM
	    && ((long) InstallTrackingHandler !=
		kUnresolvedCFragSymbolAddress && !PrefIsSet(PREF_NO_DRAG))
#endif
	    ;
}

/**********************************************************************
 * MyWaitMouseMoved - call WMM but only if we have the drag manager
 **********************************************************************/
Boolean MyWaitMouseMoved(Point pt, Boolean honorControl)
{
	return (HasDragManager()
		/*&& (!honorControl || !PrefIsSet(PREF_CONTROL_DRAG) || MainEvent.modifiers&controlKey) */
		&&WaitMouseMoved(pt));
}

/**********************************************************************
 * MyDragHas - does a drag have a particular type?
 **********************************************************************/
Boolean MyDragHas(DragReference drag, short item, OSType type)
{
	FlavorFlags flags;
	ItemReference ref;

	if (GetDragItemReferenceNumber(drag, item, &ref))
		return (False);

	if (GetFlavorFlags(drag, ref, type, &flags))
		return (False);

	return (!(flags & flavorSenderOnly));
}

/**********************************************************************
 * MyGetDragItemData - dig data out of a drag
 **********************************************************************/
OSErr MyGetDragItemData(DragReference drag, short item, OSType type,
			Handle * data)
{
	ItemReference ref;
	long len;
	OSErr err;
	Handle local = nil;

	if (data)
		*data = nil;

	err = GetDragItemReferenceNumber(drag, item, &ref);

	//Dprintf("\pRef %o %d %d %d;g",type,item,err,ref);

	if (err)
		return (err);

	err = GetFlavorDataSize(drag, ref, type, &len);

	//Dprintf("\pSiz %o %d %d %d;g",type,item,err,len);

	if (err)
		return (err);

	if (!(local = NuHTempBetter(len)))
		return (err = MemError());

	err = GetFlavorData(drag, ref, type, LDRef(local), &len, 0);

	//Dprintf("\pDat %o %d %d %p;g",type,item,err,*local);

	if (err) {
		ZapHandle(local);
		return (err);
	}

	UL(local);

	if (data)
		*data = local;
	else
		ZapHandle(local);

	return (noErr);
}

/**********************************************************************
 * MySetDragItemFlavorData - stick data into a flavor
 **********************************************************************/
OSErr MySetDragItemFlavorData(DragReference drag, short item, OSType type,
			      void *data, long len)
{
	ItemReference ref;
	OSErr err;

	if (!(err = GetDragItemReferenceNumber(drag, item, &ref)))
		err = SetDragItemFlavorData(drag, ref, type, data, len, 0);

	return (err);
}

/**********************************************************************
 * 
 **********************************************************************/
short DragOrMods(DragReference drag)
{
	short m1, m2, m3;

	GetDragModifiers(drag, &m1, &m2, &m3);
	return (m1 | m2 | m3);
}

/**********************************************************************
 * MyCountDragItems - count the items in a drag
 **********************************************************************/
short MyCountDragItems(DragReference drag)
{
	short count;
	OSErr err = CountDragItems(drag, &count);

	if (err)
		return (0);
	return (count);
}

/**********************************************************************
 * MyCountDragItemFlavors - count drag item flavors
 **********************************************************************/
short MyCountDragItemFlavors(DragReference drag, short item)
{
	short count;
	ItemReference ref;

	if (GetDragItemReferenceNumber(drag, item, &ref))
		return (0);

	if (CountDragItemFlavors(drag, ref, &count))
		return (0);

	return (count);
}

/**********************************************************************
 * MyGetDragItemFlavorType - get the type of a particular flavor
 **********************************************************************/
OSType MyGetDragItemFlavorType(DragReference drag, short item,
			       short flavor)
{
	ItemReference ref;
	OSType type;

	if (GetDragItemReferenceNumber(drag, item, &ref))
		return (0);

	if (GetFlavorType(drag, ref, flavor, &type))
		return (0);

	return (type);
}

/**********************************************************************
 * MyGetDragItemFlavorFlags
 **********************************************************************/
FlavorFlags MyGetDragItemFlavorFlags(DragReference drag, short item,
				     short flavor)
{
	FlavorFlags flags;
	ItemReference ref;
	OSType type;

	if (GetDragItemReferenceNumber(drag, item, &ref))
		return (0);

	if (GetFlavorType(drag, ref, flavor, &type))
		return (0);

	if (GetFlavorFlags(drag, ref, type, &flags))
		return (0);

	return (flags);
}

/************************************************************************
 * MiniEvents - call WNE, handle a few events, allow cmd-.
 ************************************************************************/
Boolean MiniEventsLo(long sleepTime, uLong mask)
{
	EventRecord event;
	Boolean newCommandPeriod = False;

	if (NEED_YIELD) {
#ifndef SLOW_CLOSE
		TcpFastFlush(False);	/* give lingering connections a chance to die */
#endif
		newCommandPeriod = HasCommandPeriod();
		if (newCommandPeriod)
			CommandPeriod = true;
		if (WNE(mask, &event, sleepTime))
			(void) MiniMainLoop(&event);
	}
	return (newCommandPeriod);
}

#ifdef DEBUG
/**********************************************************************
 * Rude - be rude to memory
 **********************************************************************/
void Rude(void)
{
	PurgeMem(0x7fffffff);
	CompactMem(0x7fffffff);
}
#endif
/************************************************************************
 * PlayNamedSound - play a sound with a given name
 ************************************************************************/
void PlayNamedSound(PStr name)
{
	Handle sound = GetNamedResource('snd ', name);
	if (sound) {
		HNoPurge(sound);
		SndPlay(nil, (void *) sound, False);
		HPurge(sound);
	} else {
		//      Not sound resource. May be system sound
		if (gSoundMovie)
			//      Busy playing a sound. Queue and try later
			QueueSound(name);
		else
			PlaySystemSound(name);
	}
}

/************************************************************************
 * PlaySystemSound - play a system sound
 ************************************************************************/
void PlaySystemSound(PStr name)
{
	//      Look for file in system sounds folder
	FSSpec spec;

	if (!FindSystemSound
	    (kUserDomain, kSystemSoundsFolderType, name, &spec)
	    || !FindSystemSound(kOnSystemDisk, kSystemSoundsFolderType,
				name, &spec)) {
		short fileRefNum;
		OSErr err;

		//      Make sure QuickTime is inited
		if (!QTMoviesInited) {
			if (EnterMovies())
				return;
			QTMoviesInited = true;
		}

		//      QT movies are attached to a port, even if the movie is just a sound.
		//  By default, the current port is used. This is bad news if the port
		//  gets closed, so we specifically assign a port. We use an off screen port.
		if (!gGWorld) {
			Rect bounds;

			SetRect(&bounds, 0, 0, 16, 16);
			if (NewGWorld(&gGWorld, 0, &bounds, nil, nil, 0))
				return;
		}

		if (!OpenMovieFile(&spec, &fileRefNum, fsRdPerm)) {
			err =
			    NewMovieFromFile(&gSoundMovie, fileRefNum, 0,
					     nil, newMovieActive, nil);
			CloseMovieFile(fileRefNum);
			if (!err) {
				SetMovieGWorld(gSoundMovie, gGWorld, nil);
				GoToBeginningOfMovie(gSoundMovie);
				StartMovie(gSoundMovie);
			}
		}
	}
}

/************************************************************************
 * FindSystemSound - look in a particular location for a sound
 ************************************************************************/
OSErr FindSystemSound(OSType disk, OSType folder, PStr name,
		      FSSpecPtr spec)
{
	Str255 suffices;
	UPtr spot;
	Str31 suffix;
	OSErr err = fnfErr;

	if (!FindFolder(disk, folder, false, &spec->vRefNum, &spec->parID)) {
		PStrCopy(spec->name, name, sizeof(spec->name));
		if (err = FSpExists(spec)) {
			//      This file doesn't exist. Try putting some file extensions on
			GetRString(suffices, SOUND_SUFFICES);
			spot = suffices + 1;
			while (PToken(suffices, suffix, &spot, "�")
			       && *suffix) {
				PCat(spec->name, suffix);
				if (!(err = FSpExists(spec)))
					break;	//      This one exists
				PStrCopy(spec->name, name,
					 sizeof(spec->name));
			}
		}
	}
	return err;
}


/************************************************************************
 * PlaySoundIdle - idle function for playing sounds with QuickTime
 ************************************************************************/
void PlaySoundIdle(void)
{
	if (gSoundMovie) {
		MoviesTask(gSoundMovie, 0);
		if (IsMovieDone(gSoundMovie)) {
			//      Done with this movie
			DisposeMovie(gSoundMovie);
			gSoundMovie = nil;

			//      Any more sounds to play?
			if (gSystemSoundList) {
				SoundEntryPtr next =
				    gSystemSoundList->next;

				PlaySystemSound(gSystemSoundList->name);

				//      Dispose of queue entry
				ZapPtr(gSystemSoundList);
				gSystemSoundList = next;
			}
		}
	} else if (gGWorld) {
		// No longer need gworld
		DisposeGWorld(gGWorld);
		gGWorld = nil;
	}
}

/************************************************************************
 * QueueSound - queue this system sound to play it later
 ************************************************************************/
void QueueSound(PStr name)
{
	SoundEntryPtr entry;

	if (entry = NuPtrClear(sizeof(SoundEntry))) {
		SoundEntryPtr t = gSystemSoundList;

		PCopy(entry->name, name);

		//      Put at end of queue     
		if (t) {
			while (t->next)
				t = t->next;
			t->next = entry;
		} else
			gSystemSoundList = entry;
	}
}

/************************************************************************
 * PlaySoundId - play a sound with a given resource id
 ************************************************************************/
void PlaySoundId(short id)
{
	Handle sound = GetResource('snd ', id);
	if (sound) {
		HNoPurge(sound);
		SndPlay(nil, (void *) sound, False);
		HPurge(sound);
	}
}

/************************************************************************
 * AddSoundsToMenu - add sound names to menu
 ************************************************************************/
void AddSoundsToMenu(MenuHandle mh)
{
	short vRef = 0, vRefWas;
	long dirID = 0, dirIDWas;

	//      Now add all sounds from system sounds folder
	if (!FindFolder
	    (kUserDomain, kSystemSoundsFolderType, false, &vRef, &dirID))
		AddSoundsToMenuFrom(mh, vRef, dirID);

	dirIDWas = dirID;
	vRefWas = vRef;
	if (!FindFolder
	    (kOnSystemDisk, kSystemSoundsFolderType, false, &vRef, &dirID))
		if (vRef != vRefWas || dirID != dirIDWas)
			AddSoundsToMenuFrom(mh, vRef, dirID);
}

/************************************************************************
 * AddSoundsToMenuFrom - add sound names to menu, from a particular folder
 ************************************************************************/
void AddSoundsToMenuFrom(MenuHandle mh, short vRef, long dirID)
{
	Str32 name;
	Str255 suffices;
	UPtr spot;
	Str31 suffix;
	Boolean needsDivider = CountMenuItems(mh) != 0;
	CInfoPBRec hfi;

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(vRef, dirID, &hfi)) {
		//      Look for a file extension we can remove 
		GetRString(suffices, SOUND_SUFFICES);
		spot = suffices + 1;
		while (PToken(suffices, suffix, &spot, "�") && *suffix) {
			if (EndsWith(name, suffix)) {
				//      Remove suffix
				*name -= *suffix;
				break;
			}
		}

		// don't add second item with same name
		if (FindItemByName(mh, name))
			continue;

		// add divider if first sound from this folder
		if (needsDivider) {
			AppendMenu(mh, "\p-");	//      Divider line
			needsDivider = false;
		}

		//      Add to menu
		MyAppendMenu(mh, name);
	}
}

/************************************************************************
 * UnadornKey - unadorn an event message
 ************************************************************************/
long UnadornKey(long message, short modifiers)
{
	Handle curKCHR;
	static long state = 0;
	short key;

	curKCHR = GetResource_('KCHR',
			       GetScriptVariable(GetScriptManagerVariable
						 (smKeyScript),
						 smScriptKeys));
	if (curKCHR && (modifiers & optionKey)) {
		/*
		 * get what it would have been without the option key
		 */
		key =
		    ((message >> 8) & 0xff) | (modifiers & (~optionKey) &
					       0xff00);
		message = KeyTranslate(LDRef(curKCHR), key, &state);
		UL(curKCHR);

		/*
		 * now massage this into an event "message"
		 */
		message = message & 0xff;
	}

	return (message);
}

extern char checkKey;
/************************************************************************
 * MyMenuKey - fix MenuKey to ignore option key
 ************************************************************************/
long MyMenuKeyLo(EventRecord * event, Boolean enable)
{
	long select;

	if (enable && event->modifiers & cmdKey)
		EnableMenuItems(False);
	select = MenuEvent(event);
	if (select & 0xffff0000)
		return (select);

	// STUPID hack.
	if (PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD)
	    || !PrefIsSet(PREF_NO_ALTERNATE_ATTACH_CMD)) {
		if ((select = STUPIDCheckMailHack(event)) != 0)
			return (select);
	}

	// cmd-opt-J - don't strip the opt since it will hit cmd-J
	if (JunkPrefSwitchCmdJ() && ((event->message) & 0xff) == 0xC6)
		return (MenuKey(event->message));

	return (MenuKey(UnadornMessage(event)));
}

/************************************************************************
 * AFPopUpMenuSelect - pop up menu in current font
 ************************************************************************/
long AFPopUpMenuSelect(MenuHandle mh, short top, short left, short item)
{
	short oldSysFont = LMGetSysFontFam();
	short oldSysSize = LMGetSysFontSize();
	short oldWmgrSize;
	short oldCWmgrSize;
	long sel;
	short i;
	GrafPtr wMgrPort, cwMgrPort;
	PushGWorld();

	GetWMgrPort(&wMgrPort);
	SetPort(wMgrPort);
	oldWmgrSize = GetPortTextSize(wMgrPort);
	TextSize(SmallSysFontID());
	if (ThereIsColor) {
		GetCWMgrPort((void *) &cwMgrPort);
		SetPort(cwMgrPort);
		oldCWmgrSize = GetPortTextSize(cwMgrPort);
		TextSize(ScriptVar(smScriptSmallFondSize));
	}
	PopGWorld();

	LMSetSysFontFam(SmallSysFontID());
	LMSetSysFontSize(SmallSysFontSize());
	LMSetLastSPExtra(-1);

	if (item >= 0) {
		for (i = CountMenuItems(mh); i; i--)
			if (item == i)
				SetItemMark(mh, i, diamondChar);
			else
				SetItemMark(mh, i, noMark);
	} else {
		//      Don't mess with check marks
		item = 0;
	}

	sel = PopUpMenuSelect(mh, top, left, item);

	LMSetSysFontFam(oldSysFont);
	LMSetSysFontSize(oldSysSize);
	LMSetLastSPExtra(-1);

	PushGWorld();
	SetPort(wMgrPort);
	TextSize(oldWmgrSize);
	if (ThereIsColor) {
		SetPort(cwMgrPort);
		TextSize(oldCWmgrSize);
	}
	PopGWorld();
	return (sel);
}


/************************************************************************
 * SetHiliteMode - Turn on hilite mode
 ************************************************************************/
void SetHiliteMode(void)
{
	Byte mode = LMGetHiliteMode();
	BitClr(&mode, pHiliteBit);
	LMSetHiliteMode(mode);
}

/************************************************************************
 * IsPowerNoVM - is powermac vm turned on?
 ************************************************************************/
Boolean IsPowerNoVM(void)
{
	return (!VM);
}


/************************************************************************
 * ChangeStrn - change a string in an Str# resource
 ************************************************************************/
UPtr ChangeStrn(short resId, short num, UPtr string)
{
	Handle resH;
	UPtr spot;
	short count;
	short i;
	long hSize;
	short diff;
	long offset;

	SCClear(resId + num);

	if (RecountStrn(resId))
		Aprintf(OK_ALRT, Caution, FIXED_STRN, resId);

	if (!(resH = GetResource_('STR#', resId)))
		return (nil);
	HNoPurge(resH);
	spot = LDRef(resH);
	hSize = GetHandleSize_(resH);
	count = 256 * spot[0] + spot[1];
	if (num <= count) {
		spot += 2;
		for (i = 1; i < num; i++)
			spot += *spot + 1;
		diff = *string - *spot;
		if (diff < 0) {
			diff *= -1;
			if (num < count)
				BMD(spot + diff, spot,
				    hSize - (spot - *resH) - diff);
			SetHandleBig_(resH, hSize - diff);
		} else if (diff > 0) {
			offset = spot - *resH;
			UL(resH);
			SetHandleBig_(resH, hSize + diff);
			if (i = MemError()) {
				WarnUser(MEM_ERR, i);
				UL(resH);
				return (nil);
			}
			spot = LDRef(resH) + offset;
			if (num < count)
				BMD(spot, spot + diff, hSize - offset);
		}
	} else {
		UL(resH);
		SetHandleBig_(resH, hSize + *string + num - count);
		if (i = MemError()) {
			WarnUser(MEM_ERR, i);
			return (nil);
		}
		spot = LDRef(resH);
		if (num > count + 1)
			WriteZero(spot + hSize, num - count - 1);
		spot[0] = num / 256;
		spot[1] = num % 256;
		spot += hSize + num - count - 1;
	}
	hSize = GetHandleSize_(resH);
	BMD(string, spot, *string + 1);
	UL(resH);
	ChangedResource(resH);
	RecountStrn(resId);
	return (string);
}


/************************************************************************
 * RecountStrn - make sure an STR# resource really has the right number
 * of strings
 *
 * returns True if the resource had to be changed
 ************************************************************************/
Boolean RecountStrn(short resId)
{
	Handle resH = GetResource_('STR#', resId);
	UPtr spot, end;
	short count;
	short realCount = 0;
	Boolean changed = False;

	/*
	 * no resource?
	 */
	if (!resH || !*resH)
		return (False);
	spot = *resH;

	/*
	 * does it have a count?
	 */
	end = spot + GetHandleSize_(resH);

	if (end - spot < 2) {
		RemoveResource(resH);
		ZapHandle(resH);
		return (True);
	}			// not even two bytes long!

	if (end - spot == 2 && !spot[0] && !spot[1])
		return false;	// empty but that's ok

	/*
	 * what's the current count?
	 */
	count = spot[0] * 256 + spot[1];

	/*
	 * count the actual strings, stopping if we go beyond the end of the resource
	 */
	for (spot += 2; spot + *spot + 1 < end; spot += *spot + 1)
		realCount++;

	/*
	 * if the last string hits the very end of the resource, we count it, too
	 */
	if (spot + *spot + 1 == end) {
		spot += *spot + 1;
		realCount++;
	}

	/*
	 * ok, at this point:
	 *
	 * spot points just AFTER the last valid string in the resource
	 * realCount is the actual count of the strings contained in the resource
	 */

	/*
	 * remove extra data from resource
	 */
	if (spot != end) {
		SetHandleBig_(resH, spot - *resH);
		ChangedResource(resH);
		HNoPurge_(resH);
		changed = True;
	}

	/*
	 * repair the count
	 */
	if (realCount != count) {
		spot = *resH;
		spot[0] = realCount / 256;
		spot[1] = realCount % 256;
		ChangedResource(resH);
		HNoPurge_(resH);
		changed = True;
	}

	return (changed);
}

/************************************************************************
 * CountStrn - count the strings an STR# resource says it has
 ************************************************************************/
short CountStrn(short resId)
{
	Handle resH = GetResource_('STR#', resId);
	return (CountStrnRes(resH));
}

/************************************************************************
 *
 ************************************************************************/
void NukeMenuItemByName(short menuId, UPtr itemName)
{
	MenuHandle mh = GetMHandle(menuId);
	short itemNum = FindItemByName(mh, itemName);

	if (itemNum)
		NukeMenuItem(mh, itemNum);
}

void NukeMenuItem(MenuHandle mh, short item)
{
	if (HasSubmenu(mh, item)) {
		short subId, subItem;
		MenuHandle subMh;

		subId = SubmenuId(mh, item);
		subMh = GetMHandle(subId);
		for (subItem = CountMenuItems(subMh); subItem; subItem--)
			NukeMenuItem(mh, subItem);
	}
	DeleteMenuItem(mh, item);
}

/************************************************************************
 *
 ************************************************************************/
void RenameItem(short menuId, UPtr oldName, UPtr newName)
{
	MenuHandle mh = GetMHandle(menuId);

	SetMenuItemText(mh, FindItemByName(mh, oldName), newName);
}

/************************************************************************
 *
 ************************************************************************/
Boolean HasSubmenu(MenuHandle mh, short item)
{
	short cmd;

	if (g16bitSubMenuIDs) {
		if (GetMenuItemHierarchicalID(mh, item, &cmd))
			return false;	//      error
		else
			return cmd != 0;
	}
	GetItemCmd(mh, item, &cmd);
	return (cmd == 0x1b);
}

/************************************************************************
 * SubmenuId - return the id of a submenu if an item has one, or zero
 ************************************************************************/
short SubmenuId(MenuHandle mh, short item)
{
	short cmd;

	if (g16bitSubMenuIDs) {
		if (GetMenuItemHierarchicalID(mh, item, &cmd))
			return 0;	//      error
		else
			return cmd;
	} else {
		GetItemCmd(mh, item, &cmd);
		if (cmd == 0x1b) {
			GetItemMark(mh, item, &cmd);
			return (cmd);
		} else
			return (0);
	}
}

/************************************************************************
 * SetGreyControl - grey a control, if it isn't already
 ************************************************************************/
Boolean SetGreyControl(ControlHandle cntl, Boolean shdBeGrey)
{
	Boolean lifeSucksThanksToOSX =
	    IsControlActive(cntl) == shdBeGrey && HaveTheDiseaseCalledOSX()
	    && IsControlVisible(cntl);

	if (lifeSucksThanksToOSX)
		SetControlVisibility(cntl, false, false);

	ActivateMyControl(cntl, !shdBeGrey);

	if (lifeSucksThanksToOSX) {
		Rect r;
		SetControlVisibility(cntl, true, false);
		GetControlBounds(cntl, &r);
		InvalWindowRect(GetControlOwner(cntl), &r);
	}

	return (shdBeGrey);
}

/************************************************************************
 * IsAUX - is A/UX running?
 ************************************************************************/
Boolean IsAUX(void)
{
	return false;
}

/************************************************************************
 * ZoneSecs - get the timezone offset, in seconds
 ************************************************************************/
long ZoneSecs(void)
{
	MachineLocation ml;
	static long delta;
	static uLong ticks;

	if (TickCount() - ticks < 600)
		return (delta);
	ticks = TickCount();
#define GMTDELTA u.gmtDelta
	ReadLocation(&ml);
	if (ml.latitude == ml.longitude && ml.GMTDELTA == ml.longitude
	    && ml.GMTDELTA == 0)
		return (delta = -1);

	delta = ml.GMTDELTA & 0xffffff;
	if ((delta >> 23) & 1)
		delta |= 0xff000000;

	return (delta);
}

/**********************************************************************
 * AddMyResource - add resource, but set Eudora Settings, too
 **********************************************************************/
void AddMyResource(Handle h, OSType type, short id, ConstStr255Param name)
{
	if (SettingsRefN)
		UseResFile(SettingsRefN);
	AddResource(h, type, id, name);
}

/************************************************************************
 * Provide the same benefit as a politician
 ************************************************************************/
void NOOP(void)
{
}



/************************************************************************
 * RoundDiv - Divide with rounding away from the origin
 ************************************************************************/
long RoundDiv(long quantity, long unit)
{
	if (quantity < 0)
		quantity -= unit - 1;
	else
		quantity += unit - 1;
	return (quantity / unit);
}

/************************************************************************
 * TZName2Offset - interpret the time zone with a resource
 ************************************************************************/
long TZName2Offset(CStr zoneName)
{
	UPtr this, end;
	Handle tznH = GetResource_('zon#', TZ_NAMES);
	long offset = 0;
	Str15 psName;

	CtoPCpy(psName, zoneName);
	TrimWhite(psName);

	if (tznH) {
		this = LDRef(tznH);
		end = this + GetHandleSize_(tznH);
		for (; this < end; this += *this + 1 + 2 * sizeof(short))
			if (StringSame(this, psName)) {
				short hrs, mins;
				this += *this + 1;
				hrs = this[0] * 256 + this[1];
				mins = this[2] * 256 + this[3];
				offset = hrs * 3600 + 60 * mins;
				break;
			}
		UL(tznH);
	}
	return (offset);
}

/************************************************************************
 * CenterRectIn - center one rect in another
 ************************************************************************/
void CenterRectIn(Rect * inner, Rect * outer)
{
	OffsetRect(inner,
		   (outer->left + outer->right - inner->left -
		    inner->right) / 2,
		   (outer->top + outer->bottom - inner->top -
		    inner->bottom) / 2);
}

/************************************************************************
 * TopCenterRectIn - center one rect in (the bottom of) another
 ************************************************************************/
void TopCenterRectIn(Rect * inner, Rect * outer)
{
	OffsetRect(inner,
		   (outer->left + outer->right - inner->left -
		    inner->right) / 2, outer->top - inner->top);
}

/************************************************************************
 * BottomCenterRectIn - center one rect in (the bottom of) another
 ************************************************************************/
void BottomCenterRectIn(Rect * inner, Rect * outer)
{
	OffsetRect(inner,
		   (outer->left + outer->right - inner->left -
		    inner->right) / 2, outer->bottom - inner->bottom);
}

/************************************************************************
 * ThirdCenterRectIn - center one rect in (the top 1/3 of) another
 ************************************************************************/
void ThirdCenterRectIn(Rect * inner, Rect * outer)
{
	OffsetRect(inner,
		   (outer->left + outer->right - inner->left -
		    inner->right) / 2,
		   outer->top - inner->top + (outer->bottom - outer->top -
					      inner->bottom +
					      inner->top) / 3);
}

/**********************************************************************
 * IsEnabled - is a menu item enabled
 **********************************************************************/
Boolean IsEnabled(short menu, short item)
{
	MenuHandle mh = GetMHandle(menu);

	if (!mh)
		return (False);
	if (!IsMenuItemEnabled(mh, 0))
		return false;
	return IsMenuItemEnabled(mh, item);
}

/**********************************************************************
 * 
 **********************************************************************/
void ShowDragRectHilite(DragReference drag, Rect * r, Boolean inside)
{
	RgnHandle rgn = NewRgn();
	if (rgn) {
		RectRgn(rgn, r);
		ShowDragHilite(drag, rgn, inside);
		DisposeRgn(rgn);
	}
}

/**********************************************************************
 * CheckNone - make sure no items are marked in a menu
 **********************************************************************/
void CheckNone(MenuHandle mh)
{
	short i;

	if (mh)
		for (i = CountMenuItems(mh); i; i--)
			if (!HasSubmenu(mh, i))
				SetItemMark(mh, i, noMark);
}

/**********************************************************************
 * ButtonFit - shrink button to min possibe size
 **********************************************************************/
void ButtonFit(ControlHandle button)
{
	Str255 title;
	short wi, hi;
	Rect r, rBtn;
	short base;
	MyWindowPtr win = GetWindowMyWindowPtr(GetControlOwner(button));

	if (ControlIsUgly(button)) {
		Zero(r);
		GetBestControlRect(button, &r, &base);
		GetControlBounds(button, &rBtn);
		MoveMyCntl(button, rBtn.left, rBtn.top, RectWi(r),
			   RectHi(r));
	} else {
		GetControlTitle(button, title);
		wi = StringWidth(title) + win->hPitch * 3;
		hi = win->vPitch + 4;
		SizeControl(button, wi, hi);
	}
}

/**********************************************************************
 * return a string from an STR# resource
 **********************************************************************/
PStr GetRString(PStr theString, short theIndex)
{
	SCPtr start, end;
	uLong ticks = TickCount();
	uLong oldest = ticks;
	short oldSpot;
	long n;
	Str63 sizeStr;
	uLong curPersId =
	    CurThreadGlobals ? (CurPers ? (*CurPers)->persId : 0) : 0;
	Boolean dontReadCache = NoDominant || !StringCache || NoProxify;
	Boolean dontWriteCache = NoDominant || NoProxify || GrowZoning
	    || (EjectBuckaroo && !StringCache);
	Boolean replaceOld = false;

	/*
	 * find it in the cache
	 */
	if (!dontReadCache) {
		n = HandleCount(StringCache);
		start = *StringCache;
		end = start + n;
		for (; start < end; start++) {
			if (theIndex == start->id
			    && start->persId == curPersId) {
				PCopy(theString, start->string);
				theString[*theString + 1] = 0;
				start->used = ticks;
				return (ProxifyStr(theString, theIndex));
			} else if (oldest)
				if (!start->id) {
					oldest = 0;
					oldSpot = start - *StringCache;
					replaceOld = true;	// no need to replace one
				} else if (oldest > start->used) {
					oldest = start->used;
					oldSpot = start - *StringCache;
					replaceOld = true;	// found one we can replace
				}
		}
	}

	/*
	 * not in the cache.  Grab it.
	 */
	GetRStringLo(theString, theIndex, CurPers);

	/*
	 * create cache
	 */
	if (!StringCache && !dontWriteCache) {
		GetRStringLo(sizeStr, STRING_CACHE, nil);
		StringToNum(sizeStr, &n);
		StringCache = NuHandleClear(n * sizeof(StringCacheEntry));
		oldSpot = 0;
	}

	/*
	 * cache string
	 */
	if (StringCache && !dontWriteCache && replaceOld)	// skip the cache just this once
	{
		PCopy((*StringCache)[oldSpot].string, theString);
		(*StringCache)[oldSpot].id = theIndex;
		(*StringCache)[oldSpot].used = ticks;
		(*StringCache)[oldSpot].persId = curPersId;
	}

	return (ProxifyStr(theString, theIndex));
}

/**********************************************************************
 * SCClear - clear the string cache
 **********************************************************************/
void SCClear(short theId)
{
	short i;
	short n;

	if (StringCache) {
		n = HandleCount(StringCache);
		for (i = 0; i < n; i++)
			if (theId == -1 || (*StringCache)[i].id == theId) {
				(*StringCache)[i].id = 0;
				//break;
			}
	}
}

/**********************************************************************
 * return a string from an STR# resource
 **********************************************************************/
PStr GetRStringLo(PStr theString, int theIndex, PersHandle forPers)
{
	StringHandle s;
	short oldRes = CurResFile();

	if (SettingsRefN)
		UseResFile(SettingsRefN);
	if (theIndex != 1001 &&
	    (forPers
	     && (s =
		 GetResource(PERS_TYPE('STR ', (*forPers)->resEnd),
			     theIndex)) && HomeResFile(s) || (!forPers
							      || forPers !=
							      PersList
							      &&
							      !NoDominant)
	     && (s = GetResource('STR ', theIndex)) && HomeResFile(s))) {
		PCopy(theString, *s);
		ReleaseResource_(s);
	} else {
		theString[0] = 0;
		if (!NoDominant || CurPers == PersList)
			GetIndString(theString, 100 * (theIndex / 100),
				     theIndex % 100);
	}
	theString[*theString + 1] = 0;
	UseResFile(oldRes);
	return (theString);
}

/************************************************************************
 * FindSTRNIndex - find a string in a resource id
 ************************************************************************/
short FindSTRNIndex(short resId, PStr string)
{
	UHandle resH = GetResource_('STR#', resId);

	return (FindSTRNIndexRes(resH, string));
}

/**********************************************************************
 * FindSTRNIndexRes - Find a string in an STR# resource
 **********************************************************************/
short FindSTRNIndexRes(UHandle resource, PStr string)
{
	short n = CountStrnRes(resource);
	short i = 0;
	UPtr p;

	if (n && resource && *resource) {
		for (i = 1, p = LDRef(resource) + 2; i <= n;
		     i++, p += *p + 1) {
			if (StringSame(string, p))
				break;
		}
		UL(resource);
		if (i > n)
			i = 0;
	}
	return (i);
}

/************************************************************************
 * FindSTRNSubIndex - find a substring in a resource id
 ************************************************************************/
short FindSTRNSubIndex(short resId, PStr string)
{
	UHandle resH = GetResource_('STR#', resId);

	return (FindSTRNSubIndexRes(resH, string));
}

/**********************************************************************
 * FindSTRNSubIndexRes - Find a substring in an STR# resource
 **********************************************************************/
short FindSTRNSubIndexRes(UHandle resource, PStr string)
{
	short n = CountStrnRes(resource);
	short i = 0;
	UPtr p;

	if (n && resource && *resource) {
		for (i = 1, p = LDRef(resource) + 2; i <= n;
		     i++, p += *p + 1) {
			if (string + 1 ==
			    PPtrFindSub(p, string + 1, *string))
				break;
		}
		UL(resource);
		if (i > n)
			i = 0;
	}
	return (i);
}

/**********************************************************************
 * CountStrnRes - count strings in an STR# resource, given the resource
 **********************************************************************/
short CountStrnRes(UHandle resH)
{
	if (resH && *resH)
		return ((*resH)[0] * 256 + (*resH)[1]);
	else
		return (0);
}

/**********************************************************************
 * Get a color out of a resource file, and darken for text
 **********************************************************************/
RGBColor *GetRTextColor(RGBColor * color, int index)
{
	return DarkenColor(GetRColor(color, index), GetRLong(TEXT_DARKER));
}

/**********************************************************************
 * Get a color out of a resource file
 **********************************************************************/
RGBColor *GetRColor(RGBColor * color, int index)
{
	Str255 scratch;
	Str31 token;
	long aLong;
	UPtr spot;

	Zero(*color);
	if (GetRString(scratch, index)) {
		spot = scratch + 1;
		if (PToken(scratch, token, &spot, ",")) {
			StringToNum(token, &aLong);
			color->red = (short) (aLong & 0xffff);
			if (PToken(scratch, token, &spot, ",")) {
				StringToNum(token, &aLong);
				color->green = (short) (aLong & 0xffff);
				if (PToken(scratch, token, &spot, ",")) {
					StringToNum(token, &aLong);
					color->blue =
					    (short) (aLong & 0xffff);
					return (color);
				}
			}
		}
	}
	return (nil);
}

/**********************************************************************
 * Color2String - convert a color to a string
 **********************************************************************/
PStr Color2String(PStr string, RGBColor * color)
{
	ComposeString(string, "\p%d,%d,%d", color->red, color->green,
		      color->blue);
	return (string);
}

/**********************************************************************
 * Get a long out of a resource file
 **********************************************************************/
long GetRLong(int index)
{
	Str255 scratch;
	long aLong;

	if (GetRString(scratch, index) == nil)
		return (0L);
	else {
		StringToNum(scratch, &aLong);
		return (aLong);
	}
}

/**********************************************************************
 * Get an OSType out of a resource file
 **********************************************************************/
OSType GetROSType(int index)
{
	Str255 scratch;
	long len;
	OSType theType;
	char *src;
	char *dst;
	long i;

	if (GetRString(scratch, index) == nil)
		return ((OSType) 0);
	len = *scratch;
	if (len > 4)
		len = 4;
	else if (len < 4) {
		for (i = len, dst = ((char *) &theType) + 3; i < 4; i++)
			*dst-- = ' ';
	}
	for (i = 0, src = scratch + 1, dst = (char *) &theType; i < len;
	     i++)
		*dst++ = *src++;
	return theType;
}

/************************************************************************
 * RemoveChar - remove a char from some text
 ************************************************************************/
long RemoveChar(Byte c, UPtr text, long size)
{
	UPtr from, to, limit;

	for (to = text, limit = text + size; to < limit && *to != c; to++);
	if (to < limit)
		for (from = to; from < limit; from++)
			if (*from != c)
				*to++ = *from;
	return (to - text);
}

/************************************************************************
 * RemoveCharHandle - remove a character from a handle
 ************************************************************************/
long RemoveCharHandle(Byte c, UHandle text)
{
	long len = GetHandleSize(text);
	long newLen = RemoveChar(c, LDRef(text), len);

	UL(text);
	if (newLen < len)
		SetHandleBig(text, newLen);
	return (newLen);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AddLf(Handle text)
{
	UPtr spot, end;
	long len = GetHandleSize_(text);
	long newLen = len;

	end = *text + len;
	for (spot = *text; spot < end; spot++)
		if (*spot == '\015')
			newLen++;

	if (newLen > len) {
		SetHandleBig(text, newLen);
		if (MemError())
			return (MemError());
		end = *text + newLen;
		for (spot = *text + len - 1; spot > *text; spot--) {
			if (*spot == '\015')
				*--end = '\012';
			*--end = *spot;
		}
	}
	return (noErr);
}

/************************************************************************
 * GetRStr - get a string from an 'STR ' resource
 ************************************************************************/
UPtr GetRStr(UPtr string, short id)
{
	Handle strH = GetString(id);
	if (strH) {
		PCopy(string, *strH);
		ReleaseResource_(strH);
	} else
		*string = 0;
	return (string);
}


/**********************************************************************
 * CountChars - count characters in a handle
 **********************************************************************/
long CountChars(Handle text, Byte c)
{
	long count = CountCharsPtr(LDRef(text), GetHandleSize(text), c);
	UL(text);
	return (count);
}

/**********************************************************************
 * CountCharsPtr - count characters in a pointer
 **********************************************************************/
long CountCharsPtr(UPtr ptr, long size, Byte c)
{
	short n = 0;
	UPtr end = ptr + size;

	while (ptr < end)
		if (*ptr++ == c)
			n++;

	return (n);
}

/**********************************************************************
 * HandleLineBreaks - figure out linebreaks in a block of text
 **********************************************************************/
OSErr HandleLinebreaks(Handle text, long ***breaks, short inWidth)
{
	Fixed width;
	short n = 0;
	UPtr ptr;
	long len;
	long textOffset;
	long cum = 0;
	OSErr err;

	if ((*breaks = NuHTempBetter(0)) == nil)
		return (MemError());
	if (!text || !(len = GetHandleSize(text)))
		return (noErr);

	ptr = LDRef(text);
	while (len) {
		width = inWidth << 16;
		textOffset = 1;
		StyledLineBreak(ptr, len, 0, len, 0, &width, &textOffset);
		len -= textOffset;
		ptr += textOffset;
		cum += textOffset;
		if (err = PtrPlusHand_(&cum, *breaks, sizeof(cum)))
			return (err);
	}
	UL(text);

	return (noErr);
}



/************************************************************************
 * TransLitString - transliaterate with the default viewing table
 ************************************************************************/
void TransLitString(UPtr string)
{
	short id = GetPrefLong(PREF_IN_XLATE);
	Handle xlh;
	UPtr end;
	UPtr table;

	if (id != NO_TABLE && (xlh = GetResource_('taBL', id))) {
		table = LDRef(xlh);
		for (end = string + *string; end > string; end--)
			*end = table[*end];
		UL(xlh);
	}
}

/************************************************************************
 * TransLitRes - translit, fetching table from a resource
 ************************************************************************/
void TransLitRes(UPtr string, long len, short resId)
{
	Handle res = GetResource_('taBL', resId);

	if (res) {
		TransLit(string, len, LDRef(res));
		UL(res);
	}
}

/************************************************************************
 * TransLit - transliterate some chars
 ************************************************************************/
void TransLit(UPtr string, long len, UPtr table)
{
	UPtr end = string + len;

	for (end = string + len; string < end; string++)
		*string = table[*string];
}

/**********************************************************************
 * ScriptVar - return a script variable
 **********************************************************************/
long ScriptVar(short selector)
{
	long result = GetScriptVariable(smSystemScript, selector);

	// apple won't tell us what the small system font is yet
	if (!result && selector == smScriptSmallSysFondSize)
		result = 0x0001000A;

	return (result);
}

#define StackSpot(s,n) ((UPtr)*s+sizeof(StackType)+(n)*(*stack)->elSize)

#pragma segment Stack
/**********************************************************************
 * StackInit - initialize a stack
 *  First four bytes are the element size
 *  Second four bytes are the # of valid elements in the stack
 **********************************************************************/
OSErr StackInit(long size, StackHandle * stack)
{
	*stack = NuHTempBetter(sizeof(StackType));
	if (!*stack)
		return (MemError());
	(**stack)->elSize = size;
	(**stack)->elCount = 0;
	return (noErr);
}

/**********************************************************************
 * StackPush - push an item onto a stack
 **********************************************************************/
OSErr StackPush(void *what, StackHandle stack)
{
	if (!stack)
		return fnfErr;
	else {
		short nSpace =
		    (GetHandleSize_(stack) -
		     sizeof(StackType)) / (*stack)->elSize;

		ASSERT(nSpace >= (*stack)->elCount);
		if (nSpace == (*stack)->elCount) {
			SetHandleBig_(stack,
				      GetHandleSize_(stack) +
				      20 * (*stack)->elSize);
			if (MemError())
				return (MemError());
		}
		BMD(what, StackSpot(stack, (*stack)->elCount),
		    (*stack)->elSize);
		(*stack)->elCount++;
		return (noErr);
	}
}

/**********************************************************************
 * StackQueue - queue an item onto a bottom of stack
 **********************************************************************/
OSErr StackQueue(void *what, StackHandle stack)
{
	if (!stack)
		return fnfErr;
	else {
		short nSpace =
		    (GetHandleSize_(stack) -
		     sizeof(StackType)) / (*stack)->elSize;

		ASSERT(nSpace >= (*stack)->elCount);
		if (nSpace == (*stack)->elCount) {
			SetHandleBig_(stack,
				      GetHandleSize_(stack) +
				      20 * (*stack)->elSize);
			if (MemError())
				return (MemError());
		}
		BMD(StackSpot(stack, 0), StackSpot(stack, 1),
		    (*stack)->elCount * (*stack)->elSize);
		BMD(what, StackSpot(stack, 0), (*stack)->elSize);
		(*stack)->elCount++;
		return (noErr);
	}
}

/**********************************************************************
 * StackPop - pop an item off a stack
 **********************************************************************/
OSErr StackPop(void *into, StackHandle stack)
{
	if (!stack || !(*stack)->elCount)
		return (fnfErr);
	(*stack)->elCount--;
	if (into)
		BMD(StackSpot(stack, (*stack)->elCount), into,
		    (*stack)->elSize);
	return (noErr);
}

/**********************************************************************
 * StackTop - fetch top stack item
 **********************************************************************/
OSErr StackTop(void *into, StackHandle stack)
{
	if (!stack || !(*stack)->elCount)
		return (fnfErr);
	if (into)
		BMD(StackSpot(stack, (*stack)->elCount - 1), into,
		    (*stack)->elSize);
	return (noErr);
}

/**********************************************************************
 * StackItem - fetch a stack item
 **********************************************************************/
OSErr StackItem(void *into, short item, StackHandle stack)
{
	if (!stack || !(*stack)->elCount || item >= (*stack)->elCount)
		return (fnfErr);
	if (into)
		BMD(StackSpot(stack, item), into, (*stack)->elSize);
	return (noErr);
}

/**********************************************************************
 * StackCompact - get rid of waste space
 **********************************************************************/
void StackCompact(StackHandle stack)
{
	if (!stack)
		return;
	SetHandleBig_(stack,
		      (*stack)->elCount * (*stack)->elSize +
		      sizeof(StackType));
}

/**********************************************************************
 * StackStringFind - find an item in the stack
 **********************************************************************/
short StackStringFind(PStr find, StackHandle stack)
{
	Str255 s;
	short item;

	if (!stack)
		return -1;

	for (item = (*stack)->elCount; item > 0;) {
		item--;
		StackItem(s, item, stack);
		if (StringSame(s, find))
			return item;
	}

	return -1;
}

#pragma segment AssocArray
#define AAElemSize(aa) ((*(aa))->dataSize+(*(aa))->keySize)
#define AAKeySpot(aa,indx) ((UPtr)(*(aa))+sizeof(AssocArray)+((indx)-1)*AAElemSize(aa))
#define AADataSpot(aa,indx) (AAKeySpot(aa,indx)+(*(aa))->keySize)
/************************************************************************
 * AANew - create an associative array
 ************************************************************************/
AAHandle AANew(short keySize, short dataSize)
{
	AAHandle aa = NewZHTB(AssocArray);
	if (aa) {
		(*aa)->keySize = keySize;
		(*aa)->dataSize = dataSize;
	}
	return (aa);
}

/************************************************************************
 * AAAddItem - add an item to an associative array
 ************************************************************************/
OSErr AAAddItem(AAHandle aa, Boolean replace, PStr key, UPtr data)
{
	short count = AACountItems(aa);
	short spot = AAFindKey(aa, key);
	Str255 lwrKey;

	if (spot > 0) {
		/* already exists */
		if (!replace)
			return (1);
	} else {
		spot *= -1;	/* spot now is the index of the item just after us */
		SetHandleBig_(aa, GetHandleSize_(aa) + AAElemSize(aa));
		if (MemError())
			return (MemError());
		if (spot && spot <= count)	/* move old data */
			BMD(AAKeySpot(aa, spot),
			    AAKeySpot(aa, spot) + AAElemSize(aa),
			    AAElemSize(aa) * (count - spot + 1));
	}
	BMD(data, AADataSpot(aa, spot), (*aa)->dataSize);
	PCopy(lwrKey, key);
	MyLowerStr(lwrKey);
	BMD(lwrKey, AAKeySpot(aa, spot), (*aa)->keySize);
	return (noErr);
}

/************************************************************************
 * AAAddResItem - add an item to an associative array, using a resource for a key
 ************************************************************************/
OSErr AAAddResItem(AAHandle aa, Boolean replace, short keyId, UPtr data)
{
	Str255 key;

	GetRString(key, keyId);
	return (AAAddItem(aa, replace, key, data));
}

/************************************************************************
 * AADeleteKey - delete an item by key
 ************************************************************************/
OSErr AADeleteKey(AAHandle aa, PStr key)
{
	short spot = AAFindKey(aa, key);
	if (spot > 0) {
		short count = AACountItems(aa);
		if (spot < count)
			BMD(AAKeySpot(aa, spot + 1), AAKeySpot(aa, spot),
			    AAElemSize(aa) * (count - spot));
		SetHandleBig_(aa, GetHandleSize_(aa) - AAElemSize(aa));
		return (noErr);
	} else
		return (1);	/* not found */
}

/************************************************************************
 * AAFetchData - fetch data from an assoc array, by key
 ************************************************************************/
OSErr AAFetchData(AAHandle aa, PStr key, UPtr data)
{
	short spot = AAFindKey(aa, key);
	if (spot > 0) {
		BMD(AADataSpot(aa, spot), data, (*aa)->dataSize);
		return (noErr);
	}
	return (1);		/* not found */
}

/************************************************************************
 * AAFetchResData - fetch data from an assoc array, by a resource key
 ************************************************************************/
OSErr AAFetchResData(AAHandle aa, short keyId, UPtr data)
{
	Str255 key;

	return (AAFetchData(aa, GetRString(key, keyId), data));
}

/************************************************************************
 * AAFetchIndData - fetch data from an assoc array, by index
 ************************************************************************/
OSErr AAFetchIndData(AAHandle aa, short index, UPtr data)
{
	BMD(AADataSpot(aa, index), data, (*aa)->dataSize);
	return (noErr);
}

/************************************************************************
 * AAFetchIndKey - fetch key from an assoc array, by index
 ************************************************************************/
OSErr AAFetchIndKey(AAHandle aa, short index, PStr key)
{
	BMD(AAKeySpot(aa, index), key, (*aa)->keySize);
	return (noErr);
}

/************************************************************************
 * AAFindKey - find a key in an associative array
 *  returns:
 *		positive: index of found item
 *    negative: index of smallest item > current item
 ************************************************************************/
short AAFindKey(AAHandle aa, PStr key)
{
	short count = AACountItems(aa);
	short first = 1;
	short last = count;
	short mid;
	short greater = count + 1;
	short result;
	Str255 lwrKey;

	PCopy(lwrKey, key);
	MyLowerStr(lwrKey);

	LDRef(aa);

	while (first <= last) {
		mid = (first + last) / 2;
		result = StringComp(lwrKey, AAKeySpot(aa, mid));
		if (result == 0) {
			greater = -mid;	/* found it! */
			break;
		} else if (result < 0) {
			greater = mid;
			last = mid - 1;
		} else
			first = mid + 1;
	}

	UL(aa);
	return (-greater);
}

/************************************************************************
 * AACountItems - count the items in an associative array
 ************************************************************************/
short AACountItems(AAHandle aa)
{
	if (!aa || !*aa)
		return (-1);
	return ((GetHandleSize_(aa) -
		 sizeof(AssocArray)) / ((*aa)->keySize + (*aa)->dataSize));
}

/**********************************************************************
 * AccuInit - initialize an accumulator
 **********************************************************************/
OSErr AccuInit(AccuPtr a)
{
	a->offset = 0;
	a->size = 1 K;
	a->data = NuHTempBetter(a->size);
	return (a->err = MemError());
}

/**********************************************************************
 * AccuInitWithHandle - make an accumulator out of an existing handle
 **********************************************************************/
void AccuInitWithHandle(AccuPtr a, Handle h)
{
	a->size = a->offset = GetHandleSize(h);
	a->data = h;
}

/************************************************************************
 * AccuWrite - write an accumulator to a file
 ************************************************************************/
OSErr AccuWrite(AccuPtr a, short refN)
{
	long count = a->offset;
	OSErr err;

	if (!count)
		return (noErr);
	err = AWrite(refN, &count, LDRef(a->data));
	UL(a->data);
	return (err);
}

/************************************************************************
 * AccuFTell - tell the position of the file pointer, assuming that the bytes
 * in the accumulator had been written
 ************************************************************************/
long AccuFTell(AccuPtr a, short refN)
{
	long spot;

	GetFPos(refN, &spot);
	return (spot + a->offset);
}

/************************************************************************
 * AccuFSeek - move the pointer back to a given spot
 ************************************************************************/
OSErr AccuFSeek(AccuPtr a, short refN, long spot)
{
	long curSpot = AccuFTell(a, refN);
	if (curSpot - spot <= a->offset) {
		a->offset -= curSpot - spot;
		return (0);
	} else {
		a->offset = 0;
		return (SetFPos(refN, fsFromStart, spot));
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void AccuTrim(AccuPtr a)
{
	OSErr err;

	if (!a->data && (err = AccuInit(a)))
		return;

#ifdef DEBUG
	if (RunType != Production) {
		if (a->size != GetHandleSize(a->data)
		    || a->size < a->offset) {
			Dprintf("\po %d s %d hs %d h %x", a->offset,
				a->size, GetHandleSize(a->data), a->data);
		}
	}
#endif

	SetHandleBig(a->data, a->offset);
	a->size = a->offset;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddChar(AccuPtr a, Byte c)
{
	return (AccuAddPtr(a, &c, 1));
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddLong(AccuPtr a, uLong longVal)
{
	return (AccuAddPtr(a, &longVal, sizeof(longVal)));
}

/**********************************************************************
 * AccuAddPtr64 - add some data, base-64 encoded
 **********************************************************************/
OSErr AccuAddPtrB64(AccuPtr a, void *bytes, long len)
{
	long newLen = len * 4 / 3 + 4;
	Handle encoded = NuHandle(newLen);
	OSErr err;

	if (!encoded)
		return MemError();

	Encode64DataPtr(LDRef(encoded), &newLen, bytes, len);
	ASSERT(newLen <= len * 4 / 3 + 4);

	err = AccuAddPtr(a, *encoded, newLen);

	ZapHandle(encoded);

	return err;
}

/**********************************************************************
 * AccuAddTrPtr - add to an accumulator, but translate first
 **********************************************************************/
OSErr AccuAddTrPtr(AccuPtr a, void *bytes, long len, UPtr from, UPtr to)
{
	OSErr err;

	// translate
	TrLo(bytes, len, from, to);

	// add
	err = AccuAddPtr(a, bytes, len);

	// untranslate
	TrLo(bytes, len, to, from);

	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddPtr(AccuPtr a, void *bytes, long len)
{
	OSErr err;

	if (!a->data && (err = AccuInit(a)))
		return (err);

#ifdef DEBUG
	if (RunType != Production) {
		if (a->size != GetHandleSize(a->data)
		    || a->size < a->offset) {
			Dprintf("\po %d s %d hs %d h %x", a->offset,
				a->size, GetHandleSize(a->data), a->data);
		}
	}
#endif

	if (a->offset + len > a->size) {
		a->size += len + 4 K;
		SetHandleBig_(a->data, a->size);
		if (MemError())
			return (a->err = MemError());
	}
	BMD(bytes, *a->data + a->offset, len);
	a->offset += len;
	return (noErr);
}

/**********************************************************************
 * AccuAddSortedLong - add a long to an accumulator, but keep it sorted
 **********************************************************************/
OSErr AccuAddSortedLong(AccuPtr a, long addVal)
{
	OSErr err = AccuAddPtr(a, &addVal, sizeof(addVal));

	if (!err) {
		long *start = *a->data;	// start of data
		long *spot = *a->data + a->offset - sizeof(addVal);	// spot we added addVal
		long *newSpot = spot - 1;	// spot addVal really belongs

		// if the value before us is bigger than we are, we'll need to move
		if (newSpot >= start && *newSpot > addVal) {
			// keep going until the value we're looking at is
			// not greater than the value we're putting in
			while (newSpot >= start && *newSpot > addVal)
				newSpot--;

			// Note: if we wanted to eliminate duplicates, here is the spot
			// if (newSpot>=start && *newSpot==addVal)
			// {
			//       a->offset -= sizeof(addVal);
			//       return noErr;
			// }

			// newSpot now points one BEFORE the proper spot; increment
			newSpot++;

			// move everything above us to make room
			BMD(newSpot, newSpot + 1,
			    sizeof(addVal) * (spot - newSpot));

			// and put us into place
			*newSpot = addVal;
		}
	}

	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddRes(AccuPtr a, short res)
{
	Str255 s;

	GetRString(s, res);
	return (AccuAddStr(a, s));
}

/**********************************************************************
 * AccuAddHandleToPtr - copy some data into a handle and add the handle to the accumulator
 **********************************************************************/
OSErr AccuAddHandleToPtr(AccuPtr a, UPtr data, long size)
{
	Handle h = NuDHTempBetter(data, size);
	OSErr err;
	if (h) {
		err = AccuAddPtr(a, (void *) &h, sizeof(h));
		if (err)
			ZapHandle(h);
	} else
		err = MemError();
	return (err);
}

/**********************************************************************
 * AccuAddTrHandle - add a translated handle
 **********************************************************************/
OSErr AccuAddTrHandle(AccuPtr a, Handle data, UPtr from, UPtr to)
{
	OSErr err;

	Tr(data, from, to);
	err = AccuAddHandle(a, data);
	Tr(data, to, from);
	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddHandle(AccuPtr a, Handle data)
{
	OSErr err;
	long len;

	if (!a->data && (err = AccuInit(a)))
		return (err);

	ASSERT(data);
	if (!data)
		return noErr;

#ifdef DEBUG
	if (RunType != Production) {
		if (a->size != GetHandleSize(a->data)
		    || a->size < a->offset) {
			Dprintf("\po %d s %d hs %d h %x", a->offset,
				a->size, GetHandleSize(a->data), a->data);
		}
	}
#endif

	len = GetHandleSize(data);

	if (a->offset + len > a->size) {
		a->size += len + 4 K;
		SetHandleBig_(a->data, a->size);
		if (MemError())
			return (a->err = MemError());
	}
	BMD(*data, *a->data + a->offset, len);
	a->offset += len;
	return (noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuAddFromHandle(AccuPtr a, Handle data, long offset, long len)
{
	OSErr err;

	if (!a->data && (err = AccuInit(a)))
		return (err);

	if (len < 0)
		len = GetHandleSize(data);

	if (len < 0)
		return len;

#ifdef DEBUG
	if (RunType != Production) {
		if (a->size != GetHandleSize(a->data)
		    || a->size < a->offset) {
			Dprintf("\po %d s %d hs %d h %x", a->offset,
				a->size, GetHandleSize(a->data), a->data);
		}
	}
#endif

	if (a->offset + len > a->size) {
		a->size += len + 4 K;
		SetHandleBig_(a->data, a->size);
		if (MemError())
			return (a->err = MemError());
	}
	BMD(*data + offset, *a->data + a->offset, len);
	a->offset += len;
	return (noErr);
}

/************************************************************************
 * AccuFindPtr - find a pointer in an accumulator
 ************************************************************************/
long AccuFindPtr(AccuPtr a, UPtr stuff, short len)
{
	UPtr spot;
	UPtr end;

	if (!a->data || !len)
		return -1;

	spot = *a->data;
	end = spot + a->offset - len;

	for (; spot <= end; spot += len)
		if (!memcmp(spot, stuff, len))
			return spot - *a->data;

	return -1;
}

/************************************************************************
 * AccuFindLong - find a long in an accumulator; returns an index
 ************************************************************************/
long AccuFindLong(AccuPtr a, uLong theLong)
{
	uLong *spot;
	uLong *end;

	if (!a->data)
		return -1;

	spot = (uLong *) * a->data;
	end = (uLong *) spot + (a->offset - sizeof(uLong)) / sizeof(uLong);

	for (; spot <= end; spot++)
		if (*spot == theLong)
			return spot - (uLong *) * a->data;

	return -1;
}

/************************************************************************
 * DecodeB64Accu - decode a base64 accumulator
 ************************************************************************/
short DecodeB64Accu(AccuPtr a, Boolean isText)
{
	Dec64 d64;
	Handle data = NuHandle((3 * a->offset) / 4 + 4);
	long len;
	long result;

	if (!data)
		return MemError();
	if (!a->offset)
		return noErr;

	Zero(d64);
	result =
	    Decode64(LDRef(a->data), a->offset, LDRef(data), &len, &d64,
		     isText);
	if ((d64.decoderState + d64.padCount) % 4)
		result++;

	if (!result) {
		a->offset = len;
		BMD(*data, *a->data, len);
	}

	ZapHandle(data);
	UL(a->data);

	return (result);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuInsertChar(AccuPtr a, Byte c, long offset)
{
	return (AccuInsertPtr(a, &c, 1, offset));
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AccuInsertPtr(AccuPtr a, UPtr bytes, long len, long offset)
{
	OSErr err;

	if (!a->data && (err = AccuInit(a)))
		return (err);

#ifdef DEBUG
	if (RunType != Production) {
		if (a->size != GetHandleSize(a->data)
		    || a->size < a->offset) {
			Dprintf("\po %d s %d hs %d h %x", a->offset,
				a->size, GetHandleSize(a->data), a->data);
		}
	}
#endif

	if (a->offset + len > a->size) {
		a->size += len + 4 K;
		SetHandleBig_(a->data, a->size);
		if (MemError())
			return (a->err = MemError());
	}
	BMD(*a->data + offset, *a->data + offset + len,
	    a->offset - offset);
	BMD(bytes, *a->data + offset, len);
	a->offset += len;
	return (noErr);
}


/**********************************************************************
 * Strip 'n' bytes off of the end of an accumulator
 **********************************************************************/
OSErr AccuStrip(AccuPtr a, long num)
{
	OSErr theError;

	if (!a->data && (theError = AccuInit(a)))
		return (theError);

	a->offset = a->offset > num ? a->offset - num : 0;
	return (noErr);
}


/************************************************************************
 * Long2Hex - write a long in 8 hex bytes
 ************************************************************************/
PStr Long2Hex(PStr hex, long aLong)
{
	Bytes2Hex((void *) &aLong, sizeof(aLong), (void *) (hex + 1));
	*hex = 8;
	return (hex);
}

char *hexdig = "0123456789ABCDEF";
/************************************************************************
 * Bytes2Hex - encode some bytes in hex
 ************************************************************************/
UPtr Bytes2Hex(UPtr bytes, long size, UPtr hex)
{
	Byte c;
	UPtr spot = hex;

	while (size--) {
		c = *bytes++;
		*spot++ = hexdig[(c >> 4) & 0xf];
		*spot++ = hexdig[c & 0xf];
	}

	return (hex);
}

/************************************************************************
 * IsHexDig - is a char a hex digit?
 ************************************************************************/
Boolean IsHexDig(Byte c)
{
	if (islower(c))
		c = toupper(c);
	return ('0' <= c && c <= '9' || 'A' <= c && c <= 'F');
}

#define Hex2Nyb(c) (c<='9'?c-'0':(c>='a'?c-'a'+10:c-'A'+10))
/************************************************************************
 * Hex2Bytes - decode some bytes from hex
 ************************************************************************/
OSErr Hex2Bytes(UPtr hex, long size, UPtr bytes)
{
	Byte hi, lo;

	while (size >= 2) {
		hi = Hex2Nyb(*hex);
		hex++;
		lo = Hex2Nyb(*hex);
		hex++;
		*bytes++ = (hi << 4) | lo;
		size -= 2;
	}
	return (noErr);
}

#pragma segment Main
/************************************************************************
 * MyOSEventAvail - OSEventAvail with resource chain protection
 ************************************************************************/
#undef OSEventAvail
Boolean MyOSEventAvail(short mask, EventRecord * event)
{
	Boolean result;
	MightSwitch();
	result = EventAvail(mask, event);
	AfterSwitch();
	return (result);
}

#define OSEventAvail MyOSEventAvail

/************************************************************************
 * SafeToAllocate - is it safe to allocate this much memory?
 ************************************************************************/
Boolean SafeToAllocate(long size)
{
	static uLong allocated;

	if (size > 4 K || allocated + 50 K > LastContigSpace) {
		allocated = 0;
		PurgeSpace(&LastTotalSpace, &LastContigSpace);
	}
	allocated += size;
	if (LastContigSpace && size + 1 K K > LastContigSpace)
		return (False);
	if (!MemLastFailed)
		return (True);
	if (size > MemLastFailed)
		return (False);
	return (True);
}

/************************************************************************
 * NuHTempOK - New Handle, prefer my heap but temp mem ok
 ************************************************************************/
void *NuHTempOK(long size)
{
	Handle h;
	RANDOM_FAILURE;
	if (!SafeToAllocate(size) || !(h = NuHandle(size)))
		h = NuHTempBetter(size);
	return (h);
}

/**********************************************************************
 * NuDHTempBetter - allocate a handle and copy data into it
 **********************************************************************/
void *NuDHTempBetter(void *data, long size)
{
	Handle h;
	RANDOM_FAILURE;

	h = NuHTempBetter(size);
	if (h)
		BMD(data, *h, size);
	return (h);
}

/**********************************************************************
 * NuDHTempOK - allocate a handle and copy data into it
 **********************************************************************/
void *NuDHTempOK(void *data, long size)
{
	Handle h;

	RANDOM_FAILURE;

	h = NuHTempOK(size);
	if (h)
		BMD(data, *h, size);
	return (h);
}

/************************************************************************
 * NuHTempBetter - New Handle, prefer temp mem
 ************************************************************************/
void *NuHTempBetter(long size)
{
	Handle theMem;
	OSErr err;

	RANDOM_FAILURE;
#ifdef DEBUG
	if (BUG9)
		return (NuHandle(size));
#endif

	if (size > 1 K)
		CompactTempZone();
	theMem = TempNewHandleGlue(size, &err);
	if (!theMem)
		theMem = NuHandle(size);
	else if (size > 1 K)
		MoveHHi(theMem);
	return (theMem);
}

/**********************************************************************
 * 
 **********************************************************************/
void *TempNewHandleGlue(long size, OSErr * err)
{
	return (TempNewHandle(size, err));
}

/**********************************************************************
 * ZeroHandle - clear the contents of a handle
 **********************************************************************/
void *ZeroHandle(void *hand)
{
	Handle h = hand;
	long len;

	if (h && *h) {
		len = GetHandleSize(h);
		WriteZero(LDRef(h), len);
		UL(h);
	}
	return (h);
}

/************************************************************************
 * CompactTempZone - compact the temp memory zone
 ************************************************************************/
void CompactTempZone(void)
{
}

/************************************************************************
 * NewIOBHandle - New IO buffer, Handle
 ************************************************************************/
Handle NewIOBHandle(long min, long max)
{
	Handle theMem;

	CompactTempZone();
	do {
		theMem = NuHTempOK(max);
		max /= 2;
	}
	while (!theMem && max >= min);
	if (theMem)
		MoveHHi(theMem);

	return (theMem);
}

/************************************************************************
 * GetTableCName - get the canonical name of a table
 ************************************************************************/
Boolean GetTableCName(short tid, PStr name)
{
	UHandle res;
	short spot, end;
	short idFromResource;
	short rIndex;

	for (rIndex = 1; res = GetIndResource_('euTM', rIndex); rIndex++) {
		HNoPurge_(res);
		end = GetHandleSize_(res);
		spot = 0;
		for (spot = 0; spot < end; spot += (*res)[spot + 2] + 3) {
			idFromResource = (*res)[spot] * 256 + (*res)[spot + 1];	/* read id */
			if (idFromResource == tid) {	/* found it */
				PCopy(name, (*res) + spot + 2);	/* copy name */
				HPurge(res);
				return (True);
			}
		}
		HPurge(res);
	}
	return (False);
}

/************************************************************************
 * GetTableID - get the id of a named table
 ************************************************************************/
Boolean GetTableID(PStr name, short *tid)
{
	UHandle res;
	short spot, end;
	Str63 nameFromResource;
	short rIndex;

	for (rIndex = 1; res = GetIndResource_('euTM', rIndex); rIndex++) {
		HNoPurge_(res);
		end = GetHandleSize_(res);
		spot = 0;
		for (spot = 0; spot < end; spot += (*res)[spot + 2] + 3) {
			PCopy(nameFromResource, (*res) + spot + 2);
			if (StringSame(nameFromResource, name)) {
				*tid = (*res)[spot] * 256 + (*res)[spot + 1];	/* read id */
				HPurge(res);
				return (True);
			}
		}
		HPurge(res);
	}
	return (False);
}

#pragma segment Main
/************************************************************************
 * EventPending - is an event waiting for us?
 ************************************************************************/
Boolean EventPending(void)
{
	EventRecord event;
	static uLong ticks;

	if (TickCount() - ticks <= 8)
		return (False);
	else {
		ticks = TickCount();
		return (OSEventAvail
			(mUpMask | mDownMask | keyDownMask | updateMask |
			 activMask | osMask, &event));
	}
}

#ifdef DEBUG
#undef UseResFile
void MyUseResFile(short refN)
{
	FSSpec oldSpec, newSpec;
	short oldRefN;

	if (BUG8) {
		oldRefN = CurResFile();
		if (oldRefN != refN) {
			Zero(oldSpec);
			Zero(newSpec);
			GetFileByRef(refN, &newSpec);
			GetFileByRef(oldRefN, &oldSpec);
			Dprintf("\pUseResFile �%p� -> �%p�;g",
				oldSpec.name, newSpec.name);
		}
	}
	UseResFile(refN);
}

/************************************************************************
 * RESCHK - verify that a resource is a resource
 ************************************************************************/
void RESCHK(OSType type, short resId);
void RESCHK(OSType type, short resId)
{
	Handle resH = GetResource_(type, resId);
	short flags;

	if (resH) {
		flags = HGetState(resH);
		ASSERT(flags & 32);
	}
}
#endif


#define		gestaltGatewayExternal		'VIGE'
/************************************************************************
 * IsVICOM - return true if VICOM Internet gateway is running
 ************************************************************************/
Boolean IsVICOM(void)
{
	long interfaceVICOM = 0;

	if (Gestalt(gestaltGatewayExternal, &interfaceVICOM) == noErr)
		return (interfaceVICOM != 0);
	else
		return (false);
}

/************************************************************************
 * MyRemoveResource - fix bug in RemoveResource:
 *   it doesn't work if CurResFile is not set to the resource's resource file
 ************************************************************************/
#undef RemoveResource
OSErr MyRemoveResource(Handle h)
{
	short useFile, saveFile = CurResFile();
	OSErr err = noErr;

	if (h) {
		useFile = HomeResFile(h);
		if (!(err = ResError())) {
			UseResFile(useFile);
			RemoveResource(h);
			err = ResError();
			UseResFile(saveFile);
		}
	}
	return err;
}

/************************************************************************
 * FinderDragVoodoo - hack around a Finder 8.1 (at least) bug
 ************************************************************************/
OSErr FinderDragVoodoo(DragReference drag)
{
	// This magic incantation seems to avoid a bug in (at least) the 8.1 finder
	// which seems to sometimes go boom if the first drag it sees is non-TEXT,
	// promised, sender-only, and not saved.
	return (AddDragItemFlavor
		(drag, 1L, 'xyzy', "", 0, flavorNotSaved));
}

/**********************************************************************
 * ShortCompare - compare two shorts, return 0 if equal, -1 if value1 < value2, 1 if value1 2 value2
 **********************************************************************/
short ShortCompare(short value1, short value2)
{
	if (value1 == value2)
		return 0;
	if (value1 < value2)
		return -1;
	return 1;
}

/************************************************************************
 * DateCompare - compare two dates 
 *		return 0 if equal, -1 if date1 < date2, 1 if date1 2 date2
 *  Doesn't compare time portion of DateTimeRec
 ************************************************************************/
short DateCompare(DateTimeRec * date1, DateTimeRec * date2)
{
	short result;

	if (!(result = ShortCompare(date1->year, date2->year)))
		if (!(result = ShortCompare(date1->month, date2->month)))
			result = ShortCompare(date1->day, date2->day);
	return result;
}

/************************************************************************
 * TimeCompare - compare two dates including time
 *		return 0 if equal, -1 if date1 < date2, 1 if date1 2 date2
 ************************************************************************/
short TimeCompare(DateTimeRec * date1, DateTimeRec * date2)
{
	short result;

	if (!(result = DateCompare(date1, date2)))
		if (!(result = ShortCompare(date1->hour, date2->hour)))
			if (!
			    (result =
			     ShortCompare(date1->minute, date2->minute)))
				result =
				    ShortCompare(date1->second,
						 date2->second);
	return result;
}


//#define IsColorWin(win) \
//      (ThereIsColor && \
//       (((GrafPtr)(win))->portBits.rowBytes & 0xC000) && \
//   ((**((CGrafPtr)(win))->portPixMap).pixelSize > 1))
Boolean IsColorWin(WindowPtr winWP)
{
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	CGrafPtr thePort = GetWindowPort(winWP);
	Boolean isThere;	// well?

	isThere = false;
	if (ThereIsColor && thePort)
		isThere = GetPortPixelDepth(thePort) > 1;
	return (isThere);
}

/************************************************************************
 * GetOSVersion - get OS version in BCD format: 9.01 = 0x0901
 ************************************************************************/
short GetOSVersion(void)
{
	static short sysVers;
	long result;

	if (!sysVers) {
		if (!Gestalt(gestaltSystemVersion, &result))
			sysVers = result;
	}
	return (sysVers);
}

/************************************************************************
 * HaveOSX - are we running Mac OS X or bettter
 ************************************************************************/
Boolean HaveOSX(void)
{
	return GetOSVersion() >= 0x1000;
}

#ifdef DEBUG
#undef GetResource
#undef Get1Resource
#undef GetNamedResource
#undef Get1NamedResource
#undef GetMenu
#undef Get1IndResource
#undef GetIndResource
Handle NoSLGetResource(OSType type, short id)
{
	Handle h;
	SLDisable();
	h = GetResource(type, id);
	SLEnable();
	return h;
}

Handle NoSLGet1Resource(OSType type, short id)
{
	Handle h;
	SLDisable();
	h = Get1Resource(type, id);
	SLEnable();
	return h;
}

Handle NoSLGet1IndResource(OSType type, short ind)
{
	Handle h;
	SLDisable();
	h = Get1IndResource(type, ind);
	SLEnable();
	return h;
}

Handle NoSLGetIndResource(OSType type, short ind)
{
	Handle h;
	SLDisable();
	h = GetIndResource(type, ind);
	SLEnable();
	return h;
}

Handle NoSLGetNamedResource(OSType type, PStr name)
{
	Handle h;
	SLDisable();
	h = GetNamedResource(type, name);
	SLEnable();
	return h;
}

Handle NoSLGet1NamedResource(OSType type, PStr name)
{
	Handle h;
	SLDisable();
	h = Get1NamedResource(type, name);
	SLEnable();
	return h;
}

MenuHandle NoSLGetMenu(short id)
{
	MenuHandle h;
	SLDisable();
	h = GetMenu(id);
	SLEnable();
	return h;
}

MenuHandle NoSLGetMHandle(short id)
{
	MenuHandle h;
	SLDisable();
	h = GetMenuHandle(id);
	SLEnable();
	return h;
}
#endif
