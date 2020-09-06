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

#ifdef TWO
#ifndef FILTDEFS_H
#define FILTDEFS_H

typedef enum {
	flkZero,
	flkNone,
	flkDash1,
	flkStatus,
	flkPriority,
	flkLabel,
	flkPersonality,
	flkSubject,
	flkDash7,
	flkSound,
	flkSpeak,
	flkOpenMessage,
	flkPrint,
	flkAddHistory,
	flkNotifyUser,
	flkDash14,
	flkForward,
	flkRedirect,
	flkReply,
	flkDash18,
	flkServerOpts,
	flkDash20,
	flkCopy,
	flkTransfer,
	flkJunk,
	flkMoveAttach,
	flkDash25,
	flkStop,
	flkRule,
	flkIncoming,
	flkOutgoing,
	flkManual,
	flkHeader,
	flkVerb,
	flkValue,
	flkConjunction,
	flkName,
	flkCopyInstead,
	flkRaise,
	flkLower,
	flkId,
	flkMiniMessage,
	flkMiniMailbox,
	flkhNUser,
	flkhNReport,
	flkhForward,
	flkhRedirect,
	flkhReply,
	flkhTransfer,
	flkhCopy,
	flkhSubject,
	flkhPersonality,
	flkhStatus,
	flkhPriority,
	flkhLabel,
	flkhSound,
	flkhSpeak,
	flkhSpeakSender,
	flkhSpeakSubject,
	flkhOpenBox,
	flkhOpenMess,
	flkhFetch,
	flkhDelete,
	flkDelivery,
	flkLimit
} FilterKeywordEnum;

#define FilterKeywordStrn 25200

void *FATable(FilterKeywordEnum fk);
short FAPass(FilterKeywordEnum fk);
short FAMultiple(FilterKeywordEnum fk);
Boolean FAProOnly(FilterKeywordEnum fk);
#endif
#endif
