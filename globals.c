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

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Main
/**********************************************************************
 * Global variables for POP mail
 **********************************************************************/
int FontID; 						/* font parameters */
int FontSize;
int FixedID; 						/* fixed font parameters */
int FixedSize;
int FontWidth;					/* width of average char in font */
int FontLead;
int FontDescent;
int FontAscent;
Boolean Done; 					/* set to True when we're done processing */
Boolean DontTranslate;	/* don't use translation tables */
Boolean FontIsFixed;		/* is font fixed-width? */
short InBG; 						/* whether or not we're in the background */
Boolean NoSaves;				/* don't prompt user to save documents */
Boolean SFWTC;					/* Somebody Fooled With The Cursor */
Boolean ScrapFull;			/* is there stuff on the scrap? */
Boolean UseCTB; 				/* are we using the CTB? */
Boolean Offline; 				/* are we Offline (don't send or check)? */
Boolean Stationery; 		/* Stationery on Save As? */
Boolean AmQuitting; 		/* are we quitting? */
Boolean WrapWrong;			/* wrap the wrong way */
Boolean HasHelp;				/* do we have balloon help? */
Boolean HasPM;					/* Do we have the process manager? */
Boolean NewTables;			/* using new translation table scheme */
Boolean LockCode;				/* lock all code resources? */
Boolean CantLock;				/* the above failed */
Boolean Toshiba;				/* toshiba hard drive? */
Boolean LooseTrans;			/* is there a translator with loose morals? */
Boolean QTMoviesInited;	/* has QuickTime been initialized for playing movies? */
BoxCountHandle BoxCount;/* list of mailboxes for find */
XfUndoHandle XfUndoH;		/* for undoing transfers */
GrafPtr InsurancePort;	/* a port for use when no others are available */
#if TARGET_RT_MAC_CFM
ICMPReport ICMPMessage; /* the last such report */
#endif
MessHandle MessList;		/* list of open messages */
MyWindow *HandyMyWindow;/* spare private window record */
RgnHandle MousePen; 		/* a pen for the mouse */
Boolean TBTurnedOnBalloons;	/* the toolbar forced balloon help on */
Byte NewLine[4];				/* current newline string */
Byte CheckOnIdle;				/* check next time we're idle? */
Str15 Type2SelString;		/* current type-2-select string */
uLong Type2SelTicks;		/* ticks for last key for type-2-select */
short DragSumNum;				/* sumNum for finishing a drag */
TOCHandle DragTOCFrom;	/* window drag came from */
TOCHandle DragTOCTo;		/* window drag went into */
short DragModsWere;			/* what the flags were */
uLong DragSequence;			// sequence number for drag
short StationVRef;			/* vref for stationery folder */
long StationDirId;			/* dirid for stationery folder */
void *PETE;							/* magic cookie for pete's editor */
#ifdef DEBUG
long ____RandomFailThresh;	// random memory failure threshhold.  set this to nonzero at your peril
#endif
#define kCheck 0x02
#define kSend 0x01
TOCHandle TOCList;			/* list of open TOC's */
#ifndef NICKATNIGHT
struct AliasDStruct **Aliases;		/* list of alias files */
#endif
UHandle eSignature;			/* signature text */
UHandle RichSignature;	/* richtext signature */
UHandle HTMLSignature;	/* richtext signature */
Boolean SigStyled;			/* was the signature per se styled? */
UPtr TransIn; 					/* Translation table for incoming characters */
UPtr TransOut;					/* Translation table for outgoing characters */
UPtr Flatten;						/* Translation table to flatten smart quotes */
int CTBTimeout; 				/* current timeout for use with CTB */
HostInfoQHandle HIQ;		/* queue of pending DNS lookups */
Boolean NoPreflight;
Boolean NoNewMailMe;		// put up no new mail alert, please
short Dragging;					/* are we in the drag manager? */
PETEStyleListHandle Pslh;
int SendQueue;					/* # of messages waiting to be sent */
uLong ForceSend;				/* next delayed queue */
short **StdSizes;				// standard font sizes
short **FixedSizes;			// standard fixed-width font sizes
struct BoxMapStruct **BoxMap;	/* map of menu id's to dirId's */
short **BoxWidths; 			/* where the lines go in a mailbox window */
short AliasRefCount;		/* how many subsystems are using aliases? */
short ICMPAvail;				/* there is an ICMP report available */
short RunType;					/* Production, Debugging, or Steve */
struct NMRec *MyNMRec;	/* notification record */
MenuHandle CheckedMenu; /* currently checked mailbox menu */
short CheckedItem;			/* currently checked mailbox item */
Handle SpareSpace[NSpare];		/* extra memory for emergencies */
Boolean DoMonitor;			// run the grow-zone monitor
Boolean EjectBuckaroo;	/* out of memory; die at next opportunity */
Boolean GrowZoning;			/* we are executing the grow zone proc */
unsigned short WhyTCPTerminated;				/* why the ctp connection died */
EventRecord MainEvent;	/* the event currently being processed */
Boolean NoInitialCheck;	/* don't check at startup */
long YieldTicks;				/* last CPU yield */
Boolean HesOK;					// Is the Hesiod info OK, or should we refetch?
uLong GlobalIdleTicks = 90;	// Idle ticks for whole machine
Boolean NoAnalDictionary;	// we didn't find a moodmail dictionary
Boolean WashMe;					// we have dirt on our windows
#ifdef CTB
short CTBHasChars;
#endif
WindowPtr ModalWindow;	/* run modally for this window */
#ifdef CTB
ConnHandle CnH;
#endif
VDId Root;							// Root of the Eudora Folder
VDId MailRoot;					// Root of the Mail Folder within the Eudora Folder
VDId IMAPMailRoot;				// Root of the world's problems
VDId ItemsFolder;				// Items folder
Style UnreadStyle;			/* style for a mailbox with unread items in it */
short LogRefN;					/* ref number of open log file */
long LogLevel;					/* current logging level */
long LogTicks;					/* time log was opened */
Accumulator AuditAccu;	// accumulator for audit data
FMBHandle FMBMain;			// Face measurement block for entire app run
short AppResFile;				/* application resource file */
short HelpResFile;				/* help resource file */
Handle Filters;					/* filter rules */
short FiltersRefCount;	/* reference count for same */
Handle PreFilters;					/* filter rules generated externally (most likely by plug-in), run before normal filters */
Handle PostFilters;					/* filter rules generated externally (most likely by plug-in), run after normal filters */
ProcessSerialNumber **WordServices;
short OriginalHelpCount;	/* # of items in the help menu */
short EndHelpCount;				/* # of items after we got done with it */
Str255 IsWordChar;				/* are these things words? */
short PrefPlugEnd;			/* last Plug-In file from Prefs folder, or app itself */ 
long TypeToOpen;
WindowPtr UglyHackFrontWindow;	// used for easy-open
Str127 MyHostname;			/* our hostname, if we know it */
TOCHandle DamagedTOC;
Boolean ThereIsColor;
Boolean NoDominant;	// don't inherit setting from dominant, just this once
ICacheHandle ICache;
Boolean VM;
Boolean BreakMe;
uLong Yesterday;				/* seconds yesterday */
Boolean MemCanFail;			/* is it ok for this request to fail? */
short FakeTabs;		/* cache this pref for performance reasons */
Handle WrapHandle;
short ClickType;	/* single, double, or triple */
short Windex;			/* window index */
short SysRefN;				/* system file reference number */
Boolean StartingUp;
Boolean SyncRW;
Boolean AutoDoubler;
Boolean PrefsPlugIns;
Boolean TBarHasChanged;	// we changed something that affects the toolbar
Boolean D3;	/* 3d look? */
Boolean Typing;	// are we typing?
Boolean TypingRecently;	// have we typed recently?
short PlaylistNagCount;	// how many nags does the playlist manager want us to give?
short NewClientModePlusOne;	// the client mode the playlist server wants us in, plus one
long TypingTicks;	// last tickcount we noticed a keystroke
long ActiveTicks;	// last time user hit key or mouse
long NonNullTicks;	// last time we got an even other than a null event
Boolean OpenedMacSLIP;
Str15 Re;
Str15 Fwd;
Str15 OFwd;
BoxLinesEnum TOCInversionMatrix[2][BoxLinesLimit];	// order of columns in a table of contents
Boolean DragFxxkOff;	/* tell the drag manager where to put it */
Boolean Sensitive;	/* case-sensitive? */
Boolean WholeWord;	/* whole words? */
Boolean FurrinSort;	/* do we want to watch your for furriners? */
MyWindowPtr DragSource;
MyWindowPtr DragTOCSource;
short	DragSourceKind;
Boolean EmoTurdCache;
Boolean AttentionNeeded;
Byte YesStr[2] = {1,'y'};
Byte NoStr[2] = {1,'n'};
Byte Slash[3] = {1,'/',0};
Byte Cr[2] = {1,'\015'};
Byte Lf[2] = {1,'\012'};
Byte CrLf[3] = {2,'\015','\012'};
Boolean OTIs;
Boolean OptiMEMIs;
Boolean CheckNow;
long StupidTagForACAPandI4;
uLong gCheckSessionID = 0;	/* session ID of mailcheck for auditing purposes */
#ifdef PERF
TP2PerfGlobals ThePGlobals;
#endif
#ifdef DEBUG
short BugFlags;
#define BUG0 ((BugFlags&(1<<0))!=0)
#define BUG1 ((BugFlags&(1<<1))!=0)
#define BUG2 ((BugFlags&(1<<2))!=0)
#define BUG3 ((BugFlags&(1<<3))!=0)
#define BUG4 ((BugFlags&(1<<4))!=0)
#define BUG5 ((BugFlags&(1<<5))!=0)
#define BUG6 ((BugFlags&(1<<6))!=0)
#define BUG7 ((BugFlags&(1<<7))!=0)
#define BUG8 ((BugFlags&(1<<8))!=0)
#define BUG9 ((BugFlags&(1<<9))!=0)
#define BUG10 ((BugFlags&(1<<10))!=0)
#define BUG11 ((BugFlags&(1<<11))!=0)
#define BUG12 ((BugFlags&(1<<12))!=0)
#define BUG13 ((BugFlags&(1<<13))!=0)
#define BUG14 ((BugFlags&(1<<14))!=0)
#define BUG15 ((BugFlags&(1<<15))!=0)
#endif
long SpinSpot;
MyWindowPtr InsertWin;
FSSpec AttFolderSpec;
#ifdef CTB
TransVector CTBTrans = {CTBConnectTrans,CTBSendTrans,CTBRecvTrans,CTBDisTrans,CTBDestroyTrans,CTBTransError,CTBSilenceTrans,GenSendWDS,CTBWhoAmI,NetRecvLine,nil};
#endif
TransVector UUPCTrans = {nil,UUPCSendTrans,nil,UUPCDry,UUPCDry,nil,nil,GenSendWDS,nil,UUPCRecvLine,nil};
TransVector OTTCPTrans = {OTTCPConnectTrans,OTTCPSendTrans,OTTCPRecvTrans,OTTCPDisTrans,OTTCPDestroyTrans,OTTCPTransError,OTTCPSilenceTrans,nil,OTTCPWhoAmI,NetRecvLine,nil/*OTTCPAsyncSendTrans*/};
MyOTTCPStreamHandle pendingCloses = 0;
Boolean	gUseOT = false;
Boolean	gHasOTPPP = false;
Boolean gPPPConnectFailed = false;
FSSpec TCPprefFileSpec;
FSSpec PPPprefFileSpec;
Boolean gMissingNSLib = false;
#ifdef SPEECH_ENABLED
FSSpec SpeechPrefFileSpec;
#endif
#ifdef ADWARE
FSSpec SettingsFileSpec;
#endif
long gActiveConnections = 0;
Boolean gConnecting = false;
Boolean gStayConnected = false;
#ifdef TWO
TransVector PGPTrans = {nil,nil,nil,nil,nil,nil,nil,nil,nil,PGPRecvLine,nil};
TransVector TransTrans = {nil,nil,nil,nil,nil,nil,nil,nil,nil,TransRecvLine,nil};
StackHandle TransContextStack;
StackHandle MBRenameStack;
long MemLastFailed;
long LastTotalSpace, LastContigSpace;
Boolean EmptyRecip;
short NoScannerResets;
Boolean DirtyHackForChooseMailbox;
Boolean OpenAddrErrs;
#ifdef WINTERTREE
short SpellSession = -1;
uLong WinterTreeOptions;
#endif
#endif
#ifdef USECMM
Boolean gHasCMM;
#endif
Boolean	gHave85MenuMgr;

/* -------------------- Appearance Manager Globals -------------------- */
Boolean gAppearanceIsLoaded = false;		// AM is loaded properly
Boolean gUseAppearance;						// Register Eudora as Appearance Client
Boolean gGoodAppearance;					// Appearance 1.0.1 or later
Boolean gBetterAppearance;					// Appearance 1.1 or later
Boolean gBestAppearance;					// Appearance 1.1.4 or later
SInt32	gLastCtlValue;						// used for live scrolling
Boolean gUseLiveScroll = false;		// preference for live scrolling
/* -------------------------------------------------------------------- */
Boolean gAXIsSupported = false;					// Active X global
Rect gAXLocation;
#ifdef	FLOAT_WIN
WindowPtr lastHilitedWinWP;		// keep track of the last window to be hilighted.
MyWindowPtr keyFocusedFloater;	// this floater wants to intercept key presses.
#endif	//FLOAT_WIN
Boolean VicomIs = false;
long VicomFactor = 0;
Boolean NoMenus = 1;	// disable menus?
Boolean PleaseQuit;	// toolbar signals it would like a quit
Boolean	g16bitSubMenuIDs;	//	Are 16-bit hierarchical menu ID's supported?
short		gMaxBoxLevels;	//	Maximum number of mailbox folders in menus
IMAPConnectionHandle gIMAPConnectionPool = nil;
Str255 gIMAPErrorString;
Boolean gbDisplayIMAPWarnings = false;
											//f1	f2		f3		f4	f5	f6		f7		f8	f9	f10		f11	f12		f13	f14		f15
Byte FunctionKeys[] = {0x7a,0x78,0x63,0x76,0x60,0x61,0x62,0x64,0x65,0x6d,0x67,0x6f,0x69,0x6b,0x71};

/************************************************************************
 * mimestore declarations
 ************************************************************************/
MStoreSubFile MSSubs[] = {{0,MAILDB_FILE,kMStoreMailFileType,MSMailDBFunc},
													{0,IDDB_FILE,kMStoreIDDBFileType,MSIDDBFunc},
													{0,TOC_FILE,kMStoreTOCFileType,MSTOCFunc},
													{0,INFO_FILE,kMStoreInfoFileType,MSInfoFunc}};

/**********************************************************************
 * a few temp vars for macros
 **********************************************************************/
uLong M_T1, M_T2, M_T3;

/**********************************************************************
 * thread globals - defined in threading.h
 **********************************************************************/

threadGlobalsRec ThreadGlobals;
threadGlobalsPtr CurThreadGlobals = &ThreadGlobals;

#ifdef THREADING_ON
/**********************************************************************
 * thread sets this to true when there are messages to filter
 **********************************************************************/
short TempInCount;
short NeedToFilterIn;
short NeedToFilterOut;
short NeedToNotify;
short NeedToFilterIMAP;
Boolean NoXfer = false;

/* When send fails while other threads running (low mem), set this flag to true */
Boolean SendImmediately = false;
Boolean CheckThreadRunning = false;
Boolean SendThreadRunning = false;
threadDataHandle gThreadData = nil;

short IMAPCheckThreadRunning = 0;		/* number of IMAP Check Mail threads currently running */
short gNewMessages = 0;					/* number of messages requiring notification in the entire mailcheck */
Boolean gSkipIMAPBoxes;					/* Set to true when we want to skip IMAP mailboxes during filtering */
Boolean gWasManualIMAPCheck = false;	/* set to true when a manual IMAP check happens */

#ifdef TASK_PROGRESS_ON
uLong LastCheckTime = 0;
Boolean TaskDontAutoClose = true;
#endif

MyWindowPtr TaskProgressWindow = nil;
Boolean gTaskProgressInitied = false;
long ThreadYieldTicks;				/* last thread yield */
OSErr CheckThreadError = noErr;
OSErr SendThreadError = noErr;
Boolean DFWTC = false;	/* Don't fool with the cursor */
#endif

int TotalQueuedSize = 0;
Str255 P1,P2,P3,P4;
Boolean NewError = false;
long BgYieldInterval = 0; 
long FgYieldInterval = 0;

long GroupSubjThreshTime;

#ifdef NAG
NagStateHandle	nagState = nil;
long						gHighestAppVersionAtLaunch = 0;
#endif

FeatureRecHandle	gFeatureList = nil;		// A list of all features presently known by Eudora

Boolean gNeedRemind;	// set to true when there's a link the user needs to be reminded about

FSSpec **gRegFiles;

ScriptFontInfo normFonts;
ScriptFontInfo monoFonts;
ScriptFontInfo printNormFonts;
ScriptFontInfo printMonoFonts;

Boolean gImportersAvailable = false;	// set to true if there are importer plugins available for use
Boolean	gScreenChange;	// set when the screen size (resolution) changes
Boolean	gMenuBarIsSetup;	//	We DO have menus
ProxyHandle Proxies;
StackHandle CompactStack;	// specs of mailboxes that need compaction

FSSpec SettingsSpec;	// our current settings file

NoAdsAuxRec NoAdsRec;

Boolean gCanPayMode;	// could the user be in paid mode if they wanted?

#define CurPersSafe	PERS_FORCE(CurPers)

short gEnterWheelHandlerCount;	// prevent reentrant wheelies

Boolean UsingAnyWindows;	// any windows in use?

short ActiveSearchCount;	// how many searches are running now?

long AnyTOCDirty;	// global toc dirty count, used for updating the dock badge

Accumulator OutgoingMIDList;	// list of outgoing message id's
Boolean OutgoingMIDListDirty;	// is it dirty?

AccuPtr ExportErrors;	// accumulate exporting errors here