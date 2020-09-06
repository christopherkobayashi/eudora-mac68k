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

#if !defined(__MENUSHARING_H__)
#define __MENUSHARING_H__

/* 
Copyright (c) 1991-2000 UserLand Software, Inc. 

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to the 
following conditions: 

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/


#ifndef __APPLEEVENTS__

#include <AppleEvents.h>

#endif

#ifndef __COMPONENTS__

#include <Components.h>		/*3.0 */

#endif

#ifndef __MENUS__

#include <Menus.h>		/*3.0 */

#endif


#define __MENUSHARING__		/*so other modules can tell that we've been included */

//      RMS 960614
#if !TARGET_API_MAC_CARBON
#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif				//PRAGMA_ALIGN_SUPPORTED
#endif				//TARGET_API_MAC_CARBON

typedef struct tysharedmenurecord {	/*must match scripting system record structure */

	short idmenu;		/*the resource id of the menu */

	short flhierarchic:1;	/*if true it's a hiearchic menu */

	short flinserted:1;	/*if true the menu has been inserted in the menu bar */

	MenuHandle hmenu;	/*a handle to the Mac Menu Manager's data structure */
} tysharedmenurecord;


typedef tysharedmenurecord tymenuarray[1];

typedef tymenuarray **hdlmenuarray;

typedef pascal void (*tyMSerrordialog)(Str255);

typedef pascal void (*tyMSeventfilter)(EventRecord *, ...);

typedef pascal void (*tyMSmenusinstaller)(hdlmenuarray);



typedef struct tyMSglobals {	/*Menu Sharing globals, all in one struct */

	OSType serverid;	/*identifier for shared menu server */

	OSType clientid;	/*id of this application */

	hdlmenuarray hsharedmenus;	/*data structure that holds shared menus */

	Boolean fldirtysharedmenus;	/*if true, menus are reloaded next time app comes to front */

	Boolean flscriptcancelled;	/*set true by calling CancelSharedScript */

	Boolean flscriptrunning;	/*true if a script is currently running */

	Boolean flinitialized;	/*true if InitSharedMenus was successful */

	long idscript;		/*the server's id for the currently running script, makes it easy to kill it */

	ComponentInstance menuserver;	/*3.0 */

	long serverversion;	/*4.1 */

	tyMSerrordialog scripterrorcallback;	/*3.0 */

	tyMSeventfilter eventfiltercallback;	/*3.0 */

	tyMSmenusinstaller menusinsertercallback;	/*4.1 */

	tyMSmenusinstaller menusremovercallback;	/*4.1 */
} tyMSglobals;


extern tyMSglobals MSglobals;	/*menu sharing globals */


/*basic Menu Sharing routines*/

pascal Boolean InitSharedMenus(tyMSerrordialog, tyMSeventfilter);

pascal Boolean SharedMenuHit(short, short);

pascal Boolean SharedScriptRunning(void);

pascal Boolean CancelSharedScript(void);

pascal Boolean CheckSharedMenus(short);

pascal Boolean SharedScriptCancelled(AppleEvent *, AppleEvent *);


/*special-purpose routines*/

pascal Boolean DisposeSharedMenus(void);

pascal Boolean IsSharedMenu(short);

pascal Boolean EnableSharedMenus(Boolean);

pascal Boolean RunSharedMenuItem(short, short);

pascal Boolean SetMenusInserterCallback(tyMSmenusinstaller);

pascal Boolean SetMenusRemoverCallback(tyMSmenusinstaller);

//      RMS 960614      
#if !TARGET_API_MAC_CARBON
#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif				//PRAGMA_ALIGN_SUPPORTED
#endif				//TARGET_API_MAC_CARBON

#endif				// __MENUSHARING_H__
