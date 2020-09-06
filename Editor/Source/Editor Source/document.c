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

/* Create a new document handle */
OSErr CreateNewDocument(PETEGlobalsHandle globals, DocumentInfoHandle *docInfo, PETEDocInitInfoPtr initInfo)
{
	DocumentInfoHandle newDocInfo;
	ParagraphInfoHandle newParaInfo;
	IdleGraphicHandle idleGraphics;
	PETEParaInfoPtr paraInfo;
	LongSTTable newStyleTable;
	WindowRef theWindow;
	ControlRef tempControl;
	Handle tempEmptyHandle, tempHandle;
	RgnHandle tempRgn, tempRgn1, tempRgn2;
	PETEPortInfo savedPortInfo;
	FMetricRec theMetrics;
	long paraInfoSize;
	DocFlagsBlock flags;
	short docWidth;
	MyTextStylePtr textStyle;
	Rect viewRect, controlRect;
	OSErr errCode, tempErr;
	short tabIndex;
	
	theWindow = initInfo->inWindow;
	paraInfo = &initInfo->inParaInfo;
	docWidth = initInfo->docWidth;
	textStyle = &initInfo->inStyle;
	Zero(flags);
	if(initInfo->docFlags & peVScroll) {
		flags.hasVScroll = true;
	}
	if(initInfo->docFlags & peHScroll) {
		flags.hasHScroll = true;
	}
	if(initInfo->docFlags & peGrowBox) {
		flags.hasGrowBox = true;
	}
	if(initInfo->docFlags & peUseHLineWidth) {
		flags.useHLineWidth = true;
	}
	if(initInfo->docFlags & peUseOffscreen) {
		flags.offscreenBitMap = true;
	}
	if(initInfo->docFlags & peDrawDebugSymbols) {
		flags.drawDebugSymbols = true;
	}
	if(initInfo->docFlags & peDragOnControl) {
		flags.dragOnControlOnly = true;
	}
	if(initInfo->docFlags & peNoStyledPaste) {
		flags.noStyledPaste = true;
	}
	if(initInfo->docFlags & peClearAllReturns) {
		flags.clearAllReturns = true;
	}
	if(initInfo->docFlags & peEmbedded) {
		flags.embedded = true;
		flags.offscreenBitMap = false;
	}
	
#ifdef TESTAPPLICATION
	flags.peDrawDebugSymbols = true;
#endif
	
	/* Get this size of the initial paragraph info structure */
	paraInfoSize = sizeof(ParagraphInfo) + (sizeof(short) * ((paraInfo != nil) ? (ABS(paraInfo->tabCount) - 1) : -1));

	/* Allocate the memory */
	if(((tempRgn = NewRgn()) == nil) || ((tempRgn1 = NewRgn()) == nil) || ((tempRgn2 = NewRgn()) == nil)) {
		tempErr = memFullErr;
	} else if((newParaInfo = (ParagraphInfoHandle)MyNewHandle(paraInfoSize + sizeof(LongStyleRun), &tempErr, hndlClear)) != nil) {
		if((tempEmptyHandle = MyNewHandle(0L, &tempErr, hndlEmpty)) != nil) {
			if((newStyleTable = (LongSTTable)MyNewHandle(sizeof(LongSTTable), &tempErr, 0)) != nil) {
				if((idleGraphics = (IdleGraphicHandle)MyNewHandle(0L, &tempErr, 0)) != nil) {
					if((tempHandle = MyNewHandle(sizeof(LastScriptFont), &tempErr, hndlClear)) != nil) {
						newDocInfo = (DocumentInfoHandle)MyNewHandle(sizeof(DocumentInfo) + sizeof(ParagraphElement), &tempErr, hndlClear);
						(**newStyleTable).docInfo = newDocInfo;
					}
				}
			}
		}
	}

	/* If all of the memory was allocated OK, create scrollbars and set view rect */
	if((errCode = tempErr) == noErr) {
		
		short scrollProc = (**globals).flags.hasAppearanceManager ? ((**globals).flags.useLiveScrolling ? kControlScrollBarLiveProc : kControlScrollBarProc) : scrollBarProc;
	
		G_MEMCANTFAIL(globals);

		/* Start with the viewRect the same as the entire docRect  */
		viewRect = initInfo->inRect;
//		InsetRect(&viewRect, 1, 1);
		
		/* Is there a vertical scrollbar, or no scrollbars and a grow box? */
		if((flags.hasVScroll) || ((flags.hasGrowBox) && !(flags.hasHScroll))) {
			/* Yes, so leave room on right */
			viewRect.right -= kScrollbarAdjust;
		}
		
		/* If there's a horizontal scrollbar, leave room on the bottom */
		if(flags.hasHScroll) {
			viewRect.bottom -= kScrollbarAdjust;
		}
		
		/* Create the vertical scrollbar, if necessary */
		if(flags.hasVScroll) {
			GetVScrollbarLocation(&initInfo->inRect, &controlRect, flags.hasHScroll, flags.hasGrowBox);
			
			/* Create the vertical scrollbar */
			tempControl = NewControl(theWindow, &controlRect, nil,
#ifdef SHOWHIDESCROLLS
			                         (controlRect.bottom - controlRect.top >= kTooSmallForScroll),
#else
			                         true,
#endif
									 0, 0, 0, scrollProc, (long)newDocInfo);
			(**newDocInfo).vScroll = tempControl;

			/* Set the error value */
			if((**newDocInfo).vScroll == nil) {
				errCode = memFullErr;
			} else if((**globals).flags.hasAppearanceManager && initInfo->containerControl) {
				EmbedControl(tempControl, initInfo->containerControl);
			}
		}
		
		/* Create the horizontal scrollbar, if necessary */
		if((errCode == noErr) && (flags.hasHScroll)) {
			GetHScrollbarLocation(&initInfo->inRect, &controlRect, flags.hasVScroll, flags.hasGrowBox);

			/* Create the horizontal scrollbar */
			tempControl = NewControl(theWindow, &controlRect, nil,
#ifdef SHOWHIDESCROLLS
			                         (controlRect.bottom - controlRect.top >= kTooSmallForScroll),
#else
			                         true,
#endif
									 0, 0, 0, scrollProc, (long)newDocInfo);

			(**newDocInfo).hScroll = tempControl;
			/* Set the error value */
			if((**newDocInfo).hScroll == nil) {
				errCode = memFullErr;
			} else if((**globals).flags.hasAppearanceManager && initInfo->containerControl) {
				EmbedControl(tempControl, initInfo->containerControl);
			}
		}

		G_MEMCANFAIL(globals);
	}
	
	/* Set up the globals, which may be needed below */
	if(errCode == noErr) {
		(**newDocInfo).globals = globals;
	}

	/* If everything was allocated OK, set up the initial style info */
	if(errCode == noErr) {
		/* Add the default style to the style table */
		errCode = AddStyle((PETEStyleInfoPtr)textStyle, newStyleTable, nil, false, false);
	}

	/* Add the document info handle to the end of the globals */
	if(errCode == noErr) {
		/* Set the value of the new document to return */
		*docInfo = newDocInfo;
		errCode = PtrAndHand((Ptr)docInfo, (Handle)globals, sizeof(DocumentInfoHandle));
	}

	/* If an error occurred, deallocate all memory */
	if(errCode != noErr) {
		if(tempRgn != nil) {
			if(tempRgn1 != nil) {
				if(tempRgn2 != nil) {
					if(newParaInfo != nil) {
						if(tempEmptyHandle != nil) {
							if(newStyleTable != nil) {
								if(idleGraphics != nil) {
									if(tempHandle != nil) {
										if(newDocInfo != nil) {
											if((**newDocInfo).vScroll != nil) {
												G_MEMCANTFAIL(globals);
												if((**newDocInfo).hScroll != nil) {
													DisposeControl((**newDocInfo).hScroll);
												}
												DisposeControl((**newDocInfo).vScroll);
												G_MEMCANFAIL(globals);
											}
											DisposeHandle((Handle)newDocInfo);
										}
										DisposeHandle(tempHandle);
									}
									DisposeHandle((Handle)idleGraphics);
								}
								DisposeHandle((Handle)newStyleTable);
							}
							DisposeHandle(tempEmptyHandle);
						}
						DisposeHandle((Handle)newParaInfo);
					}
					DisposeRgn(tempRgn2);
				}
				DisposeRgn(tempRgn1);
			}
			DisposeRgn(tempRgn);
		}
		*docInfo = nil;
		return errCode;
	}
	
	/* If there was default paragraph info, move it into the document */
	if(paraInfo != nil) {
		BlockMoveData(&paraInfo->startMargin, &(*newParaInfo)->startMargin, offsetof(ParagraphInfo, tabStops) - offsetof(ParagraphInfo, startMargin));
		BlockMoveData(*paraInfo->tabHandle, &(*newParaInfo)->tabStops, ABS(paraInfo->tabCount) * sizeof(short));
	}
	
	/* Set up default values where requested */
	if(docWidth < 0L) {
		/* Default document width is the window width */
		docWidth = RectWidth(&viewRect);
	}
	
	if((paraInfo == nil) || ((**newParaInfo).startMargin < 0L)) {
		/* Default starting margin is #defined */
		(**newParaInfo).startMargin = kDefaultMargin;
		
		/* Move over the tabs that might have been given */
		for(tabIndex = (**newParaInfo).tabCount; --tabIndex >= 0; ) {
			(**newParaInfo).tabStops[tabIndex] += (**newParaInfo).startMargin;
		}
	}
	
	if((paraInfo == nil) || ((**newParaInfo).endMargin == kPETEDefaultMargin)) {
		/* Default ending margin is short of the document width */
		(**newParaInfo).endMargin = -kDefaultMargin;
	}
	
	if((paraInfo == nil) || ((**newParaInfo).direction == kHilite)) {
		/* Default direction is the system direction */
		(**newParaInfo).direction = (short)(Byte)GetScriptVariable(smSystemScript, smScriptRight);
	}
	
	if((paraInfo == nil) || ((**newParaInfo).justification == teFlushDefault)) {
		/* Default justification is the system justification */
		(**newParaInfo).justification = ((Byte)GetScriptVariable(smSystemScript, smScriptJust) == 0) ? teFlushLeft : teFlushRight;
	}

	FlushStyleRunCache(newParaInfo);
	
	/* Set up the line start array as an empty handle */
	(**newParaInfo).lineStarts = (LSTable)tempEmptyHandle;
	
	/* Put the terminating style run information */
	((LongStyleRunPtr)(((Ptr)*newParaInfo) + paraInfoSize))->srStyleLen = -1L;
	((LongStyleRunPtr)(((Ptr)*newParaInfo) + paraInfoSize))->srStyleIndex = 0L;
	
	/* Set the values for the document */
	(**newDocInfo).docWidth = docWidth;
	(**newDocInfo).docWindow = theWindow;
	(**newDocInfo).docRect = initInfo->inRect;
	(**newDocInfo).viewRect = viewRect;
	(**newDocInfo).containerControl = initInfo->containerControl;
	(**newDocInfo).textRefNum = -1;
	(**newDocInfo).styleRefNum = -1;
	(**newDocInfo).lastFont = (LastScriptFontHandle)tempHandle;
	(**newDocInfo).theStyleTable = newStyleTable;
	(**newDocInfo).hiliteRgn = tempRgn;
	(**newDocInfo).graphicRgn = tempRgn1;
	(**newDocInfo).updateRgn = tempRgn2;
	SetPETEDefaultColor((**newDocInfo).defaultColor);
	SetPETEDefaultColor((**newDocInfo).defaultBGColor);
	(**newDocInfo).idleGraphics = idleGraphics;

	flags.selectionDirty = true;
	(**newDocInfo).flags = flags;

	/* Initialize the line cache information */
	FlushDocInfoLineCache(newDocInfo);
	
	SetStyleAndKeyboard(newDocInfo, textStyle);
	
	/* Get the initial font size information */
	SavePortInfo(GetWindowPort(theWindow), &savedPortInfo);
	SetTextStyleWithDefaults(globals, nil, textStyle, false, false);
	FontMetrics(&theMetrics);
	ResetPortInfo(&savedPortInfo);
	
	/* Set the selection line heights so the cursor draws properly */
	(**newDocInfo).shortLineHeight = (**newDocInfo).selStart.vLineHeight = (**newDocInfo).selEnd.vLineHeight = FixRound(theMetrics.ascent + theMetrics.descent + theMetrics.leading);
	
	/* Set the initial paragraph info */
	(**newDocInfo).paraCount = 1L;
	(**newDocInfo).paraArray[0L].paraInfo = newParaInfo;
	SetParaDirty(newDocInfo, 0L);

	return noErr;
}

/* Dispose of a document handle */
OSErr DisposeDocument(PETEGlobalsHandle globals, DocumentInfoHandle docInfo)
{
	long paraIndex, docIndex;
	ParagraphInfoHandle paraInfo;
	DocumentInfoHandle *docInfoArray;

	if(docInfo == nil) {
		return paramErr;
	}
	
	docIndex = (InlineGetHandleSize((Handle)globals) - sizeof(PETEGlobals)) / sizeof(DocumentInfoHandle);
	docInfoArray = (**globals).docInfoArray;
	do {
		--docIndex;
	} while((docIndex >= 0L) && (docInfoArray[docIndex] != docInfo));
	
	if(docIndex < 0L) {
		return tsmInvalidDocIDErr;
	}
	
	(**docInfo).flags.docCorrupt = true;
	
	/* Dispose of the regions */
	DisposeRgn((**docInfo).hiliteRgn);
	DisposeRgn((**docInfo).graphicRgn);
	DisposeRgn((**docInfo).updateRgn);

	/* Clear the line cache */
	FlushDocInfoLineCache(docInfo);

	DI_MEMCANTFAIL(docInfo);

	/* Dispose of the vertical scrollbar if it exists */
	if((**docInfo).vScroll != nil) {
		DisposeControl((**docInfo).vScroll);
	}

	/* Dispose of the horizontal scrollbar if it exists */
	if((**docInfo).hScroll != nil) {
		DisposeControl((**docInfo).hScroll);
	}
	DI_MEMCANFAIL(docInfo);
	
	if((**docInfo).undoData.theText != nil) {
		DisposeHandle((Handle)(**docInfo).undoData.theText);
	}

	if((**docInfo).undoData.paraScrap != nil) {
		DisposeHandle((Handle)(**docInfo).undoData.paraScrap);
	}
	

	if((**docInfo).undoData.styleScrap != nil) {
		DeleteScrapStyles((**docInfo).theStyleTable, (**docInfo).undoData.styleScrap);
		DisposeHandle((Handle)(**docInfo).undoData.styleScrap);
	}
	
	if((**docInfo).printData != nil) {
		DisposeHandle((Handle)(**docInfo).printData);
	}
	
	if((**docInfo).lastFont != nil) {
		DisposeHandle((Handle)(**docInfo).lastFont);
	}

	if((**docInfo).defaultFonts != nil) {
		DisposeHandle((Handle)(**docInfo).defaultFonts);
	}

	if((**docInfo).labelStyles != nil) {
		DisposeHandle((Handle)(**docInfo).labelStyles);
	}
	
	if((**docInfo).idleGraphics != nil) {
		DisposeHandle((Handle)(**docInfo).idleGraphics);
	}

	/* Loop through the paragraph handles */
	for(paraIndex = (**docInfo).paraCount; --paraIndex >= 0L; ) {
		paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
		
		DeleteParaStyleRuns(paraInfo, (**docInfo).theStyleTable, 0L, CountStyleRuns(paraInfo));
		
		/* If there is a line starts array, dispose of it */
		if((**paraInfo).lineStarts != nil) {
			DisposeHandle((Handle)(**paraInfo).lineStarts);
		}
		
		/* Dispose of the paragraph handle */
		DisposeHandle((Handle)paraInfo);
	}

	/* Dispose of the text handle */
	if((**docInfo).theText != nil) {
		DisposeHandle((Handle)(**docInfo).theText);
	}

	/* Dispose of the style table */
	DisposeHandle((Handle)(**docInfo).theStyleTable);

	docInfoArray[docIndex] = nil;
	
	/* Dispose of the document handle */
	DisposeHandle((Handle)docInfo);

	Munger((Handle)globals, sizeof(PETEGlobals) + (docIndex * sizeof(DocumentInfoHandle)), nil, sizeof(DocumentInfoHandle), (Ptr)1L, 0L);

	return noErr;

}

Handle GetDocTextHandle(DocumentInfoHandle docInfo)
{
	if((**docInfo).theText != nil) {
		SetHandleSize((Handle)(**docInfo).theText, (**docInfo).textLen);
	}
	return (Handle)(**docInfo).theText;
}

OSErr SetCallbackRoutine(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, ProcPtr theProc, PETECallbackEnum procType)
{
	OSErr errCode;
	
	errCode = noErr;
	switch(procType) {
		case peSetDragContents :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				(**docInfo).callbacks.setDragContents = (PETESetDragContentsProcPtr)theProc;
			}
			break;
		case peGetDragContents :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				(**docInfo).callbacks.getDragContents = (PETEGetDragContentsProcPtr)theProc;
			}
			break;
		case peProgressLoop :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				(**docInfo).callbacks.progressLoop = (PETEProgressLoopProcPtr)theProc;
			}
			break;
		case peDocChanged :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				(**docInfo).callbacks.docChanged = (PETEDocHasChangedProcPtr)theProc;
			}
			break;
		case peHasBeenCalled :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				(**globals).hasBeenCalledCallback = (PETEHasBeenCalledProcPtr)theProc;
			}
			break;
		case peWordBreak :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				(**globals).wordBreakCallback = (PETEWordBreakProcPtr)theProc;
			}
			break;
		case peIntelligentCut :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				(**globals).intelligentCutCallback = (PETEIntelligentCutProcPtr)theProc;
			}
			break;
		case peIntelligentPaste :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				(**globals).intelligentPasteCallback = (PETEIntelligentPasteProcPtr)theProc;
			}
			break;
		default :
			errCode = paramErr;
	}
	return errCode;
}

OSErr GetCallbackRoutine(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, ProcPtr *theProc, PETECallbackEnum procType)
{
	OSErr errCode;
	
	errCode = noErr;
	switch(procType) {
		case peSetDragContents :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**docInfo).callbacks.setDragContents;
			}
			break;
		case peGetDragContents :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**docInfo).callbacks.getDragContents;
			}
			break;
		case peProgressLoop :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**docInfo).callbacks.progressLoop;
			}
			break;
		case peDocChanged :
			if(!docInfo) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**docInfo).callbacks.docChanged;
			}
			break;
		case peHasBeenCalled :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**globals).hasBeenCalledCallback;
			}
			break;
		case peWordBreak :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**globals).wordBreakCallback;
			}
			break;
		case peIntelligentCut :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**globals).intelligentCutCallback;
			}
			break;
		case peIntelligentPaste :
			if(!globals) {
				errCode = nilHandleErr;
			} else {
				*theProc = (ProcPtr)(**globals).intelligentPasteCallback;
			}
			break;
		default :
			errCode = paramErr;
	}
	return errCode;
}

RGBColor DocOrGlobalColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo)
{
	if(docInfo != nil) {
		if(IsPETEDefaultColor((**docInfo).defaultColor)) {
			return (**(**docInfo).globals).defaultColor;
		} else {
			return (**docInfo).defaultColor;
		}
	} else if(globals != nil) {
		return (**globals).defaultColor;
	} else {
		RGBColor tempColor;
		
		tempColor.red = tempColor.blue = tempColor.green = 0;
		return tempColor;
	}
}

RGBColor DocOrGlobalBGColor(PETEGlobalsHandle globals, DocumentInfoHandle docInfo)
{
	if(docInfo != nil) {
		if(IsPETEDefaultColor((**docInfo).defaultBGColor)) {
			return (**(**docInfo).globals).defaultBGColor;
		} else {
			return (**docInfo).defaultBGColor;
		}
	} else if(globals != nil) {
		return (**globals).defaultBGColor;
	} else {
		RGBColor tempColor;
		
		tempColor.red = tempColor.blue = tempColor.green = 0xFFFF;
		return tempColor;
	}
}
