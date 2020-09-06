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

#include <Types.h>
#include "PrefDefs.h"
#include "StringDefs.h"
Boolean PrefBounds(short prefN,long *lower,long *upper)
{
#ifdef LIGHT
	Boolean Light = true;
#else
	Boolean Light = false;
#endif

	long l,u;
	switch (prefN)
	{
		case PREF_FONT_SIZE: l=7; u=127; break;
		case PREF_INTERVAL: l=0; u=999999; break;
		case PREF_PRINT_FONT_SIZE: l=3; u=127; break;
		case PREF_MWIDTH: l=20; u=140; break;
		case PREF_MHEIGHT: l=10; u=90; break;
		case PREF_NW_DEV: l=1; u=3; break;
		case PREF_POP3_LIMIT: l=0; u=1000; break;
		case PREF_LMOS_XDAYS: l=1; u=30; break;
		case POP_SCAN_INTERVAL: l=1; u=7; break;
		case UNREAD_LIMIT: l=1; u=30; break;
		case OPEN_TIMEOUT: l=10; u=255; break;
		case RECV_TIMEOUT: l=10; u=255; break;
		case SHORT_TIMEOUT: l=1; u=30; break;
		case AE_TIMEOUT_TICKS: l=60; u=32000; break;
		case BIG_MESSAGE: l=1; u=9999999; break;
		case MAX_MESSAGE_SIZE: l=0; u=1000000; break;
		case TOOLBAR_EXTRA_PIXELS: l=-3; u=20; break;
		case TOOLBAR_SEP_PIXELS: l=-3; u=20; break;
		case VICOM_FACTOR: l=1; u=100; break;
		case JUNK_MAILBOX_THRESHHOLD: l=1; u=100; break;
		case JUNK_MAILBOX_EMPTY_DAYS: l=0; u=365; break;
		case JUNK_MAILBOX_EMPTY_THRESH: l=0; u=100; break;
		case JUNK_XFER_SCORE: l=0; u=100; break;
		case JUNK_JUNK_SCORE: l=1; u=100; break;
		case JUNK_MIN_REASONABLE: l=0; u=100; break;
		case FLUSH_TIMEOUT: l=0; u=30; break;
		case JUNK_TRIM_SOON: l=1; u=30; break;
		case JUNK_TRIM_INTERVAL: l=1; u=30; break;
		case GRAPHICS_CACHE_MAX: l=1000; u=100000; break;
		case CONCON_MULTI_MAX: l=1; u=500; break;
		case ENCODED_FLOWED_WRAP_SPOT: l=20; u=70; break;
		case PREVIEW_SINGLE_DELAY: l=0; u=120; break;
		case PREVIEW_MULTI_DELAY: l=0; u=300; break;
		case WIN_GEN_WARNING_THRESH: l=2; u=100; break;
		case MULT_RESPOND_WARNING_THRESH: l=2; u=100; break;
		case IMAP_DEFAULT_JUNK_SCORE: l=0; u=100; break;
		case IMAP_FLAGGED_LABEL: l=0; u=15; break;
		case SEARCH_DF_AGE: l=0; u=100000; break;
		case SEARCH_DF_AGE_REL: l=1; u=4; break;
		case SEARCH_DF_AGE_SPFY: l=1; u=6; break;
		case SEARCH_DF_ATT: l=0; u=200; break;
		case SEARCH_DF_ATT_REL: l=1; u=4; break;
		default: return(0);
	}
	*lower = l;
	*upper = u;
	return(-1);
}
