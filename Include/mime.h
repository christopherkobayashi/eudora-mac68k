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

#ifndef MIME_H
#define MIME_H

#define Estimate64(x) ((M_T1=(4*(x)+3)/3),M_T1+((M_T1+75)/76)*2)
#define Estimate64Bin(x) (((x)*3)/4)

/*
 * state buffer for encoding
 */
typedef struct
{
	Byte partial[4];
	short partialCount;
	short bytesOnLine;
} Enc64, *Enc64Ptr, **Enc64Handle;

/*
 * state buffer for decoding
 */
typedef struct
{
	short decoderState;	/* which of 4 bytes are we seeing now? */
	long invalCount;		/* how many bad chars found so far? */
	long padCount;			/* how many pad chars found so far? */
	Byte partial;				/* partially decoded byte from/for last/next time */
	Boolean wasCR;			/* was the last character a carriage return? */
} Dec64, *Dec64Ptr, **Dec64Handle;

typedef enum
{
	qpNormal,
	qpEqual,
	qpByte1
}	QPStates;

typedef struct
{
	QPStates state;
	Byte lastChar;
} DecQP, *DecQPPtr, **DecQPHandle;

typedef struct
{
	short leftBytes;
	Str63 buffer;
} UUState, *UUStatePtr, **UUStateHandle;

/*
 * to do the encoding/decoding
 */
long Encode64(UPtr bin,long len,UPtr sixFour,PStr newLine,Enc64Ptr e64);
long Decode64(UPtr sixFour,long sixFourLen,UPtr bin,long *binLen,Dec64Ptr d64,Boolean text);
long EncodeQP(UPtr bin,long len,UPtr qp,PStr newLine,long *bplp);
long DecodeQP(UPtr qp,long qpLen,UPtr bin,long *binLen,DecQPPtr dqp);

#ifndef TOOL

/*
 * encoder/decoder call types
 */
typedef enum
{
	kDecodeInit,
	kDecodeData,
	kDecodeDone,
	kDecodeDispose
} CallType;


typedef enum
{
	btNotBoundary,
	btInnerBoundary,
	btOuterBoundary,
	btEndOfMessage,
	btError
} BoundaryType;

typedef struct MIMEMapStruct
{
	Str31 mimetype;
	Str31 subtype;
	Str31 suffix;
	OSType creator;
	OSType type;
	unsigned long flags;
	OSType specialId;
} MIMEMap, *MIMEMapPtr, **MIMEMapHandle;

typedef struct AttMapStruct
{
	MIMEMap mm;
	Boolean isPostScript;
	Boolean isText;
	Boolean isBasic;
	Boolean suppressXMac;
	Str31 shortName;
	Str127 longName;
	Str15 uuName;
} AttMap, *AttMapPtr, **AttMapHandle;

#define ATT_MAP_NAME(a) (*a->longName?a->longName:a->shortName)

#define mmIsText 0x8000
#define mmIsBasic 0x4000
#define mmIsAURL 0x2000
#define mmAlwaysDetach 0x1000
#define mmIgnoreXType 0x0800
#define mmDiscard 0x0400
#define mmApplySuffix 0x0200


typedef struct DecoderPB DecoderPB, *DecoderPBPtr, **DecoderPBHandle;
typedef struct MIMEState MIMEState, *MIMESPtr, **MIMESHandle;
typedef OSErr DecoderFunc(CallType callType, DecoderPBPtr decPB);
typedef BoundaryType ReadBodyFunc(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr);
DecoderFunc *FindMIMEDecoder(PStr encoding,Boolean *isExtern,Boolean load);
DecoderFunc QPEncoder, B64Encoder, UUEncoder;
OSErr FindAttMap(FSSpecPtr spec,AttMapPtr mmp);

typedef struct
{
	long offset;
	PETETextStyle style;
	long validBits;
	short sizeIndex;
}	OffsetAndStyle, *OffsetAndStylePtr, **OffsetAndStyleHandle;

/*
 * for decoders and file savers
 */
struct DecoderPB
{
	UPtr input;
	long inlen;
	UPtr output;
	long outlen;
	long refCon;
	Boolean text;
	Boolean noLineBreaks;
};

/*
 * MIME converter state structure
 */
struct MIMEState
{
	long headerOffset;
	HeaderDHandle hdh;
	DecoderFunc *decoder;
	DecoderPB dpb;
	ReadBodyFunc *readBody;
	Str127 boundary;
	MIMESHandle next;
	Boolean xDecoder;
	Boolean xFileSaver;
	Boolean isDigest;
	TLMHandle translators;
	long context;	// translation context
	short mhtmlID;
};

MIMESHandle NewMIMES(TransStream stream,HeaderDHandle hdh,Boolean forceMIME,short context);
OSErr RecordTLMIME(FSSpecPtr spec,emsMIMEHandle tlMIME);
OSErr RecordTL(FSSpecPtr spec,TLMHandle tl);
void DisposeMIMES(MIMESHandle msh);
#define ZapMIMES(msh) do{DisposeMIMES(msh);(msh)=nil;}while(0);
short FindMIMECharsetLo(PStr charSet,Boolean *found);
#define FindMIMECharset(cset) FindMIMECharsetLo(cset,nil)
void FigureMIMEFromApple(OSType creator, OSType type,PStr name,PStr mimeType,PStr mimeSub,PStr mimeSuffix, long *flags, OSType *specialId);
Boolean FindMIMEMapPtr(PStr type, PStr subType,PStr name,MIMEMapPtr mmp);
PStr Encode64Data(PStr encoded, UPtr data, short len);
void Encode64DataPtr(UPtr encoded, long *outLen,UPtr data,short len);
#define kMIMEBoring ((MIMESHandle)(-1L))
#define READ_MESSAGE ((void *)-1)
#endif

#endif
