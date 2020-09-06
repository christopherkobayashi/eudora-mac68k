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

#include "uupc.h"
#define FILE_NUM 47

Boolean SkipToFromLine(void);
short GetUUPCLip(TOCHandle destMBox, Boolean isMailbox, short *gotSome);

/************************************************************************
 * Incoming from UUPC
 *	- We put the pathname in the POP account, preceeded by a !.
 *	- Then, it's just a matter of sicking FetchMessageText on each message.
 *	--- Well, we make a new RecvLine that returns ".\015" for EOF or
 *			a uucp envelope; that helps.
 *	--- We also have the luxury of being able to back up and suck in raw
 *			binhex/uudecode if the conversion fails
 ************************************************************************/
#pragma segment UUPCIn
static LineIOP Lip;

/************************************************************************
 * GetUUPCMail - read mail from the named mailbox
 ************************************************************************/
short GetUUPCMail(Boolean quietly, short *gotSome)
{
#pragma unused(quietly)
	Str255 mailpath;
	short err;
	short anyErr = 0;
	FSSpec spec;

	/*
	 * get the pathname of the maildrop
	 */
	GetPOPPref(mailpath);
	BMD(mailpath + 2, mailpath + 1, *mailpath - 1);
	(*mailpath)--;
	if (mailpath[*mailpath] == '@')
		-- * mailpath;
	Prr = 0;

	/*
	 * make the spec
	 */
	if (!(err = FSMakeFSSpec(Root.vRef, Root.dirId, mailpath, &spec))) {
		/*
		 * is it empty?
		 */
		if (FSpDFSize(&spec)) {
			Prr = GetUUPCMailSpec(&spec, quietly, gotSome);
		}
	}

	if (err)
		FileSystemError(READ_MBOX, mailpath, err);
	else
		err = Prr;

	return (err);
}


short GetUUPCMailSpecToMailBox(FSSpecPtr spec, SInt8 permission,
			       TOCHandle destBox, short *gotSome)
{
	short err = noErr;
	Boolean isMailbox = IsMailbox(spec);

/*	Check for space on the disk */
	if (Prr = RoomForMessage(FSpDFSize(spec))) {
		FileSystemError(NOT_ENOUGH_ROOM, spec->name, 0);
		return Prr;
	}

/*	allocate the lineio pointer */
	if (Lip)
		DisposePtr((void *) Lip);
	Lip = NuPtrClear(sizeof(*Lip));
	if (!Lip)
		return WarnUser(MEM_ERR, MemError());

/*	Finally, open the file */
	if (err = FSpOpenLine(spec, permission, Lip))
		Prr = FileSystemError(READ_MBOX, spec->name, err);
	else {
		err = GetUUPCLip(destBox, isMailbox, gotSome);
		//      Should we empty the file?
		//      If all worked fine, and we opened the file r/w, then go for it
		if (err == noErr && permission == fsRdWrPerm
		    && !PrefIsSet(PREF_LMOS) && !CommandPeriod)
			SetEOF(Lip->refN, 0);
		//      If there was an error, and the user said ok, then go for it
		if ((err != noErr || CommandPeriod)
		    && ReallyDoAnAlert(CLEAR_DROP_ALRT, Normal) == 1)
			SetEOF(Lip->refN, 0);
		CloseLine(Lip);
		DisposePtr((Ptr) Lip);
		Lip = nil;
	}

	return err;
}



short GetUUPCMailSpec(FSSpecPtr spec, Boolean quietly, short *gotSome)
{
#pragma unused ( quietly )
	return GetUUPCMailSpecToMailBox(spec, fsRdWrPerm, GetInTOC(),
					gotSome);
}


/**********************************************************************
 * GetUUPCMailSpec - get mail from a particular filespec
 **********************************************************************/
short GetUUPCLip(TOCHandle destMBox, Boolean isMailbox, short *gotSome)
{
	short err;
	short anyErr = 0;
	TOCHandle inTocH = destMBox;
	short count;
	long oldPos;
	Boolean foundOne = True;
#ifdef BATCH_DELIVERY_ON
	short batchNum = GetRLong(DELIVERY_BATCH);
	short fetched = 0;
	Boolean inThread = InAThread();
	TOCHandle tocH = GetInTOC();
#endif

	ASSERT(Lip != NULL);
	ASSERT(Lip->refN > 0);
	ASSERT(gotSome != NULL);

	CurTrans = UUPCTrans;

	/*
	 * now, grab messages
	 */
	*gotSome = 0;
	if (!inTocH) {
		Prr = 1;
		goto done;
	}
	if (isMailbox)
		SkipToFromLine();
	do {
		TOCSetDirty(inTocH, true);
		oldPos = TellLine(Lip);
		ProgressR(NoChange, *gotSome + 1, 0, UUPC_COPY, nil);
		BadBinHex = False;
		BadEncoding = 0;
		if (!AttachedFiles)
			AttachedFiles = NuHandle(0);
		SetHandleBig_(AttachedFiles, 0);
		if (destMBox == GetInTOC())
			count = FetchMessageText(NULL, 0, nil, 0, NULL);
		else
			count =
			    FetchMessageTextLo(NULL, 0, nil, 0, inTocH,
					       false, true);
		err = Prr;
		SetHandleBig_(AttachedFiles, 0);
		SaveAbomination(nil, 0);
		if (!err)
			err = CommandPeriod;
		anyErr |= err;
		if (err)
			break;
#ifdef BAD_ENCODING_HANDLING
		if (!CommandPeriod && (BadBinHex || BadEncoding)) {
			RecvLine(NULL, nil, nil);	/* clear old indicator */
			SeekLine(oldPos, Lip);
			SkipToFromLine();
			NoAttachments = True;
			if (BadBinHex) {
				while (count--)
					DeleteMessageLo(inTocH,
							(*inTocH)->count -
							1, True);
			}
			continue;
		} else
#endif
		{
			oldPos = TellLine(Lip);
			NoAttachments = False;
		}
		*gotSome += count;
#ifdef NEVER			// this is a bad idea for reasons I don't entirely understand
#ifdef BATCH_DELIVERY_ON
		if (count)
			fetched++;
		if (inThread && tocH && (fetched % batchNum == 0))
			tocH = RenameInTemp(tocH);
#endif
#endif
		foundOne = SkipToFromLine();
	}
	while (foundOne);
      done:
#ifdef BATCH_DELIVERY_ON
	if (inThread && *gotSome && tocH)
		RenameInTemp(tocH);
#endif
	anyErr |= err || Prr;
	if (!anyErr)
		Progress(100, NoBar, nil, nil, nil);
	NoAttachments = False;
	CurTrans = GetTCPTrans();
	return (anyErr);
}


/************************************************************************
 * UUPCRecvLine - read a line at a time from the maildrop.	Returns ".\015"
 * at the ends of messages.
 ************************************************************************/
OSErr UUPCRecvLine(TransStream stream, UPtr buffer, long *size)
{
	static Boolean wasFrom;
	static Boolean wasNl = True;
	short lineType;

	if (MiniEvents())
		return (userCancelled);
	if (!buffer) {
		Boolean retVal = wasFrom;
		wasFrom = False;
		return (retVal);
	}
	wasFrom = False;
	(*size)--;

#ifdef DEBUG
	if (*size < 100)
		Debugger();
#endif

	ASSERT(Lip != NULL);
	lineType = GetLine(buffer, *size, size, Lip);
	if (!*size || !lineType || wasNl && (wasFrom = IsFromLine(buffer))) {
		*size = 2;
		buffer[0] = '.';
		buffer[1] = '\015';
		buffer[2] = 0;
	} else if (lineType && wasNl && *buffer == '.') {
		BMD(buffer, buffer + 1, *size);
		(*size)++;
		*buffer = '.';
		buffer[*size] = 0;
	}
	wasNl = !lineType || buffer[*size - 1] == '\015';
	if (IsRecAudit(stream))
		stream->bytesTransferred += *size;
	return (noErr);
}

/************************************************************************
 * SkipToFromLine - skip to the next envelope.	Also gives time to others,
 * and keeps track of whether or not we last read a From line.
 ************************************************************************/
Boolean SkipToFromLine(void)
{
	Str255 buffer;
	long eof, atNow, size;
	short err;

	ASSERT(Lip != NULL);
	if (MiniEvents() && CommandPeriod) {
		Prr = userCancelled;
		return (False);
	}
	if ((err = !Lip) || (err = GetEOF(Lip->refN, &eof))) {
		FileSystemError(READ_MBOX, GetPOPPref(buffer), err);
		Prr = err;
		return (False);
	}
	atNow = TellLine(Lip);
	if (eof)
		ByteProgress(nil, atNow, eof);
	if (Lip->eof)
		return (False);
	if (RecvLine(NULL, nil, nil))
		return (True);
	do {
		size = sizeof(buffer);
		if (err = RecvLine(NULL, buffer, &size)) {
			FileSystemError(READ_MBOX, GetPOPPref(buffer),
					err);
			Prr = err;
			return (False);
		} else
			ByteProgress(nil, -size, 0);
	}
	while (size != 2 || *buffer != '.');

	return (True);
}

#pragma segment UUPCOut
/************************************************************************
 * Ok; sending mail is more complicated, but it's easier to do
 * SMTP server item has to have the following info, ! separated:
 *	- the name of our mac
 *	- the name of the remote system (optional, ignored if present)
 *	- the path to the spool directory
 *	- our user name
 *	- our current sequence number
 *	ie, !mymac!relay!spoolvol:spooldir:!username!0000
 *	- We generate a sequence number, 4 digits (####)
 *	- We deposit the mail in D.relay0####, newline-separated
 *	- In D.mymac0####, we put commands for the UUCP system, newline-separated
 *	--- U username mymac				; this is us
 *	--- F D.mymac0####					; this is the mail file, on the remote system
 *	--- I D.mymac0####					; and we're using it for input
 *	--- C rmail <recip-list>		; the command; rmail and recipients as args
 *
 *	By declaring our own SendTrans, we actually can use TransmitMessage
 *	from sendmail.c, which is tres nice.
 *
 *	There are some race conditions in creating the files.  So what?
 ************************************************************************/

typedef struct {
	Str255 spoolPath;
	Str63 myMac;
	unsigned char relay[9];
	Str63 userName;
	short sequence;
	short refNs[2];
	Str255 files[2];
	Str31 fileNames[2];
	short oldSeq;
	MessHandle messH;
} UUXBlock, *UUXPtr, **UUXHandle;
UUXHandle UUX;
#define SpoolPath (*UUX)->spoolPath
#define MyMac (*UUX)->myMac
#define Relay (*UUX)->relay
#define UserName (*UUX)->userName
#define Sequence (*UUX)->sequence
#define MailRefN (*UUX)->refNs[0]
#define XRefN (*UUX)->refNs[1]
#define MailFile (*UUX)->files[0]
#define XFile (*UUX)->files[1]
#define Dmymac (*UUX)->fileNames[0]
#define Xmymac (*UUX)->fileNames[1]
#define OldSeq (*UUX)->oldSeq
#define MessH (*UUX)->messH

void UUPCGenFilenames(void);
Boolean UUPCBadFilenames(void);
int UUPCMakeFiles(void);
void UUPCKillFiles(void);
void UUPCCloseFiles(void);
int UUPCOpenFiles(void);
int UUPCWriteXFile(CSpecHandle specList);
int UUPCWriteMailFile(void);
void UUPCGetNewline(UPtr newline, UPtr buffer);
/************************************************************************
 * UUPCPrime - get ready
 ************************************************************************/
OSErr UUPCPrime(UPtr server)
{
	UPtr bangs[6];		/* these will point to exclams in the server text */
	UPtr *bang;
	short err;
	short seq;

	/*
	 * count the bangs, shall we?
	 */
	WriteZero(bangs, sizeof(bangs));
	bangs[0] = server + 1;	/* first one */
	for (bang = bangs + 1; bang < bangs + sizeof(bangs) / sizeof(UPtr);
	     bang++) {
		*bang = strchr(bang[-1] + 1, '!');	/* look for next exclam */
		if (!*bang)
			break;
	}

	if (!bangs[3]) {
		WarnUser(UUPC_WRONG_SMTP, 0);
		return (1);
	}			/* not enough */
	if (!bangs[4]) {
		/* relay not included; insert a pretend relay */
		BMD(bangs + 1, bangs + 2, 3 * sizeof(UPtr));
	}
	bangs[5] = server + *server + 1;	/* end of the string */

	/*
	 * allocate some space
	 */
	if (!UUX)
		UUX = NewZH(UUXBlock);
	if (!UUX) {
		WarnUser(MEM_ERR, err = MemError());
		return (err);
	}

	/*
	 * copy values into it
	 */
#define MOVE(to,b1,b2) do{ \
	*to=MIN(sizeof(to)-2,bangs[b2]-bangs[b1]-1); to[*to+1]=0; \
	BMD(bangs[b1]+1,to+1,*to); } while (0)

	MOVE(MyMac, 0, 1);
	MOVE(SpoolPath, 2, 3);
	MOVE(UserName, 3, 4);
	seq = Atoi(bangs[4] + 1);
	Sequence = seq;

	CurTrans = UUPCTrans;

	return (noErr);
}

/************************************************************************
 * UUPCDry - put away the toys
 ************************************************************************/
OSErr UUPCDry(TransStream stream)
{
	if (UUX) {
		if (*UUX) {
			if (Sequence != OldSeq) {
				Str255 smtp;
				GetPref(smtp, PREF_SMTP);
				if (*smtp > 4) {
					UPtr cp = smtp + *smtp;
					short n = 4;
					while (n--) {
						*cp-- =
						    Sequence % 10 + '0';
						Sequence /= 10;
					}
				}
				SetPref(PREF_SMTP, smtp);
			}
		}
		ZapHandle(UUX);
	}
	CurTrans = GetTCPTrans();
	return (noErr);
}

/************************************************************************
 * UUPCSendMessage - the hard work goes here
 ************************************************************************/
int UUPCSendMessage(TOCHandle tocH, short sumNum, CSpecHandle specList)
{
	short err;
	MessHandle messH;

	/*
	 * grab the message
	 */
	if (!(messH = SaveB4Send(tocH, sumNum)))
		return (1);
	MessH = messH;

	/*
	 * generate some unique filenames in the spool directory
	 */
	do {
		UUPCGenFilenames();
	} while (UUPCBadFilenames());
	err = UUPCMakeFiles();
	if (!err) {
		err = UUPCOpenFiles();
		if (!err) {
			err = UUPCWriteXFile(specList);
			if (!err)
				err = UUPCWriteMailFile();
		}
	}

	UUPCCloseFiles();
	if (err)
		UUPCKillFiles();
	//else TimeStamp(tocH,sumNum,GMTDateTime(),ZoneSecs());
	return (err);
}

/************************************************************************
 * UUPCGenFilenames - make the filenames
 ************************************************************************/
void UUPCGenFilenames(void)
{
	Str15 seq;
	int n;
	Str255 scratch;
	Str255 mymacShort;

	PCopy(mymacShort, MyMac);
	*mymacShort = MIN(7, *mymacShort);
	MyLowerStr(mymacShort);

	n = ++Sequence;
	seq[0] = 4;
	seq[4] = n % 10 + '0';
	n /= 10;
	seq[3] = n % 10 + '0';
	n /= 10;
	seq[2] = n % 10 + '0';
	n /= 10;
	seq[1] = n % 10 + '0';
	ComposeRString(scratch, UUPC_DMYMAC, mymacShort, seq);
	PCopy(Dmymac, scratch);
	ComposeRString(scratch, UUPC_XMYMAC, mymacShort, seq);
	PCopy(Xmymac, scratch);
	PCopy(MailFile, SpoolPath);
	PCat(MailFile, Dmymac);
	PCopy(XFile, SpoolPath);
	PCat(XFile, Xmymac);
}

/************************************************************************
 * UUPCBadFilenames - check that our files don't already exist
 ************************************************************************/
Boolean UUPCBadFilenames(void)
{
	Str255 scratch;
	short i;
	CInfoPBRec hfi;

	for (i = 0; i < sizeof((*UUX)->refNs) / sizeof(short); i++) {
		PCopy(scratch, (*UUX)->files[i]);
		if (!AHGetFileInfo(0, 0, scratch, &hfi))
			return (True);
	}
	return (False);
}

/************************************************************************
 * UUPCMakeFiles - create our files
 ************************************************************************/
int UUPCMakeFiles(void)
{
	Str255 scratch;
	short i, err;
	long creator;

	GetPref(scratch, PREF_CREATOR);
	if (*scratch != 4)
		GetRString(scratch, TEXT_CREATOR);
	BMD(scratch + 1, &creator, 4);

	for (i = 0; i < sizeof((*UUX)->refNs) / sizeof(short); i++) {
		PCopy(scratch, (*UUX)->files[i]);
		if (err = HCreate(0, 0, scratch, creator, 'TEXT'))
			return (FileSystemError(TEXT_WRITE, scratch, err));
	}
	return (False);
}

/************************************************************************
 * UUPCCloseFiles - close our files
 ************************************************************************/
void UUPCCloseFiles(void)
{
	short i;
	for (i = 0; i < sizeof((*UUX)->refNs) / sizeof(short); i++)
		if ((*UUX)->refNs[i])
			(void) MyFSClose((*UUX)->refNs[i]);
}

/************************************************************************
 * UUPCKillFiles - destroy our files
 ************************************************************************/
void UUPCKillFiles(void)
{
	Str255 scratch;
	short i;
	for (i = 0; i < sizeof((*UUX)->refNs) / sizeof(short); i++) {
		PCopy(scratch, (*UUX)->files[i]);
		(void) HDelete(0, 0, scratch);
	}
	Sequence--;
}

/************************************************************************
 * UUPCOpenFiles - open our files
 ************************************************************************/
int UUPCOpenFiles(void)
{
	FSSpec spec;
	Str255 scratch;
	short i, err, refN;

	for (i = 0; i < sizeof((*UUX)->refNs) / sizeof(short); i++) {
		PCopy(scratch, (*UUX)->files[i]);
		FSMakeFSSpec(0, 0, scratch, &spec);
		if (err = FSpOpenDF(&spec, fsRdWrPerm, &refN))
			return (FileSystemError(TEXT_WRITE, scratch, err));
		(*UUX)->refNs[i] = refN;
	}
	return (0);
}

/************************************************************************
 * UUPCWriteXFile - write the file to send to the other side for execution
 ************************************************************************/
int UUPCWriteXFile(CSpecHandle specList)
{
	short err;
	Str255 buffer;
	Str15 newline;
	Accumulator newsGroupAcc;

	LDRef(UUX);

	ComposeRString(buffer, UUPC_U_CMD, UserName, MyMac);
	err = FSWriteP(XRefN, buffer);

	if (!err) {
		UUPCGetNewline(newline, buffer);

		ComposeRString(buffer, UUPC_F_CMD, Dmymac);
		err = FSWriteP(XRefN, buffer);

		if (!err) {
			ComposeRString(buffer, UUPC_I_CMD, Dmymac);
			err = FSWriteP(XRefN, buffer);

			if (!err) {
				UL(UUX);
				GetRString(buffer, UUPC_C_CMD);
				err = FSWriteP(XRefN, buffer);

				Zero(newsGroupAcc);
				if (!err)
					err =
					    DoRcptTos(NULL, MessH, False,
						      specList,
						      &newsGroupAcc);
				(*MessH)->newsGroupAcc = newsGroupAcc;
				if (!err)
					err = FSWriteP(XRefN, newline);
			}
		}
	}

	if (err)
		FileSystemError(TEXT_WRITE, XFile, err);
	UL(UUX);
	return (err);
}

/************************************************************************
 * UUPCWriteMailFile - get the message into the file
 ************************************************************************/
int UUPCWriteMailFile(void)
{
	MSumType sum;
	Str255 envelope;
	short len;
	Str15 oldNewLine;
	short err;

	PCopy(sum.from, UserName);
	*envelope = SumToFrom(&sum, envelope + 1);

	LDRef(UUX);
	ComposeRString(envelope + *envelope, UUPC_REMOTE, MyMac);
	UL(UUX);

	len = *envelope;
	*envelope += envelope[len];
	envelope[len] = ' ';
	if (err = FSWriteP(MailRefN, envelope)) {
		PCopy(envelope, MailFile);
		FileSystemError(TEXT_WRITE, envelope, err);
		return (err);
	}

	PCopy(oldNewLine, NewLine);
	UUPCGetNewline(NewLine, envelope);
	err = TransmitMessageHi(NULL, MessH, False, False);
	PCopy(NewLine, oldNewLine);
	return (err);
}

/**********************************************************************
 * WriteMailFile - write a fully-formatted piece of mail to a file
 **********************************************************************/
OSErr WriteMailFile(MessHandle messH, short refN, Boolean mime,
		    emsMIMEHandle * tlMIME)
{
	UUXHandle oldUUX = UUX;
	UUXHandle newUUX = NewZH(UUXBlock);
	TransVector oldTrans = CurTrans;
	OSErr err;
	Str15 oldNewLine;
	Boolean oldUUPCOut = UUPCOut;

	if (!newUUX)
		return (MemError());

	UUPCOut = True;
	UUX = newUUX;
	MailRefN = refN;
	MessH = messH;
	CurTrans = UUPCTrans;
	PCopy(oldNewLine, NewLine);
	if (!tlMIME)
		PCopy(NewLine, Cr);
	else
		PCopy(NewLine, CrLf);

	err =
	    TransmitMessage(NULL, MessH, False, True, !mime, tlMIME,
			    False);

	PCopy(NewLine, oldNewLine);
	UUX = oldUUX;
	ZapHandle(newUUX);
	CurTrans = oldTrans;
	UUPCOut = oldUUPCOut;
	return (err);
}

/************************************************************************
 * UUCPWriteAddr - write an address to the X file
 ************************************************************************/
int UUPCWriteAddr(UPtr addr)
{
	Str255 buffer;
	short err;

	if (!XRefN)
		return (noErr);
	*buffer = 1;
	buffer[1] = ' ';
	PCat(buffer, addr);
	if (err = FSWriteP(XRefN, buffer)) {
		PCopy(buffer, XFile);
		FileSystemError(TEXT_WRITE, buffer, err);
	}
	return (err ? 500 : 0);
}

/************************************************************************
 * UUPCGetNewline - what's this guy using for a newline?
 ************************************************************************/
void UUPCGetNewline(UPtr newline, UPtr buffer)
{
	UPtr bp;
	for (bp = buffer + *buffer; bp[-1] < ' '; bp--);
	*newline = (buffer + *buffer) - bp + 1;
	BMD(bp, newline + 1, *newline);
}

/************************************************************************
 * UUPCSendTrans - sendtrans to a file
 ************************************************************************/
OSErr UUPCSendTrans(TransStream stream, UPtr text, long size, ...)
{
	long bSize;
	short err;

	if (size == 0)
		return (noErr);	/* allow vacuous sends */
	if (MiniEvents())
		return (userCancelled);

	bSize = size;
	if (err = AWrite(MailRefN, &bSize, text))
		return (FileSystemError(TEXT_WRITE, MailFile, err));

	if (IsSendAudit(stream))
		stream->bytesTransferred += bSize;

	{
		Uptr buffer;
		va_list extra_buffers;
		va_start(extra_buffers, size);
		while (buffer = va_arg(extra_buffers, UPtr)) {
			bSize = va_arg(extra_buffers, int);
			if (err = UUPCSendTrans(nil, buffer, bSize, nil))
				break;
		}
		va_end(extra_buffers);
	}
	return (err);
}

/************************************************************************
 * GenSendWDS - send a lot of text to the remote host.
 ************************************************************************/
OSErr GenSendWDS(TransStream stream, wdsEntry * theWDS)
{
#pragma unused (stream)
	short err = 0;

	CycleBalls();
	for (; theWDS->length; theWDS++)
		if (err = SendTrans(nil, theWDS->ptr, theWDS->length, nil))
			break;

	return (err);
}
