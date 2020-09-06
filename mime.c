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

#include "mime.h"
#define FILE_NUM 53
/* Copyright (c) 1993 by QUALCOMM Incorporated */

#pragma segment MIME

#define SKIP -1
#define FAIL -2
#define PAD  -3
#ifdef DEBUG
void PrintMIMEList(PStr where,MIMESHandle msh);
#endif
static UPtr gEncode =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static UPtr gUUEncode =
	"`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

static short gDecode[] = 
{
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,SKIP,SKIP,FAIL,FAIL,SKIP,FAIL,FAIL,	/* 0 */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 1 */
	SKIP,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,0x3e,FAIL,FAIL,FAIL,0x3f,	/* 2 */
	0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,FAIL,FAIL,FAIL,PAD ,FAIL,FAIL,	/* 3 */
	FAIL,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,	/* 4 */
	0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 5 */
	FAIL,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,	/* 6 */
	0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 7 */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 8 */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 9 */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* A */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* B */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* C */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* D */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* E */
	FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* F */
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
};

/*
 * the encoding is like this:
 *
 * Binary:<111111><222222><333333>
 *				765432107654321076543210
 * Encode:<1111><2222><3333><4444>
 */
/*
 * Bit extracting macros
 */
#define Bot2(b) ((b)&0x3)
#define Bot4(b) ((b)&0xf)
#define Bot6(b) ((b)&0x3f)
#define Top2(b) Bot2((b)>>6)
#define Top4(b) Bot4((b)>>4)
#define Top6(b) Bot6((b)>>2)

/*
 * the decoder
 */
#define EncodeThree64(bin,b64,bpl,nl)	EncodeThreeFour(bin,b64,bpl,nl,gEncode)
#define EncodeThreeUU(bin,b64,bpl,nl)	EncodeThreeFour(bin,b64,bpl,nl,gUUEncode)
#define EncodeThreeFour(bin,b64,bpl,nl,vector)								\
	do																													\
	{																														\
		if (nl && (bpl)==68)																			\
		{																													\
			int m_nl;																								\
			for (m_nl=0;m_nl<*(nl);)																\
				*(b64)++ = (nl)[++m_nl];															\
			(bpl) = 0;																							\
		}																													\
		(bpl) += 4;																								\
		*(b64)++ = vector[Top6((bin)[0])];												\
		*(b64)++ = vector[Bot2((bin)[0])<<4 | Top4((bin)[1])];		\
		*(b64)++ = vector[Bot4((bin)[1])<<2 | Top2((bin)[2])];		\
		*(b64)++ = vector[Bot6((bin)[2])];												\
	}																														\
	while (0)

/************************************************************************
 * Encode64 - convert binary data to base64
 *  bin -> the binary data (or nil to close encoder)
 *	len -> the length of the binary data
 *  sixFour -> pointer to buffer for the base64 data
 *  newLine -> newline character(s) to use
 *  e64 <-> state; caller must preserve
 *  returns the length of the base64 data
 ************************************************************************/
long Encode64(UPtr bin,long len,UPtr sixFour,PStr newLine,Enc64Ptr e64)
{
	UPtr binSpot;								/* the byte currently being decoded */
	UPtr sixFourSpot=sixFour;		/* the spot to which to copy the encoded chars */
	short bpl;
	UPtr end;										/* end of integral decoding */
	
	bpl = e64->bytesOnLine;	/* in inner loop; want local copy */
	
	if (len)
	{
		
		/*
		 * do we have any stuff left from last time?
		 */
		if (e64->partialCount)
		{
			short needMore = 3-e64->partialCount;
			if (len>=needMore)
			{
				/*
				 * we can encode some bytes
				 */
				BMD(bin,e64->partial+e64->partialCount,needMore);
				len -= needMore;
				bin += needMore;
				EncodeThree64(e64->partial,sixFourSpot,bpl,newLine);
				e64->partialCount = 0;
			}
			/*
			 * if we don't have enough bytes to complete the leftovers, we
			 * obviously don't have 3 bytes.  So the encoding code will fall
			 * through to the point where we copy the leftovers to the partial
			 * buffer.  As long as we're careful to append and not copy blindly,
			 * we'll be fine.
			 */
		}
		
		/*
		 * we encode the integral multiples of three
		 */
		end = bin+3*(len/3);
		for (binSpot=bin;binSpot<end;binSpot+=3)
			EncodeThree64(binSpot,sixFourSpot,bpl,newLine);
		
		/*
		 * now, copy the leftovers to the partial buffer
		 */
		len = len%3;
		if (len)
		{
			BMD(binSpot,e64->partial+e64->partialCount,len);
			e64->partialCount += len;
		}
	}
	else
	{
		/*
		 * we've been called to cleanup the leftovers
		 */
		if (e64->partialCount)
		{
			if (e64->partialCount<2) e64->partial[1] = 0;
			e64->partial[2] = 0;
			EncodeThree64(e64->partial,sixFourSpot,bpl,newLine);
			
			/*
			 * now, replace the unneeded bytes with ='s
			 */
			sixFourSpot[-1] = '=';
			if (e64->partialCount==1) sixFourSpot[-2] = '=';
		}
	}
	e64->bytesOnLine = bpl;	/* copy back to state buffer */
	return(sixFourSpot-sixFour);
}

#define FIX_NL do {																\
	if (text)																				\
		if (binSpot[-1]=='\012')												\
		{																							\
			if (wasCR)																	\
			{																						\
				wasCR = False;														\
				binSpot--;		/* crlf -> cr */						\
			}																						\
			else																				\
				binSpot[-1] = '\015'; /* lf -> cr */				\
		}																							\
		else wasCR = binSpot[-1]=='\015';								\
	} while(0)

/************************************************************************
 * Decode64 - convert base64 data to binary
 *  sixFour -> the base64 data (or nil to close decoder)
 *  sixFourLen -> the length of the base64 data
 *  bin -> pointer to buffer to hold binary data
 *  binLen <- length of binary data
 *  d64 <-> pointer to decoder state
 *  returns the number of decoding errors found
 ************************************************************************/
long Decode64(UPtr sixFour,long sixFourLen,UPtr bin,long *binLen,Dec64Ptr d64,Boolean text)
{
	short decode;											/* the decoded short */
	Byte c;														/* the decoded byte */
		/* we separate the short & the byte to the compiler can worry about byteorder */
	UPtr end = sixFour + sixFourLen;	/* stop decoding here */
	UPtr binSpot = bin;								/* current output character */
	short decoderState;	/* which of 4 bytes are we seeing now? */
	long invalCount;		/* how many bad chars found this time around? */
	long padCount;			/* how many pad chars found so far? */
	Byte partial;				/* partially decoded byte from/for last/next time */
	Boolean wasCR;

	/*
	 * fetch state from caller's buffer
	 */
	decoderState = d64->decoderState;
	invalCount = 0;	/* we'll add the invalCount to the buffer later */
	padCount = d64->padCount;
	partial = d64->partial;
	wasCR = d64->wasCR;
	
	if (sixFourLen)
		for (;sixFour<end;sixFour++)
		{
			switch(decode=gDecode[*sixFour])
			{
				case SKIP: break;									/* skip whitespace */
				case FAIL: invalCount++; break;		/* count invalid characters */
				case PAD: padCount++; break;			/* count pad characters */
				default:
					/*
					 * found a non-pad character, so if we had previously found a pad,
					 * that pad was an error
					 */
					if (padCount) {invalCount+=padCount;padCount=0;}
					
					/*
					 * extract the right bits
					 */
					c = decode;
					switch (decoderState)
					{
						case 0:
							partial = c<<2;
							decoderState++;
							break;
						case 1:
							*binSpot++ = partial|Top4(c);
							partial = Bot4(c)<<4;
							decoderState++;
							FIX_NL;
							break;
						case 2:
							*binSpot++ = partial|Top6(c);
							partial = Bot2(c)<<6;
							decoderState++;
							FIX_NL;
							break;
						case 3:
							*binSpot++ = partial|c;
							decoderState=0;
							FIX_NL;
							break;
					}
			}
		}
	else
	{
		/*
		 * all done.  Did all end up evenly?
		 */
		switch (decoderState)
		{
			case 0:
				invalCount += padCount;		/* came out evenly, so should be no pads */
				break;
			case 1:
				invalCount++;							/* data missing */
				invalCount += padCount;		/* since data missing; should be no pads */
				break;
			case 2:
				invalCount += ABS(padCount-2);	/* need exactly 2 pads */
				break;
			case 3:
				invalCount += ABS(padCount-1);	/* need exactly 1 pad */
				break;
		}
	}
	
	*binLen = binSpot - bin;
	
	/*
	 * save state in caller's buffer
	 */
	d64->decoderState = decoderState;
	d64->invalCount += invalCount;
	d64->padCount = padCount;
	d64->partial = partial;
	d64->wasCR = wasCR;

	return(invalCount);
}

/************************************************************************
 * EncodeQP - convert binary data to quoted-printable
 *  bin -> the binary data
 *	len -> the length of the binary data (or 0 to close the encoder)
 *  qp -> pointer to buffer for the quoted-printable data
 *  newLine -> newline character(s) to use
 *  eqp <-> state; caller must preserve
 *  returns the length of the quoted-printable data
 ************************************************************************/
long EncodeQP(UPtr bin,long len,UPtr qp,PStr newLine,long *bplp)
{
	UPtr binSpot;								/* the byte currently being decoded */
	UPtr end = bin+len;					/* end of decoding */
	UPtr qpSpot = qp;						/* the spot to which to copy the encoded chars */
	short bpl;
	short c;
	static char *hex="0123456789ABCDEF";
	Boolean encode;
	UPtr nextSpace;
	
	bpl = *bplp;	/* in inner loop; want local copy */

#define QPNL		do {														\
			BMD(newLine+1,qpSpot,*newLine);			\
			qpSpot += *newLine;												\
			bpl = 0;} while (0)
	
#define ROOMFOR(x)															\
	do {																					\
		if (bpl+x>76)																\
		{																						\
			*qpSpot++ = '=';													\
			QPNL;																			\
		}																						\
	} while(0)

	for (binSpot=bin;binSpot<end;binSpot++)
	{
		/*
		 * make copy of char
		 */
		c = *binSpot;
		
		/*
		 * handle newlines
		 */
		if (c==NewLine[1])
		{
			QPNL;
		}
		else if (c=='\012' || c=='\015') ;	/* skip other newline characters */
		else
		{
			if (c==' ' || c=='\t')
			{
				encode = (binSpot<end-1 && binSpot[1]=='\015');	/* trailing white space */
				if (!encode)
				{
					for(nextSpace=binSpot+1;nextSpace<end;nextSpace++)
						if (*nextSpace==' ' || *nextSpace=='\015')
						{
							if (nextSpace-binSpot<20)  ROOMFOR(nextSpace-binSpot);
							break;
						}
				}
			}
			else
				encode =  c=='=' || c<33 || c>126;	/* weird characters */
			if (encode) ROOMFOR(3); else ROOMFOR(1);
			encode = encode || !bpl && (c=='F' || c=='.');
			if (encode)
			{
				*qpSpot++ = '=';
				*qpSpot++ = hex[(c>>4)&0xf];
				*qpSpot++ = hex[c&0xf];
				bpl += 3;
			}
			else
			{
				*qpSpot++ = c;
				bpl++;
			}
		}
	}
	*bplp = bpl;	/* copy back to state buffer */
	return(qpSpot-qp);
}

#define HEX(c)	((c)>='0' && (c)<='9' ? (c)-'0' : ((c)>='A' && (c)<='F' ? (c)-'A'+10 : ((c)>='a' && (c)<='f'  ? (c)-'a'+10 : -1)))
/************************************************************************
 * DecodeQP - convert quoted printable data to binary
 *  sixFour -> the quoted printable data
 *  sixFourLen -> the length of the base64 data (or 0 to close)
 *  bin -> pointer to buffer to hold binary data
 *  binLen <- length of binary data
 *  dqp <-> pointer to decoder state
 *  returns the number of decoding errors found
 ************************************************************************/
long DecodeQP(UPtr qp,long qpLen,UPtr bin,long *binLen,DecQPPtr dqp)
{
	Byte c;									/* the decoded byte */
	UPtr end;								/* stop decoding here */
	UPtr binSpot = bin;			/* current output character */
	UPtr qpSpot;
	QPStates state;
	Byte lastChar;
	short errs=0;
	short upperNib, lowerNib;

	/*
	 * trim spaces before newlines
	 */
	if (qp)
	{
		qpSpot=qp+qpLen-1;
		if (qpSpot>qp && *qpSpot=='\015')
		{
			for (qpSpot--;qpSpot>=qp && *qpSpot==' ';qpSpot--);
			qpSpot[1] = '\015';
			qpLen = qpSpot-qp+2;
		}
	}
	end = qp + qpLen;
	
	/*
	 * fetch state from caller's buffer
	 */
	state = dqp->state;
	lastChar = dqp->lastChar;
	
	if (qpLen)
	{
		for (qpSpot=qp;qpSpot<end;qpSpot++)
		{
			c = *qpSpot;
			switch (state)
			{
				case qpNormal:
					if (c=='=') state = qpEqual;
					else *binSpot++ = c;
					break;
					
				case qpEqual:
					state = c=='\015' ? qpNormal : qpByte1;
					break;
				
				case qpByte1:
					upperNib = HEX(lastChar);
					lowerNib = HEX(c);
					if (upperNib<0 || lowerNib<0) errs++;
					else
						*binSpot++ = (upperNib<<4) | lowerNib;
					state = qpNormal;
					break;
			}
			lastChar = c;
		}
	}
	else if (state != qpNormal) errs++;
	
	/*
	 * save state in caller's buffer
	 */
	dqp->state = state;
	dqp->lastChar = lastChar;
	*binLen = binSpot-bin;
	return(errs);
}
#ifndef TOOL

/************************************************************************
 * Other MIME stuff
 ************************************************************************/
BoundaryType HuntBoundary(TransStream stream,short refN,MIMESHandle msh,UPtr buf,long bSize, long *size);
BoundaryType IsBoundaryLine(POPLineType lt, UPtr buf, long len, PStr boundary);
OSErr WriteBoundary(short refN,BoundaryType boundaryType,MIMESHandle msh);
void GenMIMEMap(MIMEMapHandle *mmhp, OSType resType);
void FindMIMEMap(MIMESHandle msh,MIMEMapPtr mmp);
short EncodeUULine(UPtr input,short inLen,UPtr output,PStr newline);
long EncodeUU(UPtr input,long inlen,UPtr output,PStr newline,UUStateHandle uush, Boolean isText);
OSErr FetchAttribute(HeaderDHandle hdh, short attribute, PStr value);
void ExtractFetchFilename(MIMESHandle mimeSList,HeaderDHandle hdh, PStr filename);
void ExtractHDHFilename(MIMESHandle msh,HeaderDHandle hdh,PStr suffix,PStr fName);
Boolean SetupDigest(MIMESHandle msh,HeaderDHandle hdh,short *refPtr);
Boolean FinishDigest(short refN,short origRefN);
OSErr CanonNLWrite(short refN,long *size,UPtr buf);
BoundaryType ReadTL(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr,FSSpecPtr spec,OSType creator,OSType type);
void NukeEnvelopes(UPtr buf,long *size);
void MHTMLStuff(HeaderDHandle outerHDH, HeaderDHandle doubleHDH, HeaderDHandle hdh);

/*
 * functions to find savers & decoders
 */
ReadBodyFunc *FindMIMEBodyFunc(TransStream stream,PStr contentType, PStr contentSubType, Boolean *isExtern, Boolean isAttach, Boolean exMulti);

/*
 * some savers & decoders
 */
ReadBodyFunc ReadExternalMulti, ReadMulti, ReadTLNow, ReadTLNotNow, ReadGeneric, ReadSingle, ReadText, ReadNothing, ReadPGP, ReadExternal, ReadMailServer, ReadAnonFTP;
DecoderFunc B64Decoder, QPDecoder, QPEncoder;

/************************************************************************
 * HuntBoundary - hunt for a given boundary
 ************************************************************************/
BoundaryType HuntBoundary(TransStream stream,short refN,MIMESHandle msh,UPtr buf,long bSize, long *size)
{
	Str127 boundary;
	POPLineType lineType;
	BoundaryType boundaryType;
	OSErr err;

	PCopy(boundary,(*msh)->boundary);
	
	for (lineType=ReadPOPLine(stream,buf,bSize,size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=ReadPOPLine(stream,buf,bSize,size))
	{
		/*
		 * write the line
		 */
		if (err = AWrite(refN,size,buf))
		{
			FileSystemError(WRITE_MBOX,"",err);
			lineType = plError;
			break;
		}
		
		/*
		 * is it the first boundary?
		 */
		if (boundaryType = IsBoundaryLine(lineType,buf,*size,boundary))
			break;
	}
	
	return(lineType==plComplete ? boundaryType : btEndOfMessage);
}

/************************************************************************
 * NewMIMES - instantiate a MIME descriptor for a header
 *  - returns handle to descriptor
 *  - nil if error
 *  - kMIMEBoring if message isn't MIME or it doesn't matter if it's MIME
 *	- can be forced to return MIME header
 ************************************************************************/
MIMESHandle NewMIMES(TransStream stream,HeaderDHandle hdh,Boolean forceMIME,short context)
{
	MIMESHandle msh;
	Str127 scratch;
	OSErr err=noErr;
	MIMEMap mm;
	TLMHandle translators=nil;
	
	/*
	 * maybe we don't have to worry about this
	 */
	if (!LooseTrans && !forceMIME && !(*hdh)->isMIME) return(kMIMEBoring);
	
	if (!*(*(*hdh)->tlMIME)->mimeType)
	{
		// fix up some fake headers
		AddTLMIME((*hdh)->tlMIME,TLMIME_TYPE,GetRString(scratch,MIME_TEXT),nil);
		PSCopy((*hdh)->contentType,scratch);
		AddTLMIME((*hdh)->tlMIME,TLMIME_SUBTYPE,GetRString(scratch,MIME_PLAIN),nil);
		PSCopy((*hdh)->contentSubType,scratch);
	}
		
	msh = NewZHTB(MIMEState);
	if (!msh) {WarnUser(MEM_ERR,MemError()); return(nil);}
	(*msh)->hdh = hdh;
	(*msh)->context = context;
	
	/*
	 * we're going to be messing with these quite a bit
	 */
	LDRef(hdh);
	LDRef(msh);
	
	/*
	 * Is it encoded in a way that we can decode?
	 */
	(*msh)->decoder = FindMIMEDecoder((*hdh)->contentEnco,&(*msh)->xDecoder,True);
		
	/*
	 * Is it a multipart message?  Worry about the boundary
	 */
	if (EqualStrRes((*hdh)->contentType,MIME_MULTIPART) &&	/* multipart */
			!FetchAttribute(hdh,aBoundary,scratch))
	{
		PCopy((*msh)->boundary,"\p--");
		PSCat((*msh)->boundary,scratch);
	}
	
	/*
	 * translator?
	 */
#ifdef ETL
	if (!(ETLListAllTranslators(&translators,context)))
	{
		err = ETLCanTranslate(translators,EMSF_ON_ARRIVAL,(*hdh)->tlMIME,nil,nil,nil,hdh);
	}
	else
#endif
		err = EMSR_CANT_TRANS;
	
	if (err==EMSR_NOT_NOW)
	{
		(*msh)->readBody = ReadTLNotNow;
		(*msh)->translators = translators;
		err = noErr;
	}
	else if (err==EMSR_NOW)
	{
		(*msh)->readBody = ReadTLNow;
		(*msh)->translators = translators;
		err = noErr;
	}
	else
	{
		ZapHandle(translators);
		err = noErr;
		
		/*
		 * Do we need to save this thing to a file?
		 */
		FindMIMEMap(msh,&mm);
		if (mm.flags & mmDiscard) (*msh)->readBody = ReadNothing;
		else
			(*msh)->readBody = FindMIMEBodyFunc(stream,(*hdh)->contentType,(*hdh)->contentSubType,&(*msh)->xFileSaver,(mm.flags&mmAlwaysDetach)||(*hdh)->isAttach,0!=(mm.flags&mmAlwaysDetach));
	}
	
	/*
	 * We're all ready to make up our minds now
	 */
	if (!forceMIME && !(*msh)->readBody && !(*msh)->decoder && !(*msh)->readBody &&
			!*(*msh)->boundary)
	{
		ZapHandle(msh);
		msh = kMIMEBoring;
	}
	else
		UL(msh);
		

done:
	if (msh && msh!=kMIMEBoring) UL(msh);
	UL(hdh);
	
	if (err)
	{
		DisposeMIMES(msh);
		msh = nil;
	}
	
	return(msh);
}

/************************************************************************
 * DisposeMIMES - get rid of a MIME state buffer
 ************************************************************************/
void DisposeMIMES(MIMESHandle msh)
{
	Handle nuke;
	
	if (msh)
	{
		ZapHandle((*msh)->translators);
		if (msh!=kMIMEBoring)
		{
			if ((*msh)->xDecoder)
			{
				nuke = RecoverHandle((Ptr)(*msh)->decoder);
				ZapHandle(nuke);
			}
			if ((*msh)->xFileSaver)
			{
				nuke = RecoverHandle((Ptr)(*msh)->readBody);
				ZapHandle(nuke);
			}
			ZapHandle(msh);
		}
	}
}

/************************************************************************
 * IsBoundaryLine: is a given line a MIME boundary?
 ************************************************************************/
BoundaryType IsBoundaryLine(POPLineType lt, UPtr buf, long len, PStr boundary)
{
	short lDiff;
	UPtr bEnd;
	
	if (lt==plComplete)
	{
		bEnd = buf + len - 1;
		lDiff = len - *boundary;
		if (lDiff == 1 && *bEnd == '\015' ||
				lDiff == 3 && *bEnd == '\015' && bEnd[-1] == '-' && bEnd[-2] == '-')
		{
			if (!strncmp(boundary+1,buf,*boundary)) return(lDiff==1 ? btInnerBoundary : btOuterBoundary);
		}
	}
	return(btNotBoundary);
}

/************************************************************************
 * FindMIMEDecoder - find the decoder for a given content-transfer-encoding
 ************************************************************************/
DecoderFunc *FindMIMEDecoder(PStr encoding,Boolean *isExtern,Boolean load)
{
	Handle xcmd;
	*isExtern = False;
	
	if (xcmd = GetNamedResource(DECODER_RTYPE,encoding))
	{
		if (load)
		{
			DetachResource(xcmd);
			MoveHHi(xcmd);
			*isExtern = True;
			return((DecoderFunc *)LDRef(xcmd));
		}
		else
		{
			ReleaseResource_(xcmd);
			return((DecoderFunc *)1);
		}
	}
	if (EqualStrRes(encoding,MIME_BASE64)) return(B64Decoder);
	if (EqualStrRes(encoding,MIME_QP)) return(QPDecoder);
	return(nil);
}

/************************************************************************
 * FindMIMEBodyFunc - find the function that should save this part to a file
 ************************************************************************/
ReadBodyFunc *FindMIMEBodyFunc(TransStream stream, PStr contentType, PStr contentSubType, Boolean *isExtern, Boolean isAttach, Boolean exMulti)
{
	Handle xcmd;
	Str255 wholeName;
	*isExtern = False;
	
	PCopy(wholeName,contentType);
	PCatC(wholeName,'/');
	PCopy(wholeName,contentSubType);
	
	if ((xcmd = GetNamedResource(SAVER_RTYPE,wholeName)) || 
			(xcmd = GetNamedResource(SAVER_RTYPE,contentType)))
	{
		DetachResource(xcmd);
		MoveHHi(xcmd);
		*isExtern = True;
		return((ReadBodyFunc*)LDRef(xcmd));
	}

	if (EqualStrRes(contentType,MIME_TEXT))
#ifdef OLDPGP
		if (EqualStrRes(contentSubType,PGP_PROTOCOL))
			return(ReadPGP);
		else
#endif
			return(isAttach ? ReadGeneric : ReadText);
	if (EqualStrRes(contentType,MIME_MESSAGE))
	{
		if (EqualStrRes(contentSubType,MIME_PARTIAL))
		{
			NoAttachments=True;
			return(ReadText);
		}
		else if (EqualStrRes(contentSubType,MIME_RFC822))
			return(READ_MESSAGE);
		else if (EqualStrRes(contentSubType,EXTERNAL_BODY))
			return(ReadExternal);
		else
			return(ReadText);
	}
	if (EqualStrRes(contentType,MIME_MULTIPART)) return(exMulti ? ReadExternalMulti:ReadMulti);
	if (EqualStrRes(contentType,MIME_APPLICATION))
	{
		if (EqualStrRes(contentSubType,MIME_APPLEFILE))
			return(ReadSingle);
#ifdef OLDPGP
		else if (EqualStrRes(contentSubType,PGP_PROTOCOL))
			return(ReadPGP);
#endif
		else if (EqualStrRes(contentSubType,MIME_BINHEX) || EqualStrRes(contentSubType,MIME_BINHEX2))
			return(ReadText);
	}
	return(ReadGeneric);
}

/************************************************************************
 * B64Decoder - wrapper for the Base64 decoder
 ************************************************************************/
OSErr B64Decoder(CallType callType,DecoderPBPtr pb)
{
	Dec64Ptr d64p;
	OSErr err=noErr;
	
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				d64p = NuPtrClear(sizeof(Dec64));
				if (!d64p) WarnUser(MEM_ERR,err=MemError());
				pb->refCon = (long)d64p;
				break;
			
			case kDecodeDone:
				err = Decode64(nil,0,pb->output,&pb->outlen,(Dec64Ptr)pb->refCon,pb->text);
				break;
			
			case kDecodeDispose:
				if (pb->refCon)
				{
					DisposePtr((Ptr)pb->refCon);
					pb->refCon = 0;
				}
				break;
			
			case kDecodeData:
				if (pb->inlen)
					err = Decode64(pb->input,pb->inlen,pb->output,&pb->outlen,(Dec64Ptr)pb->refCon,pb->text);
				break;
		}
	}
	
	return(err);
}

/************************************************************************
 * B64Encoder - wrapper for the Base64 decoder
 ************************************************************************/
OSErr B64Encoder(CallType callType,DecoderPBPtr pb)
{
	Enc64Ptr e64p;
	short err;
	
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				e64p = NuPtrClear(sizeof(Dec64));
				if (!e64p) {WarnUser(MEM_ERR,err=MemError());return(err);}
				pb->refCon = (long)e64p;
				break;
			
			case kDecodeDone:
				pb->outlen = Encode64(nil,0,pb->output,pb->noLineBreaks?nil:NewLine,(Enc64Ptr)pb->refCon);
				if (!pb->noLineBreaks && ((Enc64Ptr)pb->refCon)->bytesOnLine)
				{
					BMD(NewLine+1,pb->output+pb->outlen,*NewLine);
					pb->outlen += *NewLine;
				}
				break;
			
			case kDecodeDispose:
				if (pb->refCon)
				{
					DisposePtr((Ptr)pb->refCon);
					pb->refCon = 0;
				}
				break;
			
			case kDecodeData:
				if (pb->inlen)
					pb->outlen = Encode64(pb->input,pb->inlen,pb->output,pb->noLineBreaks?nil:NewLine,(Enc64Ptr)pb->refCon);
				else
					pb->outlen = 0;
				break;
		}
	}
	
	return(noErr);
}

/************************************************************************
 * QPDecoder - wrapper for the Base64 decoder
 ************************************************************************/
OSErr QPDecoder(CallType callType,DecoderPBPtr pb)
{
	DecQPPtr dqpp;
	OSErr err=noErr;
	
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				dqpp = NuPtrClear(sizeof(DecQP));
				if (!dqpp) WarnUser(MEM_ERR,err=MemError());
				pb->refCon = (long)dqpp;
				break;
			
			case kDecodeDone:
				err = DecodeQP(nil,0,pb->output,&pb->outlen,(DecQPPtr)pb->refCon);
				break;
			
			case kDecodeDispose:
				if (pb->refCon)
				{
					DisposePtr((Ptr)pb->refCon);
					pb->refCon = 0;
				}
				break;
			
			case kDecodeData:
				if (pb->inlen)
					err = DecodeQP(pb->input,pb->inlen,pb->output,&pb->outlen,(DecQPPtr)pb->refCon);
				break;
		}
	}
	
	return(err);
}

/************************************************************************
 * QPEncoder - wrapper for the Base64 encoder
 ************************************************************************/
OSErr QPEncoder(CallType callType,DecoderPBPtr pb)
{
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				pb->refCon = 0;
				break;
			
			case kDecodeDone:
				BMD(NewLine+1,pb->output,*NewLine);
				pb->outlen = *NewLine;
				break;
				
			case kDecodeDispose:
				break;
			
			case kDecodeData:
				if (pb->inlen)
					pb->outlen = EncodeQP(pb->input,pb->inlen,pb->output,NewLine,&pb->refCon);
				break;
		}
	}
	
	return(noErr);
}

/************************************************************************
 * UUEncoder - wrapper for the uuencode encoder
 ************************************************************************/
OSErr UUEncoder(CallType callType,DecoderPBPtr pb)
{
	UUStateHandle uush;
	
	if (pb)
	{
		switch (callType)
		{
			case kDecodeInit:
				pb->refCon = (long)NewZH(UUState);
				break;
			
			case kDecodeDone:
				pb->outlen = 0;
				uush = (UUStateHandle)pb->refCon;
				if ((*uush)->leftBytes) pb->outlen = EncodeUULine(&LDRef(uush)->buffer,(*uush)->leftBytes,pb->output,NewLine);
				pb->output[pb->outlen++] = '`';
				BMD(NewLine+1,pb->output+pb->outlen,*NewLine); pb->outlen+=*NewLine;
				BMD("end",pb->output+pb->outlen,3); pb->outlen+=3;
				BMD(NewLine+1,pb->output+pb->outlen,*NewLine); pb->outlen+=*NewLine;
				(*uush)->leftBytes = 0;
				break;
				
			case kDecodeDispose:
				DisposeHandle((Handle)pb->refCon);
				break;
			
			case kDecodeData:
				if (pb->inlen)
					pb->outlen = EncodeUU(pb->input,pb->inlen,pb->output,NewLine,(UUStateHandle)pb->refCon,pb->text);
				break;
		}
	}
	return(noErr);
}


/************************************************************************
 * Encode64Data - encode a string in Base64
 ************************************************************************/
PStr Encode64Data(PStr encoded,UPtr data,short len)
{
	DecoderPB pb;
	
	Zero(pb);
	pb.input = data;
	pb.output = encoded+1;
	pb.inlen = len;
	pb.noLineBreaks = true;
	B64Encoder(kDecodeInit,&pb);
	B64Encoder(kDecodeData,&pb);
	*encoded = pb.outlen;
	pb.output = encoded+*encoded+1;
	B64Encoder(kDecodeDone,&pb);
	*encoded += pb.outlen;
	B64Encoder(kDecodeDispose,&pb);
	return(encoded);
}

/************************************************************************
 * Encode64DataPtr - encode lots o data in Base64
 ************************************************************************/
void Encode64DataPtr(UPtr encoded,long *outLen,UPtr data,short len)
{
	DecoderPB pb;
	
	Zero(pb);
	pb.input = data;
	pb.output = encoded;
	pb.inlen = len;
	pb.noLineBreaks = true;
	B64Encoder(kDecodeInit,&pb);
	B64Encoder(kDecodeData,&pb);
	*outLen = pb.outlen;
	pb.output = encoded+pb.outlen;
	B64Encoder(kDecodeDone,&pb);
	*outLen += pb.outlen;
	B64Encoder(kDecodeDispose,&pb);
	return;
}

/************************************************************************
 * EncodeUU - encode some bytes with uuencode
 ************************************************************************/
long EncodeUU(UPtr input,long inlen,UPtr output,PStr newline,UUStateHandle uush, Boolean isText)
{
	UPtr spot = output;
	long outlen=0;
	long count;
	short fragment;
	
	/*
	 * send leftovers
	 */
	if ((*uush)->leftBytes)
	{
		fragment = MIN(inlen,45-(*uush)->leftBytes);
		BMD(input,(*uush)->buffer+(*uush)->leftBytes,fragment);
		(*uush)->leftBytes += fragment;
		input += fragment;
		inlen -= fragment;
		if ((*uush)->leftBytes==45)
		{
			count = EncodeUULine(&LDRef(uush)->buffer,45,output,newline);
			output += count;
			outlen += count;
			(*uush)->leftBytes = 0;
		}
		else return(0);
	}
	
	if (!isText)
	{
		/*
		 * send full lines
		 */
		for (;inlen>=45;inlen-=45,input+=45)
		{
			count = EncodeUULine(input,45,output,newline);
			output += count;
			outlen += count;
		}
	
		/*
		 * save leftovers
		 */
		BMD(input,(*uush)->buffer,inlen);
		(*uush)->leftBytes = inlen;
	}
	else
	{
		Str63 lineBuffer;
		Byte c;
		UPtr lineSpot,lineEnd;
		
		lineSpot = lineBuffer;
		lineEnd = lineBuffer+45;
		while (inlen--)
		{
			c = *lineSpot++ = *input++;
		maySend:
			if (lineSpot==lineEnd)
			{
				count = EncodeUULine(lineBuffer,45,output,newline);
				output += count;
				outlen += count;
				lineSpot = lineBuffer;
			}
			if (c=='\015')
			{
				c = *lineSpot++ = '\012';	/* add linefeed */
				goto maySend;						/* ooh, I feel so naughty! */
			}
		}
			
		/*
		 * save leftovers
		 */
		BMD(lineBuffer,(*uush)->buffer,lineSpot-lineBuffer);
		(*uush)->leftBytes = lineSpot-lineBuffer;
	}
	return(outlen);
}

/************************************************************************
 * EncodeUULine - uuencode some data into a line
 ************************************************************************/
short EncodeUULine(UPtr input,short inLen,UPtr output,PStr newline)
{
	short outLen;
	short bpl = 0;
	
	if (!inLen) outLen = 0;
	else
	{
		/*
		 * count bytes
		 */
		outLen = 4*((inLen+2)/3);
				
		/*
		 * insert the length byte
		 */
		*output++ = gUUEncode[inLen];
		if (!UUPCOut && output[-1]=='.') *output++ = '.';	// double dots
		
		/*
		 * now, the encoding
		 */
		for(;inLen>0;inLen-=3,input+=3)
			EncodeThreeUU(input,output,bpl,newline);
		BMD(newline+1,output,*newline);
		output += *newline;
		
		outLen += 1+*newline;
	}
	
	return(outLen);
}

			
/************************************************************************
 * FindMIMECharset - find the right xlate table for a particular MIME char set
 ************************************************************************/
short FindMIMECharsetLo(PStr charset,Boolean *found)
{
	Handle resH;
	short id;
	OSType type;

	// cross our fingers and set the found flag now;
	// we'll clear it later if need be
	if (found) *found = true;
	
	if (EqualStrRes(charset,MIME_ISO_LATIN1)) return(TRANS_IN_TABL);

	if (EqualStrRes(charset,MIME_ISO_LATIN15)) return(ktISO15Mac);

	if (EqualStrRes(charset,MIME_WIN_1252)) return(ktWindowsMac);

	/*
	 * Mac?
	 */
	if (EqualStrRes(charset,MIME_MAC)) return(NO_TABLE);
	
	/*
	 * use canonical name to id mapping table
	 */
	if (GetTableID(charset,&id))
		if (resH = GetResource_('taBL',id)) return(id%2 ? id : id-1);

	/*
	 * find named resource for the table in question, if any
	 */
	if (resH = GetNamedResource('taBL',charset))
	{
		GetResInfo(resH,&id,&type,charset);
		if ((id%2)==0) id--;
		if (id>2999) id -= 1000;	/* map x- sets onto real ones */
		if (GetResource_('taBL',id)) return(id);
	}

	/*
	 * if a Eudora-specific table has been used, it might have x- in front of it
	 * remove any "x-"
	 */
	if (*charset>2 && (charset[1]=='x' || charset[1]=='X') && charset[2]=='-')
	{
		*charset -= 2;
		BMD(charset+3,charset+1,*charset);
	}
	
	/*
	 * find resource
	 */
	if (resH = GetNamedResource('taBL',charset))
	{
		GetResInfo(resH,&id,&type,charset);
		if ((id%2)==0) id--;
		if (id>2999) id -= 1000;			/* map x- sets onto real ones */
		if (GetResource_('taBL',id)) return(id);
	}

	// not found.  Make sure the caller knows
	if (found) *found = false;
	
	return(NO_TABLE);
}

/************************************************************************
 * The ReadBodyFunc's - read the body of a multipart MIME message.
 *  We have now finished looking at the header of a message, and have discovered
 *  that is has something MIME-ish about it.
 *  refN - the ref number of the open mailbox file may be written to or seeked,
 *    as we please
 *	mimeSList - handle to the OUTERMOST MIME state block.  The state block
 *		for our current message is at the end of the chain of which this is the
 *		head
 *	buf - a buffer to use for reading
 *	bSize - how big the buffer is
 *	lr - line reading function
 *  Returns LineType of last line read.  The useful information this conveys
 *  	is whether or not there has been an error (plError) or whether or not the message
 *  	has ended (plEndOfMessage)
 ************************************************************************/

/************************************************************************
 * ReadMulti - read a multipart message body
 ************************************************************************/
BoundaryType ReadMulti(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
#pragma unused(lr)
	long size;
	BoundaryType boundaryType;
	MIMESHandle msh;
	HeaderDHandle innerHDH=nil;
	MIMESHandle innerMSH=nil;
	HeaderDHandle hdh;
	short err=noErr;
	long offset;
	Boolean wasApplefile = False;
	short origRefN = refN;
	Boolean isDigest=False;
	Boolean mightDigest=False;
	Boolean digestTop;
	long len;
	short hState;
	short altSubType=0;
	long altOffset=0;
	Boolean alternative;
	long dates[3];
	CInfoPBRec hfi;
	FSSpec spec;
	Boolean first = True;
	Boolean related;
	Boolean areDouble;
	MIMESHandle pMSH;
	
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;
	ReadPOPLine(stream,nil,0,nil);
	
	/*
	 * digest processing
	 */
	mightDigest = EqualStrRes(LDRef(hdh)->contentSubType,MIME_DIGEST); UL(hdh);
	alternative = EqualStrRes(LDRef(hdh)->contentSubType,MIME_ALTERNATIVE); UL(hdh);
	related = EqualStrRes(LDRef(hdh)->contentSubType,MIME_RELATED); UL(hdh);
	
	if (related) (*hdh)->mhtmlID = ++(*mimeSList)->mhtmlID;
	
	if (mightDigest && PrefIsSet(PREF_OLD_DIGEST))
	{
		(*msh)->isDigest = True;
		mightDigest = False;
	}

	LL_Parent(mimeSList,msh,pMSH);
	if (!pMSH) pMSH = msh;					/* this may seem funny, but it will work */
	areDouble = EqualStrRes(LDRef(hdh)->contentSubType,MIME_APPLEDOUBLE) ||
							EqualStrRes(LDRef(hdh)->contentSubType,MIME_HEADERSET);
	if (areDouble)
	{
		related = EqualStrRes(LDRef((*pMSH)->hdh)->contentSubType,MIME_RELATED);	// revisit this
		UL((*pMSH)->hdh);
		(*hdh)->mhtmlID = (*pMSH)->mhtmlID;	// do NOT increment the mhtmlid!
	}
	UL(hdh);
	
	// x-folder
	if (!AttFolderStack)
	{
		// we're at the top level; push the base attachment folder onto the stack
		if (!StackInit(sizeof(FSSpec),&AttFolderStack))
		{
			CurrentAttFolderSpec = AttFolderSpec;
			StackPush(&AttFolderSpec,AttFolderStack);
		}
	}
	else
	{
		long newDirID;
		
		// inside.  Push the current folder spec
		StackPush(&CurrentAttFolderSpec,AttFolderStack);
		
		// are we enclosed by an x-folder?
		if (StrIsItemFromRes(LDRef((*msh)->hdh)->contentSubType,X_FOLDER_ITEMS,nil))
		{
			// yes.  Grab its name
			ExtractHDHFilename(pMSH,(*msh)->hdh,0,&spec.name);
			
			// uniquify it
			AutoWantTheFile(&spec,true,false);
			
			// make it
			if (FSpDirCreate(&spec,smSystemScript,&newDirID))
				// failed to make it; continue with old attachment folder.  This sucks
				StackItem(&CurrentAttFolderSpec,(*AttFolderStack)->elCount,AttFolderStack);
			else
			{
				// made it.
				// record it
				RecordAttachment(&spec,(*msh)->hdh);
				
				// Zero the name in the spec, since we want the folder itself
				*CurrentAttFolderSpec.name = 0;
				CurrentAttFolderSpec.parID = newDirID;
			}
		}
		UL((*msh)->hdh);
	}
	
	/*
	 * Skip introductory text
	 */
	boundaryType = HuntBoundary(stream,refN,msh,buf,bSize,&size);
	if (boundaryType==btInnerBoundary)
	{
		/*
		 * we have found the first boundary
		 */
		
#ifdef SAVE_MIME
		if (isDigest)
			/* remove text between header and multipart intro */
			TruncOpenFile(refN,(msh!=mimeSList) ?
														(*hdh)->diskStart :
														(*hdh)->diskEnd);
#else
			/* remove text between header and multipart intro */
			/* or, if we understood the whole header, remove it */
			TruncOpenFile(refN,(msh!=mimeSList && (*hdh)->grokked) ?
														(*hdh)->diskStart :
														(*hdh)->diskEnd);
#endif

		/*
		 * prepare to digest
		 */				
		if (mightDigest) isDigest = SetupDigest(msh,hdh,&refN);

		/*
		 * we have read a boundary.  What follows will be a message header
		 * (probably; else we have to infer one :-().  Read the header, and
		 * [re]curse.  We continue with this until we find the end of the
		 * message or a terminal boundary
		 */
		while (boundaryType==btInnerBoundary)
		{
			if (wasApplefile) GetFPos(refN,&offset);

			/*
			 * digest?
			 */
			if (isDigest)
				PutOutFromLine(refN,&len);
#ifdef SAVE_MIME // if reading MIME, first boundary already written
			else if (!first && (err=WriteBoundary(refN,boundaryType,msh))) goto done;
#else
			else if (err=WriteBoundary(refN,boundaryType,msh)) goto done;
#endif
			
			first = False;	// we've seen the first boundary

			/*
			 * prepare a new header desc and read the header
			 */
			digestTop = True;
		reRead:
			if (!(innerHDH = NewHeaderDesc(hdh))) {WarnUser(MEM_ERR,err=MemError()); break;}
			hState = ReadHeader(stream,innerHDH,0,refN,(*msh)->isDigest || isDigest&&digestTop);			
			if (hState!=EndOfHeader)
			{
				if (hState==EndOfMessage) boundaryType = btEndOfMessage;
				else err = 1;
				break;
			}
			
			MHTMLStuff(areDouble?(*pMSH)->hdh:hdh,areDouble?hdh:nil,innerHDH);
			
			/*
			 * extract MIME info
			 */
			if (related) (*innerHDH)->relatedPart = True;
			if (!(innerMSH = NewMIMES(stream,innerHDH,True,(*msh)->context))) {err=1;break;}
			if ((*innerMSH)->readBody==READ_MESSAGE)
			{
				digestTop = EqualStrRes(LDRef(innerHDH)->contentSubType,MIME_RFC822);
				if (digestTop) TruncOpenFile(refN,(*innerHDH)->diskStart);
				ZapHeaderDesc(innerHDH);
				ZapMIMES(innerMSH);
				goto reRead;
			}
			LL_Queue(mimeSList,innerMSH,(MIMESHandle));
/*PrintMIMEList("\pReadMulti 1",mimeSList);*/

			/*
			 * alternative processing
			 */
			if (alternative)
			{
				/*
				 * do we like this better than the last one?
				 */
				LDRef(innerHDH);
				if ((*innerMSH)->readBody==ReadText)
				{
					short newtype;
					
					if (EqualStrRes((*innerHDH)->contentSubType,MIME_PLAIN))
						newtype = MIME_PLAIN;
					else if (EqualStrRes((*innerHDH)->contentSubType,MIME_RICHTEXT))
						newtype = MIME_RICHTEXT;
					else if (EqualStrRes((*innerHDH)->contentSubType,HTMLTagsStrn+htmlTag))
						newtype = HTMLTagsStrn+htmlTag;
					else
						newtype = 0;
					
					UL(innerHDH);
					
					if (newtype==MIME_RICHTEXT || newtype==HTMLTagsStrn+htmlTag&&altSubType!=MIME_RICHTEXT || !altSubType)
					{
						/*
						 * we like the new type better
						 */
						// throw away what we already have
						if (altOffset)
						{
							CopyFBytes(refN,(*innerHDH)->diskStart,(*innerHDH)->diskEnd-(*innerHDH)->diskStart,refN,altOffset);
							(*innerHDH)->diskEnd = altOffset + (*innerHDH)->diskEnd-(*innerHDH)->diskStart;
							(*innerHDH)->diskStart = altOffset;
						}
						// record our stuff
						altOffset = (*innerHDH)->diskStart;
						altSubType = newtype;
					}
					else
					{
						/*
						 * we like what we already have better than what we're getting now
						 */
						SetFPos(refN,fsFromStart,(*innerHDH)->diskStart);	// toss header
						(*innerMSH)->readBody=ReadNothing;
						if (newtype==HTMLTagsStrn+htmlTag) AnyHTML = False;
					}
				}
				else altOffset = 0;
			}
						
			
			/*
			 * are we appledouble?
			 * if so, record the last attachment we grabbed in SingleSpec, so
			 * ReadGeneric will pick it up later
			 */
			if (wasApplefile && areDouble && LastAttSpec && !FSpIsItAFolder(*LastAttSpec))
				SingleSpec = LastAttSpec;
			else
				SingleSpec = nil;
			UL(hdh);
			
			if (SingleSpec)
			{
				// store dates
				Zero(hfi);
				spec = **SingleSpec;
				HGetCatInfo(spec.vRefNum,spec.parID,spec.name,&hfi);
				dates[0] = hfi.hFileInfo.ioFlCrDat;
				dates[1] = hfi.hFileInfo.ioFlMdDat;
				dates[2] = hfi.hFileInfo.ioFlBkDat;
			}
			
			/*
			 * read the body of the message
			 */
			boundaryType = SingleSpec
										 ? ReadGeneric(stream,refN,mimeSList,buf,bSize,ReadPOPLine)
										 : (*(*innerMSH)->readBody)(stream,refN,mimeSList,buf,bSize,ReadPOPLine);
			/*
			 * clean up
			 */
			if (SingleSpec && !BadBinHex)
			{
				TruncOpenFile(refN,offset);
				// restore dates
				if (!HGetCatInfo(spec.vRefNum,spec.parID,spec.name,&hfi))
				{
					hfi.hFileInfo.ioFlCrDat = dates[0];
					hfi.hFileInfo.ioFlMdDat = dates[1];
					hfi.hFileInfo.ioFlBkDat = dates[2];
					HSetCatInfo(spec.vRefNum,spec.parID,spec.name,&hfi);
				}
				//	Appledouble attachments get counted statistically for each fork.
				//	Subtract back out each additional fork.
				UpdateNumStatWithTime(kStatReceivedAttach,-1,hdh?(*hdh)->gmtSecs+ZoneSecs():LocalDateTime());
			}
					
			wasApplefile = EqualStrRes(LDRef(innerHDH)->contentSubType,MIME_APPLEFILE);
			if (!wasApplefile) SingleSpec = nil;
/*PrintMIMEList("\pReadMulti2",mimeSList);*/
			LL_Remove(mimeSList,innerMSH,(MIMESHandle));
			ZapHeaderDesc(innerHDH);
			ZapMIMES(innerMSH);
			SingleSpec = nil;
		}
	}
	
	if (boundaryType==btOuterBoundary)
	{
		
		/*
		 * write out the final boundary
		 */
		if (err=WriteBoundary(refN,boundaryType,msh)) goto done;
		
		/*
		 * we have finished our multipart.
		 * if we are part of a larger multipart, then we must now hunt for the
		 * larger multipart's boundary.  If not, we hunt for the end of the
		 * message.  In either case, HuntBoundary will do it for us.
		 */		
		GetFPos(refN,&offset);					/* remember where we started tossing */
		do
		{
			boundaryType = HuntBoundary(stream,refN,pMSH,buf,bSize,&size);
		}
		while (pMSH == msh && boundaryType < btEndOfMessage);
		/* this condition is the way it is because, if we are the outermost
		 * multipart, we need to keep reading until end of message */
		
#ifndef SAVE_MIME
		TruncOpenFile(refN,offset);		/* toss the excess bytes */
#endif
	}

done:
	if (isDigest) FinishDigest(refN,origRefN);
	if (innerHDH) ZapHeaderDesc(innerHDH);
	if (innerMSH) ZapMIMES(innerMSH);
	if (AttFolderStack) StackPop(&CurrentAttFolderSpec,AttFolderStack);
	return(err ? btError : boundaryType);
}
	
/************************************************************************
 * MHTMLStuff - do MHTML stuff
 ************************************************************************/
void MHTMLStuff(HeaderDHandle outerHDH, HeaderDHandle doubleHDH, HeaderDHandle hdh)
{
	Str255 val, val2;
	uLong hash, hash2;
	UHandle valH=nil;
	if (!doubleHDH) doubleHDH = hdh;
	
	if (!AAFetchResData((*doubleHDH)->funFields,InterestHeadStrn+hContentId,val))
	if (!SuckPtrAddresses(&valH,val+1,*val,False,False,False,nil))
	{
		PCopy(val,*valH);
		ZapHandle(valH);
		MyLowerStr(val);
		hash = HashWithSeed(val,(*outerHDH)->mhtmlID);
		(*hdh)->cidHash = hash;
	}
	
	*val = *val2 = 0;
	hash = hash2 = 0;

	AAFetchResData((*doubleHDH)->funFields,InterestHeadStrn+hContentLocation,val);
	if (AAFetchResData((*doubleHDH)->funFields,InterestHeadStrn+hContentBase,val2) &&
			!AAFetchResData((*outerHDH)->funFields,InterestHeadStrn+hContentBase,val2))
		AAAddResItem((*hdh)->funFields,True,InterestHeadStrn+hContentBase,val2);
	
	TrimWhite(val); TrimInitialWhite(val);
	TrimWhite(val2); TrimInitialWhite(val2);
	
	/*
	 * hash of content-location alone
	 */
	if (*val)
	{
		MyLowerStr(val);
		hash = HashWithSeed(val,(*outerHDH)->mhtmlID);
	}
	
	/*
	 * hash of content-location + content-base, or repeat content-location if no content-base
	 */
	if (*val && *val2)
	{
		URLCombine(val,val2,val);
		MyLowerStr(val);
		hash2 = HashWithSeed(val,(*outerHDH)->mhtmlID);
	}
	else
		hash2 = hash;
		
	(*hdh)->absURLHash = hash2;
	(*hdh)->relURLHash = hash;
	(*hdh)->mhtmlID = (*outerHDH)->mhtmlID;
}

/**********************************************************************
 * SetupDigest - get ready to receive a digest.  If we fail, it will just go in
 *  the regular mailbox
 **********************************************************************/
Boolean SetupDigest(MIMESHandle msh,HeaderDHandle hdh,short *refPtr)
{
	FSSpec spec;
	short refN;
	OSErr err;

	/*
	 * fish out the filename
	 */
	ExtractHDHFilename(msh,hdh,nil,spec.name);
	*spec.name = MIN(*spec.name,24);
	if (!AutoWantTheFile(&spec,False,(*hdh)->relatedPart)) return(False);

	/*
	 * create the file
	 */
	if (err=FSpCreate(&spec,CREATOR,'TEXT',smSystemScript))
		return(False);
	
	/*
	 * open the file
	 */
	if (err=FSpOpenDF(&spec,fsRdWrPerm,&refN)) return(False);
	
	/*
	 * success
	 */
	*refPtr = refN;
	return(True);
}

/**********************************************************************
 * FinishDigest - clean up after writing a digest
 **********************************************************************/
Boolean FinishDigest(short refN,short origRefN)
{
	FSSpec spec;
	OSErr err;
	
	GetFileByRef(refN,&spec);
	MyFSClose(refN);
	err = RecordAttachment(&spec,nil);
	WriteAttachNote(origRefN);
	PopProgress(False);
	return err == noErr;
}

/************************************************************************
 * ReadExternal - read an external message body
 ************************************************************************/
BoundaryType ReadExternal(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	MIMESHandle msh;
	HeaderDHandle hdh;
	Str127 access;	
	
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;

	FetchAttribute(hdh,aAccessType,access);
		
	if (EqualStrRes(access,ANON_FTP))
		return(ReadAnonFTP(stream,refN,mimeSList,buf,bSize,lr));
	
	if (EqualStrRes(access,MAIL_SERVER))
		return(ReadMailServer(stream,refN,mimeSList,buf,bSize,lr));
	
	return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
}

/************************************************************************
 * ReadMailServer - read a mailserver external body
 ************************************************************************/
BoundaryType ReadMailServer(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
#pragma unused(lr)
	BoundaryType boundaryType;
	MIMESHandle msh;
	HeaderDHandle hdh;
	short err=noErr;
	Str255 s;
	short body=0;
	POPLineType lineType;
	long size;
	Str255 bound;
	MIMESHandle parentMSH;
	Accumulator url;
	Byte separator='?';
	
	
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;

	/*
	 * fetch the particulars
	 */
	if (FetchAttribute(hdh,aServer,s) || AccuInit(&url))
		return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
	
#ifndef SAVE_MIME
	/*
	 * is the header superfluous?
	 */
	if (msh!=mimeSList)
		TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif

	// <mailto:
	AccuAddChar(&url,'<');
	AccuAddRes(&url,ProtocolStrn+proMail);
	AccuAddChar(&url,':');
	
	// the server
	URLEscape(s);
	AccuAddStr(&url,s);
	
	// subject?
	if (!FetchAttribute(hdh,aSubject,s))
	{
		AccuAddChar(&url,separator);
		separator = ';';
	}
			
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
	
	/*
	 * now, read the message
	 */
	ReadPOPLine(stream,nil,0,nil);
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
						
		/*
		 * header?
		 */
		if (!body)
		{
			if (size==1 && *buf == '\015')
			{
				body = 1;
			}
		}
		else
		{
			// we are now in the body
			MakePStr(s,buf,size);
			if (s[*s]=='\015') --*s;
			if (*s>0)
			{
				if (body==1)
				{
					// got to add body=
					AccuAddChar(&url,separator);
					AccuAddRes(&url,MAILTO_BODY);
					AccuAddChar(&url,'=');
				}
				URLEscape(s);
				if (body==2) PInsert(s,sizeof(s),"\p%0D%0A",s+1);
				AccuAddStr(&url,s);
				body = 2;	// done adding body=
			}
		}
	}
	if (lineType==plError) boundaryType=btError;
	
	if (!err && boundaryType!=btError)
	{
		// add the trailing '>'
		AccuAddChar(&url,'>');
		// and write it
		FSWriteP(refN,GetRString(s,EXTERNAL_MAIL));
		err = AccuWrite(&url,refN);
	}
	
	AccuZap(url);
	
	/*
	 * report error (if any)
	 */
  if (err) FileSystemError(WRITE_MBOX,"",err);
	
	return(err ? btError : boundaryType);
}

/**********************************************************************
 * 
 **********************************************************************/
void ExtractFetchFilename(MIMESHandle mimeSList,HeaderDHandle hdh, PStr filename)
{
	Str255 scratch, longName;

	/*
	 * extract name from header
	 */
	if (FetchAttribute(hdh,aName,scratch))
		PCopy(scratch,(*(*mimeSList)->hdh)->subj);
	Other2MacName(scratch,scratch);
	if (!*scratch) GetRString(scratch,UNTITLED);
	
	/*
	 * prefix the filename
	 */
	ComposeRString(longName,FETCH_FN_FMT,scratch);
	*longName = MIN(31,*longName);
	PCopy(filename,longName);
}

/************************************************************************
 * ReadAnonFTP - read anonymous ftp external-body type
 ************************************************************************/
BoundaryType ReadAnonFTP(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	MIMESHandle msh;
	HeaderDHandle hdh;
	short err=noErr;
	Str127 site, name, mode, directory;
	MessHandle messH = nil;
	Boolean body = False;
	short attachRefN=0;
	Str255 command;
	BoundaryType bt;
	
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;
	
	bt = ReadNothing(stream,refN,mimeSList,buf,bSize,lr);

	if (bt!=btError)
	{
		// collect the relevant params
		if (FetchAttribute(hdh,aSite,site) ||
				FetchAttribute(hdh,aName,name))
			return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
		*mode = 0; FetchAttribute(hdh,aMode,mode);
		*directory = 0; FetchAttribute(hdh,aDirectory,directory);
		FSWriteP(refN,GetRString(command,EXTERNAL_FTP));

		// compose a url
		*command = 0;
		PCatC(command,'<');
		PCatR(command,ANARCHIE_FTP);
		PCatC(command,':');
		PCatC(command,Slash[1]);
		PCatC(command,Slash[1]);
		PCat(command,site);
		if (directory[1]!='/') PCatC(command,Slash[1]);
		PCat(command,directory);
		if (directory[*directory]!='/' && name[1]!='/') PCatC(command,Slash[1]);
		PCat(command,name);
		PCatC(command,'>');
		PCat(command,Cr);
		
		// and write it out
		FSWriteP(refN,command);
	}
	
	return(bt);
}

/************************************************************************
 * FetchAttribute - grab the value of an attribute from a header
 ************************************************************************/
OSErr FetchAttribute(HeaderDHandle hdh, short attribute, PStr value)
{
	return(AAFetchResData((*hdh)->contentAttributes,AttributeStrn+attribute, value));
}

/************************************************************************
 * ReadText - read a plain text MIME message body
 ************************************************************************/
BoundaryType ReadText(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	DecoderPB pb;
	MIMESHandle msh;
	Boolean attach;
	Boolean hexing,singling,pgping;
	POPLineType lineType;
	BoundaryType boundaryType=btEndOfMessage;
	MIMESHandle parentMSH;
	Str127 bound;
	Str63 charset;
	Str31 macCharset;
	long size;
	short err=0;
	Boolean decode;
	Handle xlate=nil;
	MIMEMapPtr mmp=nil;
#ifdef OLDPGP
	PGPContext pgp;
#endif
	MIMEMap mm;
	Boolean isUU;
	long offsetBeforeMarkup;
	long offsetAfterMarkup;
	long whiteCount = 0;
	long offsetWhenDone;
	
	ReadPOPLine(stream,nil,0,nil);	
	LL_Last(mimeSList,msh);
	attach = !NoAttachments && !(*(*msh)->hdh)->isPartial;
#ifndef SAVE_MIME
	if(!(*(*msh)->hdh)->hasCharset) {
		xlate = GetResource_('taBL',(*(*msh)->hdh)->xlateResID);
		if (xlate) HNoPurge_(xlate);
	}
#endif

	/*
	 * uudecode?
	 */
	PCopy(bound,(*(*msh)->hdh)->contentEnco);
	if (isUU = EqualStrRes(bound,X_UUENCODE)||FindSTRNIndex(WhatWillLotusCallItNextStrn,bound))
	{
		FindMIMEMap(msh,&mm);
		mmp = &mm;
	}
	
	/*
	 * initialize the decoder
	 */
	decode = !NoAttachments && (*msh)->decoder;
	if (decode && (*(*msh)->decoder)(kDecodeInit,&pb))
		return(btError);
	else pb.output = pb.input = buf;
	pb.text = True;
	
#ifndef SAVE_MIME
	/*
	 * is the header superfluous?
	 */
	if (msh!=mimeSList && (*(*msh)->hdh)->grokked)
		TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif
	
	/*
	 * do we want attachments?
	 */
	if (attach)
	{
		BeginHexBin((*msh)->hdh);
		BeginAbomination("",(*msh)->hdh);
#ifdef OLDPGP
		BeginPGP(&pgp);
#endif
		hexing = singling = pgping = False;
	}
	
	/*
	 * write out rich text delimiter, if need be
	 */
	GetFPos(refN,&offsetBeforeMarkup);
	if(!(*(*msh)->hdh)->hasCharset)
	{
		if (!PrefIsSet(PREF_ALWAYS_CHARSET) && (*(*msh)->hdh)->xlateResID)
			// we are transliterating.  Mention this to the html interpreter with
			// a charset indicating what we think we're transliterating from and to
			ComposeRString(charset,CHARSET_FLUX_FMT,SimpleNameCharset(GlobalTemp,(*(*msh)->hdh)->xlateResID),SimpleNameCharset(macCharset,0));
		else
			charset[0]=0;
	}
	else if(AAFetchResData((*(*msh)->hdh)->contentAttributes,AttributeStrn+aCharSet,charset) != noErr)
	{
		GetRString(charset, UNSPECIFIED_CHARSET);
	}
	if ((*(*msh)->hdh)->hasRich)
	{
		FSWriteP(refN,ComposeRString(buf,MIME_RICH_ON,EnrichedStrn+enXRich));
		if((*(*msh)->hdh)->hasCharset)
			FSWriteP(refN,ComposeRString(buf,MIME_RICH_PARAM,charset));
	}
	else if ((*(*msh)->hdh)->hasHTML)
	{
		Str255 base, loc;
		FSWriteP(refN,ComposeRString(buf,MIME_RICH_ON,EnrichedStrn+enXHTML));
		*base = *loc = 0;
		AAFetchResData((*(*msh)->hdh)->funFields,InterestHeadStrn+hContentBase,base);
		AAFetchResData((*(*msh)->hdh)->funFields,InterestHeadStrn+hContentLocation,loc);
		GetRString(buf,MHTML_INFO_TAG);
		URLEscape(base);
		URLEscape(loc);
		if (*buf + *base + *loc > 255)
		{
			// trouble in river city
			if (*base>*loc) *base = 0;
			else *loc = 0;
			if (*buf + *base + *loc > 255)
				*loc = *base = 0;
		}
		FSWriteP(refN,ComposeRString(buf,MHTML_INFO_TAG,base,loc,(*(*msh)->hdh)->mhtmlID,charset));
	}
	else if ((*(*msh)->hdh)->hasFlow || (*(*msh)->hdh)->hasCharset)
	{
		short resID, fmtID;
		
		fmtID = (*(*msh)->hdh)->hasCharset ? MIME_FLOWED_ON : MIME_RICH_ON;
		resID = (*(*msh)->hdh)->hasFlow ? EnrichedStrn+enXFlowed : EnrichedStrn+enXCharset;
		EnsureNewline(refN);
		FSWriteP(refN,ComposeRString(buf,fmtID,resID,charset));
	}
	GetFPos(refN,&offsetAfterMarkup);

	
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
	
	/*
	 * now, read the message
	 */
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
		
		/*
		 * decode
		 */
		if (decode)
		{
			pb.inlen = size;
			BadEncoding += (*(*msh)->decoder)(kDecodeData,&pb);
			size = pb.outlen;
		}
		
		/*
		 * give each converter a crack at the line
		 */
		if (attach)
		{
			if (!(singling || pgping)) hexing = ConvertHexBin(refN,buf,&size,lineType,0);
			if (!(hexing || pgping)) singling = ConvertUUSingle(refN,buf,&size,lineType,0,mmp,(*msh)->hdh);
#ifdef OLDPGP
			if (!(singling || hexing)) pgping = ConvertPGP(refN,buf,&size,lineType,0,&pgp);
#endif
		}
		
		/*
		 * convert
		 */
		if (xlate)
		{
			TransLit(buf,size,LDRef(xlate));
			UL(xlate);
		}
		
		/*
		 * protect ourselves from envelopes
		 */
		if (decode) NukeEnvelopes(buf,&size);
		
		/*
		 * write the line
		 */
		if (size && (err=AWrite(refN,&size,buf)))
			break;
		
		/*
		 * keep track if we're writing only whitespace
		 */
		if (whiteCount >= 0)
			if (!IsAllLWSPPtr(buf,size)) whiteCount = -1;
			else whiteCount += size;
	}
	if (lineType==plError) boundaryType=btError;
	if (xlate) HPurge(xlate);
	
	/*
	 * close converters
	 */
	if (attach)
	{
		EndHexBin();
		SaveAbomination(nil,0);
#ifdef OLDPGP
		EndPGP(&pgp);
#endif
		WriteAttachNote(refN);
	}
	if (decode)
	{
		BadEncoding += (*(*msh)->decoder)(kDecodeDone,&pb);
		(*(*msh)->decoder)(kDecodeDispose,&pb);
	}
	
	/*
	 * write out rich text delimiter, if need be
	 */
	GetFPos(refN,&offsetWhenDone);
	if (offsetWhenDone == offsetAfterMarkup+whiteCount)
	{
		SetFPos(refN,fsFromStart,offsetBeforeMarkup);
		SetEOF(refN,offsetBeforeMarkup);
		(*(*msh)->hdh)->hasRich = (*(*msh)->hdh)->hasHTML = (*(*msh)->hdh)->hasFlow = (*(*msh)->hdh)->hasCharset = false;
	}
	else
	{
		*buf = 0;
		if ((*(*msh)->hdh)->hasRich)
			ComposeRString(buf,MIME_RICH_OFF,EnrichedStrn+enXRich);
		else if ((*(*msh)->hdh)->hasHTML)
			ComposeRString(buf,MIME_RICH_OFF,EnrichedStrn+enXHTML);
		else if ((*(*msh)->hdh)->hasFlow)
			ComposeRString(buf,MIME_RICH_OFF,EnrichedStrn+enXFlowed);
		else if ((*(*msh)->hdh)->hasCharset)
			ComposeRString(buf,MIME_RICH_OFF,EnrichedStrn+enXCharset);
		if (*buf)
		{
			EnsureNewline(refN);
			PCatC(buf,'\015');
			FSWriteP(refN,buf);
		}
	}
	
	/*
	 * report error (if any)
	 */
  if (err) FileSystemError(WRITE_MBOX,"",err);
// progress is now cumulative
//	if (msh==mimeSList && !UUPCIn) Progress(100,NoChange,nil,nil,nil);
/*PrintMIMEList("\pReadText",mimeSList);*/
	return(err ? btError : boundaryType);
}

/**********************************************************************
 * NukeEnvelopes - kill envelopes in mail
 **********************************************************************/
void NukeEnvelopes(UPtr buf,long *size)
{
	UPtr spot, end;
	static char *evil = "From ";
	
	if (!*size) return;
	
	spot = buf-1;
	end = buf+*size;
	end[0] = 0;
	
	do
	{
		spot++;
		if (*spot && !strscmp(spot,evil))
		{
			BlockMove(spot,spot+1,end-spot+1);
			*spot = '>';
			end++;
			++*size;
		}
	}
	while (spot=strchr(spot,'\015'));
}

/************************************************************************
 * ReadNothing - throw away a body part
 ************************************************************************/
BoundaryType ReadNothing(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	MIMESHandle msh;
	POPLineType lineType;
	BoundaryType boundaryType=btEndOfMessage;
	MIMESHandle parentMSH;
	Str127 bound;
	long size;
	short err=0;
#ifdef OLDPGP
	PGPContext pgp;
#endif
	
	ReadPOPLine(stream,nil,0,nil);	
	LL_Last(mimeSList,msh);

		
#ifndef SAVE_MIME
	/*
	 * is the header superfluous?
	 */
	if (msh!=mimeSList) TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif
	
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
	
	/*
	 * now, read the message
	 */
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
	}
	if (lineType==plError) boundaryType=btError;
	if (msh==mimeSList && !UUPCIn) Progress(100,NoChange,nil,nil,nil);
/*PrintMIMEList("\pReadText",mimeSList);*/
	return(err ? btError : boundaryType);
}

/************************************************************************
 * ReadPGP - read a PGP message body
 ************************************************************************/
BoundaryType ReadPGP(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	MIMESHandle msh;
	Boolean pgping = False;
	POPLineType lineType;
	BoundaryType boundaryType=btEndOfMessage;
	MIMESHandle parentMSH;
	Str127 bound;
	short err=0;
	PGPContext pgp;
	long spot;
	FSSpec spec;
	long size;
	Boolean foundFile=False;
	Boolean isText;
	MIMEMap mm;
	FInfo info;
	
	LL_Last(mimeSList,msh);
	PCopy(bound,(*(*msh)->hdh)->contentType);
	isText = EqualStrRes(bound,MIME_TEXT);
	
	/*
	 * the data type we (may) treat specially is format="mime".  Check for it.
	 */
	if (AAFetchResData((*(*msh)->hdh)->contentAttributes,AttributeStrn+aFormat,bound) ||
			!EqualStrRes(bound,MIME))
	{
		/*
		 * not mime.  god help us all
		 */
		return(isText ?	 ReadText(stream,refN,mimeSList,buf,bSize,lr) :
										 ReadGeneric(stream,refN,mimeSList,buf,bSize,lr));
	}
	else
	{
		/*
		 * is MIME
		 */
		GetRString(bound,MIME_ENC_PGP);
		PCopy((*(*msh)->hdh)->contentSubType,bound);
		
		/*
		 * application/pgp means encrypted
		 */
		if (!isText) return(ReadGeneric(stream,refN,mimeSList,buf,bSize,lr));
	}

	/*
	 * is MIME
	 */
	GetRString(bound,MIME_CLEAR_PGP);
	PCopy((*(*msh)->hdh)->contentSubType,bound);
	 
	/*
	 * is the header superfluous?
	 */
	/*if (msh!=mimeSList && (*(*msh)->hdh)->grokked)
		TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);*/
		
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
		
	/*
	 * we're going to be un-kosher here, so make sure we don't screw anyone else up
	 */
	WriteAttachNote(refN);
	GetFPos(refN,&spot);
	
	/*
	 * now, read the body part into its own file
	 */
	BeginPGP(&pgp);
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
		
		/*
		 * give the converter a crack at the line
		 */
		if (ConvertPGP(refN,buf,&size,lineType,0,&pgp) && !foundFile)
		{
			/*
			 * note file
			 */
			spec = pgp.spec;
			foundFile = True;
		}
	}
	
	/*
	 * toss what we've written to the mailbox; it's irrelevant
	 */
	TruncOpenFile(refN,spot);
	
	/*
	 * If all went well, we have a file
	 */
	if (foundFile && !FSpGetFInfo(&spec,&info))
	{
		AddUniqueExt(&spec,PGP_PROTOCOL);
		
		/*
		 * apply proper creator/type
		 */
		FindMIMEMap(msh,&mm);
		info.fdType = mm.type;
		info.fdCreator = mm.creator;
		FSpSetFInfo(&spec,&info);
		
		/*
		 * now, reread the message file
		 */
		err = ReReadPGPClearText(stream,refN,buf,bSize,&spec);
		
		/*
		 * ok, message file reread.  Now, note the signature file
		 */
		if (!err)
		{
			err = RecordAttachment(&spec,nil);
			WriteAttachNote(refN);
		}
		else
		{
			BadBinHex = True;
			err = noErr;
		}
	}
	else
	{
		WarnUser(PGP_MISSING,0);
		BadBinHex = True;
	}
	
	
	/*
	 * report error (if any)
	 */
  if (err) FileSystemError(WRITE_MBOX,"",err);
	if (msh==mimeSList && !UUPCIn) Progress(100,NoChange,nil,nil,nil);
/*PrintMIMEList("\pReadText",mimeSList);*/
	return(err ? btError : boundaryType);
}

#ifdef DEBUG
void PrintMIMEList(PStr where,MIMESHandle msh)
{
	Str255 s;

	DebugStr(ComposeString(s,"\p%p;g",where));
	while (msh)
	{
		DebugStr(ComposeString(s,"\p%x %x %x %p;g",msh,(*msh)->decoder,(*msh)->readBody,(*msh)->boundary));
		msh = (*msh)->next;
	}
}
#endif
/************************************************************************
 * WriteBoundary - write out a boundary
 ************************************************************************/
OSErr WriteBoundary(short refN,BoundaryType boundaryType,MIMESHandle msh)
{
#ifdef SAVE_MIME
	Str127 bound;
	short err;
	
	EnsureNewline(refN);
	PCopy(bound,LDRef(msh)->boundary);
	UL(msh);
	if (boundaryType==btOuterBoundary) PCat(bound,"\p--");
	PCatC(bound,'\015');
	if (err=FSWriteP(refN,bound))
		FileSystemError(WRITE_MBOX,"",err);
	return(err);
#else
	Str127 bound;
	short err=noErr;

	if ((*msh)->isDigest && PrefIsSet(PREF_OLD_DIGEST))
	{
		EnsureNewline(refN);
		PCopy(bound,LDRef(msh)->boundary);
		UL(msh);
		if (boundaryType==btOuterBoundary) PCat(bound,"\p--");
		PCatC(bound,'\015');
		if (err=AWriteP(refN,bound))
			FileSystemError(WRITE_MBOX,"",err);
	}
	return(err);
#endif
}

/************************************************************************
 * ReadSingle
 ************************************************************************/
BoundaryType ReadSingle(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	DecoderPB pb;
	MIMESHandle msh;
	POPLineType lineType;
	BoundaryType boundaryType;
	MIMESHandle parentMSH;
	Str127 bound;
	long size;
	Boolean decode;
	short i;
	
	ReadPOPLine(stream,nil,0,nil);	
	LL_Last(mimeSList,msh);

	/*
	 * uudecode?
	 */
	PSCopy(bound,(*(*msh)->hdh)->contentEnco);
	if (EqualStrRes(bound,X_UUENCODE)||FindSTRNIndex(WhatWillLotusCallItNextStrn,bound)) return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
	
	/*
	 * initialize the decoder
	 */
	decode = nil!=(*msh)->decoder;
	if (decode && (*(*msh)->decoder)(kDecodeInit,&pb))
		return(btError);
	else pb.output = pb.input = buf;
	pb.text = False;

	/*
	 * prime the singler
	 */
	i = AAFindKey((*(*msh)->hdh)->contentAttributes,GetRString(bound,NAME));
	if (i>0) AAFetchIndData((*(*msh)->hdh)->contentAttributes,i,bound);
	else *bound = 0;
	BeginAbomination(bound,(*msh)->hdh);
	
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
	
	/*
	 * now, read the message
	 */
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
		
		/*
		 * decode
		 */
		if (decode)
		{
			pb.inlen = size;
			BadEncoding += (*(*msh)->decoder)(kDecodeData,&pb);
			size = pb.outlen;
		}
		
		/*
		 * give the line to the AppleSingle converter
		 */
		ConvertSingle(refN,buf,size);
		
		/*
		 * do NOT write the line
		 */
	}
	if (lineType==plError) boundaryType=btError;

	/*
	 * close decoder
	 */
	if (decode)
	{
		BadEncoding += (*(*msh)->decoder)(kDecodeDone,&pb);
		if (pb.outlen) ConvertSingle(refN,buf,pb.outlen);
		(*(*msh)->decoder)(kDecodeDispose,&pb);
	}

	/*
	 * close converters
	 */
	SaveAbomination(nil,0);
	
#ifndef SAVE_MIME
	/*
	 * is the header superfluous?
	 */
	if (!BadBinHex && msh!=mimeSList && (*(*msh)->hdh)->grokked)
		TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif
	
	/*
	 * write attachment note, if any
	 */
	WriteAttachNote(refN);
	
	if (!UUPCIn && msh==mimeSList) Progress(100,NoChange,nil,nil,nil);
	return(boundaryType);
}

/************************************************************************
 * GenMIMEMap - read and parse the MIME Map info onto a useable form
 ************************************************************************/
void GenMIMEMap(MIMEMapHandle *mmhp, OSType resType)
{
	Handle resH;
	MIMEMap mm;
	short ind;
	UPtr spot,end;
	
	if (!*mmhp || !**mmhp)
	{
		/*
		 * make sure we have an empty handle
		 */
		if (*mmhp) {ZapHandle(*mmhp);}
		*mmhp = NuHandle(0);
		if (*mmhp==nil) return;
		
		/*
		 * look through all the resources
		 */
		for (ind=1;resH=GetIndResource_(resType,ind);ind++)
		{
			spot = LDRef(resH);
			end = spot + GetHandleSize_(resH);
			while (spot<end)
			{
				/*
				 * copy over the data
				 */
				PCopy(mm.mimetype,spot); spot += *spot+1;
				PCopy(mm.subtype,spot); spot += *spot+1;
				PCopy(mm.suffix,spot); spot += *spot+1;
				BMD(spot,&mm.creator,4); spot += 4;
				BMD(spot,&mm.type,4); spot += 4;
				BMD(spot,&mm.flags,4); spot += 4;
				BMD(spot,&mm.specialId,4); spot+=4;
				spot += 12;	/* 16 unused bytes at the end */
				
				/*
				 * and stick it on the end
				 */
				PtrPlusHand_(&mm,*mmhp,sizeof(mm));
			}
			UL(resH);
		}
	}
}

/************************************************************************
 * FindMIMEMap - find the right mapping for a mime message
 ************************************************************************/
void FindMIMEMap(MIMESHandle msh,MIMEMapPtr mmp)
{
	HeaderDHandle hdh = (*msh)->hdh;
	Str255 name;
	Boolean mapped;
		
	LDRef(hdh);
	if (AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aFilename,name) &&
			AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aName,name))
		*name = 0;
	
	mapped = FindMIMEMapPtr((*hdh)->contentType,(*hdh)->contentSubType,name,mmp);
	
	/* x-mac-type and x-mac-creator take precedence */
	if (!(mmp->flags&mmIgnoreXType) && !AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aMacType,name))
	{
		/* found a valid type */
		Hex2Bytes((void*)(name+1),sizeof(OSType)*2,(void*)&mmp->type);
		if (!AAFetchResData((*hdh)->contentAttributes,AttributeStrn+aMacCreator,name))
			Hex2Bytes((void*)(name+1),sizeof(OSType)*2,(void*)&mmp->creator);
		if (!mapped && mmp->type=='TEXT') mmp->flags = mmIsText;
	}

	UL(hdh);
}

/************************************************************************
 * FindMIMEMapPtr - find the right mapping for particular type, subtype, filename
 ************************************************************************/
Boolean FindMIMEMapPtr(PStr type, PStr subType,PStr name,MIMEMapPtr mmp)
{
	MIMEMapPtr end;
	Str31 suffix;
	UPtr dot;
	MIMEMapPtr maybe, best=nil;
	Boolean result;
		
	GenMIMEMap((MIMEMapHandle*)&MMIn,INCOME_MIME_MAP);

	Zero(*mmp);

	if (MMIn && *MMIn)
	{
		/*
		 * where does the array end?
		 */
		end = LDRef(MMIn)+HandleCount(MMIn);
		
		/*
		 * fetch the suffix, if any
		 */
		name[*name+1] = 0;
		dot = strrchr(name+1,'.');
		if (dot)
			MakePStr(suffix,dot,*name-(dot-name-1));
		else
			*suffix = 0;
		
		/*
		 * search for a match
		 */
		for (maybe=(MIMEMapPtr)*MMIn;maybe<end;maybe++)
		{
			/*
			 * a zero-length string is a match, and so are identical strings
			 */
			if (*maybe->mimetype || *maybe->subtype)
			{
				if (!StringSame(type,maybe->mimetype)) continue;
				if (*maybe->subtype)
				{
					if (!StringSame(subType,maybe->subtype)) continue;
				}
			}
			
			// if the map specifies a suffix, it must match, UNLESS
			// there is no suffix on the file and we want to apply one
			if (!((maybe->flags&mmApplySuffix)&&!*suffix) && *maybe->suffix && !StringSame(suffix,maybe->suffix)) continue;
			
			if (!best) best = maybe;	// we don't have anything
			else if (*best->mimetype && !*maybe->mimetype) continue;	// prefer explicit match to wildcard
			else if (*best->subtype && !*maybe->subtype) continue;		// prefer explicit match to wildcard
			else if (*suffix && *best->suffix && !*maybe->suffix) continue;	// file has no suffix, best match has no suffix, new match has a suffix
			else if (!*suffix && !*best->suffix && *maybe->suffix && !(maybe->flags&mmApplySuffix)) continue;	// file has suffix, best match has suffix, new match has no suffix
			else best = maybe;
		}
		
	}
	
	/*
	 * copy in the new values
	 */
	if (result = (best!=nil)) *mmp = *best;
	else if (HaveTheDiseaseCalledOSX())
		;
	else
	{
		GetRString(suffix,DEFAULT_CREATOR);
		BMD(suffix+1,&mmp->creator,4);
		GetRString(suffix,DEFAULT_TYPE);
		BMD(suffix+1,&mmp->type,4);
	}
	if (MMIn) {UL(MMIn);HPurge((Handle)MMIn);}
	
	/*
	 * if the creator is unspecified and the type is 'TEXT', fill
	 * in the user's 'TEXT' preference
	 */
	if (mmp->creator=='    ' && mmp->type=='TEXT')
	{
		Str15 scratch;
		GetPref(scratch,PREF_CREATOR);
		if (*scratch!=4) GetRString(scratch,TEXT_CREATOR);
		BMD(scratch+1,&mmp->creator,4);
	}
	return(result);
}

/************************************************************************
 * ReadGeneric
 ************************************************************************/
BoundaryType ReadGeneric(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	MIMESHandle msh, parentMSH;
	HeaderDHandle hdh;
	MIMEMap mm;
	FSSpec spec;
	short attachRefN=0;
	short err=0;
	BoundaryType bt;
	Boolean decode;
	short writeRefN;
	POPLineType lineType;
	long size;
	Str127 bound;
	DecoderPB pb;
	BoundaryType boundaryType;
	Handle xlate;
#ifdef	IMAP
	Boolean imapStub = false;
	Str255 scratch;
#endif
	
	spec.vRefNum = spec.parID = 0;

	/*
	 * grab descriptors for our message
	 */
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;
	xlate = GetResource_('taBL',(*(*msh)->hdh)->xlateResID);
	
	if (xlate) HNoPurge_(xlate);
	/*
	 * uudecode?
	 */
	PCopy(bound,(*hdh)->contentEnco);
	if (EqualStrRes(bound,X_UUENCODE)||FindSTRNIndex(WhatWillLotusCallItNextStrn,bound)) return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
	
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;
	
	/*
	 * generate mapping info & figure out what map to use
	 */
	FindMIMEMap(msh,&mm);
	
	// see if this is really an inline text part
	if ((mm.flags&mmIsText) && !(mm.flags&mmAlwaysDetach) && !(mm.flags&mmDiscard) && !(*hdh)->isAttach)
		return(ReadText(stream,refN,mimeSList,buf,bSize,lr));
	
	/*
	 * extract filename
	 */
	ExtractHDHFilename(msh,hdh,(mm.flags&mmApplySuffix)?mm.suffix:0,spec.name);
		
	/*
	 * does the user wish to convert it?
	 */
#ifdef	IMAP
	imapStub = StringSame(GetRString(scratch, IMAP_STUB_ENCODING), (*hdh)->contentEnco);	
	if (SingleSpec || AutoWantTheFileLo(&spec,False,(*hdh)->relatedPart,imapStub)/*|| WantTheFile(&spec)*/)
#else
	if (SingleSpec || AutoWantTheFile(&spec,False,(*hdh)->relatedPart)/*|| WantTheFile(&spec)*/)
#endif
	{
		ASSERT ( SingleSpec || ( spec.vRefNum != 0 && spec.parID != 0 ));
		if (SingleSpec) {spec = **(FSSpecHandle)SingleSpec;}
		else if (err=FSpCreate(&spec,mm.creator,mm.type,smSystemScript))
		{
			if (err == dupFNErr) err = noErr;
			else
			{
				FileSystemError(BINHEX_CREATE,spec.name,err);
				BadBinHex = True;
				decode = *spec.name = 0;
			}
		}
		if (err = FSpOpenDF(&spec,fsRdWrPerm,&attachRefN))
		{
			FileSystemError(BINHEX_OPEN,spec.name,err);
			BadBinHex = True;
			decode = *spec.name = 0;
		}
	}
	else decode = *spec.name = 0;
	
	/*
	 * if we are writing to a file, we record that fact and
	 * toss the header if it was boring
	 * we also prime the decoder
	 */
	if (attachRefN)
	{
		if (mm.type=='euEn') AddUniqueExt(&spec,PGP_PROTOCOL);	/* hack for pgp */
		err = RecordAttachment(&spec,hdh);
#ifndef SAVE_MIME
		if ((*hdh)->grokked && mm.type!='????') TruncOpenFile(refN,(*hdh)->diskStart);
#endif
		WriteAttachNote(refN);
		
		decode = nil!=(*msh)->decoder;
		if (decode && (*(*msh)->decoder)(kDecodeInit,&pb))
		{
			BadBinHex = True;
			decode = *spec.name = 0;
		}
		else pb.output = pb.input = buf;
		pb.text = (mm.flags & mmIsText)!=0;
	}
	
	/*
	 * now, read and write the bytes
	 */
	writeRefN = attachRefN ? attachRefN : refN;
	
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;

		/*
		 * decoder ?
		 */
		if (decode)
		{
			pb.inlen = size;
			BadEncoding += (*(*msh)->decoder)(kDecodeData,&pb);
			size = pb.outlen;
		}
		
		/*
		 * convert
		 */
		if (xlate)
		{
			TransLit(buf,size,LDRef(xlate));
			UL(xlate);
		}
		
		if (err = NCWrite(writeRefN,&size,buf))
		{
			FileSystemError(BINHEX_WRITE,spec.name,err);
			BadBinHex = True;
			bt = btError;
			goto done;
		}
	}
	if (lineType==plError) boundaryType = btError;

done:	
	if (decode)
	{
		BadEncoding += (*(*msh)->decoder)(kDecodeDone,&pb);
		size = pb.outlen;
		if (size && (err = NCWrite(writeRefN,&size,buf)))
		{
			FileSystemError(BINHEX_WRITE,spec.name,err);
			BadBinHex = True;
			bt = btError;
		}
		(*(*msh)->decoder)(kDecodeDispose,&pb);
	}

	if (attachRefN)
	{
		MyFSClose(attachRefN);
		if (err)
			{FSpDelete(&spec);ASSERT(0);}
		else
		{
			FlushVol(nil,spec.vRefNum);
		}
		PopProgress(False);
	}
	if (!UUPCIn && msh==mimeSList) Progress(100,NoChange,nil,nil,nil);
  if (xlate) HPurge(xlate);

	return(err ? btError : boundaryType);
}


/************************************************************************
 * ReadTLNow - body reader for when a translator wants a file now
 ************************************************************************/
BoundaryType ReadTLNow(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	FSSpec spec;
	BoundaryType bt;
	MIMESHandle msh;

	LL_Last(mimeSList,msh);

#ifndef SAVE_MIME
	if (msh!=mimeSList) TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif

	/*
	 * read the object into a file
	 */
	bt = ReadTL(stream,refN,mimeSList,buf,bSize,lr,&spec,CREATOR,MIME_FTYPE);
	
	/*
	 * now, interpret
	 */
	if (bt!=btError)
	{
		Boolean	dontSave;
		emsHeaderData addrList;
		OSErr	err;
			
		ETLBuildAddrList(nil,nil,(*mimeSList)->hdh,&addrList,EMSF_ON_ARRIVAL);
		err = ETLInterpretFile(EMSF_ON_ARRIVAL,&spec,refN,nil,&addrList,&dontSave);
		ETLDisposeAddrList(&addrList);

		if (!err)
		{
			FSpDelete(&spec);
			return(bt);
		}
	}
	
	/*
	 * record filename
	 */
	if (bt!=btError)
	{
		if (RecordAttachment(&spec,(*mimeSList)->hdh)) bt=btError;
		WriteAttachNote(refN);
	}

	return(bt);
}

/************************************************************************
 * ReadTLNotNow - body reader for when a translator says "not now"
 ************************************************************************/
BoundaryType ReadTLNotNow(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	FSSpec spec;
	BoundaryType bt;
	MIMESHandle msh;

	LL_Last(mimeSList,msh);

#ifndef SAVE_MIME
	if (msh!=mimeSList) TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif

	/*
	 * read the object into a file
	 */
	bt = ReadTL(stream,refN,mimeSList,buf,bSize,lr,&spec,CREATOR,MIME_FTYPE);
	
	/*
	 * record filename
	 */
	if (bt!=btError)
	{
		if (RecordAttachment(&spec,(*msh)->hdh)) bt = btError;
		WriteAttachNote(refN);
	}
	return(bt);
}

/************************************************************************
 * ReadExternalMulti - body reader for an external multipart
 ************************************************************************/
BoundaryType ReadExternalMulti(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr)
{
	FSSpec spec;
	BoundaryType bt;
	MIMESHandle msh;
	MIMEMap mm;

	LL_Last(mimeSList,msh);

#ifndef SAVE_MIME
	if (msh!=mimeSList) TruncOpenFile(refN,(*(*msh)->hdh)->diskStart);
#endif
	
	/*
	 * generate mapping info & figure out what creator & type to use
	 */
	FindMIMEMap(msh,&mm);
	
	/*
	 * extract filename
	 */
	ExtractHDHFilename(msh,(*msh)->hdh,(mm.flags&mmApplySuffix)?mm.suffix:nil,spec.name);

	/*
	 * read the object into a file
	 */
	bt = ReadTL(stream,refN,mimeSList,buf,bSize,lr,&spec,mm.creator,mm.type);
	
	/*
	 * record filename
	 */
	if (bt!=btError)
	{
		if (RecordAttachment(&spec,(*msh)->hdh)) bt=btError;
		WriteAttachNote(refN);
	}
	return(bt);
}

/************************************************************************
 * ReadTL - body reader to put some stuff in a file
 ************************************************************************/
BoundaryType ReadTL(TransStream stream,short refN,MIMESHandle mimeSList,char *buf,long bSize,LineReader *lr,FSSpecPtr spec,OSType creator,OSType type)
{
	MIMESHandle msh, parentMSH;
	HeaderDHandle hdh;
	short attachRefN=0;
	short err=0;
	BoundaryType bt;
	POPLineType lineType;
	long size;
	Str127 bound;
	BoundaryType boundaryType;
	
	/*
	 * grab descriptors for our message
	 */
	LL_Last(mimeSList,msh);
	hdh = (*msh)->hdh;
	
	/*
	 * find our birth mother
	 */
	LL_Parent(mimeSList,msh,parentMSH);
	if (parentMSH)
		PCopy(bound,(*parentMSH)->boundary);
	else
		*bound = 0;

	/*
	 * extract filename
	 */
	ExtractHDHFilename(msh,hdh,nil,spec->name);
		
	/*
	 * does the user wish to convert it?
	 */
	if (AutoWantTheFile(spec,False,(*hdh)->relatedPart))
	{
		FSpCreateResFile(spec,creator,type,smSystemScript);
		if (err=ResError())
		{
			if (err == dupFNErr) err = noErr;
			else
			{
				FileSystemError(BINHEX_CREATE,spec->name,err);
				BadBinHex = True;
				*spec->name = 0;
			}
		}
		if (err = FSpOpenDF(spec,fsRdWrPerm,&attachRefN))
		{
			FileSystemError(BINHEX_OPEN,spec->name,err);
			BadBinHex = True;
			*spec->name = 0;
		}
	}
	else *spec->name = 0;
	
	/*
	 * write the headers
	 */
	size = (*hdh)->fullHeaders.offset;
	err = CanonNLWrite(attachRefN,&size,LDRef((*hdh)->fullHeaders.data));
	UL((*hdh)->fullHeaders.data);
#ifdef ETL
	if (!err) err = RecordTLMIME(spec,(*hdh)->tlMIME);
	if (!err && (*msh)->translators) err = RecordTL(spec,(*msh)->translators);
#endif
	if (err)
	{
		FileSystemError(BINHEX_OPEN,spec->name,err);
		BadBinHex = True;
		bt = btError;
		goto done;
	}

	/*
	 * now, read and write the bytes
	 */
	for (lineType=(*lr)(stream,buf,bSize,&size);
			 lineType!=plError && lineType!=plEndOfMessage;
			 lineType=(*lr)(stream,buf,bSize,&size))
	{
		if (*bound && (boundaryType=IsBoundaryLine(lineType,buf,size,bound)))
			break;
		
		if (err = CanonNLWrite(attachRefN,&size,buf))
		{
			FileSystemError(BINHEX_WRITE,spec->name,err);
			BadBinHex = True;
			bt = btError;
			goto done;
		}
	}
	if (lineType==plError) boundaryType = btError;

done:	
	if (attachRefN)
	{
		MyFSClose(attachRefN);
		PopProgress(False);
		if (err) {FSpDelete(spec);ASSERT(0);}
	}
	if (!UUPCIn && msh==mimeSList) Progress(100,NoChange,nil,nil,nil);

	return(err ? btError : boundaryType);
}

/**********************************************************************
 * RecordTLMIME - record the choice of a translator
 **********************************************************************/
OSErr RecordTLMIME(FSSpecPtr spec,emsMIMEHandle tlMIME)
{
	FlatTLMIMEHandle flat;
	short refN;
	OSErr err;
	short oldResF = CurResFile();

	if (err=FlattenTLMIME(tlMIME,&flat)) return(err);
	refN = FSpOpenResFile(spec,fsRdWrPerm);
	if (refN==-1) err = ResError();
	else
	{
		UseResFile(refN);
		AddResource((Handle)flat,MIME_FTYPE,1001,"");
		if (err=ResError()) ZapHandle(flat);
		else
		{
			err = MyUpdateResFile(refN);
		}
		CloseResFile(refN);
	}
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * RecordTL - record the choice of a translator
 **********************************************************************/
OSErr RecordTL(FSSpecPtr spec,TLMHandle tl)
{
	OSErr err=noErr;
	short i;

	for(i = HandleCount(tl);i--;)
		if ((*tl)[i].result==EMSR_NOT_NOW || (*tl)[i].result==EMSR_NOW) break;
	
	if (i<0) return fnfErr;
	
	err = RecordTLID(spec,ETLID(tl,i));
	
	return(err);
}

/**********************************************************************
 * ExtractHDHFilename - get a suggested filename from an HDH
 **********************************************************************/
void ExtractHDHFilename(MIMESHandle msh,HeaderDHandle hdh,PStr suffix,PStr name)
{
	Str31 buf;
	Str255 fName;
	
	if (AAFetchData((*hdh)->contentAttributes,GetRString(buf,AttributeStrn+aFilename),fName) &&
	    AAFetchData((*hdh)->contentAttributes,GetRString(buf,AttributeStrn+aName),fName))
		PCopy(fName,(*(*msh)->hdh)->subj);
	Other2MacName(fName,fName);
	if (!*fName) GetRString(fName,UNTITLED);
	*name = 0;
	if (suffix && !PIndex(fName,'.')) PCopy(name,suffix);
	PInsert(name,32,fName,name+1);
}

/************************************************************************
 * FigureMIMEFromApple - figure out MIME type/subtype from Apple creator/type
 ************************************************************************/
void FigureMIMEFromApple(OSType creator, OSType type,PStr name,PStr mimeType,PStr mimeSub,PStr mimeSuffix, long *flags, OSType *specialId)
{
	MIMEMapPtr end, maybe, best=nil;
	Str31 suffix;
	UPtr dot;
	
	GenMIMEMap((MIMEMapHandle*)&MMOut,OUTGO_MIME_MAP);

	if (MMOut && *MMOut)
	{
		/*
		 * fetch the suffix, if any
		 */
		name[*name+1] = 0;
		dot = strrchr(name+1,'.');
		if (dot)
			MakePStr(suffix,dot,*name-(dot-name-1));
		else
			*suffix = 0;
		
		/*
		 * where does the array end?
		 */
		end = LDRef(MMOut)+HandleCount(MMOut);
				
		/*
		 * search for a match
		 */
		for (maybe=(MIMEMapPtr)*MMOut;maybe<end;maybe++)
		{
			/*
			 * if the creators don't match, we go on, unless map creator is
			 * a wildcard
			 */
			if (maybe->creator!='    ' && maybe->creator!=creator) continue;
			
			/*
			 * if the types don't match, we go on unless map type is wildcard
			 */
			if (maybe->type!='    ' && maybe->type!=type) continue;
			
			/*
			 * ok, now we know we have a match for type and creator
			 */
			 
			/*
			 * if the is the first match, we'll take it
			 */			
			if (!best) best = maybe;
			
			/*
			 * if the old type was not a wildcard and the new one is, we'll
			 * keep the old
			 */
			else if (best->type!='    ' && maybe->type=='    ') continue;
			
			/*
			 * if the old creator was not a wildcard and the new one is, we'll
			 * keep the old
			 */
			else if (best->creator!='    ' && maybe->creator=='    ') continue;
			
			/*
			 * if the old creator or type were wildcards and the new creator or type
			 * are not, we'll go for the new
			 */
			else if (best->creator=='    ' && maybe->creator!='    ' ||
							 best->type=='    ' && maybe->type!='    ')
			{
				best = maybe;
				continue;
			}
			
			/*
			 * if the old one matches the suffix exactly, we'll keep it
			 */
			else if (*suffix && *best->suffix && StringSame(suffix,best->suffix))
				continue;
			
			/*
			 * if the new one fails to match the suffix, we'll keep the old
			 */
			else if (*suffix && *maybe->suffix && !StringSame(suffix,maybe->suffix))
				continue;
			
			/*
			 * new one is at least as good as the old
			 */
			else best = maybe;
		}
	}
	
	/*
	 * copy in the new values
	 */
	if (best)
	{
		PCopy(mimeType,best->mimetype);
		PCopy(mimeSub,best->subtype);
		PCopy(mimeSuffix,best->suffix);
		*flags = best->flags;
		*specialId = best->specialId;
	}
	else
	{
		GetRString(mimeType,MIME_APPLICATION);
		GetRString(mimeSub,MIME_OCTET_STREAM);
		*mimeSuffix = 0;
		*flags = type=='TEXT' ? mmIsText|mmIsBasic : 0;
	}

	if (MMOut) {UL(MMOut);HPurge((Handle)MMOut);}
}

/**********************************************************************
 * CanonNLWrite - write some data, canonicalizing newlines as we go
 **********************************************************************/
OSErr CanonNLWrite(short refN,long *size,UPtr buf)
{
	UPtr start, stop, end;
	long locSize;
	OSErr err = noErr;
	
	end = buf + *size;
	
	for (start=buf;start<end;start=stop+1)
	{
		for (stop=start;stop<end;stop++) if (*stop=='\015') break;
		if (stop>start)
		{
			locSize = stop-start;
			err = AWrite(refN,&locSize,start);
		}
		if (!err && stop<end)
		{
			locSize = 2;
			err = AWrite(refN,&locSize,"\015\012");
		}
	}
	return(err);
}

		
#endif