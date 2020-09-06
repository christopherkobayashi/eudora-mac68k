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

#include "text.h"
#include "pete.h"
#define FILE_NUM 39
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Text

	Boolean TextClose(MyWindowPtr win);
	Boolean TextSave(MyWindowPtr win);
	Boolean TextSaveAs(MyWindowPtr win);
	Boolean TextMenu(MyWindowPtr win, int menu, int item, short modifiers);
	Boolean TextPosition(Boolean save,MyWindowPtr win);
	void TextChanged(MyWindowPtr win, TEHandle teh, short oldNl, short newNl, Boolean scroll);
OSErr GetStyl(Handle *styl,PETEParaScrapHandle *paraScrap,FSSpecPtr spec);
Boolean TextNickFieldCheck (PETEHandle pte);
Boolean HTMLIshSpec(FSSpecPtr spec);
Boolean HTMLIshText(Handle text);
#ifdef DEBUG
Boolean ClockIshSpec(FSSpecPtr spec);
#endif

/************************************************************************
 * OpenText - open a text window
 ************************************************************************/
MyWindowPtr OpenText(FSSpecPtr spec,WindowPtr winWP,MyWindowPtr win,WindowPtr behind,Boolean showIt,UPtr alias,Boolean ro,Boolean insertAsHTML)
{
	long bytes;
	UHandle textH=nil;
	short refN=0;
	short err=noErr;
	TextDHandle th=nil;
	MyWindowPtr oldWin;
#ifdef PEE
	PETEDocInitInfo pi;
	Handle styl=nil, pstyle=nil;
	PETEParaScrapHandle paraScrap=nil;
#endif
	
	/*
	 * if we're opening an existing file, have a look for it as an open window
	 */
	if (spec)
	{
		if (oldWin=FindText(spec))
		{
			WindowPtr	oldWinWP = GetMyWindowWindowPtr (oldWin);
			PeteCalcOn(oldWin->pte);
			ShowMyWindow(oldWinWP);
			UserSelectWindow(oldWinWP);
			return(oldWin);
		}
		
		if (MemoryPreflight(FSpDFSize(spec))) return(nil);
		
		/*
		 * not open window.  Try to open the file
		 */
		if (err=AFSpOpenDF(spec,spec,fsRdPerm,&refN))
			FileSystemError(TEXT_READ,spec->name,err);
		else if (err=GetEOF(refN,&bytes))
			FileSystemError(TEXT_READ,spec->name,err);
	}
	else
		bytes = 0;	/* don't need space for text in new file */

	/*
	 * ok, we've found the file.  Now, open the window
	 */
	if (err==noErr)
	{
		if (!(win = GetNewMyWindow(MESSAGE_WIND,winWP,win,behind,False,False,TEXT_WIN)))
			err=1;
		else if (!(th=NewZH(TextDesc)))
			WarnUser(MEM_ERR,err=MemError());
		else
		{
			winWP = GetMyWindowWindowPtr (win);		
			/*
			 * allocate space for and read text
			 */
			if (!(textH=NuHTempOK(bytes)))
				WarnUser(MEM_ERR,err=MemError());
			else if (spec && (err=ARead(refN,&bytes,LDRef(textH))))
				FileSystemError(TEXT_READ,spec->name,err);
			else
			{
				/*
				 * Whoopee!  Finish it off...
				 */
				UL(textH);
				if (refN) MyFSClose(refN);
				refN = 0;
				
				if (!insertAsHTML) {
					if (spec) GetStyl(&styl,&paraScrap,spec);
					if (styl)
					{
						PETEConvertTEScrap(PETE,(void*)styl,(void*)&pstyle);
						if (ro) PeteLockStyle((PETEStyleListHandle) pstyle);
					}
				}
				DefaultPII(win,ro,peVScroll|peGrowBox,&pi);
	
				pi.inParaInfo.paraFlags = peTextOnly;
				
				if (!(err=PeteCreate(win,&win->pte,0,&pi)))
				{
					{
						PeteCalcOff(win->pte);
						if (insertAsHTML) {
							long	tStart	= 0;
							long	tOffset = -1;
							char	base[256];
							
							//	Need to insert base ref. Use directory containing html source
							ComposeRString(base,MHTML_INFO_TAG,"","",0,"");
							Munger(textH,0,nil,0,base+1,*base);
							ComposeString(base,"\p<%r>",htmlXHTMLDir+HTMLDirectiveStrn);
							Munger(textH,0,nil,0,base+1,*base);
							ComposeString(base,"\p</%r>",htmlXHTMLDir+HTMLDirectiveStrn);
							Munger(textH,GetHandleSize(textH),nil,0,base+1,*base);
							err = InsertHTML (textH, &tStart, GetHandleSize (textH), &tOffset, win->pte, 0);
							if (!err) PeteSmallParas(win->pte);
							PeteCalcOn(win->pte);
						}
						else {
							err = PETEInsertTextScrap(PETE,win->pte,0,textH,(void*)pstyle,paraScrap,true);
							if (!err) PeteSmallParas(win->pte);
							PeteCalcOn(win->pte);
							if (err==badSectionErr) err = noErr;
							pstyle = nil;	// pete always disposes
							paraScrap = nil;
							textH = nil;	
						}
					}
					if (!err)
					{
						if(ro) PeteLock(win->pte, 0, LONG_MAX, peModLock);
						else SetNickScanning (win->pte, nickComplete | nickExpand, kNickScanAllAliasFiles, TextNickFieldCheck, nil, nil);

						SetWTitle_(winWP,alias?alias:spec->name);
						if (spec) (*th)->spec = *spec;
						(*th)->textAttrbutes = 0;
						if (ro)
							(*th)->textAttrbutes |= kTextAttributeReadOnly;
						if (insertAsHTML)
							(*th)->textAttrbutes |= kTextAttributeIsHTML;
						SetMyWindowPrivateData(win,(long)th); th=nil;
						win->position = (alias ? PositionPrefsTitle : TextPosition);
						win->didResize = TextDidResize;
						win->zoomSize = CompZoomSize;
						win->dontControl = True;
						win->menu = TextMenu;
						win->close = TextClose;
						win->find = TextFind;
						win->userSave = !ro;
						PETEMarkDocDirty(PETE,win->pte,False);
						win->isDirty = False;
   					PETESetCallback(PETE,win->pte,(void*)PeteChanged,peDocChanged);
						PeteSelect(win,win->pte,0,0);
						if (showIt)
						{
#ifdef WINTERTREE
							if (HasFeature (featureSpellChecking) && !ro)
								(*PeteExtra(win->pte))->spelled = 0;	// allow spelling here;
#endif
							ShowMyWindow(winWP);
							UpdateMyWindow(winWP);
						}
					}
				}
				CleanPII(&pi);
				if (err) {CloseMyWindow(winWP);WarnUser(PETE_ERR,err);}
			}
		}
	}
	if (refN) MyFSClose(refN);
	ZapHandle(textH);
	ZapHandle(th);
	ZapHandle(styl);
	if (pstyle) ZapPeteStyleScrap(pstyle);
	if (paraScrap) ZapPeteParaScrap(paraScrap);
	return(err?nil:win);
}

Boolean TextNickFieldCheck (PETEHandle pte)

{
	return (false);
}

#pragma segment Text

/************************************************************************
 * HTMLIshSpec - does a file look like HTML?
 ************************************************************************/
Boolean HTMLIshSpec(FSSpecPtr spec)
{
	Str255 suffices;
	UPtr spot;
	Str31 suffix;
	
	spot = suffices+1;
	GetRString(suffices,HTMLISH_SUFFICES);
	while (PToken(suffices,suffix,&spot,"�") && *suffix)
		if (EndsWith(spec->name,suffix)) return(true);
	return(false);
}

/************************************************************************
 * HTMLIshText - does some text look like HTML?
 ************************************************************************/
Boolean HTMLIshText(Handle text)
{
	return(false);
}

#ifdef DEBUG
/************************************************************************
 * ClockIshSpec - does the FSSpec make me think of a clock?
 ************************************************************************/
Boolean ClockIshSpec(FSSpecPtr spec)
{
	return(StringSame(spec->name,"\pLizzie Clock"));
}
#endif

/**********************************************************************
 * GetStyl - get a style resource from a file
 **********************************************************************/
OSErr GetStyl(Handle *styl,PETEParaScrapHandle *paraScrap,FSSpecPtr spec)
{
	short refN;
	OSErr err=noErr;
	short oldResF = CurResFile();
	
	if (0<=(refN=FSpOpenResFile(spec,fsRdPerm)))
	{
		if (*styl = Get1IndResource('styl',1))
			DetachResource(*styl);
		else
			err = resNotFound;
		if (*paraScrap = (void*)Get1IndResource(kPETEParaScrap,1))
			DetachResource((Handle)*paraScrap);
		CloseResFile(refN);
	}
	else err = ResError();
	UseResFile (oldResF);
	return(err);
}

/************************************************************************
 * FindTextLo - find a window containing the contents of a text file
 ************************************************************************/
MyWindowPtr FindTextLo(FSSpecPtr spec,PStr name)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	FSSpec winSpec, newSpec;
	Str255 title;
	
	if (spec) IsAlias(spec,&newSpec);
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsKnownWindowMyWindow(winWP) && GetWindowKind(winWP)==TEXT_WIN)
		{
			win = GetWindowMyWindowPtr (winWP);
			if (spec)
			{
				if (GetMyWindowPrivateData(win))
				{
					winSpec = (*(TextDHandle) GetMyWindowPrivateData(win))->spec;
					if (winSpec.vRefNum==newSpec.vRefNum && winSpec.parID==newSpec.parID
							&& StringSame(winSpec.name,newSpec.name))
						return(win);
				}
			}
			else if (name)
			{
				GetWTitle(winWP,title);
				if (StringSame(title,name))
					return win;
			}
		}
	return(nil);
}

/**********************************************************************
 * TextDidResize - handle the resizing of a message window
 **********************************************************************/
void TextDidResize(MyWindowPtr win,Rect *oldContR)
{
	PeteDidResize(win->pte,&win->contR);
}

#pragma segment Main
/************************************************************************
 * TextClose - close a text window
 ************************************************************************/
Boolean TextClose(MyWindowPtr win)
{
	Handle	aHandle;
	short which;
	FSSpec spec = (*(TextDHandle)GetMyWindowPrivateData(win))->spec;
	
	if (!GrowZoning
#ifdef DEBUG
			 && (RunType==Production || !EqualStrRes(spec.name,LOG_NAME))
#endif
			)
	{
		win->isDirty = win->isDirty || PeteIsDirtyList(win->pte);
		if (win->isDirty)
		{
			which = WannaSave(win);
			if (which==CANCEL_ITEM) return(False);
			if (which==WANNA_SAVE_SAVE && !TextSave(win)) return(False);
		}
		aHandle = (Handle) GetMyWindowPrivateData(win);
		ZapHandle(aHandle);
		SetMyWindowPrivateData(win, 0);
	}
	return(True);
}

#pragma segment Text
	
/**********************************************************************
 * TextFind - find in the window
 **********************************************************************/
Boolean TextFind(MyWindowPtr win,PStr what)
{
	return FindInPTE(win,win->pte,what);
}

/************************************************************************
 * TextSave - save the contents of a text window
 ************************************************************************/
Boolean TextSave(MyWindowPtr win)
{
	OSErr err;
	FSSpec spec = (*(TextDHandle)GetMyWindowPrivateData(win))->spec;
	Handle text;
	
	if (!spec.vRefNum) return(TextSaveAs(win));
	
	if (err=PeteGetRawText(win->pte,&text)) WarnUser(PETE_ERR,err);
	else
	{
		err = Blat(&spec,text,False);
		PeteSaveTEStyles(win->pte,&spec);
	}
	if (!err)
	{
		PeteCleanList(win->pte);
		win->isDirty = False;
	}
	return(err==noErr);
}

/************************************************************************
 * TextSaveAs - save the contents of a text window, giving a new name
 ************************************************************************/
Boolean TextSaveAs(MyWindowPtr win)
{
	short refN,err;
	Str31 scratch;
	long creator,bytes;
	FSSpec spec = (*(TextDHandle)GetMyWindowPrivateData(win))->spec;
	Handle text;

	/*
	 * standard file stuff
	 */
	GetPref(scratch,PREF_CREATOR);
	if (*scratch!=4) GetRString(scratch,TEXT_CREATOR);
	BMD(scratch+1,&creator,4);
	if (err=SFPutOpen(&spec,creator,'TEXT',&refN,nil,nil,0,nil,nil,nil))
		return(False);
	
	if (err = PeteGetRawText(win->pte,&text)) WarnUser(PETE_ERR,err);
	else
	{
		bytes = GetHandleSize(text);
		if (err=AWrite(refN,&bytes,LDRef(text)))
			FileSystemError(TEXT_WRITE,spec.name,err);
		else if (err=SetEOF(refN,bytes))
			FileSystemError(TEXT_WRITE,spec.name,err);
		else
		{
			(*(TextDHandle)GetMyWindowPrivateData(win))->spec = spec;
			SetWTitle_(GetMyWindowWindowPtr(win),spec.name);
			win->isDirty = False;
		}
		UL(text);
	}
	
	PeteSaveTEStyles(win->pte,&spec);

	MyFSClose(refN);

	if (!err)
	{
		PeteCleanList(win->pte);
		win->isDirty = False;
	}

	return(err==noErr);
}

/************************************************************************
 * TextMenu - handle menu choices peculiar to Text windows
 ************************************************************************/
Boolean TextMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(modifiers)
	switch (menu)
	{
		case FILE_MENU:
			switch (item)
			{
				case FILE_SAVE_ITEM:
					if (win->isDirty) (void) TextSave(win);
					break;
				case FILE_SAVE_AS_ITEM:
					TextSaveAs(win);
					break;
				case FILE_PRINT_ITEM:
				case FILE_PRINT_ONE_ITEM:
					PrintOneMessage(win,modifiers&shiftKey,item==FILE_PRINT_ONE_ITEM);
					break;
				default:
					return(False);
			}
			break;
			
		default:
			return(False);
	}
	return(True);
}

/************************************************************************
 * TextPosition - remember the position of a text window
 ************************************************************************/
Boolean TextPosition(Boolean save,MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	Rect r;
	Boolean zoomed;
	FSSpec spec = (*(TextDHandle) GetMyWindowPrivateData(win))->spec;
	
	if (!*spec.name) return(False);
	
	if (save)
	{
		utl_SaveWindowPos(winWP,&r,&zoomed);
		SavePosFork(spec.vRefNum,spec.parID,spec.name,&r,zoomed);
	}
	else
	{
		if (!RestorePosFork(spec.vRefNum,spec.parID,spec.name,&r,&zoomed))
			return(False);
		utl_RestoreWindowPos(winWP,&r,zoomed,1,TitleBarHeight(winWP),LeftRimWidth(winWP),(void*)FigureZoom,(void*)DefPosition);
	}
	return(True);
}

/**********************************************************************
 * PeteLockStyle - lock all the styles in a style handle
 **********************************************************************/
void PeteLockStyle(PETEStyleListHandle style)
{
	short n = HandleCount(style);
	
  while (n-->0) (*style)[n].psStyle.textStyle.tsLock = peModLock;
}
