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

#include "filtdefs.h"
extern void *FAflkNone(void);
extern void *FAflkStatus(void);
extern void *FAflkPriority(void);
extern void *FAflkLabel(void);
extern void *FAflkPersonality(void);
extern void *FAflkSubject(void);
extern void *FAflkSound(void);
extern void *FAflkSpeak(void);
extern void *FAflkOpenMessage(void);
extern void *FAflkPrint(void);
extern void *FAflkAddHistory(void);
extern void *FAflkNotifyUser(void);
extern void *FAflkForward(void);
extern void *FAflkRedirect(void);
extern void *FAflkReply(void);
extern void *FAflkServerOpts(void);
extern void *FAflkCopy(void);
extern void *FAflkTransfer(void);
extern void *FAflkJunk(void);
extern void *FAflkMoveAttach(void);
extern void *FAflkStop(void);

#pragma segment FILTRUN
void *FATable(FilterKeywordEnum fk)
{
	switch (fk)
	{
		case flkNone: return(FAflkNone);
		case flkStatus: return(FAflkStatus);
		case flkPriority: return(FAflkPriority);
		case flkLabel: return(FAflkLabel);
		case flkPersonality: return(FAflkPersonality);
		case flkSubject: return(FAflkSubject);
		case flkSound: return(FAflkSound);
		case flkSpeak: return(FAflkSpeak);
		case flkOpenMessage: return(FAflkOpenMessage);
		case flkPrint: return(FAflkPrint);
		case flkAddHistory: return(FAflkAddHistory);
		case flkNotifyUser: return(FAflkNotifyUser);
		case flkForward: return(FAflkForward);
		case flkRedirect: return(FAflkRedirect);
		case flkReply: return(FAflkReply);
		case flkServerOpts: return(FAflkServerOpts);
		case flkCopy: return(FAflkCopy);
		case flkTransfer: return(FAflkTransfer);
		case flkJunk: return(FAflkJunk);
		case flkMoveAttach: return(FAflkMoveAttach);
		case flkStop: return(FAflkStop);
		default:
			return(nil);
	}
}

short FAPass(FilterKeywordEnum fk)
{
	switch (fk)
	{
		case flkNone: return(0);
		case flkStatus: return(2);
		case flkPriority: return(0);
		case flkLabel: return(0);
		case flkPersonality: return(0);
		case flkSubject: return(1);
		case flkSound: return(0);
		case flkSpeak: return(0);
		case flkOpenMessage: return(0);
		case flkPrint: return(0);
		case flkAddHistory: return(0);
		case flkNotifyUser: return(0);
		case flkForward: return(1);
		case flkRedirect: return(1);
		case flkReply: return(1);
		case flkServerOpts: return(0);
		case flkCopy: return(3);
		case flkTransfer: return(4);
		case flkJunk: return(4);
		case flkMoveAttach: return(3);
		case flkStop: return(9);
		default: return(0);
	}
}

#pragma segment FilterWin
short FAMultiple(FilterKeywordEnum fk)
{
	switch (fk)
	{
		case flkNone: return(1);
		case flkStatus: return(0);
		case flkPriority: return(0);
		case flkLabel: return(0);
		case flkPersonality: return(0);
		case flkSubject: return(0);
		case flkSound: return(0);
		case flkSpeak: return(0);
		case flkOpenMessage: return(0);
		case flkPrint: return(0);
		case flkAddHistory: return(0);
		case flkNotifyUser: return(0);
		case flkForward: return(1);
		case flkRedirect: return(1);
		case flkReply: return(0);
		case flkServerOpts: return(0);
		case flkCopy: return(1);
		case flkTransfer: return(0);
		case flkJunk: return(0);
		case flkMoveAttach: return(0);
		case flkStop: return(0);
		default: return(0);
	}
}

Boolean FAProOnly(FilterKeywordEnum fk)
{
	switch (fk)
	{
		case flkLabel:
		case flkPersonality:
		case flkSound:
		case flkSpeak:
		case flkOpenMessage:
		case flkPrint:
		case flkAddHistory:
		case flkForward:
		case flkRedirect:
		case flkReply:
		case flkServerOpts:
		case flkJunk:
			return(1); break;
		default:
			return(0); break;
	}
}