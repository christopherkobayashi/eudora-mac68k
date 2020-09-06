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

#include <Folders.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

// Copyright � 1996 by QUALCOMM Incorporated
#define FILE_NUM 79
#include <Gestalt.h>
#include "register.h"

extern Boolean HaveOT(void);
OSErr GatherInfo(AccuPtr accu,short vRef,long dirId);
OSErr GetTCPVersion(UPtr	versionStr);
void DoMonitorInfo(AccuPtr accu);

#ifndef NAG

//The registration screen is the same in the demo as in light.
#if defined(LIGHT)||defined(DEMO)
	#define LIGHT_REG
#endif

#define	REGISTRATION_DIALOG 20050
OSErr RegisterEudora(void);
void SetDialogFontAndSize(DialogPtr theDialog, short fontNum, short fontSize);
uLong RegVers(void);
void CountryPopup(DialogPtr dlog,short top,short left);
#ifndef	LIGHT_REG
Boolean RegNumValid(Str255 num);
#endif	//LIGHT_REG
void EnterRegValues(DialogPtr dlog,long ***itemsHandle);
void SuckRegValues(DialogPtr dlog,long **itemsHandle);

enum {
	REGISTER_BUTTON = 1,
	ASK_LATER_BUTTON,
	NEVER_REGISTER_BUTTON,
	TEXT_PICT,
	FIRST_NAME_ITEM,
	LAST_NAME,
	COMPANY_NAME,
	TITLE,
	ADDRESS1,
	ADDRESS2,
	CITY,
	STATE,
	ZIP_CODE,
	COUNTRY,
	PHONE,
	FAX,
//	EMAIL,
#ifndef	LIGHT_REG	//Pro-only reg fields
	REG_NUMBER,
	JUNKMAIL_CHECKBOX,
	PART_NUMBER,
#else		//LIGHT_REG Light/Demo reg fields
	USE_POPUP,
	DECISION_MAKER_POPUP,
#endif	//LIGHT_REG
	COUNTRY_POPUP=21,
} DIALOG_ITEMS;
	
	

#pragma segment Main
/**********************************************************************
 * 
 **********************************************************************/
void AutoRegister(void)
{
	uLong oldReg = GetPrefLong(PREF_REGISTER);
	uLong oldVers = GetPrefLong(PREF_REG_VERS);
	Boolean registered = (oldReg&4)!=0;
	Boolean sentAndRcvd = (oldReg&3)==3;
	Boolean regWithOld = (oldVers||MAJOR_VERSION>3)&&oldVers<RegVers();
	uLong lastRegAge = GMTDateTime()-(oldReg&~0xf);
#ifdef DEBUG
#define TRY_AGAIN	(RunType==Debugging ? 20 : (3600L*24*7))
#else
#define TRY_AGAIN	3600L*24*7	/* non-debug try again in a week */
#endif
	
	if (oldReg == 0x0fffffff) return;	// never register!
	if ((!registered || regWithOld) && sentAndRcvd && lastRegAge>TRY_AGAIN)
		RegisterEudoraHi();
	if (registered && !oldVers) SetPrefLong(PREF_REG_VERS,RegVers());
}

/**********************************************************************
 * 
 **********************************************************************/
uLong RegVers(void)
{
	uLong nowVers = MAJOR_VERSION;
#ifndef LIGHT
	nowVers *= 100;
#ifndef DEMO
	nowVers += 1;
#endif
#endif
	return(nowVers);
}

#pragma segment Register

/**********************************************************************
 * RegisterEudoraHi - run the register program
 **********************************************************************/
void RegisterEudoraHi(void)
{
	uLong oldReg = GetPrefLong(PREF_REGISTER);
	
	// Make sure only the dominant personality registers
	CurPers = PersList;
	UseResFile(SettingsRefN);
	
	switch(RegisterEudora())
	{
		case 0:
			SetPrefLong(PREF_REGISTER,(GMTDateTime()&~0xf)|(oldReg&0x3)|4);
			SetPrefLong(PREF_REG_VERS,RegVers());
			break;
		default:
			SetPrefLong(PREF_REGISTER,(GMTDateTime()&~0xf)|(oldReg&0x3));
			break;
	}
}

/**********************************************************************
 * RegisterSuccess - inform the registration process of the user's success
 **********************************************************************/
void RegisterSuccess(uLong whichKind)
{
	uLong oldReg = GetPrefLong(PREF_REGISTER);
	if (!(oldReg&whichKind)) SetPrefLong(PREF_REGISTER,oldReg|whichKind);
}

/**********************************************************************
 * RegisterEudora - the work goes here
 **********************************************************************/
OSErr RegisterEudora(void)
{
//
	DialogPtr	regDialog;
	short	itemHit;
	Str255	firstName;
	Str255	lastName;
	Str255	companyName;
	Str255	title;
	Str255	address1;
	Str255	address2;
	Str255	city;
	Str255	state;
	Str255	zip;
	Str255	country;
	Str255	phone;
	Str255	fax;
	Str255	scratch;
	//Str255	email;
#ifndef	LIGHT_REG	//Pro-only fields
	Str255	regNum;
	Str255	partNum;
#else	//LIGHT_REG	//Light-only fields
	Str255	use;
	Str255	decision;
#endif //LIGHT_REG
	Boolean	Okdisabled = true;
	ControlHandle	regButtonHandle;
	Handle	myMessageHandle = nil;
	Byte		CR='\r',comma=',',space = ' ';
	extern ModalFilterUPP DlgFilterUPP;
	StringHandle	tempStringHandle = nil;
	Accumulator accu;
	MyWindowPtr win;
	OSErr	theResult = 1;
	Boolean junk = True;
	Handle countryPopup;
	Rect countryPopupRect;
	short countryPopupType;
	long **itemsHandle=nil;
	UHandle uvn;
	FSSpec spec;
	SAVE_PORT;
	
	regDialog = (DialogPtr)GetNewMyDialog(REGISTRATION_DIALOG,nil,InFront);
	
	if (!regDialog)	return(WarnUser(CANT_REGISTER,ResError()));
	
	EnterRegValues(regDialog,&itemsHandle);

#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
	GetPref(regNum,PREF_SITE);
	SetDIText(regDialog,REG_NUMBER,regNum);
#endif	//LIGHT_REG

	SelIText(regDialog,FIRST_NAME_ITEM,0,REAL_BIG);
	regButtonHandle = GetDItemCtl(regDialog,REGISTER_BUTTON);
	SetMyCursor(arrowCursor);

	StartMovableModal(regDialog);
	SetDialogFontAndSize(regDialog,geneva,9);
	ShowWindow(regDialog);
	SetPort(GetDialogPort(regDialog));
	
#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
	SetDItemState(regDialog,JUNKMAIL_CHECKBOX,True);
#endif	//LIGHT_REG

	GetDItem(regDialog,COUNTRY_POPUP,&countryPopupType,&countryPopup,&countryPopupRect);

	do
	{
		EnableDItemIf(regDialog,REGISTER_BUTTON,!Okdisabled);
		SetDialogDefaultItem(regDialog,Okdisabled?0:REGISTER_BUTTON);
		MovableModalDialog(regDialog,DlgFilterUPP,&itemHit);
		switch (itemHit)
		{
			case FIRST_NAME_ITEM:
			case LAST_NAME:
#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
			case REG_NUMBER:
#endif	//LIGHT_REG
				GetDIText(regDialog,FIRST_NAME_ITEM,firstName);
				GetDIText(regDialog,LAST_NAME,lastName);
#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
				GetDIText(regDialog,REG_NUMBER,regNum);
				Okdisabled = !(*firstName && *lastName && *regNum);
#else		//LIGHT_REG
				Okdisabled = !(*firstName && *lastName);
#endif	//LIGHT_REG
				EnableDItemIf(regDialog,REGISTER_BUTTON,!Okdisabled);
				break;

#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
			case JUNKMAIL_CHECKBOX:
				SetDItemState(regDialog,itemHit,junk=(!GetDItemState(regDialog,itemHit)));
				break;
#endif	//LIGHT_REG
				
			case REGISTER_BUTTON:
				if (Okdisabled)
					itemHit = REAL_BIG;
				GetDIText(regDialog,FIRST_NAME_ITEM,firstName);
				GetDIText(regDialog,LAST_NAME,lastName);
				GetDIText(regDialog,COMPANY_NAME,companyName);
				GetDIText(regDialog,TITLE,title);
				GetDIText(regDialog,ADDRESS1,address1);
				GetDIText(regDialog,ADDRESS2,address2);
				GetDIText(regDialog,CITY,city);
				GetDIText(regDialog,STATE,state);
				GetDIText(regDialog,ZIP_CODE,zip);
				GetDIText(regDialog,COUNTRY,country);
				GetDIText(regDialog,PHONE,phone);
				GetDIText(regDialog,FAX,fax);
				//GetDIText(regDialog,EMAIL,email);
#ifndef	LIGHT_REG	//Electronic Registration - These fields missing from light reg screen
				GetDIText(regDialog,PART_NUMBER,partNum);
				GetDIText(regDialog,REG_NUMBER,regNum);
#else		//LIGHT_REG	- these fields are present only in Light
				GetDIPopup(regDialog,USE_POPUP,use);
				GetDIPopup(regDialog,DECISION_MAKER_POPUP,decision);
#endif	//LIGHT_REG
				break;
			case COUNTRY_POPUP:
				CountryPopup(regDialog,countryPopupRect.top,countryPopupRect.left);
				break;
			default:
				break;

		}
		
#ifndef LIGHT_REG	//no reg field in Light		
	//make sure the reg number at least looks like it's valid
	if (itemHit == REGISTER_BUTTON)
		if (!RegNumValid(regNum))
		{
			itemHit = REG_NUMBER;
			Alert(INVALID_REG_NUM_ALRT,0);
		}
#endif	//LIGHT_REG

	} while (!itemHit || itemHit > NEVER_REGISTER_BUTTON);
	
	if (itemHit == REGISTER_BUTTON) SuckRegValues(regDialog,itemsHandle);
	
	EndMovableModal(regDialog);
	DisposDialog_(regDialog);
	REST_PORT;


	if (itemHit == REGISTER_BUTTON)
	{
#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
		SetPref(PREF_SITE,regNum);
#endif	//LIGHT_REG

		AccuInit(&accu);
		//GetRString(separatorLine,TAG_SEPARATOR);
	
		// Put in do not modify string
		AccuAddRes(&accu,RegMessageTagsStrn+regDO_NOT_MODIFY);
	
		// Put in first name string
		AccuAddRes(&accu,RegMessageTagsStrn+regFIRST_NAME);
		AccuAddStr(&accu,firstName);
		AccuAddChar(&accu,CR);
		
		// Put in last name string
		AccuAddRes(&accu,RegMessageTagsStrn+regLAST_NAME);
		AccuAddStr(&accu,lastName);
		AccuAddChar(&accu,CR);

		// Put in company name string
		AccuAddRes(&accu,RegMessageTagsStrn+regCOMPANY_NAME);
		AccuAddStr(&accu,companyName);
		AccuAddChar(&accu,CR);

		// Put in title string
		AccuAddRes(&accu,RegMessageTagsStrn+regTITLE);
		AccuAddStr(&accu,title);
		AccuAddChar(&accu,CR);

		// Put in address1 string
		AccuAddRes(&accu,RegMessageTagsStrn+regADDRESS1);
		AccuAddStr(&accu,address1);
		AccuAddChar(&accu,CR);

		// Put in address2 string
		AccuAddRes(&accu,RegMessageTagsStrn+regADDRESS2);
		AccuAddStr(&accu,address2);
		AccuAddChar(&accu,CR);

		// Put in city string
		AccuAddRes(&accu,RegMessageTagsStrn+regCITY);
		AccuAddStr(&accu,city);
		AccuAddChar(&accu,CR);

		// Put in state string
		AccuAddRes(&accu,RegMessageTagsStrn+regSTATE);
		AccuAddStr(&accu,state);
		AccuAddChar(&accu,CR);

		// Put in zip string
		AccuAddRes(&accu,RegMessageTagsStrn+regZIP);
		AccuAddStr(&accu,zip);
		AccuAddChar(&accu,CR);

		// Put in country string
		AccuAddRes(&accu,RegMessageTagsStrn+regCOUNTRY);
		AccuAddStr(&accu,country);
		AccuAddChar(&accu,CR);

		// Put in phone string
		AccuAddRes(&accu,RegMessageTagsStrn+regPHONE);
		AccuAddStr(&accu,phone);
		AccuAddChar(&accu,CR);

		// Put in fax string
		AccuAddRes(&accu,RegMessageTagsStrn+regFAX);
		AccuAddStr(&accu,fax);
		AccuAddChar(&accu,CR);

#ifndef	LIGHT_REG	//Electronic Registration - do not include this field
		// Put in registration number
		AccuAddRes(&accu,RegMessageTagsStrn+regREGISTRATION_CODE);
		AccuAddStr(&accu,regNum);
		AccuAddChar(&accu,CR);
		// Put in part number
		AccuAddRes(&accu,RegMessageTagsStrn+regPART_NUMBER);
		AccuAddStr(&accu,partNum);
		AccuAddChar(&accu,CR);
		// Does this user want junk mail?
		AccuAddRes(&accu,RegMessageTagsStrn+regJUNK_MAIL);
		AccuAddStr(&accu,(junk ? YesStr : NoStr));
		AccuAddChar(&accu,CR);
#else		//LIGHT_REG
		// Put in where LIGHT is being used
		AccuAddRes(&accu,RegMessageTagsStrn+regREGISTRATION_CODE);
		AccuAddStr(&accu,use);
		AccuAddChar(&accu,CR);
		// Put in if our user has any buying power
		AccuAddRes(&accu,RegMessageTagsStrn+regPART_NUMBER);
		AccuAddStr(&accu,decision);
		AccuAddChar(&accu,CR);
#endif	//LIGHT_REG
	
		// Put in Eudora Version
		AccuAddRes(&accu,RegMessageTagsStrn+regEUDORA_VERSION);
		VersString(scratch);
		AccuAddStr(&accu,scratch);
		AccuAddChar(&accu,CR);
		
		// Put in UVN, if any
		if (!GetFileByRef(AppResFile,&spec))
		{
			if (!FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(lastName,STUFF_FOLDER),&spec))
			{
				IsAlias(&spec,&spec);
				spec.parID = SpecDirId(&spec);
				if (!FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(lastName,UVN),&spec))
					if (!Snarf(&spec,&uvn,1 K))
					{
						AccuAddStr(&accu,spec.name);
						AccuAddChar(&accu,':');
						AccuAddChar(&accu,' ');
						AccuAddHandle(&accu,uvn);
						AccuAddChar(&accu,CR);
						ZapHandle(uvn);
					}
			}
		}

		AccuAddChar(&accu,CR);

		AccuTrim(&accu);

		if (win=DoComposeNew(0))
		{
			MessHandle messH = Win2MessH(win);
			GetRString(scratch,RegMessageTagsStrn+regADDRESS);
			SetMessText(messH,TO_HEAD,scratch+1,*scratch);
			GetRString(scratch,RegMessageTagsStrn+regSUBJECT_LINE);
			SetMessText(messH,SUBJ_HEAD,scratch+1,*scratch);
			SetMessText(messH,0,LDRef(accu.data),GetHandleSize_(accu.data));
			SumOf(messH)->sigId = SIG_NONE;
/*			if (!QueueMessage((*messH)->tocH,(*messH)->sumNum,PrefIsSet(PREF_AUTO_SEND)?kEuSendNow:kEuSendNext,0))
			{
				SetAlertBeep(False);
				Aprintf(OK_ALRT,Normal,PrefIsSet(PREF_AUTO_SEND)?REGISTRATION_SENT:REGISTRATION_QUEUED);
				SetAlertBeep(True);
			}*/
			if (!QueueMessage((*messH)->tocH,(*messH)->sumNum,kEuSendNext,0,true))
			{
				SetAlertBeep(False);
				Aprintf(OK_ALRT,Normal,REGISTRATION_QUEUED);
				SetAlertBeep(True);
				theResult = noErr;
			}

			UL(accu.data);
			AccuZap(accu);
		}
	}
	else if (itemHit == NEVER_REGISTER_BUTTON)
		theResult = noErr;
	ZapHandle(itemsHandle);
	return(theResult);
}

/************************************************************************
 * EnterRegValues - restore registration values from last time
 ************************************************************************/
void EnterRegValues(DialogPtr dlog,long ***itemsHandle)
{
	short item = CountDITL(dlog);
	short type;
	Handle itemH;
	long pref;
	Str255 text;
	Boolean setSelect = True;
	Rect r;
	
	if (*itemsHandle = NuHandleClear(sizeof(long)*(item+1)))
	{		
		for (;item;item--)
		{
			GetDItem(dlog,item,&type,&itemH,&r);
			switch (type)
			{
				case editText:
					GetDIText(dlog,item,text);
					StringToNum(text,&pref);
					GetRString(text,RegValStrn+pref);
					SetDIText(dlog,item,text);
					(**itemsHandle)[item] = pref;
					break;
			}
		}
	}
}

/************************************************************************
 * SuckRegValues - save registration values
 ************************************************************************/
void SuckRegValues(DialogPtr dlog,long **itemsHandle)
{
	short item = CountDITL(dlog);
	short type;
	Handle itemH;
	Rect r;
	Str255 text;
	Boolean setSelect = True;
	
	if (itemsHandle)
	{		
		for (;item;item--)
		{
			if ((*itemsHandle)[item])
			{
				GetDItem(dlog,item,&type,&itemH,&r);
				switch (type)
				{
					case editText:
						GetDIText(dlog,item,text);
						ChangeStrn(RegValStrn,(*itemsHandle)[item],text);
						break;
				}
			}
		}
	}
	MyUpdateResFile(SettingsRefN);
}
#endif

/**********************************************************************
 * 
 **********************************************************************/
OSErr AddConfig(AccuPtr accu,Boolean doShort)
{
	Accumulator tempAccu;
	Str255 scratch, tempStr;
	Str255 machineType;
	long gestaltValue;
	short	theVRef,
				majorVersion;
	long	theDirID;
	char CR = '\015';
	
	// Put in sys info tag		
	AccuAddRes(accu,RegMessageTagsStrn+regSYSTEM_INFORMATION);

	// Put in system version		
	AccuAddRes(accu,RegMessageTagsStrn+regSYSTEM_VERSION);
	gestaltValue = 0;
	Gestalt(gestaltSystemVersion, &gestaltValue);
	
	majorVersion = gestaltValue >> 8;
	majorVersion = 10 * (majorVersion >> 4) + (majorVersion & 0x0f);
	ComposeString(scratch,"\p%d.%d.%d",majorVersion, (gestaltValue >> 4) & 0x0f,gestaltValue & 0x0f);
	AccuAddStr(accu,scratch);
	AccuAddChar(accu,CR);

	// Put in processor type		
	AccuAddRes(accu,RegMessageTagsStrn+regPROCESSOR);
	
	// If we can't get the processor type, we'll fall through to the default condition below
	GetRString(scratch,RegMessageTagsStrn+regUNIDENTIFIED_MACHINE);

	if (Gestalt(gestaltNativeCPUtype, &gestaltValue)) 
	{	
		if (Gestalt(gestaltProcessorType, &gestaltValue)) gestaltValue = -1;
		switch (gestaltValue)
		{	
			case gestalt68000:
				PCopy(scratch,"\p68000");
				break;
			case gestalt68010:
				PCopy(scratch,"\p68010");
				break;
			case gestalt68020:
				PCopy(scratch,"\p68020");
				break;
			case gestalt68030:
				PCopy(scratch,"\p68030");
				break;
			case gestalt68040:
				PCopy(scratch,"\p68040");
				break;
			default:
				ComposeRString(scratch,RegMessageTagsStrn+regUNIDENTIFIED_MACHINE,gestaltValue);
				break;
		}
	}
	else
	{
		// get PPC Processor
		if (Gestalt(gestaltNativeCPUtype, &gestaltValue)) gestaltValue = -1;
		switch (gestaltValue)
		{
			case 0:
				PCopy(scratch,"\p68000");
				break;
			case 1:
				PCopy(scratch,"\p68010");
				break;
			case 2:
				PCopy(scratch,"\p68020");
				break;
			case 3:
				PCopy(scratch,"\p68030");
				break;
			case 4:
				PCopy(scratch,"\p68040");
				break;
			case gestaltCPU601:
				PCopy(scratch,"\pPowerPC 601");
				break;
			case gestaltCPU603:
				PCopy(scratch,"\pPowerPC 603");
				break;
			case gestaltCPU604:
				PCopy(scratch,"\pPowerPC 604");
				break;
			case gestaltCPU603e:
				PCopy(scratch,"\pPowerPC 603e");
				break;
			case gestaltCPU603ev:
				PCopy(scratch,"\pPowerPC 603ev");
				break;
			case gestaltCPU750:
				PCopy(scratch,"\pPowerPC 750");
				break;
			case gestaltCPU604e:
				PCopy(scratch,"\pPowerPC 604e");
				break;
			case gestaltCPU604ev:
				PCopy(scratch,"\pPowerPC 604ev");
				break;
			case gestaltCPUG4:
				PCopy(scratch,"\pPowerPC G4");
				break;
			default:
				ComposeRString(scratch,RegMessageTagsStrn+regUNIDENTIFIED_MACHINE,gestaltValue);
				break;
		}
	}

	AccuAddStr(accu,scratch);
	AccuAddChar(accu,CR);


	// Put in physical RAM installed		
	AccuAddRes(accu,RegMessageTagsStrn+regPHYSICAL_RAM_INSTALLED);
	gestaltValue = 0;
	Gestalt(gestaltPhysicalRAMSize, &gestaltValue);
	gestaltValue/=1024;
	ComposeString(scratch,"\p%d,%dK",gestaltValue/1000,gestaltValue%1000);
	AccuAddStr(accu,scratch);
	AccuAddChar(accu,CR);

	// Put in total RAM installed		
	AccuAddRes(accu,RegMessageTagsStrn+regRAM_INSTALLED);

	gestaltValue = 0;
	Gestalt(gestaltLogicalRAMSize, &gestaltValue);
	gestaltValue/=1024;
	ComposeString(scratch,"\p%d,%dK",gestaltValue/1000,gestaltValue%1000);
	AccuAddStr(accu,scratch);
	AccuAddChar(accu,CR);
	
	// Put in Eudora Pro Version
	AccuAddRes(accu,RegMessageTagsStrn+regEUDORA_VERSION);
	VersString(tempStr);
	AccuAddStr(accu,tempStr);
	AccuAddChar(accu,CR);

#ifdef	NEVER	// skip the reg code in all system configurations now -jdboyd	
	// Put in reg number
	if (*GetPref(tempStr,PREF_SITE))
	{
		AccuAddRes(accu,RegMessageTagsStrn+regREGISTRATION_CODE);
		AccuAddStr(accu,tempStr);
		AccuAddChar(accu,CR);
	}
#endif
	
	// TCP/IP Version
	AccuAddRes(accu,RegMessageTagsStrn+regTCP_IP_VERSION);
	if (HaveOT())
			GetRString(scratch,RegMessageTagsStrn+regOT_TITLE);
	else
			GetRString(scratch,RegMessageTagsStrn+regMACTCP_TITLE);

	AccuAddStr(accu,scratch);
	*scratch = 0;
	GetTCPVersion(scratch);
	if (*scratch)
		AccuAddStr(accu,scratch);
	AccuAddChar(accu,CR);

	// Skip 'get Info' style stuff for OS X
	if (!HaveOSX ()	) {
		AccuAddStr(accu,ComposeRString(scratch,RegMessageTagsStrn+regCURRENT_MEM,CurrentSize()/(1 K)));
		AccuAddStr(accu,ComposeRString(scratch,RegMessageTagsStrn+regGETINFO_MEM,DefaultSize()/(1 K)));
		AccuAddStr(accu,ComposeRString(scratch,RegMessageTagsStrn+regSUGGEST_MEM,EstimatePartitionSize(false)/(1 K)));
	}
	DoMonitorInfo(accu);
	
	if (!doShort && !HaveOSX ()) {

		AccuAddChar(accu,CR);
		AccuAddRes(accu,RegMessageTagsStrn+regEXTEN_AND_CPS);

		AccuAddRes(accu,RegMessageTagsStrn+regSYSTEM_FOLDER);

		AccuInit(&tempAccu);
		FindFolder(kOnSystemDisk,kSystemFolderType,false,&theVRef,&theDirID);
		GatherInfo(&tempAccu,theVRef,theDirID);
		AccuTrim(&tempAccu);
		AccuAddHandle(accu,tempAccu.data);
		AccuZap(tempAccu);
		AccuAddChar(accu,CR);


		AccuAddRes(accu,RegMessageTagsStrn+regEXTEN_FOLDER);

		AccuInit(&tempAccu);
		FindFolder(kOnSystemDisk,kExtensionFolderType,false,&theVRef,&theDirID);
		GatherInfo(&tempAccu,theVRef,theDirID);
		AccuTrim(&tempAccu);
		AccuAddHandle(accu,tempAccu.data);

		AccuZap(tempAccu);
		
		AccuAddChar(accu,CR);
		AccuAddRes(accu,RegMessageTagsStrn+regCPS_FOLDER);

		AccuInit(&tempAccu);
		FindFolder(kOnSystemDisk,kControlPanelFolderType,false,&theVRef,&theDirID);
		GatherInfo(&tempAccu,theVRef,theDirID);
		AccuTrim(&tempAccu);
		AccuAddHandle(accu,tempAccu.data);
		AccuZap(tempAccu);
	}
	
	if (!doShort)
	{
		// Show modified settings
		short	i;
		StringPtr	sDefault=scratch,sCurrent=tempStr,sBuffer=machineType;	// Reuse some strings
		Str255	sHelp;
				
		AccuAddChar(accu,CR);
		AccuAddRes(accu,RegMessageTagsStrn+regCHANGED_SETTINGS);
		for (i=PREF_0;i<PREF_STRN_LIMIT;i++)
		{
			if (PrefAudit(i))
			{
				GetPref(sCurrent,i);
				GetDefPref(sDefault,i);
				if (!StringSame(sCurrent,sDefault))
				if (*sDefault || (!StringSame(sCurrent,"\pn") && !StringSame(sCurrent,"\p0")))	// Don't count changes from blank to "n" or zero
				{
					PrefHelpString(i,sHelp,true);
					if (*sDefault)
						ComposeString(sBuffer,"\p<x-eudora-setting:%d> %p (%p) \"%p\"",i,sCurrent,sDefault,sHelp);
					else			
						ComposeString(sBuffer,"\p<x-eudora-setting:%d> %p \"%p\"",i,sCurrent,sHelp);
					AccuAddStr(accu,sBuffer);			
					AccuAddChar(accu,CR);
				}
			}
		}
		
		AccuAddChar(accu,CR);
	}
	AccuAddRes(accu,RegMessageTagsStrn+regEND_INFO);
	AccuAddChar(accu,CR);
	AccuAddChar(accu,CR);
	AccuTrim(accu);
	return(noErr);
}

/************************************************************************
 * GetTCPVersion - gets MacTCP or Open Transport Version
 ************************************************************************/
OSErr GetTCPVersion(UPtr	versionStr)
{
	CInfoPBRec hfi;
	Str31 name;
	short refN;
	short	vRef;
	long dirId;
	VersRecHndl versInfo;
	short oldResF = CurResFile();
	long result;
	OSErr err;
	short fileOTVers, gestaltOTVers;
		
	FindFolder(kOnSystemDisk,kControlPanelFolderType,false,&vRef,&dirId);

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while(!DirIterate(vRef,dirId,&hfi))
		if (hfi.hFileInfo.ioFlFndrInfo.fdType=='cdev' && (
				 hfi.hFileInfo.ioFlFndrInfo.fdCreator=='ztcp' ||
				hfi.hFileInfo.ioFlFndrInfo.fdCreator=='mtcp'))
		{
				SetResLoad(false);
				refN=HOpenResFile(vRef, dirId, name, fsRdPerm);
				SetResLoad(true);
				if (refN>=0 && CurResFile()==refN)
				{
					SetResLoad(true);

					// get info
 					versInfo = (VersRecHndl)GetResource('vers', 1);
					*versionStr = 0;
					if (versInfo)
						{
							HLock((Handle)versInfo);
							PCopy(versionStr,(**versInfo).shortVersion);
							UL(versInfo);
						}
					
					// and get the numerical representation of the version as well
					fileOTVers = (((*versInfo)->numericVersion).majorRev << 8) + ((*versInfo)->numericVersion).minorAndBugRev;
					
					CloseResFile(refN);
				}
	}
	UseResFile (oldResF);
	
	// Under OS 9, it's possible to have an older (2.5) TCP/IP control panel, and a newer
	// (2.6) OT library installed.  Let's check the OT version via gestalt as well, if OT is installed
	if (OTIs)
	{
	 	err = Gestalt(gestaltOpenTptVersions, &result);
	 	if (err == noErr)
	 	{
	 		gestaltOTVers = HiWord(result);
	 		if (gestaltOTVers > fileOTVers)
	 		{
	 			// Gestalt returned a newer version of OT.  Let's return that.
	 			ShortVersString(gestaltOTVers, versionStr);
	 		}
	 	}
	}
	
	return(noErr);
}

/************************************************************************
 * GatherInfo - gathers info about extensions and cps
 ************************************************************************/
OSErr GatherInfo(AccuPtr accu,short vRef,long dirId)
{
	CInfoPBRec hfi;
	Str31 name;
	Str255	versionStr;
	Str255 	infoLine;
	short refN;
	VersRecHndl versInfo;
	Boolean interesting;
	short oldResF = CurResFile();

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while(!DirIterate(vRef,dirId,&hfi))
			{
				interesting = TypeIsOnList(hfi.hFileInfo.ioFlFndrInfo.fdType,'EuRg');
				CycleBalls();
				SetResLoad(false);
				refN=HOpenResFile(vRef, dirId, name, fsRdPerm);
				SetResLoad(true);
				if (refN>=0 && CurResFile()==refN)
				{
					// Check to see if it has an INIT resource, if so, get the vers info
					if ((Count1Resources('INIT') && hfi.hFileInfo.ioFlFndrInfo.fdType != 'zsys') || interesting) 
					{
	 					versInfo = (VersRecHndl)GetResource('vers', 1);
						*versionStr = 0;
						if (versInfo)
								PCopy(versionStr,(**versInfo).shortVersion);

						ComposeString(infoLine,"\p%o/%o\t%p\t%p\r",hfi.hFileInfo.ioFlFndrInfo.fdType,
							hfi.hFileInfo.ioFlFndrInfo.fdCreator,versionStr,name);
						AccuAddStr(accu,infoLine);
					}
					CloseResFile(refN);
				}
			}
	UseResFile (oldResF);
	return(noErr);
}

/**********************************************************************
 * DoMonitorInfo - video setup information.  Same as the adwin reports
 **********************************************************************/
void DoMonitorInfo(AccuPtr accu)
{
	Str255 scratch;
	Rect rScreen;
	
	GetQDGlobalsScreenBitsBounds(&rScreen);
	AccuAddStr(accu,ComposeRString(scratch,RegMessageTagsStrn+regMAIN_MONITOR,RectDepth(&rScreen),RectWi(rScreen),RectHi(rScreen)));
}

#ifndef NAG
void SetDialogFontAndSize(DialogPtr theDialog, short fontNum, short fontSize)
{
	FontInfo	f;
	GrafPtr thePort;
	
	
	GetPort(&thePort);
	SetPort(GetDialogPort(theDialog));
	
	// set up the port info
	TextFont(fontNum);
	TextSize(fontSize);

	// now deal with the static & edit text issues
	GetFontInfo(&f);
	(*((DialogPeek)theDialog)->textH)->txFont = applFont;
	(*((DialogPeek)theDialog)->textH)->txSize = 9;
	(*((DialogPeek)theDialog)->textH)->lineHeight = f.ascent + f.descent + f.leading;
	(*((DialogPeek)theDialog)->textH)->fontAscent = f.ascent;
	DrawDialog(theDialog);
	SetPort(thePort);
}

/************************************************************************
 * CountryPopup - pop up the menu for choosing a country quickly
 ************************************************************************/
void CountryPopup(DialogPtr dlog,short top,short left)
{
	MenuHandle mh;
	Str255 value;
	short item;
	long sel;
	Point pt;
	Handle countryTextItem;
	short	countryTextItemType;
	Rect countryTextItemRect;
	GrafPtr origPort;

	pt.h = left; pt.v = top; 
	
	GetPort(&origPort);
	SetPort(GetDialogPort(dlog));	
	LocalToGlobal(&pt);
	SetPort(origPort);
	
	value[0]=0;
	GetDItem(dlog,COUNTRY,&countryTextItemType,&countryTextItem,&countryTextItemRect);
	GetDialogItemText(countryTextItem,value);
	
	if (mh=GetMenu(COUNTRY_MENU))
	{
		if (*value && !(item=FindItemByName(mh,value)))
		{
			AppendMenu(mh,"\p-");
			DisableItem(mh,CountMItems(mh));
			MyAppendMenu(mh,value);
			item = CountMItems(mh);
		}
		else if (*value==0) item = 0;
		InsertMenu(mh,-1);
		sel = AFPopUpMenuSelect(mh,pt.v,pt.h,item);
		if (sel&0xffff && sel&0xff && (sel&0xff)!=item)
		{
			MyGetItem(mh,sel&0xff,value);
			SetDialogItemText(countryTextItem,value);
		}
		DeleteMenu(COUNTRY_MENU);
		ReleaseResource_(mh);
	}
}

#ifndef	LIGHT_REG	//not needed in Light or demo
/************************************************************************
 * RegNumValid - do a quick check and see if the registration number is 
 * anything like correct.  It must be 6-8 numeric characters long.
 ************************************************************************/
Boolean RegNumValid(Str255 num)
{
	unsigned char *scan = num;
	
	//Is the reg number the right length?
	if ((*scan<6)||(*scan>8))
		return false;
	
	//Is there anything other than digits in the number?
	while (++scan<=(num+num[0]))
		if ((*scan<'0')||(*scan>'9'))
			return false;

	return true;
}
#endif	//LIGHT_REG
#endif
