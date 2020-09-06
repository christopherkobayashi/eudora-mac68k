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

/*
 * Program:	Network message (SMTP/NNTP/POP2/POP3) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	8 June 1995
 * Last Edited:	12 February 1997
 *
 * Copyright 1997 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "netmsg.h"

/* String driver for stdio file strings */

STRINGDRIVER netmsg_string = {
  netmsg_string_init,		/* initialize string structure */
  netmsg_string_next,		/* get next byte in string structure */
  netmsg_string_setpos		/* set position in string structure */
};


/* Initialize mail string structure for file
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void netmsg_string_init (STRING *s,void *data,unsigned long size)
{
  s->data = data;		/* note file descriptor */
				/* big enough for one byte */
  s->chunk = s->curpos = (char *) &s->data1;
  s->size = size;		/* data size */
  s->cursize = s->chunksize = 1;/* always call stdio */
  netmsg_string_setpos (s,0);	/* initial offset is 0 */
}


/* Get next character from string
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char netmsg_string_next (STRING *s)
{
  char ret = *s->curpos;
  s->offset++;			/* advance position */
  s->cursize = 1;		/* reset size */
  *s->curpos = (char) getc ((FILE *) s->data);
  return ret;
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void netmsg_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* note new offset */
  fseek ((FILE *) s->data,i,L_SET);
  *s->curpos = (char) getc ((FILE *) s->data);
}

/* Network message read
 * Accepts: file
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long netmsg_read (void *stream,unsigned long count,char *buffer)
{
  return (fread (buffer,(size_t) 1,(size_t) count,(FILE *) stream) == count) ?
    T : NIL;
}


/* Slurp dot-terminated text from NET
 * Accepts: NET stream
 *	    place to return size
 * Returns: text
 */

char *netmsg_slurp_text (TransStream stream,unsigned long *size)
{
	FILE *f = netmsg_slurp (stream,size,NIL);
	char *s = (char *) fs_get ((size_t) *size + 1);
	
	if (s)
	{
		/* read from temp file */
		fread (s,(size_t) 1,(size_t) *size,f);
		s[*size] = '\0';		/* tie off string */
		fclose (f);			/* flush temp file */
	}
	
	return s;
}

/* Slurp dot-terminated text from NET
 * Accepts: NET stream
 *	    place to return size
 *	    place to return IMAPheader size
 * Returns: file descriptor
 */

FILE *netmsg_slurp (TransStream stream,unsigned long *size,unsigned long *hsiz)
{
  unsigned long i;
  char *s = 0, *t, tmp[MAILTMPLEN];
  FILE *f = tmpfile ();
  *size = 0;			/* initially emtpy */
  if (hsiz) *hsiz = 0;
  while (s = net_getline (stream)) {
    if (*s == '.') {		/* possible end of text? */
      if (s[1]) t = s + 1;	/* pointer to true start of line */
      else {
	fs_give ((void **) &s);	/* free the line */
		break;			/* end of data */
      }
    }
    else t = s;			/* want the entire line */
    if (f) {			/* copy it to the file */
      i = strlen (t);		/* size of line */
      if ((fwrite (t,(size_t) 1,(size_t) i,f) == i) &&
	  (fwrite ("\015\012",(size_t) 1,(size_t) 2,f) == 2)) {
	*size += i + 2;		/* tally up size of data */
				/* note IMAPheader position */
	if (!i && hsiz && !*hsiz) *hsiz = *size;
      }
      else {
	sprintf (tmp,"Error writing scratch file at byte %lu",*size);
	mm_log (tmp,ERROR);
	fclose (f);		/* forget it */
	f = NIL;		/* failure now */
      }
    }
    fs_give ((void **) &s);	/* free the line */
  }
  if (f) {			/* making a file? */
    fwrite ("\015\012",1,2,f);
    *size += 2;			/* write final newline */
				/* rewind to start of file */
    fseek (f,(unsigned long) 0,L_SET);
  }
				/* IMAPheader consumes entire message */
  if (hsiz && !*hsiz) *hsiz = *size;
  return f;			/* return the file descriptor */
}