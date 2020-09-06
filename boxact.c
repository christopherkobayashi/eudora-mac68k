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

#include "boxact.h"
#define FILE_NUM 5
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Boxes

/**********************************************************************
 * Private prototypes
 **********************************************************************/
// Drawing
void InvertSum(TOCHandle tocH,short sumNum,Boolean forActivate);
void BoxCenter(MyWindowPtr win, short mNum);
void BoxSizeBox(MyWindowPtr win,Rect *r,TOCHandle tocH);
short BoxPreviewHeightWish(TOCHandle tocH);
void BoxZoomSize(MyWindowPtr win,Rect *zoom);
void InvertSumByColumns(TOCHandle tocH,Rect *r);
Boolean BoxDrawerUseless(MyWindowPtr win, short *left, short *right);
// Clicking
void BoxTrack(TOCHandle tocH,int anchor,Boolean shift);
Boolean BoxDragDividers(Point pt, MyWindowPtr win);
Boolean BoxFindDivider(Point pt,short *which);
TOCHandle TOCHUnder(Point pt);
void BoxSetPreview(MyWindowPtr win,Boolean openIt);
void BoxDragPreviewDivider(TOCHandle tocH,Point pt);
Boolean BoxListKey(MyWindowPtr win, EventRecord *event);
Boolean BoxPreviewKey(MyWindowPtr win, EventRecord *event);
void BoxConConProfControl(TOCHandle tocH,ControlHandle cntl,Point pt);
// Dragging
OSErr BoxDrop(MyWindowPtr win, DragReference drag);
RgnHandle BoxBuildDragRgn(TOCHandle tocH);
void BoxDroppedOn(TOCHandle origTocH,MessHandle messH,Point pt);
OSErr BoxMessDragSendLo(FlavorType flavor, void *dragSendRefCon, DragReference drag, TOCHandle tocH,short sumNum);
// Menus
void BoxPriorMenu(TOCHandle tocH,short mNum,Point pt);
void BoxServerMenu(TOCHandle tocH,short mNum,Point pt);
void BoxStateMenu(TOCHandle tocH,short mNum,Point pt);
void BoxMailboxMenu(TOCHandle tocH,short mNum,Point pt);
void BoxAttachMenu(TOCHandle tocH,short mNum,Point pt);
void BoxBuildConConProfMenu(TOCHandle tocH,ControlHandle cntl);
// Misc
void DoSaveSelectedAs(TOCHandle tocH);
void SizeBoxClick(MyWindowPtr win,short modifiers);
OSErr CopySummary(TOCHandle tocH, short sumNum, Handle into);
void BoxType2Select(TOCHandle tocH,PStr string);
OSErr CopySelectedSummaries(TOCHandle tocH);
short BoxCountSelected(TOCHandle tocH);
void SetTAEScoreLo(TOCHandle tocH,short sumNum,short score);
void OneAnalMess(TOCHandle tocH,short sumNum);
OSErr BoxReply(TOCHandle tocH,short menu,short item,short modifiers);
void BoxRefreshPreview(TOCHandle tocH);
Boolean BoxRequestFocus(MyWindowPtr win, PETEHandle pte);
PStr BoxCurAddr(MyWindowPtr win, PStr addr);
// Sorting
#define DisplayLength(sum)	((sum)->length-(sum)->bodyOffset+1023)
#define EffectiveLength(sum)	((sum->flags&FLAG_SKIPPED) ? -1 K : ((sum->opts&OPT_JUSTSUB) ? -2 K : DisplayLength(sum)))
void BoxToggleLaurence(TOCHandle tocH);
void BoxCustomSort(TOCHandle tocH,short i);
void SortTOCMenu(TOCHandle tocH, short item, Boolean reverse);
void SortTOC(TOCHandle tocH, Boolean reverse, int (*compare)());
void SwapSum(MSumPtr sum1, MSumPtr sum2);
short BoxLine2Item(short line);
short BoxItem2Line(short item);
void MBRemoveSort(TOCHandle tocH,short index);
void MBAddSort(TOCHandle tocH,short index,Byte sortOrder);
#define MBGetSort(tocH,index)	(*tocH)->sorts[index-1]
int (*FindMBSort(short item,Boolean reverse))();
int MBResortCompare(MSumPtr s1, MSumPtr s2);
void MBResort(TOCHandle tocH);
void MBSortHit(TOCHandle tocH, short index, Boolean reverse, Boolean extend);
Boolean MBIsSticky(TOCHandle tocH);
int SumSubjIdCompare(MSumPtr sum1, MSumPtr sum2);
void BoxMakePreview(MyWindowPtr win);
short BoxSizeWidth(TOCHandle tocH);
void SetPriorityLo(TOCHandle tocH,short sumNum,short priority);
long MyPopupMenuSelect(MyWindowPtr win,MenuHandle mh,Point where,short item);
#define NSORT	(sizeof((*tocH)->sorts)/sizeof(Byte))
#define SORTER(what)	int Sum##what##Compare(MSumPtr,MSumPtr); int RevSum##what##Compare(MSumPtr,MSumPtr)
SORTER(Time);
SORTER(Stat);
SORTER(Junk);
SORTER(Att);
SORTER(Size);
SORTER(Prior);
SORTER(Subj);
SORTER(SubjTimed);
SORTER(From);
SORTER(Offset);
SORTER(Label);
SORTER(SizeK);
SORTER(TimeFuzz);
SORTER(Select);
SORTER(Mailbox);
SORTER(Anal);

#define SORT_BUTTON_WIDE 11
#define kHeaderButtonHeight (GROW_SIZE+5)
#define kTabHeight 25
#define kTabGap 2
#define kLargeUniqueValue 0x7f7f7f7f

short BoxPreviewHeight(TOCHandle tocH);
pascal OSErr MessDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag);
pascal OSErr BoxDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag);

void HitTab(MyWindowPtr win,ControlHandle tabCntl,TOCHandle tocH);

//	globals
TOCHandle gSortTOC;
MyWindowPtr gDrawerOpenedWin;

/************************************************************************
 * BoxLimits - return the left and right sides of a particular mailbox
 * column.  Values are NOT shifted according to scroll bars.  Values are
 * in pixels, not character widths
 ************************************************************************/
short BoxLimits(BoxLinesEnum which,short *left,short *right,TOCHandle tocH)
{
	short column;
	short wi;
	short i;
			
	*right = 0;
	for (i=1;;i++)
	{
		column = TOCInversionMatrix[0][i];
		if (!BoxColumnShowing(column,tocH))
			wi = 0;
		else
		{
			ASSERT(column > 0 && column<BoxLinesLimit);
			wi = (*BoxWidths)[column-1] ? (*BoxWidths)[column-1]*FontWidth : 4;
			*right += wi;
		}
		if (column==which) break;
	}
	*left = *right-wi;
	return(*right-*left);
}

/**********************************************************************
 * BoxColumnShowing - is a column showing?
 **********************************************************************/
Boolean BoxColumnShowing(BoxLinesEnum which,TOCHandle tocH)
{
	// junk column is hidden by default in all mailboxes except junk,
	// where it always shows
	if (which==blJunk)
	{
		if (((*tocH)->which==JUNK) || IsIMAPJunkMailbox(TOCToMbox(tocH))) return true;
		else return 0!=(GetPrefLong(PREF_BOX_HIDDEN)&(1<<(which-1)));
	}
	
	// Ordinary column processing
	if (GetPrefLong(PREF_BOX_HIDDEN)&(1<<(which-1))) return false;
	
	// other special items
	if (which==blMailbox && !(tocH && (*tocH)->virtualTOC)) return false;
	if (which==blAnal && (AnalDisabled() || !AnalDoIncoming())) return false;
	return true;
}

/**********************************************************************
 * BoxUpdate - handle an update event for a mailbox window
 **********************************************************************/
#define SUMMARY_ICON_SETUP do { \
		r.bottom = pr.bottom-1; \
		r.top = pr.top+1; \
		r.left = h0+left+2; r.right = h0+right-2; \
		SetRect(&ir,0,0,16,16); \
		CenterRectIn(&ir,&r); \
		r.bottom = MIN(r.bottom,win->contR.bottom); \
		r.top = MAX(r.top,cr.bottom-win->vPitch); \
		ClipRect(&r); \
	} while (0);

void BoxUpdate(MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	CGrafPtr	winWPPort = GetWindowPort(winWP);
	TOCHandle tocH = (TOCHandle) GetWindowPrivateData(winWP);
	MSumType sum;
	int min, max, sumNum;
	int line;
	int h,v;
	Str31 states,scratch;
	Str31 pfix;
	Rect r,pr,cr,ir;
	short h0;
	short greyMax;
  Boolean isOut = (*tocH)->which==OUT;
	short left,right;
	Boolean hasColor = IsColorWin(winWP);
	RGBColor oldColor, color, textColor;
	Str255 string;
	short label;
	PersHandle pers;
	short lPref;
	short depth = RectDepth(&win->contR);
	Boolean useUglyStupidIconsBecauseLettersAreTooClearAndUnambiguous = !PrefIsSet(PREF_NO_SUMMARY_ICONS);
	Rect visR = *GetVisibleRgnBounds(winWP,&visR);
	short inIn = GetRLong(IN_IN_STYLE);			// Style to be used for incoming messages in an incoming box
	short outOut = GetRLong(OUT_OUT_STYLE);	// Style to be used for outgoing messages in the Out box
	short inOut = GetRLong(IN_OUT_STYLE);		// Style to be used for incoming messages in the Out box
	Pattern	thePattern;
	
	/*
	 * redraw it all if we need to calculate the window width
	 */
	if (win->hMax < 0)
	{
		(*tocH)->needRedo = 0;
		return;
	}
	
	/*
	 * reset font & size, just in case
	 */
	TextFont(FontID);
	TextSize(FontSize);
		
	/*
	 * Set the clipping region to be the content minus the scroll bars plus the top margin
	 */
	r = win->contR;
	r.top = 0;
	ClipRect(&r);

	/*
	 * Set min and max to be the top & bottom visible summary lines
	 */
	min = GetControlValue(win->vBar);
	max = min+RoundDiv(win->contR.bottom-win->contR.top/*-win->topMargin*/,win->vPitch)+1;
	max = MIN((*tocH)->count-1,max);
	h = h0 = -GetControlValue(win->hBar)*win->hPitch;
	r = win->contR;

	/*
	 * draw the column colors
	 */
	if (UseListColors)
	{
		for (line=1;line<BoxLinesLimit;line++)
		{
			if (BoxLimits(line,&r.left,&r.right,tocH))
			{
				OffsetRect(&r,h0,0);
				SetThemeBackground(MBGetSort(tocH,line) ? kThemeBrushListViewSortColumnBackground:kThemeBrushListViewBackground,RectDepth(&r),true);
				EraseRect(&r);
			}
		}
		SET_COLORS;
	}
	
	/*
	 * draw the grey lines
	 */
	if (hasColor)
	{
		GetRColor(&textColor,TEXT_COLOR);
		Zero(oldColor);
	}
	if ((lPref=GetPrefLong(PREF_NO_LINES))!=3)
	{
		if (hasColor)
		{
			if (UseListColors)
				SetThemePen(kThemeBrushListViewSeparator,RectDepth(&win->contR),true);
			else
			{
				GetRColor(&color,ColorIsLight(&win->backColor) ? SEP_LINE_DARK_COLOR:SEP_LINE_LIGHT_COLOR);
				RGBForeColor(&color);
			}
		}
		else
			PenPat(GetQDGlobalsGray(&thePattern));
		greyMax = win->topMargin+(max-min+1)*(short)win->vPitch;
		
		if (!(lPref & 1)) for (v=win->topMargin;v<=greyMax;v+=win->vPitch)
		{
			MoveTo(0,v); Line(REAL_BIG,0);
		}
		if (!(lPref & 2)) for (line=1;line<BoxLinesLimit;line++)
		{
			if (BoxLimits(line,&left,&right,tocH))
			{
				MoveTo(h0+right-1,win->topMargin);
				Line(0,10000);
			}		
		}
	}
	
	// Adjust max
	if (max>(*tocH)->count-1) max = (*tocH)->count-1;

	/*
	 * now, draw the contents
	 */
	SET_COLORS;
	if (UseListColors) SetThemeTextColor(kThemeTextColorListView,depth,True); else RGBForeColor(&textColor);
	GetRString(pfix,(*tocH)->which==OUT?COMP_OUT_INTRO:COMP_IN_INTRO);
	sum=(*tocH)->sums[min];
	PenPat(GetQDGlobalsBlack(&thePattern));
	GetRString(states,STATE_LABELS);
	v = win->topMargin+(1-(GetControlValue(win->vBar)-min))*win->vPitch - FontDescent;
	r.left = 0;
	r.right = REAL_BIG;
	BoxLimits(blPrior,&pr.left,&pr.right,tocH);
	pr.left += (pr.right-pr.left-16+1)/2;
	pr.top = win->topMargin + (win->vPitch-16+1)/2;
	pr.bottom = pr.top+16;
	pr.right = pr.left+16;
	OffsetRect(&pr,h0,(min-GetControlValue(win->vBar))*win->vPitch);
	for (line=0,sumNum=min; line<=(max-min); line++,sumNum++,v+=win->vPitch,OffsetRect(&pr,0,win->vPitch))
	{
		sum = (*tocH)->sums[sumNum];
		
		if (pr.bottom<visR.top || pr.top>visR.bottom) continue;
		
		if (isOut)
			TextFace(outOut);
		else if (sum.state==SENT || sum.state==UNSENT || (sum.flags & FLAG_OUT) || (sum.opts & OPT_IMAP_SENT))
			TextFace(inOut);
		else
			TextFace(inIn);
			
		pers = SUM_TO_PPERS(&sum);

		BoxLimits(blPrior,&left,&right,tocH);
		cr.bottom = MIN(v+FontDescent+1,win->contR.bottom);
		cr.top = MAX(win->contR.top,cr.bottom-win->vPitch);
		cr.left = h0+left+1; cr.right = h0+right-2;
		ClipRect(&cr);
		if (right-left>6) DrawPriority(&pr,sum.priority);

		if (sum.flags & FLAG_HAS_ATT)
		{
			if (BoxLimits(blAttach,&left,&right,tocH)>6)
			{
				SUMMARY_ICON_SETUP;
				PlotIconID(&ir,atNone, ttNone,ATTACH_ICON);
			}
		}

		// moodmail
		if (BoxLimits(blAnal,&left,&right,tocH)>6)
		{
			// if it scored above one, display it
			if (sum.score > 1)
			{
				SUMMARY_ICON_SETUP;
				PlotIconID(&ir,atNone, ttNone,ANAL_ICON_BASE+sum.score-1);
			}
		}

		// draw the icon in the server status column
		if ((*tocH)->imapTOC)
		{			
			if (BoxLimits(blServer,&left,&right,tocH)>6)
			{
				SUMMARY_ICON_SETUP;
				
				// draw the DELETED icon if the message is marked for deletion
				if (sum.opts&OPT_DELETED) PlotIconID(&ir,atAbsoluteCenter,ttNone,IMAP_DELETED_ICON);	
				// draw the MISSING EVERYTHING icon if nothing has been fetched yet
				else if (sum.offset<0) PlotIconID(&ir,atAbsoluteCenter,ttNone,IMAP_NOTHING_FETCHED);
				// draw the MISSING ATTACHMENTS icon if one or more attachments is stubular
				else if (sum.opts&OPT_FETCH_ATTACHMENTS) PlotIconID(&ir,atAbsoluteCenter,ttNone,IMAP_FETCHED_BODY);
				// otherwise draw the COMPLETELY FETCHED icon if the message is totally present
				else PlotIconID(&ir,atAbsoluteCenter,ttNone,IMAP_FETCHED_ALL);
			}		
		}

		if (IdIsOnPOPD(PERS_POPD_TYPE(pers),POPD_ID,sum.uidHash))
		{
			short iconID;
		
			if (BoxLimits(blServer,&left,&right,tocH)>6)
			{
				SUMMARY_ICON_SETUP;
			
				iconID = ON_SERVER_ICON;
				if ((sum.flags & FLAG_SKIPPED) && IdIsOnPOPD(PERS_POPD_TYPE(pers),FETCH_ID,sum.uidHash))
				{
					if (IdIsOnPOPD(PERS_POPD_TYPE(pers),DELETE_ID,sum.uidHash))
						iconID = ON_BOTH_ICON;
					else
						iconID = ON_FETCH_ICON;
				}
				else if (IdIsOnPOPD(PERS_POPD_TYPE(pers),DELETE_ID,sum.uidHash))
					iconID = ON_DELETE_ICON;
				
				PlotIconID(&ir,atAbsoluteCenter,ttNone,iconID);
			}
		}

		if (label = SumColor(&sum))
		{
			MyGetLabel(label,&color,string);
			if (hasColor && !BlackWhite(&color) && !PrefIsSet(PREF_SMALL_COLOR)) RGBForeColor(DarkenColor(&color,GetRLong(TEXT_DARKER)));
		}
		else *string = 0;

		BoxLimits(blStat,&left,&right,tocH);

		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2+1,v);
			if (useUglyStupidIconsBecauseLettersAreTooClearAndUnambiguous)
			{
				SUMMARY_ICON_SETUP;
				PlotIconID(&ir,atAbsoluteCenter,ttNone,MESSAGE_STATUS_ICON_BASE+sum.state-1);
			}
			else			
				DrawChar(states[sum.state]);
		}
		
		
		//if (!isOut && sum.flags &FLAG_OUT) TextFace(italic);

		BoxLimits(blLabel,&left,&right,tocH);				
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			Boolean	mustRestore = false;
			short	saveTextMode = GetPortTextMode(GetQDGlobalsThePort());

			if (*string && PrefIsSet(PREF_SMALL_COLOR) && hasColor && !BlackWhite(&color))
			{
				Rect	rPaint;			

				rPaint.bottom = v+FontDescent+1;
				rPaint.top = MAX(win->contR.top,rPaint.bottom-win->vPitch);
				rPaint.bottom = MIN(rPaint.bottom,win->contR.bottom)-1;
				rPaint.left = cr.left-1;
				rPaint.right = cr.right+1;
				ClipRect(&rPaint);
				RGBForeColor(LightenColor(&color,GetRLong(AREA_LIGHTER)));
				PaintRect(&rPaint);
				TextMode(notSrcCopy);
				mustRestore = true;
			}
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2+1,v);
			DrawString(string);
			if (mustRestore)
			{
				TextMode(saveTextMode);
				if (hasColor) if (UseListColors) SetThemeTextColor(kThemeTextColorListView,depth,True); else RGBForeColor(&textColor);
			}
		}

		BoxLimits(blFrom,&left,&right,tocH);				
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2+1,v);
			if (*pfix && (isOut || (sum.flags & FLAG_OUT) || (sum.opts & OPT_IMAP_SENT))) DrawString(pfix);
			if(!(sum.flags & FLAG_UTF8) || !HasUnicode() || DrawUTF8Text(&sum.from[1], sum.from[0], right - left - 3, normFonts)) {
				DrawString(sum.from);
			}
		}
				
		BoxLimits(blDate,&left,&right,tocH);
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2+1,v);
			if (sum.seconds)
			{
				ComputeLocalDate(&sum,scratch);
				DrawString(scratch);
			}
		}

		BoxLimits(blSize,&left,&right,tocH);
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
		 	ClipRect(&cr);
#ifdef DEBUG
			if (RunType!=Production && BUG2)
			{
				NumToString(sum.serialNum,scratch);
#ifdef NEVER
				//	Check for and flag duplicate serial numbers with '�'
				//	Only works if mailbox is sorted by serial numbers
				if (sum.serialNum == sum[1].serialNum || sum.serialNum == sum[-1].serialNum)
					PCat(scratch,"\p �");
#endif
#ifdef NEVER
				NumToString(sum.score,scratch);
#endif
			}
			else
#endif
			if (sum.flags&FLAG_SKIPPED)
			{
				*scratch = 1;
				scratch[1] = '�';
			}
#ifdef NOBODY_SPECIAL
			else if (sum.opts & OPT_JUSTSUB)
			{
				*scratch = 1;
				scratch[1] = '�';
			}
#endif // NOBODY_SPECIAL
			else
			{
				NumToString(DisplayLength(&sum)/1024,scratch);
			}
			MoveTo(h0+right-FontWidth/4-StringWidth(scratch),v);
			if (!isOut && (sum.state==SENT || sum.state==UNSENT || (sum.flags & FLAG_OUT) || (sum.opts & OPT_IMAP_SENT))) Move(-(2*FontWidth)/3,0);
			DrawString(scratch);
		}

		BoxLimits(blJunk,&left,&right,tocH);
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth && sum.spamScore > 0)
		{
		 	ClipRect(&cr);
			NumToString(sum.spamScore,scratch);
#ifdef DEBUG
			if (BUG7)
			{
				PCatC(scratch,'-');
				PCatC(scratch,'0'+sum.spamBecause);
			}
#endif
			MoveTo(h0+right-FontWidth/4-StringWidth(scratch),v);
			if (!isOut && (sum.state==SENT || sum.state==UNSENT || (sum.flags & FLAG_OUT) || (sum.opts & OPT_IMAP_SENT))) Move(-(2*FontWidth)/3,0);
			DrawString(scratch);
		}
		
		BoxLimits(blMailbox,&left,&right,tocH);
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2+1,v);
			GetMailboxName(tocH,sumNum,scratch);
			DrawString(scratch);
		}		
		
		BoxLimits(blSubject,&left,&right,tocH);
		cr.left = h0+left+1; cr.right = h0+right-2;
		cr.right = MIN(cr.right,win->contR.right);
		if (cr.right-cr.left+2>=FontWidth)
		{
			ClipRect(&cr);
			MoveTo(h0+left+FontWidth/2,v);
			if(!(sum.flags & FLAG_UTF8) || !HasUnicode() || DrawUTF8Text(&sum.subj[1], sum.subj[0], cr.right - cr.left, normFonts)) {
				DrawString(sum.subj);
			}
		}
		
		ClipRect(&win->contR);
		
		if (sum.selected) InvertSum(tocH,sumNum,False);
		TextFace(normal);
		if (UseListColors) SetThemeTextColor(kThemeTextColorListView,depth,True); else RGBForeColor(&textColor);
	}
	
	/*
	 * cleanup
	 */
	TextFace(normal);
	InfiniteClip(winWPPort);
}



/**********************************************************************
 * Item2Status - turn a menu item into a status value
 **********************************************************************/
short Item2Status(short item)
{
	switch (item)
	{
		case statmUnread: return(UNREAD);
		case statmRead: return(READ);
		case statmReplied: return(REPLIED);
		case statmForwarded: return(FORWARDED);
		case statmRedirected: return(REDIST);
		case statmUnsendable: return(UNSENDABLE);
		case statmSendable: return(SENDABLE);
		case statmQueued: return(QUEUED);
		case statmTimed: return(TIMED);
		case statmUnsent: return(UNSENT);
		case statmSent: return(SENT);
		case statmMesgError: return(MESG_ERR);
		case statmRecovered: return(REBUILT);
		default: return(0);
	}
}

/**********************************************************************
 * Status2Item - turn a status value into a menu item
 **********************************************************************/
short Status2Item(short item)
{
	switch (item)
	{
		case UNREAD: return statmUnread;
		case READ: return statmRead;
		case REPLIED: return statmReplied; 
		case FORWARDED: return statmForwarded; 
		case REDIST: return statmRedirected;
		case UNSENDABLE: return statmUnsendable; 
		case SENDABLE: return statmSendable; 
		case QUEUED: return statmQueued; 
		case TIMED: return statmTimed; 
		case UNSENT: return statmUnsent; 
		case SENT: return statmSent; 
		case MESG_ERR: return statmMesgError;
		case REBUILT: return statmRecovered;
		default: return(0);
	}
}


/************************************************************************
 * SelectBoxRange - make a particular range in a mailbox the current selection.
 ************************************************************************/
void SelectBoxRange(TOCHandle tocH,int start,int end,Boolean cmd,int eStart,int eEnd)
{
	MyWindowPtr win = (*tocH)->win;
	/*
	 * topVis and botVis are overly generous, since it won't hurt to be so
	 */
	int topVis = GetControlValue(win->vBar);
	int botVis = (*tocH)->count-(GetControlMaximum(win->vBar)-GetControlValue(win->vBar));
	int sNum;
	int r1, r2;
	
	if (!(*tocH)->count) return;
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr((*tocH)->win));
	
	if (GetPrefLong(PREF_MANUAL_BOX_SELECTION)==0)
		TOCSetDirty(tocH,true);	// now that we're saving the selection...

	ClipRect(&win->contR);
	
	if (end<start) {r1=start;start=end;end=r1;} 	/* swap */
	if (eEnd<eStart) {r1=eStart;eStart=eEnd;eEnd=r1;} 		/* swap */
	
	r1 = MIN(start,(*tocH)->count);
	r2 = MIN(end,(*tocH)->count-1);
	r1 = MAX(r1,0);
	r2 = MAX(r2,0);
	win->hasSelection = False;
	
	/*
	 * first range; from the beginning to just before the new selection range
	 */
	if (cmd)
	{
		for (sNum=0;sNum<r1;sNum++)
			if ((*tocH)->sums[sNum].selected)
			{
				win->hasSelection = True;
				break;
			}
	}
	else
	{
		for (sNum=0;sNum<r1;sNum++)
			if ((*tocH)->sums[sNum].selected)
			{
				(*tocH)->userActive = false;
				(*tocH)->sums[sNum].selected = False;
				if (topVis <= sNum && sNum <= botVis)
					InvertSum(tocH,sNum,False);
			}
	}
	
	/*
	 * the new selection range
	 */
	if (cmd)
	{
		for (sNum=r1;sNum<=r2;sNum++)
			if (sNum<eStart || sNum > eEnd)
			{
				(*tocH)->userActive = false;
				(*tocH)->sums[sNum].selected = !(*tocH)->sums[sNum].selected;
				if (topVis <= sNum && sNum <= botVis)
					InvertSum(tocH,sNum,False);
				win->hasSelection = win->hasSelection || (*tocH)->sums[sNum].selected;
			}
	}
	else
	{
		for (sNum=r1;sNum<=r2;sNum++)
		{
			if (!(*tocH)->sums[sNum].selected)
			{
				(*tocH)->userActive = false;
				(*tocH)->sums[sNum].selected = True;
				if (topVis <= sNum && sNum <= botVis)
					InvertSum(tocH,sNum,False);
			}
		}
		win->hasSelection = r1<=r2;
	}
	
	/*
	 * above the new selection range
	 */
	if (cmd)
	{
		if (!win->hasSelection)
			for (sNum=r2+1;sNum<(*tocH)->count;sNum++)
				if ((*tocH)->sums[sNum].selected)
				{
					win->hasSelection = True;
					break;
				}
	}
	else
	{
		for (sNum=r2+1;sNum<(*tocH)->count;sNum++)
			if ((*tocH)->sums[sNum].selected)
			{
				(*tocH)->userActive = false;
				(*tocH)->sums[sNum].selected = False;
				if (topVis <= sNum && sNum <= botVis)
					InvertSum(tocH,sNum,False);
			}
	}
	
	InfiniteClip(GetMyWindowCGrafPtr(win));
	PopGWorld();
	
	(*tocH)->updateBoxSizes = true;
	(*tocH)->conConMultiScan = true;
}

/**********************************************************************
 * BoxSetSummarySelected - make sure a summary is selected or not
 **********************************************************************/
void BoxSetSummarySelected(TOCHandle tocH,short sumNum,Boolean selected)
{
	if ((*tocH)->sums[sumNum].selected != selected)
	{
		(*tocH)->sums[sumNum].selected = selected;
		InvalSum(tocH,sumNum);
		(*tocH)->updateBoxSizes = true;
		(*tocH)->conConMultiScan = true;
	}
}

/**********************************************************************
 * BoxActivate - perform extra mailbox processing for activate/deactivate
 **********************************************************************/
void BoxActivate(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	
	SetPort_(GetMyWindowCGrafPtr(win));
	if (!win->pte && !(*tocH)->searchFocus) BoxListFocus(tocH,win->isActive);
	ShowBoxSizes(win);
	
	if ((*tocH)->drawerWin)
		// Keep drawer in sync
		MBDrawerActivate((*tocH)->drawerWin,win->isActive);
}

/**********************************************************************
 * BoxListFocus - focus on the list
 **********************************************************************/
void BoxListFocus(TOCHandle tocH,Boolean focus)
{
	int topVis;
	int botVis;
	int sNum;
	MyWindowPtr win = (*tocH)->win;
	CGrafPtr	winPort = GetMyWindowCGrafPtr(win);
	
	SetPort_(winPort);
	if (focus!=(*tocH)->listFocus)
	{
		(*tocH)->listFocus = focus;
		if (focus) (*tocH)->searchFocus = false;
		if (FirstMsgSelected(tocH)!=-1)
		{
			ClipRect(&win->contR);
			topVis = GetControlValue(win->vBar);
			botVis = (*tocH)->count -
										(GetControlMaximum(win->vBar)-GetControlValue(win->vBar));
			if (botVis==(*tocH)->count) botVis--;
			for (sNum=topVis; sNum<=botVis; sNum++)
				if ((*tocH)->sums[sNum].selected) InvertSum(tocH,sNum,True);
			InfiniteClip(winPort);
		}
	}
}

/**********************************************************************
 * InvertSum - invert a summary, doing it correctly (more or less)
 **********************************************************************/
void InvertSum(TOCHandle tocH,short sumNum,Boolean forActivate)
{
	Rect r, r2;
	Boolean above, below;	// are sums above and below selected?
	MyWindowPtr win = (*tocH)->win;
	
	r.top = win->topMargin+(sumNum-GetControlValue(win->vBar))*win->vPitch;
	r.bottom = r.top + win->vPitch;
	r.left = 0;
	r.right = win->contR.right;

	//adjust top of rectangle, which otherwise overlaps top line
	r.top++;
	
	RGBBackColor(&win->backColor);
	if (!forActivate && (*tocH)->listFocus && !InBG)
	{
		UseListColors ? InvertSumByColumns(tocH,&r) : HiInvertRect(&r);
	}
	else
	{
		// are sums above or below selected?
		//above = sumNum && (*tocH)->sums[sumNum-1].selected;
		//below = sumNum<(*tocH)->count-1 && (*tocH)->sums[sumNum+1].selected;
		above = below = False;
		
		if (above) r.top--;	//if selected above, overlap with separator line
		if (below) r.bottom++; //if selected below, overlap with separator line
		if (forActivate)
		{
			InsetRect(&r,1,1);	//if activation, don't do rim, just inside
			UseListColors ? InvertSumByColumns(tocH,&r) : HiInvertRect(&r);
		}
		else
		{
			// top
			r2 = r;
			r2.bottom = r.top+1;
			UseListColors ? InvertSumByColumns(tocH,&r2) : HiInvertRect(&r2);
			// bottom
			r2 = r;
			r2.top = r.bottom-1;
			UseListColors ? InvertSumByColumns(tocH,&r2) : HiInvertRect(&r2);
			// left
			r2 = r;
			r2.top++; r2.bottom--;	//take care of overlap with top, bottom
			r2.right = r2.left+1;
			UseListColors ? InvertSumByColumns(tocH,&r2) : HiInvertRect(&r2);
			// right
			r2 = r;
			r2.top++; r2.bottom--;	//take care of overlap with top, bottom
			r2.left = r2.right-1;
			UseListColors ? InvertSumByColumns(tocH,&r2) : HiInvertRect(&r2);
		}
	}
}

/**********************************************************************
 * InvertSumByColums - invert a summary, paying attention to column backgrounds
 **********************************************************************/
void InvertSumByColumns(TOCHandle tocH,Rect *r)
{
	Rect r2;
	short line;
	short h = -GetControlValue((*tocH)->win->hBar)*(*tocH)->win->hPitch;
	
	for (line=1;line<BoxLinesLimit;line++)
	{
		r2 = *r;
		if (BoxLimits(line,&r2.left,&r2.right,tocH))
		{
			OffsetRect(&r2,h,0);
			if (SectRect(r,&r2,&r2))
			{
	 			SetThemeBackground(MBGetSort(tocH,line) ? kThemeBrushListViewSortColumnBackground:kThemeBrushListViewBackground,RectDepth(&r2),true);
				HiInvertRect(&r2);
			}
		}
	}
	SET_COLORS;
}

/**********************************************************************
 * BoxMenu - handle a menu choice for a mailbox
 **********************************************************************/
Boolean BoxMenu(MyWindowPtr win,int menu,int item,short modifiers)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short state;
	short tableId;
	uLong when;
	MSumPtr sum;
	Boolean swap = False;
	TextAddrHandle addr=nil;
	short sumNum;
#ifdef TWO
	FilterPB fpb;
#endif	

	if (menu==MESSAGE_MENU && item==MESSAGE_QUEUE_ITEM && modifiers&optionKey)
	{
		menu = CHANGE_HIER_MENU;
	  item = CHANGE_QUEUEING_ITEM;
	  swap = True;
	}

	switch (menu)
	{
		case FILE_MENU:
			switch(item)
			{
				case FILE_OPENSEL_ITEM:
					if (win->hasSelection)
						BoxOpen(win);
					else
						return(False);
					break;
				case FILE_SAVE_AS_ITEM:
					DoSaveSelectedAs(tocH);
					return(True);
					break;
				case FILE_BROWSE_ITEM:
					DoIterativeThingy(tocH,menu,modifiers,(void*)item);
					return(true);
					break;
				case FILE_PRINT_ONE_ITEM:
				case FILE_PRINT_ITEM:
					if (win->hasSelection)
					{
						long beginSel, endSel;
						PETEHandle pte = nil;
						
						if ((*tocH)->previewID && (*tocH)->previewPTE && PeteLen((*tocH)->previewPTE))
						{
							if ((modifiers&shiftKey) && AttIsSelected(win,win->pte,-1,-1,attOpen+attPrint,nil,nil))
								break;// done
							else
								PeteGetTextAndSelection((*tocH)->previewPTE,nil,&beginSel,&endSel);
							pte = (*tocH)->previewPTE;
						}
						else beginSel = endSel = 0;
#ifdef	GX_PRINTING
						if (gGXIsPresent)
							GXPrintSelectedMessages(tocH,beginSel!=endSel && (modifiers&shiftKey),item==FILE_PRINT_ONE_ITEM,beginSel,endSel);
						else
#endif	//GX_PRINTING
						(void) PrintSelectedMessages(tocH,beginSel!=endSel && (modifiers&shiftKey),item==FILE_PRINT_ONE_ITEM,beginSel,endSel,pte);
					}
					else
						return(False);
					break;
				default:
					return(False);
			}
			break;
		
		case EDIT_MENU:
#ifdef SPEECH_ENABLED
			if (win->pte && item != EDIT_SPEAK_ITEM) return(false);	// preview focus!
#else
			if (win->pte) return(false);	// preview focus!
#endif
			switch(item)
			{
				case EDIT_CLEAR_ITEM:
					if ((*tocH)->previewID && PreviewReadDelete && (sumNum=LastMsgSelected(tocH))>=0)
						BeenThereDoneThat(tocH,sumNum); 
					DoIterativeThingy(tocH,MESSAGE_DELETE_ITEM,modifiers,0);
					break;
				case EDIT_COPY_ITEM:
					CopySelectedSummaries(tocH);
					break;
				case EDIT_SELECT_ITEM:
					if ((modifiers&optionKey) && -1<(sumNum=LastMsgSelected(tocH)))
						BoxSelectSame(tocH,(*tocH)->lastSort==SORT_SUBJECT_ITEM?SORT_SUBJECT_ITEM:SORT_SENDER_ITEM,sumNum);
					else
						SelectBoxRange(tocH,0,(*tocH)->count-1,False,0,0);
					break;
#ifdef SPEECH_ENABLED
				case EDIT_SPEAK_ITEM:
					(void) SpeakSelectedMessages (tocH);
					break;
#endif
				default:
					return(False);
					break;
			}
			break;
			
		case SORT_HIER_MENU:
			SetMyCursor(watchCursor);
			if (item==SORT_GROUP_ITEM)
				BoxToggleLaurence(tocH);
			else if (item>SORT_BAR2_ITEM)
				BoxCustomSort(tocH,item-SORT_BAR2_ITEM);
			else
			{
				MBSortHit(tocH,BoxItem2Line(item),(modifiers&optionKey)!=0,(modifiers&shiftKey)!=0);
			}
			break;
			
		case MESSAGE_MENU:
			switch(item)
			{
				case MESSAGE_DELETE_ITEM:
					if ((*tocH)->previewID && PreviewReadDelete && (sumNum=LastMsgSelected(tocH))>=0)
						BeenThereDoneThat(tocH,sumNum); 
					NoSaves = True;
				case MESSAGE_FORWARD_ITEM:
				case MESSAGE_REDISTRIBUTE_ITEM:
				case MESSAGE_SALVAGE_ITEM:
					DoIterativeThingy(tocH,item,modifiers,0);
					NoSaves = False;
					break;
				case MESSAGE_QUEUE_ITEM:
					QueueSelectedMessages(tocH,QUEUED,0);
					if (!Offline && PrefIsSet(PREF_AUTO_SEND))
						XferMail(False,True,False,False,True,0);
					break;
				case MESSAGE_JUNK_ITEM:
				case MESSAGE_NOTJUNK_ITEM:
					Junk(tocH,-1,item==MESSAGE_JUNK_ITEM,false);
					break;
				case MESSAGE_REPLY_ITEM:
					BoxReply(tocH,menu,item,modifiers);
					NoSaves = false;
				default:
					return(False);
			}
			break;
		
		case CHANGE_HIER_MENU:
			switch(item)
			{
				case CHANGE_QUEUEING_ITEM:
					when = 0;
					for (sum=(*tocH)->sums;sum<(*tocH)->sums+(*tocH)->count;sum++)
						if (sum->selected)
						{
							if (sum->state==TIMED) when = sum->seconds;
							state = sum->state;
							break;
						}
					if (ModifyQueue(&state,&when,swap))
					{
					  Boolean now = state==SENT;
						if (now) state = QUEUED;
					  QueueSelectedMessages(tocH,state,when);
						if (now) XferMail(False,True,False,False,True,0);
					}
					break;
				default: return(False);
			}
			break;

		case TABLE_HIER_MENU:
			if (Menu2TableId(tocH,GetMHandle(TABLE_HIER_MENU),item,&tableId))
				DoIterativeThingy(tocH,menu,modifiers,(void*)tableId);
			break;
			
		case STATE_HIER_MENU:
			if (item == statmQueued) QueueSelectedMessages(tocH,QUEUED,0);
			else 
			{
				DoIterativeThingy(tocH,menu,modifiers,(void*)item);
				SetSendQueue();
			}
			break;
			
		case PERS_HIER_MENU:
			if (HasFeature (featureMultiplePersonalities)) {
				UseFeature (featureMultiplePersonalities);
				DoIterativeThingy(tocH,menu,modifiers,(void*)item);
			}
			break;
		case LABEL_HIER_MENU:
		case SERVER_HIER_MENU:
			DoIterativeThingy(tocH,menu,modifiers,(void*)item);
			break;

		case SPECIAL_MENU:
			switch(AdjustSpecialMenuSelection(item))
			{
				case SPECIAL_MAKE_NICK_ITEM:
					if (win->pte && modifiers&shiftKey) MakeNickFromSelection(win);
					else if ((*tocH)->which==OUT) MakeCboxNick(win);
					else MakeMboxNick(win,modifiers);
					break;
				case SPECIAL_MAKEFILTER_ITEM:
					DoMakeFilter(win);
					break;
				case SPECIAL_FILTER_ITEM:
					FilterSelectedMessages(flkManual,tocH,&fpb);
					FilterPostprocess(flkManual,&fpb);
					break;
				default:
					return(False);
			}
			break;
		case REPLY_WITH_HIER_MENU:
			if (HasFeature (featureStationery)) {
				UseFeature (featureStationery);
				DoIterativeThingy(tocH,MESSAGE_REPLY_ITEM,modifiers,(void*)item);
			}
			break;
		case FORWARD_TO_HIER_MENU:
			DoIterativeThingy(tocH,MESSAGE_FORWARD_ITEM,modifiers,addr=MenuItem2Handle(menu,item));
			break;
		case REDIST_TO_HIER_MENU:
			DoIterativeThingy(tocH,MESSAGE_REDISTRIBUTE_ITEM,modifiers,addr=MenuItem2Handle(menu,item));
			break;
		case PRIOR_HIER_MENU:
			DoIterativeThingy(tocH,menu,modifiers,(void*)item);
			break;
		case WINDOW_MENU:
			{
				Str255 s;
				if (!(win->pte && *PeteSelectedString(s,win->pte)))
				if (item==WIN_PH_ITEM && (modifiers&shiftKey) && 0<=(sumNum=FirstMsgSelected(tocH)))
				{
					OpenPh(PCopy(s,(*tocH)->sums[sumNum].from));
					break;
				}
			}
			return(False);
			break;

		case TLATE_SEL_HIER_MENU:
			if ((*tocH)->previewID && (*tocH)->previewPTE)
			{
				HeadSpec hs;
				UHandle text;
				
				PeteGetTextAndSelection((*tocH)->previewPTE,&text,&hs.value,&hs.stop);
				if (hs.value==hs.stop)
				{
					hs.value = BodyOffset(text);
					hs.stop = PeteLen((*tocH)->previewPTE);
				}
				hs.start = hs.value;
				ETLTransSelection((*tocH)->previewPTE,&hs,item);
			}
			else
				return(false);
			break;

		default:
			return(TransferMenuChoice(menu,item,tocH,-1,modifiers,False));
			break;
	}
	ZapHandle(addr);
	return(True);
}

/************************************************************************
 * BoxReply - reply to a selection in a mailbox
 ************************************************************************/
OSErr BoxReply(TOCHandle tocH,short menu,short item,short modifiers)
{
	short count = CountSelectedMessages(tocH);

	if (PrefIsSet(PREF_MULTI_REPLY) || count <= 1)
	{
		DoIterativeThingy(tocH,item,modifiers,nil);
		return noErr;
	}
	else
	{
		if (count > GetRLong(MULT_RESPOND_WARNING_THRESH))
			if (!MultiMessageOpOK(MULT_RESPOND_WARNING,count))
				return userCanceledErr;
		return DoReplyMultiple(tocH,modifiers)?noErr:userCanceledErr;
	}
}

/************************************************************************
 * MultiMessageOpOK - is it ok to do something to a lot of messages?
 ************************************************************************/
Boolean MultiMessageOpOK(short fmt,short count)
{
	return kAlertStdAlertCancelButton!=ComposeStdAlert(Caution,fmt,count);
}
	
void BoxToggleLaurence(TOCHandle tocH)
{
	Str255 s;
	ControlHandle cntl;
	
	GetRString(s,ColumnHeadStrn+blSubject);
	if ((*tocH)->laurence)
		(*tocH)->laurence = false;
	else
	{
		PCatC(s,'�');
		(*tocH)->laurence = true;
	}
	(*tocH)->resort = kResortNow;
	if (cntl=FindControlByRefCon((*tocH)->win,'wide'+blSubject))
		SetControlTitle(cntl,s);
}

/************************************************************************
 * ApplySortString - apply a custom sort
 ************************************************************************/
void ApplySortString(TOCHandle tocH,PStr s)
{
	Boolean group;
	Byte sorts[12];
	short i;
	ControlHandle cntl;
	
	if (!InterpretSortString(s,&group,sorts,nil))
	{
		if (group!=(*tocH)->laurence) BoxToggleLaurence(tocH);
		for (i=0;i<12;i++)
			if (cntl=FindControlByRefCon((*tocH)->win,'wide'+i+1))
				SetControlValue(cntl,(sorts[i]!=0) ? 1:0);
		BMD(sorts,(*tocH)->sorts,sizeof((*tocH)->sorts));
		(*tocH)->resort = kResortNow;
	}
}

/************************************************************************
 * BoxCustomSort - apply a custom sort
 ************************************************************************/
void BoxCustomSort(TOCHandle tocH,short which)
{
	Str255 s;
	GetRString(s,SortsStrn+which);
	ApplySortString(tocH,s);
}

/**********************************************************************
 * MenuItem2Handle - convert a menu item to a handle
 **********************************************************************/
TextAddrHandle MenuItem2Handle(short menu, short item)
{
	Str255 itemStr;
	
	MyGetItem(GetMHandle(menu),item,itemStr);
	return((TextAddrHandle)PStr2Handle(itemStr));
}

/**********************************************************************
 * CheckSortItems - check the appropriate items in the sort menu
 **********************************************************************/
void CheckSortItems(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	MenuHandle mh = GetMHandle(SORT_HIER_MENU);
	short item;
	short sortNum;
	
	for (item=1;item<=SORT_SUBJECT_ITEM;item++)
	{
		sortNum = BoxItem2Line(item);
		SetItemMark(mh,item,MBGetSort(tocH,sortNum) ? checkMark : noMark);
	}
	SetItemMark(mh,SORT_GROUP_ITEM,(*tocH)->laurence ? checkMark:noMark);
	EnableIf(mh,SORT_MOOD_ITEM,AnalDoIncoming());
	EnableIf(mh,SORT_MAILBOX_ITEM,(*tocH)->virtualTOC);
}

/************************************************************************
 * InterpretSortString - interpret a string that represents a complex sort
 ************************************************************************/
OSErr InterpretSortString(PStr s,Boolean *group,Byte *sorts,PStr menuItem)
{
	UPtr spot;
	Str63 token;
	short i;
	OSErr err = fnfErr;
	long num;
	
	spot = s+1;
	
	if (PToken(s,token,&spot," "))
	{
		if (group) *group = 0!=atoi(token+1);
		for (i=0;i<12;i++)
			if (PToken(s,token,&spot," "))
				if (sorts)
				{
					StringToNum(token,&num);
					sorts[i] = num;
				}
		if (PToken(s,token,&spot,"\377"))
		{
			err = noErr;
			if (menuItem) PCopy(menuItem,token);
		}
	}
	return(err);
}

/************************************************************************
 * Sort2String - represent a mailbox sort as a string
 ************************************************************************/
PStr Sort2String(PStr s,TOCHandle tocH)
{
	short i;
	Str15 num;
	
	*s = 0;
	PCatC(s,((*tocH)->laurence)!=0?'1':'0');
	PCatC(s,' ');
	for (i=0;i<12;i++)
	{
		NumToString((*tocH)->sorts[i],num);
		PCat(s,num);
		PCatC(s,' ');
	}
	PCatR(s,UNTITLED);
	return s;
}

/**********************************************************************
 * BoxFind - find in the window
 **********************************************************************/
Boolean BoxFind(MyWindowPtr win,PStr what)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	short	sumNum,start;
	Boolean	wrapped = false;
	MSumType *sum;
	DEC_STATE(tocH);
	
	if (win->pte && win->pte==(*tocH)->previewPTE)
		//	search in preview pane
		return FindInPTE(win,win->pte,what);
	
	if ((*tocH)->count)	//	don't search if window is empty
	{
		//	search summaries
		//	find last non-selected and start from there
		for (start=(*tocH)->count-1;start>=0;start--)
			if ((*tocH)->sums[start].selected) break;
		start++;
		
		for(sumNum = start;sumNum != start || !wrapped;sumNum++)
		{
			if (sumNum>=(*tocH)->count)
			{
				//	start from the top again
				sumNum = 0;
				wrapped = true;
				if (!start)
					break;	//	this is where we started
			}
			sum=(*tocH)->sums+sumNum;
			
			L_STATE(tocH);
			if (FindStrStr(what,sum->from)>=0 || FindStrStr(what,sum->subj)>=0)
			{
				U_STATE(tocH);
				//	found
				SelectBoxRange(tocH,sumNum,sumNum,false,-1,-1);
				BoxCenterSelection((*tocH)->win);
				if (!IsWindowVisible(winWP))
					ShowMyWindow(winWP);
				UserSelectWindow(winWP);
				UpdateMyWindow(winWP);
				SFWTC=True;
				return true;
			}
			U_STATE(tocH);
		}
		
	}
	return false;
}

/**********************************************************************
 * BoxClose - close a mailbox window, saving the toc if it's dirty
 **********************************************************************/
Boolean BoxClose(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCType	**tocH = (TOCType **) GetWindowPrivateData (winWP);
	short sumNum;
	Boolean justHide = False;
	Boolean superClose = PrefIsSet(PREF_SUPERCLOSE) && IsWindowVisible (winWP);
	Boolean isOut = (*tocH)->which == OUT;
	FSSpec spec;
	FileViewHandle	fvh = GetFileViewInfo(win);
	
	if (IsWindowVisible(GetMyWindowWindowPtr((*tocH)->win))  || TOCIsDirty(tocH)) AnyTOCDirty++;
	
	// if this is an IMAP mailbox, we'll need to do some extra stuff to close it.
	if ((*tocH)->imapTOC != 0) 
	{
		IMAPAbortResync(tocH);
		justHide = IsIMAPMailboxBusy(tocH) || IMAPAutoExpungeMailbox(tocH);	// simply hide this mailbox if it's busy for some reason.
	}
	
	if (fvh && (*tocH)->fileView)
		(*tocH)->previewHi = (*fvh)->savePreviewHi;
	
	if (!GrowZoning)
	{
		/*
		 * should we compact?
		 */
		if (NeedAutoCompact(tocH))
		{
			spec = GetMailboxSpec(tocH,-1);
			StackPush(&spec,CompactStack);
		}
		
		
		/*
		 * first, close all associated message windows
		 */
		for (sumNum=(*tocH)->count-1;sumNum>=0;sumNum--)
		{
			if ((*tocH)->sums[sumNum].messH)
			{
				if (superClose && (!isOut ||!(*(*tocH)->sums[sumNum].messH)->win->isDirty))
				{
					if (!CloseMyWindow(GetMyWindowWindowPtr((*(*tocH)->sums[sumNum].messH)->win)))
						return(False);
				}
				else
					justHide = True;
			}
			if (!justHide && (*tocH)->sums[sumNum].cache) ZapHandle((*tocH)->sums[sumNum].cache);
		}
		
		/*
		 * dispose of transport error message data
		 */
		for (sumNum=(*tocH)->count-1;sumNum>=0;sumNum--)
			ZapHandle((*tocH)->sums[sumNum].mesgErrH);

		/*
		 * write if we're dirty
		 */
		if (TOCIsDirty(tocH))	WriteTOC(tocH);
		
		/*
		 * Save the IMAP mailbox information
		 */
		 if ((*tocH)->imapTOC) UpdateIMAPMailboxInfo(tocH);
	}
	
	BoxFClose(tocH,true);

	if (justHide)
	{
		HideWindow_(winWP);
		return(False);
	}

	if ((*tocH)->drawerWin)
	{
		// Close drawer
		CloseMyWindow(GetMyWindowWindowPtr((*tocH)->drawerWin));
		(*tocH)->drawerWin = NULL;
	}

	if (fvh)
	{
		//	save changes?
		if ((*fvh)->dirtyPrefs || (*fvh)->expandList.fExpandListChanged)
		{
			short oldResFile = CurResFile(),refN = 0;
			
			if ((*tocH)->refN) UseResFile((*tocH)->refN);
			else
			{
				FSSpec	resSpec = GetMailboxSpec(tocH,0);
				refN = FSpOpenResFile(&resSpec,fsRdWrPerm);
				if (refN == -1) refN = 0;
			}
			if ((*tocH)->refN || refN)
			{
				Handle	hFVPrefs;
				Handle	hExpandList;
				
				if ((*fvh)->expandList.fExpandListChanged)
				{
					hExpandList = Get1Resource(kExandListResType,kExandListResID);
					if (!hExpandList)
					{
						hExpandList = NuHandle(0);
						AddResource_(hExpandList,kExandListResType,kExandListResID,"");
					}
					if (hExpandList)
					{
						SetHandleSize(hExpandList,0);
						HandAndHand((Handle)(*fvh)->expandList.hExpandList,hExpandList);
						ChangedResource(hExpandList);
					}
				}
				if ((*fvh)->dirtyPrefs)
				{
					hFVPrefs = Get1Resource(kFVPrefsResType,kFVPrefsResID);
					if (!hFVPrefs)
					{
						hFVPrefs = NuHandle(sizeof(FVPrefs));
						(*(FVPrefsHandle)hFVPrefs)->version = kFVPrefsVersion;
						AddResource_(hFVPrefs,kFVPrefsResType,kFVPrefsResID,"");
					}
					if (hFVPrefs)
					{
						BMD(&(*fvh)->prefs,*hFVPrefs,sizeof(FVPrefs));
						ChangedResource(hFVPrefs);
					}
				}
				if (refN) CloseResFile(refN);
			}
			UseResFile(oldResFile);		
		}
		
		//	close file viewer
		FVDispose(fvh);
	}
	
	/*
	 * now, unlink ourselves from the list
	 */
	LL_Remove(TOCList,tocH,(TOCType **));
	
	/*
	 * bye, bye love
	 */
	ZapHandle(tocH);
	
	/*
	 * turn off some stuff
	 */
	ZeroWinFuncs(win);
	
	return(True); 
}

/**********************************************************************
 * BoxOpen - open some messages from within a mailbox
 **********************************************************************/
void BoxOpen(MyWindowPtr win)
{
	//	Carbon version can't preallocate windows and doesn't need to
	MyWindowPtr lastWin=nil, w;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	int sum,limit;
	uLong count = CountSelectedMessages(tocH);

	if (count>GetRLong(WIN_GEN_WARNING_THRESH) && !MultiMessageOpOK(WIN_GEN_WARNING,count))
		return;

	// Walk through the selected entries, activating windows as needed
	for (limit=(*tocH)->count,sum=0; sum<limit; sum++)
	{
		if ((*tocH)->sums[sum].selected)
			if ((*tocH)->sums[sum].messH)
			{
				MyWindowPtr tocMessWin = (*(MessHandle)(*tocH)->sums[sum].messH)->win;
				WindowPtr	tocMessWinWP = GetMyWindowWindowPtr (tocMessWin);
				if (!IsWindowVisible (tocMessWinWP))
					ShowMyWindow(tocMessWinWP);
#ifdef	FLOAT_WIN
				UserSelectWindow(tocMessWinWP);
#else	//FLOAT_WIN
				SelectWindow_(tocMessWinWP);
#endif	//FLOAT_WIN
				lastWin = tocMessWin;
			}
	}
	
	for (sum=(*tocH)->count-1;sum>=0; sum--)
	{
		if ((*tocH)->sums[sum].selected && !(*tocH)->sums[sum].messH)
		{
			short		realSum;
			TOCHandle	realTOC;
			MiniEvents(); if (CommandPeriod) break;
			if (!(realTOC = GetRealTOC(tocH,sum,&realSum))) break;
			if (!(w=GetAMessage(realTOC,realSum,nil,nil,True))) break;
			if (tocH != realTOC)
			{
				BeenThereDoneThat(tocH,sum);	//	set status to READ in virtual mailbox
				(*Win2MessH(w))->openedFromTocH = tocH;	//	opened from virtual mailbox
				(*Win2MessH(w))->openedFromSerialNum = (*tocH)->sums[sum].serialNum;
			}		
			if (lastWin)
			{
				WindowPtr	lastWinWP = GetMyWindowWindowPtr(lastWin);
				ActivateMyWindow(lastWinWP,False);
				NotUsingWindow(lastWinWP);
			}
			lastWin = w;
			MonitorGrow(True);
		}
	}
}

/************************************************************************
 * BoxKey - handle keystrokes in a mailbox window
 ************************************************************************/
Boolean BoxKey(MyWindowPtr win, EventRecord *event)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short c = event->message & charCodeMask;
	Boolean result = true;
	
	if (c==tabKey && ((*tocH)->previewID || (*tocH)->drawer))
	{
		if (win->pte)
		{
			// Preview -> drawer or box list
			PeteFocus(win,nil,true);
			if ((*tocH)->drawer)
			{
				// Have drawer
				if ((*tocH)->drawerWin) MBDrawerSetFocus((*tocH)->drawerWin, true);
			}
			else
				// Do box list
				BoxListFocus(tocH,true);
		}
		else if ((*tocH)->listFocus)
		{
			// Box list -> preview or drawer
			BoxListFocus(tocH,false);
			if ((*tocH)->previewPTE)
			{
				// Have preview
				PeteFocus(win,(*tocH)->previewPTE,true);
				if (PreviewReadFocus) BeenThereDoneThat(tocH,-1);
			}
			else
			{
				// Do drawer
				if ((*tocH)->drawerWin) MBDrawerSetFocus((*tocH)->drawerWin, true);
			}
		}
		else
		{
			// Drawer -> box list
			if ((*tocH)->drawerWin) MBDrawerSetFocus((*tocH)->drawerWin, false);
			BoxListFocus(tocH,true);
		}
	}
	else if (win->pte) result = BoxPreviewKey(win,event);	// Key goes to preview
	else if ((*tocH)->drawer && (*tocH)->drawerWin && !(*tocH)->listFocus)
	{
		// Key goes to drawer
		(*tocH)->userActive = true;
		// Don't use tocH after this point. May no longer be valid
		return ((*tocH)->drawerWin->key)((*tocH)->drawerWin,event);
	}
	else result = BoxListKey(win,event);	// Key goes to box list
	(*tocH)->userActive = true;
	return(result);
}

/************************************************************************
 * BoxListKey - keystroke to list portion of mailbox
 ************************************************************************/
Boolean BoxListKey(MyWindowPtr win,EventRecord *event)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short c = event->message & charCodeMask;
	Boolean cmd = (event->modifiers & (cmdKey|shiftKey)) != 0;
	int mNum = -1;
	long uLetter = UnadornMessage(event)&charCodeMask;
	Boolean shift = 0!=(event->modifiers & shiftKey);

	SFWTC = True;
	
	if (leftArrowChar <= uLetter && uLetter <= downArrowChar &&
			win->hasSelection &&
			IsArrowSwitch(event->modifiers))
		c = enterChar;
		
	switch (c)
	{
		case leftArrowChar:
		case upArrowChar:
			for (mNum=0;mNum<(*tocH)->count;mNum++)
			 if ((*tocH)->sums[mNum].selected) break;
			mNum--;
			if (mNum<0) mNum = 0;
			break;
		case rightArrowChar:
		case downArrowChar:
			for (mNum=(*tocH)->count-1;mNum>=0;mNum--)
				if ((*tocH)->sums[mNum].selected) break;
			mNum++;
			if (mNum==(*tocH)->count) mNum--;
			break;
		case delChar:
		case deleteKey:
			if (event->what!=autoKey && (!PrefIsSet(PREF_NO_DELKEY) || 0!=(event->modifiers&cmdKey))) BoxMenu(win,EDIT_MENU,EDIT_CLEAR_ITEM,event->modifiers);
			else return(false);
			break;
		case ' ':
			BoxIdle(win);
			if (win->hasSelection && (*tocH)->previewID)
			{
				if (PeteScroll((*tocH)->previewPTE,0,shift ? psePageUp : psePageDn) && !shift)
				{
					mNum = LastMsgSelected(tocH);
					if (PreviewReadNext) BeenThereDoneThat(tocH,-1);
					EzOpen(tocH,mNum,(*tocH)->ezOpenSerialNum,event->modifiers,true,false);
					mNum = -1;
				}
				else if (PreviewReadFocus) BeenThereDoneThat(tocH,-1);
				break;
			}
			// fall through is deliberate if no preview
		case returnChar:
		case enterChar:
			if (win->hasSelection)
			{
				BoxOpen(win);
				break;
			}
			/* fall through is deliberate for no selection */
		default:
			if (event->modifiers & cmdKey)
			{
				return(False);
			}
			else if (*Type2SelString)
				BoxType2Select(tocH,Type2SelString);
			break;
	}
	if (mNum > -1)
	{
		SelectBoxRange(tocH,mNum,mNum,cmd,-1,-1);
		BoxCenter((*tocH)->win,mNum);
	}
	return(True);
}

/************************************************************************
 * BoxPreviewKey - keystroke to preview portion of mailbox
 ************************************************************************/
Boolean BoxPreviewKey(MyWindowPtr win,EventRecord *event)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short c = event->message & charCodeMask;
	Boolean cmd = (event->modifiers & (cmdKey|shiftKey)) != 0;
	long uLetter = UnadornMessage(event)&charCodeMask;
	short mNum;
	Boolean shift = 0 != (event->modifiers & shiftKey);

	SFWTC = True;
	
	if (cmd) return(false);
	
	if ((*tocH)->previewID && (mNum=FirstMsgSelected(tocH))!=-1 && PreviewReadNext)
		BeenThereDoneThat(tocH,-1);
	
	if (c==' ')
	{
		if (PeteScroll((*tocH)->previewPTE,0,shift ? psePageUp : psePageDn) && !shift)
		{
			mNum = FirstMsgSelected(tocH);
			if (PreviewReadNext) BeenThereDoneThat(tocH,mNum);
			EzOpen(tocH,mNum,(*tocH)->ezOpenSerialNum,event->modifiers,true,false);
		}
	}
	else if (c==returnKey || c==enterKey)
		return(BoxListKey(win,event));
	else if (DirtyKey(event->message))
		AlertStr(READ_ONLY_ALRT, Stop, nil);
	else
	{
		PeteEdit(win,win->pte,peeEvent,(void*)event);
	}
	(*tocH)->userActive = true;
	return(True);
}

/************************************************************************
 * BoxCenter - center a mailbox around a given line
 ************************************************************************/
void BoxType2Select(TOCHandle tocH,PStr string)
{
	UPtr spot;
	short difference;
	short i;
	short subFound = -1;
	short fromFound = -1;
	short thisDiff;
	short found;
	
	/* subject */
	difference = REAL_BIG;
	for (i=(*tocH)->count;i--;)
		if (spot=PPtrFindSub(Type2SelString,(*tocH)->sums[i].subj+1,*(*tocH)->sums[i].subj))
		{
			thisDiff = spot-(*tocH)->sums[i].subj;
			if (spot>(*tocH)->sums[i].subj && IsWordChar[spot[-1]]) thisDiff += 100;
			if (difference>thisDiff)
			{
				subFound = i;
				difference = thisDiff;
			}
		}

	/* sender */
	difference = REAL_BIG;
	for (i=(*tocH)->count;i--;)
		if (spot=PPtrFindSub(Type2SelString,(*tocH)->sums[i].from+1,*(*tocH)->sums[i].from))
		{
			thisDiff = spot-(*tocH)->sums[i].from;
			if (spot>(*tocH)->sums[i].from && IsWordChar[spot[-1]]) thisDiff += 100;
			if (difference>thisDiff)
			{
				fromFound = i;
				difference = thisDiff;
			}
		}

	UL(tocH);

	// if the string is found in only one column, use that column
	if (subFound < 0) found = fromFound;
	else if (fromFound < 0) found = subFound;
	// if it's found in both, use from unless the mailbox is sorted by subject
	else if ((*tocH)->lastSort==SORT_SUBJECT_ITEM) found = subFound;
	else found = fromFound;
	
	if (found>=0)
	{
		SelectBoxRange(tocH,found,found,False,-1,-1);
		BoxCenterSelection((*tocH)->win);
	}
}

/************************************************************************
 * BoxCenter - center a mailbox around a given line
 ************************************************************************/
void BoxCenter(MyWindowPtr win, short mNum)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	int topVis = GetControlValue(win->vBar);
	int botVis = topVis + RectHi(win->contR)/win->vPitch-1;
	
	if (mNum<topVis || mNum>botVis)
	{
		UpdateMyWindow(GetMyWindowWindowPtr(win));	/* this is a hack.  ScrollIt should be smarter */
		ScrollIt(win,0,(topVis+botVis)/2-mNum);
	}
}

/************************************************************************
 * BoxSelectAfter - select the message on or after a given message,
 * if no messages are already selected
 ************************************************************************/
void BoxSelectAfter(MyWindowPtr win, short mNum)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	
	if (mNum >=0 && BoxNextSelected(tocH,-1)<0)
	if (win->hasSelection = ((*tocH)->count>0))
	{
		mNum = MIN(mNum,(*tocH)->count-1);
		(*tocH)->sums[mNum].selected = True;
		BoxCenter((*tocH)->win,mNum);
	}
}

/************************************************************************
 * BoxCenterSelection - center a selection in a mailbox window
 ************************************************************************/
void BoxCenterSelection(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	int top, bottom;
	
	for (top=0;top<(*tocH)->count;top++)
		if ((*tocH)->sums[top].selected) break;
	for (bottom=(*tocH)->count-1;bottom>=0;bottom--)
		if ((*tocH)->sums[bottom].selected) break;
	if (top<=bottom) BoxCenter(win,(top+bottom)/2);
}

/************************************************************************
 *
 ************************************************************************/
Boolean BoxPosition(Boolean save,MyWindowPtr win)
{
	Rect r;
	Boolean zoomed;
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	Str31 name;
	FSSpec	spec = GetMailboxSpec(tocH,-1);

	PCopy(name,spec.name);
	
	if (save)
	{
		utl_SaveWindowPos(winWP,&r,&zoomed);
		SavePosFork(spec.vRefNum,spec.parID,name,&r,zoomed);
	}
	else
	{
		if (!RestorePosFork(spec.vRefNum,spec.parID,name,&r,&zoomed))
			{UL(tocH); return(False);}
		utl_RestoreWindowPos(winWP,&r,zoomed,1,TitleBarHeight(winWP),LeftRimWidth(winWP),(void*)FigureZoom,(void*)DefPosition);
	}
	return(True);
}

/************************************************************************
 * SaveMessageAs - save a message in text only form
 ************************************************************************/
void DoSaveSelectedAs(TOCHandle tocH)
{
	long creator;
	short refN;
	short err;
	Str31 scratch;
	int sumNum;
	MessHandle messH;
	short lastSelected = -1;
	Str255 title;
	FSSpec spec;
	MyWindowPtr win;
	short count = CountSelectedMessages(tocH);
	ModalFilterYDUPP filter=nil;
	
	/*
	 * tickle stdfile
	 */
	GetPref(scratch,PREF_CREATOR);
	if (*scratch!=4) GetRString(scratch,TEXT_CREATOR);
	BMD(scratch+1,&creator,4);
	sumNum = FirstMsgSelected(tocH);
	MakeMessFileName(tocH,sumNum,spec.name);
	Stationery = (HasFeature(featureStationery)&&(*tocH)->which==OUT&&LastMsgSelected(tocH)==sumNum) ? False : 2;	/* 2 is our signal to the filter to hide the item */
	if (err=SFPutOpen(&spec,creator,'TEXT',&refN,nil,filter,SAVEAS_DLOG,nil,nil,nil))
		return;
	
	OpenProgress();
	ProgressR(NoBar,count,MOVING_MESSAGES,LEFT_TO_TRANSFER,nil);
	
	for (;!err && sumNum<(*tocH)->count;sumNum++)
		if ((*tocH)->sums[sumNum].selected)
		{
			// if this is an IMAP message, make sure it's been downloaded before we try to save it
			if ((*tocH)->imapTOC) EnsureMsgDownloaded(tocH, sumNum, false);

			MakeMessTitle(title,tocH,sumNum,True);
			Progress(NoBar,count--,nil,nil,title);
			messH = (*tocH)->sums[sumNum].messH;
			if (win=GetAMessage(tocH,sumNum,nil,nil,False))
			{
				WindowPtr	winWP = GetMyWindowWindowPtr(win);
				err = SaveAsToOpenFile(refN,Win2MessH(win));
				if (!IsWindowVisible (winWP)) CloseMyWindow(winWP);
			}
		}

	CloseProgress();

	/*
	 * close
	 */
	(void) MyFSClose(refN);
	
	/*
	 * report error
	 */
	if (err)
	{
		FileSystemError(COULDNT_SAVEAS,spec.name,err);
		FSpDelete(&spec);
	}
}

/************************************************************************
 * MakeMessFileName - make a default name for save as
 ************************************************************************/
void MakeMessFileName(TOCHandle tocH,short sumNum, UPtr name)
{
	UPtr spot;
	short len = MIN(MAX_BOX_NAME,*(*tocH)->sums[sumNum].subj);
	
	BMD((*tocH)->sums[sumNum].subj,name,len+1);
	*name = len;
	for (spot=name+1;spot<=name+len;spot++)
		if (*spot==':') *spot = '-';
}

/************************************************************************
 * ShowBoxSizes - title a mailbox window
 ************************************************************************/
void ShowBoxSizes(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	Str31 string;
	long usedBytes,totalBytes;
	short depth=1;
	ControlHandle cntl = FindControlByRefCon(win,kBoxSizeRefCon);
	
	PushGWorld();
	SetPort_(GetMyWindowCGrafPtr(win));
	if (cntl && IsControlVisible(cntl))
	{
		usedBytes = (*tocH)->usedK K;
		totalBytes = (*tocH)->totalK K;
		if ((*tocH)->virtualTOC)
			if (PrefIsSet(PREF_SHOW_NUM_SELECTED))
				ComposeRString(string,BOX_SIZE_SELECT_SRCH_FMT,BoxCountSelected(tocH),(*tocH)->count);
			else
				NumToString((*tocH)->count,string);
		else
			if (PrefIsSet(PREF_SHOW_NUM_SELECTED))
				ComposeRString(string,BOX_SIZE_SELECT_FMT,BoxCountSelected(tocH),(*tocH)->count,usedBytes,
									 totalBytes-usedBytes);
			else
				ComposeRString(string,BOX_SIZE_FMT,(*tocH)->count,usedBytes,
									 totalBytes-usedBytes);
		
		MySetControlTitle(cntl,string);
	}
	PopGWorld();
	(*tocH)->updateBoxSizes = false;
}

/************************************************************************
 * BoxDidResize - handle the size display area
 ************************************************************************/
void BoxDidResize(MyWindowPtr win, Rect *oldContR)
{
	ControlHandle hBar = win->hBar;
	ControlHandle vBar = win->vBar;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	ControlHandle cntl, placard;
	short previewHeight;
	Boolean again = false;
	Rect r;
	short goalTopMargin = kHeaderButtonHeight + (ETLHasMBoxContextFolder(win) ? kTabHeight+INSET+kTabGap : 0);
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Boolean	fileView = (*tocH)->hasFileView && (*tocH)->fileView;

	if (cntl=FindControlByRefCon(win,'plug'+1))
		goalTopMargin += INSET + ControlHi(cntl);

	RedoTOC(tocH);
	
	if (win->topMargin!=goalTopMargin && !(*tocH)->virtualTOC)
	{
		SetTopMargin(win,goalTopMargin);
		InvalContent(win);
		again = true;
	}
	
	// preview height?
	previewHeight = BoxPreviewHeight(tocH);
	if (win->botMargin != previewHeight)
	{
		SetBotMargin(win,previewHeight);
		InvalContent(win);
		again = true;
		BoxMakePreview(win);
	}
	
	if (again)
	{
		MyWindowDidResize(win,oldContR);	/* reposition window elements */
		return;
	}
	
	win->dontGreyOnMe = win->contR;	// not much grey for us...
	
	if ((*tocH)->previewPTE)
	{
		Rect		rPort;
		
		r = win->contR;
		r.top = r.bottom + GROW_SIZE+1;
		r.right += GROW_SIZE;
		rPort = *GetPortBounds(winPort,&rPort);
		r.bottom = rPort.bottom;
		PeteDidResize((*tocH)->previewPTE,&r);
	}
	
	if (cntl=FindControlByRefCon(win,PREVIEW_TOGGLE_CNTL))
		MySetCtlValue(cntl,win->botMargin!=0);
		
	if (hBar)
	{
		Rect r = *GetControlBounds(hBar,&r);
		short delta = GROW_SIZE + BoxSizeWidth(tocH);
		short conConPrevProfWidth = (HasFeature(featureConCon) && (*tocH)->previewPTE) ? GetRLong(CONCON_PREV_PROF_WIDTH) : 0;
		
		r.left += delta + conConPrevProfWidth;
		MySetCntlRect(hBar,&r);
		if (cntl=FindControlByRefCon(win,kBoxSizeRefCon))
		{
			BoxSizeBox(win,&r,tocH);
			InsetRect(&r,-1,-1);
			OffsetRect(&r,GROW_SIZE,0);
			MySetCntlRect(cntl,&r);
			if (fileView) HideControl(cntl); else ShowControl(cntl);
			if (cntl=FindControlByRefCon(win,PREVIEW_TOGGLE_CNTL))
			{
				r.right = r.left;
				r.left = -1;
				if (!GetSuperControl(cntl,&placard))
					MySetCntlRect(placard,&r);
				InsetRect(&r,MAX_APPEAR_RIM-1,MAX_APPEAR_RIM-1);
				//OffsetRect(&r,-1,-1);
				MySetCntlRect(cntl,&r);
			}
		}

		// Size the con con preview profile control
		if (cntl=FindControlByRefCon(win,kConConProfRefCon))
			if (!fileView && conConPrevProfWidth)
			{
				BoxSizeBox(win,&r,tocH);
				OffsetRect(&r,GROW_SIZE,0);
				InsetRect(&r,-1,-1);
				r.left = r.right;
				r.right = r.left + conConPrevProfWidth-1;
				MySetCntlRect(cntl,&r);
				ShowMyControl(cntl);
			}
			else HideControl(cntl);

		if (cntl=FindControlByRefCon(win,PREVIEW_DIVIDE_CNTL))
		{
			r.top = win->contR.bottom;
			r.bottom = r.top + GROW_SIZE + 1;
			r.left = win->contR.right;
			r.right = r.left + GROW_SIZE + 1;
			if (!GetSuperControl(cntl,&placard))
				MySetCntlRect(placard,&r);
			InsetRect(&r,4,4);
			MySetCntlRect(cntl,&r);
		}
		SetRect(&r,win->contR.left,oldContR?oldContR->bottom:win->contR.bottom,win->contR.left+delta,
							 (oldContR?oldContR->bottom:win->contR.bottom)+GROW_SIZE);
		InvalWindowRect(GetMyWindowWindowPtr(win),&r);
	}
	
	BoxPlaceBevelButtons(win);

	if (cntl=FindControlByRefCon(win,'tabs'))
	{
		Rect	rWin;
		FileViewHandle	fvh = GetFileViewInfo(win);
		
		GetPortBounds(GetMyWindowCGrafPtr(win),&rWin);
		SetRect(&r,-1,win->topMargin-kTabHeight-kHeaderButtonHeight+kTabGap,rWin.right+1,win->topMargin-kHeaderButtonHeight);
		SetControlBounds(cntl,&r);

		if (fvh)
		{
			short	i;
			GetPortBounds(winPort,&r);
			r.top = win->topMargin;
			if (!(*tocH)->fileView)
				OffsetRect(&r,-4000,-4000);	// hide the list
			LVSize((*fvh)->list,&r,nil);
			FVSizeHeaderButtons(win,fvh);
			for(i=0;i<kFDColCount;i++)
				SetControlVisibility((*fvh)->controls[i],fileView,false);		
		}
	}

	if ((*tocH)->drawerWin)
	{
		// resize drawer
		MyWindowDidResize((*tocH)->drawerWin,NULL);
	}
}

/************************************************************************
 * BoxPreviewHeight - find the preview height for a mailbox
 ************************************************************************/
short BoxPreviewHeight(TOCHandle tocH)
{
	MyWindowPtr win = (*tocH)->win;
	CGrafPtr	winPort = GetMyWindowCGrafPtr (win);
	Rect	rPort = *GetPortBounds(winPort,&rPort);
	short totalHi = RectHi(rPort) - GROW_SIZE - win->topMargin;
	short hi = totalHi;
	short min = GetRLong(PREVIEW_USELESS)*FontLead;
	short max = totalHi - GetRLong(PREVIEW_USELESS)*win->vPitch;
	
	hi = BoxPreviewHeightWish(tocH);
		
	// fine-tune
	//hi -= (totalHi-hi)%win->vPitch;	// supposed to make sure we have integral height for summaries
																		// but causes window size to grow on close/open cycle.
	if (hi/FontLead < GetRLong(PREVIEW_USELESS)) hi = 0;	// too small to be useful?
	if (max<min) hi = 0;	// remainder too small to be useful?
	else hi = MIN(max,hi);
	
	return(hi);
}

/************************************************************************
 * BoxPreviewHeightWish - find the height we'd like the preview to be
 ************************************************************************/
short BoxPreviewHeightWish(TOCHandle tocH)
{
	short hi;
	
	if ((*tocH)->previewHi < 0) return(0);	// negative preview means user collapsed it
	else if ((*tocH)->previewHi > 1) hi = (*tocH)->previewHi;  // positive value > 1 means use it
	else if ((*tocH)->previewHi == 1) hi = FontLead*GetRLong(PREVIEW_HI);  // 1 means default size
	else if (PrefIsSet(PREF_NO_PREVIEW)) return(0); // no percent, default off
	else hi = FontLead*GetRLong(PREVIEW_HI);	// no percent, default on--use standard percent
	
	return(hi);
}

/************************************************************************
 * BoxMakePreview - make the preview pane for a mailbox
 ************************************************************************/
void BoxMakePreview(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	ControlHandle cntl, placard;
	Rect r;
	PETEHandle pte;
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	
	if (win->botMargin && !(*tocH)->previewPTE)
	{
		PETEDocInitInfo pdi;
		
		// first make the edit region
		DefaultPII(win,false,peVScroll|peGrowBox,&pdi);
		pdi.docWidth = RectWi(win->contR)-PETE_SCROLLY_DIFFERENCE;
		pdi.inParaInfo.paraFlags = 0;
		PeteCreate(win,&pte,0,&pdi);
		(*PeteExtra(pte))->emoDesired = true;
		(*tocH)->previewPTE = pte;
		CleanPII(&pdi);
		Zero(r);
		if (cntl=GetNewControl(PREVIEW_DIVIDE_CNTL,winWP))
		{
			if (placard = GetNewControl(PLACARD_CNTL,winWP))
				EmbedControl(cntl,placard);
 		}
	}
	else if (!win->botMargin && (*tocH)->previewPTE)
	{
		if (cntl=FindControlByRefCon(win,PREVIEW_DIVIDE_CNTL))
		{
			if (!GetSuperControl(cntl,&placard))
				DisposeControl(placard);	//	Disposes embedded cntl also
			else
				DisposeControl(cntl);
		}
		BoxListFocus(tocH,true);
		PeteDispose(win,(*tocH)->previewPTE);
		(*tocH)->previewPTE = nil;
	}
}

/************************************************************************
 * BoxSetPreview - set the preview state for a mailbox
 ************************************************************************/
void BoxSetPreview(MyWindowPtr win,Boolean openIt)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	short hi = (*tocH)->previewHi;
	Rect r = *GetPortBounds(GetWindowPort(winWP),&r);
	short totalHi = RectHi(r);
	
	if (!openIt)
	{
		hi = (!(MainEvent.modifiers&optionKey)&&hi) ? -ABS(hi) : -1;
		(*tocH)->previewHi = hi;
		hi = win->contR.bottom+GROW_SIZE;
	}
	else
	{
		if (!(MainEvent.modifiers&optionKey)&&hi) hi = ABS(hi);
		else hi = 1;
		(*tocH)->previewHi = hi;
		if (hi==1) hi = FontLead*GetRLong(PREVIEW_HI);
		hi += totalHi;
		{
			ControlHandle cntl;
			if (cntl=FindControlByRefCon(win,kConConProfRefCon))
				ShowMyControl(cntl);
		}
	}
	r.bottom = r.top + hi;
	SanitizeSize(win,&r);
	SizeWindow(winWP,RectWi(r),RectHi(r),false);
	MyWindowDidResize(win,nil);
	BoxRefreshPreview(tocH);
	TOCSetDirty(tocH,true);
}

/************************************************************************
 * BoxRefreshPreview - make the preview go again
 ************************************************************************/
void BoxRefreshPreview(TOCHandle tocH)
{
	(*tocH)->previewID = 0;
	(*tocH)->lastSameTicks = 0;
	(*tocH)->conConMultiScan = true;
}

/************************************************************************
 * BoxRequestFocus - somebody wants to change the focus
 ************************************************************************/
Boolean BoxRequestFocus(MyWindowPtr win, PETEHandle pte)
{
	TOCHandle tocH = Win2TOC(win);
	
	ASSERT(tocH);
	if (!tocH) return false;
	
	if (!(*tocH)->previewID) return false;
	
	BoxListFocus(tocH,false);	// not on the list of summaries
	PeteFocus(win,pte,true);	// on the petehandle!
	return true;
}

/************************************************************************
 * BoxCurAddr - get the address of the first selected message
 ************************************************************************/
PStr BoxCurAddr(MyWindowPtr win, PStr addr)
{
	TOCHandle tocH = Win2TOC(win);
	short sumNum = FirstMsgSelected(tocH);
	
	if (sumNum < 0) return nil;
	
	{
		MessHandle messH = (*tocH)->sums[sumNum].messH;
		Boolean opened = false;
		
		if (messH==nil)
		{
			TOCHandle realTocH;
			short realSumNum;
			
			if (realTocH = GetRealTOC(tocH,sumNum,&realSumNum))
			{
				EnsureMsgDownloaded(realTocH,realSumNum,false);	// FSOIMAP
				GetAMessage(realTocH,realSumNum,nil,nil,false);
				messH = (*realTocH)->sums[realSumNum].messH;
				opened = messH!=nil;
				if (!opened) return nil;
			}
		}
		
		addr = ((*messH)->win)->curAddr((*messH)->win,addr);
		
		if (opened) CloseMyWindow((*messH)->win);
		
		return addr;
	}
}

/************************************************************************
 * BoxIdle - idle routine for mailboxes
 ************************************************************************/
void BoxIdle(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH;
	short mNum, startNum;
	static long waitTicks;
	static long previewTicks = 5;
	static long previewMultiTicks = 30;
		
	ASSERT(winWP);
	if (!winWP) return;
	
	tocH = (TOCHandle)GetWindowPrivateData(winWP);
	ASSERT(tocH);
	if (!tocH) return;

	if (!(*tocH)->previewPTE)
		(*tocH)->previewID = 0;
	else if (IsWindowVisible (winWP) && TickCount()-(*tocH)->mouseTicks>previewTicks)
	{
		previewTicks = GetRLong(PREVIEW_SINGLE_DELAY);
		previewMultiTicks = GetRLong(PREVIEW_MULTI_DELAY);
		startNum = -1;
		for (mNum=(*tocH)->count-1;mNum>=0;mNum--)
		{
			if ((*tocH)->sums[mNum].selected)
				if (startNum!=-1) {startNum = -2 ; break;}	// multiple selection same as no selection?
				else startNum = mNum;
		}
		if (startNum!=-2 || TickCount()-(*tocH)->mouseTicks>previewMultiTicks)
		{
			(*tocH)->mouseTicks = TickCount();
			Preview(tocH,startNum);
		}
	}
	
	// Leave imap mailboxes alone that are in the middle of a resync operation.
	if (!((*tocH)->imapTOC && FindNodeByToc(tocH))) 
	{
		if ((*tocH)->updateBoxSizes)
			ShowBoxSizes(win);
	
		// Moodwatch one message while we're here
		if (IsWindowVisible(winWP) && DiskSpunUp() && AnalDoExisting() && !(*tocH)->analScanned
				&& GlobalIdleTicks > 120)
		{
			Boolean didOne = false;
			// visible on down
			for (mNum=GetControlValue((*tocH)->win->vBar);mNum<(*tocH)->count;mNum++)
				if (!(*tocH)->sums[mNum].score)
				{
					didOne = true;
					OneAnalMess(tocH,mNum);
					break;
				}
			// invisible on up
			if (!didOne)
			{
				for (mNum=GetControlValue((*tocH)->win->vBar)-1;mNum>=0;mNum--)
					if (!(*tocH)->sums[mNum].score)
					{
						didOne = true;
						OneAnalMess(tocH,mNum);
						break;
					}
			}
			(*tocH)->analScanned = !didOne;
		}
	}		

	if ((*tocH)->imapTOC)
		UpdateIMAPMailbox(tocH);	// see if we need to update IMAP mailbox
	UpdatePOPMailboxes();	// attend to any pending POP message deletions
	
	{
		ControlHandle cntl;
		if ((cntl=FindControlByRefCon(win,kConConProfRefCon)) && IsControlVisible(cntl))
			HiliteControl(cntl, (*tocH)->previewID ? 0 : 255);
	}
}

/************************************************************************
 * BoxDrawerUseless - how much space does the drawer add to each side of the window?
 ************************************************************************/
Boolean BoxDrawerUseless(MyWindowPtr win, short *left, short *right)
{
	TOCHandle tocH = Win2TOC(win);
	
	if ((*tocH)->drawerWin && (*tocH)->drawer)
	{
		WindowPtr winWP = GetMyWindowWindowPtr(win);
		WindowPtr drawWP = GetMyWindowWindowPtr((*tocH)->drawerWin);
		if (winWP && drawWP)
		{
			Rect rDrawStruc;
			Rect rWinStruc;
			short myLeft, myRight;
			
			GetWindowBounds(winWP,kWindowStructureRgn,&rWinStruc);
			GetWindowBounds(drawWP,kWindowStructureRgn,&rDrawStruc);
			
			myLeft = rDrawStruc.right < rWinStruc.right ? RectWi(rDrawStruc) : 0;
			myRight = rDrawStruc.left > rWinStruc.left ? RectWi(rDrawStruc) : 0;
			if (left) *left = myLeft;
			if (right) *right = myRight;
			return true;
		}
	}
	
	// no drawer or couldn't get window pointers
	if (left) *left = 0;
	if (right) *right = 0;
	return false;
}

/************************************************************************
 * BoxCountSelected - how many messages are selected
 ************************************************************************/
short BoxCountSelected(TOCHandle tocH)
{
	short count = 0;
	short mNum;
	
	for (mNum=(*tocH)->count;mNum--;) if ((*tocH)->sums[mNum].selected) count++;
	
	return count;
}

/************************************************************************
 * BoxGrowSize - adjust grow size
 ************************************************************************/
void BoxGrowSize(MyWindowPtr win,Point *newSize)
{
	short	nonMsgSumHt = win->topMargin + GROW_SIZE;
	short	msgSumHt = ((newSize->v-nonMsgSumHt+win->vPitch/2)/win->vPitch)*win->vPitch;
	
	//	Calculate new window height
	newSize->v = msgSumHt + nonMsgSumHt;
}

/************************************************************************
 * BoxSizeBox - return rect for window's size display
 ************************************************************************/
void BoxSizeBox(MyWindowPtr win,Rect *r,TOCHandle tocH)
{
	*r = win->contR;
	r->top = r->bottom;
	r->bottom = r->top+GROW_SIZE;
	r->right = r->left + BoxSizeWidth(tocH);
	InsetRect(r,1,1);
}

/************************************************************************
 * BoxSizeWidth - return width of window's size display
 ************************************************************************/
short BoxSizeWidth(TOCHandle tocH)
{
	if (tocH && (*tocH)->virtualTOC)
	{
		static short	vBoxWidth;
		
		if (!vBoxWidth)
		{
			vBoxWidth = GetRLong(SEARCH_HITS_SIZE)*CharWidthInFont('0',SmallSysFontID(),SmallSysFontSize());
			if (PrefIsSet(PREF_SHOW_NUM_SELECTED)) vBoxWidth += GetRLong(BOX_SIZE_SELECT_EXTRA);
		}
		return vBoxWidth;
	}
	return PrefIsSet(PREF_SHOW_NUM_SELECTED) ? GetRLong(BOX_SIZE_SIZE) + GetRLong(BOX_SIZE_SELECT_EXTRA) : GetRLong(BOX_SIZE_SIZE);
}

/************************************************************************
 * BoxGonnaShow - get ready to show a mailbox window
 ************************************************************************/
OSErr BoxGonnaShow(MyWindowPtr win)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	ControlHandle cntl, placard;
	Str255 title;
	
	PushGWorld();
	SetPort_(GetWindowPort(winWP));
	
	(*tocH)->needRedo = 0;
	win->update = BoxUpdate;
	win->scroll = BoxScroll;
	win->drag = BoxDragHandler;
	win->button = BoxButton;
	win->idle = BoxIdle;
	win->selection = BoxHasSelection;
	win->zoomSize = BoxZoomSize;
	win->drawerUseless = BoxDrawerUseless;
	win->requestPeteFocus = BoxRequestFocus;
	win->curAddr = BoxCurAddr;
	win->dontControl = true;
	(*tocH)->listFocus = true;
	(*tocH)->conConMultiScan = true;
	MyWindowDidResize(win,nil);
	
	AnyTOCDirty++;
	
	// Handle AMO (goddam it)
	GetWTitle(winWP,title);
	if (GetPrefLong(PREF_AMO_AVOIDANCE)==kAMOAvoidInvisible && title[2]==0 && *title>2)
	{
		--*title;
		BMD(title+3,title+2,*title-1);
		SetWTitle_(winWP,title);
	}
	
	if ((*tocH)->resort) MBResort(tocH);
	BoxInitialSelection(tocH);
			
	ScrollIt(win,0,SortedDescending(tocH)?REAL_BIG:-REAL_BIG);
	if (!FindControlByRefCon(win,PREVIEW_TOGGLE_CNTL))
	{
		if (cntl = GetNewControl(PREVIEW_TOGGLE_CNTL,winWP))
			if (placard = GetNewControl(PLACARD_CNTL,winWP))
				EmbedControl(cntl,placard);
	}

	if (ETLHasMBoxContextFolder(win))
	{
		//	Set up file view
		ControlRef	ctlTabs;
		Rect		rTab;
		GrafPtr	oldPort;
		FileViewHandle	hData;

		GetPort(&oldPort);
		SetPort_(GetMyWindowCGrafPtr(win));
		
		rTab = win->contR;	//	doesn't really matter what size, will get resized later anyway
		ctlTabs = FindControlByRefCon(win,'tabs');	// Do we already have a tab control?
		if (!ctlTabs)
			// Don't already have tab control. Create one.
			ctlTabs = NewControl(winWP,&rTab,"\p",true,FILE_VIEW_TABS,0,0,kControlTabSmallProc,0);
		if (ctlTabs)
		{
			LetsGetSmall(ctlTabs);
			EmbedControl(ctlTabs,win->topMarginCntl);
			SetControlReference(ctlTabs,'tabs');
			if ((*tocH)->fileView) SetControlValue(ctlTabs,2);
			if (hData = NuHandleClear(sizeof(FileViewInfo)))
			{	
				short oldResFile = CurResFile(),refN = 0;
				short		vRefNum;
				long		dirID;
				
				if (ETLMBoxContextFolder(win, &vRefNum, &dirID) == EMSR_OK)
				{
					(*hData)->vRefNum = vRefNum;
					(*hData)->dirId = dirID;
				}
				else
				{
					(*hData)->vRefNum = -1;
					(*hData)->dirId = 2;
				}
				(*tocH)->hFileView = (Handle)hData;
				(*tocH)->hasFileView = true;
				
				//	Get saved preferences
				if ((*tocH)->refN) UseResFile((*tocH)->refN);
				else
				{
					FSSpec	resSpec = GetMailboxSpec(tocH,0);
					refN = FSpOpenResFile(&resSpec,fsRdPerm);
					if (refN == -1) refN = 0;
				}
				if ((*tocH)->refN || refN)
				{
					FVPrefsHandle	hFVPrefs;
					
					if ((*hData)->expandList.hExpandList = (ExpandListHandle)Get1Resource(kExandListResType,kExandListResID))
						DetachResource((Handle)(*hData)->expandList.hExpandList);
					if (hFVPrefs = (FVPrefsHandle)Get1Resource(kFVPrefsResType,kFVPrefsResID))
					{
						(*hData)->prefs = **hFVPrefs;
						ReleaseResource((Handle)hFVPrefs);
					}
					if (refN) CloseResFile(refN);
				}
				UseResFile(oldResFile);				
				
				NewFileView(win,hData);
				(*hData)->savePreviewHi = (*tocH)->previewHi;
				HitTab(win,ctlTabs,tocH);	// set up initial window configuration
			}
		}

		SetPort_(oldPort);
	}
	else (*tocH)->hasFileView = false;

	BoxAddBevelButtons(win);

	PopGWorld();
	return(noErr);
}

/************************************************************************
 * BoxInitialSelection - make an initial selection in a mailbox
 ************************************************************************/
void BoxInitialSelection(TOCHandle tocH)
{
	short fumlub;
	
	switch (GetPrefLong(PREF_MANUAL_BOX_SELECTION))
	{
		case 2:
			if (SortedDescending(tocH)) SelectBoxRange(tocH,0,0,False,0,0);
			else SelectBoxRange(tocH,(*tocH)->count-1,(*tocH)->count-1,False,0,0);
			break;
		case 1:
			fumlub = FumLub(tocH);
			SelectBoxRange(tocH,fumlub,fumlub,False,0,0);
			break;
		default:
			break;	// leave well enough alone...
	}
}


/************************************************************************
 * BoxZoomSize - How big do we grow a mailbox?
 ************************************************************************/
void BoxZoomSize(MyWindowPtr win,Rect *zoom)
{
	Rect listZoom = *zoom;
	short zoomHi = RectHi(*zoom);
	short hi;
	
	// compute list box size, assuming no bottom margin
	hi = win->botMargin;
	win->botMargin = 0;
	MaxSizeZoom(win,&listZoom);
	win->botMargin = hi;

	hi = RectHi(listZoom) + BoxPreviewHeightWish(Win2TOC(win));
	hi = MIN(hi,zoomHi); hi = MAX(hi,win->minSize.v);
	zoom->right = zoom->left+RectWi(listZoom);
	zoom->bottom = zoom->top+hi;
}

/************************************************************************
 * BoxHasSelection - is there a selection?
 ************************************************************************/
Boolean BoxHasSelection(MyWindowPtr win)
{
	return (-1!=LastMsgSelected((TOCHandle)GetMyWindowPrivateData(win)));
}

/************************************************************************
 * BoxAddBevelButtons - add the bevel buttons
 ************************************************************************/
void BoxAddBevelButtons(MyWindowPtr win)
{
	ControlHandle cntl;
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	short i;
	ControlButtonGraphicAlignment align;
	ControlButtonTextPlacement placement;
	Boolean iconMania = !PrefIsSet(PREF_NO_SUMMARY_ICONS);
	
	for (i=1;i<BoxLinesLimit;i++)
		if (!FindControlByRefCon(win,'wide'+i))
		{
			if (cntl = GetNewControlSmall(BOX_HEAD_CNTL,winWP))
			{
				SetControlReference(cntl,'wide'+i);
				// if sticky sorting on this column, turn on the column header
				if (MBGetSort(tocH,i)) SetControlValue(cntl,1);
				// if K, right-justify the header
				if (i==blSize)
				{
					align = kControlBevelButtonAlignRight;
					placement = kControlBevelButtonPlaceToLeftOfGraphic;
					SetBevelTextAlign(cntl,teFlushRight);
				}
				else
				{
					SetBevelTextAlign(cntl,teFlushDefault);
					align = kControlBevelButtonAlignSysDirection;
					placement = kControlBevelButtonPlaceSysDirection;
				}
				//SetBevelTextOffset(cntl,2);
 				if (iconMania && i!=blSubject)
 				{
					SetBevelIcon(cntl,i==blMailbox ? MAILBOX_ONLY_ICON : COLUMN_ICON_BASE+i-1,nil,nil,nil);
 					SetControlData(cntl,0,kControlBevelButtonTextPlaceTag,sizeof(placement),(void*)&placement);
 					SetControlData(cntl,0,kControlBevelButtonGraphicAlignTag,sizeof(align),(void*)&align);
				}
				EmbedControl(cntl,win->topMarginCntl);
			}
		}
	
	if (MBDrawerSupported(win))
	{
		// Put in drawer switch
		if (!FindControlByRefCon(win,kDrawerSwitch))
		if (cntl = GetNewControlSmall(BOX_HEAD_CNTL,winWP))
		{
			SetControlReference(cntl,kDrawerSwitch);
			SetBevelIcon(cntl,DRAWER_ICON,nil,nil,nil);
			EmbedControl(cntl,win->topMarginCntl);
		}
	}
		
	// and the size box
	if (!(cntl=FindControlByRefCon(win,kBoxSizeRefCon)))
		if (cntl=GetNewControlSmall(BOX_SIZE_CNTL,winWP))
		{
			SetControlReference(cntl,kBoxSizeRefCon);
		}
		
	// and the content concentrator preview control
	if (HasFeature(featureConCon) && !(cntl=FindControlByRefCon(win,kConConProfRefCon)))
		if (cntl=GetNewControlSmall(CONCON_PROF_PREVIEW_POP,winWP))
		{
			SetControlReference(cntl,kConConProfRefCon);
		}
	
	// buttons for workgroup
	if (ETLBoxTagWidth(win))
		ETLAddBoxButtons(tocH);
}

/************************************************************************
 * BoxPlaceBevelButtons - place the bevel buttons
 ************************************************************************/
void BoxPlaceBevelButtons(MyWindowPtr win)
{
	ControlHandle cntl,cntlDrawer;
	Rect r,rDrawerCntl;
	short i;
	short h0 = -GetControlValue(win->hBar)*win->hPitch;
	Str255 s;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	Boolean	fileView = (*tocH)->hasFileView && (*tocH)->fileView;
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Boolean	winVis = IsWindowVisible(winWP);
	
	r = *GetControlBounds(win->topMarginCntl,&r);
	r.left = 0;
	r.top = r.bottom - (kHeaderButtonHeight);

   // First check drawer switch
	if (cntlDrawer = FindControlByRefCon(win,kDrawerSwitch))
	{  
	   rDrawerCntl = r;
		rDrawerCntl.right = WindowWi(winWP);
		rDrawerCntl.left = rDrawerCntl.right - 20;
	}
		
	for (i=1;i<BoxLinesLimit;i++)
		if (cntl = FindControlByRefCon(win,'wide'+i))
		{
			if (BoxLimits(i,&r.left,&r.right,tocH) && !fileView)
			{
				OffsetRect(&r,h0,0);
				if (cntlDrawer && r.right >= rDrawerCntl.left)
				   // Don't overlap drawer control
				   r.right = rDrawerCntl.left;
				MySetCntlRect(cntl,&r);
				if (RectWi(r)<32)
				{
					MySetControlTitle(cntl,"");
					SetBevelGraphicAlign(cntl,kControlBevelButtonAlignCenter);
				}
				else
				{
					GetRString(s,ColumnHeadStrn+i);
					// display group subjects by adding a bullet to the subject label
					if (i==blSubject && MBIsSticky(tocH) && ((*tocH)->laurence)!=0)
						PCatC(s,bulletChar);
					MySetControlTitle(cntl,s);
					SetBevelGraphicAlign(cntl,kControlBevelButtonAlignSysDirection);
					SetBevelGraphicOffset(cntl,2);
					SetBevelTextOffset(cntl,2);
				}
				SetControlVisibility(cntl,true,winVis);
			}
			else
				SetControlVisibility(cntl,false,winVis);
		}

	if (cntlDrawer)
		MySetCntlRect(cntlDrawer,&rDrawerCntl);

	// Layout the buttons
	if (ETLBoxTagWidth(win))
	{
		h0 = INSET;
		for (i=1;cntl = FindControlByRefCon(win,'plug'+i);i++)
		{
			MoveMyCntl(cntl,h0,INSET,0,0);
			h0 += ControlWi(cntl) + INSET;
		}
	}	
}

/************************************************************************
 * BoxSetBevelButtonValues - set bevel button values by sort values
 ************************************************************************/
void BoxSetBevelButtonValues(MyWindowPtr win)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	short i;
	ControlHandle cntl;
	
	for (i=1;i<BoxLinesLimit;i++)
		if (cntl=FindControlByRefCon(win,'wide'+i))
			SetControlValue(cntl,MBGetSort(tocH,i) ? 1 : 0);
}

/************************************************************************
 * BoxButton - handle a hit in the box buttons
 ************************************************************************/
void BoxButton(MyWindowPtr win,ControlHandle buttonHandle,long modifiers,short part)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	uLong contrlRfCon = GetControlReference(buttonHandle);
	uLong i = contrlRfCon - 'wide';
	uLong iPrime = contrlRfCon - 'plug';
	
	// audit clicks on only these controls ...
	if ((i<BoxLinesLimit) || (contrlRfCon==kBoxSizeRefCon) || (contrlRfCon==PREVIEW_TOGGLE_CNTL))
		AuditHit((modifiers&shiftKey)!=0, (modifiers&controlKey)!=0, (modifiers&optionKey)!=0, (modifiers&cmdKey)!=0, false, GetWindowKind(GetMyWindowWindowPtr(win)), contrlRfCon, mouseDown);
	
	BoxListFocus(tocH,true);
	
	if (contrlRfCon==kBoxSizeRefCon)
		SizeBoxClick(win,modifiers);
	else if (contrlRfCon==kConConProfRefCon)
		;
	else if (contrlRfCon==PREVIEW_TOGGLE_CNTL)
		BoxSetPreview(win,!GetControlValue(buttonHandle));
	else if (contrlRfCon==kDrawerSwitch)
		MBDrawerToggle(win);
	else if (contrlRfCon=='tabs')
		HitTab(win,buttonHandle,tocH);
	else if (i>0 && i<BoxLinesLimit)
		MBSortHit(tocH,i,0!=(modifiers&optionKey),0!=(modifiers&shiftKey));
	else if (iPrime > 0 && iPrime < 10 /* arbitrary hack alert */)
		ETLButtonHit(win,iPrime);
	else if ((*tocH)->hasFileView && (*tocH)->fileView)
		FileViewButton(win,buttonHandle,modifiers,part);
}

/**********************************************************************
 * RedoAllTOCs - fix all toc's to use new date format
 **********************************************************************/
void RedoAllTOCs(void)
{
	WindowPtr theWindow;
	TOCHandle tocH;
	
	for (theWindow = GetWindowList(); theWindow; theWindow = GetNextWindow (theWindow))
		if (IsKnownWindowMyWindow(theWindow) && (GetWindowKind(theWindow)==CBOX_WIN || GetWindowKind(theWindow)==MBOX_WIN))
		{
			tocH = (TOCHandle)GetWindowPrivateData(theWindow);
			(*tocH)->needRedo = 0;
			InvalTocBox(tocH,-1,blDate);
		}
}

/**********************************************************************
 * BoxScroll - do (part of) the scrolling for a mailbox.  Scrolls the header part h-wise
 **********************************************************************/
Boolean BoxScroll(MyWindowPtr win,short h,short v)
{
	if (h)
	{
		BoxPlaceBevelButtons(win);
	}
	return(True);
}

/************************************************************************
 * PriorityString - turn a priority into a string
 ************************************************************************/
UPtr PriorityString(UPtr string,Byte priority)
{
	/*
	 * normalize
	 */
	priority=Prior2Display(priority);
	GetRString(string,PRIOR_STRN+5+priority);
	return(string);
}

/**********************************************************************
 * SetPriority - set a message's priority, 
 * 				handle virtual TOCs, too
 **********************************************************************/
void SetPriority(TOCHandle tocH,short sumNum,short priority)
{
	TOCHandle realTOC;
	short	realSum;
	
	SetPriorityLo(tocH,sumNum,priority);
	realTOC = GetRealTOC(tocH,sumNum,&realSum);
	if (realTOC && realTOC != tocH)
	{
		// do real mailbox also if working in virtual mailbox
		SetPriorityLo(realTOC,realSum,priority);
		tocH = realTOC;
		sumNum = realSum;
	}
	SearchUpdateSum(tocH, sumNum, tocH, (*tocH)->sums[sumNum].serialNum, false, false);	//	Notify search window
}

/**********************************************************************
 * SetTAEScore - set a message's score, 
 * 				handle virtual TOCs, too
 **********************************************************************/
void SetTAEScore(TOCHandle tocH,short sumNum,short score)
{
	TOCHandle realTOC;
	short	realSum;
	
	if ((*tocH)->sums[sumNum].score != score)
	{
		SetTAEScoreLo(tocH,sumNum,score);
		realTOC = GetRealTOC(tocH,sumNum,&realSum);
		if (realTOC && realTOC != tocH)
		{
			// do real mailbox also if working in virtual mailbox
			SetTAEScoreLo(realTOC,realSum,score);
			tocH = realTOC;
			sumNum = realSum;
		}
		SearchUpdateSum(tocH, sumNum, tocH, (*tocH)->sums[sumNum].serialNum, false, false);	//	Notify search window
	}
}

/************************************************************************
 * SetPriorityLo - set a message's priority
 ************************************************************************/
void SetPriorityLo(TOCHandle tocH,short sumNum,short priority)
{
	MessHandle messH;
	ControlHandle cntl;
	short dp = Prior2Display(priority);
	MenuHandle mh;
	
	if (dp!=Prior2Display((*tocH)->sums[sumNum].priority))
	{
		/* invalidate priority box in the toc */
		InvalTocBox(tocH,sumNum,blPrior);
		
		// make sure the menus know
		if (mh=GetMHandle(PRIOR_HIER_MENU))
		{
			CheckNone(mh);
			SetItemMark(mh,dp,diamondMark);
		}
		
		/* set the priority display in the message */
		if ((messH=(*tocH)->sums[sumNum].messH) && IsWindowVisible(GetMyWindowWindowPtr((*messH)->win)))
		{
			if (cntl = FindControlByRefCon((*messH)->win,PRIOR_HIER_MENU))
			{
				SetBevelIcon(cntl,dp==3 ? PRIORITY_HOLLOW_ICON : PRIOR_SICN_BASE+dp-1,0,0,nil);
				SetBevelMenuValue(cntl,dp);
				Draw1Control(cntl);
			}
			if ((*messH)->hStationerySpec) (*messH)->win->isDirty = True;
		}
	}
	(*tocH)->sums[sumNum].priority = priority;
	TOCSetDirty(tocH,true);
}

/************************************************************************
 * SetTAEScoreLo - set a message's score
 ************************************************************************/
void SetTAEScoreLo(TOCHandle tocH,short sumNum,short score)
{
	if (score != (*tocH)->sums[sumNum].score)
	{
		if (!(*tocH)->noInvalBox) InvalTocBox(tocH,sumNum,blAnal);
		(*tocH)->sums[sumNum].score = score;
		TOCSetDirty(tocH,true);
	}
}

/************************************************************************
 * InvalTocBox - invalidate one area of a mailbox window
 ************************************************************************/
void InvalTocBox(TOCHandle tocH,short sumNum,short box)
{
	InvalTocBoxLo(tocH,sumNum,box);
	if (sumNum >= 0)
		SearchInvalTocBox(tocH,sumNum,box);
}

/************************************************************************
 * InvalTocBoxLo - invalidate one area of a mailbox window
 ************************************************************************/
void InvalTocBoxLo(TOCHandle tocH,short sumNum,short box)
{
	WindowPtr	tocWinWP = GetMyWindowWindowPtr ((*tocH)->win);
	Rect r;
	
	if ((*tocH)->win && IsWindowVisible (tocWinWP))
	{
		PushGWorld();
		SetPort_(GetWindowPort(tocWinWP));
	
		if (sumNum==-1)
		{
			r.top = 0;
			r.bottom = (*tocH)->win->topMargin;
		}
		else if (sumNum==-2)
		{
			r.top = (*tocH)->win->topMargin;
			r.bottom = (*tocH)->win->contR.bottom;
		}
		else
		{
			r.top = (*tocH)->win->topMargin + (sumNum-GetControlValue((*tocH)->win->vBar))*(*tocH)->win->vPitch;
			r.bottom = r.top + (*tocH)->win->vPitch;
		}
		BoxLimits(box,&r.left,&r.right,tocH);
		OffsetRect(&r,-GetControlValue((*tocH)->win->hBar)*(*tocH)->win->hPitch,0);
		InvalWindowRect(tocWinWP,&r);
		if (MBGetSort(tocH,box)) (*tocH)->resort = MAX((*tocH)->resort,kNoSlowResort);
		PopGWorld();
	}
}

/**********************************************************************
 * CopySummary - put the contents of a summary onto a text handle
 **********************************************************************/
OSErr CopySummary(TOCHandle tocH, short sumNum, Handle into)
{
	Str255 summary;
	Str255 sMBName;
	Str63 states;
	Str63 priority;
	Str63 labelString;
	Str31 dateStr;
	MSumType sum = (*tocH)->sums[sumNum];
	RGBColor color;
	
	GetRString(states,STATE_LABELS);
	if (GetSumColor(tocH,sumNum)) MyGetLabel(GetSumColor(tocH,sumNum),&color,&labelString);
		*labelString = 0;
	GetRString(priority,PRIOR_STRN+Prior2Display(sum.priority));
	
	if ((*tocH)->virtualTOC)
	{
		// Include mailbox name for virtual mailboxes
		GetMailboxName(tocH,sumNum,sMBName);
		ComposeRString(summary,SUM_SEARCH_COPY_FMT,
			states[sum.state],
			priority,
			(sum.flags & FLAG_HAS_ATT) ? YesStr : "",
			labelString,
			sum.from,
			ComputeLocalDate(&sum,dateStr),
			sum.length/(1 K),
			sMBName,
			sum.subj);		
	}
	else
	{
		ComposeRString(summary,SUM_COPY_FMT,
			states[sum.state],
			priority,
			(sum.flags & FLAG_HAS_ATT) ? YesStr : "",
			labelString,
			sum.from,
			ComputeLocalDate(&sum,dateStr),
			sum.length/(1 K),
			sum.subj);
	}
	
	return(PtrPlusHand_(summary+1,into,*summary));
}

/**********************************************************************
 * CopySelectedSummaries - copy selected summaries from a mailbox
 **********************************************************************/
OSErr CopySelectedSummaries(TOCHandle tocH)
{
	Handle copyText = NuHandle(0);
	OSErr err = noErr;
	short i;
	
	if (!copyText) return(WarnUser(COPY_FAILED,MemError()));
	
	for (i=0;i<(*tocH)->count;i++)
		if ((*tocH)->sums[i].selected && (err=CopySummary(tocH,i,copyText))) break;
	
	if (!err)
	{
		ScrapRef	scrap;
		ClearCurrentScrap();
		GetCurrentScrap(&scrap);
		err = PutScrapFlavor(scrap,kScrapFlavorTypeText,kScrapFlavorMaskNone,GetHandleSize(copyText),LDRef(copyText));
	}
	ZapHandle(copyText);

	if (err) WarnUser(COPY_FAILED,err);

	return(err);
}

/**********************************************************************
 * BoxLine2Item - turn a line # into a sort menu item
 **********************************************************************/
short BoxLine2Item(short line)
{
	short item;
	
	switch (line)
	{
		case blStat: item = SORT_STATUS_ITEM; break;
		case blJunk: item = SORT_JUNK_ITEM; break;
		case blPrior: item = SORT_PRIORITY_ITEM; break;
		case blFrom: item = SORT_SENDER_ITEM; break;
		case blDate: item = SORT_TIME_ITEM; break;
		case blMailbox:	item = SORT_MAILBOX_ITEM; break;
		case blLabel: item = SORT_LABEL_ITEM; break;
		case blSize: item = SORT_SIZE_ITEM; break;
		case blAttach: item = SORT_ATTACHMENTS_ITEM; break;
		case blSubject: item = SORT_SUBJECT_ITEM; break;
		case blAnal: item = SORT_MOOD_ITEM; break;
		default: item = 0; break;
	}
	return(item);
}

/**********************************************************************
 * BoxItem2Line - turn a sort menu item into a line number
 **********************************************************************/
short BoxItem2Line(short item)
{
	short line;
	
	switch (item)
	{
		case SORT_STATUS_ITEM: line = blStat; break;
		case SORT_JUNK_ITEM: line = blJunk; break;
		case SORT_PRIORITY_ITEM: line = blPrior; break;
		case SORT_SENDER_ITEM: line = blFrom; break;
		case SORT_TIME_ITEM: line = blDate; break;
		case SORT_MAILBOX_ITEM: line = blMailbox; break;
		case SORT_LABEL_ITEM: line = blLabel; break;
		case SORT_SIZE_ITEM: line = blSize; break;
		case SORT_ATTACHMENTS_ITEM: line = blAttach; break;
		case SORT_SUBJECT_ITEM: line = blSubject; break;
		case SORT_MOOD_ITEM: line = blAnal; break;
		default: line = 0; break;
	}
	return(line);
}


/************************************************************************
 * BoxDragDividers - see if the user is trying to drag dividers, and help
 * him if he is.
 ************************************************************************/
Boolean BoxDragDividers(Point pt, MyWindowPtr win)
{
	short line;
	long ticks = TickCount()+GetDblTime();
	Point mouse;
	short slop = GetRLong(DOUBLE_TOLERANCE);
	RgnHandle greyRgn;
	Rect r,bounds;
	short h0;
	short dh;
	long dvdh;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	PushGWorld();
	
	if (pt.v > win->topMargin && BoxFindDivider(pt,&line))
	{
		while (TickCount()<ticks)
		{
			if (!StillDown()) {PopGWorld();return(False);}
			GetMouse(&mouse);
			if (ABS(mouse.v-pt.v)>slop) {PopGWorld();return(False);}
			if (ABS(mouse.h-pt.h)>slop) break;
		}
		if (greyRgn = NewRgn())
		{
			GetMouse(&mouse);
			r = bounds = win->contR;
			r.left = pt.h;
			r.right = pt.h+1;
			r.top = win->topMargin - (GROW_SIZE+4);
			RectRgn(greyRgn,&r);
			h0 = -win->hPitch*GetControlValue(win->hBar);
			BoxLimits(line,&bounds.left,(short*)&dvdh,tocH);
			bounds.left += h0;
			bounds.top = 0;
			dvdh = DragGrayRgn(greyRgn,mouse,&bounds,&bounds,hAxisOnly,nil);
			dh = dvdh&0xffff;
			/*
			 * I don't understand why I have to do this; I guess I don't understand
			 * DragGrayRgn.
			 */
			GetMouse(&mouse);
			if (mouse.h<=pt.h && dh>0) dh = 0;
			if (pt.h+dh<bounds.left) dh = 0;
			
			if (dh&0x7fff && (dh = RoundDiv(dh,FontWidth)))
			{
				WindowPtr	theWindow;
				(*BoxWidths)[line-1] = MAX((*BoxWidths)[line-1]+dh,0);
				SaveBoxLines();
				InfiniteClip(GetMyWindowCGrafPtr(win));
				for (theWindow = GetWindowList(); theWindow; theWindow = GetNextWindow (theWindow))
					if (IsKnownWindowMyWindow(theWindow) && (GetWindowKind(theWindow)==CBOX_WIN || GetWindowKind(theWindow)==MBOX_WIN))
					{
						SetPort_(GetWindowPort(theWindow));
						(*tocH)->needRedo = 0;
						InvalContent(GetWindowMyWindowPtr(theWindow));
						BoxPlaceBevelButtons(GetWindowMyWindowPtr(theWindow));
					}
			}
		}
		PopGWorld();
		return(True);
	}
	PopGWorld();
	return(False);
}

/************************************************************************
 * BoxFindDivider - is the current point over a dividing line?
 * Returns true if it is, and puts the line number in which.
 * puts the line to the right of the mouse in which if it isn't
 ************************************************************************/
Boolean BoxFindDivider(Point pt,short *which)
{
	short i;
	WindowPtr	winWP = FrontWindow_();
	MyWindowPtr win = GetWindowMyWindowPtr (winWP);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	short h0 = -win->hPitch*GetControlValue(win->hBar);
	short l,r;
	
	if (pt.v>win->contR.bottom) return false;
	if (pt.h>win->contR.right) return false;
	for (i=1;i<BoxLinesLimit;i++)
	{
		BoxLimits(i,&l,&r,tocH);
		r += h0;
		l += h0;
		if (r-2<=pt.h && pt.h<=r) {*which=i;return(True);}
		if (l < pt.h && pt.h < r) {*which=i;return(False);}
	}
	*which = BoxLinesLimit;
	return(False);
}

/************************************************************************
 * BoxCursor - change the cursor appropriately
 ************************************************************************/
void BoxCursor(Point mouse)
{
	WindowPtr	winWP = FrontWindow_();
	MyWindowPtr win = GetWindowMyWindowPtr (winWP);
	short h0 = -win->hPitch*GetControlValue(win->hBar);
	short which,mNum;
	Rect r = win->contR;
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);

	r.top = 0;
	if (!PtInRect(mouse,&r))
		SetMyCursor(arrowCursor);
	else
	{
		mNum = (mouse.v-win->topMargin)/win->vPitch + GetControlValue(win->vBar);
		if (mouse.v>win->topMargin && BoxFindDivider(mouse,&which))
		{
			SetMyCursor(DIVIDER_CURS);
		}
		else if (mouse.v<win->topMargin)
			SetMyCursor(arrowCursor);
		else
		{
#ifdef TWO
			if (!(MainEvent.modifiers&(shiftKey|optionKey|cmdKey)) && 
					(which<=blAttach || which==blLabel || which==blServer || which==blMailbox))
			{
				mNum = (mouse.v-win->topMargin)/win->vPitch + GetControlValue(win->vBar);
				if (mNum<(*tocH)->count && (*tocH)->sums[mNum].selected)
					SetMyCursor(MENU_CURS);
				else
					SetMyCursor(plusCursor);
			}
			else
#endif
				SetMyCursor(plusCursor);
		}
	}
}

#pragma segment BoxMain
/************************************************************************
 * RedoTOC - figure out how wide a toc is
 ************************************************************************/
Boolean RedoTOC(TOCHandle tocH)
{
	short subMax;
#ifdef NEVER
	short subStart;
	short subWidth;
	short sumNum;
#endif
	Boolean needResort;
	Rect r;
	MyWindowPtr tocWin;
	WindowPtr		tocWinWP;
	short maxValid = MIN((*tocH)->needRedo,(*tocH)->maxValid);
	Boolean result = false;
	FSSpec spec;
	long top;
	short col;
	short colStart, colEnd;
	short rightCol;
	
	spec = GetMailboxSpec(tocH,-1);

	if (InAThread() || (*tocH)->which==IN_TEMP || (*tocH)->which==OUT_TEMP || IsDelivery(&spec))
	{
		(*tocH)->needRedo = (*tocH)->maxValid = (*tocH)->count;
		return false;
	}
	UL(tocH);	// just in case...
	
//	if (maxValid<(*tocH)->count && PrefIsSet(PREF_DELDUP)) TOCDelDup(tocH);
		
	if (!MBIsSticky(tocH)) needResort = (*tocH)->resort = kDontResort;
	else
		needResort = (*tocH)->resort>kNoSlowResort || (*tocH)->resort&&!PrefIsSet(PREF_SLOW_RESORT);

	tocWin = (*tocH)->win;
	tocWinWP = GetMyWindowWindowPtr (tocWin);
	
	if (maxValid < (*tocH)->count)
	{
		PushGWorld();
		SetPort_(GetMyWindowCGrafPtr(tocWin));

#ifdef NEVER
		if (RunType!=Production)
		{
			Dprintf("\pRedoing �%p� thePort %x tocWin %x;wh %x;g",spec.name,qd.thePort,(*tocH)->win,*tocH);
		}
#endif

		result = true;	// we did something
		if (maxValid<0) maxValid = 0;
				
#ifdef NEVER
		/*
		 * measure all the subject lines
		 */
		BoxLimits(blSubject,&subStart,&subMax,tocH);
		subMax = 0;
		if (FontIsFixed)
		{
			for (sumNum=maxValid;sumNum<(*tocH)->count;sumNum++)
				subMax = MAX(subMax,*(*tocH)->sums[sumNum].subj);
			subMax *= tocWin->hPitch;
		}
		else
		{
			LDRef(tocH);
			for (sumNum=maxValid;sumNum<(*tocH)->count;sumNum++)
			{
				subWidth = StringWidth((*tocH)->sums[sumNum].subj);
				subMax = MAX(subMax,subWidth);
			}
			UL(tocH);
		}
#endif
		
		/*
		 * find the rightmost column
		 */
		rightCol = 0;
		for (col=1;col<BoxLinesLimit;col++)
			if (BoxColumnShowing(col,tocH))
			{
				BoxLimits(col,&colStart,&colEnd,tocH);
				rightCol = MAX(rightCol,colEnd);
			}
		
		subMax = RoundDiv(rightCol,tocWin->hPitch);
		
		/*
		 * install new maximums; take the old max into account only if we're
		 * doing a partial
		 */
		if ((*tocH)->needRedo) subMax = MAX(subMax,tocWin->hMax);
		(*tocH)->needRedo = (*tocH)->count;			/* don't need to do this again */

		/*
		 * invalidate all the new ones
		 */
		r = tocWin->contR;
		top = tocWin->topMargin + tocWin->vPitch * (maxValid-GetControlValue(tocWin->vBar));
		if (top < r.bottom)
		{
			r.top = MAX(top,tocWin->contR.top);
			InvalWindowRect(GetMyWindowWindowPtr(tocWin),&r);
		}
		(*tocH)->maxValid = (*tocH)->count;
		if (!needResort) UpdateMyWindow(tocWinWP);
		MyWindowMaxes(tocWin,subMax,(*tocH)->count);
	}

	if (FrontWindow_()!=tocWinWP && tocWin->isActive)
	{
		result = true;
		ActivateMyWindow(tocWinWP,False);
	}
		
	if (needResort && tocWin && IsWindowVisible (tocWinWP))
	{
		if ((*tocH)->resort == kResortWhenever)
		{
			uLong elapsedTicks = TickCount() - (*tocH)->lastSortTicks;
			
			if (elapsedTicks < GetRLong(MIN_MB_SORT_TICKS))
				needResort = false;	//	Let's not resort so soon
		}
	
		if (needResort)
		{
			MBResort(tocH);
			result = true;
		}
	}
	
	if (MyWinHasSelection((*tocH)->win))
		(*tocH)->win->hasSelection = true;
	else
		(*tocH)->win->hasSelection = false;
	
	if (result)
	{
		ShowBoxSizes((*tocH)->win);
		PopGWorld();
	}
	return(result);
}

/**********************************************************************
 * MBIsSticky - is a mailbox set for sticky sort
 **********************************************************************/
Boolean MBIsSticky(TOCHandle tocH)
{
	short i;
	short max=0;
	
	for (i=1;i<=NSORT;i++) if (MBGetSort(tocH,i)) max = MBGetSort(tocH,i);
	
	return(max>0);
}

#pragma segment Balloon
/************************************************************************
 * BoxHelp - help for mailboxes
 ************************************************************************/
void BoxHelp(MyWindowPtr win, Point mouse)
{
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	Rect r;
	Rect tallContR = win->contR;
	short hnum=0;
	short column;
	short hoff = win->hPitch*GetControlValue(win->hBar);
	ControlHandle cntl;
	short factor = BoxLinesLimit-1;

	SetRect(&r,0,0,REAL_BIG/2,REAL_BIG/2);
	tallContR.top = win->topMargin - (GROW_SIZE+4);
	
	if (PtInRect(mouse,&tallContR))
	{
		if (BoxFindDivider(mouse,&column))
		{
			BoxLimits(column,&r.left,&r.right,tocH);
			r.right++; r.left = r.right-3;
			OffsetRect(&r,-hoff,0);
			SectRect(&r,&tallContR,&r);
			MyBalloon(&r,70,0,MBOX_HELP_STRN+column,0,nil);
		}
		else
		{
			BoxLimits(column,&r.left,&r.right,tocH);
			OffsetRect(&r,-hoff,0);
			r.right = MIN(r.right,win->contR.right);
			if (mouse.v<win->topMargin)
			{
				Boolean s = MBGetSort(tocH,column);
				r.bottom = win->topMargin;
				r.top = r.bottom - (GROW_SIZE+4);
				MyBalloon(&r,70,0,MBOX_HELP_STRN+(s?3:2)*factor+column,0,nil);
			}
			else
			{
				r.top = win->topMargin; r.bottom = win->contR.bottom;
				if (column==blServer)
				{
					if ((*tocH)->imapTOC)
						MyBalloon(&r,70,0,0,IMAP_BOX_SERVER_COLUMN_HELP_PICT,nil);
					else
						MyBalloon(&r,70,0,0,133,nil);
				}
				else if (column>2)
					MyBalloon(&r,70,0,MBOX_HELP_STRN+factor+column,0,nil);
				else if (column==blPrior)
					MyBalloon(&r,70,0,0,PRIOR_HELP_PICT,nil);
				else if (column==blStat)
					MyBalloon(&r,70,0,0,STATUS_HELP_PICT,nil);
			}
		}
	}
	else if (cntl=MyFindControl(mouse,win))
	{
		short n = 4*factor+1;
		
		switch(GetControlReference(cntl))
		{
			case PREVIEW_DIVIDE_CNTL: n+=2; break;
			case PREVIEW_TOGGLE_CNTL: n++; break;
			case kBoxSizeRefCon:
				if ((*tocH)->imapTOC)
				{
					r = *GetControlBounds(cntl,&r);
					MyBalloon(&r,70,0,PrefIsSet(PREF_SHOW_NUM_SELECTED)?IMAP_COMPACT_SHOW_NUM_HELP:IMAP_COMPACT_HELP,0,nil);
					n = 0;
				}
				else if (PrefIsSet(PREF_SHOW_NUM_SELECTED))
					n += 3;
				break;
			default: n = 0;
		}
		
		if (n)
		{
			r = *GetControlBounds(cntl,&r);
			MyBalloon(&r,70,0,MBOX_HELP_STRN+n,0,nil);
		}
	}
	
}

#pragma segment BoxSort
/**********************************************************************
 * BoxSelectSame - select all messages with the same <something>
 **********************************************************************/
void BoxSelectSame(TOCHandle tocH, short item,short clickedSum)
{
	short selected, candidate;
	int (*compare)();
	short last;
	Boolean need;
	
	/*
	 * phase 1 - figure out the comparator
	 */
	switch(item)
	{
		case SORT_STATUS_ITEM:
			compare = SumStatCompare;
			break;
		case SORT_JUNK_ITEM:
			compare = SumJunkCompare;
			break;
		case SORT_PRIORITY_ITEM:
			compare = SumPriorCompare;
			break;
		case SORT_SUBJECT_ITEM:
			compare = SumSubjCompare;
			break;
		case SORT_SENDER_ITEM:
			compare = SumFromCompare;
			break;
		case SORT_TIME_ITEM:
			compare = SumTimeFuzzCompare;
			break;
		case SORT_MAILBOX_ITEM:
			compare = SumMailboxCompare;
			break;
		case SORT_SIZE_ITEM:
			compare = SumSizeKCompare;
			break;
		case SORT_ATTACHMENTS_ITEM:
			compare = SumAttCompare;
			break;
		case SORT_LABEL_ITEM:
			compare = SumLabelCompare;
			break;
		case SORT_MOOD_ITEM:
			compare = SumAnalCompare;
			break;
		default: return;
	}

	LDRef(tocH);
	gSortTOC = tocH;
		
	/*
	 * phase 2 - figure out which ones we need to select
	 */
	for (selected=0;selected<(*tocH)->count;selected++)
	{
		if ((*tocH)->sums[selected].selected)
		{
			(*tocH)->sums[selected].spareShort2 = 0;	/* for compare's benefit */
			for (candidate=0;candidate<(*tocH)->count;candidate++)
				if (candidate!=selected && !((*tocH)->sums[candidate].opts&OPT_WILL_SEL))
				{
					(*tocH)->sums[candidate].spareShort2 = 0;
					if (!(*compare)(&(*tocH)->sums[selected],&(*tocH)->sums[candidate]))
						(*tocH)->sums[candidate].opts |= OPT_WILL_SEL;
				}
		}
	}

	UL(tocH);
	
	/*
	 * phase 3 - select them
	 */
	for (candidate=0;candidate<(*tocH)->count;candidate++)
		if ((*tocH)->sums[candidate].opts&OPT_WILL_SEL)
		{
			if (!(*tocH)->sums[candidate].selected)
				SelectBoxRange(tocH,candidate,candidate,True,-1,-1);
			(*tocH)->sums[candidate].opts &= ~OPT_WILL_SEL;
		}
	
	/*
	 * phase 4 - sort them
	 */
	last = -1;
	need = False;
	for (candidate=0;candidate<(*tocH)->count;candidate++)
	{
		if ((*tocH)->sums[candidate].selected)
		{
			if (last!=-1 && last != candidate-1)
			{
				need = True;
				break;
			}
			last = candidate;
		}
	}
	
	if (need)
	{
		for (candidate=0;candidate<(*tocH)->count;candidate++)
		{
			if ((*tocH)->sums[candidate].selected) (*tocH)->sums[candidate].spareShort = 1;
			else (*tocH)->sums[candidate].spareShort = (candidate<clickedSum) ? 0 : 2;
		}
		SortTOC(tocH,False,SumSelectCompare);
		InvalContent((*tocH)->win);
	}
}
/**********************************************************************
 * MBSortHit - handle hits on the box headers
 **********************************************************************/
void MBSortHit(TOCHandle tocH, short index, Boolean reverse, Boolean extend)
{
	Byte sort = MBGetSort(tocH,index);
	short i;
	short sortOrder;
	ControlHandle cntl;
	
	if (!extend)
	  for (i=1;i<=NSORT;i++) MBRemoveSort(tocH,i);
	
	if (!sort)
	{
		sort = reverse ? SORT_DESCEND : SORT_ASCEND;
		MBAddSort(tocH,index,sort);
	}
	else if (reverse)
	{
		sortOrder = sort&0x3;
		sortOrder = (sortOrder==SORT_ASCEND) ? SORT_DESCEND : SORT_ASCEND;
		MBGetSort(tocH,index) = (sort & ~0x3) | sortOrder;
		(*tocH)->resort = kResortNow;
		if (cntl=FindControlByRefCon((*tocH)->win,'wide'+index))
			SetControlValue(cntl,1);
	}
	else if (extend)
	{
		MBRemoveSort(tocH,index);
	}
}

/**********************************************************************
 * MBRemoveSort - remove a sort
 **********************************************************************/
void MBRemoveSort(TOCHandle tocH,short index)
{
	Byte oldSort = MBGetSort(tocH,index);
	Boolean needResort=False;
	ControlHandle cntl;
	
	if (MBGetSort(tocH,index))
	{
		MBGetSort(tocH,index) = 0;
		if (UseListColors) InvalTocBox(tocH,-2,index);
		if (cntl=FindControlByRefCon((*tocH)->win,'wide'+index))
			SetControlValue(cntl,0);
		
		for (index=1;index<=NSORT;index++)
			if (MBGetSort(tocH,index)>oldSort)
			{
				MBGetSort(tocH,index) -= 4;
				needResort = 2;
			}
		(*tocH)->resort = kResortNow;
		TOCSetDirty(tocH,true);
	}
}

/**********************************************************************
 * MBAddSort - add a sort
 **********************************************************************/
void MBAddSort(TOCHandle tocH,short index,Byte sortOrder)
{
	short i;
	Byte max=0;
	ControlHandle cntl;
	
	for (i=1;i<=NSORT;i++)
		if (MBGetSort(tocH,i)>max) max = MBGetSort(tocH,i);
	
	max = (((max>>2)+1)<<2) + sortOrder;
	
	MBGetSort(tocH,index) = max;
	(*tocH)->resort = kResortNow;
	TOCSetDirty(tocH,true);
	if (cntl=FindControlByRefCon((*tocH)->win,'wide'+index))
		SetControlValue(cntl,1);
}


/**********************************************************************
 * MBResort - resort a mailbox
 **********************************************************************/
int (*MBCompareTable[BoxLinesLimit+2])();
void MBResort(TOCHandle tocH)
{
	Byte max=0;
	short i;
	short n;
	Boolean reverse;
	short sub=1;
	Boolean group = ((*tocH)->laurence)!=0;
	
	// don't sort any IMAP mailboxes that are in a senstive stage of resynchronization
	if ((*tocH)->imapTOC)
	{
		MailboxNodeHandle mBox = TOCToMbox(tocH);
		if (mBox && *mBox)
		{
			if (DoesIMAPMailboxNeed(mBox, kNeedsSortLock))
			{
				(*tocH)->resort = kResortNow;
				return;
			}
		}
	}
	
	if (group)
	{
		int (*compare)();
		
		//sort by subject
		GroupSubjThreshTime = GetRLong(GROUP_SUBJ_MAX_TIME) * 3600*24;
		compare = GroupSubjThreshTime ? SumSubjTimedCompare : SumSubjCompare;
		MBCompareTable[0] = SumSubjCompare;
		// if using a time threshhold, sort secondarily by date
		MBCompareTable[1] = GroupSubjThreshTime ? SumTimeCompare : nil;
		MBCompareTable[2] = nil;
		SortTOC(tocH,False,MBResortCompare);
		
		// give identical id's to identical subjects
		(*tocH)->sums[0].subjId = sub;
		LDRef(tocH);
		for (i=1;i<(*tocH)->count;i++)
		{
			if (compare((*tocH)->sums+i,(*tocH)->sums+i-1)) sub++;
			(*tocH)->sums[i].subjId = sub;
		}
		UL(tocH);
	}
	
	for (i=1;i<=NSORT;i++) if (MBGetSort(tocH,i)>max) max = MBGetSort(tocH,i);
	
	reverse = SORT_DESCEND==(max&3);
	max >>= 2;
	max = MIN(max,BoxLinesLimit);
	
	if (MBGetSort(tocH,blMailbox) && !(tocH && (*tocH)->virtualTOC))
	{
		//	This mailbox was sorted by subject with an older version of Eudora
		MBGetSort(tocH,blSubject) = MBGetSort(tocH,blMailbox);
		MBGetSort(tocH,blMailbox) = 0;
	}
	
	if (max)
	{
		for (n=1;n<=max;n++)
			for (i=1;i<=NSORT;i++)
				if (((*tocH)->sorts[i-1]>>2)==n)
				{
					MBCompareTable[n-1] = FindMBSort(BoxLine2Item(i),(MBGetSort(tocH,i)&3)==SORT_DESCEND);
					(*tocH)->lastSort = BoxLine2Item(i);
					break;
				}
		MBCompareTable[n-1] = nil;
		SortTOC(tocH,reverse,MBResortCompare);
		TOCSetDirty(tocH,true);
	}
	(*tocH)->resort = kDontResort;
	
	if (group)
	{
		// zero the spare short
		for (i=(*tocH)->count-1;i>=0;i--) (*tocH)->sums[i].spareShort = 0;
		
		// for each equal subject, put the index of the highest subj
		// in the spare short.
		for (i=(*tocH)->count-1;i>=0;i--)
		{
			if (!(*tocH)->sums[(*tocH)->sums[i].subjId-1].spareShort)
			{
				// we haven't seen this one yet; assign id
				(*tocH)->sums[(*tocH)->sums[i].subjId-1].spareShort = sub;
				(*tocH)->sums[i].subjId = sub;
				sub--;
			}
			else //we have seen it; copy
				(*tocH)->sums[i].subjId	= (*tocH)->sums[(*tocH)->sums[i].subjId-1].spareShort;
		}
		
		// and now, one final sort
		MBCompareTable[0] = SumSubjIdCompare;
		MBCompareTable[1] = nil;
		SortTOC(tocH,False,MBResortCompare);
	}
	
	if ((*tocH)->win)
	{
		InvalContent((*tocH)->win);
		BoxCenterSelection((*tocH)->win);
	}

	(*tocH)->lastSortTicks = TickCount();
}

/**********************************************************************
 * MBResortCompare - resort a mailbox with multiple conditions
 **********************************************************************/
int MBResortCompare(MSumPtr s1, MSumPtr s2)
{
	short i;
	long res = 0;
	
	//	Check for sentinel first
	if (s1->length==kLargeUniqueValue) return 1;
	if (s2->length==kLargeUniqueValue) return -1;

	for (i=0;MBCompareTable[i] && !res;i++)
		res = (*MBCompareTable[i])(s1,s2);
	return (res ? res : s1->spareShort2-s2->spareShort2);
}
/**********************************************************************
 * FindMBSort - find the right sort function
 **********************************************************************/
int (*FindMBSort(short item,Boolean reverse))()
{
	switch(item)
	{
		case SORT_STATUS_ITEM:
			return(reverse?RevSumStatCompare:SumStatCompare);
			break;
		case SORT_JUNK_ITEM:
			return(reverse?RevSumJunkCompare:SumJunkCompare);
			break;
		case SORT_PRIORITY_ITEM:
			return(reverse?RevSumPriorCompare:SumPriorCompare);
			break;
		case SORT_SUBJECT_ITEM:
			return(reverse?RevSumSubjCompare:SumSubjCompare);
			break;
		case SORT_SENDER_ITEM:
			return(reverse?RevSumFromCompare:SumFromCompare);
			break;
		case SORT_TIME_ITEM:
			if (MainEvent.modifiers&controlKey)
				return(reverse?RevSumOffsetCompare:SumOffsetCompare);
			else
				return(reverse?RevSumTimeCompare:SumTimeCompare);
			break;
		case SORT_MAILBOX_ITEM:
			return(reverse?RevSumMailboxCompare:SumMailboxCompare);
			break;
		case SORT_SIZE_ITEM:
			return(reverse?RevSumSizeCompare:SumSizeCompare);
			break;
		case SORT_ATTACHMENTS_ITEM:
				return(reverse?RevSumAttCompare:SumAttCompare);
				break;
		case SORT_LABEL_ITEM:
			return(reverse?RevSumLabelCompare:SumLabelCompare);
		case SORT_MOOD_ITEM:
			return(reverse?RevSumAnalCompare:SumAnalCompare);
			break;
	}
	return(nil);
}	

/************************************************************************
 * SwapSum
 ************************************************************************/
void SwapSum(MSumPtr sum1, MSumPtr sum2)
{
	MSumType tempSum;
	tempSum = *sum1;
	*sum1 = *sum2;
	*sum2 = tempSum;
}

/************************************************************************
 * SumTimeCompare - compare the arrival times of two sums
 ************************************************************************/
int SumTimeCompare(MSumPtr sum1, MSumPtr sum2)
{
	long res = (unsigned)sum1->seconds > (unsigned)sum2->seconds ? 1 :
						 (sum1->seconds==sum2->seconds ? 0 : -1);
	return(res);
}

/************************************************************************
 * SumMailboxCompare - compare the mailboxes of two (virtual mailbox) sums
 ************************************************************************/
int SumMailboxCompare(MSumPtr sum1, MSumPtr sum2)
{
	FSSpecHandle	specList = (*gSortTOC)->mailbox.virtualMB.specList;
	Str63	s1,s2;


	PSCopy(s1,(*specList)[sum1->u.virtualMess.virtualMBIdx].name);
	PSCopy(s2,(*specList)[sum2->u.virtualMess.virtualMBIdx].name);
	return(StringComp(s1,s2));
}

/************************************************************************
 * SumOffsetCompare - compare the disk offsets of two sums
 ************************************************************************/
int SumOffsetCompare(MSumPtr sum1, MSumPtr sum2)
{
	long res = (unsigned)sum1->offset > (unsigned)sum2->offset ? 1 :
						 (sum1->offset==sum2->offset ? 0 : -1);
	return(res);
}

/************************************************************************
 * SumTimeFuzzCompare - compare the arrival times of two sums, fuzzily
 ************************************************************************/
int SumTimeFuzzCompare(MSumPtr sum1, MSumPtr sum2)
{
	Str31 string1, string2;
	return(StringComp(ComputeLocalDate(sum1,string1),ComputeLocalDate(sum2,string2)));
}

/************************************************************************
 * SumSubjCompare - compare the subjects of two sums
 ************************************************************************/
int SumSubjCompare(MSumPtr sum1, MSumPtr sum2)
{
	short res = SubjCompare(sum1->subj,sum2->subj);
	return (res);
}

/************************************************************************
 * SumSubjTimedCompare - compare the subjects of two summaries, but only let
 * them be equal if they are within a certain amount of time
 ************************************************************************/
int SumSubjTimedCompare(MSumPtr sum1, MSumPtr sum2)
{
	long diff = sum1->seconds-sum2->seconds;
	
	if (ABS(diff) > GroupSubjThreshTime) return diff;
	
	return SubjCompare(sum1->subj,sum2->subj);
}

/************************************************************************
 * SumSubjIdCompare - compare the spare shorts of two sums
 ************************************************************************/
int SumSubjIdCompare(MSumPtr sum1, MSumPtr sum2)
{
	short res = sum1->subjId-sum2->subjId;
	return (res);
}

/**********************************************************************
 * SubjCompare - compare two subjects
 **********************************************************************/
int SubjCompare(PStr in1, PStr in2)
{
	Str127 s1,s2;
	Boolean maxed1, maxed2;
	MSumType sum;
	
	PSCopy(s1,in1);
	PSCopy(s2,in2);

	maxed1 = *s1>=sizeof(sum.subj)-2;
	maxed2 = *s2>=sizeof(sum.subj)-2;
	
#ifdef NEVER
	ComposeLogS(-1,nil,"\pComparing �%p� �%p�",s1,s2);
#endif
	TrimRe(s1,true);
	TrimRe(s2,true);
	TrimInitialWhite(s1);
	TrimInitialWhite(s2);
	TrimWhite(s1);
	TrimWhite(s2);
	TrimInternalWhite(s1);
	TrimInternalWhite(s2);
	if (maxed1) *s2 = MIN(*s2,*s1);	// subject 1 maxed out--trim subject 2
	if (maxed2) *s1 = MIN(*s2,*s1);	// subject 2 maxed out--trim subject 1
	return(StringComp(s1,s2));
}

/************************************************************************
 * SumFromCompare - compare the senders of two sums
 ************************************************************************/
int SumFromCompare(MSumPtr sum1, MSumPtr sum2)
{
	short res=StringComp(sum1->from,sum2->from);
	return (res);
}

/************************************************************************
 * SumSizeCompare - compare two messages by size
 ************************************************************************/
int SumSizeCompare(MSumPtr sum1, MSumPtr sum2)
{
	long l1 = EffectiveLength(sum1);
	long l2 = EffectiveLength(sum2);

#ifdef DEBUG
	if (RunType!=Production && BUG2) return sum1->serialNum - sum2->serialNum;
#endif
	return (l1-l2);
}

/************************************************************************
 * SumSizeKCompare - compare two messages by size in K
 ************************************************************************/
int SumSizeKCompare(MSumPtr sum1, MSumPtr sum2)
{
	long l1 = EffectiveLength(sum1)/(1 K);
	long l2 = EffectiveLength(sum2)/(1 K);

#ifdef DEBUG
	if (RunType!=Production && BUG2) return sum1->serialNum - sum2->serialNum;
#endif
	return (l1-l2);
}

/************************************************************************
 * SumLabelCompare - compare two messages by color
 ************************************************************************/
int SumLabelCompare(MSumPtr sum1, MSumPtr sum2)
{
	short res = SumColor(sum1)-SumColor(sum2);
	return (res);
}

/************************************************************************
 * SumAnalCompare - compare two messages by analysis
 ************************************************************************/
int SumAnalCompare(MSumPtr sum1, MSumPtr sum2)
{
	short res = (unsigned)sum1->score > (unsigned)sum2->score ? 1 :
						 (sum1->score==sum2->score ? 0 : -1);
	return (res);
}

/************************************************************************
 * SumAttCompare - compare two summaries based on whether or not they have attachments
 ************************************************************************/
int SumAttCompare(MSumPtr sum1, MSumPtr sum2)
{
	long res = (long)(sum1->flags&FLAG_HAS_ATT)-(long)(sum2->flags&FLAG_HAS_ATT);
	return (res);
}

/************************************************************************
 * SumSelectCompare - compare two summaries based on whether or not they are selected
 ************************************************************************/
int SumSelectCompare(MSumPtr sum1, MSumPtr sum2)
{
	long res = (long)(sum1->spareShort)-(long)(sum2->spareShort);

	//	Check for sentinel first
	if (sum1->length==kLargeUniqueValue) return 1;
	if (sum2->length==kLargeUniqueValue) return -1;

	if (!res) res = sum1->spareShort2-sum2->spareShort2;
	return (res);
}

/**********************************************************************
 * SumStatCompare - compare two messages based on state
 **********************************************************************/ 
int SumStatCompare(MSumPtr sum1, MSumPtr sum2)
{
	static Byte table[] = {1,2,3,4,5,6,8,7,10,11,9,12,13,14,15,16,17};
	short	state1,state2;
	
	//	Check if state is 255 (sentinel) or otherwise outside range or table
	state1 = sum1->state==255 ? 255 : (sum1->state<0 || sum1->state>=sizeof(table)) ? 100 : table[sum1->state];
	state2 = sum2->state==255 ? 255 : (sum2->state<0 || sum2->state>=sizeof(table)) ? 100 : table[sum2->state];
	return state1-state2;
}

/**********************************************************************
 * SumPriorCompare - compare two messages by priority
 **********************************************************************/
int SumPriorCompare(MSumPtr sum1, MSumPtr sum2)
{
	short p1,p2,res;
	p1 = sum1->priority;
	p2 = sum2->priority;
	if (!p1) p1=Display2Prior(3);
	if (!p2) p2=Display2Prior(3);
	res = p1-p2;
	return (res);
}

/**********************************************************************
 * SumJunkCompare - compare two messages by junk score
 **********************************************************************/
int SumJunkCompare(MSumPtr sum1, MSumPtr sum2)
{
	return sum1->spamScore-sum2->spamScore;
}

/**********************************************************************
 * reverses of all of the above
 **********************************************************************/
int RevSumSizeCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumSizeCompare(sum1,sum2));}
int RevSumLabelCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumLabelCompare(sum1,sum2));}
int RevSumAnalCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumAnalCompare(sum1,sum2));}
int RevSumAttCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumAttCompare(sum1,sum2));}
int RevSumStatCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumStatCompare(sum1,sum2));}
int RevSumPriorCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumPriorCompare(sum1,sum2));}
int RevSumJunkCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumJunkCompare(sum1,sum2));}
int RevSumFromCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumFromCompare(sum1,sum2));}
int RevSumSubjCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumSubjCompare(sum1,sum2));}
int RevSumTimeCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumTimeCompare(sum1,sum2));}
int RevSumMailboxCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumMailboxCompare(sum1,sum2));}
int RevSumOffsetCompare(MSumPtr sum1, MSumPtr sum2) {return(-SumOffsetCompare(sum1,sum2));}

/************************************************************************
 * SortTOC - sort a toc, given a comparison function
 ************************************************************************/
void SortTOC(TOCHandle tocH, Boolean reverse, int (*compare)())
{
	MSumPtr sums, sPtr;
	MSumType	sentinel;
	int count = (*tocH)->count;
		
	SetHandleBig_(tocH,GetHandleSize_(tocH)+sizeof(MSumType));
	if (MemError()) {WarnUser(MEM_ERR,MemError()); return;}
	sums=LDRef(tocH)->sums;
	
	//	Set up sentinel
	Zero(sentinel);
	InfiniteString(sentinel.from,sizeof(sentinel.from));
	InfiniteString(sentinel.subj,sizeof(sentinel.subj));
	sentinel.seconds = 0xffffffff;
	sentinel.length = kLargeUniqueValue;
	sentinel.state = 255;
	sentinel.flags = 0xffffffff-FLAG_SKIPPED;
	sentinel.opts = 0xffffffff-OPT_JUSTSUB;
	sentinel.priority = 255;
	sentinel.spareShort = 0x7fff;
	sentinel.subjId = 0x7fff;
	sentinel.flags |= FLAG_HAS_ATT;
	sentinel.selected = True;
	sentinel.spareShort2 = 0x7fff;
	sentinel.u.virtualMess.virtualMBIdx = 0x7fff;
	sentinel.score = 0xf;
	sums[count] = sentinel;
	
	/* tag with original positions */
	for (sPtr=sums;sPtr<sums+(*tocH)->count;sPtr++)
		sPtr->spareShort2=/*reverse*/ 0 ?((*tocH)->count-(sPtr-sums)) : (sPtr-sums);
	
	gSortTOC = tocH;
	QuickSort((void*)sums,sizeof(MSumType),0,count-1,(void*)compare,(void*)SwapSum);
	for (sPtr=sums;sPtr<sums+(*tocH)->count;sPtr++)
		if (sPtr->messH) (*(MessHandle)sPtr->messH)->sumNum = sPtr-sums;

	UL(tocH);
	SetHandleBig_(tocH,GetHandleSize_(tocH)-sizeof(MSumType));
	TOCSetDirty(tocH,true);
}

#pragma segment BoxWidget
/**********************************************************************
 * MessDragSend - drag data proc for messages
 **********************************************************************/
pascal OSErr MessDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag)
{
	OSErr err=cantGetFlavorErr;
	MessHandle messH;
	UHandle data;
	
	if (!(err=MyGetDragItemData(drag,1,MESS_FLAVOR,(void*)&data)))
	{
		messH = **(MessHandle**)data;
		ZapHandle(data);
		err = BoxMessDragSendLo(flavor,dragSendRefCon,drag,(*messH)->tocH,(*messH)->sumNum);
	}
	return(err);
}

/**********************************************************************
 * BoxDragSend - drag data proc for mailboxes
 **********************************************************************/
pascal OSErr BoxDragSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag)
{
	OSErr err=cantGetFlavorErr;
	FSSpecPtr spec = (FSSpecPtr)dragSendRefCon;
	TOCHandle tocH;
	UHandle data;
	
	if (!(err=MyGetDragItemData(drag,1,TOC_FLAVOR,(void*)&data)))
	{
		tocH = **(TOCHandle**)data;
		ZapHandle(data);
		err = BoxMessDragSendLo(flavor,dragSendRefCon,drag,tocH,FirstMsgSelected(tocH));
	}
	return(err);
}

/**********************************************************************
 * BoxMessDragSendLo - common stuff for message & summary dragging
 **********************************************************************/
OSErr BoxMessDragSendLo(FlavorType flavor, void *dragSendRefCon, DragReference drag, TOCHandle tocH,short sumNum)
{
	AEDesc dropLocation;
	OSErr err=cantGetFlavorErr;
	FSSpecPtr spec = (FSSpecPtr)dragSendRefCon;
	UHandle addresses=nil;
		
	/*
	 * return the filename
	 */
	if (flavor==SPEC_FLAVOR)
	{
		NullADList(&dropLocation,nil);
		if (!(err=GetDropLocation(drag,&dropLocation)))
		if (!(err=GetDropLocationDirectory(&dropLocation,&spec->vRefNum,&spec->parID)))
		{
			MakeMessFileName(tocH,sumNum,spec->name);
			UniqueSpec(spec,MAX_BOX_NAME);
			FSpCreateResFile(spec,CREATOR,'TEXT',smSystemScript);
			WhackFinder(spec);
			if (!(err = ResError()))
				err = MySetDragItemFlavorData(drag,1,SPEC_FLAVOR,spec,sizeof(FSSpec));
		}
		DisposeADList(&dropLocation,nil);
	}
	else if (flavor==A822_FLAVOR)
	{
		if (!MyGetDragItemData(drag,1,MESS_FLAVOR,nil))
			err = GatherBoxAddresses(tocH,DragOrMods(drag),sumNum,sumNum,&addresses,true);
		else
			err = GatherBoxAddresses(tocH,DragOrMods(drag),-1,-1,&addresses,true);
		if (!err)
		{
			CommaList(addresses);
			MoveHHi(addresses);
			err = MySetDragItemFlavorData(drag,1,A822_FLAVOR,LDRef(addresses),GetHandleSize(addresses));
			ZapHandle(addresses);
		}
	}
	return(err);
}

/************************************************************************
 * DragXfer - drag messages from one mailbox to the next
 ************************************************************************/
void DragXfer(Point pt, TOCHandle tocH, MessHandle messH)
{
	RgnHandle dragRgn = tocH ? BoxBuildDragRgn(tocH) : MessBuildDragRgn(messH);
	DragReference drag;
	MyWindowPtr win = tocH ? (*tocH)->win : (*messH)->win;
	OSErr err;
	PromiseHFSFlavor promise;
	FSSpec spec;
	Boolean trash = False;
	FSSpecHandle handle=nil;
	FSSpec moveSpec;
	FSSpecPtr specPtr = &moveSpec;	
	DECLARE_UPP(MessDragSend,DragSendData);
	DECLARE_UPP(BoxDragSend,DragSendData);
	
	INIT_UPP(MessDragSend,DragSendData);
	INIT_UPP(BoxDragSend,DragSendData);
	if (dragRgn)
	{

		Zero(spec);
		Zero(moveSpec);
		SetMyCursor(arrowCursor); SFWTC = True;
		/*
		 * create a drag
		 */
		if (!MyNewDrag(win,&drag))
		{
			OutlineRgn(dragRgn,1);
			GlobalizeRgn(dragRgn);
			Zero(promise);
			promise.fileType = 'TEXT';
			promise.fileCreator = CREATOR;
			promise.promisedFlavor = SPEC_FLAVOR;
			
			if (messH) DragTOCSource = (*(*messH)->tocH)->win;	// real source for data
			else DragTOCSource = (*tocH)->win;

			gDrawerOpenedWin = nil;
						
			if (!(err=SetDragSendProc(drag,tocH?BoxDragSendUPP:MessDragSendUPP,(void*)&spec)))
			if (!(err=AddDragItemFlavor(drag,1,flavorTypePromiseHFS,&promise,sizeof(promise),0)))
			if (!(err=AddDragItemFlavor(drag,1,'m822',nil,0,flavorNotSaved)))
			if (!tocH || !(err=AddDragItemFlavor(drag,1,TOC_FLAVOR,&tocH,sizeof(tocH),flavorSenderOnly|flavorNotSaved)))
			if (!messH || !(err=AddDragItemFlavor(drag,1,MESS_FLAVOR,&messH,sizeof(messH),flavorSenderOnly|flavorNotSaved)))
			if (!(err=AddDragItemFlavor(drag,1,A822_FLAVOR,nil,0,0)))
			if (!(err=AddDragItemFlavor(drag,1,SPEC_FLAVOR,nil,0,0)))
			if (!(err=AddDragItemFlavor(drag,1,'euXX',&specPtr,sizeof(FSSpecPtr),flavorSenderOnly|flavorNotSaved)))
			{
				// MyTrackDrag can close the window being dragged.  Save information about the message we may need after the drag ...
				FSSpec sourceSpec = tocH ? GetMailboxSpec(tocH,-1) : GetMailboxSpec((*messH)->tocH,-1);
				TOCHandle messToc = messH ? (*messH)->tocH : nil;
				short messSum = messH ? (*messH)->sumNum : 0;
					
				if (!MyTrackDrag(drag,&MainEvent,dragRgn))
				{
					trash = DragTargetWasTrash(drag);
					MyDisposeDrag(drag); drag=nil;
					if (*moveSpec.name)
					{
						spec = moveSpec;
						trash = !(CurrentModifiers()&optionKey);
					}
							
					if (*spec.name && !SameSpec(&sourceSpec,&spec))
					{
						if (tocH) MoveSelectedMessages(tocH,&spec,!trash);
						else
						{
							if (trash)
							{
								AddXfUndo(messToc,TOCBySpec(&spec),messSum);
								EzOpen(messToc,messSum,0,MainEvent.modifiers,True,True);
							}
							MoveMessage(messToc,messSum,&spec,!trash);
						}
					}
				}
			}
			if (drag) MyDisposeDrag(drag);
			if (gDrawerOpenedWin == win)
			{
				// Opened the mailbox drawer for drag. Close it.
				MBDrawerToggle(win);
				gDrawerOpenedWin = nil;
			}
		}
		DisposeRgn(dragRgn);
	}
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr BoxDragHandler(MyWindowPtr win,DragTrackingMessage which,DragReference drag)
{
	OSErr err=noErr;
	RgnHandle rgn;
	ControlHandle cntl;
	Rect	r;
	Point mouse;
	
	if (!DragIsInteresting(drag,TOC_FLAVOR,MESS_FLAVOR,nil)) return(dragNotAcceptedErr);
	if (DragTOCSource==win)
	{
		if (which ==kDragTrackingInWindow)
		{
			// Dragging within this window
			// If cursor is over mailbox drawer button, open drawer if not already open
			cntl = FindControlByRefCon(win,kDrawerSwitch);
			if (cntl && !GetControlValue(cntl))
			{
				GetControlBounds(cntl,&r);
				GetDragMouse(drag, &mouse, nil);
				GlobalToLocal(&mouse);
				if (PtInRect(mouse,&r))
				{
					MBDrawerToggle(win);
					gDrawerOpenedWin = win;
				}
			}
		}
		return(dragNotAcceptedErr);
	}

	SetPort_(GetMyWindowCGrafPtr(win));
	
	switch (which)
	{
		case kDragTrackingEnterWindow:
			if (rgn=NewRgn())
			{
				RectRgn(rgn,&win->contR);
				OutlineRgn(rgn,2);
				ShowDragHilite(drag,rgn,True);
				DisposeRgn(rgn);
			}
			err = noErr;
			break;
		case kDragTrackingLeaveWindow:
			HideDragHilite(drag);
			err = noErr;
			break;
		case 0xfff:
			err = BoxDrop(win,drag);
			break;
	}
	
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr BoxDrop(MyWindowPtr win, DragReference drag)
{
	TOCHandle winTocH = (TOCHandle) GetMyWindowPrivateData(win);
	FSSpec spec = GetMailboxSpec(winTocH,-1);
	TOCHandle tocH;
	short sumNum;
	OSErr err;
	UHandle data=nil;
	
	if (!(err=MyGetDragItemData(drag,1,MESS_FLAVOR,(void*)&data)))
	{
		tocH = (***(MessHandle**)data)->tocH;
		sumNum = (***(MessHandle**)data)->sumNum;
	}
	else if (!(err=MyGetDragItemData(drag,1,TOC_FLAVOR,(void*)&data)))
	{
		tocH = **(TOCHandle**)data;
		sumNum = -1;
	}
	ZapHandle(data);
	
	if (!err)
	{
		DragSumNum = sumNum;
		DragTOCFrom = tocH;
		DragTOCTo = (TOCHandle) winTocH;
		DragModsWere = DragOrMods(drag);
	}
	return(err);
}

/************************************************************************
 * FinishBoxDrag - finish a drag outside of the actual drag mgr
 ************************************************************************/
OSErr FinishBoxDrag(void)
{
	FSSpec spec = GetMailboxSpec(DragTOCTo,-1);
	
	if (DragSumNum==-1)
		MoveSelectedMessages(DragTOCFrom,&spec,(DragModsWere&optionKey)!=0);
	else
	{
		if (!(DragModsWere&optionKey))
		{
			AddXfUndo(DragTOCFrom,DragTOCTo,DragSumNum);
			EzOpen(DragTOCFrom,DragSumNum,0,DragModsWere,True,True);
		}
		MoveMessage(DragTOCFrom,DragSumNum,&spec,(DragModsWere&optionKey)!=0);
	}
	DragTOCFrom = nil;
	return noErr;
}

/************************************************************************
 * TOCHUnder - what tocH is under a point?
 ************************************************************************/
TOCHandle TOCHUnder(Point pt)
{
	WindowPtr theWindow;
	
	if (FindWindow(pt,&theWindow) &&
			(GetWindowKind(theWindow)==MBOX_WIN ||
			 GetWindowKind(theWindow)==CBOX_WIN))
	{
		return((TOCHandle)GetWindowPrivateData(theWindow));
	}
	return(nil);
}


/************************************************************************
 * BoxBuildDragRgn - drag messages from one mailbox to the next
 ************************************************************************/
RgnHandle BoxBuildDragRgn(TOCHandle tocH)
{
	RgnHandle theRgn = NewRgn();
	short mNum;
	Rect r, sumR;
	MyWindowPtr win = (*tocH)->win;
	short minVis, maxVis;
	
	if (theRgn)
	{
		OpenRgn();
		SetRect(&r,win->contR.left,0,win->contR.right,win->vPitch);
		minVis = GetControlValue(win->vBar);
		OffsetRect(&r,0,-minVis*win->vPitch);
		maxVis = minVis + (win->contR.bottom-win->contR.top)/win->vPitch+1;
		maxVis = MIN(maxVis,(*tocH)->count-1);
		
		for (mNum=minVis;mNum<=maxVis;mNum++)
		{
			if ((*tocH)->sums[mNum].selected)
			{
				sumR = r;
				OffsetRect(&sumR,0,win->topMargin+mNum*win->vPitch);
				FrameRect(&sumR);
			}
		}
		
		CloseRgn(theRgn);
	}
	return(theRgn);
}
/**********************************************************************
 * BoxClick - handle a click in the content region of a window
 **********************************************************************/
void BoxClick(MyWindowPtr win,EventRecord *event)
{
	Point pt;
	short startNum,endNum,mNum;
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	TOCHandle tocH = (TOCHandle)GetWindowPrivateData(winWP);
	Boolean cmd,shift,opt,ctl;
	short which;
	ControlHandle cntl;
	Boolean clickOnSelected;
	
	pt = event->where;
	SetPort_(GetMyWindowCGrafPtr(win));
	GlobalToLocal(&pt);
	
	(*tocH)->userActive = true;
		
	if (win->isActive)
	{
		if (BoxDragDividers(pt,win)) return;
		if ((*tocH)->previewPTE && PtInPETE(pt,(*tocH)->previewPTE) && PeteLen((*tocH)->previewPTE))
		{
			PeteEditWFocus(win,(*tocH)->previewPTE,peeEvent,(void*)event,nil);
			if (win->pte) BoxListFocus(tocH,false);
			if ((*tocH)->drawerWin) MBDrawerSetFocus((*tocH)->drawerWin, false);
			if ((*tocH)->previewID && (mNum=FirstMsgSelected(tocH))>=0 && PreviewReadFocus)
				BeenThereDoneThat(tocH,-1);

			return;
		}
		else if ((cntl=FindControlByRefCon(win,PREVIEW_DIVIDE_CNTL)) && PtInControl(pt,cntl))
		{
			BoxDragPreviewDivider(tocH,pt);
			return;
		}
		else if ((cntl=FindControlByRefCon(win,'wide'+blServer)) && PtInControl(pt,cntl))
			return;	// ignore clicks
		else if ((cntl=FindControlByRefCon(win,kBoxSizeRefCon)) && PtInControl(pt,cntl) && (*tocH)->virtualTOC)
			return;	// ignore clicks in box size button for virtual mailboxes
		else if ((cntl=FindControlByRefCon(win,kConConProfRefCon)) && PtInControl(pt,cntl))
		{
			BoxConConProfControl(tocH,cntl,pt);
			return;	// ignore clicks in box size button for virtual mailboxes
		}
		else if (HandleControl(pt,win))
			return;
	}

	if (!PtInRect(pt,&win->contR))
	{
		if (!win->isActive) SelectWindow_(winWP);
		(*tocH)->userActive = true;
		return;
	}

	shift = (event->modifiers & shiftKey) != 0;
	cmd = (event->modifiers & cmdKey) != 0;
	opt = (event->modifiers & optionKey) != 0;
	ctl = (event->modifiers & controlKey) != 0;
	mNum = (pt.v-win->topMargin)/win->vPitch + GetControlValue(win->vBar);
	clickOnSelected = mNum<(*tocH)->count && (*tocH)->sums[mNum].selected;
	
	if (!win->isActive && !DragPrefSumImmediate && !ctl && !clickOnSelected)
	{
		SelectWindow_(winWP);
		return;
	}

	if (win->isActive)
	{
		PeteFocus(win,nil,true);
		BoxListFocus(tocH,true);
		if ((*tocH)->drawerWin) MBDrawerSetFocus((*tocH)->drawerWin, false);

		/*
		 * menus in to summary
		 */
		if (!cmd && !opt && !shift && ClickType==Single && clickOnSelected)
		{
			BoxFindDivider(pt,&which);
			if (which<=blAttach || which==blLabel || which==blServer || which==blMailbox)
			{
				EnableMenuItems(False);
				SetMenuTexts(0,False);
				SetMyCursor(arrowCursor);
				if (which==blLabel)
					BoxLabelMenu(tocH,mNum,nil,pt);
				else 
				if (which==blPrior)
					BoxPriorMenu(tocH,mNum,pt);
				else if (which==blAttach)
					BoxAttachMenu(tocH,mNum,pt);
				else if (which==blStat)
					BoxStateMenu(tocH,mNum,pt);
				else if (which==blServer)
					BoxServerMenu(tocH,mNum,pt);
				else if (which==blMailbox)
					BoxMailboxMenu(tocH,mNum,pt);
				return;
			}
		}
	}
	
	/*
	 * drag xfer
	 */
	if (!shift && !cmd && clickOnSelected)
	{
		if (MyWaitMouseMoved(event->where,True))
		{
			DragXfer(pt,tocH,nil);
			(*tocH)->userActive = true;
			return;
		}
		PeteFocus(win,nil,true);
		BoxListFocus(tocH,true);
	}
		
	if (shift)
	{
		for (startNum=0;startNum<(*tocH)->count;startNum++)
			if ((*tocH)->sums[startNum].selected) break;
		if (startNum < (*tocH)->count)
		{
		  if (startNum < mNum) endNum = mNum;
			else
			{
				for (endNum=startNum+1;endNum<(*tocH)->count;endNum++)
					if (!(*tocH)->sums[endNum].selected) break;
				endNum--;
				if (!(*tocH)->sums[endNum].selected) endNum = startNum;
				startNum = mNum;
			}
		}
		else startNum=endNum=mNum;
	}
	else startNum=endNum=mNum;

	SelectBoxRange(tocH,startNum,endNum,cmd,-1,-1);
	(*tocH)->userActive = true;
	if (mNum<(*tocH)->count && (*tocH)->sums[mNum].selected)
	{
		if (!shift && !cmd && DragPrefSumImmediate && MyWaitMouseMoved(event->where,True))
		{
			DragXfer(pt,tocH,nil);
			(*tocH)->userActive = true;
			return;
		}
	}

	if (!win->isActive) {SelectWindow_(winWP);}
	else
	{
		EnableMenus(winWP,False);
		if (ClickType==Double)
			DoDependentMenu(winWP,FILE_MENU,FILE_OPENSEL_ITEM,event->modifiers);
		else
		{
			BoxTrack(tocH,mNum==startNum?endNum:startNum,cmd&&!shift);
			if (opt)
			{
				BoxFindDivider(pt,&which);
				BoxSelectSame(tocH,BoxLine2Item(which),mNum);
			}
			(*tocH)->userActive = true;
		}
	}
}

/************************************************************************
 * BoxConConProfControl - Handle clicks in the profile control
 ************************************************************************/
void BoxConConProfControl(TOCHandle tocH,ControlHandle cntl,Point pt)
{
	short item, oldItem;
	Str63 name;
	
	// update the menu
	BoxBuildConConProfMenu(tocH,cntl);
	oldItem = GetBevelMenuValue(cntl);
	
	// let the control manager track
	HandleControl(pt,(*tocH)->win);
	item = GetBevelMenuValue(cntl);
	
	// Now, see what we can see
	if (item!=oldItem)
	{
		MenuHandle mh = (MenuHandle)GetBevelMenu(cntl);
		
		if (mh)
		{
			uLong cmd;
			short section, subItem;
			uLong hash;
			
			// what's the command id?
			GetMenuItemCommandID(mh,item,&cmd);
			section = cmd>>16;
			subItem = cmd&0xffff;
			
			// Figure out what profile was chosen
			switch (subItem)
			{
				case 0:	// Default
					hash = subItem;
					GetRString(name,section==1?CONCON_PREVIEW_PROFILE:CONCON_MULTI_PREVIEW_PROFILE);
					break;
				case CONCON_NONE:	// None
					hash = subItem;
					GetRString(name,subItem);
					break;
				default:	// actual profile
					MyGetItem(mh,item,name);
					hash = Hash(name);
			}
			
			// if section is 1, single selection.
			// if section is 2, multi selection.
			// set the proper one
			if (section==1)
				(*tocH)->singlePreviewProfileHash = hash;
			else
				(*tocH)->multiPreviewProfileHash = hash;
			TOCSetDirty(tocH,true);
			
			// Update the control
			SetControlTitle(cntl,name);
			SetBevelMenuValue(cntl,item);
			
			// Now let somebody else figure out what to do with the selection...
			BoxRefreshPreview(tocH);
		}
	}
}

/************************************************************************
 * BoxBuildConConProfMenu - Build the profile menu for the preview
 ************************************************************************/
void BoxBuildConConProfMenu(TOCHandle tocH,ControlHandle cntl)
{
	MenuHandle mh = (MenuHandle)GetBevelMenu(cntl);
	Str255 itemStr;
	Str63 defaultStr;
	short i;
	short j;
	short n;
	short selected = CountSelectedMessages(tocH);
	uLong myHash;
	ConConProH ccph;
	
	if (!mh) return;	// uh-oh...
	
	TrashMenu(mh,1);
	n = 0;
	
	for (i=1;i<=2;i++) // 1 == single preview, 2 == multi preview
	{
		// Which set of profiles?
		MyAppendMenu(mh,GetRString(itemStr,i==1?CONCON_PROF_SINGLE:CONCON_PROF_MULTI));
		DisableItem(mh,++n);

		// which profile are we using for this section of the menu?
		myHash = i==1?(*tocH)->singlePreviewProfileHash:(*tocH)->multiPreviewProfileHash;
		
		// the default
		GetRString(defaultStr,i==1?CONCON_PREVIEW_PROFILE:CONCON_MULTI_PREVIEW_PROFILE);
		ComposeRString(itemStr,CONCON_PROF_DEFAULT,defaultStr);
		MyAppendMenu(mh,itemStr);
		SetMenuItemCommandID(mh,++n,i<<16);
		if (myHash==0) SetItemMark(mh,n,checkMark);
		EnableIf(mh,n,i==1 && selected==1 || i==2 && selected>1);
				
		// None
		MyAppendMenu(mh,GetRString(itemStr,CONCON_NONE));
		SetMenuItemCommandID(mh,++n,i<<16|CONCON_NONE);
		if (myHash==CONCON_NONE) SetItemMark(mh,n,checkMark);
		EnableIf(mh,n,i==1 && selected==1 || i==2 && selected>1);

		// if the default for this mailbox isn't done already,
		// grab a copy of its name here
		if (myHash!=CONCON_NONE && myHash!=0 && (ccph=ConConProFindHash(myHash)))
			PSCopy(defaultStr,(*ccph)->name);
		else
			*defaultStr = 0;
		
		// The defined profiles
		ConConAddItems(mh);
		// Now fix up the items
		for (j=CountMenuItems(mh);j>n;j--)
		{
			SetMenuItemCommandID(mh,j,(i<<16)|j);
			EnableIf(mh,j,i==1 && selected==1 || i==2 && selected>1);
			MyGetItem(mh,j,itemStr);
			if (StringSame(itemStr,defaultStr)) SetItemMark(mh,j,checkMark);
		}
		// and reset our count
		n = CountMenuItems(mh);
		
		// separate the types
		if (i==1) 
		{
			n++;
			AppendMenu(mh,"\p-");
		}
	}
}

/************************************************************************
 * BoxDragPreviewDivider
 ************************************************************************/
void BoxDragPreviewDivider(TOCHandle tocH,Point pt)
{
	Rect r, bounds;
	MyWindowPtr tocWin = (*tocH)->win;
	CGrafPtr	winPort = GetMyWindowCGrafPtr(tocWin);
	short hi = 0;
	short fudge = pt.v - tocWin->contR.bottom - GROW_SIZE;
	Rect rPort = *GetPortBounds(winPort,&rPort);
	short totalHi = RectHi(rPort);
	
	SetRect(&r,0,tocWin->contR.bottom,rPort.right,tocWin->contR.bottom+GROW_SIZE);
	bounds.top = tocWin->contR.top + GetRLong(PREVIEW_USELESS)*tocWin->vPitch + GROW_SIZE + fudge;
	bounds.bottom = totalHi - GetRLong(PREVIEW_USELESS)*FontLead - fudge - GROW_SIZE - tocWin->topMargin;
	bounds.left = -REAL_BIG/2;
	bounds.right = REAL_BIG/2;
	
	pt = DragDivider(pt,&r,&bounds,tocWin);
	
	if (pt.v==0)
	{
		GetMouse(&pt);
		if (pt.v > tocWin->contR.bottom) hi = -1;
	}
	else
	{
		hi = totalHi - pt.v - fudge;	// where the user put the line
		hi = totalHi-hi-GROW_SIZE-tocWin->topMargin;	// the room the user left for summaries
		hi = RoundDiv(hi,tocWin->vPitch)*tocWin->vPitch;	// round to nearest summary
		hi = totalHi - tocWin->topMargin - GROW_SIZE - hi;	// back to the preview height
	}
	
	if (hi)
	{
		(*tocH)->previewHi = hi;
		TOCSetDirty(tocH,true);
		MyWindowDidResize(tocWin,nil);
	}
	
	AuditHit(false, false, false, false, false, GetWindowKind(GetMyWindowWindowPtr(tocWin)), PREVIEW_DIVIDE_CNTL, mouseDown);
}

/**********************************************************************
 * BoxLabelMenu - pop up the label menu in a mailbox window
 **********************************************************************/
void BoxLabelMenu(TOCHandle tocH,short mNum,MessHandle messH,Point pt)
{
	short currentLabel = GetSumColor(tocH,mNum);
	MenuHandle mh = GetMHandle(LABEL_HIER_MENU);
	long res;
	MyWindowPtr win;
	
	LocalToGlobal(&pt);
#ifdef REFRESH_LABELS_MENU
	RefreshLabelsMenu();
#endif //REFRESH_LABELS_MENU
	SetItemMark(mh,Label2Menu(currentLabel),checkMark);
	res = MyPopupMenuSelect((*tocH)->win,mh,pt,Label2Menu(currentLabel));
	if (res&0xffff0000)
		if (messH) DoMenu2(GetMyWindowWindowPtr((*messH)->win),LABEL_HIER_MENU,res&0xff,nil);
		else DoMenu2(GetMyWindowWindowPtr((*tocH)->win),LABEL_HIER_MENU,res&0xff,nil);
	
	// Audit the fact that the label menu was used ...
	if (tocH)
	{
	 	win = (*tocH)->win;
	 	if (win)
	 	{
			AuditHit(false, false, false, false, false, GetWindowKind(GetMyWindowWindowPtr(win)), AUDITCONTROLID(GetWindowKind(GetMyWindowWindowPtr(win)),LABEL_HIER_MENU), mouseDown);
		}
	}
}

/**********************************************************************
 * BoxPriorMenu - pop up the priority menu in a mailbox window
 **********************************************************************/
void BoxPriorMenu(TOCHandle tocH,short mNum,Point pt)
{
	short currentPri = Prior2Display((*tocH)->sums[mNum].priority);
	MenuHandle mh = GetMHandle(PRIOR_HIER_MENU);
	long res;
	
	LocalToGlobal(&pt);
	res = MyPopupMenuSelect((*tocH)->win,mh,pt,currentPri);
	if (res&0xffff0000) BoxMenu((*tocH)->win,PRIOR_HIER_MENU,res&0xff,nil);
}

/**********************************************************************
 * BoxServerMenu - pop up the server menu in a mailbox window
 **********************************************************************/
void BoxServerMenu(TOCHandle tocH,short mNum,Point pt)
{
	MenuHandle mh = GetMHandle(SERVER_HIER_MENU);
	long res;
	short item=0;
	
	LocalToGlobal(&pt);

	item = EnableServerItems((*tocH)->win,mNum,false,false);
		
	res = MyPopupMenuSelect((*tocH)->win,mh,pt,item);

	if (res&0xffff0000) BoxMenu((*tocH)->win,SERVER_HIER_MENU,res&0xff,nil);
}

/************************************************************************
 * ServerMenuChoice - choose an item from the server menu
 ************************************************************************/
void ServerMenuChoice(TOCHandle tocH,short mNum,short item,Boolean shiftPressed)
{
	// the server menu looks different for IMAP mailboxes
	if ((*tocH)->imapTOC)
	{	
		switch(item)
		{
			case isvmDelete:
			{
				Handle uids = nil;
				long c =  CountSelectedMessages(tocH);
				long sumNum;
				
				// build a list of uids to be deleted
				uids = NuHandleClear(c*sizeof(unsigned long));
				if (uids)
				{
					for (sumNum=0;sumNum<(*tocH)->count && c;sumNum++)
						if ((*tocH)->sums[sumNum].selected)
							BMD(&((*tocH)->sums[sumNum].uidHash),&((unsigned long *)(*uids))[--c],sizeof(unsigned long));
					
					// and delete them.
					IMAPDeleteMessages(tocH, uids, false, false, (((*tocH)->sums[mNum].opts&OPT_DELETED)!=0), false);
				}
				else
				{
					WarnUser(MemError(), MEM_ERR);
				}	
				break;
			}
			case isvmRemoveCache:
			{
				IMAPRemoveSelectedCachedContents(tocH);
				break;			
			}
			case isvmFetchMessage:
			{
				if (shiftPressed) IMAPRemoveSelectedCachedContents(tocH);
			 	IMAPFetchSelectedMessages(tocH, false);
				break;
			}
			case isvmFetchAttachments:	
			{
				if (shiftPressed) IMAPRemoveSelectedCachedContents(tocH);
				IMAPFetchSelectedMessages(tocH, true);
				break;
			}
		}
	}
	else
	{
		switch(item)
		{
			case svmNone:
				RemTSFromPOPD(DELETE_ID,tocH,mNum);
				RemTSFromPOPD(FETCH_ID,tocH,mNum);
				InvalTocBox(tocH,mNum,blServer);
				if ((*tocH)->sums[mNum].messH) Fix1MessServerArea((*(*tocH)->sums[mNum].messH)->win);
				break;
			case svmFetch:
				RemTSFromPOPD(DELETE_ID,tocH,mNum);
				AddTSToPOPD(FETCH_ID,tocH,mNum,False);
				InvalTocBox(tocH,mNum,blServer);
				if ((*tocH)->sums[mNum].messH) Fix1MessServerArea((*(*tocH)->sums[mNum].messH)->win);
				break;
			case svmDelete:
				RemTSFromPOPD(FETCH_ID,tocH,mNum);
				AddTSToPOPD(DELETE_ID,tocH,mNum,False);
				InvalTocBox(tocH,mNum,blServer);
				if ((*tocH)->sums[mNum].messH) Fix1MessServerArea((*(*tocH)->sums[mNum].messH)->win);
				break;
			case svmBoth:
				AddTSToPOPD(FETCH_ID,tocH,mNum,False);
				AddTSToPOPD(DELETE_ID,tocH,mNum,False);
				InvalTocBox(tocH,mNum,blServer);
				if ((*tocH)->sums[mNum].messH) Fix1MessServerArea((*(*tocH)->sums[mNum].messH)->win);
				break;
		}
	}
}

/************************************************************************
 * EnableServerItems - enable the server menu & return the current item
 ************************************************************************/
short EnableServerItems(MyWindowPtr win,short selectedSum,Boolean all,Boolean shiftPressed)
{
	WindowPtr	winWP = GetMyWindowWindowPtr (win);
	MenuHandle mh = GetMHandle(SERVER_HIER_MENU);
	TOCHandle tocH;
	short mNum;
	Boolean onServer=true, skipped, onDelete, onFetch;
	Boolean lmos = PrefIsSet(PREF_LMOS);
	short item = 0;
	short kind = win ? GetWindowKind (winWP) : 0;
	Boolean imapTOC = false;

	CheckNone(mh);
	
	if (all)
	{
		EnableItem(mh,0);
		for (item=CountMenuItems(mh);item>0;item--)
			EnableItem(mh,item);
		return(0);
	}
	
	if (kind!=MESS_WIN && kind!=MBOX_WIN)
		onServer = false;
	else if (kind==MESS_WIN)
	{
		tocH = (*Win2MessH(win))->tocH;
		mNum = (*Win2MessH(win))->sumNum;
	}
	else
	{
		tocH = (TOCHandle) GetWindowPrivateData (winWP);
		mNum = FirstMsgSelected(tocH);
		if (mNum==-1) onServer = false;
	}

	imapTOC = (kind==MESS_WIN || kind==MBOX_WIN) && (tocH) && ((*tocH)->imapTOC);
	
	if (onServer && !imapTOC)
	{
#ifdef	ENABLE_BASED_ON_FIRST_SELECTED
		if ((*tocH)->sums[mNum].flags & FLAG_OUT) onServer = false;
		else if (!TSOnPOPD(POPD_ID,tocH,mNum)) onServer = false;
		else
		{
			skipped = ((*tocH)->sums[mNum].flags & FLAG_SKIPPED)!=0;
			onFetch = skipped && onServer && TSOnPOPD(FETCH_ID,tocH,mNum);
			onDelete = onServer && TSOnPOPD(DELETE_ID,tocH,mNum);
		}
#else	//ENABLE_BASED_ON_FIRST_SELECTED
		short i, sumNum;
		Boolean on = false, skip = false, fetch = false, delete = false;
		enum {firstMessage = 1, lastMessage, clickedMessage};
		short firstSelected = FirstMsgSelected(tocH);
		short lastSelected = LastMsgSelected(tocH);
				
		// check the first and last selected message, as well as the message clicked on.
		// Note: checking them all will be a significant performance hit.
		onServer = skipped = onFetch = onDelete = false;
		for (i = firstMessage; i <= clickedMessage; i++)
		{
			sumNum = -1;
			switch (i)
			{
				case firstMessage:
					sumNum = firstSelected;
					break;
				
				case lastMessage:
					if (lastSelected != firstSelected) sumNum = lastSelected;
					break;
					
				case clickedMessage:
					if ((lastSelected != selectedSum) && (selectedSum != firstSelected)) sumNum = selectedSum;
					break;
			}
			
			if (sumNum >= 0)
			{
				on = true;
				if ((*tocH)->sums[sumNum].flags & FLAG_OUT) on = false;
				else if (!TSOnPOPD(POPD_ID,tocH,sumNum)) on = false;
				else
				{
					skip = ((*tocH)->sums[sumNum].flags & FLAG_SKIPPED)!=0;
					fetch = skip && on && TSOnPOPD(FETCH_ID,tocH,sumNum);
					delete = on && TSOnPOPD(DELETE_ID,tocH,sumNum);
				}
			}
						
			skipped = skipped || skip;
			onServer = onServer || on;
			onFetch = onFetch || fetch;
			onDelete = onDelete || delete;
		}
#endif	//ENABLE_BASED_ON_FIRST_SELECTED
	}
	
	if (imapTOC)
	{
		short firstItem = shiftPressed ? kisvmLimit : ksvmLimit;
		short lastItem = shiftPressed ? kimsvmLimit : kisvmLimit;;
		
		// put the right item into the server menu
		for (item = firstItem+1; item < lastItem; item++)
			SetItemR(mh, item - firstItem, ServerMenuStrnStrn + item);
		
		EnableItem(mh,0);
			
		if (!FancyTrashForThisPers(tocH)) 
		{
			EnableItem(mh,isvmDelete);
			if (((*tocH)->sums[mNum].opts&OPT_DELETED)!=0) 
				SetItemMark(mh,item=isvmDelete,checkMark);
		}
		else DisableItem(mh,isvmDelete);	

		EnableItem(mh,isvmRemoveCache);
		EnableItem(mh,isvmFetchMessage);
		EnableItem(mh,isvmFetchAttachments);
	}
	else
	{
		// put the right items into the server menu
		for (item = 1; item < ksvmLimit; item++)
			SetItemR(mh, item, ServerMenuStrnStrn + item);

		if (!onServer)
			DisableItem(mh,0);	
		else
		{
			EnableItem(mh,0);
			EnableItem(mh,svmFetch);
			EnableItem(mh,svmBoth);
			EnableItem(mh,svmDelete);

			if (!skipped) {DisableItem(mh,svmFetch); DisableItem(mh,svmBoth);}
		
			if (onFetch)
			{
				if (!lmos || onDelete)
					item = svmBoth;
				else
					item = svmFetch;
			}
			else if (onDelete)
				item = svmDelete;
			else
				item = svmNone;

			if (item) SetItemMark(mh,item,checkMark);
			if (!lmos) DisableItem(mh,svmFetch);
		}
	}
	
	return(item);
}
		

/**********************************************************************
 * BoxStateMenu - pop up the state menu in a mailbox window
 **********************************************************************/
void BoxStateMenu(TOCHandle tocH,short mNum,Point pt)
{
	short newStatus;
		
	newStatus = StatusMenu((*tocH)->win,(*tocH)->sums[mNum].state,pt);
	if (newStatus) BoxMenu((*tocH)->win,STATE_HIER_MENU,Status2Item(newStatus),nil);
}

/**********************************************************************
 * BoxMailboxMenu - pop up a mailbox hierarchy
 **********************************************************************/
void BoxMailboxMenu(TOCHandle tocH,short mNum,Point pt)
{
	short realSum;
	TOCHandle realTOC;
	
	LocalToGlobal(&pt);
	if (realTOC = GetRealTOC(tocH,mNum,&realSum))
	{
		PopupMailboxPath(nil,realTOC,realSum,pt);
	}
}

/**********************************************************************
 * BoxAttachMenu - popup a menu of attachments
 **********************************************************************/
void BoxAttachMenu(TOCHandle tocH,short mNum,Point pt)
{
	long res;
	MenuHandle mh=NewMenu(BOX_ATTACH_MENU,"");
	StackHandle stack = nil;
	CSpec spec;
	Handle text;
	short i;
	FindAttData attData;
	
	if (mh && !StackInit(sizeof(spec),&stack))
	{
		// first, find all the files
		for (mNum=0;mNum<(*tocH)->count;mNum++)
		{
			if ((*tocH)->sums[mNum].selected)
			{
				if ((*tocH)->sums[mNum].opts&OPT_HTML)
				{
					CacheMessage(tocH,mNum);
					if (text=(*tocH)->sums[mNum].cache)
					{
						HNoPurge(text);
						InitAttachmentFinder(&attData,text,false,tocH,&(*tocH)->sums[mNum]);
						while (GetNextAttachment(&attData,&spec.spec))
						{
							spec.count = mNum;
							if ((*stack)->elCount==255) break;
							StackQueue(&spec,stack);
						}
						HPurge(text);
					}
				}
				if ((*tocH)->sums[mNum].flags&FLAG_HAS_ATT)
				{
					CacheMessage(tocH,mNum);
					if (text=(*tocH)->sums[mNum].cache)
					{
						HNoPurge(text);
						InitAttachmentFinder(&attData,text,true,tocH,&(*tocH)->sums[mNum]);
						while (GetNextAttachment(&attData,&spec.spec))
						{
							spec.count = mNum;
							if ((*stack)->elCount==255) break;
							StackQueue(&spec,stack);
						}
						HPurge(text);
					}
				}
			}
			if ((*stack)->elCount==255) break;
		}
		
		// next, build the menu
		if ((*stack)->elCount)
		{
			for (i=0;!StackItem(&spec,i,stack);i++)
			{
				MyAppendMenu(mh,spec.spec.name);
				if (FileTypeOf(&spec.spec)==MIME_FTYPE) DisableItem(mh,CountMenuItems(mh)); 	// currently dangerous to open EMSAPI docs; FIX THIS
			}
			
			// now pop it up
			InsertMenu(mh,-1);
			LocalToGlobal(&pt);
			res = MyPopupMenuSelect((*tocH)->win,mh,pt,0);
			DeleteMenu(GetMenuID(mh));
			
			// and act
			if (res&0xffff0000)
			{
				i = res&0xff;
				if (!StackItem(&spec,i-1,stack))
				{
					// if the user selected an IMAP attachment stub ...
					if (IsIMAPAttachmentStub(&spec.spec))
					{
						// and it can be fetched ...
						if (CanFetchAttachment(&spec.spec)) 
						{
							// get it.
							short	realSum;
							
							DownloadIMAPAttachment(&spec.spec, TOCToMbox(GetRealTOC(tocH,spec.count,&realSum)), false);
						}
					}
					else
					{
						if (!OpenOtherDoc(&spec.spec,0!=(MainEvent.modifiers&controlKey),false,nil))
						BeenThereDoneThat(tocH,spec.count);
					}
				}
			}
		}
		
		ZapHandle(stack);
	}
	
	if (mh) DisposeMenu(mh);
}

/**********************************************************************
 * StatusMenu - popup the status menu
 **********************************************************************/
short StatusMenu(MyWindowPtr win,short origStatus,Point where)
{
	long res;
	MenuHandle mh;
	short item = Status2Item(origStatus);
	
	CheckState(win,!win,origStatus);
	
	mh = GetMHandle(STATE_HIER_MENU);
	LocalToGlobal(&where);
	res = MyPopupMenuSelect(win,mh,where,item);
	if (res&0xffff0000) return(Item2Status(res&0xff));
	else return(0);
}

/**********************************************************************
 * MyPopupMenuSelect - popup a menu adjusting the position
 **********************************************************************/
long MyPopupMenuSelect(MyWindowPtr win,MenuHandle mh,Point where,short item)
{
	short	left;
	GDHandle	gd;
	Rect	screenRect, windRect;
	Boolean	hasMB;

	//	Adjust left position to point within menu
	left = where.h-2*MenuWidth(mh)/3;
	
	//	Make sure it's still on the same screen
	utl_GetWindGD(GetMyWindowWindowPtr(win),&gd,&screenRect,&windRect,&hasMB);
	if (left < screenRect.left)
		left = screenRect.left;
	
	return AFPopUpMenuSelect(mh,where.v,left,item);
}

 /************************************************************************
 * BoxTrack - track the mouse in a mailbox window
 ************************************************************************/
void BoxTrack(TOCHandle tocH,int anchor,Boolean cmd)
{
	MyWindowPtr win = (*tocH)->win;
	Point pt;
	int lastSpot=anchor;
	int spot;
	int vVal,vMin,vMax,topVis,botVis;
	long lastTicks=0;
	
	while (StillDown())
		if (TickCount()-lastTicks>GetRLong(SCROLL_ARROW_THROTTLE))
		{
			lastTicks = TickCount();
			vVal = GetControlValue(win->vBar);
			vMin = GetControlMinimum(win->vBar);
			vMax = GetControlMaximum(win->vBar);
			topVis = vVal;
			botVis = (*tocH)->count - (vMax-vVal+1);
			GetMouse(&pt);
			spot = vVal + (pt.v-win->topMargin)/(int)win->vPitch;
			if (spot<vMin) spot=vMin;
			else if (spot>=(*tocH)->count) spot=(*tocH)->count-1;
			if (spot<topVis &&	vVal>vMin || spot>botVis && vVal<vMax)
			{
				if (spot<topVis)
					ScrollIt(win,0,topVis-spot);
				else
					ScrollIt(win,0,botVis-spot);
			}
			if (spot!=lastSpot)
			{
				if (!cmd)
					SelectBoxRange(tocH,anchor,spot,cmd,0,0);
				else
				{
					SelectBoxRange(tocH,anchor,spot,cmd,anchor,lastSpot);
					SelectBoxRange(tocH,anchor,lastSpot,cmd,anchor,spot);
				}
				lastSpot = spot;
			}
		}
}

/************************************************************************
 * SizeBoxClick - handle click in window's size box
 ************************************************************************/
void SizeBoxClick(MyWindowPtr win,short modifiers)
{
	FSSpec spec;
	TOCHandle tocH = (TOCHandle)GetMyWindowPrivateData(win);
	
	if ((*tocH)->virtualTOC) return;
	
	if (modifiers&optionKey)
	{
		CInfoPBRec hfi;
		Str255 scratch;

		GetRString(scratch,TOC_SUFFIX);
		DoCompact(MailRoot.vRef,MailRoot.dirId,&hfi,*scratch);
		
		if (IMAPExists())
			DoCompact(IMAPMailRoot.vRef,IMAPMailRoot.dirId,&hfi,*scratch);
	}
	else if (modifiers&cmdKey && !AnalDisabled())
	{
		short sumNum;
		for (sumNum=0;sumNum<(*tocH)->count;sumNum++)
			(*tocH)->sums[sumNum].score = 0;
		AnalBox(tocH,0,(*tocH)->count-1);
	}
	else if (modifiers&shiftKey)
	{
		// if this is an imap mailbox, clean it up.
		if ((*tocH)->imapTOC)
		{
			short count;
			for (count=0;count<(*tocH)->count;count++)
			{
				if ((*tocH)->sums[count].uidHash==0) DeleteSum(tocH,count);
				else count++;
			}
		}
		else if ((*tocH)->which==JUNK)
		{
			JunkRescanJunkMailbox ();
			ArchiveJunk ( tocH );
		}
		else
			JunkRescanBox ( tocH );
	}
	else
	{
		// if this is an IMAP toc, expunge deleted messages rather than compacting the mailbox.
		if ((*tocH)->imapTOC) IMAPRemoveDeletedMessages(tocH);
		else
		{
			spec = GetMailboxSpec(tocH,-1);
			CompactMailbox(&spec,false);
		}
	}
}

/************************************************************************
 * BeenThereDoneThat - mark this message as read
 ************************************************************************/
void BeenThereDoneThat(TOCHandle tocH,short sumNum)
{
	if (sumNum<0)
	{
		for (sumNum=(*tocH)->count-1;sumNum>=0;sumNum--)
			if ((*tocH)->sums[sumNum].selected)
				BeenThereDoneThat(tocH,sumNum);
	}
	else
	{
		if ((*tocH)->sums[sumNum].state==UNREAD) SetState(tocH,sumNum,READ);

		if ((*tocH)->virtualTOC)
		{
			// Virtual mailbox, do original also
			TOCHandle	realTOC;
			short		realSum;

			if (realTOC = GetRealTOC(tocH,sumNum,&realSum))
			if ((*realTOC)->sums[realSum].state==UNREAD)
				SetState(realTOC,realSum,READ);
		}
	}
}

/************************************************************************
 * AnalBox - process all the mail in a mailbox for anality
 ************************************************************************/
void AnalBox(TOCHandle tocH,short first, short last)
{
	short sumNum;
	if (first<0) first = 0;
	if (last<0 || last>=(*tocH)->count) last = (*tocH)->count-1;
	
	for (sumNum=first;sumNum<=last;sumNum++)
	{
		CycleBalls();
		OneAnalMess(tocH,sumNum);
	}
}

/************************************************************************
 * OneAnalMess - process a single message for anality
 ************************************************************************/
void OneAnalMess(TOCHandle tocH,short sumNum)
{
	short score;
	Boolean inHeader;
	
	if (!(*tocH)->sums[sumNum].score)
	{
#ifdef DEBUG
		if (RunType!=Production)
		{
			Str63 franklin;
			PSCopy(franklin,(*tocH)->sums[sumNum].from);
			if (EqualStrRes(franklin,FRANKLIN))
			{
				SetTAEScore(tocH,sumNum,5);
				return;
			}
		}
#endif
		CacheMessage(tocH,sumNum);
		if ((*tocH)->sums[sumNum].cache)
		{
			inHeader = true;
			score = AnalScanHandle((*tocH)->sums[sumNum].cache,0,-1,&inHeader);
			if (score>=0)
				SetTAEScore(tocH,sumNum,score+1);
		}
	}
}


/************************************************************************
 * BoxInversionSetup - setup the inversion matrix for mailbox columns
 ************************************************************************/
void BoxInversionSetup(void)
{
	Str255 scratch;
	Str15 word;
	UPtr spot;
	short i;
	long col;

	GetRString(scratch,TOC_INVERSION_MATRIX);
	for (i=1,spot=scratch+1;PToken(scratch,word,&spot," ");i++)
	{
		StringToNum(word,&col);
		TOCInversionMatrix[0][i] = col;
		TOCInversionMatrix[1][col] = i;
	}
}

/**********************************************************************
 * HitTab - clicked a tab
 **********************************************************************/
void HitTab(MyWindowPtr win,ControlHandle tabCntl,TOCHandle tocH)
{
	Boolean	fileView;
	
	fileView = GetControlValue(tabCntl)==2 ? true : false;
	if (fileView != (*tocH)->fileView) TOCSetDirty(tocH,true);
	(*tocH)->fileView = fileView;
	SetFileView(win,tocH, GetControlValue(tabCntl)==2 ? true : false);
}

/**********************************************************************
 * SetFileView - enable/disable fileview
 **********************************************************************/
void SetFileView(MyWindowPtr win,TOCHandle tocH, Boolean fileView)
{
	ControlHandle	cntl,placard;
	short			i;
	Boolean			boxCtlsVisible;
	PETEHandle		previewPTE;
	Rect 			oldContR = win->contR;
	FileViewHandle	fvh = GetFileViewInfo(win);
	RgnHandle   saveRgn;
	
	boxCtlsVisible = !fileView;

   saveRgn = SavePortClipRegion(GetMyWindowCGrafPtr(win));
	SetEmptyClipRgn(GetMyWindowCGrafPtr(win));		
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
	if (previewPTE = (*tocH)->previewPTE)
	{
		//	make preview pane visible/invisible by moving it
		Rect	r;
		short	offset;
		
		PeteRect(previewPTE,&r);
		offset = boxCtlsVisible ? 4000 : -4000;
		OffsetRect(&r,offset,offset);
		PeteDidResize(previewPTE,&r);
	}
	
	if (fileView)
	{
		(*fvh)->savePreviewHi = (*tocH)->previewHi;
		SetBotMargin(win,0);
		(*tocH)->previewHi = -1;
		win->update = FileViewUpdate;
		win->click = FileViewClick;
		win->bgClick = FileViewClick;
		win->cursor = FileViewCursor;
		win->drag = FileViewDragHandler;
		win->key = FileViewKey;
		win->find = FileViewFind;
		win->menu = FileViewMenu;
		win->activate = FileViewActivate;
		win->idle = FileViewIdle;
	}
	else
	{
		(*tocH)->previewHi = (*fvh)->savePreviewHi;
		win->update = BoxUpdate;
		win->click = BoxClick;
		win->bgClick = BoxClick;
		win->cursor = BoxCursor;
		win->drag = BoxDragHandler;
		win->key = BoxKey;
		win->find = BoxFind;
		win->menu = BoxMenu;
		win->activate = BoxActivate;
		win->idle = BoxIdle;
	}
	
	InvalBotMargin(win);
	SetControlVisibility((*(*fvh)->list->hList)->vScroll,!boxCtlsVisible,true);
	RestorePortClipRegion(GetMyWindowCGrafPtr(win),saveRgn);
	BoxDidResize(win,nil);
//	InvalContent(win);
}


/**********************************************************************
 * BoxPreviewProfile - what's the proper preview profile for this mailbox?
 **********************************************************************/
PStr BoxPreviewProfile(PStr profileName,TOCHandle tocH,short previewTypeID)
{
	ConConProH ccph = nil;
	Str63 name;
	uLong hash = 0;
	ControlHandle cntl;
	
	// In case we find nothing
	if (profileName) *profileName = 0;
	
	// load up the hash from the mailbox
	if (previewTypeID==CONCON_PREVIEW_PROFILE)
		hash = (*tocH)->singlePreviewProfileHash;
	else if (previewTypeID==CONCON_MULTI_PREVIEW_PROFILE)
		hash = (*tocH)->multiPreviewProfileHash;
	
	// look for the hash, unless it's the magic "none" value
	if (hash!=CONCON_NONE)
	{
		if (hash) ccph = ConConProFindHash(hash);
		
		// if we didn't find it, or hash is zero (meaning use the default), look up the default
		if (!ccph) ccph = ConConProFind(GetRString(name,previewTypeID));
		
		// And copy the name we found, if any
		if (ccph && profileName) PCopy(profileName,(*ccph)->name);
	}
	
	// Set the title of the bevel button
	if (cntl=FindControlByRefCon((*tocH)->win,kConConProfRefCon))
	{
		if (profileName && *profileName) SetControlTitle(cntl,profileName);
		else SetControlTitle(cntl,GetRString(name,CONCON_NONE));
	}
	
	return profileName;
}