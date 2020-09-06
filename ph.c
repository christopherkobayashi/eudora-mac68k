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

#include "ph.h"
#define FILE_NUM 29
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * handling the Ph panel
 **********************************************************************/

#pragma segment Ph

/************************************************************************
 * the structure for the ph window
 ************************************************************************/
typedef struct UserEntryStruct *UserEntryPtr, **UserEntryHandle;
typedef struct UserEntryStruct
{
	Str255 address;
	Str63 name;
	long start;
	long end;
	UHandle **fields;
	UserEntryHandle next;
}  UserEntry;

enum {
	phcOkButton=0,
	phcListButton,
	phcFingerButton,
	phcToButton,
	phcCcButton,
	phcBccButton,
	phcInControl,
	phcOutControl,
	phcLimit
};

static short PhCntlIds[] = {PH_CNTL,PH_SERVER_CNTL,FINGER_CNTL,MAKE_TO_CNTL,MAKE_TO_CNTL,MAKE_TO_CNTL,PH_XLIN_CNTL,PH_XLOUT_CNTL};

typedef struct
{
	PETEHandle qPTE;
	PETEHandle aPTE;
	Rect qRect;
	Rect aRect;
	ControlHandle controls[phcLimit];
	Str255 label;
	Str255 fingerServer;
	DirSvcLookupType dirSvcType;
	Str255 dirSvcServer;
	long dirSvcPort;
	Boolean dirSvcHostResolved;
	Str255 ldapBaseObject;
	short ldapSearchScope;
	LDAPAttrListHdl ldapAttrList;
	short fallback;
	AAHandle siteinfo;
	MyWindowPtr win;
	UserEntryHandle userList;
	short outlined;
	Boolean selfURL;
	Boolean live;
	long queryTicks;
	long dirtyTicks;
	long lastDirty;
	TransStream phStream;
} PhType, *PhPtr, **PhHandle;
PhHandle PhGlobals; 		/* for the ph window */
#define QPTE (*PhGlobals)->qPTE
#define APTE (*PhGlobals)->aPTE
#define Win (*PhGlobals)->win
#define QRect (*PhGlobals)->qRect
#define ARect (*PhGlobals)->aRect
#define Fallback (*PhGlobals)->fallback
#define OkButton (*PhGlobals)->controls[phcOkButton]
#define ListButton (*PhGlobals)->controls[phcListButton]
#define ToButton (*PhGlobals)->controls[phcToButton]
#define CcButton (*PhGlobals)->controls[phcCcButton]
#define BccButton (*PhGlobals)->controls[phcBccButton]
#define FingerButton (*PhGlobals)->controls[phcFingerButton]
#define Win (*PhGlobals)->win
#define Live (*PhGlobals)->live
#define Label (*PhGlobals)->label
#define XlOutControl (*PhGlobals)->controls[phcOutControl]
#define XlInControl (*PhGlobals)->controls[phcInControl]
#define Controls (*PhGlobals)->controls
#define SiteInfo (*PhGlobals)->siteinfo
#define UserList (*PhGlobals)->userList
#define Outlined (*PhGlobals)->outlined
#define FingerServer (*PhGlobals)->fingerServer
#define DirSvcType (*PhGlobals)->dirSvcType
#define DirSvcServer (*PhGlobals)->dirSvcServer
#define DirSvcPort (*PhGlobals)->dirSvcPort
#define DirSvcHostResolved (*PhGlobals)->dirSvcHostResolved
#define LDAPBaseObject (*PhGlobals)->ldapBaseObject
#define LDAPSearchScope (*PhGlobals)->ldapSearchScope
#define LDAPAttrList (*PhGlobals)->ldapAttrList
#define SelfURL (*PhGlobals)->selfURL
#define LastDirty (*PhGlobals)->lastDirty
#define QueryTicks (*PhGlobals)->queryTicks
#define DirtyTicks (*PhGlobals)->dirtyTicks
#define PhStream (*PhGlobals)->phStream

#define LockPhGlobals()		LDRef(PhGlobals)
#define UnlockPhGlobals()	UL(PhGlobals)

/************************************************************************
 * private functions
 ************************************************************************/
DirSvcLookupType ProtocolToDirSvcType(ProtocolEnum protocolType);
void GetDirSvcHostPrefix(Str255 hostStr, Str255 prefixStr);
long GetDefaultDirSvcPort(DirSvcLookupType lookupType);
OSErr VerifyDirSvcHost(PStr host, long port);
OSErr FigureDirSvcType(PStr host, DirSvcLookupType *lookupType, PStr lookupHost, long *lookupPort, PStr urlQueryStr);
void SetDirSvcServerGlobals(DirSvcLookupType serverType, Str255 host, long port, PStr urlQueryStr);
Boolean GetDirSvcServerGlobals(DirSvcLookupType *serverType, Str255 host, long *port);
Boolean GetPhServerGlobals(Str255 host, long *port);
Boolean GetLDAPServerGlobals(Str255 host, long *port, short *searchScope, Str255 searchBaseObject, LDAPAttrListHdl *attributesList);
void SwitchDirSvcType(DirSvcLookupType lookupType);
DirSvcLookupType GetTargetDirSvcType(void);
void ResetServerGlobals(void);
short CurDirSvcButton(void);
void InvalLookupButtons(void);
void PhIdle(MyWindowPtr win);
OSErr PhAddNickContents(DragReference drag);
OSErr PhAddAddrContents(DragReference drag);
OSErr PhRememberField(UHandle **fields,short i,PStr data);
	void PhClick(MyWindowPtr win, EventRecord *event);
	Boolean PhKey(MyWindowPtr win, EventRecord *event);
	Boolean PhMenu(MyWindowPtr win, int menu, int item, short modifiers);
	void InvalPhWindowTop(void);
	void PhUpdate(MyWindowPtr win);
	void PhDidResize(MyWindowPtr win, Rect *oldContR);
	void FormatLDAPEmailAddr(Str255 userName, Str255 emailAddr, Str255 combinedStr);
	void LDAPEntryFilter(LDAPSessionHdl ldapSession, Str255 userNameStr, Str255 emailAddressStr, long startOffset, long endOffset);
	OSErr OutputStrToAnswerField(Str255 theString);
	OSErr OutputStrIndexToAnswerField(short strIndex);
	OSErr OutputLDAPErrorMessage(Str255 errorStr, OSErr errCode, Boolean translateLDAPErrCodes);
	OSErr ReportLDAPSearchResults(LDAPSessionHdl ldapSession);
	OSErr DoLDAPLookup(Str255 ldapServer, int ldapPortNo, Str255 queryStr, Boolean queryStrIsURLQuery);
	void PerformQuery(DirSvcLookupType lookupType,PStr withCommand, Boolean cmdIsURLQuery,Boolean emptyFirst);
	void PhButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
	Boolean PhClose(MyWindowPtr win);
	int QiLine(TransStream stream,short *ordinal, UPtr text, long *size);
	short ContactQi(TransStream stream);
	short SendQiCommand(TransStream stream,UPtr cmd);
	short GetQiResponse(TransStream stream,PStr cmd);
	void CloseQi(TransStream stream);
	void PhFocus(PETEHandle pte);
	void DoFinger(TransStream stream,UPtr cmd);
	Boolean PhPosition(Boolean save, MyWindowPtr win);
	Boolean PhApp1(MyWindowPtr win, EventRecord *event);
	PhWinItemIndex CalcPhHelpItem(MyWindowPtr win, Point mouseLoc, Rect *itemRect);
	void PhHelp(MyWindowPtr win,Point mouse);
	void IncrementPhServer(UPtr serverName);
	void MakeFallbackName(UPtr serverName, short fallback);
	void NotePhServer(UPtr serverName);
	void PhActivate(MyWindowPtr win);
	void SetPhTablePref(ControlHandle c,short pref);
	PStr GetFingerServerName(PStr serverName);
	PStr GetDirSvcServerName(PStr serverName);
	OSErr UpdateSiteInfo(TransStream stream);
	void ZapUserList(void);
	void FormatPhEmail(PStr rem1, PStr rem2, PStr name, UserEntryHandle ueh);
	void PhOutline(short newControl);
	void PhSetGreys(void);
	Boolean CanQuery(void);
	Boolean CanAddress(void);
	OSErr InsertPhAddress(short txeIndex,short modifiers);
	void RedisplayPhHost(void);
	void TempPhHost(PStr host, DirSvcLookupType lookupType, PStr query);
	Boolean UEHIsSelected(UserEntryHandle ueh);
	OSErr PhMakeNick(void);
	Boolean UEHIsSelected(UserEntryHandle ueh);
	Boolean CurQueryTextIsURL(void);
	Boolean PhURLQuery(void);
void PhListServers(void);
void PhListResponse(TransStream stream,Boolean emptyFirst);
void ZapFields(UHandle **fields);
PStr PhMakeURL(PStr url);
PStr LDAPMakeURL(PStr url);
static Boolean PhFind(MyWindowPtr win,PStr what);
pascal OSErr PhSetDragContents(PETEHandle pte,DragReference drag);
OSErr PhListConnectError(short error);

DirSvcLookupType ProtocolToDirSvcType(ProtocolEnum protocolType)

{
	switch (protocolType)
	{
		case proPh:
		case proPh2:
			return phLookup;
		case proFinger:
			return fingerLookup;
		case proLDAP:
			return ldapLookup;
	}
	return invalidLookupType;
}


void GetDirSvcHostPrefix(Str255 hostStr, Str255 prefixStr)

{
	UPtr	curSpot;


	*prefixStr = 0;
	curSpot = hostStr + 1;
	PToken(hostStr, prefixStr, &curSpot, ".");
}


long GetDefaultDirSvcPort(DirSvcLookupType lookupType)

{
	short		port;


	switch (lookupType)
	{
		case phLookup:
			return GetRLong(PH_PORT);

		case ldapLookup:
			port = GetRLong(LDAP_PORT_REALLY);
			if (!port)
				port = LDAP_PORT;
			return port;

		default:
			return 0;
	}
}


OSErr VerifyDirSvcHost(PStr host, long port)

{
	OSErr					err;
	TransStream		tempStream;


	if (!host || !*host)
		return paramErr;

	tempStream = nil;
	err = NewTransStream(&tempStream);
	if (err)
		return err;

	err = ConnectTrans(tempStream, host, port, true,GetRLong(SHORT_OPEN_TIMEOUT));
	DestroyTrans(tempStream); /* ignore error */
	ZapTransStream(&tempStream);
	return err;
}


OSErr FigureDirSvcType(PStr host, DirSvcLookupType *lookupType, PStr lookupHost, long *lookupPort, PStr urlQueryStr)

{
	OSErr							err;
	ProtocolEnum			protocolType;
	Str255						urlProtocolStr, urlHostStr;
	long							urlPort;
	TransStream				tempStream;
	DirSvcLookupType	curTestType;
	long							curTestPort;
	DirSvcLookupType	guessType;
	Str255						guessPrefix;
	Str255						testPrefix;


	*lookupType = invalidLookupType;
	*lookupPort = 0;
	if (host != lookupHost)
		*lookupHost = 0;
	*urlQueryStr = 0;

	if (!host || !*host)
		return paramErr;

	err = ParseURL(host, urlProtocolStr, urlHostStr, urlQueryStr);
	if (!err)
	{
		protocolType = FindSTRNIndex(ProtocolStrn, urlProtocolStr);
		curTestType = ProtocolToDirSvcType(protocolType);
		switch (curTestType)
		{
			case phLookup:
			case ldapLookup:
				FixURLString(urlQueryStr);
				FixURLString(urlHostStr);
				if (!ParsePortFromHost(urlHostStr, urlHostStr, &urlPort))
					urlPort = GetDefaultDirSvcPort(curTestType);
				*lookupType = curTestType;
				BlockMoveData(urlHostStr, lookupHost, *urlHostStr + 1);
				*lookupPort = urlPort;
				return noErr;
			default:
				return paramErr; /* host is a URL, but not for Ph or LDAP */
		}
	}

	tempStream = nil;
	err = NewTransStream(&tempStream);
	if (err)
		return err;

	guessType = phLookup;
	GetDirSvcHostPrefix(host, testPrefix);
	if (*testPrefix)
	{
		GetRString(guessPrefix, LDAP_HOST_COMMON_PREFIX);
		if (EqualString(guessPrefix, testPrefix, false, true))
			guessType = ldapLookup;
	}

	if (guessType == phLookup)
	{
		curTestType = phLookup;
		curTestPort = GetDefaultDirSvcPort(phLookup);
		err = ConnectTrans(tempStream, host, curTestPort, true,GetRLong(SHORT_OPEN_TIMEOUT));
		if (!err)
			goto FoundExit;

		DestroyTrans(tempStream); /* ignore error */

		curTestType = ldapLookup;
		curTestPort = GetDefaultDirSvcPort(ldapLookup);
		err = ConnectTrans(tempStream, host, curTestPort, true,GetRLong(SHORT_OPEN_TIMEOUT));
		if (!err)
			goto FoundExit;
	}
	else
	{
		curTestType = ldapLookup;
		curTestPort = GetDefaultDirSvcPort(ldapLookup);
		err = ConnectTrans(tempStream, host, curTestPort, true,GetRLong(SHORT_OPEN_TIMEOUT));
		if (!err)
			goto FoundExit;

		DestroyTrans(tempStream); /* ignore error */

		curTestType = phLookup;
		curTestPort = GetDefaultDirSvcPort(phLookup);
		err = ConnectTrans(tempStream, host, curTestPort, true,GetRLong(SHORT_OPEN_TIMEOUT));
		if (!err)
			goto FoundExit;
	}

	ZapTransStream(&tempStream);
	return err; /* FINISH *//* should we just return noErr? */


FoundExit:
	DestroyTrans(tempStream);
	ZapTransStream(&tempStream);
	*lookupType = curTestType;
	if (host != lookupHost)
		BlockMoveData(host, lookupHost, *host + 1);
	*lookupPort = curTestPort;
	return noErr;
}


void SetDirSvcServerGlobals(DirSvcLookupType serverType, Str255 host, long port, PStr urlQueryStr)

{
	DirSvcType = serverType;
	BlockMoveData(host, DirSvcServer, *host + 1);
	DirSvcPort = port;
	DirSvcHostResolved = true;
#ifdef LDAP_ENABLED
	LDAPBaseObject[0] = 0;
	LDAPSearchScope = LDAP_SCOPE_SUBTREE;
	if (LDAPAttrList)
	{
		DisposeLDAPAttrList(LDAPAttrList);
		LDAPAttrList = nil;
	}
	if (serverType == ldapLookup)
	{
		LockPhGlobals();
		ParseLDAPURLQuery(urlQueryStr, LDAPBaseObject, &LDAPAttrList, &LDAPSearchScope, nil); /* ignore errors */
		UnlockPhGlobals();
	}
#endif
}


Boolean GetDirSvcServerGlobals(DirSvcLookupType *serverType, Str255 host, long *port)

{
	OSErr							err;
	DirSvcLookupType	hostType;
	Str255						actualHost;
	long							serverPort;
	Str255						urlQueryStr;


	if (!DirSvcHostResolved)
	{
		LockPhGlobals();
		err = FigureDirSvcType(DirSvcServer, &hostType, actualHost, &serverPort, urlQueryStr);
		UnlockPhGlobals();
		if (err)
			return false;
		DirSvcType = hostType;
		BlockMoveData(actualHost, DirSvcServer, *actualHost + 1);
		DirSvcPort = serverPort;
#ifdef LDAP_ENABLED
		LDAPBaseObject[0] = 0;
		LDAPSearchScope = LDAP_SCOPE_SUBTREE;
		if (LDAPAttrList)
		{
			DisposeLDAPAttrList(LDAPAttrList);
			LDAPAttrList = nil;
		}
		if (hostType == ldapLookup)
		{
			LockPhGlobals();
			ParseLDAPURLQuery(urlQueryStr, LDAPBaseObject, &LDAPAttrList, &LDAPSearchScope, nil); /* ignore errors */
			UnlockPhGlobals();
		}
#endif
		DirSvcHostResolved = true;
		RedisplayPhHost();
	}
	if (serverType)
		*serverType = DirSvcType;
	if (host)
		BlockMoveData(DirSvcServer, host, *DirSvcServer + 1);
	if (port)
		*port = DirSvcPort;
	return true;
}


Boolean GetPhServerGlobals(Str255 host, long *port)

{
	DirSvcLookupType	serverType;
	Str255						host_;
	long							port_;


	if (!GetDirSvcServerGlobals(&serverType, host_, &port_))
		return false;
	if (serverType != phLookup)
		return false;
	if (host)
		BlockMoveData(host_, host, *host_ + 1);
	if (port)
		*port = port_;
	return true;
}


Boolean GetLDAPServerGlobals(Str255 host, long *port, short *searchScope, Str255 searchBaseObject, LDAPAttrListHdl *attributesList)

{
	DirSvcLookupType	serverType;
	Str255						host_;
	long							port_;


	if (!GetDirSvcServerGlobals(&serverType, host_, &port_))
		return false;
	if (serverType != ldapLookup)
		return false;
	if (host)
		BlockMoveData(host_, host, *host_ + 1);
	if (port)
		*port = port_;
	if (searchScope)
		*searchScope = LDAPSearchScope;
	if (searchBaseObject)
		BlockMoveData(LDAPBaseObject, searchBaseObject, *LDAPBaseObject + 1);
	if (attributesList)
		*attributesList = LDAPAttrList;
	return true;
}


DirSvcLookupType GetCurDirSvcType(void)

{
	DirSvcLookupType lookupType;


	lookupType = GetPrefLong(PREF_CUR_DIR_SVC_TYPE);
	switch (lookupType)
	{
		case phLookup:
		case ldapLookup:
		case genericLookup:
			return genericLookup;
		case fingerLookup:
			return UseCTB ? genericLookup : fingerLookup;
		default:
			return genericLookup;
	}
}


void SetCurDirSvcType(DirSvcLookupType lookupType)

{
	switch (lookupType)
	{
		case phLookup:
		case ldapLookup:
		case genericLookup:
			lookupType = genericLookup;
			break;
		case fingerLookup:
			if (UseCTB)
				lookupType = genericLookup;
			break;
		default:
			return;
	}
	SetPrefLong(PREF_CUR_DIR_SVC_TYPE, lookupType);
}


void SwitchDirSvcType(DirSvcLookupType lookupType)

{
	if (lookupType != GetCurDirSvcType())
	{
		SetCurDirSvcType(lookupType);
		InvalLookupButtons();
		RedisplayPhHost();
	}
}


DirSvcLookupType GetTargetDirSvcType(void)

{
	DirSvcLookupType lookupType;


	lookupType = GetCurDirSvcType();
	if (lookupType == genericLookup)
		return DirSvcType;
	else
		return lookupType;
}


void ResetServerGlobals(void)

{
	Str255	hostStr;


	GetFingerServerName(hostStr);
	PSCopy(FingerServer, hostStr);

	LockPhGlobals();
	GetDirSvcServerName(DirSvcServer);
	UnlockPhGlobals();
	DirSvcPort = 0;
	DirSvcType = genericLookup;
	DirSvcHostResolved = false;
#ifdef LDAP_ENABLED
	LDAPBaseObject[0] = 0;
	LDAPSearchScope = LDAP_SCOPE_SUBTREE;
	if (LDAPAttrList)
	{
		DisposeLDAPAttrList(LDAPAttrList);
		LDAPAttrList = nil;
	}
#endif
}


short CurDirSvcButton(void)

{
	DirSvcLookupType lookupType;


	lookupType = GetCurDirSvcType();
	switch (lookupType)
	{
		case phLookup:
		case ldapLookup:
		case genericLookup:
			return phcOkButton;
		case fingerLookup:
			return phcFingerButton;
	}
	return phcOkButton;
}


void InvalLookupButtons(void)

{
	Rect r,rFinger;

	GetControlBounds(OkButton,&r);
	GetControlBounds(FingerButton,&rFinger);
	r.left = rFinger.left;
	InsetRect(&r, -4, -4);
	InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
}


/************************************************************************
 * OpenPh - open the ph window
 ************************************************************************/
OSErr OpenPh(PStr initialQuery)
{
	WindowPtr	winWP;
	Rect r;
	short item;
	short cntl;
	Str255 scratch;
	OSErr err;
	
	CycleBalls();
	if (!SelectOpenWazoo(PH_WIN) && !PhGlobals)
	{
		if (!(PhGlobals=NewZH(PhType)))
			{WarnUser(MEM_ERR,MemError()); goto fail;}
		Outlined = -1;

		ResetServerGlobals();

		LDRef(PhGlobals);
	
		if (!(Win = GetNewMyWindow(PH_WIND,nil,nil,BehindModal,False,False,PH_WIN)))
			{WarnUser(MEM_ERR,MemError()); goto fail;}
		winWP = GetMyWindowWindowPtr (Win);
		SetPort_(GetWindowPort(winWP));
		//ConfigFontSetup(Win);
		MySetThemeWindowBackground(Win,kThemeListViewBackgroundBrush,False);
		
		for (cntl=phcOkButton;cntl<(NewTables?phcLimit:phcInControl);cntl++)
		{
			if (!(Controls[cntl]=GetNewControl(PhCntlIds[cntl],winWP)))
				{WarnUser(MEM_ERR,MemError()); goto fail;}
			if (cntl!=phcOkButton && cntl!=phcFingerButton) LetsGetSmall(Controls[cntl]);
		}
		SetControlTitle(ToButton,GetRString(scratch,HEADER_LABEL_STRN+TO_HEAD));
		SetControlTitle(CcButton,GetRString(scratch,HEADER_LABEL_STRN+CC_HEAD));
		SetControlTitle(BccButton,GetRString(scratch,HEADER_LABEL_STRN+BCC_HEAD));
		ButtonFit(ToButton);
		ButtonFit(CcButton);
		ButtonFit(BccButton);
		
#ifdef TWO
		if (NewTables)
		{
			item = AddXlateTables(False,GetPrefLong(PREF_PH_IN),True,Control2Menu(XlInControl));
			SetControlMaximum(XlInControl,CountMenuItems(Control2Menu(XlInControl)));
			SetControlValue(XlInControl,MAX(1,item));
			item = AddXlateTables(True,GetPrefLong(PREF_PH_OUT),True,Control2Menu(XlOutControl));
			SetControlMaximum(XlOutControl,CountMenuItems(Control2Menu(XlOutControl)));
			SetControlValue(XlOutControl,MAX(1,item));
		}
#endif

#ifdef NEVER
if (RunType==Debugging)
{
	Rect r;
	short type;
	Handle h;
	
	GetControlBounds(ToButton,&r);
	NewControl((WindowPtr)Dlog, &r, "", true, 0,0,0, DEBUG_CDEF<<4, 0);
}
#endif

		r = Win->contR;
		
		if (!(err=PeteCreate(Win,&QPTE,peClearAllReturns,nil)))
			err=PeteCreate(Win,&APTE,peVScroll,nil);
		if (err) {WarnUser(PETE_ERR,err); goto fail;}
		
		PeteFontAndSize(APTE,FixedID,FixedSize);

		// (jp) We'll support a few nickname scanning features for the query field
		// Counter-productive at the moment.  SD 5/12/99 // SetNickScanning (QPTE, nickHighlight | nickComplete, kNickScanAllAliasFiles, nil, nil, nil);
		// (jp) putting back only nick completion (but not expansion)
		SetNickScanning (QPTE, nickComplete, kNickScanAllAliasFiles, PhNickFieldCheck, nil, nil);
		
		err = NewTransStream(&PhStream);
		if (err) {WarnUser(MEM_ERR,err); goto fail;}
		
		(*PeteExtra(APTE))->frame = True;
		(*PeteExtra(QPTE))->frame = True;
		PETESetCallback(PETE,APTE,(void*)PhSetDragContents,peSetDragContents);
		UL(PhGlobals);
		NewPhHost();
		Win->close = PhClose;
		Win->activate = PhActivate;
		Win->click = PhClick;
		Win->menu = PhMenu;
		Win->key = PhKey;
		Win->button = PhButton;
		Win->idle = PhIdle;
		PhFocus(QPTE);
		Win->position = PhPosition;
		Win->dontControl = True;
		Win->vPitch = FontLead;
		Win->update = PhUpdate;
		Win->app1 = PhApp1;
		Win->help = PhHelp;
		Win->find = PhFind;
		SetWinMinSize(Win,320,12*FontLead);
		Win->didResize = PhDidResize;
		MyWindowDidResize(Win,&r);
		ShowControl(OkButton);
		ShowControl(FingerButton);
		ShowMyWindow(winWP);
	}
	UserSelectWindow(GetMyWindowWindowPtr(Win));

	if (initialQuery)
	{
		PeteSetTextPtr(QPTE,initialQuery+1,*initialQuery);
		PerformQuery(genericLookup,nil,false,true);		
	}

	PhSetGreys();
	
	return(noErr);

fail:
	PhClose(Win);
	return(1);
}

Boolean PhNickFieldCheck (PETEHandle pte)

{
	return (false);
}

#pragma segment Menu
/**********************************************************************
 * PhCanMakeNick - can we make a nickname?
 **********************************************************************/
Boolean PhCanMakeNick(void)
{
	return (PhGlobals && GetMyWindowWindowPtr(Win)==FrontWindow_() && CanAddress());
}

#pragma segment Ph
/**********************************************************************
 * 
 **********************************************************************/
void PhIdle(MyWindowPtr win)
{
	if (!InBG && GetMyWindowWindowPtr(win)==FrontWindow_())
	{
		long dirty = PeteIsDirty(QPTE);
		Str255 query;
		Str31 retWord;
		long ticks = TickCount();
		Boolean live;
		DirSvcLookupType lookupType;

		lookupType = GetTargetDirSvcType();
		live = PrefIsSet(PREF_PH_LIVE) && (lookupType != fingerLookup);

		PhSetGreys();
		
		if (dirty!=LastDirty)
		{
			LastDirty = dirty;
			DirtyTicks = ticks;
		}
		
		if (live && DirtyTicks>QueryTicks && *PeteString(query,QPTE)>=GetRLong(PH_LIVE_MIN_CHARS))
		{
			if (ticks-DirtyTicks>GetRLong(PH_LIVE_MIN_TICKS))
			{
				if (!CurQueryTextIsURL())
				{
					switch (lookupType)
					{
						case phLookup:
							if (!PFindSub(GetRString(retWord,PH_RETURN_KEYWORD),query))
							{
								PCatC(query,'*');
								PerformQuery(phLookup,query,false,true);
								QueryTicks = TickCount();
							}
							break;

						case ldapLookup:
							if (query[1]!='(')
							{
								PCatC(query,'*');
								PerformQuery(lookupType,query,false,true);
								QueryTicks = TickCount();
							}
							break;

						default:
							PerformQuery(lookupType,query,false,true);
							QueryTicks = TickCount();
							break;
					}
				}
				else
				{
					QueryTicks = TickCount();
				}
			}
		}
			
		if (Live)
		{
			if ((ticks-QueryTicks)>GetRLong(InBG ? PH_BG_IDLE : PH_FG_IDLE)*60)
				if (lookupType == phLookup)
					PhKill();
		}
	}
}


/**********************************************************************
 * PhURLLookup - lookup a URL from the ph window
 **********************************************************************/
OSErr PhURLLookup(ProtocolEnum protocol,PStr host,PStr query,Handle *result)
{	
	DirSvcLookupType lookupType;


	lookupType = ProtocolToDirSvcType(protocol);
	if (lookupType == invalidLookupType)
		return paramErr;

	SetPort_(GetMyWindowCGrafPtr(Win));

	/*
	 * re-aim server if need be
	 */
	if (*host)
		TempPhHost(host, lookupType, query);
	
	/*
	 * enter query
	 */
	if (!SelfURL || *query) PeteSetTextPtr(QPTE,query+1,*query);
	else PeteSelect(Win,QPTE,0,0x7fffffff);
	
	/*
	 * do it
	 */
	if (*query)
		PerformQuery(lookupType, query, true,true);
	else
	{
		SwitchDirSvcType(lookupType);
		PhFocus(QPTE);
	}

	/*
	 * result?
	 */
	if (result) PeteGetRawText(APTE,result);

	return(noErr);
}

/**********************************************************************
 * TempPhHost - enter a ph host, but don't make it the default
 **********************************************************************/
void TempPhHost(PStr host, DirSvcLookupType lookupType, PStr query)
{
	Str255 server;
	long port;


	switch (lookupType)
	{
		case phLookup:
			if (!ParsePortFromHost(host,server,&port))
				port = GetDefaultDirSvcPort(phLookup);
			SetDirSvcServerGlobals(phLookup, server, port, query);
			break;

		case fingerLookup:
			PCopy(FingerServer,host);
			break;

#ifdef LDAP_ENABLED
		case ldapLookup:
			if (!ParsePortFromHost(host,server,&port))
				port = GetDefaultDirSvcPort(ldapLookup);
			SetDirSvcServerGlobals(ldapLookup, server, port, query);
			// Now fix the query
			if (!ParseLDAPURLQuery(query,nil,nil,nil,server))
				PCopy(query,server);	// the real query goes there
			break;
#endif
		default:
			return;
	}

	RedisplayPhHost();
	PeteSetTextPtr(APTE,nil,0);
}

/**********************************************************************
 * PhSetDragContents - add the right stuff to a drag
 **********************************************************************/
pascal OSErr PhSetDragContents(PETEHandle pte,DragReference drag)
{
	OSErr err;
	
	if (!(err = PhAddNickContents(drag)))
		err = PhAddAddrContents(drag);
	return(err ? err : handlerNotFoundErr);
}

/**********************************************************************
 * PhAddNickContents - add a NICK_FLAVOR to the drag
 **********************************************************************/
OSErr PhAddNickContents(DragReference drag)
{
	OSErr err = noErr;
	Accumulator nickData;
	UserEntryHandle ueh;
	Accumulator noteData;
	Str255 scratch, label;
	long len;
	short nf;
	short	dummy=-1;
	NicknameTagMapRec	tagMap;
	Boolean	haveTagMap;
	Str255 serviceName;
	
	Zero(nickData);
	Zero(noteData);

	haveTagMap = GetNicknameTagMap (GetRString (serviceName, ProtocolStrn + proPh), DirSvcServer, &tagMap) ? false : true;

	/*
	 * gather the data
	 */
	if (!(err=AccuInit(&nickData)))
		for (ueh=UserList;!err && ueh;ueh = (*ueh)->next)
			if (UEHIsSelected(ueh) && (*ueh)->fields && !(err=AccuInit(&noteData)))
			{
				// Space holders for the address book and nickname indices (which will be ignored)
				err=AccuAddPtr(&nickData,(void*)&dummy,sizeof(dummy));
				if (!err)
					err=AccuAddPtr(&nickData,(void*)&dummy,sizeof(dummy));
				// next, the nickname
				if ((*(*ueh)->fields)[0])
				{
					len = GetHandleSize((*(*ueh)->fields)[0]);
					MakePStr(scratch,*(*(*ueh)->fields)[0],len);
				}
				else PSCopy(scratch,(*ueh)->name);
				SanitizeFN(label,scratch,NICK_BAD_CHAR,NICK_REP_CHAR,false);
				if (err = AccuAddPtr(&nickData,label,*label+1)) break;
				
				// length and address
				PSCopy(scratch,(*ueh)->address);
				ShortAddr(label,scratch);
				len = *label;
				if (err=AccuAddPtr(&nickData,(void*)&len,sizeof(len))) break;
				if (err = AccuAddStr(&nickData,label)) break;
				
				// note data
				for (nf=HandleCount((*ueh)->fields)-1;nf;nf--)
					if ((*(*ueh)->fields)[nf])
					{
						// open bracket
						if (err=AccuAddChar(&noteData,'<')) break;
						
						// field name and colon
						if (haveTagMap)
							GetIndNicknameTag (&tagMap, nf, label);
						else
							GetRString(label,PhNickNickStrn+nf+1);
						if (err=AccuAddStr(&noteData,label)) break;
						if (err=AccuAddChar(&noteData,':')) break;
						
						//data
#ifdef KNOWN_PROBLEM
						resolve problem involving quoting < and > in data
#endif
						if (err=AccuAddHandle(&noteData,(*(*ueh)->fields)[nf])) break;
						
						//closing >
						if (err=AccuAddChar(&noteData,'>')) break;
					}
				if (err) break;
				
				// the name
				if (err=AccuAddChar(&noteData,'<')) break;
				GetRString(label,NickManKeyStrn+1);
				if (err=AccuAddStr(&noteData,label)) break;
				if (err=AccuAddChar(&noteData,':')) break;
				PSCopy(label,(*ueh)->name);
				if (err=AccuAddStr(&noteData,label)) break;
				if (err=AccuAddChar(&noteData,'>')) break;
				
				// now add it
				AccuTrim(&noteData);
				len = noteData.offset;
				if (err=AccuAddPtr(&nickData,(void*)&len,sizeof(len))) break;
				if (err=AccuAddHandle(&nickData,noteData.data)) break;
				ZapHandle(noteData.data);
				Zero(noteData);
			}
	
	/*
	 * add them to the drag
	 */
	if (!err && nickData.offset)
	{
		// add to the drag
		err = AddDragItemFlavor(drag,1,kNickDragType,LDRef(nickData.data),nickData.offset,0);
	}
	
	ZapHandle(nickData.data);
	ZapHandle(noteData.data);
	DisposeNicknameTagMap (&tagMap);
	return(err);
}

/**********************************************************************
 * PhAddAddrContents - add an A822_FLAVOR to the drag
 **********************************************************************/
OSErr PhAddAddrContents(DragReference drag)
{
	OSErr err = noErr;
	Accumulator addresses;
	UserEntryHandle ueh;
	Str15 commaSpace;
	
	Zero(addresses);
	GetRString(commaSpace,COMMA_SPACE);
	
	/*
	 * gather the addresses
	 */
	if (!(err=AccuInit(&addresses)))
		for (ueh=UserList;!err && ueh;ueh = (*ueh)->next)
			if (UEHIsSelected(ueh))
			{
				LDRef(ueh);
				err = AccuAddStr(&addresses,(*ueh)->address);
				UL(ueh);
				if (!err) err = AccuAddStr(&addresses,commaSpace);
			}
	
	/*
	 * add them to the drag
	 */
	if (!err && addresses.offset>*commaSpace)
	{
		// trim unneeded trailing comma
		addresses.offset -= *commaSpace;
		AccuTrim(&addresses);
		
		// add to the drag
		err = AddDragItemFlavor(drag,1,A822_FLAVOR,LDRef(addresses.data),addresses.offset,0);
	}
		
	// and kill the temp data
	ZapHandle(addresses.data);
	return(err);
}

/************************************************************************
 * PhActivate
 ************************************************************************/
void PhActivate(MyWindowPtr win)
{
}

/**********************************************************************
 * PhFind - find in the window
 **********************************************************************/
static Boolean PhFind(MyWindowPtr win,PStr what)
{
	return FindInPTE(win,APTE,what);
}

/**********************************************************************
 * PhOutline - outline the proper button
 **********************************************************************/
void PhOutline(short newControl)
{
	if (newControl!=Outlined)
	{
		if (Outlined>=0) OutlineControl(Controls[Outlined],False);
		Outlined = newControl;
		if (Outlined>=0) OutlineControl(Controls[Outlined],True);
	}
}

/**********************************************************************
 * PhSetGreys - make sure the right greys (and outlines) are set
 **********************************************************************/
void PhSetGreys(void)
{
	Str255 contents;
	Boolean hasQuery = CanQuery();
	Boolean hasAnswer = *PeteSString(contents,APTE);
	Boolean canAddress = hasAnswer && CanAddress();
	short defaultBtn;
	
	SetGreyControl(FingerButton,UseCTB || !hasQuery);
	SetGreyControl(OkButton,!hasQuery);

	SetGreyControl(ToButton,!canAddress);
	SetGreyControl(CcButton,!canAddress);
	SetGreyControl(BccButton,!canAddress);

	if (Win->pte==QPTE)
	{
		if (hasQuery)
		{
			defaultBtn = CurDirSvcButton();
			PhOutline(defaultBtn);
		}
		else
			PhOutline(-1);
	}
	else
		PhOutline(canAddress?phcToButton:-1);
}

/**********************************************************************
 * CanQuery - has the user typed something into the query line?
 **********************************************************************/
Boolean CanQuery(void)
{
	Str255 contents;


	return (*PeteSString(contents, QPTE)) ? true : false;
}

/**********************************************************************
 * CanAddress - can we possibly address mail?
 **********************************************************************/
Boolean CanAddress(void)
{
	UserEntryHandle ueh;
	Str255 contents;
	
	if (!*PeteSString(contents,APTE)) return(False);				/* we do have some results */
	if (!UserList) return(False);		/* those results do have addresses attached */
		
	for (ueh=UserList;ueh;ueh = (*ueh)->next)
	{
		if (UEHIsSelected(ueh) && *(*ueh)->address) return(True);
	}
	
	/*
	 * selection is not in any user whose email address we know
	 */
	return(False);
}

/**********************************************************************
 * UEHIsSelected - is a ueh selected?
 **********************************************************************/
Boolean UEHIsSelected(UserEntryHandle ueh)
{
	long start,stop;
	UHandle textH;
	
	if (Win->pte==QPTE) return(True);

	if (PeteGetTextAndSelection(APTE,&textH,&start,&stop)) return(False);
	if (start>=(*ueh)->end) return(False);
	if (stop<=(*ueh)->start) return(False);
	
	return(True);
}

/************************************************************************
 * PhPosition - ph window position
 ************************************************************************/
Boolean PhPosition(Boolean save, MyWindowPtr win)
{
	Str31 ph;

	MyGetItem(GetMHandle(WINDOW_MENU),WIN_PH_ITEM,ph);
	SetWTitle_(GetMyWindowWindowPtr(Win),ph);
	return(PositionPrefsTitle(save,win));
}

/************************************************************************
 * PhButton - handle a button click in the ph window
 ************************************************************************/
void PhButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	short which;

#ifdef TWO
	if (button && button==XlOutControl)
		SetPhTablePref(button,PREF_PH_OUT);
	else if (button && button==XlInControl)
		SetPhTablePref(button,PREF_PH_IN);
	else if (button==ListButton)
	{
		if (modifiers&optionKey)
			NewPhHost();
		else
			PhListServers();
	}
	else
#endif
	if (part==kControlButtonPart)
	{
		for (which=0;which<phcLimit;which++) if (Controls[which]==button) break;
		
		switch(which)
		{
			case phcOkButton: PerformQuery(genericLookup,nil,false,true); break;
			case phcFingerButton: PerformQuery(fingerLookup,nil,false,true); break;
			case phcToButton: InsertPhAddress(TO_HEAD,modifiers); break;
			case phcCcButton: InsertPhAddress(CC_HEAD,modifiers); break;
			case phcBccButton: InsertPhAddress(BCC_HEAD,modifiers); break;
		}
		Win->hasSelection = MyWinHasSelection(win);
		PhSetGreys();
	}
	
	for (which=0;which<phcLimit;which++) if (Controls[which]==button) break;
	AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),which), mouseDown);	

}

/**********************************************************************
 * InsertPhAddress - insert addresses from the ph window
 **********************************************************************/
OSErr InsertPhAddress(short txeIndex,short modifiers)
{
	OSErr err=fnfErr;
	UserEntryHandle ueh;
	long start;
	HeadSpec hs;
	
	MyWindowPtr win = TopCompositionWindow(true, false);
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	
	if (!win)
		err = fnfErr;
	else
	{
		if (GetWindowKind(winWP)==COMP_WIN && CompHeadFind(Win2MessH(win),txeIndex,&hs))
			PetePrepareUndo(win->pte,peUndoPaste,hs.stop,hs.stop,&start,nil);
		else
			PetePrepareUndo(win->pte,peUndoPaste,-1,-1,&start,nil);
		for (ueh=UserList;ueh;ueh = (*ueh)->next)
			if (UEHIsSelected(ueh))
			{
				err = InsertAddress(win,txeIndex,LDRef(ueh)->address);
				UL(ueh);
				if (err) break;
			}
		PeteFinishUndo(win->pte,peUndoPaste,start,-1);
		
		if (!IsWindowVisible(winWP))
		{
			win->isDirty = False;
		}
		ShowMyWindow(winWP);
		if (!(modifiers&(cmdKey|shiftKey|optionKey))) UserSelectWindow(winWP);
	}
	return(err);
}

/**********************************************************************
 * InsertAddress - insert an address in a text field
 **********************************************************************/
OSErr InsertAddress(MyWindowPtr win,short txeIndex,PStr address)
{
	HeadSpec hs;
	MessHandle messH = Win2MessH(win);

	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(win));
	
	if (CompHeadFind(messH,txeIndex,&hs))
	{
		InsertCommaIfNeedBe(TheBody,&hs);
		CompHeadAppendStr(TheBody,&hs,address);
		PeteSelect(win,TheBody,hs.stop,hs.stop);
		if (!PrefIsSet(PREF_NICK_CACHE))
			CacheRecentNickname(address);
	}
	PopGWorld();
	return(noErr);
}

#ifdef TWO
/************************************************************************
 * 
 ************************************************************************/
void SetPhTablePref(ControlHandle c,short pref)
{
	short item = GetControlValue(c);
	short id=0;
	Str63 scratch;
	OSType type;
	Handle res;
	
	if (item>1)
	{
		MyGetItem(Control2Menu(c),item,scratch);
		if (res=GetNamedResource('taBL',scratch))
			GetResInfo(res,&id,&type,scratch);
	}
	
	NumToString(id,scratch);
	SetPref(pref,scratch);
}
#endif

/************************************************************************
 * PhClick - handle a click in the ph window
 ************************************************************************/
void PhClick(MyWindowPtr win, EventRecord *event)
{
	Point mouse = event->where;
	PETEHandle pte = nil;
	
	GlobalToLocal(&mouse);
	
	if (PtInPETE(mouse,QPTE))
		pte = QPTE;
	else if (PtInPETE(mouse,APTE))
		pte = APTE;
	
	if (PeteIsValid(pte))
	{		
		if (PeteEdit(win,pte,peeEvent,(void*)event)==tsmDocNotActiveErr)
		{
			PhFocus(pte);
			PeteEdit(win,pte,peeEvent,(void*)event);
		}
		if (event->modifiers&cmdKey && !URL)
		{
			PhSetGreys();
			if (CanAddress()) PhButton(win,Controls[Outlined],event->modifiers,kControlButtonPart);
		}
	}
	else HandleControl(mouse,win);

	win->hasSelection = MyWinHasSelection(win);
	PhSetGreys();
}

/************************************************************************
 * PhApp1 - handle an app1 event
 ************************************************************************/
Boolean PhApp1(MyWindowPtr win, EventRecord *event)
{
	event->what = keyDown;
	PeteEdit(win,APTE,peeEvent,(void*)event);
	return(True);
}

/************************************************************************
 * PhKey - keystrokes in the ph window
 ************************************************************************/
Boolean PhKey(MyWindowPtr win, EventRecord *event)
{
	short key = event->message & 0xff;
	UserEntryHandle ueh;
	long start,end;

	if (event->modifiers & cmdKey) {return(False);}				/* no command keys! */

	if (key==returnChar || key==enterChar)
	{
		//Undo.didClick = True;
		if (Outlined>=0)
		{
			if (!PhURLQuery())
				PhButton(Win,Controls[Outlined],event->modifiers,kControlButtonPart);
		}
		return(True);
	}
	else if (key==tabChar)
	{
		start = 0;
		end = 0x7fffffff;
		if (win->pte==QPTE)
		{
			PhFocus(APTE);
			ueh = UserList;
			if (event->modifiers&shiftKey && ueh) LL_Last(UserList,ueh);
			if (ueh)
			{
				start = (*ueh)->start;
				end = (*ueh)->end;
			}
		}
		else
		{
			for (ueh=UserList;ueh;ueh=(*ueh)->next)
				if (UEHIsSelected(ueh)) break;
			if (ueh && event->modifiers&shiftKey)
				if (ueh!=UserList)
				{
					UserEntryHandle curUEH = ueh;
					LL_Parent(UserList,curUEH,ueh);
				}
				else ueh = nil;
			else	
				for (;ueh;ueh=(*ueh)->next)
					if (!UEHIsSelected(ueh)) break;
			if (ueh)
			{
				start = (*ueh)->start;
				end = (*ueh)->end;
			}
			else PhFocus(QPTE);
		}
		
		PeteSelect(nil,win->pte,start,end);
		PeteScroll(win->pte,pseNoScroll,pseCenterSelection);
	}
	else if (win->ro && DirtyKey(key))
	{
		PhFocus(QPTE);
		PeteSelect(nil,QPTE,0,REAL_BIG);
	}
	
	if (key!=tabChar) PeteEdit(win,win->pte,peeEvent,(void*)event);

	Win->hasSelection = MyWinHasSelection(win);
	PhSetGreys();

	return(True);
}

/**********************************************************************
 * CurQueryTextIsURL - returns true if beginning of query text might
 * be a URL
 **********************************************************************/
Boolean CurQueryTextIsURL(void)
{
	Str255 query;
	UPtr curSpot;
	Str255 proto;
	Str255 scratch;


	PeteSString(query,QPTE);
	if (!*query)
		return false;
	curSpot = query + 1;

	if (!PToken(query,proto,&curSpot,":") || (curSpot[-1] != ':'))
		return false;
	if (!PToken(query,scratch,&curSpot,"/") || (curSpot[-1] != '/'))
		return false;
	if (!FindSTRNIndex(ProtocolStrn,proto))
		return false;

	return true;
}

/**********************************************************************
 * PhURLQuery - if the query is a url, execute it
 **********************************************************************/
Boolean PhURLQuery(void)
{
	Str255 query;
	Boolean result;
	
	PeteSString(query,QPTE);
	SelfURL = True;
	result = (!ParseURL(query,nil,nil,nil) && !OpenLocalURL(query,nil));
	SelfURL = False;
	return(result);
}

/************************************************************************
 * PhMenu - menu selections in the ph window
 ************************************************************************/
Boolean PhMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(modifiers)
	PETEHandle pte;
	
	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_PRINT_ITEM:
				case FILE_PRINT_ONE_ITEM:
				{
					pte = Win->pte;
					PhFocus(APTE);
					PrintOneMessage(win,modifiers&shiftKey,item==FILE_PRINT_ONE_ITEM);
					PhFocus(pte);
					return(True);
				}
			}
			break;
		case EDIT_MENU:
			switch (item)
			{
				case EDIT_FINISH_ITEM:
					return(False);
					break;
				default:
					if ((item==EDIT_PASTE_ITEM || item==EDIT_QUOTE_ITEM) && win->ro)
					{
						PhFocus(QPTE);
						PeteSelect(nil,QPTE,0,REAL_BIG);
					}
					return(TextMenu(Win,menu,item,modifiers));
					break;
			}
			break;
		case SPECIAL_MENU:
			switch (AdjustSpecialMenuSelection(item))
			{
				case SPECIAL_MAKE_NICK_ITEM:
					PhMakeNick();
					return(True);
					break;
			}
	}
	return(False);
}

/**********************************************************************
 * PhMakeNick - make a nickname from a ph entry
 **********************************************************************/
OSErr PhMakeNick(void)
{
	UserEntryHandle ueh;
	Str255 address, shortAddr;
	Handle addresses = NuHandle(0);
	OSErr err = fnfErr;
	Boolean uncomma = PrefIsSet(PREF_UNCOMMA);
	
	if (!addresses) return(WarnUser(MEM_ERR,MemError()));
	
	if (uncomma) SetPref(PREF_UNCOMMA,NoStr);
	/*
	 * gather the selected addresses
	 */
	for (ueh=UserList;ueh;ueh = (*ueh)->next)
	{
		if (UEHIsSelected(ueh))
		{
			PCopy(address,(*ueh)->address);
			ShortAddr(shortAddr,address);
			CanonAddr(address,shortAddr,(LDRef(ueh))->name);
			UL(ueh);
			address[*address+1] = 0;
			err = PtrPlusHand_(address,addresses,*address+2);
			if (err) break;
		}
	}
	if (uncomma) SetPref(PREF_UNCOMMA,YesStr);
	if (!err) err = PtrPlusHand_("",addresses,1);
	if (err)
	{
		ZapHandle(addresses);
		return(WarnUser(MEM_ERR,err));
	}
	
	/*
	 * make the nickname
	 */
#ifdef VCARD
	NewNick(addresses,nil,0);
#else
	NewNick(addresses,0);
#endif
	
	return(noErr);
}


void InvalPhWindowTop(void)

{
	GrafPtr origPort;
	Rect r;


	r = Win->contR;
	r.bottom = QRect.top;

	GetPort(&origPort);
	SetPort(GetMyWindowCGrafPtr(Win));
	InvalWindowRect(GetMyWindowWindowPtr(Win),&r);
	SetPort(origPort);
}


/************************************************************************
 * PhUpdate - update the ph window
 ************************************************************************/
void PhUpdate(MyWindowPtr win)
{
#pragma unused(win)
	Rect r;
	Str255 scratch;

	r = QRect;
	MoveTo(r.left,r.top - 6);
	PCopy(scratch,Label);
	SetSmallSysFont();
	DrawString(scratch);

	if (Outlined>=0) OutlineControl(Controls[Outlined],true);
}

/************************************************************************
 * PhDidResize - handle resizing of the ph window
 ************************************************************************/
void PhDidResize(MyWindowPtr win, Rect *oldContR)
{
#pragma unused(win,oldContR)
	Rect r,rOK,rFinger,rListButton;
	Point p;
	short hi;
	short wi;
	short aquaAdjustment = 0;
	
	// the Finger and Lookup buttons are bigger under OSX
	if (HaveOSX()) aquaAdjustment = 2;

	GetControlBounds(OkButton,&rOK);
	hi = rOK.bottom - rOK.top;
	wi = rOK.right - rOK.left;
	
	/*
	 * put the Lookup and Finger buttons in the right place
	 */
	p.h = Win->contR.right - GROW_SIZE - wi;
	p.v = Win->contR.top + GROW_SIZE/2 + 3*aquaAdjustment;
	OFFSET_RECT(&rOK,p.h-rOK.left,p.v-rOK.top);
	SetControlBounds(OkButton,&rOK);
	p.h -= INSET + wi;
	GetControlBounds(FingerButton,&rFinger);
	OFFSET_RECT(&rFinger,p.h-rFinger.left,p.v-rFinger.top);
	SetControlBounds(FingerButton,&rFinger);

	/*
	 * position the server list button
	 */
	GetControlBounds(OkButton,&r);
	r.left = r.right - (r.bottom - r.top);
	OffsetRect(&r, 0, RectHi(r) + GROW_SIZE/2);
	MoveMyCntl(ListButton,r.left,r.top+aquaAdjustment,16,16);

	/*
	 * calculate the frame for the command box
	 */
	r = Win->contR;
	GetControlBounds(ListButton,&rListButton);
	r.top = rListButton.top+2;
	r.bottom = r.top + ONE_LINE_HI(Win);
	//OffsetRect(&r,0,-(hi-r.bottom+r.top)/2);
	r.left += GROW_SIZE;
	r.right = rListButton.left - GROW_SIZE/2;
	InsetRect(&r,3,0);
	QRect = r;
	
	/*
	 * calculate the frame for the answer box
	 */
	r.top = rListButton.bottom + 2*GROW_SIZE/3;
	r.bottom = win->contR.bottom;
	r.right = win->contR.right - GROW_SIZE+1;
	r.bottom -= 3*INSET + ControlHi(ToButton);
	if (XlOutControl) r.bottom -= ControlHi(XlOutControl)+2*INSET;
	r.right -= 3;
	ARect = r;
		
	/*
	 * now for the TE's
	 */
	r = QRect;
	PeteDidResize(QPTE,&r);
	r = ARect;
	PeteDidResize(APTE,&r);

	/*
	 * To, Cc, Bcc
	 */
	hi = ControlHi(ToButton);
	wi = (r.right-r.left)/3 - 4*INSET;
	MoveMyCntl(ToButton,r.left,r.bottom+3*INSET/2,wi,hi);
	MoveMyCntl(BccButton,r.right-wi-INSET,r.bottom+3*INSET/2,wi,hi);
	MoveMyCntl(CcButton,(r.right+r.left-wi-INSET)/2,r.bottom+3*INSET/2,wi,hi);

	/*
	 * xlit controls?
	 */
	if (XlOutControl)
	{
		wi = ControlWi(XlOutControl);
		hi = ControlHi(XlOutControl);
		MoveMyCntl(XlOutControl,(r.right+r.left)/2-INSET-wi,win->contR.bottom-hi-INSET,wi,hi);
		MoveMyCntl(XlInControl,r.right-INSET-wi,win->contR.bottom-hi-INSET,wi,hi);
	}
	InvalWindowRect(GetMyWindowWindowPtr(win),&win->contR);
}

#pragma segment Main
/************************************************************************
 * PhClose - close the ph window
 ************************************************************************/
Boolean PhClose(MyWindowPtr win)
{
#pragma unused(win)
	if (PhGlobals)
	{
		PhKill();
		LockPhGlobals();
		ZapTransStream(&PhStream);
		if (SiteInfo) ZapHandle(SiteInfo);
		ZapUserList();
		ZapHandle(PhGlobals);
		PhGlobals = nil;
	}
	return(True);
}

/**********************************************************************
 * ZapUserList - kill the user list
 **********************************************************************/
void ZapUserList(void)
{
	UserEntryHandle ueh;
	
	for (ueh=UserList;ueh;ueh=UserList)
	{
		LL_Remove(UserList,ueh,(UserEntryHandle));
		ZapFields((*ueh)->fields);
		ZapHandle((*ueh)->fields);
		ZapHandle(ueh);
	}
}

/**********************************************************************
 * ZapFields - zap a list of fields
 **********************************************************************/
void ZapFields(UHandle **fields)
{
	short i;
	
	if (fields)
	{
		for (i=HandleCount(fields);i--;)
			ZapHandle((*fields)[i]);
	}
}


/* FINISH *//* should we do a #pragma segment LDAP here? */

void FormatLDAPEmailAddr(Str255 userName, Str255 emailAddr, Str255 combinedStr)
{
	if (userName[0] && emailAddr[0])
		CanonAddr(combinedStr,emailAddr,userName);
	else
		PCopy(combinedStr, emailAddr);
}


void LDAPEntryFilter(LDAPSessionHdl ldapSession, Str255 userNameStr, Str255 emailAddressStr, long startOffset, long endOffset)

{
	UserEntryHandle		userEntryHdl;
	UserEntryPtr			entryPtr;
	long							len;
#pragma unused (ldapSession)


	userEntryHdl = NewZH(UserEntry);
	if (!userEntryHdl)
		return;

	entryPtr = LDRef(userEntryHdl);

	FormatLDAPEmailAddr(userNameStr, emailAddressStr, entryPtr->address);

	len = userNameStr[0];
	if (len > 63)
		len = 63;
	BlockMoveData(userNameStr, entryPtr->name, len + 1);
	entryPtr->name[0] = len;

	entryPtr->start = startOffset;
	entryPtr->end = endOffset;

	entryPtr->fields = NewZH(UHandle);

	UL(userEntryHdl);

	LL_Queue(UserList, userEntryHdl, (UserEntryHandle));
}


OSErr OutputStrToAnswerField(Str255 theString)

{
	OSErr		err;
	Handle	textHdl;
	long		textLen;


	if (!*theString)
	{
		PeteSetTextPtr(APTE, nil, 0);
		return noErr;
	}

	textLen = theString[0] + 1; /* add one to accomodate a CR at the end */
	textHdl = NewHandle(textLen);
	if (!textHdl)
		return MemError();
	BlockMoveData(theString + 1, *textHdl, theString[0]);
	*(*textHdl + theString[0]) = 0x0D;
	err = PeteSetText(APTE, textHdl);
	DisposeHandle(textHdl);
	return err;
}


OSErr OutputStrIndexToAnswerField(short strIndex)

{
	Str255	theString;


	GetRString(theString, strIndex);
	return OutputStrToAnswerField(theString);
}


#ifdef LDAP_ENABLED
OSErr OutputLDAPErrorMessage(Str255 errorStr, OSErr errCode, Boolean translateLDAPErrCodes)

{
	OSErr		err;
	Handle	errorText;
	long		textLen;
	Str255	errCodeStr;


	textLen = errorStr[0] + 1; /* add one to accomodate a CR at the end */
	if (errCode)
	{
		if (translateLDAPErrCodes)
		{
			err = LDAPErrCodeToMsgStr(errCode, errCodeStr, true);
			if (err)
				NumToString(errCode, errCodeStr);
		}
		else
			NumToString(errCode, errCodeStr);
		textLen += errCodeStr[0];
	}
	errorText = NewHandle(textLen);
	if (!errorText)
		return MemError();
	BlockMoveData(errorStr + 1, *errorText, errorStr[0]);
	if (errCode)
	{
		BlockMoveData(errCodeStr + 1, *errorText + errorStr[0], errCodeStr[0]);
		*(*errorText + errorStr[0] + errCodeStr[0]) = 0x0D;
	}
	else
		*(*errorText + errorStr[0]) = 0x0D;

	err = PeteSetText(APTE, errorText);

	DisposeHandle(errorText);
	return err;
}


OSErr ReportLDAPSearchResults(LDAPSessionHdl ldapSession)

{
	OSErr		err;
	Handle	resultText;


	err = LDAPResultsToText(ldapSession, &resultText, LDAPAttrList, !PrefIsSet(PREF_LDAP_NO_LABEL_TRANS), LDAPEntryFilter);
	if (err)
		return err;

	err = PeteSetText(APTE, resultText);

	DisposeHandle(resultText);
	return err;
}
#endif  //ifndef LDAP_ENABLED


#if 0
/* FINISH *//* REMOVE */
OSErr PrepLDAPLookup(Str255 queryStr, Boolean cmdIsURLQuery, Str255 ldapServer, long *ldapPortNo, Boolean *useRawSearchStr, short *searchScope, Str255 searchBaseObject, LDAPAttrListHdl *attributesList, LDAPAttrListHdl *attrListToKill)

{
#ifdef LDAP_ENABLED

	OSErr							err;
	Str255						urlProtocolStr, urlHostStr;
	Str255						urlQueryStr;
	Str255						searchFilter;
	ProtocolEnum			protocolType;
	DirSvcLookupType	lookupType;
	short							urlPort;
#pragma unused (attrListToKill)


	*useRawSearchStr = (queryStr[0] && (queryStr[1] == '('));
	*searchScope = LDAP_SCOPE_SUBTREE;
	searchBaseObject[0] = 0;
	*attributesList = nil;

	if (!queryStr)
		return noErr;

	if (cmdIsURLQuery)
		BlockMoveData(queryStr, urlQueryStr, queryStr[0] + 1);
	else
	{
		err = ParseURL(queryStr, urlProtocolStr, urlHostStr, urlQueryStr);
		if (err)
			return noErr;
		protocolType = FindSTRNIndex(ProtocolStrn, urlProtocolStr);
		lookupType = ProtocolToDirSvcType(protocolType);
		if (lookupType != ldapLookup)
			return noErr;
	}

	err = ParseLDAPURLQuery(urlQueryStr, searchBaseObject, attributesList, searchScope, searchFilter);
	if (err)
		return err;

	FixURLString(urlHostStr);
	if (ParsePortFromHost(urlHostStr, urlHostStr, &urlPort))
	{
		BlockMoveData(urlHostStr, ldapServer, urlHostStr[0] + 1);
		*ldapPortNo = urlPort;
	}

	BlockMoveData(searchFilter, queryStr, searchFilter[0] + 1);
	*useRawSearchStr = true;
	return noErr;

#else  //ifdef LDAP_ENABLED

#pragma unused (ldapServer, ldapPortNo, queryStr, cmdIsURLQuery, useRawSearchStr, searchScope, searchBaseObject, attributesList)

	return paramErr;

#endif  //ifdef LDAP_ENABLED
}
#endif


OSErr DoLDAPLookup(Str255 ldapServer, int ldapPortNo, Str255 queryStr, Boolean queryStrIsURLQuery)

{
#ifdef LDAP_ENABLED
	OSErr						err, scratchErr;
	short						errStrIndex;
	LDAPSessionHdl	ldapSession;
	Str255					errorStr;
	Str255					searchStr;
	Boolean					useRawSearchStr;
	short						searchScope;
	Str255					searchBaseObject;
	LDAPAttrListHdl	attributesList;
	UPtr forHost = errorStr;


	if (!ldapServer[0] || !queryStr[0])
		return noErr;

	if (!LDAPSupportPresent())
	{
		err = LDAPSupportError();
		errStrIndex = GetLDAPSpecialErrMsgIndex(err);
		if (!errStrIndex)
			errStrIndex = LDAP_NOT_SUPPORTED_MSG;
		else
			err = noErr;
		GetRString(errorStr, errStrIndex);
		scratchErr = OutputLDAPErrorMessage(errorStr, err, false);
		return noErr;
	}

	err = LoadLDAPCode();
	if (err)
	{
		errStrIndex = GetLDAPSpecialErrMsgIndex(err);
		if (!errStrIndex)
			errStrIndex = LDAP_LIB_OPEN_ERR_MSG;
		else
			err = noErr;
		GetRString(errorStr, errStrIndex);
		scratchErr = OutputLDAPErrorMessage(errorStr, err, false);
		return err;
	}

	err = VerifyDirSvcHost(ldapServer, ldapPortNo);
	if (err)
	{
		scratchErr = OutputStrIndexToAnswerField(DIR_SVC_SERVER_RESOLVE_ERR_MSG);
		goto Exit1;
	}

	err = NewLDAPSession(&ldapSession);
	if (err)
	{
		if (err == memFullErr)
		{
			GetRString(errorStr, LDAP_OPEN_MEM_ERR_MSG);
			scratchErr = OutputLDAPErrorMessage(errorStr, noErr, false);
		}
		else
		{
			GetRString(errorStr, LDAP_OPEN_ERR_MSG);
			scratchErr = OutputLDAPErrorMessage(errorStr, err, true);
		}
		goto Exit1;
	}

	err = OpenLDAPSession(ldapSession, ldapServer, ldapPortNo);
	if (err)
	{
		if (err == memFullErr)
		{
			GetRString(errorStr, LDAP_OPEN_MEM_ERR_MSG);
			scratchErr = OutputLDAPErrorMessage(errorStr, noErr, false);
		}
		else
		{
			GetRString(errorStr, LDAP_OPEN_ERR_MSG);
			scratchErr = OutputLDAPErrorMessage(errorStr, err, true);
		}
		goto Exit2;
	}

	if (!GetLDAPServerGlobals(forHost, nil, &searchScope, searchBaseObject, &attributesList))
	{
		scratchErr = OutputStrIndexToAnswerField(DIR_SVC_SERVER_RESOLVE_ERR_MSG);
		err = noErr;
		goto Exit3;
	}
	if (!searchBaseObject[0] && PrefIsSet(PREF_LDAP_REQUIRE_BASE_OBJECT))
	{
		scratchErr = OutputStrIndexToAnswerField(LDAP_SEARCH_REQUIRES_BASE_OBJECT);
		err = noErr;
		goto Exit3;
	}

	if (queryStrIsURLQuery)
	{
		useRawSearchStr = true;
		scratchErr = ParseLDAPURLQuery(queryStr, nil, nil, nil, searchStr); /* ignore errors */
	}
	else
	{
		BlockMoveData(queryStr, searchStr, queryStr[0] + 1);
		useRawSearchStr = (searchStr[0] && (searchStr[1] == '('));
	}

	err = LDAPSearch(ldapSession, searchStr, forHost, useRawSearchStr, searchScope, searchBaseObject, attributesList);
	scratchErr = ReportLDAPSearchResults(ldapSession);
	SFWTC = true;	// we don't trust ldap.
	if (!err)
		err = scratchErr;
	scratchErr = ClearLDAPSearchResults(ldapSession);
	if (!err)
		err = scratchErr;
	if (err)
		goto Exit3;


Exit3:
	scratchErr = CloseLDAPSession(ldapSession);
	if (!err)
		err = scratchErr;
Exit2:
	scratchErr = DisposeLDAPSession(ldapSession);
	if (!err)
		err = scratchErr;
Exit1:
	scratchErr = UnloadLDAPCode();
	if (!err)
		err = scratchErr;
Exit0:
	return err;

#else  //ifdef LDAP_ENABLED

	return paramErr;
#endif  //ifdef LDAP_ENABLED
}


#pragma segment Ph

/************************************************************************
 * PerformQuery - perform a ph query
 ************************************************************************/
void PerformQuery(DirSvcLookupType lookupType,PStr withCommand, Boolean cmdIsURLQuery, Boolean emptyFirst)
{
	WindowPtr	WinWP = GetMyWindowWindowPtr(Win);
	Str255 cmd;
	Str255 ph;
	Handle table=nil;
	short inId,outId;
	OSErr scratchErr;
	Str255 serverHost;
	long serverPort;


	switch (lookupType)
	{
		case phLookup:
		case fingerLookup:
		case ldapLookup:
		case genericLookup:
			break;
		default:
			return;
	}

	if (Offline && GoOnline()) return;

	ZapUserList();

	SwitchDirSvcType(lookupType);

	if (lookupType == genericLookup || lookupType == ldapLookup)
		if (!GetDirSvcServerGlobals(&lookupType, serverHost, &serverPort)) /* FINISH *//* do something different - we don't need host & port info */
		{
			scratchErr = OutputStrIndexToAnswerField(DIR_SVC_SERVER_RESOLVE_ERR_MSG);
			return;
		}

	PhFocus(QPTE);
	if (!withCommand) PeteSelectAll(Win,QPTE);
	switch (lookupType)
	{
		case phLookup:
			MyGetItem(GetMHandle(WINDOW_MENU),WIN_PH_ITEM,ph);
			break;
		case fingerLookup:
			GetRString(ph,FINGER);
			break;
		case ldapLookup:
			GetRString(ph,LDAP_NAME);
			break;
	}

	if (lookupType != phLookup) PhKill();
	
	if (!withCommand && !*PeteSString(cmd,QPTE))
	{
		SetWTitle_(WinWP,ph);
		LockPhGlobals();
		ZapTransStream(&PhStream);
		UnlockPhGlobals();
		return;
	}
	if (withCommand) PCopy(cmd,withCommand);
	else PeteSString(cmd,QPTE);
	PCatC(ph,':');
	PCat(ph,cmd);
	SetWTitle_(WinWP,ph);
	if (emptyFirst) PeteSetTextPtr(APTE,nil,0);
	
	/*
	 * install translation tables
	 */
	if (NewTables)
	{
#ifdef TWO
		 inId = GetPrefLong(PREF_PH_IN);
		 outId = GetPrefLong(PREF_PH_OUT);
#else
		 inId = GetPrefLong(PREF_IN_XLATE);
		 outId = GetPrefLong(PREF_OUT_XLATE);
#endif
	}
	else
	{
		inId = TRANS_IN_TABL;
		outId = TransOutTablID();
	}
	if (inId && (table=GetResource_('taBL',inId)))
	{
	  HNoPurge_(table);
		if (TransIn = NuPtr(256)) BMD(*table,TransIn,256);
		HPurge(table);
	}
	if (outId && (table=GetResource_('taBL',outId)))
	{
		HNoPurge_(table);
		if (TransOut = NuPtr(256)) BMD(*table,TransOut,256);
		HPurge(table);
	}

	/*
	 * do it
	 */
	InitCursor();
	SetMyCursor(watchCursor);
  	
#ifdef CTB
	if (!UseCTB || !DialThePhone(PhStream))
#endif
	{
		if (TransOut) TransLit(cmd+1,*cmd,TransOut);
		switch (lookupType)
		{
			case phLookup:
				scratchErr = ContactQi(PhStream);
				if (scratchErr==noErr)
				{
					if (!UpdateSiteInfo(PhStream) && !SendQiCommand(PhStream,cmd))
						if (EqualStrRes(cmd,PhStrn+phfServerList)) PhListResponse(PhStream,emptyFirst);
						else GetQiResponse(PhStream,cmd);
					UpdateMyWindow(WinWP);
					if (!Live) CloseQi(PhStream);
				}
				else PhListConnectError(scratchErr);	// report Ph connection errors
				break;
			case fingerLookup:
				DoFinger(PhStream, cmd);
				break;
			case ldapLookup:
				scratchErr = DoLDAPLookup(serverHost, serverPort, cmd, cmdIsURLQuery);
				break;
		}
		if (!Live) DestroyTrans(PhStream);
		PeteLock(APTE,0,0x7fffffff,peModLock);
	}
	
#ifdef CTB
  if (UseCTB && !Live) HangUpThePhone();
#endif
	
	/*
	 * clean up tables
	 */
	if (TransIn) {ZapPtr(TransIn);TransIn=nil;}
	if (TransOut) {ZapPtr(TransOut);TransOut=nil;}
	
	/*
	 * set flags
	 */
	Win->hasSelection = MyWinHasSelection(Win);
	PhSetGreys();
	
	inId = PeteIsDirty(QPTE);
	LastDirty = inId;
	QueryTicks = TickCount();
}

/**********************************************************************
 * UpdateSiteInfo - update the siteinfo information from the ph server
 **********************************************************************/
OSErr UpdateSiteInfo(TransStream stream)
{
	Str255 scratch;
	Str63 data;
	AAHandle grumble;
	short code;
	Byte colon[2];
	long size;
	UPtr spot;
	Str31 key;
	
	colon[0] = ':';
	colon[1] = 0;
	
	if (!GetPhServerGlobals(scratch, nil))
		return paramErr;

	/*
	 * fetch the info?
	 */
	if (!SiteInfo || AAFetchResData(SiteInfo,PhStrn+phfServerName,data) || !StringSame(scratch,data))
	{
		ZapHandle(SiteInfo);
		grumble = AANew(32,64);
		SiteInfo = grumble;
		if (!SiteInfo) return(WarnUser(MEM_ERR,MemError()));
		AAAddResItem(SiteInfo,True,PhStrn+phfServerName,scratch);
		
		if (!SendQiCommand(stream,GetRString(scratch,PhStrn+phfSiteInfo)))
		
		code = 600;
		for (size=sizeof(scratch)-3;!RecvLine(stream,scratch+1,&size);size=sizeof(scratch)-3)
		{
			size--;	/* trim newline */
			*scratch = size;
			scratch[size+1] = 0;
			code = Atoi(scratch+1);
			if (code>=200) break;
			if (-300<code && code <= -200)
			{
				spot = scratch+1;
				PToken(scratch,key,&spot,colon);	/* error code */
				PToken(scratch,key,&spot,colon);	/* ordinal */
				PToken(scratch,key,&spot,colon);	/* capability name */
				PToken(scratch,data,&spot,colon);	/* value */
				if (*key && *data && AAAddItem(SiteInfo,True,key,data))
				{
					WarnUser(MEM_ERR,MemError());
					code = 600;
					break;
				}
			}
		}
		if (code>=600)
		{
			ZapHandle(SiteInfo);
			return(fnfErr);
		}
	}
	return(noErr);
}

/**********************************************************************
 * ListServers - list the ph servers
 **********************************************************************/
void PhListServers(void)
{
	Str31 cmd;
	Str255 newServer;
#ifdef LDAP_ENABLED
	OSErr err;
	Handle ldapServerList;
	char cr;
#endif
	Str255 origDirSvcServer;
	DirSvcLookupType origDirSvcType;
	long origDirSvcPort;
	Boolean origDirSvcHostResolved;
	Boolean canDoPhLookup;
	Boolean useFallbackServer;
	Boolean scratch;
	UPtr s = origDirSvcServer;	// notational convenience
	short n;

	// clear it out
	PeteSetTextPtr(APTE,"",0);
	
	// first, list our current server
	if (*GetPref(newServer,PREF_PH))
	{
		GetRString(s,CONFIG_DIR_SERVER);
		PeteAppendText(s+1,*s,APTE);
		ComposeRString(s,RECENT_DIR_FMT,newServer);
		PeteAppendText(s+1,*s,APTE);
		PeteAppendText(Cr+1,*Cr,APTE);
	}
	
	// next, list recent servers
	if (*GetRString(newServer,RecentDirServStrn+1))
	{
		GetRString(s,RECENT_DIR_SERVERS);
		PeteAppendText(s+1,*s,APTE);
		n = RecentDirServStrn+1;
		do
		{
			URLEscape(newServer);
			ComposeRString(s,RECENT_DIR_FMT,newServer);
			PeteAppendText(s+1,*s,APTE);
		}
		while (*GetRString(newServer,++n));
		PeteAppendText(Cr+1,*Cr,APTE);
	}
	
	if (!(MainEvent.modifiers&shiftKey))
	{
		// next, ph servers
		canDoPhLookup = false;
		useFallbackServer = false;
		if (!DirSvcHostResolved)
			scratch = GetDirSvcServerGlobals(nil, nil, nil);
			
		// if ph_server_server set, always use it SD
		if (*GetRString(newServer,PH_SERVER_SERVER))
		{
			canDoPhLookup = true;
			useFallbackServer = true;
		}
		// if current server is ph, use that SD
		else if (DirSvcHostResolved && (DirSvcType == phLookup))
			canDoPhLookup = true;
		// if not, fall back to master server SD
		else if (*GetRString(newServer,MASTER_SERVER_SERVER))
		{
			canDoPhLookup = true;
			useFallbackServer = true;
		}

		if (canDoPhLookup)
		{
			if (useFallbackServer)
			{
				origDirSvcType = DirSvcType;
				PCopy(origDirSvcServer, DirSvcServer);
				origDirSvcPort = DirSvcPort;
				origDirSvcHostResolved = DirSvcHostResolved;
		
				DirSvcType = phLookup;
				PCopy(DirSvcServer, newServer);
				DirSvcPort = GetDefaultDirSvcPort(phLookup);
				DirSvcHostResolved = true;
			}
			PerformQuery(phLookup,GetRString(cmd,PhStrn+phfServerList),false,false);
			if (useFallbackServer)
			{
				DirSvcType = origDirSvcType;
				PCopy(DirSvcServer, origDirSvcServer);
				DirSvcPort = origDirSvcPort;
				DirSvcHostResolved = origDirSvcHostResolved;
		
				RedisplayPhHost();
			}
#ifdef LDAP_ENABLED
			PeteAppendText(LDAPDividerStr + 1, *LDAPDividerStr, APTE);
			cr = 13;
			PeteAppendText(&cr, 1, APTE);
#endif
		}
	}

#ifdef LDAP_ENABLED
	err = GetLDAPServerList(&ldapServerList);
	if (err)
		return;
	err = PeteAppendText(LDRef(ldapServerList), GetHandleSize(ldapServerList), APTE);
	DisposeHandle(ldapServerList);
#endif
}

/**********************************************************************
 * 
 **********************************************************************/
void PhListResponse(TransStream stream,Boolean emptyFirst)
{
	Str255 scratch;
	short code;
	Byte colon[2];
	long size;
	UPtr spot;
	Str255 key, name, server;
	
	colon[0] = ':';
	colon[1] = 0;
	
	if (emptyFirst) PeteSetTextPtr(APTE,nil,0);
	PeteCalcOff(APTE);
		
	code = 600;
	for (size=sizeof(scratch)-3;!RecvLine(stream,scratch+1,&size);size=sizeof(scratch)-3)
	{
		size--;	/* trim newline */
		*scratch = size;
		scratch[size+1] = 0;
		code = Atoi(scratch+1);
		if (-300<code && code <= -200)
		{
			spot = scratch+1;
			PToken(scratch,key,&spot,colon);	/* error code */
			PToken(scratch,key,&spot,colon);	/* ordinal */
			PToken(scratch,key,&spot,colon);	/* field name */
			PToken(scratch,key,&spot,colon);	/* subfield */
			TrimWhite(key);
			TrimInitialWhite(key);
			if (EqualStrRes(key,PhStrn+phfSite))
				MakePStr(name,spot,(scratch+(*scratch+1))-spot);
			else if (EqualStrRes(key,PhStrn+phfServer) && *name)
			{
				MakePStr(server,spot,(scratch+(*scratch+1))-spot);
				PeteAppendText(name+1,*name,APTE);
				ComposeRString(name,URL_FMT,ProtocolStrn+proPh2,server);
				PCatC(name,'\015');
				PeteAppendText(name+1,*name,APTE);
				PeteSetURLRescan(APTE,0);
				*name = 0;
			}
		}
		else
		{
			PCatC(scratch,'\015');
			PeteAppendText(scratch+1,*scratch,APTE);
		}
		if (code>=200) break;
	}
	PeteCalcOn(APTE);
	PhFocus(QPTE);
}

/************************************************************************
 * ContactQi - make contact with the QI server
 ************************************************************************/
short ContactQi(TransStream stream)
{
	Str255 serverName;
	short err;
	long phPort;

	if (Live) return(noErr);

	if (!GetPhServerGlobals(nil, &phPort))
		return paramErr;

	MakeFallbackName(serverName,Fallback);
	err=ConnectTrans(stream,serverName,phPort,!UseCTB,GetRLong(OPEN_TIMEOUT));
	while (!UseCTB&&((err==openFailed)||(err==errLostConnection)))
	{
		DestroyTrans(stream);
		if (isdigit(serverName[1])) {Fallback++;break;}	/* don't fallback on ip addresses. */
		IncrementPhServer(serverName);
		err=ConnectTrans(stream,serverName,phPort,True,GetRLong(OPEN_TIMEOUT));
	}
	if (err&&Fallback)
	{
		Fallback = 0;
		SilenceTrans(stream,False);
		NotePhServer(GetRString(serverName,NOONE));
	}
	else if (Fallback)
		NotePhServer(serverName);
	if (!err && UseCTB) Pause(120L);

	if (!err && PrefIsSet(PREF_PH_LIVE)) Live = True;
	else Live = False;

	return(err);
}

/************************************************************************
 * NotePhServer - tell the user who we're talking to.
 ************************************************************************/
void NotePhServer(UPtr serverName)
{
	Str255 msg;
	ComposeRString(msg,PH_SUCCEED,serverName);
	PeteAppendText(msg+1,*msg,APTE);
}

/************************************************************************
 * IncrementPhServer - go on to the next ph server
 ************************************************************************/
void IncrementPhServer(UPtr serverName)
{
	Str255 fmt,msg,newServer;
	
	Fallback++;
	MakeFallbackName(newServer,Fallback);
	GetRString(fmt,PH_FAIL);
	utl_PlugParams(fmt,msg,serverName,newServer,nil,nil);
	PeteAppendText(msg+1,*msg,APTE);
	PCopy(serverName,newServer);
}

/************************************************************************
 * MakeFallbackName - add the appropriate numeral to a hostname
 ************************************************************************/
void MakeFallbackName(UPtr serverName, short fallback)
{
	char *spot;
	Str15 num;

	if (!GetPhServerGlobals(serverName, nil))
	{
		serverName[0] = 0;
		return;
	}

	if (fallback)
	{
		NumToString(fallback,num);
		spot = strchr(serverName+1,'.');
		if (!spot) spot=serverName+*serverName+1;
		BMD(spot,spot+*num,*serverName-(spot-serverName)+2);
		BMD(num+1,spot,*num);
		*serverName += *num;
	}
}

/**********************************************************************
 * GetFingerServerName - return the name of the finger server
 **********************************************************************/
PStr GetFingerServerName(PStr serverName)
{
	GetPref(serverName,PREF_FINGER_HOST);
	if (!*serverName) GetSMTPInfo(serverName);
	return serverName;
}

/**********************************************************************
 * GetDirSvcServerName - return the name of the server for Ph or LDAP
 **********************************************************************/
PStr GetDirSvcServerName(PStr serverName)
{
	GetPref(serverName, PREF_PH);
	if (!*serverName)
		GetRString(serverName, PH_HOST);
	return(serverName);
}


/************************************************************************
 * SendQiCommand - send the current command to qi
 ************************************************************************/
short SendQiCommand(TransStream stream,UPtr cmd)
{
	Str15 newline;
	
	if (!*GetRString(newline,PH_NEWLINE)) PCopy(newline,NewLine);
	
	return(SendTrans(stream,cmd+1,*cmd,newline+1,*newline,nil));
}

/************************************************************************
 * GetQiResponse - read a response from the nameserver
 ************************************************************************/
short GetQiResponse(TransStream stream,PStr cmd)
{
	Str63 separator;
	Str255 buffer;
	long size;
	short ordinal = 0;
	short lastOrdinal = 0;
	short response=600;
	Boolean tooBig = False;
	Str63 mailfield, maildomain, mailbox;
	Str63 rem1, rem2, rem1Content, rem2Content, key;
	UserEntryHandle ueh;
	long start;
	Str255 name, fContent;
	UPtr spot;
	short nFields = CountStrn(PhNickPhStrn);
	UHandle **fields = nil;
	short fIndex;
	NicknameTagMapRec	tagMap;
	Boolean	haveTagMap;
	Str255 serviceName;
	
	/*
	 * figure out what we need to remember, if anything
	 */
	if (SiteInfo)
	{
		*mailfield = *mailbox = *maildomain = *rem1 = *rem2 = 0;
		AAFetchResData(SiteInfo,PhStrn+phfMaildomain,maildomain);
		AAFetchResData(SiteInfo,PhStrn+phfMailfield,mailfield);
		AAFetchResData(SiteInfo,PhStrn+phfMailbox,mailbox);
		
		if (*maildomain)
		{
			if (*mailfield) PCopy(rem1,mailfield);
			else GetRString(rem1,PhStrn+phfAlias);
			if (*mailbox) PCopy(rem2,mailbox);
			else GetRString(rem2,PhStrn+phfEmail);
		}
		else if (*mailfield)
			PCopy(rem1,mailfield);
		else if (*mailbox)
			PCopy(rem1,mailbox);
		else
			GetRString(rem1,PhStrn+phfEmail);
	}
	
	/*
	 * do it
	 */
	if (haveTagMap = GetNicknameTagMap (GetRString (serviceName, ProtocolStrn + proPh), DirSvcServer, &tagMap) ? false : true)
		nFields = tagMap.count;

	fields = NuHTempOK(nFields*sizeof(UHandle));
	WriteZero(LDRef(fields),nFields*sizeof(UHandle));
	UL(fields);
	
	PeteCalcOff(APTE);
	
	GetRString(separator,PH_SEPARATOR);
	do
	{
		size=sizeof(buffer);
		response = QiLine(stream,&ordinal,buffer,&size);
		if (tooBig) continue;
		if (response==598)
		{
			Str255 query;
			GetRString(query,PH_QUERY);
			PCopy(buffer,cmd);
			PSCat(query,buffer);
			PCatC(query,' ');
			PCatR(query,PH_RETURN);
			//if (TransOut) TransLit(query+1,*query,TransOut);
			response = 0;
			if (SendTrans(stream,query+1,*query,Lf+1,1,nil)) response = 600;
			continue;
		}
		
		/*
		 * new entry.  handle separator and address extraction
		 */
		if (ordinal != lastOrdinal)
		{
			if (!lastOrdinal) PhRememberServer();
			if (lastOrdinal && *rem1 && *rem1Content && (!*rem2 || *rem2Content))
			{
				/* hey, we have to save it! */
				ueh = NewZH(UserEntry);
				if (ueh)
				{
					FormatPhEmail(rem1Content,rem2Content,name,ueh);
					(*ueh)->start = start;
					start = PETEGetTextLen(PETE,APTE);
					(*ueh)->end = start;
					LL_Queue(UserList,ueh,(UserEntryHandle));
					(*ueh)->fields = fields;
					fields = NuHTempOK(nFields*sizeof(UHandle));
					WriteZero(LDRef(fields),nFields*sizeof(UHandle));
					UL(fields);
					PSCopy((*ueh)->name,name);
				}
			}
			*name = *rem1Content = *rem2Content = 0;	/* clear */
			PeteAppendText(separator+1,*separator,APTE);
			start = PETEGetTextLen(PETE,APTE);
			lastOrdinal = ordinal;
		}
		
		if (response < 600)
				(void) PeteAppendText(buffer,size,APTE);
		
		if (-300<response && response<=-200 && (*rem1 || *rem2))
		{
			*buffer = size-1;	/* make into pascal string and trim last char */
			for (spot=buffer+1;spot<buffer+*buffer;spot++) if (*spot==' ') break;
			for (spot++;spot<buffer+*buffer;spot++) if (*spot!=' ') break;
			if (spot<buffer+*buffer)
			{
				if (PToken(buffer,key,&spot,":") && PToken(buffer,fContent,&spot,"\377"))
				{
					if (!*name && EqualStrRes(key,PhStrn+phfName))
						PSCopy(name,fContent);
					if (*rem1 && !*rem1Content && StringSame(key,rem1))
						PSCopy(rem1Content,fContent);
					if (*rem2 && !*rem2Content && StringSame(key,rem2))
						PSCopy(rem2Content,fContent);
					if (fields)
					{
						fIndex = haveTagMap ? FindServiceTagIndex (&tagMap, key) : FindSTRNIndex(PhNickPhStrn,key);
						if (fIndex)
							if (*fContent)
								PhRememberField(fields,fIndex,fContent);
					}
				}
			}
		}
	}
	while(response<200);
	
	PeteCalcOn(APTE);

	ZapFields(fields);
	ZapHandle(fields);
	if (haveTagMap)
		DisposeNicknameTagMap (&tagMap);
	return(response);
}

/**********************************************************************
 * PhRememberField - remember an interesting ph field
 **********************************************************************/
OSErr PhRememberField(UHandle **fields,short i,PStr data)
{
	Handle temp;
	
	/*
	 * 0-indexed, not 1-indexed
	 */
	i--;
	
	/*
	 * fix up the data
	 */
	TrimWhite(data);
	TrimInitialWhite(data);
	
	/*
	 * create the handle?
	 */
	if (!(*fields)[i])
	{
		temp = NuHTempOK(0);
		if (!temp) return(MemError());
		(*fields)[i] = temp;
	}
	
	/*
	 * add an encoded newline
	 */
	if (GetHandleSize((*fields)[i]))
	{
		PtrAndHand("\003",(*fields)[i],1);
		if (MemError()) return(MemError());
	}
	
	/*
	 * add the data
	 */
	PtrAndHand(data+1,(*fields)[i],*data);
	return(MemError());
}

/**********************************************************************
 * FormatPhEmail - format the email address for a ph entry
 **********************************************************************/
void FormatPhEmail(PStr rem1, PStr rem2, PStr name, UserEntryHandle ueh)
{
	Str255 address;
	Str31 fmt;
	Str63 maildomain;
	Str255 mailbox;
	
	TrimWhite(rem1); TrimInitialWhite(rem1);
	TrimWhite(rem2); TrimInitialWhite(rem2);
	TrimWhite(name); TrimInitialWhite(name);
	GetRString(fmt,(*rem2 && !PIndex(rem1,'@'))?PH_ALIAS_FMT:PH_BOX_FMT);
	if (AAFetchResData(SiteInfo,PhStrn+phfMaildomain,maildomain)) *maildomain = 0;
	utl_PlugParams(fmt,mailbox,rem1,maildomain,rem2,nil);
	CanonAddr(address,mailbox,name);
	PCopy((*ueh)->address,address);
}

/************************************************************************
 * CloseQi - close the qi connection
 ************************************************************************/
void CloseQi(TransStream stream)
{
	Str255 buffer;
	long size;
	Str63 quitCmd;
		
	GetRString(quitCmd,PH_QUIT);
	/*
	 * do it
	 */
	if (!CommandPeriod && !SendTrans(stream,quitCmd+1,*quitCmd,Lf+1,1,nil))
	{
		for (size=sizeof(buffer);
				 QiLine(stream,nil,buffer,&size)<200;
				 size=sizeof(buffer))
			;
	}
	
	DisTrans(stream);
}

/************************************************************************
 * QiLine - read a line from QI
 ************************************************************************/
int QiLine(TransStream stream,short *ordinal, UPtr text, long *size)
{
	short result;
	UPtr spot;
	short ord;
	
	if (RecvLine(stream,text,size)) return(600);
	result = Atoi(text);
	if (!result)
	{
		*size = 0;
		return(100);
	}
	for (spot=text;*spot!=':';spot++);
	ord = 0;
	for (spot++;*spot>='0' && *spot <= '9';spot++)
		ord = ord * 10 + *spot - '0';
	if (ord)
	{
		*size -= spot-text+1;
		BMD(spot+1,text,*size);
	}
	if (ordinal) *ordinal = ord;
	if (TransIn) TransLit(text,*size,TransIn);
	return(result);
}

/************************************************************************
 * PhFocus - set the text area
 ************************************************************************/
void PhFocus(PETEHandle pte)
{
	PeteFocus(Win,pte,True);
	Win->ro = pte==APTE;
	if (Win->ro) PhOutline(-1);
	else PhOutline(PETEGetTextLen(PETE,QPTE)?CurDirSvcButton():-1);
}

/************************************************************************
 * PhFixFont - fix the font in the Ph window
 ************************************************************************/
void PhFixFont(void)
{
#ifdef BETA
	BIG UGLY COMPILER ERROR
#endif
}

/************************************************************************
 * NewPhHost - install a new ph host
 ************************************************************************/
void NewPhHost(void)
{
	if (PhGlobals)
	{
		PhKill();
		ResetServerGlobals();
		RedisplayPhHost();
		PeteDelete(APTE,0,0x7fffffff);
	}
}

/**********************************************************************
 * RedisplayPhHost - display a new ph host
 **********************************************************************/
void RedisplayPhHost(void)
{
	Str255 newLabel;
	StringPtr serverNamePtr;
	Str255 serverNameStr;
	Str255 serverTypeStr;

	LockPhGlobals();
	switch (GetTargetDirSvcType())
	{
		case phLookup:
			serverNamePtr = DirSvcServer;
			GetRString(serverTypeStr, PH_NAME);
			break;
		case fingerLookup:
			serverNamePtr = FingerServer;
			GetRString(serverTypeStr, FINGER);
			break;
		case ldapLookup:
			serverNamePtr = DirSvcServer;
			GetRString(serverTypeStr, LDAP_NAME);
			break;
		default:
			serverNamePtr = DirSvcServer;
			serverTypeStr[0] = 0;
			break;
	}
	PCopy(serverNameStr, serverNamePtr);
	FixURLString(serverNameStr);
	if (serverTypeStr[0])
		ComposeRString(newLabel, PH_LABEL, serverTypeStr, serverNameStr);
	else
		ComposeRString(newLabel, PH_LABEL_NO_TYPE, serverNameStr);
	UnlockPhGlobals();
	PCopy(Label,newLabel);
	InvalPhWindowTop();
	UpdateMyWindow(GetMyWindowWindowPtr(Win));
	Fallback = 0;
	PhKill();
}

/************************************************************************
 * DoFinger - do the finger 'protocol'
 ************************************************************************/
void DoFinger(TransStream stream, UPtr cmd)
{
	UPtr atSign = strchr(cmd+1,'@');
	short uLen;
	Str255 buffer;
	long size;
	Boolean gotSome=False;
  
	if (atSign)
	{
		uLen = atSign-cmd-1;
		MakePStr(buffer,atSign+1,*cmd - uLen - 1);
		buffer[*buffer+1] = 0;
	}
	else
	{
		PCopy(buffer,FingerServer);
		uLen = *cmd;
	}
	
	if (!ConnectTrans(stream,buffer,GetRLong(FINGER_PORT),False,GetRLong(OPEN_TIMEOUT)))
	{
		if (!SendTrans(stream,uLen?cmd+1:" ",uLen?uLen:1,CrLf+1,2,nil))
		{
			SilenceTrans(stream,True);
			for (size=sizeof(buffer)-2;!NetRecvLine(stream,buffer,&size);size=sizeof(buffer)-2)
			{
				if (TransIn) TransLit(buffer,size,TransIn);
				PeteAppendText(buffer,size,APTE); SFWTC = True;
			}
		}
		DisTrans(stream);
	}
}

#pragma segment Transport
/************************************************************************
 * RecvLine - get some text from the remote host.
 ************************************************************************/
OSErr NetRecvLine(TransStream stream, UPtr line,long *size)
{
	long count;
	UPtr anchor, end;
	short err;
	static int RcvBufferSize;
	long bSize = *size;
	Byte c;
	
	if (!(stream->RcvBuffer) || !*(stream->RcvBuffer)) return(CommandPeriod ? userCancelled : memFullErr);
	*size = 0;
	if (!RcvBufferSize) RcvBufferSize = GetHandleSize_(stream->RcvBuffer);

	anchor = line;
	end = line+bSize-1;
	while (anchor < end) 										/* need more chars */
		if (stream->RcvSpot>=0) 											/* there are some buffered chars */
		{
			UPtr rPtr = *(stream->RcvBuffer)+stream->RcvSpot;
			for(c=*rPtr++;anchor<end;c=*rPtr++)
				if (c && c!='\015')
				{
					*anchor++ = c;
					if (c=='\012')
					{
						anchor[-1] = '\015';
						break;
					}
				}
			if (c!='\012') rPtr--;									/* whoops.  back up. */
			stream->RcvSpot = rPtr - *(stream->RcvBuffer);
			if (stream->RcvSpot>stream->RcvSize)									/* newline was sentinel */
				anchor--;
			if (stream->RcvSpot>=stream->RcvSize)									/* we have emptied the buffer */
				stream->RcvSpot = -1;
			if (anchor>line && anchor[-1]=='\015' || anchor>=end)	/* found a valid newline, or filled buffer */
			{
				*size = anchor-line;
				*anchor = 0;										/* terminate */
				return(noErr);
			}
		}
		else
		{
			count = RcvBufferSize-1;
			if (err=RecvTrans(stream,LDRef(stream->RcvBuffer),&count)) 	/* get some chars */
				 count=0;
			UL(stream->RcvBuffer);
			if (count)
			{
#ifdef TREAT_BODY_CR_AS_CRLF
				// Turn bare CR into bare LF.  Bare LF will be treated by
				// the rest of the code as CRLF, and bare CR is illegal,
				// though it is sometimes generated by -- you guessed it --
				// Exchange, when CRLF is really meant.
				//
				// We do a sloppy job below, and actually will miss a lone
				// CR that happens to land on a buffer boundary, but ASK ME
				// IF I CARE!!!!
				{
					UPtr cr = *stream->RcvBuffer;
					UPtr end = cr+count-1;
					for (;cr<end;cr++)
						if (cr[0]=='\015' && cr[1]!='\012') cr[0] = '\012';
				}
				// End of disgusting Exchange workaround
#endif //TREAT_BODY_CR_AS_CRLF
				
				(*(stream->RcvBuffer))[count] = '\012'; 											/* sentinel */
				stream->RcvSize = count;
				stream->RcvSpot = 0;			}
			if (err)
			{
				*size = anchor-line;
				line[*size] = 0;
				return(err);
			}
		}
	*size = anchor - line;
	if (*size) line[*size] = 0; 					/* null terminate */
	return(err==userCancelled ? err : (*size ? noErr : err));
}

/**********************************************************************
 * 
 **********************************************************************/
void PhKill(void)
{
	if (PhGlobals && Live)
	{
		CloseQi(PhStream);
		DestroyTrans(PhStream);
#ifdef CTB
		if (UseCTB) HangUpThePhone();
#endif
		Live = False;
	}
}

/************************************************************************
 * PhRememberServer - remember the current ph server
 ************************************************************************/
void PhRememberServer(void)
{
	Str255 url, server;
	short which;
	short n;
	
	if (DirSvcType==phLookup)
		PhMakeURL(url);
	else if (DirSvcType==ldapLookup)
		LDAPMakeURL(url);
	else
		return; // unknown type!
	
	which = FindSTRNIndex(RecentDirServStrn,url);
	
	if (which==1) return;	// already at the top of the list
	
	// move all the rest down
	n = which ? which-1 : CountStrn(RecentDirServStrn);
	for (;n;n--)
		ChangeStrn(RecentDirServStrn,n+1,GetRString(server,RecentDirServStrn+n));
	// put it at the top of the list
	ChangeStrn(RecentDirServStrn,1,url);
	
	// make sure we don't remember more servers than we're supposed to
	ChangeStrn(RecentDirServStrn,GetRLong(RECENT_DIR_LIMIT)+1,"");
}

/************************************************************************
 * PhMakeURL - make a ph URL out of the current server stuff
 ************************************************************************/
PStr PhMakeURL(PStr url)
{
	Str31 portStr;
	
	GetRString(url,ProtocolStrn+proPh);
	PCat(url,"\p://");
	PCat(url,DirSvcServer);
	if (DirSvcPort && DirSvcPort!=GetRLong(PH_PORT))
	{
		PCatC(url,':');
		NumToString((uLong)DirSvcPort,portStr);
		PCat(url,portStr);
	}
	PCatC(url,'/');
	return(url);
}

/************************************************************************
 * LDAPMakeURL - make an LDAP URL out of the current server stuff
 ************************************************************************/
PStr LDAPMakeURL(PStr url)
{
	Str255 s;
	short a;
	UPtr attr;
	
	// ldap://
	GetRString(url,ProtocolStrn+proLDAP);
	PCat(url,"\p://");
	
	// server
	PCat(url,DirSvcServer);
	// port
	if (DirSvcPort && DirSvcPort!=GetRLong(LDAP_PORT_REALLY))
	{
		PCatC(url,':');
		NumToString((uLong)DirSvcPort,s);
		PCat(url,s);
	}
	PCatC(url,'/');
	
	// is there a base object?
	PCat(url,LDAPBaseObject);
	
	// do we have fields?
	if (LDAPAttrList)
	{
		PCatC(url,'?');
		for (a=0;a<(*LDAPAttrList)->numLogicalAttrs;a++)
		{
			attr = ((char**)(*(*LDAPAttrList)->attrList))[a];
			CtoPCpy(s,attr);
			if (a) PCatC(url,',');
			PCat(url,s);
		}
	}
	
	return(url);
}


#pragma segment Balloon
PhWinItemIndex CalcPhHelpItem(MyWindowPtr win, Point mouseLoc, Rect *itemRect)

{
#pragma unused (win)


	if (PtInControl(mouseLoc, OkButton))
	{
		GetControlBounds(OkButton,itemRect);
		return phWinLookupBtnItem;
	}
	if (PtInControl(mouseLoc, FingerButton))
	{
		GetControlBounds(FingerButton,itemRect);
		return phWinFingerBtnItem;
	}
	if (PtInControl(mouseLoc, ListButton))
	{
		GetControlBounds(ListButton,itemRect);
		return phWinGlobeBtnItem;
	}
	if (PtInControl(mouseLoc, ToButton))
	{
		GetControlBounds(ToButton,itemRect);
		return phWinToBtnItem;
	}
	if (PtInControl(mouseLoc, CcButton))
	{
		GetControlBounds(CcButton,itemRect);
		return phWinCcBtnItem;
	}
	if (PtInControl(mouseLoc, BccButton))
	{
		GetControlBounds(BccButton,itemRect);
		return phWinBccBtnItem;
	}
	if (PtInRect(mouseLoc, &QRect))
	{
		*itemRect = QRect;
		return phWinQueryTextItem;
	}
	if (PtInRect(mouseLoc, &ARect))
	{
		*itemRect = ARect;
		return phWinResponseTextItem;
	}

	itemRect->top = itemRect->left = itemRect->bottom = itemRect->right = 0;
	return 0;
}


#pragma segment Balloon
/************************************************************************
 * PhHelp - do balloon help for the ph window
 ************************************************************************/
void PhHelp(MyWindowPtr win,Point mouse)
{
	Rect r;
	short hnum;
	short itemIndex;
	DirSvcLookupType defaultLookupType;

	itemIndex = CalcPhHelpItem(win, mouse, &r);
	if (!itemIndex)
	{
		hnum = 100; /* index value that's off the scale, to indicate it's not a UI item */
		MyBalloon(&r,100,0,PHWIN_HELP_STRN+hnum,0,nil);
		return;
	}

	defaultLookupType = GetTargetDirSvcType();
	switch (defaultLookupType)
	{
		case phLookup:
		case fingerLookup:
		case ldapLookup:
		case genericLookup:
			break;
		default:
			hnum = 100; /* index value that's off the scale, to indicate it's not a UI item */
			MyBalloon(&r,100,0,PHWIN_HELP_STRN+hnum,0,nil);
			return;
	}

	switch (itemIndex)
	{
		case phWinLookupBtnItem:
			switch (defaultLookupType)
			{
				case phLookup:
					hnum = CanQuery() ? 1 : 4;
					break;
				case ldapLookup:
					hnum = CanQuery() ? 2 : 5;
					break;
				case genericLookup:
					hnum = CanQuery() ? 3 : 6;
					break;
				default:
					hnum = 100; /* index value that's off the scale, to indicate it's not a UI item */
					break;
			}
			break;

		case phWinFingerBtnItem:
			if (UseCTB)
				hnum = 9;
			else if (CanQuery())
				hnum = 7;
			else
				hnum = 8;
			break;

		case phWinGlobeBtnItem:
			hnum = 10;
			break;

		case phWinToBtnItem:
			if (defaultLookupType == fingerLookup)
				hnum = 13;
			else if (CanAddress())
				hnum = 11;
			else
				hnum = 12;
			break;

		case phWinCcBtnItem:
			if (defaultLookupType == fingerLookup)
				hnum = 16;
			else if (CanAddress())
				hnum = 14;
			else
				hnum = 15;
			break;

		case phWinBccBtnItem:
			if (defaultLookupType == fingerLookup)
				hnum = 19;
			else if (CanAddress())
				hnum = 17;
			else
				hnum = 18;
			break;

		case phWinQueryTextItem:
			hnum = 19 + defaultLookupType;
			break;

		case phWinResponseTextItem:
			hnum = 23 + defaultLookupType;
			break;

		default:
			hnum = 100; /* index value that's off the scale, to indicate it's not a UI item */
	}

	MyBalloon(&r,100,0,PHWIN_HELP_STRN+hnum,0,nil);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PhListConnectError(short error)
{
	OSErr err = noErr;
	Str255 errorString, e;
	
	if (error != userCancelled)
	{
		Zero(e);
		OTErrorToString(error, e);
		
		ComposeRString(errorString,PH_CONNECT_ERROR,error,e);
		if (PeteLen(APTE)) PeteAppendText(Cr+1,*Cr,APTE);
		PeteAppendText(errorString+1,*errorString,APTE);
	}	
	return (err);
}