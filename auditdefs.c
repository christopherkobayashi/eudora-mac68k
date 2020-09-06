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

#include <conf.h>
#include <mydefs.h>

extern OSErr Audit(short auditType,...);
extern OSErr AuditFlush(Boolean force);


OSErr AuditShutdown(long faceTime, long rearTime, long connectTime, long totalTime)
{
	OSErr err = Audit(kAuditShutdown,faceTime,rearTime,connectTime,totalTime);
	if (!err) err = AuditFlush(true);
	return err;
}

OSErr AuditTimestamp(long faceTime, long rearTime, long connectTime, long totalTime)
{
	OSErr err = Audit(kAuditTimestamp,faceTime,rearTime,connectTime,totalTime);
	if (!err) err = AuditFlush(false);
	return err;
}

OSErr AuditCheckStart(uLong sessionID, uLong personalityID, Boolean isAuto)
{
	OSErr err = Audit(kAuditCheckStart,sessionID,personalityID,isAuto);
	if (!err) err = AuditFlush(false);
	return err;
}

OSErr AuditCheckDone(uLong sessionID, long messagesRcvd, long bytesRcvd)
{
	return Audit(kAuditCheckDone,sessionID,messagesRcvd,bytesRcvd);
}

OSErr AuditHit(Boolean shift, Boolean control, Boolean option, Boolean command, Boolean alt, uLong windowID, long controlId, long eventType)
{
	return Audit(kAuditHit,shift,control,option,command,alt,windowID,controlId,eventType);
}

OSErr AuditWindowOpen(uLong windowID, uLong windowKind, long wazooState)
{
	return Audit(kAuditWindowOpen,windowID,windowKind,wazooState);
}

OSErr AuditWindowClose(uLong windowID)
{
	return Audit(kAuditWindowClose,windowID);
}

OSErr AuditAdOpen(uLong serverID, uLong adID)
{
	return Audit(kAuditAdOpen,serverID,adID);
}

OSErr AuditAdClose(uLong serverID, uLong adID)
{
	return Audit(kAuditAdClose,serverID,adID);
}

OSErr AuditAdHit(uLong serverID, uLong adID)
{
	return Audit(kAuditAdHit,serverID,adID);
}

OSErr AuditSendStart(uLong sessionID, uLong personalityID, Boolean isAuto)
{
	OSErr err = Audit(kAuditSendStart,sessionID,personalityID,isAuto);
	if (!err) err = AuditFlush(false);
	return err;
}

OSErr AuditSendDone(uLong sessionID, long messagesSent, long bytesSent)
{
	return Audit(kAuditSendDone,sessionID,messagesSent,bytesSent);
}

OSErr AuditPersCreate(uLong personalityID)
{
	return Audit(kAuditPersCreate,personalityID);
}

OSErr AuditPersDelete(uLong personalityID)
{
	return Audit(kAuditPersDelete,personalityID);
}

OSErr AuditStartup(long platform, long version, long buildNumber)
{
	OSErr err = Audit(kAuditStartup,platform,version,buildNumber);
	if (!err) err = AuditFlush(false);
	return err;
}

OSErr AuditPersRename(uLong oldPersID, uLong newPersID)
{
	return Audit(kAuditPersRename,oldPersID,newPersID);
}

OSErr AuditConnect(Boolean connectionUp)
{
	return Audit(kAuditConnect,connectionUp);
}

OSErr AuditNonPersonalSettings(short prefId)
{
	return Audit(kAuditNonPersonalSettings,prefId);
}

OSErr AuditNonPersonalSysInfo(short prefId)
{
	return Audit(kAuditNonPersonalSysInfo,prefId);
}

OSErr AuditDemographicData(short prefId)
{
	return Audit(kAuditDemographicData,prefId);
}
short AuditCategories[] = {4,4,4,4,5,5,5,2,2,2,4,4,5,5,4,5,4,3,3,1};
