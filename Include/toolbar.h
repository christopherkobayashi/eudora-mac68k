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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <ControlDefinitions.h>

#ifdef TWO

typedef enum {
	tbvBigName = 1,
	tbvBig,
	tbvSmallName,
	tbvSmall,
	tbvName,
	tbvLimit
} ToolbarVEnum;

#define VarHasName(aVarCode) ((aVarCode)!=tbvBig && (aVarCode)!=tbvSmall)
#define VarHasIcon(aVarCode) ((aVarCode)!=tbvName)


void OpenToolbar(void);
OSErr ReloadToolbar(void);
void ToolbarRect(Rect * r);
Boolean ToolbarSect(Rect * r);
void ToolbarReduce(Rect * r);
void ShowToolbar(void);
void HideToolbar(void);
Boolean ToolbarShowing(void);
void ToolbarBack(void);
Boolean TBAddMenuButton(short menu, short item, PStr itemText);
#endif

void ToolbarCursor(Point mouse);
Boolean TBEventFilter(EventRecord * event, Boolean oldResult);
Boolean ToolbarNudge(MyWindowPtr win, Boolean gently);
void ToolbarNudgeAll(Boolean gently);
void TBClearBalloon(void);
Boolean ToolbarNudgeRect(MyWindowPtr win, Rect * newRect, Boolean gently);
void ToolbarIdleControls(void);
Boolean IsTPIdleControlVisible(void);
void AssignCmdKey(short modifiers);
void ChangeCmdKeys(void);
void TBDisable(void);
OSErr TBRemoveDefunctMenuButton(short menu, short item);
void TBRemoveDefunctNicknameButton(PStr nickname);
void TBAddAdButtons(TBAdHandle hTBAds);
void TBUpdateAdButtonIcon(AdId adId, Handle iconSuite);
void TBForceIdle(void);
Boolean TBRenameNickButton(PStr oldName, PStr newName);
void GetButtonAlignment(ToolbarVEnum varCode,
			ControlButtonTextAlignment * alignment,
			ControlButtonTextPlacement * placement,
			short *textOffset,
			ControlButtonGraphicAlignment * gAlignment,
			Point * gOffset);
OSErr TellToolMBRename(FSSpecPtr spec, FSSpecPtr newSpec, Boolean folder,
		       Boolean will, Boolean dontWarn);
Boolean ConfiguringToolbarMenuItem(short menuId, short item);

#endif
