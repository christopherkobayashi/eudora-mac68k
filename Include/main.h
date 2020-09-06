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

#ifndef MAIN_H
#define MAIN_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
void main(void);
Boolean DoDependentMenu(WindowPtr,int,int,short);
#define DieWithError(cmbid,num) DWE(cmbid,num,FNAME_STRN+FILE_NUM,__LINE__)
void DWE(int,int,int,int);
#define WarnUser(cmbid,num) WU(kAlertCautionAlert,cmbid,num,FNAME_STRN+FILE_NUM,__LINE__)
#define NoteUser(cmbid,num) WU(kAlertNoteAlert,cmbid,num,FNAME_STRN+FILE_NUM,__LINE__)
long MonitorGrow(Boolean report);
void FlushHIQ(void);
int WU(AlertType,int,int,int,int);
void DumpData(UPtr description, UPtr data,int length);
pascal long GrowZone(unsigned long needed);
void MakeGrow(long howMuch,short which);
void Trace(UPtr message,...);
#define FileSystemError(ctext,name,err)\
	FSE(ctext,name,err,FNAME_STRN+FILE_NUM,__LINE__)
int FSE(int context, UPtr name, int err, int file,int line);
Boolean WNELo(short eventMask,EventRecord *event,long sleep);
Boolean MiniMainLoop(EventRecord *event);
Boolean HandleControl(Point pt, MyWindowPtr win);
void TransferMenuHelp(short id);
uLong CurrentSize(void);
uLong DefaultSize(void);
uLong EstimatePartitionSize(Boolean log);
void MemoryWarning(void);
pascal void Hook(void);
void *NuHandle(long size);
void *NuPtr(long size);
void *NuPtrClear(long size);
void RefreshLabelsMenu(void);
Boolean HandleEvent(EventRecord *event);
Boolean DoApp4(WindowPtr topWinWP,EventRecord *event);
void FccFlop(MyWindowPtr win);
void CheckSLIP(void);
void MightSwitch(void);
void AfterSwitch(void);
Boolean WinBGDrag(MyWindowPtr win,EventRecord *event);
Boolean HasCommandPeriod(void);
Boolean DoApp1NoPTE(MyWindowPtr topWin,EventRecord *event);
pascal long MyGrowZone(unsigned long needed);
void TendNotificationManager(Boolean isActive);
#ifdef DEBUG
void ValidateResourceMaps(void);
void ValidateResourceMap(UHandle map);
void CheckHandle(UHandle map,long offset,long size,THz hz,PStr string);
#endif
/*
	NeedYield is now a function:
	-- use str# resource to allow on-the-fly changing of yield intervals 
	-- make threads more cooperative by using ThreadYieldTicks instead of just YieldTicks
 */
Boolean NeedYield(void);
#define NEED_YIELD NeedYield()

pascal OSErr PantyTrack(DragTrackingMessage message, WindowPtr qWinWP, Ptr handlerRfCon, DragReference drag);
pascal OSErr PantyReceive(WindowPtr qWin, Ptr handlerRfCon, DragReference drag);

#endif