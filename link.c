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

#include "link.h"
#ifdef NEVER

/* Copyright (c) 1993 by QUALCOMM Incorporated */
#pragma segment Links
#define FILE_NUM 57

OSErr GleanLinkInfo(TOCHandle *tocH,short *sumNum,UPtr buffer,long bSize);
Handle GrabHexHead(UPtr buffer,long bSize,short hId);

/************************************************************************
 * OpenLink - open a link to a message
 ************************************************************************/
MyWindowPtr OpenLink(TOCHandle tocH,short sumNum,WindowPtr theWindow, MyWindowPtr win,Boolean showIt)
{
	UPtr buffer = NuPtr(GetMessageLength(tocH,sumNum));

	if (!buffer) {WarnUser(NO_MESS_BUF,MemError()); return(nil);}
	if (!ReadMessage(tocH,sumNum,buffer))
	{
		if (!GleanLinkInfo(&tocH,&sumNum,buffer,(*tocH)->sums[sumNum].length))
		{
			DisposePtr(buffer); buffer = nil;
			if (IsALink(tocH,sumNum)) return(OpenLink(tocH,sumNum,theWindow,win,showIt));
			else return(GetAMessage(tocH,sumNum,theWindow,win,showIt));
		}
		else WarnUser(BROKEN_LINK,0);
	}
	if (buffer) DisposePtr(buffer);
}

/************************************************************************
 * GleanLinkInfo - extract the linked-to data
 ************************************************************************/
short GleanLinkInfo(TOCHandle *tocH,short *sumNum,UPtr buffer,long bSize)
{
	Handle tocAliasH;
	Handle msgIdH;
	FSSpec spec, rootSpec;
	long msgId;
	short err = 1;
	Boolean junk;
	TOCHandle localTocH;
	short mNum;
	
	tocAliasH = GrabHexHead(buffer,bSize,TOC_ALIAS_HEAD);
	msgIdH = GrabHexHead(buffer,bSize,XLINK_HEAD);
	
	if (tocAliasH && msgIdH)
	{
		msgId = **msgIdH;
		FSMakeFSSpec(Root.vRef,Root.dirId,"",&rootSpec);
		if (!(err=ResolveAlias(&rootSpec,tocAliasH,&spec,&junk)))
		{
			if (!(err = GetTOCByFSS(&spec,&localTocH)))
			{
				for (mNum=0;mNum<(*localTocH)->count;mNum++)
					if ((*localTocH)->sums[mNum].msgId == msgId) break;
				if (mNum<(*localTocH)->count)
				{
					*tocH = localTocH;
					*sumNum = mNum;
					err = 0;
				}
				else err = fnfErr;
			}
		}
	}
	ZapHandle(tocAliasH);
	ZapHandle(msgIdH);
	return(err);
}

/************************************************************************
 * GrabHexHead - grab the value of a hexadecimally encoded header
 ************************************************************************/
Handle GrabHexHead(UPtr buffer,long bSize,short hId)
{
	Str63 head;
	Handle data;
	UPtr spot;
	
	if (spot = FindHeaderString(buffer,GetRString(head,hId),&bSize))
	{
		if (!(data = NuHandle(bSize/2))) WarnUser(MEM_ERR,MemError());
		else
		{
			Hex2Bytes(spot,bSize,LDRef(data));
			UL(data);
		}
	}
	return(data);
}
#endif