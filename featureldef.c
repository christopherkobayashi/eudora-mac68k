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

#define FILE_NUM 129

#pragma segment Nagging

void FeatureListDraw(Boolean lSelect,Rect *lRect,Cell lCell,ListHandle lHandle);

pascal void FeatureListDef(short lMessage, Boolean lSelect, Rect *lRect, Cell lCell,
	short lDataOffset, short lDataLen, ListHandle lHandle)
{
	switch (lMessage) {
		case lDrawMsg:
			FeatureListDraw (lSelect, lRect, lCell, lHandle);
			break;
		case lCloseMsg:
			if ((*lHandle)->userHandle)
				ZapHandle ((*lHandle)->userHandle);
			break;
	}
}

void FeatureListDraw (Boolean lSelect, Rect *lRect, Cell lCell, ListHandle lHandle)

{
	FeatureCellPtr	featureCellsPtr;
	Rect						iconRect;
	Point			portPenLoc;

	SAVE_STUFF;
	ClipRect (lRect);
	EraseRect (lRect);
	SmallSysFontSize ();
	MoveTo (lRect->left + (*lHandle)->indent.h + 4, lRect->bottom - 3 - (*lHandle)->indent.v);
	featureCellsPtr	= (FeatureCellPtr) LDRef ((*lHandle)->userHandle) + lCell.v;
	if (featureCellsPtr->isSubFeature)
		Move (kSubFeatureIndent, 0);
	if (!featureCellsPtr->isName)
		Move (kDescriptionIndent, -kVerticalDescriptionSqueeze);
	if (featureCellsPtr->used || featureCellsPtr->type == featureTechSupportType) {
		GetPortPenLocation(GetQDGlobalsThePort(),&portPenLoc);
		iconRect.left		= portPenLoc.h;
		iconRect.top		= lRect->top + ((lRect->bottom - lRect->top) >> 1) - (kCheckmarkIconSize >> 1);
		iconRect.right	= iconRect.left + kCheckmarkIconSize;
		iconRect.bottom	= iconRect.top + kCheckmarkIconSize;
		PlotIconID (&iconRect, atAbsoluteCenter, ttNone, featureCellsPtr->type == featureTechSupportType ? TECH_SUPPORT_ICON : MESSAGE_STATUS_ICON_CHECKMARK);
	}
	Move (kCheckmarkIconSize + 4, 0);
	GetPortPenLocation(GetQDGlobalsThePort(),&portPenLoc);
	
	if (featureCellsPtr->isName)
		TextFace (GetPortTextFace(GetQDGlobalsThePort()) | bold);
	if (portPenLoc.h + StringWidth (featureCellsPtr->description) > lRect->right)
		TextFace (GetPortTextFace(GetQDGlobalsThePort()) | condense);
	TruncString (lRect->right - portPenLoc.h, featureCellsPtr->description, truncMiddle);
	DrawString (featureCellsPtr->description);
	if (!featureCellsPtr->isName && lCell.v < (*lHandle)->dataBounds.bottom - 1) {
		SetForeGrey (k8Grey2);
		MoveTo (lRect->left + (*lHandle)->indent.h + 4, lRect->bottom - (*lHandle)->indent.v - 2);
		LineTo (lRect->right - (*lHandle)->indent.h - 4, lRect->bottom - (*lHandle)->indent.v - 2);
		SetForeGrey (k8Grey1);
		MoveTo (lRect->left + (*lHandle)->indent.h + 4, lRect->bottom - (*lHandle)->indent.v - 1);
		LineTo (lRect->right - (*lHandle)->indent.h - 4, lRect->bottom - (*lHandle)->indent.v - 1);
	}
	UL ((*lHandle)->userHandle);
	REST_STUFF;
	InfiniteClip (GetQDGlobalsThePort());
}
