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

#include <Power.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

#include "main.h"

#define FILE_NUM 22
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Main

#define TICKS2MINS 3600

void HouseKeeping(long activeTicks);
void DoKeyDown(WindowPtr topWinWP, EventRecord * event);
void DoMouseDown(WindowPtr topWinWP, EventRecord * event);
Boolean DoUpdate(WindowPtr topWin, EventRecord * event);
Boolean DoActivate(WindowPtr topWin, EventRecord * event);
Boolean DoApp1(WindowPtr topWinWP, EventRecord * event);
void DoContentClick(WindowPtr topWinWP, WindowPtr winWP,
		    EventRecord * event);
Boolean DoDisk(long message);
long CloseUnused(void);
void SetYesterday(void);
long NickDrain(void);
#ifdef DRAG_GETOSEVT
void UpdateAWindow(void);
#endif
#if defined(EXPIRE) || defined(DEMO)
void ExpireWarning(void);
#endif
void AutoSave(void);

void ConfigureIfNeeded(void);
void UnloadSegments(void);
Boolean MyIsShowContextualMenuClick(EventRecord * event);
void ActiveUserIdleTasks(void);
static void SetupEventHandlers(void);
static pascal OSStatus WheelHandlerProc(EventHandlerCallRef nextHandler,
					EventRef event, void *data);
static pascal OSStatus ServicesHandlerProc(EventHandlerCallRef nextHandler,
					   EventRef event, void *data);
static void AppendOSTypeToCFArray(CFMutableArrayRef arrayRef, OSType type);
pascal void __start(void);
pascal void SimpleStart(void);
pascal void SimpleStart(void)
{
#ifdef DEBUG
	SLInit();
#endif
	__start();
}

void main()
{
	EventRecord theEvent;
	Boolean active = True;
	long houseTicks = 0;
	Boolean lock = False;
	long activeTicks;
	Boolean keyHit;
	//uLong startTicks = 0;

	ActiveTicks = activeTicks = TickCount();
	SyncRW = True;		// protect from RAM Doubler

	/*
	 * perform set-up, including initializing mac managers
	 */
#if __profile__
	ProfilerInit(collectDetailed, bestTimeBase, 20000, 2000);
	ProfilerSetStatus(True);
#endif

	Initialize();

	SetYesterday();
	SeeIfWeShouldPreloadNav();

	CHECK_EXPIRE;

	CHECK_DEMO;

#if defined(EXPIRE) || defined(DEMO)
	ExpireWarning();
#endif

	SetCursorByLocation();

	ConfigureIfNeeded();
	CheckForDefaultMailClient();

	SetupEventHandlers();

	/*
	 * run the event loop
	 */
#if __profile__
	//ProfilerSetStatus(True);
#endif

	while (!EjectBuckaroo && !Done) {
		//if (!startTicks) startTicks = TickCount();
		//if (TickCount()-startTicks > 3600) break;
		//ProfilerSetStatus(ActiveSearchCount > 0);
#ifdef DEBUG
		if (BUG3 && !FrontWindow_()) {
			Str63 s;
			long grow, newFree;
			static long oldFree = 1L << 30;

			(void) MaxMem(&grow);
			DisposeHandle(NewHandle(400 K));
			newFree = FreeMem();
			if (oldFree > newFree) {
				DebugStr(ComposeString
					 (s, "\p#%d;ht;g",
					  oldFree - newFree));
				AlertStr(OK_ALRT, Stop,
					 ComposeString(s,
						       "\pLeak? was %d, now %d (%d)",
						       oldFree, newFree,
						       oldFree - newFree));
			}
			oldFree = newFree;
		}
#endif
		CommandPeriod = NoInitialCheck = False;

		/*
		 * handle any events we might come across 
		 */
		if (!Typing)
			PurgeSpace(&LastTotalSpace, &LastContigSpace);
		//ProfilerSetStatus(False);
		active =
		    GrabEvent(&theEvent) ? HandleEvent(&theEvent) : False;

		CurPers = PersList;	// make darn sure.

		// Did the user tell us to quit?
		if (PleaseQuit) {
			PleaseQuit = False;
			DoQuit(false);
		}
		if (Done)
			break;

		keyHit = (theEvent.what == autoKey
			  || theEvent.what == keyDown)
		    && !(theEvent.modifiers & cmdKey);

		if (DragTOCFrom)
			FinishBoxDrag();
		Dragging = 0;
		DragSequence++;
#if __profile__
		//ProfilerSetStatus(Typing);
#endif

		if (ProgressIsOpen())
			CloseProgress();
		if (keyHit || theEvent.what == mouseDown
		    || theEvent.what == keyDown
		    || theEvent.what == app1Evt)
			ActiveTicks = TickCount();
		if (Typing)
			active = True;
		if (active)
			activeTicks = TickCount();
		if (!Typing && IsMyWindow(FrontWindow_())
		    && GetWindowKind(FrontWindow_()) == TASKS_WIN)
			ActiveTicks = activeTicks = 0;	// if progress window up, user is idle

		TendNotificationManager(active);

		Typing = Typing || !HaveTheDiseaseCalledOSX()
		    && OSEventAvail(keyDownMask, &theEvent)
		    && !(theEvent.modifiers & cmdKey);

		if (!Typing) {
			WindowPtr winWP = FrontWindow_();

			if (IsMyWindow(winWP) && !InBG) {
				if (GetWindowKind(winWP) == CBOX_WIN
				    || GetWindowKind(winWP) == MBOX_WIN)
					RedoTOC((TOCHandle)
						GetWindowPrivateData
						(winWP));
				else {
					MyWindowPtr win =
					    GetWindowMyWindowPtr(winWP);
					if (!win
					    || (win && !win->isActive))
						ActivateMyWindow(winWP,
								 true);
//                                      UpdateMyWindow(winWP);
				}
			}
			if (SFWTC)
				SetCursorByLocation();


			//      See if any plug-ins need some idle time
			ETLIdle(((TickCount() - ActiveTicks < 120)
				 || (ModalWindow
				     && !AreAllModalsPlugwindows())?
				 EMSFIDLE_QUICK : (InBG ? 0 :
						   EMSFIDLE_UI_ALLOWED)) |
				(Offline ? EMSFIDLE_OFFLINE : 0) |
				(GetNumBackgroundThreads()?
				 EMSFIDLE_TRANSFERRING : 0));

			if (!active && !TypingRecently) {
				URLScan();
				QuoteScan();
#ifdef WINTERTREE
				if (HasFeature(featureSpellChecking)
				    && !PrefIsSet(PREF_NO_WINTERTREE))
					SpellScan();
#endif
				if (ActiveSearchCount)
					SearchAllIdle();
			}

			if (TickCount() - houseTicks > 30) {
				HouseKeeping(TickCount() - activeTicks);
				houseTicks = TickCount();
			}
			//              if (QTMoviesInited)
			//              This can cause movies to draw in the wrong place so don't use it
			//                      MoviesTask(nil,50);

			GraphicDownloadIdle();	//      See if any graphic downloads need any processing
			PlaySoundIdle();	//      Give time to any sounds that might be playing
#ifdef TWO
			/*
			 * tickle Frontier
			 */
			if (active)
				CheckSharedMenus(HIER_MENU_LIMIT);
#endif

#ifndef SLOW_CLOSE
			/*
			 * destroy defunct streams
			 */
			TcpFastFlush(False);
#endif

			/*
			 * write dirty toc's
			 */
			if (!active)
				RedoTOCs();
			if (!active && !PrefIsSet(PREF_CORVAIR))
				FlushTOCs(False, True);

			/*
			 * cursor & help
			 */
			if (SFWTC)
				SetCursorByLocation();
			if (theEvent.what == nullEvent && HasHelp && !InBG
			    && HMGetBalloons())
				DoHelp(FrontWindow_());

			ASSERT(CurResFile() == SettingsRefN);
			UseResFile(SettingsRefN);
			ASSERT(Count1Resources(TOC_TYPE) == 0);


#ifdef TWO
			/*
			 * toolbar
			 */
			ToolbarBack();
#endif
			/*
			 * all windows are fair game
			 */
			NotUsingAllWindows();

			/*
			 * menu bar
			 */
			if (active)
				EnableMenus(MyFrontWindow(), False);
			NoMenus = false;

			/*
			 * check?
			 */
			if (!active && CheckNow) {
				if (AutoCheckOKWithDBRead(false))	// make sure we're really allowed to do an automatic check
					XferMail(True,
						 PrefIsSet
						 (PREF_SEND_CHECK), False,
						 False, True, 0);

				CheckNow = False;
			}
		}

		/*
		 * clear gworld, personality stack
		 */
		ASSERT(StackPop(nil, GWStack) == fnfErr);
		while (!StackPop(nil, GWStack));
		ASSERT(!PersStack || StackPop(nil, PersStack) == fnfErr);
		while (PersStack && !StackPop(nil, PersStack));
		CurPers = PersList;
		ASSERT(CurThreadGlobals == &ThreadGlobals);
		if (EjectBuckaroo)
			DoQuit(false);
	}
#ifdef	IMAP
	ZapAllIMAPConnections(true);	// abort all IMAP connections
#endif
	TcpFastFlush(True);	//abort open streams
	CheckSLIP();		//kill macslip
#ifdef	USE_NETWORK_SETUP
	CloseNetworkSetup();
#endif
	// archive the junk mailbox
	if (JunkPrefBoxArchive() && JunkTrimOK())
		ArchiveJunk(GetJunkTOC());
#ifdef	IMAP
	// empty only the local trash.  The IMAP trash was taken care of before network was closed down.
	if (PrefIsSet(PREF_AUTO_EMPTY))
		EmptyTrash(true, false, false);
#else
	if (PrefIsSet(PREF_AUTO_EMPTY))
		EmptyTrash();
#endif
	FlushTOCs(True, False);

	Cleanup();

#if __profile__
	ProfilerSetStatus(False);
	ProfilerDump("\peudora-profile");
	ProfilerTerm();
#endif
}

void ConfigureIfNeeded(void)
{
	Str255 pop;

	if (!*GetPOPPref(pop) && PrefIsSet(PREF_ACAP))
		ACAPLoad(True);
	if (!*GetPOPPref(pop) || pop[1] == '@')	// ACAP helped?
	{
		// ask the user if they'd like to import mail from another account ...
		if (gImportersAvailable
		    && ComposeStdAlert(Note, IMPORT_ON_STARTUP) == 1) {
			DoMailImport();
		} else {
			DoSettings(0);
		}
	}
}

#if defined(EXPIRE) || defined(DEMO)
/************************************************************************
 * ExpireWarning - warn the user that Eudora will expire in the next 7 days
 * This routine works with an EXPIRE version AND a DEMO version.
 ************************************************************************/
void ExpireWarning(void)
{
	Str255 date;
	DateTimeRec dtr;
	uLong expireSecs;

#ifdef EXPIRE
	Zero(dtr);
	dtr.year = EXP_YEAR;
	dtr.month = EXP_MONTH + 1;
	dtr.day = 1;

	Date2Secs(&dtr, &expireSecs);
	if (expireSecs - LocalDateTime() < 3600L * 24 * 7) {
		DateString(expireSecs, longDate, date, nil);
		if (ComposeStdAlert(Caution, WILL_EXPIRE, date) ==
		    kAlertStdAlertOKButton)
			OpenAdwareURL(GetNagState(), UPDATE_SITE,
				      actionUpdate, updateQuery, nil);
	}
#endif

#ifdef DEMO
	if (gTimeBombData.days != TB_DISARMED)	//only warn if the timebomb is not disarmed, 
	{
		long expiresOn =
		    gTimeBombData.date +
		    (gTimeBombData.days * NUMBER_OF_SECS_DAY);

		Secs2Date(expiresOn, &dtr);
		Date2Secs(&dtr, &expireSecs);
		if (expireSecs - LocalDateTime() < 3600L * 24 * 7) {
			SetMyCursor(arrowCursor);
			DateString(expireSecs, longDate, date, nil);
			MyParamText(date, 0, 0, 0);
			if (Alert(WILL_EXPIRE_ALRT, nil) ==
			    BUY_NOW_BUTTON_WILL_EXPIRE)
				PurchaseEudora();
		}
	}
#endif

}
#endif

/************************************************************************
 * HouseKeeping - periodic cleanup functions
 ************************************************************************/
void HouseKeeping(long activeTicks)
{
	static long flushTicks = 0;
	uLong checkTicks;
	static long AutoSaveTicks;
	long autoSecs;
	static long spoolCleaned;

	if (activeTicks > 10) {
		long ticks;
		WindowPtr winWP;
		MyWindowPtr win;
		static long lastBadgeTOCDirty = -1;

		CurPers = PersList;	// make darn sure

		EnableMenus(MyFrontWindow(), False);

		/*
		 * warn the user if we're growing low (get it) on memory
		 */
		MonitorGrow(True);

		if (!InBG && TickCount() % 20 == 0)
			FloatingWinIdle();

		if (FixServers) {
			RedoTOCs();
			FixMessServerAreas();
			FixServers = False;
		}

		/*
		 * When was yesterday?
		 */
		SetYesterday();

		if (PrefBadgeDo() && lastBadgeTOCDirty != AnyTOCDirty) {
			lastBadgeTOCDirty = AnyTOCDirty;
			BadgeTheSupidDock(GlobalUnreadCount(), nil,
					  AttentionNeeded || MyNMRec);
		}

		/*
		 * Do we need to switch modes?
		 */
		if (NewClientModePlusOne)
			AutoSwitchClientMode();

		//
		// Update the emo turd cache
		EmoTurdCache = EmoLitterInternetWithTurds();
		/*
		 * Do we need to nag the user?
		 */
		if (PlaylistNagCount) {
			short i;
			Handle res;

			for (i = 1;
			     res = GetIndResource(NAG_REQUEST_TYPE, i);
			     i++) {
				short id;
				OSType tipe;

				GetResInfo(res, &id, &tipe, GlobalTemp);
				DoPlaylistNag(id);
			}
			PlaylistNagCount = 0;
		}

		/*
		 * Register?
		 */
#ifndef NAG
		if (!ModalWindow)
			AutoRegister();
#endif

		//if ((win=FrontWindow_()) && IsMyWindow(win) && win->qWindow.windowKind==MESS_WIN)
		//GenerateReceipt(Win2MessH(win),MDN_DISPLAYED,True);

#ifdef THREADING_ON
		/*
		 * report fatal thread errors
		 */
		if (CheckThreadError) {
			WarnUser(THREAD_CANT_CHECK, CheckThreadError);
			CheckThreadError = noErr;
		}
		if (SendThreadError) {
			WarnUser(THREAD_CANT_SEND, SendThreadError);
			SendThreadError = noErr;
		}
#endif

		if ((autoSecs = GetRLong(AUTOSAVE_INTERVAL))
		    && TickCount() - AutoSaveTicks > autoSecs * 60) {
			AutoSaveTicks = TickCount();
			AutoSave();
		}

		if (NewError && !GetNumBackgroundThreads()) {
			WindowPtr mywin;

			if (!IsTPIdleControlVisible()
			    || PrefIsSet(PREF_TASK_PROGRESS_AUTO))
				OpenTasksWinErrors();
			else if ((mywin = FrontWindow_())
				 && GetWindowKind(mywin) == TASKS_WIN) {
				if (!InBG)
					SysBeep(1);
				NewError = false;
			}
		}

		/*
		 * save window state if need be
		 */
		if (WashMe && DiskSpunUp())
			RememberOpenWindows();

		/*
		 * save the mid list
		 */
		OutgoingMIDListSave();

		/*
		 * check mail?
		 */
#ifdef THREADING_ON
		// for now, don't start thread if modal window up. this may change...
		if (((!PrefIsSet(PREF_THREADING_OFF) && ThreadsAvailable()
		      && activeTicks > 120)
		     || (activeTicks > GetRLong(IDLE_SECS) * 60))
		    && (!ModalWindow || AreAllModalsPlugwindows())
		    && !CheckThreadRunning && !(PrefIsSet(PREF_POP_SEND)
						&& SendThreadRunning))
#else
		if (activeTicks > GetRLong(IDLE_SECS) * 60
		    && (!ModalWindow || AreAllModalsPlugwindows()))
#endif
		{
			if (CheckOnIdle) {
				XferMail((CheckOnIdle & kCheck) != 0,
					 (CheckOnIdle & kSend) != 0, False,
					 True, True, 0);
				CheckOnIdle = 0;
				return;
			}
			if (checkTicks = PersCheckTicks()) {
				if (TickCount() > checkTicks) {
					if (!Offline
					    && !(CurrentModifiers() &
						 shiftKey)) {
						// skip this check if we're not online.  Be anal and slow about checking the connection.
						if (AutoCheckOKWithDBRead
						    (true)) {
							XferMail(True,
								 PrefIsSet
								 (PREF_SEND_CHECK),
								 False,
								 False,
								 True, 0);
							return;
						}
					}
				}
			}
			if (ForceSend && ForceSend <= GMTDateTime()
			    && AutoCheckOK()) {
				XferMail(False, True, False, False, True,
					 0);
				return;
			}
		}
#ifdef THREADING_ON
		/* if a send thread couldn't be spawned, try it now */
		if (SendImmediately && !SendThreadRunning) {
			XferMail(False, True, False, False, True, 0);
			return;
		}
#endif

		if (OpenAddrErrs && !AmQuitting && !ModalWindow
		    && activeTicks >
		    GetRLong(SHORT_MODAL_IDLE_SECS) * 60) {
			//      Recently sent one or more messages with addressing errors. Open 'em.
			TOCHandle outTocH;
			short sumNum;

			if (outTocH = GetOutTOC())
				for (sumNum = (*outTocH)->count - 1;
				     sumNum >= 0; sumNum--)
					if ((*outTocH)->sums[sumNum].
					    flags & FLAG_ADDRERR) {
						(*outTocH)->sums[sumNum].
						    flags &= ~FLAG_ADDRERR;
						if (!GetAMessage
						    (outTocH, sumNum, nil,
						     nil, True))
							break;
					}
			OpenAddrErrs = false;
		}


		/*
		 * close unneeded TOC's
		 */
		if (activeTicks > GetRLong(TOC_SMALLDIRTY)) {
			TOCHandle tocH;
			for (tocH = TOCList; tocH; tocH = (*tocH)->next)
				(*tocH)->reallyDirty = TOCIsDirty(tocH)
				    || (*tocH)->reallyDirty;
		}
		FlushTOCs(True, True);

		/*
		 * just for good measure
		 */
		if (TickCount() - flushTicks > 60 * GetRLong(FLUSH_SECS)
		    && DiskSpunUp()) {
			FlushVol(nil, MailRoot.vRef);
			flushTicks = TickCount();
		}

		/*
		 * close speller, if idle for a while
		 */
#ifdef WINTERTREE
		if (HasFeature(featureSpellChecking))
			SpellClose(false);
#endif				//WINTERTREE

		/*
		 *      Wander over to the speaker and make sure he or she is happy
		 */
#ifdef SPEECH_ENABLED
		(void) SpeechIdle();
#endif

		/*
		 * Save the personalities
		 */
		if (HasFeature(featureMultiplePersonalities))
			PersSaveAll();

		/*
		 * The resolver may have left us little poodle bombs...
		 */
		FlushHIQ();

		/*
		 * and logging...
		 */
#ifdef THREADING_ON
		if (!GetNumBackgroundThreads())
#endif
			if (!(LogLevel & LOG_EVENT))
				CloseLog();

		/*
		 * and that pesky settings file
		 */
		MyUpdateResFile(SettingsRefN);

#ifdef TWO
		/*
		 * stop MacSLIP?
		 */
		CheckSLIP();
#endif

		/*
		 * close unused IMAP connections
		 */
		CheckIMAPConnections();

		/*
		 * display any pending IMAP warnings
		 */
		IMAPWarnings();

		/*
		 *      Remind the user about marked links, if we're online.
		 */
		if (activeTicks > GetRLong(OFFLINE_LINK_NAG_TIME))
			RemindNag();

#ifdef NEVER
		if (RunType != Production) {
			Str255 item, item2;
			short i;
			MenuHandle mh;

			*item2 = 0;
			for (i = NEW_TO_HIER_MENU;
			     i <= INSERT_TO_HIER_MENU; i++) {
				if (mh = GetMHandle(i)) {
					GetItem(mh, 1, item);
					if (*item2
					    && !StringSame(item, item2)) {
						PSCat(item, "\p<->");
						PSCat(item, item2);
						DebugStr(item);
						break;
					}
					PCopy(item2, item);
				}
			}
		}
#endif

		/*
		 * fcc?
		 */
		//Folder Carbon Copy - light does not support the FCC feature
		if (HasFeature(featureFcc))
			FccFlop(GetWindowMyWindowPtr(FrontWindow_()));

		/*
		 *      Link History Aging
		 */
		if (HasFeature(featureLink))
			AgeLinks();

		/*
		 * idle routines
		 */
		ticks = TickCount();
		for (winWP = GetWindowList();
		     winWP && TickCount() - ticks < 10;
		     winWP = GetNextWindow(winWP)) {
			win = GetWindowMyWindowPtr(winWP);
			if (IsKnownWindowMyWindow(winWP) && win
			    && win->idle
			    && ticks - win->lastIdle > win->idleInterval) {
				SetPort_(GetWindowPort(winWP));
				win->idle(win);
				// (jp) Handle idle processing for intelligent nick fields
				if (win->pte
				    && HasNickScanCapability(win->pte))
					NicknameWatcherIdle(win->pte);
				win->lastIdle = TickCount();
			}
		}
#ifdef THREADING_ON
#ifndef BATCH_DELIVERY_ON
		if (!ModalWindow || AreAllModalsPlugwindows())	// no modal window uppermost
			if ((NeedToFilterIn && !CheckThreadRunning) || (NeedToFilterOut && !SendThreadRunning))	// we might plausibly have some messages
				if (InBG || ((TickCount() - ActiveTicks) > GetRLong(MODAL_IDLE_SECS) * 60))	// the user is not busy in Eudora
					FilterXferMessages();
#else
		if (NeedToFilterIn || NeedToFilterOut || NeedToNotify || NeedToFilterIMAP)	// there might be something there
			if (!ModalWindow || AreAllModalsPlugwindows())	// no modal window uppermost
				if (TickCount() - ActiveTicks > GetRLong(SHORT_MODAL_IDLE_SECS) * 60)	// user is not busy in Eudora
					FilterXferMessages();
#endif
#endif
		if ((!ModalWindow || AreAllModalsPlugwindows())
		    && TickCount() - ActiveTicks >
		    GetRLong(SHORT_MODAL_IDLE_SECS) * 60 && NoNewMailMe) {
			NoNewMailMe = false;
			NotifyNewMail(0, false, nil, nil);
		}
#ifdef	IMAP
		// update any IMAP message windows that need it
		UpdateIMAPWindows();

		// do New Mail alert if an IMAP check recently completed
		if ((!ModalWindow || AreAllModalsPlugwindows())
		    && TickCount() - ActiveTicks >
		    GetRLong(SHORT_MODAL_IDLE_SECS) * 60
		    && !CheckThreadRunning && !IMAPCheckThreadRunning
		    && gNewMessages) {
			NotifyNewMail(0, false, nil, nil);
			gNewMessages = 0;
		}
#endif
	}

	/*
	 * toolbar?
	 */
	if (TBarHasChanged) {
		TBarHasChanged = False;
		if (PrefIsSet(PREF_TOOLBAR) != ToolbarShowing())
			OpenToolbar();
		else if (ToolbarShowing()) {
			OpenToolbar();
			OpenToolbar();
		}
	}
	if (!InBG)
		ShowToolbar();

	/*
	 * run the text analyzer
	 */
	if (!TypingRecently && BeingAnal())
		AnalScan();

	// and the emoticon scanner
	if (!TypingRecently && HasFeature(featureEmo) && EmoDo())
		EmoScan();

	/*
	 * do idle tasks that matter only for active users
	 */
	ActiveUserIdleTasks();

	AdWinIdle();
	StatsIdle();

	if (gScreenChange && !InBG)	// change in screen size (resolution)
	{
		gScreenChange = false;
		ScreenChange();
	}

	/*
	 * Really obno stuff
	 */
	if (InBG || activeTicks > GetRLong(LONGTERM_IDLE) * 60) {
		// perform any scheduled AddressBook Sync
		OSXAddressBookScheduledSync();

		/*
		 * clean up the spool folder
		 */
		if (GetPrefLong(PREF_SPOOL_DESTROY)
		    && GMTDateTime() - spoolCleaned > 4 * 3600)
			if (!CleanSpoolFolder
			    (GetPrefLong(PREF_SPOOL_DESTROY)))
				spoolCleaned = GMTDateTime();

		// perform IMAP auto expunge.
		IMAPAutoExpunge();

		// any mailboxes need compaction?
		if (!GetNumBackgroundThreads())	// don't compact if threads running; might lead to conflicts
		{
			FSSpec spec;
			if (!StackPop(&spec, CompactStack)) {
				CompactMailbox(&spec, false);
				if (GetHandleSize(CompactStack) /
				    (*CompactStack)->elSize -
				    (*CompactStack)->elCount > 20)
					StackCompact(CompactStack);
				return;
			}
		}
	}
}

/************************************************************************
 * ActiveUserIdleTasks - do idle tasks we do if the user is active
 ************************************************************************/
void ActiveUserIdleTasks(void)
{
	static uLong stampTicks;
	static uLong netTicks;

	// if the user has been idle for a minute or more, stop doing these things
	if (!UserIdle(ALMOST(TICKS2MINS))) {
		/*
		 *      Check on our Network Connection every minute or so
		 */

		if (TickCount() - netTicks > TICKS2MINS) {
			// update the cached network status
			TCPWillDial(true);

			netTicks = TickCount();
		}

		/*
		 *     Timestamp and flush the audit log once every two minutes
		 */

		if (!stampTicks)
			stampTicks = TickCount();
		else if (FMBMain
			 && (TickCount() - stampTicks > 2 * TICKS2MINS)) {
			long faceTime, rearTime, connectTime, totalTime;
			FaceMeasureReport(FMBMain, &faceTime, &rearTime,
					  &connectTime, &totalTime);
			AuditTimestamp(faceTime, rearTime, connectTime,
				       totalTime);
			stampTicks = TickCount();
		}
	}
}

/************************************************************************
 * AutoSave - save open windows
 ************************************************************************/
void AutoSave(void)
{
	WindowPtr winWP;
	MyWindowPtr win;
	Boolean saveMe;

	for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr(winWP);
		if (IsKnownWindowMyWindow(winWP) && win
		    && IsDirtyWindow(win)) {
			saveMe = false;
			switch (GetWindowKind(winWP)) {
			case ALIAS_WIN:
			case TEXT_WIN:
			case COMP_WIN:
			case FILT_WIN:
				saveMe = true;
				break;
			case MESS_WIN:
				saveMe =
				    MessOptIsSet(Win2MessH(win),
						 OPT_WRITE);
				break;
			}
			if (saveMe)
				DoMenu2(winWP, FILE_MENU, FILE_SAVE_ITEM,
					0);
		}
	}
}

/**********************************************************************
 * CheckSLIP - check SLIP and PPP, hangup if need be
 **********************************************************************/
void CheckSLIP(void)
{
	if (gActiveConnections == 0 && !gConnecting && !gStayConnected
	    && !AdWinNeedsNetwork()) {
		if (gUseOT) {
			extern MyOTPPPInfoStruct MyOTPPPInfo;

			if (gHasOTPPP && !pendingCloses
			    && MyOTPPPInfo.weConnectedPPP)
				OTPPPDisconnect(false, true);
		}
	}
}

/**********************************************************************
 * FccFlop - change the transfer menu for Fcc
 **********************************************************************/
void FccFlop(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Str255 s;
	MenuHandle mh;
	static short curName = 0;
	short shdName;
	PETEHandle pte;

	//Folder Carbon Copy - Light does not support the FCC feature
	if (!HasFeature(featureFcc))
		return;

	if (win &&
	    IsWindowVisible(winWP) &&
	    GetWindowKind(winWP) == COMP_WIN &&
	    SumOf(Win2MessH(win))->state != SENT &&
	    (pte = (*Win2MessH(win))->bodyPTE) &&
	    BCC_HEAD == CompHeadCurrent(pte)) {
		shdName = FCC_NAME;
		UseFeature(featureFcc);
	} else
		shdName = TRANSFER_MNAME;

	if (shdName != curName && (mh = GetMHandle(TRANSFER_MENU))) {
		MySetMenuTitle(mh, GetRString(s, shdName));
		DrawMenuBar();
		curName = shdName;
		TransferMenuHelp(shdName ==
				 FCC_NAME ? FCC_HMNU : TFER_HMNU);
	}
}

/************************************************************************
 * TransferMenuHelp - copy the correct hmnu into our settings file
 ************************************************************************/
void TransferMenuHelp(short id)
{
	Handle h;
	Zap1Resource('hmnu', TRANSFER_MENU);
	ResourceCpy(SettingsRefN, HelpResFile, 'hmnu', id);
	{
		if (h = Get1Resource('hmnu', id)) {
			HNoPurge(h);
			SetResInfo(h, TRANSFER_MENU, "");
		}
	}
}

/**********************************************************************
 * SetYesterday - set our idea of when today begins
 **********************************************************************/
void SetYesterday(void)
{
	DateTimeRec dtr;
	static long ticks;
	static long lastYesterday;

	if (TickCount() - ticks > TICKS2MINS || !ticks) {
		long secs = LocalDateTime();
		ticks = TickCount();
#ifdef	DEMO
		today = secs;
#endif
		SecondsToDate(secs, &dtr);

		dtr.hour = dtr.minute = dtr.second = 0;

		DateToSeconds(&dtr, &secs);

		if (secs != lastYesterday) {
			lastYesterday = secs;
			RedoAllTOCs();
		}

		Yesterday = LocalDateTime() - secs;
	}
}

/**********************************************************************
 * HandleEvent - handle an event
 **********************************************************************/
Boolean HandleEvent(EventRecord * event)
{
	WindowPtr topWinWP = MyFrontWindow();	/* in case we need to know */
	Boolean active = False;

	MainEvent = *event;	/* copy it into an accessible place */

#ifdef	GX_PRINTING
	if (!isGXDialog && MyIsDialogEvent(event))
#else				//GX_PRINTING
	if (MyIsDialogEvent(event))
#endif				//GX_PRINTING
	{
		DialogPtr dlgPtr =
		    GetDialogFromWindow(Event2Window(event));
		if (!dlgPtr)
			dlgPtr = GetDialogFromWindow(topWinWP);
		if (IsMyWindow(GetDialogWindow(dlgPtr)))
			DoModelessEvent(dlgPtr, event);
		SetPort(InsurancePort);
		return (True);
	}


	if (topWinWP) {
		SetPort_(GetWindowPort(topWinWP));
		UsingWindow(topWinWP);
	} else
		SetPort_(InsurancePort);
#ifdef USECMM
	//check for contextual menu event
	if (gHasCMM && MyIsShowContextualMenuClick(event)) {
		HandleContextualMenuClick(topWinWP, event);
		return (active);
	}
#endif

	/*
	 * slog through all the events...
	 */
	switch (event->what) {
	case keyDown:
	case autoKey:
#ifdef	FLOAT_WIN
		if (keyFocusedFloater)
			topWinWP = GetMyWindowWindowPtr(keyFocusedFloater);	//FLOAT_WIN
#endif				//FLOAT_WIN
		/*if (HasHelp && HMIsBalloon()) HMRemoveBalloon(); */
		UsingWindow(topWinWP);
		DoKeyDown(topWinWP, event);
		active = True;
		break;

	case mouseUp:
	case mouseDown:
		/*if (HasHelp && HMIsBalloon()) HMRemoveBalloon(); */
		DoMouseDown(topWinWP, event);
		active = True;
		break;

	case updateEvt:
		UsingWindow(topWinWP);
		DoUpdate(topWinWP, event);
		break;

	case activateEvt:
		if (!InBG)
			DoActivate(topWinWP, event);
		break;

	case app4Evt:
		active = DoApp4(topWinWP, event);
		break;

	case app1Evt:
		UsingWindow(topWinWP);
		active = DoApp1(topWinWP, event);
		break;

	case kHighLevelEvent:
		(void) AEProcessAppleEvent(event);
		break;
	}
	SetPort(InsurancePort);
	if (!ForceSend)
		SetSendQueue();	/* a message from beyond */
	return (active);
}

/************************************************************************
 * MyIsShowContextualMenuClick - is this a context menu event?
 ************************************************************************/
Boolean MyIsShowContextualMenuClick(EventRecord * event)
{
	uLong opts = GetPrefLong(PREF_DRAG_OPTIONS);

	if ((!(opts & 2) || !(event->modifiers & controlKey))
	    && IsShowContextualMenuClick(event))
		return (true);

	return (false);
}

/**********************************************************************
 * DoApp4 - handle an app4 event; suspend/resume/mouse
 **********************************************************************/
Boolean DoApp4(WindowPtr topWinWP, EventRecord * event)
{
	WindowPtr winWP;
	MyWindowPtr topWin;

	if (((event->message >> 24) & 0xff) == 0xfa) {
		SFWTC = True;
	} else {
		ClickType = Single;	// no longer double-clicking if suspend/resume

		if (((event->message) >> 24) & 1 == 1) {
			InBG = (event->message & 1) == 0;
			if (!InBG)
				SFWTC = True;
			topWinWP = FrontWindow_();
#ifdef CTB
			if (CnH)
				CMResume(CnH, !InBG);
#endif
			InBG = (event->message & 1) == 0;
			if (InBG) {
				MyHMHideTag();
				HiliteMenu(0);
			} else {
#ifdef REFRESH_LABELS_MENU
				RefreshLabelsMenu();
#endif				// REFRESH_LABELS_MENU
#ifdef	GX_PRINTING
				if (gGXIsPresent)
					UpdateGXPageSetup();
#endif				//GX_PRINTING
			}
			topWin = GetWindowMyWindowPtr(topWinWP);
			if (topWinWP && IsMyWindow(topWinWP)
			    && InBG == topWin->isActive)
				ActivateMyWindow(topWinWP, !InBG);

			//      Show/hide floating/docked windows
			for (winWP = GetWindowList(); winWP;
			     winWP = GetNextWindow(winWP))
				if (IsFloating(winWP)) {
					topWin =
					    GetWindowMyWindowPtr(winWP);
					if (InBG)
						StashStructure(topWin);
					ShowHide(winWP, !InBG);
					if (!InBG)
						StashStructure(topWin);
				}
			if (!InBG) {
				EventRecord mouseDownEvent;

				//      Make sure docked windows are still appropriately docked
				//      in case user changed screen resolution while Eudora
				//      was in background
				PositionDockedWindows();
				//      Make sure pending mouse downs don't hit a floating
				//      window that has just been made visible  
				if (OSEventAvail
				    (mDownMask, &mouseDownEvent))
					if (FindWindow_
					    (event->where,
					     &winWP) >= inContent)
						if (IsFloating(winWP))
							//      Remove mousedown event for click in this floating window
							GetNextEvent
							    (mDownMask,
							     &mouseDownEvent);
			}
		}
	}
	return (False);
}

/**********************************************************************
 * RefreshLabelsMenu - get a new copy of the Finder's label strings
 **********************************************************************/
void RefreshLabelsMenu(void)
{
	MenuHandle mh = GetMHandle(LABEL_HIER_MENU);

	if (mh) {
		TrashMenu(mh, 3);
		AddFinderItems(mh);
		SetItemMark(mh, 1, noMark);
		EnableItem(mh, 0);
		EnableItem(mh, 1);	//      For some reason OS X may leave this disabled
	}
}

/************************************************************************
 * SetSendQueue - preload the SendQueue global with the number of queued
 * messages.
 ************************************************************************/
void SetSendQueue(void)
{
	TOCHandle tocH;
	int sumNum;
	uLong gmtSecs = GMTDateTime();
	PersHandle pers;

//      if (InAThread())
//              return;
	SendQueue = 0;
	ForceSend = 0xffffffff;
	if (!(tocH = GetRealOutTOC()))
		return;

	for (pers = PersList; pers; pers = (*pers)->next)
		(*pers)->sendQueue = 0;
	for (sumNum = 0; sumNum < (*tocH)->count; sumNum++) {
		if ((*tocH)->sums[sumNum].messH)
			continue;	// skip open messages
		if ((*tocH)->sums[sumNum].state == TIMED) {
			if ((*tocH)->sums[sumNum].seconds &&
			    (*tocH)->sums[sumNum].seconds < ForceSend)
				ForceSend = (*tocH)->sums[sumNum].seconds;
			if ((*tocH)->sums[sumNum].seconds <= gmtSecs)
				SetState(tocH, sumNum, QUEUED);
		}
		if ((*tocH)->sums[sumNum].state == QUEUED) {
			SendQueue++;
			pers = TS_TO_PERS(tocH, sumNum);
			if (!pers) {
				pers = PersList;
				(*tocH)->sums[sumNum].persId = 0;
				TOCSetDirty(tocH, true);
			}
			(*pers)->sendQueue++;
		}
	}
}

/**********************************************************************
 * DoApp1 - handle an app1 event; scrolling by saratoga keys
 **********************************************************************/
Boolean DoApp1(WindowPtr topWinWP, EventRecord * event)
{
	if (topWinWP && IsMyWindow(topWinWP)) {
		MyWindowPtr topWin = GetWindowMyWindowPtr(topWinWP);
		if (topWin->app1 && (*topWin->app1) (topWin, event));
		else if (topWin->pte) {
			event->what = keyDown;
			PeteEdit(topWin, topWin->pte, peeEvent,
				 (void *) event);
		} else
			return DoApp1NoPTE(topWin, event);
	}
	return (True);
}

/**********************************************************************
 * DoApp1NoPTE - handle an app1 event but ignore window & pte handlers
 **********************************************************************/
Boolean DoApp1NoPTE(MyWindowPtr topWin, EventRecord * event)
{
	if (topWin->vBar)
		switch (event->message & charCodeMask) {
		case homeChar:
			ScrollIt(topWin, REAL_BIG, REAL_BIG);
			break;
		case endChar:
			ScrollIt(topWin, 0, -REAL_BIG);
			break;
		case pageUpChar:
			ScrollIt(topWin, 0,
				 RoundDiv(topWin->contR.bottom -
					  topWin->contR.top,
					  topWin->vPitch) - 1);
			break;
		case pageDownChar:
			ScrollIt(topWin, 0,
				 RoundDiv(topWin->contR.top -
					  topWin->contR.bottom,
					  topWin->vPitch) + 1);
			break;
		}
	return true;
}

/**********************************************************************
 * DoKeyDown - handle a keystroke aimed at a window
 **********************************************************************/
void DoKeyDown(WindowPtr topWinWP, EventRecord * event)
{
	long select;
	short c = UnadornMessage(event) & 0xff;
	OSErr err = errAEEventNotHandled;
	Boolean done;

	// (jp) Check for cmd-period while speaking
#ifdef SPEECH_ENABLED
	if (event->what == keyDown && event->modifiers & cmdKey && IsCmdChar(event, '.') ||	// command-period
	    (event->message & charCodeMask) == escChar)	//esc
		if (CancelSpeech()) {
			FlushEvents(keyDownMask | keyUpMask | mDownMask | mUpMask, 0);	/* kill events on cmd-period */
			return;
		}
#endif

	ClickType = Single;	// can't keydown during a double-click

	if (IsMyWindow(topWinWP)) {
		MyWindowPtr topWin = GetWindowMyWindowPtr(topWinWP);

		SetPort_(GetWindowPort(topWinWP));

		// (jp) If the field supports nick stuff, handle the keystroke
		done = false;
		if (HasNickScanCapability(topWin->pte)) {
			if (!
			    (done =
			     NicknameWatcherKey(topWin->pte, event)))
				if (topWin->key) {
					if ((*topWin->key) (topWin, event))
						done = true;
				} else if (!(event->modifiers & cmdKey)) {
					(void) PeteEdit(topWin, topWin->pte, peeEvent, (void *) event);	// (jp) Just in case no key function is defined (dialogs)
					done = true;
				}
			NicknameWatcherKeyFollowup(topWin->pte);
//                      if (IsWazoo (topWin))
//                              ValidRect (PeteRect (topWin->pte, &r));         // (jp) Lame, I know... but we have a flicker problem.  :|
			//                      The problem is caused because the nickname watching
			//                      code invalidates the field.  Okay, fair enough...
			//                      But, if the nickname window is wazooed, WazooPreUpdate
			//                      is called which calls Draw1Control on the tab -- which
			//                      erases the text we just finished drawing.  Boo.  Hiss.
			//                      We con't have this problem in either composition windows
			//                      or in windows that not wazooed.  A more general solution
			//                      should eventually replace this hackery.
		}
		if (!done) {
			short charCode = event->message & charCodeMask;

			if (!topWin->pte && charCode == backSpace
			    && event->modifiers == cmdKey)
				DoMenu(topWinWP,
				       (EDIT_MENU << 16) | EDIT_CLEAR_ITEM,
				       0);
			else if (topWin->key
				 && (*topWin->key) (topWin, event));
			else if (event->modifiers & cmdKey) {
				if (IsMyWindow(topWinWP) && topWin
				    && topWin->pte)
					err =
					    PeteEdit(topWin, topWin->pte,
						     peeEvent,
						     (void *) event);
				if (err == errAEEventNotHandled)
					if (select = MyMenuKey(event))
						DoMenu(topWinWP, select,
						       event->modifiers);
					else
						SysBeep(20L);
				return;
			} else if (topWin->pte)
				PeteKey(topWin, topWin->pte, event);
		}
	} else if (event->modifiers & cmdKey) {
		if (select = MyMenuKey(event))
			DoMenu(topWinWP, select, event->modifiers);
		else
			SysBeep(20L);
	} else
		SysBeep(20L);
}

pascal void Hook(void)
{
	if (RunType != Production && CurrentModifiers() & controlKey) {
		Handle fkey3;
		if (fkey3 = GetResource_('FKEY', 3)) {
			(*(ProcPtr) LDRef(fkey3)) ();
			UL(fkey3);
		} else
			SysBeep(10);
	}
}

/**********************************************************************
 * DoMouseDown - handle a mouse-down event, goodness knows where
 **********************************************************************/
void DoMouseDown(WindowPtr topWinWP, EventRecord * event)
{
	static Point downSpot;	/* Point of previous mouseDown */
	static long upTicks;	/* time of previous mouseUp */
	long dblTime;		/* double-click time from PRAM */
	long wPart;		/* window part where click occurred */
	MyWindowPtr topWin;
	WindowPtr winWP;
	MyWindowPtr win;
	short tolerance;
	long mSelect;

	topWin = GetWindowMyWindowPtr(topWinWP);
	if (event->what == mouseUp) {
		upTicks = event->when;
		if (IsMyWindow(topWinWP) && topWin && topWin->pte)
			PeteEdit(topWin, topWin->pte, peeEvent, event);
		return;
	}

	dblTime = GetDblTime();
	tolerance = GetRLong(DOUBLE_TOLERANCE);

	/*
	 * figure out whether this is a double or triple click,
	 * and update double and triple click times
	 */
	if (event->when - upTicks < dblTime) {
		int dx = event->where.h - downSpot.h;
		int dy = event->where.v - downSpot.v;
		if (ABS(dx) < tolerance && ABS(dy) < tolerance
		    && !(event->modifiers & cmdKey)) {
			/* upgrade the click */
			ClickType++;
			if (ClickType > Triple)
				ClickType = Single;
		} else
			ClickType = Single;
	} else
		ClickType = Single;

	upTicks = event->when;
	downSpot = event->where;
	SFWTC = True;

	/*
	 * where was the click?
	 */
	wPart = FindWindow_(event->where, &winWP);
	win = GetWindowMyWindowPtr(winWP);
	UsingWindow(winWP);
#ifdef CTB
	if (CnH && winWP && (long) CnH == GetWindowPrivateData(winWP))
		CMEvent(CnH, event);
	else
#endif
	if (ModalWindow && winWP && winWP != ModalWindow)
		SysBeep(20L);
	else
		switch (wPart) {
		case inMenuBar:
			EnableMenuItems(False);
			SetMyCursor(arrowCursor);
			SFWTC = True;
			SetMenuTexts(event->modifiers, False);
			mSelect = MenuSelect(event->where);
			DoMenu(topWinWP, mSelect, event->modifiers);
			break;

		case inProxyIcon:
			if (win && win->proxy) {
				win->proxy(win, event);
				break;
			}
			// fall-through is deliberate
		case inDrag:
#ifdef	FLOAT_WIN
			if (!InBG && event->modifiers & cmdKey
			    && IsTopNonFloater(winWP)
			    && (IsBoxWindow(winWP) || IsMessWindow(winWP)))
#else				//FLOAT_WIN
			if (!InBG && event->modifiers & cmdKey
			    && winWP == topWinWP && (IsBoxWindow(winWP)
						     ||
						     IsMessWindow(winWP)))
#endif				//FLOAT_WIN
				PopupMailboxPath(win, nil, 0, downSpot);
			else {
				DragMyWindow(winWP, event->where);
#ifdef TWO
				if (IsMessWindow(winWP)
				    && ClickType == Double) {
					MyWindowPtr messTocWin =
					    (*(*Win2MessH(win))->tocH)->
					    win;
					WindowPtr messTocWinWP =
					    GetMyWindowWindowPtr
					    (messTocWin);
					ShowMyWindow(messTocWinWP);
					UserSelectWindow(messTocWinWP);
					SelectMessage((*Win2MessH(win))->
						      tocH,
						      (*Win2MessH(win))->
						      sumNum);
					if (event->modifiers & optionKey)
						BoxSelectSame((*Win2MessH
							       (win))->
							      tocH,
							      SORT_SUBJECT_ITEM,
							      (*Win2MessH
							       (win))->
							      sumNum);
				}
#endif
			}
			break;
		case inZoomIn:
		case inZoomOut:
			if (IsMyWindow(winWP))
				win->saveSize = True;
			ZoomMyWindow(winWP, event->where, wPart);
			break;
		case inGrow:
			if (!(GetWindowKind(winWP) == dialogKind ||
			      IsMyWindow(winWP) && win && win->isRunt)) {
				GrowMyWindow(win, event);
				break;
			}
			/* fall-through is deliberate */
		case inContent:
			DoContentClick(topWinWP, winWP, event);
			break;
		case inGoAway:
			if (event->modifiers & optionKey)
				CloseAll(False);
			else
				GoAwayMyWindow(winWP, event->where);
			break;
		}
}

/**********************************************************************
 * DoContentClick - handle a click in the content region of a window
 **********************************************************************/
void DoContentClick(WindowPtr topWinWP, WindowPtr winWP,
		    EventRecord * event)
{
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	PETEHandle pte;
	Point pt;

	// (jp) Ignore clicks if for some reason we don't have a MyWindowPtr
	if (!win)
		return;

#ifdef	FLOAT_WIN
	if (IsMyWindow(winWP) && IsFloating(winWP))
		SetKeyFocusedFloater(win);
	else {
		SetKeyFocusedFloater(nil);
#endif				//FLOAT_WIN
		if (winWP != topWinWP) {
			SetPort_(GetWindowPort(winWP));
			if (IsMyWindow(winWP) && WinBGDrag(win, event));	// leave it alone
			else if (IsMyWindow(winWP) && win->bgClick)
				(*win->bgClick) (win, event);
			else
				SelectWindow_(winWP);
			return;
		}
#ifdef	FLOAT_WIN
	}
#endif				//FLOAT_WIN

	SetPort_(GetWindowPort(winWP));
	pt = event->where;
	GlobalToLocal(&pt);
	if (!win->dontControl && HandleControl(pt, win));
	else if (IsMyWindow(winWP)) {
		if (!ClickWazoo(win, event, pt, nil))
			if (!ClickSponsorAd(win, event, pt)) {
				if (win->click)
					(*win->click) (win, event);
				else {
					// (jp) Figure out which PETE actually received the click
					for (pte = win->pteList; pte;
					     pte = PeteNext(pte))
						if (PtInPETE(pt, pte))
							break;
					if (PeteIsValid(pte)) {
						if (pte != win->pte)
							PeteSelect(win,
								   win->
								   pte, 0,
								   0);
						PeteEditWFocus(win, pte,
							       peeEvent,
							       (void *)
							       event, nil);
					}
				}
			}
	}
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean WinBGDrag(MyWindowPtr win, EventRecord * event)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	PETEHandle pte;
	Point where;
	Boolean result = False;

	PushGWorld();

	SetPort_(GetWindowPort(winWP));
	where = event->where;
	GlobalToLocal(&where);

	if (IsWazoo(winWP) && ClickWazoo(win, event, where, &result) && result);	// bg wazoo drag; all done now
	else {
		for (pte = win->pteList; pte; pte = PeteNext(pte)) {
			if (PtInPETE(where, pte)) {
				if (tsmDocNotActiveErr !=
				    PeteEdit(win, pte, peeEvent, event))
					result = True;
				break;
			}
		}
	}

	PopGWorld();
	return (result);
}

/************************************************************************
 * HandleControl
 ************************************************************************/
Boolean HandleControl(Point pt, MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	int part;
	int oldValue, difference;
	ControlHandle cntl;

	if (part = FindControl(pt, winWP, &cntl)) {
		// Not in scrollbar thumb
		if (part != kControlIndicatorPart) {
			part =
			    TrackControl(cntl, pt,
					 (ControlActionUPP) (-1));
			ScrollAction(cntl, 0);	// reset scroll throttle when mouse goes up
		}

		// Live scroll
		else if (GetControlVariant(cntl) != 0) {
			gLastCtlValue = GetControlValue(cntl);
			part =
			    TrackControl(cntl, pt,
					 NewControlActionUPP
					 (AMLiveScrollAction));
		}

		// Regular scroll
		else {
			oldValue = GetControlValue(cntl);
			part = TrackControl(cntl, pt, nil);
			difference = oldValue - GetControlValue(cntl);
			if (difference) {
				if (ScrollIsH(cntl))
					ScrollMyWindow(win, difference, 0);
				else
					ScrollMyWindow(win, 0, difference);
				UpdateMyWindow(winWP);
			}
		}
		if (part && win->button)
			(*win->button) (win, cntl, CurrentModifiers(),
					part);
		return (True);
	}
	return (False);
}

/**********************************************************************
 * DoUpdate - handle an update event
 **********************************************************************/
Boolean DoUpdate(WindowPtr topWin, EventRecord * event)
{
#pragma unused(topWin)
	WindowPtr theWindow;

	if (theWindow = Event2Window(event)) {
#ifdef CTB
		if (CnH && GetWindowPrivateData(theWindow) == (long) CnH) {
			CMEvent(CnH, event);
			return (False);
		}
#endif
		if (IsMyWindow(theWindow)) {
			UpdateMyWindow(theWindow);
			return (False);
		}
	}
	return (True);		/* somebody else better do this one */
}

StackHandle ContextStack;
typedef struct {
	GrafPtr port;
	short curResFile;
} SwitchContext;

/**********************************************************************
 * MightSwitch - protect from the vagaries of other apps
 **********************************************************************/
void MightSwitch(void)
{
	SwitchContext context;

	GetPort(&context.port);
	context.curResFile = CurResFile();
	if (!ContextStack)
		StackInit(sizeof(context), &ContextStack);
	if (!ContextStack)
		return;
	StackPush(&context, ContextStack);
	SetPort(InsurancePort);
}

/**********************************************************************
 * AfterSwitch - restore things after a switch back in
 **********************************************************************/
void AfterSwitch(void)
{
	SwitchContext context;

	if (!ContextStack)
		return;
	if (!StackPop(&context, ContextStack)) {
		SetPort(context.port);
		UseResFile(context.curResFile);
	}
}

/**********************************************************************
 * DoActivate - handle an activate event (and a prior deactivate, to boot)
 **********************************************************************/
Boolean DoActivate(WindowPtr topWin, EventRecord * event)
{
#pragma unused(topWin)
	WindowPtr theWindow;
	Boolean active = !InBG && (event->modifiers & activeFlag) != 0;
	Boolean yours = False;

	if (theWindow = Event2Window(event)) {
		UsingWindow(theWindow);
#ifdef CTB
		if (CnH && (long) CnH == GetWindowPrivateData(theWindow))
			CMActivate(CnH, active);
		else
#endif
		if (IsMyWindow(theWindow)) {
			if (!CorrectlyActivated(theWindow))
				ActivateMyWindow(theWindow, active != 0);	//(event->modifiers&activeFlag)!=0);
		} else
			yours = True;
	}
	SFWTC = True;
	return (yours);
}

/**********************************************************************
 * WarnUser - tell the user that something bad is happening
 **********************************************************************/
int WU(AlertType type, int errorStr, int errorNum, int file, int line)
{
	Str255 message;
	Str255 explanation;
	Str63 techStuff;
	AlertStdAlertParamRec param;
	UPtr sec;
	extern ModalFilterUPP DlgFilterUPP;
	short realSettingsRef = SettingsRefN;

	if (errorNum != userCancelled) {
		SettingsRefN = GetMainGlobalSettingsRefN();
		GetRString(message, errorStr);
		if (sec = PIndex(message, '�')) {
			*explanation = *message - (sec - message);
			BMD(sec + 1, explanation + 1, *explanation);
			*message -= *explanation + 1;
		} else if (errorNum == -108 && errorStr != MEM_ERR)
			GetRString(explanation, MEM_ERR);
		else
			*explanation = 0;

		if (errorNum)
			ComposeRString(techStuff, WU_FMT, errorNum, file,
				       line);
		else
			ComposeRString(techStuff, WU_NOERR_FMT, file,
				       line);
		PCat(explanation, techStuff);
		SettingsRefN = realSettingsRef;

		Zero(param);
		param.movable = True;
		param.filterProc = DlgFilterUPP;
		param.defaultButton = kAlertStdAlertOKButton;
		param.position = kWindowDefaultPosition;
		ReallyStandardAlert(type, message, explanation, &param);
	}
	return (errorNum);
}

/**********************************************************************
 * DieWithError - die with the given error message
 **********************************************************************/
void DWE(int errorStr, int errorNum, int file, int line)
{
	Str255 fatal;
	Str255 message;
	Str255 number;
	Str255 debugStr;

	GetRString(fatal, FATAL);
	GetRString(message, errorStr);
	NumToString((long) errorNum, number);
	ComposeRString(debugStr, FILE_LINE_FMT, file, line);
	MyParamText(fatal, message, number, debugStr);
	if (gAppearanceIsLoaded)
		ComposeStdAlert(kAlertStopAlert,
				ERR_ASTR + ALRTStringsStrn, P1, P2, P3,
				P4);
	else
		StopAlert(NON_AM_ERR_ALRT, nil);
	Cleanup();
	ExitToShell();
}

/**********************************************************************
 * DumpData - put up an alert that shows some data.
 **********************************************************************/
void DumpData(UPtr description, UPtr data, int length)
{
	Str255 asAscii;
	Str255 asHex;
	static char hex[] = "0123456789abcdef";
	char *ac, *hx;
	char *from;

	if (length > 255 / 2)
		length = 255 / 2;

	/*
	 * prepare display strings
	 */
	*asAscii = 2 * length;
	*asHex = 2 * length;
	ac = asAscii + 1;
	hx = asHex + 1;
	for (from = data; from < data + length; from++) {
		*ac++ = optSpace;
		if (*from == ' ')
			*ac++ = optSpace;
		else if (*from < ' ')
			*ac++ = '.';
		else
			*ac++ = *from;
		*hx++ = hex[((*from) >> 4) & 0xf];
		*hx++ = hex[(*from) & 0xf];
	}

	SetDialogFont(kFontIDMonaco);
	MyParamText(description, asAscii, asHex, "");
	switch (Alert(DUMP_ALRT, nil)) {
	case DEBUG_BUTTON:
		Debugger();
		break;
	case EXIT_BUTTON:
		Cleanup();
		ExitToShell();
		break;
	}
	SetDialogFont(applFont);
}

/**********************************************************************
 * monitor the heap situation
 **********************************************************************/
long MonitorGrow(Boolean report)
{
	static long memLeft = NSpare * SPARE_SIZE;
	long roomLeft;
	long roomAvailable;
	short i;
	long deficit = 0;

	if (!DoMonitor)
		return (0);

	roomLeft = 0;
	for (i = 0; i < NSpare; i++)
		if (SpareSpace[i])
			roomLeft += GetHandleSize_(SpareSpace[i]);

	if (roomLeft < memLeft) {
#ifdef LDAP_ENABLED
		MonitorLDAPCodeGrow(true);
#endif
		if (report
		    && (memLeft == NSpare * SPARE_SIZE
			|| roomLeft + 10 K < memLeft
			|| roomLeft < MEM_CRITICAL))
			MemoryWarning();
	}

	if (roomLeft < NSpare * SPARE_SIZE) {
		memLeft = roomLeft;
#ifdef LDAP_ENABLED
		MonitorLDAPCodeGrow(true);
#endif
		for (i = 0; i < sizeof(SpareSpace) / sizeof(Handle); i++) {
			ZapHandle(SpareSpace[i]);
			roomAvailable = CompactMem(SPARE_SIZE);
			if (roomAvailable < SPARE_SIZE)
				roomAvailable = MaxMem(&roomLeft);
			if (roomAvailable > SPARE_SIZE)
				roomAvailable = SPARE_SIZE;
			MakeGrow(roomAvailable, i);
			if (SpareSpace[i])
				deficit +=
				    SPARE_SIZE -
				    GetHandleSize_(SpareSpace[i]);
			else
				deficit += SPARE_SIZE;
		}
	}

#ifdef LDAP_ENABLED
	MonitorLDAPCodeGrow(false);
#endif
	if (MemLastFailed || deficit) {
		long total;
		PurgeSpace(&total, &MemLastFailed);
	}

	return (deficit);
}

/************************************************************************
 * MemoryWarning - warn the user about how much memory is left.
 ************************************************************************/
void MemoryWarning(void)
{
	Str255 partitionString;
	long currentSize, estSize, spareSize;
	short i;

#ifdef THREADING_ON
	/* just cancel what you're doing and set error flag */
	if (InAThread()) {
		TaskKindEnum taskKind = GetCurrentTaskKind();
		CommandPeriod = True;
		if (taskKind == CheckingTask)
			CheckThreadError = memFullErr;
		else if (taskKind == SendingTask)
			SendThreadError = memFullErr;
		return;
	}
#endif
	currentSize = CurrentSize();
	estSize = EstimatePartitionSize(false);

	spareSize = 0;
	for (i = 0; i < NSpare; i++)
		if (SpareSpace[i])
			spareSize += GetHandleSize_(SpareSpace[i]);
	estSize =
	    MAX(estSize, NSpare * SPARE_SIZE - spareSize + currentSize);
	(void) ComposeRString(partitionString, MEM_PARTITION,
			      currentSize / (1 K), estSize / (1 K));
	if (ComposeStdAlert
	    (Stop, MEMORY_ALRT, MEM_LOW, MEM_EXPLANATION,
	     partitionString) == MEMORY_QUIT)
		CommandPeriod = EjectBuckaroo = True;
}

/************************************************************************
 * CurrentSize - how big is our partition?
 ************************************************************************/
uLong CurrentSize(void)
{
	ProcessInfoRec pi;
	ProcessSerialNumber psn;
	uLong size = 0;

	pi.processInfoLength = sizeof(pi);
	pi.processName = nil;
	pi.processAppSpec = nil;
	if (!GetProcessInformation(CurrentPSN(&psn), &pi)) {
		return (pi.processSize - 44 K);	/* dunno where finder gets 44K, but... */
	} else
		return (DefaultSize());
}

/************************************************************************
 * DefaultSize - how big is Eudora's normal partition?
 ************************************************************************/
uLong DefaultSize(void)
{
	SizeHandle sizeH;
	uLong size = 0;

	if (sizeH = GetResource_('SIZE', -1)) {
		size = (*sizeH)->prefSize;
		ReleaseResource_(sizeH);
	}
	return (size);
}

/************************************************************************
 * EstimatePartitionSize - estimate the "right" size for Eudora
 ************************************************************************/
uLong EstimatePartitionSize(Boolean log)
{
	Str31 suffix;
	uLong drain = 0;
	static short strids[] = { IN, OUT, TRASH };
	short id;
	MyWindowPtr win;
	WindowPtr winWP;
	uLong dFork = 0;
	uLong size;
	FSSpec spec;
	short tocFactor = PrefIsSet(PREF_NO_RF_TOC_BACKUP) ? 1 : 2;
#ifdef DEBUG
	uLong oldDrain;
#endif

	GetRString(suffix, TOC_SUFFIX);
	spec.vRefNum = MailRoot.vRef;
	spec.parID = MailRoot.dirId;

	for (id = 0; id < sizeof(strids) / sizeof(short); id++) {
		GetRString(spec.name, strids[id]);
		if (!PrefIsSet(PREF_NEW_TOC) || !(size = FSpRFSize(&spec))) {
			PSCat(spec.name, suffix);
			size = tocFactor * FSpDFSize(&spec);
		}
		drain += size / tocFactor;
#ifdef DEBUG
		if (log)
			ComposeLogS(-1, nil, "\pMailbox: %p: %d",
				    spec.name, size / tocFactor);
#endif
	}

	/*
	 * nicknames
	 */
	drain += NickDrain() + ETLDrain();
#ifdef DEBUG
	if (log)
		ComposeLogS(-1, nil, "\pNicknames: %d", NickDrain());
	if (log)
		ComposeLogS(-1, nil, "\pEMSAPI: %d", ETLDrain());
#endif

	for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr(winWP);
#ifdef DEBUG
		oldDrain = drain;
#endif
		drain += sizeof(MyWindow) + 1 K + 5 K;	// 5K seems to cover appearance requirements, give or take
		switch (GetWindowKind(winWP)) {
		case COMP_WIN:
		case MESS_WIN:
			drain +=
			    PETEGetMemInfo(PETE,
					   (*Win2MessH(win))->bodyPTE);
			drain += (*Win2MessH(win))->extras.offset;
			drain += 7 K;	// lots of controls in those windows
			break;
		case TEXT_WIN:
			drain += PETEGetMemInfo(PETE, win->pte);
			break;
		case MBOX_WIN:
		case CBOX_WIN:
			if (!(*(TOCHandle) GetMyWindowPrivateData(win))->
			    which)
				drain +=
				    GetHandleSize_((TOCHandle)
						   GetMyWindowPrivateData
						   (win));
			break;
		default:
			drain += 5 K;
		}
#ifdef DEBUG
		if (log) {
			Str255 title;
			GetWTitle(winWP, title);
			ComposeLogS(-1, nil, "\pMailbox: %p: %d", title,
				    drain - oldDrain);
		}
#endif
	}

	if (IsPowerNoVM()) {
		FSSpec appSpec;

		if (!GetFileByRef(AppResFile, &appSpec)) {
			dFork = FSpDFSize(&appSpec);
			// Hack!  Hack!  Hack!
			// Fat version is way big, and ppc is about 5/9 of it.
			// The proper way to calculate this is to walk the cfrg resource,
			// but that looks more complicated than it's worth.  SD
			if (dFork > 3 K K)
#if TARGET_CPU_PPC
				dFork = (dFork * 5) / 9;
#else
				dFork = (dFork * 4) / 9;
#endif
		}
	}

#ifdef NEVER
	if (RunType != Production)
		drain += 2000 K;
#endif

	drain = MAX(0, drain - 1500 K);
	return (DefaultSize() + drain + dFork);
}

/**********************************************************************
 * NickDrain - compute drain due to nicknames
 **********************************************************************/
long NickDrain(void)
{
	Str31 name;
	FSSpec folderSpec;
	CInfoPBRec hfi;
	FSSpec spec;
	long drain = 0;

	/*
	 * add the Eudora Nicknames file
	 */
	FSMakeFSSpec(Root.vRef, Root.dirId, GetRString(name, ALIAS_FILE),
		     &spec);
	drain += FSpRFSize(&spec);

#ifdef TWO
	/*
	 * add the Eudora Nicknames Cache file
	 */
	FSMakeFSSpec(Root.vRef, Root.dirId,
		     GetRString(name, CACHE_ALIAS_FILE), &spec);
	drain += FSpRFSize(&spec);

#ifdef VCARD
	/*
	 * add the Personal Nicknames file
	 */
	if (HasFeature(featureVCard)) {
		FSMakeFSSpec(Root.vRef, Root.dirId,
			     GetRString(name, PERSONAL_ALIAS_FILE), &spec);
		drain += FSpRFSize(&spec);
	}
#endif
	/*
	 * is there a nicknames folder?
	 */
	if (!SubFolderSpec(NICK_FOLDER, &folderSpec)) {
		/*
		 * iterate through all the files, adding them to the list
		 */
		hfi.hFileInfo.ioNamePtr = name;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate
		       (folderSpec.vRefNum, folderSpec.parID, &hfi)) {
			SimpleMakeFSSpec(folderSpec.vRefNum,
					 folderSpec.parID, name, &spec);
			drain += FSpRFSize(&spec);
		}
	}
#endif
	return (drain);
}

/**********************************************************************
 * create some space for the grow zone function
 **********************************************************************/
void MakeGrow(long howMuch, short which)
{
	Boolean oldMonitor = DoMonitor;

	DoMonitor = False;
	if (SpareSpace[which] = NuHandle(howMuch))
		**(long **) SpareSpace[which] = 'SPAR';
	DoMonitor = oldMonitor;
}

/**********************************************************************
 * grow that zone...
 **********************************************************************/
pascal long MyGrowZone(unsigned long needed)
{
	long roomLeft;
	long freed = 0;
	long theA5 = SetCurrentA5();
	Handle dontMove = (Handle) GZSaveHnd();
	short resFile = CurResFile();
	short i;

	MemLastFailed = 1 K;

	if (SettingsRefN)
		UseResFile(SettingsRefN);
	if (!MemCanFail) {
		for (i = 0; i < NSpare; i++)
			if (dontMove != (Handle) SpareSpace[i]
			    && (roomLeft =
				InlineGetHandleSize((Handle)
						    SpareSpace[i]))) {
				freed =
				    needed <= roomLeft ? needed : roomLeft;
				SetHandleSize(SpareSpace[i],
					      roomLeft - freed);
			}
		if (!freed && DamagedTOC
		    && DamagedTOC != (TOCHandle) dontMove) {
			freed = InlineGetHandleSize((Handle) DamagedTOC);
			ZapHandle(DamagedTOC);
		}
	}
	UseResFile(resFile);

	(void) SetA5(theA5);
	return (freed);
}

/************************************************************************
 * CloseUnused - close unused windows
 ************************************************************************/
long CloseUnused(void)
{
	static short kinds[] =
	    { MB_WIN, PH_WIN, TEXT_WIN, ALIAS_WIN, MESS_WIN, COMP_WIN,
 FILT_WIN, PICT_WIN, PERS_WIN, SIG_WIN, STA_WIN, LINK_WIN };
	WindowPtr winWP, deadWinWP = nil;
	MyWindowPtr win;
	short kind;

	for (kind = 0; kind < sizeof(kinds) / sizeof(short); kind++) {
		for (winWP = FrontWindow(); winWP;
		     winWP = GetNextWindow(winWP)) {
			win = GetWindowMyWindowPtr(winWP);
			if (GetWindowKind(winWP) == kinds[kind] && win
			    && !IsDirtyWindow(win) && !win->inUse)
				if (kinds[kind] != MESS_WIN
				    || win->pte !=
				    (*Win2MessH(win))->subPTE)
					deadWinWP = winWP;
		}
		if (deadWinWP) {
			if (!CloseMyWindow(deadWinWP)) {
				UsingWindow(deadWinWP);	/* couldn't close it ??? */
				deadWinWP = nil;
				kind--;	/* try again with this kind */
			} else {
				return (1);
			}
		}
	}
	return (0);
}

/**********************************************************************
 * FileSystemError - report an error regarding the file system
 **********************************************************************/
int FSE(int context, UPtr name, int realErr, int file, int line)
{
	Str255 text;
	Str255 contextStr;
	Str63 debugStr;
	int offset = 0;
	AlertStdAlertParamRec param;
	extern ModalFilterUPP DlgFilterUPP;
	OSErr err = realErr;
	short realSettingsRef = SettingsRefN;

	if (err != userCancelled && err != userCanceledErr) {
		SettingsRefN = GetMainGlobalSettingsRefN();
		ComposeRString(text, FSE_NAME_ERR, name, err);
		if (err < wrPermErr)
			offset += wrPermErr - dirNFErr - 1;
		if (err < volGoneErr)
			offset += volGoneErr + 128;
		err += offset;
		err = dirFulErr - err + 2;
		if (err > 37)
			err = 37;
		GetRString(contextStr, FILE_STRN + err);
		PCat(text, contextStr);
		ComposeRString(debugStr, FILE_LINE_FMT, file, line);
		PCat(text, debugStr);
		GetRString(contextStr, context);
		SettingsRefN = realSettingsRef;

		Zero(param);
		param.movable = True;
		param.filterProc = DlgFilterUPP;
		param.defaultButton = kAlertStdAlertOKButton;
		param.position = kWindowDefaultPosition;
		ReallyStandardAlert(kAlertCautionAlert, contextStr, text,
				    &param);
	}
	return (realErr);
}

/************************************************************************
 * MiniMainLoop - handle only a limited set of events.	returns the event
 * if someone else needs to handle it
 ************************************************************************/
Boolean MiniMainLoopLo(EventRecord * event);
Boolean MiniMainLoop(EventRecord * event)
{
	Boolean result;
	PushGWorld();
	result = MiniMainLoopLo(event);
	PopGWorld();
	return (result);
}

Boolean MiniMainLoopLo(EventRecord * event)
{
	WindowPtr topWinWP = FrontWindow_();

	switch (event->what) {
#ifndef NO_KEYUP
	case keyUp:
#endif
	case keyDown:
		if ((event->modifiers & cmdKey) && IsCmdChar(event, '.') ||
		    ((event->message & charCodeMask) == escChar) &&
		    (((event->message & keyCodeMask) >> 8) == escKey)) {
			FlushEvents(keyDownMask, 0);	/* kill all downs if we have a command-period */
#ifdef SPEECH_ENABLED
			if (CancelSpeech())
				return (false);
#endif
			CommandPeriod = True;
			if (ProgressIsTop())
				PressStop();
			return (True);
		}
		break;
	case app4Evt:
		if (((event->message) >> 24) & 1 == 1)
			DoApp4(topWinWP, event);
		return (False);
	case updateEvt:
		DoUpdate(topWinWP, event);
		return (False);
		break;
	case activateEvt:
		DoActivate(topWinWP, event);
		return (False);
		break;
	case mouseDown:
		if (ModalWindow && ModalWindow == FrontWindow_())
			DoMouseDown(ModalWindow, event);
		break;
	}


#ifdef CTB
	if (CnH && topWinWP
	    && GetWindowPrivateData(topWinWP) == (long) CnH) {
		CMEvent(CnH, event);
		return (False);
	}
#endif

#ifdef NO_KEYUP
	if (HasCommandPeriod())
		return (True);
#endif

	return (False);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean HasCommandPeriod(void)
{
	if (InAThread())
		return false;

	if (!InBG) {
		if (CheckEventQueueForUserCancel()) {
			FlushEvents(keyDownMask | keyUpMask | mDownMask | mUpMask, 0);	/* kill events on cmd-period */
			if (CancelSpeech())
				return false;
			CommandPeriod = True;
			if (ProgressIsTop())
				PressStop();
			return true;
		}
	}
	return false;
}


/************************************************************************
 *
 ************************************************************************/
void FlushHIQ(void)
{
	HostInfoQHandle hiq, nextHIQ;
	for (hiq = HIQ; hiq; hiq = nextHIQ) {
		nextHIQ = (*hiq)->next;
		if ((*hiq)->done && !(*hiq)->inUse) {
			LL_Remove(HIQ, hiq, (HostInfoQHandle));
			ZapHandle(hiq);
		}
	}
}

/************************************************************************
 * TendNotificationManager - see if an NM rec is active, and if it should
 * be taken down
 ************************************************************************/
void TendNotificationManager(Boolean isActive)
{
	if (MyNMRec) {
		long ticksSince = TickCount() - MyNMRec->nmRefCon;
		if (isActive && ticksSince > 10) {
			NMRemove(MyNMRec);
			if (MyNMRec->nmIcon)
				DisposeIconSuite(MyNMRec->nmIcon, True);
			ZapPtr(MyNMRec->nmStr);
			ZapPtr(MyNMRec);
		}
	}

	if (isActive)
		AttentionNeeded = false;
}

/**********************************************************************
 * NuHandle - allocate a handle
 **********************************************************************/
void *NuHandle(long size)
{
	void *h;

	RANDOM_FAILURE;

#ifdef NEVER
	if (h = GetResource(POPD_TYPE, POPD_ID))
		ASSERT(!(GetResAttrs(h) & resChanged)
		       || !(HGetState(h) & 0x40));
#endif

#ifdef NEVER
	if (MonitorGrow(False) > 0) {
#ifdef DEBUG
		if (RunType != Production) {
			SysBeep(10);
			DebugStr("\p;sc;g");
		}
#endif
#ifdef DEBUG
		if (RunType == Debugging || size > 100)
#endif
		{
			LMSetMemErr(-108);
			return (nil);
		}
	}
#endif

	MemCanFail = True;
	h = NewHandle(size);
	MemCanFail = False;
	if (!h && MemLastFailed == 1 K || MemLastFailed > size)
		MemLastFailed = MIN(size, 1 K);
	return (h);
}

/**********************************************************************
 * NuPtr - allocate a pointer
 **********************************************************************/
void *NuPtr(long size)
{
	void *h;

	RANDOM_FAILURE;

	MemCanFail = True;
	MonitorGrow(False);
	h = NewPtr(size);
	MemCanFail = False;
	if (!h && MemLastFailed == 1 K || MemLastFailed > size)
		MemLastFailed = MIN(size, 1 K);
	return (h);
}

/**********************************************************************
 * NuPtrClear - allocate a zeroed pointer
 **********************************************************************/
void *NuPtrClear(long size)
{
	void *h;

	RANDOM_FAILURE;

	MemCanFail = True;
	MonitorGrow(False);
	h = NewPtrClear(size);
	MemCanFail = False;
	return (h);
}


/**********************************************************************
 * get an event, giving time to system tasks as necessary
 **********************************************************************/
Boolean GrabEvent(EventRecord * theEvent)
{
	Boolean result;

	/*
	 * get the event
	 */
#ifndef NO_KEYUP
	FlushEvents(keyUpMask, 0);
#endif
	DragFxxkOff = False;
	result =
	    WNE(everyEvent, theEvent,
		InBG ? 300L : ((Typing || ActiveSearchCount) ? 0 : 10L));
	DragFxxkOff = theEvent->what != nullEvent;

	// make sure the rest of Eudora knows if we're typing
	if ((theEvent->what == autoKey || theEvent->what == keyDown))
		if (theEvent->modifiers & cmdKey)
			TypingTicks = 0;
		else
			TypingTicks = TickCount();

	Typing = TickCount() - TypingTicks < GetRLong(TYPING_THRESH);
	TypingRecently = Typing
	    || TickCount() - TypingTicks <
	    GetRLong(TYPING_RECENTLY_THRESH);

	/*
	 * handle special keys
	 */
	if (theEvent->what == keyDown || theEvent->what == autoKey)
		SpecialKeys(theEvent);

	/*
	 * global type-2-select buffer
	 */
	Type2Select(theEvent);

	/*
	 * gd dialog manager
	 */
	if (!result && MyIsDialogEvent(theEvent)) {
		Typing = False;
		DoModelessEvent(GetDialogFromWindow(FrontWindow_()),
				theEvent);
	}

	return (result);
}

/**********************************************************************
 * PantyTrack - handle drags
 **********************************************************************/
pascal OSErr PantyTrack(DragTrackingMessage message, WindowPtr qWinWP,
			Ptr handlerRfCon, DragReference drag)
{
	OSErr err = dragNotAcceptedErr;
	MyWindowPtr qWin = GetWindowMyWindowPtr(qWinWP);

	if (DragFxxkOff)
		return (dragNotAcceptedErr);

	Dragging++;
	if (IsMyWindow(qWinWP)) {
		if (!DraggingWazoo(qWin, message, drag, &err))
			if (qWin->drag)
				err = (*qWin->drag) (qWin, message, drag);
		if (err == dragNotAcceptedErr)
			err = PeteDrag(qWin, message, drag);
		else if (!qWin->butch)
			PeteDrag(nil, message, drag);
	} else if (IsPlugwindow(qWinWP))
		err = PlugwindowDrag(qWinWP, message, drag);
	else
		err = PeteDrag(nil, message, drag);
	Dragging--;

	return (err == kAcceptableGraphicDrag ? noErr : err);
}

pascal OSErr PantyReceive(WindowPtr qWin, Ptr handlerRfCon,
			  DragReference drag)
{
	OSErr err;
	DECLARE_UPP(PantyTrack, DragTrackingHandler);

	INIT_UPP(PantyTrack, DragTrackingHandler);
	Dragging++;
	err = PantyTrack(0x0fff, qWin, handlerRfCon, drag);
	Dragging--;
	return (err == kAcceptableGraphicDrag ? noErr : err);
}

/************************************************************************
 * WNE - call WaitNextEvent.	Make sure not to call from thread.
 ************************************************************************/
Boolean WNE(short eventMask, EventRecord * event, long sleep)
{
	EventRecord localEvt;
	Boolean result;
#ifdef THREADING_ON
	static Boolean lastResult;
#endif

#ifdef THREADING_ON
	if (InAThread()) {
		MyYieldToAnyThread();
		return false;
	} else if (!lastResult && !Typing)
		MyYieldToAnyThread();
#endif

#ifdef THREADING_ON
	result =
	    WNELo(eventMask, &localEvt,
		  GetNumBackgroundThreads()? 0 : sleep);
#else
	result = WNELo(eventMask, &localEvt, sleep);
#endif

#ifdef THREADING_ON
	lastResult = result;
#endif

	ThreadYieldTicks = TickCount();

	result = FMBEventFilter(&localEvt, result);
	if (ToolbarShowing())
		result = TBEventFilter(&localEvt, result);
	if (PETE)
		PETEEventFilter(PETE, &localEvt);
	if (!PlugwindowEventFilter(&localEvt))
		return (false);
#ifdef TWO			// frontier
	if (localEvt.what == keyDown && localEvt.modifiers & cmdKey &&
	    (localEvt.message & charCodeMask) == '.'
	    && SharedScriptRunning()) {
		CancelSharedScript();
		return (False);
	}
#endif

	if (event)
		*event = localEvt;

	return (result);
}

/************************************************************************
 * WNELo - call WaitNextEvent and do low-level processing
 ************************************************************************/
Boolean WNELo(short eventMask, EventRecord * event, long sleep)
{
	EventRecord localEvt;
	Boolean result;
	ProcessSerialNumber me, him;
	static Boolean mask;
	Boolean foreground;
#if __profile__
	//short profilerWas = ProfilerGetStatus();
#endif
	static short oldmods;

	if (eventMask && event && !sleep)
		sleep = 4;
#if __profile__
	//ProfilerSetStatus(False);
#endif

	MightSwitch();

#ifndef DRAG_GETOSEVT
	if (Dragging) {
		result = False;
		Zero(localEvt);
		localEvt.what = nullEvent;
		localEvt.when = TickCount();
	}
#else
	if (Dragging) {
		result = GetNextEvent(eventMask, &localEvt);
		if (!result)
			UpdateAWindow();
	}
#endif
	else
		result =
		    WaitNextEvent(eventMask, &localEvt, sleep, MousePen);

	if (localEvt.what != nullEvent)
		NonNullTicks = TickCount();

#define HaveGetLastActivity ((GestaltBits(gestaltPowerMgrAttr)&(1<<gestaltPMgrDispatchExists))!=0)
#ifdef HAVE_GETLASTACTIVITY
	if (localEvt.what == mouseDown || localEvt.what == keyDown
	    || localEvt.what == keyUp || localEvt.what == autoKey) {
		if (HaveGetLastActivity)
			GlobalIdleTicks = 0;
	} else {
		ActivityInfo info;
		OSErr err;

		if (HaveGetLastActivity) {
			info.ActivityTime = 0;
			info.ActivityType = UsrActivity;
			if (!(err = GetLastActivity(&info)))
				GlobalIdleTicks =
				    TickCount() - info.ActivityTime;
		}
	}
#endif				//HAVE_GETLASTACTIVITY

	//if (RunType!=Production && localEvt.what) Dprintf("\p%d %x %x;g",localEvt.what,localEvt.message,localEvt.modifiers);
#if __profile__
	//ProfilerSetStatus(profilerWas);
#endif
	AfterSwitch();
	YieldTicks = TickCount();

	if (oldmods != localEvt.modifiers) {
		oldmods = localEvt.modifiers;
		SFWTC = True;
		NonNullTicks = TickCount();
	}

	GetCurrentProcess(&me);
	GetFrontProcess(&him);
	SameProcess(&me, &him, &foreground);
	InBG = !foreground;
	NoInitialCheck = NoInitialCheck || (localEvt.what != keyDown
					    && !InBG
					    && (oldmods & shiftKey) != 0);

#ifndef NO_KEYUP
	if (!InBG && !mask) {
		mask = True;
		SetEventMask(LMGetSysEvtMask() | keyUpMask);
	}
#endif

	if (event)
		*event = localEvt;

	return (result);
}

/**********************************************************************
 * NeedYield
 **********************************************************************/
Boolean NeedYield(void)
{
	short numThreads = GetNumBackgroundThreads();
	long elapsedTicks = TickCount() - ThreadYieldTicks;
	long vFactor =
	    (InBG ? 1 : ((VicomIs && VicomFactor > 0) ? VicomFactor : 1));
	long yieldInterval =
	    (InBG ? BgYieldInterval : FgYieldInterval) /
	    ((numThreads ? numThreads : 1) * vFactor);

#ifdef HAVE_GETLASTACTIVITY
	if (GlobalIdleTicks < 60)
		yieldInterval = 0;
	else if (GlobalIdleTicks < 120)
		yieldInterval /= 2;
#endif				//HAVE_GETLASTACTIVITY

//      ASSERT(InBG ? (elapsedTicks < 120) : 1); // are we hogging time somewhere?
	if (numThreads && !Typing)
		ToolbarIdleControls();
	if (EventPending() || (elapsedTicks > yieldInterval))
		return true;
	return false;
}

#ifdef DRAG_GETOSEVT
/************************************************************************
 * UpdateAWindow - update a window
 ************************************************************************/
void UpdateAWindow(void)
{
	WindowPtr theWindow;

	for (theWindow = FrontWindow(); theWindow;
	     theWindow = GetNextWindow(theWindow)) {
		if (IsKnownWindowMyWindow(theWindow)
		    && HasUpdateRgn(theWindow)) {
			UpdateMyWindow(theWindow);
			break;
		}
	}
}
#endif

#ifdef DEBUG

#define MemCorrupt(map,offset,string) MemCorruptLo(map,offset,string,__LINE__)
void MemCorruptLo(UHandle map, long offset, PStr msg, short line);
#define AtOffset(map,offset,type)	*(type*)(*map+offset)

/**********************************************************************
 * CheckHandle - check a handle for validity
 **********************************************************************/
void CheckHandle(UHandle map, long offset, long size, THz hz, PStr string)
{
	Handle h = *(Handle *) (*map + offset);
	OSErr err = noErr;

	/*
	 * in range?
	 */
	if (offset + 3 > GetHandleSize(map)) {
		MemCorrupt(map, offset, string);
		return;
	}

	if (!h)
		return;		/* nil ok */

	/*
	 * handle a multiple of four?
	 */
	if ((uLong) h % 4) {
		MemCorrupt(map, offset, string);
		return;
	}

	/*
	 * handle in the zone?
	 */
	if ((uLong) h < (uLong) & hz->heapData
	    || (uLong) h > (uLong) hz->bkLim) {
		MemCorrupt(map, offset, string);
		return;
	}

	if (!*h)
		return;		/* purged ok */

	/*
	 * pointer a mult of four?
	 */
	if ((uLong) * h % 4) {
		MemCorrupt(map, offset, string);
		return;
	}

	/*
	 * pointer in the zone?
	 */
	if ((uLong) * h < (uLong) & hz->heapData
	    || (uLong) * h > (uLong) hz->bkLim) {
		MemCorrupt(map, offset, string);
		return;
	}

	/*
	 * size correct?
	 */
	if (size >= 0 && GetHandleSize(h) != size) {
		MemCorrupt(map, offset, string);
		return;
	}

	/*
	 * handle looks ok to me
	 */
	return;
}

/**********************************************************************
 * 
 **********************************************************************/
void MemCorruptLo(UHandle map, long offset, PStr msg, short line)
{
	Str255 db;
	Str255 scratch;

	/*
	 * turn on logging
	 */
	DebugStr("\p;log MemCorrupt;g");

	/*
	 * display line # & message
	 */
	NumToString(line, scratch);
	PCopy(db, scratch);
	PCatC(db, ' ');
	PCat(db, msg);
	PCat(db, "\p;");

	/*
	 * display area around the offending data
	 */
	PCat(db, "\pdm #");
	NumToString((uLong) * map + offset, scratch);
	PCat(db, scratch);
	PCat(db, "\p-20 40;#");

	/*
	 * print the address itself
	 */
	NumToString((uLong) * map + offset, scratch);
	PCat(db, scratch);

	/*
	 * turn off logging
	 */
	PCat(db, "\p;log");

	/*
	 * now do it
	 */
	DebugStr(db);
}
#endif

#if 0				// CK -- no wheel in 7.5.5, maybe no carbon

/**********************************************************************
 * SetupEventHandlers - setup carbon event handlers
 **********************************************************************/
static void SetupEventHandlers(void)
{
	// Scroll wheel support
	EventTypeSpec wheelEventSpec =
	    { kEventClassMouse, kEventMouseWheelMoved };
	// Services menu support
	EventTypeSpec servicesEventSpec[] = {
		{ kEventClassService, kEventServiceCopy },
		{ kEventClassService, kEventServicePaste },
		{ kEventClassService, kEventServiceGetTypes }
	};
	// Application target
	EventTargetRef appTarget = GetApplicationEventTarget();

	// Scroll wheel handler
	InstallEventHandler(appTarget,
			    NewEventHandlerUPP(WheelHandlerProc),
			    sizeof(wheelEventSpec) / sizeof(EventTypeSpec),
			    &wheelEventSpec, nil, nil);

	// Services menu handler
	if (!PrefIsSet(PREF_NO_SERVICES))
		InstallEventHandler(appTarget,
				    NewEventHandlerUPP
				    (ServicesHandlerProc),
				    sizeof(servicesEventSpec) /
				    sizeof(EventTypeSpec),
				    &servicesEventSpec, nil, nil);
}


/**********************************************************************
 * WheelHandlerProc - carbon event handler for scroll wheel
 **********************************************************************/
static pascal OSStatus WheelHandlerProc(EventHandlerCallRef nextHandler,
					EventRef event, void *data)
{
#pragma unused(nextHandler,data)
	if (++gEnterWheelHandlerCount == 1) {
		EventMouseWheelAxis axis;
		SInt32 deltaH = 0, deltaV = 0;
		WindowPtr winWP;
		MyWindowPtr win;
		Boolean vertical;
		short lines;
		PETEHandle pte;
		HIPoint hiPt;
		Point pt;

		// Find window mouse is over
		GetEventParameter(event, kEventParamMouseLocation,
				  typeHIPoint, nil, sizeof(hiPt), nil,
				  &hiPt);
		pt.h = hiPt.x;
		pt.v = hiPt.y;
		FindWindow(pt, &winWP);
		win = GetWindowMyWindowPtr(winWP);

		if (win) {
			GetEventParameter(event, kEventParamMouseWheelAxis,
					  typeMouseWheelAxis, nil,
					  sizeof(axis), nil, &axis);
			vertical = axis == kEventMouseWheelAxisY;
			GetEventParameter(event,
					  kEventParamMouseWheelDelta,
					  typeLongInteger, nil,
					  sizeof(deltaH), nil,
					  vertical ? &deltaV : &deltaH);

			lines = GetRLong(SCROLL_WHEEL_LINES);
			if (lines > 0 && lines <= 100) {
				deltaH *= lines;
				deltaV *= lines;
			}

			// See if mouse is over a pte
			SetPort(GetWindowPort(winWP));
			GlobalToLocal(&pt);
			for (pte = win->pteList; pte; pte = PeteNext(pte)) {
				Rect rPTE;

				PeteRect(pte, &rPTE);
				if (PtInRect(pt, &rPTE))
					break;	// Found one!
			}

			if (pte) {
				short n, i;

				n = deltaV >= 0 ? deltaV : -deltaV;
				for (i = n; i--;)
					PeteScroll(pte, 0,
						   deltaV >
						   0 ? pseLineUp :
						   pseLineDn);
				if (IsBoxWindow(winWP)) {
					TOCHandle tocH = Win2TOC(win);
					if (pte == (*tocH)->previewPTE
					    && PreviewReadFocus)
						BeenThereDoneThat(tocH,
								  -1);
				}
			} else if (win->pView && deltaV
				   && PtInRect(pt, &win->pView->bounds))
				LVScroll(win->pView, deltaV);
			else if (win->scrollWheel
				 && (win->scrollWheel) (win, pt, deltaH,
							deltaV));
			else
				ScrollIt(win, deltaH, deltaV);
		}
	}
	gEnterWheelHandlerCount--;
	return noErr;
}
#endif

/**********************************************************************
 * AppendOSTypeToCFArray - add data type to CF array as CF string
 **********************************************************************/
static void AppendOSTypeToCFArray(CFMutableArrayRef arrayRef, OSType type)
{
	CFStringRef typeString = CreateTypeStringWithOSType(type);
	if (typeString) {
		CFArrayAppendValue(arrayRef, typeString);
		CFRelease(typeString);
	}
}

/**********************************************************************
 * ServicesHandlerProc - carbon event handler for Services menu
 **********************************************************************/
static pascal OSStatus ServicesHandlerProc(EventHandlerCallRef nextHandler,
					   EventRef event, void *data)
{
#pragma unused(nextHandler,data)
	WindowPtr winWP = FrontWindow_();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	OSStatus err = noErr;

	if (win && win->pte) {
		UInt32 eventKind = GetEventKind(event);
		long start, stop;
		Boolean selection;
		CFMutableArrayRef copyTypes, pasteTypes;
		ScrapRef serviceScrap;

		selection =
		    !PeteGetTextAndSelection(win->pte, nil, &start, &stop)
		    && stop != start;
		switch (eventKind) {
#if 0				// CK can't find this anywhere
		case kEventServiceGetTypes:
			// Add the data types that we support to the copy/paste data-type arrays
			if (selection) {
				// Do copy only if something is selected
				if (!GetEventParameter
				    (event, kEventParamServiceCopyTypes,
				     typeCFMutableArrayRef, NULL,
				     sizeof(CFMutableArrayRef), NULL,
				     &copyTypes)) {
					// We copy only text
					AppendOSTypeToCFArray(copyTypes,
							      kScrapFlavorTypeText);
				}
			}
			if (!GetEventParameter
			    (event, kEventParamServicePasteTypes,
			     typeCFMutableArrayRef, NULL,
			     sizeof(CFMutableArrayRef), NULL,
			     &pasteTypes)) {
				// We paste text, styl, or PICT
				AppendOSTypeToCFArray(pasteTypes,
						      kScrapFlavorTypeText);
				AppendOSTypeToCFArray(pasteTypes,
						      kScrapFlavorTypeTextStyle);
				AppendOSTypeToCFArray(pasteTypes,
						      kScrapFlavorTypePicture);
				AppendOSTypeToCFArray(pasteTypes, 'RTF ');
				AppendOSTypeToCFArray(pasteTypes, 'TIFF');
			}
			break;

		case kEventServiceCopy:
			if (!selection)
				//      Nothing to copy
				break;
			// Fall through
		case kEventServicePaste:
			// Get the service scrap
			if (!GetEventParameter
			    (event, kEventParamScrapRef, typeScrapRef,
			     NULL, sizeof(ScrapRef), NULL,
			     &serviceScrap)) {
				PETEUseScrap(PETE, serviceScrap);
				// Make sure it's not locked. We allow text services to change
				// read-only messages.
				if (eventKind == kEventServicePaste)
					PeteLock(win->pte,
						 kPETECurrentSelection,
						 kPETECurrentSelection,
						 peNoLock);
				err =
				    PeteEdit(win, win->pte,
					     eventKind ==
					     kEventServiceCopy ? peeCopy :
					     peePaste, &MainEvent);
				PETEUseScrap(PETE, nil);
			}
			break;
#endif
		}
	}
	return err;
}

#pragma segment MacOS
#pragma segment StdC
#pragma segment Hesiod
#pragma segment MiscLibs
#pragma segment ICGlue
