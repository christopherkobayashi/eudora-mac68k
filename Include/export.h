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

#ifndef EXPORT_H
#define EXPORT_H

void DoFileExport(void);

typedef OSErr ExporterFunc( FSSpecPtr specPtr );

OSErr ExportTo(FSSpecPtr exportFolderSpec);
ExporterFunc 
	ExportMailTo,
	ExportAddressBookTo,
	ExportFiltersTo,
	ExportStationeryTo;


#define MOZ_MSG_FLAG_READ     0x0001    /* has been read */
#define MOZ_MSG_FLAG_REPLIED  0x0002    /* a reply has been successfully sent */
#define MOZ_MSG_FLAG_MARKED   0x0004    /* the user-provided mark */
#define MOZ_MSG_FLAG_EXPUNGED 0x0008    /* already gone (when folder not
                                       compacted.)  Since actually
                                       removing a message from a
                                       folder is a semi-expensive
                                       operation, we tend to delay it;
                                       messages with this bit set will
                                       be removed the next time folder
                                       compaction is done.  Once this
                                       bit is set, it never gets
                                       un-set.  */
#define MOZ_MSG_FLAG_HAS_RE   0x0010    /* whether subject has "Re:" on
                                       the front.  The folder summary
                                       uniquifies all of the strings
                                       in it, and to help this, any
                                       string which begins with "Re:"
                                       has that stripped first.  This
                                       bit is then set, so that when
                                       presenting the message, we know
                                       to put it back (since the "Re:"
                                       is not itself stored in the
                                       file.)  */
#define MOZ_MSG_FLAG_ELIDED   0x0020    /* Whether the children of this
                                       sub-thread are folded in the
                                       display.  */
#define MOZ_MSG_FLAG_OFFLINE  0x0080    /* db has offline news or imap article
                                             */
#define MOZ_MSG_FLAG_WATCHED  0x0100    /* If set, then this thread is watched (in
                                       3.0, this was MOZ_MSG_FLAG_UPDATING).*/
#define MOZ_MSG_FLAG_SENDER_AUTHED    0x0200    /* If set, then this message's sender
                                       has been authenticated when sending this msg. */
#define MOZ_MSG_FLAG_PARTIAL  0x0400    /* If set, then this message's body is
                                       only the first ten lines or so of the
                                       message, and we need to add a link to
                                       let the user download the rest of it
                                       from the POP server. */
#define MOZ_MSG_FLAG_QUEUED   0x0800    /* If set, this message is queued for
                                       delivery.  This only ever gets set on
                                       messages in the queue folder, but is
                                       used to protect against the case of
                                       other messages having made their way
                                       in there somehow -- if some other
                                       program put a message in the queue, we
                                       don't want to later deliver it! */
#define MOZ_MSG_FLAG_FORWARDED  0x1000  /* this message has been forwarded */
#define MOZ_MSG_FLAG_PRIORITIES 0xE000  /* These are used to remember the message
ine MOZ_MSG_FLAG_NEW        0x10000 /* This msg is new since the last time
                                       the folder was closed.
                                       */
#define MOZ_MSG_FLAG_IGNORED    0x40000 /* the thread is ignored */

#define MOZ_MSG_FLAG_IMAP_DELETED   0x200000 /* message is marked deleted on the server */

#define MOZ_MSG_FLAG_MDN_REPORT_NEEDED 0x400000 /* This msg required to send an MDN 
                                             * to the sender of the message
                                             */
#define MOZ_MSG_FLAG_MDN_REPORT_SENT   0x800000 /* An MDN report message has been
                                             * sent for this message. No more
                                             * MDN report should be sent to the
                                             * sender
                                             */
#define MOZ_MSG_FLAG_TEMPLATE       0x1000000   /* this message is a template */
#define MOZ_MSG_FLAG_ATTACHMENT     0x10000000  /* this message has files attached to it */

#define MOZ_MSG_FLAG_LABELS 0xE000000   /* These are used to remember the message labels */

#endif // EXPORT_H
