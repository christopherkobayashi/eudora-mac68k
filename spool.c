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

#define FILE_NUM 117

#include "spool.h"

OSErr GetOtherHeaders(MessHandle messH, AccuPtr a, UHandle *text, long *bodyOffset);
Boolean StringStartsHandle(PStr string, Handle h, long offset, long len);
OSErr SpoolOutTrans(TransStream stream, UPtr text,long size, ...);

static TransVector SpoolTrans = {nil,SpoolOutTrans,nil,nil,nil,nil,nil,GenSendWDS,nil,nil,nil};

static short spoolRefNum;
static StringPtr spoolFileName;

OSErr SpoolMessage(MessHandle messH, FSSpecPtr theSpec, short refN)
{
	FSSpec testSpec;
	Str255 scratch;
	OSErr err;
	OSType creator;
	Boolean newFile = false;
	TransVector	saveCurTrans = CurTrans;
	Handle table;
	long oldSpot;
		
	/* Test crap */
	if(theSpec == nil)
	{
		PCopy(scratch, "\pMacintosh HD:Desktop Folder:testspool");
		theSpec = &testSpec;
		err = FSMakeFSSpec(0, 0L, scratch, theSpec);
		if(err)
		{
			if(err != fnfErr)
				return(FileSystemError(TEXT_WRITE,scratch,err));
		}
		else
		{
			goto TryOpen;
		}
	}

	/* Try to create the bugger */
	GetPref(scratch,PREF_CREATOR);
	if (*scratch!=4) GetRString(scratch,TEXT_CREATOR);
	BMD(scratch+1,&creator,4);
	err = FSpCreate(theSpec, creator, 'TEXT', smRoman);
	if(err && (err != dupFNErr)) return(FileSystemError(TEXT_WRITE,theSpec->name,err));
	if(!err) newFile = true;

TryOpen :
	if (!refN)
		/* Open up the output file */
		err = FSpOpenDF(theSpec, fsRdWrPerm, &spoolRefNum);
	else
	{
		// position to the end
		err = SetFPos(refN,fsFromLEOF,0);
		GetFPos(refN,&oldSpot);
		spoolRefNum = refN;
	}
	
	if(err) goto DitchFile;
	
	/* File name for error reporting */
	spoolFileName = &theSpec->name;
	
	/* Set the trans routine */
	CurTrans = SpoolTrans;
	
	err = SetFPos(spoolRefNum, fsFromLEOF, 0);
	
	if (!PrefIsSet(PREF_NO_FLATTEN))
	{
		Flatten = GetFlatten();
	}

	if (!NewTables && !TransOut && (table=GetResource_('taBL',TransOutTablID())))
	{
		BMD(*table,scratch,256);
		TransOut = scratch;
	}
	if(!err) err = TransmitMessageForSpool(NULL, messH);

	ZapPtr(Flatten);
	TransOut = nil;
	
	/* Reset the trans routine */
	CurTrans = saveCurTrans;

	if (!refN) MyFSClose(spoolRefNum);
	
DitchFile :
	/* Ditch the file on error */
	if (err && refN) 
	{
		SetFPos(refN,fsFromStart,oldSpot);
		SetEOF(refN,oldSpot);
	}
	else if(err && newFile) 
		FSpDelete(theSpec);
	
	if(err) return(FileSystemError(TEXT_WRITE,theSpec->name,err));
	return err;
}

OSErr TransmitMessageForSpool(TransStream stream, MessHandle messH)
{
	OSErr err = noErr;
	Boolean newMess = false, topLevel;

	if(!(topLevel = MessFlagIsSet(messH,FLAG_OUT)) || (AllAttachOnBoardLo(messH, false) != noErr)) {
		
		long bodyOffset;
		UHandle text;
		Accumulator a = {0L,0L,nil};
		
		if(!topLevel) {
			if (MessFlagIsSet(messH,FLAG_RICH) || MessOptIsSet(messH,OPT_HTML) || MessOptIsSet(messH,OPT_FLOW))
			{
				SetMessFlag(messH,FLAG_WRAP_OUT);
			}

			err = GetOtherHeaders(messH, &a, &text, &bodyOffset);
		}
		
		if(!err)
		{
			MyWindowPtr newWin;
			SignedByte hState;
			long offset;
			
			if(!topLevel)
			{
				hState = HGetState(text);
				HNoPurge(text);
				offset = FindAnAttachment(text,bodyOffset,nil,true,nil,nil,nil);
				HSetState(text, hState);
			}
			
			if(topLevel || (offset >= 0))
			{
				newWin = DoSalvageMessageLo((*messH)->win,true,true);
				if(newWin == nil)
				{
					err = userCanceledErr;
				}
				else
				{
					messH = (MessHandle)GetMyWindowPrivateData(newWin);
					newMess = true;
				}
			}
		}
		
		if(!topLevel)
		{
			if(!err) err = SendExtras(stream, a.data, True, EffectiveTID(SumOf(messH)->tableId));
			if(!err) err = BufferSend(stream, nil, nil, 0, true);

			DisposeHandle(a.data);
			
			if(!err)
			{
				TransmitPB pb;
				
				Zero(pb);
				pb.stream = stream;
				err = TransmitMimeVersion(&pb);
			}
		}
	}
	
	if(!err)
	{
		SetMessRich(messH);
		err = TransmitMessageLo(stream, messH, False, True, True, nil, False, False, topLevel);
	}
	
	if(newMess) DeleteMessageLo((*messH)->tocH, (*messH)->sumNum, true);
	return err;
}

OSErr SpoolOutTrans(TransStream stream, UPtr text,long size, ...)
{
	long bSize;
	OSErr err;
	
	if (size==0) return(noErr); 	/* allow vacuous sends */
	if (MiniEvents()) return(userCancelled);
	
	bSize = size;
	if (err=AWrite(spoolRefNum,&bSize,text))
		return(FileSystemError(TEXT_WRITE,spoolFileName,err));
	{
		Uptr buffer;
		va_list extra_buffers;
		va_start(extra_buffers,size);
		while (buffer = va_arg(extra_buffers,UPtr))
		{
			bSize = va_arg(extra_buffers,int);
			if (err=SpoolOutTrans(nil,buffer,bSize,nil)) break;
		}
		va_end(extra_buffers);
	}
	return(err);
}

OSErr GetOtherHeaders(MessHandle messH, AccuPtr a, UHandle *text, long *bodyOffset)
{
	SignedByte hState;
	UHandle cache;
	long cacheOffset = 0, hLen;
	OSErr err;
	Str15 content, mimevers;
	
	GetRString(mimevers, InterestHeadStrn+hMimeVersion);
	GetRString(content, MIME_CONTENT_PREFIX);
	if(!mimevers[0] || !content[0]) return memFullErr;
	
	err = CacheMessage((**messH).tocH, (**messH).sumNum);
	if(err) return err;
	hLen = (*(**messH).tocH)->sums[(**messH).sumNum].length;
	cache = (*(**messH).tocH)->sums[(**messH).sumNum].cache;
	hState = HGetState(cache);
	HNoPurge(cache);
	
	/* Skip "From " line */		
	while(++cacheOffset < hLen && *(*cache + cacheOffset) != 13) ;
	++cacheOffset;

	while(!err && cacheOffset < hLen && *(*cache + cacheOffset) != 13)
	{
		long nextOffset = cacheOffset;

		do
		{
			while(++nextOffset < hLen && *(*cache + nextOffset) != 13) ;
		}
		while(++nextOffset < hLen && (*(*cache + nextOffset) == 32 || *(*cache + nextOffset) == 9));
		
		if(!StringStartsHandle(content, cache, cacheOffset, nextOffset - cacheOffset) &&
		   !StringStartsHandle(mimevers, cache, cacheOffset, nextOffset - cacheOffset))
		{
			err = AccuAddFromHandle(a, cache, cacheOffset, nextOffset - cacheOffset);
		}
		
		cacheOffset = nextOffset;
	}
	
	if(bodyOffset) *bodyOffset = cacheOffset;
	if(text) *text = cache;
	
	if(!err) AccuTrim(a);
	HSetState(cache, hState);
	return err;
}

Boolean StringStartsHandle(PStr string, Handle h, long offset, long len)
{
	Byte i;
	
	if(len<0) len = InlineGetHandleSize(h);

	if(*string > len) return false;
	
	for(i = *string; i > 0; --i)
	{
		Byte sb = string[i];
		Byte hb = (*h + offset)[i - 1];
		
		if(sb != hb) {
			if(isupper(sb)) sb = tolower(sb);
			if(isupper(hb)) hb = tolower(hb);
			if(sb != hb) break;
		}
	}
	return(!i);
}
