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

#include <conf.h>
#include <mydefs.h>

#include "buildtoc.h"
#define FILE_NUM 45
/* Copyright (c) 1991-1992 by the University of Illinois Board of Trustees */

#pragma segment BuildTOC

	void GleanFrom(UPtr line,MSumPtr sum);
	short SalvageTOC(TOCHandle old, TOCHandle new);
	PStr TrimWrap(UPtr str,int openC,int closeC);
	PStr TrimNonWord(PStr str);
	short MonthNum(CStr cp);
void FixOrigOffset(MSumPtr sum);
long CStr2Zone(char *s);
Boolean IsBulk(UPtr line);

/************************************************************************
 * RebuildTOC - rebuild a corrupt or out-of-date toc, salvaging what we
 * can.
 ************************************************************************/
TOCHandle RebuildTOC(FSSpecPtr spec,TOCHandle oldTocH, Boolean resource, Boolean tempBox)
{
	TOCHandle newTocH=nil;
	short oldCount, newCount, salvCount;
	
	if (oldTocH)
		DamagedTOC = oldTocH;
	else
	{
		if (resource)
			ReadRForkTOC(spec,&DamagedTOC);
		else
			ReadDForkTOC(spec,&DamagedTOC);
	}
	
	if (DamagedTOC || tempBox || !PrefIsSet(PREF_TOC_REBUILD_ALERTS) && AlertStr(TOC_SALV_ALRT,Stop,spec->name)==OK)
	{
		/*
		* build the new one
		*/
		ComposeLogR(LOG_ALRT|LOG_TOC,nil,REBUILDING_TOC,spec->name);
		if ((newTocH = BuildTOC(spec)) && DamagedTOC)
		{
			oldCount = (*DamagedTOC)->count;
			newCount = (*newTocH)->count;
			if (oldCount && newCount)
			{
				Str255 s;
				salvCount = SalvageTOC(DamagedTOC,newTocH); 		/* and salvage */
#ifdef	IMAP	
				if ((*newTocH)->imapTOC) SalvageIMAPTOC(DamagedTOC, newTocH, &newCount);	// salvage the minimal headers and clean it up.
#endif
				if (!tempBox)
				{
#ifdef	IMAP
					if ((*newTocH)->imapTOC)
						ComposeRString(s,IMAP_SALV_REPORT,newCount,oldCount,oldCount,oldCount-newCount,oldCount-newCount);
					else
#endif
						ComposeRString(s,SALV_REPORT,salvCount,oldCount,oldCount,newCount-salvCount,newCount-salvCount);
					Log(LOG_ALRT|LOG_TOC,s);
				}
			}
			BMD((*DamagedTOC)->sorts,(*newTocH)->sorts,sizeof((*newTocH)->sorts));
			(*newTocH)->unused = (*DamagedTOC)->unused;
			(*newTocH)->listFocus = (*DamagedTOC)->listFocus;
			(*newTocH)->laurence = (*DamagedTOC)->laurence;
			(*newTocH)->lastSort = (*DamagedTOC)->lastSort;
			(*newTocH)->pluginKey = (*newTocH)->pluginKey;
			(*newTocH)->pluginValue = (*DamagedTOC)->pluginValue;
			(*newTocH)->previewHi = (*DamagedTOC)->previewHi;
			(*newTocH)->nextSerialNum = (*DamagedTOC)->nextSerialNum;
			(*newTocH)->unreadBase = (*DamagedTOC)->unreadBase;
			(*newTocH)->doesExpire = (*DamagedTOC)->doesExpire;
			(*newTocH)->expUnits = (*DamagedTOC)->expUnits;
			(*newTocH)->expInterval = (*DamagedTOC)->expInterval;
		}
	}
	
	ZapHandle(DamagedTOC);
	
	if (newTocH) (*newTocH)->reallyDirty = True;	// force a rewrite
	
	return(newTocH);
}

/************************************************************************
 * SalvageTOC - reconcile and old a new TOC
 ************************************************************************/
short SalvageTOC(TOCHandle old, TOCHandle new)
{
	short first,last,mid;
	MSumPtr oldSum;
	long offset;
	short salvaged=0;
	long bo;
	long seconds;

	LDRef(old); LDRef(new);
	(*old)->count = (GetHandleSize_(old)-(sizeof(TOCType)-sizeof(MSumType)))/
									sizeof(MSumType);
	
	for (mid=(*new)->count-1;mid>=0;mid--) (*new)->sums[mid].state = REBUILT;	// set state to rebuilt
	
	for (oldSum=(*old)->sums;oldSum<(*old)->sums+(*old)->count;oldSum++)
	{
		offset = oldSum->offset;
#ifdef	IMAP
		if ((*old)->imapTOC && (offset < 0)) continue;	//skip minimal headers
#endif
		first = 0; last=(*new)->count-1;
		for (mid=(first+last)/2;first<=last;mid=(first+last)/2)
			if (offset<(*new)->sums[mid].offset)
				last = mid -1;
			else if (offset==(*new)->sums[mid].offset)
				break;
			else
				first = mid+1;
		if (first<=last && (*new)->sums[mid].length==oldSum->length)
		{
			salvaged++;
			bo = (*new)->sums[mid].bodyOffset;	// preserve new bodyOffset
			seconds = (*new)->sums[mid].seconds;	// preserve new seconds
			(*new)->sums[mid] = *oldSum;
			(*new)->sums[mid].bodyOffset = bo;	// and restore it--old one might be trash
			// overwrite old seconds unless the message is timed queue.  If it's timed queue,
			// the old seconds is a much more precious commodity, and we'll take the risk that
			// it might be off.
			if ((*new)->sums[mid].state!=TIMED)
				(*new)->sums[mid].seconds = seconds;	// and restore it--old one might be trash
		}
#ifdef RESYNC_MID
		else if (oldSum->msgIdHash)
		{
			for (first=0;first<(*new)->count;first++)
				if (oldSum->msgIdHash == (*new)->sums[first].msgIdHash && oldSum->length == (*new)->sums[first].length)
				{
					salvaged++;
					bo = (*new)->sums[first].bodyOffset;	// preserve new bodyOffset
					offset = (*new)->sums[first].offset;
					(*new)->sums[first] = *oldSum;
					(*new)->sums[first].bodyOffset = bo;	// and restore it--old one might be trash
					(*new)->sums[first].offset = offset;	// and restore it--old one might be different
					(*new)->sums[first].seconds = seconds;	// and restore it--old one might be different
				}
		}
#endif
	}
	
	UL(old); UL(new);
	CleanseTOC(new);
	return(salvaged);
}

/**********************************************************************
 * BuildTOC - build a table of contents for a file.  The TOC is built
 * in memory.  This gets a little hairy in spots because the routine is
 * used both for received messages and messages under composition; but
 * the complication does not substantially affect the flow of the
 * function, so I let it stand.
 **********************************************************************/
TOCHandle BuildTOC(FSSpecPtr spec)
{
	TOCHandle tocH=nil;
	MSumType sum;
	Boolean isOut;
	Str255 scratch;
	LineIOD lid;
	OSErr err;
	short which = 0;

// need to find out if toc is temp in case it is corrupt
#ifdef THREADING_ON
	if (IsSpool(spec))
	{
		if (EqualStrRes(spec->name,IN_TEMP))
			which = IN_TEMP;
		else
			if (EqualStrRes(spec->name,OUT_TEMP))
			which = OUT_TEMP;
	}
	else
#endif
		ComposeLogS(LOG_TOC,nil,"\pBuildTOC(%p)",spec->name);
		
	Zero(lid);
	
	if (err=FSpExists(spec))
	{
		FileSystemError(OPEN_MBOX,spec->name,err);
		return(nil);
	}
	if (!IsMailbox(spec))
	{
#ifdef THREADING_ON
// if the temp tocs are corrupt we need to just delete and recreate them
		if ((which == OUT_TEMP) || (which == IN_TEMP))
		{
			if (err = FSpDelete(spec))
					DieWithError(CREATING_MAILBOX,err);
			CreateTempBox(which);
		}
		else
#endif
		{
			FileSystemError(NOT_MAILBOX,spec->name,0);
			return(nil);
		}
	}
	
	/*
	 * first, try opening the file
	 */
	if (err=OpenLine(spec->vRefNum,spec->parID,spec->name,fsRdWrPerm,&lid))
	{
		FileSystemError(OPEN_MBOX,spec->name,err);
		return(nil);
	}
	
	if ((tocH=NewZH(TOCType))==nil)
	{
		WarnUser(READ_MBOX,MemError());
		goto failure;
	}
	
	/*
	 * figure out for once and for all if we are an in or an out box
	 */
	isOut = False;
	if (!which && IsRoot(spec))
	{
		GetRString(scratch,IN);
		if (StringSame(scratch,spec->name))
			which = IN;
		else
		{
			GetRString(scratch,OUT);
			if (StringSame(scratch,spec->name))
			{
				which = OUT;
				isOut = True;
			}
			else
			{
				GetRString(scratch,TRASH);
				if (StringSame(scratch,spec->name))
					which = TRASH;
			}
		}
	}

	(*tocH)->which = which;
	(*tocH)->nextSerialNum = 1;
	
	ReadSum(nil,False,&lid,True);
	(*tocH)->building = True;
	while (!(err=ReadSum(&sum,isOut,&lid,True)))
	{
		if (!SaveMessageSum(&sum,&tocH))
		{
			WarnUser(SAVE_SUM,MemError());
			goto failure;
		}
	}
	if (err!=fnfErr) goto failure;
	ReadSum(nil,False,&lid,True);
	(*tocH)->building = False;

	/*
	 * success
	 */
	CloseLine(&lid);
	(*tocH)->refN = 0;
	(*tocH)->mailbox.spec = *spec;
	(*tocH)->version = CURRENT_TOC_VERS;
	(*tocH)->minorVersion = CURRENT_TOC_MINOR;
	TOCSetDirty(tocH,true);
#ifdef	IMAP
	// this is an IMAP toc if this mailbox is in one of the IMAP mailbox trees.
	LDRef(tocH);
	(*tocH)->imapTOC = IsIMAPMailboxFileLo(spec, &((*tocH)->imapMBH))?1:0;
	UL(tocH);
#endif
	SetHandleBig_(tocH,sizeof(TOCType) + 
		((*tocH)->count ? (*tocH)->count-1 : 0)*sizeof(MSumType));
	return (tocH);

failure:
	if (tocH != nil) ZapHandle(tocH);
	CloseLine(&lid);
	return(nil);
}

 /************************************************************************
 * ReadSum - read a summary, from the current position in the lineio
 * routines.
 ************************************************************************/
OSErr ReadSum(MSumPtr sum,Boolean isOut,LineIOP lip,Boolean lookEnvelope)
{ 
	static int type;
	Str255 line, toLine;
	static UHandle oldLine=nil;
	enum {BEGIN, IN_BODY, IN_HEADER, ERROR} state;
	Str255 duck;
	Str63 headerName;
	short headerIndex;
	UPtr spot;
	long secs;
	Str31 prior;
	Boolean lookPrior;
	long origZone;
	short senderHead=REAL_BIG;
	Str15 statusRead;
	short outFlags = DefaultOutFlags();
	short maybeOut;
	long len;
	Str15 xhtml, html, rich, flow, charset;
#ifdef NOBODY_SPECIAL
	Str15 sigIntro;
	Boolean afterSig = false;
	Boolean isWhite = true;
#endif NOBODY_SPECIAL
	
	if (!sum)
	{
		if (oldLine)
		{
			ZapHandle(oldLine);
			oldLine = nil;
		}
		return(noErr);
	}
		
	/*
	 * fetch the sender head candidates
	 */
	GetRString(statusRead,ALREADY_READ);
	GetRString(html,HTMLTagsStrn+htmlTag);
	GetRString(xhtml,EnrichedStrn+enXHTML);
	GetRString(rich,EnrichedStrn+enXRich);
	GetRString(flow,EnrichedStrn+enXFlowed);
	GetRString(charset,EnrichedStrn+enXCharset);
#ifdef NOBODY_SPECIAL
	GetRString(sigIntro,SIG_INTRO);
#endif // NOBODY_SPECIAL
	
	/*
	 * set up the priority stuff
	 */
	if (lookPrior = !PrefIsSet(PREF_NO_IN_PRIOR)) GetRString(prior,HEADER_STRN+PRIORITY_HEAD);

	/*
	 * now, read from it line by line
	 * break messages when sendmail-style From line is found
	 */
	state = BEGIN;
	WriteZero(sum,sizeof(MSumType));
	sum->state = UNREAD;
	sum->tableId = DEFAULT_TABLE;
	sum->persId = sum->popPersId = (*PERS_FORCE(CurPers))->persId;
	if (isOut) sum->flags = outFlags;
	if (PrefIsSet(PREF_SHOW_ALL)) sum->flags |= FLAG_SHOW_ALL;
	if(HasUnicode())
	{
		outFlags |= FLAG_UTF8;
		sum->flags |= FLAG_UTF8;
	}
	maybeOut = -1;
	*toLine = 0;
	while (oldLine || (type=GetLine(line,sizeof(line),&len,lip)))
	{
		if (oldLine)
		{
			len = GetHandleSize(oldLine)-1;
			BMD(*oldLine,line,len+1);
			ZapHandle(oldLine);
			oldLine = nil;
		}
		switch(type)
		{
			case LINE_START:
				if ((state==BEGIN || lookEnvelope) && IsFromLine(line))
				{
					if (state!=BEGIN)
					{
						if ((oldLine=NuHandle(len+1))==nil)
						{
							WarnUser(MEM_ERR,MemError());
							return(False);
						}
						BMD(line,*oldLine,len+1);
						goto done;
					}
					WriteZero(sum,sizeof(MSumType));
					sum->tableId = DEFAULT_TABLE;
					sum->persId = sum->popPersId = (*CurPersSafe)->persId;
					sum->spamScore = -1;
					/*if (!isOut)*/ GleanFrom(line,sum);
					sum->offset = TellLine(lip);
					sum->state = isOut ? UNSENDABLE : UNREAD;
					state = IN_HEADER;
					if (isOut) sum->flags = outFlags;
					if (PrefIsSet(PREF_SHOW_ALL)) sum->flags |= FLAG_SHOW_ALL;
					if (HasUnicode()) sum->flags |= FLAG_UTF8;
					maybeOut = -1;
					*toLine = 0;
				}
				else if (state==IN_HEADER)
				{ 											/* look for date, subj, from lines */
					if (*line=='\015')
					{
						state = IN_BODY;
						sum->bodyOffset = TellLine(lip)-sum->offset;
						if (!isOut && maybeOut>=ATTACH_HEAD+1)
						{
							CopyHeaderLine(duck,sizeof(duck),toLine);
							if(outFlags & FLAG_UTF8) HeaderToUTF8(duck);
							BeautifyFrom(duck);
							PSCopy(sum->from,duck);
							if(outFlags & FLAG_UTF8) TrimUTF8(sum->from);
							if (*sum->from) sum->state = UNSENT;
							sum->flags = outFlags;
						}
					}
					else if (!IsWhite(*line) || headerIndex==tchSubject)
					{
						if (!IsWhite(*line))	/* skip continuations */
						{
							/*
							 * grab header name
							 */
							for (spot=line;*spot!=':' && *spot;spot++);
							if (*spot==':'  && spot>line+1)
							{
								MakePStr(headerName,line,spot+1-line);
								headerIndex = FindSTRNIndex(TOCHeaderStrn,headerName);
							}
							else
							{
								*headerName = 0;
								headerIndex = 0;
							}
							
							/*
							 * is this an Out message in a non-out mailbox?
							 */
							if (!isOut && maybeOut)
							{
								if (maybeOut>0 && maybeOut<=ATTACH_HEAD)
									if (EqualStrRes(headerName,HEADER_STRN+maybeOut))
										maybeOut++;
									else
										maybeOut = 0;
							}
						}
						
						/*
						 * special handling for special headers
						 */
						switch (headerIndex)
						{
							case tchDate:
								CopyHeaderLine(duck,sizeof(duck),line);
								if (secs=BeautifyDate(duck,&origZone)) PtrTimeStamp(sum,secs,origZone);
								break;
								
							case tchTo:
								if (isOut)
								{
									CopyHeaderLine(duck,sizeof(duck),line);
									if(sum->flags & FLAG_UTF8) HeaderToUTF8(duck);
									BeautifyFrom(duck);
									PSCopy(sum->from,duck);
									if(sum->flags & FLAG_UTF8) TrimUTF8(sum->from);
									if (*sum->from) sum->state = SENDABLE;
								}
								else if (maybeOut)
								{
									maybeOut = TO_HEAD+1;
									BMD(line,toLine,sizeof(line));
								}
								break;
							
							case tchBcc:
								if (isOut || maybeOut>0)
								{
									if (isOut && (!*sum->from || *sum->from=='?'))
									{
										CopyHeaderLine(duck,sizeof(duck),line);
										if(sum->flags & FLAG_UTF8) HeaderToUTF8(duck);
										BeautifyFrom(duck);
										PSCopy(sum->from,duck);
										if(sum->flags & FLAG_UTF8) TrimUTF8(sum->from);
										if (*sum->from) sum->state = SENDABLE;
									}
									else if (!*toLine) BMD(line,toLine,sizeof(line));
								}
								break;
							
							case tchSubject:
								if (!IsWhite(*line))
								{
									CopyHeaderLine(duck,sizeof(duck),line);
									if(sum->flags & FLAG_UTF8) HeaderToUTF8(duck);
									BeautifySubj(duck,sizeof(sum->subj));
									PSCopy(sum->subj,duck);
									if(sum->flags & FLAG_UTF8) TrimUTF8(sum->subj);
								}
								else
								{
									CtoPCpy(duck,line);
									TrLo(duck+1,*duck,"\t"," ");
									TrimWhite(duck);
									if(sum->flags & FLAG_UTF8) HeaderToUTF8(duck);
									BeautifySubj(duck,sizeof(sum->subj));
									PSCat(sum->subj,duck);
									if(sum->flags & FLAG_UTF8) TrimUTF8(sum->subj);
								}
								break;
							
							case tchStatus:
								if (PrefIsSet(PREF_BELIEVE_STATUS) && PPtrFindSub(statusRead,line+*headerName,len-*headerName))
									sum->state=READ;
								break;
							
							case tchXPrior:
								if (lookPrior)
									sum->priority = sum->origPriority = Display2Prior(Atoi(line+*prior));
								break;
							
							case tchMessageId:
								if (!ValidHash(sum->msgIdHash))
								{
									CopyHeaderLine(duck,sizeof(duck),line);
									sum->msgIdHash = MIDHash(duck+1,*duck);
									if (!ValidHash(sum->uidHash)) sum->uidHash = sum->msgIdHash;
								}
								break;
							
							case tchPrecedence:
								if (!(sum->opts&OPT_BULK) && PPtrFindSub(GetRString(duck,BULK),line+*headerName,len-*headerName))
									sum->opts |= OPT_BULK;
								break;
								
							case tchImportance:
								if (lookPrior && sum->origPriority==0)
								{
									MakePStr(duck,spot+1,len-(spot+1-line));
									TrimWhite(duck);
									TrimInitialWhite(duck);
									sum->priority = FindSTRNIndex(ImportanceStrn,duck);
									sum->priority = sum->origPriority = Display2Prior(sum->priority);
								}
								break;
								
							default:
								if (!isOut	&&
										(headerIndex=FindSTRNIndex(SUM_SENDER_HEADS,headerName)))
								{
									if (!(sum->opts&OPT_BULK) && IsBulk(line)) sum->opts |= OPT_BULK;
									if (headerIndex<=senderHead)
									{
										CopyHeaderLine(duck,sizeof(duck),line);
										if(sum->flags & FLAG_UTF8) HeaderToUTF8(duck);
										BeautifyFrom(duck);
										PSCopy(sum->from,duck);
										if(sum->flags & FLAG_UTF8) TrimUTF8(sum->from);
										senderHead = headerIndex;
									}
								}
								else if (!(sum->opts&OPT_BULK) && FindSTRNSubIndex(BulkHeadersStrn,headerName)) sum->opts |= OPT_BULK;
								break;
						}
					}
				}
				else if (len>1 && *line=='<')
				{
#ifdef NOBODY_SPECIAL
					if (!afterSig) isWhite = false;
#endif // NOBODY_SPECIAL
					if (len>*html && !striscmp(line+1,html+1)) sum->opts |= OPT_HTML;
					else if (len>*xhtml && !striscmp(line+1,xhtml+1)) sum->opts |= OPT_HTML;
					else if (len>*flow && !striscmp(line+1,flow+1)) sum->opts |= OPT_FLOW;
					else if (len>*rich && !striscmp(line+1,rich+1)) sum->flags |= FLAG_RICH;
					else if ((len>*charset && !striscmp(line+1,charset+1))) sum->opts |= OPT_CHARSET;
					if((sum->opts & (OPT_HTML | OPT_FLOW)) || (sum->flags & FLAG_RICH)) sum->opts |= OPT_CHARSET;
				}
#ifdef NOBODY_SPECIAL
				else if (!afterSig && len==*sigIntro && !memcmp(sigIntro+1,line,*sigIntro))
					afterSig = true;
				else if (!afterSig && isWhite)
				{
					if (!IsAllWhitePtr(line,len-1)) isWhite = false;
				}
#endif // NOBODY_SPECIAL
				break;
			case LINE_MIDDLE:
				break;
			default:
				return(-36);					// I/O error
				break;
		}
		if (state==BEGIN) state=IN_HEADER;
	}
done:
	if (state!=BEGIN)
	{
		sum->length = TellLine(lip)-sum->offset;
		/*if (oldLine) sum->length -= GetHandleSize(oldLine)-1;*/
#ifdef NOBODY_SPECIAL
		if (isWhite) sum->opts |= OPT_JUSTSUB;
#endif // NOBODY_SPECIAL
	}
	return(state==BEGIN ? fnfErr : noErr);
}

/**********************************************************************
 * IsBulk - does this header line represent an automated mailer?
 **********************************************************************/
Boolean IsBulk(UPtr line)
{
	BinAddrHandle lineAddrs=nil;
	Str255 scratch;
	Handle daemonText=nil;
	BinAddrHandle daemonAddrs=nil;
	BinAddrHandle daemonRaw=nil;
	short dOffset;
	short lOffset;
	Boolean is = False;
	Boolean res = False;
	EAL_VARS_DECL;
	
	if (!SuckPtrAddresses(&lineAddrs,line,strlen(line),False,False,False,nil))
	{
		GetRString(scratch,DAEMON_NICKNAME);
		if (IsNickname(scratch,0))
		{
			if (!SuckPtrAddresses(&daemonRaw,scratch+1,*scratch,False,False,False,nil))
			{
				ExpandAliasesLow(&daemonAddrs,daemonRaw,0,False,nil,EAL_VARS);
			}
		}
		else if (daemonText = GetResource('TEXT',DAEMON_TEXT))
		{
			HNoPurge(daemonText);
			SuckAddresses(&daemonAddrs,daemonText,False,False,False,nil);
			res = True;
		}
		if (daemonAddrs)
		{
			LDRef(daemonAddrs);
			LDRef(lineAddrs);
			for (lOffset = 0; (*lineAddrs)[lOffset]; lOffset+=(*lineAddrs)[lOffset]+2)
				for (dOffset = 0; (*daemonAddrs)[dOffset]; dOffset += (*daemonAddrs)[dOffset]+2)
					if (PPtrFindSub((*daemonAddrs)+dOffset,
							(*lineAddrs)+lOffset+1,(*lineAddrs)[lOffset]))
					{
						is = True;
						goto done;
					}
		}
	}
done:
	ZapHandle(lineAddrs);
	if (res) HPurge(daemonText);
	else ZapHandle(daemonText);
	ZapHandle(daemonAddrs);
	ZapHandle(daemonRaw);
	return(is);
}

/**********************************************************************
 * BeautifySum - beautify a message summary before saving it.  This
 * involves beautifying the from address and the date.
 **********************************************************************/
void BeautifySum(MSumPtr sum)
{
	if (sum->seconds) PtrTimeStamp(sum,sum->seconds,ZoneSecs());
	BeautifyFrom(sum->from);
}

/**********************************************************************
 * BeautifySubj - Beautify a subject, including removing outlookisms
 **********************************************************************/
void BeautifySubj(PStr subject,short size)
{
	Str31 crap, treasure;
	Str255 crapStr, treasureStr;
	UPtr crapSpot = crapStr+1;
	UPtr treasureSpot = treasureStr+1;
	
	if (!PrefIsSet(PREF_NO_OUTLOOK_FIX))
	{
		GetRString(crapStr,OUTLOOK_CRAP_FIND);
		GetRString(treasureStr,OUTLOOK_CRAP_FIX);
		while (PToken(crapStr,crap,&crapSpot,","))
		{
			PToken(treasureStr,treasure,&treasureSpot,",");
			if (StartsWith(subject,crap))
			{
				if (*treasure > *crap)
				{
					// expanding; protect from overflow
					if (*subject + *crap - *treasure > size-2)
						*subject -= *treasure - *crap;
					BMD(subject+1,subject+1+(*treasure-*crap),*subject);
					*subject += *treasure - *crap;
				}
				else if (*crap > *treasure)
				{
					// shortening
					BMD(subject+1+*crap,subject+1+*crap-*treasure,*subject-*crap);
					*subject -= *crap - *treasure;
				}
				
				// move treasure into place
				BMD(treasure+1,subject+1,*treasure);
			}
		}
	}
}

		

/**********************************************************************
 * IsFromLine - determine whether or not a given line is a sendmail From
 * line.
 **********************************************************************/
Boolean IsFromLine(UPtr line)
{
	int num,len;
	int quote=0;
	Str255 scratch;
	UPtr cp;
	short weekDay,year,tym,day,month,other,remote,from;

#define isdig(c) ('0'<=(c) && (c)<='9') 
	if (line[0]!='F' || line[1]!='r' || line[2]!='o' || line[3]!='m')
		return (False);  /* quick and dirty */
	
	/*
	 * it passed the first test.	Now, we have to be more rigorous.
	 * this is a sample sendmail From line:
	 * >From paul@uxc.cso.uiuc.edu Wed Jun 14 12:36:18 1989<
	 * However, various systems do various bad things with the From line.
	 * therefore, I've changed it to look for:
	 */
	
	/* check for the space after the from */
	line += 4;
	if (*line++!=' ') return (False);
	
	/* skip the return address */
	while (*line && (quote || *line!=' '))
	{
		if (*line=='"') quote = !quote;
		line++;
	}
	if (!*line++) return (False);
	while (*line==' ') line++;
	
	remote=from=weekDay=day=from=year=tym=month=other=0;
	len = strlen(line);
	if (len>sizeof(scratch)-1) return(False);
	strcpy(scratch,line);
	for (cp=strtok(scratch," \t\015,");cp;cp=strtok(nil," \t\015,"))
	{
		len = strlen(cp);
		num = Atoi(cp);
		if (num<24 && (len>=5 && cp[2]==':') && (len==5 || len==8 && cp[5]==':'))
		{
			if (tym++) return(False);
		}
		else if (!year && day && len==2 && isdig(cp[len-1]))
		{
			if (year++) return(False);
		}
		else if (len<=2 && num && num<32)
		{
			if (day++) return(False);
		}
		else if (len==4&&num>1900)
		{
			if (year++) return(False);
		}
		else if (len==6 && !striscmp(cp,"remote"))
		{
			if (remote++ || from) return(False);
		}
		else if (len==4 && !striscmp(cp,"from"))
		{
			if (!remote || from++) return(False);
		}
		else if (len==3 &&
			!(striscmp(cp,"mon") && striscmp(cp,"tue") && striscmp(cp,"wed")
			 && striscmp(cp,"thu") && striscmp(cp,"fri")
			 && striscmp(cp,"sat") && striscmp(cp,"sun")))
		{
			if (weekDay++) return(False);
		}
		else if (len==3 && !(striscmp(cp,"jan") && striscmp(cp,"feb") &&
			striscmp(cp,"mar") && striscmp(cp,"apr") &&
			striscmp(cp,"may") && striscmp(cp,"jun") &&
			striscmp(cp,"jul") && striscmp(cp,"aug") &&
			striscmp(cp,"sep") && striscmp(cp,"oct") &&
			striscmp(cp,"nov") && striscmp(cp,"dec")))
		{
			if (month++) return(False);
		}
		else
		{
			other++;
		}
	}
	return (day && year && month && tym && other<=2);
}
/**********************************************************************
 * GleanFrom - grab relevant info from sendmail From line
 * input line is in C format, fields in summary are in Pascal form
 **********************************************************************/
void GleanFrom(UPtr line,MSumPtr sum)
{
	Str255 copy;
	register char *cp=copy;
	register char *ep;
	long seconds;
	long offset = ZoneSecs();
	Str31 dateStr;
	
	strcpy(copy,line);
	/*
	 * from address
	 */
	while (*cp++ != ' ');
	for (ep=cp; *ep!= ' '; ep++);
	*ep = '\0';
	MakePStr(sum->from,cp,ep-cp);
	
	/*
	 * date
	 */
	for (cp=++ep;*ep!='\015' && *ep; ep++);
	*ep = '\0';
	MakePStr(dateStr,cp,ep-cp);
	
	/*
	 * TimeStamp
	 */
	seconds = UnixDate2Secs(dateStr)-offset;
	PtrTimeStamp(sum,seconds,offset);
	sum->arrivalSeconds = seconds+offset;
}


/************************************************************************
 * UnixDate2Secs - convert a UNIX date into seconds
 ************************************************************************/
uLong UnixDate2Secs(PStr date)
{
	Str63 copy;
	UPtr end;
	DateTimeRec dtr;
	uLong secs = 0;
	
	PCopy(copy,date);
	end = copy+*copy;
	/* let's null-terminate this, shall we? */
	end[1] = 0;
	
	/* back up and grab the year */
	while (*end!=' ' && end>copy) end--;
	dtr.year = Atoi(end);
	if (dtr.year>0 && dtr.year<1900) dtr.year+=1900;
	
	/* seconds */
	*end = 0;
	while (*end!=':'&&end>copy) end--;
	dtr.second = Atoi(end+1);
	
	/* minutes */
	*end = 0;
	while (*end!=':'&&end>copy) end--;
	dtr.minute = Atoi(end+1);
	
	/* hours */
	*end = 0;
	while (*end!=' '&&end>copy) end--;
	dtr.hour = Atoi(end+1);
	
	/* date */
	*end = 0;
	while (*end!=' '&&end>copy) end--;
	dtr.day = Atoi(end+1);

	/* Month */
	*end = 0;
	while (*--end==' ' && end>=copy);
	while (*end!=' '&&end>copy) end--;
	dtr.month = MonthNum(end+1);
	
	if (dtr.year && dtr.month) DateToSeconds(&dtr,&secs);
	return(secs);
}

/************************************************************************
 * MonthNum - get the month number from a month name
 ************************************************************************/
short MonthNum(CStr cp)
{
	char monthStr[4];
	short month=0;
	
	/*
	 * copy and lowercase
	 */
	BMD(cp,monthStr,3);
	monthStr[3] = 0;
	MyLowerText(monthStr,3);
	cp = monthStr;
	
	switch(cp[0])
	{
		case 'j': month = cp[1]=='a' ? 1 : (cp[2]=='n' ? 6 : 7); break;
		case 'f': month = 2; break;
		case 'm': month = cp[2]=='r' ? 3 : 5; break;
		case 'a': month = cp[1]=='p' ? 4 : 8; break;
		case 's': month = 9; break;
		case 'o': month = 10; break;
		case 'n': month = 11; break;
		case 'd': month = 12; break;
	}
	return(month);
}

/************************************************************************
 * BeautifyDate - make a date look better
 * assumes the date is in smtp date format
 ************************************************************************/
uLong BeautifyDate(UPtr dateStr,long *origZone)
{
	UPtr spot;
	Str255 token;
	DateTimeRec dtr;
	Str31 shortTok;
	long numericTok;
	short index;
	uLong secs;
	Boolean inComment = false;
	
	*origZone = -1;
	Zero(dtr);
	
	spot = dateStr+1;
	while (PToken(dateStr,token,&spot," \t,\r"))
	{
		// ignore empty tokens
		if (!*token) continue;
		
		// comments in dates?  it can happen...
		if (token[1]=='(') inComment = true;
		if (token[*token]==')') {inComment = false; continue;}
		if (inComment) continue;
		
		// precompute some values
		token[*token+1] = 0;
		PSCopy(shortTok,token);
		numericTok = -1;
		if (((token[1]=='-'||token[1]=='+') && AllDigits(token+2,*token-1))
				|| AllDigits(token+1,*token))
			StringToNum(token,&numericTok);
		else numericTok = -1;
		index = 0;
		
		// what do we have?
		if (FindSTRNIndex(WEEKDAY_STRN,shortTok)) continue;	// ignore weekdays
		// month?
		else if (index = FindSTRNIndex(MONTH_STRN,shortTok))
		{
			if (dtr.month) goto bail;	// already have a month
			dtr.month = index;
		}
		// named timezone?
		else if (StringSame(token,"\pUT")||StringSame(token,"\pGMT")||(index=TZName2Offset(token+1)))
		{
			if (*origZone != -1) goto bail;
			*origZone = index;
		}
		else if (numericTok!=-1)
		{
			
			if (numericTok<0 ||	// negative numbers must be timezones
					token[0]==5 && (token[1]=='-'||token[1]=='+') ||  // explicit signs must be timezones
					token[0]==4 && dtr.year > 0)	// four digits, no sign, but we have a year already; probably timezone
			{
				if (*origZone!=-1) goto bail;	// already have a zone!
				*origZone = CStr2Zone(token+1);
			}
			else if (numericTok<2050 && (numericTok>1904 || numericTok>31 || numericTok>=0 && dtr.day>0))
			{
				if (dtr.year!=0) goto bail;	// thought we had a year!
				if (numericTok > 1904)
					dtr.year = numericTok;
				else if (numericTok > 38)
					dtr.year = numericTok+1900;
				else
					dtr.year = numericTok+2000;
			}
			else if (numericTok<=31 && numericTok>0)
			{
				if (dtr.day!=0) goto bail;	// already had a day
				dtr.day = numericTok;
			}
			else goto bail;	// huh?  Some number we don't understand
		}
		else if (*token<31 && strchr(token+1,':'))
		{
			// might have a time
			UPtr spot2;
			Str31 token2;
			
			spot2 = token+1;
			if (PToken(token,token2,&spot2,":"))
			{
				if (*token2>2 || !AllDigits(token2+1,*token2)) goto bail;	// numbers only, please!
				token2[*token2+1] = 0;
				dtr.hour = atoi(token2+1);
				if (PToken(token,token2,&spot2,":"))
				{
					if (*token2!=2 || !AllDigits(token2+1,*token2)) goto bail;	// numbers only, please!
					token2[*token2+1] = 0;
					dtr.minute = atoi(token2+1);
					if (PToken(token,token2,&spot2,":"))
					{
						if (*token2!=2 || !AllDigits(token2+1,*token2)) goto bail;	// numbers only, please!
						token2[*token2+1] = 0;
						dtr.second = atoi(token2+1);
					}
				}
			}
		}
	}
	
	// ok, we're out.  Did we get what we needed?
	if (dtr.day && dtr.month && dtr.year)
	{
		if (*origZone==-1) *origZone = ZoneSecs();	// fill in our zone if need be
		DateToSeconds(&dtr,&secs);
		secs -= *origZone;
		return(secs);
	}
	
bail:
	// couldn't get it
	*origZone = ZoneSecs();
	secs = GMTDateTime();
	return(secs);
}

#ifdef NEVER
/************************************************************************
 * BeautifyDate - make a date look better
 * assumes the date is in smtp date format
 ************************************************************************/
uLong OLDBeautifyDate(UPtr dateStr,long *origZone)
{
	UPtr cp, cp2;
	DateTimeRec dtr;
	uLong secs;
	long offset;
	*origZone = 0;
	
	if (*dateStr < 10) return(0);  /* ignore really short dates */
	dateStr[*dateStr+1] = 0;
	WriteZero(&dtr,sizeof(dtr));
	
	/*
	 * delete day of the week
	 */
	if (cp=strchr(dateStr+1,','))
	{
		for (cp++;IsSpace(*cp);cp++);
		strcpy(dateStr+1,cp);
		*dateStr = strlen(dateStr+1);
	}
	
	/*
	 * make sure that single-digit dates begin with a space, not a '0' and
	 * not nothing
	 */
	dtr.day = Atoi(dateStr+1);
	if (dateStr[1]=='0')
		dateStr[1] = ' ';
	else if (dateStr[2]==' ')
	{
		Str255 temp;
		*temp = 1;
		PCat(temp,dateStr);
		PCopy(dateStr,temp);
		dateStr[1] = ' ';
	}
		
	/*
	 * delete year, if present
	 */
	/* pointing at day */
	for (cp=dateStr+2;*cp && !IsSpace(*cp);cp++);
	for (;IsSpace(*cp);cp++);
	/* pointing at month */
	dtr.month = MonthNum(cp);
	for (;*cp && !IsSpace(*cp);cp++);
	for (cp2=cp;IsSpace(*cp2);cp2++);
	
	if (*cp && isdigit(*cp2))
	{
		dtr.year = Atoi(cp2);
		if (dtr.year<20) dtr.year += 2000;
		else if (dtr.year<1900) dtr.year += 1900;
		while (isdigit(*cp2)) cp2++;
		strcpy(cp,cp2);
		*dateStr -= cp2-cp;
		dateStr[*dateStr+1] = 0;
	}
	
	/*
	 * delete the seconds, if present
	 */
	dtr.hour = Atoi(cp);
	if (cp=strchr(cp,':'))
	{
		dtr.minute = Atoi(cp+1);
		if (cp[3]==':')
		{
			dtr.second = Atoi(cp+4);
			strcpy(cp+3,cp+6);
			*dateStr -= 3;
			dateStr[*dateStr+1] = 0;
		}
	}
	
	if (dtr.year)
	{
		DateToSeconds(&dtr,&secs);
		cp = strchr(dateStr+1,':');
		cp2 = dateStr+*dateStr+1;
		if (cp&&cp+4<cp2)
		{
			cp+=4;
			offset = CStr2Zone(cp);
		}
		else
			offset = ZoneSecs();
		secs -= offset;
		*origZone = offset;
	}
	else secs = 0;
	return(secs);
}
#endif

/**********************************************************************
 * CStr2Zone - turn a C string into a zone
 **********************************************************************/
long CStr2Zone(char *s)
{
	long offset;

	if (offset = Atoi(s))
	{
		if (offset>2400 || offset<-2400) offset = ZoneSecs();
		else
		{
			if (*s == '-') offset *= -1;
			offset = 60*((offset/100)*60 + offset%100);
			if (*s == '-') offset *= -1;
		}
	}
	else	// zero timezone; is it really zero, or not?
	{
		while (*s && IsWhite(*s)) s++;
		if ((*s=='-' || *s=='+') && s[1]=='0') return(offset);	// yes, it *was* zero
		offset = TZName2Offset(s);
	}
	return(offset);
}
	
/************************************************************************
 * BeautifyFrom - make a from line look better
 ************************************************************************/
void BeautifyFrom(UPtr fromStr)
{
	UPtr cp1,cp2;
	int len;
	Boolean wasNotEmpty;
	
	fromStr[*fromStr+1] = 0;
	
	wasNotEmpty = *fromStr>0;
	
	/*
	 * elide everything after the last <, unless it's the first character
	 */
	TrimAllWhite(fromStr);
	if (fromStr[1]!='<' && (cp1=strrchr(fromStr+1,'<')))
	{
		*cp1 = 0;
		*fromStr = cp1-fromStr-1;
	}
	/*
	 * no phrase; prefer parenthesized text
	 */
	else if (fromStr[1]!='"' && (cp1=strchr(fromStr+1,'(')))
		if (cp2=strchr(cp1+1,')'))
		{
			len = cp2 - cp1 - 1;
			if (len>0 && (len!=3 || !AllDigits(cp1+1,len)))
			{
				*fromStr = len;
				strncpy(fromStr+1,cp1+1,len);
				fromStr[*fromStr+1] = 0;
			}
		}
	TrimAllWhite(fromStr);
	TrimWrap(fromStr,'(',')');
	TrimWrap(fromStr,'"','"');
//	TrimNonWord(fromStr);
	if (PrefIsSet(PREF_UNCOMMA)) Uncomma(fromStr);
	if (!*fromStr && wasNotEmpty) GetRString(fromStr,SOME_BOZO);
}

PStr TrimWrap(UPtr str,int openC,int closeC)
{
  if (*str>1 && str[1] == openC && str[*str] == closeC)
	{
	  BMD(str+2,str+1,*str-2);
		*str -= 2;
		str[*str+1] = 0;
	}
	return(str);
}

PStr TrimNonWord(PStr str)
{
	UPtr spot;
  while (*str && !IsWordOrDigit(str[*str])) --*str;
  for (spot=str+1;spot<str+*str+1 && !IsWordOrDigit(*spot);spot++);
  *str -= (spot-str)-1;
  BMD(spot,str+1,*str);
  return(str);
}

	
/************************************************************************
 * SumToFrom - build a sendmail-style from line from a message summary
 ************************************************************************/
int SumToFrom(MSumPtr sum, UPtr fromLine)
{
	char *start, *end;
	Str31 delims;
	Str63 fromWhom;
	short n;
	
	GetRString(delims,DELIMITERS);
	if (sum && Tokenize(sum->from+1,*sum->from,&start,&end,delims))
	{
		strncpy(fromWhom+1,start,end-start);
		*fromWhom = end-start;
	}
	else
		GetRString(fromWhom,UNKNOWN_SENDER);

	PCopy(fromLine,"\pFrom ");
	PCat(fromLine,fromWhom);
	n = fromLine[0]+1;
	LocalDateTimeStr(fromLine+n);
	fromLine[0] += fromLine[n]+1;
	fromLine[n] = ' ';
	p2cstr(fromLine);
	return(strlen(fromLine));
}

/**********************************************************************
 * CopyHeaderLine - copy the contents of a header line (C format)
 * into a Pascal string.
 **********************************************************************/
void CopyHeaderLine(UPtr to,int size,UPtr from)
{
	while (*++from != ':');
	while (*++from == ' ');
	strncpy(to,from,size-2);
	to[size-2] = 0;
	c2pstr(to);
	TrimWhite(to);
}

/**********************************************************************
 * HeaderToUTF8 - take a Pascal string with an 822 header and convert
 * all of the RFC2047 encoded words to UTF8.
 **********************************************************************/
OSStatus HeaderToUTF8(PStr head)
{
	Accumulator a;
	static Handle h = nil;
	OSStatus err;
	
	if(!HasUnicode()) return paramErr;
	
	// This gets called alot, so keep a handle around to do this
	if(!h)
		h = NuHTempBetter(1 K);
	else if(!*h)
		ReallocateHandle(h, 1 K);
	else
		HNoPurge(h);
	err = MemError();
	if(err) return err;
	AccuInitWithHandle(&a, h);
	a.offset = 0;
	
	/*
	 *	InsertIntlHeaders can take a handle or a pointer (here it's a pointer) and returns
	 *	the un-2047'ed text in the Accumulator
	 */
	err = InsertIntlHeaders((Handle)(head + 1), *head, -1, &a, kTextEncodingUnknown, nil, nil);
	if(!err)
	{
		AccuToStr(&a, head);
		// Get rid of trailing crud in case it truncated to 255 in the middle of a UTF-8 sequence
		TrimUTF8(head);
	}
	HPurge(h);
	return err;
}

/************************************************************************
 * FindTOCSpot - find a spot in a file that's big enough to hold a
 * message.
 ************************************************************************/
long FindTOCSpot(TOCHandle tocH, long length)
{
#pragma unused(length)
	long end=0;
	MSumPtr sum;
	MSumPtr limit;
	
	/*
	 * for now, just return the end
	 */
	sum = (*tocH)->sums;
	limit = sum + (*tocH)->count;
	for (; sum<limit ; sum++)
		if (sum->offset > -1 && end < sum->offset+sum->length)
			end = sum->offset + sum->length;
	
	return(end);
}

/************************************************************************
 * ComputeLocalDateLo - put the time stamp in local time
 ************************************************************************/
PStr ComputeLocalDateLo(uLong secs, long origZone,PStr dateStr)
{
	Str31 zone;
	Str255 product;
	Boolean local = PrefIsSet(PREF_LOCAL_DATE);
	long timeZone;
	
	if (local)
	{
		timeZone = ZoneSecs();
		*zone = 0;
	}
	else
	{
		timeZone = 60*origZone;
		FormatZone(zone,60*origZone);
	}
	SecsToLocalDateTime(secs,timeZone,zone,product);
	if (*product>31)
		*product = 31;	// date was too long
	PCopy(dateStr,product);
	return dateStr;
}

/************************************************************************
 * SecsToLocalDateTime - turn a value of seconds into a local date time
 ************************************************************************/
void SecsToLocalDateTime(long secs, long timeZone, Str31 zone, Str255 product)
{
	Str31 time;
	Str31 date;
	Str31 weekday;
	Str31 fmt;
	long now = GMTDateTime();
	long age = now-secs;

	TimeString(secs+timeZone,False,time,nil);
	DateString(secs+timeZone,shortDate,date,nil);
	WeekDay(weekday,secs+timeZone);
	
	if (age<0 || PrefIsSet(PREF_FIXED_DATES))
		utl_PlugParams(GetRString(fmt,FIXED_DATE_FMT),product,time,date,weekday,zone);
	else
	{
		long oldDate = 3600*GetRLong(OLD_DATE);
		Boolean old = age<0 || (age > oldDate && (oldDate>=0 || age>Yesterday));
		Boolean ancient = age<0 || old && age > 3600*GetRLong(ANCIENT_DATE);
		
		utl_PlugParams(GetRString(fmt,old?
			(ancient?ANCIENT_LOCAL_DATE_FMT:OLD_LOCAL_DATE_FMT)
			 :LOCAL_DATE_FMT),
		  product,time,date,weekday,zone);
	}
}

/************************************************************************
 * RemoveUTF8FromSum - convert utf8 in summary to mac roman, if that can be done safely
 ************************************************************************/
Boolean RemoveUTF8FromSum(MSumPtr sum)
{
	Str63 subject, who;
	ByteCount sLen, wLen;
	
	// if the summary doesn't have UTF8 in it, no work to do
	if (!(sum->flags&FLAG_UTF8)) return false;
	
	// copy the strings into local storage
	PSCopy(subject,sum->subj);
	PSCopy(who,sum->from);
	sLen = *subject;
	wLen = *who;
	
	// See if they both convert cleanly
	if (UTF8ToRoman(subject+1,&sLen,sizeof(subject)-1))
	if (UTF8ToRoman(who+1,&wLen,sizeof(who)-1))
	{
		// they did!  write them back as roman, and clear the flag
		*subject = sLen;
		*who = wLen;
		PSCopy(sum->subj,subject);
		PSCopy(sum->from,who);
		sum->flags &= ~FLAG_UTF8;
		return true;
	}
	
	return false;
}
