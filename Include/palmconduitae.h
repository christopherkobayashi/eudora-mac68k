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

/* Copyright (c) 2000 by QUALCOMM Incorporated */

#ifndef PALMCONDUITAE_H
#define PALMCONDUITAE_H

// Palm Conduit Commands
enum
{
	kPCCGetSettings = 0x00000001,		// return the settings file
	kPCCCheckAddressBooks,				// close address books in Eudora
	kPCCBackupChangedAddressBooks,		// backup modified address books
	kPCCReloadAddressBooks,				// reopen the address books in Eudora
	kPCCGenerateUniqueID,				// generate an id for a new nickname
	kPCCCommandLimit
};

// return the active settings file
OSErr ABConduitGetSettingsFile(AppleEvent *reply, uLong refCon);

// ABConduitCanSyncAddressBooks - can we synchronize the address books?
OSErr ABConduitCanSyncAddressBooks(AppleEvent *reply, void *data, long dataSize);

// ABConduitBackupAddressBooks - backup the changed address book files
OSErr ABConduitBackupAddressBooks(AppleEvent *reply, FSSpecPtr cbSpecs, long ptrSize);

// reopen all the nickname files
OSErr ABConduitReopenAddressBooks(AppleEvent *reply);

// generate an id for a new nickname
OSErr ABconduitGenerateUniqueID(AppleEvent *reply, void *data, long dataSize);

#endif	//PALMCONDUITAE_H