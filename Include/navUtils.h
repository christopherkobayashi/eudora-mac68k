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

#ifndef NAVUTILS_H
#define	NAVUTILS_H
#include <Navigation.h>

#define	rNavSaveAsGuessParagraphsItem		1
#define	rNavSaveAsIncludeHeadersItem		2
#define	rNavSaveAsStationeryItem				3
#define	rNavSaveAsGoToStationeryItem		4

#define	rNavOdocUseFinder								1
#define	rNavOdocAlwaysUseThisApp				2

#define	rNavURLAppAlwaysUseThisApp			1

//
//	"Hey, I know!  Let's create a key new system service and not
//	 tell anyone that we might fiddle with the current heap zone
//	 and resource chain!"
//
//	"No, no... not like that...  Instead, let's not tell AND supply
//	 lots of sample code that also leaves out this important fact!"
//
//	"Wait!  Wait!  Let's make it even better...  Later after they've
//	 converted their applications to use this new service we'll put
//	 out a new draft of Inside Macintosh that indiscretely mentions
//	 an "important note" about the f-ed up resource chain and heap
//	 zone during Nav callbacks."
//
//	"Yeah!  Good thinking!!!  Something that important should be
//	 splashed all over a Tech Note -- they'll never think to discover
//	 a well hidden new draft of Inside Macintosh!"
//
//	"And -- this is pure genius -- let's tell them that this new
//	 service replaces an existing service and HAS to be used because
//	 we're planning on dropping the old service for Mac OS X... and,
//	 we'll recommend that they use SetZone to manage the heap mess
//	 we've created -- and we're not planning on supporting that call
//	 under Mac OS X!!"
//
//	"Brilliant!"
//
//	"Yeah!"
//
//	"Okay, now let's go over to the Pepper Mill and hassle the fry cooks."
//

typedef	struct {
	THz		heapZone;
	short	resFile;
} ZoneAndResFileRec, *ZoneAndResFilePtr, **ZoneAndResFileHandle;

typedef struct {
	ZoneAndResFileRec	zoneAndResFile;
	Boolean						permanently;
	Boolean 					finder;
	OSType 						type;
} SFODocType, *SFODocPtr;

typedef struct {
	ZoneAndResFileRec	zoneAndResFile;
	Boolean 					permanently;
	short							protocol;
} URLHookOptions, *URLHookOptionsPtr;


void SeeIfWeShouldPreloadNav (void);
Boolean UseNavServices (void);
void GetZoneAndResFile (ZoneAndResFilePtr zoneAndResFile);
void SetZoneAndResFile (ZoneAndResFilePtr zoneAndResFile);
void CompAttachNav (MyWindowPtr win, Boolean insertDefault);

void DoSFOpenNav (short modifiers);
NavTypeListHandle NewNavTypeList (OSType applicationSignature, short numTypes, OSType typeList[]);
OSErr GetSpecFromNthDesc (AEDescList *list, short n, FSSpec *spec);

Boolean GetFolderNav(char *name,short *volume,long *folder);
pascal Boolean getFolderObjectFilterProc (AEDesc *theItem, void *info, ZoneAndResFilePtr zoneAndResFile, NavFilterModes filterMode);
Boolean MakeUserFindSettingsNav (FSSpecPtr theSpec);

OSErr SFPutOpenNav (FSSpecPtr spec, long creator, long type, short *refN, short ditlID, ScriptCode *script, FSSpecPtr defaultSpec,PStr windowTitle, PStr message);
pascal void saveAsNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, ZoneAndResFilePtr zoneAndResFile);
void saveAsCustomHilite (DialogPtr navDialog, NavDialogRef context);
OSErr HandleSaveAsNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context);

short CountOSTypes (OSType *types, short limit);
OSErr GetFileNav (OSType *types, short titleResID, Str255 prompt, short actionButtonLabelResID, Boolean allowPackages, FSSpec *spec, Boolean *good, ProcPtr objectFilterProc);

pascal Boolean mailboxFileFilterNavProc (AEDesc *theItem, void *info, ZoneAndResFilePtr zoneAndResFile, NavFilterModes filterMode);

OSErr SFODocNav (FSSpecPtr doc, SFTypeList types, Str255 prompt, FSSpec *spec, SFODocPtr optionsPtr, Boolean *good);
pascal void sfODocNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, SFODocPtr optionsPtr);
OSErr HandleODocNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, SFODocPtr optionsPtr);
pascal Boolean sfOdocObjectFilterProc (AEDesc *theItem, void *info, SFODocPtr optionsPtr, NavFilterModes filterMode);

OSErr SFURLAppNav (PStr proto,AliasHandle *alias, SFTypeList types, Str255 prompt, FSSpec *spec, URLHookOptionsPtr optionsPtr, Boolean *good);
pascal void SFURLAppNavProc (NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms, URLHookOptionsPtr optionsPtr);
OSErr HandleURLAppNavEvent (DialogPtr navDialog, EventRecord *event, NavDialogRef context, URLHookOptionsPtr optionsPtr);

#endif