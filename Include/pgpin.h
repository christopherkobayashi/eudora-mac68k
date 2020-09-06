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

#ifndef PGPIN_H
#define PGPIN_H

#ifndef ONE
typedef struct PGPContextStruct
{
	PGPEnum type;
	FSSpec spec;
	short refN;
	Str31 intro;	/* how we know it's pgp */
	struct PGPContextStruct *next;
} PGPContext, *PGPCPtr, **PGPCHandle;

typedef struct PGPRecvContextStruct
{
	Str31 intro;
	LineIOD lio;
} PGPRecvContext, *PGPRecvContextPtr, **PGPRecvContextHandle;

typedef struct HeaderDesc **HeaderDHandle;

Boolean ConvertPGP(short refN,UPtr buf,long *size,POPLineType lineType,long estSize,PGPCPtr pgpc);
void EndPGP(PGPCPtr pgpc);
void BeginPGP(PGPCPtr pgpc);
OSErr ReReadPGPClearText(TransStream stream,short refN,UPtr buf,long bSize,FSSpecPtr spec);
OSErr PGPRecvLine(TransStream stream,UPtr line,long *size);
OSErr PGPVerifyFile(FSSpecPtr spec);
OSErr PGPOpenEncrypted(FSSpecPtr spec);
OSErr ReadHeadAndBody(TransStream stream,short refN,UPtr buf,long bSize,Boolean display,HeaderDHandle *headersFound);
#endif

#endif