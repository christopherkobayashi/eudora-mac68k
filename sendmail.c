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

#include "sendmail.h"
#include "myssl.h"
#define FILE_NUM 34
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for dealing with a sendmail (or other?) smtp server
 ************************************************************************/

#pragma segment SMTP
typedef enum
{
	helo=1, mail, rcpt, data, rset, send, soml, saml,
	vrfy, expn, help, noop, quit, turn, ehlo, auth, starttls
} SMTPEnum;

typedef enum {k1342LWSP,k1342Word,k1342Plain,k1342End} Enum1342;
typedef struct {
	Str255 word;
	Enum1342 wordType;
} Token1342, *Token1342Ptr;

typedef struct
{
	Boolean sawGreeting;
	long maxSize;
	Boolean mime8bit;
	Boolean pipeline;
	SASLEnum saslMech;
	Boolean starttls;
	Str255 digest;
} EhloStuff, *EhloStuffPtr, **EhloStuffHandle;

EhloStuffHandle Ehlo;

#define RingNext(pointer,array,size) ((array) + (((pointer)-(array))+1)%(size))
#define kSpecialSendDidntPanOut -1
#define CMD_BUFFER 1024
typedef struct wdsEntry *WDSPtr;
/************************************************************************
 * declarations for private routines
 ************************************************************************/
OSErr SendPtrHead(TransStream stream, Ptr label, long labelLen, Ptr body, long bodyLen, Boolean allowQP,short tid);
OSErr SendRawMIME(TransStream stream,FSSpecPtr spec);
OSErr SendEnriched(TransStream stream,UHandle text,DecoderFunc *encoder);
void EhloLine(UPtr line,long size);
int DoIntroductions(TransStream stream);
OSErr DoSMTPAuth(TransStream stream);
int SayByeBye(TransStream stream);
int SendCmd(TransStream stream, int cmd, UPtr args, AccuPtr argsAcc);
int SendCmdGetReply(TransStream stream, int cmd, UPtr args, Boolean chatter,MessHandle messH);
int SendHeaderLine(TransStream stream,MessHandle messH,short index,Boolean allowQP,short tid);
int SendAttachments(TransStream stream,MessHandle messH,long flags,PStr boundary,short tableID,short idBase);
OSErr SendNewsGroups(TransStream stream,AccuPtr newsGroupAcc,short tid);
OSErr SendCID(TransStream stream,MessHandle messH,long part,short n);
int SMTPCmdError(int cmd, UPtr args, UPtr message);
void PrimeProgress(MessHandle messH);
OSErr SendDigest(TransStream stream,FSSpecPtr spec);
int WannaSend(MyWindowPtr win);
short SendXSender(TransStream stream,MessHandle messH);
OSErr SendMIMEHeaders(TransStream stream,MessHandle messH,UHandle enriched,PStr boundary,emsMIMEHandle *tlMIME,Boolean isRelated);
OSErr SendContentType(TransStream stream,Handle text1, long offset1, Handle text2, long offset2, short tableID,long *flags,long *opts,PStr name,emsMIMEHandle *tlMIME,PStr subType);
long DecideEncoding(Handle text1, Handle text2,Boolean anyfunny,short etid,long flags);
Boolean SevenBitTable(short tableID);
Boolean Any2022(Handle text,long offset);
PStr NameCharset(PStr charset,short tid,emsMIMEHandle *tlMIME);
Boolean LongerThan(Handle text, short len);
Boolean LongerWordThan(Handle text, short len);
Handle Encode1342(UPtr source,long len,short lineLimit,short *charsOnLine,PStr nl,short tid);
void Encode1342String(PStr s,short tid);
void Next1342Word(UPtr *startP,UPtr end,Token1342Ptr current,PStr delim,Boolean *wasQuote,Boolean *encQuote);
OSErr SendAnonFTP(TransStream stream,FSSpecPtr spec);
OSErr SendSpecial(TransStream stream,FSSpecPtr spec,AttMapPtr amp);
OSErr SendAddressHead(TransStream stream,PETEHandle pte,HSPtr hs,Boolean allowQP,short tid);
OSErr SendNormalHead(TransStream stream,PETEHandle pte,HSPtr hs,Boolean allowQP,short tid);
OSErr SendSubjectHead(TransStream stream,PETEHandle pte,HSPtr hs, Boolean allowQP,short tid);
//OSErr SendPipeRCPT(TransStream stream,PStr newRecip,AccuPtr pipe,Boolean chatter);
OSErr SendPipeRCPT(TransStream stream,PStr newRecip,AccuPtr pipe,Boolean chatter, MessHandle messH);
OSErr PeriodEncoder(CallType callType,DecoderPBPtr pb);
long StuffPeriods(UPtr in, long inLen, UPtr out, PStr newLine, short *nlStatePtr);
int SendRelatedParts(TransStream stream,MessHandle messH,long flags,StackHandle parts,PStr boundary);
int SendAnAttachment(TransStream stream,MessHandle messH,long flags,Boolean canQP,Boolean plainText,short tableID,PStr boundary,FSSpecPtr spec,short multiID,short partID);
int SendAttachmentFolder(TransStream stream,MessHandle messH,long flags,Boolean canQP,Boolean plainText,short tableID,PStr boundary,FSSpecPtr folderSpec,short multiID,short partID,short *partBase,CInfoPBRec *hfi);
int sErr;
void ConvertPictPart(FSSpecPtr origSpec,FSSpecPtr spec);
OSErr FlattenAndSpool(FSSpecPtr spec);
OSErr FlattenQTMovie(FSSpecPtr inSpec,FSSpecPtr outSpec);
OSErr AllAttachOnBoard(MessHandle messH);

OSErr TransmitMessageMixed(TransmitPBPtr pb,Boolean topLevel);
OSErr TransmitMessageRelated(TransmitPBPtr pb,Boolean topLevel);
OSErr TransmitMessageText(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitMessageBody(TransmitPBPtr pb,Boolean withClosure);
OSErr TransmitMessageBodyHeaders(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitTopHeaders(TransmitPBPtr pb);
OSErr TransmitMultiHeaders(TransmitPBPtr pb,short subType,PStr boundary,short otherParm,PStr otherVal);
OSErr TransmitMessageTextBloat(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitMessageTextStrip(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitMessageTextPlain(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitMessageTextRich(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel);
OSErr TransmitMessageSig(TransmitPBPtr pb);
OSErr TransmitMessageSigBloat(TransmitPBPtr pb);
OSErr TransmitMessageSigBody(TransmitPBPtr pb,Boolean withHeaders);
OSErr TransmitMessageSigBodyPlain(TransmitPBPtr pb);

/************************************************************************
 * Public routines
 ************************************************************************/

/************************************************************************
 * StartSMTP - initiate a connection with the specified SMTP server
 ************************************************************************/
int StartSMTP(TransStream stream, UPtr serverName, long port)
{
	if (UUPCOut) sErr=UUPCPrime(serverName);
	else if (!(sErr=ConnectTrans(stream,serverName,port,False,GetRLong(OPEN_TIMEOUT))))
		sErr=DoIntroductions(stream);
	return(sErr);
}

/************************************************************************
 * MySendMessage - send a message to the SMTP server
 ************************************************************************/
int MySendMessage(TransStream stream,TOCHandle tocH,int sumNum,CSpecHandle specList)
{
	WindowPtr	messWinWP;
	Str255 buffer;
	Str255 param;
	MessHandle messH;
	Accumulator newsGroupAcc;
	
	Zero(newsGroupAcc);
	
	/*
	 * handle open, dirty windows
	 */
	if (!(messH=SaveB4Send(tocH,sumNum))) return(1);
	
	messWinWP = GetMyWindowWindowPtr((*messH)->win);
	/*
	 * Log, if we must
	 */
	GetWTitle(messWinWP,buffer);
	ComposeLogR(LOG_SEND,nil,SENDING,buffer);
	
#ifdef TWO
	if (PrefIsSet(PREF_POP_SEND))
	{
		long size=sizeof(buffer);
		*buffer = '+';
		if (POPCmdGetReply(stream,kpcXmit,"",buffer,&size) || *buffer!='+')
		{
			if (*buffer=='-') POPCmdError(kpcXmit,"",buffer);
			return(1);
		}
	}
	else
#endif
	{
		/*
		 * reset SMTP
		 */
		sErr = SendCmdGetReply(stream,rset,nil,True,messH);
		if (sErr/100 != 2) return(sErr);
		sErr = 0;
		
		/*
		 * envelope
		 */
		MessReturnAddr(messH,buffer);
		if (Ehlo && *(*Ehlo)->digest)
		{
			ComposeString(param,"\p %r=%p",EsmtpStrn+esmtpAmd5,LDRef(Ehlo)->digest);
			PSCat(buffer,param);
			UL(Ehlo);
		}
		if (Ehlo && (*Ehlo)->maxSize)
		{
			ComposeString(param,"\p %r=%d",EsmtpStrn+esmtpSize,ApproxMessageSize(messH) K);
			PSCat(buffer,param);
		}
		if (Ehlo && (*Ehlo)->mime8bit && PrefIsSet(PREF_ALLOW_8BITMIME))
		{
			ComposeRString(param,BODY_EQUALS,EsmtpStrn+esmtp8BMIME);
			PSCat(buffer,param);
		}
		sErr = SendCmdGetReply(stream,mail,buffer,True,messH);
		
		if (Ehlo && *(*Ehlo)->digest && IsAddrErr(sErr)) InvalidatePasswords(False,True,False);
		if (sErr/100 != 2) return(sErr);
		sErr = 0;
		
		if (WrapWrong) OffsetWindow(messWinWP);
		
		if (sErr=DoRcptTos(stream,messH,True,specList,&newsGroupAcc)) goto done;
	}
		
	(*messH)->newsGroupAcc = newsGroupAcc;
	if (sErr=TransmitMessageHi(stream,messH,True,!PrefIsSet(PREF_POP_SEND))) goto done;
	AccuZap(newsGroupAcc);
	Zero((*messH)->newsGroupAcc);
	
	//TimeStamp(tocH,sumNum,GMTDateTime(),ZoneSecs());
			
done:
	(void)ComposeLogR(LOG_SEND,nil,sErr?LOG_FAILED:LOG_SUCCEEDED,sErr);
	AccuZap(newsGroupAcc);
	return(sErr);
}

/**********************************************************************
 * MessReturnAddr - get the right return addr for this message; if addr
 *  is in "Me" nickname, returns from addr.  Else returns configured addr
 **********************************************************************/
PStr MessReturnAddr(MessHandle messH,PStr buffer)
{
	Str255 from;
	Str255 returnAddr;
	short template =  MessOptIsSet(messH,OPT_RECEIPT) && MessOptIsSet(messH,OPT_BULK) ? EMPTY_MFROM:NORMAL_MFROM;
	
	// return address usually
	GetReturnAddr(returnAddr,False);

	// or contents of from field
	if (!CompHeadGetStr(messH,FROM_HEAD,from) && !StringSame(returnAddr,from) && IsMe(from))
		PCopy(returnAddr,from);
	
	// strip <>'s
	ShortAddr(returnAddr,returnAddr);
	
	// and compose the argument
	ComposeRString(buffer,template,returnAddr);

	return(buffer);
}

/************************************************************************
 * EndSMTP - done talking to SMTP
 ************************************************************************/
int EndSMTP(TransStream stream)
{
	if (UUPCOut) UUPCDry(stream);
	else
	{
		SilenceTrans(stream,True);	// ignore all TCP/IP errors from here on out -jdboyd 030113
		if ((!sErr || (sErr<600 && sErr>=400)) && !(sErr=SayByeBye(stream)))
			DisTrans(stream);
		sErr = DestroyTrans(stream);
	}
	ZapHandle(eSignature);
	ZapHandle(RichSignature);
	ZapHandle(HTMLSignature);
	ZapHandle(Ehlo);
	return(sErr);
}

/************************************************************************
 * SMTPError - return the last SMTP error
 ************************************************************************/
int SMTPError(TransStream stream)
{
	return (sErr);
}

/************************************************************************
 * Private routines
 ************************************************************************/
/************************************************************************
 * DoIntroductions - take care of the beginning of the SMTP protocol
 ************************************************************************/
int DoIntroductions(TransStream stream)
{
	Str255 buffer;
	
	Ehlo = NewZH(EhloStuff);
	
	/*
	 * get banner from the remote end
	 */
	sErr = GetReply(stream,buffer,sizeof(buffer),True,False);
	if (sErr/100 != 2) return(sErr);
	sErr = 0;

SayHello :
	/*
	 * tell it who we are
	 */
	sErr = SendCmdGetReply(stream,ehlo,WhoAmI(stream,buffer),True,nil);
	if (sErr>=400) sErr = SendCmdGetReply(stream,helo,WhoAmI(stream,buffer),True,nil);
	
	if (sErr/100 == 2)
	{
#ifdef ESSL
		if( ShouldUseSSL(stream) && !(stream->ESSLSetting & esslSSLInUse))
		{
			if (!(*Ehlo)->starttls)
			{
				if(!(stream->ESSLSetting & esslOptional))
				{
					sErr = 502;
				//	SMTPCmdError(starttls,nil,GetRString(buffer,SSL_ERR_STRING)+1);
					ComposeStdAlert ( Note, ALRTStringsStrn+NO_SERVER_SSL );

				}
			}
			else
			{
				OSStatus sslErr;
				int tempErr;
				
				tempErr = SendCmdGetReply(stream,starttls,nil,(stream->ESSLSetting & esslOptional) != 0,nil);
				if(tempErr/100 == 2)
				{
					sslErr = ESSLStartSSL(stream);
					if(sslErr)
					{
						if(!(stream->ESSLSetting & esslOptional))
						{
							sErr = 554;
							SMTPCmdError(starttls,nil,GetRString(buffer,SSL_ERR_STRING)+1);
						}
					}
					else if(stream->ESSLSetting & esslSSLInUse)
					{
						ZeroHandle(Ehlo);
						goto SayHello;
					}
				}
				else if(!(stream->ESSLSetting & esslOptional))
				{
					sErr = tempErr;
				}
			}
		}
#endif
		if ((sErr/100 == 2))
		{
			if (!PrefIsSet(PREF_SMTP_AUTH_NOTOK))
			{
				short (*authfunc)(TransStream stream) = nil;
				
				if (!(*Ehlo)->saslMech)
				{
					ComposeLogS(LOG_PROTO,nil,"\pSMTP auth not available, disabling");
					SetPref(PREF_SMTP_GAVE_530,NoStr);
					SetPref(PREF_SMTP_DOES_AUTH,NoStr);
				}
				else
				{
					SetPref(PREF_SMTP_DOES_AUTH,YesStr);
					if (PrefIsSet(PREF_KERBEROS) || *(*CurPers)->password)
					{
						ComposeLogS(LOG_PROTO,nil,"\pSMTP auth %r under way...",SASLStrn+(*Ehlo)->saslMech);
						// We have everything we need to attempt authentication
						sErr = DoSMTPAuth(stream);
					}
					else
						ComposeLogS(LOG_PROTO,nil,"\pSMTP auth %r not being attempted, no credentials",SASLStrn+(*Ehlo)->saslMech);
				}
			}
			else if ((*Ehlo)->saslMech)
				ComposeLogS(LOG_PROTO,nil,"\pSMTP auth %r available but forbidden",SASLStrn+(*Ehlo)->saslMech);
		}
	}
	
	return((sErr/100 != 2) ? sErr :  (sErr=0));
}

/************************************************************************
 * DoSMTPAuth - authenticate for SMTP
 ************************************************************************/
OSErr DoSMTPAuth(TransStream stream)
{
	Accumulator chalAcc, respAcc;
	short rounds = 0;
	Str63 service;
	long state;
	
	Zero(chalAcc);
	Zero(respAcc);
	
	// put auth command in initial response
	AccuAddRes(&respAcc,EsmtpStrn+esmtpAuth);
	AccuAddChar(&respAcc,' ');
	
	// grab service name for kerberos
	GetRString(service,K5_SMTP_SERVICE);
	
	// run the mechanism
	do
	{
		// Build the response
		if (SASLDo(service,(*Ehlo)->saslMech,rounds++,&state,&chalAcc,&respAcc)) sErr = 601;
		else
		{
			// Send the response
			if (SendCmd(stream,0,nil,&respAcc)) sErr = 601;
			else
			{
				// get the reply
				sErr = GetReplyLo(stream,nil,0,&respAcc,false,false);
				chalAcc.offset = 0;
				if (sErr==334)
				{
					UPtr spot;
					
					// extract the challenge token
					AccuAddChar(&respAcc,0);
					spot = *respAcc.data;
					while (*spot && !IsWhite(*spot)) spot++;	// skip code
					
					// rest will be base64 or whitespace, the decoder won't care
					AccuAddFromHandle(&chalAcc,respAcc.data,spot-*respAcc.data,respAcc.offset-(spot-*respAcc.data)-1);
				}
			}
		}
		
		// the response needs no pre-loading anymore.
		respAcc.offset = 0;
	}
	while (sErr/100 == 3);
	
	// if the server rejects our auth, but doesn't actually
	// require auth for anything, pretend like nothing happened
	if (sErr/100==5 && !ShouldSMTPAuth()) sErr = 0;
	
	// Kill the accumulators
	AccuZap(chalAcc);
	AccuZap(respAcc);

	// Let the sasl mechanism know how it all came out
	SASLDone(service,(*Ehlo)->saslMech,rounds,&state,sErr);
	
	return sErr;
}

/************************************************************************
 * SayByeBye - take care of the end of the SMTP protocol
 ************************************************************************/
int SayByeBye(TransStream stream)
{ 
	sErr = SendCmdGetReply(stream,quit,nil,False,nil);
	return((sErr/100 != 2) ? sErr : (sErr=0));	
}

/************************************************************************
 * SendCmd - send an smtp command, with optional arguments
 ************************************************************************/
int SendCmd(TransStream stream, int cmd, UPtr args, AccuPtr argsAcc)
{
	Byte buffer[CMD_BUFFER];

	GetRString(buffer,SMTP_STRN+cmd);
	if (args && *args)
		PCat(buffer,args);
	if (cmd) ProgressMessage(kpMessage,buffer); 
	
	if (!argsAcc)
	{
		PCat(buffer,NewLine);
		if (sErr=SendPString(stream,buffer)) return(sErr);
	}
	else
	{
		// send the command
		if (cmd) PCatC(buffer,' ');
		if (sErr=SendPString(stream,buffer)) return(sErr);
		
		// add a newline to the accumulator
		AccuAddStr(argsAcc,NewLine);
		
		// send the data
		sErr = SendTrans(stream,LDRef(argsAcc->data),argsAcc->offset,nil);
		
		// erase what we did to the accumulator
		UL(argsAcc->data);
		argsAcc->offset -= *NewLine;
		
		if (sErr) return sErr;
	}
		
	return(noErr);
}

/************************************************************************
 * SMTPCmdError - report an error for an SMTP command
 ************************************************************************/
int SMTPCmdError(int cmd, UPtr args, UPtr message)
{
	Str255 theCmd;
	Str255 theError;
	int err;

	GetRString(theCmd,2800+cmd);
	if (args && *args)
		PCat(theCmd,args);
	strcpy(theError+1,message);
	*theError = strlen(theError+1);
	if (theError[*theError]=='\012') (*theError)--;
	if (theError[*theError]=='\015') (*theError)--;
	MyParamText(theCmd,theError,"\pSMTP","");
	err = ReallyDoAnAlert(PROTO_ERR_ALRT,Note);
	return(err);
}

/************************************************************************
 * SendCmdGetReply - send an smtp command, with optional arguments, and
 * wait for the reply.	Returns reply code.
 ************************************************************************/
int SendCmdGetReply(TransStream stream, int cmd, UPtr args, Boolean chatter,MessHandle messH)
{
	char buffer[CMD_BUFFER];
	
	if (sErr=SendCmd(stream,cmd,args,nil)) return(601);			/* error in transmission */
	sErr = GetReply(stream,buffer,sizeof(buffer),False,cmd==ehlo);
	if (cmd==rcpt && sErr>499 && sErr<600) sErr = 550;
	if (sErr>399 && sErr<=600 && (IsAddrErr(sErr)||cmd!=rcpt) && cmd!=ehlo && chatter)
		SMTPCmdError(cmd?cmd:auth,(cmd&&cmd!=auth)?args:nil,buffer);
	if (messH && IsAddrErr(sErr) && cmd!=rcpt)
	{
		if (strchr(buffer,'\015')) *strchr(buffer,'\015') = 0;
		AddOutgoingMesgError((*messH)->sumNum,SumOf(messH)->uidHash,sErr,BAD_XMIT_ERR_TEXT,"",buffer);
	}
	return(sErr);
}


/************************************************************************
 * EhloLine - process an Ehlo return
 ************************************************************************/
void EhloLine(UPtr line,long size)
{
	Str63 directive;
	Str255 value,digest;
	UPtr start,end,stop;
	long longVal;
	
	if (!Ehlo) return;
	
	if ((*Ehlo)->sawGreeting)
	{
		(*Ehlo)->sawGreeting = True;
		return;
	}
	
	// Stupid AUTH= nonsense
	GetRString(value,EsmtpStrn+esmtpAuth);
	PCatC(value,'=');
	if (start=PPtrFindSub(value,line,size))
		start[4] = ' ';
	
	/*
	 * grab the Ehlo directive
	 */
	end = line+size;
	start = line+4;
	while (start<end && IsWhite(*start)) start++;
	for (stop=start;stop<end && !IsWhite(*stop);stop++);
	MakePStr(directive,start,stop-start);
	if (directive[*directive]=='\r') --*directive;
	
	/*
	 * and the value
	 */
	while (stop<end && IsWhite(*stop)) stop++;
	MakePStr(value,stop,size-(stop-line));
	
	/*
	 * now what?
	 */
	switch(FindSTRNIndex(EsmtpStrn,directive))
	{
		case esmtpSize:
			(*Ehlo)->maxSize = 0x7fffffff;
			if (*value <= 9) // avoid rilly big numbers
			{
				StringToNum(value,&longVal);
				if (longVal > 1 K) (*Ehlo)->maxSize = longVal;
				ComposeLogS(LOG_PROTO,nil,"\pESMTP size %d",longVal);
			}
			else
				ComposeLogS(LOG_PROTO,nil,"\pESMTP size invalid (%p)",value);
			break;
		case esmtp8BMIME:
			(*Ehlo)->mime8bit = True;
			ComposeLogS(LOG_PROTO,nil,"\pESMTP mime8bit");
			break;
		case esmtpAuth:
			{
				SASLEnum mech = (*Ehlo)->saslMech;
				UPtr spot = value+1;
				Str31 service;
				
				GetRString(service,K5_SMTP_SERVICE);
				
				while (PToken(value,directive,&spot," \011\012\015"))
					mech = SASLFind(service,directive, mech);
				
				(*Ehlo)->saslMech = mech;
				if (mech) ComposeLogS(LOG_PROTO,nil,"\pESMTP SASL mech %r",SASLStrn+mech);
			}
			break;
		case esmtpAmd5:
			SetPref(PREF_SMTP_DOES_AUTH,YesStr);
			PSCopy(directive,(*CurPers)->password);
			GenDigest(value,directive,digest);
			PCopy((*Ehlo)->digest,digest);
			ComposeLogS(LOG_PROTO,nil,"\pESMTP AMD5 auth found");
			break;
		case esmtpPipeline:
			(*Ehlo)->pipeline = True;
			ComposeLogS(LOG_PROTO,nil,"\pESMTP pipeline");
			break;
		case esmtpStartTLS:
			(*Ehlo)->starttls = True;
			ComposeLogS(LOG_PROTO,nil,"\pESMTP starttls");
			break;
		default:
			ComposeLogS(LOG_PROTO,nil,"\pESMTP has NFI what �%p� is supposed to mean",directive);
			break;
	}
}

/************************************************************************
 * DoRcptTos - tell the remote sendmail who is getting the message
 ************************************************************************/
int DoRcptTos(TransStream stream,MessHandle messH, Boolean chatter, CSpecHandle fccList,AccuPtr newsGroupAcc)
{
	sErr=DoRcptTosFrom(stream,messH,TO_HEAD,chatter,fccList,newsGroupAcc);
	if (sErr) return(sErr);
	sErr=DoRcptTosFrom(stream,messH,BCC_HEAD,chatter,fccList,newsGroupAcc); 
	if (sErr) return(sErr);
	sErr=DoRcptTosFrom(stream,messH,CC_HEAD,chatter,fccList,newsGroupAcc);
	return(sErr);
}

/************************************************************************
 * DoRcptTosFrom - do the Rcpt to's from a particular TERec
 ************************************************************************/
int DoRcptTosFrom(TransStream stream, MessHandle messH, short index, Boolean chatter,CSpecHandle fccSpecs,AccuPtr newsGroupAcc)
{
	Str255 toWhom;
	UHandle addresses=nil;
	UHandle rawAddresses;
	UPtr address;
	HeadSpec hs;
	UHandle text;
	Boolean evilSendmail=PrefIsSet(PREF_EVIL_SENDMAIL);
	UPtr spot;
	Str255 server;
	long junk;
	Accumulator pipe;
	short err;
	
	Zero(pipe);
	
	if (evilSendmail)
	{
		GetSMTPInfo(server);
		if (DotToNum(server,&junk))
		{
			/* ip address; turn into domain literal */
			PInsert(server,sizeof(server),"\p[",server+1);
			PCatC(server,']');
		}
	}

	sErr = 550;
	if (CompHeadFind(messH,index,&hs) && !CompHeadGetText(TheBody,&hs,&text))
	{
		if (!(sErr=SuckAddresses(&rawAddresses,text,False,True,False,nil)))
		{
			sErr=200;
			if (**rawAddresses)
			{
				ExpandAliases(&addresses,rawAddresses,0,False);
				ZapHandle(rawAddresses);
				if (!addresses)
				{
					AddOutgoingMesgError ((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, BAD_ADDRESS);
					return(sErr=550);
				}
				for (address=LDRef(addresses); *address; address += *address + 2)
				{
					/*
					 * skip groups
					 */
					if (address[*address]==':' || address[1]==';') continue;	/* skip group identifiers */

					/*
					 * handle Fcc's
					 */
					//Folder Carbon Copy - do not support FCC in Light					
					if (HasFeature (featureFcc)) {
						if (IsFCCAddr(address))
						{
							if (fccSpecs)
							{
								if (sErr=AddFccToList(address,fccSpecs)) break;
							}
							continue;
						}
						else if (IsNewsgroupAddr(address))
						{
							if (newsGroupAcc)
							{
								sErr = noErr;
								if (newsGroupAcc->offset) sErr = AccuAddRes(newsGroupAcc,COMMA_SPACE);
								if (!sErr) sErr=AccuAddPtr(newsGroupAcc,address+2,*address-1);
								if (sErr) break;
							}
							continue;
						}
					}

					if (*address > MAX_ALIAS)
					{
						sErr = 550;
						AddOutgoingMesgError ((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, BAD_ADDRESS);
						break;
					}
					if (UUPCOut)
					{
						if (sErr=UUPCWriteAddr(address)) break;
					}
					else
					{
						toWhom[0] = 1; toWhom[1] = '<';
						if (evilSendmail && *toWhom+*server+6<sizeof(toWhom))
						{
							for (spot=address+*address;spot>address;spot--)
								if (*spot=='@') *spot = '%';
						}
						PCat(toWhom,address);
						if (evilSendmail && *toWhom+*server+6<sizeof(toWhom))
						{
							PCatC(toWhom,'@');
							PSCat(toWhom,server);
						}
						PCatC(toWhom,'>');
						
						if (Ehlo && (*Ehlo)->pipeline)
							sErr = SendPipeRCPT(stream,toWhom,&pipe,chatter, messH);
						else
						{
							char buffer[CMD_BUFFER];
							
							if (sErr=SendCmd(stream,rcpt,toWhom,nil)) sErr = 601;			/* error in transmission */
							sErr = GetReply(stream,buffer,sizeof(buffer),False,false);
							if (sErr>499 && sErr<600) sErr = 550;
							if (IsAddrErr(sErr))
							{
								c2pstr(buffer); if (buffer[*buffer]=='\r') --*buffer;
								AddOutgoingMesgError ((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, BAD_ADDRESS_ERR_TEXT,address,buffer);
							}
						}
						if (sErr/100 != 2) {chatter = False;break;}
					}
				}
				ZapHandle(addresses);
				if (Ehlo && (*Ehlo)->pipeline)
				{
					err = SendPipeRCPT(stream,nil,&pipe,chatter, messH);
					if (sErr/100==2 || !sErr) sErr = err;
				}
			}
			else
				ZapHandle(rawAddresses);
		}
		else
		{
			sErr = 550;
			AddOutgoingMesgError ((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, BAD_ADDRESS);
		}
		ZapHandle(text);
	}
	return(sErr/100!=2 ? sErr : (sErr=0));
}

/************************************************************************
 * SendPipeRCPT - send rcpt to commands in a pipeline
 ************************************************************************/
OSErr SendPipeRCPT(TransStream stream,PStr newRecip,AccuPtr pipe,Boolean chatter, MessHandle messH)
{
	OSErr err = 200;
	OSErr firstErr = 200;
	char buffer[CMD_BUFFER];
	Str255 address;
	
	/*
	 * reap outstanding rcpts if need be
	 */
	while (!newRecip && pipe->offset || pipe->offset > 3 K)
	{
		PCopy(address,*pipe->data);
		err = GetReply(stream,buffer,sizeof(buffer),chatter,false);
		if (err>499 && err<600) err = 550;
		if (firstErr/100==2) firstErr = err;
		if (err/100 != 2)
		{
			if (chatter && IsAddrErr(err))
			{
				c2pstr(buffer); if (buffer[*buffer]=='\r') --*buffer;
				AddOutgoingMesgError ((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, err, BAD_ADDRESS_ERR_TEXT,address,buffer);
			}
			chatter = False;
		}
		// take the first address out of the accumulator
		BMD(*pipe->data+*address+1,*pipe->data,pipe->offset-*address-1);
		pipe->offset -= *address+1;
	}
	
	/*
	 * send current address
	 */
	if (err/100 == 2 && newRecip)
	{
		if (err=SendCmd(stream,rcpt,newRecip,nil)) return(601);			/* error in transmission */
		if (err=AccuAddPtr(pipe,newRecip,*newRecip+1))
		{
			WarnUser(MEM_ERR,err);
			return(601);
		}
	}
	return(firstErr);
}

/**********************************************************************
 * AddFccToList - add a mailbox to the fcc list.
 **********************************************************************/
OSErr AddFccToList(PStr fcc,CSpecHandle list)
{
	FSSpec spec;
	Str255 name;
	Str15 prefix;
	OSErr err;

	UseFeature (featureFcc);
	PSCopy(name,fcc);
	if (name[1]=='"')
	{
		BMD(name+2,name+1,*name-1);
		*name -= 2;
	}
	
	TrimPrefix(name,GetRString(prefix,FCC_PREFIX));
	
	if (err=BoxSpecByName(&spec,name))
		return(FileSystemError(NOT_MAILBOX,name,err));
	
	AddSpecToList(&spec,list);
	
	if (err=MemError()) WarnUser(MEM_ERR,err);
	return(err);
}	

/************************************************************************
 * TransmitMessage - send a message to the remote sendmail
 ************************************************************************/
int TransmitMessage(TransStream stream, MessHandle messH, Boolean chatter,Boolean mime,Boolean others,emsMIMEHandle *tlMIME,Boolean sendDataCmd)
{
	return TransmitMessageLo(stream, messH, chatter, mime, others, tlMIME, sendDataCmd, !UUPCOut, true);
}

/************************************************************************
 * TransmitMessageLo - send a message to the remote sendmail
 ************************************************************************/
int TransmitMessageLo(TransStream stream, MessHandle messH, Boolean chatter,Boolean mime,Boolean others,emsMIMEHandle *tlMIME,Boolean sendDataCmd,Boolean finishSMTP,Boolean doTopLevel)
{
	TransmitPB pb;
	FSSpec spec;
	Str255 scratch;
	FSSpec	errSpec;
	
	Zero(pb);
	pb.messH = messH;
	pb.mime = mime;
	pb.others = others;
	pb.receipt = MessOptIsSet(messH,OPT_BULK) && MessOptIsSet(messH,OPT_RECEIPT);
	pb.parts = nil;
	pb.isRelated = false;
	pb.hasAttachments = 1!=GetIndAttachment(messH,1,&spec,&pb.hs);
	pb.flags = SumOf(messH)->flags;
	pb.opts = SumOf(messH)->opts;
	pb.stream = stream;
	pb.tlMIME = tlMIME;
	pb.html = MessOptIsSet(messH,OPT_HTML);
	pb.rich = MessFlagIsSet(messH,FLAG_RICH);
	pb.allLWSP = !pb.hasAttachments && !pb.html && !pb.rich && IsAllLWSPMess(messH);
	pb.hasSig = !pb.allLWSP && (SumOf(messH)->sigId!=-1) && !MessOptIsSet(messH,OPT_INLINE_SIG) && eSignature && *eSignature && GetHandleSize(eSignature);
	pb.strip = !pb.allLWSP && MessOptIsSet(messH,OPT_STRIP) || MessOptIsSet(messH,OPT_JUST_EXCERPT);
	pb.bloat = !pb.allLWSP && MessOptIsSet(messH,OPT_BLOAT);
	if (pb.allLWSP) pb.flags &= ~FLAG_WRAP_OUT;
	
	Zero(pb.enriched);
	if (tlMIME && NewTLMIME(tlMIME)) goto fail;

	PrimeProgress(messH);
	
	//Make sure we have all the attachments
	if (sErr = AllAttachOnBoard(messH)) goto fail;
	
	CompHeadFind(messH,0,&pb.hs);
	
	// Add back in the inline sig if removed...
	pb.hs.stop = PeteLen(TheBody);

	/*
	 * rich?
	 */
	if (pb.mime && !pb.strip && pb.html)
	{
		if (sErr=AccuInit(&pb.enriched)) goto fail;
		if (sErr=HTMLPreamble(&pb.enriched,PCopy(scratch,SumOf(messH)->subj),0,False))
			goto fail;
		if (sErr=StackInit(sizeof(FSSpec),&pb.parts)) goto fail;
		Zero(errSpec);
		if (sErr=BuildHTML(&pb.enriched,TheBody,nil,pb.hs.stop,pb.hs.value,nil,nil,1,CompGetMID(messH,scratch),pb.parts,&errSpec))
		{
			if (errSpec.vRefNum)
			{
				//	Report error with graphic file
				AddOutgoingMesgError((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, GRAPHIC_FILE_ERR,sErr,errSpec.name);
			}
			sErr = 543;
			goto fail;
		}
		AccuTrim(&pb.enriched);
		pb.isRelated = pb.parts && (*pb.parts)->elCount > 0;
	}
	else if (pb.mime && !pb.strip && pb.rich)
	{
		if (sErr=AccuInit(&pb.enriched)) goto fail;
		if (BuildEnriched(&pb.enriched,TheBody,nil,pb.hs.stop,pb.hs.value,nil,False))
			goto fail;
	}
	
	if (sendDataCmd)
	{
		sErr = SendCmdGetReply(stream,data,nil,True,messH);
		if (sErr && sErr/100!=3) goto fail;
	}

	if (pb.hasAttachments) sErr = TransmitMessageMixed(&pb,doTopLevel);
	else if (pb.isRelated) sErr = TransmitMessageRelated(&pb,doTopLevel);
	else sErr = TransmitMessageText(&pb,true,doTopLevel);
	
	if (sErr) goto fail;
	
	if (pb.mime) if (finishSMTP) sErr = FinishSMTP(stream,pb.messH);

done:
	ZapHandle(pb.enriched.data);
	ZapHandle(pb.parts);
	return(sErr/100 != 2 ? sErr : (sErr=0));
	
fail:
	if (IsAddrErr(sErr))
		SumOf(messH)->state = MESG_ERR;
	else
	 	sErr = 600;
	if (tlMIME) ZapTLMIME(*tlMIME);
	goto done;
}

/************************************************************************
 * AllAttachOnBoardLo - do we have all the attachments? No error reporting.
 ************************************************************************/
OSErr AllAttachOnBoardLo(MessHandle messH, Boolean errReport)
{
	short index;
	FSSpec spec;
	OSErr err = noErr;
	
	for (index=1;!err;index++)
	{
		if (err=GetIndAttachment(messH,index,&spec,nil))
			if ((err!=1) && errReport)
			{
				AddOutgoingMesgError((*messH)->sumNum, (*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, err, ATTACH_MESS_ERR,err,spec.name);
				FileSystemError(BINHEX_OPEN,spec.name,err);
			}
	}
	if (err==1) err = noErr;
	else if (err) err = 543;
	return(err);
}

/************************************************************************
 * AllAttachOnBoard - do we have all the attachments?
 ************************************************************************/
OSErr AllAttachOnBoard(MessHandle messH)
{
	return AllAttachOnBoardLo(messH, true);
}

/************************************************************************
 * TransmitMessageMixed - transmit a multipart/mixed message
 ************************************************************************/
OSErr TransmitMessageMixed(TransmitPBPtr pb,Boolean topLevel)
{
	OSErr err;
	Str127 boundary;
	
	// build the boundary
	BuildBoundary(pb->messH,boundary,"");
	
	// mime-version first of all
	if (topLevel) if (err=TransmitMimeVersion(pb)) return(err);
	
	// send the non-mime headers
	if (topLevel) if (err=TransmitTopHeaders(pb)) return(err);

	// send the mime headers
	if (pb->mime && (err=TransmitMultiHeaders(pb,MIME_MIXED,boundary,0,nil))) return(err);
	
	if (pb->mime)
	{
		// send the header/body separator
		if (err=SendPString(pb->stream,NewLine)) return(err);
		
		// send the first boundary
		if (err=SendBoundary(pb->stream)) return(err);
		
		// send the message itself
		if (pb->isRelated)
			err = TransmitMessageRelated(pb,false);
		else
			err = TransmitMessageText(pb,false,false);
		if (err) return(err);
		
		// send the attachments
		err = SendAttachments(pb->stream,pb->messH,pb->flags,boundary,SumOf(pb->messH)->tableId,pb->isRelated ? (*pb->parts)->elCount : 0);
		if (err) return(err);
		
		// signature?
		if (pb->hasSig)
		{
			// attachment/signature boundary
			if (err=SendBoundary(pb->stream)) return(err);
			if (err=TransmitMessageSig(pb)) return(err);
		}
		
		// and the final boundary
		PCat(boundary,"\p--");
		if (err=SendBoundary(pb->stream)) return(err);
	}
	
	return(noErr);
}

/************************************************************************
 * TransmitMessageRelated - transmit a multipart/related message
 ************************************************************************/
OSErr TransmitMessageRelated(TransmitPBPtr pb,Boolean topLevel)
{
	OSErr err;
	Str127 boundary;
	Str31 textSlashHtml;
	
	// build the boundary
	BuildBoundary(pb->messH,boundary,"\pmr");
	
	// mime-version first of all
	if (topLevel) if (err=TransmitMimeVersion(pb)) return(err);
	
	// send the non-mime headers
	if (topLevel) if (err=TransmitTopHeaders(pb)) return(err);
		
	// send the mime headers
	if (pb->mime) if (err=TransmitMultiHeaders(pb,MIME_RELATED,boundary,AttributeStrn+aType,ComposeRString(textSlashHtml,THING_SLASH_THING,MIME_TEXT,HTMLTagsStrn+htmlTag))) return(err);

	if (pb->mime)
	{
		// send the header/body separator
		if (err=SendPString(pb->stream,NewLine)) return(err);
		
		// send the first boundary
		if (err=SendBoundary(pb->stream)) return(err);
		
		// send the message itself
		err = TransmitMessageText(pb,!pb->hasAttachments,false);
		if (err) return(err);
		
		// send the parts
		err = SendRelatedParts(pb->stream,pb->messH,pb->flags,pb->parts,boundary);
		if (err) return(err);
			
		// and the final boundary
		PCat(boundary,"\p--");
		if (err=SendBoundary(pb->stream)) return(err);
	}
	
	return(noErr);
}

/************************************************************************
 * TransmitMessageText - transmit body and sig, with or without bloat
 ************************************************************************/
OSErr TransmitMessageText(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel)
{
	OSErr err;
	
	if (pb->bloat && !pb->strip && (pb->html||pb->rich))
	{
		// The user wants multipart/alternative
		if (err = TransmitMessageTextBloat(pb,sigToo,topLevel)) return(err);
	}
	else if (!pb->strip && (pb->html||pb->rich))
	{
		// The user wants the rich version
		if (err=TransmitMessageTextRich(pb,sigToo,topLevel)) return(err);
	}
	else
	{
		// The user is chicken
		if (err=TransmitMessageTextStrip(pb,sigToo,topLevel)) return(err);
	}
	return(noErr);
}
	
/************************************************************************
 * TransmitMessageTextBloat - transmit body and possibly sig with bloat
 ************************************************************************/
OSErr TransmitMessageTextBloat(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel)
{
	OSErr err;
	Str127 boundary;

	// we'll need a boundary
	BuildBoundary(pb->messH,boundary,"\pma");
	
	// mime-version first of all
	if (topLevel) if (err=TransmitMimeVersion(pb)) return(err);

	// send the non-mime headers
	if (topLevel) if (err=TransmitTopHeaders(pb)) return(err);
		
	// send the part headers, header/body separator, and initial boundary
	if (pb->mime) if (err=TransmitMultiHeaders(pb,MIME_ALTERNATIVE,boundary,0,nil)) return(err);

	if (pb->mime)
	{
		// now the header/body separator and the initial boundary
		if (err=SendPString(pb->stream,NewLine)) return(err);
		if (err=SendBoundary(pb->stream)) return(err);
		
		// send the stripped version, not top-level
		if (err=TransmitMessageTextStrip(pb,sigToo,false)) return(err);
		
		// now the mid boundary
		if (err=SendBoundary(pb->stream)) return(err);
		
		// send the rich version, not top-level
		if (err=TransmitMessageTextRich(pb,sigToo,false)) return(err);
		
		// and the final boundary
		PCat(boundary,"\p--");
		if (err=SendBoundary(pb->stream)) return(err);
	}
	
	return(noErr);
}

/************************************************************************
 * TransmitMessageTextStrip - strip styles before sending
 ************************************************************************/
OSErr TransmitMessageTextStrip(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel)
{
	Boolean oldFlat = Flatten!=nil;
	OSErr err;

	if ((pb->opts&OPT_BLOAT) || MessOptIsSet(pb->messH,FLAG_WRAP_OUT)) pb->flags |= FLAG_WRAP_OUT; // force wrapping on plain part of m/a
	if ((pb->opts&OPT_BLOAT) && !Flatten) Flatten = GetFlatten();	// force flattening
	ConvertExcerpt((*pb->messH)->bodyPTE,pb->hs.value,0x7fffffff,nil,nil);	// and convert the excerpts
	pb->hs.stop = PETEGetTextLen(PETE,(*pb->messH)->bodyPTE);
	PeteCleanList((*pb->messH)->bodyPTE);
	(*pb->messH)->win->isDirty = false;
	
	// send it
	if (err=TransmitMessageTextPlain(pb,sigToo,topLevel)) return(err);
	
	// put flatten back
	if (!oldFlat) ZapPtr(Flatten);
	return(err);
}

/************************************************************************
 * TransmitMessageTextPlain - send plaintext version of message & sig
 ************************************************************************/
OSErr TransmitMessageTextPlain(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel)
{
	OSErr err;
	// save off some stuff
	Boolean oldStrip = pb->strip;	// save the old strip value
	long oldOpts = pb->opts;
	long oldFlags = pb->flags;
	
	// we be strippin'
	pb->strip = true;
	pb->opts |= OPT_STRIP;
	
	// mime-version first of all
	if (topLevel) if (err=TransmitMimeVersion(pb)) return(err);
		
	// send the non-mime headers
	if (topLevel) if (err=TransmitTopHeaders(pb)) return(err);

	// send the mime headers
	if (pb->mime && !pb->receipt) if (err=TransmitMessageBodyHeaders(pb,sigToo,topLevel)) return(err);
	
	if (pb->mime)
	{
		// send the header/body separator
		if (!pb->receipt)
			if (err=SendPString(pb->stream,NewLine)) return(err);
			
		// send the message itself
		err = TransmitMessageBody(pb,false);
		if (err) return(err);
			
		// signature?  Do not generate headers.
		if (sigToo && pb->hasSig)
			if (err=TransmitMessageSigBody(pb,false)) return(err);
	}
	
	// put stuff back
	pb->strip = oldStrip;
	pb->opts = oldOpts;
	pb->flags = oldFlags;
		
	return(noErr);
}

/************************************************************************
 * TransmitMessageTextRich - send rich version of message & sig
 ************************************************************************/
OSErr TransmitMessageTextRich(TransmitPBPtr pb,Boolean sigToo,Boolean topLevel)
{
	OSErr err;

	// we don't wrap rich text
	pb->flags &= ~FLAG_WRAP_OUT;
	
	// mime-version first of all
	if (topLevel) if (err=TransmitMimeVersion(pb)) return(err);
		
	// send the non-mime headers
	if (topLevel) if (err=TransmitTopHeaders(pb)) return(err);

	// send the mime headers
	if (pb->mime) if (err=TransmitMessageBodyHeaders(pb,sigToo,topLevel)) return(err);
	
	if (pb->mime)
	{
		// send the header/body separator
		if (err=SendPString(pb->stream,NewLine)) return(err);
			
		// send the message itself; close out html if no signature
		err = TransmitMessageBody(pb,!pb->hasSig);
		if (err) return(err);
			
		// signature?  Do not generate headers & preamble
		if (sigToo && pb->hasSig)
			if (err=TransmitMessageSigBody(pb,false)) return(err);
	}		
	return(noErr);
}

/************************************************************************
 * TransmitMimeVersion - transmit the silly mime-version header
 ************************************************************************/
OSErr TransmitMimeVersion(TransmitPBPtr pb)
{
	OSErr err;
	
	if (err = ComposeRTrans(pb->stream,MIME_V_FMT,InterestHeadStrn+hMimeVersion,MIME_VERSION,NewLine))
		return(err);
	
	return(noErr);
}

/************************************************************************
 * TransmitTopHeaders - transmit the non-MIME headers at the top of the message
 ************************************************************************/
OSErr TransmitTopHeaders(TransmitPBPtr pb)
{
	OSErr sErr=noErr;
	Str255 buffer;
	Str255 scratch;
	short header;
	short tid = EffectiveTID(SumOf(pb->messH)->tableId);
	short priority;
	Accumulator newsGroupAcc;

	if (!pb->others) return(noErr);
	
	/*
	 * The dreaded X-Sender:
	 */
	if (sErr=SendXSender(pb->stream,pb->messH)) return(sErr);

	/*
	 * extra headers saved with message
	 */
	BufferSendRelease(pb->stream);
	if ((*pb->messH)->extras.data) SendExtras(pb->stream,(*pb->messH)->extras.data,(pb->flags&FLAG_CAN_ENC)!=0,tid);
	BSCLOSE(pb->stream,nil);
	
	/*
	 * newsgroups
	 */
	newsGroupAcc = (*pb->messH)->newsGroupAcc;
	if (newsGroupAcc.offset)
	{
		BufferSendRelease(pb->stream);
		SendNewsGroups(pb->stream,&newsGroupAcc,tid);
		BSCLOSE(pb->stream,nil);
	}
	
	/*
	 * Return Receipts
	 */
	if (MessFlagIsSet(pb->messH,FLAG_RR) && !MessOptIsSet(pb->messH,OPT_RECEIPT))
	{
		UseFeature (featureReturnReceiptTo);
		if (PrefIsSet(PREF_RRT)) {
			if (sErr=ComposeRTrans(pb->stream,RRT_FMT,GetReturnAddr(scratch,true),NewLine)) return(sErr);
		}
		if (sErr=ComposeRTrans(pb->stream,MIME_P_FMT,InterestHeadStrn+hMDN,GetReturnAddr(scratch,true),NewLine)) return(sErr);
	}

	/*
	 * Bulk?
	 */
	if (MessOptIsSet(pb->messH,OPT_BULK))
		if (sErr=ComposeRTrans(pb->stream,IMPORTANCE_FMT,TOCHeaderStrn+tchPrecedence,BULK,NewLine))
			return(sErr);
	
	/*
	 * extra static headers
	 */
	if (GetResource_('STR#',EX_HEADERS_STRN))
	{
		for (header=1;;header++)
		{
			if (*GetRString(buffer,EX_HEADERS_STRN+header))
			{
				if (sErr=SendTrans(pb->stream,buffer+1,*buffer,NewLine+1,*NewLine,nil))
					return(sErr);
			}
			else break;
		}
	}
	
	/*
	 * Registration headers
	 */
	if (MessOptIsSet(pb->messH,OPT_SEND_REGINFO))
	{
		UserStateType state = GetNagState ();

		BufferSendRelease(pb->stream);

		GetRegFirst(state, scratch);
		GetRString(buffer, RegCodeHeadStrn+hRegFirst);
		buffer[++buffer[0]] = ':';
		SendPtrHead(pb->stream, buffer+1, *buffer, scratch+1, *scratch, (pb->flags&FLAG_CAN_ENC)!=0, tid);

		GetRegLast(state, scratch);
		GetRString(buffer, RegCodeHeadStrn+hRegLast);
		buffer[++buffer[0]] = ':';
		SendPtrHead(pb->stream, buffer+1, *buffer, scratch+1, *scratch, (pb->flags&FLAG_CAN_ENC)!=0, tid);

		GetRegCode(state, scratch);
		GetRString(buffer, RegCodeHeadStrn+hRegCode);
		buffer[++buffer[0]] = ':';
		SendPtrHead(pb->stream, buffer+1, *buffer, scratch+1, *scratch, (pb->flags&FLAG_CAN_ENC)!=0, tid);

		BSCLOSE(pb->stream,nil);
	}
	
	/*
	 * real headers
	 */
	// priority
	priority = SumOf(pb->messH)->priority;
	priority = Prior2Display(priority);
	if (priority!=3)
	{
		if (!PrefIsSet(PREF_SUP_PRIORITY))
		{
			PriorityHeader(buffer,priority);
			if (sErr=SendTrans(pb->stream,buffer+1,*buffer,NewLine+1,*NewLine,nil))
			  return(sErr);
		}
		
		if (PrefIsSet(PREF_GEN_IMPORTANCE))
		{
			ComposeRString(buffer,IMPORTANCE_FMT,TOCHeaderStrn+tchImportance,ImportanceOutStrn+priority,NewLine);
			if (SendPString(pb->stream,buffer)) return(600);
		}
	}
	
	// date
	BuildDateHeader(buffer,SumOf(pb->messH)->seconds);
	if (*buffer && SendTrans(pb->stream,buffer+1,*buffer,NewLine+1,*NewLine,nil))
		return(600);

	BufferSendRelease(pb->stream);

	//finally what the user thinks of as the headers...
	for (header=1;header<BODY_HEAD;header++)
		if (header!=ATTACH_HEAD && (
#ifdef TWO
				PrefIsSet(PREF_POP_SEND) ||
#endif
				header!=BCC_HEAD))
			if (sErr=SendHeaderLine(pb->stream,pb->messH,header,(pb->flags&FLAG_CAN_ENC)!=0,tid))
				return(sErr);
	BSCLOSE(pb->stream,nil);
	
done:
	return(sErr/100 != 2 ? sErr : 0);
}

/************************************************************************
 * TransmitMultiHeaders - transmit the headers for multipart/something
 ************************************************************************/
OSErr TransmitMultiHeaders(TransmitPBPtr pb,short subType,PStr boundary,short otherParam,PStr otherVal)
{
	OSErr err;
	Str255 scratch;
	MessHandle messH = pb->messH;
	UHandle headerContent = nil;

	// The multipart header
	if (err = ComposeRTrans(pb->stream,MIME_MP_FMT,InterestHeadStrn+hContentType,
													MIME_MULTIPART,subType,
													AttributeStrn+aBoundary,boundary,NewLine))
		return(err);
	if (otherParam && (err = ComposeRTrans(pb->stream,MIME_CT_ANNOTATE,otherParam,otherVal,NewLine)))
		return(err);
		
	if (!GetRHeaderAnywhere(messH,PLUGIN_INFO,&headerContent))
	{
		MakePStr(scratch,*headerContent+2,GetHandleSize(headerContent)-2);	// 2 adjusts for colon-space
		ZapHandle(headerContent);
		if (err = ComposeRTrans(pb->stream,MIME_CT_ANNOTATE,PLUGIN_INFO,scratch,NewLine))
			return err;
	}
	
	// Let the translators know
	if (pb->tlMIME)
	{
		AddTLMIME(*pb->tlMIME,TLMIME_TYPE,GetRString(scratch,MIME_MULTIPART),nil);
		AddTLMIME(*pb->tlMIME,TLMIME_SUBTYPE,GetRString(scratch,subType),nil);
		AddTLMIME(*pb->tlMIME,TLMIME_PARAM,GetRString(scratch,AttributeStrn+aBoundary),boundary);
	}
	
	return(noErr);
}

/************************************************************************
 * TransmitMessageBodyHeaders - send the header for the body
 ************************************************************************/
OSErr TransmitMessageBodyHeaders(TransmitPBPtr pb,Boolean withSignature,Boolean topLevel)
{
	MessHandle messH = pb->messH;	// keep macros happy
	UHandle text;
	UHandle sig;
	OSErr err;
	
	if (pb->strip)
	{
		PETEGetRawText(PETE,TheBody,&text);
		sig = withSignature ? eSignature : nil;
		err = SendContentType(pb->stream,text,BodyOffset(text),sig,0,SumOf(messH)->tableId,&pb->flags,&pb->opts,nil,topLevel?pb->tlMIME:nil,nil);
	}
	else
	{
		sig = pb->html ? HTMLSignature : RichSignature;
		sig = withSignature ? eSignature : nil;
		err = SendContentType(pb->stream,pb->enriched.data,0,sig,0,SumOf(messH)->tableId,&pb->flags,&pb->opts,nil,topLevel?pb->tlMIME:nil,nil);
	}
	pb->encoder = (pb->receipt || 0==(pb->flags&FLAG_ENCBOD)) ? nil : QPEncoder;
	return(noErr);
}

/************************************************************************
 * TransmitMessageBody - send the message's body
 ************************************************************************/
OSErr TransmitMessageBody(TransmitPBPtr pb,Boolean withClosure)
{
	OSErr sErr = noErr;
	UHandle body;
	
	if (pb->strip)
	{
		// send the plain text
		PETEGetRawText(PETE,(*pb->messH)->bodyPTE,&body);
		sErr = SendBodyLines(pb->stream,body,pb->hs.stop,pb->hs.value,pb->flags,True,nil,0,False,pb->encoder);
	}
	else
	{
		if (pb->html)
		{
			// gotta close out the html?
			if (withClosure) if (sErr=HTMLPostamble(&pb->enriched,False)) return(sErr);
			
			// send the html
			sErr = SendBodyLines(pb->stream,pb->enriched.data,pb->enriched.offset,0,pb->flags,True,nil,0,False,pb->encoder);
		}
		else if (pb->rich)
		{
			// send the enriched
			sErr = SendEnriched(pb->stream,pb->enriched.data,pb->encoder);
		}
		
		// done with that...
		pb->enriched.offset = 0;
		AccuTrim(&pb->enriched);
	}

	BSCLOSE(pb->stream,pb->encoder);
	if (withClosure) pb->encoder = nil;

done:
	return(sErr);
}

/************************************************************************
 * TransmitMessageSig - transmit the sig as its own part, possibly bloated
 ************************************************************************/
OSErr TransmitMessageSig(TransmitPBPtr pb)
{
	OSErr err;
	
	if (pb->bloat && SigStyled && !pb->strip && (pb->rich||pb->html))
		err = TransmitMessageSigBloat(pb);
	else if (SigStyled && !pb->strip)
		err = TransmitMessageSigBody(pb,true);
	else
		err = TransmitMessageSigBodyPlain(pb);
	return(err);
}

/************************************************************************
 * TransmitMessageSigBodyPlain - send the sig, forcing to plain
 ************************************************************************/
OSErr TransmitMessageSigBodyPlain(TransmitPBPtr pb)
{
	OSErr err;
	// save off some stuff
	Boolean oldStrip = pb->strip;	// save the old strip value
	long oldOpts = pb->opts;
	long oldFlags = pb->flags;
	Boolean oldSigStyled = SigStyled;
	
	// we be strippin'
	pb->strip = true;
	pb->opts |= OPT_STRIP;
	SigStyled = false;
	
	// Send the darn thing
	err = TransmitMessageSigBody(pb,true);
	
	// put stuff back
	pb->strip = oldStrip;
	pb->opts = oldOpts;
	pb->flags = oldFlags;
	SigStyled = oldSigStyled;
	return(err);
}


/************************************************************************
 * TransmitMessageSigBloat - Send the message's signature, possibly with m/a
 ************************************************************************/
OSErr TransmitMessageSigBloat(TransmitPBPtr pb)
{
	OSErr err;
	Str127 boundary;
	Boolean oldFlat = Flatten!=nil;
	
	// we'll need a boundary
	BuildBoundary(pb->messH,boundary,"\pma");
	
	// send the part headers, header/body separator, and initial boundary
	if (err=TransmitMultiHeaders(pb,MIME_ALTERNATIVE,boundary,0,nil)) return(err);

	// now the header/body separator and the initial boundary
	if (err=SendPString(pb->stream,NewLine)) return(err);
	if (err=SendBoundary(pb->stream)) return(err);
	
	// send the plain version
	pb->flags  |= FLAG_WRAP_OUT; // force wrapping on plain part
	if (!Flatten) Flatten = GetFlatten();	// force flattening
	if (err=TransmitMessageSigBodyPlain(pb)) return(err);
	if (!oldFlat) ZapPtr(Flatten);
	
	// now the mid boundary
	if (err=SendBoundary(pb->stream)) return(err);
	
	// send the rich version
	if (err=TransmitMessageSigBody(pb,true)) return(err);
	
	// and the final boundary
	PCat(boundary,"\p--");
	if (err=SendBoundary(pb->stream)) return(err);
	
	return(noErr);
}

/************************************************************************
 * TransmitMessageSigBody - Send the message's signature
 ************************************************************************/
OSErr TransmitMessageSigBody(TransmitPBPtr pb,Boolean withHeaders)
{
	Handle sigTemp=nil;
	Str63 scratch;
	
	// which sig do we want?
	if (!SigStyled&&withHeaders || pb->strip || !(pb->rich||pb->html))
		sigTemp = eSignature;
	else if (pb->html)
		sigTemp = HTMLSignature;
	else if (pb->rich)
		sigTemp = RichSignature;

	// copy the sig
	if (sErr = MyHandToHand(&sigTemp)) {sigTemp = nil; goto done;}
	
	// send headers if we must
	if (withHeaders)
	{
		if (sErr = SendContentType(pb->stream,sigTemp,0,nil,0,SumOf(pb->messH)->tableId,&pb->flags,&pb->opts,nil,nil,nil)) goto done;
			pb->encoder = (pb->receipt || 0==(pb->flags&FLAG_ENCBOD)) ? nil : QPEncoder;
	
		// header/body separator
		if (withHeaders) if (sErr=SendPString(pb->stream,NewLine)) goto done;
		
		// preamble if html
		if (!pb->strip && pb->html)
		{
			// generate the preamble
			AccuZap(pb->enriched);
			if (sErr = AccuInit(&pb->enriched)) goto done;
			sErr = HTMLPreamble(&pb->enriched,GetRString(scratch,SIGNATURE),0,False);
			
			// add the signature to it
			if (!sErr) sErr = AccuAddHandle(&pb->enriched,sigTemp);
			if (sErr) goto done;
			AccuTrim(&pb->enriched);
			
			// now put the accumulator handle in sigTemp and zero the accumulator
			ZapHandle(sigTemp);
			sigTemp = pb->enriched.data;
			Zero(pb->enriched);
		}
	}
	
	// and finally, send it
	if (sErr=SendBodyLines(pb->stream,sigTemp,GetHandleSize_(sigTemp),0,pb->flags,True,nil,0,False,pb->encoder))
		goto done;
	BSCLOSE(pb->stream,pb->encoder);
	pb->encoder = nil;
	
done:
	ZapHandle(sigTemp);
	return(sErr);
}

/**********************************************************************
 * SendEnriched - send a block of text as text/enriched
 **********************************************************************/
OSErr SendEnriched(TransStream stream,UHandle text,DecoderFunc *encoder)
{
	UPtr start, space, spot, end;
	long soft = GetRLong(ENRICHED_SOFT_LINE);
	Boolean wasNl=True;
	long lastLen, lastC;
	short nofill=0;
	Str31 dir;
	
	LDRef(text);
	end = *text + GetHandleSize(text);
	
	for (start=*text;start<end;start=spot+1)
	{
		for (space=spot=start;spot<end;spot++)
		{
			if (*spot=='\015') break;
			if (*spot==' ') space = spot;
			if (!nofill && space>start && spot-start>soft) break;
		}
		
		if (!nofill && spot-start>soft && space>start) spot = space;
		if (!UUPCOut && *start=='.') BS(stream,encoder,".",1);
		BS(stream,encoder,start,spot-start);
		
		if (*start=='<' && spot[-1]=='>')
		{
			if (start[1]=='/') MakePStr(dir,start+2,spot-start-3);
			else MakePStr(dir,start+1,spot-start-2);
			
			if (EqualStrRes(dir,EnrichedStrn+enNoFill))
			{
				if (start[1]=='/') nofill = MAX(nofill-1,0);
				else nofill++;
			}
		}
		BS(stream,encoder,NewLine+1,*NewLine);
		if (space>start && spot==space && spot<end-1 && spot[1]=='\015')
			spot++;	// we already sent a newline, don't want to send another.
	}

done:
	UL(text);
	return(sErr);
}

/**********************************************************************
 * SMTPFinish - close out the SMTP session
 **********************************************************************/
OSErr FinishSMTP(TransStream stream,MessHandle messH)
{
	Str255 buffer;
		
	if (!UUPCOut)
	{
		ComposeString(buffer,"\p.%p",NewLine);
		if (sErr=SendTrans(stream,buffer+1,*buffer,nil)) return(600);
	}
	
	if (!UUPCOut)
	{
		sErr = GetReply(stream,buffer,sizeof(buffer),False,False);
		if (sErr > 399) SMTPCmdError(data,"",buffer);
		if (IsAddrErr(sErr))
		{
			if (*buffer) buffer[strlen(buffer)-1] = 0;
			AddOutgoingMesgError((*messH)->sumNum,(*(*messH)->tocH)->sums[(*messH)->sumNum].uidHash, sErr, BAD_XMIT_ERR_TEXT,"",buffer);
		}
	}
	
	return(sErr/100 != 2 ? sErr : (sErr=0));
}

/************************************************************************
 * SendHeaderLine - send a line of header information to sendmail
 ************************************************************************/
int SendHeaderLine(TransStream stream,MessHandle messH,short header,Boolean allowQP,short tid)
{
	Str63 note;
	Str63 label;
	HeadSpec hs;
	
	if (!CompHeadFind(messH,header,&hs))
	{
		if (header!=TO_HEAD) return(noErr);
		else
			// something wrong.  We didn't find the header we should have.  This message is toast
			return fnfErr;
	}
	
	if (hs.stop==hs.value)
	{
		if (header==TO_HEAD && *GetRString(note,BCC_ONLY))
		{
			GetRString(label,HEADER_STRN+TO_HEAD);
			return(sErr=SendPtrHead(stream,label+1,*label,note+1,*note,allowQP,tid));
		}
		else return(noErr);
	}

	/*
	 * is it an address header?
	 */
	else if (IsAddressHead(header))
		sErr = SendAddressHead(stream,TheBody,&hs,allowQP,tid);
	else
		sErr = SendNormalHead(stream,TheBody,&hs,allowQP,tid);

done:
	return(sErr); 
}

/************************************************************************
 * SendAddressHead - send an address header
 ************************************************************************/
OSErr SendAddressHead(TransStream stream,PETEHandle pte,HSPtr hs, Boolean allowQP,short tid)
{
	UPtr start;
	int lineLimit = GetRLong(WRAP_SPOT)-2;
	UHandle safe=nil;
	Handle fix = nil;
	Boolean high, wasHigh;
	Boolean first = True;
	short lastLen, lastC;
	UHandle addresses=nil;
	UHandle rawAddresses;
	short inGroup = 0;
	UHandle text;
	short charsOnLine = 0;
	OSErr err;
	Boolean popSend = PrefIsSet(PREF_POP_SEND);
	Boolean wasGroup = False;
	Str31 dontHide;
	
	PETEGetRawText(PETE,pte,&text);
	GetRString(dontHide,GROUP_DONT_HIDE);
	
	/*
	 * start by sending the label
	 */
	BS(stream,nil,LDRef(text)+hs->start,hs->value-hs->start-1);
	charsOnLine = hs->value-hs->start;
	
	SuckPtrAddresses(&rawAddresses,*text+hs->value,hs->stop-hs->value,True,True,False,nil);
	UL(text);
	if (rawAddresses && **rawAddresses)
	{
		err = ExpandAliases(&addresses,rawAddresses,0,True);
		ZapHandle(rawAddresses);
		if (err) return(err);
		
		/*
		 * now we have the fully-expanded address list
		 */
		if (addresses)
		{
			for (start=LDRef(addresses); *start; start += *start+2)
			{
				//Folder Carbon Copy - do no support FCC in Light
				if (HasFeature (featureFcc)) {
					if (IsFCCAddr(start)) continue;
					if (IsNewsgroupAddr(start)) continue;
				}
				if (inGroup && start[1]==';') inGroup--;
				if (popSend || !inGroup)
				{
					if (Flatten) TransLit(start+1,*start,Flatten);
					high = allowQP && AnyHighBits(start+1,*start);
	
					/*
					 * put out a return if we need to start a new line, or a comma if we just
					 * need a separator.  If it's the very first address, it goes on the
					 * first line, period.  If it's the trailing semicolon, it goes on this line,
					 * period.
					 */
					if (first || *start==1 && start[1]==';') first = False;
					else
					{
						if (!wasGroup && start[*start]!=';') {BS(stream,nil,",",1); charsOnLine++;}		/* send the comma separator */
						if ((charsOnLine && *start+charsOnLine>lineLimit) ||			/* overflow */
										 (high || wasHigh))																/* 1522 stuff gets own line always */
						{
							BS(stream,nil,NewLine+1,*NewLine);
							charsOnLine = 0;
						}
					}
					
					/*
					 * space
					 */
					if (start[*start]!=';') {BS(stream,nil," ",1); charsOnLine++;}
					
					/*
					 * hey!  we can finally send this!
					 */
					wasHigh = high;	/* make sure 1522 stuff gets own line next time */
					
					/*
					 * need to use RFC 1522
					 */
					if (high)
					{
						fix=Encode1342(start+1,*start,lineLimit,&charsOnLine,NewLine,tid);
						if (fix) BS(stream,nil,LDRef(fix),GetHandleSize_(fix));
					}
					
					/*
					 * RFC 1522 not used
					 */
					if (!fix)
					{
						BS(stream,nil,start+1,*start);
						charsOnLine += *start;
					}
					ZapHandle(fix);
				}
				if (wasGroup = start[*start]==':')
					if (!PFindSub(dontHide,start)) inGroup++;
			}
						
			/*
			 * finish off header with newline
			 */
			BS(stream,nil,NewLine+1,*NewLine);
		}
		ZapHandle(addresses);
	}
	else
		ZapHandle(rawAddresses);

done:
	ZapHandle(fix);
	return(sErr); 
}

/************************************************************************
 * SendNormalHead - send a normal header
 ************************************************************************/
OSErr SendNormalHead(TransStream stream,PETEHandle pte,HSPtr hs, Boolean allowQP,short tid)
{
	UHandle text;
	Str15 kiran;
	
	if (hs->index == SUBJ_HEAD && *GetRString(kiran,JUST_FOR_KIRAN))
		return(SendSubjectHead(stream,pte,hs,allowQP,tid));
	
	PETEGetRawText(PETE,pte,&text);
	
	LDRef(text);

	sErr = SendPtrHead(stream,*text+hs->start,hs->value-hs->start-1,*text+hs->value,hs->stop-hs->value,allowQP,tid);
	
	UL(text);
	
	return(sErr);
}

/************************************************************************
 * SendSubjectHead - send the subject header
 ************************************************************************/
OSErr SendSubjectHead(TransStream stream,PETEHandle pte,HSPtr hs, Boolean allowQP,short tid)
{
	UHandle text;
	Str15 kiran;
	long offset;
	long len;
	long stop;
	long colon;
	long k;
	
	GetRString(kiran,JUST_FOR_KIRAN);
	
	PETEGetRawText(PETE,pte,&text);
	
	LDRef(text);
	len = hs->stop;
	offset = hs->start;
	
	do
	{
		// find delimitter
		k = SearchStrPtr(kiran,*text,offset,len,true,false,nil);
		// if not found, pretend at end
		stop = k < 0 ? len : k;
		
		if (k>=0) (*text)[k] = '\015';	// replace first delim with newline 'cuz sendptrhead expects it
		
		// find colon
		colon = SearchPtrPtr(":",1,*text,offset,stop,true,false,nil);
		if (colon<0)
			sErr = SendPtrHead(stream," ",1,*text+offset,stop-offset,allowQP,tid);
		else
			sErr = SendPtrHead(stream,*text+offset,colon-offset+1,*text+colon+2,stop-colon-2,allowQP,tid);
		// skip delimitter
		offset = stop + *kiran;
		
		if (k>=0) (*text)[k] = kiran[1];	// put char back.  

	}
	while (stop<len && !sErr);
	
	UL(text);
	
	return(sErr);
}

/************************************************************************
 * SendPtrHead - send a normal header with label and text
 ************************************************************************/
OSErr SendPtrHead(TransStream stream,Ptr label, long labelLen, Ptr body, long bodyLen, Boolean allowQP,short tid)
{
	UPtr start, stop, end, space, limit;
	int lineLimit = GetRLong(WRAP_SPOT)-2;
	UHandle safe=nil;
	Handle fix = nil;
	Boolean first = True;
	short lastLen, lastC;
	short charsOnLine;
	
	/*
	 * start by sending the label
	 */
	BS(stream,nil,label,labelLen);
	charsOnLine = labelLen;

	/*
	 * send it, a line at a time
	 * prepend a ' ' to each line, ala RFC 822
	 */
	start = body;
	stop = start + bodyLen;
	
	// trim trailing spaces
	if (stop>start && IsWhite(stop[-1]))
	{
		while (stop>start && IsWhite(stop[-1])) --stop;
		bodyLen = stop-start;
		start[bodyLen] = '\015';	// pretend we have newline there
	}
	
	if (Flatten) TransLit(start,bodyLen,Flatten);	/* yes, tromps on in-memory copy;
																							 doesn't matter, will get pitched at close anyway */
	if (allowQP && AnyHighBits(start,stop-start))
	{
		if (fix=Encode1342(start,stop-start,lineLimit,nil,NewLine,tid))
		{
			start = LDRef(fix);
			BS(stream,nil," ",1); BS(stream,nil,start,GetHandleSize_(fix));
			start = stop;
		}
	}
	for (;start<stop;start=end+1)
	{
		Boolean shortLine;
	restart:
		limit = start + lineLimit - charsOnLine;
		if ((shortLine=stop<limit)) limit = stop;
		for (space=end=start;end<limit && *end!='\015'; end++)
			if (IsSpace(*end)) space=end;
		if (!shortLine && space==start && charsOnLine && *end!='\015')
		{
			BS(stream,nil,NewLine+1,*NewLine);
			charsOnLine = 0;	/* charsOnLine used only for header label */
			goto restart;
		}
		charsOnLine = 0;	/* charsOnLine used only for header label */
		if (space>start && end >= limit && limit<stop) end = space;
		ByteProgress(nil,-1,0);
		BS(stream,nil," ",1); BS(stream,nil,start,end-start);
		if (end<stop)
		{
			BS(stream,nil,NewLine+1,*NewLine);
			if (!IsSpace(*end)) end--;
		}
	}
	BS(stream,nil,NewLine+1,*NewLine);
done:
	ZapHandle(fix);
	return(sErr); 
}

/************************************************************************
 * SendExtras - send extra headers
 ************************************************************************/
OSErr SendExtras(TransStream stream, Handle extras,Boolean allowQP,short tid)
{
	UPtr start, stop, limit;
	UPtr labelStart, labelStop;
	Str31 label;
	Str31 uglyStupidHackForWindowsIMAP;
	
	GetRString(uglyStupidHackForWindowsIMAP,PLUGIN_INFO);
	
	start = LDRef(extras);
	limit = start + GetHandleSize_(extras);
	
	for (;start<limit;start=stop+1)
	{
		labelStart = start;
		if(*start==' ' || *start==9)
		{
			labelStop = labelStart;
		}
		else
		{
			/*
			 * find the colon
			 */
		  for (stop=start;stop<limit && *stop!=':';stop++);
		  if (stop==limit) break;
		  labelStop = stop+1;
		  if (labelStop-labelStart>2) MakePStr(label,labelStart,labelStop-labelStart-1);
		  else *label = 0;
			
			/*
			 * find the newline
			 */
			start=stop+1;
			if (*start==' ') start++;
		}
		for (stop=start;stop<limit && *stop!='\015';stop++);
		
		/*
		 * send the header
		 */
		if (!StringSame(label,uglyStupidHackForWindowsIMAP))
		if (sErr = SendPtrHead(stream,labelStart,labelStop-labelStart,start,stop-start,allowQP,tid)) break;
	}
	UL(extras);
	return(sErr);		
}

/************************************************************************
 * SendNewsGroups - send the NewsGroups header
 ************************************************************************/
OSErr SendNewsGroups(TransStream stream,AccuPtr newsGroupAcc,short tid)
{
	Str63 headerName;
	
	AccuAddChar(newsGroupAcc,'\015');
	AccuTrim(newsGroupAcc);
	GetRString(headerName,NEWSGROUPS);
	return(sErr=SendPtrHead(stream,headerName+1,*headerName,LDRef(newsGroupAcc->data),newsGroupAcc->offset-1,false,tid));
}

/************************************************************************
 * SendXSender - construct and send the X-Sender header
 ************************************************************************/
short SendXSender(TransStream stream,MessHandle messH)
{
	Handle popCanon=nil,returnCanon=nil;
	Str255 buffer;
	Str255 from;
	short err = 0;
	
	if ((*CurPers)->popSecure)
	{
		CompHeadGetStr(messH,FROM_HEAD,from);
		/* figure out what the return addr means */
		SuckPtrAddresses(&returnCanon,from+1,*from,False,True,False,nil);
		
		/* grab the POP account, and figure out what it means */		
		GetPOPPref(buffer);
		SuckPtrAddresses(&popCanon,buffer+1,*buffer,False,True,False,nil);
	}
	
	/* if different or no password, send Sender field */
	if (!UUPCOut && (!(*CurPers)->popSecure || !popCanon || !returnCanon || !StringSame(LDRef(popCanon),LDRef(returnCanon))))
	{
		if (!(*CurPers)->popSecure)	GetRString(buffer,UNVERIFIED);
		else *buffer = 0;
		
		err = ComposeRTrans(stream,XSENDER_FMT,UUPCIn ? 0 : GetPOPPref(from),buffer,NewLine);
	}

	ZapHandle(popCanon);
	ZapHandle(returnCanon);
	return(err);
}

/************************************************************************
 * SendBodyLines - send the actual body of the message
 *	Don't look at this; it's a mess.
 *	text				Handle to the text to send
 *	length			length of same
 *  offset			offset at which to begin
 *	flags				message flags
 *	forceLines	should I force a newline at the end of the text?
 *	lineStarts	pointer to array for determining wrap (may be nil)
 *	nLines			length of same
 *	partial			should I listen to the partial information?
 ************************************************************************/
int SendBodyLines(TransStream stream,UHandle  text,long length,long offset,long flags,Boolean forceLines,short *lineStarts,short nLines,Boolean partial,DecoderFunc *encoder)
{
	UPtr start;			/* the beginning of the text left to be sent */
	UPtr stop;			/* the end of the entire text block */
	UPtr end;				/* one past the last character of a line of text to be sent
										 for complete lines, this will be a return */
	UPtr space;			/* the last space before end */
	int lineLimit;	/* # of chars at which to wrap */
	int hardLimit;	/* limit for hard returns; don't wrap if para < hardLimit */
	static short quoteLevel;		/* the # of quote chars at start of line */
	Byte suspendChar;						/* the quote character */
	static short partialSize;		/* the size of the last line output, if it
																 was an incomplete line */
	static Boolean softNewline;	/* was the last newline added by us? */
	Str31 scratch;
	short i;
	UPtr nl;
	Boolean doWrap = 0!=(flags&FLAG_WRAP_OUT);
	short lastC, lastLen;
	short maxQuote = GetRLong(MAX_QUOTE);
	short curLen;
	short tab = GetRLong(TAB_DISTANCE);
	Boolean flowed = doWrap && UseFlowOut;
	Boolean withSpace;
	int smtpMaxLen = GetRLong(MAX_SMTP_LINE);
	Str15 spaceNewLine;
	
	if (flowed)
	{
		*spaceNewLine = 0;
		PCatC(spaceNewLine,' ');
		PCat(spaceNewLine,NewLine);
	}
		
	if (!partial)
	{
		softNewline=False;	/* the caller has told us not to */
		partialSize = 0;		/* bother with partial processing */
	}
	start = LDRef(text)+offset;
	stop = *text + length;

	/*
	 * gather up important info for wrap calculations
	 */
	if (doWrap)
	{
		suspendChar = (GetRString(scratch,UseFlowOut ? FLOWED_QUOTE:QUOTE_PREFIX))[1];
		withSpace = UseFlowOut ? scratch[0]>1 : false;
		lineLimit = GetRLong(UseFlowOut ? (encoder?ENCODED_FLOWED_WRAP_SPOT:FLOW_WRAP_SPOT):WRAP_SPOT);
		hardLimit = GetRLong(UseFlowOut ? FLOW_WRAP_THRESH:WRAP_THRESH);
		
		/*
		 * if this is a new line, count the quote level
		 */
		if (!partialSize)
		{
			for (end = start;end<stop && *end==suspendChar && end-start<10;end++);
			quoteLevel = end-start;
			if (quoteLevel > maxQuote) quoteLevel = 0;
		}
	}
	else lineLimit = REAL_BIG;
	
	/*
	 * main loop; loop through the buffer, sending one line at a time
	 */
	for (; start<stop; start = end+1)
	{
		/* calculate the spot before which we should wrap */
		if (doWrap)
			curLen = partialSize; //limit = start + lineLimit - partialSize;
		
		/* if we don't want wrapping, or there is less text than the wrap limit */
		//if (!doWrap) limit = stop;	/* no need to wrap */
		
		/*
		 * look through the buffer, from start to the calculated line limit
		 * keep track of the last space we see, since it's a potential wrap point
		 * if we find a return, we have a whole line, and can send it
		 */
		if (doWrap)
		{
			if (softNewline) while (*start==' ' && start<stop) start++;	/* skip leading spaces after a soft newline */
			for (space=nl=start;nl<stop && *nl!='\015'; nl++)
			{
				if ((curLen<lineLimit||space==start&&curLen<smtpMaxLen) && (*nl==' ' || *nl=='\t') && (nl==stop-1||nl[1]!='>')) space=nl;
				if (*nl=='\t') curLen = tab*((curLen+tab)/tab);
				else curLen++;
			}
			
			// adjust for lines ending in whitespace
			if (nl<stop)
			{
				// special case for "-- "
				if (nl-start==3 && start[0]==start[1] && start[0]=='-' && start[2]==' ')
					; // leave it alone
				else
					for (end=nl-1;end>start;end--)
					{
						if (*end==' ') curLen--;
						else if (*end=='\t') curLen = tab*((curLen-tab)/tab);
						else break;
					}
			}
			end = nl;
			
			if (curLen>lineLimit)  /* we went over the wrap limit */
			{
				if (space>start)			/* and we found a space */
					end = space;				/* Wrap it! */
				else if (curLen>smtpMaxLen)
					end = start+smtpMaxLen;
			}
		}
		else
		{
			for (nl=start;nl<stop && *nl!='\015'; nl++);	/* just look for newlines */
			end = nl;
		}
		
		/*
		 * make special allowance for lines >wrap limit but < 80
		 */
		if (!softNewline && end<stop && curLen<hardLimit) end = nl;
	 
		/* are we adding the newline?  We'll want to know for the next line. */
		if (end<stop) softNewline = *end!='\015';
		
		/*
		 * at this point, start points at the beginning of the line to send,
		 * and end points one character past the end of the line to send
		 */
		
		// Protect a few things that are liable to transport damage
		if (!encoder && !partialSize && end>start && ((void*)SendTrans)!=((void*)WrapSendTrans))
		{
			/* escape initial periods, if need be */ 
			if (!UUPCOut && *start=='.') BS(stream,encoder,".",1);
			/* if doing f=f, space-stuff "From " */
			else if (flowed && *start=='F' && end-start>5 && *(uLong*)(start+1)=='rom ') BS(stream,encoder," ",1);
		}
	
		/*
		 * find last non-space character on line,
		 * unless it's the end of the buffer, or not being wrapped,
		 * in which case we'd best not drop spaces
		 */
		space = end;
		// special case for "-- "
		if (space-start==3 && start[0]==start[1] && start[0]=='-' && start[2]==' ')
			; // leave it alone
		else if (doWrap && end<stop) while(space>start && (space[-1]=='\t'||space[-1]==' ')) space--;
		
		/* if there is data to send, send it */
		if (space>start)
		{
			// Ok, we might need to insert an extra space if doing f=f
			if (flowed && !partialSize && quoteLevel && space-start>quoteLevel && start[quoteLevel]==' ' && ((void*)SendTrans)!=((void*)WrapSendTrans))
			{
				BS(stream,encoder,start,quoteLevel);
				BS(stream,encoder," ",1);
				BS(stream,encoder,start+quoteLevel,space-start-quoteLevel);
			}
			else
			{
				/* if doing f=f, space-stuff initial space */
				if (!partialSize && flowed && *start==' ') BS(stream,encoder," ",1);
				/* Now send the line */
				BS(stream,encoder,start,space-start);
			}
		}
		
		/*
		 * send the newline, unless we've run out of characters and so don't know
		 * if this should be a complete line or not
		 */
		if (forceLines || end<stop)
		{
			if (softNewline && flowed) BS(stream,encoder,spaceNewLine+1,*spaceNewLine);	// indicate that the newline is soft
			else BS(stream,encoder,NewLine+1,*NewLine);
		}
		
		/*
		 * We just put out a line, so we know we're starting fresh for
		 * the next one, if there is a next one
		 */
		partialSize = 0;

		/*
		 * quoted line processing, if there are any chars left
		 */
		if (end<stop)
			/*
			 * if we sent out a complete line, peek at the next line to see how
			 * many quote characters it has
			 */
			if (*end=='\015')
			{
				UPtr p;
				for (p=end+1;p<stop && *p==suspendChar;p++);
				quoteLevel = p-end-1;
				if (quoteLevel > maxQuote) quoteLevel = 0;
			}
			else	/* if we wrapped it, prequote the next line */
			{
				for (i=0;i<quoteLevel;i++) BS(stream,encoder,&suspendChar,1);
				partialSize = quoteLevel;	/* guess we have a partial line after all */
				if (flowed && *end=='>') BS(stream,encoder," ",1);	// space-stuff initial >
				if (quoteLevel && withSpace) {BS(stream,encoder," ",1);partialSize++;}
			}
		
		/*
		 * normally, end points at a newline (for complete lines) or space
		 * (for wrapped ones).  So, we normally skip the character end points
		 * to.  However, long solid lines or Rong-wrapped lines might not obey
		 * this behavior; adjust end back by one to make up for the increment
		 * we'll do in just a few cycles...
		 */
		if (end<stop && *end!=' ' && *end!='\015') end--;
	}
	
	/*
	 * all done with that buffer.  If the last character is a newline,
	 * we don't have much to do.  Otherwise, we may (forceLines) wish to
	 * newline-terminate, else we want to remember how long the line
	 * fragment we sent was
	 */
	if (lastC!=NewLine[*NewLine])
	  if (forceLines) BS(stream,encoder,NewLine+1,*NewLine);
		else partialSize = lastLen;

done:
	UL(text);
	return(sErr); 
}

#pragma segment SMTP2

/************************************************************************
 * PrimeProgress - get the progress window started.
 ************************************************************************/
void PrimeProgress(MessHandle messH)
{
	Str255 buff;
	
	MyGetWTitle(GetMyWindowWindowPtr((*messH)->win),buff);
//	ByteProgress(buff,0,CountCompBytes(messH));
	Progress(NoChange,NoChange,nil,nil,buff);
}


/************************************************************************
 * WannaSend - find out of the user wants to send a dirty window
 ************************************************************************/
int WannaSend(MyWindowPtr win)
{
	Str255 title;
	
	MyGetWTitle(GetMyWindowWindowPtr(win),title);
	return(AlertStr(WANNA_SEND_ALRT,Stop,title));
}

/************************************************************************
 * SendAttachments - send the files the user has attached to his message.
 ************************************************************************/
int SendAttachments(TransStream stream,MessHandle messH,long flags,PStr boundary,short tableID,short idBase)
{
	FSSpec spec;
	short index;
	short err=noErr;
	Boolean plainText = 0==(flags & FLAG_BX_TEXT);
	Boolean canQP = 0!=(flags & FLAG_CAN_ENC);
	Boolean isUU;
	CInfoPBRec hfi;
	Str31 name;
	short aType;
	
	aType = AttachOptNumber(flags);
	isUU = aType+1==atmUU;
	hfi.hFileInfo.ioNamePtr = name;
	for (index=1;!err;index++)
	{
		if (err=GetIndAttachment(messH,index,&spec,nil))
			if (err==1) break;
			else
				return(FileSystemError(BINHEX_OPEN,spec.name,err));
		IsAlias(&spec,&spec);
		if (FSpIsItAFolder(&spec))
			err = SendAttachmentFolder(stream,messH,flags,canQP,plainText,tableID,boundary,&spec,0,idBase+index-1,&idBase,&hfi);
		else
			err = SendAnAttachment(stream,messH,flags,canQP,plainText,tableID,boundary,&spec,0,idBase+index-1);
	}
	if (index-1)
		UpdateNumStat(kStatSentAttach,index-1);	
	if (err==1) err = noErr;
	return(err);
}

/************************************************************************
 * SendRelatedParts - send the files the user has put in his html
 ************************************************************************/
int SendRelatedParts(TransStream stream,MessHandle messH,long flags,StackHandle stack,PStr boundary)
{
	FSSpec origSpec,spec;
	short index;
	short err=noErr;

	for (index=(*stack)->elCount;!err && index--;)
	{
		if (err = StackItem(&origSpec,index,stack)) err = 1;
		else
		{
			IsAlias(&origSpec,&spec);
			ConvertPictPart(&origSpec,&spec);
			if (err=SendAnAttachment(stream,messH,flags,true,true,0,boundary,&spec,1,(*stack)->elCount-index-1)) break;
		}
	}
	if (err==1) err = noErr;
	return(err);
}

/************************************************************************
 * ConvertPictPart - convert any PICT HTML parts to something more universal
 ************************************************************************/
void ConvertPictPart(FSSpecPtr origSpec,FSSpecPtr spec)
{
	static OSType exportType;
	Str255 scratch;
	Str31 token;
	GraphicsExportComponent	exCI = nil;
	OSErr	err;

	if (PrefIsSet(PREF_NO_PICT_CONVERSION) ||	//	User doesn't want conversion
		exportType == -1 ||	//	Exporter not found
#if TARGET_RT_MAC_CFM
		!HaveQuickTime(0x0400) || !GraphicsExportDoExport ||	//	Make sure we have version 4 or greater and QT library
#else
		!HaveQuickTime(0x0400) ||
#endif
		FileTypeOf(spec) != 'PICT')
			return;	//	Don't convert this one
		
		if (!exportType)
		{
			//	Find best exporter
			if (GetRString(scratch,EXPORT_PICT_LIST))
			{
				OSType	thisType;
				UPtr		spot;
				
				for(spot=scratch+1;PToken(scratch,token,&spot,",");)
				{
					if (*token==sizeof(thisType))
					{
						BMD(token+1,&thisType,sizeof(thisType));
						if (exCI = OpenDefaultComponent(GraphicsExporterComponentType,thisType))
						{
							//	Found one!
							exportType = thisType;
							break;
						}
					}
				}
			}

			if (!exportType)
			{
				//	Exporter not found
				exportType = -1;
				return;
			}
		}
		else
			exCI = OpenDefaultComponent(GraphicsExporterComponentType,exportType);
		
		if (exCI)
		{
			//	Do conversion
			GraphicsImportComponent	imCI;
			
			if (!GetGraphicsImporterForFile(spec,&imCI))
			{
				FSSpec	tempSpec;
				
				tempSpec = *origSpec;
				UniqueSpec(&tempSpec,31);
				GraphicsExportSetOutputFile(exCI,&tempSpec); 
				GraphicsExportSetInputGraphicsImporter(exCI,imCI);
				if (exportType == 'PNGf')
				{
					//	Don't allow alpha channel. QuickTime PNG exporter messes up alpha channel with
					//	PICT vector images rendering them invisible when not viewing with QuickTime
					GraphicsExportSetDepth (exCI,24);
				}
				err = GraphicsExportDoExport(exCI,nil);
				CloseComponent(imCI);
				CloseComponent(exCI);
				if (!err)
				{
					//	Replace old PICT file (or alias)
					FSpDelete(origSpec);
					FSpRename(&tempSpec,origSpec->name);
					*spec = *origSpec;
				}
			}
			else
				CloseComponent(exCI);
		}
}

/************************************************************************
 * SendAttachmentFolder - send a folder full of files
 ************************************************************************/
int SendAttachmentFolder(TransStream stream,MessHandle messH,long flags,Boolean canQP,Boolean plainText,short tableID,PStr boundary,FSSpecPtr folderSpec,short multiID,short partID,short *partBase,CInfoPBRec *hfi)
{
	short err = noErr;
	short index;
	FSSpec spec;
	Str127 ourBoundary;
	long dirId;
	
	// start by sending a boundary for the outer multipart
	if (err=SendBoundary(stream)) return err;
	// and a content-id, why not?
	if (err=SendCID(stream,messH,multiID,partID)) return(err);	
	
	// now, let's compose our boundary
	NumToString(partID,spec.name);
	BuildBoundary(nil,ourBoundary,spec.name);
	/*
	 * send the multipart header
	 */
	if (err = ComposeRTrans(stream,MIME_MP_FMT,
									 InterestHeadStrn+hContentType,
									 MIME_MULTIPART,
									 MIME_X_FOLDER,
									 AttributeStrn+aBoundary,
									 ourBoundary,
									 NewLine))
		return(err);

	/*
	 * content-disposition
	 */
	if (!err) err = ComposeRTrans(stream,MIME_CD_FMT,
								 InterestHeadStrn+hContentDisposition,
								 ATTACHMENT,
								 AttributeStrn+aFilename,
								 folderSpec->name,
								 NewLine);
	
	// header/body separator
	if (err=SendPString(stream,NewLine)) return(err);
	
	// get our dirID
	spec.vRefNum = folderSpec->vRefNum;
	spec.parID = dirId = SpecDirId(folderSpec);
	if (!spec.parID) return fnfErr;

	// setup our iterator
	hfi->hFileInfo.ioNamePtr = spec.name;
	hfi->hFileInfo.ioFDirIndex = 0;
	index = 0;
	
	// loop through the contents
	while (!DirIterate(spec.vRefNum,spec.parID,hfi))
	{
		if (partBase) ++*partBase;
		partID++;
		if (HFIIsFolder(hfi))
		{
			index = hfi->hFileInfo.ioFDirIndex;	// save the index
			err = SendAttachmentFolder(stream,messH,flags,canQP,plainText,tableID,ourBoundary,&spec,multiID,partID,partBase,hfi);
			hfi->hFileInfo.ioNamePtr = spec.name;
			hfi->hFileInfo.ioFDirIndex = index;	// restore the index
			spec.parID = dirId;
		}
		else
			err = SendAnAttachment(stream,messH,flags,canQP,plainText,tableID,ourBoundary,&spec,multiID,partID);
		if (err) break;
	}
	
	// we need to send our final boundary
	if (!err) err = SendTrans(stream,"--",2,ourBoundary+1,*ourBoundary,"--",2,NewLine+1,*NewLine,nil);
	
	return err;
}

/************************************************************************
 * SendAnAttachment - send a single file
 ************************************************************************/
int SendAnAttachment(TransStream stream,MessHandle messH,long flags,Boolean canQP,Boolean plainText,short tableID,PStr boundary,FSSpecPtr spec,short multiID,short partID)
{
	short err=noErr;
	Boolean isUU;
	CInfoPBRec hfi;
	Str255 s;
	Str31 name;
	short aType;
	AttMap am;
	uLong oldTicks;
	Boolean flat = false;
	FSSpec local;
	Boolean noRFork;
	
	aType = AttachOptNumber(flags);
	isUU = aType+1==atmUU;
	hfi.hFileInfo.ioNamePtr = name;
	if (err=FSpGetHFileInfo(spec,&hfi)) return(FileSystemError(BINHEX_OPEN,spec->name,err));
	ComposeRString(s,BINHEX_PROG_FMT,spec->name);
	Progress(NoChange,NoChange,nil,nil,s);
	noRFork = hfi.hFileInfo.ioFlRLgLen==0;
	
	oldTicks = TickCount();
	if (err=SendBoundary(stream)) return(err);
	if (err=SendCID(stream,messH,multiID,partID)) return(err);
	if (err=FindAttMap(spec,&am)) return(err);
	
	if (am.mm.specialId=='flat')
	{
		local = *spec;
		if (!FlattenAndSpool(&local))
		{
			spec = &local;	// send the spooled copy
			flat = true;
		}
		am.mm.specialId = nil;	// special processing done
	}
	
#ifdef TWO
	if (plainText && am.mm.specialId && am.mm.specialId != '    ' || am.mm.specialId=='MiME')
		err = SendSpecial(stream,spec,&am);
	else
#endif
		err = kSpecialSendDidntPanOut;
	if (err != kSpecialSendDidntPanOut) ;
#ifdef TWO
	else if (plainText && IsMailbox(spec) && hfi.hFileInfo.ioFlLgLen)
		err = SendDigest(stream,spec);
#endif
	else if (plainText && (am.isPostScript&&!canQP || EqualStrRes(am.mm.mimetype,MIME_TEXT) && am.isBasic))
		err = SendPlain(stream,spec,flags,tableID,&am);
	else if (!isUU && plainText && (am.isBasic || noRFork))
		err = SendDataFork(stream,spec,flags,tableID,&am);
	else
	{
		switch(aType+1)
		{
			case atmDouble:	err = SendDouble(stream,spec,flags,tableID,&am); break;
			case atmSingle: err = SendSingle(stream,spec,True,&am); break;
#ifdef TWO
			case atmUU: err = SendUU(stream,spec,&am); break;
#endif
			default: err = SendBinHex(stream,spec,&am); break;
		}
	}
//		if (!err) {Progress(100,NoBar,nil,nil,nil);PopProgress(False);}
	{
		long rate = (600*(hfi.hFileInfo.ioFlLgLen+hfi.hFileInfo.ioFlRLgLen))/((TickCount()-oldTicks+1)*1024);
		ComposeLogS(LOG_TPUT,nil,"\p%p: %d %d.%d KBps",spec->name,aType,rate/10,rate%10);
	}

	if (flat) FSpDelete(spec);
	return(err);
}

/************************************************************************
 * SendCID - send a content-id
 ************************************************************************/
OSErr SendCID(TransStream stream,MessHandle messH,long part,short n)
{
	Str255 mid, cid;
	OSErr err=noErr;
	
	if (*CompGetMID(messH,mid))
	{
		// compose
		BuildContentID(cid,mid,part,n);
		// send
		err = ComposeRTrans(stream,CID_SEND_FMT,InterestHeadStrn+hContentId,cid,NewLine);
	}
	return(err);
}

/************************************************************************
 * BuildContentID - build a content-id, without <>'s or header or newline
 ************************************************************************/
PStr BuildContentID(PStr into,PStr mid,long part,short i)
{
	return(ComposeRString(into,CID_ONLY_FMT,mid,part,i));
}

#ifdef TWO

/**********************************************************************
 * SendDigest - send a mailbox, as a digest
 **********************************************************************/
OSErr SendDigest(TransStream stream, FSSpecPtr spec)
{
	TOCHandle tocH = TOCBySpec(spec);
	Str255 boundary;
	Str63 date;
	short i;
	MyWindowPtr win;
	Boolean newWin;
	
	if (!tocH) return(1);
	
	/*
	 * build the boundary
	 */
	BuildBoundary(nil,boundary,"\pd");
	
	sErr = ComposeRTrans(stream,MIME_MP_FMT,
									 InterestHeadStrn+hContentType,
									 MIME_MULTIPART,
									 MIME_DIGEST,
									 AttributeStrn+aBoundary,
									 boundary,
									 NewLine);
	if (!sErr) sErr = ComposeRTrans(stream,MIME_CD_FMT,
									InterestHeadStrn+hContentDisposition,
									ATTACHMENT,
									AttributeStrn+aFilename,spec->name,
									NewLine);
	if (!sErr && *R822Date(date,AFSpGetMod(spec)-ZoneSecs()))
			sErr = ComposeRTrans(stream,MIME_CT_ANNOTATE, AttributeStrn+aModDate,date,NewLine);
	if (!sErr) sErr=SendPString(stream,NewLine);

	if (!sErr) for (i=0;i<(*tocH)->count;i++)
	{
		UHandle tSig, tRSig, tHSig;
		
		tSig = eSignature;
		tRSig = RichSignature;
		tHSig = HTMLSignature;

		eSignature = nil;
		RichSignature = nil;
		HTMLSignature = nil;
		
		/*
		 * send a boundary
		 */
		if (sErr=SendBoundary(stream)) break;
		
		win = GetAMessageLo(tocH, i, nil, nil, false, &newWin);
		if(win)
		{
			WindowPtr	winWP = GetMyWindowWindowPtr(win);
			
			sErr = TransmitMessageForSpool(stream, Win2MessH(win));
			if(newWin)
				CloseMyWindow(winWP);
			else
				NotUsingWindow(winWP);
		}
		else sErr = mFulErr;
		
		BSCLOSE(stream,nil);	/* unfortunate, but gotta do it because of how SendBoundary works */	

		eSignature = tSig;
		RichSignature = tRSig;
		HTMLSignature = tHSig;
		
	}
	
done:
	FlushTOCs(True,False);
	
	/*
	 * and the terminal boundary
	 */
	if (!sErr)
	{
		PCat(boundary,"\p--");
		sErr = SendBoundary(stream);
	}
	return(sErr);
}

/************************************************************************
 * SendSpecial - send a special attachment type
 ************************************************************************/
OSErr SendSpecial(TransStream stream,FSSpecPtr spec,AttMapPtr amp)
{
	OSErr err;
	
	switch (amp->mm.specialId)
	{
		case 'AURL':
			err = SendAnonFTP(stream,spec);
			break;
		case 'MiME':
			err = SendRawMIME(stream,spec);
			break;
		default:
			WarnUser(INVALID_MAP,0);
			err = 1;
			break;
	}
	return(err);
}
#endif

/************************************************************************
 * GetFlatten - Copy the flatten table into a pointer
 ************************************************************************/
UPtr GetFlatten(void)
{
	Handle flatH;
	UPtr flatten;

	flatten = NuPtr(256);
	flatH = GetResource_('taBL',ktFlatten);
	if (flatH) BMD(*flatH,flatten,256);
	else ZapPtr(flatten);
	
	return(flatten);
}

#ifdef TWO
/************************************************************************
 * SendAnonFTP - send an 'AURL' doc as an anonymous ftp thingie
 ************************************************************************/
OSErr SendAnonFTP(TransStream stream,FSSpecPtr spec)
{
	OSErr err;
	Handle text;
	Str127 type, ftp, host, dir, name, token;
	Str255 data;
	UPtr spot;
	short size;
	
	if (err=Snarf(spec,&text,254))
		FileSystemError(BINHEX_READ,spec->name,err);
	else
	{
		size = GetHandleSize_(text);
		MakePStr(data,*text,size);
		
		/*
		 * parse the string
		 */
		spot = data+1;
		if (PToken(data,type,&spot," ") &&
				PToken(data,ftp,&spot,":"))
		{
			while (*spot=='/') spot++;
			if (PToken(data,host,&spot,"/"))
			{
				*name = *dir = 0;
				while (PToken(data,token,&spot,"/"))
				{
					if (dir[*dir] != '/') PCatC(dir,'/');
					PCat(dir,token);
					PCopy(name,token);
				}
				if (*name)
				{
					*dir -= 1+*name;
					
					/*
					 * hey; that all parsed
					 */
					if (EqualStrRes(type,ANARCHIE_GET) || EqualStrRes(type,ANARCHIE_TXT))
					if (EqualStrRes(ftp,ANARCHIE_FTP) && !PPtrFindSub("\p@",host+1,*host))
					{
						/*
						 * and it is even something we can handle.  Hurrah!
						 */
						err = ComposeRTrans(stream,MIME_TEXTPLAIN,
																InterestHeadStrn+hContentType,
																MIME_MESSAGE,EXTERNAL_BODY,"",NewLine);
						if (!err)
							err = ComposeRTrans(stream,NQ_ANNOTATE,
																	AttributeStrn+aAccessType,
																	GetRString(data,ANON_FTP),
																	NewLine);
						if (!err)
							err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
																	AttributeStrn+aSite,
																	host,
																	NewLine);
						if (*dir && !err)
							err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
																	AttributeStrn+aDirectory,
																	dir,
																	NewLine);
						if (!err)
							err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
																	AttributeStrn+aName,
																	name,
																	NewLine);
						if (!err)
							err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
																	AttributeStrn+aMode,
																	GetRString(data,EqualStrRes(type,ANARCHIE_TXT)?ASCII:IMAGE),
																	NewLine);
						
						if (!err) err = SendPString(stream,NewLine);
						if (!err)
							err = ComposeRTrans(stream,MIME_MP_FMT,
																	InterestHeadStrn+hContentType,
																	MIME_APPLICATION,MIME_OCTET_STREAM,
																	AttributeStrn+aName,
																	name,
																	NewLine);
						if (!err)
							err = ComposeRTrans(stream,MIME_CD_FMT,
																	InterestHeadStrn+hContentDisposition,
																	ATTACHMENT,
																	AttributeStrn+aFilename,name,
																	NewLine);
						return(err);
					}
				}
			}
		}
		return(-1);
	}
	return(err);
}
/************************************************************************
 * SendRawMIME - send a raw MIME document
 ************************************************************************/
OSErr SendRawMIME(TransStream stream,FSSpecPtr spec)
{
	long size = FSpDFSize(spec);
	Handle buffer = nil;
	long bSize;
	long count;
	long sendCount;
	OSErr err = noErr;
	short refN;
	DecoderFunc *encoder = UUPCOut ? nil : PeriodEncoder;
	Boolean needNL = false;	// we do not need to add a newline

	bSize = MIN(size,GetRLong(BUFFER_SIZE));
	//Debugger();
	
	if (buffer=NewIOBHandle(size/4,size))
	{
		MoveHHi(buffer);
		HLock(buffer);
		if (!(err = FSpOpenDF(spec,fsRdWrPerm,&refN)))
		{
			// sniff the end of the file for crlf
			GetEOF(refN,&count);
			if (count<2) needNL = true;
			else
			{
				SetFPos(refN,fsFromLEOF,-2);
				count = 2;
				ARead(refN,&count,*buffer);
				needNL = (*buffer)[0]!='\015' || (*buffer)[1]!='\012';
				SetFPos(refN,fsFromStart,0);
			}
			
			// send the file
			while (!err && size>0)
			{
				count = MIN(size,bSize);
				if (!(err=ARead(refN,&count,*buffer)))
				{
					if (NewLine[0]==1 && NewLine[1]=='\015')
						sendCount = RemoveChar('\012',*buffer,count);
					else if (NewLine[0]==1 && NewLine[1]=='\012')
						sendCount = RemoveChar('\015',*buffer,count);
					else
						sendCount = count;
					err = BufferSend(stream,encoder,*buffer,sendCount,True);
				}
				else FileSystemError(BINHEX_READ,spec->name,err);
				size -= count;
			}
			// if the file didn't end with a newline, add one
			if (!err && needNL) err = BufferSend(stream,encoder,NewLine+1,*NewLine,True);
			
			// send any remainder
			if (!err) err = BufferSend(stream,nil,nil,0,False);
			BufferSendRelease(stream);
			FSClose(refN);
		}
		ZapHandle(buffer);
	}
	else WarnUser(BINHEX_READ, err = MemError());
	return(err);
}
#endif
		
/************************************************************************
 * PeriodEncoder - encode periods
 ************************************************************************/
OSErr PeriodEncoder(CallType callType,DecoderPBPtr pb)
{
	static short nlState;
	
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				nlState = *NewLine;
				break;
			
			case kDecodeDone:
				break;
			
			case kDecodeDispose:
				break;
			
			case kDecodeData:
				if (pb->inlen)
					pb->outlen = StuffPeriods(pb->input,pb->inlen,pb->output,NewLine,&nlState);
				else
					pb->outlen = 0;
				break;
		}
	}
	return(noErr);
}

/************************************************************************
 * StuffPeriods - byte-stuff periods for SMTP
 ************************************************************************/
long StuffPeriods(UPtr in, long inLen, UPtr out, PStr newLine, short *nlStatePtr)
{
	short nlState = *nlStatePtr;
	UPtr end = in + inLen;
	short newLineLen = *newLine;
	UPtr origOut = out;
	
	for (end=in+inLen;in<end;in++)
	{
		if (*in=='.' && nlState==newLineLen)	// found period we need to stuff
		{
			*out++ = '.';
			nlState = 0;
		}
		else
		{
			if (nlState==newLineLen) nlState = 0;	// start over again
			
			if (*in==newLine[nlState+1]) nlState++;	// found one of the newline chars
			else nlState = 0;	// start over again
		}
		*out++ = *in;
	}
	
	*nlStatePtr = nlState;
	return(out-origOut);
}

/************************************************************************
 * IsPostScript - is a file a PostScript file?
 ************************************************************************/
Boolean IsPostScript(FSSpecPtr spec)
{
	short refN;
	Str31 psMagic;
	Str31 fileMagic;
	long count;
	Boolean result = False;
	
	if (!FSpOpenDF(spec,fsRdPerm,&refN))
	{
		GetRString(psMagic,PS_MAGIC);
		count = *psMagic;
		if (!ARead(refN,&count,fileMagic+1))
		{
			*fileMagic = *psMagic;
			result = StringSame(fileMagic,psMagic);
		}
		MyFSClose(refN);
	}
	return(result);
}

/************************************************************************
 * SendPlain - send a plain text file
 ************************************************************************/
short SendPlain(TransStream stream,FSSpec *spec,long flags,short tableId,AttMapPtr amp)
{
	int err;
	DecoderFunc *encoder = nil;
	UHandle taste = nil;
	Str31 scratch;
	FInfo info;
	
	/*
	 * send header
	 */
	if (amp->isPostScript)
	{
		if ((flags&FLAG_CAN_ENC))
		{
			encoder = B64Encoder;
			flags |= FLAG_ENCBOD;
		}
		if (err=MIMEFileHeader(stream,amp,POSTSCRIPT,AFSpGetMod(spec))) goto done;
		if (err = ComposeRTrans(stream,MIME_V_FMT,InterestHeadStrn+hContentEncoding,encoder ? MIME_BASE64 : MIME_BINARY,NewLine))
			goto done;
		DontTranslate = True;
		flags &= ~FLAG_WRAP_OUT;
	}
	else
	{
		flags &= ~FLAG_ENCBOD;	/* we may not need to encode this; we'll find out later */
		Snarf(spec,&taste,GetRLong(TEXT_QP_TASTE));
		if (err = SendContentType(stream,taste,0,nil,0,tableId,&flags,nil,ATT_MAP_NAME(amp),nil,amp->mm.subtype)) goto done;
		ZapHandle(taste);
		encoder = flags&FLAG_ENCBOD ? QPEncoder : nil;
		if (err = ComposeRTrans(stream,MIME_CD_FMT,
									 InterestHeadStrn+hContentDisposition,
									 ATTACHMENT,
									 AttributeStrn+aFilename,
									 ATT_MAP_NAME(amp),
									 NewLine))
			goto done;
		if (*R822Date(scratch,AFSpGetMod(spec)-ZoneSecs()) && (err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
										 AttributeStrn+aModDate,scratch,NewLine)))
			goto done;
	}
	FSpGetFInfo(spec,&info);
	if (!err && !amp->suppressXMac) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacType,Long2Hex(scratch,info.fdType),
									 NewLine);
	if (!err && !amp->suppressXMac) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacCreator,Long2Hex(scratch,info.fdCreator),
									 NewLine);

	if (err = SendPString(stream,NewLine)) goto done;
	
	err = SendTextFile(stream,spec,flags,encoder);
		
done:
	DontTranslate = False;
	BufferSendRelease(stream);
	return(err);	
} 			

/**********************************************************************
 * SendTextFile - send text from a file
 **********************************************************************/
OSErr SendTextFile(TransStream stream,FSSpecPtr spec,long flags,DecoderFunc *encoder)
{
	short refN=0;
	UHandle dataBuffer=nil;
	long dataSize;
	int err;
	long fileSize,sendSize,readSize;
	Boolean partial = False;

	/*
	 * allocate the buffers
	 */
	dataSize = GetRLong(BUFFER_SIZE);
	if (!(dataBuffer=NuHTempOK(dataSize)))
		{WarnUser(MEM_ERR,err=MemError()); goto done;}

	/*
	 * open it
	 */
	if (err = FSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	if (err = GetEOF(refN,&fileSize))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	
	/*
	 * send it
	 */
	for (;fileSize;fileSize-=readSize)
	{
		readSize=MIN(dataSize,fileSize);
		sendSize = readSize;
		if (err=ARead(refN,&sendSize,LDRef(dataBuffer)))
			{FileSystemError(BINHEX_READ,spec->name,err); goto done;}
		UL(dataBuffer);
		if (err = SendBodyLines(stream,dataBuffer,sendSize,0,flags,False,nil,0,partial,encoder))
			goto done;
		partial = (*dataBuffer)[sendSize-1] != '\015';
	}
	if (!err) err=BufferSend(stream,encoder,nil,0,True);
	if (!err && partial) SendPString(stream,NewLine);

done:
	DontTranslate = False;
	BufferSendRelease(stream);
	if (refN) MyFSClose(refN);
	if (dataBuffer) ZapHandle(dataBuffer);
	return(err);	
}

/************************************************************************
 * BuildDateHeader - build an RFC 822 date header
 ************************************************************************/
void BuildDateHeader(UPtr buffer,long seconds)
{
	Str63 date;
	if (*R822Date(date,seconds))
		ComposeRString(buffer,DATE_HEADER,HeaderStrn+DATE_HEAD,date);
	else *buffer = 0;
	return;
}

/************************************************************************
 * BuildDateHeader - build an RFC 822 date header
 ************************************************************************/
PStr R822Date(PStr date,long seconds)
{
	DateTimeRec dtr;
	long delta = ZoneSecs();
	Boolean negative;
	
	if (delta==-1) {*date=0;return date;}
	if (seconds)
		SecondsToDate(seconds+delta,&dtr);
	else
		GetTime(&dtr);
	if (negative=delta<0) delta *= -1;
	delta /= 60; /* we want minutes */
	return(ComposeRString(date,R822_DATE_FMT,
											WEEKDAY_STRN+dtr.dayOfWeek,
											dtr.day,
											MONTH_STRN+dtr.month,
											dtr.year,
											dtr.hour/10, dtr.hour%10,
											dtr.minute/10, dtr.minute%10,
											dtr.second/10, dtr.second%10,
											negative ? '-' : '+',
											delta/600,(delta%600)/60,(delta%60)/10,delta%10));
}

/************************************************************************
 * SaveB4Send - grab an outgoing message, saving if necessary
 ************************************************************************/
MessHandle SaveB4Send(TOCHandle tocH,short sumNum)
{
	short which;
	MessHandle messH = (MessHandle)(*tocH)->sums[sumNum].messH;
	
	if (messH && (*messH)->win->isDirty)
	{
		which = WannaSend((*messH)->win);
		if (which==CANCEL_ITEM || which==WANNA_SAVE_CANCEL)
			return (nil);
		else if (which == WANNA_SAVE_SAVE && !SaveComp((*messH)->win))
			return(nil);
		else if (which == WANNA_SAVE_DISCARD)
		{
			(*messH)->win->isDirty = False;
			CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
			messH = nil;
		} 	
	}
	if (!messH)
	{
		MyWindowPtr	winResult;
				
		MyThreadBeginCritical();	// Make sure OpenComp doesn't switch threads
		winResult = OpenComp(tocH,sumNum,nil,nil,False,False);
		MyThreadEndCritical();
		if (!winResult) return(nil);
		messH = (MessHandle)(*tocH)->sums[sumNum].messH;
	}
	return(messH);
}


/************************************************************************
 * BuildBoundary - build a boundary line for a message
 ************************************************************************/
void BuildBoundary(MessHandle messH,PStr boundary,PStr middle)
{
#pragma unused(messH)
	ComposeRString(boundary,MIME_BOUND1_FMT,GMTDateTime(),middle,MIME_BOUND2);
}

/************************************************************************
 * SendContentType - deduce and send the appropriate content-type
 *  (and CTE) for two blocks of text (body and signature, typically)
 *	text1 - one block of text (may be nil)
 *  text2 - second block of text (may be nil)
 *  tableID - xlate table id
 *  flags - message flags
 ************************************************************************/
OSErr SendContentType(TransStream stream,Handle text1, long offset1, Handle text2, long offset2, short tableID,long *flags,long *opts,PStr name,emsMIMEHandle *tlMIME,PStr subtype)
{
	short etid = EffectiveTID(tableID);
	Str127 scratch;
	Str63 flowed;
#ifdef ETL
	Str63 s2;
#endif
	Boolean anyfunny;
	Boolean any2022;
	short err;
	short encId;
	Boolean strip = 0!=(opts && (*opts & OPT_STRIP));
	Boolean rich = !strip && 0!=(flags && (*flags & FLAG_RICH));
	Boolean html = !strip && 0!=(opts && (*opts & OPT_HTML));
	short computedSubType = html?HTMLTagsStrn+htmlTag:(rich?MIME_RICHTEXT:MIME_PLAIN);
		
	if (rich || html) *flags &= ~FLAG_WRAP_OUT;
		
	anyfunny = !text1 || AnyFunny(text1,offset1) || text2 && AnyFunny(text2,offset2);
	any2022 = !PrefIsSet(PREF_NO_2022) && (text1 && Any2022(text1,offset1) || text2 && Any2022(text2,offset2));
	
	/*
	 * figure out proper charset
	 */
	if (anyfunny) NameCharset(scratch,etid,tlMIME);
	else if (any2022) NameCharset(scratch,kt2022,tlMIME);
	else NameCharset(scratch,ktMacUS,tlMIME);

	if (0!=(*flags&FLAG_WRAP_OUT) && UseFlowOut)
	{
		if (tlMIME)
			AddTLMIME(*tlMIME,TLMIME_PARAM,GetRString(flowed,AttributeStrn+aFormat),GetRString(s2,FORMAT_FLOWED));
		ComposeRString(flowed,MIME_CT_ANNOTATE,AttributeStrn+aFormat,GetRString(s2,FORMAT_FLOWED),"");
		PCat(scratch,flowed);
	}
	
	/*
	 * send the content type
	 */
	if (!name)
	{
		if (subtype)
			err = ComposeRTrans(stream,MIME_TEXTNOTPLAIN,
							InterestHeadStrn+hContentType,
							MIME_TEXT,
							subtype,
							scratch,
							NewLine);
		else
			err = ComposeRTrans(stream,MIME_TEXTPLAIN,
							InterestHeadStrn+hContentType,
							MIME_TEXT,
							computedSubType,
							scratch,
							NewLine);
#ifdef ETL
		if (tlMIME)
		{
			AddTLMIME(*tlMIME,TLMIME_TYPE,GetRString(scratch,MIME_TEXT),nil);
			AddTLMIME(*tlMIME,TLMIME_SUBTYPE,subtype?subtype:GetRString(scratch,computedSubType),nil);
		}
#endif
	}
	else
	{
		PCat(scratch,NewLine);
		if (subtype)
			err = ComposeRTrans(stream,MIME_TEXT_SUBTYPE_FMT,
											InterestHeadStrn+hContentType,
											MIME_TEXT,
											subtype,
											AttributeStrn+aName,
											name,
											scratch);
		else
			err = ComposeRTrans(stream,MIME_MP_FMT,
											InterestHeadStrn+hContentType,
											MIME_TEXT,
											computedSubType,
											AttributeStrn+aName,
											name,
											scratch);
#ifdef ETL
		if (tlMIME)
		{
			AddTLMIME(*tlMIME,TLMIME_TYPE,GetRString(scratch,MIME_TEXT),nil);
			AddTLMIME(*tlMIME,TLMIME_SUBTYPE,subtype?subtype:GetRString(scratch,computedSubType),nil);
		}
#endif
	}	
	if (err) return(err);
	
	/*
	 * content-transfer-encoding, if any
	 */
	if (0==(*flags&FLAG_ENCBOD))																		/* set manually? */
		*flags = DecideEncoding(text1,text2,anyfunny,etid,*flags);		/* determine automatically */
	
	if (0!=(*flags&FLAG_ENCBOD))
	{
		if ((*flags&FLAG_CAN_ENC) && (UUPCOut || (Ehlo && !(*Ehlo)->mime8bit) || !PrefIsSet(PREF_ALLOW_8BITMIME)))
			encId = MIME_QP;
		else
		{
			encId = MIME_8BIT;
			*flags &= ~FLAG_ENCBOD;
		}
		if (err = ComposeRTrans(stream,MIME_V_FMT,
										 InterestHeadStrn+hContentEncoding,
										 encId,
										 NewLine))
			return(err);
#ifdef ETL
		if (tlMIME)
		{
			AddTLMIME(*tlMIME,TLMIME_TYPE,GetRString(scratch,MIME_TEXT),nil);
			AddTLMIME(*tlMIME,TLMIME_PARAM,GetRString(scratch,MIME_CTE),GetRString(s2,encId));
		}
#endif
	}
	return(noErr);
}

/************************************************************************
 * DecideEncoding - decide which encoding (if any) to use
 ************************************************************************/
long DecideEncoding(Handle text1, Handle text2,Boolean anyfunny,short etid,long flags)
{
	if (etid==ktMacUS) anyfunny = False;
	
	if (anyfunny)
		flags |= FLAG_ENCBOD;	/* encode funny chars in QP */
	else if (0==(flags&FLAG_WRAP_OUT) && !(flags&FLAG_RICH))
	{
		if (!text1 || LongerThan(text1,GetRLong(MAX_SMTP_LINE)) ||
				text2 && LongerThan(text2,GetRLong(MAX_SMTP_LINE)))
			flags |= FLAG_ENCBOD;
	}
	else if (0==(flags&FLAG_ENCBOD) && (flags&FLAG_RICH))
	{
		if (!text1 || LongerWordThan(text1,GetRLong(ENRICHED_MAX_WORD)) ||
				text2 && LongerWordThan(text1,GetRLong(ENRICHED_MAX_WORD)))
			flags |= FLAG_ENCBOD;
	}
	return(flags);
}

/************************************************************************
 * SevenBitTable - is the table in question full of only 7-bit chars?
 ************************************************************************/
Boolean SevenBitTable(short tableID)
{
	Handle table;
	UPtr spot,end;
	
	if (!tableID || !(table = GetResource_('taBL',tableID))) return(False);
	
	for (spot=*table+127,end=*table+256;spot<end;spot++) if (*spot>126) return(False);

	return(True);
}

/************************************************************************
 * AnyFunny - does a block of text contain funny chars?
 ************************************************************************/
Boolean AnyFunny(Handle text,long offset)
{
	UPtr spot,end;
	Str255 line;
	
	if (!text || !*text) return(True);
	
	// check for high bits
	if (Flatten)
	{
		for (spot=*text+offset,end=spot+GetHandleSize_(text)-offset; spot<end; spot++)
			if (Flatten[*spot]>126) return(True);
	}
	else
	{
		for (spot=*text+offset,end=spot+GetHandleSize_(text)-offset; spot<end; spot++)
			if (*spot>126) return(True);
	}

	// check for uucp envelopes
	for (spot=*text+offset;spot<end;spot++)
	{
		if (*spot=='\015') break;
	}
	if (spot<end)
	{
		MakePStr(line,*text+offset,spot-*text-offset);
		if (*line && IsFromLine(line+1)) return true;
	}
	
	// all quiet on the western front
	return(False);
}

/************************************************************************
 * Any2022 - does a block of text contain 2022?
 ************************************************************************/
Boolean Any2022(Handle text,long offset)
{
	UPtr spot,end;
	
	if (!text || !*text) return(True);
	
	for (spot=*text+offset,end=spot+GetHandleSize_(text)-offset; spot<end; spot++)
		if (spot[0]==escChar && spot[1]=='$') return(True);

	return(False);
}

/************************************************************************
 * EffectiveTID - what is the effective table id?
 ************************************************************************/
short EffectiveTID(short tid)
{
	Str31 pTable;
	
	if (!NewTables) return(TransOutTablID());
	if (tid==NO_TABLE) return(0);
	if (tid==DEFAULT_TABLE)
		return(*GetPref(pTable,PREF_OUT_XLATE) ?
			GetPrefLong(PREF_OUT_XLATE) : TransOutTablID());
	else return(tid);
}

/************************************************************************
 * TransOutTablID - return the translit table name to use for high-bit chars
 *   when not using the new table support.  Pretty hacky, I'm afraid.
 ************************************************************************/
short TransOutTablID(void)
{
	Str255 name;
	GetPref(name,PREF_SEND_CSET);
	if (EqualStrRes(name,MIME_ISO_LATIN1)) return TRANS_OUT_TABL_8859_1;
	if (EqualStrRes(name,MIME_ISO_LATIN15)) return ktMacISO15;
	if (EqualStrRes(name,MIME_WIN_1252)) return ktMacWindows;
	if (EqualStrRes(name,MIME_MAC)) return ktIdendity;
	return TRANS_OUT_TABL_8859_1; // Oh well, pref is junk...
}

/************************************************************************
 * TransOutTablName - return the translit table name to use for high-bit chars
 *   when not using the new table support.  Pretty hacky, I'm afraid.
 ************************************************************************/
PStr TransOutTablName(PStr name)
{
	GetPref(name,PREF_SEND_CSET);
	if (EqualStrRes(name,MIME_ISO_LATIN1)) return name;
	if (EqualStrRes(name,MIME_ISO_LATIN15)) return name;
	if (EqualStrRes(name,MIME_WIN_1252)) return name;
	if (EqualStrRes(name,MIME_MAC)) return name;
	return GetRString(name,MIME_ISO_LATIN1); // Oh well, pref is junk...
}


/************************************************************************
 * NameCharset - build a charset= parameter
 ************************************************************************/
PStr NameCharset(PStr charset,short tid,emsMIMEHandle *tlMIME)
{
	Str63 scratch;
#ifdef ETL
	Str63 header;
#endif
	
	if (*SimpleNameCharset(scratch,tid))
	{
		ComposeRString(charset,MIME_CSET,scratch);
#ifdef ETL
		if (tlMIME) AddTLMIME(*tlMIME,TLMIME_PARAM,GetRString(header,MIME_CHARSET),scratch);
#endif
	}
	else
		*charset = 0;

	return(charset);
}

/**********************************************************************
 * SimpleNameCharset - return just the name of a charset
 **********************************************************************/
PStr SimpleNameCharset(PStr name,short tid)
{
	Handle res;
	short id;
	ResType type;
	
	if (!tid)
		GetRString(name,MIME_MAC);
	else if (tid==ktMacUS)
		GetRString(name,MIME_USASCII);
	else if (tid==TransOutTablID())
		TransOutTablName(name);
	else if (tid==TRANS_OUT_TABL_8859_1 || tid==ktMacISO || tid==ktISOMac || tid==TRANS_IN_TABL)
		GetRString(name,MIME_ISO_LATIN1);
	else if (tid==kt2022)
		GetRString(name,ISO_2022_JP);
	else
	{
		if (!GetTableCName(tid-1,name))
		{
			res = GetResource_('taBL',tid);
			if (res)
			{
				GetResInfo(res,&id,&type,name);
			}
			else
				*name = 0;
		}
	}
	return(name);
}

/************************************************************************
 * LongerThan - is there a line longer than some number of chars?
 ************************************************************************/
Boolean LongerThan(Handle text, short len)
{
	UPtr spot,end,nl;
	
	spot = *text;
	end = spot + GetHandleSize_(text);
	nl = spot-1;
	
	for (;spot<end;spot++)
	{
		if (*spot=='\015')
		{
			if (spot-nl-1>len) return(True);
			nl = spot;
		}
	}
	return(False);
}

/************************************************************************
 * LongerWordThan - is there a "word" longer than some number of chars?
 ************************************************************************/
Boolean LongerWordThan(Handle text, short len)
{
	UPtr spot,end,nl;
	
	spot = *text;
	end = spot + GetHandleSize_(text);
	nl = spot-1;
	
	for (;spot<end;spot++)
	{
		if (*spot=='\015' || *spot==' ')
		{
			if (spot-nl-1>len) return(True);
			nl = spot;
		}
	}
	return(False);
}

/************************************************************************
 * Next1342Word - parse a word from a 1342 stream
 ************************************************************************/
void Next1342Word(UPtr *startP,UPtr end,Token1342Ptr current,PStr delim,Boolean *wasQuote,Boolean *encQuote)
{
	Str63 word;
	short wordLim = 48;
	Enum1342 wordType;
	Byte c;
	UPtr source = *startP;
	Boolean justSpace;
	Boolean newWasQuote;
	UPtr qSpot;
			
	/*
	 * are we off the end?
	 */
	if (source>=end)
	{
		wordType = k1342End;
		*word = 0;
	}
	else
	{
		c = *source++;
		*word = 1;
		word[1] = c;
		if (*wasQuote || c=='"')			/* collect a quote */
		{
			if (!*wasQuote)
			{	/* search to end of quote to see if quote contains any high bits */
				for (qSpot=source;qSpot<end;qSpot++)
					if (qSpot[0]=='"' && qSpot[-1]!='\\') break;
				*encQuote = AnyHighBits(source,qSpot-source);
			}
			newWasQuote = True;
			while (source<end && *word<wordLim)
			{
				PCatC(word,*source);
				if (*source>=0x80) wordLim -= 2;	/* allow space for encoding */
				if (*source++ == '"' && word[*word-1]!='\\')
				{
					newWasQuote = False;
					break;	/* closing " */
				}
			}
			wordType = *encQuote ? k1342Word : k1342Plain;
			*wasQuote = newWasQuote;
		}
		else if (strchr(delim+1,c))	/* collect a string of delimiters */
		{
			justSpace = c==' ';
			while (source<end && *word<wordLim)
			  if (*source!='"' && strchr(delim+1,*source))
				{
					PCatC(word,*source);
					if (*source!=' ') justSpace = False;
					source++;
				}
				else break;
			wordType = justSpace ? k1342LWSP : k1342Plain;
		}
		else	/* collect a regular word */
		{
			UPtr whichDelim;
			UPtr lastSP = nil;
			short oldWordLim;
			short oldWordSize;
			
			while (source<end && *word<wordLim)
			  if (whichDelim=strchr(delim+1,*source))
			  {
			  	if (*whichDelim==' ')
			  	{
			  		if (!AnyHighBits(word+1,*word)) break;
			  		// We've seen some stuff we need to encode, and now we've
			  		// seen a space.  Remember where we saw it.  If we overflow
			  		// our buffer, then we'll truncate to before here so we always
			  		// have integral words in our encoding.  With luck, this will
			  		// allow us to make less stupid choices about encoding words by
			  		// sometimes including spaces in encoded words rather than encoding
			  		// space runs
			  		lastSP = source;
			  		oldWordLim = wordLim;
			  		oldWordSize = *word;
			  		PCatC(word,*source++);
			  	}
			  	else
			  		break;
			  }
				else
				{
				  if (*source>=0x80) wordLim -= 2;	/* allow space for encoding */
					PCatC(word,*source);
					source++;
				}
			
			// did we end with chars left? If not, and if we have a prior space,
			// back up to that prior space
			if (*word >= wordLim && lastSP)
			{
				source = lastSP;
				wordLim = oldWordLim;
				*word = oldWordSize;
			}
						
			wordType = AnyHighBits(word+1,*word) ? k1342Word : k1342Plain;
			// trim trailing spaces from encoded words
			if (wordType == k1342Word)
				while (*word && word[*word]==' ')
				{
					--*word;
					--source;
				}
		}
	}
	
	PCopy(current->word,word);			/* copy word and type into current buffer */
	current->wordType = wordType;
	*startP = source;								/* mark new position in string */
}

/************************************************************************
 * Encode1342 - encode a header line ala RFC 1342 (1522, actually)
 ************************************************************************/
Handle Encode1342(UPtr source,long len,short lineLimit,short *charsOnLine,PStr nl,short tid)
{
	Token1342 tokens[3];
	Token1342Ptr prev, curr, next;
	Str31 dl1342;
	Boolean continueQuote = False;
	Boolean encQuote = False;
	UPtr spot = source;
	short line;
	Handle encoded = NuHandle(0);
	Boolean wrapped;
	Byte c;
	
	if (!encoded) goto fail;
	
	/*
	 * grab delimiter list
	 */
	GetRString(dl1342,RFC1342_DELIMS);
	
	/*
	 * prime initial line length
	 */
	line = charsOnLine ? *charsOnLine : 0;
	
	/*
	 * initialize token buffer
	 */
	tokens[0].wordType = k1342End;
	*tokens[0].word = 0;
	
	/*
	 * read first token
	 */
	Next1342Word(&spot,source+len,tokens+1,dl1342,&continueQuote,&encQuote);
	
	/*
	 * main loop
	 */
	for (curr = tokens+1;curr->wordType!=k1342End;curr = next)
	{
		next = RingNext(curr,tokens,3);
		prev = RingNext(next,tokens,3);
		Next1342Word(&spot,source+len,next,dl1342,&continueQuote,&encQuote);
		
		/*
		 * whitespace between two encoded words gets elided.  Therefore, if we
		 * are looking at whitespace and both the previous and next words are
		 * encoded, we must encode the current word.
		 *
		 * if either the previous or next words are not encoded, then we don't
		 * need to do anything special
		 */
		if (curr->wordType==k1342LWSP && prev->wordType==k1342Word && next->wordType==k1342Word)
				curr->wordType = k1342Word;
		
		/*
		 * if we are encoding, get with it
		 */
		if (curr->wordType == k1342Word) Encode1342String(curr->word,tid);
		
		/*
		 * worry about line wrapping
		 */
		wrapped = (prev->wordType!=k1342Plain || curr->wordType!=k1342Plain) &&
							(lineLimit && line+*curr->word>=lineLimit);
		if (wrapped)
		{
			if (PtrPlusHand_(nl+1,encoded,*nl)) goto fail;
			if (PtrPlusHand_(" ",encoded,1)) goto fail;
			line = 1;
		}
		
		/*
		 * if we are outputting an encoded word and the previous thing was an
		 * encoded word, or if the previous thing was a regular word that did
		 * NOT end with a space or a ')', then we must prepend a space to the
		 * encoded word
		 */
		c = prev->word[*prev->word];
		if (!wrapped && curr->wordType==k1342Word &&
		    (prev->wordType==k1342Word ||
				 prev->wordType==k1342Plain && c!=' ' && c!=')'))
		{
			BMD(curr->word+1,curr->word+2,*curr->word);
			++*curr->word;
			curr->word[1] = ' ';
		}
		
		/*
		 * if we are outputting an encoded word and the next thing is a
		 * plain word, then we must append a space to the encoded word
		 */
		c = next->word[1];
		if (curr->wordType==k1342Word && next->wordType==k1342Plain)
			PCatC(curr->word,' ');
			
		/*
		 * stick the word on the end
		 */
		if (PtrPlusHand_(curr->word+1,encoded,*curr->word)) goto fail;
		line += *curr->word;
	}

	if (charsOnLine) *charsOnLine = line;
	return(encoded);

fail:
	ZapHandle(encoded);
	return(nil);
}

/************************************************************************
 * Encode1342String - encode a string in 1342-speak
 ************************************************************************/
void Encode1342String(PStr s,short tid)
{
	Str255 encoded;
	Str63 name;
	UPtr from, to, end;
	Byte c;
		
	/*
	 * first, we translit to ISO-latin1
	 */
	if (tid) TransLitRes(s+1,*s,tid);
	
	/*
	 * now, we encode
	 */
	to = encoded+1;
	end = s + *s + 1;
	for (from=s+1;from<end;from++)
	{
		c = *from;
		if ('a'<=c&&c<='z' || 'A'<=c&&c<='Z' || '0'<=c&&c<='9') *to++ = c;
		else if (c==' ')
			*to++='_';
		else
		{
			*to++ = '=';
			Bytes2Hex(&c,1,to);
			to += 2;
		}
		if (to>encoded+sizeof(encoded)-20) return;	/* OVERFLOW */
	}
	
	*encoded = to-encoded-1;
	ComposeRString(s,RFC1342_FMT,SimpleNameCharset(name,tid),encoded);		
}

#pragma segment SMTPUtil
/************************************************************************
 * SendPString - send a pascal string
 ************************************************************************/
OSErr SendPString(TransStream stream,PStr string)
{
	return(SendTrans(stream,string+1,*string,nil));
}

/************************************************************************
 * TimeStamp - put a time stamp on a message
 ************************************************************************/
void TimeStamp(TOCHandle tocH,short sumNum,uLong when,long delta)
{
	PtrTimeStamp(LDRef(tocH)->sums+sumNum,when,delta);
	UL(tocH);
#ifdef NEVER
	CalcSumLengths(tocH,sumNum);
#endif
	InvalSum(tocH,sumNum);
	TOCSetDirty(tocH,true);
}

/************************************************************************
 * PtrTimeStamp - timestamp, but into a sum directly
 ************************************************************************/
void PtrTimeStamp(MSumPtr sum,uLong when,long delta)
{
	sum->seconds = when;
	sum->origZone = delta/60;
}

PStr FormatZone(PStr string,long delta)
{
	Boolean neg = delta<0;
	
	if (neg) delta *= -1;
	delta /= 60;	/* minutes*/
	ComposeString(string,"\p %c%d%d%d%d",neg?'-':'+',
										delta/600,(delta%600)/60,(delta%60)/10,delta%10);
	return(string);
}



/************************************************************************
 * BufferSend - send a buffer of (possibly encoded) data
 *  encoder - function to call for encoding
 *	data - data to encode/send (or nil to send remaining data and close)
 *	dataLen - length of data to encode/send
 ************************************************************************/
OSErr BufferSend(TransStream stream, DecoderFunc *encoder, UPtr data, long dataLen, Boolean text)
{
	short err = noErr;
	static long used;
	long consumed;
	UPtr spot,end;
	long bSize;
	long progBytes;
	
	if (EncoderGlobalsOldEncoder && EncoderGlobalsOldEncoder!=encoder)
	{
		BufferSendRelease(stream);
		EncoderGlobalsOldEncoder = encoder;
	}
	
	/*
	 * are we being fed data?
	 */
	if (data)
	{
		/*
		 * do we need to initialize?
		 */
		if (!EncoderGlobalsBuffers[0])
		{
			bSize = 3*GetRLong(BUFFER_SIZE);
			while (!(EncoderGlobalsBuffers[0] = NuHTempOK(bSize)) && bSize > 256) bSize /= 2;
			if (!EncoderGlobalsBuffers[0])
			{
				WarnUser(MEM_ERR,err=MemError());
				return(err);
			}
#ifdef DEBUG
			if (!BUG5)
#endif
			if (AsyncSendTrans) EncoderGlobalsBuffers[1] = NuHTempOK(bSize);
			EncoderGlobalsBuffer = EncoderGlobalsBuffers[0];
			if (encoder) err = (*encoder)(kDecodeInit,&EncoderGlobalsPb);
			EncoderGlobalsPb.text = text;
			if (err) {BufferSend(stream,encoder,nil,0,0); return(err);}
			used = 0;
		}
	
		bSize = GetHandleSize_(EncoderGlobalsBuffer);
		
		if (!DontTranslate && Flatten)
			for (spot=data,end=data+dataLen;spot<end; spot++) *spot = Flatten[*spot];
		
		if (!DontTranslate && TransOut)
			for (spot=data,end=data+dataLen; spot<end; spot++) *spot = TransOut[*spot];
		
		while (dataLen)
		{
			progBytes = consumed = 0;
			
			/*
			 * encode?
			 */
			if (encoder)
			{
				if ((!used || bSize-used > 4*dataLen))
				{
					EncoderGlobalsPb.output = LDRef(EncoderGlobalsBuffer)+used;
					consumed = EncoderGlobalsPb.inlen = MIN((bSize-used)/4,dataLen);
					EncoderGlobalsPb.input = data;
					err = (*encoder)(kDecodeData,&EncoderGlobalsPb);
					UL(EncoderGlobalsBuffer);
					if (err) return(err);
					used += EncoderGlobalsPb.outlen;
					progBytes = EncoderGlobalsPb.outlen;
				}
			}
			/*
			 * no, just copy
			 */
			else
			{
				consumed = MIN(bSize-used,dataLen);
				BMD(data,*EncoderGlobalsBuffer+used,consumed);
				used += consumed;
			}
			
			/*
			 * send
			 */
			if (consumed < dataLen)
			{
				if (AsyncSendTrans && EncoderGlobalsBuffers[1])
				{
					err = AsyncSendTrans(stream,LDRef(EncoderGlobalsBuffer),used);
					EncoderGlobalsBuffer = EncoderGlobalsBuffer == EncoderGlobalsBuffers[0] ? EncoderGlobalsBuffers[1] : EncoderGlobalsBuffers[0];
				}
				else
				{
					err = SendTrans(stream,LDRef(EncoderGlobalsBuffer),used,nil);
					UL(EncoderGlobalsBuffer);
				}
				if (err) return(err);
				used = 0;
			}
			
			dataLen -= consumed;
			data += consumed;
			if (progBytes) ByteProgress(nil,-progBytes,0);
		}
	}
	
	/*
	 * no data; clear out encoder
	 */
	else
	{
		if (AsyncSendTrans)  err = AsyncSendTrans(stream,nil,-1);
		if (!err && used && !dataLen && EncoderGlobalsBuffer) err = SendTrans(stream,LDRef(EncoderGlobalsBuffer),used,nil);
		
		if (encoder && EncoderGlobalsPb.refCon)
		{
			if (!err)
			{
				EncoderGlobalsPb.output = LDRef(EncoderGlobalsBuffer);
				err = (*encoder)(kDecodeDone,&EncoderGlobalsPb);
				if (!err && EncoderGlobalsPb.outlen && !dataLen)
					err = SendTrans(stream,*EncoderGlobalsBuffer,EncoderGlobalsPb.outlen,nil);
			}
			(*encoder)(kDecodeDispose,&EncoderGlobalsPb);
		}
		WriteZero(&EncoderGlobalsPb,sizeof(EncoderGlobalsPb));
		used = 0;
		ZapHandle(EncoderGlobalsBuffers[0]);
		ZapHandle(EncoderGlobalsBuffers[1]);
		EncoderGlobalsBuffer = nil;
	}

	return(err);
}

/************************************************************************
 * GetIndAttachment - get a particular attacment
 ************************************************************************/
OSErr GetIndAttachment(MessHandle messH,short index,FSSpecPtr spec,HSPtr where)
{
   OSErr err=1;
	Handle text=nil;
	HeadSpec hs;
	
	if (CompHeadFind(messH,ATTACH_HEAD,&hs))
	{
	   if (!(err = PETEGetRawText(PETE,TheBody,&text)))
   	   err = GetIndAttachmentLo(text,index,spec,where,&hs);
	}
	return(err);
}

/************************************************************************
 * GetIndAttachmentLo - get a particular attacment
 ************************************************************************/
OSErr GetIndAttachmentLo(Handle text,short index,FSSpecPtr spec,HSPtr where,HeadSpec *hs)
{
	short colons[4];
	short onColon;
	Str31 name;
	Str31 volName;
	long id;
	int onChar;
	OSErr err;
		
	onColon = 0;
	for (onChar=hs->value;onChar<hs->stop;onChar++)
		if ((*text)[onChar] == ':')
		{
			colons[onColon] = onChar;
			if (++onColon==sizeof(colons)/sizeof(short))
			{
				index--;
				onColon = 0;
				if (!index)
				{
					BMD((*text)+colons[0]+1,volName+1,colons[1]-colons[0]);
					*volName = colons[1]-colons[0];
					id = Atoi(LDRef(text)+colons[1]+1);
					BMD((*text)+colons[2]+1,name+1,colons[3]-colons[2]-1);
					if (where)
					{
						where->start = where->value = colons[0];
						where->stop = colons[3]+1;
					}
					*name = colons[3]-colons[2]-1;
					if (err = FSMakeFSSpec(GetMyVR(volName),id,name,spec))
					{
						// This file probably wasn't found. Go ahead and
						// build spec manually. May need name later on.
						spec->vRefNum = GetMyVR(volName);
						spec->parID = id;
						PStrCopy(spec->name,name,sizeof(spec->name));
					}
					hs->value = onChar+1;
					return err;
				}
			}
		}
	hs->value = onChar;
	return(1);	/* no more files */
}

/************************************************************************
 * PriorityHeader: Build a priority header
 ************************************************************************/
UPtr PriorityHeader(UPtr buffer,Byte priority)
{
	return(ComposeRString(buffer,PRIORITY_FMT,HEADER_STRN+PRIORITY_HEAD,priority,PRIOR_STRN+priority));
}

/************************************************************************
 * GetReply - get a reply to an SMTP command
 ************************************************************************/
int GetReplyLo(TransStream stream, UPtr buffer, int size, AccuPtr bufAcc, Boolean verbose,Boolean isEhlo)
{
	long rSize;
	Str127 scratch;
	char *cp;
	short err;
	Str255 tempBuffer;
	
	// if a buffer was not passed in...
	if (!buffer)
	{
		buffer = tempBuffer;
		size = 255;
	}
	
#ifdef TWO
	if (PrefIsSet(PREF_POP_SEND))
	{
		rSize = size;
		err = POPCmdGetReply(stream,-1,"",buffer,&rSize);
		if (err) return(602);
		if (*buffer=='+') BMD("200 ",buffer,4);
		else BMD("550 ",buffer,4);
		cp = buffer;
	}
	else
#endif
	do
	{
		Boolean partialBuffer;
		
		rSize = size;
		if (bufAcc) bufAcc->offset = 0;
		if (CommandPeriod || (sErr=RecvLine(stream,buffer,&rSize))) return(602);	/* error receiving */
		if (bufAcc) AccuAddPtr(bufAcc,buffer,rSize);
		partialBuffer = rSize && buffer[rSize-1]!='\r';

		// if we've been given an accumulator,
		// accumulate the whole response if this one is incomplete
		if (bufAcc && partialBuffer)
		{
			while(buffer[rSize-1]!='\r')
			{
				rSize = size;
				sErr = RecvLine(stream,buffer,&rSize);
				if (CommandPeriod || sErr) return 602;
				AccuAddPtr(bufAcc,buffer,rSize);
			}
			
			// pretend that what we got was just that first bufferful
			rSize = min(size,bufAcc->offset);
			BMD(*bufAcc->data,buffer,rSize);
		}
		else if (partialBuffer)
		{
			// Ick - not all of the reply will fit in the buffer, and we
			// weren't given an accumulator to keep it in.  Throw stuff away until
			// we find the end of the line
			Str63 dumpBuffer;
			long dumpSize;
			
			do
			{
				dumpSize = sizeof(dumpBuffer);
				sErr = RecvLine(stream,dumpBuffer,&dumpSize);
				if (CommandPeriod || sErr) return 602;
				CarefulLog(LOG_PROTO,DISCARD_LOG_FMT,dumpBuffer,dumpSize);
			}
			while (dumpBuffer[dumpSize-1]!='\r');
		}
		
		if (verbose)
		{
			*scratch = MIN(127,rSize);
			strncpy(scratch+1,buffer,*scratch);
			ProgressMessage(kpMessage,scratch);
		}
		for (cp=buffer;cp<buffer+rSize && (*cp < ' ' || *cp>'~');cp++);
		if (isEhlo && cp[0]=='2' && cp[1]=='5' && cp[2]=='0')
			EhloLine(buffer,rSize);
		rSize -= cp-buffer;			
	}
	while (rSize<3 ||
				 !isdigit(cp[0])||!isdigit(cp[1])||!isdigit(cp[2]) ||
								 rSize>3 && cp[3]=='-');
	cp[rSize] = 0;
	err = Atoi(cp);
	if (verbose && err>399 && err<600) SMTPCmdError(nil,nil,buffer);
	if (err==505 || err==530) SetPref(PREF_SMTP_GAVE_530,YesStr);
	if (err==452) err = 552;	// Goddam stupid SMTP spec gave a 4xx series response
														// to a permanent error code
	return(err);
} 

/************************************************************************
 * FlattenAndSpool - flatten and spool a movie
 * Changes the filespec passed to it!
 ************************************************************************/
OSErr FlattenAndSpool(FSSpecPtr spec)
{
	FSSpec tempSpec;
	OSErr err = NewTempSpec(0,0,spec->name,&tempSpec);
	
	if (!err)
	{
		if (err=FlattenQTMovie(spec,&tempSpec)) FSpDelete(&tempSpec);
		else
		{
			*spec = tempSpec;
			FSpKillRFork(spec);
		}
	}
	return(err);
}

/**********************************************************************
 * FlattenQTMovie - put movie in data fork of new file
 **********************************************************************/
OSErr FlattenQTMovie(FSSpecPtr inSpec,FSSpecPtr outSpec)
{
  short	movieResFile;
	Movie theMovie,tempMovie;
	OSErr	err = noErr;
	
	if (!HaveQuickTime(0x0100))
		return cantOpenHandler;	//	Don't have QuickTime
		
	if (!QTMoviesInited)
	{
		if (!EnterMovies())	//	Need to do this once
			QTMoviesInited = true;
		err = GetMoviesError();				
	}
	
	if (QTMoviesInited && !(err = OpenMovieFile(inSpec,&movieResFile,fsRdPerm)))
	{
		short		movieResID = 0;   /* want first movie */
    Boolean	wasChanged;

		err = NewMovieFromFile (&theMovie,movieResFile,&movieResID,nil,0,&wasChanged);
		CloseMovieFile(movieResFile);       

		if (!err)
		{
			tempMovie = FlattenMovieData(theMovie,flattenAddMovieToDataFork,outSpec,FileCreatorOf(inSpec),smSystemScript,createMovieFileDeleteCurFile);
			err = GetMoviesError();
			if (tempMovie) DisposeMovie(tempMovie);
			DisposeMovie(theMovie);
		}
	}
		
	return err;
}