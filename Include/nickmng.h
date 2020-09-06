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

#ifndef NICKMNG_H
#define NICKMNG_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

// Nickname TOC flags (new for 5.0)
typedef enum {
	nfNone							= 0x00000000,
	nfMultipleAddresses	= 0x00000001,
	nfAllFlags					= 0xFFFFFFFF
} NicknameFlagsType;

typedef enum {
	changeBitUnmodified	= 0x0000,
	changeBitModified		= 0x0001,
	changeBitDeleted		= 0x0002,
	changeBitAdded			= 0x0004,
	changeBitArchived		= 0x0008,
	changeBitPrivate		= 0x0010
} ChangeBitType;

#define	AllButPrivate (~changeBitUnmodified & ~changeBitPrivate)

typedef enum {
	nickFieldReplaceExisting,				// Replace the value if the field is found -- create it if not
	nickFieldAppendExisting,				// Append the value if the field is found -- create it if not
	nickFieldIgnoreExisting,				// Ignore this field if it already exists
} NickFieldSetValueType;

typedef enum {
	noPrimary,
	homePrimary,
	workPrimary
} PrimaryLocationType;

/* Structure for a given nickname */
//	ALB 7/15/96, took out handle to nickname and to NickInfoStruct. Added nameTOCOffset,
//		theViewData, theAddresses, and theNotes
typedef struct
{
	long	hashName; 			/* Hash value on nickname */
	long	hashAddress; 			/* Hash value on address */
//	long	createDate;				/* Creation date for this nickname (eventually) */
//	long	modDate;				/* Modification date for this nickname (eventually) */
//	long	usageDate;				/* Date this nickname was most recently used (eventually) */
	long	cacheDate;				/* Date this nickname was last cached (which differes from the above) */
	long	addressesDirty:1;		/* Have we modified the nickname addresses?  -- eventually!!! */
	long	notesDirty:1;				/* Have we modified the nickname notes?  -- eventually!!!  */
	long	pornography:1;				/* Is the photo dirty?  */
	long	deleted:1;				/* Has the nickname been deleted */
	long	group:1;				/* Does the nickname represent a group */
	long	unused:27;				/* Future expansion */
	long	addressOffset;		/* Offset in file that address is at */
	long	notesOffset;		/* Offset in file that notes is at */
	long	nameTOCOffset;	/* Offset to the nickname */
	long	valueOffset;		// Offset into notes where we'll find the sort value
	long	valueLength;		// Length of the sort value
	Handle	theAddresses; /* expansion addresses */
	Handle	theNotes;			/* Notes for nickname; will contain other info such as real name, phone, etc. */
} NickStruct, *NickStructPtr, **NickStructHandle;

typedef enum {
	eudoraAddressBook,
	regularAddressBook,
	pluginAddressBook,
	historyAddressBook,
#ifdef VCARD
	personalAddressBook
#endif
} AddressBookType;

#define	IsEudoraAddressBook(aShort)			((*Aliases)[aShort].type == eudoraAddressBook)
#define	IsRegularAddressBook(aShort)		((*Aliases)[aShort].type == regularAddressBook)
#define	IsPluginAddressBook(aShort)			((*Aliases)[aShort].type == pluginAddressBook)
#define	IsHistoryAddressBook(aShort)		((*Aliases)[aShort].type == historyAddressBook)
#define	IsPersonalAddressBook(aShort)		((*Aliases)[aShort].type == personalAddressBook)

typedef struct AliasDStruct
{
	FSSpec spec;
	NickStructHandle theData;
	Handle	hNames;				//	ALB 7/16/96, handle to nicknames
	short		**sortData;		// Contains nickname ID's -- 0 based -- of the sorted data for this address book
	AddressBookType	type;
	Boolean collapsed;
	Boolean ro;
	Boolean dirty;
	Boolean	containsBogusNicks;
	Accumulator addressHashes;
} AliasDesc, *AliasDPtr, **AliasDHandle;

// Structure to represent the contents of a 'TGMP' (Tag Map) resource
typedef struct {
	Str255	service;			// Name of the service (Ph, LDAP, etc...)
	Str255	server;				// Server (if any)
	short		count;				// Number of tags
	Handle	serviceTags;		// Concatenation of PStr's representing tags for this service and server
	Handle	nicknameTags;	// Concatenation of PStr's representing mapped nickname tags
} NicknameTagMapRec, *NicknameTagMapRecPtr, **NicknameTagMapRecHandle;

#define NAliases (GetHandleSize_(Aliases)/sizeof(AliasDesc))
#define NNicknames (GetHandleSize_(This.theData)/sizeof(NickStruct))
#define issep(c) (IsSpace(c) || (c)==',')

#define NICK_TOC_TYPE			'NToc'
#define NICK_NAMES_TYPE		'NNam'
#define NICK_BASE_RESID		128
#define NICK_RESID_V2		129
#define NICK_RESID_V3		130
#define	NICK_RESID_V4		131
#define	NICK_RESID_V5		132
#define	NICK_RESID			133	// removed opt-space from hashes, too

#define OLD_NICK_TYPE			'NICK'
#define OLD_NICK_RESID1		3001
#define OLD_NICK_RESID2		3002

#define kNickGenOptAsian 1
#define kNickGenOptLastFirst 2

UPtr AliasExpansion(UPtr data, long offset);
Handle GetTaggedFieldValue (short ab, short nick, PStr tag);
Handle GetTaggedFieldValueInNotes (Handle notes, PStr tag);
PStr GetTaggedFieldValueStr (short ab, short nick, PStr tag, PStr value);
PStr GetTaggedFieldValueStrInNotes (Handle notes, PStr tag, PStr value);
OSErr SetTaggedFieldValue (short ab, short nick, PStr tag, PStr value, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored);
OSErr SetTaggedFieldValueInNotes (Handle notes, PStr tag, Ptr value, long length, NickFieldSetValueType setValue, short separatorIndex, Boolean *ignored);
OSErr SetNicknameChangeBit (Handle notes, ChangeBitType changeBits, Boolean clearFirst);
long GetNicknameChangeBits (Handle notes);
Boolean FindTaggedFieldValueOffsets (short ab, short nick, PStr tag, long *attributeOffset, long *attributeLength, long *valueOffset, long *valueLength);
OSErr RegenerateAllAliases(Boolean rebuild);
OSErr BuildAddressHashes(short which);
Handle	GetNicknameData(short which,short index,Boolean wantAddresses,Boolean readFromDisk);
Handle	GetNicknameName(short which,short index);
PStr		GetNicknameNamePStr(short which,short index,PStr theName);
void	GetNicknameViewData(short which,short index,UPtr sViewData);
long NickHash(UPtr newName);
long NickHashString (PStr string);
long NickHashHandle (Handle h);
long NickHashRawAddresses (Handle addresses,Boolean *group);
long NickGenerateUniqueID(void);
OSErr PrepAllAddressBooksForSync (void);
OSErr PrepAddressBookForSync (short ab);
OSErr PrepNicknameForSync (short ab, short nick, Str255 idTag, Str255 changeBitsTag);
OSErr ClearAllAddressBookChangeBits (long mask);
OSErr ClearAddressBookChangeBits (short ab, long mask);
OSErr ClearNicknameChangeBits (short ab, short nick, long mask);

Boolean IsAnyNickname(PStr name);

void CommaList(Handle h);
#ifdef NEVER
long CountAliasTotal(NickHandle aliases,long offset);
long CountAliasAlias(NickHandle aliases,long offset);
long CountAliasExpansion(NickHandle aliases,long offset);
#endif

#define CountAliasTotal(a,o)	(CountAliasAlias(a,o)+1+CountAliasExpansion(a,o)+2)
#define CountAliasAlias(a,o) 	((unsigned)(*(UHandle)(a))[o])
#define ___nba(a,o) ((o)+CountAliasAlias(a,o)+1)
#define CountAliasExpansion(a,o) (256*(unsigned)(*(UHandle)(a))[___nba(a,o)] + (unsigned)(*(UHandle)(a))[___nba(a,o)+1])

#define	ContainsMultipleAddresses(aHandle)	((aHandle) ? ((GetHandleSize (aHandle) > 2) && *(*(aHandle) + **(aHandle) + 2) ? true : false) : false)

Boolean SaveIndNickFile(short which,Boolean saveChangeBits);
OSErr URLStringToSpec (StringHandle urlString, FSSpec *spec);
short ReplaceNicknameAddresses(short which,UPtr oldName,TextAddrHandle text);
short ReplaceNicknameNotes(short which,UPtr oldName,TextAddrHandle text);
static short ReplaceNicknameInfo(short which,UPtr theName,TextAddrHandle text,Boolean fAddresses);
void RemoveNamedNickname(short which,UPtr name);
short AddNickToTOCfromName(short which,UPtr name,Handle addresses);
short AddNickToTOCfromNotes(short which,UPtr name,Handle notes);
static short AddNickToTOC(short which,UPtr name,Handle hData,Boolean fFromAddress);
long	NickMatchFound(NickStructHandle theNicknames,long hashName,PStr theName,short which);
long	NickAddressMatchFound (NickStructHandle theNicknames, long hashAddress, PStr theAddress, short which);
void MakeMessNick(MyWindowPtr win,short modifiers);
#ifdef VCARD
void MakeCompNick(MyWindowPtr win, FSSpec *vcardSpec);
#else
void MakeCompNick(MyWindowPtr win);
#endif
void MakeMboxNick(MyWindowPtr win,short modifiers);
void MakeCboxNick(MyWindowPtr win);
void FlattenListWith(Handle h,Byte c);
Boolean SaveAliases(Boolean saveChangeBits);
OSErr NickUniq(TextAddrHandle addresses,PStr sep,Boolean wantErrors);
#define MAX_NICKNAME 30
void MakeNickFromSelection(MyWindowPtr win);
short GatherCompAddresses(MyWindowPtr win,Handle biglist);
OSErr AddTextToNick(short which, PStr name, Handle text,Boolean append);
short ChangeNameOfNick(short which,UPtr oldName,UPtr newName);
OSErr GatherBoxAddresses(TOCHandle tocH,short modifiers,short from, short to, UHandle *addresses, Boolean caching);
void ReadNickFileList(FSSpec *pSpec, AddressBookType type, Boolean reread);
void ReadPluginNickFiles(Boolean reread);
OSErr RegenerateAliases(short which, Boolean rebuild);

void ZapAliasHash(short which);
void ZapAliases(void);
void ZapAliasFile(short which);
void ZapPluginAliases(void);
void SetAliasDirty(short which);

Boolean MaybeApplySplittingAlgorithm (Handle notes);
PStr ParseFirstLast (PStr realName, PStr firstName, PStr lastName);
PStr JoinFirstLast (PStr fullName, PStr firstName, PStr lastName);
PStr ScanNameForSpaces (PStr name);
void MakeUniqueNickname (short ab, Str31 nickname);
void SetNickname (short ab, short nick, UPtr name);

OSErr NickBackup(FSSpecPtr spec);

// Prototypes for using the Nickname Tag Map
OSErr GetNicknameTagMap (PStr service, PStr server, NicknameTagMapRecPtr tagMapPtr);
void	DisposeNicknameTagMap (NicknameTagMapRecPtr tagMapPtr);
PStr	NicknameTag2ServiceTag (NicknameTagMapRecPtr tagMapPtr, PStr nicknameTag, PStr serviceTag);
PStr	ServiceTag2NicknameTag (NicknameTagMapRecPtr tagMapPtr, PStr serviceTag, PStr nicknameTag);
PStr	GetIndNicknameTag (NicknameTagMapRecPtr tagMapPtr, short index, PStr nicknameTag);
short FindServiceTagIndex (NicknameTagMapRecPtr tagMapPtr, PStr serviceTag);

PrimaryLocationType GetPrimaryLocation (Handle notes);

short FindAddressBookType (AddressBookType type);
OSErr WhiteListAddr(TextAddrHandle addr);
OSErr WhiteListTS(TOCHandle tocH,short sumNum);

BinAddrHandle UniqBinAddr(BinAddrHandle addresses);
BinAddrHandle SortBinAddr(BinAddrHandle addresses);

#ifdef VCARD
Boolean AnyPersonalNicknames (void);
#endif

#endif