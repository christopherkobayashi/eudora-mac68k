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

#define FILE_NUM 63
/* Copyright (c) 1994 by QUALCOMM Incorporated */
/************************************************************************
 * reading pgp
 ************************************************************************/

#pragma segment PGP
#include "pgpin.h"
#ifdef TWO

void AbortPGP(PGPCPtr pgp);
void EndPGP(PGPCPtr pgp);
void ClosePGP(PGPCPtr pgp,Boolean abort);
PGPEnum PGPTypeOf(PGPCPtr pgpc,UPtr buf,long size);
OSType PGPCreator(void);
OSErr PGPRecvInit(FSSpecPtr spec,PGPRecvContextPtr recv);
OSErr PGPRecvClose(PGPRecvContextPtr recv);

/**********************************************************************
 * PGPCreator - return the creator for PGP files
 **********************************************************************/
OSType PGPCreator(void)
{
	OSType type;
	Str15 text;
	
	GetRString(text,MAC_PGP_CREATOR);
	BMD(text+1,&type,sizeof(OSType));
	return(type);
}

/**********************************************************************
 * ConvertPGP - write pgp stuff to a file
 **********************************************************************/
Boolean ConvertPGP(short refN,UPtr buf,long *size,POPLineType lineType,long estSize,PGPCPtr pgpc)
{
	Boolean isPGPLine;
	OSErr err;
	
	isPGPLine = (*size > *pgpc->intro) && !(strncmp(pgpc->intro+1,buf,*pgpc->intro));
	
	if (pgpc->type == pgpNone)
	{
		if (!isPGPLine) return(False);	/* not pgp */
		
		/*
		 * we have begun to PGP
		 */
		pgpc->type = PGPTypeOf(pgpc,buf,*size);
		
		/*
		 * open the file
		 */
		GetRString(pgpc->spec.name,PGPStrn+pgpc->type);
		if (!(err=!AutoWantTheFile(&pgpc->spec,True,False)))
		{
			err = FSpCreate(&pgpc->spec,PGPCreator(),'TEXT',smSystemScript);
			if (!err) err = FSpOpenDF(&pgpc->spec,fsRdWrPerm,&pgpc->refN);
			if (err) FileSystemError(BINHEX_WRITE,&pgpc->spec.name,err);
		}
		if (err) AbortPGP(pgpc);
		else
		{
			/*
			 * now we're looking for "END PGP"
			 */
			GetRString(pgpc->intro,PGPStrn+pgpEnd);
			isPGPLine = False;	/* clear flag, because meaning has changed */
		}
	}
	
	if (pgpc->type != pgpNone)
	{
		err = AWrite(pgpc->refN,size,buf);
		if (err)
		{
			FileSystemError(BINHEX_WRITE,pgpc->spec.name,err);
			AbortPGP(pgpc);
		}
		else if (pgpc->type != pgpSigned) *size = 0;
		
		if (isPGPLine)
		{
			pgpc->type = pgpNone;
			ClosePGP(pgpc,False);
			EndPGP(pgpc);
			if (*size)
			{
				AWrite(refN,size,buf);
				*size = 0;
			}
			WriteAttachNote(refN);
		}
	}
	
	return(pgpc->type != pgpNone);
}

/**********************************************************************
 * PGPTypeOf - get the PGP type of something
 **********************************************************************/
PGPEnum PGPTypeOf(PGPCPtr pgpc,UPtr buf,long size)
{
	Str255 scratch;
	PGPEnum type;
	
	MakePStr(scratch,buf+*pgpc->intro+1,size-(*pgpc->intro+1)-6);
	type = FindSTRNIndex(PGPStrn,scratch);
	return(type ? type : pgpUnknown);
}

/**********************************************************************
 * AbortPGP - something went wrong
 **********************************************************************/
void AbortPGP(PGPCPtr pgpc)
{
	FSSpec spec = pgpc->spec;
	EndPGP(pgpc);
	BadBinHex = True;
}

/**********************************************************************
 * EndPGP - end PGP processing
 **********************************************************************/
void EndPGP(PGPCPtr pgpc)
{
	if (pgpc->type != pgpNone)
	{
		BadBinHex = True;
		ClosePGP(pgpc,True);
	}
	BeginPGP(pgpc);
}

/**********************************************************************
 * ClosePGP - close the open file
 **********************************************************************/
void ClosePGP(PGPCPtr pgpc,Boolean abort)
{
	OSErr err;
	if (pgpc->refN)
	{
		if (!(err=MyFSClose(pgpc->refN)) && !abort)
			err = RecordAttachment(&pgpc->spec,nil);
		if (err && !abort)
			FileSystemError(BINHEX_WRITE,pgpc->spec.name,err);
		pgpc->refN = 0;
	}
	if (abort) FSpDelete(&pgpc->spec);
}

/**********************************************************************
 * BeginPGP - initialize the context
 **********************************************************************/
void BeginPGP(PGPCPtr pgpc)
{
	Zero(*pgpc);
	GetRString(pgpc->intro,PGPStrn+pgpBegin);
}

/**********************************************************************
 * ReReadPGPClearText - read a cleartext message from a signed PGP file
 **********************************************************************/
OSErr ReReadPGPClearText(TransStream stream,short refN,UPtr buf,long bSize,FSSpecPtr spec)
{
	TransVector oldTrans = CurTrans;
	OSErr err;
	PGPRecvContextPtr oldRecv = PGPRContext;
	PGPRecvContext recv;
	
	PGPRContext = &recv;
	CurTrans = PGPTrans;
	if (!(err = PGPRecvInit(spec,PGPRContext)))
	{
		err = ReadHeadAndBody(stream,refN,buf,bSize,False,nil);
		PGPRecvClose(PGPRContext);
	}
	
	CurTrans = oldTrans;
	PGPRContext = oldRecv;
	return(err);
}

/**********************************************************************
 * ReadHeadAndBody - read the header and body of a message
 **********************************************************************/
OSErr ReadHeadAndBody(TransStream stream,short refN,UPtr buf,long bSize,Boolean display,HeaderDHandle *headersFound)
{
	HeaderDHandle hdh=NewHeaderDesc(nil);
	short lastHeaderTokenType;
	
	if (!hdh) Prr = MemError();
	else
	{
reRead:
		lastHeaderTokenType = ReadHeader(stream,hdh,0,refN,False);
		if (lastHeaderTokenType==EndOfHeader)
		{
			if (headersFound)
			{
				*headersFound = hdh;
				if (!display) return(noErr);
			}
			if ((*hdh)->grokked)
			{
				TruncOpenFile(refN,(*hdh)->diskStart);
				(*hdh)->diskEnd = (*hdh)->diskStart;
			}
			ReadEitherBody(stream,refN,hdh,buf,bSize,0,EMSF_ON_DISPLAY);
		}
		else
		{
			EnsureNewline(refN);
			FSWriteP(refN,Cr);
		}
		
		/*
		 * encapsulated sxxt
		 */
		if (Prr == '82')
		{
			ZapHeaderDesc(hdh);
			hdh = NewHeaderDesc(nil);
			Prr = noErr;
			goto reRead;
		}
	}
	return(Prr);
}

/**********************************************************************
 * POGPRecvInit - initialize the PGP receiver
 **********************************************************************/
OSErr PGPRecvInit(FSSpecPtr spec,PGPRecvContextPtr recv)
{
	OSErr err;
	Str255 buf;
	long len;
	
	Zero(*recv);
	
	if (err = FSpOpenLine(spec,fsRdWrPerm,&recv->lio))
		FileSystemError(BINHEX_READ,spec->name,err);
	
	GetRString(recv->intro,PGPStrn+pgpBegin);
	while (!(err=0>NLGetLine(buf,sizeof(buf),&len,&recv->lio)))
	{
		/*
		 * has it begun?
		 */
		if (len>*recv->intro && !strncmp(buf,recv->intro+1,*recv->intro)) break;
	}
	
	/*
	 * now, find the beginning of the headers
	 */
	if (!err)
	{
		while (!(err=0>NLGetLine(buf,sizeof(buf),&len,&recv->lio)))
			if (len>1)
			{
			  SeekLine(TellLine(&recv->lio),&recv->lio);
			  break;
			}
	}
	
	if (err) PGPRecvClose(recv);
	
	return(err);
}

/**********************************************************************
 * PGPRecvClose - close a file
 **********************************************************************/
OSErr PGPRecvClose(PGPRecvContextPtr recv)
{
	CloseLine(&recv->lio);
	Zero(*recv);
	return(noErr);
}

/**********************************************************************
 * PGPRecvLine - receive a line with PGP
 * stops at first --BEGIN PGP line
 **********************************************************************/
OSErr PGPRecvLine(TransStream stream,UPtr line,long *size)
{
	OSErr err = 0>NLGetLine(line,*size,size,&PGPRContext->lio);
	
	if (!err)
	{
		if (*size>*PGPRContext->intro && !strncmp(line,PGPRContext->intro+1,*PGPRContext->intro))
		{
			*size = 2;
			line[0]='.'; line[1]='\015'; line[2] = 0;
		}
	}
	return(err);
}
	
/**********************************************************************
 * PGPVerifyFile - see if the signature matches on a file
 **********************************************************************/
OSErr PGPVerifyFile(FSSpecPtr spec)
{
	AppleEvent reply;
	OSErr err;
	ProcessSerialNumber psn;
	FSSpec tempSpec;
	Str255 who;
	AEDesc whoDesc;
	
	NullADList(&reply,&whoDesc,nil);
	
	if (!(err = StartPGP(&psn)))
#ifdef NEVER
	if (!(err = NewTempSpec(Root.vRef,nil,&tempSpec)))
#endif
	{
		err = SimpleAESend(&psn,kPGPClass,kPGPDecrypt,&reply,kEAEImmediate,
						keyDirectObject,typeFSS,spec,sizeof(FSSpec),
#ifdef NEVER
						keyPGPToFile,typeFSS,&tempSpec,sizeof(FSSpec),
#endif
						nil,nil);
		if (!err && !(err=GetAEError(&reply)) && !(err=PGPFetchResult(&reply,&tempSpec)))
		{
			FSpDelete(&tempSpec);
			if (AEGetParamDesc(&reply,keyDirectObject,typeWildCard,&whoDesc) ||
					!*GetAEPStr(who,&whoDesc))
				GetRString(who,MR_PGP_BROKEN);
			Aprintf(OK_ALRT,Note,PGP_SIG_MATCH,who);
		}
	}
	
	if (err) WarnUser(PGP_CANT_VERIFY,err);
	
	DisposeADList(&reply,&whoDesc,nil);
	return(err);
}

/**********************************************************************
 * PGPOpenEncrypted - open an encrypted message
 **********************************************************************/
OSErr PGPOpenEncrypted(FSSpecPtr spec)
{
	AppleEvent reply;
	OSErr err;
	ProcessSerialNumber psn;
	FSSpec tempSpec;
	short howMany = 0;
	TOCHandle tocH;
	
	NullADList(&reply,nil);
	
	if (!(err = StartPGP(&psn)))
#ifdef NEVER
	if (!(err = NewTempExtSpec(Root.vRef,nil,PGP_PROTOCOL,&tempSpec)))
#endif
	{
		err = SimpleAESend(&psn,kPGPClass,kPGPDecrypt,&reply,kEAEImmediate,
						keyDirectObject,typeFSS,spec,sizeof(FSSpec),
#ifdef NEVER
						keyPGPToFile,typeFSS,&tempSpec,sizeof(FSSpec),
#endif
						nil,nil);
		if (!err && !(err=GetAEError(&reply)) && !(err=PGPFetchResult(&reply,&tempSpec)))
		{
			if (!( err = GetUUPCMailSpec(&tempSpec,False,&howMany)))
			{
				if (howMany)
				{
					if (tocH = GetInTOC())
						while (howMany--)
						{
							(*tocH)->sums[(*tocH)->count-1-howMany].opts |= OPT_WIPE;
							GetAMessage(tocH,(*tocH)->count-1-howMany,nil,nil,True);
						}
				}
				else
					WarnUser(PGP_NOT_MESSAGE,0);
			}
		}
		FSpDelete(&tempSpec);
	}
	
	if (err) WarnUser(PGP_CANT_DECRYPT,err);
	
	DisposeADList(&reply,nil);
	return(err);
}
#endif
