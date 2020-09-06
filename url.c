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

#define FILE_NUM 66
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#pragma segment URL
#include "url.h"
#include <LaunchServices.h>

Boolean gHelpWinClick = false;

typedef struct
{
	Str31 scheme;
	Boolean isSite;
	Str127 site;
	Str255 path;
	Boolean isQuery;
	Str255 query;
	Boolean isFragment;
	Str255 fragment;
} DeepURL, *DeepURLPtr, **DeepURLHandle;

OSErr SFURLApp(PStr proto,AliasHandle *alias);
OSErr SFURLAppStd(PStr proto,AliasHandle *alias, SFTypeList types, Str255 prompt, FSSpec *spec, URLHookOptionsPtr optionsPtr, Boolean *good);
OSErr BuildGURL(ProcessSerialNumberPtr psn,PStr url,AppleEvent *ae,Boolean newWindow);
OSErr BuildGURLPtr(ProcessSerialNumberPtr psn,UPtr url,short length,AppleEvent *ae,Boolean newWindow);
pascal OSErr HandleAEURL(AppleEvent *event,AppleEvent *reply,long refCon);
Boolean IsURLCharLo(Byte c);
OSErr MailtoURL(PStr query,AEDescList *dox,Boolean spool);
OSErr FileURL(PStr path);
void DeepURLParse(PStr url, DeepURLHandle duh);
PStr URLCombinePaths(PStr into,PStr base,PStr rel);
PStr RemLastComponent(PStr path);
PStr DuhString(PStr url,DeepURLHandle duh);
Byte URLToken(PStr url,PStr token,UPtr *spot);
Boolean TrailingDotDot(PStr string);
PStr URLPathEscape(PStr path);
OSErr ClearLink(PETEHandle pte,Boolean clear,long start, long end,long *urlStart, long *newStart, long *newEnd,PStr oldURL);
Handle URLVarSub(Ptr *string, long *len);
Boolean IsHostname(PStr string);
OSErr HostnameURL(PStr hostname);
OSErr JumpURL(PStr action);
void TurnBareLFs2BareCRs(Ptr s,long len);
void SubVar(Handle h,PStr find,PStr replace);
pascal Boolean URLDlgFilter(DialogPtr dgPtr,EventRecord *event,short *item);
Boolean PeteLinkAt(PETEHandle pte,long offset,Handle *hURL,PStr link);
void URLHelpTag(PETEHandle pte,Point mouse);
short URLIsNaughty(PStr linkText,Handle hURL,Boolean interact);
short URLIsNaughtyLo(PStr linkText,Handle hURL,PStr urlHost,PStr linkHost);
PStr StripCountryFromHost(PStr stripped,PStr clothed);

#define IsSchemeChar(x)	(IsWordChar[x] || x==':' || x=='.' || x=='-' || x=='+' || '0'<=x && x<='9')
#define IsURLChar(x) (IsWordChar[x] || IsURLCharLo(x))
#define IsSlackURLChar(x)	(IsURLChar(x)&&!IsLWSP(x)&&(x)!='('&&(x)!=')')
#define IsHostChar(x)	(IsSchemeChar(x)) // This used to disallow colon, too.  I'm not sure why, but it messed up host:port syntax

#define kW3Class	'WWW!'
#define kOpenURL	'OURL'
#define keyW3Window	'WIND'

/**********************************************************************
 * 
 **********************************************************************/
Boolean IsURLCharLo(Byte c)
{
	static Str63 urlOK;
	
	if (!*urlOK) GetRString(urlOK,URL_IN_OK);
	
	return(IsLWSP(c) || '0'<=c && c<='9' || PIndex(urlOK,c));
}

/************************************************************************
 * HandleAEURL - handle a URL event
 ************************************************************************/
pascal OSErr HandleAEURL(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDesc text, list;
	Str63 proto;
	Handle hURL;
	Handle result;
	ProcessSerialNumber psn;
	extern void SystemEudoraFolder(void);
	Boolean spool;
	
	FRONTIER;	
	if (ModalWindow) return(userCanceledErr);

	if (!SettingsRefN) SystemEudoraFolder();
	NullADList(&text,&list,nil);
	if (!(err = AEGetParamDesc(event,keyDirectObject,typeChar,&text)))
	{
		AEGetParamDesc(event,kURLDocumentList,typeAEList,&list);
		spool = GetAEDefaultBool(event,keyEuSpool,false);

#ifdef NETSCAPE_WERE_NOT_BRAINDEAD
		if (!(err=GotAERequired(event)))
#endif
		{
			AEDesc coerced;
			Boolean wasCoerced;
			UPtr	pURL;
			uLong	lenURL;

			if (!(hURL = GetAEText(&text,&coerced,&wasCoerced)))
				err = errAEEventNotHandled;
			else
			{
				pURL = LDRef(hURL);
				lenURL = GetHandleSize(hURL);
				if (lenURL && pURL[0]=='<')
				{
					//	Get rid of brackets
					pURL++;
					lenURL--;
					if (lenURL && pURL[lenURL-1]=='>') lenURL--;
				}
				if (!(err=ParseProtocolFromURLPtr(pURL,lenURL,proto)))
				{
					err=OpenLocalURLPtr(pURL,lenURL,&result,&list,spool);
					if (!err)
					{
						if (refCon==kURLFetch)
						{
							err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(result),GetHandleSize(result));
							UL(result);
						}
					}
					else
#ifdef TWO
						err = (refCon==kURLFetch||err!=fnfErr) ? err : OpenOtherURLPtr(proto,pURL,lenURL);
#else
						err = errAEEventNotHandled;
#endif
				}
				if (wasCoerced)	AEDisposeDesc(&coerced);	
				AEDisposeDescDataHandle(hURL); 
			}
		}
	}
	DisposeADList(&text,&list,nil);
	
		
	/*
	 * frontify ourselves
	 */
	if (!err) SetFrontProcess(CurrentPSN(&psn));

	return(err);
}

/**********************************************************************
 * OpenLocalURLPtr - open a URL in Eudora
 **********************************************************************/
OSErr OpenLocalURLPtr(PStr url,long len,Handle *result,AEDescList *dox,Boolean spool)
{
	ProtocolEnum protocol;
	Str255 proto,host,query;
	Ptr	queryPtr;
	long	queryLen;
	OSErr err;
	
	// weed out the payment urls
	MakePStr(proto,url,len);
	if (EqualStrRes(proto,BUYING_BY_HTTP) || EqualStrRes(proto,BUYING_BY_MAIL))
	{
		StartPaymentProcess();
		return noErr;
	}
		
	if (!(err=ParseURLPtr(url,len,proto,host,&queryPtr,&queryLen)))
	{
		if (queryPtr && queryLen)
			MakePPtr(query,queryPtr,queryLen);
		else
			*query = 0;
		protocol = FindSTRNIndex(ProtocolStrn,proto);
		FixURLString(host);
		if (protocol!=proMail && protocol!=proFile && protocol!=proCompFile) FixURLString(query);
		if ((protocol==proFinger||protocol==proMail) && *host && !*query)
		{
			PCopy(query,host);
			*host = 0;
		}
		err = fnfErr;
		if (protocol)
		{
			err = 1;
			if (*query || protocol==proPh || protocol==proPh2 || protocol==proLDAP || protocol==proMail)
			{
				if (protocol==proFinger || protocol==proPh || protocol==proPh2 || protocol==proLDAP)
				{
					/*
					 * open the finger/ph window
					 */
					if (!(err = OpenPh(nil)))
					{
						err = PhURLLookup(protocol,host,query,result);
					}
				}
				else if (protocol==proMail)
					err = MailtoURLPtr(queryPtr,queryLen,dox,spool);
				else if (protocol==proFile || protocol==proCompFile)
					err = FileURL(query);
				else if (protocol==proSetting)
					err = SettingURL(query);
				else if (protocol==proXEudoraJump)
					err = JumpURL(query);
			}
			if (err && err!=userCanceledErr) WarnUser(URL_PARSE,err);
		}
	}
	gHelpWinClick = false;
	return(err);
}

/**********************************************************************
 * OpenLocalURLLo - open a URL in Eudora
 **********************************************************************/
OSErr OpenLocalURLLo(PStr url,Handle *result,AEDescList *dox,Boolean spool)
{
	return OpenLocalURLPtr(url+1,*url,result,dox,spool);
}

/**********************************************************************
 * MailtoURL - handle a mailto url
 **********************************************************************/
OSErr MailtoURL(PStr query,AEDescList *dox,Boolean spool)
{
	Str255 scratch;
	PCopy(scratch,query);	//	Make copy. It may get changed.
	return MailtoURLPtr(scratch+1,*scratch,dox,spool);	
}

/**********************************************************************
 * MailtoURL - handle a mailto url
 **********************************************************************/
OSErr MailtoURLPtr(Ptr srcQuery,long queryLen,AEDescList *dox,Boolean spool)
{
	OSErr err=noErr;
	Ptr head,hVal,hdrName;
	long	headLen,hValLen,hdrNameLen;
	Str255 hName;
	Str255 val;
	UPtr spot;
	char qMark[3];
	char eSign[2];
	UPtr equal;
	short hid;
	HeadSpec hs;
	MyWindowPtr	win;
	WindowPtr winWP;
	PersHandle pers = nil;
	Handle	valHdl;
	Ptr	query;
	
	// Make a copy of url. We may modify it while decoding.
	if (!(query = NuPtr(queryLen))) return MemError();
	BMD(srcQuery,query,queryLen);
	
	winWP = FrontWindow_();
	win = GetWindowMyWindowPtr (winWP);
	if (MainEvent.what != kHighLevelEvent && (GetWindowKind(winWP)==MESS_WIN || GetWindowKind(winWP)==COMP_WIN))
		pers = MESS_TO_PERS(Win2MessH(win));
	DoMenu(winWP,(MESSAGE_MENU<<16)|MESSAGE_NEW_ITEM,0);
	if (winWP==FrontWindow_()) err = errAEEventNotHandled;
	else
	{
		win = GetWindowMyWindowPtr (FrontWindow_());
		if(gHelpWinClick)
		{
			gHelpWinClick = false;
			SetMessOpt(Win2MessH(win),OPT_SEND_REGINFO);
		}
		if (pers) SetPers((*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,pers,true);
		qMark[0] = '?';
		qMark[1] = '&';
		eSign[1] = qMark[2] = 0;
		eSign[0] = '=';
		spot = query;
		if (TokenPtr(query,queryLen,&hVal,&hValLen,&spot,qMark))
		{
			// grab the to address
			if (hValLen)
			{
				FixURLPtr(hVal,&hValLen);
				if (CompHeadFind(Win2MessH(win),TO_HEAD,&hs))
				{
					valHdl = URLVarSub(&hVal,&hValLen);
					err = CompHeadSetPtr((*Win2MessH(win))->bodyPTE,&hs,hVal,hValLen);
					ZapHandle(valHdl);
					if (!err)
						err = NickExpandAndCacheHead (Win2MessH(win), TO_HEAD, true);
				}
			}
			// now, the rest of the headers
			while (!err && TokenPtr(query,queryLen,&head,&headLen,&spot,qMark))
			{
				equal = head;
				if (TokenPtr(head,headLen,&hdrName,&hdrNameLen,&equal,eSign))
				{
					if (TokenPtr(head,headLen,&hVal,&hValLen,&equal,eSign))
					{
						MakePPtr(hName,hdrName,hdrNameLen);
						FixURLString(hName);
						FixURLPtr(hVal,&hValLen);
						TurnBareLFs2BareCRs(hVal,hValLen);
						hValLen = StripChar(hVal,hValLen,'\012');
						if (EqualStrRes(hName,URL_BODY))
							CompHeadFind(Win2MessH(win),0,&hs);
						else
						{
							PCatC(hName,':');
							hid = FindSTRNIndex(HEADER_STRN,hName);
							if (hid && hid<=BCC_HEAD)
								CompHeadFind(Win2MessH(win),hid,&hs);
							else
							{
								MakePPtr(val,hVal,hValLen);
								switch (hid)
								{
									case PERSONA_HEAD:
										{
											PersHandle pers = FindPersByName(val);
											if (pers) SetPers((*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,pers,true);
										}
										break;
									case SIG_HEAD:
										{
											if (!*val)
												SetSig((*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,SIG_NONE);
											else
											{
												MyLowerStr(val);
												SetSig((*Win2MessH(win))->tocH,(*Win2MessH(win))->sumNum,Hash(val));
											}
										}
										break;
								}
								continue;
							}
						}
						//	Do variable substitution.
						valHdl = URLVarSub(&hVal,&hValLen);
						err = CompHeadSetPtr((*Win2MessH(win))->bodyPTE,&hs,hVal,hValLen);
						ZapHandle(valHdl);
					}
				}
				else break;
			}
		}
		
		// now, attach dox
		if (!err && dox && dox->descriptorType!='null')
		{
			long i, n;
			FSSpec spec;
			DescType junkKW, junkType;
			long junkSize;
			
			if (err = AECountItems(dox,&n)) goto done;
			for (i=1;i<=n;i++)
			{
				// grab the spec
				if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
					break;
					
				// better be there
				if (err=FSpExists(&spec)) {if (err==fnfErr) err=errAEEventNotHandled; break;}
				
				// we don't do folders, sorry
				if (FSpIsItAFolder(&spec)) {err=errAEEventNotHandled;break;}
				
				// attach it
				CompAttachSpec(win,&spec);
				
				// spool it if need be
				if (spool)
					if (err=SpoolIndAttachment(Win2MessH(win),i)) break;
			}
		}	
		
		if (win) CompActivateAppropriate(Win2MessH(win));	
	}
 done:
 	ZapPtr(query);	// dispose of URL copy
	return(err);
}

/**********************************************************************
 * FixURLPtr - undo escaped stuff in URL's
 **********************************************************************/
void FixURLPtr(Ptr url,long *len)
{
	UPtr spot, end;
	UPtr romanized;
	long romanLen;
	
	spot = url;
	end = url+*len;
	
	for (;spot<end;spot++) if (*spot=='%') *spot = lowerDelta;
	
	CaptureHexPtr(url,url,len);
	
	// convert utf8 if we can
	if (romanized = NuPtr(*len))
	{
		romanLen = *len;
		BMD(url,romanized,*len);
		if (UTF8ToRoman(romanized,&romanLen,romanLen))
		{
			BMD(romanized,url,romanLen);
			*len = romanLen;
		}
		ZapPtr(romanized);
	}
}

/**********************************************************************
 * TurnBareLFs2BareCRs - turn standalone LF's into standalone CR's
 **********************************************************************/
void TurnBareLFs2BareCRs(Ptr s,long len)
{
	UPtr spot;
	
	for (spot=s;spot<s+len;spot++)
		if (*spot=='\012' && (spot==s||spot[-1]!='\015'))
			*spot = '\015';
	return;
}

/**********************************************************************
 * SettingURL - handle a settings url
 **********************************************************************/
OSErr SettingURL(PStr query)
{
	Str31 numStr;
	Str255 val, s;
	Str31 pers;
	UPtr spot, persSpot;
	long num;
	DialogPtr dgPtr;
	MyWindowPtr	dgPtrWin;
	short item;
	extern ModalFilterUPP DlgFilterUPP;
	short forbidden[] = {PREF_PASS_TEXT, PREF_AUXPW, PREF_ACAP_PASS};
	
	*pers = 0;
	
	spot = query+1;
	if (PToken(query,numStr,&spot,"="))
	{
		if (PIndex(numStr,'@'))
		{
			PCopy(s,numStr);
			persSpot = s+1;
			PToken(s,numStr,&persSpot,"@");
			PToken(s,pers,&persSpot,"\377");
			if (!FindPersByName(pers))
			{
				Aprintf(OK_ALRT,Note,NO_SUCH_PERSONALITY,pers);
				return userCanceledErr;
			}
		}
		StringToNum(numStr,&num);
		
		for (item=0;item<sizeof(forbidden)/sizeof(forbidden[0]);item++)
			if (num==forbidden[item])
			{
				WarnUser(FORBIDDEN_SETTING,0);
				return userCanceledErr;
			}

		if (!MommyMommy(ATTENTION,nil)) return(userCanceledErr);
		if ((dgPtrWin = GetNewMyDialog(URL_SETTING_CONFIRM_DLOG,nil,nil,InFront))==nil)
		{
			WarnUser(GENERAL,MemError());
			return(userCanceledErr);
		}
		dgPtr = GetMyWindowDialogPtr (dgPtrWin);
		
		// if no =, use current value
		if (*pers) PushPers(FindPersByName(pers));
		NoProxify = true;
		if (spot[-1]=='=') PToken(query,val,&spot,"\377");
		else GetPref(val,num);
	
		if (*pers) ComposeRString(s,FOR_PERSONALITY,pers);
		else *s = 0;
		
		ParamText(numStr,s,"","");
		SetDIText(dgPtr,uscNewValueItem,val);
		SetDIText(dgPtr,uscCurValueItem,GetPref(s,num));
		SetDIText(dgPtr,uscHelpItem,*PrefHelpString(num,s,true)?s:GetRString(s,NO_SETTING_HELP));
		SetDIText(dgPtr,uscDefValueItem,GetDefPref(s,num));
		SelectDialogItemText(dgPtr,uscNewValueItem,0,REAL_BIG);
		NoProxify = false;
		AutoSizeDialog(dgPtr);
		
		{
			// AutoSizeDialog doesn't seem to move the text edit field under OS X.
			Rect teR, cbR;
			
			GetControlBounds(GetDItemCtl(dgPtr,uscCancelItem),&cbR);
			GetControlBounds(GetDItemCtl(dgPtr,uscNewValueItem),&teR);
			
			MoveMyCntl(GetDItemCtl(dgPtr,uscNewValueItem),teR.left,cbR.top-(teR.bottom-teR.top)-10,0,0);
		}

		ShowWindow(GetDialogWindow(dgPtr));
		SetMyCursor(arrowCursor);

		StartMovableModal(dgPtr);
		do
		{
			MovableModalDialog(dgPtr,DlgFilterUPP,&item);
		}
		while (item>3);
		EndMovableModal(dgPtr);
		
		if (item==uscRevertItem) GetDIText(dgPtr,uscDefValueItem,val);
		else GetDIText(dgPtr,uscNewValueItem,val);
		
		DisposDialog_(dgPtr);

		if (item==uscCancelItem || item==CANCEL_ITEM) {if (*pers) PopPers();return(userCanceledErr);}
		
		SetPref(num,val);
		if (*pers) PopPers();
		return(noErr);
	}
	return(fnfErr);
}

/************************************************************************
 * SubVar - substitute variables
 ************************************************************************/
void SubVar(Handle h,PStr find,PStr replace)
{
	long	offset = 0;
	for (offset=0;(offset = Munger(h,offset,find+1,*find,replace+1,*replace))>0;offset += *replace);
}

/************************************************************************
 * URLVarSub - substitute variables
 ************************************************************************/
Handle URLVarSub(Ptr *string, long *len)
{
	Str31 var;
	Str255 subMe;
	Handle	h = nil;

	if (h = NuDHTempBetter(*string,*len))
	{
		SubVar(h,GetRString(var,NAME_VAR),GetPref(subMe,PREF_REALNAME));
		SubVar(h,GetRString(var,ADDR_VAR),GetReturnAddr(subMe,true));
		GetPOPInfo(subMe,nil);
		SubVar(h,GetRString(var,USER_VAR),subMe);
		*string = LDRef(h);
		*len = GetHandleSize(h);
	}
	return(h);
}

//
//	OpenOtherURLPtr
//
//		Pointer version of OpenOtherURL so we can handle URL's in excess of 255 characters.
//		Eventually, this should either replace (or be called by) OpenOtherURL, but for now,
//		Let's be safe.
//

OSErr OpenOtherURLPtr(PStr proto,UPtr url,short length)

{
	short err = noErr;
	LaunchParamBlockRec lpb;
	FSSpec spec;
	ProcessSerialNumber psn, myPSN;
	AppleEvent ae;
	AEDesc launchDesc;
	AliasHandle appAlias = nil;
	Boolean isRunning;
	Boolean wasChanged;
	Boolean same;
	Boolean dontFront = (MainEvent.modifiers&cmdKey) && PrefIsSet(PREF_CMD_DONT_FRONT);
	
	if (HaveTheDiseaseCalledOSX() && !PrefIsSet(PREF_USE_OWN_URL_HELPERS))
	{
		OSErr err = fnfErr;
		CFURLRef theRef = CFURLCreateWithBytes(nil,url,length,kCFStringEncodingUTF8,nil);
		if (theRef)
		{
			err = LSOpenCFURLRef(theRef,nil);
			CFRelease(theRef);
			if (!err) return noErr;
		}
	}		
		
	NullADList(&ae,&launchDesc,nil);
	GetCurrentProcess(&myPSN);

	/*
	 * figure out what should open it
	 */
	if (MainEvent.modifiers & optionKey) err = SFURLApp(proto,&appAlias);
	else (err=FindURLApp(proto,&appAlias,kWildCardOK));
		
	if (err) return(err);
	
	/*
	 * is that thing running?
	 */
	isRunning = !FindPSNByAlias(&psn,appAlias);
	
	/*
	 *	Do we need to be online for this URL, and are we?
	 */
	if ((*(long*)(proto+1)=='http') && PreventOfflineLink(&err, false)) return (err);
	
	/*
	 * build the GURL
	 */
	MyLowerStr(proto);
	if (err=BuildGURLPtr(&psn,url,length,&ae,*(long*)(proto+1)=='http'&&(MainEvent.modifiers&cmdKey)&&PrefIsSet(PREF_CMD_DONT_FRONT))) return(err);
	
	/*
	 * if running, cake
	 */
	if (isRunning)
	{
		// don't send ourselves a GURL event.
		// This may seem a bit drastic (sending yourself events is supposed to be
		// great and wonderful, I know).  However, the current code will chain
		// infinitely if send a GURL for a protocol it can't handle if the helper
		// app is set to Eudora; safest to avoid this for now
		if (SameProcess(&psn,&myPSN,&same) || same) return(errAEEventNotHandled);
		//if (RunType==Debugging) DebugStr("\pAlready running!");
		err = MyAESend(&ae,nil,kAEQueueReply|kAECanInteract|kAECanSwitchLayer,
								 kAENormalPriority,kAEDefaultTimeout);
		if (!err && !dontFront) SetFrontProcess(&psn);
	}

	else
	{
		/* not running; gotta launch */	
		/*
		 * coerce to proper descriptor, like DTS says
		 */
		err = AECoerceDesc(&ae,typeAppParameters,&launchDesc);
		if (err) WarnUser(AE_TROUBLE,err);
		
		/*
		 * let's launch it
		 */
		if (!err)
		{
			Handle	hAppParms;
			Zero(lpb);
			AEGetDescDataHandle(&launchDesc,&hAppParms);
			lpb.launchAppParameters = (AppParametersPtr)LDRef(hAppParms);
			lpb.launchBlockID = extendedBlock;
			lpb.launchEPBLength = extendedBlockLen;
			lpb.launchControlFlags = launchContinue | launchNoFileFlags;
			if (dontFront) lpb.launchControlFlags |= launchDontSwitch;
			err = ResolveAlias(nil,appAlias,&spec,&wasChanged);
			//if (RunType==Debugging) DebugStr(spec.name);
			if (!err)
			{
				lpb.launchAppSpec = &spec;
				WriteZero(&lpb.launchProcessSN,sizeof(lpb.launchProcessSN));
				if (err = LaunchApplication(&lpb)) WarnUser(COULDNT_LAUNCH,err);
			}
			AEDisposeDescDataHandle(hAppParms);
		}
	}	
	DisposeADList(&ae,&launchDesc,nil);
	
	return(err);
}
/**********************************************************************
 * FindURLApp - find a map for a URL
 **********************************************************************/
OSErr FindURLApp(PStr proto,AliasHandle *alias,Boolean mayWildCard)
{
	AliasHandle mapAlias=nil;
	
	*alias = nil;
	
	/*
	 * first, let's see if we know about this type in particular
	 */
	if (mapAlias = (void*)GetNamedResource(URLMAP_TYPE,proto))
		HNoPurge((Handle)mapAlias);
	
	/*
	 * how about a wildcard?
	 */
	if (!mapAlias && mayWildCard)
	{
		if (mapAlias = (void*)GetNamedResource(URLMAP_TYPE,"\p****"))
			HNoPurge((Handle)mapAlias);
	}
	
	/*
	 * do we have any idea?
	 */
	if (mapAlias)
	{
		// search by creator?
		if ((!PrefIsSet(PREF_NO_FUZZY_HELPERS) || SimpleResolveAliasNoUI(mapAlias,nil)) && (*mapAlias)->userType && !TypeIsOnList((*mapAlias)->userType,APPLET_LIST_TYPE))
			CreatorToApp((*mapAlias)->userType,alias);
		// search by alias only
		else if (LocateApp(mapAlias))
			{ReleaseResource_(mapAlias);mapAlias=nil;}
	}
	
	/*
	 * have we filled it in yet?
	 */
	if (!*alias)
	{
		// do we know what it should be?  If not, ask the user
		if (!mapAlias) SFURLApp(proto,&mapAlias);
		
		ASSERT(CurResFile()==SettingsRefN);
		// push has come to shove.  Either we have it now or we give up
		if (mapAlias)
		{
			*alias = mapAlias;
			if (MyHandToHand((Handle*)alias)) *alias = nil;
			PurgeIfClean((Handle)mapAlias);
			return(noErr);
		}
	}
	return(*alias?noErr : fnfErr);
}

/************************************************************************
 * BuildGURL - build a GURL event
 ************************************************************************/
OSErr BuildGURL(ProcessSerialNumberPtr psn,PStr url,AppleEvent *ae,Boolean newWindow)
{
	OSErr err;
	AEAddressDesc target;
	
	if (!(err=AECreateDesc(typeProcessSerialNumber,psn,sizeof(*psn),&target)))
	{
		if (!(err=AECreateAppleEvent(newWindow?kW3Class:kURLClass,newWindow?kOpenURL:kURLGet,&target,
																 kAutoGenerateReturnID,kAnyTransactionID,ae)))
		{
			err=AEPutParamPtr(ae,keyDirectObject,typeChar,url+1,*url);
			// add newWindow parameter as optional param
			if (newWindow && !err)
			{
				AEDesc optD;
				OSType keyword = keyW3Window;
	
				NullADList(&optD,nil);
				if (!(err = AECreateList(nil,0,False,&optD)))
				if (!(err = AEPutPtr(&optD,1,typeKeyword,&keyword,sizeof(keyword))))
				if (!(err = AEPutAttributeDesc(ae,keyOptionalKeywordAttr,&optD)))
					err = AEPutLong(ae,keyW3Window,0);
				DisposeADList(&optD,nil);
			}
		}
		AEDisposeDesc(&target);
	}
	
	if (err) WarnUser(AE_TROUBLE,err);
	
	return(err);
}

//
//	BuildGURLPtr
//
//		Pointer version of BuildGURL so we can handle URL's in excess of 255 characters.
//		Eventually, this should either replace (or be called by) BuildGURL, but for now,
//		Let's be safe.
//

OSErr BuildGURLPtr(ProcessSerialNumberPtr psn,UPtr url,short length,AppleEvent *ae,Boolean newWindow)
{
	OSErr err;
	AEAddressDesc target;
	
	if (!(err=AECreateDesc(typeProcessSerialNumber,psn,sizeof(*psn),&target)))
	{
		if (!(err=AECreateAppleEvent(newWindow?kW3Class:kURLClass,newWindow?kOpenURL:kURLGet,&target,
																 kAutoGenerateReturnID,kAnyTransactionID,ae)))
		{
			err=AEPutParamPtr(ae,keyDirectObject,typeChar,url,length);
			// add newWindow parameter as optional param
			if (newWindow && !err)
			{
				AEDesc optD;
				OSType keyword = keyW3Window;
	
				NullADList(&optD,nil);
				if (!(err = AECreateList(nil,0,False,&optD)))
				if (!(err = AEPutPtr(&optD,1,typeKeyword,&keyword,sizeof(keyword))))
				if (!(err = AEPutAttributeDesc(ae,keyOptionalKeywordAttr,&optD)))
					err = AEPutLong(ae,keyW3Window,0);
				DisposeADList(&optD,nil);
			}
		}
		AEDisposeDesc(&target);
	}
	
	if (err) WarnUser(AE_TROUBLE,err);
	
	return(err);
}


/************************************************************************
 * SFURLApp - ask the user what app to use for a URL
 ************************************************************************/
OSErr SFURLApp (PStr proto, AliasHandle *alias)
{
	URLHookOptions	options;
	SFTypeList			types;
	FSSpec					spec;
	Str255					prompt;
	OSErr						theError;
	Boolean					good;

	types[0] = 'APPL';
	types[1] = 'adrp';
	types[2] = 'appe';
	types[3] = 0;

	options.permanently	= True;
	options.protocol		= FindSTRNIndex (ProtocolStrn, proto);

	ComposeRString (prompt, CHOOSE_URL_APP, proto);

		theError = SFURLAppNav (proto, alias, types, prompt, &spec, &options, &good);
	if (!theError && good) {
		theError = MakeHelperAlias (&spec, alias);
		if (!theError && options.permanently)
			SaveURLPref (proto, *alias);
	}
	else
		if (!theError || theError == userCanceledErr)
			theError = fnfErr;
	return (theError);
}

/************************************************************************
 * MakeHelperAlias - return an alias to a helper app
 ************************************************************************/
OSErr MakeHelperAlias(FSSpecPtr app,AliasHandle *alias)
{
	OSType creator;
	OSErr err;
	
	*alias = nil;
	err = NewAlias(nil,app,alias);
	if (*alias && !err)
	{
		creator = FileCreatorOf(app);
		(**alias)->userType = creator;
	}
	return(err);
}

/**********************************************************************
 * SaveURLPref - save the application we want to use to open url's
 **********************************************************************/
OSErr SaveURLPref(PStr proto,AliasHandle alias)
{
	Handle oldH;
	
	while (oldH = GetNamedResource(URLMAP_TYPE,proto))
	{
		if (HomeResFile(oldH)!=SettingsRefN) break;
		RemoveResource(oldH); if (ResError()) break;
		UpdateResFile(SettingsRefN); if (ResError()) break;
		DisposeHandle(oldH);
	}
		
	if (alias)
		AddMyResource_(alias,URLMAP_TYPE,MyUniqueID(URLMAP_TYPE),proto);
	return(ResError());
}

/**********************************************************************
 * URLIsSelected - is the current selection in a URL?
 **********************************************************************/
URLEnum URLIsSelected(MyWindowPtr win,PETEHandle pte,long startWith,long endWith,short what,long *uStart,long *uEnd)
{
	Handle textH=nil;
	UPtr text;
	UPtr start,stop,end;
	Str255 proto,host,scratch,linkText;
	Boolean point;
	long selStart, selEnd;
	Byte state;
	Byte startChar=' ';
	UPtr spot;
	UPtr colon;
	Boolean chiral=False;
	PETEDocInfo info;
	Boolean rude = False;
	PETEStyleEntry pse;
	URLEnum result = urlNot;
	long size;
	Boolean colorThem = 0!=(what&urlColor);
	Boolean selectThem = 0!=(what&urlSelect);
	Boolean openThem = 0!=(what&urlOpen);
	Handle	hURL = nil;
	OSErr	err;
	Size	urlSize,queryLen;
	Ptr		urlPtr,queryPtr;
	short	i;
	Boolean bowelSection = false;
	uLong linkStart,linkStop;
	
	if (!pte) return(urlNot);
	
	if (PeteGetTextAndSelection(pte,&textH,&selStart,&selEnd)) return(urlNot);
	size = GetHandleSize(textH);
	if (!size) return(urlNot);
	
	state = HGetState(textH);

	text = LDRef(textH);
	start = text+((startWith<0)?selStart:startWith);
	stop = text+((endWith<0)?selEnd:endWith);
	size = MIN(start-text+253,size);
	end = text+size;
	point = ClickType==Double || start==stop;
	if (!point) end=stop;
	*linkText = 0;
	
	PETEGetDocInfo(PETE,pte,&info);
	PeteStyleAt(pte,startWith,&pse);
	if (openThem || pse.psStyle.textStyle.tsLabel&pLinkLabel)
	{
		if (pse.psStyle.textStyle.tsLabel&(pURLLabel|pLinkLabel))
		{
			if (point || pse.psStyle.textStyle.tsLabel&pLinkLabel)
			{
				if (pse.psStyle.textStyle.tsLabel&pLinkLabel)
				{
					PETEFindLabelRun(PETE,pte,(info.styleRunStart+info.styleRunStop)/2,
						&info.styleRunStart,&info.styleRunStop,pLinkLabel,pLinkLabel);
					PETEFindLabelRun(PETE,pte,info.styleRunStop-1,&linkStart,&linkStop,pLinkLabel,pLinkLabel|pURLLabel);
					MakePStr(linkText,text+linkStart+1,linkStop-linkStart-1);
				}
				PETEFindLabelRun(PETE,pte,info.styleRunStart+2,
					&info.styleRunStart,&info.styleRunStop,pURLLabel,pURLLabel);
				start = text + info.styleRunStart;
				stop = text + info.styleRunStop;
			}
		}
		else if (point) goto fail;
	}
	
	if (!openThem && pse.psStyle.textStyle.tsLabel&pLinkLabel)
	{
		if (uStart) *uStart = info.styleRunStart;
		if (uEnd) *uEnd = info.styleRunStop;
		HSetState(textH,state);
		return(urlGood);
	}
		
	if (point)
	{
		if (!openThem)
		{
			/*
			 * scan backward for colon
			 */
			for (colon=MIN(start,end-1);colon>text;colon--) if (*colon==':') break;
			
			/*
			 * nope.  scan forward
			 */
			if (*colon!=':') for (colon=start;colon<end;colon++) if (*colon==':') break;
			
			/*
			 * if no colon, done
			 */
			if (colon<text || colon>=end || *colon!=':') goto fail;
			
			/*
			 * Found a colon.  Back up to next non-scheme character
			 */
			start = colon;
			bowelSection = colon < end-2 && colon[1]=='/' && colon[2]=='/';
			while (start>=text && IsSchemeChar(*start)) start--;
			
			/*
			 * note the character that began the "URL"
			 */
			startChar = (start<text || IsAnySP(*start) || *start=='>') ? ' ' : *start;
			
			/*
			 * if the start char is chiral, mirror-image
			 */
			GetRString(scratch,URL_LEFT);
			if (spot=PIndex(scratch,startChar))
			{
				chiral = True;
				GetRString(scratch,URL_RIGHT);
				startChar = *spot;
			}
			
			/*
			 * the first char of the URL is actually *after* the start char
			 */
			start++;
		
			/*
			 * now, scan for ending character
			 */
			if (startChar==' ' || !chiral)
			{
				while (stop<end && !IsAnySP(*stop) && *stop!=startChar && IsURLChar(*stop)) stop++;
				if (startChar==' ')
				{
					if (stop<end && !(IsAnySP(*stop))) goto fail;
				}
				else if (stop==end || *stop!=startChar) goto fail;
			}
			else
			{
				while (stop<end && *stop!=startChar && IsURLChar(*stop)) stop++;
				if (stop==end || *stop!=startChar) goto fail;
			}
			
			/*
			 * vacuuity
			 */
			if (start==colon || end-colon==1) goto fail;
		}
	}	
	
	rude = (start>text) && (start[-1]!='<') && point;
	if (rude)
		while(stop>start && PIndex(GetRString(scratch,URL_TRAIL_IGNORE),stop[-1])) stop--;
		
	/*
	 * make a URL handle out of it
	 */
	if (start<stop && start>=text && stop<=end)
	{ 
		if (err = PtrToHand(start,&hURL,stop-start))
		{
			WarnUser(GENERAL,err);
			goto fail;
		}
	}
	else
		return(urlNot);

	selStart = start-*textH;
	selEnd = stop-*textH;
	HSetState(textH,state);

	/*
	 * GRRRRRRRRR! Strip out some characters such as returns
	 */
	GetRString(scratch,URL_STRIP_CHARS);
	for (i=1;i<=*scratch;i++)
		RemoveCharHandle(scratch[i],hURL);
	
	/*
	 * remove URL:
	 */
	GetRString(scratch,URL);
	PCatC(scratch,':');
	if (*scratch && StartsWithPtr(*hURL,GetHandleSize(hURL),scratch))
		Munger(hURL,0,nil,*scratch,scratch,0);
	
	/*
	 * is it really a url?
	 */
	urlPtr = LDRef(hURL);
	urlSize = GetHandleSize(hURL);
	if (GetHandleSize(hURL))
	{
		/* allow no newlines */
		if (!ParseURLPtr(urlPtr,urlSize,proto,host,&queryPtr,&queryLen))
		{
#ifdef URL_PROTECTION
			if (openThem && URLIsNaughty(linkText,hURL,true))
			{
				ZapHandle(hURL);
				ZapPtr(queryPtr);
				return urlNaughty;
			}
#endif //URL_PROTECTION
			if (selectThem)
			{
				PeteSelect(nil,pte,selStart,selEnd);
				PeteSetDirty(pte);
			}
			if (colorThem && (queryLen||*host||startChar=='>'||bowelSection) && (openThem || startChar=='>' || FindSTRNIndex(UrlColorStrn,proto) || bowelSection))
			{
				result = rude ? urlRude : urlGood;
				URLStyle(pte,selStart,selEnd,rude);
			}
			else result = urlMaybe;
			
			if (openThem)
			{
				OSErr urlOpenErr = noErr;
				
				gHelpWinClick = (win && GetWindowKind(GetMyWindowWindowPtr(win)) == HELP_WIN);
				//if (rude) PlaySoundId(1003);
				if (((MainEvent.modifiers&optionKey)&&!EqualStrRes(proto,ProtocolStrn+proFile)) || (fnfErr==(urlOpenErr=OpenLocalURLPtr(urlPtr,urlSize,nil,nil,false))))
					urlOpenErr = OpenOtherURLPtr(proto,urlPtr,urlSize);
				
				// Add the URL to the history list, if we ought to.
				if (!PrefIsSet(PREF_NO_LINK_HISTORY))
				{
					MakePStr(scratch,urlPtr,urlSize);
					AddURLToMainHistory(scratch, nil, urlOpenErr);
				}
			}
			
			if (uStart) *uStart = selStart;
			if (uEnd) *uEnd = selEnd;
			
			ZapHandle(hURL);
			return(result);
		}
	}
		
fail:
	ZapHandle(hURL);
	if (textH) UL(textH);
	return(urlNot);
}

/**********************************************************************
 * SlackURL - is the current selection something we can try to open as URL?
 **********************************************************************/
URLEnum SlackURL(MyWindowPtr win,PETEHandle pte,long startWith,long endWith,short what,long *uStart,long *uEnd)
{
	Handle textH=nil;
	UPtr text;
	UPtr start,stop,end;
	Boolean point;
	long selStart, selEnd;
	Byte state;
	Byte startChar=' ';
	UPtr spot;
	Boolean chiral=False;
	long size;
	Boolean colorThem = 0!=(what&urlColor);
	Boolean selectThem = 0!=(what&urlSelect);
	Boolean openThem = 0!=(what&urlOpen);
	Str255 url;
	FSSpec spec;
	
	if (!PeteIsValid(pte)) return(urlNot);
	
	if (PeteGetTextAndSelection(pte,&textH,&selStart,&selEnd)) return(urlNot);
	size = GetHandleSize(textH);
	if (!size) return(urlNot);
	
	state = HGetState(textH);

	text = LDRef(textH);
	start = text+((startWith<0)?selStart:startWith);
	stop = text+((endWith<0)?selEnd:endWith);
	size = MIN(start-text+253,size);
	end = text+size;
	point = start==stop;
	if (!point) end=stop;
			
	if (point)
	{
		/*
		 * expand to delimitters
		 */
		
		// back up start
		while (start > text && IsSlackURLChar(start[-1])) start--;
		
		// figure out what the start char was, so we know what the end char will be
		startChar = (start==text || IsAnySP(start[-1]) || start[-1]=='>') ? ' ' : start[-1];
		
		// if the start char is chiral, mirror-image
		GetRString(url,URL_LEFT);
		if (spot=PIndex(url,startChar))
		{
			chiral = True;
			GetRString(url,URL_RIGHT);
			startChar = *spot;
		}
		
		// move stop forward until we hit the delimitter
		for (;stop < end;stop++) if (*stop==startChar || startChar==' ' && IsAnySP(*stop)) break;
	}	
	
	// trim bogus trailers
	if (!chiral)
	{
		GetRString(url,URL_TRAIL_IGNORE);
		while(stop>start && PIndex(url,stop[-1])) stop--;
	}
		
	/*
	 * make a string out of it
	 */
	*url = 0;
	if (start<stop && start>=text && stop<=end) MakePStr(url,start,stop-start);
	selStart = start-*textH;
	selEnd = stop-*textH;
	HSetState(textH,state);
	
	/*
	 * did we get something?
	 */
	if (*url)
	{
		// is it an email address?
		if (PIndex(url,'@'))
		{
			if (openThem) MailtoURL(url,nil,false);
		}
		// is it a mailbox name in the filter report?
		else if (GetWindowKind(GetMyWindowWindowPtr(win))==TEXT_WIN && win->ro && !*(*(TextDHandle)GetMyWindowPrivateData(win))->spec.name && !BoxSpecByName(&spec,url))
		{
			if (openThem) OpenMailbox(&spec,true,nil);
		}
		else if (IsHostname(url))
		{
			HostnameURL(url);
		}
		else
			*url = 0;
		
		// if we found a URL, select it and return maybe
		if (*url)
		{
			if (selectThem && *url)
			{
				PeteSelect(nil,pte,selStart,selEnd);
				PeteSetDirty(pte);
			}
			return(urlMaybe);
		}
	}
	
fail:
	if (textH) UL(textH);
	return(urlNot);
}

/**********************************************************************
 * IsHostname - is something a host name?
 **********************************************************************/
Boolean IsHostname(PStr string)
{
	UPtr spot, end;
	short dots = 0;
	
	// trim off trailing punctuation (but leave :port syntax alone)
	while (!(IsWordChar[string[*string]] || isdigit(string[*string]) || ':'==string[*string])) --*string;
	end = string+*string+1;
	
	// make sure all chars are ok, and count dots
	for (spot=string+1;spot<end;spot++)
	{
		if (!IsHostChar(*spot)) return(false);
		if (*spot=='.') dots++;
	}
	
	// we need at least one dot
	return(dots>0);
}

/**********************************************************************
 * HostnameURL - launch a URL that is a hostname
 **********************************************************************/
OSErr HostnameURL(PStr hostname)
{
	Str255 url;
	UPtr spot;
	Str31 proto;
	OSErr err;
	
	// first component might be a URL protocol
	spot = hostname+1;
	PToken(hostname,url,&spot,".");
	if (!FindSTRNIndex(UrlColorStrn,url))
		GetRString(url,HTTP);	// if not, assume web
	PSCopy(proto,url);
	PCatC(url,':');
	PCatC(url,'/');
	PCatC(url,'/');
	PSCat(url,hostname);
	err = OpenLocalURL(url,nil);
	if (err==fnfErr) err = OpenOtherURLPtr(proto,url+1,*url);
	return(err);
}

/**********************************************************************
 * URLStyle - set some text in the URL style
 **********************************************************************/
void URLStyle(PETEHandle pte, long selStart,long selEnd,Boolean rude)
{
	short peteDirtyWas;
	Boolean winDirtyWas;
		
	peteDirtyWas = PeteIsDirty(pte);
	winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	PeteLabel(pte,selStart,selEnd,pURLLabel,pSpellLabel|pURLLabel|pTightAnalLabel|pLooseAnalLabel);
	PETEMarkDocDirty(PETE,pte,peteDirtyWas);
	(*PeteExtra(pte))->win->isDirty = winDirtyWas;
}

/**********************************************************************
 * PeteURLScan - scan for & hilite URL's in text
 **********************************************************************/
void PeteURLScan(MyWindowPtr win,PETEHandle pte)
{
	long colon=-1;
	Handle text;
	long plainStart = (*PeteExtra(pte))->urlScanned;
	long plainEnd = plainStart;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	PETEStyleEntry pse;
	long lEnd, sLen;

	Zero(pse);
	
	if (plainStart >= PETEGetTextLen(PETE,pte))
	{
		(*PeteExtra(pte))->urlScanned = -1;	// don't scan again
		return;
	}
	if ((*PeteExtra(pte))->urlScanned==-1) return;
		
	// First, see if we have a graphic, which we'd better
	// not mark as a URL
	PETEGetStyle(PETE,pte,plainStart,&sLen,&pse);
	if (pse.psGraphic)
	{
		(*PeteExtra(pte))->urlScanned = sLen + pse.psStartChar;
		return;
	}
	
	NoScannerResets++;	// these changes don't really count
	
	// If we're in the middle of a URL, back up to the
	// beginning of the URL
	if (pse.psStyle.textStyle.tsLabel&pURLLabel)
	{
		PETEFindLabelRun(PETE,pte,plainStart,&plainStart,&lEnd,pURLLabel,pURLLabel);
	}
	
	// Now, find the colon that might be the scheme character for a URL
	PETEGetRawText(PETE,pte,&text);
	if ((colon = SearchPtrHandle(":",1,text,plainStart,True,False,nil))>0)  //if (!(err=PETEFindText(PETE,pte,":",1,(*PeteExtra(pte))->urlScanned,0x7fffffff,&colon,smSystemScript)))
	{
		// If we're at the end of the text, or if
		// we have a double-colon, this can't be a URL
		if (colon>=GetHandleSize(text)-1 || (*text)[colon+1]==':')
		{
			plainEnd = colon+2;
			(*PeteExtra(pte))->urlScanned = colon+2;
		}
		// If we've landed in the middle of a graphic, don't 
		// recognize a URL, but do "clean" everything before us.
		else if (PETEGetStyle(PETE,pte,colon,&sLen,&pse), pse.psGraphic)
		{
			plainEnd = pse.psStartChar;
			(*PeteExtra(pte))->urlScanned = sLen + pse.psStartChar;
		}
		// If the thing we found a URL in is in a link, set the
		// scanner past it, but clear url to before it
		else if (pse.psStyle.textStyle.tsLabel&pLinkLabel)
		{
			PETEFindLabelRun(PETE,pte,lEnd,&plainEnd,&lEnd,pLinkLabel,pLinkLabel);
			(*PeteExtra(pte))->urlScanned = lEnd;
		}
		// Ok, now find out if we have a URL here
		else if (AttIsSelected(win,pte,colon,colon,attColor,&plainEnd,&lEnd) ||
						 urlMaybe<URLIsSelected(win,pte,colon,colon,urlColor,&plainEnd,&lEnd))
		{
			plainEnd--;
			(*PeteExtra(pte))->urlScanned = lEnd+1;
			PeteNoLabel(pte,lEnd+1,lEnd+2,pURLLabel);
		}
		else
		{
			// Nope, not URL.  Set scanner past it
			plainEnd = colon+1;
			(*PeteExtra(pte))->urlScanned = colon+1;
		}
	}
	else
	{
		// found no colons; clear url style all the way to the end
		plainEnd = 0x7ffffff;
		(*PeteExtra(pte))->urlScanned = -1;	// don't scan again
	}

	// Clear label from stuff we've determined is plain
	if (plainStart<plainEnd) PeteNoLabel(pte,plainStart,plainEnd,pURLLabel);
	
	// Reset dirty bits
	if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
	(*PeteExtra(pte))->win->isDirty = winDirtyWas;

	// If there are more changes, they do count
	NoScannerResets--;
}

/**********************************************************************
 * ParseURL - parse a URL
 *  protocol://[host/]query
 **********************************************************************/
OSErr ParseURL(PStr url,PStr proto,PStr host,PStr query)
{
	OSErr	err = noErr;
	Ptr		queryPtr;
	long	queryLen;

	err = ParseURLPtr(url+1,*url,proto,host,&queryPtr,&queryLen);
	if (err!=fnfErr)
	{
		if (query)
		{
			if (queryPtr && queryLen)
				MakePPtr(query,queryPtr,queryLen);
			else
				*query = 0;
		}
	}
	return(err);
}

/**********************************************************************
 * ParseURL - parse a URL
 *  protocol://[host/]query
 **********************************************************************/
OSErr ParseURLPtr(PStr url,long length,PStr proto,PStr host,Ptr *queryPtr,long *queryLen)
{
	UPtr spot = url;
	UPtr oldSpot;
	Str255 temp;
	char slash[2];
	Boolean slashyScheme;
	short missingSlashes = 0;

	slash[0] = '/'; slash[1] = 0;
	
	/*
	 * protocol is first bit
	 */
	if (IndexPtr(url,length,':') && PTokenPtr(url,length,temp,&spot,":") && 
		  (!EqualStrRes(temp,URL) || PTokenPtr(url,length,temp,&spot,":")))
	{
		if (proto) PCopy(proto,temp);
		if (!*temp) return(fnfErr);	// no proto
		if ('0'<=temp[1] && temp[1]<='9') return(fnfErr);	// digits not allowed
		if (!IsWordChar[temp[1]]) return(fnfErr);	// must be alpha char
		slashyScheme = StrIsItemFromRes(temp,SLASHY_SCHEMES,",");
	
		if (slashyScheme || spot[0] == spot[1] && spot[1] == '/')
		{
			/*
			 * ignore run of slashes
			 */
			missingSlashes = 2;
			while (spot<url+length && *spot=='/') 
			{
				missingSlashes--;
				spot++;
				if (!slashyScheme && !missingSlashes) break;	// only worry about extra slashes
																											// for slashy schemes.  This especially 
																											// excludes file:, which can legitimately
																											// have more than two slashes
			}
			
			/*
			 * Extract next component
			 */
			if (PTokenPtr(url,length,temp,&spot,slash))
			{
				if (host) PCopy(host,temp);
				oldSpot = spot;
				if (PTokenPtr(url,length,temp,&spot,slash))
				{
					/*
					 * reparse from old spot to end of url, to pick up slashes in query
					 */
					if (queryPtr)
					{
						*queryPtr = oldSpot;
						*queryLen = url + length - oldSpot;
						if (*queryLen <= 0)
						{
							*queryPtr = nil;
							*queryLen = 0;						
						}
					}
				}
				else
				{
					/*
					 * oops, no query
					 */
					if (queryPtr)
					{
						*queryPtr = nil;
						*queryLen = 0;
					}
				}
				return(missingSlashes ? kNSLBadURLSyntax : noErr);
			}
		}
		else
		{
			if (host) *host = 0;
			if (queryPtr)
			{
				*queryPtr = spot;
				*queryLen = url + length - spot;
				if (*queryLen <= 0)
				{
					*queryPtr = nil;
					*queryLen = 0;						
				}
			}
			return(missingSlashes ? kNSLBadURLSyntax : noErr);
		}
	}
	return(fnfErr);
}

//
//	ParseProtocolFromURLPtr
//
//		Like ParseURL except we handle URL's in excess of 255 characters -- and only
//		care about the protocol.  This is a quickie function written to support long
//		URL's for adware.  It's extremely lax, but servicable in our (supposedly)
//		glasshouse environment of canned URL's.
//

OSErr ParseProtocolFromURLPtr (UPtr url, short length, PStr proto)

{

	UPtr spot = url+1;
	Str255 temp;
	
	// Don't search more than 255 characters (we do limit the protocol string)
	length = MIN(length, sizeof (temp) -1);
	for (spot = url; spot < url + length; spot++)
		if (*spot == ':') {
			temp[0] = spot - url;
			BlockMoveData (url, &temp[1], temp[0]);
			if (temp[0]) {
				if (proto)
					PCopy(proto,temp);
				if (('0' <= temp[1] && temp[1] <= '9') || !IsWordChar[temp[1]])
					return(fnfErr);	// digits not allowed
				return (noErr);
			}
		}
	return (fnfErr);
}

/**********************************************************************
 * ParsePortFromHost - grab server and port number from a host str
 *  Assumes host came from a URL parsed by ParseURL, and that URL was
 *  in the form:
 *  protocol://host[:portNo]/[query]
 *  If no port number was specified, *port is set to zero, and the
 *  function returns false.
 **********************************************************************/
Boolean ParsePortFromHost(PStr host,PStr server,long *port)
{
	Str255 serverStr;
	Str255 portStr;
	UPtr spot;
	long urlPort;


	if (server && (server != host))
		*server = 0;
	if (port)
		*port = 0;

	spot = host+1;
	if (!PToken(host,serverStr,&spot,":"))
		goto NoPort;
	if (!PToken(host,portStr,&spot,"/") || !portStr[0])
		goto NoPort;
	StringToNum(portStr,&urlPort);
	if (server)
		PCopy(server,serverStr);
	if (port)
		*port = urlPort;
	return true;


NoPort:
	if (server)
		PCopy(server,serverStr);
	return false;
}

/************************************************************************
 * FileURL - open a file URL
 ************************************************************************/
OSErr FileURL(PStr path)
{
	FSSpec spec;
	OSErr err;
	Str255 local;
	FInfo info;
	
	PCopy(local,path);
	TrLo(local+1,*local,"/",":");
	FixURLString(local);
	
	err = FSMakeFSSpec(Root.vRef,Root.dirId,local,&spec);
	
	if (!err)
	{
		err = FSpGetFInfo(&spec,&info);
		if (!err && info.fdType!=SETTINGS_TYPE && info.fdCreator==CREATOR)
			// It's a Eudora document. Just open it directly.
			err = OpenOneDoc(nil,&spec,&info);
		else
			 err = OpenOtherDoc(&spec,(MainEvent.modifiers&controlKey)!=0,false,nil);
	}
	
	if (err == fnfErr)
	{
		FileSystemError(BINHEX_OPEN,local,err);
		return(errAENoSuchObject);
	}
	
	return(err);
}

/************************************************************************
 * JumpURL - open a jump URL
 ************************************************************************/
OSErr JumpURL(PStr action)
{
	switch (FindSTRNIndex(URLActionStrn,action))
	{
		case actionSupport:
			OpenAdwareURL (GetNagState (), TECH_SUPPORT_SITE, actionSupport, supportQuery, 0);
			return 0;
	}
	
	return fnfErr;
}


/************************************************************************
 * URLCombine - combine a base and a relative url
 ************************************************************************/
PStr URLCombine(PStr result,PStr base,PStr rel)
{
	DeepURLHandle baseURL=NewZH(DeepURL);
	DeepURLHandle relURL=NewZH(DeepURL);
	Str255 localResult;
	
	// move the relative url in; it may or may not stay
	PCopy(localResult,rel);
	
	if (baseURL && relURL)
	{
		DeepURLParse(rel,relURL);
		// if the relative URL has a scheme, we're done already
		if (!*(*relURL)->scheme)
		{
			DeepURLParse(base,baseURL);
			// if the base URL doesn't have a scheme, we're done
			if (*(*baseURL)->scheme)
			{
				// sigh.  Here we go.
				
				// first, we copy the scheme, since to get here the base must
				// have a scheme and the relative must not
				PCopy((*relURL)->scheme,(*baseURL)->scheme);
				
				// if the site is defined, we're pretty much done
				if (!(*relURL)->isSite)
				{
					// nope.  inherit the site
					if ((*relURL)->isSite = (*baseURL)->isSite)
						PCopy((*relURL)->site,(*baseURL)->site);
					
					// Combine the paths
					URLCombinePaths(LDRef(relURL)->path,LDRef(baseURL)->path,LDRef(relURL)->path);
				}
				DuhString(localResult,relURL);
			}
		}
	}
	
	ZapHandle(baseURL);
	ZapHandle(relURL);
	return(PCopy(result,localResult));
}

/************************************************************************
 * DuhString - convert a deep url handle to a string
 ************************************************************************/
PStr DuhString(PStr url,DeepURLHandle duh)
{
	*url = 0;
	
	// All duh's have schemes
	PCat(url,(*duh)->scheme);
	PCatC(url,':');
	
	// May or may not have a site.  If we do, add // and site
	if ((*duh)->isSite)
	{
		PCatC(url,'/');
		PCatC(url,'/');
		PCat(url,(*duh)->site);
	}
	
	// May or may not have a path.  (Weird, but...)
	// if we do, concatenate.  Will have leading slash already
	if ((*duh)->path)
	{
		PCat(url,(*duh)->path);
	}
	
	// May or may not have a query.  If we do, add ? and query
	if ((*duh)->isQuery)
	{
		PCatC(url,'?');
		PCat(url,(*duh)->query);
	}
	
	// May or may not have a fragment id.  If we do, add # and fragment
	if ((*duh)->isFragment)
	{
		PCatC(url,'#');
		PCat(url,(*duh)->fragment);
	}
	
	return(url);
}

/************************************************************************
 * URLCombinePaths - weld two paths into one
 ************************************************************************/
PStr URLCombinePaths(PStr into,PStr base,PStr rel)
{
	Str255 localInto;
	Str255 token;
	UPtr spot;
		
  if (!*rel)
  {
  	PCopy(localInto,base);						// empty rel
  	RemLastComponent(localInto);
  	if (*localInto && localInto[*localInto]!='/') PCatC(localInto,'/');
  }
  else if (!*base) PCopy(localInto,rel);			// empty base
  else if (rel[1]=='/') PCopy(localInto,rel);	// absolute rel
  else
  {
  	// Sigh.  Do it the hard way
  	PCopy(localInto,base);
  	RemLastComponent(localInto);	// delete everything after last /
  	
  	//if (*localInto && localInto[*localInto]!='/') PCatC(localInto,'/'); // make sure there is a trailing /
  	
  	if (*rel>=2 && rel[*rel]=='.' && rel[*rel-1]=='/') --*rel;  // delete dot from trailing /.'s
  	while (*rel>=3 && rel[*rel]=='/' && rel[*rel-1]=='.' && rel[*rel-2]=='/') *rel -= 2;  // delete ./ from trailing /./'s
  	
  	for (spot=rel+1;PToken(rel,token,&spot,Slash+1);)
  	{
  		if (*localInto>1)	// we have a non-zero component somewhere
  		{
  			if (*token==1 && token[1]=='.')
  			{
  				;	// ignore current directory specification
 					if (localInto[*localInto]!='/') PCatC(localInto,'/');	// but make sure path ends in /
				} 				
  			else if (*localInto>1 && *token==2 && token[1]==token[2] && token[2]=='.' && !TrailingDotDot(localInto))
  			{
 					if (localInto[*localInto]=='/' && *localInto>1) --*localInto;
  				RemLastComponent(localInto);
 					if (localInto[*localInto]!='/') PCatC(localInto,'/');	// make sure path ends in /
  			}
  			else
  			{
					if (localInto[*localInto]!='/') PCatC(localInto,'/');
  				PSCat(localInto,token);
  			}
  		}
  		else
			{
				if (localInto[*localInto]!='/') PCatC(localInto,'/');
				PSCat(localInto,token);
			}
  	}
  	if (rel[*rel]=='/' && localInto[*localInto]!='/') PCatC(localInto,'/');
  }
  PCopy(into,localInto);
  return(into);
}

/************************************************************************
 * TrailingDotDot - does the string end in a component of ".."?
 ************************************************************************/
Boolean TrailingDotDot(PStr string)
{
	Str255 local;
	
	PCopy(local,string);
	if (local[*local]=='/') --*local;	// trim trailing /
	if (*local<2) return(False);
	if (local[*local]!='.') return(False);
	if (local[*local-1]!='.') return(False);
	if (*local==2 || local[*local-2]=='/') return(True);
	return(False);
}
	
/************************************************************************
 * RemLastComponent - remove the last component of a url, unless that is
 *  a single slash
 ************************************************************************/
PStr RemLastComponent(PStr path)
{
	UPtr spot;
	
	for (spot=path+*path;spot>path;spot--)
		if (*spot=='/')
		{
			*path = spot-path-1;
			break;
		}
	return(path);
}

/************************************************************************
 * DeepURLParse - parse a url into lots of components
 ************************************************************************/
typedef enum
{
	urlsNot,	// we don't know what we have
	urlsScheme,	// just saw a scheme
	urlsColon,	// just saw the colon after the scheme
	urlsFirstSlash, // just saw our first slash
	urlsSecondSlash,	// just saw a second slash
	urlsSite,	// just saw a site component
	urlsQuestion,	// just saw a question mark
	urlsPoundSign,	// just saw a pound sign
	urlsAllDone
} URLStateEnum;

void DeepURLParse(PStr url, DeepURLHandle duh)
{
	UPtr spot, end;
	Str255 token;
	URLStateEnum state=urlsNot;
	Byte c;

	*url = RemoveChar(' ',url+1,*url);
	*url = RemoveChar('\015',url+1,*url);
	*url = RemoveChar('\t',url+1,*url);

	end = url+*url+1;
	
	for (spot=url+1;c=URLToken(url,token,&spot);)
	{
		switch (state)
		{
			case urlsNot:
				switch (c)
				{
					case '/': state=urlsFirstSlash; break;
					case '?': state=urlsQuestion; (*duh)->isQuery=True; break;
					case '#': state=urlsPoundSign; (*duh)->isFragment=True; break;
					default:
						if ( ('a'<=c && c<='z' || 'A'<=c && c<='Z' ||						// legal scheme chars
						 			'0'<=c && c<='9' || c=='+' || c=='-' || c=='.')		// legal scheme chars
						 		&& spot<end && *spot==':')	// followed by colon
						{
							// we have a scheme
							spot++;	// skip colon
							PCopy((*duh)->scheme,token);
							state = urlsScheme;
						}
						else
						{
							PCopy((*duh)->path,token);
							state = urlsSite;
						}
						break;
				}
				break;
			
			case urlsScheme:
				switch(c)
				{
					case '/': state=urlsFirstSlash; break;
					case '?': state=urlsQuestion; (*duh)->isQuery=True; break;
					case '#': state=urlsPoundSign; (*duh)->isFragment=True; break;
					default:
						// must be a path
						PCopy((*duh)->path,token);
						state = urlsSite;
						break;
				}
				break;
			
			case urlsFirstSlash:
				if (c=='/')
				{
					state = urlsSecondSlash;
					(*duh)->isSite = True;
				}
				else
				{
					PCatC((*duh)->path,'/');
					switch(c)
					{
						case '?': state=urlsQuestion; (*duh)->isQuery=True; break;
						case '#': state=urlsPoundSign; (*duh)->isFragment=True; break;
						default:
							state = urlsSite;
							PCat((*duh)->path,token);
							break;
					}
				}
				break;
			
			case urlsSecondSlash:
				state = urlsSite;
				switch(c)
				{
					case '/': PCopy((*duh)->path,token); break;	// empty site
					case '?': state=urlsQuestion; (*duh)->isQuery=True; break;
					case '#': state=urlsPoundSign; (*duh)->isFragment=True; break;
					default: PCopy((*duh)->site,token); break;
				}
				break;
			
			case urlsSite:
				switch(c)
				{
					case '?': state=urlsQuestion; (*duh)->isQuery=True; break;
					case '#': state=urlsPoundSign; (*duh)->isFragment=True; break;
					default: PCat((*duh)->path,token); break;
				}
				break;				
			
			case urlsQuestion:
				if (c=='#') {state=urlsPoundSign; (*duh)->isFragment=True;}
				else PCat((*duh)->query,token);
				break;
			
			case urlsPoundSign:
				PCat((*duh)->fragment,token);
				break;
			
			default:
				ASSERT(0);	// we don't belong here!
				break;
		}
	}	
}

/************************************************************************
 * URLToken - tokenize a url
 ************************************************************************/
Byte URLToken(PStr url,PStr token,UPtr *spot)
{
	UPtr end = url+*url+1;
	Byte c;
	
	token[0] = token[1] = 0;
	
	if (*spot<end)
	{
		// pop off first char
		c = **spot;
		++*spot;
		PCatC(token,c);
		
		if (c=='/' || c=='?' || c==':' || c=='#')
			; // we're done
		else
			for (c=**spot;!(c=='/' || c=='?' || c==':' || c=='#') && *spot<end;c=*++*spot)
				PCatC(token,c);
	}
	
	return(token[1]);
}

/************************************************************************
 * URLEscape - escape "dangerous" characters in a url
 ************************************************************************/
PStr URLEscape(PStr url)
{
	return URLEscapeLo(url, false);
}

PStr URLEscapeLo(PStr url,Boolean allPercents)
{
	Str255 local;
	UPtr into, from, end;
	
	TrimAllWhite(url);
	
	into = local+1;
	end = url + *url + 1;
	for (from=url+1;from<end && into<local+sizeof(local)-1;from++)
	{
		switch (*from)
		{
			case '%':
				if (!allPercents && end-from>2 && IsHexDig(from[1]) && IsHexDig(from[2]))
				{
					// add the percent, because it's (cross your fingers) part of a %-literal
					*into++ = '%';
					break;
				}
				// else fall through and escape
			case ' ':
			case '<':
			case '>':
			case '"':
				if (into-local<sizeof(local)-4)
				{
					*into++ = '%';
					Bytes2Hex(from,1,into);
					into += 2;
				}
				break;
			default:
				*into++ = *from;
				break;
		}
	}
	
	*local = into-local-1;
	PCopy(url,local);
	return(url);
}

/************************************************************************
 * URLPathEscape - escape "dangerous" characters in a path component
 ************************************************************************/
PStr URLPathEscape(PStr path)
{
	Str255 local;
	UPtr into, from, end;
	
	into = local+1;
	end = path + *path + 1;
	for (from=path+1;from<end && into<local+sizeof(local)-1;from++)
	{
		if (*from>'~' || !IsWordChar[*from] && ('0'>*from || '9'<*from))
		{
				if (into-local<sizeof(local)-4)
				{
					*into++ = '%';
					Bytes2Hex(from,1,into);
					into += 2;
				}
		}
		else
				*into++ = *from;
	}
	
	*local = into-local-1;
	PCopy(path,local);
	return(path);
}

/************************************************************************
 * URLQueryEscape - escape "dangerous" characters in a query component parameter
 ************************************************************************/
PStr URLQueryEscape(PStr query)
{
	Str255 local;
	UPtr into, from, end;
	
	into = local+1;
	end = query + *query + 1;
	for (from=query+1;from<end && into<local+sizeof(local)-1;from++)
	{
		switch (*from)
		{
			case '%':
			case '&':
			case '?':
			case ':':
			case ' ':
				if (into-local<sizeof(local)-4)
				{
					*into++ = '%';
					Bytes2Hex(from,1,into);
					into += 2;
				}
				break;
			default:
				if (*from & 0x80)
				{
					*into++ = '%';
					Bytes2Hex(from,1,into);
					into += 2;
				}
				else
					*into++ = *from;
				break;
		}
	}
	
	*local = into-local-1;
	PCopy(query,local);
	return(query);
}

/************************************************************************
 * URLSubstitute - change one url into another for MHTML.  Returns fnfErr if
 * no file exists for URL
 ************************************************************************/
OSErr URLSubstitute(PStr resultURL,PStr mhtmlIDStr,PStr origURL,PETEHandle pte)
{
	short item;
	MyWindowPtr win = (*PeteExtra(pte))->win;
	StackHandle parts;
	uLong hash;
	PartDesc pd;
	Str255 cid;
	UPtr spot;
	
	if (GetWindowKind(GetMyWindowWindowPtr(win))==MESS_WIN)
	{
		parts = (*PeteExtra(pte))->partStack;
		StringToNum(mhtmlIDStr,&hash);
		spot = origURL+1;
		PToken(origURL,cid,&spot,":");
		if (EqualStrRes(cid,ProtocolStrn+proCID))
		{
			PCopy(cid,origURL);
			// remove six chars; cid://
			*cid -= 6;
			BMD(cid+7,cid+1,*cid);
			MyLowerStr(cid);
			hash = HashWithSeed(cid,hash);
		}
		else
		{
			MyLowerStr(origURL);
			hash = HashWithSeed(origURL,hash);
		}
		for (item=0;item<(*parts)->elCount;item++)
		{
			StackItem(&pd,item,parts);
			if (pd.absURL==hash || pd.cid==hash)
			{
				MakeFileURL(resultURL,&pd.spec,0);
				return(noErr);
			}
		}
	}
	return(fnfErr);
}

/************************************************************************
 * function - purpose
 ************************************************************************/
PStr MakeFileURL(PStr url,FSSpecPtr spec,short proto)
{
	Str255 name;
	long dirID;
	CInfoPBRec hfi;
	OSErr err;
	
	/*
	 * filename
	 */
	PCopy(name,spec->name);
	URLPathEscape(name);
	PCopy(url,name);
	
	/*
	 * folders
	 */
	for (dirID = spec->parID; dirID!=2; dirID = hfi.dirInfo.ioDrParID)
	{
		Zero(hfi);
		*name = 0;
		hfi.dirInfo.ioNamePtr = name;
		hfi.dirInfo.ioFDirIndex = -1;
		hfi.dirInfo.ioVRefNum = spec->vRefNum;
		hfi.dirInfo.ioDrDirID = dirID;
		if (err=PBGetCatInfoSync(&hfi))
			hfi.dirInfo.ioDrParID = 2;
		URLPathEscape(name);
		PCatC(name,Slash[1]);
		PInsert(url,255,name,url+1);
	}
	GetMyVolName(spec->vRefNum,name);
	URLPathEscape(name);
	PCatC(name,Slash[1]);
	PInsert(url,255,name,url+1);
	
	GetRString(name,proto ? proto : ProtocolStrn+proFile);
	PCatC(name,':');
	PCatC(name,Slash[1]);
	PCatC(name,Slash[1]);
	PCatC(name,Slash[1]);
	PInsert(url,255,name,url+1);
	return(url);
}

/************************************************************************
 * URLOkHere - is it ok to put a URL here?
 ************************************************************************/
Boolean URLOkHere(PETEHandle pte)
{
	return((*PeteExtra(pte))->win->hasSelection);
}

Boolean RemoveOk;

/************************************************************************
 * URLDlgFilter - filter proc for url dialog
 ************************************************************************/
pascal Boolean URLDlgFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	ControlHandle cntl;
	Str255 text;
	extern pascal Boolean DlgFilter(DialogPtr dgPtr,EventRecord *event,short *item);
	
	GetDIText(dgPtr,urldText,text);
		
	GetDialogItemAsControl(dgPtr,urldOK,&cntl);
	if (cntl) ActivateMyControl(cntl,*text!=0);
	
	GetDialogItemAsControl(dgPtr,urldRemove,&cntl);
	if (cntl) ActivateMyControl(cntl,RemoveOk);
	
	return(DlgFilter(dgPtr,event,item));
}


/************************************************************************
 * InsertURL - ask the user for a url and insert it under the current selection
 ************************************************************************/
OSErr InsertURL(PETEHandle pte)
{
	long start, end;
	Str255 url;
	Str255 proto;
	DialogPtr dgPtr;
	MyWindowPtr	dgPtrWin;
	short item;
	extern ModalFilterUPP DlgFilterUPP;
	OSErr err = noErr;
	DECLARE_UPP(URLDlgFilter,ModalFilter);
	
	INIT_UPP(URLDlgFilter,ModalFilter);
	PeteGetTextAndSelection(pte,nil,&start,&end);
	ClearLink(pte,false,start,end,nil,&start,&end,url);
	RemoveOk = *url != 0;
	
	if (dgPtrWin = GetNewMyDialog(URL_DLOG,nil,nil,InFront))
	{
		dgPtr = GetMyWindowDialogPtr(dgPtrWin);
		
		SetDIText(dgPtr,urldText,url);
		StartMovableModal(dgPtr);
		ShowWindow(GetDialogWindow(dgPtr));
		HiliteButtonOne(dgPtr);
		
		do
		{
			MovableModalDialog(dgPtr,URLDlgFilterUPP,&item);
			GetDIText(dgPtr,urldText,url);
		}
		while (item==urldOK && (ParseURL(url,proto,nil,nil) || !*proto) && ComposeStdAlert(kAlertNoteAlert,NOT_URL)==2);
		
		EndMovableModal(dgPtr);
		CloseMyWindow(GetDialogWindow(dgPtr));
		
		if (item==urldRemove)
		{
			*url = 0;
			item = urldOK;
		}
		
		if (item==urldOK)
		{	
			URLEscape(url);
			InsertURLLo(pte,start,end,url);
		}
	}
	else err = ResError();

	return(err);
}

/************************************************************************
 * InsertURLLo - insert a specific url at a specific spot
 *  We're supposed to wind up with:
 *   <http://bite/me>BITE ME
 *   The <>'s will be linkLabel, as will the BITE ME
 *   The url will be linkLabel|URLLabel
 *   There will be a nil graphic over the <>'s and url
 ************************************************************************/
OSErr InsertURLLo(PETEHandle pte,long start,long end,PStr url)
{
	PETEStyleEntry pse;
	OSErr err;
	Str255 localURL;
	long urlStart;
	
	PeteCalcOff(pte);
	
	urlStart = start;	// in case no url there already, use start
	ClearLink(pte,false,start,end,&urlStart,&start,&end,nil);
	PetePrepareUndo(pte,peUndoMoreLink,urlStart,end,nil,nil);
	ClearLink(pte,true,start,end,nil,&start,&end,nil);
	
	*localURL = 0;
		
	/*
	 * ok, let's insert the URL now
	 */
	if (*url)
	{
		PeteStyleAt(pte,start,&pse);
		*localURL = 0;
		if (url[1]!='<') PCatC(localURL,'<');
		PCat(localURL,url);
		if (localURL[*localURL]!='>') PCatC(localURL,'>');
		
		// Make the link link-labelled
		err = PeteLabel(pte,start,end,pLinkLabel,pLinkLabel|pURLLabel); ASSERT(!err);
		
		// Insert the url
		if (!err)
		{
			pse.psStartChar = 0;
			pse.psGraphic = 1;
			pse.psStyle.graphicStyle.graphicInfo = nil;
			**Pslh = pse;
			err = PETEInsertTextPtr(PETE,pte,start,localURL+1,*localURL,Pslh); ASSERT(!err);
			urlStart = start;
		}
		
		// Set the labels
		if (!err) err = PeteLabel(pte,start,start+*localURL,pLinkLabel,pLinkLabel|pURLLabel); ASSERT(!err);
		if (!err) err = PeteLabel(pte,start+1,start+*localURL-1,pLinkLabel|pURLLabel,pLinkLabel|pURLLabel); ASSERT(!err);
	}
			
	// and cleanup
done:
	if (err) PeteKillUndo(pte);
	else
	{
		PeteGetTextAndSelection(pte,nil,nil,&end);
		PeteFinishUndo(pte,peUndoMoreLink,urlStart,end);
	}
	PeteCalcOn(pte);
	return(err);
}

/************************************************************************
 * ClearLink - Clear all links in a selection
 ************************************************************************/
OSErr ClearLink(PETEHandle pte,Boolean clear,long start, long end, long *urlStartP, long *newStart, long *newEnd,PStr oldURL)
{
	long linkStart, linkEnd;
	long urlStart, urlEnd;
	Boolean foundLink = false;
	Handle text;
	Str255 url;

	if (oldURL) *oldURL = 0;
	
	/*
	 * is there a link there already?
	 */
	for (linkStart = start;
			 !PETEFindLabelRun(PETE,pte,linkStart,&linkStart,&linkEnd,pLinkLabel,pLinkLabel) && linkStart < end;
			 linkStart = linkEnd)
	{
		if (urlStartP) {*urlStartP=linkStart; urlStartP=nil;}
		// we have a link
		if (!PETEFindLabelRun(PETE,pte,linkStart,&urlStart,&urlEnd,pLinkLabel|pURLLabel,pLinkLabel|pURLLabel))
		{
			// we have a url in the link
			
			// shall we copy it?
			if (oldURL && !*oldURL)
			{
				PeteGetTextAndSelection(pte,&text,nil,nil);
				MakePStr(url,*text+urlStart,urlEnd-urlStart);
				PCopy(oldURL,url);
			}
			
			if (!clear) return(noErr);
			
			// Delete the URL
			urlStart--; urlEnd++;
			ASSERT(urlStart==linkStart);
			ASSERT(urlEnd<linkEnd);
			PeteDelete(pte,urlStart,urlEnd);
			// adjust pointers
			if (start>urlEnd) start -= urlEnd - urlStart;
			else if (start>urlStart) start -= start-urlStart;
			linkEnd -= urlEnd-urlStart;
			end -= urlEnd-urlStart;
			
			// expand insertion point to link
			if (start==end)
			{
				start = linkStart;
				end = linkEnd;
			}
			
			// clear the linkness
			PeteLabel(pte,linkStart,linkEnd,0,pLinkLabel|pURLLabel);
			// record the very first link spot
			if (newStart) {*newStart = start; newStart=nil;}
		}
	}
	
	// if we adjusted the end, return it	
	if (newEnd) *newEnd = end;
	
	return(noErr);
}

/**********************************************************************
 * URLIsNaughty - is this a bad url?  Asks the user to see for sure
 **********************************************************************/
short URLIsNaughty(PStr linkText,Handle hURL,Boolean interact)
{
	Str255 urlHost, linkHost;
	Str255 s;
	short result = URLIsNaughtyLo(linkText,hURL,urlHost,linkHost);
		
	if (result && interact)
	{
		ComposeRString(s,URLNaughtyReasonStrn+result,urlHost,linkHost);
		if (kAlertStdAlertCancelButton!=ComposeStdAlert(Caution,NAUGHTY_URL_ALERT,s)) return 0;
	}
	return result;
}


/**********************************************************************
 * URLIsNaughtyLo - is this a bad url?
 **********************************************************************/
short URLIsNaughtyLo(PStr linkText,Handle hURL,PStr urlHost,PStr linkHost)
{
	Str255 url, urlProto, s;
	Str255 linkProto;
	OSErr err = noErr;
	
	// copy as much of it as will fit into a string
	MakePStr(url,*hURL,GetHandleSize(hURL));
		
	// Let's start by parsing the URL, shall we?
	if (ParseURL(url,urlProto,urlHost,s)) return kURLNSyntax;	// not a url

	if (urlHost[*urlHost]=='.') --*urlHost;	// ignore trailing "."

	// We try to do these tests in reverse order of badness, so that the
	// worst problem is given to the user
	
	// Try to parse the link text as a url

	// Make a local copy of the link text
	PCopy(s,linkText);
	
	// semicolon, shmemicolon
	if (PIndex(s,';') && (!PIndex(s,':') || PIndex(s,';')<PIndex(s,':')))
		*PIndex(s,';') = ':';
	
	// Kill whitespace; it's not significant here, and can be used to fool us
	CollapseLWSP(TrimAllWhite(s));
	PStripChar(s,' ');
	
	if (fnfErr==ParseURL(s,linkProto,linkHost,s))
	{
		GetRString(s,HTTP);
		PCat(s,"\p://");
		PInsert(linkText,255,s,linkText+1);
		err = ParseURL(linkText,linkProto,linkHost,s);
		if (linkHost[*linkHost]=='.') --*linkHost;  // ignore trailing "."
	}
		
	// if the link is a URL, it better be similar to the actual URL!
	if (!err && StrIsItemFromRes(urlProto,URL_HOST_CHECK_PROTOS,","))
	{
		if (IsHostname(linkHost))
		{
			// strip noise prefixes from url hosts
			StripLeadingItems(linkHost,URL_NOTNAUGHTY_PREFIXES);
			StripLeadingItems(urlHost,URL_NOTNAUGHTY_PREFIXES);
			if (!StringSame(urlHost,linkHost)) return kURLNLinkMismatch;
		}
	}
	
#ifdef DARN_EUROPEENS_HAD_CREATIVITY
	// Does the host contain any tld's?
	if (!EndsWithItem(urlHost,URL_NAUGHTY_EXCEPTIONS))
	{
		StripCountryFromHost(s,urlHost);
		if (ItemFromResAppearsInStr(NAUGHTY_URL_TLDS,s,",")) return kURLNTLD;
	}
#endif // DARN_EUROPEENS_HAD_CREATIVITY

	// Is the host numeric?
	PCopy(s,urlHost);
	PStripChar(s,'.');
	if (*s && AllDigits(s+1,*s)) return kURLNIP;
	
	// Is the host encoded?
	if (PIndex(urlHost,'%')) return kURLNEncoded;
		
	// couldn't find anything
	return 0;
}

/**********************************************************************
 * StripCountryFromHost - get country-code off a hostname
 **********************************************************************/
PStr StripCountryFromHost(PStr stripped,PStr clothed)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	GetRString(s,COUNTRY_DOMAINS);
	spot = s+1;
	PCopy(stripped,clothed);
	
	while (PToken(s,token,&spot,","))
	{
		PInsertC(token,sizeof(token),'.',token+1);
		if (EndsWith(stripped,token))
		{
			*stripped -= *token;
			break;
		}
	}
	
	return stripped;
}
	
/**********************************************************************
 * URLHelpTagList - Display the url and maybe a warning if the mouse is
 *  over a link
 **********************************************************************/
Boolean URLHelpTagList(PETEHandle pte,Point mouse)
{
	EventRecord event;
	
	for (;pte;pte=PeteNext(pte))
	{
		if (PtInPETE(mouse,pte))
		{
			OSEventAvail(nil,&event);
			URLHelpTag(pte,mouse);
			return(True);
		}
	}
	MyHMHideTag();
	return(False);
}

/**********************************************************************
 * URLHelpTag - Display the url and maybe a warning if the mouse is
 *  over a link
 **********************************************************************/
void URLHelpTag(PETEHandle pte,Point mouse)
{
	Str255 link, warning;
	Handle hURL = nil;
	HMHelpContentRec hmr;
	static PETEHandle oldPTE;
	static long oldOffset;
	long offset;
	
	*link = *warning = 0;
	
	// if different PTE, get rid of old tag
	if (pte!=oldPTE) 
	{
		oldOffset = -1;
		MyHMHideTag();
	}
	oldPTE = pte;
	
	// are we in the region?
	if (PtInPETE(mouse,pte) && !PETEPositionToOffset(PETE,pte,mouse,&offset))
	{
		// if we're at the same place, nevermind
		if (oldOffset != offset)
		{
			MyHMHideTag();
			oldOffset = offset;
			
			// got something under the cursor?
			if ( (!PrefIsSet(PREF_NO_SCAMWATCH_TIPS)) && (PeteLinkAt(pte,offset,&hURL,link)) )
			{
				Str255 urlHost, linkHost;
				short result;
				LHElement lh;
				Point pt = mouse;
				
				// jump through hoops for the brain-dead help manager
				Zero(hmr);
	    	hmr.version = kMacHelpVersion; 
	    	hmr.tagSide = kHMDefaultSide;
	    	PETEOffsetToPosition(PETE,pte,offset,&pt,&lh);
	    	LocalToGlobal(&mouse);
	    	LocalToGlobal(&pt);
	    	SetRect(&hmr.absHotRect,mouse.h-1,pt.v,mouse.h+1,pt.v+lh.lhHeight);
				hmr.content[kHMMinimumContentIndex].contentType = kHMPascalStrContent;
				
				// generate a warning, if we need to
				if (result=URLIsNaughtyLo(link,hURL,urlHost,linkHost))
				{
					ComposeRString(warning,URLNaughtyReasonStrn+result,urlHost,linkHost);
					PCatC(warning,'\015');
					PCatC(warning,'\015');
				}
				else
				{
					// cut them slack if anchor and href are the same
					MakePStr(warning,LDRef(hURL),GetHandleSize(hURL));
					if (StringSame(warning,link))
					{
						ZapHandle(hURL);
						return;
					}
					*warning = 0;
				}
				
				// copy the URL text to the link
				MakePStr(link,LDRef(hURL),GetHandleSize(hURL));
				
				// stick it all in and display it
				PSCopy(hmr.content[kHMMinimumContentIndex].u.tagString,warning);
				PSCat(hmr.content[kHMMinimumContentIndex].u.tagString,link);
	    	hmr.content[kHMMaximumContentIndex].contentType = kHMNoContent; 
				MyHMDisplayTag(&hmr);
				ZapHandle(hURL);
			}
		}
	}
}

/**********************************************************************
 * PeteLinkAt - is there a link here?
 *  If there is, we'll return the URL and the link text
 **********************************************************************/
Boolean PeteLinkAt(PETEHandle pte,long offset,Handle *hURL,PStr link)
{
	PETEStyleEntry pse;
	uLong urlStart, urlStop;
	uLong linkStart, linkStop;
	Str255 linkText;
	Handle text;
	
	if (!PeteStyleAt(pte,offset,&pse) && (pse.psStyle.textStyle.tsLabel&pLinkLabel))
	// whole think is link, get it all
	if (!PETEFindLabelRun(PETE,pte,offset,&urlStart,&urlStop,pLinkLabel,pLinkLabel))
	// get just the link part, which is at the end
	if (!PETEFindLabelRun(PETE,pte,urlStop-1,&linkStart,&linkStop,pLinkLabel,pLinkLabel|pURLLabel))
	if (!PeteGetTextAndSelection(pte,&text,nil,nil))
	{
		// the actual link starts after a '>' character, so skip first char
		MakePStr(linkText,*text+linkStart+1,linkStop-linkStart-1);
		PCopy(link,linkText);
		// the url starts at the second char of the whole thing
		if (!PETEFindLabelRun(PETE,pte,urlStart+2,&urlStart,&urlStop,pURLLabel,pURLLabel))
		if (!PETEGetText(PETE,pte,urlStart,urlStop,hURL))
			return true;
	}
	return false;
}

	
#pragma segment Main

/**********************************************************************
 * URLScan - scan open texts for URL's and hilite them
 *	Finds one URL at a time
 **********************************************************************/
void URLScan(void)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	PETEHandle pte;
	Str255 title;
	PETEDocInfo info;
	
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
	{
		if (IsKnownWindowMyWindow(winWP))
		{
			win = GetWindowMyWindowPtr(winWP);
			for (pte=win->pteList;pte;pte=PeteNext(pte))
			{
				if (errAECorruptData==PETEGetDocInfo(PETE,pte,&info))
				{
					WindowPtr	pteWinWP = GetMyWindowWindowPtr((*PeteExtra(pte))->win);
					*title = 0;
					GetWTitle(pteWinWP,title);
					ComposeStdAlert(kAlertCautionAlert,DOC_DAMAGED_FMT_ASTR+ALRTStringsOnlyStrn,title);
					CloseMyWindow(pteWinWP);
					return;
				}
				while ((*PeteExtra(pte))->urlScanned >= 0)
				{
					UsingWindow(winWP);
					PeteURLScan(win,pte);
					if (NEED_YIELD) return;
				}
			}
		}
	}
}
