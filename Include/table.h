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

#ifndef TEXTTABLE_H
#define TEXTTABLE_H

/* Row groups. Positive numbers are body row groups in order of appearance. */
#define	tableHeadRowGroup	-2
#define	tableFootRowGroup	-1
#define	tableNoRowGroup		0

/* Copyright (c) 2000 by Qualcomm, Inc. */

typedef struct {
	short hAlign;		// horizontal alignment
	UniChar alignChar;	// for character alignment
	short charOff;		// character alignment offset
	short vAlign;		// vertical alignment
	short direction;	// text direction
} TabAlignData;

//      Row group info
typedef struct {
	TabAlignData align;
	short rowGroup;
	short row;
} TabRowGroupData, *TabRowGroupPtr, **TabRowGroupHandle;

//      Row info
typedef struct {
	TabAlignData align;
	short row;		// 0-based row #
} TabRowData, *TabRowPtr, **TabRowHandle;

//      Column info, only used if COLGROUP or COL element is specified
typedef struct {
	short column;		// 0-based column #
	short span;
	short width;		// width in pixels or proportion if > 0, percent if < 0
	Boolean propWidth;	// Above width is proportional
	TabAlignData align;
} TabColData, *TabColPtr, **TabColHandle;

//      Cell info
typedef struct {
	// data parsed from HTML
	short rowSpan;
	short colSpan;
	short width;		// width in pixels
	short height;		// height in pixels
	TabAlignData align;
	Handle abbr;		// Abbreviated text
	Boolean header;		// table header?

	// misc data
	short row;		// 0-based row #
	short column;		// 0-based column #
	Rect bounds;		// display position
	long textOffset;	// offset from table beginning to cell's text
	long textLength;	// length of cell's text
	PETEStyleListHandle styleInfo;
	PETEParaScrapHandle paraInfo;
} TabCellData, *TabCellPtr, **TabCellHandle;


//      Table info
typedef struct TableData {
	// data parsed from HTML
	short align;		// horizontal alignment
	short direction;	// text direction
	short width;		// width in pixels if positive, percent if negative
	short border;		// width of border
	short cellSpacing;	// space between cell borders
	short cellPadding;	// pad between cell contents and its borders
	Boolean cellSpacingSpecified;	// so we can tell the difference between zero and not specified cellspacing

	// misc data
	short rows;		// # of rows
	short rowHtSum;		// height of all rows
	short columns;		// # of columns in columnData
	short cells;		// # of cells in cellData;
	Boolean editing;	// are we editing in this table?
	PETEHandle pteEdit;	// pte used for editing
	TabRowGroupHandle rowGroupData;	// Info for each row group
	TabRowHandle rowData;	// Info for each row
	TabColHandle columnData;	// Info for each column
	TabColHandle columnGroupData;	// Info for each column group
	TabCellHandle cellData;	// Info for each cell
} TableData, *TablePtr, **TableHandle;

typedef struct TableInfo {
	PETEGraphicInfo pgi;
	TableHandle table;
	PETEHandle pteSpare;	// nil for cloned tables
	uLong offset;
	long maxWidth;
} TableInfo, *TIPtr, **TIHandle;


OSErr MakeTableGraphic(PETEHandle pte, long offset, TableHandle,
		       PETEStyleEntryPtr pse);
void CheckTableItems(MyWindowPtr win, Boolean allMenu);
void EnableTableMenu(MyWindowPtr win, Boolean all);
void TableMenuChoice(MyWindowPtr win, short item);
short TableColIndex(TabColHandle columnData, short c);
short TableColGroupIndex(TabColHandle columnGroupData, short c);
short TableRowIndex(TabRowHandle rowData, short r);
short TableRowGroupIndex(TabRowGroupHandle rowGroupData, short r);
void TableDispose(TableHandle table);
void GetCellHAlign(TableHandle table, short cell, short *hAlign,
		   UniChar * alignChar, short *charOff);
void GetCellVAlign(TableHandle table, short cell, short *vAlign);
void GetCellDirection(TableHandle table, short cell, short *direction);
pascal OSErr TableGraphic(PETEHandle pte, TIHandle table, long offset,
			  PETEGraphicMessage message, void *data);

#endif
