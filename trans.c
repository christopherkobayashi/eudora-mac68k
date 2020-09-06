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

#include "trans.h"
#include "tlapi-MacGlue.h"

#define FILE_NUM 75
/* Copyright (c) 1995 by QUALCOMM Incorporated */
#ifdef TWO


#pragma segment TRANSLATOR

#if TARGET_RT_MAC_CFM
#define SIMPLE_UPP(routine,what)	\
routine##Type routine;\
RoutineDescriptor routine##RD = BUILD_ROUTINE_DESCRIPTOR(upp##what##ProcInfo,routine);\
what routine##UPP=(what)&routine##RD
#else
#define SIMPLE_UPP(routine,what) \
routine##Type routine;\
what##UPP routine##UPP = (void*)&routine
#endif



#ifdef ETL

#ifdef DEBUG
//#define DEBUGFILES		//	Create debug files
#endif
TLMHandle TLSelList, TLQ4List, TLAttachList, TLSpecialList, TLSettingList, TLModelessList, TLImporterList;

#define tlInstance void*

typedef struct
{
	tlInstance	instance;
	tlStringHandle moduleDescription;
	short moduleID;
	Handle moduleSuite;
	short	version;	// Used to determine what plugin supports (like parameter-block style functions version 3+)
	long	idleTimeFreq;
	long	lastIdleTime;
	long	pluginKey;
	short	numMBoxContext;
	short	tCount;
	ModeTypeEnum modeNeeded;
	Boolean	downgradeWasBeingUsed;
} TLFuncList, *TLFLPtr, **TLFLHandle;
TLFLHandle TLFL;

typedef tlStringHandle **tlAddrListHandle;

//	Keeps track of signature bar offsets within the file
enum	{ kMaxSignList = 16 };
typedef struct
{
	short	count;
	long	begin[kMaxSignList];
	long	end[kMaxSignList];
} SignOffsetList;
SignOffsetList	*gpSignList;

void ETLAddAppleItem(void);
void ETLFillMailConfig(emsMailConfigP mailConfig, emsCallBacksP callBacks, Boolean useOldMailConfig);
void ETLEmptyMailConfig(emsMailConfigP mailConfig);
OSErr PlainTextMIMEStuff(emsMIMEHandle *emsMIME,Handle *headers);
OSErr ETLListRequestedTranslators(TLMHandle *translators, TransInfoHandle hRequested);
short ETLError(tlStringHandle description,short context,tlStringHandle errorStr,OSErr err,long tlErr);
short ETLOpenModules(AccuPtr instances);
void DisposeTLMIMEParam(emsMIMEParamHandle param);
int ETLCompare(TLMPtr t1,TLMPtr t2);
void ETLSwap(TLMPtr t1,TLMPtr t2);
void ETLSortTranslators(TLMHandle translators);
OSErr ETLOutputHandle(UHandle text,short refN,AccuPtr a);
OSErr ETLOutputPtr(UPtr text,long size,short refN,AccuPtr a);
#define ETLOutputStr(s,r,a)	ETLOutputPtr((s)+1,*(s),(r),(a))
OSErr ETLGetTLMIME(FSSpecPtr spec,emsMIMEHandle *tlMIME);
OSErr ETLCopyFile(FSSpecPtr spec,short refN,Boolean display,HeaderDHandle *theHeaders);
OSErr ETLReadTLMIME(FSSpecPtr spec,emsMIMEHandle *tlMIME);
OSErr ETLBuildOldAddrList(MessHandle messH,tlAddrListHandle *addrList,Boolean expandNicks);
static OSErr ETLTranslate(TLMHandle translators,short context,emsMIMEHandle emsMIME,FSSpecPtr source,emsMIMEHandle *emsMIMEOut,tlStringHandle *textOut,FSSpecPtr into,tlStringHandle *errorStr,long *errCode,emsHeaderDataP addrList);
static OSErr ETLTransOutLow(TLMHandle transList,emsHeaderDataP addrList,emsMIMEHandle tlMIME,FSSpecPtr from,FSSpecPtr to,MessHandle messH);
static void DisposeOldAddrList(tlAddrListHandle addrList);
OSErr ETLTransHandleBuf(UHandle *text,TLMPtr tlm);
static OSErr ETLTransHandleFile(UHandle *text,TLMPtr tlm,PETEHandle pte);
void ETLAboutButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
void ETLAboutUpdate(MyWindowPtr win);
Boolean ETLAboutClose(MyWindowPtr win);
Boolean ETLKey(MyWindowPtr win, EventRecord *event);
void ETLAboutClick(MyWindowPtr win, EventRecord *event);
static emsAddressH GetAddressesFromHandle(Uhandle text,Boolean expandNicks);
static emsAddressH FindAddress(Accumulator acc, short index,Boolean expandNicks);
OSErr ETLAddVolatileMenus(void);

#define SetSize(x)	(x).size = sizeof(x);
static OSErr GetTransIcon(TLFLPtr tlfl,long id,Handle *suite);
static OSErr CanTranslateFile(TLFLPtr pTLFL,long context,short trans_id,
    emsMIMEHandle in_mime,StringPtr **out_error,
    long *out_code, StringPtr *queuedProperties, emsHeaderDataP header, MessHandle messH);
static OSErr MyTranslateFile(TLFLPtr pTLFL,long context,short trans_id,
    emsMIMEHandle in_mime,FSSpec *in_file,emsMIMEHandle *out_mime,
    FSSpec *out_file,Handle *out_icon,StringPtr **out_desc,StringPtr **out_error,
    long *out_code, StringHandle queuedProperties, emsHeaderDataP header, MessHandle messH, Handle translatorName);
static tlMIMEtypeP DegradeMIME(emsMIMEHandle inMIME);
static emsMIMEHandle UpgradeMIME(tlMIMEtypeP inMIME);
static void DisposeOldTLMIME(tlMIMEtypeP tlMIME);
static void DisposeOldTLMIMEParam(tlMIMEParamP param);
static tlMIMEParamP DegradeMIMEParam(emsMIMEparamH hParam);
static OSErr NewMIME(emsMIMEHandle *tlMIME,short mimeType,short subType);
static StringHandle GetQueuedProperties(MessHandle messH,long id);
static emsAddressH MakeAddress(StringPtr sAddress,StringPtr sName);
static void ZapAddress(emsAddressH address);
static Boolean IsSystemComponent(Component comp);
static OSErr QueuedProperties(short which, long *selected, StringHandle *pProperties,Boolean fDefaultOn);
static void GetContextIconRect(Rect *rContentBox, Rect *rIcon);
static Boolean PluginIDToIndex(long key,short *index);
static void CheckAddToToolbar(TLMHandle hTLM, MenuHandle hMenu);
static emsAddressH GetAddressesFromHeader(UHandle textIn,short header,Boolean expandNicks);

#define ContextRequiresExpansion(context)	(((context)&(EMSF_Q4_COMPLETION+EMSF_Q4_TRANSMISSION))!=0)
//	Global variables
short	gIdleTimeCount;		//	# of plug-ins that want idle time
long	gPluginDrain;

unsigned long	gTranslatorFlags;		//	All flags set by any translator
	//	List of components in System Folder. So we don't unregister
	//	any components installed a system startup time.
Component	**ghSystemComponents;

//	Callback function declarations and UPP's
typedef pascal short ETLProgressType(short percent);
SIMPLE_UPP(ETLProgress,tlProgress);	//	v1
typedef pascal short ETLProgress3Type(emsProgressDataP progData);
SIMPLE_UPP(ETLProgress3,emsProgress);	//	v3
typedef pascal void ETLGetMailboxType(emsGetMailBoxDataP getMailBoxData); 
SIMPLE_UPP(ETLGetMailbox,emsGetMailBox);	//	v4
typedef pascal void ETLSetMailBoxTagType(emsSetMailBoxTagDataP setMailBoxTagData); 
SIMPLE_UPP(ETLSetMailBoxTag,emsSetMailBoxTag);
typedef pascal void ETLGetMailBoxTagType(emsSetMailBoxTagDataP setMailBoxTagData); 
SIMPLE_UPP(ETLGetMailBoxTag,emsGetMailBoxTag);	// v5
typedef pascal short ETLGetPersonalityType(emsGetPersonalityDataP getPersonalityData); 
SIMPLE_UPP(ETLGetPersonality,emsGetPersonality);
typedef pascal short ETLGetPersonalityInfoType(emsGetPersonalityInfoDataP getPersonalityData); 
SIMPLE_UPP(ETLGetPersonalityInfo,emsGetPersonalityInfo);
typedef pascal void ETLRegenerateType(emsRegenerateDataP RegenerateData); 
SIMPLE_UPP(ETLRegenerate,emsRegenerate);
typedef pascal short ETLGetDirectoryType(emsGetDirectoryDataP GetDirectoryData); 
SIMPLE_UPP(ETLGetDirectory,emsGetDirectory);
typedef pascal void ETLUpdateWindowsType(void); 
SIMPLE_UPP(ETLUpdateWindows,emsUpdateWindows);

typedef pascal short ETLPlugwindowRegisterType(emsPlugwindowDataP wd); 
SIMPLE_UPP(ETLPlugwindowRegister,emsPlugwindowRegister);
typedef pascal void ETLPlugwindowUnRegisterType(emsPlugwindowDataP wd); 
SIMPLE_UPP(ETLPlugwindowUnRegister,emsPlugwindowUnRegister);
typedef pascal void ETLGDeviceRgnType(emsGDeviceRgnDataP wd); 
SIMPLE_UPP(ETLGDeviceRgn,emsGDeviceRgn);
typedef pascal void ETLSelectWindowType(WindowPtr wp); 
SIMPLE_UPP(ETLSelectWindow,emsSelectWindow);
typedef pascal WindowPtr ETLFrontWindowType(void); 
SIMPLE_UPP(ETLFrontWindow,emsFrontWindow);

typedef pascal short ETLMakeSigType(emsMakeSigDataP makeSigData);
SIMPLE_UPP(ETLMakeSig,emsMakeSig);	//	v6
typedef pascal short ETLMakeAddressBookType(emsMakeAddressBookDataP makeSigData);
SIMPLE_UPP(ETLMakeAddressBook,emsMakeAddressBook);
typedef pascal short ETLMakeABEntryType(emsMakeABEntryDataP makeABEntryData);
SIMPLE_UPP(ETLMakeABEntry,emsMakeABEntry);
typedef pascal short ETLMakeMailboxType(emsMakeMailboxDataP makeMailboxData);
SIMPLE_UPP(ETLMakeMailbox,emsMakeMailbox);
typedef pascal short ETLMakeOutMessType(emsMakeOutMessDataP makeOutMessData);
SIMPLE_UPP(ETLMakeOutMess,emsMakeOutMess);
typedef pascal short ETLMakeMessageType(emsMakeMessageDataP MakeMessageData);
SIMPLE_UPP(ETLMakeMessage,emsMakeMessage);
typedef pascal short ETLCreateMailBoxType(emsCreateMailBoxDataP createMailboxData);
SIMPLE_UPP(ETLCreateMailBox,emsCreateMailBox);
typedef pascal short ETLIsInAddressBookType(emsIsInAddressBookDataP isInAddressBookData);
SIMPLE_UPP(ETLIsInAddressBook,emsIsInAddressBook);
typedef pascal short ETLImportMboxType(emsImportMboxDataP importMBoxData);
SIMPLE_UPP(ETLImportMbox,emsImportMbox);

#define TO_UPP_OR_NOT_TO_UPP(proc) (HaveOSX() ? proc : proc##UPP)

// Use parameter-block style functions? (version 3+)
#define UseParameterBlockStyleFunctions(version) (version >= EMS_PB_VERSION)

// Use old mail config? (versions before 7)
#define UseOldMailConfig(version) (version < EMS_NEW_MAIL_CONFIG_VERSION)

/**********************************************************************
 * Functions for managing modules
 **********************************************************************/

	// carbon build looks for 'ecTL' type plugins when running under OS X
	#define TL_COMPONENT_TYPE (HaveOSX() ? 'ecTL' : 'euTL')

/**********************************************************************
 * ETLOpenModules - open & count the translator modules
 *	Returns:
 *		# of translator modules
 **********************************************************************/
short ETLOpenModules(AccuPtr instances)
{
	ComponentDescription looking;
	Component index=nil;
	Component lastComponent=nil;
	tlInstance instance;
	Boolean retry;
	
	if (!AccuInit(instances))
	{
#ifdef DEBUG
#ifdef NO_EMSAPI_INIT
		if (RunType!=Debugging)
#endif
#endif
		do
		{
			Zero(looking);
			looking.componentType = TL_COMPONENT_TYPE;
			retry = False;
			if (index = FindNextComponent(index, &looking))
			{
				if (instance=OpenComponent(index))
				{
					lastComponent = index;
					AccuAddPtr(instances,(void*)&instance,sizeof(instance));
				}
				else
				{
					ASSERT(0);
					UnregisterComponent(index);
					index = lastComponent;
					retry = True;
				}
			}
		}
		while(retry || index);
		return(instances->offset/sizeof(instance));
	}
	return(0);
}

/**********************************************************************
 * ETLGetSystemPlugins - get plugins that were already installed
 *	and record so we don't unregister them on cleanup
 **********************************************************************/
void ETLGetSystemPlugins(void)
{
	ComponentDescription looking;
	Component index=nil;
	
	do
	{
		Zero(looking);
		looking.componentType = TL_COMPONENT_TYPE;
		if (index = FindNextComponent(index, &looking))
		{
			//	Need to add this to system component list
			if (!ghSystemComponents) ghSystemComponents=NuHandle(0);
			if (ghSystemComponents)
				PtrAndHand(&index, (Handle)ghSystemComponents, sizeof(index));
		}
	}
	while(index);
}

/**********************************************************************
 * ETLDrain - how much memory do the plug-ins use?
 **********************************************************************/
long ETLDrain(void)
{
	return gPluginDrain;
}

/**********************************************************************
 * Functions for individual modules
 **********************************************************************/
 ModeTypeEnum GetCurrentPayMode () {
 	if ( CanPayMode ())		return EMS_ModePaid;
 	if ( IsAdwareMode ())	return EMS_ModeSponsored;
 	if ( IsFreeMode ())		return EMS_ModeFree;
 	ASSERT ( false );
 	return EMS_ModeFree;
 	}
 
/**********************************************************************
 * ETLInit - global translator initialization
 **********************************************************************/
OSErr ETLInit()
{
	short m, mCount;
	short t;
	short selItem=0,specItem=SPECIAL_BAR4_ITEM,attItem=0,setItem=0;
	OSErr err = noErr;
	Str255 s;
	//Handle nHandle;
	MenuHandle selMH = GetMHandle(TLATE_SEL_HIER_MENU);
	MenuHandle setMH = GetMHandle(TLATE_SET_HIER_MENU);
	MenuHandle specMH = GetMHandle(SPECIAL_MENU);
	MenuHandle attMH = GetMHandle(TLATE_ATT_HIER_MENU);
	TLMaster tlm;
	TLFuncList tlfl;
	Handle icon=nil;
	//Boolean anIcon;
	//short version;
	Boolean menu;
	Accumulator instances;
	short realCount=0;
	emsMIMEHandle tlMIME=nil;
	OSErr looseErr;
	struct emsMenuS emsMenuStuff;
	emsPluginInfo pluginInfo;
					
	gPluginDrain = 0;
	
	if (TLSelList) {ETLAddAppleItem(); return(ETLAddVolatileMenus());}

	TLSelList = NuHandle(0);
	TLQ4List = NuHandle(0);
	TLSpecialList = NuHandle(0);
	TLAttachList = NuHandle(0);
	TLSettingList = NuHandle(0);
	TLModelessList = NuHandle(0);
	TLImporterList = NuHandle(0);
	TLFL = NuHandle(0);
	Zero(instances);
	NewMIME(&tlMIME,MIME_TEXT,MIME_PLAIN);
	gTranslatorFlags = 0;
	
	if (TLFL && TLModelessList && TLSelList && TLQ4List && TLSpecialList && TLAttachList && TLSettingList && TLImporterList &&
			selMH && setMH && attMH && specMH && (mCount=ETLOpenModules(&instances)))
	{
		for (m=0;m<mCount;m++)
		{
			long	drain,free,mem_rqmnt;

			err = 0;
			mem_rqmnt = 0;
			free = FreeMem();
			Zero(tlfl);
			tlfl.instance = (*(ComponentInstance**)instances.data)[m];
#if defined(NEVER)
			if (err = ems_plugin_version(tlfl.instance,&version))
			{
				//	Older, version 1, plugin which doesn't have the version function. Version 3+ MUST implement the version function
				version = 1;
				err = 0;
			}
#endif
			tlfl.version = GetComponentVersion(tlfl.instance)>>16;
			if (!err)
			{
				Zero(pluginInfo);
				if (UseParameterBlockStyleFunctions(tlfl.version))
				{
					Zero(pluginInfo);
					SetSize(pluginInfo);
					if (UseOldMailConfig(tlfl.version))
					{
						emsPre7MailConfig mailConfig;
						ETLFillMailConfig(&mailConfig, nil, true);
						MightSwitch();
						err = ems_plugin_init(tlfl.instance,EMS_VERSION,&mailConfig,&pluginInfo);
						AfterSwitch();
						ETLEmptyMailConfig(&mailConfig);
					}
					else
					{
						emsCallBack callbacks;
						emsMailConfig mailConfig;
						ETLFillMailConfig(&mailConfig, &callbacks, false);
						MightSwitch();
						err = ems_plugin_init(tlfl.instance,EMS_VERSION,&mailConfig,&pluginInfo);
						AfterSwitch();
						ETLEmptyMailConfig(&mailConfig);
					}
					tlfl.moduleDescription = pluginInfo.desc;
					tlfl.moduleID = pluginInfo.id;
					tlfl.moduleSuite = pluginInfo.icon;
					tlfl.numMBoxContext = pluginInfo.numMBoxContext;
					if (tlfl.idleTimeFreq = pluginInfo.idleTimeFreq)
					{
						tlfl.lastIdleTime = (TickCount()*50)/3;	//	(ticks / 60 * 1000) Roughly in milliseconds
						gIdleTimeCount++;
					}
					tlfl.tCount = pluginInfo.numTrans;
					mem_rqmnt = pluginInfo.mem_rqmnt;
				}
				else
				{
					MightSwitch();
					err = tl_module_init(tlfl.instance,&tlfl.tCount,&tlfl.moduleDescription,&tlfl.moduleID,&tlfl.moduleSuite);
					AfterSwitch();
				}
				
			}
			
			// Make sure we get a name
			if (!tlfl.moduleDescription)
			{
				ComponentDescription cd;
				if (tlfl.moduleDescription = NuHandle(0))
				{
					GetComponentInfo(tlfl.instance,&cd,tlfl.moduleDescription,nil,nil);
					if (!GetHandleSize(tlfl.moduleDescription)) ZapHandle(tlfl.moduleDescription);
				}
			}
			
			// Ugly hack time
			if (tlfl.moduleID==29 || tlfl.moduleID==31) tlfl.modeNeeded = EMS_ModePaid;
			
			//if (!err && version!=TL_VERSION) ETLError(moduleDescription,ETL_BAD_VERSION,nil,version,0);
			//else
			if (err)
			{
				ETLError(tlfl.moduleDescription,ETL_CANT_INIT,nil,err,0);
				UnregisterComponent(tlfl.instance);
			}
			else if (!(err = PtrPlusHand_(&tlfl,TLFL,sizeof(tlfl))))
			{
				if (ComponentFunctionImplemented(tlfl.instance,kems_plugin_configRtn))
				{
					tlm.menuItem = ++setItem;							
					tlm.module = realCount;
					
					PtrPlusHand_(&tlm,TLSettingList,sizeof(tlm));
					PCopy(s,*tlfl.moduleDescription);
					PCat(s,"\p...");
					MyAppendMenu(setMH,s);
				}
				
				// translators
				for (t=0;t<tlfl.tCount;t++)
				{
					Zero(tlm);
					
					// v3
					if (UseParameterBlockStyleFunctions(tlfl.version))
					{
						emsTranslator transInfo;
						
						Zero(transInfo);
						SetSize(transInfo);
						transInfo.id = t+1;
						MightSwitch();
						err = ems_translator_info(tlfl.instance,&transInfo);
						AfterSwitch();
						tlm.type = transInfo.type;
						tlm.flags = transInfo.flags;
						tlm.nameHandle = transInfo.desc;
						tlm.suite = transInfo.icon;
					}
					
					// v1
					else
					{
						MightSwitch();
						err = tl_translator_info(tlfl.instance,t+1,&tlm.type,nil,&tlm.flags,&tlm.nameHandle,&tlm.suite);
						AfterSwitch();
					}
					
					
					if (!err)
					{
						gTranslatorFlags |= tlm.flags;
						menu = (tlm.flags & TLF_ON_REQUEST);
						if (menu) tlm.menuItem = ++selItem;
						tlm.module = realCount;
						tlm.id = t+1;
						err = PtrPlusHand_(&tlm,TLSelList,sizeof(tlm));
						if (err) ZapHandle(tlm.nameHandle);
						if (tlm.suite && (tlm.flags&(EMSF_Q4_TRANSMISSION)) &&
								(tlm.flags&TLF_WHOLE_MESSAGE))
							PtrPlusHand_(&tlm,TLQ4List,sizeof(tlm));
						if (!ComponentFunctionImplemented(tlfl.instance,kems_plugwindow_closeRtn))
							PtrPlusHand_(&tlm,TLModelessList,sizeof(tlm));
						if (!LooseTrans)
						{
							looseErr = CanTranslateFile(&tlfl,TLF_ON_ARRIVAL,tlm.id,tlMIME,
																			    nil,nil,nil,nil,nil);
							LooseTrans = looseErr==TLR_NOW || looseErr==TLR_NOT_NOW;
						}
					}
				}
				
				
				// attachers
				for (t=0;t<pluginInfo.numAttachers;t++)
				{
					Zero(emsMenuStuff); SetSize(emsMenuStuff);
					emsMenuStuff.id = t;
					if (!(err=!ComponentFunctionImplemented(tlfl.instance,kems_attacher_infoRtn)))
					{
						MightSwitch();
						err=ems_attacher_info(tlfl.instance,&emsMenuStuff);
						AfterSwitch();
						if (!err)
						{
							if (emsMenuStuff.desc)
							{
								Zero(tlm);
								tlm.id = t;
								tlm.module = realCount;
								tlm.menuItem = ++attItem;
								tlm.nameHandle = emsMenuStuff.desc;
								tlm.flags = emsMenuStuff.flags;
								PtrPlusHand_(&tlm,TLAttachList,sizeof(tlm));
								MyAppendMenu(attMH,PCopy(s,*tlm.nameHandle));
							}
							if (emsMenuStuff.icon)
								DisposeIconSuite(emsMenuStuff.icon,true);
						}
					}
				}
				
				// specials
				for (t=0;t<pluginInfo.numSpecials;t++)
				{
					Zero(emsMenuStuff); SetSize(emsMenuStuff);
					emsMenuStuff.id = t;
					if (!(err=!ComponentFunctionImplemented(tlfl.instance,kems_special_infoRtn)))
					{
						MightSwitch();
						err=ems_special_info(tlfl.instance,&emsMenuStuff);
						AfterSwitch();
						if (!err)
						{
							if (emsMenuStuff.desc)
							{
								Zero(tlm);
								tlm.id = t;
								tlm.module = realCount;
								tlm.menuItem = ++specItem;							
								tlm.nameHandle = emsMenuStuff.desc;						
								tlm.flags = emsMenuStuff.flags;
								PtrPlusHand_(&tlm,TLSpecialList,sizeof(tlm));
							}
							if (emsMenuStuff.icon)
								DisposeIconSuite(emsMenuStuff.icon,true);
						}
					}
				}
				
				// search for any importers
				for (t=0;(t<pluginInfo.numImporters);t++)
				{
					emsImporter importerInfo;
						
					Zero(importerInfo);
					SetSize(importerInfo);
					importerInfo.id = t+1;
					if (!(err=!ComponentFunctionImplemented(tlfl.instance,kems_importer_infoRtn)))
					{	
						MightSwitch();
						err=ems_importer_info(tlfl.instance,&importerInfo);
						AfterSwitch();
						if (!err)
						{
							if (importerInfo.desc)
							{
								Zero(tlm);
								tlm.id = t;
								tlm.module = realCount;							
								tlm.nameHandle = importerInfo.desc;						
								tlm.flags = importerInfo.flags;
								tlm.suite = importerInfo.icon;
								PtrPlusHand_(&tlm,TLImporterList,sizeof(tlm));
							}
							gImportersAvailable = true;	// remember that at least one importer has been found
						}
					}
				}
				
				err = 0;
				
				realCount++;
			}
			
			//	Calculate memory used by this plug-in. Use the greater of: 
			//	  1) actualy memory used, 2) memory use as reported by the plug-in
			drain = free - FreeMem();
			gPluginDrain += drain>mem_rqmnt ? drain : mem_rqmnt;
		}
		
		//	Notify all plugins that care of the current mode
		ETLEudoraModeNotification(EMS_ModeChanged, GetCurrentPayMode ());
	}
	GetRString(s,NO_TRANSLATORS);
#define NO_TRANS(x)	do{if (!x##Item) {MyAppendMenu(x##MH,s);SetItemStyle(x##MH,1,italic);DisableItem(x##MH,1);}}while(0)
	NO_TRANS(sel);
	NO_TRANS(att);
	if (realCount) GetRString(s,NO_TRANSLATORS_WITH_SETTINGS);
	NO_TRANS(set);
	ETLAddVolatileMenus();
	AccuZap(instances);
	ZapTLMIME(tlMIME);
	if (!realCount) {ZapHandle(TLFL);ZapHandle(TLSelList);ZapHandle(TLQ4List);ZapHandle(TLSpecialList);ZapHandle(TLAttachList);ZapHandle(TLImporterList);}
	else ETLAddAppleItem();
	return(realCount ? noErr : fnfErr);
}

/************************************************************************
 * ETLAddToToolbar - add any translators, attachers or special items
 *	to the toolbar that are requesting it
 ************************************************************************/
static void CheckAddToToolbar(TLMHandle hTLM, MenuHandle hMenu)
{
	short		i,n;
	Str255	s;
	short oldRefN = CurResFile();

	typedef struct TLMaster
	{
		short moduleID;
		short	menuID;
		Str32	itemText;
	} TBIconList, *TBIconListPtr, **TBIconListHandle;
	
	if (hMenu && hTLM && *hTLM && (n=HandleCount(hTLM)))
	{
	
		for (i=0;i<n;i++)
		{
			if ((*hTLM)[i].flags & EMSF_TOOLBAR_PRESENCE)
			{
				//	Put this arrogant translator in the toolbar
				Boolean	doAdd;
				TBIconListHandle	hAddedList;
				TBIconListPtr	pAddedList;
				short	menuID = GetMenuID(hMenu);
				short moduleID = (*TLFL)[(*hTLM)[i].module].moduleID;

				//	Make sure we haven't added it before
				doAdd = true;
				PCopy(s,*(*hTLM)[i].nameHandle);
				if (*s>31) *s=31;
				UseResFile(SettingsRefN);
				if (hAddedList = (TBIconListHandle)Get1Resource(TOOLBAR_ICON_RTYPE,TOOLBAR_ICON_ID))
				{
					short	count;
					
					for(pAddedList=*hAddedList,count=HandleCount(hAddedList);count--;pAddedList++)
					{
						if (pAddedList->moduleID==moduleID && pAddedList->menuID==menuID && StringSame(pAddedList->itemText,s))
						{
							//	We've previously added this one. Once is enough!
							doAdd = false;
							break;
						}
					}
				}
				
				if (doAdd && TBAddMenuButton(menuID,(*hTLM)[i].menuItem,s))
				{
					TBIconList	iconListItem;
					
					//	Add to list of items already added
					Zero(iconListItem);
					iconListItem.moduleID = moduleID;
					iconListItem.menuID = menuID;
					PCopy(iconListItem.itemText,s);
					if (!hAddedList)
					{
						//	Need to create new resource
						hAddedList = NuHandle(0);
						AddResource_(hAddedList,TOOLBAR_ICON_RTYPE,TOOLBAR_ICON_ID,"");
					}
					PtrAndHand(&iconListItem,(Handle)hAddedList,sizeof(iconListItem));
					ChangedResource((Handle)hAddedList);				
				}
			}
		}
	}
	UseResFile(oldRefN);
}

/************************************************************************
 * ETLAddToToolbar - add any translators, attachers or special items
 *	to the toolbar that are requesting it
 ************************************************************************/
void ETLAddToToolbar(void)
{
	CheckAddToToolbar(TLSelList,GetMHandle(TLATE_SEL_HIER_MENU));
	CheckAddToToolbar(TLAttachList,GetMHandle(TLATE_ATT_HIER_MENU));
	CheckAddToToolbar(TLSpecialList,GetMHandle(SPECIAL_MENU));
}


/************************************************************************
 * ETLAddVolatileMenus - add trans items to menus that need to be refreshed
 * when switching settings files
 ************************************************************************/
OSErr ETLAddVolatileMenus(void)
{
	MenuHandle specMH = GetMHandle(SPECIAL_MENU);
	MenuHandle selMH = GetMHandle(TLATE_SEL_HIER_MENU);
	short i,n;
	Str255	s;
	
	if (specMH && TLSpecialList && *TLSpecialList && (n=HandleCount(TLSpecialList)))
	{
		AppendMenu(specMH,"\p-");
		for (i=0;i<n;i++)
			MyAppendMenu(specMH,PCopy(s,*(*TLSpecialList)[i].nameHandle));
	}
	if (selMH && TLSelList && *TLSelList && (n=HandleCount(TLSelList)))
	{
		for (i=0;i<n;i++)
			if ((*TLSelList)[i].flags & TLF_ON_REQUEST)
			  MyAppendMenu(selMH,PCopy(s,*(*TLSelList)[i].nameHandle));
	}
	return(noErr);
}


/************************************************************************
 * ETLFillMailConfig - fill a mailconfig
 ************************************************************************/
void ETLFillMailConfig(emsMailConfigP mailConfig, emsCallBacksP callBacks, Boolean useOldMailConfig)
{
	Str255 scratch;
	
	// Account for differences between old and new mail config structures
	if (useOldMailConfig)
	{
		emsPre7MailConfigP oldMailConfig = (emsPre7MailConfigP) mailConfig;
		
		Zero(*oldMailConfig);
		SetSize(*oldMailConfig);
		
		// Old mail config structure accidently had callbacks directly in the structure
		callBacks = &oldMailConfig->callBacks;
	}
	else
	{
		Zero(*mailConfig);
		SetSize(*mailConfig);
		
		// New mail config structure has pointer to callbacks so that we can have fields
		// after the callbacks.
		mailConfig->callBacks = callBacks;
		
		// For a while we had eudAPIMinorVersion in the old structure, but the location became
		// obsolete as more callbacks were added. Now we only support it in the new structure.
		mailConfig->eudAPIMinorVersion = EMS_MINOR_VERSION;
	}
	
	// From here on out we've already accounted for all the differences
	FSMakeFSSpec(ItemsFolder.vRef,ItemsFolder.dirId,GetRString(scratch,PLUGINS_FOLDER),&mailConfig->configDir);
	SetSize(mailConfig->userAddr);
	mailConfig->userAddr.address = NewString(GetReturnAddr(scratch,false));
	mailConfig->userAddr.realname = NewString(GetRealname(scratch));

	//	v4 stuff
	callBacks->EMSGetMailBoxFunction = TO_UPP_OR_NOT_TO_UPP(ETLGetMailbox);
	callBacks->EMSSetMailBoxTagFunction = TO_UPP_OR_NOT_TO_UPP(ETLSetMailBoxTag);
	callBacks->EMSGetPersonalityFunction = TO_UPP_OR_NOT_TO_UPP(ETLGetPersonality);
	callBacks->EMSProgressFunction = TO_UPP_OR_NOT_TO_UPP(ETLProgress3);
	callBacks->EMSRegenerateFunction = TO_UPP_OR_NOT_TO_UPP(ETLRegenerate);
	callBacks->EMSGetDirectoryFunction = TO_UPP_OR_NOT_TO_UPP(ETLGetDirectory);
	callBacks->EMSUpdateWindowsFunction = TO_UPP_OR_NOT_TO_UPP(ETLUpdateWindows);
	//	v5 stuff
	callBacks->EMSGetMailBoxTagFunction = TO_UPP_OR_NOT_TO_UPP(ETLGetMailBoxTag);
	callBacks->EMSGetPersonalityInfoFunction = TO_UPP_OR_NOT_TO_UPP(ETLGetPersonalityInfo);
	callBacks->EMSPlugwindowRegisterFunction = TO_UPP_OR_NOT_TO_UPP(ETLPlugwindowRegister);
	callBacks->EMSPlugwindowUnRegisterFunction = TO_UPP_OR_NOT_TO_UPP(ETLPlugwindowUnRegister);
	callBacks->EMSGDeviceRgnFunction = TO_UPP_OR_NOT_TO_UPP(ETLGDeviceRgn);
	callBacks->EMSFrontWindowFunction = TO_UPP_OR_NOT_TO_UPP(ETLFrontWindow);
	callBacks->EMSSelectWindowFunction = TO_UPP_OR_NOT_TO_UPP(ETLSelectWindow);
	// v6 stuff
	callBacks->EMSMakeSigFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeSig);
	callBacks->EMSMakeAddressBookFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeAddressBook);
	callBacks->EMSMakeABEntryFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeABEntry);
	callBacks->EMSMakeMailboxFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeMailbox);	
	callBacks->EMSMakeOutMessFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeOutMess);
	callBacks->EMSMakeMessageFunction = TO_UPP_OR_NOT_TO_UPP(ETLMakeMessage);	
	callBacks->EMSCreateMailBoxFunction = TO_UPP_OR_NOT_TO_UPP(ETLCreateMailBox);
	// v7 stuff
	callBacks->EMSIsInAddressBook = TO_UPP_OR_NOT_TO_UPP(ETLIsInAddressBook);
}


/************************************************************************
 * ETLEmptyMailConfig - empty the mailconfig
 ************************************************************************/
void ETLEmptyMailConfig(emsMailConfigP mailConfig)
{
	ZapHandle(mailConfig->userAddr.address);
	ZapHandle(mailConfig->userAddr.realname);
}

/**********************************************************************
 * 
 **********************************************************************/
void ETLAddAppleItem(void)
{
	Str255 s;
	MenuHandle mh = GetMHandle(APPLE_MENU);
	InsertMenuItem(mh,GetRString(s,ABOUT_TRANSLATORS),1);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean ETLExists(void)
{
	return(TLFL!=nil);
}

/**********************************************************************
 * ETLDoAbout - give Laurence his darn about translators window
 **********************************************************************/
void ETLDoAbout(void)
{
	short n = HandleCount(TLFL);
	MyWindowPtr win;
	WindowPtr	winWP;
	ControlHandle cntl;
	ListHandle list;
	Rect r,bounds;
	Point c;
	DECLARE_UPP(TlateListDef,ListDef);
	
	INIT_UPP(TlateListDef,ListDef);
	if (ModalWindow) return;	// already a window onscreen
	if (win=GetNewMyWindow(ETL_ABOUT_WIND,nil,nil,nil,False,False,ETL_ABOUT_WIN))
	{
		winWP = GetMyWindowWindowPtr (win);
		win->position = PositionPrefsTitle;
		win->isRunt = True;
		win->button = ETLAboutButton;
		win->update = ETLAboutUpdate;
		win->close = ETLAboutClose;
		win->click = ETLAboutClick;
		win->key = ETLKey;
		win->dontControl = True;
		MySetThemeWindowBackground(win,kThemeListViewBackgroundBrush,False);
		SetRect(&r,2*INSET,2*INSET,win->contR.right-INSET-GROW_SIZE,win->contR.bottom-20-3*INSET);
		SetRect(&bounds,0,0,1,n);
		c.v = 38;
		c.h = RectWi(r);
		if (!(list = CreateNewList(TlateListDefUPP,TLATE_LDEF,&r,&bounds,c,winWP,True,False,False,True)))
			CloseMyWindow(winWP);
		else
		{
			(*list)->indent.h = INSET;
			cntl = GetNewControl(ETL_OK_CNTL,winWP);
			SetControlReference(cntl,'OKbn');
			if (!cntl) CloseMyWindow(winWP);
			else
			{
				MoveMyCntl(cntl,win->contR.right-INSET-ControlWi(cntl),
												win->contR.bottom-INSET-ControlHi(cntl),0,0);
				SetMyWindowPrivateData (win,(long)list);
				ShowMyWindow(winWP);
				PushModalWindow(winWP);
			}
		}
	}
}

/**********************************************************************
 * ETLAboutClick - handle clicks
 **********************************************************************/
void ETLAboutClick(MyWindowPtr win, EventRecord *event)
{
	Point mouse = event->where;
	Rect listR = (*(ListHandle)GetMyWindowPrivateData(win))->rView;
	
	GlobalToLocal(&mouse);
	listR.right += GROW_SIZE;
	
	if (PtInRect(mouse,&listR)) LClick(mouse,event->modifiers,(ListHandle)GetMyWindowPrivateData(win));
	else HandleControl(mouse,win);
}


/**********************************************************************
 * ETLAboutButton - shows button
 **********************************************************************/
void ETLAboutButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
#pragma unused(button)
	WindowPtr	winWP = GetMyWindowWindowPtr (win);

	if (part==kControlButtonPart) CloseMyWindow(winWP);

	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),kControlButtonPart), mouseDown);
}

/**********************************************************************
 * ETLAboutClose - close the window
 **********************************************************************/
Boolean ETLAboutClose(MyWindowPtr win)
{
	if (GetMyWindowPrivateData(win))
	{
		LDispose((ListHandle)GetMyWindowPrivateData(win));
		SetMyWindowPrivateData(win, 0);
	}
	return(True);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean ETLKey(MyWindowPtr win, EventRecord *event)
{
#pragma unused(event)
	short c = (event->message & charCodeMask);

	if (c==enterChar || c==returnChar || c==escChar) CloseMyWindow(GetMyWindowWindowPtr(win));
	return(True);
}


/**********************************************************************
 * ETLAboutUpdate - update the window
 **********************************************************************/
void ETLAboutUpdate(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Rect r;
	ControlHandle cntl = FindControlByRefCon(win,'OKbn');

	if (GetMyWindowPrivateData(win))
	{
		r = (*(ListHandle)GetMyWindowPrivateData(win))->rView;
		r.right += GROW_SIZE;
		DrawThemeListBoxFrame(&r,kThemeStateActive);
		LUpdate(MyGetPortVisibleRegion(GetWindowPort(winWP)),(ListHandle)GetMyWindowPrivateData(win));
		if (cntl)
			OutlineControl(cntl,True);
	}
}

/**********************************************************************
 * ETLMenu2Icon - get icon suite for plug-in menu selection
 **********************************************************************/
Handle ETLMenu2Icon(short menu,short item)
{	
	short n;
	TLMaster	tlm;
	Handle	suite = nil;
	Boolean	gotModule = false;
	emsMenu	emsMenuStuff;
	OSErr		err;
	
	switch (menu)
	{
		case TLATE_SEL_HIER_MENU:
			for(n=HandleCount(TLSelList);n--;)
				if ((*TLSelList)[n].menuItem==item)
				{
					tlm = (*TLSelList)[n];
					GetTransIcon(&(*TLFL)[tlm.module],tlm.id,&suite);
					gotModule = true;
					break;
				}
			break;
		
		case SPECIAL_MENU:
		case TLATE_ATT_HIER_MENU:
			if (menu == SPECIAL_MENU)
			{
				if (item>SPECIAL_BAR4_ITEM)
				{
					tlm = (*TLSpecialList)[item-SPECIAL_BAR4_ITEM-1];
					gotModule = true;
				}
			}
			else
			{
				tlm = (*TLAttachList)[item-1];
				gotModule = true;
			}
			if (gotModule)
			{
				Zero(emsMenuStuff); SetSize(emsMenuStuff);
				emsMenuStuff.id = tlm.id;
				MightSwitch();
				if (menu == SPECIAL_MENU)
					err=ems_special_info((*TLFL)[tlm.module].instance,&emsMenuStuff);
				else
					err=ems_attacher_info((*TLFL)[tlm.module].instance,&emsMenuStuff);
				AfterSwitch();
				if (!err)
				{
					suite = emsMenuStuff.icon;
					if (emsMenuStuff.desc)
						DisposeHandle((Handle)emsMenuStuff.desc);
				}
			}
			break;
	}
	
	if (gotModule && !suite)
	{
		Handle	moduleSuite;

		//	Use module's icon
		if (moduleSuite = (*TLFL)[tlm.module].moduleSuite)	
			DupIconSuite(moduleSuite,&suite,false);
	}
	return suite;
}

/**********************************************************************
 * ETLListAllTranslators - list all the translators appropriate to a given context
 *		translators - pointer to handle to translator list; will be allocated
 *		context - the translator context (eg, tlOnArrival)
 *	Returns:
 *		TLR_CANT_TRANS - no translators for current context
 *		noErr - found at least one translator
 **********************************************************************/
OSErr ETLListAllTranslatorsLo(TLMHandle *translators, short context, ModeTypeEnum forMode)
{
	short i, n;
	
	if (!TLSelList) return(TLR_CANT_TRANS);
	
	n = HandleCount(TLSelList);
	*translators = (TLMHandle)NuHTempBetter((n+1)*sizeof(TLMaster));
	if (!*translators) return(MemError());
	
	n = 0;
	for (i=HandleCount(TLSelList);i--;)
		if (((*TLSelList)[i].flags & context) && (*TLFL)[(*TLSelList)[i].module].modeNeeded<=forMode)
		{
			(**translators)[n] = (*TLSelList)[i];
			n++;
		}
	
	if (!n)
	{
		ZapHandle(*translators);
		return(TLR_CANT_TRANS);
	}
	else
	{
		SetHandleBig_(*translators,(n+1)*sizeof(TLMaster));
		(**translators)[n].module = REAL_BIG;
		(**translators)[n].result = REAL_BIG;
		return(noErr);
	}
}

/************************************************************************
 * ETLCountTranslators - count the translators that work in a context & current mode
 ************************************************************************/
OSErr ETLCountTranslatorsLo(short context,ModeTypeEnum forMode)
{
	TLMHandle tList = nil;
	short count = 0;
	
	(void) ETLListAllTranslatorsLo ( &tList, context, forMode );
	if (tList)
	{
		count = HandleCount(tList)-1;
		count = MAX(count,0);
		ZapHandle(tList);
	}
	return count;
}


/**********************************************************************
 * ETLRemoveDeadTranslators - remove translators that have been used or
 *  will never be used
 *		translators - handle to list of translators
 *	Returns:
 *		TLR_CANT_TRANS - if no live translators remain
 **********************************************************************/
OSErr ETLRemoveDeadTranslators(TLMHandle translators)
{
	short n = HandleCount(translators);
	short from, to;
	
	for (from=to=0;from<n;from++)
		switch((*translators)[from].result)
		{
			case TLR_NOT_NOW:
			case TLR_NOW:
				if (to!=from) (*translators)[to] = (*translators)[from];
				to++;
				break;
		}
	if (to<n) SetHandleBig_(translators,to*sizeof(TLMaster));
	return to > 0 ? noErr : TLR_CANT_TRANS;
}

/**********************************************************************
 * ETLSortTranslators - sort translators by module and group
 *		translators - handle to translator list to sort
 **********************************************************************/
void ETLSortTranslators(TLMHandle translators)
{
	short n = HandleCount(translators);
	
	QuickSort((void*)LDRef(translators),sizeof(TLMaster),0,n-2,(void*)ETLCompare,(void*)ETLSwap);
	UL(translators);
}

/**********************************************************************
 * ETLCompare - compare two translators
 **********************************************************************/
int ETLCompare(TLMPtr t1,TLMPtr t2)
{
	long diff;
	
	if (diff=(t1->result-t2->result)) return(diff);
	if (diff=(t1->type-t2->type)) return(diff);
	if (diff=(t1->module-t2->module)) return(diff);
	return(0);
}

/**********************************************************************
 * ETLSwap - swap two translators
 **********************************************************************/
void ETLSwap(TLMPtr t1,TLMPtr t2)
{
	TLMaster t;
	
	t = *t1;
	*t1 = *t2;
	*t2 = t;
}

/**********************************************************************
 * ETLListRequestedTranslators - make a translator list from the messages
 **********************************************************************/
OSErr ETLListRequestedTranslators(TLMHandle *translators, TransInfoHandle hRequested)
{
	short i, n, index;
	//Boolean canDo;
	short nReq;
	
	if (!TLSelList && !hRequested) return(TLR_CANT_TRANS);
	
	nReq = HandleCount(hRequested);
	*translators = (TLMHandle)NuHTempBetter((nReq+1)*sizeof(TLMaster));
	if (!*translators) return(MemError());
	
	n = 0;
	for (i=0;i<nReq;i++)
	{
		index = ETLIDToIndex((*hRequested)[i].id);
		if (index<0)
		{
			WarnUser(ETL_CANT_FIND_TRANS,0);
			n = 0;
			break;
		}
		(**translators)[n] = (*TLQ4List)[index];
		n++;
	}
	
	if (!n)
	{
		ZapHandle(*translators);
		return(TLR_CANT_TRANS);
	}
	else
	{
		SetHandleBig_(*translators,(n+1)*sizeof(TLMaster));
		(**translators)[n].module = REAL_BIG;
		(**translators)[n].result = REAL_BIG;
		
		ETLSortTranslators(*translators);
		
		for (i=0;i<n-1;i++)
		{
			if ((**translators)[i].type==TLT_SIGNATURE && (**translators)[i+1].type==TLT_PREPROCESS)
			{
				short bigI = HandleCount(TLSelList);
				while (bigI--)
				{
					if ((*TLSelList)[bigI].module==(**translators)[i].module &&
							(*TLSelList)[bigI].type==TLT_COALESCED)
					{
						(**translators)[i] = (*TLSelList)[bigI];
						for (i++;i<n;i++) (**translators)[i] = (**translators)[i+1];
						SetHandleBig_(*translators,n*sizeof(TLMaster));
						return(noErr);
					}
				}
			}
		}
		return(noErr);
	}
}


/**********************************************************************
 * ETLOutputHandle - output a handle to a file or accumulator (or nowhere)
 **********************************************************************/
OSErr ETLOutputHandle(UHandle text,short refN,AccuPtr a)
{
	OSErr err=noErr;
	
	if (refN)
	{
		long size = GetHandleSize(text);
		err = AWrite(refN,&size,LDRef(text));
		UL(text);
	}
	else if (a) err = AccuAddHandle(a,text);
	return(err);
}
	
/**********************************************************************
 * ETLOutputPtr - output a pointer to a file or accumulator (or nowhere)
 **********************************************************************/
OSErr ETLOutputPtr(UPtr text,long size,short refN,AccuPtr a)
{
	OSErr err = noErr;
	long locSize = size;
	
	if (refN) err = AWrite(refN,&locSize,text);
	else if (a) err = AccuAddPtr(a,text,size);
	
	return(err);
}

/**********************************************************************
 * progress callbacks
 **********************************************************************/
 
// support ems api 1.0
static pascal short ETLProgress(short percent)
{
	PushGWorld();
	SetPort_(InsurancePort);
	CycleBalls();
	if (percent!=-1) Progress(percent,NoChange,nil,nil,nil);
	PopGWorld();
	return(CommandPeriod);
}

// support ems api 3.0
static pascal short ETLProgress3(emsProgressDataP progData)
{
short percent = -1;
PStr message = nil;
	
	if (progData) {
		percent = progData->value;
		message = progData->message;
	}
	PushGWorld();
	SetPort_(InsurancePort);
	CycleBalls();
	if (percent!=-1) Progress(percent,NoChange,nil,nil,message);
	PopGWorld();
	return(CommandPeriod);
}

/**********************************************************************
 * ETLGetMailbox - allow user to select a mailbox
 **********************************************************************/
static pascal void ETLGetMailbox(emsGetMailBoxDataP pData)
{
	FSSpec	spec;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	
	pData->mailbox = nil;
	if (ChooseMailboxLo(MAILBOX_MENU,pData->prompt,&spec,!(pData->flags&EMSFGETMBOX_ALLOW_NEW),!(pData->flags&EMSFGETMBOX_ALLOW_OTHER),(pData->flags&EMSFGETMBOX_DISALLOW_NON_LOCAL)))
	{
		//	Return alias to mailbox
		FSSpec	mailFolderSpec;
	//	Alias is relative to mail folder
		FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &mailFolderSpec);
		NewAlias(&mailFolderSpec, &spec, &pData->mailbox);
	}

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/**********************************************************************
 * ETLGetMailBoxTag - get tag ID for a mailbox
 **********************************************************************/
static pascal void ETLGetMailBoxTag(emsSetMailBoxTagDataP pData)
{
	FSSpec	spec,mailFolderSpec;
	TOCHandle tocH;
	Boolean		wasChanged;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	
	pData->oldkey = pData->oldvalue = 0;
	
	//	Alias is relative to mail folder
	FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &mailFolderSpec);
	if (!ResolveAlias(&mailFolderSpec, pData->mailbox, &spec, &wasChanged))
	{
		//	get key from mailbox's TOC
		if (!GetTOCByFSS(&spec,&tocH))
		{
			pData->oldkey = (*tocH)->pluginKey;
			pData->oldvalue = (*tocH)->pluginValue;
		}
	}

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/**********************************************************************
 * ETLSetMailBoxTag - set tag ID for a mailbox
 **********************************************************************/
static pascal void ETLSetMailBoxTag(emsSetMailBoxTagDataP pData)
{
	FSSpec	spec,mailFolderSpec;
	TOCHandle tocH;
	Boolean		wasChanged;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	
	//	Alias is relative to mail folder
	FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &mailFolderSpec);
	if (!ResolveAlias(&mailFolderSpec, pData->mailbox, &spec, &wasChanged))
	{

		//	Add key to mailbox's TOC
		if (!GetTOCByFSS(&spec,&tocH))
		{
			if (pData->size>=sizeof(*pData))
				pData->oldkey = (*tocH)->pluginKey;
			(*tocH)->pluginKey = pData->key;
			pData->oldvalue = (*tocH)->pluginValue;
			(*tocH)->pluginValue = pData->value;
			TOCSetDirty(tocH,true);
			if ((*tocH)->win)
			{
				Rect	rOldSize;
				GetPortBounds(GetMyWindowCGrafPtr((*tocH)->win),&rOldSize);
				MyWindowDidResize((*tocH)->win,&rOldSize);
			}
		}
	}

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/************************************************************************
 * ETLPlugwindowRegister - tell Eudora that a window belongs to a plug-in
 ************************************************************************/
pascal short ETLPlugwindowRegister(emsPlugwindowDataP wd)
{
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	{
		WindowPtr theWindow = wd->nativeWindow;
		
		emsPlugwindowDataH wdh = NewH(emsPlugwindowData);
		if (!wdh) return(MemError());
		**wdh = *wd;
		SetWRefCon(theWindow,(long)wdh);
		SetWindowKind(theWindow,EMS_PW_WINDOWKIND);
		
		if (IsModalPlugwindow(theWindow))
			PushModalWindow(theWindow);
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	
	return(noErr);
}

/************************************************************************
 * ETLPlugwindowUnRegister - tell Eudora that a window is dying
 ************************************************************************/
pascal void ETLPlugwindowUnRegister(emsPlugwindowDataP wd)
{
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	{
		WindowPtr theWindow = wd->nativeWindow;
		Handle		theHandle = (Handle) GetWRefCon (theWindow);
		
		if (theHandle != nil)
		{
			//	Double check before calling PopModalWindow so that we're
			//	not at the mercy of a broken plugin.
			if (IsModalPlugwindow(theWindow) && (theWindow == ModalWindow))
				PopModalWindow();
			
			ZapHandle(theHandle);
			SetWRefCon(theWindow,nil);
		}
		
#ifdef	FLOAT_WIN
			if (theWindow == 	lastHilitedWinWP) 	lastHilitedWinWP = 0;
#endif	//FLOAT_WIN
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/************************************************************************
 * ETLGDeviceRgn - give plug-in device rgn
 ************************************************************************/
pascal void ETLGDeviceRgn(emsGDeviceRgnDataP gdr)
{
	GDHandle gd;
	Rect r;
	Boolean hasMB = true;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	
	GetQDGlobalsScreenBitsBounds(&r);
	utl_GetIndGD(gdr->gdIndex, &gd, &r, &hasMB);

	InsetRect(&r,2,4);
	r.left += GetRLong(DESK_LEFT_STRIP);
	r.right -= GetRLong(DESK_RIGHT_STRIP);
	r.bottom -= GetRLong(DESK_BOTTOM_STRIP);
	r.top += GetRLong(DESK_TOP_STRIP);
	if (hasMB)
		r.top += utl_GetMBarHeight();
	RectRgn(gdr->usableRgn,&r);
	gdr->largestRectangle = r;
	if (ToolbarSect(&r))
	{
		ToolbarReduce(&r);
		gdr->largestRectangle = r;
		ToolbarRect(&r);
		RgnMinusRect(gdr->usableRgn,&r);
	}

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/**********************************************************************
 * ETLFrontWindow - return the front window as Eudora thinks of it
 **********************************************************************/
pascal WindowPtr ETLFrontWindow(void)
{
	return MyFrontNonFloatingWindow();
}

/**********************************************************************
 * ETLSelectWindow - SelectWindow our way
 **********************************************************************/
pascal void ETLSelectWindow(WindowPtr wp)
{
	if (wp) SelectWindow_(wp);
}

/**********************************************************************
 * GetContextIconRect - get rect of context menu icon
 **********************************************************************/
static void GetContextIconRect(Rect *rContentBox, Rect *rIcon)
{
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	*rIcon = *rContentBox;
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}


/**********************************************************************
 * PluginIDToIndex - find plug-in by module id
 **********************************************************************/
static Boolean PluginIDToIndex(long key,short *index)
{
	short	i;
	
	if (TLFL && key)
	{
		for (i=HandleCount(TLFL);i--;)
		{
			if ((*TLFL)[i].moduleID==key)
			{
				*index = i;
				return true;
			}
		}
	}
	return false;
}

/**********************************************************************
 * ETLDrawBoxTag - draw context menu if mailbox has plug-in tag
 **********************************************************************/
void ETLDrawBoxTag(TOCHandle tocH,Rect *r)
{
	short	idx;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	
	if (PluginIDToIndex((*tocH)->pluginKey,&idx))
	{
		Rect	rIcon;
		IconTransformType	transform = ttNone;
		MyWindowPtr	win = (*tocH)->win;

		GetContextIconRect(r, &rIcon);				
		//FrameRect(&rIcon);
		if (win && !win->isActive) transform = ttDisabled;
		PlotIconSuite(&rIcon,atNone,transform,(*TLFL)[idx].moduleSuite);
	}

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/**********************************************************************
 * ETLBoxTagWidth - return width of context menu icon
 **********************************************************************/
short ETLBoxTagWidth(MyWindowPtr win)
{	
	short	width = 0;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	{
		TOCHandle tocH = (TOCHandle) GetMyWindowPrivateData(win);
		
		if (tocH && (*tocH)->pluginKey)
			width = 16;
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return width;
}

/**********************************************************************
 * ETLAddBoxButtons - add the buttons to a peanut mailbox
 **********************************************************************/
void ETLAddBoxButtons(TOCHandle tocH)
{
	WindowPtr	winWP = GetMyWindowWindowPtr((*tocH)->win);
	short idx;
	short item;
	
	if (PluginIDToIndex((*tocH)->pluginKey,&idx) && (*tocH)->win->topMarginCntl)
	{
		tlInstance	instance = (*TLFL)[idx].instance;
		emsMenu	menuInfo;
		Str63 title;
		ControlHandle cntl;
		Rect r;

		SetRect(&r,-200,-200,50-200,13-200);
		for(item=1;item<=(*TLFL)[idx].numMBoxContext;item++)
		{
			SetSize(menuInfo);
			menuInfo.id = item;
			menuInfo.desc = nil;
			MightSwitch();
			ems_mbox_context_info(instance,&menuInfo);
			AfterSwitch();
			PCopy(title,(*menuInfo.desc));
			ZapHandle(menuInfo.desc);
			cntl = NewControlSmall(winWP,&r,title,true,0,0,0,0,'plug'+item);
			EmbedControl(cntl,(*tocH)->win->topMarginCntl);
			ButtonFit(cntl);
		}
	}
}

/**********************************************************************
 * ETLButtonHit - the user clicked a plug-in button
 **********************************************************************/
void ETLButtonHit(MyWindowPtr win,short item)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short idx;
	
	if (tocH && PluginIDToIndex((*tocH)->pluginKey,&idx))
	{
		tlInstance	instance = (*TLFL)[idx].instance;
		emsMenu	menuInfo;
		FSSpec	mailFolderSpec,spec;
		AliasHandle	mailbox=nil;
				
		spec = GetMailboxSpec(tocH,-1);
		FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &mailFolderSpec);
		NewAlias(&mailFolderSpec, &spec, &mailbox);
		SetSize(menuInfo);
		menuInfo.id = item;
		menuInfo.desc = nil;				
		MightSwitch();
		ems_mbox_context_hook(instance,mailbox,&menuInfo);
		AfterSwitch();
		ZapHandle(mailbox);
		
		AuditHit(false, false, false, false, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),ETL_CONTEXT_POPUP_MENU), mouseDown);
	}
}

#if 0 // Not used anymore
/**********************************************************************
 * ETLClickContextMenu - check for click in plug-in context menu
 **********************************************************************/
Boolean ETLClickContextMenu(MyWindowPtr win,Point pt,Rect *rContentBox)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Rect	rIcon;
	Boolean	wasContextMenu = false;
	TOCHandle tocH;
	short	idx;
	Str255	s;
	
	GetContextIconRect(rContentBox, &rIcon);
	if (PtInRect(pt,&rIcon) && 
		(tocH = (TOCHandle)GetMyWindowPrivateData(win)) && 
		PluginIDToIndex((*tocH)->pluginKey,&idx))
	{
		//	Click on context menu icon
		tlInstance	instance = (*TLFL)[idx].instance;
		MenuHandle	mh = NewMenu(ETL_CONTEXT_POPUP_MENU,"");
		
		if (mh)
		{
			emsMenu	menuInfo;
			short	item,sel;
			
			InsertMenu(mh,-1);
			MightSwitch();
			for(item=1;item<=(*TLFL)[idx].numMBoxContext;item++)
			{
				SetSize(menuInfo);
				menuInfo.id = item;
				menuInfo.desc = nil;
				ems_mbox_context_info(instance,&menuInfo);
				MyAppendMenu(mh,PCopy(s,(*menuInfo.desc)));
			}
			AfterSwitch();

			pt.v = rIcon.top;	//	Display menu at top/right of icon
			pt.h = rIcon.right;	//	Display menu at top/right of icon
			LocalToGlobal(&pt);
			if (sel = AFPopUpMenuSelect(mh,pt.v,pt.h-2*MenuWidth(mh)/3,0))
			{
				FSSpec	mailFolderSpec,spec;
				AliasHandle	mailbox=nil;
				
				spec = GetMailboxSpec(tocH,-1);
				FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &mailFolderSpec);
				NewAlias(&mailFolderSpec, &spec, &mailbox);
				SetSize(menuInfo);
				menuInfo.id = sel;
				menuInfo.desc = nil;				
				MightSwitch();
				ems_mbox_context_hook(instance,mailbox,&menuInfo);
				AfterSwitch();
				ZapHandle(mailbox);
			}
			DeleteMenu(GetMenuID(mh));
			DisposeMenu(mh);
		}
		wasContextMenu = true;
		
		AuditHit(false, false, false, false, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),ETL_CONTEXT_POPUP_MENU), mouseDown);
	}
	return wasContextMenu;
}
#endif

/**********************************************************************
 * ETLHasMBoxContextFolder - Does the plugin have ems_mbox_context_folder?
 **********************************************************************/
Boolean ETLHasMBoxContextFolder(MyWindowPtr win)
{
	TOCHandle	tocH = (TOCHandle) GetMyWindowPrivateData(win);
	short		idx;
		
	return ( tocH && (*tocH)->pluginKey && PluginIDToIndex((*tocH)->pluginKey, &idx) &&
			 ComponentFunctionImplemented((*TLFL)[idx].instance, kems_mbox_context_folderRtn) );
}

/**********************************************************************
 * ETLMBoxContextFolder - Get the plugin folder, if any
 **********************************************************************/
short ETLMBoxContextFolder(MyWindowPtr win, short * vRefNum, long * dirID)
{
	short		result = EMSR_UNKNOWN_FAIL;
	TOCHandle	tocH = (TOCHandle) GetMyWindowPrivateData(win);
	short		idx;
		
	if ( tocH && (*tocH)->pluginKey && PluginIDToIndex((*tocH)->pluginKey, &idx) )
	{
		tlInstance	instance = (*TLFL)[idx].instance;
		if ( ComponentFunctionImplemented(instance, kems_mbox_context_folderRtn) )
		{
			emsMailBoxContextFolderData		contextFolderData;
			
			Zero(contextFolderData);
			SetSize(contextFolderData);
			
			contextFolderData.value = (*tocH)->pluginValue;
			
			result = ems_mbox_context_folder(instance, &contextFolderData);
			
			if (vRefNum)
				*vRefNum = contextFolderData.pluginFolder.vRefNum;
				
			if (dirID)
				*dirID = SpecDirId(&contextFolderData.pluginFolder);
		}
	}
	
	return result;
}

/**********************************************************************
 * ETLGetPersonality - allow user to select a personality
 **********************************************************************/
static pascal short ETLGetPersonality(emsGetPersonalityDataP pData)
{
	short		result = EMSR_OK;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	{
		Str255	scratch;
		PersHandle oldCur = CurPers;
		long oldSize = pData->size;
		
		Zero(pData->personality);
		SetSize(pData->personality);

		if (CurPers = pData->defaultPers ? PersList : PersChoose(pData->prompt))
		{
			//	Return address info from personality
			GetReturnAddr(scratch,false);
			if (scratch[*scratch]=='>') --*scratch;
			if (*scratch && scratch[1]=='<')
			{
				--*scratch;
				BMD(scratch+2,scratch+1,*scratch);
			}
			pData->personality.address = NewString(scratch);
			pData->personality.realname = NewString(GetPref(scratch,PREF_REALNAME));
			pData->persCount = PersCount();
			if (oldSize>(char*)&pData->personalityName-(char*)pData)
				pData->personalityName = NewString(PCopy(scratch,(*CurPers)->name));
		}
		else
			result = EMSR_USER_CANCELLED;
		
		CurPers = oldCur;
		
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return result;
}

/**********************************************************************
 * ETLGetPersonalityInfo - allow user to select a personality
 **********************************************************************/
static pascal short ETLGetPersonalityInfo(emsGetPersonalityInfoDataP pData)
{
	short		result = EMSR_OK;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	{
		Str255	scratch;
		PersHandle oldCur = CurPers;
		long oldSize = pData->size;
		
		Zero(pData->personality);
		SetSize(pData->personality);

		PCopy(scratch,*pData->personalityName);
		if (CurPers = FindPersByName(scratch))
		{
			//	Return address info from personality
			GetReturnAddr(scratch,false);
			if (scratch[*scratch]=='>') --*scratch;
			if (*scratch && scratch[1]=='<')
			{
				--*scratch;
				BMD(scratch+2,scratch+1,*scratch);
			}
			pData->personality.address = NewString(scratch);
			pData->personality.realname = NewString(GetPref(scratch,PREF_REALNAME));
		}
		else
			result = EMSR_NO_ENTRY;
		
		CurPers = oldCur;
		
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return result;
}
/**********************************************************************
 * ETLRegenerate - regenerate plugin filters or nicknames
 **********************************************************************/
static pascal void ETLRegenerate(emsRegenerateDataP pData)
{
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	switch (pData->which)
	{
		case emsRegenerateFilters:
			DisposeFilters(&PreFilters);
			DisposeFilters(&PostFilters);
			GeneratePluginFilters();
			break;
		
		case emsRegenerateNicknames:
			ZapPluginAliases();
			ReadPluginNickFiles(true);
			break;
	}
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
}

/**********************************************************************
 * ETLGetDirectory - return indicated folder spec
 **********************************************************************/
static pascal short ETLGetDirectory(emsGetDirectoryDataP pData)
{
	short		result = EMSR_OK;
	OSErr		err = noErr;
	
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
		
	switch (pData->which)
	{
		case EMS_EudoraDir:						err = FSMakeFSSpec(Root.vRef,Root.dirId,"",&pData->directory); break;
		case EMS_AttachmentsDir:			err = GetAttFolderSpec(&pData->directory); break;
		case EMS_PluginFiltersDir:		err = ETLGetPluginFolderSpec(&pData->directory,PLUGIN_FILTERS); break;
		case EMS_PluginNicknamesDir:	err = ETLGetPluginFolderSpec(&pData->directory,PLUGIN_NICKNAMES); break;
		case EMS_ConfigDir:						err = ETLGetPluginFolderSpec(&pData->directory,0); break;
		case EMS_MailDir:							err = FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,"",&pData->directory); break;
		case EMS_NicknamesDir:				err = SubFolderSpec(NICK_FOLDER,&pData->directory); break;
		case EMS_SignaturesDir:				err = SubFolderSpec(SIG_FOLDER,&pData->directory); break;
		case EMS_SpoolDir:						err = SubFolderSpec(SPOOL_FOLDER,&pData->directory); break;
		case EMS_StationeryDir:				err = SubFolderSpec(STATION_FOLDER,&pData->directory); break;
		default:	result = EMSR_INVALID; break;
	}
	if (err)
		result = EMSR_UNKNOWN_FAIL;

	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	
	return result;
}

/**********************************************************************
 * ETLUpdateWindows - handle Eudora window updates
 **********************************************************************/
static pascal void ETLUpdateWindows(void)
{
	UpdateAllWindows();
}

/**********************************************************************
 * ETLCanTranslate - can any of the translators take a whack?
 **********************************************************************/
OSErr ETLCanTranslate(TLMHandle translators,short context,emsMIMEHandle tlMIME,tlStringHandle *errorStr,long *errCode,emsHeaderDataP addrList,HeaderDHandle hdh)
{
	short t;
	short nT;
	OSErr err;
	FSSpec spec;
	OSErr result = TLR_CANT_TRANS;
	short group = REAL_BIG;
	Boolean	fGeneratedHeaders = false;
	emsHeaderData list;
	
	Zero(spec);	// just to keep the compiler quiet
	
	if (!TLSelList) return(TLR_CANT_TRANS);
	
	ETLSortTranslators(translators);
	
	nT = HandleCount(translators)-1;
	
	if (hdh && !addrList)
	{
		//	Generate header info from header desc
		ETLBuildAddrList(nil,nil,hdh,&list,context);
		addrList = &list;
		fGeneratedHeaders = true;
	}
	
	for (t=0;t<nT;t++)
	{
		// call can_translate
		err = CanTranslateFile(&(*TLFL)[(*translators)[t].module],context,(*translators)[t].id,tlMIME,
											    errorStr,errCode, nil, addrList, nil);
		if (!err) err = TLR_NOW;
		ZapHandle(errorStr);
		
		// record error
		(*translators)[t].result = err;

		// if they're willing, I'm willing!
		if (err==TLR_NOT_NOW || err==TLR_NOW)
		{
			result = err;
			break;
		}
	}
	
	if (fGeneratedHeaders)
		ETLDisposeAddrList(addrList);
		
	return(result);
}

/**********************************************************************
 * ETLTranslate - actually call the translators
 **********************************************************************/
OSErr ETLTranslate(TLMHandle translators,short context,emsMIMEHandle tlMIME,
	FSSpecPtr source,emsMIMEHandle *tlMIMEOut,tlStringHandle *textOut,FSSpecPtr into,
	tlStringHandle *errorStr,long *errCode,emsHeaderDataP addrList)
{
	short n = HandleCount(translators)-1;
	short nGood=0;
	OSErr err;
	emsMIMEHandle localTLMIME=nil;
	long localErr;
#ifdef DEBUG
#ifdef DEBUGFILES
	static short dbCount;
	Str255 dbStr;
	FInfo info;
	FSSpec temp;
	
	if (RunType==Debugging)
	{
		FSMakeFSSpec(Root.vRef,Root.dirId,ComposeString(dbStr,"\pETInput %d",++dbCount),&temp);
		FSpDupFile(&temp,source,True,False);
		FSpGetFInfo(&temp,&info);
		info.fdType = 'TEXT';
		info.fdCreator = 'MPS ';
		FSpSetFInfo(&temp,&info);
	}
#endif
#endif
	
	if (!errCode) errCode = &localErr;
	
	/*
	 * set up the translator vector
	 */
	while(n--)
	{
		if ((*translators)[n].result == TLR_NOW) break;
	}
	if ((*translators)[n].result != TLR_NOW) return(TLR_CANT_TRANS);
	
	/*
	 * call the translator
	 */
	err = MyTranslateFile (&(*TLFL)[(*translators)[n].module], context, (*translators)[n].id, tlMIME,
										    source, &localTLMIME, into, nil, textOut, errorStr, errCode ,nil,
										    addrList, nil, (*translators)[n].nameHandle);

	
	if (err==TLR_ABORTED) err = userCanceledErr;
	
	if (err && err!=userCanceledErr && errorStr && *errorStr) ETLError((*TLFL)[(*translators)[n].module].moduleDescription,ETL_CANT_TRANSLATE,*errorStr,err,*errCode);
	//ZapHandle(errorStr);
	
	if (err==EMSR_DATA_UNCHANGED || err==TLR_NOT_NOW)
	{
		RecordTLID(source,ETLID(translators,n));
		(*translators)[n].result = TLR_NOT_NOW;
	}
	
	/*
	 * record output
	 */
	if (!err && localTLMIME)
	{
		RecordTLMIME(into,localTLMIME);
		if (tlMIMEOut)
		{
			*tlMIMEOut = localTLMIME;
			localTLMIME = nil;
		}
	}
	
	if (localTLMIME) ZapTLMIME(localTLMIME);
	
	return(err);
}

/**********************************************************************v
 * NewTLMIME - Allocate a tlMIME structure
 **********************************************************************/
OSErr NewTLMIME(emsMIMEHandle *tlMIME)
{
	if (*tlMIME = NewZH(struct emsMIMEtypeS))
		(**tlMIME)->size = sizeof(struct emsMIMEtypeS);
	return(MemError());
}

/**********************************************************************v
 * NewMIME - Allocate a MIME structure and add some defaults
 **********************************************************************/
static OSErr NewMIME(emsMIMEHandle *tlMIME,short mimeType,short subType)
{
	OSErr	err = NewTLMIME(tlMIME);
	
	if (!err)
	{
		emsMIMEtypeP	pMIME;
		
		pMIME = LDRef(*tlMIME);
		
		//	Set up some defaults
		if (mimeType)
			GetRString(pMIME->mimeType,mimeType);
		if (subType)
			GetRString(pMIME->subType,subType);
		GetRString(pMIME->mimeVersion,MIME_VERSION);	//	Default MIME version is 1.0
		UL(*tlMIME);
	}
	return err;
}

/**********************************************************************
 * DisposeTLMIME - dispose of an emsMIME structure
 **********************************************************************/
void DisposeTLMIME(emsMIMEHandle tlMIME)
{
	if (tlMIME)
	{
		if (*tlMIME)
		{
			DisposeTLMIMEParam((*tlMIME)->params);
			DisposeTLMIMEParam((*tlMIME)->contentParams);
		}
		ZapHandle(tlMIME);
	}
}

/**********************************************************************
 * DisposeTLMIMEParam - dispose of an emsMIME parameter list
 **********************************************************************/
void DisposeTLMIMEParam(emsMIMEParamHandle param)
{
	if (param)
	{
		if (*param)
		{
			if ((*param)->next) DisposeTLMIMEParam((*param)->next);
			ZapHandle((*param)->value);
		}
		ZapHandle(param);
	}
}

/**********************************************************************
 * DisposeTLMIME - dispose of an old tlMIME structure
 **********************************************************************/
static void DisposeOldTLMIME(tlMIMEtypeP tlMIME)
{
	if (tlMIME)
	{
		if (*tlMIME) DisposeOldTLMIMEParam((*tlMIME)->params);
		ZapHandle(tlMIME);
	}
}

/**********************************************************************
 * DisposeTLMIMEParam - dispose of an old parameter list
 **********************************************************************/
static void DisposeOldTLMIMEParam(tlMIMEParamP param)
{
	if (param)
	{
		if (*param)
		{
			if ((*param)->next) DisposeOldTLMIMEParam((*param)->next);
			//	Don't dispose of (*param)->value since we didn't make a copy of it
		}
		ZapHandle(param);
	}
}

/************************************************************************
 * RecordTLID - record the id of this translator
 ************************************************************************/
OSErr RecordTLID(FSSpecPtr spec,uLong id)
{
	short oldResF = CurResFile();
	Handle idHandle = NuHandle(sizeof(long));
	short refN;
	OSErr err;
	
	if (!idHandle) return(MemError());
	
	**(long**)idHandle = id;
	refN = FSpOpenResFile(spec,fsRdWrPerm);
	if (refN==-1) err = ResError();
	else
	{
		AddResource(idHandle,'etlI',1001,"");
		if (err=ResError()) ZapHandle(idHandle);
		else
		{
			idHandle = nil;
			err = MyUpdateResFile(refN);
			CloseResFile(refN);
		}
	}
	ZapHandle(idHandle);
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr AddTLMIME(emsMIMEHandle tlMIME,short what,PStr name, PStr value)
{
	UHandle h2=nil;
	emsMIMEParamHandle h3=nil;
	OSErr err = noErr;
	
	switch(what)
	{
		/*
		 * put in the type
		 */
		case TLMIME_TYPE:
			PSCopy((*tlMIME)->mimeType,name);
			break;
		
		/*
		 * put in the subtype
		 */
		case TLMIME_SUBTYPE:
			PSCopy((*tlMIME)->subType,name);
			break;
		
		/*
		 * add a parameter
		 */
		case TLMIME_CONTENTDISP_PARAM:
		case TLMIME_PARAM:
			// already in the list?
			for (h3=(what==TLMIME_PARAM ? (*tlMIME)->params : (*tlMIME)->contentParams);h3;h3=(*h3)->next)
			{
				if (StringSame(LDRef(h3)->name,name))
				{
					// append
					UL(h3);
					PtrPlusHand(value+1,(*h3)->value,*value);
					if (MemError()) return(MemError());
					(*(*h3)->value)[0] += *value;
					return(noErr);
				}
				UL(h3);
			}
			
			// not in the list.  Make a new one.
			h3 = NewZH(struct emsMIMEparamS);
			h2 = NuHTempBetter(*value);
			if (h2 && h3)
			{
				PSCopy((*h3)->name,name);
				BMD(value+1,*h2,*value);
				(*h3)->value = h2;
				if (what==TLMIME_PARAM)
					LL_Queue((*tlMIME)->params,h3,(emsMIMEParamHandle));
				else
					LL_Queue((*tlMIME)->contentParams,h3,(emsMIMEParamHandle));
				h2 = nil;
				h3 = nil;
			}
			else
				err = MemError();
			break;
		
		/*
		 * put in the MIME version
		 */
		case TLMIME_VERSION:
			PSCopy((*tlMIME)->mimeVersion,name);
			break;
			
		/*
		 * put in the content-disposition
		 */
		case TLMIME_CONTENTDISP:
			PSCopy((*tlMIME)->contentDisp,name);
			break;
	}
	
	ZapHandle(h2);
	ZapHandle(h3);
	return(err);
}

/**********************************************************************
 * FlattenTLMIME - take an awful tlMIME thing and turn it into a single handle
 **********************************************************************/
OSErr FlattenTLMIME(emsMIMEHandle tlMIME,FlatTLMIMEHandle *flat)
{
	Accumulator a;
	OSErr err;
	emsMIMEParamHandle p;
	emsMIMEtype localType;
	emsMIMEparam localParam;
	short len;
	
	/*
	 * complete?
	 */
	if (!tlMIME || !(*tlMIME)->mimeType || !(*tlMIME)->subType) return(fnfErr);
	
	localType = **tlMIME;
	
	/*
	 * add basic stuff
	 */
	if (!(err=AccuInit(&a)))
	if (!(err=AccuAddPtr(&a,localType.mimeType,*localType.mimeType+1)))
	if (!(err=AccuAddPtr(&a,localType.subType,*localType.subType+1)))
	{
		/*
		 * add params
		 */
		for (p=(*tlMIME)->params;!err && p;p=(*p)->next)
		{
			localParam = **p;
			if (!(err=AccuAddPtr(&a,localParam.name,*localParam.name+1)))
			{
				len = GetHandleSize((*p)->value);
				if (!(err=AccuAddPtr(&a,(void*)&len,2)))
					err = AccuAddHandle(&a,(*p)->value);
			}
		}
	}
	
	/*
	 * failure?
	 */
	if (err) AccuZap(a);
	else
	{
		AccuTrim(&a);
		*flat = a.data;
	}
	
	return(err);
}

/**********************************************************************
 * UnflattenTLMIME - take a handle and turn it into an awful tlMIME thing
 **********************************************************************/
OSErr UnflattenTLMIME(FlatTLMIMEHandle flat,emsMIMEHandle *tlMIME)
{
	OSErr err = noErr;
	Str63 name;
	Str255 value;
	long offset=0;
	long len;
	short paramLen;
	
	if (err = NewTLMIME(tlMIME)) return(err);
	
	len = GetHandleSize(flat);

	/*
	 * type
	 */
	if (!err && offset<len)
	{
		PSCopy(name,(*flat)+offset);
		offset += *name+1;
		err = AddTLMIME(*tlMIME,TLMIME_TYPE,name,nil);
		ASSERT(!err);
	}
	else err = fnfErr; //missing parts
	
	/*
	 * subtype
	 */
	if (!err && offset<len)
	{
		PSCopy(name,(*flat)+offset);
		offset += *name+1;
		err = AddTLMIME(*tlMIME,TLMIME_SUBTYPE,name,nil);
		ASSERT(!err);
	}
	else err = fnfErr;
	
	/*
	 * params
	 */
	while (!err && offset<len)
	{
		PSCopy(name,(*flat)+offset);
		offset += *name+1;
		if (offset<len)
		{
			paramLen = (((uShort)(*flat)[offset])<<256)|(*flat)[offset+1];
			MakePStr(value,*flat+offset+2,paramLen);
			offset += paramLen+2;
			err = AddTLMIME(*tlMIME,TLMIME_PARAM,name,value);
			ASSERT(!err);
		}
		else err = fnfErr;
	}
	
	/*
	 * failure?
	 */
	if (err) ZapTLMIME(*tlMIME);
	return(err);
}

/**********************************************************************
 * ETLInterpretFile - interpret a MIME file, possibly using a translator
 *		source - source filespec
 *		resultRefN - result fileref; this is where the text display will go
 *		resultAcc - result accumulator; this is where text will go if resultRefN nil
 **********************************************************************/
OSErr ETLInterpretFile(short context,FSSpecPtr source, short resultRefN, AccuPtr resultAcc, emsHeaderDataP addrList,Boolean *dontSave)
{
	OSErr err;
	emsMIMEHandle tlMIME=nil;
	TLMHandle translators=nil;
	FSSpec tempSpec;
	UHandle textH=nil;
	emsMIMEHandle newtlMIME = nil;
	Handle errorStr=nil;
	long	returnCode;
	
	Zero(tempSpec);
	
	if (!(err=ETLGetTLMIME(source,&tlMIME)))
	if (!(err=ETLListAllTranslators(&translators,context)))
	{
		ETLSortTranslators(translators);
		err = ETLCanTranslate(translators,context,tlMIME,nil,nil,addrList,nil);
		if (err==TLR_CANT_TRANS)
		{
			err = ETLCopyFile(source,resultRefN,True,nil);
		}
		else if (err==TLR_NOT_NOW && context==EMSF_ON_ARRIVAL)
		{
			RecordTL(source,translators);
		}
		else if (!err || err==TLR_NOW)
		{
			NewTempSpec(Root.vRef,Root.dirId,nil,&tempSpec);
			FSpCreateResFile(&tempSpec,CREATOR,MIME_FTYPE,smSystemScript);
			returnCode = 0;
			err = ETLTranslate(translators,context,tlMIME,source,&newtlMIME,&textH,&tempSpec,&errorStr,&returnCode,addrList);
			DisposeTLMIME(newtlMIME);
			if (err == EMSR_DELETE_MESSAGE)
			{
				// the translator has asked that the message be deleted
				if (!PrefIsSet(PREF_ETL_IGNORE_DELETES)) ETLDeleteRequest = true;
				err = EMSR_OK;				// continue processing
			}
			if (!err) 
			{
				//	See if should be marked clean
				if (returnCode & EMSF_DONTSAVE)
				{
					*dontSave = true;
					returnCode &= ~EMSF_DONTSAVE;
				}

				if (!textH || !(err=ETLOutputPtr(LDRef(textH)+1,**textH,resultRefN,resultAcc)))
				{	
					Boolean	fDoSig = false;
					short	sigListCount;
									
					if (textH) ETLOutputPtr("\015",1,resultRefN,resultAcc);
					ZapHandle(textH);
					if (gpSignList && returnCode == TLC_SIGOK && gpSignList->count < kMaxSignList)
					{
						//	Beginning of signature decoration
						sigListCount = gpSignList->count++;
						GetFPos(resultRefN, &gpSignList->begin[sigListCount]);
						fDoSig = true;
					}
					err = ETLInterpretFile(context,&tempSpec,resultRefN,resultAcc,addrList,dontSave);
					if (fDoSig)
					{
						//	End of signature decoration
						GetFPos(resultRefN, &gpSignList->end[sigListCount]);
					}
					// if new translator doesn't want it now, copy back to original
					if (err==TLR_NOT_NOW && context==EMSF_ON_ARRIVAL)
						FSpExchangeFiles(source,&tempSpec);
				}
			
			}
			
		}
	}
	if (err==EMSR_ABORTED) err = userCanceledErr;
	if (err && err!=userCanceledErr && err!=EMSR_DATA_UNCHANGED)
		if (errorStr) AlertStr(BIG_OK_ALRT,Stop,LDRef(errorStr));
		else if (context!=EMSF_ON_ARRIVAL) WarnUser(ETL_TRANS_FAILED,err);

	if (err==userCanceledErr) ETLDeleteRequest = false;		// leave the message alone if the user cancelled.
	
	FSpDelete(&tempSpec);
	ZapHandle(textH);
	ZapHandle(errorStr);
	ZapTLMIME(tlMIME);
	ZapHandle(translators);
	return(err);
}

/**********************************************************************
 * ETLGetTLMIME - read the tlMIME thing back out of a file
 **********************************************************************/
OSErr ETLGetTLMIME(FSSpecPtr spec,emsMIMEHandle *tlMIME)
{
	Handle res;
	short refN;
	OSErr err = fnfErr;
	short oldResF = CurResFile();
	
	*tlMIME = nil;
	
	if (-1!=(refN=FSpOpenResFile(spec,fsRdPerm)))
	{
		if (res=Get1Resource(MIME_FTYPE,1001))
			err=UnflattenTLMIME(res,tlMIME);
		CloseResFile(refN);
	}
	
	if (err==fnfErr) err = ETLReadTLMIME(spec,tlMIME);
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * ETLReadTL - read the translator out of a file
 **********************************************************************/
OSErr ETLReadTL(FSSpecPtr spec,long *id)
{
	short refN;
	OSErr err=fnfErr;
	Handle res;
	short oldResF = CurResFile();
	
	refN = FSpOpenResFile(spec,fsRdPerm);
	if (refN==-1) err = ResError();
	else
	{
		if (res=GetResource('etlI',1001))
		{
			err = noErr;
			*id = **(long**)res;
		}
		CloseResFile(refN);
	}
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * ETLReadTLMIME - read mime info
 **********************************************************************/
OSErr ETLReadTLMIME(FSSpecPtr spec,emsMIMEHandle *tlMIME)
{
	HeaderDHandle hdh = nil;
	OSErr err = ETLCopyFile(spec,0,False,&hdh);
	
	if (!err && hdh)
	{
		*tlMIME = (*hdh)->tlMIME;
		(*hdh)->tlMIME = nil;
		ZapHeaderDesc(hdh);
	}
	else if (!hdh)
	{
		err = NewMIME(tlMIME,MIME_TEXT,MIME_PLAIN);
	}
	return(err);
}

/**********************************************************************
 * ETLCopyFile - copy a MIME file
 **********************************************************************/
OSErr ETLCopyFile(FSSpecPtr spec,short refN,Boolean display,HeaderDHandle *theHeaders)
{
	LineIOD lid;
	LineIOP lip;
	OSErr err;
	UHandle buf = nil;
	long size = GetRLong(BUFFER_SIZE);
	
	/*
	 * set up stack, lineio, io buffer
	 */
	if (!TransContextStack && (err=StackInit(sizeof(LineIOP),&TransContextStack)))
		return(WarnUser(ETL_TRANS_FAILED,err));
	if (err=OpenLine(spec->vRefNum,spec->parID,spec->name,fsRdWrPerm,&lid))
		return(FileSystemError(ETL_TRANS_FAILED,spec->name,err));
	if (!(buf = NewIOBHandle(256,size))) err = MemError();
	
	/*
	 * do the work
	 */
	lip = &lid;
	if (buf && !(err=StackPush(&lip,TransContextStack)))
	{
		TransVector oldTrans = CurTrans;
		CurTrans = TransTrans;
		size = GetHandleSize(buf);
		MoveHHi(buf);
		err = ReadHeadAndBody(NULL,refN,LDRef(buf),size,display,theHeaders);	//NULL is probably OK here.  JDB 6/7/97
		StackPop(nil,TransContextStack);
		CurTrans = oldTrans;
	}
	
	/*
	 * cleanup
	 */
	CloseLine(&lid);
	ZapHandle(buf);
	if (err) WarnUser(ETL_TRANS_FAILED,err);
	return(err);
}

/**********************************************************************
 * ETLDisplayFile - display the contents of a file
 **********************************************************************/
OSErr ETLDisplayFile(FSSpecPtr spec,PETEHandle pte)
{
	FSSpec tempSpec;
	OSErr err = noErr;
	short refN;
	MyWindowPtr win;
	WindowPtr	winWP;
	long sel;
	Str255 title;
	long oldLen;
	SignOffsetList	signList;
	Boolean	dontSave = false;
	UHandle textH;
	TOCHandle tocH;
	short sumNum;
	
	AnyRich = AnyHTML = AnyFlow = AnyCharset = False;
	if (err = NewTempSpec(Root.vRef,Root.dirId,nil,&tempSpec))
		WarnUser(ETL_TRANS_FAILED,err);
	else
	{
		FSpCreate(&tempSpec,CREATOR,'TEXT',smSystemScript);
		if (err=FSpOpenDF(&tempSpec,fsRdWrPerm,&refN))
			FileSystemError(ETL_TRANS_FAILED,tempSpec.name,err);
		else
		{
			Handle	hSignBounds = nil;
			emsHeaderData addrList;
			
			//	Set up list to keep track of signature bars
			gpSignList = &signList;
			signList.count = 0;
			
			// grab cache if we can
			win = (*PeteExtra(pte))->win;
			winWP = GetMyWindowWindowPtr(win);
			textH = PeteText(pte);
			sumNum = -1;
			if (GetWindowKind(winWP) == MBOX_WIN)
			{
				tocH = (TOCHandle)GetMyWindowPrivateData(win);
				if (pte==(*tocH)->previewPTE) sumNum = FirstMsgSelected(tocH);
			}
			else if (GetWindowKind(winWP) == MESS_WIN)
			{
				tocH = (*Win2MessH(win))->tocH;
				sumNum = (*Win2MessH(win))->sumNum;
			}
			if (sumNum>=0)
			{
				CacheMessage(tocH,sumNum);
				if ((*tocH)->sums[sumNum].cache)
					HNoPurge((*tocH)->sums[sumNum].cache);
				else
					sumNum = -1;
			}

			// build the address list
			ETLBuildAddrList(textH,nil,nil,&addrList,EMSF_Q4_TRANSMISSION);
			if (sumNum!=-1) HPurge((*tocH)->sums[sumNum].cache);
			err=ETLInterpretFile(TLF_ON_DISPLAY,spec,refN,nil,&addrList,&dontSave);
			ETLDisposeAddrList(&addrList);
			if (!err)
			{
				/*My - no need for flush */FSClose(refN); refN = 0;
				if (pte)
				{
					PeteGetTextAndSelection(pte,nil,nil,&sel);
					PETESelect(PETE,pte,sel,sel);
					PeteSetURLRescan(pte,sel);
					PetePlain(pte,kPETECurrentStyle,kPETECurrentStyle,peAllValid);
					PETEInsertTextPtr(PETE,pte,-1,"\015\015",2,nil);
					sel += 2;	//	Added 2 cr's
					PETESelect(PETE,pte,sel,sel);
					oldLen = PETEGetTextLen(PETE,pte);
					err = InsertTextFile((*PeteExtra(pte))->win,&tempSpec,AnyRich||AnyHTML||AnyFlow||AnyCharset||AnyDelSP,AnyDelSP);

					if (signList.count)
					{
						//	Mark signatures
						short		sigIdx;
						
						for (sigIdx=0;sigIdx<signList.count;sigIdx++)
						{							
							long		offsetBegin,offsetEnd,paraBegin,paraEnd,paraIdx;
							PETEParaInfo pinfo;

							offsetBegin = sel+signList.begin[sigIdx];
							offsetEnd = sel+signList.end[sigIdx];
							PeteEnsureBreak(pte,offsetBegin);
							PeteEnsureBreak(pte,offsetEnd);
							paraBegin = PeteParaAt(pte,offsetBegin);
							paraEnd = PeteParaAt(pte,offsetEnd);

							pinfo.tabHandle = nil;
							PETEGetParaInfo(PETE,pte,kPETECurrentSelection,&pinfo);	//	Get the paragraph style here						
							pinfo.signedLevel++;
							
							//	Mark each paragraph
							for (paraIdx = paraBegin;paraIdx <= paraEnd;paraIdx++)
								PETESetParaInfo(PETE,pte,paraIdx,&pinfo,peSignedLevelValid);
						}
						//	Restore original selection
						PETESelect(PETE,pte,sel,sel);
					}

				}
				else
				{
					GetRString(title,UNTITLED);
					err = !(win=OpenText(&tempSpec,nil,nil,nil,True,title,False,False));
					if (win) (*(TextDHandle)GetMyWindowPrivateData(win))->spec.vRefNum = 0;
				}
				WipeSpec(&tempSpec);
			}
			gpSignList = nil;
		}
		if (refN) /*My - no need for flush */FSClose(refN);
		FSpDelete(&tempSpec);
	}

	if (dontSave && pte)
	{
		PeteCleanList(pte);
		(*PeteExtra(pte))->win->isDirty = false;
	}	
	return err;
}

/**********************************************************************
 * ETLNameAndIcon - get translator name and icon
 **********************************************************************/
void ETLNameAndIcon(short i,PStr name,Handle *suite)
{
	*suite = (*TLFL)[i].moduleSuite;
	PCopy(name,*(*TLFL)[i].moduleDescription);
}

/**********************************************************************
 * TransRecvLine - receive a line for a translator
 **********************************************************************/
OSErr TransRecvLine(TransStream stream, UPtr line,long *size)
{
#pragma unused (stream)
	OSErr err;
	LineIOP lip;
	short result;
	static Boolean wasNl=True;
	
	err = StackTop(&lip,TransContextStack);
	ASSERT(!err);
	
	if (!err)
	{
		result = NLGetLine(line,*size,size,lip);
		err = result<0;
		
		if (!result)
		{
			*size = 2;
			line[0]='.'; line[1]='\015'; line[2] = 0;
		}
		else if (wasNl && line[0]=='.')
		{
			BMD(line,line+1,*size);
			++*size;
		}
		wasNl = line[*size]=='\015';
	}
	return(err);
}

/**********************************************************************
 * ETLError - error with translator
 **********************************************************************/
short ETLError(tlStringHandle description,short context,tlStringHandle errorStr,OSErr err,long subError)
{
	Str255 string, error;
	
	if (description) PSCopy(string,*description); else *string = 0;
	if (errorStr) PSCopy(error,*errorStr); else *error = 0;
	return(Aprintf(BIG_OK_ALRT,Caution,ETL_ERROR_FMT,string,context,error,err,subError));
}

/**********************************************************************
 * ETLAddIcons - add translator icons to the composition window
 **********************************************************************/
OSErr ETLAddIcons(MyWindowPtr win,short startNumber)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle theCtl;
	short i;
	Handle suite;
	Rect r;
	OSErr err = fnfErr;
	long	flags;
	
	Zero(r);
	if (TLQ4List)
	{
		for (i=HandleCount(TLQ4List);i--;)
		{
			flags = (*TLQ4List)[i].flags;
		
			if (suite = (*TLQ4List)[i].suite)
			{
				if (theCtl = GetNewControlSmall(ICON_BAR_CNTL,winWP))
				{
					err = noErr;	// found at least one
					EmbedControl(theCtl,win->topMarginCntl);
					SetBevelIcon(theCtl,0,0,0,suite);
					SetControlReference(theCtl, 0xff000000 | startNumber+i);
					(*Win2MessH(win))->nTransIcons++;
					if (InTranslator((*Win2MessH(win))->hTranslators,ETLIconToID(i)))
						SetControlValue(theCtl,1);
					else if (SumOf(Win2MessH(win))->length==0 && flags & EMSF_DEFAULT_Q_ON)
					{
						//	Default is on
						//	Turn on the translator
						StringHandle	properties = nil;
						long	selected;
						
						selected = true;
						QueuedProperties(i,&selected,&properties,true);
						if (selected)
							AddMessTranslator(Win2MessH(win),i,properties);
					}
				}
			}
		}
	}
	return(err);
}

/**********************************************************************
 * ETLIconToID - turn an icon index into an id
 **********************************************************************/
long ETLIconToID(short which)
{
	if (TLQ4List && which<HandleCount(TLQ4List)) return(((*TLFL)[(*TLQ4List)[which].module].moduleID<<16L)|(*TLQ4List)[which].id);
	else return(0);
}

/**********************************************************************
 * ETLIconToDescriptions - grab the descriptions for a translator
 **********************************************************************/
OSErr ETLIconToDescriptions(short which,PStr module,PStr translator)
{
	if (TLQ4List && which<HandleCount(TLQ4List))
	{
		PCopy(module,*(*TLFL)[(*TLQ4List)[which].module].moduleDescription);
		PCopy(translator,*(*TLQ4List)[which].nameHandle);
		return(noErr);
	}
	else
	{
		*module = *translator = 0;
		return(fnfErr);
	}
}

/**********************************************************************
 * ETLIDToIndex - turn an id into an index
 **********************************************************************/
short ETLIDToIndex(long id)
{
	short module = (id>>16)&0xffff;
	short translator = id & 0xffff;
	short index;
	
	if (TLQ4List)
		for (index = HandleCount(TLQ4List);index--;)
			if ((*TLFL)[(*TLQ4List)[index].module].moduleID==module && (*TLQ4List)[index].id==translator)
				return(index);
	return(-1);
}

#define M_FTYPE	MIME_FTYPE

/**********************************************************************
 * ETLQueueMessage - if we have any ON_COMPLETION translators, translate how
 **********************************************************************/
short ETLQueueMessage(MessHandle messH)
{
	OSErr err = noErr;

	TransInfoHandle translators = (*messH)->hTranslators;
	Boolean	doCompletion = false;
	short i;
	Str32	s;

	//	Got any ON_COMPLETION translators?
	if (translators)
	{
		long flags = SumOf(messH)->flags;
		for (i=HandleCount(translators);i--;)
		{
			short	index;
			index = ETLIDToIndex((*translators)[i].id);
			if (index>=0 && (*TLQ4List)[index].flags & EMSF_Q4_COMPLETION)
			{
				doCompletion = true;
				break;
			}
		}
	}
	
	if (doCompletion)
	{
		FSSpec raw, cooked;
		OSErr	transErr = 0;

		//	Do all EMSF_Q4_COMPLETION and EMSF_Q4_TRANSMISSION translators
		if (!(err=NewTempSpec(Root.vRef,Root.dirId,nil,&raw)))
		if (!(err=FSpCreate(&raw,CREATOR,M_FTYPE,smSystemScript)))
		{
			Uhandle text=nil;
			HeadSpec hs;					
			
			//	Create destination file in spool folder with same name as subject
			if (!(err=MakeAttSubFolder(messH,SumOf(messH)->uidHash,&cooked)))
			if (CompHeadFind(messH,SUBJ_HEAD,&hs) && 
				!(CompHeadGetText(TheBody,&hs,&text)))
			{
				short	saveLen;
				short	suffix = 2;
				
				//	Get Subject
				MakePStr(cooked.name,*text,GetHandleSize(text));
				ZapHandle(text);

				//	Make sure we have a unique file name
				saveLen = *cooked.name;
				while (!FSpExists(&cooked))
				{
					//	No error means that the file/folder exists. Change the file name by adding a numeric suffix
					NumToString(suffix++,s);
					if (saveLen+*s > 30)
						saveLen = 30-*s;	//	Name was too long
					*cooked.name = saveLen;	//	Remove any suffix
					PCatC(cooked.name,' ');
					PCat(cooked.name,s);
				}
				if (!(err=FSpCreate(&cooked,CREATOR,M_FTYPE,smSystemScript)))
				{
					emsMIMEHandle tlMIME = nil;
					short refN;

					//	Get raw message
					if (!(err=FSpOpenDF(&raw,fsRdWrPerm,&refN)))
					{
						err = WriteMailFile(messH,refN,True,&tlMIME);
						FSClose(refN);
					}
					
					//	Do translation
					if (!err) err = ETLTransOut(messH,tlMIME,&raw,&cooked);
					transErr = err;
					
					if (!err)
					{
						CompAttachSpec((*messH)->win,&cooked);
						CompHeadFind(messH,0,&hs);
						PeteDelete((*messH)->bodyPTE,hs.value,hs.stop);
						ZapHandle(translators);
						(*messH)->hTranslators = nil;
					}
					ZapTLMIME(tlMIME);
				}
			}
			PrefIsSet(PREF_WIPE) ? WipeSpec(&raw):FSpDelete(&raw);
		}
		if (err && err!=userCanceledErr && !transErr) WarnUser(GENERAL,err);
	}
	return(err);
}

/**********************************************************************
 * ETLSendMessage - send a message with the translators
 **********************************************************************/
short ETLSendMessage(TransStream stream,MessHandle messH,Boolean chatter,Boolean sendDataCmd)
{
	FSSpec raw, cooked;
	OSErr err;
	short refN;
	long flags = SumOf(messH)->flags;
	emsMIMEHandle tlMIME = nil;
	OSErr	transErr = 0;
	
	if (!(err=NewTempSpec(Root.vRef,Root.dirId,nil,&raw)))
	if (!(err=FSpCreate(&raw,CREATOR,M_FTYPE,smSystemScript)))
	if (!(err=NewTempSpec(Root.vRef,Root.dirId,nil,&cooked)))
	if (!(err=FSpCreate(&cooked,CREATOR,M_FTYPE,smSystemScript)))
	{
		{
			if (!(err=FSpOpenDF(&raw,fsRdWrPerm,&refN)))
			{
				err = WriteMailFile(messH,refN,True,&tlMIME);
				/*My - no need for flush */FSClose(refN);
			}
			
			if (!err) err = ETLTransOut(messH,tlMIME,&raw,&cooked);
			transErr = err;
			
			if (!err)
			{
				if (!(err=TransmitMessage(stream,messH,chatter,False,True,nil,sendDataCmd)))
				if (!(err = SendRawMIME(stream,&cooked))) err = FinishSMTP(stream,messH);
			}
		}
		PrefIsSet(PREF_WIPE) ? WipeSpec(&raw):FSpDelete(&raw);
		if (*cooked.name) FSpDelete(&cooked);
	}
	if (err && err!=userCanceledErr && !transErr) WarnUser(GENERAL,err);
	ZapTLMIME(tlMIME);
	return(err);
}

/**********************************************************************
 * ETLCanTransOut - can we translate this thing?
 **********************************************************************/
OSErr ETLCanTransOut(MessHandle messH)
{
	emsHeaderData list;
	OSErr err = ETLBuildAddrList(PeteText(TheBody),(*messH)->extras.data,nil,&list,EMSF_Q4_TRANSMISSION);
	TLMHandle transList=nil;
	emsMIMEHandle tlmHandle=nil;
	short i,n;
	Handle errorStr=nil;
	long errCode;
	
	if (!err)
	{
		if (!(err=ETLListRequestedTranslators(&transList,(*messH)->hTranslators)))
		{
			short	mimeType,subType;
			
			if (MessFlagIsSet(messH,FLAG_HAS_ATT))
			{
				mimeType = MIME_MULTIPART;
				subType = MIME_MIXED;
			}
			else
			{
				mimeType = MIME_TEXT;
				subType = MIME_PLAIN;
			}
			if (!(err=NewMIME(&tlmHandle,mimeType,subType)))
			{
				n = HandleCount(transList);
				for (i=0;i<n-1;i++)
				{
					err = CanTranslateFile(&(*TLFL)[(*transList)[i].module],EMSF_Q4_TRANSMISSION,(*transList)[i].id,
										    				tlmHandle,&errorStr,&errCode,nil,&list,messH);
					if (err==TLR_ABORTED) err = userCanceledErr;
					if (err && err!=userCanceledErr && err!=TLR_NOW) ETLError((*TLFL)[(*transList)[i].module].moduleDescription,ETL_CANT_TRANSLATE,errorStr,err,errCode);
					ZapHandle(errorStr);
					if (!err) err = TLR_NOW;
					if (err!=TLR_NOW) break;
				}
			}
		}
	}
	ZapHandle(transList);
	ETLDisposeAddrList(&list);
	ZapTLMIME(tlmHandle);
	return(err);
}

/**********************************************************************
 * ETLTransOut - actually translate something
 **********************************************************************/
OSErr ETLTransOut(MessHandle messH,emsMIMEHandle tlMIME,FSSpecPtr from,FSSpecPtr to)
{
	emsHeaderData addrList;
	OSErr err;
	TLMHandle transList=nil;
	
	err = ETLBuildAddrList(PeteText(TheBody),(*messH)->extras.data,nil,&addrList,EMSF_Q4_TRANSMISSION);
	if (!err)
	{
		if (!(err=ETLListRequestedTranslators(&transList,(*messH)->hTranslators)))
			err = ETLTransOutLow(transList,&addrList,tlMIME,from,to,messH);
	}
	ZapHandle(transList);
	ETLDisposeAddrList(&addrList);
	return(err);
}

/**********************************************************************
 * ETLTransOutLow - perform first translation in the list, and recurse
 **********************************************************************/
static OSErr ETLTransOutLow(TLMHandle transList,emsHeaderDataP addrList,
	emsMIMEHandle tlMIME,FSSpecPtr from,FSSpecPtr into,MessHandle messH)
{
	OSErr err;
	short n,i;
	FSSpec temp;
	emsMIMEHandle tlMIMEOut=nil;
	tlStringHandle errorStr=nil;
	long localErr;
	StringHandle	properties;
#ifdef DEBUG
#ifdef DEBUGFILES
	static short dbCount;
	Str255 dbStr;
	FInfo info;
	
	if (RunType==Debugging)
	{
		FSMakeFSSpec(Root.vRef,ComposeString(dbStr,"\pEOInput %d",++dbCount),&temp);
		FSpDupFile(&temp,from,True,False);
		FSpGetFInfo(&temp,&info);
		info.fdType = 'TEXT';
		info.fdCreator = 'MPS ';
		FSpSetFInfo(&temp,&info);
	}
#endif
#endif

	Zero(temp);
	
	//	Look for queued properties
	properties = GetQueuedProperties(messH,((*TLFL)[(*transList)[0].module].moduleID<<16L)|(*transList)[0].id);
	
	err = CanTranslateFile(&(*TLFL)[(*transList)[0].module],EMSF_Q4_TRANSMISSION,(*transList)[0].id,
													tlMIME,&errorStr,&localErr,properties,addrList,messH);
	if (err==TLR_ABORTED) err = userCanceledErr;
	if (!err) err = TLR_NOW;
	if (err!=TLR_NOW && err!=userCanceledErr)
	{
		ETLError((*TLFL)[(*transList)[0].module].moduleDescription,ETL_CANT_TRANSLATE,errorStr,err,localErr);
	}
	else
	{
		ZapHandle(errorStr);
		err = MyTranslateFile (&(*TLFL)[(*transList)[0].module], EMSF_Q4_TRANSMISSION, (*transList)[0].id, tlMIME,
												from, &tlMIMEOut,into, nil, nil, &errorStr, &localErr, properties,
												addrList,	messH, (*transList)[0].nameHandle);
		if (err==TLR_ABORTED) err = userCanceledErr;
		if (err && err!=EMSR_DATA_UNCHANGED)
		{
			if (err!=userCanceledErr) ETLError((*TLFL)[(*transList)[0].module].moduleDescription,ETL_CANT_TRANSLATE,errorStr,err,localErr);
			WipeSpec(into);
		}
		else
		{
			ZapHandle(errorStr);
			
			if (err==EMSR_DATA_UNCHANGED)
			{
				// Data unchanged, so put from file into into file and we're done
				if (err=FSpExchangeFiles(from,into))
					FileSystemError(ETL_CANT_TRANSLATE,from->name,err);
				else
					err = 0;
			}
			else
				RecordTLMIME(into,tlMIMEOut);
		
			if (!err)
			{
				n = HandleCount(transList);
				if (n>2)
				{
					for (i=0;i<n-1;i++) (*transList)[i] = (*transList)[i+1];
					SetHandleSize((Handle)transList,(n-1)*sizeof(**transList));
					NewTempSpec(into->vRefNum,into->parID,nil,&temp);
					FSpCreate(&temp,CREATOR,MIME_FTYPE,smSystemScript);
					if (err=ResError()) FileSystemError(ETL_CANT_TRANSLATE,temp.name,err);
					else if (err=FSpExchangeFiles(&temp,into))
					{
						WipeSpec(&temp);
						FileSystemError(ETL_CANT_TRANSLATE,temp.name,err);
					}
					else
					{
						err = ETLTransOutLow(transList,addrList,tlMIMEOut,&temp,into,messH);
						WipeSpec(&temp);
					}
				}
			}
		}
	}
	ZapHandle(errorStr);
	ZapTLMIME(tlMIMEOut);
	return(err);
}

/**********************************************************************
 * MakeAddress - return a emsAddressH for the indicated address
 **********************************************************************/
static emsAddressH MakeAddress(StringPtr sAddress,StringPtr sName)
{
	emsAddressH	hAddress = nil;
	
	if ((*sAddress || *sName) && (hAddress=NuHandleClear(sizeof(emsAddress))))
	{
		emsAddressP	pAddress;
		
		pAddress = LDRef(hAddress);
		pAddress->size = sizeof(emsAddress);
		if (*sAddress)
			pAddress->address = NewString(sAddress);
		if (*sName)
			pAddress->realname = NewString(sName);
		UL(hAddress);
	}
	return hAddress;
}

/**********************************************************************
 * GetAddressesFromHandle - get addresses from a handle
 **********************************************************************/
static emsAddressH GetAddressesFromHandle(Uhandle text,Boolean expandNicks)
{
	UHandle addr;
	UHandle rawAddresses;
	short err=0;
	Str255 s;
	Str255 raw;
	emsAddressH	hAddress = nil;
	emsAddressH	hLastAddress = nil;
	emsAddressH	hFirstAddress = nil;
	
	err=SuckAddresses(&rawAddresses,text,True,false,False,nil);
	ZapHandle(text);
	if (rawAddresses && **rawAddresses)
	{
		long offset;
		
		if (expandNicks)
		{
			ExpandAliases(&addr,rawAddresses,0,True);
			ZapHandle(rawAddresses);
		}
		else
			addr = rawAddresses;

		for (offset=0;!err && (*addr)[offset];offset+=(*addr)[offset]+2)
		{
			MakePStr(raw,(*addr)+offset+1,(*addr)[offset]);
			ShortAddr(s,raw);
			//Folder Carbon Copy no FCC's in Light. Flag them as wrong
			if (HasFeature (featureFcc))
				if (IsFCCAddr(s)) continue;	// skip fcc's
			BeautifyFrom(raw);
			if (StringSame(raw,s))
				*raw = 0;	//	No real name
			if (hAddress = MakeAddress(s,raw))
			{
				if (hLastAddress)
					(*hLastAddress)->next = hAddress;
				hLastAddress = hAddress;
				if (!hFirstAddress)
					hFirstAddress = hAddress;
			}
		}
		ZapHandle(addr);
	}
	return hFirstAddress;
}

/**********************************************************************
 * GetAddressesFromHeader - get addresses from a header
 **********************************************************************/
static emsAddressH GetAddressesFromHeader(UHandle textIn,short header,Boolean expandNicks)
{
	short err=0;
	Uhandle text=nil;
	emsAddressH	hFirstAddress = nil;
	HeadSpec hs;
	Str63 hName;
	
	if (HandleHeadFindStr(textIn,header?GetRString(hName,HeaderStrn+header):"",&hs) && 
		!(err=HandleHeadGetText(textIn,&hs,&text)))
	{
		hFirstAddress = GetAddressesFromHandle(text,expandNicks);
	}
	return hFirstAddress;
}

/**********************************************************************
 *  - get addresses from a header
 **********************************************************************/
static emsAddressH FindAddress(Accumulator acc, short index,Boolean expandNicks)
{
	short err=0;
	Uhandle text=nil;
	emsAddressH	hAddress = nil;
	emsAddressH	hLastAddress = nil;
	emsAddressH	hFirstAddress = nil;
	Str32	name;
	UPtr	pHeader;
	long	length;

	GetRString(name,HeaderStrn+index);
	length = acc.offset;
	if (pHeader = FindHeaderString(*acc.data,name,&length,false))
	{
		PtrToHand(pHeader, &text, length);
		hFirstAddress = GetAddressesFromHandle(text,expandNicks);
	}
	return hFirstAddress;
}

/**********************************************************************
 * ETLBuildAddrList - build an address list
 **********************************************************************/
OSErr ETLBuildAddrList(UHandle textIn,UHandle moreHeaders,HeaderDHandle hdh,emsHeaderDataP addrList,short context)
{
	Str255 sName;
	HeadSpec hs;
	Uhandle text=nil;
	Accumulator	acc;
	
	WriteZero(addrList,sizeof(emsHeaderData));
	addrList->size = sizeof(emsHeaderData);

	if (textIn || hdh)
	{

		if (gTranslatorFlags & (EMSF_BASIC_HEADERS+EMSF_ALL_HEADERS))
		{
			//	Do BASIC headers
			Boolean	expandNicks = ContextRequiresExpansion(context);
			
			if (textIn)
			{
				//	Get from message header

				//	Get From, To, CC, BCC headers
				addrList->from = GetAddressesFromHeader(textIn,FROM_HEAD,expandNicks);
				addrList->to = GetAddressesFromHeader(textIn,TO_HEAD,expandNicks);
				addrList->cc = GetAddressesFromHeader(textIn,CC_HEAD,expandNicks);
				addrList->bcc = GetAddressesFromHeader(textIn,BCC_HEAD,expandNicks);

				//	Get Subject
				if (HandleHeadFindStr(textIn,GetRString(sName,HeaderStrn+SUBJ_HEAD),&hs) && 
					!(HandleHeadGetText(textIn,&hs,&text)))
				{
						MakePStr(sName,*text,GetHandleSize(text));
						addrList->subject = NewString(sName);
						ZapHandle(text);
				}
			}
			else
			{
				long	length;
				UPtr	pHeader;
				acc = (*hdh)->fullHeaders;
				HLock(acc.data);

				//	Get From, To, CC, BCC headers
				addrList->from = FindAddress(acc,FROM_HEAD,expandNicks);
				addrList->to = FindAddress(acc,TO_HEAD,expandNicks);
				addrList->cc = FindAddress(acc,CC_HEAD,expandNicks);
				addrList->bcc = FindAddress(acc,BCC_HEAD,expandNicks);
				
				//	Get Subject
				GetRString(sName,HeaderStrn+SUBJ_HEAD);
				length = acc.offset;
				if (pHeader = FindHeaderString(*acc.data,sName,&length,false))
				{
					MakePStr(sName,pHeader,length);
					addrList->subject = NewString(sName);
				}

				HUnlock(acc.data);
			}
		}
		
		if ((gTranslatorFlags & EMSF_ALL_HEADERS) && (context==EMSF_ON_ARRIVAL || context==EMSF_ON_DISPLAY || context==EMSF_Q4_TRANSMISSION || context==EMSF_ON_REQUEST))
		{
			//	Get raw 822 headers
			if (textIn)
			{
				if (HandleHeadFindStr(textIn,"",&hs))
				{					
					hs.stop = hs.start-1;
					hs.start = 0;
					hs.value = 0;
					if (!(HandleHeadGetText(textIn,&hs,&text)))
					{
						addrList->rawHeaders = text;
						if (moreHeaders) HandPlusHand(moreHeaders,text);
					}
				}
			}
			else
			{
				acc = (*hdh)->fullHeaders;
				HLock(acc.data);
				PtrToHand(*acc.data, &addrList->rawHeaders, acc.offset);
				HUnlock(acc.data);
			}
		}
	}
	
	return(noErr);
}

/**********************************************************************
 * ETLBuildOldAddrList - build a translation address list for version 1
 **********************************************************************/
OSErr ETLBuildOldAddrList(MessHandle messH,tlAddrListHandle *addrList,Boolean expandNicks)
{
	Accumulator a;
	OSErr err = AccuInit(&a);
	BinAddrHandle addr = NuHandle(0);
	Str255 s;
	Str255 raw;
	long offset;
	long theNil = 0;
	
	if (!err && !(err=MemError()))
	{
		MessReturnAddr(messH,s);
		if (!(err=AccuAddHandleToPtr(&a,s,*s+1)))
		{
			GetRealname(s);
			if (*s)
				err=AccuAddHandleToPtr(&a,s,*s+1);
			else
				err=AccuAddPtr(&a,(void*)&theNil,sizeof(addr));
			if (!err)
			if (!(err=AccuAddPtr(&a,(void*)&theNil,sizeof(addr))))
			{
				if (!(err = GatherCompAddresses((*messH)->win,addr)))
				{
					if (expandNicks)
					{
						BinAddrHandle exp;
						if (!(err=ExpandAliases(&exp,addr,0,true)))
						{
							ZapHandle(addr);
							addr = exp;
						}
					}
					
					for (offset=0;!err && (*addr)[offset];offset+=(*addr)[offset]+2)
					{
						MakePStr(raw,(*addr)+offset+1,(*addr)[offset]);
						ShortAddr(s,raw);
						//Folder Carbon Copy no FCC's in Light. Flag them as wrong
						if (HasFeature (featureFcc))
							if (IsFCCAddr(s)) continue;	// skip fcc's
						if (!(err=AccuAddHandleToPtr(&a,s,*s+1)))
						{
							BeautifyFrom(raw);
							if (!(err=AccuAddHandleToPtr(&a,raw,*raw+1)))
								err=AccuAddPtr(&a,(void*)&theNil,sizeof(addr));
						}
					}
				}
			}
		}
	}
	if (!err) err = AccuAddPtr(&a,(void*)&theNil,sizeof(addr));
	if (!err)
	{
		AccuTrim(&a);
		*addrList = (void*)a.data;
		a.data = nil;
	}
	ZapHandle(addr);
	AccuZap(a);
	return(err);
}

/**********************************************************************
 * ZapAddress - dispose of an address list
 **********************************************************************/
static void ZapAddress(emsAddressH addr)
{
	emsAddressH	next;
	
	while (addr)
	{
		StringHandle	address, name;
		
		if (address = (*addr)->address) DisposeHandle(address);
		if (name = (*addr)->realname) DisposeHandle(name);
		next = (*addr)->next;
		DisposeHandle((Handle)addr);
		addr = next;
	}
}

/**********************************************************************
 * DisposeAddrList - destroy a translation address list thing
 **********************************************************************/
void ETLDisposeAddrList(emsHeaderDataP addrList)
{
	if (addrList)
	{
		ZapAddress(addrList->to);
		ZapAddress(addrList->from);
		ZapAddress(addrList->cc);
		ZapAddress(addrList->bcc);
		ZapHandle(addrList->subject);
		ZapHandle(addrList->rawHeaders);
	}
}

/**********************************************************************
 * DisposeOldAddrList - destroy a version 1 translation address list thing
 **********************************************************************/
static void DisposeOldAddrList(tlAddrListHandle addrList)
{
	short i;
	
	if (addrList)
	{
		for (i=HandleCount(addrList);i--;) ZapHandle((*addrList)[i]);
		ZapHandle(addrList);
	}
}

/**********************************************************************
 * ETLTransSelection - translate the selection in a window
 **********************************************************************/
OSErr ETLTransSelection(PETEHandle pte,HSPtr hs,short item)
{
	short n = HandleCount(TLSelList);
	TLMaster tlm;
	UHandle text=nil;
	OSErr err = noErr;
	long len;
	
	while (n-- && (*TLSelList)[n].menuItem!=item);
	
	if (n<0) return(WarnUser(ETL_CANT_FIND_TRANS,item));
	
	tlm = (*TLSelList)[n];
	
	if (tlm.flags & TLF_REQUIRES_MIME) return(WarnUser(ETL_IM_STUPID,0));
	
	if (tlm.flags & EMSF_ALL_TEXT)
	{
		//	Use all text
		hs->start = hs->value = 0;
		hs->stop = PETEGetTextLen(PETE,pte);
	}
	
	if ((CurrentModifiers()&optionKey) && PeteParaAt(pte,hs->start)!=PeteParaAt(pte,hs->stop) && UseFlowIn)
	{
		LightQuote(pte,hs->start,hs->stop,&hs->start,&hs->stop);
		hs->value = hs->start;
		PeteSelect(nil,pte,hs->start,hs->stop);
		PeteWrap(nil,pte,true);
		PeteGetTextAndSelection(pte,nil,&hs->start,&hs->stop);
		hs->value = hs->start;
	}
	
	if (!(err=CompHeadGetText(pte,hs,&text)))
	{
#ifdef NEVER
		if () err = ETLTransHandleBuf(&text,&tlm);
		else
#endif
		err = ETLTransHandleFile(&text,&tlm,pte);
		
		len = GetHandleSize(text);
		
		if (!err)
		{
			PetePrepareUndo(pte,peUndoPaste,hs->value,hs->stop,nil,nil);
			if (!(err = PeteDelete(pte,hs->value,hs->stop)))
			if (!(err = PETEInsertText(PETE,pte,hs->value,text,nil)))
			{
				text = nil;
				PeteSelect((*PeteExtra(pte))->win,pte,hs->value,hs->value+len);
			}
			PeteFinishUndo(pte,peUndoPaste,hs->value,hs->value+len);
			PeteChanged(pte,hs->value,hs->value+len);
		}
	}
	
	ZapHandle(text);
	return(err);
}

#ifdef NEVER
/**********************************************************************
 * ETLTransHandleBuf - translate a handle, using the buffer calls
 **********************************************************************/
OSErr ETLTransHandleBuf(UHandle *text,TLMPtr tlm)
{
	return(WarnUser(ETL_IM_STUPID,1));
}
#endif

/**********************************************************************
 * ETLTransHandleFile - translate a handle, using files as intermediaries
 **********************************************************************/
static OSErr ETLTransHandleFile(UHandle *text,TLMPtr tlm,PETEHandle pte)
{
	FSSpec raw, translated;
	OSErr err;
	UHandle errorStr = nil;
	emsMIMEHandle tlMIME = nil;
	Handle headers=nil;
	long localErr;
	long len;
long bo;
	emsMIMEHandle localTLMIME=nil;
	emsHeaderData addrList;

#ifdef DEBUG
#ifdef DEBUGFILES
	static short dbCount;
	Str255 dbStr;
	FInfo info;
	FSSpec temp;
#endif
#endif
	
	Zero(raw);
	Zero(translated);
	
	PlainTextMIMEStuff(&tlMIME,(tlm->flags&TLF_REQUIRES_MIME) ? &headers : nil);
	
	NewTempSpec(Root.vRef,Root.dirId,nil,&raw);
	NewTempSpec(Root.vRef,Root.dirId,nil,&translated);
	if ((tlm->flags&TLF_REQUIRES_MIME) && (err=AddLf(*text)) ||
			(err=FSpCreate(&raw,CREATOR,'TEXT',smSystemScript)) ||
			(err=FSpCreate(&translated,CREATOR,'TEXT',smSystemScript)) ||
			(headers && (err=Blat(&raw,headers,False))) ||
			(err=Blat(&raw,*text,True)))
		FileSystemError(ETL_CANT_TRANSLATE,raw.name,err);
	else
	{
#ifdef DEBUG
#ifdef DEBUGFILES
		if (RunType==Debugging)
		{
			FSMakeFSSpec(Root.vRef,ComposeString(dbStr,"\pESInput %d",++dbCount),&temp);
			FSpDupFile(&temp,&raw,True,False);
			FSpGetFInfo(&temp,&info);
			info.fdType = 'TEXT';
			info.fdCreator = 'MPS ';
			FSpSetFInfo(&temp,&info);
		}
#endif
#endif
		ZapHandle(*text);
		Zero(addrList);
		if (tlm->flags & (EMSF_BASIC_HEADERS+EMSF_ALL_HEADERS))
			ETLBuildAddrList(PeteText(pte),nil,nil,&addrList,EMSF_ON_REQUEST);
		err = CanTranslateFile(&(*TLFL)[tlm->module],TLF_ON_REQUEST,tlm->id,
											    tlMIME,&errorStr,&localErr,nil,&addrList,nil);
		if (err==TLR_ABORTED) err = userCanceledErr;
		if (!err || err==TLR_NOW)
		{
			err = MyTranslateFile(&(*TLFL)[tlm->module],TLF_ON_REQUEST,tlm->id,tlMIME,&raw,&localTLMIME,
														&translated,nil,nil,&errorStr, &localErr,nil,
														&addrList,nil, tlm->nameHandle);
			if (err==TLR_ABORTED) err = userCanceledErr;
		}
		ETLDisposeAddrList(&addrList);
		if (err)
		{
			if (err!=userCanceledErr && err!=EMSR_DATA_UNCHANGED) ETLError((*TLFL)[tlm->module].moduleDescription,ETL_CANT_TRANSLATE,errorStr,err,localErr);
			ZapHandle(errorStr);
		}
		else
		{
			if (err=Snarf(&translated,text,0))
				FileSystemError(ETL_CANT_TRANSLATE,translated.name,err);
#ifdef DEBUG
#ifdef DEBUGFILES
			if (RunType==Debugging)
			{
				FSMakeFSSpec(Root.vRef,ComposeString(dbStr,"\pESOutput %d",dbCount),&temp);
				FSpDupFile(&temp,&translated,True,False);
				FSpGetFInfo(&temp,&info);
				info.fdType = 'TEXT';
				info.fdCreator = 'MPS ';
				FSpSetFInfo(&temp,&info);
			}
#endif
#endif
			if (tlm->flags&TLF_GENERATES_MIME)
			{
				len = RemoveChar('\012',LDRef(*text),GetHandleSize_(*text));
				UL(text);
				SetHandleBig(*text,len);
				bo = BodyOffset(*text);
				BMD(**text+len,**text,len-bo);
				SetHandleBig(*text,len-bo);
			}
		}
	}
	if (err) ZapHandle(*text);
	ZapTLMIME(tlMIME);
	ZapTLMIME(localTLMIME);
	WipeSpec(&raw);
	WipeSpec(&translated);
	ZapHandle(errorStr);
	ZapHandle(headers);
	return(err);
}

/**********************************************************************
 * PlainTextMIMEStuff - generate plaintext headers
 **********************************************************************/
OSErr PlainTextMIMEStuff(emsMIMEHandle *tlMIME,Handle *headers)
{
	emsMIMEHandle localTLMIME=nil;
	Str255 scratch;
	Str63 scratch2;
	OSErr err;
	
	if (tlMIME) *tlMIME = nil;
	if (headers) *headers = nil;
	
	if (!(err=NewMIME(&localTLMIME,headers?MIME_TEXT:MIME_APPLICATION,headers?MIME_PLAIN:MIME_MACTEXT)))
	if (!(err=AddTLMIME(localTLMIME,TLMIME_PARAM,
							GetRString(scratch,MIME_CHARSET),GetRString(scratch2,MIME_MAC))))
	if (headers)
	{
		ComposeString(scratch,"\p%r: %r/%r; %r=%r\015\012\015\012",InterestHeadStrn+hContentType,
										headers?MIME_TEXT:MIME_APPLICATION,
										headers?MIME_PLAIN:MIME_MACTEXT,
										MIME_CHARSET,MIME_MAC);
		if (*headers=NuHTempBetter(*scratch))
			BMD(scratch+1,**headers,*scratch);
		else err = MemError();
	}
	
	if (err) ZapTLMIME(localTLMIME);
	else if (tlMIME) *tlMIME = localTLMIME;
	
	return(err);
}

/**********************************************************************
 * ETLID - get the id value for a translator
 **********************************************************************/
long ETLID(TLMHandle tl,short index)
{
	short module;
	short id;
	
	id = (*tl)[index].id;
	module = (*TLFL)[(*tl)[index].module].moduleID;
	return((module<<16L)|id);
}

/**********************************************************************
 * GetTransIcon - get a translator icon suite
 **********************************************************************/
static OSErr GetTransIcon(TLFLPtr tlfl,long id,Handle *suite)
{
	OSErr err = fnfErr;
	
	*suite = nil;
	if (UseParameterBlockStyleFunctions(tlfl->version))
	{
		emsTranslator transInfo;
		
		Zero(transInfo);
		SetSize(transInfo);
		transInfo.id = id;
		MightSwitch();
		err = ems_translator_info(tlfl->instance,&transInfo);
		AfterSwitch();
		ZapHandle(transInfo.desc);
		*suite = transInfo.icon;
	}
	else
	{
		MightSwitch();
		err = tl_translator_info(tlfl->instance,id,nil,nil,nil,nil,suite);
		AfterSwitch();
	}
	return(err);
}

/**********************************************************************
 * ETLIDToFileIcon - turn a translator id into an icon suite
 **********************************************************************/
OSErr ETLIDToFileIcon(long id,Handle *suite)
{
	short mId = id>>16;
	short tId = id&0xffff;
	short i;
	OSErr err = fnfErr;
	
	if (TLFL)
	{
		for (i=HandleCount(TLFL);i--;)
			if ((*TLFL)[i].moduleID==mId)
			{
				err = GetTransIcon(&(*TLFL)[i],tId,suite);
				break;
			}
	}
	return(err);
}

/**********************************************************************
 * IsSystemComponent - is this a component in the System folder?
 **********************************************************************/
static Boolean IsSystemComponent(Component comp)
{
	short i;

	if (ghSystemComponents)
	{
		for (i=HandleCount(ghSystemComponents);i--;)
			if (comp == (*ghSystemComponents)[i])
				return true;
	}
	return false;
}

/**********************************************************************
 * 
 **********************************************************************/
void ETLCleanup(void)
{
	short i;
	
	// clean up importer plugin icons
	if (TLImporterList)
		for (i=HandleCount(TLImporterList);i--;)
			DisposeIconSuite((*TLImporterList)[i].suite, true);
	
	
	if (TLFL)
		for (i=HandleCount(TLFL);i--;)
		{
			tlInstance	instance = (*TLFL)[i].instance;
			ComponentDescription info;
			Component comp=nil;
			Boolean		fUnregister;
			
			if (UseParameterBlockStyleFunctions((*TLFL)[i].version))
			{
				if (ComponentFunctionImplemented(instance,kems_plugin_finishRtn))
				{
					MightSwitch();
					ems_plugin_finish(instance);
					AfterSwitch();
				}
			}
			else
			{
				if (ComponentFunctionImplemented(instance,ktl_module_finishRtn))
				{
					MightSwitch();
					tl_module_finish(instance);
					AfterSwitch();
				}
			}
			
			//	Unregister only if it is not a system component
			fUnregister = !HaveTheDiseaseCalledOSX() && !(!GetComponentInfo((Component)instance,&info,nil,nil,nil) &&
						(comp = FindNextComponent(comp, &info)) &&
						IsSystemComponent(comp));

			CloseComponent(instance);

			if (fUnregister)
				UnregisterComponent(comp);
		}
}

/**********************************************************************
 * ETLIdle - called from main event loop. Call plug-ins that want idle time
 **********************************************************************/
void ETLIdle(long flags)
{
	long	thisTime;
	short	i;
	emsIdleData	idleData;
	
	if (gIdleTimeCount && TLFL)
	{
		TLFLPtr	pTLFL;
		
		//	Search for and call any plug-ins whose time has come
		thisTime = (TickCount()*50)/3;	//	(ticks / 60 * 1000) Roughly in milliseconds
		pTLFL = LDRef(TLFL);
		for (i=HandleCount(TLFL);i--;pTLFL++)
		{
			if (pTLFL->idleTimeFreq && ((thisTime - pTLFL->lastIdleTime >= pTLFL->idleTimeFreq) || flags==EMSFIDLE_PRE_SEND))
			{
				//	Call this one
				idleData.flags = flags;
				idleData.idleTimeFreq = &pTLFL->idleTimeFreq;
				idleData.progress = TO_UPP_OR_NOT_TO_UPP(ETLProgress3);
				MightSwitch();
				ems_idle(pTLFL->instance,&idleData);
				AfterSwitch();
				pTLFL->lastIdleTime = thisTime;
			}
		}
		UL(TLFL);
	}
}

/**********************************************************************
 * ETLEudoraModeNotification - called after loading plugins and after registration mode change.
 **********************************************************************/
void ETLEudoraModeNotification(ModeEventEnum modeEvent, ModeTypeEnum newMode )
{
	short	i;
	emsEudoraModeNotificationData	modeNotificationData;
		
	if (TLFL)
	{
		TLFLPtr	pTLFL;
		
		pTLFL = LDRef(TLFL);
		for (i=HandleCount(TLFL);i--;pTLFL++)
		{
			if (ComponentFunctionImplemented(pTLFL->instance,kems_eudora_mode_notificationRtn))
			{
				//	Call this one
				modeNotificationData.size = sizeof(emsEudoraModeNotificationData);
				modeNotificationData.modeEvent = EMS_ModeChanged;
				modeNotificationData.isFullFeatureSet = newMode;
				modeNotificationData.needsFullFeatureSet = EMS_ModeFree;
				modeNotificationData.downgradeWasBeingUsed = false;
				MightSwitch();
				ems_eudora_mode_notification(pTLFL->instance,&modeNotificationData);
				AfterSwitch();
				pTLFL->modeNeeded = modeNotificationData.needsFullFeatureSet;
				pTLFL->downgradeWasBeingUsed = modeNotificationData.downgradeWasBeingUsed;
			}
		}
		UL(TLFL);
	}
}

/**********************************************************************
 * CanTranslateFile - determine if translator can act on this file
 **********************************************************************/
static OSErr CanTranslateFile(TLFLPtr pTLFL,long context,short trans_id,
    emsMIMEHandle in_mime, StringPtr **out_error,
    long *out_code, StringHandle queuedProperties, emsHeaderDataP header, MessHandle messH)
{
  long out_len;		              /* Place to return estimated length (unused) */
  OSErr	err;
	tlInstance instance = pTLFL->instance;
	Boolean pb = UseParameterBlockStyleFunctions(pTLFL->version);
	
	if (pb)
	{
    emsTranslator trans;       	/* In: Translator Info */
    emsDataFile inTransData;   	/* In: What to translate */
    emsResultStatus transStatus; /* Out: Translations Status information */

		Zero(trans);
		SetSize(trans);
		trans.id = trans_id;
		trans.properties = queuedProperties;
		
		Zero(inTransData);
		SetSize(inTransData);
		inTransData.context = context;
		inTransData.mimeInfo = in_mime;
		inTransData.header = header;
		//	inTransData.file (file parameter not used for can_translate)

		Zero(transStatus);
		SetSize(transStatus);
		MightSwitch();
		err = ems_can_translate(instance,&trans,&inTransData,&transStatus);
		AfterSwitch();
		if (transStatus.desc)
			ZapHandle(transStatus.desc);
		if (out_error)
			*out_error = transStatus.error;
		else
			ZapHandle(transStatus.error);
		if (out_code)
			*out_code = transStatus.code;
		HUnlock((Handle)in_mime);
	}
	else
	{
		tlMIMEtypeP	tlMIME = DegradeMIME(in_mime);
		tlAddrListHandle	oldAddresses=nil;
				
		if (messH)
			ETLBuildOldAddrList(messH,&oldAddresses,ContextRequiresExpansion(context));
		MightSwitch();
		err = tl_can_translate_file(instance,context,trans_id,tlMIME,nil,oldAddresses,&out_len,out_error,out_code);
		AfterSwitch();
		DisposeOldTLMIME(tlMIME);	//	Dispose of copy
		DisposeOldAddrList(oldAddresses);
	}
	return err;
}

/**********************************************************************
 * MyTranslateFile - translate a file
 **********************************************************************/
static OSErr MyTranslateFile(TLFLPtr pTLFL,long context,short trans_id,
    emsMIMEHandle in_mime,FSSpec *in_file, emsMIMEHandle *out_mime,
    FSSpec *out_file,Handle *out_icon,StringPtr **out_desc,StringPtr **out_error,
    long *out_code, StringHandle queuedProperties, emsHeaderDataP header, 
    MessHandle messH, Handle translatorName)
{
  OSErr	err;
	tlInstance instance = pTLFL->instance;
	Boolean pb = UseParameterBlockStyleFunctions(pTLFL->version);
	char hState = 0;
	
//	OpenProgress();	
	PushProgress();
	ProgressMessageR(kpTitle,TRANSLATING);		
	
	// right now we'll just set the subTitle to the name of the translator. we may want to include the module description?
	hState = HGetState (translatorName);
	HLock (translatorName);
	ProgressMessage(kpSubTitle,*translatorName);
	HSetState (translatorName, hState);
	
	if (pb)
	{
	    emsTranslator trans;       	/* In: Translator Info */
	    emsDataFile inTransData;   	/* In: What to translate */
	    emsDataFile outTransData;   /* Out: Translation retult */
	    emsResultStatus transStatus; /* Out: Translations Status information */

		Zero(trans);
		SetSize(trans);
		trans.id = trans_id;
		trans.properties = queuedProperties;
		
		Zero(inTransData);
		SetSize(inTransData);
		inTransData.context = context;
		inTransData.mimeInfo = in_mime;
		inTransData.header = header;
		inTransData.file = *in_file;
		
		Zero(outTransData);
		SetSize(outTransData);
		outTransData.mimeInfo = nil;
		outTransData.file = *out_file;

		Zero(transStatus);
		SetSize(transStatus);
		MightSwitch();
		err = ems_translate_file(instance,&trans,&inTransData, TO_UPP_OR_NOT_TO_UPP(ETLProgress3), &outTransData,&transStatus);
		AfterSwitch();
		    
		if (out_desc)
			*out_desc = transStatus.desc;
		else
			ZapHandle(transStatus.desc);		
		if (out_error)
			*out_error = transStatus.error;
		else
			ZapHandle(transStatus.error);
		if (out_code)
			*out_code = transStatus.code;
		if (out_mime)
			*out_mime = outTransData.mimeInfo;
		else 
			DisposeTLMIME(outTransData.mimeInfo);
	}
	else
	{
		tlMIMEtypeP	outMIME = nil;
		tlMIMEtypeP	tlMIME = DegradeMIME(in_mime);
		tlAddrListHandle	oldAddresses=nil;

		if (messH)
			ETLBuildOldAddrList(messH,&oldAddresses,ContextRequiresExpansion(context));
		MightSwitch();
		err = tl_translate_file(instance,context,trans_id,tlMIME,in_file,oldAddresses,TO_UPP_OR_NOT_TO_UPP(ETLProgress3),&outMIME,
			out_file,out_icon,out_desc,out_error,out_code);
		AfterSwitch();
		DisposeOldTLMIME(tlMIME);	//	Dispose of copy
		DisposeOldAddrList(oldAddresses);
		
		*out_mime = UpgradeMIME(outMIME);
	}
	
	PopProgress(false);
//	CloseProgress ();

	return err;
}

/**********************************************************************
 * ETLSpecial - call a transator's special menu item
 **********************************************************************/
OSErr ETLSpecial(short item)
{
	emsMenu mInfo;
	OSErr err;
	
	Zero(mInfo);
	SetSize(mInfo);
	mInfo.id = (*TLSpecialList)[item-1].id;
	MightSwitch();
	err = ems_special_hook((*TLFL)[(*TLSpecialList)[item-1].module].instance,&mInfo);
	AfterSwitch();
	return(err);
}

/**********************************************************************
 * ETLSpecialEnableBasedOnMode - enable transator's special menu items
 *	based on whether or not they work in the reg mode that Eudora is in
 **********************************************************************/
void ETLEnableSpecialItems()
{
	MenuHandle	hMenu = GetMHandle(SPECIAL_MENU);
	short		i, n, menuItem;

	if (hMenu && TLSpecialList && *TLSpecialList && (n=HandleCount(TLSpecialList)))
	{
		for (i=0;i<n;i++)
		{
			menuItem = i+SPECIAL_BAR4_ITEM+1;
			if ((*TLFL)[(*TLSpecialList)[i].module].modeNeeded > GetCurrentPayMode ())
			{
				DisableMenuItemWithProOnlyIcon(hMenu, menuItem);
			}
			else
			{
				SetMenuItemIconHandle(hMenu, menuItem, kMenuNoIcon, nil);
				EnableItem(hMenu, menuItem);
			}
		}
	}
}

/**********************************************************************
 * ETLAttach - call an attachment translator
 **********************************************************************/
OSErr ETLAttach(short item,MyWindowPtr win)
{
	emsMenu mInfo;
	OSErr err;
	long numFiles=0;
	FSSpec spec;
	emsDataFileH dfs=nil;
	short i;
	
	Zero(mInfo);
	SetSize(mInfo);
	mInfo.id = (*TLAttachList)[item-1].id;
	
	if (err=MakeAttSubFolder(Win2MessH(win),SumOf(Win2MessH(win))->uidHash,&spec)) return(err);
	
	MightSwitch();
	err=ems_attacher_hook((*TLFL)[(*TLAttachList)[item-1].module].instance,&mInfo,&spec,&numFiles,&dfs);
	AfterSwitch();
	if (err) return(err);
	
	if (dfs)
	{
		ASSERT(numFiles==HandleCount(dfs));
		
		for (i=0;i<numFiles;i++)
		{
			spec = (*dfs)[i].file;
			CompAttachSpec(win,&spec);
		}
		ZapHandle(dfs);
	}
	
	return(noErr);
}

/**********************************************************************
 * ETLDoSettings - call translators for settings
 **********************************************************************/
OSErr ETLDoSettings(short item)
{
	OSErr err;
	short	module = (*TLSettingList)[item-1].module;
	TLFuncList tlfl = (*TLFL)[module];
	short	t;
	
	if (UseOldMailConfig(tlfl.version))
	{
		emsPre7MailConfig mailConfig;
		ETLFillMailConfig(&mailConfig, nil, true);
		MightSwitch();
		err = ems_plugin_config(tlfl.instance,&mailConfig);
		AfterSwitch();
		ETLEmptyMailConfig(&mailConfig);
	}
	else
	{
		emsCallBack callbacks;
		emsMailConfig mailConfig;
		ETLFillMailConfig(&mailConfig, &callbacks, false);
		MightSwitch();
		err = ems_plugin_config(tlfl.instance,&mailConfig);
		AfterSwitch();
		ETLEmptyMailConfig(&mailConfig);
	}

	//	Call ems_translator_info for each translator to see if flags, icons, etc. have changed
	for (t=0;t<tlfl.tCount;t++)
	{
		TLMaster tlm;

		Zero(tlm);
		if (UseParameterBlockStyleFunctions(tlfl.version))
		{
			emsTranslator transInfo;
			
			Zero(transInfo);
			SetSize(transInfo);
			transInfo.id = t+1;
			MightSwitch();
			err = ems_translator_info(tlfl.instance,&transInfo);
			AfterSwitch();
			tlm.type = transInfo.type;
			tlm.flags = transInfo.flags;
			tlm.nameHandle = transInfo.desc;
			tlm.suite = transInfo.icon;
		}
		
		// v1
		else
		{
			MightSwitch();
			err = tl_translator_info(tlfl.instance,t+1,&tlm.type,nil,&tlm.flags,&tlm.nameHandle,&tlm.suite);
			AfterSwitch();
		}
		
		
		if (!err)
		{
			Handle	oldText=nil,suite=nil;
			TLMPtr	pTLM;
			short	n;
			
			gTranslatorFlags |= tlm.flags;
			
			//	Search for translator in SelList and update fields
			for(n=HandleCount(TLSelList),pTLM=*TLSelList+n-1;n--;pTLM--)
				if (pTLM->module==module && pTLM->id==t+1)
				{
					oldText = pTLM->nameHandle;
					pTLM->nameHandle = tlm.nameHandle;
					pTLM->flags = tlm.flags;
					pTLM->type = tlm.type;
					if (!suite && tlm.suite)
					{
						suite = pTLM->suite;
						DupIconSuite(tlm.suite,&suite,suite!=nil);	// dup the suite, right into the old suite if we have one
						DisposeIconSuite(tlm.suite,true);	// only dup once
						tlm.suite = nil;
						pTLM=*TLSelList+n;	// update our silly pointer, since might have moved after DisposeIconSuite above
					}
					pTLM->suite = suite;
					break;
				}
			
			//	Search for translator in Q4List and update fields
			for(n=HandleCount(TLQ4List),pTLM=*TLQ4List+n-1;n--;pTLM--)
				if (pTLM->module==module && pTLM->id==t+1)
				{
					if (!oldText) oldText = pTLM->nameHandle;
					pTLM->nameHandle = tlm.nameHandle;
					pTLM->flags = tlm.flags;
					pTLM->type = tlm.type;
					if (!suite && tlm.suite)
					{
						suite = pTLM->suite;
						DupIconSuite(tlm.suite,&suite,suite!=nil);	// dup the suite, right into the old suite if we have one
						DisposeIconSuite(tlm.suite,true);	// only dup once
						tlm.suite = nil;
						pTLM=*TLSelList+n;	// update our silly pointer, since might have moved after DisposeIconSuite above
					}
					pTLM->suite = suite;
					break;
				}
			
			if (oldText) DisposeHandle(oldText);
		}
	}	
	
	return(err);
}

/**********************************************************************
 * QueuedProperties - allow plugin to store properties
 **********************************************************************/
OSErr QueuedProperties(short which, long *selected, StringHandle *pProperties,Boolean fDefaultOn)
{
	emsTranslator	trans;
	OSErr	err = noErr;
#define	pTLFL (&(*TLFL)[(*TLQ4List)[which].module])
	
	if (UseParameterBlockStyleFunctions(pTLFL->version) && ComponentFunctionImplemented(pTLFL->instance,kems_queued_propertiesRtn))
	{
		Zero(trans);
		SetSize(trans);
		trans.id = pTLFL->moduleID;
		trans.properties = *pProperties;
		if (fDefaultOn)
			trans.flags = 1;	//	Indicate that the translator is default on instead of clicked on by the user
		MightSwitch();
		err = ems_queued_properties(pTLFL->instance,&trans,selected);
		AfterSwitch();
		//better to leak than crash... if (*pProperties && trans.properties!=*pProperties)	ZapHandle(*pProperties);	// kill the old one if not still used
		if (trans.properties)
		{
			if (err || !*selected)
				ZapHandle(trans.properties);	//	Translator really should not have returned this handle
			else
				*pProperties = trans.properties;	//	Return properties
		}
	}
	else ZapHandle(*pProperties);	// get rid of copy of properties
	return err;
}

/**********************************************************************
 * ETLSelect - user clicked on plugin icon, allow plugin to store properties
 **********************************************************************/
OSErr ETLSelect(short which, Boolean selecting, MessHandle messH)
{
	long	selected;
	StringHandle properties;
	OSErr	result;

	properties = selecting ? nil :
		//	Deselecting. Get any existing queued properties
		GetQueuedProperties(messH,ETLIconToID(which));
	if (properties) properties = DupHandle(properties);	// protect our handle
	
	selected = selecting;
	result = QueuedProperties(which, &selected, &properties, false);
	if (selecting)
	{
		if (selected)
			AddMessTranslator(messH,which,properties);
	}
	else
	{
		RemoveMessTranslator(messH,which);
		if (selected)
			AddMessTranslator(messH,which,properties);
	}
	return result;
}

/**********************************************************************
 * ETLGetPluginFolderSpec - return filespec for a folder in plugin filters folder
 **********************************************************************/
OSErr ETLGetPluginFolderSpec(FSSpec *spec,short nameId)
{
	Str255	s;
	OSErr		err;
	
	if (!(err = FSMakeFSSpec(ItemsFolder.vRef,ItemsFolder.dirId,GetRString(s,PLUGINS_FOLDER),spec)))
	{
		IsAlias(spec,spec);
		if (nameId)
		{
			err = FSMakeFSSpec(spec->vRefNum,SpecDirId(spec),GetRString(s,nameId),spec);
			IsAlias(spec,spec);
		}
	}
	return err;
}


/**********************************************************************
 * UpgradeMIME - Convert from older mime structure in place
 **********************************************************************/
static emsMIMEHandle UpgradeMIME(tlMIMEtypeP inMIME)
{
	emsMIMEHandle	outMIME = (emsMIMEHandle)inMIME;

	if (inMIME)
	{
		emsMIMEtype		tempMIME;
		tlMIMEtype		*pInMIME;
		tlMIMEParamP	hParam;
		
		//	Convert MIME info to newer data structure
		SetHandleSize((Handle)inMIME,sizeof(emsMIMEtype));	//	Need to enlarge for larger structure
		if (MemError())
			return nil;	//	Error, don't return anything
		pInMIME = *inMIME;
		Zero(tempMIME);
		SetSize(tempMIME);
		BMD(pInMIME->mime_version,tempMIME.mimeVersion,sizeof(tempMIME.mimeVersion));
		BMD(pInMIME->mime_type,tempMIME.mimeType,sizeof(tempMIME.mimeType));
		BMD(pInMIME->sub_type,tempMIME.subType,sizeof(tempMIME.subType));
		hParam = pInMIME->params;
		tempMIME.params = (emsMIMEparamH)hParam;
		BMD(&tempMIME,*inMIME,sizeof(tempMIME));
		
		//	Now convert any parameters to newer data structure
		while (hParam)
		{
			tlMIMEparam		*pParam;
			emsMIMEparam	tempParam;
			
			SetHandleSize((Handle)hParam,sizeof(emsMIMEparam));	//	Need to enlarge for larger structure
			if (MemError())
				return nil;	//	Error, don't return anything
			pParam = *hParam;
			SetSize(tempParam);
			BMD(pParam->name,tempParam.name,sizeof(tempParam.name));
			tempParam.value = pParam->value;
			hParam = pParam->next;
			tempParam.next = (emsMIMEparamH)hParam;
			BMD(&tempParam,pParam,sizeof(tempParam));
		}
	}
	return outMIME;
}

/*	Junk Stuff */
OSErr ETLScoreJunk ( TLMPtr thePlugin, 
	    emsTranslatorP           transInfo,          /* In: Translator Info */
	    emsJunkInfoP             junkInfo,           /* In: Junk information */
	    emsMessageInfoP          messageInfo,        /* In: Message to score */
	    emsJunkScoreP            junkScore,          /* Out: Junk score */
	    emsResultStatusP         junkStatus          /* Out: Status information */
		) {
	return ems_score_junk ( (*TLFL)[ thePlugin->module].instance, transInfo, junkInfo, messageInfo, junkScore, junkStatus );
	}
	
/*	Junk Stuff */
OSErr ETLMarkJunk ( TLMPtr thePlugin, 
	    emsTranslatorP           transInfo,          /* In: Translator Info */
	    emsJunkInfoP             junkInfo,           /* In: Junk information */
	    emsMessageInfoP          messageInfo,        /* In: Message to score */
	    emsJunkScoreP            junkScore,          /* Out: Junk score */
	    emsResultStatusP         junkStatus          /* Out: Status information */
		) {
	return ems_user_mark_junk ( (*TLFL)[ thePlugin->module].instance, transInfo, junkInfo, messageInfo, junkScore, junkStatus );
	}
	

/**********************************************************************
 * DegradeMIMEParam - Convert to older mime paramater structure. Returns a copy.
 **********************************************************************/
static tlMIMEParamP DegradeMIMEParam(emsMIMEparamH hParam)
{
	tlMIMEParamP	hResult = nil;
	
	if (hParam && (hResult = NuHandle(sizeof(tlMIMEparam))))
	{		
		BMD((*hParam)->name,(*hResult)->name,sizeof((*hResult)->name));
		(*hResult)->value = (*hParam)->value;	//	We didn't make a copy of this so don't dispose of it!!!
		(*hResult)->next = DegradeMIMEParam((*hParam)->next);	//	Recurse
	}
	return hResult;
}

/**********************************************************************
 * DegradeMIME - Convert to older mime structure. Returns a copy.
 **********************************************************************/
static tlMIMEtypeP DegradeMIME(emsMIMEHandle inMIME)
{
	tlMIMEtypeP	outMIME = nil;

	if (inMIME && (outMIME = NuHandle(sizeof(tlMIMEtype))))
	{		
		//	Convert MIME info to older data structure
		BMD((*inMIME)->mimeVersion,(*outMIME)->mime_version,sizeof((*outMIME)->mime_version));
		BMD((*inMIME)->mimeType,(*outMIME)->mime_type,sizeof((*outMIME)->mime_type));
		BMD((*inMIME)->subType,(*outMIME)->sub_type,sizeof((*outMIME)->sub_type));
		(*outMIME)->params = DegradeMIMEParam((*inMIME)->params);
	}
	return outMIME;
}

/**********************************************************************
 * GetQueuedProperties - Find queued properties from message
 **********************************************************************/
static StringHandle GetQueuedProperties(MessHandle messH,long id)
{
	TransInfoHandle translators = (*messH)->hTranslators;
	short i;
	
	if (translators)
		for (i=HandleCount(translators);i--;)
			if ((*translators)[i].id==id) 
				return (*translators)[i].properties;
	return nil;
}

/************************************************************************
 * Modeless EMSAPI stuff
 ************************************************************************/
/************************************************************************
 * IsPlugwindow - is something a plugin window?
 ************************************************************************/
Boolean IsPlugwindow(WindowPtr theWindow)
{
	return(theWindow && GetWindowKind(theWindow)==EMS_PW_WINDOWKIND);
}

/************************************************************************
 * IsModalPlugwindow - is something a plugin window that wants to
 * be non-blocking modal?
 ************************************************************************/
Boolean IsModalPlugwindow(WindowPtr theWindow)
{
	Boolean		isModalPlugwindow = false;
	
	if (theWindow && GetWindowKind(theWindow)==EMS_PW_WINDOWKIND)
	{
		long refCon = GetWRefCon(theWindow);
		if (refCon != 0)
		{
			emsPlugwindowDataP wdPtr = *(emsPlugwindowDataH)refCon;

			//	If the plugin is new enough then check the emsPlugwindowData
			if (wdPtr->size > (&wdPtr->isDialogLayer - wdPtr))
				isModalPlugwindow = wdPtr->isDialogLayer;
		}
	}
	
	return isModalPlugwindow;
}

/************************************************************************
 * IsNonModalPlugwindow - is something a plugin window that wants to
 * be normal (i.e. not non-blocking modal)?
 ************************************************************************/
Boolean IsNonModalPlugwindow(WindowPtr theWindow)
{
	return (IsPlugwindow(theWindow) && !IsModalPlugwindow(theWindow));
}

/************************************************************************
 * PlugwindowEventFilter - filter an event for a plugin window
 ************************************************************************/
Boolean PlugwindowEventFilter(EventRecord *event)
{
	emsPlugwindowData wd;
	short module;
	emsPlugwindowEvent ed;
	WindowPtr theWindow = nil;
	Boolean result=true;
	
	switch(event->what)
	{
		case updateEvt:
		case activateEvt:
			theWindow = Event2Window(event);
			break;
		case mouseDown:
		case mouseUp:
			//	If modal window is frontmost and if it's not a dialog modal window, then bail
			FindWindow(event->where,&theWindow);
			if ( ModalWindow && ((ModalWindow != theWindow) || !IsModalPlugwindow(theWindow)) )
				return(true);
			break;
	}
	if (!theWindow) theWindow = MyFrontNonFloatingWindow();
	
	
	if (IsPlugwindow(theWindow))	
	{
		long refCon = GetWRefCon(theWindow);
		if (refCon != 0)
		{
			wd = **(emsPlugwindowDataH)refCon;
			SetSize(wd);
			if (PluginIDToIndex(wd.pluginID,&module))
			{
				Zero(ed);
				SetSize(ed);
				ed.nativeEvent = event;
				//Dprintf("\pem %x win %x wd.nativeWindow %x pID %d m %d;g",event->message,win,wd.nativeWindow,wd.pluginID,module);
				result = 0!=ems_plugwindow_event((*TLFL)[module].instance,&wd,&ed);
			}
		}
	}
	return(result);
}

/************************************************************************
 * PlugwindowEnable - grab flags for plugwindow enabling
 ************************************************************************/
void PlugwindowEnable(WindowPtr theWindow,long *flags)
{
	long refCon = GetWRefCon(theWindow);
	if (refCon != 0)
	{
		emsPlugwindowData wd = **(emsPlugwindowDataH)refCon;
		short module;
		emsMenuData md;
		
		*flags = 0;
		if (PluginIDToIndex(wd.pluginID,&module))
		{
			Zero(md);
			SetSize(md);
			SetSize(wd);
			ems_plugwindow_menu_enable((*TLFL)[module].instance,&wd,&md);
			*flags = md.menuMask;
		}
	}
}

/************************************************************************
 * PlugwindowMenu - do a menu command for a plugin window
 ************************************************************************/
Boolean PlugwindowMenu(WindowPtr theWindow,long select)
{
	long refCon = GetWRefCon(theWindow);
	if (refCon != 0)
	{
		emsPlugwindowData wd = **(emsPlugwindowDataH)refCon;
		short module;
		emsMenuData md;
		
		Zero(md);
		SetSize(md);
		switch(select)
		{
			case (FILE_MENU<<16)|FILE_CLOSE_ITEM: md.menuMask = EMS_MENU_FILE_CLOSE; break;
			case (FILE_MENU<<16)|FILE_SAVE_ITEM: md.menuMask = EMS_MENU_FILE_SAVE; break;
			case (EDIT_MENU<<16)|EDIT_UNDO_ITEM: md.menuMask = EMS_MENU_EDIT_UNDO; break;
			case (EDIT_MENU<<16)|EDIT_CUT_ITEM: md.menuMask = EMS_MENU_EDIT_CUT; break;
			case (EDIT_MENU<<16)|EDIT_COPY_ITEM: md.menuMask = EMS_MENU_EDIT_COPY; break;
			case (EDIT_MENU<<16)|EDIT_PASTE_ITEM: md.menuMask = EMS_MENU_EDIT_PASTE; break;
			case (EDIT_MENU<<16)|EDIT_CLEAR_ITEM: md.menuMask = EMS_MENU_EDIT_CLEAR; break;
			case (EDIT_MENU<<16)|EDIT_SELECT_ITEM: md.menuMask = EMS_MENU_EDIT_SELECTALL; break;
			default: return(false);
		}
		
		if (PluginIDToIndex(wd.pluginID,&module))
		{
			SetSize(wd);
			return(!ems_plugwindow_menu((*TLFL)[module].instance,&wd,&md));
		}
	}
	return(false);
}

/************************************************************************
 * PlugwindowClose - close a plug-in window
 ************************************************************************/
Boolean PlugwindowClose(WindowPtr theWindow)
{
	Boolean		windowWasClosed = false;
	long refCon = GetWRefCon(theWindow);
	if (refCon != 0)
	{
		emsPlugwindowData wd = **(emsPlugwindowDataH)refCon;
		short module;
		
		if (PluginIDToIndex(wd.pluginID,&module))
		{
			SetSize(wd);
			windowWasClosed = !ems_plugwindow_close((*TLFL)[module].instance,&wd);
		}
		else
		{
			DisposeWindow(theWindow);
			windowWasClosed = true;
		}
	}
	else
	{
		DisposeWindow(theWindow);
		windowWasClosed = true;
	}
	
	//	PopModalWindow and setting lastHilitedWinWP to 0 if necessary
	//	should have already been done in ETLPlugwindowUnRegister.
	//	Check to see if we need to do either of these now in case the
	//	plugin didn't call ETLPlugwindowUnRegister, or we had to close the
	//	window ourselves.
	if (IsModalPlugwindow(theWindow) && (theWindow == ModalWindow))
		PopModalWindow();
	
#ifdef	FLOAT_WIN
	if (theWindow == 	lastHilitedWinWP) 	lastHilitedWinWP = 0;
#endif	//FLOAT_WIN
	
	return (windowWasClosed);
}

/************************************************************************
 * PlugwindowDrag - drag tracking in a plugwindow
 ************************************************************************/
OSErr PlugwindowDrag(WindowPtr theWindow,DragTrackingMessage message,DragReference drag)
{
	long refCon = GetWRefCon(theWindow);
	if (refCon != 0)
	{
		emsPlugwindowData wd = **(emsPlugwindowDataH)refCon;
		emsPlugwindowDragData pd;
		short module;
		
		if (PluginIDToIndex(wd.pluginID,&module))
		{
			SetSize(wd);
			Zero(pd);
			SetSize(pd);
			pd.drag = drag;
			pd.message = message;
			return(ems_plugwindow_drag((*TLFL)[module].instance,&wd,&pd));
		}
	}
	return(dragNotAcceptedErr);
}


/************************************************************************
 * PlugwindowSendFakeEvent - only call for plugin windows
 ************************************************************************/
void PlugwindowSendFakeEvent(WindowPtr theWindow, UInt32 message, EventKind what, EventModifiers modifiers, Point * where)
{
	short module;
	emsPlugwindowEvent ed;
	EventRecord event;
	long refCon = GetWRefCon(theWindow);
	
	if (refCon != 0)
	{
		emsPlugwindowData wd = **(emsPlugwindowDataH)refCon;
		SetSize(wd);
		if (PluginIDToIndex(wd.pluginID,&module))
		{
			Zero(event);
			event.what = what;
			event.message = message;
			event.modifiers = modifiers;
			if (where)
				event.where = *where;
			Zero(ed);
			SetSize(ed);
			ed.nativeEvent = &event;
			ems_plugwindow_event((*TLFL)[module].instance,&wd,&ed);
		}
	}
}


/************************************************************************
 * PlugwindowActivate - 
 ************************************************************************/
void PlugwindowActivate(WindowPtr theWindow, Boolean active)
{
	if (IsPlugwindow(theWindow))	
	{
		#ifdef	FLOAT_WIN
			HiliteWindow(theWindow,active);
		#endif	//FLOAT_WIN
		
		PlugwindowSendFakeEvent(theWindow, (long)theWindow, activateEvt, active ? activeFlag : 0, nil);
	}
}


/************************************************************************
 * PlugwindowUpdate - if a window is a plugin window, update it
 ************************************************************************/
void PlugwindowUpdate(WindowPtr theWindow)
{
	if (IsPlugwindow(theWindow))
		PlugwindowSendFakeEvent(theWindow, (long)theWindow, updateEvt, 0, nil);
}


/***************************************************************************
 * ETLCreateMailBox - Callback to create a mailbox, but not for the importers
 ***************************************************************************/
static pascal short ETLCreateMailBox(emsCreateMailBoxDataP makeMailboxData)
{
	FSSpec folderSpec;
	FSSpec createdSpec;
	OSErr err = noErr;
	
	SLDisable();
	
	SetSize(*makeMailboxData);

	// Where do we make it?
	if (makeMailboxData->parentFolder)
	{
		err = SimpleResolveAlias(makeMailboxData->parentFolder,&folderSpec);
		if (!err) folderSpec.parID = SpecDirId(&folderSpec);
	}
	else
	{
		folderSpec.vRefNum = MailRoot.vRef;
		folderSpec.parID = MailRoot.dirId;
	}
	
	if (!err)
	{
		// What do we call it?
		err = FSMakeFSSpec(folderSpec.vRefNum,folderSpec.parID,makeMailboxData->name,&createdSpec);

		// die if something went wrong
		if (!err)
		{
			// It exists, check to see if it's what the caller expected
			Boolean isFolder, boolJunk;
			
			err = ResolveAliasFile(&createdSpec, True, &isFolder, &boolJunk);

			if ( !err && (isFolder != makeMailboxData->createFolder) )
				err = dupFNErr;
		}
		else if (err==fnfErr)
		{
			// Make it
			if (err=BadMailboxName(&createdSpec, makeMailboxData->createFolder)) err = bdNamErr;
			
			if (!err)
			{
				// let the menus and mailboxes window know about it
				BuildBoxMenus();
				MBTickle(nil,nil);
				
				// Make an alias to it
				FSMakeFSSpec(MailRoot.vRef, MailRoot.dirId, "", &folderSpec);
				NewAlias(&folderSpec, &createdSpec, &makeMailboxData->mailboxOrFolder);
			}
		}
	}
	
	SLEnable();
	
	// We be done
	return (err);
}

/***************************************************************************
 * ETLIsInAddressBook - Callback to create a mailbox, but not for the importers
 ***************************************************************************/
static pascal short ETLIsInAddressBook(emsIsInAddressBookDataP makeMailboxData)
{
	// To do: Marshall - please implement.
	return unimpErr;
}

/************************************************************************
 * Import Plugin support
 ************************************************************************/
/**********************************************************************
 * ETLImport - call the importer with the spcified ID
 **********************************************************************/
OSErr ETLImport(long id, ImportOperationEnum what, void *params, void *results)
{
	OSErr err = noErr;
	emsImporterData data;
	TLMPtr importer;
	tlInstance instance;
	
	//
	// find the importer
	//
		
	if (TLImporterList)
	{
		importer = &((*TLImporterList)[id]);
		if (importer)
		{
			instance = 	(&(*TLFL)[importer->module])->instance;
			if (!(err=!ComponentFunctionImplemented(instance,kems_importer_hookRtn)))
			{
				//
				// make the call
				//
				
				SetSize(data);
				data.what = what;
				data.params = params;
				data.results = results;
				
				PushProgress();
				err = ems_importer_hook(instance,&data);
				PopProgress(false);
			}
		}
		else	// no importer with that id
			err = TLR_CANT_TRANS;
	}
	else	// no importer translator handle
		err = TLR_CANT_TRANS;
	
	return (err);
}

/***************************************************************************
 * ETLQueryImporters - check importer plugins and see what can be imported
 ***************************************************************************/
OSErr ETLQueryImporters(ImportAccountInfoH *results, long id, Boolean search)
{
	OSErr err = noErr;
	short i, j;
		
	if (TLImporterList)
	{
		// Loop through all importers and call the Query function
		for (i=HandleCount(TLImporterList);i--;)
		{	
			if ((id == kUnspecifiedImportPluginId) || (id == i))
			{
				err = ETLImport(i, search?EMS_IMPORT_Query:EMS_IMPORT_Name_Query, (void *)TO_UPP_OR_NOT_TO_UPP(ETLProgress3), (void *)(results));
				if (err) ImportError(i, kImportSetupErr, kImportQueryError, err, __LINE__);
				
				// go through and fill in the id field of al new results
				for (j=HandleCount(*results);j--;)
					if ((**results)[j].id == -1) 
						(**results)[j].id = i;
			}
		}
	}
	else	// no importer translator handle
	{
		err = TLR_CANT_TRANS;
		ImportError(kUnspecifiedImportPluginId, kImportSetupErr, kImportMemError, err, __LINE__);
	}
	
	return (err);
}

/***************************************************************************
 * GetImporterAppIcon - return a handle to the impoter's app icon
 ***************************************************************************/
Handle GetImporterAppIcon(long id)
{
	Handle suite = nil;

	if (TLImporterList) 
		if (id <= HandleCount(TLImporterList))
			suite = (*TLImporterList)[id].suite;
			
	return (suite);
}

/***************************************************************************
 * GetImporterName - return the name of an importer
 ***************************************************************************/
void GetImporterName(long id, Str255 name)
{
	name[0] = 0;
	
	if (TLImporterList) 
		if (id <= HandleCount(TLImporterList))
			PCopy(name,*(*TLImporterList)[id].nameHandle);
}

/************************************************************************
 * ETLImportSignatures - Import an account's signatures
 ************************************************************************/
OSErr ETLImportSignatures(ImportAccountInfoP account)
{
	ImportSignaturesDataS makeSigData;

	Zero(makeSigData);
	
	SetSize(makeSigData);
	makeSigData.importSpec = account->importSpec;
	makeSigData.makeSig = TO_UPP_OR_NOT_TO_UPP(ETLMakeSig);
	makeSigData.progress = TO_UPP_OR_NOT_TO_UPP(ETLProgress3);
	
	return (ETLImport(account->id, EMS_IMPORT_Signatures, &makeSigData, nil));
}

/************************************************************************
 * ETLImportAddresses - Import an account's address book(s)
 ************************************************************************/
OSErr ETLImportAddresses(ImportAccountInfoP account)
{
	ImportAddressDataS addressImportData;
	
	Zero(addressImportData);
	
	SetSize(addressImportData);
	addressImportData.importSpec = account->importSpec;
	PCopy(addressImportData.accountName, account->accountName);
	addressImportData.makeAddressBook = TO_UPP_OR_NOT_TO_UPP(ETLMakeAddressBook);
	addressImportData.makeABEntry = TO_UPP_OR_NOT_TO_UPP(ETLMakeABEntry);
	addressImportData.progress = TO_UPP_OR_NOT_TO_UPP(ETLProgress3);
		
	return (ETLImport(account->id, EMS_IMPORT_Addresses, &addressImportData, nil));
}

/************************************************************************
 * ETLImportMail - Import an account's messages and mailboxes
 ************************************************************************/
OSErr ETLImportMail(ImportAccountInfoP account)
{
	ImportMailDataS mailImportData;
	
	Zero(mailImportData);
	
	SetSize(mailImportData);
	mailImportData.importSpec = account->importSpec;
	PCopy(mailImportData.accountName, account->accountName);
	mailImportData.makeMailbox = TO_UPP_OR_NOT_TO_UPP(ETLMakeMailbox);
	mailImportData.makeOutMess = TO_UPP_OR_NOT_TO_UPP(ETLMakeOutMess);
	mailImportData.makeMessage = TO_UPP_OR_NOT_TO_UPP(ETLMakeMessage);
	mailImportData.progress	   = TO_UPP_OR_NOT_TO_UPP(ETLProgress3);
	mailImportData.importMBox  = TO_UPP_OR_NOT_TO_UPP(ETLImportMbox);
	return (ETLImport(account->id, EMS_IMPORT_Mail, &mailImportData, nil));
}

/************************************************************************
 * ETLImportSettings - Import an account's settings
 ************************************************************************/
OSErr ETLImportSettings(ImportAccountInfoP account, ImportPersDataH *persData)
{
	OSErr err = noErr;
	
	*persData = NuHandle(0L);
	err = MemError();
	
	if ((err == noErr) && *persData && **persData)
	{
		err = ETLImport(account->id, EMS_IMPORT_Settings, &(account->importSpec), persData);
		if (err != noErr)
		{
			// an error occurred importing settings information
			ImportError(account->id, kImportSettingsErr, kImportSettingsError, err=eofErr, __LINE__);
			ZapHandle(*persData);
		}
	}
	else
	{
		// a memory related error occurred.
		ImportError(kUnspecifiedImportPluginId, kImportSettingsErr, kImportMemError, err, __LINE__);
	}	
	return (err);	
}

/***************************************************************************
 * ETLMakeSig - Callback to create a signature file
 ***************************************************************************/
static pascal short ETLMakeSig(emsMakeSigDataP makeSigData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMakeNewSigCalback(makeSigData->name, makeSigData->sigText));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}


/***************************************************************************
 * ETLImportMbox - Callback to import mail from an mbox file
 ***************************************************************************/
static pascal short ETLImportMbox(emsImportMboxDataP importMBoxData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = ImportMboxCallback ( &importMBoxData->sourceMBox, &importMBoxData->boxSpec );
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}

/***************************************************************************
 * ETLMakeAddressBook - Callback to create an address book file
 ***************************************************************************/
static pascal short ETLMakeAddressBook(emsMakeAddressBookDataP makeAddressBookData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMakeAddressBookCallback(makeAddressBookData->name));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}

/***************************************************************************
 * ETLMakeABEntry - Callback to create an address book entry
 ***************************************************************************/
static pascal short ETLMakeABEntry(emsMakeABEntryDataP makeABEntryData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMakeABEntryCallback(makeABEntryData->which, makeABEntryData->isGroup, makeABEntryData->nickName, makeABEntryData->addresses, makeABEntryData->notes));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}

/***************************************************************************
 * ETLMakeMailbox - Callback to create a mailbox
 ***************************************************************************/
static pascal short ETLMakeMailbox(emsMakeMailboxDataP makeMailboxData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMakeMailboxCallback(makeMailboxData->command, &(makeMailboxData->boxSpec), makeMailboxData->isFolder, makeMailboxData->noSelect));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}

/***************************************************************************
 * ETLMakeOutMess - Callback to create an outgoing message
 ***************************************************************************/
static pascal short ETLMakeOutMess(emsMakeOutMessDataP makeOutMessData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMakeOutMessCallback(makeOutMessData));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}

/***************************************************************************
 * ETLMakeMessage - Callback to create an outgoing message
 ***************************************************************************/
static pascal short ETLMakeMessage(emsMakeMessageDataP makeMessageData)
{
	short returnMe;
	SLDisable();	// Spotlight doesn't like emsapi callbacks, alas
	returnMe = (ImportMessageCallback(makeMessageData->ref, makeMessageData->offset, makeMessageData->len, makeMessageData->state, makeMessageData->attachments, &(makeMessageData->boxSpec)));
	SLEnable();	// Spotlight doesn't like emsapi callbacks, alas
	return returnMe;
}
#endif

#if TARGET_RT_MAC_CFM

/* For old-style calls to components */
enum {
	uppCallComponentProcInfo = kPascalStackBased
		| RESULT_SIZE(kFourByteCode)
		| STACK_ROUTINE_PARAMETER(1, kFourByteCode)
};

/* This global takes the routine parameters for each component function call */
tl_ComponentParameters tl_GCP;

/* Routine to put a copy of the component params on the stack and calls
CallComponentUPP */
ComponentResult Calltl_ComponentFunction(tl_ComponentParameters params)
{
	return CallComponentDispatch(/*tl_GCP.params.ems_translate_file.globals,*/&params);
}

/* This global takes the routine parameters for each component function call */
ems_ComponentParameters ems_GCP;

/* Routine to put a copy of the component params on the stack and calls
CallComponentUPP */
ComponentResult Callems_ComponentFunction(ems_ComponentParameters params)
{
	return CallComponentDispatch(/*ems_GCP.params.ems_translate_file.globals,*/&params);
}

#endif


#endif
