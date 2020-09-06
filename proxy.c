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

#include <conf.h>
#include <mydefs.h>

#include "proxy.h"
#define FILE_NUM 145

/**********************************************************************
 * ProxifyStr - take a string and massage it according to a proxy
 **********************************************************************/
PStr ProxifyStr(PStr theString, short theIndex)
{
	ProxyEntryHandle peh;
	Str255 localStr;

	if (NoProxify || !Proxies)
		return theString;

	CurPers = PERS_FORCE(CurPers);

	if ((*CurPers)->proxy) {
		// no nested proxies -- for now
		NoProxify = true;

		for (peh = (*(*CurPers)->proxy)->peh; peh;
		     peh = (*peh)->next)
			if ((*peh)->id == theIndex) {
				ComposeString(localStr, LDRef(peh)->value,
					      theString);
				PCopy(theString, localStr);
				UL(peh);
				break;
			}
		// re-allow proxying
		NoProxify = false;
	}

	return theString;
}

/**********************************************************************
 * ProxyInit - initialize the list of proxies
 **********************************************************************/
void ProxyInit(void)
{
	Handle res;
	short i;
	ProxyHandle ph;
	ProxyEntryHandle peh;
	Str255 scratch;
	Str255 value;
	Str31 token;
	OSType type;
	short id;
	short start, end;
	UPtr spot;

	// Phase 1 - initialize the proxies
	for (i = 1; res = GetIndResource(PROXY_RTYPE, i); i++) {
		if (ph = NewZH(ProxyType)) {
			// build the name
			GetResInfo(res, &id, &type, scratch);
			PSCopy((*ph)->name, scratch);

			// now the strings
			end = GetHandleSize(res);
			start = 2;
			while (start < end) {
				PCopy(scratch, (*res) + start);
				spot = scratch + 1;
				PToken(scratch, token, &spot, ":");
				PTerminate(token);
				id = atoi(token + 1);
				MakePStr(value, scratch + *token + 2,
					 *scratch - *token - 1);
				if (peh = NewZH(ProxyEntryType)) {
					(*peh)->id = id;
					PCopy((*peh)->value, value);
					LL_Queue((*ph)->peh, peh,
						 (ProxyEntryHandle));
				} else
					DieWithError(PROXY_INIT_FAILED,
						     MemError());
				start += (*res)[start] + 1;
			}
			ReleaseResource(res);
			LL_Queue(Proxies, ph, (ProxyHandle));
		} else
			DieWithError(PROXY_INIT_FAILED, MemError());
	}

	// Phase 2 - let the personalities know
	for (CurPers = PersList; CurPers; CurPers = (*CurPers)->next) {
		if (*GetPrefNoDominant(scratch, PREF_PROXY))
			if (ph = ProxyFind(scratch)) {
				(*CurPers)->proxy = ph;
				break;
			}
	}
}

/**********************************************************************
 * ProxyFind - find a proxy by name
 **********************************************************************/
ProxyHandle ProxyFind(PStr lookingFor)
{
	Str255 name;
	ProxyHandle ph;

	for (ph = Proxies; ph; ph = (*ph)->next) {
		PCopy(name, (*ph)->name);
		if (StringSame(name, lookingFor))
			return ph;
	}

	return nil;
}

/**********************************************************************
 * ProxyZap - clear out the old proxies
 **********************************************************************/
void ProxyZap(void)
{
	ProxyHandle ph;
	ProxyEntryHandle peh;

	while (ph = Proxies) {
		LL_Remove(Proxies, ph, (ProxyHandle));
		while (peh = (*ph)->peh) {
			LL_Remove((*ph)->peh, peh, (ProxyEntryHandle));
			ZapHandle(peh);
		}
		ZapHandle(ph);
	}
}

/**********************************************************************
 * ProxyMenu - build up a menu of proxy names
 **********************************************************************/
OSErr ProxyMenu(MenuHandle mh)
{
	ProxyHandle ph;
	Str31 name;

	if (!Proxies)
		return fnfErr;

	for (ph = Proxies; ph; ph = (*ph)->next) {
		PSCopy(name, (*ph)->name);
		MyAppendMenu(mh, name);
	}
	return noErr;
}
