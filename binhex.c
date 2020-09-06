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

#include "binhex.h"
#define FILE_NUM 4
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions to convert files to binhex and back again
 * much of this code is patterned after xbin by Dave Johnson, Brown University
 * Some is lifted straight from xbin.  Our thanks to Dave & Brown.
 ************************************************************************/

#pragma segment BinHex

/************************************************************************
 * Declarations for private routines
 ************************************************************************/
void comp_q_crc_out(unsigned short c);
short EncodeDataChar(Byte c, UPtr toSpot);
int BinHexFork(TransStream stream,short refN, UHandle dataBuffer,short dataSize,
																				Handle codedBuffer,UPtr name);
/************************************************************************
 * six to eight bit conversion table
 ************************************************************************/
Byte BinHexTable[64] = {
				0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
				0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x30, 0x31, 0x32,
				0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x40, 0x41,
				0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
				0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x50, 0x51, 0x52,
				0x53, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5a, 0x5b,
				0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x68,
				0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x70, 0x71, 0x72 };
unsigned long CalcCrc;
Byte State86;
Byte SavedBits;
Byte LineLength;
	
#define LCODE(dc)  codedSpot += EncodeDataChar(dc,*codedBuffer+codedSpot)
#define CODE(dc) do { 																									\
	LCODE(dc);																														\
	if ((Byte)dc==(Byte)0x90) LCODE(0); 																	\
	comp_q_crc_out(dc); 																									\
	} while (0)
#define CODESHORT(ds) do {short copy=(short)ds; UPtr cp=(UPtr)&ds;			\
	CODE(cp[0]); CODE(cp[1]);} while(0)
#define CODELONG(dl) do {long copy=(long)dl; UPtr cp=(UPtr)&dl; 				\
	CODE(cp[0]); CODE(cp[1]); CODE(cp[2]); CODE(cp[3]);} while(0)
/************************************************************************
 * SendBinHex - send a file as BinHex data								
 ************************************************************************/
int SendBinHex(TransStream stream, FSSpecPtr spec, AttMapPtr amp)
{
	short refN=0;
	UHandle dataBuffer=nil;
	UHandle codedBuffer=nil;
	short dataSize;
	short codedSize;
	short codedSpot;
	int err;
	CInfoPBRec hfp;
	Str255 scratch;
	UPtr spot;
	short nameLen;
	
	/*
	 * find the file, and get info
	 */
	err=FSpGetHFileInfo(spec,&hfp);
	if (err) {FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	
	/*
	 * allocate the buffers
	 */
	codedSize = 2*GetRLong(BUFFER_SIZE);
	dataSize = codedSize/3;
	if (!(dataBuffer=NuHTempOK(dataSize)) || !(codedBuffer=NuHTempOK(codedSize)))
		{WarnUser(MEM_ERR,MemError()); goto done;}

	/*
	 * send the header
	 */
	if (err=MIMEFileHeader(stream,amp,MIME_BINHEX2,AFSpGetMod(spec))) goto done;
	if (err=SendPString(stream,NewLine)) goto done;
	GetRString(scratch,BINHEX_OUT);
	if (err=SendTrans(stream,scratch+1,*scratch,NewLine+1,*NewLine,nil)) goto done;
	if (err=SendTrans(stream,NewLine+1,*NewLine,":",1,nil)) goto done;
		
	/*
	 * send the file information
	 */
	DontTranslate = True;
	LineLength = 1;
	State86 = CalcCrc = codedSpot = 0;
	nameLen = *spec->name+1;
	for (spot=spec->name;nameLen--;spot++)
		CODE(*spot);
	CODE(0);
	CODELONG(hfp.hFileInfo.ioFlFndrInfo.fdType);
	CODELONG(hfp.hFileInfo.ioFlFndrInfo.fdCreator);
	CODESHORT(hfp.hFileInfo.ioFlFndrInfo.fdFlags);
	CODELONG(hfp.hFileInfo.ioFlLgLen);
	CODELONG(hfp.hFileInfo.ioFlRLgLen);
	{
		unsigned short tempCrc;
		comp_q_crc_out(0);
		comp_q_crc_out(0);
		tempCrc = CalcCrc&0xffff;
		CODESHORT(tempCrc);
		CalcCrc = 0;
	}
	if (err=SendTrans(stream,LDRef(codedBuffer),codedSpot,nil)) goto done;
	codedSpot = 0;
	UL(codedBuffer);
	
	/*
	 * data fork
	 */
	if (err = FSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	err = BinHexFork(stream,refN,dataBuffer,dataSize,codedBuffer,spec->name);
	if (err) goto done;

	/*
	 * resource fork
	 */
	if (refN) MyFSClose(refN);
	refN = 0;
	if (err = FSpOpenRF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	err = BinHexFork(stream,refN,dataBuffer,dataSize,codedBuffer,spec->name);
	if (err) goto done;

	/*
	 * leftovers
	 */
	if (State86) CODE(0);
	(*codedBuffer)[codedSpot++] = ':';
	for (err=0;err<*NewLine;) (*codedBuffer)[codedSpot++] = NewLine[++err]; 
	err=SendTrans(stream,LDRef(codedBuffer),codedSpot,nil);
	
	
done:
	if (refN) MyFSClose(refN);
	if (dataBuffer) ZapHandle(dataBuffer);
	if (codedBuffer) ZapHandle(codedBuffer);
	DontTranslate = False;
	return(err);	
}

/************************************************************************
 * MIMEFileHeader - send the BinHex header for a file
 ************************************************************************/
OSErr MIMEFileHeader(TransStream stream,AttMapPtr amp,short convertID,long modDate)
{
	short err;
	Str63 date;
	
	if (err = ComposeRTrans(stream,MIME_MP_FMT,
								 InterestHeadStrn+hContentType,
								 MIME_APPLICATION,
								 convertID,
								 AttributeStrn+aName,
								 ATT_MAP_NAME(amp),
								 NewLine))
		return(err);
	if (err = ComposeRTrans(stream,MIME_CD_FMT,
								 InterestHeadStrn+hContentDisposition,
								 ATTACHMENT,
								 AttributeStrn+aFilename,
								 ATT_MAP_NAME(amp),
								 NewLine))
		return(err);
	if (modDate && *R822Date(date,modDate-ZoneSecs()) && (err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aModDate,date,NewLine)))
		return(err);
	return(noErr);
}								 

/************************************************************************
 * BinHexFork - send one fork of a file as BinHex data								
 ************************************************************************/
int BinHexFork(TransStream stream,short refN, UHandle dataBuffer,short dataSize,
																				Handle codedBuffer,UPtr name)
{
	long dataEnd;
	short codedSpot=0;
	int err;
	unsigned char *spot;
	int errWas;
	
	do
	{
		dataEnd = dataSize;
		err = ARead(refN,&dataEnd,LDRef(dataBuffer));
		if (!err || err==eofErr)
		{
			errWas = err;
			for (spot = *dataBuffer; spot<*dataBuffer+dataEnd; spot++)
				CODE(*spot);
			err=SendTrans(stream,LDRef(codedBuffer),codedSpot,nil);
			UL(codedBuffer);
			codedSpot = 0;
			if (!err) err = errWas;
			if (dataEnd > 0)
				ByteProgress(nil,-dataEnd,0);
		}
		UL(dataBuffer);
		if (err && err!=eofErr && !CommandPeriod)
			FileSystemError(BINHEX_READ,name,err);
	}
	while (!err);
	
	if (err==eofErr)
	{
		{
			unsigned short tempCrc;
			comp_q_crc_out(0);
			comp_q_crc_out(0);
			tempCrc = CalcCrc&0xffff;
			CODESHORT(tempCrc);
			CalcCrc = 0;
		}
		err=SendTrans(stream,LDRef(codedBuffer),codedSpot,nil);
		UL(codedBuffer);
	}
	
	return(err==eofErr ? noErr : err);
}

/************************************************************************
 * EncodeDataChar - encode an 8-bit data char into a six-bit buffer
 * returns the number of valid encoded characters generated
 ************************************************************************/
short EncodeDataChar(Byte c, UPtr toSpot)
{
	UPtr spotWas=toSpot;
	UPtr nSpot;
#define ADDNEWLINE() do { 																			\
	LineLength = 0; 																							\
	for (nSpot=NewLine+1;nSpot<=NewLine+*NewLine;nSpot++) 				\
		*toSpot++ = *nSpot; 																				\
	} while (0)

	switch (State86++)
	{
		case 0:
			*toSpot++ = BinHexTable[(c>>2)&0x3f];
			SavedBits = (c&0x3)<<4;
			if (++LineLength == 64) ADDNEWLINE();
			break;
		case 1:
			*toSpot++ = BinHexTable[SavedBits | ((c>>4)&0xf)];
			SavedBits = (c&0xf)<<2;
			if (++LineLength == 64) ADDNEWLINE();
			break;
		case 2:
			*toSpot++ = BinHexTable[SavedBits | ((c>>6)&0x3)];
			if (++LineLength == 64) ADDNEWLINE();
			*toSpot++ = BinHexTable[c&0x3f];
			if (++LineLength == 64) ADDNEWLINE();
			State86 = 0;
			break;
	}
	return(toSpot-spotWas);
} 

/************************************************************************
 * comp_q_crc_out - lifted from xbin
 ************************************************************************/
#define BYTEMASK 0xff
#define BYTEBIT 0x100
#define WORDMASK 0xffff
#define WORDBIT 0x10000
#define CRCCONSTANT 0x1021

void comp_q_crc_out(unsigned short c)
{
				register unsigned long temp = CalcCrc;

/* Never mind why I call it WOP... */
#define WOP { \
								c <<= 1; \
								if ((temp <<= 1) & WORDBIT) \
												temp = (temp & WORDMASK) ^ CRCCONSTANT; \
								temp ^= (c >> 8); \
								c &= BYTEMASK; \
				}
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				WOP;
				CalcCrc = temp;
}
