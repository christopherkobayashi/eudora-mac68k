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

#include "print.h"
#define FILE_NUM 32
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for dealing with printing
 ************************************************************************/

#pragma segment Print

#define OSX_PRINT_RTYPE 'EupX'

pascal void PrintIdle(void);

PMPageFormat gPageSetup;			/* our Page Setup */
PMPrintSettings gPrintSettings;
Boolean	gDontBeginPage;	// True if we have already begun the page
static ResType PrintResType(void);
UInt32 gFirstPage, gLastPage, gCurrentPage;

/************************************************************************
 * private function declarations
 ************************************************************************/
int PrintMessage(PMPrintContext printContext,MyWindowPtr win,Boolean isOut, Rect *uRect, Boolean select, PETEHandle printMe);
int PrintMessagePage(PMPrintContext printContext,UPtr title, PETEHandle pte,Rect *uRect,short hMar,short pageNum,long *para,long *line);
void CollectReturnAddr(UPtr returnAddr);
pascal OSErr PetePrintBusy(void);
OSErr GetPrintSettings(PMPageFormat *pPageSetup,PMPrintSettings *pPrintSettings);

pascal void PrintIdle(void)
{
	CommandPeriod = HasCommandPeriod();
}

/**********************************************************************
 * PrintPreamble - get everything setup for printing
 **********************************************************************/
OSErr PrintPreamble(PMPrintContext *printContext, Rect *uRect,Boolean now)
{
	OSErr err=0;
	WindowPtr	docWin;
	DECLARE_UPP(PrintIdle,PMIdle);
	Boolean	validResults;
	
	//	Put document window in front of floating windows temporarily so the
	//	print manager gets the title from the right window
	if (docWin=MyFrontNonFloatingWindow()) BringToFront(docWin);
	
	PMBegin();
	if (err=PMError())
	{
		WarnUser(HaveOSX()?NO_OSX_PRINTER:NO_PRINTER,err);
		return(err);
	}
	
	if (GetPrintSettings(&gPageSetup,&gPrintSettings)) goto done;

	PushCursor(arrowCursor);
	//if (now) PrintDefault(PageSetup);
	
	if (!now) PMPrintDialog(gPrintSettings,gPageSetup,&validResults);
	if (now || validResults)
	{
		GrafPtr	printerPort;
		CFStringRef	jobName;
		
		//	Set job name
		CopyWindowTitleAsCFString (docWin,&jobName);
		PMSetJobNameCFString(gPrintSettings,jobName);
		CFRelease (jobName);

		// Setup to print page range
		PMGetFirstPage (gPrintSettings, &gFirstPage);
		PMGetLastPage (gPrintSettings, &gLastPage);
		gCurrentPage = 1;

		PopCursor();
		PMBeginDocument(gPrintSettings,gPageSetup,printContext);
		if (!*printContext) {WarnUser(NO_PPORT,err=MemError()); goto done;}

		//	We have to do BeginPage here so PMGetGrafPtr is in scope
		PMBeginPage(*printContext,nil);
		if (err=PMError()) goto done;
		gDontBeginPage = true;
		
		PMGetGrafPtr(*printContext,&printerPort);
		SetPort(printerPort);
		GetURect(uRect);
		
		INIT_UPP(PrintIdle,PMIdle);
		PMSetIdleProc(PrintIdleUPP);
		
		SetupPrintFont();
		TextFont(FontID);
		TextSize(FontSize);
	}
	else
	{
		err = userCancelled;
		PopCursor();
	}
done:	
	if (err) {
	//	Clean up lingering error from previous print jobs
	//	Bug #4157 - mtc 11/03/2004
		PMSetError ( noErr );
		PMEnd();
		}
	
	return(err);
}

/**********************************************************************
 * PrintCleanup - cleanup printing
 **********************************************************************/
OSErr PrintCleanup(PMPrintContext printContext,OSErr err)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	err = err ? err : PMError();
	
	if (printContext) PMEndDocument(printContext);

//	Clean up lingering error from previous print jobs
//	Bug #4157 - mtc 11/03/2004
	PMSetError ( noErr );
	PMEnd();

 	winWP = FrontWindow_();
 	win = GetWindowMyWindowPtr (winWP);
 	if (IsMyWindow(winWP) && !CorrectlyActivated(winWP)) {ActivateMyWindow(winWP,!win->isActive);}
	PMDisposePageFormat(gPageSetup);
	PMDisposePrintSettings(gPrintSettings);
	FigureOutFont(False);
	UseResFile(SettingsRefN);
	FloatingWinIdle();	// Make sure document window goes back behind floating windows
	return(err);
}

/************************************************************************
 * PrintSelectedMessages - print out a given message.  Handles the print
 * manager stuff.
 ************************************************************************/
int PrintSelectedMessages(TOCHandle tocH,Boolean select,Boolean now,long beginSel,long endSel,PETEHandle printMe)
{
	short err, cumErr=0;
	PMPrintContext printContext=nil;
	int sumNum;
	Boolean isOut;
	Str31 outName;
	Rect uRect;
	MyWindowPtr win;
	FSSpec	spec;
	long oldBeginSel, oldEndSel;
	PushGWorld();
		
	if (!(err=PrintPreamble(&printContext,&uRect,now)))
	{
		spec = GetMailboxSpec(tocH,-1);
		isOut = StringSame(GetRString(outName,OUT),spec.name);
		UL(tocH);
		for (sumNum=0;!CommandPeriod && sumNum<(*tocH)->count;sumNum++)
		{
			CycleBalls();
			if ((*tocH)->sums[sumNum].selected)
			{
#ifdef IMAP
				// if this is an IMAP message, make sure it's been downloaded.
				if ((*tocH)->imapTOC) EnsureMsgDownloaded(tocH, sumNum, false);
#endif
				win = GetAMessage(tocH,sumNum,nil,nil,False);
				err = !win;
				if (!err)
				{
					WindowPtr	winWP = GetMyWindowWindowPtr (win);
					GrafPtr		printPort;
					
					PMGetGrafPtr(printContext,&printPort);
					SetPort(printPort);
					PeteCalcOn(win->pte);
					
					// change selection (in case of print selection from preview pane)
					if (!printMe && select && beginSel!=endSel)
					{
						PeteGetTextAndSelection(win->pte,nil,&oldBeginSel,&oldEndSel);
						PeteSelect(win,win->pte,beginSel,endSel);
					}
					
					// print
					err = PrintMessage(printContext,win,isOut,&uRect,select,printMe);
					
					if (!IsWindowVisible(winWP))
						// close
						CloseMyWindow(winWP);
					else
						// restore selection
						if (select && beginSel!=endSel) PeteSelect(win,win->pte,oldBeginSel,oldEndSel);
					if (err) cumErr = err;
				}
				// for multiple previews, print only one copy
				if (printMe) break;
			}
		}
		err = PrintCleanup(printContext,err);
		if (cumErr && cumErr!=userCanceledErr 
			) WarnUser(PART_PRINT_FAIL,cumErr);
	}
	PopGWorld();
	return(err);
}

/************************************************************************
 * PrintOneMessage - print out a given message.  Handles the print manager
 * stuff.
 ************************************************************************/
int PrintOneMessage(MyWindowPtr win,Boolean select,Boolean now)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	short err;
	Rect uRect;
	Boolean isOut;
	PMPrintContext printContext;

	if (IsMyWindow (winWP))
		if (win->pte && HasNickScanCapability (win->pte))
			NicknameWatcherFocusChange (win->pte); /* MJN */

	PushGWorld();
		
	if (!(err=PrintPreamble(&printContext,&uRect,now)))
	{
		GrafPtr		printPort;
				
		isOut = GetWindowKind(winWP)==COMP_WIN;
		PMGetGrafPtr(printContext,&printPort);
		SetPort(printPort);
		err = PrintMessage(printContext,win,isOut,&uRect,select,nil);
		err = PrintCleanup(printContext,err);
	}
	
	PopGWorld();
	return(err);
}

/************************************************************************
 * PrintClosedMessage - print out a given message.  Handles the print manager
 * stuff, and opening the message
 ************************************************************************/
int PrintClosedMessage(TOCHandle tocH,short sumNum,Boolean now)
{
	Boolean opened = !(*tocH)->sums[sumNum].messH;
	MyWindowPtr win = GetAMessage(tocH,sumNum,nil,nil,False);
	OSErr err = !win;
	if (!err)
	{
		err = PrintOneMessage(win,False,now);
		if (opened) CloseMyWindow(GetMyWindowWindowPtr(win));
	}
	return(err);
}

/**********************************************************************
 * PeteBusy - Pete is busy
 **********************************************************************/
pascal OSErr PetePrintBusy(void)
{
	PrintYield();
	return(CommandPeriod ? userCanceledErr : noErr);
}

/************************************************************************
 * PrintMessage - print a given message, assuming everything is already
 * set up with the print manager.
 ************************************************************************/
int PrintMessage(PMPrintContext printContext,MyWindowPtr win,Boolean isOut, Rect *uRect,Boolean select,PETEHandle printMe)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	long offset = 0;
	short hMar = GetRLong(PRINT_H_MAR);
	short err;
	short pageNum=0;
	Str255 title;
	GrafPtr port;
//	short device = ((*PageSetup)->prStl.wDev>>8)&0xff;
	Boolean isMessage;
	long para, line;
	extern void *PeteBusyUPP;
	//long n = PeteParaAt(win->pte,PETEGetTextLen(PETE,win->pte)-1);
	long oldPara=0, oldLine=0;
	
	isMessage = IsMessWindow(winWP);
						
#define bDevLaser 3
	
	GetPort(&port);

	if (isMessage) PushPers(PERS_FORCE(MESS_TO_PERS(Win2MessH(win))));
		
	if (isMessage/* && PrefIsSet(PREF_LOCAL_DATE)*/)
		MakeMessTitle(title,(*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,False);
	else
		MyGetWTitle(winWP,title);

	line = para = 0;
	
	if (!printMe) printMe = win->pte;
	
	if (!(err=PeteCalcOn(printMe)))
	if (!(err=PETESetCallback(PETE,printMe,(void*)PetePrintBusy,peProgressLoop)))
	if (!(err=PETEPrintSetup(PETE,printMe)))
	if (!(err=PETEChangeDocWidth(PETE,printMe,RectWi(*uRect),True)))
	{
		if (select) PETEPrintSelectionSetup(PETE,printMe,&para,&line);
		
		if (PrefIsSet(PREF_PRINT_BLACK)) PeteSetupTextColors(printMe,true);

		do
		{
			PrintYield();
			oldPara = para; oldLine = line;
			err=PrintMessagePage(printContext,title,printMe,uRect,hMar,++pageNum,&para,&line);
			if (oldPara==para && oldLine>line)
			{
				err = 1;
#ifdef DEBUG
				if (RunType!=Production) Dprintf("\p %d.%d -> %d.%d;g",oldPara,oldLine,para,line);
#endif
			}
		}
		while (!err);
	}
	
	if (PrefIsSet(PREF_PRINT_BLACK)) PeteSetupTextColors(printMe,false);

	if (err==errEndOfDocument) err = noErr;
	
	PETESetCallback(PETE,printMe,(void*)PeteBusy,peProgressLoop);
	PETEPrintCleanup(PETE,printMe);

	if (!err && isMessage && SumOf(Win2MessH(win))->state == UNREAD)
		SetState((*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,READ);
	
	if (isMessage) PopPers();
	
	return(err);
}


/**********************************************************************
 * PrintYield - yield time while printing
 **********************************************************************/
OSErr PrintYield(void)
{
	if (NEED_YIELD)
	{
		PushGWorld();
		
		FigureOutFont(False);
		MiniEvents();
		PopGWorld();
		
		SetupPrintFont();
	}
	return noErr;
}

#define ISCR(c) ((c)=='\015' || (c)=='\014')
/************************************************************************
 * PrintMessagePage - print a pageful of the current message
 ************************************************************************/
int PrintMessagePage(PMPrintContext printContext,UPtr title, PETEHandle pte,Rect *uRect,short hMar,short pageNum,long *para,long *line)
{
	short baseV=uRect->top+hMar+FontLead;
	short lastBase = uRect->bottom-hMar-FontDescent;
	short width=uRect->right-uRect->left;
	OSErr err;
	Rect r = *uRect;
	GrafPtr		printPort;
	
	InsetRect(&r,0,hMar+5);
		
	if (gCurrentPage > gLastPage)
		// We're done
		return errEndOfDocument;

	if (gCurrentPage < gFirstPage)
		// Haven't reached first page yet
		err = PETEPrintPage(PETE,pte,nil,&r,para,line);
	else
	{
		if (!gDontBeginPage) PMBeginPage(printContext,nil);
		gDontBeginPage = false;
		if (!(err=PMError()))
		{
			PushGWorld();
			PMGetGrafPtr(printContext,&printPort);
			SetPort(printPort);
			if (hMar) PrintMessageHeader(title,pageNum,hMar,
																		uRect->top+hMar,uRect->left,uRect->right);

			PMGetGrafPtr(printContext,&printPort);
			err = PETEPrintPage(PETE,pte,(void*)printPort,&r,para,line);

			if (hMar) PrintBottomHeader(pageNum,uRect);

			PMEndPage(printContext);
			if (!err) err = PMError();
			PopGWorld();
		}
	}
	gCurrentPage++;
	return(err);
}


/**********************************************************************
 * PrintBottomHeader - print bottom header
 **********************************************************************/
void PrintBottomHeader(short pageNum,Rect *uRect)
{
	Str255 returnAddr;
	short hMar = GetRLong(PRINT_H_MAR);

	CollectReturnAddr(returnAddr);
	PrintMessageHeader(returnAddr,pageNum,-hMar,
																uRect->bottom,uRect->left,uRect->right);
}

/************************************************************************
 * PrintMessageHeader - print a header with the message info in it.
 ************************************************************************/
void PrintMessageHeader(UPtr title, short pageNum, short height, short bottom, short left, short right)
{
	//Rect border;
	short h,v;
	Str31 pageStr;
	short pageStrWidth;
	GrafPtr port;
	short oldFontId;
	short textV;
	
	if (height)
	{
#ifdef NEVER
		SetRect(&border,left,bottom-height,right,bottom);
		FillRect(&border,&qd.gray);
		InsetRect(&border,1,1);
		EraseRect(&border);
#endif
		if (height<0)
		{
			v = bottom + height;
			height *= -1;
			textV = v+GetRLong(PRINT_H_SIZE)+4;
		}
		else
		{
			v = bottom;
			textV = v-4;
		}
			
		MoveTo(left,v); LineTo(right,v);
		
		GetPort(&port);
		oldFontId = GetPortTextFont(port);
		
		v = bottom - height/4;
		TextFace(bold);
		TextSize(GetRLong(PRINT_H_SIZE));
		TextFont(GetFontID(GetRString(pageStr,PRINT_H_FONT)));
		NumToString(pageNum,pageStr);
		pageStrWidth = StringWidth(pageStr);
		h = right - pageStrWidth;
		MoveTo(h,textV);
		DrawString(pageStr);
		h = left;
		MoveTo(h,textV);
		DrawText(title,1,CalcTrunc(title,right-3*height-pageStrWidth-left,port));
	}
	
	TextFont(FontID);
	TextFace(0);
	TextSize(FontSize);
}

/************************************************************************
 * CollectReturnAddr - collect the return addr for the bottoms of printouts
 ************************************************************************/
void CollectReturnAddr(UPtr returnAddr)
{
	short len;
	char saveChar;
	
	GetRString(returnAddr,RETURN_PRINT_INTRO);
	len = *returnAddr;
	saveChar = returnAddr[len];
	GetReturnAddr(returnAddr+len,True);
	*returnAddr += returnAddr[len];
	returnAddr[len] = saveChar;
}

/************************************************************************
 * SetupPrintFont - set up the font to use for printing
 ************************************************************************/
void SetupPrintFont(void)
{
	Str31 scratch;
	short fID,fSize;
//	short device = ((*PageSetup)->prStl.wDev>>8)&0xff;
	
	GetPref(scratch,PREF_PRINT_FONT);
	fID = *scratch ? GetFontID(GetPref(scratch,PREF_PRINT_FONT))
								 : FontIsFixed ?
									 GetFontID(GetRString(scratch,PRINT_FONT)) : FontID;
	GetPref(scratch,PREF_PRINT_FONT_SIZE);
	fSize = *scratch ? GetPrefLong(PREF_PRINT_FONT_SIZE) : FontSize;
	if (fSize!=FontSize || fID!=FontID)
	{
		FontID = fID;
		FontSize = fSize;
		FontWidth = GetWidth(FontID,FontSize);
		FontLead = GetLeading(FontID,FontSize);
		FontDescent = GetDescent(FontID,FontSize);
		FontAscent = GetAscent(FontID,FontSize);
		FontIsFixed = IsFixed(FontID,FontSize);
	}
}

/************************************************************************
 *
 ************************************************************************/
void GetURect(Rect *useRect)
{
	short t;
	Rect rPage,rPaper;
	
//	PMGetPhysicalPageSizeAsRect(gPageSetup,&rPage);
	PMGetAdjustedPageSizeAsRect(gPageSetup,&rPage);
	InsetRect(&rPage,3,3);	/* Apple lies about page size sometimes */
//	PMGetPhysicalPaperSizeAsRect(gPageSetup,&rPaper);
	PMGetAdjustedPaperSizeAsRect(gPageSetup,&rPaper);
	t = rPaper.left+GetRLong(PRINT_LEFT_MAR);
	rPage.left = MAX(rPage.left,t);
	t = rPaper.right-GetRLong(PRINT_RIGHT_MAR);
	rPage.right = MIN(rPage.right,t);
	*useRect = rPage;
}

/**********************************************************************
 * DoPageSetup - carry on the Page Setup dialog with the user
 **********************************************************************/
void DoPageSetup()
{
	short err=noErr;
	PMPageFormat pageSetup;
	PMPrintSettings printSettings;
	Boolean	accepted;
	short saveFile = CurResFile();

	/*
	 * try to open the printer
	 */
	PMBegin();
	if (PMError())
	{
		WarnUser(HaveOSX()?NO_OSX_PRINTER:NO_PRINTER,PMError());
		goto done;
	}
	
	/*
	 * make sure we have some sort of page setup record
	 */
	if (err = GetPrintSettings(&pageSetup,&printSettings))
	{
		WarnUser(COULDNT_SETUP,err);
		PMEnd();
		goto done;	
	}
	
	/*
	 * let the user see it
	 */
	PushCursor(arrowCursor);
	PMPageSetupDialog(pageSetup,&accepted);
	PopCursor();

   	if (accepted)
	{
		/*
		 * save it in our prefs file
		 */
		Handle	hPageSetup;
    	PMFlattenPageFormat(pageSetup,&hPageSetup);
		if (err=SettingsHandle(PrintResType(),nil,PRINT_PAGE_SETUP,hPageSetup))
			WarnUser(SAVE_SETUP,err);
		else
		{
			MyUpdateResFile(SettingsRefN);
			ReleaseResource(hPageSetup);
		}
	}
	
	/*
	 * done
	 */
	PMDisposePageFormat(pageSetup);
	PMDisposePrintSettings(printSettings);
  done:
	PMEnd();
	UseResFile(saveFile);
}

/************************************************************************
 * GetPrintSettings - load previous print settings or create new one
 ************************************************************************/
OSErr GetPrintSettings(PMPageFormat *pPageSetup,PMPrintSettings *pPrintSettings)
{
	PMPageFormat pageSetup=nil;
	PMPrintSettings printSettings=nil;
	OSErr	err=noErr;
	
	Handle	hPageSetup=nil,hPrintSettings=nil;
	
	hPageSetup = GetResource(PrintResType(),PRINT_PAGE_SETUP);
	hPrintSettings = GetResource(PrintResType(),PRINT_SETTINGS);
	if (hPageSetup || hPrintSettings)
	{
		//	got carbon print settings
		if (hPageSetup)
		{
			PMUnflattenPageFormat(hPageSetup,&pageSetup);
			ReleaseResource(hPageSetup);
		}
		if (hPrintSettings)
		{
			PMUnflattenPrintSettings(hPrintSettings,&printSettings);
			ReleaseResource(hPrintSettings);
		}
	}
	else if (hPageSetup = GetResource_(PrintResType(),PRINT_CSOp))
	{
		//	got older printing manager settings
		PMConvertOldPrintRecord(hPageSetup,&printSettings,&pageSetup);
	}

	if (!hPageSetup)
		if (!(err = PMNewPageFormat(&pageSetup))) 
			err = PMDefaultPageFormat(pageSetup);
	if (!hPrintSettings)
		if (!(err = PMNewPrintSettings(&printSettings)))
			err = PMDefaultPrintSettings(printSettings);
	if (!pageSetup || !printSettings)
	{
		WarnUser(COULDNT_SETUP,err=MemError()); 
		return err;
	}

	*pPageSetup = pageSetup;
	*pPrintSettings = printSettings;
	return noErr;
}

/************************************************************************
 * PrintResType - OS X or older printer resource?
 ************************************************************************/
static ResType PrintResType(void)
{
	return HaveOSX() ? OSX_PRINT_RTYPE : PRINT_RTYPE;
}
