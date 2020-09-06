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

#ifndef CONF_H
#define CONF_H


/*
 *	Define this once we're ready to make the build that doesn't register
 */
//#define	DEATH_BUILD

/*
 * Define this unless there is a miracle on the horizon
 */
//#define EUDORA_IS_ABOUT_TO_DIE_A_LUDICROUS_AND_MISGUIDED_FATE_OF_OVER_REACTION

/*
 * define I_HATE_THE_BOX to make a box or ESD build
 *
 *		Sites can turn off the registration nag (and the update nag, for that matter) by
 *		creating a plugin that overrides 'Nag#' (1011)  Remove the nag list here and no
 *		nags will occur.  The default values for this resource:
 *
 *				1)  'Nag ' ID  =  1002		Registration nag
 *				2)  'Nag ' ID  =  1005		Update check (silent check)
 *				3)  'Nag ' ID  =  1004		Update nag (non-silent)
 *
 *			Delete any one item to remove the particular nagging behaviour
 *
 */
//#define	I_HATE_THE_BOX


/*
 * define ADWARE to make an ADWARE build
 */
#define	ADWARE
#define	NAG

/*
 * define DEBUG if you want debugging code & resources
 * E_OUT_SNIFF for the code that looks for trashed handles to out
 * E_OUT_FLUSH for the code that flushes the Out mailbox to disk
 */
#define DEBUG

/*
 * define one of VALPHA, VBETA, VFINAL, VRELEASE
 */
#define VBETA

/*
 * this results in more frequent checking for updates
 */
#ifdef VBETA
#define	BETA_UPDATE_SCHEDULE
#endif

/*
 *	Define these big depressing constants if you want to build without things like
 *	forcing users into deadbeat mode, or if you want to remove any other functionality
 *	that depends on a continuing Eudora business.
 */

#ifdef EUDORA_IS_ABOUT_TO_DIE_A_LUDICROUS_AND_MISGUIDED_FATE_OF_OVER_REACTION
#define THEY_STUPIDLY_KILLED_EUDORA_SO_LETS_AT_LEAST_GIVE_THE_FAITHFUL_USERS_A_BREAK
#define	DO_NOT_UPDATE_CHECK_IN_DEATH_BUILDS
#ifdef VBETA
#undef	DO_NOT_UPDATE_CHECK_IN_DEATH_BUILDS
#endif
#endif

/*
 *	define DEMO to build a demo version of Eudora.
 *  its numeric value is the # of days after launch the demo will expire
 */
//#define	DEMO 30

/*
 * define EXPIRE if you want Eudora to expire.  Don't forget to set the proper
 * expiration dates
 */
//#define EXPIRE
#ifdef EXPIRE
#	define										EXP_YEAR	2002
#	define										EXP_MONTH	4
#endif

/*
 * version information
 */
#define										MAJOR_VERSION	6
#define										MINOR_VERSION	2
#define										INC_VERSION		5

//#define	GX_PRINTING		// Turn GX printing features on or off.  Keep this around, we'll dump GX someday.
#define	FLOAT_WIN		// leave this defined.  Will remove flag later
#define PEE						// use Pete's editor (obsolete; always used)
#define ETL						// translators
#define ATT_ICONS			// attachments in icon view
#define EXCERPT				// use rich text excerpt commands
#define RESYNC_MID		// use the message-id to resynch .toc's
#define NO_KEYUP			// define this to not process key-ups for command-period
#define DRAG_GETOSEVT	// define this to use GETOSEVT during drags
#define FANCY_FILT_LDEF	// fancy list def for filters
//#define SAVE_MIME			// Save the mime structure?
//#define CTB						// Support the Comm. Toolbox
#define	USECMM					// Provide Contextual Menu Manager Support
#define IMAP			//turn this on to enable the beast
//#ifndef LIGHT	// No Wintertree
#define WINTERTREE	// use internal wintertree speller
//#endif //LIGHT
#define LDAP_USE_STD_LINK_MECHANISM
#define NEWFIND
#define	VCARD					// Define if you're building new vcard savviness into the app
//#define NO_CHARSETS // Define if you want to turn off the Unicode stuff
#define CONTEXT_FILING	// Define to turn on contextual filing
#ifndef rez
#if TARGET_CPU_PPC
#define	SPEECH_ENABLED
#define URLACCESS
#endif
#if TARGET_CPU_68K
#pragma opt_strength_reduction off
#endif
#endif
// #define NO_EMSAPI_INIT	// define this to have Eudora ignore translators in debug mode
#define USERELATIVESIZES
#define USEFIXEDDEFAULTFONT
#define LABEL_ICONS		// define this to show color patches next to label items
//#define REFRESH_LABELS_MENU	// define this to refresh the finder labels frequently; may be crashing Tiger!
#define	USE_FANCY_HTML_EXPORT	// define this to get the right base directives in HTML
#define NOBODY_SPECIAL	// is a message without a body special?
#define TREAT_BODY_CR_AS_CRLF	// turn bare CR's in message bodies into CRLF.

#ifndef rez
#define PETEINLINE
#if __profile__
#include "Profiler.h"
#endif
#endif

#ifndef rez
#define HAVE_GETLASTACTIVITY
#define HAVE_KEYCHAIN
#define NSpare 2
#define SPARE_SIZE (35 K)
#endif

#define THREADING_ON									
#define TASK_PROGRESS_ON
#define MULTI_THREADING_ON	
#define BATCH_DELIVERY_ON
#define CONTEXT_SEARCH
#define URL_PROTECTION

#define GSSAPI

#define AD_WINDOW	//	display ad window
//	#define DIAL

#define CLIENT_BUILD_MONTH REG_EUD_CLIENT_6_2_MONTH
#ifdef VALPHA
#define DONT_CHECK_REGMONTH		// define this to allow users to use bad regcodes
#endif

#define ESSL	// Do SSL
#define LDAP_ENABLED	// Do LDAP
#define OFFLINE_LINK_DIALOG_ENABLED

#define GetMHandle GetMenuHandle 	// dammit
#endif