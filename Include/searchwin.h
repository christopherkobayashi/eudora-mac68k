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

#ifndef SEARCHWIN_H
#define SEARCHWIN_H

MyWindowPtr SearchOpen(short searchMode);
#define SearchNewFindString(w) SearchNewFindStringLo(w,false)
void SearchNewFindStringLo(PStr what,Boolean withPrejuidice);
void StartSearch(MyWindowPtr win);
void SearchUpdateSum(TOCHandle tocH, short sumNum, TOCHandle oldTocH, long oldSerialNum, Boolean transfer, Boolean nuke);
MyWindowPtr OpenSearchFile(FSSpecPtr spec);
MyWindowPtr OpenSearchFileAndStart(FSSpecPtr spec);
Boolean IsSearchWindow(WindowPtr theWindow);
Boolean GetSearchWinSpec(WindowPtr winWP,FSSpecPtr spec);
void BuildSearchMenu(void);
void OpenSearchMenu(short item);
void TellSearchMBRename(FSSpecPtr spec,FSSpecPtr newSpec);
TOCHandle GetTOCFromSearchWin(FSSpecPtr spec);
void SearchMBUpdate(void);
void SearchFixUnread(MenuHandle mh,short item,Boolean unread);
void GetSearchTOC(MyWindowPtr win,TOCHandle *ptoc);
void SearchInvalTocBox(TOCHandle tocH, short sumNum, short box);
void CopySum(MSumPtr sumFrom, MSumPtr sumTo, short virtualMBIdx);
Boolean SearchViewIsMailbox(TOCHandle tocH);
void SearchAllIdle(void);
void IMAPSearchIncremental(MailboxNodeHandle mbox);
#endif