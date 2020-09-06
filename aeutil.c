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

#define FILE_NUM 65
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#include "aeutil.h"
#pragma segment MAEUtil
pascal OSErr MyAEIdle(EventRecord *event,long *sleep, RgnHandle *rgn);

/**********************************************************************
 * SimpleAESend - send an AE, without all the crap
 *  psn - the psn to send to
 *  eClass - the event class
 *  eId - the event id
 *  mode - the send mode
 *  varargs parameters come next
 *    keyword - keyword.  special are typeNull, keyEuIgnoreDesc, keyEuDesc
 *		for Desc types, next argument is pointer to descriptor
 *    for non-Desc types, next three pointers are type, data, and size
 *			size<0 means argument is real, not pointer
 *		if the keyword is typeNull or keyEuIgnoreDesc, arguments are consumed but not
 *      actually added to the event
 *  nil ends the parameters
 *  then we begin with attributes, with the same scheme
 **********************************************************************/
OSErr SimpleAESend(ProcessSerialNumber *psn,DescType eClass, DescType eId, AppleEvent *reply,AESendMode mode,...)
{
	AppleEvent ae;
	AEDesc d;
	OSErr err;
	DescType type, key;
	long size;
	UPtr arg;
	UPtr aVal;
	Boolean attribs = False;
	va_list args;
	va_start(args,mode);

	NullADList(&ae,&d,reply,nil);
	
	if (!(err=AECreateDesc(typeProcessSerialNumber,psn,sizeof(*psn),&d)))
	if (!(err=AECreateAppleEvent(eClass,eId,&d,kAutoGenerateReturnID,kAnyTransactionID,&ae)))
	{
		for (key = va_arg(args,DescType);!err && (key||!attribs);key = va_arg(args,DescType))
		{
			if (!key) attribs = True;
			else
			{
				type = va_arg(args,DescType);
				arg = va_arg(args,void *);
				if (type==keyEuIgnoreDesc || type==keyEuDesc)
				{
					if (type!=keyEuIgnoreDesc)
						if (arg)
							err = attribs?AEPutAttributeDesc(&ae,key,(void*)arg):AEPutParamDesc(&ae,key,(void*)arg);
						else err = paramErr;
				}
				else
				{
					size = va_arg(args,long);
					if (size<0)
					{
						size = -size;
						aVal = arg;
						arg = (void*)&aVal;
					}
					if (type!=typeNull)
						err = attribs?AEPutAttributePtr(&ae,key,type,arg,size):AEPutParamPtr(&ae,key,type,arg,size);
				}
			}
		}
	}
	
	if (!err)
		err = MyAESend(&ae,reply,mode,kAENormalPriority,GetRLong(AE_TIMEOUT_TICKS));
	
	if (err<0 && err!=-1711) WarnUser(AE_TROUBLE,err);

	va_end(args);
	DisposeADList(&ae,&d,nil);
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
Boolean HaveScriptableFinder(void)
{
	long response;
	
	if (Gestalt(gestaltFinderAttr,&response)) return(False);
	if (response & (1L<<gestaltOSLCompliantFinder)) return(True);
	return(False);
}

/**********************************************************************
 * WhackFinder - whack the finder.  Why I have to do this, I don't know
 **********************************************************************/
OSErr WhackFinder(FSSpecPtr spec)
{
	OSErr err=noErr;
	ProcessSerialNumber psn;
	FSSpec mySpec = *spec;
	AliasHandle folderAlias=nil;
	
	if (HaveScriptableFinder() && !(err=FindPSNByCreator(&psn,'MACS')))
	{
#ifdef NEVER
		FInfo info;
		if (!FSpGetFInfo(spec,&info))
		{
			info.fdFlags &= ~fInited;
			FSpSetFInfo(spec,&info);
		}
#endif
		*mySpec.name = 0;
		err = FSpTouch(&mySpec);
		if (!err && !(err = NewAlias(nil,&mySpec,&folderAlias)))
		{
			err = SimpleAESend(&psn,'fndr','fupd',nil,kAENoReply,
				keyDirectObject,typeAlias,LDRef(folderAlias),GetHandleSize_(folderAlias),nil,nil);
			ZapHandle(folderAlias);
		}
	}
	ASSERT(!err);
	return(err);
}

/**********************************************************************
 * SimpleAEList - build a list, without all the crap
 *  list - the list
 *  varargs parameters come next
 *		type comes first
 *		for Desc types, next argument is pointer to descriptor
 *    for non-Desc types, next two longwords are data, and size
 *		if the type is typeNull or keyEuIgnoreDesc, arguments are consumed but not
 *      actually added to the event
 *  nil ends the list
 **********************************************************************/
AEDescList *SimpleAEList(AEDescList *list,...)
{
	OSErr err;
	DescType type;
	long size;
	UPtr arg;
	Boolean attribs = False;
	va_list args;
	va_start(args,list);

	NullADList(list,nil);
	
	if (!(err=AECreateList(nil,0,False,list)))
	{
		for (type = va_arg(args,DescType);!err && type;type = va_arg(args,DescType))
		{
			arg = va_arg(args,void *);
			if (type==keyEuIgnoreDesc || type==keyEuDesc)
			{
				if (type!=keyEuIgnoreDesc)
					if (arg)
						err = AEPutDesc(list,0,(void*)arg);
					else err = paramErr;
			}
			else
			{
				size = va_arg(args,long);
				if (type!=typeNull)
					err = AEPutPtr(list,0,type,arg,size);
			}
		}
	}
	
	if (err) WarnUser(AE_TROUBLE,err);

	va_end(args);
	return(err?nil:list);
}

/**********************************************************************
 * MyAESend - send an event with protection
 **********************************************************************/
OSErr MyAESend (const AppleEvent *theAppleEvent, AppleEvent *reply,
	 AESendMode sendMode, AESendPriority sendPriority,
	 long timeOutInTicks)
{
	OSErr err;
	DECLARE_UPP(MyAEIdle,AEIdle);
	
	INIT_UPP(MyAEIdle,AEIdle);
	MightSwitch();
	err = AESend(theAppleEvent, reply, sendMode, sendPriority, timeOutInTicks, MyAEIdleUPP, nil);
	AfterSwitch();
	return(err);
}

/**********************************************************************
 * MyAEIdle - handle idle for an apple event
 **********************************************************************/
pascal OSErr MyAEIdle(EventRecord *event,long *sleep, RgnHandle *rgn)
{
	if (NEED_YIELD)
	{
		CyclePendulum();
		if (MiniEvents()) return(True);
	}
	return(False);
}

/************************************************************************
 * AEPutLong - put a long to a descriptor list
 ************************************************************************/
OSErr AEPutLong(AppleEvent *event,DescType key,long theLong)
{
	return(AEPutParamPtr(event,key,typeLongInteger,&theLong,sizeof(long)));
}

/************************************************************************
 * AEPutPStr - put a Pstring to a descriptor list
 ************************************************************************/
OSErr AEPutPStr(AppleEvent *event,DescType key,PStr str)
{
	Str255 s;

	PCopy(s,str);
	return(AEPutParamPtr(event,key,typeChar,s+1,*s));
}

/************************************************************************
 * AEPutBool - put a boolean to a descriptor list
 ************************************************************************/
OSErr AEPutBool(AppleEvent *event,DescType key,Boolean b)
{
	char boolByte = b;

	return(AEPutParamPtr(event,key,typeBoolean,&boolByte,1));
}

/************************************************************************
 * GetAEText - Get text out of an AEDesc
 *		Must dispose of handle using AEDisposeDescDataHandle
 ************************************************************************/
Handle GetAEText(AEDescPtr desc,AEDesc *coerced,Boolean *wasCoerced)
{
	Handle data;
	
	if (desc->descriptorType==typeChar)
	{
		*wasCoerced = false;
		AEGetDescDataHandle(desc,&data);
	}
	else
	{
		*wasCoerced = true;
		if (!AECoerceDesc(desc,typeChar,coerced))
			AEGetDescDataHandle(coerced,&data);
		else
			data = nil;
	}
	return data;
}

/************************************************************************
 * GetAEPStr - Get a PStr out of an AEDesc
 ************************************************************************/
PStr GetAEPStr(PStr s,AEDescPtr desc)
{
	AEDesc coerced;
	Handle data;
	Boolean wasCoerced;

	if (data = GetAEText(desc,&coerced,&wasCoerced))
	{
		*s = GetHandleSize_(data);
		BMD(*data,s+1,*s);
		s[*s+1] = 0;	/* just for good measure */
		if (wasCoerced)	AEDisposeDesc(&coerced);	
		AEDisposeDescDataHandle(data); 
	}
	else
		*s = 0;

	return(s);
}

/************************************************************************
 * GetAELong - Get a short out of an AEDesc
 ************************************************************************/
long GetAELong(AEDescPtr desc)
{
	AEDesc coerced;
	long theLong;
	
	if (desc->descriptorType==typeLongInteger || desc->descriptorType==typeEnumerated)
		AEGetDescData(desc,&theLong,sizeof(theLong));
	else
	{
		if (AECoerceDesc(desc,typeLongInteger,&coerced)) return(0);
		AEGetDescData(&coerced,&theLong,sizeof(theLong));
		AEDisposeDesc(&coerced);
	}
	return(theLong);
}

/************************************************************************
 * GetAEFSSpec - Get an fsspec out of an AEDesc
 ************************************************************************/
OSErr GetAEFSSpec(AEDescPtr desc,FSSpecPtr spec)
{
	AEDesc coerced;
	OSErr err;
	
	if (desc->descriptorType==typeFSS)
		AEGetDescData(desc,spec,sizeof(FSSpec));
	else
	{
		if (err=AECoerceDesc(desc,typeFSS,&coerced)) return(err);
		AEGetDescData(&coerced,spec,sizeof(FSSpec));
		AEDisposeDesc(&coerced);
	}
	return(noErr);
}

/************************************************************************
 * GetAEBool - Get a boolean out of an AEDesc
 ************************************************************************/
Boolean GetAEBool(AEDescPtr desc)
{
	AEDesc coerced;
	Boolean b;
	
	if (desc->descriptorType==typeBoolean)
		AEGetDescData(desc,&b,sizeof(b));
	else
	{
		if (AECoerceDesc(desc,typeBoolean,&coerced)) return(False);
		AEGetDescData(&coerced,&b,sizeof(b));
		AEDisposeDesc(&coerced);
	}
	return(b);
}

/************************************************************************
 * GetAERect - Get a rect out of an AEDesc
 ************************************************************************/
OSErr GetAERect(AEDescPtr desc,Rect *r)
{
	AEDesc coerced;
	OSErr err;
	long junk;
	
	if (desc->descriptorType==typeRectangle)
		AEGetDescData(desc,r,sizeof(Rect));
	else if (desc->descriptorType==typeAEList)
	{
		if (!(err = AEGetNthPtr(desc,1,typeShortInteger,(void*)&junk,(void*)&junk,&r->left,sizeof(short),(void*)&junk)))
		if (!(err = AEGetNthPtr(desc,2,typeShortInteger,(void*)&junk,(void*)&junk,&r->top,sizeof(short),(void*)&junk)))
		if (!(err = AEGetNthPtr(desc,3,typeShortInteger,(void*)&junk,(void*)&junk,&r->right,sizeof(short),(void*)&junk)))
			err = AEGetNthPtr(desc,4,typeShortInteger,(void*)&junk,(void*)&junk,&r->bottom,sizeof(short),(void*)&junk);
	}
	else
	{
		if (err=AECoerceDesc(desc,typeRectangle,&coerced)) return(err);
		AEGetDescData(&coerced,r,sizeof(Rect));
		AEDisposeDesc(&coerced);
	}
	return(noErr);
}

/************************************************************************
 * AEPutRect - Put a rect into a an event
 ************************************************************************/
OSErr AEPutRect(AppleEvent *event,DescType key,Rect *r)
{
	return(AEPutParamPtr(event,key,typeQDRectangle, (void*)r, sizeof(*r)));
}

/************************************************************************
 * AEPutLongDate - put a long date into an event
 ************************************************************************/
OSErr AEPutLongDate(AppleEvent *event,DescType key,uLong secs)
{
	LongDateCvt longDate;
	Zero(longDate);
	longDate.hl.lLow = secs;
	return(AEPutParamPtr(event,key,typeLongDateTime, (void*)&longDate.c, sizeof(longDate)));
}

/************************************************************************
 * GetAEPoint - Get a point out of an AEDesc
 ************************************************************************/
OSErr GetAEPoint(AEDescPtr desc,Point *pt)
{
	AEDesc coerced;
	OSErr err;
	OSType junk;
	
	if (desc->descriptorType==typeQDPoint)
		AEGetDescData(desc,pt,sizeof(Point));
	else if (desc->descriptorType==typeAEList)
	{
		if (!(err = AEGetNthPtr(desc,1,typeShortInteger,(void*)&junk,(void*)&junk,&pt->h,sizeof(short),(void*)&junk)))
			err = AEGetNthPtr(desc,2,typeShortInteger,(void*)&junk,(void*)&junk,&pt->v,sizeof(short),(void*)&junk);
	}
	else
	{
		if (err=AECoerceDesc(desc,typeQDPoint,&coerced)) return(err);
		AEGetDescData(&coerced,pt,sizeof(Point));
		AEDisposeDesc(&coerced);
	}
	return(noErr);
}

/************************************************************************
 * NullADList - null a list of ad's
 ************************************************************************/
void NullADList(AEDescPtr first,...)
{
	va_list args;
	
	if (!first) return;
	
	va_start(args,first);
	do
	{
		NullDesc(first);
	}
	while (first = va_arg(args,AEDescPtr));
	va_end(args);
}

/************************************************************************
 * DisposeADList - dispose a list of ad's
 ************************************************************************/
void DisposeADList(AEDescPtr first,...)
{
	va_list args;
	
	if (!first) return;
	
	va_start(args,first);
	do
	{
		AEDisposeDesc(first);
		NullDesc(first);
	}
	while (first = va_arg(args,AEDescPtr));
	va_end(args);
}

/**********************************************************************
 * MyAEPutAlias - put an alias record into an event
 **********************************************************************/
OSErr MyAEPutAlias(AppleEvent *event, AEKeyword keyword, FSSpecPtr spec)
{
	OSErr err;

#ifdef NEVER
	err = NewAlias(nil,spec,&alias);
	if (!err)
	{
		err = AEPutParamPtr(event,keyword,typeAlias,LDRef(alias),GetHandleSize_(alias));
		ZapHandle(alias);
	}
#else
	err = AEPutParamPtr(event,keyword,typeFSS,spec,sizeof(FSSpec));
#endif
	return(err);
}

/************************************************************************
 * GetAEDefaultBool - get a boolean param, with a default if it's not there
 ************************************************************************/
Boolean GetAEDefaultBool(AppleEvent *ae,DescType param,Boolean deflt)
{
	Boolean paramBool;
	DescType gotType;
	Size gotSize;

	if (AEGetParamPtr_(ae,param,typeBoolean,&gotType,&paramBool,sizeof(Boolean),&gotSize))
		return(deflt);
	return(paramBool);
}

/*------------------------------------------------------------------------------
#
#	"AEForwardEvent.c"
#	Apple Macintosh Developer Technical Support
#	by Andy Bachorski
#
#	Copyright � 1996 Apple Computer, Inc.
#	All rights reserved.
#
#	Versions:	1.0		5/96		Andy Bachorski
#				1.1		7/23/96		Andy Bachorski && Pete Gontier
#
#*************************************************************************************
#**      You may incorporate this sample code into your applications without		**
#**      restriction, though the sample code has been provided "AS IS" and the		**
#**      responsibility for its operation is 100% yours.  However, what you are		**
#**      not permitted to do is to redistribute the source as "Apple Sample			**
#**      Code" after having made changes. If you're going to re-distribute the		**
#**      source, we require that you make it clear in the source that the code		**
#**      was descended from Apple Sample Code, but that you've made changes.		**
#*************************************************************************************
#
#------------------------------------------------------------------------------*/

//�������������������������������������������//
//�����         MoveReplyParams         �����//
//�������������������������������������������//

OSErr CopyReply (const AppleEvent *sourceReplyEvent, AppleEvent *targetReplyEvent)
{
	OSErr err = noErr;

	AEDesc replyMagic;

	if (!(err = AEGetAttributeDesc (targetReplyEvent,keyAddressAttr,typeWildCard,&replyMagic)))
	{
		OSErr err2 = noErr;

		AEDesc returnMagic;

		if (!(err = AEGetAttributeDesc (targetReplyEvent,keyReturnIDAttr,typeWildCard,&returnMagic)))
		{
			if (!(err = AEDisposeDesc (targetReplyEvent)))
				if (!(err = AEDuplicateDesc (sourceReplyEvent,targetReplyEvent)))
					if (!(err = AEPutAttributeDesc (targetReplyEvent,keyAddressAttr,&replyMagic)))
						err = AEPutAttributeDesc (targetReplyEvent,keyReturnIDAttr,&returnMagic);

			err2 = AEDisposeDesc (&returnMagic);
			if (!err) err = err2;
		}

		err2 = AEDisposeDesc (&replyMagic);
		if (!err) err = err2;
	}

	return err;
}
//end DuplicateEventReply

//�������������������������������������//
//�����    ReflectAppleEvent      �����//
//�������������������������������������//

OSErr	ReflectAppleEvent (const AppleEvent *appleEvent, AppleEvent *replyEvent)
{
	OSErr	anErr;
	DECLARE_UPP(MyAEIdle,AEIdle);

	AppleEvent appleEventToForward;
	anErr = AEDuplicateDesc (appleEvent, &appleEventToForward);

	if ( !anErr )
	{
		OSErr err2 = noErr;

		Boolean		senderWantsReply	= replyEvent->descriptorType != typeNull;
		AESendMode	sendMode			= senderWantsReply ? kAEWaitReply : kAENoReply;
		AppleEvent	replyToForward		= { typeNull, nil };

		INIT_UPP(MyAEIdle,AEIdle);
		anErr = AESend (&appleEventToForward,&replyToForward,sendMode,kAENormalPriority,60*15,MyAEIdleUPP,nil);
		err2 = AEDisposeDesc( &appleEventToForward );
		if (!anErr) anErr = err2;

		if (!anErr && senderWantsReply)
		{		
			anErr = CopyReply (&replyToForward, replyEvent);
			AEDisposeDesc( &replyToForward );
		}
	}

	return anErr;
}
