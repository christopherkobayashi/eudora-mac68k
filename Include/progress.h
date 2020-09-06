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

#ifndef PROGRESS_H
#define PROGRESS_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * Declarations for progress monitoring
 ************************************************************************/
#ifdef DEBUG
typedef enum {NoBar = -2,NoChange,kpMessage,kpRemaining,kpSubTitle,kpTitle,kpBar,kpBeat} ProgressRectEnum;
#else
typedef enum {NoBar = -2,NoChange,kpMessage,kpRemaining,kpSubTitle,kpTitle,kpBar} ProgressRectEnum;
#endif

int OpenProgress(void);
void Progress(short percent,short remaining,PStr title,PStr subTitle,PStr message);
void ProgressMessage(short which,PStr message);
void ProgressMessageR(short which,short messageId);
void ProgressR(short percent,short remaining,short titleId,short subTitleId,PStr message);
void CloseProgress(void);
void ByteProgress(UPtr message, int onLine, int totLines);
void PushProgress(void);
void PopProgress(Boolean messageOnly);
void SetProgressN(short n);
Boolean ProgressIsOpen(void);
Boolean ProgressIsTop(void);
void PressStop(void);
void ByteProgressExcess(int excess);
int GetProgressBytes(void);

typedef struct ProgressBlock ProgressBlock, *ProgressBPtr, **ProgressBHandle;
struct ProgressBlock {
	short percent;
	int on, total,
			excessOn;	// amount we under-estimated
#ifdef DEBUG
	Rect rects[kpBeat+1];
#else
	Rect rects[kpBar+1];
#endif
	Str255 messages[kpTitle+1];
	ProgressBHandle next;
	Str255 title;
	ControlHandle bar;
	ControlHandle stop;
};
void ProgressUpdate(MyWindowPtr win);
void DisposProgress(ProgressBHandle prbl);
#define PROG_BAR_HI 12
#define PROG_BOX_HI 16
#endif