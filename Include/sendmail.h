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

#ifndef SENDMAIL_H
#define SENDMAIL_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with sendmail
 ************************************************************************/
typedef struct AttMapStruct *AttMapPtr;
typedef struct CountedSpecStruct **CSpecHandle;
OSErr AllAttachOnBoardLo(MessHandle messH, Boolean errReport);
PStr MessReturnAddr(MessHandle messH, PStr buffer);
int StartSMTP(TransStream stream, UPtr serverName, long Port);
int MySendMessage(TransStream stream, TOCHandle tocH, int sumNum,
		  CSpecHandle specList);
int SMTPError(TransStream stream);
int EndSMTP(TransStream stream);
MessHandle SaveB4Send(TOCHandle tocH, short sumNum);
int DoRcptTos(TransStream stream, MessHandle messH, Boolean chatter,
	      CSpecHandle fccList, AccuPtr newsGroupAcc);
int DoRcptTosFrom(TransStream stream, MessHandle messH, short index,
		  Boolean chatter, CSpecHandle fccList,
		  AccuPtr newsGroupAcc);
int TransmitMessageLo(TransStream stream, MessHandle messH,
		      Boolean chatter, Boolean mime, Boolean others,
		      emsMIMEHandle * tlMIME, Boolean sendDataCmd,
		      Boolean finishSMTP, Boolean doTopLevel);
int TransmitMessage(TransStream stream, MessHandle messH, Boolean chatter,
		    Boolean mime, Boolean others, emsMIMEHandle * tlMIME,
		    Boolean sendDataCmd);
OSErr TransmitHeaders(TransStream stream, MessHandle messH,
		      UHandle enriched, PStr boundary, Boolean mime,
		      Boolean others, emsMIMEHandle * tlMIME,
		      Boolean isRelated);
void TimeStamp(TOCHandle tocH, short sumNum, uLong when, long delta);
void PtrTimeStamp(MSumPtr sum, uLong when, long delta);
#define GetReply(stream,buffer,size,chatter,isEhlo) GetReplyLo(stream,buffer,size,nil,chatter,isEhlo)
int GetReplyLo(TransStream stream, UPtr buffer, int size, AccuPtr bufAcc,
	       Boolean chatter, Boolean isEhlo);
int SendBodyLines(TransStream stream, UHandle text, long length,
		  long offset, long flags, Boolean forceLines,
		  short *lineStarts, short nLines, Boolean partial,
		  DecoderFunc * encoder);
void BuildDateHeader(UPtr buffer, long seconds);
PStr R822Date(PStr date, long seconds);
PStr SimpleNameCharset(PStr charset, short tid);
short SendPlain(TransStream stream, FSSpec * spec, long flags,
		short tableId, AttMapPtr amp);
OSErr GetIndAttachment(MessHandle messH, short index, FSSpecPtr spec,
		       HSPtr where);
OSErr GetIndAttachmentLo(Handle text, short index, FSSpecPtr spec,
			 HSPtr where, HeadSpec * hs);
void BuildBoundary(MessHandle messH, PStr boundary, PStr middle);
Boolean IsPostScript(FSSpecPtr spec);
short EffectiveTID(short tid);
short TransOutTablID(void);
PStr TransOutTablName(PStr name);
UPtr PriorityHeader(UPtr buffer, Byte priority);
OSErr SendRawMIME(TransStream stream, FSSpecPtr spec);
OSErr SendTextFile(TransStream stream, FSSpecPtr spec, long flags,
		   DecoderFunc * encoder);
OSErr FinishSMTP(TransStream stream, MessHandle messH);
OSErr AddFccToList(PStr fcc, CSpecHandle list);
PStr BuildContentID(PStr into, PStr mid, long partID, short index);
typedef enum {
	SysStatCode = 211,
	HelpCode = 214,
	ReadyCode = 220,
	CloseCode,
	OkCode = 250,
	ForwardCode = 251,

	StartInputCode = 354,

	NoServiceCode = 421,
	BoxBusyCode = 450,
	LocalErrCode,
	SysFullCode,

	SyntaxCode = 500,
	ArgsBadCode,
	CmdUnImpCode,
	OrderBadCode,
	ArgUnImpCode,
	NoBoxCode = 550,
	YouForwardCode,
	BoxFullCode,
	BoxBadCode,
	PuntCode,

	TransErr = 601,
	RecvErr,
	ReplyErr
} SMErrEnum;

OSErr BufferSend(TransStream stream, DecoderFunc * encoder, UPtr data,
		 long dataLen, Boolean text);
#define BS(stream,x,y,z) do{											\
	lastLen = z;														\
	lastC = (y)[z-1];													\
	if (sErr=BufferSend(stream,x,y,z,True))				\
		goto done;														\
	}while(0)
#define BSCLOSE(stream,e) do{if (sErr=BufferSend(stream,e,nil,nil,True)) goto done;}while(0)
#define SendBoundary(stream) (/*SendTrans(nil,NewLine+1,*NewLine,NewLine+1,*NewLine,nil)||*/SendTrans(stream,"--",2,boundary+1,*boundary,NewLine+1,*NewLine,nil))
#define BufferSendRelease(stream)	(void) BufferSend(stream,nil,nil,-1,False)
OSErr SendPString(TransStream stream, PStr string);
PStr FormatZone(PStr string, long delta);
UPtr GetFlatten(void);
OSErr SendExtras(TransStream stream, Handle extras, Boolean allowQP,
		 short tid);
Boolean AnyFunny(Handle text, long offset);

typedef struct {
	MessHandle messH;	// The message we are sending
	Boolean strip;		// Should we strip styles?
	Boolean bloat;		// Should we send multipart/alternative?
	Boolean rich;		// Should we send text/enriched?
	Boolean html;		// Should we send text/html?
	Boolean isRelated;	// Should we send multipart/related?
	Boolean hasAttachments;	// Do we have attachments?
	Boolean mime;		// Do we want to generate the MIME headers?
	Boolean others;		// Do we want to generate the other headers?
	Boolean hasSig;		// Does the message have a signature?
	Boolean receipt;	// Are we sending a return receipt?
	Boolean allLWSP;	// Is the message solely LWSP?
	StackHandle parts;	// Stack of parts
	Accumulator enriched;	// Rich data of message body, including preamble
	long flags;		// local copy of message flags
	long opts;		// local copy of message options
	HeadSpec hs;		// headspec describing body of message
	TransStream stream;	// the stream to send it on
	DecoderFunc *encoder;	// and the encoder
	emsMIMEHandle *tlMIME;	// MIME headers for translators
} TransmitPB, *TransmitPBPtr, **TransmitPBHandle;

OSErr TransmitMimeVersion(TransmitPBPtr pb);

#define IsAddrErr(c)	(((c)/10)==55||(c)==503||(c)==543)

#endif
