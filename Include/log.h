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

#ifndef LOG_H
#define LOG_H

/* Copyright (c) 1992 by Qualcomm, Inc */
/**********************************************************************
 * the log file
 **********************************************************************/
UPtr ComposeLogR(uLong level, UPtr into, short format, ...);
UPtr ComposeLogS(uLong level, UPtr into, UPtr format, ...);
UPtr Log(uLong level, Uptr string);
void MyParamText(PStr p1, PStr p2, PStr p3, PStr p4);
void CloseLog(void);
void LogAlert(short template);
void CarefulLog(uLong level, short format, UPtr data, short dSize);
void LineLog(uLong level, short format, UPtr data, short dSize);
void HexLog(uLong level, short format, UPtr data, short dSize);
#ifdef DEBUG
#define LOGFLOW(x) do{ComposeLogS(LOG_FLOW,nil,"\p{%r:%d}:%d",FNAME_STRN+FILE_NUM,__LINE__,x);}while(0)
#else
#define LOGFLOW(x)
#endif
#define LOG_SEND	1
#define LOG_RETR	2
#define LOG_NAV		4
#define LOG_ALRT	8
#define LOG_PROG	16
#define LOG_TRANS	32
#define LOG_EVENT	64
#define LOG_MENU	128
#define LOG_FLOW	256
#define LOG_AE		512
#define LOG_TPUT	1024
#define LOG_LMOS	2048
#define LOG_PLUG	4096
#define LOG_FILT	8192
#define LOG_MOVE	16384
#define LOG_SEARCH	32768
#define LOG_PLIST	(1L<<16)
#define LOG_SSL		(1L<<17)
#define LOG_PROTO	(1L<<18)
#define LOG_TOC		(1L<<19)
#define LOG_MORE	(1L<<20)

#define	LOG_ALL		-1

#endif
