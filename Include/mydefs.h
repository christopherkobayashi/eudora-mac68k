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

#ifndef MYDEFS_C
#define MYDEFS_C

#include <Events.h>
#include <AppleEvents.h>
#include <AEDataModel.h>
#include <MacTCP.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * This file contains important things common to all source files
 **********************************************************************/
#include "conf.h"
#if __profile__
#include "Profiler.h"
#endif

/**********************************************************************
 * a pointer into nowhere
 **********************************************************************/
#ifndef nil
#define nil ((long)0)
#endif

/**********************************************************************
 * make dealing with handles a little less unpleasant
 **********************************************************************/
#define LDRef(aHandle)	(HLock((void *)(aHandle)),*(aHandle))
#define UL(aHandle) 		HUnlock((void *)(aHandle))
#define New(aType)		((aType *)NuPtr(sizeof(aType)))
#define NewH(aType) 	((aType **)NuHTempOK(sizeof(aType)))
#define NewZH(aType)	 ((aType **)ZeroHandle(NuHTempOK(sizeof(aType))))
#define NewHTB(aType) 	((aType **)NuHTempBetter(sizeof(aType)))
#define NewZHTB(aType)	 ((aType **)ZeroHandle(NuHTempBetter(sizeof(aType))))
#define NuHandleClear(size)	 (ZeroHandle(NuHandle(size)))
#define ZapHandle(aHandle)	do{if (aHandle){DisposeHandle((void *)(aHandle));(aHandle)=nil;}}while(0)
#define ZapPtr(aPtr)	while(aPtr){DisposePtr((void *)aPtr);aPtr=nil;}
#define HandleCount(h)	((h) ? (*(h) ? GetHandleSize_(h)/sizeof(**h) : 0) : 0)
#define DEC_STATE(h)	char h##_state;
#define L_STATE(h)	do{h##_state=HGetState((Handle)h);HLock((void*)h);}while(0)
#define U_STATE(h)	do{HSetState((Handle)h,h##_state);}while(0)

// (jp) Lots of casting will be evil under Carbon, for now we'll concern ourselves
//                      with Window/Dialog/etc types of casts that won't work in a world without
//                      extended WindowRecords
#ifdef	FLOAT_WIN
#define FrontWindow_ MyFrontNonFloatingWindow
#else				//FLOAT_WIN
#define FrontWindow_ FrontWindow
#endif				//FLOAT_WIN
#define HandToHand_(h)	MyHandToHand((void *)(h))
#define AddResource_(h,t,i,n)	AddResource((void *)(h),(ResType)(t),i,(ConstStr255Param)(n))
#define AddMyResource_(h,t,i,n)	AddMyResource((void *)(h),(ResType)(t),i,(ConstStr255Param)(n))
#define GetResource_(t,i) (void*)GetResource((ResType)t,i)
#define GetIndResource_(t,i) (void *)GetIndResource((ResType)t,i)
#define AEGetParamPtr_(e,k,dt,t,p,m,a) AEGetParamPtr((void *)e,(AEKeyword)k,(DescType)dt,(void*)t,(void *)p,(Size)m,(void*)a)
#define SetPort_(p)	SetPort(p)
#define	GetHandleSize_(h)	InlineGetHandleSize((Handle)h)
#define HNoPurge_(h)	HNoPurge((Handle)h)
#define GetAuxWin_(w,h)	GetAuxWin(w,h)
#define GetNewControl_(i,w)	GetNewControl(i,w)
#define GetNewControlSmall_(i,w)	GetNewControlSmall(i,w)
#define ReleaseResource_(r)	ReleaseResource((Handle)r)
#define PtrPlusHand_(p,h,s) PtrPlusHand((void*)(p),(void*)(h),s)
#define HandPlusHand_(h1,h2) HandPlusHand((void*)(h1),(void*)(h2))
#define SetHandleBig_(h,s) SetHandleBig((Handle)(h),(long)(s))
#define SetWTitle_(w,t)	SetWTitle(w,t)
#define FindWindow_(p,w)	FindWindow(p,w)
#define GetDItem_(d,i,t,h,r) GetDialogItem(d,i,t,(void*)h,r)
#ifdef	FLOAT_WIN
#define SelectWindow_(w) MySelectWindow(w)
#else				//FLOAT_WIN
#define SelectWindow_(w) SelectWindow(w)
#endif				//FLOAT_WIN
#ifdef	FLOAT_WIN
#define HideWindow_(w) MyHideWindow(w)
#else				//FLOAT_WIN
#define HideWindow_(w) HideWindow(w)
#endif				//FLOAT_WIN

#define	DisposeWindow_(aWindowPtr)	MyDisposeWindow(aWindowPtr)
#define	DisposeDialog_(aDialogPtr)	MyDisposeDialog(aDialogPtr)
#define	DisposDialog_(aDialogPtr)		MyDisposeDialog(aDialogPtr)

/************************************************************************
 * Make UPP's less painful
 ************************************************************************/
#if TARGET_RT_MAC_CFM

/*
 * NOTE: We grabbed BUILD_ROUTINE_DESCRIPTOR from MixedMode.h and put it here
 * because we need to call old plug-ins when running under CarbonLib in MacOS
 * classic.  SD 6/11/02
 */

/* A macro which creates a static instance of a non-dispatched routine descriptor */
#define BUILD_ROUTINE_DESCRIPTOR(procInfo, procedure)                               \
    {                                                                               \
        _MixedModeMagic,                            /* Mixed Mode A-Trap */         \
        kRoutineDescriptorVersion,                  /* version */                   \
        kSelectorsAreNotIndexable,                  /* RD Flags - not dispatched */ \
        0,                                          /* reserved 1 */                \
        0,                                          /* reserved 2 */                \
        0,                                          /* selector info */             \
        0,                                          /* number of routines */        \
        {                                           /* It�s an array */             \
            {                                       /* It�s a struct */             \
                (procInfo),                         /* the ProcInfo */              \
                0,                                  /* reserved */                  \
                GetCurrentArchitecture(),           /* ISA and RTA */               \
                kProcDescriptorIsAbsolute |         /* Flags - it�s absolute addr */\
                kFragmentIsPrepared |               /* It�s prepared */             \
                kUseNativeISA,                      /* Always use native ISA */     \
                (ProcPtr)(procedure),               /* the procedure */             \
                0,                                  /* reserved */                  \
                0                                   /* Not dispatched */            \
            }                                                                       \
        }                                                                           \
    }

#endif

/************************************************************************
 * repeat ten times: "I hate the Memory Manager."
 ************************************************************************/
#define OFFSET_RECT(tangle,dh,dv) do {																	\
	Rect r = *(tangle); 																									\
	OffsetRect(&r,dh,dv); 																								\
	*(tangle) = r;																												\
	} while (0)
#define INSET_RECT(tangle,dh,dv) do { 																	\
	Rect r = *(tangle); 																									\
	InsetRect(&r,dh,dv);																									\
	*(tangle) = r;																												\
	} while (0)
#define SET_RECT(tangle,lf,tp,rt,bt) do { 															\
	Rect r = *(tangle); 																									\
	SetRect(&r,lf,tp,rt,bt);																							\
	*(tangle) = r;																												\
	} while (0)
#define INVAL_RECT(tangle) do { 																				\
	Rect r = *(tangle); 																									\
	InvalRect(&r);																												\
	} while (0)
#define VALID_RECT(tangle) do { 																				\
	Rect r = *(tangle); 																									\
	ValidRect(&r);																												\
	} while (0)
#define ERASE_RECT(tangle) do { 																				\
	Rect r = *(tangle); 																									\
	EraseRect(&r);																												\
	} while (0)
#define FRAME_RECT(tangle) do { 																				\
	Rect r = *(tangle); 																									\
	FrameRect(&r);																												\
	} while (0)

/**********************************************************************
 * some #defines that don't seem to belong anywhere else
 **********************************************************************/
#define INSET 6
// (jp) already defined in Universal Headers 3.4
//#define TRUE  true
//#define FALSE false
#define True	true
#define False false
#define IsSpace(c) ((c)=='\t' || (c)==' ' || (c)=='\f' || (c)=='\r' || (c)=='\n')
#define ENVIRONS_VERSION 2	/* the version of SysEnvirons we expect */
#define InFront ((void *)-1)	/* for window creation */
#define BehindModal (ModalWindow?ModalWindow:InFront)	/* for window creation */
#define REAL_BIG 32766		/* REAL_BIG, more or less */
#define TABKEY 9
#define CANCEL_ITEM 		(-1)
#define MIN(x,y)				(((x) < (y)) ? (x) : (y))
#define MAX(x,y)				(((x) < (y)) ? (y) : (x))
#define ABS(x)					((x)<0 ? -(x) : (x))
#define GROW_SIZE 15
#define MAX_DEPTH 12		/* max depth for alias tree */
#define fInited (1<<8)		/* why doesn't apple define this? */
#define userCancelled 	-29999	/* user cancelled tcp operations */
#define IsWhite(c)			(c==' ' || c=='\t')
#define IsLWSP(c)				(c==' ' || c=='\t' || c=='\r')
#define IsAnySP(c)				(c==optSpace || c==' ' || c=='\t' || c=='\r')
#define K *1024
#define MAX_ALIAS 200
#define SAVE_PORT				GrafPtr oldPort = GetQDGlobalsThePort()
#define REST_PORT				SetPort_(oldPort)
#ifdef DEBUG
#define CHECKPOINT do{SpinSpot = __LINE__;}while(0)
#define DBLINE do{Str255 s;DebugStr(ComposeString(s,"\p%s %d;hc;sc;g",__FILE__,__LINE__));}while(0)
#define ASSERT(expr) do{if (!(expr) && RunType!=Production) {Str255 s;DebugStr(ComposeString(s,"\passertion failed: " #expr ", %s:%d--type 'g' and return to continue",__FILE__,__LINE__));}}while(0)
#define VERIFY(expr) do{if (!(expr) && RunType!=Production) {Str255 s;DebugStr(ComposeString(s,"\passertion failed: " #expr ", %s:%d--type 'g' and return to continue",__FILE__,__LINE__));}}while(0)
#else
#define CHECKPOINT
#define DBLINE
#define ASSERT(expr)
#define VERIFY(expr) expr
#endif
#define MEM_CRITICAL	(SPARE_SIZE/4)
#ifdef DEBUG
#define LOGLINE ComposeLogS(-1,nil,"\p%r:%d",FNAME_STRN+FILE_NUM,__LINE__)
#else
#define LOGLINE
#endif

#define A822_FLAVOR	'a822'

enum { kMyIntl0 = 1000 };

#ifdef EXP_YEAR
#define CHECK_EXPIRE do																					\
	{																												\
		DateTimeRec dtr;																			\
		GetTime(&dtr);																				\
		if (dtr.year>EXP_YEAR || dtr.year == EXP_YEAR && dtr.month > EXP_MONTH)	\
		{\
			if (ComposeStdAlert(kAlertStopAlert,BETA_EXPIRED)==kAlertStdAlertOKButton) \
				OpenAdwareURL (GetNagState(), UPDATE_SITE, actionUpdate, updateQuery, nil);	\
			EjectBuckaroo = true;	\
		}\
	} while (0)
#else
#define CHECK_EXPIRE
#endif

#ifdef	DEMO
Boolean DemoExpired(void);
#ifndef LIGHT
#define	CHECK_DEMO DemoExpired()
#else				//LIGHT
#define	CHECK_DEMO false
#endif				//LIGHT
#else
#define	CHECK_DEMO
#endif

#define TICKS2MINS 3600

typedef struct TransStreamStruct *TransStream;

/**********************************************************************
 * 
 **********************************************************************/
#ifdef DEBUG
#define DebugStr0 if (BUG0) DebugStr
#define DebugStr1 if (BUG1) DebugStr
#define DebugStr2 if (BUG2) DebugStr
#define DebugStr3 if (BUG3) DebugStr
#define DebugStr4 if (BUG4) DebugStr
#define DebugStr5 if (BUG5) DebugStr
#define DebugStr6 if (BUG6) DebugStr
#define DebugStr7 if (BUG7) DebugStr
#define DebugStr8 if (BUG8) DebugStr
#define DebugStr9 if (BUG9) DebugStr
#define DebugStr10 if (BUG10) DebugStr
#define DebugStr11 if (BUG11) DebugStr
#define DebugStr12 if (BUG12) DebugStr
#define DebugStr13 if (BUG13) DebugStr
#define DebugStr14 if (BUG14) DebugStr
#define DebugStr15 if (BUG15) DebugStr
#else
#define DebugStr0(x)
#define DebugStr1(x)
#define DebugStr2(x)
#define DebugStr3(x)
#define DebugStr4(x)
#define DebugStr5(x)
#define DebugStr6(x)
#define DebugStr7(x)
#define DebugStr8(x)
#define DebugStr9(x)
#define DebugStr10(x)
#define DebugStr11(x)
#define DebugStr12(x)
#define DebugStr13(x)
#define DebugStr14(x)
#define DebugStr15(x)
#endif

/**********************************************************************
 * some handy types
 **********************************************************************/
typedef enum { Single, Double, Triple } ClickEnum;
typedef enum {
	OUR_WIN = 0x20,		// The first window kind we recognize as our own
	MBOX_WIN = OUR_WIN,
	CBOX_WIN,
	COMP_WIN,
	TEXT_WIN,
	MESS_WIN,
	FIND_WIN,
	ALIAS_WIN,
	PH_WIN,
	MB_WIN,
	PROG_WIN,
	FILT_WIN,
	TBAR_WIN,
	PICT_WIN,
	PREF_WIN,
	ETL_ABOUT_WIN,
	PERS_WIN,
	SIG_WIN,
	STA_WIN,
	TASKS_WIN,
	LOG_WIN,
	SEARCH_WIN,
	AD_WIN,
	LINK_WIN,
	PAY_WIN,
	HELP_WIN,
	STAT_WIN,
	IMPORTER_WIN,
	TOOLBAR_POPUP_WIN,
	DRAWER_WIN,
	LIMIT_WIN
} WKindEnum;

#define CONFIG_KIND(k)\
	(k==PH_WIN || k==TBAR_WIN || k==FILT_WIN || k==PREF_WIN || k==FIND_WIN || k==MB_WIN || k==ALIAS_WIN || k==PERS_WIN || k==SIG_WIN || k==STA_WIN || k==LINK_WIN || k==PAY_WIN)
typedef unsigned char *UPtr, **UHandle, *PStr, *CStr;
#define Uptr UPtr
#define Uhandle UHandle
typedef unsigned short uShort;
typedef unsigned long uLong;
typedef enum { Production, Debugging, Steve } RunTypeEnum;
typedef unsigned char Str127[128];
typedef EventRecord *EventPtr;
typedef AEDesc *AEDescPtr;
typedef AEAddressDesc *AEAddressDescPtr;
typedef AppleEvent *AppleEventPtr;
typedef struct MIMEMapStruct **MIMEMapHandle;
typedef enum {
	plComplete,
	plPartial,
	plEndOfMessage,
	plError
} POPLineType;
typedef POPLineType LineReader(TransStream stream, UPtr buf, long bSize,
			       long *len);
typedef UHandle TextAddrHandle;
typedef UPtr TextAddrPtr;
typedef UHandle BinAddrHandle;
typedef UPtr BinAddrPtr;
typedef UHandle NickHandle;
typedef UPtr NickPtr;
typedef UHandle TabFieldHandle;
typedef UPtr TabFieldPtr;
#endif

#ifndef MYDEFS_C_2
#define MYDEFS_C_2
typedef struct {
	short flags;
	uLong prefSize;
	uLong minSize;
} SizeRec, *SizePtr, **SizeHandle;

/************************************************************************
 * transport mechanisms
 ************************************************************************/
typedef struct {
	OSErr(*vConnectTrans) (TransStream stream, UPtr serverName,
			       long port, Boolean silently, uLong timeout);
	OSErr(*vSendTrans) (TransStream stream, UPtr text, long size, ...);
	 OSErr(*vRecvTrans) (TransStream stream, UPtr line, long *size);
	 OSErr(*vDisTrans) (TransStream stream);
	 OSErr(*vDestroyTrans) (TransStream stream);
	 OSErr(*vTransError) (TransStream stream);
	void (*vSilenceTrans)(TransStream stream, Boolean silence);
	 OSErr(*vSendWDS) (TransStream stream, wdsEntry * theWDS);
	unsigned char *(*vWhoAmI)(TransStream stream, Uptr who);
	 OSErr(*vRecvLine) (TransStream stream, UPtr line, long *size);
	 OSErr(*vAsyncSendTrans) (TransStream stream, UPtr buffer,
				  long size);
} TransVector;

OSErr ConnectTrans(TransStream stream, UPtr serverName, long port,
		   Boolean silently, uLong timeout);

#define ConnectTransLo (*CurTrans.vConnectTrans)
#define SendTrans (*CurTrans.vSendTrans)
#define RecvTrans (*CurTrans.vRecvTrans)
#define DisTrans (*CurTrans.vDisTrans)
#define DestroyTrans (*CurTrans.vDestroyTrans)
#define TransError (*CurTrans.vTransError)
#define SilenceTrans (*CurTrans.vSilenceTrans)
#define SendWDS (*CurTrans.vSendWDS)
#define WhoAmI (*CurTrans.vWhoAmI)
#define RecvLine (*CurTrans.vRecvLine)
#define AsyncSendTrans (*CurTrans.vAsyncSendTrans)
// #endif

#ifdef CK
#include "KClient.h"
#include "KClientCompat.h"
#include "KClientDeprecated.h"
#endif
#if TARGET_RT_MAC_CFM
#include "GSS.h"		// !!! Marshall sez - not yet for MachO
#endif
#include "regexp.h"
#include "OpenSSL.h"		// needed for TransStream
#include "trans.h"
#include "color.h"
#include "macslip.h"
#include "euErrors.h"
#include "lineio.h"
// #include "AEUtil.h" CK
#include "icon.h"
#include "prefdefs.h"
#include "StrnDefs.h"
#include "nickae.h"
#ifdef WINTERTREE
//#include "ssce.h" CK
#endif
#include "spell.h"
#ifndef ONE
#include "pgpin.h"
#endif
#ifdef SPEECH_ENABLED
#include "speechutil.h"
#endif
#include "wrappers.h"
#include "util.h"
#include "anal.h"
#include "pete.h"
#include "menusharing.h"
#include "lex822.h"
#include "header.h"
#include "numcode.h"
#include "MyRes.h"
#include "features.h"
#include "featureldef.h"
#include "downloadurl.h"
#include "nag.h"
#include "StringUtil.h"
#include "StringDefs.h"
// #include "adutil.h" CK
#include "appleevent.h"
#include "cursor.h"
#include "mime.h"
#include "threading.h"
#include "progress.h"
#include "taskProgress.h"
#include "mywindow.h"
#include "peteglue.h"
#include "modeless.h"
#include "ends.h"
#include "functions.h"
#include "appcdef.h"
#include "inet.h"
#include "mailbox.h"
#include "toc.h"
#include "boxact.h"
#include "main.h"
#include "message.h"
#include "messact.h"
#include "navUtils.h"
#include "comp.h"
#include "compact.h"
#include "concentrator.h"
#include "multi.h"
#include "fmtbar.h"
#include "register.h"
#include "search.h"
#include "proxy.h"
#include "schizo.h"
#include "sendmail.h"
#include "filegraphic.h"
#include "pop.h"
#include "tcp.h"
#include "ctb.h"
#include "menu.h"
#include "shame.h"
#include "sort.h"
#include "tefuncs.h"
#include "fileutil.h"
#include "winutil.h"
#include "tabmania.h"
#include "labelfield.h"
#ifdef VCARD
#include "vcard.h"
#endif
#include "nickwin.h"
#include "nickexp.h"
// #include "paywin.h" CK
#include "peteuserpane.h"
#include "oops.h"
#include "listview.h"
#include "find.h"
#include "searchwin.h"
#include "text.h"
#include "address.h"
#include "print.h"
#include "nickmng.h"
#include "binhex.h"
#include "hexbin.h"
#include "ph.h"
#include "utl.h"
#include "prefs.h"
#include "mbwin.h"
#include "buildtoc.h"
#include "squish.h"
#include "uudecode.h"
#include "uupc.h"
#include "md5.h"
#include "lmgr.h"
#include "log.h"
#include "FiltDefs.h"
#include "filters.h"
#include "filtmng.h"
#include "filtwin.h"
#include "filtrun.h"
#include "link.h"
#include "rich.h"
#include "mstore.h"
#include "msmaildb.h"
#include "msiddb.h"
#include "mstoc.h"
#include "msinfo.h"
#ifndef ONE
#include "pgpout.h"
#endif
#include "url.h"
#include "mailxfer.h"
#include "adwin.h"
#include "toolbar.h"
#include "toolbarpopup.h"
#include "html.h"
#include "TransStream.h"
#include "listcdef.h"
#include "wazoo.h"
#include "personalitieswin.h"
#include "signaturewin.h"
#include "stationerywin.h"
#include "floatingwin.h"
#include "stickypopup.h"
#include "appear_util.h"
#include "makefilter.h"
#include "table.h"
#include "acap.h"
#include "ldaputils.h"
#include "filtthread.h"
#include "mail.h"
#include "env.h"
#include "fs.h"
#include "misc.h"
#include "imapnetlib.h"
#include "imapmailboxes.h"
#include "imapdownload.h"
#include "imapconnections.h"
#include "imapauth.h"
#include "NetworkSetup.h"
// #include "MoreNetworkSetup.h" CK
#include "networksetuplibrary.h"
#include "linkwin.h"
#include "linkmng.h"
#include "dial.h"
#include "import.h"
#include "export.h"

#include "spool.h"
#include "unicode.h"

#include "audit.h"
#include "auditdefs.h"

#include "graph.h"
#include "statmng.h"
#include "statwin.h"
#include "xml.h"
#include "scriptmenu.h"
#include "carbonutil.h"
#include "fileview.h"
#include "palmconduitae.h"
#include "junk.h"

#include "sasl.h"
#include "mbdrawer.h"
#include "emoticon.h"

#include "osxabsync.h"


//      SSLCerts.h includes SSL.h, which requires a bunch of #defines
//      before you include it.
//      in SSLCerts.h
Boolean CanDoSSL(void);

#ifdef	DEMO
#include "timebomb.h"
#endif

// #include "regcode_eudora.h" CK

#include "light.h"

#include "Globals.h"

void DebugSomething(void);

// CK stuff from here
static void MoreAssetQ(void)
{
	/* NOOP for now */
}

#endif				// MYDEFS_C
