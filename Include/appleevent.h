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

#ifndef APPLEEVENT_H
#define APPLEEVENT_H

/************************************************************************
 * AE constants
 ************************************************************************/

#define cEuMailfolder			'euMF'	/* folder for mailboxes and mail folders */
#define pEuTopLevel				'euTL'	/* is top-level of Eudora Folder? */
#define pEuFSS						'euFS'	/* FSS for file */

#define cEuMailbox				'euMB'	/* mailbox */
#define pEuMailboxType		'euMT'	/* in, out, trash, ... */
#define pEuMailboxUnread	'euMU'	/* # unread messages */
#define pEuMailboxRecentUnread	'euRU'	/* # recent unread messages */
#define pEuWasteSpace			'euWS'	/* space wasted in mailbox */
#define pEuNeededSpace		'euNS'	/* space needed by messages in mailbox */
#define pEuTOCFSS					'eTFS'	/* FSS for toc file (pEuFSS is for mailbox) */

#define cEuNickFile				'eNfl'	// nickname file
#define cEuNickname				'eNck'	// nickname
#define pEuNickNick				'eNck'	// nickname itself
#define pEuNickAddrOld				'eNAd'	// addresses from nickname
#define pEuNickAddrNew				'euNa'	// addresses from nickname
#define pEuNickNotes			'eNNo'	// notes from nicknames
#define pEuNickRawNotes		'eNRN'	// raw notes from nicknames
#define pEuIsRecip				'eNRp'	// is nickname on recipient list
#define pEuNickExpansion	'eNEx'	// the nickname's expansion, in its full glory

#define cEuNotify					'eNot'	/* applications to notify */
																	/* pEuFSS is the fsspec */

#define cEuMessage				'euMS'	/* message */
#define pEuPriority				'euPY'	/* priority */
#define pEuStatus					'euST'	/* message status */
#define pEuSender					'euSe'	/* sender */
#define pEuDate						'euDa'	/* date */
#define pEuGMTSecs				'euUT'	/* date */
#define pEuLocalSecs			'euLT'	/* date */
#define pEuSize						'euSi'	/* size */
#define pEuSubject				'euSu'	/* subject */
#define pEuOutgoing				'euOu'	/* outgoing? */
#define pEuSignature			'eSig'	/* signature? */
#define pEuWrap						'eWrp'	/* wrap? */
#define pEuFakeTabs				'eTab'	/* fake tabs? */
#define pEuKeepCopy				'eCpy'	/* keep copy? */
#define pEuHqxText				'eXTX'	/* HQX -> TEXT? */
#define pEuMayQP					'eMQP'	/* may use quoted-printable? */
#define pEuAttachType			'eATy'	/* attachment type; 0 double, 1 single, 2 hqx, 3 uuencode */
#define pEuShowAll				'eBla'	/* show all headers */
#define pEuWholeText			'eHlt'	/* all the text of the message */
#define pEuTableId				'eTbl'	/* resource id of table */
#define pEuBody						'eBod'	/* resource id of table */
#define pEuSelectedText		'eStx'	/* the text selected now */
#define pEuOnServer				'eOsv'	/* is on the server, so far as Eudora knows */
#define pEuWillFetch			'eWFh'	/* is on list to fetch next time */
#define pEuWillDelete			'eWDl'	/* is on list to delete next time */
#define pEuAttDel					'eADl'	/* should delete attachments after sending */
#define pEuReturnReceipt	'eRRR'	/* return receipt requested */
#define pEuLabel					'eLbl'	/* label index */
#define pEuPencil					'ePcl'	/* pencil-edit */

#define cEuField					'euFd'	/* field in message */

#define cEuAttachment			'eAtc'	/* attachement in message */

#define cEu822Address			'e822'	/* RFC 822 address */

#define cEuTEInWin				'EuWT'	/* the teh of a window */
#define cEuWTEText				'eWTT'	/* text from the teh of a window */

#define cEuPreference			'ePrf'	/* a preference string */

#define kEudoraSuite			'CSOm'	/* Eudora suite */
#define keyEuNotify				'eNot'	/* Notify of new mail */
#define kEuNotify					keyEuNotify
#define kEuCompact				'eCpt'	/* Compact mailboxes */
#define kEuInstallNotify	'nIns'	/* install a notification */
#define kEuRemoveNotify		'nRem'	/* remove a notification */
#define keyEuWhatHappened	'eWHp'	/* what happened */
#define keyEuMessList			'eMLs'	/* Message list */
#define keyEuMailboxList		'eBLs'	/* Mailbox list */
#define eMailArrive				'wArv'	/* mail has arrived */
#define eMailSent					'wSnt'	/* mail has been sent */
#define eWillConnect			'wWCn'	/* will connect */
#define eHasConnected			'wHCn'	/* has connected */
#define eManFilter				'mFil'	/* manual filter */
#define eFilterWin				'wFil'	/* filter window */

#ifdef NEVER
#define eMailArrive				1L	/* mail has arrived */
#define eMailSent					2L	/* mail has been sent */
#define eWillConnect			3L	/* will connect */
#define eHasConnected			4L	/* has connected */
#endif

#define kEuReply					'eRep'	/* Reply */
#define keyEuToWhom				'eRWh'	/* Reply to anyone in particular? */
#define keyEuReplyAll			'eRAl'	/* Reply to all? */
#define keyEuIncludeSelf	'eSlf'	/* Include self? */
#define keyEuQuoteText		'eQTx'	/* Quote original message text? */
#define keyEuStationeryName		'eSty'	/* Name of stationery to use */

#define kEuForward				'eFwd'	/* Forward */

#define kEuRedirect				'eRdr'	/* Redirect */

#define kEuSalvage				'eSav'	/* Salvage a message */

#define kEuAttach					'eAtc'	/* Attach a document */
#define keyEuDocumentList	'eDcl'	/* List of dox to attach */

#define kEuQueue					'eQue'	/* Queue a message */
#define keyEuWhen					'eWhn'	/* When to send message */

#define kEuUnQueue				'eUnQ'	/* Unqueue a message */

#define kEuConnect				'eCon'	/* Connect (send/queue) */
#define keyEuSend					'eSen'
#define keyEuCheck				'eChk'
#define keyEuOnIdle				'eIdl'	/* wait until Eudora is idle? */

#define kEuNewAttach			'euAD'	/* attach document, new style */
#define keyEuToWhat				'euMS'	/* attach to what message? */
#define keyEuSpool				'eSpo'	/* spool the attachments */

#define typeVDId					'VDId'	/* vref & dirid */

#define cEuFilter					'euFi'	/* eudora filters */
#define pEuConjunction		'euFC'	/* conjunction for Eudora filter */
#define pEuManual					'Onan'	/* manual filter */
#define eEuConjunction		pEuConjunction	/* enum for conjunction */

#define cEuFilterTerm			'euFT'	/* term of a Eudora filter */
#define pEuFilterUse			'eFUS'	/* filter use seconds */
#define pEuFilterHeader		'euFH'	/* header of the filter */
#define pEuFilterValue		'euFV'	/* value of the filter */
#define pEuFilterVerb			'eVRB'	/* verb of the filter */
#define eEuFilterVerb			pEuFilterVerb	/* enum for same */

#define cEuFilterAction		'euFA'	/* action for a filter */
#define pEuCopy						'eFCP'	/* copy instead of transfer */

#define cEuPersonality		'euPr'	// a personality

#define kURLSuite					'GURL'
#define kURLClass					'GURL'
#define kURLFetch					'FURL'	/* fetch URL and return in reply */
#define kURLGet						'GURL'	/* get URL and display */
#define kURLDocumentList	'Atch'	// list of documents to attach

#define cEuHelper					'euHp'	// URL helper

#define keyEuInvisible		'eInv'	/* make new message invisible */

#define keyEuEditRegion		'euER'	/* edit region a doc was double-clicked from; eudora private use only */

#define kEuPalmConduitAction	'ePCA'	/* handle actions for the palm conduits  */
#define cEuPalmConduitCommand	'ePCC'	/* perform some function for the conduit */
#define cEuPalmConduitData		'ePCD'	/* use this parameter for that action */

#ifndef rez

#define kIn								IN
#define	kOut							OUT
#define kTrash						TRASH
#define KRegular					0

typedef struct MyWindowStruct *MyWindowPtr;

#define keyEuIgnoreDesc	'0Dsc'
#define keyEuDesc				'DesC'

/*
 * standard hander types
 */
typedef	pascal OSErr AEHandler(AppleEvent *event,AppleEvent *reply,long refCon);
typedef pascal OSErr ObjectAccessor(DescType desiredClass, AEDescPtr containerToken, DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,long refCon);
typedef pascal OSErr AECounter(DescType desiredType, DescType containerClass, const AEDesc *container, long *result);
typedef OSErr ElementCounter(DescType countClass,AEDescPtr countIn,long *count);
typedef OSErr TokenTaker(AEDescPtr token,AppleEvent *reply,long refCon);

void InstallAE(void);
void InstallAERest(void);
OSErr OpenOtherDoc(FSSpecPtr spec,Boolean finderSelect,Boolean printIt,PETEHandle sourcePTE);
OSErr LaunchAppWith(FSSpecPtr app, FSSpecPtr doc, Boolean printIt);
OSErr QuitApp(AEAddressDescPtr aead);
void HandleWordServices(MyWindowPtr win, short item);
typedef struct TOCType **TOCHandle;
void NotifyHelpers(short newCount, OSType ofWhat, TOCHandle tocH);
typedef enum {kTOLNot, kTOLSettings, kTOLOther} TypeIsOnListEnum;
TypeIsOnListEnum TypeIsOnListWhereAndIndex(OSType type,OSType listType,short *where,short *index);
#define TypeIsOnList(x,y) TypeIsOnListWhere(x,y,nil)
#define TypeIsOnListWhere(x,y,z) TypeIsOnListWhereAndIndex(x,y,z,nil)

OSErr CreatorToApp(OSType creator,AliasHandle *app);
OSErr OpenDocWith(FSSpecPtr doc,FSSpecPtr app,Boolean printIt);
OSErr CreateTEHObj(MyWindowPtr win, AEDescPtr objAD);

Boolean CanOpen(OSType type,short vRef,long dirId,PStr name);
OSErr OpenOneDoc(PETEHandle pte,FSSpecPtr spec,FInfo *info);

OSErr FinderOpen(FSSpecPtr spec,Boolean finderSelect,Boolean printIt);

/*
 * Frontier macro
 */
#ifdef TWO
#define FRONTIER	do{if (SharedScriptCancelled(event,reply)) return(noErr);}while(0)
#else
#define FRONTIER
#endif

/*
 * token types
 */
typedef struct {DescType propertyId; DescType tokenClass;} PropToken, *PropTokenPtr, **PropTokenHandle;
#ifdef TWO
typedef struct {DescType form; long selector;} FilterToken, *FilterTokenPtr, **FilterTokenHandle;
typedef struct {FilterToken filter; short term;} TermToken, *TermTokenPtr, **TermTokenHandle;
#endif
typedef struct {short vRef; long dirId;} VDId, *VDIdPtr, **VDIdHandle;
typedef struct {FSSpec spec; short index;} MessToken, *MessTokenPtr, **MessTokenHandle;
typedef struct {Str31 name; MessToken messT; NickToken nickT; Boolean isNick;} FieldToken, *FieldTokenPtr, **FieldTokenHandle;
typedef struct {MyWindowPtr win; PETEHandle pte;} TEToken, *TETokenPtr, **TETokenHandle;
typedef struct {short start; short stop; TEToken teT;} TERangeToken, *TERTPtr, **TERTHandle;
typedef struct Personality Personality, *PersPtr, **PersHandle;
typedef struct {short pref; PersHandle personality;} PrefToken, *PrefTPtr, **PrefTHandle;
typedef struct {MessToken messT; short index;} AttachmentToken, *AttachmentTPtr, **AttachmentTHandle;
#endif

#endif
