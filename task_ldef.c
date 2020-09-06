/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define FILE_NUM 109
/* Copyright (c) 1997 by QUALCOMM Incorporated */

#include "task_ldef.h"

#pragma segment ListItemLDEF


typedef struct taskCellData_ taskCellData, *taskCellDataPtr,
    **taskCellDataHandle;
struct taskCellData_ {
	drawCellType draw;
	Handle data;
};

OSErr AddListItemEntry(short where, drawCellType draw, Handle data,
		       ListHandle lHandle)
{
	taskCellData cellData;
	Point cell;
	Size origSize;
	short origCount;
	OSErr err = noErr;

	if (!lHandle)
		return -1;
	PushGWorld();
	SetPort((*lHandle)->port);
	cellData.draw = draw;
	cellData.data = data;
	cell.h = 0;
	// don't draw on screen because we could be wazooed, but update list
	SetOrigin(-REAL_BIG, -REAL_BIG);
	origCount = (*lHandle)->dataBounds.bottom;
	cell.v = LAddRow(1, where, lHandle);
	if (origCount >= (*lHandle)->dataBounds.bottom)
		err = memFullErr;
	if (!err) {
		origSize = GetHandleSize((*lHandle)->cells);
		LSetCell(&cellData, sizeof(cellData), cell, lHandle);
		if (origSize == GetHandleSize((*lHandle)->cells))
			err = memFullErr;
	}
	SetOrigin(0, 0);
	PopGWorld();
	return err;
}

void RemoveListItemEntry(Handle data, ListHandle lHandle)
{
	Cell searchCell;
	taskCellData cellData;
	short dataLen;

	if (!lHandle)
		return;
	PushGWorld();
	SetPort((*lHandle)->port);
	searchCell.v = searchCell.h = 0;
	while (1) {
		cellData.draw = nil;
		cellData.data = nil;
		dataLen = sizeof(cellData);
		LGetCell(&cellData, &dataLen, searchCell, lHandle);
		ASSERT(dataLen == sizeof(cellData));
		if (cellData.data == data) {
			// don't draw on screen because we could be wazooed, but update list
			SetOrigin(-REAL_BIG, -REAL_BIG);
			LDelRow(1, searchCell.v, lHandle);
			SetOrigin(0, 0);
			break;
		}
		if (!LNextCell(false, true, &searchCell, lHandle))
			break;
	}
	PopGWorld();
}

pascal void ListItemDef(short lMessage, Boolean lSelect, Rect * lRect,
			Cell lCell, short lDataOffset, short lDataLen,
			ListHandle lHandle)
{
#pragma unused(lDataOffset,lDataLen)
	Rect fullRect;
	if (lMessage == lDrawMsg || lMessage == lHiliteMsg) {
		fullRect = *lRect;
		fullRect.bottom = fullRect.top + (*lHandle)->cellSize.v;
	}
	switch (lMessage) {
	case lDrawMsg:
		ListItemDraw(lSelect, &fullRect, lCell, lHandle);
		break;
//              case lHiliteMsg:
//                      MyListHilite(lSelect,&fullRect,lCell,lHandle);
//                      break;
	}
}

void ListItemDraw(Boolean lSelect, Rect * lRect, Cell lCell,
		  ListHandle lHandle)
{
	MyWindowPtr win = GetPortMyWindowPtr(GetQDGlobalsThePort());
	Rect oldRect = *lRect, dividerRect, clipRect;
	short dataLen;
	taskCellData cellData;

	PushGWorld();
	SetPort((*lHandle)->port);

	{
		SAVE_STUFF;

		SET_COLORS;
		/*
		 * erase the rectangle first
		 */
		SectRect(lRect, &(*lHandle)->rView, &clipRect);
		ClipRect(&clipRect);

		/*
		 * setup font and size
		 */
		SetSmallSysFont();

		/*
		 * draw
		 */
		cellData.draw = nil;
		cellData.data = nil;
		dataLen = sizeof(cellData);
		LGetCell(&cellData, &dataLen, lCell, lHandle);
		if (dataLen == sizeof(cellData)) {
			if (cellData.draw) {
				Rect indentedLRect;
				indentedLRect = *lRect;
				InsetRect(&indentedLRect,
					  (*lHandle)->indent.h,
					  (*lHandle)->indent.v);
				(*cellData.draw) (win, &indentedLRect,
						  cellData.data);
			}
		}

		SetRect(&dividerRect, (*lHandle)->rView.left,
			lRect->bottom - 2, (*lHandle)->rView.right + 1,
			lRect->bottom);
		SectRect(&dividerRect, &(*lHandle)->rView, &clipRect);
		ClipRect(&clipRect);
		DrawDivider(&dividerRect, True);
		//      if (lSelect)    HiInvertRect(&oldRect);
		/*
		 * restore font & size & clip
		 */
		REST_STUFF;
	}
	InfiniteClip(GetQDGlobalsThePort());
	PopGWorld();
}
