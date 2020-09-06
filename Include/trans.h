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

/* Copyright (c) 1995 by QUALCOMM Incorporated */
#ifndef TRANS_H
#define TRANS_H

#include <emsapi-mac.h>

#include <Drag.h>

/*
 * Translator description structure
 */
typedef struct TLMaster {
	Handle nameHandle;
	short module;
	short id;
	long flags;
	long type;
	short menuItem;
	OSErr result;
	Handle suite;
} TLMaster, *TLMPtr, **TLMHandle;

typedef emsMIMEtype **emsMIMEHandle;
typedef emsMIMEparam **emsMIMEParamHandle;
typedef UHandle FlatTLMIMEHandle;
typedef struct Accumulator *AccuPtr;
typedef StringPtr *tlStringHandle;
typedef Handle tlBufferHandle;
typedef struct MyWindowStruct *MyWindowPtr;
typedef struct mstruct **MessHandle;
typedef struct PETEPrivateDocumentInfo **PETEHandle;
typedef struct HeadSpec *HSPtr;
typedef struct HeaderDesc *HeaderDPtr, **HeaderDHandle;
typedef struct TOCType TOCType, **TOCHandle;

#define TOOLBAR_ICON_RTYPE	'TIcn'
#define TOOLBAR_ICON_ID	1001

OSErr ETLInit(void);		// Initialize the whole darn thing, including UI
void ETLCleanup(void);		// Shut it all down
OSErr ETLListAllTranslatorsLo(TLMHandle * translators, short context,
			      ModeTypeEnum forMode);
OSErr ETLCountTranslatorsLo(short context, ModeTypeEnum forMode);
#define ETLListAllTranslators(translators,context) ETLListAllTranslatorsLo(translators,context,GetCurrentPayMode())
#define ETLCountTranslators(context) ETLCountTranslatorsLo(context,GetCurrentPayMode())
OSErr ETLCanTranslate(TLMHandle translators, short context,
		      emsMIMEHandle emsMIME, tlStringHandle * errorStr,
		      long *errCode, emsHeaderDataP addrList,
		      HeaderDHandle hdh);
OSErr ETLRemoveDeadTranslators(TLMHandle translators);
OSErr ETLInterpretFile(short context, FSSpecPtr source, short resultRefN,
		       AccuPtr resultAcc, emsHeaderDataP addrList,
		       Boolean * dontSave);
void ETLDoAbout(void);
enum { TLMIME_TYPE, TLMIME_SUBTYPE, TLMIME_PARAM, TLMIME_VERSION,
	    TLMIME_CONTENTDISP, TLMIME_CONTENTDISP_PARAM };
OSErr NewTLMIME(emsMIMEHandle * emsMIME);
void DisposeTLMIME(emsMIMEHandle emsMIME);
#define ZapTLMIME(m) do{DisposeTLMIME(m); m = nil;}while(0)
OSErr RecordTLID(FSSpecPtr spec, uLong id);
OSErr AddTLMIME(emsMIMEHandle emsMIME, short what, PStr name, PStr value);
OSErr FlattenTLMIME(emsMIMEHandle emsMIME, FlatTLMIMEHandle * flat);
OSErr UnflattenTLMIME(FlatTLMIMEHandle flat, emsMIMEHandle * tlMIME);
OSErr TransRecvLine(TransStream stream, UPtr line, long *size);
OSErr ETLDisplayFile(FSSpecPtr spec, PETEHandle pte);
OSErr ETLAddIcons(MyWindowPtr win, short startNumber);
long ETLIconToID(short which);
OSErr ETLIconToDescriptions(short which, PStr module, PStr translator);
short ETLIDToIndex(long id);
short ETLSendMessage(TransStream stream, MessHandle messH, Boolean chatter,
		     Boolean sendDataCmd);
OSErr ETLCanTransOut(MessHandle messH);
OSErr ETLTransOut(MessHandle messH, emsMIMEHandle emsMIME, FSSpecPtr from,
		  FSSpecPtr to);
OSErr ETLTransSelection(PETEHandle pte, HSPtr hs, short item);
long ETLID(TLMHandle tl, short index);
OSErr ETLIDToFileIcon(long id, Handle * suite);
OSErr ETLReadTL(FSSpecPtr spec, long *id);
Boolean ETLExists(void);
OSErr ETLSpecial(short item);
void ETLEnableSpecialItems();
OSErr ETLAttach(short item, MyWindowPtr win);
OSErr ETLDoSettings(short item);
void ETLNameAndIcon(short i, PStr name, Handle * suite);
OSErr ETLSelect(short which, Boolean selecting, MessHandle messH);
void ETLGetSystemPlugins(void);
OSErr ETLBuildAddrList(UHandle textIn, UHandle moreHeaders,
		       HeaderDHandle hdh, emsHeaderDataP addrList,
		       short context);
void ETLDisposeAddrList(emsHeaderDataP addrList);
void ETLIdle(long flags);
ModeTypeEnum GetCurrentPayMode();
void ETLEudoraModeNotification(ModeEventEnum modeEvent,
			       ModeTypeEnum newMode);
void ETLDrawBoxTag(TOCHandle tocH, Rect * r);
void ETLAddBoxButtons(TOCHandle tocH);
void ETLButtonHit(MyWindowPtr win, short item);
Boolean ETLClickContextMenu(MyWindowPtr win, Point pt, Rect * rSizeBox);
Boolean ETLHasMBoxContextFolder(MyWindowPtr win);
short ETLMBoxContextFolder(MyWindowPtr win, short *vRefNum, long *dirID);
OSErr ETLGetPluginFolderSpec(FSSpec * spec, short nameId);
Handle ETLMenu2Icon(short menu, short item);
void ETLAddToToolbar(void);
short ETLBoxTagWidth(MyWindowPtr win);
short ETLQueueMessage(MessHandle messH);
long ETLDrain(void);
Boolean IsPlugwindow(WindowPtr theWindow);
Boolean IsModalPlugwindow(WindowPtr theWindow);
Boolean IsNonModalPlugwindow(WindowPtr theWindow);
Boolean PlugwindowEventFilter(EventRecord * event);
void PlugwindowEnable(WindowPtr theWindow, long *flags);
Boolean PlugwindowMenu(WindowPtr theWindow, long select);
Boolean PlugwindowClose(WindowPtr theWindow);
OSErr PlugwindowDrag(WindowPtr theWindow, DragTrackingMessage message,
		     DragReference drag);
void PlugwindowSendFakeEvent(WindowPtr theWindow, UInt32 message,
			     EventKind what, EventModifiers modifiers,
			     Point * where);
void PlugwindowActivate(WindowPtr theWindow, Boolean active);
void PlugwindowUpdate(WindowPtr theWindow);
OSErr ETLImport(long id, ImportOperationEnum what, void *params,
		void *results);
OSErr ETLQueryImporters(ImportAccountInfoH * results, long id,
			Boolean search);
Handle GetImporterAppIcon(long id);
void GetImporterName(long id, Str255 name);
OSErr ETLImportSignatures(ImportAccountInfoP account);
OSErr ETLImportAddresses(ImportAccountInfoP account);
OSErr ETLImportMail(ImportAccountInfoP account);
OSErr ETLImportSettings(ImportAccountInfoP account,
			ImportPersDataH * persData);
OSErr ETLScoreJunk(TLMPtr thePlugin, emsTranslatorP transInfo,	/* In: Translator Info */
		   emsJunkInfoP junkInfo,	/* In: Junk information */
		   emsMessageInfoP message,	/* In: Message to score */
		   emsJunkScoreP junkScore,	/* Out: Junk score */
		   emsResultStatusP status	/* Out: Status information */
    );

OSErr ETLMarkJunk(TLMPtr thePlugin, emsTranslatorP transInfo,	/* In: Translator Info */
		  emsJunkInfoP junkInfo,	/* In: Junk information */
		  emsMessageInfoP message,	/* In: Message to score */
		  emsJunkScoreP junkScore,	/* Out: Junk score */
		  emsResultStatusP status	/* Out: Status information */
    );

#endif
