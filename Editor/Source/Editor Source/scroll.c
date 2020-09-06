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

void ResetScrollbars(DocumentInfoHandle docInfo)
{
	long winHeight, height, width, winWidth;
#ifdef SHOWHIDESCROLLS
	Rect controlRect;
	PETEPortInfo savedPortInfo;
#endif
	
	if(!(**docInfo).flags.docHeightValid) {
		RecalcDocHeight(docInfo);
		if(!(**docInfo).flags.docHeightValid) {
			(**docInfo).flags.scrollsDirty = true;
			return;
		}
	}
	if((**docInfo).flags.reposition) {
		RepositionDocument(docInfo);
	}
	if((**docInfo).flags.hasHScroll) {
		winWidth = RectWidth(&(**docInfo).viewRect);
		width = GetScrollingWidth(docInfo) - winWidth;
		if(width < 0L) {
			width = 0L;
		}
		DI_MEMCANTFAIL(docInfo);
#if TARGET_CPU_PPC
		if((**(**docInfo).globals).flags.hasControlManager) {
			SetControl32BitMinimum((**docInfo).hScroll, 0L);
			SetControl32BitMaximum((**docInfo).hScroll, width < (**docInfo).hPosition ? (**docInfo).hPosition : width);
			SetControlViewSize((**docInfo).hScroll, winWidth);
		} else
#endif
		{
			SetControlMaximum((**docInfo).hScroll, width < (**docInfo).hPosition ? (**docInfo).hPosition : width);
		}
		DI_MEMCANFAIL(docInfo);
		SetScrollPosition(docInfo, true);
#ifdef SHOWHIDESCROLLS
		DI_MEMCANTFAIL(docInfo);
		GetControlRect((**docInfo).hScroll, &controlRect);
		if((controlRect.right - controlRect.left < kTooSmallForScroll) || !(**docInfo).flags.scrollsVisible) {
			HideControl((**docInfo).hScroll);
			SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
			EraseRect(&controlRect);
			FrameRect(&controlRect);
			ResetPortInfo(&savedPortInfo);
		} else {
			ShowControl((**docInfo).hScroll);
		}
		DI_MEMCANFAIL(docInfo);
#endif
	}

	if((**docInfo).flags.hasVScroll) {
		winHeight = RectHeight(&(**docInfo).viewRect);
		height = (**docInfo).docHeight - winHeight;
		if(height < 0L) {
			height = 0L;
		}
		DI_MEMCANTFAIL(docInfo);
#if TARGET_CPU_PPC
		if((**(**docInfo).globals).flags.hasControlManager) {
			SetControl32BitMinimum((**docInfo).vScroll, 0L);
			SetControl32BitMaximum((**docInfo).vScroll, height < (**docInfo).vPosition ? (**docInfo).vPosition : height);
			SetControlViewSize((**docInfo).vScroll, winHeight);
		} else
#endif
		{
			if(height < (**docInfo).vPosition) {
				height = (**docInfo).vPosition;
			}
			SetControlMaximum((**docInfo).vScroll, height > winHeight ? winHeight : height);
		}
		DI_MEMCANFAIL(docInfo);
		SetScrollPosition(docInfo, false);
#ifdef SHOWHIDESCROLLS
		DI_MEMCANTFAIL(docInfo);
		GetControlRect((**docInfo).vScroll, &controlRect);
		if((controlRect.bottom - controlRect.top < kTooSmallForScroll) || !(**docInfo).flags.scrollsVisible) {
			HideControl((**docInfo).vScroll);
			SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
			EraseRect(&controlRect);
			FrameRect(&controlRect);
			ResetPortInfo(&savedPortInfo);
		} else {
			ShowControl((**docInfo).vScroll);
		}
		DI_MEMCANFAIL(docInfo);
#endif
	}
	(**docInfo).flags.scrollsDirty = false;
}

void ScrollDoc(DocumentInfoHandle docInfo, short dh, short dv)
{
	PETEPortInfo savedPortInfo;
	Rect viewRect;
	RgnHandle clipRgn;
	
	if((dh == 0) && (dv == 0)) {
		return;
	}
	
	
	if(dh != 0) {
		(**docInfo).hPosition += dh;
		SetScrollPosition(docInfo, true);
	}
	
	if(dv != 0) {
		(**docInfo).vPosition += dv;
		SetScrollPosition(docInfo, false);
	}

	clipRgn = NewRgn();
	if(clipRgn != nil) {
		SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		viewRect = (**docInfo).viewRect;
		ScrollRectInDoc(docInfo, &viewRect, -dh, -dv, clipRgn);
		OffsetRgn((**docInfo).hiliteRgn, -dh, -dv);
		if(CalcSelectionRgn(docInfo, true) == noErr) {
			Boolean progressLoopCalled = (**docInfo).flags.progressLoopCalled;
			(**docInfo).flags.progressLoopCalled = true;
			DrawDocument(docInfo, clipRgn, true);
			(**docInfo).flags.progressLoopCalled = progressLoopCalled;
		}
		DisposeRgn(clipRgn);
		ResetPortInfo(&savedPortInfo);
	}
}

void SetDocPosition(DocumentInfoHandle docInfo, long vPosition, short hPosition)
{
	long vOffset;
	short hOffset;
	PETEPortInfo savedPortInfo;

	hOffset = hPosition - (**docInfo).hPosition;
	vOffset = vPosition - (**docInfo).vPosition;
	
	if((hOffset == 0L) && (vOffset == 0L)) {
		return;
	}

	if((vOffset > SHRT_MAX) || (vOffset < SHRT_MIN)) {
		(**docInfo).vPosition += vOffset;
		(**docInfo).hPosition += hOffset;
		SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		if(CalcSelectionRgn(docInfo, true) == noErr) {
			DrawDocument(docInfo, nil, true);
		}
		ResetPortInfo(&savedPortInfo);
	} else {
		ScrollDoc(docInfo, hOffset, vOffset);
	}
}

Boolean TrackScrolls(DocumentInfoHandle docInfo, ControlRef theControl, Point where, short partCode)
{
	Boolean noScrollProc = ((partCode == kControlIndicatorPart) && (!(**(**docInfo).globals).flags.hasAppearanceManager || !(**(**docInfo).globals).flags.useLiveScrolling));
	long saved;
	short rect;
	Boolean vertical;
	unsigned long lastClickTime;
	DECLARE_UPP(ScrollActionProc,ControlAction);
	
	INIT_UPP(ScrollActionProc,ControlAction);
	
	if(!(vertical = (theControl == (**docInfo).vScroll))) {
		if(theControl != (**docInfo).hScroll) {
			return false;
		}
	}
	
	if(vertical) {
		RecalcDocHeight(docInfo);
		saved = (**docInfo).docHeight;
		rect = RectHeight(&(**docInfo).viewRect);
		if((**docInfo).vPosition > (**docInfo).docHeight - rect) {
			(**docInfo).docHeight = (**docInfo).vPosition + rect;
		}
	} else {

		saved = (**docInfo).docWidth;
		rect = RectWidth(&(**docInfo).viewRect);
		(**docInfo).docWidth = GetScrollingWidth(docInfo);
		if((**docInfo).hPosition > (**docInfo).docWidth - rect) {
			(**docInfo).docWidth = (**docInfo).hPosition + rect;
		}
	}

	/* If the caret was on, turn it off first */
	EraseCaret(docInfo);
	
	lastClickTime = (**docInfo).lastClickTime;
	(**docInfo).lastClickTime = 0L;
	
	DI_MEMCANTFAIL(docInfo);
	partCode = TrackControl(theControl, where, noScrollProc ? nil : ScrollActionProcUPP);
	DI_MEMCANFAIL(docInfo);
	
	(**docInfo).lastClickTime = lastClickTime;
	
	if(noScrollProc) {
		SetPositionFromScroll(docInfo, vertical, !vertical);
	}
	
	if(vertical) {
		(**docInfo).docHeight = saved;
	} else {
		(**docInfo).docWidth = saved;
	}
	
	ResetScrollbars(docInfo);
	return true;
}

Fixed GetScrollRatio(ControlRef theControl);
Fixed GetScrollRatio(ControlRef theControl)
{
	short max, value;

	if((max = GetControlMaximum(theControl)) == 0) {
		return -1L;
	} else if((value = GetControlValue(theControl)) == 0) {
		return 0L;
	} else {
		return FixRatio(value, max);
	}
}

/* Set the doc position according to the scroll values */
void SetPositionFromScroll(DocumentInfoHandle docInfo, Boolean vScroll, Boolean hScroll)
{
	long vPosition;
	short hPosition;
	Fixed hRatio, vRatio;
	
	DI_MEMCANTFAIL(docInfo);

#if TARGET_CPU_PPC
	if((**(**docInfo).globals).flags.hasControlManager) {
		vPosition = vScroll ? GetControl32BitValue((**docInfo).vScroll) : (**docInfo).vPosition;
		hPosition = hScroll ? GetControl32BitValue((**docInfo).hScroll) : (**docInfo).hPosition;
	} else
#endif
	{
		vRatio = vScroll ? GetScrollRatio((**docInfo).vScroll) : -1;
		hRatio = hScroll ? GetScrollRatio((**docInfo).hScroll) : -1;
		
		if(vRatio >= 0) {
			vPosition = FixMul((**docInfo).docHeight - RectHeight(&(**docInfo).viewRect), vRatio);
		} else {
			vPosition = (**docInfo).vPosition;
		}
		
		if(hRatio >= 0) {
			hPosition = FixMul((**docInfo).docWidth - RectWidth(&(**docInfo).viewRect), hRatio);
		} else {
			hPosition = (**docInfo).hPosition;
		}
	}
	DI_MEMCANFAIL(docInfo);
	SetDocPosition(docInfo, vPosition, hPosition);
}


/* Set the scroll value according to the doc position */
void SetScrollPosition(DocumentInfoHandle docInfo, Boolean horizontal)
{
	long position, total, screen;
	Fract scrollFraction;
	ControlHandle theControl;
	short max, value;
	
	if(horizontal) {
		if((**docInfo).flags.hasHScroll) {
			theControl = (**docInfo).hScroll;
			position = (**docInfo).hPosition;
			total = GetScrollingWidth(docInfo);
			screen = RectWidth(&(**docInfo).viewRect);
		} else {
			return;
		}
	} else {
		if((**docInfo).flags.hasVScroll) {
			theControl = (**docInfo).vScroll;
			position = (**docInfo).vPosition;
			total = (**docInfo).docHeight;
			screen = RectHeight(&(**docInfo).viewRect);
		} else {
			return;
		}
	}
	
	DI_MEMCANTFAIL(docInfo);
#if TARGET_CPU_PPC
	if((**(**docInfo).globals).flags.hasControlManager) {
		if(position != GetControl32BitValue(theControl)) {
			SetControl32BitValue(theControl, position);
		}
	} else
#endif
	{
		if(total - position < screen) {
			scrollFraction = fract1;
		} else if(position <= 0L) {
			scrollFraction = 0L;
		} else {
			scrollFraction = FracDiv(position, total - screen);
		}
		
		max = GetControlMaximum(theControl);
		value = FixRound(FracMul(Long2Fix(max), scrollFraction));
		if(value != GetControlValue(theControl)) {
			SetControlValue(theControl, value);
		}
	}
	DI_MEMCANFAIL(docInfo);
}

void GetVScrollbarLocation(Rect *docRect, Rect *controlRect, Boolean hasHScroll, Boolean hasGrowBox)
{
	/* Top is 1 pixel underneath the title bar */
	controlRect->top = docRect->top - 1;
	/* Left is the scrollbar width from the right, plus 1 pixel overlap */
	controlRect->left = docRect->right - kScrollbarAdjust;
	/* Bottom is 1 pixel past the bottom of the window */
	controlRect->bottom = docRect->bottom + 1;
	/* If there is a horizontal scrollbar or grow box, subtract from the bottom */
	if(hasHScroll || hasGrowBox) {
		controlRect->bottom -= kScrollbarAdjust;
	}
	/* Right is 1 pixel past the right side of the window */
	controlRect->right = docRect->right + 1;
}

void GetHScrollbarLocation(Rect *docRect, Rect *controlRect, Boolean hasVScroll, Boolean hasGrowBox)
{
	/* Top is the scrollbar width from the bottom, plus 1 pixel overlap */
	controlRect->top = docRect->bottom - kScrollbarAdjust;
	/* Left is 1 pixel before the left side of the window */
	controlRect->left = docRect->left - 1;
	/* Bottom is 1 pixel past the bottom of the window */
	controlRect->bottom = docRect->bottom + 1;
	/* Right is 1 pixel past the right side of the window */
	controlRect->right = docRect->right + 1;
	/* If there is a vertical scrollbar or grow box, subtract from the right */
	if(hasVScroll || hasGrowBox) {
		controlRect->right -= kScrollbarAdjust;
	}
}

/* Return whether docWidth or the longest line determines the scrollbar width */
short GetScrollingWidth(DocumentInfoHandle docInfo)
{
	if((**docInfo).flags.useHLineWidth) {
		if((**docInfo).textLen == 0L) {
			(**docInfo).longLineWidth = 0L;
		}
		return (**docInfo).longLineWidth;
	} else {
		return (**docInfo).docWidth;
	}
}

/* Move the document if there is extra space */
void RepositionDocument(DocumentInfoHandle docInfo)
{
	short rightSpace;
	long bottomSpace;
	Rect viewRect;
	
	if(!(**docInfo).flags.docHeightValid) {
		RecalcDocHeight(docInfo);
	}
	if((**docInfo).flags.recalcOff || !(**docInfo).flags.docHeightValid) {
		(**docInfo).flags.reposition = true;
		return;
	}
	
	(**docInfo).flags.reposition = false;
	
	viewRect = (**docInfo).viewRect;

	bottomSpace = (**docInfo).docHeight - (**docInfo).vPosition - RectHeight(&viewRect);
	if((bottomSpace >= 0L) || ((**docInfo).vPosition <= 0)) {
		bottomSpace = 0L;
	} else if(-bottomSpace > (**docInfo).vPosition) {
		bottomSpace = -(**docInfo).vPosition;
	}
	
	rightSpace = GetScrollingWidth(docInfo) - (**docInfo).hPosition - RectWidth(&viewRect);
	if((rightSpace >= 0L) || ((**docInfo).hPosition <= 0)) {
		rightSpace = 0L;
	} else if(-rightSpace > (**docInfo).hPosition) {
		rightSpace = -(**docInfo).hPosition;
	}
	
	if((rightSpace != 0L) || (bottomSpace != 0)) {
		PETEPortInfo savedPortInfo;

		(**docInfo).hPosition += rightSpace;
		(**docInfo).vPosition += bottomSpace;
		SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		InvalWindowRect((**docInfo).docWindow,&viewRect);
		ResetPortInfo(&savedPortInfo);
		(**docInfo).flags.scrollsDirty = true;
	}
}

/* Change the size of the viewing and scrollbar rectangle; no margin change */
void ResizeDocRect(DocumentInfoHandle docInfo, short h, short v)
{
	PETEPortInfo savedPortInfo;
	Rect docRect, controlRect;
	RgnHandle oldRgn, scratchRgn;
	short dh, dv;
	
	dh = h - RectWidth(&(**docInfo).docRect);
	dv = v - RectHeight(&(**docInfo).docRect);
	if((dh == 0) && (dv == 0)) {
		return;
	}
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	oldRgn = NewRgn();
	if(oldRgn == nil) {
		return;
	}
	EraseCaret(docInfo);
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	SetRectRgn(oldRgn, (**docInfo).viewRect.left, (**docInfo).viewRect.top, (**docInfo).viewRect.right, (**docInfo).viewRect.bottom);
	(**docInfo).viewRect.right += dh;
	(**docInfo).viewRect.bottom += dv;
	(**docInfo).docRect.right += dh;
	(**docInfo).docRect.bottom += dv;
	SetRectRgn(scratchRgn, (**docInfo).viewRect.left, (**docInfo).viewRect.top, (**docInfo).viewRect.right, (**docInfo).viewRect.bottom);
	DiffRgn(scratchRgn, oldRgn, oldRgn);
	InvalWindowRgn((**docInfo).docWindow,oldRgn);
	SetRectRgn(scratchRgn, (**docInfo).docRect.left, (**docInfo).docRect.top, (**docInfo).docRect.right, (**docInfo).docRect.bottom);
	DiffRgn(scratchRgn, oldRgn, oldRgn);
	InvalWindowRgn((**docInfo).docWindow,oldRgn);
	DisposeRgn(oldRgn);
	
	docRect = (**docInfo).docRect;

	DI_MEMCANTFAIL(docInfo);
	if((**docInfo).flags.hasVScroll) {
		GetVScrollbarLocation(&docRect, &controlRect, (**docInfo).flags.hasHScroll, (**docInfo).flags.hasGrowBox);
#ifdef SHOWHIDESCROLLS
		if((controlRect.bottom - controlRect.top >= kTooSmallForScroll) && (**docInfo).flags.scrollsVisible) {
			ShowControl((**docInfo).vScroll);
		} else {
			HideControl((**docInfo).vScroll);
		}
#endif
		SizeControl((**docInfo).vScroll, controlRect.right - controlRect.left, controlRect.bottom - controlRect.top);
		MoveControl((**docInfo).vScroll, controlRect.left, controlRect.top);
	}
	if((**docInfo).flags.hasHScroll) {
		GetHScrollbarLocation(&docRect, &controlRect, (**docInfo).flags.hasVScroll, (**docInfo).flags.hasGrowBox);
#ifdef SHOWHIDESCROLLS
		if((controlRect.right - controlRect.left >= kTooSmallForScroll) && (**docInfo).flags.scrollsVisible) {
			ShowControl((**docInfo).hScroll);
		} else {
			HideControl((**docInfo).hScroll);
		}
#endif
		SizeControl((**docInfo).hScroll, controlRect.right - controlRect.left, controlRect.bottom - controlRect.top);
		MoveControl((**docInfo).hScroll, controlRect.left, controlRect.top);
	}
	DI_MEMCANFAIL(docInfo);
	CalcSelectionRgn(docInfo, true);
	ResetScrollbars(docInfo);
	ResetPortInfo(&savedPortInfo);
}

/* Move the document rectangle to another position in the window */
void MoveDocRect(DocumentInfoHandle docInfo, short h, short v)
{
	PETEPortInfo savedPortInfo;
	Rect tempRect, controlRect;
	short dh, dv;
	
	tempRect = (**docInfo).docRect;
	dh = h - tempRect.left;
	dv = v - tempRect.top;
	if((dh == 0) && (dv == 0)) {
		return;
	}
	
	EraseCaret(docInfo);
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	InvalWindowRect((**docInfo).docWindow,&tempRect);
	OffsetRect(&tempRect, dh, dv);
	InvalWindowRect((**docInfo).docWindow,&tempRect);
	(**docInfo).docRect = tempRect;
	tempRect = (**docInfo).viewRect;
	OffsetRect(&tempRect, dh, dv);
	(**docInfo).viewRect = tempRect;
	DI_MEMCANTFAIL(docInfo);
	if((**docInfo).flags.hasVScroll) {
		GetVScrollbarLocation(&(**docInfo).docRect, &controlRect, (**docInfo).flags.hasHScroll, (**docInfo).flags.hasGrowBox);
		MoveControl((**docInfo).vScroll, controlRect.left, controlRect.top);
	}
	if((**docInfo).flags.hasHScroll) {
		GetHScrollbarLocation(&(**docInfo).docRect, &controlRect, (**docInfo).flags.hasVScroll, (**docInfo).flags.hasGrowBox);
		MoveControl((**docInfo).hScroll, controlRect.left, controlRect.top);
	}
	DI_MEMCANFAIL(docInfo);
	CalcSelectionRgn(docInfo, true);
	ResetPortInfo(&savedPortInfo);
}

void ChangeDocWidth(DocumentInfoHandle docInfo, short docWidth, Boolean pinMargins)
{
	short widthDiff;
	long paraIndex;
	ParagraphInfoPtr paraInfo;
	
	if(docWidth < 0L) {
		docWidth = RectWidth(&(**docInfo).viewRect);
	}
	widthDiff = docWidth - (**docInfo).docWidth;
	if(widthDiff != 0L) {
		(**docInfo).docWidth = docWidth;
		FlushDocInfoLineCache(docInfo);
		if(pinMargins) {
			for(paraIndex = 0L; paraIndex < (**docInfo).paraCount; ++paraIndex) {
				paraInfo = *((**docInfo).paraArray[paraIndex].paraInfo);
				if(paraInfo->direction == leftCaret) {
					if(paraInfo->endMargin > 0L) {
						paraInfo->endMargin += widthDiff;
					}
				} else {
					paraInfo->startMargin -= widthDiff;
				}
				SetParaDirty(docInfo, paraIndex);
			}
			RecalcDocHeight(docInfo);
		}
	
		SetEmptyRgn((**docInfo).hiliteRgn);
		InvalidateDocument(docInfo, true);
		(**docInfo).flags.selectionDirty = (**docInfo).flags.scrollsDirty = true;
		ResetScrollbars(docInfo);
	}
}

pascal void ScrollActionProc(ControlRef theControl, short partCode)
{
	DocumentInfoHandle docInfo;
	Boolean horizontal;
	short amount;
	long tickDiff;
	unsigned long tickCount;
	
	docInfo = (DocumentInfoHandle)GetControlReference(theControl);
	horizontal = (theControl == (**docInfo).hScroll);
	
	switch(partCode) {
		case kControlIndicatorPart :
			SetPositionFromScroll(docInfo, !horizontal, horizontal);
			return;
		case kControlPageUpPart :
			amount = psePageUp;
			break;
		case kControlPageDownPart :
			amount = psePageDn;
			break;
		case kControlUpButtonPart :
			amount = pseLineUp;
			break;
		case kControlDownButtonPart :
			amount = pseLineDn;
			break;
		default :
			amount = pseNoScroll;
	}
	
	tickCount = TickCount();
	tickDiff = (**(**docInfo).globals).autoScrollTicks - (tickCount - (**docInfo).lastClickTime);
	if(tickDiff > 0L) {
		Delay(tickDiff, &tickCount);
	}
	(**docInfo).lastClickTime = tickCount;

	DoScroll(docInfo, horizontal ? amount : pseNoScroll, horizontal ? pseNoScroll : amount);
}

void GetSelectionVisibleDistance(DocumentInfoHandle docInfo, SelDataPtr selection, Rect *viewRect, long *vDistance, short *hDistance)
{
	long vPosition;
	short hPosition;
	Rect tempViewRect;

	if(viewRect == nil) {
		tempViewRect = (**docInfo).viewRect;
		viewRect = &tempViewRect;
	}

	vPosition = (**docInfo).vPosition + (viewRect->top - (**docInfo).viewRect.top);
	if(selection->vLineHeight > RectHeight(viewRect)) {
		vPosition = (selection->vPosition + (selection->lastLine ? selection->vLineHeight - RectHeight(viewRect) : 0)) - vPosition;
//		vPosition = selection->vPosition + (RectHeight(viewRect) / 2) - (selection->vLineHeight / 2) - vPosition;
	} else {
		if(selection->vPosition < vPosition) {
			vPosition = selection->vPosition - vPosition;
		} else {
			vPosition += (RectHeight(viewRect) - selection->vLineHeight);
			if(selection->vPosition > vPosition) {
				vPosition = selection->vPosition - vPosition;
			} else {
				vPosition = 0L;
			}
		}
	}
	
	if((viewRect->top != (**docInfo).viewRect.top) || (viewRect->bottom != (**docInfo).viewRect.bottom)) {
		if(vPosition < 0L) {
			if((**docInfo).vPosition + vPosition < 0L) {
				if((**docInfo).vPosition >= 0L) {
					vPosition = -(**docInfo).vPosition;
				} else {
					vPosition = 0L;
				}
			}
		} else {
			if((**docInfo).vPosition + vPosition + RectHeight(&(**docInfo).viewRect) > (**docInfo).docHeight) {
				if((**docInfo).vPosition + RectHeight(&(**docInfo).viewRect) < (**docInfo).docHeight) {
					vPosition = (**docInfo).docHeight - (**docInfo).vPosition + RectHeight(&(**docInfo).viewRect);
				} else {
					vPosition = 0L;
				}
			}
		}
	}
	
	hPosition = (**docInfo).hPosition + (viewRect->left - (**docInfo).viewRect.left);
	if(selection->lPosition < (**docInfo).hPosition) {
		hPosition = selection->lPosition - hPosition;
	} else {
		hPosition += RectWidth(viewRect);
		if(selection->rPosition > hPosition) {
			hPosition = selection->rPosition - hPosition;
		} else {
			hPosition = 0L;
		}
	}

	if((viewRect->left != (**docInfo).viewRect.left) || (viewRect->right != (**docInfo).viewRect.right)) {
		if(hPosition < 0L) {
			if((**docInfo).hPosition + hPosition < 0L) {
				if((**docInfo).hPosition > 0L) {
					hPosition = -(**docInfo).hPosition;
				} else {
					hPosition = 0L;
				}
			}
		} else {
			if((**docInfo).hPosition + hPosition + RectWidth(&(**docInfo).viewRect) > GetScrollingWidth(docInfo)) {
				if((**docInfo).hPosition + RectWidth(&(**docInfo).viewRect) < GetScrollingWidth(docInfo)) {
					hPosition = GetScrollingWidth(docInfo) - (**docInfo).hPosition + RectWidth(&(**docInfo).viewRect);
				} else {
					hPosition = 0L;
				}
			}
		}
	}
	*vDistance = vPosition;
	*hDistance = hPosition;
}

void MakeSelectionVisible(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	long vPosition;
	short hPosition;
	Rect viewRect;
	PETEPortInfo savedPortInfo;

	GetSelectionVisibleDistance(docInfo, selection, nil, &vPosition, &hPosition);

	if((vPosition > SHRT_MAX) || (vPosition < SHRT_MIN)) {
		(**docInfo).vPosition += vPosition;
		(**docInfo).hPosition += hPosition;
		SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
		CalcSelectionRgn(docInfo, true);
		viewRect = (**docInfo).viewRect;
		InvalWindowRect((**docInfo).docWindow,&viewRect);
		ResetPortInfo(&savedPortInfo);
	} else {
		ScrollDoc(docInfo, hPosition, vPosition);
	}
	if((vPosition != 0L) || (hPosition != 0L)) {
		(**docInfo).flags.reposition = true;
		ResetScrollbars(docInfo);
	}
}

Boolean ShouldCenterSelection(DocumentInfoHandle docInfo, Boolean force)
{
	if(!force && ((**docInfo).selStart.vPosition >= (**docInfo).vPosition) && ((**docInfo).selEnd.vPosition + (**docInfo).selEnd.vLineHeight <= (**docInfo).vPosition + RectHeight(&(**docInfo).viewRect))) {
		return false;
	}
	if((**docInfo).selStart.vPosition >= (**docInfo).vPosition + RectHeight(&(**docInfo).viewRect)) {
		return true;
	}
	if((**docInfo).selEnd.vPosition + (**docInfo).selEnd.vLineHeight <= (**docInfo).vPosition) {
		return true;
	}
	return(((**docInfo).selEnd.vPosition + (**docInfo).selEnd.vLineHeight - (**docInfo).selStart.vPosition) < RectHeight(&(**docInfo).viewRect));
}

OSErr DoScroll(DocumentInfoHandle docInfo, short horizontal, short vertical)
{
	SelData selection;
	long vPosition, vMax;
	short hPosition, distance, hMax;
	OSErr vErrCode, hErrCode;
	Boolean doHorizontal, doVertical;
	
	vErrCode = hErrCode = noErr;
	if((horizontal == pseSelection) || (vertical == pseSelection)) {
		selection = (**docInfo).selStart;
		MakeSelectionVisible(docInfo, &selection);
	}

	vPosition = (**docInfo).vPosition;
	hPosition = (**docInfo).hPosition;
	
	vMax = (**docInfo).docHeight - RectHeight(&(**docInfo).viewRect);
	if(vMax < vPosition) {
		vMax = vPosition;
	}
	hMax = GetScrollingWidth(docInfo) - RectWidth(&(**docInfo).viewRect);
	if(hMax < (**docInfo).hPosition) {
		hMax = (**docInfo).hPosition;
	}
	
	switch(vertical) {
		case psePageUp :
		case psePageDn :
			if(vMax > vPosition) {
				vMax = GetDocHeight(docInfo) - (**docInfo).shortLineHeight;
				distance = vMax;
			}
			distance = RectHeight(&(**docInfo).viewRect) - (**docInfo).shortLineHeight;
			goto AddVerticalDistance;
		case pseLineUp :
		case pseLineDn :
			distance = (**docInfo).shortLineHeight;
	AddVerticalDistance:
			if((vertical == pseLineUp) || (vertical == psePageUp)) {
				distance = -distance;
			}
			vPosition += distance;
		case pseSelection :
		case pseNoScroll :
			break;
		case pseCenterSelection :
		case pseForceCenterSelection : 
			if(ShouldCenterSelection(docInfo, (vertical == pseForceCenterSelection))) {
				vPosition = (**docInfo).selStart.vPosition;
				distance = (RectHeight(&(**docInfo).viewRect) / 2) - ((**docInfo).selStart.vLineHeight / 2);
				vPosition -= distance;
			}
			break;
		default :
			if(vertical >= 0) {
				vPosition = FixMul(GetDocHeight(docInfo) - RectHeight(&(**docInfo).viewRect), FixRatio(vertical, SHRT_MAX));
			} else {
				vErrCode = paramErr;
			}
	}
	
	if(vErrCode == noErr) {
		if(vPosition < 0L) {
			if((**docInfo).vPosition > 0) {
				vPosition = 0L;
			} else {
				vPosition = (**docInfo).vPosition;
				vErrCode = errTopOfDocument;
			}
		} else if(vMax <= vPosition) {
			vPosition = vMax;
			if(vMax <= (**docInfo).vPosition) {
				vErrCode = errEndOfDocument;
			}
		}
	}
	
	switch(horizontal) {
		case psePageUp :
		case psePageDn :
			distance = RectWidth(&(**docInfo).viewRect);
			goto AddHorizontalDistance;
		case pseLineUp :
		case pseLineDn :
			distance = 1;
	AddHorizontalDistance :
			if((horizontal == pseLineUp) || (horizontal == psePageUp)) {
				distance = -distance;
			}
			hPosition += distance;
		case pseSelection :
		case pseNoScroll :
		case pseCenterSelection :
			break;
		default :
			if(horizontal >= 0) {
				hPosition = FixMul(GetScrollingWidth(docInfo) - RectWidth(&(**docInfo).viewRect), FixRatio(horizontal, SHRT_MAX));
			} else {
				hErrCode = paramErr;
			}
	}
	
	if(hErrCode == noErr) {
		if(hPosition < 0L) {
			if((**docInfo).hPosition > 0L) {
				hPosition = 0L;
			} else {
				hPosition = (**docInfo).hPosition;
				hErrCode = errTopOfDocument;
			}
		} else if(hPosition > hMax) {
			if(hMax > (**docInfo).hPosition) {
				hPosition = hMax;
			} else {
				hErrCode = errEndOfDocument;
			}
		}
	}
		
	doHorizontal = ((hErrCode == noErr) && (hPosition != (**docInfo).hPosition));
	doVertical = ((vErrCode == noErr) && ((**docInfo).vPosition != vPosition));
	
	if(doHorizontal || doVertical) {
		SetDocPosition(docInfo, vPosition, hPosition);
		ResetScrollbars(docInfo);
	}
	
	if(vErrCode != noErr) {
		return vErrCode;
	} else {
		return hErrCode;
	}
}

void ScrollRectInDoc(DocumentInfoHandle docInfo, Rect *scrollRect, short dh, short dv, RgnHandle scrollRgn)
{
	RgnHandle scratchRgn, graphicRgn;
	RGBColor oldColor, newColor;
	Boolean colorChange = false;
	
	newColor = DocOrGlobalBGColor(nil, docInfo);
	if(!IsPETEDefaultColor(newColor)) {
		colorChange = true;
		GetBackColor(&oldColor);
		RGBBackColorAndPat(&newColor);
	}
	graphicRgn = (**docInfo).graphicRgn;
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	RectRgn(scrollRgn, scrollRect);
	DiffRgn(graphicRgn, scrollRgn, scratchRgn);
	OffsetRgn(graphicRgn, dh, dv);
	SectRgn(scrollRgn, graphicRgn, graphicRgn);
	UnionRgn(scratchRgn, graphicRgn, graphicRgn);
	GetWindowUpdateRgn((**docInfo).docWindow, scratchRgn);
	GlobalToLocalRgn(scratchRgn);
	SectRgn(scrollRgn, scratchRgn, scratchRgn);
	OffsetRgn(scratchRgn, dh, dv);
	ScrollRect(scrollRect, dh, dv, scrollRgn);
	UnionRgn(scratchRgn, scrollRgn, scrollRgn);
	if(colorChange) {
		RGBBackColorAndPat(&oldColor);
	}
}