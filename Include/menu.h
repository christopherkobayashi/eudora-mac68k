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

#ifndef MENU_H
#define MENU_H

void EnableMenus(WindowPtr winWP,Boolean all);
void EnableMenuItems(Boolean all);
void DoMenu(WindowPtr topWin,long selection,short modifiers);
#define DoMenu2(w,m,i,mods)	DoMenu(w,(m)<<16|(i),mods)
Boolean EnableIf(MenuHandle mh, short item, Boolean expr);
void CheckBox(MyWindowPtr win,Boolean all);
void CheckPrior(MyWindowPtr win,Boolean all);
short MailboxKindaMenu(short mnu,short item,PStr s,FSSpecPtr spec);
void SetCheckItem(PStr item);
Boolean DoIndependentMenu(int menu,int item,short modifiers);
void SetMenuTexts(short modifiers,Boolean all);
void CheckState(MyWindowPtr win,Boolean all,short origItem);
void SaveAll(void);
OSErr NewMessageWith(short item);
void FixRecipMenu(void);
void SetupRecipMenus(void);
void SetupRecipMenusWith(Handle	hRecip);
void PruneRecipMenu(void);
void InsertSystemConfig(PETEHandle pte);
OSErr HMGetHelpMenuHandle(MenuRef *mh);
void MyEnableItem(MenuHandle mh,short item);
void MyDisableItem(MenuHandle mh,short item);
void AdjustStupidCommandKeys(void);
OSErr ReadReplacmentCommandKeyPref(short pref, char *key, UInt8 *modifiers);
long STUPIDCheckMailHack(EventRecord *event);
Boolean MenuItemIsSeparator(MenuHandle theMenu, short item);

Boolean ChangePWMenuShown ();
short   AdjustSpecialMenuSelection ( short item );
short   AdjustSpecialMenuItem      ( short item );

#ifdef MENUSTATS
void NoteMenuSelection(long selection);
#endif
#ifdef USECMM
void		HandleContextualMenuClick( WindowPtr topWinWP, EventRecord* event );
#endif

#ifdef DEBUG
enum {
/*    1 */ 	dbNoOffscreenBM=1,
/*    2 */ 	dbPeteBusy,
/*    3 */ 	dbSerialNos,
/*    4 */ 	dbRandNickEvents,
/*    5 */ 	dbAEStuff,
/*    6 */ 	dbNoDblBuffs,
/*    7 */ 	dbHeapCheck,
/*    8 */ 	dbArrivalTimes,
/*    9 */ 	dbUseResFile,
/*   10 */ 	dbNoTempMem,
/*   11 */ 	dbOneShotProfile,
/*   12 */ 	dbGrowZone,
/*   13 */ 	dbPorts10k,
/*   14 */ 	dbTCPBuff,
/*   15 */ 	dbVisComp,
/*   16 */ 	dbMsgFetchTrace,
/*   17 */ 	dbSeparator1,
/*   18 */ 	dbDebugger,
/*   19 */ 	dbIMAPMsgFlags,
/*   20 */ 	dbSLReseLeaks,
/*   21 */ 	dbAds,
/*   22 */ 	dbCmdKey,
/*   23 */ 	dbTestSpooler,
/*   24 */ 	dbDumpPte,
/*   25 */ 	dbNag,
/*   26 */ 	dbDumpCtl,
/*   27 */ 	dbAdWin,
/*   28 */	dbAudit,
/*   29 */	dbLWSPMatch,
/*   30 */	dbAddRecvdStat,
/*   31 */	dbAddSentStat,
/*   32 */	dbWindowPtrs,
dbLimit
};
#endif		

#endif
