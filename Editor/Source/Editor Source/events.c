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

#include "petepch.h"

OSErr MyDoActivate(DocumentInfoHandle docInfo, Boolean activeText, Boolean activeScrolls)
{
#ifdef SHOWHIDESCROLLS
	Rect scrollRect, inScrollRect;
#else
#pragma unused(activeScrolls)
#endif
	PETEPortInfo savedPortInfo;
	RgnHandle scratchRgn;
	MyTextStyle textStyle;
	RGBColor backColor;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;

	/* Save old port info */
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);

	backColor = DocOrGlobalBGColor(nil, docInfo);
	if(!IsPETEDefaultColor(backColor)) {
		RGBBackColorAndPat(&backColor);
	}

	/* If the caret was on, turn it off first */
	EraseCaret(docInfo);
	
	if((!activeText && ((**docInfo).flags.isActive)) || (activeText & !((**docInfo).flags.isActive))) {
		
		if(!EmptyRgn((**docInfo).hiliteRgn)) {
			
			CopyRgn((**docInfo).hiliteRgn, scratchRgn);
			InsetRgn(scratchRgn, 1, 1);
//			LMSetHiliteMode(LMGetHiliteMode() & ~(1 << hiliteBit));
//			InvertRgn(scratchRgn);
			PenMode(hilite);
			PaintRgn(scratchRgn);
		}

		(**docInfo).flags.typingUndoCurrent = false;
		(**docInfo).flags.isActive = !(**docInfo).flags.isActive;
		if((**docInfo).flags.isActive) {
			textStyle = (**docInfo).curTextStyle;
			SetStyleAndKeyboard(docInfo, &textStyle);
		}
	}

#ifdef SHOWHIDESCROLLS
	if((activeScrolls && !((**docInfo).flags.scrollsVisible)) || (!activeScrolls && ((**docInfo).flags.scrollsVisible))) {
		DI_MEMCANTFAIL(docInfo);
		if((**docInfo).vScroll != nil) {
			GetControlRect((**docInfo).vScroll, &scrollRect);
			if(activeScrolls && (scrollRect.bottom - scrollRect.top >= kTooSmallForScroll)) {
				ShowControl((**docInfo).vScroll);
			} else {
				HideControl((**docInfo).vScroll);
				FrameRect(&scrollRect);
				inScrollRect = scrollRect;
				InsetRect(&inScrollRect, 1, 1);
				EraseRect(&inScrollRect);
			}
			ValidRect(&scrollRect);
		}
	
		if((**docInfo).hScroll != nil) {
			GetControlRect((**docInfo).hScroll, &scrollRect);
			if(activeScrolls && (scrollRect.right - scrollRect.left >= kTooSmallForScroll)) {
				ShowControl((**docInfo).hScroll);
			} else {
				HideControl((**docInfo).hScroll);
				FrameRect(&scrollRect);
				inScrollRect = scrollRect;
				InsetRect(&inScrollRect, 1, 1);
				EraseRect(&inScrollRect);
			}
			ValidRect(&scrollRect);
		}

		(**docInfo).flags.scrollsVisible = !(**docInfo).flags.scrollsVisible;
		DI_MEMCANFAIL(docInfo);
	}
#endif
	
	ResetPortInfo(&savedPortInfo);
	
	return noErr;
}

void MyEventFilter(PETEGlobalsHandle globals, EventRecord *theEvent)
{
	long docIndex, idleIndex;
	DocumentInfoHandle docInfo;
	unsigned long caretTime;
	
	caretTime = GetCaretTime();
	docIndex = (InlineGetHandleSize((Handle)globals) - sizeof(PETEGlobals)) / sizeof(DocumentInfoHandle);
	while(--docIndex >= 0L) {
		docInfo = (**globals).docInfoArray[docIndex];
		if(!((**docInfo).flags.progressLoopCalled || (**docInfo).flags.docCorrupt)) {

			for(idleIndex = InlineGetHandleSize((Handle)(**docInfo).idleGraphics) / sizeof(IdleGraphic); --idleIndex >= 0L; ) {
				CallGraphic(docInfo, (*(**docInfo).idleGraphics)[idleIndex].graphicInfo, (*(**docInfo).idleGraphics)[idleIndex].offset, peGraphicEvent, theEvent);
			}
				
			if(theEvent->what == nullEvent) {

				if((**docInfo).flags.drawInProgressLoop) {
					(**docInfo).flags.drawInProgressLoop = false;
					if(!((**docInfo).flags.recalcOff)) {
						InvalidateDocUpdate(docInfo);
					}
				}

				if((**docInfo).flags.isActive && !(**docInfo).flags.selectionDirty && ((**docInfo).selStart.offset == (**docInfo).selEnd.offset)) {
					if((**docInfo).lastCaret + caretTime < theEvent->when) {
						DrawCaret(docInfo, nil);
						(**docInfo).lastCaret = theEvent->when;
					}
				}
			}
		}
	}
}

Boolean MyMenuSelectFilter(PETEGlobalsHandle pi, long menuResult)
{
	return (pi != pi && menuResult != menuResult);
}

OSErr MyHandleEditCall(DocumentInfoHandle docInfo, PETEEditEnum what, EventRecord *theEvent)
{
	OSErr errCode;
	long textLen, startOffset, start, end;
	StringHandle textScrap;
	PETEStyleListHandle styleScrap;
	PETEParaScrapHandle paraScrap;
	PETEUndoEnum undoType;
	SelData selection;
	Boolean plainText, appendPaste;
	
	if((what != peeEvent) || (theEvent->what != nullEvent)) {
		errCode = AddParagraphBreaks(docInfo);
		if(errCode != noErr) {
			return errCode;
		}
	}
	
	plainText = false;
	undoType = peCantUndo;
	switch(what) {
		case peeEvent :
			errCode = MyHandleEvent(docInfo, theEvent);
			break;
		case peeUndo :
			errCode = MyDoUndo(docInfo);
			break;
		case peeCutPlain :
			what = peeCut;
			goto DoPlain;
		case peeCopyPlain :
			what = peeCopy;
		DoPlain :
			plainText = true;
		case peeCut :
		case peeCopy :
			if((**docInfo).selStart.offset == (**docInfo).selEnd.offset) {
				errCode = errAENoUserSelection;
				break;
			}

			errCode = CopySelectionToClip(docInfo, plainText);
			
			if(errCode != noErr) {
				break;
			}
			if(what == peeCopy) {
				(**docInfo).flags.typingUndoCurrent = false;
				break;
			}
			undoType = plainText ? peUndoCutPlain : peUndoCut;
			goto DoClear;
		case peePastePlain :
			plainText = true;
			what = peePaste;
		case peePaste :
			errCode = GetClipContents((**docInfo).globals, &textScrap, &styleScrap, &paraScrap);
			if(errCode != noErr) {
				break;
			}
		case peeClear :
			undoType = peUndoClear;
	DoClear :
			if((**docInfo).selStart.offset == (**docInfo).selEnd.offset) {
				if(what != peePaste) {
					errCode = errAENoUserSelection;
					break;
				} else {
					errCode = noErr;
				}
				start = (**docInfo).selStart.offset;
				end = (**docInfo).selEnd.offset;
				appendPaste = false;
			} else {
				appendPaste = true;
				if(what != peeCut) {
					start = (**docInfo).selStart.offset;
					end = (**docInfo).selEnd.offset;
					errCode = noErr;
				} else {
					errCode = GetIntelligentCutBoundaries(docInfo, &start, &end);
				}
				if(errCode == noErr) {
					errCode = SetDeleteUndo(docInfo, start, end, undoType);
				}
			}
			
			if(errCode == noErr) {
				errCode = DeleteText(docInfo, start, end);
			}
			
			if(what != peePaste) {
				if(errCode == noErr) {
					goto DoReposition;
				}
				break;
			}
			
			if(errCode == noErr) {
				errCode = AddIntelligentPasteSpace(docInfo, nil, textScrap, styleScrap, paraScrap, nil);
			}
			if(errCode == noErr) {
				textLen = InlineGetHandleSize((Handle)textScrap);
				startOffset = (**docInfo).selStart.offset;
				errCode = PasteText(docInfo, -1L, textScrap, styleScrap, paraScrap, plainText, true);
			}
			
			if(errCode == noErr) {
				SetInsertUndo(docInfo, startOffset, textLen, startOffset, startOffset, peUndoPaste, appendPaste);
		DoReposition :
				selection.offset = (**docInfo).selStart.offset;
				selection.lastLine = false;
				if(OffsetToPosition(docInfo, &selection, false) == noErr) {
					if(GetPrimaryLineDirection(docInfo, selection.offset) == leftCaret) {
						selection.hPosition = selection.lPosition;
					} else {
						selection.hPosition = selection.rPosition;
					}
					MakeSelectionVisible(docInfo, &selection);
					(**docInfo).selStart = (**docInfo).selEnd = selection;
				}
			}
			break;
		
		default :
			errCode = paramErr;
	}
	
	return errCode;
}

OSErr MyHandleEvent(DocumentInfoHandle docInfo, EventRecord *theEvent)
{
	OSErr errCode;
	
	switch(theEvent->what) {
		case keyDown :
		case autoKey :
			errCode = HandleKey(docInfo, theEvent);
			break;
		case mouseDown :
			errCode = HandleClick(docInfo, theEvent);
			break;
		case mouseUp :
			(**docInfo).lastClickTime = theEvent->when;
		case nullEvent :
			errCode = noErr;
			break;
		default :
			errCode = errAEEventNotHandled;
	}
	
	if((errCode == noErr) && (theEvent->what != nullEvent)) {
		errCode = AddParagraphBreaks(docInfo);
	}
	return errCode;
}

OSErr HandleKey(DocumentInfoHandle docInfo, EventRecord *theEvent)
{
	Byte charCode, keyCode;
	OSErr errCode;
	long startOffset, endOffset;
	SelData selection;
	long vPosition;
	short hPosition;
	
	errCode = noErr;
	startOffset = (**docInfo).selStart.offset;
	endOffset = (**docInfo).selEnd.offset;
	charCode = theEvent->message & charCodeMask;
	keyCode = ((theEvent->message & keyCodeMask) >> CHAR_BIT);
	hPosition = (**docInfo).hPosition;

	/* If the caret was on, turn it off first */
	EraseCaret(docInfo);
	
	switch(charCode) {
		case kHomeChar :
			if((**docInfo).vPosition <= 0L) {
				errCode = errTopOfDocument;
			} else {
				vPosition = 0L;
				if(hPosition > 0L) {
					hPosition = 0L;
				}
			}
			goto SetPosition;
		case kEndChar :
			vPosition = GetDocHeight(docInfo);
			vPosition -= RectHeight(&(**docInfo).viewRect);
			if(vPosition < (**docInfo).vPosition) {
				errCode = errEndOfDocument;
			} else {
				hPosition = GetScrollingWidth(docInfo) - RectWidth(&(**docInfo).viewRect);
				if(hPosition < (**docInfo).hPosition) {
					hPosition = (**docInfo).hPosition;
				}
			}
	SetPosition :
			if(errCode == noErr) {
				SetDocPosition(docInfo, vPosition, hPosition);
				SetScrollPosition(docInfo, false);
			}
			break;
		case kPageUpChar :
		case kPageDownChar :
			DoScroll(docInfo, pseNoScroll, (charCode == kPageUpChar) ? psePageUp : psePageDn);
			break;

		case kBackspaceChar :
		case kForwardDeleteChar :
			if(startOffset == endOffset) {
				SelData selection = (**docInfo).selStart;
				
				errCode = GetHorizontalSelection(docInfo, &selection, theEvent->modifiers, (charCode == kBackspaceChar), false);
				if(errCode == noErr) {
					if(selection.offset < (**docInfo).selStart.offset) {
						startOffset = selection.offset;
					} else {
						endOffset = selection.offset;
					}
					goto DoDelete;
				} else {
					break;
				}
			} else {
				goto IntelligentCut;
			}
			
		case kClearChar :
			if(keyCode == kClearKey) {
	IntelligentCut :
//				errCode = GetIntelligentCutBoundaries(docInfo, &startOffset, &endOffset);
				startOffset = (**docInfo).selStart.offset;
				endOffset = (**docInfo).selEnd.offset;
				if(errCode == noErr) {
	DoDelete :
					if(startOffset != endOffset) {
						errCode = DeleteWithKey(docInfo, startOffset, endOffset);
						goto MakeVisible;
					}
				}
			}
			break;
		
		case kLeftArrowChar :
		case kRightArrowChar :
			(**docInfo).flags.typingUndoCurrent = false;
			HandleHorizontalArrow(docInfo, theEvent->modifiers, (charCode == kLeftArrowChar));
			break;
		case kUpArrowChar :
		case kDownArrowChar :
			(**docInfo).flags.typingUndoCurrent = false;
			HandleVerticalArrow(docInfo, theEvent->modifiers, (charCode == kUpArrowChar));
			break;

		case kCarriageReturnChar :
			if((**docInfo).flags.clearAllReturns) {

		case kNullChar :
		case kControlBChar :
		case kEnterChar :
		case kHelpChar :
		case kControlFChar :
		case kControlGChar :
		case kLineFeedChar :
		case kControlNChar :
		case kControlOChar :
		case kFKeyChar :
		case kControlQChar :
		case kControlRChar :
		case kControlSChar :
		case kControlTChar :
		case kControlUChar :
		case kControlVChar :
		case kControlWChar :
		case kControlXChar :
		case kControlYChar :
		case kControlZChar :
	NotHandled :
				errCode = errAEEventNotHandled;
				break;
			}
		default :
			if(theEvent->modifiers & cmdKey) {
				goto NotHandled;
			}
			errCode = InsertKeyChar(docInfo, charCode);
		
			if((errCode == noErr) &&
			   (charCode == kCarriageReturnChar) &&
			   !(theEvent->modifiers & shiftKey) &&
			   !((**(**docInfo).paraArray[(**docInfo).selStart.paraIndex].paraInfo).paraFlags & peNoParaPaste)) {
				errCode = InsertParagraphBreak(docInfo, (**docInfo).selStart.offset);
			}

	MakeVisible :
			if(errCode == noErr) {
				ObscureCursor();
				selection = (**docInfo).selStart;
				if(GetPrimaryLineDirection(docInfo, selection.offset) == leftCaret) {
					selection.hPosition = selection.lPosition;
				} else {
					selection.hPosition = selection.rPosition;
				}
				MakeSelectionVisible(docInfo, &selection);
			}
	}
	
	if(((**docInfo).flags.isActive) && ((**docInfo).selStart.offset == (**docInfo).selEnd.offset)) {
		DrawCaret(docInfo, nil);
	}
	
	return errCode;
}

OSErr HandleClick(DocumentInfoHandle docInfo, EventRecord *theEvent)
{
	PETEPortInfo savedPortInfo;
	GraphicInfoHandle graphicInfo;
	long graphicLength;
	Point where;
	SelData selAnchor;
	ControlRef theControl;
	Rect tempViewRect;
	short partCode;
	OSErr errCode;
	
	/* Set up the port information and get the local coord of the click */
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	where = theEvent->where;
	GlobalToLocal(&where);
	
	errCode = noErr;
	
	DI_MEMCANTFAIL(docInfo);
	partCode = FindControl(where, (**docInfo).docWindow, &theControl);
	DI_MEMCANFAIL(docInfo);

	if((partCode != 0) && TrackScrolls(docInfo, theControl, where, partCode)) {
		goto ClearDoubleClick;
		
	} else if((tempViewRect = (**docInfo).viewRect), PtInRect(where, &tempViewRect)) {
	
		(**docInfo).flags.typingUndoCurrent = false;

		/* If the caret was on, turn it off first */
		EraseCaret(docInfo);
		
		PtToSelPosition(docInfo, where, &selAnchor);
		
		/* Get the offset for the click position */
		if((errCode = PositionToOffset(docInfo, &selAnchor)) != noErr) {
			goto DoReturn;
		}
	
		if((**docInfo).flags.isActive) {
			/* Is this click within the double click time and in the same place? */
			if(((**docInfo).lastClickTime + GetDblTime() > theEvent->when) &&
			   (selAnchor.offset == (**docInfo).lastClickOffset) &&
			   !selAnchor.graphicHit) {
				/* Yes, set the double click bit, and triple click bit if double click was set */
				if((**docInfo).flags.doubleClick) {
					(**docInfo).flags.tripleClick = true;
				}
				(**docInfo).flags.doubleClick = true;
			} else {
				(**docInfo).flags.doubleClick = (**docInfo).flags.tripleClick = false;
			}
			
			(**docInfo).lastClickTime = theEvent->when;
			(**docInfo).lastClickOffset = selAnchor.offset;
		} else {
			(**docInfo).flags.doubleClick = (**docInfo).flags.tripleClick = false;
		}
		
		if(((**(**docInfo).globals).flags.hasDragManager) &&
		   !((**docInfo).flags.doubleClick) &&
		   (!((**docInfo).flags.dragOnControlOnly) ||
		    (theEvent->modifiers & controlKey))) {

			if(PtInRgn(where, (**docInfo).hiliteRgn)) {
				if(WaitMouseMoved(theEvent->where)) {
					errCode = HandleDragging(docInfo, theEvent);
				} else if(!(theEvent->modifiers & cmdKey)) {
					goto HandleNormalClick;
				}
		
			} else if(SameGraphic(docInfo, &selAnchor)) {
				graphicInfo = (**docInfo).selectedGraphic;
				goto HandleNewGraphicClick;
			} else if(selAnchor.graphicHit) {
				goto HandleGraphicClick;
			} else {
				goto HandleNormalClick;
			}
		} else {
	HandleNormalClick:
			if((**docInfo).flags.isActive) {
			HandleGraphicClick :
				if(selAnchor.graphicHit) {
					errCode = GetGraphicInfoFromOffset(docInfo, selAnchor.offset - (selAnchor.leadingEdge ? 0L : 1L), &graphicInfo, &graphicLength);
				} else {
					graphicInfo = nil;
				}
				
				if(errCode == noErr) {
					if(((**docInfo).selectedGraphic != nil) && !SameGraphic(docInfo, &selAnchor)) {
						MyCallGraphicHit(docInfo, kPETECurrentSelection, nil);
						(**docInfo).selectedGraphic = nil;
					}
					
					if(selAnchor.graphicHit) {
						(**docInfo).selectedGraphic = graphicInfo;
						graphicInfo = nil;
						(**docInfo).selEnd = (**docInfo).selStart = selAnchor;
						if(selAnchor.leadingEdge) {
							(**docInfo).selEnd.offset += graphicLength;
						} else {
							(**docInfo).selStart.offset -= graphicLength;
						}
						CalcSelectionRgn(docInfo, false);
			HandleNewGraphicClick :
						errCode = MyCallGraphicHit(docInfo, kPETECurrentSelection, theEvent);
						if((errCode != noErr) && (errCode != tsmDocNotActiveErr)) {
							ExchangePtr(graphicInfo, (**docInfo).selectedGraphic);
							CalcSelectionRgn(docInfo, false);
						}
					} else {
						errCode = HandleSelection(docInfo, &selAnchor, where, theEvent->modifiers);
					}
				}
	
			} else {
				errCode = tsmDocNotActiveErr;
			}
		}
	} else {
		if(!((**docInfo).flags.isActive)) {
			errCode = tsmDocNotActiveErr;
		}
ClearDoubleClick :
		(**docInfo).flags.doubleClick = (**docInfo).flags.tripleClick = false;
	}

DoReturn :

	ResetPortInfo(&savedPortInfo);
	
	return errCode;
}

void SetCorrectCursor(DocumentInfoHandle docInfo, Point mouseLoc, RgnHandle curRgn, EventRecord *theEvent)
{
	short cursorID;
	Rect tempRect;
	RgnHandle scratchRgn;
	Boolean couldDrag;
	Cursor	arrow;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	cursorID = 0;
	couldDrag = (((**(**docInfo).globals).flags.hasDragManager) && (!((**docInfo).flags.dragOnControlOnly) || (theEvent->modifiers & controlKey)));
	tempRect = (**docInfo).viewRect;
	if(curRgn != nil) {
		RectRgn(curRgn, &tempRect);
	}
	if(!PtInRect(mouseLoc, &tempRect)) {
		if(curRgn != nil) {
			tempRect = (**docInfo).docRect;
			RectRgn(scratchRgn, &tempRect);
			DiffRgn(scratchRgn, curRgn, curRgn);
		}
	} else if(couldDrag && PtInRgn(mouseLoc, (**docInfo).hiliteRgn)) {
		if(curRgn != nil) {
			CopyRgn((**docInfo).hiliteRgn, curRgn);
		}
	} else if(PtInRgn(mouseLoc, (**docInfo).graphicRgn)) {
		if(curRgn != nil) {
			CopyRgn((**docInfo).graphicRgn, curRgn);
		}
	} else {
		cursorID = iBeamCursor;
		if(curRgn != nil) {
			DiffRgn(curRgn, (**docInfo).graphicRgn, curRgn);
			if(couldDrag) {
				DiffRgn(curRgn, (**docInfo).hiliteRgn, curRgn);
			}
		}
	}
	SetCursor((cursorID == 0) ? GetQDGlobalsArrow(&arrow) : *GetCursor(cursorID));
}

OSErr MyCallProgressLoop(DocumentInfoHandle docInfo)
{
	Boolean global, local;
	OSErr errCode;
	CGrafPtr savedGWorld;
	GDHandle savedGDevice;

	if((**docInfo).callbacks.progressLoop != nil) {
		GetGWorld(&savedGWorld, &savedGDevice);
		SetGWorld(GetWindowPort((**docInfo).docWindow), nil);
		global = (**(**docInfo).globals).flags.progressLoopCalled;
		local = (**docInfo).flags.progressLoopCalled;
		(**(**docInfo).globals).flags.progressLoopCalled = true;
		(**docInfo).flags.progressLoopCalled = true;
		errCode = CallPETEProgressLoopProc((**docInfo).callbacks.progressLoop);
		(**(**docInfo).globals).flags.progressLoopCalled = global;
		(**docInfo).flags.progressLoopCalled = local;
		SetGWorld(savedGWorld, savedGDevice);
	} else {
		errCode = noErr;
	}
	
	return errCode;
}
