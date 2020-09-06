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

/* Copyright (c) 1999 by QUALCOMM Incorporated */
#define	FILE_NUM 120


/**********************************************************************
 *	imapauth.c
 *
 *	This file contains the functions needed to do IMAP authentication.
 *
 *
 *	Currently Supported:
 *		
 *		CRAM-MD5
 *		KERBEROS_V4
 *		GSSAPI (Kerberos V5)
 **********************************************************************/
#include "imapauth.h"

/**********************************************************************
 * CRAM-MD5
 *
 *	These routines taken straight from the windows code with only minor
 *	modifications.  Thanks JOK!
 **********************************************************************/
#pragma mark ~~~CRAM_MD5~~~

unsigned char * GetStamp(unsigned char * stamp,unsigned char * banner);
int produce_hmac(unsigned char* text,unsigned char* key ,char* digest);

/**********************************************************************
 * CramMD5Authenticator 
 *
 * Implement the CRAM-MD5 authenticator. This is called by imap_auth.
 * It uses "challenger" to get the latest challenge the challenger
 * ( the IMAP server in the case of IMAP), and "responnder" to send the 
 * response.
 *
 **********************************************************************/
long CramMD5Authenticator (authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s, unsigned long *trial, char *user)
{
	MAILSTREAM *stream = (MAILSTREAM *) s;
	char tmp[MAILTMPLEN];
	unsigned char szDigest[1024];
	unsigned long len;
	unsigned char *pChallenge = NULL;
	long ret = NIL;
	unsigned char dlen;
	unsigned char i;
	
	// Sanity:
	if (! (challenger && responder && mb && s && trial && user) )
	{
		ASSERT (0);
		return NIL;
	}

	//
	// This authenticator uses a login password. The caller manages the
	// number of allowed trials. Here, we simply, (and always), get the password
	// and do the CRAM-MD5 thing.
	//
	tmp[0] = 0;			/* get password in this */

	//
	// JOK - Call back to upper layers. to get user and password.
	//
	mm_login (mb, user, tmp, *trial);

	//
	// Must have both a user and a password.
	//
	if (! (tmp[0] && user[0]) )
	{
		//
		// user refused to give a password
		//

		// Don't allow any more trials.

		*trial = MAXLOGINTRIALS + 1;

		mm_log ("CRAM-MD5 authenticator aborted", IMAP_ERROR);
		return NIL;
	}

	//
	// Get the first ready response.
	//
	len = 0;
	pChallenge = (unsigned char *)(*challenger) (stream, &len);

	// Must have a challenge string.
	if ( NULL == pChallenge || len == 0)
		return NIL;

	//
	// Formulate our response. The password returned in "tmp"
	// is used as the key.
	// This returns the response in szDigest, starting at szDigest[1].
	// The size of the digest in returned in szDigest[0].
	//
	memset(szDigest, 0, sizeof(szDigest));

    produce_hmac( (unsigned char* )pChallenge, (unsigned char* )tmp, (char *)szDigest);

	if ( !*szDigest )
	{
		ASSERT (0);
		goto end;
	}

	// Formulate response in the for: user<sp>digest. 
	sprintf (tmp, "%s ",user);

	// So far we've just got text in "tmp".
	len = strlen (tmp);

	// szDigest is not necessarily text:
	dlen = szDigest[0];

	for (i = 0 ; i < dlen; i++)
		tmp[len + i] = szDigest[i + 1];

	// Total length.		
	len += dlen;

	// Increment trial
	(*trial)++;

	// Ok. Send the response.
	if (! (*responder) (stream, tmp, len ) )
	{
		ret = NIL;
		goto end;
	}

	// If we get here, we've done our bit.
	ret = 1;

end:
	// Cleanup!!

	if (pChallenge)
		fs_give ((void **) &pChallenge);
		
	return ret;
}

/************************************************************************
 * GetStamp - grab the timestamp out of a CRAM banner
 ************************************************************************/
unsigned char * 
GetStamp(unsigned char * stamp,unsigned char * banner)
{
	unsigned char * cp1,*cp2;
	int		len = 0;
	//*stamp = 0;
	
	//banner[*banner+1] = 0;
	
	if (cp1 = (unsigned char *)strchr((char *)banner,'<'))
	{
		if (cp2=(unsigned char *)strchr((char *)cp1+1,'>'))
		{
			len = cp2-cp1+1;
			strncpy((char *)stamp,(char *)cp1,len);
		} 
	}
	
	return (stamp);
}  

/************************************************************************
 * produce hmac using md5 - based on the acap stuff.
 *
 * The result is returned in "digest", starting at byte 1. The size of 
 * the digest is returned in digets[0] - yuck. This should change.
 ************************************************************************/
int produce_hmac(unsigned char* text, unsigned char* key, char* digest)
{
	MD5_CTX context;
    unsigned char stamp[255];

    unsigned char k_ipad[64];   
    unsigned char k_opad[64];    /* outer padding -  key XORd with opad */
    unsigned char tk[16];
    static char hex[]="0123456789abcdef";
    int i;
    int ntext=strlen((const char* )text);
    int nkey=strlen((const char* )key);   
    
    memset(stamp,0,sizeof(stamp));


    if (!*GetStamp((unsigned char *)stamp,(unsigned char *)text))
    {
	   	// Error_Notify(IDS_APOP_NO_BANNER_ERROR, -1);
	     return (FALSE);
	}
   
    text=stamp;
        
    if (nkey > 64) 
    {
         MD5_CTX      tctx;
         MD5Init(&tctx);
         MD5Update(&tctx,(unsigned char*) key, nkey);
         MD5Final(&tctx);
          
         key  = (unsigned char* )tk;
         nkey = 16;
    }

    memset( k_ipad,0, sizeof(k_ipad));
    memset( k_opad,0, sizeof(k_opad));
    memcpy( k_ipad,key, nkey);
    memcpy( k_opad,key,nkey);

        
    for (i=0; i<64; i++) 
    {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5C;
    }
            
    MD5Init(&context);                
    MD5Update(&context, k_ipad, 64);  
    MD5Update(&context, text, ntext); 
    MD5Final(&context);          
        

    MD5Init(&context);                
    MD5Update(&context, k_opad, 64);  
    MD5Update(&context, (unsigned char* )context.digest, 16); 
    MD5Final(&context);
        
  	for (i = 0; i < sizeof(context.digest); i++)
  	{
    	digest[2*i+1] = hex[(context.digest[i]>>4)&0xf];
	    digest[2*i+2] = hex[context.digest[i]&0xf];
    }

    digest[0] = 2 * sizeof(context.digest);
    
	return (TRUE);
}


/**********************************************************************
 * KERBEROS_V4
 *
 *	The algorithm for these routines taken from Windows.  Major mods
 *	required to work with Macintosh KClient.
 **********************************************************************/
#pragma mark ~~~KERBEROS_V4~~~

Boolean gIMAPAuthedKerberos = false;	// set to true once KClient is invoked.

// CKrbAuthData	- struct to hold info for IMAP KV4 authentication
typedef struct CKrbAuthData_ CKrbAuthData, *CKrbAuthPtr;
struct CKrbAuthData_
{
	authchallenge_t 	challenger;		// routine to receive challeng from the IMAP server
	authrespond_t		responder;		// routine to send response to the IMAP server
	MAILSTREAM*			mailstream;		// mailstream to authenticate

	KClientSessionInfo 	session;		// session information
	KClientKey 			clientKey;
	
	Str255				pService;
	Str255				pInstance;
	char				realm[REALM_SZ];
	Str255				pUser;
	Str255				pServerFQDN;
};

typedef void *(*authchallenge_t) (void *stream,unsigned long *len);
typedef char *(*authresponse_t) (void *challenge,unsigned long clen,unsigned long *rlen);
typedef struct mail_stream MAILSTREAM;

OSErr NewKrbV4(CKrbAuthPtr cKrbAuth, authchallenge_t challenger, authrespond_t responder, PStr fqdn, MAILSTREAM* s, PStr user);
void ZapKrbV4(CKrbAuthPtr cKrbAuth);
long KrbV4Authenticate(CKrbAuthPtr cKrbAuth);
Boolean KrbV4Stage0Challenge(CKrbAuthPtr cKrbAuth, unsigned long *ulRndNum);
Boolean KrbV4Stage1Response(CKrbAuthPtr cKrbAuth, unsigned long ulRndNum);
Boolean KrbV4Stage2Challenge(CKrbAuthPtr cKrbAuth, unsigned long ulRndNum);
OSErr KerbGetTicketForIMAP(PStr serviceName,PStr inHost,PStr realm,unsigned long rndNum,UHandle *ticket,CKrbAuthPtr cKrb4Data);
OSErr KerbGetCredentials(CKrbAuthPtr cKrbAuth, CREDENTIALS *credentials);

/***************************************************************************************
 * KrbV4Authenticator - Implement the Kerbv4 authenticator. This is called by imap_auth.
 ***************************************************************************************/
long KrbV4Authenticator (authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s,unsigned long *trial, char *user)
{
	MAILSTREAM *stream = (MAILSTREAM *) s;
	CKrbAuthData kerbData;
	long result;
	Str255 pUser, pHost;
	OSErr err = noErr;
	
	if (!challenger || !responder || !mb || !s || !user) return (0);
	
	// Get host and username
	GetPOPInfo(pUser, pHost);

	// Build a CKrb4 to do all the work.
	if ((err=NewKrbV4(&kerbData, challenger, responder, pHost, stream, pUser))!=noErr) 
	{
		WarnUser(NO_KERBEROS,err);
		return (false);
	}

	// remember that we started up Kerberos.  We might have to kill the beast later.
	UsedKerberos();

	// return the user name
	user = PtoCcpy(user, pUser);
	
	// do the work
	result = KrbV4Authenticate(&kerbData);
	
	// clean up
	ZapKrbV4(&kerbData);
	
	return (result);
}

/***************************************************************************************
 * KrbV4Authenticate - authenticate
 ***************************************************************************************/
long KrbV4Authenticate(CKrbAuthPtr cKrbAuth)
{
	unsigned long ulRndNum = 0;
		
	if (!KrbV4Stage0Challenge(cKrbAuth, &ulRndNum)) return (0);

	if (!KrbV4Stage1Response(cKrbAuth, ulRndNum)) return (0);

	if (!KrbV4Stage2Challenge(cKrbAuth, ulRndNum)) return (0);
	
	// successfully authenticated
	return (1);
}

/***************************************************************************************
 * NewKrbV4 - build a KrbV4 object to do the authentication work
 ***************************************************************************************/
OSErr NewKrbV4(CKrbAuthPtr cKrbAuth, authchallenge_t challenger, authrespond_t responder, PStr fqdn, MAILSTREAM* s, PStr user)
{
	OSErr err = fnfErr;
	Str255 serverFQDN;
	
	WriteZero(cKrbAuth, sizeof(CKrbAuthData));
	
	cKrbAuth->challenger = challenger;
	cKrbAuth->responder = responder;
	cKrbAuth->mailstream = s;
	PCopy(cKrbAuth->pUser,user);
	GetRString(cKrbAuth->pService,KERBEROS_IMAP_SERVICE);
	PCopy(cKrbAuth->pServerFQDN, fqdn);
	
	if (DESLibraryIsPresent())	// the DES library must be preset to do IMAP Kerberos V4 authentication
	{
		// start a new session
		err = KClientNewSessionCompat(&(cKrbAuth->session), 1/*lAddr*/, GetRLong(IMAP_PORT), 2/*fAddr*/, GetRLong(IMAP_PORT));
		if (err == noErr)
		{
			// figure out the server realm
			PtoCcpy(serverFQDN, cKrbAuth->pServerFQDN);
			err = KClientGetRealmDeprecated(serverFQDN, cKrbAuth->realm);
			
			// log in if we have to
			if (err == cKrbNotLoggedIn) err = KClientLoginCompat(&(cKrbAuth->session), &(cKrbAuth->clientKey));
			
			// grab the user name
			if (err = noErr) err = KClientGetSessionUserNameCompat(&(cKrbAuth->session), cKrbAuth->pUser, 0);
		}
	}
	
	return (err);
}

/***************************************************************************************
 * ZapKrbV4 - clean up after a kerb object
 ***************************************************************************************/
void ZapKrbV4(CKrbAuthPtr cKrbAuth)
{
	OSErr err = noErr;

	err = KClientDisposeSessionCompat(&(cKrbAuth->session));
	
	return;
}

/***************************************************************************************
 * Stage0Challenge - grab the first ready response and return it.  It should be a random 
 *	4-byte integer in network byte order that becomes the CRC check.
 ***************************************************************************************/
Boolean KrbV4Stage0Challenge(CKrbAuthPtr cKrbAuth, unsigned long *ulRndNum)
{
	unsigned long len = 0;
	unsigned char* challenge;

	// Must have had a MAILSTREAM and challenger ....
	if (!cKrbAuth->mailstream || !cKrbAuth->challenger) return (false);

	// grab the first ready response ...
	challenge = (unsigned char *)(*(cKrbAuth->challenger))(cKrbAuth->mailstream, &len);

	// Must have grabbed something ...
	if (!challenge || len != 4) return false;
	
	// save the number
	BMD(challenge, (void *)ulRndNum, 4);

	// Cleanup
	fs_give ((void **) &challenge);

	return true;
}

/***************************************************************************************
 * KrbV4Stage1Response - given the random number returned from the server in Stage0,
 *	contact the Kerberos library and build a ticket using that checksum.  Send the
 *	ticket to the server.
 ***************************************************************************************/
Boolean KrbV4Stage1Response(CKrbAuthPtr cKrbAuth, unsigned long ulRndNum)
{
	Boolean result = false;
	OSErr err = noErr;
	Handle hTicket = nil;
	KTEXT_ST *ticket;
	
	// Must have had a mailstream and responder
	if (!(cKrbAuth->mailstream) || !(cKrbAuth->responder)) return (false);
		
	// get the ticket from the Kerberos extension, using the random number we got	
	err = KerbGetTicketForIMAP(cKrbAuth->pService,cKrbAuth->pServerFQDN,cKrbAuth->realm,ulRndNum,&hTicket,cKrbAuth);
	if ((err == noErr) && (hTicket && *hTicket))
	{
		LDRef(hTicket);
		ticket = (KTEXT_ST*)*hTicket;

		// send the ticket data to the IMAP server.  I'm just following orders (RFC 1731)
		if (ticket->length!=0)
		{
			result = (*(cKrbAuth->responder))((void *)(cKrbAuth->mailstream), ticket->dat, ticket->length) != 0;
		}
		UL(hTicket);
	}
	
	// Cleanup
	ZapHandle(hTicket);
	
	return (result);
}

/***************************************************************************************
 * KrbV4Stage2Challenge - given the random number returned from the server in Stage0,
 *	contact the Kerberos library and build a ticket using that checksum.  Send the
 *	ticket to the server.
 ***************************************************************************************/
Boolean KrbV4Stage2Challenge(CKrbAuthPtr cKrbAuth, unsigned long ulRndNum)
{
	Boolean result = false;
	OSErr err = noErr;
	unsigned long len = 0;
	unsigned char *challenge = nil;
 	CREDENTIALS credentials;
	unsigned char sout[1024];
	unsigned char in [8];
	short i;
	unsigned long testnum;
	char userid[512];
	void *clientOut;
					
  	des_key_schedule keysched; 
  	
	// Must have had a mailstream and a challenger
	if (!(cKrbAuth->mailstream) || !(cKrbAuth->challenger)) return (false);

	//
	// Get the second ready response. Length is now 8 bytes.
	//
	
	challenge = (unsigned char *)(*(cKrbAuth->challenger)) (cKrbAuth->mailstream, &len);

	// Must have received an 8 byte challenge string from the server
	if (!challenge || len != 8) return (false);
	
	// make a copy of the server's challenge
	for (i=0; i<8; i++) in[i] = challenge[i];
	
	// we can now throw away the challenge
	fs_give ((void **)&challenge);
	challenge = NULL;
	   	
	//
	// check the server response
	//
	
	// get the credentials
	if (KerbGetCredentials(cKrbAuth, &credentials)!=noErr) return (false);
	
	// get the keyschedule
	des_key_sched(credentials.session, keysched);

	// verify data 1st 4 octets must be equal to chal+1
 	des_ecb_encrypt((des_cblock *)in, (des_cblock *)in, keysched, DES_DECRYPT);

    testnum = (in[0]*256*256*256)+(in[1]*256*256)+(in[2]*256)+in[3];
 	if (testnum != ulRndNum + 1) return (false);

	//
	// build an answer to send back to the server
	//
	//
    // According to RFC 1731, the final response from the client is composed of
    // 8 octets and the user name, encrypted.  The 8 octects are:
    // 
    // 1-4 are the original checksum
    // 5 is bitmask specifying selected protection mechanism
    // 6-8 max cipher text buffer size
    //

	BMD(&ulRndNum, sout, 4);
    sout[4] = 1; 	 // no protection mechanism at this time
    sout[5] = 0x00;  // max ciphertext buffer size
    sout[6] = 0x04;
    sout[7] = 0x00;

	// add the user name to the response
	PtoCcpy(userid, cKrbAuth->pUser);
    for (i = 0; i < (int) strlen(userid); i++) sout[8+i] = userid[i];    
    len = 8 + strlen(userid);

    // append 0 based octets so is multiple of 8 
    do
    {
		sout[len] = 0;
		len++;
    } while (len%8);
    sout[len] = 0;
    
    // encrypt the response
   	des_key_sched(credentials.session, keysched);
    des_pcbc_encrypt((des_cblock *)sout, (des_cblock *)sout, len - 1, keysched, (des_cblock *)credentials.session, DES_ENCRYPT);

    // Send this to IMAP. The caller will get the final response from IMAP to determine if the user was authenticated.
	if ((clientOut = fs_get(len))!=0)
	{
		BMD(sout,(char *)clientOut,len);
	
		// This gets sent finally to the server:
		result = (*(cKrbAuth->responder)) ((void *)(cKrbAuth->mailstream), (char *)clientOut, len) != 0;

		// Can now free this:
		//
		fs_give ((void **)&clientOut);
	}
		
	return (result);
}

/**********************************************************************
 * KrbGetTicket - get our ticket for the IMAP server
 **********************************************************************/
OSErr KerbGetTicketForIMAP(PStr serviceName,PStr inHost,CStr realm,unsigned long rndNum,UHandle *ticket,CKrbAuthPtr CKrbAuth)
{
	Str63 fmt;
	Str255 fullName;
	Str255 host;
	OSErr err;
	UPtr spot;
	struct hostInfo *hip, hi;
	Str255 uRealm;
	long ticketLength = 0;
	
	/*
	 * the best thing in the world is four million DNS calls
	 */
	if (err=GetHostByName(inHost,&hip)) return(err);
	if (err=GetHostByAddr(&hi,hip->addr[0])) return(err);
	CtoPCpy(host,hi.cname);
				
	/*
	 * build the service name
	 */
	spot = host+1; PToken(host,CKrbAuth->pInstance,&spot,".");
	EscapeChars(host,GetRString(fmt,KERBEROS_ESCAPES));
	EscapeChars(serviceName,GetRString(fmt,KERBEROS_ESCAPES));
	GetRString(fmt,IMAP_KERBEROS_SERVICE_FMT);	// note, the format is determined by RFC 1731.	
	CtoPCpy(uRealm, realm);
	utl_PlugParams(fmt,fullName,serviceName,host,uRealm,CKrbAuth->pInstance);
	
	/*
	 * save the actual host name used in the request for stage 2 later
	 */
	 
	spot = fullName+1; spot = PIndex(fullName, '.'); spot++;
	PToken(spot,CKrbAuth->pInstance,&spot,"@");	// save the host name for later
	
	/*
	 * call the driver
	 */ 
	 
	fullName[*fullName+1] = 0;

	ticketLength = GetRLong(KERBEROS_BSIZE);
	if (!(*ticket = NuHandle(ticketLength))) return(MemError());	
	LDRef(*ticket);	
	err = KClientGetTicketForServiceWithChecksumCompat(&(CKrbAuth->session),rndNum,fullName+1,**ticket,&ticketLength);
	UL(*ticket);
		
	/*
	 * adjust the ticket size
	 */
	if (err) ZapHandle(*ticket);
	else SetHandleSize(*ticket,ticketLength);
	 
	return(err);
}

/************************************************************************
 * KerbGetCredentials - get the credentials from the IMAP server.
 ************************************************************************/
OSErr KerbGetCredentials(CKrbAuthPtr cKrbAuth, CREDENTIALS *credentials)
{
	OSErr err = noErr;
	char uName[ANAME_SZ];
	char uInstance[INST_SZ];
	char uRealm[REALM_SZ];
	
	// fill in the credentials structure with server information
	WriteZero(credentials, sizeof(CREDENTIALS));
	PtoCcpy(credentials->service, cKrbAuth->pService);
	PtoCcpy(credentials->instance, cKrbAuth->pInstance);
	strcpy(credentials->realm, cKrbAuth->realm);
	
	// we'll leave instance blank.  Don't really know what it's for anyway.
	Zero(uInstance);
	
	// must pass the KClient routine c string user information
	if ((err=KClientGetSessionUserNameCompat(&(cKrbAuth->session), uName, 0)) == noErr)
	{
		// must also pass the user's realm
		if ((err=KClientGetLocalRealmDeprecated(uRealm))==noErr)
		{
			// get the credentials
			err = KClientGetCredentialsDeprecated(uName, uInstance, uRealm, credentials);
		}
	}
	return (err);
}

/************************************************************************
 * UsedKerberos - set the global flag when Kerberos gets used
 ************************************************************************/
 void UsedKerberos(void)
 {
 	gIMAPAuthedKerberos = true;
 }

/************************************************************************
 * KerberosWasUsed - return true if we should destroy the Kerberos
 *	ticket and/or user.
 ************************************************************************/
Boolean KerberosWasUsed(void)
{
	Boolean result = false;
	PersHandle oldPers = CurPers;
	
	// destroy Kerberos if any personality is set to do Kerberos authenticaion for POP
	for (CurPers = PersList; CurPers && !result; CurPers = (*CurPers)->next) result = PrefIsSet(PREF_KERBEROS);
	
	// destroy Kerberos if an IMAP personality authenticated with Kerberos
	if (!result)
	{
		if (IMAPExists())
		{
			result = gIMAPAuthedKerberos;
		}
	}
	CurPers = oldPers;
	return(result);
}

/**********************************************************************
 * GSSAPI
 *
 *	Kerberos V5 via GSSAPI via SASL.  Requires MIT Kfm 4.0.  Taken from 
 * Windows Eudora with major modifications.
 **********************************************************************/
#pragma mark ~~~GSSAPI~~~

#define AUTH_GSSAPI_P_NONE 1

// CGSSAPIAuthData	- struct to hold info for IMAP KV4 authentication
typedef struct CGSSAPIAuthData_ CGSSAPIAuthData, *CGSSAPIAuthDataPtr;
struct CGSSAPIAuthData_
{
	KClientSessionInfo	session;		// the KClient session
	authchallenge_t 	challenger;		// routine to receive challeng from the IMAP server
	authrespond_t		responder;		// routine to send response to the IMAP server
	MAILSTREAM*			mailstream;		// mailstream to authenticate
		
	char				cService[255];
	char 				cUser[255];
	char				cHost[255];
};

static gss_OID_desc oids[] = 
{
   {10, "\052\206\110\206\367\022\001\002\001\004"},
};
static gss_OID_desc * gss_nt_service_name = oids+0;


OSErr InitGSSAPIAuth(CGSSAPIAuthDataPtr cgssapiPtr, authchallenge_t challenger, authrespond_t responder, PStr fqdn, MAILSTREAM* s, PStr user);
long GSSAPIAuthenticate(CGSSAPIAuthDataPtr cgssapiPtr);

/***************************************************************************************
 * KrbV4Authenticator - Implement the GSSAPI authenticator. This is called by imap_auth.
 ***************************************************************************************/
long GssapiAuthenticator(authchallenge_t challenger, authrespond_t responder, NETMBX *mb, void *s,  unsigned long *trial, char *user)
{
	MAILSTREAM *stream = (MAILSTREAM *) s;
	long result = 0;
	Str255 pUser, pHost;
	OSErr err = noErr;
	CGSSAPIAuthData authData;
	
	if (!challenger || !responder || !mb || !s || !user) return (0);
	
	// Get host and username
	GetPOPInfo(pUser, pHost);

	// Build a CGSSAPIAuthData to do all the work.
	err = InitGSSAPIAuth(&authData, challenger, responder, pHost, stream, pUser);
	if (err == noErr)
	{		
		// return the user name
		user = PtoCcpy(user, pUser);

		// never retry
		*trial = 0;

		// do the work
		result = GSSAPIAuthenticate(&authData);
	}
	
	// Display an error message if KClient failed at some point
	if (err != noErr)
	{
		WarnUser(NO_KERBEROS,err);
	}
	
	return (result);
}

/***************************************************************************************
 * GSSAPIAuthenticate - authenticate
 ***************************************************************************************/
long GSSAPIAuthenticate(CGSSAPIAuthDataPtr cgssapiPtr)
{
	long		ret = NIL;
	char		tmp[MAILTMPLEN];
	OM_uint32	maj,min,mmaj,mmin;
	OM_uint32	mctx = 0;
	gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
	gss_buffer_desc chal,resp,buf;
	gss_name_t	crname = NIL;
	long		i;
	int			conf;
	gss_qop_t	qop;
	OM_uint32	result;
	Str255 pErrorStr;
	
    // Get initial (empty) challenge */
	//
	chal.value = (unsigned char *)(*cgssapiPtr->challenger)(cgssapiPtr->mailstream, (unsigned long *) &chal.length);

	if (chal.value)
	{
		// get initial (empty) challenge:
		//
		sprintf (tmp,"%s@%s", cgssapiPtr->cService, cgssapiPtr->cHost);

		buf.length = strlen ( (char *) (buf.value = tmp)) + 1;

				/* get service name */
		if (result = gss_import_name(&min,&buf,gss_nt_service_name,&crname)!=GSS_S_COMPLETE)
		{
			(*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);
		}
		else
		{
    	// Work around Kfm40 bug...
    	Kfm40Hack();
    	
			switch (maj =		/* get context */
				gss_init_sec_context (&min,GSS_C_NO_CREDENTIAL,&ctx,
				       crname,GSS_C_NO_OID,
				       GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
				       0,GSS_C_NO_CHANNEL_BINDINGS,
				       GSS_C_NO_BUFFER,NIL,&resp,NIL,NIL))
			{
			case GSS_S_CONTINUE_NEEDED:
				do
				{
					/* negotiate authentication */
					if (chal.value)
						fs_give ((void **) &chal.value);

					/* send response */
					i = (*cgssapiPtr->responder) (cgssapiPtr->mailstream, (char *) resp.value, resp.length);

					gss_release_buffer (&min,&resp);
				}
				while (i &&		/* get next challenge */
					(chal.value=(*cgssapiPtr->challenger)(cgssapiPtr->mailstream, (unsigned long *)&chal.length))&&
					(maj = gss_init_sec_context (&min,GSS_C_NO_CREDENTIAL,&ctx,
						crname,GSS_C_NO_OID,
						GSS_C_MUTUAL_FLAG|GSS_C_REPLAY_FLAG,
						0,GSS_C_NO_CHANNEL_BINDINGS,&chal,
						NIL,&resp,NIL,NIL) ==
						GSS_S_CONTINUE_NEEDED));


			case GSS_S_COMPLETE:
				if (chal.value)
				{
					fs_give ((void **) &chal.value);
					if (maj != GSS_S_COMPLETE)
						(*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);
				}

				/* get prot mechanisms and max size */
				if ((maj == GSS_S_COMPLETE) &&
						(*cgssapiPtr->responder) (cgssapiPtr->mailstream, resp.value ? (char *) resp.value : "", resp.length) &&
						(chal.value = (*cgssapiPtr->challenger) (cgssapiPtr->mailstream, (unsigned long *)&chal.length))&&
						(gss_unwrap (&min,ctx,&chal,&resp,&conf,&qop) == GSS_S_COMPLETE) &&
						(resp.length >= 4) && (*((char *) resp.value) & AUTH_GSSAPI_P_NONE))
				{
					/* make copy of flags and length */
					memcpy (tmp,resp.value,4);
					gss_release_buffer (&min,&resp);

					/* no session protection */
					tmp[0] = AUTH_GSSAPI_P_NONE;

					/* install user name */
					strcpy (tmp+4, cgssapiPtr->cUser);

					buf.value = tmp; buf.length = strlen (cgssapiPtr->cUser) + 4;

					/* successful negotiation */
					if (gss_wrap (&min,ctx,FALSE,qop,&buf,&conf,&resp) == GSS_S_COMPLETE)
					{
						if ((*cgssapiPtr->responder) (cgssapiPtr->mailstream, (char *) resp.value, resp.length)) ret = T;
						gss_release_buffer (&min,&resp);
					}
					else (*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);
				}

				/* flush final challenge */
				if (chal.value) fs_give ((void **) &chal.value);

				/* don't need context any more */
				gss_delete_sec_context (&min,&ctx,NIL);

				break;

			case GSS_S_CREDENTIALS_EXPIRED:

				if (chal.value) fs_give ((void **) &chal.value);

				pmm_log (GetRString(pErrorStr,GSSAPIErrorsStrn+kGSSAPICredExpiredError),IMAP_ERROR);
				
				(*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);

				break;

			case GSS_S_FAILURE:
				if (chal.value) fs_give ((void **) &chal.value);

				if (min == (OM_uint32) KRB5_FCC_NOFILE)
				{
					pmm_log (GetRString(pErrorStr,GSSAPIErrorsStrn+kGSSAPINoCredError),IMAP_ERROR);
				}
				else
					do switch (mmaj = gss_display_status (&mmin,min,GSS_C_MECH_CODE,
						 GSS_C_NULL_OID,&mctx,&resp))
					{
						case GSS_S_COMPLETE:
						case GSS_S_CONTINUE_NEEDED:
							ComposeRString(pErrorStr,GSSAPIErrorsStrn+kGSSAPIFailure,resp.value);
							pmm_log (pErrorStr,IMAP_ERROR);

							gss_release_buffer (&mmin,&resp);
					}
					while (mmaj == GSS_S_CONTINUE_NEEDED);

					(*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);

					break;

				default:			/* miscellaneous errors */

					if (chal.value) fs_give ((void **) &chal.value);

					do switch (mmaj = gss_display_status (&mmin,maj,GSS_C_GSS_CODE,
									GSS_C_NULL_OID,&mctx,&resp))
					{
						case GSS_S_COMPLETE:
						mctx = 0;

						case GSS_S_CONTINUE_NEEDED:
							ComposeRString(pErrorStr,GSSAPIErrorsStrn+kGSSAPIUnknownFailure,resp.value);
							pmm_log (pErrorStr,IMAP_ERROR);
							gss_release_buffer (&mmin,&resp);
					}
					while (mmaj == GSS_S_CONTINUE_NEEDED);

					do switch (mmaj = gss_display_status (&mmin,min,GSS_C_MECH_CODE,
							GSS_C_NULL_OID,&mctx,&resp))
					{
						case GSS_S_COMPLETE:
						case GSS_S_CONTINUE_NEEDED:
							ComposeRString(pErrorStr,GSSAPIErrorsStrn+kGSSAPIStatus,resp.value);
							pmm_log (pErrorStr,IMAP_ERROR);
							gss_release_buffer (&mmin,&resp);
					}
					while (mmaj == GSS_S_CONTINUE_NEEDED);

					(*cgssapiPtr->responder) (cgssapiPtr->mailstream, NIL, 0);

					break;
			}
		}

		/* finished with credentials name */
		if (crname) gss_release_name (&min, &crname);
	}
	return ret;			/* return status */
}

/***************************************************************************************
 *	KfM 4.0 Hack
 *
 *	There's a bug in the GSSAPI Library.  The credentials are stored as a global, which
 *	is maintained even after the user manually destroys tickets.  gss_init_sec_context()
 *	will fail with a nasty dialog under classic, fail quietly in OS X, and never allow
 *	the user to re-authenticate.
 *
 *	The workaround is to update that global with the following two calls. Thanks to the
 *	support team at MIT.
 *
 *	- jdboyd 041902
 ***************************************************************************************/
void Kfm40Hack(void)
{
	OM_uint32	hackmin, hacklifetime;
	gss_cred_id_t hackcred = GSS_C_NO_CREDENTIAL;
	gss_name_t	hackname = NIL;
	gss_cred_usage_t hackusage;
	gss_OID_set	hackoids = GSS_C_NO_OID;
			
	if (gss_inquire_cred(&hackmin,hackcred,&hackname,&hacklifetime,&hackusage,&hackoids) == GSS_S_COMPLETE) 
   		 	gss_release_cred(&hackmin,&hackcred);
}

/***************************************************************************************
 * NewKrbV4 - build a KrbV4 object to do the authentication work
 ***************************************************************************************/
OSErr InitGSSAPIAuth(CGSSAPIAuthDataPtr cgssapiPtr, authchallenge_t challenger, authrespond_t responder, PStr fqdn, MAILSTREAM* s, PStr user)
{
	OSErr err = noErr;
	Str255 pService;
	
	WriteZero(cgssapiPtr, sizeof(CGSSAPIAuthDataPtr));

	cgssapiPtr->challenger = challenger;
	cgssapiPtr->responder = responder;
	cgssapiPtr->mailstream = s;
	
	PtoCcpy(cgssapiPtr->cUser,user);
	GetRString(pService,KERBEROS_IMAP_SERVICE);
	PtoCcpy(cgssapiPtr->cService,pService);
	PtoCcpy(cgssapiPtr->cHost,fqdn);

	return (err);
}