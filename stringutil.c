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

#include <Resources.h>
#include <string.h>

#include <conf.h>
#include <mydefs.h>

#define FILE_NUM 73
/* Copyright (c) 1995 by QUALCOMM Incorporated */

#include "StringUtil.h"
#pragma segment StringUtil

PStr FormatString(unsigned int arg, PStr string, short format, short digits);
Boolean PPtrMatchLWSPSpot(PStr look,Ptr text,uLong textLen,UPtr *matchEnd);

/************************************************************************
 * AllDigits - is a string made up only of digits?
 ************************************************************************/
Boolean AllDigits(UPtr s, long len)
{
	while (len && '0'<=*s && *s<='9') {s++;len--;}
	return(len==0);
}

/************************************************************************
 * HighBits - count high bits in a string
 ************************************************************************/
long HighBits(UPtr s,long len)
{
	long count = 0;
	while (len-->0) if (*s++>127) count++;
	return count;
}

/**********************************************************************
 * BeginsWith - does one string begin with another?
 **********************************************************************/
Boolean BeginsWith(PStr string,PStr prefix)
{
	uShort size;
	Boolean result;
	
	if (*string<*prefix) return(False);
	size = *string;
	*string = *prefix;
	result = StringSame(string,prefix);
	*string = size;
	return(result);
}

/**********************************************************************
 * CaptureHex - read hex strings
 **********************************************************************/
void CaptureHex(PStr from,PStr to)
{
	Str255 scratch;
	long	len;

	len = *from;
	CaptureHexPtr(from+1,scratch+1,&len);
	*scratch = len;
	PCopy(to,scratch);
}

/**********************************************************************
 * CaptureHexPtr - read hex strings
 **********************************************************************/
void CaptureHexPtr(Ptr from,Ptr to,long *pLen)
{
	UPtr spot,end;
	UPtr toSpot,toEnd;
		
	spot = from;
	end = from+*pLen;
	
	toSpot = to;
	toEnd = to+*pLen;
	
	for (;spot<end && toSpot<toEnd;spot++)
	{
		if (*spot==lowerDelta)
		{
			Hex2Bytes(spot+1,2,toSpot);
			toSpot++;
			spot += 2;
		}
		else
			*toSpot++ = *spot;
	}
	*pLen = toSpot - (UPtr)to;
}

/************************************************************************
 * ComposeString - sprintf, only smaller
 * %s - c string
 * %d - int
 * %c - char (int)
 * %p - pascal string
 * %q - internet-style quoted string
 * %i - internet address
 * %I - internet address, turned into hostname
 * %r - string from a resource
 * %O - OSType, including ''s
 * %o - OSType, no ''s
 * %#	- integer argument, prints "s" if not 1
 * %$ - integer argument, prints "es" if not 1
 * %& - integer argument, prints "'s" if not 1
 * %a - AEPrint
 ************************************************************************/
UPtr ComposeString(UPtr into,UPtr format,...)
{
	va_list args;
	va_start(args,format);
	(void) VaComposeString(into,format,args);
	va_end(args);
	return(into);
}
UPtr ComposeRString(UPtr into,short format,...)
{
	va_list args;
	va_start(args,format);
	(void) VaComposeRString(into,format,args);
	va_end(args);
	return(into);
}
OSErr ComposeRTrans(TransStream stream,short format,...)
{
	Str255 into;
	va_list args;
	va_start(args,format);
	(void) VaComposeRString(into,format,args);
	va_end(args);
	return(SendPString(stream,into));
}
OSErr AccuComposeR(AccuPtr a,short format,...)
{
	Str255 into;
	va_list args;
	va_start(args,format);
	(void) VaComposeRString(into,format,args);
	va_end(args);
	return(AccuAddStr(a,into));
}
OSErr AccuCompose(AccuPtr a,PStr format,...)
{
	Str255 into;
	va_list args;
	va_start(args,format);
	(void) VaComposeString(into,format,args);
	va_end(args);
	return(AccuAddStr(a,into));
}
UPtr VaComposeRString(UPtr into,short format,va_list args)
{
	Str255 stringFormat;
	
	GetRString(stringFormat,format);
	return (VaComposeString(into,stringFormat,args));
}

#define MAX_SUBS	5
UPtr VaComposeStringDouble(UPtr into,int maxInto,UPtr format,va_list args,UPtr into2,int maxInto2,UPtr format2)
{
	UPtr formatP;
	Str255 argString;
	long n;
	Boolean suppress;
	Str255 buffers[MAX_SUBS];
	long arg;
	short which;
	long resId;

top:
	which=0;
	for (n=0;n<MAX_SUBS;n++) buffers[n][0] = 0;
	
	*into = 0;
	for (formatP=format+1;formatP<format+*format+1;formatP++)
		if (*formatP==lowerOmega && which<MAX_SUBS)
		{
			formatP++;
			if (*formatP==lowerOmega) PMaxCatC(into,maxInto,*formatP);
			else
			{
				arg = va_arg(args,unsigned int);
				FormatString(arg,buffers[which++],*formatP,0);
			}
		}
		else if (*formatP!='%')
			PMaxCatC(into,maxInto,*formatP);
		else
		{
			formatP++;
			if (suppress = *formatP=='�') formatP++;
			if (*formatP=='%')
				PMaxCatC(into,maxInto,'%');
			else
			{
				if (*formatP=='^' && '0'<=formatP[1] && formatP[1]<='9')
				{
					PSCopy(argString,buffers[formatP[1]-'0']);
					*formatP++;
				}
				else if (*formatP=='R')
				{
					resId = 0;
					while (formatP<format+*format && isdigit(formatP[1]))
					{
						resId *= 10;
						resId += formatP[1]-'0';
						formatP++;
					}
					GetRString(argString,resId);
				}
				else
				{
					short digits=0;
					if (isdigit(*formatP)) {digits = *formatP-'0'; formatP++;}
					arg = va_arg(args,unsigned int);
					if (suppress) *argString = 0;
					else FormatString(arg,argString,*formatP,digits);
				}
				if (maxInto<=0)
					PCat(into,argString);
				else
					PSCat_C(into,argString,maxInto);
			}
		}

	into[*into+1] = 0;
	
	// ugly hack here...
	if (into2)
	{
		into = into2;
		format = format2;
		maxInto = maxInto2;
		into2 = format2 = nil;
		maxInto2 = 0;
		goto top;
	}
	
	return(into);
}

/************************************************************************
 * EndsWith - does one string end with another?
 ************************************************************************/
Boolean EndsWith(PStr name,PStr suffix)
{
	Boolean res;
	Byte c;
	UPtr spot;
	if (*name<*suffix) return(False);			/* too short */
	
	spot = name + *name - *suffix ;				/* before start of putative suffix */
	c = *spot;														/* save byte */
	*spot = *suffix;											/* pretend equal length */
	res = StringSame(suffix,spot);
	*spot = c;														/* restore byte */
	return(res);
}

/************************************************************************
 * HandleEndsWithR - does a handle end with a string from a resource?
 ************************************************************************/
Boolean HandleEndsWithR(Handle name,short index)
{
	Str255 string;
	
	PCopy(string,*name);
	return(EndsWithR(string,index));
}

/************************************************************************
 * EndsWithR - does a string end with a suffix in a resource?
 ************************************************************************/
Boolean EndsWithR(PStr name,short resId)
{
	Str255 suffix;
	
	GetRString(suffix,resId);
	return(EndsWith(name,suffix));

}

/************************************************************************
 * StartsWithR - does a string start with a prefix in a resource?
 ************************************************************************/
Boolean StartsWithR(PStr name,short resId)
{
	Str255 prefix;
	
	GetRString(prefix,resId);
	return(StartsWith(name,prefix));

}

/************************************************************************
 * StartsWith - does a string start with a prefix?
 ************************************************************************/
Boolean StartsWith(PStr name,PStr prefix)
{
	name[*name+1] = 0;
	prefix[*prefix+1] = 0;
	return *name>=*prefix && !striscmp(name+1,prefix+1);
}

/************************************************************************
 * StartsWithPtr - does a string start with a prefix?
 ************************************************************************/
Boolean StartsWithPtr(UPtr name,uLong len,PStr prefix)
{
	return len>=*prefix && !strincmp(name,prefix+1,*prefix);
}

/************************************************************************
 * EqualStrRes - is a string the same as a resource?
 ************************************************************************/
Boolean EqualStrRes(PStr string, short resId)
{
	Str255 s;
	return(StringSame(GetRString(s,resId),string));
}

/************************************************************************
 * EscapeChars - escape characters in a string
 ************************************************************************/
PStr EscapeChars(PStr string, PStr toEscape)
{
	Str255 scratch;
	UPtr to = scratch+1;
	UPtr from = string+1;
	UPtr end = string+*string+1;
	Boolean escaped = False;
	
	while (from<end)
	{
		if (escaped)
		{
			*to++ = *from;
			escaped = False;
		}
		
		if (*from=='\\')
		{
			*to++ = '\\';
			escaped = True;
		}
		else
		{
			if (PIndex(toEscape,*from)) *to++ = '\\';
			*to++ = *from;
		}
		
		from++;
	}
	
	*scratch = to-scratch-1;
	
	/*
	 * did we change anything?
	 */
	if (*scratch != *string) PCopy(string,scratch);
	
	return(string);
}	

/**********************************************************************
 * EscapeInHex - write hex strings safely
 **********************************************************************/
void EscapeInHex(PStr from,PStr to)
{
	UPtr spot,end;
	Str255 scratch;
	UPtr toSpot,toEnd;
	
	
	spot = from+1;
	end = spot+*from;
	
	toSpot = scratch+1;
	toEnd = toSpot+250;
	
	for (;spot<end && toSpot<toEnd;spot++)
	{
		if ((*spot>' ' && *spot!=lowerDelta) || (*spot==' ' && spot!=(end-1)))
			*toSpot++ = *spot;
		else
		{
			*toSpot++ = lowerDelta;
			Bytes2Hex(spot,1,toSpot);
			toSpot += 2;
		}
	}
	
	*scratch = toSpot - scratch - 1;
	PCopy(to,scratch);
}

/**********************************************************************
 * Transmogrify - change one string into another, using two STR# for xlation
 **********************************************************************/
PStr Transmogrify(PStr toStr,short toId,PStr fromStr,short fromId)
{
	short index;
	
	if (index=FindSTRNIndex(fromId,fromStr))
		GetRString(toStr,toId+index);
	else if (toStr!=fromStr)
		PCopy(toStr,fromStr);
	return(toStr);
}


/************************************************************************
 * FixNewlines - remove cr, and turn nl into cr
 ************************************************************************/
void FixNewlines(UPtr string,long *count)
{
	unsigned char *from, *to;
	long n;
	
	for (to=from=string,n= *count;n;n--,from++)
		if (*from=='\012') *to++ = '\015';
		else if (*from!='\015') *to++ = *from;
	*count = to-string;
}

/**********************************************************************
 * 
 **********************************************************************/
PStr FormatString(unsigned int arg, PStr string, short format,short digits)
{
	short n;
	struct hostInfo hi;
	
	*string = 0;
	
	switch (format)
	{
		case 'c':
			string[0] = 1;
			string[1] = arg;
			break;
		case 's':
			*string = strlen((UPtr)arg);
			*string = MIN(*string,253);
			BMD((UPtr)arg,string+1,*string);
			break;
		case 'p':
			PCopy(string,(PStr)arg);
			break;
		case 'e':
			PCopy(string,(PStr)arg);
			EscapeInHex(string,string);
			break;
		case 'i':
			NumToDot(arg,string);
			break;
		case 'I':
			if (!GetHostByAddr(&hi,arg))
			{
				*string = strlen(hi.cname);
				BMD(hi.cname,string+1,*string);
			}
			else
			{
				NumToDot(arg, string+1);
				string[0] = string[1]+2;
				string[1] = '[';
				string[string[0]] = ']';
			}
			break;
		case 'd':
			NumToString(arg,string);
			break;
		case 'K':
			if (arg<1 K)
				NumToString(arg,string);
			else if (arg < 10 K)
			{
				arg *= 10;
				arg /= 1 K;
				if (arg%10) ComposeString(string,"\p%d.%dK",arg/10,arg%10);
				else ComposeString(string,"\p%dK",arg/10);
			}
			else if (arg < 1 K K)
			{
				arg /= 1 K;
				NumToString(arg,string);
				PCatC(string,'K');
			}
			else if (arg < 10 K K)
			{
				arg *= 10;
				arg /= 1 K K;
				if (arg%10) ComposeString(string,"\p%d.%dM",arg/10,arg%10);
				else ComposeString(string,"\p%dM",arg/10);
			}
			else
			{
				arg /= 1 K K;
				NumToString(arg,string);
				PCatC(string,'M');
			}
			break;
		case 'q':
			Quote822(string,(UPtr)arg,True);
			break;
		case 'r':
			GetRString(string,arg);
			break;
		case 'b':
			n = *string = 32;
			for (string++;n;string++)
			{
				*string = arg&(1<<31) ? '1' : '0';
				arg <<= 1;
				n--;
			}
			break;
		case 'x':
			Long2Hex(string,arg);
			if (digits)
			{
				while (*string<digits) PInsertC(string,256,'0',string+1);
				if (*string>digits)
				{
					BMD(string+1+(*string-digits),string+1,digits);
					*string = digits;
				}
			}
			break;
		case '#':
			GetRString(string,PluralStrn+(arg==1?1:2));
			break;
		case '$':
			GetRString(string,PluralStrn+(arg==1?3:4));
			break;
		case '&':
			GetRString(string,PluralStrn+(arg==1?5:6));
			break;
		case '*':
			GetRString(string,PluralStrn+(arg==1?7:8));
			break;
		case 'O':
			*string = 6;
			string[1] = string[6] = '\'';
			string[2] = ((Uptr)&arg)[0];
			string[3] = ((Uptr)&arg)[1];
			string[4] = ((Uptr)&arg)[2];
			string[5] = ((Uptr)&arg)[3];
			break;
		case 'o':
			*string = 4;
			string[1] = ((Uptr)&arg)[0];
			string[2] = ((Uptr)&arg)[1];
			string[3] = ((Uptr)&arg)[2];
			string[4] = ((Uptr)&arg)[3];
			break;
		case 'B':
			if (arg) PCopy(string,"\pTRUE");
			else PCopy(string,"\pFALSE");
			break;
	}
	
	return(string);
}

/************************************************************************
 * LCD - find the least common denom of two strings
 ************************************************************************/
PStr LCD(PStr s1,PStr s2)
{
	UPtr p1,p2,end;
	char c1,c2;
	
	end = s1+MIN(*s1,*s2)+1;
	for (p1=s1+1,p2=s2+1;p1<end;p1++,p2++)
		if (*p1!=*p2)
		{
			c1 = *p1;
			c2 = *p2;
			if (isupper(c1)) c1=tolower(c1);
			if (isupper(c2)) c2=tolower(c2);
			if (c1!=c2) break;
		}
	*s1 = p1-s1-1;
	s1[*s1+1] = 0;
	return(s1);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 **********************************************************************/
UPtr PCat(PStr string,PStr suffix)
{
	short sufLen;
	
	sufLen = MIN(255-*string,*suffix);
	
	BMD(suffix+1,string+*string+1,sufLen);
	*string += sufLen;
	
	return(string);
}

/**********************************************************************
 * PCatR - concatenate a string from a resource to the end of a string
 **********************************************************************/
UPtr PCatR(PStr string,short resId)
{
	Str255 suffix;
	
	GetRString(suffix,resId);
	return(PCat(string,suffix));
}

/************************************************************************
 * PCopyTrim - copy and trim a string
 ************************************************************************/
PStr PCopyTrim(PStr toString,PStr fromString,short max)
{
	Str255 tString;
	PCopy(tString,fromString);
	TrimWhite(tString);
	TrimInitialWhite(tString);
	*tString = MIN(*tString,max-1);
	PCopy(toString,tString);
	return(toString);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 * escape certain chars in the string
 **********************************************************************/
UPtr PEscCat(UPtr string, UPtr suffix, short escape, char *escapeWhat)
{
	short sufLen;
	unsigned char *suffSpot,*stringSpot;
	
	sufLen = *suffix;
	stringSpot = string+*string+1;
	
	for (suffSpot=suffix+1;sufLen--;suffSpot++)
	{
		if (*suffSpot==escape || strchr(escapeWhat,*suffSpot))
			*stringSpot++ = escape;
		*stringSpot++ = *suffSpot;
	}
	*string = stringSpot-string-1;
	
	return(string);
}

/************************************************************************
 * PIndex - find a char in a pascal string
 ************************************************************************/
UPtr PIndex(PStr string, char c)
{
	Ptr spot;
	
	for (spot=string+1;spot<string+*string+1;spot++) if (*spot==c) return(spot);
	return(nil);
}

/************************************************************************
 * IndexPtr - find a char in a string specified by pointer and length
 ************************************************************************/
UPtr IndexPtr(UPtr string,long stringLen, char c)
{
	Ptr spot;
	
	for (spot=string;spot<string+stringLen;spot++) if (*spot==c) return(spot);
	return(nil);
}

/************************************************************************
 * PRIndex - find a char in a pascal string, backwards
 ************************************************************************/
UPtr PRIndex(PStr string, char c)
{
	Ptr spot;
	
	for (spot=string+*string;spot>string;spot--) if (*spot==c) return(spot);
	return(nil);
}

/**********************************************************************
 * PInsert - insert some text
 **********************************************************************/
PStr PInsert(PStr string,short size,PStr insert,UPtr spot)
{
	short toInsert = MIN(*insert,size-*string-1);
	
	if (toInsert>0)
	{
		BMD(spot,spot+toInsert,*string-(spot-string-1));
		BMD(insert+1,spot,toInsert);
		*string += toInsert;
	}
	return(string);
}

/**********************************************************************
 * PInsertC - insert a single character
 **********************************************************************/
PStr PInsertC(PStr string,short size,Byte c,UPtr spot)
{
	Str15 s;
	
	*s = 1;
	s[1] = c;
	
	return(PInsert(string,size,s,spot));
}

/************************************************************************
 * PLCat - concat a long onto a string (preceed it with a space)
 ************************************************************************/
UPtr PLCat(UPtr string,long num)
{
	short n;			/* length of old string + 1 */
	n = *string+1;
	NumToString(num,string+n);
	string[0] += string[n]+1;
	string[n] = ' ';
	return(string);
}

/************************************************************************
 * PXCat - concat a long onto a string in hex
 ************************************************************************/
UPtr PXCat(UPtr string,long num)
{
	Str31 x;
	
	Bytes2Hex((void*)&num,sizeof(long),x+1);
	*x = 8;
	return(PCat(string,x));
}

/************************************************************************
 * PXWCat - concat a short onto a string in hex
 ************************************************************************/
UPtr PXWCat(UPtr string,short num)
{
	Str31 x;
	
	Bytes2Hex((void*)&num,sizeof(short),x+1);
	*x = 4;
	return(PCat(string,x));
}

/************************************************************************
 * Tr - translate text in a handle
 ************************************************************************/
Boolean Tr(Handle text,Uptr fromS, Uptr toS)
{
	long len = GetHandleSize(text);
	return(TrLo(*text,len,fromS,toS));	//no handle lock; keep in segment with TrLo
}

/************************************************************************
 * TrLo - translate text in a pointer
 ************************************************************************/
Boolean TrLo(UPtr text,long len,Uptr fromS, Uptr toS)
{
	UPtr end,spot;
	Boolean did=False;
	short fromChar,toChar;
	
	end = text+len;
	for (;*fromS;fromS++,toS++)
	{
		fromChar = *fromS;
		toChar = *toS;
		for (spot=text;spot<end;spot++)
			if (*spot==fromChar)
			{
				did = True;
				*spot = toChar;
			}
	}
	return(did);
}

/************************************************************************
 * PPtrFindSub - is a pascal string a substring of a string
 ************************************************************************/
UPtr PPtrFindSub(PStr sub, UPtr string, long len)
{
	UPtr end = string+len-*sub+1;
	UPtr stringSpot,subSpot, subEnd;
	Byte c1, c2;
	
	subEnd = sub+*sub+1;
	while (string<end)
	{
		for (subSpot=sub+1,stringSpot=string;subSpot<subEnd;subSpot++,stringSpot++)
		{
			if (*stringSpot!=*subSpot)
			{
				c1 = *stringSpot;
				c2 = *subSpot;
				if (isupper(c1)) c1=tolower(c1);
				if (isupper(c2)) c2=tolower(c2);
				if (c1!=c2) break;
			}
		}
		if (subSpot>=subEnd) return(stringSpot-*sub);
		string++;
	}
	return(nil);
}

/************************************************************************
 * PReplace - replace one string with another
 ************************************************************************/
PStr PReplace(PStr string,PStr find,PStr replace)
{
	UPtr spot;
	
	if (*find && !EqualString(find,replace,true,true))
		while (spot=PFindSub(find,string))
		{
			if (*string+*replace-*find>255) break;
			BMD(spot+*find,spot+*replace,*string-(spot-string-1)-*find);
			BMD(replace+1,spot,*replace);
			*string += *replace-*find;
		}
	
	return(string);
}

/************************************************************************
 * PSCat_C - C routine to concat a string, worrying about length
 ************************************************************************/
UPtr PSCat_C(PStr string,PStr suffix,short max)
{
	short tot = MIN(max-1,*string+*suffix);
	short add = tot - *string;
	if (add>0)
	{
		BMD(suffix+1,string+*string+1,add);
		*string += add;
	}
	return(string);
}

/**********************************************************************
 * copy a pascal string into a c string
 **********************************************************************/
UPtr PtoCcpy(UPtr cStr, UPtr pStr)
{
	BMD(pStr+1,cStr,*pStr);
	cStr[*pStr] = 0;
	return(cStr);
}

/**********************************************************************
 * PStrCopy - copy a pascal string
 **********************************************************************/
PStr PStrCopy(PStr to,PStr from,short max)
{
	long	len = MIN(max,*from+1);	//	length includes length byte
	BlockMoveData(from,to,len);
	*to = len-1;
	return to;
}

/**********************************************************************
 * InfiniteString - set string to all 0xFFs
 **********************************************************************/
PStr InfiniteString(PStr s,short size)
{
	short	i;
	
	*s = size-1;
	for(i=1;i<=size;i++)
		s[i] = 0xFF;
	return s;
}

/************************************************************************
 * ItemFromResAppearsInStr - does a string contain an item from a list of
 *  items in a resource
 ************************************************************************/
Boolean ItemFromResAppearsInStr(short resID,PStr string,UPtr delims)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	//default delimitter is comma
	if (!delims) delims = ",";
	
	GetRString(s,resID);
	spot = s+1;
	
	while (PToken(s,token,&spot,delims))
		if (PFindSub(token,string)) return true;
	
	return false;
}

/************************************************************************
 * StrIsItemFromRes - is a string one of the items in a resource?
 ************************************************************************/
Boolean StrIsItemFromRes(PStr string,short resID,UPtr delims)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	//default delimitter is comma
	if (!delims) delims = ",";
	
	GetRString(s,resID);
	spot = s+1;
	
	while (PToken(s,token,&spot,delims))
		if (StringSame(token,string)) return true;
	
	return false;
}

/************************************************************************
 * PToken - grab a token out of a string
 *  Returns pointer to token argument
 *  Saves state in spotP
 ************************************************************************/
PStr PToken(PStr string,PStr token,UPtr *spotP,UPtr delims)
{
	UPtr spot;
	UPtr end = string+*string+1;
	UPtr tSpot = token+1;
	
	*token = 0;
	if (*spotP>=end) return(nil);
	for (spot = *spotP; spot<end; spot++)
		if (!strchr(delims,*spot)) *tSpot++ = *spot;
		else break;
	*spotP = spot+1;
	*token = tSpot-token-1;
	return(token);
}

/************************************************************************
 * TokenPtr - grab a token out of text
 *  Returns boolean indicating if token found
 *  Saves state in spotP
 ************************************************************************/
Boolean TokenPtr(Ptr string,long stringLen,Ptr *token,long *tokenLen,UPtr *spotP,UPtr delims)
{
	UPtr spot;
	UPtr end = string+stringLen;
	long	len = 0;
	
	*token = *spotP;
	if (*spotP>=end) return(false);
	for (spot = *spotP; spot<end; spot++)
		if (!strchr(delims,*spot)) len++;
		else break;
	*spotP = spot+1;
	*tokenLen = len;
	return(true);
}

/************************************************************************
 * TokenPtr - grab a token out of text
 *  Returns boolean indicating if token found
 *  Saves state in spotP
 *	Returns token in p-string
 ************************************************************************/
Boolean PTokenPtr(Ptr string,long stringLen,Ptr token,UPtr *spotP,UPtr delims)
{
	UPtr	tokenPtr;
	long	tokenLen;
	Boolean	result;
	
	if (result = TokenPtr(string,stringLen,&tokenPtr,&tokenLen,spotP,delims))
		MakePPtr(token,tokenPtr,tokenLen);
	return result;
}

/**********************************************************************
 * ReMatch - does a string have a reply intro?
 **********************************************************************/
Boolean ReMatch(PStr string, PStr re)
{
	UPtr colon;
	UPtr reSpot;
	Str255 remainder;
	
	if (*re)
	{																					/* intro string not empty */
		if (colon=PIndex(string,re[*re]))
		{																				/* final char appears */
			if (colon-string >= *re)
			{																			/* it's long enough */
				reSpot = re+*re;
				do
				{
					reSpot--;
					colon--;
					while (reSpot>re && !IsWordChar[*reSpot]) reSpot--;
					while (colon>string && !IsWordChar[*colon]) colon--;
					if (colon>string && reSpot>re)
						if ((*colon & 0x1f)!=(*reSpot&0x1f)) break;
				}
				while (colon>string && reSpot>re);
				
				if (reSpot!=re) return false;
				if (colon==string) return true;
				
				//ok, we didn't use up all of the subject.  See if it's a lyris-type thing
				MakePStr(remainder,string+1,colon-string+1);
				TrimAllWhite(remainder);
				TrimSquares(remainder,true,true);
				return(*remainder==0);
			}
		}
	}
	return(False);
}

/************************************************************************
 * TrimSquares - trim square-bracketed stuff from start of string
 ************************************************************************/
Boolean TrimSquares(PStr s,Boolean multiple,Boolean internal)
{
	Str31 left, right;
	UPtr spot;
	Boolean result = false;
	
	GetRString(left,SQUARE_LEFT);
	GetRString(right,SQUARE_RIGHT);
	TrimInitialWhite(s);
	
	if (!internal)
	{
		while (*s>2)
		{
			if (spot=PIndex(left,s[1]))	// left delimiter
			{
				if (spot=PIndex(s,right[spot-left])) // found both delimiters!
				{
					BMD(spot+1,s+1,*s - (spot-s));
					*s -= spot-s;
					TrimInitialWhite(s);
					result = true;
				}
				else break;	// didn't find it
			}
			else break;	// didn't find it
			if (!multiple) break;
		}
	}
	else
	{
		short brk;
		UPtr lPtr;
		UPtr rPtr;
		
		for (brk=1;*s>2 && brk<=*left;brk++)
		{
			if (lPtr=PIndex(s,left[brk]))
			if (rPtr=PIndex(s,right[brk]))
			if (lPtr<rPtr)
			{
				if (rPtr<s+*s) BMD(rPtr+1,lPtr,*s-(lPtr-s-1 + rPtr-lPtr+1));
				*s -= rPtr-lPtr+1;
				result = true;
				if (multiple) brk--;	// try again with this one
				else break;
			}
		}
	}
	
	return result;
}

/************************************************************************
 * RemoveParens - remove parenthesized information
 ************************************************************************/
void RemoveParens(UPtr string)
{
	UPtr to, from, end;
	short pLevel=0;
	
	for (to=from=string+1,end=string+*string; from<=end; from++)
		switch(*from)
		{
			case '(':
				pLevel++;
				break;
			case ')':
				if (pLevel) pLevel--;
				else *to++ = *from;
				break;
			case ' ':
				if (!pLevel) break;
				/* fall through is deliberate */
			default:
				*to++ = *from;
				break;
		}
	*string = to-string-1;
}

/************************************************************************
 * PStripChar - remove all occurrences of a char from a string
 ************************************************************************/
PStr PStripChar(PStr string,Byte c)
{
	*string = StripChar(string+1,*string,c);
	return(string);
}

/************************************************************************
 * StripChar - remove all occurrences of a char from text, return new length
 ************************************************************************/
long StripChar(Ptr string,long len,Byte c)
{
	UPtr from, to, end;
	
	end = string+len;
	from = string-1;
	to = string;
	
	while (++from<end) if (*from != c) *to++ = *from;
	return to-(UPtr)string;
}

/************************************************************************
 * strincmp - compare two strings, don't care about case
 ************************************************************************/
int strincmp(UPtr s1,UPtr s2,short n)
{
	register c1, c2;
	for (c1= *s1, c2= *s2; n--; c1 = *++s1, c2= *++s2)
	{
		if (c1-c2)
		{
			if (isupper(c1)) c1=tolower(c1);
			if (isupper(c2)) c2=tolower(c2);
			if (c1-c2) return (c1-c2);
		}
	}
	return(0);
}

/**********************************************************************
 * striscmp - compare two strings, up to the length of the shorter string,
 * and ignoring case
 **********************************************************************/
int striscmp(UPtr s1,UPtr s2)
{
	register c1, c2;
	for (c1= *s1, c2= *s2; c1 && c2; c1 = *++s1, c2= *++s2)
	{
		if (c1-c2)
		{
			if (isupper(c1)) c1=tolower(c1);
			if (isupper(c2)) c2=tolower(c2);
			if (c1-c2) return (c1-c2);
		}
	}
	return(0);
}

/**********************************************************************
 * strscmp - compare two strings, up to the length of the shorter string,
 * paying attention to case
 **********************************************************************/
int strscmp(UPtr s1,UPtr s2)
{
	register c1, c2;
	for (c1= *s1, c2= *s2; c1 && c2; c1 = *++s1, c2= *++s2)
		if (c1-c2) return (c1-c2);
	return(0);
}

/************************************************************************
 * Tokenize - set pointers to the beginning and end of a delimited token
 ************************************************************************/
UPtr Tokenize(UPtr string, int size, UPtr *start, UPtr *end, UPtr delims)
{
	UPtr stop = string+size;
	char safe = *stop;
	UPtr last;
	
	*stop = 0;
	while (strchr(delims,*string)) string++;
	*stop = *delims;
	for (last=string; !strchr(delims,*last); last++);
	*stop = safe;
	if (string==stop) return(nil);
	*start = string;
	*end = stop;
	return(string);
}

/**********************************************************************
 * TrimInitialWhite - remove whitespace characters from the beginning of a string
 **********************************************************************/
PStr TrimInitialWhite(PStr s)
{
	UPtr cp=s+1;
	short len;
	
	for (cp=s+1;cp<=s+*s && IsSpace(*cp);cp++);
	if (cp>s+1 && cp<=s+*s)
	{
		len = *s - (cp-(s+1));
		BMD(cp,s+1,len);
		*s = len;
	}
	return(s);
}

/**********************************************************************
 * TrimInternalWhite - collapse internal whitespace into single
 **********************************************************************/
PStr TrimInternalWhite(PStr s)
{
	Boolean wasWhite = false;
	Boolean isWhite;
	char *spot;
	char *end = s+*s;
	char *copySpot = s+1;
	
	for (spot=s+1;spot<=end;spot++)
	{
		isWhite = IsSpace(*spot);
		if (isWhite && wasWhite) continue;	// if both white, skip
		*copySpot++ = *spot;	// otherwise, copy the char
		wasWhite = isWhite;	// remember if we were looking at whitespace
	}
	*s = copySpot-(char *)s-1;
	
	return(s);
}

/************************************************************************
 * TrimPrefix - strip a prefix from a string
 ************************************************************************/
Boolean TrimPrefix(UPtr string, UPtr prefix)
{
	short oldLen = *string;

	if (oldLen < *prefix) return(False);
	
	*string = *prefix;
	if (StringSame(string,prefix))
	{
		BMD(string+1+*prefix,string+1,oldLen-*prefix);
		*string = oldLen - *prefix;
		return(True);
	}
	else
	{
		*string = oldLen;
		return(False);
	}
}

/**********************************************************************
 * StringSame - are two strings the same?
 **********************************************************************/
Boolean StringSame(PStr s1,PStr s2)
{
	if (*s1!=*s2) return false;	// quick test
	
	if (FurrinSort)
		return(!IdenticalString(s1,s2,nil));
	else
		return(EqualString(s1,s2,False,True));
}

/**********************************************************************
 * StringComp - return whether s1<s2 (negative), s1==s2 (0), s1>s2 (positive)
 **********************************************************************/
long StringComp(PStr s1,PStr s2)
{
	if (FurrinSort)
		return(CompareString(s1,s2,nil));
	else
		return(RelString(s1,s2,False,True));
}

/**********************************************************************
 * MyUpperText - uppercase text with or without fancy furrin stuff
 **********************************************************************/
void MyUpperText(UPtr buffer,long bufferSize)
{
	if (FurrinSort)
		UppercaseText(buffer,bufferSize,smSystemScript);
	else
	{
		short i;
		for (i=0;i<bufferSize;i++) if (islower(buffer[i])) buffer[i] = toupper(buffer[i]);
	}
}

/**********************************************************************
 * MyLowerText - lowercase text with or without fancy furrin stuff
 **********************************************************************/
void MyLowerText(UPtr buffer,long bufferSize)
{
	if (FurrinSort)
		LowercaseText(buffer,bufferSize,smSystemScript);
	else
	{
		short i;
		for (i=0;i<bufferSize;i++) if (isupper(buffer[i])) buffer[i] = tolower(buffer[i]);
	}
}

/**********************************************************************
 * MyLowercaseText - call lowercasetext carefully
 **********************************************************************/
OSErr MyLowercaseText(UPtr text,long len)
{
	OSErr err;
	
	// Try the easy way
	LowercaseText(text,len,smSystemScript);
	
	// Did it work?
	if (err = ResError())
	{
		long saveScriptSort;
		
		// set the port and set to system font, just in case
		PushGWorld();
		SetPort(InsurancePort);
		TextFont(0);
		
		// point the system at our resource
		saveScriptSort = GetScriptVariable(smSystemScript, smScriptSort);
		SetScriptVariable(smSystemScript, smScriptSort, kMyIntl0);
		ClearIntlResourceCache();
		
		// call lowertext
		LowercaseText(text,len,smSystemScript);
		err = ResError();
		
		// put stuff back
		SetScriptVariable(smSystemScript, smScriptSort, saveScriptSort);
		ClearIntlResourceCache();
		PopGWorld();
	}
	
	return(err);
}

/**********************************************************************
 * TrimReLo - remove an Re: string
 **********************************************************************/
Boolean TrimReLo(PStr string, PStr re)
{
	UPtr colon;
	if (ReMatch(string,re))
	{
		colon = PIndex(string,re[*re]);
		while (IsWhite(colon[1]) && colon<string+*string-1) colon++;
		BMD(colon+1,string+1,*string-(colon-string));
		*string -= colon-string;
		return(True);
	}
	return(False);
}

/************************************************************************
 * TrimRe - trim Re: and Fwd: from a string
 ************************************************************************/
Boolean TrimRe(PStr string, Boolean squares)
{
	Boolean did = False;
	
	while (TrimReLo(string,Re) || TrimReLo(string,Fwd) || TrimReLo(string,OFwd) || squares && TrimSquares(string,false,false)) did = True;
	
	return(did);
}

/**********************************************************************
 * TrimWhite - remove whitespace characters from the end of a string
 **********************************************************************/
PStr TrimWhite(PStr s)
{
	register int len=*s;
	register UPtr cp=s+len;
	
	while (len && IsSpace(*cp)) cp--,len--;
	
	*s = len;
	return(s);
}

/**********************************************************************
 * CollapseLWSP - convert lwsp runs to single spaces
 **********************************************************************/
PStr CollapseLWSP(PStr s)
{
	UPtr to,from,end;
	Boolean space = true;	// beginning of string counts as space
	Boolean lwsp;
	
	end = s+*s+1;
	for (to=from=s+1;from<end;from++)
	{
		// is current char space?
		lwsp = IsLWSP(*from);
		if (space && lwsp) continue;	// skip subsequent space
		
		// if we are adding lwsp, add a space
		if (lwsp) *to++ = ' ';
		// for regular characters, just add
		else *to++ = *from;
		
		space = lwsp;
	}
	
	// count chars
	*s = to-s-1;
	
	// strip trailing space, if there is one
	if (s[*s]==' ') --*s;
	
	// ta da
	return s;
}


/**********************************************************************
 * IsAllWhitePtr - is a string all whitespace?
 **********************************************************************/
Boolean IsAllWhitePtr(UPtr s,long len)
{
	for (;len-->0;s++) if (!IsWhite(*s)) return false;
	return true;
}

/**********************************************************************
 * IsAllLWSPPtr - is a string all lwsp?
 **********************************************************************/
Boolean IsAllLWSPPtr(UPtr s,long len)
{
	for (;len-->0;s++) if (!IsLWSP(*s)) return false;
	return true;
}

/**********************************************************************
 * IsAllUpper - is a string all lwsp?
 **********************************************************************/
Boolean IsAllUpper(PStr s)
{
	UPtr end = s+*s+1;
	
	if (!*s) return false;
	
	for (s++;s<end;s++)
		if (!isupper(*s)) return false;
	
	return true;
}

/**********************************************************************
 * Uncomma - reformat a name to remove a comma
 **********************************************************************/
PStr Uncomma(PStr name)
{
	Str255 scratch;
	UPtr comma;
	
	if (comma=PIndex(name,':'))
		*name = comma-name-1;
	else if (isupper(name[1]) && (comma=PIndex(name,',')))
	{
		MakePStr(scratch,comma+1,*name-(comma-name));
		*name = *name-*scratch-1;
		TrimInitialWhite(scratch);
		TrimWhite(scratch);
		TrimInitialWhite(name);
		TrimWhite(name);
		PInsert(name,*name+2,"\p ",name+1);
		PInsert(name,*name+*scratch+2,scratch,name+1);
	}
	return(name);
}

/************************************************************************
 * CharWidthInFont - how big is a font?
 ************************************************************************/
short CharWidthInFont(Byte c,short font,short size)
{
	short width = size/2;
	
	if (InsurancePort)
	{
		PushGWorld();
		SetPort(InsurancePort);
		TextFont(font);
		TextSize(size);
		width = CharWidth(c);
		PopGWorld();
	}
	return(width);
}

/************************************************************************
 * UTF8ToMac - convert utf8 to mac
 ************************************************************************/
PStr UTF8ToMac(PStr string)
{
	long len = *string;
	
	UTF8To88591(string+1,*string,string+1,&len);
	*string = len;
	TransLitRes(string+1,*string,ktISOMac);
	return(string);
}

/************************************************************************
 * UTF8To88591 - convert utf8 to 8859-1
 ************************************************************************/
void UTF8To88591(Ptr inStr, long inLen, Ptr outStr, long *outLen)
{
	long len;
	Byte tempChar;
	
	len = 0L;
	while(--inLen >= 0L)
	{
		tempChar = *inStr++;
		if(tempChar & 0x80)
		{
			if(tempChar & 0x3C)
			{
				*outStr++ = '?';
				++len;
				while((tempChar <<= 1) & 0x80)
				{
					--inLen;
					++inStr;
				}
			}
			else
			{
				*outStr++ = ((tempChar & 0x03) << 6) + (*inStr & 0x7F);
				++len;
				--inLen;
			}
		}
		else
		{
			*outStr++ = tempChar;
			++len;
		}
	}
	*outLen = len;
}

#undef StringToNum
void MyStringToNum(PStr string,long *num)
{
	if(!*string || !(isdigit(string[1])||string[1]=='-'||string[1]=='+')) *num=0L; else StringToNum(string,num);
}

/************************************************************************
 * PtrPtrMatchLWSP - match two strings, considering all LWSP as the same
 ************************************************************************/
Boolean PtrPtrMatchLWSP(Ptr lookFor, short lookLen, Ptr text, uLong textLen, Boolean atStart, Boolean atEnd)
{
	Str255 shortLook;
	UPtr spot, end;
	UPtr matchEnd;
	
	lookLen = MIN(lookLen,250);
	
	// First, copy the string being looked for, collapsing LWSP
	end = lookFor+lookLen;
	spot = shortLook+1;
	while (lookFor<end)
	{
		// For lwsp, copy a single space
		if (IsLWSP(*lookFor))
		{
			*spot++ = ' ';
			do {lookFor++;} while(lookFor<end && IsLWSP(*lookFor));
		}
		// Copy anything else
		else
			*spot++ = *lookFor++;
	}
	*shortLook = spot-shortLook-1;
	TrimAllWhite(shortLook);
	if (!*shortLook) return true;	// empty matches all
	
	// does match need to be at beginning?
	if (atStart)
		if (PPtrMatchLWSPSpot(shortLook,text,textLen,&matchEnd))
			if (atEnd) return matchEnd==text+textLen;
			else return true;
		else return false;
		
	// Now, test at each spot in the string, except be moderately clever around LWSP
	end = text+textLen-*shortLook+1;
	spot = text;
	
	while (spot<end)
	{
		if (PPtrMatchLWSPSpot(shortLook,spot,textLen,&matchEnd))
			if (atEnd)
			{
				// if match doesn't go to end, ignore it
				if (matchEnd==text+textLen+1) return true;
				// That +1 is because PPtrMatchLWSPSpot points matchEnd
				// past the return rather than at the return.
			}
			else return true;	// we found it!
		
		// Here's the moderate intelligence
		if (IsLWSP(*spot)) do {spot++;} while (spot<end && IsLWSP(*spot));
		else spot++;	// move ahead one
	}
	
	// if we get here, we didn't match
	return false;
}

/************************************************************************
 * PPtrMatchLWSPSpot - match two strings, considering all LWSP as the same, starting at a given spot
 ************************************************************************/
Boolean PPtrMatchLWSPSpot(PStr look,Ptr text,uLong textLen,UPtr *matchEnd)
{
	UPtr textEnd = text+textLen;
	UPtr lookEnd = look+*look+1;
	UPtr lookSpot = look+1;
	Byte c1, c2;
	
	while (1)
	{
		// advance text to next non-LWSP char
		while (text<textEnd && IsLWSP(*text)) text++;
		
		// advance look to next non-LWSP char
		while (lookSpot<lookEnd && IsLWSP(*lookSpot)) lookSpot++;
		
		// did we run out of string being looked for?  If so, we've succeeded
		if (lookSpot==lookEnd) break;
		
		// did we run out of string being looked in?  If so, we've failed
		if (text==textEnd) return(false);
		
		// Ok, so now we know that we have chars left in each string and they're not LWSP
		// Do they match?
		c1 = *lookSpot;
		c2 = *text;
		if (isupper(c1)) c1=tolower(c1);
		if (isupper(c2)) c2=tolower(c2);
		if (c1!=c2) return false;	// not the same; fail
		
		// So far, so good.  Advance both strings
		lookSpot++;
		text++;
		
		// Ok, now they both have to be either LWSP or non-LWSP
		if (lookSpot!=lookEnd && (text==textEnd||IsLWSP(*text)) != (IsLWSP(*lookSpot)))
			return false;
		
		// round and round we go
	}
	
	// Life is good!  We have a match!
	
	// Point at where we matched
	if (matchEnd) *matchEnd = text;
	
	return true;
}

/************************************************************************
 * PStrToNum - StringToNum as a function
 ************************************************************************/
long PStrToNum(PStr string)
{
	long	num;
	
	StringToNum(string,&num);
	return num;
}

/************************************************************************
 * ShortVersString - turn a short into an x.x.x.x version string
 ************************************************************************/
UPtr ShortVersString(short vers, UPtr versionStr)
{
	Str255 scratch, hex; 
	unsigned char *scan;

	*versionStr = 0;
	*hex = 0;
				
	ComposeString(scratch,"\p%x",vers);
	scan = scratch+1;
	 
	// skip all leading 0's
	while ((scan <= (scratch + scratch[0])) && (*scan == '0')) scan++;

	while (scan <= (scratch + scratch[0]))
	{
		PCatC(hex,*scan);
		PCatC(hex,'.');
		scan++;
	}
	hex[0]--;	//take off trailing period
	
	PCopy(versionStr,hex);
	return (versionStr);
}

/************************************************************************
 * StripLeadingItems - strip items from the beginning of a string
 ************************************************************************/
PStr StripLeadingItems(PStr string, short resID)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	GetRString(s,resID);
	spot = s+1;
	
	while (PToken(s,token,&spot,","))
	{
		if (BeginsWith(string,token))
		{
			BMD(string+1+*token,string+1,*string - *token);
			*string -= *token;
			break;
		}
	}
	
	return string;
}

/************************************************************************
 * StripTrailingItems - strip items from the end of a string
 ************************************************************************/
PStr StripTrailingITems(PStr string, short resID)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	GetRString(s,resID);
	spot = s+1;
	
	while (PToken(s,token,&spot,","))
	{
		if (EndsWith(string,token))
		{
			*string -= *token;
			break;
		}
	}
	
	return string;
}

/************************************************************************
 * EndsWithItem - does a string end with one of these items?
 ************************************************************************/
Boolean EndsWithItem(PStr string, short resID)
{
	Str255 s;
	Str63 token;
	UPtr spot;
	
	GetRString(s,resID);
	spot = s+1;
	
	while (PToken(s,token,&spot,","))
		if (EndsWith(string,token)) return true;
	
	return false;
}
