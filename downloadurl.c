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

/************************************************************************
 *
 * Download a URL. Downloads only HTTP files at the moment.
 *
 ************************************************************************/
#include "downloadurl.h"
#include "buildversion.h"

#define FILE_NUM 117
/* Copyright (c) 1998 by QUALCOMM Incorporated */

#pragma segment DownURL

/*
 *	constants
 */
enum
{
	kNotHTTPURLErr = -666,
	kWhatImplementationErr = -667,

	kTransferBufferSize = 4096
};

//	Thread parameter struct
typedef struct
{
	char	hostName[256];
	char	httpMethodCommand[512];
	DownloadInfo	info;
	short	destFileRefNum;
	OSErr	err;
	Ptr		transferBuffer;
	long	refCon;
	void	(*FinishFunc)(long,OSErr,DownloadInfo*);
	ThreadID threadID;
	HTTPinfo HTTPstuff;
	Boolean aborted;
	Boolean	completedDownload;
} URLParms, *URLParmsPtr, **URLParmsHandle;
					
//	prototypes
static pascal void *DownloadURLThread (void *threadParameter);
static pascal void MyThreadTermination (ThreadID threadTerminated, void *terminationProcParam);
static pascal void YieldingNotifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);
static OSErr DownloadHTTPSimple(URLParmsHandle threadData,char *hostName,char *httpCommand, Boolean *redirect,HTTPinfo *pHTTPstuff);
static OSErr ProcessHeader(Ptr *transferBuffer,long *bytesReceived,ResType *type,ResType *creator, URLParmsHandle threadData);
static OSErr GetURLHost(const char *urlString, char *hostName,char *httpMethodCommand,Boolean doPOST);
static short ParseHTTPHeader(StringPtr sHeader,char *sResult,Ptr p,long bufLen);

/************************************************************************
 * DownloadURL - download file specified by URL, supports HTTP only
 *
 * To use POST: Pass HTTPinfo. The handles for  will be
 *   disposed of automatically. Do not dispose from caller!
 ************************************************************************/
OSErr DownloadURL(const char *urlString, FSSpecPtr destSpec,long refCon,void (*FinishFunc)(long,OSErr,DownloadInfo*),long *pReference,HTTPinfo *pHTTPstuff)
{
	URLParmsHandle	threadData = nil;
	OSErr	err = noErr;
	ThreadID threadID;
	short destFileRefNum = 0;
	DECLARE_UPP(DownloadURLThread,ThreadEntry);
	DECLARE_UPP(MyThreadTermination,ThreadTermination);
	
	if (TCPWillDial(true)) return -1;	//	Don't dial to download
	
#ifdef DEBUG
		if (pHTTPstuff && pHTTPstuff->post) ComposeLogS(LOG_PLIST,nil,"\pDownloadURL: %s",urlString);
#endif

	//	Set up thread
	if (threadData = NuHandleClear(sizeof(**threadData)))
	{
		// Parse the URL to get the hostname and HTTP GET command strings
		LDRef(threadData);
		err = GetURLHost(urlString,(*threadData)->hostName,(*threadData)->httpMethodCommand,pHTTPstuff && pHTTPstuff->post);
		UL(threadData);
		if (!err)
		{
			//	Create and open the file
			CInfoPBRec	hfi;
			
			FSpCreate(destSpec,CREATOR,kFileInProcessType,smSystemScript);	//	Type and creator will be changed later
			if (!AFSpGetCatInfo(destSpec,destSpec,&hfi) && hfi.hFileInfo.ioFRefNum)
			{
				//	This file's already open. We can't write to it.
				err = fBsyErr;
#ifdef DEBUG
				if (RunType!=Production) DebugStr("\pDownload file is already open!");
#endif						
			}
			else
				err = FSpOpenDF(destSpec,fsRdWrPerm,&destFileRefNum);

			if (!err)
			{
				(*threadData)->info.spec = *destSpec;
				(*threadData)->destFileRefNum = destFileRefNum;
				(*threadData)->transferBuffer = NuPtr(kTransferBufferSize);
				(*threadData)->refCon = refCon;
				(*threadData)->FinishFunc = FinishFunc;
				if (pHTTPstuff)
					(*threadData)->HTTPstuff = *pHTTPstuff;
				INIT_UPP(DownloadURLThread,ThreadEntry);
				err = NewThread(kCooperativeThread, DownloadURLThreadUPP,(void *)threadData,0,kCreateIfNeeded,nil,&threadID);
				if (!err)
				{
					INIT_UPP(MyThreadTermination,ThreadTermination);
					SetThreadTerminator(threadID, MyThreadTerminationUPP, (void *)threadData);
					(*threadData)->threadID = threadID;
				}
			}
		}
	}
	else
		err = MemError();
	
	if (err)
	{
		ZapHandle(threadData);
		if (destFileRefNum)
		{
			FSClose(destFileRefNum);
			FSpDelete(destSpec);
		}
	}

	*pReference = (long)threadData;
	return err;
}

/************************************************************************
 * GetURLHost - get hostname and HTTP GET command strings
 ************************************************************************/
static OSErr GetURLHost(const char *urlString, char *hostName,char *httpMethodCommand, Boolean doPOST)
{
	size_t hostCharCount;
	Str255	scratch;
	const char *fullUrl = urlString;
	Boolean	useProxy = PrefIsSet(PREF_USE_HTTP_PROXY);

	*hostName = *httpMethodCommand = 0;

	// First check that the urlString begins with "http://"
	if (strscmp((UPtr)urlString, "http://"))
		return kNotHTTPURLErr;

	// Now skip over the "http://" and extract the host name.
	// Skip over the "http://".
	urlString += strlen("http://");
	
	// Ignore any username/password stuff
	if (strchr(urlString,'@')) urlString = strchr(urlString,'@')+1;
	
	// Count the characters before the next slash.
	hostCharCount = strcspn(urlString, "/");
	
	// Extract those characters from the URL into hostName
	//  and then make sure it's null terminated.
	strncpy(hostName, urlString, hostCharCount);
	hostName[hostCharCount] = 0;
	urlString += hostCharCount;

	// Now place the URL into the HTTP command that we send to DownloadHTTPSimple.
	strcpy(httpMethodCommand,doPOST ? "POST " : "GET ");
	strcat(httpMethodCommand,useProxy?fullUrl:urlString);
	if (*urlString == 0) strcat(httpMethodCommand,"/");
	strcat(httpMethodCommand," HTTP/1.0\r\nAccept: */*\r\nHost: ");
	strcat(httpMethodCommand,hostName);
	strcat(httpMethodCommand,"\r\n");
	
	if (useProxy)
	{
		//	Use HTTP proxy host
		GetPref(scratch,PREF_HTTP_PROXY_HOST);
		PtoCcpy(hostName,scratch);
	}
	
	// Add a ":80" to the host name if necessary.
	if ( strchr( hostName, ':' ) == nil )
		strcat( hostName, ":80" );

	return noErr;
}


/************************************************************************
 * MyThreadTermination - clean up when thread dies
 ************************************************************************/
static pascal void MyThreadTermination (ThreadID threadTerminated, void *terminationProcParam)
{
	URLParmsHandle	threadData;
	Boolean completed;
	
	threadData = (URLParmsHandle)terminationProcParam;
	completed = (*threadData)->completedDownload;
	
	if ((*threadData)->destFileRefNum)
	{
		TruncAtMark((*threadData)->destFileRefNum);
		FSClose((*threadData)->destFileRefNum);
	}
	
	if ((*threadData)->transferBuffer)
		ZapPtr((*threadData)->transferBuffer);

	if (!completed && !(*threadData)->err)
		//	Download not complete. Make sure we report an error to completion function.
		(*threadData)->err = -1;	//	Shouldn't need to do this, but let's make sure
	
	// the data pointed to by refCon is not guaranteed to be around after we aborted.
	if (!(*threadData)->aborted)
	{
		LDRef(threadData);
		(*(*threadData)->FinishFunc)((*threadData)->refCon,(*threadData)->err,&(*threadData)->info);
		UL(threadData);
	}
	
	DisposeHandle((Handle)threadData);	
}

/************************************************************************
 * DownloadURLThread - thread entry for URL download
 ************************************************************************/
static pascal void *DownloadURLThread (void *threadParameter)
{
	URLParmsHandle	threadData;
	char	hostName[256];
	char	httpMethodCommand[256];
	OSErr	err;
	ThreadID	threadID;
	Boolean	redirect;
	HTTPinfo	HTTPstuff;
	
	threadData = (URLParmsHandle)threadParameter;

	GetCurrentThread(&threadID);
	(*threadData)->threadID = threadID;

	if (!(*threadData)->transferBuffer)
		return nil;

	strcpy(hostName,(*threadData)->hostName);
	strcpy(httpMethodCommand,(*threadData)->httpMethodCommand);
	
	HTTPstuff = (*threadData)->HTTPstuff;				
	do
	{
		err = DownloadHTTPSimple(threadData,hostName,httpMethodCommand,&redirect,&HTTPstuff);
	} while (redirect);
	
	(*threadData)->err = err;
	return nil;
}

/************************************************************************
 * DownloadHTTPSimple - thread entry for URL download
 ************************************************************************/
static OSErr DownloadHTTPSimple(URLParmsHandle threadData,char *hostName,char *httpCommand, Boolean *redirect,HTTPinfo *pHTTPstuff)
	// Download a URL from the a web server.  hostName is a pointer
	// to a string that contains the DNS address of the web server.
	// The DNS address must be suffixed by ":<port>", where <port>
	// is the port number the web server is operating on.
	// httpCommand contains the HTTP command to send.  Typically this
	// is of the form:
	//
	//		GET <x> HTTP/1.0\0x13\0x10\0x13\0x10
	//
	// where <x> is the URL path.  destFileRefNum is the file
	// reference number to which the results of the HTTP command
	// are written.  This routine does not parse the returned HTTP
	// header in any way.  The entire incoming stream goes into
	// the file verbatim.
	//
	// For example, if you were asked to download a URL like:
	//
	//		http://devworld.apple.com/dev/technotes.shtml
	//
	// you would set:
	//
	// 		o hostName to "devworld.apple.com:80" (80 is the
	//		  default port for HTTP.
	//		o httpCommand to "GET /dev/technotes.shtml HTTP/1.0\0x13\0x10\0x13\0x10"
{
	TransStream stream = nil;
	OSStatus err = NewTransStream(&stream);
	Str255 pHost,sTemp;
	Str32	sShort;
	long port;
	OTResult bytesReceived;
	ResType type=nil,creator=nil;
	Boolean hdrDone = false;
	char urlString[256];
	unsigned char *p;
	TransVector netTrans =  GetTCPTrans();	// might as well use MacTCP calls if we're supposed to.
	unsigned char *transferBuffer = (*threadData)->transferBuffer;
	
	// error allocating the transstream
	if (err) return (err);
	
	// one of the parameters is missing
	if (!hostName || !httpCommand) return (paramErr);
	
	*redirect = false;
	
	// figure out the server and port from the hostname.
	WriteZero(pHost, sizeof(Str255));
	pHost[0] = MIN(strlen(hostName), 255);
	BlockMoveData(hostName,&pHost[1],pHost[0]);
	p = PIndex(pHost, ':');
	if (p) 
	{
		pHost[0] = p - pHost - 1;	// pHost now contains the hostname;
		p++;
		if (port = atoi(p));	// port is whatever's left over
		else port = 80;			// or the default http: port
	}
	
	// connect to the http server
	err = (*netTrans.vConnectTrans)(stream, pHost, port, true, GetRLong(OPEN_TIMEOUT));
#ifdef DEBUG
	if (pHTTPstuff->post) ComposeLogS(LOG_PLIST,nil,"\pPlayList Connect %d",err);
#endif
	if (err == noErr)
	{
		//	Set up HTTP headers (and request body)
		Accumulator	a;
		
		AccuInit(&a);
		//	HTTP command
		AccuAddPtr(&a,httpCommand,strlen(httpCommand));
		//	User-Agent header
		ComposeString(sTemp,"\pUser-Agent: Eudora/%d.%d.%db%d (MacOS)\r\n",MAJOR_VERSION,MINOR_VERSION,INC_VERSION,BUILD_VERSION);
		AccuAddStr(&a,sTemp);
		//	Content-Language header
		ComposeString(sTemp,"\pContent-Language: %p\r\n",GetLanguageCode(sShort));
		AccuAddStr(&a,sTemp);
		//	Content-Type header
		if (*pHTTPstuff->sContentType)
		{
			ComposeString(sTemp,"\p%r: %p\r\n",InterestHeadStrn+hContentType,pHTTPstuff->sContentType);
			AccuAddStr(&a,sTemp);
		}
		//	Content-Length header
		if (pHTTPstuff->hRequestData)
		{
			ComposeString(sTemp,"\pContent-Length: %d\r\n",GetHandleSize(pHTTPstuff->hRequestData));
			AccuAddStr(&a,sTemp);
		}
		//	MessageType header
		if (*pHTTPstuff->sMessageType)
		{
			ComposeString(sTemp,"\pMessageType: %p\r\n",pHTTPstuff->sMessageType);
			AccuAddStr(&a,sTemp);
		}
		//	Checksum header
		if (*pHTTPstuff->sCheckSum)
		{
			ComposeString(sTemp,"\pChecksum: %p\r\n",pHTTPstuff->sCheckSum);
			AccuAddStr(&a,sTemp);
		}
		//	End of headers
		AccuAddStr(&a,"\p\r\n");
		//	Body of request
		if (pHTTPstuff->hRequestData)
			AccuAddHandle(&a,pHTTPstuff->hRequestData);
		//	Send request
		err = (*netTrans.vSendTrans)(stream,LDRef(a.data),a.offset,nil);
		if (pHTTPstuff->post)
			CarefulLog(LOG_PLIST,LOG_SENT,*a.data,a.offset);
		AccuZap(a);

		if (err == noErr)
		{
			OSErr	OTErr = noErr;
			
			// receive the data comming back from the server.
			do 
			{
				bytesReceived = kTransferBufferSize;
				OTErr = (*netTrans.vRecvTrans)(stream, transferBuffer, &bytesReceived);
				if (OTErr == noErr)
				{
					// if we received some data, and have not aborted, write it to the cache file
					if ((bytesReceived > 0) && !((*threadData)->aborted))
					{
						Ptr	buffer;
					
						buffer = transferBuffer;
						if (!hdrDone)
						{
							err = ProcessHeader(&buffer,&bytesReceived,&type,&creator,threadData);
							if ((err==301 || err==302 || err==305) && ParseHTTPHeader("\pLocation:",urlString,transferBuffer,bytesReceived))
							{
								//	Error 301 "Moved Permamently", 302 "Moved Temporarily" or 305 "Use Proxy". Redirect to new URL
								if (!GetURLHost(urlString,hostName,httpCommand,pHTTPstuff->post))
									*redirect = true;
							}
							hdrDone = true;
						}
						if (!err && bytesReceived > 0)
						{
#ifdef DEBUG
							if (pHTTPstuff->post)
							{
								//	The only requested item that does a POST is playlist. We are having a problem
								//	with the playlist getting written to the wrong file. Let's do some verification here.
								FSSpec	playlistSpec;
								
								if (GetFileByRef((*threadData)->destFileRefNum,&playlistSpec))
									if (RunType!=Production) DebugStr("\pBad file ref for playlist.");
								CarefulLog(LOG_PLIST,LOG_GOT,buffer,bytesReceived);
							}
#endif						
							err = FSWrite((*threadData)->destFileRefNum, &bytesReceived, buffer);
						}
					} 
				}
		
			} while ((OTErr == noErr) && (err == noErr) && !((*threadData)->aborted));
		}
	}
	
	// Clean up.
	(*netTrans.vDisTrans)(stream);			// send a disconnect to the other end
	(*netTrans.vDestroyTrans)(stream);	// wait for the disconnect in the queue of closing connections
	ZapTransStream(&stream);						// free up all other memory used for the connection
	
	if (!*redirect)
	{
		FSSpec spec = (*threadData)->info.spec;

		// close and delete the file if we aborted.  Make sure caller resets (*thread)->destFileRefNum!
		if (err || (*threadData)->aborted)
		{
			FSClose((*threadData)->destFileRefNum);
			FSpDelete(&spec);
			(*threadData)->destFileRefNum = 0;
		}
		else
		{
			// Tweak the file type of the new cache file
			if (type || creator) 
				TweakFileType(&spec,type,creator);
			(*threadData)->completedDownload = true;	
		}
	}
	
	ZapHandle(pHTTPstuff->hRequestData);
	
	return (err);
}

/************************************************************************
 * ProcessHeader - get past HTTP header, look for Content-Type header
 ************************************************************************/
static OSErr ProcessHeader(Ptr *transferBuffer,long *bytesReceived,ResType *type,ResType *creator,URLParmsHandle threadData)
{
	Str255	s;
	long	offset;
	Ptr		p = *transferBuffer;
	Ptr		pTemp;
	MIMEMap	mm;
	Str32	sContType,sContSubType;
	OSErr	err = noErr;
	Str32	sErr;
	long	errNum;
	
	//	look for error code
	if (!strscmp(p, "HTTP/"))
	{
		pTemp = strchr(p,' ');
		if (pTemp < strchr(p,'\n') || pTemp < strchr(p,'\r'))
		{
			if (isdigit(sErr[1] = pTemp[1]) && isdigit(sErr[2] = pTemp[2]) && isdigit(sErr[3] = pTemp[3]))
			{
				sErr[0] = 3;
				StringToNum(sErr,&errNum);
				if (errNum != 200)	//	200 = OK
					return errNum;	//	HTTP error
			}
		}
	}
	
	LDRef(threadData);
	//	search for Content-Type
	GetRString(s,InterestHeadStrn+hContentType);
	if (ParseHTTPHeader(s,s,p,*bytesReceived))
		//	get filetype and creator
		if (pTemp = strchr(s,'/'))
		{
			MakePStr(sContType,s,pTemp-s);
			MakePStr(sContSubType,pTemp+1,strlen(pTemp+1));
			if (FindMIMEMapPtr(sContType,sContSubType,(*threadData)->info.spec.name,&mm))
			{
				*type = mm.type;
				*creator = mm.creator;
			}
		}
	
	//	search for checksum
	ParseHTTPHeader("\pChecksum",s,p,*bytesReceived);
	CtoPCpy((*threadData)->info.checksum,s);

	//	Search for end of header
	if ((offset = SearchPtrPtr("\r\n\r\n",4,p,0,*bytesReceived,true,false,nil))>=0)
	{
		*transferBuffer += offset+4;
		*bytesReceived -= offset+4;
	}
	else if ((offset = SearchPtrPtr("\n\n",2,p,0,*bytesReceived,true,false,nil))>=0 ||
		(offset = SearchPtrPtr("\r\r",2,p,0,*bytesReceived,true,false,nil))>=0)
	{
		*transferBuffer += offset+2;
		*bytesReceived -= offset+2;
	}
	UL(threadData);
	return err;
}

/************************************************************************
 * ParseHTTPHeader - get data for a header
 ************************************************************************/
static short ParseHTTPHeader(StringPtr sHeader,char *sResult,Ptr p,long bufLen)
{
	long	offset,end;
	short	len = 0;
	
	//	search for header
	if (offset = SearchPtrPtr(sHeader+1,*sHeader,p,0,bufLen,false,false,nil))
	{
		offset += *sHeader + 1;
		while (*(p+offset)==' ')	//	get past any spaces
			offset++;
		for(end = offset;*(p+end)!='\r'&&*(p+end)!='\n';end++);
		len = end-offset;
		if (len < 250)
		{
			//	make type/subtype into c-string
			BMD(p+offset,sResult,len);
			sResult[len] = 0;
		}
	}
	return len;
}

/************************************************************************
 * URLDownloadAbort - abort this download
 ************************************************************************/
void URLDownloadAbort(long urlRef)
{
	URLParmsHandle	threadData = (URLParmsHandle)urlRef;
	ThreadID threadID = (*threadData)->threadID;

	// tell the thread to cancel itself.
	(*threadData)->err = userCanceledErr;
	(*threadData)->aborted = true;
}

/************************************************************************
 * DownloadURLOk - return true if we're allow to start URL downloads
 ************************************************************************/
Boolean DownloadURLOK(void)
{
	Boolean result = false;
	
	// don't download if we're offline
	if (Offline);
	else
	{
		//
		// don't download if PPP is the selected mode of connection, and we're not (yet) connected
		if (CanCheckPPPState() && HaveTheDiseaseCalledOSX() ? PPPDown() : !PPPIsMostDefinitelyUpAndRunning());
		else
		{
			result = true;
		}
	}
	
	return (result);
}