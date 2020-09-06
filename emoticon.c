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

#include "emoticon.h"

typedef struct EmoRec EmoRec, *EmoRecPtr;

struct EmoRec
{
	Str31 ascii;
	Str31 meaning;
	FSSpec spec;
	Handle iconSuite;
	Boolean notInMenu;
	EmoRecPtr next;
};

Boolean EmoInitted;
EmoRecPtr EmoRoot;

#define EmoScanFinished(pte) (!(*PeteExtra(pte))->emoDesired || (*PeteExtra(pte))->emoScanned==kEmoScanFinished)

void EmoScanPete(PETEHandle pte, Boolean toCompletion);
void EmoScanPeteInner(PETEHandle pte, long *pScanned);
UPtr EmoScanPtr(UPtr start,long offset,UPtr end,EmoRecPtr *emoOut);
OSErr EmoApply(PETEHandle pte,long spot,EmoRecPtr emo);
OSErr EmoApplyIcon(PETEHandle pte,long start,EmoRecPtr emo);
void EmoInitMenu(void);
OSErr EmoSniffer(short vRefNum, long parID, uLong refCon);
Boolean IsEmoFile(FSSpecPtr spec,Handle *iconSuite);

/**********************************************************************
 * EmoInit - initialize the emoticon system
 **********************************************************************/
void EmoInit(void)
{
	short size = GetPrefLong(PREF_FONT_SIZE);
	long minSize;
	Str63 itemStr;
	UPtr spot;
	Str255 alternateFolders;
	
	FolderSniffer(EMOTICON_FOLDER,EmoSniffer,0);
	
	GetRString(alternateFolders,ALTERNATE_EMOTICONS);
	spot = alternateFolders+1;
	for(;;)
	{
		// get size
		if (!PToken(alternateFolders,itemStr,&spot,",") || !*itemStr) break;
		StringToNum(itemStr,&minSize);
		
		// string is ordered.  If our font size is smaller than the min
		// font size for this folder, we're done
		if (minSize > size) break;

		// foldername
		if (!PToken(alternateFolders,itemStr,&spot,",") || !*itemStr) break;
		
		// Go forth and sniff!
		FolderSnifferStr(itemStr,EmoSniffer,0);
	}
	
	if (EmoRoot) EmoInitMenu();
	
	EmoInitted = true;
}

/**********************************************************************
 * EmoSniffer - sniff a folder for emoticons
 **********************************************************************/
OSErr EmoSniffer(short vRefNum, long parID, uLong refCon)
{
	CInfoPBRec hfi;
	FSSpec spec;
	OSErr err = noErr;
	EmoRecPtr emo, lastEmo;
	Handle suite;
	
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = spec.name;
	
	while (!err && !DirIterate(vRefNum,parID,&hfi))
	{
		// files only.  So solly, no recursion...
		if (!(hfi.hFileInfo.ioFlAttrib & ioDirMask))
		spec.vRefNum = vRefNum;
		spec.parID = parID;
		IsAlias(&spec,&spec);
		if (IsEmoFile(&spec,&suite) && (emo=NewPtr(sizeof(EmoRec))))
		{
			long len;
			Str31 s;
			Str63 name;
			Str63 nameAndMeaning;
			UPtr spot;

			WriteZero(emo,sizeof(EmoRec));
			PSCopy(name,spec.name);
			if (name[1]=='-')
			{
				emo->notInMenu = true;
				BMD(name+2,name+1,--*name);
			}
			SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(name,nameAndMeaning,s,sizeof(nameAndMeaning)-1);
			spot = nameAndMeaning+1;
			PToken(nameAndMeaning,emo->ascii,&spot," ");
			PToken(nameAndMeaning,emo->meaning,&spot,"\015");
			len = *emo->ascii;
			TrLo(emo->ascii+1,len,"!","�");
			CaptureHexPtr(emo->ascii+1,emo->ascii+1,&len);
			*emo->ascii = len;
			emo->iconSuite = suite;
			if (!emo->iconSuite || FileTypeOf(&spec)!='rsrc')
				emo->spec = spec;
			emo->next = EmoRoot;
			EmoRoot = emo;
			
			// handle duplicate ascii values...
			lastEmo = EmoRoot;
			for (emo=EmoRoot->next;emo;emo=emo->next)
			{
				if (*EmoRoot->ascii==*emo->ascii && !memcmp(EmoRoot->ascii+1,emo->ascii+1,*emo->ascii))
				{
					lastEmo->next = emo->next;
					DisposePtr(emo);
					break;
				}
				else
					lastEmo = emo;
			}
		}
	}
	return noErr;
}

/**********************************************************************
 * IsEmoFile - is this an emoticon file?  If so, return any custom icon suite it might have
 **********************************************************************/
Boolean IsEmoFile(FSSpecPtr spec,Handle *iconSuite)
{
	*iconSuite = FSpGetCustomIconSuite(spec,svAllSmallData);
	if (FileTypeOf(spec)=='rsrc') return true;
	else return IsGraphicFile(spec);
}

/**********************************************************************
 * EmoInit - initialize the menu of emoticons
 **********************************************************************/
void EmoInitMenu(void)
{
	short count=0;
	Str63 menuString;
	MenuHandle mh = NewMenu(EMOTICON_HIER_MENU,GetRString(menuString,EMOTICON_FOLDER));
	EmoRecPtr emo;
	
	for (emo=EmoRoot;emo;emo=emo->next)
		if (!emo->notInMenu)
		{
			if (*emo->meaning)
				ComposeRString(menuString,EMO_MENU_FMT,emo->ascii,emo->meaning);
			else
				PCopy(menuString,emo->ascii);
			MyAppendMenu(mh,menuString);
			count++;
			if (emo->iconSuite) SetMenuItemIconHandle(mh,count,kMenuIconSuiteType,emo->iconSuite);
		}
	
	InsertMenu(mh,-1);
	AttachHierMenu(EDIT_MENU,EDIT_INSERT_EMOTICON_ITEM,EMOTICON_HIER_MENU);
}
	

/**********************************************************************
 * EmoScan - the emoticon scanner
 **********************************************************************/
void EmoScan(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	PETEHandle pte;

	if (!EmoInitted) EmoInit();
	if (EmoRoot)
		for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		{
			if (IsKnownWindowMyWindow(winWP))
			{
				win = GetWindowMyWindowPtr (winWP);
				for (pte=win->pteList;pte;pte=PeteNext(pte))
				{
					PeteExtra extra = **PeteExtra(pte);
					while (!EmoScanFinished(pte))
					{
						UsingWindow(winWP);
						EmoScanPete(pte,false);
						if (NEED_YIELD) return;
					}
				}
			}
		}
}

/**********************************************************************
 * EmoScanPete - the emoticon scanner, for a single Pete record
 **********************************************************************/
void EmoScanPete(PETEHandle pte, Boolean toCompletion)
{
	long scanned = (*PeteExtra(pte))->emoScanned;
	long len;
	short batch = GetRLong(EMO_BATCH_SIZE);
	
	if (!EmoInitted) EmoInit();
	if (!EmoRoot)
	{
		(*PeteExtra(pte))->emoScanned = kEmoScanFinished;
		return;
	}
	
	// Are we done?
	if (!EmoScanFinished(pte))
	{
		len = PETEGetTextLen(PETE,pte);
		
		PeteCalcOff(pte);
		
		do
		{
			EmoScanPeteInner(pte,&scanned);
		} // Keep scanning if we want to finish
		while ((toCompletion||(batch-->0)) && scanned<len-1);
		
		PeteCalcOn(pte);
		
		// Update the scanned pointers
		(*PeteExtra(pte))->emoScanned = scanned<len ? scanned:kEmoScanFinished;
	}
}

/**********************************************************************
 * EmoScanPeteInner - do a single scan, assuming we're all setup
 **********************************************************************/
void EmoScanPeteInner(PETEHandle pte, long *pScanned)
{
	UPtr spot, end, eol;
	long len;
	EmoRecPtr emo;
	UHandle text;
	
	// grab the text and setup pointers to beginning and end
	len = PETEGetTextLen(PETE,pte);
	PeteGetTextAndSelection(pte,&text,nil,nil);
	spot = *text + *pScanned;
	end = *text+len;
	
	// reset the end pointer to the next return, if any; we won't
	// scan more than one line at a time
	for (eol=spot;eol<end && *eol!='\015';eol++);
	end = eol;
	
	// now, we do the actual scanning
	if (spot = EmoScanPtr(*text,spot-*text,end,&emo))
	{
		// found one.  point the scanner past it
		*pScanned = spot - *text + *emo->ascii;
		// then apply the style
		EmoApply(pte,spot-*text,emo);
	}
	else
		// set scanner to eol, cause we didn't find anything
		*pScanned = end - *text + 1;
}

/**********************************************************************
 * EmoScanPtr - Scan some text for emoticons
 **********************************************************************/
UPtr EmoScanPtr(UPtr start,long offset,UPtr end,EmoRecPtr *emoOut)
{
	EmoRecPtr emo;
	EmoRecPtr maybeEmo = nil;
	UPtr spot = start+offset;
	
	for (;spot<end;spot++)
	{
		long len = end-spot;
		
		// We don't want a word character just before the text
		if (spot==start || !IsWordOrDigit(spot[-1]))
			for (emo=EmoRoot; emo; emo = emo->next)
				// don't run off the end
				if (*emo->ascii <= len)
				// is it the right text?
				if (!memcmp(spot,emo->ascii+1,*emo->ascii))
				// or just after the text
				if (!(spot+*emo->ascii<end && IsWordOrDigit(spot[*emo->ascii])))
				// Ok, we found an emoticon.  If we don't have one already, or if
				// the new one is longer than the old one, use the new one
				if (!maybeEmo || *maybeEmo->ascii < *emo->ascii)
					maybeEmo = emo;
		if (maybeEmo) break;
	}
	
	if (maybeEmo)
	{
		*emoOut = maybeEmo;
		return spot;
	}
	else
		return nil;
}

/**********************************************************************
 * EmoApply - apply an emoticon
 **********************************************************************/
OSErr EmoApply(PETEHandle pte,long start,EmoRecPtr emo)
{
	OSErr err;
	uLong selStart,selStop;
	uLong stop = start+*emo->ascii;
	
	PeteGetTextAndSelection(pte,nil,&selStart,&selStop);
	
	NoScannerResets++;
	if (!(*emo->spec.name)) err = EmoApplyIcon(pte,start,emo);
	else err = PeteFileGraphicRange(pte,start,stop,&emo->spec,fgEmoticon|fgDisplayInline|fgDontCopyToClip);
	NoScannerResets--;
	
	if (!err)
	{
		if (selStart >= start && selStart <= stop || selStop >= start && selStop <= stop)
			PeteSelect(nil,pte,stop,stop);
		UseFeature(featureEmo);
	}
	return err;
}

OSErr EmoApplyIcon(PETEHandle pte,long start,EmoRecPtr emo)
{
	FGIHandle graphic = NewZH(FileGraphicInfo);
	uLong stop = start + *emo->ascii;
	
	if (graphic)
	{
		Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
		Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
		
		(*graphic)->u.icon.suite = emo->iconSuite;
		(*graphic)->u.icon.sharedSuite = true;
		(*graphic)->width = (*graphic)->height = 16;
		(*graphic)->noImage = true;
		(*graphic)->pgi.privateType = pgtIcon;
		(*graphic)->pgi.width = 16;
		(*graphic)->pgi.height = 16;
		(*graphic)->pgi.cloneOnlyText = 1;
		(*graphic)->pgi.itemProc = FileGraphic;
		(*graphic)->isEmoticon = true;
				
#ifdef PETESetStyleWorkedRight
		{
			PeteStyleInfo psi;
			Zero(psi);
			psi.graphicStyle.graphicInfo = graphic;
			PETESetStyle(PETE,pte,start,stop,&psi,peGraphicValid|peGraphicColorChangeValid);
		}
#else
		{
			PETEStyleEntry pse;
			long junk;
			OSErr err=noErr;
			Handle text;
			Str255 old;
			PETEStyleListHandle pslh;
			long selStart, selEnd;
			
			Zero(pse);
			PETEGetStyle(PETE,pte,start,&junk,&pse);
			if (!pse.psGraphic)
			{
				pse.psGraphic = true;
				pse.psStyle.graphicStyle.graphicInfo = graphic;
				pse.psStartChar = 0;
				//pse.psStyle.textStyle.tsSize = -1;
				PeteGetTextAndSelection(pte,&text,&selStart,&selEnd);
				MakePStr(old,*text+start,stop-start);
				PeteDelete(pte,start,stop);
				if (pslh = NewZH(PETEStyleEntry))
				{
					**pslh = pse;
					err = PETEInsertTextPtr(PETE,pte,start,old+1,*old,pslh);
					ZapHandle(pslh);
					PeteSelect(nil,pte,selStart,selEnd);
				}
			}
		}
#endif
		if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
	}
	return noErr;
}

/**********************************************************************
 * EmoInsert - insert an emoticon at the current insertion point
 **********************************************************************/
OSErr EmoInsert(MyWindowPtr win,short item)
{
	MenuHandle mh = GetMHandle(EMOTICON_HIER_MENU);
	Str31 ascii;
	OSErr err = paramErr;
	UPtr breakMe;
	long selStart,selEnd;
	
	MyGetItem(mh,item,ascii);
	if (breakMe=PIndex(ascii,optSpace)) *ascii = breakMe-ascii-1;
	
	if (win->pte)
	{
		PETESetRecalcState(PETE,win->pte,false);
		PeteGetTextAndSelection(win->pte,nil,&selStart,&selEnd);
		if (selStart && !IsLWSP(PeteCharAt(win->pte,selStart-1)))
			PInsertC(ascii,sizeof(ascii),' ',ascii+1);
		if (selEnd<PeteLen(win->pte) && !IsLWSP(PeteCharAt(win->pte,selEnd)))
			PCatC(ascii,' ');
		err = PeteInsertStr(win->pte,-1,ascii);
		if (EmoDo())
		{
			EmoScanPete(win->pte,false);
			EmoScanPete(win->pte,false);
			EmoScanPete(win->pte,false);
		}
		PETESetRecalcState(PETE,win->pte,true);
	}
	return err;
}
	
/**********************************************************************
 * EmoSearchAndDestroy - destroy all emoticons
 **********************************************************************/
void EmoSearchAndDestroy(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	PETEHandle pte;

	if (EmoRoot)
		for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		{
			if (IsKnownWindowMyWindow(winWP))
			{
				win = GetWindowMyWindowPtr (winWP);
				for (pte=win->pteList;pte;pte=PeteNext(pte))
					EmoSearchAndDestroyPete(pte);
			}
		}
}

/**********************************************************************
 * EmoSearchAndDestroyPte - destroy all emoticons in a single edit region
 **********************************************************************/
void EmoSearchAndDestroyPete(PETEHandle pte)
{
	PETEStyleEntry pse;
	long offset;
	long len = PeteLen(pte);
	long runLen;
	long peteDirtyWas = PeteIsDirty(pte);
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	
	NoScannerResets++;
	
	for (offset=0; offset<len; offset+=runLen)
	{
		Zero(pse);
		if (PETEGetStyle(PETE,pte,offset,&runLen,&pse)) break;
		if (pse.psGraphic && IsEmoticonStyle(&pse.psStyle.graphicStyle))
			PeteClearGraphic(pte,offset,offset+runLen);
		
		// let's not get stuck, ok?
		if (!runLen) runLen = 1;
	}
	
	// reset this scanner just in case the user turns them back on
	(*PeteExtra(pte))->emoScanned = 0;
	
	if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
	(*PeteExtra(pte))->win->isDirty = winDirtyWas;
	
	NoScannerResets--;
}
				
