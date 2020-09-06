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

#ifndef LINEIO_H
#define LINEIO_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
typedef struct LineIODesc
{
	UHandle buffer;	 /* where our characters go */
	long bufferSize; 		/* how big our buffer is */
	long bFilled;			/* the number of chars in the buffer */
	short refN;					/* the path number of the open file */
	long bSpot;					/* index of next character to transfer */
	long lastSpot; 			/* the file position of beginning of
																the last line read */
	long fSpot;					/* the position in the file of the start
																of the buffer */
	Boolean eof;				/* have we hit eof? */
} LineIOD, *LineIOP, **LineIOH;

short OpenLine(short vRef,long dirId,UPtr name,short perm,LineIOP lip);
#define FSpOpenLine(spec,perm,lip) OpenLine((spec)->vRefNum,(spec)->parID,(spec)->name,perm,lip)
int GetLine(UPtr line,int size,long *len,LineIOP lip);
void CloseLine(LineIOP lip);
long TellLine(LineIOP lip);
int SeekLine(long spot,LineIOP lip);
typedef enum {LINE_START=1, LINE_MIDDLE} GetLineEnum;
int NLGetLine(UPtr line,int size, long *len,LineIOP lip);


#endif
