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

#include "appleevent.h"
#define FILE_NUM 16
/* Copyright (c) 1993 by QUALCOMM Incorporated */

#pragma segment AppleEvents
typedef AEDesc *AEDescPtr;


#ifdef DEBUG
AEHandler DebugAEHandler;
ObjectAccessor DebugOAccessor;
#endif

/*
 * Handlers
 */
#define DECLARE_AEHANDLER(routine) AEEventHandlerUPP routine##UPP; \
	pascal OSErr routine(AppleEvent *,AppleEvent *,long)
#define DECLARE_OACCESSOR(routine) OSLAccessorUPP routine##UPP; \
	pascal OSErr routine(DescType,AEDescPtr,DescType,DescType,AEDescPtr,AEDescPtr,long)
DECLARE_AEHANDLER(HandleAEQuit);
DECLARE_AEHANDLER(HandleAEPref);
DECLARE_AEHANDLER(HandleAEOApp);
DECLARE_AEHANDLER(HandleAEODoc);
DECLARE_AEHANDLER(HandleAEOObj);
DECLARE_AEHANDLER(HandleEuConnect);
DECLARE_AEHANDLER(HandleAEGetData);
DECLARE_AEHANDLER(HandleAEMove);
DECLARE_AEHANDLER(HandleAESetData);
DECLARE_AEHANDLER(HandleAEAnswer);
DECLARE_AEHANDLER(HandleAECreate);
DECLARE_AEHANDLER(HandleAEDelete);
DECLARE_AEHANDLER(HandleEuINot);
DECLARE_AEHANDLER(HandleEuRNot);
DECLARE_AEHANDLER(HandleEuCompact);
DECLARE_AEHANDLER(HandleEuReply);
DECLARE_AEHANDLER(HandleEuQueue);
DECLARE_AEHANDLER(HandleEuUnQueue);
DECLARE_AEHANDLER(HandleEuAttach);
DECLARE_AEHANDLER(HandleAECount);
DECLARE_AEHANDLER(HandleAEClose);
DECLARE_AEHANDLER(HandleAEExists);
DECLARE_AEHANDLER(HandleAEPrint);
DECLARE_AEHANDLER(HandleAESave);
DECLARE_AEHANDLER(HandleFinderCruft);
DECLARE_AEHANDLER(HandleAEURL);
DECLARE_AEHANDLER(HandleEuConduitAction);

DECLARE_OACCESSOR(FindBoxInMFolder);
DECLARE_OACCESSOR(FindBox);
DECLARE_OACCESSOR(FindMFolderInMFolder);
DECLARE_OACCESSOR(FindMFolder);
DECLARE_OACCESSOR(FindMessage);
DECLARE_OACCESSOR(FindProperty);
DECLARE_OACCESSOR(FindField);
DECLARE_OACCESSOR(FindAttachment);
DECLARE_OACCESSOR(FindWin);
DECLARE_OACCESSOR(FindWinTE);
DECLARE_OACCESSOR(FindWTStuff);
DECLARE_OACCESSOR(FindWTNest);
DECLARE_OACCESSOR(FindPreference);
DECLARE_OACCESSOR(FindPreferenceInPersonality);
DECLARE_OACCESSOR(FindPersonality);
DECLARE_OACCESSOR(FindFilter);
DECLARE_OACCESSOR(FindTerm);
DECLARE_OACCESSOR(FindURLHelper);
DECLARE_OACCESSOR(FindNickFile);
DECLARE_OACCESSOR(FindNickname);
DECLARE_OACCESSOR(FindNickField);

ElementCounter CountMailboxElements,CountMailfolderElements,CountMessageElements;
TokenTaker GetMailboxProperty,GetMailfolderProperty,GetMessProperty,GetApplicationProperty,GetMessData,GetField,GetAttachment,GetSomeText;
TokenTaker GetAEPref, GetAEHelper;
Boolean MessExists(AEDescPtr token);
Boolean BoxExists(AEDescPtr token);
Boolean FolderExists(AEDescPtr token);
Boolean FieldExists(AEDescPtr token);
Boolean AttachmentExists(AEDescPtr token);
Boolean PrefExists(AEDescPtr token);
OSErr GetAEStationeryIndex(AppleEvent *ae,short *index);

/*
 * private routines
 */
OSErr AddSourcePTE(AppleEvent *ae,PETEHandle sourcePTE);
OSErr OpenAESettings(AEDescList *dox);
OSErr OpenSettingsIfDifferent(FSSpecPtr spec);
OSErr OpenAEDox(AEDescList *dox,PETEHandle pte);
void InstallAEOrDie(AEEventClass theAEEventClass,AEEventID theAEEventID,AEEventHandlerProcPtr proc,AEEventHandlerUPP *upp, long refCon);
void InstallOAOrDie(DescType desiredClass, DescType containerType,OSLAccessorProcPtr proc,OSLAccessorUPP *upp);
OSErr OpenMessByToken(MessTokenPtr tokenP,Boolean showIt,MessHandle *messHP);
OSErr CloseMessByToken(MessTokenPtr tokenP);
OSErr GetMessField(MessHandle messH,PStr wantHead,AppleEvent *reply,long refCon);
OSErr MessObjFromBoxD(AEDescPtr boxDescP,short mNum,AEDescPtr messDescP);
OSErr MessObjFromBoxDHash(AEDescPtr boxDescP,uLong hash,AEDescPtr messDescP);
OSErr MessObjFromTOC(TOCHandle tocH,short mNum,AEDescPtr messDescP);
OSErr BoxObj(TOCHandle tocH,AEDescPtr boxDescP);
OSErr SetMessProperty(AEDescPtr token,AEDescPtr descP);
OSErr SetApplicationProperty(AEDescPtr token,AEDescPtr descP);
OSErr SetSomeText(AEDescPtr token,AEDescPtr descP);
OSErr SetField(AEDescPtr token,AEDescPtr descP);
OSErr SetPreference(AEDescPtr token,AEDescPtr descP);
OSErr SetMessField(MessHandle messH,PStr wantHead,AEDescPtr descP);
OSErr CreatorODoc(FSSpecPtr spec,OSType *mapCreator,Boolean printIt);
OSErr VolumeCreatorODoc(short vRef,OSType creator,FSSpecPtr spec,Boolean printIt);
OSErr RunningODoc(FSSpecPtr spec,OSType *mapCreator,PETEHandle sourcePTE,Boolean printIt);
OSErr BuildODoc(ProcessSerialNumberPtr psn,FSSpecPtr doc,AppleEvent *ae,Boolean printIt);
OSErr PSNODoc(ProcessSerialNumberPtr psn, FSSpecPtr doc,PETEHandle sourcePTE,Boolean printIt);
OSErr SFODoc(FSSpecPtr doc,Boolean printIt);
OSErr OpenAECruft(AEDescList *dox);
Boolean SimilarAppIsRunning(FSSpecPtr app,ProcessSerialNumberPtr psn);
Boolean CreatorMap(FSSpecPtr spec,OSType *creator);
OSErr SaveCreatorPref(OSType type, FSSpecPtr app);
OSErr FindWTChar(UPtr text,long spot,TERTPtr token);
OSErr FindWTWord(UPtr text,long spot,TERTPtr token);
OSErr BadRange(long start, long end, long len);
long StartOfWord(UPtr text, long len, long word);
long EndOfWord(UPtr text, long len, long word);
void BackWord(UPtr text, long len, long word, long *begin, long *end);
void ForWord(UPtr text, long len, long word, long *begin, long *end);
OSErr MyTextRange(long len,AEDescPtr keyData,TERTPtr token);
OSErr ExtractTargetAD(AppleEvent *event,AEAddressDesc *targetAD);
OSErr AddNotifyDesc(AEDesc *targetAD,OSType ofWhat,Boolean justRemove);
void NotifyAHelper(AliasHandle alias,AEDescList *listD,OSType ofWhat,AEDescList *optD);
void NotifyPSN(ProcessSerialNumber *psn,AEDescList *listD,OSType ofWhat,AEDescList *optD);
typedef pascal OSErr AECountStuffType(DescType desiredClass,DescType containerClass,AEDescPtr containerToken,long *howMany);
OSErr GetMessageDO(AppleEvent *event, AEKeyword keyword, AEDescPtr obj);
OSErr CurrentMessageDesc(AEDescPtr obj);
OSErr CurrentMessage(TOCHandle *tocHP, short *mNumP);
OSErr FindFolderByIndex(AEDescPtr inDesc,long index,VDIdPtr vdid);
OSErr FindBoxByIndex(short vRef, long dirId,long index,FSSpecPtr spec);
OSErr PrintMessFromToken(AEDescPtr token,Boolean selection);
OSErr SaveMessFromToken(AEDescPtr token);
OSErr AttachEvent(AppleEvent *event, AppleEvent *reply, AEDescPtr obj, AEDescPtr list);
OSErr SetSelText(MyWindowPtr win,AEDescPtr descP);
OSErr GetSelText(MyWindowPtr win,AppleEvent *reply,long refCon);
OSErr RemoveNotifyDesc(OSType ofWhat,OSType creator,FSSpecPtr spec);
OSErr GetWinProperty(AEDescPtr token,AppleEvent *reply, long refCon);
OSErr OpenAEStationery(AEDescList *dox);
OSErr OpenAEAttachedApps(AEDescList *dox);
OSErr AECreateMessage(DescType theClass, AEDescPtr inToken,AppleEvent *event, AppleEvent *reply);
short CountWindows(void);
short CountURLHelpers(void);
OSErr AEOpenWindow(WindowPtr theWindow);
OSErr AddWinToEvent(AppleEvent *reply,AEKeyword keyword,MyWindowPtr win);
OSErr SetWinProperty(AEDescPtr token, AEDescPtr data);
OSErr MyCreateRange(DescType elemClass,long start,long finish,AEDescPtr inObj,AEDescPtr range);
OSErr CompPutSpellableFields(MyWindowPtr win,AEDescPtr textAD,AEDescPtr objAD);
OSErr AESetSig(TOCHandle tocH,short sumNum,AEDescPtr descP);
OSErr SaveFinderType(OSType type);
OSErr RemoveFinderType(OSType type);
OSErr AECreateURLHelper(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply);
OSErr SetURLHelperProperty(AEDescPtr token,AEDescPtr descP);
Boolean GetAttachmentFromMsg(MessHandle mess,short index,FSSpec *spec);
OSErr MessObjFromBoxDTOC(AEDescPtr boxDescP,TOCHandle tocH,short mNum,AEDescPtr messDescP);
pascal OSErr AECountStuff(DescType desiredClass,DescType containerClass,AEDescPtr containerToken,long *howMany);
pascal Boolean CantOpenFilt(CInfoPBPtr pb);

/************************************************************************
 * HandleEuAttach - attach something to a message
 ************************************************************************/
pascal OSErr HandleEuAttach(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEDesc obj;
	AEDescList list;
	
	FRONTIER;	
	NullADList(&obj,&list,nil);
	
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	if (!(err = AEGetParamDesc(event,keyEuDocumentList,typeAEList,&list)))
		err = AttachEvent(event,reply,&obj,&list);
	DisposeADList(&obj,&list,nil);
	return(err);
}

/**********************************************************************
 * AttachEvent - handle the basic attachment stuff
 **********************************************************************/
OSErr AttachEvent(AppleEvent *event, AppleEvent *reply, AEDescPtr obj, AEDescPtr list)
{
	OSErr err;
	AEDesc token;
	DescType gotType;
	Size gotSize;
	MessToken messT;
	long i,n;
	FSSpec spec;
	DescType junkKW;
	MessHandle messH;
	Boolean spool;
	
	NullADList(&token,nil);

	err = AEGetParamPtr_(event,keyEuSpool,typeBoolean,&gotType,&spool,sizeof(spool),&gotSize);
	if (err==errAEDescNotFound) spool = False;
	else if (err) return(err);
	
	if (!(err=GotAERequired(event)))
	{
		err = AEResolve(obj,kAEIDoMinimum,&token);
		if (!err)
		{
			switch (token.descriptorType)
			{
				case cEuMessage:
					AEGetDescData(&token,&messT,sizeof(messT));
					if (!EqualStrRes(messT.spec.name,OUT) || !SameVRef(MailRoot.vRef,messT.spec.vRefNum) || !MailRoot.dirId==messT.spec.parID)
						err = errAEEventNotHandled;
					else if (!(err = OpenMessByToken(&messT,false,&messH)))
					{
						WindowPtr	messWinWP;
						
						if (SumOf(messH)->state!=SENDABLE && SumOf(messH)->state!=UNSENDABLE)
							err = errAEEventNotHandled;
						else if (!(err = AECountItems(list,&n)))
						{
							for (i=1;i<=n;i++)
							{
								if (err = AEGetNthPtr(list,i,typeFSS,&junkKW,&gotType,&spec,sizeof(spec),&gotSize))
									break;
								CompAttachSpec((*messH)->win,&spec);
								SpoolIndAttachment(messH,CountAttachments(messH));
							}
						}
						messWinWP = GetMyWindowWindowPtr ((*messH)->win);
						if (!IsWindowVisible (messWinWP))
						{
							SaveComp((*messH)->win);
							CloseMyWindow(messWinWP);
						}
					}
					else err = errAEEventNotHandled;
					break;
				default:
					err = errAEEventNotHandled;
					break;
			}
		}
	}
	DisposeADList(&token,nil);
	return(err);
}

/************************************************************************
 * GetMessageDO - get a message as a direct object.
 *  Uses the DO in the event if present, current message otherwise
 ************************************************************************/
OSErr GetMessageDO(AppleEvent *event, AEKeyword keyword, AEDescPtr obj)
{
	OSErr err;
	
	err = AEGetParamDesc(event,keyword,typeObjectSpecifier,obj);
	
	if (err == errAEDescNotFound)
		err = CurrentMessageDesc(obj);
	return(err);
}

/************************************************************************
 * CurrentMessageDesc - build aedesc for current message
 ************************************************************************/
OSErr CurrentMessageDesc(AEDescPtr obj)
{
	TOCHandle tocH;
	short sumNum;
	
	if (!CurrentMessage(&tocH,&sumNum))
		return(MessObjFromTOC(tocH,sumNum,obj));

	return(errAENoSuchObject);
}


/************************************************************************
 * 
 ************************************************************************/
OSErr CurrentMessage(TOCHandle *tocHP, short *mNumP)
{
	WindowPtr	winWP = FrontWindow_ ();
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	OSErr err = errAENoSuchObject;
	
	if (IsMyWindow(winWP))
	{
		switch (GetWindowKind (winWP))
		{
			case MESS_WIN:
			case COMP_WIN:
				*tocHP = (*Win2MessH(win))->tocH;
				*mNumP = (*Win2MessH(win))->sumNum;
				err = 0;
				break;
			case CBOX_WIN:
			case MBOX_WIN:
				*tocHP = (TOCHandle)GetWindowPrivateData(winWP);
				*mNumP = FirstMsgSelected(*tocHP);
				if (*mNumP>=0) err = 0;
				break;
		}
	}
	return(err);
}

/************************************************************************
 * HandleEuQueue - queue a message
 ************************************************************************/
pascal OSErr HandleEuQueue(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEDesc obj, token;
	long secs=0;
	MessToken messT;
	TOCHandle tocH;
	OSErr whenErr;
	AEDesc when;
	Str255 date;
	long gotType,gotSize;
	LongDateCvt longDate;
	
	FRONTIER;	
	
	NullADList(&obj,&token,&when,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		whenErr = AEGetParamPtr_(event,keyEuWhen,typeLongInteger,&gotType,&secs,sizeof(secs),&gotSize);
		if (whenErr)
		{
			whenErr = AEGetParamPtr_(event,keyEuWhen,typeLongDateTime,&gotType,&longDate.c,sizeof(longDate),&gotSize);
			if (!whenErr) secs = longDate.hl.lLow - ZoneSecs();
		}
		if (whenErr)
		{
			whenErr = AEGetParamDesc(event,keyEuWhen,typeWildCard,&when);
			if (!whenErr) secs = BeautifyDate(GetAEPStr(date,&when),&gotType);
		}
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cEuMessage:
						AEGetDescData(&token,&messT,sizeof(messT));
						if (!EqualStrRes(messT.spec.name,OUT) || !SameVRef(MailRoot.vRef,messT.spec.vRefNum) || !MailRoot.dirId==messT.spec.parID)
							err = errAEEventNotHandled;
						else if (tocH=TOCBySpec(&messT.spec)) err = QueueMessage(tocH,messT.index,refCon,whenErr ? 0 : secs,true,false);
						else err = errAEEventNotHandled;
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
			}
		}
	}
done:
	DisposeADList(&obj,&token,&when,nil);
	return(err);
}


/************************************************************************
 * HandleEuReply - reply to a message
 ************************************************************************/
pascal OSErr HandleEuReply(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDesc obj, token, repObj;
	Handle addresses = nil;
	short flags = 0;
	Boolean all, quote, self;
	MessHandle messH;
	MyWindowPtr repWin=nil;
	MessToken messT;
	short withStationery;

	ReplyDefaults(0,&all,&self,&quote);
	
	FRONTIER;	
	NullADList(&obj,&token,&repObj,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		all = GetAEDefaultBool(event,keyEuReplyAll,all);
		self = GetAEDefaultBool(event,keyEuIncludeSelf,self);
		quote = GetAEDefaultBool(event,keyEuQuoteText,quote);
		if (!(err=GetAEStationeryIndex(event,&withStationery)))
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cEuMessage:
						AEGetDescData(&token,&messT,sizeof(messT));
						if (!(err = OpenMessByToken(&messT,False,&messH)))
						{
							WindowPtr	messWinWP;
							switch (refCon)
							{
								case kEuReply:
									repWin = DoReplyMessage((*messH)->win,all,self,quote,True,withStationery,True,True,True);
									break;
								case kEuForward:
									repWin = DoForwardMessage((*messH)->win,0,True);
									break;
								case kEuRedirect:
									repWin = DoRedistributeMessage((*messH)->win,0,False,False,True);
									break;
								case kEuSalvage:
									repWin = DoSalvageMessage((*messH)->win,False);
									break;
							}
							if (!repWin) err = errAEEventNotHandled;
							else
							{
								err = MessObjFromTOC((*Win2MessH(repWin))->tocH,(*Win2MessH(repWin))->sumNum,&repObj);
								if (!err) err = AEPutParamDesc(reply,keyDirectObject,&repObj);
							}
							messWinWP = GetMyWindowWindowPtr ((*messH)->win);
							if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
						}
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
			}
		}
	}
	DisposeADList(&obj,&token,&repObj,nil);
	return(err);
}

/************************************************************************
 * HandleAEPrint - print something
 ************************************************************************/
pascal OSErr HandleAEPrint(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDesc obj, token;
	
	FRONTIER;	
	NullADList(&obj,&token,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cEuMessage:
						PrintMessFromToken(&token,False);
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
			}
		}
	}
	DisposeADList(&obj,&token,nil);
	return(err);
}

/************************************************************************
 * HandleAESave - save something
 ************************************************************************/
pascal OSErr HandleAESave(AppleEvent *event,AppleEvent *reply,long refCon)
{
	WindowPtr	theWindow;
	OSErr err;
	AEDesc obj, token;
	uLong	theWin;
	
	FRONTIER;	
	NullADList(&obj,&token,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cEuMessage:
						SaveMessFromToken(&token);
						break;
					case cWindow:
						AEGetDescData(&token,&theWin,sizeof(theWin));
						if (theWin>WIN_BAR_ITEM) 
						{
							theWindow = (WindowPtr)theWin;
							DoMenu(theWindow,(FILE_MENU<<16)|FILE_SAVE_ITEM,0);
						}
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
			}
		}
	}
	DisposeADList(&obj,&token,nil);
	return(err);
}


/************************************************************************
 * HandleFinderCruft - reflect the Finder's weird 'cwin' events back to itself
 ************************************************************************/
pascal OSErr HandleFinderCruft(AppleEvent *event,AppleEvent *reply,long refCon)
{
	return(noErr);
}


/**********************************************************************
 * PrintMessFromToken - print a message from a token
 **********************************************************************/
OSErr PrintMessFromToken(AEDescPtr token,Boolean selection)
{
	MessHandle messH;
	OSErr err;
	MessToken messT;
	
	AEGetDescData(token,&messT,sizeof(messT));
	err = OpenMessByToken(&messT,True,&messH);
	
	if (!err)
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);
		PrintOneMessage((*messH)->win,selection,True);
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);	
}

/**********************************************************************
 * SaveMessFromToken - print a message from a token
 **********************************************************************/
OSErr SaveMessFromToken(AEDescPtr token)
{
	MessHandle messH;
	OSErr err;
	MessToken messT;
	
	AEGetDescData(token,&messT,sizeof(messT));
	err = OpenMessByToken(&messT,True,&messH);
	
	if (!err)
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);
		if ((*(*messH)->tocH)->which==OUT) err = !SaveComp((*messH)->win);
		else err = errAEEventNotHandled;
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);	
}

/************************************************************************
 * MessObjFromTOC - create a message object from scratch
 ************************************************************************/
OSErr MessObjFromTOC(TOCHandle tocH,short mNum,AEDescPtr messDescP)
{
	OSErr err;
	AEDesc boxDesc;
	
	NullADList(&boxDesc,nil);
	
	if (!(err=BoxObj(tocH,&boxDesc)))
		err = MessObjFromBoxDTOC(&boxDesc,tocH,mNum,messDescP);
	DisposeADList(&boxDesc,nil);
	return(err);
}

/************************************************************************
 * MessObjFromTOC - create a message object from scratch using container box
 ************************************************************************/
OSErr MessObjFromBoxDTOC(AEDescPtr boxDescP,TOCHandle tocH,short mNum,AEDescPtr messDescP)
{	
	uLong	hash = (*tocH)->sums[mNum].uidHash;

	if (hash!=kNeverHashed && hash!=kNoMessageId)
		return MessObjFromBoxDHash(boxDescP,hash,messDescP);
	else
		return MessObjFromBoxD(boxDescP,mNum,messDescP);
}

/************************************************************************
 * BoxObj - create an object specifier for a mailbox
 *	Currently only handles top level
 ************************************************************************/
OSErr BoxObj(TOCHandle tocH,AEDescPtr boxDescP)
{
	FSSpec spec = GetMailboxSpec(tocH,-1);
	short err;
	AEDesc root,name,folder,fname;
	short menu;
	short item;
	MenuHandle mh;
	Str31 folderName;
	StackHandle stack;
	
	// make a stack to hold inner folder names
	if (err=StackInit(sizeof(folderName),&stack)) return err;
	
	// clear the decks
	NullADList(&root,&name,&folder,&fname,nil);
	
	// trace our path back to the start onto a stack,
	// using the menus as our guide
	err=Spec2Menu(&spec,False,&menu,&item);
	if (!err) mh = GetMHandle(menu);
	while (!err && mh && GetMenuID(mh)!=MAILBOX_MENU)
	{
		if (mh = ParentMailboxMenu(mh,&item))
		{
			MyGetItem(mh,item,folderName);
			err=StackQueue(folderName,stack);
		}
	}

	// Create the specifier for the root of the hierarchy
	if (!err)
	if (!(err=AECreateDesc(typeChar,"",0,&fname)))
	if (!(err=CreateObjSpecifier(cEuMailfolder,&root,formName,&fname,False,&folder)))
	{
		// Now, walk back the stack from the root to the enclosing folder
		for (item=1;item<=(*stack)->elCount;item++)
		{
			// make a descriptor for the name
			StackItem(folderName,item-1,stack);
			if (err=AECreateDesc(typeChar,folderName+1,*folderName,&fname)) break;
			
			// The folder is the root for the new folder
			DisposeADList(&root,nil);
			root = folder;
			NullADList(&folder,nil);
			
			// make the object specifier for the folder, relative to parent
			if (err=CreateObjSpecifier(cEuMailfolder,&root,formName,&fname,False,&folder)) break;
		}
		if (!err)
		{
				// now tack on the mailbox name and we're done
				if (!(err=AECreateDesc(typeChar,spec.name+1,*spec.name,&name)))
					err=CreateObjSpecifier(cEuMailbox,&folder,formName,&name,False,boxDescP);
		}
	}
	
	ZapHandle(stack);
	DisposeADList(&root,&name,&folder,&fname,nil);
	return(err);
}

/************************************************************************
 * HandleEuINot - handle a request to notify of new mail
 ************************************************************************/
pascal OSErr HandleEuINot(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	DescType gotType;
	Size gotSize;
	AEAddressDesc notifyDesc;
	OSType ofWhat;
	
	FRONTIER;	
	NullADList(&notifyDesc,nil);
	
	if (AEGetParamPtr_(event,keyEuWhatHappened,typeWildCard,&gotType,&ofWhat,sizeof(ofWhat),&gotSize))
		ofWhat = typeWildCard;

	if (!(err = AEGetParamDesc(event,keyDirectObject,typeAlias,&notifyDesc)) &&
			!(err = GotAERequired(event)))
	{
		err = AddNotifyDesc(&notifyDesc,ofWhat,False);
	}
	
	DisposeADList(&notifyDesc,nil);
	return(err);
}

/************************************************************************
 * HandleEuINot - handle a request to notify of new mail
 ************************************************************************/
pascal OSErr HandleEuRNot(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEDescPtr theDO;
	DescType gotType;
	Size gotSize;
	AEAddressDesc notifyDesc;
	OSType ofWhat;
	
	FRONTIER;	
	NullADList(&notifyDesc,nil);
	
	if (AEGetParamPtr_(event,keyEuWhatHappened,typeType,&gotType,&ofWhat,sizeof(ofWhat),&gotSize))
		ofWhat = typeWildCard;

	theDO = &notifyDesc;
	err = AEGetParamDesc(event,keyDirectObject,typeAlias,&notifyDesc);
	if (err || notifyDesc.descriptorType==typeNull) {err=noErr; theDO = nil;}
	if (!err && !(err = GotAERequired(event)))
	{
		err = AddNotifyDesc(theDO,ofWhat,True);
	}
	
	DisposeADList(&notifyDesc,nil);
	return(err);
}

/************************************************************************
 * HandleEuCompact - handle a request to compact mailboxes
 ************************************************************************/
pascal OSErr HandleEuCompact(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEAddressDesc mbListDesc;
	Str255 scratch;
	
	FRONTIER;	
	NullADList(&mbListDesc,nil);
	
	if (err = AEGetParamDesc(event,keyEuMailboxList,typeAEList,&mbListDesc))
	{
		//	No mailboxes specified. Compact all
		CInfoPBRec hfi;

		GetRString(scratch,TOC_SUFFIX);
		DoCompact(MailRoot.vRef,MailRoot.dirId,&hfi,*scratch);
		err = noErr;
	}
	else if (!(err = GotAERequired(event)))
	{
		//	Compact specified mailboxes
		uLong	n;
		short	i;
		
		if (!(err = AECountItems(&mbListDesc,&n)))
		{
			for (i=1;i<=n;i++)
			{
				AEDesc obj,token;
				FSSpec	spec;
				AEKeyword	keyword;

				if (!(err = AEGetNthDesc(&mbListDesc,i,typeObjectSpecifier,&keyword,&obj)))
				{
					if (!(err = AEResolve(&obj,kAEIDoMinimum,&token)))
					{
						if (token.descriptorType == cEuMailbox)
						{
							AEGetDescData(&token,&spec,sizeof(spec));
							CompactMailbox(&spec,false);
						}
						else
							err = errAEWrongDataType;
					}
					AEDisposeDesc(&obj);
				}
				if (err) break;
			}
		}		
	}
	
	DisposeADList(&mbListDesc,nil);
	return(err);
}

/************************************************************************
 * AddNotifyDesc - add an app to the list of apps to notify
 ************************************************************************/
OSErr AddNotifyDesc(AEDescPtr notifyDesc,OSType ofWhat,Boolean justRemove)
{
	OSErr err=noErr;
	Str31 name;
	AliasHandle alias, myAlias;
	FSSpec spec;
	FInfo info;
	Boolean junk;
	Handle resH;
	
	if (justRemove && notifyDesc==nil)
	{
		UseResFile(SettingsRefN);
		SetResLoad(False);
		while (resH=Get1IndResource(NOTIFY_TYPE,1))
		{
			SetResLoad(True);
			RemoveResource(resH);
			SetResLoad(False);
		}
		SetResLoad(True);
		MyUpdateResFile(SettingsRefN);
		return(noErr);
	}
	
	AEGetDescDataHandle(notifyDesc, (Handle *) &alias );
	if ((err=ResolveAlias(nil,alias,&spec,&junk)) ||
			(err=FSpGetFInfo(&spec,&info))) { AEDisposeDescDataHandle((Handle)alias); return(err); }
	*name = 8;
	BMD(&info.fdCreator,name+1,4);
	BMD(&ofWhat,name+5,4);
	RemoveNotifyDesc(ofWhat,info.fdCreator,&spec);
	if (!justRemove)
	{
		myAlias = alias;
		if (!(err=HandToHand_(&myAlias)))
		{
			(*myAlias)->userType = info.fdCreator;
			AddMyResource_(myAlias,NOTIFY_TYPE,MyUniqueID(NOTIFY_TYPE),name);
			err = ResError();
		}
	}
	AEDisposeDescDataHandle((Handle)alias);
	return(err);
}

/**********************************************************************
 * RemoveNotifyDesc; get rid of a notification resource
 **********************************************************************/
OSErr RemoveNotifyDesc(OSType ofWhat,OSType creator,FSSpecPtr spec)
{
	Str15 name;
	Handle old=nil;
	short i=0;
	OSType resCreator,resType,junk;
	OSErr err=errAENoSuchObject;
	FSSpec noteSpec;
	
	for (i=1;old=GetIndResource(NOTIFY_TYPE,i);i++)
	{
		GetResInfo(old,(void*)&junk,(void*)&junk,name);
		BMD(name+1,(void*)&resCreator,4);
		BMD(name+5,(void*)&resType,4);
		if (resCreator==creator && (ofWhat==typeWildCard || ofWhat==resType))
		{
			if (ResolveAlias(nil,(AliasHandle)old,&noteSpec,(Boolean*)&junk) || SameSpec(&noteSpec,spec))
			{
				RemoveResource(old);
				err = ResError();
				ZapHandle(old);
				err = noErr;
			}
		}
	}
	return(err);
}

/************************************************************************
 * NotifyHelpers - notify helper apps that mail has arrived
 ************************************************************************/
void NotifyHelpers(short newCount,OSType ofWhat, TOCHandle tocH)
{
	short mNum, aNum;
	AEAddressDesc listD, optD;
	AEDesc messD;
	OSErr err=noErr;
	Handle resH;
	Str31 name;
	OSType type;
	DescType messList;
	
	NullADList(&listD,&messD,&optD,nil);
	if (aNum = CountResources(NOTIFY_TYPE))
	{
		/*
		 * build list of messages
		 */
		if (!AECreateList(nil,0,False,&listD))
		{
			if (ofWhat==eMailArrive || ofWhat==eMailSent || ofWhat==eManFilter)
			{
				if (!tocH) tocH = ofWhat==eMailArrive ? GetInTOC() : GetOutTOC();
				if (!tocH) return;
				if (ofWhat==eManFilter)
				{
					for (mNum = 0;mNum<(*tocH)->count;mNum++)
						if ((*tocH)->sums[mNum].selected)
						{
							if (err=MessObjFromTOC(tocH,mNum,&messD)) break;
							if (err=AEPutDesc(&listD,0,&messD)) break;
						}
				}
				else
				{
					for (mNum = (*tocH)->count-newCount;mNum<(*tocH)->count;mNum++)
					{
						if (err=MessObjFromTOC(tocH,mNum,&messD)) break;
						if (err=AEPutDesc(&listD,0,&messD)) break;
					}
				}
			}
		}
		if (err) return;
		if (!AECreateList(nil,0,False,&optD))
		{
			messList = keyEuMessList;
			AEPutPtr(&optD,1,typeKeyword,&messList,sizeof(messList));
		}
	}
	
	/*
	 * do the notification
	 */
	while (aNum)
		if (resH=GetIndResource(NOTIFY_TYPE,aNum--))
		{
			GetResInfo(resH,(void*)&type,(void*)&type,name);
			BMD(name+5,&type,4);
			if (type==ofWhat || type==typeWildCard)
				NotifyAHelper((AliasHandle)resH,&listD,ofWhat,&optD);
		}
	
	DisposeADList(&listD,&messD,&optD,nil);
}

/************************************************************************
 * NotifyAHelper - notify a helper
 ************************************************************************/
void NotifyAHelper(AliasHandle alias,AEDescList *listD,OSType ofWhat,AEDescList *optD)
{
	ProcessSerialNumber psn;
	
	if (alias)
	{
		if (!FindPSNByAlias(&psn,alias)) NotifyPSN(&psn,listD,ofWhat,optD);
		else if (!LaunchByAlias(alias))
		{
			MiniEvents();
			if (!FindPSNByAlias(&psn,alias)) NotifyPSN(&psn,listD,ofWhat,optD);
		}
	}
}

/************************************************************************
 * NotifyPSN - notify a process by psn
 ************************************************************************/
void NotifyPSN(ProcessSerialNumber *psn,AEDescList *listD,OSType ofWhat,AEDescList *optD)
{
	SimpleAESend(psn,kEudoraSuite,kEuNotify,nil,kEAEWhenever,
		keyEuWhatHappened,typeEnumeration,&ofWhat,sizeof(ofWhat),
		keyEuMessList,keyEuDesc,listD,
		nil,
		keyOptionalKeywordAttr,keyEuDesc,optD,
		nil);
}

	
/************************************************************************
 * GetAEError - extract error, if any, from event
 ************************************************************************/
OSErr GetAEError(AppleEventPtr ae)
{
	short err;
	short internalErr;
	DescType gotType;
	Size gotSize;
	
	err = AEGetParamPtr_(ae,keyErrorNumber,typeShortInteger,&gotType,&internalErr,sizeof(short),&gotSize);
	if (err==errAEDescNotFound) return(noErr);
	else if (err) return(err);
	else return(internalErr);
}

/************************************************************************
 * HandleAEOApp - Handle the open application event
 ************************************************************************/
pascal OSErr HandleAEOApp(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	
	//LOGLINE;
	
	err = GotAERequired(event);
	if (err) return(err);

	/*
	 * if we're starting up, MyVRef will still be zero, and we need to go
	 * looking for our Eudora Folder.  If MyVRef is not zero, then we're
	 * just being switched in from the Finder (whoopee.)
	 */
	//LOGLINE;
	if (!SettingsRefN) SystemEudoraFolder();
	return(noErr);
}

/************************************************************************
 * HandleAEODoc - handle an open documents event
 ************************************************************************/
pascal OSErr HandleAEODoc(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDescList dox; //LOGLINE;
	PETEHandle pte=nil;
	DescType junk;
		
	NullDesc(&dox);
	
	if (ModalWindow && !AreAllModalsPlugwindows()) return(errAEEventNotHandled);
	
	FRONTIER;
		
	/*
	 * fetch the document list
	 */
	err = AEGetParamDesc(event,keyDirectObject,typeWildCard,&dox);
	
	/*
	 * fetch the source edit region (if any)
	 */
	if (AEGetParamPtr(event,keyEuEditRegion,typeLongInteger,&junk,(long*)&pte,sizeof(pte),(Size*)&junk))
		pte = nil;
	
	/*
	 * is opening objects?
	 */
	if (err == errAEDescNotFound || dox.descriptorType == cObjectSpecifier)
	{
		AEDisposeDesc(&dox);
		if (!SettingsRefN) SystemEudoraFolder();
		return(HandleAEOObj(event,reply,refCon));
	}
	
	/*
	 * nope, opening documents
	 */	
	if (!err)
	{
		err = GotAERequired(event);	/* check for required parms */ //LOGLINE;
		if (!err)
		{
			/*
			 * open the settings file first
			 */
			err = OpenAESettings(&dox); //LOGLINE;
			
			if (!err)
			{
				/*
				 * Ok, now make sure we're looking at SOME Eudora Folder; otherwise,
				 * life will not be good for opening the other documents
				 */
				if (!SettingsRefN) SystemEudoraFolder();
				
				/*
				 * open applications?
				 */
				err = OpenAEAttachedApps(&dox);
				
				/*
				 * cruft?
				 */
				if (!err && !(CurrentModifiers()&cmdKey)) err = OpenAECruft(&dox);
		
				/*
				 * now, open other files
				 */
				if (!err) err = OpenAEStationery(&dox);
				if (!err) err = OpenAEDox(&dox,pte);
			}
		}
		
		/*
		 * that was fun
		 */
		AEDisposeDesc(&dox);
	}
	return(err);
}

/************************************************************************
 * HandleAEQuit - Handle the quit event
 ************************************************************************/
pascal OSErr HandleAEQuit(AppleEvent *event,AppleEvent *reply,long refCon)
{
	PleaseQuit = true;
	return(noErr);
}

/************************************************************************
 * HandleAEPref - Handle the preferences event
 ************************************************************************/
pascal OSErr HandleAEPref(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	// If configuring the toolbar, we received this AE because Preferences
	// was chosen from the application menu
	if (!ConfiguringToolbarMenuItem(SPECIAL_MENU,SPECIAL_SETTINGS_ITEM))
	{
		OSErr err = GotAERequired(event);
		if (err) return(err);
		FRONTIER;	
		
		DoSettings(GetPrefLong(PREF_LAST_SCREEN));
	}
	return(noErr);
}

/************************************************************************
 * GotAERequired - did we get all the parameters?
 ************************************************************************/
OSErr GotAERequired(AppleEvent *event)
{
	DescType junkType;
	Size junkSize;
	OSErr err;
	
	err=AEGetAttributePtr(event,keyMissedKeywordAttr,typeWildCard,
												&junkType,nil,0,&junkSize);
	if (err==errAEDescNotFound) err = noErr;			/* no required params left */
	else if (err==noErr) err = errAEParamMissed;	/* uh-oh; found a required one */
	else ;																				/* other errs just get passed back */
	return(err);
}

/************************************************************************
 * OpenAESettings - open the settings files in the current dox list
 ************************************************************************/
OSErr OpenAESettings(AEDescList *dox)
{
	long i,n;
	OSErr err;
	AEKeyword junkKW;
	DescType junkType;
	Size junkSize;
	FSSpec spec;
	FInfo info;
	
	if (err = AECountItems(dox,&n)) return(err);  //LOGLINE;
	for (i=1;i<=n;i++)
	{
		if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
			break;  //LOGLINE;
		if (err = AFSpGetFInfo(&spec,&spec,&info)) break;
		if (info.fdType==SETTINGS_TYPE && info.fdCreator==CREATOR)
		{
			err = OpenSettingsIfDifferent(&spec);
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
	}
	return(err);
}

/************************************************************************
 * OpenAEStationery - open the stationery files in the current dox list
 ************************************************************************/
OSErr OpenAEStationery(AEDescList *dox)
{
	long i,n;
	OSErr err;
	AEKeyword junkKW;
	DescType junkType;
	Size junkSize;
	FSSpec spec;
	FInfo info;
	
	if (err = AECountItems(dox,&n)) return(err);
	for (i=1;i<=n;i++)
	{
		if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
			break;
		if (err = AFSpGetFInfo(&spec,&spec,&info)) break;
		if (info.fdType==STATIONERY_TYPE && info.fdCreator==CREATOR)
		{
			DoCompWStationery(&spec);
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
	}
	return(err);
}

/************************************************************************
 * OpenAEAttachedApps - open received applications
 ************************************************************************/
OSErr OpenAEAttachedApps(AEDescList *dox)
{
	long i,n;
	OSErr err;
	AEKeyword junkKW;
	DescType junkType;
	Size junkSize;
	FSSpec spec;
	FInfo info;
	
	if (err = AECountItems(dox,&n)) return(err);
	for (i=1;i<=n;i++)
	{
		if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
			break;
		if (err = AFSpGetFInfo(&spec,&spec,&info)) break;
		if ((info.fdType&0xffffff00)==(kFakeAppType&0xffffff00))
		{
			OpenAttachedApp(&spec);
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
	}
	return(err);
}

/************************************************************************
 * OpenAECruft - tell the user he can't open these
 ************************************************************************/
OSErr OpenAECruft(AEDescList *dox)
{
	long i,n;
	OSErr err;
	AEKeyword junkKW;
	DescType junkType;
	Size junkSize;
	FSSpec spec;
	FInfo info;
	Boolean plugWarned = False;
	Boolean helpWarned = False;
	Boolean filterWarned = False;
	
	if (err = AECountItems(dox,&n)) return(err);
	for (i=1;i<=n;i++)
	{
		if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
			break;
		if (err = AFSpGetFInfo(&spec,&spec,&info)) break;
		if (info.fdType==PLUG_TYPE && !plugWarned)
		{
			plugWarned = True;
			NoteUser(OPEN_PLUG_ERR,0);
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
		else if (info.fdType==HELP_TYPE && !helpWarned)
		{
			helpWarned = True;
			NoteUser(OPEN_HELP_ERR,0);
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
		else if (info.fdType==FILTER_FTYPE)
		{
			OpenFiltersWindow();
			AEDeleteItem(dox,i); i--; n--;		/* been there.  done that. */
		}
	}
	return(err);
}

/************************************************************************
 * OpenAEDox - open the non-settings files in the current dox list
 ************************************************************************/
OSErr OpenAEDox(AEDescList *dox,PETEHandle pte)
{
	long i,n;
	OSErr err;
	AEKeyword junkKW;
	DescType junkType;
	Size junkSize;
	FSSpec spec;
	FInfo info;
	
	if (err = AECountItems(dox,&n)) return(err);
	for (i=1;i<=n;i++)
	{
		if (err = AEGetNthPtr(dox,i,typeFSS,&junkKW,&junkType,&spec,sizeof(spec),&junkSize))
			break;
		if (err = AFSpGetFInfo(&spec,&spec,&info)) break;
		if (CurrentModifiers()&cmdKey) info.fdType = '????';	/* force attach */
		if (info.fdType==SETTINGS_TYPE && info.fdCreator==CREATOR) break;	// skip settings files for now
		if (err = OpenOneDoc(pte,&spec,&info)) break;
	}
	return(err);
}

/************************************************************************
 * OpenOneDoc - open a single file
 ************************************************************************/
OSErr OpenOneDoc(PETEHandle pte,FSSpecPtr spec,FInfo *info)
{
	OSErr err = noErr;
	FInfo localInfo;
	int pnPolicyCode;
	Boolean needsRegistration;
	
	if (!info)
	{
		Zero(localInfo);
		info = &localInfo;
		AFSpGetFInfo(spec,spec,info);
	}
	
	switch (info->fdType)
	{
		case 'TEXT':
		case 'ttro':
			if (IsMailbox(spec))
			{
				if (FindDirLevel(spec->vRefNum,spec->parID)>=0)
					err = GetMailbox(spec,True);
				else if (!GraftMailbox(MailRoot.vRef,MailRoot.dirId,spec,spec,True))
					err = GetMailbox(spec,True);
			}
			else
				err = !OpenText(spec,nil,nil,nil,True,nil,info->fdType=='ttro',false);
			break;
#ifdef	IMAP
		case IMAP_MAILBOX_TYPE:
			if (IsMailbox(spec)) err = GetMailbox(spec,True);
			break;
#endif
		case SETTINGS_TYPE: 
			err = OpenSettingsIfDifferent(spec);
			break;	/* ignore */
#ifdef ETL
		case MIME_FTYPE:
			if (PeteIsValid(pte))
			{
				// shdn't be necessary, but for the moment etldisplayfile has to have pte, won't
				// create own window  FIX THIS.
				InsertWin = (*PeteExtra(pte))->win;
				err = ETLDisplayFile(spec,pte);
				InsertWin = nil;
			}
			break;
#endif
		case STATIONERY_TYPE:
			/* already done */
			break;
		case SEARCH_FILE_TYPE:
			err = !OpenSearchFile(spec);
			break;
		case REG_FILE_TYPE:
			err = ParseRegFile(spec,parseDoDialog,&needsRegistration,&pnPolicyCode);
			break;
		default:
			// (jp) Okay to pass nil MyWindowPtr to AttachDoc
			err = AttachDoc(GetWindowMyWindowPtr(FrontWindow_()),spec);
			break;
	}

	return(err);
}


/************************************************************************
 * OpenSettingsIfDifferent - open a single settings file, if different from ours
 ************************************************************************/
OSErr OpenSettingsIfDifferent(FSSpecPtr spec)
{
	FSSpec curSpec;
	
	if (SettingsRefN)
	{
		GetFileByRef(SettingsRefN,&curSpec); //LOGLINE;
		if (SameSpec(&curSpec,spec)) return noErr; /* don't reopen settings */
	}
	OpenNewSettings(spec,false,GetNagState());	/* this will exit if it has trouble */ //LOGLINE;
	return noErr;
}


/************************************************************************
 * HandleEuConnect - handle the connect message
 ************************************************************************/
pascal OSErr HandleEuConnect(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	Boolean send,check,onIdle;
	DescType actType;
	Size actSize;
		
	FRONTIER;	
	/*
	 * do we do a send?
	 * if the send parameter is there, obey, otherwise use preference
	 */
	err = AEGetParamPtr_(event,keyEuSend,typeBoolean,&actType,&send,sizeof(send),&actSize);
	if (err==errAEDescNotFound) send = PrefIsSet(PREF_SEND_CHECK);
	else if (err) return(err);
	
	/*
	 * do we do a check?
	 * if the check parameter is there, obey, otherwise yes
	 */
	err = AEGetParamPtr_(event,keyEuCheck,typeBoolean,&actType,&check,sizeof(check),&actSize);
	if (err==errAEDescNotFound) check = True;
	else if (err) return(err);
	
	/*
	 * wait for idle time?
	 */
	err = AEGetParamPtr_(event,keyEuOnIdle,typeBoolean,&actType,&onIdle,sizeof(onIdle),&actSize);
	if (err==errAEDescNotFound) onIdle = False;
	else if (err) return(err);
	if (ModalWindow) onIdle = True;

	/*
	 * drudge
	 */
	if (err=GotAERequired(event)) return(err);
	
	/*
	 * Nifty!
	 */
	if (onIdle)
	{
		CheckOnIdle = (check ? kCheck : 0) | (send ? kSend : 0);
		return(noErr);
	}
	else
		return(XferMail(check,send,False,True,True,0));
}

/************************************************************************
 * HandleAEAnswer - handle AE replies.  Does nothing
 ************************************************************************/
pascal OSErr HandleAEAnswer(AppleEvent *event,AppleEvent *reply,long refCon)
{
	return(noErr);	/* okey-doke */
}

/************************************************************************
 * HandleAEOObj - handle an open object event
 ************************************************************************/
pascal OSErr HandleAEOObj(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	AEDesc obj, token;
	FSSpec spec;
	MessToken messT;
	WindowPtr	win;
	
	FRONTIER;	
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			if (!(err = AEResolve(&obj,kAEIDoMinimum,&token)))
			{
				switch (token.descriptorType)
				{
					case cEuMailbox:
						AEGetDescData(&token,&spec,sizeof(spec));
						err = GetMailbox(&spec,True);
						break;
					case cEuMessage:
						AEGetDescData(&token,&messT,sizeof(messT));
						err = OpenMessByToken(&messT,True,nil);
						break;
					case cWindow:
						AEGetDescData(&token,&win,sizeof(win));
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				AEDisposeDesc(&token);
			}
		}
		AEDisposeDesc(&obj);
	}
	return(err);
}

/**********************************************************************
 * AEOpenWindow - open a window for the AE routines
 **********************************************************************/
OSErr AEOpenWindow(WindowPtr theWindow)
{
	WindowPtr oldWin = FrontWindow_();

	if ((long)theWindow>WIN_BAR_ITEM)
	{
		ShowMyWindow(theWindow);
		UserSelectWindow(theWindow);
		return(noErr);
	}
	else
	{
		DoMenu(oldWin,(WINDOW_MENU<<16)|(short)theWindow,0);
		return(oldWin==FrontWindow_());
	}
}

/************************************************************************
 * HandleAEClose - handle a close event
 ************************************************************************/
pascal OSErr HandleAEClose(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	AEDesc obj, token;
	FSSpec spec;
	TOCHandle tocH;
	MessToken messT;
	WindowPtr	win;
	
	FRONTIER;	
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			if (!(err = AEResolve(&obj,kAEIDoMinimum,&token)))
			{
				switch (token.descriptorType)
				{
					case cEuMailbox:
						AEGetDescData(&token,&spec,sizeof(spec));
						if (tocH=FindTOC(&spec)) err = !CloseMyWindow(GetMyWindowWindowPtr((*tocH)->win));
						break;
					case cEuMessage:
						AEGetDescData(&token,&messT,sizeof(messT));
						err = CloseMessByToken(&messT);
						break;
					case cWindow:
						AEGetDescData(&token,&win,sizeof(win));
						if ((uLong)win>WIN_BAR_ITEM)
							err = !CloseMyWindow(win);
						else err = noErr;
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				AEDisposeDesc(&token);
			}
		}
		AEDisposeDesc(&obj);
	}
	return(err);
}

#pragma segment AE2

/************************************************************************
 * HandleAEDelete - handle a delete event
 ************************************************************************/
pascal OSErr HandleAEDelete(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	AEDesc obj, token;
	FilterToken filtToken;
	PersHandle	pers;
	NickToken	nick;
	NickFileToken	nickFile;
	Handle	hHelper;
	
	FRONTIER;	
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			if (!(err = AEResolve(&obj,kAEIDoMinimum,&token)))
			{
				switch (token.descriptorType)
				{
					case cEuFilter:
						AEGetDescData(&token,&filtToken,sizeof(filtToken));
						err = AEDeleteFilter(&filtToken);
						break;
					case cEuPersonality:
						if (HasFeature (featureMultiplePersonalities))
						{
							AEGetDescData(&token,&pers,sizeof(pers));
							err = PersDelete(pers);
						}
						break;
					case cEuNickname:
						AEGetDescData(&token,&nick,sizeof(nick));
						err = AEDeleteNickname(&nick);
						break;
					case cEuNickFile:
						AEGetDescData(&token,&nickFile,sizeof(nickFile));
						err = AEDeleteNickFile(&nickFile);
						break;
					case cEuHelper:
						AEGetDescData(&token,&hHelper,sizeof(hHelper));
						RemoveResource(hHelper);
						err = ResError();
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				AEDisposeDesc(&token);
			}
		}
		AEDisposeDesc(&obj);
	}
	return(err);
}

/************************************************************************
 * HandleAEGetData - get data out of an object
 ************************************************************************/
pascal OSErr HandleAEGetData(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDesc obj, token;
	PropToken	property;
	
	FRONTIER;	
	NullADList(&obj,&token,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cProperty:
						AEGetDescData(&token,&property,sizeof(property));
						switch (property.tokenClass)
						{
							case cEuNickFile:
								err = GetNickFileProperty(&token,reply,refCon);
								break;
							case cEuNickname:
								err = GetNickProperty(&token,reply,refCon);
								break;
							case cEuMailfolder:
								err = GetMailfolderProperty(&token,reply,refCon);
								break;
							case cEuMailbox:
								err = GetMailboxProperty(&token,reply,refCon);
								break;
							case cEuMessage:
								err = GetMessProperty(&token,reply,refCon);
								break;
							case cEuPersonality:
								err = GetPersProperty(&token,reply,refCon);
								break;
							case cWindow:
								err = GetWinProperty(&token,reply,refCon);
								break;
							case typeNull:
								err = GetApplicationProperty(&token,reply,refCon);
								break;
							case cEuFilter:
								err = GetFilterProperty(&token,reply,refCon);
								break;
							case cEuFilterTerm:
								err = GetTermProperty(&token,reply,refCon);
								break;
							default:
								err = errAEEventNotHandled;
								break;
						}
						break;
					case cEuMessage:
						err = GetMessData(&token,reply,refCon);
						break;
					case cEuField:
						err = GetField(&token,reply,refCon);
						break;
					case cEuAttachment:
						err = GetAttachment(&token,reply,refCon);
						break;
					case cEuTEInWin:
					case cEuWTEText:
						err = GetSomeText(&token,reply,refCon);
						break;
					case cEuPreference:
						err = GetAEPref(&token,reply,refCon);
						break;
					case cEuHelper:
						err = GetAEHelper(&token,reply,refCon);
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				AEDisposeDesc(&token);
			}
		}
		AEDisposeDesc(&obj);
	}
	return(err);
}

/************************************************************************
 * HandleAEExists - does an object exist?
 ************************************************************************/
pascal OSErr HandleAEExists(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	AEDesc obj, token;
	WindowPtr	win;
	
	FRONTIER;	
	NullADList(&obj,&token,nil);
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	{
		if (!(err=GotAERequired(event)))
		{
			err = AEResolve(&obj,kAEIDoMinimum,&token);
			if (!err)
			{
				switch (token.descriptorType)
				{
					case cEuNickFile:
						err = !AENickFileExists(&token);
						break;
					case cEuNickname:
						err = !AENicknameExists(&token);
						break;
					case cEuMailfolder:
						err = !FolderExists(&token);
						break;
					case cEuMailbox:
						err = !BoxExists(&token);
						break;
					case cEuMessage:
						err = !MessExists(&token);
						break;
					case cEuField:
						err = !FieldExists(&token);
						break;
					case cEuAttachment:
						err = !AttachmentExists(&token);
						break;
					case cEuPreference:
						err = !PrefExists(&token);
						break;
					case cEuPersonality:
						err = noErr;	// if we found it, it exists!
						break;
					case 'csel':
					case cEuFilter:
						err = noErr;
						break;
					case cWindow:
						AEGetDescData(&token,&win,sizeof(win));
						err = ((uLong)win>WIN_BAR_ITEM) ? noErr : errAENoSuchObject;
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				AEDisposeDesc(&token);
			}
		}
		AEDisposeDesc(&obj);
	}
	AEPutBool(reply,keyAEResult,err==0);
	return(noErr);
}

/**********************************************************************
 * FolderExists - is a folder currently part of the Eudora hierarchy
 **********************************************************************/
Boolean FolderExists(AEDescPtr token)
{
	FSSpec spec;
	
	AEGetDescData(token,&spec,sizeof(spec));
	return(VD2MenuId(spec.vRefNum,spec.parID)!=-1);
}

/**********************************************************************
 * BoxExists - does a mailbox exist?
 **********************************************************************/
Boolean BoxExists(AEDescPtr token)
{
	FSSpec spec;
	
	AEGetDescData(token,&spec,sizeof(spec));
	return(IsMailbox(&spec));
}

/**********************************************************************
 * MessExists - does a message exist?
 **********************************************************************/
Boolean MessExists(AEDescPtr token)
{
	MessToken messT;
	TOCHandle tocH;
	
	AEGetDescData(token,&messT,sizeof(messT));
	return (!GetTOCByFSS(&messT.spec,&tocH) && (*tocH)->count>messT.index);
}

/**********************************************************************
 * FieldExists - is a field in a message?
 **********************************************************************/
Boolean FieldExists(AEDescPtr token)
{
	FieldToken ft;
	MessHandle messH;
	short index;
	Boolean exists=False;
	long size;
	Handle text;
	OSErr err;
	
	AEGetDescData(token,&ft,sizeof(ft));
	if (ft.isNick) return(AENickFieldExists(token));
	
	/*
	 * grab message
	 */
	if (!(err=OpenMessByToken(&ft.messT,False,&messH)))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		if (!*ft.name) exists = True;	/* body always exists */
		
		else if ((*(*messH)->tocH)->which==OUT)
		{
			if (ft.name[*ft.name]!=':') PCatC(ft.name,':');
			index = FindSTRNIndex(HEADER_STRN,ft.name);
			exists = index && index<ATTACH_HEAD;
		}
		else
		{
			PeteGetRawText(TheBody,&text);
			size = GetHandleSize(text);
			exists = nil!=FindHeaderString(LDRef(text),ft.name,&size,False);
			UL(text);
		}

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(exists);
}

/**********************************************************************
 * PrefExists - does a preference exist?
 **********************************************************************/
Boolean PrefExists(AEDescPtr token)
{
	short	pref;
	
	AEGetDescData(token,&pref,sizeof(pref));
	return(pref < PREF_STRN_LIMIT);
}

/**********************************************************************
 * AttachmentExists - does an attachment exist?
 **********************************************************************/
Boolean AttachmentExists(AEDescPtr token)
{
	AttachmentToken	at;
	FSSpec spec;
	MessHandle messH;
	Boolean	exists = false;
	
	AEGetDescData(token,&at,sizeof(at));
	if (!OpenMessByToken(&at.messT,false,&messH))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);
		if (GetAttachmentFromMsg(messH,at.index,&spec))
		if (!FSpExists(&spec))
			exists = true;

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return exists;
}

/************************************************************************
 * HandleAESetData - Set data of an object
 ************************************************************************/
pascal OSErr HandleAESetData(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(reply,refCon)
	OSErr err;
	AEDesc obj, token, data;
	PropToken	property;
	
	FRONTIER;	
	NullADList(&obj,&token,&data,nil);
	
	if (!(err = GetMessageDO(event,keyDirectObject,&obj)))
	if (!(err = AEGetParamDesc(event,keyAEData,typeWildCard,&data)))
	if (!(err = GotAERequired(event)))
	if (!(err = AEResolve(&obj,kAEIDoMinimum,&token)))
	{
		switch (token.descriptorType)
		{
			case cProperty:
				AEGetDescData(&token,&property,sizeof(property));
				switch (property.tokenClass)
				{
					case cEuNickname:
						err = SetNickProperty(&token,&data);
						break;
					case cEuMessage:
						err = SetMessProperty(&token,&data);
						break;
					case cWindow:
						err = SetWinProperty(&token,&data);
						break;
					case cEuFilter:
						err = SetFilterProperty(&token,&data);
						break;
					case cEuFilterTerm:
						err = SetTermProperty(&token,&data);
						break;
					case cEuPersonality:
						err = SetPersProperty(&token,&data);
						break;
					case cEuHelper:
						err = SetURLHelperProperty(&token,&data);
						break;
					case typeNull:
						err = SetApplicationProperty(&token,&data);
						break;
					default:
						err = errAEEventNotHandled;
						break;
				}
				break;
			case cEuWTEText:
				err = SetSomeText(&token,&data);
				break;
			case cEuField:
				err = SetField(&token,&data);
				break;
			case cEuPreference:
				err = SetPreference(&token,&data);
				break;
			default:
				err = errAEEventNotHandled;
				break;
		}
	}
	DisposeADList(&obj,&token,&data,nil);
	return(err);
}
/************************************************************************
 * HandleAECount - count stuff
 ************************************************************************/
pascal OSErr HandleAECount(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err, objErr;
	AEDesc obj, token, countType;
	long count;
	
	FRONTIER;	
	NullADList(&obj,&token,&countType,nil);
	
	objErr = GetMessageDO(event,keyDirectObject,&obj);
	if (!(err = AEGetParamDesc(event,keyAEObjectClass,typeType,&countType)))
	if (!(err = GotAERequired(event)))
	if (objErr || !(err = AEResolve(&obj,kAEIDoMinimum,&token)))
	{
		long	countTypeValue;
		
		AEGetDescData(&countType,&countTypeValue,sizeof(countTypeValue));
		err = AECountStuff(countTypeValue,token.descriptorType,&token,&count);
		if (!err) AEPutLong(reply,keyAEResult,count);
	}
	DisposeADList(&obj,&token,&countType,nil);
	return(err);
}

/************************************************************************
 * HandleAEMove - move something (only messages supported now)
 ************************************************************************/
pascal OSErr HandleAEMove(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEDesc moveMe, moveMeToken;	/* object to be moved */
	AEDesc moveTo, moveToToken; /* object to move into */
	AEDesc moveToPosition;			/* position to move to */
	AERecord moveToRec;					/* insertionloc for move to */
	AEDesc newMess;							/* object specifier for moved object */
	TOCHandle fromTocH, toTocH;
	MessHandle messH;
	FSSpec spec;
	MessToken messT;
	
	FRONTIER;	
	NullADList(&moveMe,&moveMeToken,&moveTo,&moveToToken,&moveToPosition,&moveToRec,&newMess,nil);
	
	if (!(err = GetMessageDO(event,keyDirectObject,&moveMe)))
	if (!(err = AEGetParamDesc(event,keyAEInsertHere,typeAERecord,&moveToRec)))
	if (!(err = GotAERequired(event)))
	if (!(err = AEGetKeyDesc(&moveToRec,keyAEObject,typeObjectSpecifier,&moveTo)))
	if (!(err = AEGetKeyDesc(&moveToRec,keyAEPosition,typeEnumeration,&moveToPosition)))
	if (!(err = AEResolve(&moveMe,kAEIDoMinimum,&moveMeToken)))
	{
		/*
		 * we support only one kind of move; of a message to the end of a mailbox
		 */
		uLong	moveToValue;
		
		AEGetDescData(&moveToPosition,&moveToValue,sizeof(moveToValue));
		if (moveToValue != kAEEnd || moveMeToken.descriptorType != cEuMessage)
			err = errAEEventNotHandled;
		else if (!(err = AEResolve(&moveTo,kAEIDoMinimum,&moveToToken)))
		{
			if (moveToToken.descriptorType != cEuMailbox)
				err = errAEEventNotHandled;
			else
			{
				/*
				 * WOW.  FINALLY.  We know what to do
				 */
				AEGetDescData(&moveMeToken,&messT,sizeof(messT));
				if (!(err=OpenMessByToken(&messT,False,&messH)))
				{
					fromTocH = (*messH)->tocH;
					AEGetDescData(&moveToToken,&spec,sizeof(spec));
					if (!(err = GetTOCByFSS(&spec,&toTocH)))
					{
						if (!(err = MoveMessageLo(fromTocH,(*messH)->sumNum,&spec,refCon==kAEClone,false,true)) &&
								!(err = MessObjFromBoxDTOC(&moveTo,toTocH,(*toTocH)->count-1,&newMess)))
							err = AEPutParamDesc(reply,keyAEResult,&newMess);
					}
				}
			}
		}
	}
	
	DisposeADList(&moveMe,&moveMeToken,&moveTo,&moveToToken,&moveToPosition,&moveToRec,&newMess,nil);
	return(err);
}

/************************************************************************
 * HandleAECreate - create a new element
 ************************************************************************/
pascal OSErr HandleAECreate(AppleEvent *event,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	AEDesc inContainer;	/* container in which to create */
	AERecord insertLoc;
	OSType junk;
	DescType theClass;
	
	if (ModalWindow && !AreAllModalsPlugwindows()) return(errAEEventNotHandled);

	FRONTIER;	
	NullADList(&inContainer,&insertLoc,nil);
	
	AEGetParamDesc(event,keyAEInsertHere,typeAERecord,&insertLoc);
	if (!(err = AEGetParamPtr_(event,keyAEObjectClass,typeType,&junk,&theClass,sizeof(DescType),&junk)))
	{
		switch (theClass)
		{
			case cEuNickname:
				if (!(err = AEGetKeyDesc(&insertLoc,keyAEObject,typeObjectSpecifier,&inContainer)))
					err = AECreateNick(theClass,&inContainer,event,reply);
				break;
			case cEuNickFile:
				//if (!(err = AEGetKeyDesc(&insertLoc,keyAEObject,typeObjectSpecifier,&inContainer)))
					err = AECreateNickFile(theClass,&inContainer,event,reply);
				break;
			case cEuMessage:
				if (!(err = AEGetKeyDesc(&insertLoc,keyAEObject,typeObjectSpecifier,&inContainer)))
					err = AECreateMessage(theClass,&inContainer,event,reply);
				break;
			case cEuFilter:
				err = AECreateFilter(theClass,&insertLoc,event,reply);
				break;
			case cEuHelper:
				err = AECreateURLHelper(theClass,&insertLoc,event,reply);
				break;
			case cEuPersonality:
				if (HasFeature (featureMultiplePersonalities)) {
					UseFeature (featureMultiplePersonalities);
					err = AECreatePersonality(theClass,&insertLoc,event,reply);
				}
				break;
		}
	}
	DisposeADList(&inContainer,&insertLoc,nil);
	return(err);
}

/**********************************************************************
 * AECreateMessage - create a message with AE's
 **********************************************************************/
OSErr AECreateMessage(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	OSErr err;
	AEDesc messDesc, dataDesc,textAD,inToken;
	AEDescPtr useMe;
	OSType junk;
	FSSpec spec;
	MyWindowPtr win;
	MessHandle messH;
	Str31 s;
	TOCHandle tocH;
	Boolean	invisible;
	Size actSize;
	DescType actType;

	NullADList(&messDesc,&dataDesc,&textAD,&inToken,nil);
	if (!(err = AEResolve(inContainer,kAEIDoMinimum,&inToken)))
	{
		AEGetDescData(&inToken,&spec,sizeof(spec));
		if (inToken.descriptorType!=cEuMailbox)
			err = errAEEventNotHandled;
		else if (SameVRef(spec.vRefNum,MailRoot.vRef) && MailRoot.dirId==spec.parID && StringSame(spec.name,GetRString(s,OUT)))
		{
			/*
			 * Out mailbox
			 */
			//	Check for Invisible key
			err = AEGetParamPtr_(event,keyEuInvisible,typeBoolean,&actType,&invisible,sizeof(invisible),&actSize);
			if (err==errAEDescNotFound) invisible = false;
			else if (err) return(err);

			if (!(err = GotAERequired(event)))
			{
				if (!(win=DoComposeNew(0)))
					err = errAEEventNotHandled;
				else
				{
					messH = Win2MessH(win);
#ifdef TWO
					ApplyDefaultStationery(win,True,True);
					UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
#endif
					if (!invisible)
						ShowMyWindow(GetMyWindowWindowPtr (win));
					if (!(err = MessObjFromBoxDTOC(inContainer,(*messH)->tocH,(*messH)->sumNum,&messDesc)))
					{
						err = AEPutParamDesc(reply,keyAEResult,&messDesc);
					}
				}
			}
		}
		else
		{
			/*
			 * other mailbox
			 */
			if (!(err=AEGetParamDesc(event,keyAEData,typeWildCard,&dataDesc)))
			if (!(err = GotAERequired(event)))
			{
				if (dataDesc.descriptorType == typeChar)
					useMe = &dataDesc;
				else
				{
					err=AECoerceDesc(&dataDesc,typeChar,&textAD);
					useMe = &textAD;
				}
				if (!err)
				{
					/* hey, hey, we have some text.... */
					if (!(tocH=TOCBySpec(&spec))) err = errAENoSuchObject;
					else
					{
						Handle	hText;
						
						if (!AEGetDescDataHandle(useMe,&hText))
						{
							if (!(err = SaveTextAsMessage(nil,hText,tocH,&junk)))
							{
								Rehash(tocH,(*tocH)->count-1,hText);
								if (!(err=MessObjFromTOC(tocH,(*tocH)->count-1,&messDesc)))
								{
									err = AEPutParamDesc(reply,keyAEResult,&messDesc);
								}
							}
							AEDisposeDescDataHandle(hText);
						}
					}
				}
			}
		}
	}
	DisposeADList(&messDesc,&dataDesc,&textAD,&inToken,nil);
	return(err);
}

/************************************************************************
 * OpenMessByToken - open a message, given a token
 ************************************************************************/
OSErr OpenMessByToken(MessTokenPtr tokenP,Boolean showIt,MessHandle *messHP)
{
	MessToken token = *tokenP;	/* make a local copy */
	OSErr err;
	TOCHandle tocH;
	MyWindowPtr win;
	
	if (!(err = GetTOCByFSS(&token.spec,&tocH)))
	{
		if (!(win=GetAMessage(tocH,token.index,nil,nil,showIt))) err = 1;
	}
	if (messHP) *messHP = (MessHandle) GetMyWindowPrivateData (win);
	return(err);
}

/************************************************************************
 * CloseMessByToken - close a message, given a token
 ************************************************************************/
OSErr CloseMessByToken(MessTokenPtr tokenP)
{
	MessToken token = *tokenP;	/* make a local copy */
	OSErr err=0;
	TOCHandle tocH;
	MessHandle messH;
	
	if (tocH = FindTOC(&token.spec))
		if (messH = (*tocH)->sums[token.index].messH)
			err = !CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
	return(err);
}

/************************************************************************
 * MyTextRange - handle a range descriptor
 ************************************************************************/
OSErr MyTextRange(long len,AEDescPtr keyData,TERTPtr token)
{
	AEDesc rangeRec, obj1, obj2, tert1, tert2;
	short err;
	TERangeToken	TERange;
	
	NullADList(&obj1, &obj2, &tert1, &tert2, &rangeRec, nil);
	
	/*
	 * suck out the object specifiers for the beginning and end of the range,
	 * and resolve them
	 */
	if (err = AECoerceDesc(keyData,typeAERecord,&rangeRec)) goto done;
	if (err = AEGetKeyDesc(&rangeRec,keyAERangeStart,typeObjectSpecifier,&obj1)) goto done;
	if (err = AEGetKeyDesc(&rangeRec,keyAERangeStop,typeObjectSpecifier,&obj2)) goto done;
	if (err = AEResolve(&obj1,kAEIDoMinimum,&tert1)) goto done;
	if (err = AEResolve(&obj2,kAEIDoMinimum,&tert2)) goto done;
	
	/*
	 * pretty simple now
	 */
	AEGetDescData(&tert2,&TERange,sizeof(TERange));
	token->stop = TERange.stop;
	AEGetDescData(&tert1,&TERange,sizeof(TERange));
	token->start = TERange.start;
	
	err = BadRange(token->start,token->stop,len);
	
	/*
	 * the worst thing about AE's is all the mess you have to clean up
	 * afterwards.
	 */
done:
	DisposeADList(&obj1, &obj2, &tert1, &tert2, &rangeRec, nil);
	return(err);
}

/************************************************************************
 * FindWTChar - find a char in a text block
 ************************************************************************/
OSErr FindWTChar(UPtr text,long spot,TERTPtr token)
{
#pragma unused(text)
	long newSpot;
	long len = token->stop;
	
	if (spot > 0)
		newSpot = token->start + spot - 1;
	else
		newSpot = token->stop + spot;
		
	token->start = newSpot;
	token->stop = newSpot+1;
	
	return(BadRange(token->start,token->stop,len));
}

/************************************************************************
 * BadRange - is a range unacceptable?
 ************************************************************************/
OSErr BadRange(long start, long end, long len)
{
	if (start<0 || start>=len)
		return(errAEImpossibleRange);
	if (end<0 || end>len)
		return(errAEImpossibleRange);
	if (end<=start)
		return(errAEImpossibleRange);
	return(noErr);
}

/************************************************************************
 * FindWTWord - find a range of words
 ************************************************************************/
OSErr FindWTWord(UPtr text,long spot,TERTPtr token)
{
	long nStart;
	long len = token->stop;
	
	nStart = StartOfWord(text+token->start,token->stop-token->start,spot);
	token->stop = EndOfWord(text+token->start,token->stop-token->start,spot);
	token->start = nStart;
	return(BadRange(token->start,token->stop,len));
}

/************************************************************************
 * StartOfWord - find the beginning of a word
 ************************************************************************/
long StartOfWord(UPtr text, long len, long word)
{
	long wStart, wEnd;

	if (word<0)	BackWord(text,len,-word,&wStart,&wEnd);
	else ForWord(text,len,word,&wStart,&wEnd);
	
	return(wStart);
}

/************************************************************************
 * EndOfWord - find the end of a word
 ************************************************************************/
long EndOfWord(UPtr text, long len, long word)
{
	long wStart, wEnd;

	if (word<0)	BackWord(text,len,-word,&wStart,&wEnd);
	else ForWord(text,len,word,&wStart,&wEnd);
	
	return(wEnd);
}

/************************************************************************
 * BackWord - find a word in some text, referenced from the end
 ************************************************************************/
void BackWord(UPtr text, long len, long word, long *begin, long *end)
{
	UPtr spot = text + len-1;
	
	while (word-->0 && spot>text)
	{
		/*
		 * back over delimiters
		 */
		while (!IsWordChar[*spot] && spot>text) spot--;
		
		/*
		 * spot now points at the end of the word
		 */
		*end = spot-text+1;
		
		/*
		 * back over the word (vroom!  vroom!)
		 */
		while (IsWordChar[*spot] && spot>=text) spot--;
		
		/*
		 * pointing just before start of word
		 */
		*begin = spot-text+2;
	}
}

/************************************************************************
 * ForWord - find a word in some text, referenced from the end
 ************************************************************************/
void ForWord(UPtr text, long len, long word, long *begin, long *end)
{
	UPtr spot = text;
	UPtr stop = text+len;
	
	while (word-->0 && spot<stop)
	{
		/*
		 * skip delimiters
		 */
		while (!IsWordChar[*spot] && spot<stop) spot++;
		
		/*
		 * spot now points at the beginning of the word
		 */
		*begin = spot-text+1;
		
		/*
		 * run over the word (vroom!  vroom!)
		 */
		while (IsWordChar[*spot] && spot<=stop) spot++;
		
		/*
		 * spot pointing one past end of word
		 */
		*end = spot-text;
	}
}

/************************************************************************
 * FindFolderByIndex - find the indexed mail folder
 ************************************************************************/
OSErr FindFolderByIndex(AEDescPtr inDesc,long index,VDIdPtr vdid)
{
	OSErr err = errAENoSuchObject;
	VDId folder;
	short menuId;
	short i, n;
	MenuHandle mh;
	
	if (inDesc)
		AEGetDescData(inDesc,&folder,sizeof(folder));
	else
	{
		folder.vRef = MailRoot.vRef;
		folder.dirId = MailRoot.dirId;
	}
	menuId = VD2MenuId(folder.vRef,folder.dirId);

	if (menuId>=0)
	{
		/*
		 * gather info
		 */
		mh = GetMHandle(menuId);
		n = CountMenuItems(mh);
		for (i=1;i<=n;i++)
		{
			if (HasSubmenu(mh,i))
			{
				index--;
				if (index<=0)
				{
					menuId = SubmenuId(mh,i);
					Menu2VD(GetMHandle(menuId),&vdid->vRef,&vdid->dirId);
					err = noErr;
					break;
				}
			}
		}
	}
	return(err);
}

/************************************************************************
 * FindBoxByIndex - find the indexed mailbox
 ************************************************************************/
OSErr FindBoxByIndex(short vRef, long dirId,long index,FSSpecPtr spec)
{
	OSErr err = errAENoSuchObject;
	short menuId = VD2MenuId(vRef,dirId);
	short i, n;
	MenuHandle mh;
	
	// Some scripts rely on Trash being 3rd, even though Junk is now 3rd and
	// Trash is 4th.  So we play a little game...
	ASSERT(MAILBOX_JUNK_ITEM==3);
	ASSERT(MAILBOX_TRASH_ITEM==4);
	if (vRef==MailRoot.vRef && dirId==MailRoot.dirId && (index==3 || index==4))
		index = 7-index;
	
	if (menuId>=0)
	{
		/*
		 * gather info
		 */
		mh = GetMHandle(menuId);
		n = CountMenuItems(mh);
		for (i=1;i<=n;i++)
		{
			if (!HasSubmenu(mh,i) &&
					(menuId==MAILBOX_MENU ? (i<MAILBOX_BAR1_ITEM || i>=MAILBOX_FIRST_USER_ITEM)
																 : (i>=MAILBOX_FIRST_USER_ITEM-MAILBOX_BAR1_ITEM)))
			{
				index--;
				if (index<=0)
				{
					GetTransferParams(menuId,i,spec,nil);
					err = noErr;
					break;
				}
			}
		}
	}
	return(err);
}

/**********************************************************************
 * CountWindows - count the number of windows
 **********************************************************************/
short CountWindows(void)
{
	short i;
	WindowPtr theWindow;
	
	i = 0;
	for (theWindow = FrontWindow_(); theWindow; theWindow = GetNextWindow (theWindow)) i++;
	return(i);
}


/************************************************************************
 * CountMailfolderElements - count stuff in mailboxes
 ************************************************************************/
OSErr CountMailfolderElements(DescType countClass,AEDescPtr countIn,long *count)
{
	OSErr err = errAENoSuchObject;
	VDId folder;
	short menuId;
	short itemCount;
	short subCount;
	short i;
	MenuHandle mh;
	
	AEGetDescData(countIn,&folder,sizeof(folder));
	menuId = VD2MenuId(folder.vRef,folder.dirId);
	if (menuId>=0)
	{
		/*
		 * gather info
		 */
		mh = GetMHandle(menuId);
		itemCount = CountMenuItems(mh);
		subCount = 0;
		for (i=1;i<=itemCount;i++) if (HasSubmenu(mh,i)) subCount++;
		
		if (countClass == cEuMailfolder)
		{
			*count = subCount;
			err = noErr;
		}
		else if (countClass == cEuMailbox)
		{
			if (menuId==MAILBOX_MENU) itemCount--;	/* separator bar */
			itemCount -= subCount;								 	/* submenus */
			itemCount -= MAILBOX_FIRST_USER_ITEM-MAILBOX_BAR1_ITEM-1;	/* New... and Other... */
			*count = itemCount;
			err = noErr;
		}
	}
	return(err);
}

/************************************************************************
 * CountMailboxElements - count stuff in mailboxes
 ************************************************************************/
OSErr CountMailboxElements(DescType countClass,AEDescPtr countIn,long *count)
{
	OSErr err = errAENoSuchObject;
	TOCHandle tocH;
	FSSpec spec;
	
	AEGetDescData(countIn,&spec,sizeof(spec));
	switch(countClass)
	{
		case cEuMessage:
			if (!(err=GetTOCByFSS(&spec,&tocH)))
				*count = (*tocH)->count;
			break;
	}
	return(err);
}

/************************************************************************
 * CountMessageElements - count stuff in messages
 ************************************************************************/
OSErr CountMessageElements(DescType countClass,AEDescPtr countIn,long *count)
{
	OSErr err;
	FSSpec	spec;
	MessHandle messH;
	short	i;
	MessToken messT;
	
	/*
	 * grab message
	 */
	AEGetDescData(countIn,&messT,sizeof(messT));
	err = OpenMessByToken(&messT,false,&messH);
	if (!err)
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		switch(countClass)
		{
			case cEuAttachment:
				for (i=1;GetAttachmentFromMsg(messH,i,&spec);i++);
				*count = i-1;
				break;
			default:
				err = errAENoSuchObject;
		}

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);
}

/************************************************************************
 * GetAEPref - get the value of a pref
 ************************************************************************/
OSErr GetAEPref(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	PrefToken pt;
	PersHandle oldCur;
	Str255 s;
	OSErr err;
	
	AEGetDescData(token,&pt,sizeof(pt));
	oldCur = CurPers;
	CurPers = pt.personality;
	if (pt.pref==PREF_PASS_TEXT && pt.personality)
		//	Handle password a little differently
		PCopy(s,(*pt.personality)->password);
	else
		GetPref(s,pt.pref);
	err = AEPutPStr(reply,keyAEResult,s);
	CurPers = oldCur;
	return(err);
}

/************************************************************************
 * GetAEStationeryIndex - get the name of a stationery 
 ************************************************************************/
OSErr GetAEStationeryIndex(AppleEvent *ae,short *index)
{
	AEDesc desc;
	OSErr err = noErr;
	Str255 s;
	
	NullADList(&desc,nil);
	
	err = AEGetParamDesc(ae,keyEuStationeryName,typeChar,&desc);
	if (err) 
	{
		*index = 0;
		return noErr;
	}
	
	if (*GetAEPStr(s,&desc)) *index = FindItemByName(GetMHandle(REPLY_WITH_HIER_MENU),s);
	
	DisposeADList(&desc,nil);
	
	if (!err && !*index) err = fnfErr;
	
	return err;
}

/************************************************************************
 * GetAEHelper - get a helper app
 ************************************************************************/
OSErr GetAEHelper(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	Handle res = nil;
	OSErr err;
	
	AEGetDescData(token,&res,sizeof(res));
	if (res)
	{
		err = AEPutParamPtr(reply,keyAEResult,typeAlias,LDRef(res),GetHandleSize(res));
		UL(res);
	}
	else err = errAENoSuchObject;
	
	return(err);
}

/************************************************************************
 * CountURLHelpers - count stuff in mailboxes
 ************************************************************************/
short CountURLHelpers(void)
{
	return(CountResources(URLMAP_TYPE));
}

/************************************************************************
 * GetMailboxProperty - get a property of a mailbox
 ************************************************************************/
OSErr GetMailboxProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	DescType property;
	FSSpec spec;
	TOCHandle tocH;
	short err;
	Str31 suffix;
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;
	property = (*(PropTokenHandle)hProperty)->propertyId;
	spec = *(FSSpec *)((*hProperty)+sizeof(PropToken));
	if (property==pName)
		err = AEPutPStr(reply,keyAEResult,spec.name);
	else if (!(err=GetTOCByFSS(&spec,&tocH)))
	{
		switch (property)
		{
			case pEuMailboxType:
				err = AEPutLong(reply,keyAEResult,(*tocH)->which);
				break;
			case pEuMailboxUnread:
			case pEuMailboxRecentUnread:
				err = AEPutLong(reply,keyAEResult,TOCUnreadCount(tocH,property==pEuMailboxRecentUnread));
				break;
			case pEuWasteSpace:
				err = AEPutLong(reply,keyAEResult,(*tocH)->totalK-(*tocH)->usedK);
				break;
			case pEuNeededSpace:
				err = AEPutLong(reply,keyAEResult,(*tocH)->usedK);
				break;
			case pEuFSS:
				err = MyAEPutAlias(reply,keyAEResult,&spec);
				break;
			case pEuTOCFSS:
				PCat(spec.name,GetRString(suffix,TOC_SUFFIX));
				err = MyAEPutAlias(reply,keyAEResult,&spec);
				break;
			case cWindow:
				if (IsWindowVisible (GetMyWindowWindowPtr ((*tocH)->win)))
					err = AddWinToEvent(reply,keyAEResult,(*tocH)->win);
				else
					err = errAENoSuchObject;
				break;
			default:
				err = errAENoSuchObject;
				break;
		}
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}


/**********************************************************************
 * GetWinProperty - get properties for windows
 **********************************************************************/
OSErr GetWinProperty(AEDescPtr token,AppleEvent *reply, long refCon)
{
	DescType property;
	WindowPtr	winWP;
	MyWindowPtr win;
	OSErr err=errAENoSuchObject;
	Boolean mine;
	Str255 title;
	short n;
	WindowPtr w;
	Rect r;	
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;
	property = (*(PropTokenHandle)hProperty)->propertyId;
	winWP = *(WindowPtr *)((*hProperty)+sizeof(PropToken));
	if ((long)winWP<WIN_BAR_ITEM) err = errAENoSuchObject;
	else
	{
		mine = IsMyWindow(winWP);
		switch (property)
		{
			case pBounds:
				PushGWorld();
				SetPort_(GetWindowPort(winWP));
				r = *GetPortBounds(GetWindowPort(winWP),&r);
				LocalToGlobal((Point*)&r);
				LocalToGlobal((Point*)&r + 1);
				err = AEPutRect(reply,keyAEResult,&r);
				PopGWorld();
				break;
			case pHasCloseBox:
				err = AEPutBool(reply,keyAEResult,mine);
				break;
			case pIsResizable:
			case pIsZoomable:
				if (win = GetWindowMyWindowPtr(winWP))
					err = AEPutBool(reply,keyAEResult,mine && !win->isRunt);
				break;
			case pIsZoomed:
			{
				Rect oldStd;
				Rect curState = CurState(winWP);
		
				GetWindowStandardState(winWP,&oldStd);
				err = AEPutBool(reply,keyAEResult,AboutSameRect(&curState,&oldStd));
				break;
			}
			case pName:
				GetWTitle(winWP,title);
				err = AEPutPStr(reply,keyAEResult,title);
				break;
			case pIndex:
				w = FrontWindow_();
				for (n=1;w!=winWP;n++,w=GetNextWindow(w));
				err = AEPutLong(reply,keyAEResult,n);
				break;
			case formUniqueID:
				if (win = GetWindowMyWindowPtr(winWP))
					err = AEPutLong(reply,keyAEResult,win->windex);
				break;
			case pIsModified:
				if (win = GetWindowMyWindowPtr(winWP))
					err = AEPutBool(reply,keyAEResult,win->isDirty);
				break;
			case 'ppos':
				PushGWorld();
				SetPort_(GetWindowPort(winWP));
				r = *GetPortBounds(GetWindowPort(winWP),&r);
				LocalToGlobal((Point*)&r);
				err = AEPutParamPtr(reply,keyAEResult,typeQDPoint,&r,sizeof(Point));
				PopGWorld();
				break;
			case pEuSelectedText:
				if (win = GetWindowMyWindowPtr(winWP))
					err = GetSelText(win,reply,refCon);
				break;
		}
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}

/**********************************************************************
 * SetWinProperty - set properties for windows
 **********************************************************************/
OSErr SetWinProperty(AEDescPtr token, AEDescPtr data)
{
	DescType property;
	WindowPtr winWP;
	MyWindowPtr win;
	OSErr err = noErr;
	short n;
	Rect r;
	Point pt;
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;	
	property = (*(PropTokenHandle)hProperty)->propertyId;
	winWP = *(WindowPtr *)((*hProperty)+sizeof(PropToken));
	win = GetWindowMyWindowPtr(winWP);
	if ((long)winWP<WIN_BAR_ITEM) err = errAENoSuchObject;
	else
	{
		PushGWorld();
		SetPort_(GetWindowPort(winWP));
		switch (property)
		{
			case pBounds:
				err = GetAERect(data,&r);
				if (IsMyWindow(winWP) && win && !err)
				{
					Rect oldContR = win->contR;
					if (utl_CouldDrag(winWP,&r,4,TitleBarHeight(winWP),LeftRimWidth(winWP)))
					{
						Rect	rPort;
						
						MoveWindow(winWP,r.left,r.top,False);
						SizeWindow(winWP,r.right-r.left,r.bottom-r.top,True);
						MyWindowDidResize(win,&oldContR);
						if (IsMyWindow(winWP)) win->saveSize = True;
						rPort = *GetPortBounds(GetWindowPort(winWP),&rPort);
						InvalWindowRect(winWP,&rPort);
					}
					else err = errAEEventNotHandled;
				}
				else err = errAEEventNotHandled;
				break;
			case pIsZoomed:
				pt.h = -1;
				ZoomMyWindow(winWP,pt,0);
				if (IsMyWindow(winWP) && win) win->saveSize = True;
				break;
			case pIndex:
				n = GetAELong(data);
				if (n<=1)
					SelectWindow_(winWP);
				else err = errAEEventNotHandled;
				break;
			case 'ppos':
				err = GetAEPoint(data,&pt);
				if (IsMyWindow(winWP) && win && !err)
				{
					Rect oldContR = win->contR;
					r = *GetPortBounds(GetWindowPort(winWP),&r);
					r.right = r.right-r.left+pt.h;
					r.bottom = r.bottom-r.top+pt.v;
					r.left = pt.h;
					r.top = pt.v;
					if (utl_CouldDrag(winWP,&r,4,TitleBarHeight(winWP),LeftRimWidth(winWP)))
					{
						MoveWindow(winWP,r.left,r.top,False);
						if (IsMyWindow(winWP)) win->saveSize = True;
					}
					else err = errAEEventNotHandled;
				}
				else err = errAEEventNotHandled;
				break;
			case pEuSelectedText:
				if (win)
					err = SetSelText(win,data);
				break;
		}
		PopGWorld();
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}

/************************************************************************
 * GetMailfolderProperty - get a property of a mailbox
 ************************************************************************/
OSErr GetMailfolderProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	DescType property;
	VDId vd;
	FSSpec spec;
	OSErr err;
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;
	property = (*(PropTokenHandle)hProperty)->propertyId;
	vd = *(VDId *)((*hProperty)+sizeof(PropToken));
	if (!(err = FSMakeFSSpec(vd.vRef,vd.dirId,"",&spec)))
	{
		if (property==pName)
			err = AEPutPStr(reply,keyAEResult,spec.name);
		else
			err = errAENoSuchObject;
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}

/************************************************************************
 * GetMessProperty - get a property of a message
 ************************************************************************/
OSErr GetMessProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	DescType property;
	MessToken messT;
	TOCHandle tocH;
	short err;
	MSumPtr sum;
	FSSpec spec;
	PersHandle pers, ppers;
	Handle text;
	Handle	hProperty;
	Str31 scratch;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;	
	property = (*(PropTokenHandle)hProperty)->propertyId;
	messT = *(MessTokenPtr)((*hProperty)+sizeof(PropToken));
	if (!(err=GetTOCByFSS(&messT.spec,&tocH)))
	{
		ppers = TS_TO_PPERS(tocH,messT.index);
		pers = TS_TO_PERS(tocH,messT.index);
		sum = &(*tocH)->sums[messT.index];	/* volatile; careful */
		switch (property)
		{
			case pEuWholeText:
				if ((*tocH)->imapTOC && !EnsureMsgDownloaded(tocH,messT.index,false))	/* we may need to fetch IMAP messages */
					err = errAEEventNotHandled;
				else
				{
					CacheMessage(tocH,messT.index);
					if (text=(*tocH)->sums[messT.index].cache)
					{
						HNoPurge(text);
						err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(text),GetHandleSize(text));
						UL(text);
						HPurge(text);
					}
					else
						err = errAEEventNotHandled;
				}
				break;
			case pEuTableId:
				err = AEPutLong(reply,keyAEResult,sum->tableId);
				break;
			case pEuPriority:
				err = AEPutLong(reply,keyAEResult,sum->priority);
				break;
			case formUniqueID:
				err = AEPutLong(reply,keyAEResult,sum->uidHash);
				break;
			case pEuStatus:
				err = AEPutEnum(reply,keyAEResult,sum->state|'euS\000');
				break;
			case pEuSize:
				err = AEPutLong(reply,keyAEResult,sum->length);
				break;
			case pEuDate:
				ComputeLocalDate(sum,scratch);
				err = AEPutPStr(reply,keyAEResult,scratch);
				break;
			case pEuGMTSecs:
				err = AEPutLongDate(reply,keyAEResult,sum->seconds);
				break;
			case pEuLocalSecs:
				err = AEPutLongDate(reply,keyAEResult,sum->seconds+ZoneSecs());
				break;
			case pEuSender:
				err = AEPutPStr(reply,keyAEResult,sum->from);
				break;
			case pEuSubject:
				err = AEPutPStr(reply,keyAEResult,sum->subj);
				break;
			case pEuOutgoing:
				err = AEPutBool(reply,keyAEResult,(*tocH)->which==OUT);
				break;
			case pEuShowAll:
				err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_SHOW_ALL));
				break;
			case pEuPencil:
				err = AEPutBool(reply,keyAEResult,0!=(sum->opts&OPT_WRITE));
				break;
			case cWindow:
				if (sum->messH && IsWindowVisible (GetMyWindowWindowPtr((*sum->messH)->win)))
					err = AddWinToEvent(reply,keyAEResult,(*sum->messH)->win);
				else
					err = errAENoSuchObject;
				break;
			case cEuMailbox:
				{
					AEDesc boxDesc;
					NullADList(&boxDesc,nil);
					if (!(err=BoxObj(tocH,&boxDesc)))
						err = AEPutParamDesc(reply,keyAEResult,&boxDesc);
					DisposeADList(&boxDesc,nil);
				}
				break;
			case cEuPersonality:
				if (HasFeature (featureMultiplePersonalities)) {
					AEDesc persDesc;
					NullADList(&persDesc,nil);
					if (!(err=AEPersObj(pers,&persDesc)))
						err = AEPutParamDesc(reply,keyAEResult,&persDesc);
					if (pers!=PersList)
						UseFeature (featureMultiplePersonalities);
						
					DisposeADList(&persDesc,nil);
				}
				break;
			case pEuLabel:
				err = AEPutLong(reply,keyAEResult,GetSumColor(tocH,messT.index));
				break;
			case pEuOnServer:
				err = AEPutBool(reply,keyAEResult,IdIsOnPOPD(PERS_POPD_TYPE(ppers),POPD_ID,sum->uidHash));
				break;
			case pEuWillFetch:
				err = AEPutBool(reply,keyAEResult,IdIsOnPOPD(PERS_POPD_TYPE(ppers),FETCH_ID,sum->uidHash));
				break;
			case pEuWillDelete:
				err = AEPutBool(reply,keyAEResult,IdIsOnPOPD(PERS_POPD_TYPE(ppers),DELETE_ID,sum->uidHash));
				break;
			case pEuBody:
				{
					FieldToken ft;
					AEDesc theToken;
					ft.messT = messT;
					ft.isNick = false;
					*ft.name = 0;
					if (!(err = AECreateDesc(cEuField,&ft,sizeof(ft),&theToken)))
					{
						err = GetField(&theToken,reply,refCon);
						AEDisposeDesc(&theToken);
					}
				}
				break;
			default:
				if ((*tocH)->which==OUT)
					switch(property)
					{
						case pEuSignature:
							if (sum->sigId!=SIG_NONE && sum->sigId!=SIG_STANDARD && sum->sigId!=SIG_ALTERNATE)
							{
								SigSpec(&spec,sum->sigId);
								err = AEPutPStr(reply,keyAEResult,spec.name);
							}
							else
								err = AEPutEnum(reply,keyAEResult,'sig\000' | (sum->sigId!=SIG_NONE?sum->sigId+1:0));
							break;
						case pEuWrap:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_WRAP_OUT));
							break;
						case pEuKeepCopy:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_KEEP_COPY));
							break;
						case pEuHqxText:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_BX_TEXT));
							break;
						case pEuFakeTabs:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_TABS));
							break;
						case pEuMayQP:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_CAN_ENC));
							break;
						case pEuReturnReceipt:
							err = AEPutBool(reply,keyAEResult,0!=(sum->flags&FLAG_RR));
							break;
						case pEuAttDel:
							err = AEPutBool(reply,keyAEResult,0!=(sum->opts&OPT_ATT_DEL));
							break;
						case pEuAttachType:
							err = AEPutEnum(reply,keyAEResult,'atc\000' | AttachOptNumber(sum->flags));
							break;
						default:
							err = errAENoSuchObject;
							break;
					}
				else
					err = errAENoSuchObject;
				break;
		}
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}

/************************************************************************
 * SetApplicationProperty - set a property of the application
 ************************************************************************/
OSErr SetApplicationProperty(AEDescPtr token,AEDescPtr descP)
{
	MyWindowPtr	win;
	PropToken property;
	OSErr err;
	
	AEGetDescData(token,&property,sizeof(property));
	switch (property.propertyId)
	{
		case pEuSelectedText:
			if (win = GetWindowMyWindowPtr(FrontWindow_()))
				err = SetSelText(win,descP);
			break;
		default:
			err = errAEEventNotHandled;
			break;
	}
	return(err);
}

/************************************************************************
 * GetApplicationProperty - get a property of the application
 ************************************************************************/
OSErr GetApplicationProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	MyWindowPtr	win;
	PropToken property;
	OSErr err;
	Handle vers;
	
	AEGetDescData(token,&property,sizeof(property));
	switch (property.propertyId)
	{
		case pEuSelectedText:
			if (win = GetWindowMyWindowPtr(FrontWindow_()))
				err = GetSelText(win,reply,refCon);
			break;
		case pVersion:
			vers = GetResource(CREATOR,2);
			if (vers)
			{
				err = AEPutPStr(reply,keyAEResult,LDRef(vers));
				UL(vers);
				break;
			}
		default:
			err = errAENoSuchObject;
			break;
	}
	return(err);
}

/************************************************************************
 * SetMessProperty - Set a property of a message
 ************************************************************************/
OSErr SetMessProperty(AEDescPtr token,AEDescPtr descP)
{
	DescType property;
	MessToken messT;
	TOCHandle tocH;
	short err=noErr;
	Str255 str;
	short num;
	uLong uidHash;
	PersHandle pers;
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;	
	property = (*(PropTokenHandle)hProperty)->propertyId;
	messT = *(MessTokenPtr)((*hProperty)+sizeof(PropToken));
	if (!(err=GetTOCByFSS(&messT.spec,&tocH)))
	{
		switch (property)
		{
			case pEuDate:
			case pEuSize:
			case pEuOutgoing:
				err = errAEEventNotHandled;
				break;
			case pEuPriority:
				num = GetAELong(descP);
				SetPriority(tocH,messT.index,MAX(0,MIN(num,200)));
				break;
			case pEuTableId:
				num = GetAELong(descP);
				SetMessTable(tocH,messT.index,num);
				break;
			case pEuStatus:
				SetState(tocH,messT.index,0xff&GetAELong(descP));
				break;
			case pEuSender:
				GetAEPStr(str,descP);
				if (!*str)
					err = errAEEventNotHandled;
				else
					SetSender(tocH,messT.index,str);
				break;
			case pEuSubject:
				GetAEPStr(str,descP);
				if (!*str)
					err = errAEEventNotHandled;
				else
					SetSubject(tocH,messT.index,str);
				break;
			case pEuShowAll:
				if (GetAEBool(descP))
					(*tocH)->sums[messT.index].flags |= FLAG_SHOW_ALL;
				else
					(*tocH)->sums[messT.index].flags &= ~FLAG_SHOW_ALL;
				if ((*tocH)->sums[messT.index].messH)
				{
					MessIBarUpdate((*tocH)->sums[messT.index].messH);
					ReopenMessage((*(*tocH)->sums[messT.index].messH)->win);
				}
				break;
			case pEuPencil:
				if (GetAEBool(descP))
					(*tocH)->sums[messT.index].opts |= OPT_WRITE;
				else
					(*tocH)->sums[messT.index].opts &= ~OPT_WRITE;
				if ((*tocH)->sums[messT.index].messH)
					MessMakeEditable((*(*tocH)->sums[messT.index].messH)->win,MessOptIsSet((*tocH)->sums[messT.index].messH,OPT_WRITE));
				break;
			case cEuPersonality:
				if (HasFeature (featureMultiplePersonalities))
					err = AESetPers(tocH,messT.index,descP);
				break;

			case pEuLabel:
				num = GetAELong(descP);
				SetSumColor(tocH,messT.index,num);
				break;
			case pEuWillFetch:
				uidHash = (*tocH)->sums[messT.index].uidHash;
				pers = TS_TO_PPERS(tocH,messT.index);
				if (!IdIsOnPOPD(PERS_POPD_TYPE(pers),POPD_ID,uidHash))
					err = errAEEventNotHandled;
				else
				{
					if (GetAEBool(descP))
					{
						AddIdToPOPD(PERS_POPD_TYPE(pers),FETCH_ID,uidHash,False);
						if ((*tocH)->sums[messT.index].messH)
							InvalTopMargin((*(*tocH)->sums[messT.index].messH)->win);
					}
					else
					{
						RemIdFromPOPD(PERS_POPD_TYPE(pers),FETCH_ID,uidHash);
						if ((*tocH)->sums[messT.index].messH)
							InvalTopMargin((*(*tocH)->sums[messT.index].messH)->win);
					}
				}
				break;
			case pEuWillDelete:
				uidHash = (*tocH)->sums[messT.index].uidHash;
				pers = TS_TO_PPERS(tocH,messT.index);
				if (!IdIsOnPOPD(PERS_POPD_TYPE(pers),POPD_ID,uidHash))
					err = errAEEventNotHandled;
				else
				{
					if (GetAEBool(descP))
					{
						AddIdToPOPD(PERS_POPD_TYPE(pers),DELETE_ID,uidHash,False);
					}
					else
					{
						RemIdFromPOPD(PERS_POPD_TYPE(pers),DELETE_ID,uidHash);
					}
				}
				break;
			case pEuBody:
				{
					FieldToken ft;
					AEDesc theToken;
					
					ft.messT = messT;
					*ft.name = 0;
					ft.isNick = False;
					if (!(err = AECreateDesc(cEuField,&ft,sizeof(ft),&theToken)))
					{
						err = SetField(&theToken,descP);
						AEDisposeDesc(&theToken);
					}
				}
				break;
			default:
				if ((*tocH)->which==OUT)
				{
					switch(property)
					{
						case pEuSignature:
							err = AESetSig(tocH,messT.index,descP);
							break;
						case pEuWrap:
							SetFlag(tocH,messT.index,FLAG_WRAP_OUT,GetAEBool(descP));
							break;
						case pEuKeepCopy:
							SetFlag(tocH,messT.index,FLAG_KEEP_COPY,GetAEBool(descP));
							break;
						case pEuHqxText:
							SetFlag(tocH,messT.index,FLAG_BX_TEXT,GetAEBool(descP));
							break;
						case pEuFakeTabs:
							SetFlag(tocH,messT.index,FLAG_TABS,GetAEBool(descP));
							break;
						case pEuAttachType:
							num = GetAELong(descP)&0x3;
							SetAttachmentType(tocH,messT.index,num);
							break;
						case pEuMayQP:
							SetFlag(tocH,messT.index,FLAG_CAN_ENC,GetAEBool(descP));
							break;
						case pEuReturnReceipt:
							SetFlag(tocH,messT.index,FLAG_RR,GetAEBool(descP));
							break;
						case pEuAttDel:
							SetOpt(tocH,messT.index,OPT_ATT_DEL,GetAEBool(descP));
							break;
						default:
							err = errAENoSuchObject;
							break;
					}
					if ((*tocH)->sums[messT.index].messH) CompIBarUpdate((*tocH)->sums[messT.index].messH);
				}
				else
					err = errAENoSuchObject;
				break;
		}
	}
	return(err);
	AEDisposeDescDataHandle(hProperty);
}

#pragma segment AE3

/**********************************************************************
 * 
 **********************************************************************/
OSErr AESetSig(TOCHandle tocH,short sumNum,AEDescPtr descP)
{
	long sig = GetAELong(descP);
	Str255 name;
	
	if ((sig&0xffffff00)=='sig\000')
	{
		sig &= 0xff;
		sig--;
	}
	else
	{
		MyLowerStr(GetAEPStr(name,descP));
		sig = Hash(name);
	}
	return(SetSig(tocH,sumNum,sig));
}

/************************************************************************
 * SetPreference - set a preference
 ************************************************************************/
OSErr SetPreference(AEDescPtr token,AEDescPtr descP)
{
	PrefToken pt;
	Str255 value;
	PersHandle oldCur;
	
	AEGetDescData(token,&pt,sizeof(pt));
	oldCur = CurPers;
	CurPers = PERS_FORCE(pt.personality);
	GetAEPStr(value,descP);
	if (pt.pref==PREF_PASS_TEXT && pt.personality)
	{
		//	Handle password a little differently
		PCopy((*pt.personality)->password,value);
		if (PrefIsSet(PREF_SAVE_PASSWORD)) PersSavePw(pt.personality);
	}
	else if (pt.pref == PREF_POP_SIGH)
	{
		// split out user and host and set separately
		Str255 user, host;
		UPtr atSign;
		
		PTerminate(value);
		if (atSign = strrchr(value+1,'@'))
		{
			MakePStr(user,value+1,atSign-value-1);
			MakePStr(host,atSign+1,*value-(atSign-value));
		}
		else
		{
			*host = 0;
			PCopy(user,value);
		}
		SetPref(PREF_STUPID_USER,user);
		SetPref(PREF_STUPID_HOST,host);
	}
	else
		SetPref(pt.pref,value);
	CurPers = oldCur;
	return(noErr);
}

/************************************************************************
 * SetField - change a field in a message
 ************************************************************************/
OSErr SetField(AEDescPtr token,AEDescPtr descP)
{
	short err;
	FieldToken ft;
	MessHandle messH;
	
	
	AEGetDescData(token,&ft,sizeof(ft));
	if (ft.isNick) return(SetNickField(token,descP));

#ifdef NEVER //DEBUG
	SetPort_(nil);
#endif

	/*
	 * grab message
	 */
	if (!(err=OpenMessByToken(&ft.messT,False,&messH)))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		err = SetMessField(messH,ft.name,descP);

		/*
		 * cleanup message
		 */
		(*messH)->win->isDirty = true;
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);
}


/************************************************************************
 * SetMessField - set a field in a message
 ************************************************************************/
OSErr SetMessField(MessHandle messH,PStr wantHead,AEDescPtr descP)
{
	AEDescPtr useMe;
	AEDesc textAD;
	short err=noErr;
	HeadSpec hSpec;
	Handle	hText;
	
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr((*messH)->win));
	NullADList(&textAD,nil);
	if (descP->descriptorType == typeChar)
		useMe = descP;
	else if (err=AECoerceDesc(descP,typeChar,&textAD)) {PopGWorld();return(err);}
	else useMe = &textAD;

	if (*wantHead && wantHead[*wantHead]!=':') PCatC(wantHead,':');
	
	if (!(err = AEGetDescDataHandle(useMe,&hText)))
	{
		if ((*(*messH)->tocH)->which==OUT && !*wantHead)
		{
			CompHeadFind(messH,0,&hSpec);
			err = CompHeadSet(TheBody,&hSpec,hText);
		}
		else if (CompHeadFindStr(messH,wantHead,&hSpec))
		{
			err = CompHeadSet(TheBody,&hSpec,hText);
		}
		else if (!EqualStrRes(wantHead,PLUGIN_INFO))
		{
			err = CompAddExtraHeaderDangerDangerLookOutWillRobinson(messH,wantHead,hText);
		}
		else err = errAENoSuchObject;
		AEDisposeDescDataHandle(hText);
	}
	
	DisposeADList(&textAD,nil);
	PopGWorld();
	
	return(err);
}

/************************************************************************
 * SetSomeText - change some text in a message
 ************************************************************************/
OSErr SetSomeText(AEDescPtr token,AEDescPtr descP)
{
	TERangeToken trToken;
	long len;
	short err;
	AEDesc textAD;
	AEDesc *useMe;
	Handle	hText;
	
	NullADList(&textAD,nil);
	if (descP->descriptorType == typeChar)
		useMe = descP;
	else if (err=AECoerceDesc(descP,typeChar,&textAD)) return(err);
	else useMe = &textAD;
	
	if (token->descriptorType == cEuWTEText)
		AEGetDescData(token,&trToken,sizeof(trToken));
	else
	{
		AEGetDescData(token,&trToken.teT,sizeof(trToken.teT));
		trToken.start = 0;
		trToken.stop = PeteLen(trToken.teT.pte);
	}
	
#ifdef DEBUG
	ComposeLogS(LOG_AE,nil,"\pSet %d:%d %d",trToken.start,trToken.stop,AEGetDescDataSize(useMe));
#endif

	len = PeteLen(trToken.teT.pte);
	PeteSelect(nil,trToken.teT.pte,trToken.start,trToken.stop);
	if (!AEGetDescDataHandle(useMe,&hText))
	{
		PeteInsert(trToken.teT.pte,-1,hText);
		AEDisposeDescDataHandle(hText);
	}
	PeteSetDirty(trToken.teT.pte);
	
	DisposeADList(&textAD,nil);
	
	return(noErr);
}

/************************************************************************
 * SetSelText - change some text in a message
 ************************************************************************/
OSErr SetSelText(MyWindowPtr win,AEDescPtr descP)
{
	OSErr err = errAENoSuchObject;
	
	if (IsMyWindow(GetMyWindowWindowPtr(win)) && win->pte)
	{
		AEDesc textAD;
		AEDesc *useMe;
		Handle	hText;
		long oldSelStart, oldSelEnd;
		
		NullADList(&textAD,nil);
		if (descP->descriptorType == typeChar)
			useMe = descP;
		else if (err=AECoerceDesc(descP,typeChar,&textAD)) return(err);
		else useMe = &textAD;
		
		if (!(err = AEGetDescDataHandle(useMe,&hText)))
		{
			PeteGetTextAndSelection(win->pte,nil,&oldSelStart,&oldSelEnd);
			err = PeteInsert(win->pte,-1,hText);
			if (oldSelStart != oldSelEnd)
				PeteSelect(nil,win->pte,oldSelStart,oldSelStart+GetHandleSize(hText));
			AEDisposeDescDataHandle(hText);
		}
		
		DisposeADList(&textAD,nil);
	}

	return(err);
}

/************************************************************************
 * GetMessData - get a message's data
 ************************************************************************/
OSErr GetMessData(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	short err;
	MessToken mt;
	MessHandle messH;
	
	/*
	 * grab message
	 */
	AEGetDescData(token,&mt,sizeof(mt));
	if (!(err=OpenMessByToken(&mt,False,&messH)))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		err = GetMessField(messH,nil,reply,refCon);

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);
}

/************************************************************************
 * GetSelText - get a selection's data
 ************************************************************************/
OSErr GetSelText(MyWindowPtr win,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err=errAENoSuchObject;
	Handle text;
	long start,stop;
	
	if (IsMyWindow(GetMyWindowWindowPtr(win)) && win->pte)
	{
		err = PeteGetTextAndSelection(win->pte,&text,&start,&stop);
		if (!err) err = AEPutParamPtr(reply,keyAEResult,typeChar,
							LDRef(text)+start,stop-start);
		UL(text);
	}
	return(err);
}

/************************************************************************
 * GetField - get a field out of a message
 ************************************************************************/
OSErr GetField(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	short err;
	FieldToken ft;
	MessHandle messH;
	
	AEGetDescData(token,&ft,sizeof(ft));
	if (ft.isNick) return(GetNickField(token,reply,refCon));
	
	/*
	 * grab message
	 */
	if (!(err=OpenMessByToken(&ft.messT,False,&messH)))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		err = GetMessField(messH,ft.name,reply,refCon);

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}
	return(err);
}

/************************************************************************
 * GetAttachment - get an attachment of a message
 ************************************************************************/
OSErr GetAttachment(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	short err;
	AttachmentToken	at;
	FSSpec spec;
	MessHandle messH;
	
	AEGetDescData(token,&at,sizeof(at));
	if (!(err = OpenMessByToken(&at.messT,false,&messH)))
	{
		WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);

		if (GetAttachmentFromMsg(messH,at.index,&spec))
			err = AEPutParamPtr(reply,keyAEResult,typeFSS,&spec,sizeof(spec));
		else
			err = errAENoSuchObject;

		/*
		 * cleanup message
		 */
		if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
	}

	return(err);
}

/************************************************************************
 * GetSomeText - get some text out of something
 ************************************************************************/
OSErr GetSomeText(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	TERangeToken trToken;
	UPtr text;
	long len;
	short err;
	Handle textH;
	
	if (token->descriptorType == cEuWTEText)
		AEGetDescData(token,&trToken,sizeof(trToken));
	else
	{
		AEGetDescData(token,&trToken.teT,sizeof(trToken.teT));
		trToken.start = 0;
		trToken.stop = PeteLen(trToken.teT.pte);
	}
	
	PeteGetTextAndSelection(trToken.teT.pte,&textH,nil,nil);
	text = LDRef(textH);
	len = GetHandleSize(textH);

#ifdef DEBUG
	ComposeLogS(LOG_AE,nil,"\pGet %d:%d",trToken.start,trToken.stop);
	CarefulLog(LOG_AE,LOG_SENT,text+trToken.start,trToken.stop-trToken.start);
#endif
	
	err = AEPutParamPtr(reply,keyAEResult,typeChar,
		text+trToken.start,
		trToken.stop-trToken.start);

	UL(textH);
	return(err);
}

/************************************************************************
 * GetMessField - return data from a regular message field
 ************************************************************************/
OSErr GetMessField(MessHandle messH,PStr wantHead,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	UPtr h1, h2;
	long s1, s2;
	short n;
	UPtr body,end;
	AEDescList lst;
	short err;
	UHandle text;
	UHandle cache = nil;
	Boolean isComp = GetWindowKind (GetMyWindowWindowPtr((*messH)->win))==COMP_WIN;
						
	if (wantHead && *wantHead && !isComp)
	{
		CacheMessage((*messH)->tocH,(*messH)->sumNum);
		text = SumOf(messH)->cache;
		if (!text) return(errAEEventNotHandled);
		cache = text;	// so we know we must HPurge it later
		HNoPurge(cache);
	}
	else text = MessText(messH);
	
	/*
	 * find first occurrence of header
	 */
	body = LDRef(text);
	end = body + GetHandleSize(text);
	s1 = end-body;
	if (!wantHead)
	{
		err = AEPutParamPtr(reply,keyAEResult,typeChar,body,end-body);
	}
	else if (!*wantHead)
	{
		if (isComp)
		{
			HeadSpec hs;
			CompHeadFind(messH,0,&hs);
			err = AEPutParamPtr(reply,keyAEResult,typeChar,*text+hs.value,hs.stop-hs.value);
		}
		else
		{
			n = BodyOffset(text);
			err = AEPutParamPtr(reply,keyAEResult,typeChar,body+n,(end-body)-n);
		}
	}
	else if (h1=FindHeaderString(body+(cache?1:0),wantHead,&s1,False))	// the +1 skips the uucp envelope in cache
	{
		if (wantHead[*wantHead]==':') h1--,s1++;
		h1 -= *wantHead;	/* want header name, too */
		s1 += *wantHead;
		/*
		 * find second occurrence
		 */
		body = h1+s1+1;
		s2 = end-body;
		if (h2=FindHeaderString(body,wantHead,&s2,False))
		{
			/*
			 * more than one.  Have to return a list
			 */
			err = AECreateList(nil,0,False,&lst);
			if (!err)
			{
				/*
				 * add first string to list
				 */
				err = AEPutPtr(&lst,0,typeChar,h1,s1);
				if (!err)
				{
					/*
					 * add second and subsequent
					 */
					do
					{
						h2 -= *wantHead;	/* want header name, too */
						s2 += *wantHead;
						if (wantHead[*wantHead]==':') h1--,s1++;
						err = AEPutPtr(&lst,0,typeChar,h2,s2);
						body = h2+s2+1;
						s2 = end-body;
					}
					while(!err && (h2=FindHeaderString(body,wantHead,&s2,False)));
				}
				if (!err) err = AEPutParamDesc(reply,keyAEResult,&lst);
				/*
				 * get rid of list
				 */
				AEDisposeDesc(&lst);
			}
		}
		else	/* just one header found */
			err = AEPutParamPtr(reply,keyAEResult,typeChar,h1,s1);
	}
	else err = errAENoSuchObject;		/* no header found */
	UL(text);
	if (cache) HPurge(cache);
	return(err);
}

/************************************************************************
 * AEPutEnum - put an enum to a descriptor list
 ************************************************************************/
OSErr AEPutEnum(AppleEvent *event,DescType key,long theLong)
{
	return(AEPutParamPtr(event,key,typeEnumerated,&theLong,sizeof(long)));
}


/************************************************************************
 * MessObjFromBoxDHash - specify a message object
 ************************************************************************/
OSErr MessObjFromBoxDHash(AEDescPtr boxDescP,uLong hash,AEDescPtr messDescP)
{
	OSErr err;
	AEDesc mNumDesc;
	
	if (!(err = AECreateDesc(typeLongInteger,&hash,sizeof(hash),&mNumDesc)))
	{
		err = CreateObjSpecifier(cEuMessage,boxDescP,formUniqueID,&mNumDesc,False,messDescP);
		AEDisposeDesc(&mNumDesc);
	}
	return(err);
}

/************************************************************************
 * MessObjFromBoxD - specify a message object
 ************************************************************************/
OSErr MessObjFromBoxD(AEDescPtr boxDescP,short mNum,AEDescPtr messDescP)
{
	OSErr err;
	AEDesc mNumDesc;
	
	mNum++;
	if (!(err = AECreateDesc(typeLongInteger,&mNum,sizeof(mNum),&mNumDesc)))
	{
		err = CreateObjSpecifier(cEuMessage,boxDescP,formAbsolutePosition,&mNumDesc,False,messDescP);
		AEDisposeDesc(&mNumDesc);
	}
	return(err);
}

/************************************************************************
 * LaunchAppWith - launch an application, giving it a file to open
 ************************************************************************/
OSErr LaunchAppWith(FSSpecPtr app, FSSpecPtr doc, Boolean printIt)
{
	short err = noErr;
	LaunchParamBlockRec lpb;
	ProcessSerialNumber psn;
	AppleEvent ae;
	AEDesc launchDesc;
	AEAddressDesc appAD;
	
	
	if (doc)
	{
		/*
		 * build an odoc
		 */
		WriteZero(&psn,sizeof(psn));	/* pm will fix this */
		if (err=BuildODoc(&psn,doc,&ae,printIt)) return(err);
		
		/*
		 * coerce to proper descriptor, like DTS says
		 */
		err = AECoerceDesc(&ae,typeAppParameters,&launchDesc);
		if (err) WarnUser(AE_TROUBLE,err);
	}
	
	/*
	 * let's launch it
	 */
	if (!err)
	{
		Zero(lpb);
		if (doc)
		{
			Size appParamSize = AEGetDescDataSize(&launchDesc);
			if (lpb.launchAppParameters = NuPtr(appParamSize))
				AEGetDescData(&launchDesc,lpb.launchAppParameters,appParamSize);
		}
		lpb.launchBlockID = extendedBlock;
		lpb.launchEPBLength = extendedBlockLen;
		lpb.launchControlFlags = launchContinue | launchNoFileFlags;
		(void) IsAlias(app,app);
		lpb.launchAppSpec = app;
		WriteZero(&lpb.launchProcessSN,sizeof(lpb.launchProcessSN));
		if (err = LaunchApplication(&lpb)) WarnUser(COULDNT_LAUNCH,err);
		if (lpb.launchAppParameters) DisposePtr(lpb.launchAppParameters);
		if (printIt && !err)
		if (!(err=FindPSNByCreator(&psn,FileCreatorOf(app))))
		{
			if (!(err=AECreateDesc(typeProcessSerialNumber,&psn,sizeof(psn),&appAD)))
			{
				QuitApp(&appAD);
				AEDisposeDesc(&appAD);
			}
		}
	}
	
	if (doc)
	{
		AEDisposeDesc(&ae);
		AEDisposeDesc(&launchDesc);
	}
	
	return(noErr);
}

/************************************************************************
 * QuitApp - make an application quit
 ************************************************************************/
OSErr QuitApp(AEAddressDescPtr aead)
{
	AppleEvent ae;
	short err;
	extern AEIdleUPP MyAEIdleUPP;
	
	NullADList(&ae,nil);
	
	/*
	 * create the event
	 */
	err = AECreateAppleEvent(kCoreEventClass,kAEQuitApplication,aead,kAutoGenerateReturnID,kAnyTransactionID,&ae);
	
	/*
	 * now, send the event
	 */
	if (!err) err = MyAESend(&ae,nil,kAEQueueReply|kAECanInteract,kAENormalPriority,0);

	DisposeADList(&ae,nil);

	return(err);
}

/************************************************************************
 * BuildODoc - build an ODoc event
 ************************************************************************/
OSErr BuildODoc(ProcessSerialNumberPtr psn,FSSpecPtr doc,AppleEvent *ae,Boolean printIt)
{
	OSErr err;
	AEAddressDesc target;
	AEDescList theDoc;
	AliasHandle alias=nil;
	
	if (err = NewAlias(nil,doc,&alias)) return(WarnUser(AE_TROUBLE,err));
	
	if (!(err=AECreateDesc(typeProcessSerialNumber,psn,sizeof(*psn),&target)))
	{
		if (!(err=AECreateAppleEvent(kCoreEventClass,printIt?kAEPrintDocuments:kAEOpenDocuments,&target,
																 kAutoGenerateReturnID,kAnyTransactionID,ae)))
		{
			if (!(err=AECreateList(nil,0,False,&theDoc)))
			{
				if (!(err=AEPutPtr(&theDoc,0,typeAlias,LDRef(alias),GetHandleSize_(alias))))
				{
					err = AEPutParamDesc(ae,keyDirectObject,&theDoc);
				}
				AEDisposeDesc(&theDoc);
			}
		}
		AEDisposeDesc(&target);
	}
	
	ZapHandle(alias);
	
	if (err) WarnUser(AE_TROUBLE,err);
	
	return(err);
}

/************************************************************************
 * TypeIsOnList - is this file of a type the Finder should open?
 ************************************************************************/
TypeIsOnListEnum TypeIsOnListWhereAndIndex(OSType type,OSType listType,short *where,short *index)
{
	short i,n;
	OSType **resH=nil;
	Boolean is = False;
	SignedByte state;
	TypeIsOnListEnum retVal = kTOLNot;
	
	for (i=1;!is && (resH=GetIndResource_(listType,i));i++)
	{
		state = HGetState((Handle)resH);
		HNoPurge_(resH);
		n = GetHandleSize_(resH)/sizeof(OSType);
		while (n--)
		{
			if ((*resH)[n]==type || (*resH)[n]=='****')
			{
				is = True;
				if (where) *where = i;
				if (index) *index = n;
				break;
			}
		}
		if (is) retVal = HomeResFile((void*)resH)==SettingsRefN ? kTOLSettings : kTOLOther;
		HSetState((Handle)resH,state);
	}
	return(retVal);
}

/************************************************************************
 * OpenOtherDoc - open a document with some other application
 ************************************************************************/
OSErr OpenOtherDoc(FSSpecPtr spec,Boolean finderSelect,Boolean printIt,PETEHandle sourcePTE)
{
	short err;
	FInfo info;
	OSType newCreator;
	Boolean option = (MainEvent.modifiers & optionKey)!=0;
	FSSpec	trashSpec;
	
	if (err = AFSpGetFInfo(spec,spec,&info))
	{
		if (FSpIsItAFolder(spec)) return(FinderOpen(spec,False,printIt));	// if folder, open in finder
		return(FileSystemError(BINHEX_OPEN,spec->name,err));
	}
	
	if (finderSelect) return(FinderOpen(spec,True,printIt));
	
	if (info.fdType=='APPL') return(LaunchAppWith(spec,nil,printIt));
	
#ifdef OLDPGP
	if (info.fdType==PGP_ENCRYPTED_TYPE) return(PGPOpenEncrypted(spec));
	
	if (info.fdType==PGP_JUSTSIG_TYPE) return(PGPVerifyFile(spec));
#endif

	if (info.fdType==kFakeAppType) return(OpenAttachedApp(spec));
	
	// Stuff we do directly
	if (info.fdType!=SETTINGS_TYPE && info.fdCreator==CREATOR)
		return (OpenOneDoc(sourcePTE,spec,&info));
	
	if (HaveTheDiseaseCalledOSX()&&!PrefIsSet(PREF_USE_OWN_DOC_HELPERS) || !option && TypeIsOnList(info.fdType,FINDER_LIST_TYPE)) return(FinderOpen(spec,False,printIt));

	if (!option)
	{
		if (!(err=RunningODoc(spec,nil,sourcePTE,printIt))) return(noErr);
		if (err!=1) return(err);
		
		//	Let Finder open it if it will.
		//	There's a problem with Mac OS X opening applications
		//	that are in bundles. However, don't let Finder open it
		//	if it's in the trash. Finder refuses to do that.
		GetTrashSpec(spec->vRefNum,&trashSpec);
		if (spec->vRefNum != trashSpec.vRefNum || spec->parID != trashSpec.parID)
			if (!FinderOpen(spec,false,printIt))
				return noErr;
		
		if (!(err=CreatorODoc(spec,nil,printIt))) return(noErr);
		if (err!=1) return(err);

		if (CreatorMap(spec,&newCreator))
		{
			if (!(err=RunningODoc(spec,&newCreator,sourcePTE,printIt))) return(noErr);
			if (err!=1) return(err);
			
			if (!(err=CreatorODoc(spec,&newCreator,printIt))) return(noErr);
			if (err!=1) return(err);
		}
	}
	
	return(SFODoc(spec,printIt));
}

/************************************************************************
 * FinderOpen - have the Finder open a file
 ************************************************************************/
OSErr FinderOpen(FSSpecPtr spec,Boolean finderSelect,Boolean printIt)
{
	AliasHandle fileAlias=nil;
	OSErr err;
	ProcessSerialNumber psn;
	DescType eClass, eId;

	if (!(err = FindPSNByCreator(&psn,'MACS')))
	if (!(err = NewAlias(nil,spec,&fileAlias)))
	{
		if (finderSelect) { eClass = kAEMiscStandards; eId = kAEMakeObjectsVisible; }
		else { eClass = kCoreEventClass; eId = printIt?kAEPrint:kAEOpen; }		
		err = SimpleAESend(&psn,eClass,eId,nil,kEAEWhenever,
				 keyDirectObject,typeAlias,LDRef(fileAlias),GetHandleSize_(fileAlias),nil,nil);
	}
	
	//	MacOS betas are not bring Finder to the front when it gets a kAERevealSelection event
	//	so let's make sure it comes to the front
	if (finderSelect || FSpIsItAFolder(spec))
		SetFrontProcess(&psn);

	ZapHandle(fileAlias);

	if (err) WarnUser(AE_TROUBLE,err);
	
	return(err);
}

/************************************************************************
 * CreatorMap - map the creator of a file
 ************************************************************************/
Boolean CreatorMap(FSSpecPtr spec,OSType *newCreator)
{
	short err;
	OSType **new;
	Str31 name;
	FInfo info;
	
	if (err=AFSpGetFInfo(spec,spec,&info))
	{
		FileSystemError(BINHEX_OPEN,spec->name,err);
		return(False);
	}
	
	MakePStr(name,&info.fdType,4);
	
	new = (OSType **)GetNamedResource('EuCM',name);
	
	if (new)
	{
		*newCreator = **new;
		return(True);
	}
	
	return(False);
}
	
/************************************************************************
 * CreatorODoc - do any desktop db's know how to open this file?
 ************************************************************************/
OSErr CreatorODoc(FSSpecPtr spec,OSType *mapCreator,Boolean printIt)
{
	short err;
	FInfo info;
	short volIndex;
	short vRef;
	OSType creator;
	
	if (mapCreator) creator = *mapCreator;
	else
	{
		if (err = AFSpGetFInfo(spec,spec,&info)) return(FileSystemError(BINHEX_OPEN,spec->name,err));
		creator = info.fdCreator;
	}
	
	if (!(err = VolumeCreatorODoc(BlessedVRef(),creator,spec,printIt))) return(noErr);
	
	for (volIndex=1;!(err=IndexVRef(volIndex,&vRef));volIndex++)
	{
		if (!(err=VolumeCreatorODoc(vRef,creator,spec,printIt))) return(noErr);
		if (err!=1) return(err);
	}

	return(1);	/* not found */
}

/************************************************************************
 * VolumeCreatorODoc - look for a file's creator on a volume, and launch
 ************************************************************************/
OSErr VolumeCreatorODoc(short vRef,OSType creator,FSSpecPtr spec,Boolean printIt)
{
	short dtRef;
	short err;
	FSSpec appSpec;
	
	if (err = DTRef(vRef,&dtRef)) return(1);	/* but don't complain too loudly */
	err = DTGetAppl(vRef,dtRef,creator,&appSpec);
	
	switch(err)
	{
		case noErr:
			err = LaunchAppWith(&appSpec,spec,printIt);
			break;
		
		default:
#ifdef CAP_PLAYED_NICELY
			WarnUser(DT_TROUBLE,err);
			break;
#endif
		
		case extFSErr:
		case afpItemNotFound:
			err = 1;
			break;
	}
	return(err);
}

/************************************************************************
 * RunningODoc - can a running application open this file?
 ************************************************************************/
OSErr RunningODoc(FSSpecPtr spec,OSType *mapCreator,PETEHandle sourcePTE,Boolean printIt)
{
	ProcessSerialNumber psn;
	short err;
	FInfo info;
	OSType creator;
	
	if (mapCreator) creator = *mapCreator;
	else
	{
		if (err = AFSpGetFInfo(spec,spec,&info)) return(FileSystemError(BINHEX_OPEN,spec->name,err));
		creator = info.fdCreator;
	}
	
	if (!FindPSNByCreator(&psn,creator))
		return(PSNODoc(&psn,spec,sourcePTE,printIt));
	
	return(1);	/* not found */
}

/************************************************************************
 * PSNODoc - send an ODOC to a given PSN.
 ************************************************************************/
OSErr PSNODoc(ProcessSerialNumberPtr psn, FSSpecPtr doc,PETEHandle sourcePTE,Boolean printIt)
{
	AppleEvent ae;
	short err;
	
	if (!(err=BuildODoc(psn,doc,&ae,printIt)))
	{
		if (sourcePTE) AddSourcePTE(&ae,sourcePTE);
		err = MyAESend(&ae,nil,kAEQueueReply|kAECanInteract|kAECanSwitchLayer,
								 kAENormalPriority,kAEDefaultTimeout);
		AEDisposeDesc(&ae);
		if (!err) SetFrontProcess(psn);
		else WarnUser(BINHEX_OPEN,err);
	}
	return(err);
}

/************************************************************************
 * AddSourcePTE - add the source edit region as optional parameter to an event
 ************************************************************************/
OSErr AddSourcePTE(AppleEvent *ae,PETEHandle sourcePTE)
{
	OSErr err;
	OSType editorKeyword = keyEuEditRegion;
	AEDesc optD;
	
	NullADList(&optD,nil);
	
	if (!(err = AEPutParamPtr(ae,keyEuEditRegion,typeLongInteger,(void*)&sourcePTE,sizeof(sourcePTE))))
	if (!(err = AECreateList(nil,0,False,&optD)))
	if (!(err = AEPutPtr(&optD,1,typeKeyword,&editorKeyword,sizeof(editorKeyword))))
		err = AEPutAttributeDesc(ae,keyOptionalKeywordAttr,&optD);
	
	DisposeADList(&optD,nil);
	return(err);
}

/************************************************************************
 * SFODoc - ask the user what to use
 ************************************************************************/
OSErr SFODoc(FSSpecPtr doc,Boolean printIt)
{
	ProcessSerialNumber	psn;
	SFODocType					options;
	FInfo								info;
	SFTypeList					types;
	FSSpec							spec;
	Str255							prompt;
	OSErr								theError;
	Boolean							good;

	if (theError = AFSpGetFInfo(doc,doc,&info)) return(FileSystemError(BINHEX_OPEN,doc->name,theError));
	TypeToOpen = info.fdType;
	
	types[0] = 'APPL';
	types[1] = 'adrp';
	types[2] = 'adrp';
	types[3] = 0;

	options.permanently	= True;
	options.finder			= False;
	options.type				= info.fdType;

	ComposeRString (prompt, CHOOSE_APP, doc->name);

	theError = SFODocNav (doc, types, prompt, &spec, &options, &good);
	if (theError == userCanceledErr && options.finder)
		theError = noErr;

	if (!theError) {
		if (good || options.finder) {
			if (TypeToOpen != FetchOSType (DEFAULT_TYPE))
				if (options.permanently) {
					if (options.finder)
						SaveFinderType (info.fdType);
					else {
						RemoveFinderType (info.fdType);
						SaveCreatorPref (info.fdType, &spec);
					}
				}
			if (options.finder)
				FinderOpen (doc, False,printIt);
			else
				if (SimilarAppIsRunning (&spec,&psn))
				 return (PSNODoc (&psn, doc, nil,printIt));
			else
				return (LaunchAppWith (&spec, doc,printIt));
		}
		else
			if (options.permanently)
				SaveCreatorPref (info.fdType, nil);
	}
	return (1);
}


/************************************************************************
 * SaveFinderType - save this type as one the finder should open
 ************************************************************************/
OSErr SaveFinderType(OSType type)
{
	Handle resource=nil;
	OSErr err;
	
	if (TypeIsOnList(type,FINDER_LIST_TYPE)) return(noErr);	// nothing to do
	
	/*
	 * add it
	 */
	UseResFile(SettingsRefN);
	
	// if resource not already there, make one
	if (!(resource=Get1Resource(FINDER_LIST_TYPE,FINDER_LIST_ID)))
	{
		resource = NuHandle(0);
		if (!resource) return(MemError());
		AddResource(resource,FINDER_LIST_TYPE,FINDER_LIST_ID,"");
		if (ResError()) return(ResError());
	}
	
	// append type to resource
	err = PtrPlusHand(&type,resource,sizeof(type));
	if (err) return(err);
	
	
	ChangedResource(resource);
	err = MyUpdateResFile(SettingsRefN);
	
  return(err);
}
 
/************************************************************************
 * RemoveFinderType - clear this type as one the finder should open
 ************************************************************************/
OSErr RemoveFinderType(OSType type)
{
	OSType **resource=nil;
	OSErr err;
	short i,n;
	
	if (!TypeIsOnList(type,FINDER_LIST_TYPE)) return(noErr);	// nothing to do
	
	/*
	 * remove it
	 */
	UseResFile(SettingsRefN);
	
	// if resource not there, nothing we can do
	if (!(resource=(void*)Get1Resource(FINDER_LIST_TYPE,FINDER_LIST_ID)))
		return(fnfErr);
	
	// find type in resource
	n = HandleCount(resource);
	for (i=0;i<n;i++) if ((*resource)[i]==type) break;
	
	// if type not there, nothing we can do
	if (i==n) return(fnfErr);
	
	// remove
	for (;i<n-1;i++) (*resource)[i] = (*resource)[i+1];
	SetHandleBig((void*)resource,(n-1)*sizeof(type));
	
	ChangedResource((void*)resource);
	err = MyUpdateResFile(SettingsRefN);
	
  return(err);
}

/**********************************************************************
 * OpenDocWith - open a document with a particular app
 **********************************************************************/
OSErr OpenDocWith(FSSpecPtr doc,FSSpecPtr app, Boolean printIt)
{
	ProcessSerialNumber psn;
	
	if (!FindPSNBySpec(&psn,app)) return(PSNODoc(&psn,doc,nil,printIt));
	else return(LaunchAppWith(app,doc,printIt));
}

/************************************************************************
 * SaveCreatorPref - save the creator for a file
 ************************************************************************/
OSErr SaveCreatorPref(OSType type, FSSpecPtr app)
{
	short err;
	FInfo info;
	OSType **creatorH, creator;
	Str31 name;

	BMD(&type,name+1,4);*name = 4;
	
	/*
	 * remove old resource
	 */
	if (creatorH = (OSType**)GetNamedResource('EuCM',name))
	{
		if (HomeResFile((Handle)creatorH)==SettingsRefN)
		{
			RemoveResource((Handle)creatorH);
			ZapHandle(creatorH);
		}
		else
			ReleaseResource_((Handle)creatorH);
	}
	
	/*
	 * find app info
	 */
	if (!app) return(noErr);
	if (err=AFSpGetFInfo(app,app,&info)) return(err);
	
	/*
	 * build EuCM resource
	 */
	if (!(creatorH = NuHandle(sizeof(OSType)))) return(MemError());
	**creatorH = creator = info.fdCreator;
	AddMyResource_(creatorH,'EuCM',MyUniqueID('EuCM'),name);
	err = ResError();
	UpdateResFile(SettingsRefN);
	
	return(err);
}

/************************************************************************
 * SimilarAppIsRunning - is an application running?
 ************************************************************************/
Boolean SimilarAppIsRunning(FSSpecPtr app,ProcessSerialNumberPtr psn)
{
	short err;
	FInfo info;
	
	if (err = AFSpGetFInfo(app,app,&info)) return(FileSystemError(COULDNT_LAUNCH,app->name,err));
	
	if (!FindPSNByCreator(psn,info.fdCreator)) return(True);
	
	return(False);	/* not found */
}

/**********************************************************************
 * CreatorToApp - map a creator to the app that made it
 **********************************************************************/
OSErr CreatorToApp(OSType creator,AliasHandle *app)
{
	ProcessSerialNumber psn;
	ProcessInfoRec pi;
	FSSpec spec;
	short volIndex;
	short vRef;
	short dtRef;
	
	if (!FindPSNByCreator(&psn,creator))
	{
		pi.processInfoLength = sizeof(pi);
		pi.processName = nil;
		pi.processAppSpec = &spec;
		if (!GetProcessInformation(&psn,&pi))
			return(NewAlias(nil, &spec, app));
	}
		
	for (volIndex=1;!IndexVRef(volIndex,&vRef);volIndex++)
		if (!DTRef(vRef,&dtRef) && !DTGetAppl(vRef,dtRef,creator,&spec))
			return(NewAlias(nil,&spec,app));
	
	return(fnfErr);
}

/************************************************************************
 * CantOpenFilt - can an app open the type?
 ************************************************************************/
pascal Boolean CantOpenFilt(CInfoPBPtr pb)
{
	short err;

	if (pb->hFileInfo.ioFlFndrInfo.fdType == 'adrp')
	{
		short vRef;
		long dirId;
		Str31 name;
		
		PCopy(name,pb->hFileInfo.ioNamePtr);
		vRef = pb->hFileInfo.ioVRefNum;
		dirId = pb->hFileInfo.ioFlParID;
		PCopy(name,pb->hFileInfo.ioNamePtr);
		if (err = MyResolveAlias(&vRef,&dirId,name,nil)) return(True);
		return(!CanOpen(TypeToOpen,vRef,dirId,name));
	}
		
	return(!CanOpen(TypeToOpen,pb->hFileInfo.ioVRefNum,
														 pb->hFileInfo.ioFlParID,
														 pb->hFileInfo.ioNamePtr));
}

/************************************************************************
 * CanOpen - can an app open a file?
 ************************************************************************/
Boolean CanOpen(OSType type,short vRef,long dirId,PStr name)
{
	short resF;
	short i;
	OSType **resH;
	Boolean can = False;
	short oldResF = CurResFile();

	SetResLoad(False);
	resF = HOpenResFile(vRef,dirId,name,fsRdPerm);
	SetResLoad(True);
	if (resF>=0 && CurResFile()==resF)
	{
		for (i=1;resH=(OSType **)Get1IndResource('FREF',i);i++)
			if (**resH==type || **resH == '****')
			{
				can = True;
				break;
			}
		CloseResFile(resF);
	}
	UseResFile (oldResF);
	return(can);
}

#ifdef DEBUG
pascal OSErr DebugAEHandler(AppleEvent *event,AppleEvent *reply,long refCon)
{
	AEHandler *realHandler = (AEHandler *)refCon;
	if (BUG4)	Dprintf("\pAEHandler: %a %i",event,refCon);
	return((*realHandler)(event,reply,0));
}

pascal OSErr DebugOAccessor(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
	ObjectAccessor *realAccessor = (ObjectAccessor *)refCon;
	if (BUG4)	Dprintf("\pOA: %O %O %O %a %i",desiredClass,containerClass,keyForm,keyData,refCon);
	return((*realAccessor)(desiredClass,containerToken,containerClass,keyForm,keyData,theToken,0));
}
#endif



#pragma segment Ends

/************************************************************************
 * InstallAE - put in the handlers for required appleevents
 ************************************************************************/
void InstallAE(void)
{
	InstallAEOrDie(kCoreEventClass,kAEOpenApplication,HandleAEOApp,&HandleAEOAppUPP,0);
	InstallAEOrDie(kCoreEventClass,kAEOpenDocuments,HandleAEODoc,&HandleAEODocUPP,0);
	InstallAEOrDie(kCoreEventClass,kAEQuitApplication,HandleAEQuit,&HandleAEQuitUPP,0);
	InstallAEOrDie(kCoreEventClass,kAEShowPreferences,HandleAEPref,&HandleAEPrefUPP,0);
	InstallAEOrDie(kURLSuite,kURLGet,HandleAEURL,&HandleAEURLUPP,kURLGet);
	InstallAEOrDie(kURLSuite,kURLFetch,HandleAEURL,&HandleAEURLUPP,kURLFetch);
}

/************************************************************************
 * InstallAERest - install handlers for remaining events
 ************************************************************************/
void InstallAERest(void)
{
	short err;
//	extern OSLAccessorUPP FindNickFileUPP, FindNicknameUPP, FindNickFieldUPP;
	DECLARE_UPP(AECountStuff,OSLCount);
	
	INIT_UPP(AECountStuff,OSLCount);
	InstallAEOrDie(kAECoreSuite,kAEOpen,HandleAEOObj,&HandleAEOObjUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEClose,HandleAEClose,&HandleAECloseUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEDelete,HandleAEDelete,&HandleAEDeleteUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEGetData,HandleAEGetData,&HandleAEGetDataUPP,0);
	InstallAEOrDie(kAECoreSuite,kAESetData,HandleAESetData,&HandleAESetDataUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEMove,HandleAEMove,&HandleAEMoveUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEClone,HandleAEMove,&HandleAEMoveUPP,kAEClone);
	InstallAEOrDie(kAECoreSuite,kAECreateElement,HandleAECreate,&HandleAECreateUPP,0);
	InstallAEOrDie(kAECoreSuite,kAEDoObjectsExist,HandleAEExists,&HandleAEExistsUPP,0);
	InstallAEOrDie(kCoreEventClass,kAEPrint,HandleAEPrint,&HandleAEPrintUPP,0);
	InstallAEOrDie(kAECoreSuite,kAESave,HandleAESave,&HandleAESaveUPP,0);
	InstallAEOrDie('cwin',typeWildCard,HandleFinderCruft,&HandleFinderCruftUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuConnect,HandleEuConnect,&HandleEuConnectUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuInstallNotify,HandleEuINot,&HandleEuINotUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuRemoveNotify,HandleEuRNot,&HandleEuRNotUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuCompact,HandleEuCompact,&HandleEuCompactUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuReply,HandleEuReply,&HandleEuReplyUPP,kEuReply);
	InstallAEOrDie(kEudoraSuite,kEuForward,HandleEuReply,&HandleEuReplyUPP,kEuForward);
	InstallAEOrDie(kEudoraSuite,kEuRedirect,HandleEuReply,&HandleEuReplyUPP,kEuRedirect);
	InstallAEOrDie(kEudoraSuite,kEuSalvage,HandleEuReply,&HandleEuReplyUPP,kEuSalvage);
	InstallAEOrDie(kEudoraSuite,kEuQueue,HandleEuQueue,&HandleEuQueueUPP,kEuSendNext);
	InstallAEOrDie(kEudoraSuite,kEuUnQueue,HandleEuQueue,&HandleEuQueueUPP,kEuSendNever);
	InstallAEOrDie(kEudoraSuite,kEuAttach,HandleEuAttach,&HandleEuAttachUPP,0);
	InstallAEOrDie(typeWildCard,kAEAnswer,HandleAEAnswer,&HandleAEAnswerUPP,0);
	InstallAEOrDie(typeWildCard,kAECountElements,HandleAECount,&HandleAECountUPP,0);
	InstallAEOrDie(kEudoraSuite,kEuPalmConduitAction,HandleEuConduitAction,&HandleEuConduitActionUPP,0);

	InstallOAOrDie(cEuNickFile,typeNull,FindNickFile,&FindNickFileUPP);
	InstallOAOrDie(cEuNickname,cEuNickFile,FindNickname,&FindNicknameUPP);
	InstallOAOrDie(cEuNickname,typeNull,FindNickname,&FindNicknameUPP);
	InstallOAOrDie(cEuField,cEuNickname,FindNickField,&FindNickFieldUPP);

	InstallOAOrDie(cEuMailbox,cEuMailfolder,FindBoxInMFolder,&FindBoxInMFolderUPP);
	InstallOAOrDie(cEuMailbox,typeNull,FindBox,&FindBoxUPP);
	InstallOAOrDie(cEuMailfolder,typeNull,FindMFolder,&FindMFolderUPP);
	InstallOAOrDie(cEuMailfolder,cEuMailfolder,FindMFolderInMFolder,&FindMFolderInMFolderUPP);
	InstallOAOrDie(cEuMessage,cEuMailbox,FindMessage,&FindMessageUPP);
	InstallOAOrDie(cEuMessage,typeNull,FindMessage,&FindMessageUPP);
	InstallOAOrDie(cEuField,cEuMessage,FindField,&FindFieldUPP);
	InstallOAOrDie(cEuAttachment,cEuMessage,FindAttachment,&FindAttachmentUPP);
	InstallOAOrDie(cEuTEInWin,cWindow,FindWinTE,&FindWinTEUPP);
	InstallOAOrDie(cChar,cEuWTEText,FindWTNest,&FindWTNestUPP);
	InstallOAOrDie(cChar,cEuTEInWin,FindWTStuff,&FindWTStuffUPP);
	InstallOAOrDie(cWord,cEuTEInWin,FindWTStuff,&FindWTStuffUPP);
	InstallOAOrDie(cText,cEuTEInWin,FindWTStuff,&FindWTStuffUPP);
	InstallOAOrDie(cWindow,typeNull,FindWin,&FindWinUPP);
	InstallOAOrDie(cEuFilter,typeNull,FindFilter,&FindFilterUPP);
	InstallOAOrDie(cEuFilterTerm,cEuFilter,FindTerm,&FindTermUPP);
	InstallOAOrDie(cEuPreference,typeNull,FindPreference,&FindPreferenceUPP);
	InstallOAOrDie(cEuHelper,typeNull,FindURLHelper,&FindURLHelperUPP);
	if (HasFeature (featureMultiplePersonalities)) {
		InstallOAOrDie(cEuPersonality,typeNull,FindPersonality,&FindPersonalityUPP);
		InstallOAOrDie(cEuPreference,cEuPersonality,FindPreferenceInPersonality,&FindPreferenceInPersonalityUPP);
	}
	InstallOAOrDie(cProperty,typeWildCard,FindProperty,&FindPropertyUPP);
	
	if (err=AESetObjectCallbacks(nil,AECountStuffUPP,nil,nil,nil,nil,nil))
		DieWithError(INSTALL_AE,err);
}

/************************************************************************
 * InstallAEOrDie - install a handler or die
 ************************************************************************/
void InstallAEOrDie(AEEventClass theAEEventClass,AEEventID theAEEventID,AEEventHandlerProcPtr proc,AEEventHandlerUPP *upp,long refCon)
{
	OSErr err;
	if (!*upp) *upp = NewAEEventHandlerUPP(proc);
	err = AEInstallEventHandler(theAEEventClass,theAEEventID,*upp,refCon,false);
	if (err) DieWithError(INSTALL_AE,err);
}

/************************************************************************
 * InstallOAOrDie - install an object accessor or die
 ************************************************************************/
void InstallOAOrDie(DescType desiredClass, DescType containerType,OSLAccessorProcPtr proc,OSLAccessorUPP *upp)
{
	OSErr err;
	if (!*upp) *upp = NewOSLAccessorUPP(proc);
	err = AEInstallObjectAccessor(desiredClass,containerType,*upp,0,false);
	if (err) DieWithError(INSTALL_AE,err);
}

#pragma segment Spell

#ifdef TWO
/************************************************************************
 * HandleWordServices - handle a WS menu choice
 ************************************************************************/
void HandleWordServices(MyWindowPtr win, short item)
{
	AEDesc objAD;
	
	NullADList(&objAD,nil);
	if (!CreateTEHObj(win,&objAD)) ServeThemWords(item,&objAD);
	DisposeADList(&objAD,nil);
}
#endif

/**********************************************************************
 * AddWinToEvent - add a window object to an event
 **********************************************************************/
OSErr AddWinToEvent(AppleEvent *reply,AEKeyword keyword,MyWindowPtr win)
{
	short err;
	AEDesc root,window,num;
	long lData;
	
	NullADList(&root,&window,&num,nil);
	
	lData = win->windex;
	if (!(err=AECreateDesc(typeLongInteger,&lData,sizeof(lData),&num)))
	if (!(err=CreateObjSpecifier(cWindow,&root,formUniqueID,&num,False,&window)))
		err = AEPutParamDesc(reply,keyword,&window);
	DisposeADList(&root,&window,&num,nil);
	return(err);
}

/************************************************************************
 * CreateTEHObj - create a TE object specifier
 ************************************************************************/
OSErr CreateTEHObj(MyWindowPtr win, AEDescPtr objAD)
{
	short err;
	AEDesc root,window,num,text,range,subText;
	long lData;
	long start,stop,len;
	
	NullADList(&root,&window,&num,&text,&range,&subText,nil);
	
	lData = win->windex;
	if (!(err=AECreateDesc(typeLongInteger,&lData,sizeof(lData),&num)))
	if (!(err=CreateObjSpecifier(cWindow,&root,formUniqueID,&num,False,&window)))
	{
		lData = 1;
		if (!(err=AECreateDesc(typeLongInteger,&lData,sizeof(lData),&num)))
		if (!(err=CreateObjSpecifier(cEuTEInWin,&window,formAbsolutePosition,&num,False,&text)))
		if (!(err=AECreateList(nil,0,False,objAD)))
		{
			if (win->pte)
			{
				err = PeteGetTextAndSelection(win->pte,nil,&start,&stop);
				if (!err)
				{
					len = PeteLen(win->pte);
					if (start!=stop)
					{
						if (!(err=MyCreateRange(cChar,start-len,stop-len-1,&text,&range)))
						if (!(err=CreateObjSpecifier(cChar,&text,formRange,&range,False,&subText)))
							err = AEPutDesc(objAD,0,&subText);
					}
					else if (GetWindowKind (GetMyWindowWindowPtr (win))==COMP_WIN)
						err = CompPutSpellableFields(win,&text,objAD);
					else
						err = AEPutDesc(objAD,0,&text);
				}
			}
		}
	}
#ifdef NEVER
	err = AEBuild(objAD,"[obj{want:type(EuWT),form:indx,seld:1,from:obj{want:type(cwin),form:indx,seld:1,from:()}}]");
#endif
	DisposeADList(&root,&window,&num,&text,&range,&subText,nil);
	return(err);
}


OSErr CompPutSpellableFields(MyWindowPtr win,AEDescPtr textAD,AEDescPtr objAD)
{
	OSErr err = noErr;
	AEDesc range,subText;
	HeadSpec hs;
	long start;
	Str31 prefix;
	Handle text;
	Boolean quote;
	Boolean allWhite;
	PETEHandle pte = (*Win2MessH(win))->bodyPTE;
	long len = PeteLen(pte);
	PETEParaInfo pinfo;
	
	NullADList(&range,&subText,nil);
	
	if (CompHeadFind(Win2MessH(win),SUBJ_HEAD,&hs))
	{
		if (hs.value!=hs.stop)
		{
			if (!(err=MyCreateRange(cChar,hs.value-len,hs.stop-1-len,textAD,&range)))
			if (!(err=CreateObjSpecifier(cChar,textAD,formRange,&range,False,&subText)))
				err = AEPutDesc(objAD,0,&subText);
		}
	}
	
	if (!err && CompHeadFind(Win2MessH(win),0,&hs))
	{
		if (hs.value!=hs.stop)
		{
			GetRString(prefix,QUOTE_PREFIX);
			PeteGetTextAndSelection(pte,&text,nil,nil);
			len = GetHandleSize(text);
			start = hs.value;
			do
			{
				// is this line quoted?
				LDRef(text);
				quote = !striscmp(*text+hs.value,prefix+1);
				UL(text);
				if (!quote)
				{
					short pIndex = PeteParaAt(pte,hs.value);
					Zero(pinfo);
					PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
					quote = pinfo.quoteLevel > 0;
				}
				
				if (quote)
				{
					if (start!=hs.value)
					{
						if (!(err=MyCreateRange(cChar,start-len,hs.value-2-len,textAD,&range)))
						if (!(err=CreateObjSpecifier(cChar,textAD,formRange,&range,False,&subText)))
							err = AEPutDesc(objAD,0,&subText);
					}
					allWhite = True;
				}
				
				// advance to next line
				while (hs.value<hs.stop)
				{
					if ((*text)[hs.value]=='\015')
					{
						hs.value++;
						break;
					}
					else if (allWhite) allWhite = IsWhite((*text)[hs.value]);
					hs.value++;
				}
				
				// if the last line was a quote or all white, skip it
				if (quote || allWhite) start = hs.value;
			}
			while(hs.value<hs.stop);

			if (start!=hs.value)
			{
				if (!(err=MyCreateRange(cChar,start-len,hs.value-1-len,textAD,&range)))
				if (!(err=CreateObjSpecifier(cChar,textAD,formRange,&range,False,&subText)))
					err = AEPutDesc(objAD,0,&subText);
			}
			
		}			
	}
	DisposeADList(&range,&subText,nil);
	return(err);
}

/**********************************************************************
 * MyCreateRange - create a range descriptor
 **********************************************************************/
OSErr MyCreateRange(DescType elemClass,long start,long finish,AEDescPtr inObj,AEDescPtr range)
{
	AEDesc startAD, finishAD, startNumAD, finishNumAD;
	OSErr err;
	
	NullADList(&startAD,&finishAD,&startNumAD,&finishNumAD,nil);
	
	//finish--; // because we point at a discrete object
	
	if (!(err=AECreateDesc(typeLongInteger,&start,sizeof(start),&startNumAD)))
	if (!(err=CreateObjSpecifier(elemClass,inObj,formAbsolutePosition,&startNumAD,False,&startAD)))
	if (!(err=AECreateDesc(typeLongInteger,&finish,sizeof(finish),&finishNumAD)))
	if (!(err=CreateObjSpecifier(elemClass,inObj,formAbsolutePosition,&finishNumAD,False,&finishAD)))
		err = CreateRangeDescriptor(&startAD,&finishAD,False,range);
	DisposeADList(&startAD,&finishAD,&startNumAD,&finishNumAD,nil);
	return(err);
}

#pragma segment ObjSupport

/************************************************************************
 * FindProperty - Access a property
 ************************************************************************/
pascal OSErr FindProperty(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,keyForm,refCon)
	PropTokenPtr ptp;
	short tokenSize;
	short err;
	
	tokenSize = AEGetDescDataSize(containerToken);
	ptp = NuPtr(tokenSize+sizeof(PropToken));
	if (!ptp) return(MemError());
	
	AEGetDescData(containerToken,ptp+1,tokenSize);	
	AEGetDescData(keyData,&ptp->propertyId,sizeof(long));	
	ptp->tokenClass = containerClass;
	err = AECreateDesc(cProperty,ptp,sizeof(PropToken)+tokenSize,theToken);
	ZapPtr(ptp);

 	return(err);
}

/************************************************************************
 * FindMessage - access a message object
 ************************************************************************/
pascal OSErr FindMessage(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,refCon)
	MessToken token;
	FSSpec spec;
	short err=noErr;
	short mNum;
	TOCHandle tocH;
	uLong id;
	
	if (containerClass==typeNull)		/* choose current message */
	{
		err = CurrentMessage(&tocH,&mNum);
		if (!err)
		{
			token.spec = GetMailboxSpec(tocH,mNum);
			token.index = mNum;
		}
	}
	else
		switch (keyForm)
		{
			case formAbsolutePosition:
				mNum = GetAELong(keyData);
				AEGetDescData(containerToken,&spec,sizeof(spec));
				if (!(err=GetTOCByFSS(&spec,&tocH)))
				{
					if (0 < mNum && mNum<=(*tocH)->count)
					{
						token.spec = spec;
						token.index = mNum-1;
					}
					else if (0 > mNum && mNum >= -(*tocH)->count)
					{
						token.spec = spec;
						token.index = (*tocH)->count + mNum;
					}
					else err = errAENoSuchObject;
				}
				break;
			case formUniqueID:
				id = GetAELong(keyData);
				AEGetDescData(containerToken,&spec,sizeof(spec));
				if (!(err=GetTOCByFSS(&spec,&tocH)))
				{
					for (mNum=0;mNum<(*tocH)->count;mNum++)
					{
						if ((*tocH)->sums[mNum].uidHash==id)
						{
							token.spec = spec;
							token.index = mNum;
							break;
						}
					}
					if (mNum>=(*tocH)->count) err = errAENoSuchObject;
				}
				break;
			default:
				err = errAENoSuchObject;
				break;
		}
	if (!err) err = AECreateDesc(cEuMessage,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindAttachment - access an attachment object
 ************************************************************************/
pascal OSErr FindAttachment(DescType desiredClass, AEDescPtr containerToken,
		DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
		long refCon)
{
#pragma unused(desiredClass,refCon)
	AttachmentToken token;
	FSSpec spec;
	short err=noErr;
	short mNum;
	TOCHandle tocH;
	Str255	name;
	
	if (containerClass==typeNull)		/* choose current message */
	{
		err = CurrentMessage(&tocH,&mNum);
		if (!err)
		{
			token.messT.spec = GetMailboxSpec(tocH,mNum);
			token.messT.index = mNum;
		}
	}
	else
	{
		MessHandle	messH;
		
		AEGetDescData(containerToken,&token.messT,sizeof(token.messT));
		switch (keyForm)
		{
			case formAbsolutePosition:
				token.index = GetAELong(keyData);
				break;
			
			case formName:
				if (!(err = OpenMessByToken(&token.messT,false,&messH)))
				{
					WindowPtr	messWinWP = GetMyWindowWindowPtr ((*messH)->win);
					
					err = errAENoSuchObject;
					GetAEPStr(name,keyData);
					for (token.index=1;GetAttachmentFromMsg(messH,token.index,&spec);token.index++)
					{
						if (StringSame(name,spec.name))
						{
							err = noErr;
							break;
						}
					}
					
					/*
					 * cleanup message
					 */
					if (!IsWindowVisible (messWinWP)) CloseMyWindow(messWinWP);
				}
				break;				
		}


	}
	if (!err) err = AECreateDesc(cEuAttachment,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindBoxInMFolder - access a mailbox object from a particular folder
 ************************************************************************/
pascal OSErr FindBoxInMFolder(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	VDId folder;
	FSSpec spec;
	short err=noErr;
	Str255 name;
	FInfo info;
	long index;
	
	AEGetDescData(containerToken,&folder,sizeof(folder));
	switch (keyForm)
	{
		case formName:
			GetAEPStr(name,keyData);
			if (!(err=FSMakeFSSpec(folder.vRef,folder.dirId,name,&spec)))
				err = FSpGetFInfo(&spec,&info);
			break;
		case formAbsolutePosition:
			index = GetAELong(keyData);
			err = FindBoxByIndex(folder.vRef,folder.dirId,index,&spec);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuMailbox,&spec,sizeof(spec),theToken);
	return(err);
}

/************************************************************************
 * FindBox - access a mailbox object from the top level
 ************************************************************************/
pascal OSErr FindBox(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	FSSpec spec;
	short err=noErr;
	Str255 name;
	FInfo info;
	long index;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(name,keyData);
			if (!(err=BoxSpecByName(&spec,name)))
				err = FSpGetFInfo(&spec,&info);
			break;
		case formAbsolutePosition:
			index = GetAELong(keyData);
			err = FindBoxByIndex(MailRoot.vRef,MailRoot.dirId,index,&spec);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuMailbox,&spec,sizeof(spec),theToken);
	return(err);
}

/************************************************************************
 * FindWin - access a window object from the null container
 ************************************************************************/
pascal OSErr FindWin(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,containerToken,refCon)
	MyWindowPtr win;
	WindowPtr	winWP;
	short n;
	short err = noErr;
	Str255 name, title;
	
	switch (keyForm)
	{
		case formUniqueID:
			n = GetAELong(keyData);
			win = GetNthWindow(n);
			winWP = GetMyWindowWindowPtr (win);
			if (winWP) break;
			
		case formAbsolutePosition:
			n = GetAELong(keyData);
			for (winWP = FrontWindow_(); --n && winWP; winWP = GetNextWindow (winWP));
				if (winWP == (WindowPtr)n) break;
			if (winWP) break;
		
		case formName:
			GetAEPStr(name,keyData);
			
			/*
			 * search open windows
			 */
			for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow (winWP))
			{
				GetWTitle(winWP,title);
				if (StringSame(title,name)) break;
			}
			
			/*
			 * special windows
			 */
			if (!winWP)
			{
				EnableMenuItems(False);
				for (n=WIN_BAR_ITEM-1;n>WIN_BAR2_ITEM;n--)
				{
					MyGetItem(GetMHandle(WINDOW_MENU),n,title);
					if (StringSame(title,name))
					{
						winWP = (WindowPtr)n;
						break;
					}
				}
			}
			if (winWP) break;
			
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cWindow,&winWP,sizeof(WindowPtr),theToken);
	return(err);
}


/************************************************************************
 * FindWinTE - find the (numbered) te of a window
 ************************************************************************/
pascal OSErr FindWinTE(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	TEToken token;
	WindowPtr	tokenWP;
	short err=noErr;
	short n;
	
	switch (keyForm)
	{
		case formAbsolutePosition:
			n = GetAELong(keyData);
			AEGetDescData(containerToken,&tokenWP,sizeof(tokenWP));
			token.win = GetWindowMyWindowPtr (tokenWP);
			if (n==1 && token.win && (long)token.win>10 && token.win->windex && token.win->pte)
			{
				token.pte = token.win->pte;
				break;
			}
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuTEInWin,&token,sizeof(token),theToken);
	return(err);
}


/************************************************************************
 * FindMFolderInMFolder - access a folder object from a particular folder
 ************************************************************************/
pascal OSErr FindMFolderInMFolder(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	VDId parFolder;
	VDId myFolder;
	short err=noErr;
	CInfoPBRec info;
	Str63 name;
	long index;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(name,keyData);
			AEGetDescData(containerToken,&parFolder,sizeof(parFolder));
			err = HGetCatInfo(parFolder.vRef,parFolder.dirId,name,&info);
			if (!err)
			{
				if (!info.hFileInfo.ioFlAttrib&0x10) err = errAENoSuchObject;
				else
				{
					myFolder.vRef = info.hFileInfo.ioVRefNum;
					myFolder.dirId = info.hFileInfo.ioDirID;
				}
			}
			break;
		case formAbsolutePosition:
			index = GetAELong(keyData);
			err = FindFolderByIndex(containerToken,index,&myFolder);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuMailfolder,&myFolder,sizeof(myFolder),theToken);
	return(err);
}

/************************************************************************
 * FindURLHelper - Find a given URL helper
 ************************************************************************/
pascal OSErr FindURLHelper(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass,containerToken)
	OSErr err=noErr;
	Handle res=nil;
	Str63 name;
		
	switch (keyForm)
	{
		case formUniqueID:
			res = GetResource(URLMAP_TYPE,GetAELong(keyData));
			break;
		case formAbsolutePosition:
			res = GetIndResource(URLMAP_TYPE,GetAELong(keyData));
			break;
		case formName:
			GetAEPStr(name,keyData);
			FindURLApp(name,(void*)&res,false);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!res) err = errAENoSuchObject;
	if (!err) err = AECreateDesc(cEuHelper,&res,sizeof(res),theToken);
	return(err);
}

/************************************************************************
 * AECreateURLHelper - create an url helper
 ************************************************************************/
OSErr AECreateURLHelper(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	Handle res = NuHandle(0);
	short id;
	AEDesc obj, desc, root;
	OSErr err;
	long	longId;
	
	NullADList(&obj,&desc,&root,nil);
	
	// add the resource
	if (!res) return(MemError());
	AddResource(res,URLMAP_TYPE,id=MyUniqueID(URLMAP_TYPE),"");
	if (err=ResError())
	{
		ZapHandle(res);
		return(err);
	}
	
	// tell the caller about it
	longId = id;
	if (!(err = AECreateDesc(typeLongInteger,&longId,sizeof(longId),&desc)))
	{
		if (!(err=CreateObjSpecifier(cEuHelper,&root,formUniqueID,&desc,false,&obj)))
			err = AEPutParamDesc(reply,keyAEResult,&obj);
	}

	DisposeADList(&obj,&desc,&root,nil);
	return(err);
}

/************************************************************************
 * SetURLHelperProperty - set properties of URL helpers
 ************************************************************************/
OSErr SetURLHelperProperty(AEDescPtr token,AEDescPtr descP)
{
	DescType property;
	Handle res;
	short err=noErr;
	Str255 str;
	short id;
	long junk;
	FSSpec spec;
	AliasHandle alias=nil;
	Handle	hProperty;
	
	if (err = AEGetDescDataHandle(token,&hProperty)) return err;	
	property = (*(PropTokenHandle)hProperty)->propertyId;
	res = *(Handle*)((*hProperty)+sizeof(PropToken));
	GetResInfo(res,&id,&junk,str);
	
	switch (property)
	{
		case pName:
			GetAEPStr(str,descP);
			SetResInfo(res,id,str);
			err = ResError();
			break;

		case cApplication:
			if (!(err=GetAEFSSpec(descP,&spec)))
			if (!(err=MakeHelperAlias(&spec,&alias)))
			if (!(err = SaveURLPref(str,alias)))
				alias = nil;
			ZapHandle(alias);
			break;
		
		case formUniqueID:
			return(errAENotModifiable);
			break;
		
		default:
			err = errAENoSuchObject;
			break;
	}
	AEDisposeDescDataHandle(hProperty);
	return(err);
}

/************************************************************************
 * FindPreference - Find a given preference string
 ************************************************************************/
pascal OSErr FindPreference(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass,containerToken)
	OSErr err=noErr;
	PrefToken token;
	
	Zero(token);
	
	switch (keyForm)
	{
		case formAbsolutePosition:
			token.pref = GetAELong(keyData);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
		
	if (!err) err = AECreateDesc(cEuPreference,&token,sizeof(token),theToken);
	return(err);
}

#ifdef TWO
/************************************************************************
 * FindFilter - Find a given filter
 ************************************************************************/
pascal OSErr FindFilter(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass,containerToken)
	FilterToken token;
	OSErr err = RegenerateFilters();
	
	if (err) return(err);
	
	token.form = keyForm;
	token.selector = GetAELong(keyData);
	
	switch (keyForm)
	{
		case formAbsolutePosition:
		case formUniqueID:
			err = FilterExists(token.form,token.selector) ? noErr : errAENoSuchObject;
			break;
		default:
			err = errAEEventNotHandled;
			break;
	}
	
	FiltersDecRef();
		
	if (!err) err = AECreateDesc(cEuFilter,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindTerm - Find a given term
 ************************************************************************/
pascal OSErr FindTerm(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass,containerToken)
	TermToken token;
	OSErr err;
	
	AEGetDescData(containerToken,&token.filter,sizeof(token.filter));
	
	switch (keyForm)
	{
		case formAbsolutePosition:
			token.term = GetAELong(keyData)-1;
			err = (token.term==0 || token.term==1) ? noErr : errAENoSuchObject;
			break;
		default:
			err = errAEEventNotHandled;
			break;
	}
		
	if (!err) err = AECreateDesc(cEuFilterTerm,&token,sizeof(token),theToken);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
pascal OSErr FindPersonality(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass,containerToken)
	PersHandle pers;
	OSErr err;
	short i;
	Str255 name;
	uLong id;
	
	switch (keyForm)
	{
		case formAbsolutePosition:
			i = GetAELong(keyData)-1;
			for (pers=PersList;pers && i;i--,pers=(*pers)->next);
			err = pers ? noErr : errAENoSuchObject;
			break;

		case formName:
			GetAEPStr(name,keyData);
			pers = FindPersByName(name);
			err = pers ? noErr : errAENoSuchObject;
			break;

		case formUniqueID:
			id = GetAELong(keyData)-1;
			pers = FindPersById(id);
			err = pers ? noErr : errAENoSuchObject;
			break;

		default:
			err = errAEEventNotHandled;
			break;
	}
		
	if (!err) err = AECreateDesc(cEuPersonality,&pers,sizeof(PersHandle),theToken);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
pascal OSErr FindPreferenceInPersonality(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon,containerClass,desiredClass)
	OSErr err=noErr;
	PrefToken token;
	
	Zero(token);
	
	switch (keyForm)
	{
		case formAbsolutePosition:
			token.pref = GetAELong(keyData);
			AEGetDescData(containerToken,&token.personality,sizeof(token.personality));
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
		
	if (!err) err = AECreateDesc(cEuPreference,&token,sizeof(token),theToken);
	return(err);
}

#endif

/************************************************************************
 * FindWTStuff - Access some chars from a TE
 ************************************************************************/
pascal OSErr FindWTStuff(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon)
	TERangeToken token;
	UPtr text;
	long len;
	long spot;
	short err = noErr;
	PETEHandle pte;
	Handle textH;
	Byte state;
	
	/*
	 * this may seem like it uses a lot of temporaries; that's
	 * because of bugs in the MPW C compiler :-(
	 */
	AEGetDescData(containerToken,&token.teT,sizeof(token.teT));
	pte = token.teT.pte;
	PeteGetTextAndSelection(pte,&textH,nil,nil);
	state = HGetState(textH);
	text = LDRef(textH);
	token.start = 0;
	token.stop = len = GetHandleSize(textH);
	
	if (keyForm==formRange)
	{
		err = MyTextRange(len,keyData,&token);
	}
	else
	{
		switch (keyForm)
		{
			case formAbsolutePosition:
				spot = GetAELong(keyData);
				break;
			case typeNull:
				break;
			default:
				err = errAENoSuchObject;
				break;
		}
		
		
		if (!err) switch (desiredClass)
		{
			case cChar:
				err = FindWTChar(text,spot,&token);
				break;
			case cWord:
				err = FindWTWord(text,spot,&token);
				break;
			case cText:
				token.stop = len;
				break;
		}
	}
	
	HSetState(textH,state);
	if (!err) err = AECreateDesc(cEuWTEText,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindWTNest - Access some chars from inside other chars
 ************************************************************************/
pascal OSErr FindWTNest(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(refCon)
	TERangeToken token;
	UPtr text;
	long len;
	long spot;
	short err = noErr;
	PETEHandle pte;
	Handle textH;
	Byte state;
	
	/*
	 * this may seem like it uses a lot of temporaries; that's
	 * because of bugs in the MPW C compiler :-(
	 */
	AEGetDescData(containerToken,&token,sizeof(token));
	pte = token.teT.pte;
	PeteGetTextAndSelection(pte,&textH,nil,nil);
	state = HGetState(textH);
	text = LDRef(textH);
	len = GetHandleSize(textH);
	
	if (keyForm==formRange)
	{
		err = MyTextRange(len,keyData,&token);
	}
	else
	{
		switch (keyForm)
		{
			case formAbsolutePosition:
				spot = GetAELong(keyData);
				break;
			case typeNull:
				break;
			default:
				err = errAENoSuchObject;
				break;
		}
		
		
		if (!err) switch (desiredClass)
		{
			case cChar:
				err = FindWTChar(text,spot,&token);
				break;
			case cWord:
				err = FindWTWord(text,spot,&token);
				break;
			case cText:
			
				break;
		}
	}
	
	HSetState(textH,state);
	if (!err) err = AECreateDesc(cEuWTEText,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindField - access a field object from a message
 ************************************************************************/
pascal OSErr FindField(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	FieldToken token;
	short err;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(token.name,keyData);
			AEGetDescData(containerToken,&token.messT,sizeof(token.messT));
			token.isNick = false;
			err = noErr;	/* cheating; we'll report errors if the field is used and not present */
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuField,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * FindMFolder - access a folder object
 ************************************************************************/
pascal OSErr FindMFolder(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerToken,containerClass,refCon)
	short err=noErr;
	Str255 name;
	VDId vdid;
	FSSpec spec;
	CInfoPBRec hfi;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(name,keyData);
			if (!*name)
			{
				vdid.vRef = MailRoot.vRef;
				vdid.dirId = MailRoot.dirId;
			}
			else
			{
				if (!(err=FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,name,&spec)) ||
					!(err=FSMakeFSSpec(IMAPMailRoot.vRef,IMAPMailRoot.dirId,name,&spec)))
				{
					if (!(err=AFSpGetCatInfo(&spec,&spec,&hfi)))
					{
						if (VD2MenuId(hfi.hFileInfo.ioVRefNum,hfi.hFileInfo.ioDirID)!=-1)
						{
							vdid.vRef = hfi.hFileInfo.ioVRefNum;
							vdid.dirId = hfi.hFileInfo.ioDirID;
						}
						else
							err = errAENoSuchObject;
					}
				}
			}
			break;
		case formAbsolutePosition:
			err = FindFolderByIndex(nil,GetAELong(keyData),&vdid);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuMailfolder,&vdid,sizeof(vdid),theToken);
	return(err);
}


/************************************************************************
 * AECountStuff - count stuff
 ************************************************************************/
pascal OSErr AECountStuff(DescType desiredClass,DescType containerClass,AEDescPtr containerToken,long *howMany)
{
	OSErr err=errAENoSuchObject;
	
	switch (containerClass)
	{
		case cEuNickFile:
			err = CountNickFileElements(desiredClass,containerToken,howMany);
			break;
		case cEuMailfolder:
			err = CountMailfolderElements(desiredClass,containerToken,howMany);
			break;
		case cEuMailbox:
			err = CountMailboxElements(desiredClass,containerToken,howMany);
			break;
		case cEuMessage:
			err = CountMessageElements(desiredClass,containerToken,howMany);
			break;
		case typeNull:
			switch(desiredClass)
			{
				case cEuNickFile:
					err = CountNickFiles(desiredClass,containerToken,howMany);
					break;
				case cWindow:
					*howMany = CountWindows();
					err = noErr;
					break;
				case cEuHelper:
					*howMany = CountURLHelpers();
					err = noErr;
					break;
				case cEuPersonality:
					*howMany = PersCount();
					err = noErr;
					break;
#ifdef TWO
				case cEuFilter:
					err = CountFilters(howMany);
					break;
#endif
			}
	}
	return(err);
}


/************************************************************************
 * GetAttachmentFromMsg - get attachment from message
 ************************************************************************/
Boolean GetAttachmentFromMsg(MessHandle mess,short index,FSSpec *spec)
{
	TOCHandle tocH = (*mess)->tocH;
	MSumType *sum;
	short	mNum;
	
	if (!tocH) return false;
	
	mNum = (*mess)->sumNum;
	sum=(*tocH)->sums+mNum;
	if ((*tocH)->which==OUT || sum->state==SENT || sum->state==UNSENT || sum->flags & FLAG_OUT)
		// Outgoing message. Easy.
		return 1!=GetIndAttachment(mess,index,spec,nil);
	else
	{
		// Received message
		long	offset;
		short	count;
		Handle text;
		
		CacheMessage(tocH,mNum);
		if (text=(*tocH)->sums[mNum].cache)
		{
			Boolean	found = false;
			
			offset = (*tocH)->sums[mNum].bodyOffset-1;
			for (count=1;count<=index && 0<=(offset = FindAnAttachment(text,offset+1,spec,true,nil,nil,nil));count++)
				if (count==index)
					found = true;
			return found;
		}
	}
	return false;
}

/************************************************************************
 * HandleEuConduitAction - handle events for Eudora Conduits.
 *	These will not be exposed.
 ************************************************************************/
pascal OSErr HandleEuConduitAction(AppleEvent *event,AppleEvent *reply,long refCon)
{
	OSErr err;
	long command;
	DescType dataType;
	Size dataSize;
	long junk;
	Ptr pPtr = nil;
		
	FRONTIER;	

	// determine which command was sent ...
	err = AEGetParamPtr_(event,cEuPalmConduitCommand,typeLongInteger,&dataType,&command,sizeof(long),&dataSize);
	if (err == noErr)
	{
		// determine the parameters ...
		err = AEGetParamPtr_(event,cEuPalmConduitData,typeLongInteger,&dataType,&junk,sizeof(long),&dataSize);
		if (err == noErr)
		{
			if (dataSize)
			{
				pPtr = NuPtr(dataSize);
				err = MemError();
				if ((err==noErr) && pPtr)
				{
					// grab the paramters
					err = AEGetParamPtr_(event,cEuPalmConduitData,typeLongInteger,&dataType,pPtr,dataSize,&junk);
				}
			}
			
			if (err == noErr)
			{
				switch (command)
				{
					// return settings to the conduit
					case kPCCGetSettings:
						err = ABConduitGetSettingsFile(reply, refCon);
						break;
					
					// close open address books
					case kPCCCheckAddressBooks:
						err = ABConduitCanSyncAddressBooks(reply, pPtr, dataSize);
						break;
						
					// backup changed address books
					case kPCCBackupChangedAddressBooks:
						err = ABConduitBackupAddressBooks(reply, (FSSpecPtr)(pPtr), dataSize);
						break;
						
					// reopen all the address books
					case kPCCReloadAddressBooks:
						err = ABConduitReopenAddressBooks(reply);
						break;
						
					// generrate a unique ID for a new nickname
					case kPCCGenerateUniqueID:
						err = ABconduitGenerateUniqueID(reply, pPtr, dataSize);
						break;
						
					default:
						err = errAEEventNotHandled;
						break;
				}
			}
			
			ZapPtr(pPtr);
		}
	}

	return(err);
}