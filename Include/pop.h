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

#ifndef POP_H
#define POP_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with POP server
 ************************************************************************/

typedef struct POPDesc {
	uLong receivedGMT;	/* timestamp on outermost Received: header */
	uLong uidHash;		/* hash function of message-id */
	long msgSize;		/* size of the message */
	Boolean retr;		/* should we fetch this message? */
	Boolean retred;		/* fetched successfully */
	Boolean delete;		/* should we delete it if we manage to fetch it? */
	Boolean deleted;	/* did we delete it? */
	Boolean stub:1;		/* should we download a stub? */
	Boolean stubbed:1;	/* do we have a stub? */
	Boolean skip:1;		/* are we stubbing this because of sbm? */
	Boolean big2:1;		/* are we stubbing this because it's too big? */
	Boolean head:1;		/* are we stubbing because of download headers? */
	Boolean error:1;	/* was there an error downloading? */
	Boolean spare:2;	/* rest of the bits */
} POPDesc, *POPDPtr, **POPDHandle;

Boolean GenKeyedDigest(UPtr banner, UPtr secret, UPtr digest);
Boolean GenDigest(UPtr banner, UPtr secret, UPtr digest);
void FixMessServerAreas(void);
OSErr RoomForMessage(long msgsize);
short ReadEitherBody(TransStream stream, short refN, HeaderDHandle hdh,
		     char *buf, long bSize, long estSize, long context);
POPLineType ReadPOPLine(TransStream stream, UPtr buf, long bSize,
			long *len);
short GetMyMail(TransStream stream, Boolean quietly, short *gotSome,
		struct XferFlags *flags);
int POPError(void);
OSErr RecordAttachment(FSSpecPtr spec, HeaderDHandle hdh);
void AddAttachInfo(short theIndex, long result);
OSErr WriteAttachNote(short refN);
int FetchMessageText(TransStream stream, long estSize, POPDPtr pd,
		     short messageNumber, TOCHandle useTocH);
int FetchMessageTextLo(TransStream stream, long estSize, POPDPtr pd,
		       short messageNumber, TOCHandle useTocH,
		       Boolean imap, Boolean import);
int POPrror(void);
int POPIntroductions(TransStream stream, PStr user,
		     Boolean * capabilities);
int POPCmdGetReply(TransStream stream, short cmd, UPtr args, UPtr buffer,
		   long *size);
int POPCmdError(short cmd, UPtr args, UPtr message);
int EndPOP(TransStream stream);
int StartPOP(TransStream stream, UPtr serverName, long port);
void AttachNoteLo(FSSpecPtr spec, PStr theMessage);
#ifdef BATCH_DELIVERY_ON
TOCHandle RenameInTemp(TOCHandle tocH);
#endif

/*
 * POPD manipulation stuff
 */
OSErr AddIdToPOPD(OSType listType, short listId, uLong uidHash,
		  Boolean dupOK);
OSErr RemIdFromPOPD(OSType listType, short listId, uLong uidHash);
Boolean IdIsOnPOPD(OSType listType, short listId, uLong uidHash);
// Convenience macros
#define PERS_FORCE(x)	((M_T1=(long)(x)),(M_T1?(PersHandle)M_T1:PersList))

#define SUM_TO_PERS(sum)	(FindPersById((sum)->persId))
#define TS_TO_PERS(tocH,sumNum)	(SUM_TO_PERS((*tocH)->sums+sumNum))
#define MESS_TO_PERS(messH)	(SUM_TO_PERS(SumOf(messH)))

#define SUM_TO_PPERS(sum)	(FindPersById((sum)->popPersId))
#define TS_TO_PPERS(tocH,sumNum)	(SUM_TO_PPERS((*tocH)->sums+sumNum))
#define MESS_TO_PPERS(messH)	(SUM_TO_PPERS(SumOf(messH)))

#define MessOnPOPD(list,messH) (IdIsOnPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),list,SumOf(messH)->uidHash))
#define SumOnPOPD(list,sum) (IdIsOnPOPD(PERS_POPD_TYPE(SUM_TO_PPERS(sum)),list,(sum)->uidHash))
#define TSOnPOPD(list,tocH,sumNum) (IdIsOnPOPD(PERS_POPD_TYPE(TS_TO_PPERS(tocH,sumNum)),list,(*tocH)->sums[sumNum].uidHash))
#define RemMessFromPOPD(list,messH) (RemIdFromPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),list,SumOf(messH)->uidHash))
#define RemSumFromPOPD(list,sum) (RemIdFromPOPD(PERS_POPD_TYPE(SUM_TO_PPERS(sum)),list,(sum)->uidHash))
#define RemTSFromPOPD(list,tocH,sumNum) (RemIdFromPOPD(PERS_POPD_TYPE(TS_TO_PPERS(tocH,sumNum)),list,(*tocH)->sums[sumNum].uidHash))
#define AddMessToPOPD(list,messH,dupOk) (AddIdToPOPD(PERS_POPD_TYPE(MESS_TO_PPERS(messH)),list,SumOf(messH)->uidHash,dupOk))
#define AddSumToPOPD(list,sum,dupOk) (AddIdToPOPD(PERS_POPD_TYPE(SUM_TO_PPERS(sum)),list,(sum)->uidHash,dupOk))
#define AddTSToPOPD(list,tocH,sumNum,dupOk) (AddIdToPOPD(PERS_POPD_TYPE(TS_TO_PPERS(tocH,sumNum)),list,(*tocH)->sums[sumNum].uidHash,dupOk))

OSErr KerbDestroy(void);
OSErr KerbDestroyUser(void);
int PutOutFromLine(short refN, long *fromLen);
OSErr KerbUsername(PStr name);

#ifdef POPSECURE
short VetPOP(void);
#endif


#endif
