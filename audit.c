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

#include "audit.h"
#include "auditdefs.h"
#define FILE_NUM 124
/* Copyright (c) 1999 by Qualcomm, Inc */
OSErr Audit(short auditType,...);
OSErr AuditFlush(Boolean force);
OSErr GenerateAuditMessage(DialogPtr theDialog);
void AuditEntryStart(short auditType, Str255 into);
OSErr AddNonPersonalSettings(MessHandle messH);
OSErr InsertAuditAccuText(MessHandle messH, Str255 auditPrefix, Accumulator *a);
OSErr InsertShortSystemConfig(MessHandle messH);
OSErr InsertDemographicData(MessHandle messH);

#define NUM_PREFS_PER_LINE 8	// number of prefs listed in one non-personal settings audit entry

/************************************************************************
 * AuditFlush - write audit information out to the world
 ************************************************************************/
OSErr AuditFlush(Boolean force)
{
	FSSpec auditSpec;
	Str255 scratch;
	OSErr err;
	short refN;
	Boolean created;
	
	if (!AuditAccu.offset) return noErr;	// nothing to do

	if (!force && !DiskSpunUp()) return noErr;	// if the disk isn't spun up, bail
		
	// find/create the file
	err = FSMakeFSSpec(Root.vRef,Root.dirId,GetRString(scratch,AUDIT_FILE),&auditSpec);
	if (!err)
	{
		CInfoPBRec hfi;
		if (!FSpGetHFileInfo(&auditSpec,&hfi))
		{
			if (LocalDateTime()-hfi.hFileInfo.ioFlCrDat > GetRLong(AUDIT_NUKE_DAYS)*24*3600)	// nuke after a week (default)
				if (!(err=FSpDelete(&auditSpec))) err = fnfErr;	// pretend we didn't find it after all
		}
	}
	if (created = err==fnfErr) err = FSpCreate(&auditSpec,'MPS ','TEXT',smSystemScript);
	
	// open it
	if (!err) err = FSpOpenDF(&auditSpec,fsRdWrPerm,&refN);
	if (!err)
	{
		long bytes = AuditAccu.offset;
		
		if (created) FSWriteP(refN,GetRString(scratch,AUDIT_RELAX));
		
		SetFPos(refN,fsFromLEOF,0);
		err = FSWrite(refN,&bytes,LDRef(AuditAccu.data));
		UL(AuditAccu.data);
		AuditAccu.offset = 0;
		if (AuditAccu.size > 1 K) AccuTrim(&AuditAccu);
		err = FSClose(refN);
	}

	// all done
	return(err);
}

/************************************************************************
 * Audit - remember some audit info
 ************************************************************************/
OSErr Audit(short auditType,...)
{
	Str255 into;
	OSErr err;
	va_list args;

	AuditEntryStart(auditType, into);
	err = AccuAddStr(&AuditAccu,into);
	if (err) return(err);
	
	// variable stuff
	va_start(args,auditType);
	(void) VaComposeRString(into,AuditFmtStrn+auditType,args);
	va_end(args);
	
	return(AccuAddStr(&AuditAccu,into));
}

/************************************************************************
 * AuditEntryStart - set up an audit entry
 ************************************************************************/
void AuditEntryStart(short auditType, Str255 into)
{
	DateTimeRec dtr;

	// date/product stamp
	SecondsToDate(GMTDateTime(),&dtr);
	ComposeRString(into,AUDIT_INTRO_FORMAT,
		(dtr.year%100)/10,(dtr.year%100)%10,dtr.month/10,dtr.month%10,dtr.day/10,dtr.day%10,
		dtr.hour/10,dtr.hour%10,dtr.minute/10,dtr.minute%10,
		kMyProductCode,auditType);
}

/************************************************************************
 * Face Time Measurement
 ************************************************************************/
typedef struct FaceMeasureBlock
{
	uLong absStartTime;
	uLong startTime;
	uLong endTime;
	uLong faceTime;
	uLong rearTime;
	uLong connectTime;
	Boolean active;
	FMBHandle next;
} FaceMeasureBlock;

FMBHandle FMBList;

typedef enum {ASEInactive, ASEActive, ASEBackground} AppStateEnum;

uLong IntervalStart;
uLong LastActivity;
AppStateEnum CurrentAppState;

OSErr FaceMeasureUpdate(FMBHandle fmb);

/************************************************************************
 * NewFaceMeasure - make a new FMB
 ************************************************************************/
FMBHandle NewFaceMeasure(void)
{
	FMBHandle newOne = NewZH(FaceMeasureBlock);
	if (!newOne) return nil;
	LL_Queue(FMBList,newOne,(FMBHandle));
	return newOne;
	}

/************************************************************************
 * FaceMeasureBegin - start measurement in an fmb
 ************************************************************************/
OSErr FaceMeasureBegin(FMBHandle fmb)
{
	uLong secs = GMTDateTime();
	if (!fmb) return(nilHandleErr);
	(*fmb)->startTime = secs;
	if (!(*fmb)->absStartTime) (*fmb)->absStartTime = secs;
	(*fmb)->active = true;
	return noErr;
}

/************************************************************************
 * FaceMeasureEnd - end measurement of a particular block
 ************************************************************************/
OSErr FaceMeasureEnd(FMBHandle fmb)
{
	if (!fmb) return(nilHandleErr);
	if ((*fmb)->active)
	{
		uLong secs = GMTDateTime();
		FaceMeasureUpdate(fmb);
		(*fmb)->active = false;
		(*fmb)->endTime = secs;
	}
	return(noErr);		
}

/************************************************************************
 * FaceMeasureReset - reset timers, start over
 ************************************************************************/
OSErr FaceMeasureReset(FMBHandle fmb)
{
	FMBHandle	saveLink;
	
	if (!fmb) return(nilHandleErr);
	
	saveLink = (*fmb)->next;
	ZeroHandle(fmb);
	(*fmb)->next = saveLink;
	FaceMeasureBegin(fmb);
	return(noErr);		
}

/************************************************************************
 * DisposeFaceMeasure - get rid of one
 ************************************************************************/
OSErr DisposeFaceMeasure(FMBHandle fmb)
{
	if (!fmb) return nilHandleErr;
	LL_Remove(FMBList,fmb,(FMBHandle));
	ZapHandle(fmb);
	return noErr;
}

/************************************************************************
 * FaceMeasureReport - report what's in an FMB
 ************************************************************************/
OSErr FaceMeasureReport(FMBHandle fmb,uLong *faceTime,uLong *rearTime,uLong *connectTime,uLong *totalTime)
{
	OSErr err;
	
	if (!fmb) return(nilHandleErr);
	if (err = FaceMeasureUpdate(fmb)) return err;
	if (faceTime) *faceTime = (*fmb)->faceTime;
	if (rearTime) *rearTime = (*fmb)->rearTime;
	if (connectTime) *connectTime = 30*(((*fmb)->connectTime+15)/30);
	if (totalTime) *totalTime = ((*fmb)->active ? GMTDateTime():(*fmb)->endTime) - (*fmb)->absStartTime;
	return err;
}

/************************************************************************
 * FaceMeasureUpdate - update an individual face time block
 ************************************************************************/
OSErr FaceMeasureUpdate(FMBHandle fmb)
{
	if (!fmb) return nilHandleErr;
	
	if ((*fmb)->active)
	{
		uLong secs = GMTDateTime();
		uLong startSecs = MAX((*fmb)->startTime,IntervalStart);

#ifdef DEBUG
		if (BUG11 && secs>startSecs) secs = startSecs + 60*(secs-startSecs);
#endif
		
		switch (CurrentAppState)
		{
			case ASEInactive: break;	// nothing to do
			case ASEActive:
				if (secs > startSecs)	// don't do negative if clock is set back
					(*fmb)->faceTime += secs - startSecs;
				break;
			case ASEBackground:
				if (secs > startSecs)
					(*fmb)->rearTime += secs - startSecs;
				break;
		}
		if (SurfsUp()) (*fmb)->connectTime += secs-startSecs;
		(*fmb)->startTime = secs;
	}
	return noErr;
}

/************************************************************************
 * HaveFace - is the user watching us right now?
 ************************************************************************/
Boolean HaveFace(void)
{
	return (CurrentAppState==ASEActive);
}

/************************************************************************
 * SurfsUp - is the net connected?
 ************************************************************************/
Boolean SurfsUp(void)
{
	static uLong ticks;
	static Boolean connected = false;
	Boolean newConnected = connected;
	
	// if we're doing stuff, we know the net is connected
	if (gActiveConnections>0)
	{
		ticks = TickCount();
		newConnected = true;
	}
	// don't check more than once every 30 seconds; that's enough data
	else if (TickCount() - ticks > 30*60)
	{
		newConnected = !TCPWillDial(false);
		ticks = TickCount();
	}

	if (connected != newConnected)
	{
		connected = newConnected;
		AuditConnect(connected);
	}
	
	return connected;
}

/************************************************************************
 * FMBEventFilter - make the facetime measurement stuff work
 ************************************************************************/
Boolean FMBEventFilter(EventRecord *event,Boolean oldResult)
{
	FMBHandle fmb;
	AppStateEnum newAppState;
	static Point oldWhere;
	static Boolean wasConnected;
	Boolean newConnected = SurfsUp();
	static uLong lastCall;
	
	//	See if we aren't getting called like we should. We should be
	//	called several times a second. If 15 seconds goes by without a 
	//	call, offset facetime timers.
	if (lastCall && GMTDateTime()-lastCall > 15)
	{
		//	We've been in la-la-land for a while
		for (fmb=FMBList;fmb;fmb=(*fmb)->next)
			(*fmb)->startTime += GMTDateTime()-lastCall;		
	}
	lastCall = GMTDateTime();

	// Determin the new application state
	if (InBG)
		newAppState = ASEBackground;
	else
	{		
		switch (event->what)
		{
			// app in foreground, user doing stuff
			case keyDown:
			case keyUp:
			case mouseDown:
			case mouseUp:
			case app1Evt:
				LastActivity = GMTDateTime();
				break;
			// if user not obviously doing stuff, see if mouse has moved
			default:
				if (ABS(event->where.h-oldWhere.h)>2 || ABS(event->where.v-oldWhere.v)>2)
				{
					LastActivity = GMTDateTime();
					oldWhere = event->where;
				}
				break;
		}
		
		if (GMTDateTime() - LastActivity > kFaceIntervalAfter)
			newAppState = ASEInactive;
		else
			newAppState = ASEActive;
	}
	
	
	// If the app state changes, update our records
	if (CurrentAppState!=newAppState || newConnected!=wasConnected)
	{
		//ComposeLogS(-1,nil,"\pFMBFilter Current %d New %d LastActivity %x IntervalStart %x Connected %d",CurrentAppState,newAppState,LastActivity,IntervalStart,newConnected);
		for (fmb=FMBList;fmb;fmb=(*fmb)->next)
			FaceMeasureUpdate(fmb);
		CurrentAppState = newAppState;
		IntervalStart = GMTDateTime();
		if (CurrentAppState==ASEActive) IntervalStart -= kFaceIntervalBefore;
		wasConnected = newConnected;
	}
		
	return(oldResult);	// we never change the event
}

/************************************************************************
 * RandomSendAudit - Ask user if ok to send audit stats
 ************************************************************************/
void RandomSendAudit(void)
{
	uLong thresh = GetRLong(AUDIT_SEND_THRESH);
	uLong rand = Random()+0x8000;

	if (0x10000/rand > thresh) AskSendAudit();
}


extern ModalFilterUPP DlgFilterUPP;
Boolean AuditHitProc (EventRecord *event, DialogPtr theDialog, short itemHit,long refcon);
OSErr AuditInitProc(DialogPtr theDialog,long refcon);
/************************************************************************
 * AskSendAudit - Ask the user if it's ok to send the audit stats
 ************************************************************************/
void AskSendAudit(void)
{
#ifndef THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
	Nag(AUDIT_XMIT,AuditInitProc,AuditHitProc,DlgFilterUPP,false, nil);
#endif
}

#define kAuditDlogSend 1
#define kAuditDlogCancel 2
#define kAuditDlogFirstCheck 4
/************************************************************************
 * AuditHitProc - handle item hits
 ************************************************************************/
Boolean AuditHitProc(EventRecord *event, DialogPtr theDialog, short itemHit, long refcon)
{
	switch (itemHit) {
		case kAuditDlogSend:
			GenerateAuditMessage(theDialog);
		case kAuditDlogCancel:
			return true;
			break;
		default:
			SetDItemState(theDialog,itemHit,!GetDItemState(theDialog,itemHit));
			break;
	}
	return (false);
}

/************************************************************************
 * AuditInitProc - init dialog by turning on all the checkboxes
 ************************************************************************/
OSErr AuditInitProc(DialogPtr theDialog,long refcon)
{
	short item, itemType;
	Handle itemH;
	Rect rect;
	
	for (item=kAuditDlogFirstCheck;;item++)
	{
		GetDialogItem(theDialog, item, &itemType, &itemH, &rect);
		if (itemType!=chkCtrl+ctrlItem) break;
		SetDItemState(theDialog,item,true);
	}
	return noErr;
}

/************************************************************************
 * GenerateAuditMessage - generate the message from the log file
 ************************************************************************/
OSErr GenerateAuditMessage(DialogPtr theDialog)
{
	MyWindowPtr win;
	MessHandle messH;
	Str255 s;
	LineIOD lid;
	OSErr err = userCanceledErr;
	long len;
	short facility;
	Boolean okCats[MaxAuditType];
	Boolean anyOK = false;
	Boolean anyUsage = false;
	Boolean anySettings = false;
	Boolean demoGraphic = false;
	Handle itemH;
	Rect rect;
	short item, cat, itemType;
	
	// Check the checkboxes
	for (cat=0;cat<MaxAuditType;cat++) okCats[cat] = false;
	
	for (item=kAuditDlogFirstCheck;;item++)
	{
		GetDialogItem(theDialog, item, &itemType, &itemH, &rect);
		if (itemType!=chkCtrl+ctrlItem) break;
		if (GetDItemState(theDialog,item))
		{
			anyOK = true;
			for (cat=0;cat<MaxAuditType;cat++)
			{
				if (AuditCategories[cat]==item-kAuditDlogFirstCheck+1) 
				{
					okCats[cat] = true;
					
					// figure out what we're actually going to audit.
					if (okCats[cat] && (cat+1==kAuditNonPersonalSettings)) anySettings = true;
					else if (okCats[cat] && (cat+1==kAuditNonPersonalSysInfo)) anySettings = true;
					else if (okCats[cat] && (cat+1==kAuditDemographicData)) demoGraphic=true;
					else anyUsage = true;
				}
			}	
		}
	}
	
	if (anyOK)
	{
		Handle legend;
		
		if (win=DoComposeNew(nil))
		{
			WindowPtr	winWP = GetMyWindowWindowPtr (win);
			
			messH = Win2MessH(win);
			GetRString(s,AUDIT_SEND_ADDR);
			SetMessText(messH,TO_HEAD,s+1,*s);
			GetRString(s,AUDIT_SUBJECT);
			SetMessText(messH,SUBJ_HEAD,s+1,*s);
			
			// Audit intro for the curious user
			GetRString(s,AUDIT_RELAX); PeteAppendText(s+1,*s,TheBody);
			GetRString(s,AUDIT_PLEASE_SEND); PeteAppendText(s+1,*s,TheBody);
					
			// Non-personal settings audit
			if (anySettings) err = AddNonPersonalSettings(messH);
			
			// Demographic data
			if (demoGraphic) err = InsertDemographicData(messH);
			
			// Usage statistics audit
			if (anyUsage)
			{
				// Open the file...
				if (!(err=OpenLine(Root.vRef,Root.dirId,GetRString(s,AUDIT_FILE),fsRdPerm,&lid)))
				{
					while ((err=GetLine(s+1,255,&len,&lid))>0)
						if (len > 14 && isdigit(s[1]))
						{
							*s = len;
							facility = atoi(s+1 + 10 + 4) - 1;	// audit facilities are 1-based in the usage file
							if (facility < MaxAuditType && okCats[facility])
							{
								if (err=PeteAppendText(s+1,*s,TheBody)) break;
							}
						}
				}
			}
			
			// Audit log legend
			if (legend=GetResource('TEXT',AUDIT_LEGEND_TEXT))
				PETEInsertTextHandle(PETE,TheBody,PeteLen(TheBody),legend,GetHandleSize(legend),0,nil);
					
			if (err)
			{
				PeteCleanList(TheBody);
				CloseMyWindow(winWP);
			}
			else
				ShowMyWindow(winWP);
		}
		else err = MemError();
	}
	return(err);
}

/**********************************************************************
 * AddNonPersonalSettings - slap in non personal settings
 **********************************************************************/
OSErr AddNonPersonalSettings(MessHandle messH)
{
	OSErr err = noErr;
	short i, numAdded;
	Str255 s, scratch;

	// insert a quick blurb about the system configuration
	InsertShortSystemConfig(messH);
					
	// insert the settings themselves
	numAdded = 0;
	for (i=PREF_0;(err == noErr) && (i<PREF_STRN_LIMIT);i++)
	{
		if (PrefAudit(i))
		{
			// 8 settings per line ...
			if ((numAdded%NUM_PREFS_PER_LINE)==0)
			{
				AuditEntryStart(kAuditNonPersonalSettings, s);
				if (numAdded >0 ) err = PeteAppendText(Cr+1,*Cr,TheBody);
				if (err == noErr) err = PeteAppendText(s+1,*s,TheBody);
			}
			
			if (err == noErr)
			{
				GetPref(scratch, i);
				ComposeString(s,"\p%d %p ",i,scratch[0]?scratch:NoStr);
				err = PeteAppendText(s+1,*s,TheBody);
				numAdded++;
			}
		}
	}
	
	if (err == noErr) err = PeteAppendText(Cr+1,*Cr,TheBody);
	
	return (err);
}

/**********************************************************************
 * InsertShortSystemConfig - insert the short version of the sys config
 **********************************************************************/
OSErr InsertShortSystemConfig(MessHandle messH)
{
	OSErr err = noErr;
	Str255 auditPrefix;
	Accumulator a;

	AuditEntryStart(kAuditNonPersonalSysInfo, auditPrefix);

	if (((err=AccuInit(&a))==noErr) && ((err=AddConfig(&a,true))==noErr))
	{
		err = InsertAuditAccuText(messH, auditPrefix, &a);
	}
	else AccuZap(a);

	return (err);
}

/**********************************************************************
 * InsertDemographicData - insert the demographic data
 **********************************************************************/
OSErr InsertDemographicData(MessHandle messH)
{
	OSErr err = noErr;
#ifdef NOT_YET
	Str255 auditPrefix;
	Accumulator a;
	Handle profileData = nil;

#warning	Update the Audit Legend to explain the demographic data entries!
	
	// get the demographic data
	profileData = GetProfileData();
	if (profileData && *profileData)
	{
		AuditEntryStart(kAuditDemographicData, auditPrefix);

		if ((err=AccuInit(&a))==noErr)
		{
			if ((err=AccuAddHandle(&a, profileData))==noErr)
			{
				err = InsertAuditAccuText(messH, auditPrefix, &a);
			}
		}

		// Cleanup
		if (err != noErr)
		{
			AccuZap(a);
		}
	}
#endif	
	return (err);
}

/**********************************************************************
 * InsertAuditAccuText - insert text from an accumulalor into the audit
 *	message
 **********************************************************************/
OSErr InsertAuditAccuText(MessHandle messH, Str255 auditPrefix, Accumulator *a)
{
	long offset;
	OSErr err = noErr;
	
	// add the audit prefix string to the beginning of each line ...
	for(offset = 0; (err==noErr) && (offset < a->offset); ++offset) 
	{
		err = AccuInsertPtr(a, &auditPrefix[1], auditPrefix[0], offset);
		if (err == noErr)
			while ((offset < a->offset) && ((*a->data)[offset]!=Cr[1])) offset++;
	}

	if (err == noErr)
	{
		// clean it up ...
		AccuTrim(a);
		
		// put it all in the audit message ...
		PETEInsertTextHandle(PETE,TheBody,PeteLen(TheBody),a->data,GetHandleSize(a->data),0,nil);
	}

	return (err);
}