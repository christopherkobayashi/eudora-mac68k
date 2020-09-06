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

#include "menu.h"
#define FILE_NUM 11
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Menu
	void SetQItemText(MyWindowPtr win);
	Boolean DoDependentMenu(WindowPtr topWinWP,int menu,int item,short modifiers);
	void BuildWindowsMenu(Boolean allMenu);
	Boolean SameSettingsFile(short vRef,long dirId,UPtr name);
	void InsertRecipient(MyWindowPtr win, short which,Boolean all);
	void CheckTable(MyWindowPtr win,Boolean all);
	void CheckLabel(MyWindowPtr win,Boolean all);
	void EnableHelpItems(void);
	void NewSigFile(void);
Boolean TextMenuOK(MyWindowPtr win);
void CheckTextItems(MyWindowPtr win,short modifiers,Boolean all);
void TextMenuChoice(short menu,short item,short modifiers);
void MarginMenuChoice(PETEHandle pte,short item,PETEParaInfoPtr pinfo,long *paraValid);
void SaveRecipMenu(MenuHandle mh);

//	All this to get rid of the "Change Password Menu Item"
short gDeletedSpecialItem = 9999;		// Where have we deleted an item?
Boolean ChangePWMenuShown () 					{ return gDeletedSpecialItem > 1000; }
short AdjustSpecialMenuSelection ( short item ) { return item >= gDeletedSpecialItem ? item + 1 : item; }
short AdjustSpecialMenuItem      ( short item ) { return item >= gDeletedSpecialItem ? item - 1 : item; }

#define TICKS2MINS 3600
/*************************************CMM Stuff ***************************************************/
#ifdef USECMM
typedef enum
{
	CONTEXT_TERM_ITEM,
	CONTEXT_SEPARATOR_ITEM,
	CONTEXT_ITEMS_LIMIT
} ContextSpecialItems;
OSStatus	DoHandleContextualMenuClick( WindowPtr topWinWP, EventRecord* event );
OSStatus	BuildBasicContextMenu( MenuHandle contextMenu );
#endif

/**********************************************************************
 * DoMenu - handle a menu selection
 **********************************************************************/
//	(jp)	Not all windows will be ours when this is called (some may be dialog)
//				so we really need to pass in a WindowPtr instead of MyWindowPtr
void DoMenu(WindowPtr topWin,long selection,short modifiers)
{
	short menu = (selection >> 16) & 0xffff;
	short item = selection & 0xffff;
	Str255	s;
	
	CHECK_EXPIRE;
#ifdef MENUSTATS
	if (LOG_MENU&LogLevel)
	{
		Str31 mStr,iStr;
		PCopy(mStr,"\pnone");
		PCopy(iStr,mStr);
		if (menu)
		{
			GetMenuTitle(GetMHandle(menu),mStr);
			if (item) MyGetItem(GetMHandle(menu),item,iStr);
		}
		ComposeLogS(LOG_MENU,nil,"\p%p %p",mStr,iStr);
	}
	
	NoteMenuSelection(selection);
#endif

	if (!item)
	{
		long	nonSelection;
		if (nonSelection = MenuChoice())	//	Was a non-selectable menu item selected?
		{
			short	nonMenu = (nonSelection >> 16) & 0xffff; 
			short	nonItem = nonSelection & 0xffff;
			
			if (IsMailboxChoice(nonMenu,nonItem))
			if (MyGetItem(GetMHandle(nonMenu),nonItem,s)[1]!='-' || *s!=1)
			if (nonMenu==MAILBOX_MENU || ((menu-(g16bitSubMenuIDs?BOX_MENU_START:1))/MAX_BOX_LEVELS) == MAILBOX)
			{
				//	Mailbox folder
				MenuHandle	hAddMenu;
				Handle		hStringList;
				
				if (hStringList = NuHandleClear(1))
				{
					Boolean		IsIMAP;
					short	vRef;
					long	dirID;
					short	subMenu;

					//	If in main mailbox menu, we need to go to a sub level to accurately
					//	determine if this is an IMAP folder
					subMenu = nonMenu==MAILBOX_MENU ? SubmenuId(GetMHandle(nonMenu), nonItem) : 0;
					for(hAddMenu = GetMHandle(nonMenu);hAddMenu;hAddMenu = ParentMailboxMenu(hAddMenu,&nonItem))
					{
						MyGetItem(hAddMenu,nonItem,s);
						Munger(hStringList,0,nil,0,s,*s+1);	//	Insert at front of handle
					}
					MenuID2VD(subMenu?subMenu:nonMenu,&vRef,&dirID);
					IsIMAP = IsIMAPVD(vRef,dirID);
					if (!IsIMAP)
					{
						//	Add Eudora Folder
						GetDirName(nil,Root.vRef,Root.dirId,s);			
						Munger(hStringList,0,nil,0,s,*s+1);	//	Insert at front of handle
					}
					MBOpenFolder(hStringList,IsIMAP);
					DisposeHandle(hStringList);
				}
			}
		}
	}

	if (!PETEMenuSelectFilter(PETE,selection) && menu && item)
	{
		PushCursor(watchCursor);
#ifdef TWO
		if (!SharedMenuHit(menu,item))
#endif
		{
#if __profile__
			Boolean	doProfile = BUG10;

			if (doProfile) ProfilerClear();
#endif
			if (menu==FILE_MENU && item==FILE_SAVE_ITEM && 0!=(modifiers&optionKey))
				SaveAll();
			if (!(topWin && GetWindowKind(topWin)==dialogKind &&
						menu==EDIT_MENU && DoModelessEdit(GetWindowMyWindowPtr(topWin),item)) &&
					!DoDependentMenu(topWin,menu,item,modifiers) &&
					!DoIndependentMenu(menu,item,modifiers))
					/*NotImplemented()*/;

#if __profile__
			if (doProfile)
			{
				ProfilerDump("\pEudora Menu Profile");
				SetItemMark(GetMHandle(DEBUG_MENU),11,0);
				BugFlags &= ~(1L<<(11-1));	
			}
#endif
		}
		PopCursor();
  }
	HiliteMenu(0);
}

#ifdef MENUSTATS
/**********************************************************************
 * NoteMenuSelection - save a user's menu selection
 **********************************************************************/
void NoteMenuSelection(long selection)
{
	struct {short what; long selection;} addme;
	static Handle saved;
	long size;
	FSSpec spec;
	short refN;
	Str255 user, host;
	
	if (selection!=-1)
	{
		addme.what = MainEvent.what;
		addme.selection = selection;
		if (!saved) saved = NuHTempBetter(0);
		if (saved) PtrPlusHand(&addme,saved,sizeof(addme));
	}
	
	if (saved)
	{
		size = GetHandleSize(saved);
		if (size && (size>20 K || selection==-1))
		{
			GetPOPInfo(user,host);
			PCat(user,"\p-MenuStats");
			SimpleMakeFSSpec(Root.vRef,Root.dirId,user,&spec);
			FSpDelete(&spec);
			PCat(user,"\pNew");
			FSpCreate(&spec,CREATOR,'menu',smSystemScript);
			if (!FSpOpenDF(&spec,fsRdWrPerm,&refN))
			{
				SetFPos(refN,fsFromLEOF,0);
				AWrite(refN,&size,LDRef(saved));
				UL(saved);
				MyFSClose(refN);
			}
			SetHandleBig(saved,0);
		}
	}
}
#endif

/************************************************************************
 * SaveAll - save all windows
 ************************************************************************/
void SaveAll(void)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	
	RememberOpenWindows();
	for (winWP=MyFrontWindow();winWP;winWP=GetNextWindow(winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsKnownWindowMyWindow(winWP) && win && IsDirtyWindow(win))
			DoMenu(winWP,(FILE_MENU<<16)|FILE_SAVE_ITEM,0);
	}
}

/**********************************************************************
 * DoIndependentMenu - handle a menu selection that does not depend on
 * what window is frontmost.	returns True if selection handled, False else
 **********************************************************************/
Boolean DoIndependentMenu(int menu,int item,short modifiers)
{
	Str255 scratch;
	short function;
	MenuHandle mh=GetMHandle(menu);
	MyWindowPtr newWin;
	WindowPtr winWP=MyFrontWindow();
	MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
	FSSpec spec;
	short n;
	TextAddrHandle addr;
	Boolean plain = 0!=(modifiers&shiftKey);
	short kind;
#ifdef ETL
	HeadSpec hs;
#endif
	
	switch (menu)
	{
		case APPLE_MENU:
			switch (item)
			{
				case APPLE_ABOUT_ITEM:
					DoAboutUIUCmail();
					return(True);
				case APPLE_ABOUT_TRANS_ITEM:
					ETLDoAbout();
					return(True);
				default:
					return(False);
			}
			break;

		case EDIT_MENU:
			if (item==EDIT_PASTE_ITEM && modifiers&optionKey) item = EDIT_QUOTE_ITEM;	// ugly hack, but otherwise shift-opt-cmd-v doesn't work
			
			switch (item)
			{
				case EDIT_FINISH_ITEM:
					// (jp) this is being moved in here to handle things a bit more generically...
					/* If the user pressed Command-Shift-Option-Comma, temporarily toggle nickname type-ahead.  This
							change will get reset to the setting in preferences the next time a composition window receives
							an activate/deactivate event. */
					if (IsMyWindow(winWP) && win) {
						if (win->pte && HasNickCompletion (win->pte))
							if ((modifiers & (cmdKey | shiftKey | optionKey)) == (cmdKey | shiftKey | optionKey))
								ToggleNicknameTypeAhead (win->pte);
							else
								FinishAlias (win->pte, (modifiers & optionKey) != 0 && (HasNickExpansion (win->pte) || HasNickCompletion (win->pte)), True, false);
						return(True);
					}
					break;
					
				case EDIT_UNDO_ITEM:
					DoUndo();
					return(True);
					break;
				case EDIT_SELECT_ITEM:
					{
						if (win->pte)
						{
							PeteSelectAll(win,win->pte);
							return(True);
						}
					}
					break;
				case EDIT_COPY_ITEM:
					if (win->pte)
					{
						PeteEdit(win,win->pte,plain ? peeCopyPlain:peeCopy,&MainEvent);
						if (modifiers&optionKey) UnwrapClip();
					}
					break;
				case EDIT_CLEAR_ITEM:
					if (win->pte) PeteEdit(win,win->pte,peeClear,&MainEvent);
					break;
				case EDIT_CUT_ITEM:
					if (win->pte)
					{
						PeteEdit(win,win->pte,plain ? peeCutPlain:peeCut,&MainEvent);
						if (modifiers&optionKey) UnwrapClip();
					}
					break;

				case EDIT_PASTE_ITEM:
					if (win->pte)
					{
						PeteEdit(win,win->pte,PrefIsSetOrNot(PREF_PASTE_PLAIN,modifiers,shiftKey) ? peePastePlain:peePaste,&MainEvent);
					}
					break;

				case EDIT_QUOTE_ITEM:
					if (win->pte)
					{
						PetePasteQ(win,PrefIsSetOrNot(PREF_PASTE_PLAIN,modifiers,shiftKey));
					}
					break;

				case EDIT_WRAP_ITEM:
					if (win->pte)
					{
						PeteWrap(win,win->pte,(modifiers&optionKey)!=0);
					}
					break;

#ifdef SPEECH_ENABLED
				case EDIT_SPEAK_ITEM:
					if ((modifiers & shiftKey)!=0)
						(void) SpeakSelectedText (nil, win->pte);
					break;
#endif
				case EDIT_SPELL_ITEM:
					FindSpeller();
					return(True);
					break;
				
#ifdef DIAL
				case EDIT_DIAL_ITEM:
					if (win->pte)
					{
						PeteString(scratch,win->pte);
						Dial(scratch,kDialDefault);
					}
					break;				
#endif
				default:
					if (item>EDIT_SPELL_ITEM)
						if (modifiers & optionKey)
							DelWSService(item,true);
						else
							HandleWordServices(win,item);
					return(True);
					break;
			}
			break;
		
#ifdef WINTERTREE
		case SPELL_HIER_MENU:
			SpellMenu(item,modifiers);
			return(True);
			break;
#endif
		
#ifdef ETL
		case TLATE_SEL_HIER_MENU:
			if (win && win->hasSelection && win->pte)
			{
				PeteGetTextAndSelection(win->pte,nil,&hs.value,&hs.stop);
				hs.start = hs.value;
			}
			else if (IsMessWindow(winWP))
				CompHeadFind(Win2MessH(win),0,&hs);
			else if (win && win->pte)
			{
				hs.start = hs.value = 0;
				hs.stop = PETEGetTextLen(PETE,win->pte);
			}
			else break;
			ETLTransSelection(win->pte,&hs,item);
			break;
#endif

		case TEXT_HIER_MENU:
		case FONT_HIER_MENU:
		case COLOR_HIER_MENU:
		case MARGIN_HIER_MENU:
		case TEXT_SIZE_HIER_MENU:
			TextMenuChoice(menu,item,modifiers);
			break;
			
		case HELP_MENU:
		case kHMHelpMenuID:
			n = EndHelpCount;
			if (item==n--) {ReportABug();break;}
			else if (item==n--) {MakeASuggestion();break;}
			else if (item==n--) {InsertSystemConfig(win->pte); break;}
#ifndef DEATH_BUILD
#ifdef NAG
			else
				// Payment & Registration�
				if (item==n--)
					OpenPayWin ();
#else
			else if (GetPrefLong(PREF_REGISTER)!=0x0fffffff && item==n--) {RegisterEudoraHi(); break;}
			else if (item==n--) PurchaseEudora();
#endif
#endif
			else HelpTextWin(item);
			break;
		
		case FILE_MENU:
			switch (item)
			{
				case FILE_NEW_ITEM:
					(void) OpenText(nil,nil,nil,nil,True,GetRString(scratch,UNTITLED),False,False);
					return(True);
					break;
#ifdef TWO
				case FILE_OPENSEL_ITEM:
					if (!AttIsSelected(win,win->pte,-1,-1,attAll+(modifiers&controlKey?attFinder:0),nil,nil))
						URLIsSelected(win,win->pte,-1,-1,urlAll,nil,nil);
					break;
#endif
				case FILE_OPENDOC_ITEM:
					DoSFOpen(modifiers);
					break;
				case FILE_CLOSE_ITEM:
					if (modifiers&optionKey)
					{
						CloseAll(False);
						return(True);
					}
					else if (IsMyWindow(MyFrontWindow()))
					{
						CloseMyWindow(MyFrontWindow());
						return(True);
					}
					else
						return(False);
					break;
				case FILE_IMPORT_MAIL_ITEM:
					DoMailImport();
					break;
				case FILE_EXPORT_ITEM:
					DoFileExport();
					break;
				case FILE_SEND_ITEM:
					if (SendThreadRunning)
						SendImmediately=True;
					else
						XferMail(False,True,True,False,True,modifiers);
					return(True);
				case FILE_CHECK_ITEM:
						XferMail(True,PrefIsSet(PREF_SEND_CHECK),True,False,True,modifiers);
					return(True);
				case FILE_PAGE_ITEM:
					DoPageSetup();
					return(True);
				case FILE_QUIT_ITEM:
					DoQuit(false);
					return(True);
			}
			break;
		case NEW_TO_HIER_MENU:
			if (newWin=DoComposeNew(addr=MenuItem2Handle(menu,item)))
			{
#ifdef TWO
				MessHandle messH =(MessHandle)GetMyWindowPrivateData(newWin);
				ApplyDefaultStationery(newWin,True,True);
				UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
#endif
				ShowMyWindow(GetMyWindowWindowPtr(newWin));
			}
			ZapHandle(addr);
			return(True);
			break;
			
		case FIND_HIER_MENU:
			DoFind(item,modifiers);
			return(True);
			break;
		
		case MESSAGE_MENU:
			switch(item)
			{
				case MESSAGE_NEW_ITEM:
					// ignore cmd-shift-N here when we're using an alternate command key for check mail.
					if (!PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD) || ((modifiers&shiftKey)==0))
					{
						if (newWin=DoComposeNew(0))
						{
							MessHandle messH =(MessHandle)GetMyWindowPrivateData(newWin);
#ifdef TWO
							ApplyDefaultStationery(newWin,True,True);
							UpdateSum(messH,SumOf(messH)->offset,SumOf(messH)->length);
#endif
							ShowMyWindow(GetMyWindowWindowPtr(newWin));
						}
						return(True);
					}
					break;
			}
			break;
			
		case MAILBOX_MENU:
			{
				short refN = CurResFile ();
				
				if (item>=MAILBOX_FIRST_USER_ITEM || item<MAILBOX_BAR1_ITEM)
				{
					MailboxMenuFile(menu,item,scratch);
					FSMakeFSSpec(MailRoot.vRef,MailRoot.dirId,scratch,&spec);
					(void) GetMailbox(&spec,True);
				}
	#ifdef TWO
				else if (item==MAILBOX_NEW_ITEM)
				{
					GetTransferParams(MAILBOX_MENU,item,&spec,nil);
				}
				else if (item==MAILBOX_OTHER_ITEM)
				{
					if (!AskGraft(MailRoot.vRef,MailRoot.dirId,&spec))
						(void) GetMailbox(&spec,True);
				}
	#endif
				UseResFile (refN);
				return(True);
				break;
			}

		case NEW_WITH_HIER_MENU:
			//Stationery - no support for this in Light
			if (HasFeature (featureStationery))
				NewMessageWith(item);
			break;

		case SPECIAL_MENU:
			item = AdjustSpecialMenuSelection (item);
			switch ( item )
			{
#ifdef CTB
				case SPECIAL_CTB_ITEM:
					(void) InitCTB(True);
					return(True);
#endif
				case SPECIAL_SETTINGS_ITEM:
					DoSettings(GetPrefLong(PREF_LAST_SCREEN));
					return(True);
				case SPECIAL_FORGET_ITEM:
					if (HasFeature (featureMultiplePersonalities) && DoWeUseSelectedPersonalities())
						ForgetPersPassword();
					else
						InvalidatePasswords(False,False,True);
					if (PrefIsSet(PREF_KERBEROS)) KerbDestroy();
					return(True);
				case SPECIAL_TRASH_ITEM:
					if (HasFeature (featureMultiplePersonalities) && DoWeUseSelectedPersonalities())
						EmptyIMAPPersTrash();
					else
					{
#ifdef	IMAP
						if (IMAPExists())
						{
							Boolean expunge = false;
							
							//
							// Remove Deleted Messages
							//
							
							// if the frontmost window is a mailbox
							if (winWP && GetWindowKind(winWP)==MBOX_WIN)
							{
								// and it's an IMAP mailbox
								if (((TOCHandle)GetMyWindowPrivateData(win)) && (*((TOCHandle)GetMyWindowPrivateData(win)))->imapTOC)
								{
									// and fancy trash is off for it
									if (!FancyTrashForThisPers((TOCHandle)GetMyWindowPrivateData(win)))
									{
										// and the option key is pressed, expunge it
										if (!(modifiers&shiftKey)&&(modifiers&optionKey)) 
										{
											expunge = true;
											return (IMAPRemoveDeletedMessages((TOCHandle)GetMyWindowPrivateData(win)));
										}
									}
								}
							}
							
							//
							// Empty Trash
							//
							
							if (!expunge) EmptyTrash(false, ((modifiers&shiftKey)&&!(modifiers&optionKey)), (!(modifiers&shiftKey)&&(modifiers&optionKey)));
						}
						else
							EmptyTrash(true, false, false);
#else
						EmptyTrash();
#endif
					}
					return(True);
				case SPECIAL_CHANGE_ITEM:
					if (HasFeature (featureMultiplePersonalities) && DoWeUseSelectedPersonalities())
						ChangePersPassword();
					else
						ChangePassword();
					return(True);
#ifdef THREADING_ON
#ifndef BATCH_DELIVERY_ON
				case SPECIAL_FILTER_ITEM:
					FilterXferMessages();
					break;
#endif
#endif
				default:
					if (item>SPECIAL_BAR4_ITEM)
					{
						ETLSpecial(item-SPECIAL_BAR4_ITEM);
						return(True);
					}
					break;
			}
			break;
		
		case SCRIPTS_MENU:	
			DoScriptMenu(item);
			return true;
					
#ifdef DEBUG
		case WIND_PROP_HIER_MENU:
			if (IsMyWindow(winWP)) DoWindowPropMenu(win,item,modifiers);
			return true;
#endif


		case WINDOW_MENU:
			if (item==WIN_PROPERTIES_ITEM)
			{
				DoWindowPropMenu(win,wpmTabs,modifiers);
				return true;
			}
			switch (item)
			{
#ifdef TWO
				case WIN_FILTERS_ITEM:
					OpenFiltersWindow();
					break;
#endif			
				case WIN_ALIASES_ITEM:
					OpenABWin((MainEvent.modifiers&shiftKey)?CurAddr(win,scratch):nil);
					break;
				case WIN_PH_ITEM:
				{
					Str255 string;
					
					if (win && IsMyWindow(winWP) && (modifiers&shiftKey) && win->hasSelection && win->pte)
					{
						PeteSelectedString(string,win->pte);
						if (*string && string[*string]==',') --*string;
						OpenPh(string);
					}
					else
						OpenPh(nil);
					break;
				}
				case WIN_MAILBOX_ITEM:
					OpenMBWin();
					break;
				case WIN_SIGNATURES_ITEM:
					OpenSignaturesWin();
					break;
				case WIN_PERSONALITIES_ITEM:
					if (HasFeature (featureMultiplePersonalities))
						OpenPersonalitiesWin();
					break;
				case WIN_STATIONERY_ITEM:
					if (HasFeature (featureStationery))
						OpenStationeryWin();
					break;
				case WIN_LINK_ITEM:
					if (HasFeature (featureLink))
						OpenLinkWin();
					break;
				case WIN_STATISTICS_ITEM:
					if (HasFeature (featureStatistics))
						OpenStatWin();
					break;
#ifdef TASK_PROGRESS_ON
				case WIN_TASKS_ITEM:
					OpenTasksWin();
					TaskDontAutoClose = true;
					break;
#endif
				case WIN_DRAWER_ITEM:
				   kind = GetWindowKind(winWP);
					if (kind==MBOX_WIN||kind==CBOX_WIN)
						MBDrawerToggle(win);
					break;
				default:
					HandleWindowChoice(item);
					break;
			}
			return true;

		case INSERT_TO_HIER_MENU:
			if (IsMyWindow(MyFrontWindow()))
				InsertRecipient(GetWindowMyWindowPtr(MyFrontWindow()),item,HasFeature (featureNicknameWatching) && PrefIsSetOrNot(PREF_NICK_AUTO_EXPAND,modifiers,optionKey));
			return(True);

		case EMOTICON_HIER_MENU:
			if (IsMyWindow(MyFrontWindow()))
				EmoInsert(GetWindowMyWindowPtr(MyFrontWindow()),item);
			return(True);
			break;
		
		case TLATE_SET_HIER_MENU:
			ETLDoSettings(item);
			break;
					
#ifdef DEBUG
		case DEBUG_MENU:
			{
				short theMark;

				if (item<=16)
				{
					GetItemMark(mh,item,&theMark);
					if (theMark)
					{
						SetItemMark(mh,item,0);
						BugFlags &= ~(1L<<(item-1));
					}
					else
					{
						SetItemMark(mh,item,checkMark);
						BugFlags |= (1L<<(item-1));
					}
				}
				else 
					switch (item)
					{
						case dbDebugger:
							Debugger();
							n = *(short*)0;	//	Cause exception to fall into GDB
							break;
#ifdef	IMAP
						case dbIMAPMsgFlags:	IMAPCollectFlags(); break;
#endif
						case dbSLReseLeaks:		SLResetLeaks();
						case dbDumpPte:
							if (IsMyWindow(winWP) && win && win->pte) PeteDump(win->pte);
							break;
#ifdef NAG
						case dbNag:				DebugNagging (); break;
#endif
						case dbDumpCtl:
							if (winWP)
							{
								FindFolder(kOnSystemDisk,kDesktopFolderType,false,&spec.vRefNum,&spec.parID);
								PCopy(spec.name,"\pControl Hierarchy");
								UniqueSpec(&spec,31);
								DumpControlHierarchy(winWP,&spec);
							}
							break;
#ifdef AD_WINDOW
						case dbAdWin:
							ToggleAdWindow((modifiers&optionKey)==0);
							break;
#endif
						case dbAudit:
							{
//								extern void AskSendAudit(void);
								AskSendAudit();
								break;
							}
						
						case dbLWSPMatch:
#define TM(res,lf,tx,s,e) if (res!=PPMatchLWSP(lf,tx,s,e)) {Debugger();PPMatchLWSP(lf,tx,s,e);}
#define TM2(cntns,strt,nd,lf,tx) TM(cntns,lf,tx,false,false); TM(cntns&&strt,lf,tx,true,false); TM(cntns&&nd,lf,tx,false,true); TM(cntns&&strt&&nd,lf,tx,true,true);
							TM2(true,false,false,"\p123","\p01234");
							TM2(true,false,true,"\p123","\p0123");
							TM2(true,true,false,"\p123","\p1234");
							TM2(true,true,true,"\p1 2 3","\p1 2 3");
							TM2(true,true,true,"\p1 2 3","\p1  2  3");
							TM2(true,true,true,"\p1  2  3","\p1 2 3");
							TM2(true,true,true,"\p1  2\r\r\r3","\p1 2 3");
							TM2(false,false,false,"\p1  23","\p1 2 3");
							TM2(true,false,false,"\p1  2  3","\p0 1 2 3 4");
							TM2(true,false,true,"\p1  2  3","\p0 1 2 3");
							TM2(true,true,false,"\p1  2  3","\p1 2 3 4");
							TM2(true,true,true,"\p1  2\r\r\r3","\p 1 2 3 ");
							break;

						case dbAddRecvdStat:
						{
							//	display all open files
							FCBPBRec fcb;
							short err;
							Str63 name;
							
							Zero(fcb);
							for(fcb.ioFCBIndx=1;true;fcb.ioFCBIndx++)
							{
								fcb.ioNamePtr = name;
								fcb.ioVRefNum = 0;
								if (err=PBGetFCBInfo(&fcb,False)) break;
								Dprintf("\p%d - %p, vol: %d, fileref: %d",fcb.ioFCBIndx,name,fcb.ioVRefNum,fcb.ioRefNum);
							}
						}	
						break;

						case dbAddSentStat:		UpdateNumStat(kStatSentMail,1); break;
					}
				return(True);
			}
			break;
		case AD_SELECT_HIER_MENU:	DebugAdMenu(item,modifiers); break;
#endif
		default:
			if (!IsMailboxChoice(menu,item)) return(False);
			function = (menu-(g16bitSubMenuIDs?BOX_MENU_START:1))/MAX_BOX_LEVELS;
			switch (function)
			{
				case MAILBOX:
					if (item>=MAILBOX_FIRST_USER_ITEM-MAILBOX_BAR1_ITEM)
					{
						GetTransferParams(GetMenuID(mh),item,&spec,nil);
						(void) GetMailbox(&spec,True);
					}
#ifdef TWO
					else if (item==MAILBOX_OTHER_ITEM-MAILBOX_BAR1_ITEM)
					{
						Menu2VD(mh,&spec.vRefNum,&spec.parID);
#ifdef	IMAP
						// if this is an IMAP cache directory, open the mailbox instead of grafting
						GetMenuTitle(mh,spec.name);
						if (IsIMAPMailboxFile(&spec))
						{
							(void) GetMailbox(&spec,True);
						}
						else
						{
						if (!AskGraft(spec.vRefNum,spec.parID,&spec))
							(void) GetMailbox(&spec,True);
					}
#else
						if (!AskGraft(spec.vRefNum,spec.parID,&spec))
							(void) GetMailbox(&spec,True);
#endif
					}
					else if (item==MAILBOX_NEW_ITEM-MAILBOX_BAR1_ITEM)
						GetTransferParams(menu,item,&spec,nil);
#endif
					return(True);
				case TRANSFER:
					break;
			}
	}

	return(False);
}


/**********************************************************************
 * DoDependentMenu - handle a menu selection whose behavior depends
 * on what window is frontmost.  returns True if item handled, False else
 **********************************************************************/
Boolean DoDependentMenu(WindowPtr topWinWP,int menu,int item,short modifiers)
{
	MyWindowPtr	topWin = GetWindowMyWindowPtr (topWinWP);
#ifdef THREADING_ON
#ifndef BATCH_DELIVERY_ON
	if((menu==SPECIAL_MENU) && (item==SPECIAL_FILTER_ITEM) && modifiers&optionKey)
		return False;
#endif		
#endif		
	if (topWinWP != nil)
	{
		if (IsPlugwindow(topWinWP))
			return(PlugwindowMenu(topWinWP,(menu<<16)|item));
		else if ((GetWindowKind(topWinWP)>userKind ||
			 GetWindowKind(topWinWP)==dialogKind) &&
			topWin->menu)
			return((*(topWin)->menu)(topWin,menu,item,modifiers));
	}
	return(False);
}


/**********************************************************************
 * MenuItemIsSeparator - returns true if the given item in the given
 * menu is a divider line
 **********************************************************************/
Boolean MenuItemIsSeparator(MenuHandle theMenu, short item)
{
	Str255 itemString;

	if (!theMenu) return false;
	GetMenuItemText(theMenu, item, itemString);
	return (itemString[0] && (itemString[1] == '-')) ? true : false;
}


/**********************************************************************
 * EnableMenuItems - enable menu items, depending on the type and state of
 * the topmost window
 **********************************************************************/
void EnableMenuItems(Boolean all)
{
	int kind;
	Boolean dirty = False, selection = False;
	MenuHandle mh;
	WindowPtr winWP = MyFrontWindow();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	WindowPtr nextWin;
	Boolean hasTx=False, ro=False;
	Boolean scrapFull;
	Boolean outside = False;
	Boolean curMessage = False;
	Boolean force = False;
	Boolean compRo = False;
	Boolean html = False;
	Boolean composeHTML = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
	Boolean plugwindow = IsPlugwindow(winWP);
	Boolean hasNickScan = False;
	short field=0;
	PETEHandle pte=nil;
	Str63 txt;
	long transFlags;
#ifdef TWO
	short itm,n;
	MenuHandle txtSubMenuHdl;
	short itemNo, numTxtSubMenuItems;
	Boolean textOK;
	Boolean hasPreview = false;
	Boolean hasDrawer = false;
	TOCHandle tocH = NULL;
#endif
	
	/*
	 * figure out just what's in front
	 */
	if (winWP==(WindowPtr)(-1)) {winWP = nil; force = True;}
	if (winWP==nil)
	{
		kind=0;
		dirty=False; selection=False; hasTx=False; ro = True;
		
		if (keyFocusedFloater)
		{
			hasTx = true;
			selection = MyWinHasSelection(keyFocusedFloater);
			pte = keyFocusedFloater->pte;
			ro = PETESelectionLocked(PETE,pte,-1,-1);
		}
	}
	else
	{
		kind = GetWindowKind(winWP);
		if (kind >= userKind && kind!=EMS_PW_WINDOWKIND)
		{
			dirty = IsDirtyWindow(win);
			selection = win->hasSelection || MyWinHasSelection(win);
			pte = win->pte;
			hasTx = pte!=nil;
			if (kind==COMP_WIN)
			{
				compRo = SENT_OR_SENDING(SumOf(Win2MessH(win))->state);
				field = CompHeadCurrent(pte);
			}
			else if (kind==PREF_WIN)
				kind = 1;	/* not a normal window */
			else if (kind==MESS_WIN)
			{
				html = MessOptIsSet(Win2MessH(win),OPT_HTML);
			}
			else if (kind==MBOX_WIN || kind==CBOX_WIN)
			{
				tocH = (TOCHandle)GetMyWindowPrivateData(win);
				if (LastMsgSelected(tocH)>=0)
				{
		 			n = FirstMsgSelected(tocH);
		 			ASSERT(n>=0);
					html = 0!=((*tocH)->sums[n].opts&OPT_HTML);
				}
				hasPreview = (*tocH)->previewID && (*tocH)->previewPTE;
				hasDrawer = (*tocH)->drawer;
			}
				
			if (pte)
			{
				ro = PETESelectionLocked(PETE,pte,-1,-1);
			}
		}
		if (IsMyWindow (winWP))
			if (hasNickScan = HasNickScanCapability (win->pte)) {
				hasTx = true;
				ro = PETESelectionLocked (PETE, win->pte, -1, -1);
			}
				
		// figure out what's behind it
		for (nextWin=GetNextWindow(winWP);nextWin;nextWin=GetNextWindow(nextWin))
			if (IsWindowVisible(nextWin)) break;
	}
	
	if (plugwindow) PlugwindowEnable(winWP,&transFlags);
	/*
	 * enable menus accordingly, if anything has changed
	 */
	curMessage = kind==MESS_WIN || kind==COMP_WIN ||
								((kind==MBOX_WIN || kind==CBOX_WIN) && selection);
	outside = curMessage && (kind==MESS_WIN || kind==MBOX_WIN);
	
	if (!HMGetHelpMenuHandle(&mh) && mh) /* the check for nil is needed courtesy of Norton Utilities, crap that it is */
		EnableIf(mh,EndHelpCount-2,all||hasTx&&!ro);
	EnableIf(GetMHandle(APPLE_MENU),APPLE_ABOUT_ITEM,all||!ModalWindow);
	if (ETLExists()) EnableIf(GetMHandle(APPLE_MENU),APPLE_ABOUT_ITEM+1,all||!ModalWindow);
	mh = GetMHandle(FILE_MENU);
	EnableIf(mh,FILE_NEW_ITEM,(all||!ModalWindow));
	EnableIf(mh,FILE_OPENDOC_ITEM,(all||!ModalWindow));
	EnableIf(mh,FILE_CLOSE_ITEM,(all||kind!=0&&kind!=TBAR_WIN&&kind!=PROG_WIN&&!ModalWindow&&(!plugwindow||(transFlags&EMS_MENU_FILE_CLOSE))));
	EnableIf(mh,FILE_OPENSEL_ITEM,all||
		kind==MESS_WIN && AttIsSelected(win,pte,-1,-1,0,nil,nil) || hasTx && urlNot!=URLIsSelected(win,pte,-1,-1,0,nil,nil) ||
		((kind==MBOX_WIN||kind==CBOX_WIN||kind==MB_WIN||kind==PERS_WIN||kind==SIG_WIN||kind==STA_WIN||kind==LINK_WIN||kind==IMPORTER_WIN) && selection));
	EnableIf(mh,FILE_BROWSE_ITEM,all||html);
	EnableIf(mh,FILE_SAVE_ITEM,all||(win!=nil && (CurrentModifiers()&optionKey))||dirty||(plugwindow&&(transFlags&EMS_MENU_FILE_SAVE)));
	EnableIf(mh,FILE_SAVE_AS_ITEM,all||curMessage||kind==TEXT_WIN||IsSearchWindow(winWP)||kind==ALIAS_WIN);
	EnableIf(mh,FILE_IMPORT_MAIL_ITEM,gImportersAvailable && (all||!ModalWindow));
	EnableIf(mh,FILE_PAGE_ITEM,(all||!ModalWindow));
	EnableIf(mh,FILE_PRINT_ITEM,all||curMessage||kind==PH_WIN||kind==TEXT_WIN||kind==ALIAS_WIN||kind==FILT_WIN||kind==STAT_WIN);
	EnableIf(mh,FILE_PRINT_ONE_ITEM,all||curMessage||kind==PH_WIN||kind==TEXT_WIN||kind==ALIAS_WIN||kind==FILT_WIN||kind==STAT_WIN);

	mh = GetMHandle(WINDOW_MENU);
	EnableIf(mh,WIN_MINIMIZE_ITEM,all||win!=nil&&kind!=TBAR_WIN&&kind!=AD_WIN);
	
	EnableIf(mh,WIN_BEHIND_ITEM,all||win!=nil&&nextWin!=nil&&GetWindowKind(nextWin)!=TBAR_WIN);
	EnableIf(mh,WIN_DRAWER_ITEM,all||kind==MBOX_WIN||kind==CBOX_WIN);
	SetItemMark(mh,WIN_DRAWER_ITEM,hasDrawer?checkMark:0);
	
	mh = GetMHandle(EDIT_MENU);
	if ((win && kind<userKind) || (plugwindow&&(transFlags&EMS_MENU_EDIT_UNDO)))
	{
		Str63 scratch;
		GetRString(scratch,UNDO);
		SetMenuItemText(mh,EDIT_UNDO_ITEM,scratch);
		EnableIf(mh,EDIT_UNDO_ITEM,(all||win!=nil||plugwindow) && !ModalWindow);
	}
	else
		SetUndoMenu(win);
	if (all) EnableItem(mh,EDIT_UNDO_ITEM);
	EnableIf(mh,EDIT_CUT_ITEM,(all||win && (kind<userKind || selection && hasTx && !ro && field!=ATTACH_HEAD)) || (plugwindow&&(transFlags&EMS_MENU_EDIT_CUT)));
#ifdef ONE
	EnableIf(mh,EDIT_COPY_ITEM,(all||win && (kind<userKind || selection&&hasTx&&field!=ATTACH_HEAD)) || (plugwindow&&(transFlags&EMS_MENU_EDIT_COPY)));
#else
	EnableIf(mh,EDIT_COPY_ITEM,(all||win && (kind<userKind||selection&&field!=ATTACH_HEAD)) || (plugwindow&&(transFlags&EMS_MENU_EDIT_COPY)));
#endif
	scrapFull = IsScrapFull();
	textOK = TextMenuOK(win);
	EnableIf(mh,EDIT_PASTE_ITEM,(all||win && (scrapFull &&
			(!ro && hasTx || kind==PH_WIN) || kind<userKind)) || (plugwindow&&(transFlags&EMS_MENU_EDIT_PASTE)));
	EnableIf(mh,EDIT_QUOTE_ITEM,all||scrapFull && !ro && hasTx && (kind!=COMP_WIN || !CompHeadCurrent(pte)));
	EnableIf(mh,EDIT_CLEAR_ITEM,(all||win && (kind<userKind || selection && (!ro || kind==COMP_WIN))) || (plugwindow&&(transFlags&EMS_MENU_EDIT_CLEAR)));
	EnableIf(mh,EDIT_WRAP_ITEM,all||win && selection && !ro && hasTx && textOK);
	EnableIf(mh,EDIT_SELECT_ITEM,(all||win && !plugwindow && kind!=PICT_WIN && (hasTx||kind!=FILT_WIN) && kind!=TBAR_WIN) || (plugwindow&&(transFlags&EMS_MENU_EDIT_SELECTALL)));
	EnableIf(mh,EDIT_INSERT_TO_ITEM,all||hasTx&&!ro);
	EnableIf(mh,EDIT_INSERT_EMOTICON_ITEM,all||hasTx&&!ro);
	EnableIf(mh,EDIT_FINISH_ITEM,all||hasNickScan && (hasTx&&!ro));
	EnableIf(mh,EDIT_TEXT_ITEM,all||textOK||TFBMenusAllowed(win));
	EnableIf(mh,EDIT_SPELL_ITEM,all||!ModalWindow);
#ifdef SPEECH_ENABLED
//	EnableIf(mh,EDIT_SPEAK_ITEM,SpeechAvailable () && (all||kind==MESS_WIN || kind==COMP_WIN
//									|| (kind==MBOX_WIN || kind==CBOX_WIN)&&selection));
	EnableIf(mh,EDIT_SPEAK_ITEM,SpeechAvailable () && (all
									|| (kind==MESS_WIN || kind==COMP_WIN)
									|| ((kind==MBOX_WIN || kind==CBOX_WIN) && selection)
									|| (hasTx&& selection)));
#else
	EnableIf(mh,EDIT_SPEAK_ITEM,false);
#endif
	EnableIf(mh,EDIT_PROCESS_ITEM,all||(hasTx||hasPreview)&&!compRo);
#ifdef WINTERTREE
	EnableIf(mh,EDIT_ISPELL_HOOK_ITEM,all||hasTx&&HaveSpeller());
	EnableSpellMenu(all);
#endif
#ifdef DIAL
	EnableIf(mh,EDIT_DIAL_ITEM,all|| selection&&hasTx);
#endif
	n = CountMenuItems(mh);
	for (itm=EDIT_SPELL_ITEM+1;itm<=n;itm++)
	{
#ifdef WINTERTREE
		SetItemCmd(mh,itm,'6'+itm-EDIT_SPELL_ITEM);
#else
		SetItemCmd(mh,itm,'5'+itm-EDIT_SPELL_ITEM);
#endif
		EnableIf(mh,itm,all||hasTx/*&&!ro*/);
	}
	
#ifdef DEBUG
	{
		extern Boolean gPLDownloadInProgress;
		EnableIf(GetMHandle(AD_SELECT_HIER_MENU),1/*so sue me*/,!gPLDownloadInProgress);
	}
#endif

	EnableIf(GetMHandle(TEXT_HIER_MENU),0,all||textOK||TFBMenusAllowed(win));
	EnableIf(GetMHandle(FONT_HIER_MENU),0,all||textOK);
	EnableIf(GetMHandle(COLOR_HIER_MENU),0,all||textOK);
	mh = GetMHandle(MARGIN_HIER_MENU);
	EnableIf(mh,0,all||textOK);
	EnableIf(mh,CountMenuItems(mh),all||textOK&&composeHTML);
	EnableIf(GetMHandle(TLATE_SEL_HIER_MENU),0,all||(hasTx||hasPreview)&&!compRo);

	txtSubMenuHdl=GetMHandle(TEXT_HIER_MENU);
	numTxtSubMenuItems=CountMenuItems(txtSubMenuHdl);
	for (itemNo=1;itemNo<=numTxtSubMenuItems;itemNo++)
	{
		if (!MenuItemIsSeparator(txtSubMenuHdl,itemNo))
			EnableIf(txtSubMenuHdl,itemNo,all||textOK);
	}
	if (textOK && !all)
	{
		EnableIf(txtSubMenuHdl,tmURL,curMessage && composeHTML && URLOkHere(pte));
		EnableIf(txtSubMenuHdl,tmRule,curMessage && composeHTML);
		EnableIf(txtSubMenuHdl,tmGraphic,curMessage && composeHTML);
	}

	if (kind==MBOX_WIN || kind==CBOX_WIN) CheckSortItems(win);	
	
	EnableHelpItems();
	
	EnableFindMenu(all);
			
	mh = GetMHandle(MESSAGE_MENU);
	GetRString(txt,PrefIsSet(PREF_REPLY_ALL)?REPLY_ALL:REPLY);
	SetMenuItemText(mh,MESSAGE_REPLY_ITEM,txt);
	EnableIf(mh,MESSAGE_REPLY_ITEM,all||outside);
	EnableIf(mh,MESSAGE_FORWARD_ITEM,all||kind==MESS_WIN || kind==COMP_WIN
									|| (kind==MBOX_WIN || kind==CBOX_WIN)&&selection);
	EnableIf(mh,MESSAGE_REDISTRIBUTE_ITEM,all||outside);
	EnableIf(mh,MESSAGE_SALVAGE_ITEM,all||kind==MESS_WIN || kind==COMP_WIN
									|| (kind==MBOX_WIN || kind==CBOX_WIN)&&selection);
	EnableIf(mh,MESSAGE_REPLY_WITH_ITEM,HasFeature(featureStationery)&&(all||outside));
	EnableIf(GetMHandle(REPLY_WITH_HIER_MENU),0,HasFeature(featureStationery)&&(all||outside));
	EnableIf(mh,MESSAGE_FORWARD_TO_ITEM,all||kind==MESS_WIN || kind==COMP_WIN
									|| (kind==MBOX_WIN || kind==CBOX_WIN)&&selection);
	EnableIf(mh,MESSAGE_REDIST_TO_ITEM,all||outside);
	EnableIf(mh,MESSAGE_QUEUE_ITEM,all||
										kind==COMP_WIN || kind==CBOX_WIN&&selection);
	EnableIf(mh,MESSAGE_CHANGE_ITEM,all||curMessage);
	EnableIf(mh,MESSAGE_DELETE_ITEM,all||curMessage);
	EnableIf(mh,MESSAGE_JUNK_ITEM,HasFeature(featureJunk)&&(all||JunkItemsEnable(win,false)));
	EnableIf(mh,MESSAGE_NOTJUNK_ITEM,HasFeature(featureJunk)&&(all||JunkItemsEnable(win,true)));
	EnableIf(mh,MESSAGE_ATTACH_ITEM,all||kind==COMP_WIN&&!compRo);
	EnableIf(mh,MESSAGE_ATTACH_TLATE_ITEM,all||kind==COMP_WIN&&!compRo);
	EnableIf(GetMHandle(TLATE_ATT_HIER_MENU),0,all||kind==COMP_WIN&&!compRo);
	EnableIf(GetMHandle(TLATE_SET_HIER_MENU),0,all||!ModalWindow);
	GetRString(txt,PrefIsSet(PREF_AUTO_SEND)?SEND_M_ITEM:QUEUE_M_ITEM);
	SetMenuItemText(mh,MESSAGE_QUEUE_ITEM,txt);
	mh = GetMHandle(CHANGE_HIER_MENU);

	EnableIf(mh,0,curMessage||all);
	EnableIf(mh,CHANGE_QUEUEING_ITEM,
										all||kind==COMP_WIN || kind==CBOX_WIN&&selection);
#ifdef TWO
	EnableIf(mh,CHANGE_STATUS_ITEM,all||curMessage);
#endif
	EnableIf(mh,CHANGE_PRIOR_ITEM,all||curMessage);
	EnableIf(mh,CHANGE_LABEL_ITEM,all||curMessage);
	EnableIf(mh,CHANGE_PERS_ITEM,HasFeature(featureMultiplePersonalities)&&(all||(curMessage&&PersCount()>1)));
		
	// I dunno why we did this SD 6/23/04
	// if (EnableIf(GetMHandle(TRANSFER_MENU),0,all||curMessage||DirtyHackForChooseMailbox)) DrawMenuBar();

	mh = GetMHandle(SPECIAL_MENU);
#ifdef CTB
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_CTB_ITEM),all||UseCTB);
#endif
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_FORGET_ITEM),all || 
		(PersAnyPasswords() || PrefIsSet(PREF_KERBEROS) || KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN)) && !SendThreadRunning && !CheckThreadRunning);
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_MAKE_NICK_ITEM),all||curMessage||CanMakeNick());
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_MAKEFILTER_ITEM),all||curMessage);
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_SORT_ITEM),all||kind==MBOX_WIN||kind==CBOX_WIN);
	if (kind==MBOX_WIN||kind==CBOX_WIN)
		EnableIf(GetMHandle(SORT_HIER_MENU),SORT_MAILBOX_ITEM,all||(*(TOCHandle)GetMyWindowPrivateData(win))->virtualTOC);
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_TLATE_SETTINGS_ITEM),all||!ModalWindow&&ETLExists());
#ifdef TWO
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM),all||curMessage);
	EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_CHANGE_ITEM),all||!PrefIsSet(PREF_KERBEROS));
#endif
#ifdef THREADING_ON
#ifndef BATCH_DELIVERY_ON
	if (CurrentModifiers()&optionKey)
		EnableIf(mh,AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM),all||((NeedToFilterIn && !CheckThreadRunning) || (NeedToFilterOut && !SendThreadRunning)));
#endif
#endif

	if (kind==COMP_WIN || kind==CBOX_WIN) SetQItemText(win);

	/*
	 * this is a maverick menu item
	 */
#ifdef THREADING_ON
	EnableIf(GetMHandle(FILE_MENU),FILE_SEND_ITEM,(all||(!ModalWindow && SendQueue>0 && !SendImmediately)));
	EnableIf(GetMHandle(FILE_MENU),FILE_CHECK_ITEM,(all||(!ModalWindow && !CheckThreadRunning && !(PrefIsSet(PREF_POP_SEND) && SendThreadRunning))));
#else
	EnableIf(GetMHandle(FILE_MENU),FILE_SEND_ITEM,(all||(SendQueue>0)&&!ModalWindow));
#endif

	EnableServerItems(win,-1,all || (CurrentModifiers()&optionKey),((CurrentModifiers()&shiftKey)!=0));
	CheckPrior(win,all);
	
	// Preferences menu for MacOS X
	if (HaveTheDiseaseCalledOSX())
	{
		if (all|| (!all && !ModalWindow))
			EnableMenuCommand (NULL, kHICommandPreferences);
		else
			DisableMenuCommand (NULL, kHICommandPreferences);
	}
		
	if (win && win->menuEnable)
		(*win->menuEnable)(win);
}
#ifdef TWO

/**********************************************************************
 * EnableHelpItems - enable the help menu items
 **********************************************************************/
void EnableHelpItems(void)
{
	MenuHandle mh;
	short itm;
	Str255 name;
	
	if (!HMGetHelpMenuHandle(&mh) && OriginalHelpCount)
	{
		for (itm = EndHelpCount-1;itm>OriginalHelpCount;itm--)
		{
			if (ModalWindow) DisableItem(mh,itm);
			else
			{
				GetMenuItemText(mh,itm,name);
				if (*name!=1 || name[1]!='-') EnableItem(mh,itm);
			}
		}
	}
}
#endif


/**********************************************************************
 * MailboxKindaMenu - is this a mailbox kind of menu?
 **********************************************************************/
short MailboxKindaMenu(short mnu,short item,PStr s,FSSpecPtr spec)
{
	Boolean xfer;
	
	if (spec) *spec->name = 0;
	if (IsMailboxChoice(mnu,item))
	{
		GetTransferParams(mnu,item,spec,&xfer);
		return(xfer ? TRANSFER_MENU:MAILBOX_MENU);
	}
	return(mnu);
}



#pragma segment MenuMain
/**********************************************************************
 * EnableMenus - enable menus, depending on the type and state of
 * the topmost window
 **********************************************************************/
void EnableMenus(WindowPtr winWP,Boolean all)
{
	Boolean mBar = False;
	MyWindowPtr win = GetWindowMyWindowPtr (winWP);
	Boolean curMessage = False;
	short kind;
	MenuHandle mh = nil;
	
	/*
	 * figure out just what's in front
	 */
	if (winWP!=nil)
	{
		kind = GetWindowKind(winWP);
		if (kind >= userKind)
		{
			
			switch (kind) {
				case MESS_WIN:
				case COMP_WIN:
					curMessage = true;
					break;
				case MBOX_WIN:
				case CBOX_WIN:
					if (win) {
						TOCHandle	tocH = (TOCHandle) GetMyWindowPrivateData (win);
						if (!((*tocH)->hasFileView && (*tocH)->fileView) && !SearchViewIsMailbox(tocH))
						if (win->hasSelection || (tocH && (*tocH)->previewID))
							curMessage = true;
					}
					break;
			}
		}
	}
		
	/*
	 * enable menus accordingly, if anything has changed
	 */
	mBar=EnableIf(mh = GetMHandle(FILE_MENU),0,!NoMenus && (all||!ModalWindow||AreAllModalsPlugwindows()))||mBar;
	mBar=EnableIf(GetMHandle(APPLE_MENU),0,True)||mBar;
	mBar=EnableIf(GetMHandle(MAILBOX_MENU),0,!NoMenus && (all||!ModalWindow))||mBar;
	mBar=EnableIf(GetMHandle(EDIT_MENU),0,!NoMenus && !(win&&win->noEditMenu) && (all||XfUndoH||IsPlugwindow(winWP)||win!=nil&&!ProgressIsOpen()))||mBar;
	mBar=EnableIf(GetMHandle(MESSAGE_MENU),0,!NoMenus && (all||!ModalWindow))||mBar;
	mBar=EnableIf(GetMHandle(TRANSFER_MENU),0,!NoMenus && (all||curMessage||DirtyHackForChooseMailbox))||mBar;
	mBar=EnableIf(GetMHandle(SPECIAL_MENU),0,!NoMenus && (all||!ModalWindow))||mBar;
	mBar=EnableIf(GetMHandle(WINDOW_MENU),0,!NoMenus && (all||!ModalWindow))||mBar;
	SetWindowPropMenu(win,true,all,checkMark,CurrentModifiers());
		
	/*
	 * draw the menu bar if that's necessary
	 */
	if (mBar)
	{
		DrawMenuBar();
		TBForceIdle();
	}
}	

/************************************************************************
 * EnableIf - enable a menu item based on an expression.
 * Returns True is the menu bar needs to be redrawn.
 ************************************************************************/
Boolean EnableIf(MenuHandle mh, short item, Boolean expr)
{ 
	Boolean result;
	short mark; 

	if (!mh) return(False);
	
	expr = expr ? 1 : 0;					/* normalize */
	result = item==0 && ((IsMenuItemEnabled(mh,0))!=(expr));
	
	if (expr)
		EnableItem(mh,item);
	else
		DisableItem(mh,item);
	
	/*
	 * if submenu, able that
	 */
	if (item && HasSubmenu(mh,item))
	{
		mark = SubmenuId(mh,item);
		EnableIf(GetMHandle(mark),0,expr);
	}
	
	return (result);
}


/**********************************************************************
 * TextMenuOK - is it ok to show the text menu?
 **********************************************************************/
Boolean TextMenuOK(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	
	if (!IsMyWindow(winWP)) return(False);
	if (!win->pte) return(False);
	if (PETESelectionLocked(PETE,win->pte,-1,-1)) return(False);
	switch(GetWindowKind(winWP))
	{
		case COMP_WIN: return(!CompHeadCurrent(win->pte));
		case MESS_WIN: return(win->pte==(*Win2MessH(win))->bodyPTE);
		case TEXT_WIN: return(True);
		default: return(False);
	}
}


#pragma segment Ends
/************************************************************************
 *
 ************************************************************************/
Boolean SameSettingsFile(short vRef,long dirId,UPtr name)
{
#pragma unused(dirId)
	FSSpec spec;
	
	if (GetFileByRef(SettingsRefN,&spec)) return(False);
	
	return(vRef == Root.vRef && dirId == Root.vRef && StringSame(name,spec.name));
}
#pragma segment BuildMenus

/**********************************************************************
 * NewMessageWith - start a new message with a bit of stationery
 **********************************************************************/
OSErr NewMessageWith(short item)
{
	MyWindowPtr win;
	
	//Stationery - no support for this in Light
	if (HasFeature (featureStationery)) {
		win = DoComposeNew(nil);
		if (!win) return(1);
		ApplyIndexStationery(win,item,True,True);
		ShowMyWindow(GetMyWindowWindowPtr(win));
	}
	return(0);
}

/**********************************************************************
 * 
 **********************************************************************/
void TextMenuChoice(short menu, short item,short modifiers)
{
	MyWindowPtr	win = GetWindowMyWindowPtr (MyFrontWindow());
	PETEHandle pte;
	PETETextStyle ps;
	PETEStyleEntry pse;
	Str255 font;
	MCEntryPtr mc;
	PETEParaInfo pinfo;
	long textValid=0;
	long paraValid=0;
	Boolean hasInsert;
	long pStart, pStop;
	Boolean clearGraphic = false;
	
	pte = win ? win->pte : nil;
	if (PeteIsValid(pte))
	{
		PeteGetTextAndSelection(pte,nil,&pStart,&pStop);
		hasInsert = pStart==pStop;
		PeteStyleAt(pte,(pStart==pStop)?kPETECurrentStyle:kPETECurrentSelection,&pse);
		ps = pse.psStyle.textStyle;
		switch(menu)
		{
			case TEXT_HIER_MENU:
				if (item==tmPlain)
				{
					if (modifiers&optionKey)
					{
						PeteStyleAt(pte,kPETEDefaultStyle,&pse);
						ps = pse.psStyle.textStyle;
						textValid = peAllValid;
						pinfo.tabHandle = nil;
						PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&pinfo);
						paraValid = peAllParaValid & ~peTabsValid;
						if (UseFlowOutExcerpt) paraValid &= ~peQuoteLevelValid;
						clearGraphic = true;
					}
					else
					{
						ps.tsFace = 0;
						textValid = bold|italic|underline|outline|shadow|condense|extend;
					}
				}
				else if (item<tmBar1)
				{
					if (ps.tsFace&(1<<(item-2)))
						ps.tsFace &= ~(1<<(item-2));
					else
						ps.tsFace |= 1<<(item-2);
					textValid = (1<<(item-2));
				}
				else if (tmLeft<=item && item<=tmRight)
				{
					UseFeature (featureStyleJust); 
					pStart = pStop = -1;	// use selection
					PeteParaRange(pte,&pStart,&pStop);
					PeteParaConvert(pte,pStart,pStop);
					pinfo.justification = teFlushDefault;
					paraValid = peJustificationValid;
					switch(item)
					{
						case tmLeft:
							pinfo.justification = teFlushLeft;
							break;
						case tmCenter:
							pinfo.justification = teCenter;
							break;
						case tmRight:
							pinfo.justification = teFlushRight;
							break;
					}
				}
				else if (item==tmQuote || item==tmUnquote) {
					UseFeature (featureStyleQuote);
					IncrementQuoteLevel(pte,-1,-1,item==tmQuote?1:-1);
				}
				else if (item==tmURL)
				{
					UseFeature (featureStyleLink);
					InsertURL(pte);
					return;
				}
				else if (item==tmRule)
				{
					UseFeature (featureStyleHorzRule);
					PeteInsertRule(pte,-1,0,0,true,true,true);
					return;
				}
				else if (item==tmGraphic)
				{
					CompAttach(win,true);
					return;
				}
				break;
			
			case TEXT_SIZE_HIER_MENU:
#ifdef USERELATIVESIZES
				ps.tsSize = kPETERelativeSizeBase+item-tfbSizeNormal;
#else
				ps.tsSize = IncrementTextSize(FontSize,item-tfbSizeNormal);
#endif
				textValid = peSizeValid;
				break;
		
			case FONT_HIER_MENU:
				GetMenuItemText(GetMHandle(FONT_HIER_MENU),item,font);
				ps.tsFont = GetFontID(font);
				if (ps.tsFont==FontID) ps.tsFont = kPETEDefaultFont;
#ifdef USEFIXEDDEFAULTFONT
				if (ps.tsFont==FixedID) ps.tsFont = kPETEDefaultFixed;
#endif
				textValid = peFontValid;
				break;
				
			case COLOR_HIER_MENU:
				if (ThereIsColor)
				{
					UseFeature (featureStyleColor);
					if (item==1 || !(mc = GetMCEntry(COLOR_HIER_MENU,item)))
						DEFAULT_COLOR(ps.tsColor);
					else
						ps.tsColor = mc->mctRGB2;
					textValid = peColorValid;
				}
				break;
			
			case MARGIN_HIER_MENU:
				pStart = pStop = -1;	// use selection
				PeteParaRange(pte,&pStart,&pStop);
				PeteParaConvert(pte,pStart,pStop);
				MarginMenuChoice(pte,item,&pinfo,&paraValid);
				break;
		}
	}
	if (!hasInsert && paraValid && textValid)	PetePrepareUndo(pte,peUndoStyleAndPara,-1,-1,nil,nil);
	if (clearGraphic) PeteClearGraphic(pte,-1,-1);
	if (paraValid) PETESetParaInfo(PETE,pte,-1,&pinfo,paraValid);
	if (textValid) PETESetTextStyle(PETE,pte,-1,-1,&ps,textValid);
	if (!hasInsert && paraValid && textValid)	PeteFinishUndo(pte,peUndoStyleAndPara,-1,-1);
	PeteSetDirty(pte);
	if (paraValid) PeteWhompHiliteRegionBecausePeteWontFixIt(pte);
	if (textValid || paraValid) (*PeteExtra(pte))->quoteScanned = 0;
}

/**********************************************************************
 * MarginMenuChoice - apply a margin style
 **********************************************************************/
void MarginMenuChoice(PETEHandle pte,short item,PETEParaInfoPtr pinfo,long *paraValid)
{
	PeteSaneMargin marg;
	Str255 scratch;
	short width = GetRLong(TAB_DISTANCE)*FontWidth;
	MenuHandle mh = GetMHandle(MARGIN_HIER_MENU);
	
	*paraValid = 0;
	Zero(*pinfo);
	PETEGetParaInfo(PETE,pte,-1,pinfo);
	
	if (item==CountMenuItems(mh))
	{
		UseFeature (featureStyleBullet);
		GetCurEditorMargins(pte,&marg);
		if (pinfo->paraFlags & peListBits)
		{
			pinfo->paraFlags &= ~peListBits;
			marg.first--;
			marg.second--;
			// It would look better right-indented, but Pete didn't do it that way
			// marg.right--;
		}
		else
		{
			pinfo->paraFlags |= peDiskList;
			marg.first++;
			marg.second++;
			// It would look better right-indented, but Pete didn't do it that way
			// marg.right++;
		}
		*paraValid = peFlagsValid;
	}
	else
	{
		UseFeature (featureStyleMargin);
		if (Ascii2Margin(GetRString(scratch,MarginStrn+item),nil,&marg)) return;
		if (pinfo->paraFlags & peListBits)
		{
			marg.first++;
			marg.second++;
			marg.right++;
		}
	}
		
	*paraValid |= peIndentValid|peStartMarginValid|peEndMarginValid;
	
	PeteConvertMarg(pte,kPETEDefaultPara,&marg,pinfo);
}

/**********************************************************************
 * 
 **********************************************************************/
void SetMenuTexts(short modifiers,Boolean allMenu)
{
	WindowPtr	winWP = MyFrontWindow();
	MyWindowPtr win = GetWindowMyWindowPtr(winWP);
	short kind = winWP?GetWindowKind(winWP) : 0;
	Boolean option = (modifiers & optionKey)!=0;
	Boolean shift = (modifiers & shiftKey)!=0;
	Boolean command = (modifiers & cmdKey)!=0;
	Boolean select = IsMyWindow(winWP) && win && win->hasSelection;
	Str255 scratch, suffix;
	MenuHandle mh;
	Boolean all = PrefIsSet(PREF_REPLY_ALL);
	Boolean turbo = PrefIsSet(PREF_TURBO_REDIRECT);
#ifdef	IMAP
	Boolean imap = IMAPExists();
	TOCHandle tocH = nil;
	
	if (win && kind == MBOX_WIN) tocH = (TOCHandle)GetMyWindowPrivateData(win);
#endif
	
	/*
	 * check the mailbox represented by the topmost window
	 */
	CheckBox(win,allMenu);
#ifdef TWO
	CheckState(win,allMenu,0);
	CheckLabel(win,allMenu);
	CheckPers(win,HasFeature(featureMultiplePersonalities)&&allMenu);
#endif
	CheckTable(win,allMenu);
	/*
	 * build the Windows menu
	 */
	BuildWindowsMenu(allMenu);
	
	/*
	 * file menu
	 */
	mh = GetMHandle(FILE_MENU);
	SetItemR(mh,FILE_SAVE_ITEM,option?SAVE_ALL_ITEXT:SAVE_ITEXT);
	SetItemR(mh,FILE_CLOSE_ITEM,option?CLOSE_ALL_ITEXT:CLOSE_ITEXT);
	SetItemR(mh,FILE_SAVE_AS_ITEM,shift&&kind==ALIAS_WIN?SAVE_AS_SEL_ITEXT:SAVE_AS_ITEXT);
	SetItemR(mh,FILE_PRINT_ITEM,shift?PRINT_SEL_ITEXT:PRINT_ITEXT);
	SetItemR(mh,FILE_SEND_ITEM,option?SEND_SPECIAL_ITEXT:SEND_ITEXT);

#ifdef	IMAP
	if (tocH && !option && shift && (*tocH)->imapTOC && !PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD)) 
		GetRString(scratch,IMAP_CHECK_SPECIAL_ITEXT);
	else
#endif
		GetRString(scratch,option?CHECK_SPECIAL_ITEXT:CHECK_MAIL);

	SetCheckItem(scratch);
	
	/*
	 * edit menu
	 */
	mh = GetMHandle(EDIT_MENU);
	SetItemR(mh,EDIT_COPY_ITEM,option?
		(shift?COPY_UNWRAP_PLAIN_ITEXT:COPY_UNWRAP_ITEXT):
		(shift?COPY_PLAIN_ITEXT:COPY_ITEXT));
	SetItemR(mh,EDIT_PASTE_ITEM,(PrefIsSet(PREF_PASTE_PLAIN)?!shift:shift)?PASTE_PLAIN_ITEXT:PASTE_ITEXT);
	SetItemR(mh,EDIT_WRAP_ITEM,option?UNWRAP_ITEXT:WRAP_ITEXT);
	SetItemR(mh,EDIT_FINISH_ITEM,HasFeature(featureNicknameWatching)&&option?FINISH_EXP_ITEXT:FINISH_ITEXT);
	SetItemR(mh,EDIT_INSERT_TO_ITEM,HasFeature(featureNicknameWatching)&&option?INSERT_EXP_ITEXT:INSERT_ITEXT);
#ifdef SPEECH_ENABLED
	SetItemR(mh,EDIT_SPEAK_ITEM,shift?SPEAK_SEL_ITEXT:SPEAK_ITEXT);
#endif	
	
	SetMenuItemBasedOnFeature (mh, EDIT_FINISH_ITEM, featureNicknameWatching, -1);
	SetMenuItemBasedOnFeature (mh, EDIT_SPEAK_ITEM, featureSpeak, -1);
	SetMenuItemBasedOnFeature (mh, EDIT_ISPELL_HOOK_ITEM, featureSpellChecking, -1);

	mh=GetMHandle(TEXT_HIER_MENU);
	SetMenuItemBasedOnFeature (mh, tmQuote, featureStyleQuote, -1);
	SetMenuItemBasedOnFeature (mh, tmUnquote, featureStyleQuote, -1);
	SetMenuItemBasedOnFeature (mh, tmLeft, featureStyleJust, -1);
	SetMenuItemBasedOnFeature (mh, tmCenter, featureStyleJust, -1);
	SetMenuItemBasedOnFeature (mh, tmRight, featureStyleJust, -1);
	SetMenuItemBasedOnFeature (mh, tmMargin, featureStyleMargin, -1);
	SetMenuItemBasedOnFeature (mh, tmColor, featureStyleColor, -1);
	SetMenuItemBasedOnFeature (mh, tmRule, featureStyleHorzRule, -1);
	SetMenuItemBasedOnFeature (mh, tmGraphic, featureStyleGraphics, -1);
	SetMenuItemBasedOnFeature (mh, tmURL, featureStyleLink, -1);
	
	/*
	 * message menu
	 */
	mh = GetMHandle(MESSAGE_MENU);
	GetRString(scratch,REPLY);
	*suffix = 0;
	if (shift) PCatR(suffix,REP_SELECTION);//select?REP_SELECTION:REP_NOSEL);
	if (all!=option) PCatR(suffix,REP_ALL);
	PCat(scratch,suffix);
	SetMenuItemText(mh,MESSAGE_REPLY_ITEM,scratch);
	if (HasFeature (featureStationery)) {
	PCatR(scratch,REP_WITH);
	SetMenuItemText(mh,MESSAGE_REPLY_WITH_ITEM,scratch);
	}
	SetMenuItemBasedOnFeature (mh, MESSAGE_JUNK_ITEM, featureJunk, -1);
	SetMenuItemBasedOnFeature (mh, MESSAGE_NOTJUNK_ITEM, featureJunk, -1);
	
	*scratch = 0;
	if (turbo!=option) GetRString(scratch,TURBO);
	PCatR(scratch,REDIRECT);
	if ((turbo!=option) && shift) PCatR(scratch,TB_REDIR_NO_DEL);
	PCatR(scratch,TO_ITEXT);
	SetMenuItemText(mh,MESSAGE_REDIST_TO_ITEM,scratch);
	
	SetItemR(mh,MESSAGE_QUEUE_ITEM,option?CHANGE_QUEUEING:
							(PrefIsSet(PREF_AUTO_SEND)?SEND_M_ITEM:QUEUE_M_ITEM));
	SetItemR(mh,MESSAGE_DELETE_ITEM,option&&shift?NUKE_ITEXT:DELETE_ITEXT);
	
	SetMenuItemBasedOnFeature (mh, MESSAGE_NEW_WITH_ITEM, featureStationery, -1);
	SetMenuItemBasedOnFeature (mh, MESSAGE_REPLY_WITH_ITEM, featureStationery, -1);
	
	/*
	 * special
	 */
	mh = GetMHandle(SPECIAL_MENU);
	SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_MAKE_NICK_ITEM),
					 shift&&select?MAKE_SEL_NICK_ITEXT:MAKE_NICK_ITEXT);
	SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_SORT_ITEM),option?SORT_DESCEND_ITEXT:SORT_ITEXT);
#ifdef THREADING_ON
#ifndef BATCH_DELIVERY_ON
	SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM),option?FILTER_WAITING_ITEXT:FILTER_ITEXT);
#endif
#endif

#ifdef	IMAP
	// an imap personality exists
	if (imap)
	{
		// There's a frontmost window, and it's a mailbox
		if (tocH)
		{
			// if fancy trash is off for this mailbox, use the REMOVE_DELETED_MESSAGE string
			if ((*tocH)->imapTOC && !FancyTrashForThisPers(tocH))
			{
				SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_TRASH_ITEM),(option&!shift?IMAP_REMOVE_DELETED_MESSAGES_ITEXT:IMAP_EMPTY_TRASH));
			}
			else
			{
				// fancy trash is on, or the frontmost mailbox is a pop mailbox ...
				
				short text = (*tocH)->imapTOC ? IMAP_EMPTY_CURRENT_TRASH : IMAP_EMPTY_LOCAL_TRASH;
				
				SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_TRASH_ITEM),(shift&!option?text:(option&!shift?IMAP_EMPTY_REMOTE_TRASH:IMAP_EMPTY_TRASH)));
			}
		}
		else
		{
			// There's no toc frontmost.  Use "Empty Trash for Selected Personalities" if we ought to
			SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_TRASH_ITEM),HasFeature(featureMultiplePersonalities)&&DoWeUseSelectedIMAPPersonalities()?IMAP_EMPTY_PERS_TRASH:IMAP_EMPTY_TRASH);

			// if option is pressed, but not shift, use "Empty All Trash Mailoxes"
			if (option && !shift) SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_TRASH_ITEM),IMAP_EMPTY_REMOTE_TRASH);
		}
	}
#endif

	//	Use special items for change/forget password if personalities window is open		 
	if ( ChangePWMenuShown ())
		SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_CHANGE_ITEM),HasFeature(featureMultiplePersonalities)&&DoWeUseSelectedPersonalities()?CHANGE_PERS_PW:CHANGE_PW);
	SetItemR(mh,AdjustSpecialMenuItem(SPECIAL_FORGET_ITEM),HasFeature(featureMultiplePersonalities)&&DoWeUseSelectedPersonalities()?FORGET_PERS_PW:FORGET_PW);

	//	Enable plugin special menu items for current user state
	ETLEnableSpecialItems();
	
	/*
	 * text?
	 */
	CheckTextItems(win,modifiers,allMenu);		//Styled Text - don't allow user to compose or send styled text	

	/*
	 * windows menu
	 */
	mh = GetMHandle (WINDOW_MENU);
	SetMenuItemBasedOnFeature (mh, WIN_PERSONALITIES_ITEM, featureMultiplePersonalities, 1);
	SetMenuItemBasedOnFeature (mh, WIN_STATIONERY_ITEM, featureStationery, 1);
	SetMenuItemBasedOnFeature (mh, WIN_LINK_ITEM, featureLink, 1);
	SetMenuItemBasedOnFeature (mh, WIN_STATISTICS_ITEM, featureStatistics, 1);
	SetMenuItemBasedOnFeature (mh, WIN_DRAWER_ITEM, featureMBDrawer, 1);

	// contextual search
	if (mh = GetMHandle(FIND_HIER_MENU))
	{
		Str255 mItemStr;
		
		// for now, we just search the web
		GetRString(suffix,SEARCH_FOR_WEB);
		
		// if we have a string, show it in the menu
		if (win && win->pte && !AttIsSelected(nil,win->pte,-1,-1,0,nil,nil) && *PeteSelectedString(scratch,win->pte) && !IsAllLWSP(scratch))
		{
			CollapseLWSP(scratch);
			*scratch = MIN(*scratch,63);
			ComposeRString(mItemStr,SEARCH_FOR_FMT,suffix,scratch);
		}
		// if no string, just say search web
		else 
			ComposeRString(mItemStr,SEARCH_NOTHING_FMT,SEARCH_FOR_WEB);
		
		// update the item
		SetMenuItemText(mh,FIND_SEARCH_WEB_ITEM,mItemStr);
	}
	
	SetWindowPropMenu(win,false,false,checkMark,modifiers);
#ifdef DEBUG
	CheckCurrentAd();
#endif
}

/**********************************************************************
 * CheckTextItems - check the appropriate items in the text menu & submenus
 **********************************************************************/
void CheckTextItems(MyWindowPtr win,short modifiers,Boolean all)
{
	PETETextStyle ps;
	PETEStyleEntry pse;
	MenuHandle mh;
	Str255 fontName;
	short item;
	PETEParaInfo pinfo;

	mh = GetMHandle(TEXT_HIER_MENU);
	CheckNone(mh);

	if (!TextMenuOK(win)) return;
		
	PeteStyleAt(win->pte,kPETECurrentStyle,&pse);
	ps = pse.psStyle.textStyle;
	if (ps.tsSize==kPETEDefaultSize) ps.tsSize = FontSize;
	if (ps.tsFont==kPETEDefaultFont) ps.tsFont = FontID;
#ifdef USEFIXEDDEFAULTFONT
	if (ps.tsFont==kPETEDefaultFixed) ps.tsFont = FixedID;
#endif
	
	if (!ps.tsFace)
	{
		if (!(modifiers&optionKey)) SetItemMark(mh,tmPlain,checkMark);
	}
	else
	{
		if (ps.tsFace&bold) SetItemMark(mh,tmBold,checkMark);
		if (ps.tsFace&italic) SetItemMark(mh,tmItalic,checkMark);
		if (ps.tsFace&underline) SetItemMark(mh,tmUnderline,checkMark);
	}
	
	SetItemR(mh,tmPlain,
						(modifiers&optionKey)?PLAIN_ALL_ITEM:PLAIN_TEXT_MITEM);
	
	mh = GetMHandle(TEXT_SIZE_HIER_MENU);
#ifdef USERELATIVESIZES
	if(ps.tsSize < 0)
		item = ps.tsSize - kPETERelativeSizeBase;
 	else
#endif
	item = FindSizeInc(ps.tsSize)-FindSizeInc(FontSize);
	if (item<-3) item = -3;
	else if (item>3) item = 3;
	CheckNone(mh);
	SetItemMark(mh,tfbSizeNormal+item,checkMark);
						
	mh = GetMHandle(FONT_HIER_MENU);
	CheckNone(mh);
	GetFontName(ps.tsFont,fontName);
	if (item=FindItemByName(mh,fontName))
		SetItemMark(mh,item,checkMark);
	
	Zero(pinfo);
	PETEGetParaInfo(PETE,win->pte,-1,&pinfo);
	mh = GetMHandle(MARGIN_HIER_MENU);
	SetItemMark(mh,CountMenuItems(mh),(pinfo.paraFlags & peListBits) ? checkMark:0);
	
}

/************************************************************************
 * SetQItemText - set the text of the Queue For Delivery menu item
 ************************************************************************/
void SetQItemText(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH = nil;
	int sumNum = -1;
	
	if (GetWindowKind(winWP)==COMP_WIN)
	{
		tocH = (*(MessHandle)GetMyWindowPrivateData(win))->tocH;
		sumNum = (*(MessHandle)GetMyWindowPrivateData(win))->sumNum;
	}
	else if (GetWindowKind(winWP)==CBOX_WIN)
	{
		tocH = (TOCHandle)GetMyWindowPrivateData(win);
		for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
		if ((*tocH)->sums[sumNum].selected) break;
	}
	if (tocH && sumNum >= 0 && sumNum < (*tocH)->count)
	{
#ifdef THREADING_ON
		if ((*tocH)->sums[sumNum].state==SENT || ((*tocH)->sums[sumNum].state==BUSY_SENDING))
#else
		if ((*tocH)->sums[sumNum].state==SENT)		
#endif
		{
			DisableItem(GetMHandle(MESSAGE_MENU),MESSAGE_QUEUE_ITEM);
			DisableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_QUEUEING_ITEM);
		}
		else
		{
			EnableItem(GetMHandle(MESSAGE_MENU),MESSAGE_QUEUE_ITEM);
			EnableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_QUEUEING_ITEM);
		}
	}
}
/**********************************************************************
 * CheckTable - check the proper translit table
 **********************************************************************/
void CheckTable(MyWindowPtr win,Boolean all)
{
	MenuHandle mh = GetMHandle(TABLE_HIER_MENU);
	TOCHandle tocH;
	Boolean isOut = False;
	short tableId;
	
	if (all && mh)
	{
		EnableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_TABLE_ITEM);
		CheckNone(mh);
		return;
	}
	
	if (win && mh && NewTables)
	{
	  switch (GetWindowKind(GetMyWindowWindowPtr (win)))
	  {
	  	case COMP_WIN:
	  		isOut = True;
	  	case MESS_WIN:
	  		tableId = SumOf(Win2MessH(win))->tableId;
	  		break;
	  	case CBOX_WIN:
	  		isOut = True;
	  	case MBOX_WIN:
	  		if (!win->hasSelection && !all)
	  		{
	  			DisableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_TABLE_ITEM);
	  			return;
	  		}
	  		else
	  		{
	  			tocH = (TOCHandle)GetMyWindowPrivateData(win);
	  			tableId = (*tocH)->sums[FirstMsgSelected(tocH)].tableId;
	  		}
  	}
	  		
		TrashMenu(mh,1);
		AddXlateTables(isOut,tableId,False,mh);
		EnableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_TABLE_ITEM);
	}
	else if (mh) DisableItem(GetMHandle(CHANGE_HIER_MENU),CHANGE_TABLE_ITEM);
}

#ifdef TWO
/************************************************************************
 * CheckLabel - put a check next to the current message's label
 ************************************************************************/
void CheckLabel(MyWindowPtr win,Boolean all)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MenuHandle mh = GetMHandle(LABEL_HIER_MENU);
	short n;
	
	if (!mh) return;
	
	/*
	 * uncheck old one
	 */
	CheckNone(mh);
	if (all) return;
	
	if (IsMyWindow(winWP))
	{
		MessHandle messH = (MessHandle) GetMyWindowPrivateData(win);
		TOCHandle tocH = (TOCHandle) GetMyWindowPrivateData(win);
		
		/*
		 * set n to current color
		 */
		n = REAL_BIG;
		if (IsMessWindow(winWP))
			n = SumColor(SumOf(messH));
		else if (GetWindowKind(winWP)==MBOX_WIN && win->hasSelection)
		{
		  n = FirstMsgSelected(tocH);
			n = GetSumColor(tocH,n);
		}
		else return;
		
		/*
		 * turn into item number
		 */
		n = Label2Menu(n);
		
		/*
		 * mark it
		 */
		SetItemMark(mh,n,checkMark);
	}
}

/************************************************************************
 * CheckState - put a check next to the current message's state
 ************************************************************************/
void CheckState(MyWindowPtr win,Boolean all,short origItem)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MenuHandle mh = GetMHandle(STATE_HIER_MENU);
	MessHandle messH = win?(MessHandle) GetMyWindowPrivateData(win):nil;
	TOCHandle tocH = win?(TOCHandle) GetMyWindowPrivateData(win):nil;
	short n;
	Boolean out=False;
	short kind = winWP ? GetWindowKind(winWP) : 0;
	
	/*
	 * uncheck old one
	 */
	CheckNone(mh);
	for (n=CountMenuItems(mh);n;n--)
		if (n!=statmBar) EnableItem(mh,n);
	DisableItem(mh,statmUnsendable);
	DisableItem(mh,statmTimed);
	DisableItem(mh,statmBusySending);
	
	if (all)
	{
		EnableItem(mh,0);
		return;
	}
	
	if (IsMyWindow(winWP))
	{
		/*
		 * set n to current state
		 */
		n = 0;
		if (kind==MESS_WIN || kind==COMP_WIN)
		{
			n = SumOf(messH)->state;
			EnableItem(mh,0);
			out = kind==COMP_WIN;
		}
		else if ((kind==MBOX_WIN || kind==CBOX_WIN) && win->hasSelection)
		{
			n = (*tocH)->sums[FirstMsgSelected(tocH)].state;
			EnableItem(mh,0);
			out = kind==CBOX_WIN;
		}
		else
		{
			DisableItem(mh,0);
			return;
		}
		
		/*
		 * check the appropriate item
		 */
		if (n>0) SetItemMark(mh,Status2Item(n),checkMark);
		
		/*
		 * disable the appropriate items
		 */
		if (!out || n==SENT)
		{
			DisableItem(mh,statmSendable);
			DisableItem(mh,statmQueued);
		}
	}
	else SetItemMark(mh,origItem,diamondChar);
}
#endif

/************************************************************************
 * CheckPrior - put a check next to the current message's priority
 ************************************************************************/
void CheckPrior(MyWindowPtr win,Boolean all)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MenuHandle mh = GetMHandle(PRIOR_HIER_MENU);
	MessHandle messH = win?(MessHandle) GetMyWindowPrivateData(win):nil;
	TOCHandle tocH = win?(TOCHandle) GetMyWindowPrivateData(win):nil;
	short n;
	
	/*
	 * uncheck old one
	 */
	EnableItem(mh,0);
	CheckNone(mh);
	if (all)
	{
		EnableItem(mh,pymLower);
		EnableItem(mh,pymRaise);
		return;
	}
	
	if (IsMyWindow(winWP))
	{
		/*
		 * set n to current state
		 */
		n = REAL_BIG;
		if (IsMessWindow(winWP))
			n = Prior2Display(SumOf(messH)->priority);
		else if (GetWindowKind(winWP)==MBOX_WIN && win->hasSelection)
		{
		  n = FirstMsgSelected(tocH);
			n = Prior2Display((*tocH)->sums[n].priority);
		}
		else return;
				
		/*
		 * mark it
		 */
		if (n) SetItemMark(mh,n,checkMark);
		EnableIf(mh,pymLower,n!=pymLowest);
		EnableIf(mh,pymRaise,n!=pymHighest);
	}
}

/**********************************************************************
 * CheckBox - check the topmost mailbox, and disable in transfer menu
 **********************************************************************/
void CheckBox(MyWindowPtr win,Boolean all)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH;
	Str31 name;
	short n;
	FSSpec	spec;

	if (CheckedMenu && CheckedItem)
	{
		SetItemMark(CheckedMenu,CheckedItem,0);
		CheckedMenu = nil;
	}
	if (all) return;
			
	if (!IsMyWindow(winWP)) return;
	if (GetWindowKind(winWP)==MBOX_WIN)
		tocH = (TOCHandle) GetMyWindowPrivateData(win);
	else if (GetWindowKind(winWP)==MESS_WIN)
		tocH = (*(MessHandle) GetMyWindowPrivateData(win))->tocH;
	else
	{
		if (GetWindowKind(winWP)==CBOX_WIN ||
					 GetWindowKind(winWP)==COMP_WIN)
			SetItemMark(CheckedMenu=GetMHandle(MAILBOX_MENU),
								CheckedItem=MAILBOX_OUT_ITEM,checkMark);
		return;
	}
	
	if ((*tocH)->which==IN)
		SetItemMark(CheckedMenu=GetMHandle(MAILBOX_MENU),
								CheckedItem=MAILBOX_IN_ITEM,checkMark);
	else if ((*tocH)->which==TRASH)
		SetItemMark(CheckedMenu=GetMHandle(MAILBOX_MENU),
								CheckedItem=MAILBOX_TRASH_ITEM,checkMark);
	else if ((*tocH)->which==OUT)
		SetItemMark(CheckedMenu=GetMHandle(MAILBOX_MENU),
								CheckedItem=MAILBOX_OUT_ITEM,checkMark);
	else if ((*tocH)->which==JUNK)
		SetItemMark(CheckedMenu=GetMHandle(MAILBOX_MENU),
								CheckedItem=MAILBOX_JUNK_ITEM,checkMark);
	else
	{
		spec = GetMailboxSpec(tocH,-1);
		if ((*tocH)->imapTOC)	// look for enclosing folder if this is an IMAP cache mailbox
		{	
			short menu, item;
			
			if (noErr==Spec2Menu(&spec, false, &menu, &item))
			{
				CheckedMenu=GetMHandle(menu?menu:MAILBOX_MENU);
				CheckedItem=item;
				SetItemMark(CheckedMenu,CheckedItem,checkMark);
			}
		}
		else if (0<=(n=FindDirLevel(spec.vRefNum,spec.parID)))
		{
			PCopy(name,spec.name);
			CheckedMenu=GetMHandle(n?n:MAILBOX_MENU);
			CheckedItem=FindItemByName(CheckedMenu,name);
			SetItemMark(CheckedMenu,CheckedItem,checkMark);
		}
	}
}

/**********************************************************************
 * 
 **********************************************************************/
void InsertSystemConfig(PETEHandle pte)
{
	Accumulator a;
	long start;
	
	if (!AccuInit(&a) && !AddConfig(&a,False))
	{
		CompReallyPreferBody((*PeteExtra(pte))->win);
		PetePrepareUndo(pte,peUndoPaste,-1,-1,&start,nil);
		if (PETEInsertText(PETE,pte,-1,a.data,nil))
		{
			AccuZap(a);
			PeteKillUndo(pte);
		}
		else PeteFinishUndo(pte,peUndoPaste,start,-1);
	}
}

/************************************************************************
 * SetCheckItem - put the time of the next mail check in the Check Mail
 * item.
 ************************************************************************/
void SetCheckItem(PStr item)
{
	short n;
	uLong checkTicks = PersCheckTicks();
	
	if (checkTicks && AutoCheckOK())
	{
		checkTicks = MAX(checkTicks,TickCount()+3600);
		PCat(item,"\p ");
		n = *item+1;
		TimeString((LocalDateTime()-TickCount()/60)+checkTicks/60,False,item+n,nil);
		item[0] += item[n]+1;
		item[n] = '(';
		PCatC(item,')');
	}
	SetMenuItemText(GetMHandle(FILE_MENU),FILE_CHECK_ITEM,item);
}

/************************************************************************
 * InsertRecipient - insert a given recipient in a window
 ************************************************************************/
void InsertRecipient(MyWindowPtr win, short which,Boolean all)
{
	Str63 scratch;
	long start;

	NicknameWatcherFocusChange (win->pte); /* MJN */

	//Undo.didClick=True;
	PetePrepareUndo(win->pte,peCantUndo,-1,-1,&start,nil);
	InsertCommaIfNeedBe(win->pte,nil);
	MyGetItem(GetMHandle(NEW_TO_HIER_MENU),which,scratch);
	PeteInsertPtr(win->pte,-1,scratch+1,*scratch);
	if (all && HasNickExpansion (win->pte))
//	if (all && ((*PeteExtra(win->pte))->nick.fieldCheck ? (*(*PeteExtra(win->pte))->nick.fieldCheck) (win->pte) : HasNickExpansion (win->pte)))
		FinishAlias(win->pte,all,False, true);
	PeteFinishUndo(win->pte,peUndoPaste,start,-1);

//	NicknameWatcherMaybeModifiedField (win->pte); /* MJN */
}


/************************************************************************
 * BuildWindowsMenu
 ************************************************************************/
void BuildWindowsMenu(Boolean allMenu)
{
	Str255 title;
	WindowPtr winWP;
	MyWindowPtr	win;
	MenuHandle mh = GetMHandle(WINDOW_MENU);
	RGBColor color;
	
	if (mh)
	{
		TrashMenu(mh,WIN_LIMIT);
		for (winWP=FrontWindow();winWP;winWP=GetNextWindow(winWP))
			if (WinWPWindowsMenuEligible(winWP))
			{
				win = GetWindowMyWindowPtr (winWP);
				GetWTitle(winWP,title);
				if (*title>31)
				{
					*title = 31;
					title[31] = '�';
				}
				MyAppendMenu(mh,title);
				if (allMenu) DisableItem(mh,CountMenuItems(mh));
				if (GetWindowKind(winWP)==COMP_WIN)
					SetItemStyle(mh,CountMenuItems(mh),italic);
				if (IsMyWindow(winWP) && ThereIsColor && win->label)
					SetMenuColor(mh,CountMenuItems(mh),GetLabelColor(win->label,&color));
				if (GetWindowKind(winWP)==MBOX_WIN || GetWindowKind(winWP)==CBOX_WIN)
				{
					if ((*(TOCHandle)GetMyWindowPrivateData(win))->unread) SetItemStyle(mh,CountMenuItems(mh),UnreadStyle);
				}
			}
	}
}

/************************************************************************
 * TrashMenu - get rid of a menu, beginning at a particular item
 ************************************************************************/
void TrashMenu(MenuHandle mh, short beginAtItem)
{
	short count,id,item;
	MenuHandle subMh;
	Byte type;
	Handle oldSuite;
	
	if (!mh) return;
	for (count=CountMenuItems(mh),item=beginAtItem?beginAtItem:1;item<=count;item++)
	{
		oldSuite = nil;
		if (!GetMenuItemIconHandle(mh,item,&type,&oldSuite) && type==kMenuIconSuiteType && oldSuite)
			DisposeIconSuite(oldSuite,true);

		if (HasSubmenu(mh,item))
		{
			id = SubmenuId(mh,item);
			subMh = GetMHandle(id);
			TrashMenu(subMh,0);
			SetItemCmd(mh,item,0);
			DeleteMenu(id);
			DisposeMenu(subMh);
		}
	}
	if (beginAtItem>0)
		while (count>=beginAtItem) DeleteMenuItem(mh,count--);
}

#ifdef USECMM
#pragma mark -
#pragma mark ---CMM Stuff---
/*************************************CMM Stuff ***************************************************/
/************************************************************************************************
What's left to do with implementing the Contextual Menu Manager?
Here's the scoop:

First, to turn CMM use on, you need to define USECMM in conf.h

Within the contextual menu, there are two groups of items:
1>	There are universal, non-context-specific (but hopefully unversally relevant) items which are
	always present in the contextual menu.
2>	There are context-specific items which are present depending on where the context click
	occurs and conditions at the time of the click.
	
Implementation of the first group is pretty much done.  Which items are included in this group
may be easily adjust by editing the static array called 'basicCMenuInfo', which is defined at the
top of the DoHandleContextualMenuClick function.  This array is a list of items to be added to
the contextual menu, designated by menu id and menu item number.  The list is terminated by an
entry whose menu id is CONTEXT_MENU and whose item number is CONTEXT_TERM_ITEM.

Implementation of the second group is only just begun.  The framework is as follows:

I added a field to the MyWindowStruct data structure called buildCMenu.  buildCMenu is a pointer
to a function whic should describe which context-specific items to add for a context click in
that window.  Two examples below for the Message and Composition window are MessBuildCMenu and
CompBuildCMenu.  If buildCMenu is nil for a window, then no context-specific menu items are shown
for context clicks in that window.

In addition, the general case of selecting text in a window and then context-clicking in that
window has not been addressed yet.  Once this is addressed, several items from the edit menu
should be appended to any context-specific menu.  The function to append these menu items has
already been written, though it is not called anywhere just yet.  It is the AppendEditCMenu
function below and should be passed the handle-style array of context-specific items for a window
once that array has been created and it is deemed that text is selected.

One final note is that selected items in general need to be passed through the contextual menu
manager's facilities for processing various pre-defined data types (i.e. Apple Data Detectors).
This is the superset of the text selection situation described in the paragraph above.  This can
be acocmplished by passing the appropriate AEDesc to the ContextualMenuSelect function, where I
am presently passing a nil.

Known problems:
couldn't figure out how to detect if a context-click had occurred in the toolbar, so when a
context-click occurs there, the current front window is deactivated and I attempt to make the
toolbar the currently active window.  Obviously, this is not an especially good thing, but I'm
sure it is easy to fix for someone with a more thorough understanding of the source than I have.

-- Forest Hill 1/27/98 <mailto:alm_hillf@carleton.edu>

************************************************************************************************/
void HandleContextualMenuClick( WindowPtr topWinWP, EventRecord* event )
{
	switch( DoHandleContextualMenuClick( topWinWP, event ) )
	{
		case noErr:
		case userCanceledErr:
			break;
		default:
			SysBeep( 10 );
	}
}

/************************************************************************************************
DoHandleContextualMenuClick - handles contextual menu click events.  Needs to be passed pointers
to the top window and to the contextual menu click event.
************************************************************************************************/
OSStatus DoHandleContextualMenuClick( WindowPtr topWinWP, EventRecord* event )
{
	MyWindowPtr		win;
	MyWindowPtr		topWin;
	WindowPtr			winWP;						//window where the click happened
	OSStatus			err = noErr;						//for returning errors
	UInt32				outSelectionType;			//what type of selection happened in the contextual menu?
	SInt16				outMenuID;					//which menu was the item selected from (probably the contextual menu)
	UInt16				menuItem;					//which item was selected
	short					winPart;					//which part of a window did the click occur in?
	MenuHandle			contextMenu;				//handle to the ocntextual menu
	uLong res;			// result of menu select
	AEDesc selectionDesc;
	MenuHandle	mh;		// handle to the menu that contains the selected item
	
	NullADList(&selectionDesc,nil);

	//which window (and part) did the click occur in?
	winPart = FindWindow_( event->where, &winWP );

	//if it wasn't the current top window, then makeit so
	if( winWP != topWinWP )
	{
		if( winWP )
		{
			SelectWindow_( winWP );
			UsingWindow( winWP );
			UpdateMyWindow(winWP);
			NotUsingWindow( winWP );
		}
		topWinWP = winWP;
	}

	//make sure menu items are correctly enabled and disabled, and that the texts are correct
	EnableMenus(winWP,false);
	EnableMenuItems(false);
	SetMenuTexts(CurrentModifiers(),false);
	
	//down, boy!
	//scamwatch was leaving its tooltip up over the menu
	MyHMHideTag();

	//create the contextual menu
	contextMenu = NewMenu( CONTEXT_MENU, "\p" );
	if( !contextMenu ) return( MemError() );

	//insert it as a hierarchical menu
	InsertMenu( contextMenu, hierMenu );

#ifdef USECMM_BAD
	//build the basic menu items
	err = BuildBasicContextMenu( contextMenu );
	if( err ){ goto oops; }
#endif
	
	//we are now using this window! nobody else touch it!
	UsingWindow( topWinWP );

	//add on the context-specific menus, if there are any
	topWin = GetWindowMyWindowPtr (topWinWP);
	if (IsMyWindow(topWinWP) && topWin && topWin->buildCMenu)
		err = (*topWin->buildCMenu)(topWin,event,contextMenu);
	if( err ) goto oops;
	
	//add edit items, if any
	ASSERT(topWin);
	if (topWin)
	{
		err = AppendEditItems(topWin,event,contextMenu);
		if( err ){ goto oops; }
	}
	
	// got some text?
	//err = CreateTEHObj(topWinWP,&selectionDesc);
	win = GetWindowMyWindowPtr (winWP);
	if (IsMyWindow(topWinWP) && win && win->pte)
	{
		long startSel, endSel;
		Handle text;

		PeteGetTextAndSelection(win->pte,&text,&startSel,&endSel);
		err = AECreateDesc(typeChar,LDRef(text)+startSel,endSel-startSel,&selectionDesc);
		UL(text);
	}
	else err = errAENoSuchObject;

	MightSwitch();

	//display the menu and allow the user to select items from it
	err = ContextualMenuSelect( contextMenu, event->where, false, kCMHelpItemNoHelp, "\p", err ? nil: &selectionDesc,
		&outSelectionType, &outMenuID, &menuItem );
		
	AfterSwitch();
	
	if( err ) goto oops;

	// In which menu does the selected item actually live in?
	mh = GetMenuHandle(outMenuID);

	//respond to the item selected, if any
	switch( outSelectionType )
	{
		case kCMNothingSelected:
		case kCMShowHelpSelected:
			break;
		//if a menu items was selected
		case kCMMenuItemSelected:
			GetMenuItemCommandID(mh,menuItem,&res);
			if (MailboxKindaMenu((res>>16)&0xffff,res&0xffff,nil,nil)) UseFeature(featureContextualFiling);
			DoMenu(topWinWP, res, event->modifiers);
			break;
	}
	
oops:
	//we're done using the window
	NotUsingWindow( topWinWP );

	//get rid of the contextual menu
	DeleteMenu( CONTEXT_MENU );
 	DisposeMenu( contextMenu );
 	DisposeADList(&selectionDesc,nil);
	
	return( err );
}

#ifdef NEVER
/************************************************************************************************
BuildBasicContextMenu - builds the basic universal-items-only contextual menu, given an array of
CMenuInfo's whose last element contains the values CONTEXT_MENU and CONTEXT_TERM_ITEM.
************************************************************************************************/
OSStatus BuildBasicContextMenu( MenuHandle contextMenu, CMenuInfo* basicCMenuInfo )
{
	MenuHandle	srcMenu;		//source menu for desired items to add to contextual menu
	short		i,				//counter
				startItems;		//number of items in the menu to begin with
	Str255		itemString;		//string for transfering menu items to contextual menu
	
	//find out how many items there are to begin with ( most likely 0 )
	startItems = CountMenuItems( contextMenu );
	
	//walk through until we get to the CONTEXT_TERM_ITEM
	for( i=0; !( basicCMenuInfo[i].srcMenuID == CONTEXT_MENU && basicCMenuInfo[i].srcMenuItem == CONTEXT_TERM_ITEM ); i++ )
	{
		//get a handle to the specified source menu for this item
		srcMenu = GetMenuHandle( basicCMenuInfo[i].srcMenuID );
		if( !srcMenu ) return( nilHandleErr );

		//if it's a separator item, then add a separator
		if( basicCMenuInfo[i].srcMenuID == CONTEXT_MENU && basicCMenuInfo[i].srcMenuItem == CONTEXT_SEPARATOR_ITEM )
			AppendMenu( contextMenu, "\p-" );
		//otherwise, add the specific item
		else
		{
			//get the text for the desired item
			GetMenuItemText( srcMenu, basicCMenuInfo[i].srcMenuItem, itemString );

			//add a new item to the context menu
			AppendMenu( contextMenu, "\pitem" );

			//set the text for the new menu item
			SetMenuItemText( contextMenu, startItems+i+1, itemString );

			//if the item in the original menu is disabled, then disable the item in the context menu also
			if( basicCMenuInfo[i].srcMenuItem < 32 && !( (**srcMenu).enableFlags & ( 0x00000001 << basicCMenuInfo[i].srcMenuItem ) ) )
				DisableItem( contextMenu, startItems+i+1 );
		}
	}
	
	//we had no problems
	return( noErr );
}
#endif

#pragma mark -
#ifdef USECMM_BAD
/************************************************************************************************
AppendEditCMenu - appends items from the edit menu to a CMenuInfo handle-style array.  THIS CALL
IS NOT IN USE UNTIL TEXT-SENSITIVE CONTEXTS CAN BE DISCERNED.
************************************************************************************************/
OSStatus AppendEditCMenu( CMenuInfoHndl* specCMenuInfo )
{
	short				numItems,				//number of items to add to the contextual menu
						start;					//starting point in the CMenuInfo list
	OSStatus			err;					//for returning errors
	static CMenuInfo	editCMInfo[] =			//items to add to the contextual menu list
		{	CONTEXT_MENU,	CONTEXT_SEPARATOR_ITEM,	//separator line
			EDIT_MENU,		EDIT_CUT_ITEM,			//"cut" item
			EDIT_MENU,		EDIT_COPY_ITEM,			//"copy" item
			EDIT_MENU,		EDIT_PASTE_ITEM,		//"paste" item
			EDIT_MENU,		EDIT_QUOTE_ITEM,		//"paste as quotation" item
			EDIT_MENU,		EDIT_CLEAR_ITEM,		//"clear" item
			EDIT_MENU,		EDIT_SELECT_ITEM,		//"select all" item
			EDIT_MENU,		EDIT_WRAP_ITEM,			//"wrap selection" item
			CONTEXT_MENU,	CONTEXT_TERM_ITEM };	//item to mark end of array
	
	//make sure we got a valid handle-array to begin with
	if( !(*specCMenuInfo) )
		return( nilHandleErr );
	
	//count how many items there are to be added
	for( numItems=0; !( editCMInfo[numItems].srcMenuID == CONTEXT_MENU && editCMInfo[numItems].srcMenuItem == CONTEXT_TERM_ITEM ); numItems++ )
		;
	
	//figure out where to start adding items in the array
	for( start=0; !( (**specCMenuInfo)[start].srcMenuID == CONTEXT_MENU && (**specCMenuInfo)[start].srcMenuItem == CONTEXT_TERM_ITEM ); start++ )
		;

	//adjust the size of the array
	SetHandleSize( (Handle)*specCMenuInfo, GetHandleSize( (Handle)*specCMenuInfo ) + sizeof( CMenuInfo ) * ( numItems + 1 ) );
	err = MemError(); if( err ) return( err );

	//add the items to the list
	BlockMove( editCMInfo, &(**specCMenuInfo)[start], numItems+1 );

	return( noErr );
}
#endif
#endif

/************************************************************************
 * MyEnableItem - override EnableItem to support > 31 items if available
 ************************************************************************/
#undef EnableItem
void MyEnableItem(MenuHandle mh,short item)
{
	EnableMenuItem(mh,item);
}

/************************************************************************
 * MyDisableItem - override DisableItem to support > 31 items if available
 ************************************************************************/
#undef DisableItem
void MyDisableItem(MenuHandle mh,short item)
{
	DisableMenuItem(mh,item);
}
#define DisableItem MyDisableItem

/************************************************************************
 * AddSelectionAsTo - add the current selection to the recipient menus
 ************************************************************************/
void AddSelectionAsTo(void)
{
	MyWindowPtr win = GetWindowMyWindowPtr (FrontWindow_());
	Str255 scratch;
	
	if (!win->pte) return;
	
	AddStringAsTo(PeteSelectedString(scratch,win->pte));
}

/************************************************************************
 * AddStringAsTo - add a string to the recipient menus
 ************************************************************************/
void AddStringAsTo(UPtr string)
{ 
	short spot,result;
	MenuHandle mh;
	Str127 oldItem;
	short menu;

	if (EmptyRecip) spot = 1;
	else
	{
		mh = GetMHandle(NEW_TO_HIER_MENU);
		for (spot=1;spot<=CountMenuItems(mh);spot++)
		{
			MyGetItem(mh,spot,oldItem);
			result = StringComp(string,oldItem);
			if (result==0) return;
			if (result<0) break;
		}
	}
		
	for (menu=NEW_TO_HIER_MENU;menu<=INSERT_TO_HIER_MENU;menu++)
	{
		if (EmptyRecip) DeleteMenuItem(GetMHandle(menu),1);
		MyInsMenuItem(GetMHandle(menu),string,spot-1);
	}
	
	EmptyRecip = False;
	
	ToMenusChanged();
}

/************************************************************************
 * RemoveFromTo - remove something from the recipient menus
 ************************************************************************/
void RemoveFromTo(UPtr name)
{
	short menu,item;
	
	if (item=BinFindItemByName(GetMHandle(NEW_TO_HIER_MENU),name))
	{
		EmptyRecip = CountMenuItems(GetMHandle(NEW_TO_HIER_MENU))==1;
		for (menu=NEW_TO_HIER_MENU;menu<=INSERT_TO_HIER_MENU;menu++)
			if (EmptyRecip)
			{
				SetItemR(GetMHandle(menu),item,NO_RECIPS);
				DisableMenuItem(GetMHandle(menu),item);
			}
			else DeleteMenuItem(GetMHandle(menu),item);
		ToMenusChanged();
	}
}

/************************************************************************
 * ToMenusChanged - see that the recipient menus get written out
 ************************************************************************/
void ToMenusChanged(void)
{
	SaveRecipMenu(GetMenuHandle(NEW_TO_HIER_MENU));
}

/**********************************************************************
 * FixRecipMenu - fix old recip menus
 **********************************************************************/
void FixRecipMenu(void)
{
	MenuHandle mh = GetMHandle(NEW_TO_HIER_MENU);
	Str255 str, scratch;
	short item;
	Str31 verboten;
	UPtr key, spot, end;
	Str31 nickname, candidate;
	short digit;
	Handle h;
	Boolean changed = False;

	GetRString(verboten,ALIAS_VERBOTEN);
	end = verboten+*verboten+1;

	if (!RegenerateAllAliases(False) && !EmptyRecip)
	{
		for (item=CountMenuItems(mh);item;item--)
		{
			MyGetItem(mh,item,str);
			for (key=str+1;key<=str+*str;key++)
				for (spot=verboten+1;spot<end;spot++)
					if (*spot==*key)
					{
						PSCopy(scratch,str);
						BeautifyFrom(scratch);
						SanitizeFN(nickname,scratch,NICK_BAD_CHAR,NICK_REP_CHAR,false);
						digit = 0;
						if (IsAnyNickname(nickname))
						{
							*nickname = MIN(27,*nickname);
							do
							{
								NumToString(++digit,scratch);
								PCopy(candidate,nickname);
								PSCat(candidate,scratch);
							}
							while (IsAnyNickname(candidate));
							PCopy(nickname,candidate);
						}
						h = NuHTempBetter(*str+3);
						if (h)
						{
							RemoveFromTo(str);
							PCopy(*h,str);
							(*h)[*str+1] = 0;
							(*h)[*str+2] = 0;
							NewNickLow(h,nil,0,nickname,True,nrDifferent,True);
							ZapHandle(h);
							item=CountMenuItems(mh)+1;//rescan
						}
						goto nextItem;
					}
			nextItem:;
		}
	}
}

/**********************************************************************
 * SetupRecipMenus - setup the recipient menus
 **********************************************************************/
void SetupRecipMenus(void)
{
	Handle	hRecip = GetResource('STR#',RECIPIENT_STRN);
	
	if (!hRecip)
	{
		//	Check for old recipient menu as MENU resource
		MenuHandle mh;
		if ((mh = GetMenu(NEW_TO_HIER_MENU+1000)) ||
			(mh = GetMenu(NEW_TO_HIER_MENU+900)))
		{
			//	Save as STR# resource
			SaveRecipMenu(mh);
			ReleaseResource(mh);
			hRecip = GetResource('STR#',RECIPIENT_STRN);
		}		
	}	
	SetupRecipMenusWith(hRecip);
}

/**********************************************************************
 * SetupRecipMenusWith - setup the recipient menus using specified data
 **********************************************************************/
void SetupRecipMenusWith(Handle	hRecip)
{
	MenuHandle mh;
	short	menu,itemCnt,i;
	Str63 scratch;
	StringPtr	sItem;

	//	If menus are already set up, delete them and redo them
	for (menu=NEW_TO_HIER_MENU;menu<=INSERT_TO_HIER_MENU;menu++)
		if (mh=GetMenuHandle(menu))
		{
			DeleteMenu(menu);
			DisposeMenu(mh);
		}
	
	//	Create menus
	itemCnt = hRecip ? CountStrnRes(hRecip) : 0;
	EmptyRecip = !itemCnt;
	for (menu=NEW_TO_HIER_MENU;menu<=INSERT_TO_HIER_MENU;menu++)
	{
		switch (menu)
		{
			case NEW_TO_HIER_MENU: MyGetItem(GetMenuHandle(MESSAGE_MENU),MESSAGE_NEW_TO_ITEM,scratch); break;
			case FORWARD_TO_HIER_MENU: MyGetItem(GetMenuHandle(MESSAGE_MENU),MESSAGE_FORWARD_TO_ITEM,scratch); break;
			case REDIST_TO_HIER_MENU: MyGetItem(GetMenuHandle(MESSAGE_MENU),MESSAGE_REDIST_TO_ITEM,scratch); break;
			case INSERT_TO_HIER_MENU: MyGetItem(GetMenuHandle(EDIT_MENU),EDIT_INSERT_TO_ITEM,scratch); break;
		}
		if (!(mh = NewMenu(menu,scratch)))
			DieWithError(GET_MENU,MemError());			
		InsertMenu(mh,-1);
		if (EmptyRecip)
		{
			//	Put in "No Recipients" menu item
			AppendMenu(mh,"\p ");
			SetItemR(mh,1,NO_RECIPS);
			SetItemStyle(mh,1,italic);
			DisableItem(mh,1);
		}
		else
		{
			//	Load up menu
			for(i=1,sItem=LDRef(hRecip)+sizeof(short);i<=itemCnt;i++,sItem+=*sItem+1)
				MyAppendMenu(mh,sItem);
			UL(hRecip);
		}
		CalcMenuSize(mh);
	}

	AttachHierMenu(MESSAGE_MENU,MESSAGE_NEW_TO_ITEM,NEW_TO_HIER_MENU);
	AttachHierMenu(MESSAGE_MENU,MESSAGE_FORWARD_TO_ITEM,FORWARD_TO_HIER_MENU);
	AttachHierMenu(MESSAGE_MENU,MESSAGE_REDIST_TO_ITEM,REDIST_TO_HIER_MENU);
	AttachHierMenu(EDIT_MENU,EDIT_INSERT_TO_ITEM,INSERT_TO_HIER_MENU);
}

/**********************************************************************
 * SaveRecipMenu - save recipient menu to resource
 **********************************************************************/
void SaveRecipMenu(MenuHandle mh)
{
	Handle	hRecip;
	Accumulator	a;
	short	n,i;
	Str255	s;
	
	if (!mh) return;
	
	if (!(hRecip = GetResource('STR#',RECIPIENT_STRN)))
	{
		//	Create a new resource
		hRecip = NuHandleClear(sizeof(short));
		if (!hRecip) return;
		AddResource_(hRecip,'STR#',RECIPIENT_STRN,"");		
	}
	else
		SetHandleSize(hRecip,sizeof(short));
	
	n = CountMenuItems(mh);
	if (n==1 && !IsMenuItemEnabled(mh,1))
		n = 0;	//	Only "no recipients" item
	AccuInitWithHandle(&a,hRecip);
	for(i=1;i<=n;i++)
	{
		GetMenuItemText(mh,i,s);
		AccuAddPtr(&a,s,*s+1);
	}
	AccuTrim(&a);
	**(short**)hRecip = n;
	
	ChangedResource(hRecip);
	WriteResource(hRecip);
}

/**********************************************************************
 * PruneRecipMenu - remove anything that isn't a nickname
 **********************************************************************/
void PruneRecipMenu(void)
{
	Str255 str;
	short item;
	MenuHandle mh = GetMHandle(NEW_TO_HIER_MENU);

	if (mh)
	{
		for (item=CountMenuItems(mh);item;item--)
		{
			MyGetItem(mh,item,str);
			if (!IsAnyNickname(str))
				RemoveFromTo(str);
		}
	}
}

/**********************************************************************
 * HMGetHelpMenuHandle - under Carbon need to setup our own help menu
 **********************************************************************/
OSErr HMGetHelpMenuHandle(MenuRef *mh)
{
	long sysVers;
	static MenuRef	helpMenu;
	
	if (mh==nil)
	{
		// do it all over again!
		helpMenu = nil;
		return noErr;
	}
	
	if (!helpMenu)
	{
		Gestalt(gestaltSystemVersion, &sysVers);
		if (sysVers >= 0x0a00)
		{
			//	Only do this for OS X or greater
			if (helpMenu = GetMenu(HELP_MENU))
				InsertMenu(helpMenu,0);
		}
		else
			HMGetHelpMenu(&helpMenu,nil);		
	}
	*mh = helpMenu;
	return helpMenu ? noErr : -1;
}

static char checkKey = 0;
static char attachKey = 0;
static char minimizeKey = 0;

/**********************************************************************
 * AdjustStupidCommandKeys - OS X steps on cmd-M and cmd-A.  Reassign
 *	the command key for these menu items.
 **********************************************************************/
void AdjustStupidCommandKeys(void)
{
	MenuHandle mHandle;
	short command;
	char key;
	UInt8 modifiers;
	Boolean altCheck = PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD); 
	Boolean altAttach = !PrefIsSet(PREF_NO_ALTERNATE_ATTACH_CMD); 
	
	// Check Mail ...
	mHandle = GetMHandle(FILE_MENU);
	if (!checkKey) 
	{
		GetItemCmd(mHandle,FILE_CHECK_ITEM,&command);
		checkKey = (char)command;
	}
	if (checkKey)
	{
		ReadReplacmentCommandKeyPref(COMMAND_KEY_CHECKMAIL_REPLACEMENT, &key, &modifiers);
		SetItemCmd(mHandle,FILE_CHECK_ITEM,altCheck?key:checkKey);
		SetMenuItemModifiers(mHandle,FILE_CHECK_ITEM,altCheck?modifiers:kMenuNoModifiers);
	}

	// Attach Document ...
	mHandle = GetMHandle(MESSAGE_MENU);
	if (!attachKey) 
	{
		GetItemCmd(mHandle,MESSAGE_ATTACH_ITEM,&command);
		attachKey = (char)command;
	}
	if (attachKey)
	{
		ReadReplacmentCommandKeyPref(COMMAND_KEY_ATTACH_REPLACEMENT, &key, &modifiers);
		SetItemCmd(mHandle,MESSAGE_ATTACH_ITEM,altAttach?key:attachKey);
		SetMenuItemModifiers(mHandle,MESSAGE_ATTACH_ITEM,altAttach?modifiers:kMenuNoModifiers);
	}

	// Minimize Window ...
	mHandle = GetMHandle(WINDOW_MENU);
	if (!minimizeKey) 
	{
		GetItemCmd(mHandle,WIN_MINIMIZE_ITEM,&command);
		minimizeKey = (char)command;
	}
	if (minimizeKey) SetItemCmd(mHandle,WIN_MINIMIZE_ITEM,altCheck?minimizeKey:0);		
}

/**********************************************************************
 * ReadReplacmentCommandKeyPref - read the replacement command key.
 **********************************************************************/
OSErr ReadReplacmentCommandKeyPref(short pref, char *key, UInt8 *modifiers)
{
	OSErr err = noErr;
	Str255 commandKeyPref;
	
	// initialize
	*key = 0;
	*modifiers = kMenuNoModifiers;
	
	GetRString(commandKeyPref, pref);
	
	if (commandKeyPref[0] == 2)
	{
		// first character are the modifers
		if ((commandKeyPref[1] >= '0') && (commandKeyPref[1] <= '8')) *modifiers = commandKeyPref[1] - '0';
	
		// second character is the command key
		*key = commandKeyPref[2];
	}
	else err = 1;
	
	return (err);
}

/**********************************************************************
 * STUPIDCheckMailHack - Make sure cmd-option-M still invokes Check 
 *	Mail Specially.
 **********************************************************************/
long STUPIDCheckMailHack(EventRecord *event)
{
	long select = 0;
	char keyPressed = UnadornMessage(event);
	Boolean optionPressed = ((event->modifiers) & optionKey)!=0;
	char origKey = checkKey;
	char replacementKey;
	UInt8 modifiers;
	
	// if the replacement command doesn't use option, option-replacement will invoke check mail specially.
	if (ReadReplacmentCommandKeyPref(COMMAND_KEY_CHECKMAIL_REPLACEMENT, &replacementKey, &modifiers)==noErr)
	{
		if ((modifiers & optionKey)==0)
		{
			origKey = replacementKey;
		}
	}

	if (isupper(keyPressed)) keyPressed = tolower(keyPressed);
	if (isupper(origKey)) origKey = tolower(origKey);	
		
	if ((keyPressed == origKey) && optionPressed) select = ((FILE_MENU << 16)&0xFFFF0000) + FILE_CHECK_ITEM;
	
	return (select);
}