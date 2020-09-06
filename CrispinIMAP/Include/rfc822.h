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

#ifndef RFC822_H
#define RFC822_H

/*
 * Program:	RFC-822 routines (originally from SMTP)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 July 1988
 * Last Edited:	29 January 1997
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1997 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Function prototypes */

void rfc822_header (char *IMAPheader,ENVELOPE *env,IMAPBODY *body);
void rfc822_address_line (char **IMAPheader,char *type,ENVELOPE *env,ADDRESS *adr);
void rfc822_header_line (char **IMAPheader,char *type,ENVELOPE *env,char *text);
void rfc822_write_address (char *dest,ADDRESS *adr);
void rfc822_address (char *dest,ADDRESS *adr);
void rfc822_cat (char *dest,char *src,const char *specials);
void rfc822_write_body_header (char **IMAPheader,IMAPBODY *body);
char *rfc822_default_subtype (unsigned short type);
void rfc822_parse_msg (ENVELOPE **en,IMAPBODY **bdy,char *s,unsigned long i,
		       STRING *bs,char *host);
void rfc822_parse_content (IMAPBODY *body,STRING *bs,char *h);
void rfc822_parse_content_header (IMAPBODY *body,char *name,char *s);
void rfc822_parse_parameter (PARAMETER **par,char *text);
void rfc822_parse_adrlist (ADDRESS **lst,char *string,char *host);
ADDRESS *rfc822_parse_address (ADDRESS **lst,ADDRESS *last,char **string,
			       char *defaulthost);
ADDRESS *rfc822_parse_group (ADDRESS **lst,ADDRESS *last,char **string,
			     char *defaulthost);
ADDRESS *rfc822_parse_mailbox (char **string,char *defaulthost);
ADDRESS *rfc822_parse_routeaddr (char *string,char **ret,char *defaulthost);
ADDRESS *rfc822_parse_addrspec (char *string,char **ret,char *defaulthost);
char *rfc822_parse_phrase (char *string);
char *rfc822_parse_word (char *string,const char *delimiters);
char *rfc822_cpy (char *src);
char *rfc822_quote (char *src);
ADDRESS *rfc822_cpy_adr (ADDRESS *adr);
void rfc822_skipws (char **s);
char *rfc822_skip_comment (char **s,long trim);
char *rfc822_contents (char **dst,unsigned long *dstl,unsigned long *len,
		       char *src,unsigned long srcl,unsigned short encoding);

typedef long (*soutr_t) (void *stream,char *string);
typedef long (*rfc822out_t) (char *t,ENVELOPE *env,IMAPBODY *body,soutr_t f,
			     void *s,long ok8bit);

long rfc822_output (char *t,ENVELOPE *env,IMAPBODY *body,soutr_t f,void *s,
		    long ok8bit);
void rfc822_encode_body_7bit (ENVELOPE *env,IMAPBODY *body);
void rfc822_encode_body_8bit (ENVELOPE *env,IMAPBODY *body);
long rfc822_output_body (IMAPBODY *body,soutr_t f,void *s);
void *rfc822_base64 (unsigned char *src,unsigned long srcl,unsigned long *len);
unsigned char *rfc822_binary (void *src,unsigned long srcl,unsigned long *len);
unsigned char *rfc822_qprint (unsigned char *src,unsigned long srcl,
			      unsigned long *len);
unsigned char *rfc822_8bit (unsigned char *src,unsigned long srcl,
			    unsigned long *len);

#endif //RFC822_H