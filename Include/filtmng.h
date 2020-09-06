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

#ifndef FILTMNG_H
#define FILTMNG_H

#include <appleevent.h>

#ifdef TWO

OSErr RegenerateFilters(void);
OSErr ReadFilters(Handle hFilters,short vRef,long dirID,StringPtr name);
void FiltersDecRef(void);
Boolean FilterExists(DescType form, long selector);
OSErr CountFilters(long *howMany);
TokenTaker GetFilterProperty, GetTermProperty;
OSErr AECreateFilter(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply);
OSErr SetFilterProperty(AEDescPtr token, AEDescPtr data);
OSErr SetTermProperty(AEDescPtr token, AEDescPtr data);
OSErr AEDeleteFilter(FilterTokenPtr fp);
void FRInit(FRPtr fr);
void DisposeFAction(FActionHandle fa);
long FilterNewId(void);
OSErr SaveFiltersLo(FSSpecPtr toSpec,Boolean all,Boolean clean);
#define SaveFilters()	SaveFiltersLo(nil,true,true)
#define ZapFAction(f)	while(f){DisposeFAction(f);f=nil;}
void ZapFilters(void);
void ZapActions(FActionHandle fa);
OSErr FWriteKey(short refN,FilterKeywordEnum flk,PStr value);
OSErr FWriteBool(short refN,FilterKeywordEnum flk,Boolean value);
OSErr FWriteEnum(short refN,FilterKeywordEnum flk,short e);
void StudyFilter(FRPtr fr);
OSErr TellFiltMBRename(FSSpecPtr spec,FSSpecPtr newSpec,Boolean folder,Boolean will,Boolean dontWarn);
void DisposeFilters(Handle *hFilters);
void GeneratePluginFilters(void);
OSErr AppendFilter(FRPtr fr,Handle hFilters);
typedef struct
{
	FSSpec oldSpec;
	FSSpec newSpec;
} MBRenamePB, *MBRenamePBPtr, **MBRenamePBH;
typedef enum
{
	fRenameAsk,
	fRenameChange,
	fRenameIgnore
}	FilterRenameEnum;
#endif
#endif
