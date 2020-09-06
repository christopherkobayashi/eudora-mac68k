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

#ifndef AUDITDEFS_H
#define AUDITDEFS_H

#define kAuditShutdown 1 //
#define kAuditTimestamp 2 //
#define kAuditCheckStart 3 //
#define kAuditCheckDone 4 //
#define kAuditHit 5 //
#define kAuditWindowOpen 6 //
#define kAuditWindowClose 7 //
#define kAuditAdOpen 8 //
#define kAuditAdClose 9 //
#define kAuditAdHit 10 //
#define kAuditSendStart 11 //
#define kAuditSendDone 12 //
#define kAuditPersCreate 13 //
#define kAuditPersDelete 14 //
#define kAuditStartup 15 //
#define kAuditPersRename 16 //
#define kAuditConnect 17 //
#define kAuditNonPersonalSettings 18 //
#define kAuditNonPersonalSysInfo 19 //
#define kAuditDemographicData 20 //
OSErr AuditShutdown(long faceTime, long rearTime, long connectTime, long totalTime);
OSErr AuditTimestamp(long faceTime, long rearTime, long connectTime, long totalTime);
OSErr AuditCheckStart(uLong sessionID, uLong personalityID, Boolean isAuto);
OSErr AuditCheckDone(uLong sessionID, long messagesRcvd, long bytesRcvd);
OSErr AuditHit(Boolean shift, Boolean control, Boolean option, Boolean command, Boolean alt, uLong windowID, long controlId, long eventType);
OSErr AuditWindowOpen(uLong windowID, uLong windowKind, long wazooState);
OSErr AuditWindowClose(uLong windowID);
OSErr AuditAdOpen(uLong serverID, uLong adID);
OSErr AuditAdClose(uLong serverID, uLong adID);
OSErr AuditAdHit(uLong serverID, uLong adID);
OSErr AuditSendStart(uLong sessionID, uLong personalityID, Boolean isAuto);
OSErr AuditSendDone(uLong sessionID, long messagesSent, long bytesSent);
OSErr AuditPersCreate(uLong personalityID);
OSErr AuditPersDelete(uLong personalityID);
OSErr AuditStartup(long platform, long version, long buildNumber);
OSErr AuditPersRename(uLong oldPersID, uLong newPersID);
OSErr AuditConnect(Boolean connectionUp);
OSErr AuditNonPersonalSettings(short prefId);
OSErr AuditNonPersonalSysInfo(short prefId);
OSErr AuditDemographicData(short prefId);

extern short AuditCategories[];
#define MaxAuditType 20

#endif //AUDITDEFS_H
