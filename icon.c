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

#include "icon.h"
#define FILE_NUM 62
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#pragma segment Icon

OSErr DT1GetIcon(short dtRef,OSType creator,OSType type,short iconType, Ptr data, short size);
pascal Handle ICacheGetter(ResType theType,ICacheHandle ich);

/**********************************************************************
 * PlotIconFromICache - plot an icon from a cache
 **********************************************************************/
OSErr PlotIconFromICache(ICacheHandle ich,IconTransformType transform,Rect *inRect)
{
	if (ich)
		return (PlotIconSuite(inRect,atAbsoluteCenter,transform,(*ich)->cache));
	else
		return (fnfErr);
}

/**********************************************************************
 * Names2Icon - turn names into an icon
 **********************************************************************/
short Names2Icon(PStr baseName,PStr modifierNames)
{
	short id = 0;
	Str255 wholeName;
	Handle resH;
	OSType junk;
	
	PCopy(wholeName,modifierNames);
	PSCat(wholeName,baseName);
	
	SetResLoad(False);
	if (!(resH = GetNamedResource('icns',wholeName)))
	if (!(resH = GetNamedResource('icns',baseName)))
	if (!(resH = GetNamedResource('ICN#',wholeName)))
	if (!(resH = GetNamedResource('ICN#',baseName)))
		;
	SetResLoad(True);
	
	if (!resH) return(0);
	
	GetResInfo(resH,&id,&junk,wholeName);
	
	return(id);
}
	
/**********************************************************************
 * ICacheToRgn - turn an icon cache into a region
 **********************************************************************/
OSErr ICacheToRgn(ICacheHandle ich,Rect *inRect,RgnHandle rgn)
{
	if (ich)
		return (IconSuiteToRgn(rgn,inRect,atAbsoluteCenter,(*ich)->cache));
	else
		return (fnfErr);
}

/**********************************************************************
 * ICacheGetter - all the work
 **********************************************************************/
pascal Handle ICacheGetter(ResType theType,ICacheHandle ich)
{
	OSErr err = noErr;
	Handle data,resH;
	typedef struct {short dtType; short size; OSType suiteType;} IconDesc, *IconDescPtr, **IconDescHandle;
	IconDescPtr idp;
	short dtRef;
	static IconDesc descList[] = {
		kLargeIcon,		kLargeIconSize,		large1BitMask,
		kLarge4BitIcon,	kLarge4BitIconSize,	large4BitData,
		kLarge8BitIcon,	kLarge8BitIconSize,	large8BitData,
		kSmallIcon,		kSmallIconSize,		small1BitMask,
		kSmall4BitIcon,	kSmall4BitIconSize,	small4BitData,
		kSmall8BitIcon,	kSmall8BitIconSize,	small8BitData
	};
	
	idp = descList + sizeof(descList)/sizeof(IconDesc) - 1;	// last item!
	while ( idp >= descList && idp->suiteType != theType )
		idp--;
	if ( idp < descList )
		return NULL;
	ASSERT ( idp->suiteType == theType );
	
	if (data = NuHandle(idp->size))
	{
		/*
		 * if we haven't found the right desktop file yet, find it now
		 */
		if (!(*ich)->dtRefValid)
		{
			//	Find the correct desktop file
			short volIndex;
			short vRef;
			
			for (volIndex=1;!IndexVRef(volIndex,&vRef);volIndex++)
				if (!DTRef(vRef,&dtRef))
					if (!DT1GetIcon(dtRef,(*ich)->creator,(*ich)->type,idp->dtType,LDRef(data),idp->size))
					{
						//	Found it here
						(*ich)->dtRef = dtRef;
						(*ich)->dtRefValid = true;
						break;
					}
		}
		else if ((*ich)->dtRef)
			err = DT1GetIcon((*ich)->dtRef,(*ich)->creator,(*ich)->type,idp->dtType,LDRef(data),idp->size);
		UL(data);
		
		/*
		 * did we fail?
		 */
		if (err || !(*ich)->dtRef)
		{
			// no icon
			// if that's because we didn't find the app, return generic icon
			// or, if we did find the app but for some reason its icons are damaged,
			// return generic icon if asking for the B&W icon, since that icon
			// should always be present
			if ((!(*ich)->dtRef || theType==large1BitMask) && (resH=GetResource(theType,(*ich)->type=='APPL' ?
					genericApplicationIconResource:genericDocumentIconResource)))
				BMD(*resH,*data,idp->size);
			else
				ZapHandle(data);	/* failure */
		}
	}
	return(data);
}

/**********************************************************************
 * GetICache - find (or create) the icon cache entry for a type and creator
 **********************************************************************/
ICacheHandle GetICache(OSType type,OSType creator)
{
	ICacheHandle ich;
	ICacheType ic;
	DECLARE_UPP(ICacheGetter,IconGetter);
	
	/*
	 * is it there already?
	 */
	if (ich=FindICache(type,creator)) return(ich);
	
	/*
	 * not there.  gotta make it
	 */
	if (ich=NewH(ICacheType))
	{
		Zero(ic);
		INIT_UPP(ICacheGetter,IconGetter);
		if (!MakeIconCache(&ic.cache,ICacheGetterUPP,ich))
		{
			ic.magic = 'iCaC';
			ic.type = type;
			ic.creator = creator;
			**ich = ic;
			LL_Push(ICache,ich);
		}
		else ZapHandle(ich);
	}
	return(ich);
}
#pragma segment Icon

/**********************************************************************
 * FindICache - find the icon cache entry for a type and creator
 **********************************************************************/
ICacheHandle FindICache(OSType type,OSType creator)
{
	ICacheHandle ich;
	
	for (ich=ICache;ich;ich=(*ich)->next)
	{
		if ((*ich)->type==type && (*ich)->creator==creator) break;
	}
	return(ich);
}

/**********************************************************************
 * DisposeICache - dispose of an entry in the ICache
 *  returns non-zero if bytes were disposed of
 **********************************************************************/
long DisposeICache(ICacheHandle ich)
{
	if (ich)
	{		
		/*
		 * pop it from the list
		 */
		LL_Remove(ICache,ich,(ICacheHandle));
		
		/*
		 * nuke the icon cache
		 */
		DisposeIconSuite((*ich)->cache,True);
		
		/*
		 * and now the whole thing
		 */
		ZapHandle(ich);
		
		/*
		 * found one!
		 */
		return(1);
	}
	
	return(0);
}

/************************************************************************
 * DTGetIconInfo - get icon info
 ************************************************************************/
OSErr DTGetIconInfo(short dtRef,OSType creator, short index,OSType *type)
{
	DTPBRec pb;
	short err;
	
	pb.ioCompletion = nil;
	pb.ioDTRefNum = dtRef;
	pb.ioIndex = index;
	pb.ioFileCreator = creator;
	
	if (err=PBDTGetIconInfo(&pb,False)) return(err);
	
	*type = pb.ioFileType;
	return(pb.ioResult);
}

/************************************************************************
 * DT1GetIcon - find an icon for a creator
 ************************************************************************/
OSErr DT1GetIcon(short dtRef,OSType creator,OSType type,short iconType, Ptr data, short size)
{
	DTPBRec pb;
	short err;
	Str63 appName;
	
	Zero(pb);
	pb.ioNamePtr = appName;
	pb.ioDTRefNum = dtRef;
	pb.ioFileCreator = creator;
	pb.ioFileType = type;
	pb.ioDTBuffer = data;
	pb.ioIconType = iconType;
	pb.ioDTReqCount = size;
	
	if (err=PBDTGetIcon(&pb,False)) return(err);
	return(pb.ioResult);
}

/**********************************************************************
 * NamedIconCalc - calc where things go in a named icon
 **********************************************************************/
void NamedIconCalc(Rect *inRect,PStr name,short hIndent,short vIndent,Rect *iconR,Point *textP)
{
	short hi = RectHi(*inRect);
	short wi = RectWi(*inRect);
	Rect inner = *inRect;
	short iHi;
	short	txFont = GetPortTextFont(GetQDGlobalsThePort());
	short	txSize = GetPortTextSize(GetQDGlobalsThePort());
	
	InsetRect(&inner,hIndent,vIndent);

	if (hi<14+2*vIndent)
	{
		/*
		 * no icons
		 */
		short lead = GetLeading(txFont,txSize);
		short descent = GetDescent(txFont,txSize);
		SetPt(textP,inner.left,inner.bottom - (inner.bottom-inner.top-lead)/2 - descent);
		SetRect(iconR,0,0,0,0);
	}
	else
	{
		if (hi<32+2*vIndent)
		{
			/*
			 * small icons
			 */
			iHi = hi-2*vIndent;
			iHi = MIN(iHi,16);
			
			SetRect(iconR,0,0,iHi,iHi);
			CenterRectIn(iconR,&inner);
			if (wi>30)
			{
				OffsetRect(iconR,inner.left-iconR->left,0);
				textP->h = iconR->right + INSET;
				textP->v = iconR->top + GetLeading(txFont,txSize);
			}
			else textP->h = textP->v = 0;
		}
		else
		{
			SetRect(iconR,0,0,32,32);
			if (hi>40)
			{
				TopCenterRectIn(iconR,&inner);
				textP->v = iconR->bottom + GetAscent(txFont,txSize);
				textP->h = inRect->left + (RectWi(*inRect)-StringWidth(name))/2;
				textP->h = MAX(textP->h,inRect->left+hIndent);
				hi = (RectHi(inner)-(textP->v+FontDescent-inner.top))/2;
				if (hi<0) hi = (RectHi(inner)-32)/2;
				if (hi>0)
				{
					textP->v += hi;
					OffsetRect(iconR,0,hi);
				}
			}
			else
			{
				CenterRectIn(iconR,&inner);
				textP->h = textP->v = 0;
			}
		}
	}
}

/**********************************************************************
 * DupIconSuite - copy all the data out of an icon suite
 **********************************************************************/
OSErr DupIconSuite(Handle fromSuite,Handle *toSuite,Boolean reuseSuite)
{
	OSErr err = noErr;
	Handle data, oldData;
	OSType types[] = {
		large1BitMask,
		large4BitData,
		large8BitData,
		small1BitMask,
		small4BitData,
		small8BitData,
		mini1BitMask,
		mini4BitData,
		mini8BitData };
	short index;

	
	if (!fromSuite) return(fnfErr);
	if (!reuseSuite) *toSuite = nil;
	
	if (*toSuite || !(err=NewIconSuite(toSuite)))
		for (index=sizeof(types)/sizeof(OSType);!err && index--;)
		{
			if (!GetIconFromSuite(&data,fromSuite,types[index]) &&
					data &&		//	GetIconFromSuite may return no err even when there is no icon available
					!(err=MyHandToHand(&data)))
			{
				oldData = nil;
				if (reuseSuite && !GetIconFromSuite(&oldData,*toSuite,types[index]) && oldData)
					ZapHandle(oldData);
				err = AddIconToSuite(data,*toSuite,types[index]);
			}
			else if (reuseSuite && !GetIconFromSuite(&oldData,*toSuite,types[index]) && oldData)
				ZapHandle(oldData);
		}

	
	if (err && !reuseSuite)
	{
		if (*toSuite) DisposeIconSuite(*toSuite,True);
	}
	else
		SetSuiteLabel(*toSuite,GetSuiteLabel(fromSuite));
	
	return(err);
}


/**********************************************************************
 * FSpGetCustomIconSuite - get a custom icon, if a file has one
 **********************************************************************/
Handle FSpGetCustomIconSuite(FSSpecPtr spec,short dataSelector)
{
	short iconRes, oldResFile = CurResFile();
	Handle rIconSuite, theIcon = nil;
	OSErr err;
	
	// open the file
	iconRes = FSpOpenResFile(spec,fsRdPerm);
	if (iconRes != -1)
	{										
		// read in the icon
		err = GetIconSuite(&rIconSuite, kCustomIconResource, dataSelector);
		if (rIconSuite && *rIconSuite && (err==noErr))
		{	
			err = DupIconSuite(rIconSuite, &theIcon, false);
			if (err == noErr)
			{
				HNoPurge(theIcon);
			}
		}
		CloseResFile(iconRes);
	}
	UseResFile(oldResFile);
	return theIcon;
}

/**********************************************************************
 * DupDeskIconSuite - copy all the data out of an icon suite
 **********************************************************************/
OSErr DupDeskIconSuite(OSType type, OSType creator,Handle *toSuite)
{
	OSErr err = noErr;
	Handle data;
	OSType types[] = {
		large1BitMask,
		large4BitData,
		large8BitData,
		small1BitMask,
		small4BitData,
		small8BitData };
	short index;
	ICacheHandle ich = NewZH(ICacheType);
	DECLARE_UPP(ICacheGetter,IconGetter);

	
	if (!ich) return(fnfErr);
	*toSuite = nil;
	
	(*ich)->type = type;
	(*ich)->creator = creator;
	
	INIT_UPP(ICacheGetter,IconGetter);
	if (!(err=NewIconSuite(toSuite)))
		for (index=sizeof(types)/sizeof(OSType);!err && index--;)
			if ((data=ICacheGetter(types[index],ich)) &&		//	GetIconFromSuite may return no err even when there is no icon available
					!(err=MyHandToHand(&data)))
				err = AddIconToSuite(data,*toSuite,types[index]);
	
	if (err)
	{
		if (*toSuite) DisposeIconSuite(*toSuite,True);
		*toSuite = nil;
	}
	
	return(err);
}

/************************************************************************
 * PlotTinyIconAtPenID - plot a very small icon as though it were a character
 * Icon is small icon, scaled
 ************************************************************************/
void PlotTinyIconAtPenID(IconTransformType transform,short icon)
{
	Rect tempR;
	Point	penLoc;
	
	GetPortPenLocation(GetQDGlobalsThePort(),&penLoc);
	SetRect(&tempR,penLoc.h,penLoc.v-8,penLoc.h+8,penLoc.v);
	PlotIconID(&tempR,atNone,transform,icon);
	Move(12,0);
}


/************************************************************************
 * PlotTinyULIconAtPenID - plot a very small icon as though it were a character
 * Icon lives in UL corner of small icon
 ************************************************************************/
void PlotTinyULIconAtPenID(IconTransformType transform,short icon)
{
	Rect tempR;
	RgnHandle newClip = NewRgn();
	RgnHandle oldClip;
	if (newClip)
	{
		Point	penLoc;
		
		GetPortPenLocation(GetQDGlobalsThePort(),&penLoc);	
		SetRect(&tempR,penLoc.h,penLoc.v-8,penLoc.h+8,penLoc.v);
		RectRgn(newClip,&tempR);
		oldClip = SavePortClipRegion(GetQDGlobalsThePort());
		SetPortClipRegion(GetQDGlobalsThePort(),newClip);
		SectRgn(newClip,oldClip,newClip);
		tempR.bottom = tempR.top+16;
		tempR.right = tempR.left+16;
		PlotIconID(&tempR,atNone,transform,icon);
		RestorePortClipRegion(GetQDGlobalsThePort(),oldClip);
		DisposeRgn(newClip);
	}
	Move(12,0);
}

/************************************************************************
 * RefreshRGBIcon - refresh a color icon
 ************************************************************************/
OSErr RefreshRGBIcon(short id,RGBColor *color,short template,Boolean *changed)
{
	Str31 nameShdBe;
	Str255 nameIs;
	Handle res;
	long type;
	OSErr err = noErr;
	Boolean addIt = false;
	Handle maskSuite = nil;
		
	SetResLoad(false);
	res=Get1Resource('ICN#',id);
	if (!res && !(err=ResError())) err = resNotFound;
/*
	if ( !res ) {
		err = ResError ();
		if ( !err )
			err = resNotFound;
		}
*/
	
	SetResLoad(true);
	
	if (err==resNotFound) addIt = true;
	else
	{
		GetResInfo(res,&id,&type,nameIs);
		Color2String(nameShdBe,color);
		addIt = !StringSame(nameShdBe,nameIs);
	}
	
	if (addIt)
	{
		if (template) GetIconSuite(&maskSuite, template, svAllAvailableData);
		err = RGBColorToIconFamily(color,id,nameShdBe,true,maskSuite);
		if (maskSuite) DisposeIconSuite(maskSuite,false);
	}
	
	if (changed) *changed = addIt && !err;
	
	return(err);
}


#ifdef LABEL_ICONS

#define	kicl8Size		1024
#define	kics8Size		256
#define	kicm8Size		192

#define	kicl4Size		512
#define	kics4Size		128
#define	kicm4Size		96

#define	kICNSize		256
#define	kicnSize		64
#define	kicmSize		48


OSErr	RGBColorToNBitIcon( RGBColor* theColor, short bitDepth, short iconID, ConstStr255Param name, Boolean overWrite, Handle maskSuite );
OSErr	GetNBitColors( short bitDepth, RGBColor* theColor, Byte maskIndex, Byte* colorIndex, Byte* compIndex, Boolean computeComposite );
void	BuildNBitIconFromMask( Handle icon, Handle mask, Byte colorIndex, Byte compIndex, short bitDepth, short size );

OSErr RGBColorToIconFamily( RGBColor* theColor, short iconID, ConstStr255Param name, Boolean overWrite, Handle maskSuite )
{
	OSErr theErr;

	theErr = RGBColorToNBitIcon( theColor, 1, iconID, name, overWrite, maskSuite );
	if( theErr ) return( theErr );

	theErr = RGBColorToNBitIcon( theColor, 4, iconID, name, overWrite, maskSuite );
	if( theErr ) return( theErr );

	theErr = RGBColorToNBitIcon( theColor, 8, iconID, name, overWrite, maskSuite );
	if( theErr ) return( theErr );

	return( noErr );
}
	
OSErr RGBColorToNBitIcon( RGBColor* theColor, short bitDepth, short iconID, ConstStr255Param name, Boolean overWrite, Handle maskSuite )
{
	enum
	{
		depth8,
		depth4,
		depth1
	};
	enum
	{
		large,
		small,
		mini,
		numIconSizes
	};
	OSErr theErr;
	short tmpSize;
	Byte colorIndex, maskIndex=0, compIndex=0;
	short depthIndex, i;
	Handle icons[numIconSizes], masks[numIconSizes], tmpRes, mask;
	static struct IconInfo{ short depth;	long type[numIconSizes];							short size[numIconSizes]; }
		iconInfo[] = {		depth8,			{kLarge8BitData,	kSmall8BitData,	kMini8BitData},	{kicl8Size,	kics8Size,	kicm8Size},
							depth4,			{kLarge4BitData,	kSmall4BitData,	kMini4BitData},	{kicl4Size,	kics4Size,	kicm4Size},
							depth1,			{kLarge1BitMask,	kSmall1BitMask,	kMini1BitMask},	{kICNSize,	kicnSize,	kicmSize } };
	
	switch( bitDepth )
	{
		case 8:
			depthIndex = depth8;
			break;
		case 4:
			depthIndex = depth4;
			break;
		case 1:
			depthIndex = depth1;
			break;
		default:
			return( paramErr );
			break;
	}
	
	if( maskSuite )
	{
		for( i=0; i<numIconSizes; i++ )
		{
			theErr = GetIconFromSuite( &masks[i], maskSuite, iconInfo[depthIndex].type[i] );
			switch( theErr )
			{
				case noErr:
					break;
				case paramErr:
					masks[i] = nil;
					break;
				default:
					return( theErr );
					break;
			}
		}
	}
	
	for( i=0; i<numIconSizes; i++ )
	{
		icons[i] = NuHTempBetter( iconInfo[depthIndex].size[i] );
		if( !icons[i] ) return( MemError() );
	}

	//get index for composite color (between mask and main color)
	//first, find a valid mask
	tmpSize = 0;
	mask = nil;
	for( i=0; i<numIconSizes; i++ )
	{
		if( masks[i] )
		{
			tmpSize = iconInfo[depthIndex].size[i];
			mask = masks[i];
		}
	}
	
	//find a pixel in the mask that isn't white
	for( i=0; i<tmpSize; i++ )
		if( (*mask)[i] )
		{
			maskIndex = (*mask)[i];
			i = tmpSize;
		}

	theErr = GetNBitColors( bitDepth, theColor, maskIndex, &colorIndex, &compIndex, mask != nil );
	if( theErr ) return( theErr );
	
	//build each icon
	for( i=0; i<numIconSizes; i++ )
		BuildNBitIconFromMask( icons[i], masks[i], colorIndex, compIndex, bitDepth, iconInfo[depthIndex].size[i] );
		
	for( i=0; i<numIconSizes; i++ )
	{
		//if overWrite is enabled, then deleted any duplicates
		if( overWrite )
		{
			if( tmpRes = Get1Resource( iconInfo[depthIndex].type[i], iconID ) )
			{
				RemoveResource( tmpRes );
				theErr = ResError(); if( theErr ) return( theErr );
			}
		}
		//if no overwrite, then return a duplicate file name error if there are already such resources
		else
		{
			tmpRes = Get1Resource( iconInfo[depthIndex].type[i], iconID );
			if( tmpRes ){ ReleaseResource( tmpRes ); return( dupFNErr ); }
		}
	}
	
	//add the resource
	for( i=0; i<numIconSizes; i++ )
	{
		if( !name )
			AddResource( icons[i], iconInfo[depthIndex].type[i], iconID, "\p" );
		else
			AddResource( icons[i], iconInfo[depthIndex].type[i], iconID, name );
		theErr = ResError(); if( theErr ) return( theErr );
		WriteResource( icons[i] );
		theErr = ResError(); if( theErr ) return( theErr );
		SetResAttrs( icons[i], resPurgeable );
		theErr = ResError(); if( theErr ) return( theErr );
	}

	return( noErr );
}

OSErr GetNBitColors( short bitDepth, RGBColor* theColor, Byte maskIndex, Byte* colorIndex, Byte* compIndex, Boolean computeComposite )
{
	OSErr theErr;
	GWorldPtr offscreenGWorld;
	CGrafPtr oldPort;
	GDHandle oldGD;
	Rect smallRect={0,0,1,1};
	RGBColor tmpRGB;

	GetGWorld( &oldPort, &oldGD );

	theErr = NewGWorld( &offscreenGWorld, bitDepth, &smallRect, nil, nil, 0 );

	SetGWorld( offscreenGWorld, nil );

	*colorIndex = Color2Index( theColor );
	if( computeComposite )
	{
		Index2Color ( maskIndex, &tmpRGB );
		
		//get a composite of the mask color and the desired color index
		tmpRGB.red = ( tmpRGB.red + theColor->red )/2;
		tmpRGB.green = ( tmpRGB.green + theColor->green )/2;
		tmpRGB.blue = ( tmpRGB.blue + theColor->blue )/2;
		
		*compIndex = Color2Index( &tmpRGB );
	}
	
	SetGWorld( oldPort, oldGD );
	
	DisposeGWorld( offscreenGWorld );

	return( theErr );
}

void BuildNBitIconFromMask( Handle icon, Handle mask, Byte colorIndex, Byte compIndex, short bitDepth, short size )
{
	short i, j;
	Byte byteVal;

	if( mask )
	{
		switch( bitDepth )
		{
			case 1:
				//bit by bit...
				for( i=0; i<size/2; i++ )
				{
					(*icon)[i] = 0x00;
					for( j=0; j<8; j++ )
					{
						byteVal = 1 << j;
						if( (*mask)[i] & byteVal )
							(*icon)[i] |= byteVal;
						else
							(*icon)[i] |= ( colorIndex << j );
					}
				}
				//copy the mask
				for( ; i<size; i++ )
					(*icon)[i] = (*mask)[i];
				break;
			case 4:
				for( i=0; i<size; i++ )
				{
					//clear all of the bits
					(*icon)[i] = 0x00;
					
					//set first 4 bits
					if( (*mask)[i] & 0x0f )
						(*icon)[i] |= ( compIndex & 0x0f );
					else
						(*icon)[i] |= ( colorIndex & 0x0f );

					//set 2nd 4 bits
					if( (*mask)[i] & 0xf0 )
						(*icon)[i] |= ( ( compIndex << 4 ) & 0xf0 );
					else
						(*icon)[i] |= ( ( colorIndex << 4 ) & 0xf0 );
				}
				break;
			case 8:
				//thank heaven for whole bytes!
				for( i=0; i<size; i++ )
					if( (*mask)[i] )
						(*icon)[i] = compIndex;
					else
						(*icon)[i] = colorIndex;
				break;
		}
	}
	//for no mask, just set all the bytes in the icon
	else
	{
		switch( bitDepth )
		{
			case 1:
				//it's all or nothing
				if( colorIndex )
					byteVal = 0xff;
				else
					byteVal = 0x00;
				
				for( i=0; i<size/2; i++ )
					(*icon)[i] = byteVal;
				
				//make (outgoing) icon mask all black
				for( ; i<size; i++ )
					(*icon)[i] = 0xff;
				break;
			case 4:
				//2 pixels per byte
				byteVal = (colorIndex << 4) | colorIndex;
				for( i=0; i<size; i++ )
					(*icon)[i] = byteVal;
				break;
			case 8:
				//1 pixel per byte
				for( i=0; i<size; i++ )
					(*icon)[i] = colorIndex;
				break;
		}
	}
}
#endif

#ifdef DEBUG
#undef PlotIconID
/************************************************************************
 * MyPlotIconID - ploticonid pacifying Spotlight
 ************************************************************************/
OSErr MyPlotIconID (Rect *theRect, IconAlignmentType align, IconTransformType transform, short theResID)
{
	OSErr err;
	SLDisable();
	err = PlotIconID(theRect,align,transform,theResID);
	SLEnable();
	return(err);
}
#endif