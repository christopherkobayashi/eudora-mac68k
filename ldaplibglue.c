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

#include "dflutils.h"
#include "dflsuppl.h"
#include "ldaplibglue.h"


#define FILE_NUM 111
#pragma segment LDAPUtils



#ifndef LDAP_USE_STD_LINK_MECHANISM
/* FINISH *//* do I need all of these */
static CFragConnectionID LDAPSharedLibConnID;
static Boolean LDAPSharedLibLoaded;
static OSErr LDAPSharedLibLoadErr;
#endif  // LDAP_USE_STD_LINK_MECHANISM



#ifndef LDAP_USE_STD_LINK_MECHANISM
/* for each routine */
DECLARE_DFR_UPP(GetEudoraLDAPLibVers);
DECLARE_DFR_UPP(ldap_count_entries);
DECLARE_DFR_UPP(ldap_first_attribute);
DECLARE_DFR_UPP(ldap_first_entry);
DECLARE_DFR_UPP(ldap_get_values);
DECLARE_DFR_UPP(ldap_msgfree);
DECLARE_DFR_UPP(ldap_next_attribute);
DECLARE_DFR_UPP(ldap_next_entry);
DECLARE_DFR_UPP(ldap_open);
DECLARE_DFR_UPP(ldap_search_s);
DECLARE_DFR_UPP(ldap_search);
DECLARE_DFR_UPP(ldap_simple_bind_s);
DECLARE_DFR_UPP(ldap_simple_bind);
DECLARE_DFR_UPP(ldap_unbind);
DECLARE_DFR_UPP(ldap_value_free);
DECLARE_DFR_UPP(ldap_abandon);
DECLARE_DFR_UPP(ldap_result);
DECLARE_DFR_UPP(ldap_result2error);
#endif  // LDAP_USE_STD_LINK_MECHANISM



OSErr LoadLDAPSharedLib(void)

{
#ifndef LDAP_USE_STD_LINK_MECHANISM

	OSErr			err, scratchErr;
	Str255		libName;
	OSType		libArchType;
	CFragConnectionID		connID;
	unsigned long				libVers, libMinVers;


	LDAPSharedLibLoadErr = noErr;

	GetRString(libName, LDAP_SHARED_LIB_TRUE_NAME);
	if (libName[0] > 63)
	{
		err = paramErr;
		goto Error;
	}
	libArchType = GetROSType(LDAP_SHARED_LIB_ARCH_TYPE);
	if (!(long)libArchType)
	{
		err = paramErr;
		goto Error;
	}

	err = LoadDFRLibrary(libName, libArchType, &connID, nil);
	if (err)
		goto Error;

	/* for each routine */
	LoadDFR(GetEudoraLDAPLibVers);
	LoadDFR(ldap_count_entries);
	LoadDFR(ldap_first_attribute);
	LoadDFR(ldap_first_entry);
	LoadDFR(ldap_get_values);
	LoadDFR(ldap_msgfree);
	LoadDFR(ldap_next_attribute);
	LoadDFR(ldap_next_entry);
	LoadDFR(ldap_open);
	LoadDFR(ldap_search_s);
	LoadDFR(ldap_search);
	LoadDFR(ldap_simple_bind_s);
	LoadDFR(ldap_simple_bind);
	LoadDFR(ldap_unbind);
	LoadDFR(ldap_value_free);
	LoadDFR(ldap_abandon);
	LoadDFR(ldap_result);
	LoadDFR(ldap_result2error);

	err = DFRLoaderErr();

	LDAPSharedLibConnID = connID;
	LDAPSharedLibLoadErr = err;
	LDAPSharedLibLoaded = err ? false : true;

	ResetDFRLoaderGlobals();

	if (err)
	{
		scratchErr = UnloadLDAPSharedLib();
		return err;
	}

	GetEudoraLDAPLibVers(&libVers, &libMinVers);
	if ((AppEudoraLDAPLibCurVers < libMinVers) || (libVers < AppEudoraLDAPLibMinVers))
	{
		scratchErr = UnloadLDAPSharedLib();
		err = kDFLBadLibVersErr;
		goto Error;
	}

	return noErr;


Error:
	LDAPSharedLibLoadErr = err;
	return err;

#else  // LDAP_USE_STD_LINK_MECHANISM

	OSErr						err;
	unsigned long		libVers, libMinVers;


	err = SystemSupportsDFRLibraries(kAnyCFragArch);
	if (err)
		return err;
	if ((long)ldap_open == kUnresolvedCFragSymbolAddress)
		return cfragNoSymbolErr;
	GetEudoraLDAPLibVers(&libVers, &libMinVers);
	if ((AppEudoraLDAPLibCurVers < libMinVers) || (libVers < AppEudoraLDAPLibMinVers))
		return kDFLBadLibVersErr;
	return noErr;

#endif  // LDAP_USE_STD_LINK_MECHANISM
}


OSErr UnloadLDAPSharedLib(void)

{
#ifndef LDAP_USE_STD_LINK_MECHANISM
	OSErr		err;


	/* call UnloadDFRLibrary before calling UnloadDFR for that library's functions */
	err = UnloadDFRLibrary(&LDAPSharedLibConnID);
	if (err)
		return err;

	/* for each routine */
	UnloadDFR(GetEudoraLDAPLibVers);
	UnloadDFR(ldap_count_entries);
	UnloadDFR(ldap_first_attribute);
	UnloadDFR(ldap_first_entry);
	UnloadDFR(ldap_get_values);
	UnloadDFR(ldap_msgfree);
	UnloadDFR(ldap_next_attribute);
	UnloadDFR(ldap_next_entry);
	UnloadDFR(ldap_open);
	UnloadDFR(ldap_search_s);
	UnloadDFR(ldap_search);
	UnloadDFR(ldap_simple_bind_s);
	UnloadDFR(ldap_simple_bind);
	UnloadDFR(ldap_unbind);
	UnloadDFR(ldap_value_free);
	UnloadDFR(ldap_abandon);
	UnloadDFR(ldap_result);
	UnloadDFR(ldap_result2error);

	LDAPSharedLibConnID = (void *)kNoConnectionID;
	LDAPSharedLibLoaded = false;
#endif  // LDAP_USE_STD_LINK_MECHANISM

	return noErr;
}
