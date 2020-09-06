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

#include "tcp.h"
#define FILE_NUM 37
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for i/o over a pseudo-telnet MacTcp stream
 * these functions are oriented toward CONVENIENCE, not performance
 ************************************************************************/

#pragma segment TcpTrans

/************************************************************************
 * private functions
 ************************************************************************/
#define TcpTrouble(stream,which,err) TT(stream,which,err,FNAME_STRN+FILE_NUM,__LINE__)
int TT(TransStream stream,int which, int theErr, int file, int line);
uLong RandomAddr(uLong *addrs);
void NoteAddrGoodness(struct hostInfo *hip,uLong addr,short err);
pascal void BindDone(struct hostInfo *hostInfoPtr, char *userData);
pascal void TcpASR(StreamPtr tcpStream, unsigned short eventCode,UPtr userDataPtr, unsigned short terminReason, struct ICMPReport *icmpMsg);

/**********************************************************************
 * ConnectTransLo - connect to a host
 **********************************************************************/
OSErr ConnectTrans(TransStream stream, UPtr serverName, long port, Boolean silently,uLong timeout)
{
	Str255 realHost;
	
	PCopy(realHost,serverName);
	
	SplitPort(realHost,&port);

	return ConnectTransLo(stream,realHost,port,silently,timeout);
}

/**********************************************************************
 * SplitPort - Split host:port into components
 *  returns true if port found
 **********************************************************************/
Boolean SplitPort(PStr host,long *port)
{
	UPtr colon;
	long localPort;
	
	if (colon=PIndex(host,':'))
	{
		*colon = *host - (colon-host);
		*host = colon-host-1;
		StringToNum(colon,&localPort);
		ASSERT(localPort>0);
		if (localPort>0 && port) *port = localPort;
		return true;
	}
	
	return false;
}

/**********************************************************************
 * RandomAddr - pick a random address out of a set of addresses
 **********************************************************************/
uLong RandomAddr(uLong *addrs)
{
	short count;
	if (!PrefIsSet(PREF_DNS_BALANCE)) return(addrs[0]);
	
	for (count=NUM_ALT_ADDRS;count;count--) if (addrs[count-1]) break;
	return(addrs[count<2?0:TickCount()%count]);
}

/**********************************************************************
 * NoteAddrGoodness - remove an address or all except an address
 **********************************************************************/
void NoteAddrGoodness(struct hostInfo *hip,uLong addr,short err)
{
	short count,i;

	if (!PrefIsSet(PREF_DNS_BALANCE)) return;
	
	for (count=NUM_ALT_ADDRS;count;count--) if (hip->addr[count-1]==addr) break;

	if (err)
	{
		/* this addr didn't work.  remove it */
		for (i=count;i<NUM_ALT_ADDRS;i++) hip->addr[i-1] = hip->addr[i];
		hip->addr[NUM_ALT_ADDRS-1] = 0;
		if (count==1) *hip->cname = 0;	/* if used last addr, kill the name to force another lookup */
	}
	else
	{
		/* mark the OTHERS as dead. */
		WriteZero(hip->addr,sizeof(uLong)*NUM_ALT_ADDRS);
		hip->addr[0] = addr;
	}
}

#pragma segment Main
/************************************************************************
 * BindDone - report that the resolver has done its duty
 ************************************************************************/
pascal void BindDone(struct hostInfo *hostInfoPtr, char *userData)
{
	*(short *)userData = hostInfoPtr->rtnCode;
	return;
}
#pragma segment TcpTrans

/************************************************************************
 * TcpTrouble - report an error with TCP and break the connection
 ************************************************************************/
int TT(TransStream stream, int which, int theErr, int file, int line)
{	
	if (stream==0 || (!stream->BeSilent && (!CommandPeriod || stream->Opening&&!PrefIsSet(PREF_OFFLINE)&&!PrefIsSet(PREF_NO_OFF_OFFER))))
	{
		Str255 message;
		Str255 tcpMessage;
		Str63 debugStr;
		Str31 rawNumber;
		short realSettingsRef = SettingsRefN;

		NumToString(theErr,rawNumber);
		
		SettingsRefN = GetMainGlobalSettingsRefN();
		GetRString(message, which);
		if (-23000>=theErr && theErr >=-23048)
			GetRString(tcpMessage,MACTCP_ERR_STRN-22999-theErr);
		else if (2<=theErr && theErr<=9)
			GetRString(tcpMessage,MACTCP_ERR_STRN+theErr+(23048-23000));
		else
			*tcpMessage = 0;
		ComposeRString(debugStr,FILE_LINE_FMT,file,line);
		SettingsRefN = realSettingsRef;


		MyParamText(message,rawNumber,tcpMessage,debugStr);
		if (stream==0 || stream->Opening)
		{
			if (2==ReallyDoAnAlert(OPEN_ERR_ALRT,Caution))
				SetPref(PREF_OFFLINE,YesStr);
		}
		else ReallyDoAnAlert(BIG_OK_ALRT,Caution);
	}
	return(stream ? (stream->streamErr=theErr) : theErr);
}

/************************************************************************
 * TcpASR - asynchronous notification routine
 ************************************************************************/
pascal void TcpASR(StreamPtr tcpStream, unsigned short eventCode,
	UPtr userDataPtr, unsigned short terminReason, struct ICMPReport *icmpMsg)
{
//#pragma unused(userDataPtr)
	if (tcpStream==((TransStreamPtr)userDataPtr)->TcpStream)
	{
		if (eventCode==TCPDataArrival) ((TransStreamPtr)userDataPtr)->CharsAvail = 1;
		else if (eventCode==TCPICMPReceived)
		{
			ICMPAvail = 1;
			ICMPMessage = *icmpMsg;
		}
		else if (eventCode==TCPTerminate)
		{
			WhyTCPTerminated = terminReason;
		}
	}
}


#pragma segment TcpTrans
/*********************************************************************************
 * Eudora might should take advantage of native Open Transport TCP/IP when it's 
 * installed.  The following functions implement the same as the MacTCP ones above,
 * but employ OT.  There's also some PPP stuff here, too.
 *********************************************************************************/
 
OSErr pppErr = noErr;
MyOTPPPInfoStruct	MyOTPPPInfo;
unsigned long connectionSelection = kOtherSelected;
long oldDlogState = 0;
Boolean needPPPConnection = false;	//used to determine when PPP is down but should be up.
Boolean OTinitialized = false;
Boolean dialingThePhone = false;			//set to true after we've dialed the phone.
Boolean gUpdateTPWindow = false;	// set to true when the connection method changes to invalidate the TP window

//setup
Boolean HasOTInetLib(void);
Boolean OTSupported(void);
OSErr OpenOTInternetServices(MyOTInetSvcInfo *myOTInfo);
pascal void MyOTNotifyProc(MyOTInetSvcInfo *info, OTEventCode theEvent, OTResult theResult, void *theParam);
OSErr CreateOTStream(TransStream stream);
OSErr OTTCPOpen(TransStream stream, InetHost tryAddr, InetPort port,uLong timeout);
void EnqueueMyTStream(MyOTTCPStreamPtr myStream);
void DestroyMyTStream(MyOTTCPStreamPtr myStream);

//communication
OSErr OTGetHostByName(InetDomainName hostName, InetHostInfo *hostInfoPtr);
OSErr OTGetHostByAddr(InetHost addr, InetHostInfo *domainNamePtr);
InetHost OTRandomAddr(InetHostInfo *host);
OSErr OTGetDomainMX(InetDomainName hostName, InetMailExchange *MXPtr, short *numMX);
void GetPreferredMX(InetDomainName preferred, InetMailExchange *MXPtr, short numMX);
short OTWaitForChars(TransStream stream, long timeout, UPtr line, long *size, OTResult *otResult);
pascal void MyOTStreamNotifyProc (MyOTTCPStream *myStream, OTEventCode code, OTResult theResult, void *theParam);
short SpinOnWithConnectionCheck(short *rtnCodeAddr,long maxTicks,Boolean allowCancel,Boolean forever);

//error reporting
OSErr OTTE(TransStream stream, OSErr generalError, OSErr specficError, short file, short line);
#define OTTCPError(stream,generalError,specficError) OTTE(stream,generalError,specficError,FNAME_STRN+FILE_NUM,__LINE__)
#define IsMyOTError(x) (x >= errOTInitFailed && x <= errMyLastOTErr)
#define IsOTPPPError(x) (x <= kCCLErrorStart && x >= kCCLErrorEnd)

//PPP functions
OSErr OTVerifyOpen(TransStream stream);
OSErr OTPPPConnect(Boolean *attemptedConnection);
OSErr GetPPPConnectionState(EndpointRef endPoint, unsigned long *PPPState);
pascal void MyPPPEndpointNotifier(void *context, OTEventCode code, OTResult result, void *cookie);
OSErr TurnOnPPPConnectionDialog(EndpointRef endPoint, Boolean on);
OSErr ResetPPPConnectionDialog(EndpointRef endPoint);
OSErr WaitForOTPPPDisconnect(Boolean showStatus);
OTResult NewPPPControlEndpoint(void);
OSErr GetCurrentUInt32Option(EndpointRef endPoint, OTXTIName theOption, UInt32 *value);
OSErr SetCurrentUInt32Option(EndpointRef endPoint, OTXTIName theOption, UInt32 value);
OSErr OTPPPDialingInformation(Boolean *redial, unsigned long *numRedials, unsigned long *delay);
Boolean SearchDirectoryForFile(CInfoPBRec *info, long dirToSearch, OSType type, OSType creator);
void UpdateCachedConnectionMethodInfo(void);

/*********************************************************************************
 * OTInitOpenTransport - initialize open transport if present.
 *********************************************************************************/
void OTInitOpenTransport(void)
{	
	//if we have OT && TCP, then try to initialize it
	if (OTIs)	
	{
		if (!HasOTInetLib())
		{
		 	gUseOT = false;
		 	
		 	// Only warn the user if we know OT works on this machine
		 	if (OTSupported()) OTTCPError(NULL,errOTInitFailed,errOTMissingLib);
		}
		else 
		{
			gUseOT = true;
		
			//Initialize if we haven't already.
			if (!OTinitialized)
			{	
				OSStatus osStatus;	
					
				if ((osStatus = InitOpenTransportInContext(kInitOTForApplicationMask,nil)) != kOTNoError)
				{
					gUseOT = false;
					OTTCPError(NULL,errOTInitFailed,osStatus);
				}
				else	//Open Transport is here.  
				{	
					//See if RemoteAcess/OT/PPP is present while we're here.
					long result;
					OSErr err = Gestalt(gestaltOpenTptRemoteAccess, &result);
					
					if ((err == noErr) 
					 && (result & (1 << gestaltOpenTptRemoteAccessPresent)) 
					 && (result & (1 << gestaltOpenTptPPPPresent)))
					{
						gHasOTPPP = true;
					}
					else 
					{
						gHasOTPPP = false;
					}

#ifdef USE_NETWORK_SETUP	
					// check for the network setup library as well ...
					UseNetworkSetup();
#endif	//UseNetworkSetup

					
					OTinitialized = true;		//remember that we started OT.
				}
			}
		}
	}
}

/*********************************************************************************
 * HasOTInetLib - (CFM Only) returns true if the OpenTptInetLib functions are present.
 *********************************************************************************/
Boolean HasOTInetLib(void)
{
	Boolean HasOTInetLib = true;
	
#if TARGET_RT_MAC_CFM
  HasOTInetLib = (long)(&OTInetGetInterfaceInfo) != kUnresolvedCFragSymbolAddress
  	&& (long)(&OTInetStringToAddress) != kUnresolvedCFragSymbolAddress
  	&& (long)(&OTInetAddressToName) != kUnresolvedCFragSymbolAddress
  	&& (long)(&OTInetMailExchange) != kUnresolvedCFragSymbolAddress; 	
#endif	//TARGET_RT_MAC_CFM

	return(HasOTInetLib);
}

/*********************************************************************************
 * OTSupported - (CFM Only) returns true if OT can be used on this 68K machine
 *********************************************************************************/
Boolean OTSupported(void)
{
	Boolean OTSupported = true;
 	long result;

#if TARGET_RT_MAC_CFM
 	//	OT can't be used if this is a 68K machine AND an OT version less than 1.2 is installed.
 	OTSupported = false;
	if (Gestalt(gestaltOpenTptVersions, &result) == noErr)
	{
		if ((result & 0xFFFF0000) >= 0x01200000) OTSupported = true;
	}
#endif	//TARGET_RT_MAC_CFM

	return(OTSupported);
}

/*********************************************************************************
 * OTCloseOpenTransport - cleanup after Open Transport has been used
 *********************************************************************************/
void OTCleanUpAfterOpenTransport(void)
{
	if (OTinitialized)
	{			
		//shut down OT/PPP if appropriate ...
		if (gHasOTPPP)
			if (MyOTPPPInfo.weConnectedPPP == true) 
				OTPPPDisconnect(false, true);
			
		CloseOpenTransportInContext(nil);
	}
}

/*********************************************************************************
 * OTTCPConnectTrans - connect to the remote host.	This version uses OT TCP.
 *********************************************************************************/
OSErr OTTCPConnectTrans(TransStream stream, UPtr serverName, long port,Boolean silently,uLong timeout)
{
	InetHostInfo		hostInfo;
	Str255				scratch;
	InetMailExchange 	MXRecords[NUM_MX];
	short				numMX = NUM_MX;
	InetDomainName		hostName;
	Boolean				useMX = PrefIsSet(PREF_IGNORE_MX) && (port == GetRLong(SMTP_PORT)) && GetOSVersion()<0x1030;
	InetHost			tryAddr;
	long				receiveBufferSize = 0;
	OSErr				err = noErr;
							
#ifdef DEBUG
	if (BUG12) port += 10000;
#endif

	ASSERT(stream);
	ASSERT(stream->OTTCPStream == 0);
	
	stream->streamErr = noErr;
	stream->Opening = true;
	stream->BeSilent = silently;
	
	//allocate memory for a new connection
	stream->OTTCPStream = New(MyOTTCPStream);
	if (stream->OTTCPStream != 0)
	{	
		WriteZero(stream->OTTCPStream,sizeof(MyOTTCPStream));
	
		ProgressMessageR(kpSubTitle,WHO_AM_I);
		//Make sure TCP is up and going.  Connect with OTPPP or something at this point.
		if ((err=OTVerifyOpen(stream)) == noErr)
		{			
			gActiveConnections++;	// a TCP connection has been established
				
			PCat(GetRString(scratch, DNR_LOOKUP), serverName);
			ProgressMessage(kpSubTitle, scratch);
			
			strncpy(hostName,serverName+1,(*serverName > kMaxHostNameLen? kMaxHostNameLen : *serverName));
			hostName[*serverName] = 0;
			
			//MX allows us to pick the best address to send mail to.
			if (useMX)
			{
				if ((err = OTGetDomainMX(hostName, MXRecords, &numMX)) != noErr) useMX = false;
				else GetPreferredMX(hostName, MXRecords, numMX);
			}
		
			if (err != userCancelled)
			{
				//Lookup the host by its name
				if ((err = OTGetHostByName(hostName, &hostInfo)) == noErr)
				{
					tryAddr = OTRandomAddr(&hostInfo);
					
					//OTGetHostByName succeeded.  Create the myStream structure, and allocate a buffer
					ProgressMessageR(kpSubTitle,HOUSEKEEPING);
					if ((err = CreateOTStream(stream)) == noErr)
					{
						// allocate a buffer for tcp, and create the stream
						receiveBufferSize = GetRLong(RCV_BUFFER_SIZE);
						if ((stream->RcvBuffer=NuHTempOK(receiveBufferSize))!=nil)
						{
							//Create stream succeeded.  Now try opening the connection.
							ComposeRString(scratch,CNXN_OPENING,serverName,tryAddr);
							ProgressMessageR(kpSubTitle,PREPARING_CONNECTION);
							ProgressMessage(kpMessage,scratch);
							
							// log the connection we're about to make
							ComposeLogS(LOG_PROTO,nil,"\pConnecting to %i:%d",tryAddr,port);
								
							if ((err = OTTCPOpen(stream, tryAddr, port, timeout)) == noErr)
							{
								//open succeeded.
								stream->RcvSpot = -1;
								ComposeLogS(LOG_PROTO,nil,"\pConnected to %i:%d",tryAddr,port);
							}
							else if (err != userCancelled) 
							{
								OTTCPError(stream,errOpenStream, stream->streamErr);
								ComposeLogS(LOG_PROTO,nil,"\pConnection to %i:%d failed.  Error %d",tryAddr,port,err);
							}
						}
						else WarnUser(MEM_ERR, stream->streamErr);	  // Memory error while creating buffer
					}
					else if (err != userCancelled) OTTCPError(stream,errCreateStream, stream->streamErr=err);
				}
				else
				{
					if (err != userCancelled) OTTCPError(stream,errDNR,stream->streamErr=err);
//					else OTTCPError(stream,errUserCancelledConnection,stream->streamErr=err);

					// Log the DNS Error
					ComposeLogS(LOG_PROTO,nil,"\pConnection failed.  DNS error %d", err);
				}
			}
		}
		else 
		{
			if (stream->streamErr == -7109) stream->streamErr = userCancelled;
			if (stream->streamErr != userCancelled) OTTCPError(stream,errPPPConnect, stream->streamErr);	
		}		
	}
	else WarnUser(OT_CON_MEM_ERR, stream->streamErr);	
		
	stream->Opening = false;	
	return ((OSErr)stream->streamErr);
}

#define	SWING_THE_THING	60
/*********************************************************************************
 * OTTCPSendTrans - send some text to the remote host.	This version uses OT TCP.
 *********************************************************************************/
OSErr OTTCPSendTrans(TransStream stream, UPtr text,long size, ...)
{
	OTResult 	result = noErr;
	va_list 	extra_buffers;
	UPtr		curBuf;
	long		curSize = 0;
	long		last = TickCount();
	long		now = 0;
	Boolean slow = False;
	
	ASSERT(stream);
	ASSERT(stream->OTTCPStream);
	
	stream->streamErr = noErr;
	
	if (CommandPeriod) return(userCancelled);
			
	if (size==0) return(noErr); 	// allow vacuous sends
	
	va_start(extra_buffers,size);

	//We'll send the text buffer first.
	curBuf = text;
	curSize = size;
	do
	{	
		//send the current buffer.
		while (curSize > 0)
		{		
			now = TickCount();
			result = OTSnd(stream->OTTCPStream->ref, curBuf, curSize, 0);
			if (result >= 0) 
			{
				//log what we sent
				if (LogLevel&LOG_TRANS && !stream->streamErr && result) CarefulLog(LOG_TRANS,LOG_SENT,curBuf,result);
				CycleBalls();
				curSize -= result;
				curBuf += result;
				last = now;
				if (IsSendAudit(stream)) stream->bytesTransferred += result;
			} 
			else 		//no data was sent
			{
				if (result != kOTFlowErr) 		//something happened to the connection.  Stop the transfer
				{
					if (stream->OTTCPStream->otherSideClosed)
					{
						stream->streamErr = result = errLostConnection;
					}
					else stream->streamErr = errMiscSend;
						
					break;
				}
				else													//just some flow control issue or something.  Swing the thing, and check for a command-.
				{
					// check to see if we need ppp, but the connection is down
					if (needPPPConnection && MyOTPPPInfo.state != kPPPUp) return(stream->streamErr = result = errLostConnection);
				}
			}
		
			if (slow || (now - last) > SWING_THE_THING)
			{
				if (slow) YieldTicks = 0;
				MiniEvents();
				if (CommandPeriod) return(stream->streamErr = userCancelled);
				if ((now - last) > SWING_THE_THING)
				{
					slow = True;
					CyclePendulum();
					last = now - SWING_THE_THING/2;
				}
			}
		}

		//point to the next buffer to send
		if (!stream->streamErr)
		{
			curBuf = va_arg(extra_buffers,UPtr);
			curSize = va_arg(extra_buffers,long);
		}
		
	} while (!stream->streamErr && curBuf);
	
	va_end(extra_buffers);
	
	if (stream->streamErr)
	{
		if (stream->streamErr!=commandTimeout && stream->streamErr!=userCancelled)
			OTTCPError(stream,stream->streamErr,result);
	}
		
	return (stream->streamErr);
}

/*********************************************************************************
 * OTTCPRecvTrans - get some text from the remote host.  This version uses OT TCP.
 *********************************************************************************/
OSErr OTTCPRecvTrans(TransStream stream, UPtr line,long *size)
{
	OTResult result = noErr;
	Str31 scratch;
	long timeout = InAThread() ? GetRLong(THREAD_RECV_TIMEOUT) : GetRLong(RECV_TIMEOUT);
	
	ASSERT(stream);
	ASSERT(stream->OTTCPStream);
	
	stream->streamErr = noErr;
	
	// Spin until something comes in.
	do
	{
		// read characters from the network, or die trying.
		stream->streamErr = OTWaitForChars(stream, timeout, line, size, &result);
		if (stream->streamErr == noErr)
		{				
			if (result >= 0)								// got something
				*size = result;
			else if (stream->OTTCPStream->otherSideClosed)	// lost connection
				stream->streamErr = errLostConnection;
			else if (result == kOTNoDataErr)				// got nothing
				*size = 0; 
			else 											// got some other error
				stream->streamErr = errMiscRec;
		}
	}
	while (stream->streamErr == commandTimeout &&
				 AlertStr(TIMEOUT_ALRT,Caution,GetRString(scratch,InAThread() ? THREAD_RECV_TIMEOUT : RECV_TIMEOUT))==1);
	
	if (stream->streamErr)
	{
		if (stream->streamErr!=commandTimeout && stream->streamErr!=userCancelled)
			OTTCPError(stream,stream->streamErr,(result < 0 ? result : kOTNoDataErr));
			// Note: result could be 0 if OTWaitForChars fails.  We want a real error message displayed in that case, too.
	}
	else 
	{
		if (*size) ResetAlertStage();
		if (*size && LogLevel&LOG_TRANS) CarefulLog(LOG_TRANS,LOG_GOT,line,*size);	//log what we got ...
		if (*size && IsRecAudit(stream)) stream->bytesTransferred += *size;
	}
			
	return (stream->streamErr);
}

/*********************************************************************************
 * TCPDisTrans - disconnect from the remote host.  This version uses OT TCP.
 *********************************************************************************/
OSErr OTTCPDisTrans(TransStream stream)
{
	if (stream && stream->OTTCPStream)
	{	
		stream->streamErr = noErr;
		
		//We're going to do an orderly disconnect on this stream.
		stream->OTTCPStream->weAreClosing = true;
			
		if ((stream->OTTCPStream->ref) && (!stream->OTTCPStream->ourSideClosed))
		{
			//close our end if the other side has aborted.
			if (stream->OTTCPStream->otherSideClosed) 
			{
				OTSndDisconnect(stream->OTTCPStream->ref, nil);
				stream->OTTCPStream->ourSideClosed = true;
				stream->OTTCPStream->releaseMe = true;				//we can destroy this connection completely.
			}
			else
			{
				if (OTSndOrderlyDisconnect(stream->OTTCPStream->ref)!=noErr) stream->OTTCPStream->releaseMe = true;
				stream->OTTCPStream->ourSideClosed = true;
				stream->OTTCPStream->age = TickCount();
			}
		}
		
		return (stream->streamErr);
	}
	else return (noErr);
}

/*********************************************************************************
 * OTTCPDestroyTrans - destroy the connection.  This version uses OT TCP
 *********************************************************************************/
OSErr OTTCPDestroyTrans(TransStream stream)
{
	OSStatus status = noErr;
		
	if ((stream == 0) || (stream->OTTCPStream == 0)) return (noErr);
	
	//We are destroying the stream without disconnecting it.  An error occured, or the the user cancelled.
	//Do an abortive disconnect.
	if (!stream->OTTCPStream->weAreClosing)
	{	
		DestroyMyTStream(stream->OTTCPStream);
		ZapPtr(stream->OTTCPStream);
	}
	else	//otherwise, queue up the stream for an orderly disconnect.
	{
		if (stream->OTTCPStream)
		{	
 			//if both sides are closed, throw out this connection immediately
 			if (stream->OTTCPStream->ourSideClosed && stream->OTTCPStream->otherSideClosed) stream->OTTCPStream->releaseMe = true;
 			
 			if (!stream->OTTCPStream->releaseMe) 			
			{
				//Queue it up in our queue of MyTStreams waiting to hear an orderly disconnect
				EnqueueMyTStream(stream->OTTCPStream);
				stream->OTTCPStream = 0;
			}
			else
			{
				//an error occurred disconnecting our stream.  Just kill it.
				DestroyMyTStream(stream->OTTCPStream);
				ZapPtr(stream->OTTCPStream);
			}
		}
	}
	
	//dispose our receive buffer.
	if (stream->RcvBuffer) ZapHandle(stream->RcvBuffer);
	
	return (stream->streamErr);
}
		
		
/*********************************************************************************
 * OTTCPTransError - report our most recent error
 *********************************************************************************/
OSErr OTTCPTransError(TransStream stream)
{
	ASSERT(stream);
	ASSERT(stream->OTTCPStream);
	
	return (stream->streamErr);
}

/*********************************************************************************
 * OTTCPSilenceTrans - turn off error reports from ot tcp routines
 *********************************************************************************/
void  OTTCPSilenceTrans(TransStream stream, Boolean silence)
{
	ASSERT(stream);
	
	stream->BeSilent = silence;
}

/********************************************************************************
 * OTTCPWhoAmI - return the mac's tcp name.  This version uses OT TCP.
 ********************************************************************************/
UPtr  OTTCPWhoAmI(TransStream stream, Uptr who)
{
#pragma unused(stream)

	InetInterfaceInfo localInfo;
	MyOTInetSvcInfo	myOTInfo;
	short	len;
	OSErr whoAmIErr = noErr;
	
	if (!*MyHostname)
	{
		whoAmIErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
		if (whoAmIErr == kOTNotFoundErr)	//OT hasn't been loaded yet ...
		{
			if (whoAmIErr = OpenOTInternetServices(&myOTInfo)) 
				return (who);
			whoAmIErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
			OTCloseProvider(myOTInfo.ref);
			myOTInfo.ref = 0;
		}
		if (whoAmIErr != noErr) 
			return (who);
		ComposeRString(who,TCP_ME,localInfo.fAddress);
		SetPref(PREF_LASTHOST,who);
		PCopy(MyHostname,who);
	}
	PCopy(who,MyHostname);
	
	//remove the '.' from the end of who, if there is one.
	len = strlen(who);
	if (who[len-1]=='.') who[len-1]=0;	
	
	return(who);
}

/********************************************************************************
 * DNSHostid - return the mac's dns server hostid.  This version uses OT TCP.
 ********************************************************************************/
OSErr  DNSHostid(uLong *dnsAddr)
{
	InetInterfaceInfo localInfo;
	MyOTInetSvcInfo	myOTInfo;
	OSErr dnsHostidErr = noErr;

	dnsHostidErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
	if (dnsHostidErr == kOTNotFoundErr)	//OT hasn't been loaded yet ...
	{
		if (!(dnsHostidErr = OpenOTInternetServices(&myOTInfo)))
		{ 
			dnsHostidErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
			OTCloseProvider(myOTInfo.ref);
			myOTInfo.ref = 0;
		}
	}

	if (!dnsHostidErr) *dnsAddr = *(long*)&localInfo.fDNSAddr;

	return(dnsHostidErr);
}

/********************************************************************************
 * OTMyHostid - return the mac's hostid.  This version uses OT TCP.
 ********************************************************************************/
OSErr  OTMyHostid(uLong *myAddr,uLong *myMask)
{
	InetInterfaceInfo localInfo;
	MyOTInetSvcInfo	myOTInfo;
	OSErr myHostidErr=noErr;
	
	myHostidErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
	if (myHostidErr == kOTNotFoundErr)	//OT hasn't been loaded yet ...
	{
		if (!(myHostidErr = OpenOTInternetServices(&myOTInfo)))
		{ 
			myHostidErr = OTInetGetInterfaceInfo(&localInfo, kDefaultInetInterface);
			OTCloseProvider(myOTInfo.ref);
			myOTInfo.ref = 0;
		}
	}

	if (!myHostidErr)
	{
		*myAddr = *(long*)&localInfo.fAddress;
		*myMask = *(long*)&localInfo.fNetmask;
	}
	
	return(myHostidErr);
}

/********************************************************************************
 * OTMyHostid - return the mac's hostid.  This version uses OT TCP.
 ********************************************************************************/
OSErr GetMyHostid(uLong *addr,uLong *mask)
{
		return(OTMyHostid(addr,mask));
}

/************************************************************************
 * OTGetHostByName - get host information, given a hostname
 * this routine maintains a small, unflushable cache.
 ************************************************************************/
OSErr OTGetHostByName(InetDomainName hostName, InetHostInfo *hostInfoPtr)
{
	MyOTInetSvcInfo	myOTInfo;
	OSErr nameErr = noErr;
				
	if (nameErr = OpenOTInternetServices(&myOTInfo)) 
		return (nameErr);
	myOTInfo.status = inProgress;
	nameErr = OTInetStringToAddress(myOTInfo.ref,hostName,hostInfoPtr);
	if (nameErr == noErr) nameErr = SpinOnWithConnectionCheck(&(myOTInfo.status),0,True,False);
	if (nameErr == noErr) nameErr = myOTInfo.result; 
	OTCloseProvider(myOTInfo.ref);
	myOTInfo.ref = 0;	
	//return without changing the name of the server we wish to look up.
	if (nameErr == noErr) strcpy(hostInfoPtr->name,hostName);	
	
	// log the result of the name lookup. Be careful not to overrun buffers.
	if (LogLevel&LOG_PROTO)
	{
		short count;
		Str63 logHostName;
		
		MakePStr(logHostName,hostName,strlen(hostName));
		ComposeLogS(LOG_PROTO,nil,"\pDNS Lookup of \"%p\"",logHostName);
		if (nameErr == noErr)
		{
			for (count=0;count<kMaxHostAddrs;count++)
				if (hostInfoPtr->addrs[count])
					ComposeLogS(LOG_PROTO,nil,"\p    %i (%d)",hostInfoPtr->addrs[count],count+1);
		}
		else ComposeLogS(LOG_PROTO,nil,"\pLookup failed (error %d)",nameErr);
	}
			
	return (nameErr);
}

/*********************************************************************************
 * OTGetHostByAddr - get host information, given an address.  This version uses OT
 * this routine maintains a small, unflushable cache.
 *********************************************************************************/
OSErr OTGetHostByAddr(InetHost addr, InetHostInfo *domainNamePtr)
{
	MyOTInetSvcInfo	myOTInfo;
	short len = 0;
	OSErr addrErr = noErr;
				
	domainNamePtr->addrs[0] = addr;	
	if (addrErr = OpenOTInternetServices(&myOTInfo)) 
		return (addrErr);
	myOTInfo.status = inProgress;
	addrErr = OTInetAddressToName(myOTInfo.ref,domainNamePtr->addrs[0],domainNamePtr->name);
	if (addrErr == noErr) addrErr = SpinOnWithConnectionCheck(&(myOTInfo.status),0,True,False);
	if (addrErr == noErr) addrErr = myOTInfo.result; 
	OTCloseProvider(myOTInfo.ref);
	myOTInfo.ref = 0;
	
	//remove the '.' from the end of name, if there is one.
	if (addrErr == noErr)
	{	
		len = strlen(domainNamePtr->name);
		if (domainNamePtr->name[len-1]=='.') domainNamePtr->name[len-1]=0;	
	}		
	
	return (addrErr);
}

/**********************************************************************
 * OTRandomAddr - pick a random address out of a set of addresses we
 * got from our OT DNR lookip.
 **********************************************************************/
InetHost OTRandomAddr(InetHostInfo *host)
{
	short count;
	
	if (!PrefIsSet(PREF_DNS_BALANCE)) 
		return (host->addrs[0]);
	
	for (count=kMaxHostAddrs;count;count--) if (host->addrs[count-1]) break;
	return (host->addrs[count<2?0:TickCount()%count]);
}

/************************************************************************
 * OTGetDomainMX - Get the mail exchange info for a particular host.
 ************************************************************************/
OSErr OTGetDomainMX(InetDomainName hostName, InetMailExchange *MXPtr, short *numMX)
{
	OSErr err = noErr;
	MyOTInetSvcInfo	myOTInfo;
	short i;
	
	if (MXPtr == 0 		// must have a place to store the MX records
	 || numMX == 0 		// must have an idea of how many records will fit there
	 || *numMX <= 0) 	// and must have allocated room for at least one
		return (paramErr);
	
	//clear out the MXPtr
	for (i = 0; i < *numMX; i++) 
		(MXPtr[i]).exchange[0] = (MXPtr[i]).preference = 0;
	
	//do the MX lookup
	if ((err = OpenOTInternetServices(&myOTInfo)) == noErr)
	{
		myOTInfo.status = inProgress;
		err = OTInetMailExchange(myOTInfo.ref, hostName, numMX, MXPtr);
		if (err == noErr) err = SpinOnWithConnectionCheck(&(myOTInfo.status),0,True,False);
		OTCloseProvider(myOTInfo.ref);
		myOTInfo.ref = 0;
		
		if (err == noErr)
			if (MXPtr->exchange[0] == 0) return (errNoMXRecords);			
	}
	
	return (err);
}

/**********************************************************************
 * GetPreferredMX - return the preferred host to send mail to.
 **********************************************************************/
void GetPreferredMX(InetDomainName preferredName, InetMailExchange *MXPtr, short numMX)
{
	short lowestPref, preferredHost;
	short count = numMX - 1;
	short len;
	
	preferredHost = count;
	lowestPref = MXPtr[count].preference;
	for (count; count >= 0; count--)
	{
		if (MXPtr[count].preference < lowestPref) 
		{
			lowestPref = MXPtr[count].preference;
			preferredHost = count;
		}
	}
	strcpy(preferredName, MXPtr[preferredHost].exchange);
	
	//remove the '.' from the end of preferredName, if there is one.
	len = strlen(preferredName);
	if (preferredName[len-1]=='.') preferredName[len-1]=0;	
}

#pragma segment Main
/*********************************************************************************
 * MyOTNotifyProc - This callback handles the results of all asynch OT calls.
 *********************************************************************************/
pascal void MyOTNotifyProc(MyOTInetSvcInfo *info, OTEventCode theEvent, OTResult theResult, void *theParam)
{
	switch (theEvent)
	{
		case T_OPENCOMPLETE:							// The OTAsyncOpenInternetServices function has completed
		case T_DNRSTRINGTOADDRCOMPLETE:		// The OTInetStringToAddress function has finished	
		case T_DNRADDRTONAMECOMPLETE:			// The OTInetAddressToName function has finished
		case T_DNRMAILEXCHANGECOMPLETE:		// The OTInetMailExchange function has finished
			info->status = 0;
			info->result = theResult;
			info->cookie = theParam;
			break;
				
		default:													// All other network events can be ignored
			break;
	}
}
#pragma segment TcpTrans

/*********************************************************************************
 * OpenOTInternetServices - Open up the internet service provider OT gives us.
 *********************************************************************************/
OSErr OpenOTInternetServices(MyOTInetSvcInfo *myOTInfo)
{
	OSErr err = noErr;
	DECLARE_UPP(MyOTNotifyProc,OTNotify);
	
	INIT_UPP(MyOTNotifyProc,OTNotify);
	myOTInfo->status = inProgress;
	if ((err = OTAsyncOpenInternetServicesInContext(kDefaultInternetServicesPath, 0, MyOTNotifyProcUPP, myOTInfo,NULL)) == noErr) 
		if ((err = SpinOnWithConnectionCheck(&(myOTInfo->status),0,True,False)) == noErr) 
		{
			myOTInfo->ref = myOTInfo->cookie;
		}
	
	if (err != noErr && err != userCancelled) err = errOTInetSvcs;
			
	return (err);
}

#pragma segment Main
/*********************************************************************************
 *	MyOTStreamNotifyProc - my OT notifier proc for TCP streams.
 *********************************************************************************/
pascal void MyOTStreamNotifyProc (MyOTTCPStream *myStream, OTEventCode code, OTResult theResult, void *cookie)
{
	OSStatus err;
	
	switch (code) 
	{
		case T_DISCONNECT:								// Other side has aborted
			myStream->otherSideClosed = true;
			myStream->status = 0;
			break;
			
		case T_ORDREL:									// Other side has closed in an orderly fashion
			myStream->otherSideClosed = true;
			myStream->status = 0;
			err = OTRcvOrderlyDisconnect(myStream->ref);
			if (!myStream->ourSideClosed)
			{
				err = OTSndOrderlyDisconnect(myStream->ref);
				myStream->ourSideClosed = true;
			}
			myStream->releaseMe = true;					// we don't need this stream anymore.
			break;
			
		case T_OPENCOMPLETE:							// OTOpenAsyncEndpoint has finished
		case T_BINDCOMPLETE:							// OTBind has finished
		case T_UNBINDCOMPLETE:							// OTUnbind has finished
		case T_CONNECT:									// OTConnect has finished
		case T_PASSCON:									// state is now T_DATAXFER
			myStream->status = 0;
			myStream->code = code;
			myStream->result = theResult;
			myStream->cookie = cookie;
			break;
	}
}
#pragma segment TcpTrans

/*********************************************************************************
 *	CreateOTStream - create a MyOTTCPStreamPtr
 *********************************************************************************/
OSErr CreateOTStream(TransStream stream)
{
	OSStatus	OTErr = noErr;
	DECLARE_UPP(MyOTStreamNotifyProc,OTNotify);
	
	INIT_UPP(MyOTStreamNotifyProc,OTNotify);
	ASSERT(stream);
	
	stream->streamErr = noErr;
	
	//Open a TCP endpoint asynchronously
	stream->OTTCPStream->status = inProgress;
	stream->streamErr = OTAsyncOpenEndpointInContext(OTCreateConfiguration(kTCPName),0,0,MyOTStreamNotifyProcUPP,stream->OTTCPStream,nil);
	if (stream->streamErr == noErr) stream->streamErr = SpinOnWithConnectionCheck(&(stream->OTTCPStream->status),0,True,False);
	if ((stream->streamErr == noErr) || ((stream->streamErr = stream->OTTCPStream->result) == noErr)) 
	{
		if (stream->OTTCPStream->code != T_OPENCOMPLETE) 
			return (stream->streamErr = errOpenStream);
		stream->OTTCPStream->ref = stream->OTTCPStream->cookie;
		if (stream->OTTCPStream->ref == kOTInvalidEndpointRef)
			return (stream->streamErr = errOpenStream);

		//Initialize the MyOTTCPStreamPtr flags
		stream->OTTCPStream->weAreClosing = false;
		stream->OTTCPStream->otherSideClosed = false;
		stream->OTTCPStream->ourSideClosed = false;
		stream->OTTCPStream->releaseMe = false;
	}

	if (stream->streamErr != noErr)
	{
		if (stream->OTTCPStream->ref) OTCloseProvider(stream->OTTCPStream->ref);
		stream->OTTCPStream->ref = 0;
	}
	return (stream->streamErr);
} 

/*********************************************************************************
 *	OTTCPOpen - actually open a connection
 *	
 *	3/18/99 no longer using OT memory allocation routines.
 *********************************************************************************/
OSErr OTTCPOpen(TransStream stream, InetHost tryAddr, InetPort port,uLong timeout)
{
	InetAddress connectAddr;
	TCall sndCall;
	
	ASSERT(stream);
	ASSERT(stream->OTTCPStream);
	
	stream->streamErr = noErr;
	stream->OTTCPStream->status = inProgress;
	if (stream->streamErr = OTBind(stream->OTTCPStream->ref, 0, 0)) 	// OTBind doesn't need any parameters.  This is an outgoing connection.  jdboyd 04/11/02
		return (stream->streamErr);
	if (stream->streamErr = SpinOnWithConnectionCheck(&(stream->OTTCPStream->status),60*timeout,True,False)) 
		return (stream->streamErr);
	if (stream->OTTCPStream->code != T_BINDCOMPLETE) 
	{
		stream->streamErr = stream->OTTCPStream->result;
		return (errOpenStream);
	}
	
	// set up the TCall structure needed to connect to tryAddr		
	OTInitInetAddress(&connectAddr, port, tryAddr);
	WriteZero(&sndCall, sizeof(TCall));
	sndCall.addr.len = sizeof(InetAddress);				
	sndCall.addr.buf = (unsigned char*) &connectAddr;
	
	// now connect to the address asynchronously
	stream->OTTCPStream->status = inProgress;	
	stream->streamErr = OTConnect(stream->OTTCPStream->ref, &sndCall, 0);
	if (stream->streamErr != noErr && stream->streamErr != kOTNoDataErr) 
		return (stream->OTTCPStream->otherSideClosed ? errLostConnection : stream->streamErr);
	if (stream->streamErr = SpinOnWithConnectionCheck(&(stream->OTTCPStream->status),60*timeout,True,False))
		return (stream->OTTCPStream->otherSideClosed ? errLostConnection : stream->streamErr);
	
	// did we connect?
	if (stream->OTTCPStream->code != T_CONNECT) 		
	{
		stream->streamErr = errLostConnection;
		return (errOpenStream);
	}
	if (stream->streamErr = OTRcvConnect(stream->OTTCPStream->ref, 0)) 
		return (stream->streamErr);
	
	return (stream->streamErr);
}

/*********************************************************************************
 * OTWaitForChars - spin, giving everybody else time, until chars available
 *********************************************************************************/
short OTWaitForChars(TransStream stream, long timeout, UPtr line, long *size, OTResult *otResult)
{
	EventRecord event;
	long ticks=TickCount();
	static long waitTicks=0;
	long tookTicks;
	Boolean result = false;
	long timeoutTicks = ticks + 60*timeout;
	Boolean slow = False;
	OTFlags junkFlags = 0;
	
	ASSERT(stream);
	ASSERT(stream->OTTCPStream);
	
	if (!InBG) waitTicks = 0;
	
	// spin until we see data on the network ...
	do
	{
		// check to see if our connection is still up.  Only care if remote end has closed.
		if (stream->OTTCPStream->otherSideClosed) return(errLostConnection);			

		// check to see if we need ppp, but the connection is down
		if (needPPPConnection && MyOTPPPInfo.state != kPPPUp) return(errLostConnection);
		
		//check to see if cmd-. has been pressed.
		if (CommandPeriod) return (userCancelled);
									
		if (TickCount()-ticks  > 10)
		{
			slow = True;
			CyclePendulum();
			ticks=TickCount();
			if (ticks >timeoutTicks) return(commandTimeout);
		}

		// receive size bytes and put it in line.  We expect some data, so wait around until we see it.
		*otResult = OTRcv(stream->OTTCPStream->ref, line, *size, &junkFlags);
		
		// we'll spin until we see data or an error.
		if (*otResult == kOTNoDataErr) *otResult = 0;
		
		if (slow) YieldTicks = 0;
		
		//	To speed up threaded xfers when app is in bg, don't call WaitNextEvent as often
		// also speeded up typing by changing NEED_YIELD -- it checks Typing when in a thread. (though I'm not quite sure how thread gets time when Typing is true)
		if (NEED_YIELD || ((*otResult == 0) && !stream->DontWait))
		{	
			tookTicks = TickCount();

// 11-13-97 change to fix cmd period bug when in main thread
			result = WNE(MINI_MASK|keyDownMask,&event,waitTicks);
			if (CommandPeriod) return(userCancelled);
//was:
//		result = WNE(MINI_MASK,&event,waitTicks);
			
			tookTicks = TickCount()-tookTicks;
			if (InBG)
				if (tookTicks > waitTicks+1)
					waitTicks = MIN(120,tookTicks);
				else
					waitTicks = waitTicks>>1;
			else
				waitTicks = 1;
			if (result) (void) MiniMainLoop(&event);
		}
	}
	while ((*otResult == 0) && !stream->DontWait);
	stream->CharsAvail = 0;
	return(0);
}


/*********************************************************************************
 *	OTFlushInput - dump all chars that arrive on a stream, until there are no
 *   chars for a given period of time
 *********************************************************************************/
void OTFlushInput(TransStream stream,uLong timeout)
{
	Str255 junk;
	OTResult result;
	long got;

	do
	{
		got = sizeof(junk);
		if (OTWaitForChars(stream, timeout, junk, &got, &result)) break;
		if (LogLevel&LOG_TRANS && !stream->streamErr && result>0) CarefulLog(LOG_TRANS,LOG_FLUSHED,junk,result);
	}
	while (result>0);
}

static short errorMessages[errMyLastOTErr-errOTInitFailed] = 
	{ 
		OT_INIT_ERR,	//errOTInitFailed,
		OT_INIT_ERR,	//errOTInetSvcs, 
		BIND_ERR,	//errDNR,
		0,	//errNoMXRecords,
		TCP_TROUBLE,	//errCreateStream,
		NO_SMTP_SERVER,	//errOpenStream,
		TCP_TROUBLE,	//errLostConnection,
		TCP_TROUBLE,	//errMiscRec,
		TCP_TROUBLE,	//errMiscSend
		OT_DIALUP_CONNECT_ERR,//errPPPConnect
		0,	//errPPPPrefNotFound
		0,	//errPPPStateUnknown
		OT_MISSING_LIBRARY	//errOTMissingLib:
	};



static void OTTELo ( OSErr generalError, OSErr specificError, StringPtr message ) {
	if (IsMyOTError(generalError))
		GetRString(message, errorMessages[generalError - errOTInitFailed]);
	else
		GetRString(message, TCP_TROUBLE);
	}

/*********************************************************************************
 *	OTTE - give the user some helpful information if an error occurs.
 *		generalError contains a general description of the error, set by me.
 *		specificError contains the actual error returned by the failed call
 *********************************************************************************/
OSErr OTTE(TransStream stream, OSErr generalError, OSErr specificError, short file, short line)
{
	short	errorString = 0;
	OSErr	MacTCPErr = noErr;
	
	if ((generalError != noErr)
		&& (stream==0 || !stream->BeSilent) 
		&& !AmQuitting
		&& (!CommandPeriod || stream==0 || stream->Opening && !PrefIsSet(PREF_OFFLINE) && !PrefIsSet(PREF_NO_OFF_OFFER)))
	{
		Str255 message;
		Str255 tcpMessage;
		Str63 debugStr;
		Str31 rawNumber;
		short realSettingsRef = SettingsRefN;

		tcpMessage[0] = 0;
		
		NumToString(specificError,rawNumber);
		
		SettingsRefN = GetMainGlobalSettingsRefN();		
		OTErrorToString(specificError, tcpMessage);
		OTTELo ( generalError, specificError, message );
		ComposeRString(debugStr,FILE_LINE_FMT,file,line);
		SettingsRefN = realSettingsRef;

		
		MyParamText(message,rawNumber,tcpMessage,debugStr);
		if (stream==0 || stream->Opening)
		{
			if (2==ReallyDoAnAlert(OPEN_ERR_ALRT,Caution))
				SetPref(PREF_OFFLINE,YesStr);
		}
		else ReallyDoAnAlert(BIG_OK_ALRT,Caution);

	}
		
	return (generalError);
}

/*********************************************************************************
 *	OTErrorToString - pick an error string that best describes the
 *	specific error.
 *********************************************************************************/
void OTErrorToString(short specificError, Str255 tcpMessage)
{	
	short errorString = 0;
	tcpMessage[0] = 0;

	//Was this some error that I defined?
	if (IsMyOTError(specificError))
	{
		switch (specificError)
		{
			case errOTInetSvcs:
				errorString = OT_INET_SVCS_ERR;
				break;
			
			case errLostConnection:
				specificError = TCPRemoteAbort;
				break;
			
			case errPPPStateUnknown:
				errorString = OT_PPP_STATE_ERR;
				break;
					
			case errPPPPrefNotFound:
				errorString = OT_TCPIP_PREF_ERR;
				break;
			
			case errOTMissingLib:
				errorString = OT_MISSING_LIBRARY;
				break;
				
			default:
				errorString = OT_UNKNOWN_ERR;
				break;
		}
	}
	
	if (errorString!=0) GetRString(tcpMessage,errorString);
	else
	{	
		if (IsXTIError(specificError))
			GetRString(tcpMessage,OTTCP_ERR_STRN + XTI2OSStatus(specificError));
		else if (IsOTPPPError(specificError))							// was this an OT/PPP related error?
			GetRString(tcpMessage,OTPPP_ERR_STRN - specificError + kCCLErrorBaseCode - 1);
		else if (-23000>=specificError && specificError >=-23048)		// was this a MacTCP error code?
			GetRString(tcpMessage,MACTCP_ERR_STRN-22999-specificError);	
		else if (2<=specificError && specificError<=9)
			GetRString(tcpMessage,MACTCP_ERR_STRN+specificError+(23048-23000));
		else
			tcpMessage = 0;
	}
}


/*********************************************************************************
 * DestroyMyTStream - deallocate everything a MyStream grabs for itself.
 *********************************************************************************/
void DestroyMyTStream(MyOTTCPStreamPtr myStream)
{
	OSErr destroyErr = noErr;
	
	if (myStream)
	{		
		//clean up after the myStream
		if (myStream->ref != 0)
		{
			//make sure the connection is closed.
			if (!myStream->otherSideClosed || !myStream->ourSideClosed) OTSndDisconnect(myStream->ref, nil);
			
			//unbind the endpoint
			myStream->status = inProgress;
			destroyErr = OTUnbind(myStream->ref);
			if (destroyErr == noErr) destroyErr = SpinOnWithConnectionCheck(&(myStream->status),0,True,False);
	
			// remove the endpoint's notifier.  We won't be needing it anymore.
			OTRemoveNotifier(myStream->ref);
			
			// now we can kill the endpoint.
			destroyErr = OTCloseProvider(myStream->ref);	
			myStream->ref = 0;
		}
		
		if (gActiveConnections) gActiveConnections--;
	}
}

/*********************************************************************************
 * EnqueueMyTStream - put myStream in the queue of streams waiting to close.
 *********************************************************************************/
void EnqueueMyTStream(MyOTTCPStreamPtr myStream)
{
	MyOTTCPStreamPtr queueScan = 0;
	OSStatus status = noErr;
	
	ASSERT(myStream);
	
	if (pendingCloses == 0)		//this is the only stream in the queue
	{
		pendingCloses = myStream;
		myStream->next = 0;
		myStream->prev = 0;
	}	
	else						//there are some other streams waiting to close
	{
		queueScan = pendingCloses;
		while (queueScan->next != 0) queueScan = queueScan->next;
		queueScan->next = myStream;
		myStream->prev = queueScan;
		myStream->next = 0;
	}
}

/*********************************************************************************
 * KillDeadMyTStreams - deallocate memory and TStreams that have received an
 * orderly disconnect.  destroy determines whether to kill all the streams, or
 * only the ones that have been disconnected in an orderly fashion.
 *********************************************************************************/
 void KillDeadMyTStreams(Boolean destroy)
 {
 	MyOTTCPStreamPtr queueScan = pendingCloses;
 	MyOTTCPStreamPtr releaseThisOne = 0;
 	
 	//loop through and find stream that can be released.
 	while (queueScan != 0)
 	{
 		OSStatus queueScanStatus;
 		
 		if (queueScan->ref == 0) queueScan->releaseMe = true;	//this should never happen ...
 		else
 		{
	 		queueScanStatus = OTLook(queueScan->ref);
	 		
	 		//if this endpoint has some data waiting to be read, read it.
			if (queueScanStatus == T_DATA || queueScanStatus == T_EXDATA)	
			{
				do {
					queueScanStatus = OTRcv(queueScan->ref, queueScan->dummyBuffer, sizeof(queueScan->dummyBuffer), nil);
				} while (queueScanStatus >= 0);
				
				queueScan->age = TickCount();	//this stream made a noise, earning it a new minute.
			}
				
			if ((TickCount() - queueScan->age) > 3600) 
				queueScan->releaseMe = true;	//keep silent connections around for 1 minute.
		}
 		
 		//the MyOTStream callback will set releaseMe once this stream can die quietly
 		if (destroy || queueScan->releaseMe)
 		{	
 			releaseThisOne = queueScan;
 			queueScan = queueScan->next;
 			
 			if (releaseThisOne == pendingCloses) pendingCloses = releaseThisOne->next;
 			
 			if (releaseThisOne->prev) (releaseThisOne->prev)->next = releaseThisOne->next;
 			if (releaseThisOne->next) (releaseThisOne->next)->prev = releaseThisOne->prev;
 			DestroyMyTStream(releaseThisOne);
 			ZapPtr(releaseThisOne);
 		}
 		else
 			queueScan = queueScan->next;
 	}
}
 
 
/*********************************************************************************
 * OTVerifyOpen - Make sure OT TCP is ready to go.  May have to conenct with PPP
 * or SLIP, depending on what is selected in the TCP/IP control panel
 *********************************************************************************/
OSErr OTVerifyOpen(TransStream stream)
{
	Boolean weAttemptedPPP = false;
	
	stream->streamErr = noErr;

	//signal everyone else that, no, PPP is not needed for the connection	
	needPPPConnection = false;
	gPPPConnectFailed = false;
		
	if (!PrefIsSet(PREF_IGNORE_PPP))	//this pref lets us ignore MacSLIP/PPP if we want
	{	
		if ((stream->streamErr = SelectedConnectionMode(&connectionSelection, true)) == noErr)
		{
			if (dialingThePhone)
			{
				//sit and wait for a connection.
				if (connectionSelection == kPPPSelected)
				{
					ProgressMessageR(kpSubTitle,OT_PPP_WAIT);
					do
					{
						MiniEvents();
						if (CommandPeriod) 	return (stream->streamErr = userCancelled);
					}
					while (dialingThePhone);
					if (PPPDown()) return(stream->streamErr = errPPPConnect);
				}
			}
			else if (connectionSelection == kPPPSelected)
			{
				dialingThePhone = true;
				needPPPConnection = true;	//let everyone know the connection is being made over PPP.
				gConnecting = true;			//likewise, tell the world we're dialing the phone.
				if (stream->streamErr = OTPPPConnect(&weAttemptedPPP))
				{
					gPPPConnectFailed = true;	//flag to tell the world that our PPP connection attempt failed.
					
					// Holy crap, Batman! This has been broken all these years.  Only force clase the connection
					// if we were the fools to think we could connect it to begin with. -JDB 12/16/98
					OTPPPDisconnect(weAttemptedPPP, true);
				}
				gConnecting = false;
				dialingThePhone = false;
			}
		}
	}
	return (stream->streamErr);
}

/*********************************************************************************
 * OTPPPConnect - Cause an OTPPP connection to happen.  I will create one single
 * OTPPP control endpoint, and keep it around.  Apple says there's a bug with the
 * current OT/PPP that causes OTCloseProvider() to crash when closing a PPP control
 * endpoint.
 *
 *	6/18/97	This routine can be called from multiple threads.  The first call
 *		will initiate the PPP connection.  Subsequent calls will wait for the PPP
 *		connection.
 *
 *	12/16/98 Added attemotedConnection paramter to let caller know whether we make
 *		the connection attempt or not.  There might be one underway, in which case
 *		we wait for it.
 *
 *	May, 1999 Added a delay after (a) the connection is made, or (b) the connection
 *		we were waiting for finishes.  We delat for <x-eudora-setting:13102> seconds
 *		to allow ARA to actually connect.
 *********************************************************************************/ 
OSErr OTPPPConnect(Boolean *attemptedConnection)
{					
	OSErr err = noErr;
	OTResult result = kOTNoError;
	unsigned long PPPState = 0;
	Boolean PPPInForeGround = PrefIsSet(PREF_PPP_FOREGROUND);
	Str255 scratch;
	Boolean redial = false;
	long numRedials = 0;
	long delay = 0;
	long dialCount = 0;
	Boolean doDelay = false;	// do we need to delay for stupid ARA?
	
#ifdef	DEBUG
	ASSERT(attemptedConnection);	// must have this parameter
#endif
					
	pppErr = noErr;
	*attemptedConnection = false;
					
	//Make a new PPP endpoint.  We're only going to keep one around.
	if (MyOTPPPInfo.ref == 0) result = NewPPPControlEndpoint();
	
	if ((result == kOTNoError) && (MyOTPPPInfo.ref != kOTInvalidEndpointRef))
	{	
		// Check the current state of the PPP connection.  
		err = GetPPPConnectionState(MyOTPPPInfo.ref, &PPPState);
		if (err == noErr)
		{		
			// have we already connected PPP at some point in the past?
			if (MyOTPPPInfo.weConnectedPPP==true && MyOTPPPInfo.state==kPPPUp && PPPState==kPPPStateOpened) return noErr;
			
			//If we don't already have an open connection, open one.
			if ((PPPState != kPPPStateOpened && PPPState != kPPPStateOpening) || MyOTPPPInfo.state == kPPPClosing)
			{
				// remember that we're the one starting this connection
				*attemptedConnection = true;
				
				// we'll have to delay for ARA
				doDelay = true;
				
				// Get the redial information
				if (!PPPInForeGround)
				{
					err = OTPPPDialingInformation(&redial, &numRedials, &delay);
					if (err != noErr || numRedials < 1) redial = false;	// couldn't get redial data.  Ignore it.							
				
					dialCount = redial ? numRedials + 1 : 1;
				}
				else dialCount = 1;		//redials are handled in the PPP connection dialog already.
				
				// Establish a new connection		
				TurnOnPPPConnectionDialog(MyOTPPPInfo.ref, PPPInForeGround);
				
				// force the current connection to close if it hasn't been to to already
				if (PPPState == kPPPStateOpened || PPPState == kPPPStateOpening)
				{					
					OTPPPDisconnect(true, false);
					if (pppErr = WaitForOTPPPDisconnect(true)) return (pppErr);
					MyOTPPPInfo.state = kPPPDown;
				}
		
				MyOTPPPInfo.result = noErr;
				ProgressMessageR(kpSubTitle,OT_PPP_CONNECT);				
				if (!PPPInForeGround) 				
				{
					GetRString(scratch, OT_PPP_CONNECT_MESSAGE);
					ProgressMessage(kpMessage,scratch);
				}

				// Actually connect OT/PPP
				MyOTPPPInfo.code = 0;	
				MyOTPPPInfo.state = kPPPOpening;
				result = kCCLErrorLineBusyErr;
				pppErr = OTIoctl(MyOTPPPInfo.ref, I_OTConnect, NULL);			
				
				// Bug 1648 - if we're connecting PPP with the connection dialog, we should sit and spin until it finishes dialig the phone.
				if (PPPInForeGround)
				{
					MyOTPPPInfo.status = inProgress;
					if (pppErr == noErr) pppErr = SpinOn(&(MyOTPPPInfo.status),0,True,False);
				}
				
				// Spin until we can do TCP/IP over the connection
				while (dialCount > 0 && result == kCCLErrorLineBusyErr && !PPPInForeGround && pppErr == noErr)
				{
					while (true)
					{
						MyOTPPPInfo.event = 0;
						MyOTPPPInfo.result = noErr;
						MyOTPPPInfo.status = inProgress;
						if (pppErr == noErr) pppErr = SpinOn(&(MyOTPPPInfo.status),0,True,False);
						
						result = MyOTPPPInfo.result;
						if (MyOTPPPInfo.state == kPPPUp || MyOTPPPInfo.state == kPPPDown || pppErr != noErr || MyOTPPPInfo.result < 0) break;
							
						// Update the progress dialog.
						GetRString(scratch, MyOTPPPInfo.event);
						ProgressMessage(kpMessage,scratch);
					}
					dialCount--;
					if (redial && result == kCCLErrorLineBusyErr) ProgressMessageR(kpSubTitle,OT_PPP_REDIALING);
				}
			}
			else
			{	
				//there's a connection current trying to open
				if (PPPState == kPPPStateOpening)
				{
					// do the ARA delay ...
					doDelay = true;
					
					//Spin until we can talk TCP over the connection
					ProgressMessageR(kpSubTitle,OT_PPP_CONNECT);				
					GetRString(scratch, OT_PPP_CONNECT_MESSAGE);
					ProgressMessage(kpMessage,scratch);
					
					MyOTPPPInfo.state = kPPPOpening;
					while (true)
					{
						MyOTPPPInfo.event = 0;
						MyOTPPPInfo.result = noErr;
						MyOTPPPInfo.status = inProgress;
						err = SpinOn(&(MyOTPPPInfo.status),0,True,False);
					
						result = MyOTPPPInfo.result;
						if (MyOTPPPInfo.state == kPPPUp || MyOTPPPInfo.state == kPPPDown || err != noErr || MyOTPPPInfo.result < 0) break;
							
						// Update the progress dialog.
						GetRString(scratch, MyOTPPPInfo.event);
						ProgressMessage(kpMessage,scratch);
					}
					
					if (err == noErr) err = MyOTPPPInfo.result;
				}
				
				if (err == noErr)
				{
					MyOTPPPInfo.state = kPPPUp;
					*attemptedConnection = false;	// PPP is up, but we didn't connect it.
					pppErr = noErr;
				}
				else return (pppErr = err);
			}
		}
		else	//could not determine the state of the PPP connection
		{
			return (pppErr = errPPPStateUnknown);		
		}
	}				
	else
		return (pppErr = result);
	
	// The connection is up because we connected it, or someone else did
	if (pppErr == noErr)
	{
		if (MyOTPPPInfo.state == kPPPUp) 
		{
			// must we delay for ARA?
			if (doDelay)
			{
				// 	PPP Race Condition Hack.  
				//
				//	Since ARA and PPP have been combined, it's now possible to get the kPPPConnectCompletedEvent
				//	*before* ARA does.  This will cause the first network operation to fail.  So, let's Pause
				//	for a while, and let ARA know about it's successful connection.
				long delay = GetRLong(OT_PPP_RACE_HACK);
				Str255 delayStr;
				
				if (delay > 0) 
				{
					NumToString(delay, delayStr);
					ComposeRString(scratch,OT_PPP_SMART_ASS,delayStr);
					ProgressMessage(kpMessage,scratch);
					
					Pause(60*delay);
				}
			}			
			
			MyOTPPPInfo.weConnectedPPP = *attemptedConnection;	// remember if we started the connections ourselves or not.	
		}
		else
		{
			if (!PPPInForeGround) pppErr = MyOTPPPInfo.result ? MyOTPPPInfo.result : kCCLErrorGeneric;	//return some sort of error
			else pppErr = userCancelled;	//errors are handled in the PPP connection dialog
		}
	}
					
	return (pppErr);
}

/*********************************************************************************
 * NewPPPControlEndpoint - Set up the global PPP enpoint we use to control PPP.
 *********************************************************************************/
OTResult NewPPPControlEndpoint(void)
{
	OTResult	result = kOTNoError;
	TEndpointInfo epInfo;
	short oldResFile = CurResFile();
	DECLARE_UPP(MyPPPEndpointNotifier,OTNotify);
	
	INIT_UPP(MyPPPEndpointNotifier,OTNotify);

	MyOTPPPInfo.ref = OTOpenEndpointInContext(OTCreateConfiguration(kPPPControlName),0, &epInfo, &result,nil);
	UseResFile(oldResFile);	// bug in Modem control panel resets curresfile SD 8/5/98
	
	if((result == kOTNoError) && (MyOTPPPInfo.ref != kOTInvalidEndpointRef))
	{
		result = OTInstallNotifier(MyOTPPPInfo.ref, MyPPPEndpointNotifierUPP, (void *)NULL);
		if(result == kOTNoError)
		{
			if ((result = OTIoctl(MyOTPPPInfo.ref, I_OTGetMiscellaneousEvents, (void*)1)) == kOTNoError ) 
			{
				MyOTPPPInfo.state = kPPPDown;		//kPPPDown means PPP has not been touched by us yet.
			}
			OTSetAsynchronous(MyOTPPPInfo.ref);
		}
	}

	//if some error occurred, return 0, but don't close the endpoint.
	if (result != kOTNoError) MyOTPPPInfo.ref = kOTInvalidEndpointRef;

	return (result);
}
	
	
/*********************************************************************************
 * TurnOnPPPConnectionDialog - make the PPP connection a modal event
 *
 * If on is true, then we will turn the connection dialog ON.  If on is false,
 * we will restore the conenction dialog to its previous state.
 *********************************************************************************/
OSErr TurnOnPPPConnectionDialog(EndpointRef endPoint, Boolean on)
{
	OSErr err = noErr;
	long dlogState = 0;

	err = GetCurrentUInt32Option(endPoint, OPT_ALERTENABLE, &dlogState);
	if (err == noErr)
	{
		if (oldDlogState == 0) oldDlogState = dlogState;
		
		if (on)		//set the kPPPConnectionStatusDialogsFlag
			dlogState = dlogState | kPPPConnectionStatusDialogsFlag;
		else 		//turn the kPPPConnectionStatusDialogsFlag off.
			dlogState = dlogState & (kPPPAllAlertsEnabledFlag - kPPPConnectionStatusDialogsFlag);
		
		if (dlogState != oldDlogState)
			err = SetCurrentUInt32Option(endPoint, OPT_ALERTENABLE, dlogState);
	}
	
	return (err);
}

/*********************************************************************************
 * ResetPPPConnectionDialog - rest the statusDialogFlag to what it was before we
 * started messing with it.
 *********************************************************************************/
OSErr ResetPPPConnectionDialog(EndpointRef endPoint)
{
	OSErr err = noErr;
	long dlogState = 0;

	err = GetCurrentUInt32Option(endPoint, OPT_ALERTENABLE, &dlogState);
	if (err == noErr)
	{	
		if (dlogState != oldDlogState)	
		{
			if (!(oldDlogState&kPPPConnectionStatusDialogsFlag))	//if it was off to begin with, turn it off to reset
				dlogState = dlogState & (kPPPAllAlertsEnabledFlag - kPPPConnectionStatusDialogsFlag);
			else 	//if it was on before, turn it back on to reset.
				dlogState = dlogState | kPPPConnectionStatusDialogsFlag;
			
			err = SetCurrentUInt32Option(endPoint, OPT_ALERTENABLE, dlogState);
		}
		oldDlogState = 0;
	}
	
	return (err);
}

/*********************************************************************************
 * GetPPPConnectionState - determine the state of PPP
 *********************************************************************************/
OSErr GetPPPConnectionState(EndpointRef endPoint, unsigned long *PPPState)
{
	OSErr			err = noErr;

	err = GetCurrentUInt32Option(endPoint, PPP_OPT_GETCURRENTSTATE, PPPState);
	
	if (err != noErr) *PPPState = kPPPStateInitial;
		
	return (err);
}

/*********************************************************************************
 * GetCurrentUInt32Option - gets a UInt32 option from a PPP control point.
 *********************************************************************************/
OSErr GetCurrentUInt32Option(EndpointRef endPoint, OTXTIName theOption, UInt32 *value)
{
	OSErr err = noErr;
	unsigned char buf[sizeof(TOption)];
	TOption *option;
	TOptMgmt command;

	*value = 0;
	
	command.opt.buf = buf;
	command.opt.len = sizeof(TOption);
	command.opt.maxlen = sizeof(TOption);
	command.flags = T_CURRENT;
	
	option = (TOption *)buf;
	option->len = sizeof(TOption);
	option->level = COM_PPP;
	option->name = theOption;
	option->status = 0;
	option->value[0] = 0;
	
	//get the current alert flags
	err = OTOptionManagement(endPoint, &command, &command);
	
	if ((err != noErr) || (option->status == T_FAILURE) || (option->status == T_NOTSUPPORT))
		*value = 0L;
	else
		*value = option->value[0];
		
	return (err);
}

/*********************************************************************************
 * SetCurrentUInt32Option - sets a UInt32 option for PPP control point options.
 *********************************************************************************/
OSErr SetCurrentUInt32Option(EndpointRef endPoint, OTXTIName theOption, UInt32 value)
{
	OSErr err = noErr;
	unsigned char buf[sizeof(TOption)];
	TOption *option;
	TOptMgmt command;
	
	command.opt.buf = buf;
	command.opt.len = sizeof(TOption);
	command.opt.maxlen = sizeof(TOption);
	command.flags = T_NEGOTIATE;
	
	option = (TOption *)buf;
	option->len = sizeof(TOption);
	option->level = COM_PPP;
	option->name = theOption;
	option->status = 0;
	option->value[0] = value;
	
	//get the current alert flags
	err = OTOptionManagement(endPoint, &command, &command);
	
	return (err);
}

/*********************************************************************************
 * OTPPPDisconnect - disconnect OTPPP if Eudora connected it in the first place.
 * if (forceDisconnect) then we disconnect no matter who connected PPP
 * if (endConnectionAttempt) then we reset the PPP connection dialog.
 *********************************************************************************/
OSErr OTPPPDisconnect(Boolean forceDisconnect, Boolean endConnectionAttempt)
{
	OSErr err = noErr;
	
	if ((gHasOTPPP == true) 
		&& ((MyOTPPPInfo.ref && PrefIsSet(PREF_PPP_DISC) && MyOTPPPInfo.weConnectedPPP) || forceDisconnect))
	{						
		err = OTIoctl(MyOTPPPInfo.ref, I_OTDisconnect, 0);
		
		if (endConnectionAttempt) 
		{
			ResetPPPConnectionDialog(MyOTPPPInfo.ref);
		}
		
		MyOTPPPInfo.weConnectedPPP = false;
		MyOTPPPInfo.status = MyOTPPPInfo.result = 0;
	}
	
	return (err);
}

/*********************************************************************************
 * OTPPPConnectForLink - connect OT/PPP to follow a link.  Ignore the fact that
 *	we connected it, so we don't go try to close the connection later.
 *********************************************************************************/
OSErr OTConnectForLink(void)
{
	OSErr err = userCanceledErr;
	unsigned long conMethod = 0;
	Boolean weAttemptedPPP = false;
	
	// Only makes sense to do this when OT is present
	if (gUseOT)
	{
		// How are we connecting to the internet? Read from the preference files or NS database
		err = SelectedConnectionMode(&conMethod,true);
			
		// do we have PPP?
		if (gHasOTPPP)
		{
			// Is OT/PPP the mode of connection?
			if (conMethod == kPPPSelected)
			{
				dialingThePhone = true;
				needPPPConnection = true;	
				gConnecting = true;	
				if (err = OTPPPConnect(&weAttemptedPPP))
				{					
					OTPPPDisconnect(weAttemptedPPP, true);
				}
				gConnecting = false;
				dialingThePhone = false;
				
				// Forget about the fact that we connected OT/PPP.
				MyOTPPPInfo.weConnectedPPP = false;
			}
		}
	}
		
	return (err);
}
			
/*********************************************************************************
 * SelectedConnectionMode - determine the connection mode set in the TCP/IP control
 * panel.
 *
 *	12-16-98 JDB
 *	 Added the forceRead paramter to read from the TCP file, even if cache time
 *	has not yet run out.  We really want to know the state of TCP/IP at each
 *	connection attempt.  We don't care as much at idle time for things like
 *	the next check menu item.
 *
 *	5-19-99 JDB
 *	 Added calls to the Network Setup library.  This should prevent breakage.
 *
 *	7-16-99 JDB
 *	 Use the cached connection method unless told not to
 *
 *	12-3-99 JDB
 *	 Use the cached connection method if we just read the preferences
 *********************************************************************************/
OSErr SelectedConnectionMode(unsigned long *connectionSelection, Boolean forceRead)
{
	OSErr err = noErr;
	char currentPortName[kMaxProviderNameSize];
	char junk[kMaxProviderNameSize];
	Boolean enabled;
	static uLong method;
	static uLong lastRead = 0;

	if (HaveOSX()) 
	{
		// there's no way to tell if we're dialed up or not under OS X.
		*connectionSelection = kOtherSelected;
		return (noErr);
	}
	
	// do NOT call this unless OT is installed
	if (OTIs == false) return (fnfErr);
			
	// did we just read the preferences recently?
	if (!forceRead && (lastRead>0) && ((TickCount()-lastRead) < GetRLong(TCP_PREF_REUSE_INTERVAL)))
	{
		*connectionSelection = method;
		return (noErr);
	}

	// are we falling asleep?  Then return value from the last time, no matter what.
	if (UserIdle(TICKS2MINS) && (lastRead>0))
	{
		*connectionSelection = method;
		return (noErr);
	}

#ifdef	DEBUG
	Log(LOG_TRANS,"\pReading TCP/IP preferences from the disk now.");
#endif

#ifdef	USE_NETWORK_SETUP
	if (UseNetworkSetup()) 
	{
		if (IsNetworkSetupAvailable())
		{
			err = GetConnectionModeFromDatabase(connectionSelection);

			lastRead = TickCount();
			method = *connectionSelection;
		}
		else
		{
			// we have to use the Network Setup Library, but it's not available.  
			// Assume non-PPP and non-MacSLIP connection.
			lastRead = TickCount();
			method = *connectionSelection = kOtherSelected;
		}
	}
	else
	{
#endif	
		*connectionSelection = kOtherSelected;
		
		// read the port name from the TCP/IP preference file
		err = GetCurrentPortNameFromFile(currentPortName, junk, &enabled);
		if (err == noErr)
		{
			if (StringSame(currentPortName,PPP_NAME))	
				*connectionSelection = kPPPSelected;
		}
		
		if (!err)
		{
			lastRead = TickCount();
			method = *connectionSelection;
		}
#ifdef	USE_NETWORK_SETUP
	}
#endif
		
	return (err);
}

/*******************************************************************************
 * UserIdle - see if the user has been idle.
 *******************************************************************************/
Boolean UserIdle(uLong ticks)
{
	Boolean result = false;
#ifdef	HAVE_GETLASTACTIVITY	
	static uLong idleTicks;
	ActivityInfo info;
	OSErr err = noErr;

	if (((GestaltBits(gestaltPowerMgrAttr)&(1<<gestaltPMgrDispatchExists))!=0))
	{
		info.ActivityTime = 0;
		info.ActivityType = UsrActivity;
		if (!(err=GetLastActivity(&info)))
		idleTicks = TickCount()-info.ActivityTime;
		
		if (idleTicks > ticks)
			result = true;	
	}
#endif

	return (result);
}

/*******************************************************************************
 * CanCheckPPPState - is it possible for us to check PPP's state?
 *******************************************************************************/
Boolean CanCheckPPPState(void)
{
	return !PrefIsSet(PREF_IGNORE_PPP) && (HaveTheDiseaseCalledOSX()||!gMissingNSLib);
}

/*******************************************************************************
 * CanChangePPPState - is it possible for us to change PPP's state?
 *******************************************************************************/
Boolean CanChangePPPState(void)
{
	return !PrefIsSet(PREF_IGNORE_PPP) && !gMissingNSLib && !HaveTheDiseaseCalledOSX();
}

/*******************************************************************************
 * PPPDown - is PPP installed, the selected mode of connection & disconnected?
 *******************************************************************************/
Boolean PPPDown(void)
{
	unsigned long con = 0;
	static Boolean ret = false;
	static uLong lastCheck;

	ASSERT(!PrefIsSet(PREF_IGNORE_PPP));
	
	if (HaveTheDiseaseCalledOSX())
	{
		if (TickCount()-lastCheck > GetRLong(TCP_PREF_REUSE_INTERVAL)/10+1)
		{
			Str255 host;
			
			// grab a specific host to check; if there
			// isn't one, use the mailhost
			if (!*GetRString(host,PPP_REACHABLE_HOST))
				GetPOPInfo(nil,host);
			
			// check it
			if (*host && CHostUnreachableByPPP(host+1)) ret = true;
			else ret = false;
			
			lastCheck = TickCount();
		}
	}
	else if (gHasOTPPP)
	{
		SelectedConnectionMode(&con,false);
		if (con == kPPPSelected)
		{
			if (MyOTPPPInfo.ref == 0) NewPPPControlEndpoint();
			GetPPPConnectionState(MyOTPPPInfo.ref, &con);
			if (con != kPPPStateOpened) ret = true;
		}
	}
	
	return (ret);
}

#pragma segment Main
/*********************************************************************************
 * MyPPPEndpointNotifier - notifier function for PPP endpoint events.  It directly
 * modifies the MyOTPPPInfo global structure.
 *********************************************************************************/
pascal void MyPPPEndpointNotifier(void *context, OTEventCode code, OTResult result, void *cookie)
{		
	if (code > kPPPEvent && code <= kPPPDCECallFinishedEvent) 
	{
		MyOTPPPInfo.status = 0;
		MyOTPPPInfo.result = (OTResult)cookie;
		
		if ((code == kPPPConnectCompleteEvent) && (MyOTPPPInfo.result != kOTNoError))			
			MyOTPPPInfo.event = OTPPP_MSG_STRN + (kPPPDCECallFinishedEvent + 1) - kPPPEvent + 1;
		else
			MyOTPPPInfo.event = OTPPP_MSG_STRN + code - kPPPEvent + 1;
	}
	
	if (code == kPPPDisconnectCompleteEvent)	// Disconnect has completed.
		MyOTPPPInfo.state = MyOTPPPInfo.result ? kPPPClosing : kPPPDown;
	if (code == kPPPLowerLayerDownEvent)	//remote server isn't responding	added this 11-21-97
		MyOTPPPInfo.state = kPPPDown;
	else if ((code == kPPPConnectCompleteEvent) && (MyOTPPPInfo.result == kOTNoError))			
		MyOTPPPInfo.state = kPPPUp;
}
#pragma segment TcpTrans

/*********************************************************************************
 * WaitForOTPPPDisconnect - sit and spin until the PPP state is kPPPStateInitial
 *********************************************************************************/
OSErr WaitForOTPPPDisconnect(Boolean showStatus)
{
	OSErr 			err = noErr;
	long 			ticks=TickCount();
	long 			startTicks=ticks+120;
	long 			now;
	Str255 			scratch;
	unsigned long 	PPPState;
	
	//wait for the connection to close before we start another
	if (showStatus)
	{
		GetRString(scratch, OT_PPP_DISCONNECT);
		ProgressMessage(kpMessage,scratch);
	}
	
	do
	{
		now = TickCount();
		if (now>startTicks && now-ticks>10) 
		{
			CyclePendulum();
			ticks=now;
		}
		MiniEvents();
		if (CommandPeriod) return(pppErr = userCancelled);
		err = GetPPPConnectionState(MyOTPPPInfo.ref, &PPPState);
	}
	while ((PPPState != kPPPStateInitial) && (err == noErr));
	
	if (err != noErr && err != userCancelled) err = errPPPStateUnknown;
	
	return (err);
}


/*********************************************************************************
 * OTPPPDialingInformation - retrieve the redial options from the PPP settings file.
 *
 * 	- Look in Preferences folder for lzcn/rmot, the Remote Access Connections file
 *	- Open the resource fork
 *	- Fetch 'cdia' id 128, which contains the info we need.
 *	
 *		the fourth long in this resource contains 3 or 4 if we are to redial.
 *		the fifth long contains the number of redials to do before giving up.
 *		the sixth long contains 1000*number of seconds to delay.
 *
 * If any of this fails, we continue on as if nothing happened.  Redialing just won't
 * happen.
 *
 * The spec locating the rmot preference file is cached.  This way, we can check
 * the preference file periodically, and make sure the setting hasn't changed.
 *
 * This will break in a future OT. 
 *	
 *	5-19-99 JDB
 *	 Added calls to the Network Setup library.  This should prevent breakage.
 *********************************************************************************/
OSErr OTPPPDialingInformation(Boolean *redial, unsigned long *numRedials, unsigned long *delay)
{
	OSErr err = noErr;
	short vRef = 0;
	long dirId = 0;
	CInfoPBRec hfi;
	Str31 name;
	short refNum = 0;
	Handle probe = 0;
	short oldRes;
	
	oldRes = CurResFile();
	
	//do NOT call this unless OT/PPP is installed and being used
	if (gHasOTPPP == false) return (fnfErr);
	
	*numRedials = *delay = 0;
	*redial = false;

#ifdef	USE_NETWORK_SETUP
	// grok the settings from the TCP/IP preference file using the Network Setup Library.
	if (UseNetworkSetup()) 
	{
		err = GetPPPDialingInformationFromDatabase(redial, numRedials, delay);
		return (err);
	}
#endif
		
	if (PPPprefFileSpec.name[0] == 0)	//have we not yet already located the PPP pref file?
	{
		/* Locate the PPP preferences file */
				
		//find the active Preferences folder
		err = FindFolder(kOnSystemDisk,kPreferencesFolderType,False,&vRef,&dirId);
		if (err == noErr)
		{
			hfi.hFileInfo.ioNamePtr = name;
			hfi.hFileInfo.ioVRefNum = vRef;
			SearchDirectoryForFile(&hfi, dirId, PPP_PREF_FILE_TYPE, PPP_PREF_FILE_CREATOR);
		}
	}
		
	if (PPPprefFileSpec.name[0] != 0)	//have we located the PPP pref file?
	{
		refNum = FSpOpenResFile(&PPPprefFileSpec,fsRdPerm);
		err = ResError();
		if (err == noErr && refNum >= 0)
		{	
			//this could breka in future versions of OT
			probe = Get1Resource(DIAL_RESOURCE,DIAL_RESOURCE_ID);
			err = ResError();
			if (err == noErr && probe != 0)
			{
				//this is agonna break in future versions ot fer sure
				if (((long *)*probe)[3] == 2)
				{
					*redial = false;
					*numRedials = *delay = 0;
				}
				else
				{
					*redial = true;
					*numRedials = ((long *)*probe)[4];
					*delay = ((long *)*probe)[5]/1000;
				}
			}
		}
		CloseResFile(refNum);
	}
		
	if ((err != noErr) || (PPPprefFileSpec.name[0] == 0)) err = errPPPPrefNotFound;
	
	UseResFile(oldRes);
	
	return (err);
}

/*********************************************************************************
 * SearchDirectoryForFile - Recursively search a directory for a file with a
 * given creator and file type.  This is an expensive function to call.
 *********************************************************************************/
Boolean SearchDirectoryForFile(CInfoPBRec *info, long dirToSearch, OSType type, OSType creator)
{
	short index = 1;
	OSErr err = noErr;
	Boolean static foundIt;
	
	foundIt = false;
	
	do
	{
		info->hFileInfo.ioFDirIndex = index;
		info->hFileInfo.ioDirID = dirToSearch;
		
		err = PBGetCatInfoSync(info);
		
		if (err == noErr)
		{
			//found a directory
			if ((info->hFileInfo.ioFlAttrib & ioDirMask)	!= 0)
			{
				SearchDirectoryForFile(info, info->hFileInfo.ioDirID, type, creator);
				err = noErr;
			}
			else	//found a file
			{
				if ((info->hFileInfo.ioFlFndrInfo.fdType == type &&
						info->hFileInfo.ioFlFndrInfo.fdCreator == creator))
				{
					// Found it.  Stop the search!
					foundIt = true;
					FSMakeFSSpec(info->hFileInfo.ioVRefNum, info->hFileInfo.ioFlParID, info->hFileInfo.ioNamePtr, &PPPprefFileSpec);
				}
			}
			++index;
		}
	} while (err == noErr && !foundIt);
	
	return (foundIt);
}

/*********************************************************************************
 * SpinOnWithConnectionCheck - spin until a return code is not inProgress.  Check 
 * connection while spinning.
 *********************************************************************************/
short SpinOnWithConnectionCheck(short *rtnCodeAddr,long maxTicks,Boolean allowCancel,Boolean forever)
{
	long ticks=TickCount();
	long startTicks=ticks+120;
	long now;
#ifdef CTB
	extern ConnHandle CnH;
#endif
	Boolean oldCommandPeriod = CommandPeriod;
	Boolean slow = False;
	static short slowThresh;
	
	if (!slowThresh) slowThresh = GetRLong(SPIN_LENGTH);
	
	if (allowCancel) YieldTicks = 0;
	if (allowCancel || *rtnCodeAddr==inProgress)
	{
		CommandPeriod = False;
		do
		{
			// check to see if we need ppp, but the connection is down
			if (needPPPConnection && MyOTPPPInfo.state != kPPPUp) return(errLostConnection);
					
			now = TickCount();
			if (now>startTicks && now-ticks>slowThresh) {slow = True;if (!InAThread()) CyclePendulum(); else MyYieldToAnyThread();ticks=now;}
			MiniEvents();
			if (CommandPeriod  && !forever) return(userCancelled);
			if (maxTicks && startTicks+maxTicks < now+120) break;
		}
		while (*rtnCodeAddr == inProgress);
		if (CommandPeriod) return(userCancelled);
		CommandPeriod = oldCommandPeriod;
	}
	return(*rtnCodeAddr);
}

//Some sticky stuff

/*********************************************************************************
 * GetHostByAddr - Call either TCPGetHostByAddr or OTGetHostByAddr, depending on
 * whether OT is installed or not.
 *********************************************************************************/
OSErr GetHostByAddr(struct hostInfo *hostInfoPtr,long addr)
{
	// Do we have a NAT?
	if (0x0A000000 == (addr&0xff000000) ||
			0xAC100000 == (addr&0xfff00000) ||
			0xC0A80000 == (addr&0xffff0000))
	{
		Str31 literal;
		ComposeRString(literal,NAT_FMT,addr);
		literal[*literal+1] = 0;
		strcpy(hostInfoPtr->cname,literal+1);
		hostInfoPtr->addr[1] = addr;
		hostInfoPtr->rtnCode = 0;
		return noErr;
	}
	
	if (gUseOT == true)		// OT is installed
	{
		InetHostInfo domainName;
		OSErr err = OTGetHostByAddr(addr, &domainName);
		
		if (err == noErr)	// our caller expects the hostInfoPtr to point a hostInfo struct.
		{
			short count;
			
			strcpy(hostInfoPtr->cname,domainName.name);		
			for (count = 0; count < MIN(NUM_ALT_ADDRS,kMaxHostAddrs); count ++)
				hostInfoPtr->addr[count] = domainName.addrs[count];
			hostInfoPtr->rtnCode = 0;
		}
		return (err);
	}
	ASSERT ( false );
	return unimpErr;	/* unreachable, I think */
}


/*********************************************************************************
 * GetHostByName - Call either TCPGetHostByName or OTGetHostByName, depending on
 * whether OT is installed or not.
 *
 * The caller expects hostInfoPtr to be pointing to a TCP hostInfo struct.  So
 * if we do the OT thing, point hostInfoPtr at a hostInfo struct we fill with
 * the results of the OTGetHostByName call.
 *********************************************************************************/
int GetHostByName(UPtr name, struct hostInfo **hostInfoPtr)
{
	static struct hostInfo trickCaller;
	
	if (gUseOT == true)	// OT is installed
	{
		InetHostInfo domainName;
		int err = noErr;
		short count;
		InetDomainName hostName;
		
		PtoCcpy(hostName,name);
		
		err = OTGetHostByName(hostName, &domainName);
		
		if (err == noErr)	// our caller expects the hostInfoPtr to point a hostInfo struct.
		{					
			*hostInfoPtr = &trickCaller;
			
			strcpy(trickCaller.cname,domainName.name);
			for (count = 0; count < MIN(NUM_ALT_ADDRS,kMaxHostAddrs); count ++)
				trickCaller.addr[count] = domainName.addrs[count];
			trickCaller.rtnCode = 0;
		}
		return (err);
	}
	ASSERT ( false );
	return unimpErr;	/* unreachable, I think */
}

/*********************************************************************************
 * PPPIsMostDefinitelyUpAndRunning - return true if we're connected with PPP, or
 *	it's not an issue.
 *********************************************************************************/
Boolean PPPIsMostDefinitelyUpAndRunning(void)
{
	Boolean connected = true;
	unsigned long con = 0;
	
	if (gHasOTPPP)
	{
		SelectedConnectionMode(&con,false);
		if ((con == kPPPSelected) && (MyOTPPPInfo.state != kPPPUp)) connected = false;
	}
	
	return (connected);
}

/*********************************************************************************
 * UpdateCachedTCPIPPrefInfo - read from the preference files or NS Library now
 *********************************************************************************/
void UpdateCachedConnectionMethodInfo(void)
{
	unsigned long con = 0;
	static uLong method;
	
	if (SelectedConnectionMode(&con, true)==noErr)
	{
		// return true if the method has changed
		if (con != method)
		{
			method = con;
			gUpdateTPWindow = true;	// update the TP window
		}
	}
}

/*********************************************************************************
 * NeedToUpdateTP - do we need to adjust the next check time in the TP window?
 *********************************************************************************/
Boolean NeedToUpdateTP(void)
{
	Boolean updateIt = false;
	
	if (gUpdateTPWindow)
	{
		updateIt = true;
		gUpdateTPWindow = false;
	}
	
	return (updateIt);
}

/*********************************************************************************
 * AutoCheckOKWithDBRead - read from the preference files or NS Library and see
 *	if an autocheck is appropriate.
 *********************************************************************************/
Boolean AutoCheckOKWithDBRead(Boolean updatePers)
{
	Boolean result = false;
	OSErr err = noErr;
	
	// are we set up to not check when not connected?
	if (gUseOT && !PrefIsSet(PREF_IGNORE_PPP) && PrefIsSet(PREF_PPP_NOAUTO))
	{
		// make sure we're really, truly connected
		UpdateCachedConnectionMethodInfo();
		result = AutoCheckOK();
		
		// we're not.  Tell this personality to cram it.
		if (!result && updatePers) 
		{
			PersSkipNextCheck();
			gUpdateTPWindow = true;
		}
	}
	else
	{
		// not set to not check when not connected.  Do the normal thing.
		result = AutoCheckOK();
	}	
	
	return (result);
}

#ifdef ESSL
// Declare the routine to set up the TransVector for doing SSL
TransVector ESSLSetupVector(TransVector theTrans);
#endif

TransVector GetTCPTrans()
{
	TransVector theTrans;
	
	theTrans = OTTCPTrans;
#ifdef ESSL
	return ESSLSetupVector(theTrans);
#else
	return theTrans;
#endif
}

/************************************************************************
 * TcpFastFlush - run through the queue, killing off defunct streams
 * Call KillDeadKyTStreams if we happen to be using open transport.
 ************************************************************************/
void TcpFastFlush(Boolean destroy)
{
	static Boolean flushing = false;
	
	// are we already flushing streams from somewhere?
	if (flushing) return;
	
	// kill defunct streams
	flushing = true;
	
	if (gUseOT) 
		KillDeadMyTStreams(destroy);
	
	// we're done flushing streams for now.
	flushing = false;
}


/**********************************************************************
 * CheckConnectionSettings - Attempt to connect to a host/port pair.
 **********************************************************************/
OSErr CheckConnectionSettings ( UPtr host, long port, StringPtr errorMessage ) {
	OSErr err = noErr;
	TransStream stream = NULL;
	Boolean oldPref = PrefIsSet(PREF_IGNORE_PPP);

	
//	Init the TransStream
	if ( noErr == ( err = NewTransStream ( &stream ))) {
	
	//	See if the host is there....
		SetPref(PREF_IGNORE_PPP,YesStr);
		err = ConnectTrans ( stream, host, port, true, GetRLong(SHORT_OPEN_TIMEOUT));
		SetPref(PREF_IGNORE_PPP,oldPref ? YesStr : NoStr);

	//	Grab an error message if we failed
		if ( noErr != err && errorMessage != NULL ) {
			short realSettingsRef = SettingsRefN;

			SettingsRefN = GetMainGlobalSettingsRefN();		
			OTTELo ( errOpenStream, stream->streamErr, errorMessage );
			SettingsRefN = realSettingsRef;
			}
			
	//	That's all we wanted.  Cleanup.
		if ( noErr == err )
			DestroyTrans(stream);
		ZapTransStream ( &stream );
		}
	
	return err;
	}
