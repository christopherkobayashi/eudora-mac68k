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

#define DRAGMANAGERHASACLUE

pascal OSErr SendDragDataProc(FlavorType theType, void *dragSendRefCon, ItemReference theItemRef, DragReference theDragRef);
void PictDispose(PETEStyleEntryPtr styleEntry);

OSErr HandleDragging(DocumentInfoHandle docInfo, EventRecord *theEvent)
{
	OSErr errCode;
	Rect viewRect;
	Point tempPt, mouseLoc;
	DragReference dragRef;
	RgnHandle scratchRgn;
	WindowRef dropWindow;
	long start, end;
	DECLARE_UPP(SendDragDataProc,DragSendData);
	
	INIT_UPP(SendDragDataProc,DragSendData);	
	if((errCode = NewDrag(&dragRef)) != noErr)
		return errCode;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	CopyRgn((**docInfo).hiliteRgn, scratchRgn);
	InsetRgn(scratchRgn, 1, 1);
	DiffRgn((**docInfo).hiliteRgn, scratchRgn, scratchRgn);
	LocalToGlobalRgn(scratchRgn);

	if((**docInfo).callbacks.setDragContents != nil) {
		errCode = CallPETESetDragContentsProc((**docInfo).callbacks.setDragContents, docInfo, dragRef);
	} else {
		errCode = handlerNotFoundErr;
	}
	
	if(errCode == handlerNotFoundErr) {
		errCode = AddDragItemFlavor(dragRef, 1L, flavorTypeText, nil, (**docInfo).selEnd.offset - (**docInfo).selStart.offset, 0);
		if(errCode == noErr) {
			if((**docInfo).selStart.paraIndex != (**docInfo).selEnd.paraIndex) {
				errCode = AddDragItemFlavor(dragRef, 1L, kPETEParaFlavor, nil, 0L, 0);
			}
			if(errCode == noErr) {
				errCode = AddDragItemFlavor(dragRef, 1L, kPETEStyleFlavor, nil, 0L, flavorNotSaved);
				if((errCode == noErr) && HasTEStyles(docInfo, (**docInfo).selStart.offset, (**docInfo).selEnd.offset, !(!(theEvent->modifiers & shiftKey)))) {
					errCode = AddDragItemFlavor(dragRef, 1L, flavorTypeStyle, nil, 0L, 0);
				}
			}
		}
		
		if(errCode == noErr) {
			errCode = SetDragSendProc(dragRef, SendDragDataProcUPP, docInfo);
		}
	}
	
	if(errCode == noErr) {
		(**docInfo).flags.dragActive = true;
		errCode = TrackDrag(dragRef, theEvent, scratchRgn);
		(**docInfo).flags.dragActive = false;
		(**docInfo).flags.dragHasLeft = false;
		if((errCode == noErr) && DragWasToTrash(dragRef)) {
			errCode = GetIntelligentCutBoundaries(docInfo, &start, &end);
			errCode = SetDeleteUndo(docInfo, start, end, peUndoDrag);
			if(errCode == noErr) {
				errCode = DeleteText(docInfo, start, end);
			}
			if((errCode == errAENotModifiable) || (errCode == errAEImpossibleRange)) {
				errCode = noErr;
			}
		} else if(errCode == userCanceledErr) {
			if(GetDragMouse(dragRef, &tempPt, &mouseLoc) == noErr) {
				if(FindWindow(mouseLoc, &dropWindow) > inMenuBar) {
					if(dropWindow == (**docInfo).docWindow) {
						GlobalToLocal(&mouseLoc);
						viewRect = (**docInfo).viewRect;
						if(PtInRect(mouseLoc, &viewRect)) {
							theEvent->what = nullEvent;
							errCode = tsmDocNotActiveErr;
						}
					}
				}
			}
		}
	}

	DisposeDrag(dragRef);
	return errCode;
}

pascal OSErr SendDragDataProc(FlavorType theType, void *dragSendRefCon, ItemReference theItemRef, DragReference theDragRef)
{
	DocumentInfoHandle docInfo;
	StringHandle theText=nil;
	long startOffset, endOffset, handleSize;
	short modifiers, mouseDownModifiers, mouseUpModifiers;
	PETEStyleListHandle styleScrap=nil, tempScrap;
	PETEStyleList stylePtr;
	StScrpHandle teScrap;
	PETEParaScrapHandle paraScrap=nil;
	OSErr errCode, tempMemErr;
	
	docInfo = (DocumentInfoHandle)dragSendRefCon;
	startOffset = (**docInfo).selStart.offset;
	endOffset = (**docInfo).selEnd.offset;
	
	errCode = GetTextStyleScrap(docInfo, startOffset, endOffset, (Handle *)&theText, &styleScrap, &paraScrap, false, true, true, true, true);
	if (!errCode)
	{
		switch(theType) {
			case flavorTypeText :
				if(theText) {
					HLock((Handle)theText);
					errCode = SetDragItemFlavorData(theDragRef, theItemRef, flavorTypeText, *theText, InlineGetHandleSize((Handle)theText), 0L);
				}
				break;

			case kPETEParaFlavor :
				if(paraScrap) {
					HLock((Handle)paraScrap);
					errCode = SetDragItemFlavorData(theDragRef, theItemRef, kPETEParaFlavor, *paraScrap, InlineGetHandleSize((Handle)paraScrap), 0L);
				}
				break;
				
			case kPETEStyleFlavor :
			case flavorTypeStyle :
				GetDragModifiers(theDragRef, &modifiers, &mouseDownModifiers, &mouseUpModifiers);
				if(styleScrap) {
					handleSize = InlineGetHandleSize((Handle)styleScrap);
					for(stylePtr = *styleScrap + (handleSize / sizeof(PETEStyleEntry)); stylePtr != *styleScrap; ) {
						(--stylePtr)->psStyle.textStyle.tsLock = peNoLock;
					}
					
					if(mouseDownModifiers & shiftKey) {
						errCode = MakePlainStyles(styleScrap, &tempScrap, true, false, true);
						if((errCode == noErr) && (tempScrap != nil)) {
							DisposeHandle((Handle)styleScrap);
							styleScrap = tempScrap;
						}
					}
					
					if (theType == kPETEStyleFlavor)
					{
						if(errCode == noErr) {
							HLock((Handle)styleScrap);
							errCode = SetDragItemFlavorData(theDragRef, theItemRef, kPETEStyleFlavor, *styleScrap, InlineGetHandleSize((Handle)styleScrap), 0L);
							HUnlock((Handle)styleScrap);
						}
					}
					else
					{
						if(errCode == noErr) {
							teScrap = (StScrpHandle)MyNewHandle(sizeof(short), &tempMemErr, hndlTemp);
							if((errCode = tempMemErr) == noErr) {
								errCode = PETEStyleToTEScrap(nil, docInfo, styleScrap, &teScrap);
							}
						} else {
							tempMemErr = badDragFlavorErr;
						}
						if((tempMemErr == noErr) && (errCode == noErr)) {
							HLock((Handle)teScrap);
							errCode = SetDragItemFlavorData(theDragRef, theItemRef, flavorTypeStyle, *teScrap, InlineGetHandleSize((Handle)teScrap), 0L);
						}
						if(tempMemErr == noErr) {
							DisposeHandle((Handle)teScrap);
						}
						if(errCode != noErr) {
							DisposeScrapGraphics(styleScrap, 0L, -1L, false);
						}
					}
				}
				break;
			default :
				errCode = cantGetFlavorErr;
		}
	}
	
	if (theText) DisposeHandle((Handle)theText);
	if (paraScrap) DisposeHandle((Handle)paraScrap);
	if (styleScrap) DisposeHandle((Handle)styleScrap);

	return errCode;
}

RgnHandle GetDragAutoscrollRgn(DocumentInfoHandle docInfo)
{
	Rect docRect, tempRect;
	RgnHandle autoScrollRgn, scratchRgn;
	
	docRect = (**docInfo).docRect;
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	autoScrollRgn = NewRgn();
	if(autoScrollRgn != nil) {
		tempRect = docRect;
		InsetRect(&tempRect, -1, -1);
		tempRect.right = tempRect.left + kScrollbarAdjust;
		RectAndRgn(autoScrollRgn, scratchRgn, &tempRect);
		tempRect = docRect;
		InsetRect(&tempRect, -1, -1);
		tempRect.bottom = tempRect.top + kScrollbarAdjust;
		RectAndRgn(autoScrollRgn, scratchRgn, &tempRect);
		GetVScrollbarLocation(&docRect, &tempRect, false, false);
		RectAndRgn(autoScrollRgn, scratchRgn, &tempRect);
		GetHScrollbarLocation(&docRect, &tempRect, false, false);
		RectAndRgn(autoScrollRgn, scratchRgn, &tempRect);
		
		tempRect = (**docInfo).viewRect;
		RectRgn(scratchRgn, &tempRect);
		DiffRgn(scratchRgn, autoScrollRgn, scratchRgn);
		if(EmptyRgn(scratchRgn)) {
			SetEmptyRgn(autoScrollRgn);
		}
	}
	return autoScrollRgn;
}

long ClosestValue(long target, long source1, long source2)
{
	long diff1, diff2;
	
	if((diff1 = target - source1) < 0L) {
		diff1 = -diff1;
	}
	if((diff2 = target - source2) < 0L) {
		diff2 = -diff2;
	}
	
	return (diff1 < diff2) ? source1 : source2;
}

OSErr DragTextOnly(DragReference theDragRef)
{
	OSErr errCode;
	FlavorFlags flags;
	unsigned short numItems;
	ItemReference itemRef;
	
	errCode = CountDragItems(theDragRef, &numItems);
	if(errCode != noErr) {
		return errCode;
	}
	
	do {
		errCode = GetDragItemReferenceNumber(theDragRef, numItems, &itemRef);
		if(errCode == noErr) {
			errCode = GetFlavorFlags(theDragRef, itemRef, flavorTypeText, &flags);
			if(errCode == badDragFlavorErr) {
				errCode = GetFlavorFlags(theDragRef, itemRef, kPETEA822Flavor, &flags);
			}
		}
	
	} while(errCode == noErr && --numItems > 0);

	return errCode;
}

OSErr CheckDragAcceptable(DocumentInfoHandle docInfo, Boolean dragTextOnly)
{
	long index;
	LongSTElement tempStyle;
	
	if(!((**docInfo).flags.ignoreSelectLock)) {
		index = NumStyles((**docInfo).theStyleTable);
		while(--index >= 0L) {
			GetStyle(&tempStyle, (**docInfo).theStyleTable, index);
			if((tempStyle.stCount > 0L) && ((tempStyle.stInfo.textStyle.tsLock & (peModLock | peClickAfterLock | peClickBeforeLock)) != peModLock)) {
				break;
			}
		}
		if(index < 0L) {
			return dragNotAcceptedErr;
		}
	}
	
	if(dragTextOnly) {
		return noErr;
	}
	
	for(index = (**docInfo).paraCount; --index >= 0L; ) {
		if(!((**(**docInfo).paraArray[index].paraInfo).paraFlags & peTextOnly)) {
			return noErr;
		}
	}
	
	return dragNotAcceptedErr;
}

pascal OSErr MyTrackingHandler(DragTrackingMessage message, WindowPtr theWindow, void *handlerRefCon, DragReference theDragRef)
{
#pragma unused(theWindow)
	DocumentInfoHandle docInfo;
	DragDataHandle dragData;
	DragAttributes dragAttr;
	RgnHandle tempRgn, scratchRgn;
	Rect viewRect, docRect;
	Point dragMouse, tempPt;
	OSErr errCode, tempErr;
	PETEPortInfo savedInfo;
	unsigned long currentTicks, caretTime;
	SelData oldSelection, selection;
	long vDistance;
	short hDistance;
	short sdh, sdv;
	
	errCode = GetDragMouse(theDragRef, &dragMouse, &tempPt);
	if(errCode != noErr) {
		return errCode;
	}
	
	errCode = GetDragAttributes(theDragRef, &dragAttr);
	if(errCode != noErr) {
		return errCode;
	}
	
	docInfo = (DocumentInfoHandle)handlerRefCon;
	dragData = (**docInfo).dragData;
	viewRect = (**docInfo).viewRect;
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	currentTicks = TickCount();

	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedInfo);
	GlobalToLocal(&dragMouse);
	
	switch(message) {
		case kDragTrackingEnterHandler :
		case kDragTrackingLeaveHandler :
		default :
			break;
		case kDragTrackingEnterWindow :
			dragData = (DragDataHandle)MyNewHandle(sizeof(DragData), &tempErr, hndlClear);
			(**docInfo).dragData = dragData;
			if((errCode = tempErr) != noErr) {
				break;
			}
			
			errCode = DragTextOnly(theDragRef);
			(**dragData).dragTextOnly = (errCode != badDragFlavorErr);
			if((errCode != noErr) && (errCode != badDragFlavorErr)) {
				goto DisposeAndReturn;
			}
			
			errCode = CheckDragAcceptable(docInfo, (**dragData).dragTextOnly);
			(**dragData).dragAcceptable = (errCode != dragNotAcceptedErr);
			if((errCode != noErr) && (errCode != dragNotAcceptedErr)) {
				goto DisposeAndReturn;
			}
			
			errCode = noErr;
			
			tempRgn = GetDragAutoscrollRgn(docInfo);
			(**dragData).autoScrollRgn = tempRgn;
			if(tempRgn == nil) {
				errCode = memFullErr;
				goto DisposeAndReturn;
			}
			
		case kDragTrackingInWindow :
			if(dragData == nil) {
				errCode = memFullErr;
				break;
			}
			
			if(!(**dragData).dragAcceptable) {
				errCode = dragNotAcceptedErr;
				break;
			}
			
			oldSelection = (**dragData).caretLocation;
			
			if(PtInRect(dragMouse, &viewRect)) {
				if(!(**dragData).isHilited && ((dragAttr & kDragHasLeftSenderWindow) || ((**docInfo).flags.dragHasLeft) || !((**docInfo).flags.dragActive))) {
					RectRgn(scratchRgn, &viewRect);
					errCode = ShowDragHilite(theDragRef, scratchRgn, true);
					if(errCode == noErr) {
						(**dragData).isHilited = true;
					}
				}
			} else {
				if((**dragData).caretOn) {
					DrawCaret(docInfo, &oldSelection);
					(**dragData).caretOn = false;
				}
				if((**dragData).isHilited) {
					errCode = HideDragHilite(theDragRef);
					if(errCode == noErr) {
						(**dragData).isHilited = false;
					}
				}
			}
			
			if(errCode != noErr) {
				break;
			}
			
			EraseCaret(docInfo);
			
			if(PtInRgn(dragMouse, (**dragData).autoScrollRgn)) {
				if((**dragData).timeEnteredAutoscroll == 0L) {
					(**dragData).timeEnteredAutoscroll = currentTicks;
				}
			} else {
				(**dragData).timeEnteredAutoscroll = 0L;
			}
			
			PtToSelPosition(docInfo, dragMouse, &selection);

			if(((**dragData).timeEnteredAutoscroll != 0L) &&
			   ((**dragData).timeEnteredAutoscroll + 10L < currentTicks) &&
			   ((**docInfo).flags.isActive)) {
			   
				docRect = (**docInfo).docRect;
				RectRgn(scratchRgn, &docRect);
				DiffRgn(scratchRgn, (**dragData).autoScrollRgn, scratchRgn);
				GetRegionBounds(scratchRgn,&docRect);
				GetSelectionVisibleDistance(docInfo, &selection, &docRect, &vDistance, &hDistance);
				
				if(vDistance + (**docInfo).vPosition > (**docInfo).docHeight - (**docInfo).extraHeight - RectHeight(&(**docInfo).viewRect)) {
					vDistance = (**docInfo).docHeight - (**docInfo).extraHeight - (**docInfo).vPosition - RectHeight(&(**docInfo).viewRect);
				}
				
				if(vDistance + (**docInfo).vPosition < 0L) {
					vDistance = -(**docInfo).vPosition;
				}
				
				sdh = -hDistance;
				sdv = -vDistance;
				if(sdh != 0 || sdv != 0) {
					
#ifdef DRAGMANAGERHASACLUE
					errCode = DragPreScroll(theDragRef, sdh, sdv);
#else
					if((**dragData).isHilited) {
						errCode = HideDragHilite(theDragRef);
					}
#endif
					
					if(errCode == noErr) {
						if((**dragData).caretOn) {
							DrawCaret(docInfo, &oldSelection);
							(**dragData).caretOn = false;
						}
						ScrollDoc(docInfo, hDistance, vDistance);

#ifdef DRAGMANAGERHASACLUE
						errCode = DragPostScroll(theDragRef);
#else
						if((**dragData).isHilited) {
							RectRgn(scratchRgn, &viewRect);
							errCode = ShowDragHilite(theDragRef, scratchRgn, true);
						}						
#endif
					}
				}
			}
			
			if((errCode != noErr) || !PtInRect(dragMouse, &viewRect)) {
				break;
			}
			
			/* Get the offset for the mouse position */
			errCode = PositionToOffset(docInfo, &selection);
			if(errCode != noErr) {
				break;
			}
			
			errCode = FindNextInsertLoc(docInfo, &selection, !(**dragData).dragTextOnly);
			if(errCode < noErr) {
				if((**dragData).caretOn) {
					DrawCaret(docInfo, &oldSelection);
					(**dragData).caretOn = false;
				}
				break;
			}
			
			if(((**docInfo).flags.dragActive) && (selection.offset >= (**docInfo).selStart.offset) && (selection.offset <= (**docInfo).selEnd.offset)) {
				if((**dragData).caretOn) {
					DrawCaret(docInfo, &oldSelection);
					(**dragData).caretOn = false;
				}
				errCode = dragNotAcceptedErr;
				break;
			}
			
			errCode = OffsetToPosition(docInfo, &selection, (errCode != noErr));
			if(errCode != noErr) {
				break;
			}
			
			selection.lPosition = selection.rPosition = ClosestValue(selection.hPosition, selection.lPosition, selection.rPosition);
			
			if((oldSelection.lPosition != selection.lPosition) || (oldSelection.vPosition != selection.vPosition)) {
				if((**dragData).caretOn) {
					DrawCaret(docInfo, &oldSelection);
					(**dragData).caretOn = false;
				}
				goto DrawCurrentCaret;
			} else {
				caretTime = GetCaretTime();
				if((**dragData).lastCaret + caretTime < currentTicks) {
			DrawCurrentCaret :
					(**dragData).caretOn = !(**dragData).caretOn;
					(**dragData).lastCaret = currentTicks;
					DrawCaret(docInfo, &selection);
					(**dragData).caretLocation = selection;
				}
			}
			
			
			break;
		case kDragTrackingLeaveWindow :
			if((**docInfo).flags.dragActive) {
				(**docInfo).flags.dragHasLeft = true;
			}
			if(dragData != nil) {
				if((**dragData).isHilited) {
					HideDragHilite(theDragRef);
				}
				if((**dragData).caretOn) {
					selection = (**dragData).caretLocation;
					DrawCaret(docInfo, &selection);
				}
				if((**dragData).autoScrollRgn != nil) {
					DisposeRgn((**dragData).autoScrollRgn);
				}
	DisposeAndReturn :
				DisposeHandle((Handle)dragData);
				(**docInfo).dragData = nil;
			}
	}
	
	ResetPortInfo(&savedInfo);
	return errCode;
}

OSErr GetDragContents(DocumentInfoHandle docInfo, StringHandle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, DragReference theDragRef, long dropLocation);
OSErr GetDragContents(DocumentInfoHandle docInfo, StringHandle *textScrap, PETEStyleListHandle *styleScrap, PETEParaScrapHandle *paraScrap, DragReference theDragRef, long dropLocation)
{
	long textSize, styleSize, paraSize, endTextLen;
	OSErr errCode, memErr;
	FlavorType styleFlavor;
	unsigned short numItems, itemIndex;
	ItemReference itemRef;
	StringHandle newTextScrap;
	PETEStyleListHandle newStyleScrap;
	PETEParaScrapHandle newParaScrap;
	PETEStyleEntry defaultStyle;
	SelData selection;
	
	*textScrap = nil;
	*styleScrap = nil;
	*paraScrap = nil;
	newTextScrap = nil;
	newStyleScrap = nil;
	newParaScrap = nil;
	endTextLen = 0L;
	
	if((**docInfo).callbacks.getDragContents != nil) {
		errCode = CallPETEGetDragContentsProc((**docInfo).callbacks.getDragContents, docInfo, textScrap, styleScrap, paraScrap, theDragRef, dropLocation);
	} else {
		errCode = handlerNotFoundErr;
	}

	if(errCode != handlerNotFoundErr) {
		return errCode;
	}

	errCode = CountDragItems(theDragRef, &numItems);
	
	for(itemIndex = 1; errCode == noErr && itemIndex <= numItems; ++itemIndex) {
		
		errCode = GetDragItemReferenceNumber(theDragRef, itemIndex, &itemRef);
		if(errCode != noErr) {
			break;
		}
	
		styleSize = paraSize = 0L;
		errCode = GetFlavorDataSize(theDragRef, itemRef, flavorTypeText, &textSize);
		
		// Hack for Safari, which puts TEXT and url flavors in what ought to be picture drags.
		// If we have TEXT + url + PICT, pretend we don't have TEXT, so we use PICT
		if(errCode == noErr) {
			long otherSize;
			
			// does it have a 'url ' flavor?
			errCode = GetFlavorDataSize(theDragRef, itemRef, flavorTypeURL, &otherSize);
			if(errCode == badDragFlavorErr) 
			{
				errCode = noErr;	// proceed as normal
			} else {
				// does it have a 'PICT' flavor?
				errCode = GetFlavorDataSize(theDragRef, itemRef, flavorTypePict, &otherSize);
				if (errCode == badDragFlavorErr) {
					errCode = noErr;	// proceed as normal
				} else {
						// pretend it doesn't have a TEXT flavor, so we'll use the PICT
						errCode = badDragFlavorErr;
				}
			}
		}
		// End hack for Safari
			
		if(errCode == badDragFlavorErr) {
			errCode = GetFlavorDataSize(theDragRef, itemRef, kPETEA822Flavor, &textSize);
			if(errCode == badDragFlavorErr) {
				errCode = GetFlavorDataSize(theDragRef, itemRef, flavorTypePict, &textSize);
				if(errCode == noErr) {
					PicHandle thePic;
					PictGraphicInfoHandle pictInfo;
					
					thePic = (PicHandle)MyNewHandle(textSize, &memErr, 0);
					if((errCode = memErr) == noErr) {
						HLock((Handle)thePic);
						errCode = GetFlavorData(theDragRef, itemRef, flavorTypePict, *thePic, &textSize, 0L);
						HUnlock((Handle)thePic);
						if(errCode == noErr) {
							pictInfo = (PictGraphicInfoHandle)MyNewHandle(sizeof(PictGraphicInfo), &memErr, hndlClear);
							errCode = memErr;
						}
						if(errCode == noErr) {
							(**pictInfo).gi.width = RectWidth(&(**thePic).picFrame);
							(**pictInfo).gi.height = RectHeight(&(**thePic).picFrame) + 1;
							(**pictInfo).pict = thePic;
							(**pictInfo).counter = 1L;
							selection.offset = dropLocation;
							selection.paraIndex = ParagraphIndex(docInfo, dropLocation);
							selection.lineIndex = 0L;
							selection.lastLine = false;
							selection.leadingEdge = false;
							GetSelectionStyle(docInfo, &selection, nil, nil, &defaultStyle.psStyle.textStyle);
							defaultStyle.psStartChar = 0L;
							defaultStyle.psGraphic = true;
							defaultStyle.psStyle.graphicStyle.tsLock = peNoLock;
							defaultStyle.psStyle.graphicStyle.graphicInfo = (GraphicInfoHandle)pictInfo;
							errCode = MyPtrToHand(&defaultStyle, (Handle *)&newStyleScrap, sizeof(PETEStyleEntry), hndlTemp);
							if(errCode == noErr) {
								newTextScrap = (StringHandle)MyNewHandle(1L, &memErr, 0);
								if((errCode = memErr) == noErr) {
									goto DoAppend;
								}
							} else {
								DisposeHandle((Handle)pictInfo);
								goto DisposeThePic;
							}
						} else {
					DisposeThePic :
							DisposeHandle((Handle)thePic);
						}
					}
				}
			}
		}
		if(errCode != noErr) {
			break;
		}
		
		styleFlavor = kPETEStyleFlavor;
		errCode = GetFlavorDataSize(theDragRef, itemRef, styleFlavor, &styleSize);
		if(errCode == badDragFlavorErr) {
			paraSize = 0L;
			styleFlavor = flavorTypeStyle;
			errCode = GetFlavorDataSize(theDragRef, itemRef, styleFlavor, &styleSize);
			if(errCode == badDragFlavorErr) {
				styleSize = 0L;
				errCode = noErr;
			} else if(errCode != noErr) {
				break;
			}
		} else if(errCode != noErr) {
			break;
		} else {
			errCode = GetFlavorDataSize(theDragRef, itemRef, kPETEParaFlavor, &paraSize);
			if(errCode == badDragFlavorErr) {
				paraSize = 0L;
				errCode = noErr;
			} else if(errCode != noErr) {
				break;
			}
		}
		
		newTextScrap = (StringHandle)MyNewHandle(textSize, &memErr, hndlTemp);
		if((errCode = memErr) != noErr) {
			break;
		}
		
		if(styleSize != 0L) {
			newStyleScrap = (PETEStyleListHandle)MyNewHandle(styleSize, &memErr, hndlTemp);
			if(((errCode = memErr) == noErr) && (paraSize != 0L)) {
				newParaScrap = (PETEParaScrapHandle)MyNewHandle(paraSize, &memErr, hndlTemp);
				errCode = memErr;
			}
		} else {
			defaultStyle.psStartChar = 0L;
			defaultStyle.psGraphic = 0L;
			selection.offset = dropLocation;
			selection.paraIndex = ParagraphIndex(docInfo, dropLocation);
			selection.lineIndex = 0L;
			selection.lastLine = false;
			selection.leadingEdge = false;
			GetSelectionStyle(docInfo, &selection, nil, nil, &defaultStyle.psStyle.textStyle);
			defaultStyle.psStyle.textStyle.tsLock = peNoLock;
			errCode = MyPtrToHand(&defaultStyle, (Handle *)&newStyleScrap, sizeof(PETEStyleEntry), hndlTemp);
		}
		
		if(errCode != noErr) {
			break;
		}
		
		HLock((Handle)newTextScrap);
		errCode = GetFlavorData(theDragRef, itemRef, flavorTypeText, *newTextScrap, &textSize, 0L);
		if(errCode == badDragFlavorErr) {
			errCode = GetFlavorData(theDragRef, itemRef, kPETEA822Flavor, *newTextScrap, &textSize, 0L);
		}
		HUnlock((Handle)newTextScrap);
		
		if((errCode == noErr) && (styleSize != 0L)) {
			HLock((Handle)newStyleScrap);
			errCode = GetFlavorData(theDragRef, itemRef, styleFlavor, *newStyleScrap, &styleSize, 0L);
			HUnlock((Handle)newStyleScrap);
			
			if((errCode == noErr) && (styleFlavor != kPETEStyleFlavor)) {
				errCode = TEScrapToPETEStyle((StScrpHandle)newStyleScrap, &newStyleScrap);
			}
			
			if((errCode == noErr) && (paraSize != 0L)) {
				HLock((Handle)newParaScrap);
				errCode = GetFlavorData(theDragRef, itemRef, kPETEParaFlavor, *newParaScrap, &paraSize, 0L);
				HUnlock((Handle)newParaScrap);
			}
		}
		
		if(errCode != noErr) {
			break;
		}
	DoAppend :
		errCode = AppendToScrap(textScrap, styleScrap, paraScrap, newTextScrap, newStyleScrap, newParaScrap, &endTextLen, true);
		if(newTextScrap != nil) {
			DisposeHandle((Handle)newTextScrap);
			newTextScrap = nil;
		}
		if(newStyleScrap != nil) {
			if(errCode != noErr) {
				PictDispose(*newStyleScrap);
			}
			DisposeHandle((Handle)newStyleScrap);
			newStyleScrap = nil;
		}
		if(newParaScrap != nil) {
			DisposeHandle((Handle)newParaScrap);
			newParaScrap = nil;
		}
	}
	
	if(errCode != noErr) {
		if(newTextScrap != nil) {
			DisposeHandle((Handle)newTextScrap);
		}
		if(newStyleScrap != nil) {
			PictDispose(*newStyleScrap);
			DisposeHandle((Handle)newStyleScrap);
		}
		if(newParaScrap != nil) {
			DisposeHandle((Handle)newParaScrap);
		}
		if(*textScrap != nil) {
			DisposeHandle((Handle)*textScrap);
		}
		if(*styleScrap != nil) {
			long styleCount = InlineGetHandleSize((Handle)*styleScrap) / sizeof(PETEStyleEntry);

			while(--styleCount >= 0L) {
				PictDispose(&(**styleScrap)[styleCount]);
			}
			
			DisposeHandle((Handle)*styleScrap);
		}
		if(*paraScrap != nil) {
			DisposeHandle((Handle)*paraScrap);
		}
	}
	return errCode;
}

void PictDispose(PETEStyleEntryPtr styleEntry)
{
	GraphicInfoHandle graphicInfo;

	if(styleEntry->psGraphic) {
		graphicInfo = styleEntry->psStyle.graphicStyle.graphicInfo;
		if((graphicInfo != nil) && ((**graphicInfo).itemProc == nil)) {
			if((**(PictGraphicInfoHandle)graphicInfo).counter == 1L) {
				DisposeHandle((Handle)(**(PictGraphicInfoHandle)graphicInfo).pict);
				DisposeHandle((Handle)graphicInfo);
			}
		}
	}
}

pascal OSErr MyReceiveHandler(WindowPtr theWindow, void *handlerRefCon, DragReference theDragRef)
{
#pragma unused(theWindow)

	DocumentInfoHandle docInfo;
	Point dragMouse, pinnedMouse;
	Rect viewRect;
	OSErr errCode;
	SelData selection;
	DragDataHandle dragData;
	StringHandle textScrap;
	PETEStyleListHandle styleScrap;
	PETEParaScrapHandle paraScrap;
	short modifiers, mouseDownModifiers, mouseUpModifiers, added;
	long startOffset, endOffset, selStart, selEnd, oldSelStart, oldSelEnd, textSize;
	LongStyleRun tempStyleRun;
	MyTextStyle textStyle, saveTextStyle;
	
	errCode = GetDragModifiers(theDragRef, &modifiers, &mouseDownModifiers, &mouseUpModifiers);
	if(errCode != noErr) {
		return errCode;
	}

	errCode = GetDragMouse(theDragRef, &dragMouse, &pinnedMouse);
	if(errCode != noErr) {
		return errCode;
	}
	GlobalToLocal(&dragMouse);

	docInfo = (DocumentInfoHandle)handlerRefCon;
	viewRect = (**docInfo).viewRect;
	dragData = (**docInfo).dragData;
	if(dragData == nil) {
		return paramErr;
	}
	
	if(!(**dragData).dragAcceptable) {
		return dragNotAcceptedErr;
	}
	
	if((**dragData).isHilited) {
		HideDragHilite(theDragRef);
		(**dragData).isHilited = false;
	}

	if(!PtInRect(dragMouse, &viewRect)) {
		return dragNotAcceptedErr;
	}
	
	if(((**docInfo).flags.dragActive) && PtInRgn(dragMouse, (**docInfo).hiliteRgn)) {
		return dragNotAcceptedErr;
	}
	
	if((**dragData).caretOn) {
		selection = (**dragData).caretLocation;
		DrawCaret(docInfo, &selection);
		(**dragData).caretOn = false;
	}

	/* Set the vertical position to the top of window plus the click position */
	selection.vPosition = (**docInfo).vPosition;
	selection.vPosition += (dragMouse.v - viewRect.top);

	/* Set the horizontal position to the left of window plus the click position */
	selection.hPosition = (dragMouse.h - viewRect.left) + (**docInfo).hPosition;
	
	/* Get the offset for the click position */
	errCode = PositionToOffset(docInfo, &selection);
	if(errCode != noErr) {
		return errCode;
	}
	
	errCode = FindNextInsertLoc(docInfo, &selection, !(**dragData).dragTextOnly);
	if(errCode < noErr) {
		return errCode;
	}
	
	if(((**docInfo).flags.dragActive) && (selection.offset >= (**docInfo).selStart.offset) && (selection.offset <= (**docInfo).selEnd.offset)) {
		return dragNotAcceptedErr;
	}

	
	GetCurrentTextStyle(docInfo, &saveTextStyle);
	errCode = GetSelectionStyle(docInfo, &selection, &tempStyleRun, nil, &textStyle);
	if(errCode != noErr) {
		return errCode;
	}
	if(tempStyleRun.srIsGraphic) {
		SetPETEDefaultColor(textStyle.tsColor);
	}
	textStyle.tsLabel = 0;
	textStyle.tsLock = peNoLock;
	
	(**docInfo).curTextStyle = textStyle;
	
	oldSelStart = (**docInfo).selStart.offset;
	oldSelEnd = (**docInfo).selEnd.offset;
	
	errCode = GetDragContents(docInfo, &textScrap, &styleScrap, &paraScrap, theDragRef, selection.offset);
	if(errCode != noErr) {
		return errCode;
	}
	
	errCode = AddIntelligentPasteSpace(docInfo, &selection, textScrap, styleScrap, paraScrap, &added);
	
	if(errCode == noErr) {
		textSize = InlineGetHandleSize((Handle)textScrap);

		errCode = PasteText(docInfo, selection.offset, textScrap, styleScrap, paraScrap, !(!((mouseDownModifiers | mouseUpModifiers) & shiftKey)), true);
		
		(**docInfo).curTextStyle = saveTextStyle;
		GetCurrentTextStyle(docInfo, &saveTextStyle);
	}
	
	if(errCode == noErr) {
		SetInsertUndo(docInfo, selection.offset, textSize, oldSelStart, oldSelEnd, peUndoDrag, false);
		startOffset = (**docInfo).undoData.startOffset;
		endOffset = (**docInfo).undoData.endOffset;
	}
	

	if((errCode == noErr) && (**docInfo).flags.dragActive && !(mouseDownModifiers & optionKey)) {
		errCode = GetIntelligentCutBoundaries(docInfo, &selStart, &selEnd);
		
		if(errCode == noErr) {
			errCode = SetDeleteUndo(docInfo, selStart, selEnd, peUndoDrag);
			if(errCode == noErr) {
				errCode = DeleteText(docInfo, selStart, selEnd);
			}
			
			(**docInfo).undoData.startOffset = startOffset;
			(**docInfo).undoData.endOffset = endOffset;
			
			if((errCode == errAENotModifiable) || (errCode == errAEImpossibleRange)) {
				errCode = noErr;
			} else if((errCode == noErr) && (selStart <= selection.offset)) {
				selection.offset -= selEnd - selStart;
			}
		}
	}
	
	if(errCode == noErr) {
		PunctuateUndo(docInfo);
		
		if((**docInfo).selectedGraphic != nil) {
			MyCallGraphicHit(docInfo, (**docInfo).selStart.offset, nil);
			(**docInfo).selectedGraphic = nil;
		}

		(**docInfo).selStart.offset = selection.offset + (added == -1 ? 1L : 0L);
		(**docInfo).selEnd.offset = (**docInfo).selStart.offset + (textSize - (added == 0 ? 0L : 1L));
		(**docInfo).selStart.lastLine = (**docInfo).selEnd.lastLine = false;
		CalcSelectionRgn(docInfo, false);
		(**docInfo).undoData.selStart = oldSelStart;
		(**docInfo).undoData.selEnd = oldSelEnd;
	}
	
	return errCode;
}

OSErr AliasAEToDir(AEDesc *aliasAE, short *volumeID, long *directoryID)
{
	OSErr errCode;
	AEDesc targetDescriptor;
	FSSpec targetLocation;
	CInfoPBRec getTargetInfo;
	Boolean targetIsFolder, wasAliased;
	
	errCode = AECoerceDesc(aliasAE, typeFSS, &targetDescriptor);
	if (errCode != noErr) return errCode;
	
	AEGetDescData(&targetDescriptor, &targetLocation, sizeof(FSSpec));
	
	errCode = AEDisposeDesc(&targetDescriptor);
	if (errCode != noErr) return errCode;
	
	errCode = ResolveAliasFile(&targetLocation, true, &targetIsFolder, &wasAliased);
	if (errCode != noErr) return errCode;
	
	if(!targetIsFolder) return dirNFErr;
	
	getTargetInfo.dirInfo.ioNamePtr = targetLocation.name;
	getTargetInfo.dirInfo.ioVRefNum = targetLocation.vRefNum;
	getTargetInfo.dirInfo.ioFDirIndex = 0;
	getTargetInfo.dirInfo.ioDrDirID = targetLocation.parID;
	
	errCode = PBGetCatInfoSync(&getTargetInfo);
	if (errCode != noErr) return errCode;
	
	*directoryID = getTargetInfo.dirInfo.ioDrDirID;
	*volumeID = targetLocation.vRefNum;
	return noErr;
}

Boolean DragWasToTrash(DragReference dragRef)
{
	AEDesc dropLocation;
	OSErr errCode;
	long dropDirID, trashDirID;
	short dropVRefNum, trashVRefNum;
	Boolean wasTrash;
	
	wasTrash = false;
	
	errCode = GetDropLocation(dragRef, &dropLocation);
	if (errCode != noErr) goto exit;
	
	if (dropLocation.descriptorType == typeNull) goto dispose;
	
	errCode = AliasAEToDir(&dropLocation, &dropVRefNum, &dropDirID);
	if (errCode != noErr) goto dispose;
	
	errCode = FindFolder(dropVRefNum, kTrashFolderType, false, &trashVRefNum, &trashDirID);
	if (errCode != noErr) goto dispose;
	
	wasTrash = (dropVRefNum == trashVRefNum && dropDirID == trashDirID);

dispose:
	AEDisposeDesc(&dropLocation);

exit:
	return wasTrash;
}

Boolean DragItemHasFlavor(DragReference theDragRef, ItemReference theItemRef, FlavorType targetType)
{
	unsigned short numFlavors, index;
	FlavorType theType;
	OSErr errCode;

	errCode = CountDragItemFlavors(theDragRef, theItemRef, &numFlavors);
	if(errCode != noErr) {
		return false;
	}
	for(index = numFlavors; index > 0; --index) {
		errCode = GetFlavorType(theDragRef, theItemRef, index, &theType);
		if(errCode != noErr) {
			return false;
		}
		if(theType == targetType) {
			return true;
		}
	}
	return false;
}

OSErr SetControlDrag(DocumentInfoHandle docInfo, Boolean useControl)
{
	(**docInfo).flags.dragOnControlOnly = useControl;
	return noErr;
}