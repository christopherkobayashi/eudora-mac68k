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

#include "mailxfer.h"
#include "myssl.h"
#define FILE_NUM 52
/* Copyright (c) 1993 by QUALCOMM Incorporated */

#pragma segment Mailxfer
	void NewMailSound(void);
	short CheckForMail(TransStream stream,short *gotSome,XferFlags *flags);
	short SendTheQueue(TransStream stream,XferFlags flags);
	void ResetCheckTime(Boolean force);
OSErr SpecialXfer(struct XferFlags *flags);
OSErr POPHostLimit(void);
short XferMailLo(Boolean check, Boolean send, Boolean manual, XferFlags flags,long *totalGot,OSErr *dialErrPtr);
Boolean NeedPassword(Boolean check, Boolean send);
void ResetPersCheckTime(Boolean force);
#ifdef THREADING_ON
Boolean OKToThread(Boolean check, Boolean send, Boolean manual, Boolean ae);
long FindTotalQueuedSize(TOCHandle tocH,long gmtSecs);
Boolean AddSigIntro(PETEHandle pte,UHandle text);
Boolean RemoveSigIntro(PETEHandle pte,UHandle text);
pascal Boolean SpecialXferFilter(DialogPtr dgPtr,EventRecord *event,short *item);
PersHandle SMTPRelayPers(void);
OSErr RememberMID(uLong midHash);
long GlobalOpenUnreadCount();
long GlobalInUnreadCount();

/************************************************************************
 * OKToThread - returns false if threading should be used because of
 *	transport limitations
 *
 *	(a) Do not thread if the connection method is the CTB.  The CTB
 *		supports one and only one connection at a time.
 *
 *	(b) Do not thread if any personality is set to uucpOut and is allowed to 
 *		send mail.
 *
 *	(c) Do not thread if any personality is set to uucpIn and is allowed to 
 *		check mail manually or automatically.
 *
 ************************************************************************/
Boolean OKToThread(Boolean check, Boolean send, Boolean manual, Boolean ae)
{
	Boolean threadOK = true;
//	Str255 pass;
				
	if (UseCTB) 
		threadOK = false;
// go ahead and thread uucp stuff
#if 0
	else
	{
		for (CurPers=PersList;CurPers&&threadOK;CurPers=(*CurPers)->next)
		{	
			if ((*CurPers)->uupcOut || (*CurPers)->uupcIn)
			{
				(*CurPers)->checkMeNow = (*CurPers)->sendMeNow = (*CurPers)->checked = 0;
				if (check && *GetPref(pass,PREF_POP))
				{
					if ((manual||ae) && !PrefIsSet(PREF_JUST_SAY_NO))
						(*CurPers)->checkMeNow = True;
				}
				if (send && (*CurPers)->sendQueue && !PrefIsSet(PREF_PERS_NO_SEND))
					(*CurPers)->sendMeNow = True;
				if ((*CurPers)->uupcOut && (*CurPers)->checkMeNow)
					threadOK = false;
				
				if ((*CurPers)->uupcIn && ((*CurPers)->sendMeNow || (*CurPers)->autoCheck))
					threadOK = false;
			}
		}
	}
#endif
	return (threadOK);
}
#endif	//THREADING_ON

/************************************************************************
 * XferMail - do the right thing
 * 						if thread is true, try to do it
 *
 *  To support separate send and check threads:
 *
 *  - don't allow more than one check thread at a time
 *  - don't allow more than one send thread at a time
 *  - if user tries to send while a send thread is running, queue the request up (via SendImmediately flag) 
 *    	and let the check go
 *  - before sending, set the SendQueue. if it's zero, don't send and reset SendImmediately flag
 *
 ************************************************************************/


short XferMail(Boolean check, Boolean send, Boolean manual,Boolean ae,Boolean thread,short modifiers)
{
	XferFlags flags;
	OSErr err = noErr;
	PersHandle pers;
	Boolean persSend = false;
	Boolean didWeUseMultiplePersonalities = false;

		// archive the junk mailbox?  Only if we're desperate
	if (JunkPrefBoxArchive() && JunkTrimOK()) ArchiveJunk(GetJunkTOC());

	// Cmd-Shift-M resyncs frontmost IMAP mailbox.
	if (!PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD))
		if (!(modifiers&optionKey) && (modifiers&shiftKey) && ResyncCurrentIMAPMailbox()) 
			return (noErr);
	
	// Don't allow more than one checkmail or sendmail thread
	if ((CheckThreadRunning && check) || (PrefIsSet(PREF_POP_SEND) && SendThreadRunning))
		return noErr;
	
	// if "send when check" pref set, spawn the check and send later	
	if ((SendThreadRunning && send) || (PrefIsSet(PREF_POP_SEND) && CheckThreadRunning))
	{
		SendImmediately = true; // = SendImmediately || send;
		if (check)
			send = false;
		else
		return noErr;
	}
	
	// clear the subfolder cache; as good a place as any
	SubFolderSpec(0,nil);
  
	if (err=XferMailSetup (&check, &send, manual, ae, &flags, modifiers))
		return err;
	
	// no need in spawning send thread if none of the queued messages belong to a personality 
	// marked for sending whenever sends are done	
	for (pers=PersList;pers;pers=(*pers)->next)
		if ((*pers)->sendQueue && (*pers)->sendMeNow)
			persSend = true;
	if (!SendQueue || !persSend)
	{
		send = false;
		SendImmediately = false;
	}
	if (check)
	{
		GetDateTime(&LastCheckTime);
		TaskProgressRefresh();
	}
	if (!(check || send))
		return noErr;

	FlushTOCs(True,True);			/* flush unnecessary TOC's */
	RememberOpenWindows();		/* for good measure */
	
	ActiveTicks = 0;	// we're pretending; manual check mail should not wait for idle time to filter

	// fetch any mailbox lists we need for this mailcheck
	for (pers=PersList;pers;pers=(*pers)->next)
	{
		PersHandle oldPers = CurPers;

		if ((*pers)->checkMeNow)
		{		
			CurPers = pers;
			if (PrefIsSet(PREF_IS_IMAP))
			{
				if (!MailboxTreeGood(CurPers))
				{
					if (CreateLocalCache() == noErr)
						EnsureSpecialMailboxes(CurPers);
				}	
			}
			CurPers = oldPers;
		}
	}			

#ifdef THREADING_ON
	if (PrefIsSet(PREF_THREADING_OFF) || !OKToThread(check, send, manual, ae) || !ThreadsAvailable() || !thread)
		err = XferMailRun (check, send, manual, ae, flags, nil);
	else 
		err = SetupXferMailThread (check, send, manual, ae, flags, nil);
#else
	err = XferMailRun (check, send, manual, ae);
#endif
	if (send) SetSendQueue();	// redo sendqueue if need be
	SendImmediately = false;	// clear this for next time around

#ifdef NAG
	if (check && nagState)
		CheckNagging ((*nagState)->state);
#endif

#ifdef AD_WINDOW
	if (check && IsAdwareMode())
		AdCheckingMail();
#endif
	return(err);
}

/************************************************************************
 * XferMailSetup - 
 *	12-8-98 JDB
 *	The check and send parameters may be modified by the SpecialXfer
 *	dialog, in which case the caller should know not to check/send.
 ************************************************************************/
short XferMailSetup(Boolean *check, Boolean *send, Boolean manual,Boolean ae, XferFlags *xFlags, short modifiers)
{
	PersHandle oldCur = CurPers;
	XferFlags flags;
	Boolean special = 0!=(modifiers&optionKey);
	Str255 pass;
	uLong ticks, ivalTicks;
	PersHandle pers;
	
	CHECK_EXPIRE;
	 
	CHECK_DEMO;		
	
	if (Offline && GoOnline()) return(OFFLINE);
	
	if (*send)	// 5/15/97 ccw -- fixes "Send queued messages" deactive menu and timed mesg bugs
	{
		SetSendQueue();
		ETLIdle(EMSFIDLE_PRE_SEND);
	}
	
	//PeteSetRudeColor();

	Zero(flags);
	flags.check = flags.servFetch = flags.servDel = *check;
	flags.send = *send;
	//flags.nuke = check && !PrefIsSet(PREF_LMOS);
	flags.isAuto = *check && !(manual||ae);
	
	CheckNow = False;	// clear flag
	
	if (!SelectXferMailPers(*check, *send, manual))	//	Set if checking/sending from Personalities window
	{
		ticks = TickCount() + TICKS2MINS * GetRLong(PERS_CHECK_SLOP);
		for (pers=PersList;pers;pers=(*pers)->next)
		{
			CurPers = pers;
			(*CurPers)->doMeNow = (*CurPers)->checkMeNow = (*CurPers)->sendMeNow = (*CurPers)->checked = 0;
			if (*check && *GetPOPPref(pass))
			{
				ivalTicks = (*CurPers)->autoCheck ? (*CurPers)->ivalTicks : 0;
				if (ivalTicks && (!(*CurPers)->checkTicks || (*CurPers)->checkTicks+ivalTicks<ticks))
					(*CurPers)->doMeNow = (*CurPers)->checkMeNow = True;
				else if ((manual||ae) && !PrefIsSet(PREF_JUST_SAY_NO))
					(*CurPers)->doMeNow = (*CurPers)->checkMeNow = True;
			}
			
			if (*send && (*CurPers)->sendQueue && !PrefIsSet(PREF_PERS_NO_SEND))
				(*CurPers)->doMeNow = (*CurPers)->sendMeNow = True;
			ASSERT(CurPers==pers);
		}
	}
	
	if (manual&&!ae && special && SpecialXfer(&flags)) 
	{
		CurPers = oldCur;
		return(userCancelled);
	}
	
	*send = flags.send;
	*check = flags.check||flags.servFetch||flags.servDel||flags.nuke||flags.nukeHard||flags.stub;
	
	if (!*send && !*check) 
	{
		CurPers = oldCur;
		return(userCancelled);
	}
	
	// collect passwords
	for (pers=PersList;pers;pers=(*pers)->next)
	{
		CurPers = pers;
		
		// Check for checking mail first
		if (NeedPassword(*check && (*CurPers)->checkMeNow,false))
		{
			if (PersFillPw(CurPers,0)!=noErr)
			{
				ResetCheckTime(True);
				CurPers = oldCur;
				return(1);
			}
		}
		// Double check that all the special things IMAP needs are in place
		if (*check && (*CurPers)->checkMeNow && PrefIsSet(PREF_IS_IMAP) && !EnsureSpecialMailboxes(CurPers))
		{
			ResetCheckTime(True);
			CurPers = oldCur;
			return(1);
		}
		// Now check for checking mail
		if (*send && (*CurPers)->sendMeNow)
		{
			PersHandle relayPers = SMTPRelayPers();
			Boolean oldDoMeNow;
			
			// if we're using a relay personality, switch to it
			if (relayPers)
			{
				// gotta force doMeNow or we won't ask for a password
				oldDoMeNow = (*relayPers)->doMeNow;
				(*relayPers)->doMeNow = true;
				PushPers(relayPers);
			}
			
			if (NeedPassword(false,true))
			{
				if (PersFillPw(CurPers,0)!=noErr)
				{
					if (relayPers) PopPers();
					CurPers = oldCur;
					return(1);
				}
			}
			
			// put back the personality, and reset the doMeNow flag
			if (relayPers)
			{
				PopPers();
				(*relayPers)->doMeNow = oldDoMeNow;
			}
		}

		ASSERT(CurPers==pers);
	}

			
	BlockMoveData(&flags,xFlags,sizeof(flags));
	
	CurPers = oldCur;
	return(noErr);
}

/************************************************************************
 * XferMailReal - transfer mail; check or send, for each personality
 ************************************************************************/
short XferMailRun(Boolean check, Boolean send, Boolean manual,Boolean ae, XferFlags flags, IMAPTransferPtr imapInfo)
{
	PersHandle oldCur = CurPers;
	PersHandle pers;
	OSErr err, anyErr;
	long gotSome;
	short dialErr;
	Boolean offlineWas;
	Boolean popChecked = false;
	
	ASSERT(!InAThread() || CurThreadGlobals != &ThreadGlobals);
		
	anyErr = err = noErr;

#ifdef THREADING_ON			
	if (InAThread())
	{ 
		if (PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !TaskProgressWindow && !FindOpenWazoo(TASKS_WIN))
		{
			if (imapInfo && imapInfo->command!=UndefinedTask && !PrefIsSet(PREF_IMAP_TP_BRING_TO_FRONT))
				OpenTasksWinBehind(nil);
			else
				OpenTasksWinBehind(manual ? BehindModal : nil);
		}
	}
	NoXfer = flags.stub;	
#endif THREADING_ON

	gotSome = 0;
	dialErr = noErr;
	offlineWas = Offline;
	Offline = False;
	gPPPConnectFailed = false;

	// is this an IMAP operation of some kind?
	if (imapInfo && imapInfo->command!=UndefinedTask)
	{	
#if __profile__
//		ProfilerClear();
#endif
		SetCurrentTaskKind(imapInfo->command);

		switch (imapInfo->command)
		{
			// if a mailbox was specified, then we're checking mail for an IMAP personality.
			case IMAPResyncTask:
			{
				err = DoFetchNewMessages(&(imapInfo->targetSpec), true, false) ? noErr : 1;
				check = true;
				gotSome++;
				break;
			}
			// if a tocH and uid is specified, then we should fetch a message
			case IMAPFetchingTask:
			{
				err = DoDownloadMessages(imapInfo->destToc, imapInfo->uids, imapInfo->attachmentsToo) ? noErr : 1;	// some error is needed here
				break;
			}
			// if a delToc and uid is specified, delete the sepcified message(s)
			case IMAPDeleteTask:
			case IMAPUndeleteTask:
			{
				err = DoDeleteMessage(imapInfo->delToc, imapInfo->uids, imapInfo->nuke, imapInfo->expunge, (imapInfo->command==IMAPUndeleteTask)) ? noErr : 1;
				break;
			}
			// if a source, destination, and uid list is specified, then do a transfer
			case IMAPTransferTask:
			{
				err = DoTransferMessages(imapInfo->sourceToc, imapInfo->destToc, imapInfo->uids, imapInfo->copy);
				break;
			}
			case IMAPExpungeTask:
			{
				err = DoExpungeMailbox(imapInfo->delToc) ? noErr : 1;
				break;
			}
			case IMAPAttachmentFetch:
			{
				err = DoDownloadIMAPAttachments(imapInfo->attachments, imapInfo->targetBox) ? noErr : 1;
				break;
			}
			case IMAPSearchTask:
			{
				err = DoIMAPServerSearch(imapInfo->destToc, imapInfo->boxesToSearch, imapInfo->toSearch, imapInfo->searchC, imapInfo->matchAll, imapInfo->firstUID) ? noErr : 1;
				break;
			}
			case IMAPMultResyncTask:
			case IMAPMultExpungeTask:
			{
				err = DoIMAPProcessMailboxes(imapInfo->toResync, imapInfo->command) ? noErr : 1;
				break;
			}
			case IMAPUploadTask:
			{
				err = DoTransferMessagesToServer(imapInfo->destToc, imapInfo->appendData, imapInfo->copy, false);
				break;
			}
			case IMAPPollingTask:
			{
				IMAPPollMailboxes(imapInfo->targetBox);
				break;
			}
			case IMAPFilterTask:
			{
				err = DoIMAPFilterProgress();
				break;
			}
			default:
			{
				// err = something went horribly wrong to get here
				break;
			}
		}
		Offline = Offline || offlineWas;
#if __profile__
//	ProfilerDump("\pthreadedimap-profile");
//	ProfilerClear();
#endif
	}
	// otherwise, it's a mailcheck, sweet and boring.
	else
	{
		IMAPCheckThreadRunning = gNewMessages = 0;	// forget about all past IMAP mail checks
		NoNewMailMe = false; // also forget about any pending NoNewMail alerts we may have around.

		gStayConnected = true;	// Tell the dial code to keep the connection up until further notice.
		for (pers=PersList;pers && !Offline && !gPPPConnectFailed && !CommandPeriod;pers=(*pers)->next)
		{
			CurPers = pers;

			// only display the new mail alert if a pop personality is being checked
			if ((*CurPers)->checkMeNow && !PrefIsSet(PREF_IS_IMAP)) popChecked = true;

			err = XferMailLo(check && (*CurPers)->checkMeNow,send && (*CurPers)->sendMeNow,manual||ae,flags,&gotSome,&dialErr);
			if (!anyErr) anyErr = err;
			ASSERT(CurPers==pers);
		}
		CurPers = oldCur;
		Offline = Offline || offlineWas;
		gStayConnected = false;	// the dialup connection can be torn down when no longer needed.

	}
	
#ifdef THREADING_ON
	if (InAThread())
	{
#ifndef BATCH_DELIVERY_ON
		if (send)
			SetNeedToFilterOut();
		if (check)
		{
			TOCHandle tempInTocH=GetTempInTOC();
			TempInCount = tempInTocH ? (*tempInTocH)->count  :  0;			
			if (TempInCount)
				AddFilterTask();
		}
#endif
		// silly no mail alert; here is as good a place as any
		if (!dialErr && flags.check && manual)
			// do NoNewMailMe only if a pop personality was checked, and there aren't any other check threads going.
			if (popChecked && !IMAPCheckThreadRunning && !gNewMessages)
				NoNewMailMe = gotSome==0;
	}
	else	
#endif
	{
		/*
		 * notify the user if new mail arrived (or not, in some cases)
		 */
		if (!dialErr && flags.check && (manual || gotSome>0))
		{
			NotifyNewMail(gotSome,flags.stub,GetRealInTOC(),nil);
		}
		NotifyHelpers(0,eHasConnected,nil);
	}

	if (check) ResetCheckTime(true);	// Reset intervals for *all* personalities that were supposed to be checked. JDB 12-16-98 
	ZapHandle(LastAttSpec);
	
	ASSERT(!InAThread() || CurThreadGlobals != &ThreadGlobals);
		
	return(err);
}


/**********************************************************************
 * XferMailLo - transfer mail for a given personality
 **********************************************************************/
short XferMailLo(Boolean check, Boolean send, Boolean manual, XferFlags flags,long *totalGot,OSErr *dialErrPtr)
{
	short err = 0;
	short gotSome = 0;
#ifdef CTB
	Boolean needDial;
	Boolean need2ndPW = False;
	short dialErr = 0;
	Boolean needPW;
	Str31 pass;
#endif
	short anyErr = 0;
	Boolean offlineWas;
	Str255 s;
	TransStream mailStream = 0;
	Boolean willCheck = false,
					willSend = false;

	if (PrefIsSet(PREF_THREADING_OFF) || UseCTB) PhKill();
		
	offlineWas = Offline;
	Offline = False;
	
	/*
	 * clear the decks
	 */
	if (send && !(*CurPers)->sendQueue) send = False;
#ifdef THREADING_ON	
	/*
	 * Set which task we're about to do in case we need to report it if we're low on memory
	 */
	SetCurrentTaskKind((check && !((*CurPers)->popSecure && send)) ? CheckingTask : SendingTask);
#endif
	FlushTOCs(True,True);			/* flush unnecessary TOC's */
	if (MonitorGrow(True) || CommandPeriod) goto done;
	if (check) HesOK = True;	// force re-fetch of hesiod info
	GetPOPInfo(s,s+127);
	HesOK = False;

	/*
	 * figure out what we need to do our duty
	 * We need the password if we're checking (or sending POPSecure)
	 * We need to dial the phone if we're going to use the CTB for
	 * sending or checking (or verifying the account under POPSecure)
	 */
	if (!send && !check) goto done;	/* nothing to do */
	
	if (CountResources(NOTIFY_TYPE)) NotifyHelpers(0,eWillConnect,nil);

	if (check && (anyErr=POPHostLimit())) goto done;

#ifdef CTB
	needDial = UseCTB && (send && !UUPCOut || check && !UUPCIn);
	if (needDial) CheckNavPW(&needPW,&need2ndPW);
#endif
	
	/*
	 *	Set up our TransStream
	 */
	if (anyErr=NewTransStream(&mailStream)) goto done;
	
	/*
	 * Doing network stuff; alerts should timeout
	 */
#ifdef CTB
	if (need2ndPW)
	{
		if (GetSecondPass(pass)) return(1);
		PSCopy((*CurPers)->secondPass,pass);
	}
#endif

	/*
	 * Do we need to dial the phone?
	 */
	OpenProgress();
#ifdef CTB
	if (needDial) dialErr = err = DialThePhone(mailStream);
#endif
	
	/*
	 * Now, do the mail transfers
	 */
	if (!err)
	{
		if (MonitorGrow(True)) return(0);
		
		if (check) (*CurPers)->checked = True;	// don't retry instantly
				
		/*
		 * check for mail, if need be
		 */
		willCheck = !Offline && 		// Check to see if we just went offline
				!CommandPeriod && check && (!UseCTB || !err);
				
#ifdef THREADING_ON	
		/*
		 * Set task we're about to do in case we need to report it if we're low on memory
		 */
		if (willCheck)
			SetCurrentTaskKind(CheckingTask);
#endif
		if (MonitorGrow(True)) return(0);
		if (willCheck)
		{
			if (IsIMAPPers(CurPers))
			{
				//checking mail on IMAP server
				MailboxNodeHandle imapNode;
				TOCHandle tocH;
				
				// locate the inbox, and resync it.
				if (imapNode = LocateInboxForPers(CurPers))
				{
					FSSpec inboxSpec = (*imapNode)->mailboxSpec;
					tocH = TOCBySpec(&inboxSpec);
					IMAPCheckThreadRunning++;
					if (FetchNewMessages(tocH,true, true, true, !manual)) 
					{
						// remember if this was a manual mail check.  the No New mail dialog might have to be shown.
						gWasManualIMAPCheck = manual;
						
						// resync all open IMAP mailboxes as well, if we should
						if (PrefIsSet(IMAP_RESYNC_OPEN_MAILBOXES)) 
							ResyncOpenMailboxes(CurPers);
							
						// poll mailboxes if we ought to
						IMAPPoll(CurPers);
					}
					IMAPCheckThreadRunning--;
				}
			}
			else
			{
				AuditCheckStart(++gCheckSessionID,(*CurPers)->persId,!manual);
				StartStreamAudit(mailStream, kAuditBytesReceived);
				
				err = CheckForMail(mailStream,&gotSome,&flags) || err;
				
				StopStreamAudit(mailStream);
				AuditCheckDone(gCheckSessionID,gotSome,gotSome?ReportStreamAudit(mailStream):0);
			}
			(*CurPers)->checked = True;
#ifdef CTB
			if (needDial && send && !err) err = CTBNavigateSTRN(NAVMID);
#endif
		}

#ifdef POPSECURE
		/* if the password has been invalidated, we can't send */
		if (!POPSecure) send = False;
#endif

		/*
		 * send mail
		 * For CTB connections, we only do this if the POP check went ok,
		 * since we don't know what state the line may be in.  For MacTCP or UUPC,
		 * the two are independent so we really don't care.
		 */
		willSend = !Offline && !CommandPeriod && send && (!UseCTB || !err) && !gPPPConnectFailed;
#ifdef THREADING_ON	
		/*
		 * Set task we're about to do in case we need to report it if we're low on memory
		 */
		if (willSend)
	SetCurrentTaskKind(SendingTask);
#endif
		if (MonitorGrow(True)) return(0);
		
		if (willSend)
			err = SendTheQueue(mailStream,flags) || err;
	}
	
	anyErr = err || CommandPeriod;
	
	/*
	 * cleanup connection
	 */
#ifdef CTB
	if (needDial) HangUpThePhone();
#endif
	//CloseProgress();
	
done:
	Offline = Offline || offlineWas;
#ifdef CTB
	if (dialErrPtr) *dialErrPtr = dialErr;
#endif
	if (totalGot) *totalGot += gotSome;
	if (mailStream) ZapTransStream(&mailStream);
	NonNullTicks = TickCount();	// We pretend that finishing a mailcheck is a meaningful "event"
	return(anyErr);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean NeedPassword(Boolean check,Boolean send)
{
	Boolean needPW;
	Str255 s;
	Boolean doesAuth = PrefIsSet(PREF_SMTP_DOES_AUTH);
	Boolean authOK = !PrefIsSet(PREF_SMTP_AUTH_NOTOK);
	Boolean xtndXmit = PrefIsSet(PREF_POP_SEND);
	Boolean doggieStyle = PrefIsSet(PREF_KERBEROS);
	Boolean gave530 = ShouldSMTPAuth();
	
	
	if (!(*CurPers)->doMeNow) return(False);
	needPW = !PrefIsSet(PREF_KERBEROS) && !UUPCIn && check && *GetPOPPref(s);			/* canonical; a POP check */
	
	// Are we going to send in a potentially auth-able way?
	if (!needPW && (doesAuth || xtndXmit) && send && !UUPCOut && !doggieStyle)
	{
		needPW = authOK || xtndXmit;
		
		// We'd like to auth.
		if (!xtndXmit && doesAuth)
		{
			// If the server says yes but the user says no,
			// better ask the user to change their mind
			if (gave530 && !authOK)
			{
				PCopy(s,(*CurPers)->name);
				switch (ComposeStdAlert(Note,RECONSIDER_AUTH,s))
				{
					// user will give us the password.  Yippee.
					case kAlertStdAlertOKButton:
						SetPref(PREF_SMTP_AUTH_NOTOK,NoStr);
						needPW = true;
						break;
					// user wants to abort the send.  Ok.
					case kAlertStdAlertCancelButton:
						(*CurPers)->sendMeNow = false;
						(*CurPers)->doMeNow = (*CurPers)->checkMeNow;
						break;
					// user thinks he can spit at god.  Good luck.
					default:
						needPW = false;
						break;
				}
			}
		}
	}
	
	return(needPW && !*(*CurPers)->password);
}	

typedef enum {
	sxfOK = 1,
	sxfCancel,
	sxfCheck,
	sxfSend,
	sxfServDel,
	sxfServFetch,
	sxfNuke,
	sxfNukeHard,
	sxfStub,
	sxfIcon=11,
	sxfLabel=14,
	sxfVertical,
	sxfList=17
}	SXFDialogEnum;

/**********************************************************************
 * POPHostLimit - limit the domain of hosts the user is allowed to connect to
 **********************************************************************/
OSErr POPHostLimit(void)
{
	Str255 user,host;
	Str63 sub;
	short i;
	
	if (*GetRString(sub,OnlyHostsStrn+2))
	{
		GetPOPInfo(user,host);
		for (i=2;*GetRString(sub,OnlyHostsStrn+i);i++)
			if (PFindSub(sub,host)) return(noErr);
		WarnUser(OnlyHostsStrn+1,0);
		return(fnfErr);
	}
	return(noErr);
}
	


/************************************************************************
 * SpecialXferFilter - support arrow keys in special xfer dialog
 ************************************************************************/
pascal Boolean SpecialXferFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	short key = (event->message & charCodeMask);
	ControlHandle cntl=nil;
	ListHandle list;
	short type;
	Rect r;
	short selectMe;
	short last;
	static uLong lastTicks;
	static Point lastWhere;
	
	Type2Select(event);
	
	if (event->what==mouseDown)
	{
		if (TickCount()-lastTicks < GetDblTime())
		if (ABS(event->where.h-lastWhere.h)<GetRLong(DOUBLE_TOLERANCE))
		if (ABS(event->where.v-lastWhere.v)<GetRLong(DOUBLE_TOLERANCE))
		{
			Point localWhere = event->where;
			GlobalToLocal(&localWhere);
			GetDialogItem(dgPtr,sxfList,&type,(void*)&cntl,&r);
			if (cntl)
			if (list = LCGetList(cntl))
			{
				Rect rView = (*list)->rView;
				if (PtInRect(localWhere,&rView) && Next1Selected(0,list))
				{
					*item = 1;	// hit ok on 2-click
					return true;
				}
			}
		}
		lastTicks = event->when;
		lastWhere = event->where;
	}
	
	if ((event->what==keyDown || event->what==autoKey) && (key==upArrowChar || key==downArrowChar))
	{
		GetDialogItem(dgPtr,sxfList,&type,(void*)&cntl,&r);
		if (cntl)
			if (list = LCGetList(cntl))
			{
				if (key==upArrowChar)
				{
					selectMe = Next1Selected(0,list);
					selectMe = MAX(selectMe-1,1);
				}
				else
				{
					for (last=selectMe=Next1Selected(0,list);selectMe=Next1Selected(selectMe,list);last=selectMe);
					if (!last)
						last = (*list)->dataBounds.bottom;
					selectMe = MIN(last+1,(*list)->dataBounds.bottom);
				}
				SelectSingleCell(selectMe-1,list);
				event->what=nullEvent;
				Draw1Control(cntl);				
			}
	}
	// type 2 select
	else if (event->what==keyDown && !(event->modifiers&cmdKey) && Type2SelString[0])
	{
		GetDialogItem(dgPtr,sxfList,&type,(void*)&cntl,&r);
		if (cntl)
			if (list = LCGetList(cntl))
			{
				PersHandle pers;
				short i;
				short score,selectScore;
				UPtr spot;
				
				selectMe = i = 0;
				selectScore = 1000;
				for (pers=PersList;pers;pers=(*pers)->next)
				{
					i++;
					spot = PFindSub(Type2SelString,LDRef(pers)->name);
					if (spot)
					{
						// we want the leftmost match
						score = spot-(*pers)->name-1;
						// but if it's in the middle of a word, it's less tasty
						if (spot>(*pers)->name+1 && IsWordChar[spot[-1]]) score += 100;
						if (score<selectScore)
						{
							selectMe = i;
							selectScore = score;
						}
					}
					UL(pers);
				}
				if (selectMe)
				{
					SelectSingleCell(selectMe-1,list);
					event->what=nullEvent;
					Draw1Control(cntl);	
				}			
			}
	}
		
	return(DlgFilter(dgPtr,event,item));
}

/**********************************************************************
 * SpecialXfer - set the special transfer flags
 **********************************************************************/
OSErr SpecialXfer(struct XferFlags *flags)
{
	MyWindowPtr	dlogWin;
	DialogPtr dlog;
	short res;
	Boolean stub, fetch, del;
	PersHandle pers;
	short type;
	Rect r;
	ControlHandle cntl;
	short item;
	ListHandle list;
	Point c;
	Str63 name;
	short nPers = PersCount();
	DECLARE_UPP(SpecialXferFilter,ModalFilter);
	short vOffset = 0;

#define N_PERS_START	9
#define N_PERS_MAX	25

	SetDialogFont(SmallSysFontID());
	dlogWin = GetNewMyDialog(SPECIAL_CHECK_ALRT,nil,nil,InFront);
	SetDialogFont(0);

	dlog = GetMyWindowDialogPtr (dlogWin);
	if (!dlog) return(WarnUser(MEM_ERR,ResError()));
	
	SetPort(GetDialogPort(dlog));
	ConfigFontSetup(dlogWin);
	
	/*
	 * add multiple personality stuff
	 */
	if (nPers>1)
	{
		AppendDITL(dlog, GetResource('DITL',PERS_LIST_DITL), overlayDITL);
		GetDialogItem(dlog,sxfList,&type,(void*)&cntl,&r);
		
		if (nPers > N_PERS_START)
		{
			vOffset = MIN(nPers,N_PERS_MAX) - N_PERS_START;
			vOffset *= RectHi(r)/N_PERS_START;
			r.bottom += vOffset;
			SetDialogItem(dlog,sxfList,type,(void*)cntl,&r);
			if ((type&ctrlItem) == ctrlItem)
				MySetCntlRect((ControlHandle)cntl,&r);

			GetPortBounds(GetDialogPort(dlog),&r);
			SizeWindow(GetDialogWindow(dlog),RectWi(r),RectHi(r)+vOffset,true);
		}
		
		// Move old panel to right
		GetDialogItem(dlog,sxfVertical,&type,(void*)&cntl,&r);
		r.bottom += vOffset;
		SetDialogItem(dlog,sxfVertical,type,(void*)cntl,&r);
		if ((type&ctrlItem) == ctrlItem)
			MySetCntlRect((ControlHandle)cntl,&r);
		for (item=1;item<sxfVertical;item++)
		{
			if (item!=sxfIcon) OffsetDItem(dlog,item,r.left,0);
			if (item==sxfIcon || item==sxfOK || item==sxfCancel)
				OffsetDItem(dlog,item,0,vOffset);
		}
		
		GetDialogItem(dlog,sxfList,&type,(void*)&cntl,&r);
		if (list = LCGetList(cntl))
		{
			r = (*list)->rView;
			LSize(RectWi(r),RectHi(r)+vOffset,list);
			c.h = c.v = 0;
			LAddRow(nPers,0,list);
			for (pers=PersList;pers;pers=(*pers)->next)
			{
				PSCopy(name,(*pers)->name);
				LSetCell(name+1,*name,c,list);
				if ((*pers)->doMeNow) LSetSelect(True,c,list);
				c.v++;
			}
		}
	}
	
	/*
	 * fix the dialog controls
	 */
	ReplaceAllControls(dlog);
	
	/*
	 * set the appropriate boxes
	 */
	fetch = del = False;
	for (pers=PersList;pers;pers=(*pers)->next)
	{
		if (!fetch) fetch = GetResource(PERS_POPD_TYPE(pers),FETCH_ID)!=nil;
		if (!del) del = GetResource(PERS_POPD_TYPE(pers),DELETE_ID)!=nil;
	}
	
	EnableDItemIf(dlog,sxfServFetch,fetch);
	EnableDItemIf(dlog,sxfServDel,del);
	
	SetDItemState(dlog,sxfCheck,flags->check);
	SetDItemState(dlog,sxfSend,flags->send);
	SetDItemState(dlog,sxfServDel,del && flags->servDel);
	SetDItemState(dlog,sxfServFetch,fetch && flags->servFetch);
	SetDItemState(dlog,sxfNuke,flags->nuke);
	SetDItemState(dlog,sxfNukeHard,flags->nukeHard);
	SetDItemState(dlog,sxfStub,flags->stub);		
		
	/*
	 * put up the alert
	 */
	StartMovableModal(dlog);
	ShowWindow(GetDialogWindow(dlog));
	SetMyCursor(arrowCursor);
	
	INIT_UPP(SpecialXferFilter,ModalFilter);
	do
	{
		MovableModalDialog(dlog,SpecialXferFilterUPP,&res);
		
		if (res>sxfOK&&res<=sxfStub) SetDItemState(dlog,res,!GetDItemState(dlog,res));
		
		if (res==sxfStub)
		{
			stub = GetDItemState(dlog,sxfStub);
			EnableDItemIf(dlog,sxfCheck,!stub);
			EnableDItemIf(dlog,sxfSend,!stub);
			EnableDItemIf(dlog,sxfServDel,del && !stub);
			EnableDItemIf(dlog,sxfServFetch,fetch && !stub);
			EnableDItemIf(dlog,sxfNuke,!stub);
			EnableDItemIf(dlog,sxfNukeHard,!stub);
		}
	}
	while (res>sxfCancel);

	if (PersCount()>1)
	{
		item = 1;
		for (pers=PersList;pers;pers=(*pers)->next)
			if (Cell1Selected(item++,list))
				(*pers)->doMeNow = (*pers)->checkMeNow = (*pers)->sendMeNow = True;
			else
				(*pers)->doMeNow = (*pers)->checkMeNow = (*pers)->sendMeNow = False;
	}
	
	/*
	 * read the flags
	 */
	flags->stub = GetDItemState(dlog,sxfStub);
	flags->check = !flags->stub && GetDItemState(dlog,sxfCheck);
	flags->send = !flags->stub && GetDItemState(dlog,sxfSend);
	flags->servDel = del && !flags->stub && GetDItemState(dlog,sxfServDel);
	flags->servFetch = fetch && !flags->stub && GetDItemState(dlog,sxfServFetch);
	flags->nuke = !flags->stub && GetDItemState(dlog,sxfNuke);
	flags->nukeHard = !flags->stub && GetDItemState(dlog,sxfNukeHard);
	
	/*
	 * close
	 */
	EndMovableModal(dlog);
	DisposeDialog_(dlog);
	
	return(res==1?noErr:userCancelled);
}

/**********************************************************************
 * GoOnline - does the user wish to go online?
 **********************************************************************/
OSErr GoOnline(void)
{
#ifdef TWO
	OnlineAlertEnum button = ReallyDoAnAlert(ONLINE_ALRT,Note);
	
	if (button==olaOnline) SetPref(PREF_OFFLINE,"");
	if (button==olaOnline || button==olaConnect) return(noErr);
#endif
	return(OFFLINE);
}

/************************************************************************
 * SendTheQueue - send queued messages, assuming cnxn is setup
 ************************************************************************/
short SendTheQueue(TransStream stream, XferFlags flags)
{
	WindowPtr	tocMessWinWP;
	TOCHandle tocH=nil;
	int sumNum = -1;
	Str255 server;
	long port;
	MessHandle messH;
	Handle table;
	short err,code=0;
	short tableId;
	uLong gmtSecs = GMTDateTime();
	short defltTableId;
	UPtr tablePtr=NuPtr(256);
	short lastId = 0;
	uLong lastSig = 0xffffffff;
	short stayed = 0;
	long count;
	CSpecHandle fccList = NuHandle(0);
	Boolean openedFilters=False;
#ifdef THREADING_ON
	TOCHandle realTocH=nil;
	Boolean inThread = InAThread();
	short realSumNum = -1;
#endif
	static uLong sessionID;
	uLong numSent;
	uLong beforeBytes,
			actualBytes,
			approxBytes;
	uLong beforeTicks;
	Str255 s;
	PersHandle relayPers = nil;
	
#ifdef THREADING_ON	
	if (inThread)
	{
		SetCurrentTaskKind(SendingTask);
		RemoveTaskErrors(SendingTask,(*CurPers)->persId);
	}
	if (PersCount()==1) GetRString(s,SENDING_MAIL);
	else
	{
		LDRef(CurPers);
		ComposeRString(s,PERS_SENDING_MAIL,(*CurPers)->name);
		UL(CurPers);
	}
	ProgressMessage(kpTitle,s);
#endif
	/*
	 * clear this stupid error condition
	 */
	CommandPeriod = False;
		
	defltTableId = GetPrefLong(PREF_OUT_XLATE);
	if (!NewTables || defltTableId==DEFAULT_TABLE) defltTableId = TransOutTablID();
	if (!PrefIsSet(PREF_NO_FLATTEN))
	{
		Flatten = GetFlatten();
	}

	// If using a relay personality, switch to it now
	if (relayPers = SMTPRelayPers()) PushPers(relayPers);
	
#ifdef ESSL
	stream->ESSLSetting = GetPrefLong(PREF_SSL_SMTP_SETTING);
	if(stream->ESSLSetting & esslUseAltPort)
		port = GetRLong(SMTP_SSL_PORT);
	else
#endif
		port = GetSMTPPort();
	GetSMTPInfoLo(server,&port);
	
#ifdef POPSECURE
	if (!POPSecure && (err=VetPOP())) goto done;	//not used.  Must change for multi-connect
#endif
	ComposeLogR(LOG_SEND,nil,START_SEND_LOG,server,port);
	AuditSendStart(++sessionID,(*CurPers)->persId,flags.isAuto);
	numSent = 0;
	StartStreamAudit(stream,kAuditBytesSent);
#ifdef TWO
	if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND))
	{
#ifdef ESSL
		stream->ESSLSetting = GetPrefLong(PREF_SSL_POP_SETTING);
		if(stream->ESSLSetting & esslUseAltPort)
			port = GetRLong(POP_SSL_PORT);
		else
#endif
			port = PrefIsSet(PREF_KERBEROS)?GetRLong(KERB_POP_PORT):GetRLong(POP_PORT);
		if (err=GetPOPInfoLo(server+128,server,&port)) goto done;
		if (err=StartPOP(stream,server,port)) goto done;
		POPIntroductions(stream,server+128,nil);
		if (POPrror()) goto done;
	}
	else
#endif
	err=StartSMTP(stream,server,port);

	// if using a relay personality, kill it now
	if (relayPers) {PopPers();relayPers=nil;}
	
	if (err) goto done;
	
	if (!(tocH=GetOutTOC())) goto done;
	if (!NewTables && !TransOut && (table=GetResource_('taBL',TransOutTablID())))
	{
		BMD(*table,tablePtr,256);
		TransOut = tablePtr;
		lastId = TransOutTablID();
	}
	count = (*CurPers)->sendQueue;
	if (!inThread) TotalQueuedSize = FindTotalQueuedSize(tocH,gmtSecs);
	
	ByteProgress(nil,0,TotalQueuedSize);	
	
#ifdef THREADING_ON	
	if (inThread || (openedFilters=!RegenerateFilters()))
#else	
	if (openedFilters=!RegenerateFilters())
#endif	
		for (sumNum=0; sumNum<(*tocH)->count && code<600 && !CommandPeriod && !EjectBuckaroo; sumNum++)
			if (!(*tocH)->sums[sumNum].messH && IsQueued(tocH,sumNum) && (*tocH)->sums[sumNum].persId==(*CurPers)->persId && (*tocH)->sums[sumNum].seconds<=gmtSecs)
			{
			  //TimeStamp(tocH,sumNum,0,0);
//			  ProgressR(NoBar,count--,0,LEFT_TO_TRANSFER,nil);
			  ProgressR(NoChange,count--,0,LEFT_TO_TRANSFER,nil);	// clarence
				
				/*
				 * ready a translation table, if needed
				 */
				tableId = EffectiveTID((*tocH)->sums[sumNum].tableId);
				if (tableId != lastId)
				{
					if (tableId!=NO_TABLE && tablePtr && (table = GetResource_('taBL',tableId)))
					{
						BMD(*table,tablePtr,256);
						TransOut = tablePtr;
						lastId = tableId;		/* so we don't have to fetch it next time */
					}
					else
						TransOut = nil;	/* no table */
				}
				
				/*
				 * signature
				 */
				if (lastSig != (*tocH)->sums[sumNum].sigId)
					GrabSignature(lastSig = (*tocH)->sums[sumNum].sigId);
				
				beforeBytes = GetProgressBytes();
				beforeTicks = TickCount();
				
				/*
				 * actually send the message
				 */
				if (!(code=(UUPCOut ? UUPCSendMessage(tocH,sumNum,fccList) :
															MySendMessage(stream,tocH,sumNum,fccList))))
				{
					OutTypeEnum	outType;
#ifdef NAG
#else
					RegisterSuccess(1);	// note success in registration
#endif
					numSent++;
					messH = (*tocH)->sums[sumNum].messH;
					
					if (outType = (*tocH)->sums[sumNum].outType)
						UpdateNumStat(outType==OUT_FORWARD?kStatForwardMsg:outType==OUT_REPLY?kStatReplyMsg:kStatRedirectMsg,1);
	
					// adjust progress bar		
					if (messH)
					{
						long rate;
						actualBytes = GetProgressBytes() - beforeBytes;
						rate = (actualBytes*600)/((TickCount()-beforeTicks+1)*1024);
						ComposeLogS(LOG_TPUT,nil,"\p%d.%d KBps",rate/10,rate%10);
						approxBytes = (ApproxMessageSize(messH) K);
						if (actualBytes < approxBytes)
							ByteProgress(nil,actualBytes - approxBytes,0);
						else
							ByteProgressExcess(approxBytes - actualBytes);
					}

					SetState(tocH,sumNum,SENT);
					if (!PrefIsSet(PREF_CORVAIR) && WriteTOC(tocH)) {code=600;break;}	/* happy, Dave? */
	#ifdef TWO
	#ifdef THREADING_ON
	/* fcc and filtering of outgoing messages should be done after all messages sent in the main thread */
					if (!inThread)
					{
	#endif
						if (fccList && GetHandleSize_(fccList))
							DoFcc(tocH,sumNum,fccList);
						if (messH)
						{
							err = FilterMessage(flkOutgoing,tocH,sumNum);
							if (err == euFilterXfered)
							{
								sumNum--;
								continue;
							}
							else stayed++;
						}
	#ifdef THREADING_ON
					}
					else 
						stayed++;
	#endif
	#else
					stayed++;
	#endif
	#ifdef THREADING_ON
		/* don't delete message if we're in a thread. we'll do that from the main thread. */
				if (!inThread && ((*tocH)->sums[sumNum].flags&FLAG_KEEP_COPY)==0)
	#else
					if (((*tocH)->sums[sumNum].flags&FLAG_KEEP_COPY)==0)
	#endif
					{
						if (messH && MessOptIsSet(messH,OPT_ATT_DEL)) CompAttDel(messH);
						
						if (messH && (*messH)->win) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
						DeleteMessage(tocH,sumNum,False);
						sumNum--; 			/* back up, since we deleted the message */
						RedoTOC(tocH);	/* keep nit-pickers happy */
						stayed--;
					}
					else if (messH && (*messH)->win) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
				}
				else
				{
					if ((*tocH)->sums[sumNum].messH) {
						tocMessWinWP = GetMyWindowWindowPtr ((*(*tocH)->sums[sumNum].messH)->win);
						if (tocMessWinWP && !IsWindowVisible (tocMessWinWP))
							CloseMyWindow(tocMessWinWP);
					}
					if (IsAddrErr(code))
					{
						(*tocH)->sums[sumNum].flags |= FLAG_ADDRERR;
						OpenAddrErrs = true;
					}
				}
#ifdef THREADING_ON
			// update outgoing message status if we're in a thread
				if (inThread)	
				{
					if (realTocH=GetRealOutTOC())
					{
						realSumNum = FindSumByHash(realTocH,(*tocH)->sums[sumNum].uidHash);
						if (realSumNum!=-1)
						{
							SetState(realTocH,realSumNum,(*tocH)->sums[sumNum].state);
							if ((*tocH)->sums[sumNum].state==MESG_ERR)
							{
								(*realTocH)->sums[realSumNum].flags |= FLAG_ADDRERR;
								OpenAddrErrs = true;
							}
							(*realTocH)->sums[realSumNum].flags |= FLAG_UNFILTERED;
#ifdef BATCH_DELIVERY_ON
							NeedToFilterOut++;
#endif
							if (!PrefIsSet(PREF_CORVAIR))
								WriteTOC(realTocH);
						}	
					}		
				}		
#endif				
			}
done:
	if (relayPers) PopPers();
	StopStreamAudit(stream);
	AuditSendDone(sessionID,numSent,ReportStreamAudit(stream));
	UpdateNumStat(kStatSentMail,numSent);	
	if (openedFilters) FiltersDecRef();
	ZapHandle(fccList);
	ProgressMessageR(kpSubTitle,CLEANUP_CONNECTION);
	if (tablePtr) ZapPtr(tablePtr);
	ZapPtr(Flatten);
	TransOut = nil;
#ifdef TWO
	if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND))
	{
		(void) EndPOP(stream);
	}
	else
#endif
	(void) EndSMTP(stream);
	if (tocH && (*tocH)->win && sumNum>=0)
		BoxSelectAfter((*tocH)->win,sumNum);

	FlushTOCs(True,False);	/* save memory, in case Out and Trash are large,
														 and we're going on to do a check */
	NotifyHelpers(stayed,eMailSent,nil);

	return(err);
}

/************************************************************************
 * FindTotalQueuedSize - find out how big all the messages are
 ************************************************************************/
long FindTotalQueuedSize(TOCHandle tocH,long gmtSecs)
{
	short sumNum;
	long size = 0;
	MyWindowPtr win;

	for (sumNum=0; sumNum<(*tocH)->count; sumNum++)
		if (!(*tocH)->sums[sumNum].messH && IsQueued(tocH,sumNum) && (*tocH)->sums[sumNum].persId==(*CurPers)->persId && (*tocH)->sums[sumNum].seconds<=gmtSecs)
		{
			if (win=GetAMessage(tocH,sumNum,nil,nil,false))
			{
				size += ApproxMessageSize(Win2MessH(win)) K;
				CloseMyWindow(GetMyWindowWindowPtr (win));
			}
		}
	return(size);
}


/**********************************************************************
 * CompAttDel - delete attachments with an outgoing message
 **********************************************************************/
void CompAttDel(MessHandle messH)
{
	short index;
	FSSpec spec;
	OSErr err;
	
	for (index=1;!(err=GetIndAttachment(messH,index,&spec,nil));index++)
		FSpTrash(&spec);
}

/**********************************************************************
 * DoFcc - do the fcc's for a message
 **********************************************************************/
OSErr DoFcc(TOCHandle tocH,short sumNum,CSpecHandle list)
{
	CSpec spec;
	short n = HandleCount(list);
	OSErr err=noErr;
	OSErr oneErr;
	
	UseFeature (featureFcc);
	while (n--)
	{
		spec = (*list)[n];
		if (oneErr=MoveMessageLo(tocH,sumNum,&spec.spec,True,false,true))
		{
			(*tocH)->sums[sumNum].flags |= FLAG_KEEP_COPY;
			TOCSetDirty(tocH,true);
			if (!err) err = oneErr;
		}
	}
	
	SetHandleBig_(list,0);
	return(err);
}
	
/************************************************************************
 * CheckForMail - need I say more?
 ************************************************************************/
short CheckForMail(TransStream stream,short *gotSome,XferFlags *flags)
{
	long interval=TICKS2MINS * GetPrefLong(PREF_INTERVAL);
	Handle table;
	short err = 0;
	Str255 s;
	
#ifdef DEBUG
	if (BUG15)
	{
		Str63 dts;
		Dprintf("\p;log Log.%p;g",LocalDateTimeShortStr(dts));
	}
#endif
	
#ifdef THREADING_ON	
	if (InAThread())
	{
		SetCurrentTaskKind(CheckingTask);
		RemoveTaskErrors(CheckingTask,(*CurPers)->persId);
	}
	if (PersCount()==1) GetRString(s,CHECKING_MAIL);
	else
	{
		LDRef(CurPers);
		ComposeRString(s,PERS_CHECKING_MAIL,(*CurPers)->name);
		UL(CurPers);
	}
	ProgressMessage(kpTitle,s);
#endif
	
	Headering = flags->stub;
	
	/*
	 * is there enough room on this volume?
	 */
	if (err=VolumeMargin(MailRoot.vRef,0)) return(WarnUser(NOT_ENOUGH_ROOM,err));
	
	/*
	 * clear this stupid error condition
	 */
	CommandPeriod = False;
	
	/*
	 * you know, I don't remember why this was done, but I'm going to
	 * leave it alone for now
	 */
	ResetAlertStage ();

	/*
	 * set up translation table
	 */
	if (!NewTables && !TransIn && (table=GetResource_('taBL',TRANS_IN_TABL)))
	{
		HNoPurge_(table);
		if (TransIn=NuPtr(256)) BMD(*table,TransIn,256);
		HPurge(table);
	}
	
	/*
	 * check mail
	 */
	err = UUPCIn ? GetUUPCMail(True,gotSome) : GetMyMail(stream,True,gotSome,flags);
	
#ifdef NAG
#else
	if (gotSome) RegisterSuccess(2);
#endif

#ifdef DEBUG
	if (BUG15) Dprintf("\p%d;sc;g",err);
#endif
	
	/*
	 * get rid of table
	 */
	if (TransIn) {ZapPtr(TransIn); TransIn=nil;}
	
#ifdef DEBUG
	if (BUG15)
	{
		Str63 dts;
		Dprintf("\pp;log;g",LocalDateTimeShortStr(dts));
	}
#endif

	return(err);
}

/**********************************************************************
 * ResetCheckTime - reset the mail check interval
 **********************************************************************/
void ResetCheckTime(Boolean force)
{
	PushPers(CurPers);
	for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next)
		ResetPersCheckTime(force);
	PopPers();
}

/************************************************************************
 * ResetPersCheckTime - reset the mail check interval for a personality
 ************************************************************************/
void ResetPersCheckTime(Boolean force)
{
	long interval = PrefIsSet(PREF_AUTO_CHECK) ? TICKS2MINS * GetPrefLong(PREF_INTERVAL) : 0;
	
	if (interval)
	{
		/*
		 * setup the initial check interval
		 */
		if (!(*CurPers)->checkTicks) (*CurPers)->checkTicks = TickCount();

		/*
		 * manage the mail check interval
		 */
		if (force&&(*CurPers)->doMeNow || (*CurPers)->checked)
		{
			if ((*CurPers)->checkTicks+interval<TickCount()+45*60)
			{
				(*CurPers)->checkTicks += interval*((TickCount()-(*CurPers)->checkTicks+1)/interval);
				if ((*CurPers)->checkTicks+interval<TickCount()+45*60) (*CurPers)->checkTicks += interval;
			}
		}
	}
	else (*CurPers)->checkTicks = 0;
}


/************************************************************************
 * NotifyNewMail - notify the user that new mail has arrived, via the
 * notification manager.
 ************************************************************************/
 
// DO NOT FIX THIS ROUTINE
// IF IT MUST BE FIXED, THROW IT AWAY AND REWRITE IT & ALL RELATED TO IT
// SD 8/20/02

void NotifyNewMail(short gotSome,Boolean noXfer,TOCHandle tocH, FilterPB *fpbDelivery)
{
	NotifyNewMailLo(gotSome, noXfer, tocH, fpbDelivery, true);
}

void NotifyNewMailLo(short gotSome,Boolean noXfer,TOCHandle tocH, FilterPB *fpbDelivery, Boolean OpenIn)
{
	Boolean justTrash = False;
	Boolean soundAnyway = False;
	WindowPtr oldFront = FrontWindow_();
	FilterPB fpb;
	
	// display NoNewMail alert if no mail was received, and no other check threads are running
	if (!gotSome && !POPrror() && !IMAPCheckThreadRunning && !CheckThreadRunning && !gNewMessages)
	{
		CloseProgress();
		if (PrefIsSet(PREF_NEW_ALERT))
		{
#ifdef NONEWMAIL_ALERTS
			(void) ReallyDoAnAlert(NO_MAIL_ALRT,Normal);
			TendNotificationManager(true);
#endif //NONEWMAIL_ALERTS
		}
	}
	else if (gotSome || gNewMessages)
	{
		CommandPeriod = False;	/* if mail check cancelled, don't cancel filtering */
		if (fpbDelivery && tocH)
		{
			FilterPostprocess(flkIncoming,fpbDelivery);
			gotSome = fpbDelivery->notify;
			soundAnyway = fpbDelivery->doNotifyThing > 0;
		}
		else if (tocH && !InitFPB(&fpb,false,true))
		{
			if ((*tocH)->imapTOC)	// filter flagged messages only if this is an IMAP mailbox
			{			
				// Score the incoming mail all at once. 
				// This is ok, we're in the foreground anyway.
				if (HasFeature(featureJunk) && JunkPrefBoxHold() && CanScoreJunk())
				{
					JunkScoreIMAPBox(tocH, -1, -1, true);
					MoveToIMAPJunk(tocH, -1, GetRLong(JUNK_MAILBOX_THRESHHOLD), &fpb);
				}										
				
				// filter what's left over	
				FilterFlaggedMessages(flkIncoming, tocH, &fpb);
					
		
				//
				// clean up fpb
				//
				// after an IMAP mailcheck, if messages aren't to be filtered anywhere, the list of mailboxes to be opened
				// is allocated, but zero.  FilterPostprocess will do a GetSpecialTOC(IN), which causes the In mailbox to be
				// opened even though it doesn't need to be.  This causes some window layering confusion in the Carbon version. -jdboyd
				//
				if (fpb.mailbox && (GetHandleSize(fpb.mailbox)==0)) ZapHandle(fpb.mailbox);
				
				// Show NoNewMail if no mail arrived, and there's no other check threads running
				// only do this if a manual IMAP check happened recently
				if (gWasManualIMAPCheck && !IMAPCheckThreadRunning && !CheckThreadRunning && !gNewMessages && !NeedToFilterIn && !NeedToNotify && !CountFlaggedMessages(tocH)) 
				{
					gWasManualIMAPCheck = false;
					NoNewMailMe = true;
				}
			}
			else
				FilterMessagesFrom(flkIncoming,tocH,(*tocH)->count-gotSome,&fpb,noXfer);

			FilterPostprocess(flkIncoming,&fpb);
			gotSome = fpb.notify;
			soundAnyway = fpb.doNotifyThing > 0;

			// keep track of all new messages we're receiving
			gNewMessages += gotSome;
			
			// make sure notification happens if soundAnyway
			if (soundAnyway) gNewMessages++;
		}
	
		// process any reg files we received ...
		ProcessReceivedRegFiles();

		if (!gotSome && !soundAnyway && !gNewMessages) return;

		/*
		 * let the helper apps know
		 */
		if (gotSome)
		{
			if ((tocH && (*tocH)->imapTOC) || (tocH = GetRealInTOC()))
			{
				NotifyHelpers(gotSome,eMailArrive,nil);
				
				if (OpenIn && !PrefIsSet(PREF_NO_OPEN_IN))
				{
					WindowPtr	tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
					ShowBoxAt(tocH,(*tocH)->previewPTE ? -1:FumLub(tocH),OpenBehindMePlease());
					if (PrefIsSet(PREF_ZOOM_OPEN)) ReZoomMyWindow(tocWinWP);
					UpdateMyWindow(tocWinWP);
				}
			}
		}

		// display the new mail alert if there are no check threads running, and we received some new messages.
		if (CheckThreadRunning || IMAPCheckThreadRunning || !gNewMessages) return;
		else gNewMessages = 0;

		AttentionNeeded = true;
		if (PrefIsSet(PREF_NEW_SOUND)) NewMailSound();
		if (InBG && PrefIsSet(PREF_NEW_ALERT) || !PrefIsSet(PREF_NO_APPLE_FLASH))
		{
			if (MyNMRec) return;				/* already done */
			MyNMRec = New(struct NMRec);
			if (!MyNMRec) return; 			/* couldn't allocate memory (bad) */
			WriteZero(MyNMRec,sizeof(*MyNMRec));
			MyNMRec->qType = nmType;
			MyNMRec->nmMark = 1;
			MyNMRec->nmRefCon = TickCount();
			if (!PrefIsSet(PREF_NO_APPLE_FLASH))
			{
				GetIconSuite(&MyNMRec->nmIcon,FLAG_SICN,svAllSmallData);
				if (MyNMRec->nmIcon) HNoPurge(MyNMRec->nmIcon);
			}
			if (InBG && PrefIsSet(PREF_NEW_ALERT))
			{
				Str255 scratch;
				GetRString(scratch,NEW_MAIL);
				MyNMRec->nmStr = NuPtr(*scratch+1);
				if (MyNMRec->nmStr) PCopy(MyNMRec->nmStr,scratch);
				CloseProgress();
			}
			if (NMInstall(MyNMRec))
			{
				if (MyNMRec->nmIcon) DisposeIconSuite(MyNMRec->nmIcon,True);
				if (MyNMRec->nmSound) HPurge(MyNMRec->nmSound);
				ZapPtr(MyNMRec->nmStr);
				ZapPtr(MyNMRec);
			}
		}
		if (!InBG && PrefIsSet(PREF_NEW_ALERT))
		{
			CloseProgress();
			(void) ReallyDoAnAlert(NEW_MAIL_ALRT,Normal);
			TendNotificationManager(true);
		}
	}
	if (!InBG && oldFront && FrontWindow_()!=oldFront)
		FlushEvents(everyEvent&~highLevelEventMask,0);
}

/************************************************************************
 * OpenBehindMePlease - Travel down the windowlist until we find where we
 * should open new windows
 ************************************************************************/
WindowPtr OpenBehindMePlease(void)
{
	MyWindowPtr	win;
	WindowPtr 	winWP,
							frontWP,
							returnWinWP = nil;

	frontWP = MyFrontNonFloatingWindow();
	
	switch (GetPrefLong(PREF_OPEN_WHERE))
	{
		case 0:	// last comp on top
			for (winWP = frontWP; winWP; winWP = GetNextWindow (winWP))
				if (IsWindowVisibleClassic (winWP))
					if (GetWindowKind(winWP)==COMP_WIN) returnWinWP = winWP;
					else break;
			break;
		
		case 1: // on top
			returnWinWP = nil;
			break;
		
		case 2: // behind In
			for (winWP = frontWP; winWP; winWP = GetNextWindow (winWP))
				if (IsWindowVisibleClassic (winWP))
				if (GetWindowKind(winWP)==MBOX_WIN)
				if ((*(TOCHandle)GetWindowPrivateData(winWP))->which==IN)
				{
					returnWinWP = winWP;
					break;
				}
			if (returnWinWP) break;
			// if no visible In box, fall through to behind current mailbox
		
		case 3: // behind current mailbox
			for (winWP = frontWP; winWP; winWP = GetNextWindow (winWP)) if (IsWindowVisibleClassic (winWP)) break;
			win = GetWindowMyWindowPtr (winWP);
			if (GetWindowKind(winWP)==MBOX_WIN || GetWindowKind(winWP)==CBOX_WIN)
			{
				returnWinWP = winWP;
				break;
			}
			else if (GetWindowKind(winWP)==MESS_WIN || GetWindowKind(winWP)==COMP_WIN)
			{
				returnWinWP = winWP;	// just in case the toc's not visible
				win = (*(*Win2MessH(win))->tocH)->win;
				winWP = GetMyWindowWindowPtr (win);
				if (IsWindowVisibleClassic (winWP))
				{
					returnWinWP = winWP;
					break;
				}
			}
			break;
		
		case 4: // in the back
			for (winWP = frontWP; winWP; winWP = GetNextWindow (winWP))
				if (IsWindowVisibleClassic (winWP))
					returnWinWP = winWP;
			break;
		
		case 5: // behind frontmost window
			for (winWP = frontWP; winWP; winWP = GetNextWindow (winWP))
				if (IsWindowVisibleClassic (winWP))
				{
					returnWinWP = winWP;
					break;
				}
			break;
	}

	// If there's a modal window open, make sure the next window isn't opened on top of it.
	if ((returnWinWP==nil) && ModalWindow) returnWinWP = ModalWindow;
	
	win = GetWindowMyWindowPtr (frontWP);
	if (win && win->isNag)
		returnWinWP = frontWP;
		
	return(returnWinWP);
}


/************************************************************************
 * ShowBoxSel - show the mailbox with a selection
 ************************************************************************/
void ShowBoxAt(TOCHandle tocH,short selectMe,WindowPtr behindWin)
{
	WindowPtr	tocWinWP = GetMyWindowWindowPtr((*tocH)->win);
	
	if (selectMe>=0) SelectBoxRange(tocH,selectMe,selectMe,False,-1,-1);
	RedoTOC(tocH);
	ScrollIt((*tocH)->win,0,SortedDescending(tocH)?REAL_BIG:-REAL_BIG);
	if (IsWindowVisible(tocWinWP))
	{
		if (behindWin)
		{
			if (behindWin!=tocWinWP)
				SendBehind(tocWinWP,behindWin);
		}
		else  SelectWindow_(tocWinWP);
	}
	else
		ShowMyWindowBehind(tocWinWP,behindWin);
}

/************************************************************************
 * FumLub - find the FumLub
 ************************************************************************/
short FumLub(TOCHandle tocH)
{
	short i;
	if (!tocH) return(-1);
	
	RedoTOC(tocH);
	
	if (SortedDescending(tocH)) return(0);
	
	for (i=(*tocH)->count-1;i>=0;i--)
		if ((*tocH)->sums[i].state!=UNREAD)
		{
			i++;
			break;
		}
	return(i<(*tocH)->count ? MAX(i,0) : (*tocH)->count-1);
}

/************************************************************************
 * NewMailSound - play the sound for new mail
 ************************************************************************/
void NewMailSound(void)
{
	Str255 name;
	
	GetPref(name,PREF_NEWMAIL_SOUND);
	if (!*name)
		GetResName(name,'snd ',NEW_MAIL_SND);
	PlayNamedSound(name);
}

/************************************************************************
 * GrabSignature - read the signature file
 ************************************************************************/
void GrabSignature(uLong fid)
{
	short err;
	FSSpec spec;
	Accumulator enriched, html;
	MyWindowPtr win=nil;
	Boolean iOpened = false;
	Boolean oldDirty;
	Boolean addedIntro;
	
	ZapHandle(eSignature);
	ZapHandle(RichSignature);
	ZapHandle(HTMLSignature);
	if (fid==SIG_NONE) return;
	if (AccuInit(&enriched)) return;
	if (AccuInit(&html)) {AccuZap(enriched);return;}
	if (!(err = SigSpec(&spec,fid)))
	if (!(win=FindText(&spec)))
		if (win=OpenText(&spec,nil,nil,nil,False,nil,False,False))
			iOpened = true;
	if (win)
	{
		PeteGetTextAndSelection(win->pte,&eSignature,nil,nil);
		if (!(err=MyHandToHand(&eSignature)))
		{
			RemoveCharHandle(0,eSignature);
			
			// add sig intro if it's not there
			PeteCalcOff(win->pte);
			oldDirty = 0!=PeteIsDirty(win->pte);
			addedIntro = AddSigIntro(win->pte,eSignature);
			
			// build the styled versions
			SigStyled = HasStyles(win->pte,0,PETEGetTextLen(PETE,win->pte),false);
			err = BuildEnriched(&enriched,win->pte,nil,PETEGetTextLen(PETE,win->pte),0,nil,False);
			if (!err)
			{
				err = BuildHTML(&html,win->pte,nil,PETEGetTextLen(PETE,win->pte),0,nil,nil,2,nil,nil,nil);
				if (!err)
					err = HTMLPostamble(&html,False);
			}
			
			// remove sig intro from window
			if (addedIntro)
			{
				RemoveSigIntro(win->pte,nil);
				if (!oldDirty)
				{
					PeteCleanList(win->pteList);
					win->isDirty = false;
				}
			}
			PeteCalcOn(win->pte);
		}
		else eSignature = nil;
	}
	else err = 1;
		
	if (!err)
	{
		AccuTrim(&enriched);
		RichSignature = enriched.data;
		enriched.data = 0;
		AccuTrim(&html);
		HTMLSignature = html.data;
		html.data = 0;
	}

	if (win && iOpened) CloseMyWindow(GetMyWindowWindowPtr(win));
	AccuZap(enriched);
	AccuZap(html);
	if (err) FileSystemError(CANT_READ_SIG,"",err);
}

/************************************************************************
 * AddSigIntro - add the sig introducer to a petehandle or a text handle or both
 ************************************************************************/
Boolean AddSigIntro(PETEHandle pte,UHandle text)
{
	Str31 sigIntro;
	Boolean didIt = false;
	uLong len;
	
	if (!*GetRString(sigIntro,SIG_INTRO)) return(false);
	
	// do the text block first
	if (text)
	{
		if (0>SearchStrHandle(sigIntro,text,0,false,false,nil))
		{
			len = GetHandleSize(text);
			if (len && !IsAllWhitePtr(LDRef(text),len))
			{
				didIt = true;
				UL(text);
				SetHandleBig(text,len+*sigIntro);
				LDRef(text);
				if (!MemError())
				{
					BMD(*text,*text+*sigIntro,len);
					BMD(sigIntro+1,*text,*sigIntro);
				}
			}
			UL(text);
		}
	}
	
	// now the petehandle
	if (pte)
	{
		PeteGetTextAndSelection(pte,&text,nil,nil);
		if (0>SearchStrHandle(sigIntro,text,0,false,false,nil))
		{
			len = GetHandleSize(text);
			if (len && !IsAllWhitePtr(LDRef(text),len))
			{
				didIt = true;
				PETEInsertTextPtr(PETE,pte,0,sigIntro+1,*sigIntro,nil);
			}
			UL(text);
		}
	}
	
	// all done
	return didIt;
}

/************************************************************************
 * RemoveSigIntro - take the sig introducer from a petehandle or a text handle or both
 ************************************************************************/
Boolean RemoveSigIntro(PETEHandle pte,UHandle text)
{
	Str31 sigIntro;
	long len;
	Boolean didIt = false;
	
	if (!*GetRString(sigIntro,SIG_INTRO)) return(false);
	
	// do the text block first
	if (text)
	{
		len = GetHandleSize(text);
		if (len && (len>=*sigIntro && *(long*)(sigIntro+1) == *(long*)*text))	// check first four chars only
		{
			didIt = true;
			BMD(*text+*sigIntro,*text,len-*sigIntro);
			SetHandleBig(text,len-*sigIntro);
		}
	}
	
	// now the petehandle
	PeteGetTextAndSelection(pte,&text,nil,nil);
	len = GetHandleSize(text);
	if (len && (len>=*sigIntro && *(long*)(sigIntro+1) == *(long*)*text))	// check first four chars only
	{
		didIt = true;
		PeteDelete(pte,0,*sigIntro);
	}
	
	// all done
	return didIt;
}

/************************************************************************
 * SigSpec - get the FSSpec for the (standard) signature file
 ************************************************************************/
OSErr SigSpec(FSSpecPtr spec,long fid)
{
	OSErr err;
	Str63 s;
	short i;
	
	err = SubFolderSpec(SIG_FOLDER,spec);
	if (fid<0) return(noErr);
	if (!err)
	{
		if (fid<=1)
		{
			err = FSMakeFSSpec(spec->vRefNum,spec->parID,
#ifdef TWO
												 GetRString(s,fid==1?ALT_SIG:SIGNATURE),spec);
#else
												 GetRString(s,SIGNATURE),spec);
#endif
		}
		else
		{
			for (i=SignatureCount();i;i--)
			{
				GetSignatureName(i,s);
				MyLowerStr(s);
				if (Hash(s)==fid)
				{
					err = FSMakeFSSpec(spec->vRefNum,spec->parID,s,spec);
					break;
				}
			}
			if (!i) return(fnfErr);
		}
	}
	return(err);
}

/**********************************************************************
 * TransmitMessageHi - send a message, possibly with PGP
 **********************************************************************/
OSErr TransmitMessageHi(TransStream stream,MessHandle messH,Boolean chatter,Boolean sendDataCmd)
{
	OSErr err;
#ifdef OLDPGP
	if (SumOf(messH)->flags&(FLAG_ENCRYPT|FLAG_SIGN))
		err = PGPSendMessage(stream,messH,chatter);
	else
#endif
#ifdef ETL
	if ((*messH)->hTranslators)
		err = ETLSendMessage(stream,messH,chatter,sendDataCmd);
	else
#endif
		err = TransmitMessage(stream,messH,chatter,True,True,nil,sendDataCmd);
	
	if (!err) RememberMID(SumOf(messH)->msgIdHash);
	
	return err;
}

/**********************************************************************
 * ProcessReceivedRegFiles - process any received registration files
 **********************************************************************/
void ProcessReceivedRegFiles(void)
{
	if(gRegFiles)
	{
		FSSpec spec, **list;
		long count;
		int pnPolicyCode;
		Boolean needsRegistration;
		
		list = gRegFiles;
		gRegFiles = nil;
		for(count = GetHandleSize(list)/sizeof(FSSpec); --count >= 0; )
		{
			spec = (*list)[count];
			// Parse the file we received, but do it silently if a box user has received a reg file
			ParseRegFile(&spec,IsBoxUser()?parseMaybeSilent:parseDoDialog,&needsRegistration,&pnPolicyCode);
		}
		DisposeHandle(list);
	}
}

/************************************************************************
 * SMTPRelayPers - do we have a relay personality?
 ************************************************************************/
PersHandle SMTPRelayPers(void)
{
	Str63 persName;
	
	if (PrefIsSet(PREF_NO_RELAY_PARTICIPATE)) return nil;
	
	return FindPersByName(GetPref(persName,PREF_RELAY_PERSONALITY));
}

/**********************************************************************
 * RememberMID - remember that we sent this message
 **********************************************************************/
OSErr RememberMID(uLong midHash)
{
	OutgoingMIDListDirty = true;
	return AccuAddPtr(&OutgoingMIDList,&midHash,sizeof(midHash));
}

/**********************************************************************
 * OutgoingMIDListSave - save our list of outgoing message id's
 **********************************************************************/
OSErr OutgoingMIDListSave(void)
{
	OSErr err = noErr;
	
	if (OutgoingMIDListDirty)
	{
		ZapResource(OUTGOING_MSG_MID_LIST,1001);
		if (OutgoingMIDList.data)
		{
			short count = OutgoingMIDList.offset/sizeof(uLong);
			short limit = GetRLong(OUTGOING_MID_LIST_SIZE);
			
			if (count>limit)
			{
				UPtr spot = (UPtr)(((uLong*)*OutgoingMIDList.data) + count - limit);
				BMD(spot,*OutgoingMIDList.data,limit*sizeof(uLong));
				OutgoingMIDList.offset = limit*sizeof(uLong);
			}
			AccuTrim(&OutgoingMIDList);
			AddMyResource(OutgoingMIDList.data,OUTGOING_MSG_MID_LIST,1001,"");
			err = ResError();
			if (!err)
			{
				UpdateResFile(SettingsRefN);
				DetachResource(OutgoingMIDList.data);
				OutgoingMIDListDirty = false;
			}
		}
	}
	return err;
}

/**********************************************************************
 * OutgoingMIDListLoad - load our list of outgoing message id's
 **********************************************************************/
OSErr OutgoingMIDListLoad(void)
{
	OSErr err = noErr;
	
	AccuZap(OutgoingMIDList);
	OutgoingMIDListDirty = false;
	
	OutgoingMIDList.data = Get1Resource(OUTGOING_MSG_MID_LIST,1001);
	if (OutgoingMIDList.data)
	{
		OutgoingMIDList.offset = OutgoingMIDList.size = GetHandleSize(OutgoingMIDList.data);
		DetachResource(OutgoingMIDList.data);
	}
	else
		err = ResError();
	return err;
}

/**********************************************************************
 * BadgeTheStupidDock - put a badge on the dock with the # of unread messages in it
 **********************************************************************/
void BadgeTheSupidDock(short count, PStr text, Boolean attentionColor)
{
	Str15 localText;
	static Str15 lastLocalText;
	static Boolean lastAttentionColor;
	
	if (text) PSCopy(localText,text);
	else if (count < 0) PCopy(localText,lastLocalText);
	else if (count) NumToString(count,localText);
	else *localText = 0;
	
	if (StringSame(localText,lastLocalText) && lastAttentionColor==attentionColor) return;
	
	lastAttentionColor = attentionColor;
	PCopy(lastLocalText,localText);

	RestoreApplicationDockTileImage();
	
	if (!*localText) return;
	
	// update stupid dock
	{
		CGrafPtr 	theTilePort;
		RGBColor color;

		PushGWorld();
		
		theTilePort = BeginQDContextForApplicationDockTile();

		if ( theTilePort != nil )
		{
			Rect r;
			short wi;
			short hi;
			short div;
			
			GetPortBounds( theTilePort, &r );
			PenNormal();

			for (div=3;div<7;div++)
			{
				TextFont(FontID);
				TextSize(hi=RectHi(r)/div);
				wi = StringWidth(localText);
				if (wi + 4*INSET < RectWi(r)) break;
			}
			
			RGBForeColor(GetRColor(&color,attentionColor?BADGE_ATTENTION_COLOR:BADGE_NORMAL_COLOR));
			
			SetRect(&r,r.right-3*INSET-wi,r.bottom-2*INSET-hi,r.right-INSET,r.bottom-INSET);
			PaintRect(&r);
			MoveTo(r.left+INSET, r.bottom-INSET);
			RGBForeColor(GetRColor(&color,BADGE_TEXT_COLOR));
			DrawString(localText);
			
			QDFlushPortBuffer( theTilePort, NULL );
			EndQDContextForApplicationDockTile( theTilePort );
		}
		
		PopGWorld();
	}

}

/**********************************************************************
 * GlobalUnreadCount - how many unread messages are there?
 **********************************************************************/
long GlobalUnreadCount(void)
{
	return PrefBadgeOpenBoxes()  ? GlobalOpenUnreadCount() : GlobalInUnreadCount();
}

/**********************************************************************
 * GlobalOpenUnreadCount - how many unread messages are there in open mailboxes?
 **********************************************************************/
long GlobalOpenUnreadCount()
{
	long count = 0;
	TOCHandle tocH;
	
	for (tocH=TOCList;tocH;tocH=(*tocH)->next)
		if (!(*tocH)->virtualTOC && (*tocH)->win && IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win)))
			count += TOCUnreadCount(tocH,PrefBadgeRecent());
	
	return count;
}

/**********************************************************************
 * GlobalInUnreadCount - how many unread messages are there in all In mailboxes?
 **********************************************************************/
long GlobalInUnreadCount()
{
	long count = TOCUnreadCount(GetInTOC(),PrefBadgeRecent());
	Boolean allIMAPPers = EqualStrRes("\p*",BADGE_PERS_LIST);
	PersHandle pers;
	Str63 persName;
	
	for (pers=PersList;pers;pers=(*pers)->next)
	{
		MailboxNodeHandle node;
		
		if (!IsIMAPPers(pers)) continue;
		if (!allIMAPPers)
		{
			if (pers==PersList) GetRString(persName,DOMINANT);
			else PSCopy(persName,(*pers)->name);
			if (!StrIsItemFromRes(persName,BADGE_PERS_LIST,",")) continue;
		}
		
		// ok, we have a candidate
		if (node=LocateInboxForPers(pers))
		{
			FSSpec inboxSpec = (*node)->mailboxSpec;
			count += TOCUnreadCount(TOCBySpec(&inboxSpec),PrefBadgeRecent());
		}
	}
	
	return count;
}