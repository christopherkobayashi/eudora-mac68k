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

#ifndef WINUTIL_H
#define WINUTIL_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
Boolean WinRectUnderFloaters(WindowPtr winWP, Rect * rect,
			     WindowRegionCode rgnCode);
void BoldString(PStr string);
void DrawRString(short id);
void BoldRString(short id);
void Hairline(Boolean on);
OSErr OutlineRgn(RgnHandle rgn, short inset);
Boolean DragIsInteresting(DragReference drag, ...);
Boolean DragIsInterestingFileType(DragReference drag,
				  Boolean(*testFunc) (FSSpecPtr), ...);
Boolean DragIsImage(DragReference drag);
void GetDisplayRect(GDHandle gd, Rect * r);
void RedrawAllWindows(void);
void SetDIText(DialogPtr dialog, int item, UPtr text);
void GetDIText(DialogPtr dialog, int item, UPtr text);
short SetDItemState(DialogPtr pd, short dItem, short hilite);
short GetDItemState(DialogPtr pd, short dItem);
pascal Boolean DlgFilter(DialogPtr dgPtr, EventRecord * event,
			 short *item);
short WindowHi(WindowPtr theWindow);
short WindowWi(WindowPtr theWindow);
void GlobalizeRgn(RgnHandle rgn);
void LocalizeRgn(RgnHandle rgn);
void HiliteButtonOne(DialogPtr dgPtr);
void DlgCursor(Point mouse);
void SetDICTitle(DialogPtr dlog, short item, PStr title);
PStr GetDICTitle(DialogPtr dlog, short item, PStr title);
void HiliteDIControl(DialogPtr dlog, short item, short hilite);
typedef short SICN[16];
typedef SICN **SICNHand;
void PlotSICN(Rect * theRect, SICNHand theSICN, long theIndex);
void PlotSICNRes(Rect * theRect, short resId, long theIndex);
void PlotFullSICNRes(Rect * theRect, short resId, long theIndex);
void SavePosPrefs(UPtr name, Rect * r, Boolean zoomed);
ControlHandle GetDItemCtl(DialogPtr pd, short dItem);
void SavePosFork(short vRef, long dirId, UPtr name, Rect * r,
		 Boolean zoomed);
Boolean RestorePosPrefs(UPtr name, Rect * r, Boolean * zoomed);
Boolean RestorePosFork(short vRef, long dirId, UPtr name, Rect * r,
		       Boolean * zoomed);
Boolean PositionPrefsTitle(Boolean save, MyWindowPtr win);
Boolean PositionPrefsByName(Boolean save, MyWindowPtr win,
			    StringPtr title);
void ZoomPosition(MyWindowPtr win);
void DefPosition(WindowPtr theWindow, Rect * r);
void GreyOutRoundRect(Rect * r, short r1, short r2);
void PushGWorld(void);
void PopGWorld(void);
UPtr SetDIPopup(DialogPtr pd, short item, UPtr toName);
UPtr GetDIPopup(DialogPtr pd, short item, UPtr whatName);
void SanitizeSize(MyWindowPtr win, Rect * r);
void HotRect(Rect * r, Boolean on);
Boolean MyWinHasSelection(MyWindowPtr win);
Boolean PtInSlopRect(Point pt, Rect r, short slop);
ControlHandle FindControlByRefCon(MyWindowPtr win, long refCon);
void MaxSizeZoom(MyWindowPtr win, Rect * zoom);
void RgnMumbleRect(RgnHandle rgn, Rect * r, Boolean add);
#define RgnMinusRect(x,y) RgnMumbleRect(x,y,False)
#define RgnPlusRect(x,y) RgnMumbleRect(x,y,True)
MenuHandle PopUpMenuH(ControlHandle ctl);
#define StdState(aWindowPtr) (*(WStateData**)((WindowPeek)aWindowPtr)->dataHandle)->stdState
#define UserState(aWindowPtr) (*(WStateData**)((WindowPeek)aWindowPtr)->dataHandle)->userState
Rect CurState(WindowPtr theWindow);
Boolean AboutSameRect(Rect * r1, Rect * r2);
void MySetControlTitle(ControlHandle cntl, PStr title);
void MySetCtlValue(ControlHandle cntl, short val);
void EraseRectExceptPete(MyWindowPtr win, Rect * r);
void InvalRectExceptPete(MyWindowPtr win, Rect * r);
OSStatus MySetThemeWindowBackground(MyWindowPtr window, ThemeBrush brush,
				    Boolean update);
ControlHandle MyFindControl(Point pt, MyWindowPtr win);
Boolean PtInControlLo(Point pt, ControlHandle cntl, Boolean invisibleOK);
#define PtInControl(pt,cntl) PtInControlLo(pt,cntl,false)
Boolean MouseInControl(ControlHandle cntl);
Boolean MouseInRect(Rect * r);
void ReplaceControl(DialogPtr dlog, short item, short type,
		    ControlHandle * itemH, Rect * r);
void OffsetDItem(DialogPtr dlog, short item, short dh, short dv);
void ReplaceAllControls(DialogPtr dlog);
void ChangeTEFont(TEHandle teh, short font, short size);
short TitleBarHeight(WindowPtr winWP);
short UselessHeight(WindowPtr winWP);
short LeftRimWidth(WindowPtr winWP);
short UselessWidth(WindowPtr winWP);
void ComputeWinUseless(WindowPtr theWindow, short *title, short *leftRim,
		       short *uselessHi, short *uselessWi);
typedef struct {
	Byte hPitch;
	Byte vPitch;
	short txFont;
	Style txFace;
	short txSize;
	short txMode;
	RGBColor fore;
	RGBColor back;
	PenState pnState;
	ThemeDrawingState themeState;
} PortSaveStuff, *PortSaveStuffPtr, **PortSaveStuffHandle;
void SetPortMyWinColors(void);
void SavePortStuff(PortSaveStuffPtr stuff);
void SetPortStuff(PortSaveStuffPtr stuff);
void DisposePortStuff(PortSaveStuffPtr stuff);
#define RGBBackColor	MyRGBBackColor
void MyRGBBackColor(RGBColor * color);
#define RGBForeColor	MyRGBForeColor
void MyRGBForeColor(RGBColor * color);
#define SAVE_STUFF	PortSaveStuff stuff; SavePortStuff(&stuff)
#define REST_STUFF	do{SetPortStuff(&stuff);DisposePortStuff(&stuff);}while(0)
#define SET_COLORS	SetPortMyWinColors()
#define DrawDialog	MyDrawDialog
void MyDrawDialog(DialogPtr dgPtr);
#define UpdateDialog(aDialogPtr,y)	DlgUpdate(GetDialogMyWindowPtr(aDialogPtr))
PStr MyGetWTitle(WindowPtr theWindow, PStr title);
void DlgUpdate(MyWindowPtr win);
void SetSmallSysFont(void);
short SmallSysFontID(void);
short SmallSysFontSize(void);
void OutlineControl(ControlHandle cntl, Boolean blackOrWhite);
void Update1Control(ControlHandle cntl, RgnHandle rgn);
MyWindowPtr GetNthWindow(short n);
short GetAnswer(PStr prompt, PStr answer);
Boolean TrackRect(Rect * r);
MenuHandle Control2Menu(ControlHandle cntl);
void DrawRJust(PStr s, short h, short v);
Boolean MouseStill(uLong howManyTicks);
long DragMyGreyRgn(RgnHandle dragRgn, Point pt,
		   void (*DragProc)(Point *, RgnHandle, long),
		   long refCon);
Boolean PtInSloppyRect(Point pt, Rect * r, short slop);
void FrameXorContent(MyWindowPtr win);
void ConfigFontSetup(MyWindowPtr dlogWin);
#ifdef DEBUG
void ShowGlobalRgn(RgnHandle globalRgn, UPtr string);
void ShowLocalRgn(RgnHandle localRgn, UPtr string);
#endif
PStr GetResName(PStr into, ResType type, short id);
void AppendDITLId(DialogPtr dlog, short id, DITLMethod method);
Boolean EnableDItemIf(DialogPtr pd, short dItem, Boolean on);
Boolean DialogRadioButtons(MyWindowPtr dlogWin, short item);
Boolean DialogCheckbox(MyWindowPtr win, short item);
void HiInvertRect(Rect * r);
void HiInvertRgn(RgnHandle r);
void DrawRString(short id);
short RStringWidth(short id);
void Frame3DRectDepth(Rect * bigRect, IconTransformType transform,
		      short depth);
void FrameGreys(Rect * r, short *greys);
void Frame3DRect(Rect * r, IconTransformType transform);
short RectDepth(Rect * localRect);
Point DragDivider(Point pt, Rect * divider, Rect * bounds,
		  MyWindowPtr win);
void MyDisposeDrag(DragReference drag);
OSErr MyNewDrag(MyWindowPtr win, DragReference * drag);
void ScreenChange(void);
#define MIN_APPEAR_SPACE	(2*MAX_APPEAR_RIM)
#define MAX_APPEAR_RIM (4)
#define MyStaggerWindow(w,r,c) 	utl_StaggerWindow(w,r,1,TitleBarHeight(w),TitleBarHeight(w),LeftRimWidth(w),c,GetPrefLong(PREF_NW_DEV));
Boolean MyStdFilterProc(DialogRef dgPtr, EventRecord * event,
			DialogItemIndex * item);

#endif
