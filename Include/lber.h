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

#ifndef _LBER_H
#define _LBER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note that LBER_ERROR and LBER_DEFAULT are values that can never appear
 * as valid BER tags, and so it is safe to use them to report errors.  In
 * fact, any tag for which the following is true is invalid:
 *     (( tag & 0x00000080 ) != 0 ) && (( tag & 0xFFFFFF00 ) != 0 )
 */
#define LBER_ERROR		0xffffffffL
#define LBER_DEFAULT		LBER_ERROR
#define LBER_END_OF_SEQORSET	0xfffffffeL

/* BER classes and mask */
#define LBER_CLASS_UNIVERSAL    0x00
#define LBER_CLASS_APPLICATION  0x40
#define LBER_CLASS_CONTEXT      0x80
#define LBER_CLASS_PRIVATE      0xc0
#define LBER_CLASS_MASK         0xc0

/* BER encoding type and mask */
#define LBER_PRIMITIVE          0x00
#define LBER_CONSTRUCTED        0x20
#define LBER_ENCODING_MASK      0x20

#define LBER_BIG_TAG_MASK       0x1f
#define LBER_MORE_TAG_MASK      0x80

/* general BER types we know about */
#define LBER_BOOLEAN		0x01L
#define LBER_INTEGER		0x02L
#define LBER_BITSTRING		0x03L
#define LBER_OCTETSTRING	0x04L
#define LBER_NULL		0x05L
#define LBER_ENUMERATED		0x0aL
#define LBER_SEQUENCE		0x30L
#define LBER_SET		0x31L

/* BerElement set/get options */
#define LBER_OPT_REMAINING_BYTES	0x01
#define LBER_OPT_TOTAL_BYTES		0x02
#define LBER_OPT_USE_DER		0x04
#define LBER_OPT_TRANSLATE_STRINGS	0x08

/* Sockbuf set/get options */
#define LBER_SOCKBUF_OPT_TO_FILE		0x001
#define LBER_SOCKBUF_OPT_TO_FILE_ONLY		0x002
#define LBER_SOCKBUF_OPT_MAX_INCOMING_SIZE	0x004
#define LBER_SOCKBUF_OPT_NO_READ_AHEAD		0x008
#define LBER_SOCKBUF_OPT_DESC			0x010
#define LBER_SOCKBUF_OPT_COPYDESC		0x020
#define LBER_SOCKBUF_OPT_READ_FN		0x040
#define LBER_SOCKBUF_OPT_WRITE_FN		0x080

#define LBER_OPT_ON	((void *) 1)
#define LBER_OPT_OFF	((void *) 0)

extern int lber_debug;

struct berval {
	unsigned long	bv_len;
	char		*bv_val;
};

typedef struct berelement BerElement;
typedef struct sockbuf Sockbuf;
typedef int (*BERTranslateProc)( char **bufp, unsigned long *buflenp,
	int free_input );
#ifndef macintosh
#if defined( _WINDOWS ) || defined( _WIN32 )
typedef SOCKET LBER_SOCKET;
#else
typedef int LBER_SOCKET;
#endif /* _WINDOWS */
#else /* macintosh */
typedef void *LBER_SOCKET;
#endif /* macintosh */

/* calling conventions used by library */
#ifndef LDAP_CALL
#if defined( _WINDOWS ) || defined( _WIN32 )
#define LDAP_C __cdecl
#ifndef _WIN32 
#define __stdcall _far _pascal
#define LDAP_CALLBACK _loadds
#else
#define LDAP_CALLBACK
#endif /* _WIN32 */
#define LDAP_PASCAL __stdcall
#define LDAP_CALL LDAP_PASCAL
#else /* _WINDOWS */
#define LDAP_C
#define LDAP_CALLBACK
#define LDAP_PASCAL
#define LDAP_CALL
#endif /* _WINDOWS */
#endif /* LDAP_CALL */

/*
 * function prototypes for lber library
 */

#ifndef LDAP_API
#if defined( _WINDOWS ) || defined( _WIN32 )
#if defined(_WIN32)
#define LDAP_API(rt) __declspec( dllexport ) rt
#else
#define LDAP_API(rt) rt
#endif
#else /* _WINDOWS */
#define LDAP_API(rt) rt
#endif /* _WINDOWS */
#endif /* LDAP_API */

/*
 * decode routines
 */
LDAP_API(unsigned long) LDAP_CALL ber_get_tag( BerElement *ber );
LDAP_API(unsigned long) LDAP_CALL ber_skip_tag( BerElement *ber, 
	unsigned long *len );
LDAP_API(unsigned long) LDAP_CALL ber_peek_tag( BerElement *ber, 
	unsigned long *len );
LDAP_API(unsigned long) LDAP_CALL ber_get_int( BerElement *ber, long *num );
LDAP_API(unsigned long) LDAP_CALL ber_get_stringb( BerElement *ber, char *buf,
	unsigned long *len );
LDAP_API(unsigned long) LDAP_CALL ber_get_stringa( BerElement *ber, 
	char **buf );
LDAP_API(unsigned long) LDAP_CALL ber_get_stringal( BerElement *ber,
	struct berval **bv );
LDAP_API(unsigned long) LDAP_CALL ber_get_bitstringa( BerElement *ber, 
	char **buf, unsigned long *len );
LDAP_API(unsigned long) LDAP_CALL ber_get_null( BerElement *ber );
LDAP_API(unsigned long) LDAP_CALL ber_get_boolean( BerElement *ber, 
	int *boolval );
LDAP_API(unsigned long) LDAP_CALL ber_first_element( BerElement *ber,
	unsigned long *len, char **last );
LDAP_API(unsigned long) LDAP_CALL ber_next_element( BerElement *ber,
	unsigned long *len, char *last );
LDAP_API(unsigned long) LDAP_C ber_scanf( BerElement *ber, char *fmt, ... );
LDAP_API(void) LDAP_CALL ber_bvfree( struct berval *bv );
LDAP_API(void) LDAP_CALL ber_bvecfree( struct berval **bv );
LDAP_API(struct berval *) LDAP_CALL ber_bvdup( struct berval *bv );
LDAP_API(void) LDAP_CALL ber_set_string_translators( BerElement *ber,
	BERTranslateProc encode_proc, BERTranslateProc decode_proc );

/*
 * encoding routines
 */
LDAP_API(int) LDAP_CALL ber_put_enum( BerElement *ber, long num, 
	unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_int( BerElement *ber, long num, 
	unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_ostring( BerElement *ber, char *str, 
	unsigned long len, unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_string( BerElement *ber, char *str,
	unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_bitstring( BerElement *ber, char *str,
	unsigned long bitlen, unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_null( BerElement *ber, unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_boolean( BerElement *ber, int boolval,
	unsigned long tag );
LDAP_API(int) LDAP_CALL ber_start_seq( BerElement *ber, unsigned long tag );
LDAP_API(int) LDAP_CALL ber_start_set( BerElement *ber, unsigned long tag );
LDAP_API(int) LDAP_CALL ber_put_seq( BerElement *ber );
LDAP_API(int) LDAP_CALL ber_put_set( BerElement *ber );
LDAP_API(int) LDAP_C ber_printf( BerElement *ber, char *fmt, ... );

/*
 * miscellaneous routines
 */
LDAP_API(void) LDAP_CALL ber_free( BerElement *ber, int freebuf );
LDAP_API(int) LDAP_CALL ber_flush( Sockbuf *sb, BerElement *ber, int freeit );
LDAP_API(BerElement*) LDAP_CALL ber_alloc( void );
LDAP_API(BerElement*) LDAP_CALL der_alloc( void );
LDAP_API(BerElement*) LDAP_CALL ber_alloc_t( int options );
LDAP_API(BerElement*) LDAP_CALL ber_dup( BerElement *ber );
LDAP_API(unsigned long) LDAP_CALL ber_get_next( Sockbuf *sb, unsigned long *len,
	BerElement *ber );
LDAP_API(long) LDAP_CALL ber_read( BerElement *ber, char *buf, 
	unsigned long len );
LDAP_API(long) LDAP_CALL ber_write( BerElement *ber, char *buf, 
	unsigned long len, int nosos );
LDAP_API(void) LDAP_CALL ber_init( BerElement *ber, int options );
LDAP_API(void) LDAP_CALL ber_reset( BerElement *ber, int was_writing );
LDAP_API(int) LDAP_CALL ber_set_option( BerElement *ber, int option, 
	void *value );
LDAP_API(int) LDAP_CALL ber_get_option( BerElement *ber, int option, 
	void *value );
LDAP_API(Sockbuf*) LDAP_CALL ber_sockbuf_alloc( void );
LDAP_API(int) LDAP_CALL ber_sockbuf_set_option( Sockbuf *sb, int option, 
	void *value );
LDAP_API(int) LDAP_CALL ber_sockbuf_get_option( Sockbuf *sb, int option, 
	void *value );

#ifdef __cplusplus
}
#endif
#endif /* _LBER_H */
