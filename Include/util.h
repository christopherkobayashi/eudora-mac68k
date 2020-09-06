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

#ifndef UTIL_H
#define UTIL_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * some linked-list macros
 **********************************************************************/
#define LL_Remove(head,item,cast) 																			\
	do {																																	\
	if (head==item) head = (*head)->next; 																\
	else																																	\
		for (M_T1=(uLong)head; M_T1; M_T1=(uLong)(*cast M_T1)->next)				\
		{ 																																	\
			if ((*cast M_T1)->next == item) 																	\
			{ 																																\
				(*cast M_T1)->next = (*item)->next; 														\
				break;																													\
			} 																																\
		} 																																	\
	} while (0)

#define LL_Push(head,item)																							\
	M_T1 = (uLong) ((*(item))->next = head, head = item)
#define LL_Queue(head,item,cast)																				\
	do {																																	\
		void *t = head;																											\
		if (head)																														\
		{																																		\
			while ((* cast t)->next) t = (* cast t)->next;										\
			(* cast t)->next = item;																					\
		}																																		\
		else																																\
			head = item;																											\
	} while (0)
#define LL_Last(head,item)																							\
	do {																																	\
		item = head;																												\
		while ((*(item))->next) item = (*(item))->next;											\
	} while (0)
#define LL_Parent(head,item,parent)																			\
	do {																																	\
		parent = head;																											\
		while (parent && ((*parent)->next)!=(item))	\
			parent = (*(parent))->next;																				\
	} while (0)

/************************************************************************
 * Associative Array stuff
 ************************************************************************/
typedef struct AssocArray {
	short keySize;		/* length of keys */
	short dataSize;		/* length of data blocks */
} AssocArray, *AAPtr, **AAHandle;
AAHandle AANew(short keySize, short dataSize);
OSErr AAAddItem(AAHandle aa, Boolean replace, PStr key, UPtr data);
OSErr AAAddResItem(AAHandle aa, Boolean replace, short keyId, UPtr data);
OSErr AADeleteKey(AAHandle aa, PStr key);
OSErr AAFetchData(AAHandle aa, PStr key, UPtr data);
OSErr AAFetchResData(AAHandle aa, short keyId, UPtr data);
OSErr AAFetchIndData(AAHandle aa, short index, UPtr data);
OSErr AAFetchIndKey(AAHandle aa, short index, PStr key);
short AACountItems(AAHandle aa);
#define AAZap(aaH) ZapHandle(aaH)
short AAFindKey(AAHandle aa, PStr key);

typedef struct {
	short id;
	uLong used;
	uLong persId;
	Str255 string;
} StringCacheEntry, *SCPtr, **SCHandle;


/**********************************************************************
 * replace RemoveResource calls to fix OS bug
 **********************************************************************/
//#define RmveResource(h) MyRemoveResource(h)   * Already defined to RemoveResource
// #define RemoveResource(h) MyRemoveResource(h) CK

/**********************************************************************
 * declarations for functions in util.c
 **********************************************************************/
typedef struct {
	long elSize;
	long elCount;
} StackType, *StackPtr, **StackHandle;
OSErr StackInit(long size, StackHandle * stack);
OSErr StackPush(void *what, StackHandle stack);
OSErr StackPop(void *into, StackHandle stack);
OSErr StackQueue(void *what, StackHandle stack);
OSErr StackTop(void *into, StackHandle stack);
OSErr StackItem(void *into, short item, StackHandle stack);
void StackCompact(StackHandle stack);
short StackStringFind(PStr find, StackHandle stack);
void SCClear(short theId);
short CountStrnRes(UHandle resH);
void Rude(void);
void CheckNone(MenuHandle mh);
Boolean OnBatteries(void);
void WriteZero(void *pointer, long size);
#define Zero(x) WriteZero(&(x),sizeof(x))
void MacInitialize(int masterCount, long ensureStack);
PStr GetRString(PStr theString, short theIndex);
short GetFontID(UPtr theName);
Boolean GrabEvent(EventRecord * theEvent);
short SubmenuId(MenuHandle mh, short item);
Boolean MyOSEventAvail(short mask, EventRecord * event);
#define OSEventAvail MyOSEventAvail
typedef struct Accumulator {
	long offset;
	long size;
	Handle data;
	OSErr err;
} Accumulator, *AccuPtr, **AccuHandle;
OSErr AccuInit(AccuPtr a);
void AccuInitWithHandle(AccuPtr a, Handle h);
void AccuTrim(AccuPtr a);
OSErr AccuAddTrPtr(AccuPtr a, void *bytes, long len, UPtr from, UPtr to);
OSErr AccuAddTrHandle(AccuPtr a, Handle data, UPtr from, UPtr to);
OSErr AccuAddPtr(AccuPtr a, void *bytes, long len);
OSErr AccuAddPtrB64(AccuPtr a, void *bytes, long len);
OSErr AccuAddHandle(AccuPtr a, Handle data);
OSErr AccuAddHandleToPtr(AccuPtr a, UPtr data, long size);
OSErr AccuAddLong(AccuPtr a, uLong longVal);
#define AccuAddHandleToStr(a,s)	AccuAddHandleToPtr((a),(s)+1,*(s))
OSErr AccuAddChar(AccuPtr a, Byte c);
OSErr AccuInsertPtr(AccuPtr a, UPtr bytes, long len, long offset);
OSErr AccuInsertChar(AccuPtr a, Byte c, long offset);
OSErr AccuAddFromHandle(AccuPtr a, Handle data, long offset, long len);
long AccuFTell(AccuPtr a, short refN);
OSErr AccuFSeek(AccuPtr a, short refN, long fromStart);
long Atoi(UPtr s);
OSType AToOSType(UPtr s);
OSErr AccuAddRes(AccuPtr a, short res);
OSErr AccuWrite(AccuPtr a, short refN);
#define AccuAddStr(a,s) AccuAddPtr(a,s+1,*s)
#define AccuAddStrB64(a,s) AccuAddPtrB64(a,s+1,*s)
#define AccuToStr(a,s)	do {*(s) = min(255,(a)->offset); BMD(*(a)->data,(s)+1,*(s));}while(0)
OSErr AccuStrip(AccuPtr a, long num);
#define AccuZap(a) do{ZapHandle((a).data);(a).offset=(a).size=0;}while(0)
long AccuFindPtr(AccuPtr a, UPtr stuff, short len);
long AccuFindLong(AccuPtr a, uLong theLong);
OSErr AccuAddSortedLong(AccuPtr a, long addVal);
short DecodeB64Accu(AccuPtr a, Boolean isText);
void CheckFontSize(int menu, int size, Boolean check);
void CheckFont(int menu, int fontID, Boolean check);
void OutlineFontSizes(int menu, int fontID);
int GetLeading(int fontID, int fontSize);
int GetWidth(int fontID, int fontSize);
int GetDescent(int fontID, int fontSize);
int GetAscent(int fontID, int fontSize);
Boolean IsFixed(int fontID, int fontSize);
void AwaitKey(void);
void AddPResource(UPtr, int, long, int, UPtr);
void ChangePResource(UPtr theData, int theLength, long theType, int theID,
		     UPtr theName);
long GetRLong(int index);
OSType GetROSType(int index);
int ResourceCpy(short toRef, short fromRef, long type, int id);
void WhiteRect(Rect * r);
void DrawTruncString(UPtr string, int len);
int CalcTextTrunc(UPtr text, short length, short width, GrafPtr port);
#define CalcTrunc(text,width,port) CalcTextTrunc((text)+1,*(text),width,port)
int WannaSave(MyWindowPtr win);
void ButtonFit(ControlHandle button);
uLong GestaltBits(OSType selector);
void GetPassStuff(PStr persName, PStr uName, PStr hName);
#ifdef	KERBEROS
int GetPassword(void);
#else
int GetPassword(PStr personality, PStr userName, PStr serverName,
		UPtr word, int size, short prompt);
#endif
void CenterRectIn(Rect * inner, Rect * outer);
void TopCenterRectIn(Rect * inner, Rect * outer);
void BottomCenterRectIn(Rect * inner, Rect * outer);
void ThirdCenterRectIn(Rect * inner, Rect * outer);
short MyUniqueID(ResType type);
Boolean HasDragManager();
OSErr FinderDragVoodoo(DragReference drag);
typedef enum { Stop, Note, Caution, Normal } AlertEnum;
void MyAppendMenu(MenuHandle menu, UPtr name);
void MyInsMenuItem(MenuHandle menu, UPtr name, short afterItem);
void MySetItem(MenuHandle menu, short item, PStr itemStr);
PStr MyGetItem(MenuHandle menu, short item, PStr name);
OSErr CopyMenuItem(MenuHandle fromMenu, short fromItem, MenuHandle toMenu,
		   short toItem);
short CurrentModifiers(void);
void SpecialKeys(EventRecord * event);
short FindItemByName(MenuHandle menu, UPtr name);
short BinFindItemByName(MenuHandle menu, UPtr name);
void AttachHierMenu(short menu, short item, short hierId);
void *NuDHTempBetter(void *data, long size);
Boolean DirtyKey(long keyAndChar);
long RemoveChar(Byte c, UPtr text, long size);
long RemoveCharHandle(Byte c, UHandle text);
UPtr GetRStr(UPtr string, short id);
UPtr LocalDateTimeStr(UPtr string);
PStr LocalDateTimeShortStr(PStr s);
// (jp) Universal Headers 3.4 now contains a structure named "LocalDateTime"
#define	LocalDateTime	MyLocalDateTime
uLong LocalDateTime(void);
uLong GMTDateTime(void);
long MyMenuKeyLo(EventRecord * event, Boolean enable);
#define MyMenuKey(e)	MyMenuKeyLo((e),true)
#define UnadornMessage(event)	UnadornKey((event)->message,(event)->modifiers)
long UnadornKey(long message, short modifiers);
UPtr ChangeStrn(short resId, short num, UPtr string);
OSErr MyTrackDrag(DragReference drag, EventRecord * event, RgnHandle rgn);
OSErr MySetDragItemFlavorData(DragReference drag, short item, OSType type,
			      void *data, long len);
short DragOrMods(DragReference drag);
Boolean RecountStrn(short resId);
short CountStrn(short resId);
void NukeMenuItemByName(short menuId, UPtr itemName);
void RenameItem(short menuId, UPtr oldName, UPtr newName);
Boolean HasSubmenu(MenuHandle mh, short item);
OSErr ComposeRTrans(TransStream stream, short format, ...);
Boolean SetGreyControl(ControlHandle cntl, Boolean shdBeGrey);
Boolean IsAUX(void);
long ZoneSecs(void);
short MyCountDragItems(DragReference drag);
short MyCountDragItemFlavors(DragReference drag, short item);
OSType MyGetDragItemFlavorType(DragReference drag, short item,
			       short flavor);
FlavorFlags MyGetDragItemFlavorFlags(DragReference drag, short item,
				     short flavor);
Boolean MyDragHas(DragReference drag, short item, OSType type);
OSErr MyGetDragItemData(DragReference drag, short item, OSType type,
			Handle * data);
void NOOP(void);
Boolean WNE(short eventMask, EventRecord * event, long sleep);
long RoundDiv(long quantity, long unit);
void TransLitString(UPtr string);
void TransLit(UPtr string, long len, UPtr table);
void TransLitRes(UPtr string, long len, short resId);
long TZName2Offset(CStr zoneName);
#ifdef DEBUG
#define UseResFile MyUseResFile
void MyUseResFile(short refN);
#endif
void InvalidatePasswords(Boolean pwGood, Boolean auxpwGood, Boolean all);
void InvalidateCurrentPasswords(Boolean pwGood, Boolean auxpwGood);
Boolean MiniEventsLo(long sleepTime, uLong mask);
#define MiniEvents() MiniEventsLo(0,MINI_MASK)
short FindSTRNIndex(short resId, PStr string);
short FindSTRNIndexRes(UHandle resH, PStr string);
short FindSTRNSubIndex(short resId, PStr string);
short FindSTRNSubIndexRes(UHandle resH, PStr string);
WindowPtr Event2Window(EventRecord * event);
PStr Long2Hex(PStr hex, long aLong);
UPtr Bytes2Hex(UPtr bytes, long size, UPtr hex);
OSErr Hex2Bytes(UPtr hex, long size, UPtr bytes);
void *NuHTempOK(long size);
void *NuHTempBetter(long size);
Handle NewIOBHandle(long min, long max);
long AFPopUpMenuSelect(MenuHandle mh, short top, short left, short item);
Boolean GetTableCName(short tid, PStr name);
Boolean GetTableID(PStr name, short *tid);
Boolean EventPending(void);
void ShowDragRectHilite(DragReference drag, Rect * r, Boolean inside);
PStr WeekDay(PStr string, long secs);
OSErr ZapResourceLo(OSType type, short id, Boolean one);
#define ZapResource(x,y) ZapResourceLo(x,y,False)
#define Zap1Resource(x,y) ZapResourceLo(x,y,True)
#define ControlIsGrey(cntl)	(GetControlHilite(cntl)==255)
void AddMyResource(Handle h, OSType type, short id, ConstStr255Param name);
#define CurrentPSN(psn) (((psn)->highLongOfPSN = 0),((psn)->lowLongOfPSN = kCurrentProcess),(psn))
long CountChars(Handle text, Byte c);
long CountCharsPtr(UPtr ptr, long size, Byte c);
OSErr HandleLinebreaks(Handle text, long ***breaks, short inWidth);

short MenuWidth(MenuHandle mh);
//#define IsColorWin(win) \
//      (ThereIsColor && \
//       (((GrafPtr)(win))->portBits.rowBytes & 0xC000) && \
//   ((**((CGrafPtr)(win))->portPixMap).pixelSize > 1))
Boolean IsColorWin(WindowPtr winWP);
#define PurgeIfClean(h) do{UL(h);if (!(GetResAttrs((Handle)h)&resChanged)) HPurge((Handle)h);}while(0)
typedef struct {
	Handle textH;
	UPtr textP;
	long len;
	long lineBegin;
	long lineEnd;
	short partial;
} WrapDescriptor, WrapPtr;
void PlayNamedSound(PStr name);
void PlaySoundId(short id);
short FindMenuByName(UPtr name);
RGBColor *GetItemColor(short menu, short item, RGBColor * color);
Boolean IsHexDig(Byte c);
Boolean IsEnabled(short menu, short item);
Boolean SafeToAllocate(long size);
RGBColor *GetRColor(RGBColor * color, int index);
RGBColor *GetRTextColor(RGBColor * color, int index);
OSErr AddLf(Handle text);
void *NuDHTempOK(void *data, long size);
PStr Color2String(PStr string, RGBColor * color);
void InitWrap(WrapPtr wp, Handle textH, UPtr textP, long len, long offset,
	      long lastLen);
short Wrap(WrapPtr wp);
Boolean IsPowerNoVM(void);
void SetHiliteMode(void);
void SetItemReducedIcon(MenuHandle menu, short item, short iconid);
UHandle PStr2Handle(PStr string);
long ScriptVar(short selector);
Boolean MyWaitMouseMoved(Point pt, Boolean honorControl);
void *ZeroHandle(void *hand);
void SetItemR(MenuHandle menu, short item, short id);
Boolean IsVICOM(void);
OSErr MyRemoveResource(Handle h);
short ShortCompare(short value1, short value2);
short DateCompare(DateTimeRec * date1, DateTimeRec * date2);
short TimeCompare(DateTimeRec * date1, DateTimeRec * date2);
short GetOSVersion(void);
#define HaveTheDiseaseCalledOSX HaveOSX
Boolean HaveOSX(void);
void AddSoundsToMenu(MenuHandle mh);
void PlaySoundIdle(void);

#ifdef DEBUG
void SetBalloons(Boolean on);
#else
#define SetBalloons HMSetBalloons
#endif
#define Pause(t) do {long tk=TickCount()+t; while (TickCount()<tk) {WNE(0,nil,tk-TickCount());}} while(0)
#define DIR_MASK	8	/* mask to use to test file attrib for directory bit */
#define OPTIMAL_BUFFER (64 K)	/* file buffer size */
#define SAME_COLOR(c1,c2)	((c1).red==(c2).red && (c1).green==(c2).green && (c1).blue==(c2).blue)

#define RectHi(r) ((r).bottom-(r).top)
#define RectWi(r) ((r).right-(r).left)

#define optSpace 0xca
#define enterChar 0x03
#define escChar 0x1b
#define clearChar 0x1b
#define escKey 0x35
#define clearKey 0x47
#define delChar 0x7f
#define backSpace 0x08
#define returnChar 0x0d
#define bulletChar 0xa5
#define tabChar 0x09
#define leftArrowChar 0x1c
#define rightArrowChar 0x1d
#define upArrowChar 0x1e
#define downArrowChar 0x1f
#define homeChar 0x01
#define endChar 0x04
#define helpChar 0x05
#define pageUpChar 0x0b
#define pageDownChar 0x0c
#define undoKey 0x7a
#define cutKey 0x78
#define copyKey 0x63
#define pasteKey 0x76
#define clearKey 0x47
#define betaChar 0xa7
#define deltaChar 0xc6
#define lowerDelta	((unsigned char)'�')
#define nbSpaceChar 0xca
#define lowerOmega ((unsigned char)'�')
#define diamondChar ((unsigned char)'�')
#define commaChar 0x2C
#define lessThanOrEqualToChar 0xB2
#define optionCommaChar lessThanOrEqualToChar
#define periodChar 0x2E
#define ellipsesChar 0xC9	/* ASCII code for ellipses character (�) */

enum { smScriptSmallSysFondSize = 0x1243 };

#define MINI_MASK	(everyEvent & ~(highLevelEventMask|keyDownMask|((InAThread()||!ModalWindow)?(mDownMask|mUpMask):0)))

#define GetResource NoSLGetResource
#define Get1Resource NoSLGet1Resource
#define Get1IndResource NoSLGet1IndResource
#define GetIndResource NoSLGetIndResource
#define GetNamedResource NoSLGetNamedResource
#define Get1NamedResource NoSLGet1NamedResource
#define GetMenu NoSLGetMenu
#undef GetMHandle
#define GetMHandle NoSLGetMHandle
Handle NoSLGetResource(OSType type, short id);
Handle NoSLGet1Resource(OSType type, short id);
Handle NoSLGet1IndResource(OSType type, short ind);
Handle NoSLGetIndResource(OSType type, short ind);
Handle NoSLGetNamedResource(OSType type, PStr name);
Handle NoSLGet1NamedResource(OSType type, PStr name);
MenuHandle NoSLGetMenu(short id);
MenuHandle NoSLGetMHandle(short id);
#endif
