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

#include "search.h"
/* Copyright (c) 1995 by QUALCOMM Incorporated */
#define FILE_NUM 77

#pragma segment Search

long SearchPtrPtrSensitive(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen, Boolean words, AccuPtr cache);
long SearchPtrPtrInsensitive(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen, Boolean words, AccuPtr cache);
long ExactSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen);
long SearchFromCache(AccuPtr cache, long offset, long inLen);
long BruteSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen);
long RKSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen);

/**********************************************************************
 * SearchPtrHandle - look for a pointer in a handle
 **********************************************************************/
long SearchPtrHandle(UPtr lookPtr, short lookLen, UHandle textHandle, long offset, Boolean sensitive, Boolean words, AccuPtr cache)
{
	long result = SearchPtrPtr(lookPtr,lookLen,LDRef(textHandle),offset,GetHandleSize(textHandle),sensitive,words,cache);
	UL(textHandle);
	return(result);
}

/**********************************************************************
 * SearchPtrPtr - look for a pointer in another pointer
 **********************************************************************/
long SearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen, Boolean sensitive, Boolean words, AccuPtr cache)
{
	/*
	 * are we looking in the cache?
	 */
	if (cache && cache->data)
		return(SearchFromCache(cache,offset,inLen));
	
	/*
	 * no pre-loaded cache
	 */
	if (sensitive) return(SearchPtrPtrSensitive(lookPtr,lookLen,textPtr,offset,inLen,words,cache));
	else return(SearchPtrPtrInsensitive(lookPtr,lookLen,textPtr,offset,inLen,words,cache));
}

/**********************************************************************
 * SearchPtrPtrSensitive - case-sensitive search
 **********************************************************************/
long SearchPtrPtrSensitive(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen, Boolean words, AccuPtr cache)
{
	long found;
	
	while (0 <= (found=ExactSearchPtrPtr(lookPtr,lookLen,textPtr,offset,inLen)))
	{
		if (!words) break;
		if (!found || !IsWordChar[textPtr[found-1]])
			if (found+lookLen==inLen || !IsWordChar[textPtr[found+lookLen]])
				break;
		offset = found+1;
	}
	return(found);
}

/**********************************************************************
 * SearchPtrPtrInsensitive - case-insensitive search
 **********************************************************************/
long SearchPtrPtrInsensitive(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen, Boolean words, AccuPtr cache)
{
	Byte buffer[768];
	Str63 look;
	long bufferOffset;
	short bufferSize;
	long found;
	long foundAdjust;
	
	bufferOffset = offset;
	
	/*
	 * make search string uppercase
	 */
	MakePStr(look,lookPtr,lookLen);
	MyUpperStr(look);
	
	while (bufferOffset<inLen)
	{
		/*
		 * copy bytes into the buffer
		 */
		bufferSize = MIN(inLen-bufferOffset,sizeof(buffer));
		if (bufferSize < *look) break;
		BMD(textPtr+bufferOffset,buffer,bufferSize);
		
		/*
		 * make them upper case
		 */
		MyUpperText(buffer,bufferSize);

		/*
		 * find exact match
		 */
		found = 0;
		while (0 <= (found=ExactSearchPtrPtr(look+1,*look,buffer,found,bufferSize)))
		{
			foundAdjust = found+bufferOffset;
			if (!words) return(foundAdjust);	// don't care about words

			// at a word boundary?
			if (!found || !IsWordChar[textPtr[foundAdjust-1]])
				if (foundAdjust+*look==inLen || !IsWordChar[textPtr[foundAdjust+*look]])
					return(foundAdjust);
			
			// resume searching one byte beyond
			found++;
		}
		
		/*
		 * the string might span our buffer.  Be sure to recopy
		 * string-1 bytes just in case
		 */
		bufferOffset += bufferSize - (*look-1);
	}
	
	return(-1);
}

/**********************************************************************
 * ExactSearchPtrPtr - look for one precise set of bytes in another
 **********************************************************************/
long ExactSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen)
{
#ifdef NEVER
	if (lookLen>2 && inLen-offset>10)
		return(RKSearchPtrPtr(lookPtr,lookLen,textPtr,offset,inLen));
	else
#endif
		return(BruteSearchPtrPtr(lookPtr,lookLen,textPtr,offset,inLen));
}

/**********************************************************************
 * BruteSearchPtrPtr - do a search using exact matches and brute force
 **********************************************************************/
long BruteSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen)
{
	UPtr lookEnd = lookPtr+lookLen;
	UPtr textEnd = textPtr+inLen-lookLen+1;
	UPtr lookSpot;
	UPtr textSpot;
	UPtr textBase;
	
	for (textBase=textPtr+offset;textBase<textEnd;textBase++)
	{
		textSpot = textBase;
		for (lookSpot=lookPtr;lookSpot<lookEnd;lookSpot++)
			if (*lookSpot != *textSpot++) break;
		if (lookSpot==lookEnd) return(textBase-textPtr);
	}
	
	return(-1);
}

#define rkQ 33554393
#define rkD 32
/**********************************************************************
 * RKSearchPtrPtr - do a search using exact matches and Rabin-Karp
 **********************************************************************/
long RKSearchPtrPtr(UPtr lookPtr, short lookLen, UPtr textPtr, long offset, long inLen)
{
	long h1, h2, dM;
	UPtr end;
	short i;
	UPtr spot;
	
	dM = 1;
	for (i=1;i<lookLen;i++) dM = (dM*rkD)%rkQ;
	
	h1 = 0;
	end = lookPtr+lookLen;
	for (spot=lookPtr;spot<end;spot++) (h1*rkD+*spot)%rkQ;

	h2 = 0;
	end = textPtr+offset+lookLen;
	for (spot=textPtr+offset;spot<end;spot++) (h2*rkD+*spot)%rkQ;
	
	end = textPtr+offset+inLen-lookLen+1;
	for (spot=textPtr+offset;h1!=h2 && spot<end;spot++)
	{
		h2 = (h2+rkD*rkQ-*spot*dM)%rkQ;
		h2 = (h2*rkD+spot[inLen])%rkQ;
	}
	
	if (spot<end) return(spot-textPtr);
	else return(-1);
}

/**********************************************************************
 * SearchFromCache - find the next offset in a cache
 **********************************************************************/
long SearchFromCache(AccuPtr cache, long offset, long inLen)
{
	long *spot, *end;
	
	spot = (long*)*cache->data;
	end = spot + cache->offset/sizeof(long);
	while (spot<end)
		if (*spot>=offset) return(*spot);
		else spot++;
	
	return(-1);	// no more occurrences
}

typedef struct
{
	ParamBlockRec pb;
	long offset;
	long size;
	Boolean free;
	Boolean pending;
	Handle buffer;
	long bSize;
}	BulkSearchBuffer;

#define READY(b)	(!buf[b].pending && !buf[b].free)
#define kFoundIt ' fnd'

/**********************************************************************
 * BulkSearch - search a file, but do it fast
 *	string - the string to search for
 *	spec - the file to search in
 *	offset - pointer (may be nil)
 *						entry - start of search
 *						exit - will be filled to 1st occurrence
 *	allOffsets - accumulator (may be nil) that will get all hits
 **********************************************************************/
OSErr BulkSearch(PStr string,FSSpecPtr spec,long *offset,AccuPtr allOffsets)
{
	short refN;
	long spot, len;
	short i;
	BulkSearchBuffer buf[2];
	long tempOffset;
	long fullOffset = -1;
	OSErr err = fnfErr;
	FSSpec localSpec;
	
	/*
	 * allocate stuff
	 */
	Zero(buf[0]);
	Zero(buf[1]);
	buf[0].buffer = NewIOBHandle(1 K, 8 K);
	buf[1].buffer = NewIOBHandle(1 K, 8 K);
	buf[0].free = buf[1].free = True;
	if (offset) *offset = -1;
			
	if (buf[0].buffer && buf[1].buffer)
	{
		// record buffer sizes
		buf[0].bSize = GetHandleSize(buf[0].buffer);
		buf[1].bSize = GetHandleSize(buf[1].buffer);

		// open the file
		if (!(err=AFSpOpenDF(spec,&localSpec,fsRdPerm,&refN)))
		{
			// record initial state
			spot = offset ? *offset : 0;
			GetEOF(refN,&len);
			len -= spot;
			
			// search loop
			while ((!err && len>0) || !buf[0].free || !buf[1].free)
			{
				CycleBalls();
				if (CommandPeriod) err = userCanceledErr;
				
				/*
				 * fill the buffers if need be
				 */
				for (i=0;i<sizeof(buf)/sizeof(buf[0]);i++)
					if (len && !err && buf[i].free && len)
					{
						buf[i].offset = spot;
						buf[i].size = MIN(len,buf[i].bSize);
						spot += buf[i].size;
						len -= buf[i].size;
						
						if (SyncRW)
						{
							// boring synch stuff
							err = FSRead(refN,&buf[i].size,LDRef(buf[i].buffer));
							UL(buf[i].buffer);
							buf[i].pb.ioParam.ioResult = err;
							if (!err) buf[i].free = False;
						}
						else
						{
							// zippy asynch stuff
							buf[i].pb.ioParam.ioRefNum = refN;
							buf[i].pb.ioParam.ioBuffer = LDRef(buf[i].buffer);
							buf[i].pb.ioParam.ioReqCount = buf[i].size;
							buf[i].pb.ioParam.ioResult = inProgress;
							buf[i].pb.ioParam.ioPosMode = fsFromStart;
							buf[i].pb.ioParam.ioPosOffset = buf[i].offset;
							PBReadAsync(&buf[i].pb);
							buf[i].pending = True;
							buf[i].free = False;
						}
					}
				
				/*
				 * check to see if any i/o has completed
				 */
				for (i=0;i<sizeof(buf)/sizeof(buf[0]);i++)
					if (buf[i].pending && buf[i].pb.ioParam.ioResult!=inProgress)
					{
						if (!err) err = buf[i].pb.ioParam.ioResult;
						buf[i].pending = False;
						UL(buf[i].buffer);
					}
				
				/*
				 * do we have a buffer to search?
				 */
				i = -1;
				if (READY(0))
				{
					// buffer 0 is ready.  How about buffer 1?
					if (READY(1))
						// both ready.  take lesser
						i = (buf[0].offset < buf[1].offset) ? 0 : 1;
					else
						i = 0;	// just buffer 0 is ready
				}
				else if (READY(1)) i = 1;	// just buffer 1 is ready
				
				/*
				 * search
				 */
				if (i>=0)
				{
					if (err) buf[i].free = True;	// backing out
					else
					{
						tempOffset = 0;
						while (0<=(tempOffset=FindStrHandle(string,buf[i].buffer,tempOffset,nil)))
						{
							// string found
							fullOffset = tempOffset + buf[i].offset;
							if (allOffsets)
								err = AccuAddPtr(allOffsets,(void*)&fullOffset,sizeof(fullOffset));
							else
								err = kFoundIt;
							tempOffset++;
							if (err) break;
						}
						buf[i].free = True;
					}
				}
			}
		}
		FSClose(refN);
	}
	else err = MemError();
	
	/*
	 * dump the buffers
	 */
	ZapHandle(buf[0].buffer);
	ZapHandle(buf[1].buffer);
	
	/*
	 * ok, what have we done?
	 */
	// clear pseudo-error
	if (err==kFoundIt) err = noErr;

	// is the user interested in the first one?
	if (!err && offset)
		if (allOffsets) *offset = **(long**)allOffsets->data;
		else *offset = fullOffset;
	
	// all done
	return(err ? err : (fullOffset==-1 ? fnfErr : noErr));
}
