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

#include <conf.h>
#include <mydefs.h>

#include "features.h"
#define FILE_NUM 121

#pragma segment Main

#ifndef DEATH_BUILD
//
//      Right now we pull this out of the resource fork...
//      We should probably hard code this (maybe).  Either that,
//      or don't call it if we're running as Eudora Light.
//

void RegisterProFeatures(void)
{
	FeatureResHandle resFeature;
	ResType resType;
	Str255 resName;
	short oldResFile, resCount, resID, index;
	// Two sort of kind of dummy features...
	RegisterFeature(featureNone, featureNone, featNoResource, false);
	RegisterFeature(featureEudoraPro, featureMatchAny, featNoResource,
			false);

	oldResFile = CurResFile();
	UseResFile(AppResFile);

	// The "real" features...
	resCount = Count1Resources(featureResourceType);
	for (index = 0; index < resCount; ++index)
		if (resFeature =
		    Get1Resource(featureResourceType,
				 rFeatureBaseResID + index)) {
			GetResInfo(resFeature, &resID, &resType, resName);
			ASSERT(resID == 1000 + index);
			RegisterFeature((*resFeature)->primary,
					(*resFeature)->sub, resID,
					(*resFeature)->hasSubFeatures);
			ReleaseResource(resFeature);
		}
	// Whoops!  We don't support this on the Mac
	DisableFeature(featureSimultaneousDirService);

	// clip2phone doesn't exist yet
	DisableFeature(featureClip2Phone);

	UseResFile(oldResFile);
}



//
//      RegisterFeature
//
//              Registers a feature and also enables it for use.  If you'd like Eudora to know about
//              a certain feature, but you do not want this feature to be used, you'll need to call
//              DisableFeature to turn it off.
//

void RegisterFeature(FeatureType primary, FeatureType sub, short resID,
		     Boolean hasSubFeatures)
{
	FeatureRec newFeature;
#ifdef OLD_WAY_TO_REGISTER_FEATURES
	FeatureRecPtr featurePtr;
#endif
	OSErr theError;

	// Registered features are present and enabled to start.
	newFeature.primary = primary;
	newFeature.sub = sub;
	newFeature.flags = featurePresent | featureEnabled;
	newFeature.lastUsed = 0;
	newFeature.resID = resID;
	newFeature.hasSubFeatures = hasSubFeatures;

	theError = noErr;
	if (!gFeatureList)
		gFeatureList = NuHandle(0);

	if (gFeatureList) {
#ifdef OLD_WAY_TO_REGISTER_FEATURES
		featurePtr = FindFeatureType(gFeatureList, primary, sub);

		// If we didn't find the feature we want to grow the feaure list and tack the new feature onto the end
		if (!featurePtr)
			PtrPlusHand(&newFeature, gFeatureList,
				    sizeof(newFeature));
		else
			BlockMoveData(&newFeature, *gFeatureList,
				      sizeof(newFeature));
#else
		PtrPlusHand(&newFeature, gFeatureList, sizeof(newFeature));
#endif
	}
}


//
//      UpdateFeatureUsage
//
//              - Read in the 'F U ' resource
//              - For each record...
//                              ...mark it used.  Simple!
//

void UpdateFeatureUsage(FeatureRecHandle featureSet)
{
	FeatureUsageHandle featureUsage;
	FeatureUsagePtr usagePtr;
	short count;

	if (featureUsage =
	    GetResourceFromFile(featureUsageResourceType,
				rFeatureUsageResID, SettingsRefN)) {
		if (usagePtr = LDRef(featureUsage))
			for (count =
			     GetHandleSize(featureUsage) /
			     sizeof(FeatureUsageRec); count; --count) {
				if (usagePtr->lastUsed)
					UseFeatureType(usagePtr->type);
				++usagePtr;
			}
		UL(featureUsage);
		ReleaseResource(featureUsage);
	}
}


//
//      SaveFeaturesWithExtemeProfanity
//
//              Create and save a 'F U ' resource to the Settings File
//

void SaveFeaturesWithExtemeProfanity(FeatureRecHandle featureSet)
{
	FeatureUsageHandle featureUsage;
	FeatureUsagePtr usagePtr;
	FeatureRecPtr featurePtr;
	short count;

	// First build a fresh FeatureUsageHandle
	count = GetHandleSize(gFeatureList) / sizeof(FeatureRec);
	if (featureUsage = NuHandle(count * sizeof(FeatureUsageRec))) {
		featurePtr = LDRef(gFeatureList);
		usagePtr = LDRef(featureUsage);
		for (; count; --count) {
			usagePtr->type =
			    featurePtr->sub ==
			    '????' ? featurePtr->primary : featurePtr->sub;
			usagePtr->lastUsed = featurePtr->lastUsed;
			++featurePtr;
			++usagePtr;
		}
		UL(gFeatureList);
		UL(featureUsage);

		// Write the FeatureUsageHandle into the Settings File
		ZapResource(featureUsageResourceType, rFeatureUsageResID);
		AddMyResource((Handle) featureUsage,
			      featureUsageResourceType, rFeatureUsageResID,
			      "\p");
		UpdateResFile(SettingsRefN);	// You don't add a resource and then release it
		// unless you update the file first.
		// A) It doesn't get the resource saved unless you're lucky
		// B) Settings file may die.  BAD BAD BAD BAD.
		ReleaseResource(featureUsage);
	}
}



//
//      HasFeature
//
//              Test to see whether or not a particular feature is present and enabled
//

Boolean HasFeature(FeatureID feature)
{
#ifdef LIGHT
	return (false);
#else
	FeatureRecPtr featurePtr;
	Boolean hasFeature;

	hasFeature = false;
	// Simple right now since we don't downgrade on the fly...  Eventually we'll
	// really want to look at the featureEnabled bit (it's always on now).
	if (!IsFreeMode())
		if (featurePtr = FindFeatureID(gFeatureList, feature))
			hasFeature =
			    featurePtr->
			    flags & featureEnabled ? true : false;
	return (hasFeature);
#endif
}


//
//      DisableFeature
//
//              Called to turn off a particular feature.
//

void DisableFeature(FeatureID feature)
{
	FeatureRecPtr featurePtr;

	if (gFeatureList)
		if (featurePtr = FindFeatureID(gFeatureList, feature))
			featurePtr->flags &= ~featureEnabled;
}


//
//      EnableFeature
//
//              Called to turn on a particular feature.
//

void EnableFeature(FeatureID feature)
{
	FeatureRecPtr featurePtr;

	if (gFeatureList)
		if (featurePtr = FindFeatureID(gFeatureList, feature))
			featurePtr->flags |= featureEnabled;
}


//
//      UseFeature
//
//              This routine should be called each time a feature is used to timestamp
//              the filter's usage information in memory.  Be careful, though... we don't
//              want to call this too frequently to the point of impacting performance.
//

void UseFeature(FeatureID feature)
{
	FeatureRecPtr featurePtr;

	if (gFeatureList)
		if (featurePtr = FindFeatureID(gFeatureList, feature)) {
			featurePtr->lastUsed = GMTDateTime();
			NotifyDownGradeDialog();
		}
}


//
//      UseFeatureType
//
//              Like UseFeature, only much less efficient!
//

void UseFeatureType(FeatureType feature)
{
	FeatureRecPtr featurePtr;

	if (gFeatureList)
		if (featurePtr =
		    FindFeatureType(gFeatureList, feature, feature)) {
			featurePtr->lastUsed = GMTDateTime();
			NotifyDownGradeDialog();
		}
}

//
//      FindFeatureType
//
//              Find a particular feature record in a feature set, by passing in a feature type.
//              This is the slow and laborious way to search for a given feature.  Upon closer
//              examination, it's linear.  This routine is meant to only be used by the feature
//              manager, and even then probably shouldn't ever really be used.
//

FeatureRecPtr FindFeatureType(FeatureRecHandle featureSet,
			      FeatureType primary, FeatureType sub)
{
	FeatureRecPtr featurePtr;
	short count;

	count = GetHandleSize(featureSet) / sizeof(FeatureRec);
	featurePtr = *featureSet;
	while (count--) {
		if (primary == sub) {
			if (featurePtr->primary == primary
			    || featurePtr->sub == primary)
				return (featurePtr);
		} else if (!primary || featurePtr->primary == primary)
			if (!sub || featurePtr->sub == sub)
				return (featurePtr);
		++featurePtr;
	}
	return (nil);
}


//
//      FindFeatureID
//
//              Find a particular feature record in a feature set, by passing in an index.
//              This is a much faster way to search for a given feature.  This routine is
//              meant to only be used by the feature manager.
//

FeatureRecPtr FindFeatureID(FeatureRecHandle featureSet, FeatureID feature)
{
	short count;

	count = GetHandleSize(featureSet) / sizeof(FeatureRec);
	return (feature < count ? *featureSet + feature : nil);
}

#define	SUBSTITUTE_ICON

void DisableMenuItemWithProOnlyIcon(MenuHandle theMenu, short item)
{
//      long    resp;
	Handle proOnlyIconHandle = nil;

	(void) GetIconSuite(&proOnlyIconHandle, PRO_ONLY_ICON,
			    svAllSmallData);
	if (proOnlyIconHandle)
		SetMenuItemIconHandle(theMenu, item, kMenuIconSuiteType,
				      proOnlyIconHandle);
	DisableItem(theMenu, item);
//#if TARGET_CPU_PPC
//              if (!Gestalt (gestaltMenuMgrAttr, &resp) && resp & gestaltMenuMgrPresent && (long) EnableMenuItemIcon != kUnresolvedCFragSymbolAddress)
//                      EnableMenuItemIcon (theMenu, item);
//#endif
}

//      Pass in enable:  -1 = do nothing  0 = disable  1 = enable

void SetMenuItemBasedOnFeature(MenuHandle theMenu, short item,
			       FeatureType feature, short enable)
{
#ifdef SUBSTITUTE_TEXT
	UPtr subString;
	Str255 proOnlyString, itemString;

	GetRString(proOnlyString, PRO_ONLY_FEATURE);
	GetMenuItemText(theMenu, item, itemString);
	subString =
	    PPtrFindSub(proOnlyString, itemString + 1, itemString[0]);

	if (HasFeature(feature)) {
		if (subString)
			itemString[itemString[0] =
				   subString - itemString] = 0;
		if (enable >= 0)
			if (enable)
				EnableItem(theMenu, item);
			else
				DisableItem(theMenu, item);
	} else {
		if (!subString) {
			PCat(itemString, proOnlyString);
			SetMenuItemText(theMenu, item, itemString);
		}
		DisableItem(theMenu, item);
	}
#else				// SUBSTITUTE_ICON
	if (HasFeature(feature)) {
		SetMenuItemIconHandle(theMenu, item, kMenuNoIcon, nil);
		if (enable >= 0)
			if (enable)
				EnableItem(theMenu, item);
			else
				DisableItem(theMenu, item);
	} else {
		DisableMenuItemWithProOnlyIcon(theMenu, item);
	}
#endif
}
#endif
