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

#include <conf.h>
#include <mydefs.h>

#include "lineio.h"
#define FILE_NUM 20
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment FileUtil
																
#define LIKE_BUFFER 8192

#define Buffer lip->buffer
#define BufferSize lip->bufferSize
#define BFilled lip->bFilled
#define RefN lip->refN
#define BSpot lip->bSpot
#define LastSpot lip->lastSpot
#define FSpot lip->fSpot
#define Eof lip->eof

short OpenLine(short vRef,long dirId,UPtr name,short perm,LineIOP lip)
{
	int err;
	short refN;
	
	Zero(*lip);
		
	/*
	 * open the file
	 */
	if (err=AFSHOpen(name,vRef,dirId,&refN,perm)) goto failure;
	RefN = refN;
	
	/*
	 * allocate a buffer
	 */
	GetEOF(RefN,&BufferSize);
	BufferSize += 2;
	if (BufferSize > LIKE_BUFFER) BufferSize = LIKE_BUFFER;
	if ((Buffer=NuHTempOK(BufferSize))==nil)
	{
		err=MemError();
		goto failure;
	}
	
	/*
	 * fill the first buffer
	 */
	BFilled = BufferSize-1;
	LDRef(Buffer);
	err=ARead(RefN,&BFilled,*Buffer);
	UL(Buffer);
	if (err && err!=eofErr) goto failure;
	BSpot = LastSpot = FSpot = 0;
	(*Buffer)[BFilled] = '\015';			/* a marker, to expedite searches */
	return(noErr);

failure:
	CloseLine(lip);
	return(err);
}

/************************************************************************
 * SeekLine - seek the line routines to a given spot
 ************************************************************************/
int SeekLine(long spot,LineIOP lip)
{
	int err;
	
	if (err = SetFPos(RefN,fsFromStart,spot)) goto failure;
	
	/*
	 * fill the first buffer
	 */
	BFilled = BufferSize-1;
	LDRef(Buffer);
	err=ARead(RefN,&BFilled,*Buffer);
	UL(Buffer);
	if (err && err!=eofErr) goto failure;
	Eof = False;
	BSpot = 0;
	LastSpot = FSpot = spot;
	(*Buffer)[BFilled] = '\015';			/* a marker, to expedite searches */
	return(noErr);

failure:
	CloseLine(lip);
	return(err);
}

/**********************************************************************
 * NLGetLine - get a line, possiby preceeded by a linefeed
 **********************************************************************/
int NLGetLine(UPtr line,int size, long *len,LineIOP lip)
{
	short l = GetLine(line,size,len,lip);
	
	if (l==LINE_START && *len && *line=='\012')
	{
		BMD(line+1,line,--*len);
		if (!*len) l = 0;
	}
	return(l);
}

/**********************************************************************
 * GetLine - read a line of a given size.  returns 0 for eof, negative
 * for file manager errors, LINE_START if returning the beginning of
 * a line, LINE_MIDDLE if a partial line is being returned.
 **********************************************************************/
int GetLine(UPtr line,int size, long *len,LineIOP lip)
{
	register UPtr bp;
	register UPtr cp=line;
	int where;
	int err;
	static FILE *trace=0;
	
	if (!BFilled) return (0); 		/* we have no chars */
	size--; //make sure we don't overrun buffer
	CycleBalls();
	bp = LDRef(Buffer) + BSpot;
	where = (bp==*Buffer || bp[-1]=='\015') ? LINE_START : LINE_MIDDLE;
	LastSpot = FSpot + BSpot; 	/* remember where this line begins */
	for (;;)
	{
		while (*bp!='\015' && --size>0) {*cp = *bp++; if (!*cp) *cp=' '; cp++;}
		BSpot = bp - *Buffer;
		if (BSpot==BFilled)
		{
			FSpot += BFilled;
			BFilled = BufferSize - 1;
			err=ARead(RefN,&BFilled,*Buffer);
			Eof = !BFilled;
			if (err==eofErr)
			{
				if (BFilled==0)
				{
					*cp = 0;
					BFilled = 0;
					UL(Buffer);
					if (len) *len = cp-line;
					return (where);
				}
			}
			else if (err)
			{
				UL(Buffer);
				FileSystemError(READ_MBOX,"",err);
				return(err);
			}
			(*Buffer)[BFilled] = '\015';	/* a marker, to expedite searches */
			BSpot = 0;
			bp = *Buffer;
		}
		else
		{
			if (size>0)
			{
				*cp++ = '\015';
				BSpot++;
			}
			*cp = 0;
			UL(Buffer);
			if (len) *len = cp-line;
			return(where);
		}
	}
}

/**********************************************************************
 * CloseLine - shut up shop.	Calling it on closed file does no harm.
 **********************************************************************/
void CloseLine(LineIOP lip)
{
	if (lip)
	{
		if (RefN != 0)
		{
			FSClose(RefN);	// not MyFSClose, because reading only
			RefN = 0;
		}
		if (Buffer != nil)
		{
			ZapHandle(Buffer);
			Buffer = nil;
		}
		BFilled = 0;
		Zero(*lip);
	}
}

long TellLine(LineIOP lip)
{
	return (LastSpot);
} 

