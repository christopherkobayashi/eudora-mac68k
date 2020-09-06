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

#ifndef _LDAP_H
#define _LDAP_H

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if defined( _WINDOWS ) || defined( _WIN32 )
#include <winsock.h>
#elif defined(macintosh)
#include <utime.h>
#if !defined(FD_SET)
#define	NBBY	8
typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define FD_SETSIZE 64
typedef	struct fd_set{
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)		memset (p, 0, sizeof(*(p)))
#endif /* !FD_SET */
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef _AIX
#include <sys/select.h>
#endif /* _AIX */

#include "lber.h"

#define LDAP_PORT       389
#define LDAPS_PORT      636
#define LDAP_PORT_MAX	65535
#define LDAP_VERSION1   1
#define LDAP_VERSION2   2
#define LDAP_VERSION    LDAP_VERSION2

#define LDAP_OPT_DESC                   1
#define LDAP_OPT_DEREF                  2
#define LDAP_OPT_SIZELIMIT              3
#define LDAP_OPT_TIMELIMIT              4
#define LDAP_OPT_THREAD_FN_PTRS         5
#define LDAP_OPT_REBIND_FN              6
#define LDAP_OPT_REBIND_ARG             7
#define LDAP_OPT_REFERRALS              8
#define LDAP_OPT_RESTART                9
#define LDAP_OPT_SSL			10
#define LDAP_OPT_IO_FN_PTRS		11
#define LDAP_OPT_REFERRAL_HOP_LIMIT	16

/* for on/off options */
#define LDAP_OPT_ON     ((void *)1)
#define LDAP_OPT_OFF    ((void *)0)

extern int ldap_debug;

typedef struct ldap     LDAP;           /* opaque connection handle */
typedef struct ldapmsg  LDAPMessage;    /* opaque result/entry handle */

#define NULLMSG ((LDAPMessage *)0)

/* structure representing an LDAP modification */
typedef struct ldapmod {
	int             mod_op;         /* kind of mod + form of values*/
#define LDAP_MOD_ADD            0x00
#define LDAP_MOD_DELETE         0x01
#define LDAP_MOD_REPLACE        0x02
#define LDAP_MOD_BVALUES        0x80
	char            *mod_type;      /* attribute name to modify */
	union {
		char            **modv_strvals;
		struct berval   **modv_bvals;
	} mod_vals;                     /* values to add/delete/replace */
#define mod_values      mod_vals.modv_strvals
#define mod_bvalues     mod_vals.modv_bvals
} LDAPMod;

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
 * structure to hold thread function pointers
 */
struct ldap_thread_fns {
	void    *(LDAP_C LDAP_CALLBACK *ltf_mutex_alloc)( void );
	void    (LDAP_C LDAP_CALLBACK *ltf_mutex_free)( void * );
	int     (LDAP_C LDAP_CALLBACK *ltf_mutex_lock)( void * );
	int     (LDAP_C LDAP_CALLBACK *ltf_mutex_unlock)( void * );
	int     (LDAP_C LDAP_CALLBACK *ltf_get_errno)( void );
	void    (LDAP_C LDAP_CALLBACK *ltf_set_errno)( int );
	int     (LDAP_C LDAP_CALLBACK *ltf_get_lderrno)( char **, char **, 
				void * );
	void    (LDAP_C LDAP_CALLBACK *ltf_set_lderrno)( int, char *, char *, 
				void * );
	void    *ltf_lderrno_arg;
};

/*
 * structure to hold I/O function pointers
 */
struct ldap_io_fns {
	int	(LDAP_C LDAP_CALLBACK *liof_read)( LBER_SOCKET, void *, int );
	int	(LDAP_C LDAP_CALLBACK *liof_write)( LBER_SOCKET , const void *, int );
	int	(LDAP_C LDAP_CALLBACK *liof_select)( int, fd_set *, fd_set *, 
			fd_set *, struct timeval * );
	LBER_SOCKET	(LDAP_C LDAP_CALLBACK *liof_socket)( int, int, int );
	int	(LDAP_C LDAP_CALLBACK *liof_ioctl)( LBER_SOCKET, int, ... );
	int	(LDAP_C LDAP_CALLBACK *liof_connect)( LBER_SOCKET, void *, 
			int );
	int	(LDAP_C LDAP_CALLBACK *liof_close)( LBER_SOCKET );
	int	(LDAP_C LDAP_CALLBACK *liof_ssl_enable)( LBER_SOCKET );
};

/*
 * structure for ldap friendly mapping routines
 */

typedef struct friendly {
	char    *f_unfriendly;
	char    *f_friendly;
} *FriendlyMap;

/*
 * structures for ldap getfilter routines
 */

typedef struct ldap_filt_info {
	char                    *lfi_filter;
	char                    *lfi_desc;
	int                     lfi_scope;      /* LDAP_SCOPE_BASE, etc */
	int                     lfi_isexact;    /* exact match filter? */
	struct ldap_filt_info   *lfi_next;
} LDAPFiltInfo;

#define LDAP_FILT_MAXSIZ        1024

typedef struct ldap_filt_list LDAPFiltList; /* opaque filter list handle */
typedef struct ldap_filt_desc LDAPFiltDesc; /* opaque filter desc handle */

/*
 * types for ldap URL handling
 */
typedef struct ldap_url_desc {
    char                *lud_host;
    int                 lud_port;
    char                *lud_dn;
    char                **lud_attrs;
    int                 lud_scope;
    char                *lud_filter;
    unsigned long       lud_options;
#define LDAP_URL_OPT_SECURE     0x01
    char        *lud_string;    /* for internal use only */
} LDAPURLDesc;
#define NULLLDAPURLDESC ((LDAPURLDesc *)NULL)

/* Version reporting */
typedef struct _LDAPVersion {
	int                     sdk_version;    /* Version of the SDK, * 100 */
	int                     protocol_version; /* Highest protocol version
												 supported by the SDK, * 100 */
	int                     SSL_version;    /* SSL version if this SDK
											   version supports it, * 100 */
	int                     security_level; /* highest level available */
	int                     reserved[4];
} LDAPVersion;
#define LDAP_SECURITY_NONE      0

#define LDAP_URL_ERR_NOTLDAP    1       /* URL doesn't begin with "ldap://" */
#define LDAP_URL_ERR_NODN       2       /* URL has no DN (required) */
#define LDAP_URL_ERR_BADSCOPE   3       /* URL scope string is invalid */
#define LDAP_URL_ERR_MEM        4       /* can't allocate memory space */
#define LDAP_URL_ERR_PARAM	5	/* bad parameter to an URL function */

/* possible result types a server can return */
#define LDAP_RES_BIND                   0x61L
#define LDAP_RES_SEARCH_ENTRY           0x64L
#define LDAP_RES_SEARCH_RESULT          0x65L
#define LDAP_RES_MODIFY                 0x67L
#define LDAP_RES_ADD                    0x69L
#define LDAP_RES_DELETE                 0x6bL
#define LDAP_RES_MODRDN                 0x6dL
#define LDAP_RES_COMPARE                0x6fL
#define LDAP_RES_SESSION                0x72L
#define LDAP_RES_SEARCH_REFERENCE       0x73L
#define LDAP_RES_RESUME                 0x75L
#define LDAP_RES_EXTENDED               0x78L
#define LDAP_RES_ANY                    (-1L)

/* authentication methods available */
#define LDAP_AUTH_NONE          0x00L
#define LDAP_AUTH_SIMPLE        0x80L

/* search scopes */
#define LDAP_SCOPE_BASE         0x00
#define LDAP_SCOPE_ONELEVEL     0x01
#define LDAP_SCOPE_SUBTREE      0x02

/* alias dereferencing */
#define LDAP_DEREF_NEVER        0
#define LDAP_DEREF_SEARCHING    1
#define LDAP_DEREF_FINDING      2
#define LDAP_DEREF_ALWAYS       3

/* size/time limits */
#define LDAP_NO_LIMIT           0

/* possible error codes we can be returned */
#define LDAP_SUCCESS                    0x00
#define LDAP_OPERATIONS_ERROR           0x01
#define LDAP_PROTOCOL_ERROR             0x02
#define LDAP_TIMELIMIT_EXCEEDED         0x03
#define LDAP_SIZELIMIT_EXCEEDED         0x04
#define LDAP_COMPARE_FALSE              0x05
#define LDAP_COMPARE_TRUE               0x06
#define LDAP_STRONG_AUTH_NOT_SUPPORTED  0x07
#define LDAP_STRONG_AUTH_REQUIRED       0x08
#define LDAP_PARTIAL_RESULTS            0x09 /* non-standard extension to LDAP v2 */
#define LDAP_REFERRAL                   0x0a
#define LDAP_ADMIN_LIMIT_EXCEEDED       0x0b
#define LDAP_UNAVAILABLE_CRITICAL_EXTN  0x0c

#define LDAP_NO_SUCH_ATTRIBUTE          0x10
#define LDAP_UNDEFINED_TYPE             0x11
#define LDAP_INAPPROPRIATE_MATCHING     0x12
#define LDAP_CONSTRAINT_VIOLATION       0x13
#define LDAP_TYPE_OR_VALUE_EXISTS       0x14
#define LDAP_INVALID_SYNTAX             0x15

#define LDAP_NO_SUCH_OBJECT             0x20
#define LDAP_ALIAS_PROBLEM              0x21
#define LDAP_INVALID_DN_SYNTAX          0x22
#define LDAP_IS_LEAF                    0x23
#define LDAP_ALIAS_DEREF_PROBLEM        0x24

#define NAME_ERROR(n)   ((n & 0xf0) == 0x20)

#define LDAP_INAPPROPRIATE_AUTH         0x30
#define LDAP_INVALID_CREDENTIALS        0x31
#define LDAP_INSUFFICIENT_ACCESS        0x32
#define LDAP_BUSY                       0x33
#define LDAP_UNAVAILABLE                0x34
#define LDAP_UNWILLING_TO_PERFORM       0x35
#define LDAP_LOOP_DETECT                0x36

#define LDAP_NAMING_VIOLATION           0x40
#define LDAP_OBJECT_CLASS_VIOLATION     0x41
#define LDAP_NOT_ALLOWED_ON_NONLEAF     0x42
#define LDAP_NOT_ALLOWED_ON_RDN         0x43
#define LDAP_ALREADY_EXISTS             0x44
#define LDAP_NO_OBJECT_CLASS_MODS       0x45
#define LDAP_RESULTS_TOO_LARGE          0x46
#define LDAP_AFFECTS_MULTIPLE_DSAS      0x47

#define LDAP_OTHER                      0x50
#define LDAP_SERVER_DOWN                0x51
#define LDAP_LOCAL_ERROR                0x52
#define LDAP_ENCODING_ERROR             0x53
#define LDAP_DECODING_ERROR             0x54
#define LDAP_TIMEOUT                    0x55
#define LDAP_AUTH_UNKNOWN               0x56
#define LDAP_FILTER_ERROR               0x57
#define LDAP_USER_CANCELLED             0x58
#define LDAP_PARAM_ERROR                0x59
#define LDAP_NO_MEMORY                  0x5a
#define LDAP_CONNECT_ERROR              0x5b

/* function prototypes for ldap library */
#ifndef LDAP_API
#if defined( _WINDOWS ) || defined( _WIN32 )
#if defined( _WIN32 )
#define LDAP_API(rt) __declspec( dllexport ) rt
#else
#define LDAP_API(rt) rt
#endif
#else /* _WINDOWS */
#define LDAP_API
#endif /* _WINDOWS */
#endif /* LDAP_API */

typedef int (LDAP_CALL LDAP_CALLBACK LDAP_REBINDPROC_CALLBACK)( LDAP *ld, 
	char **dnp, char **passwdp, int *authmethodp, int freeit, void *arg);

typedef int (LDAP_C LDAP_CALLBACK LDAP_CMP_CALLBACK)(const char*, 
	const char*);

typedef int (LDAP_C LDAP_CALLBACK LDAP_CANCELPROC_CALLBACK)( void *cl );

LDAP_API(LDAP *) LDAP_CALL ldap_open( const char *host, int port );
LDAP_API(LDAP *) LDAP_CALL ldap_init( const char *defhost, int defport );
LDAP_API(int) LDAP_CALL ldap_set_option( LDAP *ld, int option, void *optdata );
LDAP_API(int) LDAP_CALL ldap_get_option( LDAP *ld, int option, void *optdata );
LDAP_API(int) LDAP_CALL ldap_unbind( LDAP *ld );
LDAP_API(int) LDAP_CALL ldap_unbind_s( LDAP *ld );
LDAP_API(int) LDAP_CALL ldap_version( LDAPVersion *ver );

/*
 * perform ldap operations and obtain results
 */
LDAP_API(int) LDAP_CALL ldap_abandon( LDAP *ld, int msgid );
LDAP_API(int) LDAP_CALL ldap_add( LDAP *ld, const char *dn, LDAPMod **attrs );
LDAP_API(int) LDAP_CALL ldap_add_s( LDAP *ld, const char *dn, LDAPMod **attrs );
LDAP_API(void) LDAP_CALL ldap_set_rebind_proc( LDAP *ld, 
	LDAP_REBINDPROC_CALLBACK *rebindproc, void *arg );
LDAP_API(int) LDAP_CALL ldap_simple_bind( LDAP *ld, const char *who,
	const char *passwd );
LDAP_API(int) LDAP_CALL ldap_simple_bind_s( LDAP *ld, const char *who,
	const char *passwd );
LDAP_API(int) LDAP_CALL ldap_modify( LDAP *ld, const char *dn, LDAPMod **mods );
LDAP_API(int) LDAP_CALL ldap_modify_s( LDAP *ld, const char *dn, 
	LDAPMod **mods );
LDAP_API(int) LDAP_CALL ldap_modrdn( LDAP *ld, const char *dn, 
	const char *newrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn_s( LDAP *ld, const char *dn, 
	const char *newrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn2( LDAP *ld, const char *dn, 
	const char *newrdn, int deleteoldrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn2_s( LDAP *ld, const char *dn, 
	const char *newrdn, int deleteoldrdn);
LDAP_API(int) LDAP_CALL ldap_compare( LDAP *ld, const char *dn,
	const char *attr, const char *value );
LDAP_API(int) LDAP_CALL ldap_compare_s( LDAP *ld, const char *dn, 
	const char *attr, const char *value );
LDAP_API(int) LDAP_CALL ldap_delete( LDAP *ld, const char *dn );
LDAP_API(int) LDAP_CALL ldap_delete_s( LDAP *ld, const char *dn );
LDAP_API(int) LDAP_CALL ldap_search( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly );
LDAP_API(int) LDAP_CALL ldap_search_s( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_search_st( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly,
	struct timeval *timeout, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_result( LDAP *ld, int msgid, int all,
	struct timeval *timeout, LDAPMessage **result );
LDAP_API(int) LDAP_CALL ldap_msgfree( LDAPMessage *lm );
LDAP_API(void) LDAP_CALL ldap_mods_free( LDAPMod **mods, int freemods );
LDAP_API(int) LDAP_CALL ldap_msgid( LDAPMessage *lm );
LDAP_API(int) LDAP_CALL ldap_msgtype( LDAPMessage *lm );

/*
 * parse/deal with results and errors returned
 */
LDAP_API(int) LDAP_CALL ldap_get_lderrno( LDAP *ld, char **m, char **s );
LDAP_API(int) LDAP_CALL ldap_set_lderrno( LDAP *ld, int e, char *m, char *s );
LDAP_API(int) LDAP_CALL ldap_result2error( LDAP *ld, LDAPMessage *r, 
	int freeit );
LDAP_API(char *) LDAP_CALL ldap_err2string( int err );
LDAP_API(void) LDAP_CALL ldap_perror( LDAP *ld, const char *s );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_first_entry( LDAP *ld, 
	LDAPMessage *chain );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_next_entry( LDAP *ld, 
	LDAPMessage *entry );
LDAP_API(int) LDAP_CALL ldap_count_entries( LDAP *ld, LDAPMessage *chain );
LDAP_API(char *) LDAP_CALL ldap_get_dn( LDAP *ld, LDAPMessage *entry );
LDAP_API(char *) LDAP_CALL ldap_dn2ufn( const char *dn );
LDAP_API(char **) LDAP_CALL ldap_explode_dn( const char *dn, 
	const int notypes );
LDAP_API(char **) LDAP_CALL ldap_explode_rdn( const char *dn, 
	const int notypes );
LDAP_API(char **) LDAP_CALL ldap_explode_dns( const char *dn );
LDAP_API(char *) LDAP_CALL ldap_first_attribute( LDAP *ld, LDAPMessage *entry,
	BerElement **ber );
LDAP_API(char *) LDAP_CALL ldap_next_attribute( LDAP *ld, LDAPMessage *entry,
	BerElement *ber );
LDAP_API(void) LDAP_CALL ldap_ber_free( BerElement *ber, int freebuf );
LDAP_API(char **) LDAP_CALL ldap_get_values( LDAP *ld, LDAPMessage *entry,
	const char *target );
LDAP_API(struct berval **) LDAP_CALL ldap_get_values_len( LDAP *ld,
	LDAPMessage *entry, const char *target );
LDAP_API(int) LDAP_CALL ldap_count_values( char **vals );
LDAP_API(int) LDAP_CALL ldap_count_values_len( struct berval **vals );
LDAP_API(void) LDAP_CALL ldap_value_free( char **vals );
LDAP_API(void) LDAP_CALL ldap_value_free_len( struct berval **vals );
LDAP_API(void) LDAP_CALL ldap_memfree( void *p );

/*
 * entry sorting routines
 */
LDAP_API(int) LDAP_CALL ldap_multisort_entries( LDAP *ld, LDAPMessage **chain,
	char **attr, LDAP_CMP_CALLBACK *cmp);
LDAP_API(int) LDAP_CALL ldap_sort_entries( LDAP *ld, LDAPMessage **chain, 
	char *attr, LDAP_CMP_CALLBACK *cmp);
LDAP_API(int) LDAP_CALL ldap_sort_values( LDAP *ld, char **vals, 
	LDAP_CMP_CALLBACK *cmp);
LDAP_API(int) LDAP_C LDAP_CALLBACK ldap_sort_strcasecmp( const char **a, 
	const char **b );

/*
 * getfilter routines
 */
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_init_getfilter( char *fname );
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_init_getfilter_buf( char *buf, 
	long buflen );
LDAP_API(LDAPFiltInfo *) LDAP_CALL ldap_getfirstfilter( LDAPFiltDesc *lfdp,
	char *tagpat, char *value );
LDAP_API(LDAPFiltInfo *) LDAP_CALL ldap_getnextfilter( LDAPFiltDesc *lfdp );
LDAP_API(int) LDAP_CALL ldap_set_filter_additions( LDAPFiltDesc *lfdp, 
	char *prefix, char *suffix );
LDAP_API(int) LDAP_CALL ldap_create_filter( char *buf, unsigned long buflen,
	char *pattern, char *prefix, char *suffix, char *attr,
	char *value, char **valwords );
LDAP_API(void) LDAP_CALL ldap_getfilter_free( LDAPFiltDesc *lfdp );

/*
 * friendly routines
 */
LDAP_API(char *) LDAP_CALL ldap_friendly_name( char *filename, char *name,
	FriendlyMap *map );
LDAP_API(void) LDAP_CALL ldap_free_friendlymap( FriendlyMap *map );

/*
 * ldap url routines
 */
LDAP_API(int) LDAP_CALL ldap_is_ldap_url( char *url );
LDAP_API(int) LDAP_CALL ldap_url_parse( char *url, LDAPURLDesc **ludpp );
LDAP_API(void) LDAP_CALL ldap_free_urldesc( LDAPURLDesc *ludp );
LDAP_API(int) LDAP_CALL ldap_url_search( LDAP *ld, char *url, int attrsonly );
LDAP_API(int) LDAP_CALL ldap_url_search_s( LDAP *ld, char *url, int attrsonly,
	LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_url_search_st( LDAP *ld, char *url, int attrsonly,
	struct timeval *timeout, LDAPMessage **res );


/********** the functions, etc. below are unsupported at this time ***********/
#ifdef LDAP_DNS
#define LDAP_OPT_DNS			12
#endif
#define LDAP_OPT_CACHE_FN_PTRS          13
#define LDAP_OPT_CACHE_STRATEGY         14
#define LDAP_OPT_CACHE_ENABLE           15

/* cache strategies */
#define LDAP_CACHE_CHECK        	0
#define LDAP_CACHE_POPULATE     	1
#define LDAP_CACHE_LOCALDB      	2


struct ldap_cache_fns {
	void    *lcf_private;
	int     (LDAP_C LDAP_CALLBACK *lcf_bind)( LDAP *, int, unsigned long, 
				const char *, struct berval *, int );
	int     (LDAP_C LDAP_CALLBACK *lcf_unbind)( LDAP *, int, 
				unsigned long );
	int     (LDAP_C LDAP_CALLBACK *lcf_search)( LDAP *, int, unsigned long, 
				const char *, int, const char LDAP_CALLBACK *, char **, int );
	int     (LDAP_C LDAP_CALLBACK *lcf_compare)( LDAP *, int, 
				unsigned long, const char *, const char *, struct berval * );
	int     (LDAP_C LDAP_CALLBACK *lcf_add)( LDAP *, int, unsigned long, 
				const char *, LDAPMod ** );
	int     (LDAP_C LDAP_CALLBACK *lcf_delete)( LDAP *, int, unsigned long, 
				const char * );
	int     (LDAP_C LDAP_CALLBACK *lcf_modify)( LDAP *, int, unsigned long, 
				const char *, LDAPMod ** );
	int     (LDAP_C LDAP_CALLBACK *lcf_modrdn)( LDAP *, int, unsigned long, 
				const char *, const char *, int );
	int     (LDAP_C LDAP_CALLBACK *lcf_result)( LDAP *, int, int, 
				struct timeval *, LDAPMessage ** );
	int     (LDAP_C LDAP_CALLBACK *lcf_flush)( LDAP *, const char *, 
				const char * );
};

/*
 * generalized bind
 */
LDAP_API(int) LDAP_CALL ldap_bind( LDAP *ld, const char *who, 
	const char *passwd, int authmethod );
LDAP_API(int) LDAP_CALL ldap_bind_s( LDAP *ld, const char *who, 
	const char *cred, int method );

/*
 * experimental DN format support
 */
LDAP_API(int) LDAP_CALL ldap_is_dns_dn( const char *dn );

/*
 * cacheing routines
 */
LDAP_API(int) LDAP_CALL ldap_cache_flush( LDAP *ld, const char *dn, 
	const char *filter );

/*
 * user friendly naming/searching routines
 */
LDAP_API(int) LDAP_CALL ldap_ufn_search_c( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res,
	LDAP_CANCELPROC_CALLBACK *cancelproc, void *cancelparm );
LDAP_API(int) LDAP_CALL ldap_ufn_search_ct( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res,
	LDAP_CANCELPROC_CALLBACK *cancelproc, void *cancelparm, 
	char *tag1, char *tag2, char *tag3 );
LDAP_API(int) LDAP_CALL ldap_ufn_search_s( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res );
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_ufn_setfilter( LDAP *ld, char *fname );
LDAP_API(void) LDAP_CALL ldap_ufn_setprefix( LDAP *ld, char *prefix );
LDAP_API(int) LDAP_C ldap_ufn_timeout( void *tvparam );

/*
 * utility routines
 */
LDAP_API(int) LDAP_CALL ldap_charray_add( char ***a, char *s );
LDAP_API(int) LDAP_CALL ldap_charray_merge( char ***a, char **s );
LDAP_API(void) LDAP_CALL ldap_charray_free( char **array );
LDAP_API(int) LDAP_CALL ldap_charray_inlist( char **a, char *s );
LDAP_API(char **) LDAP_CALL ldap_charray_dup( char **a );
LDAP_API(char **) LDAP_CALL ldap_str2charray( char *str, char *brkstr );
LDAP_API(int) LDAP_CALL ldap_charray_position( char **a, char *s );

/*
 * functions that have been replaced by new improved ones
 */
/* use ldap_create_filter() instead of ldap_build_filter() */
LDAP_API(void) LDAP_CALL ldap_build_filter( char *buf, unsigned long buflen,
	char *pattern, char *prefix, char *suffix, char *attr,
	char *value, char **valwords );
/* use ldap_set_filter_additions() instead of ldap_setfilteraffixes() */
LDAP_API(void) LDAP_CALL ldap_setfilteraffixes( LDAPFiltDesc *lfdp, 
	char *prefix, char *suffix );
/*********************** end of unsupported functions ************************/

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif
#endif /* _LDAP_H */
