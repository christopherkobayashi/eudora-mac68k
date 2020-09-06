/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LEX822_H
#define LEX822_H

typedef enum
{
	LinearWhite,
	Atom,
	QText,
	RegText,
	DomainLit,
	t822Comment,			/* conflict with "Comment" in AIFF.h */
	Special,
	EndOfHeader,
	EndOfMessage,
	ErrorToken,				/* something went wrong */
	Continue822
} Token822Enum;

typedef enum
{
	Init822,				/* get started */
	CollectLine,		/* not doing anything special right at the moment */
	CollectLWSP,		/* collecting a run of LWSP */
	CollectAtom,		/* collecting an atom */
	CollectComment,	/* collecting an RFC 822 comment */
	CollectQText,		/* collecting a quoted text string */
	CollectDL,			/* collecting a domain literal */
	CollectSpecial,	/* collecting a single special character */
	CollectText,		/* collecting an unstructured field body */
									/*   we have to be put in this state by an external force */
	ReceiveError,		/* ran out of characters */
	State822Limit
} State822Enum;

typedef struct
{
	State822Enum state;
	Str255 token;							/* tokens over 255 characters will be shot */
	Str255 buffer;						/* input buffer line */
	UPtr spot;								/* spot we're currently processing */
	UPtr end;									/* time to die */
	short inStructure;				/* in a quote, comment, or domain literal */
	Boolean reinitToken;			/* have we seen the end of the input stream? */
	Boolean uhOh;							/* we tawt we taw a putty tat */
	Boolean has1522;					/* RFC 1522 encoded data found */
} Lex822State, *L822SPtr, **L822SHandle;

typedef struct Accumulator *AccuPtr;
Token822Enum Lex822(TransStream stream, L822SPtr l822p,AccuPtr fullHeaders);

PStr Quote822(PStr into,PStr from,Boolean spaces);
short DecodeB64String(PStr s);
void PseudoQP(PStr text);

#endif
