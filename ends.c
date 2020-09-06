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

#include <Folders.h>
#include <KeychainCore.h>
#include <KeychainHI.h>

#include <conf.h>
#include <mydefs.h>

#include <Globals.h>

#include "ends.h"
#include "buildversion.h"
#define FILE_NUM 12
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * initialization and cleanup
 **********************************************************************/

#pragma segment Ends
typedef struct
{
	Str31 *badNames;
	short badCount;
	Str31 newText;
#ifdef TWO
	Str31 otherText;
#endif
#ifdef	IMAP
	Str31 thisBoxText;
#endif
	short level;
	CInfoPBRec hfi;
	Str63 hfi_namePtr;
#ifdef	IMAP
	Str63 enclosingFolderName;
#endif
} BoxFolderDesc, *BoxFolderDescPtr;
#ifdef	IMAP
typedef struct {FSSpec spec; Boolean isDir; Boolean hasUnread; Boolean skipIt; Boolean isIMAPpers; Boolean moveToTop; long imapAttributes;} NameType;
#else
typedef struct {FSSpec spec; Boolean isDir; Boolean hasUnread; Boolean skipIt;} NameType;
#endif
void ZapResMenu(short id);
Boolean HaveOptiMEM(void);
	void doInitDEF(short resId,UniversalProcPtr upp,OSType type);
	void SettingsInit(FSSpecPtr spec,Boolean system,Boolean changeUserState);
	void OpenSettingsFile(FSSpecPtr spec);
	void PutUpMenus(void);
	void CreateBoxes(void);
	void SetupInsurancePort(void);
	void SetRunType(void);
	Boolean ModernSystem(void);
	void BoxFolder(short vRef,long dirId,MenuHandle *menus,BoxFolderDescPtr bfd,Boolean *anyUnread,Boolean appending,Boolean isIMAP);
	void RemoveBoxMenus(void);
void BuildMarginMenu(void);
#ifdef NEVER
	void MakeHmnusPurge(void);
	void TrashHmnu(void);
#endif
	void OpenPlugIns(void);
	void MarkHavingUnread(MenuHandle mh, short item);
	Boolean UsingNewTables(void);
	void AwaitStartupAE(void);
	void AddHelpMenuItems(void);
	void AddWSMenuItems(void);
	void Splash(void);
	void MakeWordChars(void);
	void ConvertSignature(void);
	short PlugInDir(short vRef,long dirId);
	void GenAliasList(void);
	void BoxNamesSort(NameType **names);
	int NameCompare(NameType *n1, NameType *n2);
	void NameSwap(NameType *n1, NameType *n2);
	void MakeUserFindSettings(FSSpecPtr spec);
	Boolean MakeUserFindSettingsStd(FSSpecPtr spec);
	void GetAndInsertHier(short menu);
#ifdef	IMAP
	Boolean InferUnread(CInfoPBRec *hfi, Boolean imap);
#else
	Boolean InferUnread(CInfoPBRec *hfi);
#endif
	void CheckAttachFolder(void);
	void AddRes2HelpMenu(MenuHandle hm,Handle resH);
OSErr AddIconsToColorMenu(MenuHandle mh);
OSErr LogPlugins(void);
void SaneSettings(FSSpecPtr spec);
void HuntSpellswell(void);
Boolean HaveOT(void);
void CreateItemsFolder(void);
void CreateSpoolfolder(void);
void CreateMailFolder(void);
void CreatePartsfolder(void);
void CreateCachefolder(void);
#ifdef BATCH_DELIVERY_ON
void CreateDeliveryfolder(void);
#endif
OSErr LocalizeFiles(void);
void MoveMailboxesToMailFolder(void);
void ReloadStdSizes(void);
#ifdef LABEL_ICONS
OSErr RefreshLabelIcon(short item,RGBColor *color);
void UpdateLabelControls(void);
#endif
#ifdef USECMM
Boolean HaveCMM();
#endif
Boolean HaveVM(void);
Boolean FallbackSettings(FSSpecPtr spec);
OSErr LocalizeRename(FSSpecPtr spec,short id);
static void InitIntScript(void);
void ScanPrefs(void);
OSErr RegisterEudoraComponentFiles(void); // In peteglue.c
void AddCustomSortItems(void);
void InitCDEF(short resId,ControlDefProcPtr proc);
void InitLDEF(short resId,ListDefProcPtr proc);
pascal OSStatus ControlCNTLToCollection (Rect *bounds, SInt16 value, Boolean visible, SInt16 max, SInt16 min, SInt16 procID, SInt32 refCon, ConstStr255Param title, Collection collection);

#pragma segment Main

pascal void PeteCalled(Boolean entry);
pascal void PeteCalled(Boolean entry) {MemCanFail = entry;}
Boolean OkSettingsFileSpot(FSSpecPtr spec);
static void PrefillSettings(void);
static OSErr FindEudoraFolder(FSSpecPtr spec,StringPtr folder,short vRefNum,OSType folderType);

ModalFilterUPP DlgFilterUPP;
PETEGraphicHandlerProcPtr GraphicHandlers[] = 
{
	PersGraphic,
	FileGraphic,
	HorizRuleGraphic,
	TableGraphic,
	nil
};

#pragma segment Ends
/**********************************************************************
 * Initialize - set up my stuff as well as the Mac managers
 **********************************************************************/
void Initialize(void)
{
	SInt32			gestaltResult;

#ifdef PEE
	OSErr err;
	short i;	
#endif

	VM = HaveVM();
	MacInitialize(20,40 K); 			/* initialize mac managers */
	SetQDGlobalsRandomSeed(LocalDateTime());
	
	// Check for Appearance Extension
	if (err = Gestalt(gestaltAppearanceAttr, &gestaltResult))	DieWithError(NO_APPEARANCE, err);
	/* MJN *//* make sure the library is REALLY loaded and linked */
	if ((long)RegisterAppearanceClient == kUnresolvedCFragSymbolAddress)
		DieWithError(NO_APPEARANCE, cfragNoSymbolErr);
	gAppearanceIsLoaded = true;	// Okay, so we didn't die on loading AM
	gGoodAppearance = !Gestalt(gestaltAppearanceVersion,&gestaltResult);
	g16bitSubMenuIDs = gestaltResult >= 0x0110;
	gBetterAppearance = gestaltResult >= 0x0110;
	gBestAppearance = gestaltResult >= 0x0114;
	gMaxBoxLevels = g16bitSubMenuIDs ? (BOX_MENU_LIMIT-BOX_MENU_START)/2 : OLD_MAX_BOX_LEVELS;

	gHave85MenuMgr = !Gestalt(gestaltMenuMgrAttr,&gestaltResult) 
		&& gestaltResult&gestaltMenuMgrPresent 
		&& (long)EnableMenuItem != kUnresolvedCFragSymbolAddress;
		
	SetRunType(); 							/* figure out what kind of run this is */
	for (i=0;i<NSpare;i++) MakeGrow(SPARE_SIZE,i); 			/* reserve some memory for emergencies */
	SetGrowZone(NewGrowZoneUPP(MyGrowZone));			/* let the mac know it's there */
	DoMonitor = True;
	ThereIsColor = utl_HaveColor();
	if (!ModernSystem()) DieWithError(OLD_SYSTEM,0);
	if (!(MousePen = NewRgn())) DieWithError(MEM_ERR,MemError());
	if ((err = InitUnicode()) != noErr) DieWithError(MEM_ERR,err);

	InitCarbonUtil();	//	Set up Carbon utilities

	RegisterProFeatures ();	// (jp) Added to register each of the features supported in Pro
	
	#ifdef USECMM
	gHasCMM = HaveCMM();
	if( gHasCMM && InitContextualMenus() )
		gHasCMM = false;
	#endif

	InitIntScript();
	OpenPlugIns();
	
#ifdef TWO
	Splash();
#endif
	InitLDEF(ICON_LDEF,SettingsListDef);
	InitLDEF(TLATE_LDEF,TlateListDef);
#ifdef FANCY_FILT_LDEF
	InitLDEF(FILT_LDEF,FiltListDef);
#endif
	InitLDEF(FEATURE_LDEF,FeatureListDef);
	InitCDEF(APP_CDEF,AppCdef);
	InitCDEF(COL_CDEF,ColCdef);
	InitCDEF(LIST_CDEF,ListCdef);
#ifdef DEBUG
	InitCDEF(DEBUG_CDEF,DebugCdef);
#endif
	InitLDEF(LISTVIEW_LDEF,ListViewListDef);
#ifdef TASK_PROGRESS_ON
	InitLDEF(TASK_LDEF,ListItemDef);
#endif
	
	INIT_UPP(DlgFilter,ModalFilter);

	//	Remember any plugins in the system (or extensions) folder.
	//	This must be called before PETEInit which installs all the
	//	components including the plug-ins. (ALB)
	ETLGetSystemPlugins();
	// Register for PETE and EMSAPI
	RegisterEudoraComponentFiles();
#ifdef PEE
	/*
	 * initialize editor
	 */
	if (err=PETEInit(&PETE,GraphicHandlers))
		DieWithError(err==invalidComponentID ? NO_PETE:(err==afpBadVersNum?BAD_PETE:PETE_ERR),err);
	PETESetMemFail(PETE,&MemCanFail);
#ifdef DEBUG
	PETEDebugMode(PETE,RunType!=Production);
#endif
	Pslh = NewZH(PETEStyleEntry);
	PETESetCallback(PETE,nil,PeteCalled,peHasBeenCalled);
	PETESetLabelCopyMask(PETE,LABEL_COPY_MASK);
#endif

	ScanPrefs();

	/* MJN *//* initialize LDAP and the LDAP utilities */
#ifdef LDAP_ENABLED
	err = InitLDAP();
	if (err)
		DieWithError(LDAP_INIT_ERR, err);
#endif

	/* MJN *//* initialize the nickname watcher */
	err = InitNicknameWatcher();
	if (err)
		DieWithError(NICK_WATCHER_LOAD_ERR, err);

	if (err=AEObjectInit()) DieWithError(INSTALL_AE,err);	
	InstallAE();
	if (HasDragManager())
	{
		DECLARE_UPP(PantyTrack,DragTrackingHandler);
		DECLARE_UPP(PantyReceive,DragReceiveHandler);

		INIT_UPP(PantyTrack,DragTrackingHandler);
		INIT_UPP(PantyReceive,DragReceiveHandler);
		
		InstallTrackingHandler(PantyTrackUPP,nil,nil);
		InstallReceiveHandler(PantyReceiveUPP,nil,nil);
	}
	MakeWordChars();
	
	OTIs = HaveOT();

	OptiMEMIs = HaveOptiMEM();

	VicomIs = IsVICOM();
	
	// The mailbox rename stack
	if (!StackInit(sizeof(MBRenameProcPtr),&MBRenameStack))
	{
		MBRenameProcPtr proc;
		
		proc = TellFiltMBRename; StackQueue(&proc,MBRenameStack);
		proc = TellToolMBRename; StackQueue(&proc,MBRenameStack);
	}

#ifdef	DEMO
	TimeBomb();
#endif

	//LOGLINE;
	AwaitStartupAE();						/* wait for the Finder to tell us what to do */
	//LOGLINE;

	UpdateFeatureUsage (gFeatureList);

#ifdef NAG
	if (nagState)
		CheckNagging ((*nagState)->state);
#endif

	if (!JunkPrefDeadPluginsWarning() && ETLCountTranslatorsLo(EMSF_JUNK_MAIL,EMS_ModePaid)>ETLCountTranslators(EMSF_JUNK_MAIL))
		JunkDownDialog();
}

/************************************************************************
 * ScanPrefs - scan the prefs folder for "interesting" items
 ************************************************************************/
void ScanPrefs(void)
{
	Str63 name;
	CInfoPBRec hfi;
	short vRef;
	long dirId;
	
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
  if (!FindFolder(kOnSystemDisk,kPreferencesFolderType,False,&vRef,&dirId))
		// keep going until we run out of files or find everything
#ifdef SPEECH_ENABLED
		while((!AutoDoubler || !PrefsPlugIns || !*TCPprefFileSpec.name || !*PPPprefFileSpec.name || !*SpeechPrefFileSpec.name) &&
					!DirIterate(vRef,dirId,&hfi))
#else
		while((!AutoDoubler || !PrefsPlugIns || !*TCPprefFileSpec.name || !*PPPprefFileSpec.name) &&
					!DirIterate(vRef,dirId,&hfi))
#endif
			switch(hfi.hFileInfo.ioFlFndrInfo.fdCreator)
			{
				case TCP_PREF_FILE_CREATOR:	// ot tcp prefs file
					if (hfi.hFileInfo.ioFlFndrInfo.fdType == TCP_PREF_FILE_TYPE)
						SimpleMakeFSSpec(vRef,dirId,name,&TCPprefFileSpec);
					break;
				case PPP_PREF_FILE_CREATOR:
					if (hfi.hFileInfo.ioFlFndrInfo.fdType == PPP_PREF_FILE_TYPE)
						SimpleMakeFSSpec(vRef,dirId,name,&PPPprefFileSpec);
					break;
				case CREATOR:	// one of ours
					PrefsPlugIns = True;
					break;
				case 'DDAP':	// Autodoubler
					if (hfi.hFileInfo.ioFlFndrInfo.fdType=='ADDA')
						AutoDoubler = SyncRW = True;
					break;
#ifdef SPEECH_ENABLED
				case SPEECH_PREF_FILE_CREATOR:	// Speech Preferences
					if (hfi.hFileInfo.ioFlFndrInfo.fdType == SPEECH_PREF_FILE_TYPE)
						SimpleMakeFSSpec (vRef, dirId, name, &SpeechPrefFileSpec);
					break;
#endif
			}
}

/************************************************************************
 * ReloadStdSizes - reload standard font size array
 ************************************************************************/
void ReloadStdSizes(void)
{
	short size;
	short i;
	short narml = FontSize;
	short finnarml = FixedSize;
	
	ZapHandle(StdSizes);
	ZapHandle(FixedSizes);
	StdSizes = NuHandle(0);
	if (!StdSizes) DieWithError(MEM_ERR,MemError());	// yeah, right
	FixedSizes = NuHandle(0);
	if (!FixedSizes) DieWithError(MEM_ERR,MemError());	// yeah, right
	for (i=1;;i++)
		if (size=GetRLong(StdSizesStrn+i))
		{
			if (size==narml) narml = 0;
			if (size==finnarml) finnarml = 0;
			if (size>narml && narml)
			{
				PtrPlusHand(&narml,(Handle)StdSizes,sizeof(short));
				narml = 0;
			}
			if (size>finnarml && finnarml)
			{
				PtrPlusHand(&finnarml,(Handle)FixedSizes,sizeof(short));
				finnarml = 0;
			}
			PtrPlusHand(&size,(Handle)StdSizes,sizeof(short));
			PtrPlusHand(&size,(Handle)FixedSizes,sizeof(short));
		}
		else break;
}

#ifdef USECMM
/**********************************************************************
 * HaveCMM - do we have the contextual memory manager?
 **********************************************************************/
Boolean HaveCMM()
{
#if !TARGET_RT_MAC_CFM
	return true;
#else
	return (InitContextualMenus != (void *)kUnresolvedCFragSymbolAddress) &&
	       (IsShowContextualMenuClick != (void *)kUnresolvedCFragSymbolAddress) &&
	       (ContextualMenuSelect != (void *)kUnresolvedCFragSymbolAddress) &&
	       (ProcessIsContextualMenuClient != (void *)kUnresolvedCFragSymbolAddress);
#endif
}
#endif

/**********************************************************************
 * HaveVM - do we have VM?
 **********************************************************************/
Boolean HaveVM(void)
{
	long value;
	
	if (Gestalt(gestaltVMAttr, &value))  return(False);
	return((value&1)!=0);
}

/**********************************************************************
 * HaveOT - OpenTransport is lurking
 **********************************************************************/
Boolean HaveOT(void)
{
	long result;
	OSErr err = Gestalt(gestaltOpenTpt, &result);

	return(!err &&
				 (result & gestaltOpenTptPresentMask) &&
				 (result & gestaltOpenTptTCPPresentMask));
}

/**********************************************************************
 * HaveOptiMEM - OptiMEM is lurking
 **********************************************************************/
Boolean HaveOptiMEM(void)
{
	long result;
	OSErr err = Gestalt('xMe0', &result);

	return(!err);
}

/************************************************************************
 * MakeWordChars - set the IsWordChar array
 ************************************************************************/
void MakeWordChars(void)
{
	short i;
	
	for (i=0;i<256;i++)
		IsWordChar[i] = (CharacterType(((Ptr)&i)+1,0,nil)&0xf)!=0;
}

#ifdef TWO
DialogPtr DBSplash;
void ChangeTEFont(TEHandle teh,short font,short size);
/************************************************************************
 * Splash - show the splash box
 ************************************************************************/
void Splash(void)
{
	MyWindowPtr	dgWin;
	DialogPtr dg;
	Str255 versionString, regInfo;
	Handle h;
	short t;
	Rect r,portR;
	
	VersString(versionString);
	AboutRegisterString(regInfo);
	
	MyParamText(versionString,regInfo,"","");
	SetDialogFont(SmallSysFontID());
	dgWin = GetNewMyDialog(ABOUT_ALRT,nil,nil,InFront);
	ConfigFontSetup(dgWin);
	SetDialogFont(0);
	if (dgWin)
	{
		dg = GetMyWindowDialogPtr (dgWin);
		DBSplash = (void*)dg;
		for (t=CountDITL(dg)-1;t>=12;t--) HideDialogItem(dg,t);
		GetDialogItem(dg,5,&t,&h,&r);
		GetPortBounds(GetDialogPort(dg),&portR);
		SizeWindow(GetDialogWindow(dg),portR.right-portR.left,r.bottom+1,False);
		ShowWindow(GetDialogWindow(dg));
		UpdateMyWindow(GetDialogWindow(dg));
	}
}
#endif

/************************************************************************
 * ConvertSignature - move signature from Eudora Settings to external file
 ************************************************************************/
void ConvertSignature(void)
{
	Str31 s;
	FSSpec spec;
	Handle sig;
	long newDirId;
	
	if (!DirCreate(Root.vRef,Root.dirId,GetRString(s,SIG_FOLDER),&newDirId))
	{
		/*
		 * created directory; read old sig
		 */
		GetRString(s,SETTINGS_FILE);
		FSMakeFSSpec(Root.vRef,Root.dirId,s,&spec);
		if (Snarf(&spec,&sig,0)) sig = NuHandle(0);
		/*
		 * create new sig file
		 */
		FSMakeFSSpec(Root.vRef,newDirId,GetRString(s,SIGNATURE),&spec);
		Blat(&spec,sig,False);
#ifdef TWO
		FSMakeFSSpec(Root.vRef,newDirId,GetRString(s,ALT_SIG),&spec);
		SetHandleBig_(sig,0);
		Blat(&spec,sig,False);
#endif
		ZapHandle(sig);
	}
	else
	{
		if (fnfErr==SigSpec(&spec,0)) FSpCreate(&spec,CREATOR,'TEXT',smSystemScript);
#ifdef TWO
		if (fnfErr==SigSpec(&spec,1)) FSpCreate(&spec,CREATOR,'TEXT',smSystemScript);
#endif
	}
	
	// Convert old sig preference
	if (CountStrn(PREF2_STRN)<PREF_SIGFILE-100)
	{
		if (PrefIsSet(PREF_SIG)) SetPrefLong(PREF_SIGFILE,SIG_STANDARD);
		else SetPrefLong(PREF_SIGFILE,SIG_NONE);
	}
}

/************************************************************************
 * VersString - get the version number
 ************************************************************************/
PStr VersString(PStr versionString)
{
	Handle version;
	
	if (version = GetResource_(CREATOR,2))
	{
		PCopy(versionString,*version);
		ReleaseResource_(version);
	}
	else
		*versionString = 0;
		
#ifdef NEVER
	if (PETE)
	{
		Str31 editor;
		ComposeString(editor,"\p/Editor %d.%d",GetComponentVersion(PETE)>>16,GetComponentVersion(PETE)&0xffff);
		PCat(versionString,editor);
	}
#endif

	return(versionString);
}

/************************************************************************
 * SettingsInit - Initialize with a settings file
 ************************************************************************/
void SettingsInit(FSSpecPtr spec,Boolean system,Boolean changeUserState)
{
#ifdef NAG
	InitNagResultType initNagResult;
#endif
	Str255 scratch;
	uLong aLong;
	OSErr err;

	StartingUp = true;
		
	DateWarning(true);	// annoy the user about his timezone
	
	SubFolderSpec(0,nil);	// clear the subfolder cache
	
	MailRoot = Root;	// until we create the mail folder
	
	ZapHandle(CompactStack);
	StackInit(sizeof(FSSpec),&CompactStack);

	/*
	 * install other AE handlers
	 */
	InstallAERest();
	
	if (EjectBuckaroo) {Cleanup();ExitToShell();}
	
	ZapHandle(StringCache);
	OpenSettingsFile(spec); 		/* open our settings file */
	
	AuditStartup(kMyPlatformCode,MAJOR_VERSION*100+MINOR_VERSION*10+INC_VERSION,BUILD_VERSION);
	
	/*
	 * Check out our ... anal
	 */
	AnalFindMine();
	
	VicomFactor=GetRLong(VICOM_FACTOR);
	BgYieldInterval=GetRLong(BG_YIELD_INTERVAL);
	FgYieldInterval=GetRLong(FG_YIELD_INTERVAL);
	
	ASSERT(Count1Resources(TOC_TYPE)==0);
	

	InitPersonalities();
	ProxyInit();
	BuildSigList();
	OTInitOpenTransport();

	InitAppearanceMgr();	// Register Appearance and Live Scrolling
	
	PrefillSettings();

#ifdef NAG
	initNagResult = InitNagging ();
#endif

	SwapQDTextFlags(PrefIsSet(PREF_QD_TEXT_ROUTINES) ? kQDUseTrueTypeScalerGlyphs : kQDUseCGTextRendering|kQDUseCGTextMetrics);

	PETELiveScroll(PETE, PrefIsSet(PREF_LIVE_SCROLL));
	HesiodKill();
	FurrinSort = PrefIsSet(PREF_FURRIN_SORT);
	SyncRW = AutoDoubler || PrefIsSet(PREF_SYNC_IO);
	Offline = PrefIsSet(PREF_OFFLINE);
	UseCTB = PrefIsSet(PREF_TRANS_METHOD);
	CurTrans = GetTCPTrans();
	UnreadStyle = GetRLong(UNREAD_STYLE);
	NewTables = UsingNewTables();
	NoPreflight = PrefIsSet(PREF_NO_CHECK_MEM);
	LogLevel = GetPrefLong(PREF_LOG);	// start logging
	LocalizeFiles();
	ConvertSignature();
	CreateMailFolder();					/* make the mail folder and move stuff in it */
															// do this before creating other folders & crap
#ifdef	IMAP
	CreateIMAPMailFolder();				/* make the folder for the IMAP cache */
	RebuildAllIMAPMailboxTrees();		/* and build the personalities IMAP trees. */
#endif	
	CreateItemsFolder();				/* create folder for misc items */
	CreateSpoolfolder();				/* create folder for outgoing spools */
#ifdef BATCH_DELIVERY_ON
	CreateDeliveryfolder();
#endif
	CreatePartsfolder();				/* create folder for mhmtl parts */
	CreateCachefolder();				/* create folder for downloading html images */
	FigureOutFont(True);						/* figure out what font && size to use */
	CreateBoxes();							/* create In and Out boxes, for later use */
	GetAttFolderSpecLo(&AttFolderSpec);
	PutUpMenus(); 							/* menus */
	if (err=ConConInit()) DieWithError(COULDNT_INIT_CONCON,err);	// ContentConcentrator
#ifdef	IMAP
	// make sure all the personalities have cache folders
	CreateNewPersCaches();
	// initialize the string we'll pass back IMAP errors with.
	gIMAPErrorString[0] = 0;
#endif
	if (!OptiMEMIs && CurrentSize()+10 K<EstimatePartitionSize(false)) MemoryWarning();
	if (EjectBuckaroo) {Cleanup();ExitToShell();}
	TOCList = nil;							/* no open TOC's yet */
	MessList = nil; 						/* no open messages yet */
	SetSendQueue(); 						/* set SendQueue if there are queued messages */
	EnableMenus(nil,False); 					/* enable appropriate items */
	GenAliasList();							/* figure out where the alias lists are */
	WrapWrong = CountResources('Rong') > 0;
	Toshiba = PrefIsSet(PREF_TOSHIBA_FLUSH);
	FakeTabs = PrefIsSet(PREF_TAB_IN_TEXT);
	gNeedRemind = FindRemindLink();			/* see if the user needs to be reminded about any missed links */
	D3 = ThereIsColor && PrefIsSet(PREF_3D);
	if (!PrefIsSet(PREF_SAVE_PASSWORD)) SetPref(PREF_PASS_TEXT,"");
//	Since SCNetworkCheckReachabilityByName is a piece of crap in 10.3 and before
//	we make sure that we don't call it unless the user has explicitly enabled it.
	if ( HaveTheDiseaseCalledOSX ()) 
		if ( !*GetPref ( scratch, PREF_IGNORE_PPP ))
			SetPref ( PREF_IGNORE_PPP, YesStr );
	if (!*GetPref(scratch,PREF_AUTO_CHECK))
		SetPref(PREF_AUTO_CHECK,GetPrefLong(PREF_INTERVAL)?YesStr:NoStr);
	PersSetAutoCheck();
	PETEAllowIntelligentEdit(PETE,!PrefIsSet(PREF_DUMB_PASTE));
	PETEAnchoredSelection(PETE,PrefIsSet(PREF_ANCHORED_SELECTION));
	
	OutgoingMIDListLoad();

	GetRString(Re,REPLY_INTRO);
	GetRString(Fwd,FWD_PREFIX);
	GetRString(OFwd,OUTLOOK_FW_PREFIX);
	TrimWhite(Re);
	TrimWhite(Fwd);
	TrimWhite(OFwd);

	GetRString(NewLine,UseCTB ? CTB_NEWLINE : NEWLINE);	

	/*
	 * saved password?
	 */
	{
		PersHandle old = CurPers;
		for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next) {
			if(!PrefIsSet(PREF_SAVE_PASSWORD))
				InvalidateCurrentPasswords(False,False);
			else if(CurPers==PersList)
			{
				GetPref(scratch,PREF_PASS_TEXT);
				PCopy((*CurPers)->password,scratch);
				GetPref(scratch,PREF_AUXPW);
				PCopy((*CurPers)->secondPass,scratch);
			}
		}
		CurPers = old;
	}

#ifdef HAVE_KEYCHAIN
	if (KeychainAvailable() && !*GetPref(scratch,PREF_KEYCHAIN))
	{
		if (ComposeStdAlert(Normal,KEYCHAIN_WANNA)==1)
			KeychainConvert();
		else SetPref(PREF_KEYCHAIN,NoStr);
	}
#endif //HAVE_KEYCHAIN

	/*
	 * fix up some older settings
	 */
	ASSERT(resNotFound==Zap1Resource('STR ',973));
	Zap1Resource('STR ',973);
	if (!*GetPref(scratch,PREF_NEWMAIL_SOUND))
		SetPref(PREF_ERROR_SOUND,GetResName(scratch,'snd ',ATTENTION_SND));
	if (!*GetPref(scratch,PREF_NEWMAIL_SOUND))
		SetPref(PREF_NEWMAIL_SOUND,GetResName(scratch,'snd ',NEW_MAIL_SND));
	if (!PrefIsSet(PREF_BINHEX) && !PrefIsSet(PREF_DOUBLE) &&
			!PrefIsSet(PREF_SINGLE) && !PrefIsSet(PREF_UUENCODE))
{
		GetPref(scratch,PREF_ATT_TYPE);
		if (!*scratch) scratch[1] = '0';
		switch(scratch[1])
		{
			default: SetPref(PREF_DOUBLE,YesStr);break;
			case atmSingle: SetPref(PREF_SINGLE,YesStr);break;
			case atmUU: SetPref(PREF_UUENCODE,YesStr);break;
			case atmHQX: SetPref(PREF_BINHEX,YesStr);break;
		}
	}
	GetPref(scratch,PREF_NO_EZ_OPEN);
	if (!*scratch || scratch[1]=='n') SetPref(PREF_NO_EZ_OPEN,"\p2");
	else if (scratch[1]=='y') SetPref(PREF_NO_EZ_OPEN,"\p0");
	
	if (GetPref(scratch,PREF_NO_PREVIEW_READ)[1]=='y') SetPrefLong(PREF_NO_PREVIEW_READ,1);

	if (LogLevel & LOG_PLUG) LogPlugins();
	
	if (CountStrn(900)<PREF_FURRIN_SORT-100)
	{
		SetPref(PREF_TOOLBAR,YesStr);
		SetPref(PREF_TB_VARIATION,"\p1");
		SetPref(PREF_3D,YesStr);
		SetPref(PREF_FURRIN_SORT,NoStr);
	}
	
	if (!GetPrefLong(PREF_POP_MODE)) SetPrefLong(PREF_POP_MODE,popRUIDL);
	
	switch (GetPref(scratch,PREF_NO_LINES)[1])
	{
		case 'y': SetPrefLong(PREF_NO_LINES,3); break;
		case '\0': SetPrefLong(PREF_NO_LINES,2); break;
	}
	
	switch (GetPref(scratch,PREF_SEND_STYLE)[1])
	{
		case 'y': SetPrefLong(PREF_SEND_STYLE,1); break;
		case 'n': case '\0': SetPrefLong(PREF_SEND_STYLE,0); break;
	}
	
	switch (GetPref(scratch,PREF_NICK_GEN_OPTIONS)[1])
	{
		case 'y': SetPrefLong(PREF_NICK_GEN_OPTIONS,1); break;
		case 'n': case '\0': SetPrefLong(PREF_NICK_GEN_OPTIONS,0); break;
	}
		
	/*
	 * colors
	 */
	PeteSetupTextColors(nil,false);
	
	/*
	 * Is the attachment folder ok?
	 */
	CheckAttachFolder();
	
	/*
	 * fix old recip menus
	 */
	FixRecipMenu();
	
	/*
	 * fix arrow settings
	 */
	GetPref(scratch,PREF_SWITCH_MODIFIERS);
	if (scratch[1]=='y' || scratch[1]=='n' || !*scratch)
		SetPrefLong(PREF_SWITCH_MODIFIERS,cmdKey);
	
	/*
	 * fix connection hours weekend thing
	 */
	aLong = GetPrefLong(PREF_CHECK_HOURS);
	if (!(aLong&(kHourAlwaysWeekendMask|kHourNeverWeekendMask|kHourUseWeekendMask)))
		SetPrefLong(PREF_CHECK_HOURS,aLong|kHourAlwaysWeekendMask);

	/*
	 * install help for transfer menu
	 */
	TransferMenuHelp(TFER_HMNU);
	
#ifdef WINTERTREE
	// Spelling options
	if (HasFeature (featureSpellChecking))
		WinterTreeOptions = GetPrefLong(PREF_WINTERTREE_OPTS);
#endif
			
	/*
	 * fix stupid new user settings
	 */
	if (*GetPref(scratch,PREF_POP_SIGH) && !*GetPref(scratch,PREF_STUPID_USER))
	{
		for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next)
			SetStupidStuff();
	}

	CurPers = PersList;

	/*
	 * OS X AddressBook sync
	 */
	OSXAddressBookSync();

	InitWazoos();

#ifdef LDAP_ENABLED
	RefreshLDAPSettings();
#endif
	RefreshNicknameWatcherGlobals (false);

#ifdef	IMAP
	/*
	 * Set Offline if shift was pressed before now.  
	 */
	 if (NoInitialCheck) SetPref(PREF_OFFLINE,YesStr);
#endif

	// Initialize emoticons before we open any windows
	EmoInit();
	
	/*
	 * Upen the toolbar
	 */
	if (ToolbarShowing()) OpenToolbar();
	if (PrefIsSet(PREF_TOOLBAR)) OpenToolbar();

	/*
	 * start measuring face time
	 */
	FMBMain = NewFaceMeasure();
	FaceMeasureBegin(FMBMain);
	
	// start recording stats
	InitStats();
	
	/*
	 * Start showing ads
	 */
	OpenAdWindow();
	
	/*
	 * Bring Out Yer Dead!
	 */
	if (!EjectBuckaroo) RecallOpenWindows();
	
	/*
	 * run initial check
	 */
#ifndef	IMAP //Too late, we could be opening and resyncing IMAP windows already
	 if (NoInitialCheck) SetPref(PREF_OFFLINE,YesStr);
#endif
	if (!EjectBuckaroo && PersCheckTicks() && AutoCheckOK())
		CheckNow = True;

	/*
	 * filter messages that were left behind
	 */
#ifdef THREADING_ON		
	CleanRealOutTOC();
	CleanTempOutTOC();
	SetNeedToFilterOut();
	NeedToFilterIn = 1;	// make sure we check for delivery stuff
	if (!PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING))
		NeedToFilterIMAP = 1;
	gSkipIMAPBoxes = true;	// we can skip any IMAP mailboxes for this first filter pass.
	FilterXferMessages();
	gSkipIMAPBoxes = false;
#endif

	//
	// Swap command keys with manual filtering
	if (JunkPrefSwitchCmdJAsked())
	{
		if (JunkPrefSwitchCmdJ()) JunkReassignKeys(true);
	}
	else if (!HaveManualFilters() || kAlertStdAlertOKButton==ComposeStdAlert(Note,USE_CMD_J_FOR_JUNK))
	{
		JunkReassignKeys(true);
	}
	else
	{
		JunkReassignKeys(false);
	}
	SetPrefBit(PREF_JUNK_MAILBOX,bJunkPrefSwitchCmdJAsked);
	
	if (GetCurrentPayMode()==EMS_ModePaid)
		ClearPrefBit(PREF_JUNK_MAILBOX,bJunkPrefDeadPluginsWarning);

	/*
	 * send the audit record?
	 */
	if (!NoInitialCheck) RandomSendAudit();
	
#ifdef NAG
	if (initNagResult)
		DoPendingNagDialog (initNagResult);
#endif
	
	StartingUp = false;
}

#ifdef HAVE_KEYCHAIN
/**********************************************************************
 * KeychainConvert - setup personalities to do the keychain
 **********************************************************************/
void KeychainConvert(void)
{
	PushPers(PersList);
	for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next)
	{
		InvalidateCurrentPasswords(False,False);
		SetPref(PREF_SAVE_PASSWORD,YesStr);
	}
	SetPref(PREF_KEYCHAIN,YesStr);
	PopPers();
}
#endif //HAVE_KEYCHAIN

/**********************************************************************
 * CleanTempFolder - get rid of our stuff in the temp folder
 **********************************************************************/
void CleanTempFolder(void)
{
	short vRef;
	long dirId;
	CInfoPBRec hfi;
	Str63 name;
	
	FindFolder(Root.vRef,kTemporaryFolderType,True,&vRef,&dirId);
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate(vRef,dirId,&hfi))
		if (hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			OSErr	err;
			err = HDelete(vRef,dirId,name);
			if (!err)
				hfi.hFileInfo.ioFDirIndex--;	//	Don't skip next entry
		}
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr LocalizeFiles(void)
{
	FSSpec spec, tocSpec;
	short rootfiles[] = {
		USA_NICKNAME_FOLDER, NICK_FOLDER,
		USA_EUDORA_NICKNAMES, ALIAS_FILE,
		USA_CACHE_ALIAS_FILE, CACHE_ALIAS_FILE,
		USA_PERSONAL_ALIAS_FILE, PERSONAL_ALIAS_FILE,
		USA_STATISTICS_FILE, STATISTICS_FILE,
		USA_ATTACHMENTS_FOLDER, ATTACH_FOLDER,
		USA_SIG_FOLDER, SIG_FOLDER,
		USA_STATION_FOLDER, STATION_FOLDER,
		USA_SPOOL_FOLDER, SPOOL_FOLDER,
#ifdef BATCH_DELIVERY_ON
		USA_DELIVERY_FOLDER, DELIVERY_FOLDER,
#endif
		USA_MISPLACED_FOLDER, MISPLACED_FOLDER,
		0,0};
	short mailfiles[] = {
		USA_IN, IN,
		USA_OUT, OUT,
		USA_TRASH, TRASH,
		0,0};
	OSErr err = noErr;
	short *file;
	
	spec.vRefNum = Root.vRef;
	spec.parID = Root.dirId;
	for (file=rootfiles;*file;file+=2)
	{
		if (!EqualStrRes(GetRString(spec.name,file[1]),file[0]))
			LocalizeRename(&spec,file[0]);
	}
	spec.vRefNum = MailRoot.vRef;
	spec.parID = MailRoot.dirId;
	for (file=mailfiles;*file;file+=2)
	{
		if (!EqualStrRes(GetRString(spec.name,file[1]),file[0]))
		{
			LocalizeRename(&spec,file[0]);
			Box2TOCSpec(&spec,&tocSpec);	// usa .toc name in tocSpec
			GetRString(spec.name,file[1]);
			Box2TOCSpec(&spec,&spec);			// local .toc name in spec
			FSpRename(&tocSpec,&spec.name);		// will fail if local .toc exists; ok
		}
	}
	
	/*
	 * a few files in subfolders
	 */
	if (!(err=SubFolderSpec(NICK_FOLDER,&spec)))
	{
		GetRString(spec.name, PHOTO_FOLDER);
		LocalizeRename(&spec, USA_PHOTO_FOLDER);
#ifdef VCARD
		GetRString(spec.name, VCARD_FOLDER);
		LocalizeRename(&spec, USA_VCARD_FOLDER);
#endif
	}
	if (!(err=SubFolderSpec(STATION_FOLDER,&spec)))
	{
		GetRString(spec.name,DEFAULT);
		LocalizeRename(&spec,USA_DEFAULT);
	}
	if (!(err=SubFolderSpec(SIG_FOLDER,&spec)))
	{
		GetRString(spec.name,SIGNATURE);
		LocalizeRename(&spec,USA_SIGNATURE);
		GetRString(spec.name,ALT_SIG);
		LocalizeRename(&spec,USA_ALTERNATE);
	}
	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr LocalizeRename(FSSpecPtr spec,short id)
{
	Str63 newName;
	FSSpec origSpec = *spec;
	
	PCopy(newName,spec->name);
	GetRString(origSpec.name,id);
	return(FSpRename(&origSpec,&newName));
}

/**********************************************************************
 * CreateItemsFolder - make the Eudora Items folder
 **********************************************************************/
void CreateItemsFolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	CInfoPBRec hfi;
	short	pluginsVRefNum;
	long	pluginsDirID;
	
	//	Make Eudora Items folder
	FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,ITEMS_FOLDER),&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);	
	AFSpGetCatInfo(&spec,&spec,&hfi);
	IsAlias(&spec,&spec);
	ItemsFolder.dirId = SpecDirId(&spec);
	ItemsFolder.vRef = spec.vRefNum;

	//	Make Plugins folder for plugin preferences
	FSMakeFSSpec(ItemsFolder.vRef,ItemsFolder.dirId,GetRString(folder,PLUGINS_FOLDER),&spec);
	IsAlias(&spec,&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);
	pluginsVRefNum = spec.vRefNum;
	pluginsDirID = SpecDirId(&spec);
	
	//	Make plugin Filter and Nicknames folders inside plugin folder
	FSMakeFSSpec(pluginsVRefNum,pluginsDirID,GetRString(folder,PLUGIN_FILTERS),&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);
	FSMakeFSSpec(pluginsVRefNum,pluginsDirID,GetRString(folder,PLUGIN_NICKNAMES),&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);	
}

/**********************************************************************
 * CreateSpoolfolder - make the user's spool folder
 **********************************************************************/
void CreateSpoolfolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	
	
	FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,SPOOL_FOLDER),&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);
}

#ifdef BATCH_DELIVERY_ON
/**********************************************************************
 * CreateDeliveryfolder - make the user's delivery folder
 **********************************************************************/
void CreateDeliveryfolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	
	FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,DELIVERY_FOLDER),&spec);
	FSpDirCreate(&spec,smSystemScript,&junk);
}
#endif

/**********************************************************************
 * CreatePartsfolder - make the user's part folder
 **********************************************************************/
void CreatePartsfolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	OSErr err;
	Boolean isFolder, boolJunk;
	Boolean created;
	
	
	err=FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,PARTS_FOLDER),&spec);
	if (created = err==fnfErr) err = FSpDirCreate(&spec,smSystemScript,&junk);
	
	if (!err)
	{
		err = ResolveAliasFile(&spec,True,&isFolder,&boolJunk);
		if (!err && !folder) err = fnfErr;
	}
}

/**********************************************************************
 * CreateCachefolder - make the user's part folder
 **********************************************************************/
void CreateCachefolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	OSErr err;	
	
	err=FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,CACHE_FOLDER),&spec);
	if (err==fnfErr) FSpDirCreate(&spec,smSystemScript,&junk);
}

/**********************************************************************
 * CreateMailFolder - make the user's mail folder
 **********************************************************************/
void CreateMailFolder(void)
{
	FSSpec spec;
	Str63 folder;
	long junk;
	OSErr err;
	Boolean isFolder, boolJunk;
	Boolean created;
	
	
	err=FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(folder,MAIL_FOLDER_NAME),&spec);
	if (created = err==fnfErr) err = FSpDirCreate(&spec,smSystemScript,&junk);
	
	if (!err)
	{
		err = ResolveAliasFile(&spec,True,&isFolder,&boolJunk);
		if (!err && !folder) err = fnfErr;
	}
	if (err) DieWithError(MAIL_FOLDER,err);
	
	MailRoot.vRef = spec.vRefNum;
	MailRoot.dirId = SpecDirId(&spec);

	if (created) MoveMailboxesToMailFolder();
}

/************************************************************************
 * MoveMailboxesToMailFolder - move things to the mail folder
 ************************************************************************/
void MoveMailboxesToMailFolder(void)
{
#ifdef BATCH_DELIVERY_ON
	short badIndices[] = {ALIAS_FILE,CACHE_ALIAS_FILE,PERSONAL_ALIAS_FILE,LOG_NAME,OLD_LOG,FILTERS_NAME,SIG_FOLDER,NICK_FOLDER,PHOTO_FOLDER,ATTACH_FOLDER,STATION_FOLDER,SPOOL_FOLDER,ITEMS_FOLDER,MAIL_FOLDER_NAME,IMAP_MAIL_FOLDER_NAME,CACHE_FOLDER,PARTS_FOLDER,LINK_HISTORY_FOLDER,AUDIT_FILE,DELIVERY_FOLDER};
#else
	short badIndices[] = {ALIAS_FILE,CACHE_ALIAS_FILE,PERSONAL_ALIAS_FILE,LOG_NAME,OLD_LOG,FILTERS_NAME,SIG_FOLDER,NICK_FOLDER,PHOTO_FOLDER,ATTACH_FOLDER,STATION_FOLDER,SPOOL_FOLDER,ITEMS_FOLDER.MAIL_FOLDER_NAME,IMAP_MAIL_FOLDER_NAME,CACHE_FOLDER,PARTS_FOLDER,LINK_HISTORY_FOLDER,AUDIT_FILE};
#endif
	Str31 badNames[sizeof(badIndices)/sizeof(short)];
	short i,bad;
	CInfoPBRec hfi;
	Str31 name;
	OSErr err;
	FSSpec spec;
	
	// load up the list of names we don't like
	for (i=0;i<sizeof(badIndices)/sizeof(short);i++)
	  GetRString(badNames[i],badIndices[i]);
	  
	// Now, go through the root folder, moving as we go
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	for (hfi.hFileInfo.ioFDirIndex=i=0;
			 !DirIterate(Root.vRef,Root.dirId,&hfi);
			 hfi.hFileInfo.ioFDirIndex=++i)
	{
		SimpleMakeFSSpec(Root.vRef,Root.dirId,name,&spec);
		if (HFIIsFolderOrAlias(&hfi) || hfi.hFileInfo.ioFlFndrInfo.fdType==MAILBOX_TYPE || hfi.hFileInfo.ioFlFndrInfo.fdType==TOC_TYPE)
		{
			for (bad=0;bad<sizeof(badIndices)/sizeof(short);bad++)
				if (StringSame(badNames[bad],name)) break;
			if (bad==sizeof(badIndices)/sizeof(short))
			{
				// move it!
				if (err = HMove(Root.vRef,Root.dirId,name,MailRoot.dirId,nil))
				{
					FileSystemError(MOVING_MAILBOXES,name,err);
					if (CommandPeriod) break;
				}
				else i--;
			}
		}
	}
}


/**********************************************************************
 * LogPlugins - log the resources in open files
 **********************************************************************/
OSErr LogPlugins(void)
{
	short refN;
	FSSpec spec;
	Str31 volName;
	Handle r;
	OSType type;
	Str255 rName;
	short id;
	short typeCount;
	short resCount;

	for (GetTopResourceFile(&refN); refN!=AppResFile; GetNextResourceFile(refN,&refN))
	{
		if (GetFileByRef(refN,&spec)) break;
		GetDirName(nil,spec.vRefNum,spec.parID,volName);
		ComposeLogS(LOG_PLUG,nil,"\p%p:%p",volName,spec.name);
		
		UseResFile(refN);
		for (typeCount = Count1Types(); typeCount; typeCount--)
		{
			Get1IndType(&type,typeCount);
			for (resCount = Count1Resources(type); resCount; resCount--)
			{
				SetResLoad(False);
				r = Get1IndResource(type,resCount);
				SetResLoad(True);
				if (r)
				{
					GetResInfo(r,&id,&type,rName);
					UseResFile(SettingsRefN);
					ComposeLogS(LOG_PLUG,nil,"\p  %O %d %p",type,id,rName);
					UseResFile(refN);
				}
			}
		}
	}
	UseResFile(SettingsRefN);
	return noErr;
}

/**********************************************************************
 * CheckAttachFolder - check to see if the user's real attachment folder exists
 **********************************************************************/
void CheckAttachFolder(void)
{
	Str127 pref, volname;
	long dirID;
	
	GetPref(pref,PREF_AUTOHEX_VOLPATH);
	if (*pref && !GetVolpath(pref,volname,&dirID))
	{
		switch (ComposeStdAlert(kAlertNoteAlert,NO_ATTACH_FOLDER))
		{
			case kAlertStdAlertOKButton:
				SetPref(PREF_AUTOHEX_VOLPATH,"");
				SetPref(PREF_AUTOHEX_NAME,"");
				break;
			case kAlertStdAlertOtherButton:
				if (GetHexPath(volname,pref))
				{
					SetPref(PREF_AUTOHEX_NAME,volname);
					SetPref(PREF_AUTOHEX_VOLPATH,pref);
				}
				break;
			default:;	// leave it for next time
		}
	}
}

/************************************************************************
 * GenAliasList - find the alias list
 ************************************************************************/
void GenAliasList(void)
{
	Str31 name;
	FSSpec folderSpec;
	AliasDesc ad;
	
	/*
	 * clear out the old, allocate empty handle for new
	 */
	if (Aliases) ZapHandle(Aliases);
	if (!(Aliases=NuHandle(0L))) DieWithError(MEM_ERR,MemError());

#ifdef VCARD
	/*
	 * add the Personal Nicknames file (we do this whether or not the feature is turned on)
	 */
	if (HasFeature (featureVCard)) {
		Zero(ad);
		ad.type = personalAddressBook;
		FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(name,PERSONAL_ALIAS_FILE),&ad.spec);
		if (PtrPlusHand_(&ad,Aliases,sizeof(ad))) DieWithError(MEM_ERR,MemError());
	}
#endif
	
	/*
	 * add the Eudora Nicknames file
	 */
	Zero(ad);
	ad.type = eudoraAddressBook;
	FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(name,ALIAS_FILE),&ad.spec);
	if (PtrPlusHand_(&ad,Aliases,sizeof(ad))) DieWithError(MEM_ERR,MemError());
	
#ifdef TWO
	/*
	 * is there a nicknames folder?
	 */
	if (HasFeature (featureMultipleNicknameFiles) && !SubFolderSpec(NICK_FOLDER,&folderSpec))
		ReadNickFileList (&folderSpec, regularAddressBook, false);

	/*
	 * add the Eudora Nicknames Cache file
	 */
	if (HasFeature (featureNicknameWatching)) {
		Zero(ad);
		ad.type = historyAddressBook;
		FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(name,CACHE_ALIAS_FILE),&ad.spec);
		if (PtrPlusHand_(&ad,Aliases,sizeof(ad))) DieWithError(MEM_ERR,MemError());
	}
	ReadPluginNickFiles(false);
#endif
}

/************************************************************************
 * UsingNewTables - are we using the new translit table scheme?
 ************************************************************************/
Boolean UsingNewTables(void)
{
	Boolean found=False;
	Handle table;
	short ind=0;
	Str255 name;
	short id;
	ResType type;
	
	SetResLoad(False);
	while (table=GetIndResource_('taBL',++ind))
	{
		GetResInfo(table,&id,&type,&name);
		if (found=(id<1001||id>1003)&&*name) break;
	}
	SetResLoad(True);
	return(found);
}

/************************************************************************
 * OpenNewSettings - open a new settings file, closing things out
 * properly first
 *
 * (10/4/99 jp) Modified a bit for adware to save the settings FSSpec so
 *							we can pass it back in on a relaunch.
 ************************************************************************/
void OpenNewSettings(FSSpecPtr spec, Boolean changeUserState, UserStateType newState)
{
	short menu;
	MenuHandle mHandle;
	
	if (!OkSettingsFileSpot(spec))
	{
		if (SettingsRefN) return;
		Cleanup();
		ExitToShell();
	}
	
	if (SettingsRefN)
	{
		/*
		 * a simulated Quit
		 */
		DoQuit(changeUserState);
		if (changeUserState)
			(*nagState)->state = newState;
		if (!AmQuitting) return;
		Done = False;
		
		/*
		 * let's get rid of some stuff
		 */
		KillWazoos();
		RemoveBoxMenus();
#ifdef TWO
		NukeXfUndo();
#endif
		CloseAdWindow();

		if (mHandle=GetMHandle(LABEL_HIER_MENU)) TrashMenu(mHandle,3);	// kill it to get rid of icon suites
		ZapResMenu(COLOR_HIER_MENU);
		ZapResMenu(SIG_HIER_MENU);
		ZapResMenu(SORT_HIER_MENU);
		ZapResMenu(FONT_HIER_MENU);
		ZapResMenu(MARGIN_HIER_MENU);
		ZapResMenu(TEXT_HIER_MENU);
		if (HasFeature (featureMultiplePersonalities))
			ZapResMenu(PERS_HIER_MENU);
		ZapResMenu(TEXT_SIZE_HIER_MENU);
		ZapResMenu(SERVER_HIER_MENU);
#ifdef WINTERTREE
		if (HasFeature (featureSpellChecking))
			ZapResMenu(SPELL_HIER_MENU);
#endif
		
		for (menu=APPLE_MENU;menu<MENU_LIMIT;menu++)
		{
			if (mHandle=GetMHandle(menu)) ReleaseResource_(mHandle);
		}
		if (!HMGetHelpMenuHandle(&mHandle) && mHandle) /* the check for nil is needed courtesy of Norton Utilities, crap that it is */
			if (HaveTheDiseaseCalledOSX())
			{
				// destroy the entire menu
				HMGetHelpMenuHandle(nil);
				ReleaseResource_(mHandle);
			}
			else
				// kill our items only
				TrashMenu(mHandle,OriginalHelpCount+1);
		ClearMenuBar();
		ZapAliases();
		ZapHandle(Aliases);
		ZapHistoriesList(true);
		ZapFilters();
		ZapHandle(Filters);
		ConConDispose();
#ifdef	IMAP
	 	DisposePersIMAPMailboxTrees();
#endif
		DisposePersonalities();
		ProxyZap();
	}
	AmQuitting = False;
	//if (ToolbarShowing()) OpenToolbar();

	/*
	 * Now, point us at the right directory
	 */
	
	(void) IsAlias(spec,spec);
	Root.vRef = spec->vRefNum;
	Root.dirId = spec->parID;
	SettingsFileSpec = *spec;

	/*
	 * keep yer fingers crossed...
	 */
	SettingsInit(spec,False,changeUserState);
}

/************************************************************************
 * OkSettingsFileSpot - does this look like a good spot for Eudora Settings?
 ************************************************************************/
Boolean OkSettingsFileSpot(FSSpecPtr spec)
{
	Str31 name;
	Str31 otherName;
	short vRef;
	long dirID;
	short i;
	static OSType baaaadPlaces[] = {kAppleMenuFolderType,kControlPanelFolderType,kDesktopFolderType,
		kExtensionFolderType,kFontsFolderType,kPreferencesFolderType,kPrintMonitorDocsFolderType,
		kStartupFolderType,kSystemFolderType,kTemporaryFolderType,kTrashFolderType,kWhereToEmptyTrashFolderType};
	FSSpec otherSpec;
	
	// we may need the name
	GetDirName(nil,spec->vRefNum,spec->parID,name);
	
	// root volumes no good
	if (spec->parID==2)
	{
		ComposeStdAlert(Stop,NO_EF_DESKTOP,name);
		return(False);
	}
	
	// special folders no good
	for (i=0;i<sizeof(baaaadPlaces)/sizeof(OSType);i++)
		if (!FindFolder(spec->vRefNum,baaaadPlaces[i],false,&vRef,&dirID))
			if (spec->vRefNum==vRef && spec->parID==dirID)
			{
				ComposeStdAlert(Stop,NO_EF_DESKTOP,name);
				return(False);
			}
	
	if (!FSMakeFSSpec(spec->vRefNum,spec->parID,GetRString(otherName,MAIL_FOLDER_NAME),&otherSpec)) return (True);	// we like it!
	if (FSMakeFSSpec(spec->vRefNum,spec->parID,GetRString(otherName,IN),&otherSpec) ||
			FSMakeFSSpec(spec->vRefNum,spec->parID,GetRString(otherName,OUT),&otherSpec))
		return(ComposeStdAlert(Caution,EF_WARNING,name,name)==kAlertStdAlertOKButton);
	
	return(True);
}

/**********************************************************************
 * 
 **********************************************************************/
void ZapResMenu(short id)
{
	MenuHandle mHandle;
	
	if (mHandle=GetMHandle(id))
	{
		DeleteMenu(id);
		ReleaseResource_(mHandle);
	}
}

/************************************************************************
 * SetRunType - figure out from the name of the app whether we're prod-
 * uction, testing, or my private version.	This is a HACK.
 ************************************************************************/
void SetRunType(void)
{
#ifdef DEBUG
	FSSpec appSpec;
#endif
	
	AppResFile = HomeResFile(GetIndResource(CREATOR, 1));

#ifdef DEBUG
	GetFileByRef(AppResFile,&appSpec);
	switch (appSpec.name[1])
	{
		case 'P': RunType=Steve;break;
		case 'e': RunType=Debugging;break;
		case 'l': RunType=Debugging;break;
		default: RunType=Production;break;
	}
#else
	RunType=Production;
#endif DEBUG
}

/**********************************************************************
 * do what we need before exiting functions
 **********************************************************************/
void Cleanup()
{
#ifdef ETL
	ETLCleanup();
#endif
#ifdef THREADING_ON
	KillThreads ();
#endif
#ifdef	IMAP	
	if (KerberosWasUsed() && PrefIsSet(PREF_DIE_DOG)) KerbDestroy();
	if (KerberosWasUsed() && PrefIsSet(PREF_DIE_DOG_USER)) KerbDestroyUser();
#else
	if (PrefIsSet(PREF_KERBEROS) && PrefIsSet(PREF_DIE_DOG)) KerbDestroy();
	if (PrefIsSet(PREF_KERBEROS) && PrefIsSet(PREF_DIE_DOG_USER)) KerbDestroyUser();
#endif
#ifdef MENUSTATS
	NoteMenuSelection(-1);
#endif
#ifndef SLOW_CLOSE
	TcpFastFlush(True);
#endif
	OTCleanUpAfterOpenTransport();
	SaveFeaturesWithExtemeProfanity (gFeatureList);
	if (SettingsRefN) {short refN=SettingsRefN; SettingsRefN=0; CloseResFile(refN);}	// fool closeresfile, because it's ok to close settings here
	CloseLog();
	FlushVol(nil,Root.vRef);
	FlushVol(nil,MailRoot.vRef);
	if (PETE) PETECleanup(PETE);
	if (TBTurnedOnBalloons) {TBTurnedOnBalloons = False; HMSetBalloons(False);}
	CleanupUnicode();
	OSXAddressBookSyncCleanup(false);
}


/************************************************************************/
/*   d o   i n i t   l d e f - initialize LDEF resource.			 **/
/************************************************************************/
void doInitDEF(short resId,UniversalProcPtr upp,OSType type)
{
	#pragma options align=mac68k
	struct AbsJMP {
		/* See the LDEF 1001 resource (stub) in Muslin.r for an explanation */
		long	LEA;
		short	MOVEA;
		short	JMP;
		Ptr		theAddr;
	};
	#pragma options align=reset
	typedef struct AbsJMP AbsJMP;

	Handle				theResource;
	AbsJMP				*theJMP;

	/* Lifted from some DTS code.
		 The LDEF resource is a stub into which we write
	   the address of the "real" LDEF (in the application.) The toolbox calls this 
	   stub, which in turn jumps to our LDEF.
	   
	   This technique not only allows us to debug the LDEF more easily (and give it
	   access to the program's globals under 68K), it also simplifies working with
	   Mixed Mode (see below) */

	theResource = GetResource(type,resId);
	theJMP = (AbsJMP*)(*theResource);
	theJMP->theAddr = (Ptr)upp;
} /* end of doInitLDEF */

/************************************************************************/
/* InitCDEF - setup a CDEF
/************************************************************************/
void InitCDEF(short resId,ControlDefProcPtr proc)
{
	//	Register this resId so we will automatically use our CDEF
	ControlDefSpec	cdefSpec;
	static ControlCNTLToCollectionUPP  conversionProc;

	cdefSpec.defType = kControlDefProcPtr;
	cdefSpec.u.defProc = NewControlDefUPP(proc);
	if (!conversionProc) conversionProc = NewControlCNTLToCollectionUPP(ControlCNTLToCollection);
	RegisterControlDefinition(resId,&cdefSpec,conversionProc);
}

/************************************************************************/
/* ControlCNTLToCollection - put control values into collection for CDEF
/************************************************************************/
pascal OSStatus ControlCNTLToCollection (Rect *bounds, SInt16 value, Boolean visible, SInt16 max, SInt16 min, SInt16 procID, SInt32 refCon, ConstStr255Param title, Collection collection)
{
	OSErr err;

	if (!(err = AddCollectionItem(collection,kControlCollectionTagBounds,0,sizeof(Rect),bounds)))
	if (!(err = AddCollectionItem(collection,kControlCollectionTagValue,0, sizeof (value), &value)))
	if (!(err = AddCollectionItem(collection,kControlCollectionTagMinimum,0,sizeof (min), &min)))
	if (!(err = AddCollectionItem(collection,kControlCollectionTagMaximum,0, sizeof (max), &max)))
	if (!(err = AddCollectionItem(collection,kControlCollectionTagVisibility,0,sizeof(visible),&visible)))
	if (!(err = AddCollectionItem(collection,kControlCollectionTagRefCon,0, sizeof (refCon), &refCon)))
		err = AddCollectionItem(collection,kControlCollectionTagTitle,0,title[0],title+1);
	return err;
}

/************************************************************************/
/* InitLDEF - setup an LDEF
/************************************************************************/
void InitLDEF(short resId,ListDefProcPtr proc)
{
	//	Register this resId so we will automatically use our LDEF
	if (RegisterListDefinition)
	{
		ListDefSpec	ldefSpec;
		ldefSpec.defType = kListDefUserProcType;
		ldefSpec.u.userProc = NewListDefUPP(proc);
		RegisterListDefinition(resId,&ldefSpec);
	}
}

/**********************************************************************
 * figure out what font and size to use
 **********************************************************************/
void FigureOutFont(Boolean peteToo)
{
	Str255 fontName;
	PETEDefaultFontEntry defaultFont;
	int i;
	
	/*
	 * which font?
	 */
	if (EqualStrRes(GetPref(fontName,PREF_FONT_NAME),APPL_FONT))
		FontID = 1;
	else
		FontID = GetFontID(fontName);
		
	/*
	 * which fixed font?
	 */
	GetRString(fontName,FIXED_FONT);
	if((FixedID = GetFontID(fontName)) == applFont)
		FixedID = HiWord(ScriptVar(smScriptMonoFondSize));
		
	/*
	 * how big?
	 */
	FontSize = GetPrefLong(PREF_FONT_SIZE);
	if (!FontSize) FontSize = LoWord(ScriptVar(smScriptAppFondSize));

	FixedSize = GetRLong(DEF_FIXED_SIZE);
	if (!FixedSize) FixedSize = LoWord(ScriptVar(smScriptMonoFondSize));
	
	normFonts[0].fontID = FontID;
	normFonts[0].fontSize = FontSize;
	printNormFonts[0].fontID = GetFontID(GetPref(fontName,PREF_PRINT_FONT));
	printNormFonts[0].fontSize = GetPrefLong(PREF_PRINT_FONT_SIZE);
	if (!printNormFonts[0].fontID) printNormFonts[0].fontID = FontID;
	if (!printNormFonts[0].fontSize) printNormFonts[0].fontSize = FontSize;
	printMonoFonts[0].fontID = monoFonts[0].fontID = FixedID;
	monoFonts[0].fontSize = FixedSize;
	printMonoFonts[0].fontSize = GetPrefLong(PREF_PRINT_FONT_SIZE);
	if (!printMonoFonts[0].fontSize) printNormFonts[0].fontSize = FixedSize;
	
	/* Get from the international font settings; will override above if available */
	for(i = 32; --i >= 0; )
	{
		short normID = 0, printNormID = 0, monoID = 0, printMonoID = 0;
		long normSize = 0, printNormSize = 0, monoSize = 0, printMonoSize = 0;
		
		GetRString(fontName, IntlFontsStrn+i+1);
		if(fontName[0])
		{
			unsigned char delims[3] = {',',':',0};
			UPtr spotP = &fontName[1];
			
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) StringToNum(GlobalTemp, &normSize);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) GetFNum(GlobalTemp, &normID);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) StringToNum(GlobalTemp, &monoSize);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) GetFNum(GlobalTemp, &monoID);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) StringToNum(GlobalTemp, &printNormSize);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) GetFNum(GlobalTemp, &printNormID);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) StringToNum(GlobalTemp, &printMonoSize);
			PToken(fontName, GlobalTemp, &spotP, delims);
			if(*GlobalTemp) GetFNum(GlobalTemp, &printMonoID);
		}

		if(i != 0)
		{
			long normVal, monoVal;

			normVal = GetScriptVariable(i, smScriptAppFondSize);
			monoVal = GetScriptVariable(i, smScriptMonoFondSize);
			normFonts[i].fontSize = normSize ? normSize : LoWord(normVal);
			normFonts[i].fontID = normID ? normID : HiWord(normVal);
			monoFonts[i].fontSize = monoSize ? monoSize : LoWord(monoVal);
			monoFonts[i].fontID = monoID ? monoID : HiWord(monoVal);
			printNormFonts[i].fontSize = printNormSize ? printNormSize : normFonts[i].fontSize;
			printNormFonts[i].fontID = printNormID ? printNormID : normFonts[i].fontID;
			printMonoFonts[i].fontSize = printMonoSize ? printMonoSize : monoFonts[i].fontSize;
			printMonoFonts[i].fontID = printMonoID ? printMonoID : monoFonts[i].fontID;
		}
		else
		{
			if(normSize) FontSize = normFonts[i].fontSize = normSize;
			if(normID) FontID = normFonts[i].fontID = normID;
			if(monoSize) FixedSize = monoFonts[i].fontSize = monoSize;
			if(monoID) FixedID = monoFonts[i].fontID = monoID;
			if(printNormSize) printNormFonts[i].fontSize = printNormSize;
			if(printNormID) printNormFonts[i].fontID = printNormID;
			if(printMonoSize) printMonoFonts[i].fontSize = printMonoSize;
			if(printMonoID) printMonoFonts[i].fontID = printMonoID;
		}
	}
	
	/*
	 * create a spare port for necessities
	 */
	SetupInsurancePort();
	
	/*
	 * and how big is big?
	 */
	FontWidth = GetWidth(FontID,FontSize);
	FontLead = GetLeading(FontID,FontSize);
	FontDescent = GetDescent(FontID,FontSize);
	FontAscent = GetAscent(FontID,FontSize);
	FontIsFixed = IsFixed(FontID,FontSize);
	
	/*
	 * and get the widths of boxes
	 */
	GetBoxLines();
	
	/*
	 * notify pete
	 */
	if (peteToo)
	{
		// save these, because utf8 stuff uses them
		short saveNormFont = normFonts[0].fontID;
		short saveNormFontSize = normFonts[0].fontSize;
		
		// handle the new prefs for text region
		// font and size
		if (*GetPref(fontName,PREF_TEXT_FONT))
		{
			short id=0;
			GetFNum(fontName,&id);
			if (id) normFonts[0].fontID = id;
		}
		
		{
			short size = GetPrefLong(PREF_TEXT_SIZE);
			if (size) normFonts[0].fontSize = size;
		}
		
		for(i = 32; --i >= 0; )
		{
			Zero(defaultFont);
			defaultFont.pdFont = normFonts[i].fontID;
			defaultFont.pdSize = normFonts[i].fontSize;
			PETESetDefaultFont(PETE,nil,&defaultFont);
			
			Zero(defaultFont);
			defaultFont.pdFont = printNormFonts[i].fontID;
			defaultFont.pdSize = printNormFonts[i].fontSize;
			defaultFont.pdPrint = True;
			PETESetDefaultFont(PETE,nil,&defaultFont);
			
#ifdef USEFIXEDDEFAULTFONT
			Zero(defaultFont);
			defaultFont.pdFont = monoFonts[i].fontID;
			defaultFont.pdSize = monoFonts[i].fontSize;
			defaultFont.pdFixed = True;
			PETESetDefaultFont(PETE,nil,&defaultFont);
			
			Zero(defaultFont);
			defaultFont.pdFont = printMonoFonts[i].fontID;
			defaultFont.pdSize = printMonoFonts[i].fontSize;
			defaultFont.pdFixed = True;
			defaultFont.pdPrint = True;
			PETESetDefaultFont(PETE,nil,&defaultFont);
#endif
		}

		// restore these, because utf8 stuff uses them
		normFonts[0].fontID = saveNormFont;
		normFonts[0].fontSize = saveNormFontSize;
	}
		
	ReloadStdSizes();
}

/**********************************************************************
 * open (or create) the settings file
 **********************************************************************/
void OpenSettingsFile(FSSpecPtr spec)
{
	Str255 settingsName;
	Handle appVersion;
	Handle prefVersion;
	int err;
	FInfo info;
	FSSpec appSpec;
	static short copyMe[] = {PREF2_STRN,PREF3_STRN,PREF4_STRN,RegValStrn,RecentDirServStrn,0};
	short *copyPtr;
	Boolean	newSettingsFile;
	enum	{ kAppVersionNum = MAJOR_VERSION*10000+MINOR_VERSION*100+INC_VERSION };
	FSSpec	theSpec;
	
	NoMenus = true;
	
	/*
	 * close it if it's open
	 */
	if (SettingsRefN)
	{
		// CloseResFile won't close the settings file without anger; fool it
		short refN = SettingsRefN;
		SettingsRefN = 0;
		CloseResFile(refN);
		Zero(SettingsSpec);
	}
	
	/*
	 * and any plug-ins in its directory
	 */
	if (PrefPlugEnd) while (CurResFile()!=PrefPlugEnd) CloseResFile(CurResFile());
	
	/*
	 * we'll create it before we open it, to avoid the PMSP
	 */
	if (spec && spec->name && !FSpGetFInfo(spec,&info) && 
			info.fdType==SETTINGS_TYPE && info.fdCreator==CREATOR)
		PCopy(settingsName,spec->name);
	else
	{
		if (!spec) spec = &theSpec;
		GetRString(settingsName,SETTINGS_FILE);
		PSCopy(spec->name,settingsName);
		spec->vRefNum = Root.vRef;
		spec->parID = Root.dirId;
		LocalizeRename(spec,USA_SETTINGS);
	}
	FSpCreateResFile(spec,CREATOR,SETTINGS_TYPE,smSystemScript);
	
	/*
	 * is it sane?
	 */
	SaneSettings(spec);
	SimpleMakeFSSpec (Root.vRef, Root.dirId, settingsName, &SettingsFileSpec);
	
	/*
	 * grab some stuff from the application resource file first
	 */
	if ((appVersion = GetResource_(CREATOR,1))==nil)
		DieWithError(READ_SETTINGS,ResError());
	HNoPurge(appVersion);
	
	/*
	 * if there are plug-ins in the Eudora Folder, open them, too
	 */
	GetFileByRef(AppResFile,&appSpec);
	if (appSpec.vRefNum!=Root.vRef || appSpec.parID!=Root.dirId)
		(void) PlugInDir(Root.vRef,Root.dirId);

	
	/*
	 * now, open the preferences file
	 */
	if ((SettingsRefN=FSpOpenResFile(spec,fsRdWrPerm))==-1)
	{
		err = ResError();
		DieWithError(err==opWrErr ? SETTINGS_BUSY : OPEN_SETTINGS,err);
	}
	
	// keep track of this little puppy
	SettingsSpec = *spec;
		
	/*
	 * copy the preferences into it if need be
	 * "need be" means either the resources aren't now there,
	 * or the program version has changed since last time the
	 * preferences file was written.
	 */
	if ((prefVersion = GetResource_(CREATOR,1))==nil)
		DieWithError(READ_SETTINGS,ResError());
	HNoPurge(prefVersion);
				
	newSettingsFile = HomeResFile(prefVersion)!=SettingsRefN;	//	No version resource. Most likely a new settings file
	if (newSettingsFile ||
			!StringSame(*prefVersion,*appVersion))
	{
		ReleaseResource_(prefVersion);
		ReleaseResource_(appVersion);
		if (err=ResourceCpy(SettingsRefN,AppResFile,CREATOR,1))
			DieWithError(WRITE_SETTINGS,err);
		if (err=ResourceCpy(SettingsRefN,AppResFile,'STR#',PREF_STRN))
			DieWithError(WRITE_SETTINGS,err);
		MyUpdateResFile(SettingsRefN);
		if (ResError()) DieWithError(WRITE_SETTINGS,ResError());
	}
	else
	{
		ReleaseResource_(prefVersion);
		ReleaseResource_(appVersion);
	}
	
	for (copyPtr=copyMe;*copyPtr;copyPtr++)
		if (!(prefVersion=Get1Resource('STR#',*copyPtr)) || 
				HomeResFile(prefVersion)!=SettingsRefN)
		{
			if (err=ResourceCpy(SettingsRefN,AppResFile,'STR#',*copyPtr))
				DieWithError(WRITE_SETTINGS,err);
		}
	
	// previous versions copied this in, we don't really want it
	if (prefVersion=Get1Resource('STR#',CmdKeyStrn))
	if (GetHandleSize(prefVersion)==2)
		Zap1Resource('STR#',CmdKeyStrn);
	
	//	Copy over default wazoos for new settings file or if
	//	settings file has never been opened with version 4.0.0 or higher
	if (newSettingsFile || GetPrefLong(PREF_HIGHEST_APP_VERSION) < 4*10000)
	{
		if (!Count1Resources(kWazooListResType))
		{
			ResourceCpy(SettingsRefN,AppResFile,kWazooListResType,kWazooRes1);
			ResourceCpy(SettingsRefN,AppResFile,kWazooListResType,kWazooRes2);
		}

		/* set default prefs for nickname type-ahead and nickname hiliting */
		SetDefaultNicknameWatcherPrefs();
	}
	
	//	Copy over default LIGHT wazoos for new settings file or if
	//	settings file has never been opened with version 4.3.0 or higher
	if (newSettingsFile || GetPrefLong(PREF_HIGHEST_APP_VERSION) < 403*100)
	{
		if (!Count1Resources(kFreeWazooListResType))
		{
			ResourceCpy(SettingsRefN,AppResFile,kFreeWazooListResType,kWazooRes1);
			ResourceCpy(SettingsRefN,AppResFile,kFreeWazooListResType,kWazooRes2);
		}
	}
	
		for (copyPtr=copyMe;*copyPtr;copyPtr++)
			if (RecountStrn(*copyPtr)) Aprintf(OK_ALRT,Caution,FIXED_STRN,*copyPtr);
	
	// For new settings files, do not give the preexisting junk mailbox warning
	if (newSettingsFile) SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefBoxExistWarning|bJunkPrefNeedIntro);

	//	Update application version preferences
	SetPrefLong(PREF_LAST_APP_VERSION,kAppVersionNum);
	if ((gHighestAppVersionAtLaunch = GetPrefLong(PREF_HIGHEST_APP_VERSION)) < kAppVersionNum)
		SetPrefLong(PREF_HIGHEST_APP_VERSION,kAppVersionNum);
}

/**********************************************************************
 * SaneSettings - make sure a settings file is sane, and then backup
 **********************************************************************/
void SaneSettings(FSSpecPtr spec)
{
	Boolean sane;
	FSSpec bkup;
	OSErr err;
	
	if ((err=utl_RFSanity(spec,&sane)) || !sane)
		if (!FallbackSettings(spec))
			DieWithError(SETTINGS_BAD,err?err:1);
	
	/*
	 * is sane.  make backup
	 */
	bkup = *spec;
	if (*bkup.name > 26) *bkup.name = 26;
	PSCat(bkup.name,"\p.bkup");
	FSpDupFile(&bkup,spec,True,False);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean FallbackSettings(FSSpecPtr spec)
{
	FSSpec backup;
	Boolean sane;
	OSErr err;
	
	backup = *spec;
	if (*backup.name > 26) *backup.name = 26;
	PSCat(backup.name,"\p.bkup");
	if (!FSpExists(&backup) && !utl_RFSanity(&backup,&sane) && sane)
	{
		if (ReallyDoAnAlert(USE_BACKUP_ALRT,Note)==1)
		{
			err = FSpExchangeFiles(spec,&backup);
			if (!err) PSCat(spec->name,"\p.bad");
			if (!err) FSpDelete(spec);
			if (!err) err = FSpRename(&backup,spec->name);
			if (err) FileSystemError(READ_SETTINGS,spec->name,err);
			else return(True);
		}
	}
	return(False);
}

/**********************************************************************
 * SystemEudoraFolder - find the directory for us in the system folder.
 * If it doesn't exist, create it.
 *
 * Actually, we now do this:
 * - Look for the folder in the system folder
 * - Look for the folder in the documents folder
 * - Create it in the documents folder
 * - With OSX will also look in the classic folders
 **********************************************************************/
void SystemEudoraFolder(void)
{
	CInfoPBRec hfi;
	int err=0;
	FSSpec spec;
	Str31 folder,scratch;
	Boolean system = True;

	/*
	 * now, look for our directory
	 */
#ifdef DEBUG
	GetRString(folder,RunType==Steve ? STEVE_FOLDER : FOLDER_NAME);
#else
	GetRString(folder,FOLDER_NAME);
#endif DEBUG

	/*
	 * try system folder
	 */
	err = FindEudoraFolder(&spec,folder,kOnSystemDisk,kSystemFolderType);
	
	/*
	 * if system folder failed, try documents folder
	 */
	if (err) err = FindEudoraFolder(&spec,folder,kOnSystemDisk,kDocumentsFolderType);

	if (err && HaveOSX())
	{
		//	Search classic folders
		//	Try system folder
		err = FindEudoraFolder(&spec,folder,kClassicDomain,kSystemFolderType);
		//	Try documents folder
		if (err) err = FindEudoraFolder(&spec,folder,kClassicDomain,kDocumentsFolderType);		
		if (err)
		{
			// Finding the classic documents folder current doesn't work (OS X 10.1.3). Let's try
			// to find it ourselves.
			err = FindFolder(kClassicDomain,kSystemFolderType,False,&spec.vRefNum,&spec.parID);
			if (!err)
			{
				FSMakeFSSpec(spec.vRefNum,spec.parID,nil,&spec);	// system folder
				err = FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(scratch,USA_DOCUMENTS),&spec);	// documents folder
				Root.vRef = spec.vRefNum;
				Root.dirId = SpecDirId(&spec);
				err = FSMakeFSSpec(Root.vRef,Root.dirId,folder,&spec);
				if (err==fnfErr) err = LocalizeRename(&spec,USA_EUDORA_FOLDER);
			}
		}

		// Need to end with OS X documents folder
		if (err) err = FindEudoraFolder(&spec,folder,kOnSystemDisk,kDocumentsFolderType);
	}
	
	/*
	 * now we are pointed where we think the folder should go.  Create it, so that it will
	 * exist if we can make it
	 */
	FSpDirCreate(&spec,smSystemScript,&Root.dirId);
	
	/*
	 * Get info, and possibly resolve alias
	 */
	err = AFSpGetCatInfo(&spec,&spec,&hfi);
			
	/*
	 * it exists.  Is it a folder?
	 */
	if (!err && hfi.hFileInfo.ioFlAttrib&0x10)
	{
		Root.vRef = spec.vRefNum;
		spec.parID = Root.dirId = hfi.hFileInfo.ioDirID;
		GetRString(spec.name,SETTINGS_FILE);
		LocalizeRename(&spec,USA_SETTINGS);
	}
	else if (!err)
	{
		MakeUserFindSettings(&spec);
		system = False;
	}
	if (err) DieWithError(MAIL_FOLDER,err);

	SettingsInit(&spec,system,false);
}

/************************************************************************
 * FindEudoraFolder - see if Eudora Folder is at specified location, set up FSSpec
 ************************************************************************/
static OSErr FindEudoraFolder(FSSpecPtr spec,StringPtr folder,short vRefNum,OSType folderType)
{
	OSErr	err;
	
	if (!(err = FindFolder(vRefNum,folderType,False,&Root.vRef,&Root.dirId)))
	{
		err = FSMakeFSSpec(Root.vRef,Root.dirId,folder,spec);
		if (err==fnfErr) err = LocalizeRename(spec,USA_EUDORA_FOLDER);
	}
	return err;
}	

/************************************************************************
 * MakeUserFindSettings - force the user to find a Eudora Settings file
 ************************************************************************/
void MakeUserFindSettings(FSSpecPtr spec)
{
	Boolean	justQuit;
	
	justQuit = true;
	if (ReallyDoAnAlert(INSIST_SETTINGS_ALRT,Stop)==1) {
		justQuit = MakeUserFindSettingsNav (spec);
	}
	if (justQuit) {
		Cleanup();
		ExitToShell();
	}
}

/************************************************************************
 * DateWarning - warn the user about the date, if need be.  Returns true
 *  if date not set
 ************************************************************************/
Boolean DateWarning(Boolean uiOK)
{
	short vRef;
	long dirId;
	Str63 date;
	CInfoPBRec hfi;
	FSSpec spec;
	short where = -1;
	short bestWhere = REAL_BIG;
	
	BuildDateHeader(date,0);
	
	if (!*date)
	{
		if (uiOK)
		{
			WarnUser(USE_MAP,0);
		  if (!FindFolder(kOnSystemDisk,kControlPanelFolderType,False,&vRef,&dirId))
		  {
				hfi.hFileInfo.ioNamePtr = date;
				hfi.hFileInfo.ioFDirIndex = 0;
				while(!DirIterate(vRef,dirId,&hfi))
					if (TypeIsOnListWhere(hfi.hFileInfo.ioFlFndrInfo.fdCreator,DATELOC_TYPE,&where) && where<bestWhere)
					{
						bestWhere = where;
						SimpleMakeFSSpec(vRef,dirId,date,&spec);
						if (!where) break;
					}
		  }
	  }
		if (bestWhere<REAL_BIG) OpenOtherDoc(&spec,false,false,nil);
	  return(true);
	}

	return(false);
}

#ifdef TWO
#pragma segment Main
pascal void FMSError(PStr s);
pascal void FMSError(PStr s)
{
	AlertStr(OK_ALRT,Caution,s);
}
pascal void FMSFilter(EventRecord *event);
pascal void FMSFilter(EventRecord *event)
{
	MiniMainLoop(event);
}
#pragma segment Ends
#endif

/**********************************************************************
 * PutUpMenus - set up the menu bar
 **********************************************************************/
void PutUpMenus()
{
	int menu;
	MenuHandle mHandle;
	Str63 scratch;
	short key;
	
	/*
	 * add the standard ones
	 */
	for (menu=APPLE_MENU;menu<MENU_LIMIT;menu++)
	{
		if (mHandle=GetMHandle(menu)) ReleaseResource_(mHandle);
		mHandle = GetMenu(menu+1000);
		if (mHandle==nil)
			DieWithError(GET_MENU,MemError());
		InsertMenu(mHandle,0);
		if (menu==FILE_MENU)
		{
			SInt32	result;
			Gestalt(gestaltMenuMgrAttr,&result);
			if (result&gestaltMenuMgrAquaLayoutMask)
			{
				//	No File->Quit menu item under aqua
				DeleteMenuItem(mHandle,FILE_QUIT_ITEM);
				DeleteMenuItem(mHandle,FILE_QUIT_ITEM-1);
			}
		}
		else if (menu==SPECIAL_MENU)
		{
		extern short gDeletedSpecialItem;
		if ( !PrefIsSet ( PREF_SHOW_CHANGE_PASSWORD )) {
			DeleteMenuItem ( mHandle, SPECIAL_CHANGE_ITEM );
			gDeletedSpecialItem = SPECIAL_CHANGE_ITEM;
			}
		}
			
	}
	
	if (mHandle=GetMHandle(WINDOW_MENU)) ReleaseResource_(mHandle);
	mHandle = GetMenu(WINDOW_MENU+1000);
	if (mHandle==nil)
		DieWithError(GET_MENU,MemError());
	InsertMenu(mHandle,0);
#ifdef DEBUG
	if (RunType!=Production)
	{
		InsertMenu(GetMenu(DEBUG_MENU+1000),0);
		AttachHierMenu(DEBUG_MENU,dbAds,AD_SELECT_HIER_MENU);
		GetAndInsertHier(AD_SELECT_HIER_MENU);
	}
#endif
	
	/*
	 * take care of special menus
	 */
	BuildBoxMenus();						/* build the menus that refer to Mailboxes */

	/*
	 * the user menus
	 */
	AttachHierMenu(EDIT_MENU,EDIT_TEXT_ITEM,TEXT_HIER_MENU);
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking)) {
		AttachHierMenu(EDIT_MENU,EDIT_ISPELL_HOOK_ITEM,SPELL_HIER_MENU);
		GetAndInsertHier(SPELL_HIER_MENU);
	}
#endif
	GetAndInsertHier(TEXT_HIER_MENU);
	GetAndInsertHier(SIG_HIER_MENU);
	GetAndInsertHier(SERVER_HIER_MENU);
	GetAndInsertHier(ATTACH_MENU);
	GetAndInsertHier(TEXT_SIZE_HIER_MENU);
	AttachHierMenu(TEXT_HIER_MENU,tmFont,FONT_HIER_MENU);
	AttachHierMenu(TEXT_HIER_MENU,tmColor,COLOR_HIER_MENU);
	AttachHierMenu(TEXT_HIER_MENU,tmMargin,MARGIN_HIER_MENU);
#ifdef NEVER
	AttachHierMenu(CHANGE_HIER_MENU,CHANGE_STATUS_ITEM,STATE_HIER_MENU);
#endif
	SetupRecipMenus();

	if (HasFeature (featureStationery))
		BuildStationMenu();
	if (HasFeature (featureMultiplePersonalities))
		BuildPersMenu();

	/*
	 * hier menus
	 */
	GetAndInsertHier(FIND_HIER_MENU);
	GetAndInsertHier(SORT_HIER_MENU);
	GetAndInsertHier(CHANGE_HIER_MENU);
	GetAndInsertHier(COLOR_HIER_MENU);
	AddIconsToColorMenu(GetMHandle(COLOR_HIER_MENU));
	MightSwitch();
	GetAndInsertHier(FONT_HIER_MENU);
	AppendResMenu(GetMHandle(FONT_HIER_MENU),'FONT');
	AfterSwitch();
	GetAndInsertHier(MARGIN_HIER_MENU);
	GetAndInsertHier(TLATE_SEL_HIER_MENU);
	GetAndInsertHier(TLATE_SET_HIER_MENU);
	GetAndInsertHier(TLATE_ATT_HIER_MENU);
#ifdef TWO
	GetAndInsertHier(STATE_HIER_MENU);
#endif
	GetAndInsertHier(PRIOR_HIER_MENU);
	GetAndInsertHier(LABEL_HIER_MENU);
	{
		Byte type;
		Handle suite=nil;
		
		if ((mHandle=GetMHandle(LABEL_HIER_MENU)) && (GetMenuItemIconHandle(mHandle,1,&type,&suite) || !suite))
		{
			GetIconSuite(&suite,NO_LABEL_SICN,svAllSmallData);
			SetMenuItemIconHandle(mHandle,1,kMenuIconSuiteType,suite);
		}
	}

	
#ifdef DEBUG
	if (RunType!=Production)
	{
		GetAndInsertHier(WIND_PROP_HIER_MENU);
		AttachHierMenu(WINDOW_MENU,WIN_PROPERTIES_ITEM,WIND_PROP_HIER_MENU);
	}
#endif
	if (NewTables)
	{
		mHandle = GetMenu(TABLE_HIER_MENU+1000);
		if (CountMenuItems(GetMHandle(CHANGE_HIER_MENU))<CHANGE_TABLE_ITEM)
		{
			AppendMenu(GetMHandle(CHANGE_HIER_MENU),GetMenuTitle(mHandle,scratch));
		}
		AttachHierMenu(CHANGE_HIER_MENU,CHANGE_TABLE_ITEM,TABLE_HIER_MENU);
		InsertMenu(mHandle,-1);
	}
	
	// Adjust command keys
	mHandle = GetMHandle(EDIT_MENU);
	GetItemCmd(mHandle,EDIT_PASTE_ITEM,&key);
	if (key)
	{
		SetItemCmd(mHandle,EDIT_QUOTE_ITEM,key);
		SetMenuItemModifiers(mHandle,EDIT_QUOTE_ITEM,kMenuOptionModifier);
	}

	mHandle = GetMHandle(TEXT_HIER_MENU);
	GetItemCmd(mHandle,tmQuote,&key);
	if (key)
	{
		SetItemCmd(mHandle,tmUnquote,key);
		SetMenuItemModifiers(mHandle,tmUnquote,kMenuOptionModifier);
	}

	AdjustStupidCommandKeys();
	
	AddCustomSortItems();

	if (HasFeature (featureStyleMargin))
		BuildMarginMenu();

#ifdef TWO
#ifdef ETL

	ETLInit();
#endif
#endif

	BuildSearchMenu();

	/*
	 * add the help menus
	 */
	AddHelpMenuItems();
	
	/*
	 * add finder menu items
	 */
	RefreshLabelsMenu();
	
	/*
	 * add the Word Services menus
	 */

	AddWSMenuItems();

	// Scripts menu
	BuildScriptMenu();

	ChangeCmdKeys();

	InitSharedMenus((void*)FMSError,(void*)FMSFilter);					/* Frontier menusharing */

	/*
	 * draw the menu bar
	 */
	gMenuBarIsSetup = true;
	EnableMenus(nil,false);
	
}

/************************************************************************
 * AddCustomSortItems - add custom sorts
 ************************************************************************/
void AddCustomSortItems(void)
{
	Str255 s;
	Str63 menuItem;
	short i;
	MenuHandle mh = GetMHandle(SORT_HIER_MENU);
	
	if (mh)
		for (i=1;*GetRString(s,SortsStrn+i);i++)
		{
			if (i==1) AppendMenu(mh,"\p-");
			if (!InterpretSortString(s,nil,nil,menuItem))
				MyAppendMenu(mh,menuItem);
			else MyAppendMenu(mh,s);
		}
}

/************************************************************************
 * GetAndInsertHier - get a menu and insert it
 ************************************************************************/
void GetAndInsertHier(short menu)
{
	MenuHandle mHandle;
	
	ZapResMenu(menu);
	mHandle = GetMenu(menu+1000);
	if (mHandle==nil) DieWithError(GET_MENU,menu);
	InsertMenu(mHandle,-1);
}

/************************************************************************
 * AddFinderItems - add the items from the Finder's label menu
 ************************************************************************/
void AddFinderItems(MenuHandle mh)
{
	short item;
	Str255 string;
	RGBColor color;
	Handle suite;
	short mItem;
	
	if (mh)
	{
		mItem = CountMenuItems(mh);
		for (item=1;item<=15;item++)
		{
			if (item==8)
			{
				AppendMenu(mh,"\p-");
				DisableItem(mh,CountMenuItems(mh));
				mItem++;
			}
			MyGetLabel(item,&color,string);
			if (!*string) PCatC(string,item);
			MyAppendMenu(mh,string);
			mItem++;
#ifdef LABEL_ICONS
			RefreshLabelIcon(item,&color);
			if (!GetIconSuite(&suite,LABEL_SICN_BASE+item-1,svAllSmallData))
				SetMenuItemIconHandle(mh,mItem,kMenuIconSuiteType,suite);
#endif
			if (!BlackWhite(&color))
			{
				SetMenuColor(mh,mItem,&color);
			}
		}
	}
}

/************************************************************************
 * RefreshLabelIcon - refresh the icon for a label
 ************************************************************************/
#ifdef LABEL_ICONS
OSErr RefreshLabelIcon(short item,RGBColor *color)
{
	Boolean changed;
	OSErr err = RefreshRGBIcon(LABEL_SICN_BASE+item-1,color,LABEL_SICN_TEMPLATE,&changed);
	if (changed) UpdateLabelControls();	
	return(err);
}

/************************************************************************
 * UpdateLabelControls - update label controls in windows
 ************************************************************************/
void UpdateLabelControls(void)
{
	TBarHasChanged = True;
}

#endif

/************************************************************************
 * AddIconsToColorMenu - add icons to the color menu
 ************************************************************************/
OSErr AddIconsToColorMenu(MenuHandle mh)
{
	short item;
	RGBColor color;
	OSErr err = noErr;
	MCEntryPtr mc;
	Handle suite;
	
	ASSERT(mh);
	
	for (item=CountMenuItems(mh);!err && item;item--)
	{
		if (mc = GetMCEntry(COLOR_HIER_MENU,item))
		{
			color = mc->mctRGB2;
			if (!BlackWhite(&color))
			{
				err = RefreshRGBIcon(COLORS_MENU_ICON_BASE+item-1,&color,COLORS_MENU_ICON_TEMPLATE,nil);
				if (!GetIconSuite(&suite,COLORS_MENU_ICON_BASE+item-1,svAllSmallData))
					SetMenuItemIconHandle(mh,item,kMenuIconSuiteType,suite);
			}
			ASSERT(!err);
		}
	}
	return(err);
}

/************************************************************************
 * AddHelpMenuItems - add items to the help menu
 ************************************************************************/
void AddHelpMenuItems(void)
{
	Handle resH;
	short r;
	short id;
	OSType type;
	Str255 name;
	MenuHandle hm;

	if (!HMGetHelpMenuHandle(&hm) && hm) /* the check for nil is needed courtesy of Norton Utilities, crap that it is */
	{
		OriginalHelpCount = CountMenuItems(hm);
		SetResLoad(False);
		for (r = HELP_PICT_BASE; r<HELP_PICT_BASE+50; r++)
			if ((resH=GetResource_('TEXT',r)) || (resH=GetResource_('PICT',r)))
				AddRes2HelpMenu(hm,resH);
				
		for (r = 1;(resH=GetIndResource_('TEXT',r)) || (resH=GetIndResource_('PICT',r));r++)
		{
			GetResInfo(resH,&id,&type,name);
			if (id>=HELP_PICT_BASE+50 && id<HELP_PICT_BASE+1000)
				AddRes2HelpMenu(hm,resH);
		}
		SetResLoad(True);
		
		AppendMenu(hm,"\p-");DisableItem(hm,CountMenuItems(hm));

#ifndef DEATH_BUILD
#ifdef NAG
		// Append the "Payment & Registration�" menu item
		AppendMenu (hm, GetRString (name, PAY_AND_REGISTER));
/*
		// Append the "Enter Registration Code�" menu item
		if (!RegisteredUser ((*nagState)->state))
			AppendMenu (hm, GetRString (name, ENTER_REG_CODE));
		// Append the "Try Full Features�" menu item
		if (LightUser ((*nagState)->state))
			AppendMenu (hm, GetRString (name, TRY_FULL_FEATURES));
*/
#else
		AppendMenu(hm,GetRString(name,PURCHASE_EUDORA));
		if (GetPrefLong(PREF_REGISTER)!=0x0fffffff) AppendMenu(hm,GetRString(name,REGISTER_EUDORA));
#endif
#endif
		AppendMenu(hm,GetRString(name,INSERT_CONFIGURATION));
		AppendMenu(hm,GetRString(name,HELP_SUGGEST_MTEXT));
		AppendMenu(hm,GetRString(name,HELP_BUG_MTEXT));
		
		EndHelpCount = CountMenuItems(hm);
	}
}


/**********************************************************************
 * 
 **********************************************************************/
 void AddRes2HelpMenu(MenuHandle hm,Handle resH)
{
	short id;
	OSType type;
	Str255 name;

	GetResInfo(resH,&id,&type,name);
	if (*name) MyAppendMenu(hm,name);
	else {AppendMenu(hm,"\p-");DisableItem(hm,CountMenuItems(hm));}
	ReleaseResource_(resH);
}

/************************************************************************
 * AddWSMenuItems - add Word Services
 ************************************************************************/
void AddWSMenuItems(void)
{
	Handle resH;
	short r;
	short id;
	OSType type;
	Str255 name;
	MenuHandle mh;
	
	if (mh = GetMHandle(EDIT_MENU))
	{
		SetResLoad(False);
		for (r = 1;resH=GetIndResource_(WS_ALIAS_TYPE,r);r++)
		{
			GetResInfo(resH,&id,&type,name);
			MyAppendMenu(mh,name);
		}
		SetResLoad(True);
	}
	WordServices = NuHandle((r-1)*sizeof(**WordServices));
	if (WordServices)
	{
		WriteZero(LDRef(WordServices),GetHandleSize_(WordServices));
		UL(WordServices);
	}
}

/**********************************************************************
 * HuntSpellswell - find & install spellswell, if not there already
 **********************************************************************/
void HuntSpellswell(void)
{
	OSType creator = FetchOSType(SPELLSWELL_CREATOR);
	FSSpec spec;
	OSErr err;
	short dtRef;
	short vRef;
	
	if (!(err = GetFileByRef(AppResFile,&spec)))
	{
		vRef = spec.vRefNum;
		if (!(err = DTRef(vRef,&dtRef)))
		{
			if (!(err = DTGetAppl(vRef,dtRef,creator,&spec)))
			{
				if (FileCreatorOf(&spec)==creator)
				{
					InstallSpeller(&spec);
				}
			}
		}
	}
}

/**********************************************************************
 * InferUnread - figure out if a mailbox has unread mail from its label
 *  label 0xe means unread
 *  if there is an age limit, and mailbox is older, remove label
 *
 *	- Added imap parameter to do special things to IMAP mail folders
 **********************************************************************/
Boolean InferUnread(CInfoPBRec *hfi, Boolean imap)
{
	CInfoPBRec *info = hfi;
	CInfoPBRec mailboxFileInfo;
	
	// if we're looking at an IMAP mailbox, peek inside the folder at the mailbox file.
	if (imap)
	{
		Zero(mailboxFileInfo);
		if (noErr==HGetCatInfo(info->hFileInfo.ioVRefNum,info->hFileInfo.ioDirID,info->hFileInfo.ioNamePtr,&mailboxFileInfo))
			info = &mailboxFileInfo;
	}
	
	if ((info->hFileInfo.ioFlFndrInfo.fdFlags&0xe)==0xe)
	{
		uLong minDate;
		FSSpec spec;
		uLong curDate;
		
		minDate = GetRLong(UNREAD_LIMIT)*24*3600;
		if (minDate)
		{
			minDate = LocalDateTime()-minDate;
			if (PrefIsSet(PREF_NEW_TOC))
				curDate = info->hFileInfo.ioFlMdDat;
			else
			{
				SimpleMakeFSSpec(info->hFileInfo.ioVRefNum,info->hFileInfo.ioFlParID,info->hFileInfo.ioNamePtr,&spec);
				Box2TOCSpec(&spec,&spec);
				curDate = FSpModDate(&spec);
			}
			if (curDate < minDate)
			{
				info->hFileInfo.ioFlFndrInfo.fdFlags &= ~0xe;
				HSetFInfo(info->hFileInfo.ioVRefNum,info->hFileInfo.ioFlParID,info->hFileInfo.ioNamePtr,&info->hFileInfo.ioFlFndrInfo);
				return(False);	/* too old to have really unread mail */
			}
		}
		return(True);	/* label set to 1 and no min date or min date ok */
	}
	return(False);	/* label not set to 1 */
}

/**********************************************************************
 * BoxFolder - build the mailbox menus according to the files in
 * a mailbox folder.
 *
 * - heavily uglified to add IMAP cache folders to menus
 **********************************************************************/
void BoxFolder(short vRef,long dirId,MenuHandle *menus,BoxFolderDescPtr bfd, Boolean *anyUnread, Boolean appending,Boolean isIMAP)
{
	NameType thisOne;
	NameType **names=nil;
	short count=0,i,item,dirItem;
	MenuHandle newMenus[MENU_ARRAY_LIMIT];
	Str15 suffix;
	Str15 tmpsuffix;
	Str32 s;
	Boolean localAnyUnread=False;
	Boolean isIMAPDir = false;	// set to true once we know we're processing an IMAP cache directory
	Boolean isSelectable = false;
	Boolean	haveIMAPpers = false;
	Boolean isRoot = vRef==MailRoot.vRef && dirId==MailRoot.dirId;
	Boolean junkIsSpecial = isRoot && JunkPrefBoxNoUnread();
	uLong notAgain = 0;
	
	/*
	 * make sure we're kosher
	 */
	if (bfd->level>MAX_BOX_LEVELS) DieWithError(TOO_MANY_LEVELS,bfd->level);
	AddBoxMap(vRef,dirId);

	/*
	 * get started
	 */
	if ((names=NuHandle(0L))==nil) DieWithError(ALLO_MBOX_LIST,MemError());
	if (!PrefIsSet(PREF_NEW_TOC)) GetRString(suffix,TOC_SUFFIX);
	else *suffix = 0;
	GetRString(tmpsuffix,TEMP_SUFFIX);

	/*
	 * read names of mailbox files
	 */
	for (bfd->hfi.hFileInfo.ioFDirIndex=0;!DirIterate(vRef,dirId,&bfd->hfi);)
	{
		if (HaveOSX() && bfd->hfi.hFileInfo.ioNamePtr[0] && bfd->hfi.hFileInfo.ioNamePtr[1]=='.')
			// On OS X, files beginning with "." are invisible
			goto nextfile;
			
		/*
		 * make an FSSpec out of the info
		 */
		SimpleMakeFSSpec(vRef,dirId,bfd->hfi.hFileInfo.ioNamePtr,&thisOne.spec);

		/*
		 * aliases at the root level that are marked with the stationery bit
		 * mean they were temporary mailboxes and should be cleaned up next
		 * time Eudora is launched on this mail folder
		 */
		if (StartingUp && isRoot &&
				(bfd->hfi.hFileInfo.ioFlFndrInfo.fdFlags&(kIsStationery|kIsAlias))==(kIsStationery|kIsAlias))
		{
			if (notAgain != Hash(thisOne.spec.name))
			{
				notAgain = Hash(thisOne.spec.name);
				if (!FSpDelete(&thisOne.spec))
				{
					bfd->hfi.hFileInfo.ioFDirIndex--;
					continue;
				}
			}
		}
		
		/*
		 * is it a special name?
		 */
		if (IsRoot(&thisOne.spec))
			for (i=0;i<bfd->badCount;i++)
				if (StringSame(bfd->badNames[i],bfd->hfi.hFileInfo.ioNamePtr))
				{
					if (i<3 && InferUnread(&bfd->hfi, false))
						MarkHavingUnread(menus[MAILBOX],i+1);
					goto nextfile;
				}

		
		/*
		 * is this a special name, IMAP-ly speaking?
		 */
		if (IsSpecialIMAPName(bfd->hfi.hFileInfo.ioNamePtr, NULL)) 
		{
			isIMAPDir = true;
			goto nextfile;
		}
			
		/*
		 * Is this another IMAP cache folder?
		 */
		 
		if (IsFreeMode() && IsIMAPSubPers(&thisOne.spec))
		{
			isIMAPDir = true;
			goto nextfile;
		}
			
		/*
		 * read the IMAP info from this box, ONLY if it's an IMAP mailbox.
		 */
		if (bfd->hfi.hFileInfo.ioFlFndrInfo.fdType == IMAP_MAILBOX_TYPE)
			if (ReadIMAPMailboxAttributes(&thisOne.spec, &thisOne.imapAttributes)==noErr) 
				isIMAPDir = true;
			
		/*
		 * does this mailbox have the same name as the enclosing folder, and is it an IMAP folder?
		 */
		if (isIMAPDir && StringSame(bfd->enclosingFolderName,bfd->hfi.hFileInfo.ioNamePtr) && IsIMAPMailboxFile(&thisOne.spec))
		{
			if (!(thisOne.imapAttributes&LATT_NOSELECT))	// we're building a menu to list mailboxes within a selectable mailbox
				isSelectable = true;	
		
			goto nextfile;
		}
		
		thisOne.skipIt = false;
		thisOne.isIMAPpers = false;
		thisOne.moveToTop = isIMAP && StringSame(thisOne.spec.name,GetRString(s,IMAP_INBOX_NAME));
		
		// is this file a folder?
		thisOne.isDir = HFIIsFolderOrAlias(&bfd->hfi);
		
		if (!thisOne.isDir)
		{			
			if ((bfd->hfi.hFileInfo.ioFlFndrInfo.fdType!=MAILBOX_TYPE) && (bfd->hfi.hFileInfo.ioFlFndrInfo.fdType!=IMAP_MAILBOX_TYPE)) continue;
			if (*bfd->hfi.hFileInfo.ioNamePtr>*tmpsuffix && EndsWith(bfd->hfi.hFileInfo.ioNamePtr,tmpsuffix))
				TempWarning(bfd->hfi.hFileInfo.ioNamePtr);
			if (*bfd->hfi.hFileInfo.ioNamePtr>31-*suffix)
			{
				TooLong(bfd->hfi.hFileInfo.ioNamePtr);
				continue;
			}
			
			if (!junkIsSpecial || !SpecIsJunkSpec(&thisOne.spec))
			{
				if (thisOne.hasUnread = InferUnread(&bfd->hfi,false)) localAnyUnread = True;
			}
		}
		else
		{
			FSSpec	tempSpec;
			
			// look inside all IMAP mailboxes and folders to see if they contain unread messages
			if (isIMAP) 
			{
				if (thisOne.hasUnread = InferUnread(&bfd->hfi, true))
					localAnyUnread = True;
			}
			else
				thisOne.hasUnread = false;
			
			if (IsAlias(&thisOne.spec,&tempSpec))		/* resolve alias */
			{
				if (!(thisOne.skipIt=SameSpec(&thisOne.spec,&tempSpec)))
				{
					thisOne.spec.parID = SpecDirId(&tempSpec);	/* get dirid of folder */
					thisOne.spec.vRefNum = tempSpec.vRefNum;
					if (tempSpec.parID == IMAPMailRoot.dirId)
					{
						thisOne.isIMAPpers = true;
						haveIMAPpers = true;
					}
				}
			}
			else
				thisOne.spec.parID = bfd->hfi.hFileInfo.ioDirID;
		}
		PtrPlusHand_ (&thisOne,names,sizeof(thisOne));
		if (MemError()) DieWithError(ALLO_MBOX_LIST,MemError());
		count++;
		nextfile:;
	}
	
	/*
	 * sort them
	 */
	BoxNamesSort(names);
		
	/*
	 * add the New item
	 */
	if (!appending)
	{
		AppendMenu(menus[TRANSFER],bfd->newText);
#ifdef TWO
		AppendMenu(menus[MAILBOX],bfd->newText);
	}
	if (isIMAPDir == false && !appending)
	{
		AppendMenu(menus[MAILBOX],bfd->otherText);
		AppendMenu(menus[TRANSFER],bfd->otherText);
	}
	else
		if (isSelectable == true)	// this menu represents the contents of a SELECTable IMAP mailbox
		{
			AppendMenu(menus[MAILBOX],bfd->thisBoxText);
			AppendMenu(menus[TRANSFER],bfd->thisBoxText);
		}
		else	// this menu represents the contents of a non-SELECTable mailbox - a folder.
		{
			AppendMenu(menus[MAILBOX],"\p-");
			AppendMenu(menus[TRANSFER],"\p-");
		}
#endif

	/*
	 * stick them into the proper menus
	 */
	{
		for (item=0;item<count;item++)
		{
			if ((*names)[item].isDir || (*names)[item].skipIt) continue;
			thisOne = (*names)[item];
			MyAppendMenu(menus[MAILBOX],thisOne.spec.name);
			GetRString(thisOne.spec.name,TRANSFER_PREFIX);
			PCat(thisOne.spec.name,(*names)[item].spec.name);
			MyAppendMenu(menus[TRANSFER],thisOne.spec.name);
			if (thisOne.hasUnread)
				MarkHavingUnread(menus[MAILBOX],CountMenuItems(menus[MAILBOX]));
			CycleBalls();
		}
	}
	
	for (item=0;item<count;item++)
	{
		IMAPMailboxAttributes att;
		
		if (!(*names)[item].isDir || (*names)[item].skipIt) continue;
		thisOne = (*names)[item];
			
		MyAppendMenu(menus[MAILBOX],thisOne.spec.name);
		if (isIMAPDir && thisOne.hasUnread) MarkHavingUnread(menus[MAILBOX],CountMenuItems(menus[MAILBOX]));	
		GetRString(thisOne.spec.name,TRANSFER_PREFIX);
		PCat(thisOne.spec.name,(*names)[item].spec.name);
		MyAppendMenu(menus[TRANSFER],thisOne.spec.name);
		thisOne = (*names)[item];
		
		// Special cases when not to stick a submenu onto this guy
		if (isIMAPDir && MailboxAttributes(&thisOne.spec,&att))
		{
			// no submenu on mailboxes that can't have children
			if (att.noInferiors) continue;
			
			// no submenu on mailboxes with messages but no children
			if (!att.noSelect && !att.hasChildren) continue;
		}
		
		for (i=0;i<MENU_ARRAY_LIMIT;i++)
		{
			dirItem = i*MAX_BOX_LEVELS+bfd->level+1;
			if (!(newMenus[i] = NewMenu(dirItem,thisOne.spec.name)))
				DieWithError(ALLO_MBOX_LIST,MemError());
			AttachHierMenu(GetMenuID(menus[i]),CountMenuItems(menus[i]),dirItem);
			InsertMenu(newMenus[i],-1);
#ifdef NEVER
			if (HasHelp && !isRoot && !Get1Resource('hmnu',dirItem))
			{
				Handle mHand=GetResource_('hmnu',i*MAX_BOX_LEVELS+1);
				Handle newHand=mHand;
				if (mHand && !HandToHand_(&newHand))
				{
					AddResource_(newHand,'hmnu',dirItem,"");
					SetResAttrs(newHand,resChanged|resPurgeable);
				}
			}
#endif
		}
		bfd->level++;
		PCopy(bfd->enclosingFolderName,thisOne.spec.name);
		BoxFolder(thisOne.spec.vRefNum,thisOne.spec.parID,newMenus,bfd,&thisOne.hasUnread,false,isIMAP);
		if (MemError()) DieWithError(ALLO_MBOX_LIST,MemError());
		if (thisOne.hasUnread)
		{
			localAnyUnread = True;
			MarkHavingUnread(menus[MAILBOX],CountMenuItems(menus[MAILBOX]));
		}
	}

	//	If MacOS 8.5 (Appearance 1.1) or greater, make sure
	//	menu items 32+ are enabled using menu manager
	if (gHave85MenuMgr)
	{
		count = CountMenuItems(menus[MAILBOX]);
		for (item=32;item<=count;item++)
		{
			EnableMenuItem(menus[MAILBOX],item); 
			EnableMenuItem(menus[TRANSFER],item);
		}
	}

	ZapHandle(names);
	*anyUnread = localAnyUnread || *anyUnread;
}

/************************************************************************
 * MarkHavingUnread - mark a mailbox as having unread messages
 ************************************************************************/
void MarkHavingUnread(MenuHandle mh, short item)
{
	SetItemStyle(mh,item,UnreadStyle);
}

/************************************************************************
 * BoxNamesSort - sort mailbox names
 ************************************************************************/
void BoxNamesSort(NameType **names)
{
	NameType last;
	short n = GetHandleSize_(names)/sizeof(last);

	Zero(last);
	InfiniteString(last.spec.name,sizeof(last.spec.name));
	PtrPlusHand_(&last,names,sizeof(last));
	
	QuickSort((void*)LDRef(names),sizeof(last),0,n-1,(void*)NameCompare,(void*)NameSwap);
	UL(names);
	
	SetHandleBig_(names,n*sizeof(last));
}

/************************************************************************
 * NameCompare - compare two names
 ************************************************************************/
int NameCompare(NameType *n1, NameType *n2)
{
#ifdef IMAP
	if (n1->moveToTop != n2->moveToTop)
		return n1->moveToTop ? -1 : 1;
#endif
	return(StringComp(n1->spec.name,n2->spec.name));
}


/************************************************************************
 * NameSwap - swap two names
 ************************************************************************/
void NameSwap(NameType *n1, NameType *n2)
{
	NameType temp = *n2;
	*n2 = *n1;
	*n1 = temp;
}

/************************************************************************
 * TempWarning - warn the user about a temp file in his mailbox structure
 ************************************************************************/
void TempWarning(UPtr filename)
{
	Str255 msg;
	GetRString(msg,TEMP_WARNING);
	MyParamText(msg,filename,"","");
	ReallyDoAnAlert(OK_ALRT,Caution);
}

/**********************************************************************
 * BuildBoxMenus - build the mailbox menus according to the files in
 * our mailbox folder.
 **********************************************************************/
void BuildBoxMenus()
{
	BoxFolderDesc bfd;
	short badIndices[] = {IN,OUT,JUNK,TRASH};	// 1st 3 must be in,out,trash
	Str31 badNames[sizeof(badIndices)/sizeof(short)];
	MenuHandle menus[MENU_ARRAY_LIMIT];
	short i;
	Boolean anyUnread;

#ifdef DEBUG
	Log(LOG_TPUT,"\pBuildBoxMenus begin");
#endif

	/*
	 * this may mess with the head of the filters.
	 * trash them if we dare
	 */
	if (Filters && !FiltersRefCount) ZapFilters();
	
	/*
	 * set up descriptor (saves stack space, parameter shuffling)
	 */
	bfd.badNames = badNames;
	bfd.badCount = sizeof(badIndices)/sizeof(short);
	bfd.level = g16bitSubMenuIDs ? BOX_MENU_START-1: 0;
	Zero(bfd.hfi);
	bfd.hfi.hFileInfo.ioNamePtr = bfd.hfi_namePtr;
	
	if (BoxMap!=nil) RemoveBoxMenus();
	if ((BoxMap=NuHandle(0L))==nil) DieWithError(ALLO_MBOX_LIST,MemError());
	for (i=0;i<sizeof(badIndices)/sizeof(short);i++)
	  GetRString(badNames[i],badIndices[i]);
	GetRString(bfd.newText,NEW_ITEM_TEXT);
#ifdef TWO
	GetRString(bfd.otherText,OTHER_ITEM_TEXT);
#ifdef	IMAP
	GetRString(bfd.thisBoxText,IMAP_THIS_MAILBOX_NAME);	
	GetRString(bfd.enclosingFolderName,MAIL_FOLDER_NAME);
#endif
#endif
			
	/*
	 * stick them into the proper menus
	 */
	menus[MAILBOX] = GetMHandle(MAILBOX_MENU);
	menus[TRANSFER] = GetMHandle(TRANSFER_MENU);
	
	/*
	 * rename a few
	 */
	{
		short aliases[] = {FILE_ALIAS_IN,FILE_ALIAS_OUT,FILE_ALIAS_JUNK,FILE_ALIAS_TRASH,0};
		Str63 name;
		Str15 prefix;
		
		GetRString(prefix,TRANSFER_PREFIX);
		for (i=0;aliases[i];i++)
		{
			GetRString(name,aliases[i]);
			SetMenuItemText(menus[MAILBOX],i+1,name);
			PInsert(name,sizeof(name),prefix,name+1);
			SetMenuItemText(menus[TRANSFER],i+1,name);
		}
	}
			
	/*
	 * now, do the recursive thing
	 */
	BoxFolder(MailRoot.vRef,MailRoot.dirId,menus,&bfd,&anyUnread,false,false);
#ifdef IMAP
	if (IMAPExists())
	{
		bfd.level++;
		BoxFolder(IMAPMailRoot.vRef,IMAPMailRoot.dirId,menus,&bfd,&anyUnread,true,true);
	}
#endif
#ifdef NEVER
	if (HasHelp) MakeHmnusPurge();
#endif
	BuildBoxCount();
#ifdef DEBUG
	Log(LOG_TPUT,"\pBuildBoxMenus end");
#endif
}

/************************************************************************
 * RemoveBoxMenus - get a clean slate
 ************************************************************************/
void RemoveBoxMenus(void)
{
	ZapHandle(BoxMap); BoxMap = nil;
#ifdef NEVER
	TrashHmnu();
#endif
	CheckBox(nil,False);
#ifdef TWO
	TrashMenu(GetMHandle(MAILBOX_MENU),MAILBOX_NEW_ITEM);
#else
	TrashMenu(GetMHandle(MAILBOX_MENU),MAILBOX_FIRST_USER_ITEM);
#endif
	TrashMenu(GetMHandle(TRANSFER_MENU),TRANSFER_NEW_ITEM);
}

/**********************************************************************
 * BuildStationMenu - build the stationery menu
 **********************************************************************/
void BuildStationMenu(void)
{
	MenuHandle mhReply,mhNew;
	Str255 string;
	FSSpec folderSpec;
	CInfoPBRec hfi;
	long newDirId;
	
	if (!HasFeature (featureStationery))
		return;
		
	/*
	 * create the folder
	 */
	DirCreate(Root.vRef,Root.dirId,GetRString(string,STATION_FOLDER),&newDirId);
	
	/*
	 * wipe out the old menu
	 */
	if (mhReply=GetMHandle(REPLY_WITH_HIER_MENU)) {DeleteMenu(REPLY_WITH_HIER_MENU); DisposeMenu(mhReply);}
	if (mhNew=GetMHandle(NEW_WITH_HIER_MENU)) {DeleteMenu(NEW_WITH_HIER_MENU); DisposeMenu(mhNew);}
	
	/*
	 * is there a folder?
	 */
	if (!SubFolderSpec(STATION_FOLDER,&folderSpec))
	{
		StationVRef = folderSpec.vRefNum;
		StationDirId = folderSpec.parID;
		
		/*
		 * attach the menu
		 */
		AttachHierMenu(MESSAGE_MENU,MESSAGE_REPLY_WITH_ITEM,REPLY_WITH_HIER_MENU);
		AttachHierMenu(MESSAGE_MENU,MESSAGE_NEW_WITH_ITEM,NEW_WITH_HIER_MENU);
	
		/*
		 * create the new menu
		 */
		MyGetItem(GetMHandle(MESSAGE_MENU),MESSAGE_REPLY_WITH_ITEM,string);
		if (mhReply = NewMenu(REPLY_WITH_HIER_MENU,string))
		if (mhNew = NewMenu(NEW_WITH_HIER_MENU,GetRString(string,NEW_MESSAGE_WITH)))
		{
			InsertMenu(mhReply,-1);
			InsertMenu(mhNew,-1);
			
			/*
			 * iterate through all the files, adding them to the list
			 */
			hfi.hFileInfo.ioNamePtr = string;
			hfi.hFileInfo.ioFDirIndex = 0;
			while (!DirIterate(folderSpec.vRefNum,folderSpec.parID,&hfi))
				if (hfi.hFileInfo.ioFlFndrInfo.fdType==STATIONERY_TYPE)
				{
					MyAppendMenu(mhReply,string);
					MyAppendMenu(mhNew,string);
				}
			
			if (!CountMenuItems(mhReply))
			{
				MyAppendMenu(mhReply,GetRString(string,NO_STATIONERY));
				DisableItem(mhReply,1);
				SetItemStyle(mhReply,1,italic);
				MyAppendMenu(mhNew,string);
				DisableItem(mhNew,1);
				SetItemStyle(mhNew,1,italic);
			}
		}
	}
}

/**********************************************************************
 * BuildPersMenu - build the personality menu
 **********************************************************************/
void BuildPersMenu(void)
{
	MenuHandle mh;
	Str255 string;
	PersHandle pers;
	
	if (!HasFeature (featureMultiplePersonalities))
		return;

	/*
	 * wipe out the old menu
	 */
	if (mh=GetMHandle(PERS_HIER_MENU)) {DeleteMenu(PERS_HIER_MENU); ReleaseResource_(mh);}
	
	/*
	 * grab the menu
	 */
	GetAndInsertHier(PERS_HIER_MENU);
	
	if (mh=GetMHandle(PERS_HIER_MENU))
	{
		for (pers=(*PersList)->next;pers;pers=(*pers)->next)
			MyAppendMenu(mh,PCopy(string,(*pers)->name));
	}
}

/**********************************************************************
 * BuildMarginMenu - build the margin menu
 **********************************************************************/
void BuildMarginMenu(void)
{
	MenuHandle mh;
	Str255 scratch;
	Str255 name;
	short i;
	
	if (mh=GetMHandle(MARGIN_HIER_MENU)) {TrashMenu(mh,3);}
	
	for (i=1;*GetRString(scratch,MarginStrn+i);i++)
	{
		if (!Ascii2Margin(scratch,name,nil))
		{
			if (*name==1 && name[1]=='-') AppendMenu(mh,name);
			else MyAppendMenu(mh,name);
		}
		else AppendMenu(mh,"\p-");
	}
	GetRString(scratch,ADD_BULLETS);
	AppendMenu(mh,scratch);
}


#ifdef NEVER
/************************************************************************
 * TrashHmnu - get rid of the help resources for subfolders
 ************************************************************************/
void TrashHmnu(void)
{
	short i,n;
	Handle hHand;
	
	n = GetHandleSize_(BoxMap)/sizeof(long);
	for (i=2;i<=n;i++)
	{
		ZapSettingsResource('hmnu',i);
		ZapSettingsResource('hmnu',i+MAX_BOX_LEVELS);
	}
	MyUpdateResFile(CurResFile());
}

/************************************************************************
 * MakeHmnusPurge - make sure we can purge the Hmnus
 ************************************************************************/
void MakeHmnusPurge(void)
{
	short i,n;
	Handle hHand;
	
	/*
	 * first off, get them written out
	 */
	MyUpdateResFile(CurResFile());
	
	/*
	 * trash 'em
	 */
	n = GetHandleSize_(BoxMap)/sizeof(long);
	SetResLoad(False);
	for (i=1;i<=n;i++)
	{
		if (hHand=Get1Resource('hmnu',i))
			ReleaseResource_(hHand);
		if (hHand=Get1Resource('hmnu',i+MAX_BOX_LEVELS))
			ReleaseResource_(hHand);
	}
	SetResLoad(True);
}
#endif

#ifdef THREADING_ON
void CreateTempBox(short which)
{
	Str63 name;
	int err;
	FSSpec spec,
				spool;
	
	if (err=SubFolderSpec(SPOOL_FOLDER,&spool))
		DieWithError(CREATING_MAILBOX,err);	// or some other error

	// create temporary mailbox
	GetRString(name,which);
	if (FSMakeFSSpec(spool.vRefNum,spool.parID,name,&spec))
	if (err=MakeResFile(name,spool.vRefNum,spool.parID,CREATOR,MAILBOX_TYPE))
		DieWithError(CREATING_MAILBOX,err);		
}
#endif THREADING_ON

/**********************************************************************
 * CreateBoxes - Create the "In" and "Out" mailboxes if need be
 **********************************************************************/
void CreateBoxes()
{
	Str63 name;
	int err;
	FSSpec spec;
	
#ifdef THREADING_ON
	CreateTempBox(IN_TEMP);
	CreateTempBox(OUT_TEMP);
#endif THREADING_ON

	GetRString(name,IN);
	if (FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,name,&spec))
	if (err=MakeResFile(name,MailRoot.vRef,MailRoot.dirId,CREATOR,MAILBOX_TYPE))
		DieWithError(CREATING_MAILBOX,err);
	GetRString(name,OUT);
	if (FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,name,&spec))
	if (err=MakeResFile(name,MailRoot.vRef,MailRoot.dirId,CREATOR,MAILBOX_TYPE))
		DieWithError(CREATING_MAILBOX,err);
	GetRString(name,TRASH);
	if (FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,name,&spec))
	if (err=MakeResFile(name,MailRoot.vRef,MailRoot.dirId,CREATOR,MAILBOX_TYPE))
		DieWithError(CREATING_MAILBOX,err);
		
	if (JunkPrefNeedIntro()) JunkIntro();
	GetRString(name,JUNK);
	if (FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,name,&spec))
	{
		if (err=MakeResFile(name,MailRoot.vRef,MailRoot.dirId,CREATOR,MAILBOX_TYPE))
			DieWithError(CREATING_MAILBOX,err);
		// we made it, we don't need to warn about it
		SetPrefLong(PREF_JUNK_MAILBOX,GetPrefLong(PREF_JUNK_MAILBOX)|bJunkPrefBoxExistWarning);
	}
	else if (FSpIsItAFolder(&spec) || JunkPrefBoxExistWarning())
		PreexistingJunkWarning(&spec);

	GetRString(name,ALIAS_FILE);
	if (FSMakeFSSpec(Root.vRef,Root.dirId,name,&spec))
	if (err=MakeResFile(name,Root.vRef,Root.dirId,CREATOR,'TEXT'))
		DieWithError(CREATING_ALIAS,err);
	if (HasFeature (featureNicknameWatching)) 
	{
		GetRString(name,CACHE_ALIAS_FILE);
		if (FSMakeFSSpec(Root.vRef,Root.dirId,name,&spec))
			(void) MakeResFile(name,Root.vRef,Root.dirId,CREATOR,'TEXT');	// No error check, we just won't cache addresses
	}
#ifdef VCARD
	if (IsVCardAvailable ()) {
		GetRString(name,PERSONAL_ALIAS_FILE);
		if (FSMakeFSSpec(Root.vRef,Root.dirId,name,&spec))
		if (err=MakeResFile(name,Root.vRef,Root.dirId,CREATOR,'TEXT'))
			DieWithError(CREATING_PERSONAL_ALIAS,err);
	}
#endif
	
	GetRString(name,LINK_HISTORY_FILE);
	if (FSMakeFSSpec(Root.vRef,Root.dirId,name,&spec))
	if (err=MakeResFile(name,Root.vRef,Root.dirId,CREATOR,'TEXT'))
		DieWithError(CREATING_LINK_HISTORY,err);
}

/**********************************************************************
 * GetBoxLines - read the horizontal widths of the various mailbox
 * data areas.
 **********************************************************************/
void GetBoxLines(void)
{
	Handle strn;
	int count;
	Str255 scratch;
	long rightSide;
	short last=0;
	short offset=0;
	short i;
	
	strn=GetResource_('STR#',BoxLinesStrn);
	if (strn==nil) DieWithError(GENERAL,ResError());
	
	count = *((short *)*strn);
	if (count<BoxLinesLimit-1)
	{
		RemoveResource(strn); strn = nil;
		strn = GetResource_('STR#',BoxLinesStrn);
		count = *((short *)*strn);
		SetPref(TOC_INVERSION_MATRIX,"");
	}
	
	if (!BoxWidths) BoxWidths = NuHandle(count*sizeof(short));
	if (BoxWidths==nil) DieWithError(MEM_ERR,MemError());
	
	for(i=0;i<count;i++)
	{
		StringToNum(GetRString(scratch,BoxLinesStrn+i+1),&rightSide);
		(*BoxWidths)[i] = rightSide;
	}
	
	BoxInversionSetup();
}


/************************************************************************
 * SetupInsurancePort - create a grafport for use when others are unavailable
 ************************************************************************/
void SetupInsurancePort(void)
{
	if (!InsurancePort)
	{
		MyCreateNewPort(InsurancePort);
		if (!InsurancePort) DieWithError(MEM_ERR,MemError());
	}
	SetPort_(InsurancePort);
	TextFont(FontID);
	TextSize(FontSize);
}

/************************************************************************
 * ModernSystem - is the system 7 or better?
 ************************************************************************/
Boolean ModernSystem(void)
{
	long resp;
	long sysVers;

	HasHelp = !Gestalt(gestaltHelpMgrAttr,&resp) &&
										resp&(1L<<gestaltHelpMgrPresent);
	HasPM = !Gestalt(gestaltOSAttr,&resp) && resp&(1L<<gestaltLaunchControl);
	if (HasPM && !Gestalt(gestaltAUXVersion, &resp))
		HasPM = (resp>>8)>=3;
	if (Gestalt(gestaltComponentMgr,&resp)) DieWithError(NEED_COMPONENT_MGR,0);
	return !Gestalt(gestaltSystemVersion, &sysVers) ? sysVers >= 0x0700 : false;
}

void RememberOpenWindows(void)
{
	DejaVuHandle dvh;
	DejaVu dv;
	short ind;
	WindowPtr theWindow;
	
	if (!SettingsRefN) return;
	else UseResFile(SettingsRefN);
	
	/*
	 * out with the old
	 */
	SetResLoad(False);
	while (dvh=(DejaVuHandle)Get1IndResource(DEJA_VU_TYPE,1))
	{
		SetResLoad(True); RemoveResource((Handle)dvh); SetResLoad(False);
		DisposeHandle((Handle)dvh);
	}
	SetResLoad(True);
	
	/*
	 * save each window
	 */
	ind = 1000;
	for (theWindow = GetWindowList (); theWindow; theWindow = GetNextWindow (theWindow))
		if (IsKnownWindowMyWindow(theWindow) && IsWindowVisible(theWindow))
		if (RememberWindow(theWindow,&dv))
		if (dvh=NuHandle(sizeof(dv)))
		{
			BMD(&dv,*dvh,sizeof(dv));
					AddMyResource_(dvh,DEJA_VU_TYPE,++ind,"");
					ReleaseResource_(dvh);
		}
	UpdateResFile(SettingsRefN);
	WashMe = false;
}


/************************************************************************
 * RecallOpenWindows - reopen windows there when we quit
 ************************************************************************/
void RecallOpenWindows(void)
{
	short ind;
	DejaVuHandle dvh;
	DejaVu dv;
	WindowPtr	theWindow;
	
#ifdef TWO
	if (DBSplash != nil)
	{
		DisposDialog_(DBSplash);	/* splash screen */
		DBSplash = nil;
	}
	FlushEvents(everyEvent,0);
#endif
	
	UseResFile(SettingsRefN);
	
	for (ind=1000+Count1Resources(DEJA_VU_TYPE);!EjectBuckaroo && ind>1000;ind--)
	{
		CycleBalls();
		UseResFile(SettingsRefN);
		if (dvh=(DejaVuHandle)Get1Resource(DEJA_VU_TYPE,ind))
		{
			ActivateMyWindow(FrontWindow(),False);
			dv = **dvh;
			if (GetHandleSize_(dvh)<sizeof(dv)) dv.id = 0;
			ReleaseResource_(dvh);
			RecallWindow(&dv);
		}
		MonitorGrow(True);
		FloatingWinIdle();	// fix any layering problems we might have
	}

	for (theWindow = GetWindowList (); theWindow; theWindow = GetNextWindow (theWindow))
	{
		Rect	rContent;
		Point o;	
		
		SetPort(GetWindowPort(theWindow));
		GetContentRgnBounds(theWindow,&rContent);
		o.h = o.v = 0;
		GlobalToLocal(&o);
		OffsetRect(&rContent,o.h,o.v);
		InvalWindowRect(theWindow,&rContent);
		if (IsKnownWindowMyWindow(theWindow)) UpdateMyWindow(theWindow);
	}

	if (IsAdwareMode()) OpenAdWindow();
}

/************************************************************************
 * FolderSniffer - look for a folder in various places, and give a shout
 *  for each one found.  Similar to OpenPlugins, but general-purpose.
 *  OpenPlugins left alone for now, for backward compatibility
 ************************************************************************/
void FolderSniffer(short subfolderID,FolderSnifferFunc sniffer,uLong refCon)
{
	Str31 subfolderName;
	
	GetRString(subfolderName,subfolderID);
	FolderSnifferStr(subfolderName,sniffer,refCon);
}

void FolderSnifferStr(PStr subfolderName,FolderSnifferFunc sniffer,uLong refCon)
{
	FSSpec spec;
	Str31 name;
	FSSpec appSpec;
	OSErr err;

	/*
	 * in folder with Eudora
	 */
	if (!GetFileByRef(AppResFile,&spec))
	{
		appSpec = spec;
		
		// right next to Eudora
		if (!FSMakeFSSpec(spec.vRefNum,spec.parID,subfolderName,&spec))
		{
			IsAlias(&spec,&spec);
			spec.parID = SpecDirId(&spec);
			sniffer(spec.vRefNum,spec.parID,refCon);
		}
	
		// subfolder of Eudora Stuff folder
		if (!FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,STUFF_FOLDER),&spec))
		{
			IsAlias(&spec,&spec);
			if (!(err=FSMakeFSSpec(spec.vRefNum,SpecDirId(&spec),subfolderName,&spec)))
			{
				IsAlias(&spec,&spec);
				spec.parID = SpecDirId(&spec);
				sniffer(spec.vRefNum,spec.parID,refCon);
			}
		}
		
		/*
		 * if we're packaged, in Eudora Stuff folder up and over
		 */
		if (!(err=ParentSpec(&appSpec,&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,PACKAGE_MACOS_FOLDER),&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,STUFF_FOLDER),&spec)))
		{
			IsAlias(&spec,&spec);
			if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,subfolderName,&spec)))
			{
				IsAlias(&spec,&spec);
				spec.parID = SpecDirId(&spec);
				sniffer(spec.vRefNum,spec.parID,refCon);
			}
		}
	}
	
	/*
	 * various application support folders/Eudora
	 */
	{
		long domains[]={kUserDomain,kLocalDomain,kSystemDomain,kNetworkDomain};
		FSSpec spec, oldSpec;
		short n = sizeof(domains)/sizeof(domains[0]);
		
		Zero(oldSpec);
		while (n-->0)
			if (!FindSubFolderSpec(domains[n],kApplicationSupportFolderType,EUDORA_EUDORA,false,&spec))
			if (!SameSpec(&spec,&oldSpec))
			{
				oldSpec = spec;
				if (!SubFolderSpecOfStr(&spec,subfolderName,false,&spec))
					sniffer(spec.vRefNum,spec.parID,refCon);
			}
	}
}

/************************************************************************
 * OpenPlugIns - open extra Eudora resource files
 ************************************************************************/
void OpenPlugIns(void)
{
	FSSpec spec;
	Str31 name;
	short vRef;
	long dirId;
	FSSpec appSpec;
	OSErr err;

	/*
	 * in folder with Eudora
	 */
	if (!GetFileByRef(AppResFile,&spec))
	{
		appSpec = spec;
		
		PlugInDir(spec.vRefNum,spec.parID);
	
		/*
		 * in Eudora Stuff folder
		 */
		if (!FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,STUFF_FOLDER),&spec))
		{
			IsAlias(&spec,&spec);
			spec.parID = SpecDirId(&spec);
			PlugInDir(spec.vRefNum,spec.parID);
		}
		
		/*
		 * if we're packaged, in plugins folder up and over
		 */
		if (!(err=ParentSpec(&appSpec,&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,PACKAGE_MACOS_FOLDER),&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,PACKAGE_PLUGINS_FOLDER),&spec)))
		{
			IsAlias(&spec,&spec);
			spec.parID = SpecDirId(&spec);
			PlugInDir(spec.vRefNum,spec.parID);
		}
	}

	/*
	 * in system prefs folder
	 */
  if (PrefsPlugIns && !FindFolder(kOnSystemDisk,kPreferencesFolderType,False,&vRef,&dirId))
		PrefPlugEnd = PlugInDir(vRef,dirId);
	
	/*
	 * various application support folders/Eudora
	 */
	{
		long domains[]={kUserDomain,kLocalDomain,kSystemDomain,kNetworkDomain};
		FSSpec spec, oldSpec;
		short n = sizeof(domains)/sizeof(domains[0]);
		
		Zero(oldSpec);
		while (n-->0)
			if (!FindSubFolderSpec(domains[n],kApplicationSupportFolderType,EUDORA_EUDORA,false,&spec))
			if (!SameSpec(&spec,&oldSpec))
			{
				oldSpec = spec;
				if (!SubFolderSpecOf(&spec,PACKAGE_PLUGINS_FOLDER,false,&spec))
					PlugInDir(spec.vRefNum,spec.parID);
			}
	}
}

/************************************************************************
 * PlugInDir - open plug-ins in a directory
 ************************************************************************/
short PlugInDir(short vRef,long dirId)
{
	CInfoPBRec hfi;
	Str31 name;
	short refN;
	short lastRefN = CurResFile();
	FSSpec spec;

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while(!DirIterate(vRef,dirId,&hfi))
		if ((hfi.hFileInfo.ioFlFndrInfo.fdType==PLUG_TYPE ||
				 hfi.hFileInfo.ioFlFndrInfo.fdType==HELP_TYPE) &&
				hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			FSMakeFSSpec(vRef,dirId,name,&spec);
			ResolveAliasNoMount(&spec,&spec,nil);
			if ((refN=FSpOpenResFile(&spec, fsRdPerm))==-1)
				FileSystemError(OPEN_SETTINGS,name,ResError());
			else
			{
				lastRefN = refN;
				if (hfi.hFileInfo.ioFlFndrInfo.fdType==HELP_TYPE && !HelpResFile) HelpResFile = refN;
			}
		}
	return(lastRefN);
}

/************************************************************************
 * AwaitStartupAE - wait for our startup AE.  If it doesn't come, die.
 ************************************************************************/
void AwaitStartupAE(void)
{
	EventRecord event;
	uLong giveUp = TickCount() + 60*GetRLong(AE_INITIAL_TIMEOUT_SECS);
	
	/*
	 * wait for the AE handlers to work their magic, and find us a folder
	 * if they don't, just use the standard folder
	 */
	while (!SettingsRefN && TickCount()<giveUp)
	{
		//LOGLINE;
		if (WaitNextEvent(highLevelEventMask,&event,10,nil))
		{
			//LOGLINE;
			(void) AEProcessAppleEvent(&event);
			if (!SettingsRefN) SystemEudoraFolder();
		}
		else
			CyclePendulum();
	}
	//LOGLINE;
	
	/*
	 * did we get setup?  If not, open the standard folder
	 */
	if (!SettingsRefN) SystemEudoraFolder();
}

/************************************************************************
 * InitIntScript - make and use a local copy of 'itl2' in case system resource can't be loaded later
 ************************************************************************/
static void InitIntScript(void)
{
	Handle	hIntl0,hMyIntl0;
	GrafPtr	port;
	short		saveFont;
	OSErr err = noErr;
		
	//	Make sure we're using system script by setting textfont of current port to 0
	GetPort(&port);
	if (!port)
	{
		//	Oops, nothing set. Use WMgrPort
		GetWMgrPort(&port);
		SetPort(port);
	}
	saveFont = GetPortTextFont(port);
	TextFont(0);
	
	if ((hIntl0 = GetIntlResource(2)) &&
		(hMyIntl0 = GetResource('itl2',kMyIntl0)))
	{
		char saveState = HGetState(hIntl0);
		
		HNoPurge(hIntl0);
		HLock(hIntl0);
		SetHandleSize(hMyIntl0,0);
		if (!HandAndHand(hIntl0, hMyIntl0))
		{
			ChangedResource(hMyIntl0);	//	Resource is protected so it won't get written
		}
		else err = MemError();
		HSetState(hIntl0,saveState);
	}
	else err = ResError();
	TextFont(saveFont);
	if (err) DieWithError(INTERNATIONAL_FAILURE,err);
}

/************************************************************************
 * PrefillSettings - process initial settings from a text file
 ************************************************************************/
static void PrefillSettings(void)
{
	FSSpec	spec;
	Handle	text;
	Str255 s;
	Ptr	queryPtr;
	long	queryLen;

	if (!FindMyFile(&spec,kStuffFolderBit,SETTINGS_PREFILL_FILE))
	if (!Snarf(&spec,&text,0))
	{
		Ptr	p = LDRef(text);	
		while (*p)
		{
			//	Line must start with <x-eudora-setting
			if (*p=='<')
			{
				Ptr	pStart = ++p;
				while (*p && *p!='>' && *p!=0x0d && *p!=0x0a) p++;
				if (*p=='>')
				{
					if (!(ParseURLPtr(pStart,p-pStart,s,nil,&queryPtr,&queryLen)))
					if (FindSTRNIndex(ProtocolStrn,s) == proSetting)
					{
						Ptr	spot = queryPtr;
						if (PTokenPtr(queryPtr,queryLen,s,&spot,"="))
						{
							long num;
							StringToNum(s,&num);
							if (PTokenPtr(queryPtr,queryLen,s,&spot,""))
								SetPref(num,s);
						}
					}
				}
			}
			//	Next line
			while (*p && *p!=0x0d && *p!=0x0a) p++;
			while (*p && (*p==0x0d || *p==0x0a)) p++;
		}
		
		//	Rename file so we don't process it again
		FSpRename(&spec,GetRString(s,SETTINGS_PREFILL_PROCESSED));	
	}
}
