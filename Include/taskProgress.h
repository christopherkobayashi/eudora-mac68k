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

/*
	File: taskProgress.h
	Author: Clarence Wong <cwong@qualcomm.com>
	Date: Augutst 1997 - ...
	Copyright (c) 1997 by QUALCOMM Incorporated 
*/


#ifndef TASK_PROGRESS_H
#define TASK_PROGRESS_H

#include "progress.h"
#include "threading.h"

typedef struct taskErrData_ taskErrData, *taskErrPtr, **taskErrHandle;

struct taskErrData_ {
	TaskKindEnum taskKind;
	long persId;
	Str255 taskDesc, errMess, errExplanation;
	ControlHandle helpButton;
	taskErrHandle next;
};

OSErr AddFilterTask(void);
void RemoveFilterTask(void);
OSErr AddProgressTask(threadDataHandle threadData);
void RemoveProgressTask(threadDataHandle threadData);
OSErr AddTaskErrorsS(StringPtr error, StringPtr explanation,
		     TaskKindEnum taskKind, long persId);
void RemoveTaskErrors(TaskKindEnum taskKind, long persId);
void DrawTaskProgressBar(ControlHandle bar);
void InvalTPRect(Rect * invalRect);
void OpenTasksWin(void);
void OpenTasksWinBehind(void *behind);
void OpenTasksWinErrors(void);
void InitPrbl(ProgressBlock ** prbl, short vert,
	      ControlHandle * stopButton);
void TaskProgressRefresh(void);
#endif
