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

#include "anal.h"
#define FILE_NUM 135

long AnalCount;
struct TAEDictState TAEDictionary;
Handle TAEDictHandle;
long LastAnalUse;

void AnalScanPeteInner(PETEHandle pte, struct TAESessionState *pSession, long *pScanned,Boolean toSpeak);
OSErr AnalInit(void);
OSErr AnalInitDictionary(struct TAEDictState *pDictionary);
OSErr AnalDispose(void);
OSErr AnalStart(void);
void AnalEnd(void);
void AnalIdle(void);
short AnalScore(struct TAESessionState *pSession);
#define kAnalScanFinished (-1)
#define AnalScanFinished(pte) (!(*PeteExtra(pte))->taeDesired || (*PeteExtra(pte))->taeScanned==kAnalScanFinished)
#define IsAnalHot(collection) ((collection)%2==0)
void SetupTAEDictionary(void);
void TeardownTAEDictionary(void);
char **AnalWhite(void);

/**********************************************************************
 * AnalScan - the big analysis scanner, for all Pete records
 **********************************************************************/
void AnalScan(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	PETEHandle pte;
			
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
	{
		if (IsKnownWindowMyWindow(winWP))
		{
			win = GetWindowMyWindowPtr (winWP);
			for (pte=win->pteList;pte;pte=PeteNext(pte))
			{
				PeteExtra extra = **PeteExtra(pte);
				while (!AnalScanFinished(pte))
				{
					if (AnalStart()) return;
					UsingWindow(winWP);
					AnalScanPete(pte,false,(*PeteExtra(pte))->taeSpeak);
					AnalEnd();
					if (NEED_YIELD) return;
				}
			}
		}
	}
	
	AnalIdle();
}

/**********************************************************************
 * AnalScanPete - the analysis scanner, for a single Pete record
 **********************************************************************/
void AnalScanPete(PETEHandle pte,Boolean toCompletion,Boolean toSpeak)
{
	long scanned = (*PeteExtra(pte))->taeScanned;
	long len;
	
	// Are we done?
	if (!AnalScanFinished(pte))
	{
		// Make a copy of the session
		struct TAESessionState session = (*PeteExtra(pte))->taeSession;

		// do we need to destroy the session?
		if (scanned != (*PeteExtra(pte))->taeScannedSession)
		{
			if (session.pvtext) TAEEndSession(&session);
			Zero(session);
		}

		// do we need a session?
		if (!session.pvtext)
		{
			SetupTAEDictionary();
			TAEStartSession(&session,&TAEDictionary);
			TeardownTAEDictionary();
		}
		
		len = PETEGetTextLen(PETE,pte);
		
		// Scan at least once
		if (session.pvtext) 
			do
			{
				AnalScanPeteInner(pte,&session,&scanned,toSpeak);
			} // Keep scanning if we want to finish
			while (toCompletion && scanned<len-1);
		
		// Update the scanned pointers
		(*PeteExtra(pte))->taeScanned = (*PeteExtra(pte))->taeScannedSession = scanned<len ? scanned:kAnalScanFinished;

		// If we're done, record the score and kill the session
		if (AnalScanFinished(pte))
		{
			short score = AnalScore(&session);
			
			(*PeteExtra(pte))->taeScore = score+1;
			TAEEndSession(&session);
			Zero(session);
		}
		
		// Put the session back
		(*PeteExtra(pte))->taeSession = session;
	}
}

/**********************************************************************
 * AnalScanPeteInner - do a single scan, assuming we're all setup
 **********************************************************************/
void AnalScanPeteInner(PETEHandle pte, struct TAESessionState *pSession, long *pScanned, Boolean toSpeak)
{
	long l1, l2;
	Handle text;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	Ptr spot, end;
	long len;
	Str15 quote;
	Boolean is=false;
	PETEParaInfo info;
	Boolean scanQuote = AnalScanQuotes() || toSpeak;
	struct TAEAllMatches phrases;
	struct TAEMatch *match, *lastMatch;
		
	len = PETEGetTextLen(PETE,pte);

	PeteGetTextAndSelection(pte,&text,nil,nil);
	spot = *text + *pScanned;
	end = *text+len;
		
	// find the first return
	if (!*pScanned)
		l1 = 0;
	else
	{
		for (;spot<end;spot++) if (*spot=='\015') break;
		l1 = spot-*text+1;
	}

	// find the next return
	for (spot++;spot<end;spot++) if (*spot=='\015') break;
	l2 = spot-*text;
	
	// Got a line?
	if (l1<=l2 && l1<len) *pScanned = l2;
	else {*pScanned = len; return;}
	
	// Is it in an attachment header?
	if (GetWindowKind(GetMyWindowWindowPtr((*PeteExtra(pte))->win))==COMP_WIN)
	{
		long para;
		PETEGetParaIndex(PETE,pte,(l1+l2)/2,&para);
		if (para==ATTACH_HEAD-1) return;
	}
	
#ifdef DEBUG
	spot = *text+l1;
#endif
	
	// excerpted?
	Zero(info);
	if (scanQuote && !PETEGetParaInfo(PETE,pte,PeteParaAt(pte,l1),&info))
		if (info.quoteLevel) is = true;
	
	// Great.  Grab the quote chars
	if (!scanQuote || !is || *GetRString(quote,QUOTE_PREFIX))
	{
		if (scanQuote && !is) is = quote[1]==(*text)[l1];
		
		if (!scanQuote || !is)
		{
			NoScannerResets++;	// these changes don't count
						
			TAEInitAllMatches(&phrases);

			LDRef(text);
			SetupTAEDictionary();
			TAEProcessText(pSession,*text+l1,l2-l1,&phrases,true,AnalWhite());
			TeardownTAEDictionary();
			UL(text);

			LastAnalUse = TickCount();
			
			// Mark the phrases
			if (!phrases.iNumMatches) {if (!toSpeak) PeteNoLabel(pte,l1,l2,pAnalMask);}
			else
			{
				match = phrases.ptaematches;
				lastMatch = match + phrases.iNumMatches-1;
				if (!toSpeak)
				{
					PeteNoLabel(pte,l1,l1+match->lStart,pAnalMask);
					PeteNoLabel(pte,l1+lastMatch->lStart+lastMatch->lLength,l2,pAnalMask);
				}
				while (match<=lastMatch)
				{
					if (toSpeak)
					{
						Str255 s;
						MakePStr(s,*text+l1+match->lStart,match->lLength);
						Speak(nil,s+1,*s);
					}
					else if (!PeteIsLabelled(pte,l1+match->lStart,l1+match->lStart+match->lLength,pURLLabel) && !PeteIsLabelled(pte,l1+match->lStart,l1+match->lStart+match->lLength,pLinkLabel))
					{
						PeteLabel(pte,l1+match->lStart,l1+match->lStart+match->lLength,IsAnalHot(match->nCollection)?pLooseAnalLabel:pTightAnalLabel,pAnalMask);
						if (match<lastMatch)
							PeteNoLabel(pte,l1+match->lStart+match->lLength,l1+match[1].lStart,pAnalMask);
					}
					match++;
				}
			}
			
			TAEFreeAllMatches(&phrases);
		
			NoScannerResets--;
			
			if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
			(*PeteExtra(pte))->win->isDirty = winDirtyWas;
		}
	}
	
	// update where we left off
	*pScanned = l2;
}

/**********************************************************************
 * AnalScanHandle - scan a handle with the analyzer
 **********************************************************************/
short AnalScanHandle(UHandle text,long offset,long len,Boolean *inHeader)
{
	struct TAESessionState session;
	short score = -1;	// invalid...

	// initialization
	if (AnalStart())
		return -1;	//	Unable to initialize. Return invalid score.
	
	// get a session going
	SetupTAEDictionary();
	TAEStartSession(&session,&TAEDictionary);
	TeardownTAEDictionary();
	
	if (session.pvtext) 
	{
		UPtr start, stop, end;
		long len = GetHandleSize(text);
		long offset;
		long maxBite = GetRLong(MAX_ANAL_BITE);
		
		for (offset=0;offset<len;offset+=stop-start)
		{
			start = *text+offset;
			end = *text+len;

			// Scan headers?
			if (inHeader && *inHeader)
			{
				Str31 rcvd;
				Str31 foundHead;
				
				// find a header
				for (stop=start+1;stop<end;stop++)
					if (*stop=='\015' && (stop==end-1 || !IsWhite(stop[1]))) {stop++;break;}
				if (stop>=end-1 || *stop=='\r') *inHeader = false;
				
				// is it a Received header?
				GetRString(rcvd,RECEIVED_HEAD);
				MakePStr(foundHead,start,stop-start);
				if (*foundHead>=*rcvd)
				{
					*foundHead = *rcvd;
					if (StringSame(foundHead,rcvd)) continue;	// skip Received: headers
				}
			}
			else
			{
				// find a blank line, or else just a return if the paragraph is very long
				for (stop=start+1;stop<end;stop++)
				{
					if (*stop=='\015' && stop-start>maxBite) {stop++;break;}
					if (*stop=='\015' && (stop==end-1 || stop[1]=='\r')) {stop++;break;}
				}
			}

			// analyze it...
			LDRef(text);
			SetupTAEDictionary();
			TAEProcessText(&session,start,stop-start,nil,true,AnalWhite());
			TeardownTAEDictionary();
			UL(text);
			LastAnalUse = TickCount();
			
			if (NEED_YIELD) MyYieldToAnyThread();
		}
		
		// ok, now score it
		score = AnalScore(&session);
		
		// kill the session
		TAEEndSession(&session);
	}
	
	AnalEnd();
	return score;
}

/**********************************************************************
 * AnalInit - prepare the analyzer for entry
 **********************************************************************/
OSErr AnalInit(void)
{
	if (AnalCount) return noErr;
	if (TAEStartSession)	// found the library
	if (HasFeature(featureAnal))	// user allowed to use it
	if (!AnalDisabled())	// user wants to use it
	if (!AnalInitDictionary(&TAEDictionary))	// it lives!
	{
		TAEDictHandle = RecoverHandle(TAEDictionary.pvdict);
		TeardownTAEDictionary();
		UseFeature(featureAnal);
		AnalCount++;	// successfully initialized!
		return noErr;
	}
	return fnfErr;
}

/**********************************************************************
 * AnalDispose - all done with the scanner
 **********************************************************************/
OSErr AnalDispose(void)
{
	if (TAEDictHandle)
	{
		SetupTAEDictionary();
		TAECloseDictionary(&TAEDictionary);
		Zero(TAEDictionary);
		TAEDictHandle = nil;
	}
	AnalCount = 0;
	return noErr;
}

/**********************************************************************
 * AnalStart - a particular process will start using the analyzer
 **********************************************************************/
OSErr AnalStart(void)
{
	OSErr err=noErr;
	
	if (!AnalCount)
		err=AnalInit();

	if (!err) AnalCount++;
	
	return err;
}

/**********************************************************************
 * AnalEnd - a particular process is done using the analyzer
 **********************************************************************/
void AnalEnd(void)
{
	if (AnalCount) AnalCount--;
}

/**********************************************************************
 * AnalIdle - an idle anal is the devil's plaything
 **********************************************************************/
void AnalIdle(void)
{
	if (AnalCount>1) return;	// still active
	if (!AnalCount) return;	// dead already
	
	// flush the anal stuff if we haven't used it in a long time
	if (DiskSpunUp() && LastAnalUse-TickCount()>GetRLong(MAX_ANAL_IDLE))
		AnalDispose();
}

/**********************************************************************
 * AnalInitDictionary - initialize the dictionary
 **********************************************************************/
OSErr AnalInitDictionary(struct TAEDictState *pDictionary)
{
	FSSpec spec;
	OSErr err = FindMyFile(&spec,kStuffFolderBit,FLAME_DICTIONARY);
	
	if (err) return err;
	
	if (TAEInitDictionary(pDictionary, &spec))
		return noErr;
	else
	{
		NoAnalDictionary = true;
		err = fnfErr;
	}

	return err;
}

/**********************************************************************
 * SetupTAEDictionary - get the dictionary pointer right
 **********************************************************************/
void SetupTAEDictionary(void)
{
	TAEDictionary.pvdict = LDRef(TAEDictHandle);
}

/**********************************************************************
 * TeardownTAEDictionary - let the dictionary pointer float
 **********************************************************************/
void TeardownTAEDictionary(void)
{
	UL(TAEDictHandle);
	// dictionary is now unsafe, but TAE likes to know it's there,
	// so we won't make it nil anymore
	// TAEDictionary.pvdict = nil;
}

/**********************************************************************
 * AnalWarning - warn the user if queueing an offensive message
 **********************************************************************/
Boolean AnalWarning(MessHandle messH)
{	
	if (AnalWarnOutgoing())
	{
		AnalStart();
		AnalScanPete(TheBody,true,AnalSpeakPhrases());
		AnalEnd();
		SetTAEScore((*messH)->tocH,(*messH)->sumNum,(*PeteExtra(TheBody))->taeScore);
		if (SumOf(messH)->score > GetRLong(ANAL_WARNING_LEVEL))
		{
			Str63 who, subj;
			PSCopy(who,SumOf(messH)->from);
			PSCopy(subj,SumOf(messH)->subj);
			if (kAlertStdAlertCancelButton==ComposeStdAlert(Caution,ANAL_WARNING,who,subj,OffensivenessStrn+SumOf(messH)->score))
				return true;
		}
	}	
	return false;
}

/**********************************************************************
 * AnalFindMine - is the dictionary there?
 **********************************************************************/
OSErr AnalFindMine(void)
{
	FSSpec spec;
	OSErr err = FindMyFile(&spec,kStuffFolderBit,FLAME_DICTIONARY);
	
	NoAnalDictionary = err!=noErr;
	return err;
}

/**********************************************************************
 * AnalScore - compute the score
 **********************************************************************/
short AnalScore(struct TAESessionState *pSession)
{
	short (**customScoring)[7] = nil;
	short n = 0;
	short score;
	
	if (customScoring = (void *)GetResource('EuSc',128))
		n = GetHandleSize_(customScoring)/(7*sizeof(short));
	
	SetupTAEDictionary();
	score = TAEGetScoreData(pSession,nil,customScoring ? LDRef(customScoring):nil,n);
	TeardownTAEDictionary();

	if (customScoring) UL(customScoring);

	return score;
}

/**********************************************************************
 * AnalWhite - build a whitelist
 **********************************************************************/
char ** AnalWhite(void)
{
	static char *whitelist[MAX_ANAL_WHITE];
	static Str255 whitelistString;
	char *comma, *end;
	char **spot = whitelist;
	
	GetRString(whitelistString,ANAL_WHITE);
	
	// pretend the first character of the string is the comma after a previous item
	comma = whitelistString;
	end = whitelistString+*whitelistString;
	PTerminate(whitelistString);
	
	// process while we still have slots and string
	while (spot<whitelist+MAX_ANAL_WHITE-1 && comma<end)
	{
		//next string starts right after the prior comma
		//we stick it into our list here; if we don't actually have a word,
		//we'll overwrite it later with NULL
		*spot = comma+1;
		
		// scan to end of string or comma
		for (comma++;comma<end && *comma!=',';comma++);
		
		// turn the comma into a NULL to terminate current word
		// harmless if we're at the end of the string, since it's a NULL anyway
		*comma = 0;
		
		// if we moved, we have a non-comma character, in which case we have a word
		if (comma > *spot) spot++;
		
		// if we haven't moved, we may be at the end or just dealing with some misguided
		// soul who has entered adjacent commas.  We do NOT break in that case, because
		// we *have* moved the comma pointer since the start of the iteration.
		// We rely on the comma<end test to exit the loop
		
		// We also don't need to NULL *spot here, we'll either overwrite it with the next
		// word or we'll NULL it after the loop
	}
	
	// NULL the final entry in our array of chars
	*spot = NULL;
	
	// if the first pointer is nil, return nil overall.  Otherwise, return our static array
	return *whitelist ? whitelist : nil;
}
		
