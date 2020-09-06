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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
OSErr RemSpoolFolder(long uidHash);
void SendBack(WindowPtr theWindow);
void DoAboutUIUCmail(void);
MyWindowPtr DoComposeNew(TextAddrHandle toWhom);
void DoQuit(Boolean changeUserState);
void NotImplemented(void);
#ifdef	IMAP
void EmptyTrash(Boolean localOnly, Boolean currentOnly, Boolean all);
#else
void EmptyTrash(void);
#endif
void AddSelectionAsTo(void);
void AddStringAsTo(UPtr string);
void RemoveFromTo(UPtr name);
void ToMenusChanged(void);
void MyBalloon(Rect *tipRect,short percentRight,short method,short resSelect,short pict,PStr message);
Boolean CloseAll(Boolean toolbarToo);
Boolean CloseAllBoxes(void);
Boolean CloseAllMessages(void);
void DoHelp(WindowPtr winWP);
void HandleWindowChoice(short item);
short ChangePassword(void);
OSErr GetAppLocation(AEAddressDescPtr aead,AliasHandle *alias);
OSErr LaunchByAlias(AliasHandle alias);
OSErr FindPSNBySpec(ProcessSerialNumberPtr psnPtr,FSSpecPtr spec);
OSErr FindPSNByCreator(ProcessSerialNumberPtr psnPtr,OSType creator);
OSErr FindPSNByAlias(ProcessSerialNumberPtr psnPtr,AliasHandle alias);
OSErr LocateApp(AliasHandle alias);
uLong DefaultOutFlags(void);
void HelpTextWin(short itm);
void TextResWin(PStr name);
void HelpPICTWin(short itm);
void PICTResWin(PStr name);
void PreciseHelpRectString(Rect *tipRect,UPtr string,Point tip,short method);
void Type2Select(EventRecord *event);
#ifdef TWO
OSErr FindSpeller(void);
OSErr ServeThemWords(short item,AEDescPtr objAD);
void ApplyDefaultStationery(MyWindowPtr win,Boolean bodyToo,Boolean personality);
OSErr DoCompWStationery(FSSpecPtr spec);
#endif
OSErr OpenAttachedApp(FSSpecPtr spec);
void PurchaseEudora(void);
void ApplyIndexStationery(MyWindowPtr win,short which,Boolean bodyToo,Boolean personality);
OSErr InstallSpeller(FSSpecPtr spec);
void DoSFOpen(short modifiers);
void DoSFOpenStd(short modifiers);
void DelWSService(short item,Boolean toolbarToo);
OSErr InsertTextFile(MyWindowPtr win, FSSpecPtr spec, Boolean rich, Boolean delSP);
PStr AboutRegisterString(PStr s);
typedef struct
{
	Boolean insert;
	Str255 buttonName;
	StandardFileReply *reply;
	Boolean attaching;
	Boolean html;
	Boolean insertDefault;
} InsertHookData;
pascal short TextInsertHook(short item,DialogPtr dgPtr, InsertHookData *ihdp);
pascal Boolean SFOpenFilter(CInfoPBPtr pb, UPtr data);
pascal Boolean ModalThreadYieldDialogFilter(DialogPtr theDialog, EventRecord *theEvent,short *itemHit,void *data);
void ReportABug(void);
void MakeASuggestion(void);
Boolean MyHMHideTag(void);
OSErr MyHMDisplayTag(HMHelpContentPtr hmp);
#endif