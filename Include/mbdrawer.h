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

#ifndef MBDRAWER_H
#define MBDRAWER_H

void MBDrawerToggle(MyWindowPtr winParent);
void MBDrawerFixUnread(MenuHandle mh,short item,Boolean unread);
MyWindowPtr MBDrawerOpen(MyWindowPtr winParent);
Boolean MBDrawerSupported(MyWindowPtr win);
void MBDrawerSetFocus(MyWindowPtr win, Boolean focus);
void MBDrawerActivate(MyWindowPtr win, Boolean activate);

enum 
{
	kDefaultMBDrawerWidth = 225
};

#if TARGET_RT_MAC_CFM
// These items aren't in the carbon headers yet
enum 
{ 
	kDrawerWindowClass = 20,
	kWindowCompositingAttribute = (1L << 19)
};
enum { kWindowEdgeDefault = 0, kWindowEdgeTop = 1 << 0, kWindowEdgeLeft = 1 << 1, kWindowEdgeBottom = 1 << 2, kWindowEdgeRight = 1 << 3 }; 
#endif

extern OSStatus SetDrawerParent(WindowRef inDrawerWindow, WindowRef inParent);
extern OSStatus ToggleDrawer(WindowRef inDrawerWindow);
extern OSStatus OpenDrawer(WindowRef inDrawerWindow, OptionBits inEdge, Boolean inAsync);
extern OSStatus CloseDrawer (WindowRef inDrawerWindow, Boolean inAsync); 
extern Boolean MBDrawerClose(MyWindowPtr win);
void MBDrawerReload(void);

#endif