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

#include "nickae.h"
/* Copyright (c) 1995 by QUALCOMM Incorporated */
#define FILE_NUM 78

#define This	(*Aliases)[which]

OSErr NickObjFromNum(AEDescPtr nickFileDescP,short mNum,AEDescPtr nickDescP);
OSErr NickFileObjFromNum(AEDescPtr dummyDescP,short nNum,AEDescPtr nickFileDescP);

typedef struct
{
	PropToken property;
	short	file;
} NickFileSpecifier;

typedef struct
{
	PropToken property;
	NickToken nick;
} NickSpecifier;

#pragma segment NickAE

Boolean AENickFileExists(AEDescPtr token)
{
	short	which;

	AEGetDescData(token,&which,sizeof(which));
	if (which >=0 && which < NAliases)
		return (true);
	else
		return(False);
}
Boolean AENicknameExists(AEDescPtr token)
{
	NickToken nickToken;

	AEGetDescData(token,&nickToken,sizeof(nickToken));
	if (nickToken.nickIndex >=0 && nickToken.nickFileNumber >=0)
		return (true);

	return(False);
}
Boolean AENickFieldExists(AEDescPtr token)
{
	FieldToken ft;

	AEGetDescData(token,&ft,sizeof(ft));
	return (NickUserFieldExists(ft.name));
}
OSErr GetNickFileProperty(AEDescPtr token,AppleEvent *reply, uLong refCon)
{
	NickFileSpecifier	whichNickFile;
	short which;
	FSSpec spec;
	short err = errAENoSuchObject;

	AEGetDescData(token,&whichNickFile,sizeof(whichNickFile));
	which = whichNickFile.file;
	spec = This.spec;
	switch (whichNickFile.property.propertyId)
	{
		case pEuFSS:
			err = MyAEPutAlias(reply,keyAEResult,&spec);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}

	return err;
}
OSErr GetNickProperty(AEDescPtr token,AppleEvent *reply, uLong refCon)
{
#pragma unused(refCon)
	AttributeValueHandle	avPairs;
	AttributeValuePtr			avPairPtr;
	short									count,i;
	Str255								tag;
	NickSpecifier	whichNick;
	OSErr err= noErr;
	short	index;
	short	which;
	Str63	name;
	Handle tempHandle = nil,temp2Handle = nil,theNames = nil;
	
	AEGetDescData(token,&whichNick,sizeof(whichNick));
	index = whichNick.nick.nickIndex;
	which = whichNick.nick.nickFileNumber;
	if (index >=0 && which >=0)
	{
		switch (whichNick.property.propertyId)
		{
			case pEuNickNick:
//				PCopy(name,*((*((*Aliases)[which].theData))[index].theName));
				GetNicknameNamePStr(which,index,name);
				err = AEPutPStr(reply,keyAEResult,name);
				break;
			case pEuNickAddrOld:
			case pEuNickAddrNew:
				tempHandle = GetNicknameData(which,index,true,true);
				if (tempHandle)
					{
						MyHandToHand(&tempHandle);
						err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(tempHandle),GetHandleSize_(tempHandle));
						UL(tempHandle);
						ZapHandle(tempHandle);
					}
				else				
					err = AEPutPStr(reply,keyAEResult,"\p");
				break;
			case pEuNickNotes:
				tempHandle = GetNicknameData(which,index,false,true);
				LDRef(tempHandle);
				if (tempHandle != nil)
					if (avPairs = ParseAllAttributeValuePairs (tempHandle, &temp2Handle, avAllButHiddenPairs, avNoPairs)) {
						avPairPtr = LDRef (avPairs);
						count = GetHandleSize (avPairs) / sizeof (AttributeValueRec);
						for (i = 0; i < count; ++i, ++avPairPtr) {
							MakePStr (name, *tempHandle + avPairPtr->attributeOffset, avPairPtr->attributeLength);
							if (StringSame (name, GetRString (tag, ABReservedTagsStrn + abTagNote))) {
								PtrPlusHand (*tempHandle + avPairPtr->valueOffset, temp2Handle, avPairPtr->valueLength);
								break;
							}
						}
					}
				UL(tempHandle);
				if (temp2Handle)
					err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(temp2Handle),GetHandleSize_(temp2Handle));
				else
					err = AEPutPStr(reply,keyAEResult,"\p");
				UL(temp2Handle);
				ZapHandle(temp2Handle);
				break;
			case pEuNickRawNotes:
				tempHandle = GetNicknameData(which,index,false,true);
				if (tempHandle)
					{
						MyHandToHand(&tempHandle);
						err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(tempHandle),GetHandleSize_(tempHandle));
						UL(tempHandle);
						ZapHandle(tempHandle);
					}
				else				
					err = AEPutPStr(reply,keyAEResult,"\p");
				break;
			case cEuNickFile:
				err = AEPutPStr(reply,keyAEResult,This.spec.name);
				break;
			case pEuNickExpansion:
//				PCopy(name,*((*((*Aliases)[which].theData))[index].theName));
				GetNicknameNamePStr(which,index,name);
				theNames = NuHandle(0);
				if (!theNames)
					break;
				if (err=PtrPlusHand(name,theNames,*name+2)) break;
				err = PtrPlusHand("",theNames,1); // final delimiter
				if (!err)
						err = ExpandAliases(&tempHandle,theNames,0,true);
				if (!err && tempHandle) CommaList(tempHandle);
				err = AEPutParamPtr(reply,keyAEResult,typeChar,LDRef(tempHandle),GetHandleSize_(tempHandle));
				UL(tempHandle);
				ZapHandle(tempHandle);
				ZapHandle(theNames);
				break;
			case pEuIsRecip:
				err = AEPutBool(reply,keyAEResult,IsNicknameOnRecipList(which,index));
				break;
			default:
				err=errAEEventNotHandled;

		}
	}
	else
		err=errAEEventNotHandled;



	return(err);
}
OSErr SetNickProperty(AEDescPtr token,AEDescPtr data)
{
	NickSpecifier	whichNick;
	OSErr err= noErr;
	short	index;
	short	which;
	Str63	name,oldName;
	Handle tempHandle = nil,temp2Handle = nil,theNames = nil,dataHandle = nil;
	AEDesc	obj,textAD;
	AEDesc *useMe;
	Boolean	isRecip;
	Handle	hData;

	AEGetDescData(token,&whichNick,sizeof(whichNick));
	index = whichNick.nick.nickIndex;
	which = whichNick.nick.nickFileNumber;

	NullADList(&textAD,&obj,nil);

	if (index >=0 && which >=0)
	{
//		PCopy(oldName,*((*((*Aliases)[which].theData))[index].theName));
		GetNicknameNamePStr(which,index,oldName);
		switch (whichNick.property.propertyId)
		{
			case pEuNickNick:
				GetAEPStr(name,data);
				ChangeNameOfNick(which,oldName,name);
				break;
			case pEuIsRecip:
				isRecip = GetAEBool(data);
				if (!IsNicknameOnRecipList(which,index) && isRecip)
						AddStringAsTo(oldName);
				else if (!isRecip && IsNicknameOnRecipList(which,index))
					RemoveFromTo(oldName);
				break;	
			case pEuNickAddrNew:
			case pEuNickAddrOld:
				if (data->descriptorType == typeChar)
					useMe = data;
				else if (err=AECoerceDesc(data,typeChar,&textAD)) return(err);
				else useMe = &textAD;
				AEGetDescDataHandle(useMe,&hData);
				ReplaceNicknameAddresses(which,oldName,hData);
				AEDisposeDescDataHandle(useMe);
				break;
			case pEuNickNotes:
				AESaveCurrentAlias(which,index);
				if (!(dataHandle = GetNicknameData(which,index,false,true)))
					if (!(dataHandle = NuHandle(0)))
						return(MemError());
				if (data->descriptorType == typeChar)
					useMe = data;
				else if (err=AECoerceDesc(data,typeChar,&textAD)) return(err);
				else useMe = &textAD;
				AEGetDescDataHandle(useMe,&hData);
				HandPlusHand(hData,dataHandle);
				AEDisposeDescDataHandle(hData);
				ReplaceNicknameNotes(which,oldName,dataHandle);
				ZapHandle(dataHandle);
				break;
			default:
				err = errAEEventNotHandled;
		}
	}
	ForceSelectedAliasUpdate(which,index,whichNick.property.propertyId!=pEuIsRecip);
	DisposeADList(&textAD,&obj,nil);
	return(err);
}
OSErr GetNickField(AEDescPtr token,AppleEvent *reply, uLong refCon)
{
	FieldToken ft;
	NickToken	theNickToken;
	Str63	sViewData;
	Boolean	nickNameEmpty;
	OSErr	err = errAEEventNotHandled;
	
	AEGetDescData(token,&ft,sizeof(ft));
	theNickToken = ft.nickT;
	NickGetDataFromField(ft.name,sViewData,theNickToken.nickFileNumber,theNickToken.nickIndex,false,true,&nickNameEmpty);
	if (!nickNameEmpty && *sViewData)
	{
		err = AEPutParamPtr(reply,keyAEResult,typeChar,sViewData + 1,*sViewData);
	}
	else
		err = AEPutPStr(reply,keyAEResult,"\p");


	return(err);
}
OSErr SetNickField(AEDescPtr token,AEDescPtr data)
{
	FieldToken ft;
	NickToken	theNickToken;
	short which;
	short index;
	long	offset;
	Handle	tempHandle,dataHandle;
	Str63	field,name;
	char	endTag = '>',beginTag = '<',colonTag = ':';
	OSErr	err = noErr;
	AEDesc	textAD;
	AEDesc *useMe;
	Handle	hData;
	
	AEGetDescData(token,&ft,sizeof(ft));
	theNickToken = ft.nickT;
	which = theNickToken.nickFileNumber;
	index = theNickToken.nickIndex;

	NullADList(&textAD,nil);

	AESaveCurrentAlias(which,index);
	PCopy(field,ft.name);
	MyLowercaseText(field+1,*field);
	
//	PCopy(name,*((*((*Aliases)[which].theData))[index].theName));
	GetNicknameNamePStr(which,index,name);
	dataHandle = NuHandle(0);
	if (!dataHandle) return(MemError());

	if (tempHandle = GetNicknameData(which,index,false,true))
	{
		offset = SearchPtrHandle(field + 1,*field,tempHandle,0,false,false,nil);
		if (offset >= 0)
		{
			PtrPlusHand(LDRef(tempHandle),dataHandle,offset - 1);
			offset = SearchPtrHandle(&endTag,1,tempHandle,offset,false,false,nil);
			if (offset >= 0)
				PtrPlusHand(LDRef(tempHandle) + offset + 1,dataHandle,GetHandleSize_(tempHandle) - offset - 1);
			else
				err = errAEEventNotHandled;
			UL(tempHandle);
		}
		else
			HandPlusHand(tempHandle,dataHandle);
	}
		
	PtrPlusHand(&beginTag,dataHandle,1);
	PtrPlusHand(field + 1,dataHandle,*field);
	PtrPlusHand(&colonTag,dataHandle,1);
	if (data->descriptorType == typeChar)
		useMe = data;
	else if (err=AECoerceDesc(data,typeChar,&textAD)) return(err);
	else useMe = &textAD;
	AEGetDescDataHandle(useMe,&hData);
	HandPlusHand(hData,dataHandle);
	AEDisposeDescDataHandle(hData);
	PtrPlusHand(&endTag,dataHandle,1);
	ReplaceNicknameNotes(which,name,dataHandle);
	
	
	ForceSelectedAliasUpdate(which,index,true);
	DisposeADList(&textAD,nil);
	return(err);
}
OSErr AECreateNick(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	NickFileToken theFileToken;
	short which, index = -1;
	Str63	name;
	AEDesc inToken,nickDesc,dataDesc,textAD;
	AEDescPtr useMe;
	OSErr	err = noErr;
	long	theHandleSize,theDataSize;

	NullADList(&dataDesc,&textAD,&nickDesc,&inToken,nil);

	#ifdef ALIAS_UNTITLED_NICK
	GetRString(name,ALIAS_UNTITLED_NICK);
	#else
	PCopy(name,"\puntitled");
	#endif
	
	if (!(err = AEResolve(inContainer,kAEIDoMinimum,&inToken)))
	{
		AEGetDescData(&inToken,&theFileToken,sizeof(theFileToken));
		which = theFileToken.nickFileIndex;
		if (inToken.descriptorType!=cEuNickFile)
			err = errAEEventNotHandled;
		if (!err && !(err=AEGetParamDesc(event,keyAEData,typeWildCard,&dataDesc)))
		if (!(err = GotAERequired(event)))
		{
			if (dataDesc.descriptorType == typeChar)
				useMe = &dataDesc;
			else
			{
				err=AECoerceDesc(&dataDesc,typeChar,&textAD);
				useMe = &textAD;
			}
			if (!err) // We have a file name.
			{
				theHandleSize = AEGetDescDataSize(useMe);
				theDataSize = theHandleSize < sizeof(name) - 1 ? theHandleSize:sizeof(name) -1;
				AEGetDescData(useMe,name + 1,theDataSize);
				name[0] = theDataSize;
			}
		}

		if (!err)
			index = NewNickLow(nil,nil,which,name,false,nrDifferent,true);
		if (index < 0)
			err = errAEEventNotHandled;
		if (!err && !(err = NickObjFromNum(inContainer,index - 1,&nickDesc)))
		{
			err = AEPutParamDesc(reply,keyAEResult,&nickDesc);
			ABTickleHardEnoughToMakeYouPuke();
		}
	}
	DisposeADList(&dataDesc,&textAD,&nickDesc,&inToken,nil);
	return(err);
}
OSErr AECreateNickFile(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	OSErr err = errAEEventNotHandled;
	AEDesc dataDesc,textAD,nickFileDesc;
	AEDescPtr useMe;
	Str63	fileName;
	long	theHandleSize,theDataSize;
	short	which = -1;

	NullADList(&nickFileDesc,&dataDesc,&textAD,nil);
	if (!(err=AEGetParamDesc(event,keyAEData,typeWildCard,&dataDesc)))
	if (!(err = GotAERequired(event)))
	{
		if (dataDesc.descriptorType == typeChar)
			useMe = &dataDesc;
		else
		{
			err=AECoerceDesc(&dataDesc,typeChar,&textAD);
			useMe = &textAD;
		}
		if (!err) // We have a file name.
		{
			theHandleSize = AEGetDescDataSize(useMe);
			theDataSize = theHandleSize < sizeof(fileName) - 1 ? theHandleSize:sizeof(fileName) -1;
			AEGetDescData(useMe,fileName + 1,theDataSize);
			fileName[0] = theDataSize;
			which = NickMakeNewFile(fileName);
			err = noErr;
		}
	}
	if (!err && !(err = NickFileObjFromNum(inContainer,which,&nickFileDesc)))
	{
		err = AEPutParamDesc(reply,keyAEResult,&nickFileDesc);
	}
	
	DisposeADList(&nickFileDesc,&dataDesc,&textAD,nil);
	return(err);
}
OSErr AEDeleteNickname(NickTPtr token)
{
	short	which,index;
	which = token->nickFileNumber;
	index = token->nickIndex;
	
	if (!AERemoveNick(index,which))
		return(errAEEventNotHandled);
	
	return(noErr);
}
OSErr AEDeleteNickFile(NickFileTPtr token)
{
	short	which,index;
	which = token->nickFileIndex;
	index = -1;

	if (!AERemoveNick(index,which))
		return(errAEEventNotHandled);
	
	return(noErr);
}
OSErr CountNickFileElements(DescType theClass,AEDescPtr inContainer,long *howMany)
{
	NickFileToken theFileToken;
	short which;
	short	i,totalNicks,realCount;
	
	AEGetDescData(inContainer,&theFileToken,sizeof(theFileToken));
	which = theFileToken.nickFileIndex;
	totalNicks = NNicknames;
	if (which >= 0 && which < NAliases)
	{
	realCount = 0;
		if ((*Aliases)[which].theData)
			for (i=0;i<totalNicks;i++)
				{
					if (!(*((*Aliases)[which].theData))[i].deleted)
						realCount++;
				}
		*howMany = realCount;
	}
	ABTickleHardEnoughToMakeYouPuke();	
	return(noErr);
}
OSErr CountNickFiles(DescType theClass,AEDescPtr inContainer,long *howMany)
{
	*howMany = NAliases;

	return(noErr);
}

/**********************************************************************
 * ONLY object accessor functions belong in the ObjSupport segment
 **********************************************************************/
#pragma segment ObjSupport
pascal OSErr FindNickFile(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{

#pragma unused(desiredClass,refCon)
	NickFileToken token;
	short err=noErr;
	long index = -1;
	Str255 name;
	short	which,numOfAliases;
	
	RegenerateAllAliases(false);
	numOfAliases = NAliases;
	
	token.nickFileIndex = -1;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(name,keyData);
			for (which=0;which<numOfAliases;which++)
				{
					if (StringSame(This.spec.name,name))
						{
							token.nickFileIndex = which;
							break;
						}
				}
				if (token.nickFileIndex < 0)
					err = errAENoSuchObject;
				break;
		case formAbsolutePosition:
			index = GetAELong(keyData);
			if (index<0) index += numOfAliases;	// negative counts back from end?
			if (index < numOfAliases && index >= 0)
				token.nickFileIndex = index;
			else
				err = errAENoSuchObject;
			break;
		default:
			err = errAENoSuchObject;
			break;
	}


	if (!err) err = AECreateDesc(cEuNickFile,&token,sizeof(token),theToken);
	return(err);

}
pascal OSErr FindNickname(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,refCon)
	NickToken token;
	NickFileToken theFileToken;
	short err=errAENoSuchObject,which;
	long index = -1;
	Str255 name;

	RegenerateAllAliases(false);

	token.nickIndex = -1;
	token.nickFileNumber = -1;
	
	if (containerClass==typeNull)	
	{
			GetAEPStr(name,keyData);
			for (which=0;which<NAliases;which++)
				{
					index = NickMatchFound(((*Aliases)[which].theData),NickHash(name),name,which);
					if (index >= 0)
						{
							token.nickIndex = index;
							token.nickFileNumber = which;
							err = noErr;
							break;
						}
				}
	}
	else
		switch (keyForm)
		{
			case formName:
				GetAEPStr(name,keyData);
				AEGetDescData(containerToken,&theFileToken,sizeof(theFileToken));
				which = theFileToken.nickFileIndex;
				index = NickMatchFound(((*Aliases)[which].theData),NickHash(name),name,which);
				if (index >= 0 && which >= 0)
				{
					token.nickIndex = index;
					token.nickFileNumber = which;
					err = noErr;
				}
				break;
			case formAbsolutePosition:
				index = GetAELong(keyData);
				AEGetDescData(containerToken,&theFileToken,sizeof(theFileToken));
				which = theFileToken.nickFileIndex;
				if (index >=0 && which >=0 && index < NNicknames)
					{
						token.nickIndex = index;
						token.nickFileNumber = which;
						err = noErr;
					}
				break;
			default:
				err = errAENoSuchObject;
				break;
		}


	if (!err) err = AECreateDesc(cEuNickname,&token,sizeof(token),theToken);
	return(err);
}
pascal OSErr FindNickField(DescType desiredClass, AEDescPtr containerToken,
												DescType containerClass, DescType keyForm, AEDescPtr keyData, AEDescPtr theToken,
												long refCon)
{
#pragma unused(desiredClass,containerClass,refCon)
	FieldToken token;
	short err;
	
	switch (keyForm)
	{
		case formName:
			GetAEPStr(token.name,keyData);
			AEGetDescData(containerToken,&token.nickT,sizeof(token.nickT));
			token.isNick = true;
			err = noErr;	/* cheating; we'll report errors if the field is used and not present */
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	if (!err) err = AECreateDesc(cEuField,&token,sizeof(token),theToken);
	return(err);
}

/************************************************************************
 * NickObjFromNum - specify a nickname object
 ************************************************************************/
OSErr NickObjFromNum(AEDescPtr nickFileDescP,short nNum,AEDescPtr nickDescP)
{
	OSErr err;
	AEDesc nNumDesc;
	long longNum = nNum+1;
	
	if (!(err = AECreateDesc(typeLongInteger,&longNum,sizeof(longNum),&nNumDesc)))
	{
		err = CreateObjSpecifier(cEuNickname,nickFileDescP,formAbsolutePosition,&nNumDesc,False,nickDescP);
		AEDisposeDesc(&nNumDesc);
	}
	return(err);
}

/************************************************************************
 * NickFileObjFromNum - specify a nickname file object
 ************************************************************************/
OSErr NickFileObjFromNum(AEDescPtr dummyDescP,short nNum,AEDescPtr nickFileDescP)
{
	OSErr err;
	AEDesc nNumDesc;
	long longNum = nNum;
	
	if (!(err = AECreateDesc(typeLongInteger,&longNum,sizeof(longNum),&nNumDesc)))
	{
		err = CreateObjSpecifier(cEuNickFile,dummyDescP,formAbsolutePosition,&nNumDesc,False,nickFileDescP);
		AEDisposeDesc(&nNumDesc);
	}
	return(err);
}