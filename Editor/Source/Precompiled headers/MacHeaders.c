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

/*
 *	MacHeaders.c
 *
 *	Script to generate the 'MacHeaders<xxx>' precompiled header for Metrowerks C/C++.
 *  Copyright � 1993 metrowerks inc.  All rights reserved.
 */

/*
 *	Required for c-style toolbox glue function: c2pstr and p2cstr
 *	the inverse operation (pointers_in_A0) is performed at the end ...
 */

#if !(powerc || __CFM68K__)
 #pragma d0_pointers on
#endif

/*
 *	To allow the use of ToolBox calls which have now become obsolete on PowerPC, but
 *	which are still needed for System 6 applications, we need to #define OBSOLETE.  If
 *	your application will never use these calls then you can comment out this #define.
 *	NB: This is only for 68K ...
 */

//#if !defined(powerc) && !defined(OBSOLETE)
 #define OBSOLETE	0
//#endif

#define OLDROUTINENAMES 0
#define OLDROUTINELOCATIONS 0
#define SystemSevenFiveOrLater 1

/*
 *	Metrowerks-specific definitions
 *
 *	These definitions are commonly used but not in Apple's headers. We define
 *	them in our precompiled header so we can use the Apple headers without modification.
 */

#define PtoCstr		p2cstr
#define CtoPstr		c2pstr
#define PtoCString	p2cstr
#define CtoPString	c2pstr

#define topLeft(r)	(((Point *) &(r))[0])
#define botRight(r)	(((Point *) &(r))[1])

#ifndef	TRUE
#define TRUE		true
#endif

#ifndef	FALSE
#define FALSE		false
#endif

#ifndef powerc
 #include <MixedMode.h>
 long GetCurrentA5(void)
  ONEWORDINLINE(0x200D);
#endif

/*
 *	Apple #include files
 *
 *	Uncomment any additional #includes you want to add to MacHeaders.
 */

//	#include <ADSP.h>
	#include <AEObjects.h>
	#include <AEPackObject.h>
	#include <AERegistry.h>
//	#include <AEUserTermTypes.h>
//	#include <AIFF.h>
	#include <Aliases.h>
	#include <Appearance.h>
	#include <AppleEvents.h>
//	#include <AppleGuide.h>
//	#include <AppleScript.h>
//	#include <AppleTalk.h>
//	#include <ASDebugging.h>
//	#include <ASRegistry.h>
	#include <Balloons.h>
//	#include <CMApplication.h>
//	#include <CMComponent.h>
//	#include <CodeFragments.h>
	#include <ColorPicker.h>
//	#include <CommResources.h>
	#include <Components.h>
	#include <ConditionalMacros.h>
//	#include <Connections.h>
//	#include <ConnectionTools.h>
	#include <Controls.h>
	#include <ControlDefinitions.h>
//	#include <ControlStrip.h>
//	#include <CRMSerialDevices.h>
//	#include <CTBUtilities.h>
//	#include <CursorCtl.h>
//	#include <CursorDevices.h>
//	#include <DatabaseAccess.h>
//	#include <DeskBus.h>
	#include <Devices.h>
	#include <Dialogs.h>
//	#include <Dictionary.h>
//	#include <DisAsmLookup.h>
//	#include <Disassembler.h>
#ifndef	__MACH__
	#include <DiskInit.h>
#endif
//	#include <Disks.h>
//	#include <Displays.h>
	#include <Drag.h>
#ifndef	__MACH__
	#include <Editions.h>
#endif
//	#include <ENET.h>
//	#include <EPPC.h>
//	#include <ErrMgr.h>
	#include <Errors.h>
	#include <Events.h>
//	#include <fenv.h>
	#include <Files.h>
//	#include <FileTransfers.h>
//	#include <FileTransferTools.h>
#ifndef	__MACH__
	#include <FileTypesAndCreators.h>
#endif
	#include <Finder.h>
	#include <FixMath.h>
	#include <Folders.h>
	#include <Fonts.h>
//	#include <fp.h>
//	#include <FragLoad.h>
//	#include <FSM.h>
	#include <Gestalt.h>
//	#include <HyperXCmd.h>
	#include <Icons.h>
//	#include <ImageCodec.h>
//	#include <ImageCompression.h>
	#include <IntlResources.h>
//	#include <Language.h>
	#include <Lists.h>
	#include <LowMem.h>
//	#include <MachineExceptions.h>
//	#include <MacTCP.h>
//	#include <MediaHandlers.h>
	#include <Memory.h>
	#include <Menus.h>
//	#include <MIDI.h>
	#include <MixedMode.h>
//	#include <Movies.h>
//	#include <MoviesFormat.h>
	#include <Notification.h>
//	#include <OSA.h>
//	#include <OSAComp.h>
//	#include <OSAGeneric.h>
	#include <OSUtils.h>
#ifndef	__MACH__
	#include <Packages.h>
#endif
	#include <Palettes.h>
//	#include <Picker.h>
//	#include <PictUtil.h>
//	#include <PictUtils.h>
//	#include <PLStringFuncs.h>
//	#include <Power.h>
//	#include <PPCToolbox.h>
#ifndef	__MACH__
	#include <Printing.h>
#endif
	#include <Processes.h>
	#include <QDOffscreen.h>
	#include <Quickdraw.h>
	#include <QuickdrawText.h>
//	#include <QuickTimeComponents.h>
	#include <Resources.h>
//	#include <Retrace.h>
//	#include <ROMDefs.h>
#ifndef powerc
//	#include <SANE.h>
#endif
	#include <Scrap.h>
	#include <Script.h>
//	#include <SCSI.h>
#ifndef	__MACH__
	#include <SegLoad.h>
#endif
//	#include <Serial.h>
//	#include <ShutDown.h>
//	#include <Slots.h>
//	#include <Sound.h>
//	#include <SoundComponents.h>
//	#include <SoundInput.h>
//	#include <Speech.h>
#ifndef	__MACH__
	#include <StandardFile.h>
#endif
//	#include <Start.h>
	#include <Strings.h>
//	#include <Terminals.h>
//	#include <TerminalTools.h>
	#include <TextEdit.h>
	#include <TextServices.h>
	#include <TextUtils.h>
//	#include <Threads.h>
//	#include <Timer.h>
	#include <ToolUtils.h>
//	#include <Translation.h>
//	#include <TranslationExtensions.h>
#ifndef	__MACH__
	#include <Traps.h>
#endif
//	#include <TSMTE.h>
	#include <Types.h>
//	#include <Unmangler.h>
//	#include <Video.h>
	#include <Windows.h>
#ifndef	__MACH__
	#include <WorldScript.h>
#endif


/*
 *	required for c-style toolbox glue function: c2pstr and p2cstr
 *	(match the inverse operation at the start of the file ...
 */

#if !(powerc || __CFM68K__)
 #pragma d0_pointers reset
#endif
