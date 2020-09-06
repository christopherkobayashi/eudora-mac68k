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

#ifndef FMTBAR_H
#define FMTBAR_H

/* MJN *//* new file *//* moved text formatting toolbar code from compact.h to here */

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for functions relating to composition window actions
 ************************************************************************/
Boolean TextFormattingBarHelp(MyWindowPtr win,Point mouse); /* MJN *//* new routine */
Boolean TextFormattingBarClick(MyWindowPtr win, EventRecord* event, Point clickLoc); /* MJN *//* new routine */
Boolean TextFormattingBarButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part,short which); /* MJN *//* new routine */
void TextFormattingBarIdle(MyWindowPtr win); /* MJN *//* new routine */
void AddTextFormattingBarIcons(MyWindowPtr win,Boolean visible,Boolean enabled); /* MJN *//* new routine */
short GetTxtFmtBarHeight(void); /* MJN *//* new routine */
void GetTxtFmtBarRect(MyWindowPtr win,Rect *txtFmtBarRect); /* MJN *//* new routine */
Boolean TextFormattingBarVisible(MyWindowPtr win); /* MJN *//* new routine */
Boolean TextFormattingBarEnabled(MyWindowPtr win); /* MJN *//* new routine */
Boolean TextFormattingBarOK(MyWindowPtr win); /* MJN *//* new routine */
Boolean TextFormattingBarAllowed(MyWindowPtr win); /* MJN *//* new routine */
Boolean TFBMenusAllowed(MyWindowPtr activeWindow); /* MJN *//* new routine */
void InvalTextFormattingBar(MyWindowPtr win); /* MJN *//* new routine */
void HideTxtFmtBar(MyWindowPtr win); /* MJN *//* new routine */
void ShowTxtFmtBar(MyWindowPtr win); /* MJN *//* new routine */
void HideShowTxtFmtBar(MyWindowPtr win,Boolean visible); /* MJN *//* new routine */
void ToggleTxtFmtBarVisible(MyWindowPtr win); /* MJN *//* new routine */
void HideShowAllTFB(Boolean visible); /* MJN *//* new routine */
void EnableTxtFmtBar(MyWindowPtr win); /* MJN *//* new routine */
void DisableTxtFmtBar(MyWindowPtr win); /* MJN *//* new routine */
void SetTxtFmtBarEnabled(MyWindowPtr win,Boolean enabled); /* MJN *//* new routine */
void ToggleTxtFmtBarEnabled(MyWindowPtr win); /* MJN *//* new routine */
void EnableTxtFmtBarIfOK(MyWindowPtr win); /* MJN *//* new routine */
void RequestTFBEnableCheck(MyWindowPtr win); /* MJN *//* new routine */
void TFBRespondToSettingsChanges(void); /* MJN *//* new routine */
void MatchTFBToCurSettings(MyWindowPtr win); /* MJN *//* new routine */
void GetCurEditorMargins(PETEHandle pte, PSMPtr currentTxtMargin);
#define kSeparatorRefCon 'fmSp'

#endif  //ifndef FMTBAR_H
