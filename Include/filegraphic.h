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

#ifndef PETEGRAPHIC_H
#define PETEGRAPHIC_H

/* Copyright (c) 1996 by Qualcomm, Inc. */

#define kFileInProcessType 0

//	File graphic flags
enum
{
	fgDisplayInline		= 0x0001,
	fgAttachment			= 0x0002,
	fgDontCopyToClip	= 0x0004,
	fgCenterInWindow	= 0x0008,
	fgEmoticon				= 0x0010
};

typedef struct
{
	//	Hashes to identify the part
	uLong	cid;
	uLong url;
	uLong withBase;
	uLong withSource;
	//	Image attributes
	short	height;
	short	width;
	short	border;
	short	hSpace;
	short	vSpace;
	short	pictResID;
	PicHandle	pictHandle;
	StringPtr	alt;	//	Just a ptr, not string itself. Valid only when passing in to PeteFileGraphicRange
	StringHandle absURL;
	HTMLAlignEnum align;
} HTMLGraphicInfo;

typedef struct GraphicFileRef
{
	AliasHandle alias;
	short	count;
} GraphicFileRef, *GFileRefPtr, **GFileRefHandle;

//	Info for QuickTime images. May be used by multiple graphics
//	in case of duplicates
typedef struct QTImageInfoStruct **QTImageHandle;
typedef struct QTImageInfoStruct
{
	QTImageHandle	next;
	GraphicsImportComponent importer;
	Handle hData;
	Component component;
	Rect		rBounds;
	FSSpec	spec;
	uLong	modDate;
	short	refCount;
} QTImageInfo, QTImagePtr;

typedef struct FileGraphicInfo
{
	PETEGraphicInfo pgi;
	FSSpec spec;
	OSType type;
	OSType creator;
	GFileRefHandle fileRef;
	Boolean displayInline;
	Boolean	noImage;
	Boolean	clone;
	Boolean	notDownloaded;
	Boolean	wasDownloaded;
	Boolean urlLink;
	Boolean attachment;
	Boolean	centerInWin;
	Boolean	isEmoticon;
 	HTMLGraphicInfo	htmlInfo;
	long	peteID;
	short	width, height;
	union
	{
		//	Data for each display type
		struct { Handle suite; IconRef iconRef; Boolean attachmentStub; Boolean sharedSuite;} icon;
		struct { PicHandle picture; Boolean spool; } pict;
		struct { QTImageHandle hQTImage; } image;
		struct { Movie theMovie; MovieController aController; Boolean animatedGraphic; } movie;
		struct { Handle hPlugin; } plugin;
	} u;
	Str127 name;
	GDHandle	gd;	
	CGrafPtr port;	//	May be offscreen port	
	Ptr pURLInfo;	//	For downloading graphics
	long	maxWidth;
} FileGraphicInfo, *FGIPtr, **FGIHandle;

#define kAcceptableGraphicDrag 'gd'

#ifdef ATT_ICONS
OSErr PeteFileGraphicRange(PETEHandle pte,long start,long stop,FSSpecPtr spec,long flags);
OSErr PeteFileGraphicStyle(PETEHandle pte,FSSpecPtr spec,HTMLGraphicInfo *html,PETEStyleEntryPtr pse,long flags);
#endif
Boolean IsGraphicFile(FSSpecPtr spec);
Boolean HaveQuickTime(short minVersion);
Boolean FindPart(StackHandle parts,uLong hash,PartDesc *pd,Boolean checkCID);
Boolean DisplayGetGraphics(MyWindowPtr win);
void GraphicDownloadIdle(void);
void GetCacheSpec(StringPtr sURL,FSSpec *spec,Boolean useAdCache);
void SetupPNGTransparency(GraphicsImportComponent importer,FSSpec *spec);
Boolean GetPNGTransColor(GraphicsImportComponent importer,FSSpec *spec,RGBColor *transColor);
OSErr FileGraphicChangeGraphic(PETEHandle pte,long offset,FSSpecPtr spec);
PicHandle SpecResPicture(FSSpecPtr spec);
pascal OSErr FileGraphic(PETEHandle pte,FGIHandle graphic,long offset,PETEGraphicMessage message,void *data);

#endif