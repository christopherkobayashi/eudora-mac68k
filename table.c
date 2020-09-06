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

#include "table.h"
#define FILE_NUM 102
/* Copyright (c) 2000 by QUALCOMM Incorporated */

#pragma segment Table

static Boolean EditingTable(MyWindowPtr win);
static OSErr InsertTable(MyWindowPtr win,TIHandle table);
static void MakeTable(MyWindowPtr win);
static void TableDisplay(PETEHandle pte,TIHandle table,long offset);
static void SizeTable(PETEHandle pte,TIHandle tableInfo,short maxWidth);
static short **GetColWidths(PETEHandle pte,TIHandle tableInfo,short *sumWidths,short tableWidth,Boolean tableWidthSpecified);
static short **GetRowHeights(PETEHandle pte,TIHandle tableInfo,short *sumHeights,short **hColWidths);
static void SetCellBounds(PETEHandle pte,TableHandle table,short **hColWidths,short **hRowHeights);
static short GetCellWidth(PETEHandle pte,TIHandle tableInfo,short cellIdx);
static short GetCellHeight(PETEHandle pte,TIHandle tableInfo,short cellIdx,short colWidth);
static OSErr SetCellText(PETEHandle pte,TIHandle tableInfo,short cellIdx,short cellWidth);
static OSErr TableClone(TableHandle *table);
static void CloneHandle(Handle *pHand,OSErr *pErr);
RgnHandle SavePeteWinUpdateRgn(PETEHandle pte);
void RestorePeteWinUpdateRgn(PETEHandle pte,RgnHandle rgn);
static OSErr CreateSparePETE(MyWindowPtr win,PETEHandle *pte);
static OSErr ResetCellStyle(TIHandle tableInfo, short cellIdx);

/************************************************************************
 * MakeTableGraphic - make a table, insert as graphic
 ************************************************************************/
OSErr MakeTableGraphic(PETEHandle pte,long offset,TableHandle table,PETEStyleEntryPtr pse)
{
	TIHandle hTableInfo;
	OSErr err = noErr;
	PETEHandle	pteSpare;
	MyWindowPtr	win = (*PeteExtra(pte))->win;
	
	if (!(hTableInfo = NuHandleClear(sizeof(TableInfo))))
		return MemError();

	(*hTableInfo)->pgi.itemProc = TableGraphic;
	(*hTableInfo)->pgi.wantsEvents = true;
	(*hTableInfo)->table = table;
	(*hTableInfo)->offset = offset;

	// make a PETEHandle to measure, draw cells
	if (err = CreateSparePETE(win,&pteSpare)) return err;
	(*hTableInfo)->pteSpare = pteSpare;

	SizeTable(pte,hTableInfo,RectWi(win->contR));

	Zero(*pse);
	pse->psGraphic = 1;
	pse->psStyle.graphicStyle.tsFont = kPETEDefaultFont;
	pse->psStyle.graphicStyle.tsSize = kPETERelativeSizeBase;
	pse->psStyle.graphicStyle.graphicInfo = (PETEGraphicInfoHandle)hTableInfo;

	return(err);
}

/**********************************************************************
 * TableGraphic - editor callback to handle tables
 **********************************************************************/
pascal OSErr TableGraphic(PETEHandle pte,TIHandle table,long offset,PETEGraphicMessage message,void *data)
{
	OSErr err = noErr;
	short oldResFile = CurResFile();
	EventRecord	*event;
	MyWindowPtr	win;
	Boolean	keyEvent;
	Rect	r;
	short	maxWidth;
	
	PushGWorld();
	
	(*table)->offset = offset;
	switch (message)
	{
		case peGraphicDraw:	/* data is (Point *) with penLoc at baseline left as a point */
			TableDisplay(pte,table,offset);
			break;

		case peGraphicClone:	/* data is (PETEGraphicInfoHandle *) into which to put duplicate */
			//	When user does copy or drag and drop, make copy of table
			if (!(err = MyHandToHand(&table)))
			{
				TableHandle tempTable;
				
				// make clone
				tempTable = (*table)->table;
				err = TableClone(&tempTable);	
				if (err) ZapHandle(table);
				else
				{
					(*table)->table = tempTable;
					(*table)->pteSpare = nil;
				}
			}
			*(TIHandle*)data = table;
			break;
		
		case peGraphicInsert:		/* data is nil. Graphic has been inserted into a new document. */
			err = InsertTable((*PeteExtra(pte))->win,table);
			break;
			
		case peGraphicTest:	/* data is (Point *) from top left; return errOffsetIsOutsideOfView if not hit */
			//	Can cancel hit by returning errOffsetIsOutsideOfView
			break;
			
		case peGraphicHit:	/* data is (EventRecord *) if mouse down, nil if time to turn off */
			break;
			
		case peGraphicRemove:	/* data is nil; just dispose of table info */
			TableDispose((*table)->table);
			if((*table)->pteSpare)
				PeteDispose((*PeteExtra((*table)->pteSpare))->win,(*table)->pteSpare);
			ZapHandle(table);
			break;
			
		case peGraphicResize:	/* data is a (short *) of the max width */
			maxWidth = *(short*)data;
			//	don't bother calculating size if the maxWidth hasn't changed
			if (maxWidth != (*table)->maxWidth)
			{
				(*table)->maxWidth = maxWidth;
				SizeTable(pte,table,maxWidth);
			}
			break;
		
		case peGraphicRegion:	/* data is a RgnHandle which is to be changed to the graphic's region */
			//	Used mostly for changing the cursor
			PeteGraphicRect(&r,pte,(PETEGraphicInfoHandle)table,offset);
			RectRgn((RgnHandle)data,&r);
			break;
		
		case peGraphicEvent:	/* data is (EventRecord *) */
			event = (EventRecord *)data;
			win = (*PeteExtra(pte))->win;
			keyEvent = event->what==keyDown || event->what==autoKey;
			break;
	}
	
	PopGWorld();
	UseResFile(oldResFile);

	return(err);
}


/**********************************************************************
 * TableMenuChoice - handle a selection from the table submenu
 **********************************************************************/
void TableMenuChoice(MyWindowPtr win, short item)
{
	//Styled Text - don't allow user to compose or send styled text
	if (!HasFeature (featureExtendedStyles))
		return;
		
	switch (item)
	{
		case tblInsert:
			if (EditingTable(win))
			{
				//	Insert columns or rows
			}
			else
			{
				//	Insert or make table
				if (PeteLen(win->pte))
					MakeTable(win);
				else
					InsertTable(win,nil);
			}
			break;
			
		case tblDelete:
			break;
			
		case tblGridLine:
			break;
			
		case tblBorder:
			break;
			
		case tblRowHt:
			break;
			
		case tblColWd:
			break;
			
	}
}

/**********************************************************************
 * CheckTableItems - check items in Table submenu
 **********************************************************************/
void CheckTableItems(MyWindowPtr win,Boolean allMenu)
{
	Boolean	inTable;
	
	//Styled Text - don't allow user to compose or send styled text
	if (!HasFeature (featureExtendedStyles))
		return;

	inTable = EditingTable(win);
#if 0
	if (inTable)
	{
		MenuHandle mh = GetMHandle(TEXT_TABLE_HIER_MENU);
		TableHandle	hTable = win->table;
	
		SetItemMark(mh,tblGridLine,(*hTable)->gridline ? checkMark : noMark);
		SetItemMark(mh,tblBorder,(*hTable)->borders ? checkMark : noMark);
	}
#endif
}

/**********************************************************************
 * EnableTableMenu - enable/disable/rename items in Table submenu
 **********************************************************************/
void EnableTableMenu(MyWindowPtr win,Boolean all)
{
	Boolean	inTable;	
#if 0
	MenuHandle mh;
#endif
	
	//Styled Text - don't allow user to compose or send styled text
	if (!HasFeature (featureExtendedStyles))
		return;

	inTable = EditingTable(win);	
#if 0
	mh = GetMHandle(TEXT_TABLE_HIER_MENU);

	if (mh)
	{
		EnableIf(mh,tblInsert,GetWindowKind(GetMyWindowWindowPtr(win))==COMP_WIN);
		EnableIf(mh,tblDelete,inTable);
		EnableIf(mh,tblGridLine,inTable);
		EnableIf(mh,tblBorder,inTable);
		EnableIf(mh,tblRowHt,inTable);
		EnableIf(mh,tblColWd,inTable);
	}
#endif
}

/**********************************************************************
 * EditingTable - are we editing a table?
 **********************************************************************/
static Boolean EditingTable(MyWindowPtr win)
{
	return win && IsMyWindow(GetMyWindowWindowPtr(win)) && win->table && (*win->table)->pteEdit && (*win->table)->pteEdit == win->pte; 
}

/**********************************************************************
 * MakeTable - make a table from existing text
 **********************************************************************/
static void MakeTable(MyWindowPtr win)
{

}

/**********************************************************************
 * InsertTable - insert a new table
 **********************************************************************/
static OSErr InsertTable(MyWindowPtr win,TIHandle table)
{
	PETEHandle pteSpare;
	OSErr err = noErr;
	
	if (!(*table)->pteSpare && !(err = CreateSparePETE(win,&pteSpare)))
		(*table)->pteSpare = pteSpare;
	return err;
}

/**********************************************************************
 * InsertTable - insert a new table
 **********************************************************************/
static void TableDisplay(PETEHandle pte,TIHandle tableInfo,long offset)
{
	Rect	rTable;
	TableHandle	table = (*tableInfo)->table;
	TabCellHandle	hCells = (*table)->cellData;
	PETEHandle	pteSpare = (*tableInfo)->pteSpare;
	short	border,cellIdx;
	RgnHandle	clipRgn,saveUpdate = SavePeteWinUpdateRgn(pte);
	
	PeteGraphicRect(&rTable,pte,(PETEGraphicInfoHandle)tableInfo,offset);
	if (border = (*table)->border)
	{
		// draw border
		SetForeGrey(0x8000);
		PenSize(border,border);
		FrameRect(&rTable);
		PenNormal();
		SetForeGrey(0);
	}
	
	// draw each cell if in clip region
	if(clipRgn=NewRgn()) GetClip(clipRgn);
	for(cellIdx=0;cellIdx<(*table)->cells;cellIdx++)
	{
		Rect	rCell = (*hCells)[cellIdx].bounds;
		
		OffsetRect(&rCell,rTable.left,rTable.top);
		if (RectInRgn(&rCell,clipRgn))
		{
			short	align,top,left;
			PETEDocInfo info;
			
			ClipRect(&rCell);
			if (!SetCellText(pte,tableInfo,cellIdx,RectWi(rCell)))
			{
				top = rCell.top;
				left = rCell.left;
				if ((align = (*hCells)[cellIdx].align.vAlign) != htmlTopAlign)
				if (!PETEGetDocInfo(PETE,pteSpare,&info))
				{
					//	vertical alignment
					switch (align)
					{
						case htmlBottomAlign:	top = rCell.bottom-info.docHeight; break;
						case htmlMiddleAlign:	top = (rCell.top+rCell.bottom-info.docHeight)/2; break;
					}
				}
				PETESizeDoc(PETE,pteSpare,RectWi(rCell),RectHi(rCell));
				PETEMoveDoc(PETE,pteSpare,left,top);
				PETEDraw(PETE,pteSpare);
				ResetCellStyle(tableInfo, cellIdx);
			}
			
			if (border)
			{
				short	inset = -(*table)->cellPadding-1;
				
				SetForeGrey(0x8000);
				InsetRect(&rCell,inset,inset);
				ClipRect(&rCell);
				FrameRect(&rCell);
				SetForeGrey(0);
			}
		}
	}
	if(clipRgn) SetClip(clipRgn);

	RestorePeteWinUpdateRgn(pte,saveUpdate);
}

/**********************************************************************
 * TableColIndex - column to index
 **********************************************************************/
short TableColIndex(TabColHandle columnData, short c)
{
	if(columnData)
	{
		short i;
		
		i = GetHandleSize(columnData) / sizeof(TabColData);
		while(--i >= 0)
		{
			if(((*columnData)[i].column >= c) && ((*columnData)[i].column + (*columnData)[i].span < c))
				return i;
		}
	}
	return -1;
}

/**********************************************************************
 * TableColGroupIndex - column group to index
 **********************************************************************/
short TableColGroupIndex(TabColHandle columnGroupData, short c)
{
	if(columnGroupData)
	{
		short i;
		
		i = GetHandleSize(columnGroupData) / sizeof(TabColData);
		while(--i >= 0)
		{
			if((((*columnGroupData)[i].span == 0) && ((*columnGroupData)[i].column < c)) ||
			   (((*columnGroupData)[i].column >= c) && ((*columnGroupData)[i].column + (*columnGroupData)[i].span < c)))
				return i;
		}
	}
	return -1;
}

/**********************************************************************
 * TableRowIndex - table row to index
 **********************************************************************/
short TableRowIndex(TabRowHandle rowData, short r)
{
	if(rowData)
	{
		short i;
		
		i = GetHandleSize(rowData) / sizeof(TabRowData);
		while(--i >= 0)
		{
			if((*rowData)[i].row == r)
				return i;
		}
	}
	return -1;
}

/**********************************************************************
 * TableRowGroupIndex - table group to index
 **********************************************************************/
short TableRowGroupIndex(TabRowGroupHandle rowGroupData, short r)
{
	if(rowGroupData)
	{
		short i;
		
		i = GetHandleSize(rowGroupData) / sizeof(TabRowGroupData);
		while(--i >= 0)
		{
			if((*rowGroupData)[i].row < r)
				return i;
		}
	}
	return -1;
}

/**********************************************************************
 * GetCellHAlign - get horizontal alignment for cell
 **********************************************************************/
void GetCellHAlign(TableHandle table, short cell, short *hAlign, UniChar *alignChar, short *charOff)
{
	TabAlignData align;
	short r, c, i;
	
	r = (*(**table).cellData)[cell].row;
	c = (*(**table).cellData)[cell].column;
	align = (*(**table).cellData)[cell].align;
	if(!align.hAlign)
	{
		i = TableColIndex((**table).columnData, c);
		if(i >= 0)
		{
			align = (*(**table).columnData)[i].align;
		}
		if(!align.hAlign)
		{
			i = TableColIndex((**table).columnGroupData, c);
			if(i >= 0)
			{
				align = (*(**table).columnGroupData)[i].align;
			}
			if(!align.hAlign)
			{
				i = TableRowIndex((**table).rowData, r);
				if(i >= 0)
				{
					align = (*(**table).rowData)[i].align;
				}
				if(!align.hAlign)
				{
					i = TableRowGroupIndex((**table).rowGroupData, r);
					if(i >= 0)
					{
						align = (*(**table).rowGroupData)[i].align;
					}
					if(!align.hAlign)
					{
						align.hAlign = (**table).align;
					}
				}
			}
		}
	}
	if(hAlign)
		*hAlign = align.hAlign;
	if(alignChar)
		*alignChar = align.alignChar;
	if(charOff)
		*charOff = align.charOff;
}

/**********************************************************************
 * GetCellVAlign - get vertical alignment for cell
 **********************************************************************/
void GetCellVAlign(TableHandle table, short cell, short *vAlign)
{
	short r, c, i, v;
	
	r = (*(**table).cellData)[cell].row;
	c = (*(**table).cellData)[cell].column;
	v = (*(**table).cellData)[cell].align.vAlign;
	if(!v)
	{
		i = TableRowIndex((**table).rowData, r);
		if(i >= 0)
		{
			v = (*(**table).rowData)[i].align.vAlign;
		}
		if(!v)
		{
			i = TableRowGroupIndex((**table).rowGroupData, r);
			if(i >= 0)
			{
				v = (*(**table).rowGroupData)[i].align.vAlign;
			}
			if(!v)
			{
				i = TableColIndex((**table).columnData, c);
				if(i >= 0)
				{
					v = (*(**table).columnData)[i].align.vAlign;
				}
				if(!v)
				{
					i = TableColIndex((**table).columnGroupData, c);
					if(i >= 0)
					{
						v = (*(**table).columnGroupData)[i].align.vAlign;
					}
				}
			}
		}
	}
	if(vAlign)
		*vAlign = v;
}

/**********************************************************************
 * GetCellDirection - get cell direction
 **********************************************************************/
void GetCellDirection(TableHandle table, short cell, short *direction)
{
	short r, c, i, d;
	
	r = (*(**table).cellData)[cell].row;
	c = (*(**table).cellData)[cell].column;
	d = (*(**table).cellData)[cell].align.direction;
	if(d == kHilite)
	{
		i = TableColIndex((**table).columnData, c);
		if(i >= 0)
		{
			d = (*(**table).columnData)[i].align.direction;
		}
		if(d == kHilite)
		{
			i = TableColIndex((**table).columnGroupData, c);
			if(i >= 0)
			{
				d = (*(**table).columnGroupData)[i].align.direction;
			}
			if(d == kHilite)
			{
				i = TableRowIndex((**table).rowData, r);
				if(i >= 0)
				{
					d = (*(**table).rowData)[i].align.direction;
				}
				if(d == kHilite)
				{
					i = TableRowGroupIndex((**table).rowGroupData, r);
					if(i >= 0)
					{
						d = (*(**table).rowGroupData)[i].align.direction;
					}
					if(d == kHilite)
						d = (**table).direction;
				}
			}
		}
	}
	if(direction)
		*direction = d;
}

/**********************************************************************
 * TableDispose - dispose of table data
 **********************************************************************/
void TableDispose(TableHandle table)
{
	TabCellHandle	hCells = (*table)->cellData;
	short		cellIdx;
	
	if(hCells)
		for(cellIdx=0;cellIdx<(*table)->cells;cellIdx++)
		{
			DisposeHandle((*hCells)[cellIdx].abbr);
			PETEDisposeStyleScrap(PETE, (*hCells)[cellIdx].styleInfo);
			PETEDisposeParaScrap(PETE, (*hCells)[cellIdx].paraInfo);
		}
	DisposeHandle((*table)->rowGroupData);
	DisposeHandle((*table)->rowData);
	DisposeHandle((*table)->columnData);
	DisposeHandle((*table)->columnGroupData);
	DisposeHandle((*table)->cellData);
	DisposeHandle(table);
}

/**********************************************************************
 * TableClone - make a copy of table data
 **********************************************************************/
static OSErr TableClone(TableHandle *table)
{
	TableData tableData;
	OSErr	err;
	long cellIdx;
	
	if((err = MyHandToHand(table)) != noErr)
		return err;
	
	tableData = ***table;
	(**table)->rowGroupData = nil;
	(**table)->rowData = nil;
	(**table)->columnData = nil;
	(**table)->columnGroupData = nil;
	(**table)->cellData = nil;
	if(tableData.cellData && !(err = MyHandToHand(&tableData.cellData)))
	{
		(**table)->cellData = tableData.cellData;
		for(cellIdx=0;cellIdx<(**table)->cells;cellIdx++)
		{
			Handle	abbr;
			PETEParaScrapHandle paraInfo= nil;
			PETEStyleListHandle styleInfo = nil;
			
			abbr = (*tableData.cellData)[cellIdx].abbr;
			if(!abbr || !(err = MyHandToHand(&abbr)))
			{
				paraInfo = (*tableData.cellData)[cellIdx].paraInfo;
				if(!paraInfo || !(err = MyHandToHand(&paraInfo)))
				{
					styleInfo = (*tableData.cellData)[cellIdx].styleInfo;
					if(styleInfo && (err = PETEDuplicateStyleScrap(PETE, &styleInfo)) != noErr)
						styleInfo = nil;
				}
				else
					paraInfo = nil;
			}
			else
				abbr = nil;

			(*tableData.cellData)[cellIdx].abbr = abbr;
			(*tableData.cellData)[cellIdx].paraInfo = paraInfo;
			(*tableData.cellData)[cellIdx].styleInfo = styleInfo;
			
			if(err)
			{
				while(cellIdx >= 0)
				{
					DisposeHandle((*tableData.cellData)[cellIdx].abbr);
					PETEDisposeStyleScrap(PETE, (*tableData.cellData)[cellIdx].styleInfo);
					PETEDisposeParaScrap(PETE, (*tableData.cellData)[cellIdx].paraInfo);
					--cellIdx;
				}
				break;
			}
		}
	}
	
	if(!err)
	{
		if(!tableData.rowGroupData || !(err = MyHandToHand(&tableData.rowGroupData)))
		{
			(**table)->rowGroupData = tableData.rowGroupData;
			if(!tableData.rowData || !(err = MyHandToHand(&tableData.rowData)))
			{
				(**table)->rowData = tableData.rowData;
				if(!tableData.columnData || !(err = MyHandToHand(&tableData.columnData)))
				{
					(**table)->columnData = tableData.columnData;
					if(!tableData.columnGroupData || !(err = MyHandToHand(&tableData.columnGroupData)))
					{
						(**table)->columnGroupData = tableData.columnGroupData;
					}
				}
			}
		}
	}
	
	if(err)
		TableDispose(*table);
	
	return err;
}

/**********************************************************************
 * DuplicateHandle - duplicate a handle
 **********************************************************************/
static void CloneHandle(Handle *pHand,OSErr *pErr)
{
	if (!*pHand || *pErr) return;
	
	*pErr = HandToHand(pHand);
}

/**********************************************************************
 * GetColWidths - get column widths
 **********************************************************************/
static short **GetColWidths(PETEHandle pte,TIHandle tableInfo,short *sumWidths,short tableWidth,Boolean tableWidthSpecified)
{
	short	**hWidths,colCount;
	TabColHandle	hColData;
	short		propCount = 0,cellIdx;
	Str255		colStat;
	enum	{ kNoWidth,kFixedWidth,kPropWidth };
	short	column;
	TableHandle	table = (*tableInfo)->table;
	short	width,span,i;
	short	leftoverWidth,noSpecCount;
	
	colCount = (*table)->columns;
	if (!(hWidths = NuHandleClear(sizeof(short)*colCount))) return nil;
	
	Zero(colStat);
	if ((hColData = (*table)->columnData) || (hColData = (*table)->columnGroupData))
	{
		short	colDataCount,width;
		TabColPtr	pColData;
		
		colDataCount = HandleCount(hColData);
		for(colDataCount=HandleCount(hColData),pColData=*hColData;colDataCount--;)
		{
			span = pColData->span ? pColData->span : 1;
			if ((width = pColData->width) < 0)
				width = tableWidth*(-width)/100;	// width is %
			if (pColData->column + span <= colCount)
			{
				for (column = pColData->column;span--;column++)
				{
					(*hWidths)[column] = width;
					if (pColData->propWidth)
					{
						propCount += width;
						colStat[column] = kPropWidth;
					}
					else
						colStat[column] = kFixedWidth;
				}
			}
		}
		
		//	do we have any columns that we still don't know the width of?
		for(column=0;column<colCount;column++)
			if (!colStat[column])
			{
				hColData = 0;	// signal we need to do some column calculations
				break;
			}
	}
	
	if (!hColData)
	{
		//	we need to calculate column sizes based on cell contents
		TabCellHandle	hCells = (*table)->cellData;
		short			sumWidth,overCount,aveWidth;
		Boolean			doSpan=false;
		
		for(cellIdx=0;cellIdx<(*table)->cells;cellIdx++)
		{
			column = (*hCells)[cellIdx].column;
			if (!colStat[column])
			{
				if ((*hCells)[cellIdx].colSpan<2)
				{
					// need to calculate this column
					width = GetCellWidth(pte,tableInfo,cellIdx);
					if (width > (*hWidths)[column]) (*hWidths)[column] = width;
				}
				else
					doSpan=true;
			}
		}
		
		if (doSpan)
		{
			// there are column spans we need to check
			for(cellIdx=0;cellIdx<(*table)->cells;cellIdx++)
			{
				span = (*hCells)[cellIdx].colSpan;
				column = (*hCells)[cellIdx].column;
				if (!colStat[column] && span>1)
				{
					// calculate this span
					short	sum=0;
					
					width = GetCellWidth(pte,tableInfo,cellIdx);
					// get sum of widths in span
					for(i=column;i<column+span;i++)
						sum += (*hWidths)[i];
					sum += (span-1)*((*table)->cellSpacing+2*((*table)->cellPadding+(*table)->border?1:0));						
					if (width > sum)
					{
						// cell is too wide for span
						// divide excess up equally between each column
						short	addThis = (width-sum+span-1)/span;
						for(i=column;i<column+span;i++)
							(*hWidths)[i] += addThis;
					}
				}
			}		
		}
		
		// make sure none of the columns are too wide
		sumWidth = 0;
		leftoverWidth = 0;
		overCount = 0;
		aveWidth = tableWidth/colCount;
		for(column=0;column<colCount;column++)
		{
			if (colStat[column] != kPropWidth)
			{
				short	thisWidth = (*hWidths)[column];
				
				sumWidth += thisWidth;
				if (thisWidth <= aveWidth) leftoverWidth += thisWidth;
				else overCount++;
			}
		}
		
		if (sumWidth > tableWidth)
		{
			//	too wide, reduce width of wide ones
			short	bigWidth = (tableWidth+leftoverWidth)/colCount;
			for(column=0;column<colCount;column++)
				if (colStat[column] != kPropWidth && (*hWidths)[column]>aveWidth)
					(*hWidths)[column] = bigWidth;
		
		}
	}
	
	if (propCount)
	{
		// calculate size of proportional columns
		leftoverWidth = tableWidth;
		
		// how much table width do we have left over?
		for(column=0;column<colCount;column++)
		{
			if (colStat[column] != kPropWidth)
				leftoverWidth -= (*hWidths)[column];
		}
		
		if (leftoverWidth < 0) leftoverWidth = 0;	// this should never happen

		for(column=0;column<colCount;column++)
		{
			if (colStat[column] == kPropWidth)
				(*hWidths)[column] = (*hWidths)[column]*leftoverWidth/propCount;
		}
	}
	
	if (tableWidthSpecified)
	{
		//	Any extra remaining width should be divided up among
		//	cells with no specified width
		leftoverWidth = tableWidth;
		noSpecCount = 0;
		for(column=0;column<colCount;column++)
		{
			leftoverWidth -= (*hWidths)[column];
			if (!colStat[column])
				noSpecCount++;
		}
			
		if (leftoverWidth)
		{
			short	lastNoSpec=0;
			for(column=0;column<colCount;column++)
				if (!colStat[column])
				{
					(*hWidths)[column] += leftoverWidth/noSpecCount;
					lastNoSpec = column;
				}
			(*hWidths)[lastNoSpec] += leftoverWidth%noSpecCount;	//	Fill up to very end
		}
	}
	

	// calculate sum of widths
	*sumWidths = 0;
	for(column=0;column<colCount;column++)
		*sumWidths += (*hWidths)[column];
	
	return hWidths;
}

/**********************************************************************
 * SetCellText - put cell text in edit pte
 **********************************************************************/
static OSErr SetCellText(PETEHandle pte,TIHandle tableInfo,short cellIdx,short cellWidth)
{
	TableHandle	table = (*tableInfo)->table;
	PETEHandle	pteSpare = (*tableInfo)->pteSpare;
	UHandle		text = nil;
	TabCellHandle	hCells = (*table)->cellData;
	OSErr		err;
	Boolean		saveDirty;
	MyWindowPtr	win;
	
	err = PETESetRecalcState(PETE,pteSpare,false);
	if (err) return err;
	
	win = (*PeteExtra(pteSpare))->win;
	saveDirty = win->isDirty;
	err = PeteDelete(pteSpare,0,0x7fffffff);
	if (!err)
	{
		err = PETEGetText(PETE, pte, (*hCells)[cellIdx].textOffset + (*tableInfo)->offset, (*hCells)[cellIdx].textOffset + (*tableInfo)->offset + (*hCells)[cellIdx].textLength, &text);
		if(!err)
		{
			err = PETEChangeDocWidth(PETE,pteSpare,cellWidth,true);
			if(!err)
			{
				err = PETEInsertTextScrap(PETE,pteSpare,0,text,(*hCells)[cellIdx].styleInfo,(*hCells)[cellIdx].paraInfo,false);
				if(!err)
					ZapHandle((*hCells)[cellIdx].styleInfo);
			}
			ZapHandle(text);
		}
	}
	win->isDirty = saveDirty;
	PETESetRecalcState(PETE,pteSpare,true);
	return err;
}

/**********************************************************************
 * GetCellWidth - get width of cell contents
 **********************************************************************/
static short GetCellWidth(PETEHandle pte,TIHandle tableInfo,short cellIdx)
{
	TableHandle	table = (*tableInfo)->table;
	TabCellHandle	hCells = (*table)->cellData;
	Point startPt,endPt;
	LHElement lineHeight;
	short	width;
	
	width = 0;
	if ((*hCells)[cellIdx].width)
		width = (*hCells)[cellIdx].width;
	else if (!SetCellText(pte,tableInfo,cellIdx,REAL_BIG)) 
	{
		if (!PETEOffsetToPosition(PETE,(*tableInfo)->pteSpare,0,&startPt,&lineHeight))
		if (!PETEOffsetToPosition(PETE,(*tableInfo)->pteSpare,(*hCells)[cellIdx].textLength,&endPt,&lineHeight))
			width = endPt.h - startPt.h + 9;
		ResetCellStyle(tableInfo, cellIdx);
	}
	return width;
}

/**********************************************************************
 * GetRowHeights - get row heights
 **********************************************************************/
static short **GetRowHeights(PETEHandle pte,TIHandle tableInfo,short *sumHeights,short **hColWidths)
{
	short	**hHeights,rowCount;
	short		cellIdx, row, height, column, span, cellWidth, i;
	TableHandle	table = (*tableInfo)->table;
	TabCellHandle	hCells = (*table)->cellData;
	
	rowCount = (*table)->rows;
	if (!(hHeights = NuHandleClear(sizeof(short)*rowCount))) return nil;
	
	for(cellIdx=0;cellIdx<(*table)->cells;cellIdx++)
	{
		row = (*hCells)[cellIdx].row;
		span = (*hCells)[cellIdx].colSpan;
		column = (*hCells)[cellIdx].column;
		cellWidth = (*hColWidths)[column];
		if (span>1)
		{
			for(i=column+1;i<column+span;i++)
				cellWidth += (*hColWidths)[i];
			cellWidth += (span-1)*((*table)->cellSpacing+2*((*table)->cellPadding+(*table)->border?1:0));		
		}
		height = GetCellHeight(pte,tableInfo,cellIdx,cellWidth);
		if (height > (*hHeights)[row]) (*hHeights)[row] = height;
	}
	
	// calculate sum of heights
	*sumHeights = 0;
	for(row=0;row<rowCount;row++)
		*sumHeights += (*hHeights)[row];
	
	return hHeights;
}

/**********************************************************************
 * GetCellHeight - get height of cell contents
 **********************************************************************/
static short GetCellHeight(PETEHandle pte,TIHandle tableInfo,short cellIdx,short colWidth)
{
	TableHandle	table = (*tableInfo)->table;
	PETEHandle	pteEdit = (*table)->pteEdit;
	TabCellHandle	hCells = (*table)->cellData;
	short	height;
	
	height = 0;
	if ((*hCells)[cellIdx].height)
		height = (*hCells)[cellIdx].height;
	else if (!SetCellText(pte,tableInfo,cellIdx,colWidth)) 
	{
		PETEDocInfo info;

		if (!PETEGetDocInfo(PETE,(*tableInfo)->pteSpare,&info))
			height = info.docHeight;
		ResetCellStyle(tableInfo, cellIdx);
	}
	return height;
}

/**********************************************************************
 * SetCellBounds - set the size and position of each cell
 **********************************************************************/
static void SetCellBounds(PETEHandle pte,TableHandle table,short **hColWidths,short **hRowHeights)
{
	TabCellHandle	hCells = (*table)->cellData;
	short		lastRow=0,colIdx,cellCount;
	Rect		r;
	TabCellPtr	pCell;
	short	border = (*table)->border;
	short	cellPadding = (*table)->cellPadding;
	short	cellSpacing = (*table)->cellSpacing;
	short	cellBorder = border?1:0;
	short	row,column,span;
	
	if (border && !(*table)->cellSpacingSpecified)
		cellSpacing = 1;	// if we're drawing cell borders, make sure there is at least some cell spacing
	r.top = border+cellSpacing+cellBorder+cellPadding;
	for (cellCount=(*table)->cells,pCell=*hCells;cellCount--;pCell++)
	{		
		while (pCell->row>lastRow)
			// new row, might have skipped some with row span
			r.top += (*hRowHeights)[lastRow++]+cellSpacing+2*(cellPadding+cellBorder);
		
		r.left = border+cellSpacing+cellBorder+cellPadding;
		for(colIdx=0;colIdx<pCell->column;colIdx++)
			r.left += (*hColWidths)[colIdx]+cellSpacing+2*(cellPadding+cellBorder);
		
		row = pCell->row;
		column = pCell->column;
		r.bottom = r.top + (*hRowHeights)[row++];
		r.right = r.left + (*hColWidths)[column++];

		for (span = pCell->colSpan?pCell->colSpan-1:0;span-- && column < (*table)->columns;)
		{
			// column span
			r.right += (*hColWidths)[column++] + cellSpacing+2*(cellPadding+cellBorder);
		}
		
		for (span = pCell->rowSpan?pCell->rowSpan-1:0;span-- && row < (*table)->rows;)
		{
			// row span
			r.bottom += (*hRowHeights)[row++] + cellSpacing+2*(cellPadding+cellBorder);
		}
		
		pCell->bounds = r;
	}
}

/**********************************************************************
 * SizeTable - set the size of the table
 **********************************************************************/
static void SizeTable(PETEHandle pte,TIHandle tableInfo,short maxWidth)
{
	short	width, height, colWidths;
	short	**hColWidths=nil,**hRowHeights=nil;
	short	widthPadding,heightPadding,widthAvail,columns,rows;
	TableHandle	table = (*tableInfo)->table;
	short	cellSpacing;
	RgnHandle	saveClip = SavePeteWinUpdateRgn(pte);
	
	height = 0;
	width = (*table)->width;
	if (width < 0) width =  maxWidth*(-width)/100;	// width is %
	
	//	determine extra padding
	columns = (*table)->columns;
	rows = (*table)->rows;
	cellSpacing = (*table)->cellSpacing;
	if ((*table)->border && !(*table)->cellSpacingSpecified)
		cellSpacing = 1;	// if we're drawing cell borders, make sure there is at least some cell spacing
	widthPadding = 2*(*table)->border + 2*columns*(*table)->cellPadding
		+ (columns+1)*cellSpacing;
	heightPadding = 2*(*table)->border + 2*rows*(*table)->cellPadding
		+ (rows+1)*cellSpacing;
	
	widthAvail = width?width:maxWidth;
	if (widthPadding >= widthAvail) widthPadding = 0;
	else widthAvail -= widthPadding;
	if (hColWidths = GetColWidths(pte,tableInfo,&colWidths,widthAvail,width!=0))
	if (hRowHeights = GetRowHeights(pte,tableInfo,&height,hColWidths))
	{
		if (!width)
			width = colWidths;
		SetCellBounds(pte,table,hColWidths,hRowHeights);
	}
	
	ZapHandle(hColWidths);
	ZapHandle(hRowHeights);

	//	if we're measuring the width of a cell that contains a table
	//	this may be really large. Reduce it down a bit to avoid short int overflow
	width = MIN(width,16000);
	width += widthPadding;
	(*tableInfo)->pgi.width = MIN(width,maxWidth);
	(*tableInfo)->pgi.height = height + heightPadding;
	(*tableInfo)->pgi.descent = 3;

	RestorePeteWinUpdateRgn(pte,saveClip);
}

/**********************************************************************
 * SavePeteWinUpdateRgn - save the updatergn of the window containing a pete handle
 **********************************************************************/
RgnHandle SavePeteWinUpdateRgn(PETEHandle pte)
{
	RgnHandle	rgn;
	
	if (rgn = NewRgn())
		GetWindowUpdateRgn(GetMyWindowWindowPtr((*PeteExtra(pte))->win),rgn);
	return rgn;	
}

/**********************************************************************
 * RestorePeteWinUpdateRgn - restore the updatergn of the window containing a pete handle
 **********************************************************************/
void RestorePeteWinUpdateRgn(PETEHandle pte,RgnHandle rgn)
{
	if (rgn)
	{
		LocalizeRgn(rgn);
		InvalWindowRgn(GetMyWindowWindowPtr((*PeteExtra(pte))->win),rgn);
		DisposeRgn(rgn);
	}
}

/**********************************************************************
 * CreateSparePTE - create a spare PETE handle
 **********************************************************************/
static OSErr CreateSparePETE(MyWindowPtr win,PETEHandle *pte)
{
	PETEDocInitInfo pdi;
	OSErr	err;

	DefaultPII(win,false,peEmbedded,&pdi);
	pdi.inParaInfo.paraFlags = 0;
	pdi.inParaInfo.startMargin = 0;
	pdi.inParaInfo.endMargin = WindowWi(GetMyWindowWindowPtr(win))+1;
	err = PeteCreate(win,pte,peEmbedded,&pdi);
	CleanPII(&pdi);
	if (err) return err;

	// remove from win's pteList, we don't want anyone to know about this pte
	win->pteList = PeteRemove(win->pteList,*pte);

	// get rid of progress callback function
	// it can cause re-entrancy problems
	PETESetCallback(PETE,*pte,nil,peProgressLoop);
	return noErr;
}

static OSErr ResetCellStyle(TIHandle tableInfo, short cellIdx)
{
	PETEStyleListHandle styleInfo;
	TabCellHandle hCells = (*(*tableInfo)->table)->cellData;
	OSErr err;

	if(!(err = PeteGetTextStyleScrap((*tableInfo)->pteSpare, 0, LONG_MAX, nil, &styleInfo, nil)))
	{
		(*hCells)[cellIdx].styleInfo = styleInfo;
	}
	return err;
}
