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

void DrawListBullet(DocumentInfoHandle docInfo, long paraIndex, short leftPosition, short ascent);

/* Draw the document in its window from an outside call */
OSErr UpdateDocument(DocumentInfoHandle docInfo)
{
	OSErr errCode;
	RgnHandle scratchRgn, tempRgn;
	Rect viewRect;
	PETEPortInfo savedPortInfo;
	
	if((**docInfo).printData != nil) {
		errCode = DrawDocumentWithPicture(docInfo);
	} else if((**docInfo).flags.progressLoopCalled || (**docInfo).flags.updating) {
		(**docInfo).flags.drawInProgressLoop = true;
		errCode = noErr;
		goto AddToUpdateRgn;
	} else {
		(**docInfo).flags.updating = true;
		errCode = DrawDocument(docInfo, nil, true);
		(**docInfo).flags.updating = false;
		if(errCode == cantDoThatInCurrentMode) {
		AddToUpdateRgn :
			tempRgn = NewRgn();
			if(tempRgn != nil) {
				scratchRgn = (**(**docInfo).globals).scratchRgn;
				SavePortInfo(((**docInfo).printData == nil) ? GetWindowPort((**docInfo).docWindow) : (**(**docInfo).printData).pPort, &savedPortInfo);
				GetClip(scratchRgn);
				if((**docInfo).printData == nil) {
					GetVisibleRgn(tempRgn);
					SectRgn(tempRgn, scratchRgn, scratchRgn);
				}
				ResetPortInfo(&savedPortInfo);
				viewRect = (**docInfo).viewRect;
				RectRgn(tempRgn, &viewRect);
				SectRgn(tempRgn, scratchRgn, scratchRgn);
				DisposeRgn(tempRgn);
				UnionRgn(scratchRgn, (**docInfo).updateRgn, (**docInfo).updateRgn);
			} else {
				errCode = memFullErr;
			}
		}
	}
	return errCode;
}

/* Draw the entire document in its window */
OSErr DrawDocument(DocumentInfoHandle docInfo, RgnHandle clipRgn, Boolean preserveLine)
{
	long paraIndex;
	short lineIndex, lineCount, curV;
	LSTable lineStarts;
	Rect drawRect, lineRect;
	LayoutCache lineCache;
	PETEPortInfo savedPortInfo;
	RgnHandle oldClip, scratchRgn, newClip;
	LineInfo lineInfo;
	CGrafPtr savedGWorld, newGWorld;
	GDHandle savedGDevice;
	GWorldPtr offscreenGWorld;
	GWorldFlags gFlags;
	PixMapHandle pixMap, tempPixMap;
	RGBColor backColor /*  = {0xffff,0xffff,0xffff} */;
	QDErr errCode;
	Byte hState;

/* Stupid compiler */
	backColor.red = backColor.green = backColor.blue = 0xffff;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;

	/* Create region for old clip region */
	oldClip = NewRgn();
	if(oldClip == nil) {
		return memFullErr;
	}
	
	/* Create region for new clip region */
	newClip = NewRgn();
	if(newClip == nil) {
		DisposeRgn(oldClip);
		return memFullErr;
	}
	
	/* Save old port info */
	newGWorld = ((**docInfo).printData == nil) ? GetWindowPort((**docInfo).docWindow) : (**(**docInfo).printData).pPort;
	if((**docInfo).flags.embedded && ((**(**docInfo).globals).savedGWorld == newGWorld))
		newGWorld = (**(**docInfo).globals).offscreenGWorld;
	SavePortInfo(newGWorld, &savedPortInfo);
	
	/* Get the view rect region */
	drawRect = (**docInfo).viewRect;
	RectRgn(newClip, &drawRect);

	if((**docInfo).printData == nil) {
		/* Clip down to the visRgn. Though this is automatic, the rgnBBox is used later */
		GetVisibleRgn(oldClip);
		SectRgn(oldClip, newClip, newClip);
	}

	/* Clip down to the old clipping region */
	GetClip(oldClip);
	SectRgn(oldClip, newClip, newClip);

	if(clipRgn != nil) {
		SectRgn(clipRgn, newClip, newClip);
	}

	SetClip(newClip);
	DiffRgn((**docInfo).graphicRgn, newClip, (**docInfo).graphicRgn);

	backColor = DocOrGlobalBGColor(nil, docInfo);
	if(!IsPETEDefaultColor(backColor)) {
		RGBBackColorAndPat(&backColor);
	} else {
		GetBackColor(&backColor);
	}
	
	offscreenGWorld = nil;
	if((**docInfo).flags.offscreenBitMap && !(**(**docInfo).globals).flags.progressLoopCalled && !(**(**docInfo).globals).savedGWorld && !EmptyRgn(newClip)) {
		savedGWorld = GetGlobalGWorld((**docInfo).globals);
		if(savedGWorld != nil) {
			DI_MEMCANTFAIL(docInfo);
			GetPortRect(&drawRect);
			LocalToGlobalRect(&drawRect);
			pixMap = GetGWorldPixMap(savedGWorld);
			if(QDError() == noErr) {
				gFlags = GetPixelsState(pixMap);
				NoPurgePixels(pixMap);
				if(QDError() == noErr) {
					if(!(UpdateGWorld(&savedGWorld, 0, &drawRect, nil, nil, 0) & gwFlagErr)) {
						tempPixMap = GetGWorldPixMap(savedGWorld);
						if(QDError() == noErr) {
							pixMap = tempPixMap;
							(**(**docInfo).globals).offscreenGWorld = savedGWorld;
							if(LockPixels(pixMap)) {
								offscreenGWorld = savedGWorld;
								GetGWorld(&savedGWorld, &savedGDevice);
								SetGWorld(offscreenGWorld, nil);
								SetClip(newClip);
								RGBBackColorAndPat(&backColor);
								(**(**docInfo).globals).savedGWorld = savedGWorld;
							} else {
								goto ResetPixels;
							}
						}
					} else {
				ResetPixels :
						SetPixelsState(pixMap, gFlags);
					}
				}
			}
			DI_MEMCANFAIL(docInfo);
		}
	}
	
	/* Erase the area in which to draw */
	if(!((**docInfo).flags.drawWithoutErase) && ((**docInfo).printData == nil)) {
		EraseRgn(newClip);
	}

	/* Save the bounds rectangle for drawing purposes */
	GetRegionBounds(newClip,&drawRect);
	
	PenAndColorNormal();

	/* Find the first line that appears in the drawRect */
	errCode = FindFirstVisibleLine(docInfo, &lineInfo, drawRect.top);
	if(errCode == errOffsetIsOutsideOfView) {
		errCode = noErr;
	}
	
	if((errCode == noErr) && (lineInfo.paraIndex >= 0L)) {
	
		paraIndex = lineInfo.paraIndex;
		lineIndex = lineInfo.lineIndex;
		curV = lineInfo.vPosition;
	
		/* Loop through the the paragraphs until the bottom of the drawing rect */
		for(; (paraIndex < (**docInfo).paraCount) && (curV < drawRect.bottom) && (errCode == noErr); ++paraIndex) {

			/* If the height hasn't been calculated, get it */
			errCode = CheckParagraphMeasure(docInfo, paraIndex, true);
			
			if(errCode == noErr) {
				/* Get the line starts for this paragraph */
				lineStarts = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts;
	
				hState = HGetState((Handle)lineStarts);
				HNoPurge((Handle)lineStarts);
				
				/* Count the lines in the paragraph */
				lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
				
				/* If an empty paragraph, just draw whatever quote marks are needed */
				if((lineIndex == 0L) && (lineIndex >= lineCount) && (curV < drawRect.bottom) && (errCode == noErr) && !(**docInfo).flags.drawDebugSymbols) {
					/* Make sure lineRect is empty so it won't try to draw an empty line */
					lineRect.left = lineRect.right = 0;
					goto JustDrawQuotes;
				}
				
				/* Loop through the lines in the paragraph */
				for(; lineIndex < lineCount && curV < drawRect.bottom && (errCode == noErr); ++lineIndex) {
	
					/* Create a rectangle into which the text will be drawn */
	
					/* Infinitely wide at first */
					lineRect.left = SHRT_MIN;
					lineRect.right = SHRT_MAX;
	
			JustDrawQuotes :
					/* Set the top to the top of the line */
					lineRect.top = curV;
					MoveTo((**docInfo).viewRect.left, curV);
					
					DrawQuoteMarks(docInfo, paraIndex, (*lineStarts)[lineIndex].lsMeasure.height);
					
					/* Move down the ascent to the base line */
					curV += (*lineStarts)[lineIndex].lsMeasure.ascent;
					Move(0, (*lineStarts)[lineIndex].lsMeasure.ascent);
					
					/* Add the rest of the height to the current vertical position */
					curV += (*lineStarts)[lineIndex].lsMeasure.height - (*lineStarts)[lineIndex].lsMeasure.ascent;
					
					/* Set the bottom to the bottom of the line */
					lineRect.bottom = curV;
	
					/* Calculate where the drawing rectangle intersects the clipRgn */
					RectRgn(scratchRgn, &lineRect);
					SectRgn(newClip, scratchRgn, scratchRgn);
					GetRegionBounds(scratchRgn,&lineRect);
					
					if(!EmptyRect(&lineRect)) {
	
						/* Layout the line to draw */
						lineCache.lineIndex = lineIndex;
						lineCache.paraIndex = paraIndex;
						errCode = GetLineLayout(docInfo, &lineCache);
		
						/* Draw the current line on the screen */
						if(errCode == noErr) {
							if(lineIndex == 0L) {
								DrawListBullet(docInfo, paraIndex, lineCache.leftPosition, (*lineStarts)[lineIndex].lsMeasure.ascent);
							}
			
							errCode = RenderLine(docInfo, &lineCache, &lineRect, newClip, offscreenGWorld != nil);
						}
					}
				}

				HSetState((Handle)lineStarts, hState);
				
			}

			if(((**docInfo).flags.drawDebugSymbols) && (lineIndex == lineCount)) {
				Move(3,0);
				Line(0,-5);
				Line(-2,0);
				Line(0,2);
				Line(3,0);
				Line(0,-2);
				Line(0,5);
			}
			
			if(lineCount <= 0L) {
				curV += (**docInfo).paraArray[paraIndex].paraHeight;
			}
			
			/* Reset to the first line in the paragraph */
			lineIndex = 0L;
		}
	}
	
	PenAndColorNormal();
	
	if(offscreenGWorld != nil) {
		SetGWorld(savedGWorld, savedGDevice);
		if(errCode == noErr) {
			RGBColor blackColor /* = {0,0,0} */, whiteColor /* = {0xffff,0xffff,0xffff} */;
			
			blackColor.red = blackColor.green = blackColor.blue = 0;
			whiteColor.red = whiteColor.green = whiteColor.blue = 0xffff;
			
			GetRegionBounds(newClip,&drawRect);
			RGBForeColor(&blackColor);
			RGBBackColorAndPat(&whiteColor);
			CopyBits(GetPortBitMapForCopyBits(offscreenGWorld),
			         GetPortBitMapForCopyBits(GetWindowPort((**docInfo).docWindow)),
			         &drawRect, &drawRect, srcCopy, newClip);
			errCode = QDError();
			RGBBackColorAndPat(&backColor);
		}
		SetPixelsState(pixMap, gFlags);
		(**(**docInfo).globals).savedGWorld = nil;
	}
	
	if(errCode == noErr) {
		/* If the caret was on before drawing started, leave it on and redraw the caret */
		if((**docInfo).flags.caretOn) {
			(**docInfo).flags.caretOn = false;
			DrawCaret(docInfo, nil);
		}
		
		/* Redraw the selection region */
		if((**docInfo).flags.selectionDirty) {
			DiffRgn((**docInfo).hiliteRgn, newClip, (**docInfo).hiliteRgn);
			CalcSelectionRgn(docInfo, preserveLine);
		} else {
			InvertSelection(docInfo);
		}
	}
		
	DisposeRgn(newClip);
	SetClip(oldClip);
	DisposeRgn(oldClip);

	if((errCode != noErr) && (errCode != cantDoThatInCurrentMode) && ((**docInfo).flags.offscreenBitMap)) {
		(**docInfo).flags.offscreenBitMap = false;
		errCode = DrawDocument(docInfo, clipRgn, preserveLine);
		(**docInfo).flags.offscreenBitMap = true;
	
	} else if(errCode == noErr) {
		
		/* If the scrolls are dirty, reset and redraw them */
		if((**docInfo).flags.scrollsDirty) {
			ResetScrollbars(docInfo);
		/* If this is in response to an update event, redraw the scrolls */
		} else if((clipRgn == nil) || ((**docInfo).flags.drawWithoutErase)) {
			DI_MEMCANTFAIL(docInfo);
			GetVisibleRgn(scratchRgn);
			if((**docInfo).vScroll != nil) {
#ifndef SHOWHIDESCROLLS
				Update1Control((**docInfo).vScroll, scratchRgn);
#else
				GetControlRect((**docInfo).vScroll, &drawRect);
				if(((**docInfo).flags.scrollsVisible) && (drawRect.bottom - drawRect.top >= kTooSmallForScroll)) {
					ShowControl((**docInfo).vScroll);
					Update1Control((**docInfo).vScroll, scratchRgn);
				} else {
					if(!((**docInfo).flags.drawWithoutErase)) {
						HideControl((**docInfo).vScroll);
						EraseRect(&drawRect);
					}
					FrameRect(&drawRect);
				}
#endif
			}
			if((**docInfo).hScroll != nil) {
#ifndef SHOWHIDESCROLLS
				Update1Control((**docInfo).hScroll, scratchRgn);
#else
				GetControlRect((**docInfo).hScroll, &drawRect);
				if(((**docInfo).flags.scrollsVisible) && (drawRect.right - drawRect.left >= kTooSmallForScroll)) {
					ShowControl((**docInfo).hScroll);
					Update1Control((**docInfo).hScroll, scratchRgn);
				} else {
					if(!((**docInfo).flags.drawWithoutErase)) {
						HideControl((**docInfo).hScroll);
						EraseRect(&drawRect);
					}
					FrameRect(&drawRect);
				}
#endif
			}
			DI_MEMCANFAIL(docInfo);
		}
	}
	
	ResetPortInfo(&savedPortInfo);

	return errCode;
}

/* Draw the line of text */
OSErr RenderLine(DocumentInfoHandle docInfo, LayoutCachePtr lineCache, Rect *drawRect, RgnHandle drawRgn, Boolean offscreen)
{
	OrderHandle orderingHndl;
	ParagraphInfoHandle paraInfo;
	GraphicInfoHandle graphicInfo;
	short numRuns, runIndex, curDirection, leftWS, rightWS;
	JustStyleCode styleRunPosition;
	Byte hState;
	LongSTElement tempStyle;
	Point scaling, penLoc;
	short hPosition;
	PicHandle thePict;
	Rect pictRect;
	OSErr errCode;
	long handleSize;
	RgnHandle scratchRgn;
	Style tabFace;
	Boolean ws = false;
	
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	paraInfo = (**docInfo).paraArray[lineCache->paraIndex].paraInfo;
	hPosition = (**docInfo).hPosition - lineCache->leftPosition;
	orderingHndl = lineCache->orderingHndl;
	handleSize = InlineGetHandleSize((Handle)orderingHndl);
	if(handleSize < 0L) {
		return handleSize;
	}
	numRuns = handleSize / sizeof(OrderEntry);	

	for(leftWS = runIndex = 0; runIndex < numRuns; ++runIndex) {
		if((*orderingHndl)[runIndex].isWhitespace) {
			++leftWS;
		} else {
			break;
		}
	}
		
	for(rightWS = 0, runIndex = numRuns; --runIndex >= 0; ) {
		if((*orderingHndl)[runIndex].isWhitespace) {
			++rightWS;
		} else {
			break;
		}
	}

	/* Not really necessary for un-justified text */
	styleRunPosition = (numRuns == 1) ? onlyStyleRun : leftStyleRun;
	
	/* Initialize scaling factors to 1:1 */
	scaling.h = scaling.v = 1;

	/* Go through all of the style runs to draw them from left to right */
	for(errCode = noErr, runIndex = 0; errCode == noErr && runIndex < numRuns && (GetPen(&penLoc), penLoc.h < drawRect->right) && ((**docInfo).flags.drawDebugSymbols || numRuns - runIndex > rightWS); ++runIndex, --leftWS) {

		/* If the current location is scrolled out of view, just move over */
		if((hPosition > 0L) || (!(**docInfo).flags.drawDebugSymbols && (leftWS > 0))) {
			
			hPosition -= (*orderingHndl)[runIndex].width;
			
			/* If some is visible, move left to the starting drawing location */
			if((hPosition < 0L) && (leftWS <= 0)) {
				Move(-(((*orderingHndl)[runIndex].width + hPosition)), 0);
				hPosition = 0L;
				
				/* Repeat this run */
				--runIndex;
				++leftWS;
				continue;
			} else {
				/* Go on to the next run */
				goto NextRun;
			}
		}

		/* If the current location is in the middle, move over to it */
		if(hPosition < 0L) {
			penLoc.h -= hPosition;
			hPosition = 0L;
			
			/* If the location is beyond the right side, there's nothing to draw */
			if(penLoc.h > drawRect->right)
				break;
			
			/* Move the pen over */
			MoveTo(penLoc.h, penLoc.v);
		}
		
		/* Calculate where the pen will be after drawing occurs */
		penLoc.h += (*orderingHndl)[runIndex].width;
		
		/* If it's not in the visible drawing area, just move the pen over */
		if(penLoc.h < drawRect->left) {
			MoveTo(penLoc.h, penLoc.v);
			
			/* Go on to the next run */
			goto NextRun;
		}
		
		GetStyle(&tempStyle, (**docInfo).theStyleTable, (*orderingHndl)[runIndex].styleIndex);

		/* Is this text or a graphic? */
		if(!((*orderingHndl)[runIndex].isGraphic)) {

			/* It's text, so set the text style */
			SetTextStyleWithDefaults(nil, docInfo, &tempStyle.stInfo.textStyle, true, ((**docInfo).printData != nil));
			TextMode(srcOr);
			
			/* Set direction and lock the text */
			curDirection = GetSysDirection();
			SetSysDirection((**paraInfo).direction);

			/* If it's a tab and the paragraph isn't centered, draw the tab */
			if(((*orderingHndl)[runIndex].isTab) && ((**paraInfo).justification != teCenter)) {
				tabFace = GetPortFace() & (Byte)GetScriptVariable(smCurrentScript, smScriptValidStyles);
				if((tabFace & underline) && (leftWS <= 0)) {
					if(tabFace & (outline | shadow)) {
						MoveTo(penLoc.h - 1, penLoc.v);
						Line(-(*orderingHndl)[runIndex].width + 1, 0);
					}
					Move(0, (tabFace & (outline | shadow)) ? 2 : 1);
					Line((*orderingHndl)[runIndex].width - 1, 0);
					Move(1, (tabFace & (outline | shadow)) ? -2 : -1);
					if(tabFace & shadow) {
						Move(-1, 3);
						Line(-(*orderingHndl)[runIndex].width + 1, 0);
						if(tabFace & outline) {
							Move(0, 1);
							Line((*orderingHndl)[runIndex].width - 1, 0);
							Move(1, -4);
						} else {
							Move((*orderingHndl)[runIndex].width, -3);
						}
					}
				} else {
					Move((*orderingHndl)[runIndex].width, 0);
				}
			} else if((*orderingHndl)[runIndex].len > 0) {
				long offset, len;
				
				hState = HGetState((Handle)(**docInfo).theText);
				HLock((Handle)(**docInfo).theText);
				
				offset = (*orderingHndl)[runIndex].offset - (**docInfo).textOffset;
				len = (*orderingHndl)[runIndex].len;
				if(GetPortFace() & underline) {
					Boolean leftToRight;
					
					leftToRight = !(Byte)GetScriptVariable(smCurrentScript, smScriptRight);
					if(leftWS == 0) {
						if(leftToRight) {
							long leadWS;

							IsWhitespace(*(**docInfo).theText + offset, len, smCurrentScript, (**(**docInfo).globals).whitespaceGlobals, &leadWS);
							if(leadWS != 0L) {
								Move(TextWidth(*(**docInfo).theText + offset, 0, leadWS), 0);
								len -= leadWS;
								offset += leadWS;
							}
						} else if(len != (*orderingHndl)[runIndex].rightLen) {
							len = (*orderingHndl)[runIndex].rightLen;
							Move((*orderingHndl)[runIndex].width - (*orderingHndl)[runIndex].rightWidth, 0);
						}
					}
					if((styleRunPosition == onlyStyleRun) || (styleRunPosition == rightStyleRun) || (rightWS - 1 == numRuns - runIndex)) {
						long trailWS;

						if(!leftToRight) {
							IsWhitespace(*(**docInfo).theText + offset, len, smCurrentScript, (**(**docInfo).globals).whitespaceGlobals, &trailWS);
							if(trailWS != 0L) {
								len -= trailWS;
								offset += trailWS;
								ws = true;
							}
						} else {
							trailWS = (*orderingHndl)[runIndex].len - (*orderingHndl)[runIndex].leftLen;
							if(trailWS != 0L) {
								len -= trailWS;
								ws = true;
							}
						}
					}
				}
				/* Simply draw the text */
				DrawJustified((Ptr)*(**docInfo).theText + offset, len, 0L, styleRunPosition, scaling, scaling);

				if((**docInfo).flags.drawDebugSymbols) {
					if(((Ptr)*(**docInfo).theText + (((*orderingHndl)[runIndex].offset - (**docInfo).textOffset)))[(*orderingHndl)[runIndex].len - 1L] == kCarriageReturnChar) {
						Move(2,-2);
						Line(0,1);
						Line(-1,-1);
						Line(1,-1);
						Line(0,1);
						Line(2,0);
						Line(0,-2);
						Move(1,4);
					}
				}
				
				HSetState((Handle)(**docInfo).theText, hState);
			}
			
			SetSysDirection(curDirection);

		} else {
			if((graphicInfo = tempStyle.stInfo.graphicStyle.graphicInfo) != nil) {
				
				/* Call the routine to draw the graphic */
				if((**graphicInfo).itemProc == nil) {
					thePict = (**(PictGraphicInfoHandle)graphicInfo).pict;
					pictRect = (**thePict).picFrame;
					OffsetRect(&pictRect, penLoc.h - pictRect.right, penLoc.v - pictRect.bottom);
					DrawPicture(thePict, &pictRect);
				} else {
					errCode = CallGraphic(docInfo, graphicInfo, (*orderingHndl)[runIndex].offset, peGraphicDraw, &tempStyle.stInfo.graphicStyle);
					if(errCode == noErr) {
						SetEmptyRgn(scratchRgn);
						CallGraphic(docInfo, graphicInfo, (*orderingHndl)[runIndex].offset, peGraphicRegion, scratchRgn);
						UnionRgn(scratchRgn, (**docInfo).graphicRgn, (**docInfo).graphicRgn);
						if((**graphicInfo).drawInWindow && offscreen) {
							DiffRgn(drawRgn, scratchRgn, drawRgn);
						}
					}
					SetClip(drawRgn);
				}
			}
			
			MoveTo(penLoc.h, penLoc.v);
		}
NextRun :
		styleRunPosition = (runIndex == numRuns - 2) ? rightStyleRun : middleStyleRun;
	}
	
	if((!(**docInfo).paraArray[lineCache->paraIndex].paraLSDirty) &&
	   ((**paraInfo).lineStarts != nil) &&
	   (*(**paraInfo).lineStarts != nil)) {
		(*(**paraInfo).lineStarts)[lineCache->lineIndex].lsULFlag = (ws || rightWS != 0);
	}
	
	return errCode;
}

OSErr RedrawParagraph(DocumentInfoHandle docInfo, long paraIndex, long startChar, long endChar, LSTable oldLineStarts)
{
	LSTable lineStarts;
	long lineCount, oldLineCount, firstDiffLine, oldNextSameLine, newNextSameLine;
	PETEPortInfo savedPortInfo;
	long topDiffPosition, oldScrollPosition, newScrollPosition, windowHeight, scrollDistance, vPosition;
	Rect tempRect;
	RgnHandle tempRgn;
	OSErr errCode;
	Byte hState;
	
	errCode = ParagraphPosition(docInfo, paraIndex, &vPosition);
	if(errCode == noErr) {
		errCode = ReflowParagraph(docInfo, paraIndex, startChar, endChar);
	}
	if(errCode != noErr) {
		if(errCode == cantDoThatInCurrentMode) {
			InvalidateDocument(docInfo, true);
		}
		SetParaDirty(docInfo, paraIndex);
		return errCode;
	}
	topDiffPosition = oldScrollPosition = newScrollPosition = vPosition;
	
	lineStarts = (**(**docInfo).paraArray[paraIndex].paraInfo).lineStarts;
	hState = HGetState((Handle)lineStarts);
	HNoPurge((Handle)lineStarts);
	
	lineCount = InlineGetHandleSize((Handle)lineStarts) / sizeof(LSElement) - 1L;
	oldLineCount = InlineGetHandleSize((Handle)oldLineStarts) / sizeof(LSElement) - 1L;

	for(firstDiffLine = 0L;
	    (firstDiffLine < lineCount) &&
	    (firstDiffLine < oldLineCount) &&
		(startChar > (*oldLineStarts)[firstDiffLine + 1L].lsStartChar) &&
		((*lineStarts)[firstDiffLine + 1L].lsStartChar == (*oldLineStarts)[firstDiffLine + 1L].lsStartChar);
	    ++firstDiffLine) {
	
		;
	}
	
	for(oldNextSameLine = oldLineCount, newNextSameLine = lineCount;
	    (oldNextSameLine > 0L) &&
	    (newNextSameLine > 0L) &&
	    (endChar < (*lineStarts)[newNextSameLine - 1L].lsStartChar) &&
	    (endChar < (*oldLineStarts)[oldNextSameLine - 1L].lsStartChar) &&
	    ((*lineStarts)[newNextSameLine - 1L].lsStartChar == (*oldLineStarts)[oldNextSameLine - 1L].lsStartChar);
	    --oldNextSameLine, --newNextSameLine) {
	
		;
	}
	
	if(firstDiffLine != 0L) {
		topDiffPosition += ResetLineCache((**docInfo).paraArray[paraIndex].paraInfo, firstDiffLine);
	}
	topDiffPosition -= (**docInfo).vPosition;

	if(oldNextSameLine > firstDiffLine) {
		oldScrollPosition += (**(**docInfo).paraArray[paraIndex].paraInfo).lineStartCacheHeight;
	}
	oldScrollPosition += LineHeights(oldLineStarts, (oldNextSameLine > firstDiffLine) ? firstDiffLine : 0L, oldNextSameLine) - (**docInfo).vPosition;

	if(newNextSameLine != 0L) {
		newScrollPosition += ResetLineCache((**docInfo).paraArray[paraIndex].paraInfo, newNextSameLine);
	}
	newScrollPosition -= (**docInfo).vPosition;
	
	HSetState((Handle)lineStarts, hState);
	
	windowHeight = RectHeight(&(**docInfo).viewRect);

	if(topDiffPosition >= windowHeight) {
		return noErr;
	}

	if(topDiffPosition < 0L) {
		topDiffPosition = 0L;
	}

	scrollDistance = newScrollPosition - oldScrollPosition;

	if((oldScrollPosition < 0L) && (newScrollPosition < 0L)) {
		(**docInfo).vPosition += scrollDistance;
		return noErr;
	}
	
	if(oldScrollPosition < 0L) {
		oldScrollPosition = 0L;
	}

	if(newScrollPosition < 0L) {
		newScrollPosition = 0L;
	}

	tempRgn = NewRgn();
	if(tempRgn == nil) {
		return memFullErr;
	}

	/* Save old port info */
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	
	tempRect = (**docInfo).viewRect;
	if(oldScrollPosition < windowHeight) {
		tempRect.top += (oldScrollPosition < newScrollPosition) ? oldScrollPosition : newScrollPosition;
		if(scrollDistance != 0) {
			ScrollRectInDoc(docInfo, &tempRect, 0, scrollDistance, tempRgn);
		}
	} else {
		tempRect.top += newScrollPosition;
		RectRgn(tempRgn, &tempRect);
	}
	
	tempRect = (**docInfo).viewRect;
	tempRect.top += topDiffPosition;
	if(newScrollPosition < windowHeight) {
		tempRect.bottom = (**docInfo).viewRect.top + newScrollPosition;
	}
	if(!EmptyRect(&tempRect)) {
		RectAndRgn(tempRgn, (**(**docInfo).globals).scratchRgn, &tempRect);
	}
	
	DiffRgn((**docInfo).hiliteRgn, tempRgn, (**docInfo).hiliteRgn);
	DiffRgn((**docInfo).graphicRgn, tempRgn, (**docInfo).graphicRgn);
	(**docInfo).flags.selectionDirty = true;
	
	errCode = DrawDocument(docInfo, tempRgn, false);

	DisposeRgn(tempRgn);

	ResetPortInfo(&savedPortInfo);
	
	return errCode;

}

OSErr RedrawDocument(DocumentInfoHandle docInfo, long textOffset)
{
	SelData selection;
	RgnHandle clipRgn;
	long rectHeight;
	Rect viewRect;
	OSErr errCode;

	selection.paraIndex = ParagraphIndex(docInfo, textOffset);
	selection.offset = textOffset;
	selection.lastLine = (textOffset != 0L);
	errCode = OffsetToVPosition(docInfo, &selection, false);
	if(errCode != noErr) {
		if(errCode == cantDoThatInCurrentMode) {
			InvalidateDocument(docInfo, true);
		}
		return errCode;
	}
	viewRect = (**docInfo).viewRect;
	selection.vPosition -= (**docInfo).vPosition;
	rectHeight = RectHeight(&viewRect);
	if(selection.vPosition < rectHeight) {
		if(selection.vPosition > 0L) {
			viewRect.top += selection.vPosition;
		}

		clipRgn = NewRgn();
		if(clipRgn != nil) {
			RectRgn(clipRgn, &viewRect);
			DiffRgn((**docInfo).hiliteRgn, clipRgn, (**docInfo).hiliteRgn);
			(**docInfo).flags.selectionDirty = true;
			errCode = DrawDocument(docInfo, clipRgn, false);
			DisposeRgn(clipRgn);
		} else {
			errCode = memFullErr;
		}
	} else {
		errCode = noErr;
	}
	
	if(errCode == cantDoThatInCurrentMode) {
		InvalidateDocument(docInfo, true);
	}
	return errCode;
}

OSErr ScrollAndDrawDocument(DocumentInfoHandle docInfo, long vPosition, long distance)
{
	long rectHeight;
	Rect viewRect;
	short position, scrollDistance;
	PETEPortInfo savedPortInfo;
	RgnHandle clipRgn;
	OSErr errCode;

	viewRect = (**docInfo).viewRect;
	rectHeight= RectHeight(&viewRect);
	vPosition -= (**docInfo).vPosition;
	if(vPosition >= rectHeight) {
		return noErr;
	}
	if(vPosition > 0L) {
		position = vPosition;
		if(distance > 0L) {
			viewRect.top += position;
		} else {
			viewRect.bottom = viewRect.top + position;
		}
	}

	scrollDistance = distance;
	if(scrollDistance != 0) {
		clipRgn = NewRgn();
		if(clipRgn != nil) {
			SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
			ScrollRectInDoc(docInfo, &viewRect, 0, scrollDistance, clipRgn);
			errCode = DrawDocument(docInfo, clipRgn, false);
			DisposeRgn(clipRgn);
			ResetPortInfo(&savedPortInfo);
		} else {
			errCode = memFullErr;
		}
	} else {
		errCode = noErr;
	}
	
	return errCode;
}

void EraseCaret(DocumentInfoHandle docInfo)
{
	if((**docInfo).flags.caretOn) {
		DrawCaret(docInfo, nil);
	}
}

void DrawCaret(DocumentInfoHandle docInfo, SelDataPtr selection)
{
	short hFixedPrimary, hFixedSecondary;
	long vWideTop, vWideMiddle, vWideBottom, windowHeight;
	Rect viewRect;
	PETEPortInfo savedPortInfo;
	short vTop, vMiddle, vBottom, hPrimary, hSecondary;
	RgnHandle scratchRgn;
	SelData tempSelection;
	
	if(selection == nil) {
		tempSelection = (**docInfo).selStart;
		selection = &tempSelection;
	}
	
	viewRect = (**docInfo).viewRect;
	vWideTop = selection->vPosition;
	vWideTop -= (**docInfo).vPosition;
	vWideTop += 1;
	vWideBottom = vWideMiddle = vWideTop;
	vWideMiddle += selection->vLineHeight >> 1;
	vWideBottom += selection->vLineHeight;

	windowHeight = RectHeight(&viewRect);

	if((vWideBottom < 0L) || (vWideTop >= windowHeight)) {
		return;
	}

	hFixedPrimary = selection->lPosition - (**docInfo).hPosition;
	hFixedSecondary = selection->rPosition - (**docInfo).hPosition;
	if((**(**docInfo).paraArray[selection->paraIndex].paraInfo).direction != leftCaret) {
		Exchange(hFixedPrimary, hFixedSecondary);
	}

	if(!(GetScriptManagerVariable(smGenFlags) & (1L << smfDualCaret))) {
		if(((**(**docInfo).paraArray[selection->paraIndex].paraInfo).direction != leftCaret) ==
		   (!(Boolean)GetScriptVariable((short)GetScriptManagerVariable(smKeyScript), smScriptRight))) {
			hFixedPrimary = hFixedSecondary;
		} else {
			hFixedSecondary = hFixedPrimary;
		}
	}
	

	hPrimary = hFixedPrimary - 1 + viewRect.left;
	hSecondary = hFixedSecondary - 1 + viewRect.left;
	vTop = vWideTop - 1 + viewRect.top;
	vMiddle = vWideMiddle - 1 + viewRect.top;
	vBottom = vWideBottom - 1 + viewRect.top;

	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	scratchRgn = (**(**docInfo).globals).scratchRgn;
	GetClip(scratchRgn);
	ClipRect(&viewRect);
	
	PenAndColorNormal();
	PenMode(patXor);

	if((hPrimary > viewRect.left) && (hPrimary < viewRect.right) && (vMiddle >= viewRect.top)) {
		if(vTop < viewRect.top) {
			vTop = viewRect.top;
		}
		if(vMiddle >= viewRect.bottom) {
			vMiddle = viewRect.bottom - 1;
		}
		MoveTo(hPrimary, vTop);
		LineTo(hPrimary, vMiddle);
	}
	if((hSecondary > viewRect.left) && (hSecondary < viewRect.right) && (vMiddle + 1 < viewRect.bottom)) {
		if(vMiddle + 1 < viewRect.top) {
			vMiddle = viewRect.top - 1;
		}
		if(vBottom >= viewRect.bottom) {
			vBottom = viewRect.bottom - 1;
		}
		MoveTo(hSecondary, vMiddle + 1);
		LineTo(hSecondary, vBottom);
	}

	SetClip(scratchRgn);
	ResetPortInfo(&savedPortInfo);
	
	if(selection == &tempSelection) {
		(**docInfo).flags.caretOn = !(**docInfo).flags.caretOn;
	}

}

void InvalidateDocument(DocumentInfoHandle docInfo, Boolean viewOnly)
{
	PETEPortInfo savedPortInfo;
	Rect tempRect;
	
	if((**docInfo).printData == nil) {
		tempRect = (viewOnly && !((**docInfo).flags.scrollsDirty)) ? (**docInfo).viewRect : (**docInfo).docRect;
		if((**docInfo).flags.recalcOff) {
			RectAndRgn((**docInfo).updateRgn, (**(**docInfo).globals).scratchRgn, &tempRect);
		} else {
			SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
			InvalWindowRect((**docInfo).docWindow,&tempRect);
			ResetPortInfo(&savedPortInfo);
		}
	}
}

void InvalidateDocUpdate(DocumentInfoHandle docInfo)
{
	PETEPortInfo savedPortInfo;
	
	if(!EmptyRgn((**docInfo).updateRgn)) {
		SavePortInfo(((**docInfo).printData == nil) ? GetWindowPort((**docInfo).docWindow) : (**(**docInfo).printData).pPort, &savedPortInfo);
		InvalWindowRgn((**docInfo).docWindow,(**docInfo).updateRgn);
		ResetPortInfo(&savedPortInfo);
		SetEmptyRgn((**docInfo).updateRgn);
	}
}

OSErr CreateDocPict(DocumentInfoHandle docInfo, PicHandle *thePic)
{
	PicHandle newPic;
	OpenCPicParams picHeader;
	OSErr errCode;
	PETEPortInfo savedPortInfo;
	RgnHandle oldVisRgn, oldClipRgn, tempRgn;
	DocFlagsBlock flags;
	
	errCode = memFullErr;
	tempRgn = NewRgn();
	if(tempRgn == nil) {
		goto JustReturn;
	}
	
	oldVisRgn = NewRgn();
	if(oldVisRgn == nil) {
		goto DisposeTemp;
	}
	
	oldClipRgn = NewRgn();
	if(oldVisRgn == nil) {
		goto DisposeVis;
	}
	
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	
	EraseCaret(docInfo);
	
	picHeader.srcRect = (**docInfo).docRect;
	picHeader.hRes = 0x00480000;
	picHeader.vRes = 0x00480000;
	picHeader.version = -2;
	picHeader.reserved1 = 0L;
	picHeader.reserved2 = 0L;
	
	RectRgn(tempRgn, &picHeader.srcRect);
	GetVisibleRgn(oldVisRgn);
	GetClip(oldClipRgn);
	SetVisibleRgn(tempRgn);
	SetClip(tempRgn);
	
	newPic = OpenCPicture(&picHeader);
	
	if((errCode = QDError()) == noErr) {
		flags = (**docInfo).flags;

		(**docInfo).flags.offscreenBitMap = false;
		(**docInfo).flags.scrollsVisible = false;
		(**docInfo).flags.isActive = false;
		(**docInfo).flags.scrollsDirty = false;
		(**docInfo).flags.drawWithoutErase = true;

		errCode = DrawDocument(docInfo, tempRgn, false);

		(**docInfo).flags = flags;

		ClosePicture();
	}
	
	SetClip(oldClipRgn);
	SetVisibleRgn(oldVisRgn);
	
	ResetPortInfo(&savedPortInfo);

	DisposeRgn(oldClipRgn);
	
DisposeVis :
	DisposeRgn(oldVisRgn);
	
DisposeTemp :
	DisposeRgn(tempRgn);
	
JustReturn:
	if(errCode == noErr) {
		*thePic = newPic;
	} else {
		*thePic = nil;
	}
	
	return errCode;
}

OSErr DrawDocumentWithPicture(DocumentInfoHandle docInfo)
{
	PETEPortInfo savedPortInfo;
	Rect drawRect;
	RGBColor backColor;
	OSErr errCode;
	
	/* Save old port info */
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
	
	backColor = DocOrGlobalBGColor(nil, docInfo);
	if(!IsPETEDefaultColor(backColor)) {
		RGBBackColorAndPat(&backColor);
	}
	drawRect = (**docInfo).docRect;
	EraseRect(&drawRect);
	DrawPicture((**(**docInfo).printData).docPic, &drawRect);
	errCode = QDError();
	
	ResetPortInfo(&savedPortInfo);
	
	return errCode;
}

void DrawQuoteMarks(DocumentInfoHandle docInfo, long paraIndex, short lineHeight)
{
	ParagraphInfoHandle paraInfo;
	PenState curPen;
	short hPosition, width, inset;
	Byte quoteLevel, signedLevel;
	Boolean rightToLeft;
	Pattern	gray,black;
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	signedLevel = (**paraInfo).signedLevel;
	quoteLevel = (**paraInfo).quoteLevel + signedLevel;
	if(quoteLevel == 0) {
		return;
	}
	
	rightToLeft = ((**(**docInfo).paraArray[paraIndex].paraInfo).direction != leftCaret);
	hPosition = kDefaultMargin;
	if(rightToLeft) {
		hPosition = (**docInfo).docWidth - hPosition;
	}
	hPosition -= (**docInfo).hPosition;
	
	GetPenState(&curPen);
	PenAndColorNormal();
	PenMode(patXor);
	PenSize(2, 1);
	if(signedLevel > 0) {
		PenPat(GetQDGlobalsGray(&gray));
	}
	
	for(width = RectWidth(&(**docInfo).viewRect), inset = ((rightToLeft ? -kDefaultMargin : kDefaultMargin) * 2);
	    quoteLevel > 0 && hPosition >= 0L && hPosition < width;
	    hPosition += inset, --quoteLevel, --signedLevel) {

		if(signedLevel == 0) {
			PenPat(GetQDGlobalsBlack(&black));
		}
		MoveTo(hPosition + 1 + (**docInfo).viewRect.left, curPen.pnLoc.v);
		Line(0, lineHeight - 1);
	}
	
	SetPenState(&curPen);
		
	return;
}

void DrawOneChar(DocumentInfoHandle docInfo, StringPtr theChar)
{
	PETEPortInfo savedPortInfo;
	short vPosition;
	SelData selection;
	MyTextStyle textStyle;
	Point penLoc;
	Rect drawRect;
	short hPosition;
	LSTable lineStarts;
	Boolean ws = false;
	RGBColor backColor/* = {0xffff,0xffff,0xffff}*/;
	
	SavePortInfo(GetWindowPort((**docInfo).docWindow), &savedPortInfo);
/* Stupid compiler */
	backColor.red = backColor.green = backColor.blue = 0xffff;
	backColor = DocOrGlobalBGColor(nil, docInfo);
	if(!IsPETEDefaultColor(backColor)) {
		RGBBackColorAndPat(&backColor);
	} else {
		GetBackColor(&backColor);
	}
	PenAndColorNormal();
	GetCurrentTextStyle(docInfo, &textStyle);
	SetTextStyleWithDefaults(nil, docInfo, &textStyle, true, false);
	if(IsWhitespace(theChar + 1, theChar[0], smCurrentScript, (**(**docInfo).globals).whitespaceGlobals, nil)) {
		Style face = GetPortFace();
		if(face & underline) {
			ws = true;
			TextFace(face & ~underline);
		}
	}
	TextMode(srcOr);
	selection = (**docInfo).selStart;
	lineStarts = (**(**docInfo).paraArray[selection.paraIndex].paraInfo).lineStarts;
	(*lineStarts)[selection.lineIndex].lsULFlag = ws;
	vPosition = (selection.vPosition - (**docInfo).vPosition) + (*lineStarts)[selection.lineIndex].lsMeasure.ascent;
	hPosition = selection.lPosition - (**docInfo).hPosition + (**docInfo).viewRect.left;
	MoveTo(hPosition, vPosition + (**docInfo).viewRect.top);
	drawRect.top = (**docInfo).viewRect.top + (selection.vPosition - (**docInfo).vPosition);
	drawRect.bottom = drawRect.top + (*lineStarts)[selection.lineIndex].lsMeasure.height;
	drawRect.left = hPosition;
	drawRect.right = hPosition + TextWidth(theChar, 1, theChar[0]);
	EraseRect(&drawRect);
	DrawText(theChar, 1, theChar[0]);
	GetPen(&penLoc);
	selection.hPosition = selection.rPosition = (selection.lPosition += (penLoc.h - hPosition));
	(**docInfo).selStart = (**docInfo).selEnd = selection;
	FlushDocInfoLineCache(docInfo);
	PenAndColorNormal();
	ResetPortInfo(&savedPortInfo);	
}

void DrawListBullet(DocumentInfoHandle docInfo, long paraIndex, short leftPosition, short ascent)
{
	ParagraphInfoHandle paraInfo;
	short position, indentWidth;
	PenState curPen;
	Rect bullet;
	short sAscent;
	
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	if((**paraInfo).paraFlags & peListBits) {
		position = (**docInfo).hPosition;
		position -= leftPosition;
		indentWidth = (((**paraInfo).tabCount == 0) ? (4 * kDefaultMargin) : ((**paraInfo).tabStops[0] / 3));
		position += indentWidth;
		if(position < 0L) {
			GetPenState(&curPen);
			indentWidth = FixMul(indentWidth, 0x0000C000 /* .75 */);
			sAscent = (ascent > indentWidth ? indentWidth : ascent);
			bullet.left = curPen.pnLoc.h - (position);
			bullet.bottom = curPen.pnLoc.v - 1;
			bullet.top = bullet.bottom - (sAscent - 4);
			bullet.right = bullet.left + (sAscent - 4);
			PenAndColorNormal();
			PenMode(patXor);
			switch((**paraInfo).paraFlags & peListBits) {
				case peSquareList :
					FrameRect(&bullet);
					break;
				case peCircleList :
					FrameOval(&bullet);
					break;
				default :
					PaintOval(&bullet);
			}
			SetPenState(&curPen);
		}
	}
}