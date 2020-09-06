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

#ifndef LISTVIEW_H
#define LISTVIEW_H

//	VLNodeID - identifies a particular node in the list. Client specifies format for fields.
//	Can use menu ID, file folder it, etc.
typedef long	VLNodeID;

// VLCallbackMessage - Callback messages for list view
typedef enum 
{
	kLVAddNodeItems,			//	data = node ID
	kLVDeleteItem,				//	data = 0, use selected items
	kLVMoveItem,					//	data = VLNodeInfo *
	kLVCopyItem,					//	data = VLNodeInfo *
	kLVOpenItem,					//	data = 0, use selected items
	kLVRenameItem,				//	data = StringPtr to new name, use selected item
	kLVExpandCollapseItem,	//	data = VLNodeInfo *
	kLVGetParentID,				//	data = VLNodeInfo *
	kLVQueryItem,					//	data = VLNodeInfo *
	kLVSendDragData,			//	data = SendDragDataInfo *
	kLVGetFlags,					//	data = 0
	kLVAddDragItem,				//	data = SendDragDataInfo *
	kLVSelectItem,				//	data = VLNodeInfo *
	kLVUnselectItem,			//	data = VLNodeInfo *
	kLVDragDrop					//	data = SendDragDataInfo *
} VLCallbackMessage;

//	Queries
enum	{ kQuerySelect, kQueryDrag, kQueryRename, kQueryDrop,
	kQueryDragExpand, kQueryDropParent, kQueryDCOpens };

//	Flags
enum	
{
	kfAutoSelectAll						= 0x0001,	// Auto select all when initialized
	kfAutoSelectChild					=	0x0002,	// Set to automatically select child if parent is selected
	kfListSupportsFocus				= 0x0004,	// Draw the list with a focus rectangle when appropriate
	kfSupportsSelectCallbacks	= 0x0008,	// Supports list item select and unselect callbacks
	kfSupportsMondoBigList		= 0x0010,	// Supports lists containing more data than the List Manager was designed for
	kfManualRowAddsExpected		= 0x0020,	// Expects to perform its own LAddRow's in bulk
	kfSingleSelection          = 0x0040,   // Only 1 item selected at a time
	kfDropRoot						= 0x0080		// Allow dropping items at root level
 };

//	Focus
typedef enum
{
	kFocusNone = 0,
	kFocusList,
	kFocusEdit
} ListFocusType;

// Data for certain callbacks
typedef enum
{
	dataDeleteFromKeyboard,
	dataDeleteFromDrag
} DataCallbackType;

//	Use this constant on LVNew to indicate you want to add the
//	drag flavors yourself
#define kLVDoOwnDragAdd -1

typedef struct CellRec CellRec;

//	Prototype for callback function
typedef struct ViewListRec *ViewListPtr;
typedef long (*ViewListCallBackPtr)(ViewListPtr pView, VLCallbackMessage message, long data);

//	Prototype for drawing callback function
typedef Boolean (*DrawItemCallBackPtr)(ViewListPtr pView,short item,Rect *pRect,CellRec *pCellData,Boolean select,Boolean eraseName);
typedef Boolean (*FillBlankCallbackPtr)(ViewListPtr pView);
typedef Boolean (*GetDragRegionCallbackPtr)(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Rect *iconRect, Rect *nameRect);
typedef Boolean (*InterestingClickCallbackPtr)(ViewListPtr pView, CellRec *cellData, Rect *cellRect, Point pt);

/**********************************************************************
 * DrawDetails - details for list drawing
 **********************************************************************/
typedef struct DrawDetails
{	
	short 	arrowLeft;
	short	iconTop;
	short	iconLeft;			//	First icon level
	short 	iconLevelWidth;
	short 	textBottom;
	short 	rowHt;				// 	Hieght of a row
	short 	nameAddMargin;		//	Add to name width when drawing
	short 	maxNameWidth;		//	Approximate maximum name width
	short 	keyNavTicks;		//	Delay accepted between navigation keys
	
	
	DrawItemCallBackPtr DrawRowCallback;	// callback to draw an individual row
	FillBlankCallbackPtr FillBlankCallback;		// callback to fill empty space at end of list
	GetDragRegionCallbackPtr GetCellRectsCallBack;	// callback to calculate the drag region
	InterestingClickCallbackPtr InterestingClickCallback;	// callback to tell if a point is interesting
	
} DrawDetailsStruct, *DrawDetailsPtr;

typedef struct {
	VLNodeID	nodeID;			// ID of this selected node
	short			row;				// List row (zero based)
} SelectedListRec, *SelectedListPtr, **SelectedListHandle;

/**********************************************************************
 * ViewListRec - Main record for a view list
 **********************************************************************/
typedef struct ViewListRec
{	
	Rect			bounds;					//	List bounds, includes vertical scroll bar on right
	short			selectCount;		//	# of items currently selected
	SelectedListHandle hSelectList;	//	List of row numbers (0-based) and nodeID's of selected items
	MyWindowPtr	wPtr;					//	The window containing the list
	VLNodeID		rootNodeID;		//	ID of root directory of list
	ViewListCallBackPtr		pCallBack;	//	Callback function
	ListHandle	hList;				//	List handle
	OSType		dragFlavor;			//	Flavor of items dragged out of list	
	short			dragHiliteRow;	//	Row currently highlighted during a drag
	PETEHandle	pte;					//	pte for renaming
	short			renameRow;			//	Row # of item being renamed
	short			listStatus;			//	Zero if draw list
	short			maxLevel;				//	Deepest level
	short			maxNameWidth;		//	Width of widest name
	short			font;						//	List font
	short			fontSize;				//	List font size
	long			flags;					//	See flags enum
	long			dragGroup;			//	What group do these drag items belong to?
	Boolean		dragSelect;			//  Is a drag selection in progress?
	ListFocusType	listFocus;	//	Where should the focus be?
	DrawDetailsStruct details;			// constants for list drawing
} ViewList,*ViewListPtr;

//	SICN for triangle icon
enum
{
	kRightOutlineSICN=0,
	kRightSICN,
	kDiagonalSICN,
	kDownSICN,
	kDownOutlineSICN
};

/**********************************************************************
 * VLNodeInfo - Info requested by view list manager
 **********************************************************************/
typedef struct
{	
	short	iconID;
	VLNodeID	nodeID;
	Boolean	isParent;		//	Set if is a parent
	Boolean	isCollapsed;		//	Set if parent and collapsed
	Boolean	useLevelZero;	//	Used for adding items
	short	query;	//	Used for kLVQueryItem
	short	rowNum;	//	1-based
	Style	style;	//	Style for drawing name
	long	refCon;
	Str32	name;
} VLNodeInfo;

/**********************************************************************
 * SendDragDataInfo - Info for sending drag data
 **********************************************************************/
typedef struct
{
	DragReference	drag;
	ItemReference itemRef;
	FlavorType flavor;
	VLNodeInfo	*info;
	
} SendDragDataInfo;

/**********************************************************************
 * CellRec - Info contained in each cell
 **********************************************************************/
typedef struct CellRec
{	
	short	iconID;
	VLNodeID	nodeID;
	struct
	{
		short	fParent:1;
		short fCollapsed:1;
		short	fHaveChildren:1;
		short	level:5;
		short	style:8;
	}		misc;
	long	refCon;
	Str32	name;
} CellRec;

/**********************************************************************
 * Prototypes
 **********************************************************************/

//	Create a new list view. Pass ID of node of root folder.
OSErr LVNew(ViewListPtr pView,MyWindowPtr wPtr,Rect *pBounds,VLNodeID rootNodeID,
	ViewListCallBackPtr pCallBack,OSType dragFlavor);

//	Create a new list view. More control
OSErr LVNewWithDetails(ViewListPtr pView,MyWindowPtr wPtr,Rect *pBounds,VLNodeID rootNodeID,
	ViewListCallBackPtr pCallBack,OSType dragFlavor,DrawDetailsPtr details);
	
//	Initialize details for LVNewWithDetails
void LVInitDetails(DrawDetailsPtr details);

//	Dispose of a list view
void LVDispose(ViewListPtr pView);

//	Add items to a list view. Should only be called in response to a kLVAddNodeItems request
OSErr LVAdd(ViewListPtr pView, VLNodeInfo *pInfo);

//	Call when there is a click in the window, even if not in the list view
//	Returns true if handled
Boolean LVClick(ViewListPtr pView,EventRecord *event);

//	Call on a key event. Returns true if key event was handled
Boolean LVKey(ViewListPtr pView,EventRecord *event);

//	Draw the list view. Call on update events.
void LVDraw(ViewListPtr pView, RgnHandle hRgn, Boolean checkList, Boolean fDrawFrame);

//	Activate/deactivate list view.
void LVActivate(ViewListPtr pView, Boolean activate);

//	Resize list view. pHeightAdjustment returns with a value indicating the differenct between the
//	height as indicated with pRect and the actual height. The actual height may be different because the
//	list height is an even multiple of the row height of each entry.
void LVSize(ViewListPtr pView,Rect *pRect,short *pHeightAdjustment);

//	Returns the number of items currently selected.
short	LVCountSelection(ViewListPtr pView);

//	Returns the cell data for the indicated item. item is either the nth item (1-based) in the
//	list or the nth item in the current selection.
Boolean	LVGetItem(ViewListPtr pView,short item,VLNodeInfo *pInfo,Boolean fFromSelection);

//	Returns the cell data for the item matching a particular nodeID.
Boolean	LVGetItemWithNodeID(ViewListPtr pView,VLNodeID nodeID,VLNodeInfo *pInfo);

//	Sets the cell data for the item matching a particular nodeID.
Boolean LVSetItemWithNodeID(ViewListPtr pView,VLNodeID nodeID,VLNodeInfo *pInfo, Boolean update);

//	Force the entire list to be regenerated.
void InvalidListView(ViewListPtr pView);

//	Process a Drag Manager message
OSErr LVDrag(ViewListPtr pView,DragTrackingMessage which,DragReference drag);

//	Check to see if the list view is doing a drag slection
Boolean LVDragSelectInProgress (ViewListPtr pView);

//	finish up drag and return item dropped on
Boolean LVDrop(ViewListPtr pView,VLNodeInfo *pInfo);

//	update the text style for an item
void LVUpdateStyle(ViewListPtr pView,VLNodeID id,StringPtr sName,Style theStyle,Boolean fDraw);

//	Rename a list view item
void LVRename(ViewListPtr pView,VLNodeID id,StringPtr sName,Boolean openAfterRename,Boolean hahaOnlyKidding);

//	Expand/collapse a list view item
void LVExpand(ViewListPtr pView,VLNodeID id,StringPtr sName,Boolean expand);

//	Select a list view item
Boolean LVSelect(ViewListPtr pView,VLNodeID id,StringPtr sName,Boolean addToSelection);

//	Unselect a list view item
Boolean LVUnselect(ViewListPtr pView,VLNodeID id,StringPtr sName,Boolean removeFromSelection);

//	Make sure our internal selection tables are correct
void LVUpdateSelection(ViewListPtr pView);

//	See if "checkInfo" is a descendent of "inInfo". 
//	Return 0 if not or generation if it is: 1=child, 2=grandchild, etc
short LVDescendant(ViewListPtr pView,VLNodeInfo *checkInfo,VLNodeInfo *inInfo);

//	Get the size of the list. Used to determine the maximum size to zoom the window.
void LVMaxSize(ViewListPtr pView, short *pWd, short *pHt);

//	Return the drag flavor if currently dragging
OSType LVDragFlavor(void);

//	Is the indicated row selected?
Boolean LVIsSelected(ViewListPtr pView,short row);

//	Calculate the adjust size the list will be
void LVCalcSize(ViewListPtr pView, Rect *r);

//	Select all selectable items in list
Boolean LVSelectAll(ViewListPtr pView);

// 	Scroll a list to the top automagically
void ScrollTopListView(ViewListPtr pView);

// 	Scroll a list up or down
void LVScroll(ViewListPtr pView,SInt32 lines);

//	Copy selected item names to clipboard
void LVCopy(ViewListPtr pView);

// Draw the focus
void LVChangeFocus(ViewListPtr pView, ListFocusType newFocus);
Boolean LVSetKeyboardFocus(ViewListPtr pView, Boolean getFocus);
Boolean LVAdvanceKeyboardFocus (ViewListPtr pView);
Boolean LVReverseKeyboardFocus (ViewListPtr pView);
void LVDrawFocus(ViewListPtr pView);

short LVGetNextRowToAdd (void);

#endif
