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

#include "uudecode.h"
#define FILE_NUM 46
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Major modifications Copyright (c)1991-1992, Apple Computer Inc. */
/* More modifications Copyright (c)1993, QUALCOMM Incorporated */
/************************************************************************
 * functions to convert files from uuencoded applesingle (yuck!)
 * Major modifications (c)1991-1992, Apple Computer Inc.
 * released to public domain.
 ************************************************************************/

#pragma segment Abomination

#define SINGLE_MAGIC 0x00051600
#define DOUBLE_MAGIC 0x00051607
#define OLD_VERSION	0x00010000
#define MAP_NAME 3
#define MAP_RFORK 2
#define MAP_DFORK 1
#define MAP_DATES 8
#define MAP_INFO 9
#define NEW_VERSION 0x00020000

typedef struct
{
	uLong type;
	uLong offset;
	uLong length;
} Map, *MapPtr;

typedef struct
{
	uLong magic;
	uLong version;
	char homefs[16];
	uShort mapCount;
	Map maps[9];
} UUHeader;

typedef struct UUGlobals_ UUGlobals, **UUGlobalsHandle;
struct UUGlobals_
{
	UUHeader header; /* AppleSingle header */
	AbStates state; /* Current decoder state */
	UHandle buffer; /* receive map buffer */
	short bSpot; /* current point in receive map buffer */
	short bSize; /* Size of receive map buffer */
	FSSpec spec;	/* FSSpec */
	short refN;	/* file ref number */
	Str63 tmpName; /* temporary file name */
	Str255 name; /* file name */
	long offset; /* Offset into the stream */
	long currmap; /* Current map that we are working on, set in AbNextState */
	Boolean seenFinfo; /* Have we found the Finfo in the stream yet? */
	Boolean seenName; /* Have we found the real file name in the stream yet? */
	Boolean hasName; /* Are we going to find the real file name in the stream ? */
	Boolean usedTemp; /* Did we use a temporary name? */
	Boolean noteAttached; /* Did we attache the enclosure note yet? */
	Boolean invalState;	/* have we told the user things have gone awry? */
	FInfo info; /* Macintosh file information */
	FXInfo xInfo;	/* More Macintosh file information */
	short mailboxRefN;	/* ref number of mailbox */
	long origOffset;	/* offset where we found first indication of file */
	Boolean isText;	/* is this text data? */
	Boolean wasCR;	/* was the last char a CR? */
	Boolean hasDates;
	uLong dates[4];
	HeaderDHandle hdh;
};

#define Hdh (*UUG)->hdh
#define Header (*UUG)->header
#define HeaderData ((UPtr)&Header)
#define State (*UUG)->state
#define Buffer (*UUG)->buffer
#define BSpot (*UUG)->bSpot
#define BSize (*UUG)->bSize
#define Spec (*UUG)->spec
#define Name (*UUG)->name
#define TmpName (*UUG)->tmpName
#define Maps Header.maps
#define Info (*UUG)->info
#define InfoData ((UPtr)&Info)
#define XInfo (*UUG)->xInfo
#define XInfoData ((UPtr)&XInfo)
#define RefN (*UUG)->refN
#define Offset (*UUG)->offset
#define CurrMapNum (*UUG)->currmap
#define CurrMap Header.maps[(*UUG)->currmap]
#define SeenFinfo (*UUG)->seenFinfo
#define SeenName (*UUG)->seenName
#define HasName (*UUG)->hasName
#define UsedTemp (*UUG)->usedTemp
#define NoteAttached (*UUG)->noteAttached
#define MailboxRefN (*UUG)->mailboxRefN
#define OrigOffset (*UUG)->origOffset
#define InvalState (*UUG)->invalState
#define MapCount Header.mapCount
#define IsText (*UUG)->isText
#define WasCR (*UUG)->wasCR
#define HasDates (*UUG)->hasDates
#define Dates (*UUG)->dates

short UULine(UPtr text, long size);
Boolean UUData(uShort byte);
short AbOpen(void);
short AbClose(void);
short AbWriteBuffer(void);
Boolean AbNameStuff(uShort byte);
Boolean AbNextState( void );
Boolean AbTempName( void );
Boolean AbSetFinfo(uShort byte);
Boolean AbSaveFDates(uShort byte);
OSErr AbSetDates(void);
short ClearAbomination(void);
OSErr SendFromOpenFile(TransStream stream,DecoderFunc *encoder,short refN,long size);
OSErr UUDecodeLine(UPtr encoded,long size,UPtr decoded,long *binSize);
Boolean IsAppleSomething(UPtr text,long size);
void RemoveDuds(void);
void UUFileName(PStr uuName,PStr shortName);
Boolean ReallyIsText(FSSpecPtr spec);
Boolean JustDataWanna(MIMEMapPtr hintMM);
short SaveJustData(UPtr encoded,long size);
PStr GetLongName(PStr longName,FSSpecPtr spec);

#pragma segment POP
/************************************************************************
 * ConvertUUSingle - the UUencoded AppleSingle converter
 ************************************************************************/
Boolean ConvertUUSingle(short refN,UPtr buf,long *size,POPLineType lineType,long estSize,MIMEMapPtr hintMM,HeaderDHandle hdh)
{
#pragma unused(estSize)
	long offset;
	
	if (!UUG)
	{
		BeginAbomination("",hdh);
		if (!UUG) return(False);
	}
	
	switch(State)
	{
		case AbDone:
			if (lineType==plComplete && IsAbLine(buf,*size,hdh))
			{
				State = NotAb;
				GetFPos(refN,&offset);  /* save start */
				MailboxRefN = refN;
				OrigOffset = offset;
			}
			break;
		
		case NotAb:
			/*
			 * we just saw a line that looked like a begin line
			 * if this line is the right length, we'll give it a go
			 */
			if (lineType==plComplete && UURightLength(buf,*size)>=0)
			{
				if (!IsAppleSomething(buf,*size))
				{
					if (JustDataWanna(hintMM))
					{
						SaveAbomination(buf,*size);
						*size = 0;
					}
					else
						ClearAbomination();
				}
				else
					SaveAbomination(buf,*size);
			}
			else
				ClearAbomination();
			break;
		
		case AbHeader:
		case AbName:
			SaveAbomination(buf,*size);
			if (State>AbName && State!=AbDone)
				*size = 0;	/* do NOT save the line into the message proper */
			break;
							
		default:
			if (OrigOffset)
			{
				TruncOpenFile(refN,OrigOffset);	/* toss the saved bits */
				OrigOffset = 0;
			}
			SaveAbomination(buf,*size);
			*size = 0;	/* do NOT save the line into the message proper */
			break;
	}
	
	/*
	 * We're uudecoding unless we're not
	 */
	return(State!=AbDone);
}

/************************************************************************
 * ConvertSingle - the AppleSingle converter
 ************************************************************************/
Boolean ConvertSingle(short refN,UPtr buf,long size)
{
	UPtr spot,end;
	
	if (!UUG) return(False);
	if (!size) return(False);
	
	switch(State)
	{
		case AbDone:
			State = NotAb;
			MailboxRefN = refN;
		
		default:
			for (spot=buf,end=spot+size;spot<end;spot++)
				if (!UUData(*spot)) break;
			break;
	}
	
	/*
	 * if we're not decoding anymore, kill the globals
	 */
	if (State==AbDone) SaveAbomination(nil,0);
	
	/*
	 * We're uudecoding unless we're not
	 */
	return(UUG!=nil);
}

/************************************************************************
 * IsAbLine - does the UUencoded applesingle file begin?
 ************************************************************************/
Boolean IsAbLine(UPtr text, long size, HeaderDHandle hdh)
{
	UPtr spot;
	Str63 name;
	char *namePtr;
	short i = 0;
	UPtr permSpot;
	
	namePtr = name;
	if (size<11) return(False);
	if (strncmp(text,"begin ",6)) return(False);
	permSpot = text + 6;
	while (*permSpot==' ') permSpot++;
	spot = permSpot;
	while (*spot>='0' && *spot<='7') spot++;
	if (*spot!=' ' || spot-permSpot > 5 || spot-permSpot<3) return(False);
	if (spot[1]=='\015') return(False);
	if( !BeginAbomination("",hdh) ) return(False);
	spot++; /* skip the space */
	namePtr++; /* Skip the size */
	while( (*spot != '\015') && (i < 63) ){
				*namePtr++ = *spot++;
				i++;
	}
				if( i>27 ) i = 27; /* Trim filname so number can fit on end */
	name[0] = i;
	Other2MacName(name,name);
	PCopy(Name,name);
	return(True);
}

Boolean BeginAbomination( PStr name, HeaderDHandle hdh)
{
	if (UUG==nil)
	{
		if ((UUG=NewZH(UUGlobals))==nil) return( false );
		ClearAbomination();
		Hdh = hdh;
		// watch out for long filenames here!
		PSCopy(Spec.name,name);
		if (*Spec.name>31) *Spec.name = 31;
	}
	return( true );
}

/************************************************************************
 * SaveAbomination - returns the state of the converter
 ************************************************************************/
short SaveAbomination(UPtr text, long size)
{
	if (!text)
	{
		if (UUG)
		{
			if (State==AbJustData) BadBinHex = True;
			else if (State!=AbDone)
			{
				if (State > AbHeader) AbNextState();
				if (State!=AbDone && State!=AbExcess) BadBinHex = True;
			}
			if (AbClose()) BadBinHex = True;
			if (Spec.vRefNum && CommandPeriod)
				{FSpDelete(&LDRef(UUG)->spec);ASSERT(0);}
			else if (Spec.vRefNum && HasDates)
				AbSetDates();
			if (Buffer) ZapHandle(Buffer);
			ZapHandle(UUG);
			PopProgress(False);
		}
		return(AbDone);
	}
	return(State==AbJustData ? SaveJustData(text,size) : UULine(text,size));
}

/************************************************************************
 * ClearAbomination - zero the UUG
 ************************************************************************/
short ClearAbomination(void)
{
	AbClose();
	State = AbDone;
	Offset = -1;
	SeenFinfo = false;
	NoteAttached = false;
	SeenName = false;
	HasName = false;
	UsedTemp = false;
	OrigOffset = 0;
	InvalState = IsText = WasCR = false;
	return(AbDone);
}

#pragma segment Abomination
#define UU(c) (((c)-' ')&077)
/************************************************************************
 * UULine - handle a line of uuencoded stuff
 ************************************************************************/
short UULine(UPtr text, long size)
{
	short length;
	Boolean result=True;
	
	/*
	 * check for end line
	 */
	if ((size==3 || size==4) && !striscmp("end\015",text))
	{
		if (State!=AbJustData && State!=AbDone)
		{
			AbNextState();
			if (State!=AbDone && State!=AbExcess)
			{
				WarnUser(BINHEX_SHORT,0);
				ClearAbomination();
				BadBinHex = True;
				return(AbDone);
			}
		}
		return(ClearAbomination());
	}
	
	/*
	 * check for invalid start char
	 */
	if (*text<' ' || *text>'`')
	{
		WarnUser(BINHEX_BADCHAR,*text);
		ClearAbomination();
		BadBinHex = True;
		return(AbDone);
	}
	
	if (State==AbDone) State=NotAb;
	
	/*
	 * check length of line against line count
	 */
	if ((length=UURightLength(text,size))<0)
	{
		WarnUser(UU_BAD_LENGTH,(length+2)/3-((size*3)/4)/3);
		ClearAbomination();
		BadBinHex = True;
		return(AbDone);
	}
	
	/*
	 * empty lines mean nothing
	 */
	if (length==0) return(State);
	
	/*
	 * skip length byte, and trailing newline
	 */
	text++; size--;
	if (text[size-1]=='\015') size--;
	
	/*
	 * hey!  we're ready to decode!
	 */
	for (;length>0;text+=4,length-=3)
	{
		if (text[0]<' ' || text[0]>'`' || text[1]<' ' || text[1]>'`' ||
				length>1 && (text[2]<' ' || text[2]>'`') ||
				length>2 && (text[3]<' ' || text[3]>'`'))
		{
			WarnUser(BINHEX_BADCHAR,0);
			ClearAbomination();
			BadBinHex = True;
			return(AbDone);
		}
		if (!(result=UUData(0xff & (UU(text[0])<<2 | UU(text[1])>>4)))) break;
		if (length>1 && !(result=UUData(0xff & (UU(text[1])<<4 | UU(text[2])>>2))))
			break;
		if (length>2 && !(result=UUData(0xff & (UU(text[2])<<6 | UU(text[3])))))
			break;
	}
	if (!result) return(ClearAbomination());
	return(State);
}

/************************************************************************
 * IsAppleSomething - is this file applesingle or appledouble?
 ************************************************************************/
Boolean IsAppleSomething(UPtr text,long size)
{
	long magic;
	long mSize = sizeof(long);
	
	if (UUDecodeLine(text,size,(void*)&magic,&mSize)) return(True);	/* let applesingle report errors */
	if (magic==SINGLE_MAGIC  || magic==DOUBLE_MAGIC) return(True);
	return(False);
}

/************************************************************************
 * JustDataWanna - do we want to save the data fork of this file?
 ************************************************************************/
Boolean JustDataWanna(MIMEMapPtr hintMM)
{
	FSSpec spec;
	MIMEMap mm;
	FInfo info;
	OSErr err;
	
	PCopy(spec.name,Name);
	if (!*spec.name) GetRString(spec.name,UNTITLED);
	if (!hintMM) FindMIMEMapPtr("\p?","\p?",spec.name,&mm);
	else mm = *hintMM;
	
	if (AutoWantTheFile(&spec,False,Hdh ? (*Hdh)->relatedPart:false)/*|| WantTheFile(&spec)*/)
	{
		err = FSpCreate(&spec,mm.creator,mm.type,smSystemScript);
		if (err==dupFNErr)
		{
			FSpGetFInfo(&spec,&info);
			info.fdType = mm.type;
			info.fdCreator = mm.creator;
			SafeInfo(&info,nil);
			FSpSetFInfo(&spec,&info);
			err = 0;
		}
		Spec = spec;
		if (err) {PopProgress(False); FileSystemError(BINHEX_CREATE,spec.name,err); return(False);}
		if (err = AbOpen()) return(False);
		State = AbJustData;
		IsText = (mm.flags & mmIsText)!=0;
		err = RecordAttachment(&spec,Hdh);
		Spec = spec;	// RecordAttachment may have changed the name....
		if (err) ClearAbomination();
		return(True);
	}
	return(False);
}

/************************************************************************
 * SaveJustData - save into a data fork
 ************************************************************************/
short SaveJustData(UPtr encoded,long size)
{
	Str63 decoded;
	short err;
	long binSize;
	
	/*
	 * check for end line
	 */
	if ((size==3 || size==4) && !striscmp("end\015",encoded))
		return(ClearAbomination());
	
	/*
	 * save
	 */
	binSize = sizeof(decoded);
	if (!(err = UUDecodeLine(encoded,size,decoded,&binSize)))
	{
		if (BSpot + binSize > BSize) err = AbWriteBuffer();
		if (!err)
		{
			BMD(decoded,*Buffer+BSpot,binSize);
			BSpot += binSize;
		}
	}
	if (err) ClearAbomination();
	return(State);
}
	
/************************************************************************
 * UUDecodeLine - decode a uuencoded line
 ************************************************************************/
OSErr UUDecodeLine(UPtr encoded,long size,UPtr decoded,long *binSize)
{
	UPtr spot,end;
	short len;
	
	spot = decoded;
	end = decoded + *binSize;
	*binSize = 0;

	/*
	 * check len of line against line count
	 */
	if ((len=UURightLength(encoded,size))<0)
	{
		WarnUser(UU_BAD_LENGTH,(len+2)/3-((size*3)/4)/3);
		BadBinHex = True;
		return(1);
	}
	
	/*
	 * empty lines mean nothing
	 */
	if (len==0) return(noErr);
	
	if (!IsText)
	{
		for (encoded++;len>0;encoded+=4,len-=3)
		{
			if (spot<end) *spot++ = 0xff & (UU(encoded[0])<<2 | UU(encoded[1])>>4);
			if (len>1 && spot<end) *spot++ = 0xff & (UU(encoded[1])<<4 | UU(encoded[2])>>2);
			if (len>2 && spot<end) *spot++ = 0xff & (UU(encoded[2])<<6 | UU(encoded[3]));
		}
	}
	else
	{
		/*
		 * we're decoding a text file.  Turn CRLF into CR, LF into CR, leave CR alone
		 */
#define FIX_NL																																\
		do {																																			\
			if (spot[-1]=='\012')																											\
			{																																				\
				if (WasCR) spot--;		/* turn CRLF into just CR */										\
				else spot[-1] = '\015';	/* turn bare LF (like from UNIX) into CR */			\
				WasCR = False;																												\
			}																																				\
			else																																		\
				WasCR = spot[-1]=='\015';																								\
		} while(0)

		for (encoded++;len>0;encoded+=4,len-=3)
		{
			if (spot<end) *spot++ = 0xff & (UU(encoded[0])<<2 | UU(encoded[1])>>4);
			FIX_NL;
			if (len>1 && spot<end) *spot++ = 0xff & (UU(encoded[1])<<4 | UU(encoded[2])>>2);
			FIX_NL;
			if (len>2 && spot<end) *spot++ = 0xff & (UU(encoded[2])<<6 | UU(encoded[3]));
			FIX_NL;
		}
	}
	*binSize = spot-decoded;
	return(noErr);
}

/************************************************************************
 * UURightLength - find out if the current line is of the proper length
 *  Returns length if so, negative number if mismatch
 ************************************************************************/
long UURightLength(UPtr text,long size)
{
	long length = UU(*text);
	
	if (*text<' ' || *text>'`') return(-1);  /* BZZZZT */
	
	return(length);	// meaningless, really, but that's uuencode for you
}
	

/************************************************************************
 * UUData - handle a data character
 ************************************************************************/
Boolean UUData(uShort byte)
{
	Boolean result=True;
	
	Offset++;
	switch (State)
	{
		case NotAb:
								State = AbHeader;
		case AbHeader:
								HeaderData[BSpot++] = byte;
								if (BSpot>=(sizeof(UUHeader)-(sizeof(Map)*9))) {
												if( Header.magic != SINGLE_MAGIC &&
														Header.magic != DOUBLE_MAGIC ) {
																WarnUser(UU_BAD_VERSION,Header.magic);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
												if( (Header.version != OLD_VERSION) && (Header.version != NEW_VERSION) ) {
																WarnUser(UU_BAD_VERSION,Header.version);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
												if( (Header.mapCount<1) || (Header.mapCount>9) ) {
																WarnUser(UU_INVALID_MAP,Header.mapCount);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
								}
								if( BSpot>=(sizeof(UUHeader) - (sizeof(Map) * (9 - Header.mapCount))) ) {
												RemoveDuds();
												AbNextState();
												if(State != AbName){
																result = AbTempName();
												}
												BSpot = 0;
								}
								break;
								
		case AbName:
			if (Offset<CurrMap.offset) break;
			result = AbNameStuff(byte);
			break;
					
		case AbFinfo:
			if (Offset<CurrMap.offset) break;
			result = AbSetFinfo(byte);
			break;
	
		case AbFDates:
			if (Offset<CurrMap.offset) break;
			result = AbSaveFDates(byte);
			break;
	
		case AbResFork:
		case AbDataFork:
						if (Offset<CurrMap.offset) break;
						if (Offset>=CurrMap.offset+CurrMap.length ) {
										result = !AbClose();
										Offset--;AbNextState();
										if (result) {result=UUData(byte);}
						}
						else if (!RefN && AbOpen())
										result = False;
						else {
										(*Buffer)[BSpot++] = byte;
										if (BSpot>=BSize && AbWriteBuffer()) result=False;
						}
						break;
						
		case AbSkip:
						if( Offset < CurrMap.offset ) break;
						if(Offset>=CurrMap.offset+CurrMap.length ) {
										Offset--;
										AbNextState();
										BSpot = 0;
										result = UUData( byte );
						}
						break;
						
		default:
			result = True;	/* sorry, invalid state is not meaningful -- AppleSingle files can be padded to any length */
			break;
	}
	return(result); 		
}

/************************************************************************
 * Remove dopey zero-length maps
 ************************************************************************/
void RemoveDuds(void)
{
	short okMap, onMap;
	
	okMap = 0;
	for (onMap=0;onMap<MapCount;onMap++)
	{
		if (Maps[onMap].length!=0)
		{
			if (okMap!=onMap)
				Maps[okMap] = Maps[onMap];
			okMap++;
		}
	}
	MapCount = okMap;
}

Boolean AbTempName( void )
{
	short err;
	FSSpec spec;

	UsedTemp = true;
	spec = Spec;
	
	if (!*spec.name) GetRString(spec.name,SINGLE_TEMP);
	if (AutoWantTheFile(&spec,False,(*Hdh)->relatedPart)/*|| WantTheFile(&spec)*/)
	{
		Spec = spec;
		PCopy(TmpName,Spec.name);
		if (err=FSpCreate(&spec,'CSOm','TEMP',smSystemScript))
		{
			if (err == dupFNErr) err = noErr;
			else
			{
				FileSystemError(BINHEX_CREATE,spec.name,err);
				(void) ClearAbomination();
				return(False);
			}
		}
				AbNextState();
		BSpot = 0;
	}
	else
	{
		State = AbDone;
		return(False);
	}
	
	return(True);
}


Boolean AbNameStuff(uShort byte)
{
	Str255 name;
	short err;
	FSSpec spec;

	// be careful about long filenames here
	if (BSpot<255) Name[++BSpot] = byte;
	
	if (BSpot<CurrMap.length) return(True);
				if (CurrMap.length > 255){ /* Trim name so number fits! */
								*Name = 255;
				} else {
				*Name = CurrMap.length;
				}
	spec = Spec;
	SeenName = true;
	if( !UsedTemp ){
					// The situation here is that the first map we've come to is the name map
					// That means we can go ahead and create the file with the proper name
					// That name is kept in the globals, in the field Name
					
					// First, create the file with some name, it doesn't much matter
					// what, but we'll base it on the name we were given.  What does matter
					// is that we use AutoWantTheFile to create it, since it will choose
					// the proper folder to put the file in.
					PSCopy(spec.name,Name);
					*spec.name = MIN(*spec.name,27);
					AutoWantTheFile(&spec,False,(*Hdh)->relatedPart);
					err = FSpCreate(&spec,'CSOm','TEMP',smSystemScript);
										
					// If the name was very long, rename it to the proper name here
					if (!err && *Name>27)
					{
						PSCopy(name,Name);	// copy to temp var to avoid memory movement...
						MakeUniqueLongFileName(spec.vRefNum,spec.parID,name,kTextEncodingUnknown,sizeof(name)-1);
						err = FSpSetLongName(&spec,kTextEncodingUnknown,name,&spec);
						PSCopy(Name,name);  // and copy back...
					}
					
					// Did we win?
					if (err)
					{
						FileSystemError(BINHEX_CREATE,spec.name,err);
						(void) ClearAbomination();
						return(False);
					}
					
					// Now, copy the stuff we used back into the globals
					Spec = spec;
					PCopy( TmpName, Name );
					AbNextState();
					BSpot = 0;
					if( SeenName && SeenFinfo && !NoteAttached){
									err = RecordAttachment(&spec,Hdh);
									Spec = spec;	// RecordAttachment may have changed the name
									NoteAttached = true;
									if (err) ClearAbomination();
					}
	} else {
		// Here, on the other hand, we didn't come across the name map first.
		// Instead, we had to write one or more maps using a temporary filename,
		// which is in Spec and TmpName.  The correct name is now in the globals,
		// in the field Name.  So we need to rename our temporary file.
		spec = Spec;
		PSCopy(name,Name);
		if (!StringSame(spec.name,name)) 
		{
			// the name differs from temp name
			// We're just going to use the long filename routine here,
			// since it will also work for short filenames
			MakeUniqueLongFileName(spec.vRefNum,spec.parID,name,kTextEncodingUnknown,sizeof(name)-1);
			err = FSpSetLongName(&spec,kTextEncodingUnknown,name,&spec);
			if (!err) 
			{
				Spec = spec;
				PSCopy( Name, name );
				PSCopy( TmpName, name );
			}
			else
			{
				/* Rename failed, name stays TmpName */
				PCopy( name, TmpName );
				PCopy( Name, name );
			}
		}
		AbNextState();
		BSpot = 0;
		if( SeenName && SeenFinfo && !NoteAttached){
						err = RecordAttachment(&spec,Hdh);
						Spec = spec;	// RecordAttachment may have changed the name
						if (err) ClearAbomination();
						NoteAttached = true;
		}
	}
	return(True);
}

Boolean AbSetFinfo(uShort byte)
{
	FInfo info;
	FXInfo fxInfo;
	short err;
	FSSpec spec = Spec;

				if(!SeenFinfo){
								if( BSpot<sizeof(FInfo) ){
												InfoData[BSpot++] = byte;
								} else if (BSpot<sizeof(FInfo)+sizeof(FXInfo)) {
												XInfoData[BSpot++ - sizeof(FInfo)] = byte;
								}
								if (BSpot<CurrMap.length) return(True);
								SeenFinfo = true;
				}
				info = Info;
				fxInfo = XInfo;
				SafeInfo(&info,&fxInfo);
				if (err=FSpSetFInfo(&spec,&info))
				{
					FileSystemError(BINHEX_OPEN,spec.name,err);
					(void) ClearAbomination();
					return(False);
				}
				FSpSetFXInfo(&spec,&fxInfo);
				if( SeenName && SeenFinfo && !NoteAttached){
								err = RecordAttachment(&spec,Hdh);
								Spec = spec;	// RecordAttachment may have changed the name
								if (err) ClearAbomination();
								NoteAttached = true;
				}
				BSpot = 0;
				AbNextState();
				return(True);
}

#define SLOPPY_MILLENIUM 3029529600

Boolean AbSaveFDates(uShort byte)
{
	short i;

	if (BSpot<sizeof(Dates))
	{
		((Ptr)Dates)[BSpot++] = byte;
	}
	else
	{
		BSpot++;
	}
	if (BSpot<CurrMap.length) return(True);
	
	HasDates = True;
	for (i=0;i<sizeof(Dates)/sizeof(long);i++)
		Dates[i] += SLOPPY_MILLENIUM;
	BSpot = 0;
	AbNextState();
	return(True);
}

OSErr AbSetDates(void)
{
	FSSpec spec = Spec;
	CInfoPBRec hfi;
	OSErr err = noErr;
	uLong tooEarly = (uLong)GetRLong(TOO_EARLY_FILE);
	
	if (!(err = HGetCatInfo(spec.vRefNum,spec.parID,spec.name,&hfi)))
	{
		hfi.hFileInfo.ioFlCrDat = Dates[0] > tooEarly ? Dates[0] : LocalDateTime();
		hfi.hFileInfo.ioFlMdDat = Dates[1] > tooEarly ? Dates[1] : LocalDateTime();
		hfi.hFileInfo.ioFlBkDat = Dates[2] > tooEarly ? Dates[2] : LocalDateTime();
		err = HSetCatInfo(spec.vRefNum,spec.parID,spec.name,&hfi);
	}
	return err;
}


/************************************************************************
 *
 ************************************************************************/
short AbOpen(void)
{
	short err;
	short refN;
	UHandle buffer;
	FSSpec spec = Spec;
	
	if (!Buffer)
		if (buffer=NuHTempBetter(GetRLong(RCV_BUFFER_SIZE)))
			Buffer = buffer;
		else
			return(WarnUser(BINHEX_MEM,err=MemError()));
	BSize = GetHandleSize_(Buffer);
	if (err=(State==AbResFork?FSpOpenRF(&spec,fsRdWrPerm,&refN):FSpOpenDF(&spec,fsRdWrPerm,&refN)))
		FileSystemError(BINHEX_OPEN,spec.name,err);
	else
		RefN = refN;
	BSpot = 0;
	return(err);
}

/************************************************************************
 *
 ************************************************************************/
short AbClose(void)
{
	short wrErr=0;
	short err;
	
	if (!RefN) return(noErr);
	if (BSpot) wrErr = AbWriteBuffer();
	err = MyFSClose(RefN);
	if (!wrErr && err) {LDRef(UUG);FileSystemError(BINHEX_WRITE,Name,err);UL(UUG);}
	RefN = 0;
	BSpot = 0;
	return(wrErr ? wrErr : err);
}

/************************************************************************
 *
 ************************************************************************/
short AbWriteBuffer(void)
{
	long writeBytes = BSpot;
	int err;
	
	if (err=NCWrite(RefN,&writeBytes,LDRef(Buffer)))
		FileSystemError(BINHEX_WRITE,Name,err);
	UL(Buffer);
	BSpot = 0;
	return(err);
}

Boolean AbNextState( void )
{
				short i;
				long biggest = 0;
				long totalLen = 0;

				CurrMapNum = 0;
				for( i=0; i<Header.mapCount; i++){ /* Find the biggest offset */
								if( Header.maps[i].offset > biggest )
								{
									biggest = Header.maps[i].offset + 1;
									totalLen = biggest + Header.maps[i].length - 2;
								}
								if( Header.maps[i].type == 3 ) HasName = true;
								
				}
				if (Offset >= totalLen) State = AbExcess;
				else
				  for( i=0; i<Header.mapCount; i++){ /* Find the header that is next */ 	
								if ( (Header.maps[i].offset > Offset) && (Header.maps[i].offset < biggest) ) {
												CurrMapNum = i;
												biggest = CurrMap.offset + 1;
												switch (CurrMap.type){
																case 1: /* Data Fork */
																				State = AbDataFork;
																				break;
																case 2: /* Resource Fork */
																				State = AbResFork;
																				break;
																case 3: /* Real file name */
																				State = AbName;
																				break;
																case 9: /* File Finder information */
																				State = AbFinfo;
																				break;
																case 8: /* File dates */
																				State = AbFDates;
																				break;
																default:
																				AddAttachInfo( (int)UU_SKIP_MAP_INFO, (int)CurrMap.type );
																				State = AbSkip;
																				break;
												}
								}
				}

				if (State==AbExcess && !NoteAttached)
				{
					FSSpec spec = Spec;
					FInfo info = Info;
					
					if (SeenFinfo) FSpSetFInfo(&spec,&info);
					if (RecordAttachment(&spec,Hdh)) ClearAbomination();
					Spec = spec;	// RecordAttachment may have changed the name
					NoteAttached = true;
				}

				return( true );
}

#pragma segment SendAppleFile
/************************************************************************
 * SendSingle - send a file in AppleSingle format
 ************************************************************************/
OSErr SendSingle(TransStream stream,FSSpecPtr spec,Boolean dataToo,AttMapPtr amp)
{
	UUHeader header;
	short mapCount = 2;
	CInfoPBRec hfi;
	short err;
	long offset;
	short curMap = 0;
	short refN=0;
	AttMap localAM = *amp;
	long dates[4];
	
	WriteZero(&header,sizeof(header));
	if (err=FSpGetHFileInfo(spec,&hfi))
		return(FileSystemError(BINHEX_OPEN,spec->name,err));
	
	/*
	 * send the MIME header
	 */
	if (err = ComposeRTrans(stream,MIME_V_FMT,InterestHeadStrn+hContentEncoding,
									 MIME_BASE64,NewLine))
		goto done;
	if (!dataToo)
	{
		ComposeRString(localAM.shortName,DOUBLE_RFORK_FMT,amp->shortName);
		if (*amp->longName) ComposeRString(localAM.longName,DOUBLE_RFORK_FMT,amp->longName);
	}
		
	if (err = MIMEFileHeader(stream,&localAM,MIME_APPLEFILE,hfi.hFileInfo.ioFlMdDat)) goto done;
	if (err = SendPString(stream,NewLine)) goto done;

	/*
	 * fill in the AppleSingle header
	 */
	header.mapCount = 3;
	if (dataToo && hfi.hFileInfo.ioFlLgLen) header.mapCount++;
	if (hfi.hFileInfo.ioFlRLgLen) header.mapCount++;
	header.magic = dataToo ? SINGLE_MAGIC : DOUBLE_MAGIC;
	header.version = NEW_VERSION;
	offset = sizeof(header)-sizeof(header.maps)+header.mapCount*sizeof(Map);
	
	/* filename */
	header.maps[curMap].type = MAP_NAME;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = *spec->name;
	
	offset += header.maps[curMap++].length;
	
	/* finder information */
	header.maps[curMap].type = MAP_INFO;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = 32;
	
	offset += header.maps[curMap++].length;
	
	/* dates */
	header.maps[curMap].type = MAP_DATES;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = 16;
	
	offset += header.maps[curMap++].length;
	
	/* resource fork? */
	if (hfi.hFileInfo.ioFlRLgLen)
	{
		header.maps[curMap].type = MAP_RFORK;
		header.maps[curMap].offset = offset;
		header.maps[curMap].length = hfi.hFileInfo.ioFlRLgLen;
		
		offset += header.maps[curMap++].length;
	}
	
	/* data fork? */
	if (dataToo && hfi.hFileInfo.ioFlLgLen)
	{
		header.maps[curMap].type = MAP_DFORK;
		header.maps[curMap].offset = offset;
		header.maps[curMap].length = hfi.hFileInfo.ioFlLgLen;
		
		offset += header.maps[curMap++].length;
	}
	
	/*
	 * hey!  we're getting there!  allocate a buffer
	 */	
	DontTranslate = True;
	
	/*
	 * send the header
	 */
	if (err=BufferSend(stream,B64Encoder,(void*)&header,header.maps[0].offset,False)) goto done;
	
	/*
	 * send the name
	 */
	if (err=BufferSend(stream,B64Encoder,spec->name+1,*spec->name,False)) goto done;
	
	/*
	 * send the info
	 */
	if (err=BufferSend(stream,B64Encoder,(void*)&hfi.hFileInfo.ioFlFndrInfo,16,False)) goto done;
	if (err=BufferSend(stream,B64Encoder,(void*)&hfi.hFileInfo.ioFlXFndrInfo,16,False)) goto done;
	
	/*
	 * send the dates
	 */
	dates[0] = hfi.hFileInfo.ioFlCrDat - SLOPPY_MILLENIUM;
	dates[1] = hfi.hFileInfo.ioFlMdDat - SLOPPY_MILLENIUM;
	dates[2] = hfi.hFileInfo.ioFlBkDat - SLOPPY_MILLENIUM;
	dates[3] = (long)LocalDateTime() - SLOPPY_MILLENIUM;
	if (err=BufferSend(stream,B64Encoder,(void*)dates,16,False)) goto done;
	
	/*
	 * resource fork?
	 */
	if (hfi.hFileInfo.ioFlRLgLen)
	{
		if (err=FSpOpenRF(spec,fsRdPerm,&refN))
		{
			FileSystemError(BINHEX_OPEN,spec->name,err);
			goto done;
		}
		if (err=SendFromOpenFile(stream,B64Encoder,refN,hfi.hFileInfo.ioFlRLgLen)) goto done;
		MyFSClose(refN); refN = 0;
	}
	
	/*
	 * data fork?
	 */
	if (dataToo && hfi.hFileInfo.ioFlLgLen)
	{
		if (err=FSpOpenDF(spec,fsRdPerm,&refN))
		{
			FileSystemError(BINHEX_OPEN,spec->name,err);
			goto done;
		}
		if (err=SendFromOpenFile(stream,B64Encoder,refN,hfi.hFileInfo.ioFlLgLen)) goto done;
		MyFSClose(refN); refN = 0;
	}
	
	/*
	 * WOW!  All done!
	 */
	err = BufferSend(stream,B64Encoder,nil,0,False);

done:
	DontTranslate = False;
	BufferSendRelease(stream);
	if (refN) MyFSClose(refN);
	return(err);
}
	
/************************************************************************
 * SendDouble
 ************************************************************************/
OSErr SendDouble(TransStream stream,FSSpecPtr spec,long flags,short tableID, AttMapPtr amp)
{
	Str127 boundary;
	short err;
	CInfoPBRec hfi;
	
	if (err=FSpGetHFileInfo(spec,&hfi)) return(FileSystemError(BINHEX_OPEN,spec->name,err));
	if (!hfi.hFileInfo.ioFlLgLen) return(SendSingle(stream,spec,True,amp));
	
	/*
	 * build the internal boundary
	 */
	BuildBoundary(nil,boundary,"\pD");
	
	/*
	 * send the multipart header
	 */
	if (err = ComposeRTrans(stream,MIME_MP_FMT,
									 InterestHeadStrn+hContentType,
									 MIME_MULTIPART,
									 MIME_DOUBLE_SENDSUB,
									 AttributeStrn+aBoundary,
									 boundary,
									 NewLine))
		return(err);
	if (err=SendPString(stream,NewLine)) return(err);
	
	/*
	 * send the first boundary
	 */
	if (err=SendBoundary(stream)) return(err);
	
	/*
	 * send the resource part
	 */
	if (err=SendSingle(stream,spec,False,amp)) return(err);
	
	/*
	 * and a boundary
	 */
	if (err=SendBoundary(stream)) return(err);
	
	/*
	 * and the data fork
	 */
	
	if (err=SendDataFork(stream,spec,flags,tableID,amp)) return(err);
	
	/*
	 * and the terminal boundary
	 */
	PCat(boundary,"\p--");
	return(SendBoundary(stream));
}


/************************************************************************
 * SendDataFork - send the data fork of a file, in the proper format
 ************************************************************************/
OSErr SendDataFork(TransStream stream,FSSpecPtr spec,long flags,short tableID,AttMapPtr amp)
{
	short refN=0;
	short err=noErr;
	long fileSize;
	FInfo info;
	Str15 hexCreator,hexType;
	
	if (EqualStrRes(amp->mm.mimetype,MIME_TEXT))
		return(SendPlain(stream,spec,flags & ~FLAG_WRAP_OUT,tableID,amp));
	
	if (!err) err = ComposeRTrans(stream,MIME_CT_PFMT,
									 InterestHeadStrn+hContentType,
									 amp->mm.mimetype,
									 amp->mm.subtype,
									 AttributeStrn+aName,
									 ATT_MAP_NAME(amp),
									 NewLine);
	FSpGetFInfo(spec,&info);
	if (!err && !amp->suppressXMac && !TypeIsOnList(info.fdType,MACOSXSUCKS_TYPE_LIST)) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacType,Long2Hex(hexType,info.fdType),
									 NewLine);
	if (!err && !amp->suppressXMac && !TypeIsOnList(info.fdCreator,MACOSXSUCKS_CREATOR_LIST)) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacCreator,Long2Hex(hexCreator,info.fdCreator),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CD_FMT,
								 InterestHeadStrn+hContentDisposition,
								 ATTACHMENT,
								 AttributeStrn+aFilename,
								 ATT_MAP_NAME(amp),
								 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_V_FMT,
									 InterestHeadStrn+hContentEncoding,
									 amp->isText&&!amp->isPostScript ? MIME_QP : MIME_BASE64,
									 NewLine);
	if (err) goto done;
	
	/*
	 * separate the head from the bod
	 */
	if (err=SendPString(stream,NewLine)) goto done;
	DontTranslate = True;
	
	/*
	 * open it
	 */
	if (err = FSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	if (err = GetEOF(refN,&fileSize))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	
	err = SendFromOpenFile(stream,amp->isText&&!amp->isPostScript ? QPEncoder : B64Encoder,refN,fileSize);
	if (!err) BufferSend(stream,amp->isText&&!amp->isPostScript ? QPEncoder : B64Encoder,nil,0,False);

done:
	BufferSendRelease(stream);
	DontTranslate = False;
	if (refN) MyFSClose(refN);
	return(err);
}

/************************************************************************
 * SendFromOpenFile - send bytes from an open file
 ************************************************************************/
OSErr SendFromOpenFile(TransStream stream,DecoderFunc *encoder,short refN,long size)
{
	Ptr buffer = nil;
	long bSize;
	long count;
	short err=noErr;

	/*
	 * allocate buffer
	 */
	bSize = GetRLong(BUFFER_SIZE);
	bSize = MIN(bSize,size);
	if (bSize<256) bSize = 256;
	for (;bSize>255 && !(buffer=NuPtr(bSize));bSize/=2);
	if (!buffer) {WarnUser(MEM_ERR,err=MemError()); return(err);}

	while(size)
	{
		count = MIN(bSize,size);
		if (err=ARead(refN,&count,buffer))
		{
			FileSystemError(BINHEX_READ,"",err);
			break;
		}
		if (err=BufferSend(stream,encoder,buffer,count,False)) break;
		size -= count;
	}
	
	if (buffer) DisposePtr(buffer);
	
	return(err);
}

/************************************************************************
 * FindAttMap - figure the types of a file to send
 ************************************************************************/
OSErr FindAttMap(FSSpecPtr spec,AttMapPtr amp)
{
	FInfo info;
	short err;
	long flags;
	
	Zero(*amp);
	
	/*
	 * now, get the file info
	 */
	if (err=FSpGetFInfo(spec,&info)) return(err);
	
	FigureMIMEFromApple(info.fdCreator,info.fdType,spec->name,
											amp->mm.mimetype,amp->mm.subtype,
											amp->mm.suffix,&flags,&amp->mm.specialId);
	amp->isText = (flags & mmIsText)!=0;
	amp->isBasic = (flags & mmIsBasic)!=0;
	amp->suppressXMac = (flags & mmIgnoreXType)!=0;
	
	if (amp->isText && !ReallyIsText(spec))
		amp->isText = false;
	
	if (amp->isText &&
			EqualStrRes(amp->mm.mimetype,MIME_APPLICATION) &&
			EqualStrRes(amp->mm.subtype,MIME_OCTET_STREAM))
	{
		if (IsPostScript(spec))
		{
			GetRString(amp->mm.subtype,POSTSCRIPT);
			GetRString(amp->mm.suffix,PS_SUFFIX);
		}
		else
		{
			GetRString(amp->mm.mimetype,MIME_TEXT);
			GetRString(amp->mm.subtype,MIME_PLAIN);
		}
		amp->isBasic = True;
	}
	amp->isPostScript = EqualStrRes(amp->mm.subtype,POSTSCRIPT);


	Mac2OtherName(amp->shortName,spec->name);
	GetLongName(amp->longName,spec);
	
	if (*amp->mm.suffix && !EndsWith(amp->shortName,amp->mm.suffix))
		PSCat(amp->shortName,amp->mm.suffix);
	UUFileName(amp->uuName,amp->shortName);
	
	return(noErr);
}

/************************************************************************
 * GetLongName - get a long filename from an fsspec
 ************************************************************************/
PStr GetLongName(PStr longName,FSSpecPtr spec)
{
	// we force us-ascii here because we're too chicken to generate the *= stuff
	if (FSpGetLongName(spec,kTextEncodingUS_ASCII,longName) || StringSame(spec->name,longName))
		*longName = 0;
	
	return longName;
}

/************************************************************************
 * SendUU - send just the data fork, uuencoded
 ************************************************************************/
OSErr SendUU(TransStream stream, FSSpecPtr spec, AttMapPtr amp)
{
	short err;
	short refN;
	long fileSize;
	Str15 hexType, hexCreator;
	Str63 date;
	FInfo info;
	FSpGetFInfo(spec,&info);
	
	err = ComposeRTrans(stream,MIME_CT_PFMT,
									 InterestHeadStrn+hContentType,
									 amp->mm.mimetype,
									 amp->mm.subtype,
									 AttributeStrn+aName,
									 ATT_MAP_NAME(amp),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacType,Long2Hex(hexType,info.fdType),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacCreator,Long2Hex(hexCreator,info.fdCreator),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CD_FMT,
									 InterestHeadStrn+hContentDisposition,
									 ATTACHMENT,
									 AttributeStrn+aFilename,
									 ATT_MAP_NAME(amp),
									 NewLine);
	if (!err && *R822Date(date,AFSpGetMod(spec)-ZoneSecs())) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aModDate,date,NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_V_FMT,
									 InterestHeadStrn+hContentEncoding,
									 X_UUENCODE,
									 NewLine);
	
	/*
	 * separate the head from the bod
	 */
	if (!err) err = ComposeRTrans(stream,UUDECODE_FMT,NewLine,amp->uuName,NewLine);
	DontTranslate = True;
	
	/*
	 * open it
	 */
	if (err = FSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
	if (err = GetEOF(refN,&fileSize))
		{FileSystemError(BINHEX_OPEN,spec->name,err); goto done;}
		
  /*
	 * make sure encoder gets initialized
	 */
	BufferSend(stream,UUEncoder,"",0,amp->isText&&!amp->isPostScript);
	
	/*
	 * send file
	 */
	err = SendFromOpenFile(stream,UUEncoder,refN,fileSize);
	if (!err) err = BufferSend(stream,UUEncoder,nil,0,False);
	if (!err) err = SendPString(stream,NewLine);

done:
	BufferSendRelease(stream);
	DontTranslate = False;
	if (refN) MyFSClose(refN);
	return(err);
}

/************************************************************************
 * UUFileName - shorten a name to the 8.3 format DOS so loves
 ************************************************************************/
void UUFileName(PStr uuName,PStr inShortName)
{
	UPtr firstDot, lastDot;
	short len;
	Str63 shortName;
	
	PCopy(shortName,inShortName);
	ASSERT(*inShortName<=31);
	
	shortName[*shortName+1] = 0;
	
	firstDot = strchr(shortName+1,'.');
	lastDot = strrchr(shortName+1,'.');
	
	if (!firstDot) firstDot = shortName+*shortName+1;
	if (!lastDot) lastDot = firstDot;
	
	len = firstDot-shortName-1;
	len = MIN(len,8);
	
	*uuName = len;
	BMD(shortName+1,uuName+1,len);
	
	if (lastDot<shortName+*shortName)
	{
		len = *shortName - (lastDot-shortName);
		len = MIN(len,3);
		
		PCatC(uuName,'.');
		BMD(lastDot+1,uuName+*uuName+1,len);
		*uuName += len;
	}
}

/************************************************************************
 * ReallyIsText - is a file really a text file?
 ************************************************************************/
Boolean ReallyIsText(FSSpecPtr spec)
{
	Handle taste=nil;
	long controls = 0;
	long size;
	UPtr spot,end;
	
	Snarf(spec,&taste,GetRLong(TEXT_QP_TASTE));
	if (!taste) return(true);// cross your fingers...
	if (!(size=GetHandleSize(taste))) return(true);
	end = *taste + size;
	for (spot = *taste; spot<end; spot++)
		if (*spot<' ' && *spot!='\009' && *spot!='\015' && *spot!='\014') controls++;
	ZapHandle(taste);
	return ((controls*1000)/size < GetRLong(TOLERABLE_CTLCHARS_PPT));
}