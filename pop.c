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

#include "pop.h"
#include "myssl.h"
#define FILE_NUM 30
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/*
5/29/97 cwong
NOTE: 

You should use the following macros when accessing POPD resources in the settings file:

	 GetResourceMainThread_ 
	 ZapSettingsResourceMainThread 
	 AddMyResourceMainThread_

They access the main thread's Settings file (not the background thread's copy.)
*/

/************************************************************************
 * functions for dealing with a pop 2 server
 ************************************************************************/

#ifdef	KERBEROS
#include				<krb.h>
#endif
#pragma segment POP

#define CMD_BUFFER 514
#define PSIZE (UseCTB ? 256 : 4096)
#define errNotFound -2

#define MatchPOPD(old,oldSpot,hash)	\
		((hash) && (*(old))[oldSpot].uidHash==(hash))

#define POP_TERM(buffer,size) ((size)==2 && (buffer)[0]=='.' && (buffer)[1]=='\015')
/************************************************************************
 * private routines
 ************************************************************************/
void POPDelDup(POPDHandle popDH);
OSErr POPPreFetch(TransStream stream,POPDHandle popDH,short message,Boolean *capabilities);
int POPGetReplyLo(TransStream stream,short cmd, UPtr buffer, long *size,AccuPtr resAcc);
#define POPGetReply(stream,cmd,buffer,size) POPGetReplyLo(stream,cmd,buffer,size,nil)
void RelatedNote(FSSpecPtr spec,HeaderDHandle hdh,PStr theMessage);
	int POPByeBye(TransStream stream);
	int POPCmdLo(TransStream stream, short cmd, UPtr args, AccuPtr argsAcc);
#define POPCmd(stream,cmd,args) POPCmdLo(stream,cmd,args,nil)
	int POPGetMessage(TransStream,long messageNumber,short *gotSome,POPDHandle popDH,Boolean *capabilities);
	int DupHeader(short refN,UPtr buff,long bSize,long offset,long headerSize);
	int SaveAndSplit(TransStream stream,short refN,long estSize,HeaderDHandle *hdhp,Boolean isIMAP);
	Boolean StackLowErr = false;
	Boolean PopConnected;
	int FirstUnread(TransStream stream,int count);
	Boolean HasBeenRead(TransStream stream,short msgNum,short count);
	void StampPartNumber(MSumPtr sum,short part,short count);
	UPtr ExtractStamp(UPtr stamp,UPtr banner);
	short POPLast(TransStream,short *lastRead);
	POPLineType ReadPlainBody(TransStream stream,short refN,char *buf,long bSize,long estSize);
	short SplitMessage(short refN, long hStart, long hEnd, long msgEnd);
	void DisposePOPD(POPDHandle *popDH);
	OSErr BuildPOPD(TransStream stream,POPDHandle *popDH,short count,XferFlags *flags,Boolean *capabilities);
  void FillPOPD(POPDPtr pdp,HeaderDHandle hdh);
	short CountFetch(POPDHandle popDH);
	PStr HeaderMsgId(HeaderDHandle hdh,PStr msgId);
	uLong FakeMIDHash(HeaderDHandle hdh);
	void SetFetchDel(POPDHandle popDH,short from, short to,Boolean fetch, Boolean delete);
	void SetFetched(POPDHandle popDH,short from, short to);
	void SetBeforeAfter(POPDHandle popDH,uLong gmt,short *after,short *before);
	OSErr POPMsgSize(short messageNumber,long *msgsize);
	short FindExistSpot(POPDHandle popDH,uLong hash);
	OSErr DeletePOPMessage(TransStream stream,short number,long uidHash);
	OSErr FillWithUidl(TransStream stream,POPDHandle popDH);
	OSErr FillWithTop(TransStream stream,POPDHandle new, POPDHandle old);
OSErr FillSizesWithList(TransStream stream,POPDHandle popDH);
	short FindUndelete(POPDHandle popDH,uLong gmt,uLong hash);
	OSErr FillPOPDFromServer(TransStream stream,POPDHandle popDH,short spot);

	OSErr InitKerberos();
	OSErr KerbGetTicket(PStr popName,PStr host,PStr realm,PStr version,UHandle *ticket);
	OSErr SendPOPTicket(TransStream stream);
	void LogPOPD(PStr intro,POPDHandle newDH);
	void Log1POPD(PStr intro, PStr which, POPDHandle popDH);
	Boolean NoClearPass(Boolean *capabilities,UPtr response, short len);
	void PrunePOPD(OSType listType,short listId, POPDHandle onServer);
OSErr ReapCmds(TransStream stream, short cmd);
void PopCapabilities(TransStream stream, Boolean *capabilities,SASLEnum *mechPtr);
int POPSasl(TransStream stream, Boolean *capabilities, SASLEnum mech, UPtr buffer, long *size);
OSErr FixLongFilename(HeaderDHandle hdh,FSSpecPtr spec);
PStr Un2184Append(PStr dest,short sizeofDest,PStr orig,PStr charset,Boolean isEncoded);
PStr Un2184(PStr dest, PStr orig, PStr charset);

/* stack sniffer defines */

// 5k seems to work for ppc. may need to tweak it some more. s/b smaller for 68k?
#define kLowStackSize ((GetCurrentISA() == kPowerPCISA) ? (5 K) : (4 K))
#define kMoreStackSpace (10 K)

/* Globals */

Boolean gPOPKerbInited = false;		// true when Kerberos has been initialized for POP
KClientSessionInfo gSession;		// session info
KClientKey gPrivateKey;				// private key


/************************************************************************
 * GetMyMail - the biggie; transfers mail into In mailbox
 ************************************************************************/
short GetMyMail(TransStream stream,Boolean quietly,short *gotSome,struct XferFlags *flags)
{
#pragma unused(quietly)
	int messageCount;
	Str255 msgname;
	Str63 hostName;
	long port;
	TOCHandle tocH;
	short err;
	short fetchCount, message, fetched;
	POPDHandle popDH=nil;
	Boolean built = True;
	int beforeBytes,
			actualBytes,
			approxBytes;
#ifdef BATCH_DELIVERY_ON
	Boolean inThread=InAThread();
#endif
	Boolean capabilities[pcapaLimit+1];
	
	WriteZero(capabilities,sizeof(capabilities));	// we don't know if we have them yet!!!
	
	if (Prr=StackInit(sizeof(short),&POPCmds)) return(Prr);
	
	*gotSome = 0;
#ifdef ESSL
	stream->ESSLSetting = GetPrefLong(PREF_SSL_POP_SETTING);
	if(stream->ESSLSetting & esslUseAltPort)
		port = GetRLong(POP_SSL_PORT);
	else
#endif
		port = PrefIsSet(PREF_KERBEROS)?GetRLong(KERB_POP_PORT):GetRLong(POP_PORT);
	if (Prr=GetPOPInfoLo(msgname,hostName,&port)) return(Prr);
	if ((err=StartPOP(stream,hostName,port))==noErr)
	{
		messageCount = POPIntroductions(stream, msgname, capabilities);
#ifdef DEBUG
		if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif
		if (capabilities[0] && !capabilities[pcapaUIDL])
		{
			(*CurPers)->noUIDL = true;
			Log(LOG_LMOS,"\pCAPA says no UIDL");
		}
		if (!Prr)
		{
			if (messageCount==0)
			{
				FixServers = FixServers || nil!=GetResource_(CUR_POPD_TYPE,POPD_ID);
				ZapSettingsResourceMainThread_(CUR_POPD_TYPE,POPD_ID);
#ifdef TWO
				ZapSettingsResourceMainThread_(CUR_POPD_TYPE,DELETE_ID);
				ZapSettingsResourceMainThread_(CUR_POPD_TYPE,FETCH_ID);
#endif
			}
			else if (!BuildPOPD(stream,&popDH,messageCount,flags,capabilities))
			{
				CanPipeline = (capabilities[0] ? capabilities[pcapaPipelining] : PrefIsSet(PREF_CAN_PIPELINE))
										  && !(*CurPers)->noUIDL;
				
				ComposeLogR(LOG_RETR,nil,START_POP_LOG,hostName,port,messageCount);
				if (tocH=GetInTOC())
				{
					/*
					 * run through the pure deletes
					 */
					for (message=0;message<messageCount;message++)
						if ((*popDH)[message].delete && !(*popDH)[message].retr && !(*popDH)[message].stub)
						{
							Prr = DeletePOPMessage(stream,message,(*popDH)[message].uidHash);
							if (!Prr) (*popDH)[message].deleted = True;
							else break;
						}
					
					/*
					 * and now do the fetches
					 */
					if (!Prr)
					{
#ifdef BATCH_DELIVERY_ON
						short batchNum = GetRLong(DELIVERY_BATCH);
#endif
						fetchCount = CountFetch(popDH);
						if (CanPipeline) POPPreFetch(stream,popDH,0,capabilities);
						for (fetched=message=0;message<messageCount;message++)
							if ((*popDH)[message].retr || (*popDH)[message].stub)
							{
								ProgressR(NoChange,fetchCount-fetched,0,LEFT_TO_TRANSFER,nil);
								TOCSetDirty(tocH,true);
								beforeBytes = GetProgressBytes();
#ifdef DEBUG
								if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif
								if (CommandPeriod || POPGetMessage(stream,message,gotSome,popDH,capabilities)) break;
#ifdef DEBUG
								if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif
								fetched++;
								// adjust progress bar		
								actualBytes = GetProgressBytes() - beforeBytes;
								approxBytes = (*popDH)[message].stub ? (3 K) : (*popDH)[message].msgSize;
								if (actualBytes < approxBytes)
									ByteProgress(nil,actualBytes - approxBytes,0);
								else
									ByteProgressExcess(approxBytes - actualBytes);
								
								// delete this message if a translator requested it.
								if (ETLDeleteRequest) 
								{
									DeleteSum(tocH, (*tocH)->count-1);
									fetched--;
									ETLDeleteRequest = false;
								}
								
#ifdef BATCH_DELIVERY_ON
								if (inThread && (fetched % batchNum == 0))
								{
									tocH = RenameInTemp(tocH);
									// This is bad.  We don't know why this happens,
									// and if this codebase had a future, we would need
									// to find out.  But for now, we're just going to stop
									// the crashing and feel ashamed.  SD 5/2005
									if (!tocH) break;
#ifdef THIS_CODE_HAD_A_FUTURE
#error FIX ME!
#endif
								}
#endif
							}
#ifdef BATCH_DELIVERY_ON
							if (inThread)
								RenameInTemp(tocH);
#endif
					}
						
					if (CommandPeriod) Prr = userCancelled;
				}
				else
					Prr = 1;
				if (!Prr)
				{
					ProgressMessageR(kpSubTitle,CLEANUP_CONNECTION);
				}

				PrunePOPD(CUR_POPD_TYPE,DELETE_ID,popDH);
				PrunePOPD(CUR_POPD_TYPE,FETCH_ID,popDH);
				DisposePOPD(&popDH);
			}
			else /* popd build failed */
				ZapHandle(popDH);
		}
	}
	if (!err) err = Prr;
	if (!err && messageCount==0) ZapSettingsResourceMainThread_(CUR_POPD_TYPE,POPD_ID);
	ProgressMessageR(kpSubTitle,CLEANUP_CONNECTION);
	if (AttachedFiles) SetHandleBig_(AttachedFiles,0);
	err = Prr;
	EndPOP(stream);
	
	ZapHandle(POPCmds);
	return(err);
}

#ifdef BATCH_DELIVERY_ON
/**********************************************************************
 * RenameInTemp
 **********************************************************************/
TOCHandle RenameInTemp(TOCHandle tocH)
{
	Str63 name;
	FSSpec deliverSpec,
				inSpec,
				deliverFolder;
	FSSpec deliverTOCSpec, tocSpec;
	CInfoPBRec hfi;
	long maxFileNum=0, fileNum;
	OSErr err;
		
	if (!tocH || !(*tocH)->count)
		return tocH;
	if (err=SubFolderSpec(DELIVERY_FOLDER,&deliverFolder))
	{
		Aprintf(OK_ALRT,Note,THREAD_SUBFOLDER_ERR,DELIVERY_FOLDER,err);
		return tocH;
	}
		
	// make sure the toc is written
	if (TOCIsDirty(tocH) || (*tocH)->reallyDirty) 
		if (err=WriteTOC(tocH)) return tocH;

	/* find highest-numbered file in delivery folder */
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex=0;
	while(!DirIterate(deliverFolder.vRefNum,deliverFolder.parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==MAILBOX_TYPE)	
		{
			StringToNum(name, &fileNum);
			if (fileNum > maxFileNum)
				maxFileNum = fileNum;
		}
	}

	// Make name for new mailbox
	inSpec = GetMailboxSpec(tocH,-1);
	NumToString(maxFileNum+1, name);
	while (*name<6) PInsertC(name,sizeof(name),'0',name+1);
	FSMakeFSSpec(deliverFolder.vRefNum,deliverFolder.parID,name,&deliverSpec);


	// toc file?
	tocSpec = inSpec;
	PCatR(tocSpec.name,TOC_SUFFIX);
	if (!FSpExists(&tocSpec))
	{
		FSMakeFSSpec(deliverFolder.vRefNum,deliverFolder.parID,name,&deliverTOCSpec);
		PCatR(&deliverTOCSpec.name,TOC_SUFFIX);
	}
	else *tocSpec.name = 0;
	
	// Move files
	if ((*tocH)->win) CloseMyWindow(GetMyWindowWindowPtr((*tocH)->win));
	if (err=SpecMoveAndRename(&inSpec,&deliverSpec))
	{
		Aprintf(OK_ALRT,Note,THREAD_DELIVER_CREATE_ERR,deliverSpec.name,err);
	}
	else
	{
		if (*tocSpec.name)
		{
			if (err=SpecMoveAndRename(&tocSpec,&deliverTOCSpec))
			{
				Aprintf(OK_ALRT,Note,THREAD_DELIVER_CREATE_ERR,deliverTOCSpec.name,err);
				FSpDelete(&tocSpec);	// hell with it.  We can rebuild it
			}
		}
		
		// Ok, we have moved the temp.in.  Make a new one
		if (err=MakeResFile(inSpec.name,inSpec.vRefNum,inSpec.parID,CREATOR,MAILBOX_TYPE))
		{
			Aprintf(OK_ALRT,Note,THREAD_DELIVER_CREATE_ERR,inSpec.name,err);
			// this is bad
		}
		
		NeedToFilterIn++;	// some filtering to be done
	}

	tocH = GetTempInTOC();

	return tocH;
}
#endif

/**********************************************************************
 * POPPreFetch
 **********************************************************************/
OSErr POPPreFetch(TransStream stream, POPDHandle popDH,short message,Boolean *capabilities)
{
	short messageCount = HandleCount(popDH);
	Str63 args;
	Str15 top;
	OSErr err = noErr;
	short cmd;
	
	for (;message<messageCount;message++)
		if ((*popDH)[message].retr || (*popDH)[message].stub)
		{
			NumToString(message+1,args);
			if ((*popDH)[message].stub)
			{
			  NumToString(GetRLong(BIG_MESSAGE_FRAGMENT),top);
				PCatC(args,' ');
				PCat(args,top);
				if (capabilities[pcapaMangle] || capabilities[pcapaXMangle])
				{
					PCatC(args,' ');
					PCatR(args,POPCapaStrn+(capabilities[pcapaMangle]?pcapaMangle:pcapaXMangle));
					PCatR(args,MANGLE_ARGS);
				}
				cmd = kpcTop;
			}
			else cmd = kpcRetr;
			
			return(POPCmd(stream, cmd,args));
		}
	return(noErr);
}

/************************************************************************
 * POPrror - see if there was a POP error
 ************************************************************************/
int POPrror(void)
{
	return(Prr);
}
	
/************************************************************************
 * private routines
 ************************************************************************/
/************************************************************************
 * StartPOP - get connected to the POP server
 ************************************************************************/
int StartPOP(TransStream stream, UPtr serverName, long port)
{
	PopConnected=False;
	Prr=ConnectTrans(stream,serverName,port,False,GetRLong(OPEN_TIMEOUT));
	return(Prr);
}

/************************************************************************
 * EndPOP - get rid of the POP server
 ************************************************************************/
int EndPOP(TransStream stream)
{
	SilenceTrans(stream,True);
	if (CommandPeriod && TransError(stream)==userCancelled) POPCmd(stream,kpcQuit,nil);
	if (!Prr)
	{
		if (!CommandPeriod) (void) POPByeBye(stream);
		Prr=DisTrans(stream);
	}
	Prr = DestroyTrans(stream) || Prr;
	return(Prr);
}

/************************************************************************
 * POPIntroductions - sniff the POP server's bottom, and vice-versa
 ************************************************************************/
int POPIntroductions(TransStream stream, PStr user,Boolean *capabilities)
{
	Str255 buffer;
	Str255 args;
	long size;
	int result = -1;
	Boolean useAPOP = PrefIsSet(PREF_APOP);
	Boolean kerb4 = PrefIsSet(PREF_KERBEROS) && !PrefIsSet(PREF_K5_POP);
	Str255 digest;
	SASLEnum mech=0;
				
#ifdef TWO
	if (kerb4)
		if (Prr=SendPOPTicket(stream))
		{
			(*CurPers)->popSecure = False;
			goto done;
		}
#endif
	
	do
	{
		size = sizeof(buffer)-1;
		Prr = RecvLine(stream,buffer+1,&size);
		if (Prr) goto done;
		buffer[0] = MIN(size,127);
		ProgressMessage(kpMessage,buffer);
		buffer[0] = MIN(size,255);
	}
	while (buffer[1]!='+' && buffer[1]!='-');
	
	PopConnected = size && (buffer[1]=='+' || buffer[1]=='-');
	if (buffer[1] != '+')
	{
		Prr = buffer[1];
		POPCmdError(-1,nil,buffer);
		if (kerb4 && !NoClearPass(capabilities,buffer,size)) KerbDestroy();
		goto done;
	}
	
	if (capabilities) PopCapabilities(stream,capabilities,&mech);
	
#ifdef ESSL
	if( ShouldUseSSL(stream) && !(stream->ESSLSetting & esslSSLInUse))
	{
		if (!capabilities || !capabilities[pcapaSTLS])
		{
			if(!(stream->ESSLSetting & esslOptional))
			{
				Prr = unimpErr;
				ComposeStdAlert ( Note, ALRTStringsStrn+NO_SERVER_SSL );
				goto done;
			}
		}
		else
		{
			StringPtr errStr;
			
			errStr = buffer;
			size = sizeof(buffer)-1;
			Prr = POPCmdGetReply(stream,kpcStls,nil,buffer,&size);
			if(!Prr)
			{
				Prr = ESSLStartSSL(stream);
				if(Prr)
				{
DoSSLErrString :
					GetRString(buffer,SSL_ERR_STRING);
					errStr = buffer + 1;
					goto DoSSLErr;
				}
				else if(stream->ESSLSetting & esslSSLInUse)
					PopCapabilities(stream,capabilities,&mech);
				else
				{
					// Cyrus sucks.
					// After a failed TLS negotiation, Cyrus will issue a bogus
					// -ERR response to the NEXT command.  They shouldn't be issuing
					// any protocol-level response at all.  bxxxxxxs
					OTFlushInput(stream,GetRLong(FLUSH_TIMEOUT));
				}
			}
			else
DoSSLErr :
			{
				if(stream->ESSLSetting & esslOptional)
					Prr = noErr;
				else
				{
					POPCmdError(kpcStls,nil,errStr);
					goto done;
				}
			}
		}
	}
#endif

	ProgressMessageR(kpSubTitle,LOGGING_IN);
	
	if (mech && !kerb4)
	{
		// SASL ahoy!
		size = sizeof(buffer)-1;
		Prr = POPSasl(stream,capabilities,mech,buffer,&size);
	}
	else
	{
		if (useAPOP) {PCopy(args,(*CurPers)->password);useAPOP = GenDigest(buffer,args,digest);}
		
		if (useAPOP)
		{
			size = sizeof(buffer)-1;
			if (PrefIsSet(PREF_POP_SENDHOST))
				GetPOPPref(args);
			else
				PCopy(args,user);
			PCatC(args,' ');
			PCat(args,digest);
			Prr = POPCmdGetReply(stream,kpcApop,args,buffer,&size);
		}
		else
		{
			if (PrefIsSet(PREF_POP_SENDHOST))
				GetPOPPref(args);
			else
				PCopy(args,user);
			size = sizeof(buffer)-1;
			Prr = POPCmdGetReply(stream,kpcUser,args,buffer,&size);
			if (Prr || *buffer != '+')
			{
				if (!Prr) POPCmdError(kpcUser,args,buffer);
				Prr = '-';
				goto done;
			}
		
	#ifdef TWO
			if (kerb4)
				GetRString(args,KERBEROS_FAKE_PASS);
			else
	#endif
				PCopy(args,(*CurPers)->password);
		
			size = sizeof(buffer)-1;
			Prr = POPCmdGetReply(stream,kpcPass,args,buffer,&size);
		}
	}
	if (Prr || *buffer != '+')
	{
		if (!Prr)
		{
			(*CurPers)->popSecure = False;
			POPCmdError(kpcPass,nil,buffer);
			if (!NoClearPass(capabilities,buffer,size))
				InvalidatePasswords(False,True,False);
		}
		Prr = '-';
		goto done;
	}
	(*CurPers)->popSecure = True;
	SetPrefLong(PREF_POP_LAST_AUTH,GMTDateTime());
	
	ProgressMessageR(kpSubTitle,LOOK_MAIL);

	size = sizeof(buffer)-1;
	Prr = POPCmdGetReply(stream,kpcStat,nil,buffer,&size);
	if (Prr || *buffer != '+')
	{
		if (!Prr)  POPCmdError(kpcStat,nil,buffer);
		Prr = '-';
		goto done;
	}
	
	result = Atoi(buffer+3);
done:
	return(result);
}

/************************************************************************
 * PopCapabilities - what can our pop server do?
 ************************************************************************/
void PopCapabilities(TransStream stream, Boolean *capabilities, SASLEnum *mechPtr)
{
	short i;
	Str255 buffer;
	long size;
	UPtr spot;
	Str31 token;
	Str31 service;
	
	for (i=0;i<=pcapaLimit;i++) capabilities[i]=0;
	*mechPtr = 0;

	if (*GetRString(buffer,POPCmdsStrn+kpcCapa)<=1) return;	// hack, but hey
	GetRString(service,K5_POP_SERVICE);
	
	size = sizeof(buffer);
	if (!POPCmdGetReply(stream,kpcCapa,nil,buffer,&size) && *buffer=='+')
	{
		capabilities[0] = 1;	// we have them!
		for(;;)
		{
			size = sizeof(buffer)-1;
		  if (RecvLine(stream,buffer+1,&size)) break;
			buffer[0] = MIN(size,255);
			if (buffer[*buffer]=='\015') --*buffer;
			if (buffer[0]==1 && buffer[1]=='.') break;
			spot = buffer+1;
			if (PToken(buffer,token,&spot," "))
			{
				i = FindSTRNIndex(POPCapaStrn,token);
				if (i && i<pcapaLimit) capabilities[i] = 1;
				
				if (i==pcapaSASL)
				{
					while (PToken(buffer,token,&spot," "))
						*mechPtr = SASLFind(service,token,*mechPtr);
				}
			}
		}
	}
}

/**********************************************************************
 * POPSasl - do SASL for POP
 **********************************************************************/
int POPSasl(TransStream stream, Boolean *capabilities, SASLEnum mech, UPtr buffer, long *size)
{
	Accumulator chalAcc, respAcc;
	short rounds = 0;
	long bSize = *size;
	short smtpEquivCode = 501;
	Str63 service;
	long state = 0;
	Str255 scratch;
	
	Zero(chalAcc);
	Zero(respAcc);
	
	// put auth command in inital response
	AccuAddRes(&respAcc,EsmtpStrn+esmtpAuth);
	AccuAddChar(&respAcc,' ');
	
	// Grab stuff only Kerberos wants
	GetRString(service,K5_POP_SERVICE);
	
	// run the mechanism
	do
	{
		// Build the response
		if (SASLDo(service,mech,rounds++,&state,&chalAcc,&respAcc)) Prr = '-';
		else
		{
			// Send the response
			if (POPCmdLo(stream,kpcAuth,nil,&respAcc)) Prr = '-';
			else
			{
				// get the reply
				bSize = *size;
				Prr = POPGetReplyLo(stream,kpcAuth,buffer,&bSize,&respAcc);
				chalAcc.offset = 0;
				if (!Prr && respAcc.offset && **respAcc.data=='+')
				{
					if (respAcc.offset>1 && (*respAcc.data)[1]!=' ')
					{
						// We win!  We win!
						Prr = 0;
					}
					else
					{
						Prr = ' ';
						AccuAddFromHandle(&chalAcc,respAcc.data,1,respAcc.offset-1);
						// clean it out
						respAcc.offset = 0;
					}
				}
			}
		}
	}
	while (Prr==' ');
	
	if (Prr || **respAcc.data!='+')
	{
		(*CurPers)->popSecure = False;
		AccuToStr(&respAcc,scratch);
		POPCmdError(kpcAuth,nil,scratch);
		if (!NoClearPass(capabilities,scratch,*size))
			smtpEquivCode = 535;
	}
	else
		smtpEquivCode = 237;	// auth succeeded!

	// Let the sasl mechanism know how it all came out
	SASLDone(service,mech,rounds,&state,smtpEquivCode);
	
	AccuZap(chalAcc);
	AccuZap(respAcc);

	return Prr;
}

/**********************************************************************
 * NoClearPass - is a pass error one that should not reset the password?
 **********************************************************************/
Boolean NoClearPass(Boolean *capabilities,UPtr response, short len)
{
	Str255 string;
	short i;
	
	// If the pop server has the AUTH_RESP_CODE caapability,
	// then errors do NOT clear the password unless
	// [auth] appears in them
	if (capabilities && capabilities[pcapaAuthRespCode])
	{
		GetRString(string,POP3_AUTH_RESP_CODE);
		return !PFindSub(string,response);
	}
	
	// without the capability, we have to refer to 
	// our list of non-clearing errors
	for (i=1;*GetRString(string,NoClearPassStrn+i);i++)
		if (PPtrFindSub(string,response,len)) return(True);
	
	// Have we ever auth'ed using this password?  If so,
	// let's assume this is a server problem and not an authentication
	// problem.
	if (GetPrefLong(PREF_POP_LAST_AUTH)) return true;
	
	// all else as failed.  Sigh.
	return(False);
}

/************************************************************************
 * POPByeBye - tell the POP server we're leaving
 ************************************************************************/
int POPByeBye(TransStream stream)
{
	char buffer[CMD_BUFFER];
	long size=sizeof(buffer);
	if (!PopConnected) return(noErr);
	if (PrefIsSet(PREF_SLOW_QUIT)) Prr = POPCmdGetReply(stream,kpcQuit,nil,buffer,&size);
	else {Prr = POPCmd(stream,kpcQuit,nil); *buffer = '+';	/* fast TCP disconnect */}
	return (Prr || *buffer != '+');
}

/************************************************************************
 * POPCmd - Send a command to the POP server
 ************************************************************************/
int POPCmdLo(TransStream stream, short cmd, UPtr args, AccuPtr argsAcc)
{
	char buffer[CMD_BUFFER];
	short err;
	
	/*
	 * reap outstanding commands
	 */
	if (!CanPipeline || POPCmds && (*POPCmds)->elCount>=15)
	{
		err=ReapCmds(stream, -1);
		if (err==fnfErr) err = noErr;
		if (err) return(Prr=err);
	}
	
	if (CanPipeline && cmd==kpcQuit) ReapCmds(stream, 0);

	if (cmd) GetRString(buffer,POP_STRN+cmd);
	if (cmd==kpcPass || cmd==kpcAuth) ProgressMessage(kpMessage,buffer);
	if (cmd==kpcAuth) *buffer = 0;
	if (args && *args)
		PCat(buffer,args);
	if (cmd!=kpcAuth && cmd!=kpcPass && cmd!=kpcRetr && cmd!=kpcTop) ProgressMessage(kpMessage,buffer);
	if (cmd==kpcRetr || cmd==kpcDele) Log(LOG_LMOS,buffer);
	
	if (!argsAcc) PCat(buffer,NewLine);
	else if (cmd && cmd!=kpcAuth) PCatC(buffer,' ');
	
	err=SendTrans(stream,buffer+1,*buffer,nil);
	if (!err && argsAcc)
	{
		// add a newline to the accumulator
		AccuAddStr(argsAcc,NewLine);
		
		// send the data
		err = SendTrans(stream,LDRef(argsAcc->data),argsAcc->offset,nil);
		
		// erase what we did to the accumulator
		UL(argsAcc->data);
		argsAcc->offset -= *NewLine;
	}
	
	if (!err && POPCmds) err = StackQueue(&cmd,POPCmds);
	
	return(err);
}

/**********************************************************************
 * ReapCmds - reap commands until the named command is at the top
 *            of the stack, ready to be handled
 **********************************************************************/
OSErr ReapCmds(TransStream stream, short cmd)
{
	Str255 buffer;
	long size;
	OSErr err=noErr;
	short thisCmd=0;
	
	if (!POPCmds) return(noErr);
	
	while ((*POPCmds)->elCount)
	{
		if (cmd!=-1)
		{
			StackTop(&thisCmd,POPCmds);
			if (cmd==thisCmd) return(noErr);
		}
		StackPop(&thisCmd,POPCmds);
		do
		{
			size = sizeof(buffer);
			err = RecvLine(stream,buffer,&size);
		}
		while (!err && *buffer!='+' && *buffer!='-');
		if (thisCmd==cmd) return(noErr);
		if (thisCmd==kpcTop || thisCmd==kpcRetr)
		{
			do
			{
				size = sizeof(buffer);
				err = RecvLine(stream,buffer,&size);
			}
			while (!err && !POP_TERM(buffer,size));
		}
		if (cmd==-1) return(noErr);
	}
	
	if (err) Prr=err;
	
	return(err ? err : fnfErr);
}

/************************************************************************
 * POPCmdGetReply - send a POP command and get a reply
 ************************************************************************/
int POPCmdGetReply(TransStream stream, short cmd, UPtr args, UPtr buffer, long *size)
{ 
	if (cmd>=0 && (Prr=POPCmd(stream,cmd,args))) return(Prr);				/* error in transmission */

	return(POPGetReply(stream,cmd,buffer,size));
}

/************************************************************************
 * POPGetReply - get a reply to a POP command
 ************************************************************************/
int POPGetReplyLo(TransStream stream, short cmd, UPtr buffer, long *size, AccuPtr resAcc)
{ 
	long rSize;
	
	// So what's errChar?  Well, two things:
	// 1. Way Back When, when we did serial lines, some POP servers echoed;
	//    errChar is used to detect and ignore echoes.
	// 2. Some POP servers add extraneous blank lines (not that I'm naming
	//    names, but if someone were to nuke a company named "ipswitch", we
	//    might not have this problem anymore), and we can skip those, too
	// So when we see an actual valid POP error indicator (either + or -), we
	// know the true response has begun.  Hence, errChar.
	Byte errChar = 0;
	
	if (Prr=ReapCmds(stream, cmd)) return(Prr);
	if (resAcc) resAcc->offset = 0;
	do
	{
		rSize = *size;
		Prr = RecvLine(stream,buffer,&rSize);
		if (!rSize)
		{
			errChar = '-';
			break;
		}
		if (!errChar && (*buffer=='+' || *buffer=='-')) errChar = *buffer;
		if (!Prr && errChar && POPCmds && buffer[rSize-1]=='\r') StackPop(nil,POPCmds);
		if (errChar && resAcc) AccuAddPtr(resAcc,buffer,rSize);
	}
	while (!Prr && !CommandPeriod && !(errChar && buffer[rSize-1]=='\r'));
	*size = rSize;
	buffer[0] = errChar;
	return(Prr);
}

/************************************************************************
 * POPGetMessage - get a message from the POP server
 ************************************************************************/
int POPGetMessage(TransStream stream, long messageNumber,short *gotSome,POPDHandle popDH,Boolean *capabilities)
{
	char buffer[CMD_BUFFER];
	long size = sizeof(buffer);
	short count;
	TOCHandle tocH = GetInTOC();	/* shd already be in memory, so NBD to grab it here */
	POPDesc pd;
	long msgsize;
	Boolean notFetched = False;

	/*
	 * if there's no room at all, we won't even try
	 */
	if (RoomForMessage(0)) return(WarnUser(NOT_ENOUGH_ROOM,Prr=dskFulErr));

	pd = (*popDH)[messageNumber];
	msgsize = pd.msgSize;
	
	POPPreFetch(stream, popDH,CanPipeline?messageNumber+1:messageNumber,capabilities);
	
	/*
	 * stub or retr
	 */
	if (pd.stub)
	{
		msgsize *= -1;	/* let everyone down the line know what's going down */
	  size = sizeof(buffer);
		Prr = POPGetReply(stream,kpcTop,buffer,&size);
		NoAttachments = True;	/* don't do BinHex */
		RemIdFromPOPD(CUR_POPD_TYPE,DELETE_ID,(*popDH)[messageNumber].uidHash);	/* and clear the force del flag if set */
	}
	else
	{
		NoAttachments = pd.error ? True : False;
refetch:
	  size = sizeof(buffer);
		Prr=POPGetReply(stream,kpcRetr,buffer,&size);
	}
	
	if (Prr) return(Prr);
	if (*buffer!='+')
	{
		POPCmdError(kpcRetr,nil,buffer);
		return(Prr=1);
	}
	
	/*
	 * command issued and accepted - now read the message
	 */
	BadBinHex = False;
	BadEncoding = 0;
#ifdef DEBUG
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif
	count=FetchMessageText(stream,msgsize,&pd,messageNumber,nil);
#ifdef DEBUG
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif
	
	
	if (CommandPeriod && StackLowErr)
	{
		StackLowErr = false;
#ifdef THREADING_ON
		// increase thread stack size for next try
		if (InAThread())
		{
			WarnUser(THREAD_LOW_STACK,0);
		}
		else
#endif
	// try lowering appllimit for non-threaded operation????
	// ...Will look into if users frequently experience
			WarnUser(LOW_STACK,0);
	}
	
	/*
	 * did it work?
	 */
	if (!Prr && !CommandPeriod)
	{
		/*
		 * ask user what to do about encoding errors
		 */
#ifdef BAD_ENCODING_HANDLING
		if (BadBinHex || BadEncoding)
		{
			pd.delete = False;
			pd.stubbed = True;
			pd.error = True;
			notFetched = True;
		}
#endif
		if (pd.delete)
		{
			Prr = DeletePOPMessage(stream,messageNumber,pd.uidHash);
			if (!Prr) pd.deleted = True;
		}
		if (pd.stub) pd.stubbed = True;
		else if (!notFetched && pd.retr)
		{
			pd.retred = True;
			RemIdFromPOPD(CUR_POPD_TYPE,FETCH_ID,pd.uidHash);
		}
	}
	
	(*popDH)[messageNumber] = pd;

	if (!Prr) (*gotSome)+=count;
	return(Prr);
}

/************************************************************************
 * DeletePOPMessage - delete a message from the POP server
 ************************************************************************/
OSErr DeletePOPMessage(TransStream stream, short number,long uidHash)
{
	Str255 buffer;
	Str63 args;
	long size;
	
	NumToString(number+1,args);
	size = sizeof(buffer);
	Prr=POPCmd(stream,kpcDele,args);
	if (!Prr) RemIdFromPOPD(CUR_POPD_TYPE,DELETE_ID,uidHash);
	return(Prr);
}

/************************************************************************
 * FillSizesWithList - fill message sizes with the LIST command
 ************************************************************************/
OSErr FillSizesWithList(TransStream stream,POPDHandle popDH)
{
	Str127 buffer;
	long size = sizeof(buffer);
	short msgNum;
	UPtr spot;
	short n = HandleCount(popDH);
	
	if (Prr=POPCmdGetReply(stream,kpcList,nil,buffer,&size)) return(Prr);

	if (*buffer != '+')
	{
		Prr = *buffer;
		POPCmdError(kpcList,nil,buffer);
		return(Prr);
	}

//	if (n>100) ByteProgress(nil,0,n);
	
	for (size=sizeof(buffer);
			 !(Prr=RecvLine(stream,buffer,&size)) && !POP_TERM(buffer,size);
			 size=sizeof(buffer))
	{
		CycleBalls(); if (CommandPeriod) break;

		spot = strtok(buffer," \t");
		if (!spot) continue;
		msgNum = Atoi(spot);
		
		spot = strtok(nil," \t");
		if (!spot) continue;
		size = Atoi(spot);
		
		if (msgNum<1 || msgNum>n) continue;
		
//		if (n>100) ByteProgress(nil,-1,0);
		
		(*popDH)[msgNum-1].msgSize = size;
	}
	if (CommandPeriod && !Prr) Prr = userCancelled;
	
//	if (n > 100 && !Prr) ByteProgress(nil,1,1);
	Progress(NoBar,0,nil,nil,nil);
	
	return(Prr);
}

/************************************************************************
 * POPCmdError - report an error for an POP command
 ************************************************************************/
int POPCmdError(short cmd, UPtr args, UPtr message)
{
	Str255 theCmd;
	Str255 theError;
	int err;

	*theCmd = 0;
	GetRString(theCmd,POP_STRN+cmd);
	if (args && *args)
		PCat(theCmd,args);
	strcpy(theError+1,message);
	*theError = strlen(theError+1);
	if (theError[*theError]=='\012') (*theError)--;
	if (theError[*theError]=='\015') (*theError)--;
	MyParamText(theCmd,theError,"\pPOP","");
	err = ReallyDoAnAlert(PROTO_ERR_ALRT,Note);
	return(err);
}

/************************************************************************
 * FetchMessageText - read in the body of a message
 ************************************************************************/
int FetchMessageText(TransStream stream,long estSize,POPDPtr pdp,short messageNumber,TOCHandle useTocH)
{
	return (FetchMessageTextLo(stream, estSize, pdp, messageNumber, useTocH, false, false));
}

/************************************************************************
 * FetchMessageText - read in the body of a message
 ************************************************************************/
int FetchMessageTextLo(TransStream stream,long estSize,POPDPtr pdp,short messageNumber,TOCHandle useTocH,Boolean imap,Boolean import)
{
	UPtr text=nil;
	TOCHandle tocH;
	MSumType sum;
	long eof,chopHere;
	Str255 name;
	short count=0,part;
	HeaderDHandle hdh = nil;
	LineIOD lid;
	OSErr err;
	FSSpec	spec;
	extern OSErr ImportErr;
	Str63 savedSub;
	
	/*
	 * make the message summary
	 */
	WriteZero(&sum,sizeof(MSumType));
	*savedSub = 0;
	
	/*
	 * haven't seen any rich text yet
	 */
	AnyRich = AnyHTML = AnyFlow = AnyCharset = AnyDelSP = False;
	ZapHandle(LastAttSpec);	/* or attachments */
	ETLDeleteRequest = False;	/* and no translators have been run on this message yet */

	/*
	 * grab the destination mailbox (usually "In")
	 */
	tocH = useTocH ? useTocH : GetInTOC();
	if (!tocH) {Prr=-108;return(0);}
	spec = GetMailboxSpec(tocH,-1);
	PCopy(name,spec.name);
	
	// if we're adding IMAP messages or importing mail, we've taken care of opening the mailbox already	
	if (!imap && !import)	
	{
		Prr = BoxFOpen(tocH);
		if (Prr) {FileSystemError(OPEN_MBOX,name,Prr); goto done;}
	}
	
	eof = FindTOCSpot(tocH,estSize);

	Prr = SetFPos((*tocH)->refN, fsFromStart, eof);
	if (Prr) {FileSystemError(WRITE_MBOX,name,Prr); goto done;}
		
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////

	count = SaveAndSplit(stream,(*tocH)->refN,estSize,&hdh,(*tocH)->imapTOC);

#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
	
done:
	if (!Prr && !GetFPos((*tocH)->refN,&chopHere))
		SetEOF((*tocH)->refN,chopHere);

	// if we're adding IMAP messages, or importing mail, we'll close and flush later.
	if (Prr || (!imap && !import))	
	{
		Prr = BoxFClose(tocH,true) || Prr;
		if (Prr || !count) return(0);
	}
	
	/*
	 * now, read it back from the file
	 */
	if (Prr=OpenLine(spec.vRefNum,spec.parID,name,(imap||import)?fsRdPerm:fsRdWrPerm,&lid))
		 {FileSystemError(READ_MBOX,name,Prr);return(0);}
	if (Prr=SeekLine(eof,&lid)) {FileSystemError(READ_MBOX,name,Prr);return(0);}
	
	ReadSum(nil,False,&lid,True);
	for (part=1;!(err=ReadSum(&sum,False,&lid,True));part++)
	{
		if (!*savedSub) PSCopy(savedSub,sum.subj);
		if (part==1 && pdp)
		{
			FillPOPD(pdp,hdh);
			DBNoteUIDHash(sum.uidHash,pdp->uidHash);
			sum.uidHash = pdp->uidHash;
		}
		else
		{
			DBNoteUIDHash(sum.uidHash,kNoMessageId);
			sum.uidHash = kNoMessageId;
		}
		if (!(*hdh)->isMIME)
		{
			TransLitString(sum.from);
			TransLitString(sum.subj);
		}
		else sum.tableId = ViewTable(hdh);
		if (part==1) sum.msgIdHash = (*hdh)->msgIdHash;
		if (!ValidHash(sum.uidHash)) sum.uidHash = sum.msgIdHash;
#ifdef BAD_ENCODING_HANDLING
		if ((estSize<0) || BadBinHex || BadEncoding) sum.flags |= FLAG_SKIPPED;
#else
		if ((estSize<0)) sum.flags |= FLAG_SKIPPED;
#endif

		// set or clear html/enriched flags.  Clear is necessary because toc build might give
		// false positive
		if (count==1 && AnyRich) sum.flags |= FLAG_RICH; else sum.flags &= ~FLAG_RICH;
		if (count==1 && AnyHTML) sum.opts |= OPT_HTML; else sum.opts &= ~OPT_HTML;
		if (count==1 && AnyFlow) sum.opts |= OPT_FLOW; else sum.opts &= ~OPT_FLOW;
		if (count==1 && AnyDelSP) sum.opts |= OPT_DELSP; else sum.opts &= ~OPT_DELSP;
		if (count==1 && AnyCharset) sum.opts |= OPT_CHARSET; else sum.opts &= ~OPT_CHARSET;
		if ((*hdh)->hasMDN) sum.opts |= OPT_RECEIPT;
		if (LastAttSpec) sum.flags |= FLAG_HAS_ATT;
		if (!sum.seconds) sum.seconds = GMTDateTime();
		if (count>1) StampPartNumber(&sum,part,count);
		sum.spamScore = 0;
		sum.arrivalSeconds = GMTDateTime();
		if (Prr) break;
		if (useTocH && !import)		// create a new summary if we're importing
		{
			MSumType	newSum;
			
			newSum = (*tocH)->sums[messageNumber];
			newSum.offset = sum.offset;
			newSum.length = sum.length;
			newSum.bodyOffset = sum.bodyOffset;
			newSum.flags |= sum.flags;
			newSum.opts |= sum.opts;
			newSum.msgIdHash = sum.msgIdHash;
			if (!newSum.priority) newSum.priority = sum.priority;
			PSCopy(newSum.subj,sum.subj);
			if (!(newSum.opts & OPT_IMAP_SENT)) 
				PSCopy(newSum.from,sum.from);
			RemoveUTF8FromSum(&newSum);
			(*tocH)->sums[messageNumber] = newSum;
		}
		else
			if (Prr=!SaveMessageSum(&sum,&tocH))
			{
				if (import) ImportErr = memFullErr;		// stop if we're importing.
				break;
			}
	}
	ReadSum(nil,False,&lid,True);
	if (err!=fnfErr) Prr = err;
	Prr = Prr || part<=count;
	ZapHeaderDesc(hdh);
			
	CloseLine(&lid);

#ifdef DEBUG
	if (ETLDeleteRequest) ComposeLogS(LOG_PLUG,nil,"\pA plugin has requested the deletion of '%p' in '%p'",savedSub,spec.name);
#endif

	if (BadBinHex || BadEncoding)
	{
		Str255 hex,
					enc,
					errorStr;

		if (BadBinHex) GetRString(hex,BAD_HEX_MSG);
			else *hex = 0;
		if (BadEncoding) ComposeRString(enc,BAD_ENC_MSG,BadEncoding,BadEncoding);
			else *enc = 0;
			
		ComposeRString(errorStr,(imap?IMAP_BAD_HEXBIN_ERR_TEXT:BAD_HEXBIN_ERR_TEXT),hex,enc);
	
		if (imap)	// add the message error to the IMAP message we just downloaded
			AddMesgError (tocH,messageNumber,errorStr,-1);
		else		// add it to the last message added to the mailbox.  OK for POP.
			AddMesgError (tocH,(*tocH)->count-1,errorStr,-1);
	}

	if (Prr)
	{
		WarnUser(READ_MBOX,Prr);
		return(0);
	}
	
	count = part-1;

	InvalSum(tocH,useTocH?messageNumber:(*tocH)->count-1);
	if (!PrefIsSet(PREF_CORVAIR) && !((*tocH)->count%5))
	{
		Prr = WriteTOC(tocH);
		FlushVol(nil,spec.vRefNum);
	}
	MakeMessTitle(name,tocH,useTocH?messageNumber:(*tocH)->count-count,False);
	ComposeLogR(LOG_RETR,nil,MSG_GOT,name,count);
	if (!imap) UpdateNumStatWithTime(kStatReceivedMail,1,(*tocH)->sums[useTocH?messageNumber:(*tocH)->count-1].seconds+ZoneSecs());	
  return(Prr ? 0:count);
}

/************************************************************************
 * SaveAndSplit - read a message, (possibly) splitting it into parts and
 * saving it.
 ************************************************************************/
int SaveAndSplit(TransStream stream,short refN,long estSize,HeaderDHandle *hdhp,Boolean isIMAP)
{
	Str255 buf;
	short count=0;
	HeaderDHandle hdh=NewHeaderDesc(nil);
	long fromSize;
	long end;
	long oldStart;
	short lastHeaderTokenType;
	long ticks = TickCount();
	
	if (!hdh) {Prr=MemError(); return(0);}

//	if (estSize>0) ByteProgress(nil,0,estSize);
	
	if (Prr=PutOutFromLine(refN,&fromSize)) return(0);
	
reRead:
	if (!hdh) {Prr=MemError(); return(0);}

	lastHeaderTokenType = ReadHeader(stream,hdh,estSize,refN,False);

#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",lastHeaderTokenType);
#endif //DEBUG //////////////////////////

	if (lastHeaderTokenType!=EndOfHeader && lastHeaderTokenType!=EndOfMessage)
	{
		Prr = 1;
		goto done;
	}
	
	if (fromSize)
	{
		(*hdh)->diskStart -= fromSize;	/* count the envelope as part of the header */
		fromSize = 0;										/* in case we pass this way again. */
	}
	else
		(*hdh)->diskStart = oldStart;

	
	if (!Prr)
	{
		/*
		 * I've wanted to do this for years.  say who it's from!
		 */
		PCopy(buf,(*hdh)->who);
		*buf = MIN(*buf,31);	// not too long here...
		PCatC(buf,',');
		PCatC(buf,' ');
		PSCat(buf,(*hdh)->subj);
		if (!(*hdh)->isMIME) TransLitString(buf);
		ProgressMessage(kpMessage,buf);
		
		// regenerate full info for comment
		PCopy(buf,(*hdh)->who);
		PCatC(buf,',');
		PCatC(buf,' ');
		PSCat(buf,(*hdh)->subj);
		PSCopy((*hdh)->summaryInfo,buf);
		
		
		/*
		 * now, go save the body
		 */
		if (lastHeaderTokenType!=EndOfMessage) ReadEitherBody(stream,refN,hdh,buf,sizeof(buf),estSize,EMSF_ON_ARRIVAL);
		else FSWriteP(refN,"\p\015\015");

#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
		
		/*
		 * darn encapsulated stuf
		 */
		if (Prr == '82')
		{
			oldStart = (*hdh)->diskStart;
			ZapHeaderDesc(hdh);
			hdh = NewHeaderDesc(nil);
			PSCopy((*hdh)->summaryInfo,buf);
			Prr = noErr;
			goto reRead;
		}
		
		/*
		 * ok, got real body now
		 */
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////

		EnsureNewline(refN);
		ticks = TickCount()-ticks + 1;
		{
			long rate = (estSize*600)/(ticks*1024);
			ComposeLogS(LOG_TPUT,nil,"\p%dK in %d.%d sec; %d.%d KBps",estSize/1024,ticks/60,(ticks/6)%10,rate/10,rate%10);
		}
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d %d;file %x;sc;g",Prr,GetFPos(refN,&end),refN);
#endif //DEBUG //////////////////////////
		if (!Prr && !(Prr=GetFPos(refN,&end)))
		{
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
			TruncOpenFile(refN,end);
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d e %d ds %d st %d;sc;g",Prr,end,(*hdh)->diskStart,GetRLong(SPLIT_THRESH));
#endif //DEBUG //////////////////////////
			if (!isIMAP && (end-(*hdh)->diskStart>GetRLong(SPLIT_THRESH)))
				count = SplitMessage(refN,(*hdh)->diskStart,(*hdh)->diskEnd,end);
			else
				count = 1;
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
		}
	}

#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
	
done:
	*hdhp = hdh;
	if (Prr) return(0);
	return(estSize<0 && GetPrefLong(PREF_POP_MODE)==popRStatus && *(*hdh)->status ? 0 : count);
}

/************************************************************************
 * ReadEitherBody - read the body of a message from a pop-3 server
 ************************************************************************/
short ReadEitherBody(TransStream stream,short refN,HeaderDHandle hdh,char *buf,long bSize,long estSize,long context)
{
	MIMESHandle mimeSList=nil;
	BoundaryType endType;
	
	/*
	 * is our MIME converter interested in this thing?
	 */
	if (!NoAttachments && estSize>=0)
	{
		mimeSList = NewMIMES(stream,hdh,False,context);
		if (!mimeSList) return(Prr = MemError());
		if (mimeSList == kMIMEBoring) mimeSList = nil;
		else if ((*mimeSList)->readBody==READ_MESSAGE)
		{
			ZapMIMES(mimeSList);
			return(Prr = '82');
		}
	}
	
	/*
	 * call the proper body reading function
	 */
	endType = mimeSList
							? (*(*mimeSList)->readBody)(stream,refN,mimeSList,buf,bSize,ReadPOPLine)
								: ReadPlainBody(stream,refN,buf,bSize,estSize);

	if (endType == btError) Prr = 1;
#ifdef DEBUG ////////////////////////////
	if (BUG15) Dprintf("\p%d;sc;g",Prr);
#endif //DEBUG //////////////////////////
	ZapMIMES(mimeSList);
	return(Prr);
}

/**********************************************************************
 * RoomForMessage - make sure there's room for a message on both the
 *  attachments folder volume and the in box volume
 **********************************************************************/
OSErr RoomForMessage(long msgsize)
{
	FSSpec attSpec;
	OSErr err=noErr;
	
	GetAttFolderSpec(&attSpec);
	err = VolumeMargin(MailRoot.vRef,msgsize);
	if (!err && MailRoot.vRef==attSpec.vRefNum) return(noErr);
	if (!err) err = VolumeMargin(attSpec.vRefNum,msgsize);
	
	return(err);
}

/************************************************************************
 * ReadPlainBody - handle the body of a non-MIME message.
 ************************************************************************/
POPLineType ReadPlainBody(TransStream stream,short refN,char *buf,long bSize,long estSize)
{
	long size;
	Boolean hexing, singling, pgping;
	POPLineType lineType;
#ifdef OLDPGP
	PGPContext pgp;
#endif
	
	/*
	 * prepare converters
	 */
	if (!NoAttachments)
	{
		BeginHexBin(nil);
		BeginAbomination("",nil);
#ifdef OLDPGP
		BeginPGP(&pgp);
#endif
		hexing = singling = pgping = False;
	}
	ReadPOPLine(stream,nil,0,nil);

	/*
	 * main processing loop
	 */
	for (lineType=ReadPOPLine(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=ReadPOPLine(stream,buf,bSize,&size))
	{		
		/*
		 * give each converter a crack at the line
		 */
		if (!NoAttachments)
		{
			if (!(singling || pgping)) hexing = ConvertHexBin(refN,buf,&size,lineType,estSize);
			if (!(hexing || pgping)) singling = ConvertUUSingle(refN,buf,&size,lineType,estSize,nil,nil);
#ifdef OLDPGP
			if (!(singling || hexing)) pgping = ConvertPGP(refN,buf,&size,lineType,estSize,&pgp);
#endif
		}
		
		/*
		 * write the line
		 */
		if (size && (Prr=AWrite(refN,&size,buf)))
			break;
	}
	
	/*
	 * record skipped message
	 */
	if (!Prr && lineType == plEndOfMessage && estSize < 0)
	{
	  Str255 msg;
		long count;
#ifdef TWO
		if (Headering || PrefIsSet(PREF_NO_BIGGIES))
			ComposeRString(msg,BIG_MESSAGE_MSG2,-estSize);
		else
			ComposeRString(msg,NOSPACE_SKIP,-estSize);
#else
		ComposeRString(msg,BIG_MESSAGE_MSG,-estSize);
#endif
		count = *msg;
		Prr=AWrite(refN,&count,msg+1);
	}

	/*
	 * close converters
	 */
	if (!NoAttachments)
	{
		EndHexBin();
		SaveAbomination(nil,0);
#ifdef OLDPGP
		EndPGP(&pgp);
#endif
		
		/*
		 * write attachment notes, if any
		 */
		WriteAttachNote(refN);
	}
	
	/*
	 * report error (if any)
	 */
  if (Prr) FileSystemError(WRITE_MBOX,"",Prr);
//	else if (!UUPCIn) Progress(100,NoChange,nil,nil,nil);

	return(Prr?btError : btEndOfMessage);
}

/************************************************************************
 * PutOutFromLine - write an envelope
 ************************************************************************/
int PutOutFromLine(short refN,long *fromLen)
{
	Str255 fromLine;
	long len;
	
	*fromLen = len = SumToFrom(nil,fromLine);
	if (Prr=AWrite(refN,&len,fromLine))
		return(FileSystemError(WRITE_MBOX,"",Prr));
	return(noErr);
}

/************************************************************************
 * DupHeader - copy the header of a split message
 ************************************************************************/
int DupHeader(short refN,UPtr buff,long bSize,long offset,long headerSize)
{
	long currentOffset;
	long readBytes,writeBytes;
	long copied;
	
	if (Prr=GetFPos(refN,&currentOffset))
		return(FileSystemError(READ_MBOX,"",Prr));
	for (copied=0; copied<headerSize; copied += readBytes)
	{
		if (Prr=SetFPos(refN,fsFromStart,offset+copied))
			return(FileSystemError(READ_MBOX,"",Prr));
		readBytes = bSize < headerSize-copied ? bSize : headerSize-copied;
		if (Prr=ARead(refN,&readBytes,buff))
			return(FileSystemError(READ_MBOX,"",Prr));
		if (Prr=SetFPos(refN,fsFromStart,currentOffset))
			return(FileSystemError(WRITE_MBOX,"",Prr));
		writeBytes = readBytes;
		if (Prr=FSZWrite(refN,&writeBytes,buff))
			return(FileSystemError(WRITE_MBOX,"",Prr));
		currentOffset += writeBytes;
	}
	return(noErr);
}

/************************************************************************
 * FirstUnread - find the first unread message
 *	 We do try to be clever about it. 
 ************************************************************************/
int FirstUnread(TransStream stream,int count)
{
	short first, theLast, on;
	static short lastCount=0;
	Boolean hasBeen;
	
	/*
	 * give LAST a whirl
	 */
	if (POPLast(stream,&theLast)) return(count+1);
	theLast = MAX(theLast,0);
	theLast = MIN(theLast,count);

	/*
	 * if LAST returns nonzero, believe it
	 * Also believe it if the user tells us to believe it
	 */
	if (theLast || GetPrefLong(PREF_POP_MODE)==popRLast)
		return(theLast+1);
		
	/*
	 * LAST was a dead end.  Do it the hard way
	 */
	on = lastCount;
	lastCount = count;

#define SETHASBEEN(o,c)\
	do {hasBeen=HasBeenRead(stream,o,c);if (CommandPeriod) return(c+1);} while (0) 
	if (PrefIsSet(PREF_NO_BIGGIES))
	{
		/* Heuristics */
		if (on && on<=count)
		{
			SETHASBEEN(on,count);
			if (hasBeen)
			{
				SETHASBEEN(on+1,count);
				if (!hasBeen) return(on+1);
			}
		}
		SETHASBEEN(count,count);
		if (hasBeen) return(count+1);
		if (count==1) return(1);
		
		/* search... */
		for (on=count-1;on;on--)
		{
			SETHASBEEN(on,count);
			if (hasBeen) break;
		}
		return(on+1);
	}
	else
	{
		first = 1;
		theLast = count;
		/*
		 * try to cut the search short via heuristics
		 */
		if (on && on<=count)
		{
			SETHASBEEN(on,count);
			if (hasBeen)
			{
				if (on<count)
				{
					SETHASBEEN(++on,count);
					if (!hasBeen) return(on);
				}
				first = on+1;
			}
			else
				theLast = on-1;
		}
		else
		{
			SETHASBEEN(count,count);
			if (hasBeen) return(count+1);
			SETHASBEEN(1,count);
			if (count==1 || !hasBeen) return(1);
			theLast=count-1;
			first = 2;
			on = count;
			hasBeen = False;
		}
			
		/*
		 * hi ho, hi ho, it's off to search we go
		 */
		while (first<=theLast)
		{
			on = (first+theLast)/2;
			SETHASBEEN(on,count);
			if (hasBeen) first=on+1;
			else theLast=on-1;
		}
		if (!hasBeen)
			return(on);
		else
			return(on+1);
	}
}

/************************************************************************
 * HasBeenRead - has a particular message been read?
 * look for a "Status:" header; if it's "Status: R<something>", message
 * has been read
 ************************************************************************/
Boolean HasBeenRead(TransStream stream,short msgNum,short count)
{
	Str127 scratch;
	Boolean unread=False, statFound=False;
	Str31 terminate;
	Str31 status;
	UPtr cp;
	long size;

	if (msgNum>count) return(0);
	Progress((msgNum*100)/count,NoBar,nil,GetRString(scratch,FIRST_UNREAD),nil);
	GetRString(terminate,ALREADY_READ);
	GetRString(status,STATUS);
	NumToString(msgNum,scratch);
	PLCat(scratch,1);
	POPCmd(stream,kpcTop,scratch);
	for (size=sizeof(scratch);
				!(Prr=RecvLine(stream,scratch,&size)) &&
				!POP_TERM(scratch,size);
				size=sizeof(scratch))
		if (!unread && !statFound && !striscmp(scratch,status+1))
		{
			statFound = True;
			for (cp=scratch;cp<scratch+size;cp++)
			{
				if (*cp==':')
				{
					for (cp++;cp<=scratch+size-*terminate;cp++)
						if (!striscmp(cp,terminate+1)) break;
					unread = cp> scratch+size-*terminate;
					break;
				}
			}
		}
	ComposeLogS(LOG_LMOS,nil,"\pHasBeenRead: %d sf %d un %d %p",msgNum,statFound,!unread,statFound&&!unread?"\pREAD":"\pUNREAD");
	return(statFound && !unread);
}

/************************************************************************
 * StampPartNumber - put the part number on a mail message
 ************************************************************************/
void StampPartNumber(MSumPtr sum,short part,short count)
{
	char *spot;
	short i, digits, len;
	
	if (part==1)
		sum->flags |= FLAG_FIRST;
	else
		sum->flags |= FLAG_SUBSEQUENT;

	for (i=count,digits=0;i;i/=10,digits++);
	len = 2*digits+2;
	spot=sum->subj+MIN(*sum->subj+len,sizeof(sum->subj)-1);
	*sum->subj = spot-sum->subj;
	for (i=digits;i;i--,count/=10) *spot-- = '0'+count%10;
	*spot-- = '/';
	for (i=digits;i;i--,part/=10) *spot-- = '0'+part%10;
	*spot = ' ';
	
}

/************************************************************************
 * RecordAttachment - note that we've attached a file
 ************************************************************************/
OSErr RecordAttachment(FSSpecPtr spec,HeaderDHandle hdh)
{
	Str255 theMessage;
	OSErr err;
	Boolean deleted = false;
	
	if(FileTypeOf(spec) == REG_FILE_TYPE)
	{
		if(!hdh || AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aRegFile,theMessage))
		{
			FSpDelete(spec);
			deleted = true;
			GetRString(theMessage, STOLEN_REG_FILE);
		}
		else
		{
			if(!gRegFiles) gRegFiles = NuHandle(0);
			if(gRegFiles) PtrPlusHand_(spec, gRegFiles, sizeof(FSSpec));
		}
	}
	
	// Long filename?
	if (hdh && !(*hdh)->relatedPart) FixLongFilename(hdh,spec);
	
	/*
	 * update global fsspec record
	 */
	if (!deleted)
	{
		if (!LastAttSpec) LastAttSpec = NewH(FSSpec);
		if (LastAttSpec) **(FSSpecHandle)LastAttSpec = *spec;
	}
	
	// only record top-level files
	if (AttFolderStack && !SameSpec(&CurrentAttFolderSpec,&AttFolderSpec)) return noErr;
	
	if(!deleted)
	{
		if (hdh && (*hdh)->relatedPart)
			RelatedNote(spec,hdh,theMessage);
		else AttachNoteLo(spec,theMessage);
	}
	
	/*
	 * tack on the note
	 */
	if (!AttachedFiles) AttachedFiles = NuHandle(0);
	if (AttachedFiles) PtrPlusHand_(theMessage+1,AttachedFiles,*theMessage);
	if (err=MemError())
	{
		WarnUser(BINHEX_MEM,err);
		CommandPeriod = true;
		return(err);
	}
	
	if(deleted)
		return(noErr);
	
	RecordTransAttachments(spec);
	
	/*
	 * add a comment?
	 */
	if (hdh && !PrefIsSet(PREF_NO_ATT_COMMENT))
		DTSetComment(spec,PCopy(theMessage,(*hdh)->summaryInfo));
	
	/*
	 * is there a date?
	 */
	if (hdh && !AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aModDate,theMessage))
	{
		uLong mod;
		long zone;
		
		if (mod=BeautifyDate(theMessage,&zone))
			if (mod>(long)GetRLong(TOO_EARLY_FILE))	AFSpSetMod(spec,mod+ZoneSecs());
	}

	//	record for attachment received statistics
	UpdateNumStatWithTime(kStatReceivedAttach,1,hdh?(*hdh)->gmtSecs+ZoneSecs():LocalDateTime());
		
	return(noErr);
}

/************************************************************************
 * FixLongFilename - attach a long filename to an FSSpec made from a shorter
 *  filename
 ************************************************************************/
OSErr FixLongFilename(HeaderDHandle hdh,FSSpecPtr spec)
{
	Str127 longFilename;
	Str31 filenameAtt;
	Str255 part;
	Str255 charset;
	
	*longFilename = *charset = 0;
	
	// is there a filename at all?
	if (AAFetchData((*hdh)->contentAttributes,GetRString(filenameAtt,AttributeStrn+aFilename),longFilename))
	{
		// no "filename".  Is there a "filename*"?
		PCatC(filenameAtt,'*');
		if (!AAFetchData((*hdh)->contentAttributes,filenameAtt,part))
			Un2184Append(longFilename,sizeof(longFilename),part,charset,true);
		else
		{
			// no "filename*'.
			short i;
			for (i=0;;i++)
			{
				// Is there a "filename*0"?
				if (!AAFetchData((*hdh)->contentAttributes,ComposeRString(filenameAtt,AttributeStrn+aFilename,i),part))
				{
					if (i==0) GetRString(charset,UNSPECIFIED_CHARSET);
					Un2184Append(longFilename,sizeof(longFilename),part,charset,false);
				}
				// Is there a "filename*0*"?
				else if (!AAFetchData((*hdh)->contentAttributes,PCat(filenameAtt,"\p*"),part))
					Un2184Append(longFilename,sizeof(longFilename),part,charset,true);
				else
					break;
			}
		}
	}
	
	// check for applesingle
	if (EqualStrRes(LDRef(hdh)->contentSubType,MIME_APPLEFILE))
		if (longFilename[1]=='%' && *longFilename>1)
			BMD(longFilename+2,longFilename+1,--*longFilename);
	UL(hdh);

	// is it a short filename?
	if (*longFilename <= 31 && !AnyHighBits(longFilename+1,*longFilename)) return noErr;
	
	// Ok, it's a long filename.  Set the name of the file to it
	//	Make sure that the file is unique
	(void) MakeUniqueLongFileName ( spec->vRefNum, spec->parID, longFilename, kTextEncodingUnknown, sizeof ( longFilename ) - 1 );
	return FSpSetLongName(spec,kTextEncodingUnknown,longFilename,spec);
}

/************************************************************************
 * Un2184Append - undo some 2184, and append to an existing string
 ************************************************************************/
PStr Un2184Append(PStr dest,short sizeofDest,PStr orig,PStr charset,Boolean isEncoded)
{
	Str255 decoded;
	short spaceNeeded;
	short leftAppend = GetRLong(MIN_LEFT_APPEND);
	Str31 elide;
	
	if (isEncoded) Un2184(decoded,orig,charset);
	else PCopy(decoded,orig);
	
	spaceNeeded = 1+*dest+*decoded - sizeofDest;
	if (spaceNeeded > 0)
	{
		// add in space for the ellipsis
		spaceNeeded += *GetRString(elide,ELIDE_2184_STRING);
		// Make sure we leave at least 64 characters of the original string
		if (*dest > leftAppend)
		{
			// we can get all we want from the excess of dest
			if (spaceNeeded <= *dest - leftAppend)
				*dest -= spaceNeeded;
			else
			{
				// take some from dest...
				spaceNeeded -= *dest - leftAppend;
				
				// and take the rest from the RIGHT side of decoded
				BMD(decoded+1,decoded+1+spaceNeeded,*decoded-spaceNeeded);
				*decoded -= spaceNeeded;
			}
		}
		
		// copy the elision string
		PCat(dest,elide);
	}
		
	// copy the decoded string
	PCat(dest,decoded);

	return dest;
}

/************************************************************************
 * Un2184 - undo 2184 encoding
 ************************************************************************/
PStr Un2184(PStr dest, PStr orig, PStr charset)
{
	short tableID;
	Boolean found;
	
	// copy it in
	PCopy(dest,orig);
		
	// do we have a charset?
	if (!*charset)
	{
		if (PIndex(orig,'\''))
		{
			UPtr spot = orig+1;
			PToken(orig,charset,&spot,"'");	// grab charset
			PToken(orig,dest,&spot,"'");		// skip language
			PToken(orig,dest,&spot,"'");	// put the rest into dest
		}
		if (!*charset) GetRString(charset,UNSPECIFIED_CHARSET);
	}
	
	// undo the QP
	FixURLString(dest);
	
	// transliterate
	tableID = FindMIMECharsetLo(charset,&found);
	if (!found) tableID = FindMIMECharset(GetRString(charset,UNSPECIFIED_CHARSET));
	TransLitRes(dest+1,*dest,tableID);
	
	return dest;
}

/************************************************************************
 * AttachNoteLo - format the attachment note
 ************************************************************************/
void AttachNoteLo(FSSpecPtr spec,PStr theMessage)
{
	Str31 folderName;
	Str15 typeString, creatorString;
	FInfo info;
	long fid;
	Str31 fidStr;
	
	if (FSpIsItAFolder(spec))
	{
		info.fdCreator = kSystemIconsCreator;
		info.fdType = kGenericFolderIcon;
		fid = SpecDirId(spec);
	}
	else
	{
		FSpGetFInfo(spec,&info);
		FSMakeFID(spec,&fid);
	}
	
	BMD(&info.fdCreator,creatorString+1,4);
	BMD(&info.fdType,typeString+1,4);
	*creatorString = *typeString = 4;
	SanitizeFN(creatorString,creatorString,ATTCONV_BAD_CHARS,ATTCONV_REP_CHARS,false);
	SanitizeFN(typeString,typeString,ATTCONV_BAD_CHARS,ATTCONV_REP_CHARS,false);
	GetMyVolName(spec->vRefNum,folderName);
	ComposeRString(theMessage,FILE_FOLDER_FMT,
								 folderName,spec->name,typeString,creatorString,Long2Hex(fidStr,fid)); 
}

/************************************************************************
 * RelatedNote - format the related note
 ************************************************************************/
void RelatedNote(FSSpecPtr spec,HeaderDHandle hdh,PStr theMessage)
{
	Str31 folderName;
	long fid;
	Str255 quoteName;
	
	FSMakeFID(spec,&fid);
	PCopy(quoteName,spec->name);
	
	GetMyVolName(spec->vRefNum,folderName);
	ComposeRString(theMessage,RELATED_FMT,MIME_RELATED,
								 folderName,URLEscape(quoteName),fid,(*hdh)->cidHash,(*hdh)->relURLHash,(*hdh)->absURLHash); 
}

/************************************************************************
 * AddAttachInfo - attach a note about problems with the enclosure
 ************************************************************************/
void AddAttachInfo(short theIndex, long result)
{
	Str255 theMessage;
	
	ComposeString(theMessage,"\p%r%d\015", theIndex, result);
	PtrPlusHand_(theMessage+1,AttachedFiles,*theMessage);
}

/************************************************************************
 * WriteAttachNote - write the attachment note
 ************************************************************************/
OSErr WriteAttachNote(short refN)
{
	long size;
	short err = noErr;
	
	if (AttachedFiles && *AttachedFiles && (size=GetHandleSize_(AttachedFiles)))
	{
		if (!(err=EnsureNewline(refN)))
		{
			err = AWrite(refN,&size,LDRef(AttachedFiles));
			UL(AttachedFiles);
			SetHandleBig_(AttachedFiles,0);
		}
	}
	return(err);
}


/************************************************************************
 * POPLast - give the LAST command to find the last unread message
 ************************************************************************/
short POPLast(TransStream stream,short *lastRead)
{
	Str127 buffer;
	UPtr spot;
	long size = sizeof(buffer);
	
	if (Prr=POPCmdGetReply(stream,kpcLast,nil,buffer,&size)) return(Prr);
	ComposeLogS(LOG_LMOS,nil,"\pLast: %s",buffer);
	if (*buffer != '+') Prr = *buffer;
	else
	{
		strtok(buffer," "); 																	/* skip ok */
		if (spot=strtok(nil," \015"))
			*lastRead = Atoi(spot); 		/* read message size */
		else
			return(1);
	}
	return(0);
}

/************************************************************************
 * ReadPOPLine - read a line from the POP server.
 *  Returns the kind of line it is:
 *		plComplete	- a line that began with a newline
 *		plPartial - the remainder of a line
 *		plBlank - a blank line
 *		plEndOfMessage - the message is OVER.
 *		plError - there has been an error (recorded in global Prr)
 *  Also removes the "." that escapes lines beginning with ".",
 *   and adds a ">" to escape envelopes
 ************************************************************************/
POPLineType ReadPOPLine(TransStream stream,UPtr buf, long bSize, long *len)
{
	static wasNl;
	POPLineType returnType;
	long freeStack;

// sniff the stack to see if we're low
#ifdef THREADING_ON
	if (InAThread())
	{
		ThreadID threadID;

		GetCurrentThread (&threadID);
		ThreadCurrentStackSpace(threadID, &freeStack);
	}
	else
#endif
		freeStack = StackSpace();

	if (freeStack < kLowStackSize)
	{
		CommandPeriod = true;
		StackLowErr = true;
	}
	
	/*
	 * nil buffer initializes
	 */
	if (!buf)
	{
		wasNl = True;
		return(plEndOfMessage);
	}

	/*
	 * grab the line
	 */
	*len = bSize-1;		/* allow extra char for escaped envelopes */
	if (Prr = RecvLine(stream,buf,len)) return(plError);
	
	/*
	 * update progress indicator
	 */
	ByteProgress(nil,-*len,0);
	
	/*
	 * are we looking at the beginning of a line?
	 */
	if (wasNl)
	{
		returnType = plComplete;
		
		if (buf[0] == '.')									/* leading period */
		{
			if (buf[1]=='.')									/* if dot doubled, was in message data */
			{
				BMD(buf+1,buf,*len);
				--*len;
			}
			else if (buf[1] == '\015')					/* if dot followed by \015, end of message */
				returnType = plEndOfMessage;
		}
		else if (IsFromLine(buf))						/* is envelope? */
		{
			BMD(buf,buf+1,*len);				/* escape with '>' */
			++*len;
			buf[0] = '>';
		}
	}
	else
		returnType = plPartial;
	
	wasNl = *len ? buf[*len-1] == '\015' : True;						/* set for next go-round */
	
	return(returnType);
}

/************************************************************************
 * SplitMessage - split a message into pieces
 ************************************************************************/
short SplitMessage(short refN, long hStart, long hEnd, long msgEnd)
{
	long splitSize = GetRLong(FRAGMENT_SIZE);
	long bodySplit;
	short err;
	short count;
	short headerNl=0;
	long *froms=nil;
	long *tos=nil;
	Boolean *reals=nil;
	short i;
	Boolean headerReal;
	
	if (hEnd-hStart > splitSize)
	{
		/* uh-oh.  the header is waaaaaaay big */
		if (err = HuntNewline(refN,hStart+4096,&hEnd,&headerReal)) goto done;
		headerNl = headerReal?1:2;
	}
	
	/*
	 * how many splits do we need?
	 */
	bodySplit = splitSize - (hEnd-hStart);	/* how much of the body goes into each split */
	count = (msgEnd-hEnd+bodySplit-1)/bodySplit;	/* how many splits do we need? */
	bodySplit = (msgEnd-hEnd)/count;				/* divide them evenly */
	
	/*
	 * Ok, now let's find all the split locations
	 */
	if (!(froms = NuPtr((count+1)*sizeof(long *))) ||
			!(tos = NuPtr((count+1)*sizeof(long *))) ||
			!(reals = NuPtr((count+1)*sizeof(Boolean))))
	{
		WarnUser(MEM_ERR,err=MemError());
		goto done;
	}
	
	for (i=1,*froms=hEnd;i<count;i++)
	{
		if (err = HuntNewline(refN,hEnd+i*bodySplit,&froms[i],&reals[i]))
			goto done;
	}
	
	/*
	 * stuff the end of the message into the last split location,
	 * and the beginning into the first
	 */
	froms[count] = msgEnd;
	reals[count] = True;
	reals[0] = True;
	tos[0] = hEnd + headerNl;
	
	/*
	 * now, calculate where it all goes
	 */
	for (i=1;i<count;i++)
	{
		tos[i] = 	tos[i-1] +													/* start of last body */
							hEnd-hStart+headerNl +							/* header size */
							(reals[i]?0:1) + 										/* room for extra nl? */
							froms[i]-froms[i-1];								/* size of last body segment */
	}
	tos[count] = tos[count-1]+froms[count]-froms[count-1];
	
	/*
	 * copy the segments, one by one
	 */
	for (i=count-1;i>=0;i--)
	{
		if (tos[i] != froms[i])
		{
			/*
			 * copy body bytes
			 */
			if (err = CopyFBytes(refN,froms[i],froms[i+1]-froms[i],refN,tos[i]))
				{WarnUser(WRITE_MBOX,err); goto done;}
			
			if (i)
			{
				/*
				 * copy the header bytes
				 */
				if (err = CopyFBytes(refN,hStart,hEnd-hStart,refN,tos[i]-(hEnd-hStart+headerNl)))
					{FileSystemError(WRITE_MBOX,"",err); goto done;}
			}
			
			/*
			 * do we need to add an ending newline to the header?
			 */
			if (headerNl)
			{
				if (err = SetFPos(refN,fsFromStart,tos[i]-headerNl))
					{FileSystemError(WRITE_MBOX,"",err); goto done;}
				if (err = FSWriteP(refN,headerReal?Cr:"\p\015\015"))
					{FileSystemError(WRITE_MBOX,"",err); goto done;}
			}
		}
			
		/*
		 * do we need to add an ending newline to the body?
		 */
		if (!reals[i+1])
		{
			if (err = SetFPos(refN,fsFromStart,tos[i]+froms[i+1]-froms[i]))
				{FileSystemError(WRITE_MBOX,"",err); goto done;}
			if (err = FSWriteP(refN,Cr))
				{FileSystemError(WRITE_MBOX,"",err); goto done;}
		}
	}
	
	TruncOpenFile(refN,tos[count]);

done:
	if (tos) ZapPtr(tos);
	if (froms) ZapPtr(froms);
	if (reals) ZapPtr(reals);
	Prr = err;
	return(err ? 0 : count);
}

#ifdef POPSECURE
/************************************************************************
 * VetPOP - make sure the 's POP account is ok.
 ************************************************************************/
short VetPOP(void)
{
	Str255 , host;
	long port;
	short err;
	
	if (UUPCIn && !UUPCOut) {WarnUser(UUPC_SECURE,0);return(1);}
	GetPOPInfo(,host);
	port = GetRLong(POP_PORT);
	if ((err=StartPOP(host,port))==noErr)
	{
		(void) POPIntroductions();
		if (Prr) err=Prr;
	}
	EndPOP();
	if (UseCTB && !err) err = CTBNavigateSTRN(NAVMID);
	return(err);
}
#endif

#pragma segment MD5
static char hex[]="0123456789abcdef";
/************************************************************************
 * GenDigest - generate a digest for APOP
 ************************************************************************/
Boolean GenDigest(UPtr banner,UPtr secret,UPtr digest)
{
	Str255 stamp;
	MD5_CTX md5;
	short i;
	
	if (!*ExtractStamp(stamp,banner)) return(False);
	
	MD5Init(&md5);
	MD5Update(&md5,stamp+1,*stamp);
	MD5Update(&md5,secret+1,*secret);
	MD5Final(&md5);
	
	for (i=0;i<sizeof(md5.digest);i++)
	{
		digest[2*i+1] = hex[(md5.digest[i]>>4)&0xf];
		digest[2*i+2] = hex[md5.digest[i]&0xf];
	}
	digest[0] = 2*sizeof(md5.digest);
	return(True);
}

#define kmd5opad	(0x5C)
#define kmd5ipad	(0x36)
#define kmd5Len		(64)
/************************************************************************
 * GenKeyedDigest - generate a keyed digest for APOP
 ************************************************************************/
Boolean GenKeyedDigest(UPtr banner,UPtr secret,UPtr digest)
{
	Str255 stamp;
	MD5_CTX /*ipadMD5,*/ md5;
	short i;
/*	Str255 localS; */
	
	if (!*ExtractStamp(stamp,banner)) return(False);

/*	
	Zero(localS);
	BMD(secret+1,localS,*secret);
	for (i=0;i<kmd5Len;i++) localS[i] ^= kmd5ipad;
	
	MD5Init(&ipadMD5);
	MD5Update(&ipadMD5,localS,kmd5Len);
	MD5Update(&ipadMD5,stamp+1,*stamp);
	MD5Final(&ipadMD5);
	
	Zero(localS);
	BMD(secret+1,localS,*secret);
	for (i=0;i<kmd5Len;i++) localS[i] ^= kmd5opad;
		
	MD5Init(&md5);
	MD5Update(&md5,localS,kmd5Len);
	MD5Update(&md5,ipadMD5.digest,sizeof(ipadMD5.digest));
	MD5Final(&md5);
*/	
	hmac_md5(stamp+1,*stamp,secret+1,*secret,&md5.digest);

	for (i=0;i<sizeof(md5.digest);i++)
	{
		digest[2*i+1] = hex[(md5.digest[i]>>4)&0xf];
		digest[2*i+2] = hex[md5.digest[i]&0xf];
	}
	digest[0] = 2*sizeof(md5.digest);
	return(True);
}

/************************************************************************
 * ExtractStamp - grab the timestamp out of a POP banner
 ************************************************************************/
UPtr ExtractStamp(UPtr stamp,UPtr banner)
{
	UPtr cp1,cp2;
	
	*stamp = 0;
	banner[*banner+1] = 0;
	if (cp1=strchr(banner+1,'<'))
		if (cp2=strchr(cp1+1,'>'))
		{
			*stamp = cp2-cp1+1;
			strncpy(stamp+1,cp1,*stamp);
		}
	return(stamp);
}


/************************************************************************
 * New LMOS strategy
 ************************************************************************/

/************************************************************************
 * FillPOPD - fill the pop descriptor with important info from a header
 ************************************************************************/
void FillPOPD(POPDPtr pdp,HeaderDHandle hdh)
{
	Str255 msgId;
	uLong hash;
	
	if (pdp->uidHash==0)
	{
		pdp->receivedGMT = GMTDateTime();
		if (pdp->uidHash==kNeverHashed ||
				pdp->uidHash==kNoMessageId)
		{
			if (*HeaderMsgId(hdh,msgId))
				hash = MIDHash(msgId+1,*msgId);
			else
				hash = FakeMIDHash(hdh);
			pdp->uidHash = hash;
		}
	}
}

/************************************************************************
 * BuildPOPD - build the descriptor of the work we have to do with the
 *  pop server
 * -- HERE BE DRAGONS --
 *  be careful with this code.  It does things in a specific order for a reason
 ************************************************************************/
OSErr BuildPOPD(TransStream stream,POPDHandle *popDH,short count,XferFlags *flags,Boolean *capabilities)
{
	POPDesc new, old;
	short i;
	short spot;
	Boolean room;
	Boolean anyRoom = True;	/* we wouldn't be here if there weren't at least a little room */
	long skipSize = GetRLong(BIG_MESSAGE) K;
	POPDHandle oldDH = nil;
	Boolean sbm = PrefIsSet(PREF_NO_BIGGIES);
	Boolean lmos = PrefIsSet(PREF_LMOS);
	long spaceNeeded = 0;
	short lastRead = 0;				/* for the status or last methods */
	uLong age = GetPrefLong(PREF_LMOS_XDAYS);
	Boolean onFetch, onDelete;
	Boolean aged;
	Boolean plentyRoom;
	short popMode = GetPrefLong(PREF_POP_MODE);
	int total = 0;
	#ifdef THREADING_ON
	TOCHandle tempInTocH = nil;
	
	if (InAThread())
		tempInTocH = GetTempInTOC();
	#endif
	
	age = (age&&lmos) ? (GMTDateTime()-24*3600*age) : 0;
	/*
	 * allocate it
	 */
	if (!(*popDH = ZeroHandle(NuHandle(count*sizeof(POPDesc)))))
		return(WarnUser(MEM_ERR,Prr=MemError()));
	
	if (Prr=FillSizesWithList(stream,*popDH)) return(Prr);
		
	if (popMode!=popRUIDL) lastRead = FirstUnread(stream,count)-1;

	if (!(*CurPers)->noUIDL && (Prr = FillWithUidl(stream,*popDH)))
		return(Prr);
	
	ASSERT(CurResFile()==SettingsRefN);
	UseResFile(SettingsRefN);	// make sure!
	if (oldDH = (void*)Get1Resource(CUR_POPD_TYPE,POPD_ID)) HNoPurge((Handle)oldDH);
	else
	{
		Prr = ResError();
		if (Prr==resNotFound) Prr = noErr;
		if (Prr) return(WarnUser(BUILD_POPD,Prr));
	}

	if (LogLevel & LOG_LMOS) Log1POPD("\pBuildPOPD","\pOld",oldDH);
	
	if ((*CurPers)->noUIDL && (Prr = FillWithTop(stream,*popDH,oldDH))) return(Prr);
	
	for (i=0;i<count;i++) spaceNeeded += (**popDH)[i].msgSize;
	plentyRoom = !RoomForMessage(spaceNeeded);
	spaceNeeded = 0;
	
	/*
	 * examine each of the new messages
	 */
	for (i=0;i<count;i++)
	{
		MyThreadBeginCritical();
		CycleBalls();
		MyThreadEndCritical();
		aged = False;
		
		new = (**popDH)[i];
		
		/*
		 * have we seen it before?
		 */
		if (!oldDH || errNotFound==(spot=FindExistSpot(oldDH,new.uidHash)))
		{
			Zero(old);
			old.retred = i<lastRead;
			old.stubbed = i<lastRead;

#ifdef THREADING_ON
		/*
		 * if the message is already in in.temp and we don't have a popd entry, machine probably crashed
		 * during download before popd could get updated. mark these messages as fetched. make an exception
		 * for the last message, since it could be incomplete. remove this message and re-fetch it.
		 */
			if (InAThread())
			{
				short sum;
			
				if (tempInTocH)
				{
					sum = FindSumByHash (tempInTocH,new.uidHash);
					if (sum != -1)
					{
					 if (sum == ((*tempInTocH)->count - 1))
							{
								DeleteSum (tempInTocH, sum);
								old.retred = False;
								old.stubbed = False;
							}
							else
							{
								old.retred = True;
								new.retred = True;
							}
						}
					}
				}
#endif
		}
		else
		{
			old = (*oldDH)[spot];
			new = old;
			new.retr = False;
			new.stub = False;
			new.delete = False;
			if (popMode!=popRUIDL)
			{
				old.stubbed = old.stubbed || i<lastRead;
				old.retred = old.retred || i<lastRead;
			}
		}
		
		/*
		 * hard nuke?
		 */
		if (flags->nukeHard) new.delete = True;
		
		/*
		 * just stub?
		 */
		else if (flags->stub) {new.head = new.stub = True;}
		
		/*
		 * is this message supposed to be toast?
		 */
		else if (old.deleted) new.delete = True;		/* yes.  Just kill it. */
		else
		{
			/*
			 * ok, message is not just to be killed.  Make a rough pass on whether
			 * or not it should be fetched or deleted.  We'll refine our decision
			 * later
			 */
			
			/*
			 * check lists
			 */
			onFetch = IdIsOnPOPD(CUR_POPD_TYPE,FETCH_ID,new.uidHash);
			onDelete = IdIsOnPOPD(CUR_POPD_TYPE,DELETE_ID,new.uidHash);

			/*
			 * is there room?
			 */
			room = anyRoom && (plentyRoom || !RoomForMessage(spaceNeeded+new.msgSize));
			if (room) new.big2 = False;	/* clear old flag */
			
			/*
			 * Should we fetch it? (ROUGH)
			 */
			if (!old.retred) new.retr = flags->check;
			
			/*
			 * should we delete it? (ROUGH)
			 */
			aged = age && new.receivedGMT && age>new.receivedGMT;
			new.delete = !lmos ||
										flags->servDel && onDelete	||/* user told us to */
										aged && new.retred;
			
			/*
			 * ok, now we fine-tune the process.  Check that there is room for
			 * the message, and that we don't want to skip it because of the
			 * skip big messages preference.
			 */
			
			/*
			 * if we're skipping big messages, see if we want to skip this one
			 */
			if (!sbm) new.skip = False;	/* clear old flag */
			else if (new.msgSize>skipSize && skipSize > 0) new.skip = True;
			else new.skip = False;
			
			if (new.retr && new.skip && !onFetch)
			{
				new.retr = False;
				if (!old.stubbed)
				{
					new.stub = True;
					new.delete = False;	/* don't delete, because only fetching a stub */
				}
			}
			
			/*
			 * under what circumstances do we not fetch when we already have a stub?
			 *	We get stubs from:
			 *		Fetching headers - such stubs are never expanded except manually
			 *		Skip big messages - if the pref changes, the messages will be fetched
			 *		Not enough room - if the user makes room, the messages will be fetched
			 */
			if (old.stubbed && new.retr)
			{
				if (new.head || new.error) new.retr = False;	// if the user fetched headers, must request fetch
			}
			
			new.retr = new.retr || flags->servFetch && onFetch;

			/*
			 * If we want the whole message, do we have room?
			 */
			if (new.retr && !room)
			{
				new.big2 = True;
				new.delete = False;	/* since we want to fetch it but can't, don't delete it */
				new.retr = False;
				if (!old.stubbed) new.stub = True;	/* shd we fetch the stub? */
			}
		}
		
		/*
		 * maybe we don't even have room for the stub?
		 */
		if (!anyRoom && new.stub)
		{
			new.stub = False;	/* no room at the inn */
			new.delete = False;	/* again, since we want it but can't get it, don't delete it */
		}
		
		/*
		 * ok, we did it!  Now, record how much disk this message will use.
		 */
		if (new.stub) spaceNeeded += 3 K;	/* arbitrary conservative guess at stub size */
		else if (new.retr) spaceNeeded += new.msgSize;
		anyRoom = anyRoom && (plentyRoom || !RoomForMessage(spaceNeeded));

		/*
		 * special processing
		 */
		if (flags->nuke) new.delete = True;
		
		/*
		 * last sanity check
		 */
		if (!(new.retred || new.retr) && (!onDelete || onFetch) && !flags->nukeHard && !old.deleted) new.delete = False;
				
		/*
		 * store it back
		 */
		(**popDH)[i] = new;
		if (new.retr) 
			total+=new.msgSize;
		else
		if (new.stub)
			total+=3 K;
	}
	ByteProgress(nil,0,total);	
	POPDelDup(*popDH);
		
	if (LogLevel & LOG_LMOS) LogPOPD("\pBUILT",*popDH);
	
	return(noErr);
}

/**********************************************************************
 * POPDelDup - delete duplicate messages before downloading
 **********************************************************************/
void POPDelDup(POPDHandle popDH)
{
	short i,j,n;
	short killme;
	
	n = HandleCount(popDH);
	for (i=0;i<n;i++)
		for (j=i+1;j<n;j++)
			if ((*popDH)[i].uidHash && (*popDH)[i].uidHash==(*popDH)[j].uidHash)
			{
				if ((*popDH)[i].retred)
					killme = j;
				else if ((*popDH)[j].retred)
					killme = i;
				else if ((*popDH)[i].msgSize>(*popDH)[j].msgSize)
					killme = j;
				else
					killme = i;
				(*popDH)[killme].retr = (*popDH)[killme].stub = False;
				(*popDH)[killme].delete = True;
			}
}					

/************************************************************************
 * FillWithTop - fill up the descriptor without uidl.  Not fun.
 ************************************************************************/
OSErr FillWithTop(TransStream stream,POPDHandle new, POPDHandle old)
{
	short oldCount;
	short newCount = HandleCount(new);
	short oldSpot, newSpot;
	short oldUndelCount;
	short oldUndelSpot;
	
	if (old)
	{
		oldCount = HandleCount(old);
		/*
		 * find last undeleted message in old descriptor, and count them
		 */
		oldUndelCount = 0;
		for (oldSpot=0;oldSpot<oldCount;oldSpot++)
		{
			if (!(*old)[oldSpot].deleted && (*old)[oldSpot].uidHash)
			{
				oldUndelSpot = oldSpot;
				oldUndelCount++;
			}
		}
		
		/*
		 * I DON'T TRUST THIS
		 */
		if (!oldUndelCount) return(noErr); /* we will either delete or stub these, so we can stop now.  maybe */
		
		/*
		 * Now, check to see if it's in the same spot on the server
		 */
		if (oldUndelCount && newCount>=oldUndelCount)
		{
			Prr = FillPOPDFromServer(stream,new,oldUndelCount-1);
			if (Prr) return(Prr);
			
			if (MatchPOPD(old,oldUndelSpot,(*new)[oldUndelCount-1].uidHash))
			{
				/*
				 * ok, now we make the big leap of faith.  Assume that since we found
				 * this one where we expected it, the others will be there, too
				 */
				ComposeLogS(LOG_LMOS,nil,"\pCopy old to %d",oldUndelCount);
				for (newSpot=oldSpot=0;oldSpot<oldCount;oldSpot++)
				{
					if (!(*old)[oldSpot].deleted)
					{
						(*new)[newSpot].uidHash = (*old)[oldSpot].uidHash;
						(*new)[newSpot++].receivedGMT = (*old)[oldSpot].receivedGMT;
					}
				}
				
				/*
				 * NO TRUST HERE
				 */
				return(noErr);	/* we can stop now, we'll fetch or stub the rest.  maybe */
			}
		}
	}
	else
	/*
	 * DONT TRUST
	 */
	 return(noErr);	/* no old one, so don't need to top.  maybe */

	/*
	 * ok, so much for clever.  now, we act with force
	 */
	for (newSpot=0;!Prr && newSpot<newCount;newSpot++)
		Prr = FillPOPDFromServer(stream,new,newSpot);

	return(Prr);
}

/************************************************************************
 * FillPOPDFromServer - fille the POP Descriptor block by asking the server
 ************************************************************************/
OSErr FillPOPDFromServer(TransStream stream,POPDHandle popDH,short spot)
{
	long msgSize;
	Str255 scratch;
	HeaderDHandle hdh=nil;
	short refN = 0;
	Token822Enum tokenType;
	POPDesc pd;
	long size;
	
	if ((*popDH)[spot].uidHash) {return(noErr);}	/* already done */
	pd = (*popDH)[spot];
	
	ComposeRString(scratch,FIRST_UNREAD,spot+1);
	ProgressMessage(kpSubTitle,scratch);
	
	if (!(hdh=NewHeaderDesc(nil)))
		return(WarnUser(MEM_ERR,Prr=MemError()));

	/*
	 * get rest of message
	 */
	NumToString(spot+1,scratch);
	PLCat(scratch,1);
	size = sizeof(scratch);
	if (Prr=POPCmdGetReply(stream,kpcTop,scratch,scratch,&size)) goto done;

	/*
	 * read in the header
	 */
	tokenType = ReadHeader(stream,hdh,pd.msgSize,0,False);
	if (CommandPeriod || tokenType==ErrorToken) {Prr=ErrorToken; goto done;}
	
	if (tokenType!=EndOfMessage)
	{
		for (msgSize=sizeof(scratch);
				 !(Prr=RecvLine(stream,scratch,&msgSize)) && !POP_TERM(scratch,msgSize);
				 msgSize=sizeof(scratch));
	}
	
	/*
	 * fill the descriptor
	 */
	FillPOPD(&pd,hdh);
	ComposeLogS(LOG_LMOS,nil,"\pFill %d: hash %x gmt %x.",spot,pd.uidHash,pd.receivedGMT);
	
	(*popDH)[spot] = pd;
	
done:
	ZapHeaderDesc(hdh);
	return(Prr);
}

/************************************************************************
 * FindExistSpot - find a message already in a descriptor
 ************************************************************************/
short FindExistSpot(POPDHandle popDH,uLong hash)
{
	short spot=0;
	short count = GetHandleSize_(popDH)/sizeof(POPDesc);
	
	for (spot=0;spot<count;spot++)
		if (MatchPOPD(popDH,spot,hash)) return(spot);
	return(errNotFound);
}

/************************************************************************
 * FindUndelete - find a message already in a descriptor,
 *                without the delete flag if possible
 ************************************************************************/
short FindUndelete(POPDHandle popDH,uLong gmt,uLong hash)
{
	short spot;
	short count = GetHandleSize_(popDH)/sizeof(POPDesc);
	short found=errNotFound;
	
	for (spot=0;spot<count;spot++)
		if (MatchPOPD(popDH,spot,hash))
		{
			if (!(*popDH)[spot].delete) return(spot);
			found = spot;
		}
	return(found);
}


/************************************************************************
 * DisposePOPD - get rid of the POP descriptors
 ************************************************************************/
void DisposePOPD(POPDHandle *popDH)
{
	short err;
	Handle oldDH;
		
	if ((*popDH))
	{
		if (LogLevel & LOG_LMOS) LogPOPD("\pAFTER",*popDH);
		
		/*
		 * is the old one there?
		 */
		if (oldDH = GetResourceMainThread_(CUR_POPD_TYPE,POPD_ID))
		{
			HNoPurge(oldDH);
			
			/*
			 * is it the same as the new one?
			 */
			if (GetHandleSize(oldDH)==GetHandleSize_(*popDH) &&
					!memcmp(LDRef(oldDH),LDRef(*popDH),GetHandleSize(oldDH)))
			{
				/*
				 * yup.  pitch the new one.
				 */
				ComposeLogS(LOG_LMOS,nil,"\pNo changes");
				ZapHandle(*popDH);
				PurgeIfClean(oldDH);
				return;
			}
		}
		
		/*
		 * there was no old resource, or the old one is different
		 * kill the old one
		 */
		ZapSettingsResourceMainThread_(CUR_POPD_TYPE,POPD_ID);

		/*
		 * put in the new one
		 */
		err = AddMyResourceMainThread_(*popDH,CUR_POPD_TYPE,POPD_ID,"");
		
		/*
		 * bad things?
		 */
		if (err)
		{
			WarnUser(SAVE_POPD,err);
			ZapHandle(*popDH);
		}
		else
		{
		#ifdef THREADING_ON
			MyUpdateResFile(GetMainGlobalSettingsRefN ());
		#else
			MyUpdateResFile(SettingsRefN);
		#endif
			FixServers = True;
	
			/*
			 * done with the handle
			 */
			*popDH = nil;
		}	
	}
}

#ifdef TWO
/**********************************************************************
 * FixMessServerAreas - fix the server displays of message windows
 **********************************************************************/
void FixMessServerAreas(void)
{
	WindowPtr 	winWP;
	MyWindowPtr	win;
	
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsWindowVisible(winWP))
			if (win = GetWindowMyWindowPtr (winWP))
				Fix1MessServerArea (win);
}
#endif

/************************************************************************
 * HeaderMsgId - nab the message-id
 ************************************************************************/
PStr HeaderMsgId(HeaderDHandle hdh,PStr msgId)
{
	Str255 scratch;
	
	if (!AAFetchResData((*hdh)->funFields,InterestHeadStrn+hMessageId,scratch))
		PCopyTrim(msgId,scratch,128);
	else
		*msgId = 0;
	return(msgId);
}

/************************************************************************
 * FakeMIDHash - fake a message-id for something that doesn't have one
 ************************************************************************/
uLong FakeMIDHash(HeaderDHandle hdh)
{
	Str255 scratch;
	
	/*
	 * no message-id in this message.  Look for other headers of interest,
	 * and add them to our string one at a time
	 */
	*scratch = 0;
	AAFetchResData((*hdh)->funFields,InterestHeadStrn+hReceived,scratch);
	if (!*scratch)
		AAFetchResData((*hdh)->funFields,InterestHeadStrn+hDate,scratch);
	
	LDRef(hdh);
	PSCat(scratch,(*hdh)->who);
	PSCat(scratch,(*hdh)->subj);
	UL(hdh);
	
	return(Hash(scratch));
}

/************************************************************************
 * CountFetch - count the number of messages to be fetched
 ************************************************************************/
short CountFetch(POPDHandle popDH)
{
	short n;
	short fetch = 0;
	
	if (*popDH)
	{
		n = HandleCount(popDH);
		while (n--)
		{
			if ((*popDH)[n].retr || (*popDH)[n].stub) fetch++;
		}
	}
	return(fetch);
}

/************************************************************************
 * AddIdToPOPD - add a message to a POPD list
 ************************************************************************/
OSErr AddIdToPOPD(OSType theType,short listId, uLong uidHash,Boolean dupOk)
{
	POPDHandle resH;
	short n;
	short i;
	POPDesc popd;
	OSErr err;
	
	if (!ValidHash(uidHash)) return(errNotFound);
	resH = GetResourceMainThread_(theType,listId);
	if (!resH)
	{
		resH = NuHandle(0);
		if (resH) AddMyResourceMainThread_(resH,theType,listId,"");
		else return(resNotFound);
	}

	HNoPurge_(resH);
	
	n = HandleCount(resH);
	for (i=0;i<n;i++)
		if ((*resH)[i].uidHash==uidHash)
			return(noErr);
	
	/*
	 * not there.  add it.
	 */
	Zero(popd);
	popd.uidHash = uidHash;
	if (err=PtrPlusHand_(&popd,resH,sizeof(popd)))
		WarnUser(MEM_ERR,err);
	else
		ChangedResource((Handle)resH);
	PurgeIfClean(resH);
	return(noErr);
}

/************************************************************************
 * RemIdFromPOPD - remove a message from a POPD list
 ************************************************************************/
OSErr RemIdFromPOPD(OSType theType,short listId, uLong uidHash)
{
	POPDHandle resH = GetResourceMainThread_(theType,listId);
	short n;
	short i;
	OSErr err=errNotFound;
	
	if (!ValidHash(uidHash)) return(errNotFound);
	if (!resH) return(resNotFound);
	HNoPurge_(resH);
	
	n = HandleCount(resH);
	for (i=0;i<n;i++)
		if ((*resH)[i].uidHash==uidHash)
		{
			BMD((*resH)+i+1,(*resH)+i,(n-1-i)*sizeof(POPDesc));
			SetHandleBig_(resH,(n-1)*sizeof(POPDesc));
			ChangedResource((Handle)resH);
			err = noErr;
			break;
		}
	PurgeIfClean(resH);
	return(err);
}

/**********************************************************************
 * PrunePOPD - prune old stuff from fetch & delete lists
 **********************************************************************/
void PrunePOPD(OSType theType,short listId, POPDHandle onServer)
{
	POPDHandle resH = GetResourceMainThread_(theType,listId);
	short n;
	uLong uidHash;
	short sCount;
	short i;

	if (!resH) return;
	HNoPurge_(resH);
	n = HandleCount(resH);
	sCount = HandleCount(onServer);
	while (n--)
	{
		uidHash = (*resH)[n].uidHash;
		for (i=0;i<sCount;i++)
		{
			if ((*onServer)[i].uidHash==uidHash) break;
		}
		if (i==sCount)
		{
			ComposeLogS(LOG_LMOS,nil,"\pPrune %p: %d %x",listId%4==DELETE_ID%4?"\pDELETE":"\pFETCH",n,uidHash);
			RemIdFromPOPD(theType,listId,uidHash);
		}
	}
}

/************************************************************************
 * IdIsOnPOPD - is a message on a POPD list?
 ************************************************************************/
Boolean IdIsOnPOPD(OSType listType,short listId, uLong uidHash)
{
	POPDHandle resH = GetResourceMainThread_(listType,listId);
	short n;
	short i;
	Boolean result=False;
	
	if (!ValidHash(uidHash)) return(False);
	if (!resH) return(False);
	HNoPurge_(resH);
	
	n = HandleCount(resH);
	for (i=0;i<n;i++)
		if ((*resH)[i].uidHash==uidHash)
		{
			result = !(*resH)[i].deleted;
			break;
		}
	PurgeIfClean(resH);
	return(result);
}


/************************************************************************
 * FillWithUidl - fill the descriptor using the uidl command
 ************************************************************************/
OSErr FillWithUidl(TransStream stream,POPDHandle popDH)
{
	Str255 buffer;
	long size = sizeof(buffer);
	UPtr spot,end;
	short msgNum;
	short n = HandleCount(popDH);
	uLong uidHash;
	POPDHandle oldDH = GetResourceMainThread_(CUR_POPD_TYPE,POPD_ID);
	short i;

	for (i=n;i--;)
		(*popDH)[i].uidHash = 0;
	i = 0;
	
	if (Prr=POPCmdGetReply(stream,kpcUidl,nil,buffer,&size))
		return(Prr);

	if (*buffer=='-')
	{
		buffer[size] = 0;
		ComposeLogS(LOG_LMOS,nil,"\pUIDL err: %s",buffer);
		(*CurPers)->noUIDL = True;
		return(noErr);
	}
	
//	if (n>100) ByteProgress(nil,0,n);

	for (size=sizeof(buffer);
			 !(Prr=RecvLine(stream,buffer,&size)) && !POP_TERM(buffer,size);
			 size=sizeof(buffer))
	{
		MiniEvents(); if (CommandPeriod) break; CycleBalls();
		if (*buffer!=' ' && *buffer!='\t')
		{
			msgNum = Atoi(buffer);
			if (msgNum<1 || msgNum>n) continue;
			
//			if (n>100) ByteProgress(nil,-1,0);

#ifdef DEBUG
			if (RunType!=Production && ++i!=msgNum) AlertStr(OK_ALRT,Stop,"\pBad UIDL!");
#endif

			end = buffer+size;
			while (end[-1]<=' ') end--;
			for (spot=buffer;spot<end && *spot==' ';spot++);
			while(spot<end && *spot!=' ') spot++;
			while(spot<end && *spot==' ') spot++;
			if (spot<end)
			{
				spot[-1] = end-spot;
				uidHash = Hash(spot-1);
				(*popDH)[msgNum-1].uidHash = uidHash;
				(*popDH)[msgNum-1].receivedGMT = GMTDateTime();
				ComposeLogS(LOG_LMOS,nil,"\pUIDL %d �%p� %x",msgNum,spot-1,uidHash);
			}
		}
	}
//	if (n > 100 && !Prr) ByteProgress(nil,1,1);

	for (i=0;i<n;i++)
		if ((*popDH)[i].uidHash==0)
			FillPOPDFromServer(stream,popDH,i);
	
	Progress(NoBar,0,nil,nil,nil);

	if (CommandPeriod && !Prr) Prr = userCancelled;
	return(Prr);
}

/************************************************************************
 * LogPOPD - write the POPD's to the log file
 ************************************************************************/
void LogPOPD(PStr intro,POPDHandle newDH)
{
	Log1POPD(intro,"\pFetch",(void*)GetResource(CUR_POPD_TYPE,FETCH_ID));
	Log1POPD(intro,"\pDelete",(void*)GetResource(CUR_POPD_TYPE,DELETE_ID));
	Log1POPD(intro,"\pOld",(void*)GetResource(CUR_POPD_TYPE,POPD_ID));
	Log(LOG_LMOS,"\p---");
	Log1POPD(intro,"\pNew",newDH);
}

/************************************************************************
 * Log1POPD - write a POPD to the log file
 ************************************************************************/
void Log1POPD(PStr intro, PStr which, POPDHandle popDH)
{
	short i;
	short count;

	if (popDH==nil || GetHandleSize_(popDH)==0)
		ComposeLogS(LOG_LMOS,nil,"\p%p: %p: <empty>",intro,which);
	else
	{
		HNoPurge_(popDH);
		count = GetHandleSize_(popDH)/sizeof(POPDesc);
		for (i=0;i<count;i++)
		{
			CycleBalls();
			ComposeLogS(LOG_LMOS,nil,"\p%p: %p: %d hash %x gmt %x %c%c %c%c %c%c %c%c",
				intro,which,i+1,(*popDH)[i].uidHash,(*popDH)[i].receivedGMT,
				(*popDH)[i].retr ? 'F' : 'f',(*popDH)[i].retred ? 'F' : 'f',
				(*popDH)[i].stub ? 'S' : 's',(*popDH)[i].stubbed ? 'S' : 's',
				(*popDH)[i].big2 ? 'T' : 't',(*popDH)[i].skip ? 'K' : 'k',
				(*popDH)[i].delete ? 'D' : 'd',(*popDH)[i].deleted ? 'D' : 'd'
				);
		}
	}
}
	
/**********************************************************************
 * KerbDestroy - destroy the user's current ticket
 **********************************************************************/
OSErr KerbDestroy(void)
{
	if (gPOPKerbInited) 
	{
		KClientDisposeSessionCompat(&gSession);
		gPOPKerbInited = false;
	}
	return(KClientLogoutCompat());
}

/**********************************************************************
 * KerbDestroyUser - destroy the username
 **********************************************************************/
OSErr KerbDestroyUser(void)
{
	OSErr err = noErr;
	
	/* KerbDestroy does everything that it needs to */
	
	return (err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr KerbUsername(PStr name)
{
	OSErr err;
	UPtr atSign;
		
	*name = 0;	
	err = KClientGetUserNameDeprecated(name);

	if (err==cKrbNotLoggedIn)
	{
		/*
		 * log kerberos in
		 */
		 
		if (err = KClientNewSessionCompat(&gSession, 1/*nLocalAddress*/, GetRLong(POP_PORT) /*inLocalPort*/, 2 /*inRemoteAddress*/, GetRLong(POP_PORT)  /*inRemotePort*/)) return (err);
		if (err = KClientLoginCompat(&gSession, &gPrivateKey)) return(err);

		/*
		 * get username again
		 */
		if (noErr!=(err=KClientGetUserNameDeprecated(name))) return(err);
	}
		
	/*
	 * convert to pascal, trim realm
	 */
	if (!err)
	{
		c2pstr(name);
		if (atSign=PIndex(name,'@'))
			name[0] = atSign-name-1;
	}
			
	return(err);
}

/**********************************************************************
 * KerbGetTicket - get our ticket
 **********************************************************************/
OSErr KerbGetTicket(PStr serviceName,PStr inHost,PStr realm,PStr version,UHandle *ticket)
{
	Str63 fmt;
	Str255 fullName;
	Str255 scratch;
	Str255 host;
	Str255 shortHost;
	OSErr err;
	UPtr spot;
	struct hostInfo *hip, hi;
	unsigned long bufLen;
	
	// create a new session if we must
	err = InitKerberos();
	if (err != noErr) return (err);
	
	// get the user name from KCLient
	err = KerbUsername(scratch);
	if (err != noErr) return (err);
	
	/*
	 * the best thing in the world is four million DNS calls
	 */
	if (err=GetHostByName(inHost,&hip)) return(err);
	if (err=GetHostByAddr(&hi,hip->addr[0])) return(err);
	CtoPCpy(host,hi.cname);
	
	/*
	 * build the service name
	 */
	spot = host+1; PToken(host,shortHost,&spot,".");
	EscapeChars(host,GetRString(fmt,KERBEROS_ESCAPES));
	EscapeChars(serviceName,GetRString(fmt,KERBEROS_ESCAPES));
	GetRString(fmt,KERBEROS_SERVICE_FMT);
	utl_PlugParams(fmt,fullName,serviceName,host,realm,shortHost);
	ProgressMessage(kpMessage,ComposeRString(scratch,KERBEROS_TICK_FMT,fullName));
	
	// Null terminate these, KCLient expects C-Strings
	version[*version+1] = 0;
	fullName[*fullName+1] = 0;

	bufLen = GetRLong(KERBEROS_BSIZE);
	if (!(*ticket = NuHandle(bufLen))) return(MemError());
	
	/*
	 * call the driver
	 */
		
	LDRef(*ticket);
	err = KClientMakeSendAuthCompat(&gSession, fullName+1, **ticket, &bufLen, GetRLong(KERBEROS_CHECKSUM), version+1);
	UL(*ticket);


	/*
	 * adjust the ticket size
	 */
	if (err) ZapHandle(*ticket);
	else SetHandleSize(*ticket,bufLen);
	
	/*
	 * Cleanup
	 */

	return(err);
}

/**********************************************************************
 * InitKerberos - get ready to start making Kerberos calls
 **********************************************************************/
 OSErr InitKerberos()
 {
 	OSErr err = noErr;
 	
	if (!gPOPKerbInited)
	{
		// make sure Kerberos is present before we start calling it.
		if ((Ptr) (KClientNewSessionCompat) == (Ptr) (kUnresolvedCFragSymbolAddress))
		{
			// Kerberos is not installed.  Warn and crap out.
			WarnUser(NO_KERBEROS,err);
			err = fnfErr;
		}
		else
		{
			// Kerberos is here.  Try to start a new session.
			err = KClientNewSessionCompat(&gSession, 1, GetRLong(POP_PORT), 2, GetRLong(POP_PORT));
			if (err != noErr) return (err);
			else gPOPKerbInited = true;
		}
	}
	return (err);
}

/**********************************************************************
 * SendPOPTicket - send a ticket to the Pop server
 **********************************************************************/
OSErr SendPOPTicket(TransStream stream)
{
	Str63 popName,host,realm,version;
	Handle ticket=nil;
	OSErr err;
	
	if (err=GetPOPInfo(popName,host)) return(err);
	GetRString(popName,KERBEROS_POP_SERVICE);
	GetPref(realm,PREF_REALM);
	GetRString(version,KERBEROS_VERSION);
 
	if (err=KerbGetTicket(popName,host,realm,version,&ticket)) return(WarnUser(NO_KERBEROS,err));
	
	err=SendTrans(stream,LDRef(ticket),GetHandleSize(ticket),nil);
	ZapHandle(ticket);
	return(err);
}