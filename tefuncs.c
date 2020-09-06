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

#include "tefuncs.h"
#define FILE_NUM 38
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment TEFuncs

	void UnwrapClip(void);
/**********************************************************************
 * ShowWinInsertion - show the insertion point
 **********************************************************************/
void ShowInsertion(MyWindowPtr win,short whichEnd)
{
	if (win->showInsert) (*win->showInsert)(win,whichEnd);
}

/************************************************************************
 * UnwrapClip - unwrap the text on the clipboard
 ************************************************************************/
void UnwrapClip(void)
{
	long len;
	long junk;
	Handle textH=NuHandle(0);
	
	len = GetScrap(nil,'TEXT',&junk);
	if (textH && 0<len && len<REAL_BIG)
	{
		len = GetScrap(textH,'TEXT',&junk);
		if (0<len)
		{
			void *oldSendTrans;
			Str31 oldNewLine;
			
			if (WrapHandle = NuHandle(0))
			{
				oldSendTrans = SendTrans;
				SendTrans = (void*)WrapSendTrans;
				PCopy(oldNewLine,NewLine);
				PCopy(NewLine,Cr);
				UnwrapSave(LDRef(textH),len,0,0);
				ZapHandle(textH);
				if (WrapHandle) 
				{
					ZeroScrap();
					PutScrap(GetHandleSize_(WrapHandle),'TEXT',LDRef(WrapHandle));
				}
				SendTrans = oldSendTrans;
				ZapHandle(WrapHandle);	
				PCopy(NewLine,oldNewLine);
			}
		}
		ZapHandle(textH);
	}
}

/************************************************************************
 * InsertCommaIfNeedBe - put a comma after previous address, if one is
 * not there
 ************************************************************************/
Boolean InsertCommaIfNeedBe(PETEHandle pte,HSPtr hsp)
{
	short cur = hsp ? hsp->index:CompHeadCurrent(pte);
	UHandle text;
	long spot;
	HeadSpec hs;
	long insertAt;
	
	if (cur!=TO_HEAD && cur!=CC_HEAD && cur!=BCC_HEAD) return(False);
	
	if (PeteGetTextAndSelection(pte,&text,&spot,nil)) return(False);

	if (hsp)
	{
		insertAt = spot = hsp->stop;
	}
	else if (IsMessWindow(GetMyWindowWindowPtr((*PeteExtra(pte))->win)))
	{
		CompHeadFind(Win2MessH((*PeteExtra(pte))->win),cur,&hs);
		hsp = &hs;
		insertAt = -1;
	}
	else
	{
		hsp = &hs;
		hsp->value = 0;
		insertAt = -1;
	}
	
	while (spot>hsp->value && (IsWhite((*text)[spot-1]) || (*text)[spot-1]=='\015')) spot--;
	if (spot>hsp->value && (*text)[spot-1]!=',')
	{
		PeteInsertPtr(pte,insertAt,", ",2);
		hsp->stop+=2;
	}

	NicknameWatcherModifiedField (pte); /* MJN */

	return(True);
}

/**********************************************************************
 * TextWrap - wrap some text into WrapHandle
 **********************************************************************/
OSErr TextWrap(Handle text,long start,long stop, Boolean unwrap)
{
	void *oldSendTrans, *oldAsync;
	Str31 oldNewLine;
	OSErr err = noErr;
	
	if (WrapHandle = NuHandle(0))
	{
		oldSendTrans = SendTrans;
		oldAsync = AsyncSendTrans;
		SendTrans = (void*)WrapSendTrans;
		AsyncSendTrans = nil;
		PCopy(oldNewLine,NewLine);
		PCopy(NewLine,Cr);
		if (!unwrap)
		{
			BufferSendRelease(NULL);
			SendBodyLines(NULL,text,stop,start,FLAG_WRAP_OUT,False,nil,0,False,nil);
			BufferSend(NULL,nil,nil,0,False);
			UL(text);
		}
		else
		{
			UnwrapSave(LDRef(text),stop,start,0);
			UL(text);
		}
		SendTrans = oldSendTrans;
		AsyncSendTrans = oldAsync;
		PCopy(NewLine,oldNewLine);
	}
	else
		err = MemError();
	return(err);
}

/************************************************************************
 * WrapSendTrans - accumulate "sent" text.
 ************************************************************************/
OSErr WrapSendTrans(TransStream stream, UPtr text,long size, ...)
{
	short err=0;
	UPtr buffer;
	long bSize;
	
	if (WrapHandle)
	{
		err = PtrPlusHand_(text,WrapHandle,size);
		if (!err)
		{
			va_list extra_buffers;
			va_start(extra_buffers,size);
			while (!err && (buffer = va_arg(extra_buffers,UPtr)))
			{
				CycleBalls();
				bSize = va_arg(extra_buffers,int);
				err = PtrPlusHand_(buffer,WrapHandle,bSize);
			}
			va_end(extra_buffers);
		}
	}
	if (err) ZapHandle(WrapHandle);
	return(err);
}