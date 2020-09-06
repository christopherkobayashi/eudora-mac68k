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

#include "filtrun.h"
/************************************************************************
 * Filters window - Copyright (C) 1994 QUALCOMM Incorporated
 ************************************************************************/
#define FILE_NUM 69

#ifdef TWO

#pragma segment FILTRUN

#define MAX_FILTER_PASS 10

Boolean DoesIntersectNick(UHandle nickAddresses,UHandle nickExpanded,UPtr spot,long len);
Boolean DoesIntersectNickFile(PStr file,UPtr spot,long len);
Boolean TermDateMatch(MTPtr mt, TOCHandle tocH, short sumNum);
Boolean TermJunkMatch(MTPtr mt, TOCHandle tocH, short sumNum);
Boolean TermPersMatch(MTPtr mt, TOCHandle tocH, short sumNum);
Boolean TermPriorMatch(MTPtr mt, TOCHandle tocH, short sumNum);
Boolean AnyFilters(FilterKeywordEnum fType);
Boolean AnyFiltersLo(FilterKeywordEnum fType,Handle hFilters);
Boolean RightFilterType(FilterKeywordEnum fType,short filter);
Boolean FilterMatch(short filter,TOCHandle tocH, short sumNum,FilterPBPtr fpb);
OSErr TakeFilterAction(short filter,FilterPBPtr fpb,Boolean noXfer);
Boolean TermMatch(MTPtr mt,TOCHandle tocH, short sumNum,FilterPBPtr fpb);
Boolean TermPtrMatch(MTPtr mtmt,UPtr spot,UPtr end);
void Filter1Postprocess(FilterKeywordEnum fType,FilterPBPtr fpb);
OSErr FilterMessageLo(FilterKeywordEnum fType,TOCHandle tocH,short sumNum,FilterPBPtr fpb,Boolean noXfer);
Boolean TermExpMatch(MTPtr mtmt, UPtr spot, UPtr end,UHandle *cache);
void FiltLogMatch(short filter,TOCHandle tocH,short sumNum);
uLong FilterLastMatch(short filter);
Boolean FromIntersectNickFile(MTPtr mt, TOCHandle tocH,short sumNum);
Boolean FromIntersectNickFileMatch(MTPtr mt, TOCHandle tocH,short sumNum);

OSErr FGlobalErr;

/**********************************************************************
 * Filter1PostProcess - postprocessing of a single filter
 **********************************************************************/
void Filter1Postprocess(FilterKeywordEnum fType,FilterPBPtr fpb)
{
	FSSpec spec;
	Boolean openPref = !PrefIsSet(PREF_NO_OPEN_IN);
	Boolean openBox = (openPref && !fpb->dontUser && fType==flkIncoming) || fpb->openMailbox;
	short which;
	
	if (fpb->xferred)
	{
		spec = fpb->spec;
		which = 0;
		if (!fpb->xferredFromIMAP)	// if the message was filtered from an IMAP account, report where it went no matter what.
		{
			if (spec.parID==MailRoot.dirId && spec.vRefNum==MailRoot.vRef)
				if (EqualStrRes(spec.name,IN)) which = IN;
				else if (EqualStrRes(spec.name,OUT)) which = OUT;
				else if (EqualStrRes(spec.name,TRASH)) which = TRASH;
		}
	}
	else
	{
		spec = GetMailboxSpec(fpb->tocH,-1);
		which = (*fpb->tocH)->which;
#ifdef THREADING_ON
		if (which==IN_TEMP)
		{
			TOCHandle tocH=GetRealInTOC();
			which=IN;
			ASSERT(tocH);
			if (!tocH)
				return;
			spec=GetMailboxSpec(tocH,-1);
		}
#ifdef BATCH_DELIVERY_ON
		else if (IsDelivery(&spec))
		{
			spec.vRefNum = MailRoot.vRef;
			spec.parID = MailRoot.dirId;
			GetRString(spec.name,IN);
		}
#endif
#endif
	}
	//fpb->doNotifyThing = fpb->notify;
	
	// IMAP - make sure we open/report the visible TOC, not the hidden one.
	if ((*(fpb->tocH))->imapTOC) 
		GetRealIMAPSpec(spec, &spec);
	if (spec.name)
	{
		/*
		 * do we need to open the mailbox?
		 */
		if (openBox && (fpb->xferred||!openPref))
			AddSpecToList(&spec,fpb->mailbox);
		
		/*
		 * do we need to open the message?
		 */
		if (fpb->openMessage)
		{	
			// if this message has been transferred to an IMAP mailbox, we won't be able to open it.
			if (!IsIMAPMailboxFile(&spec) || !fpb->xferred)
				AddSpecToList(&spec,fpb->message);
			if (!fpb->xferred) (*fpb->tocH)->sums[fpb->sumNum].opts |= OPT_OPEN;
		}
		
		/*
		 * do we need to do the report?
		 */
		if ((fpb->doReport || (PrefIsSet(PREF_REPORT)&&!which)) && !fpb->dontReport)
			AddSpecToList(&spec,fpb->report);
	}
	
	/*
	 * how about normal notification?
	 */
	if (fpb->xferred || fpb->dontUser)
	{
		fpb->notify--;
		if (fpb->dontUser || (which==TRASH)) fpb->doNotifyThing--;
		if (!fpb->xferred) (*fpb->tocH)->sums[fpb->sumNum].opts &= ~OPT_NOTIFY;
	}
	else (*fpb->tocH)->sums[fpb->sumNum].opts |= OPT_NOTIFY;
	
	/*
	 * printing?
	 */
	if (fpb->print && !fpb->xferred)
	{
		short oldstat = (*fpb->tocH)->sums[fpb->sumNum].state;
		PrintClosedMessage(fpb->tocH,fpb->sumNum,True);
		SetState(fpb->tocH,fpb->sumNum,oldstat);
	}
}

/**********************************************************************
 * FilterPostProcess - take all the final actions needed after filtering
 **********************************************************************/
void FilterPostprocess(FilterKeywordEnum fType,FilterPBPtr fpb)
{
#pragma unused(fType)
	CSpec spec;
	short n;
	short i;
	WindowPtr behindWP=nil;

	ComposeLogS(LOG_FILT,nil,"\pPostprocess %d %d",HandleCount(fpb->mailbox),HandleCount(fpb->message));
	
	ZapHandle(fpb->ccAddresses);
	ZapHandle(fpb->bccAddresses);
	ZapHandle(fpb->toAddresses);
	
	/*
	 * first, we make the filter report
	 */
	if (fpb->report && GetHandleSize_(fpb->report)) {GenSpecWindow(fpb->report);ZapHandle(fpb->report);}

	/*
	 *	Expunge any IMAP mailbox that may have been touched during filtering
	 */
	IMAPPostFilterExpunge();
	
	/*
	 * now, we open mailboxes
	 */
	if (fpb->mailbox)
	{
		TOCHandle tocH = GetSpecialTOC(IN);
		WindowPtr	tocWinWP = nil;

		behindWP = OpenBehindMePlease();
		if (tocH && (*tocH)->win)
		{
			// if in box open, show it first
			tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
			if (!PrefIsSet(PREF_NO_OPEN_IN) && fpb->notify && fType==flkIncoming && IsWindowVisible(tocWinWP))
			{
				if (!behindWP)
				{
					SelectWindow_(tocWinWP);
					behindWP = GetMyWindowWindowPtr((*tocH)->win);
				}
				else SendBehind (tocWinWP,behindWP);
			}
		}
		
		n=HandleCount(fpb->mailbox);
		for (i=0;i<n;i++)
		{
			spec = (*fpb->mailbox)[i];
			if (tocH=TOCBySpec(&spec.spec))
			{
				ShowBoxAt(tocH,(*tocH)->previewPTE ? -1:FumLub(tocH),behindWP);
				if (PrefIsSet(PREF_ZOOM_OPEN)) ReZoomMyWindow(GetMyWindowWindowPtr((*tocH)->win));
				if (!behindWP && tocWinWP) UpdateMyWindow(tocWinWP);	// make sure topmost one updates NOW
				behindWP = GetMyWindowWindowPtr((*tocH)->win);
			}
		}
	}
	
	/*
	 * now, we open messages
	 */
	if (fpb->message)
	{
		n=HandleCount(fpb->message);
		while (n--)
		{
			spec = (*fpb->message)[n];
			OpenFilterMessages(&spec.spec);
		}
	}
	
	/*
	 * and sounds
	 */
	if (fpb->sounds)
	{
		n=HandleCount(fpb->sounds);
		while (n--)
			PlaySoundId((*fpb->sounds)[n]);
	}

	/*
	 *	Resynchronize any IMAP mailbox that may have been touched during filtering
	 */

	if (!gSkipIMAPBoxes)
	{
		IMAPStopFiltering(true);	// done doing IMAP filtering at this point.
		IMAPPostFilterResync();
	}

	/*
	 * done
	 */
	ZapHandle(fpb->message);
	ZapHandle(fpb->mailbox);
	ZapHandle(fpb->report);
	ZapHandle(fpb->sounds);
}

/**********************************************************************
 * InitFPB - initialize an FPB
 **********************************************************************/
OSErr InitFPB(FilterPBPtr fpb,Boolean zapAddrs,Boolean listsToo)
{
	FilterPB saveFPB = *fpb;
	
	// user requested we clear addresses
	if (zapAddrs)
	{
		ZapHandle(fpb->toAddresses);
		ZapHandle(fpb->ccAddresses);
		ZapHandle(fpb->bccAddresses);
	}
		
	// zero it all
	Zero(*fpb);
	
	// get the header names
	GetRString(fpb->to,HEADER_STRN+TO_HEAD);
	GetRString(fpb->cc,HEADER_STRN+CC_HEAD);
	GetRString(fpb->bcc,HEADER_STRN+BCC_HEAD);
	
	// allocate or restore lists
	if (listsToo)
	{
		if (!(fpb->message = NuHandle(0)) ||
				!(fpb->mailbox = NuHandle(0)) ||
				!(fpb->report = NuHandle(0)))
			return(MemError());
	}
	else
	{
		fpb->message = saveFPB.message;
		fpb->mailbox = saveFPB.mailbox;
		fpb->report = saveFPB.report;
		fpb->sounds = saveFPB.sounds;
		fpb->doNotifyThing = saveFPB.doNotifyThing;
		fpb->notify = saveFPB.notify;
	}
	
	return(noErr);
}

/************************************************************************
 * FilterSelectedMessage - filter the selection from a mailbox
 ************************************************************************/
OSErr FilterSelectedMessages(FilterKeywordEnum fType,TOCHandle tocH,FilterPBPtr fpb)
{
	short err=noErr;
	short sumNum;
	short countWas;
	short lastSelected = -1;
	short initialCount = (*tocH)->count;
	Boolean isOut = (*tocH)->which==OUT;
	uLong pTicks=0;
	long count;
	TOCHandle realTocH;
	short realSumNum;
	long realSearialNum;
	FSSpec inSpec;
	TOCHandle inTocH;
	Boolean justFakingIt = false;
	Boolean bHidden = false;
			
	if (fType==flkIncoming)
	{
		if (!(inTocH=GetRealInTOC())) return(userCanceledErr);
		inSpec = GetMailboxSpec(inTocH,-1);
	}
	
	if (err=InitFPB(fpb,false,true)) return(WarnUser(MEM_ERR,err));
	
	if (!PrefIsSet(PREF_MA))
	{
		if (err=RegenerateFilters()) return(err);
		justFakingIt = fType==flkIncoming && !AnyFilters(fType);
		if (justFakingIt || AnyFilters(fType))
		{
			if ((*tocH)->imapTOC) IMAPStartManualFiltering();
			count = CountSelectedMessages(tocH);
			if (count>10) OpenProgress();
			ProgressMessageR(kpTitle,FILTERING);
			ProgressMessageR(kpSubTitle,LEFT_TO_PROCESS);
			Progress(NoBar,count,nil,nil,nil);
			for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
			{
#ifdef SPEECH_ENABLED
				(void) SpeechIdle ();
#endif
				if (count>1) MiniEvents(); if (CommandPeriod) break;
				if ((*tocH)->sums[sumNum].selected)
				{
					fpb->notify++;
					if (!(--count%10) || TickCount()-pTicks>30)
					{
						Progress(NoBar,count,nil,nil,nil);
						pTicks = TickCount();
					}
					lastSelected = sumNum;
					countWas = (*tocH)->count;
					
					/*
					 * load up cache
					 */
					 
					if ((realTocH = GetRealTOC(tocH,sumNum,&realSumNum)))
					{
						realSearialNum = (*realTocH)->sums[realSumNum].serialNum;
						
						CacheMessage(realTocH,realSumNum);

						if (!(*realTocH)->imapTOC)
						{
							if (!(*realTocH)->sums[realSumNum].cache)
							{
								WarnUser(MEM_ERR,MemError());
								err = 1;
							}
							else
								HNoPurge((*realTocH)->sums[realSumNum].cache);
						}
						
						/*
						 * do the filtering
						 */
						if (!err && !justFakingIt) err = FilterMessageLo(fType,realTocH,realSumNum,fpb,False);
						
						if (!err && fType==flkIncoming)
						{
							// transfer to in
							if (!(err=MoveMessageLo(realTocH,realSumNum, &inSpec, false, false, true)))
								err = euFilterXfered;
						}
		
						// Hide the message if it was deleted.
						if ((*tocH)->imapTOC)
							bHidden = ShowHideFilteredSummary(tocH, sumNum);
						
						// adjust the count to account for messages that are removed from the toc during the filtering process
						if (bHidden || ((err==euFilterXfered) && ((!(*tocH)->imapTOC && !(*tocH)->virtualTOC))))
						{
							// a message was either moved into the hidden IMAP cache,
							// or a message was transferred from a non IMAP, non virtual toc
							sumNum--;
							InvalContent((*tocH)->win);
							UpdateMyWindow(GetMyWindowWindowPtr((*tocH)->win));
						}
						
						/*
						 * clean up after
						 */
						if (err==euFilterXfered)
						{
							// update the search window if it's open
							if ((*realTocH)->imapTOC) (*tocH)->sums[sumNum].u.virtualMess.virtualMBIdx = -1;
							if (!bHidden) InvalSum(tocH, sumNum);
						}
						else
						{
							if ((*realTocH)->sums[realSumNum].cache)
								HPurge((*realTocH)->sums[realSumNum].cache);
						}
					}
					if (err==euFilterStop||err==euFilterXfered) err = noErr;
					if (err || CommandPeriod) break;
				}
			}
			
			// we're done filtering
			IMAPStopFiltering(true);	 
			
			CloseProgress();
		}
		FiltersDecRef();
	}
	NotifyHelpers(0,eManFilter,tocH);
	if (initialCount>(*tocH)->count && !CommandPeriod) BoxSelectAfter((*tocH)->win,lastSelected);
	return(err);
}

/************************************************************************
 * FilterFlaggedMessages - filter the flagged messages in a mailbox
 ************************************************************************/
OSErr FilterFlaggedMessages(FilterKeywordEnum fType,TOCHandle tocH,FilterPBPtr fpb)
{
	short err=noErr;
	short sumNum;
	short countWas;
	short lastSelected = -1;
	short initialCount = (*tocH)->count;
	Boolean isOut = (*tocH)->which==OUT;
	uLong pTicks=0;
	long count = CountFlaggedMessages(tocH);
			
	fpb->doNotifyThing = fpb->notify = count;
		
	if (err=RegenerateFilters()) return(err);
	if (AnyFilters(fType))
	{
		if (count>5) OpenProgress();
		ProgressMessageR(kpTitle,FILTERING);
		ProgressMessageR(kpSubTitle,LEFT_TO_PROCESS);
		Progress(NoBar,count,nil,nil,nil);
		fpb->doNotifyThing = fpb->notify = 0;
		for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
		{
#ifdef SPEECH_ENABLED
			(void) SpeechIdle ();
#endif
			if (count>1) MiniEvents(); if (CommandPeriod) break;
			if ((*tocH)->sums[sumNum].flags&FLAG_UNFILTERED)
			{
				fpb->notify++;
				fpb->doNotifyThing++;
				if (!(--count%10) || TickCount()-pTicks>30)
				{
					Progress(NoBar,count,nil,nil,nil);
					pTicks = TickCount();
				}
				lastSelected = sumNum;
				countWas = (*tocH)->count;
				
				/*
				 * load up cache
				 */

				CacheMessage(tocH,sumNum);

				if (!(*tocH)->imapTOC)
				{
					if (!(*tocH)->sums[sumNum].cache)
					{
						WarnUser(MEM_ERR,MemError());
						err = 1;
					}
					else
						HNoPurge((*tocH)->sums[sumNum].cache);
				}
									
				/*
				 * do the filtering
				 */
				if (!err) err = FilterMessageLo(fType,tocH,sumNum,fpb,False);

				/*
				 * clean up after
				 */
				 
				if ((*tocH)->sums[sumNum].cache) HPurge((*tocH)->sums[sumNum].cache);
				if (err==euFilterStop||err==euFilterXfered) err = noErr;
				if (err || CommandPeriod) break;
			}
		}
		IMAPStopFiltering(true);
		CloseProgress();
	}
	FiltersDecRef();
	NotifyHelpers(0,eManFilter,tocH);
	if (initialCount>(*tocH)->count && !CommandPeriod) BoxSelectAfter((*tocH)->win,lastSelected);
	return(err);
}

/************************************************************************
 * FilterFlaggedMessagesIncrementally - spam score and filter messages in
 *	an IMAP mailbox a chunk at a time.
 ************************************************************************/
OSErr FilterIMAPTocIncrementally(TOCHandle tocH,FilterPBPtr fpb,Boolean noXfer)
{
	short err = noErr;
	short sumNum;
	short filterHogTicks = GetRLong(FILTER_HOG_TICKS);
	uLong startTick = TickCount();
	Boolean dirty = false;		
	long spamThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
	FSSpec mailboxspec;
	
	// init
	mailboxspec.name[0] = 0;
	
	// regenerate filters
	if (err=RegenerateFilters()) return(err);
	
beginFilter:

	for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
	{	
		// skip messages that don't need to be filtered
		if (((*tocH)->sums[sumNum].flags&FLAG_UNFILTERED) == 0)
			continue;
			
		//filter the whole darn thing if need more time and we got the idle time 
		//we need more time if we didn't filter any messages and we ran out of time

		if (EventPending())
			return kNotEnoughTime;
		if (TickCount() - startTick > filterHogTicks)	
			if (dirty) return kNotEnoughTime;

#ifdef SPEECH_ENABLED
		(void) SpeechIdle ();
#endif
		if (CommandPeriod) break;
		CheckSLIP();
		fpb->doNotifyThing++;
		fpb->notify++;
		
		//
		// score the message.  We'll junk it when filtering completes.
		//
		if (HasFeature(featureJunk) && CanScoreJunk())
		{
			JunkScoreIMAPBox(tocH, sumNum, sumNum, false);
			
			// Junk it if we ought to
			if (JunkPrefBoxHold() && ((*tocH)->sums[sumNum].spamScore >= spamThresh))
			{
				(*tocH)->sums[sumNum].flags &= ~FLAG_UNFILTERED;
				ZapHandle((*tocH)->sums[sumNum].cache);
				fpb->doNotifyThing--;
				err = euFilterXfered;
			}
		}										
			
		
		//
		// do the filtering
		//
		if (!err) 
		{
			// only filter this message if it's not open
			if ((*tocH)->sums[sumNum].messH == NULL)
			{
				err = FilterMessageLo(flkIncoming,tocH,sumNum,fpb,noXfer);
				(*tocH)->sums[sumNum].flags &= ~FLAG_UNFILTERED;
			}
		}
		dirty = true;

		//
		// clean up after
		//
		if (err!=euFilterXfered)
		{
			// make sure we open the visible mailbox, not the hidden one.
			if (mailboxspec.name[0] == 0)
				GetRealIMAPSpec((*tocH)->mailbox.spec, &mailboxspec);
	
			// a message survived the filtering process and remained in this mailbox.
			// open this mailbox if we ought to ...
			if (mailboxspec.name[0])
			{
				if (!PrefIsSet(PREF_NO_OPEN_IN))
					AddSpecToList(&mailboxspec,fpb->mailbox);	
			}
			
			// cleanup
			if ((*tocH)->sums[sumNum].cache)
				ZapHandle((*tocH)->sums[sumNum].cache);
		}
		
		// make the summary visible or invisible as appropriate
		if (ShowHideFilteredSummary(tocH, sumNum))
			sumNum--;  // a message was removed from the toc
				
		if (err==euFilterStop||err==euFilterXfered) err = noErr;
		if (CommandPeriod) break;
	}
	
	// cancel all filtering if the user hit command-period
	if (CommandPeriod)
		IMAPFilteringCancelled(true);
	
	if (err)
	{
		Aprintf(OK_ALRT,Note,THREAD_PUNT_FILTER_ERR,err);
		NeedToFilterIMAP = false;	// give it a rest on error, maybe the user will fix
	}
	else
	{
		//
		// filtering completed
		//
		
		SetIMAPMailboxNeeds(TOCToMbox(tocH), kNeedsFilter, false);
		
		// Transfer spam to junk mailbox, if we're holding mail in the junk mailbox and 
		// we're running plugins.  Be sure to check the correct pers for the settings!
		PushPers(CurPers);
		CurPers = TOCToPers(tocH);
		if (CurPers && JunkPrefBoxHold() && !JunkPrefIMAPNoRunPlugins())
			MoveToIMAPJunk(tocH, -1, spamThresh, fpb);		
		PopPers();
	}

	CloseProgress();
	FiltersDecRef();

	return(err);
}

#ifdef NEVER
/************************************************************************
 * NotifyMA - let message assistant know the user wants to filter
 ************************************************************************/
OSErr NotifyMA(TOCHandle tocH)
{
	AliasHandle maAlias=nil;
	OSErr err;
	FSSpec spec;
	FSSpec setSpec;
	AEDesc listD, optD;
	
	NullADList(&listD,&optD,nil);
	
	if (!(err=MABuildDescriptors(tocH,&listD,&optD)) &&
			!(err=CreatorToSpec(MA_CREATOR,&spec)))
	{
		GetFileByRef(SettingsRefN,&setSpec);
		if (!(err=NewAlias(&spec,&setSpec,&maAlias)))
		{
			NotifyAHelper(maAlias,&listD,eManFilter,&optD);
		}
	}
	
	DisposeADList(&listD,&optD,nil);
	
	if (err) WarnUser(COULDNT_LAUNCH,err);
	return(err);
}

/************************************************************************
 * MABuildDescriptors - build descriptors for notifying 
 ************************************************************************/
OSErr MABuildDescriptors(TOCHandle tocH, AEDescPtr listD, AEDescPtr optD)
{
	short mNum;
	OSErr err;
	AEDesc messD;
	OSType messList;
	
	/*
	 * build list of messages
	 */
	if (!(err=AECreateList(nil,0,False,listD)))
	{
		for (mNum=0;mNum<(*tocH)->count;mNum++)
		{
			if (err=MessObjFromTOC(tocH,mNum,&messD)) break;
			if (err=AEPutDesc(listD,0,&messD)) break;
		}
	}

	if (!err && !(err=AECreateList(nil,0,False,optD)))
	{
		messList = keyEuMessList;
		AEPutPtr(optD,1,typeKeyword,&messList,sizeof(messList));
	}

	return(err);
}
#endif

/************************************************************************
 * GenSpecWindow - open the spec window
 ************************************************************************/
void GenSpecWindow(CSpecHandle specList)
{
	short n;
	short i;
	Str255 s;
	Str63 date;
	MyWindowPtr	win,
							frontWin;
	WindowPtr winWP,
						frontWinWP;
	FSSpec spec;
	long len;
	Str255 url;
	
	if (!specList || !*specList || !GetHandleSize_(specList)) return;
	
	TimeString(LocalDateTime(),False,date,nil);
	GetRString(s,SPEC_TITLE);
	
	/*
	 * find topmost filter window
	 */

	// Figure out if the front window is a nag.  If so, treat
	// the nag like a floater.

	frontWinWP = GetWindowList();
	frontWin = GetWindowMyWindowPtr (frontWinWP);
	for (winWP=frontWinWP;winWP;winWP=GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsKnownWindowMyWindow(winWP) && GetWindowKind(winWP)==TEXT_WIN && win->ro &&
		    !*(*(TextDHandle)GetWindowPrivateData(winWP))->spec.name)
			break;
	}
				
	if (!winWP && (win=OpenText(nil,nil,nil,frontWin && frontWin->isNag ? frontWinWP : nil,False,s,True,False)))
		win->position = (void*)PositionPrefsTitle;
	
	if (win)
	{
		winWP = GetMyWindowWindowPtr (win);
		ComposeRString(s,SPEC_INTRO,date);
		PeteAppendText(s+1,*s,win->pte);
		n = HandleCount(specList);
		for (i=0;i<n;i++)
		{
			spec = (*specList)[i].spec;
			ComposeRString(s,SPEC_FMT,spec.name,(*specList)[i].count);
			len = PeteLen(win->pte);
			PeteAppendText(s+1,*s,win->pte);
			InsertURLLo(win->pte,len,len+*s-1,MakeFileURL(url,&spec,ProtocolStrn+proCompFile));
			PETEMarkDocDirty(PETE,win->pte,False);
		}
		win->isDirty = False;
		win->ro = True;
		PeteSelect(win,win->pte,0x7fffffff,0x7fffffff);
		PeteLock(win->pte,0,PeteLen(win->pte),peModLock);
		PeteKillUndo(win->pte);
		PeteScroll(win->pte,0,0x7fff);
		if (!IsWindowVisible (winWP)) {
			InfiniteClip(GetWindowPort(winWP));
			ShowMyWindowBehind(winWP,frontWin && frontWin->isNag ? frontWinWP : nil);
		}
		PeteScroll(win->pte,0,0x7fff);
	}
}

/************************************************************************
 * FilterMessagesAfter - filter messages after a particular spot
 *	NOT IMAP SAFE
 ************************************************************************/
OSErr FilterMessagesFrom(FilterKeywordEnum fType,TOCHandle tocH,short startWith,FilterPBPtr fpb,Boolean noXfer)
{
	short err;
	short sumNum;
	short countWas;
	MyWindowPtr win;
	Boolean isOut = (*tocH)->which==OUT;
	short count;	
	short filterHogTicks = GetRLong(FILTER_HOG_TICKS);
	uLong startTick = TickCount();
	Boolean noInterruptions = false;
	Boolean deliveryBatch = fType==flkDelivery;
	Boolean dirty = false;
	FSSpec inSpec;
	Boolean isTempIn = (*tocH)->which==IN_TEMP;
	TOCHandle realInTocH = GetRealInTOC();
	
	if (!realInTocH) return(1);	// oops!

	if ((*tocH)->imapTOC) return(1);

	inSpec = GetMailboxSpec(realInTocH,-1);
	
	// if doing batch filtering but user is idle, do the whole shebang right now
	if (deliveryBatch)
	{
#ifdef EVER_WANT_MODAL_BG_FILTER
		if (CheckThreadRunning || (!InBG && (TickCount()-ActiveTicks) < GetRLong(LONG_MODAL_IDLE_SECS)*60))
#endif
			count = 0;	// incremental filtering, don't need count
#ifdef EVER_WANT_MODAL_BG_FILTER
		else
		{
			noInterruptions = true;	// user is idle; filter all now
			count = GetDeliveryCount();
		}
#endif
	}
	else
		count = (*tocH)->count-startWith;
	
	if (err=RegenerateFilters()) return(err);

	if (fType==flkDelivery)
		fType = flkIncoming;
beginFilter:

	if (AnyFilters(fType)) 
	{
		if (!deliveryBatch||noInterruptions)
		{
			OpenProgress();
		}
		ProgressR(NoBar,count,FILTERING,LEFT_TO_FILTER,nil);
		for (sumNum=startWith;sumNum<(*tocH)->count;sumNum++)
		{
			/*
			 filter the whole darn thing if need more time and we got the idle time 
			 we need more time if we didn't filter any messages and we ran out of time
			 */
			if (deliveryBatch && !noInterruptions)
			{
				if (EventPending())
					return kNotEnoughTime;
				if (TickCount() - startTick > filterHogTicks)	
				{
					if (dirty) return kNotEnoughTime;
				}
			}
			if (!deliveryBatch)
			MiniEvents(); 
#ifdef SPEECH_ENABLED
			(void) SpeechIdle ();
#endif
			if (CommandPeriod) break;
			CheckSLIP();
			fpb->doNotifyThing++;
			fpb->notify++;
			countWas = (*tocH)->count;
			Progress(NoBar,count--,nil,nil,nil);
			/*
			 * load up message or cache
			 */
			if (isOut)
			{
				win = GetAMessage(tocH,sumNum,nil,nil,False);
				if (!win) err = 1;
			}
			else
			{
				CacheMessage(tocH,sumNum);

				if (!(*tocH)->sums[sumNum].cache)
				{
					WarnUser(MEM_ERR,MemError());
					err = 1;
				}
				else
					HNoPurge((*tocH)->sums[sumNum].cache);
			}
			
			ASSERT(!JunkPrefBoxHold() || (*tocH)->sums[sumNum].spamScore < GetRLong(JUNK_MAILBOX_THRESHHOLD));
			
			/*
			 * do the filtering
			 */
			if (!err) err = FilterMessageLo(fType,tocH,sumNum,fpb,noXfer);
			dirty = true;
			/*
			 * clean up after
			 */
			if (err==euFilterXfered)
				 sumNum--;

			else
			{
				if (isOut)
				{
					WindowPtr	winWP	=	GetMyWindowWindowPtr(win);
					if (winWP && !IsWindowVisible (winWP)) CloseMyWindow(winWP);
				}
				else
				{
					if ((*tocH)->sums[sumNum].cache)
						HPurge((*tocH)->sums[sumNum].cache);
					ASSERT(realInTocH);
					if ((deliveryBatch || isTempIn) && realInTocH)
					{
						if (!(err=MoveMessageLo(tocH, sumNum, &inSpec, false, false, true)))
							sumNum--;
						else
						{
							WarnUser(WRITE_MBOX,err);	// may not want a second alert? maybe put in task progress window?
							break;
						}
					}
				}
			}
			if (err==euFilterStop||err==euFilterXfered) err = noErr;
			if (CommandPeriod) break;
		}
	}
	/* just transfer messages to in box */
	else
	{
		if ((deliveryBatch || isTempIn) && realInTocH)
		{
			count=(*tocH)->count;
		if (!deliveryBatch)
			OpenProgress();
			ProgressR(NoBar,count,MOVING_MESSAGES_TO_IN,LEFT_TO_MOVE,nil);
			for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
			{
				/*
				 filter the whole darn thing if need more time and we got the idle time 
				 we need more time if we didn't filter any messages and we ran out of time
				 */
				fpb->doNotifyThing++;
				fpb->notify++;
				if (!noInterruptions)
				{
					if (EventPending())
						return kNotEnoughTime;
					if (TickCount() - startTick > filterHogTicks)	
					{
						if (dirty) return kNotEnoughTime;
					}
				}
				Progress(NoBar,count--,nil,nil,nil);
				if (CommandPeriod) break;
				if (!(err=MoveMessageLo(tocH, sumNum, &inSpec, false, false, true)))
					sumNum--;
				else
				{
					WarnUser(WRITE_MBOX,err);	// may not want a second alert? maybe put in task progress window?
					break;
				}
				dirty = true;
			}
		}
		else if (realInTocH)
		{
			// just notify
			fpb->doNotifyThing = count;
			fpb->notify = count;
		}
	}
	if (deliveryBatch)
	{
		if (err)
		{
			Aprintf(OK_ALRT,Note,THREAD_PUNT_FILTER_ERR,err);
			NeedToFilterIn = false;	// give it a rest on error, maybe the user will fix
		}
	}
	CloseProgress();

	if (realInTocH) BoxFClose(realInTocH,true);
	FiltersDecRef();
	return(err);
}

/**********************************************************************
 * FilterMessage - filter a single message, including postprocessing
 **********************************************************************/
OSErr FilterMessage(FilterKeywordEnum fType,TOCHandle tocH,short sumNum)
{
	FilterPB fpb;
	OSErr err;
	
	InitFPB(&fpb,false,true);
	err = FilterMessageLo(fType,tocH,sumNum,&fpb,False);
	if (fType==flkIncoming && err!=euFilterXfered)
		TransferMenuChoice(TRANSFER_MENU,TRANSFER_IN_ITEM,tocH,sumNum,0,false);
	FilterPostprocess(fType,&fpb);
	
	// Hide the message if it was deleted from an IMAP mailbox.
	if ((fType == flkManual) && (*tocH)->imapTOC)
		ShowHideFilteredSummary(tocH, sumNum);
						
	return(err);
}


/************************************************************************
 * FilterMessageLo - filter a message; needs to be setup first
 ************************************************************************/
OSErr FilterMessageLo(FilterKeywordEnum fType,TOCHandle tocH,short sumNum,FilterPBPtr fpb,Boolean noXfer)
{
	short err;
	Boolean done=False;
	Str255 title;
	short oldCount = (*tocH)->count;
	Boolean oldSensitive = Sensitive;
	short f;
	short n;
	Boolean openIncomingErr=False;
	Boolean unjunking = fType==flkIncoming && (*tocH)->which==JUNK;
	
	Sensitive = False;	// do all filtering insensitively
	
	NukeXfUndo();
	MakeMessTitle(title,tocH,sumNum,True);
	ProgressMessage(kpMessage,title);
	CycleBalls();
	
	if (err=RegenerateFilters()) return(err);
	CacheMessage(tocH,sumNum);
	if (!(*tocH)->imapTOC)
	{
		if (!(*tocH)->sums[sumNum].cache)
		{
			WarnUser(GENERAL,MemError());
			return(1);
		}
		HNoPurge((*tocH)->sums[sumNum].cache);
	}

	openIncomingErr = (fType==flkIncoming && PrefIsSet(PREF_OPEN_IN_ERR_MESS) && ((*tocH)->which==IN || (*tocH)->which==IN_TEMP) && (*tocH)->sums[sumNum].state==MESG_ERR);

	if (AnyFilters(fType) || openIncomingErr)
	{
		Handle	saveFilters;
		short		filterIdx;
		
		InitFPB(fpb,true,false);
		fpb->tocH = tocH;
		fpb->sumNum = sumNum;
		fpb->openMessage = openIncomingErr;

		saveFilters = Filters;
		for(filterIdx=0;filterIdx<3 && !err;filterIdx++)
		{
			if (!(Filters = filterIdx==0 ? PreFilters : filterIdx==1 ? saveFilters : PostFilters)) continue;
			n = NFilters;
			for (f=0;f<n && !FGlobalErr;f++)
			{
				if (RightFilterType(fType,f))
				{
#ifdef SPEECH_ENABLED
					(void) SpeechIdle ();
#endif
					MiniEvents(); if (CommandPeriod) break; CycleBalls();
					if (FilterMatch(f,tocH,sumNum,fpb) && !FGlobalErr)
					{
						err = TakeFilterAction(f,fpb,noXfer);
						if (err) break;
					}
				}
			}
		}
		Filters = saveFilters;		
		Filter1Postprocess(fType,fpb);
	}
	FiltersDecRef();
	if (oldCount==(*tocH)->count && (*tocH)->sums[sumNum].cache)
		HPurge((*tocH)->sums[sumNum].cache);

	// Kill the message cache if we filled it with minimal headers for an IMAP filtering spree
	if (oldCount==(*tocH)->count && (*tocH)->sums[sumNum].cache && (*tocH)->sums[sumNum].offset < 0)
	{
		ZapHandle((*tocH)->sums[sumNum].cache);
		(*tocH)->sums[sumNum].cache = nil;
	}
		
	Sensitive = oldSensitive;
	
	if (err==euFilterStop && unjunking) err = noErr;
	
	return(err);
}

/************************************************************************
 * AddSpecToList - add an FSSpec to a list of FSSpecs
 ************************************************************************/
void AddSpecToList(FSSpecPtr spec,CSpecHandle specList)
{
	short n;
	CSpec cspec;
	
	if (!specList || !*specList) return;
	
	if (spec->parID == MailRoot.dirId && EqualStrRes(spec->name,TRASH)) return;
	
	n = HandleCount(specList);
	LDRef(specList);
	while(n--)
		if (SameSpec(&(*specList)[n].spec,spec))
		{
			(*specList)[n].count++;
			UL(specList);
			return;
		}
	UL(specList);
	
	cspec.spec = *spec;
	cspec.count = 1;
	PtrPlusHand_(&cspec,specList,sizeof(**specList));
}

/************************************************************************
 * TermMatch - does a message match a term?
 ************************************************************************/
Boolean TermMatch(MTPtr mt,TOCHandle tocH, short sumNum,FilterPBPtr fpb)
{
	UPtr text;
	long bodyOffset;
	UPtr end;
	UPtr spot;
	UPtr hEnd;
	Boolean match = False;
	long size;
	Boolean hasColon;
	Boolean foundHeader = False;
	UHandle cache = nil;
	Str31 headerName;
		
	/*
	 * match
	 */
	
	// Set things up to do filtering to or in an IMAP mailbox
	if (!IMAPStartFiltering(tocH, ((*tocH)->imapTOC && ((*tocH)->sums[sumNum].offset==-1))))
	{
		// failed?  Warn the user, and stop the filtering process.
		IMAPError(kIMAPSearching, kIMAPSelectMailboxErr, errIMAPSearchMailboxErr);
		CommandPeriod = true;
		return (false);
	}
	
	// if the message to be filtered has not been downloaded
	if ((*tocH)->imapTOC && ((*tocH)->sums[sumNum].offset==-1))
	{
		// if we're not looking at anything but headers
		if (mt->headerID!=FILTER_BODY)
		{
			// then download the headers and stick 'em in the cache
			if (!(*tocH)->sums[sumNum].cache)
			{
				Handle cache;
				
				if ((cache=IMAPFetchMessageHeadersForFiltering(tocH, sumNum)) != NULL)
				{
					HPurge(cache);
					(*tocH)->sums[sumNum].cache = cache;
				}
			}
			
			// make sure we've got a valid cache to work with
			if ((*tocH)->imapTOC && (!(*tocH)->sums[sumNum].cache) || !*((*tocH)->sums[sumNum].cache))
			{
				ASSERT(0);		// call John now!
				
				ComposeLogS(LOG_FILT,nil,"\pMissing Cache! %s sumnum %d uid %d",(*tocH)->mailbox.spec.name, sumNum, (*tocH)->sums[sumNum].uidHash);
				return (false);
			}
			
			if ((*tocH)->sums[sumNum].cache)
			{
				text = LDRef((*tocH)->sums[sumNum].cache);
				bodyOffset = GetHandleSize((*tocH)->sums[sumNum].cache);
			}
			else 
			{
				text = nil;
			}
		}
		// else
			// we'll need to search on the server
	}
	else
	{
		// this is a critical point to verify the cache so that we don't crash
		if (!(*tocH)->sums[sumNum].cache)
		{
			ASSERT(0);	// if this is IMAP, call John now!
			ComposeLogS(LOG_FILT,nil,"\pMissing Cache! %s sumnum %d uid %d",(*tocH)->mailbox.spec.name, sumNum, (*tocH)->sums[sumNum].uidHash);
			return false;
		}
		
		text = LDRef((*tocH)->sums[sumNum].cache);
		bodyOffset = (*tocH)->sums[sumNum].bodyOffset;
	}

	// make sure we've got a valid cache to work with
	if ((*tocH)->imapTOC && (!(*tocH)->sums[sumNum].cache) || !*((*tocH)->sums[sumNum].cache))
	{
		ASSERT(0);		// call John now!
		
		ComposeLogS(LOG_FILT,nil,"\pMissing Cache! %s sumnum %d uid %d",(*tocH)->mailbox.spec.name, sumNum, (*tocH)->sums[sumNum].uidHash);
		return (false);
	}
			
	if (*mt->header && mt->headerID!=FILTER_BODY)	/* we DO have a header to look for */
	{
		if (!striscmp(mt->header+1,"date"))
			match = TermDateMatch(mt,tocH,sumNum);
		else if (HasFeature(featureJunk) && EqualStrRes(mt->header,FiltMetaEnglishStrn+fmeJunk))
		{
			UseFeature(featureJunk);
			match = TermJunkMatch(mt,tocH,sumNum);
		}
		else if (EqualStrRes(mt->header,FiltMetaEnglishStrn+fmePersonality))
		{
			match = TermPersMatch(mt,tocH,sumNum);
		}
		else if (FromIntersectNickFile(mt,tocH,sumNum))	// performance special case
		{
			match = FromIntersectNickFileMatch(mt,tocH,sumNum);
		}
		// if this is an IMAP message that hasn't been downloaded, look on the server
		else if ((*tocH)->imapTOC && ((*tocH)->sums[sumNum].offset==-1) && !text)	
		{
			match = IMAPTermMatch(mt, &(*tocH)->sums[sumNum]);
		}		
		else
		{
			hasColon = nil!=strchr(mt->header+1,':');
			end = text + bodyOffset;
			size = end-text;
			for (spot=FindHeaderString(text,mt->header,&size,False);
					 spot;
					 spot=FindHeaderString(spot,mt->header,&size,False))
			{
				/* we found one */
				foundHeader = True;
				switch (mt->verb)
				{
					case mbmAppears: match = True; goto done; break;
					case mbmNotAppears: match = False; goto done; break;
				}
				
				/* find last char of header */
				for (hEnd=spot;hEnd<end;hEnd++)
					if (hEnd[0]=='\015' && !IsWhite(hEnd[1])) break;
				
				/*
				 * find first char of header
				 */
				if (!hasColon)
				{
					while (*spot!=':' && spot<hEnd) spot++;
					if (*spot==':') spot++; /* skip colon */
				}
				while(spot<hEnd && IsWhite(*spot)) spot++; /* skip initial white */
				
				/* match */
				match = TermPtrMatch(mt,spot,hEnd);
				if (!match && ((*tocH)->which==OUT || ((*tocH)->sums[sumNum].flags&FLAG_OUT)))
				{
					UPtr hnStart,hnEnd;
					short hid;
					UHandle addrs=nil;
					
					for (hnEnd=spot;hnEnd>text&&*hnEnd!=':';hnEnd--);
					for (hnStart=hnEnd;hnStart>text&&hnStart[-1]!='\015';hnStart--);
					MakePStr(headerName,hnStart,hnEnd-hnStart+1);
					if (StringSame(headerName,fpb->cc)) hid = CC_HEAD;
					else if (StringSame(headerName,fpb->bcc)) hid = BCC_HEAD;
					else if (StringSame(headerName,fpb->to)) hid = TO_HEAD;
					else hid = 0;
					if (hid)
					{
						switch(hid)
						{
							case CC_HEAD: addrs = fpb->ccAddresses; break;
							case TO_HEAD: addrs = fpb->toAddresses; break;
							case BCC_HEAD: addrs = fpb->bccAddresses; break;
							default: ZapHandle(addrs); break;
						}
						match = TermExpMatch(mt,spot,hEnd,&addrs);
						if (addrs)
							switch(hid)
							{
								case CC_HEAD: fpb->ccAddresses = addrs; break;
								case TO_HEAD: fpb->toAddresses = addrs; break;
								case BCC_HEAD: fpb->bccAddresses = addrs; break;
								default: ZapHandle(addrs); break;
							}
					}
				}
				
				if (match)
				{
					switch (mt->verb)
					{
						case mbmIsnt:
						case mbmNotContains:
						case mbmNotIntersects:
						case mbmNotIntersectsFile:
							match=False;
					}
					goto done;
				}
				spot = hEnd+1;
				size = end-spot;
			}
			match = mt->verb==mbmIsnt||mt->verb==mbmNotContains||mt->verb==mbmNotAppears||mt->verb==mbmNotIntersects||mt->verb==mbmNotIntersectsFile;
		}
	}
	else
	{
		// if this is an IMAP message that hasn't been downloaded, check the server.
		if ((*tocH)->imapTOC && ((*tocH)->sums[sumNum].offset==-1))	
		{
			match = IMAPTermMatch(mt, &(*tocH)->sums[sumNum]);
		}
		else
		{
			spot = text + bodyOffset + 1;
		  	end = text + GetHandleSize((*tocH)->sums[sumNum].cache);
			match = TermPtrMatch(mt,spot,end);
			switch (mt->verb)
			{
				case mbmIsnt:
				case mbmNotContains:
				case mbmNotIntersects:
				case mbmNotIntersectsFile:
					match=!match;
			}
		}
	}
		
done:
	UL((*tocH)->sums[sumNum].cache);
	return(match);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean TermExpMatch(MTPtr mt, UPtr spot, UPtr end,UHandle *cache)
{
	BinAddrHandle raw=nil;
	BinAddrHandle expanded=nil;
	Boolean result = False;
	
	if (cache)
	{
		if (*cache)
			if (!**cache) ZapHandle(*cache);
			else
			{
				expanded = *cache;
				HNoPurge(expanded);
			}
	}
	
	if (!expanded)
		if (!SuckPtrAddresses(&raw,spot,end-spot,True,False,False,nil))
			if (!ExpandAliases(&expanded,raw,0,True))
				FlattenListWith(expanded,',');
	
	if (expanded)
	{
		LDRef(expanded);
		result = TermPtrMatch(mt,*expanded,*expanded+GetHandleSize(expanded));
		UL(expanded);
		if (cache)
		{
			//HPurge(expanded);
			*cache = expanded;
			expanded = nil;
		}
	}
	
	ZapHandle(raw);
	ZapHandle(expanded);
	return(result);
}

/************************************************************************
 * DoesIntersectNick - does a string intersect a nickname?
 ************************************************************************/
Boolean DoesIntersectNick(UHandle nickAddresses,UHandle nickExpanded,UPtr spot,long len)
{
	UHandle addresses = nil;
	Boolean match = False;
	UPtr nick,addr;
	OSErr err = SuckPtrAddresses(&addresses,spot,len,False,False,False,nil);
	
	if (err>0 || err==paramErr) return(False);	// syntax error in header; intersects won't work
	if (!(FGlobalErr=err))
	{
		for (addr = LDRef(addresses); *addr; addr += *addr + 2)
		{
			for (nick=LDRef(nickAddresses); *nick; nick += *nick + 2)
				if (match=StringSame(nick,addr)) goto done;
			for (nick=LDRef(nickExpanded); *nick; nick += *nick + 2)
				if (match=StringSame(nick,addr)) goto done;
		}
	}
done:
	ZapHandle(addresses);
	return(match);
}

/************************************************************************
 * FromIntersectNickFile - Is this the special case of matching from address
 *  against nickname file?
 ************************************************************************/
Boolean FromIntersectNickFile(MTPtr mt, TOCHandle tocH,short sumNum)
{
	return
		(mt->verb==mbmIntersectsFile || mt->verb==mbmNotIntersectsFile)
		&& !striscmp(mt->header+1,"from")
		&& ValidHash((*tocH)->sums[sumNum].fromHash);
}

/************************************************************************
 * FromIntersectNickFileMatch - Special case routine for matching from address
 *  against nickname file
 ************************************************************************/
Boolean FromIntersectNickFileMatch(MTPtr mt, TOCHandle tocH,short sumNum)
{
	PStr file = nil;
	Boolean match;
	
	if (*mt->value  && !EqualStrRes(mt->value,ANY_ALIAS_FILE)) file = mt->value;
	
	match = HashAppearsInAliasFile((*tocH)->sums[sumNum].fromHash,file);
	
	if (mt->verb==mbmNotIntersectsFile) match = !match;
	
	return match;
}

/************************************************************************
 * DoesIntersectNickFile - does a string intersect a nickname file?
 ************************************************************************/
Boolean DoesIntersectNickFile(PStr file,UPtr spot,long len)
{
	UHandle addresses = nil;
	Boolean match = False;
	UPtr addr;
	OSErr err = SuckPtrAddresses(&addresses,spot,len,False,False,False,nil);
	
	if (file && (!*file || EqualStrRes(file,ANY_ALIAS_FILE)))
		file = nil;	// tells stuff below to match all files
	
	if (err>0 || err==paramErr) return(False);	// syntax error in header; intersects won't work
	if (!(FGlobalErr=err))
	{
		for (addr = LDRef(addresses); *addr; addr += *addr + 2)
			if (match=AppearsInAliasFile(addr,file)) break;
	}
done:
	ZapHandle(addresses);
	return(match);
}

/************************************************************************
 * TermPtrMatch - does the term match a particular string?
 ************************************************************************/
Boolean TermPtrMatch(MTPtr mt,UPtr spot,UPtr end)
{
	Boolean match=False;
	
	switch(mt->verb)
	{
		case mbmRegEx:
			if (!mt->regex)
			{
				PtoCcpy(P1,mt->value);
				mt->regex = regcomp(P1);
			}
			if (mt->regex)
			{
				match = 0 <= SearchRegExpPtr(LDRef(mt->regex),spot,0,end-spot);
				UL(mt->regex);
			}
			break;

		case mbmNotContains:
		case mbmContains:
			match = !PrefIsSet(PREF_NO_FILT_LWSP) ? PPtrMatchLWSP(mt->value,spot,end-spot,false,false) :
							nil!=PPtrFindSub(mt->value,spot,end-spot);
			break;
			
		case mbmIsnt:
		case mbmIs:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value,spot,end-spot,true,true);
			else if (end-spot != *mt->value) match=False;
			else match = 0==strincmp(mt->value+1,spot,*mt->value);
			break;
			
		case mbmStarts:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value,spot,end-spot,true,false);
			else if (end-spot<*mt->value) match = False;
			else match = 0==strincmp(mt->value+1,spot,*mt->value);
			break;
			
		case mbmEnds:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value,spot,end-spot,false,true);
			else if (end-spot<*mt->value) match = False;
			else match = 0==strincmp(mt->value+1,end-*mt->value,*mt->value);
			break;
			
		case mbmNotAppears:
		case mbmAppears:
			match = True;
			break;
		
		case mbmNotIntersects:
		case mbmIntersects:
		{
			/*
			 * cache nickname expansion
			 */
			if (!mt->nickExpanded || !*mt->nickExpanded || !mt->nickAddresses || !*mt->nickAddresses)
			{
				ZapHandle(mt->nickExpanded);
				ZapHandle(mt->nickAddresses);
				if (!(FGlobalErr=SuckPtrAddresses(&mt->nickAddresses,mt->value+1,*mt->value,False,False,False,nil)))
					FGlobalErr=ExpandAliases(&mt->nickExpanded,mt->nickAddresses,0,False);
			}
			
			/*
			 * did we get them?
			 */
			if (mt->nickExpanded && mt->nickAddresses)
			{
				HNoPurge(mt->nickExpanded);
				HNoPurge(mt->nickAddresses);
			}
			else return(False);
			
			/*
			 * test it
			 */
			match = DoesIntersectNick(mt->nickAddresses,mt->nickExpanded,spot,end-spot);
			
			/*
			 * allow it to be purged if need be
			 */
			HPurge(mt->nickExpanded);
			HPurge(mt->nickAddresses);
			UL(mt->nickExpanded);
			UL(mt->nickAddresses);
		}
			break;
		
		case mbmNotIntersectsFile:
		case mbmIntersectsFile:
		{
			/*
			 * test it
			 */
			match = DoesIntersectNickFile(mt->value,spot,end-spot);
		}
			break;
			
		default:
			ASSERT(1);
			break;
	}
	return(match);
}

/************************************************************************
 * TermDateMatch - does the date of a message match a term?
 ************************************************************************/
Boolean TermDateMatch(MTPtr mt, TOCHandle tocH, short sumNum)
{
	Str255 s;
	Boolean match;
	
	ComputeLocalDate(&(*tocH)->sums[sumNum],s);
	match = TermPtrMatch(mt,s+1,s+*s+1);
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * TermPersMatch - does the personality of a message match a term?
 ************************************************************************/
Boolean TermPersMatch(MTPtr mt, TOCHandle tocH, short sumNum)
{
	Str255 s;
	Boolean match;
	
	PCopy(s,(*PERS_FORCE(TS_TO_PERS(tocH,sumNum)))->name);
	if (!*s) GetRString(s,DOMINANT);
	match = TermPtrMatch(mt,s+1,s+*s+1);
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * TermJunkMatch - does the junk score of a message match a term?
 ************************************************************************/
Boolean TermJunkMatch(MTPtr mt, TOCHandle tocH, short sumNum)
{
	long num;
	
	StringToNum(mt->value,&num);
	
	switch (mt->verb)
	{
		case mbmIs: return (*tocH)->sums[sumNum].spamScore == num; break;
		case mbmIsnt: return (*tocH)->sums[sumNum].spamScore != num; break;
		case mbmJunkMore: return (*tocH)->sums[sumNum].spamScore > num; break;
		case mbmJunkLess: return (*tocH)->sums[sumNum].spamScore < num; break;
		default: return false; break;
	}
}

/************************************************************************
 * TermPriorMatch - does the priority of a message match a term?
 ************************************************************************/
Boolean TermPriorMatch(MTPtr mt, TOCHandle tocH, short sumNum)
{
	Str255 s;
	Boolean match;
	short priority;
	
	priority = (*tocH)->sums[sumNum].priority;
	priority = Prior2Display(priority);
	if (priority==3) return(mt->verb==mbmIsnt || mt->verb==mbmNotContains);
	PriorityHeader(s,priority);
	match = TermPtrMatch(mt,s+1,s+*s+1);
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * AnyFiltersLo - do any filters match the given filtertype?
 ************************************************************************/
Boolean AnyFiltersLo(FilterKeywordEnum fType,Handle hFilters)
{
	Handle	saveFilters = Filters;
	Boolean	result = false;
	short n;
	short f;
	
	Filters = hFilters;
	n = NFilters;
	for (f=0;f<n;f++)
	{
		if (RightFilterType(fType,f))
		{
			result = true;
			break;
		}
	}
	Filters = saveFilters;
	return result;
}

/**********************************************************************
 * HaveManualFilters - does the user have any manual filters
 **********************************************************************/
Boolean HaveManualFilters(void)
{
	Boolean result;
	
	if (RegenerateFilters()) return false;
	
	result = AnyFilters(flkManual);
	
	FiltersDecRef();
	
	return result;
}

/************************************************************************
 * AnyFilters - do any filters match the given filtertype?
 ************************************************************************/
Boolean AnyFilters(FilterKeywordEnum fType)
{
	return AnyFiltersLo(fType,Filters) || AnyFiltersLo(fType,PreFilters) || AnyFiltersLo(fType,PostFilters);
}

/************************************************************************
 * RightFilterType - is this filter of the right type
 ************************************************************************/
Boolean RightFilterType(FilterKeywordEnum fType,short filter)
{
	switch(fType)
	{
		case flkIncoming: return(FR[filter].incoming);
		case flkOutgoing: return(FR[filter].outgoing);
		case flkManual: return(FR[filter].manual);
	}
	/* better not get here... */
	ASSERT(1);
	return(False);
}

/************************************************************************
 * FilterMatchHi - does a message match a filter?
 ************************************************************************/
Boolean FilterMatchHi(short f,TOCHandle tocH,short sumNum)
{
	FilterPB fpb;
	Handle cache;
	Boolean match = false;
	
	CacheMessage(tocH,sumNum);
	cache = (*tocH)->sums[sumNum].cache;
	if (cache)
	{
		HNoPurge(cache);
		Zero(fpb);
		
		match = FilterMatch(f,tocH,sumNum,&fpb);
				
		ZapHandle(fpb.ccAddresses);
		ZapHandle(fpb.bccAddresses);
		ZapHandle(fpb.toAddresses);
		HPurge(cache);

		// Purge the message cache if we filled it with minimal headers for an IMAP filtering spree
		if ((*tocH)->sums[sumNum].cache && (*tocH)->sums[sumNum].offset < 0)
		{
			ZapHandle((*tocH)->sums[sumNum].cache);
			(*tocH)->sums[sumNum].cache = nil;
		}
	}
	return(match);
}

/************************************************************************
 * FilterMatch - does a message match a filter?
 ************************************************************************/
Boolean FilterMatch(short filter,TOCHandle tocH,short sumNum,FilterPBPtr fpb)
{
	MatchTerm term;
	Boolean match;
	Boolean result;
	
	term = FR[filter].terms[0];
	match = *term.header ? TermMatch(&term,tocH,sumNum,fpb) : false;
	FR[filter].terms[0] = term;						//store back any cached info
	if (FGlobalErr) result = False;
	else
	{
		term = FR[filter].terms[1];
		if (!*term.header) result = match;
		else
			switch(FR[filter].conjunction)
			{
				case cjIgnore:
					result = match;
					break;
				
				case cjAnd:
					if (!match) result = False;
					else result = TermMatch(&term,tocH,sumNum,fpb);
					break;
		
				case cjOr:
					if (match) result = True;
					else result = TermMatch(&term,tocH,sumNum,fpb);
					break;
				
				case cjUnless:
					if (!match) result = False;
					else result = !TermMatch(&term,tocH,sumNum,fpb);
					break;
			}
		FR[filter].terms[1] = term;						//store back any cached info
	}
	
	if (result && (LogLevel&LOG_FILT))
		FiltLogMatch(filter,tocH,sumNum);
	
	// Purge the message cache if we filled it with minimal headers for an IMAP filtering spree.
	// Note, only do this if we successfully matched.  Otherwise, we'll take care of it when filtering is through.
	if (result && (*tocH)->sums[sumNum].cache && (*tocH)->sums[sumNum].offset < 0)
	{
		ZapHandle((*tocH)->sums[sumNum].cache);
		(*tocH)->sums[sumNum].cache = nil;
	}
		
	return(result);
}

/**********************************************************************
 * 
 **********************************************************************/
void FiltLogMatch(short filter,TOCHandle tocH,short sumNum)
{
	Str31 name;
	Str255 title;
	
	PSCopy(name,FR[filter].name);
	MakeMessTitle(title,tocH,sumNum,True);
	ComposeLogR(LOG_FILT,nil,FILT_LOG_FMT,name,title);
}

/************************************************************************
 * NonSequitur - change the subject
 ************************************************************************/
void NonSequitur(PStr subject, TOCHandle tocH, short sumNum)
{
	Str255 newSub;
	Str127 oldSub;
	Str63 replace;
	Str31 brackets;
	UPtr ampr;
	
	GetRString(brackets,SUBJ_TRIM_STR);
	GetRString(replace,SUBJ_REPLACE);
	PCopy(oldSub,(*tocH)->sums[sumNum].subj);

	if (ampr = PPtrFindSub(replace,subject+1,*subject))
	{
		MakePStr(newSub,subject+1,ampr-subject-1);
		if (StartsWith(subject,brackets))
		{
			BMD(newSub+1+*brackets,newSub+1,*newSub-*brackets);
			*newSub -= *brackets;
			TrimSquares(oldSub,true,true);
			TrimAllWhite(oldSub);
			TrimInternalWhite(oldSub);
		}
		PSCat(newSub,oldSub);
		*ampr = *subject-(ampr-subject);
		PSCat(newSub,ampr);
	}
	else PSCopy(newSub,subject);
	SetSubject(tocH,sumNum,newSub);
}


/**********************************************************************
 * FilterLastMatch - return the time a filter was last used
 **********************************************************************/
uLong FilterLastMatch(short filter)
{
	FUHandle fuh = GetResource_(FU_TYPE,FU_ID);
	uLong when = 0;
	FUPtr fup;
	long id = FR[filter].fu.id;
	
	if (!fuh || !id) return(0);	/* not known */
	for (fup=(FUPtr)LDRef(fuh) + GetHandleSize_(fuh)/sizeof(FilterUse)-1;fup>=*fuh;fup--)
	{
		if (fup->id==id)
		{
			when = fup->lastMatch;
			break;
		}
	}
	UL(fuh);
	return(when);
}

/**********************************************************************
 * FilterLastMatchHi - return the time a filter was last used, using cache if there
 **********************************************************************/
uLong FilterLastMatchHi(short filter)
{
	uLong secs;
	
	if (FR[filter].fu.lastMatch) return(FR[filter].fu.lastMatch);
	if (secs = FilterLastMatch(filter))
		FR[filter].fu.lastMatch = secs;
	return(secs);
}

/**********************************************************************
 * FilterNoteMatch - note that a filter has matched
 **********************************************************************/
OSErr FilterNoteMatch(short filter, long secs)
{
	FUHandle fuh = GetResource_(FU_TYPE,FU_ID);
	FUPtr fup;
	OSErr err=fnfErr;
	FilterUse fu;
	long id =  FR[filter].fu.id;

	FR[filter].fu.lastMatch = secs;
	
	if (id)
	{
		/*
		 * if resource doesn't exist, add it
		 */
		if (!fuh)
		{
			fuh = NuHandle(0);
			if (!fuh) return(MemError());
			AddMyResource_(fuh,FU_TYPE,FU_ID,"");
			if (err=ResError()) return(err);
		}
		
		/*
		 * find existing record
		 */
		HNoPurge_(fuh);
		err = fnfErr;
		for (fup=(FUPtr)LDRef(fuh) + GetHandleSize_(fuh)/sizeof(FilterUse)-1;fup>=*fuh;fup--)
		{
			if (fup->id==id)
			{
				fup->lastMatch = secs;
				err = noErr;
			}
		}
		UL(fuh);
		
		/*
		 * if didn't find, add
		 */
		if (err==fnfErr)
		{
			fu.id = id;
			fu.lastMatch = secs;
			err = PtrPlusHand_(&fu,fuh,sizeof(fu));
		}
	
		/*
		 * make sure it gets written out sometime
		 */
		ChangedResource((Handle)fuh);
	}
	return(err);
}


/************************************************************************
 * TakeFilterAction - take an action that has been deemed necessary
 ************************************************************************/
OSErr TakeFilterAction(short filter,FilterPBPtr fpb,Boolean noXfer)
{
	short err = noErr;
	FActionHandle fa;
	short pass;
	
	FilterNoteMatch(filter,GMTDateTime());
	
	for (pass=0;pass<MAX_FILTER_PASS && !err;pass++)
	{
		for (fa=FR[filter].actions;!err && fa;fa=(*fa)->next)
		{
			if (FAPass((*fa)->action)==pass && !(noXfer && (*fa)->action==flkTransfer))
			{
				err = CallAction(faeDo,fa,nil,fpb);
			}
		}
	}
	
	return(err);
}


#endif
