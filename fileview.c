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

void FVSizeDispose(FileViewHandle fvh);
#include "fileview.h"
#define FILE_NUM 58

#define kHeaderButtonHeight (GROW_SIZE+5)
#define kCheckSeconds	5	//	How often to check for upates to folder?
#define kFVButtonRef 'fvew'

typedef struct
{
	Str32	name;
	VLNodeID nodeID;
	Boolean	folder;
	uLong	size;
	uLong	date;
} FVFileInfo;

long FileViewLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data);
static Boolean FileViewDrawRow(ViewListPtr pView,short item,Rect *pRect,CellRec *pCellData,Boolean select,Boolean eraseName);
static Boolean FileViewFillBlank(ViewListPtr pView);
static Boolean FileViewGetCellRects(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect);
static Boolean FileViewInterestingClick(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Point pt);
static void FileViewGetRects(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect, Point *ptName);
static void DrawListColors(FileViewHandle fvh, Rect *pRect, Boolean separator);
static void FVAddFiles(ViewListPtr pView,FileViewHandle fvh,long dirId);
void DrawFitting(StringPtr s, short width);
void SetColButton(FileViewHandle fvh);
static void FVSwapFiles(FVFileInfo *f1,FVFileInfo *f2);
static int FVCompareName(FVFileInfo *f1,FVFileInfo *f2);
static int FVCompareSize(FVFileInfo *f1,FVFileInfo *f2);
static int FVCompareDate(FVFileInfo *f1,FVFileInfo *f2);

short colLeft[kFDColCount+1] = { 0,250,395,457 };
short gCntlId[kFDColCount] = { FV_NAME_CNTL,FV_DATE_CNTL,FV_SIZE_CNTL };

/************************************************************************
 * NewFileView - create a new file view
 ************************************************************************/
void NewFileView(MyWindowPtr win, FileViewHandle hData)
{
	ViewListPtr	pList;
	WindowPtr	winPtr = GetMyWindowWindowPtr(win);
	
	if (pList=NuPtr(sizeof(ViewList)))
	{	
		Rect	rList;
		DrawDetailsStruct details;
		short	i;

		(*hData)->list = pList;
		GetPortBounds(GetMyWindowCGrafPtr(win),&rList);
		rList.top = win->topMargin;
		LVInitDetails(&details);
		details.DrawRowCallback = FileViewDrawRow;
		details.FillBlankCallback = FileViewFillBlank;
//		details.GetCellRectsCallBack = FileViewGetCellRects;
//		details.InterestingClickCallback = FileViewInterestingClick;
		LVNewWithDetails(pList,win,&rList,(*hData)->dirId,FileViewLVCallBack,kLVDoOwnDragAdd,&details);

		for (i = 0;i < kFDColCount;i++)
		{
			ControlRef	cntl = NewIconButton(gCntlId[i],winPtr);
			
			if (cntl && win->topMarginCntl) EmbedControl(cntl,win->topMarginCntl);
			SetControlReference(cntl,kFVButtonRef+i);

			switch (i)
			{
				case kNameCol:	
					SetBevelTextOffset(cntl,details.iconLevelWidth*2); 
					break;
				case kSizeCol:
					SetBevelTextAlign(cntl,kControlBevelButtonAlignTextFlushRight);
				case kDateCol:	
					SetBevelTextOffset(cntl,5); 
					break;
			}
			(*hData)->controls[i] = cntl;
		}
		SetColButton(hData);
	}
}

/************************************************************************
 * FVDispose - dispose of file view
 ************************************************************************/
void FVDispose(FileViewHandle fvh)
{
	ZapHandle((*fvh)->expandList.hExpandList);
	LVDispose((*fvh)->list);
	ZapPtr((*fvh)->list);
}

/************************************************************************
 * FVRemoveButtons - get rid of buttons added to window
 ************************************************************************/
void FVRemoveButtons(MyWindowPtr win)
{
	short	i;
	ControlRef	cntl;
	
	for (i = 0;i < kFDColCount;i++)
	{
		cntl = FindControlByRefCon(win,kFVButtonRef+i);		
		if (cntl)
			DisposeControl(cntl);
		else
			return;
	}
}

/************************************************************************
 * FileViewLVCallBack - callback function for List View
 ************************************************************************/
long FileViewLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data)
{
	OSErr	err = noErr;
	FileViewHandle	fvh = GetFileViewInfo(pView->wPtr);
	CInfoPBRec hfi;
	VLNodeInfo	*pInfo, lvInfo, folderData;
	FSSpec	spec,toSpec;
	short	i;
	short	vRefNum = fvh?(*fvh)->vRefNum:0;
	Boolean	fMovedFolder,reDo=false;
	SendDragDataInfo	*pSendData;
	HFSFlavor	hfsInfo;

	switch (message)
	{
		case kLVAddNodeItems:
			FVAddFiles(pView,fvh,data);
			break;
			
		case kLVExpandCollapseItem:
			pInfo = (VLNodeInfo*)data;
			pInfo->refCon = pInfo->nodeID;
			LDRef(fvh);
			SaveExpandStatus(pInfo,&(*fvh)->expandList);
			UL(fvh);
			break;

		case kLVGetParentID:
			hfi.dirInfo.ioDrDirID = ((VLNodeInfo *)data)->nodeID;
			hfi.dirInfo.ioNamePtr = lvInfo.name;
			hfi.dirInfo.ioVRefNum = vRefNum;
			hfi.dirInfo.ioFDirIndex = -1;		/* use ioDirID */
			if (PBGetCatInfo(&hfi,0)) /* get working directory info */
				return nil;
			return hfi.dirInfo.ioDrDirID;
			break;
		
		case kLVMoveItem:
		case kLVCopyItem:
			pInfo = (VLNodeInfo*)data;
			FSMakeFSSpec(vRefNum,pInfo->nodeID,"",&toSpec);
			IsAlias(&toSpec,&toSpec);
			if (toSpec.vRefNum != vRefNum)
			{
				//	aliased to another volume, no can do
				WarnUser(CANT_VOL_MOVE,0);
				return 0;
			}
			fMovedFolder = false;
			//	fall thru
		case kLVDeleteItem:
			if (data == dataDeleteFromDrag) break;	//	Don't handle drag to trash here
		case kLVOpenItem:
			for (i=1;i<=LVCountSelection(pView);i++)
			{
				LVGetItem(pView,i,&lvInfo,true);
				FSMakeFSSpec(vRefNum,lvInfo.nodeID,lvInfo.isParent?"":lvInfo.name,&spec);
				switch (message)
				{
					case kLVOpenItem:
						OpenOtherDoc(&spec,false,false,nil);
						break;
					case kLVDeleteItem:
						FSpTrash(&spec);
						reDo = true;
						break;
					case kLVMoveItem:
					case kLVCopyItem:
						if (fMovedFolder && LVDescendant(pView,&lvInfo,&folderData))
							//	We have already moved an ancestor of this file. Don't
							//	try to move it again
							continue;
						if (fMovedFolder = lvInfo.isParent)
							//	Moved a folder
							folderData = lvInfo;
						if (LVDescendant(pView,&lvInfo,pInfo)==1)
							//	Don't try to move a child into its parent--it's already there
							continue;
						if (message==kLVMoveItem)
							err = FSpCatMove(&spec,&toSpec);
						else
						{
							toSpec.parID = SpecDirId(&toSpec);
							PCopy(toSpec.name,spec.name);
							if (lvInfo.isParent)
							{
								uLong	createdDirID;
								
								spec.parID = SpecDirId(&spec);
								if (!(err = FSpDirCreate(&toSpec,smSystemScript,&createdDirID)))
								{
									toSpec.parID = createdDirID;
									err = FSpDupFolder(&toSpec,&spec,false,false);
								}
							}
							else
								err = FSpDupFile(&toSpec,&spec,false,false);							
						}
						reDo = true;
						break;
				}
			}
			break;

		case kLVRenameItem:
		{
			Str31 newName;
			SanitizeFN(newName,(StringPtr)data,MAC_FN_BAD,MAC_FN_REP,false);
			LVGetItem(pView,1,&lvInfo,true);
			if (*newName && !EqualString(newName,lvInfo.name,true,true))
			{
				FSMakeFSSpec(vRefNum,lvInfo.nodeID,lvInfo.isParent?"":lvInfo.name,&spec);
				FSpRename(&spec,newName);
				reDo = true;
			}
			break;
		}
		
		case kLVQueryItem:
			pInfo = (VLNodeInfo*)data;
			switch (pInfo->query)
			{
				case kQuerySelect:
				case kQueryDrag:
				case kQueryRename:
				case kQueryDropParent:
				case kQueryDragExpand:
					return true;
				case kQueryDrop:
					return pInfo->isParent;
				case kQueryDCOpens:
					return !pInfo->isParent;
				default:
					return false;
			}
			break;
		
		case kLVSendDragData:
		case kLVAddDragItem:
			pSendData = (SendDragDataInfo*)data;
			pInfo = pSendData->info;
			FSMakeFSSpec(vRefNum,pInfo->nodeID,pInfo->isParent?"":pInfo->name,&hfsInfo.fileSpec);
			if (pInfo->isParent)
			{
				hfsInfo.fileType = 'MACS';
				hfsInfo.fileType = 'fold';
				hfsInfo.fdFlags = 0;
			}
			else
			{
				FInfo	fndrInfo;
				FSpGetFInfo(&hfsInfo.fileSpec,&fndrInfo);
				hfsInfo.fileType = fndrInfo.fdType;
				hfsInfo.fileCreator = fndrInfo.fdCreator;
				hfsInfo.fdFlags = fndrInfo.fdFlags;
			}
			AddDragItemFlavor(pSendData->drag,pSendData->itemRef,flavorTypeHFS,&hfsInfo,sizeof(hfsInfo),flavorNotSaved);
			//	Make sure we cause the list to be updated
			(*fvh)->lastUpdateCheck = 0;
			(*fvh)->lastUpdate = 0;
			break;

		case kLVDragDrop:
			//	Drop from external source
			pSendData = (SendDragDataInfo*)data;
			pInfo = pSendData->info;
			if (pInfo->isParent)
			{
				short	i,n;
				short	modifiers,mouseDownModifiers,mouseUpModifiers;
				
				GetDragModifiers(pSendData->drag, &modifiers, &mouseDownModifiers, &mouseUpModifiers);				
				FSMakeFSSpec(vRefNum,pInfo->nodeID,"",&toSpec);
				n = MyCountDragItems(pSendData->drag);
				
				for (i=1;i<=n && !err; i++)
				{
					Handle data = nil;
					if (!(err=MyGetDragItemData(pSendData->drag,i,flavorTypeHFS,&data)))
					{
						//	File drag from Finder
						spec = (*(struct HFSFlavor**)data)->fileSpec;
						ZapHandle(data);
						if ((modifiers|mouseDownModifiers|mouseUpModifiers)&optionKey)
						{
							//	copy files
							toSpec.parID = SpecDirId(&toSpec);
							PCopy(toSpec.name,spec.name);
							if (AFSpIsItAFolder(&spec))
							{
								uLong	createdDirID;
								
								spec.parID = SpecDirId(&spec);
								if (!(err = FSpDirCreate(&toSpec,smSystemScript,&createdDirID)))
								{
									toSpec.parID = createdDirID;
									err = FSpDupFolder(&toSpec,&spec,false,false);
								}
							}
							else
								err = FSpDupFile(&toSpec,&spec,false,false);
						}
						else
							//	move files
							err = FSpCatMove(&spec,&toSpec);						
						reDo = true;
					}
				}
			}
			break;

		case kLVGetFlags:
			return kfDropRoot;
	}
	if (reDo) InvalidListView(pView);
	return err;
}

/**********************************************************************
 * FileViewUpdate - update the window in file view mode
 **********************************************************************/
void FileViewUpdate(MyWindowPtr win)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	
	if (fvh) LVDraw((*fvh)->list,MyGetPortVisibleRegion(GetMyWindowCGrafPtr(win)),true,false);
}

/**********************************************************************
 * FileViewClick - handle a click in the file view
 **********************************************************************/
void FileViewClick(MyWindowPtr win,EventRecord *event)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	
	if (win->isActive)
	{
		if (!fvh || !LVClick((*fvh)->list,event))
		{
			Point pt = event->where;
			SetPort_(GetMyWindowCGrafPtr(win));
			GlobalToLocal(&pt);
			HandleControl(pt,win);
		}
	}
	else
		SelectWindow_(GetMyWindowWindowPtr(win));
}

/************************************************************************
 * FileViewCursor - change the cursor appropriately
 ************************************************************************/
void FileViewCursor(Point mouse)
{
	MyWindowPtr win=GetWindowMyWindowPtr(FrontWindow_());

	if (win && !PeteCursorList(win->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/**********************************************************************
 * FileViewDragHandler - handle dragging in file view
 **********************************************************************/
OSErr FileViewDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{
	OSErr err = noErr;
	
	if (DragIsInteresting(drag,kDragFlavorTypeHFS,TOC_FLAVOR,nil))
	{
		FileViewHandle	fvh = GetFileViewInfo(win);
		
		err = LVDrag((*fvh)->list,which,drag);
	}
	return err;
}

/************************************************************************
 * FileViewKey - handle keystrokes in file view
 ************************************************************************/
Boolean FileViewKey(MyWindowPtr win, EventRecord *event)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	
	return 	LVKey((*fvh)->list,event);
}

/**********************************************************************
 * FileViewFind - find in the window
 **********************************************************************/
Boolean FileViewFind(MyWindowPtr win,PStr what)
{
	FileViewHandle	fvh = GetFileViewInfo(win);

	return FindListView(win,(*fvh)->list,what);
}

/************************************************************************
 * MBMenu - menu choice in the mailbox window
 ************************************************************************/
Boolean FileViewMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
#pragma unused(modifiers)
	FileViewHandle	fvh = GetFileViewInfo(win);

	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_OPENSEL_ITEM:
					FileViewLVCallBack((*fvh)->list,kLVOpenItem,nil);
					return(True);
			}
			break;
		case EDIT_MENU:
			switch(item)
			{
				case EDIT_SELECT_ITEM:
					LVSelectAll((*fvh)->list);
					return(true);
				case EDIT_COPY_ITEM:
					LVCopy((*fvh)->list);
					return true;
			}
			break;
	}
	return(False);
}

/************************************************************************
 * FileViewDrawRow - callback function for List View.  Draw a row.
 ************************************************************************/
static Boolean FileViewDrawRow(ViewListPtr pView,short item,Rect *pRect,CellRec *pCellData,Boolean select,Boolean eraseName)
{
	Boolean result = true;
	Rect rIcon, rName;
	Point ptName;
	FileViewHandle	fvh = GetFileViewInfo(pView->wPtr);
	FSSpec	spec;
	static Boolean checkedIconServices, haveIconServices;
	IconRef iconRef;
	SInt16	theLabel;
	CInfoPBRec hfi;
	Str255	s;
	Str31 s2;
	SInt32	gestaltResult;
		
	//	Draw the list background color and seperator lines for this row
	DrawListColors(fvh,pRect,true);

	//	Get file info
	FSMakeFSSpec((*fvh)->vRefNum,pCellData->nodeID,pCellData->misc.fParent?"":pCellData->name,&spec);
	FSpGetHFileInfo(&spec,&hfi);
	
	//	Get icon
	if (!checkedIconServices)
		haveIconServices = 	!Gestalt(gestaltIconUtilitiesAttr,&gestaltResult) && (gestaltResult&gestaltIconUtilitiesHasIconServices) && GetIconRefFromFile;
	iconRef = nil;
	if (haveIconServices) GetIconRefFromFile(&spec,&iconRef,&theLabel);

	// 	Draw the contents of this item	
	TextFont(pView->font);
	TextSize(pView->fontSize);
	FileViewGetRects(pView,pCellData,pRect,&rIcon,&rName,&ptName);

	//	Plot the icon
	if (iconRef)
	{
		PlotIconRef(&rIcon,atNone+atHorizontalCenter,select ? ttSelected : ttNone,0,iconRef);
		ReleaseIconRef(iconRef);
	}
	else if (pCellData->iconID)
		PlotIconID(&rIcon,atNone+atHorizontalCenter,select ? ttSelected : ttNone,pCellData->iconID);
	
	//	Draw the name
	MoveTo(ptName.h,ptName.v);
	DrawFitting(pCellData->name,colLeft[kNameCol+1]-ptName.h);
	if (select)
	{
		//	highlight name
		Point	penLoc;
		
		GetPortPenLocation(GetQDGlobalsThePort(),&penLoc);
		rName.right = penLoc.h+5;
		InvertRect(&rName);
	}

	//	Draw date, time
	DateString(hfi.hFileInfo.ioFlMdDat,shortDate,s,nil);
	TimeString(hfi.hFileInfo.ioFlMdDat,false,s2,nil);
	PCat(s,"\p, ");
	PCat(s,s2);	
	MoveTo(colLeft[kDateCol]+5,ptName.v);
	DrawFitting(s,colLeft[kDateCol+1]-colLeft[kDateCol]-5);
	
	//	Draw size
	if (pCellData->misc.fParent)
		//	folder
		PCopy(s,"\p-");
	else
	{
		//	file
		uLong	fileSize = (hfi.hFileInfo.ioFlPyLen + hfi.hFileInfo.ioFlRPyLen) / 1024;
		char	cBytes = 'K';
		
		if (fileSize >= 1024)
		{
			fileSize /= 1024;
			cBytes = 'M';
		}
		NumToString(fileSize,s);
		PCatC(s,' ');
		PCatC(s,cBytes);
	}
	MoveTo(colLeft[kSizeCol+1]-5-StringWidth(s),ptName.v);	//	right-justify
	DrawString(s);

	//	Set background color in case disclosure triangle needs to be drawn
	//MJD if (PrefIsSet(PREF_USE_LIST_COLORS))
	//MJD	SetThemeBackground((*fvh)->prefs.sortCol?kThemeBrushListViewBackground:kThemeBrushListViewSortColumnBackground,RectDepth(pRect),true);

	return (result);
}

/************************************************************************
 * DrawFitting - draw string making sure it fits
 ************************************************************************/
void DrawFitting(StringPtr s, short width)
{
	if (StringWidth(s)>width)
	{
		TextFace(condense);
		if (StringWidth(s)>width)
			TruncString(width,s,smTruncMiddle);
	}
	DrawString(s);
}

/************************************************************************
 * FileViewFillBlank - callback function for List View.  Fill unused 
 *	parts of the list.
 ************************************************************************/
static Boolean FileViewFillBlank(ViewListPtr pView)
{
	ListHandle	hList=pView->hList;
	Rect	rErase;
	GrafPtr	savePort;
	FileViewHandle	fvh = GetFileViewInfo(pView->wPtr);
	
	if ((*hList)->visible.bottom > (*hList)->dataBounds.bottom)
	{
		SAVE_STUFF;

		SET_COLORS;
		GetPort(&savePort);
		SetPort(GetMyWindowCGrafPtr(pView->wPtr));
		Cell1Rect(hList,(*hList)->dataBounds.bottom+1,&rErase);
		rErase.bottom = pView->bounds.bottom;

		DrawListColors(fvh,&rErase,false);
			
		SetPort(savePort);
		REST_STUFF;
	}
	return true;
}

/************************************************************************
 * DrawListColors - draw list colors
 ************************************************************************/
static void DrawListColors(FileViewHandle fvh, Rect *pRect, Boolean separator)
{
	Rect columnRect;
	short	depth;
	
	if (UseListColors)
	{
		//	draw sort column
		SetRect(&columnRect,pRect->left+colLeft[(*fvh)->prefs.sortCol],pRect->top,pRect->left+colLeft[(*fvh)->prefs.sortCol+1],pRect->bottom-1);
		depth = RectDepth(&columnRect);
		SetThemeBackground(kThemeBrushListViewSortColumnBackground,depth,true);
		EraseRect(&columnRect);

		SetThemeBackground(kThemeBrushListViewBackground,depth,true);
		if ((*fvh)->prefs.sortCol)
		{
			//	draw before sort column
			columnRect.left = 0;
			columnRect.right = colLeft[(*fvh)->prefs.sortCol]-1;
			EraseRect(&columnRect);
		}
		
		//	draw after sort column
		columnRect.left = colLeft[(*fvh)->prefs.sortCol+1]+1;
		columnRect.right = pRect->right;
		EraseRect(&columnRect);
		
		//	draw separator
		SetThemeBackground(kThemeBrushListViewSeparator,depth,true);
		SetRect(&columnRect,0,columnRect.bottom,pRect->right,columnRect.bottom+1);
		EraseRect(&columnRect);
	}
}

/************************************************************************
 * FileViewGetRects - return true if this we should drag
 ************************************************************************/
static void FileViewGetRects(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect, Point *ptName)
{
	short	offset;
	Point	pt;
	FontInfo	fInfo;
	SAVE_STUFF;
	
	//	Icon
	offset = pView->details.iconLeft + cellData->misc.level*pView->details.iconLevelWidth;
	if (cellData->iconID)
	{
		SetRect(iconRect,cellRect->left+offset,cellRect->top+pView->details.iconTop,cellRect->left+offset+16,cellRect->top+pView->details.iconTop+16);
		offset += pView->details.iconLevelWidth;
	}
	else
		SetRect(iconRect,0,0,0,0);

	//	Name
	TextFont(pView->font);
	TextSize(pView->fontSize);
	TextFace(cellData->misc.style);
	pt.h = cellRect->left + offset;
	pt.v = cellRect->top+(*pView->hList)->cellSize.v-pView->details.textBottom;
	*ptName = pt;
	GetFontInfo(&fInfo);
	SetRect(nameRect,pt.h,pt.v-fInfo.ascent-fInfo.leading,cellRect->right,pt.v+fInfo.descent);
	REST_STUFF;
}

/************************************************************************
 * FileViewGetCellRects - return true if this we should drag
 ************************************************************************/
static Boolean FileViewGetCellRects(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect)
{
	Point ptName;
	
	FileViewGetRects(pView,cellData,cellRect,iconRect,nameRect,&ptName);
	return (true);
}

/************************************************************************
 * FileViewInterestingClick - return true if this point is interesting
 ************************************************************************/
static Boolean FileViewInterestingClick(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Point pt)
{
	Boolean result = false;
	Rect rIcon, rName;
	
	FileViewGetCellRects(pView,cellData,cellRect,&rIcon,&rName);
	if (PtInRect(pt,&rIcon) || PtInRect(pt,&rName)) result = true;
	
	return (result);
}

/**********************************************************************
 * GetFileViewInfo - get file view info from tab control
 **********************************************************************/
FileViewHandle GetFileViewInfo(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	
	return tocH ? (FileViewHandle)(*tocH)->hFileView : nil;
}

/**********************************************************************
 * SetColButton - set correct header button for current sort
 **********************************************************************/
void SetColButton(FileViewHandle fvh)
{
	short	i;
	
	for(i = 0;i < kFDColCount;i++)
		SetControlValue((*fvh)->controls[i],i==(*fvh)->prefs.sortCol?1:0);
}

/**********************************************************************
 * FVSizeHeaderButtons - size and position header buttons
 **********************************************************************/
void FVSizeHeaderButtons(MyWindowPtr win, FileViewHandle fvh)
{
	short	i;	
	for(i = 0;i < kFDColCount;i++)
		MoveMyCntl((*fvh)->controls[i],colLeft[i],win->topMargin-kHeaderButtonHeight,colLeft[i+1]-colLeft[i],kHeaderButtonHeight);
}

/************************************************************************
 * FileViewButton - handle a hit in a control
 ************************************************************************/
void FileViewButton(MyWindowPtr win,ControlHandle cntl,long modifiers,short part)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	short	i;
	
	for(i = 0;i < kFDColCount;i++)
		if ((*fvh)->controls[i] == cntl)
		{
			if ((*fvh)->prefs.sortCol != i)
			{
				(*fvh)->prefs.sortCol = i;
				(*fvh)->dirtyPrefs = true;
				SetColButton(fvh);
				InvalidListView((*fvh)->list);
			}
			break;
		}
}

/************************************************************************
 * FVAddFiles - add files to list view
 ************************************************************************/
static void FVAddFiles(ViewListPtr pView,FileViewHandle fvh,long dirId)
{
	VLNodeInfo	lvInfo;
	CInfoPBRec hfi;
	Accumulator	a;
	FVFileInfo	fileInfo,*pInfo;
	void	*compare;
	short	count=0,i;

	if (AccuInit(&a)) return;
	
	Zero(lvInfo);
	hfi.hFileInfo.ioNamePtr = fileInfo.name;
	hfi.hFileInfo.ioFDirIndex = 0;
	while (!DirIterate((*fvh)->vRefNum,dirId,&hfi))
		if (!(hfi.hFileInfo.ioFlFndrInfo.fdFlags & fInvisible))
		{
			if ((hfi.hFileInfo.ioFlAttrib & ioDirMask))
			{
				//	folder
				fileInfo.folder = true;
				fileInfo.nodeID = hfi.dirInfo.ioDrDirID;
				fileInfo.size = 0;
			}
			else
			{
				//	file
				fileInfo.folder = false;
				fileInfo.nodeID = dirId;
				fileInfo.size = hfi.hFileInfo.ioFlPyLen + hfi.hFileInfo.ioFlRPyLen;
			}
			fileInfo.date = hfi.hFileInfo.ioFlMdDat;
			AccuAddPtr(&a,&fileInfo,sizeof(fileInfo));
			count++;
		}

	//	Sort
	switch ((*fvh)->prefs.sortCol)
	{
		case kDateCol:	compare = FVCompareDate; break;
		case kSizeCol:	compare = FVCompareSize; break;
		default:		compare = FVCompareName; break;
	}
	QuickSort((void*)LDRef(a.data),sizeof(fileInfo),0,count-1,compare,(void*)FVSwapFiles);

	//	Put in list
	for(i=0,pInfo=LDRef(a.data);i<count;i++,pInfo++)
	{
		PCopy(lvInfo.name,pInfo->name);
		lvInfo.nodeID = pInfo->nodeID;
		if (pInfo->folder)
		{
			//	folder
			lvInfo.iconID = kGenericFolderIconResource;
			lvInfo.isParent = true;
			LDRef(fvh);
			lvInfo.isCollapsed = !FindExpandDirID(lvInfo.nodeID,&(*fvh)->expandList);
			UL(fvh);
		}
		else
		{
			//	file
			lvInfo.iconID = kGenericDocumentIconResource;
			lvInfo.isParent = false;
		}
		LVAdd(pView,&lvInfo);
	}
	
	if (dirId == (*fvh)->dirId)
	{
		Zero(hfi);
		hfi.dirInfo.ioDrDirID = dirId;
		hfi.dirInfo.ioVRefNum = (*fvh)->vRefNum;
		hfi.dirInfo.ioFDirIndex = -1;		/* use ioDirID */
		PBGetCatInfo(&hfi,0);
		(*fvh)->lastUpdate = hfi.dirInfo.ioDrMdDat;
		(*fvh)->lastUpdateCheck = TickCount();
	}
}

/************************************************************************
 * FVSwapFiles - used for QuickSort
 ************************************************************************/
static void FVSwapFiles(FVFileInfo *f1,FVFileInfo *f2)
{
	FVFileInfo t;
	t = *f1;
	*f1 = *f2;
	*f2 = t;
}

/************************************************************************
 * FVCompareName - used for QuickSort
 ************************************************************************/
static int FVCompareName(FVFileInfo *f1,FVFileInfo *f2)
{
	return StringComp(f1->name,f2->name);
}

/************************************************************************
 * FVCompareSize - used for QuickSort
 ************************************************************************/
static int FVCompareSize(FVFileInfo *f1,FVFileInfo *f2)
{
	return (f1->size-f2->size);
}

/************************************************************************
 * FVCompareDate - used for QuickSort
 ************************************************************************/
static int FVCompareDate(FVFileInfo *f1,FVFileInfo *f2)
{
	return (f1->date-f2->date);
}

/**********************************************************************
 * FileViewActivate - activate/deactivate
 **********************************************************************/
void FileViewActivate(MyWindowPtr win)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	LVActivate((*fvh)->list, win->isActive);
}

/************************************************************************
 * FileViewIdle - idle routine for file view
 ************************************************************************/
void FileViewIdle(MyWindowPtr win)
{
	FileViewHandle	fvh = GetFileViewInfo(win);
	
	//	See if there have been any changes to the folder since the list
	//	time we got the files
	if (fvh && TickCount() > (*fvh)->lastUpdateCheck+(kCheckSeconds*60))
	{
		CInfoPBRec hfi;

		Zero(hfi);
		hfi.dirInfo.ioDrDirID = (*fvh)->dirId;
		hfi.dirInfo.ioVRefNum = (*fvh)->vRefNum;
		hfi.dirInfo.ioFDirIndex = -1;		/* use ioDirID */
		PBGetCatInfo(&hfi,0);
		if (hfi.dirInfo.ioDrMdDat > (*fvh)->lastUpdate)
			//	It's been changed. Update it.
			InvalidListView((*fvh)->list);
	}
}