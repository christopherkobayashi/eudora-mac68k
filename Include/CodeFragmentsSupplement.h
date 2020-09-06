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

#ifndef _CODEFRAGMENTSSUPPLEMENT_
#define _CODEFRAGMENTSSUPPLEMENT_


#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __CODEFRAGMENTS__
#include <CodeFragments.h>
#endif


enum {
	kCurrentCfrgRsrcVers = 0x00000001
};


#if PRAGMA_STRUCT_ALIGN
#pragma options align=mac68k
#else
#error This file requires a compiler that supports mac68k struct alignment
#endif

typedef struct {
	OSType		fragArch;
	long		updateLevel;
	NumVersion	curVersion;
	NumVersion	oldestDefVersion;
	long		applStackSize;
	short		applLibDirAlisID;
	char		fragType;
	char		fragLocationType;
	long		offset;
	long		length;
	long		reserved1;
	long		reserved2;
	short		infoRecLen;
	Str255		fragName;
	/* fragName is variable-length, and padded to make the record end on a longword boundary */
} CodeFragInfoRec, *CodeFragInfoRecPtr;

typedef struct {
	long	reserved1;
	long	reserved2;
	long	cfrgRsrcVers;
	long	reserved3;
	long	reserved4;
	long	reserved5;
	long	reserved6;
	long	numFragInfoRecs;
	CodeFragInfoRec		firstFragInfoRec;
	/* remaining data is variable length */
} CodeFragRsrcRec, *CodeFragRsrcPtr, **CodeFragRsrcHdl;

#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#endif


#endif