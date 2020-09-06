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

#ifndef MBWIN_H
#define MBWIN_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * handling the mailbox window
 **********************************************************************/
#define kMBDragType 'MBox'

//	special node ID's
enum	{ kEudoraFolder=-1,kIMAPFolder=-2 };

//	expand/collapse folder list
typedef struct ExpandListRec **ExpandListHandle;
typedef struct
{
	ExpandListHandle	hExpandList;
	short		resID;
	Boolean	fExpandListChanged;
} ExpandInfo, *ExpandInfoPtr;

#define kExpandListType 'ExMB'
enum	{ kMBExpandID=1001,kSearchExpandID,kAddressBookExpandID,kMBDrawerExpandID };

//	Data sent for a drag
typedef struct
{
	short		menuID;
	short		menuItem;
	Str32		name;
	FSSpec	spec;
} MBDragData, *MBDragPtr;

void OpenMBWin(void);
void MBTickle(UPtr fromSelect,UPtr toSelect);
MenuHandle ParentMailboxMenu(MenuHandle mh,short *itemPtr);
void MBFixUnread(MenuHandle mh,short item,Boolean unread);
void MBFixUnreadLo(ViewListPtr pView,MenuHandle mh,short item,Boolean unread,Boolean draw);
void MBOpenFolder(Handle hStringList,Boolean IsIMAP);
long MailboxesLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data);
void AddMailboxListItems(ViewListPtr pView, short menuId, ExpandInfoPtr pExpList);
void OpenMBFolder(ViewListPtr pView,short menuID,StringPtr s);
void SaveExpandStatus(VLNodeInfo *data,ExpandInfoPtr pExpList);
void SaveExpandedFolderList(ExpandInfoPtr pExpList);
long *FindExpandDirID(long dirID,ExpandInfoPtr pExpList);
Boolean IsIMAPBox(VLNodeInfo *pData);
Boolean MBFindInCollapsed(MyWindowPtr win,ViewListPtr pView,PStr what,short menuID);
short MBGetFolderMenuID(short id, StringPtr name);
void MBListOpen(ViewListPtr pView);
void MakeMBSpec(VLNodeInfo *pData,FSSpec *pSpec);
ExpandInfoPtr MBGetExpList(void);
ViewListPtr MBGetList(void);
typedef OSErr (*MBRenameProcPtr)(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn);
OSErr MBDragMessages(DragReference drag, VLNodeInfo *targetInfo);
#endif
