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

#ifndef NICKEXP_H
#define NICKEXP_H

#include "nickmng.h"

#define EAL_VARS_DECL Str255 tempStr, junk, fmt, buffer, realname, shortAddress
#define EAL_VARS tempStr, junk, fmt, buffer, realname, shortAddress
#define EAL_VARS_FORM PStr tempStr, PStr junk, PStr fmt, PStr buffer, PStr realname, PStr shortAddress

typedef struct {
	short aliasIndex;
	short nicknameIndex;
	Str255 entryStr;
} NicknameTableEntry, *NicknameTableEntryPtr;

typedef struct {
	long numEntries;
	long logicalSize;
	long physicalSize;
	Str255 prefixStr;
	long refCon;
	NicknameTableEntry table[];
} NicknameTable, *NicknameTablePtr, **NicknameTableHdl;


void NEValidateWin(MyWindowPtr win);
void NEValidatePte(PETEHandle pte);

Boolean NicknameRawEqualString(ConstStr255Param str1,
			       ConstStr255Param str2);
void NicknameNormalizeString(Str255 theString, Boolean caseSens,
			     Boolean diacSens, Boolean stickySens);
Boolean NicknameEqualString(ConstStr255Param str1, ConstStr255Param str2,
			    Boolean caseSens, Boolean diacSens,
			    Boolean stickySens);
Boolean EqualStringPrefix(Str255 prefixStr, Str255 otherStr,
			  Boolean caseSens, Boolean diacSens,
			  Boolean stickySens);
Boolean CharIsTypingChar(unsigned char keyChar, unsigned char keyCode);
PStr GetNicknameEmailAddressPStr(short aliasIndex, short nicknameIndex,
				 PStr emailAddressStr);
PStr GetNicknamePhrasePStr(short aliasIndex, short nicknameIndex,
			   PStr nameStr);
PStr GetNicknameTrueNamePStr(short aliasIndex, short nicknameIndex,
			     PStr trueNameStr, Boolean okToMakeItUp);
PStr GetExpandedNicknamePStr(short aliasIndex, short nicknameIndex,
			     PStr expandedNicknameStr);
OSErr GetNicknameRecipientText(PETEHandle pte, short aliasIndex,
			       short nicknameIndex,
			       UHandle * recipientText);
Boolean NEFindNickname(Str255 nicknameSearchStr, Boolean caseSens,
		       Boolean diacSens, Boolean stickySens,
		       short *foundAliasIndex, short *foundNicknameIndex);
Boolean FindNextNicknamePrefix(Str255 prefixStr, short startAliasIndex,
			       short startNicknameIndex,
			       short *foundAliasIndex,
			       short *foundNicknameIndex,
			       Boolean allowExactMatch,
			       Boolean findInAllFiles);
Boolean FindNextNicknameTablePrefix(Str255 prefixStr,
				    NicknameTableHdl nicknameTable,
				    long startEntryIndex,
				    long *foundEntryIndex,
				    Boolean allowExactMatch);
Boolean NicknamePrefixOccursNTimes(Str255 prefixStr, long nTimes,
				   short aliasFileToScan);
long FindNicknameTableIndex(NicknameTableHdl nicknameTable,
			    short aliasIndex, short nicknameIndex);
long FindFirstNonHistoryNicknameTableIndex(NicknameTableHdl nicknameTable);
void GetNicknameTableEntry(NicknameTableHdl nicknameTable, long entryIndex,
			   short *aliasIndex, short *nicknameIndex);
OSErr NewNicknameTable(NicknameTableHdl * nicknameTable);
void DisposeNicknameTable(NicknameTableHdl nicknameTable);
OSErr CompactNicknameTable(NicknameTableHdl nicknameTable);
OSErr GrowNicknameTable(NicknameTableHdl nicknameTable, long entryCount);
OSErr AddNicknameTableEntry(NicknameTableHdl nicknameTable,
			    short aliasIndex, short nicknameIndex,
			    Str255 newEntryStr, Boolean sortedInsert);
OSErr BuildNicknameTable(Str255 prefixStr, short inAliasFile,
			 Boolean showExpansions, Boolean allowExactMatch,
			 NicknameTableHdl * builtNicknameTable);
OSErr BuildNicknameTableSubset(Str255 prefixStr,
			       NicknameTableHdl srcNicknameTable,
			       Boolean allowExactMatch,
			       NicknameTableHdl * builtNicknameTable);
void TruncateNicknamePopupItems(NicknameTableHdl nicknameTable,
				short fontNum, short fontSize,
				MyWindowPtr win);
void CalcNicknamePopupLoc(PETEHandle pte, Point * popupLoc, Rect * teflon);
Boolean DoNicknameStickyPopup(PETEHandle pte, Boolean showExpansions,
			      Str255 prefixStr, short *resultAliasIndex,
			      short *resultNicknameIndex);
OSErr ExpandAliases(BinAddrHandle * addresses, BinAddrHandle fromH,
		    short depth, Boolean wantComments);
OSErr ExpandAliasesLow(BinAddrHandle * addresses, BinAddrHandle fromH,
		       short depth, Boolean wantComments, PStr autoQual,
		       EAL_VARS_FORM);
long FindNextRecipientStart(Handle textHdl, long startOffset);
long FindRecipientStartInText(Handle textHdl, long startOffset,
			      Boolean skipLeadingSpaces);
long FindRecipientEndInText(Handle textHdl, long startOffset,
			    Boolean skipTrailingSpaces);
long FindNicknameStartInText(Handle textHdl, long startOffset,
			     Boolean recipientField);
OSErr GetNicknamePrefixFromField(PETEHandle pte, Str255 prefixStr,
				 Boolean ignoreSelection,
				 Boolean useNicknameTypeAhead);
OSErr FinishAliasUsingTypeAhead(PETEHandle pte, Boolean wantExpansion,
				Boolean allowPopup, Boolean insertComma);
void FinishAlias(PETEHandle pte, Boolean wantExpansion, Boolean allowPopup,
		 Boolean dontUndo);
long FindAPrefix(short which, PStr word, long startIndex);
void InsertAlias(PETEHandle pte, HSPtr hs, PStr nameStr,
		 Boolean wantExpansion, long pStart, Boolean dontUndo);
Boolean IsNickname(PStr name, short which);
short FindNickExpansionForLo(UPtr name, long hash, short *theWhich,
			     Boolean pluginNicks);
Handle FindNickExpansionFor(UPtr name, short *theWhich, short *theIndex);
void ResetNicknameTypeAhead(PETEHandle pte);
Boolean CurSelectionIsTypeAheadText(PETEHandle pte);
Boolean OKToInitiateTypeAhead(PETEHandle pte);
OSErr SetNicknameTypeAheadText(PETEHandle pte, Str255 prefixStr,
			       short aliasIndex, short nicknameIndex,
			       NicknameTableHdl tableHdl, long tableIndex);
Boolean NicknameTypeAheadKey(PETEHandle pte, EventRecord * event);
void NicknameTypeAheadKeyFollowup(PETEHandle pte);
Boolean NicknameTypeAheadValid(PETEHandle pte);
void NicknameTypeAheadIdle(PETEHandle pte);
void NicknameTypeAheadFinish(PETEHandle pte);
void ToggleNicknameTypeAhead(PETEHandle pte);
void ResetNicknameHiliting(PETEHandle pte);
OSErr SetNicknameHiliting(PETEHandle pte, long startOffset, long endOffset,
			  Boolean hilite);
OSErr NicknameHilitingUpdateRange(PETEHandle pte, long startOffset,
				  long endOffset);
OSErr NicknameHilitingUpdateField(PETEHandle pte);
OSErr NicknameHilitingUnHiliteField(PETEHandle pte);
void NicknameHilitingUpdateAllFields(void);
void NicknameHilitingUnHiliteAllFields(void);
void HilitingRangeCheckInsert(PETEHandle pte, long startOffset,
			      long insertSize);
void HilitingRangeCheckDelete(PETEHandle pte, long startOffset,
			      long endOffset);
void HilitingRangeCheckInclude(PETEHandle pte, long startOffset,
			       long endOffset);
void NicknameHilitingKey(PETEHandle pte, EventRecord * event);
void NicknameHilitingKeyFollowup(PETEHandle pte);
Boolean NicknameHilitingValid(PETEHandle pte);
void NicknameHilitingIdle(PETEHandle pte);
void InitNicknameHiliting(void);
Boolean NicknameWatcherKey(PETEHandle pte, EventRecord * event);
void NicknameWatcherKeyFollowup(PETEHandle pte);
void NicknameWatcherIdle(PETEHandle pte);
void NicknameWatcherFocusChange(PETEHandle pte);
void NicknameWatcherModifiedField(PETEHandle pte);
void NicknameWatcherMaybeModifiedField(PETEHandle pte);
Boolean NicknameCachingValid(PETEHandle pte);
void NicknameCachingScan(PETEHandle pte, Handle newAddresses);
UPtr FindPStr(PStr string, UPtr spot, Size len);
OSErr InitNicknameWatcher(void);
void SetDefaultNicknameWatcherPrefs(void);
void RefreshNicknameWatcherGlobals(Boolean refreshFields);
void InvalCachedNicknameData(void);
Boolean GetNickFieldRange(PETEHandle pte, long *start, long *end);
void SetNickScanning(PETEHandle pte, NickScanType nickScan,
		     short aliasIndex,
		     NickFieldCheckProcPtr nickFieldCheck,
		     NickFieldHiliteProcPtr nickFieldHilite,
		     NickFieldRangeProcPtr nickFieldRange);
void SetNickCacheAddresses(PETEHandle pte, Handle addresses);
OSErr GatherRecipientAddresses(MessHandle messH, Handle * dest,
			       Boolean wantComments);
Boolean CanWeExpand(PETEHandle pte, EventRecord * event);
void CacheRecentNickname(PStr nickname);
void TrimNicknameCache(NickStructHandle aliases, short spaceNeeded);
short FindOldestCacheMember(NickStructHandle aliases);
Boolean CompareRawToExpansion(StringHandle raw, Handle expansion);
Boolean AppearsInAliasFile(PStr address, PStr file);
Boolean HashAppearsInAliasFile(uLong addrHash, PStr file);
OSErr NickExpFindNickFromAddr(PStr addrStr, short *outWhich,
			      short *outIndex);
#endif
