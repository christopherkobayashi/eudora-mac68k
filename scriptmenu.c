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

#include "scriptmenu.h"
#include <OSA.h>
#define FILE_NUM 137

#define MENU_FILENAME_PROP 'fNam'

typedef enum {
	SCRIPTS_OPENFOLDER_ITEM=1,
	SCRIPTS_DIVIDER_ITEM
} ScriptsMenuEnum;

static OSErr RunScript(FSSpecPtr spec);

static ComponentInstance	gGenericComponent;

/**********************************************************************
 * BuildScriptMenu - build the script menu
 **********************************************************************/
void BuildScriptMenu(void)
{
	FSSpec	spec;
	MenuHandle mh = GetMenu(SCRIPTS_MENU);
	CInfoPBRec hfi;
	short	item;
	
	if (GetScriptFolderSpec(&spec)) return;	// no scripts
	if (!HasFeature(featureScriptsMenu)) return;
	if (!mh) return;

	//	remove any scripts we've already added
	for(item=CountMenuItems(mh);item>SCRIPTS_DIVIDER_ITEM;item--)
		DeleteMenuItem(mh,item);

	IsAlias(&spec,&spec);
	*spec.name = 0;
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = spec.name;
	while (!DirIterate(spec.vRefNum,spec.parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType == 'osas' || 
			(hfi.hFileInfo.ioFlFndrInfo.fdType == 'APPL' && 
			(hfi.hFileInfo.ioFlFndrInfo.fdCreator=='aplt' || 
			hfi.hFileInfo.ioFlFndrInfo.fdCreator=='dplt')))
		{
			//	found one
			StrFileName fileName;
			StrFileName extension;

			// Remove any filename extension for menu item
			SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(spec.name,fileName,extension,31);
			MyAppendMenu(mh,fileName);

			// Save full filename as menu property
			SetMenuItemProperty(mh,CountMenuItems(mh),CREATOR,MENU_FILENAME_PROP,*spec.name+1,spec.name);
		}
	}
	if (mh) 
		InsertMenu(mh,HELP_MENU);
}

/**********************************************************************
 * DoScriptMenu - process selection from script menu
 **********************************************************************/
void DoScriptMenu(short item)
{
	FSSpec	spec;
	
	GetScriptFolderSpec(&spec);
	if (item==SCRIPTS_OPENFOLDER_ITEM)
	{
		// open Scripts folder		
		OpenOtherDoc(&spec,false,false,nil);
	}
	else
	{
		GetMenuItemProperty(GetMHandle(SCRIPTS_MENU),item,CREATOR,MENU_FILENAME_PROP,sizeof(spec.name),NULL,spec.name);
		RunScript(&spec);		
	}
	UseFeature(featureScriptsMenu);
}

/**********************************************************************
 * GetScriptFolderSpec - where is the Scripts folder?
 **********************************************************************/
OSErr GetScriptFolderSpec(FSSpecPtr spec)
{
	Str255	filename;
	OSErr	err;
	long domains[]={kUserDomain,kLocalDomain,kSystemDomain,kNetworkDomain};
	FSSpec mySpec;
	short n = sizeof(domains)/sizeof(domains[0]);

	// look for scripts in the Eudora Folder itself
	mySpec.vRefNum = ItemsFolder.vRef;
	mySpec.parID = ItemsFolder.dirId;
	*mySpec.name = 0;
	if (!SubFolderSpecOf(&mySpec,SCRIPTS_FOLDER,false,&mySpec))
	{
		*spec = mySpec;
		return noErr;
	}
	
	// look in application support folders
	while (n-->0)
		if (!FindSubFolderSpec(domains[n],kApplicationSupportFolderType,EUDORA_EUDORA,false,&mySpec))
		{
			if (!SubFolderSpecOf(&mySpec,SCRIPTS_FOLDER,false,&mySpec))
			{
				*spec = mySpec;
				return noErr;
			}
		}
	
	// look inside Eudora application folder
	if (!(err=GetFileByRef(AppResFile,spec)))
		if (err=FSMakeFSSpec(spec->vRefNum,spec->parID,GetRString(filename,SCRIPTS_FOLDER),spec))
		{
			// try inside Eudora Stuff folder
			if (!(err=GetFileByRef(AppResFile,spec)))
			if (!(err=FSMakeFSSpec(spec->vRefNum,spec->parID,GetRString(filename,STUFF_FOLDER),spec)))
			err=FSMakeFSSpec(spec->vRefNum,SpecDirId(spec),GetRString(filename,SCRIPTS_FOLDER),spec);
		}
	if (!err)
	{
		spec->parID = SpecDirId(spec);
		*spec->name = 0;
	}
	return err;
}

/**********************************************************************
 * RunScript - run this script
 *  this code is adapted from "develop" magazine, issue 26
 *  <http://developer.apple.com/dev/techsupport/develop/issue26/according.html>
 **********************************************************************/
static OSErr RunScript(FSSpecPtr spec)
{
	Handle	hScript = nil;
	AEDesc	scriptDesc;
	OSAID	scriptID;
	OSErr	err = noErr;
	short	savedRes, refNum;
	Str255	sErr;

	if (!OSALoad) return -1;	// make sure OSA is available

	IsAlias(spec,spec);

	// Script may be in resource or data fork. Look first for resource.

	// Open the resource fork and grab the script resource.
	savedRes = CurResFile();
	refNum = FSpOpenResFile(spec,fsRdPerm);
	if (refNum != -1)
	{
		UseResFile(refNum);
		if (hScript = Get1Resource(kOSAScriptResourceType,128))
			DetachResource(hScript);
		else
			err = ResError();
	}
	else
		err = ResError();
	CloseResFile(refNum);
	UseResFile(savedRes);

	if (!hScript)
	{
		// No resource. Try data fork.
		err = Snarf(spec,&hScript,0);
	}

	// make sure we're connecting to generic scripting component
	if (!gGenericComponent)
	{
		gGenericComponent = OpenDefaultComponent(kOSAComponentType,kOSAGenericScriptingComponentSubtype);
		if (!gGenericComponent) err = -1;
	}
	
	// Prepare and run the script. 
	if (hScript)
	{
		if (!(err = AECreateDesc(typeOSAGenericStorage,LDRef(hScript),GetHandleSize(hScript),&scriptDesc)))
		{
			if (OSALoadExecute(gGenericComponent,&scriptDesc,kOSANullScript,kOSAModeNull,&scriptID) == errOSAScriptError)
			{
				//	Report execution error
				AEDesc	errorDesc;
				Handle	hError;
				if (!OSAScriptError(gGenericComponent,kOSAErrorMessage,typeChar,&errorDesc))
				{
					if (!AEGetDescDataHandle(&errorDesc,&hError))
					{
						MakePStr(sErr,*hError,GetHandleSize(hError));
						AEDisposeDescDataHandle(hError);
						ComposeStdAlert(kAlertStopAlert,SCRIPT_EXECUTION_ERR,spec->name,sErr);
					}
					AEDisposeDesc(&errorDesc);
				}
			}
		}
		OSADispose(gGenericComponent,scriptID);
		AEDisposeDesc(&scriptDesc);
		ZapHandle(hScript);
	}
	else
		err = -1;

	return err;
}
