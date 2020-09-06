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

/* Copyright (c) 2000 by the University of Illinois Board of Trustees */

/**********************************************************************
 * import.h
 *
 *	Routines to import data from other email applications.
 **********************************************************************/
 
#ifndef IMPORT_H
#define IMPORT_H

// for importer related errors ...
#define kUnspecifiedImportPluginId -1

// defined in emsapi-macGlue.h to pass information about an email account
typedef struct ImportAccountInfoS ImportAccountInfoS, *ImportAccountInfoP, **ImportAccountInfoH;

// import messages and mail
void DoMailImport(void);

// Display an importer related error string
OSErr ImportError(long id, short importOperation, short importError, OSErr err, short line);

// Do the right thing to some HTML
OSErr TurnIntoEudoraHTML(Handle *t);	// in newhtml.c


// Callback routines for EMSAPI Importer Plugins

// make a new signature
short ImportMakeNewSigCalback(Str255 name, Handle sigText);

// make an address book file
short ImportMakeAddressBookCallback(Str255 name);

// make an address book entry
OSErr ImportMakeABEntryCallback(short which, Boolean isGroup, UPtr nickName, Handle addresses, Handle notes);

// make a mailbox
OSErr ImportMakeMailboxCallback(ImportMailboxOperationEnum command, FSSpecPtr boxSpec, Boolean isFolder, Boolean noSelect);

// make an outgoing message in a malbox
OSErr ImportMakeOutMessCallback(emsMakeOutMessDataP o);

// read a MIME encoded message from a file
OSErr ImportMessageCallback(short ref, long offset, long len, short state, Handle attachments, FSSpecPtr boxSpec);

// Import a MBOX file
OSErr ImportMboxCallback( FSSpecPtr sourceMBox, FSSpecPtr dest );
#endif	//IMPORT_H