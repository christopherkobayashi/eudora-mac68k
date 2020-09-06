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

#ifndef LDAPLIBGLUE_H
#define LDAPLIBGLUE_H


#include "lber.h"
#include "ldap.h"
#include "dflutils.h"
#include "dflsuppl.h"

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif



enum {
	AppEudoraLDAPLibCurVers = 0x01008000, /* current version of LDAP library used by app at time app was built */
	AppEudoraLDAPLibMinVers = 0x01008000 /* oldest version of LDAP library supported by app at time app was built */
};



#ifndef LDAP_USE_STD_LINK_MECHANISM



enum {
	kLDAPCallingConventionType = kThinkCStackBased
};
#define CALL_LDAP_VIA_UPP GENERATINGCFM
#if GENERATINGCFM
#define CallLDAPUniversalProc CallUniversalProc
#else
#define CallLDAPUniversalProc CallThinkCRegD0ResultUPP
#endif



/* for each routine */

#if GENERATING68KSEGLOAD
#pragma mpwc on
#endif


/******************************************************************************
 *
 * void GetEudoraLDAPLibVers(unsigned long *libVersion, unsigned long *libMinVersion)
 *
 * *libVersion and *libMinVersion are the same format as a NumVersion struct, as defined in Types.h
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(GetEudoraLDAPLibVers,	  kLDAPCallingConventionType
																							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(unsigned long*)))
																							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(unsigned long*))));
#if CALL_LDAP_VIA_UPP
#define GetEudoraLDAPLibVers(libVersion, libMinVersion) CallLDAPUniversalProc(GetEudoraLDAPLibVersUPP, uppGetEudoraLDAPLibVersProcInfo, (unsigned long*)(libVersion), (unsigned long*)(libMinVersion))
#else
typedef void (*GetEudoraLDAPLibVersProcPtr)(unsigned long *libVersion, unsigned long *libMinVersion);
#define GetEudoraLDAPLibVers(libVersion, libMinVersion) (*(GetEudoraLDAPLibVersProcPtr)GetEudoraLDAPLibVersUPP)((libVersion), (libMinVersion))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_count_entries( LDAP *ld, LDAPMessage *chain )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_count_entries,	kLDAPCallingConventionType
																					| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																					| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																					| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*))));
#if CALL_LDAP_VIA_UPP
#define ldap_count_entries(ld, chain) (int)CallLDAPUniversalProc(ldap_count_entriesUPP, uppldap_count_entriesProcInfo, (LDAP*)(ld), (LDAPMessage*)(chain))
#else
typedef int (*ldap_count_entriesProcPtr)(LDAP *ld, LDAPMessage *chain);
#define ldap_count_entries(ld, chain) (*(ldap_count_entriesProcPtr)ldap_count_entriesUPP)((ld), (chain))
#endif


/******************************************************************************
 *
 * LDAP_API(char *) LDAP_CALL ldap_first_attribute( LDAP *ld, LDAPMessage *entry, BerElement **ber )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_first_attribute,	kLDAPCallingConventionType
																						| RESULT_SIZE(SIZE_CODE(sizeof(char*)))
																						| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																						| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*)))
																						| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(BerElement**))));
#if CALL_LDAP_VIA_UPP
#define ldap_first_attribute(ld, entry, ber) (char*)CallLDAPUniversalProc(ldap_first_attributeUPP, uppldap_first_attributeProcInfo, (LDAP*)(ld), (LDAPMessage*)(entry), (BerElement**)(ber))
#else
typedef char* (*ldap_first_attributeProcPtr)(LDAP *ld, LDAPMessage *entry, BerElement **ber);
#define ldap_first_attribute(ld, entry, ber) (*(ldap_first_attributeProcPtr)ldap_first_attributeUPP)((ld), (entry), (ber))
#endif


/******************************************************************************
 *
 * LDAP_API(LDAPMessage *) LDAP_CALL ldap_first_entry( LDAP *ld, LDAPMessage *chain )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_first_entry,	kLDAPCallingConventionType
																				| RESULT_SIZE(SIZE_CODE(sizeof(LDAPMessage*)))
																				| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																				| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*))));
#if CALL_LDAP_VIA_UPP
#define ldap_first_entry(ld, chain) (LDAPMessage*)CallLDAPUniversalProc(ldap_first_entryUPP, uppldap_first_entryProcInfo, (LDAP*)(ld), (LDAPMessage*)(chain))
#else
typedef LDAPMessage* (*ldap_first_entryProcPtr)(LDAP *ld, LDAPMessage *chain);
#define ldap_first_entry(ld, chain) (*(ldap_first_entryProcPtr)ldap_first_entryUPP)((ld), (chain))
#endif


/******************************************************************************
 *
 * LDAP_API(char **) LDAP_CALL ldap_get_values( LDAP *ld, LDAPMessage *entry, const char *target )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_get_values,		kLDAPCallingConventionType
																				| RESULT_SIZE(SIZE_CODE(sizeof(char**)))
																				| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																				| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*)))
																				| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(const char*))));
#if CALL_LDAP_VIA_UPP
#define ldap_get_values(ld, entry, target) (char**)CallLDAPUniversalProc(ldap_get_valuesUPP, uppldap_get_valuesProcInfo, (LDAP*)(ld), (LDAPMessage*)(entry), (const char*)(target))
#else
typedef char** (*ldap_get_valuesProcPtr)(LDAP *ld, LDAPMessage *entry, const char *target);
#define ldap_get_values(ld, entry, target) (*(ldap_get_valuesProcPtr)ldap_get_valuesUPP)((ld), (entry), (target))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_msgfree( LDAPMessage *lm )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_msgfree,	kLDAPCallingConventionType
																		| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAPMessage*))));
#if CALL_LDAP_VIA_UPP
#define ldap_msgfree(lm) (int)CallLDAPUniversalProc(ldap_msgfreeUPP, uppldap_msgfreeProcInfo, (LDAPMessage*)(lm))
#else
typedef int (*ldap_msgfreeProcPtr)(LDAPMessage *lm);
#define ldap_msgfree(lm) (*(ldap_msgfreeProcPtr)ldap_msgfreeUPP)((lm))
#endif


/******************************************************************************
 *
 * LDAP_API(char *) LDAP_CALL ldap_next_attribute( LDAP *ld, LDAPMessage *entry, BerElement *ber )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_next_attribute,		kLDAPCallingConventionType
																						| RESULT_SIZE(SIZE_CODE(sizeof(char*)))
																						| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																						| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*)))
																						| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(BerElement*))));
#if CALL_LDAP_VIA_UPP
#define ldap_next_attribute(ld, entry, ber) (char*)CallLDAPUniversalProc(ldap_next_attributeUPP, uppldap_next_attributeProcInfo, (LDAP*)(ld), (LDAPMessage*)(entry), (BerElement*)(ber))
#else
typedef char* (*ldap_next_attributeProcPtr)(LDAP *ld, LDAPMessage *entry, BerElement *ber);
#define ldap_next_attribute(ld, entry, ber) (*(ldap_next_attributeProcPtr)ldap_next_attributeUPP)((ld), (entry), (ber))
#endif


/******************************************************************************
 *
 * LDAP_API(LDAPMessage *) LDAP_CALL ldap_next_entry( LDAP *ld, LDAPMessage *entry )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_next_entry,		kLDAPCallingConventionType
																				| RESULT_SIZE(SIZE_CODE(sizeof(LDAPMessage*)))
																				| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																				| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*))));
#if CALL_LDAP_VIA_UPP
#define ldap_next_entry(ld, entry) (LDAPMessage*)CallLDAPUniversalProc(ldap_next_entryUPP, uppldap_next_entryProcInfo, (LDAP*)(ld), (LDAPMessage*)(entry))
#else
typedef LDAPMessage* (*ldap_next_entryProcPtr)(LDAP *ld, LDAPMessage *entry);
#define ldap_next_entry(ld, entry) (*(ldap_next_entryProcPtr)ldap_next_entryUPP)((ld), (entry))
#endif


/******************************************************************************
 *
 * LDAP_API(LDAP *) LDAP_CALL ldap_open( const char *host, int port )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_open,		kLDAPCallingConventionType
																	| RESULT_SIZE(SIZE_CODE(sizeof(LDAP*)))
																	| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(const char*)))
																	| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(int))));
#if CALL_LDAP_VIA_UPP
#define ldap_open(host, port) (LDAP*)CallLDAPUniversalProc(ldap_openUPP, uppldap_openProcInfo, (const char*)(host), (int)(port))
#else
typedef LDAP* (*ldap_openProcPtr)(const char *host, int port);
#define ldap_open(host, port) (*(ldap_openProcPtr)ldap_openUPP)((host), (port))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_search_s( LDAP *ld, const char *base, int scope, const char *filter, char **attrs, int attrsonly, LDAPMessage **res )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_search_s,		kLDAPCallingConventionType
																			| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																			| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(const char*)))
																			| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(int)))
																			| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(const char*)))
																			| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(char**)))
																			| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(int)))
																			| STACK_ROUTINE_PARAMETER(7, SIZE_CODE(sizeof(LDAPMessage**))));
#if CALL_LDAP_VIA_UPP
#define ldap_search_s(ld, base, scope, filter, attrs, attrsonly, res) (int)CallLDAPUniversalProc(ldap_search_sUPP, uppldap_search_sProcInfo, (LDAP*)(ld), (const char*)(base), (int)(scope), (const char*)(filter), (char**)(attrs), (int)(attrsonly), (LDAPMessage**)(res))
#else
typedef int (*ldap_search_sProcPtr)(LDAP *ld, const char *base, int scope, const char *filter, char **attrs, int attrsonly, LDAPMessage **res);
#define ldap_search_s(ld, base, scope, filter, attrs, attrsonly, res) (*(ldap_search_sProcPtr)ldap_search_sUPP)((ld), (base), (scope), (filter), (attrs), (attrsonly), (res))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_search( LDAP *ld, const char *base, int scope, const char *filter, char **attrs, int attrsonly );
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_search,		kLDAPCallingConventionType
																		| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(const char*)))
																		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(const char*)))
																		| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(char**)))
																		| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(int))));
#if CALL_LDAP_VIA_UPP
#define ldap_search(ld, base, scope, filter, attrs, attrsonly) (int)CallLDAPUniversalProc(ldap_searchUPP, uppldap_searchProcInfo, (LDAP*)(ld), (const char*)(base), (int)(scope), (const char*)(filter), (char**)(attrs), (int)(attrsonly))
#else
typedef int (*ldap_searchProcPtr)(LDAP *ld, const char *base, int scope, const char *filter, char **attrs, int attrsonly);
#define ldap_search(ld, base, scope, filter, attrs, attrsonly) (*(ldap_searchProcPtr)ldap_searchUPP)((ld), (base), (scope), (filter), (attrs), (attrsonly))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_simple_bind_s( LDAP *ld, const char *who, const char *passwd )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_simple_bind_s,	kLDAPCallingConventionType
																					| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																					| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																					| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(const char*)))
																					| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(const char*))));
#if CALL_LDAP_VIA_UPP
#define ldap_simple_bind_s(ld, who, passwd) (int)CallLDAPUniversalProc(ldap_simple_bind_sUPP, uppldap_simple_bind_sProcInfo, (LDAP*)(ld), (const char*)(who), (const char*)(passwd))
#else
typedef int (*ldap_simple_bind_sProcPtr)(LDAP *ld, const char *who, const char *passwd);
#define ldap_simple_bind_s(ld, who, passwd) (*(ldap_simple_bind_sProcPtr)ldap_simple_bind_sUPP)((ld), (who), (passwd))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_simple_bind( LDAP *ld, const char *who, const char *passwd );
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_simple_bind,	kLDAPCallingConventionType
																				| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																				| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																				| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(const char*)))
																				| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(const char*))));
#if CALL_LDAP_VIA_UPP
#define ldap_simple_bind(ld, who, passwd) (int)CallLDAPUniversalProc(ldap_simple_bindUPP, uppldap_simple_bindProcInfo, (LDAP*)(ld), (const char*)(who), (const char*)(passwd))
#else
typedef int (*ldap_simple_bindProcPtr)(LDAP *ld, const char *who, const char *passwd);
#define ldap_simple_bind(ld, who, passwd) (*(ldap_simple_bindProcPtr)ldap_simple_bindUPP)((ld), (who), (passwd))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_unbind( LDAP *ld )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_unbind,		kLDAPCallingConventionType
																		| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*))));
#if CALL_LDAP_VIA_UPP
#define ldap_unbind(ld) (int)CallLDAPUniversalProc(ldap_unbindUPP, uppldap_unbindProcInfo, (LDAP*)(ld))
#else
typedef int (*ldap_unbindProcPtr)(LDAP *ld);
#define ldap_unbind(ld) (*(ldap_unbindProcPtr)ldap_unbindUPP)((ld))
#endif


/******************************************************************************
 *
 * LDAP_API(void) LDAP_CALL ldap_value_free( char **vals )
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_value_free,		kLDAPCallingConventionType
																				| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(char**))));
#if CALL_LDAP_VIA_UPP
#define ldap_value_free(vals) CallLDAPUniversalProc(ldap_value_freeUPP, uppldap_value_freeProcInfo, (char**)(vals))
#else
typedef void (*ldap_value_freeProcPtr)(char **vals);
#define ldap_value_free(vals) (*(ldap_value_freeProcPtr)ldap_value_freeUPP)((vals))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_abandon( LDAP *ld, int msgid );
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_abandon,		kLDAPCallingConventionType
																			| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																			| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(int))));
#if CALL_LDAP_VIA_UPP
#define ldap_abandon(ld, msgid) (int)CallLDAPUniversalProc(ldap_abandonUPP, uppldap_abandonProcInfo, (LDAP*)(ld), (int)(msgid))
#else
typedef int (*ldap_abandonProcPtr)(LDAP *ld, int msgid);
#define ldap_abandon(ld, msgid) (*(ldap_abandonProcPtr)ldap_abandonUPP)((ld), (msgid))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_result( LDAP *ld, int msgid, int all, struct timeval *timeout, LDAPMessage **result );
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_result,		kLDAPCallingConventionType
																		| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(int)))
																		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(struct timeval*)))
																		| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(LDAPMessage**))));
#if CALL_LDAP_VIA_UPP
#define ldap_result(ld, msgid, all, timeout, result) (int)CallLDAPUniversalProc(ldap_resultUPP, uppldap_resultProcInfo, (LDAP*)(ld), (int)(msgid), (int)(all), (struct timeval*)(timeout), (LDAPMessage**)(result))
#else
typedef int (*ldap_resultProcPtr)(LDAP *ld, int msgid, int all, struct timeval *timeout, LDAPMessage **result);
#define ldap_result(ld, msgid, all, timeout, result) (*(ldap_resultProcPtr)ldap_resultUPP)((ld), (msgid), (all), (timeout), (result))
#endif


/******************************************************************************
 *
 * LDAP_API(int) LDAP_CALL ldap_result2error( LDAP *ld, LDAPMessage *r, int freeit );
 *
 ******************************************************************************/

DECLARE_DFR_EXTERN_UPP(ldap_result2error,		kLDAPCallingConventionType
																					| RESULT_SIZE(SIZE_CODE(sizeof(int)))
																					| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(LDAP*)))
																					| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LDAPMessage*)))
																					| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(int))));
#if CALL_LDAP_VIA_UPP
#define ldap_result2error(ld, r, freeit) (int)CallLDAPUniversalProc(ldap_result2errorUPP, uppldap_result2errorProcInfo, (LDAP*)(ld), (LDAPMessage*)(r), (int)(freeit))
#else
typedef int (*ldap_result2errorProcPtr)(LDAP *ld, LDAPMessage *r, int freeit);
#define ldap_result2error(ld, r, freeit) (*(ldap_result2errorProcPtr)ldap_result2errorUPP)((ld), (r), (freeit))
#endif


#if GENERATING68KSEGLOAD
#pragma mpwc reset
#endif



#else  // LDAP_USE_STD_LINK_MECHANISM


void GetEudoraLDAPLibVers(unsigned long *libVersion, unsigned long *libMinVersion);


#endif  // LDAP_USE_STD_LINK_MECHANISM



OSErr LoadLDAPSharedLib(void);
OSErr UnloadLDAPSharedLib(void);



#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#endif
