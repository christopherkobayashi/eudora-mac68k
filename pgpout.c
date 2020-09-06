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

#define FILE_NUM 64
/* Copyright (c) 1994 by QUALCOMM Incorporated */
/************************************************************************
 * writing pgp
 ************************************************************************/

#pragma segment PGP
#include "pgpout.h"

#ifdef NEVER
OSErr PGPExtract(ProcessSerialNumber *psn,PStr address,FSSpecPtr into,Boolean armor,Boolean fromPrivate);
OSErr PGPPrivateRing(ProcessSerialNumber *psn,FSSpecPtr privateRing);
OSErr PGPEncrypt(MessHandle messH,FSSpecPtr from,Boolean encrypt,Boolean sign,FSSpecPtr result);
OSErr SendPGPEncrypted(TransStream stream,MessHandle messH,FSSpecPtr spec);

/**********************************************************************
 * PGPSendMessage - send a message with PGP
 **********************************************************************/
short PGPSendMessage(TransStream stream,MessHandle messH,Boolean chatter)
{
	FSSpec raw;
	OSErr err;
	short refN;
	long flags = SumOf(messH)->flags;
	FSSpec encrypted;
		
	if (!(err=NewTempSpec(Root.vRef,Root.dirId,nil,&raw)))
	if (!(err=UniqueSpec(&raw,31)))
	if (!(err=FSpCreate(&raw,CREATOR,'TEXT',smSystemScript)))
	{
		{
			if (!(err=FSpOpenDF(&raw,fsRdWrPerm,&refN)))
			{
				err = WriteMailFile(messH,refN,False,nil);
				MyFSClose(refN);
				
				if (!err)
				{
					if (!(err = PGPEncrypt(messH,&raw,0!=(flags&FLAG_ENCRYPT),0!=(flags&FLAG_SIGN),&encrypted)))
					{
						err = SendPGPEncrypted(stream,messH,&encrypted);
						FSpDelete(&encrypted);
					}		
				}
			}
		}
		PrefIsSet(PREF_WIPE) ? WipeSpec(&raw):FSpDelete(&raw);
	}
	if (err) WarnUser(GENERAL,err);
	return(err);
}

/**********************************************************************
 * SendPGPEncrypted - send a pre-encrypted file
 **********************************************************************/
OSErr SendPGPEncrypted(TransStream stream,MessHandle messH,FSSpecPtr spec)
{
	Str31 subject;
	Str31 shortName;
	OSErr err;
	long got=0;
	HeadSpec hs;
	
	if (!CompHeadGetTextPtr(TheBody,CompHeadFind(messH,SUBJ_HEAD,&hs),0,subject+1,sizeof(subject)-2,&got))
		*subject = got;
	if (!got) GetRString(subject,PGP_PROTOCOL);
	Mac2OtherName(shortName,subject);
	
	/*
	 * send our MIME headers
	 */
	if (err = ComposeRTrans(stream,MIME_V_FMT,InterestHeadStrn+hMimeVersion,MIME_VERSION,NewLine))
		return(err);
	if (err = ComposeRTrans(stream,MIME_MP_FMT,
								 InterestHeadStrn+hContentType,
								 MessFlagIsSet(messH,FLAG_ENCRYPT) ? MIME_APPLICATION : MIME_TEXT,
								 PGP_PROTOCOL,
								 AttributeStrn+aName,
								 shortName,
								 NewLine))
		return(err);
	if (err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
								 AttributeStrn+aFormat,
								 GetRString(subject,MIME),
								 NewLine))
		return(err);
	if (err = ComposeRTrans(stream,MIME_CD_FMT,
								 InterestHeadStrn+hContentDisposition,
								 MessFlagIsSet(messH,FLAG_ENCRYPT) ? ATTACHMENT : INLINE,
								 AttributeStrn+aFilename,
								 shortName,
								 NewLine))
		return(err);
	
	/*
	 * send rest of headers
	 */
	if (err = TransmitHeaders(stream,messH,nil,nil,True,True,nil)) return(err);
	
	/*
	 * header-body separator
	 */
	if (err = SendPString(stream,NewLine)) return(err);

	/*
	 * now, send the rest of the file
	 */
	if (err = SendTextFile(stream,spec,0,nil)) return(err);
	
	if (!UUPCOut) err = FinishSMTP(stream);
	
	return(err);
}

/**********************************************************************
 * HaveKeyFor - do we have a key for this person?
 **********************************************************************/
Boolean HaveKeyFor(PStr address,Boolean fromPrivate)
{
	ProcessSerialNumber psn;
	OSErr err;
	Str255 cmd;
	Str255 shortAddr;
	AppleEvent reply;
	
	if (StartPGP(&psn)) return(False);
	
	NullADList(&reply,nil);
	ShortAddr(shortAddr,address);
	ComposeRString(cmd,PGP_HAVEKEY_FMT,shortAddr);
	err = SimpleAESend(&psn,kPGPClass,kPGPExecute,&reply,kEAEImmediate,
					keyDirectObject,typeChar,cmd+1,*cmd,
					nil,nil);
	
	if (!err) err = GetAEError(&reply);
	DisposeADList(&reply,nil);
	
	return(!err);
}

/************************************************************************
 * HaveAllKeys - do we have all the keys in a message?
 ************************************************************************/
Boolean HaveAllKeys(MessHandle messH)
{
	static short fields[] = {TO_HEAD,FROM_HEAD,BCC_HEAD,CC_HEAD};
	short i;
	Str255 missing;
	HeadSpec hs;
	UHandle text=nil;
	
	/*
	 * check for missing keys
	 */
	for (i=0;i<sizeof(fields)/sizeof(short);i++)
	{
		if (!CompHeadFind(messH,fields[i],&hs) || CompHeadGetText(TheBody,&hs,&text))
		{
			WarnUser(PETE_ERR,1);
			return(False);
		}
		else if (!HaveKeysFrom(text,missing,fields[i]==FROM_HEAD))
			break;
		ZapHandle(text);
	}
	ZapHandle(text);
	
	/*
	 * error?
	 */
	if (*missing)
	{
		Aprintf(OK_ALRT,Note,fields[i]==FROM_HEAD?NO_ME_KEY:NO_KEY_FMT,missing);
		return(False);
	}

	return(True);
}

/************************************************************************
 * HaveKeysFrom - Do we have the keys for addresses from a given bit of text?
 ************************************************************************/
Boolean HaveKeysFrom(Handle text,PStr missing,Boolean fromPrivate)
{
	UHandle addresses=nil;
	UHandle rawAddresses=nil;
	UPtr address;
	Boolean have = True;

	*missing = 0;
	if (!SuckAddresses(&rawAddresses,text,True,True,True,nil))
	{
		if (**rawAddresses)
		{
			ExpandAliases(&addresses,rawAddresses,0,False);
			ZapHandle(rawAddresses);
			if (!addresses) return(False);
			for (address=LDRef(addresses); *address; address += *address + 2)
			{
				if (!HaveKeyFor(address,fromPrivate))
				{
					PCopy(missing,address);
					break;
				}
			}
			ZapHandle(addresses);
		}
		else
			ZapHandle(rawAddresses);
	}
	return(*missing==0);
}

/**********************************************************************
 * StartPGP - start up PGP
 **********************************************************************/
OSErr StartPGP(ProcessSerialNumber *psn)
{
	Str31 proto;
	AliasHandle app;
	OSErr err;
	
	GetRString(proto,PGP_PROTOCOL);
	if (!(err=FindURLApp(proto,&app,kNoWildCard)))
	{
		if (FindPSNByAlias(psn,app))
		{
			if (err=LaunchByAlias(app)) return(err);
			err = FindPSNByAlias(psn,app);
		}
	}
	
#ifdef NEVER
	/*
	 * viacrypt won't work in the bg
	 */
	if (!err)
	{
		SetFrontProcess(psn);
		InBG = True;
	}
#endif
	return(err);
}

/**********************************************************************
 * PGPEncrypt - encrypt a file with PGP
 **********************************************************************/
OSErr PGPEncrypt(MessHandle messH,FSSpecPtr from,Boolean encrypt,Boolean sign,FSSpecPtr spec)
{
	Handle raw = NuHandle(0);
	Handle expanded = nil;
	OSErr err;
	ProcessSerialNumber psn;
	Str255 missing, shortUser;
	Handle user=nil;
	AppleEvent reply;
	AEDescList addressList, resultList;
	HeadSpec hs;
	
	NullADList(&reply,&addressList,&resultList,nil);
	
	if (err=CompHeadGetText(TheBody,CompHeadFind(messH,FROM_HEAD,&hs),&user))
		return(WarnUser(PETE_ERR,err));
	
	/*
	 * gather up the recpients
	 */
	if (!(err = GatherCompAddresses((*messH)->win,raw)))
	{
		ExpandAliases(&expanded,raw,0,True);
		ZapHandle(raw);
		if (expanded)
		{
			err = BuildAddressList(expanded,&addressList);
			ZapHandle(expanded);
		}
		else err = 1;
	}
		
	/*
	 * ViaCrypt won't work in the background
	 */
	if (!err && !(err=StartPGP(&psn)))
	{
		/*
		 * Do we have the keys?
		 */
		if (!err && !HaveKeysFrom(user,missing,True))
		{
			err = fnfErr;
			Aprintf(OK_ALRT,Note,NO_ME_KEY,missing);
		}
		else
		{
			if (encrypt && !HaveAllKeys(messH)) err = fnfErr;
			else
			{
				LDRef(user);
				MakePStr(missing,*user,GetHandleSize(user));
				UL(user);
				ShortAddr(shortUser,missing);
				if (encrypt)
					err = SimpleAESend(&psn,kPGPClass,kPGPEncrypt,&reply,kEAEImmediate,
									keyDirectObject,typeFSS,from,sizeof(FSSpec),
									keyPGPToUser,keyEuDesc,&addressList,
									keyPGPReadMode,typeEnumeration,ePGPReadText,-4,
									keyPGPSignature,typeEnumeration,ePGPSigIncluded,-4,
									keyPGPFromUser,typeChar,shortUser+1,*shortUser,
									keyPGPOutput,typeEnumeration,ePGPWriteAscii,-4,
									nil,nil);
				else
					err = SimpleAESend(&psn,kPGPClass,kPGPSign,&reply,kEAEImmediate,
									keyDirectObject,typeFSS,from,sizeof(FSSpec),
									keyPGPReadMode,typeEnumeration,ePGPReadText,-4,
									keyPGPSignature,typeEnumeration,ePGPSigClear,-4,
									keyPGPFromUser,typeChar,LDRef(user),GetHandleSize(user),
									keyPGPOutput,typeEnumeration,ePGPWriteAscii,-4,
									nil,nil);
				
				if (!err) err = PGPFetchResult(&reply,spec);
				
				if (err) WarnUser(AE_TROUBLE,err);
			}
		}
#ifdef PGPNOBG
		/*
		 * give ViaCrypt time to finish its thing, then restore front process
		 */
		Pause(30);
#endif
	}
	
	ZapHandle(expanded);
	ZapHandle(raw);
	DisposeADList(&reply,&addressList,&resultList,nil);
	ZapHandle(user);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PGPFetchResult(AppleEvent *reply, FSSpecPtr spec)
{
	AEDescList resultList;
	long count;
	OSErr err;
	long junk;
	long size;
	
	NullADList(&resultList,nil);
	if (!(err = GetAEError(reply)))
	if (!(err = AEGetParamDesc(reply,keyPGPOutput,typeAEList,&resultList)))
	if (!(err = AECountItems(&resultList,&count)))
	{
		if (!count) err = fnfErr;
		else err = AEGetNthPtr(&resultList,1,typeFSS,&junk,&junk,spec,sizeof(*spec),&size);
	}
	DisposeADList(&resultList,nil);
	
	return(err);
}

/**********************************************************************
 * BuildAddressList - build an AE list of addresses
 **********************************************************************/
OSErr BuildAddressList(Handle addresses,AEDescList *list)
{
	OSErr err;
	UPtr spot;
	Str255 shortAddr;
	
	if (!(err = AECreateList(nil,0,False,list)))
	{
		for (spot=LDRef(addresses);*spot;spot += *spot+2)
		{
			ShortAddr(shortAddr,spot);
			if (err = AEPutPtr(list,0,typeChar,shortAddr+1,*shortAddr)) break;
		}
		UL(addresses);
	}
	if (err) WarnUser(AE_TROUBLE,err);
	return(err);
}

/**********************************************************************
 * PGPExtract - extract a key from PGP
 **********************************************************************/
OSErr PGPExtract(ProcessSerialNumber *psn,PStr address,FSSpecPtr into,Boolean armor,Boolean fromPrivate)
{
	AppleEvent reply;
	OSErr err;
	FSSpec ring;
	
	NullADList(&reply,nil);
	
	if (fromPrivate && (err = PGPPrivateRing(psn,&ring)))
		return(WarnUser(AE_TROUBLE,err));
	
	if (!(err=SimpleAESend(psn,kPGPClass,kPGPExtract,&reply,kEAEImmediate,
							keyDirectObject,typeChar,address+1,*address,
							keyPGPToUser,typeFSS,into,sizeof(FSSpec),
							keyPGPKeyRing,fromPrivate?typeFSS:typeNull,&ring,sizeof(FSSpec),
							nil,nil)))
	{
		err = GetAEError(&reply);
		if (err) WarnUser(AE_TROUBLE,err);
	}
		
	DisposeADList(&reply,nil);
	
	return(err);
}


/**********************************************************************
 * PGPPrivateRing - get the name of our private keyring
 **********************************************************************/
OSErr PGPPrivateRing(ProcessSerialNumber *psn,FSSpecPtr privateRing)
{
	ProcessInfoRec pi;
	OSErr err;
	
	Zero(pi);
	pi.processInfoLength = sizeof(pi);
	pi.processAppSpec = privateRing;
	err = GetProcessInformation(psn,&pi);
	
	GetRString(privateRing->name,SECRET_KEYRING);
	return(noErr);
}

#endif
