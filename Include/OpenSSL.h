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

/*
	This is a header file that contains just the bits of OpenSSL that we 
	use in CFM Eudora. Since OpenSSL is Mach-O, we have to bridge it, and
	these are the routines that we bridge.
*/


#ifndef EUDORA_OPENSSL
#define	EUDORA_OPENSSL

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/bio.h>

	typedef struct ssl_st SSL;
	typedef struct ssl_ctx_st SSL_CTX;
	typedef struct x509_store_st X509_STORE;
	typedef struct x509_store_ctx_st X509_STORE_CTX;
	typedef struct ssl_method_st SSL_METHOD;
	typedef struct X509_name_st X509_NAME;
// typedef struct X509_st                               X509; CK

/*	Routines that will go in the header file */
	BIO_METHOD *BIO_s_otsocket(void);
	BIO *BIO_new_ot(TransStream ts, int close_flag);

	OSStatus InitOpenSSL(void);	// load the dylibs, etc
	extern OSStatus OpenSSLReadCerts(SSL_CTX * ctx,
					 StringPtr hostName);

	typedef int (*CertVerifyProc)(int, X509_STORE_CTX *);

	OSStatus SetupSSLConnection(TransStream ep, long method,
				    CertVerifyProc callback);
	OSStatus CleanupSSLConnection(TransStream ep);

	void DumpCertStore(X509_STORE * store);
	Boolean IsCertInKeychain(X509 * theCert);
	OSStatus ShowOpenSSLCertToUser(X509 * theCert);
	OSStatus AddOpenSSLCertToKeychain(X509 * theCert);
	Handle GetOpenSSLCertText(X509 * theCert);

//      Call through to the underlying library
	SSL *SSL_new(SSL_CTX * ctx);
	void SSL_free(SSL * ssl);
	void SSL_set_bio(SSL * ssl, BIO * rbio, BIO * wbio);

	SSL_METHOD *SSLv2_client_method(void);	/* SSLv2 */
	SSL_METHOD *SSLv3_client_method(void);	/* SSLv3 */
	SSL_METHOD *SSLv23_client_method(void);	/* SSLv3 but can rollback to v2 */
	SSL_METHOD *TLSv1_client_method(void);	/* TLSv1.0 */


	SSL_CTX *SSL_CTX_new(SSL_METHOD * meth);
	void SSL_CTX_free(SSL_CTX * ctx);


	void SSL_CTX_set_verify(SSL_CTX * ctx, int mode,
				CertVerifyProc callback);

	void SSL_set_connect_state(SSL * ssl);
	int SSL_do_handshake(SSL * ssl);
	int SSL_write(SSL * ssl, const void *buffer, int len);
	int SSL_read(SSL * ssl, void *buffer, int len);
	int SSL_get_error(SSL * ssl, int ret);

	X509 *X509_STORE_CTX_get_current_cert(X509_STORE_CTX * ctx);
	int X509_STORE_CTX_get_error_depth(X509_STORE_CTX * ctx);
	int X509_STORE_CTX_get_error(X509_STORE_CTX * ctx);

	X509_NAME *X509_get_subject_name(X509 * a);
	char *X509_NAME_oneline(X509_NAME * a, char *buf, int size);
	int X509_NAME_get_text_by_NID(X509_NAME * name, int nid, char *buf,
				      int len);
	int X509_STORE_add_cert(X509_STORE * store, X509 * x);
	X509_STORE *SSL_CTX_get_cert_store(SSL_CTX * ctx);

	enum {
		SSL_ERROR_NONE = 0,
		SSL_ERROR_SSL = 1,
		SSL_ERROR_WANT_READ = 2,
		SSL_ERROR_WANT_WRITE = 3,
		SSL_ERROR_WANT_X509_LOOKUP = 4,
		SSL_ERROR_SYSCALL = 5,
		SSL_ERROR_ZERO_RETURN = 6,
		SSL_ERROR_WANT_CONNECT = 7,
		SSL_ERROR_WANT_ACCEPT = 8,

		SSL_LAST_ENUM_ERROR_VALUE = 32000	// No comma
	};
#ifdef __cplusplus
}
#endif
#endif
