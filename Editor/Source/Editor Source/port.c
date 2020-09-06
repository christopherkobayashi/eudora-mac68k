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

#include "petepch.h"

void SavePortInfo(CGrafPtr newPort, PETEPortInfoPtr savedInfo)
{
	CGrafPtr wMgrPort;
	
	if(newPort == nil) {
		GetWMgrPort((GrafPtr *)&wMgrPort);
		newPort = wMgrPort;
	}
	GetGWorld(&savedInfo->oldPort, &savedInfo->oldGDevice);
	SetGWorld(newPort, nil);
	
	savedInfo->textInfo.tsFont = GetPortTextFont(newPort);
	savedInfo->textInfo.tsFace = GetPortTextFace(newPort);
	savedInfo->textInfo.tsSize = GetPortTextSize(newPort);
	GetPenState(&savedInfo->pnState);
	GetForeColor(&savedInfo->textInfo.tsColor);
	GetBackColor(&savedInfo->backColor);
}

void ResetPortInfo(PETEPortInfoPtr savedInfo)
{
	GrafPtr curPort;

	SetPenState(&savedInfo->pnState);
	TextFont(savedInfo->textInfo.tsFont);
	TextFace(savedInfo->textInfo.tsFace);
	TextSize(savedInfo->textInfo.tsSize);
	GetPort(&curPort);
	RGBForeColor(&savedInfo->textInfo.tsColor);
	RGBBackColorAndPat(&savedInfo->backColor);
	SetGWorld(savedInfo->oldPort, savedInfo->oldGDevice);
}

void RGBBackColorAndPat(RGBColor *color)
{
	Pattern	white;
	RGBBackColor(color);
	BackPat(GetQDGlobalsWhite(&white));
}

/* Set the QuickDraw text attributes */
void SetTextStyle(MyTextStylePtr theStyle, Boolean doColor)
{
	/* Set the font info */
	TextFont(theStyle->tsFont);
	TextFace(theStyle->tsFace);
	TextSize(theStyle->tsSize);
	
	/* If the color needs to be set (e.g. it's not a picture), set it */
	if(doColor) {
		RGBForeColor(&theStyle->tsColor);
	}
}

Boolean GetLabelStyle(StyleInfoPtr theStyle, PETELabelStyleHandle labelStyles, RGBColor defaultColor);
Boolean GetLabelStyle(StyleInfoPtr theStyle, PETELabelStyleHandle labelStyles, RGBColor defaultColor)
{
	long index;
	StyleInfo tempStyle;
	PETELabelStylePtr labelStyle;
	RGBColor *labelColor;
	Fixed weight, lWeight;
	
	for(index = InlineGetHandleSize((Handle)labelStyles) / sizeof(PETELabelStyleEntry); --index >= 0L; ) {
		labelStyle = &(*labelStyles)[index];
		if(theStyle->textStyle.tsLabel == labelStyle->plLabel) {
			if(labelStyle->plValidBits & peColorValid) {
				if(labelStyle->plColorWeight > 0) {
					if(IsPETEDefaultColor(labelStyle->plColor)) {
						labelColor = &defaultColor;
					} else {
						labelColor = &labelStyle->plColor;
					}
					if(labelStyle->plColorWeight != 100) {
						weight = FixRatio(100, labelStyle->plColorWeight);
						lWeight = FixRatio(100, 100 - labelStyle->plColorWeight);
						theStyle->textStyle.tsColor.red = FixDiv(theStyle->textStyle.tsColor.red, lWeight) + FixDiv(labelColor->red, weight);
						theStyle->textStyle.tsColor.green = FixDiv(theStyle->textStyle.tsColor.green, lWeight) + FixDiv(labelColor->green, weight);
						theStyle->textStyle.tsColor.blue = FixDiv(theStyle->textStyle.tsColor.blue, lWeight) + FixDiv(labelColor->blue, weight);
					} else {
						theStyle->textStyle.tsColor = *labelColor;
					}
				}
			}
			tempStyle.textStyle.tsFont = labelStyle->plFont;
			tempStyle.textStyle.tsFace = labelStyle->plFace;
			tempStyle.textStyle.tsSize = labelStyle->plSize;
			ChangeStyleInfo(&tempStyle, theStyle, (labelStyle->plValidBits & ~(peColorValid | peLangValid | peLockValid | peLabelValid)), false);
			return true;
		}
	}
	return false;
}

void SetTextStyleWithDefaults(PETEGlobalsHandle globals, DocumentInfoHandle docInfo, MyTextStylePtr theStyle, Boolean doColor, Boolean printing)
{
	StyleInfo realStyle;
	RGBColor defaultColor;
	
	defaultColor = DocOrGlobalColor(globals, docInfo);
	realStyle.textStyle = *theStyle;
	if(IsPETEDefaultColor(realStyle.textStyle.tsColor)) {
		realStyle.textStyle.tsColor = defaultColor;
	}
	
	if((docInfo != nil) && ((**docInfo).labelStyles != nil)) {
		if(!GetLabelStyle(&realStyle, (**docInfo).labelStyles, defaultColor)) {
			goto TryGlobals;
		}
	} else {
	TryGlobals :
		if((globals == nil) && (docInfo != nil)) {
			globals = (**docInfo).globals;
		}
		if((globals != nil) && ((**globals).labelStyles != nil)) {
			GetLabelStyle(&realStyle, (**globals).labelStyles, defaultColor);
		}
	}
	
	/* Do size before font because size might depend on original font */
	realStyle.textStyle.tsSize = StyleToFontSize(globals, docInfo, &realStyle.textStyle, printing);
	realStyle.textStyle.tsFont = StyleToFont(globals, docInfo, &realStyle.textStyle, printing);
	
	SetTextStyle(&realStyle.textStyle, doColor);
}

void PenAndColorNormal()
{
	RGBColor resetColor;
	
	PenNormal();
	resetColor.red = 0;
	resetColor.green = 0;
	resetColor.blue = 0;
	RGBForeColor(&resetColor);
}

void SetStyleAndKeyboard(DocumentInfoHandle docInfo, MyTextStylePtr newStyle)
{
	ScriptCode scriptCode;
	long scriptCount;
	LastScriptFontEntry lastFont;
	
	(**docInfo).curTextStyle = *newStyle;
	(**docInfo).curTextStyle.tsLabel = 0;
	
	if((**(**docInfo).globals).flags.hasMultiScript) {
	
		scriptCode = StyleToScript(newStyle);
		
		lastFont.theScript = scriptCode;
		lastFont.theFont = newStyle->tsFont;
		
		scriptCount = (**(**docInfo).lastFont).scriptCount;
		while(--scriptCount >= 0L) {
			if((**(**docInfo).lastFont).scriptFontList[scriptCount].theScript == scriptCode) {
				(**(**docInfo).lastFont).scriptFontList[scriptCount].theFont = newStyle->tsFont;
				break;
			}
		}
		if(scriptCount < 0L) {
			PtrAndHand(&lastFont, (Handle)(**docInfo).lastFont, sizeof(LastScriptFontEntry));
			++(**(**docInfo).lastFont).scriptCount;
		}
	
		if(((**docInfo).flags.isActive) && (scriptCode != (short)GetScriptManagerVariable(smKeyScript))) {
			KeyScript(scriptCode);
		}
	}
}