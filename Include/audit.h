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

#ifndef AUDIT_H
#define AUDIT_H

#define AuditFmtStrn 19600
#define kMyProductCode	32
#define kMyPlatformCode	0
#define kFaceIntervalBefore 0	// wrong, but others are dumb
#define kFaceIntervalAfter 60	// debatable
#define kFaceInterval (kFaceIntervalBefore+kFaceIntervalAfter)

#define AUDITCONTROLID(w,c) (((long)(w)<<16)+c)

// Face Time measurement API
typedef struct FaceMeasureBlock *FMBPtr, **FMBHandle;

FMBHandle NewFaceMeasure(void);	// Creates empty FMB
OSErr FaceMeasureBegin(FMBHandle fmb);	// Start watching face time in this block
OSErr FaceMeasureEnd(FMBHandle fmb);	// Stop watching face time in this block
OSErr FaceMeasureReset(FMBHandle fmb);	// Reset face time in this block
OSErr DisposeFaceMeasure(FMBHandle fmb);	// Destroy an FMB
#define ZapFaceMeasure(fmb)	do{DisposeFaceMeasure(fmb);(fmb)=nil;}while(0)
// See what's in the block.  Reports times in seconds.
OSErr FaceMeasureReport(FMBHandle fmb, uLong * faceTime, uLong * rearTime,
			uLong * connectTime, uLong * totalTime);

// Informational stuff
Boolean HaveFace(void);		// user is using Eudora
Boolean SurfsUp(void);		// the internet is connected

// Event Filter that makes it all go
Boolean FMBEventFilter(EventRecord * event, Boolean oldResult);

// Called at startup to determine if we should ask user to send audit
void RandomSendAudit(void);
void AskSendAudit(void);

#endif				//AUDIT_H
