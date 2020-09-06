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

#if 0
#define DEBUGASSERT 0
#define DEBUGERR 0
#define DEBUGMESSAGE 0
#define DEBUGDATA 0
#include "ssl.h"
#define MAXSSLPOINTERS 32000
#endif

#include "OpenSSL.h"
#include "myssl.h"

#if 0
#ifndef _SSLCERTS_H_
#include "sslCerts.h"
#endif
#endif

/* NO MORE "relaxed pointer rules"!!! */
#pragma mpwc_relax off

//#define SSL_MEM_CHECK

OSStatus ESSLInit();
OSErr ESSLConnectTrans(TransStream stream, UPtr serverName, long port,Boolean silently,uLong timeout);
OSErr ESSLSendTrans(TransStream stream, UPtr text,long size, ...);
OSErr ESSLReceiveTrans(TransStream stream, UPtr line,long *size);
OSErr ESSLDisTrans(TransStream stream);
OSErr ESSLDestroyTrans(TransStream stream);
OSErr ESSLTransErr(TransStream stream);
void ESSLSilenceTrans(TransStream stream, Boolean silence);
unsigned char * ESSLWhoAmI(TransStream stream, Uptr who);

int SSLVerifyCert (  int, X509_STORE_CTX * );

pascal long ESSLKCCallback(KCEvent keychainEvent, KCCallbackInfo* eventInfo,void *userContext);
// OSStatus ESSLZapContext(SSLContext **context);
OSStatus ESSLStartSSLLo(TransStream stream);

Boolean gKeychainChanged = false, gKeychainProcInstalled = false;
TransVector ESSLSubTrans;
TransVector ESSLTrans = {ESSLConnectTrans,ESSLSendTrans,ESSLReceiveTrans,ESSLDisTrans,ESSLDestroyTrans,ESSLTransErr,ESSLSilenceTrans,nil,ESSLWhoAmI,NetRecvLine,nil};

TransVector ESSLSetupVector(TransVector theTrans)
{
	ESSLSubTrans = theTrans;
	return ESSLTrans;
}

/* Handshakes SSL. Disposes of the SSL context if it fails. */
OSStatus ESSLStartSSLLo(TransStream stream)
{
	OSStatus err = noErr;
	Boolean bContinue = true;
	Boolean bSuccess = false;
	int		hsErr;
	
//	If we've already started the SSL stuff, then we're done
	if(stream->ESSLSetting & esslSSLInUse)
		return noErr;

//	Load the cert for the server
//	stream->serverName is what we want.
//	(void) OpenSSLReadCerts ( stream->ssl, stream->serverName );
	
//	Do the handhake
	while ( !bSuccess && bContinue ) {
	hsErr = SSL_do_handshake ( stream->ssl );
	if ( hsErr == 1 )		// Handshake succeeded.
		bSuccess = true;
	else if ( hsErr == 0 )	// Handshake failed but shutdown was clean.
		bContinue = false;
	else {					// Handshake experienced temporary or fatal error.
		ASSERT ( hsErr < 0 );

		switch ( SSL_get_error ( stream->ssl, hsErr )) {
			case SSL_ERROR_NONE:
			// Shouldn't happen on iRet < 0.
				ASSERT(0);
				bContinue = false;
				break;
			
			case SSL_ERROR_ZERO_RETURN:
			case SSL_ERROR_SSL:
			case SSL_ERROR_SYSCALL:
			// Fatal error, stop trying.
				bContinue = false;
				break;
				
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_X509_LOOKUP:
			case SSL_ERROR_WANT_CONNECT:
			case SSL_ERROR_WANT_ACCEPT:
			// Call SSL_do_handshake() again.
				break;
			
			default:	// Huh? Log and try again
				ASSERT ( 0 );
				break;
			}
		}
	}

	ComposeLogS ( LOG_SSL, nil, "\pSSL_Handshake %d\r", hsErr );
 	if ( 1 == hsErr )	// "we hand shook successfully"
		stream->ESSLSetting |= esslSSLInUse;
	else
		err = stream->ESSLSetting & esslOptional ? noErr : paramErr;	// !!! need a better error on failure
	return err;
}

OSStatus ESSLStartSSL ( TransStream stream ) {
	OSStatus retVal = noErr;
	static volatile int SSLMutex = 0;
	
	while ( SSLMutex > 0 )
		MyYieldToAnyThread ();
	SSLMutex++;
	retVal = ESSLStartSSLLo ( stream );
	SSLMutex--;
	return retVal;
	}
	
/* Initializes the SSL global template */
OSStatus ESSLInit()
{
	OSStatus err;
	
	if(!gKeychainProcInstalled)
	{
		err = KCAddCallback(NewKCCallbackUPP(ESSLKCCallback), kAddKCEventMask|kDeleteKCEventMask|kDefaultChangedKCEventMask, &gKeychainChanged);
		if(err)
			return err;
		else
			gKeychainProcInstalled = true;
	}
	
	err = InitOpenSSL ();
	return err;
}

OSErr ESSLConnectTrans(TransStream stream, UPtr serverName, long port,Boolean silently,uLong timeout)
{
	OSStatus err = noErr;
	
	if(stream->ESSLSetting > 1)
	{
		if((!gKeychainChanged) || ((err = ESSLInit()) == noErr))
		{
			long versionOptions;
		
			if ( stream->ESSLSetting & esslUseAltPort )
				versionOptions = GetRLong ( SSL_VERSION_ALT_PORT );
			else
				versionOptions = GetRLong ( SSL_VERSION_STD_PORT );
					
		//	Set up the SSL stuff
			err = SetupSSLConnection ( stream, versionOptions, SSLVerifyCert );

		if(err && !(stream->ESSLSetting & esslOptional))
			return err;
		}
	}
	
	/* Connect to the remote host */
	err = (*ESSLSubTrans.vConnectTrans)(stream, serverName, port, silently, timeout);
	if(err)
		; //	!!! ESSLZapContext(&stream->ESSLContext);
	else
	{
		/* That looks like a convenient place! */
		stream->port = port;
		PtoCcpy(stream->serverName, serverName);
		
		/* Handshake now or wait until later? */
		if( stream->ESSLSetting & esslUseAltPort )
			err = ESSLStartSSL(stream);
	}
	
	if (err) ComposeLogS(LOG_ALRT,nil,"\p%p: %d",SSL_ERR_STRING,err);
	
	return err;
}

OSErr ESSLSendTrans(TransStream stream, UPtr text,long size, ...)
{
	OSErr err = noErr;
	va_list extra_buffers;
	
	if (CommandPeriod) return(userCancelled);
			
	if (size==0) 
		return(noErr); 	// allow vacuous sends

	/* Loop through the buffers and send them */
	va_start(extra_buffers,size);
	
	do
	{
		ASSERT ( size != 0 );
		ASSERT ( text != NULL );

		if(!(stream->ESSLSetting & esslSSLInUse))
		{
			/* If not doing SSL now, just send */
			err = (*ESSLSubTrans.vSendTrans)(stream, text, size, nil);
		}
		else
		{
			int bytesWritten;
		/*	Use SSL to write out the data, looping if not complete */
			do {
				bytesWritten = SSL_write ( stream->ssl, text, size );
				if ( bytesWritten != size )
					if ( SSL_get_error ( stream->ssl, bytesWritten ) != SSL_ERROR_WANT_WRITE ) {
						err = kOTFlowErr;
						break;
						}
				} while ( bytesWritten != size );
			
			if (LogLevel&LOG_SSL && !stream->streamErr && !err && size) CarefulLog(LOG_SSL,LOG_SENT,text,size);
		}
		text = va_arg(extra_buffers,UPtr);
		size = va_arg(extra_buffers,long);
	} while(!err && text);
	return err;
}

OSErr ESSLReceiveTrans(TransStream stream, UPtr line,long *size)
{
	OSErr err = noErr;
	if(!(stream->ESSLSetting & esslSSLInUse))
	{
		/* If not doing SSL now, just receive */
		return (*ESSLSubTrans.vRecvTrans)(stream, line, size);
	}
	else
	{
		int	bytesRead;

		stream->DontWait = true;	// read only what's on the wire now
		bytesRead = SSL_read ( stream->ssl, line, *size );
		if ( bytesRead > 0 )
			*size = bytesRead;
		else {	// no bytes moved
			*size = 0;
			err = stream->streamErr;
			if ( 0 == bytesRead )	// Some kinda error happened
				ASSERT ( noErr != err );
			else { 					// We're going to retry
				ASSERT ( -1 == bytesRead );
				if ( kOTNoDataErr == err )
					err = noErr;
				}
			}
		stream->DontWait = false;

		if (*size && !stream->streamErr && !err && LogLevel&LOG_SSL) CarefulLog(LOG_SSL,LOG_GOT,line,*size);	//log what we got ...
		ASSERT ( err <= noErr );
		return err;
	}
}

OSErr ESSLDisTrans(TransStream stream)
{
	return (*ESSLSubTrans.vDisTrans)(stream);
}

OSErr ESSLDestroyTrans(TransStream stream)
{
	(void) CleanupSSLConnection ( stream );
	stream->ESSLSetting = 0;
	return (*ESSLSubTrans.vDestroyTrans)(stream);
}

OSErr ESSLTransErr(TransStream stream)
{
	return (*ESSLSubTrans.vTransError)(stream);
}

void ESSLSilenceTrans(TransStream stream, Boolean silence)
{
	(*ESSLSubTrans.vSilenceTrans)(stream, silence);
}

unsigned char * ESSLWhoAmI(TransStream stream, Uptr who)
{
	return (*ESSLSubTrans.vWhoAmI)(stream, who);
}

pascal long ESSLKCCallback(KCEvent keychainEvent, KCCallbackInfo* eventInfo,void *userContext)
{
	*(Boolean *)userContext = true;
	return 0;
}


#define NID_commonName		13

//	Errors
#define		X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT		2
#define		X509_V_ERR_UNABLE_TO_GET_CRL			3
#define		X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE	4
#define		X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE	5
#define		X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY	6
#define		X509_V_ERR_CERT_SIGNATURE_FAILURE		7
#define		X509_V_ERR_CRL_SIGNATURE_FAILURE		8
#define		X509_V_ERR_CERT_NOT_YET_VALID			9	
#define		X509_V_ERR_CERT_HAS_EXPIRED			10
#define		X509_V_ERR_CRL_NOT_YET_VALID			11
#define		X509_V_ERR_CRL_HAS_EXPIRED			12
#define		X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD	13
#define		X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD	14
#define		X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD	15
#define		X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD	16
#define		X509_V_ERR_OUT_OF_MEM				17
#define		X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT		18
#define		X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN		19
#define		X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY	20
#define		X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE	21
#define		X509_V_ERR_CERT_CHAIN_TOO_LONG			22
#define		X509_V_ERR_CERT_REVOKED				23
#define		X509_V_ERR_INVALID_CA				24
#define		X509_V_ERR_PATH_LENGTH_EXCEEDED			25
#define		X509_V_ERR_INVALID_PURPOSE			26
#define		X509_V_ERR_CERT_UNTRUSTED			27
#define		X509_V_ERR_CERT_REJECTED			28
/* These are 'informational' when looking for issuer cert */
#define		X509_V_ERR_SUBJECT_ISSUER_MISMATCH		29
#define		X509_V_ERR_AKID_SKID_MISMATCH			30
#define		X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH		31
#define		X509_V_ERR_KEYUSAGE_NO_CERTSIGN			32

#define		X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER		33
#define		X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION		34

int SSLVerifyCert ( int preverify, X509_STORE_CTX *storeCtx ) {

	X509 *	err_cert = X509_STORE_CTX_get_current_cert	( storeCtx );
	int		err		 = X509_STORE_CTX_get_error			( storeCtx );
	int		depth	 = X509_STORE_CTX_get_error_depth	( storeCtx );

	if ( preverify )
		return 1;

//	If OpenSSL can't verify the cert and the cert is in the user's KC, we're good to go.
	if ( IsCertInKeychain ( err_cert ))
		return 1;

	ComposeLogS ( LOG_SSL, nil, "\pSSLVerifyCert(%d,xx) error = %d\r", preverify, err );

//	UNABLE_TO_GET_ISSUER_LOCALLY and SELF_SIGNED_CERT
	switch ( err ) {
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		//	I wonder what other errors we should handle this way??
			if ( noErr == ShowOpenSSLCertToUser ( err_cert )) {
				(void) AddOpenSSLCertToKeychain ( err_cert );
				preverify = 1;
				}
			break;
		
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_CERT_HAS_EXPIRED:
		//	For now, we let these slide
			preverify = 1;
			break;
		
		default:
			break;
		}
	
#if 0	//	Check the host name
	{
		char peer_CN[256];
		
		DumpCertStore ( * (X509_STORE **) storeCtx );
		X509_NAME_get_text_by_NID ( X509_get_subject_name ( err_cert ), NID_commonName, peer_CN, sizeof ( peer_CN ));
		printf ( "%s\n", peer_CN );
	}	
#endif

	
	return preverify;
//	return 1;	// it's ok!
	}

