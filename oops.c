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

#include "oops.h"
#define FILE_NUM 40
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * routines for undoing editing changes
 ************************************************************************/

#pragma segment Main

Boolean IsTransferUndo(void);
short IsTextUndo(MyWindowPtr win);
void TextUndo(void);
void TransferUndo(void);
static OSErr AddXfUndoSum(TOCHandle fromTocH, TOCHandle toTocH, short sumNum, Boolean open);
static OSErr TransferUndoBox(XfUndoPtr pxfu, short *pCount, uLong *pTicks);
OSErr IMAPTransferDeleteUndo(TOCHandle toTocH, TOCHandle fromTocH, XfUndoPtr pxfu);

/************************************************************************
 * DoUndo - undo the last undoable operation
 ************************************************************************/
void DoUndo(void)
{
	MyWindowPtr win = GetWindowMyWindowPtr(FrontWindow_());
	OSErr err;
	
#ifdef TWO
	if (IsTextUndo(win))
	{
		if (err=PeteEdit(win,win->pte,peeUndo,nil))
			WarnUser(PETE_ERR,err);
	}
	else if (IsTransferUndo()) TransferUndo();
#endif
}

#ifdef TWO
/************************************************************************
 * TransferUndo - undo the last xfer for a mailbox
 ************************************************************************/
static OSErr TransferUndoBox(XfUndoPtr pxfu, short *allCount, uLong *pTicks)
{
	XfUndo xfu = *pxfu;
	TOCHandle fromTocH,toTocH;
	short err=0;
	long sumNum;
	long origCount;
	short	count;
	
#ifdef NEVER
		if (RunType!=Production)
		{
			Str255 s;
			DebugStr(ComposeString(s,"\p%p %d %d %p;g",
			xfu.fromSpec.name,
			xfu.first,
			xfu.last,
			xfu.toSpec.name));
		}
#endif
	
	/*
	 * grab the .toc files
	 */
	fromTocH = TOCBySpec(&xfu.fromSpec);
	toTocH = TOCBySpec(&xfu.toSpec);
	if (!fromTocH || !toTocH) return 0;

	if ((*toTocH)->imapTOC)
		err = IMAPTransferDeleteUndo(toTocH, fromTocH, pxfu);
	else
	{
		origCount = (*fromTocH)->count;
		for (count = xfu.ids.offset/sizeof(uLong);!err && count;count--,(*allCount)--)
		{
			if (!(*allCount%10) || TickCount()-*pTicks>30)
			{
				Progress(NoBar,*allCount,nil,nil,nil);
				*pTicks = TickCount();
			}
			if (err=TOCFindMessByMID((*(uLong**)xfu.ids.data)[count-1],toTocH,&sumNum))
			{
				WarnUser(CANT_XF_UNDO,fnfErr);
				return err;
			}
			err = MoveMessageLo(toTocH,sumNum,&xfu.fromSpec,False,false,true);
		}
	}

	BoxFClose(toTocH,true);
	
	/*
	 * hey!  done.
	 */
	if (!err)
	{
		pxfu->fromSpec = xfu.toSpec;
		pxfu->toSpec = xfu.fromSpec;
		pxfu->redo = !xfu.redo;
#ifdef NEVER
		if (RunType!=Production)
		{
			Str255 s;
			DebugStr(ComposeString(s,"\p%p %d %d %p;g",
			pxfu->fromSpec.name,
			pxfu->first,
			pxfu->last,
			pxfu->toSpec.name));
		}
#endif
		(*fromTocH)->needRedo = 0;
		SelectBoxRange(fromTocH,origCount,(*fromTocH)->count-1,0,0,0);
		RedoTOC(fromTocH);
		BoxCenterSelection((*fromTocH)->win);
		if (pxfu->redo && pxfu->open)
			if (!TOCFindMessByMID((*(uLong**)xfu.ids.data)[0],fromTocH,&sumNum))
				GetAMessage(fromTocH,sumNum,nil,nil,True);
	}
	
	// IMAP - zap the undo.  It's invalid now because the affected messages are in transit again.
	if ((*toTocH)->imapTOC)
		NukeXfUndo();
	
	return err;
}

/************************************************************************
 * TransferUndo - undo the last xfer
 ************************************************************************/
void TransferUndo(void)
{
	short countXfers,countBoxes,i;
	uLong pTicks = TickCount();
	
	if (!XfUndoH) return;

	LDRef(XfUndoH);
		
	//	Count # of transfers
	countXfers = 0;
	countBoxes = HandleCount(XfUndoH);
	for(i=countBoxes;i--;)
		countXfers += (*XfUndoH)[i].ids.offset/sizeof(uLong);
	if (countXfers>10)
	{
		OpenProgress();
		ProgressMessageR(kpSubTitle,LEFT_TO_TRANSFER);
		Progress(NoBar,countXfers,nil,nil,nil);
	}
	
	for(i=countBoxes;i--;)
		if (TransferUndoBox(*XfUndoH+i, &countXfers, &pTicks))
		{
			NukeXfUndo();	/* oops */
			break;
		}
	
	CloseProgress();	
	UL(XfUndoH);
}

/************************************************************************
 * IMAPTransferDeleteUndo - undo deletes or transfers in IMAP mailboxes
 ************************************************************************/
OSErr IMAPTransferDeleteUndo(TOCHandle toTocH, TOCHandle fromTocH, XfUndoPtr pxfu)
{
	OSErr err = noErr;
	XfUndo xfu = *pxfu;	
	Handle uids = xfu.ids.data;
	short count, sumNum, numUids;
		
	if (xfu.delete)
	{
		// Undo a delete or transfer to trash
	
		// sanity check
		if (uids && *uids && toTocH && *toTocH && fromTocH && *fromTocH)
		{
			numUids = GetHandleSize(uids)/sizeof(unsigned long);
			if (numUids)
			{
				for (count=0;count<numUids;count++)
				{
					sumNum = UIDToSumNum(((unsigned long *)(*uids))[count], toTocH);
					if ((sumNum >= 0) && (sumNum < (*toTocH)->count))
					{
						// Undo delete.
						
						// first unmarkmark the original message as deleted.
						MarkSumAsDeleted(toTocH, sumNum, false);
						
						// move the message to the hidden toc
						HideShowSummary(toTocH, fromTocH, toTocH, sumNum); 	
						
						// de-queue the flag change
						sumNum = UIDToSumNum(((unsigned long *)(*uids))[count], fromTocH);
						QueueMessFlagChange(fromTocH, sumNum, (*fromTocH)->sums[sumNum].state, false);
					}
				}
			}
		}
	}
	else
	{
		// Undo transfer
		err = HandToHand(&uids);
		if (err == noErr)
			err = IMAPTransferMessages(toTocH, fromTocH, uids, false, false);
	}
	
	return (err);
}
#endif


#pragma segment Lib


#ifdef TWO
/************************************************************************
 * IsTransferUndo - shd we be thinking about undoing a transfer?
 ************************************************************************/
Boolean IsTransferUndo(void)
{
	MyWindowPtr win = GetWindowMyWindowPtr(FrontWindow_());
	return (!IsTextUndo(win) && XfUndoH);
}
#endif

/************************************************************************
 * SetUndoMenu - set the Undo menu properly
 ************************************************************************/
void SetUndoMenu(MyWindowPtr win)
{
	Str63 scratch;
	PETEUndoEnum textUndo = IsTextUndo(win);
	short redo;
	FSSpec spec;
	
#ifdef TWO
	if (IsTransferUndo())
	{
		Str31 name;
		if ((*XfUndoH)->delete)
		{
			spec = (*XfUndoH)->fromSpec;
			MailboxSpecAlias(&spec,name);
			ComposeRString(scratch,IMAP_UNDO_DELETE,name);
		}
		else if ((*XfUndoH)->redo)
		{
			spec = (*XfUndoH)->fromSpec;
			MailboxSpecAlias(&spec,name);
			ComposeRString(scratch,REDO_XFER,name);
		}
		else
		{
			spec = (*XfUndoH)->toSpec;
			MailboxSpecAlias(&spec,name);
			ComposeRString(scratch,UNDO_XFER,name);
		}
		EnableItem(GetMHandle(EDIT_MENU),EDIT_UNDO_ITEM);
	}
	else
#endif
	if (peCantUndo == textUndo)
	{
		DisableItem(GetMHandle(EDIT_MENU),EDIT_UNDO_ITEM);
		GetRString(scratch,UNDO);
	} 
	else
	{
		EnableItem(GetMHandle(EDIT_MENU),EDIT_UNDO_ITEM);
		if (textUndo<0) {textUndo = -textUndo; redo = 1;}
		else redo = 0;
		if (2*textUndo>redoLimit) textUndo=redoLimit/2;
		GetRString(scratch,UNDO_STRN+2*textUndo-1+redo);
	}
	ASSERT(*scratch!=0);
	SetMenuItemText(GetMHandle(EDIT_MENU),EDIT_UNDO_ITEM,scratch);
}

/**********************************************************************
 * 
 **********************************************************************/
short IsTextUndo(MyWindowPtr win)
{
	if (!IsMyWindow(GetMyWindowWindowPtr(win)) || !win->pte)
		return(peCantUndo);
	else
		return(PeteUndoType(win->pte));
}

#ifdef TWO

/************************************************************************
 * AddXfUndoSum - record transfer undo info for this summary
 ************************************************************************/
static OSErr AddXfUndoSum(TOCHandle fromTocH, TOCHandle toTocH, short sumNum, Boolean open)
{
	OSErr	err = noErr;
	short	count,i;
	Boolean	found;
	XfUndo	x;

#ifdef	IMAP
	// Problem - after an IMAP transfer, we lose track of the message.  Can't undo them.
	if ((fromTocH && (*fromTocH)->imapTOC) || (toTocH && (*toTocH)->imapTOC)) return (1);
#endif

	//	see if we have already saved this fromSpec
	Zero(x);
	x.fromSpec = GetMailboxSpec(fromTocH,sumNum);
	if (!XfUndoH)
	{
		if (!(XfUndoH = NuHTempOK(0)))
			return MemError();
	}
	count = HandleCount(XfUndoH);
	found = false;
	for(i=count;i--;)
	{
		if (SameSpec(&x.fromSpec,&(*XfUndoH)[i].fromSpec))
		{
			found = true;
			break;
		}
	}
	
	if (!found)
	{
		//	Add new mailbox entry
		x.toSpec = GetMailboxSpec(toTocH,-1);
		x.open = open;
		if (!(err=AccuInit(&x.ids)))
			err = PtrAndHand(&x,(Handle)XfUndoH,sizeof(x));
		i = count;
	}

	if (!err)
	{
			uLong id;

			EnsureMID(fromTocH,sumNum);
			id = (*fromTocH)->sums[sumNum].uidHash;
			LDRef(XfUndoH);
			if (!id || AccuAddPtr(&(*XfUndoH)[i].ids,(void*)&id,sizeof(id)))
				err = -1;
			UL(XfUndoH);
	}		

	return err;
}

/************************************************************************
 * AddXfUndo - record transfer undo info
 ************************************************************************/
void AddXfUndo(TOCHandle fromTocH, TOCHandle toTocH,short sumNum)
{
	OSErr	err;
	
#ifdef NEVER
	if (RunType!=Production)
	{
		Str255 s;
		DebugStr(ComposeString(s,"\p%s %s %d;g",(*fromTocH)->spec.name,
			(*toTocH)->spec.name,sumNum));
	}
#endif

	NukeXfUndo();
	if (sumNum>=0)
		err = AddXfUndoSum(fromTocH, toTocH, sumNum, true);
	else
	{
		for (sumNum=0;sumNum<(*fromTocH)->count;sumNum++)
		{
			if ((*fromTocH)->sums[sumNum].selected)
				if (err = AddXfUndoSum(fromTocH, toTocH, sumNum, false))
					break;
		}
	}

	if (err)
			NukeXfUndo();
	else
	{
		//	trim accumulators
		short	i;
		
		LDRef(XfUndoH);
		for(i=HandleCount(XfUndoH);i--;)
			AccuTrim(&(*XfUndoH)[i].ids);
		UL(XfUndoH);	
	}
}

/************************************************************************
 * AddIMAPXfUndoSum - record transfer undo info
 ************************************************************************/
OSErr AddIMAPXfUndoUIDs(TOCHandle fromTocH, TOCHandle toTocH, Handle uids, Boolean bDelete)
{
	OSErr	err = noErr;
	XfUndo	x;

	NukeXfUndo();
	if (!(XfUndoH = NuHTempOK(0)))
		return MemError();
	
	//	Add new mailbox entry
	Zero(x);
	x.toSpec = GetMailboxSpec(toTocH,-1);
	x.fromSpec = GetMailboxSpec(fromTocH,-1);
	x.open = false;
	x.delete = bDelete;
	if (!(err=AccuInit(&x.ids)))
		err = PtrAndHand(&x,(Handle)XfUndoH,sizeof(x));

	if (!err)
	{
		LDRef(XfUndoH);
		if (!uids || !*uids || AccuAddHandle(&(*XfUndoH)[0].ids,uids))
			err = -1;
		UL(XfUndoH);
	}		

	if (err)
			NukeXfUndo();
	else
	{
		//	trim accumulator
		LDRef(XfUndoH);
		AccuTrim(&(*XfUndoH)[0].ids);
		UL(XfUndoH);	
	}

	return err;
}

/************************************************************************
 * NukeXfUndo - can't undo xfer
 ************************************************************************/
void NukeXfUndo(void)
{
#ifdef NEVER
	if (RunType!=Production) DebugStr("\pNukeXFUndo;g");
#endif
	if (XfUndoH)
	{
		short	i;
		
		LDRef(XfUndoH);
		for(i=HandleCount(XfUndoH);i--;)
		 	AccuZap((*XfUndoH)[i].ids);
		ZapHandle(XfUndoH);
	}
}
#endif
