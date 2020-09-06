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

#ifndef CTB_H
#define CTB_H

#ifdef CTB
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with ctb streams
 ************************************************************************/
OSErr CTBConnectTrans(TransStream stream,UPtr serverName,long port,Boolean silently);
OSErr CTBSendTrans(TransStream stream,UPtr text,long size, ...);
OSErr CTBRecvTrans(TransStream stream,UPtr line,long *size);
OSErr CTBDisTrans(TransStream stream);
OSErr CTBDestroyTrans(TransStream stream);
OSErr CTBTransError(TransStream stream);
void CTBSilenceTrans(TransStream stream,Boolean silence);
Boolean InitCTB(Boolean makeUser);
short DialThePhone(TransStream stream);
void HangUpThePhone();
short CTBNavigateSTRN(short id);
UPtr CTBWhoAmI(TransStream stream,Uptr who);
void MyCMIdle(void);
void CheckNavPW(Boolean *primary,Boolean *secondary);
short GetSecondPass(UPtr pass);
#endif


#endif