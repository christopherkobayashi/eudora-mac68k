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

#include "filegraphic.h"
#include "pete.h"
#include <QuickTimeComponents.h>

#ifdef URLACCESS
#include <urlaccess.h>
#else
typedef void* URLReference;
#endif

#if NETSCAPE
#include "netscapeplugins.h"
#endif

#define FILE_NUM 81

/* Copyright (c) 1996 by Qualcomm, Inc. */

#pragma segment FILEGRAPHIC

#ifdef DEBUG
//	Display missing graphic asserts?
//	Don't normally need this. Usually missing graphics are ones that have
//	been deleted by transferring a message to the trash.
//#define MISSING_GRAPHIC_ASSERTS
#endif


#ifdef ATT_ICONS
enum	{ kBorderSize = 2 };
#define kMaxDownloads	4
#define kAttStubIconWd	20
#define kQTYesList	'QTys'
#define kQTNoList	'QTno'

typedef struct GetURLDataStruct **GetURLDataHandle;
typedef struct GetURLDataStruct
{
	GetURLDataHandle	next;
	GetURLDataHandle	duplicate;
	FGIHandle		graphic;
	PETEHandle		pte;
	URLReference	urlRef;
	FSSpec			spec;
	OSStatus 		urlError;
	Boolean			finished;
	Boolean			reschedule;
} GetURLData;

//	Globals
static short gPicRefN;
static Handle	ghSpoolBuf;
static long	gSpoolBufSize,gSpoolBufIdx;
static Handle	gMissingGraphicIconSuite;
static Handle	gBadGraphicIconSuite;
static short	gDownloadCount;
static GetURLDataHandle	gURLDataList;
static QTImageHandle	gQTImageList;
static Boolean	gMovieClick;
static MyWindowPtr	gGetGraphicsWin;

pascal OSErr FileGraphicSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag);
void PeteMakeFileGraphic(PETEHandle pte,FGIHandle newGHndl,FSSpecPtr spec,short maxWidth,short ht,Boolean displayInline);
OSErr PeteFileGraphicHit(PETEHandle pte,FGIHandle graphic,long offset,EventRecord *event);
Rect *FileGraphicIconRect(Rect *iconR, Rect *r);
void FileGraphicDisplay(PETEHandle pte, FGIHandle graphic, long offset);
static PicHandle SpecPicture(FSSpecPtr spec,long maxBytes);
void SpoolPictDisplay(PicHandle picture,FSSpecPtr spec,Rect *r);
Rect *FileGraphicTextRect(Rect *textR, Rect *r);
void FileGraphicAdjustSize(FGIHandle graphic, short width);
HFSFlavor *BuildHFSFlavor(HFSFlavor *hfs, FSSpecPtr spec);
void FileGraphicFancyRgn(PETEHandle pte,FGIHandle graphic,long offset,Point pt,RgnHandle rgn);
Boolean FileGraphicDrag(PETEHandle pte,FGIHandle graphic,long offset,EventRecord *event);
OSErr GetDropSpec(DragReference drag,FSSpecPtr spec);
OSErr FinderTricks(DragReference drag,OSErr dragErr,FSSpecPtr orig);
static void DrawBorder(Boolean doGrey,Rect *r,short thickness,Boolean linkBorder);
static void ImporterDataToHandle(FGIHandle hGraphic);
static CodecType ValidGraphicFile(FGIHandle hGraphic);
static void GetHTMLSpec(FGIHandle graphic,PETEHandle pte,long offset);
static void FileGraphicRect(Rect *r,PETEHandle pte, FGIHandle graphic, long offset, Boolean withHTMLBorder);
static OSErr GetFileGraphicSpec(FGIHandle graphic, FSSpecPtr spec);
static Boolean CanImport(FSSpecPtr spec, GraphicsImportComponent *importer, Component *c, OSType fileType);
static void SetMovieBounds(FGIHandle graphic,Rect *r,PETEHandle pte);
static OSErr OpenQTMovie(FSSpecPtr spec,Movie *theMovie,WindowPtr win,OSType fileType,FGIHandle newGHndl,Boolean active,Boolean justChecking);
static OSErr MakeFileRef(FGIHandle graphic,FSSpecPtr spec);
static void MissingGraphicIcon(FGIHandle graphic,Boolean zapDimensions);
static void BadGraphicIcon(FGIHandle graphic,Boolean zapDimensions);
static OSErr InsertGraphic(PETEHandle pte,FGIHandle graphic);
static void ReleaseFileRef(GFileRefHandle fileRef);
static short GetMaxWidth(PETEHandle pte);
static void SetupMovie(Movie theMovie,FGIHandle newGHndl,Rect *r,Boolean hideController,Handle hFile);
static void MakeURLFileSpec(FGIHandle graphic,PETEHandle pte,FSSpec *spec);
static Boolean GetHTMLGraphic(PETEHandle pte,FGIHandle graphic,Boolean allowDownload,Boolean forceDownload);
static void DisposeURLRef(FGIHandle graphic);
#ifdef URLACCESS
static pascal OSStatus URLNotifyProc(void *userContext, URLEvent event, URLCallbackInfo *callbackInfo);
#endif
static void CheckCacheLimit(void);
static void DisplayDownload(GetURLDataHandle hURLData);
static OSErr BeginDownload(GetURLDataHandle hURLData,URLReference *urlRef);
static Boolean URLAccessIsInstalled(void);
static Boolean CanQTImport(FSSpecPtr spec,OSType fileType);
static Boolean IsImporterFileType(OSType fileType,Handle h);
static Boolean IsImporterFileExt(Handle h,PStr s);
static void DownloadURLFinished(long refCon,OSErr err,DownloadInfo *info);
static GraphicsImportComponent GetImageImporter(FGIHandle graphic);
static pascal OSErr ImportProgress(short message, Fixed completeness, long refcon);
static void URLDeleteFile(GetURLDataHandle hURLData);
static void DisplayFileName(StringPtr s,Rect *rText);
static void DisposeImage(FGIHandle graphic);
static QTImageHandle CanReuseImage(FSSpecPtr spec);
static Ptr FindPNGChunk(uLong chunkType,Handle hData,long *pLength);
void SetupPNGTransparencyLo(GraphicsImportComponent importer,Handle hData);
static CodecType GetGraphicType(GraphicsImportComponent importer);
static Boolean FindPNGTransparency(Handle hData,RGBColor *transColor);
static void DisposeGraphic(FGIHandle graphic);
static Boolean TryQTImport(OSType fType,OSType fCreator);
static Boolean FindQTImporter(ResType rListResType,short rIdx,OSType ftype,OSType fCreator);
pascal void MyGetPic(Ptr dataPtr, short byteCount);
#define IsMovieFile(spec,type) (!OpenQTMovie(spec,nil,nil,type,nil,false,true))
Boolean HTMLGraphicIsObnoxious(FGIHandle graphic);
/************************************************************************
 * PeteFileGraphicStyle - make a pse for a graphic style
 ************************************************************************/
OSErr PeteFileGraphicStyle(PETEHandle pte,FSSpecPtr spec,HTMLGraphicInfo *html,PETEStyleEntryPtr pse,long flags)
{
	FGIHandle gHandle;
	OSErr err=noErr;
	short wide = RectWi((*PeteExtra(pte))->win->contR)-2*GROW_SIZE;
	
	if (gHandle = NuHandleClear(sizeof(FileGraphicInfo)))
	{
		(*gHandle)->pgi.itemProc = FileGraphic;
		(*gHandle)->attachment = (flags&fgAttachment)!=0;
		(*gHandle)->centerInWin = (flags&fgCenterInWindow)!=0;
		(*gHandle)->isEmoticon = (flags&fgEmoticon)!=0;
		if (html)
		{
			if(html->pictResID || html->pictHandle)
			{
				PicHandle picture;
				
				picture = html->pictResID ? GetPicture(html->pictResID) : html->pictHandle;
				if(!picture)
				{
					ZapHandle(gHandle);
					return ((err = ResError()) != noErr) ? err : resNotFound;
				}
				(*gHandle)->pgi.privateType = html->pictResID ? pgtIndexPict : pgtPictHandle;
				(*gHandle)->displayInline = true;
				(*gHandle)->width = (*picture)->picFrame.right - (*picture)->picFrame.left;
				(*gHandle)->height = (*picture)->picFrame.bottom - (*picture)->picFrame.top;
			}
			else
			{
				//	For HTML images we don't have the FSSpec yet. We'll set up for it and get it later
				(*gHandle)->pgi.privateType = pgtHTMLPending;
				(*gHandle)->displayInline = (flags&fgDisplayInline)!=0;
				(*gHandle)->width = 28;
				(*gHandle)->height = 18;
			}
			(*gHandle)->htmlInfo = *html;
			if (html->alt)
			{
				PCopy((*gHandle)->name,html->alt);	//	Any Alt text is here
				(*gHandle)->htmlInfo.alt = (StringPtr)-1L; // Flag for HTML generator
			} else {
				(*gHandle)->htmlInfo.alt = nil;
			}
		}
		if (spec)
			PeteMakeFileGraphic(pte,gHandle,spec,wide,RectHi((*PeteExtra(pte))->win->contR)-2*GROW_SIZE,(flags&fgDisplayInline)!=0);
		else if(!html || html->pictResID != 0)
			(*gHandle)->pgi.wantsEvents = true;
		(*gHandle)->pgi.cloneOnlyText = (flags&fgDontCopyToClip)!=0;
		pse->psStartChar = 0;
		pse->psGraphic = 1;
		pse->psStyle.graphicStyle.graphicInfo = (void*)gHandle;
	}
	else err = MemError();
	
	return(err);
}

/**********************************************************************
 * PeteFileGraphicRange - Called by editor to set up graphic file display
 **********************************************************************/
OSErr PeteFileGraphicRange(PETEHandle pte,long start,long stop,FSSpecPtr spec,long flags)
{
	PETEStyleEntry pse;
	long junk;
	OSErr err=noErr;
	Handle text;
	Str255 old;
	Boolean dirty = PeteIsDirty(pte);
	Boolean winDirty = (*PeteExtra(pte))->win->isDirty;
	short wide = RectWi((*PeteExtra(pte))->win->contR)-2*GROW_SIZE;
	
	Zero(pse);
	PETEGetStyle(PETE,pte,start,&junk,&pse);
	if (!pse.psGraphic)
	{
		if (!(err=PeteFileGraphicStyle(pte,spec,nil,&pse,flags)))
		{
			PeteGetTextAndSelection(pte,&text,nil,nil);
			MakePStr(old,*text+start,stop-start);
			PeteDelete(pte,start,stop);
			**Pslh = pse;
			if (err=PETEInsertTextPtr(PETE,pte,start,old+1,*old,Pslh))
				ZapHandle(pse.psStyle.graphicStyle.graphicInfo);
			PETEMarkDocDirty(PETE,pte,dirty);
			(*PeteExtra(pte))->win->isDirty = winDirty;
		}
		else err = MemError();
	}
	
	return(err);
}

/**********************************************************************
 * FileGraphicChangeGraphic - the graphic has changed
 **********************************************************************/
OSErr FileGraphicChangeGraphic(PETEHandle pte,long offset,FSSpecPtr spec)
{
	OSErr	err = noErr;
	PETEStyleEntry	pse;
	long junk;

	Zero(pse);
	if (!(err = PETEGetStyle(PETE,pte,offset,&junk,&pse)))
	{
		FGIHandle graphic;
		
		if (pse.psGraphic && (graphic = pse.psStyle.graphicStyle.graphicInfo))
		{
			DisposeGraphic(graphic);	// dispose of any previous graphic data
			PeteMakeFileGraphic(pte,graphic,spec,GetMaxWidth(pte),(*graphic)->height,(*graphic)->displayInline);
			PeteRecalc(pte);	// redraw
			PeteUpdate(pte);
		}
		else
			err = -1;
	}
	
	return(err);
}

/**********************************************************************
 * FileGraphic - Callback to handle a graphic that is a file
 **********************************************************************/
pascal OSErr FileGraphic(PETEHandle pte,FGIHandle graphic,long offset,PETEGraphicMessage message,void *data)
{
	OSErr err = noErr;
	Rect r;
	Handle picture=nil;
	Boolean	isSelected;
	short oldResFile = CurResFile();
	FGIHandle graphicClone;
	EventRecord	*event;
	MyWindowPtr	win;
	Boolean	keyEvent;
	short	maxWidth;
	
	PushGWorld();
	
	if ((*graphic)->pgi.privateType == pgtHTMLPending)
		//	Still waiting for the HTML graphic from the "related:" line
		GetHTMLSpec(graphic,pte,offset);
	
	switch (message)
	{
		case peGraphicDraw:	/* data is (Point *) with penLoc at baseline left as a point */
			if (data)
				(*graphic)->urlLink = (((PETEGraphicStylePtr)data)->tsLabel&pLinkLabel) != 0;
			FileGraphicDisplay(pte,graphic,offset);
			break;

		case peGraphicClone:	/* data is (PETEGraphicInfoHandle *) into which to put duplicate */
			//	When user does copy or drag and drop, make copy of graphic
			if (graphicClone = NuHTempOK(sizeof(FileGraphicInfo)))
			{
				// make clone
				FGIPtr	pClone = LDRef(graphicClone);
				*pClone = **graphic;
				pClone->clone = true;
				if (pClone->fileRef)
					(*pClone->fileRef)->count++;
				if (pClone->htmlInfo.absURL)
					HandToHand((Handle *) &pClone->htmlInfo.absURL);
				if (pClone->htmlInfo.pictHandle)
					HandToHand((Handle *) &pClone->htmlInfo.pictHandle);
				WriteZero(&pClone->u,sizeof(pClone->u));
				pClone->pURLInfo = nil;
				pClone->pgi.isSelected = false;
				UL(graphicClone);
			}
			else err = MemError();
			*(FGIHandle*)data = graphicClone;
			break;
		
		case peGraphicInsert:		/* data is nil. Graphic has been inserted into a new document. */
			err = InsertGraphic(pte,graphic);
			break;
			
		case peGraphicTest:	/* data is (Point *) from top left; return errOffsetIsOutsideOfView if not hit */
			//	Can cancel hit by returning errOffsetIsOutsideOfView
			if (ClickType==Double  && (*graphic)->urlLink)
				//	Don't allow Pete to mess up double-click on link
				err = errOffsetIsOutsideOfView;
			else
			{
				Point pt = *(Point*)data;
				Point hiwi;
				
				if (pt.h < 3 || pt.v < 3)  err = errOffsetIsOutsideOfView;
				else
				{
					hiwi.h = (*graphic)->pgi.width;
					hiwi.v = (*graphic)->pgi.height;
					
					if (hiwi.h-pt.h < 3 || hiwi.v-pt.v < 3) err = errOffsetIsOutsideOfView;
				}
			}
			break;
			
		case peGraphicHit:	/* data is (EventRecord *) if mouse down, nil if time to turn off */
			isSelected = (*graphic)->pgi.isSelected;				
			if (data)
			{
				if (!isSelected)
				{
					(*graphic)->pgi.isSelected = True;
				}
			}
			else
			{
				if (isSelected)
				{
					(*graphic)->pgi.isSelected = False;
				}
			}

			if (isSelected != (*graphic)->pgi.isSelected)
			{
				//	Changed selection, redraw
				if ((*graphic)->pgi.privateType == pgtIcon)
					FileGraphicDisplay(pte,graphic,offset);
				else
				{
					Rect	rHilite;

					FileGraphicRect(&rHilite,pte,graphic,offset,true);
					if ((*graphic)->pgi.isSelected)
						DrawBorder(true,&rHilite,kBorderSize,(*graphic)->urlLink);
					else
					{
						InsetRect(&rHilite,-2,-2);
						InvalWindowRect(GetMyWindowWindowPtr((*PeteExtra(pte))->win),&rHilite);
					}
				}
			}
			
			if (data && !(*graphic)->noImage && !(*graphic)->isEmoticon)
				err = PeteFileGraphicHit(pte,graphic,offset,(EventRecord *)data);
			break;
			
		case peGraphicRemove:	/* data is nil; just dispose a copy of graphic */
			if (!(*graphic)->clone)
			{
				GetURLDataHandle	hURLData;
				
				DisposeGraphic(graphic);
				if (hURLData = (GetURLDataHandle)(*graphic)->pURLInfo)
				{
					if (!(*hURLData)->reschedule && !(*hURLData)->finished)
					{
						//	Still downloading a graphic. Abort.
#ifdef URLACCESS
						if (URLAccessIsInstalled())
						{
							URLAbort((*hURLData)->urlRef);
							URLDeleteFile(hURLData);
						}
						else
#endif
							URLDownloadAbort((long)(*hURLData)->urlRef);
					}
					DisposeURLRef(graphic);
				}			
			}
			
			ReleaseFileRef((*graphic)->fileRef);
			ZapHandle((*graphic)->htmlInfo.absURL);
			ZapHandle(graphic);
			break;
			
		case peGraphicResize:	/* data is a (short *) of the max width */
			if ((*graphic)->pgi.privateType == pgtHTMLPending)
			{
				//	The image wasn't sent with the message. Use the "no-image" icon for now.
				MissingGraphicIcon(graphic,(*graphic)->htmlInfo.cid!=0);
#ifdef MISSING_GRAPHIC_ASSERTS
				if ((*graphic)->htmlInfo.cid && RunType!=Production) Dprintf("\pCould't find graphic in parts stack.");
#endif
				
				//	Try to load the image.
				if (!(*graphic)->htmlInfo.cid && !GetHTMLGraphic(pte,graphic,PeteIsInAdWindow(pte) || !(PrefIsSet(PREF_DONT_FETCH_GRAPHICS)||PeteIsJunk(pte)),PeteIsInAdWindow(pte)))
				{
					//	This HTML graphic hasn't been downloaded yet.
					//	Download it when the use clicks on the "GetGraphic" button which is
					//	then disabled.
					(*graphic)->notDownloaded = true;

					//	Make sure when the "GetGraphics" control is created that it is visible
					gGetGraphicsWin = (*PeteExtra(pte))->win;
				}
			}
			else if ((*graphic)->notDownloaded)
			{
				ControlHandle	hCtl = FindControlByRefCon((*PeteExtra(pte))->win,mcGetGraphics);

				if (hCtl && GetControlHilite(hCtl)==255)	//	Download if "GetGraphics" button is now disabled
				{
					GetHTMLGraphic(pte,graphic,true,true);
					(*graphic)->notDownloaded = false;
				}
			}

			maxWidth = *(short*)data;
			//	don't bother calculating size if the maxWidth hasn't changed
			if (maxWidth != (*graphic)->maxWidth)
			{
				(*graphic)->maxWidth = maxWidth;
				FileGraphicAdjustSize(graphic,maxWidth);
			}
			break;
		
		case peGraphicRegion:	/* data is a RgnHandle which is to be changed to the graphic's region */
			//	Used mostly for changing the cursor
			FileGraphicRect(&r,pte,graphic,offset,true);
			RectRgn((RgnHandle)data,&r);
			break;
		
		case peGraphicEvent:	/* data is (EventRecord *) */
			event = (EventRecord *)data;
			win = (*PeteExtra(pte))->win;
			keyEvent = event->what==keyDown || event->what==autoKey;
			if (GetMyWindowWindowPtr(win) != FrontWindow_() && (event->what==mouseDown || keyEvent))
				//	Ignore clicks and keys if window isn't in front
				break;
			
			switch ((*graphic)->pgi.privateType)
			{
				ComponentResult	movieResult;
				MovieController	aController;

				case pgtMovie:
					{
						static Point downSpot;	/* Point of previous mouseDown */
						static long upTicks;		// time of last mouseUp */
						Point localPt = event->where;
						
						SetPort_(GetMyWindowCGrafPtr(win));
						aController = (*graphic)->u.movie.aController;
						GlobalToLocal(&localPt);

						// We have to do our own double-click processing so
						// that we can avoid sending double-clicks to the movie
						// player.
						if (event->what==mouseDown && PtInPETE(localPt,pte))
						{
							long dblTime = GetDblTime();
							long tolerance = GetRLong(DOUBLE_TOLERANCE);
							static long oldWhen;
	
							/*
							 * figure out whether this is a double or triple click,
							 * and update double and triple click times
							 */
							if (event->when-upTicks<dblTime && event->when!=oldWhen)
							{
								int dx = event->where.h - downSpot.h;
								int dy = event->where.v - downSpot.v;
								if (ABS(dx)<tolerance && ABS(dy)<tolerance && !(event->modifiers&cmdKey))
								{
									/* upgrade the click */
									ClickType++;
									if (ClickType > Triple) ClickType = Single;
								}
								else
									ClickType = Single;
							}
							else
								ClickType = Single;
							downSpot = event->where;
							oldWhen = event->when;
						}
						if (event->what==mouseUp || event->what==mouseDown) upTicks = TickCount();
						FileGraphicRect(&r,pte,graphic,offset,false);
						
						// Ok, now we know if we had a doubleclick.  If we did
						// launch the app
						if (event->what==mouseDown && ClickType==Double)
						{
							Point localPt = event->where;
							GlobalToLocal(&localPt);
							if (PtInRect(localPt,&r))
							{
								FSSpec spec;
								if (!GetFileGraphicSpec(graphic,&spec))
									OpenOtherDoc(&spec,(event->modifiers&controlKey)!=0,false,pte);
								event->what = nullEvent;
							}
							break;
						}

						//	Make sure movie is in right location
						if ((*graphic)->attachment)
							r.bottom -= GetLeading(SmallSysFontID(),SmallSysFontSize());
						SetMovieBounds(graphic,&r,pte);
						if (ClickType!=Double)	// don't let the movie controller have double-clicks
						if (!keyEvent || (*graphic)->pgi.isSelected)	//	Don't process keys unless graphic image has been selected
						if ((!keyEvent && event->what != mouseDown) || MCGetVisible(aController))	//	Don't process keys or clicks if controller not visible
						{
							SAVE_STUFF;
							
							ForeColor(blackColor);
							BackColor(whiteColor);
							gMovieClick = false;
							movieResult = MCIsPlayerEvent(aController, event);
							if (movieResult && !gMovieClick &&
								(event->what != mouseDown || (*graphic)->pgi.isSelected))	//	Keep mouseDown event if graphic image hasn't been selected yet
								event->what = nullEvent;	//	Event handled by QT. Cancel it.
							REST_STUFF;
						}
					}
					break;
#if NETSCAPE
				case pgtNPlugin:
					NPluginEvent((*graphic)->u.plugin.hPlugin,(EventRecord *)data);
					break;
#endif
			}
			break;
	}
	
	PopGWorld();
	UseResFile(oldResFile);

	return(err);
}

/**********************************************************************
 * DisposeGraphic - dispose of graphic data
 **********************************************************************/
static void DisposeGraphic(FGIHandle graphic)
{
	switch ((*graphic)->pgi.privateType)
	{
		Handle	suite;
		case pgtIcon:
			if (suite = (*graphic)->u.icon.suite)
			{
				if (suite != gMissingGraphicIconSuite && suite != gBadGraphicIconSuite && !(*graphic)->u.icon.sharedSuite)
					DisposeIconSuite((*graphic)->u.icon.suite,True);
			}
			else if ((*graphic)->u.icon.iconRef)
				ReleaseIconRef((*graphic)->u.icon.iconRef);
			break;

		case pgtPICT:
		case pgtResPict:
			ZapHandle((*graphic)->u.pict.picture);
			break;
		case pgtImage:
			DisposeImage(graphic);
			break;
		case pgtMovie:
			if ((*graphic)->u.movie.aController) DisposeMovieController((*graphic)->u.movie.aController);
			if ((*graphic)->u.movie.theMovie) DisposeMovie((*graphic)->u.movie.theMovie);
			break;
#if NETSCAPE
		case pgtNPlugin:
			if ((*graphic)->u.plugin.hPlugin) NPluginClose((*graphic)->u.plugin.hPlugin);
			break;
#endif
		case pgtPictHandle:
			ZapHandle((*graphic)->htmlInfo.pictHandle);
			break;
	}
}

/**********************************************************************
 * MakeFileRef - make a new file reference
 **********************************************************************/
static OSErr MakeFileRef(FGIHandle graphic,FSSpecPtr spec)
{
	OSErr	err = noErr;
	AliasHandle alias;
	GFileRefHandle	fileRef=nil;
	
	if (!(err=NewAliasMinimal(spec,&alias)))
	{
		if (fileRef = NuHandleClear(sizeof(GraphicFileRef)))
		{
			(*fileRef)->alias = alias;
			(*fileRef)->count = 1;
		}
		else err = MemError();
	}
	(*graphic)->fileRef = fileRef;
	return err;
}

/**********************************************************************
 * SetMovieBounds - make sure the movie is in the right place
 **********************************************************************/
static void SetMovieBounds(FGIHandle graphic,Rect *r,PETEHandle pte)
{
	Rect	bounds;
	Movie theMovie = (*graphic)->u.movie.theMovie;
	MovieController	aController = (*graphic)->u.movie.aController;
	Boolean	controllerVisible = aController && MCGetVisible(aController);

	if (controllerVisible)
		MCGetControllerBoundsRect(aController, &bounds);
	else
		GetMovieBox(theMovie,&bounds);
		
	if (!EqualRect(r,&bounds))
	{
		//	Check again. May be off by one.
		InsetRect(&bounds,1,1);
		if (!EqualRect(r,&bounds))
		{
			RgnHandle	clipRgn;
			
			//	Change location
			if (controllerVisible)
				MCSetControllerBoundsRect(aController, r);
			else
			{
				 SetMovieBox(theMovie,r);
				MCPositionController(aController,r,nil,mcTopLeftMovie);
			}
			
			//	Set clipping region also
			if (clipRgn=NewRgn())
			{
				PETEDocInfo peteInfo;

				PETEGetDocInfo(PETE,pte,&peteInfo);
				RectRgn(clipRgn,&peteInfo.viewRect);
//				if (controllerVisible)
					MCSetClip(aController,clipRgn,nil);
//				else
//					SetMovieClipRgn(theMovie,clipRgn);
				DisposeRgn(clipRgn);
			}
		}
	}
}

/**********************************************************************
 * FileGraphicAdjustSize - adjust the size of a graphic
 **********************************************************************/
void FileGraphicAdjustSize(FGIHandle graphic,short maxWidth)
{
	short	width, height;
	
	if ((*graphic)->htmlInfo.height && (*graphic)->htmlInfo.width)
	{
		//	Size specified in html IMG tag
		width = (*graphic)->htmlInfo.width;
		height = (*graphic)->htmlInfo.height;
	}
	else
	{
		//	Use image's default size
		width = (*graphic)->width;
		height = (*graphic)->height;
	}

	width += 2*((*graphic)->htmlInfo.hSpace + (*graphic)->htmlInfo.border);
	height += 2*((*graphic)->htmlInfo.vSpace + (*graphic)->htmlInfo.border);

	if (maxWidth && width > maxWidth && !(*graphic)->centerInWin)
	{
		//	Too wide, scale down
		height = maxWidth*height/width;
		width = maxWidth;
	}
	(*graphic)->pgi.width = width;
	(*graphic)->pgi.height = height;
	(*graphic)->pgi.descent = 3;
} 

/**********************************************************************
 * DrawBorder - draw/erase border around graphic
 **********************************************************************/
static void DrawBorder(Boolean doGrey,Rect *r,short thickness,Boolean linkBorder)
{
	Rect	rBorder = *r;
	RGBColor	color;
	
	if (linkBorder)
	{
		GetRColor(&color,URL_COLOR);
		DarkenColor(&color,50);
		RGBForeColor(&color);
	}
	else if (doGrey)
		SetForeGrey(k8Grey4);
	PenSize(thickness,thickness);
	InsetRect(&rBorder,-2,-2);
	FrameRect(&rBorder);
	PenNormal();
	SetForeGrey(0);
}

/**********************************************************************
 * FileGraphicDisplay - draw a file
 **********************************************************************/
void FileGraphicDisplay(PETEHandle pte, FGIHandle graphic, long offset)
{
	Rect r;
	Str255 s;
	short oldResFile = CurResFile();
	OSErr	err = noErr;

	HLock((Handle)graphic);
	FileGraphicRect(&r,pte,graphic,offset,false);
	if ((*graphic)->pgi.privateType == pgtIcon)
	{
		//	Display as icon
		Rect rText,rIcon;

		if (PeteIsInAdWindow(pte)) goto Done;	//	Don't display missing graphic icon in ad window

		PSCopy(s,(*graphic)->name);
		SetSmallSysFont();
		TextFace(0);
		
		if ((*graphic)->u.icon.attachmentStub)
			r.left+=kAttStubIconWd;
		FileGraphicTextRect(&rText,&r);
		FileGraphicIconRect(&rIcon,&r);
		rText.right = rText.left + StringWidth(s);
		
		if ((*graphic)->noImage)
		{
			//	"missing graphic" icon.
			short	wi = RectWi(r);
			short	ht = RectHi(r);
			short	wiText = RectWi(rText);
			short	htText = RectHi(rText);
			
			if (wi < 16 || ht < 16) goto Done;	//	display rect too small
			
			CenterRectIn(&rIcon,&r);
			if (*s)
			{
				CenterRectIn(&rText,&r);
				if (ht > 32)
				{
					//	text below
					if (rText.right >= r.right)
						//	text is too wide, don't display
						*s = 0;
					else
					{
						OffsetRect(&rIcon,0,-8);
						OffsetRect(&rText,0,8);
					}
				}
				else
				{
					//	text right
					if (wiText+24 >= wi)
						//	text is too wide, don't display
						*s = 0;
					else
					{
						OffsetRect(&rIcon,r.left-rIcon.left+4,0);
						OffsetRect(&rText,r.left-rText.left+24,0);
					}
				}
			}
			if ((*graphic)->pgi.isSelected || (!(*graphic)->urlLink && !(*graphic)->isEmoticon))
				DrawBorder(true,&r,(*graphic)->urlLink ? kBorderSize : 1,(*graphic)->urlLink);
		}
		
		//	Draw text
		if (*s)
		{
			DisplayFileName(s,&rText);
			if ((*graphic)->pgi.isSelected) InvertRect(&rText);
		}
		
		//	Draw icon
		if ((*graphic)->u.icon.suite)
			err=PlotIconSuite(&rIcon,atAbsoluteCenter,(*graphic)->pgi.isSelected ? ttSelected:ttNone,(*graphic)->u.icon.suite);
		else if ((*graphic)->u.icon.iconRef)
		{
			PlotIconRef(&rIcon,atAbsoluteCenter,(*graphic)->pgi.isSelected ? ttSelected:ttNone,kIconServicesNormalUsageFlag,(*graphic)->u.icon.iconRef);
		}
		else
			err=PlotIconFromICache(GetICache((*graphic)->type,(*graphic)->creator),(*graphic)->pgi.isSelected ? ttSelected:ttNone,&rIcon);

		if ((*graphic)->u.icon.attachmentStub)
		{
			r.left-=kAttStubIconWd;
			FileGraphicIconRect(&rIcon,&r);
			PlotIconID(&rIcon,atAbsoluteCenter,ttNone,FETCH_SICN);
		}

	}
	else 
	{	
		FSSpec spec;
		PicHandle picture;
		GDHandle	gd;
		CGrafPtr	port;
		Boolean	fPortChange;

		GetGWorld(&port, &gd);
		if (fPortChange = true /*(port != (*graphic)->port || gd != (*graphic)->gd)*/)
		{
			(*graphic)->port = port;
			(*graphic)->gd = gd;
		}

		if ((*graphic)->pgi.isSelected)
			DrawBorder(true,&r,kBorderSize,(*graphic)->urlLink);
		
		if ((*graphic)->attachment)
		{
			//	display the attachment's file name
			Rect	rText;
			short	wdText;
			
			rText = r;
			r.bottom -= GetLeading(SmallSysFontID(),SmallSysFontSize());
			rText.top = r.bottom;
			PSCopy(s,(*graphic)->name);
			SetSmallSysFont();
			TextFace(0);
			wdText = StringWidth(s);
			DisplayFileName(s,&rText);			
		}

		if ((*graphic)->pgi.privateType == pgtIndexPict || (*graphic)->pgi.privateType == pgtPictHandle)
		{
			PicHandle picture;
			Byte hState;
			
			picture = (*graphic)->pgi.privateType == pgtIndexPict ? GetPicture((*graphic)->htmlInfo.pictResID) : (*graphic)->htmlInfo.pictHandle;
			if (picture)
			{
				hState = HGetState((Handle)picture);
				HNoPurge((Handle)picture);
				DrawPicture(picture,&r);
				if (QDError()==noMemForPictPlaybackErr)
					err = memFullErr;
				HSetState((Handle)picture, hState);
			}
		}
		else if (!GetFileGraphicSpec(graphic,&spec))
		{
			switch ((*graphic)->pgi.privateType)
			{
				case pgtPICT:
					if ((*graphic)->u.pict.spool)
						SpoolPictDisplay((*graphic)->u.pict.picture,&spec,&r);
					else
					{
						if (picture = (*graphic)->u.pict.picture)
						{
							if (!*picture)
							{
								ZapHandle(picture);
								picture = SpecPicture(&spec,0);
								(*graphic)->u.pict.picture = picture;
							}
						}
						
						if (picture)
						{
							HNoPurge((Handle)picture);
							DrawPicture(picture,&r);
							if (QDError()==noMemForPictPlaybackErr)
								err = memFullErr;
							HPurge((Handle)picture);
						}
					}
					break;

				case pgtResPict:
					if (picture = (*graphic)->u.pict.picture)
					{
						if (!*picture)
						{
							//	Purged
							ZapHandle(picture);
							picture = SpecResPicture(&spec);
							(*graphic)->u.pict.picture = picture;
						}
					}
					
					if (picture)
					{
						HNoPurge((Handle)picture);
						DrawPicture(picture,&r);
						if (QDError()==noMemForPictPlaybackErr)
							err = memFullErr;
						HPurge((Handle)picture);
					}
					break;


				case pgtImage:
				{
					//	Imported image, let QuickTime handle the display
					GWorldPtr	gWorld = nil;
					Rect		rOriginal;
					RGBColor	saveForeColor,saveBackColor;
					RGBColor	blackColor = { 0,0,0 };
					RGBColor	whiteColor = { 0xffff,0xffff,0xffff };
					GraphicsImportComponent	importer;
					PixMapHandle	hPM;
					PETEDocInfo peteInfo;
					Handle	hData;
					QTImageHandle	hQTImage = (*graphic)->u.image.hQTImage;
					RgnHandle	clipRgn = NewRgn();
					rOriginal = r;
					importer = GetImageImporter(graphic);
					if (!importer) break;
					
					hData = (*hQTImage)->hData;
					
					//	Check for purged data
					if (hData && *hData==0)
					{
						//	Need to reload
						GraphicsImportSetDataFile(importer, &spec);
						ImporterDataToHandle(graphic);
					}
					if (hData)
					{
						HNoPurge(hData);
						SetupPNGTransparencyLo(importer,hData);
					}
					if (fPortChange)
					{
						//	Don't set the GWorld unless the port has changed. 
						//	According to the documentation for GraphicsImportSetGWorld, 
						//	this is an expensive procedure
						GraphicsImportSetGWorld(importer, port, gd);
					}

					PETEGetDocInfo(PETE,pte,&peteInfo);
					if (peteInfo.printing)
					{
						//	QuickTime 2.5 will not print correctly if the image is not at 0,0
						//	Draw it offscreen at 0,0 and then CopyBits it into the printer port
						short	depth,devDepth;
						
						OffsetRect(&r,-r.left,-r.top);	//	Move to location 0,0
						depth = 0;
						devDepth = GetPortPixelDepth(port);
						if (devDepth < 8)
							depth = devDepth;	//	Printing in B&W doesn't work well without this

						err = NewGWorld(&gWorld, depth, &r, nil, nil, useTempMem);	//	Try temp memory first
						if (err)
							err = NewGWorld(&gWorld, depth, &r, nil, nil, nil);	//	Failed, use application heap
						if (!err)
						{
							GraphicsImportSetGWorld(importer, gWorld,nil);
							SetGWorld(gWorld,nil);
							fPortChange = true;
							hPM = GetGWorldPixMap(gWorld);
							LockPixels(hPM);
							EraseRect(&r);
							GetPortClipRegion(gWorld,clipRgn);
							GraphicsImportSetClip(importer,clipRgn);
						}
						else
						{
							r = rOriginal;	//	Failed to get offscreen world. Try drawing at original location. Maybe QuickTime has been fixed.
						}
					}
					else
					{
						GetPortClipRegion(port,clipRgn);
						GraphicsImportSetClip(importer,clipRgn);				
					}
					GraphicsImportSetBoundsRect(importer, &r);
					
					//	Currently there is a bug in QuickTime 2.5. The image will not draw
					//	if the forecolor is not black and only a portion of the image is being drawn
					GetForeColor(&saveForeColor);
					RGBForeColor(&blackColor);
					GetBackColor(&saveBackColor);
					RGBBackColor(&whiteColor);

					MemCanFail = True;	//	Don't be purging memory for this graphic
					err = GraphicsImportDraw(importer);
					MemCanFail = false;
					
					if (gWorld)
					{
						//	OK, copy the offscreen image to the printer port
						SetGWorld(port, gd);
						GetForeColor(&saveForeColor);
						RGBForeColor(&blackColor);
						GetBackColor(&saveBackColor);
						RGBBackColor(&whiteColor);
						GetPortClipRegion(port,clipRgn);
						CopyBits(GetPortBitMapForCopyBits(gWorld),GetPortBitMapForCopyBits(port), &r, &rOriginal, srcCopy, clipRgn);
						DisposeGWorld(gWorld);
					}

					RGBForeColor(&saveForeColor);
					RGBBackColor(&saveBackColor);
					if (hData)
						HPurge(hData);
					
					//	Close the importer component because it keeps a large handle (increments of 32K)
					//	to quickly redraw. We'll reopen if needed.
					CloseComponent(importer);
					(*hQTImage)->importer = nil;
					DisposeRgn(clipRgn);			
				}
					break;
				
				case pgtMovie:
				{
					MovieController	aController;
					
					if (aController = (*graphic)->u.movie.aController)
					{
						WindowPtr	winWP = GetMyWindowWindowPtr ((*PeteExtra(pte))->win);
						CGrafPtr	port = (*graphic)->port;
						Boolean	offscreen = GetWindowPort(winWP) != port;
						
						SetMovieBounds(graphic,&r,pte);
						if (offscreen)
							//	Draw to the offscreen port
							MCSetControllerPort(aController, port);
						MCDraw(aController,winWP);
						if (offscreen)
							//	Restore drawing to the window
							MCSetControllerPort(aController, GetWindowPort(winWP));
					//	MCDraw(aController,(GrafPtr)(*graphic)->port);
							err = UpdateMovie((*graphic)->u.movie.theMovie);
					}
				}
					break;
#if NETSCAPE
				case pgtNPlugin:
					NPluginDraw((*graphic)->u.plugin.hPlugin,&r,port,fPortChange);
					break;
#endif
			}
		}
		
		if ((*graphic)->htmlInfo.border)
		{
			short	thickness = (*graphic)->htmlInfo.border;
			InsetRect(&r,-thickness,-thickness);
			DrawBorder(false,&r,thickness,false);
		}
		
		SetGWorld(port, gd);
	}
	if (err == memFullErr)
	{
		SAVE_STUFF;
		//	Out of memory. Display out of memory message
		if (!(*graphic)->pgi.isSelected)
			FrameRect(&r);
		GetRString(s,GRAPHIC_MEM);
		InsetRect(&r,4,4);
		if (r.bottom-r.top > 50)
			//	Do some rough vertical centering
			r.top = (r.top+r.bottom)/2 - 10;
		SetSmallSysFont();
		TextFace(0);
		TETextBox(s+1,*s,&r,teCenter);
		REST_STUFF;
	}
  Done:
	HUnlock((Handle)graphic);
	UseResFile(oldResFile);
}

/**********************************************************************
 * 
 **********************************************************************/
pascal void MyGetPic(Ptr dataPtr, short byteCount)
{
	long count = byteCount;
	long remainingBytes;
	long dataOffset = 0;
	
	remainingBytes = gSpoolBufSize-gSpoolBufIdx;
	while (count > remainingBytes)
	{
		//	Don't have enough bytes left in the buffer to service this request.
		if (remainingBytes > 0)
		{
			//	Copy what we have�
			BMD(*ghSpoolBuf+gSpoolBufIdx,dataPtr+dataOffset,remainingBytes);
			dataOffset += remainingBytes;
			count -= remainingBytes;
		}
		//	�and read some more.
		HLock(ghSpoolBuf);
		FSRead(gPicRefN,&gSpoolBufSize,*ghSpoolBuf);
		HUnlock(ghSpoolBuf);
		gSpoolBufIdx = 0;
		remainingBytes = gSpoolBufSize;
	}

	BMD(*ghSpoolBuf+gSpoolBufIdx,dataPtr+dataOffset,count);
	gSpoolBufIdx += count;
}

/**********************************************************************
 * SpoolPictDisplay - display a spooled picture
 **********************************************************************/
void SpoolPictDisplay(PicHandle picture,FSSpecPtr spec,Rect *r)
{
	QDProcsPtr savedProcs;
	QDProcs myProcs;
	CQDProcsPtr savedCProcs;
	CQDProcs myCProcs;
	FSSpec newSpec;
	DECLARE_UPP(MyGetPic,QDGetPic);
	
	INIT_UPP(MyGetPic,QDGetPic);
	/*
	 * set up the bottlenecks
	 */
	if (ThereIsColor)
	{
		savedCProcs = GetPortGrafProcs(GetQDGlobalsThePort());
		if (savedCProcs)
			myCProcs = *savedCProcs;	//	Start with existing procs
		else
			SetStdCProcs(&myCProcs);	//	None existing, get standard procs
		SetPortGrafProcs(GetQDGlobalsThePort(),&myCProcs);
	}
	else
	{
		savedProcs = GetPortGrafProcs(GetQDGlobalsThePort());
		if (savedProcs)
			myProcs = *savedProcs;	//	Start with existing procs
		else
			SetStdProcs(&myProcs);	//	None existing, get standard procs
		SetPortGrafProcs(GetQDGlobalsThePort(),&myProcs);
	}
	
	myCProcs.getPicProc = MyGetPicUPP;
	myProcs.getPicProc = MyGetPicUPP;
	
	if (!AFSpOpenDF(spec,&newSpec,fsRdPerm,&gPicRefN))
	{
		SetFPos(gPicRefN,fsFromStart,512+sizeof(Picture));
		
		//	Allocate spool buffer memory
		//	Try halving the size if memory allocation fails
		for(gSpoolBufSize = 8 K;gSpoolBufSize && (!(ghSpoolBuf = NuHTempOK(gSpoolBufSize)));gSpoolBufSize >>= 1);
		gSpoolBufIdx = gSpoolBufSize;
		if (ghSpoolBuf)
		{
			DrawPicture(picture,r);
			ZapHandle(ghSpoolBuf);
		}
		FSClose(gPicRefN);
	}
	
	/*
	 * put the bottlenecks back
	 */
	if (ThereIsColor)
		SetPortGrafProcs(GetQDGlobalsThePort(),savedCProcs);
	else
		SetPortGrafProcs(GetQDGlobalsThePort(),savedProcs);
}

/**********************************************************************
 * SpecPicture - grab a picture from a file
 **********************************************************************/
static PicHandle SpecPicture(FSSpecPtr spec,long maxBytes)
{
	PicHandle picture = nil;
	long len;
	Boolean
	
	MemCanFail = True;	//	Don't be purging memory for this graphic
	Snarf(spec,(void*)&picture,maxBytes ? maxBytes+512:0);
	MemCanFail = False;
	if (picture)
	{
		len = GetHandleSize_(picture);
		if (len<512) ZapHandle(picture);
		else
		{
			BMD((UPtr)*picture+512,(UPtr)*picture,len-512);
			SetHandleBig_(picture,len-512);
		}
	}

	return(picture);
}

/**********************************************************************
 * SpecResPicture - grab a picture resource from a file
 **********************************************************************/
PicHandle SpecResPicture(FSSpecPtr spec)
{
	Handle picture = nil;
	short	refN;
	typedef struct
	{
		long	modDate;
		short	unknown;
		ResType	type;
		short	resID;
	} pnotType,**pnotHandle;
	pnotHandle	hpnot;
	short oldResF = CurResFile ();
	
	if (-1!=(refN=FSpOpenResFile(spec,fsRdPerm)))
	{
		OSType	fileType = FileTypeOf(spec);

		if (fileType=='clpp' || fileType=='SCRN' || fileType=='EPSF')
		{
			//	picture clipping, startup screen, or EPS file. Get 1st PICT resource
			picture = Get1IndResource('PICT',1);
		}

		//	Look for QuickTime preview
		else if ((hpnot = (pnotHandle)GetIndResource('pnot',1)) &&
					GetHandleSize_(hpnot)>= sizeof(pnotType) &&
					(*hpnot)->type == 'PICT')
		{
			picture = Get1Resource('PICT',(*hpnot)->resID);
		}

		if (picture)
			DetachResource(picture);
		CloseResFile(refN);
	}
	UseResFile (oldResF);
	return (PicHandle)picture;
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr PeteFileGraphicHit(PETEHandle pte,FGIHandle graphic,long offset,EventRecord *event)
{
	FSSpec spec;
	Boolean trash = False;
	Boolean dragged = False;
	PETEDocInfo di;
	
	if (event->what!=nullEvent)
	{
		if ((*graphic)->pgi.privateType==pgtPictHandle)
		{
			if (MyWaitMouseMoved(event->where,False))
			{
				trash = FileGraphicDrag(pte,graphic,offset,event);
				dragged = True;
			}		
		}
		else if (!GetFileGraphicSpec(graphic,&spec))
		{
	#ifdef	IMAP
			// if this spec is inside an IMAP Attachments folder, download it rather than open it.
			if ((*graphic)->pgi.privateType == pgtIcon && (*graphic)->u.icon.attachmentStub)
			{
				if (event->modifiers&optionKey) 
				{
					// fetch all attachments
					FetchAllIMAPAttachmentsBySpec(&spec, PETEHandleToMailboxNode(pte), false);
				}
				else
				{
					// if no one else is fetching this attachment, go get it now.
					if (CanFetchAttachment(&spec)) DownloadIMAPAttachment(&spec, PETEHandleToMailboxNode(pte), false);
			 	
			 	}
			 	return (noErr);
			}
	#endif

			if (ClickType==Double || event->modifiers&cmdKey)
			{
				if (!(*graphic)->urlLink)
				{
					OpenOtherDoc(&spec,(event->modifiers&controlKey)!=0,false,pte);
					ClickType = Single;  // prevent other triggers from going
					event->modifiers &= ~cmdKey;	// ditto
				}
			}
			else if (MyWaitMouseMoved(event->where,False))
			{
	#ifdef	IMAP
				if (!((*graphic)->pgi.privateType == pgtIcon && (*graphic)->u.icon.attachmentStub))
				{
	#endif
					trash = FileGraphicDrag(pte,graphic,offset,event);
					dragged = True;
	#ifdef IMAP
				}
	#endif IMAP
			}
		}
	}
	if (!trash) (*graphic)->pgi.isSelected = True;
	if (!dragged)
	{
		PETEGetDocInfo(PETE,pte,&di);
		if (di.docActive && event->what!=nullEvent)
			return(noErr);
		else
		{
			event->what = nullEvent;
			return(tsmDocNotActiveErr);
		}
	}
	else return(noErr);
}

//#define FINDER_AE
static Boolean DeleteIt;

/**********************************************************************
 * FileGraphicDrag - drag a file graphic.  returns true if trashed
 **********************************************************************/
Boolean FileGraphicDrag(PETEHandle pte,FGIHandle graphic,long offset,EventRecord *event)
{
	PETEStyleEntry pse;
	Boolean trash = False;
	long len;
	RgnHandle dragRgn = nil;
	FSSpec spec;
	DragReference drag;
	OSErr err = noErr;
	HFSFlavor hfsData;
	PromiseHFSFlavor promise;
	Point pt;
	Boolean finderTricks = HaveScriptableFinder();
	FSSpec dropSpec;
	WindowPtr	theWindow = GetMyWindowWindowPtr ((*PeteExtra(pte))->win);
	Boolean comp = GetWindowKind(theWindow)==COMP_WIN ||
								 GetWindowKind(theWindow)==MESS_WIN 
								 		&& MessFlagIsSet(Win2MessH((*PeteExtra(pte))->win),FLAG_OUT);
	PETEStyleListHandle	styleScrap;
	Handle	text;
	DECLARE_UPP(FileGraphicSend,DragSendData);

	INIT_UPP(FileGraphicSend,DragSendData);
	GetFileGraphicSpec(graphic,&spec);
	
	dragRgn = NewRgn();
	if (dragRgn && !MyNewDrag((*PeteExtra(pte))->win,&drag))
	{
		BuildHFSFlavor(&hfsData,&spec);
		
		if (comp)
		{
			// we promise an alias to the file to the finder
			// so as to avoid actually moving files when
			// dealing with outbound attachments, because the
			// user most likely means to detach them when
			// dragging to the trash, not really delete them
			Zero(promise);
			promise.fileType = hfsData.fileType;
			promise.fileCreator = hfsData.fileType;
			promise.promisedFlavor = SPEC_FLAVOR;
		}
				
		// build the fancy region, making sure it includes the mouse
		pt = event->where;
		GlobalToLocal(&pt);
		FileGraphicFancyRgn(pte,graphic,offset,pt,dragRgn);
		OutlineRgn(dragRgn,1);
		GlobalizeRgn(dragRgn);
		
		// copy our file into dropSpec, so the send data proc can find it
		dropSpec = spec;
		
		// add the flavors and set the send proc
		if (comp)
		{
			if (!(err=SetDragSendProc(drag,FileGraphicSendUPP,(void*)&dropSpec)))
			if (!(err=AddDragItemFlavor(drag,1,flavorTypePromiseHFS,&promise,sizeof(promise),0)))
			err=AddDragItemFlavor(drag,1,SPEC_FLAVOR,nil,0,0);
		}
		else if ((*graphic)->pgi.privateType==pgtPictHandle)
		{
			//	PICT data
			err=AddDragItemFlavor(drag,1,kScrapFlavorTypePicture,LDRef((*graphic)->htmlInfo.pictHandle),GetHandleSize((*graphic)->htmlInfo.pictHandle),0);
			UL((*graphic)->htmlInfo.pictHandle);
		}
		else
			// not comp or picthandle
			err=AddDragItemFlavor(drag,1,flavorTypeHFS,&hfsData,sizeof(hfsData),0);
		
		if (!err)
		{	
			//	add styled text so we can drag and drop graphics within a messsage
			if (!(comp && CompHeadCurrent(pte)==ATTACH_HEAD))
			if (!(err=PeteGetTextStyleScrap(pte,offset,offset+1,&text,&styleScrap,nil)))
			{
				if (!(err=AddDragItemFlavor(drag,1,flavorTypeText,LDRef(text),GetHandleSize_(text),flavorSenderOnly+flavorNotSaved)))
					err=AddDragItemFlavor(drag,1,kPETEStyleFlavor,LDRef(styleScrap),GetHandleSize_(styleScrap),flavorSenderOnly+flavorNotSaved);
				ZapHandle(text);
				ZapHandle(styleScrap);
			}
		}

		if (!err)
		{
			// ok, let's do the drag
			err = MyTrackDrag(drag,&MainEvent,dragRgn);
			
			// did the user try to trash the file?
			trash = DragTargetWasTrash(drag);
			
			// if it was dragged to the trash, delete it from the window
			if (!err)
			{
				if (trash)
				{
					PETEGetStyle(PETE,pte,offset,&len,&pse);
					PeteDelete(pte,pse.psStartChar,pse.psStartChar+len);
				}
			}
			else 	if (err==userCanceledErr)
			{
				// might be an application
				err = GetDropSpec(drag,&dropSpec);
				if (!err && FileTypeOf(&dropSpec)=='APPL')
					err = OpenDocWith(&spec,&dropSpec,false);
			}
		}
		MyDisposeDrag(drag);
	}
	if (dragRgn) DisposeRgn(dragRgn);
	return(trash && !err);
}


/**********************************************************************
 * FileGraphicSend - drag data proc for file graphics
 **********************************************************************/
pascal OSErr FileGraphicSend(FlavorType flavor, void *dragSendRefCon, ItemReference theItemRef, DragReference drag)
{
#pragma unused(theItemRef)
	OSErr err=cantGetFlavorErr;
	FSSpecPtr spec = (FSSpecPtr)dragSendRefCon;
	AEDesc dropLocation;
	FSSpec drop;
	//FInfo info;
	
	/*
	 * return the filename
	 */
	if (flavor==SPEC_FLAVOR)
	{
		drop = *spec;
		NullADList(&dropLocation,nil);
		if (!(err=GetDropLocation(drag,&dropLocation)))
		if (!(err=GetDropLocationDirectory(&dropLocation,&drop.vRefNum,&drop.parID)))
		{
			if (SameVRef(drop.vRefNum,spec->vRefNum) && drop.parID==spec->parID)
				err = dupFNErr;
			else
			{
				err = MakeAFinderAlias(spec,&drop);

				if (!err)
				{
					err = MySetDragItemFlavorData(drag,1,SPEC_FLAVOR,&drop,sizeof(FSSpec));
					*spec = drop;
				}
			}
		}
		DisposeADList(&dropLocation,nil);
	}
	return(err);
}

/**********************************************************************
 * GetDropSpec - we dropped it on the Finder
 **********************************************************************/
OSErr GetDropSpec(DragReference drag,FSSpecPtr spec)
{
	AEDesc drop, newDrop;
	OSErr err;
	
	NullADList(&drop,&newDrop,nil);
	
	if (!(err=GetDropLocation(drag,&drop)))
	if (!(err=AECoerceDesc(&drop,typeFSS,&newDrop)))
		AEGetDescData(&newDrop,spec,sizeof(FSSpec));

	DisposeADList(&drop,&newDrop,nil);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
void FileGraphicFancyRgn(PETEHandle pte,FGIHandle graphic,long offset,Point pt,RgnHandle rgn)
{
	Rect r, iconR, textR;
	
	// get rectangles for text & icon
	FileGraphicRect(&r,pte,graphic,offset,true);
	if ((*graphic)->pgi.privateType == pgtIcon)
	{
		FileGraphicIconRect(&iconR,&r);
		FileGraphicTextRect(&textR,&r);
	}
	
	// does the user want us to make sure a point is in the region?
	if ((pt.h || pt.v) && !PtInRect(pt,&iconR) && !PtInRect(pt,&r))
	{
		pt.h = pt.h - iconR.left - 8;
		pt.v = pt.v - iconR.top - 8;
		OffsetRect(&r,pt.h,pt.v);
		if ((*graphic)->pgi.privateType == pgtIcon)
		{
			OffsetRect(&iconR,pt.h,pt.v);
			OffsetRect(&textR,pt.h,pt.v);
		}
	}
	
	if ((*graphic)->pgi.privateType != pgtIcon)
	{
		RectRgn(rgn, &r);
	}
	else
	{	
		// add the icon's region
		IconSuiteToRgn(rgn,&iconR,atAbsoluteCenter,(*GetICache((*graphic)->type,(*graphic)->creator))->cache);
		// and the text region
		RgnPlusRect(rgn,&textR);
	}
}

/**********************************************************************
 * BuildHFSFlavor - build data for hfs drag flavor
 **********************************************************************/
HFSFlavor *BuildHFSFlavor(HFSFlavor *hfs, FSSpecPtr spec)
{
	FInfo info;
	
	FSpGetFInfo(spec,&info);
	hfs->fileType = info.fdType;
	hfs->fileCreator = info.fdCreator;
	hfs->fdFlags = info.fdFlags;
	hfs->fileSpec = *spec;
	return(hfs);
}


/**********************************************************************
 * FileGraphicIconRect - return the rectangle of the icon in a graphic
 **********************************************************************/
Rect *FileGraphicIconRect(Rect *iconR, Rect *r)
{
	SetRect(iconR,r->left+9,r->top+1,r->left+25,r->top+17);
	return(iconR);
}

/**********************************************************************
 * FileGraphicTextRect - return the rectangle of the text in a graphic
 **********************************************************************/
Rect *FileGraphicTextRect(Rect *textR, Rect *r)
{
	Rect tempR;
	
	FileGraphicIconRect(&tempR,r);
	
	SetRect(textR,tempR.right+2,tempR.bottom-GetLeading(SmallSysFontID(),SmallSysFontSize()),r->right,tempR.bottom);
	return(textR);
}

/**********************************************************************
 * HaveQuickTime - Do we have the indicated version of QuickTime?
 **********************************************************************/
Boolean HaveQuickTime(short minVersion)
{
	static short	qtVersion;
	static Boolean	haveChecked;
	long	gestaltResult;

	if (!haveChecked)
	{
		//	Check for QuickTime
		if (!Gestalt(gestaltQuickTime, &gestaltResult))
		{
			//	Get version
			qtVersion = HiWord(gestaltResult);
			haveChecked = true;
		}
	}
	return qtVersion >= minVersion; 	//	Make sure we have the minimum version
}

/**********************************************************************
 * ImporterDataToHandle - load the graphic image into a handle (if possible)
 **********************************************************************/
static void ImporterDataToHandle(FGIHandle hGraphic)
{
	unsigned long offset, size;
	GraphicsImportComponent	importer;
	Handle	hData;
	OSErr	err;
	QTImageHandle	hQTImage = (*hGraphic)->u.image.hQTImage;

	importer = GetImageImporter(hGraphic);
	if (!importer) return;

	if (!GraphicsImportGetDataHandle(importer,&hData) && hData && *hData)
	{
		(*hQTImage)->hData = hData;
		return;	//	Already have data in a non-purged handle
	}
	
	hData = (*hQTImage)->hData;
	
	if (!(err = GraphicsImportGetDataOffsetAndSize(importer, &offset, &size)))
	{
		//	We need to read the entire file, not just the data portion
		size += offset;
		offset = 0;
		
		if (hData)
		{
			//	Reuse handle
			ReallocateHandle(hData,size);
		}
		else
		{
			//	New handle
			hData = (*hQTImage)->hData = NuHTempBetter(size);
		}

		if (!(err = MemError()))
		{
			HLock(hData);
			if (!(err = GraphicsImportReadData(importer,*hData, offset, size)))
				err = GraphicsImportSetDataHandle(importer,hData);
			HUnlock(hData);
		}
	}
	
	if (err)
	{
		ZapHandle(hData);
		(*hQTImage)->hData = nil;
	}
	else
	{
		HPurge(hData);	//	Make it purgeable
	}
}

/**********************************************************************
 * ValidGraphicFile - check a graphic file to make sure it hasn't been truncated
 **********************************************************************/
static CodecType ValidGraphicFile(FGIHandle hGraphic)
{
	GraphicsImportComponent	importer;
	Handle	hData;
	CodecType	cType = 0;
	long	size;
	QTImageHandle	hQTImage = (*hGraphic)->u.image.hQTImage;
	
	if (!(importer = GetImageImporter(hGraphic))) return 0;
	if ((hData = (*hQTImage)->hData) && *hData && (size = GetHandleSize(hData)))
	{
		ImageDescriptionHandle	hDesc;
		
		if (!GraphicsImportGetImageDescription(importer, &hDesc))
		{
			unsigned char *pbData;
			short	*pwData;
			
			cType = (*hDesc)->cType;
	
			pbData = *hData + size - 1;	//	Point to last byte in file
			switch ((*hDesc)->cType)
			{
				case 'qdrw':
					//	PICT
					pwData = (short *)(*hData+0x200);
					if (pwData[5]==0x0011 && pwData[6]==0x02FF)
					{
						//	Version 2. Must end with 0x00FF
						if (pbData[0] != 0xFF || pbData[-1] != 0x00)
							cType = 0;
					}
					else
					{
						//	Version 1. Check length and last byte must be 0xFF
						if (pwData[0] + 512 > size || pbData[0] != 0xFF)
							cType = 0;
					}
					break;

				case 'gif ':
				case 'giff':
					//	GIF file ends with 0x3b end-of-image code. Search for it starting from the end.
					//	May have false positives due to 0x3b's found elsewhere in file.
					cType = 0;
					while (size)
					{
						if (*pbData == 0x3b)
						{
							cType = (*hDesc)->cType;
							break;
						}
						pbData--;
						size--;
					}
					break;

				case 'jpeg':
				case 'jpg ':
					//	JPEG file ends with 0xFFD9 end-of-image marker. Since the marker is unique. Let's search for it
					//	starting from the end of the file.
					cType = 0;
					while (size)
					{
						pbData--;
						if (pbData[0] == 0xFF && pbData[1] == 0xD9)
						{
							cType = (*hDesc)->cType;
							break;
						}
						size--;
					}
					break;
			}		
			ZapHandle(hDesc);
		}
	}
	return cType;
}

/**********************************************************************
 * GraphicsImportComponent - make a graphic handle from a file
 **********************************************************************/
static Boolean CanImport(FSSpecPtr spec, GraphicsImportComponent *importer, Component *c, OSType fileType)
{
	Boolean	result = false;

	if (IsPDFFile(spec,fileType) && PrefIsSet(PREF_DONT_DISPLAY_PDF)) return false;
	*importer = nil;
	*c = nil;
	MemCanFail = True;	//	Don't be purging memory
	if (HaveQuickTime(0x0250))
	{
		//	Save time by handling common file types here
		//	(Suggested by Sam Bushell at Apple)
		if (OpenADefaultComponent && (
			fileType==kQTFileTypeGIF || 
			fileType==kQTFileTypeJPEG ||
			fileType==kQTFileTypeTIFF ||
			fileType==kQTFileTypeBMP ||	
			fileType=='BMPp' ||	
			fileType=='PDF ' ||	
			fileType==kQTFileTypePNG ||			
			fileType==kQTFileTypePicture))
		{
			//	Sometimes file types are wrong and if we open the wrong importer,
			//	QuickTime may crash. Load the file into RAM and verify we have the
			//	right file type.
			Handle	hData;

			if (!Snarf(spec,&hData,0))
			{
				Boolean	valid = false;
				Ptr pData = *hData;

				switch (fileType)
				{
					case kQTFileTypeGIF:
						valid = !memcmp(pData,"GIF",3);
						break;
						
					case kQTFileTypeJPEG:
						valid = !memcmp(pData,"\xFF\xD8\xFF\xE0",4)
								&& !memcmp(pData+6,"\x4A\x46\x49\x46\x00",5);
						break;
						
					case kQTFileTypeTIFF:
						valid = !memcmp(pData,"MM",2) || !memcmp(pData,"II",2);
						break;
					
					case 'BMPp':
					case kQTFileTypeBMP:
						valid = !memcmp(pData,"BM",2);
						break;
						
					case kQTFileTypePicture:
						valid = !memcmp(pData+522,"\x11\x01",2) ||	// Version 1
							!memcmp(pData+522,"\x00\x11\x02\xFF",4);	// Version 2
						break;
					
					case kQTFileTypePNG:
						valid = !memcmp(pData,"\x89PNG\x0D\x0A\x1a\x0A",8);
						break;
					
					case 'PDF ':
						valid = !memcmp(pData,"%PDF",4);
						break;
				}
			
				if (valid && !OpenADefaultComponent(GraphicsImporterComponentType,fileType,importer))
				{
					GraphicsImportSetDataFile(*importer,spec);
					GraphicsImportSetDataHandle(*importer,hData);
				}
				else
					DisposeHandle(hData);
			}
		}

		if (!*importer && CanQTImport(spec,fileType))
		//	QuickTime 3.0's GetGraphicsImporterForFileWithFlags will speedily tell us if QuickTime can display this file
		//	OK, so it's not so speedy. Don't use it. (alb)_
		//	GetGraphicsImporterForFileWithFlags(spec,&importer,kDontUseValidateToFindGraphicsImporter);			
			GetGraphicsImporterForFile(spec, importer);

		if (*importer)
		{
		    ComponentDescription cd;

		    GetComponentInfo((Component)*importer,&cd,nil,nil,nil);
		    *c = (Component)cd.componentFlagsMask;				
			result = true;
		}
	}
	MemCanFail = false;
	return result;
}


/**********************************************************************
 * CanQTImport - see if QT can import this file as an image
 **********************************************************************/
static Boolean CanQTImport(FSSpecPtr spec,OSType fileType)
{
	static Handle	hFileTypeList,hFileExtList;
	GraphicsImportComponent	importer = nil;
	ComponentDescription looking;
	Component cmpIndex;
	Str32	sExt;
	short	i;

	if (fileType == 'PDF ' && PrefIsSet(PREF_DONT_DISPLAY_PDF))
		//	Don't display PDF
		return false;
	
	if (!hFileTypeList)
	{
		// generate file type list		
		hFileTypeList = NuHandle(0);
		cmpIndex = 0;
		do
		{
			Zero(looking);
			looking.componentType = 'grip';
			if (cmpIndex = FindNextComponent(cmpIndex, &looking))
			{
				ComponentDescription	cd;
				
				GetComponentInfo(cmpIndex,&cd,nil,nil,nil);
				if (!(cd.componentFlags&(1<<12)))
				{
					if (!IsImporterFileType(cd.componentSubType,hFileTypeList))
						PtrAndHand(&cd.componentSubType,hFileTypeList,sizeof(OSType));
				}
			}
		} while (cmpIndex);
	}
	
	if (IsImporterFileType(fileType,hFileTypeList))
		return true;	// found file type
	
	//	Look for a file extension
	for(i=spec->name[0];i;i--)
		if (spec->name[i]=='.')
			break;
	
	if (!i)
		return false;	// no file extension
	
	//	Get file extension
	MakePStr(sExt,&spec->name[i+1],spec->name[0]-i);
	MyLowerStr(sExt);
	
	if (!hFileExtList && HaveQuickTime(0x0300) && GraphicsImportGetMIMETypeList)
	{
		//	make file extension list using MIME type list (requires QT 3.0)
		hFileExtList = NuHandleClear(sizeof(short));
		cmpIndex = 0;
		do
		{
			Zero(looking);
			looking.componentType = 'grip';
			if (cmpIndex = FindNextComponent(cmpIndex, &looking))
			{
				ComponentInstance instance;
				
				if (instance=OpenComponent(cmpIndex))
				{
					QTAtomContainer	atom = nil;
					QTAtom	child;
					short	i;
					
					GraphicsImportGetMIMETypeList(instance, &atom);
					if (atom)
					{
						for(i=1;child=QTFindChildByIndex(atom,kParentAtomIsContainer,'ext ',i,nil);i++)
						{
							long		dataSize;
							Ptr			atomData;
							Str255		sFileExtList;
							Str32		sFileExt;
							UPtr 		spot;

							QTGetAtomDataPtr(atom,child,&dataSize,&atomData);
							MakePStr(sFileExtList,atomData,dataSize);
							spot = sFileExtList+1;
							while(PToken(sFileExtList,sFileExt,&spot,","))
							{
								if (!IsImporterFileExt(hFileExtList,sFileExt))
								{
									//	add to list
									PtrAndHand(sFileExt,hFileExtList,*sFileExt+1);
									(*(short *)(*hFileExtList))++;
								}
							}
						}
					}				
					CloseComponent(instance);
				}
			}
		} while (cmpIndex);
	}
	
	return IsImporterFileExt(hFileExtList,sExt);
}


/**********************************************************************
 * IsImporterFileExt - is file extension in list
 **********************************************************************/
static Boolean IsImporterFileExt(Handle h,PStr s)
{
	short n;
	short i;
	UPtr p;
	
	if (h && (n=*(short *)(*h)))
	{
		for (i=1,p=*h+sizeof(short);i<=n;i++,p+=*p+1)
		{
			if (p[0]==s[0] && !memcmp(p+1,s+1,s[0]))
				return true;
		}
	}
	return false;
}

/**********************************************************************
 * IsImporterFileType - is file type in list
 **********************************************************************/
static Boolean IsImporterFileType(OSType fileType,Handle h)
{
	short	n = GetHandleSize_(h)/sizeof(OSType);
	OSType	*pList = (OSType*)*h;
	
	while (n--)
	{
		if (pList[n]==fileType)
			return true;
	}
	return false;
}
	
/**********************************************************************
 * OpenQTMovie - see if we can open the file as a QuickTime movie
 **********************************************************************/
static OSErr OpenQTMovie(FSSpecPtr spec,Movie *theMovie,WindowPtr theWindow,OSType fileType,FGIHandle graphic,Boolean active,Boolean justCheck)
{
  short	movieResFile;
  OSErr	err = noErr;
  Boolean	isMovie = false;
	
if (fileType == 'PDF ' && PrefIsSet(PREF_DONT_DISPLAY_PDF))
	//	Don't display PDF
	return noMediaHandler;

if (spec && SpecEndsWithExtensionR(spec,NOT_MOVIE_EXTENSIONS))
	return noMediaHandler;
	
 if (!HaveQuickTime(0x0100) || !EnterMovies)	//	Make sure we have QuickTime 1.0
  	return noMediaHandler;
    		
	if (!QTMoviesInited)
	{
		if (err = EnterMovies())	//	Need to do this once
			return err;
		QTMoviesInited = true;
	}
	
	if (fileType==MovieFileType)
		isMovie = true;
	else
	{
		//	See if movie can be imported
		if (HaveQuickTime(0x0300) && GetMovieImporterForDataRef)
		{
			//	QuickTime 3.0's GetMovieImporterForDataRef will speedily tell us if QuickTime can display this file
			Boolean	tempAlias=false;
	    AliasHandle alias=nil;
	    Component	importerComponent;
	    
	    if (graphic && (*graphic)->fileRef)
	    	alias = (*(*graphic)->fileRef)->alias;
			
			if (!alias)
			{
				//	We don't have an alias yet. Make one
				if (!(err = NewAliasMinimal(spec, &alias)))
					tempAlias = true;
			}
			
			if (alias)
			{
				if (!GetMovieImporterForDataRef(rAliasType,(Handle)alias,kGetMovieImporterDontConsiderGraphicsImporters,&importerComponent))
				{
					ComponentDescription	cd;
					
					if (!GetComponentInfo(importerComponent,&cd,nil,nil,nil))
					{
					   if (cd.componentFlags & movieImportSubTypeIsFileExtension)
					   {
					     // (Got the following from Chris Flick at Apple)
					     // this is for the file extension, so check if it's a component alias and
					     // if it is, get that resolved component's information
					     Component aliasedComponent;

					     if ((long)ResolveComponentAlias != kUnresolvedCFragSymbolAddress)
						     if (aliasedComponent = ResolveComponentAlias(importerComponent))
						        GetComponentInfo(aliasedComponent,&cd,nil,nil,nil);
					   }

						if (cd.componentSubType != 'TEXT' || PrefIsSet(PREF_TEXT_MOVIES))
							isMovie = true;
					}
				}
				if (tempAlias)
					DisposeHandle((Handle)alias);
			}
		}
		else
		{	
			if (TypeIsOnList(FileTypeOf(spec),FILE_MOVIE_LIST_TYPE) != kTOLNot)
				isMovie = true;
		}
	}	
	
	// If we just want to know if it's a movie, time to return
	if (justCheck) return isMovie ? noErr : noMediaHandler;
	
	if (isMovie && !OpenMovieFile(spec, &movieResFile,fsRdPerm))
	{
		short		movieResID = 0;   /* want first movie */
    	Boolean	wasChanged;

		SetPort(GetWindowPort(theWindow));
		err = NewMovieFromFile (theMovie, movieResFile, &movieResID, nil, active?newMovieActive:0, &wasChanged);
		CloseMovieFile(movieResFile);       
	}
	else
  		err = noMediaHandler;
	return err;
}

/**********************************************************************
 * PeteMakeFileGraphic - make a graphic handle from a file
 **********************************************************************/
void PeteMakeFileGraphic(PETEHandle pte,FGIHandle newGHndl,FSSpecPtr spec,short maxWidth,short ht,Boolean displayInline)
{
	FSSpec	origSpec = *spec;
	long tlid;
	Handle suite;
	Rect r;
	FInfo info;
	short	width,height;
	short oldResFile = CurResFile();
	Movie		theMovie;
	OSErr	err;
	Str255 longName;
	DECLARE_UPP(ImportProgress,ICMProgress);

#if __profile__
//	ProfilerSetStatus(true);
#endif
	INIT_UPP(ImportProgress,ICMProgress);
	PushGWorld();		
	SetPort(InsurancePort);
	SetSmallSysFont();
	
	(*newGHndl)->spec = *spec;	//	Save original spec
	
	//	Resolve if alias
	IsAlias(spec,spec);
	
	// Grab long name
	if ((*newGHndl)->isEmoticon) *longName = 0;
	else if (FSpGetLongName(spec,kTextEncodingUnknown,longName))
		PSCopy(longName,spec->name);
		
	(*newGHndl)->pgi.privateType = pgtIcon;	//	Assume displaying icon
	SetRect(&r,0,0,StringWidth(longName)+27,18);
	(*newGHndl)->peteID = (*PeteExtra(pte))->id;
	(*newGHndl)->displayInline = displayInline;
	err = MakeFileRef(newGHndl,spec);
	if (!err)
	{
		if (FSpIsItAFolder(spec))
		{
			(*newGHndl)->type = kSystemIconsCreator;
			(*newGHndl)->creator = kGenericFolderIcon;
		}
		else
		{
			err = FSpGetFInfo(spec,&info);
			(*newGHndl)->type = info.fdType;
			(*newGHndl)->creator = info.fdCreator;
		}
	}

	if (displayInline && !(PrefIsSet(PREF_DONT_DISPLAY_PDF) && IsPDFFile(spec,info.fdType)))
	{
		if (!err)
		{
			PicHandle picture=nil;
			GraphicsImportComponent	importer;
			Component	c;
			QTImageHandle	hQTImage;
			Boolean	allowQTImport;
	#if NETSCAPE
			Handle	hNetscapePlugin;
	#endif			

			//	If this is a duplicate of an image we are already displaying,
			//	reuse it
			if (hQTImage = CanReuseImage(spec))
			{
				(*newGHndl)->u.image.hQTImage = hQTImage;
				(*newGHndl)->pgi.privateType = pgtImage;
				r = (*hQTImage)->rBounds;
				(*hQTImage)->refCount++;
			}

			//	See if it's an image QuickTime can import
			else if ((allowQTImport=TryQTImport(info.fdType,info.fdCreator)) && CanImport(spec,&importer,&c,info.fdType))
			{
				//	We're in luck. There is an importer.
				CodecType	cType;				
				QTImageHandle	hImageInfo = NuHandleClear(sizeof(QTImageInfo));

				(*hImageInfo)->importer = importer;
				(*hImageInfo)->component = c;
				(*hImageInfo)->spec = *spec;
				(*hImageInfo)->refCount = 1;
				(*hImageInfo)->modDate = FSpModDate(spec);
				(*newGHndl)->u.image.hQTImage = hImageInfo;
				LL_Queue(gQTImageList,hImageInfo,(QTImageHandle));
				ImporterDataToHandle(newGHndl);	//	Move data into a handle
				if (ValidAdImage(pte,(*hImageInfo)->hData) ||
					!(cType = ValidGraphicFile(newGHndl)) || 
					GraphicsImportGetNaturalBounds(importer, &r))
				{
					//	Not a valid graphic file
					DisposeImage(newGHndl);
					importer = nil;
				}
				else
				{
					MatrixRecord defaultMatrix;

					GraphicsImportGetNaturalBounds(importer, &r);

					//	Setup default display parameters. For example, a fax tiff file
					//	may have different horizontal and vertical resolutions.
					//	Adapted from code by Sam Bushell <bushell@apple.com> (QuickTime Engineering)
					
					//	Some or all of the following functions may not be available in older
					//	versions of QuickTime. Does the QuickTime reference doc indicate when
					//	these versions are available? Nope.
					if (SetIdentityMatrix && GraphicsImportGetDefaultMatrix && TransformRect)
					{
						SetIdentityMatrix(&defaultMatrix);
						GraphicsImportGetDefaultMatrix(importer,&defaultMatrix); // ignore errors
						TransformRect(&defaultMatrix,&r,nil);
						//GraphicsImportSetMatrix(importer,&defaultMatrix);
					}

					(*hImageInfo)->rBounds = r;
					(*newGHndl)->pgi.privateType = pgtImage;
					if (GraphicsImportSetProgressProc)
					{
						//	set up a progress function for slow drawing
						ICMProgressProcRecord	progressInfo;
						
						progressInfo.progressProc = ImportProgressUPP;
						GraphicsImportSetProgressProc(importer,&progressInfo);
					}

					if (((cType == 'gif ' || cType == 'giff') && PrefIsSet(PREF_ANIMATED_GIFS) && HaveQuickTime(0x0300))
						|| cType == 'pdf ')
					{
						//	Open as a movie to see if it's an animated GIF or multi-page PDF
					    if (!OpenQTMovie(spec,&theMovie,GetMyWindowWindowPtr((*PeteExtra(pte))->win),info.fdType,newGHndl,false,false))
					    {
							if (GetMediaSampleCount(GetTrackMedia(GetMovieIndTrack(theMovie,1))) > 1)
							{
								//	it's got multiple frames
								Handle	hData = (*(*newGHndl)->u.image.hQTImage)->hData;
								
								(*(*newGHndl)->u.image.hQTImage)->hData = nil;	//	Don't dispose yet
								DisposeImage(newGHndl);
								importer = nil;
								//	The movie was opened inactive in case there was only one frame and we didn't need the movie.
								//	This saves much time.
								SetMovieActive(theMovie,true);
								SetupMovie(theMovie,newGHndl,&r,cType != 'pdf ',hData);
								if (hData)
									ZapHandle(hData);
								if (cType != 'pdf ')
									StartMovie(theMovie);
								(*newGHndl)->u.movie.animatedGraphic = true;
							}
							else
							{
								//	simple GIF. don't need movie
								DisposeMovie(theMovie);
							}
						}
					}
				}	
			}

			//	Check for PICT
			else if (info.fdType=='PICT')
			{
				//	
				if ( FSpDFSize(spec)>GetRLong(PICT_SPOOL_SIZE))
				{
					picture = SpecPicture(spec,sizeof(Picture));
					(*newGHndl)->u.pict.spool = True;
				}
				else
				{
					if (picture = SpecPicture(spec,0))
						HPurge((Handle)picture);
				}

				if (picture)
				{
					r = (*picture)->picFrame;
					(*newGHndl)->pgi.privateType = pgtPICT;
					(*newGHndl)->u.pict.picture = picture;
				}
			}
			
			//	How about a QuickTime movie?
	    else if (allowQTImport && !OpenQTMovie(spec,&theMovie,GetMyWindowWindowPtr((*PeteExtra(pte))->win),info.fdType,newGHndl,true,false))
	    {
			SetupMovie(theMovie,newGHndl,&r,false,nil);
		}
	#if NETSCAPE
	   	//	Well, then how about a Netscape plugin?
	    else if (hNetscapePlugin = NPluginCheck(spec,&r))
	    {
				(*newGHndl)->u.plugin.hPlugin = hNetscapePlugin;
				(*newGHndl)->pgi.privateType = pgtNPlugin;
				SetRect(&r, 0, 0, maxWidth, ht);
	    }
	#endif
			
			//	Look for previews and PICT resources
			else if (picture = SpecResPicture(spec))
			{
				r = (*picture)->picFrame;
				HPurge((Handle)picture);
				(*newGHndl)->pgi.privateType = pgtResPict;
				(*newGHndl)->u.pict.picture = picture;
			}			


			//	If it's an HTML image and the size was specified, use the specified size
			if ((*newGHndl)->htmlInfo.width && (*newGHndl)->htmlInfo.height)
				SetRect(&r,0,0,(*newGHndl)->htmlInfo.width,(*newGHndl)->htmlInfo.height);
		}
	}	
	else
	{
		//	If not displying inline, get rid of any html attributes
		(*newGHndl)->htmlInfo.height = 0;
		(*newGHndl)->htmlInfo.width = 0;
		(*newGHndl)->htmlInfo.hSpace = 0;
		(*newGHndl)->htmlInfo.vSpace = 0;
		(*newGHndl)->htmlInfo.border = 0;
	}

	//	Get graphic size
	if ((*newGHndl)->htmlInfo.height || (*newGHndl)->htmlInfo.width)
	{
		//	Size specified in html IMG tag
		width = (*newGHndl)->htmlInfo.width;
		height = (*newGHndl)->htmlInfo.height;
	}
	else
	{
		//	Use image's default size
		width = r.right - r.left;
		height = r.bottom - r.top;
	}

	//	Add in any html border and/or extra space
	(*newGHndl)->width = width + 2*((*newGHndl)->htmlInfo.hSpace + (*newGHndl)->htmlInfo.border);
	(*newGHndl)->height = height + 2*((*newGHndl)->htmlInfo.vSpace + (*newGHndl)->htmlInfo.border);
	FileGraphicAdjustSize(newGHndl,maxWidth);

	PSCopy((*newGHndl)->name,longName);

	if (err)
	{
		//	File error. Use missing icon
		MissingGraphicIcon(newGHndl,true);
#ifdef MISSING_GRAPHIC_ASSERTS
		if (RunType!=Production) Dprintf("\pCan't get file info. Err: %d",err);
#endif
	}
	else
		AdWinGotImage(pte,spec);	//	See if the ad window needs to know
	
	if ((*newGHndl)->pgi.privateType == pgtIcon)
	{
		if (info.fdType==MIME_FTYPE && 
			!ETLReadTL(spec,&tlid) && 
			!(ETLIDToFileIcon(tlid,&suite)))
				(*newGHndl)->u.icon.suite = suite;
		else
		{
			IconRef	iconRef;
			SInt16	label;
			
			if ((*newGHndl)->u.icon.attachmentStub = IsIMAPAttachmentStub(spec))
				(*newGHndl)->width += kAttStubIconWd;

			if (!GetIconRefFromFile(spec,&iconRef,&label))
				(*newGHndl)->u.icon.iconRef = iconRef;
		}

	}
	else if ((*newGHndl)->attachment && displayInline)
			//	Leave room to display filename below graphic
			(*newGHndl)->height += GetLeading(SmallSysFontID(),SmallSysFontSize());

	PopGWorld();
	UseResFile(oldResFile);

#if __profile__
//	ProfilerSetStatus(false);
//	ProfilerDump("\pfilegraphic-profile");
//	ProfilerClear();
#endif
}

#endif

/**********************************************************************
 * DisplayFileName - display attachment file name
 **********************************************************************/
static void DisplayFileName(StringPtr s,Rect *rText)
{
	RGBColor	textColor;
	SAVE_STUFF;

	if (ThereIsColor)
	{
		GetRColor(&textColor,TEXT_COLOR);
		RGBForeColor(&textColor);
	}

	SetSmallSysFont();
	TextFace(0);

	MoveTo(rText->left,rText->bottom-GetDescent(GetPortTextFont(GetQDGlobalsThePort()),GetPortTextSize(GetQDGlobalsThePort())));
	EraseRect(rText);
	DrawString(s);

	REST_STUFF;
}


/**********************************************************************
 * SetupMovie - setup to display this movie
 **********************************************************************/
static void SetupMovie(Movie theMovie,FGIHandle newGHndl,Rect *r,Boolean hideController,Handle hFile)
{
	MovieController	aController=nil;
	Rect	rController;
	
	(*newGHndl)->u.movie.theMovie = theMovie;
	GetMovieBox(theMovie, r);
	if (aController = NewMovieController(theMovie, r,hideController ? mcTopLeftMovie+mcNotVisible : mcTopLeftMovie))
	{
		MCGetControllerBoundsRect(aController, &rController);
		if (hideController)
		{
			if (!hFile || (SearchStrHandle("\p\x21\xff\x0bNETSCAPE2.0\x03\x01",hFile,0,false,false,nil)>0))	//	See note below
				//	If Netscape looping extension is found, loop the animation
				MCDoAction(aController, mcActionSetLooping, (void*)true);	//	enable looping
		}
		else
		{
			MCDoAction(aController, mcActionSetKeysEnabled, (void*)true);	//	Process keys
			*r = rController;
		}
		MCDoAction(aController, mcActionSetDragEnabled, (void*)false);	//	override QT's drag and drop
		(*newGHndl)->u.movie.aController = aController;
	}
	(*newGHndl)->pgi.privateType = pgtMovie;
	(*newGHndl)->pgi.wantsEvents = true;
}
/* 
	Note: Documentation on Netscape's GIF extension for looping animations.

	Netscape Navigator has an Application Extension Block that tells Navigator to loop 
	the entire GIF file. The Netscape block must appear immediately after the global color 
	table of the logical screen descriptor. Only Navigator 2.0 Beta4 or better willl 
	recognize this Extension block. The block is 19 bytes long composed off: (note: hexadecimal equivalent supplied
	for programmers) 

	byte   1       : 33 (hex 0x21) GIF Extension code
	byte   2       : 255 (hex 0xFF) Application Extension Label
	byte   3       : 11 (hex 0x0B) Length of Application Block
	                 (eleven bytes of data to follow)
	bytes  4 to 11 : "NETSCAPE"
	bytes 12 to 14 : "2.0"
	byte  15       : 3 (hex 0x03) Length of Data Sub-Block
	                 (three bytes of data to follow)
	byte  16       : 1 (hex 0x01)
	bytes 17 to 18 : 0 to 65535, an unsigned integer in
	                 lo-hi byte format. This indicate the
	                 number of iterations the loop should
	                 be executed.
	byte  19       : 0 (hex 0x00) a Data Sub-Block Terminator.
*/


/**********************************************************************
 * FindPart - find a part
 **********************************************************************/
Boolean FindPart(StackHandle parts,uLong hash,PartDesc *pd,Boolean checkCID)
{
	if (hash)
	{
		short	item;

		for (item=0;item<(*parts)->elCount;item++)
		{
			StackItem(pd,item,parts);
				if (hash == (checkCID ? pd->cid : pd->absURL))
					return true;
		}
	}
	return false;	//	Not found
}

/**********************************************************************
 * GetHTMLSpec - get the file spec for an HTML image
 **********************************************************************/
static void GetHTMLSpec(FGIHandle graphic,PETEHandle pte,long offset)
{
	MyWindowPtr win;
	StackHandle partStack;
	PartDesc pd;
	
	if (PeteIsValid(pte))
	if (win = (*PeteExtra(pte))->win)
	if (partStack = (*PeteExtra(pte))->partStack)
	//	Find the part info for the image
	if ((FindPart(partStack,(*graphic)->htmlInfo.cid,&pd,true) ||		//	Try cid
		FindPart(partStack,(*graphic)->htmlInfo.withBase,&pd,false) ||			//	Try with base
		FindPart(partStack,(*graphic)->htmlInfo.withSource,&pd,false) ||				//	Try with source
		FindPart(partStack,(*graphic)->htmlInfo.url,&pd,false))) 	//	Try raw URL
	{
		//	Now get graphic info for the file
		Boolean wantsEvents = (*graphic)->pgi.wantsEvents;
		PeteMakeFileGraphic(pte,graphic,&pd.spec,GetMaxWidth(pte),RectHi((*PeteExtra(pte))->win->contR)-2*GROW_SIZE,(*graphic)->displayInline);		
		if (wantsEvents != (*graphic)->pgi.wantsEvents)
			//	If wantsEvents changes, we need to notify the editor
			PETEForceRecalc(PETE,pte,offset,offset+1);
	}
}

/**********************************************************************
 * MissingGraphicIcon - using the "missing graphic" icon
 **********************************************************************/
static void MissingGraphicIcon(FGIHandle graphic,Boolean zapDimensions)
{
	Str255	s;
	
	(*graphic)->pgi.privateType = pgtIcon;
	if (!gMissingGraphicIconSuite)
		GetIconSuite(&gMissingGraphicIconSuite, MISSING_IMAGE_ICON, svAllSmallData);
	(*graphic)->u.icon.suite = gMissingGraphicIconSuite;
	SetSmallSysFont();
	PCopy(s,(*graphic)->name);
	(*graphic)->width = 28 + StringWidth(s);
	(*graphic)->height = 18;
	(*graphic)->noImage = true;
	if (zapDimensions)
	{
		(*graphic)->htmlInfo.width = 0;
		(*graphic)->htmlInfo.height = 0;
		(*graphic)->htmlInfo.vSpace = 0;
		(*graphic)->htmlInfo.hSpace = 0;
		(*graphic)->htmlInfo.border = 0;
	}
//	(*graphic)->htmlInfo.height = (*graphic)->htmlInfo.width = 0;
	FileGraphicAdjustSize(graphic,300);
}

/**********************************************************************
 * BadGraphicIcon - using the "missing graphic" icon
 **********************************************************************/
static void BadGraphicIcon(FGIHandle graphic,Boolean zapDimensions)
{
	Str255	s;
	
	(*graphic)->pgi.privateType = pgtIcon;
	if (!gBadGraphicIconSuite)
		GetIconSuite(&gBadGraphicIconSuite, BAD_IMAGE_ICON, svAllSmallData);
	(*graphic)->u.icon.suite = gBadGraphicIconSuite;
	SetSmallSysFont();
	PCopy(s,(*graphic)->name);
	(*graphic)->width = 28 + StringWidth(s);
	(*graphic)->height = 18;
	(*graphic)->noImage = true;
	if (zapDimensions)
	{
		(*graphic)->htmlInfo.width = 0;
		(*graphic)->htmlInfo.height = 0;
		(*graphic)->htmlInfo.vSpace = 0;
		(*graphic)->htmlInfo.hSpace = 0;
		(*graphic)->htmlInfo.border = 0;
	}
//	(*graphic)->htmlInfo.height = (*graphic)->htmlInfo.width = 0;
	FileGraphicAdjustSize(graphic,300);
}

/**********************************************************************
 * FileGraphicRect - get the display area for a graphic
 **********************************************************************/
static void FileGraphicRect(Rect *r,PETEHandle pte, FGIHandle graphic, long offset, Boolean withHTMLBorder) 
{
	short	hSpace,vSpace,border;
	
	PeteGraphicRect(r,pte,(PETEGraphicInfoHandle)graphic,offset);
	if ((*graphic)->centerInWin)
	{
		Rect	rWin = (*PeteExtra(pte))->win->contR;
		CenterRectIn(r,&rWin);
	}
	else
	{
		border = withHTMLBorder ? 0 : (*graphic)->htmlInfo.border;
		hSpace = (*graphic)->htmlInfo.hSpace + border;
		vSpace = (*graphic)->htmlInfo.vSpace + border;
		if (hSpace || vSpace)
			InsetRect(r,hSpace,vSpace);
	}
}

/**********************************************************************
 * GetFileGraphicSpec - get the spec for the file graphic
 **********************************************************************/
static OSErr GetFileGraphicSpec(FGIHandle graphic, FSSpecPtr spec)
{
	OSErr	err;
	GFileRefHandle	fileRef;

	if (fileRef = (*graphic)->fileRef)
	{
		if (!(err=SimpleResolveAlias((*fileRef)->alias,spec)))
			IsAlias(spec,spec);
	}
	else err = paramErr;
	return err;
}

/**********************************************************************
 * IsGraphicFile - is this file a graphic?
 **********************************************************************/
Boolean IsGraphicFile(FSSpecPtr spec)
{
	Boolean	result = false;
	PicHandle	picture;
	OSType		fileType;

	//	Resolve if alias
	IsAlias(spec,spec);
	
	fileType = FileTypeOf(spec);
	
	// Text?
	if (fileType=='TEXT') return PrefIsSet(PREF_TEXT_MOVIES);
	
	//	Is it a filetype that is a graphic? or a movie?
	if (TypeIsOnList(fileType,FILE_GRAPHIC_LIST_TYPE) != kTOLNot || IsMovieFile(spec,fileType))
		result = true;
	//	Is there a PICT resource?
	else if (picture = SpecResPicture(spec))
	{
		ZapHandle((Handle)picture);
		result = true;
	}
	//	Can QuickTime display it?
	else if (CanQTImport(spec,fileType))
	{
		result = true;
	}
	return result;
}

/**********************************************************************
 * InsertGraphic - insert a graphic that has been cloned (copy & paste, drag & drop)
 **********************************************************************/
static OSErr InsertGraphic(PETEHandle pte,FGIHandle graphic)
{
	GFileRefHandle	fileRef;
	FSSpec	spec,newSpec;
	Str32		s;
	OSErr		err = noErr;

	if ((*graphic)->clone)	//	Insert clones only, otherwise it has already been inserted
	{
		if (fileRef = (*graphic)->fileRef)
		if (!(err=GetFileGraphicSpec(graphic,&spec)))
		{
			//	Make a duplicate of the file if it is from a different message and the file is in
			//	the spool folder, the parts folder or the trash
			FSSpec	folderSpec,parentSpec;
			Boolean	duplicate = false;
			
			if ((*graphic)->peteID != (*PeteExtra(pte))->id)	//	Different pete ID means different message
			{
				if (!SubFolderSpec(SPOOL_FOLDER,&folderSpec) &&
					!FSMakeFSSpec(spec.vRefNum,spec.parID,"",&parentSpec) &&
					parentSpec.parID == folderSpec.parID && SameVRef(parentSpec.vRefNum,folderSpec.vRefNum))
						//	Message is in spool folder
						duplicate = true;
				else if (!SubFolderSpec(PARTS_FOLDER,&folderSpec) &&
					spec.parID == folderSpec.parID && SameVRef(spec.vRefNum,folderSpec.vRefNum))
						//	Message is in parts folder
						duplicate = true;
				else if (!GetTrashSpec(spec.vRefNum,&folderSpec) &&
					spec.parID == folderSpec.parID && SameVRef(spec.vRefNum,folderSpec.vRefNum))
						//	Message is in the trash
						duplicate = true;
			}
			
			if (duplicate)
			{
				//	Make duplicate
				MyWindowPtr win;
				MessHandle messH;

				if (win = (*PeteExtra(pte))->win)
				if (messH = (MessHandle)GetMyWindowPrivateData(win))
				if (!MakeAttSubFolder(messH,SumOf(messH)->uidHash,&newSpec))
				{
					short	suffix = 2;
					short	saveLen;

					//	Get a unique file name
					PCopy(newSpec.name,spec.name);
					saveLen = *newSpec.name;
					while (!FSpExists(&newSpec))
					{
						//	No error means that the file/folder exists. Change the file name by adding a numeric suffix
						NumToString(suffix++,s);
						if (saveLen+*s > 30)
							saveLen = 30-*s;	//	Name was too long
						*newSpec.name = saveLen;	//	Remove any suffix
						PCatC(newSpec.name,' ');
						PCat(newSpec.name,s);
					}
					if ((err=FSpDupFile(&newSpec,&spec,false,false)))
						FileSystemError(COPY_FAILED,spec.name,err);
					spec = newSpec;
				}
			}
		}	
			
		//	Insert the graphic
		if (fileRef && !err)
		{
			ReleaseFileRef(fileRef);	//	Not referencing original file ref anymore
			PeteMakeFileGraphic(pte,graphic,&spec,(*graphic)->width,(*graphic)->height,(*graphic)->displayInline);
		}
		else if ((*graphic)->pgi.privateType != pgtHTMLPending && (*graphic)->pgi.privateType != pgtPictHandle)
		{
			//	Error. Use missing graphic icon
			MissingGraphicIcon(graphic,false);
#ifdef MISSING_GRAPHIC_ASSERTS
			{
				Str32	sHex;
				Long2Hex(sHex,(long)fileRef);
				if (RunType!=Production) Dprintf("\pInsert graphic error. Err: %d, FileRef: %p",err,sHex);
			}
#endif
		}
		(*graphic)->clone = false;
	}			
	return err;
}

/**********************************************************************
 * UnrefFileREf - no longer reference this file ref. Dispose if count goes to zero
 **********************************************************************/
static void ReleaseFileRef(GFileRefHandle fileRef)
{
	if (fileRef && --(*fileRef)->count == 0)
	{
		//	No more references to file. Dispose of file reference.
		ZapHandle((*fileRef)->alias);
		ZapHandle(fileRef);
	}
}


/**********************************************************************
 * GetMaxWidth - return the maximum width of a graphic for a given PETE handle
 **********************************************************************/
static short GetMaxWidth(PETEHandle pte)
{
	Rect	r;
	
	PeteRect(pte,&r);
	return RectWi(r);
}

/**********************************************************************
 * DisplayFetchGraphics - should we display the "GetGraphics" button in this window?
 **********************************************************************/
Boolean DisplayGetGraphics(MyWindowPtr win)
{
	Boolean	result = win==gGetGraphicsWin;
	
	gGetGraphicsWin = nil;
	return result;
}

/**********************************************************************
 * URLAccessIsInstalled - is URL Access installed?
 **********************************************************************/
static Boolean URLAccessIsInstalled(void)
{
#ifdef URLACCESS
	return URLAccessAvailable() && PrefIsSet(PREF_URL_ACCESS);
#else
	return false;
#endif
}

/**********************************************************************
 * MakeURLFileName - make a filespec for a URL. 
 *   Name format: host/hash.extension
 **********************************************************************/
static void MakeURLFileSpec(FGIHandle graphic,PETEHandle pte,FSSpec *spec)
{
	StringHandle	absURL = (*graphic)->htmlInfo.absURL;
	Str255	s;
	
	Zero(*spec);
	if (absURL)
	{
		PCopyTrim(s,*absURL,sizeof(Str255));	
		GetCacheSpec(s,spec,PeteIsInAdWindow(pte));
	}
}

/**********************************************************************
 * MakeURLFileName - make a filespec for a URL. 
 *   Name format: host/hash.extension
 **********************************************************************/
void GetCacheSpec(StringPtr sURL,FSSpec *spec,Boolean useAdCache)
{
	Str255	sHost;
	Str32	sExt;
	PStr	pExt,name;
	long	hash;
	short	maxHostLen;

	//	Get cache folder location
	SubFolderSpec(useAdCache?AD_FOLDER_NAME:CACHE_FOLDER,spec);		

	MyLowerStr(sURL);
	hash = Hash(sURL);
	
	//	Find any extension
	*sExt = 0;
	if (pExt = PRIndex(sURL,'.'))
	{
		long	lenExt;
		
		lenExt = *sURL - (pExt - sURL) + 1;
		if (lenExt < 8)
			MakePStr(sExt,pExt,lenExt);
	}

	//	Get host
	ParseURL(sURL,nil,sHost,nil);

	//	Format the name
	maxHostLen = 29-8-*sExt;
	if (*sHost>maxHostLen)
		*sHost = maxHostLen;
	name = spec->name;
	PCopy(name,sHost);
	name[++name[0]] = '/';
	PXCat(name,hash);
	PCat(name,sExt);
}

/**********************************************************************
 * DisposeURLRef - dispose of URL ref
 **********************************************************************/
static void DisposeURLRef(FGIHandle graphic)
{
	GetURLDataHandle	hURLData,hDuplicate,hNextDup;
	
	if (hURLData = (GetURLDataHandle)(*graphic)->pURLInfo)
	{
		for(hDuplicate=(*hURLData)->duplicate;hDuplicate;hDuplicate=hNextDup)
		{
			//	Dispose of any duplicates
			hNextDup = (*hDuplicate)->duplicate;
			ZapHandle(hDuplicate);
		}
#ifdef URLACCESS
		if (URLAccessIsInstalled())
			URLDisposeReference((*hURLData)->urlRef);
#endif
		(*graphic)->pURLInfo = nil;
		if (!(*hURLData)->reschedule)
			gDownloadCount--;
		LL_Remove(gURLDataList,hURLData,(GetURLDataHandle)); 
		ZapHandle(hURLData);
	}
}

#ifdef URLACCESS
/**********************************************************************
 * URLNotifyProc - download URL notification proc
 **********************************************************************/
static pascal OSStatus URLNotifyProc(void *userContext, URLEvent event, URLCallbackInfo *callbackInfo)
{
	OSStatus urlError;
	GetURLDataHandle	hData;
	
	hData = userContext;
	
	switch (event)
	{
		case kURLCompletedEvent:		
			(*hData)->finished = true;	//	Mark it for disposal by idle function
			URLGetError(callbackInfo->urlRef,&urlError);
			(*hData)->urlError = urlError;
			break;
		
		case kURLDownloadingEvent:
			//	Starting download. Temporarily change file type to "in progress"
			TweakFileType(&(*hData)->spec,kFileInProcessType,CREATOR);
			break;
	}

	return noErr;
}
#endif                                             

/**********************************************************************
 * BeginDownload - begin the download
 **********************************************************************/
static OSErr BeginDownload(GetURLDataHandle hURLData,URLReference *urlRef)
{
	OSErr	err;
	char	URLcstring[256];
	FSSpec	spec;
	short oldResFile = CurResFile();
	
	PCopyTrim(URLcstring,*(*(*hURLData)->graphic)->htmlInfo.absURL,sizeof(Str255));	//	Remove any leading or trailing spaces from URL
	p2cstr(URLcstring);
#ifdef URLACCESS
	if (URLAccessIsInstalled())
	{
		err = URLNewReference(URLcstring,urlRef);
		if (!err)
		{
			FSSpec spec = (*hURLData)->spec;
			DECLARE_UPP(URLNotifyProc,URLNotify);

			INIT_UPP(URLNotifyProc,URLNotify);
			MoveHHi((Handle)hURLData);
			HLock((Handle)hURLData);	//	Need to lock down the spec because URL Access doesn't save it
			err = URLOpen(*urlRef,&(*hURLData)->spec,kURLReplaceExistingFlag+kURLExpandFileFlag/*+kURLDisplayAuthFlag*/,
					URLNotifyProcUPP,kURLDownloadingMask+kURLErrorOccurredEventMask+kURLCompletedEventMask,hURLData);

			if (err)
				URLDisposeReference(*urlRef);
		}
	}
	else
#endif
	{
		spec = (*hURLData)->spec;
		err = DownloadURL(URLcstring,&spec,(long)hURLData,DownloadURLFinished,(long *)urlRef,nil);
	}

	if (!err)
	{
		gDownloadCount++;
		(*hURLData)->urlRef = *urlRef;
	}

	UseResFile(oldResFile);	//	URL Access is bad about changing curresfile
	return err;
}

/**********************************************************************
 * DownloadURLFinished - we have finished downloading a URL (not with URL Access)
 **********************************************************************/
static void DownloadURLFinished(long refCon,OSErr err,DownloadInfo *info)
{
	GetURLDataHandle	hURLData = (GetURLDataHandle)refCon;
	
	(*hURLData)->finished = true;	//	Mark it for disposal by idle function
	(*hURLData)->urlError = err;
}

/**********************************************************************
 * GetHTMLGraphic - get a graphic from a URL, may need to download
 **********************************************************************/
static Boolean GetHTMLGraphic(PETEHandle pte,FGIHandle graphic,Boolean allowDownload,Boolean forceDownload)
{
	URLReference 	urlRef;
	OSStatus		err = noErr;
	GetURLData	URLData;
	GetURLDataHandle	hURLData,hDuplicate=nil;
	
	Zero(URLData);
	MakeURLFileSpec(graphic,pte,&URLData.spec);
	if (!*URLData.spec.name)
		return false;	//	Unable to generate a spec

	(*graphic)->wasDownloaded = true;	// Indicate this graphic came from a URL

	//	Determine if we are already downloading this file
	for (hURLData=gURLDataList;hURLData;hURLData=(*hURLData)->next)
	{
		if (EqualString(URLData.spec.name,(*hURLData)->spec.name,true,true))
		{
			hDuplicate=hURLData;
			break;
		}
	}

	if (!hDuplicate && !FSpExists(&URLData.spec))
	{
		//	File is already in cache.
		CInfoPBRec	hfi;
		HGetCatInfo(URLData.spec.vRefNum,URLData.spec.parID,URLData.spec.name,&hfi);
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==kFileInProcessType && hfi.hFileInfo.ioFlFndrInfo.fdCreator==CREATOR)
		{
			//	File not completely downloaded.
			//	Delete and start over.					
			FSpDelete(&URLData.spec);
		}
		else
		{
			//	Use the file in the cache
			FSpTouch(&URLData.spec);	//	Set file's mod date to test for least-recently-used cache purge
			PeteMakeFileGraphic(pte,graphic,&URLData.spec,GetMaxWidth(pte),(*graphic)->height,(*graphic)->displayInline);
			return true;
		}
	}
	
	//	Don't try a download if setting is disabled or we aren't allowed to, unless the user has hit the fetch button.
	if (!forceDownload && (!allowDownload || !DownloadURLOK()))
		return false;
	
	// Don't download certain obnoxious graphics
	if (!forceDownload && HTMLGraphicIsObnoxious(graphic))
	{
		BadGraphicIcon(graphic,true);
		return false;
	}
	
	URLData.graphic = graphic;
	URLData.pte = pte;
	if (hURLData = NuHandle(sizeof(GetURLData)))
	{
		BMD(&URLData,*hURLData,sizeof(GetURLData));
		if (hDuplicate)
		{
			//	Add to list of duplicates
			(*hURLData)->duplicate = (*hDuplicate)->duplicate;
			(*hDuplicate)->duplicate = hURLData;
		}
		else
		{
			//	Not a duplicate
			if (gDownloadCount < kMaxDownloads)
				err = BeginDownload(hURLData,&urlRef);
			else
				//	Too many downloads. Try again later
				(*hURLData)->reschedule = true;

			if (!err)
			{
				(*graphic)->pURLInfo = (Ptr)hURLData;
				LL_Push(gURLDataList,hURLData);	//	Add to URL list
			}
			else
			{
				ZapHandle(hURLData);
				return false;
			}
		}		
	}
	return true;
}

Boolean HTMLGraphicIsObnoxious(FGIHandle graphic)
{
	short width = (*graphic)->htmlInfo.width;
	short height = (*graphic)->htmlInfo.height;
	Str255 s;
	
	if (width && height && width*height < GetRLong(HTML_MIN_IMAGE_SIZE)) return true;
	
	ComposeString(s,"\p%dx%d",width,height);
	if (StrIsItemFromRes(s,HTML_BAD_IMAGE_DIMENSIONS,",")) return true;
	return false;
}

/**********************************************************************
 * CheckCacheLimit - purge the cache if necessary
 **********************************************************************/
static void CheckCacheLimit(void)
{
	long	sizeLimit,cacheSize,delSize,thisSize;
	Str32	name;
	FSSpec	spec;
	
	sizeLimit = GetRLong(GRAPHICS_CACHE_MAX);
	if (!sizeLimit)
		sizeLimit = 5 K;	//	Default 5M
	sizeLimit *= 1 K;	//	Convert KBytes to bytes
	
	SubFolderSpec(CACHE_FOLDER,&spec);		
	do
	{
		CInfoPBRec hfi;
		unsigned long	delDate;

		hfi.hFileInfo.ioNamePtr = name;
		hfi.hFileInfo.ioFDirIndex = 0;
		delDate = 0;
		cacheSize = 0;
		while (!DirIterate(spec.vRefNum,spec.parID,&hfi))
		{
			thisSize = hfi.hFileInfo.ioFlPyLen + hfi.hFileInfo.ioFlRPyLen;
			cacheSize += thisSize;	//	Add in this file size
			if (!delDate || hfi.hFileInfo.ioFlMdDat < delDate)
			{
				delDate = hfi.hFileInfo.ioFlMdDat;
				delSize = thisSize;
				PCopy(spec.name,name);	//	save name of least-recently-used
			}
		}
		
		if (cacheSize > sizeLimit)
		{
			//	The cache is too big. Delete the least-recently-used file
			if (FSpDelete(&spec)) return;	//	On error, return. Can't get rid of this file!
			cacheSize -= delSize;
		}
	} while (cacheSize > sizeLimit);
}

/**********************************************************************
 * DisplayDownload - display this downloaded graphic
 **********************************************************************/
static void DisplayDownload(GetURLDataHandle hURLData)
{
	FGIHandle		graphic = (*hURLData)->graphic;
	PETEHandle		pte = (*hURLData)->pte;

	// is the pte stale?
	if (PETEDocCheck(PETE,pte,true,true))
	{
		ASSERT(0);
		return;
	}
	
	LDRef(hURLData);
	PeteMakeFileGraphic(pte,graphic,&(*hURLData)->spec,GetMaxWidth(pte),(*graphic)->height,(*graphic)->displayInline);
	(*(*hURLData)->graphic)->wasDownloaded = true;	// Indicate this graphic came from a URL
	UL(hURLData);
}

/**********************************************************************
 * GraphicDownloadIdle - check for completed downloads and ones needing scheduling
 **********************************************************************/
void GraphicDownloadIdle(void)
{
	GetURLDataHandle	hURLData,hURLNext,hDuplicate;
	OSErr		err;
	PETEHandle	redisplayPTE = nil;

	if (!gURLDataList) return;	//	Nothing in the list

	if (URLAccessIsInstalled())
		URLIdle();	// Give some time to URL Access

	//	Search for completed downloads
	for (hURLData=gURLDataList;hURLData;hURLData=hURLNext)
	{
		hURLNext=(*hURLData)->next;
		if ((*hURLData)->finished)
		{
			//	Done with this one
			if (!(*hURLData)->urlError)
			{
				//	Cause this image to be displayed
				DisplayDownload(hURLData);
				redisplayPTE = (*hURLData)->pte;
				
				//	Cause all duplicates to be displayed also
				for(hDuplicate=(*hURLData)->duplicate;hDuplicate;hDuplicate=(*hDuplicate)->duplicate)
					DisplayDownload(hDuplicate);

				CheckCacheLimit();
				DisposeURLRef((*hURLData)->graphic);
			}
#if 0
//	not going to worry about this for now --alb 2/19/99
			else if ((*hURLData)->urlError==kURLAuthenticationError)
			{
				//	Get name and password for authentication
				
			}
#endif
			else
			{
				//	Error, dispose of file.
				URLDeleteFile(hURLData);
				DisposeURLRef((*hURLData)->graphic);
			}
		}
	}

	//	Search for items needing to be scheduled
	for (hURLData=gURLDataList;hURLData && gDownloadCount < kMaxDownloads;hURLData=(*hURLData)->next)
	{
		if ((*hURLData)->reschedule)
		{
			URLReference urlRef;
			if (err = BeginDownload(hURLData,&urlRef))
			{
				//	Error. Dispose of it next time.
				(*hURLData)->finished = true;
				(*hURLData)->urlError = err;
			}
			(*hURLData)->reschedule = false;
		}
	}
	if (redisplayPTE)
	{
		MyWindowPtr win = (*PeteExtra(redisplayPTE))->win;
		Boolean noUpdates = win->noUpdates;
		
		// Don't do updates while doing Pete recalc
		win->noUpdates = true;
		PeteRecalc(redisplayPTE);
		win->noUpdates = noUpdates;
	}
}

/**********************************************************************
 * GraphicsImportComponent - reopen importer if it has been closed
 **********************************************************************/
static GraphicsImportComponent GetImageImporter(FGIHandle graphic)
{
	QTImageHandle	hQTImage = (*graphic)->u.image.hQTImage;
	GraphicsImportComponent	importer = (*hQTImage)->importer;
	FSSpec	spec;
	
	if (!importer)
	{
		//	It was closed. Reopen importer component.
	    if (importer = OpenComponent((*hQTImage)->component))
	    {
	    	(*hQTImage)->importer = importer;
			if ((*hQTImage)->hData)
				GraphicsImportSetDataHandle(importer,(*hQTImage)->hData);
			else
			{
				GetFileGraphicSpec(graphic,&spec);
				GraphicsImportSetDataFile(importer,&spec);
			}
		}	
	}
	return importer;
}

/**********************************************************************
 * ImportProgress - progress function for importer
 **********************************************************************/
static pascal OSErr ImportProgress(short message, Fixed completeness, long refcon)
{
	CycleBalls();
	return noErr;
}


/**********************************************************************
 * URLDeleteFile - close (if open) and delete a file
 **********************************************************************/
static void URLDeleteFile(GetURLDataHandle hURLData)
{
	CInfoPBRec	hfi;
	FSSpec		spec = (*hURLData)->spec;

	if (!AFSpGetCatInfo(&spec,&spec,&hfi))
	{
		if (hfi.hFileInfo.ioFRefNum)
			//	URL Access may have left file open
			FSClose(hfi.hFileInfo.ioFRefNum);
		FSpDelete(&spec);
	}
}

/**********************************************************************
 * DisposeImage - we're done with this image
 **********************************************************************/
static void DisposeImage(FGIHandle graphic)
{
	QTImageHandle	hQTImage = (*graphic)->u.image.hQTImage;
	
	if (hQTImage)
	if (--(*hQTImage)->refCount == 0)
	{
		//	Done with this component
		CloseComponent((*hQTImage)->importer);
		if ((*hQTImage)->hData)
			ZapHandle((*hQTImage)->hData);
		
		LL_Remove(gQTImageList,hQTImage,(QTImageHandle)); 
		ZapHandle(hQTImage);
		(*graphic)->u.image.hQTImage = nil;
	}
}

/**********************************************************************
 * CanReuseImage - search for a duplicate of this image that we can reuse
 **********************************************************************/
static QTImageHandle CanReuseImage(FSSpecPtr spec)
{
	QTImageHandle	hQTImage;
	uLong modDate = FSpModDate(spec);
	
	for(hQTImage=gQTImageList;hQTImage;hQTImage=(*hQTImage)->next)
		if ((modDate == (*hQTImage)->modDate) && SameSpec(spec,&(*hQTImage)->spec))
			return hQTImage;	//	Found!
	
	return nil;	//	not found
}

/************************************************************************
 * GetPNGTransColor - if it's a PNG graphic, return any transparency color
 ************************************************************************/
Boolean GetPNGTransColor(GraphicsImportComponent importer,FSSpec *spec,RGBColor *transColor)
{
	Boolean	found = false;

	if (GetGraphicType(importer)==kPNGCodecType)
	{
		Handle	hData;

		if (!Snarf(spec,&hData,0))
		{
			found = FindPNGTransparency(hData,transColor);
			ZapHandle(hData);
		}
	}
	return found;
}

/************************************************************************
 * SetupPNGTransparency - if it's a PNG graphic, enable transparency color
 ************************************************************************/
void SetupPNGTransparency(GraphicsImportComponent importer,FSSpec *spec)
{
	//	QuickTime doesn't yet support transparency colors for
	//	PNG files. We'll set it up ourselves
	if (GetGraphicType(importer)==kPNGCodecType)
	{
		Handle	hData;

		if (!Snarf(spec,&hData,0))
		{
			SetupPNGTransparencyLo(importer,hData);
			ZapHandle(hData);
		}
	}
}

/************************************************************************
 * SetupPNGTransparencyLo - enable PNG transparency color
 ************************************************************************/
void SetupPNGTransparencyLo(GraphicsImportComponent importer,Handle hData)
{
	static RGBColor	color;

	if (FindPNGTransparency(hData,&color))
		GraphicsImportSetGraphicsMode(importer,transparent,&color);
}

/************************************************************************
 * FindPNGTransparency - search for PNG transparency color
 ************************************************************************/
static Boolean FindPNGTransparency(Handle hData,RGBColor *transColor)
{
	//	Search for transparency and palette chunks
	long	length;
	Ptr		pAlpha;
	Boolean	found = false;
	
	if (pAlpha = FindPNGChunk('tRNS',hData,&length))
	{										
		unsigned short	i;
		Ptr	pColor;

		for(i=0;i<length;i++)
		{
			//	Find an alpha value of zero. 
			//	That's our transparency color index
			if (pAlpha[i]==0)
			{
				found = true;
				break;
			}
		}
		if (found)
		if (pColor = FindPNGChunk('PLTE',hData,&length))
		{
			pColor += 3*i;						
			transColor->red = pColor[0]<<8;
			transColor->green = pColor[1]<<8;
			transColor->blue =pColor[2]<<8;;
		}
	}
	return found;
}

/************************************************************************
 * FindPNGChunk - find a chunk in a PNG file
 ************************************************************************/
static Ptr FindPNGChunk(uLong chunkType,Handle hData,long *pLength)
{
	uLong	*pChunk;
	uLong	length;
	Ptr		pEnd = GetHandleSize(hData) + *hData;
	
	pChunk = (uLong*)*hData;
	//	Verify PNG signature
	if (*pChunk++ == 0x89504e47)
	if (*pChunk++ == 0x0d0a1a0a)
	{
		while(pChunk<pEnd)
		{
			length = pChunk[0];
			if (pChunk[1]==chunkType)
			{
				//	Found it
				*pLength = length;
				return (Ptr)(pChunk+2);	//	Point to chunk data
			}
			pChunk = (uLong*)((Ptr)pChunk+length+12);
		}
	}
	return nil;	//	Not found
}


/************************************************************************
 * GetGraphicType - what type is this graphic importer?
 ************************************************************************/
static CodecType GetGraphicType(GraphicsImportComponent importer)
{
	ImageDescriptionHandle	hDesc;
	CodecType	cType = 0;

	if (!GraphicsImportGetImageDescription(importer, &hDesc))
	{
		cType = (*hDesc)->cType;
		ZapHandle(hDesc);
	}
	return cType;
}

/************************************************************************
 * TryQTImport - do we want to allow QuickTime to attempt to import this file?
 ************************************************************************/
static Boolean TryQTImport(OSType fType,OSType fCreator)
{
	short	fIdx;
	short	count;

	if (fType=='PDF ' && PrefIsSet(PREF_DONT_DISPLAY_PDF)) return false;
	
	//	Check "yes" list first
	count = CountResources(kQTYesList);
	for (fIdx=1;fIdx<=count;fIdx++)
		if (FindQTImporter(kQTYesList,fIdx,fType,fCreator))
			return true;	//	Attempt to import
	
	//	Now check "no" list
	count = CountResources(kQTNoList);
	for (fIdx=1;fIdx<=count;fIdx++)
		if (FindQTImporter(kQTNoList,fIdx,fType,fCreator))
			return false;	//	Don't import
	
	return true;	//	Go ahead and try
}

/************************************************************************
 * FindQTImporter - look up type and creator in a resource list
 ************************************************************************/
static Boolean FindQTImporter(ResType rListResType,short rIdx,OSType fType,OSType fCreator)	
{
	typedef struct { OSType fType; OSType fCreator; } TypeAndCreator;
	typedef struct
	{
		short	count;
		TypeAndCreator list[];
	} **QTImportListHandle;
	QTImportListHandle	hList;

	if (hList = (QTImportListHandle)GetIndResource(rListResType,rIdx))
	{
		short	i;
		TypeAndCreator	*pList = (*hList)->list;
		
		for(i=0;i<(*hList)->count;i++,pList++)
		{
			if (pList->fType==fType || pList->fType=='****')
			if (pList->fCreator==fCreator || pList->fCreator=='****')
				return true;	//	found
		}
	}
	return false;
}