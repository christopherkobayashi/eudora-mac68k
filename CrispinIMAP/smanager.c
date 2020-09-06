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

#ifdef	IMAP

/*
 * Program:	Subscription Manager
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 December 1992
 * Last Edited:	23 July 1995
 *
 * Copyright 1995 by the University of Washington
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

#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "env_mac.h"

/* Subscribe to mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_subscribe (char *mailbox)
{
  FILE *f;
  char *s,db[MAILTMPLEN],tmp[MAILTMPLEN];
  SUBSCRIPTIONFILE (db);	/* open subscription database */
  if (f = fopen (db,"r")) {	/* make sure not already there */
    while (fgets (tmp,MAILTMPLEN,f)) {
      if (s = strchr (tmp,'\n')) *s = '\0';
      if (!strcmp (tmp,mailbox)) {/* already subscribed? */
	sprintf (tmp,"Already subscribed to mailbox %s",mailbox);
	mm_log (tmp,IMAP_ERROR);
	fclose (f);
	return NIL;
      }
    }
    fclose (f);
  }
  if (!(f = fopen (db,"a"))) {	/* append new entry */
    mm_log ("Can't create subscription database",IMAP_ERROR);
    return NIL;
  }
  fprintf (f,"%s\n",mailbox);
  return (fclose (f) == EOF) ? NIL : T;
}

/* Unsubscribe from mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_unsubscribe (char *mailbox)
{
  FILE *f,*tf;
  char *s,tmp[MAILTMPLEN],old[MAILTMPLEN],new[MAILTMPLEN];
  long ret = NIL;
  SUBSCRIPTIONFILE (old);	/* open subscription database */
  if (!(f = fopen (old,"r"))) {	/* can we? */
    mm_log ("No subscriptions",IMAP_ERROR);
    return NIL;
  }
  SUBSCRIPTIONTEMP (new);	/* make tmp file name */
  if (!(tf = fopen (new,"w"))) {
    mm_log ("Can't create subscription temporary file",IMAP_ERROR);
    fclose (f);
    return NIL;
  }
  while (fgets (tmp,MAILTMPLEN,f)) {
    if (s = strchr (tmp,'\n')) *s = '\0';
    if (strcmp (tmp,mailbox)) fprintf (tf,"%s\n",tmp);
    else ret = T;		/* found the name */
  }
  fclose (f);
  if (fclose (tf) == EOF) {
    mm_log ("Can't write subscription temporary file",IMAP_ERROR);
    return NIL;
  }
  if (!ret) {
    sprintf (tmp,"Not subscribed to mailbox %s",mailbox);
    mm_log (tmp,IMAP_ERROR);		/* error if at end */
  }
  else rename (new,old);
  return ret;
}

/* Read subscription database
 * Accepts: pointer to subscription database handle (handle NIL if first time)
 * Returns: character string for subscription database or NIL if done
 */

static char sbname[MAILTMPLEN];

char *sm_read (void **sdb)
{
  FILE *f = (FILE *) *sdb;
  char *s;
  if (!f) {			/* first time through? */
    SUBSCRIPTIONFILE (sbname);	/* open subscription database */
				/* make sure not already there */
    if (f = fopen (sbname,"r")) *sdb = (void *) f;
    else return NIL;
  }
  if (fgets (sbname,MAILTMPLEN,f)) {
    if (s = strchr (sbname,'\n')) *s = '\0';
    return sbname;
  }
  fclose (f);			/* all done */
  *sdb = NIL;			/* zap sdb */
  return NIL;
}

#endif	//IMAP