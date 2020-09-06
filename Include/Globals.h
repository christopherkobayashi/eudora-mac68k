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

#pragma segment Main
/**********************************************************************
 * Global variables for POP mail
 **********************************************************************/
extern int FontID; 						/* font parameters */
extern int FontSize;
extern int FixedID; 						/* fixed font parameters */
extern int FixedSize;
extern int FontWidth;					/* width of average char in font */
extern int FontLead;
extern int FontDescent;
extern int FontAscent;
extern Boolean Done; 					/* set to True when we're done processing */
extern Boolean DontTranslate;	/* don't use translation tables */
extern Boolean FontIsFixed;		/* is font fixed-width? */
extern short InBG; 						/* whether or not we're in the background */
extern Boolean NoSaves;				/* don't prompt user to save documents */
extern Boolean SFWTC;					/* Somebody Fooled With The Cursor */
extern Boolean ScrapFull;			/* is there stuff on the scrap? */
extern Boolean UseCTB; 				/* are we using the CTB? */
extern Boolean Offline; 				/* are we Offline (don't send or check)? */
extern Boolean Stationery; 		/* Stationery on Save As? */
extern Boolean AmQuitting; 		/* are we quitting? */
extern Boolean WrapWrong;			/* wrap the wrong way */
extern Boolean HasHelp;				/* do we have balloon help? */
extern Boolean HasPM;					/* Do we have the process manager? */
extern Boolean NewTables;			/* using new translation table scheme */
extern Boolean LockCode;				/* lock all code resources? */
extern Boolean CantLock;				/* the above failed */
extern Boolean Toshiba;				/* toshiba hard drive? */
extern Boolean LooseTrans;			/* is there a translator with loose morals? */
extern Boolean QTMoviesInited;	/* has QuickTime been initialized for playing movies? */
extern BoxCountHandle BoxCount;/* list of mailboxes for find */
extern XfUndoHandle XfUndoH;		/* for undoing transfers */
extern GrafPtr InsurancePort;	/* a port for use when no others are available */
#if TARGET_RT_MAC_CFM
extern ICMPReport ICMPMessage; /* the last such report */
#endif
extern MessHandle MessList;		/* list of open messages */
extern MyWindow *HandyMyWindow;/* spare private window record */
extern RgnHandle MousePen; 		/* a pen for the mouse */
extern Boolean TBTurnedOnBalloons;	/* the toolbar forced balloon help on */
extern Byte NewLine[4];				/* current newline string */
extern Byte CheckOnIdle;				/* check next time we're idle? */
extern Str15 Type2SelString;		/* current type-2-select string */
extern uLong Type2SelTicks;		/* ticks for last key for type-2-select */
extern short DragSumNum;				/* sumNum for finishing a drag */
extern TOCHandle DragTOCFrom;	/* window drag came from */
extern TOCHandle DragTOCTo;		/* window drag went into */
extern short DragModsWere;			/* what the flags were */
extern uLong DragSequence;			// sequence number for drag
extern short StationVRef;			/* vref for stationery folder */
extern long StationDirId;			/* dirid for stationery folder */
extern void *PETE;							/* magic cookie for pete's editor */
#ifdef DEBUG
extern long ____RandomFailThresh;	// random memory failure threshhold.  set this to nonzero at your peril
#endif
#define kCheck 0x02
#define kSend 0x01
extern TOCHandle TOCList;			/* list of open TOC's */
#ifndef NICKATNIGHT
extern struct AliasDStruct **Aliases;		/* list of alias files */
#endif
extern UHandle eSignature;			/* signature text */
extern UHandle RichSignature;	/* richtext signature */
extern UHandle HTMLSignature;	/* richtext signature */
extern Boolean SigStyled;			/* was the signature per se styled? */
extern UPtr TransIn; 					/* Translation table for incoming characters */
extern UPtr TransOut;					/* Translation table for outgoing characters */
extern UPtr Flatten;						/* Translation table to flatten smart quotes */
extern int CTBTimeout; 				/* current timeout for use with CTB */
extern HostInfoQHandle HIQ;		/* queue of pending DNS lookups */
extern Boolean NoPreflight;
extern Boolean NoNewMailMe;		// put up no new mail alert, please
extern short Dragging;					/* are we in the drag manager? */
extern PETEStyleListHandle Pslh;
extern int SendQueue;					/* # of messages waiting to be sent */
extern uLong ForceSend;				/* next delayed queue */
extern short **StdSizes;				// standard font sizes
extern short **FixedSizes;			// standard fixed-width font sizes
extern struct BoxMapStruct **BoxMap;	/* map of menu id's to dirId's */
extern short **BoxWidths; 			/* where the lines go in a mailbox window */
extern short AliasRefCount;		/* how many subsystems are using aliases? */
extern short ICMPAvail;				/* there is an ICMP report available */
extern short RunType;					/* Production, Debugging, or Steve */
extern struct NMRec *MyNMRec;	/* notification record */
extern MenuHandle CheckedMenu; /* currently checked mailbox menu */
extern short CheckedItem;			/* currently checked mailbox item */
extern Handle SpareSpace[NSpare];		/* extra memory for emergencies */
extern Boolean DoMonitor;			// run the grow-zone monitor
extern Boolean EjectBuckaroo;	/* out of memory; die at next opportunity */
extern Boolean GrowZoning;			/* we are executing the grow zone proc */
extern unsigned short WhyTCPTerminated;				/* why the ctp connection died */
extern EventRecord MainEvent;	/* the event currently being processed */
extern Boolean NoInitialCheck;	/* don't check at startup */
extern long YieldTicks;				/* last CPU yield */
extern Boolean HesOK;					// Is the Hesiod info OK, or should we refetch?
extern uLong GlobalIdleTicks;
extern Boolean NoAnalDictionary;	// we didn't find a moodmail dictionary
extern Boolean WashMe;					// we have dirt on our windows
#ifdef CTB
extern short CTBHasChars;
#endif
extern WindowPtr ModalWindow;	/* run modally for this window */
#ifdef CTB
extern ConnHandle CnH;
#endif
extern VDId Root;							// Root of the Eudora Folder
extern VDId MailRoot;					// Root of the Mail Folder within the Eudora Folder
extern VDId IMAPMailRoot;				// Root of the world's problems
extern VDId ItemsFolder;				// Items folder
extern Style UnreadStyle;			/* style for a mailbox with unread items in it */
extern short LogRefN;					/* ref number of open log file */
extern long LogLevel;					/* current logging level */
extern long LogTicks;					/* time log was opened */
extern Accumulator AuditAccu;	// accumulator for audit data
extern FMBHandle FMBMain;			// Face measurement block for entire app run
extern short AppResFile;				/* application resource file */
extern short HelpResFile;				/* help resource file */
extern Handle Filters;					/* filter rules */
extern short FiltersRefCount;	/* reference count for same */
extern Handle PreFilters;					/* filter rules generated externally (most likely by plug-in), run before normal filters */
extern Handle PostFilters;					/* filter rules generated externally (most likely by plug-in), run after normal filters */
extern ProcessSerialNumber **WordServices;
extern short OriginalHelpCount;	/* # of items in the help menu */
extern short EndHelpCount;				/* # of items after we got done with it */
extern Str255 IsWordChar;				/* are these things words? */
extern short PrefPlugEnd;			/* last Plug-In file from Prefs folder, or app itself */ 
extern long TypeToOpen;
extern WindowPtr UglyHackFrontWindow;	// used for easy-open
extern Str127 MyHostname;			/* our hostname, if we know it */
extern TOCHandle DamagedTOC;
extern Boolean ThereIsColor;
extern Boolean NoDominant;	// don't inherit setting from dominant, just this once
extern ICacheHandle ICache;
extern Boolean VM;
extern Boolean BreakMe;
extern uLong Yesterday;				/* seconds yesterday */
extern Boolean MemCanFail;			/* is it ok for this request to fail? */
extern short FakeTabs;		/* cache this pref for performance reasons */
extern Handle WrapHandle;
extern short ClickType;	/* single, double, or triple */
extern short Windex;			/* window index */
extern short SysRefN;				/* system file reference number */
extern Boolean StartingUp;
extern Boolean SyncRW;
extern Boolean AutoDoubler;
extern Boolean PrefsPlugIns;
extern Boolean TBarHasChanged;	// we changed something that affects the toolbar
extern Boolean D3;	/* 3d look? */
extern Boolean Typing;	// are we typing?
extern Boolean TypingRecently;	// have we typed recently?
extern short PlaylistNagCount;	// how many nags does the playlist manager want us to give?
extern short NewClientModePlusOne;	// the client mode the playlist server wants us in, plus one
extern long TypingTicks;	// last tickcount we noticed a keystroke
extern long ActiveTicks;	// last time user hit key or mouse
extern long NonNullTicks;	// last time we got an even other than a null event
extern Boolean OpenedMacSLIP;
extern Str15 Re;
extern Str15 Fwd;
extern Str15 OFwd;
extern BoxLinesEnum TOCInversionMatrix[2][BoxLinesLimit];	// order of columns in a table of contents
extern Boolean DragFxxkOff;	/* tell the drag manager where to put it */
extern Boolean Sensitive;	/* case-sensitive? */
extern Boolean WholeWord;	/* whole words? */
extern Boolean FurrinSort;	/* do we want to watch your for furriners? */
extern MyWindowPtr DragSource;
extern MyWindowPtr DragTOCSource;
extern short	DragSourceKind;
extern Boolean EmoTurdCache;
extern Boolean AttentionNeeded;
extern Byte YesStr[2];
extern Byte NoStr[2];
extern Byte Slash[3];
extern Byte Cr[2];
extern Byte Lf[2];
extern Byte CrLf[3];
extern Boolean OTIs;
extern Boolean OptiMEMIs;
extern Boolean CheckNow;
extern long StupidTagForACAPandI4;
extern uLong gCheckSessionID;
#ifdef PERF
extern TP2PerfGlobals ThePGlobals;
#endif
#ifdef DEBUG
extern short BugFlags;
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
extern long SpinSpot;
extern MyWindowPtr InsertWin;
extern FSSpec AttFolderSpec;
#ifdef CTB
extern TransVector CTBTrans;
#endif
extern TransVector UUPCTrans;
extern TransVector OTTCPTrans;
extern MyOTTCPStreamHandle pendingCloses;
extern Boolean	gUseOT;
extern Boolean	gHasOTPPP;
extern Boolean gPPPConnectFailed;
extern FSSpec TCPprefFileSpec;
extern FSSpec PPPprefFileSpec;
extern Boolean gMissingNSLib;
#ifdef SPEECH_ENABLED
extern FSSpec SpeechPrefFileSpec;
#endif
#ifdef ADWARE
extern FSSpec SettingsFileSpec;
#endif
extern long gActiveConnections;
extern Boolean gConnecting;
extern Boolean gStayConnected;
#ifdef TWO
extern TransVector PGPTrans;
extern TransVector TransTrans;
extern StackHandle TransContextStack;
extern StackHandle MBRenameStack;
extern long MemLastFailed;
extern long LastTotalSpace, LastContigSpace;
extern Boolean EmptyRecip;
extern short NoScannerResets;
extern Boolean DirtyHackForChooseMailbox;
extern Boolean OpenAddrErrs;
#ifdef WINTERTREE
extern short SpellSession;
extern uLong WinterTreeOptions;
#endif
#endif
#ifdef USECMM
extern Boolean gHasCMM;
#endif
extern Boolean	gHave85MenuMgr;

/* -------------------- Appearance Manager Globals -------------------- */
extern Boolean gAppearanceIsLoaded;
extern Boolean gUseAppearance;						// Register Eudora as Appearance Client
extern Boolean gGoodAppearance;					// Appearance 1.0.1 or later
extern Boolean gBetterAppearance;					// Appearance 1.1 or later
extern Boolean gBestAppearance;					// Appearance 1.1.4 or later
extern SInt32	gLastCtlValue;						// used for live scrolling
extern Boolean gUseLiveScroll;
/* -------------------------------------------------------------------- */
extern Boolean gAXIsSupported;
extern Rect gAXLocation;
#ifdef	FLOAT_WIN
extern WindowPtr lastHilitedWinWP;		// keep track of the last window to be hilighted.
extern MyWindowPtr keyFocusedFloater;	// this floater wants to intercept key presses.
#endif	//FLOAT_WIN
extern Boolean VicomIs;
extern long VicomFactor;
extern Boolean NoMenus;
extern Boolean PleaseQuit;	// toolbar signals it would like a quit
extern Boolean	g16bitSubMenuIDs;	//	Are 16-bit hierarchical menu ID's supported?
extern short		gMaxBoxLevels;	//	Maximum number of mailbox folders in menus
extern IMAPConnectionHandle gIMAPConnectionPool;
extern Str255 gIMAPErrorString;
extern Boolean gbDisplayIMAPWarnings;
											//f1	f2		f3		f4	f5	f6		f7		f8	f9	f10		f11	f12		f13	f14		f15
extern Byte FunctionKeys[];

/************************************************************************
 * mimestore declarations
 ************************************************************************/
extern MStoreSubFile MSSubs[];

/**********************************************************************
 * a few temp vars for macros
 **********************************************************************/
extern uLong M_T1, M_T2, M_T3;

/**********************************************************************
 * thread globals - defined in threading.h
 **********************************************************************/

extern threadGlobalsRec ThreadGlobals;
extern threadGlobalsPtr CurThreadGlobals;

#ifdef THREADING_ON
/**********************************************************************
 * thread sets this to true when there are messages to filter
 **********************************************************************/
extern short TempInCount;
extern short NeedToFilterIn;
extern short NeedToFilterOut;
extern short NeedToNotify;
extern short NeedToFilterIMAP;
extern Boolean NoXfer;

/* When send fails while other threads running (low mem), set this flag to true */
extern Boolean SendImmediately;
extern Boolean CheckThreadRunning;
extern Boolean SendThreadRunning;
extern threadDataHandle gThreadData;

extern short IMAPCheckThreadRunning;
extern short gNewMessages;
extern Boolean gSkipIMAPBoxes;					/* Set to true when we want to skip IMAP mailboxes during filtering */
extern Boolean gWasManualIMAPCheck;

#ifdef TASK_PROGRESS_ON
extern uLong LastCheckTime;
extern Boolean TaskDontAutoClose;
#endif

extern MyWindowPtr TaskProgressWindow;
extern Boolean gTaskProgressInitied;
extern long ThreadYieldTicks;				/* last thread yield */
extern OSErr CheckThreadError;
extern OSErr SendThreadError;
extern Boolean DFWTC;
#endif

extern int TotalQueuedSize;
extern Str255 P1,P2,P3,P4;
extern Boolean NewError;
extern long BgYieldInterval;
extern long FgYieldInterval;

extern long GroupSubjThreshTime;

#ifdef NAG
extern NagStateHandle	nagState;
extern long						gHighestAppVersionAtLaunch;
#endif

extern FeatureRecHandle	gFeatureList;

extern Boolean gNeedRemind;	// set to true when there's a link the user needs to be reminded about

extern FSSpec **gRegFiles;

extern ScriptFontInfo normFonts;
extern ScriptFontInfo monoFonts;
extern ScriptFontInfo printNormFonts;
extern ScriptFontInfo printMonoFonts;

extern Boolean gImportersAvailable;
extern Boolean	gScreenChange;	// set when the screen size (resolution) changes
extern Boolean	gMenuBarIsSetup;	//	We DO have menus
extern ProxyHandle Proxies;
extern StackHandle CompactStack;	// specs of mailboxes that need compaction

extern FSSpec SettingsSpec;	// our current settings file

extern NoAdsAuxRec NoAdsRec;

extern Boolean gCanPayMode;	// could the user be in paid mode if they wanted?

#define CurPersSafe	PERS_FORCE(CurPers)

extern short gEnterWheelHandlerCount;	// prevent reentrant wheelies

extern Boolean UsingAnyWindows;	// any windows in use?

extern short ActiveSearchCount;	// how many searches are running now?

extern long AnyTOCDirty;	// global toc dirty count, used for updating the dock badge

extern Accumulator OutgoingMIDList;	// list of outgoing message id's
extern Boolean OutgoingMIDListDirty;	// is it dirty?

extern AccuPtr ExportErrors;	// accumulate exporting errors here