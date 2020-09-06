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

#ifndef URL_H
#define URL_H

typedef enum
{
	urlNot,
	urlNaughty,
	urlMaybe,
	urlRude,
	urlGood
} URLEnum;

OSErr OpenLocalURLLo(PStr url,Handle *result,AEDescList *dox,Boolean spool);
OSErr OpenLocalURLPtr(PStr url,long len,Handle *result,AEDescList *dox,Boolean spool);
#define OpenLocalURL(u,r)	OpenLocalURLLo(u,r,nil,false)

OSErr FindURLApp(PStr proto,AliasHandle *alias,Boolean mayWildCard);

#define kWildCardOK	True
#define kNoWildCard	False
#ifdef TWO
OSErr OpenOtherURLPtr(PStr proto,UPtr url,short length);
#endif
#define urlSelect	1
#define urlColor	2
#define urlOpen		4
#define urlAll		7
URLEnum URLIsSelected(MyWindowPtr win,PETEHandle pte,long startWith,long endWith,short what,long *start,long *stop);
URLEnum SlackURL(MyWindowPtr win,PETEHandle pte,long startWith,long endWith,short what,long *start,long *stop);
void URLScan(void);
void FixURLString(PStr url);
void FixURLPtr(Ptr url,long *len);
OSErr ParseURL(PStr url,PStr proto,PStr host,PStr query);
OSErr ParseURLPtr(PStr url,long length,PStr proto,PStr host,Ptr *queryPtr,long *queryLen);
OSErr ParseProtocolFromURLPtr (UPtr url, short length, PStr proto);
Boolean ParsePortFromHost(PStr host,PStr server,long *port);
void PeteURLScan(MyWindowPtr win,PETEHandle pte);
void URLStyle(PETEHandle pte, long selStart,long selEnd,Boolean rude);
PStr URLCombine(PStr result,PStr base,PStr rel);
PStr URLEscape(PStr url);
PStr URLEscapeLo(PStr url,Boolean allPercents);
void URLEscapeEvenLower (Ptr url, Size count, Boolean allPercents);
PStr URLQueryEscape(PStr query);
OSErr URLSubstitute(PStr resultURL,PStr mhtmlIDStr,PStr origURL,PETEHandle pte);
OSErr InsertURL(PETEHandle pte);
OSErr InsertURLLo(PETEHandle pte,long start,long stop,PStr url);
Boolean URLOkHere(PETEHandle pte);
PStr MakeFileURL(PStr url,FSSpecPtr spec, short proto);
OSErr MakeHelperAlias(FSSpecPtr app,AliasHandle *alias);
OSErr SaveURLPref(PStr proto,AliasHandle alias);
OSErr MailtoURLPtr(Ptr query,long queryLen,AEDescList *dox,Boolean spool);
OSErr SettingURL(PStr query);
Boolean URLHelpTagList(PETEHandle pte,Point mouse);
#endif