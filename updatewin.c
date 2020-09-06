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

#include "updatewin.h"

#define FILE_NUM 130
/* Copyright (c) 1999 by QUALCOMM Incorporated */

#pragma segment UpdateWin

#define MemResError()	(MemError () ? MemError ()	: ResError ())

typedef struct {
	MyWindowPtr	win;
	Boolean			inited;
} WinData;

static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static Boolean DoClose (MyWindowPtr win);
static Boolean DoPosition (Boolean save, MyWindowPtr win);

/************************************************************************
 * OpenUpdateWin - open the update window
 ************************************************************************/

OSErr OpenUpdateWin (void)

{
	OSErr	theError;
	
	if (gWin.inited) {
		SelectWindow_ (gWin.win);
		return;
	}
	
	gWin.win = GetNewMyWindow (UPDATE_WIND, nil, BehindModal, false, true, UPDATE_WIN);
	theError = MemError () ? MemError ()	: ResError ();
	
	if (!theError) {
//		gWin.win->saveSize	= true;
		gWin.win->close			= DoClose;
		gWin.win->position	= DoPosition;
		gWin.inited = true;

		gWin.win->isRunt = true;
		ShowMyWindow (gWin.win);
		gWin.win->isRunt = false;
	}
	if (theError) {
		if (gWin.win)
			CloseMyWindow (gWin.win);
		WarnUser (COULDNT_WIN, theError);
	}
	return (theError);
}


/************************************************************************
 * DoClose - close the update window
 ************************************************************************/
static Boolean DoClose(MyWindowPtr win)
{
#pragma unused(win)
	
	gWin.inited = false;
	return (true);
}


/************************************************************************
 * DoPosition - remember the position of the update window
 ************************************************************************/
Boolean DoPosition (Boolean save, MyWindowPtr win)

{
	Rect r;
	Boolean zoomed;
	FSSpec spec = (*(TextDHandle) win->qWindow.refCon)->spec;
	
	if (!*spec.name) return(False);
	
	if (save)
	{
		utl_SaveWindowPos((WindowPtr)win,&r,&zoomed);
		SavePosFork(spec.vRefNum,spec.parID,spec.name,&r,zoomed);
	}
	else
	{
		if (!RestorePosFork(spec.vRefNum,spec.parID,spec.name,&r,&zoomed))
			return(False);
		utl_RestoreWindowPos((WindowPtr)win,&r,zoomed,1,TitleBarHeight(win),LeftRimWidth(win),(void*)FigureZoom,(void*)DefPosition);
	}
	return(True);
}


//
//	UpdateCheck
//
//		Contact eudora.com and check for application updates.
//		If there are updates write the results into a file
//		Open a window displaying this information
//

OSErr UpdateCheck (void)

{
	FSSpec	updateSpec;
	Handle	url;
	Str255	filename;
	OSErr 	theError;
	long		reference;
	
	// First, we need a file!
	theError = FSMakeFSSpec (Root.vRef, Root.dirId, GetRString (filename, UPDATE_FILE), &updateSpec);
	if (theError == fnfErr)
		theError = FSpCreate (&updateSpec, CREATOR, 'TEXT', smSystemScript);
	
	// Send a GET to the server to request updates
	if (!theError)
		if (url = GenerateAdwareURL (UPDATE_SITE, PATH_TO_UPDATE_QUERY_PAGE, true)) {
			theError = DownloadURL (LDRef (url), &updateSpec, nil, FinishedUpdateCheck, &reference, nil);			
			ZapHandle (url);
		}
	return (theError);
}


void FinishedUpdateCheck (long refCon, OSErr theError)

{
	MyWindowPtr win;
	FSSpec			updateSpec;
	Str255			filename;

	if (!theError)
		theError = FSMakeFSSpec (Root.vRef, Root.dirId, GetRString (filename, UPDATE_FILE), &updateSpec);
	if (!theError)
		win = OpenText (&updateSpec, nil, true, nil, true, true);

//		theError = OpenUpdateWin ();
}