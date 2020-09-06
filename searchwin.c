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

#include <Devices.h>
#include <Resources.h>

#include <conf.h>
#include <mydefs.h>

#ifdef NEWFIND
#include "searchwin.h"
#include "regexp.h"
#define FILE_NUM 114
#pragma segment Find
/* Copyright (c) 1998 by QUALCOMM Incorporated */

//	constants
enum
{
	kMaxCriteria = 16,		// should be enough
	kWinHeaderHt = 99,		// height of window header excluding criteria
	kCriteriaHt = 27,		// height of 1 criteria line
	kMBHdrHt = 18,			// height of mailbox header control
	kFrontWinTicks = 60,	// max ticks allowed when search window is in front, hog CPU time but exit for any events
	kBgdWinTicks = 10,		// max ticks allowed when search window is not in front
	kInBGTicks = 4,			// max ticks allowed when Eudora is in background
	kChaseArrowTicks = 15,	// call idle function for chase arrows 4 times a second
	
	kDateWi = 100,			//	Date control width
	kDateHt = 22,			//	Date control height (must be 20)

	kIOBufSize = 32 K,		//	Size of bulk search I/O buffers
	kSpecListType = 'Spec'
};

// criteria category menu items
enum { scAnywhere=1,scHeaders,scBody,scAttachNames,scDiv1,
	scSummary,scStatus,scJunkScore,scPriority,scAttachCount,scLabel,scDate,scSize,scAge,
	scPersonality,
	scDiv2,scTo,scFrom,scSubject,scCC,scBCC,scAnyRecipient,scLimit };

// We save the criteria differently from how they appear in the menus.
// Map saved categories to the ones we use in the menu
char SavedCategory2MenuCategory[] = 
	{0,1,2,3,4,5,6,7,9,10,11,12,13,14,15,16,17,18,19,20,21,22,8};

#define SafeSavedCategory2MenuCategory(x) ((x)<scLimit ? SavedCategory2MenuCategory[x]:1)

// Map the categories in the menu to the ones we save
char MenuCategory2SavedCategory[] = 
	{0,1,2,3,4,5,6,7,22,8,9,10,11,12,13,14,15,16,17,18,19,20,21};

enum { ssText=1,ssDate,ssAge,ssNumber };
enum { sauMinutes=1,sauHours,sauDays,sauWeeks,sauMonths,sauYears };
enum { matchAll=1,matchAny };
enum { tabMailboxes=1,tabResults };
static short MenuToDateField[] = { minuteField,hourField,dayField,dayField,monthField,yearField };
	  
// criteria relation menu items
enum { srIs=1,srIsNot,srGreater,srLess };
enum { strContains=1,strContainsWord,strDoesNotContain,strIs,strIsNot,strStarts,strEnds,strRegExp };

// category table indicates relation and specifier
enum
{
	kNeedsSum=1<<0,
	kNeedsMsg=1<<1,	
};

enum
{
	kSearchingLocal=1<<0,
	kSearchingIMAP=1<<1
};

// Auditable Search Window controls
enum
{
	kctlSearch=0,
	kctlMore,
	kctlFewer,
	kctlTabs,
	kctlSrchResults,
	kctlCriteria,
	kctlMatch
};

struct
{
	short		relation;
	short		specifier;
	short		flags;
	Boolean	proOnly;
} gCategoryTable[] = 
	{
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,false,		// anywhere
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// headers
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// body
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// attachment names
		0,0,0,0,										// --------------------	
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsSum,false,		// summary
		SRCH_EQUAL_MENU,FILT_STATUS_MENU,kNeedsSum,true,	// status
		SRCH_COMPARE_MENU,ssNumber,kNeedsSum,true,				// junk score
		SRCH_COMPARE_MENU,FILT_PRIOR_MENU,kNeedsSum,true,	// priority
		SRCH_COMPARE_MENU,ssNumber,kNeedsSum,true,				// attachment count
		SRCH_EQUAL_MENU,LABEL_HIER_MENU,kNeedsSum,true,		// label
		SRCH_DATE_COMPARE_MENU,ssDate,kNeedsSum,true,			// date
		SRCH_COMPARE_MENU,ssNumber,kNeedsSum,true,				// size
		SRCH_COMPARE_MENU,ssAge,kNeedsSum,true,						// age
		SRCH_EQUAL_MENU,PERS_HIER_MENU,kNeedsSum,true,		// personality
		0,0,0,0,										// --------------------
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// to
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// from
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// subject
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// Cc
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true,			// Bcc
		SRCH_TEXT_COMPARE_MENU,ssText,kNeedsMsg,true			// any recipient
	};

//	structs
typedef struct
{
	short box;
	short lastBoxSpec;
	long message;
	TOCHandle tocH; 		// current toc
	Boolean opened;			// we opened the TOC (we need to close it).
} SearchMarker;

typedef struct
{
	ParmBlkPtr pb;
	long offset;
	long size;
	Boolean free;
	Boolean pending;
	Handle buffer;
	long bSize;
}	BulkSearchBuffer;

typedef struct
{
	Boolean	didBulkSearch;
	short refN;
	long spot, len;
	BulkSearchBuffer buf[2];
	long fullOffset;
	short	bulkSearchCriteriaIdx;	// which criterion are we currently searching
	Byte	bulkSearchCriteria[kMaxCriteria];	// which criteria need bulk searching
	short	nBulkSearchCriteria;	// how many criteria need bulk searching
} BulkSearchInfo;

typedef struct
{
	short		day;
	short		month;
	short		year;
} SearchDate;

typedef struct
{
	ControlHandle	ctlCategory;
	ControlHandle	ctlRelation;
	ControlHandle	ctlSpecifier;
	ControlHandle	ctlPete;			// for drawing pte
	PETEHandle 		pte;				// te box for the string we're looking for
	StringHandle	searchString;		/* the string we're finding */
	StringHandle	utf8SearchString;	// ditto, but utf8-ified
	RegexpHandle	regexp;				// regular expression we are finding
	Accumulator		hits;				// all the hits in the current mailbox
	short			category;
	short			relation;
	long			specifier;
	long			misc;
	SearchDate		date;
	long			zoneSecs;
	Boolean			useSenderZone;
	Boolean fudge8bit;
} CriteriaInfo, *CriteriaPtr, **CriteriaHandle;

typedef struct
{
	short	criteriaCount;				// number of criteria visible
	CriteriaHandle	hCriteria;			// criteria controls
	SearchMarker curr;					/* place where searching is occurring */
	ControlHandle	ctlSearch,ctlChaseArrows,ctlCriteriaGroup;
	ControlHandle	ctlMore,ctlFewer;
	ControlHandle	ctlMailBox,ctlTabs;
	ControlHandle	ctlMatch,ctlSrchResults;
	ControlHandle	ctlFocus;			// control with focus
	short		searchMode;				// actively searching now?
//	Boolean		bulk;					// do bulk search?
	Boolean		mailboxView;
	Boolean		bulkSearching;			// bulk searching a mailbox file?
	Boolean		matchAny;
	Boolean		searchResults;
	Boolean		didSearch;
	Boolean		noBulkSearch;
	short		saveBotMargin;
	short 		savePreviewHi;
	short		criteriaFlags;
	ViewListPtr	list;					// list for selecting mailboxes
	BoxCountHandle 	BoxCount;			// list of selected mailboxes
	BoxCountHandle	imapBoxCount;		// list of IMAP selected mailboxes for server search
	BoxCountHandle	redoBoxCount;		// list of mailboxes we need to search again because they were busy they first time
	uLong		redoTicks;				// try again when we get to this tick count
	short		**imapBoxSpecIdx;		// mailbox spec index+1
	Byte		searchOrder[kMaxCriteria];
	long		nHits;
	long		chaseArrowTicks;
	BulkSearchInfo	bulkData;
	ExpandInfo	expandList;
	FSSpec		saveSpec;
	Boolean setup;	// has the search been setup?
#ifdef VALPHA
	long			nSearched,nTicks;
#endif
} SearchVars, *SearchPtr, **SearchHandle;

typedef struct
{
	short			category;
	short			relation;
	long			specifier;
	long			misc;
	SearchDate		date;
	short			stringID;
} CriteriaSave;

typedef struct
{
	short	version;
	short	criteriaCount;
	short	matchMode;
	short	tabMode;
	long	unused[2];
	CriteriaSave	criteria[];
} SearchSaveInfo;

#define READY(b)	(!bulkData.buf[b].pending && !bulkData.buf[b].free)
#define kFoundIt ' fnd'

//	functions
static void SearchDidResize(MyWindowPtr win, Rect *oldContR);
static void SearchUpdate(MyWindowPtr win);
static Boolean SearchClose(MyWindowPtr win);
static void SearchClick(MyWindowPtr win,EventRecord *event);
static void SearchCursor(Point mouse);
static void SearchActivate(MyWindowPtr win);
static Boolean SearchKey(MyWindowPtr win, EventRecord *event);
static Boolean SearchApp1(MyWindowPtr win, EventRecord *event);
static Boolean DoSearchKey(MyWindowPtr win, EventRecord *event);
static void SearchHelp(MyWindowPtr win,Point mouse);
static Boolean SearchMenuSelect(MyWindowPtr win, int menu, int item, short modifiers);
static void SearchGrow(MyWindowPtr win,Point *newSize);
static void SearchIdle(MyWindowPtr win);
static Boolean SearchPosition(Boolean save,MyWindowPtr win);
static void GetSearchInfo(MyWindowPtr win,TOCHandle *ptoc,SearchHandle *psh);
static void StopSearch(MyWindowPtr win);
static void SearchButton(MyWindowPtr win,ControlHandle button,long modifiers,short part);
static Boolean MessPreFind(SearchHandle sh,StringPtr s,short criteriaIdx);
static Boolean FindInHits(SearchHandle sh,AccuPtr a);
static void ZapSpecList(TOCHandle toc);
static Boolean SearchHasSelection(MyWindowPtr win);
static Boolean SearchScroll(MyWindowPtr win,short h,short v);
static OSErr SearchBulk(FSSpecPtr spec,SearchHandle sh);
static long SearchLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data);
static ViewListPtr InitMailboxList(MyWindowPtr win,Boolean useMBSelection);
static void SizeMBList(MyWindowPtr win,SearchHandle sh);
static TOCHandle GetBoxTOC(SearchHandle sh,SearchMarker *which);
static TOCHandle GetSpecTOC(FSSpecPtr spec,Boolean *opened);
static void FinishedTOC(SearchMarker *which);
static OSErr GetSelectedBoxes(SearchPtr sp,TOCHandle tocH,Boolean bStartIMAPSearch);
static void AddCriteria(MyWindowPtr win,SearchHandle sh,Boolean defaultString);
static void RemoveCriteria(MyWindowPtr win,SearchHandle sh);
static Boolean ClickCriteriaButton(MyWindowPtr win,SearchHandle sh,ControlHandle cntl,short part);
static void DisposeSpecifier(MyWindowPtr win,SearchHandle sh,CriteriaInfo *pCriteria);
static void CreateTextSpecifier(MyWindowPtr win,CriteriaInfo *pCriteria,Boolean number);
static Boolean SetupSearchCriteria(MyWindowPtr win,SearchPtr sp);
static Boolean SearchMessage(MyWindowPtr win,MSumPtr sumP,SearchHandle sh,SearchMarker *curr,Boolean *stop,Boolean sumOnly);
static Boolean SearchText(StringPtr value,UPtr pText,long len,long offset,short relation);
static Boolean SearchString(StringPtr value,StringPtr s,short relation);
static Boolean SearchHeader(short header,Handle text,long size,StringPtr searchString,short relation);
// static short ShortCompare(short value1,short value2); CK
static void SetSearchTopMargin(MyWindowPtr win,SearchHandle sh);
static Boolean MustSearchBody(short category);
static void InvalidBoxSize(MyWindowPtr win);
static void CheckChaseArrows(SearchHandle sh);
static void AutoSearch(MyWindowPtr srchWin, MyWindowPtr	win, short searchMode);
static void UpdateBoxSize(MyWindowPtr win);
static void ProcessSearch(MyWindowPtr win,TOCHandle toc,SearchHandle sh);
static void DoBulkSearch(SearchHandle sh);
static void StopBulkSearch(SearchHandle sh);
static void NormalizeSearchString(PStr s);
static Boolean SearchSetControls(MyWindowPtr win, SearchHandle sh);
static Boolean GetBoxCountSpec(SearchHandle sh, short boxNum, FSSpecPtr spec);
static Boolean GetBoxCountSpecLo(BoxCountHandle boxList, short boxNum, FSSpecPtr spec);
static void DisplayMBName(SearchHandle sh, PStr name);
static AccuPtr FindHits(SearchHandle sh, short idx);
static void GetHits(SearchHandle sh,short idx,AccuPtr a);
static void SetHits(SearchHandle sh,short idx,AccuPtr a);
long SearchStrText(PStr s,Ptr text,long length,long offset);
static Boolean SearchFind(MyWindowPtr win,PStr what);
static IMAPSCHandle SetupIMAPSearchCriteria(SearchPtr sp);
static void ProcessIMAPSearch(MyWindowPtr win,TOCHandle toc,SearchHandle sh);
static OSErr AddSearchResult(SearchHandle sh, TOCHandle srchTOC, TOCHandle tocH, short sumNum, Boolean addSpec, short specIdx);
static short FindMBIdx(TOCHandle tocH,FSSpecPtr toSpec,Boolean allowCreate);
static void SearchSave(MyWindowPtr win,Boolean saveAs);
static void SelectMailbox(FSSpecPtr spec,ViewListPtr pView,Boolean folder,Boolean addToSelection);
static MSumPtr FindLinkSummary(TOCHandle srchTocH, TOCHandle fromTocH, long serialNum, short *realSum, short *virtualMBIdx);
static void RemoveFocus(MyWindowPtr win);
static void GetCriteriaString(StringPtr s,CriteriaPtr criteria);
static void SearchSetWTitle(MyWindowPtr win,SearchHandle sh);
static void GetMenuText(ControlHandle cntl,StringPtr s);
static void ResizeCriterion(MyWindowPtr win,SearchHandle sh,short i);
static void AddCriteriaText(SearchHandle sh, StringPtr sText);
static pascal void SearchTextDraw(ControlHandle cntl, SInt16 part);
static void ShowHideTabs(ControlRef ctlTabs,Boolean show);
static void EmbedSearchCriteriaControls(CriteriaInfo *criteria,SearchHandle sh);
Boolean SearchBoxesInclude(SearchHandle sh,TOCHandle tocH);
Boolean SearchIncremental(MyWindowPtr win,TOCHandle tocH,short sumNum);

long DateTimeDifference(DateTimeRec *date,DateTimeRec *currDate,long seconds,short units);


//	globals
short	gSearchWinCount;
long	gLVFlags;

/************************************************************************
 * SearchOpen - open a new search window
 ************************************************************************/
MyWindowPtr SearchOpen(short searchMode)
{
	WindowPtr	winWP;
	MyWindowPtr	win;
	WindowPtr	topWinWP = FindTopUserWindow();
	MyWindowPtr	topWin = GetWindowMyWindowPtr(topWinWP);
	SearchVars	vars;
	OSErr		err;
	TOCHandle toc=nil;
	TOCPtr		ptoc;
	Handle		hSearchInfo=nil;
	Rect		r;
	Boolean		topMBWin = topWinWP && GetWindowKind(topWinWP) == MB_WIN;
	
	Zero(vars);
	if (!(win=GetNewMyWindow(SEARCH_WIND,nil,nil,BehindModal,true,true,MBOX_WIN)))
		{err=MemError(); goto fail;}
	winWP = GetMyWindowWindowPtr(win);
	SetWinMinSize(win,-1,-1);
	SetPort_(GetWindowPort(winWP));
	ConfigFontSetup(win);	
	win->hPitch = FontWidth;
	win->vPitch = FontLead+FontDescent;

	// controls
	SetTopMargin(win,kWinHeaderHt+kCriteriaHt);
	vars.ctlSearch = CreateControl(win,win->topMarginCntl,SEARCH_BUTTON,kControlPushButtonProc,true);
	OutlineControl(vars.ctlSearch,true);
	SetRect(&r,0,0,100,30);
	vars.ctlTabs = NewControl(winWP,&r,"\p",true,SEARCH_WIN_TABS,0,0,kControlTabSmallProc,0);	
	LetsGetSmall(vars.ctlTabs);
	EmbedControl(vars.ctlTabs,win->topMarginCntl);
	if (HasFeature (featureSearch)) {
		vars.ctlMore = CreateControl(win,win->topMarginCntl,MORE_CHOICES,kControlPushButtonProc,true);
		vars.ctlFewer = CreateControl(win,win->topMarginCntl,FEWER_CHOICES,kControlPushButtonProc,true);
		vars.ctlMatch = CreateMenuControl(win,win->topMarginCntl,"\p",SRCH_AND_OR_MENU,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,0,false);
		vars.ctlSrchResults = CreateControl(win,win->topMarginCntl,SEARCH_RESULTS,kControlCheckBoxProc,true);
	}
	vars.ctlCriteriaGroup = CreateControl(win,win->topMarginCntl,0,kControlGroupBoxTextTitleProc,false);
	SetRect(&r,-64,-64,-48,-48);
	if (HasFeature (featureSearch)) {
		SetControlVisibility(vars.ctlFewer,false,true);
		SetControlVisibility(vars.ctlMatch,false,true);
		SetControlVisibility(vars.ctlSrchResults,false,true);
	}
	vars.ctlChaseArrows = CreateControl(win,win->topMarginCntl,0,kControlChasingArrowsProc,false);
	HideControl(vars.ctlChaseArrows);
	vars.ctlMailBox = CreateControl(win,win->topMarginCntl,0,kControlStaticTextProc|kControlPopupUseWFontVariant,false);
	SetControlVisibility(vars.ctlMailBox,false,true);
	vars.mailboxView = true;
	vars.expandList.resID = kSearchExpandID;
	if (topMBWin)
	{
		//	Expand same mailfolders as in Mailboxes window
		ExpandInfoPtr pExpList = MBGetExpList();		
		if (pExpList->hExpandList)
			vars.expandList.hExpandList = DupHandle(pExpList->hExpandList);
	}
	vars.savePreviewHi = GetPrefLong(PREF_SEARCH_PREVIEW);
				
	// set up data for virtual mailbox
	// need a TOC
	vars.hCriteria = NuHandle(0);
	if (!(toc=NewZH(TOCType))) goto fail;
	if (err = PtrToHand(&vars,&hSearchInfo,sizeof(vars))) goto fail;
	ptoc = *toc;
	ptoc->which = SEARCH_WIN;
	ptoc->version = CURRENT_TOC_VERS;
	ptoc->minorVersion = CURRENT_TOC_MINOR;
	ptoc->nextSerialNum = 1;
	ptoc->virtualTOC = true;
	ptoc->mailbox.virtualMB.data = hSearchInfo;
	ptoc->mailbox.virtualMB.type = kSearchMB;
	ptoc->win = win;
	(*toc)->previewHi = vars.savePreviewHi;
	SetMyWindowPrivateData(win,(long)toc);
	
	BoxGonnaShow(win);
	BoxListFocus(toc,false);	// kill the list focus so the search button works
	(*toc)->searchFocus = true;

	win->didResize = SearchDidResize;
	win->close = SearchClose;
	win->update = SearchUpdate;
	win->position = SearchPosition;
	win->click = SearchClick;
	win->bgClick = SearchClick;
	win->activate = SearchActivate;
	win->help = SearchHelp;
	win->menu = SearchMenuSelect;
	win->key = SearchKey;
	win->app1 = SearchApp1;
	win->button = SearchButton;
	win->scroll = SearchScroll;
	win->selection = SearchHasSelection;
	win->cursor = SearchCursor;
	win->drag = nil;	//	BoxGonnaShow set this up. We don't want it.
	win->idle = SearchIdle;
	win->dontControl = true;
	win->zoomSize = MaxSizeZoom;
	win->find = SearchFind;
	
	if (searchMode)
	{
		AddCriteria(win,(SearchHandle)hSearchInfo,true);
	}

	gLVFlags = searchMode == FIND_SEARCH_ALL_ITEM ? kfAutoSelectChild + kfAutoSelectAll : kfAutoSelectChild;
	(*(SearchHandle)hSearchInfo)->list = InitMailboxList(win,topMBWin);
	if (searchMode)
	{
		ShowMyWindow(winWP);
		UserSelectWindow(winWP);
		if (topWin)
			AutoSearch(win,topWin,searchMode);
	}
	SearchSetControls(win, (SearchHandle)hSearchInfo);
	gSearchWinCount++;
	SearchSetWTitle(win,(SearchHandle)hSearchInfo);
	SearchButton(win,vars.ctlTabs,0,0);		// Make sure Mailboxes tab is set up correctly
	return win;
	
fail:
	if (win) CloseMyWindow(GetMyWindowWindowPtr(win));
	ZapHandle(toc);
	ZapHandle(hSearchInfo);
	if (err) WarnUser(COULDNT_WIN,err);
	return win;
}

/**********************************************************************
 * CreateControl - create a control for the find window
 **********************************************************************/
static void AutoSearch(MyWindowPtr srchWin, MyWindowPtr	topWin, short searchMode)
{
	Str255	s;
	SearchHandle	sh;
	Boolean		selectedText=false;
	
	//	get selected text in current window
	*s=0;
	if (topWin && topWin->pte && topWin->hasSelection)
		SetFindString(s,topWin->pte);
	
	GetSearchInfo(srchWin,nil,&sh);
	if (*s && searchMode!=FIND_SEARCH_ITEM)
	{
		PeteSetString(s,(*(*sh)->hCriteria)[0].pte);
		selectedText = true;
	}
	
	if (searchMode != FIND_SEARCH_ALL_ITEM)
	{
		//	get mailbox or mailfolder of top window
		TOCHandle	tocH = nil;
		FSSpec		spec;
		ViewListPtr	pView = (*sh)->list;
		
		switch (GetWindowKind(GetMyWindowWindowPtr(topWin)))
		{
			case COMP_WIN:
			case MESS_WIN:
				tocH = (*Win2MessH(topWin))->tocH;
				break;
			case MBOX_WIN:
			case CBOX_WIN:
				tocH = (TOCHandle)GetMyWindowPrivateData(topWin);
				break;
		}
		
		if (!tocH)
			return;	//	there is no mailbox			

		//	select item in mailbox list
		spec = GetMailboxSpec(tocH,-1);
		SelectMailbox(&spec,pView,searchMode==FIND_SEARCH_FOLDER_ITEM,false);
	}
	
	if (selectedText && searchMode!=FIND_SEARCH_ITEM)
		StartSearch(srchWin);
}


/************************************************************************
 * SearchTextDraw - draw the search string
 ************************************************************************/
static pascal void SearchTextDraw(ControlHandle cntl, SInt16 part)
{
#pragma unused(part)
	PETEHandle pte = (PETEHandle)GetControlReference(cntl);

	PeteUpdate(pte);
}

/**********************************************************************
 * ResizeCriterion - resize this criterion
 **********************************************************************/
static void ResizeCriterion(MyWindowPtr win,SearchHandle sh,short i)
{
	CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];
	short			top = INSET+9+i*kCriteriaHt;
	short			right;
	Rect			r,rCntl;

	MoveMyCntl(criteria.ctlCategory,2*INSET,top+1,0,0);
	GetControlBounds(criteria.ctlCategory,&rCntl);
	right = rCntl.right;
	MoveMyCntl(criteria.ctlRelation,right+INSET,top+1,0,0);
	GetControlBounds(criteria.ctlRelation,&rCntl);
	right = rCntl.right;
	if (criteria.ctlPete)
	{
		SetRect(&r,right+INSET+4,top+3,criteria.ctlSpecifier ? right+INSET+60 : WindowWi(GetMyWindowWindowPtr(win))-3*INSET,top+GetLeading(SmallSysFontID(),SmallSysFontSize())+3);
		PeteDidResize(criteria.pte,&r);
		MoveMyCntl(criteria.ctlPete,r.left,r.top,r.right-r.left,r.bottom-r.top);
		SetControlVisibility(criteria.ctlPete,true,false);
		right = r.right;		
	}
	if (criteria.ctlSpecifier)
	{
		short	specTop = top+1;
		
		switch (criteria.category)
		{
			case scStatus:
			case scPriority:
			case scLabel:
			case scDate:
				specTop -= 2;
				break;
		}
		MoveMyCntl(criteria.ctlSpecifier,right+INSET,specTop,0,0);
	}
}

/**********************************************************************
 * SearchDidResize - resize this search window
 **********************************************************************/
static void SearchDidResize(MyWindowPtr win, Rect *oldContR)
{
	SearchHandle	sh;
	ControlHandle	ctl;
	short			wi,v1,v3,right,left,moreRight;
	short			winWidth = WindowWi(GetMyWindowWindowPtr(win));
	short			criteriaHt;
	short			groupRight;
	short			i;
	TOCHandle 		tocH;
	Rect			rCntl;
	
	GetSearchInfo(win,&tocH,&sh);

	SetSearchTopMargin(win,sh);

	//	criteria group box
	ctl = (*sh)->ctlCriteriaGroup;
	criteriaHt = INSET+(*sh)->criteriaCount*kCriteriaHt+4;
	groupRight = winWidth-2*INSET;
	MoveMyCntl(ctl,INSET,INSET,groupRight,criteriaHt);
	SetControlVisibility(ctl,true,true);	// it's disappearing for some reason
	
	//	criteria
	for(i=0;i<(*sh)->criteriaCount;i++)
		ResizeCriterion(win,sh,i);

	v1 = INSET+(*sh)->criteriaCount*kCriteriaHt+16;
	
	//	"search" button
	ctl = (*sh)->ctlSearch;
	left = winWidth-ControlWi(ctl)-2*INSET;
	MoveMyCntl(ctl,left,v1+2,0,0);
	
	//	chasing arrows
	left = left - INSET - 16;
	MoveMyCntl((*sh)->ctlChaseArrows,left,v1+2,16,16);
	
	// more, fewer buttons, etc
	if (HasFeature (featureSearch)) {
		ctl = (*sh)->ctlMore;
		MoveMyCntl(ctl,INSET,v1,0,0);
		GetControlBounds(ctl,&rCntl);
		right = moreRight = rCntl.right;
		ctl = (*sh)->ctlFewer;
		MoveMyCntl(ctl,right+INSET,v1,0,0);
		GetControlBounds(ctl,&rCntl);
		right = rCntl.right;
		ctl = (*sh)->ctlMatch;
		MoveMyCntl(ctl,right+INSET,v1+1,0,0);
		GetControlBounds(ctl,&rCntl);
		right = (*sh)->criteriaCount > 1 ? rCntl.right : moreRight;
		ctl = (*sh)->ctlSrchResults;
		MoveMyCntl(ctl,right+INSET,v1,0,0);
	
		v3 = v1+28;
	}
	else
		v3 = v1+12;
	
	// mailbox name
	wi = winWidth - 2*INSET;
	if (wi < 100) wi = 0;	// Make it invisible if too narrowl
	ctl = (*sh)->ctlMailBox;
	MoveMyCntl(ctl,INSET+3,v3,wi,24);
	SetControlVisibility(ctl,(*sh)->searchMode,true);

	// tabs
	MoveMyCntl((*sh)->ctlTabs,-3,v3,winWidth+6,30);
	SetControlVisibility((*sh)->ctlTabs,!(*sh)->searchMode,true);

	if ((*sh)->mailboxView)
		// Mailbox list
		SizeMBList(win,sh);
	else
		BoxDidResize(win,oldContR);
	
	SetPrefLong(PREF_SEARCH_PREVIEW,(*tocH)->previewHi);

	
}

/**********************************************************************
 * SearchClose - close this search window
 **********************************************************************/
static Boolean SearchClose(MyWindowPtr win)
{
	TOCHandle toc;
	SearchHandle	sh;
	WindowPtr thisWin;

	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(win));

	StopSearch(win);
	
	GetSearchInfo(win,&toc,&sh);
		
	while ((*sh)->criteriaCount)
		RemoveCriteria(win,sh);
	ZapHandle((*sh)->hCriteria);
	LDRef(sh);
	
	//	Save expanded folder list
	if (!(*sh)->didSearch)
		//	Don't save changes. Just dispose of data
		(*sh)->expandList.fExpandListChanged = false;
	SaveExpandedFolderList(&(*sh)->expandList);	

	//	Remove references to this window from any
	//	messages opened from this window
	for (thisWin=FrontWindow_();thisWin;thisWin=GetNextWindow(thisWin))
		if (GetWindowKind(thisWin)==COMP_WIN || GetWindowKind(thisWin)==MESS_WIN)
		{
			MessHandle	messH = WinPtr2MessH(thisWin);
			
			if ((*messH)->openedFromTocH == toc)
				(*messH)->openedFromTocH = (*messH)->tocH;
		}

	ZapSpecList(toc);
	ZapHandle(toc);
	if ((*sh)->list)
	{
		LVDispose((*sh)->list);
		ZapPtr((*sh)->list);
	}
	ZapHandle((*sh)->BoxCount);
	ZapHandle((*sh)->imapBoxCount);
	ZapHandle((*sh)->redoBoxCount);
	ZapHandle((*sh)->imapBoxSpecIdx);
	ZapHandle(sh);
	win->selection = nil;
	gSearchWinCount--;
	PopGWorld();
	return true;
}

/**********************************************************************
 * SearchUpdate - update search window
 **********************************************************************/
static void SearchUpdate(MyWindowPtr win)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);	
	if ((*sh)->mailboxView)
		LVDraw((*sh)->list,nil,true,false);
	else
		BoxUpdate(win);

	// these guys tend to get disabled
	if (HasFeature (featureMultiplePersonalities))
		EnableItem(GetMHandle(PERS_HIER_MENU),0);
	EnableItem(GetMHandle(LABEL_HIER_MENU),0);
}

/**********************************************************************
 * SearchClick - handle click in search window
 **********************************************************************/
static void SearchClick(MyWindowPtr win,EventRecord *event)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Point	pt;
//	PETEHandle pte;
	SearchHandle	sh;
	ControlHandle cntl;
	TOCHandle tocH;
	Rect	rCntl;

	GetSearchInfo(win,&tocH,&sh);	
	pt = event->where;
	GlobalToLocal(&pt);

	if ((*sh)->ctlFocus && !PtInRect(pt,GetControlBounds((*sh)->ctlFocus,&rCntl)))
	{
		SetKeyboardFocus(winWP,(*sh)->ctlFocus,kControlFocusNoPart);
		(*sh)->ctlFocus = nil;
		SearchSetWTitle(win,sh);
	}

	if (pt.v > win->topMargin)
	{
		if ((*sh)->mailboxView)
		{
			RemoveFocus(win);
			LVClick((*sh)->list,event);
		}
		else
			BoxClick(win,event);
	}
	else
	{
		short	i;		
		
		if (!win->isActive) {SelectWindow_(winWP);}
		BoxListFocus(tocH,false);
		(*tocH)->searchFocus = true;
		for(i=0;i<(*sh)->criteriaCount;i++)
		{
			PETEHandle	pte;
			if ((pte=(*(*sh)->hCriteria)[i].pte) &&  PtInPETE(pt,pte))
			{
				StopSearch(win);
				PeteEditWFocus(win,pte,peeEvent,(void*)event,nil);
				return;
			}
		}

		if (FindControl(pt,winWP,&cntl))
		{
			// It's a control. If a menu control, make sure it's enabled
			MenuHandle	mh;
			
			if (mh = GetPopupMenuHandle(cntl))
				EnableItem(mh,0);
				
			// ignore hits on the server status column header like in a regular mailbox -jdboyd 03/01/01
			if ((cntl==FindControlByRefCon(win,'wide'+blServer)) && PtInControl(pt,cntl));
			else HandleControl(pt,win);
		}
	}
	SearchSetControls(win, sh);
}

/************************************************************************
 * SearchHasSelection - is there a selection?
 ************************************************************************/
static Boolean SearchHasSelection(MyWindowPtr win)
{
	SearchHandle	sh;
	long start,stop;

	GetSearchInfo(win,nil,&sh);
	if (win->pte && !PeteGetTextAndSelection(win->pte,nil,&start,&stop))
		return(stop!=start);
	else if ((*sh)->mailboxView)
		return (*sh)->list && LVCountSelection((*sh)->list) ? true : false;
	else
		return BoxHasSelection(win);
}

/**********************************************************************
 * SearchCursor - adjust the cursor
 **********************************************************************/
static void SearchCursor(Point mouse)
{
	SearchHandle	sh;
	MyWindowPtr win=GetWindowMyWindowPtr(FrontWindow_());

	GetSearchInfo(win,nil,&sh);
	if (!(*sh)->mailboxView)
		BoxCursor(mouse);
	else if (!PeteCursorList(win->pteList,mouse))
		SetMyCursor(arrowCursor);
}

/**********************************************************************
 * SearchActivate - activate/deactivate search window
 **********************************************************************/
static void SearchActivate(MyWindowPtr win)
{
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);
	if ((*sh)->mailboxView)
		LVActivate((*sh)->list, win->isActive);
	else
		BoxActivate(win);
}

/**********************************************************************
 * SearchScroll - do scrolling for search window 
 **********************************************************************/
static Boolean SearchScroll(MyWindowPtr win,short h,short v)
{
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);
	if (!(*sh)->mailboxView)
		BoxScroll(win,h,v);
	return(True);
}

/**********************************************************************
 * SearchApp1 - process scrolling keys
 **********************************************************************/
static Boolean SearchApp1(MyWindowPtr win, EventRecord *event)
{
	SearchHandle	sh;
	TOCHandle tocH;
	
	GetSearchInfo(win,&tocH,&sh);
	if ((*sh)->mailboxView)
		return LVKey((*sh)->list,event);
	else if ((*tocH)->previewID && (*tocH)->previewPTE)
	{
		event->what = keyDown;
		PeteEdit(win,(*tocH)->previewPTE,peeEvent,(void*)event);
		return true;
	}
	else
	{
		// Send to the mailbox list
		DoApp1NoPTE(win,event);
		return true;
	}
}

/**********************************************************************
 * SearchKey - process interesting keys
 **********************************************************************/
static Boolean SearchKey(MyWindowPtr win, EventRecord *event)
{
	SearchHandle	sh;
	Boolean			result;
	
	GetSearchInfo(win,nil,&sh);
	result = DoSearchKey(win,event);
	SearchSetControls(win, sh);
	return result;
}

/**********************************************************************
 * SearchKey - process interesting keys
 **********************************************************************/
static Boolean DoSearchKey(MyWindowPtr win, EventRecord *event)
{
	short key = event->message & 0xff;
	SearchHandle	sh;
	TOCHandle	tocH;
	ControlHandle	cntl;

	GetSearchInfo(win,&tocH,&sh);
	if (event->modifiers&cmdKey && IsCmdChar(event,'.') ||	// command-period
				 (event->message&charCodeMask)==escChar)
	{
#ifdef SPEECH_ENABLED
		if (CancelSpeech ())
			return (false);
#endif
		//	stop searching
		StopSearch(win);
		return true;
	}
		
	if (key=='\t')
	{
		// Who would believe this could be so complicated?
		typedef enum {
			stabOnly=0,	// there is only one pte in the search criteria, and it's focussed
			stabFrst,		// there is more than one pte in the search criteria, and the first one is focussed
			stabMddl,		// there are more than two pte's in the search criteria, and a middle one is focussed
			stabLast,		// there is more than one pte in the search criteria, and the last one is focussed
			stabNone,		// there are no pte's in the search criteria, but the criteria nonetheless have focus
			stabSumm,		// the focus is in the list of mailbox summaries
			stabPrev,		// the focus is in the preview pane
			stabBxes,		// the focus is in the mailboxes list
			stabLimit
			} SearchSelectTypeEnum;
		SearchSelectTypeEnum currentSelect, nextSelect;
		static SearchSelectTypeEnum STabStateTable[2][stabLimit] = {
					//	stabOnly	stabFrst	stabMddl	stabLast	stabNone	stabSumm	stabPrev	stabBxes
/* forward */	stabSumm,	stabMddl,	stabMddl,	stabSumm,	stabSumm,	stabPrev,	stabFrst,	stabFrst,
/* backwrd */	stabPrev,	stabPrev,	stabMddl,	stabMddl,	stabSumm,	stabLast,	stabSumm,	stabLast
		};		
		PETEHandle nextPTE;
		PETEHandle origPTE = win->pte;
		Boolean backward = (event->modifiers&shiftKey)!=0;
		
		// Figure out where we are
		if (win->pte)
		{
			nextPTE = PeteNext(win->pte);
			if (win->pte==(*tocH)->previewPTE) currentSelect = stabPrev;
			else if (nextPTE==(*tocH)->previewPTE || nextPTE==nil)
				currentSelect = win->pte==win->pteList ? stabOnly : stabLast;
			else
				currentSelect = win->pte==win->pteList ? stabFrst : stabMddl;
		}
		else if ((*tocH)->searchFocus)
			currentSelect = stabNone;
		else if ((*sh)->mailboxView)
			currentSelect = stabBxes;
		else
			currentSelect = stabSumm;
		
		// First crack at where we should go
		nextSelect = STabStateTable[backward][currentSelect];
		
		// Fine-tuning
retune:
		switch (nextSelect)
		{
			case stabSumm:
				if ((*sh)->mailboxView)
					nextSelect = stabBxes;	// mailboxes list is showing, go there
				break;

			case stabPrev:
				if ((*sh)->mailboxView)
					nextSelect = stabBxes;	// mailboxes list is showing, go there
				else if (!(*tocH)->previewID)
				{
					// no preview, go elsewhere
					if (backward)
					{
						nextSelect = stabSumm;	// backing up from the top is the summary list
					}
					else
					{
						nextSelect = stabFrst;	// foward from the summaries is the top preview edit field
						goto retune;	// but it might not be there, so try again
					}
				}
				break;
			
			case stabFrst:
			case stabMddl:
			case stabLast:
				if (win->pteList==(*tocH)->previewPTE)
					nextSelect = stabNone;	// if no edit field or only edit is preview, just focus on criteria, not pte at all
				break;
		}
		
		// Ok, things are pretty simple now
		
		// First, we defocus
		switch (currentSelect)
		{
			case stabOnly:
			case stabFrst:
			case stabMddl:
			case stabLast:
			case stabNone:
			case stabPrev:
				PeteFocus(win,nil,true);	// no-op for stabNone, but who cares?
				(*tocH)->searchFocus = false;	// no-op for stabPrev, but who cares?
				break;
				
			case stabSumm:
				BoxListFocus(tocH,false);
				break;
			
			case stabBxes:
				// nothing special here
				break;
			default:
				ASSERT(0);	// naughty processor shdn't be here
				break;
		}
		
		// Then, we focus
		switch (nextSelect)
		{
			case stabOnly:
			case stabFrst:
				PeteFocus(win,win->pteList,true);
				(*tocH)->searchFocus = true;
				break;
				
			case stabMddl:
				nextPTE = backward ? PetePrevious(win->pteList,origPTE) : PeteNext(origPTE);
				PeteFocus(win,nextPTE,true);
				(*tocH)->searchFocus = true;
				break;
				
			case stabLast:
				// this one actually requires some calculation.
				for (origPTE=win->pteList;;origPTE=nextPTE)
				{
					nextPTE = PeteNext(origPTE);
					if (nextPTE==nil || nextPTE==(*tocH)->previewPTE) break;
				}
				PeteFocus(win,origPTE,true);
				(*tocH)->searchFocus = true;
				break;
				
			case stabNone:
				(*tocH)->searchFocus = true;
				break;
				
			case stabPrev:
				// Sigh.  First, we must focus on the list
				BoxListFocus(tocH,true);
				// Then we send it a tab to focus on the preview
				BoxKey(win,event);
				break;
				
			case stabSumm:
				if (FirstMsgSelected(tocH)==-1) BoxInitialSelection(tocH);
				BoxListFocus(tocH,true);
				break;
			
			case stabBxes:
				// nothing special here
				break;
				
			default:
				ASSERT(0);	// naughty processor shdn't be here
				break;
		}
		
		if (win->pte && win->pte!=(*tocH)->previewPTE) PeteSelectAll(win,win->pte);
		
		// There, that was easy.  (sarcasm)
		return true;
	}

	if (key==returnChar || key==enterChar)
	{
		if ((*sh)->searchMode)
		{
			//	Stop if we're already searching
			StopSearch(win);
			return true;
		}
		
		//	Start search if in mailbox view or if focus not in results pane or not in preview pane or summary list
		if ((*sh)->mailboxView || win->pte && (win->pte != (*tocH)->previewPTE) || !(*tocH)->listFocus)
		{
			StartSearch(win);
			return true;
		}
	}

	if (!(event->modifiers&cmdKey) && !GetKeyboardFocus(GetMyWindowWindowPtr(win),&cntl) && cntl)
	{
		//	send key to control
		HandleControlKey(cntl,(event->message&keyCodeMask)>>8,event->message&charCodeMask,event->modifiers);
 		return true;
 	}

	if (win->pte)
	{
		if ((win->pte != (*tocH)->previewPTE) && !(event->modifiers&cmdKey))
			StopSearch(win);
		if (!(event->modifiers&cmdKey))
		{
			if ((win->pte == (*tocH)->previewPTE) && DirtyKey(event->message))
				AlertStr(READ_ONLY_ALRT, Stop, nil);
			else
			{
				PeteKey(win,win->pte,event);
				SearchSetControls(win, sh);	
			}
			return true;
		}
		else
			return false;
	}

	if ((*sh)->mailboxView)
		return LVKey((*sh)->list,event);
	else
		return BoxKey(win,event);
}

enum
{
	kHelpStrings,
	kHelpSearch,
	kHelpStop,
	kHelpMore,
	kHelpFewer,
	kHelpMatch,
	kHelpSearchResults,
	kHelpMailboxesTab,
	kHelpResultsTab,
	kHelpCategory,
	kHelpVerb,
	kHelpWhatMenu,
	kHelpWhatText,
	kHelpMBList,
	kHelpCount
};

/**********************************************************************
 * SearchHelp - provide help for the search window
 **********************************************************************/
static void SearchHelp(MyWindowPtr win,Point mouse)
{
	SearchHandle	sh;
	ControlHandle	cntl;
	short			part;
	short			helpID = 0;
	SearchPtr		sp;
	Rect			r;

	GetSearchInfo(win,nil,&sh);
	sp = LDRef(sh);

	if (part = FindControl(mouse,GetMyWindowWindowPtr(win),&cntl))
	{
		GetControlBounds(cntl,&r);
		if (cntl==sp->ctlSearch)
			helpID = sp->searchMode ? kHelpStop : kHelpSearch;			
		else if (cntl==sp->ctlMore)
			helpID = kHelpMore;
		else if (cntl==sp->ctlFewer)
			helpID = kHelpFewer;
		else if (cntl==sp->ctlMatch)
			helpID = kHelpMatch;			
		else if (cntl==sp->ctlSrchResults)
			helpID = kHelpSearchResults;
		else if (cntl==sp->ctlTabs)
			helpID = part == tabMailboxes ? kHelpMailboxesTab : kHelpResultsTab;
		else if (GetControlReference(cntl)==kBoxSizeRefCon)
			helpID = kHelpCount;
		else
		{
			//	see if it's in a criterion
			short	i;
			
			for(i=0;i<sp->criteriaCount;i++)
			{
				CriteriaPtr	pc=&(*sp->hCriteria)[i];

				if (cntl==pc->ctlCategory)
					{ helpID = kHelpCategory; break; }		
				else if (cntl==pc->ctlRelation)
					{ helpID = kHelpVerb; break; }
				else if (cntl==pc->ctlSpecifier)
					{ helpID = kHelpWhatMenu; break; }
				else if (cntl==pc->ctlPete)
					{ helpID = kHelpWhatText; break; }
			}
		}
	}
	
	if (!helpID)
	{
		if (sp->mailboxView)
		{
			if (PtInRect(mouse,&sp->list->bounds))
			{
				r = sp->list->bounds;
				helpID = kHelpMBList;
			}
		}
		else
			//	let boxhelp handle it
			BoxHelp(win, mouse);
	}
		
	if (helpID)
		MyBalloon(&r,100,0,helpID+SEARCH_HELP_STRN,0,nil);
	
	UL(sh);
}

/**********************************************************************
 * SearchMenuSelect - handle a menu selection
 **********************************************************************/
static Boolean SearchMenuSelect(MyWindowPtr win, int menu, int item, short modifiers)
{
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);
	switch (menu)
	{
		case FILE_MENU:
			switch (item)
			{
				case FILE_SAVE_ITEM:
					SearchSave(win,false);
					return true;
					break;
				case FILE_SAVE_AS_ITEM:
					SearchSave(win,true);
					return true;
					break;
			}
	}
	
	if ((*sh)->mailboxView)
	{
		if ((*sh)->list)
		{
			switch (menu)
			{
				case FILE_MENU:
					if (item==FILE_OPENSEL_ITEM)
					{
						MBListOpen((*sh)->list);
						return(True);
					}
					break;

				case EDIT_MENU:
					if (!win->pte)
					{
						switch(item)
						{
							case EDIT_SELECT_ITEM:
								if (LVSelectAll((*sh)->list))
									return(true);
								break;
							case EDIT_COPY_ITEM:
								LVCopy((*sh)->list);
								return true;
						}
					}
					break;
			}
				
		}
	}
	else
		return BoxMenu(win,menu,item,modifiers);
	
	return false;
}

/**********************************************************************
 * SearchFind - find in the Search window
 **********************************************************************/
static Boolean SearchFind(MyWindowPtr win,PStr what)
{
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);
	if ((*sh)->mailboxView)
		// mailbox view
		return FindListView(win,(*sh)->list,what);
	else
		//	results view
		return BoxFind(win,what);
}

/**********************************************************************
 * IsSearchWindow - is the window a Search window?
 **********************************************************************/
Boolean IsSearchWindow(WindowPtr theWindow)
{
	TOCHandle	tocH;
	
	return (IsKnownWindowMyWindow(theWindow) && GetWindowKind(theWindow)==MBOX_WIN &&
			(tocH = (TOCHandle)(GetWindowPrivateData(theWindow))) &&
			(*tocH)->virtualTOC &&
			 (*tocH)->mailbox.virtualMB.type == kSearchMB);
}

/**********************************************************************
 * GetSearchWinSpec - get saved spec for search window
 **********************************************************************/
Boolean GetSearchWinSpec(WindowPtr winWP,FSSpecPtr spec)
{
	SearchHandle	sh;

	if (IsSearchWindow(winWP))
	{
		MyWindowPtr	win = GetWindowMyWindowPtr (winWP);
		GetSearchInfo(win,nil,&sh);
		if ((*sh)->saveSpec.vRefNum)
		{
			*spec = (*sh)->saveSpec;
			return true;
		}
	}
	return false;
}

/**********************************************************************
 * SearchNewFindString - there's a new Find string
 **********************************************************************/
void SearchNewFindStringLo(PStr what,Boolean withPrejuidice)
{
	WindowPtr winWP;
	
	for (winWP=FrontWindow();winWP;winWP = GetNextWindow (winWP))
		if (IsWindowVisible(winWP) && IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			PETEHandle	pte;

			GetSearchInfo(GetWindowMyWindowPtr (winWP),nil,&sh);
			//	Replace search string if only one criterion, it has a text field,
			//	and the field is empty
			if ((*sh)->criteriaCount == 1 && 
				(pte=(*(*sh)->hCriteria)[0].pte) &&
				(withPrejuidice || !PeteLen(pte)))
				{
					PeteSetString(what,pte);
				}
			break;
		}
}

/**********************************************************************
 * SearchSetControls - set status of controls
 **********************************************************************/
static Boolean SearchSetControls(MyWindowPtr win, SearchHandle sh)
{
	Boolean	active = false;
	ControlHandle	ctl;
	
	//	Determine if search button should be dimmed
	if ((*sh)->searchMode)
		active = true;
	{
		//	search through criteria for something we can search on
		short		i;
		PETEHandle 	pte;
		
		for(i=0;i<(*sh)->criteriaCount;i++)
		{
			pte = (*(*sh)->hCriteria)[i].pte;
			if (!PeteIsValid(pte) || PeteLen(pte))
			{
				//	If critiera doesn't have a text field or
				//	there is something in the text field, don't dim
				active = true;
				break;
			}
		}
	}
	
	ctl = (*sh)->ctlSearch;
	if (win->isActive && IsControlActive(ctl) != active)
	{
		if (active) ActivateControl(ctl);
		else DeactivateControl(ctl);
	}
	
	win->userSave = (*sh)->saveSpec.vRefNum!=0;
	return active;
}

/**********************************************************************
 * GetSearchInfo - get search info from window
 **********************************************************************/
static void GetSearchInfo(MyWindowPtr win,TOCHandle *ptoc,SearchHandle *psh)
{
	TOCHandle toc=(TOCHandle)GetMyWindowPrivateData(win);
	SearchHandle	sh = (SearchHandle)(*toc)->mailbox.virtualMB.data;

	if (ptoc) *ptoc = toc;
	if (psh) *psh = sh;
}

/**********************************************************************
 * GetSearchTOC - get the TOC of a search window
 **********************************************************************/
void GetSearchTOC(MyWindowPtr win,TOCHandle *ptoc)
{
	TOCHandle toc=(TOCHandle)GetMyWindowPrivateData(win);
	SearchHandle	sh = (SearchHandle)(*toc)->mailbox.virtualMB.data;

	if (ptoc) *ptoc = toc;
}

/**********************************************************************
 * StartSearch - start the search
 **********************************************************************/
void StartSearch(MyWindowPtr win)
{
	SearchHandle	sh;
	SearchPtr		sp;
	TOCHandle	toc;
	short		n;
	Str32		s;

	ActiveSearchCount++;
	Log(LOG_SEARCH,"\pSearch started");
	
	GetSearchInfo(win,&toc,&sh);
	SearchSetWTitle(win,sh);

	if (!SearchSetControls(win, sh))
		//	Can't search. Probably no mailboxes selected
		return;
		
	sp = LDRef(sh);
	
	//	set up search parameters
	Zero(sp->curr);
	sp->curr.lastBoxSpec = -2;
	sp->curr.box = -1;
	sp->searchMode = kSearchingLocal;
	sp->didSearch = true;
	sp->nHits = 0;
	sp->bulkData.didBulkSearch = false;
	
	if (!SetupSearchCriteria(win,sp)) return;	// setup failed
	if (GetSelectedBoxes(sp,toc,true)) return;	// aborted

	// remove any current hits
	if (!(*sh)->searchResults)
	{
		n=(*toc)->count;
		if (n)
			UpdateBoxSize(win);
		while (n--)
			DeleteSum(toc,n);
		ZapSpecList(toc);
	}

	if ((*sh)->mailboxView)
	{
		//	switch back to results view
		SetControlValue((*sh)->ctlTabs,tabResults);
		SearchButton(win,(*sh)->ctlTabs,0,0);
	}
	ShowHideTabs(sp->ctlTabs,false);
	ShowControl(sp->ctlChaseArrows);
	ShowControl(sp->ctlMailBox);
	SetControlTitle(sp->ctlSearch,GetRString(s,STOP_BUTTON));
#ifdef VALPHA
	sp->nSearched = sp->nTicks = 0;
#endif
	UL(sh);
	win->isDirty = true;
}

/**********************************************************************
 * SetupSearchCriteria - setup search criteria
 **********************************************************************/
static Boolean SetupSearchCriteria(MyWindowPtr win,SearchPtr sp)
{
	short	i;
	Str255	s;
	Str255	cs;
	long junk;
	LongDateRec	date;
	PersHandle	pers;
	short		orderIdx;
	
	if (HasFeature (featureSearch)) {
		sp->matchAny = GetControlValue(sp->ctlMatch) == matchAny;
		sp->searchResults = GetControlValue(sp->ctlSrchResults) > 0;
	}
	sp->criteriaFlags = 0;
	sp->noBulkSearch = false;
	
	for(i=0;i<sp->criteriaCount;i++)
	{
		CriteriaInfo	criteria=(*sp->hCriteria)[i];
		short	value;
	
		criteria.specifier = 0;
		criteria.relation = GetControlValue(criteria.ctlRelation);
		if (criteria.ctlSpecifier)
			value = GetControlValue(criteria.ctlSpecifier);
		if (HasFeature (featureSearch) || !gCategoryTable[criteria.category-1].proOnly) {
			switch (criteria.category)
			{
				case scAnywhere:
				case scHeaders:
				case scBody:
				case scAttachNames:
				case scSummary:
				case scTo:
				case scFrom:
				case scSubject:
				case scCC:
				case scBCC:
				case scAnyRecipient:
					// get text
					PeteString(s,criteria.pte);
					ZapHandle(criteria.searchString);
					criteria.searchString = criteria.relation != strRegExp ? NewString(s) : (StringHandle)regcomp(PtoCcpy(cs,s));
					criteria.fudge8bit = criteria.relation != strRegExp && MixedHighBits(s+1,*s);
					if (!criteria.searchString) return false;
					HLock((Handle)criteria.searchString);
					sp->noBulkSearch = sp->noBulkSearch || PrefIsSet(PREF_NO_BULK_SEARCH) && AllHighBits(s+1,*s);
					
					//	Should we set up the default find string?
					if (i==0 && criteria.relation != strDoesNotContain && criteria.relation != strIsNot)
						FindEnterSelection(s,false);
					
					// Bug 4262 came from doing this before entering the find string...
					if (criteria.relation != strRegExp && AnyHighBits(s+1,*s))
					{
						RomanToUTF8(s,sizeof(s));
						criteria.utf8SearchString = NewString(s);
					}
					break;
				case scStatus:
					criteria.specifier = Item2Status(value);
					break;
				case scPriority:
					criteria.specifier = Display2Prior(value);
					break;
				case scAge:
					PeteString(s,criteria.pte);
					StringToNum(s,&criteria.misc);
					criteria.specifier = value;
					criteria.zoneSecs = ZoneSecs();
					break;
				case scAttachCount:
				case scSize:
				case scJunkScore:
					PeteString(s,criteria.pte);
					StringToNum(s,&junk);
					criteria.specifier = junk;
					break;
				case scLabel:
					criteria.specifier = Menu2Label(value);
					break;
				case scDate:
					GetControlData(criteria.ctlSpecifier,0,
						kControlClockLongDateTag,sizeof(date),(Ptr)&date,&junk);
					criteria.date.day = date.ld.day;
					criteria.date.month = date.ld.month;
					criteria.date.year = date.ld.year;
					criteria.zoneSecs = ZoneSecs();
					criteria.useSenderZone = !PrefIsSet(PREF_LOCAL_DATE);
					break;
				case scPersonality:
					if (HasFeature (featureMultiplePersonalities))
						if (pers = value==1 ? PersList : Index2Pers(value-2))
							criteria.specifier = (*pers)->persId;
					break;
			}
			(*sp->hCriteria)[i] = criteria;
			sp->criteriaFlags |= gCategoryTable[criteria.category-1].flags;
		}
		// (jp) 1-18-00 Support for light downgrade
		if (gCategoryTable[criteria.category-1].proOnly)
			UseFeature (featureSearch);
	}
	
	//	Set up search order. Search those that require searching through the message last.

	orderIdx=0;
	for(i=0;i<sp->criteriaCount;i++)
		if (!MustSearchBody((*sp->hCriteria)[i].category))
			sp->searchOrder[orderIdx++] = i;
	for(i=0;i<sp->criteriaCount;i++)
		if (MustSearchBody((*sp->hCriteria)[i].category))
			sp->searchOrder[orderIdx++] = i;
	sp->setup = true;
	return true;
}

/**********************************************************************
 * MustSearchBody - does this criteria category require searching the message body
 **********************************************************************/
static Boolean MustSearchBody(short category)
{
	return (gCategoryTable[category-1].flags & kNeedsMsg) != 0;
}

/**********************************************************************
 * GetSelectedBoxes - build list of selected mailboxes
 **********************************************************************/
static OSErr GetSelectedBoxes(SearchPtr sp,TOCHandle tocH,Boolean bStartIMAPSearch)
{
	ViewListPtr	pView;
	Boolean	haveIMAP = false;
	OSErr	err=noErr;
	
	ZapHandle(sp->BoxCount);
	ZapHandle(sp->imapBoxCount);
	ZapHandle(sp->redoBoxCount);
	if (pView=sp->list)
	{
		BoxCountHandle	saveBoxCount,popBoxCount,imapBoxCount=nil;
		short		i;
		short	n;

		n=LVCountSelection(pView);
		if (!n)
		{
			//	No mailboxes are selected. Select ALL of them.
			LVSelectAll(pView);
			n=LVCountSelection(pView);
		}
		saveBoxCount = BoxCount;
		popBoxCount = NuHandle(0);
		
		// build our own box count
		for(i=1;i<=n;i++)
		{
			VLNodeInfo itemInfo;
			
			//	get next selected item
			if (!LVGetItem(pView,i,&itemInfo,true)) break;
			
			BoxCount = popBoxCount;

			if (IsIMAPBox(&itemInfo))
			{
				if (sp->criteriaFlags & kNeedsMsg)
				{
					//	Need to do an IMAP on server search
					if (!imapBoxCount)
						imapBoxCount=NuHandle(0);
					BoxCount = imapBoxCount;
				}
				haveIMAP = true;
			}
		
			if ((itemInfo.nodeID == MAILBOX_MENU || itemInfo.nodeID ==kEudoraFolder) && 
				itemInfo.refCon == MailRoot.dirId && itemInfo.isParent && itemInfo.isCollapsed)
			{
				//	Eudora folder
				//	add entire Eudora folder if collapsed
				AddBoxCountItem(MAILBOX_IN_ITEM,MailRoot.vRef,MailRoot.dirId);
				AddBoxCountItem(MAILBOX_OUT_ITEM,MailRoot.vRef,MailRoot.dirId);
				AddBoxCountItem(MAILBOX_JUNK_ITEM,MailRoot.vRef,MailRoot.dirId);
				AddBoxCountItem(MAILBOX_TRASH_ITEM,MailRoot.vRef,MailRoot.dirId);
				AddBoxCountMenu(MAILBOX_MENU,MAILBOX_FIRST_USER_ITEM,MailRoot.vRef,MailRoot.dirId,false);
			}
			else
			{
				MenuHandle	mh;
				short	menuItem;
				short		menuID;
				short		vRef;
				long		dirId;

				if (itemInfo.nodeID==kEudoraFolder) continue;	//	Ignore Eudora Folder
				
				menuID = itemInfo.nodeID==kIMAPFolder ? MAILBOX_MENU : itemInfo.nodeID;
				mh = GetMHandle(menuID);
				menuItem = FindItemByName(mh,itemInfo.name);
				if (!menuItem) continue;	//	shouldn't happen
				
				if (itemInfo.isParent)
				{
					// mailbox folder
					if (haveIMAP && itemInfo.iconID == IMAP_MAILBOX_FILE_ICON)
					{
						//	IMAP hybrid mailbox/mailfolder. Need to add it
						MenuID2VD(menuID,&vRef,&dirId);
						AddBoxCountItem(menuItem,vRef,dirId);
					}
					if (itemInfo.isCollapsed)
					{
						//	add entire folder contents if collapsed
						menuID = SubmenuId(mh,menuItem);
						MenuID2VD(menuID,&vRef,&dirId);
						AddBoxCountMenu(menuID,(menuID==MAILBOX_MENU) ? 1: MAILBOX_FIRST_USER_ITEM-MAILBOX_BAR1_ITEM,vRef,dirId,false);
					}
				}
				else
				{
					// mailbox					
					MenuID2VD(menuID,&vRef,&dirId);
					AddBoxCountItem(menuItem,vRef,dirId);
				}
			}
		}
		
		sp->BoxCount = popBoxCount;
		sp->imapBoxCount = imapBoxCount;
		BoxCount = saveBoxCount;

		if (haveIMAP && bStartIMAPSearch)
		{
			if (sp->criteriaFlags & kNeedsSum && !(sp->criteriaFlags & kNeedsMsg))
			{
				//	Warn user we are only searching local cache for summary items
				if (ComposeStdAlert(Caution,IMAP_SEARCH_LOCAL_WARN)==2)
					err = userCanceledErr;
			}
			
			if ((!sp->searchResults))
			{
				// start the IMAP searches ...
				if (imapBoxCount)
				{
					//	Start IMAP search
					IMAPSCHandle searchCriteria;
					
					if (searchCriteria = SetupIMAPSearchCriteria(sp))
					{
						// IMAPSearch will return false if no searches were actually started.
						if (IMAPSearch(tocH,imapBoxCount,searchCriteria,!sp->matchAny))
							sp->searchMode |= kSearchingIMAP;
						else 
							err = userCanceledErr;
						
						if (err!=userCanceledErr) 
						{
							ZapHandle(sp->imapBoxSpecIdx);
							sp->imapBoxSpecIdx = NuHandleClear(HandleCount(imapBoxCount)*sizeof(short));
						}
					}
				}
			}
		}	
	}

	return err;
}

/**********************************************************************
 * ZapSpecList - zap Spec List handle
 **********************************************************************/
static void ZapSpecList(TOCHandle toc)
{
	FSSpecHandle	specList;

	if (specList = (*toc)->mailbox.virtualMB.specList)
	{
		ZapHandle(specList);
		(*toc)->mailbox.virtualMB.specList = nil;
	}
	(*toc)->mailbox.virtualMB.specListCount = 0;
}

/**********************************************************************
 * StopSearch - stop the search
 **********************************************************************/
static void StopSearch(MyWindowPtr win)
{
	SearchHandle	sh;
	short			i;
	Str32			s;
	Boolean			gotHits;
	SearchMarker	curr;
	CriteriaHandle	hCriteria;
	TOCHandle		tocH;

	GetSearchInfo(win,&tocH,&sh);
	if (!(*sh)->searchMode) return;
	
	ActiveSearchCount--;
	
	StopBulkSearch(sh);
	if ((*sh)->searchMode & kSearchingIMAP)
		IMAPAbortSearch(tocH);

	curr = (*sh)->curr;
	FinishedTOC(&curr);
	(*sh)->curr = curr;

	SetControlTitle((*sh)->ctlSearch,GetRString(s,SEARCH_BUTTON));
	HideControl((*sh)->ctlChaseArrows);
	HideControl((*sh)->ctlMailBox);
	ShowHideTabs((*sh)->ctlTabs,true);
	(*sh)->searchMode = 0;
	SetTextControlText((*sh)->ctlMailBox,"",nil);

	// unlock search string handles
	// dispose of accumulators
	hCriteria = (*sh)->hCriteria;
	for(i=0;i<(*sh)->criteriaCount;i++)
	{
		StringHandle	h;

		if (h = (*hCriteria)[i].searchString)
			HUnlock((Handle)h);
		SetHits(sh,i,nil);	//	dispose of bulk search hits
	}

	gotHits = (*sh)->nHits>0;
	if (HasFeature (featureSearch)) {
		SetControlVisibility((*sh)->ctlSrchResults,gotHits,true);
		if (!gotHits)
			SetControlValue((*sh)->ctlSrchResults,0);
	}
	
#if __profile__
//	ProfilerDump("\psearch-profile");
//	ProfilerClear();
#endif

	//	log search info
	if (LOG_SEARCH&LogLevel)
	{
		long	nSelMB=0;
		
		//	search stats
		if ((*sh)->BoxCount)
			nSelMB = GetHandleSize_((*sh)->BoxCount)/sizeof(BoxCountElem);
#ifdef VALPHA
		ComposeLogS(LOG_SEARCH,nil,"\pHits: %d/%d Seconds: %d.%d, Mailboxes: %d/%d",
			(*sh)->nHits,(*sh)->nSearched,
			(*sh)->nTicks/60,(*sh)->nTicks%60,
			nSelMB,GetHandleSize_(BoxCount)/sizeof(BoxCountElem));
#endif

		//	criteria
		for(i=0;i<(*sh)->criteriaCount;i++)
		{
			CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];
			Str32			sCategory,sRelation;
			Str255			sSpecifier;
			
			GetMenuItemText(GetPopupMenuHandle(criteria.ctlCategory),criteria.category,sCategory);
			GetMenuItemText(GetPopupMenuHandle(criteria.ctlRelation),criteria.relation,sRelation);
			if (criteria.searchString)
			{
				PCopy(sSpecifier,*criteria.searchString);
				ComposeLogS(LOG_SEARCH,nil,"\p %d: %p %p \"%p\"",i+1,sCategory,sRelation,sSpecifier);
			}
			else
			{
				ComposeLogS(LOG_SEARCH,nil,"\p %d: %p %p %d",i+1,sCategory,sRelation,criteria.specifier);
			}
		}
	}
}

/**********************************************************************
 * AdjustForCriteriaCount - adjust controls for number of criteria
 **********************************************************************/
static void AdjustForCriteriaCount(SearchHandle sh)
{
	if (HasFeature (featureSearch)) {
		SetControlVisibility((*sh)->ctlFewer,(*sh)->criteriaCount>1,true);
		SetControlVisibility((*sh)->ctlMatch,(*sh)->criteriaCount>1,true);
	}
}

/**********************************************************************
 * SearchButton - hit one of our controls
 **********************************************************************/
static void SearchButton(MyWindowPtr win,ControlHandle button,long modifiers,short part)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	SearchHandle	sh;
	SearchPtr		sp;
	TOCHandle 		toc;
	long 			controlId = -1;
	Rect			rWin;
	
	GetSearchInfo(win,&toc,&sh);
	sp=*sh;
	
	
	if (button==sp->ctlSearch)
	{
		if (sp->searchMode)
			StopSearch(win);
		else
			StartSearch(win);
		
		controlId = kctlSearch;
	}
	else if (button==sp->ctlMore)
	{
		Rect	rNone;
		
		controlId = kctlMore;

		if ((*sh)->criteriaCount>=kMaxCriteria)
		{
			SysBeep(0);
			return;	// enough criteria
		}
		StopSearch(win);
		AddCriteria(win,sh,false);
	  SetCriteria:
		AdjustForCriteriaCount(sh);
		SetRect(&rNone,0,0,0,0);
		ClipRect(&rNone);
		SetSearchTopMargin(win,sh);
		MyWindowDidResize(win,&win->contR);
		InfiniteClip(GetMyWindowCGrafPtr(win));
		InvalWindowRect(winWP,GetWindowPortBounds(winWP,&rWin));
		win->isDirty = true;
		SearchSetWTitle(win,sh);
	}	
	else if (button==sp->ctlFewer)
	{
		controlId = kctlFewer;

		if ((*sh)->criteriaCount >= 2)
		{
			StopSearch(win);
			RemoveCriteria(win,sh);
			goto SetCriteria;
		}
	}	
	else if (button==sp->ctlTabs)
	{
		//	Toggle view of mailboxes
		ControlHandle	cntl,placard;
		short			i;
		Boolean			boxCtlsVisible;
		PETEHandle		 previewPTE;
		Rect 			oldContR = win->contR;
		Rect	rNone;
		
		controlId = kctlTabs;

		SetRect(&rNone,0,0,0,0);
		ClipRect(&rNone);

		(*sh)->mailboxView = GetControlValue(button)==tabMailboxes;
		boxCtlsVisible = !(*sh)->mailboxView;
		for (i=1;i<BoxLinesLimit;i++)		//	hide or show mailbox header buttons
			if (cntl = FindControlByRefCon(win,'wide'+i))
				SetControlVisibility(cntl,boxCtlsVisible,true);
		SetControlVisibility(win->vBar,boxCtlsVisible,true);
		SetControlVisibility(win->hBar,boxCtlsVisible,true);
		if ((cntl=FindControlByRefCon(win,PREVIEW_TOGGLE_CNTL)) && !GetSuperControl(cntl,&placard))
			SetControlVisibility(placard,boxCtlsVisible,true);
		if ((cntl=FindControlByRefCon(win,PREVIEW_DIVIDE_CNTL)) && !GetSuperControl(cntl,&placard))
			SetControlVisibility(placard,boxCtlsVisible,true);
		if (cntl=FindControlByRefCon(win,kBoxSizeRefCon))
			SetControlVisibility(cntl,boxCtlsVisible,true);
		if (cntl=FindControlByRefCon(win,kConConProfRefCon))
			SetControlVisibility(cntl,boxCtlsVisible,true);
		if (previewPTE = (*toc)->previewPTE)
		{
			//	make preview pane visible/invisible by moving it
			Rect	r;
			short	offset;
			
			PeteRect(previewPTE,&r);
			offset = boxCtlsVisible ? 4000 : -4000;
			OffsetRect(&r,offset,offset);
			PeteDidResize(previewPTE,&r);
		}
		
		if ((*sh)->mailboxView)
		{
			(*sh)->saveBotMargin = win->botMargin;
			(*sh)->savePreviewHi = (*toc)->previewHi;
			SetBotMargin(win,0);
			(*toc)->previewHi = -1;
		}
		else
		{
			(*toc)->previewHi = (*sh)->savePreviewHi;
			SetBotMargin(win,(*sh)->saveBotMargin);
			GetSelectedBoxes(*sh,toc,false);	// This isn't quite the right place, but it's better than nothing
		}
		
		// Make sure to clear pView field if in results
		win->pView = (*sh)->mailboxView ? (*sh)->list : 0;
		
		InvalBotMargin(win);
//		if ((*sh)->mailboxView && !(*sh)->list)
//			InitMailboxList(win,sh);
		SetControlVisibility((*(*sh)->list->hList)->vScroll,!boxCtlsVisible,true);
		
		// make sure the mailbox list is active.  Certain IMAP dialogs might have deactivated it.  -jdboyd 7/26/04
		if (!boxCtlsVisible) LVActivate((*sh)->list, win->isActive);
		
		SetSearchTopMargin(win,sh);
		MyWindowDidResize(win, &oldContR);

		InfiniteClip(GetMyWindowCGrafPtr(win));
		InvalWindowRect(winWP,GetWindowPortBounds(winWP,&rWin));
	}
	else if (button==sp->ctlSrchResults)
	{
		StopSearch(win);
		SetControlValue(button,!GetControlValue(button));
		controlId = kctlSrchResults;
	}
	else if (ClickCriteriaButton(win,sh,button,part))
	{
		StopSearch(win);
		win->isDirty = true;
		SearchSetWTitle(win,sh);
		controlId = kctlCriteria;
	}
	else if (!(*sh)->mailboxView)
		BoxButton(win,button,modifiers,part);	
	else if (button==sp->ctlMatch)
		controlId = kctlMatch;
		
	if (controlId >= 0)
		AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(winWP), AUDITCONTROLID(GetWindowKind(winWP),controlId), mouseDown);
}


/************************************************************************
 * ClickCriteriaButton - check for click in a criteria button
 ************************************************************************/
static Boolean ClickCriteriaButton(MyWindowPtr win,SearchHandle sh,ControlHandle cntl,short part)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	short	i;
	Rect	r;
	
	for(i=0;i<(*sh)->criteriaCount;i++)
	{
		CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];
		short			top = INSET+6+i*kCriteriaHt;
		Boolean			resize = false;

		if (cntl == criteria.ctlCategory)
		{
			// change in category means changing relation and specifier
			short	category = GetControlValue(cntl);
			short	relation = gCategoryTable[category-1].relation;
			short	specifier = gCategoryTable[category-1].specifier;
			
			if (relation != gCategoryTable[criteria.category-1].relation)
			{
				// different relation menu
				DisposeControl(criteria.ctlRelation);
				criteria.ctlRelation = CreateMenuControl(win,win->topMarginCntl,"\p",relation,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,1,false);
				resize = true;
			}
			
			if (specifier != gCategoryTable[criteria.category-1].specifier)
			{
				short	defaultSpecifierItem = 0;
				
				DisposeSpecifier(win,sh,&criteria);
				switch (specifier)
				{
					case ssText:
					case ssNumber:
						CreateTextSpecifier(win,&criteria,specifier!=ssText);
						break;
					
					case ssDate:
						SetRect(&r,-4000,-4000,kDateWi-4000,kDateHt-4000);
						criteria.ctlSpecifier = NewControl(winWP,&r,"\p",true,0,0,0,kControlClockDateProc,0);
						//LetsGetSmall(criteria.ctlSpecifier);
						break;
					
					case ssAge:
						//	needs both text and specifier
						CreateTextSpecifier(win,&criteria,true);
						SetControlValue(criteria.ctlRelation,GetRLong(SEARCH_DF_AGE_REL));
						PeteSetString(GetRString(P1,SEARCH_DF_AGE),criteria.pte);
						specifier = SRCH_AGE_UNITS_MENU;
						defaultSpecifierItem = GetRLong(SEARCH_DF_AGE_SPFY);
						break;
						
					default:
						defaultSpecifierItem = 1;
						break;
				}
				if (defaultSpecifierItem)
					criteria.ctlSpecifier = CreateMenuControl(win,win->topMarginCntl,"\p",specifier,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,defaultSpecifierItem,false);
				resize = true;
			}
//			else if (specifier==ssText)
//			{
//				// remove any existing text
//				PeteDelete(criteria.pte,0,0x7fffffff);
//			}
			
			// prefill attachment count
			if (category == scAttachCount)
			{
						SetControlValue(criteria.ctlRelation,GetRLong(SEARCH_DF_ATT_REL));
						PeteSetString(GetRString(P1,SEARCH_DF_ATT),criteria.pte);
			}
			criteria.category = category;
			(*(*sh)->hCriteria)[i] = criteria;
			if (resize)
				ResizeCriterion(win,sh,i);
			EmbedSearchCriteriaControls(&criteria,sh);
			return true;
		}
		else if (cntl == criteria.ctlSpecifier)
		{
			if (criteria.category==scDate)
			{
				//	This control needs focus
				SetKeyboardFocus(winWP,cntl,part);
				(*sh)->ctlFocus = cntl;
				RemoveFocus(win);
			}
			return true;
		}
		else if (cntl == criteria.ctlRelation)
			// don't worry about this for now
			return true;
	}
	return false;
}

/************************************************************************
 * SearchLVCallBack - callback function for List View
 ************************************************************************/
static long SearchLVCallBack(ViewListPtr pView, VLCallbackMessage message, long data)
{
	VLNodeInfo	*pInfo;
	OSErr	err = noErr;
	SearchHandle	sh;
	DEC_STATE(sh);
	
	switch (message)
	{
		case kLVAddNodeItems:
		case kLVExpandCollapseItem:
			GetSearchInfo(pView->wPtr,nil,&sh);
			L_STATE(sh);
			if (message==kLVAddNodeItems)
				AddMailboxListItems(pView,data,&(*sh)->expandList);
			else
				SaveExpandStatus((VLNodeInfo *)data,&(*sh)->expandList);
			U_STATE(sh);
			break;

		case kLVGetParentID:
			err = MailboxesLVCallBack(pView, message, data);
			break;
		
		case kLVOpenItem:
			GetSearchInfo(pView->wPtr,nil,&sh);
			MBListOpen((*sh)->list);
			break;

		case kLVQueryItem:
			pInfo = (VLNodeInfo*)data;
			switch (pInfo->query)
			{
				case kQuerySelect:
					return true;
				case kQueryDrag:
				case kQueryRename:
				case kQueryDrop:
				case kQueryDropParent:
				case kQueryDragExpand:
					return false;
				default:
					err = MailboxesLVCallBack(pView, message, data);
			}
			break;
		
		case kLVGetFlags:
			return gLVFlags;
	}
	return err;
}

/************************************************************************
 * InitMailboxList - set up the mailbox list
 ************************************************************************/
static ViewListPtr InitMailboxList(MyWindowPtr win,Boolean useMBSelection)
{
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	ViewListPtr	pList=NuPtr(sizeof(ViewList));
	Rect	r;
	GrafPtr	oldPort;
	
	GetPortBounds(winPort,&r);
	GetPort(&oldPort);
	SetPort_(winPort);

	r.top = win->topMargin;
	if (pList)
	{
		LVNew(pList,win,&r,0,SearchLVCallBack,0);
		if (useMBSelection)
		{
			//	Select same mailboxes as in Mailboxes window
			ViewListPtr	pMBList = MBGetList();
			short	i;
			VLNodeInfo data;
			
			for (i=1;i<=LVCountSelection(pMBList);i++)
			{
				if (LVGetItem(pMBList,i,&data,true))
					LVSelect(pList,data.nodeID,data.name,true);
			}
		}
	}
	SetPort_(oldPort);

	return pList;
}

/**********************************************************************
 * SizeMBList - set size of mailbox list
 **********************************************************************/
static void SizeMBList(MyWindowPtr win,SearchHandle sh)
{
	Rect	r;
	
	if ((*sh)->list)
	{
		CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
		GetPortBounds(winPort,&r);
		r.top = win->topMargin;
		r.bottom -= GROW_SIZE;
		if (!(*sh)->mailboxView)
			OffsetRect(&r,-4000,-4000);	// hide the list
		LVSize((*sh)->list,&r,nil);
	}
}


/************************************************************************
 * SearchIdle - do actual search
 ************************************************************************/
static void SearchIdle(MyWindowPtr win)
{
	SearchHandle	sh;
	TOCHandle	toc;
	short		sumNum;

	GetSearchInfo(win,&toc,&sh);
	if (!(*sh)->mailboxView)
		BoxIdle(win);
	
	SearchSetControls(win, sh);

	if (((*sh)->searchMode & kSearchingLocal) && (!(*sh)->redoTicks || ((*sh)->redoTicks <= TickCount())))
		ProcessSearch(win,toc,sh);
	else if ((*sh)->searchMode & kSearchingIMAP)
		ProcessIMAPSearch(win,toc,sh);
	
	if ((*toc)->previewID && 
		(sumNum=FirstMsgSelected(toc))>=0 && 
		(*toc)->sums[sumNum].offset < 0)
	{
		//	We have an IMAP message in preview pane waiting for download
		TOCHandle	realTOC;
		short		realSumNum;

		realTOC = GetRealTOC(toc,sumNum,&realSumNum);
		UpdateIMAPMailbox(realTOC);
		if ((*realTOC)->sums[realSumNum].offset >= 0)
		{
			//	The message has arrived. Show it.
			(*toc)->previewID = 0;
			
			//	Make sure we don't download it again
			(*toc)->sums[sumNum].offset = (*realTOC)->sums[realSumNum].offset;
		}
	}

	if ((*toc)->resort)
	{
		// Need to resort
		(*toc)->resort = kResortWhenever;
		RedoTOC(toc);
	}
		
	// take care of any incremental IMAP searches
	UpdateIncrementalIMAPSearches();
}

/************************************************************************
 * Call the idle routines of all search windows
 ************************************************************************/
void SearchAllIdle(void)
{
	WindowPtr winWP;
	MyWindowPtr win;
	SearchHandle sh;
	
	for (winWP = GetWindowList(); winWP; winWP = GetNextWindow (winWP)) {
		win = GetWindowMyWindowPtr (winWP);
		if (IsSearchWindow(winWP))
		{
			GetSearchInfo(win,nil,&sh);
			if ((*sh)->searchMode) SearchIdle(win);
		}
	}
}


/************************************************************************
 * ProcessSearch - do actual search
 ************************************************************************/
static void ProcessSearch(MyWindowPtr win,TOCHandle toc,SearchHandle sh)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	uLong		endTicks,maxTicks;
	SearchMarker	curr;
	Boolean		stop = false;
	TOCHandle 	aTOC;
	Boolean		foundTOC = false;
	FSSpec		spec;
	Str255		s;
	Str63		name;
	GrafPtr oldPort;
	Boolean		inFront=false,match;
	Boolean		invalidateBoxSize = false;
#ifdef VALPHA
	long		startTicks;
#endif

#if __profile__
//	ProfilerSetStatus(true);
#endif

#ifdef VALPHA
	startTicks = TickCount();
#endif

	GetPort(&oldPort);
	SetPort_(GetWindowPort(winWP));

	if ((*sh)->bulkSearching)
	{
		DoBulkSearch(sh);
		if ((*sh)->bulkSearching)
			//	bulk search was interrupted again
			return;
	}
		

	// validate the saved toc, if any
	curr = (*sh)->curr;
	if (!(*sh)->searchResults && curr.tocH)
	{
		if (!curr.opened)
		{
			// make sure our saved TOC is still valid		
			for (aTOC=TOCList;aTOC;aTOC=(*aTOC)->next)
				if (aTOC == curr.tocH) foundTOC = true;
		}
		else
		{
			ASSERT(*curr.tocH!=(TOCPtr)-1);	// ACK!!!  Freed?
			foundTOC = *curr.tocH!=(TOCPtr)-1;
		}
		if (!foundTOC)
		{
			// this TOC has been disposed of, need to open it again
			GetBoxTOC(sh,&curr);
			(*sh)->curr = curr;
		}
	}
	
	if (InBG)
		maxTicks = kInBGTicks;
	else
	{
		if (FrontWindow_()==winWP)
		{
			maxTicks = kFrontWinTicks;
			inFront = true;
		}
		else
			maxTicks = kBgdWinTicks;
	}
	endTicks = TickCount() + maxTicks;
	while(TickCount() < endTicks && !EventPending() && (!inFront || !CommandPeriod))
	{
		MSumType sum;
		Boolean	found=false;
		
		CheckChaseArrows(sh);
		while (!curr.tocH || curr.message >= (*curr.tocH)->count)
		{
			//	Done with this mailbox
			if ((*sh)->searchResults)
			{
				//	searching through the results. we're finished
				if (curr.tocH) goto done;
				curr.tocH = toc;
				curr.message = 0;
			}
			else
			{
				//	do next mailbox
				short		i;
				CriteriaHandle	hCriteria = (*sh)->hCriteria;;
				
				FinishedTOC(&curr);
				curr.box++;
				curr.tocH = 0;
				curr.message = 0;
				if (curr.box>=GetHandleSize_((*sh)->BoxCount)/sizeof(BoxCountElem))
				{
					//	end of mailboxes!
					(*sh)->redoTicks = 0;
					if ((*sh)->redoBoxCount)
					{
						SearchPtr		sp;
						
						//	There were some mailboxes that were busy. Try them again
						ZapHandle((*sh)->BoxCount);
						sp = *sh;
						sp->BoxCount = sp->redoBoxCount;
						sp->redoBoxCount = nil;
						Zero(sp->curr);
						sp->curr.lastBoxSpec = -2;
						sp->curr.box = -1;
						(*sh)->redoTicks = TickCount() + 120;	//	Try again in a couple of seconds
						goto dontstop;
					}
					else
						//	We're all done
						goto done;
				}

				//	reset all hits accumulators
				for(i=0;i<(*sh)->criteriaCount;i++)
					SetHits(sh,i,nil);
				(*sh)->bulkData.didBulkSearch = false;
				
				//	If we are searching for a string in a single criterion, do the bulk
				//	search now. If nothing found, skip this mailbox
				if ((*sh)->criteriaCount==1 && 
					(*hCriteria)->searchString &&
					(*hCriteria)->category != scSummary)
				{					
					PCopy(s,*(*hCriteria)->searchString);
					if (GetBoxCountSpec(sh,curr.box,&spec))
					{
						DisplayMBName(sh,spec.name);
						if (SearchBulk(&spec,sh) == opWrErr)
						{
							//	This mailbox is busy right now. We'll come back to it.
							if (!(*sh)->redoBoxCount) (*sh)->redoBoxCount = NuHandle(0);
							if ((*sh)->redoBoxCount)
							{
								BoxCountHandle	boxList = (*sh)->BoxCount ? (*sh)->BoxCount : BoxCount;
								BoxCountElem bce = (*boxList)[curr.box];
								PtrPlusHand_(&bce,(*sh)->redoBoxCount,sizeof(bce));
							}
							break;
						}
						if ((*hCriteria)->hits.offset==0 && !(*sh)->bulkSearching && !(*sh)->noBulkSearch)
						{
							//	skip this mailbox, nothing found in bulk search
							break;
						}
					}
				}
				
				if (GetBoxTOC(sh,&curr))
				{
					(*sh)->curr = curr;
					GetMailboxName((*sh)->curr.tocH,0,name);
					DisplayMBName(sh,name);
				}
				if ((*sh)->bulkSearching)
					break;	//	Interrupted a bulk search.
			}
		}
		
		if (!curr.tocH)
			continue;	//	Did a bulk search and found nothing
		
		// search summary
		sum = (*curr.tocH)->sums[curr.message];
		(*sh)->curr = curr;	
#ifdef VALPHA
		(*sh)->nSearched++;
#endif
		match = SearchMessage(win,&sum,sh,&curr,&stop,false);
		if (stop)
			goto done;
		if ((*sh)->bulkSearching)
			break;	//	Interrupted a bulk search.
		if ((*sh)->searchResults)
		{
			if (match)
				(*sh)->nHits++;
			else
			{
				//	no match on searching results. remove this one.
				DeleteSum(curr.tocH,curr.message);
				invalidateBoxSize = true;
				curr.message--;			
			}
		}
		else if (match)
		{
			// add a message summary to search window toc
			if (AddSearchResult(sh,toc,curr.tocH,curr.message,curr.lastBoxSpec != curr.box,-1))
			{
				stop = true;
				goto done;	//	error
			}
			curr.lastBoxSpec = curr.box;
			invalidateBoxSize = true;
		}
		curr.message++;
		if (stop) goto done;
	}
	stop = inFront && (CommandPeriod || HasCommandPeriod());
	(*sh)->curr = curr;	

	if (stop)
	{
 done:
		(*sh)->curr = curr;
		if ((*sh)->searchMode != kSearchingLocal)
		{
			//	Stop local search, continue with IMAP
			(*sh)->searchMode &= ~kSearchingLocal;
			SetTextControlText((*sh)->ctlMailBox,GetRString(s,SEARCHING_SERVER),nil);
		}
		else
			//	Stop everything
			StopSearch(win);
	}
  dontstop:
	if (invalidateBoxSize)
		UpdateBoxSize(win);

#if __profile__
//	ProfilerSetStatus(false);
#endif
	SetPort(oldPort);
#ifdef VALPHA
	(*sh)->nTicks += TickCount()-startTicks;
#endif
}

/**********************************************************************
 * CopySum - copy a summary
 **********************************************************************/
void CopySum(MSumPtr sumFrom, MSumPtr sumTo, short virtualMBIdx)
{
	short	saveSerialNum = sumTo->serialNum;
	Boolean	saveSelected = sumTo->selected;
	Handle	saveCache = sumTo->cache;
	
	*sumTo = *sumFrom;
	sumTo->cache = saveCache;
	sumTo->mesgErrH = nil;
	sumTo->messH = nil;
	sumTo->selected = saveSelected;
	sumTo->u.virtualMess.virtualMBIdx = virtualMBIdx;
	sumTo->u.virtualMess.linkSerialNum = sumFrom->serialNum;
	sumTo->serialNum = saveSerialNum;
}

/**********************************************************************
 * AddSearchResult - add this search result
 **********************************************************************/
static OSErr AddSearchResult(SearchHandle sh, TOCHandle srchTOC, TOCHandle tocH, short sumNum, Boolean addSpec, short specIdx)
{
	MSumType	sum;

	Zero(sum);

	if (addSpec)
	{
		FSSpecHandle specList = (*srchTOC)->mailbox.virtualMB.specList;
		FSSpec	spec;

		if (!specList)
			(*srchTOC)->mailbox.virtualMB.specList = specList = NuHandle(0);
		Zero(spec);
		spec = GetMailboxSpec(tocH,sumNum);
		PtrAndHand(&spec,(Handle)specList,sizeof(spec));
		(*srchTOC)->mailbox.virtualMB.specListCount++;
	}
		
	CopySum(&(*tocH)->sums[sumNum], &sum, specIdx<0 ? (*srchTOC)->mailbox.virtualMB.specListCount-1 : specIdx);
	sum.serialNum = (*srchTOC)->nextSerialNum++;
	if ((*srchTOC)->count)
	{
		OSErr	err;
		
		if (err = PtrPlusHand_(&sum,srchTOC,sizeof(sum)))
		{
			WarnUser(MEM_ERR,err);
			return err;
		}
	}
	else
		(*srchTOC)->sums[0] = sum;

	InvalSum(srchTOC,(*srchTOC)->count++);			
	(*sh)->nHits++;

	return noErr;
}


/**********************************************************************
 * UpdateBoxSize - redisplay box size (# of messages in search window)
 **********************************************************************/
static void UpdateBoxSize(MyWindowPtr win)
{
	ControlHandle	boxSizeCntl;
	Rect			rCtl;
	
	if (boxSizeCntl = FindControlByRefCon(win,kBoxSizeRefCon))
	{
		GetControlBounds(boxSizeCntl,&rCtl);
		SetPort_(GetMyWindowCGrafPtr(win));	//	someone keeps changing this
		InvalWindowRect(GetMyWindowWindowPtr(win),&rCtl);
	}
}

/**********************************************************************
 * SearchText - search for string in text
 **********************************************************************/
static Boolean SearchText(StringPtr value,UPtr pText,long len,long offset,short relation)
{
	switch (relation)
	{
		case strContains:
			return SearchStrPtr(value,pText,offset,len,false,false,nil) >= 0;
		case strContainsWord:
			return SearchStrPtr(value,pText,offset,len,false,true,nil) >= 0;		
		case strDoesNotContain:
			return SearchStrPtr(value,pText,offset,len,false,false,nil) < 0;
		case strIs:
			return *value==len && SearchStrPtr(value,pText,offset,*value,false,false,nil) >= 0;
		case strIsNot:
			return *value!=len || SearchStrPtr(value,pText,offset,*value,false,false,nil) < 0;
		case strStarts:
			return *value<=len && SearchStrPtr(value,pText,offset,*value,false,false,nil) >= 0;
		case strEnds:
			return *value<=len && SearchStrPtr(value,pText,offset+len-*value,len,false,false,nil) >= 0;
		case strRegExp:
			return SearchRegExpPtr(value,pText,offset,len) >= 0;
	}
	return false;
}

/**********************************************************************
 * SearchString - search for string in string
 **********************************************************************/
static Boolean SearchString(StringPtr value,StringPtr s,short relation)
{
	return SearchText(value,s+1,*s,0,relation);
}

/**********************************************************************
 * SearchMessage - search this message summary, return true if found
 **********************************************************************/
static Boolean SearchMessage(MyWindowPtr win,MSumPtr sumP,SearchHandle sh,SearchMarker *curr,Boolean *stop,Boolean sumOnly)
{
	short	i;
//	MyWindowPtr messWin;
	CriteriaInfo	criteria;
	long		value;
	DateTimeRec	date,currDate;
	long		secs,sumSecs,result;
	Str255		string;
	short		label;
	TOCHandle	tocH = curr->tocH;
	long		mNum = curr->message;
	FSSpec		spec;
	Boolean		found;
	Handle		hText;
	PETEHandle	tempPte;
	long		textSize;
	Boolean		disposeText;
	Ptr			pText;
	long		offset;
	short		hdrID;
	Boolean		notInBulk;
	FindAttData attData;
	Str127 searchString;
	
	// shouldn't be here unless the search has already been setup
	ASSERT((*sh)->setup);
	if (!(*sh)->setup) return false;
	
	for(i=0;i<(*sh)->criteriaCount;i++)
	{
		criteria=(*(*sh)->hCriteria)[(*sh)->searchOrder[i]];
		
		found = false;
		if (criteria.searchString && !**criteria.searchString && criteria.relation!=strRegExp)
			//	ignore any blank search strings
			continue;
		if (sumOnly && !(gCategoryTable[criteria.category-1].flags & kNeedsSum))
			continue;	//	Doing summaries only. Did IMAP server search
			
		switch (criteria.category)
		{			
			case scSummary:				
				// if the summary is utf8, use the utf8 search string
				if ((sumP->flags&FLAG_UTF8)!=0 && criteria.utf8SearchString)
					PSCopy(searchString,*criteria.utf8SearchString);
				else
					PSCopy(searchString,*criteria.searchString);

				// search subject, from, date, and label in summary info
				if (criteria.relation == strDoesNotContain)
					//	for "does not contain" all must be true for a true result
					found = SearchString(searchString,sumP->subj,criteria.relation) &&
						SearchString(searchString,sumP->from,criteria.relation) &&
						SearchString(searchString,ComputeLocalDate(sumP,string),criteria.relation);				
				else
					found = SearchString(searchString,sumP->subj,criteria.relation) ||
						SearchString(searchString,sumP->from,criteria.relation) ||
						SearchString(searchString,ComputeLocalDate(sumP,string),criteria.relation);
				if (!found && (label = SumColor(sumP)))
				{
					RGBColor color;
					MyGetLabel(label,&color,string);
					if (SearchString(*criteria.searchString,string,criteria.relation))
						found = true;
				}
				break;

			case scStatus:
				value = sumP->state;
				goto DoEqual;
				
			case scPriority:
				value = sumP->priority;
				if (!value) value = Display2Prior(3);
				result=ShortCompare(criteria.specifier,value); // for priority to backward compare
				goto DoCompare;
			
			case scAttachCount:
				if (sumP->flags & FLAG_HAS_ATT)
				{
					if (criteria.specifier==0)
						// Don't need to count them. If compare to zero, any number will do.
						value=1;
					else
					{
						value = 0;
						CacheMessage(tocH,mNum);
						if (hText=(*tocH)->sums[mNum].cache)
						{
							HNoPurge(hText);
							InitAttachmentFinder(&attData,hText,true,tocH,&(*tocH)->sums[mNum]);
							while (GetNextAttachment(&attData,&spec))
								value++;
						}
					}
				}
				else
					value = 0;
			  DoCompareValue:
				result=ShortCompare(value,criteria.specifier);
				goto DoCompare;
				
			case scLabel:
				value = SumColor(sumP);
			  DoEqual:
			  	found = criteria.specifier == value ? criteria.relation == srIs : criteria.relation == srIsNot;
				break;
				
			case scAge:
				sumSecs = sumP->seconds + criteria.zoneSecs;	// Use local time
				SecondsToDate(sumSecs,&date);
				GetDateTime(&secs);
				GetTime(&currDate);
				result = DateTimeDifference(&date,&currDate,secs-sumSecs,criteria.specifier) - criteria.misc;
				goto DoCompare;

			case scDate:
				//	Convert GMT seconds to local or sender's time based on preference
				SecondsToDate(sumP->seconds+(criteria.useSenderZone?60*sumP->origZone:criteria.zoneSecs),&date);
				if (!(result=ShortCompare(date.year,criteria.date.year)))
					if (!(result=ShortCompare(date.month,criteria.date.month)))
						result=ShortCompare(date.day,criteria.date.day);
			  DoCompare:
				switch (criteria.relation)
				{
					case srIs:
						found = result==0;
						break;
					case srIsNot:
						found = result!=0;
						break;					
					case srGreater:
						found = result > 0;
						break;										
					case srLess:
						found = result < 0;
						break;
				}									
				break;
			
			case scSize:
				value = (sumP->length+1023)/1024;
				goto DoCompareValue;
			
			case scJunkScore:
				value = sumP->spamScore;
				goto DoCompareValue;
			
			case scPersonality:
				if (HasFeature (featureMultiplePersonalities)) {
					value = sumP->popPersId;
					goto DoEqual;
				}
			case scAttachNames:
				if (!(sumP->flags & FLAG_HAS_ATT))
					// no attachments
					break;
				//	fall thru
			case scAnywhere:
			case scHeaders:
			case scBody:
			case scTo:
			case scFrom:
			case scSubject:
			case scCC:
			case scBCC:
			case scAnyRecipient:
			// search message
				notInBulk = !(*sh)->searchResults && !(*sh)->noBulkSearch && !MessPreFind(sh,*criteria.searchString,(*sh)->searchOrder[i]);
				if ((*sh)->bulkSearching)
					return false;	//	bulk search was interrupted
				if (notInBulk)
				{
					if (criteria.relation == strDoesNotContain)
						found = true;
					break;
				}
				
				if (criteria.category==scAnywhere && 
					criteria.relation==strContains &&
					!(*sh)->noBulkSearch && 
					!criteria.utf8SearchString && 
					!(*sh)->searchResults && 
					!((*tocH)->sums[mNum].flags & FLAG_RICH) &&
					!((*tocH)->sums[mNum].opts & OPT_HTML))
				{
					//	if search anywhere, contains, and not HTML or RICH, we don't need to
					//	load and search through the message, bulk search is good enough
					found = true;
					break;
				}
				
				//	get text of message
				tempPte = nil;
				
				{
					//	need to read in message text
					TOCHandle	realTOC;
					short		realSumNum;

					realTOC = GetRealTOC(tocH,mNum,&realSumNum);				
						
					// if this is an IMAP message, make sure it's been downloaded
					if ((*realTOC)->imapTOC) EnsureMsgDownloaded(realTOC,realSumNum,false);

					textSize = (*realTOC)->sums[realSumNum].length;
					disposeText = true;
					
					CacheMessage(realTOC,realSumNum);
					hText = nil;
					
					if ((*realTOC)->sums[realSumNum].cache)
					{
						HNoPurge((*realTOC)->sums[realSumNum].cache);
						{
							//	If rich or HTML, need text stripped of markup
							if (criteria.fudge8bit || (*sh)->noBulkSearch ||
									(*sh)->searchResults && (((*realTOC)->sums[realSumNum].flags & FLAG_RICH) ||
								((*realTOC)->sums[realSumNum].opts & OPT_HTML+OPT_FLOW+OPT_CHARSET)))
							{
								if (!PeteCreate(win,&tempPte,0,nil))
								{
									PeteCalcOff(tempPte);
									if (!InsertRich((*realTOC)->sums[realSumNum].cache,0,-1,-1,true,tempPte,nil,0!=((*realTOC)->sums[realSumNum].opts & OPT_DELSP)))
									{
										PeteGetRawText(tempPte,&hText);
									}
								}
							}
							else
							{
								hText = DupHandle((*realTOC)->sums[realSumNum].cache);
								disposeText = true;
							}
						}
						HPurge((*realTOC)->sums[realSumNum].cache);
					}
				}

				if (!hText)
					return false;

				pText = LDRef(hText);
				textSize = GetHandleSize(hText);
				offset=0;

				switch (criteria.category)
				{
					// search message
					case scAnywhere:
					  DoSearchMsg:
						found = SearchText(*criteria.searchString,pText,textSize,offset,criteria.relation);
						break;								
					case scHeaders:
						textSize = BodyOffset(hText);
						goto DoSearchMsg;
					case scBody:				
						offset = BodyOffset(hText);
						goto DoSearchMsg;
					
					// search in a header
					case scTo:
						hdrID = TO_HEAD+HEADER_STRN;
					  DoSearchHdr:
						found = SearchHeader(hdrID,hText,textSize,*criteria.searchString,criteria.relation);
						break;
					case scFrom:
						hdrID = FROM_HEAD+HEADER_STRN;
					 	goto DoSearchHdr;
					case scSubject:
						hdrID = SUBJ_HEAD+HEADER_STRN;
					 	goto DoSearchHdr;
					case scCC:
						hdrID = CC_HEAD+HEADER_STRN;
					 	goto DoSearchHdr;
					case scBCC:
						hdrID = BCC_HEAD+HEADER_STRN;
					 	goto DoSearchHdr;

					// search any recipient
					case scAnyRecipient:
						found = SearchHeader(TO_HEAD+HEADER_STRN,hText,textSize,*criteria.searchString,criteria.relation) ||
								SearchHeader(CC_HEAD+HEADER_STRN,hText,textSize,*criteria.searchString,criteria.relation) ||
								SearchHeader(BCC_HEAD+HEADER_STRN,hText,textSize,*criteria.searchString,criteria.relation);
						break;
					
					case scAttachNames:
						InitAttachmentFinder(&attData,hText,true,tocH,&(*tocH)->sums[mNum]);
						while (GetNextAttachment(&attData,&spec))
						{
							if (found=SearchString(*criteria.searchString,spec.name,criteria.relation))
								break;
						}
						break;
				}
				
				//	Dispose of text
				if (tempPte)
					PeteDispose(win,tempPte);
				else if (disposeText)
					ZapHandle(hText);
				else
					UL(hText);
				break;
		}
		
		if (found)
		{
			if ((*sh)->matchAny)
				return true;	//	Found one, that's good enough
		}
		else
		{
			if (!(*sh)->matchAny)
				return false;	//	This one not found, need to match them all
		}
	}
			
	return (*sh)->matchAny ? false : true;
}


/**********************************************************************
 * DateTimeDifference - return discrete difference between 2 times
 **********************************************************************/
long DateTimeDifference(DateTimeRec *date,DateTimeRec *currDate,long seconds,short units)
{
	switch(units)
	{
		case sauMinutes:	return seconds/60 + (currDate->minute<date->minute?1:0);
		case sauHours:	return DateTimeDifference(date,currDate,seconds,sauMinutes)/60 + ((seconds>=0&&currDate->minute<date->minute)?1:0);
		case sauDays:	return DateTimeDifference(date,currDate,seconds,sauHours)/24 + ((seconds>=0&&currDate->hour<date->hour)?1:0);
		case sauWeeks:	return DateTimeDifference(date,currDate,seconds,sauDays)/7 + ((seconds>=0&&currDate->dayOfWeek<date->dayOfWeek)?1:0);
		case sauMonths:	return DateTimeDifference(date,currDate,seconds,sauYears)*12 + (currDate->month - date->month);
		case sauYears:	return currDate->year - date->year;
	}
	return 0;	/* mtc sez - good as any! */
}

/**********************************************************************
 * ShortCompare - compare 2 short, return 0 if equal, -1 if value1 < value2, 1 if value1 2 value2
 **********************************************************************/
short ShortCompare(short value1,short value2)
{
	if (value1 == value2) return 0;
	if (value1 < value2) return -1;
	return 1;
}

/**********************************************************************
 * SearchHeader - search a header for the string
 **********************************************************************/
static Boolean SearchHeader(short header,Handle text,long size,StringPtr searchString,short relation)
{
	Str32	s;
	long	len = size;
	UPtr found;
	
	GetRString(s,header);
	if (found = FindHeaderString(*text,s,&len,False))
		return SearchText(searchString,found,len,0,relation);
	return false;
}

/**********************************************************************
 * MessPreFind - do a quick scan to see if a message might contain
 *  the desired string.  We cop out on anything weird and return True.
 *  The only way we return false is if we read up the message and it does
 *  not contain the desired string
 **********************************************************************/
static Boolean MessPreFind(SearchHandle sh,StringPtr s,short criteriaIdx)
{
	FSSpec	spec;
	Accumulator	a;
	
	if ((*sh)->noBulkSearch) return true;
	if (!(*sh)->bulkData.didBulkSearch)
	{
		spec = (*(*sh)->curr.tocH)->mailbox.spec;
		SearchBulk(&spec,sh);
		if ((*sh)->bulkSearching)
			return false;	//	bulk search was interrupted
	}
	GetHits(sh,criteriaIdx,&a);
	if (a.data)
		return(FindInHits(sh,&a));
	else
		return false;
}

/**********************************************************************
 * FindInHits - do a find in the hit database
 **********************************************************************/
static Boolean FindInHits(SearchHandle sh,AccuPtr a)
{
	long start = (*(*sh)->curr.tocH)->sums[(*sh)->curr.message].offset;
	long stop = start + (*(*sh)->curr.tocH)->sums[(*sh)->curr.message].length;
	long *hit, *hitEnd;
	
	hitEnd = (long *)(*a->data + a->offset);
	for (hit=(long *)*a->data;hit<hitEnd;hit++)
	{
		if (stop<*hit) return(False);	// we're past the message
		if (*hit<start) continue;	// we're not there yet
		return(True);	// start <= *hit <= stop; we found it!
	}
	return(False);	// never found
}

/**********************************************************************
 * SearchBulk - search a file, but do it fast
 *	string - the string to search for
 *	spec - the file to search in
 *	allOffsets - accumulator (may be nil) that will get all hits
 **********************************************************************/
static OSErr SearchBulk(FSSpecPtr spec,SearchHandle sh)
{
	BulkSearchInfo	bulkData;
	OSErr err = fnfErr;
	FSSpec localSpec;
	
	// are we just pretending?
	if ((*sh)->noBulkSearch)
	{
		bulkData.didBulkSearch = true;
		return noErr;
	}
	
	/*
	 * allocate stuff
	 */
	Zero(bulkData.buf[0]);
	Zero(bulkData.buf[1]);
	bulkData.buf[0].buffer = NewIOBHandle(1 K, kIOBufSize);
	bulkData.buf[1].buffer = NewIOBHandle(1 K, kIOBufSize);
	bulkData.buf[0].pb = NuPtr(sizeof(ParamBlockRec));
	bulkData.buf[1].pb = NuPtr(sizeof(ParamBlockRec));
	bulkData.buf[0].free = bulkData.buf[1].free = True;
	bulkData.fullOffset = -1;
			
	if (bulkData.buf[0].buffer && bulkData.buf[1].buffer)
	{
		// record buffer sizes
		bulkData.buf[0].bSize = GetHandleSize(bulkData.buf[0].buffer);
		bulkData.buf[1].bSize = GetHandleSize(bulkData.buf[1].buffer);

		// open the file
		// open with write access also so nothing else (especially mailbox
		// compaction) tries to open and change it while we are searching
		if (!(err=AFSpOpenDF(spec,&localSpec,fsRdWrPerm,&bulkData.refN)))
		{
			short	i;
			
			// record initial state
			bulkData.spot = 0;
			bulkData.bulkSearchCriteriaIdx = 0;
			GetEOF(bulkData.refN,&bulkData.len);
			bulkData.len -= bulkData.spot;
			bulkData.didBulkSearch = true;
			
			// build list of criteria to search
			bulkData.nBulkSearchCriteria = 0;
			for(i=0;i<(*sh)->criteriaCount;i++)
			{
				CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];
				
				if (gCategoryTable[criteria.category-1].flags & kNeedsMsg)
					bulkData.bulkSearchCriteria[bulkData.nBulkSearchCriteria++] = i;
			}
					
			(*sh)->bulkSearching = true;
			(*sh)->bulkData = bulkData;
			DoBulkSearch(sh);
		}
		else
		{
			//	File error
			ZapHandle(bulkData.buf[0].buffer);
			ZapHandle(bulkData.buf[1].buffer);
			DisposePtr((Ptr)bulkData.buf[0].pb);
			DisposePtr((Ptr)bulkData.buf[1].pb);
		}
	}
	return err;
}

/**********************************************************************
 * StopBulkSearch - stop bulk searching
 **********************************************************************/
static void StopBulkSearch(SearchHandle sh)
{
	if (!(*sh)->bulkSearching)
		return;
	
	FSClose((*sh)->bulkData.refN);
	
	/*
	 * dump the buffers
	 */
	ZapHandle((*sh)->bulkData.buf[0].buffer);
	ZapHandle((*sh)->bulkData.buf[1].buffer);
	DisposePtr((Ptr)(*sh)->bulkData.buf[0].pb);
	DisposePtr((Ptr)(*sh)->bulkData.buf[1].pb);
	(*sh)->bulkSearching = false;
}

/**********************************************************************
 * DoBulkSearch - do bulk searching
 **********************************************************************/
static void DoBulkSearch(SearchHandle sh)
{
	BulkSearchInfo	bulkData = (*sh)->bulkData;
	OSErr	err = noErr;
	Str255 strings[2];
	short curString;
	UPtr string;
	
	while ((!err && bulkData.len>0) || !bulkData.buf[0].free || !bulkData.buf[1].free)
	{
		short i;
 		
		if (EventPending())
		{
			//	Need to go away for a while to process this event
			(*sh)->bulkData = bulkData;
//			SetHits(sh,bulkData.bulkSearchCriteria[bulkData.bulkSearchCriteriaIdx],&hits);
			return;
		}
//				CycleBalls();
		CheckChaseArrows(sh);
		
		/*
		 * fill the buffers if need be
		 */
		for (i=0;i<sizeof(bulkData.buf)/sizeof(bulkData.buf[0]);i++)
			if (bulkData.len && !err && bulkData.buf[i].free && bulkData.len)
			{
				ParmBlkPtr pb = bulkData.buf[i].pb;
				Zero(*pb);
				bulkData.buf[i].offset = bulkData.spot;
				bulkData.buf[i].size = MIN(bulkData.len,bulkData.buf[i].bSize);
				bulkData.spot += bulkData.buf[i].size;
				bulkData.len -= bulkData.buf[i].size;
				
				if (SyncRW)
				{
					// boring synch stuff
					err = FSRead(bulkData.refN,&bulkData.buf[i].size,LDRef(bulkData.buf[i].buffer));
					UL(bulkData.buf[i].buffer);
					pb->ioParam.ioResult = err;
					if (!err) bulkData.buf[i].free = False;
				}
				else
				{
					// zippy asynch stuff
					pb->ioParam.ioRefNum = bulkData.refN;
					pb->ioParam.ioBuffer = LDRef(bulkData.buf[i].buffer);
					pb->ioParam.ioReqCount = bulkData.buf[i].size;
					pb->ioParam.ioResult = inProgress;
					pb->ioParam.ioPosMode = fsFromStart;
					pb->ioParam.ioPosOffset = bulkData.buf[i].offset;
					PBReadAsync(pb);
					bulkData.buf[i].pending = True;
					bulkData.buf[i].free = False;
				}
			}
		
		/*
		 * check to see if any i/o has completed
		 */
		for (i=0;i<sizeof(bulkData.buf)/sizeof(bulkData.buf[0]);i++)
			if (bulkData.buf[i].pending && bulkData.buf[i].pb->ioParam.ioResult!=inProgress)
			{
				if (!err) err = bulkData.buf[i].pb->ioParam.ioResult;
				bulkData.buf[i].pending = False;
				UL(bulkData.buf[i].buffer);
			}
		
		/*
		 * do we have a buffer to search?
		 */
		i = -1;
		if (READY(0))
		{
			// buffer 0 is ready.  How about buffer 1?
			if (READY(1))
				// both ready.  take lesser
				i = (bulkData.buf[0].offset < bulkData.buf[1].offset) ? 0 : 1;
			else
				i = 0;	// just buffer 0 is ready
		}
		else if (READY(1)) i = 1;	// just buffer 1 is ready
		
		/*
		 * search
		 */
		if (i>=0)
		{
			if (err) bulkData.buf[i].free = True;	// backing out
			else
			{
				long tempOffset;
				long bufLen = GetHandleSize(bulkData.buf[i].buffer);
				Boolean regexp;

				for(;bulkData.bulkSearchCriteriaIdx < bulkData.nBulkSearchCriteria;bulkData.bulkSearchCriteriaIdx++)
				{
					short	criteriaIdx = bulkData.bulkSearchCriteria[bulkData.bulkSearchCriteriaIdx];
					StringHandle	searchString;
					Boolean	match = false;
					Accumulator		hits;
	
					// set up for this criterion
					GetHits(sh,criteriaIdx,&hits);
					if (!hits.data)
					{
						err = AccuInit(&hits);
						SetHits(sh,criteriaIdx,&hits);
					}
					ASSERT(!err);
					if (err) break;
					searchString = (*(*sh)->hCriteria)[criteriaIdx].searchString;
					regexp = (*(*sh)->hCriteria)[criteriaIdx].relation==strRegExp;
					curString = 1;	// assume we'll skip the utf8 string
					if (!regexp)
					{
						if (searchString && **searchString)
						{
							PCopy(strings[1],*searchString);
							NormalizeSearchString(strings[1]);
							if ((*(*sh)->hCriteria)[criteriaIdx].utf8SearchString && **(*(*sh)->hCriteria)[criteriaIdx].utf8SearchString)
							{
								PCopy(strings[0],*(*(*sh)->hCriteria)[criteriaIdx].utf8SearchString);
								NormalizeSearchString(strings[0]);
								curString = 0;
							}
						}
						else
							continue;
					}
				
					// wrap this in a loop, so we can look for both normal & utf8 hits
					do
					{
						string = strings[curString];
						tempOffset = 0;
						while (0<=(tempOffset= regexp ? SearchRegExpHandle(*searchString,bulkData.buf[i].buffer,tempOffset) : SearchStrText(string,(*bulkData.buf[i].buffer),bufLen,tempOffset)))
						{
							Ptr	pData;
							
							// string found
							bulkData.fullOffset = tempOffset + bulkData.buf[i].offset;
							err = AccuAddSortedLong(&hits,bulkData.fullOffset);
							tempOffset++;
							match=true;
							if (err) break;
							
							//	for regular expressions, start at the beginning of the next line
							if (regexp)
							{
								for(pData=*bulkData.buf[i].buffer;tempOffset<bufLen;)
									if (pData[tempOffset++] == returnChar)
										break;
								if (tempOffset >= bufLen)
									break;	//	end of buffer
							}
						}
					}
					while (++curString<2);
					
					if (match)
						SetHits(sh,criteriaIdx,&hits);	// save updated hits
				}
				bulkData.bulkSearchCriteriaIdx = 0;
				bulkData.buf[i].free = True;
			}
		}
	}

	//	Done!
	StopBulkSearch(sh);
}

/**********************************************************************
 * GetBoxCountSpec - get file spec for mailbox from BoxCount
 **********************************************************************/
static Boolean GetBoxCountSpec(SearchHandle sh, short boxNum, FSSpecPtr spec)
{
	return GetBoxCountSpecLo((*sh)->BoxCount ? (*sh)->BoxCount : BoxCount,boxNum,spec);
}

/**********************************************************************
 * GetBoxCountSpec - get file spec for mailbox from BoxCount
 **********************************************************************/
static Boolean GetBoxCountSpecLo(BoxCountHandle boxList, short boxNum, FSSpecPtr spec)
{
	short menuId;
	
	if (boxNum>=GetHandleSize_(boxList)/sizeof(BoxCountElem)) return false;
	spec->parID = (*boxList)[boxNum].dirId;
	spec->vRefNum = (*boxList)[boxNum].vRef;
	menuId = (spec->parID==MailRoot.dirId && SameVRef(spec->vRefNum,MailRoot.vRef)) ? MAILBOX_MENU : FindDirLevel(spec->vRefNum,spec->parID);
	MailboxMenuFile(menuId,(*boxList)[boxNum].item,spec->name);
	if (IsIMAPCacheFolder(spec)) spec->parID = SpecDirId(spec);
	return true;
}

/**********************************************************************
 * GetBoxTOC - get TOC from BoxCount or my own BoxCount
 **********************************************************************/
static TOCHandle GetBoxTOC(SearchHandle sh,SearchMarker *which)
{
	FSSpec spec;
	
	which->tocH = nil;
	which->opened = false;
	
	if (GetBoxCountSpec(sh,which->box,&spec))
		which->tocH = GetSpecTOC(&spec,&which->opened);
	return which->tocH;
}

/**********************************************************************
 * GetSpecTOC - get TOC from file spec
 **********************************************************************/
static TOCHandle GetSpecTOC(FSSpecPtr spec,Boolean *opened)
{
	FSSpec newSpec;
	TOCHandle	tocH = nil;
	
	*opened = false;
	if (!(tocH=FindTOC(spec)))
	{
		// didn't find TOC, need to open it
		if (IsAlias(spec,&newSpec) && SameSpec(spec,&newSpec)) return(nil);
		if (tocH = CheckTOC(&newSpec))
			*opened = true;
	}
	return tocH;
}

/**********************************************************************
 * FinishedTOC - we're done with this TOC, may need to dispose of it
 **********************************************************************/
static void FinishedTOC(SearchMarker *which)
{
	if (which->tocH && which->opened)
	{
		if (TOCIsDirty(which->tocH))	WriteTOC(which->tocH);
		BoxFClose(which->tocH,true);
		ZapHandle(which->tocH);
		which->opened = false;
	}
	which->tocH = nil;
}

/**********************************************************************
 * CreateTextSpecifier - add a text specifier
 **********************************************************************/
static void CreateTextSpecifier(MyWindowPtr win,CriteriaInfo *pCriteria,Boolean number)
{
	PETEDocInitInfo pdi;
	PETEHandle		previewPTE;
	TOCHandle	tocH;
	DECLARE_UPP(SearchTextDraw,ControlUserPaneDraw);

	DefaultPII(win,false,peNoStyledPaste+peClearAllReturns,&pdi);
	SetRect(&pdi.inRect,-4000,-4000,-3900,-3900);
	if (!PeteCreate(win,&pCriteria->pte,peNoStyledPaste+peClearAllReturns,&pdi))
	{
		PeteFontAndSize(pCriteria->pte,SmallSysFontID(),SmallSysFontSize());
		(*PeteExtra(pCriteria->pte))->frame = True;
		if (pCriteria->ctlPete = CreateControl(win,win->topMarginCntl,0,kControlUserPaneProc,false))
		{
			INIT_UPP(SearchTextDraw,ControlUserPaneDraw);
			SetControlData(pCriteria->ctlPete,0,kControlUserPaneDrawProcTag,sizeof(SearchTextDrawUPP),(void*)&SearchTextDrawUPP);
			SetControlReference(pCriteria->ctlPete,(long)pCriteria->pte);
		}
		PeteFocus(win,pCriteria->pte,true);

		if (number)
			(*PeteExtra(pCriteria->pte))->numberField = true;	//	Number field
		
		// If there's a preview pane, it can mess up the text field focus order
		// Put any preview pane at the end of the list of text fields		
		GetSearchInfo(win,&tocH,nil);
		if (tocH && (previewPTE = (*tocH)->previewPTE))
		{
			win->pteList = PeteRemove(win->pteList,previewPTE);
			win->pteList = PeteLink(win->pteList,previewPTE);
		}
	}
	CleanPII(&pdi);
}

/**********************************************************************
 * RemoveFocus - remove current pte focus
 **********************************************************************/
static void RemoveFocus(MyWindowPtr win)
{
	//SD Why? PeteSelect(win,win->pte,0,0);
	PeteFocus(win,nil,true);
}

/**********************************************************************
 * AddCriteria - add criteria
 **********************************************************************/
static void AddCriteria(MyWindowPtr win,SearchHandle sh,Boolean defaultString)
{
	CriteriaInfo	criteria;
	Str255				what;
	short					count,
								i;
	
	RemoveFocus(win);
	Zero(criteria);
	
	// add default search criteria
	criteria.ctlCategory = CreateMenuControl(win,win->topMarginCntl,"\p",SRCH_CATEGORY_MENU,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,1,false);
	criteria.ctlRelation = CreateMenuControl(win,win->topMarginCntl,"\p",SRCH_TEXT_COMPARE_MENU,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,1,false);
	CreateTextSpecifier(win,&criteria,false);
	criteria.category = 1;
	
	if (!PtrAndHand(&criteria,(Handle)(*sh)->hCriteria,sizeof(CriteriaInfo)))
	{
		(*sh)->criteriaCount++;
		if (defaultString && GetFindString(what))
			PeteSetString(what,criteria.pte);
		PeteSelectAll(win,criteria.pte);
	}
	
	// (jp) 1-18-00  For light...
	count = CountMenuItems (GetPopupMenuHandle (criteria.ctlCategory));
	for (i = 1; i <= count; ++i)
		if (gCategoryTable[i - 1].proOnly)
			SetMenuItemBasedOnFeature (GetPopupMenuHandle(criteria.ctlCategory), i, featureSearch, true);
	count = CountMenuItems (GetPopupMenuHandle (criteria.ctlRelation));
	for (i = strContains + 1; i <= count; ++i)
		SetMenuItemBasedOnFeature (GetPopupMenuHandle(criteria.ctlRelation), i, featureSearch, true);

	EmbedSearchCriteriaControls(&criteria,sh);
}

/**********************************************************************
 * DisposeSpecifier - dispose of any specifier controls
 **********************************************************************/
static void DisposeSpecifier(MyWindowPtr win,SearchHandle sh,CriteriaInfo *pCriteria)
{
	if (pCriteria->ctlSpecifier)
	{
		if ((*sh)->ctlFocus==pCriteria->ctlSpecifier)
			(*sh)->ctlFocus = nil;
		DisposeControl(pCriteria->ctlSpecifier);
		pCriteria->ctlSpecifier = nil;
	}
	if (pCriteria->ctlPete) { DisposeControl(pCriteria->ctlPete); pCriteria->ctlPete = nil; }
	if (pCriteria->pte)
	{
		Rect	r;
		PeteRect(pCriteria->pte,&r);
		InsetRect(&r,-3,-3);
		InvalWindowRect(GetMyWindowWindowPtr(win),&r);	//	make sure entire pete (including focus ring) is updated
		PeteDispose(win,pCriteria->pte);
		pCriteria->pte = nil;
	}
	if (pCriteria->searchString)  { DisposeHandle((Handle)pCriteria->searchString); pCriteria->searchString = nil; }
}

/**********************************************************************
 * RemoveCriteria - remove criteria
 **********************************************************************/
static void RemoveCriteria(MyWindowPtr win,SearchHandle sh)
{
	CriteriaInfo	criteria=(*(*sh)->hCriteria)[--(*sh)->criteriaCount];
	
	DisposeControl(criteria.ctlCategory);
	DisposeControl(criteria.ctlRelation);
	DisposeSpecifier(win,sh,&criteria);
	AccuZap(criteria.hits);
	SetHandleSize((Handle)(*sh)->hCriteria,(*sh)->criteriaCount*sizeof(CriteriaInfo));
}


/**********************************************************************
 * SetSearchTopMargin - set top margin for Search window
 **********************************************************************/
static void SetSearchTopMargin(MyWindowPtr win,SearchHandle sh)
{
	short	topMargin = kWinHeaderHt+kCriteriaHt*(*sh)->criteriaCount;
	
	if ((*sh)->mailboxView)
		topMargin -= kMBHdrHt;
	SetTopMargin(win,topMargin);
}


/**********************************************************************
 * CheckChaseArrows - see if we should spin the chase arrows
 **********************************************************************/
static void CheckChaseArrows(SearchHandle sh)
{
	if (TickCount()-(*sh)->chaseArrowTicks > kChaseArrowTicks)
	{
		SendControlMessage((*sh)->ctlChaseArrows,kControlMsgIdle,nil);
		(*sh)->chaseArrowTicks = TickCount();
	}
}

/**********************************************************************
 * DisplayMBName - display name of mailbox we are searching
 **********************************************************************/
static void DisplayMBName(SearchHandle sh, PStr name)
{
	Str255	s;

	GetRString(s,SEARCH_SEARCHING);
	PCat(s,name);
	SetTextControlText((*sh)->ctlMailBox,s,nil);
}

/**********************************************************************
 * SearchTransfer - a message has been transferred, update Mailbox display
 **********************************************************************/
static short FindMBIdx(TOCHandle tocH,FSSpecPtr toSpec,Boolean allowCreate)
{
	FSSpecHandle specList = (*tocH)->mailbox.virtualMB.specList;
	FSSpecPtr	pSpec;
	short		i,cnt;
	Boolean		found=false;

	if (!specList) return -1;	// no mailboxes, not found

	//	do we already have this mailbox spec in the list?
	pSpec = LDRef(specList);
	cnt = (*tocH)->mailbox.virtualMB.specListCount;
	for(i=0;i<cnt;i++,pSpec++)
	{
		if (SameSpec(toSpec,pSpec))
		{
			found = true;
			break;
		}
	}
	UL(specList);

	if (!found)
	{
		if (allowCreate)
		{
			//	not found, add to list
			PtrAndHand(toSpec,(Handle)specList,sizeof(FSSpec));
			(*tocH)->mailbox.virtualMB.specListCount++;
			i = cnt;	
		}
		else
			i = -1;	//	not found
	}
	
	return i;
}

unsigned char SrchCharTabStrict[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, ' ', ' ', 0x0B, 0x0C, ' ', 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7F,

	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
unsigned char SrchCharTabFudge[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, ' ', ' ', 0x0B, 0x0C, ' ', 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7F,

	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

unsigned char *SrchCharTab = SrchCharTabFudge;

/**********************************************************************
 * NormalizeSearchString - convert to lower case, all white space to space
 **********************************************************************/
static void NormalizeSearchString(PStr s)
{
	short	i;
	unsigned char	*p;
	
	for(i=*s,p=s+1;i;i--,p++)
		*p=SrchCharTab[*p];
}


/**********************************************************************
 * SearchStrText - search for a string in text
 **********************************************************************/
long SearchStrText(PStr s,Ptr text,long length,long offset)
{
	UPtr		tp1,tp2,sp;
	long	tn,sn;
	long	sLen = *s;
	
	if (sLen<1) return -1;

	tp1=text+offset;
	for(tn=length-sLen-offset+1;tn>0;tn--)
	{
		if (s[1] != SrchCharTab[*tp1]) goto NoMatch;	//	Chances are the first char doesn't match
		
		//	search from 2nd char
		sp=s+2;
		tp2=tp1+1;
		for(sn=sLen-1;sn;sn--)
		{
			if (*sp != SrchCharTab[*tp2]) goto NoMatch;
			sp++;
			tp2++;
		}
		
		//	match
		return tp1-(UPtr)text;

  NoMatch:
		tp1++;
	}
	
	//	not found
	return -1;	
}

/**********************************************************************
 * FindHits - return pointer to hits accumulator
 **********************************************************************/
static AccuPtr FindHits(SearchHandle sh, short idx)
{
	CriteriaHandle	hCriteria;

	hCriteria = (*sh)->hCriteria;
	if (idx>=0 && idx<(*sh)->criteriaCount)
		return &(*hCriteria)[idx].hits;
	ASSERT(0);
	return nil;
}

/**********************************************************************
 * GetHits - get hits accumulator
 **********************************************************************/
static void GetHits(SearchHandle sh,short idx,AccuPtr a)
{
	*a = *FindHits(sh,idx);
}

/**********************************************************************
 * SetHits - set hits accumulator
 **********************************************************************/
static void SetHits(SearchHandle sh,short idx,AccuPtr a)
{
	AccuPtr	acc;
	
	if (a)
	{
		if (acc = FindHits(sh,idx))
			*acc = *a;
		ASSERT(acc);
	}
	else
	{
		//	Dispose
		Accumulator	hits;
		
		GetHits(sh,idx,&hits);
		if (hits.data)
		{
			AccuZap(hits);
			*FindHits(sh,idx) = hits;
		}
	}
}

/**********************************************************************
 * SetupIMAPSearchCriteria - set up criteria for IMAP server search
 **********************************************************************/
static IMAPSCHandle SetupIMAPSearchCriteria(SearchPtr sp)
{
	IMAPSCHandle	searchCriteria=NuHandle(0);
	short			i;
	
	//	Add criteria that require a message search
	for(i=0;i<sp->criteriaCount;i++)
	{
		CriteriaInfo	criteria=(*sp->hCriteria)[i];
	
		if (gCategoryTable[criteria.category-1].flags & kNeedsMsg)
		{
			IMAPSCStruct	searchInfo;
			
			PCopy(searchInfo.string,*criteria.searchString);
			searchInfo.headerCombination = kNone;
			searchInfo.headerName = 0;
			switch (criteria.category)
			{
				case scAnywhere:	searchInfo.headerCombination = kAnyWhere; break;
				case scHeaders:		searchInfo.headerCombination = kAllHeaders; break;
				case scBody:		searchInfo.headerCombination = kBody; break;		
				case scTo:			searchInfo.headerName = TO_HEAD+HEADER_STRN; break;
				case scFrom:		searchInfo.headerName = FROM_HEAD+HEADER_STRN; break;
				case scSubject:		searchInfo.headerName = SUBJ_HEAD+HEADER_STRN; break;
				case scCC:			searchInfo.headerName = CC_HEAD+HEADER_STRN; break;
				case scBCC:			searchInfo.headerName = BCC_HEAD+HEADER_STRN; break;
				case scAnyRecipient: searchInfo.headerCombination = kAnyRecipient; break;
				case scAttachNames:	searchInfo.headerCombination = kBody; break;
			}
			PtrAndHand((Ptr)&searchInfo,(Handle)searchCriteria,sizeof(searchInfo));
		}
	}
	
	return searchCriteria;
}

/**********************************************************************
 * ProcessIMAPSearch - process IMAP server search results
 **********************************************************************/
static void ProcessIMAPSearch(MyWindowPtr win,TOCHandle srchTOC,SearchHandle sh)
{
	Handle toAdd = nil, toUpdate = nil, toDelete = nil, toCopy = nil;
	MailboxNodeHandle 	mbox = nil;
	IMAPSResultHandle	hResults=nil;
	FSSpec				spec;
	Boolean				invalidateBoxSize = false;
	
	CheckChaseArrows(sh);

	//	Check for results
	if (IMAPDelivery(srchTOC,&toAdd,&toUpdate,&toDelete,&toCopy,nil,&hResults,&mbox,nil) && hResults)
	{
		short	count = GetHandleSize_(hResults)/sizeof(IMAPSResultStruct);
		IMAPSResultPtr	pResult;
					
		for(pResult=LDRef(hResults);count--;pResult++)
		{
			TOCHandle	tocH;
			Boolean		opened;
			
			if (GetBoxCountSpecLo((*sh)->imapBoxCount,pResult->box,&spec) && 
				(tocH = GetSpecTOC(&spec,&opened)))
			{
				MSumPtr	pSum;
				short	sumNum;
				DEC_STATE(tocH);
				
				L_STATE(tocH);
				if ((sumNum = FindSumByHash(tocH,pResult->uidHash)) >= 0)
					pSum = &(*tocH)->sums[sumNum];
				else
					pSum = nil;
				if (!pSum)
				{
					//	IMAP summary info hasn't been delivered yet. Do delivery and try again
					U_STATE(tocH);
					UpdateIMAPMailbox(tocH);
					L_STATE(tocH);
					if ((sumNum = FindSumByHash(tocH,pResult->uidHash)) >= 0)
						pSum = &(*tocH)->sums[sumNum];
				}
				if (pSum)
				{				
					//	See if we need to do any searching in the summary
					if (((*sh)->criteriaFlags & kNeedsSum) && !(*sh)->matchAny)
					{
						Boolean	stop;
						SearchMarker	curr;
						
						Zero(curr);
						curr.box = pResult->box;
						curr.message = sumNum;
						curr.tocH = tocH;
						if (!SearchMessage(win,pSum,sh,&curr,&stop,true))
							pSum=nil;	//	This one doesn't match
					}
				}
				
				if (pSum)
				{
					//	Add this to search results!
					short	specIdx = (*(*sh)->imapBoxSpecIdx)[pResult->box];
					
					if (AddSearchResult(sh,srchTOC,tocH,sumNum,specIdx==0,specIdx-1))
					{
						StopSearch(win);
						U_STATE(tocH);
						break;
					}
					
					if ((specIdx-1)<0)
						//	Added new spec
						(*(*sh)->imapBoxSpecIdx)[pResult->box] = (*srchTOC)->mailbox.virtualMB.specListCount;

					invalidateBoxSize = true;					
				}
				U_STATE(tocH);
			}
		}
	}
	
	if (mbox == SEARCH_WINDOW)
		//	we're done!
		StopSearch(win);

	ZapHandle(hResults);		
	if (invalidateBoxSize)
		UpdateBoxSize(win);
}

/**********************************************************************
 *	FindLinkSummary - find virtual summary that links to this one
 **********************************************************************/
static MSumPtr FindLinkSummary(TOCHandle srchTocH, TOCHandle fromTocH, long serialNum, short *realSum, short *pVirtualMBIdx)
{
	short	i,count;
	MSumPtr	sum;
	
	if (srchTocH && fromTocH)
	{
		FSSpec	spec = GetMailboxSpec(fromTocH,-1);
		short	virtualMBIdx = FindMBIdx(srchTocH,&spec,false);		
		if (virtualMBIdx >= 0)
		{
			count = (*srchTocH)->count;
			for(i=0,sum=(*srchTocH)->sums;i<count;i++,sum++)
				if (serialNum==sum->u.virtualMess.linkSerialNum && virtualMBIdx == sum->u.virtualMess.virtualMBIdx)
				{
					// found it!
					*realSum = i;
					*pVirtualMBIdx = virtualMBIdx;
					return sum;
				}
		}
	}
	return nil;	// not found!
}


/**********************************************************************
 * SearchUpdateSum - this summary has changed. See if we need to update
 *    summary in search window
 **********************************************************************/
void SearchUpdateSum(TOCHandle tocH, short sumNum, TOCHandle fromTocH, long serialNum, Boolean transfer, Boolean nuke)
{
	WindowPtr winWP;
	MyWindowPtr	win;
	MSumPtr		pSum;
	FSSpec		spec;
	
	if (!gSearchWinCount) 	//	Don't update from search window
		return;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			TOCHandle		srchTocH;
			short			srchSum;
			short			virtualMBIdx;
			Boolean			redraw = false;

			win = GetWindowMyWindowPtr (winWP);
			GetSearchInfo(win,&srchTocH,&sh);
			if (nuke && !tocH)
			{
				//	Just emptied the trash. Delete any messages that were in the trash.
				short	trashIdx;
				
				spec.vRefNum = MailRoot.vRef;
				spec.parID = MailRoot.dirId;
				GetRString(spec.name,TRASH);
				trashIdx = FindMBIdx(srchTocH,&spec,false);
				if (trashIdx >= 0)
				{
					short	sum;
					
					for(sum=(*srchTocH)->count-1;sum>=0;sum--)
					{
						if ((*srchTocH)->sums[sum].u.virtualMess.virtualMBIdx==trashIdx)
						{
							DeleteSum(srchTocH,sum);
							redraw = true;
						}
					}
				}
			}
			else if (pSum = FindLinkSummary(srchTocH, fromTocH, serialNum, &srchSum, &virtualMBIdx))
			{
				if (nuke)
				{
					DeleteSum(srchTocH,srchSum);
					redraw = true;
				}
				else
				{
					if (transfer)
					{
						//	Change MB idx and assign new serialNumber
						spec = GetMailboxSpec(tocH,sumNum);
						(*srchTocH)->sums[srchSum].u.virtualMess.virtualMBIdx = FindMBIdx(srchTocH,&spec,true);
						(*srchTocH)->sums[srchSum].u.virtualMess.linkSerialNum = (*tocH)->sums[sumNum].serialNum;
					}
					else
						//	Update summary
						CopySum(&(*tocH)->sums[sumNum],pSum,pSum->u.virtualMess.virtualMBIdx);
					InvalSum(srchTocH,srchSum);
				}
			}
			else if (!nuke && tocH && !PrefIsSet(PREF_NO_LIVE_SEARCHES))
			{
				if (!SearchBoxesInclude(sh,fromTocH) && SearchBoxesInclude(sh,tocH))
					SearchIncremental(win,tocH,sumNum);
			}
			if (redraw)
			{
				InvalContent(win);
				UpdateBoxSize(win);
			}
		}
}

/**********************************************************************
 * SearchBoxesInclude - does the search domain include this tocH?
 **********************************************************************/
Boolean SearchBoxesInclude(SearchHandle sh,TOCHandle tocH)
{
	short count = HandleCount((*sh)->BoxCount);
	FSSpec searchSpec;
	
	while (count--)
	{
		GetBoxCountSpec(sh,count,&searchSpec);
		if (SameSpec(&searchSpec,&(*tocH)->mailbox.spec)) return true;
	}
	return false;
}

/**********************************************************************
 * SearchIncremental - add a message to this search, if it matches
 **********************************************************************/
Boolean SearchIncremental(MyWindowPtr win,TOCHandle tocH,short sumNum)
{
	TOCHandle srchTocH;
	SearchHandle sh;
	SearchMarker marker;
	Boolean oldNoBulk;
	Boolean found;
	short specIdx;
	
	marker.tocH = tocH;
	marker.message = sumNum;
	
	GetSearchInfo(win,&srchTocH,&sh);
	
	ASSERT((*sh)->setup);	// avoid searches that haven't been started yet.
	if ((*sh)->setup)
	{
		oldNoBulk = (*sh)->noBulkSearch;
		(*sh)->noBulkSearch = true;
		found = SearchMessage(win,&(*tocH)->sums[sumNum],sh,&marker,nil,false);
		(*sh)->noBulkSearch = oldNoBulk;
	
		if (found)
		{
			if (!(*srchTocH)->mailbox.virtualMB.specList) (*srchTocH)->mailbox.virtualMB.specList = NuHandle(0);
			// this routine will add the spec to the specList if necessary
			VERIFY(0<=(specIdx=FindMBIdx(srchTocH,&(*tocH)->mailbox.spec,true)));
			// and now we can add it!
			AddSearchResult(sh,srchTocH,tocH,sumNum,false,specIdx);
			return true;
		}
	}
	
	return false;
}

/**********************************************************************
 * IMAPSearchIncremental - an IMAP mailbox has changed.  Update all
 *	search windows that are currently searching this mailbox.
 **********************************************************************/
void IMAPSearchIncremental(MailboxNodeHandle mbox)
{
	WindowPtr winWP;
	MyWindowPtr win;
	SearchHandle sh;
	TOCHandle toc;
	short i;
	FSSpec spec;
	long lastSearchedUID = 0;
	long firstUid = (*mbox)->searchUID + 1;	// where to start the incremental search from
	IMAPSCHandle searchCriteria;
	
	for (winWP = GetWindowList(); winWP; winWP = GetNextWindow (winWP)) 
	{
		win = GetWindowMyWindowPtr (winWP);
		if (IsSearchWindow(winWP))
		{
			GetSearchInfo(win,&toc,&sh);
			
			if ((*sh)->setup)
			{
				// make sure we're up to date on the selected mailboxes
				if ((*sh)->imapBoxCount == NULL)
					GetSelectedBoxes(*sh, toc,false);
				if ((*sh)->imapBoxCount)
				{
					// is this mailbox amongst the mailboxes to search in this search window?
					for (i = 0; GetBoxCountSpecLo((*sh)->imapBoxCount, i, &spec); i++)
					{
						if (SameSpec(&spec, &(*mbox)->mailboxSpec))
						{
							// re-run the search on this particular mailbox	
							if (searchCriteria = SetupIMAPSearchCriteria(*sh))
							{
								// IMAPSearch will return false if no searches were actually started.
								if (IMAPSearchMailbox(toc,(*sh)->imapBoxCount,mbox,searchCriteria,!(*sh)->matchAny,firstUid))
									(*sh)->searchMode |= kSearchingIMAP;	
							}
						}
					}
				}
			}
		}
	}
}

/**********************************************************************
 * SearchInvalTocBox - this summary needs to be updated. See if any
 *  summaries in search windows need to also be updated
 **********************************************************************/
void SearchInvalTocBox(TOCHandle tocH, short sumNum, short box)
{
	WindowPtr winWP;
	
	if (!gSearchWinCount) 	//	No search windows
		return;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			TOCHandle		srchTocH;
			short			srchSum,virtualMBIdx;

			GetSearchInfo(GetWindowMyWindowPtr(winWP),&srchTocH,&sh);
			if (FindLinkSummary(srchTocH,tocH,(*tocH)->sums[sumNum].serialNum,&srchSum,&virtualMBIdx))
				InvalTocBoxLo(srchTocH,srchSum,box);
		}
}

/**********************************************************************
 * TellSearchMBRename - notify search windows that mailbox has been renamed
 **********************************************************************/
void TellSearchMBRename(FSSpecPtr spec,FSSpecPtr newSpec)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	
	if (!gSearchWinCount)
		return;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			TOCHandle		tocH;
			short			index;

			win = GetWindowMyWindowPtr (winWP);
			GetSearchInfo(win,&tocH,&sh);
			index = FindMBIdx(tocH,spec,false);
			if (index >= 0)
			{
				//	Found it. Replace.				
				(*(*tocH)->mailbox.virtualMB.specList)[index] = *newSpec;
				InvalContent(win);	//	Update window
			}			
		}
}

/**********************************************************************
 * GetTOCFromSearchWin - see if any search windows have opened a TOC not in TOC list
 *   Called from OpenMailbox. TOC window will be opened and TOC added to TOC list
 **********************************************************************/
TOCHandle GetTOCFromSearchWin(FSSpecPtr spec)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	
	if (!gSearchWinCount)
		return nil;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			TOCHandle		tocH;
			FSSpec			thisSpec;
			
			win = GetWindowMyWindowPtr (winWP);
			GetSearchInfo(win,nil,&sh);
			if ((tocH=(*sh)->curr.tocH) && (*sh)->curr.opened)
			{
				ASSERT(*tocH!=(TOCPtr)-1);
				if (*tocH==(TOCPtr)-1)
				{
					// ACK!  Fix this!
					(*sh)->curr.tocH = nil;
					(*sh)->curr.opened = false;
					continue;
				}
				thisSpec = GetMailboxSpec(tocH,-1);
				if (SameSpec(&thisSpec,spec))
				{
					(*sh)->curr.opened = false;	// TOC window will be opened and TOC added to TOC list
					return tocH;
				}
			}
		}
		
	return nil;
}


/**********************************************************************
 * SelectMailbox - select a mailbox in mailbox list
 **********************************************************************/
static void SelectMailbox(FSSpecPtr spec,ViewListPtr pView,Boolean folder,Boolean addToSelection)
{
	Str63	name;
	short	menuID;
	Str255	s;
	Boolean		isIMAP;
	
	PCopy(name,spec->name);

	if (isIMAP = IsIMAPMailboxFile(spec))
		ParentSpec(spec,spec);

	menuID = VD2MenuId(spec->vRefNum,spec->parID);
	if (folder)
	{	
		if (menuID == MAILBOX_MENU)
		{
			menuID = kEudoraFolder;
			GetRString(name,FILE_ALIAS_EUDORA_FOLDER);
		}
		else
		{
			//	get mail folder
			short	item;
			MenuHandle	mhPar = ParentMailboxMenu(GetMenuHandle(menuID),&item);
			menuID = GetMenuID(mhPar);
			GetMenuItemText(mhPar,item,name);

			if (menuID == MAILBOX_MENU && isIMAP)
				menuID = kIMAPFolder;
		}
	}
	
	//	open any folders above
	OpenMBFolder(pView,menuID,s);
	
	//	now select our item
	LVSelect(pView,menuID,name,addToSelection);
}

/**********************************************************************
 * SearchSave - save contents of Search window
 **********************************************************************/
static void SearchSave(MyWindowPtr win,Boolean saveAs)
{
	SearchHandle	sh;
	short 			refN,i;
	FSSpec			spec;
	TOCHandle		tocH;
	short			saveResFile = CurResFile();
	Str255			s;
	OSErr			err;
	SearchPtr		sp;

	GetSearchInfo(win,&tocH,&sh);

	sp = LDRef(sh);
	SetupSearchCriteria(win,sp);
	UL(sh);
	
	spec= (*sh)->saveSpec;
	if (saveAs || !spec.vRefNum)
	{
		//	User selects file
		long newDirId;
		FSSpec	folderSpec;

		DirCreate(Root.vRef,Root.dirId,GetRString(s,SEARCH_FOLDER),&newDirId);
		SubFolderSpec(SEARCH_FOLDER,&folderSpec);
		spec = folderSpec;
		*s = 0;
		AddCriteriaText(sh,s);	//	Get text description of criteria
		if (*s > 31) *s = 31;	//	Truncate to 31 chars
		PCopy(spec.name,s);
		err = SFPutOpen(&spec,CREATOR,SEARCH_FILE_TYPE,&refN,nil,nil,0,&folderSpec,nil,nil);
		if (err)
			return;
		MyFSClose(refN);
		(*sh)->saveSpec = spec;
		SetWTitle(GetMyWindowWindowPtr(win),spec.name);
	}

	FSpKillRFork(&spec);
	FSpCreateResFile(&spec,CREATOR,SEARCH_FILE_TYPE,smSystemScript);
	if (-1!=(refN=FSpOpenResFile(&spec,fsRdWrPerm)))
	{
		Handle	hSave,hSpecList;
		SearchSaveInfo	saveInfo;
		ViewListPtr	pView;
		
		UseResFile(refN);
		
		Zero(saveInfo);
		saveInfo.criteriaCount = (*sh)->criteriaCount;
		if (HasFeature (featureSearch))
			saveInfo.matchMode = GetControlValue((*sh)->ctlMatch);
		saveInfo.tabMode = GetControlValue((*sh)->ctlTabs);
		
		if (!PtrToHand(&saveInfo,&hSave,sizeof(saveInfo)))
		{
			for(i=0;i<saveInfo.criteriaCount;i++)
			{
				CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];
				CriteriaSave	saveCriteria;
				
				saveCriteria.category = MenuCategory2SavedCategory[criteria.category];
				saveCriteria.relation = GetControlValue(criteria.ctlRelation);
				saveCriteria.specifier = criteria.ctlSpecifier ? GetControlValue(criteria.ctlSpecifier) : 0;
				saveCriteria.misc = criteria.misc;
				saveCriteria.date = criteria.date;
				*s = 0;
				if (criteria.pte)				
					PeteString(s,criteria.pte);
				// No personalities in Light
				else if (HasFeature (featureMultiplePersonalities) && criteria.category==scPersonality)
				{
					if (saveCriteria.specifier > 1)
						//	Save personality by name in case menu changes
						GetMenuItemText(GetPopupMenuHandle(criteria.ctlSpecifier),saveCriteria.specifier,s);
				}
				if (*s)
				{
					//	Save string in separate resource
					StringHandle	hString = NewString(s);
					
					AddResource(hString,'STR ',1000+i,"");
					saveCriteria.stringID = 1000+i;
				}
				
				PtrAndHand(&saveCriteria,hSave,sizeof(saveCriteria));
			}
			
			AddResource(hSave,SEARCH_FILE_TYPE,1000,"");		
		}
		
		//	Save aliases to each selected mailbox if not all selected
		if ((pView = (*sh)->list) && (LVCountSelection(pView) != (*pView->hList)->dataBounds.bottom))
		{
			short	i;
			VLNodeInfo	data;
			
			for(i=1;i<=LVCountSelection(pView);i++)
			{
				AliasHandle	alias;
				
				LVGetItem(pView,i,&data,true);
				MakeMBSpec(&data,&spec);
				NewAliasMinimal(&spec,&alias);
				if (alias)
					AddResource((Handle)alias,rAliasType,1000+i,"");
			}
		}

		//	Save TOC handle
		HandToHand((Handle *) &tocH);
		AddResource((Handle)tocH,TOC_TYPE,1000,"");
		if (hSpecList = (Handle)(*tocH)->mailbox.virtualMB.specList)
		{
			HandToHand(&hSpecList);
			AddResource(hSpecList,kSpecListType,1000,"");
		}

		//	Save window position
		SearchPosition(true,win);
		
		// Save sort criteria
		Sort2String(s,tocH);
		{
			StringHandle hString = NewString(s);
			AddResource(hString,'STR ',SEARCH_SAVED_SORT_ID,"");
		}

		CloseResFile(refN);
	
		win->isDirty = false;

		BuildSearchMenu();
	}
	UseResFile(saveResFile);
}

/**********************************************************************
 * OpenSearchMenu - open a search file
 **********************************************************************/
void OpenSearchMenu(short item)
{
	FSSpec		spec;
	MenuHandle mh = GetMHandle(FIND_HIER_MENU);

	SubFolderSpec(SEARCH_FOLDER,&spec);
	GetMenuItemText(mh,item,spec.name);
	OpenSearchFileAndStart(&spec);
}

/**********************************************************************
 * OpenSearchFileAndStart - open a search file, and start the search
 **********************************************************************/
MyWindowPtr OpenSearchFileAndStart(FSSpecPtr spec)
{
	MyWindowPtr win = OpenSearchFile(spec);
	if (win)
	{
		TOCHandle	tocH;

		GetSearchInfo(win,&tocH,nil);
#ifdef WE_EVER_GET_CLEVER
		if (!(*tocH)->count)	
#else
		// setup for incremental searching.  Until we get clever, that is.
		if (!(*tocH)->count || !PrefIsSet(PREF_NO_LIVE_SEARCHES)) 
#endif
			//	If no results, start search
			StartSearch(win);
	}
	return win;
}

/**********************************************************************
 * OpenSearchFile - open a search file
 **********************************************************************/
MyWindowPtr OpenSearchFile(FSSpecPtr spec)
{
	WindowPtr	winWP;
	MyWindowPtr win;
	SearchHandle	sh;
	TOCHandle	tocH;
	Str255	s;
	Rect	r;
	short	saveResFile = CurResFile();
	short	tabMode = 0;
	FSSpec	aliasSpec;

	//	Is search window already open?
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			win = GetWindowMyWindowPtr (winWP);
			GetSearchInfo(win,nil,&sh);
			if (SameSpec(spec,&(*sh)->saveSpec))
			{
				//	Already open
				UserSelectWindow(winWP);
				return win;
			}
		}

	if (win = SearchOpen(0))
	{
		Handle	hTOCRes;
		short	refN;
		
		winWP = GetMyWindowWindowPtr (win);
		GetSearchInfo(win,&tocH,&sh);
		if (-1!=(refN=FSpOpenResFile(spec,fsRdPerm)))
		{
			Handle	hSave;
			short	cntAlias;
			ViewListPtr	pView;
						
			UseResFile(refN);	
			if (hSave = GetResource(SEARCH_FILE_TYPE,1000))
			{
				SearchSaveInfo	*pSaveInfo = LDRef(hSave);
				CriteriaHandle	hCriteria = (*sh)->hCriteria;
				CriteriaSave	*pSaveCriteria;
				short					count,
											item,
											i;
				
				while ((*sh)->criteriaCount)
					RemoveCriteria(win,sh);

				(*sh)->criteriaCount = pSaveInfo->criteriaCount;
				if (HasFeature (featureSearch))
					SetControlValue((*sh)->ctlMatch,pSaveInfo->matchMode);
				tabMode = pSaveInfo->tabMode;
				for(i=0,pSaveCriteria=pSaveInfo->criteria;i<pSaveInfo->criteriaCount;i++,pSaveCriteria++)
				{
					CriteriaInfo	criteria;
					short			specifier;
					LongDateRec	date;
					
					Zero(criteria);
					criteria.category = SafeSavedCategory2MenuCategory(pSaveCriteria->category);
					criteria.relation = pSaveCriteria->relation;
					criteria.specifier = pSaveCriteria->specifier;
					criteria.misc = pSaveCriteria->misc;
					criteria.date = pSaveCriteria->date;
					
					criteria.ctlCategory = CreateMenuControl(win,win->topMarginCntl,"\p",SRCH_CATEGORY_MENU,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,criteria.category,false);
					criteria.ctlRelation = CreateMenuControl(win,win->topMarginCntl,"\p",gCategoryTable[criteria.category-1].relation,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,criteria.relation,false);

					// (jp) 1-18-00  For light...
					count = CountMenuItems (GetPopupMenuHandle (criteria.ctlCategory));
					for (item = 1; item <= count; ++item)
						if (gCategoryTable[item - 1].proOnly)
							SetMenuItemBasedOnFeature (GetPopupMenuHandle(criteria.ctlCategory), item, featureSearch, true);
					count = CountMenuItems (GetPopupMenuHandle (criteria.ctlRelation));
					for (item = strContains + 1; item <= count; ++item)
						SetMenuItemBasedOnFeature (GetPopupMenuHandle(criteria.ctlRelation), item, featureSearch, true);

					*s = 0;
					if (pSaveCriteria->stringID)
						GetRStr(s,pSaveCriteria->stringID);
					specifier = gCategoryTable[criteria.category-1].specifier;
					switch (specifier)
					{
						case ssText:
						case ssAge:
						case ssNumber:
							CreateTextSpecifier(win,&criteria,specifier!=ssText);
							PeteSetString(s,criteria.pte);
							if (specifier==ssAge)
								criteria.ctlSpecifier = CreateMenuControl(win,win->topMarginCntl,"\p",SRCH_AGE_UNITS_MENU,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,criteria.specifier,false);
							break;
						
						case ssDate:
							SetRect(&r,-4000,-4000,kDateWi-4000,kDateHt-4000);
							criteria.ctlSpecifier = NewControl(winWP,&r,"\p",true,0,0,0,kControlClockDateProc,0);
							EmbedControl(criteria.ctlSpecifier,win->topMarginCntl);
							Zero(date);
							date.ld.day = pSaveCriteria->date.day;
							date.ld.month = pSaveCriteria->date.month;
							date.ld.year = pSaveCriteria->date.year;
							SetControlData(criteria.ctlSpecifier,0,kControlClockLongDateTag,sizeof(date),(Ptr)&date);
							break;

						case PERS_HIER_MENU:
							// No personalities in Light
							if (HasFeature (featureMultiplePersonalities) && *s)
								criteria.specifier = FindItemByName(GetMenuHandle(PERS_HIER_MENU),s);
							//	Fall thru
							
						default:
							criteria.ctlSpecifier = CreateMenuControl(win,win->topMarginCntl,"\p",specifier,kControlPopupFixedWidthVariant+kControlPopupUseWFontVariant,criteria.specifier,false);
							break;
					}
					PtrAndHand(&criteria,hCriteria,sizeof(criteria));
					EmbedSearchCriteriaControls(&criteria,sh);
				}
			}

			//	Setup selected mailboxes from alias resource
			if (pView = (*sh)->list)
			{
				if (cntAlias = Count1Resources(rAliasType))
				{
					short	i;
					
					for(i=1;i<=cntAlias;i++)
					{
						AliasHandle	alias;
						
						if (alias = (AliasHandle)GetIndResource(rAliasType,i))
						{
							if (!SimpleResolveAliasNoUI(alias,&aliasSpec))
								SelectMailbox(&aliasSpec,pView,false,i>1);
							ReleaseResource((Handle)alias);
						}
					}
				}
				else
				{
					//	Select all
					Cell	c;
					
					c.h = 0;
					for(c.v=0;c.v<(*pView->hList)->dataBounds.bottom;c.v++)					
						LSetSelect(true, c, pView->hList);
					LVUpdateSelection(pView);
				}
			}
						
			//	Get TOC
			if (hTOCRes =  GetResource(TOC_TYPE,1000))
			{
				FSSpecHandle	specList,specListRes;
				TOCType		saveTOC = **tocH;
				short		sumCount;
				short		previewHi;
				
				SetHandleSize((Handle)tocH,0);
				HandAndHand(hTOCRes,(Handle)tocH);
				sumCount = (*tocH)->count;
				previewHi = (*tocH)->previewHi;
				BMD(&saveTOC,*tocH,sizeof(TOCType)-sizeof(MSumType));	//	Only summaries and TOC summary count should change
				(*tocH)->count = sumCount;
				(*tocH)->previewHi = previewHi;
				CleanseTOC(tocH); (*tocH)->virtualTOC = true;	// CleanseTOC steps on that.  :-(

				specList = (*tocH)->mailbox.virtualMB.specList;
				if (specListRes = (FSSpecHandle)GetResource(kSpecListType,1000))
				{
					if (!specList)
					{
						specList = NuHandle(0);
						(*tocH)->mailbox.virtualMB.specList = specList;
					}
					else
						SetHandleSize(specList,0);
					HandAndHand(specListRes,specList);					
					(*tocH)->mailbox.virtualMB.specListCount = HandleCount(specList);
				}
				else
				{
					(*tocH)->mailbox.virtualMB.specListCount = 0;
					if (specList)
						SetHandleSize(specList,0);
				}
			}

			(*sh)->saveSpec = *spec;
			SetWTitle(winWP,spec->name);
			
			// restore sort
			{
				Handle h = Get1Resource('STR ',SEARCH_SAVED_SORT_ID);
				if (h)
				{
					PCopy(s,*h);
					ApplySortString(tocH,s);
					ReleaseResource(h);
				}
			}
		}		

		AdjustForCriteriaCount(sh);
		if (tabMode && tabMode != GetControlValue((*sh)->ctlTabs))
		{
			SetControlValue((*sh)->ctlTabs,tabMode);
			SearchButton(win,(*sh)->ctlTabs,0,tabMode);
		}
		
		ShowMyWindow(winWP);
		UserSelectWindow(winWP);		

#ifdef WE_EVER_GET_CLEVER
		// setup for incremental searching
		if (!PrefIsSet(PREF_NO_LIVE_SEARCHES)) 
		{
			// This would work fine except that it won't pickup messages added while window was NOT open
			// For now, that's not acceptable, so we'll assume the caller will start the search
			GetSelectedBoxes(*sh,tocH,false);
			SetupSearchCriteria(win,*sh);
		}
#endif
				

		if (refN!=-1)
			CloseResFile(refN);
	}
	UseResFile(saveResFile);

	return win;
}

/**********************************************************************
 * BuildSearchMenu - add saved search windows to menu
 **********************************************************************/
void BuildSearchMenu(void)
{
	MenuHandle mh = GetMHandle(FIND_HIER_MENU);
	Boolean	haveDivider = false;
	FSSpec		folderSpec;
	Str255		s;
	CInfoPBRec	hfi;
	short		item;
	
	//	Remove any existing items
	for(item=CountMenuItems(mh);item>=FIND_MENU_LIMIT;item--)
		DeleteMenuItem(mh,item);

	if (!SubFolderSpec(SEARCH_FOLDER,&folderSpec))
	{
		hfi.hFileInfo.ioNamePtr = s;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate(folderSpec.vRefNum,folderSpec.parID,&hfi))
			if (!(hfi.hFileInfo.ioFlAttrib&0x10) && (hfi.hFileInfo.ioFlFndrInfo.fdType==SEARCH_FILE_TYPE))
			{
				if (!haveDivider)
				{
					AppendMenu(mh,"\p-");
					haveDivider = true;
				}
				MyAppendMenu(mh,s);
			}
	}
}

/**********************************************************************
 * GetMenuText - return text of selected item in menu control
 **********************************************************************/
static void GetMenuText(ControlHandle cntl,StringPtr s)
{
	GetMenuItemText(GetPopupMenuHandle(cntl),GetControlValue(cntl),s);
}

/**********************************************************************
 * GetCriteriaString - return a string representation of a criterion
 **********************************************************************/
static void GetCriteriaString(StringPtr s,CriteriaPtr criteria)
{
	Str255	sTemp;
	
	GetMenuText(criteria->ctlCategory,s);
	GetMenuText(criteria->ctlRelation,sTemp);
	PCatC(s,' ');
	PCat(s,sTemp);
	if (criteria->pte)
	{				
		PeteString(sTemp,criteria->pte);
		if (*sTemp + *s >= 250) return;	//	too big!
		PCatC(s,' ');
		PCatC(s,'"');
		PCat(s,sTemp);
		PCatC(s,'"');
	}
	if (criteria->ctlSpecifier)
	{
		long junk;
		LongDateRec		date;
		LongDateTime	time;

		if (criteria->category==scDate)
		{
			Zero(date);
			GetControlData(criteria->ctlSpecifier,0,
				kControlClockLongDateTag,sizeof(date),(Ptr)&date,&junk);
			LongDateToSeconds(&date,&time);
			LongDateString(&time,shortDate,sTemp,nil);
		}
		else
			GetMenuText(criteria->ctlSpecifier,sTemp);
		PCatC(s,' ');
		PCat(s,sTemp);
	}
	
}

/**********************************************************************
 * AddCriteriaText - append a textual representation of criteria
 **********************************************************************/
static void AddCriteriaText(SearchHandle sh, StringPtr sText)
{
	short	i;
	Str255	s;
	
	for(i=0;i<(*sh)->criteriaCount;i++)
	{
		CriteriaInfo	criteria=(*(*sh)->hCriteria)[i];

		GetCriteriaString(s,&criteria);
		if (*sText + *s > 250)
			break;	//	it's getting too long
		if (i)
			PCat(sText,"\p, ");
		PCat(sText,s);
	}
}

/**********************************************************************
 * SearchSetWTitle - set the title of the search message
 **********************************************************************/
static void SearchSetWTitle(MyWindowPtr win,SearchHandle sh)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	Str255	sTitle,s;
	
	if (!(*sh)->saveSpec.vRefNum)	//	don't change title of saved windows
	{
		
		GetRString(sTitle,SEARCH_SEARCH);
		PCat(sTitle,"\p: ");
		AddCriteriaText(sh,sTitle);
		GetWTitle(winWP,s);
		if (!StringSame(s,sTitle))	//	don't change title if hasn't changed
			SetWTitle_(winWP,sTitle);
	}
}

/************************************************************************
 * SearchRestorePosition - save/restore window position
 ************************************************************************/
static Boolean SearchPosition(Boolean save,MyWindowPtr win)
{
	Str255 title;	
	SearchHandle	sh;

	GetSearchInfo(win,nil,&sh);
	if ((*sh)->saveSpec.vRefNum)
		//	If window has been saved, save by window title
		return PositionPrefsTitle(save,win);
	
	//	Save/restore based on title "Search"
	GetRString(title,SEARCH_SEARCH);
	return PositionPrefsByName(save,win,title);	
}

/************************************************************************
 * SearchMBUpdate - update mailbox list
 ************************************************************************/
void SearchMBUpdate(void)
{
	WindowPtr		winWP;
	
	if (!gSearchWinCount)
		return;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			
			GetSearchInfo(GetWindowMyWindowPtr (winWP),nil,&sh);
			InvalidListView((*sh)->list);
		}
}

/************************************************************************
 * SearchFixUnread - update mailbox list unread status
 ************************************************************************/
void SearchFixUnread(MenuHandle mh,short item,Boolean unread)
{
	WindowPtr winWP;
	
	if (!gSearchWinCount)
		return;
	
	//	Find search window(s)
	for (winWP=FrontWindow_();winWP;winWP=GetNextWindow(winWP))
		if (IsSearchWindow(winWP))
		{
			SearchHandle	sh;
			
			GetSearchInfo(GetWindowMyWindowPtr (winWP),nil,&sh);
				MBFixUnreadLo((*sh)->list,mh,item,unread,(*sh)->mailboxView);
		}
}

/************************************************************************
 * ShowHideTabs - need to clip tab control on Mac OS X since control itself doesn't
 ************************************************************************/
static void ShowHideTabs(ControlRef ctlTabs,Boolean show)
{
	Rect	rCtl;
	
	ClipRect(GetControlBounds(ctlTabs,&rCtl));
	if (show) ShowControl(ctlTabs);
	else HideControl(ctlTabs);
	InfiniteClip(GetWindowPort(GetControlOwner(ctlTabs)));
}

/************************************************************************
 * EmbedSearchCriteriaControls - make sure search criteria controls are
 *	imbedded within group control, otherwise they don't respond under OS X
 ************************************************************************/
static void EmbedSearchCriteriaControls(CriteriaInfo *criteria,SearchHandle sh)
{
	ControlHandle	ctlCriteriaGroup = (*sh)->ctlCriteriaGroup;
	EmbedControl(criteria->ctlCategory,ctlCriteriaGroup);
	EmbedControl(criteria->ctlRelation,ctlCriteriaGroup);
	if (criteria->ctlSpecifier)
		EmbedControl(criteria->ctlSpecifier,ctlCriteriaGroup);
}

/************************************************************************
 * SearchViewIsMailbox - are we looking at the mailboxes view of the search
 *  window?
 ************************************************************************/
Boolean SearchViewIsMailbox(TOCHandle tocH)
{
	SearchHandle sh = nil;
	
	if (!(*tocH)->virtualTOC) return false;
	if (!IsSearchWindow(GetMyWindowWindowPtr((*tocH)->win))) return false;
	
	GetSearchInfo((*tocH)->win,nil,&sh);
	return sh && (*sh)->mailboxView;
}

#endif
