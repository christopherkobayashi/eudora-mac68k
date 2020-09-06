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

//light.c
//
//	contains all the functions I'm creating for the decaying light version of Eudora.

#include <conf.h>
#include <mydefs.h>

#include	"light.h"

#ifdef	LIGHT

void MarkAsProOnly(MenuHandle mh, short item);
void UnmarkAsProOnly(MenuHandle mh, short item);



/************************************************************************************
/*	Enable or disable a menu item that depends on whether this is a light version or not
/*
/*	mh, item				location of item to be enabled or disabled
/*	enabledIf				Boolean indicating if the item is enabled or not for a Pro Version
/*	ProOnlyFeature	Boolean representing if this item should be disabled because it's 
/*									a Pro Only feature
/***********************************************************************************/

void LightEnableIf(MenuHandle mh, short item, Boolean enabledIf, Boolean ProOnlyFeature)
{
	Boolean enabled = enabledIf && !ProOnlyFeature;

#ifdef	USED_FOR_DECAYING_VERSION	
	if (ProOnlyFeature)
		MarkAsProOnly(mh,item);
	else
		UnmarkAsProOnly(mh,item);
#endif	//USED_FOR_DECAYING_VERSION

	EnableIf(mh,item,enabled);
	return;
}




/***********************************************************************************
/*	Disable a menu item in a way that makes it obvious that it's a PRO only feature 
/*	ANy item disabled with this routine can be re-enabled with LightEnable
/***********************************************************************************/

void MarkAsProOnly(MenuHandle theMenu, short item)
{
	Str255	newMenuItemText, theSubString;

	DisableItem(theMenu, item);

	//First, add some text to the menu item to mark it as a pro only feature	
	theSubString[0] = 1;		//build the substring we'll add to the menu item.  Use the string from the resource
	theSubString[1] = 0;		//plus a NULL character, so we know it's really our text when we find it
	PCatR(theSubString, PRO_ONLY_FEATURE);
		
	GetMenuItemText(theMenu,item,newMenuItemText);
	if (PPtrFindSub(theSubString, newMenuItemText+1, newMenuItemText[0]) == 0)
	{
		PCat(newMenuItemText,theSubString);
		SetMenuItemText(theMenu,item,newMenuItemText);
	}
	
	return;
}

#endif	//LIGHT
