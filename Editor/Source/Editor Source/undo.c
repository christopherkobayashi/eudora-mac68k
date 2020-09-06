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

void ClearUndo(DocumentInfoHandle docInfo)
{
	if((**docInfo).undoData.theText != nil) {
		DisposeHandle((Handle)(**docInfo).undoData.theText);
		(**docInfo).undoData.theText = nil;
	}
	if((**docInfo).undoData.styleScrap != nil) {
		DeleteScrapStyles((**docInfo).theStyleTable, (**docInfo).undoData.styleScrap);
		DisposeHandle((Handle)(**docInfo).undoData.styleScrap);
		(**docInfo).undoData.styleScrap = nil;
	}
	if((**docInfo).undoData.paraScrap != nil) {
		DisposeHandle((Handle)(**docInfo).undoData.paraScrap);
		(**docInfo).undoData.paraScrap = nil;
	}
	(**docInfo).undoData.selStart = (**docInfo).selStart.offset;
	(**docInfo).undoData.selEnd = (**docInfo).selEnd.offset;
	(**docInfo).undoData.deleteOffset = 0L;
	(**docInfo).undoData.startOffset = 0L;
	(**docInfo).undoData.endOffset = 0L;
	(**docInfo).undoData.undoType = peCantUndo;
}

OSErr SetDeleteUndo(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEUndoEnum undoType)
{
	PETEStyleListHandle styleScrapStart, styleScrapEnd, styleScrapNew;
	PETEParaScrapHandle paraScrapStart, paraScrapEnd, paraScrapNew;
	StringHandle undoTextStart, undoTextEnd, undoTextNew;
	OSErr errCode, tempErr;
	Boolean appendTyping;
	long preStart, preEnd, postStart, postEnd, endTextLen;
	
	if((**docInfo).flags.dontSaveToUndo) {
		if((**docInfo).flags.clearOnDontUndo) {
			ClearUndo(docInfo);
		}
		return noErr;
	}
	
	if(startOffset >= endOffset) {
		return errAEImpossibleRange;
	}

	if(!((**docInfo).flags.ignoreModLock) && SelectionHasLock(docInfo, startOffset, endOffset)) {
		return errAENotModifiable;
	}

	appendTyping = (undoType == peUndoTyping) && ((**docInfo).flags.typingUndoCurrent);
	
	if(appendTyping) {
		if(startOffset < (**docInfo).undoData.startOffset) {
			preStart = startOffset;
		} else {
			preStart = (**docInfo).undoData.startOffset;
		}

		preEnd = (**docInfo).undoData.startOffset;

		if(startOffset < (**docInfo).undoData.endOffset) {
			postStart = (**docInfo).undoData.endOffset;
		} else {
			postStart = startOffset;
		}
		
		if(endOffset < (**docInfo).undoData.endOffset) {
			postEnd = (**docInfo).undoData.endOffset;
		} else {
			postEnd = endOffset;
		}
		
	} else {
		preStart = startOffset;
		preEnd = endOffset;
		postStart = postEnd = 0L;
	}
	
	errCode = noErr;

	if(preStart != preEnd) {
		undoTextStart = (StringHandle)MyNewHandle(preEnd - preStart, &tempErr, hndlTemp);
		if((errCode = tempErr) != noErr) {
			goto ErrorNoMemory;
		}
	} else {
		undoTextStart = nil;
		styleScrapStart = nil;
		paraScrapStart = nil;
	}
	
	if(undoTextStart != nil) {
		errCode = LoadTextIntoHandle(docInfo, preStart, preEnd, (Handle)undoTextStart, 0L);
		if(errCode != noErr) {
			goto ErrorTextStart;
		}
		errCode = GetStyleScrap(docInfo, preStart, preEnd, &styleScrapStart, &paraScrapStart, false, true, true, true);
		if(errCode != noErr) {
			goto ErrorTextStart;
		}
	}
		
	if(postStart != postEnd) {
		undoTextEnd = (StringHandle)MyNewHandle(postEnd - postStart, &tempErr, hndlTemp);
		if((errCode = tempErr) != noErr) {
			goto ErrorStyleStart;
		}
	} else {
		undoTextEnd = nil;
		styleScrapEnd = nil;
		paraScrapEnd = nil;
	}
	
	if(undoTextEnd != nil) {
		errCode = LoadTextIntoHandle(docInfo, postStart, postEnd, (Handle)undoTextEnd, 0L);
		if(errCode != noErr) {
			goto ErrorTextEnd;
		}
		errCode = GetStyleScrap(docInfo, postStart, postEnd, &styleScrapEnd, &paraScrapEnd, false, true, true, true);
		if(errCode != noErr) {
			goto ErrorTextEnd;
		}
	}
	
	styleScrapNew = nil;
	paraScrapNew = nil;
	undoTextNew = undoTextStart;
	if(undoTextNew != nil) {
		endTextLen = InlineGetHandleSize((Handle)undoTextStart);
		if((errCode = MyHandToHand((Handle *)&undoTextNew, 0)) == noErr) {
			styleScrapNew = styleScrapStart;
			if((errCode = MyHandToHand((Handle *)&styleScrapNew, 0)) == noErr) {
				if(paraScrapStart != nil) {
					paraScrapNew = paraScrapStart;
					if((errCode = MyHandToHand((Handle *)&paraScrapNew, 0)) != noErr) {
						paraScrapNew = nil;
					} else {
						endTextLen = GetEndTextLen(paraScrapStart, endTextLen);
					}
				}
			} else {
				styleScrapNew = nil;
			}
		} else {
			undoTextNew = nil;
		}
	} else {
		endTextLen = 0L;
	}
	
	if((errCode == noErr) && appendTyping && ((**docInfo).undoData.theText != nil)) {
		errCode = AppendToScrap(&undoTextNew, &styleScrapNew, &paraScrapNew, (**docInfo).undoData.theText, (**docInfo).undoData.styleScrap, (**docInfo).undoData.paraScrap, &endTextLen, false);
	}
	
	if((errCode == noErr) && appendTyping && (undoTextEnd != nil)) {
		errCode = AppendToScrap(&undoTextNew, &styleScrapNew, &paraScrapNew, undoTextEnd, styleScrapEnd, paraScrapEnd, &endTextLen, false);
	}
	
	if(errCode == noErr) {
	
		if(styleScrapNew != nil) {
			AddScrapStyles((**docInfo).theStyleTable, styleScrapNew);
		}

		ExchangePtr(undoTextNew, (**docInfo).undoData.theText);
		ExchangePtr(styleScrapNew, (**docInfo).undoData.styleScrap);
		ExchangePtr(paraScrapNew, (**docInfo).undoData.paraScrap);
	
		if(styleScrapNew != nil) {
			DeleteScrapStyles((**docInfo).theStyleTable, styleScrapNew);
		}

		if(appendTyping && ((**docInfo).undoData.startOffset != (**docInfo).undoData.endOffset)) {
			if(startOffset > (**docInfo).undoData.startOffset) {
				if(endOffset < (**docInfo).undoData.endOffset) {
					(**docInfo).undoData.endOffset = startOffset + ((**docInfo).undoData.endOffset - endOffset);
				} else if(startOffset < (**docInfo).undoData.endOffset) {
					(**docInfo).undoData.endOffset = startOffset;
				}
			} else if(endOffset > (**docInfo).undoData.startOffset) {
				if(endOffset >= (**docInfo).undoData.endOffset) {
					(**docInfo).undoData.endOffset = startOffset;
				} else {
					(**docInfo).undoData.endOffset = (**docInfo).undoData.startOffset + ((**docInfo).undoData.endOffset - endOffset);
				}
				(**docInfo).undoData.startOffset = startOffset;
			} else {
				(**docInfo).undoData.startOffset -= endOffset - startOffset;
				(**docInfo).undoData.endOffset -= endOffset - startOffset;
			}
		} else {
			(**docInfo).undoData.startOffset = (**docInfo).undoData.endOffset = startOffset;
		}
		(**docInfo).undoData.deleteOffset = (**docInfo).undoData.endOffset;
		
		(**docInfo).flags.typingUndoCurrent = (undoType == peUndoTyping);
		(**docInfo).undoData.undoType = undoType;
	}
	
	if(paraScrapNew != nil) {
		DisposeHandle((Handle)paraScrapNew);
	}
	if(styleScrapNew != nil) {
		DisposeHandle((Handle)styleScrapNew);
	}
	if(undoTextNew != nil) {
		DisposeHandle((Handle)undoTextNew);
	}
ErrorStyleEnd :
	if(paraScrapEnd != nil) {
		DisposeHandle((Handle)paraScrapEnd);
	}
	if(styleScrapEnd != nil) {
		DisposeHandle((Handle)styleScrapEnd);
	}
ErrorTextEnd :
	if(undoTextEnd != nil) {
		DisposeHandle((Handle)undoTextEnd);
	}
ErrorStyleStart :
	if(paraScrapStart != nil) {
		DisposeHandle((Handle)paraScrapStart);
	}
	if(styleScrapStart != nil) {
		DisposeHandle((Handle)styleScrapStart);
	}
ErrorTextStart :
	if(undoTextStart != nil) {
		DisposeHandle((Handle)undoTextStart);
	}
ErrorNoMemory :
	
	if(errCode == noErr) {
		if(!appendTyping) {
			(**docInfo).undoData.selStart = (**docInfo).selStart.offset;
			(**docInfo).undoData.selEnd = (**docInfo).selEnd.offset;
		}
		return noErr;
	} else {
		return errAECantUndo;
	}
}

long GetEndTextLen(PETEParaScrapHandle paraScrap, long endTextLen)
{
	PETEParaScrapPtr tempParaScrap;
	long paraScrapLen;

	paraScrapLen = InlineGetHandleSize((Handle)paraScrap);
	tempParaScrap = *paraScrap;
	while(tempParaScrap != (PETEParaScrapPtr)(((Ptr)*paraScrap) + paraScrapLen)) {
		endTextLen -= tempParaScrap->paraLength;
		tempParaScrap = (PETEParaScrapPtr)(((Ptr)tempParaScrap) + sizeof(PETEParaScrapEntry) + (sizeof(tempParaScrap->tabStops[0]) * ABS(tempParaScrap->tabCount)));
	}
	
	return endTextLen;
}

OSErr AppendToScrap(StringHandle *undoTextNew, PETEStyleListHandle *styleScrapNew, PETEParaScrapHandle *paraScrapNew, StringHandle undoText, PETEStyleListHandle styleScrap, PETEParaScrapHandle paraScrap, long *endTextLen, Boolean tempMem)
{
	OSErr errCode;
	long tempTextLen, paraScrapLen, oldStyleCount, newStyleCount;
	PETEStyleList tempScrapEntry;
	
	errCode = noErr;
	if(*undoTextNew != nil) {
		tempTextLen = InlineGetHandleSize((Handle)*undoTextNew);
		if(tempTextLen < 0L) {
			errCode = tempTextLen;
		}

		if(errCode == noErr) {
			oldStyleCount = InlineGetHandleSize((Handle)styleScrap);
			if(oldStyleCount < 0L) {
				errCode = oldStyleCount;
			} else {
				oldStyleCount /= sizeof(PETEStyleEntry);
			}
		}
		if(errCode == noErr) {
			newStyleCount = InlineGetHandleSize((Handle)*styleScrapNew);
			if(newStyleCount < 0L) {
				errCode = newStyleCount;
			} else {
				newStyleCount /= sizeof(PETEStyleEntry);
			}
		}
		if(errCode == noErr) {
			errCode = HandAndHand((Handle)undoText, (Handle)*undoTextNew);
		}
		if(errCode == noErr) {
			errCode = HandAndHand((Handle)styleScrap, (Handle)*styleScrapNew);
			if(errCode == noErr) {
				for(tempScrapEntry = **styleScrapNew + newStyleCount; --oldStyleCount >= 0L; ++tempScrapEntry) {
					tempScrapEntry->psStartChar += tempTextLen;
				}
			}
		}
	} else {
		*undoTextNew = undoText;
		errCode = MyHandToHand((Handle *)undoTextNew, tempMem ? hndlTemp : 0);
		if(errCode == noErr) {
			*styleScrapNew = styleScrap;
			errCode = MyHandToHand((Handle *)styleScrapNew, tempMem ? hndlTemp : 0);
			if(errCode != noErr) {
				*styleScrapNew = nil;
			}
		} else {
			*undoTextNew = nil;
		}
	}
	
	if(errCode == noErr) {
		tempTextLen = InlineGetHandleSize((Handle)undoText);
		if(tempTextLen < 0L) {
			errCode = tempTextLen;
		}
	}
	
	if(errCode == noErr) {
		if(paraScrap == nil) {
			*endTextLen += tempTextLen;
		} else {
			if(*paraScrapNew != nil) {
				paraScrapLen = InlineGetHandleSize((Handle)*paraScrapNew);
				errCode = HandAndHand((Handle)paraScrap, (Handle)*paraScrapNew);
				if(errCode == noErr) {
					((PETEParaScrapPtr)((Ptr)**paraScrapNew + paraScrapLen))->paraLength += *endTextLen;
				}
			} else {	
				*paraScrapNew = paraScrap;
				errCode = MyHandToHand((Handle *)paraScrapNew, tempMem ? hndlTemp : 0);
				if(errCode != noErr) {
					*paraScrapNew = nil;
				} else {
					(***paraScrapNew).paraLength += *endTextLen;
				}
			}
			if(errCode == noErr) {
				*endTextLen = GetEndTextLen(paraScrap, tempTextLen);
			}
		}
	}
	return errCode;
}

void SetInsertUndo(DocumentInfoHandle docInfo, long startOffset, long textLen, long selStart, long selEnd, PETEUndoEnum undoType, Boolean append)
{
	if((**docInfo).flags.dontSaveToUndo) {
		if((**docInfo).flags.clearOnDontUndo) {
			ClearUndo(docInfo);
		}
		return;
	}
		
	if(append || ((undoType == peUndoTyping) && ((**docInfo).flags.typingUndoCurrent))) {
		(**docInfo).undoData.endOffset += textLen;
		(**docInfo).undoData.deleteOffset += textLen;
	} else {
		if(selStart >= 0L) {
			ClearUndo(docInfo);
			(**docInfo).undoData.selStart = selStart;
			(**docInfo).undoData.selEnd = selEnd;
		}
		(**docInfo).undoData.startOffset = startOffset;
		(**docInfo).undoData.endOffset = startOffset + textLen;
	}
	(**docInfo).undoData.undoType = undoType;
	
	(**docInfo).flags.typingUndoCurrent = (undoType == peUndoTyping);
}

OSErr SetParaStyleUndo(DocumentInfoHandle docInfo, long startOffset, long endOffset, PETEUndoEnum undoType)
{
	OSErr errCode;
	PETEStyleListHandle styleScrap;
	PETEParaScrapHandle paraScrap, *paraScrapPtr;
	
	if((**docInfo).flags.dontSaveToUndo) {
		if((**docInfo).flags.clearOnDontUndo) {
			ClearUndo(docInfo);
		}
		return noErr;
	}
	
	styleScrap = nil;
	paraScrap = nil;
	paraScrapPtr = nil;
	switch(undoType) {
		case peUndoStyleAndPara :
		case peRedoStyleAndPara :
		case peUndoPara :
		case peRedoPara :
			paraScrapPtr = &paraScrap;
		case peUndoStyle :
		case peRedoStyle :
			errCode = GetStyleScrap(docInfo, startOffset, endOffset, &styleScrap, paraScrapPtr, true, false, false, true);
			if((undoType == peUndoPara) || (undoType == peRedoPara)) {
				if(styleScrap != nil) {
					DisposeHandle((Handle)styleScrap);
					styleScrap = nil;
				}
			} else if(errCode == noErr) {
				if(styleScrap != nil) {
					AddScrapStyles((**docInfo).theStyleTable, styleScrap);
				} else {
					errCode = errAEImpossibleRange;
				}
			}
	}
	
	if(errCode == noErr) {
		ClearUndo(docInfo);
		(**docInfo).undoData.startOffset = (startOffset >= 0L) ? startOffset : (**docInfo).selStart.offset;
		(**docInfo).undoData.endOffset = (endOffset >= 0L) ? endOffset : (**docInfo).selEnd.offset;
		(**docInfo).undoData.styleScrap = styleScrap;
		(**docInfo).undoData.paraScrap = paraScrap;
		(**docInfo).undoData.selStart = (**docInfo).selStart.offset;
		(**docInfo).undoData.selEnd = (**docInfo).selEnd.offset;
		(**docInfo).undoData.undoType = undoType;
		(**docInfo).flags.typingUndoCurrent = false;
	}
	return errCode;
}

OSErr MyDoUndo(DocumentInfoHandle docInfo)
{
	OSErr errCode;
	StringHandle undoText;
	PETEStyleListHandle styleScrap;
	PETEParaScrapHandle paraScrap;
	long startOffset, endOffset, deleteOffset, selStart, selEnd, oldSelStart, oldSelEnd, textLen;
	Boolean wasRecalcOn, wasLockIgnored;
	Byte tempLock;
	SelData selection;
	
	undoText = nil;
	styleScrap = nil;
	paraScrap = nil;
	startOffset = endOffset = deleteOffset = 0L;
	
	ExchangePtr((**docInfo).undoData.theText, undoText);
	ExchangePtr((**docInfo).undoData.styleScrap, styleScrap);
	ExchangePtr((**docInfo).undoData.paraScrap, paraScrap);
	Exchange((**docInfo).undoData.deleteOffset, deleteOffset);
	Exchange((**docInfo).undoData.startOffset, startOffset);
	Exchange((**docInfo).undoData.endOffset, endOffset);
	oldSelStart = (**docInfo).selStart.offset;
	oldSelEnd = (**docInfo).selEnd.offset;
	selStart = (**docInfo).undoData.selStart;
	selEnd = (**docInfo).undoData.selEnd;
	
	wasRecalcOn = !((**docInfo).flags.recalcOff);
	(**docInfo).flags.recalcOff = true;
	
	if(undoText != nil) {
		tempLock = (**docInfo).curTextStyle.tsLock;
		(**docInfo).curTextStyle.tsLock = peNoLock;
	
		textLen = InlineGetHandleSize((Handle)undoText);
		(**docInfo).undoData.startOffset = deleteOffset;
		(**docInfo).undoData.endOffset = deleteOffset + textLen;
		errCode = PasteText(docInfo, deleteOffset, undoText, styleScrap, paraScrap, false, false);
		(**docInfo).curTextStyle.tsLock = tempLock;
	} else {
		textLen = 0L;
		errCode = noErr;
	}
	
	if(errCode == noErr) {
		switch((**docInfo).undoData.undoType) {
			case peUndoStyleAndPara :
			case peRedoStyleAndPara :
			case peUndoStyle :
			case peRedoStyle :
			case peUndoPara :
			case peRedoPara :
				errCode = SetParaStyleUndo(docInfo, startOffset, endOffset, (PETEUndoEnum)-(**docInfo).undoData.undoType);
				switch((**docInfo).undoData.undoType) {
					case peUndoStyleAndPara :
					case peRedoStyleAndPara :
					case peUndoStyle :
					case peRedoStyle :
						if((errCode == noErr) && (startOffset != endOffset)) {
							errCode = ApplyStyleList(docInfo, startOffset, endOffset, styleScrap);
						}
						if((errCode != noErr) || ((**docInfo).undoData.undoType == peUndoStyle) || ((**docInfo).undoData.undoType == peRedoStyle)) {
							break;
						}
					case peUndoPara :
					case peRedoPara :
						errCode = ApplyParaScrapInfo(docInfo, startOffset, paraScrap);
						if((errCode != noErr) && ((**docInfo).undoData.styleScrap != nil)) {
							ApplyStyleList(docInfo, startOffset, endOffset, (**docInfo).undoData.styleScrap);
						}
				}
				break;
			default :
				if(startOffset != endOffset) {
					wasLockIgnored = (**docInfo).flags.ignoreModLock;
					if(!wasLockIgnored) {
						(**docInfo).flags.ignoreModLock = true;
					}
					errCode = SetDeleteUndo(docInfo, startOffset, endOffset, (PETEUndoEnum)-(**docInfo).undoData.undoType);
					if(errCode == noErr) {
						if(undoText != nil) {
							(**docInfo).undoData.startOffset = deleteOffset;
							(**docInfo).undoData.endOffset = deleteOffset + textLen;
						}
						errCode = DeleteText(docInfo, startOffset, endOffset);
					}
					if((errCode != noErr) && (textLen != 0L)) {
						DeleteText(docInfo, deleteOffset, deleteOffset + textLen);
					}
					if(!wasLockIgnored) {
						(**docInfo).flags.ignoreModLock = false;
					}
				}
		}
	}
	
	if(wasRecalcOn) {
		(**docInfo).flags.recalcOff = false;
	}
	
	InvalidateDocument(docInfo, true);
	
	if(errCode == noErr) {
		if(styleScrap != nil) {
			DeleteScrapStyles((**docInfo).theStyleTable, styleScrap);
		}
		(**docInfo).flags.typingUndoCurrent = false;
		(**docInfo).undoData.selStart = oldSelStart;
		(**docInfo).undoData.selEnd = oldSelEnd;
		(**docInfo).selStart.offset = selStart;
		(**docInfo).selEnd.offset = selEnd;
		CalcSelectionRgn(docInfo, false);
		selection = (**docInfo).selStart;
		MakeSelectionVisible(docInfo, &selection);
	} else {
		ExchangePtr((**docInfo).undoData.theText, undoText);
		ExchangePtr((**docInfo).undoData.styleScrap, styleScrap);
		ExchangePtr((**docInfo).undoData.paraScrap, paraScrap);
		Exchange((**docInfo).undoData.deleteOffset, deleteOffset);
		Exchange((**docInfo).undoData.startOffset, startOffset);
		Exchange((**docInfo).undoData.endOffset, endOffset);
		Exchange((**docInfo).undoData.selStart, selStart);
		Exchange((**docInfo).undoData.selEnd, selEnd);
	}

	DisposeHandle((Handle)paraScrap);
	DisposeHandle((Handle)styleScrap);
	DisposeHandle((Handle)undoText);
	
	return errCode;
}

OSErr SetUndoFlag(DocumentInfoHandle docInfo, Boolean allowUndo, Boolean clearUndo)
{
	(**docInfo).flags.dontSaveToUndo = !allowUndo;
	(**docInfo).flags.clearOnDontUndo = clearUndo;
	return noErr;
}

OSErr PunctuateUndo(DocumentInfoHandle docInfo)
{
	(**docInfo).flags.typingUndoCurrent = false;
	return noErr;
}