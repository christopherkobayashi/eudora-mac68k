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

#include <conf.h>
#include <mydefs.h>

#ifndef SPELL_H
#include "spell.h"
#include "ssce.h"
#endif

#pragma segment Spell

#ifdef WINTERTREE
#define FILE_NUM 74

#define SPELL_OPT_MASK	0xf
#define SPELL_SUGGEST_BOTH (1L<<20)

Boolean SpellWordKnown(PStr word,PStr replace);
short SpellOpenLexDir(FSSpecPtr inSpec);
OSErr SpellOpenLex(FSSpecPtr spec);
void SpellGetCurrentWord(PStr word,Boolean *checked,Boolean *writeable, Boolean *misspelled,long *start,long *stop);
void SpellSuggestions(PStr word,UHandle *suggestions);
OSErr SpellReplace(PETEHandle pte,PStr replaceMe);
OSErr SpellRemove(PETEHandle pte);
OSErr SpellAdd(PETEHandle pte);
OSErr SpellNext(PETEHandle pte);
OSErr SpellMakeUserDict(FSSpecPtr dirSpec,short id);
static SpellInUse;
static SpellUserDict = -1;
static SpellUserADict = -1;
static SpellTicks;

/************************************************************************
 * SpellOpen - open a spelling session
 ************************************************************************/
short SpellOpen(void)
{
	FSSpec mySpec;
	short howMany=0;
	long domains[]={kNetworkDomain,kSystemDomain,kLocalDomain,kUserDomain};
	FSSpec oldSpec;
	short n = sizeof(domains)/sizeof(domains[0]);

	Zero(oldSpec);
	
	if (!HaveSpeller()) return(-1);
	
	SpellInUse++;
	
	if (SpellSession<0)
	{
		SpellSession = SSCE_OpenSession();
		SpellTicks = TickCount();
		if (SpellSession>=0)
		{
			SpellUserDict = SpellUserADict = -1;
			
			// Eudora Folder:Eudora Items:
			mySpec.vRefNum = ItemsFolder.vRef;
			mySpec.parID = ItemsFolder.dirId;
			*mySpec.name = 0;
			if (!SubFolderSpecOf(&mySpec,SPELL_DICTS,false,&mySpec))
				howMany += SpellOpenLexDir(&mySpec);
			
			// Eudora Folder:
			mySpec.vRefNum = Root.vRef;
			mySpec.parID = Root.dirId;
			*mySpec.name = 0;
			if (!SubFolderSpecOf(&mySpec,SPELL_DICTS,false,&mySpec))
				howMany += SpellOpenLexDir(&mySpec);
			
			// Application Support:Eudora:
			while (n-->0)
				if (!FindSubFolderSpec(domains[n],kApplicationSupportFolderType,EUDORA_EUDORA,false,&mySpec))
				if (!SameSpec(&mySpec,&oldSpec))
				{
					oldSpec = mySpec;
					if (!SubFolderSpecOf(&mySpec,SPELL_DICTS,false,&mySpec))
						howMany += SpellOpenLexDir(&mySpec);
				}

			// Eudora Stuff:
			if (!StuffFolderSpec(&mySpec))
			if (!SubFolderSpecOf(&mySpec,SPELL_DICTS,false,&mySpec))
				howMany += SpellOpenLexDir(&mySpec);
			
			// Do we need to make the user dictionaries?
			if (SpellUserDict<0 || SpellUserADict<0)
			{
				mySpec.vRefNum = ItemsFolder.vRef;
				mySpec.parID = ItemsFolder.dirId;
				*mySpec.name = 0;
				if (!SubFolderSpecOf(&mySpec,SPELL_DICTS,true,&mySpec))
				{
					if (SpellUserDict<0)
					{
						SpellMakeUserDict(&mySpec,SPELL_UDICT);
						GetRString(mySpec.name,SPELL_UDICT);
						if (!SpellOpenLex(&mySpec)) howMany++;
					}
					if (SpellUserADict<0)
					{
						SpellMakeUserDict(&mySpec,SPELL_UADICT);
						GetRString(mySpec.name,SPELL_UADICT);
						if (!SpellOpenLex(&mySpec)) howMany++;
					}
				}
			}
			
			// when counting how many dictionaries we got, don't
			// count the user dictionaries, because without any
			// real dictionaries, they're useless
			if (SpellUserDict>=0) howMany--;
			if (SpellUserADict>=0) howMany--;
		}
		if (!howMany) SpellClose(true);
		else WinterTreeOptions = GetPrefLong(PREF_WINTERTREE_OPTS);
	}
	SpellTicks = TickCount();
	return(SpellSession);
}

/************************************************************************
 * SpellOpenLexDir - open all the dictionaries in a folder; returns number opened
 ************************************************************************/
short SpellOpenLexDir(FSSpecPtr inSpec)
{
	CInfoPBRec hfi;
	FSSpec localSpec = *inSpec;
	short howMany = 0;
	
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = localSpec.name;
	while (!DirIterate(localSpec.vRefNum,localSpec.parID,&hfi))
		if (!SpellOpenLex(&localSpec)) howMany++;
	
	return howMany;
}
/************************************************************************
 * SpellMakeUserDict - create the user dictionaries if they don't exist
 ************************************************************************/
OSErr SpellMakeUserDict(FSSpecPtr dirSpec,short id)
{
	FSSpec spec;
	OSErr err;
	Str255 userContents;
	
	spec = *dirSpec;
	
	GetRString(spec.name,id);
	if (!(err=FSpCreate(&spec,SPELLER_CREATOR,'TEXT',smSystemScript)))
	{
		GetRString(userContents,id+2);
		err = BlatPtr(&spec,userContents+1,*userContents,false);
	}
	return(err);
}
	

/************************************************************************
 * SpellOpenLex - open a lexicon
 ************************************************************************/
OSErr SpellOpenLex(FSSpecPtr spec)
{
	Str255 path;
	FSSpec localSpec;
	Str31 name;
	OSErr err = noErr;
	Boolean canWrite;
	short id;
	
	if (!HaveSpeller()) return(fnfErr);
	
	PCopy(path,spec->name);
	localSpec = *spec;
	do
	{
		GetDirName(nil,localSpec.vRefNum,localSpec.parID,name);
		PCatC(name,':');
		PInsert(path,sizeof(path),name,path+1);
		if (localSpec.parID==2) break;
	} while (!(err=ParentSpec(&localSpec,&localSpec)));
	P2CStr(path);
	if (!err)
	{
		id = SSCE_OpenLex(SpellSession,path,GetRLong(SPELL_MEM_LIMIT));
		if (id<0) err = id;
	}
	if (!err)
	{
		if (SpellUserDict<0 && EqualStrRes(spec->name,SPELL_UDICT))
		{
			CanWrite(spec,&canWrite);
			if (canWrite) SpellUserDict = id;
		}
		else if (SpellUserADict<0 && EqualStrRes(spec->name,SPELL_UADICT))
		{
			CanWrite(spec,&canWrite);
			if (canWrite) SpellUserADict = id;
		}
	}
	return(err);
}

/************************************************************************
 * SpellAgain - respell with new options
 ************************************************************************/
void SpellAgain(void)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	PETEHandle pte;
	
	EnableSpellMenu(true);
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsKnownWindowMyWindow(winWP) && (win=GetWindowMyWindowPtr(winWP)))
			for (pte=win->pteList;pte;pte=PeteNext(pte))
				if (!SpellDisabled(pte))
					(*PeteExtra(pte))->spelled = 0;
}

/************************************************************************
 * SpellScan - scan msgs for misspellings and if found style
 ************************************************************************/
void SpellScan(void)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	PETEHandle pte;
	Boolean quoteScanning = AmQuoteScanning();
	Boolean scanned = false;
	
	if (!HaveSpeller()) return;

	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
	{
		if (IsKnownWindowMyWindow(winWP))
		{
			win = GetWindowMyWindowPtr (winWP);
			for (pte=win->pteList;pte;pte=PeteNext(pte))
			{
				while ((*PeteExtra(pte))->spelled >= 0 && (!quoteScanning||(*PeteExtra(pte))->quoteScanned==-1) && (*PeteExtra(pte))->urlScanned==-1)
				{
					UsingWindow(winWP);
					PeteSpellScan(pte,false);
					scanned = true;
					if (NEED_YIELD) {
						UseFeature (featureSpellChecking);
						return;
					}
				}
			}
		}
	}
	if (scanned)
		UseFeature (featureSpellChecking);
}

#define SpellFirstWordChar(x) (IsWordChar[x] || isdigit(x))
#define SpellMidWordChar(x)	(IsWordChar[x] || isdigit(x))
#define SpellSingleOK(x) ((x)==(Byte)'-'||(x)==(Byte)'�'||(x)=='\''||(x)==(Byte)'�')
#define SpellSingleFunnyOK(x) ((x)=='@'||(x=='.'))
#define SpellWordChar(x) (SpellMidWordChar(x)||SpellSingleOK(x))
#define kSpellUnitMask (pURLLabel|pLinkLabel|pQuoteLabel)

/************************************************************************
 * PeteSpellScan - scan a pete record for misspellings
 ************************************************************************/
void PeteSpellScan(PETEHandle pte,Boolean manual)
{
	Handle text;
	long plainStart = (*PeteExtra(pte))->spelled;
	long plainEnd = plainStart;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	PETEStyleEntry pse;
	long lEnd;
	UPtr spot;
	UPtr end;
	long len;
	Byte word[SSCE_MAX_WORD_LEN+2];
	Byte replace[SSCE_MAX_WORD_LEN+2];
	Boolean ok;
	long selStart,selEnd;
	Boolean iFoundAFunny;
	Boolean isWord;
	HeadSpec hs;
	Str63 frogChars;

	if (!HaveSpeller()) return;
	
	// done?
	if (SpellChecked(pte)) return;
	if (0>SpellOpen()) return;
	Zero(pse);
	*replace = 0;
	
	// pre-checks; shd probably be farmed out to gonnaShow or something, but...
	len=PETEGetTextLen(PETE,pte);
	if (GetWindowKind(GetMyWindowWindowPtr((*PeteExtra(pte))->win))==COMP_WIN)
	{
		CompHeadFind(Win2MessH((*PeteExtra(pte))->win),SUBJ_HEAD,&hs);
		lEnd = hs.value;
		if (plainStart<lEnd)
			plainStart = plainEnd = lEnd;
		else if (plainStart>=hs.stop)
		{
			CompHeadFind(Win2MessH((*PeteExtra(pte))->win),0,&hs);
			lEnd = hs.value;
			if (plainStart<lEnd)
				plainStart = plainEnd = lEnd;
		}
	}
	if (plainStart >= len)
	{
		(*PeteExtra(pte))->spelled = sprSpellComplete;	// don't scan again
	}
	if ((*PeteExtra(pte))->spelled<0) return;
		
	// ok, actual spelling now
	NoScannerResets++;	// these changes don't count
	PETEGetStyle(PETE,pte,plainStart,nil,&pse);
	pse.psStyle.textStyle.tsLabel &= ~pStationeryLabel;
	if (PeteIsExcerptAt(pte,plainStart))
	{
		PeteParaRange(pte,&plainStart,&plainEnd);
	}
	else if (pse.psStyle.textStyle.tsLabel&kSpellUnitMask)
	{
		PETEFindLabelRun(PETE,pte,plainStart,&plainStart,&plainEnd,pse.psStyle.textStyle.tsLabel&kSpellUnitMask,pse.psStyle.textStyle.tsLabel&kSpellUnitMask);
	}
	else
	{
		PeteGetTextAndSelection(pte,&text,&selStart,&selEnd);
		spot = *text+plainStart;
		
		// the starting spot might be in the middle of something.  Check.
		if (pse.psStyle.textStyle.tsLabel==pSpellLabel)
			PETEFindLabelRun(PETE,pte,plainStart,&plainStart,&lEnd,pSpellLabel,pSpellLabel);
		else if ((*PeteExtra(pte))->spellReset)
		{
			while (spot>*text && !IsLWSP(spot[-1])) spot--;
			plainStart = spot-*text;
		}
		(*PeteExtra(pte))->spellReset = false;
				
		// reinit in case they changed
		PeteGetTextAndSelection(pte,&text,&selStart,&selEnd);
		spot = *text+plainStart;
		end = *text+len;
		
		isWord = SpellFirstWordChar(*spot);
		spot++;
		
		if (!isWord)
		{
			while (spot<end && !SpellFirstWordChar(*spot)) spot++;
			plainEnd = spot-*text;
		}
		else
		{
			iFoundAFunny = false;
			for (;;)
			{
				while (spot<end && SpellMidWordChar(*spot)) spot++;
				if (spot<end-1 && IsWordChar[spot[1]])
				{
					if (SpellSingleFunnyOK(*spot))	// foo@bar, foo.bar
					{
						iFoundAFunny = true;
						spot++;
					}
					else if (SpellSingleOK(*spot)) // foo-bar, foo'bar, etc
					{
						// Special case for french contractions; c'est la vie, par example
						if (*spot=='\'')
						{
							Byte theChar = spot[1];
							LowercaseText(&theChar,1,smSystemScript);
							if (PIndex(GetRString(frogChars,FROG_CHARS),theChar))
								break;
						}
						spot++;
					}
					else
						break;
				}
				else
					break;
			}
				
			plainEnd = spot-*text;
			if (iFoundAFunny)
				/*nevermind*/;
			else
			{
				ok = true;
				if (plainEnd-plainStart<sizeof(word)-2)
				{
					MakePStr(word,*text+plainStart,plainEnd-plainStart);
					ok = SpellWordKnown(word,replace);
				}
				
				if (!ok && (PeteIsGraphicAt(pte,plainStart) || PeteIsGraphicAt(pte,plainEnd)))
					ok = true;	// avoid graphics
				
				if (!ok)
				{
					if (*replace && !PETESelectionLocked(PETE,pte,plainStart,plainEnd))
					{
						long oldSelStart, oldSelEnd;
						Boolean selWasInWord, selWasWord;
						
						PeteGetTextAndSelection(pte,nil,&oldSelStart,&oldSelEnd);
						selWasInWord = oldSelStart>=plainStart && oldSelEnd <= plainEnd;
						selWasWord = oldSelStart==plainStart && oldSelEnd == plainEnd;
						
						if (!PeteDelete(pte,plainStart,plainEnd))
						{
							plainEnd = plainStart;
							if (!PeteInsertPtr(pte,plainStart,replace+1,*replace))
							{
								plainEnd = plainStart = plainStart+*replace;
								// restore selection
								if (selWasWord) PeteSelect(nil,pte,plainStart,plainEnd);
								else if (selWasInWord) PeteSelect(nil,pte,plainEnd,plainEnd);
							}
						}
					}
					else
					{
						PeteLabel(pte,plainStart,plainEnd,pSpellLabel,pSpellLabel);
						plainStart = plainEnd;
						// mark the character after this one as not misspelled, otherwise
						// we'll scan over and over and over again in certain sitches, eg,
						// xxx-rat -> xxx- rat
						if (plainEnd<len) plainEnd++;
					}
				}
			}
		}
	}
	if (plainStart<plainEnd) PeteNoLabel(pte,plainStart,plainEnd,pSpellLabel);
	if (!*replace)
	{
		if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
	}
	(*PeteExtra(pte))->spelled = plainEnd;
	NoScannerResets--;
	SpellClose(false);
}

/************************************************************************
 * SpellWordKnown - do we know this word?
 ************************************************************************/
Boolean SpellWordKnown(PStr word,PStr replace)
{
	Str255 localWord;
	Str63 goAway;
	short result;
	UPtr spot;
	Boolean allJunk = true;
	
	if (!HaveSpeller()) return(true);
	
	for (spot=word+1;spot<=word+*word;spot++)
		if (IsWordChar[*spot])
		{
			allJunk = false;
			break;
		}
	
	if (allJunk) return true;
	
	PtoCcpy(localWord,word);
	TransLitRes(localWord+1,*localWord,ktFlatten);
	result = SSCE_CheckWord(SpellSession,WinterTreeOptions&0xfff,localWord,goAway,sizeof(goAway));
	if ((result&SSCE_CHANGE_WORD_RSLT) && replace)
	{
		C2PStr(goAway);
		PCopy(replace,goAway);
	}

	SpellTicks = TickCount();
	return(!result);
}

/************************************************************************
 * SpellClose - close a spelling session
 ************************************************************************/
void SpellClose(Boolean withPrejuidice)
{
	if (SpellSession>=0)
	{
		if (SpellInUse) SpellInUse--;
		if ((!SpellInUse && TickCount()-SpellTicks>60*GetRLong(SPELL_HOLD_OPEN_SECS)) || withPrejuidice)
		{
			SSCE_CloseSession(SpellSession);
			SpellSession = -1;
		}
	}
}

/************************************************************************
 * SpellMenu - handle a selection from the spellchecking menu
 ************************************************************************/
void SpellMenu(short item,short modifiers)
{
	MyWindowPtr win = GetWindowMyWindowPtr(FrontWindow_());
	Str63 replace;
	long sel;
	
	if (!HaveSpeller()) return;
	if (win && win->pte)
	{
		switch(item)
		{
			case spellCheckItem:
				PeteGetTextAndSelection(win->pte,nil,&sel,nil);
				PeteSetURLRescan(win->pte,sel);
				{
					SpellOpen();
					if (SpellInUse)
					{
						(*PeteExtra(win->pte))->spelled = 0;
						do
						{
							PeteSpellScan(win->pte,true);
							CycleBalls();
						}
						while (!CommandPeriod && (*PeteExtra(win->pte))->spelled>=0);
						SpellClose(false);
					}
				}
				break;
			case spellNextItem:
				if (!SpellChecked(win->pte))
					SpellMenu(spellCheckItem,0);
				SpellNext(win->pte);
				break;
			case spellAddItem:
				SpellAdd(win->pte);
				break;
			case spellRemoveItem:
				SpellRemove(win->pte);
				break;
			default:
				SpellReplace(win->pte,MyGetItem(GetMHandle(SPELL_HIER_MENU),item,replace));
				break;
		}
	}
	SpellTicks = TickCount();
}

/************************************************************************
 * SpellReplace - replace the current word
 ************************************************************************/
OSErr SpellReplace(PETEHandle pte,PStr replaceMe)
{
	long selStart=0,selEnd=0;
	OSErr err = noErr;
	Boolean selection;
	
	PeteGetTextAndSelection(pte,nil,&selStart,&selEnd);
	selection = selStart!=selEnd;
	SpellGetCurrentWord(nil,nil,nil,nil,&selStart,&selEnd);
	if (selStart!=selEnd)
	{
		PeteSetURLRescan(pte,selStart);
		PeteCalcOff(pte);
		PetePrepareUndo(pte,peUndoPaste,selStart,selEnd,nil,nil);
		PeteSelect(nil,pte,selStart,selEnd);
		if (!(err = PETEInsertTextPtr(PETE,pte,-1,replaceMe+1,*replaceMe,nil)))
		{
			PeteSelect(nil,pte,selection ? selStart:selStart+*replaceMe,selStart+*replaceMe);
			PeteFinishUndo(pte,peUndoPaste,selStart,selStart+*replaceMe);
		}
		if (err) PeteKillUndo(pte);
		PeteCalcOn(pte);
	}
	SpellTicks = TickCount();
	return err;
}

/************************************************************************
 * SpellNext - find the next misspelling
 ************************************************************************/
OSErr SpellNext(PETEHandle pte)
{
	long selStart, selEnd;
	PETEStyleEntry pse;
	long firstStart = -1;
	
	PeteGetTextAndSelection(pte,nil,nil,&selEnd);
	PeteStyleAt(pte,selEnd,&pse);
	if (pse.psStyle.textStyle.tsLabel&pSpellLabel)
		PETEFindLabelRun(PETE,pte,0,&firstStart,&selEnd,pSpellLabel,pSpellLabel);	
	if (!PETEFindLabelRun(PETE,pte,selEnd,&selStart,&selEnd,pSpellLabel,pSpellLabel) ||
			!PETEFindLabelRun(PETE,pte,0,&selStart,&selEnd,pSpellLabel,pSpellLabel) && firstStart!=selStart)
	{
		PeteSelect(nil,pte,selStart,selEnd);
		PETEScroll(PETE,pte,0,pseCenterSelection);
		return(noErr);
	}
	else
	{
		SysBeep(20L);
		return(fnfErr);
	}
	SpellTicks = TickCount();
}

/************************************************************************
 * SpellAdd - add the current word
 ************************************************************************/
OSErr SpellAdd(PETEHandle pte)
{
	OSErr err = fnfErr;
	Str63 word;
	long start,end;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;

	if (!HaveSpeller()) return(fnfErr);

	if (SpellOpen()>=0)
	{
		SpellGetCurrentWord(word,nil,nil,nil,&start,&end);
		if (*word)
		{
			P2CStr(word);
			if (SpellUserADict>=0)
				err = SSCE_DelFromLex(SpellSession,SpellUserADict,word);
			if (err)
				err = SSCE_AddToLex(SpellSession,SpellUserDict,word,nil);
		}
		PeteNoLabel(pte,start,end,pSpellLabel);
		if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
		SpellAgain();
		SpellClose(false);
	}
	SpellTicks = TickCount();
	return(err);
}

/************************************************************************
 * SpellRemove - remove the current word
 ************************************************************************/
OSErr SpellRemove(PETEHandle pte)
{
	OSErr err = fnfErr;
	Str63 word;
	long start,end;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;

	if (!HaveSpeller()) return(fnfErr);

	if (SpellOpen()>=0)
	{
		SpellGetCurrentWord(word,nil,nil,nil,&start,&end);
		if (*word)
		{
			P2CStr(word);
			if (SpellUserDict>=0)
				err = SSCE_DelFromLex(SpellSession,SpellUserDict,word);
			if (err)
				err = SSCE_AddToLex(SpellSession,SpellUserADict,word,nil);
		}
		PeteLabel(pte,start,end,pSpellLabel,pSpellLabel);
		if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
		SpellAgain();
		SpellClose(false);
	}
	SpellTicks = TickCount();
	return(err);
}

/************************************************************************
 * EnableSpellMenu - enable the spell-checking menu items
 ************************************************************************/
void EnableSpellMenu(Boolean all)
{
	static Str63 lastWord;
	Str63 word;
	Boolean checked=false,writeable=false,misspelled=false;
	MenuHandle mh = GetMHandle(SPELL_HIER_MENU);
	UHandle suggestions=nil;
	short i;
	short len;
	Boolean same;
	WindowPtr winWP = FrontWindow_();
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	Boolean have = HaveSpeller();
	short item;
	static wordCanAdd;
	static wordCanRemove;
	static short key;
	static Boolean deferSuggestions;
	
	
	if (mh)
	{
		// diddle the add command
		GetItemCmd(mh,spellCheckItem,&key);
		if (key)
		{
			SetItemCmd(mh,spellNextItem,key);
			SetMenuItemModifiers(mh,spellNextItem,kMenuOptionModifier);
			SetItemCmd(mh,spellAddItem,key);
			SetMenuItemModifiers(mh,spellAddItem,kMenuShiftModifier+kMenuOptionModifier);
		}

		if (!all && have) SpellGetCurrentWord(word,&checked,&writeable,&misspelled,nil,nil);
		else *word = 0;
		
		same = EqualString(word,lastWord,true,true);

		PCopy(lastWord,word);
		
		if (!same) {wordCanAdd = wordCanRemove = 0;TrashMenu(mh,spellItemLimit);}
		
		if (SpellOptGuesses && (!same||deferSuggestions) && *word && misspelled)
		{
			if (MainEvent.what!=mouseDown)
				deferSuggestions = true;
			else
			{
				deferSuggestions = false;	
				PCopy(lastWord,word);
				SpellSuggestions(word,&suggestions);
				if (suggestions)
				{
					item = CountMenuItems(mh);
					AppendMenu(mh,"\p-");
					DisableItem(mh,++item);
					len = GetHandleSize(suggestions);
					for (i=0;i<len;i+=(*suggestions)[i]+1)
					{
						PSCopy(word,(*suggestions)+i);
						MyAppendMenu(mh,word);
						item++;
						EnableIf(mh,item,writeable);
					}
					ZapHandle(suggestions);
				}
			}
		}
		
		if (!same && *word)
		{
			wordCanAdd = SpellUserDict>=0&&misspelled;
			wordCanRemove = (SpellUserDict>=0 || SpellUserADict>=0)&&*word&&checked&&!misspelled;
		}
				
		EnableIf(mh,spellCheckItem,all||have&&IsMyWindow(winWP)&&win&&PeteIsValid(win->pte)&&!SpelledAuto(win->pte));
		EnableIf(mh,spellNextItem,all||have&&IsMyWindow(winWP)&&win&&PeteIsValid(win->pte));
		EnableIf(mh,spellAddItem,all||wordCanAdd);
		EnableIf(mh,spellRemoveItem,all||wordCanRemove);
	}
	SpellTicks = TickCount();
}

/************************************************************************
 * SpellGetCurrentWord - get the current word
 ************************************************************************/
void SpellGetCurrentWord(PStr wordPtr,Boolean *checked,Boolean *writeable, Boolean *misspelled,long *start,long *stop)
{
	WindowPtr	winWP = FrontWindow_();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	PETEHandle pte = win && IsMyWindow(winWP) ? win->pte : nil;
	long selStart,selEnd;
	UPtr spot;
	UHandle text;
	PETEStyleEntry pse;
	Str63 word;
	
	if (wordPtr) *wordPtr = 0;
	if (checked) *checked = false;
	if (writeable) *writeable = false;
	if (misspelled) *misspelled = false;
	
	// anyone home?
	if (!PeteIsValid(pte) || !HaveSpeller()) return;
	
	// this is easy...
	if (writeable) *writeable = !PETESelectionLocked(PETE,pte,-1,-1);
	if (checked) *checked = SpellChecked(pte);
	
	// don't bother unless the user has checked spelling
	if (SpellDisabled(pte)) return;
	
	// Ok, the user has done his part.  Our turn.
	PeteGetTextAndSelection(pte,&text,&selStart,&selEnd);
	
	// too big?
	if (selEnd-selStart>63) return;
	
	// user-selected word?
	if (selEnd>selStart)
	{
		MakePStr(word,*text+selStart,selEnd-selStart);
		for (spot=word+*word;spot>word;spot--)
		{
			if (!SpellWordChar(*spot))
			{
				// user has crap selected.  Make it go away.
				return;
			}
		}
		
		// ok, the user has a valid word selected.  Do we think it misspelled?
		PeteStyleAt(pte,(selStart+selEnd)/2,&pse);
		if (misspelled) *misspelled = (pse.psStyle.textStyle.tsLabel&pSpellLabel) != 0;
		if (wordPtr) PCopy(wordPtr,word);
	}
	else
	{
		// is insertion point in misspelled word?
		PeteStyleAt(pte,-1,&pse);
		if ((pse.psStyle.textStyle.tsLabel&pSpellLabel) != 0)
		{
			if (misspelled) *misspelled = true;
			PETEFindLabelRun(PETE,pte,selStart-1,&selStart,&selEnd,pSpellLabel,pSpellLabel);
			if (wordPtr)
			{
				MakePStr(word,*text+selStart,selEnd-selStart);
				PCopy(wordPtr,word);
			}
		}
	}
	if (start) *start = selStart;
	if (stop) *stop = selEnd;
}				

/************************************************************************
 * SpellSuggestions - give the user some suggestions
 ************************************************************************/
void SpellSuggestions(PStr word,UHandle *suggestions)
{
	Str63 local;
	Accumulator a;
	Ptr crappySuggestPtr;
	UPtr spot;
	short scores[20];
	long topScore;
	short *score;

	if (!HaveSpeller()) return;
	
	SpellOpen();
	if (SpellInUse)
	{
		crappySuggestPtr = NuPtr(4 K);
		AccuInit(&a);
		*suggestions = nil;
		
		if (WinterTreeOptions&(1L<<20))
		{
			WinterTreeOptions |= SSCE_SUGGEST_PHONETIC_OPT;
			WinterTreeOptions &= ~SSCE_SUGGEST_TYPOGRAPHICAL_OPT;
		}
		
		SSCE_SetOption(SpellSession,SSCE_SUGGEST_PHONETIC_OPT,0!=(WinterTreeOptions&SSCE_SUGGEST_PHONETIC_OPT));
		SSCE_SetOption(SpellSession,SSCE_SUGGEST_TYPOGRAPHICAL_OPT,0==(WinterTreeOptions&SSCE_SUGGEST_TYPOGRAPHICAL_OPT));
		
		if (crappySuggestPtr)
		{
			PCopy(local,word);
			P2CStr(local);
			if (!SSCE_Suggest(SpellSession,
												local,
												GetRLong(SPELL_SUGGEST_DEPTH),
												crappySuggestPtr, 4 K,
												scores,20))
			{
				topScore = scores[0]*16;
				for (score=scores,spot=crappySuggestPtr;*spot;score++,spot+=*local+1)
				{
					if ((*score)*20<topScore) break;
					CtoPCpy(local,spot);
					AccuAddPtr(&a,local,*local+1);
				}
				AccuTrim(&a);
				*suggestions = a.data;
				a.data = nil;
			}
		}
		
		if (crappySuggestPtr) ZapPtr(crappySuggestPtr);
		AccuZap(a);
		SpellClose(false);
	}
	SpellTicks = TickCount();
}

/************************************************************************
 * SpellAnyWrongHuh - figure out if anything is misspelled
 ************************************************************************/
Boolean SpellAnyWrongHuh(PETEHandle pte)
{
	long oldSpelled;
	Boolean result = false;
	long selStart, selEnd;
	
	if (0<=SpellOpen())
	{
		if ((*PeteExtra(pte))->spelled!=sprSpellComplete)
		{
			oldSpelled = (*PeteExtra(pte))->spelled;
			if ((*PeteExtra(pte))->spelled<0) (*PeteExtra(pte))->spelled = 0;
			while (!CommandPeriod && (*PeteExtra(pte))->spelled!=sprSpellComplete)
			{
				CycleBalls();
				PeteSpellScan(pte,true);
			}
		}
		
		if (!CommandPeriod)
			result = !PETEFindLabelRun(PETE,pte,0,&selStart,&selEnd,pSpellLabel,pSpellLabel);
		SpellClose(false);
	}
	
	return(result);
}

/************************************************************************
 * AppendSpellItems - add spell items to context menu
 ************************************************************************/
OSErr AppendSpellItems(PETEHandle pte,MenuHandle contextMenu)
{
	MenuHandle mh;
	short i;
	short n;
	Boolean bar1, bar2;
	
	if (!HaveSpeller()) return noErr;
	
	if (SpellDisabled(pte)) return noErr; // don't bother unless the user has checked spelling

	EnableSpellMenu(false);
	
	if (mh = GetMHandle(SPELL_HIER_MENU))
	{
		n = CountMenuItems(mh);
		bar2 = n>spellItemLimit;
		bar1 = false;
		for (i=spellBar1Item;i<spellItemLimit;i++)
			if (IsEnabled(GetMenuID(mh),i)) {bar1 = true; break;}

		for (i=spellCheckItem+1;i<=n;i++)
		{
			if (i==spellBar1Item&&bar1 || i==spellItemLimit&&bar2)
				AppendMenu(contextMenu,"\p-");	// add a divider
			else if (IsEnabled(GetMenuID(mh),i))
			{
				CopyMenuItem(mh,i,contextMenu,REAL_BIG);
				SetMenuItemCommandID(contextMenu,CountMenuItems(contextMenu),(GetMenuID(mh)<<16)|i);
			}
		}
	}
	return(noErr);
}

#endif

/************************************************************************
 * HaveSpeller - is the speller installed?
 ************************************************************************/
Boolean HaveSpeller(void)
{
#ifdef WINTERTREE
	return(HasFeature (featureSpellChecking) && (void*)SSCE_OpenSession!=(void*)kUnresolvedCFragSymbolAddress);
#else
	return(false);
#endif
}
