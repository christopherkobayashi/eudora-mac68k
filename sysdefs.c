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

#include <string.h>

#include <conf.h>
#include <mydefs.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
#define SystemSevenOrLater 1
#define PM_USE_SESSION_APIS 0

#include <Aliases.h>
#include <Appearance.h>
#include <AppleEvents.h>
//#include <AppleTalk.h>
#include <assert.h>
#include <AEObjects.h>
#include <AEPackObject.h>
#include <AERegistry.h>
#include <ColorPicker.h>
//#include <CommResources.h>
//#include <Connections.h>
//#include <ContextualMenu.h>
//#include <CRMSerialDevices.h>
//#include <CTBUtilities.h>
#include <Controls.h>
#include <ControlDefinitions.h>
//#include <ctype.h>
//#include <CursorCtl.h>
#include <Devices.h>
#include <Dialogs.h>
#ifdef NEVER
#include <DisAsmLookUp.h>
#endif
//#include <DiskInit.h>
//#include <Disks.h>
#include <Drag.h>
//#include <ErrMgr.h>
#include <errno.h>
#include <Errors.h>
#include <Events.h>
#include <Files.h>
#include <Finder.h>
#include <FixMath.h>
#include <float.h>
#include <Folders.h>
#include <Fonts.h>
//#include <GXEnvironment.h>
//#include <GXErrors.h>
//#include <GXGraphics.h>
//#include <GXPrinting.h>
//#include <GXTypes.h>
#include <ImageCompression.h>
#if TARGET_CPU_PPC	// No cfm-68K support for this!  :-(
#include <KeyChain.h>
#else //TARGET_CPU_PPC
#define KeychainManagerAvailable()	(false)
#endif //TARGET_CPU_PPC
#include <limits.h>
#include <Lists.h>
#include <locale.h>
#if TARGET_RT_MAC_CFM
#include <MacTCP.h>
#endif
#include <MacApplication.h>
#include <math.h>
#include <Memory.h>
#include <Menus.h>
#include <Movies.h>
#include <Notification.h>
#include <OSUtils.h>
//#include <Packages.h>
#ifdef NEVER
#include <Perf.h>
#endif
#include <PMApplication.h>
#include <PMDefinitions.h>
//#include <PMCore.h>
#include <Quickdraw.h>
#include <Processes.h>
#include <Resources.h>
//#include <Retrace.h>
#include <LowMem.h>
#ifdef NEVER
#include <SANE.h>
#endif
#include <Scrap.h>
#include <Script.h>
#include <SCSI.h>
//#include <SegLoad.h>
//#include <Serial.h>
#include <setjmp.h>
//#include <ShutDown.h>
#include <signal.h>
//#include <Slots.h>
#include <Sound.h>
//#include <Start.h>
#include <stdarg.h>
// #include <StdRef.h> CK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <TextEdit.h>
#include <time.h>
#include <Timer.h>
#include <ToolUtils.h>
//#include <Traps.h>
#include <MacTypes.h>
#include <Video.h>
#include <Windows.h>
#include <Strings.h>
#include <Gestalt.h>
#include <Balloons.h>
#include <Icons.h>
#include <Power.h>
#include <OpenTransport.h>
#include <OpenTptAppleTalk.h>
#include <OpenTptInternet.h>
#include <OpenTptLinks.h>
#include <QDOffscreen.h>
#include <TextEncodingConverter.h>
#include <UnicodeConverter.h>
#include "WordServices.h"
#include "SpotLightAPI.h"
#ifdef TWO
#if TARGET_RT_MAC_CFM
#include "hesiod.h"	// !!! Marshall sez - not now for MachO
#endif
#endif
#include <InternetConfig.h>
#ifdef applec
#include "AEBuild.h"
#include "AEPrint.h"
#pragma dump SYS_LOAD
#endif
#include <Threads.h>
