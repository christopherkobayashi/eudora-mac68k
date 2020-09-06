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

/*
	File: filtthread.c
	Author: Clarence Wong <cwong@qualcomm.com>
	Date: October 1997 - ...
	Copyright (c) 1997 by QUALCOMM Incorporated 

	Comments:
		these functions support filtering in a threaded environment
		for now, that means filtering in the main thread
		these functions were scattered all over the place. they really belong together in a separate file. 		
*/

#define FILE_NUM 108

#ifdef THREADING_ON
#include "filtthread.h"
#include "junk.h"
#pragma segment FILTTHREAD

static void FilterRealOutTOC (void);
static int GetFCCs(MessHandle messH, CSpecHandle fccSpecs);
#ifdef BATCH_DELIVERY_ON
static OSErr  FilterDeliveryTOCs(void);


/**********************************************************************
 * FilterDeliverTOCs - 
 **********************************************************************/
static OSErr FilterDeliveryTOCs(void)
{
	OSErr err = noErr;
	static FilterPB fpb;
	TOCHandle tocH;
	static Boolean needToOpenIn;
	
	if (NeedToFilterIn)
	{
		do
		{
			if (tocH=GetNextDeliveryTOC())  
			{
				/* initialize the fpb */	
				InitFPB(&fpb,true,!fpb.message);

				//	Score the incoming mail
				//		if we're not in lite mode, _and_
				//		We have the feature turned on, _and_
				//		The user has not asked to "just download headers".
				if ( HasFeature ( featureJunk ) && !NoXfer )
				{
					JunkScoreBox ( tocH, -1, -1, false );
					if (JunkPrefBoxHold ())
						err = MoveToJunk ( tocH, GetRLong ( JUNK_MAILBOX_THRESHHOLD ), &fpb );
				}										
				
				if ((*tocH)->count)
				{
					// moodmail?
					if ( AnalDoIncoming ())
						AnalBox(tocH,-1,-1);

					err = FilterMessagesFrom(flkDelivery,tocH,0,&fpb,NoXfer);
					
					if (!err) NeedToNotify = true;
				}
				
				if (!err)
				{
					// kill the mailbox
					FSSpec deliverSpec = GetMailboxSpec(tocH,-1);
					if ((*tocH)->count)
						; // we'll catch the mail next time around
					else
					{
						// kill the delivery mailbox now that it's empty
						needToOpenIn = true;
						if ((*tocH)->win) CloseMyWindow(GetMyWindowWindowPtr((*tocH)->win));
						FSpDelete(&deliverSpec);	
						Box2TOCSpec(&deliverSpec,&deliverSpec);
						FSpDelete(&deliverSpec);
					}
				}
				else if (err == kNotEnoughTime)
					break;
			}
			else NeedToFilterIn = 0;
		}
		while (NeedToFilterIn && (InBG || (TickCount()-ActiveTicks > GetRLong(SHORT_MODAL_IDLE_SECS)*60))); // if things are slow, go around again
	}
	else if (NeedToFilterIMAP)
	{
		GetNextWaitingIMAPToc(&tocH);
		if (tocH)
		{
			// Initialize the fpb 
			InitFPB(&fpb,true,!fpb.message);				
			
			// display some progress ...
			IMAPFilterProgress(tocH);
			
			// Filter imap messages, spam scoring them as we go.
			err = FilterIMAPTocIncrementally(tocH,&fpb,NoXfer);
				
			if (!err) 
			{
				TOCSetDirty(tocH,true);	
				NeedToNotify = true;
			}
			
			// was filtering cancelled?
			IMAPFilteringCancelled(false);
		}
		else 
		{
			// no more progress needed
			IMAPFilterProgress(NULL);
			
			// filtering is complete
			NeedToFilterIMAP = 0;
		}
	}
	else if (NeedToNotify && fpb.message) /* do the post processing if we have time */
	{
		if (!CheckThreadRunning && !IMAPCheckThreadRunning && (InBG || (TickCount()-ActiveTicks > GetRLong(SHORT_MODAL_IDLE_SECS)*60)))
		{
			NeedToNotify = false;
			if (fpb.report && GetHandleSize_(fpb.report)) {GenSpecWindow(fpb.report);ZapHandle(fpb.report);}
			if (fpb.doNotifyThing||fpb.openMessage||fpb.openMailbox) gNewMessages++;	// there's at least one message to notify the user about
			NotifyNewMailLo(fpb.doNotifyThing||fpb.openMessage||fpb.openMailbox,NoXfer,GetRealInTOC(),&fpb,needToOpenIn);		
			NotifyHelpers(0,eHasConnected,nil);
			needToOpenIn = false;
		}
	}
	else NeedToNotify = false;
	return err;
}

/**********************************************************************
 * GetNextDeliveryTOC - 
 **********************************************************************/
TOCHandle GetNextDeliveryTOC(void)
{
	Str63 name;
	FSSpec deliverSpec,
				deliverFolder;
	CInfoPBRec hfi;
	long minFileNum=0x7fffffff, fileNum;	
	TOCHandle tocH = nil;
		
	if (SubFolderSpec(DELIVERY_FOLDER,&deliverFolder))
		return tocH;	

	/* find lowest numbered toc file in delivery folder */
tryAgain:
	Zero(deliverSpec);
	Zero(hfi);
	Zero(name);
	hfi.hFileInfo.ioNamePtr = name;
	while (!DirIterate(deliverFolder.vRefNum,deliverFolder.parID,&hfi))
	{	
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==MAILBOX_TYPE)	
		{
			StringToNum(name, &fileNum);
			if (fileNum < minFileNum)
			{
				SimpleMakeFSSpec(deliverFolder.vRefNum,deliverFolder.parID,name,&deliverSpec);
				minFileNum = fileNum;			
			}
		}
		else if (hfi.hFileInfo.ioFlFndrInfo.fdType==TOC_TYPE)
			; // ignore it for now
		else if ((!hfi.hFileInfo.ioFlFndrInfo.fdType || hfi.hFileInfo.ioFlFndrInfo.fdType=='    ' || hfi.hFileInfo.ioFlFndrInfo.fdType=='MBOX') 
						&& !(hfi.hFileInfo.ioFlAttrib&0x10))
		{
			// if the type is funky, but we "know" it's a mailbox, fix it
			if (hfi.hFileInfo.ioFlFndrInfo.fdType!='MBOX' && PIndex(name,'.'))
				hfi.hFileInfo.ioFlFndrInfo.fdType = TOC_TYPE;
			else
				hfi.hFileInfo.ioFlFndrInfo.fdType = MAILBOX_TYPE;
			if (!HSetFInfo(deliverFolder.vRefNum,deliverFolder.parID,name,&hfi.hFileInfo.ioFlFndrInfo))
			{
				CycleBalls();
				hfi.hFileInfo.ioFDirIndex--;	// try again
			}
		}
	}
	// Check in advance to see if this is a mailbox
	if (deliverSpec.name[0])
		if (IsMailbox(&deliverSpec))
			GetTOCByFSS(&deliverSpec, &tocH);
		else
		{
			MisplaceItem (&deliverSpec);
			goto tryAgain;
		}
		
	if (tocH)
		(*tocH)->which = DELIVERY_BATCH;
	return tocH;
}
#endif
/**********************************************************************
 * FilterXferMessages - 
 **********************************************************************/
void FilterXferMessages (void)
{
	Boolean oldDFWTC = DFWTC;

#ifdef NEVER
	if (RunType!=Production) Dprintf("\pFilterXferMessages NFI %d NFO %d NNN %d",NeedToFilterIn,NeedToFilterOut,NeedToNotify);
#endif
	
	DFWTC = true;
	if (FilterDeliveryTOCs() != kNotEnoughTime)
	{
		FilterRealOutTOC ();
	}
	DFWTC = oldDFWTC;
//	SetNeedToFilterOut();
#ifndef BATCH_DELIVERY_ON
#ifdef TASK_PROGRESS_ON
	if (TaskProgressWindow)
	{
		if (!NeedToFilterIn)
		{
				RemoveFilterTask();
				if (!GetNumBackgroundThreads() && !TaskDontAutoClose && PrefIsSet(PREF_TASK_PROGRESS_AUTO)  && !IsWazoo(TaskProgressWindow))
				{
					CloseMyWindow(TaskProgressWindow);
					return;
				}
			}
		if (GetWindowKind(GetMyWindowWindowPtr(TaskProgressWindow))==TASKS_WIN)
			InvalContent(TaskProgressWindow);
		}
#endif
#endif
}

/**********************************************************************
 * GetDeliveryCount - 
 **********************************************************************/
 #ifdef BATCH_DELIVERY_ON
short GetDeliveryCount(void)
{
	short count=0;
	CInfoPBRec hfi;
	Str31 name;
	FSSpec spec,
				deliverFolder;
	TOCType tocPart;

	if (SubFolderSpec(DELIVERY_FOLDER,&deliverFolder))
		return count;

	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	while (!DirIterate(deliverFolder.vRefNum,deliverFolder.parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==MAILBOX_TYPE)
		{
			SimpleMakeFSSpec(deliverFolder.vRefNum,deliverFolder.parID,name,&spec);
			if (!PeekTOC(&spec,&tocPart))
				count+=tocPart.count;		
		}
	}
	return count;
}
#endif

/**********************************************************************
 * SetNeedToFilterOut - 
 **********************************************************************/
void SetNeedToFilterOut (void)
{
	TOCHandle realOutTocH = GetRealOutTOC();
	short ii;
	
	NeedToFilterOut = false;
	ASSERT (realOutTocH);
	if (!realOutTocH)
		return;
	for (ii=(*realOutTocH)->count-1;ii>=0;ii--)
		if (((*realOutTocH)->sums[ii].flags&FLAG_UNFILTERED) && (((*realOutTocH)->sums[ii].state==SENT) || ((*realOutTocH)->sums[ii].state==MESG_ERR)))
		{
			NeedToFilterOut=true;
			break;
		}
}

/************************************************************************
 * FilterRealOutTOC - 	if we sent mail, go through all unfiltered messages: 
 										set state, fcc, filter, delete 
 ************************************************************************/
static void FilterRealOutTOC (void)
{
	int ii,
			mesgCount = 0;
	short		sum = 0;
	TOCHandle tocH;
	MessHandle messH;
#ifndef BATCH_DELIVERY_ON
	MyWindowPtr win;
#endif
	CSpecHandle fccList; 
	StateEnum state;
	Boolean dirty = false;
#ifdef BATCH_DELIVERY_ON
	short filterHogTicks;
	uLong startTick = TickCount();
	static Boolean needMoreTime = false;
	Boolean noInterruptions = false;
	static FilterPB fpb;

	if (!NeedToFilterOut)
	{
		/*  do the post processing if we have time */
		if (!SendThreadRunning &&  (InBG || (TickCount()-ActiveTicks > GetRLong(SHORT_MODAL_IDLE_SECS)*60)))
			FilterPostprocess(flkOutgoing,&fpb);
		return;
	}
	filterHogTicks = GetRLong(FILTER_HOG_TICKS);
	/* return if needMoreTime and we haven't been idle long enough */
	if (needMoreTime)
	{
		if (SendThreadRunning || (!InBG && (TickCount()-ActiveTicks) < GetRLong(LONG_MODAL_IDLE_SECS)*60))
			return;
		noInterruptions = true;
	}

	/* initialize the fpb */	
	InitFPB(&fpb,true,!fpb.message);
#endif

	if  (!(tocH = GetRealOutTOC ()))
		return;
	
#ifdef BATCH_DELIVERY_ON
	if (needMoreTime)	
	{
		OpenProgress();
		needMoreTime = false;
	}
#endif
	ProgressMessageR(kpSubTitle,PROCESSING_OUT);
	for (ii = 0; ii < (*tocH)->count; ii++)
	{
#ifdef BATCH_DELIVERY_ON
		/*
		 filter the whole darn thing if need more time and we got the idle time 
		 we need more time if we didn't filter any messages and we ran out of time
		 */
		if (!noInterruptions)
		{
			if (EventPending())
				return;
			if (TickCount() - startTick > filterHogTicks && dirty)	// filter at least one message SD 2/19/02
			{
				if (!dirty)
					needMoreTime = true;
				else
				return;
			}
		}
#endif
			state = (*tocH)->sums[ii].state;
			if ((*tocH)->sums[ii].flags&FLAG_UNFILTERED)
			{
				dirty = true;
				(*tocH)->sums[ii].flags &= ~FLAG_UNFILTERED;
				if (state==MESG_ERR)
				{
#ifdef BATCH_DELIVERY_ON
				/* add erroneous message to fpb list */
				FSSpec	spec = GetMailboxSpec(tocH,-1);
				AddSpecToList(&spec,fpb.message);
				fpb.openMessage = true;					
				(*tocH)->sums[ii].opts |= OPT_OPEN;
#else
					// if mesg error banner not visible, will need to refresh message
					messH = (*tocH)->sums[ii].messH;
					if (messH && (win=(*messH)->win) && (!FindControlByRefCon(win,mcMesgErrors)))
						CloseMyWindow(win);
					GetAMessage(tocH,ii,nil,True);
#endif					
				}
				else
				if (state==SENT)
				{
					// fcc
					messH = (*tocH)->sums[ii].messH;
					if (!messH)
					{
						if (!OpenComp(tocH,ii,nil,nil,False,False)) continue;  // error
						messH = (*tocH)->sums[ii].messH;
					}
					if (messH)
					{
						if (HasFeature (featureFcc)) {
							fccList = NuHandle(0);
							if (GetFCCs (messH, fccList)) continue; // something wrong if this fails
							if (fccList && GetHandleSize_(fccList))
									DoFcc(tocH,ii,fccList);
							ZapHandle(fccList);	// zap it
						}
						// filter
#ifdef BATCH_DELIVERY_ON
					if (FilterMessageLo(flkOutgoing,tocH,ii,&fpb,False) == euFilterXfered)
#else
					if (FilterMessage(flkOutgoing,tocH,ii) == euFilterXfered)
#endif
							ii--;
						else 
						// delete message
						{
							messH = (*tocH)->sums[ii].messH;
							if (((*tocH)->sums[ii].flags&FLAG_KEEP_COPY)==0)
							{
								if (messH && MessOptIsSet(messH,OPT_ATT_DEL)) CompAttDel(messH);

								if (messH && (*messH)->win) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
								DeleteMessage(tocH,ii,False);
								ii--; 			/* back up, since we deleted the message */
								RedoTOC(tocH);	/* keep nit-pickers happy */
							}
							else if (messH && (*messH)->win) CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));				
						}
					}
				}
			}
			if (CommandPeriod)
			break;	// return;
		}	
		if (dirty)
			TOCSetDirty(tocH,true);
#ifdef BATCH_DELIVERY_ON
	if (!CommandPeriod)
		NeedToFilterOut = 0;
#endif

	IMAPStopFiltering(true);	//We may have filtered a message to an IMAP mailbox

	// Generate the filter report at this point, too.
	if (fpb.report && GetHandleSize_(fpb.report)) {GenSpecWindow(fpb.report);SetHandleSize(fpb.report,0);}
}

/************************************************************************
 * GetFCCs 
 ************************************************************************/
static int GetFCCs(MessHandle messH, CSpecHandle fccSpecs)
{
	UHandle addresses=nil;
	UHandle rawAddresses;
	UPtr address;
	HeadSpec hs;
	UHandle text;
	
	if (!HasFeature (featureFcc))
		return (noErr);
		
	if (CompHeadFind(messH,BCC_HEAD,&hs) && !CompHeadGetText(TheBody,&hs,&text))
	{
		if (!SuckAddresses(&rawAddresses,text,False,True,False,nil))
		{
			if (**rawAddresses)
			{
				ExpandAliases(&addresses,rawAddresses,0,False);
				ZapHandle(rawAddresses);
				if (!addresses) return(-1);
				for (address=LDRef(addresses); *address; address += *address + 2)
				{
					/*
					 * skip groups
					 */
					if (address[*address]==':' || address[1]==';') continue;	/* skip group identifiers */

					/*
					 * handle Fcc's
					 */
					if (IsFCCAddr(address))
					{
						if (fccSpecs)
						{
							if (AddFccToList(address,fccSpecs)) break;
						}
						continue;
					}
			}
				ZapHandle(addresses);
			}
			else
				ZapHandle(rawAddresses);
		}
		ZapHandle(text);
	}
	return(noErr);
}

#endif
