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

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <StandardFile.h>
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

OSErr TruncAtMark(short refN);
short GetMyVR(UPtr name);
long GetMyDirID(short refNum);
short GetDirName(UPtr volName,short vRef, long dirId,UPtr name);
OSErr ParentSpec(FSSpecPtr child,FSSpecPtr parent);
UPtr GetMyVolName(short refNum,UPtr name);
int BlessedDirID(long *sysDirIDPtr);
short BlessedVRef(void);
OSErr IndexVRef(short index, short *vRef);
int MakeResFile(UPtr name,int vRef,long dirId,long creator,long type);
OSErr ExchangeAndDel(FSSpecPtr tmpSpec,FSSpecPtr spec);
short DirIterate(short vRef,long dirId,CInfoPBRec *hfi);
int CopyFBytes(short fromRefN,long fromOffset,long length,short toRefN,long toOffset);
void StdFileSpot(Point *where, short id);
short ARFHOpen(UPtr name,short vRefN,long dirId,short *refN,short perm);
int MyAllocate(short refN,long size);
short SFPutOpen(FSSpecPtr spec,long creator,long type,short *refN,ModalFilterYDUPP filter,DlgHookYDUPP hook,short id,FSSpecPtr defaultSpec,PStr windowTitle, PStr message);
Boolean IsText(FSSpecPtr spec);
short MyOpenResFile(short vRef,long dirId,UPtr name);
short SpinOnLo(volatile OSErr *rtnCodeAddr,long maxTicks,Boolean allowCancel,Boolean forever,Boolean remainCalm, Boolean allowMouseDown);
#define SpinOn(r,mt,ac,f) SpinOnLo(r,mt,ac,f,false,false)
#define FSZWrite(refN,count,buf) (((*count)>0) ? AWrite(refN,count,buf):0)
Boolean IsItAFolder(short vRef,long inDirId,UPtr name);
Boolean HFIIsFolder(CInfoPBRec *hfi);
Boolean HFIIsFolderOrAlias(CInfoPBRec *hfi);
void FolderSizeHi(short vRef,long dirID,uLong *cumSize);
void FolderSize(short vRef,long dirID,CInfoPBRec *hfi,uLong *cumSize);
#ifdef DEBUG
#define FSpDirCreate MyFSpDirCreate
#define DirCreate MyDirCreate
OSErr MyFSpDirCreate(FSSpecPtr spec, ScriptCode scriptTag, long *createdDirID);
OSErr MyDirCreate (short vRefNum, long parentDirID, PStr directoryName, long *createdDirID);
#endif
Boolean AliasFolderType(OSType type);
#define FSpIsItAFolder(spec) IsItAFolder((spec)->vRefNum,(spec)->parID,(spec)->name)
Boolean AFSpIsItAFolder(FSSpecPtr spec);
short FolderFileCount(FSSpecPtr spec);
short AFSHOpen(UPtr name,short vRefN,long dirId,short *refN,short perm);
short GetMyWD(short vRef,long dirID);
short HMove(short vRef,long fromDirId,UPtr fromName,long toDirId,UPtr toName);
short AHGetFileInfo(short vRef,long dirId,UPtr name,CInfoPBRec *hfi);
short AHSetFileInfo(short vRef,long dirId,UPtr name,CInfoPBRec *hfi);
#define AFSpGetHFileInfo(spec,hfi) AHGetFileInfo((spec)->vRefNum,(spec)->parID,(spec)->name,hfi)
#define AFSpSetHFileInfo(spec,hfi) AHSetFileInfo((spec)->vRefNum,(spec)->parID,(spec)->name,hfi)
OSErr FSpGetHFileInfo(FSSpecPtr spec,CInfoPBRec *hfi);
long FSpFileSize(FSSpecPtr spec);
long FSpDFSize(FSSpecPtr spec);
long FSpRFSize(FSSpecPtr spec);
OSErr SubFolderSpec(short nameId,FSSpecPtr spec);
OSErr StuffFolderSpec(FSSpecPtr spec);
OSErr FindSubFolderSpec(long domain,long folder,short subfolderID,Boolean create,FSSpecPtr subSpec);
OSErr SubFolderSpecOf(FSSpecPtr inSpec,short subfolderID,Boolean create,FSSpecPtr subSpec);
OSErr SubFolderSpecOfStr(FSSpecPtr inSpec,PStr subfolderName,Boolean create,FSSpecPtr subSpec);
uLong FSpModDate(FSSpecPtr spec);
short AHSetFileInfo(short vRef,long dirId,UPtr name,CInfoPBRec *hfi);
Boolean GetFolder(char *name,short *volume,long *folder,Boolean writeable,Boolean system,Boolean floppy,Boolean desktop);
#define FSpCopyRFork(t,f,p) CopyRFork(t->vRefNum,t->parID,t->name,f->vRefNum,f->parID,f->name,p)
#define FSpCopyDFork(t,f,p) CopyDFork(t->vRefNum,t->parID,t->name,f->vRefNum,f->parID,f->name,p)
#define FSpCopyFInfo(t,f) CopyFInfo(t->vRefNum,t->parID,t->name,f->vRefNum,f->parID,f->name)
short CopyFork(short vRef,long dirId,UPtr name,short fromVRef,
								long fromDirId,Uptr fromName,Boolean rFork,Boolean progress);
#define CopyRFork(v,d,n,fv,fd,fn,p) CopyFork(v,d,n,fv,fd,fn,True,p)
#define CopyDFork(v,d,n,fv,fd,fn,p) CopyFork(v,d,n,fv,fd,fn,False,p)
short CopyFInfo(short vRef,long dirId,UPtr name,short fromVRef,
								long fromDirId,Uptr fromName);
OSErr MyUpdateResFile(short resFile);
OSErr FSpDupFile(FSSpecPtr to,FSSpecPtr from,Boolean replace,Boolean progress);
OSErr FSpDupFolder(FSSpecPtr toSpec,FSSpecPtr fromSpec,Boolean replace,Boolean progress);
OSErr RemoveDir(FSSpecPtr spec);
OSErr MakeDarnSure(short refN);
OSErr FlushFile(short refN);
OSErr SimpleResolveAlias(AliasHandle alias,FSSpecPtr spec);
OSErr SimpleResolveAliasNoUI(AliasHandle alias,FSSpecPtr spec);
int UniqueSpec(FSSpecPtr spec,short max);
short SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(PStr name,PStr dfName,PStr dfQuoteExtensionUnquote,short max);
short NCWriteP(short refN,UPtr pString);
short AWriteP(short refN,UPtr pString);
OSErr TweakFileType(FSSpecPtr spec,OSType type,OSType creator);
OSErr HuntNewline(short refN,long aroundSpot,long *newline,Boolean *realNl);
OSErr Snarf(FSSpecPtr spec, Handle *hp, long limit);
OSErr SnarfRoman(FSSpecPtr spec, Handle *hp, long limit);
short MyResolveAlias(short *vRef,long *dirId,UPtr name,Boolean *wasAlias);
#define FSpMyResolve(s,wasAlias) MyResolveAlias(&(s)->vRefNum,&(s)->parID,(s)->name,wasAlias)
void PromptGetFile(FileFilterProcPtr filter,DlgHookYDProcPtr hook,long hookData,short numTypes,SFTypeList tl,StandardFileReply *reply,PStr prompt);
short FSWriteP(short refN,UPtr pString);
short GetFileByRef(short refN,FSSpecPtr specPtr);
OSErr AFSpSetMod(FSSpecPtr spec,uLong mod);
uLong AFSpGetMod(FSSpecPtr spec);
OSErr ChainDelete(FSSpecPtr spec);
long VolumeFree(short vRef);
Boolean SpecInSubfolderOf(FSSpecPtr att,FSSpecPtr folder);
short FSTabWrite(short refN,long *count,UPtr buf);
short ARead(short refN,long *count,UPtr buf);
short AWrite(short refN,long *count,UPtr buf);
short NCWrite(short refN,long *count,UPtr buf);
void SimpleMakeFSSpec(short vRef,long dirId,PStr name, FSSpecPtr spec);
short HGetCatInfo(short vRef,long inDirId,UPtr name,CInfoPBRec *hfi);
short HSetCatInfo(short vRef,long inDirId,UPtr name,CInfoPBRec *hfi);
short AFSpGetCatInfo(FSSpecPtr spec,FSSpecPtr newSpec,CInfoPBRec *hfi);
OSErr FSpKillRFork(FSSpecPtr spec);
OSErr FSpRFSane(FSSpecPtr spec,Boolean *sane);
OSErr TruncOpenFile(short refN, long spot);
OSErr EnsureNewline(short refN);
OSType FileTypeOf(FSSpecPtr spec);
OSType FileCreatorOf(FSSpecPtr spec);
OSErr FSpTrash(FSSpecPtr spec);
PStr Mac2OtherName(PStr mac,PStr other);
#define Other2MacName(x,y)	SanitizeFN(x,y,MAC_FN_BAD,MAC_FN_REP,true)
PStr SanitizeFN(PStr shortName,PStr name,short badCharId,short repCharId,Boolean kill8);
OSErr AFSpOpenDF (FSSpecPtr spec,FSSpecPtr newSpec, SignedByte permission, short *refNum);
OSErr AFSpOpenRF (FSSpecPtr spec,FSSpecPtr newSpec, SignedByte permission, short *refNum); 
OSErr AFSpDelete (FSSpecPtr spec,FSSpecPtr newSpec); 
OSErr AFSpGetFInfo (FSSpecPtr spec,FSSpecPtr newSpec, FInfo *fndrInfo);
OSErr AFSpSetFInfo (FSSpecPtr spec,FSSpecPtr newSpec, FInfo *fndrInfo);
OSErr AFSpSetFLock (FSSpecPtr spec,FSSpecPtr newSpec);
OSErr AFSpRstFLock (FSSpecPtr spec,FSSpecPtr newSpec);
OSErr FSpSetFXInfo(FSSpecPtr spec,FXInfo *fxInfo);
Boolean IsAlias(FSSpecPtr spec,FSSpecPtr newSpec);
Boolean IsAliasNoMount(FSSpecPtr spec,FSSpecPtr newSpec);
OSErr ResolveAliasOrElse(FSSpecPtr spec,FSSpecPtr newSpec,Boolean *wasIt);
OSErr FSMakeFID(FSSpecPtr spec,long *fid);
OSErr FSResolveFID(short vRef,long fid,FSSpecPtr spec);
OSErr DTRef(short vRef, short *dtRef);
OSErr DTGetAppl(short vRef,short dtRef,OSType creator,FSSpecPtr appSpec);
short DTFindAppl(OSType creator);
OSErr DTSetComment(FSSpecPtr spec,PStr comment);
OSErr MorphDesktop(short vRef,FSSpecPtr where);
OSErr Blat(FSSpecPtr spec,Handle text,Boolean append);
OSErr BlatPtr(FSSpecPtr spec,Ptr text, long size,Boolean append);
#define SameVRef(vr1,vr2) (vr1==vr2)
Boolean SameSpec(FSSpecPtr sp1,FSSpecPtr sp2);
short RealVRef(short wdRef);
long SpecDirId(FSSpecPtr spec);
OSErr CanWrite(FSSpecPtr spec, Boolean *can);
OSErr MakeAFinderAlias(FSSpecPtr originalSpec,FSSpecPtr aliasSpec);
OSErr SpecMove(FSSpecPtr moveMe,FSSpecPtr moveTo);
OSErr SpecMoveAndRename(FSSpecPtr moveMe,FSSpecPtr moveTo);
OSErr GetTrashSpec(short vRef,FSSpecPtr spec);
OSErr ResolveAliasNoMount(FSSpecPtr alias,FSSpecPtr orig,Boolean *wasAlias);
OSErr WipeSpec(FSSpecPtr spec);
OSErr WipeDiskArea(short refN,long offset, long len);
OSErr NewTempExtSpec(short vRef,PStr name,short extId,FSSpecPtr spec);
OSErr ExchangeFiles(FSSpecPtr tmpSpec,FSSpecPtr spec);
OSErr FSpTouch(FSSpecPtr spec);
OSErr ExtractCreatorFromBndl(FSSpecPtr spec,OSType *creator);
OSErr CreatorToName(OSType creator,PStr appName);
OSErr NewTempSpec(short vRef,long dirId,PStr name,FSSpecPtr spec);
OSErr FSpExists(FSSpecPtr spec);
OSErr AddUniqueExt(FSSpecPtr spec,short extId);
OSErr VolumeMargin(short vRef,long spaceNeeded);
Boolean DiskSpunUp(void);
short SFPutNew(FSSpecPtr spec);
OSErr FindTemporaryFolder(short vRef,long dirId,long *tempDirId,short *tempVRef);
Boolean IsPDFFile(FSSpecPtr spec,OSType fileType);
#ifdef DEBUG
OSErr MyFSClose(short refN); 
#else
#define MyFSClose FSClose
#endif

#ifdef DEBUG
void MyCloseResFile(short refN);
#define CloseResFile MyCloseResFile
#endif

#ifdef DEBUG
#define FSpDelete MyFSpDelete
OSErr MyFSpDelete(FSSpecPtr);
#endif

#define kStuffFolderBit	0x1
OSErr FindMyFile(FSSpecPtr spec,long whereToLook,short fileName);

void MakeUniqueUntitledSpec (short vRefNum, long dirID, short strResID, FSSpec *spec);
OSErr MisplaceItem (FSSpec *spec);
OSErr FSpGetLongName ( FSSpec *spec, TextEncoding destEncoding, Str255 longName );
OSErr FSpGetLongNameUnicode ( FSSpec *spec, HFSUniStr255 *longName );

OSErr FSpSetLongName ( FSSpec *spec, TextEncoding destEncoding, ConstStr255Param longName, FSSpec *newName );
OSErr FSpSetLongNameUnicode ( FSSpec *spec, ConstHFSUniStr255Param longName, FSSpec *newName );

OSErr MakeUniqueLongFileName ( short vRefNum, long dirID, StringPtr name, TextEncoding srcEncoding, short maxLen );

short FSpOpenResFilePersistent(FSSpecPtr spec,short mode);
#define FSpOpenResFile FSpOpenResFilePersistent
OSErr FSpOpenDFPersistent(FSSpecPtr spec,short mode,short *refN);
#define FSpOpenDF FSpOpenDFPersistent
OSErr FSpOpenRFPersistent(FSSpecPtr spec,short mode,short *refN);
#define FSpOpenRF FSpOpenRFPersistent

Boolean FSpIsLocked(FSSpecPtr spec);
Boolean SpecEndsWithExtensionR(FSSpecPtr spec,short resID);
#endif
