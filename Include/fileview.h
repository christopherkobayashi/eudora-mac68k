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

#ifndef FILEVIEW_H
#define FILEVIEW_H

typedef enum { kNameCol,kDateCol,kSizeCol,kFDColCount } FVColumnEnum;

enum {
	kFVPrefsVersion=1,
	kExandListResType='FvEl',
	kExandListResID=1000,
	kFVPrefsResType='FvPf',
	kFVPrefsResID=1000
};

typedef struct
{
	short		version;
	FVColumnEnum sortCol;
} FVPrefs, **FVPrefsHandle;

typedef struct
{
	ViewListPtr	list;
	short 		savePreviewHi;
	short		vRefNum;
	uLong		dirId;
	ExpandInfo	expandList;
	FVPrefs		prefs;
	Boolean		dirtyPrefs;
	uLong		lastUpdateCheck,lastUpdate;
	ControlRef	controls[kFDColCount];
} FileViewInfo, *FileViewPtr, **FileViewHandle;

void NewFileView(MyWindowPtr win, FileViewHandle hData);
void FVDispose(FileViewHandle fvh);
FileViewHandle GetFileViewInfo(MyWindowPtr win);

void FileViewUpdate(MyWindowPtr win);
void FileViewClick(MyWindowPtr win,EventRecord *event);
void FileViewCursor(Point mouse);
OSErr FileViewDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag);
Boolean FileViewKey(MyWindowPtr win, EventRecord *event);
Boolean FileViewFind(MyWindowPtr win,PStr what);
Boolean FileViewMenu(MyWindowPtr win, int menu, int item, short modifiers);
void FVSizeHeaderButtons(MyWindowPtr win, FileViewHandle fvh);
void FileViewButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part);
void FileViewActivate(MyWindowPtr win);
void FileViewIdle(MyWindowPtr win);
void FVRemoveButtons(MyWindowPtr win);

#endif
