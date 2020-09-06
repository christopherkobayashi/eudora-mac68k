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

OSErr GetDocInfo(DocumentInfoHandle docInfo, PETEDocInfoPtr info)
{
	LongStyleRun tempStyleRun;
	long paraIndex, styleRunIndex;
	ParagraphInfoHandle paraInfo;
	
	info->selStart = (**docInfo).selStart.offset;
	info->selStop = (**docInfo).selEnd.offset;
	
	paraIndex = ParagraphIndex(docInfo, info->selStart);
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	info->styleRunStart = info->selStart - ParagraphOffset(docInfo, paraIndex);
	styleRunIndex = StyleRunIndex(paraInfo, &info->styleRunStart);
	info->styleRunStart = info->selStart - info->styleRunStart;
	info->styleRunStop = info->styleRunStart + GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
	
	info->docHeight = (**docInfo).docHeight;
	info->paraCount = (**docInfo).paraCount;
	info->docRect = (**docInfo).docRect;
	info->viewRect = (**docInfo).viewRect;

	GetCurrentTextStyle(docInfo, &info->curTextStyle);
	info->dirty = (**docInfo).dirty;
	info->docWindow = (**docInfo).docWindow;
	info->recalcOn = !((**docInfo).flags.recalcOff);
	info->doubleClick = !(!((**docInfo).flags.doubleClick));
	info->docActive = !(!((**docInfo).flags.isActive));
	info->printing = ((**docInfo).printData != nil);
	info->filler5 = false;
	info->filler6 = false;
	info->filler7 = false;
	info->filler8 = false;
	info->undoType = (**docInfo).undoData.undoType;
	info->padding = 0;
	return noErr;
}

OSErr SetDirty(DocumentInfoHandle docInfo, long startOffset, long endOffset, Boolean dirty)
{
	if(dirty) {
		if((**docInfo).callbacks.docChanged != nil) {
			CallPETEDocHasChangedProc((**docInfo).callbacks.docChanged, docInfo, startOffset, endOffset);
		}
		if(++(**docInfo).dirty == 0L) {
			++(**docInfo).dirty;
		}
	} else {
		(**docInfo).dirty = 0L;
	}
	return noErr;
}

OSErr SetHonorLock(DocumentInfoHandle docInfo, Byte honorLockBits)
{
	(**docInfo).flags.ignoreModLock = !(honorLockBits & peModLock);
	(**docInfo).flags.ignoreSelectLock = !(honorLockBits & peSelectLock);
	return noErr;
}

long GetDocSize(DocumentInfoHandle docInfo)
{
	long docSize = 0L;
	long paraIndex;
	ParagraphInfoHandle paraInfo;
	
	docSize += InlineGetHandleSize((Handle)(**docInfo).hiliteRgn);

	docSize += InlineGetHandleSize((Handle)(**docInfo).graphicRgn);

	docSize += InlineGetHandleSize((Handle)(**docInfo).updateRgn);

	if((**docInfo).lineCache.orderingHndl != nil) {
		docSize += InlineGetHandleSize((Handle)(**docInfo).lineCache.orderingHndl);
	}

	if((**docInfo).vScroll != nil) {
		docSize += InlineGetHandleSize((Handle)(**docInfo).vScroll);
	}

	if((**docInfo).hScroll != nil) {
		docSize += InlineGetHandleSize((Handle)(**docInfo).hScroll);
	}

	if(((**docInfo).theText != nil) && (*(**docInfo).theText != nil)) {
		docSize += InlineGetHandleSize((Handle)(**docInfo).theText);
	}

	docSize += InlineGetHandleSize((Handle)(**docInfo).theStyleTable);

	for(paraIndex = (**docInfo).paraCount; --paraIndex >= 0L; ) {
		paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
		
		if(((**paraInfo).lineStarts != nil) && (*(**paraInfo).lineStarts != nil)) {
			docSize += InlineGetHandleSize((Handle)(**paraInfo).lineStarts);
		}
		
		docSize += InlineGetHandleSize((Handle)paraInfo);
	}

	docSize += InlineGetHandleSize((Handle)docInfo);
	
	return docSize;
}

Boolean IsSelectionLocked(DocumentInfoHandle docInfo, long startOffset, long endOffset)
{
	if((**docInfo).flags.ignoreModLock) {
		return false;
	}
	
	if(startOffset < 0L) {
		startOffset = (**docInfo).selStart.offset;
	} else if(startOffset > (**docInfo).textLen) {
		startOffset = (**docInfo).textLen;
	}
	
	if(endOffset < 0L) {
		endOffset = (**docInfo).selEnd.offset;
	} else if(endOffset > (**docInfo).textLen) {
		endOffset = (**docInfo).textLen;
	}
	
	if(endOffset < startOffset) {
		endOffset = startOffset;
	}
	
	if((startOffset == endOffset) && (startOffset == (**docInfo).selStart.offset) && (endOffset == (**docInfo).selEnd.offset)) {
		return ((**docInfo).curTextStyle.tsLock != peNoLock);
	} else {
		return SelectionHasLock(docInfo, startOffset, endOffset);
	}
}

Boolean SelectionHasLock(DocumentInfoHandle docInfo, long startOffset, long endOffset)
{
	long startParaIndex, paraIndex, startStyleRunIndex, styleRunIndex, tempOffset;
	ParagraphInfoHandle paraInfo;
	LongStyleRun tempStyleRun;
	LongSTElement tempStyle;
	
	startParaIndex = ParagraphIndex(docInfo, startOffset);
	tempOffset = startOffset - ParagraphOffset(docInfo, startParaIndex);
	paraInfo = (**docInfo).paraArray[startParaIndex].paraInfo;
	startStyleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	if(startOffset != endOffset) {
		paraIndex = ParagraphIndex(docInfo, endOffset - 1L);
		tempOffset = endOffset - ParagraphOffset(docInfo, paraIndex) - 1L;
		paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
		styleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	} else {
		paraIndex = startParaIndex;
		styleRunIndex = startStyleRunIndex;
	}
	
	do {
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		if(tempStyle.stInfo.textStyle.tsLock != peNoLock) {
			return true;
		}
		
		--styleRunIndex;
		if(styleRunIndex < 0L) {
			--paraIndex;
			if(paraIndex >= startParaIndex) {
				paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
				styleRunIndex = CountStyleRuns(paraInfo) - 1L;
			}
		}
	} while((paraIndex >= startParaIndex) && (styleRunIndex >= startStyleRunIndex));
	
	return false;	
}

OSErr GetParaInfo(DocumentInfoHandle docInfo, long index, PETEParaInfoPtr info)
{
	OSErr errCode;
	
	if(index < 0L) {
		if(index == kPETECurrentSelection) {
			index = (**docInfo).selStart.paraIndex;
			errCode = noErr;
		} else if(index == kPETEDefaultPara) {
			info->paraOffset = 0L;
			info->paraLength = 0L;
			info->startMargin = kDefaultMargin;
			info->endMargin = -kDefaultMargin;
			info->indent = 0L;
			info->direction = (short)(Byte)GetScriptVariable(smSystemScript, smScriptRight);
			info->justification = ((Byte)GetScriptVariable(smSystemScript, smScriptJust) == 0) ? teFlushLeft : teFlushRight;
			info->signedLevel = 0;
			info->quoteLevel = 0;
			info->paraFlags = 0;
			info->tabCount = 0;
			return noErr;
		} else {
			index = 0L;
			errCode = invalidIndexErr;
		}
	} else if(index >= (**docInfo).paraCount) {
		index = (**docInfo).paraCount - 1L;
		errCode = invalidIndexErr;
	} else {
		errCode = noErr;
	}
	info->paraOffset = ParagraphOffset(docInfo, index);
	info->paraLength = (**docInfo).paraArray[index].paraLength;
	info->startMargin = (**(**docInfo).paraArray[index].paraInfo).startMargin;
	info->endMargin = (**(**docInfo).paraArray[index].paraInfo).endMargin;
	info->indent = (**(**docInfo).paraArray[index].paraInfo).indent;
	info->direction = (**(**docInfo).paraArray[index].paraInfo).direction;
	info->justification = (**(**docInfo).paraArray[index].paraInfo).justification;
	info->signedLevel = (**(**docInfo).paraArray[index].paraInfo).signedLevel;
	info->quoteLevel = (**(**docInfo).paraArray[index].paraInfo).quoteLevel;
	info->paraFlags = (**(**docInfo).paraArray[index].paraInfo).paraFlags;
	info->tabCount = (**(**docInfo).paraArray[index].paraInfo).tabCount;
	if(info->tabHandle != nil) {
		SetHandleSize((Handle)info->tabHandle, ABS(info->tabCount) * sizeof(short));
		if((errCode = MemError()) == noErr) {
			BlockMoveData(&(**(**docInfo).paraArray[index].paraInfo).tabStops, *info->tabHandle, ABS(info->tabCount) * sizeof(short));
		}
	}
	return errCode;
}

OSErr GetParaIndex(DocumentInfoHandle docInfo, long offset, long *index)
{
	if(offset < 0L) {
		offset = (**docInfo).selStart.offset;
	} else if(offset > (**docInfo).textLen) {
		offset = (**docInfo).textLen;
	}
	
	*index = ParagraphIndex(docInfo, offset);
	if(*index < 0L) {
		return errOffsetInvalid;
	} else {
		return noErr;
	}
}

OSErr SetRecalcState(DocumentInfoHandle docInfo, Boolean recalc)
{
	OSErr errCode;
	SelData selection;
	
	if(docInfo == nil) {
		return paramErr;
	}
	errCode = noErr;
	if(recalc) {
		(**docInfo).flags.recalcOff = false;
		errCode = RecalcDocHeight(docInfo);
		if(errCode == noErr) {
			selection = (**docInfo).selStart;
			errCode = OffsetToPosition(docInfo, &selection, false);
		}
		if(errCode == noErr) {
			(**docInfo).selStart = selection;
			if(selection.offset != (**docInfo).selEnd.offset) {
				selection = (**docInfo).selEnd;
				errCode = OffsetToPosition(docInfo, &selection, false);
			}
		}
		if(errCode == noErr) {
			(**docInfo).selEnd = selection;
			(**docInfo).flags.scrollsDirty = true;
			(**docInfo).flags.drawInProgressLoop = false;
			if((**docInfo).flags.reposition) {
				RepositionDocument(docInfo);
			}
			InvalidateDocUpdate(docInfo);
		}
	}
	if((errCode != noErr) || !recalc) {
		(**docInfo).flags.recalcOff = true;
	}
	return errCode;
}

Boolean HasTEStyles(DocumentInfoHandle docInfo, long startOffset, long endOffset, Boolean plainText)
{
	long startParaIndex, startStyleRunIndex, styleRunIndex, paraIndex, tempOffset;
	ParagraphInfoHandle paraInfo;
	ScriptCode scriptCode;
	LongSTElement tempStyle;
	LongStyleRun tempStyleRun;
	
	if(startOffset == endOffset) {
		return false;
	}
	
	startParaIndex = ParagraphIndex(docInfo, startOffset);
	tempOffset = startOffset - ParagraphOffset(docInfo, startParaIndex);
	paraInfo = (**docInfo).paraArray[startParaIndex].paraInfo;
	startStyleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	paraIndex = ParagraphIndex(docInfo, endOffset - 1L);
	tempOffset = endOffset - ParagraphOffset(docInfo, paraIndex) - 1L;
	paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
	styleRunIndex = StyleRunIndex(paraInfo, &tempOffset);
	
	scriptCode = GetScriptManagerVariable(smSysScript);
	
	do {
		GetStyleRun(&tempStyleRun, paraInfo, styleRunIndex);
		GetStyle(&tempStyle, (**docInfo).theStyleTable, tempStyleRun.srStyleIndex);
		if((!plainText &&
		    ((tempStyle.stInfo.textStyle.tsFont != kPETEDefaultFont) ||
		     (tempStyle.stInfo.textStyle.tsFace != 0) ||
		     ((tempStyle.stInfo.textStyle.tsSize != kPETEDefaultSize) && (tempStyle.stInfo.textStyle.tsSize != kPETERelativeSizeBase)) ||
		     (!tempStyle.stIsGraphic && !IsPETEDefaultColor(tempStyle.stInfo.textStyle.tsColor)))) ||
		   (LangToScript(tempStyle.stInfo.textStyle.tsLang) != scriptCode)) {
			return true;
		}
		
		--styleRunIndex;
		if(styleRunIndex < 0L) {
			--paraIndex;
			if(paraIndex >= startParaIndex) {
				paraInfo = (**docInfo).paraArray[paraIndex].paraInfo;
				styleRunIndex = CountStyleRuns(paraInfo) - 1L;
			}
		}
	} while((paraIndex >= startParaIndex) && (styleRunIndex >= startStyleRunIndex));
	
	return false;
}