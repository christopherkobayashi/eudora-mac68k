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

#ifndef MAKEFILTER_H
#define MAKEFILTER_H

/* Items of interest in the MakeFilter dialog */
enum {
	OK_BUTTON = 1,
	MF_CANCEL_BUTTON = 2,
	MF_DETAILS_BUTTON = 3,
	MF_MATCH = 4,
	MF_INCOMING_CHECKBOX = 5,
	MF_OUTGOING_CHECKBOX = 6,
	MF_MANUAL_CHECKBOX = 7,
	MF_FROM_RADIO = 8,
	MF_ANYR_RADIO = 9,
	MF_SUBJECT_RADIO = 10,
	MF_FROM_EDIT = 11,
	MF_ANYR_EDIT = 12,
	MF_SUBJECT_EDIT = 13,
	MF_ACTION = 14,
	MF_DELETE_RADIO = 15,
	MF_TRANSFERNEW_RADIO = 16,
	MF_TRANSFEREXISTING_RADIO = 17,
	MF_TRANSFERNEW_EDIT = 18,
	MF_IN = 19,
	MF_DEFAULT = 20,
	MF_TRANSFEREXISTING_BUTTON = 21,
	MF_ANYR_POPUP = 22
};

/* errors we could see during Make Filter */
enum {
	errMakeFilterOpen = 1,
	errScanningMbox,
	errNoCommonAddresses,
	errNoCommonElements,
	errCantSaveFilter
};

/* Values we want to put in and take out of the MakeFilter dialog */
typedef struct MakeFilterRec {
	Boolean incoming;
	Boolean outgoing;
	Boolean manual;
	short match;
	short action;
	Str255 subjectText;
	Str255 fromText;
	Str255 fromRealName;
	Str255 anyRText;
	short anyRSelection;
	Str255 defaultText;
	FSSpec transferToNew;
	FSSpec transferToExisting;
} MakeFilterRec, *MakeFilterRecPtr;

/* Interface to MakeFilter */

//Do everything involved in Make Filter.
void DoMakeFilter(MyWindowPtr win);

//Like chooseMailbox, but allow the user to select a folder.
Boolean ChooseMailboxFolder(short menu, short msg, UPtr name);

#endif				//MAKEFILTER_H
