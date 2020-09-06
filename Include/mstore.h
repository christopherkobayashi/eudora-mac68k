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

#ifndef MIMESTORE_H
#define MIMESTORE_H

/************************************************************************
 * types
 ************************************************************************/
#define STRUCTURE(x) typedef struct x x, *x##Ptr, **x##Handle
STRUCTURE(MStoreMailDBHeader);
STRUCTURE(MStoreTOCFileHeader);
STRUCTURE(EudoraMessInstanceID);
STRUCTURE(I4UIDPlus);
STRUCTURE(MStoreSummary);
STRUCTURE(MStoreIDDBHeader);
STRUCTURE(MStoreIDDBBlock);
STRUCTURE(MStoreInfo);
STRUCTURE(MStoreMemIDDB);
STRUCTURE(MStoreBox);
STRUCTURE(MStoreBlockHeader);
STRUCTURE(MStoreVersion);
STRUCTURE(MStoreSubFile);

/************************************************************************
 * On-Disk Constants and Structures
 ************************************************************************/

/************************************************************************
 * Version Number
 ************************************************************************/
#define kMStoreMajorVersion 1
#define kMStoreMinorVersion 0

/************************************************************************
 * a 47-char string
 ************************************************************************/
typedef unsigned char Str47[48];

/************************************************************************
 * File types 
 ************************************************************************/
typedef enum
{
	kMStoreMailDBMagic = 'MlDb',
	kMStoreMailDBTerm = 'bDlM',
	kMStoreMailFileType = 'EuMB',
	kMStoreIDDBFileType = 'EuID',
	kMStoreTOCFileType = 'EuTC',
	kMStoreInfoFileType = 'EuIf',
	kMStoreDummyFileType = 0
} MStoreFileTypeEnum;

/************************************************************************
 * Version number
 ************************************************************************/
struct MStoreVersion
{
	short majorVersion;
	short minorVersion;
};

/************************************************************************
 * MailDB header
 ************************************************************************/
struct MStoreMailDBHeader
{
	unsigned long magic;				// magic number for MailDB file
	MStoreVersion highVersion;	// highest version # of file ever written
	MStoreVersion lastVersion;	// last version # of file written
	unsigned long modSeconds;		// mod seconds of last time written
};

/************************************************************************
 * MStoreBlockHeader - block header in MailDB file
 ************************************************************************/
struct MStoreBlockHeader
{
	long magic;
	long type;
	long id;
	long size;
	long refCount;
	long attrSize;
};

/************************************************************************
 * Types for blocks
 ************************************************************************/
typedef enum
{
	kMStoreFreeType = 'Free',					// Free block
	kMStoreMessageType = 'Mess',			// Message descriptor
	kMStoreContHeaderType = 'Hdrs',		// Headers of a content
	kMStoreContBodyType = 'Body',			// Body of a content
	kMStoreContentType = 'Cont',			// Headers and body of content in one piece
	kMStoreStubType = 'Stub',					// POP3 message stub
	kMStoreI4EnvelopeType = 'I4Ev',		// IMAP4 message envelope
	kMStoreI4Structuretype = 'I4St',	// IMAP4 bodystructure
	kMStoreTerminator	= 'tErM',				// block terminator; not really a block type...
	kMStoreDummyType = 0
} MStoreBlockTypeEnum;

/************************************************************************
 * Types for attributes
 ************************************************************************/
typedef enum
{
	// ID's
	kMStorePOP3IIDAttr = 'POP3',				// POP3 message instance id
	kMStoreMessageIIDAttr = 'MIID',			// mailstore message instance id
	kMStoreMessageIDAttr = 'MsID',			// Hash of message-id field
	kMStoreI4MailMessIIDAttr = 'I4MM',	// IMAP4 Mailbox-message instance id
	kMStoreI4MessIIDAttr = 'I4ID',			// IMAP4 Message instance id
	kMStoreI4MessINameAttr = 'I4MN',		// IMAP4 Message instance name
	kMStoreI4ContINameAttr = 'I4CN',		// IMAP4 Content instance name
	// MIME Stuff
	kMStoreMIMECTAttr	= 'MICT',					// Content-Type parameter
	kMStoreMIMESTAttr	= 'MIST',					// Content-Subtype parameter
	kMStoreMIMECTParamAttr = 'MICP',		// Content-type parameter (name=value)
	kMStoreMIMECDispAttr = 'MICD',			// Content-Disposition
	kMStoreMIMECDispParamAttr = 'MIDP',	// Content-Disposition parameter (name=value)
	// Internal DB stuff
	kMStoreReferencedAttr = 'Refr',			// Block that references this one
	kMStoreFlagsAttr = 'Flag',					// Flags
	// Misc
	kMStoreComputedHashAttr = 'CHsh',		// Hash algorithm & value computed by us
	kMStoreContentSizeAttr = 'CSiz',		// Size of content
	kMStoreDummyAttr = 0
} MStoreAttributeEnum;
#define kMStoreContentID kMStoreMessageID	// same as message-id, only content-id



/************************************************************************
 * Content flags
 ************************************************************************/
typedef enum
{
	kMStoreFlagOriginal,		// content has not been transformed in any way
	kMStoreFlagNotUTF8,			// content is not in UTF-8
	kMStoreFlagMacintosh,		// message came from Mac Eudora <4.0
	kMStoreFlagWindows,		// message came from Win Eudora <4.0
	kMStoreFlagNotMIME,		// message is non-MIME
	kMStoreFlagStub,			// partially downloaded
	kMStoreFlagErrors,		// content had errors during download
	kMStoreFlagWipe,			// content should be wiped after being freed
	kMStoreFlagDummy
} MStoreTextFlagsEnum;

/************************************************************************
 * Morsels
 ************************************************************************/
typedef enum
{
	kMStorePartMorsel = 'Part',	// alternate forms of a single content
	kMStoreMultiMorsel = 'Mult',	// a multipart content
	kMStoreDummyMorsel = 0
} MStoreMorselEnum;

/************************************************************************
 * TOC File Header
 ************************************************************************/
struct MStoreTOCFileHeader
{
	MStoreVersion highVersion;	// highest version # of file ever written
	MStoreVersion lastVersion;	// last version # of file written
	unsigned long modSeconds;		// mod seconds of last time written
	unsigned long sumSize;			// size of summaries
	unsigned long sumCount;			// # of summaries
};

/************************************************************************
 * Message instance id
 ************************************************************************/
struct EudoraMessInstanceID
{
	unsigned long mailboxID;
	unsigned long messageID;
};

/************************************************************************
 * I4UIDPlus - IMAP4 uid+; uidvalidity+uid
 ************************************************************************/
struct I4UIDPlus
{
	unsigned long i4UIDValidity;
	unsigned long i4UID;
};

/************************************************************************
 * Message Summary
 ************************************************************************/
struct MStoreSummary
{
	EudoraMessInstanceID mIID;	// Where is it stored?
	uLong mid;									// Hash of message-id header
	uLong size;									// Content size, for user's benefit
	uLong seconds;							// Message date, in seconds since 12am 1 Jan 1970 GMT
	short zone;									// Message original timezone, in seconds
	short pad;									// unused
	Byte state;									// unread, read, etc
	Byte priority;							// current user-assigned priority
	Byte origPriority;					// priority message came in with
	Byte label;									// user-assigned message label
	uLong flags[2];							// message flags; see below
	Rect position;							// position of window
	uLong personality;					// personality id
	I4UIDPlus i4UIDPlus;				// IMAP4 UID+; interesting only if in synced mailbox
	Str31 from;									// Human string for sender of message
	Str47 subject;							// Human string for subject of message
};

/************************************************************************
 * summary flags
 *   number is bit count from lowest bit of mFlags[1] up to high bit of mFlags[1]
 ************************************************************************/
typedef enum
{
	kSumFlagMDN,						// user has requested an MDN for this message (outgoing)
	kSumFlagDSN,						// user has requested a DSN for this message (outgoing)
	kSumFlagPartial,				// only part of the message has been downloaded
	kSumFlagOnServer,				// this message still exists on a server
	kSumFlagEncodingErrors,	// encoding errors were found while transferring
	kSumFlagOutgoing,				// this is an outgoing message
	kSumFlagZoomed,					// window was in zoomed state when closed
	kSumFlagReport,					// message is an MDN or DSN report
	kSumFlagWipe,						// bits should be wiped when message moved or deleted
	kSumFlagEdited,					// user has changed this message
	kSumFlagWeirdReply,			// message has reply-to different from from:
	kSumFlagFormatBar,			// message has format bar (outgoing)
	kSumFlagWordWrap,				// word wrap when sending (outgoing)
	kSumFlagKeepCopy,				// keep a copy after sending (outgoing)
	kSumFlagLocked,					// user has locked the message in this mailbox
	kSumFlagSpooledAttachments,	// message has spool folder associated with it
	kSumFlagBodyAllowEncoding,	// user says ok to encode this message body
	kSumFlagBodyForceEncoding,	// user says body of message must be encoded
	kSumFlagBodyWordWrap,				// user says word wrap body of message
	kSumFlagTextAllowEncoding,	// user says ok to encode text attachments
	kSumFlagTextForceEncoding,	// user says text attachments must be encoded
	kSumFlagTextWordWrap,				// user says word wrap text attachments
	kSumFlagAttach1,						// low bit of attachment type
	kSumFlagAttach2,						// middle bit of attachment type
	kSumFlagAttach3,						// high bit of attachment type
	kSumFlagResourceFork,				// include resource fork with attachments
	kSumDummy
} MStoreSummaryFlagEnum;

/************************************************************************
 * IDDB File header
 ************************************************************************/
struct MStoreIDDBHeader
{
	OSType iddbMagic;						// magic number
	MStoreVersion highVersion;	// highest version # of file ever written
	MStoreVersion lastVersion;	// last version # of file written
	unsigned long modSeconds;		// mod seconds of last time written
};

/************************************************************************
 * IDDB Blocks
 ************************************************************************/
struct MStoreIDDBBlock
{
	unsigned long blockID;
	unsigned long offset;
	unsigned long size;
};

/************************************************************************
 * Info file
 ************************************************************************/
struct MStoreInfo
{
	Str255 name;
};

/************************************************************************
 * In-Memory Constants and Structures
 ************************************************************************/

/************************************************************************
 * IDDB in-memory version
 ************************************************************************/
struct MStoreMemIDDB
{
	MStoreIDDBBlock block;
	long dirty:1;
	long locked:1;
};

/************************************************************************
 * SubFile - manage the aspects of a subfile of a mailbox
 ************************************************************************/
typedef enum {msfcCreate,msfcOpen,msfcFlush,msfcClose,msfcCloseHard,msfcDestroy} MSSubCallEnum;
struct MStoreSubFile
{
	short refN;
	short fileNameID;
	OSType fileType;
	OSErr (*func)(MSSubCallEnum selector,MStoreBoxHandle boxH);
	long dirty:1;
};


/************************************************************************
 * Mailbox
 ************************************************************************/
struct MStoreBox
{
	short type;					// In/Out/Trash or regular
	short refCount;			// number of references
	short locked;				// somebody is using it
	FSSpec spec;				// Spec (could be alias) of the folder
	VDId dir;						// Volume and directory containing the mailbox
	MStoreSubFile subs[4];		// subfile info
	StackHandle iddb;		// The IDDB for this mailbox
	StackHandle summaries;	// message summaries
	MStoreInfo info;		// The info file
	MStoreMailDBHeader mailDBHeader;	// Header for maildb file
	uLong mailDBModTime;
};

typedef enum {mssfMailDB,mssfIDDB,mssfTOC,mssfInfo,mssfLimit} MSSubFileEnum;  // indices into subfile array

/************************************************************************
 * Functions
 ************************************************************************/
OSErr MSCreateMailbox(FSSpecPtr spec,MStoreBoxHandle *boxH);
OSErr MSOpenMailbox(FSSpecPtr spec,MStoreBoxHandle *boxH);
OSErr MSFlushMailbox(MStoreBoxHandle boxH);
OSErr MSCloseMailbox(MStoreBoxHandle boxH,Boolean force);

// utilities for other mime store stuff
OSErr MSCreateSubFile(MStoreBoxHandle boxH,MSSubFileEnum file);
OSErr MSOpenSubFile(MStoreBoxHandle boxH,MSSubFileEnum file);
OSErr MSCloseSubFile(MStoreBoxHandle boxH,MSSubFileEnum file);
OSErr MSDestroySubFile(MStoreBoxHandle boxH,MSSubFileEnum file);

#define JAN11970 2082844800
#define MStoreDateTime()	(GMTDateTime()-JAN11970)

// version utilities
#define MStoreCurrentVersion(v) do{(v)->majorVersion = kMStoreMajorVersion; (v)->minorVersion = kMStoreMinorVersion;}while(0)
#define MStoreCompatibleVersion(v) ((v)->majorVersion <= kMStoreMajorVersion)
#define MStoreOlderVersion(v)	((v)->majorVersion<kMStoreMajorVersion || (v)->majorVersion==kMStoreMajorVersion && (v)->minorVersion<kMStoreMinorVersion)

#ifdef DEBUG
void DebugMimeStore(void);
#endif
#endif
