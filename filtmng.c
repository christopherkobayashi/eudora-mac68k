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

#include "filtmng.h"
/************************************************************************
 * Filters window - Copyright (C) 1994 QUALCOMM Incorporated
 ************************************************************************/
#define FILE_NUM 68

#ifdef TWO

#pragma segment FiltMng

OSErr GetFilterLine(FilterKeywordEnum *key,PStr value,LineIOP lip);
OSErr FilterVerb(PStr s, MatchEnum *mbm);
OSErr FilterConj(PStr s, ConjunctionEnum *cj);
OSErr WriteFilter(short refN,FRPtr fr);
void FUWeed(void);
short FindFilterById(long id);
short FilterCount(Handle hFilters);
extern OSErr FGlobalErr;

/**********************************************************************
 * FindFilterById - find which filter has the given id
 **********************************************************************/
short FindFilterById(long id)
{
	short nf;
	
	for (nf=NFilters-1;nf>=0;nf--)
		if (FR[nf].fu.id==id) break;
	
	return(nf);
} 

/**********************************************************************
 * FUWeed - weed out filter use records for non-existent filters
 **********************************************************************/
void FUWeed(void)
{
	short nf = NFilters;
	FUHandle fuh = GetResource_(FU_TYPE,FU_ID);
	FUPtr fup, fuEnd;
	OSErr err;
	
	/*
	 * more use records than filters?
	 */
	if (fuh && GetHandleSize_(fuh)/sizeof(FilterUse)>nf)
	{
		HNoPurge_(fuh);
		err = fnfErr;
		fup = (FUPtr)LDRef(fuh);
		fuEnd = fup+GetHandleSize_(fuh)/sizeof(FilterUse);
		for (fup=fuEnd;fup>=*fuh;fup--)
		{
			nf=FindFilterById(fup->id);
			
			/*
			 * didn't find it
			 */
			if (nf<0)
			{
				/* move others down */
				if (fuEnd-fup>1) BMD(fup+1,fup,(fuEnd-fup-1)*sizeof(FilterUse));
				
				/* shrink handle */
				SetHandleBig_(fuh,GetHandleSize_(fuh)-sizeof(FilterUse));
				
				/* watch the end */
				fuEnd--;
				
				/* mark as changed */
				ChangedResource((Handle)fuh);
			}
		}
		UL(fuh);
		PurgeIfClean(fuh);
	}
}

/**********************************************************************
 * FilterNewId - get a new id for a filter.
 **********************************************************************/
long FilterNewId(void)
{
	short f;
	long id = 1;
	
	for (f=NFilters;f;)
	{
		f--;
		if (id<=FR[f].fu.id) id = FR[f].fu.id+1;
	}
	return(id);
}

/**********************************************************************
 * GeneratePluginFilters - generate plugin filters
 **********************************************************************/
void GeneratePluginFilters(void)
{
	if (!PreFilters && !PostFilters)
	{
		Str31 name;
		FSSpec folderSpec;
		CInfoPBRec hfi;
		long	dirID;
		Handle	*pFilterSet;
#define TYPE hfi.hFileInfo.ioFlFndrInfo.fdType

		//	Load any plug-in filters
		ETLGetPluginFolderSpec(&folderSpec,PLUGIN_FILTERS);
		dirID = SpecDirId(&folderSpec);
			
		// iterate through all the plugin filter files
		hfi.hFileInfo.ioNamePtr = name;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate(folderSpec.vRefNum,dirID,&hfi))
		{
			pFilterSet = (TYPE==PRE_FILTER_TYPE || TYPE=='TEXT' && EndsWithR(name,PREFILTER_SUFFIX)) ? &PreFilters :
				(TYPE==POST_FILTER_TYPE || TYPE=='TEXT' && EndsWithR(name,POSTFILTER_SUFFIX)) ? &PostFilters : nil;
			if (pFilterSet)
			{
				if (!*pFilterSet) *pFilterSet = NuHandle(0);
				if (*pFilterSet) ReadFilters(*pFilterSet,folderSpec.vRefNum,dirID,name);
			}
		}
		FiltersRefCount++;
	}
}

/************************************************************************
 * RegnerateFilters - read the filters list
 ************************************************************************/
OSErr RegenerateFilters(void)
{
	short err = noErr;
	Str255 s;
	
	if (!Filters || !*Filters)
	{
		if (Filters) ZapHandle(Filters);
		Filters = NuHandle(0);
		if (!Filters)
		{
			WarnUser(READ_FILTERS,err=MemError());
			return(err);
		}
		ProgressR(NoBar,NoBar,0,READING_FILTERS,"");
		err = ReadFilters(Filters,Root.vRef,Root.dirId,GetRString(s,FILTERS_NAME));
		HNoPurge_(Filters);
		if (!err)
			GeneratePluginFilters();
	}
	
	if (!err)
	{
		short nf;
		short t;
		FiltersRefCount++;
		for (nf=NFilters-1;nf>=0;nf--)
		{
			for (t=0;t<2;t++)
			{
				ZapHandle(FR[nf].terms[t].nickExpanded);
				ZapHandle(FR[nf].terms[t].nickAddresses);
				ZapHandle(FR[nf].terms[t].regex);
			}
		}
	}
	else
		ZapFilters();

	FGlobalErr = err;
	
	return(err);
}

/************************************************************************
 * ReadFilters - read the filters database
 ************************************************************************/
OSErr ReadFilters(Handle hFilters,short vRef,long dirId,StringPtr name)
{
	short err;
	FilterRecord fr;
	FilterKeywordEnum key;
	Str255 s;
	short term=0;
	LineIOD lid;
	FActionProc *fap;
	FActionHandle fa=nil;

	Boolean	proOnlyKeywordFound = false;
	
	Zero(lid);
	
	FRInit(&fr);
	err = OpenLine(vRef,dirId,name,fsRdWrPerm,&lid);
	if (err==fnfErr) return(noErr);	/* vaccuous success */
	else if (err) return(FileSystemError(READ_FILTERS,s,err));

	while(!(err=GetFilterLine(&key,s,&lid)))
	{
		//Enhanced Filters - warn user ONCE if the filters file contains a Pro-only keyword
		if (!HasFeature (featureFilters) && !proOnlyKeywordFound && FAProOnly(key)) proOnlyKeywordFound = true;
		/*
		 * prescan for obsolete keys
		 */
		switch(key)
		{
			case flkRaise:
				PCopy(s,"\p7");
				key = flkPriority;
				break;
			case flkLower:
				PCopy(s,"\p8");
				key = flkPriority;
				break;
		}
		
		/*
		 * look for actions
		 */
		if (fap = FATable(key))
		{
			if (!(fa=NewZH(FAction)))
			{
				WarnUser(READ_FILTERS,err=MemError());
				goto done;
			}
			(*fa)->action = key;
			if (err = (*fap)(faeRead,fa,nil,s)) goto done;
			LL_Queue(fr.actions,fa,(FActionHandle));
			fa = nil;
		}
		
		/*
		 * nope; other stuff
		 */
		else
		{
			switch(key)
			{
				case flkRule:
					if (*fr.name && (err=AppendFilter(&fr,hFilters))) goto done;
					FRInit(&fr);
					term = 0;
					PSCopy(fr.name,s);
					break;
				case flkIncoming:
					fr.incoming = True;
					break;
				case flkOutgoing:
					fr.outgoing = True;
					break;
				case flkManual:
					fr.manual = True;
					break;
				case flkCopyInstead:
					for (fa=fr.actions;fa && (*fa)->next;fa = (*fa)->next);
					if (fa && (*fa)->action==flkTransfer) (*fa)->action=flkCopy;
					break;
				case flkId:
					StringToNum(s,&fr.fu.id);
					break;
				case flkHeader:
					if (EqualStrRes(s,FILTER_ADDRESSEE_OLD)) GetRString(s,FILTER_ADDRESSEE);
					PSCopy(fr.terms[term].header,s);
					break;
				case flkVerb:
					err = FilterVerb(s,&fr.terms[term].verb);
					break;
				case flkValue:
					PSCopy(fr.terms[term].value,s);
					break;
				case flkConjunction:
					err = FilterConj(s,&fr.conjunction);
					if (term) WarnUser(FILTER_OVERTERM,0);
					else term++;
					break;
			}
		}
	}
	if (*fr.name)	err = AppendFilter(&fr,hFilters);
	fa = fr.actions = nil;	/* success! */
		
done:
	ZapFAction(fa);
	ZapActions(fr.actions);
	CloseLine(&lid);
	//Enhanced Filters - warn user if the filters file contains a Pro-only keyword
	if (proOnlyKeywordFound&&(!PrefIsSet(PREF_PRO_FILT_WARN))&&hFilters==Filters) Wife(0, PREF_PRO_FILT_WARN, PRO_FILT_WARNING);
	return(err==eofErr ? noErr : err);
}

/**********************************************************************
 * AppendFilter - add a filter to the list
 **********************************************************************/
OSErr AppendFilter(FRPtr fr,Handle hFilters)
{
	short na=0;
	FActionHandle fa;
	OSErr err=0;
	
	StudyFilter(fr);
	
	for (fa=fr->actions;fa;fa=(*fa)->next) na++;

	/*
	 * fill out the actions
	 */
	while (na<MAX_ACTIONS)
	{
		fa = NewZH(FAction);
		if (!fa) {err = MemError();break;}
		(*fa)->action = flkNone;
		LL_Queue(fr->actions,fa,(FActionHandle));
		na++;
	}
	if (!err) err = PtrPlusHand_(fr,hFilters,sizeof(*fr));
	if (err) WarnUser(READ_FILTERS,err);
	return(err);
}

/**********************************************************************
 * TellFiltMBRename - Tell filters that a mailbox name has changed
 *  When called with "will" true, the effect is to read filters in
 *  When called with "will" false, the filters are then updated
 **********************************************************************/
OSErr TellFiltMBRename(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn)
{
	short n;
	Boolean care=False;
	FActionHandle fa;
	MBRenamePB rnPB;
	FilterRenameEnum warn = GetPrefLong(PREF_MB_FILT_WARN);
	OSErr	err = noErr;
	
	if (warn==fRenameIgnore) return noErr;	// We don't care
		
	if (RegenerateFilters()) return noErr;
	
	if (!will)
	{
		/*
		 * Ok, now all the mailboxes are in FSSpec's in the actions.
		 * If we're renaming a folder, the FSSpec's are unaffected and we just
		 * have to make sure they get written out if we're using pathnames.
		 * If we have renamed a mailbox, still work to do.
		 */
		if (folder)
		{
			care = True;
			warn = fRenameChange;
		}
		else
		{
			n = NFilters;
			rnPB.oldSpec = *spec;
			rnPB.newSpec = *newSpec;
			
			if (warn==fRenameAsk)
			{
				care = False;
				while (n-- && !care)
				for (fa=FR[n].actions;!care && fa;fa=(*fa)->next)
					care = 0!=CallAction(faeMBWillRename,fa,nil,&rnPB);
				if (!care) return noErr;
			}
		}
		
		if (care)
		{
			if (dontWarn) warn = fRenameChange;
			else if (warn==fRenameAsk)			
				warn = ReallyDoAnAlert(FILT_MB_RENAME_ALRT,Note);
		
			if (warn!=fRenameIgnore)
			{
				if (!folder)
				{
					n = NFilters;
					while(n--)
						for (fa=FR[n].actions;fa;fa=(*fa)->next)
							CallAction(faeMBDidRename,fa,nil,&rnPB);
				}
				if (!FiltWinOpen()) SaveFilters();
				else FilterWindowClean(False);
			}
			else
				err = userCanceledErr;
		}
		FiltersDecRef();
	}
	return err;
}


/**********************************************************************
 * StudyFilter - study a filter, hopefully to speed execution
 **********************************************************************/
void StudyFilter(FRPtr fr)
{
	short term;
	
	for (term=0;term<sizeof(fr->terms)/sizeof(fr->terms[0]);term++)
	{
		if (EqualStrRes(fr->terms[term].header,FILTER_BODY))
			fr->terms[term].headerID = FILTER_BODY;
		else if (EqualStrRes(fr->terms[term].header,FILTER_ANY))
			fr->terms[term].headerID = FILTER_ANY;
		else if (EqualStrRes(fr->terms[term].header,FILTER_ADDRESSEE))
			fr->terms[term].headerID = FILTER_ADDRESSEE;
		else fr->terms[term].headerID = 0;
		
		if (fr->terms[term].verb==mbmRegEx)
		{
			Str255 s;
			PCopy(s,fr->terms[term].value);
			PTerminate(s);
			if (fr->terms[term].regex) ZapHandle(fr->terms[term].regex);
			fr->terms[term].regex = regcomp(s+1);
		}
	}
}

/**********************************************************************
 * DisposeFaction - dispose of a filter action
 **********************************************************************/
void DisposeFAction(FActionHandle fa)
{
	if (fa!=nil) 
	{
		if ((*fa)->data!=nil) DisposeHandle((Handle)(*fa)->data);
		DisposeHandle((Handle)fa);
	}
}

/**********************************************************************
 * ZapFilters - kill all filters
 **********************************************************************/
void ZapFilters(void)
{
	DisposeFilters(&Filters);
	DisposeFilters(&PreFilters);
	DisposeFilters(&PostFilters);
	FiltersRefCount = 0;
}

/**********************************************************************
 * DisposeFilters - dispose filters
 **********************************************************************/
void DisposeFilters(Handle *hFilters)
{
	if (*hFilters)
	{
		short i = FilterCount(*hFilters);
		
		while (i--) ZapActions((*(FRHandle)*hFilters)[i].actions);		
		ZapHandle(*hFilters);
	}
}

/**********************************************************************
 * ZapActions - zap a list of filter actions
 **********************************************************************/
void ZapActions(FActionHandle fa)
{
	FActionHandle thisfa;
	
	while (thisfa=fa)
	{
		LL_Remove(fa,thisfa,(FActionHandle));
		DisposeFAction(thisfa);
	}
}

/**********************************************************************
 * FilterCount - return number of filters
 **********************************************************************/
short FilterCount(Handle hFilters)
{
	return hFilters ? GetHandleSize_(hFilters)/sizeof(FilterRecord) : 0;
}

/************************************************************************
 * FRInit - intialize an FR
 ************************************************************************/
void FRInit(FRPtr fr)
{
	WriteZero(fr,sizeof(*fr));
	fr->conjunction = fr->terms[0].verb = fr->terms[1].verb = 1;
}

/************************************************************************
 * FilterVerb - interpret a filter verb
 ************************************************************************/
OSErr FilterVerb(PStr s, MatchEnum *mbm)
{
	if (!*s) return(1);	/* error; no token */
	
	*mbm = FindSTRNIndex(VERB_STRN,s);
	if (!*mbm) return(1);
	return(*mbm<mbmLimit ? noErr : 1);
}

/************************************************************************
 * FilterConj - interpret a filter conjunction
 ************************************************************************/
OSErr FilterConj(PStr s, ConjunctionEnum *cj)
{
	if (!*s) return(1);	/* error; no token */
	
	*cj = FindSTRNIndex(CONJ_STRN,s);
	if (!*cj) return(1);
	return(*cj<=cjUnless ? noErr : 1);
}

/************************************************************************
 * FiltersDecRef - decrement the filters ref count, and make purgeable if 0
 ************************************************************************/
void FiltersDecRef(void)
{
	short n;
		
	if (FiltersRefCount) FiltersRefCount--;
	if (!Filters) return;
	
	/*
	 * get rid of cached info
	 */
	if (!FiltersRefCount) for (n=NFilters;n--;)
	{
		ZapHandle(FR[n].terms[0].nickExpanded);
		ZapHandle(FR[n].terms[1].nickExpanded);
		ZapHandle(FR[n].terms[0].nickAddresses);
		ZapHandle(FR[n].terms[1].nickAddresses);
	}
}

/************************************************************************
 * GetFilterLine - read a line from the filter file
 ************************************************************************/
OSErr GetFilterLine(FilterKeywordEnum *key,PStr value, LineIOP lip)
{
	Str255 line;
	short code;
	static UHandle cmdH;
	UPtr spot;
	

#ifdef TWO
	if (!cmdH || !*cmdH)
	{
		if (cmdH) LoadResource(cmdH);
		cmdH = GetResource_('STR#',FILT_CMD_STRN);
		if (!cmdH || !*cmdH) return(ResError());
		HNoPurge(cmdH);
	}
#endif
	
	do
	{
		code = GetLine(line+1,sizeof(line)-2,nil,lip);
		if (code==eofErr || !code || !line[1]) return(eofErr);
		else if (code<0) return(FileSystemError(READ_FILTERS,"",code));
	} while (line[1]=='\015' || line[1]=='#');
	
	for(spot=line+1;*spot && *spot!=' ' && *spot!='\015';spot++);
	
	line[0] = spot-line-1;
	*key = FindSTRNIndexRes(cmdH,line);
	if (!*key)
		ComposeStdAlert(Caution,FILT_BADKEY_FMT,line);
	else
	{
		*value = strlen(line+*line+2);
		BMD(line+*line+2,value+1,*value);
		if (value[*value]=='\015') --*value;
		value[*value+1] = 0; /* null terminate, just for fun */
	}

	return(noErr);
}


/************************************************************************
 * SaveFilters - save the filters
 ************************************************************************/
OSErr SaveFiltersLo(FSSpecPtr toSpec,Boolean all,Boolean clean)
{
	FSSpec realSpec, tmpSpec;
	Str31 name, sfx;
	short refN;
	short err;
	short i;
	short n = NFilters;
	long eof;
	FInfo info;
	
	SaveCurrentFilter();
	
	// What's our filename?
	if (toSpec)
	{
		realSpec = *toSpec;
		PSCopy(name,toSpec->name);
	}
	else
	{
		GetRString(name,FILTERS_NAME);
		FSMakeFSSpec(Root.vRef,Root.dirId,name,&realSpec);
		IsAlias(&realSpec,&realSpec);	/* resolve */
	}
	
	// Setup a temp file
	GetRString(sfx,TEMP_SUFFIX);
	PSCat(name,sfx);
	FSMakeFSSpec(realSpec.vRefNum,realSpec.parID,name,&tmpSpec);
	
	/*
	 * open temp file
	 */
	(void) FSpCreate(&realSpec,CREATOR,FILTER_FTYPE,smSystemScript);
	(void) FSpCreate(&tmpSpec,CREATOR,FILTER_FTYPE,smSystemScript);
	if (err=FSpOpenDF(&tmpSpec,fsRdWrPerm,&refN))
		return(FileSystemError(SAVE_FILTERS,tmpSpec.name,err));
	
	/*
	 * write the filters
	 */
	LDRef(Filters);
	for (i=0;i<n;i++)
		if (all || FilterIsSelected(i))
			if (err=WriteFilter(refN,&FR[i])) goto done;
	
	/*
	 * truncate, just in case
	 */
	if (err = GetFPos(refN,&eof))
	{
		FileSystemError(SAVE_FILTERS,tmpSpec.name,err);
		goto done;
	}
	SetEOF(refN,eof);
	
	/*
	 * close
	 */
	if (err=MyFSClose(refN))
	{
		refN = 0;
		FileSystemError(SAVE_FILTERS,tmpSpec.name,err);
		goto done;
	}
	refN = 0;
	
	/*
	 * exchange
	 */
	err=ExchangeAndDel(&tmpSpec,&realSpec);
	
	/*
	 * set file type
	 */
	if (!err && !(err=AFSpGetFInfo(&realSpec,&tmpSpec,&info)))
	{
		info.fdType = FILTER_FTYPE;
		FSpSetFInfo(&tmpSpec,&info);
	}
	
	/*
	 * hurray!
	 */
done:
	UL(Filters);
	if (refN) MyFSClose(refN);
	if (!err && clean) FilterWindowClean(True);
	if (!err) FUWeed();	/* get rid of extra use records */
	return(err);
}

/************************************************************************
 * WriteFilter - write a given filter
 ************************************************************************/
OSErr WriteFilter(short refN,FRPtr fr)
{
	short err;
	Str15 string;
	FActionHandle fa;
	
	
	if (err=FWriteKey(refN,flkRule,fr->name)) return(err);
	if (!fr->fu.id) fr->fu.id = FilterNewId();
	NumToString(fr->fu.id,string);
	if (err=FWriteKey(refN,flkId,string)) return(err);
	if (err=FWriteBool(refN,flkIncoming,fr->incoming)) return(err);
	if (err=FWriteBool(refN,flkOutgoing,fr->outgoing)) return(err);
	if (err=FWriteBool(refN,flkManual,fr->manual)) return(err);
	if (err=FWriteKey(refN,flkHeader,fr->terms[0].header)) return(err);
	if (err=FWriteEnum(refN,flkVerb,fr->terms[0].verb)) return(err);
	if (err=FWriteKey(refN,flkValue,fr->terms[0].value)) return(err);
	if (fr->conjunction && fr->conjunction!=cjIgnore)
	{
		if (err=FWriteEnum(refN,flkConjunction,fr->conjunction)) return(err);
		if (err=FWriteKey(refN,flkHeader,fr->terms[1].header)) return(err);
		if (err=FWriteEnum(refN,flkVerb,fr->terms[1].verb)) return(err);
		if (err=FWriteKey(refN,flkValue,fr->terms[1].value)) return(err);
	}
	for (fa=fr->actions;!err && fa;fa = (*fa)->next)
		err = CallAction(faeWrite,fa,nil,&refN);
	
	return(err);
}

/************************************************************************
 * FWriteKey - write a keyword and value
 ************************************************************************/
OSErr FWriteKey(short refN,FilterKeywordEnum flk,PStr value)
{
	Str255 s;
	short err=noErr;
	
	if (*value)
	{
		err = FSWriteP(refN,ComposeString(s,"\p%r %p\015",FILT_CMD_STRN+flk,value));
		if (err) FileSystemError(SAVE_FILTERS,"",err);
	}
	return(err);
}

/************************************************************************
 * FWriteBool - write a Boolean
 ************************************************************************/
OSErr FWriteBool(short refN,FilterKeywordEnum flk,Boolean value)
{
	Str255 s;
	short err=noErr;
	
	if (value)
	{
		err = FSWriteP(refN,ComposeString(s,"\p%r\015",FILT_CMD_STRN+flk));
		if (err) FileSystemError(SAVE_FILTERS,"",err);
	}
	return(err);
}

/************************************************************************
 * FWriteEnum - write an enum
 ************************************************************************/
OSErr FWriteEnum(short refN,FilterKeywordEnum flk,short e)
{
	Str255 s;
	short err=noErr;
	
	err = FSWriteP(refN,ComposeString(s,"\p%r %r\015",FILT_CMD_STRN+flk,
		(flk==flkVerb ? VERB_STRN:CONJ_STRN)+e));
	if (err) FileSystemError(SAVE_FILTERS,"",err);
	return(err);
}

Boolean FilterExists(DescType form, long selector)
{
	return(False);
}
OSErr CountFilters(long *howMany)
{
	return(noErr);
}
OSErr GetFilterProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
	return(noErr);
}
OSErr GetTermProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
	return(noErr);
}
OSErr AECreateFilter(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	return(noErr);
}
OSErr SetFilterProperty(AEDescPtr token, AEDescPtr data)
{
	return(noErr);
}
OSErr SetTermProperty(AEDescPtr token, AEDescPtr data)
{
	return(noErr);
}
OSErr AEDeleteFilter(FilterTokenPtr fp)
{
	return(noErr);
}

#endif
