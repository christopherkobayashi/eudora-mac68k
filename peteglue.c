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

#include <Folders.h>

#include <conf.h>
#include <mydefs.h>

#include "peteglue.h"
#include "pete.h"
#define FILE_NUM 48
/* Copyright (c) 1995 by Qualcomm, Inc. */
/* written by Steven Dorner */

#pragma segment PGLUE

OSErr PeteDragTrack(PETEInst pi,PETEHandle ph,DragTrackingMessage message,DragReference theDragRef);
Byte PeteLockedAt(PETEHandle pte,long offset);
OSErr AppendContextSearch(PETEHandle pte,MenuHandle contextMenu);

#define ATTRIBUTION_SCRAP_TYPE 'EuAt'

/**********************************************************************
 * PeteEdit - wrapper for PETEEdit, that also does URL's and dirty bits
 **********************************************************************/
OSErr PeteEdit(MyWindowPtr win,PETEHandle ph,PETEEditEnum what,EventRecord* event)
{
	OSErr err;
	long spot;
	long dirtyWas;
	Point mouse;
	Boolean launch;
	PETEDocInfo info;
	Str255 attribution;
	
	if (!ph && win) ph = win->pte;
	
	if (!ph) return(fnfErr);

	if (what==peeEvent && event->what==keyDown)
	{
		if ((*PeteExtra(ph))->not[event->message&charCodeMask])
			return(errAEEventNotHandled);
	}

	PeteGetTextAndSelection(ph,nil,&spot,nil);
	
	// If copying, make an attribution just in case
	if (what==peeCut || what==peeCopy)
	{
		GrabAttribution(ATTR_PASTE_AS_QUOTE,win,attribution);
		if (*attribution) PCatC(attribution,'\015');
	}
	else *attribution = 0;
	
	dirtyWas = PeteIsDirty(ph);
		
	MightSwitch();
	DragFxxkOff = False;
	err = PETEEdit(PETE,ph,what,event);
	DragFxxkOff = True;
	AfterSwitch();
	if (err) return(err);
	
	// stick the attribution on the clipboard
	if (*attribution) PutScrap(*attribution,ATTRIBUTION_SCRAP_TYPE,attribution+1);
	
	launch = (win && what==peeEvent && event->what==mouseDown &&
			(ClickType==Double || (event->modifiers & cmdKey)));
	if (launch && ClickType==Double)
	{
		GetMouse(&mouse);
		LocalToGlobal(&mouse);
		if (ABS(mouse.h-event->where.h)>3 || ABS(mouse.v-event->where.v)>3)
			launch = False;
	}
	
	if (launch && ClickType==Double)
	{
		PETEGetDocInfo(PETE,ph,&info);
		if (!info.doubleClick) launch = False;
	}
	
	if (launch)
	{
		mouse = event->where;
		GlobalToLocal(&mouse);
		if (!PtInPETEView(mouse,ph)) launch = False;
	}
	
	if (launch)
	{
		if (!AttIsSelected(win,ph,-1,-1,attAll+(MainEvent.modifiers&controlKey?attFinder:0),nil,nil))
			if (urlNot==URLIsSelected(win,ph,-1,-1,urlOpen|urlSelect,nil,nil))
				if (ClickType==Single) SlackURL(win,ph,-1,-1,urlAll,nil,nil);
	}
	
	if (dirtyWas!=PeteIsDirty(ph)) PeteSetURLRescan(ph,spot);
	
	PeteSetDirty(ph);
	return(noErr);
}

/**********************************************************************
 * PeteKey - handle key
 **********************************************************************/
OSErr PeteKey(MyWindowPtr win,PETEHandle pte,EventRecord* event)
{
	OSErr	err;
	
	if ((*PeteExtra(pte))->numberField)
	{
		//	Digits only
		short charCode = event->message&charCodeMask;
		
		if ((charCode < '0' || charCode > '9') &&
			DirtyKey(event->message) &&
			charCode != delChar &&
			charCode != backSpace)
		{
			//	Not a valid character
			SysBeep(10);
			return errAEEventNotHandled;
		}
	}
	
	if (err=PeteEdit(win,pte,peeEvent,event))
		if (err==errAENotModifiable)
		{
			if (PETESelectionLocked(PETE,pte,-1,-1))
				WarnUser(READ_ONLY,err);
		}
		else if (err==errAEEventNotHandled)
			SysBeep(20L);
		else
			WarnUser(PETE_ERR,err);
	
	return err;
}

/**********************************************************************
 * PeteSelectAll - select "all" in a Pete region
 **********************************************************************/
void PeteSelectAll(MyWindowPtr win, PETEHandle pte)
{
	long selStart, selStop;
	PETEStyleEntry pse;
	long offset, lastOffset;
	
	// Find the current selection
	PeteGetTextAndSelection(pte,nil,&selStart,&selStop);
	
	// Find the end
	
	// check for clickBefore
	if (!PeteStyleAt(pte,selStop,&pse) && pse.psStyle.textStyle.tsLock & peClickBeforeLock)
		; // clickBefore; stick with selStop
	else
	{
		// Find the locked text that follows the selection
		pse.psStyle.textStyle.tsLock = peSelectLock;
		if (PETEFindStyle(PETE,pte,selStop,&selStop,&pse.psStyle,peSelectLock<<kPETELockShift))
			selStop = PeteLen(pte);	// no locked text; use end of document
	}
	
	// Now find the beginning.
	
	// check for clickAfter
	if (!PeteStyleAt(pte,selStart-1,&pse) && pse.psStyle.textStyle.tsLock & peClickAfterLock)
		lastOffset = selStart;	// clickAfter; stick with selStart
	else
	{
		// This is less fun.  Find the last locked text BEFORE the selection
		lastOffset = offset = 0;
		pse.psStyle.textStyle.tsLock = peSelectLock;
		while (!PETEFindStyle(PETE,pte,offset,&offset,&pse.psStyle,peSelectLock<<kPETELockShift))
		{
			// have we gone past?
			if (offset>=selStart) break;
			
			// haven't gone past.  Now find next UNlocked text
			pse.psStyle.textStyle.tsLock = 0;
			if (PETEFindStyle(PETE,pte,offset,&lastOffset,&pse.psStyle,peSelectLock<<kPETELockShift))
				break;	// no unlocked text
			
			// look for more locked text
			pse.psStyle.textStyle.tsLock = peSelectLock;
			offset = lastOffset;
		}
	}
	
	PeteSelect(win,pte,lastOffset,selStop);
}

/**********************************************************************
 * PeteDidResize - resize a PETE editor thing 
 **********************************************************************/
void PeteDidResize(PETEHandle pte,Rect *view)
{
	PETEDocInfo info;
	Rect newR;
	Boolean oldFrame;
	
	PETEGetDocInfo(PETE,pte,&info);
	if (EqualRect(view,&info.docRect)) return;
	
	// invalidate old spot
	if (oldFrame = (*PeteExtra(pte))->frame)
	{
		InsetRect(&info.docRect,-2*MAX_APPEAR_RIM,-2*MAX_APPEAR_RIM);
		newR = *view;
		InsetRect(&newR,-2*MAX_APPEAR_RIM,-2*MAX_APPEAR_RIM);
		(*PeteExtra(pte))->frame = false;
	}
	
	// move to new spot
	PETEMoveDoc(PETE,pte,view->left,view->top);
	PETESizeDoc(PETE,pte,RectWi(*view),RectHi(*view));
	PETEChangeDocWidth(PETE,pte,(*PeteExtra(pte))->infinitelyWide ? REAL_BIG:-1,True);
	
	if ((*PeteExtra(pte))->frame = oldFrame)
	{
		MyWindowPtr win = (*PeteExtra(pte))->win;
		WindowPtr	winWP = GetMyWindowWindowPtr(win);
		InvalWindowRect(winWP,&info.docRect);
		InvalWindowRect(winWP,&newR);
	}
}

/**********************************************************************
 * PeteFindString - find a string in a pete
 **********************************************************************/
long PeteFindString(PStr string,long start,PETEHandle pte)
{
	long length;
	Handle text;
	
	PeteGetRawText(pte,&text);
	length = GetHandleSize(text);
	
	if (start + *string > length) return(-1);
	
	return(FindStrHandle(string,text,start,nil));
}

/**********************************************************************
 * PetePlain - make text plain
 **********************************************************************/
OSErr PetePlain(PETEHandle pte,long start,long stop,long valid)
{
	PETEStyleEntry pse;
	
	Zero(pse);
	PETEGetStyle(PETE,pte,kPETEDefaultStyle,nil,&pse);
	return(PETESetTextStyle(PETE,pte,start,stop,&pse.psStyle.textStyle,valid));
}

/**********************************************************************
 * PeteNoLabel - unlabel text
 **********************************************************************/
OSErr PeteNoLabel(PETEHandle pte,long start,long stop,long labelMask)
{
	return PetePlain(pte,start,stop,labelMask<<kPETELabelShift);
}

/**********************************************************************
 * PetePlainParaAt - make some text plain in paragraph attributes
 **********************************************************************/
OSErr PetePlainParaAtLo(PETEHandle pte,long start,long stop,long valid)
{
	PETEParaInfo pinfo;
	long fromPara, toPara;
	
	Zero(pinfo);
	PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&pinfo);
	PeteEnsureBreak(pte,start);
	PeteEnsureBreak(pte,stop);
	PETEGetParaIndex(PETE,pte,start,&fromPara);
	PETEGetParaIndex(PETE,pte,stop-1,&toPara);
	do
	{
		PETESetParaInfo(PETE,pte,fromPara++,&pinfo,valid);
	} while (fromPara<=toPara);
	return noErr; /* as good an error as any! */
}

/**********************************************************************
 * PetePlainPara - set a particular pargraph to plain
 **********************************************************************/
OSErr PetePlainParaLo(PETEHandle pte,long index,long valid)
{
	PETEParaInfo pinfo;
	OSErr err;
	
	Zero(pinfo);
	if (!(err=PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&pinfo)))
		err = PETESetParaInfo(PETE,pte,index,&pinfo,valid);
	return(err);
}


/************************************************************************
 * PeteLink - Link a PTE onto a list
 ************************************************************************/
PETEHandle PeteLink(PETEHandle head,PETEHandle pte)
{
	PETEHandle tail=head;
	
	if (!tail) head = pte;
	else
	{
		while (PeteNext(tail)) tail = PeteNext(tail);
		(*PeteExtra(tail))->next = pte;
	}
	return(head);
}

/**********************************************************************
 * PetePrevious - get the previous pte in a list
 **********************************************************************/
PETEHandle PetePrevious(PETEHandle head,PETEHandle pte)
{
	if (!PeteIsValid(pte)) return(nil);
	if (pte==head)
	{
		while (PeteNext(pte)) pte=PeteNext(pte);
		return(pte);
	}
	else
	{
		while (head && PeteNext(head)!=pte) head = PeteNext(head);
		return(head);
	}
}

/**********************************************************************
 * PeteApplyResStyle - apply a style from a resource
 **********************************************************************/
OSErr PeteApplyResStyle(PETEHandle pte,long start,long stop,short styleId)
{
	PETETextStyle style;
	
	GetRColor(&style.tsColor,styleId);
	return PETESetTextStyle(PETE,pte,start,stop,&style,peColorValid);
}

/**********************************************************************
 * PeteRemove - unlink a PETEHandle from a list
 **********************************************************************/
PETEHandle PeteRemove(PETEHandle head,PETEHandle pte)
{
	PETEHandle prev, next;
	
	if (pte==head)
		head = PeteNext(pte);
	else
	{
		prev = PetePrevious(head,pte);
		if (prev)
		{
			next = PeteNext(pte);
			(*PeteExtra(prev))->next = next;
		}
	}
	
	(*PeteExtra(pte))->next = nil;
	return(head);
}

/**********************************************************************
 * 
 **********************************************************************/
void PeteFocusPrevious(MyWindowPtr win)
{
	PeteFocus(win,PetePrevious(win->pteList,win->pte),True);
}

/**********************************************************************
 * 
 **********************************************************************/
void PeteFocusNext(MyWindowPtr win)
{
	PeteFocus(win,PeteNext(win->pte)?PeteNext(win->pte):win->pteList,True);
}

/**********************************************************************
 * PeteCreate - create a pete record, but with extra stuff
 **********************************************************************/
OSErr PeteCreate(MyWindowPtr win,PETEHandle *ph,uLong flags,PETEDocInitInfoPtr initInfo)
{
	PeteExtraHandle x = NewZH(PeteExtra);
	OSErr err;
	PETEDocInitInfo info;
	static long id;
	
	*ph = nil;
	
	if (!x) return(MemError());

	Zero(info);
	if (!initInfo)
	{
		initInfo = &info;
		DefaultPII(win,False,flags,initInfo);
	}

	(*x)->win = win;
	(*x)->headers = 0x7fffffff;
	(*x)->id = id++;
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking))
		(*x)->spelled = sprNeverSpell;	// speller off for most windows
#endif
	
	if (!HaveOSX())	// No offscreen buffering necessary for OS X
#ifdef DEBUG
		if (!BUG0)
#endif
			initInfo->docFlags |= peUseOffscreen;

#ifdef DEBUG
	if (BUG14) initInfo->docFlags |= peDrawDebugSymbols;
#endif

	err = PETECreate(PETE,ph,initInfo);
	
	if (err)
	{
		ZapHandle(x);
		*ph = nil;	// just making sure
	}
	else
	{
		PETESetRefCon(PETE,*ph,(long)x);
		win->pteList = PeteLink(win->pteList,*ph);
		PETESetCallback(PETE,*ph,(void*)PeteBusy,peProgressLoop);
	}
	
	CleanPII(&info);
	
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
void CleanPII(PETEDocInitInfoPtr pii)
{
	ZapHandle(pii->inParaInfo.tabHandle);
}

/**********************************************************************
 * 
 **********************************************************************/
void PeteDispose(MyWindowPtr win,PETEHandle ph)
{
	long extra = PETEGetRefCon(PETE,ph);
	StackHandle stack = (*(PeteExtraHandle)extra)->partStack;

	if (!ph) return;
	if (win->pte==ph) PeteFocus(win,nil,True);
	if (win) win->pteList = PeteRemove(win->pteList,ph);
	ZapHandle ((*(PeteExtraHandle)extra)->nick.addresses);
	PETEDispose(PETE,ph);
	if (stack) ZapHandle(stack);
	DisposeHandle((void*)extra);
}

/**********************************************************************
 * MPETEAppendText - append some text to a te region
 **********************************************************************/
OSErr PeteAppendText(UPtr text,long size,PETEHandle pte)
{
	OSErr err = noErr;
	
	err = PETEInsertTextPtr(PETE,pte,0x7fffffff,text,size,nil);
	return err;
}

/**********************************************************************
 * PeteFocus - focus a window on a PTE
 **********************************************************************/
void PeteFocus(MyWindowPtr win,PETEHandle pte, Boolean focus)
{
	Boolean activeState = !InBG && focus && (win->isActive || IsFloating(GetMyWindowWindowPtr(win)));
	
	if (pte && HasNickScanCapability (pte))
		NicknameWatcherFocusChange (pte);
	
	if (focus)
	{
		if (win->pte && pte!=win->pte)
		{
			if ((*PeteExtra(win->pte))->frame)
				PeteHotRect(win->pte,False);
			PeteActivate(win->pte,False,True);
		}
		win->pte = pte;
		if (IsFloating(GetMyWindowWindowPtr(win))) SetKeyFocusedFloater(pte?win:nil);
	}
	if (pte)
	{
		if ((*PeteExtra(pte))->frame)
				PeteHotRect(pte,activeState);
		PeteActivate(pte,activeState,true);
	}
	win->hasSelection = MyWinHasSelection(win);
}

/**********************************************************************
 * PeteActivateLo - activate a PETE handle, but allow one to be locked
 **********************************************************************/
void PeteActivateLo(PETEHandle pte, Boolean doc, Boolean scroll,Boolean lock)
{
	static PETEHandle lockedPTE;
	
	if (lock)
	{
		lockedPTE = pte;
		// manage the activation of the non-floater pte, if any
		{
			WindowPtr winWP = MyFrontNonFloatingWindow();
			if (winWP)
			{
				MyWindowPtr win = GetWindowMyWindowPtr(winWP);
				if (win && win->pte && win->pte != pte)
					PETEActivate(PETE,win->pte,pte==nil,true);
			}
		}
		return;
	}
	
	if (lockedPTE && pte!=lockedPTE) doc = false;
	PETEActivate(PETE,pte,doc,scroll);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteDrag(MyWindowPtr win,DragTrackingMessage message,DragReference drag)
{
	OSErr dragErr;
	static PETEHandle oldPTE;
	Point mouse;
	PETEHandle pte;

	PushGWorld();
	
	if (win)
	{
		SetPort_(GetMyWindowCGrafPtr(win));
		GetDragMouse(drag,&mouse,nil);
		GlobalToLocal(&mouse);
		PopGWorld();
		for (pte=win->pteList;pte;pte=PeteNext(pte))
			if (PtInPETE(mouse,pte)) break;
	}
	else
	{
		PopGWorld();
		pte = nil;
	}
	
	if (oldPTE!=pte)
	{
		//Dprintf("\p%x %x;g",pte,oldPTE);
		if (PeteIsValid(oldPTE))// && message!=kDragTrackingLeaveWindow)
			PeteDragTrack(PETE,oldPTE,kDragTrackingLeaveWindow,drag);
		if (pte && message!=kDragTrackingEnterWindow)
			PeteDragTrack(PETE,pte,kDragTrackingEnterWindow,drag);
		oldPTE = pte;
	}
	
	if (pte)
	{
		if (message == 0xfff)
		/* MJN *//* formatting toolbar */
/*		return(PETEDragReceiveHandler(PETE,pte,drag)); */
		{

			// preprocess
			if ((*PeteExtra(pte))->dragPreProcess)
				(*PeteExtra(pte))->dragPreProcess(pte,message,drag);
	
			dragErr = PETEDragReceiveHandler(PETE,pte,drag);
			
			// postprocess
			if ((*PeteExtra(pte))->dragPostProcess)
				(*PeteExtra(pte))->dragPostProcess(pte,message,drag);
			
			RequestTFBEnableCheck(win);
			return(dragErr);
		}
		else
		{
#ifdef NEVER
			if (message==dragTrackingLeaveWindow) oldPTE = nil;	//we're leaving, see that we don't
																													//leave twice
#endif
			return(PeteDragTrack(PETE,pte,message,drag));
		}
	}
	return(dragNotAcceptedErr);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteDragTrack(PETEInst pi,PETEHandle ph,DragTrackingMessage message,DragReference theDragRef)
{
	OSErr err;
#ifdef NEVER
	ComposeLogS(-1,nil,"\p%x %x",message,ph);
#endif

	// preprocess
	if ((*PeteExtra(ph))->dragPreProcess)
		(*PeteExtra(ph))->dragPreProcess(ph,message,theDragRef);
	
	// process
	err = PETEDragTrackingHandler(pi,ph,message,theDragRef);
	
	// postprocess
	if ((*PeteExtra(ph))->dragPostProcess)
		(*PeteExtra(ph))->dragPostProcess(ph,message,theDragRef);
	
	return err;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteWrap(MyWindowPtr win, PETEHandle pte, Boolean unwrap)
{
	Handle text;
	OSErr err;
	long start,stop;
	long len;

	PeteCalcOff(pte);
	start = stop = -1;
	PeteParaRange(pte,&start,&stop);
	PeteParaConvert(pte,start,stop);
	PeteSelect(win,pte,start,stop);
	PetePrepareUndo(pte,peUndoPaste,-1,-1,nil,nil);

	if (!unwrap)
	{
		ConvertExcerpt(pte,start,stop,&start,&stop);
		PeteSelect(win,pte,start,stop);
	}
		
	if (err=PeteGetTextAndSelection(pte,&text,nil,nil)) return(err);
	
	if (!(err=TextWrap(text,start,stop,unwrap)))
	{
		len = GetHandleSize(WrapHandle);
		
		// make sure last char is CR
		if (unwrap && (!len || (*WrapHandle)[len-1]!='\015'))
		{
			SetHandleBig(WrapHandle,len+1);
			if (!MemError()) (*WrapHandle)[len++] = '\015';
		}

		stop = len+start;
		
		err = PETEInsertText(PETE,pte,-1,WrapHandle,nil);
		
		if (err)
		{
			ZapHandle(WrapHandle);
			PeteKillUndo(pte);
		}
		else
		{
			WrapHandle = nil;
			PetePlain(pte,start,stop,peAllValid);
			PetePlainParaAtLo(pte,start,stop,peAllParaValidButTabs);
			PeteFinishUndo(pte,peUndoPaste,start,stop);
			PeteSelect(win,pte,start,stop);
		}
		PeteSetDirty(pte);
	}
	else PeteKillUndo(pte);
	
	if (err) WarnUser(PETE_ERR,err);
	PeteCalcOn(pte);
	return(err);
}

/**********************************************************************
 * PETESetDirty - set the dirty bit for a window, and return whether or not
 *  an peteedit thing is dirty.
 **********************************************************************/
Boolean PeteSetDirty(PETEHandle pte)
{
	PETEDocInfo info;
	MyWindowPtr win;
	
	if (PeteIsValid(pte))
	{
		win = (*PeteExtra(pte))->win;
		if (PETEGetDocInfo(PETE,pte,&info)) return(False);
		win->isDirty = win->isDirty || info.dirty;
		if (win->pte == pte) win->hasSelection = info.selStop!=info.selStart;
		return(info.dirty);
	}
	return(False);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteSetURLRescan(PETEHandle pte,long spot)
{
	PETEParaInfo pInfo;
	
	if (NoScannerResets) return(noErr);
	spot = MAX(0,spot-256);

#define SET_SCANNER(scanner,spot) do {if (scanner<0 || scanner>spot) scanner = spot;}while(0)

	// the URL scanner
	SET_SCANNER((*PeteExtra(pte))->urlScanned,spot);

	// emoticons
	SET_SCANNER((*PeteExtra(pte))->emoScanned,spot);

	// the speller
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking) && !SpellDisabled(pte))
	{
		SET_SCANNER((*PeteExtra(pte))->spelled,spot);
		(*PeteExtra(pte))->spellReset = true;
	}
#endif

	// for quote scanner, be sure to go back to start of last para
	Zero(pInfo);
	PETEGetParaInfo(PETE,pte,PeteParaAt(pte,spot),&pInfo);
	spot = MIN(spot,pInfo.paraOffset?pInfo.paraOffset-1:0);
	SET_SCANNER((*PeteExtra(pte))->quoteScanned,spot);
	
	// ouch; for tae, back all the way out
	(*PeteExtra(pte))->taeScanned = 0;
	return noErr;	/* ??? */
}


//
//	PeteNickScan
//
//		Performs nickname scanning operations
//

void PeteNickScan (PETEHandle pte)

{
	if ((*PeteExtra(pte))->nick.fieldCheck ? (*(*PeteExtra(pte))->nick.fieldCheck) (pte) : HasNickScanCapability (pte))
		NicknameHilitingUpdateField (pte);
}

/**********************************************************************
 * HorizRuleGraphic - Callback to handle a graphic that is an horizontal rule
 **********************************************************************/
pascal OSErr HorizRuleGraphic(PETEHandle pte,PETEGraphicInfoHandle graphic,long offset,PETEGraphicMessage message,void *data)
{
	OSErr err = noErr;
	Rect r;
	Rect rule;
	HRGraphicHandle hrg = (void*)graphic;
	short wi;
			
	switch (message)
	{
		case peGraphicDraw:	/* data is (Point *) with penLoc at baseline left as a point */
			PeteGraphicRect(&r,pte,graphic,offset);
			SetRect(&rule,0,0,RectWi(r),(*hrg)->hi?(*hrg)->hi:2);
			CenterRectIn(&rule,&r);
			if ((*hrg)->groovy && !HaveTheDiseaseCalledOSX()) DrawThemeSeparator(&rule,kThemeStateActive);
			else
			{
				PenNormal();
				SET_COLORS;
				if ((*hrg)->groovy && HaveTheDiseaseCalledOSX()) SetForeGrey(GetRLong(SEP_LINE_GREY));
				PaintRect(&rule);
			}
			break;

		case peGraphicClone:	/* data is (PETEGraphicInfoHandle *) into which to put duplicate */
			*(PETEGraphicInfoHandle*)data = (PETEGraphicInfoHandle) DupHandle((Handle)graphic);
			err = MemError();
			break;
			
		case peGraphicTest:	/* data is (Point *) from top left; return errOffsetIsOutsideOfView if not hit */
			//	Can cancel hit by returning errOffsetIsOutsideOfView
			break;
			
		case peGraphicHit:	/* data is (EventRecord *) if mouse down, nil if time to turn off */
			break;		

		case peGraphicRemove:	/* data is nil; just dispose a copy of graphic */
			DisposeHandle((Handle)graphic);
			break;
			
		case peGraphicResize:	/* data is a (short *) of the max width */
			wi = *(short*)data;
			if ((*hrg)->wi>0) wi = MIN((*hrg)->wi,wi);
			else if ((*hrg)->wi<0) wi = -(wi*(*hrg)->wi)/100;
			(*hrg)->gi.width = wi;
			(*hrg)->gi.height = (*hrg)->hi ? (*hrg)->hi : FontLead;
			break;
		
		case peGraphicRegion:	/* data is a RgnHandle which is to be changed to the graphic's region */
			//	Used mostly for changing the cursor
			PeteGraphicRect(&r,pte,graphic,offset);
			RectRgn((RgnHandle)data,&r);
			break;
	}
	
	return(err);
}


/************************************************************************
 * PeteInsertRule - insert a horizontal rule
 ************************************************************************/
OSErr PeteInsertRule(PETEHandle pte,long offset,short width,short height,Boolean groovy,Boolean withParas,Boolean undo)
{
	PETEStyleEntry pse;
	OSErr err;
	HRGraphicHandle graphic = NewZH(HorizRuleGraphicInfo);
	PETEParaInfo pinfo;
	Boolean did;
	
	if (!graphic) return(MemError());
	
	PeteStyleAt(pte,offset,&pse);
	Zero(pinfo);
	PETEGetParaInfo(PETE,pte,PeteParaAt(pte,offset),&pinfo);
	
	/*
	 * figure out how long the paragraph is
	 */
	pinfo.tabHandle = nil;
	pse.psStartChar = 0;
	pse.psGraphic = 1;
	pse.psStyle.graphicStyle.graphicInfo = (void*)graphic;
	(*graphic)->gi.width = pinfo.endMargin - pinfo.startMargin - pinfo.indent;
	(*graphic)->gi.height = FontLead;
	(*graphic)->gi.descent = FontDescent;
	(*graphic)->gi.itemProc = HorizRuleGraphic;
	(*graphic)->gi.privateType = pgtHorizontalRule;
	(*graphic)->wi = width;
	(*graphic)->hi = height;
	(*graphic)->groovy = groovy;
	pse.psStyle.textStyle.tsLock = 0;
	**Pslh = pse;
	if(undo) PetePrepareUndo(pte,peUndoPaste,-1,-1,nil,nil);
	err = PeteInsertChar(pte,offset,'\015',Pslh); ASSERT(!err);
	if (!err && withParas)
	{
		PeteGetTextAndSelection(pte,nil,&offset,nil);
		PeteEnsureBreak(pte,offset);
		PeteEnsureCrAndBreakLo(pte,offset-1,nil,&did);
		PeteEnsureTrailingEmpty(pte);
		if(undo) PeteFinishUndo(pte,peUndoPaste,did?offset-2:offset-1,offset);
	}
	else if(undo) PeteKillUndo(pte);
	return(err);
}

/**********************************************************************
 * PeteInsert - insert handle, using the style at the offset in question
 **********************************************************************/
OSErr PeteInsert(PETEHandle pte,long offset,Handle text)
{
	OSErr err;
	PETEStyleEntry pse;
	PETEDocInfo info;
	long styleOffset = offset;
	
	if (styleOffset==-1)
	{
		PETEGetDocInfo(PETE,pte,&info);
		if (info.selStart!=info.selStop)
			styleOffset = info.selStart;
		else
			styleOffset = MAX(0,info.selStart-1);
	}
	PeteStyleAt(pte,styleOffset,&pse);
	pse.psStyle.textStyle.tsLock = pse.psStartChar = 0;
	**Pslh = pse;
	err = PETEInsertTextPtr(PETE,pte,offset,LDRef(text),GetHandleSize(text),Pslh);
	UL(text);
	return(err);
}

/**********************************************************************
 * PetePasteQ - paste as quote
 **********************************************************************/
OSErr PetePasteQ(MyWindowPtr win,Boolean plain)
{
	long start, stop, undoStart, undoStop, newStop, finalInsertion;
	OSErr err;
	Boolean rich;
	Str63 quoth;
	Handle textH;
	long dirtySpot;
	Handle scrapH;
	long junk;
	long len;
	
	// Grab the current selection and keep track of it for undo
	PetePrepareUndo(win->pte,peUndoPaste,-1,-1,&start,&stop);
	undoStop = stop;
	undoStart = start;
	dirtySpot = start;
	
	// Paste as quote always goes on own line.  Check to see if we need to insert a return before
	PeteGetTextAndSelection(win->pte,&textH,nil,nil);
	if (start && (*textH)[start-1]!='\015')
	{
		PeteInsertChar(win->pte,-1,'\015',nil);
		if (start+1==PETEGetTextLen(PETE,win->pte))
			PeteInsertPlainParaAtEnd(win->pte);
		else
			PeteEnsureBreak(win->pte,start+1);
		undoStop++;
		start++;
		stop++;
		//PETESelect(PETE,win->pte,start,stop);	// The cr goes in after the start of the selection, so we
																					// need to move the selection
	}
	
	// Do we have a stored attribution?  If so, insert now
	if (scrapH=NuHandle(0))
	{
		if ((len=GetScrap(scrapH,ATTRIBUTION_SCRAP_TYPE,&junk))>0)
		{
			PeteInsert(win->pte,-1,scrapH);
			undoStop += len;
			start += len;
			stop += len;
			//PETESelect(PETE,win->pte,start,stop);	// The cr goes in after the start of the selection, so we
																						// need to move the selection
		}
	}
	
	// Do the paste
	err = PeteEdit(win,win->pte,plain?peePastePlain:peePaste,&MainEvent);
	if (!err)
	{
		PeteEnsureBreak(win->pte,start);	// make sure we get a para break in any case
		
		// Ok, where does the pasted section end?
		PeteGetTextAndSelection(win->pte,&textH,nil,&stop);
		finalInsertion = undoStop = stop;
		
		// If the insertion point isn't at a line start, insert a return before it
		if (stop && (*textH)[stop-1]!='\015')
		{
			PeteInsertChar(win->pte,stop,'\015',nil);
			PeteEnsureBreak(win->pte,stop+1);
			undoStop++;
			stop++;
			finalInsertion++;
		}
		
		// If the insertion point is at the end of the doc, insert para
		if (stop==PETEGetTextLen(PETE,win->pte))
			PeteInsertPlainParaAtEnd(win->pte);
		//If the insertion point is not at the end of a line, insert return & para
		else if ((*textH)[stop]!='\015')
		{
			PeteInsertChar(win->pte,stop,'\015',nil);
			PeteEnsureBreak(win->pte,stop+1);
			undoStop++;
		}

		// Figure out if rich text was involved
		if (IsMessWindow(GetMyWindowWindowPtr(win)))
		{
			if (UseFlowOutExcerpt && !MessFlagIsSet(Win2MessH(win),FLAG_SHOW_ALL))
				rich = true;
			else
			{
				SetMessRich(Win2MessH(win));
				rich = MessIsRich(Win2MessH(win));
			}
		}
		else rich = HasStyles(win->pte,0,PETEGetTextLen(PETE,win->pte),false);
		
		//
		// Now, either excerpt or insert quote characters
		//
		if (rich)
		{
			PetePlain(win->pte,kPETECurrentStyle,kPETECurrentStyle,peAllValid);
			if (*GetRString(quoth,QUOTH))
			{
				PCatC(quoth,'\015');
				PeteInsertPtr(win->pte,start,quoth+1,*quoth);
				PeteEnsureBreak(win->pte,start);
				PeteEnsureBreak(win->pte,start+*quoth);
				PetePlain(win->pte,start,start+*quoth,peAllValid);
				PetePlainParaAtLo(win->pte,start,start+*quoth,peAllParaValid&~(peTabsValid|peQuoteLevelValid));
				start += *quoth;
				stop += *quoth;
				undoStop += *quoth;
				finalInsertion += *quoth;
			}
			if (*GetRString(quoth,UNQUOTH))
			{
				PCatC(quoth,'\015');
				PeteInsertPtr(win->pte,stop,quoth+1,*quoth);
				PeteEnsureBreak(win->pte,stop);
				PeteEnsureBreak(win->pte,stop+*quoth);
				undoStop += *quoth;
				finalInsertion += *quoth;
				PetePlain(win->pte,stop,stop+*quoth+1,peAllValid);
				PetePlainParaAtLo(win->pte,stop,stop+*quoth,peAllParaValid&~(peTabsValid|peQuoteLevelValid));
			}
			PeteExcerpt(win->pte,start,stop-1);
#ifdef NEVER
			//convert the excerpts to plain quotes
			ConvertExcerpt(win->pte,start,stop,&newStop);
			undoStop += newStop-stop;
			finalInsertion += newStop-stop;
			//and quote the entire selection with ">"s
			QuoteLines(win->pte,start-1,stop,QUOTE_PREFIX,&newStop);
			undoStop += newStop-stop;
			finalInsertion += newStop-stop;
#endif	//NEVER
		}
		else
		{
			QuoteLines(win->pte,start-1,stop,QUOTE_PREFIX,&newStop);
			undoStop += newStop-stop;
			finalInsertion += newStop-stop;
		}
		PETESelect(PETE,win->pte,finalInsertion,finalInsertion);
		PetePlainParaAt(win->pte,finalInsertion,finalInsertion);
		PeteLock(win->pte,kPETECurrentStyle,kPETECurrentStyle,0);	// just in case lock leaked in
		PeteFinishUndo(win->pte,peUndoPaste,undoStart,undoStop);
	}
	else PeteKillUndo(win->pte);
	
	PeteSetURLRescan(win->pte,dirtySpot);
	PeteNickScan (win->pte);
	
	return(err);
}

/**********************************************************************
 * PeteInsertPlainParaAtEnd - Insert a plain para at the end of the doc
 **********************************************************************/
OSErr PeteInsertPlainParaAtEnd(PETEHandle pte)
{
	PETEParaInfo pinfo;
	OSErr err;
	Zero(pinfo);
	
	if (!(err=PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&pinfo)))
	if (pinfo.tabHandle=NuHandle(sizeof(**(pinfo.tabHandle))))
	{
		pinfo.tabCount = kPETEFixedTab;
		**pinfo.tabHandle = GetRLong(TAB_DISTANCE)*FontWidth;
		err = PETEInsertParaPtr(PETE,pte,kPETELastPara,&pinfo,nil,0,nil);
		ZapHandle(pinfo.tabHandle);
	}
	else err = MemError();
	return(err);
}

/**********************************************************************
 * MouseInPTE - is the mouse in a PETE record?
 **********************************************************************/
Boolean MouseInPTE(PETEHandle pte)
{
	Point pt;
	GetMouse(&pt);
	return(PtInPETE(pt,pte));
}

/**********************************************************************
 * PeteUndoType - return the undo state
 **********************************************************************/
PETEUndoEnum PeteUndoType(PETEHandle pte)
{
	PETEDocInfo info;
	
	PETEGetDocInfo(PETE,pte,&info);
	return(info.undoType);
}

/**********************************************************************
 * PeteInsertPtr - insert pointer, using the style at the offset in question
 **********************************************************************/
OSErr PeteInsertPtr(PETEHandle pte,long offset,Ptr text,long size)
{
	OSErr err;
	PETEStyleEntry pse;
	
	PeteStyleAt(pte,offset,&pse);
	pse.psStyle.textStyle.tsLock = 0;
	pse.psStartChar = 0;
	**Pslh = pse;
	err = PETEInsertTextPtr(PETE,pte,offset,text,size,Pslh);
	return(err);
}

/**********************************************************************
 * PeteInsertChar - insert a single character
 **********************************************************************/
OSErr PeteInsertChar(PETEHandle pte,long offset,unsigned char insertChar,PETEStyleListHandle styles)
{
	return PETEInsertTextPtr(PETE,pte,offset,&insertChar,1,styles);
}

/**********************************************************************
 * PeteGraphicRect - return the rectangle of a graphic
 **********************************************************************/
OSErr PeteGraphicRect(Rect *r,PETEHandle pte,PETEGraphicInfoHandle graphic,long offset)
{
	Point p;
	LHElement lineHeight;
	OSErr err;
	
	err = PETEOffsetToPosition(PETE,pte,offset,&p,&lineHeight);
	p.v += lineHeight.lhAscent+(*graphic)->descent;
	SetRect(r,p.h,p.v-(*graphic)->height,p.h+(*graphic)->width,p.v);
	return(err);
}

/**********************************************************************
 * PeteHide - hide some text by putting a nil graphic over it
 **********************************************************************/
OSErr PeteHide(PETEHandle pte,long start,long stop)
{
	PETEStyleEntry pse;
	long junk;
	OSErr err=noErr;
	Handle text;
	Str255 old;
	Boolean dirty = PeteIsDirty(pte);
	Boolean winDirty = (*PeteExtra(pte))->win->isDirty;
	PETEStyleListHandle pslh;
	
	Zero(pse);
	PETEGetStyle(PETE,pte,start,&junk,&pse);
	if (!pse.psGraphic)
	{
		Zero(pse);
		pse.psGraphic = true;
		PeteGetTextAndSelection(pte,&text,nil,nil);
		MakePStr(old,*text+start,stop-start);
		PeteDelete(pte,start,stop);
		if (pslh = NewZH(PETEStyleEntry))
		{
			**pslh = pse;
			err = PETEInsertTextPtr(PETE,pte,start,old+1,*old,pslh);
			ZapHandle(pslh);
		}
		PETEMarkDocDirty(PETE,pte,dirty);
		(*PeteExtra(pte))->win->isDirty = winDirty;
	}
	
	return(err);
}

/**********************************************************************
 * PeteClearGraphic - clear graphic from a selection
 **********************************************************************/
OSErr PeteClearGraphic(PETEHandle pte,long start,long stop)
{
	PETEStyleInfo psi;
	OSErr err;
	
	Zero(psi);
	
	err = PETESetStyle(PETE,pte,start,stop,&psi,peColorValid|peGraphicColorChangeValid);
	return err;
}

/**********************************************************************
 * PeteLabel - label some text
 **********************************************************************/
OSErr PeteLabel(PETEHandle pte,long selStart,long selEnd,short label,long labelMask)
{
	PETETextStyle pStyle;
		
	Zero(pStyle);
	pStyle.tsLabel = label;
	return(PETESetTextStyle(PETE,pte,selStart,selEnd,&pStyle,labelMask<<kPETELabelShift));
}

/**********************************************************************
 * PeteStyleAt - what's the style at a point?
 **********************************************************************/
OSErr PeteStyleAt(PETEHandle pte,long offset,PETEStyleEntryPtr theStyle)
{
	PETEDocInfo info;
	OSErr err;
	
	if (offset==-1)
	{
		if (err = PETEGetDocInfo(PETE,pte,&info)) return err;
		if (info.selStart!=info.selStop)
			offset = info.selStart;
		else
			offset = MAX(0,info.selStart-1);
	}
	err = PeteGetStyle(pte,offset,nil,theStyle);
	return(err);
}

/**********************************************************************
 * PeteLockedAt - how locked is something?
 **********************************************************************/
Byte PeteLockedAt(PETEHandle pte,long offset)
{
	PETEStyleEntry pse;
	Zero(pse);
	PeteStyleAt(pte,offset,&pse);
	return(pse.psStyle.textStyle.tsLock);
}

/**********************************************************************
 * PeteIsGraphicAt - how locked is something?
 **********************************************************************/
Boolean PeteIsGraphicAt(PETEHandle pte,long offset)
{
	PETEStyleEntry pse;
	Zero(pse);
	PeteStyleAt(pte,offset,&pse);
	return(pse.psGraphic);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteGetStyleLo(PETEHandle pte,long offset,long *runLen,Boolean allowGraphic,PETEStyleEntryPtr theStyle)
{
	OSErr err = PETEGetStyle(PETE,pte,offset,runLen,theStyle);
	if (!allowGraphic) PeteJustSayNoGraphic(theStyle);
	if (!runLen) theStyle->psStartChar = 0;
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
void PeteJustSayNoGraphic(PETEStyleEntryPtr theStyle)
{
	if (theStyle->psGraphic && theStyle->psStyle.graphicStyle.graphicInfo)
	{
		theStyle->psGraphic = 0;
		DEFAULT_COLOR(theStyle->psStyle.textStyle.tsColor);
	}
}

/**********************************************************************
 * PtInPTE - is a point in a PTE
 **********************************************************************/
Boolean PtInPETE(Point pt,PETEHandle pte)
{
	PETEDocInfo pinfo;
	
	PETEGetDocInfo(PETE,pte,&pinfo);
	return(PtInRect(pt,&pinfo.docRect));
}

/**********************************************************************
 * PtInPTEView - is a point in a PTE's view rectangle
 **********************************************************************/
Boolean PtInPETEView(Point pt,PETEHandle pte)
{
	PETEDocInfo pinfo;
	
	PETEGetDocInfo(PETE,pte,&pinfo);
	return(PtInRect(pt,&pinfo.viewRect));
}

/**********************************************************************
 * PeteCursorList - set cursor according to list of pte's
 **********************************************************************/
Boolean PeteCursorList(PETEHandle pte,Point mouse)
{
	EventRecord event;
	
	for (;pte;pte=PeteNext(pte))
	{
		if (PtInPETE(mouse,pte))
		{
			OSEventAvail(nil,&event);
			PETECursor(PETE,pte,mouse,nil,&event);
			return(True);
		}
	}
	return(False);
}

/**********************************************************************
 * PeteSelectWord - expand the insertion point to the current word
 **********************************************************************/
OSErr PeteSelectWord(MyWindowPtr win,PETEHandle pte,long offset)
{
	OSErr err = noErr;
#ifdef WINTERTREE
	PETEStyleEntry pse;
#endif //WINTERTREE
	long selStart,selEnd;
	
	if (offset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
	
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking)) {
		PeteStyleAt(pte,offset,&pse);
		if ((pse.psStyle.textStyle.tsLabel&(pURLLabel|pSpellLabel)) != 0)
			err = PETEFindLabelRun(PETE,pte,MAX(0,offset-1),&selStart,&selEnd,pse.psStyle.textStyle.tsLabel&(pURLLabel|pSpellLabel),pse.psStyle.textStyle.tsLabel&(pURLLabel|pSpellLabel));
		else
			err = PETEGetWord(PETE,pte,offset,true,&selStart,&selEnd,nil,nil);
	}
	else
		err = PETEGetWord(PETE,pte,offset,true,&selStart,&selEnd,nil,nil);
#else
		err = PETEGetWord(PETE,pte,offset,true,&selStart,&selEnd,nil,nil);
#endif //WINTERTREE
	PeteSelect(win,pte,selStart,selEnd);
	return err;
}

/************************************************************************
 * AppendEditItems - add pete items
 ************************************************************************/
OSErr AppendEditItems(MyWindowPtr win,EventRecord *event,MenuHandle contextMenu)
{
	Point pt = event->where;
	long offset;
	long start, stop;
	PETEHandle pte;
		
	// are we over a PETE region?
	GlobalToLocal(&pt);
	
	ASSERT(win);
	if (!win) return fnfErr;
		
	for (pte=win->pteList;pte;pte=(*PeteExtra(pte))->next)
		if (PtInPETE(pt,pte)) break;
	if (!pte) return(noErr);

	// call the context menus
	if (!PETEPositionToOffset(PETE,pte,pt,&offset) && !(PeteLockedAt(pte,offset)&peSelectLock))
	{
		PeteGetTextAndSelection(pte,nil,&start,&stop);
		if (offset<start || offset>stop)
		if (RequestPeteFocus(win,pte))
		{
			if (AttIsSelected(nil,pte,offset,offset,0,nil,nil))
			{
				;
			}
			else
			{
				PeteSelect((*PeteExtra(pte))->win,pte,offset,offset);
				PeteSelectWord((*PeteExtra(pte))->win,pte,offset);
			}
			SetMenuTexts(event->modifiers,false);
		}
#ifdef WINTERTREE
		if (HasFeature (featureSpellChecking) && !SpellDisabled(pte) && HaveSpeller()) AppendSpellItems(pte,contextMenu);
#endif
#ifdef CONTEXT_FILING
		if (HasFeature(featureContextualFiling)) AppendXferSelection(pte,contextMenu);
#endif
#ifdef CONTEXT_SEARCH
		AppendContextSearch(pte,contextMenu);
#endif
	}
	return(noErr);
}

#ifdef CONTEXT_SEARCH
/************************************************************************
 * AppendContextSearch - append contextual search items
 ************************************************************************/
OSErr AppendContextSearch(PETEHandle pte,MenuHandle contextMenu)
{
	MenuHandle mh = GetMHandle(FIND_HIER_MENU);
	
	if (mh && IsMenuItemEnabled(mh,FIND_SEARCH_WEB_ITEM))
	{
		if (CountMenuItems(contextMenu) && !MenuItemIsSeparator(contextMenu,CountMenuItems(contextMenu)))
			AppendMenu(contextMenu,"\p-");	// add a divider
		CopyMenuItem(mh,FIND_SEARCH_WEB_ITEM,contextMenu,REAL_BIG);
		SetMenuItemCommandID(contextMenu,CountMenuItems(contextMenu),(GetMenuID(mh)<<16)|FIND_SEARCH_WEB_ITEM);
		return noErr;
	}
	return fnfErr;
}
#endif //CONTEXT_SEARCH

/**********************************************************************
 * PeteEditWFocus - edit, switching focus if need be
 **********************************************************************/
OSErr PeteEditWFocus(MyWindowPtr win,PETEHandle ph,PETEEditEnum what,EventRecord* event,Boolean *activated)
{
	OSErr err;

	// (jp) Stuff to handle nickname hiliting/completion/expansion for any PETE
	if (ph && HasNickScanCapability (ph))
		NicknameWatcherFocusChange (ph);
		
	err = PeteEdit(win,ph,what,event);

	if (activated) *activated = False;
	
	if (err==tsmDocNotActiveErr)
	{
		PeteFocus(win,ph,True);
		if (activated) *activated = True;
		err=PeteEdit(win,ph,what,event);
	}
	return(err);
}

/**********************************************************************
 * PeteSelect - select stuff, then set proper flags
 **********************************************************************/
void PeteSelect(MyWindowPtr win,PETEHandle pte,long start,long stop)
{
	PETEStyleEntry pse;
	
	if (!PeteIsValid(pte)) return;
	PETESelect(PETE,pte,start,stop);
	PeteStyleAt(pte,-1,&pse);
	if (start==stop && (pse.psStyle.textStyle.tsLock&peClickAfterLock))
		pse.psStyle.textStyle.tsLock = 0;
	pse.psStyle.textStyle.tsLabel = 0;
	PETESetTextStyle(PETE,pte,-2,-2,&pse.psStyle.textStyle,peAllValid);
	if (win && win->pte==pte) win->hasSelection = MyWinHasSelection(win);
}

/**********************************************************************
 * PeteResetCurStyle - Copy default style into current style
 **********************************************************************/
void PeteResetCurStyle(PETEHandle pte)
{
	PETEStyleEntry pse;
	
	if (!PeteStyleAt(pte,kPETEDefaultStyle,&pse))
		PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,&pse.psStyle.textStyle,peAllValid);
}

/**********************************************************************
 * PeteWhompHiliteRegionBecausePeteWontFixIt - force editor to recalc hilite
 **********************************************************************/
OSErr PeteWhompHiliteRegionBecausePeteWontFixIt(PETEHandle pte)
{
  long start, stop;
  OSErr err;

  err = PeteGetTextAndSelection(pte,nil,&start,&stop);
  if (!err) PeteSelect(nil,pte,start,stop);
  return(err);
}

/**********************************************************************
 * PeteLock - lock/unlock some text
 **********************************************************************/
OSErr PeteLock(PETEHandle pte,long start,long stop,short lock)
{
	PETEStyleEntry pse;

	pse.psStyle.textStyle.tsLock = lock;
	return(PETESetTextStyle(PETE,pte,start,stop,&pse.psStyle.textStyle,peLockValid));
}

/**********************************************************************
 * PeteFontAndSize - change font & size in a pete record
 **********************************************************************/
OSErr PeteFontAndSize(PETEHandle pte,short fontID,short size)
{
	PETEDefaultFontEntry defaultFont;
	OSErr err;
	Str255 fontName;

	if (fontID==kPETEDefaultFont && size==kPETEDefaultSize)
		return(PETESetDefaultFont(PETE,pte,nil));
	else
	{
		Zero(defaultFont);
		defaultFont.pdFont = fontID;
		defaultFont.pdSize = size;
		err = PETESetDefaultFont(PETE,pte,&defaultFont);
		if (pte) // gotta set a print font if changing the default font for a particular edit region
		{
			defaultFont.pdFont = GetFontID(GetPref(fontName,PREF_PRINT_FONT));
			defaultFont.pdSize = GetPrefLong(PREF_PRINT_FONT_SIZE);
			if (!defaultFont.pdSize) defaultFont.pdSize = FontSize;
			defaultFont.pdPrint = True;
			PETESetDefaultFont(PETE,pte,&defaultFont);
		}
		return(err);
	}
}

/**********************************************************************
 * PeteGraphicHandle - return a PETeGraphicInfoHandle 
 **********************************************************************/
PETEGraphicInfoHandle PeteGraphicHandle(PicHandle thePic, short resId)
{
	struct PetePictInfo {
		PETEGraphicInfo gi;
		PicHandle pict;
		long counter;
	} **newGHndl;

	if (!thePic)
	{
		thePic = GetResource_('PICT',resId);
		if (!thePic) return(nil);
		HNoPurge((Handle)thePic);
	}

	newGHndl = NuHandleClear(sizeof(struct PetePictInfo));
	if(newGHndl != nil) {
		(**newGHndl).gi.itemProc = nil;
		(**newGHndl).gi.width = RectWidth(&(**thePic).picFrame);
		(**newGHndl).gi.height = RectHeight(&(**thePic).picFrame) + 1;
		(**newGHndl).gi.descent = 0L;
		(**newGHndl).pict = thePic;
		(**newGHndl).counter = 1L;
	}
	return (PETEGraphicInfoHandle)newGHndl;
}

/**********************************************************************
 * PeteTextStyleDiff - take the difference of two styles
 **********************************************************************/
short PeteTextStyleDiff(PETETextStylePtr s1, PETETextStylePtr s2)
{
	long diff = 0;
	PETEStyleEntry pse1, pse2;
	
	Zero(pse1);
	Zero(pse2);
	pse1.psStyle.textStyle = *s1;
	pse2.psStyle.textStyle = *s2;
	
	PETECompareStyles(PETE,nil,&pse1,&pse2,peFaceValid|peSizeValid|peFontValid|peColorValid,false,&diff);
	return(diff);
}

/**********************************************************************
 * PeteParaInfoDiff - take the difference of two ParaInfo's
 **********************************************************************/
short PeteParaInfoDiff(PETEParaInfoPtr s1, PETEParaInfoPtr s2)
{
	short diff = 0;
	
	if (s1->startMargin!=s2->startMargin) diff |= peStartMarginValid;
	if (s1->endMargin!=s2->endMargin) diff |= peEndMarginValid;
	if (s1->indent!=s2->indent) diff |= peIndentValid;
	if (s1->justification!=s2->justification) diff |= peJustificationValid;
	if (s1->quoteLevel!=s2->quoteLevel) diff |= peQuoteLevelValid;

	return(diff);
}

/************************************************************************
 * PeteCopy - PeteCopy but using the new routines
 ************************************************************************/
OSErr PeteCopy(PETEHandle fromPTE, PETEHandle toPTE,long fromFrom, long fromTo, long toTo, long *endedAt,Boolean flatten)
{
	Handle text=nil;
	PETEStyleListHandle pslh=nil;
	PETEParaScrapHandle pScrap=nil;
	OSErr err;
	long len;

	len = PETEGetTextLen(PETE,fromPTE);
	if (fromTo>len) fromTo = len;
	if (fromFrom>=fromTo) return(noErr);
	err = PeteGetTextStyleScrap(fromPTE,fromFrom,fromTo,&text,&pslh,&pScrap);
	if (flatten)
	{
		ZapPeteStyleScrap(pslh);
		ZapPeteParaScrap(pScrap);
	}
	
	if (!err)
	{
		if (toTo==-1) toTo = PeteBumpSelection(toPTE,0);
		len = GetHandleSize(text);
		err = PETEInsertTextScrap(PETE,toPTE,toTo,text,pslh,pScrap,true);
		if (!err && endedAt)
			*endedAt = toTo + len;
	}
	return(err);
}

/************************************************************************
 * PeteCopyNoLabel - copy but clear label
 ************************************************************************/
OSErr PeteCopyNoLabel(PETEHandle fromPTE, PETEHandle toPTE,long fromFrom, long fromTo, long toTo, long *endedAt,Boolean flatten,long labelMask)
{
	long localEnded;
	OSErr err;
	
	err = PeteCopy(fromPTE,toPTE,fromFrom,fromTo,toTo,&localEnded,flatten);
	if (!err)
	{
		if (endedAt) *endedAt = localEnded;
		if (labelMask) PetePlain(toPTE,toTo,localEnded,labelMask<<kPETELabelShift);
	}
	return(err);
}


/**********************************************************************
 * PeteCopyOld - copy stuff from one pete record to another
 **********************************************************************/
OSErr PeteCopyOld(PETEHandle fromPTE, PETEHandle toPTE,long fromFrom, long fromTo, long toTo, long *endedAt, Boolean flatten)
{
	long fromPara, toPara;
	OSErr err=noErr;
	
	fromPara = PeteParaAt(fromPTE,fromFrom);
	toPara = PeteParaAt(fromPTE,fromTo);
	
	while (fromPara<=toPara)
	{
		if (err=PeteCopyPara(fromPTE,toPTE,fromPara,fromFrom,fromTo,toTo,&toTo,flatten))
			break;
		fromPara++;
	}
	if (!err && endedAt) *endedAt = toTo;
	return(err);
}

/**********************************************************************
 * PeteCopyPara - copy stuff from one pete record to another
 **********************************************************************/
OSErr PeteCopyPara(PETEHandle fromPTE, PETEHandle toPTE,long fromPara,long fromFrom, long fromTo, long toTo, long *endedAt, Boolean flatten)
{
	long toPara;
	OSErr err=noErr;
	PETEParaInfo pinfo;
	long runLen;
	PETEStyleEntry pse;
	UHandle text;
	UHandle newText=nil;
	long copied;
	Boolean newBreak;
	
	/*
	 * figure out where we are
	 */
	toPara = PeteParaAt(toPTE,toTo);
	
	/*
	 * figure out how long the paragraph is
	 */
	pinfo.tabHandle = nil;
	PETEGetParaInfo(PETE,fromPTE,fromPara,&pinfo);
	
	/*
	 * don't copy from before the beginning or past the end of the paragraph
	 */
	fromTo = MIN(fromTo,pinfo.paraOffset+pinfo.paraLength);
	fromFrom = MAX(fromFrom,pinfo.paraOffset);
	
	/*
	 * prefill with the default style
	 */
	PeteGetStyle(toPTE,kPETEDefaultStyle,nil,&pse);
	pse.psStartChar = 0;
	(*Pslh)->psStyle.textStyle = pse.psStyle.textStyle;
	
	/*
	 * get hold of the text
	 */
	PETEGetRawText(PETE,fromPTE,&text);
	
	/*
	 * copy the style runs one at a time
	 */
	while (fromFrom<fromTo)
	{
		// get the run information
		PeteGetStyle(fromPTE,fromFrom,&runLen,&pse);

		// turn runLen into an absolute end of run
		runLen += pse.psStartChar;
		
		// don't copy beyond the end of the interesting bit
		runLen = MIN(runLen,fromTo);

#ifdef NEVER // dunno what point this served; maybe because the scanner would scan?  Anyway, it's in the way now.
		// if the style is a URL, forget the style change
		if (!(pse.psStyle.textStyle.tsLabel&(pURLLabel|pLinkLabel)))
#endif
		{
			**Pslh = pse;
		}

		// We don't want to worry about an offset
		(*Pslh)->psStartChar = 0;
		
		// copy the data
		newText = NuHTempBetter(runLen-fromFrom);
		if (!newText) {err = MemError(); break;}
		BMD(*text+fromFrom,*newText,runLen-fromFrom);
		copied = runLen-fromFrom;
		fromFrom = runLen;

		// insert the data
		err = PETEInsertText(PETE,toPTE,toTo,newText,Pslh);
		if (!err) newText = nil;
		else break;
		
		// need a paragraph break?
		if (!flatten)
		{
			PeteEnsureBreakLo(toPTE,toTo,&newBreak);
			if (newBreak) toPara++;
			pinfo.paraFlags &= ~peNoParaPaste;
			//Stlyed Text - prevent margins from making it into replies, etc.
			if (HasFeature (featureStyleMargin))
				PETESetParaInfo(PETE,toPTE,toPara,&pinfo,peAllParaValidButTabs);
			else
				PETESetParaInfo(PETE,toPTE,toPara,&pinfo,peAllParaValidButTabs & ~(peJustificationValid|peStartMarginValid|peEndMarginValid));
			flatten = True;	// copy paragraph info only once
		}
		
		// new offset to copy to
		toTo += copied;
	}
	ZapHandle(newText);
	
	if (endedAt) *endedAt = toTo;

	return(err);
}

#ifdef DEBUG
/**********************************************************************
 * PeteDump - dump a pete handle to the debugger
 **********************************************************************/
OSErr PeteDumpLo(PETEHandle pte,short file,short line)
{
	PETEStyleListHandle pslh=nil;
	OSErr err;
	short para;
	Str255 s, s2;
	PeteExtra extra = **PeteExtra(pte);
	long sel1,sel2;
	UHandle text;
	PETEParaInfo pinfo;
	short run;
	long runLen;
	long numRuns;
	PETETextStyle pts;
	PETEGraphicStyle pgs;
	PETEStyleEntry pse;
	
	// Some overall stats
	GetWTitle(GetMyWindowWindowPtr(extra.win),s);
	EscapeInHex(s,s);
	PeteGetTextAndSelection(pte,&text,&sel1,&sel2);
	ComposeLogS(-1,nil,"\p{%d:%d} pte %x len %x sel %d,%d win %p",file,line,pte,PeteLen(pte),sel1,sel2,s);
	
	// Current and default styles
	PeteStyleAt(pte,kPETECurrentStyle,&pse);
	pts = pse.psStyle.textStyle;
	pgs = pse.psStyle.graphicStyle;
	// print the common style info
	ComposeString(s,"\p    Current graphic %d font %d size %d face %x lock %x label %x ",
		pse.psGraphic, pts.tsFont, pts.tsSize, pts.tsFace,
		pts.tsLock, pts.tsLabel);
	// add color or graphic
	if (pse.psGraphic)
		ComposeString(s2,"\pgi %x",pgs.graphicInfo);
	else
		ComposeString(s2,"\pcolor %d.%d.%d",pts.tsColor.red,pts.tsColor.green,pts.tsColor.blue);
	PSCat(s,s2);
	Log(-1,s);
	PeteStyleAt(pte,kPETEDefaultStyle,&pse);
	pts = pse.psStyle.textStyle;
	pgs = pse.psStyle.graphicStyle;
	// print the common style info
	ComposeString(s,"\p    Default graphic %d font %d size %d face %x lock %x label %x ",
		pse.psGraphic, pts.tsFont, pts.tsSize, pts.tsFace,
		pts.tsLock, pts.tsLabel);
	// add color or graphic
	if (pse.psGraphic)
		ComposeString(s2,"\pgi %x",pgs.graphicInfo);
	else
		ComposeString(s2,"\pcolor %d.%d.%d",pts.tsColor.red,pts.tsColor.green,pts.tsColor.blue);
	PSCat(s,s2);
	Log(-1,s);
	
	for (para=0;;para++)
	{
		Zero(pinfo);
		if (err=PETEGetParaInfo(PETE,pte,para,&pinfo)) break;
		
		// paragraph stats
		ComposeLogS(-1,nil,"\ppara %d offset %d len %d flags %x quote %d sign %d sm %d em %d in %d",
			para,pinfo.paraOffset,pinfo.paraLength,pinfo.paraFlags,pinfo.quoteLevel,pinfo.signedLevel,
			pinfo.startMargin,pinfo.endMargin,pinfo.indent);
		
		// Grab the style runs
		if (err=PETEGetDebugStyleScrap(PETE,pte,para,&pslh)) break;

		numRuns = HandleCount(pslh);
		for (run=0;run<numRuns;run++)
		{
			runLen = (run<numRuns-1 ? (*pslh)[run+1].psStartChar : pinfo.paraLength) - (*pslh)[run].psStartChar;
			// text
			if (runLen>0)
			{
				PCopy(s,"\p �");
				if (runLen<64)
				{
					MakePStr(s2,*text+pinfo.paraOffset+(*pslh)[run].psStartChar,runLen);
					EscapeInHex(s2,s2);
					PSCat(s,s2);
				}
				else
				{
					MakePStr(s2,*text+pinfo.paraOffset+(*pslh)[run].psStartChar,31);
					EscapeInHex(s2,s2);
					PSCat(s,s2);
					PSCatC(s,'�');
					MakePStr(s2,*text+pinfo.paraOffset+(*pslh)[run].psStartChar+runLen-31,31);
					EscapeInHex(s2,s2);
					PSCat(s,s2);
				}
				PCat(s,"\p�");
				Log(-1,s);
			}
			
			// Style info
			pts = (*pslh)[run].psStyle.textStyle;
			pgs = (*pslh)[run].psStyle.graphicStyle;
			// print the common style info
			ComposeString(s,"\p    run %d len %d graphic %d font %d size %d face %x lock %x label %x ",
				run, runLen, (*pslh)[run].psGraphic, pts.tsFont, pts.tsSize, pts.tsFace,
				pts.tsLock, pts.tsLabel);
			// add color or graphic
			if ((*pslh)[run].psGraphic)
				ComposeString(s2,"\pgi %x",pgs.graphicInfo);
			else
				ComposeString(s2,"\pcolor %d.%d.%d",pts.tsColor.red,pts.tsColor.green,pts.tsColor.blue);
			PSCat(s,s2);
			Log(-1,s);
			
		}
		ZapHandle(pslh);	// not a real style scrap!
	}
	CloseLog();
	if (err==errAEImpossibleRange) err = noErr;
	
	return(err);
}

#endif

/**********************************************************************
 * PeteCleanList - mark all pte's as dirty
 **********************************************************************/
void PeteCleanList(PETEHandle pte)
{
	for (;pte;pte=PeteNext(pte))
		PETEMarkDocDirty(PETE,pte,False);
}

/**********************************************************************
 * PeteDirty - is a pete dirty?
 **********************************************************************/
long PeteIsDirty(PETEHandle pte)
{
	PETEDocInfo info;
	
	if (PeteIsValid(pte))
	{
		if (!PETEGetDocInfo(PETE,pte,&info))
			return(info.dirty);
		else
			return(False);
	}
	return(0);
}

/**********************************************************************
 * PeteDirtyList - is any pete dirty?
 **********************************************************************/
Boolean PeteIsDirtyList(PETEHandle pte)
{
	for (;pte;pte=PeteNext(pte))
		if (PeteIsDirty(pte)) return(True);
	return(False);
}

/**********************************************************************
 * PeteChanged - set the dirty bit on the window when an edit region changes
 **********************************************************************/
pascal OSErr PeteChanged(PETEHandle pte,long start,long stop)
{
	MyWindowPtr win;

	win = (*PeteExtra(pte))->win;
	win->isDirty = True;
	if (start<0) start = 0;
	PeteSetURLRescan(pte,start);
	NicknameWatcherMaybeModifiedField (pte);
	return noErr;
	}


/**********************************************************************
 * DefaultPII - setup defaults
 **********************************************************************/
void DefaultPII(MyWindowPtr win,Boolean ro,uLong flags,PETEDocInitInfoPtr pdi)
{
	Zero(*pdi);
	pdi->inWindow = GetMyWindowWindowPtr(win);
	pdi->inRect = win->contR;
	pdi->inRect.top = -30;
	pdi->inRect.bottom = -5;
	pdi->docWidth = -1;
	pdi->inStyle.tsFont = kPETEDefaultFont; 	//((GrafPtr)win)->txFont;
#ifdef USERELATIVESIZES
	pdi->inStyle.tsSize = kPETERelativeSizeBase;	//((GrafPtr)win)->txSize;
#else
	pdi->inStyle.tsSize = kPETEDefaultSize;	//((GrafPtr)win)->txSize;
#endif
	DEFAULT_COLOR(pdi->inStyle.tsColor);
	if (ro) pdi->inStyle.tsLock = peModLock;
	if (GetPrefLong(PREF_DRAG_OPTIONS)&2) flags |= peDragOnControl;
	pdi->docFlags = flags;
	DefaultPPI(&pdi->inParaInfo);
}

/**********************************************************************
 * PeteSetupTextColors - set the default colors
 **********************************************************************/
void PeteSetupTextColors(PETEHandle pte,Boolean black)
{
	PETELabelStyleEntry pls;
	
	Zero(pls);
	
	pls.plColorWeight = GetRLong(QUOTE_BLEND);
	if (!black) GetRColor(&pls.plColor,QUOTE_COLOR);
	
	// Quotes and quoted stationery
	pls.plValidBits = peColorValid;
	pls.plLabel = pQuoteLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel = pQuoteLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);
	
	// Recognized url's
	pls.plColorWeight = 100;
	if (!black) GetRTextColor(&pls.plColor,URL_COLOR);
	pls.plFace = GetRLong(URL_STYLE);
	pls.plValidBits = peColorValid|pls.plFace;
	pls.plLabel = pURLLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel |= pQuoteLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel = pURLLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel |= pQuoteLabel; PETESetLabelStyle(PETE,pte,&pls);
	
	// HTML links
	if (!black) GetRTextColor(&pls.plColor,LINK_COLOR);
	pls.plFace = GetRLong(LINK_STYLE);
	pls.plLabel = pLinkLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel = pLinkLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel = pLinkLabel|pQuoteLabel; PETESetLabelStyle(PETE,pte,&pls);
	pls.plLabel = pLinkLabel|pQuoteLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);
	
	// Weird reply-to lines
	pls.plValidBits = peColorValid;
	if (!black) GetRTextColor(&pls.plColor,REPLY_COLOR);
	pls.plLabel = pReplyLabel; PETESetLabelStyle(PETE,pte,&pls);
	
	// Background and foreground colors
	if (!black) GetRColor(&pls.plColor,TEXT_COLOR);
	PETESetDefaultColor(PETE,pte,&pls.plColor);
	GetRColor(&pls.plColor,BACK_COLOR);
	PETESetDefaultBGColor(PETE,pte,&pls.plColor);
	
	// and the speller
#ifdef WINTERTREE
	if (HasFeature (featureSpellChecking)) {
		if (!black) GetRTextColor(&pls.plColor,SPELL_COLOR);
		pls.plFace = GetRLong(SPELL_FACE);
		pls.plValidBits = peColorValid|pls.plFace;
		pls.plLabel = pSpellLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pSpellLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);	
	}
#endif

	if (HasFeature (featureAnal)) {
		if (!black) GetRTextColor(&pls.plColor,MOOD_COLOR);
		pls.plFace = GetRLong(MOOD_FACE);
		pls.plValidBits = peColorValid|pls.plFace;
		pls.plLabel = pTightAnalLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pTightAnalLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pTightAnalLabel|pSpellLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pTightAnalLabel|pStationeryLabel|pSpellLabel; PETESetLabelStyle(PETE,pte,&pls);	
		if (!black) GetRTextColor(&pls.plColor,MOOD_H_COLOR);
		pls.plFace = GetRLong(MOOD_H_FACE);
		pls.plValidBits = peColorValid|pls.plFace;
		pls.plLabel = pLooseAnalLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pLooseAnalLabel|pStationeryLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pLooseAnalLabel|pSpellLabel; PETESetLabelStyle(PETE,pte,&pls);	
		pls.plLabel = pLooseAnalLabel|pStationeryLabel|pSpellLabel; PETESetLabelStyle(PETE,pte,&pls);	
	}

	// nickname hiliting
	pls.plFace = (GetRLong(NICK_STYLE)&0xFF);
	pls.plValidBits = pls.plFace;
	if (!black) GetRTextColor(&pls.plColor, NICK_COLOR);
	if (!IsPETEDefaultColor(pls.plColor)) pls.plValidBits |= peColorValid;
	pls.plLabel = pNickHiliteLabel; PETESetLabelStyle(PETE,pte,&pls);	
	
	// autoscroll
	PETEAutoScrollTicks(PETE,GetRLong(SCROLL_THROTTLE)-1);
}

#if defined(NEVER)
/**********************************************************************
 * PeteSetRudeColor - set the colors for rude url's
 **********************************************************************/
void PeteSetRudeColor(void)
{
	PETELabelStyleEntry pls;
	
	Zero(pls);
	pls.plColor.blue = Random();
	pls.plColor.green = Random();
	pls.plColor.red = Random();
	DarkenColor(&pls.plColor,30);
	pls.plFace = GetRLong(URL_STYLE);
	pls.plValidBits = peColorValid|pls.plFace;
	pls.plLabel = pRudeURLLabel; PETESetLabelStyle(PETE,nil,&pls);
}
#endif

/**********************************************************************
 * DefaultPPI - default paragraph info
 **********************************************************************/
PETEParaInfoPtr DefaultPPI(PETEParaInfoPtr ppi)
{
	long size;
	
	size = GetRLong(TAB_DISTANCE)*FontWidth;
	Zero(*ppi);
	
	ppi->tabHandle = (void*)NuHandle(sizeof(**(ppi->tabHandle)));
	if (ppi->tabHandle)
	{
		ppi->tabCount = kPETEFixedTab;
		**ppi->tabHandle = size;
	}
	
	ppi->startMargin = ppi->endMargin = -1;
	ppi->justification = teFlushLeft;
	ppi->paraFlags = peTextOnly | peNoParaPaste | pePlainTextOnly;

	ppi->direction = kHilite; // Default system direction
	
	return(ppi);
}

/**********************************************************************
 * PeteParaSelect - select the current text, all the way out to the
 * paragraph boundaries
 **********************************************************************/
OSErr PeteParaRange(PETEHandle pte,long *startPtr, long *endPtr)
{
	UHandle text;
	long start,end;
	long paraPoint;
	PETEParaInfo pinfo;
	long pIndex;
	OSErr err;
	
	if (err=PeteGetTextAndSelection(pte,&text,&start,&end)) return(err);
	if (startPtr && *startPtr!=-1) start = *startPtr;
	if (endPtr && *endPtr!=-1) end = *endPtr;
	pinfo.tabHandle = nil;

	/*
	 * beginning
	 */
	pIndex = PeteParaAt(pte,start);
	PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
	while (start>pinfo.paraOffset && (*text)[start-1]!='\015') start--;
	
	/*
	 * end
	 */
	pIndex = PeteParaAt(pte,end);
	PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
	paraPoint = pinfo.paraOffset+pinfo.paraLength;
	if (start<paraPoint) end = MAX(end,start+1);	// try not to look at the same return start did
	while (end<paraPoint && (*text)[end-1]!='\015') end++;
	
	if (startPtr) *startPtr = start;
	if (endPtr) *endPtr = end;
	return(noErr);
}

/**********************************************************************
 * PeteEnsureRangeBreak - make sure returns near us have breaks
 **********************************************************************/
OSErr PeteEnsureRangeBreak(PETEHandle pte,long start,long stop)
{
	OSErr err = PeteParaRange(pte,&start,&stop);
	if (!err) err = PeteEnsureBreak(pte,start);
	if (!err) err = PeteEnsureBreak(pte,stop);
	
	return(err);
}

/**********************************************************************
 * PeteParaConvert - convert text to separate paragraphs
 **********************************************************************/
OSErr PeteParaConvert(PETEHandle pte,long start,long end)
{
	UHandle text;
	PETEParaInfo pinfo;
	Boolean bitterEnd = end >= PETEGetTextLen(PETE,pte);
	long len = PeteLen(pte);

	PeteGetTextAndSelection(pte,&text,nil,nil);
	pinfo.tabHandle = nil;
	if (end<len) end++;
	else end = len;
	
	if (start<0) start = 0;
	
	while (start<=end)
	{
		if (start && (*text)[start-1]=='\015')
			PeteEnsureBreak(pte,start);
		start++;
	}
	
	if (bitterEnd) PeteEnsureTrailingEmpty(pte);
	
	return(noErr);
}

/************************************************************************
 * PeteEnsureTrailingEmpty - make sure the last paragraph is empty
 ************************************************************************/
OSErr PeteEnsureTrailingEmpty(PETEHandle pte)
{
	long lastPara;
	long len = PETEGetTextLen(PETE,pte);
	OSErr err = PETEGetParaIndex(PETE,pte,len+1,&lastPara);
	PETEParaInfo info;
	Handle text;
	
	if (len && !err)
	{
		Zero(info);
		PeteGetTextAndSelection(pte,&text,nil,nil);
		if ((*text)[len-1]=='\015' && !(err=PETEGetParaInfo(PETE,pte,lastPara,&info)))
			if (info.paraLength)
				//last para is non-empty.  Add one
				err = PETEInsertParaBreak(PETE,pte,PETEGetTextLen(PETE,pte));
	}
	return(err);				
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteEnsureBreakLo(PETEHandle pte,long inOffset,Boolean *did)
{
	PETEParaInfo pinfo;
	long offset;
	long pIndex;
	OSErr err = noErr;
	
	if (did) *did = False;

	if (inOffset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
	else offset = inOffset;
	
	if (offset>1)
	{
		Zero(pinfo);
		pIndex = PeteParaAt(pte,offset-1);
		PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
		if (pinfo.paraOffset+pinfo.paraLength!=offset || inOffset==-1&&pinfo.paraLength)
		{
#ifdef DEBUG
			ASSERT(PeteCrAt(pte,offset-1) || offset==PeteLen(pte));
#endif
			err = PETEInsertParaBreak(PETE,pte,offset);
			if (!err && did) *did = True;
		}
	}
	return err;
}

/************************************************************************
 * PeteEnsureCrAndBreak - make sure there's a cr and a break at the offset
 ************************************************************************/
OSErr PeteEnsureCrAndBreakLo(PETEHandle pte, long inOffset, long *newOffset,Boolean *did)
{
	long offset,selEnd;
	UHandle textH;
	OSErr err = noErr;
	
	
	// Grab the text handle, find out where the insertion point is
	PeteGetTextAndSelection(pte,&textH,&offset,&selEnd);
	
	// Have we been given an absolute offset that happens to be
	// the insertion point?
	if (inOffset==offset) inOffset = -1;
	
	// If we've been given an offset, use that
	if (inOffset!=-1) offset = inOffset;
	
	// Work to do?
	if (offset && (*textH)[offset-1]!='\015')
	{
		err = PeteInsertChar(pte,offset,'\015',nil);
		if (!err) {
			err = PETEInsertParaBreak(PETE,pte,++offset);
		
			if (!err && inOffset==-1) PeteSelect((*PeteExtra(pte))->win,pte,offset,selEnd+1);
			if (did) *did = !err;
		}
	}
	else
		err = PeteEnsureBreakLo(pte,offset,did);
	if (newOffset) *newOffset = offset;
	return(err);
}
		

/**********************************************************************
 * PeteStringLo - return whole text as string
 **********************************************************************/
PStr PeteStringLo(PStr string,short size,PETEHandle pte)
{
	long got;
	
	if (!PETEGetTextPtr(PETE,pte,0,size-2,string+1,size-1,&got))
	{
		*string = got;
		string[*string+1] = 0;
	}
	else
		*string = 0;
	return(string);
}

#ifdef DEBUG
/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteScroll(PETEHandle pte,short horizontal,short vertical)
{
	return(PETEScroll(PETE,pte,horizontal,vertical));
}
#endif

/**********************************************************************
 * PeteSetTextPtr - replace entire text
 **********************************************************************/
OSErr PeteSetTextPtr(PETEHandle pte,UPtr text,long len)
{
	OSErr err = noErr;
	PeteDelete(pte,0,0x7fffffff);
	PeteScroll(pte,0,pseCenterSelection);
	err = PETEInsertTextPtr(PETE,pte,-1,text,len,nil);
	PeteSetURLRescan(pte,0);
	PeteNickScan (pte);
	return err;
}

/**********************************************************************
 * PeteSetTextPtr - replace entire text
 **********************************************************************/
OSErr PeteSetText(PETEHandle pte,Handle text)
{
	OSErr err = noErr;
	
	PeteDelete(pte,0,0x7fffffff);
	PeteScroll(pte,0,pseCenterSelection);
	if (text)
	{
		err = PETEInsertTextPtr(PETE,pte,-1,LDRef(text),GetHandleSize(text),nil);
		UL(text);
	}
	PeteSetURLRescan(pte,0);
	PeteNickScan (pte);
	return err;
}

/**********************************************************************
 * PeteSmallParas - make sure pargraphs are reasonable in size
 **********************************************************************/
void PeteSmallParas(PETEHandle pte)
{
	long len = PETEGetTextLen(PETE,pte);
	long spot;
	long last = 0;
	Handle text;
	Boolean dirty = 0!=PeteIsDirty(pte);
	
	PeteGetTextAndSelection(pte,&text,nil,nil);
	for (spot=0;spot<len;spot++)
		if ((*text)[spot]=='\015' && spot-last>(4 K))
		{
			PeteEnsureBreak(pte,spot+1);
			last = spot;
		}
	PETEMarkDocDirty(PETE,pte,dirty);
	(*PeteExtra(pte))->win->isDirty = dirty;
}
	

/**********************************************************************
 * 
 **********************************************************************/
void PeteUpdate(PETEHandle pte)
{
	MyWindowPtr win = (*PeteExtra(pte))->win;
	
	if ((*PeteExtra(pte))->frame)
	{
		PeteFrame(pte,nil);
		if (!InBG && win->isActive && win->pte==pte)
			PeteHotRect(pte,!(*PeteExtra(pte))->isInactive);
	}
	PushGWorld();
	RGBBackColor(&win->backColor);
	PETEDraw(PETE,pte);
	PopGWorld();
}

/**********************************************************************
 * PeteFrame - put a border around a PETE edit region
 **********************************************************************/
void PeteFrame(PETEHandle pte,RGBColor *baseColor)
{
	PETEDocInfo info;
	
	PETEGetDocInfo(PETE,pte,&info);
	
	if (!baseColor) DrawThemeEditTextFrame(&info.docRect,(*PeteExtra(pte))->win->isActive && !(*PeteExtra(pte))->isInactive ?kThemeStateActive:kThemeStateInactive);
	else Frame3DOrNot(&info.docRect,baseColor,False);
}

/**********************************************************************
 * PeteHotRect - put a focus ring around a PETE edit region
 **********************************************************************/
void PeteHotRect(PETEHandle pte,Boolean on)
{
	PETEDocInfo info;
	
	PETEGetDocInfo(PETE,pte,&info);
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr((*PeteExtra(pte))->win));
	if (on) DrawThemeFocusRect(&info.docRect,on);
	else
	{
		RgnHandle rgn = NewRgn();
		if (rgn)
		{
			MyWindowPtr win = (*PeteExtra(pte))->win;
			WindowPtr	winWP = GetMyWindowWindowPtr(win);

			RectRgn(rgn,&info.docRect);
			InsetRgn(rgn,-MAX_APPEAR_RIM,-MAX_APPEAR_RIM);
			RgnMinusRect(rgn,&info.docRect);
			InvalWindowRgn(winWP,rgn);
			DisposeRgn(rgn);
		}
	}
	PopGWorld();
}

/**********************************************************************
 * PeteRect - return the rectangle of a pete region
 **********************************************************************/
Rect *PeteRect(PETEHandle pte,Rect *r)
{
	PETEDocInfo info;
	PETEGetDocInfo(PETE,pte,&info);
	*r = info.docRect;
	return(r);
}

/************************************************************************
 * PeteRecalc - force pete region to recalc entire region
 ************************************************************************/
void PeteRecalc(PETEHandle pte)
{
	Boolean	dirtyWindow,dirtyPete;
	Rect	r;
	RgnHandle oldClip;
	MyWindowPtr pteWin = (*PeteExtra(pte))->win;
	GrafPtr	ptePort = GetMyWindowCGrafPtr(pteWin);
	
	//	Save dirty status
	dirtyPete = PeteIsDirty(pte);
	dirtyWindow = pteWin->isDirty;
	
	//	Turn off calculations, resize, then restore
	PeteCalcOff(pte);

	oldClip = SavePortClipRegion(ptePort);
	SetPort_(ptePort);
	SetRect(&r,0,0,0,0);
	ClipRect(&r);
	PeteRect(pte,&r);
	InsetRect(&r,-100,-100);
	PeteDidResize(pte,&r);
	InsetRect(&r,100,100);
	PeteDidResize(pte,&r);	
	PeteCalcOn(pte);
	RestorePortClipRegion(ptePort,oldClip);

	if (!dirtyPete) PETEMarkDocDirty(PETE,pte,false);
	pteWin->isDirty = dirtyWindow;
}

/**********************************************************************
 * PeteRemoveFromRgn - remove the pete rectangles from something
 **********************************************************************/
RgnHandle PeteRemoveFromRgn(RgnHandle rgn,PETEHandle pte)
{
	PETEDocInfo info;
	
	for (;pte;pte=(*PeteExtra(pte))->next)
	{
		PETEGetDocInfo(PETE,pte,&info);
		RgnMinusRect(rgn,&info.docRect);
	}
	return(rgn);
}

/**********************************************************************
 * PeteSelectedString - return selected string
 **********************************************************************/
PStr PeteSelectedString(PStr string,PETEHandle pte)
{
	long got;
	PETEDocInfo info;
	
	PETEGetDocInfo(PETE,pte,&info);
	*string = 0;
	if (info.selStop>info.selStart)
	{
		info.selStop = MIN(info.selStart+250,info.selStop);
		PETEGetTextPtr(PETE,pte,info.selStart,info.selStop,string+1,253,&got);
		*string = info.selStop-info.selStart;
		string[*string+1] = 0;
	}
	return(string);
}

/**********************************************************************
 * PeteLen - how long is a Pete?
 **********************************************************************/
long PeteLen(PETEHandle pte)
{
	return(PETEGetTextLen(PETE,pte));
}

/**********************************************************************
 * PeteGetTextAndSelection - get text handle and current selection
 **********************************************************************/
OSErr PeteGetTextAndSelection(PETEHandle pte,Handle *textH,long *selStart,long *selEnd)
{
	OSErr err = fnfErr;
	PETEDocInfo info;
	
	if (PeteIsValid(pte))
	{
		if (textH) err = PeteGetRawText(pte,textH);
		else err = noErr;
		if (selStart || selEnd)
		{
			if (err=PETEGetDocInfo(PETE,pte,&info))
				info.selStart = info.selStop = 0;
			if (selStart) *selStart = info.selStart;
			if (selEnd) *selEnd = info.selStop;
		}
	}
	return(err);
}

/************************************************************************
 * PeteBumpSelection - move the selection by an increment
 ************************************************************************/
long PeteBumpSelection(PETEHandle pte,long bumpAmount)
{
	long sel;
	
	PeteGetTextAndSelection(pte,nil,&sel,nil);
	if (bumpAmount)
	{
		sel += bumpAmount;
		PeteSelect(nil,pte,sel,sel);
	}
	
	return sel;
}

/**********************************************************************
 * PeteGetRawText - get raw text; return empty handle if no text
 **********************************************************************/
OSErr PeteGetRawText(PETEHandle pte,UHandle *text)
{
	static UHandle empty;
	OSErr err;
	
	*text = nil;
	
	err = PETEGetRawText(PETE,pte,text);
	
	if (!*text)
	{
		if (!empty) empty=NuHandle(0L);
		if (empty) SetHandleBig_(empty,0);
		*text = empty;
	}
	
	return(err);
}

/**********************************************************************
 * PeteCharAt - What character is at an offset?
 **********************************************************************/
uShort PeteCharAt(PETEHandle pte,long offset)
{
	UHandle text;
	PeteGetTextAndSelection(pte,&text,nil,nil);
	if (!text || offset<0 || offset>=GetHandleSize(text)) return(0);
	else return((*text)[offset]);
}

/**********************************************************************
 * PeteDelete - delete some text
 **********************************************************************/
OSErr PeteDelete(PETEHandle pte,long start,long stop)
{
	if (start==stop && start>=0) return(noErr);
	return(PETEInsertTextPtr(PETE,pte,start,nil,stop-start,nil));
}

/* MJN *//* moved the macros for HLFTAB2FIX and FIX2HLFTAB from here to peteglue.h */

/**********************************************************************
 * PeteConvert2Marg - convert from Pete's margin conventions to mine
 **********************************************************************/
void PeteConvert2Marg(PETEParaInfoPtr pinfo,PSMPtr marg)
{
	long width = FontWidth*GetRLong(TAB_DISTANCE);
	
	if (pinfo->indent<0)
	{
		marg->first = FIX2HLFTAB(pinfo->startMargin+pinfo->indent);
		marg->second = FIX2HLFTAB(pinfo->startMargin);
	}
	else
	{
		marg->first = FIX2HLFTAB(pinfo->startMargin);
		marg->second = marg->first + FIX2HLFTAB(pinfo->indent);
	}
 	marg->right = FIX2HLFTAB(-pinfo->endMargin);
}

/**********************************************************************
 * PeteConvertMarg - convert from my margin conventions to Pete's
 **********************************************************************/
void PeteConvertMarg(PETEHandle pte,long basePara,PSMPtr marg,PETEParaInfoPtr pinfo)
{
	PETEParaInfo dinfo;
	long width = FontWidth*GetRLong(TAB_DISTANCE);
	
	dinfo.tabHandle = nil;
	PETEGetParaInfo(PETE,pte,basePara,&dinfo);

 	/*
	 * Pete has a weird mind
	 */
	if (marg->first > marg->second)
	{
		pinfo->startMargin = dinfo.startMargin + HLFTAB2FIX(marg->second);
		pinfo->indent = HLFTAB2FIX(marg->first-marg->second);
	}
	else
	{
		pinfo->startMargin = dinfo.startMargin + HLFTAB2FIX(marg->first);
		pinfo->indent = HLFTAB2FIX(marg->first-marg->second);
	}
	
	/*
	 * set the end margin
	 */
	pinfo->endMargin = dinfo.endMargin - HLFTAB2FIX(marg->right);
}

/**********************************************************************
 * OSErr PeteParaInset - inset some paragraphs
 **********************************************************************/
OSErr PeteParaInset(PETEHandle pte,long fromOffset,long toOffset,PSMPtr marg)
{
	OSErr err = noErr;
	
	if (marg->first || marg->right)
	{
		long from = PeteParaAt(pte,fromOffset);
		long to = PeteParaAt(pte,toOffset);
		Fixed first, right;
		long width = FontWidth*GetRLong(TAB_DISTANCE);
		PETEParaInfo pinfo;

		first = HLFTAB2FIX(marg->first);
		right = HLFTAB2FIX(marg->right);
		Zero(pinfo);
	
		for (;from<=to;from++)
		{
			if (!(err = PETEGetParaInfo(PETE,pte,from,&pinfo)))
			{
				if (first) pinfo.startMargin += first;
				if (right) pinfo.endMargin -= right;
				err = PETESetParaInfo(PETE,pte,from,&pinfo,peStartMarginValid|peEndMarginValid);
			}
		}
	}
	return(err);			
}

/**********************************************************************
 * PETESetTextStyle - set a text style
 **********************************************************************/
OSErr PETESetTextStyle(PETEInst pi,PETEHandle ph,long start,long stop,PETETextStylePtr textStyle,long validBits)
{
	return(PETESetStyle(pi,ph,start,stop,(PETEStyleInfoPtr)textStyle,validBits));
}

/**********************************************************************
 * PeteExcerpt - make paragraphs more excerpty
 **********************************************************************/
OSErr PeteExcerpt(PETEHandle pte,long fromOffset,long toOffset)
{
	OSErr err = noErr;
	long from = PeteParaAt(pte,fromOffset);
	long to = PeteParaAt(pte,toOffset);
	PETEParaInfo pinfo;
	PETEStyleEntry pse;

	Zero(pinfo);
	Zero(pse);
	
	if (!(err = PETEGetParaInfo(PETE,pte,to,&pinfo)))
		toOffset = pinfo.paraOffset+pinfo.paraLength;
	pse.psStyle.textStyle.tsLabel = pQuoteLabel;
	err = PETESetTextStyle(PETE,pte,fromOffset,toOffset,&pse.psStyle.textStyle,pQuoteLabel<<kPETELabelShift);
	
	for (;!err && from<=to;from++)
	{
		if (!(err = PETEGetParaInfo(PETE,pte,from,&pinfo)))
		{
			pinfo.quoteLevel++;
			err = PETESetParaInfo(PETE,pte,from,&pinfo,peQuoteLevelValid);
		}
	}

	return(err);			
}

/**********************************************************************
 * PeteExcerpt - make a particular paragraph a particular level of excerpt
 **********************************************************************/
OSErr PeteSetExcerptLevelAt(PETEHandle pte,long offsetOfCR,short quoteLevel)
{
	long para;
	PETEParaInfo pInfo;
	OSErr err;
	PETEStyleEntry pse;
	
	// make a paragraph break
	PeteEnsureBreak(pte,offsetOfCR);
	para = PeteParaAt(pte,offsetOfCR);
	
	// set its excerpt
	pInfo.quoteLevel = quoteLevel;
	err = PETESetParaInfo(PETE,pte,para,&pInfo,peQuoteLevelValid);
	
	// and the quote label
	if (!err)
	{
		Zero(pInfo);
		if (!PETEGetParaInfo(PETE,pte,para,&pInfo))
		{
			pse.psStyle.textStyle.tsLabel = quoteLevel ? pQuoteLabel:0;
			err = PETESetTextStyle(PETE,pte,pInfo.paraOffset,pInfo.paraOffset+pInfo.paraLength,&pse.psStyle.textStyle,peLabelValid);
		}
	}
	
	return(err);
}

/**********************************************************************
 * PeteParaAt - what's the paragraph index at an offset?
 **********************************************************************/
long PeteParaAt(PETEHandle pte,long offset)
{
	long index;
	
	if (PETEGetParaIndex(PETE,pte,offset,&index)) {ASSERT(0);return(-1);}
	else return(index);
}

/************************************************************************
 * PeteIsExcerptAt - do we have excerpt here?
 ************************************************************************/
short PeteIsExcerptAt(PETEHandle pte,long offset)
{
	PETEParaInfo pinfo;
	long pIndex = PeteParaAt(pte,offset);
	
	Zero(pinfo);
	PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
	return(pinfo.quoteLevel);
}

/************************************************************************
 * PeteIsBullet - do we have bullet here?
 ************************************************************************/
Boolean PeteIsBullet(PETEHandle pte,long pIndex)
{
	PETEParaInfo pinfo;
	
	Zero(pinfo);
	PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
	return((pinfo.paraFlags&peListBits)!=0);
}

/**********************************************************************
 * PeteIsLabelled - Is something labelled?
 **********************************************************************/
Boolean PeteIsLabelled(PETEHandle pte,long start,long end,short label)
{
	long found1, found2;
	
	if (PETEFindLabelRun(PETE,pte,start,&found1,&found2,label,label)) return false;
	if (found1>=end) return false;
	return true;
}

/**********************************************************************
 * Ascii2Margin - take a margin descriptor and interpret it
 **********************************************************************/
OSErr Ascii2Margin(PStr source,PStr name,PSMPtr marg)
{
	UPtr spot;
	Str255 token;
	
	spot=source+1;
	if (!*source) return(fnfErr);
	if (!PToken(source,token,&spot,",")) return(fnfErr);
	if (name) PCopy(name,token);
	if (*token==1 && token[1]=='-') return(noErr);
	if (!PToken(source,token,&spot,",")) return(fnfErr);
	if (marg) StringToNum(token,&marg->first);
	if (!PToken(source,token,&spot,",")) return(fnfErr);
	if (marg) StringToNum(token,&marg->second);
	if (!PToken(source,token,&spot,",")) return(fnfErr);
	if (marg) StringToNum(token,&marg->right);
	return(noErr);
}

/**********************************************************************
 * Res2PInfo - take a margin descriptor resource and turn it into a peteparainfo
 **********************************************************************/
OSErr Res2PInfo(short source,PETEHandle pte,PStr name,PETEParaInfoPtr pinfop)
{
	Str255 s;
	PeteSaneMargin psm;
	
	Ascii2Margin(GetRString(s,source),name,&psm);
	Zero(*pinfop);
	PeteConvertMarg(pte,kPETELastPara,&psm,pinfop);
	return noErr;
}


/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteSaveTEStyles(PETEHandle pte,FSSpecPtr spec)
{
	OSErr err;
	short refN;
	PETEStyleListHandle pslh=nil;
	StScrpHandle ssh=nil;
	PETEParaScrapHandle paraScrapHandle=nil;
	short oldResF = CurResFile();
	
	if (!(err=PeteRemoveTEStyles(spec)))
		if (HasStyles(pte,0,PETEGetTextLen(PETE,pte),false))
		{
			FSpCreateResFile(spec,FileCreatorOf(spec),FileTypeOf(spec),smSystemScript);
			refN = FSpOpenResFile(spec,fsRdWrPerm);
			if (0<=refN)
			{
				if (!(err=PeteGetTextStyleScrap(pte,0,kPETEEndOfText,nil,&pslh,&paraScrapHandle)))
					if (!(err=PETEConvertToTEScrap(PETE,pslh,&ssh)))
					{
						AddResource((Handle)ssh,'styl',128,"");
						if (!(err=ResError()))
						{
							ssh = nil;
							AddResource((Handle)paraScrapHandle,kPETEParaScrap,128,"");
							if (!(err=ResError())) paraScrapHandle = nil;
						}
					}
				CloseResFile(refN);
				if (!err) err = ResError();
			}
			else err = ResError();
		}
	ZapHandle(pslh);
	ZapHandle(ssh);
	ZapHandle(paraScrapHandle);
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteRemoveTEStyles(FSSpecPtr spec)
{
	OSErr err=noErr;
	short refN;
	Handle resH;
	short oldResF = CurResFile();

	refN = FSpOpenResFile(spec,fsRdWrPerm);
	if (refN!=-1)
	{
		SetResLoad(False);
		while (resH=Get1IndResource('styl',1))
		{
			SetResLoad(True);
			RemoveResource(resH);
			SetResLoad(False);
		}
		while (resH=Get1IndResource(kPETEParaScrap,1))
		{
			SetResLoad(True);
			RemoveResource(resH);
			SetResLoad(False);
		}
		SetResLoad(True);
		CloseResFile(refN);
	}
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * PetePrepareUndo - set up the undo buffer
 **********************************************************************/
OSErr PetePrepareUndo(PETEHandle pte,short undoWhat,long start,long stop,long *uStart,long *uStop)
{
	OSErr err;
	
	if (start==kPETECurrentSelection) PeteGetTextAndSelection(pte,nil,&start,nil);
	if (stop==kPETECurrentSelection) PeteGetTextAndSelection(pte,nil,nil,&stop);
	if (uStart) *uStart = start;
	if (uStop) *uStop = stop;
	
	if ((*PeteExtra(pte))->undoInsertion = start==stop)
		PETEClearUndo(PETE,pte);
	else
		err = PETESetUndo(PETE,pte,start,stop,undoWhat);
		
	PETEAllowUndo(PETE,pte,False,False);
	PeteCalcOff(pte);
	return(err);
}

/**********************************************************************
 * PeteFinishUndo - finish up undo stuff
 **********************************************************************/
OSErr PeteFinishUndo(PETEHandle pte,short undoWhat,long start,long stop)
{
	OSErr err;
	Boolean select;
	
	if (start==kPETECurrentSelection) PeteGetTextAndSelection(pte,nil,&start,nil);
	if (stop==kPETECurrentSelection) PeteGetTextAndSelection(pte,nil,nil,&stop);
	
	// turn undoing back on
	PETEAllowUndo(PETE,pte,True,False);
	
	// set the new text.
	if (undoWhat==peUndoStyleAndPara)
		err = noErr;
	else
	{
		select = !(*PeteExtra(pte))->undoInsertion;
		err = PETEInsertUndo(PETE,pte,start,stop,undoWhat,select);
	}
	
	// and recalc
	PeteCalcOn(pte);
	
	UpdateMyWindow(GetMyWindowWindowPtr((*PeteExtra(pte))->win));
	
	return(err);
}

/**********************************************************************
 * PeteKillUndo - destroy undo
 **********************************************************************/
OSErr PeteKillUndo(PETEHandle pte)
{
	PETEAllowUndo(PETE,pte,True,True);
	PETEClearUndo(PETE,pte);
	PeteCalcOn(pte);
	return(noErr);
}

#ifndef NEVER
OSErr RegisterEudoraComponentDir(FSSpecPtr spec);
OSErr RegisterEudoraComponentFiles(void);

#ifndef PETEINLINE
OSErr PETEInit(PETEInst *pi)
{
	Component aComponent=0;
	ComponentDescription looking;
	OSErr errCode;
	OSErr shortVers;
	PETEInst maybeI = 0;
	Boolean badVers = False;
	
	looking.componentType = 'Edit';
	looking.componentSubType = 'Pete';
	looking.componentManufacturer = 'CSOm';
	
	looking.componentFlags = 0L;
	looking.componentFlagsMask = 0L;
	if (aComponent = FindNextComponent(aComponent, &looking))
	{
		if (maybeI=OpenComponent(aComponent))
		{
			shortVers = GetComponentVersion(maybeI)>>16;
			if (shortVers!=(kPETECurrentVersion>>16))
			{
				CloseComponent(maybeI);
				UnregisterComponent(aComponent);
				badVers = True;
			}
			else
				*pi = maybeI;
		}
		else UnregisterComponent(aComponent);
	}

	if(aComponent == 0L) {
		errCode = badVers ? afpBadVersNum : invalidComponentID;
ErrorReturn :
		*pi = nil;
		return errCode;
	}
	
	return (*pi == nil) ? invalidComponentID : noErr;
}
#endif

OSErr RegisterEudoraComponentFiles(void)
{
	FSSpec spec;
	OSErr err;
	Str63 name;
	FSSpec appSpec;
	
	/*
	 * Eudora
	 */
	RegisterComponentResourceFile(AppResFile,registerComponentGlobal);
	
	/*
	 * in folder with Eudora
	 */
	if (!(err=GetFileByRef(AppResFile,&spec)))
	{
		appSpec = spec;
		RegisterEudoraComponentDir(&spec);
		if (!FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,STUFF_FOLDER),&spec))
		{
			IsAlias(&spec,&spec);
			spec.parID = SpecDirId(&spec);
			*spec.name = 0;
			RegisterEudoraComponentDir(&spec);
		}

		/*
		 * if we're packaged, in plugins folder up and over
		 */
		if (!(err=ParentSpec(&appSpec,&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,PACKAGE_MACOS_FOLDER),&spec)))
		if (!(err=FSMakeFSSpec(spec.vRefNum,spec.parID,GetRString(name,PACKAGE_PLUGINS_FOLDER),&spec)))
		{
			IsAlias(&spec,&spec);
			spec.parID = SpecDirId(&spec);
			*spec.name = 0;
			RegisterEudoraComponentDir(&spec);
		}
		
	
		/*
		 * various application support folders/Eudora
		 */
		{
			long domains[]={kUserDomain,kLocalDomain,kSystemDomain,kNetworkDomain};
			FSSpec oldSpec;
			short n = sizeof(domains)/sizeof(domains[0]);
			
			Zero(oldSpec);
			while (n-->0)
				if (!FindSubFolderSpec(domains[n],kApplicationSupportFolderType,EUDORA_EUDORA,false,&spec))
				if (!SameSpec(&spec,&oldSpec))
				{
					oldSpec = spec;
					if (!SubFolderSpecOf(&spec,PACKAGE_PLUGINS_FOLDER,false,&spec))
						RegisterEudoraComponentDir(&spec);
				}
		}
	}
	return(err);
}

OSErr RegisterEudoraComponentDir(FSSpecPtr spec)
{
	CInfoPBRec hfi;
	Str31 name;
	short refN;
	short lastRefN = CurResFile();
	OSErr err=fnfErr;
	FSSpec newSpec;
	short oldResF = CurResFile();

	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while(!DirIterate(spec->vRefNum,spec->parID,&hfi))
		if (hfi.hFileInfo.ioFlFndrInfo.fdType=='thng' &&
				!(err = FSMakeFSSpec(spec->vRefNum,spec->parID, name, &newSpec)))
		{
			IsAlias(&newSpec,&newSpec);
			refN = FSpOpenResFile(&newSpec, fsRdPerm);
			if(refN == -1) {
				if((err = ResError()) == noErr) {
					err = ioErr;
				}
			} else {
				if(RegisterComponentResourceFile(refN,registerComponentGlobal) < 0L) {
					err = resNotFound;
				}
				CloseResFile(refN);
			}
	}
	UseResFile (oldResF);
	return(err);
}

/**********************************************************************
 * PETECleanup - destroy a component
 **********************************************************************/
#ifndef PETEINLINE
OSErr PETECleanup(PETEInst instance)
{
	ComponentDescription info;
	Component comp=nil;
	
	if (!GetComponentInfo((Component)instance,&info,nil,nil,nil))
		while (comp = FindNextComponent(comp, &info))
		{
			CloseComponent(instance);
			UnregisterComponent(comp);
		}
}
#endif

/************************************************************************
 * PeteGetTextStyleScrap - Build a stupid bitfield
 ************************************************************************/
OSErr PeteGetTextStyleScrap(PETEHandle pte,long from, long to, Handle *text, PETEStyleListHandle *pslh, PETEParaScrapHandle *pScrap)
{
	PETEGetStyleScrapFlags itsAllTrueIDontKnowWhyIHaveToBother;

	itsAllTrueIDontKnowWhyIHaveToBother.allParas = true;
	itsAllTrueIDontKnowWhyIHaveToBother.textTempPref = true;
	itsAllTrueIDontKnowWhyIHaveToBother.styleTempPref = true;
	itsAllTrueIDontKnowWhyIHaveToBother.paraTempPref = true;
	itsAllTrueIDontKnowWhyIHaveToBother.clearLock = true;
	return(PETEGetTextStyleScrap(PETE,pte,from,to,text,pslh,pScrap,&itsAllTrueIDontKnowWhyIHaveToBother));
}


/***********************************************************************************
/*	LightQuote - quote a selection of text, converting excerpts to plain ">" quotes.
/*	Also reset the quotelevel of each paragraph to 0 so excerpt bars aren't drawn.
/***********************************************************************************/
OSErr LightQuote(PETEHandle pte, long startingFrom, long upTo, long *newStart, long *newStop)
{
	OSErr					err = noErr;
	long 					from = 0, to = 0;
	long					start = 0, stop = 0, oStop = 0;
	short					i = 0, level = 0;
	PETEParaInfo 	pinfo;
	Boolean flowed = UseFlowOut;
	
	Zero(pinfo);
		
	PeteEnsureBreak(pte, startingFrom);
	
	//Loop through the selection paragraph by paragraph
	while ((startingFrom < upTo) && (err == noErr))
	{
		from = PeteParaAt(pte,startingFrom);
		
		if (!(err = PETEGetParaInfo(PETE,pte,from,&pinfo)))
		{
			start = pinfo.paraOffset;
			if (newStart) {*newStart = start; newStart = nil;}
			stop = (pinfo.paraOffset + pinfo.paraLength > upTo)?upTo:pinfo.paraOffset + pinfo.paraLength;
			oStop = stop;
			level = pinfo.quoteLevel;
			
			//Space-stuff leading >'s
			if (flowed && PeteCharAt(pte,start)=='>')
			{
				PeteInsertChar(pte,start,' ',nil);
				oStop++; stop++; upTo++;
			}
				
			//Set the quotelevel to 0 so bars aren't drawn
			pinfo.quoteLevel = 0;
			err = PETESetParaInfo(PETE,pte,from,&pinfo,peQuoteLevelValid);
			
			//Quote it with a ">" as many times as the quoteLevel dictates
			for (i=0;i<level;i++)
				QuoteLines(pte,start-1,stop,UseFlowOutExcerpt ? FLOWED_QUOTE:QUOTE_PREFIX,&stop);
			
			//Move on to next paragraph
			startingFrom = stop;		
			upTo += stop - oStop;
			if (newStop) *newStop = start + pinfo.paraLength + stop - oStop;
		}
	}

	return (err);
}


/***********************************************************************************
/*	ConvertExcerpt - Convert any excerpts to plain old ordinary quotes in the selection
/***********************************************************************************/
OSErr ConvertExcerpt(PETEHandle pte,long startingFrom,long upTo,long *newStart,long *newStop)
{
	OSErr					err = noErr;

	short					level = 0;
	long					fromPara,toPara;
	long					start,stop;
	short 				i = 0;
	PETEParaInfo 	pinfo;
	
	Zero(pinfo);

	fromPara = PeteParaAt(pte,startingFrom);
	toPara = PeteParaAt(pte,upTo-1);
	
	//Loop through all the paragraphs from start to stop
	for (i=fromPara;i<=toPara;i++)
	{
		if (!(err = PETEGetParaInfo(PETE,pte,i,&pinfo)))
		{
			//If we find an excerpt, 
			if (level = pinfo.quoteLevel)
			{
				start = pinfo.paraOffset;
				stop = start + pinfo.paraLength;
			
				LightQuote(pte,start,stop,i==fromPara?newStart:nil,i==toPara?newStop:nil);
			}
			else
			{
				if (i==fromPara && newStart) *newStart = pinfo.paraOffset;
				else if (i==toPara && newStop) *newStop = pinfo.paraOffset+pinfo.paraLength;
			}
		}
	}

	return (err);
}

/************************************************************************
 * PeteQuoteScan - scan a single region for quotes and color them
 ************************************************************************/
void PeteQuoteScan(PETEHandle pte)
{
	long l1, l2;
	Handle text;
	Boolean peteDirtyWas = PeteIsDirty(pte)!=0;
	Boolean winDirtyWas = (*PeteExtra(pte))->win->isDirty;
	Ptr spot, end;
	long len;
	Str15 quote;
	Boolean is=false;
	PETEParaInfo info;
		
	len = PETEGetTextLen(PETE,pte);
	if ((*PeteExtra(pte))->quoteScanned>=len)
	{
		(*PeteExtra(pte))->quoteScanned = -1;
		return;
	}

	PeteGetTextAndSelection(pte,&text,nil,nil);
	spot = *text + (*PeteExtra(pte))->quoteScanned;
	end = *text+len;
		
	// find the first return
	if (!(*PeteExtra(pte))->quoteScanned)
		l1 = 0;
	else
	{
		for (;spot<end;spot++) if (*spot=='\015') break;
		l1 = spot-*text+1;
	}

	// find the next return
	for (spot++;spot<end;spot++) if (*spot=='\015') break;
	l2 = spot-*text;
	
	// Got a line?
	if (l1<=l2 && l1<len) (*PeteExtra(pte))->quoteScanned = l2;
	else {(*PeteExtra(pte))->quoteScanned = -1 ; return;}
	
	// excerpted?
	Zero(info);
	if (!PETEGetParaInfo(PETE,pte,PeteParaAt(pte,l1),&info))
		if (info.quoteLevel) is = true;
	
	// Great.  Grab the quote chars
	if (is || *GetRString(quote,QUOTE_PREFIX))
	{
		if (!is) is = quote[1]==(*text)[l1];
		
		NoScannerResets++;	// these changes don't count
		
		if (is)
#ifdef WINTERTREE
			if (HasFeature (featureSpellChecking))
				PeteLabel(pte,l1,l2,pQuoteLabel,pQuoteLabel|pSpellLabel);
			else
				PeteLabel(pte,l1,l2,pQuoteLabel,pQuoteLabel);
#else
			PeteLabel(pte,l1,l2,pQuoteLabel,pQuoteLabel);
#endif
		else
			PeteNoLabel(pte,l1,l2,pQuoteLabel);
		
		NoScannerResets--;
			
		if (!peteDirtyWas) PETEMarkDocDirty(PETE,pte,False);
		(*PeteExtra(pte))->win->isDirty = winDirtyWas;
	}
	else (*PeteExtra(pte))->quoteScanned = -1;
}

#pragma segment Main
/************************************************************************
 * QuoteScan - scan msgs for quotes and if found style
 ************************************************************************/
void QuoteScan(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	PETEHandle pte;
	
	if (!AmQuoteScanning()) return;
	
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
	{
		if (IsKnownWindowMyWindow(winWP))
		{
			win = GetWindowMyWindowPtr (winWP);
			for (pte=win->pteList;pte;pte=PeteNext(pte))
			{
				while ((*PeteExtra(pte))->quoteScanned >= 0)
				{
					UsingWindow(winWP);
					PeteQuoteScan(pte);
					if (NEED_YIELD) return;
				}
			}
		}
	}
}

/**********************************************************************
 * AmQuoteScanning - we're scanning for quotes
 **********************************************************************/
Boolean AmQuoteScanning(void)
{
	//quote scanner needed for speller // RGBColor color;
	//quote scanner needed for speller // Str255 title;

	//quote scanner needed for speller // if (Black(GetRColor(&color,QUOTE_COLOR))) return(false);
	//quote scanner needed for speller // if (!*GetRString(title,QUOTE_PREFIX)) return(false);
	return(true);
}

/**********************************************************************
 * PeteBusy - Pete is busy
 **********************************************************************/
pascal OSErr PeteBusy(void)
{
	gEnterWheelHandlerCount++;	// this will lock out any scroll wheel c*&$
#ifdef DEBUG
	if (BUG1) {PushGWorld();WNE(0,nil,0);PopGWorld();}
#endif
	MemCanFail = False;
	if ((TickCount()-MainEvent.when)>120 && !StillDown())
	{
		PushGWorld();
		CycleBalls();
		PopGWorld();
	}
	else if (NEED_YIELD) WNE(0,nil,0);
	MemCanFail = True;
	gEnterWheelHandlerCount--;	// reenable scroll wheels
	return(noErr);
}

/**********************************************************************
 * PeteCancelOnEvents - Pete is busy, but cancel if user clicks or types
 **********************************************************************/
pascal OSErr PeteCancelOnEvents(void)
{
	static ticks;
	EventRecord event;
	
	if (TickCount()-ticks>6)
	{
		ticks = TickCount();
		if (!InBG && OSEventAvail(mDownMask|keyDownMask,&event)) return(userCanceledErr);
	}
	return(PeteBusy());
}

/**********************************************************************
 * PeteTrimTrailingReturns - Remove trailing returns
 **********************************************************************/
OSErr PeteTrimTrailingReturns(PETEHandle pte,Boolean leaveOne)
{
	UHandle text;
	uLong size = PeteLen(pte);
	uLong leave;
	OSErr err = PeteGetTextAndSelection(pte,&text,nil,nil);
	
	if(err) return err;
	if (size && (*text)[size-1]=='\015')
	{
		leave = size-1;
		while (leave && (*text)[leave]=='\015') leave--;
		if (leaveOne) leave++;
		leave++;
		if (leave<size)
		{
			PETEDocInfo info;
			long index;
			
			err = PETEGetDocInfo(PETE,pte,&info);
			if(err) return err;
			
			err = PETEGetParaIndex(PETE,pte,leave,&index);
			
			while(++index < info.paraCount)
			{
				err = PETEDeletePara(PETE,pte,index);
				if(err) return err;
			}
			
			size = PeteLen(pte);
			if(leave<size) err = PeteDelete(pte,leave,size);
		}
	}
	return(err);
}


OSErr PeteExpandAliases (PETEHandle pte, BinAddrHandle *expandedAddresses, BinAddrHandle originalAddresses, short depth, Boolean wantComments)

{
	if (HasFeature (featureNicknameWatching))
		NicknameCachingScan (pte, originalAddresses);	
	return (ExpandAliases (expandedAddresses, originalAddresses, depth, wantComments));
}


// #include "editor.h" CK
#endif
