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

#include "xml.h"
#define FILE_NUM 136


// prototypes
static void UnGetTokenChar(TokenInfo *pInfo);
static unsigned char PeekTokenChar(TokenInfo *pInfo,short offset);
static long CopyWithCharConv(UPtr from, UPtr to, long len);
OSErr XMLParseElement(TokenInfo *tip, XMLBinPtr xmlbP);

//	Global variables
short	gIndent;

/************************************************************************
 *					  Generate XML 
 ************************************************************************/


/************************************************************************
 * AccuAddXMLObject - add an XML value from a string
 ************************************************************************/
void AccuAddXMLObject(AccuPtr a,StringPtr sKeyword,StringPtr value)
{
	AccuIndent(a);
	AccuAddTag(a,sKeyword,false);
	AccuAddStr(a,value);
	AccuAddTag(a,sKeyword,true);
	AccuAddCRLF(a);
}

/************************************************************************
 * AccuAddXMLObjectInt - add an XML value from an integer
 ************************************************************************/
void AccuAddXMLObjectInt(AccuPtr a,StringPtr sKeyword,long value)
{
	Str32	s;
	
	NumToString(value,s);
	AccuIndent(a);
	AccuAddTag(a,sKeyword,false);
	AccuAddStr(a,s);
	AccuAddTag(a,sKeyword,true);
	AccuAddCRLF(a);
}

/************************************************************************
 * AccuAddTagLine - add a tag on a line by itself
 ************************************************************************/
void AccuAddTagLine(AccuPtr a,StringPtr sKeyword,Boolean endTag)
{
	AccuIndent(a);
	AccuAddTag(a,sKeyword,endTag);
	AccuAddCRLF(a);
}

/************************************************************************
 * AccuAddXMLObjectHandle - add an XML value from a handle
 ************************************************************************/
void AccuAddXMLObjectHandle(AccuPtr a,StringPtr sKeyword,Handle hValue)
{
	AccuIndent(a);
	AccuAddTag(a,sKeyword,false);
	AccuAddHandle(a,hValue);
	AccuAddTag(a,sKeyword,true);
	AccuAddCRLF(a);
}

/************************************************************************
 * AccuAddXMLWithAttrPtr - add an XML object with attributes from a ptr
 ************************************************************************/
void AccuAddXMLWithAttrPtr(AccuPtr a,StringPtr sKeyword,Ptr pAttr,long len,Boolean emptyElement)
{
	AccuIndent(a);
	AccuAddChar(a,'<');
	AccuAddStr(a,sKeyword);
	AccuAddChar(a,' ');
	AccuAddPtr(a,pAttr,len);
	if (emptyElement)
		AccuAddChar(a,'/');
	AccuAddChar(a,'>');
	AccuAddCRLF(a);
}

/************************************************************************
 * AccuAddXMLWithAttr - add an XML object with attributes from a string
 ************************************************************************/
void AccuAddXMLWithAttr(AccuPtr a,StringPtr sKeyword,StringPtr sAttr,Boolean emptyElement)
{
	AccuAddXMLWithAttrPtr(a,sKeyword,sAttr+1,*sAttr,emptyElement);
}

/************************************************************************
 * AccuAddTag - add a keyword
 ************************************************************************/
void AccuAddTag(AccuPtr a,StringPtr sKeyword,Boolean endTag)
{
	AccuAddChar(a,'<');
	if (endTag) AccuAddChar(a,'/');
	AccuAddStr(a,sKeyword);
	AccuAddChar(a,'>');
}

/************************************************************************
 * AccuAddCRLF - add CR and LF
 ************************************************************************/
void AccuAddCRLF(AccuPtr a)
{
	AccuAddChar(a,'\r');
	AccuAddChar(a,'\n');
}

/************************************************************************
 * AccuIndent - add tabs to indent
 ************************************************************************/
void AccuIndent(AccuPtr a)
{
	short	n;
	for(n=gIndent;n--;)
		AccuAddChar(a,tabChar);
}

/************************************************************************
 * XMLNoIndent - reset indentation
 ************************************************************************/
void XMLNoIndent(void)
{
	gIndent = 0;
}

/************************************************************************
 * XMLIncIndent - increment indentation
 ************************************************************************/
void XMLIncIndent(void)
{
	gIndent++;
}

/************************************************************************
 * XMLDecIndent - decrement indentation
 ************************************************************************/
void XMLDecIndent(void)
{
	if (gIndent) gIndent--;
}

/************************************************************************
 *					  Parse XML 
 ************************************************************************/

/************************************************************************
 * TokenToURL - put token in handle as c-string
 ************************************************************************/
OSErr TokenToURL(long *offset,AccuPtr a,TokenInfo *tokenInfo)
{
	Handle	hURL;
	OSErr	err = noErr;
	
	if (offset) *offset = a->offset;
	if (hURL = NuHTempOK(tokenInfo->tokenLen+1))
	{
		long	len;

		//	Convert any character references
		len = CopyWithCharConv(tokenInfo->pText+tokenInfo->tokenStart, LDRef(hURL), tokenInfo->tokenLen);
		UL(hURL);
		if (len != tokenInfo->tokenLen)
			SetHandleSize(hURL,len+1);

		//	Add c-string null terminator
		(*hURL)[len] = 0;
		
		err = AccuAddHandle(a,hURL);
		ZapHandle(hURL);
	}
	else err = MemError();
	return err;
}

/************************************************************************
 * GetNextTokenChar - get the next character
 ************************************************************************/
unsigned char GetNextTokenChar(TokenInfo *pInfo)
{
	if (pInfo->offset < pInfo->size)
		return (pInfo->pText)[pInfo->offset++];
	else
		return 0;
}

/************************************************************************
 * CopyWithCharConv - copy characters, processing character references
 ************************************************************************/
static long CopyWithCharConv(UPtr from, UPtr to, long len)
{
	char	c;
	enum { kLiteral,kDecimal,kHex } refType;
	Str255	s;
	long	num;
	TokenInfo	textInfo;
	long	toLen = 0;
	
	textInfo.pText = from;
	textInfo.size = len;
	textInfo.offset = 0;
	
	while (c = GetNextTokenChar(&textInfo))
	{
		if (c=='&')
		{
			refType = kLiteral;
			c = GetNextTokenChar(&textInfo);
			if (c=='#')
			{
				//	Numeric encoding
				c = GetNextTokenChar(&textInfo);
				if (c=='x' || c=='X')
				{
					//	Hexadecimal
					c = GetNextTokenChar(&textInfo);
					refType = kHex;
				}
				else
					refType = kDecimal;
			}
			
			//	Get reference
			for (*s = 0; c && c!=';'; c = GetNextTokenChar(&textInfo))
				PCatC(s,c);

			switch (refType)
			{
				case kLiteral:
					MyLowerStr(s);
					c = FindSTRNIndex(HTMLLiteralsStrn,s);
					break;
				case kDecimal:
					StringToNum(s,&num);
					c = num;
					break;
				case kHex:
					if (*s<2)
					{
						s[2]=s[1];
						s[1]='0';
						s[0] = 2;
					}
					Hex2Bytes(s+*s-1,2,&c);
					break;
			}
		}
		*to++ = c;
		toLen++;
	}
	return toLen;
}

/************************************************************************
 * TokenToString - copy a token into a string
 ************************************************************************/
PStr TokenToString(TokenInfo *pInfo, StringPtr s)
{
	if (pInfo->tokenLen < 256)
		*s = CopyWithCharConv(pInfo->pText+pInfo->tokenStart, s+1, pInfo->tokenLen);
	else
		*s = 0;
	return s;
}

/************************************************************************
 * PeekTokenChar - peek at a character
 ************************************************************************/
static unsigned char PeekTokenChar(TokenInfo *pInfo,short offset)
{
	if (pInfo->offset+offset < pInfo->size)
		return (pInfo->pText)[pInfo->offset+offset];
	else
		return 0;
}

/************************************************************************
 * UnGetTokenChar - backup a character
 ************************************************************************/
static void UnGetTokenChar(TokenInfo *pInfo)
{
	if (pInfo->offset)
		pInfo->offset--;
}

/************************************************************************
 * IsWhiteSpace - is character white space (including PC LF's)
 ************************************************************************/
Boolean IsWhiteSpace(char c)
{
	return c==' ' || c==0x9 || c==0xd || c==0xa;
}

/************************************************************************
 * SkipWhiteSpace - get the next character that's not white space
 ************************************************************************/
unsigned char SkipWhiteSpace(TokenInfo *pInfo)
{
	unsigned char	c;
	
	do
	{
		c = GetNextTokenChar(pInfo);
	} while (IsWhiteSpace(c));
	return c;
}

/************************************************************************
 * GetNextToken - return the next token (element or element content), don't return comments
 ************************************************************************/
short GetNextToken(TokenInfo *pInfo)
{
	unsigned char	c,cLast;
	short	tokenType;
	
	do
	{
		c = SkipWhiteSpace(pInfo);
		if (c=='<')
		{
			//	Tag
			tokenType = kElementTag;
			c = SkipWhiteSpace(pInfo);
			if (c=='!' && PeekTokenChar(pInfo,0)=='-' && PeekTokenChar(pInfo,1)=='-')
			{
				//	Comment
				pInfo->offset += 2;
				do
				{
					c = GetNextTokenChar(pInfo);			
				} while (c && (c!='-' || PeekTokenChar(pInfo,0)!='-' || PeekTokenChar(pInfo,1)!='>'));
				pInfo->offset += 2;
				tokenType = kCommentTag;
				continue;
			}

			if (c=='?')
			{
				//	Processing instruction, skip it
				do
				{
					c = GetNextTokenChar(pInfo);			
				} while (c && c!='>');
				tokenType = kCommentTag;
				continue;
			}

			if (c=='/')
			{
				c = GetNextTokenChar(pInfo);
				tokenType = kEndTag;
			}			
			
			//	Find end of tag
			pInfo->attrStart = pInfo->attrLen = 0;
			pInfo->tokenStart = pInfo->offset-1;
			do
			{
				cLast = c;
				c = GetNextTokenChar(pInfo);			
			} while (c && c!='>' && !IsWhiteSpace(c));
			pInfo->tokenLen = pInfo->offset-1-pInfo->tokenStart;
			if (cLast=='/')
			{
				//	Empty element tag
				tokenType = kEmptyElementTag;
				pInfo->tokenLen--;
			}
			
			if (IsWhiteSpace(c))
			{
				//	Token has attributes
				c = SkipWhiteSpace(pInfo);
				pInfo->attrStart = pInfo->offset-1;
				while (c && c!='>')
				{
					cLast = c;
					c = GetNextTokenChar(pInfo);			
				};
				pInfo->attrLen = pInfo->offset-1-pInfo->attrStart;				
				if (cLast=='/')
				{
					//	Empty element tag
					tokenType = kEmptyElementTag;
					pInfo->attrLen--;
				}
			}
		}
		else if (c)
		{
			//	Not tag. Should be element content
			pInfo->tokenStart = pInfo->offset-1;
			do
			{
				c = GetNextTokenChar(pInfo);
			}	while (c && c != '<');
			UnGetTokenChar(pInfo);
			pInfo->tokenLen = pInfo->offset - pInfo->tokenStart;
			tokenType = kContent;
		}
		else
			//	End of file
			tokenType = kTokenDone;
	} while (tokenType == kCommentTag);
	
	return tokenType;
}


/************************************************************************
 * GetTokenIdx - find token in string list or add to list
 ************************************************************************/
short GetTokenIdx(AccuPtr accuKeywordList,StringPtr sToken)
{
	short	result;
	
	if (!(result = FindSTRNIndexRes(accuKeywordList->data,sToken)))
	{
		//	Not in token list. Add it.
		if (!AccuAddPtr(accuKeywordList,sToken,*sToken+1))
			result = ++*(short *)*accuKeywordList->data;
		else
			result = 0;
	}
	
	return result;
}

/************************************************************************
 * GetAttribute - get next attribute
 ************************************************************************/
short GetAttribute(TokenInfo *pInfo,StringPtr sValue)
{
	short	result = 0;
	
	if (pInfo->attrLen)
	{
		long	saveOffset = pInfo->offset;
		long	saveSize = pInfo->size;
		unsigned char	c;
		
		//	Redirect token info to look at attributes
		pInfo->offset = pInfo->attrStart;
		pInfo->size = pInfo->attrStart+pInfo->attrLen;
		
		//	Get attribute
		if (c = SkipWhiteSpace(pInfo))
		{	
			pInfo->tokenStart = pInfo->offset-1;
			do
			{
				c = GetNextTokenChar(pInfo);			
			} while (c && c!='=' && !IsWhiteSpace(c));
			if (c=='=')
			{
				pInfo->tokenLen = pInfo->offset-1-pInfo->tokenStart;
				TokenToString(pInfo, sValue);

				//	Get attribute value
				if (SkipWhiteSpace(pInfo)=='"')
				{
					result = GetTokenIdx(pInfo->aKeywords,sValue)-1;
					pInfo->tokenStart = pInfo->offset;
					do
					{
						c = GetNextTokenChar(pInfo);			
					} while (c && c!='"');
					pInfo->tokenLen = pInfo->offset-1-pInfo->tokenStart;
					TokenToString(pInfo, sValue);
				}
			}
		}
		//	Advance to next attribute
		pInfo->attrLen -= pInfo->offset-pInfo->attrStart;
		pInfo->attrStart = pInfo->offset;
		
		//	Restore info
		pInfo->offset = saveOffset;
		pInfo->size = saveSize;
	}
	return result;
}

/************************************************************************
 * GetNumAttribute - get next numeric attribute
 ************************************************************************/
short GetNumAttribute(TokenInfo *pInfo,long *value)
{
	short	idx;
	Str255	sValue;
	
	if (idx = GetAttribute(pInfo,sValue))
		StringToNum(sValue,value);
	return idx;
}

/************************************************************************
 * XMLParse - parse an xml stream into a binary representation that can
 *  be easily traversed
 ************************************************************************/
OSErr XMLParse(UHandle text,short keyWordStrn,AccuPtr keywords,StackHandle xmlbStack)
{
	TokenInfo tokenInfo;
	short i;
	Str63 keyword;
	XMLBinary xmlb;
	OSErr err;
	Accumulator a;
	
	// setup the context
	Zero(tokenInfo);
	Zero(a);
	tokenInfo.pText = LDRef(text);
	tokenInfo.size = GetHandleSize(text);
	tokenInfo.offset = 0;
	tokenInfo.aKeywords = keywords ? keywords : &a;
	if (keyWordStrn)
	{
		AccuAddChar(&a,0);
		AccuAddChar(&a,0);
		for (i=1;*GetRString(keyword,keyWordStrn+i);i++) AccuAddPtr(tokenInfo.aKeywords,keyword,*keyword+1);
		if (a.offset > 2)
			*(short*)*a.data = i-1;
	}
	
	// read elements til we're done
	do
	{
		err = XMLParseElement(&tokenInfo,&xmlb);
		if (!err) err = StackPush(&xmlb,xmlbStack);
	}
	while (!err);
	
	if (err==-1) err = noErr;	// that's how the world ends
	
	// cleanup
	if (keywords) AccuTrim(keywords);
	AccuZap(a);
	UL(text);

	// show me the way to go home...
	return err;
}

/************************************************************************
 * XMLParseElement - parse the next element in an xml context; recurse
 *   if necessary
 ************************************************************************/
OSErr XMLParseElement(TokenInfo *tip, XMLBinPtr xmlbP)
{
	OSErr err = noErr;
	
	// start with a clean slate
	Zero(*xmlbP);
	
	// read the next token
	xmlbP->self.type = GetNextToken(tip);
	
	// are we done?
	if (xmlbP->self.type==kTokenDone) return -1;
	
	// guess not.  Analyze value.
	TokenToString(tip,GlobalTemp);
	if (*GlobalTemp < sizeof(xmlbP->self.shortValue))
	{
		// value is short.  Assume it is keywordly-interesting
		PCopy(xmlbP->self.shortValue,GlobalTemp);
		xmlbP->self.id = GetTokenIdx(tip->aKeywords,xmlbP->self.shortValue);
		StringToNum(xmlbP->self.shortValue,&xmlbP->self.numValue);
	}
	else
	{
		// value is long.  Stash in handle
		Accumulator a;
		Zero(a);
		err = TokenToURL(nil,&a,tip);
		if (!err && a.offset<2)
		{
			AccuZap(a);
			err = fnfErr;
		}
		if (!err)
		{
			a.offset--;
			AccuTrim(&a);
			xmlbP->self.longValue = a.data;
		}
	}
	
	// if the value can have attributes, grab them, too
	if (!err)
	if (xmlbP->self.type==kElementTag||xmlbP->self.type==kEmptyElementTag)
	{
		XMLNVP nvp;
		
		for(;!err;)
		{
			// clear out the old
			*nvp.shortValue = 0;
			nvp.longValue = 0;
			nvp.numValue = 0;
			nvp.id = 0;
			
			// grab the new
			nvp.type = GetAttribute(tip,GlobalTemp);
			
			// are we done?
			if (!nvp.type) break;
			
			nvp.type++;	// attributes are off by one
			
			// process the value
			if (*GlobalTemp < sizeof(nvp.shortValue))
			{
				PCopy(nvp.shortValue,GlobalTemp);
				StringToNum(nvp.shortValue,&nvp.numValue);
				nvp.id = GetTokenIdx(tip->aKeywords,nvp.shortValue);
			}
			else
			{
				nvp.longValue = NuHandle(*GlobalTemp+1);
				if (nvp.longValue) PCopy(*nvp.longValue,GlobalTemp);
				else
					err = MemError();
			}
			
			// and put it on the stack
			if (xmlbP->attribs || !(err=StackInit(sizeof(nvp),&xmlbP->attribs)))
				err = StackPush(&nvp,xmlbP->attribs);
		}
	}
	
	// ok, now the value is all safely tucked away, let's see what it is
	if (err || xmlbP->self.type!=kElementTag)
		// easy!  We're done!
		;
	else
	{
		XMLBinary xmlb;
		
		// We need to fetch the contents of the element		
		do
		{
			Zero(xmlb);
			if (!(err=XMLParseElement(tip,&xmlb)))
			{
				switch (xmlb.self.type)
				{
					case kTokenDone:
						// we ran out of tokens before the closing tag
						err = eofErr;
						break;
						
					// Stuff that goes in the contents
					case kElementTag:
					case kContent:
					case kEmptyElementTag:
						if (!xmlbP->contents && (err=StackInit(sizeof(xmlb),&xmlbP->contents))) break;
						err = StackPush(&xmlb,xmlbP->contents);
						break;
					
					// An end tag; it better be us!
					case kEndTag:
						if (xmlb.self.id != xmlbP->self.id)
							err = paramErr;
						break;
					
					case kCommentTag:
						ZapHandle(xmlb.self.longValue);	// if we have a value, we don't want it
						break;
					
					default:
						ASSERT(0);
						// we don't belong here!
						err = paramErr;
						break;
				}
			}
		}
		while (!err && xmlb.self.type != kEndTag);
		XMLBinDispose(&xmlb);
	}
	
	// Whew!  Did we win?
	if (err)
	{
		XMLBinDispose(xmlbP);
		Zero(*xmlbP);
	}
	else
	{
		StackCompact(xmlbP->attribs);
		StackCompact(xmlbP->contents);
	}
	return err;
}

/************************************************************************
 * XMLBinStackDispose - dispose of a stackful of xmlb's
 ************************************************************************/
void XMLBinStackDispose(StackHandle stack)
{
	XMLBinary xmlb;
	
	while (!StackPop(&xmlb,stack)) XMLBinDispose(&xmlb);
	DisposeHandle(stack);
}

/************************************************************************
 * XMLBinDispose - dispose the contents of an xml binary
 ************************************************************************/
void XMLBinDispose(XMLBinPtr xmlbP)
{
	XMLBinStackDispose(xmlbP->contents);
	XMLNVPStackDispose(xmlbP->attribs);
	XMLNVPDispose(&xmlbP->self);
	xmlbP->contents = nil;
	xmlbP->attribs = nil;
}

/************************************************************************
 * XMLNVPStackDispose - dispose of a stackful of nvp's
 ************************************************************************/
void XMLNVPStackDispose(StackHandle stack)
{
	XMLNVP nvp;
	
	while (!StackPop(&nvp,stack)) XMLNVPDispose(&nvp);
	DisposeHandle(stack);
}

/************************************************************************
 * XMLNVPDispose - dispose the contents of an xml binary
 ************************************************************************/
void XMLNVPDispose(XMLNVPPtr nvpP)
{
	ZapHandle(nvpP->longValue);
}

#ifdef DEBUG
/************************************************************************
 * XMLBinStackPrint - print out a stack of binary xml
 ************************************************************************/
OSErr XMLBinStackPrint(AccuPtr a,StackHandle xmlbStack)
{
	OSErr err = noErr;
	
	if (!xmlbStack) return noErr;
	else
	{
		short i;
		short n = (*xmlbStack)->elCount;
		XMLBinary xmlb;
	
		for (i=0;i<n;i++)
		{
			StackItem(&xmlb,i,xmlbStack);
			if (err = XMLBinPrint(a,&xmlb))
				break;
		}
	}
	return err;
}

/************************************************************************
 * XMLBinPrint - print an individual xml binary
 ************************************************************************/
OSErr XMLBinPrint(AccuPtr a,XMLBinPtr xmlbP)
{
	OSErr err;
	
	err = XMLNVPPrint(a,&xmlbP->self);
	if (!err)
	if (xmlbP->attribs)
	{
		XMLIncIndent();
		err = XMLNVPStackPrint(a,xmlbP->attribs);
		XMLDecIndent();
	}
	if (!err)
	if (xmlbP->contents)
	{
		XMLIncIndent();
		err = XMLBinStackPrint(a,xmlbP->contents);
		XMLDecIndent();
	}
	return err;
}

/************************************************************************
 * XMLNVPStackPrint - print a stack of xml name-value pairs
 ************************************************************************/
OSErr XMLNVPStackPrint(AccuPtr a,StackHandle nvpStack)
{
	OSErr err = noErr;

	if (!nvpStack) return noErr;
	else
	{
		short i;
		short n = (*nvpStack)->elCount;
		XMLNVP nvp;
	
		for (i=0;i<n;i++)
		{
			StackItem(&nvp,i,nvpStack);
			if (err = XMLNVPPrint(a,&nvp))
				break;
		}
	}
	return err;
}

/************************************************************************
 * XMLNVPPrint - print an individual xml name-value pair
 ************************************************************************/
OSErr XMLNVPPrint(AccuPtr a,XMLNVPPtr nvpP)
{
	OSErr err = noErr;
	AccuIndent(a); AccuCompose(a,"\pty: %d\015",nvpP->type);
	if (nvpP->id)
		{AccuIndent(a); AccuCompose(a,"\pid: %d\015",nvpP->id);}
	if (nvpP->numValue)
		{AccuIndent(a); AccuCompose(a,"\p#: %d\015",nvpP->numValue);}
	if (*nvpP->shortValue)
		{AccuIndent(a); AccuCompose(a,"\psv: %p\015",nvpP->shortValue);}
	if (nvpP->longValue)
		{AccuIndent(a); AccuCompose(a,"\plv: %x\015",nvpP->longValue);}
	if (nvpP->userData)
		{AccuIndent(a); AccuCompose(a,"\pud: %x\015",nvpP->userData);}
	return err;	/* Cheap code change to rid of the error */
}
#endif
