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

#ifndef SHAME_H
#define SHAME_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * functions of which I am not proud
 **********************************************************************/
#define DoAnAlert(level,string) ComposeStdAlert(level,OK_ASTR,string)
#define DoATimedAlert(level,string) 																		\
	AlertTicks=GetRLong(ALERT_TIMEOUT)*60+TickCount(),										\
	M_T1 = DoAnAlert(level,string), 																			\
	AlertTicks = 0, 																											\
	M_T2=M_T1
#define DoABigAlert(level,string) ComposeStdAlert(level,OK_ASTR,string);
#define DoNumAlert(level,errNum) \
	do {Str255 _s_;DoAnAlert(level,GetRString(_s_,errNum));}while(0)
#define DoNumBigAlert(level,errNum) \
	do {Str255 _s_;DoABigAlert(level,GetRString(_s_,errNum));}while(0)
int ReallyDoAnAlert(int template,int which);
int AlertStr(int template, int which, UPtr str);
Boolean Switch(void);
void SetAlertBeep(Boolean onOrOff);
Boolean MommyMommy(short sId,UPtr string);
int Aprintf(short template,short which,short rFormat,...);
void Dprintf(PStr fmt,...);
OSErr PtrPlusHand(const void *pointer, Handle hand, long size);
OSErr HandPlusHand(Handle h1, Handle h2);
void SetHandleBig(Handle h,long size);
OSErr MemoryPreflight(long size);
Handle DupHandle(Handle inHandle);
OSErr MyHandToHand(Handle *inHandle);
short ReallyStandardAlert(AlertType alertType, StringPtr error, StringPtr explanation, AlertStdAlertParamPtr alertParam);
OSErr MyStandardAlert(AlertType inAlertType,PStr inError,PStr inExplanation, AlertStdAlertParamRec *  inAlertParam,short *outItemHit);
OSErr GoGetHelp(PStr error, PStr explanation);
#ifdef DEBUG
void LeaksInit(void);
void LeaksDump(void);
void NuHandleStack(Handle h);
#endif

#ifdef DEBUG
#define RANDOM_BASE \
	while(RunType!=Production&&____RandomFailThresh&&(long)(Random()&0x3ff)<____RandomFailThresh) {DebugStr("\prandom;sc;g");LMSetMemErr(-108);
#define RANDOM_FAILURE_PROC RANDOM_BASE;return;}
#define RANDOM_FAILURE RANDOM_BASE;return(nil);}
#else
#define RANDOM_FAILURE_PROC ;
#define RANDOM_FAILURE ;
#endif

#endif
