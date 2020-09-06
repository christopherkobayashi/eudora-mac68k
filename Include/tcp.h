/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TCP_H
#define TCP_H

#include <OpenTransport.h>
#include <OpenTransportProviders.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with tcp streams
 ************************************************************************/

enum {
	NUM_ALT_ADDRS = 4
};

struct hostInfo {
	long rtnCode;
	char cname[255];
	SInt8 filler;		/* Filler for proper byte alignment      */
	unsigned long addr[NUM_ALT_ADDRS];
};
typedef struct hostInfo hostInfo;

typedef struct HIQ HostInfoQ, *HostInfoQPtr, **HostInfoQHandle;
struct HIQ {
	HostInfoQHandle next;
	struct hostInfo hi;
	short done;
	Boolean inUse;
};

void TcpFastFlush(Boolean destroy);

int GetTCPStatus(TransStream stream, TCPiopb * pb);
OSErr GetHostByAddr(struct hostInfo *hi, long addr);
int GetHostByName(UPtr name, struct hostInfo **hostInfoPtr);
OSErr GetMyHostid(uLong * myAddr, uLong * myMask);
Boolean SplitPort(PStr host, long *port);


// These are PPP error codes, not yet defined in the OT Headers.  
// Remove these once the OT/PPP header is finalized.

#define	kCCLErrorBaseCode				-6000
#define	kCCLErrorAbortMatchRead			kCCLErrorBaseCode	// internal error used to abort match read
#define	kCCLErrorGeneric				(kCCLErrorBaseCode - 2)	// CCL error base
#define	kCCLErrorSubroutineOverFlow		(kCCLErrorBaseCode - 3)	// target label undefined
#define	kCCLErrorLabelUndefined			(kCCLErrorBaseCode - 4)	// target label undefined
#define	kCCLErrorBadParameter			(kCCLErrorBaseCode - 5)	// bad parameter error
#define	kCCLErrorDuplicateLabel			(kCCLErrorBaseCode - 6)	// duplicate label error
#define kCCLErrorCloseError				(kCCLErrorBaseCode - 7)	// There is at least one script open
#define kCCLErrorScriptCancelled		(kCCLErrorBaseCode - 8)	// Script Canceled
#define	kCCLErrorTooManyLines			(kCCLErrorBaseCode - 9)	// Script contains too many lines
#define kCCLErrorScriptTooBig			(kCCLErrorBaseCode - 10)	// Script contains too many characters
#define	kCCLErrorNotInitialized			(kCCLErrorBaseCode - 11)	// CCL has not been initialized
#define	kCCLErrorCancelInProgress		(kCCLErrorBaseCode - 12)	// Cancel in progress.
#define	kCCLErrorPlayInProgress			(kCCLErrorBaseCode - 13)	// Play command already in progress.
#define	kCCLErrorExitOK					(kCCLErrorBaseCode - 14)	// Exit with no error.
#define	kCCLErrorLabelOutOfRange		(kCCLErrorBaseCode - 15)	// Label out of range.
#define kCCLErrorBadCommand				(kCCLErrorBaseCode - 16)	// Bad command.
#define	kCCLErrorEndOfScriptErr			(kCCLErrorBaseCode - 17)	// End of script reached, expecting Exit.
#define	kCCLErrorMatchStrIndxErr		(kCCLErrorBaseCode - 18)	// Match string index is out of bounds.
#define	kCCLErrorModemErr				(kCCLErrorBaseCode - 19)	// Modem error, modem not responding.
#define	kCCLErrorNoDialTone				(kCCLErrorBaseCode - 20)	// No dial tone.
#define	kCCLErrorNoCarrierErr			(kCCLErrorBaseCode - 21)	// No carrier.
#define	kCCLErrorLineBusyErr			(kCCLErrorBaseCode - 22)	// Line busy.
#define	kCCLErrorNoAnswerErr			(kCCLErrorBaseCode - 23)	// No answer.
#define	kCCLErrorNoOriginateLabel		(kCCLErrorBaseCode - 24)	// No @ORIGINATE
#define	kCCLErrorNoAnswerLabel			(kCCLErrorBaseCode - 25)	// No @ANSWER
#define	kCCLErrorNoHangUpLabel			(kCCLErrorBaseCode - 26)	// No @HANGUP
#define	kCCLErrorVarStringRange			(kCCLErrorBaseCode - 27)	// varString value out-of-range
#define	kCCLErrorBaudRateUnavail		(kCCLErrorBaseCode - 28)	// h/w can't supply this rate
#define	kCCLErrorBaudRateChanged		(kCCLErrorBaseCode - 29)	// OT can't supply this rate

#define	kCCLErrorStart					kCCLErrorGeneric	// first error in the ccl range
#define	kCCLErrorEnd					kCCLErrorBaudRateChanged	// last error in the ccl range



#define NUM_MX				10	// The number of MX's to look for from a particular host

// Some error codes describing errors we get when using OT TCP
enum {
	errOTInitFailed = 700,	// OT failed to be initialized
	errOTInetSvcs,		// OT failed to open up the internet services
	errDNR,			// A DNR-related error occured
	errNoMXRecords,		// no MX records were found for a particular server
	errCreateStream,	// failed to create a new MyTStream struct
	errOpenStream,		// failed to establish a connection
	errLostConnection,	// lost connection
	errMiscRec,		// some error occurred while receiving
	errMiscSend,		// some error occurred during send operation
	errPPPConnect,		// some error occurred while connecting with PPP
	errPPPPrefNotFound,	// could not find the TCP/IP preferences file
	errPPPStateUnknown,	// could not determine the state of the current PPP connection.
	errOTMissingLib,	// one of the OT libraries is missing.
	errMyLastOTErr
};

// Return codes for SelectedConnectionMode function
enum {
	kPPPSelected,
	kMacSLIPSelected,
	kOtherSelected
};

// PPP conenction state enum.
enum {
	kPPPLoaded = 0,
	kPPPNotLoaded,
	kPPPDown,
	kPPPClosing,
	kPPPUp,
	kPPPOpening
};

// Open Transport Internet services provider info
typedef struct MyOTInetSvcInfo {
	InetSvcRef ref;		// provider reference
	short status;		// set to 0 when complete
	OTResult result;	// result code
	void *cookie;		// something to munch on while we wait
} MyOTInetSvcInfo;

// Stream info structure, contains info about a connection
typedef struct MTS MyOTTCPStream, *MyOTTCPStreamPtr, *MyOTTCPStreamHandle;
struct MTS {
	EndpointRef ref;	// endpoint reference
	short status;		// 0 when complete, inProgress when, well, in progress
	OTEventCode code;	// event code
	OTResult result;	// result
	void *cookie;		// cookie

	Boolean weAreClosing;	// set to true when we are working on closing the stream.
	Boolean otherSideClosed;	// set to true when the other side has closed its connection
	Boolean ourSideClosed;	// true when we close our side of the connection        
	Boolean releaseMe;	// set to true after we can release this stream

	Str255 dummyBuffer;	// a dummy buffer to receive any chars after a disconnect

	MyOTTCPStreamPtr next;	// a pointer to the next MyStream, if queued
	MyOTTCPStreamPtr prev;	// a pointer to the prev MyStream, if queued

	long age;		// how old this stream is (in ticks).   
};

// PPP info structure, contains info about a PPP connection. 
typedef struct MyOTPPPInfoStruct {
	Boolean weConnectedPPP;	// set to true if we're the ones that connected ppp
	short state;		// stores the state of the PPP connection
	EndpointRef ref;	// endpoint reference
	short status;		// set to 0 when complete
	short event;		// stores which event completed.
	OTResult result;	// result code
	OTEventCode code;	// event code
} MyOTPPPInfoStruct, *MyOTPPPInfoPtr;

// Functions needed to use OT/TCP's PPP.  Others are local to tcp.c
OSErr OTPPPDisconnect(Boolean forceDisconnect,
		      Boolean endConnectionAttempt);

// Functions needed to use OT TCP
void OTInitOpenTransport(void);	// initialize OT if present.  May be called even if OT is not installed.
void OTCleanUpAfterOpenTransport(void);	// Clean up after OT.  May be called even if OT is not installed.

// These are OT versions of the TCP functions declared above.
OSErr OTTCPConnectTrans(TransStream stream, UPtr serverName, long port,
			Boolean silently, uLong timeout);
OSErr OTTCPSendTrans(TransStream stream, UPtr text, long size, ...);
OSErr OTTCPRecvTrans(TransStream stream, UPtr line, long *size);
OSErr OTTCPDisTrans(TransStream stream);
OSErr OTTCPDestroyTrans(TransStream stream);
OSErr OTTCPTransError(TransStream stream);
void OTTCPSilenceTrans(TransStream stream, Boolean silence);
UPtr OTTCPWhoAmI(TransStream stream, Uptr who);

void KillDeadMyTStreams(Boolean destroy);	//kills streams that have received an orderly disconnect

// OT/PPP and connection method related routines
OSErr SelectedConnectionMode(unsigned long *connectionSelection,
			     Boolean forceRead);
Boolean NeedToUpdateTP(void);
Boolean AutoCheckOKWithDBRead(Boolean updatePers);
Boolean CanCheckPPPState(void);
Boolean CanChangePPPState(void);
Boolean PPPDown(void);
Boolean PPPIsMostDefinitelyUpAndRunning(void);
OSErr OTConnectForLink(void);
void OTFlushInput(TransStream stream, uLong timeout);
#define ALMOST(x) (.95 * x)
Boolean UserIdle(uLong ticks);

OSErr DNSHostid(uLong * dnsAddr);
OSErr OTMyHostid(uLong * myAddr, uLong * myMask);

void OTErrorToString(short specificError, Str255 tcpMessage);

TransVector GetTCPTrans();

//      Check and see if we can connect
OSErr CheckConnectionSettings(UPtr host, long port,
			      StringPtr errorMessage);


#endif
