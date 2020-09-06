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

#ifndef ENDS_H
#define ENDS_H

#include <Lists.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
void Initialize(void);
void FigureOutFont(Boolean peteToo);
void Cleanup(void);
void BuildBoxMenus(void);
void RememberOpenWindows(void);
void RecallOpenWindows(void);
void TrashMenu(MenuHandle mh, short beginAtItem);
void OpenNewSettings(FSSpecPtr spec, Boolean changeUserState, UserStateType newState);
void SetSendQueue(void);
void GetBoxLines(void);
void SystemEudoraFolder(void);
PStr VersString(PStr string);
void AddFinderItems(MenuHandle mh);
void BuildStationMenu(void);
void BuildPersMenu(void);
void BuildSigMenu(void);
void MySetMenuTitle(MenuHandle menu,PStr title);
void CleanTempFolder(void);
Boolean DateWarning(Boolean uiOK);
#ifdef THREADING_ON
void CreateTempBox(short which);
#endif // THREADING_ON
#ifdef HAVE_KEYCHAIN
void KeychainConvert(void);
#endif // HAVE_KEYCHAIN

pascal void SettingsListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell, short lDataOffset, short lDataLen, ListHandle lHandle);
#ifdef FANCY_FILT_LDEF
pascal void FiltListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell, short lDataOffset, short lDataLen, ListHandle lHandle);
#endif
pascal void TlateListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell, short lDataOffset, short lDataLen, ListHandle lHandle);
pascal void ListViewListDef(short message, Boolean select, Rect *pRect, Cell cell, short dataOffset, short dataLen, ListHandle hList);
pascal void ListItemDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell, short lDataOffset, short lDataLen, ListHandle lHandle);
pascal long AppCdef(short varCode, ControlHandle theControl, short message, long param);
pascal long ColCdef(short varCode, ControlHandle theControl, short message, long param);
pascal long ListCdef(short varCode, ControlHandle theControl, short message, long param);
pascal void FeatureListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell, short lDataOffset, short lDataLen, ListHandle lHandle);
#ifdef DEBUG
pascal long DebugCdef(short varCode, ControlHandle theControl, short message, long param);
#endif

typedef OSErr (*FolderSnifferFunc)(short vRefNum,long parID,uLong refCon);
void FolderSniffer(short subfolderID,FolderSnifferFunc sniffer,uLong refCon);
void FolderSnifferStr(PStr subfolderName,FolderSnifferFunc sniffer,uLong refCon);

#endif
