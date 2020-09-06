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

#ifndef NICKAE_H
#define NICKAE_H

// Make this structure whatever you want; I just put something here so I could compile
typedef struct
{
	short nickFileIndex;
} NickFileToken, *NickFileTPtr, **NickFileTHandle;
typedef struct
{
	short nickIndex;
	short	nickFileNumber;
}	NickToken, *NickTPtr, **NickTHandle;

#endif
Boolean AENickFileExists(AEDescPtr token);
Boolean AENicknameExists(AEDescPtr token);
Boolean AENickFieldExists(AEDescPtr token);
OSErr GetNickFileProperty(AEDescPtr token,AppleEvent *reply, uLong refCon);
OSErr GetNickProperty(AEDescPtr token,AppleEvent *reply, uLong refCon);
OSErr SetNickProperty(AEDescPtr token,AEDescPtr data);
OSErr GetNickField(AEDescPtr token,AppleEvent *reply, uLong refCon);
OSErr SetNickField(AEDescPtr token,AEDescPtr data);
OSErr AECreateNick(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply);
OSErr AECreateNickFile(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply);
OSErr AEDeleteNickname(NickTPtr token);
OSErr AEDeleteNickFile(NickFileTPtr token);
OSErr CountNickFileElements(DescType theClass,AEDescPtr inContainer,long *howMany);
OSErr CountNickFiles(DescType theClass,AEDescPtr inContainer,long *howMany);

pascal OSErr FindNickFile(DescType desiredClass, AEDescPtr containerToken,
	DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken, long refCon);
pascal OSErr FindNickname(DescType desiredClass, AEDescPtr containerToken,
	DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken, long refCon);
pascal OSErr FindNickField(DescType desiredClass, AEDescPtr containerToken,
	DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken, long refCon);
