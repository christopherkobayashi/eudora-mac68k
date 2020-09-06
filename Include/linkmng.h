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

#ifndef LINKMNG_H
#define LINKMNG_H

/* Copyright (c) 1999 by QUALCOMM Incorporated */

/**********************************************************************
 *  routines to manage multiple link history lists
 **********************************************************************/

/* LinkSortTypeEnum - types of sorts we can do */
typedef enum 
{
	sType=0x0000,
	sName=0x0001,
	sDate=0x0002,
	sSpecialRemind=0xFFFF,
	LinkSortTypeLimit
} LinkSortTypeEnum;

/* LinkSortMaskEnum - modifications of sorts we can do */
typedef enum
{
	kLinkSortTypeMask=0x0000FFFF,			// sort type mask
	kLinkHasNoCustomIcons=0x00010000,		// no custom icons
	kLinkReverseSort=0x00020000,			// reverse sort order
	LinkSortModLimit
} LinkSortMaskEnum;


/* Offline Link Dialog Actions - errors returned by OpenURL we are interested in */
enum
{
	oldaCancel = 4747,	// user pressed cancel in offline link dialog
	oldaBookmark,		// bookmark the link
	oldaRemind			// remind the user to visit later
};

// Link management
OSErr GenHistoriesList(void);
void ZapHistoriesList(Boolean destroy);
OSErr AddURLToMainHistory(PStr url, PStr name, OSErr urlOpenErr);
OSErr AddAdToLinkHistory(AdId adId, StringPtr pUrl, Str255 adTitle, FSSpecPtr adGraphic);
OSErr SaveAllHistoryFiles(void);
void AdWasClicked(AdId adId, OSErr openErr);
void AgeLinks(void);

// Link Window related
void AddAllHistoryItems(ViewListPtr pView, Boolean needsSort, LinkSortTypeEnum sortType);
void DeleteHistoryEntry(VLNodeInfo *info);
OSErr OpenHistoryEntry(VLNodeInfo *info);
Boolean GetDateString(VLNodeID id, Str255 dateStr);
Handle GetLinkURL(VLNodeInfo *info);
Handle GetLHPreviewIcon(VLNodeID id);
void ZapPVICache(void);

// Offline Link Dialog related
Boolean IsMarkedRemind(VLNodeID id);
Boolean FindRemindLink(void);
void UnRemindLinks(Boolean labelToo);

// Icon functions
OSErr MakeIconSuite (GWorldPtr gWorld, Rect *pRect, RGBColor *transparentColor, PStr name);
Handle MakeIcon(GWorldPtr srcGWorld, Rect *srcRect, short dstDepth, short iconSize);
Handle MakeICN_pound(GWorldPtr gwp, Rect *srcRect, short iconDimension, RGBColor *transparentColor);

#endif
