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

#ifndef XML_H
#define XML_H

//	Tokenizing data
typedef struct
{
	Ptr		pText;
	long	size;
	long	offset;
	long	tokenStart;
	long	tokenLen;
	long	attrStart;
	long	attrLen;
	AccuPtr	aKeywords;
} TokenInfo;
enum { kTokenDone, kElementTag, kContent, kEndTag, kEmptyElementTag, kCommentTag };

// functions for generating XML
void AccuAddCRLF(AccuPtr a);
void AccuAddTag(AccuPtr a,StringPtr sKeyword,Boolean endTag);
void AccuAddXMLObject(AccuPtr a,StringPtr sKeyword,StringPtr value);
void AccuAddXMLObjectInt(AccuPtr a,StringPtr sKeyword,long value);
void AccuAddXMLWithAttr(AccuPtr a,StringPtr sKeyword,StringPtr sAttr,Boolean emptyElement);
void AccuAddXMLObjectHandle(AccuPtr a,StringPtr sKeyword,Handle hValue);
void AccuAddXMLWithAttrPtr(AccuPtr a,StringPtr sKeyword,Ptr pAttr,long len,Boolean emptyElement);
void AccuAddTagLine(AccuPtr a,StringPtr sKeyword,Boolean endTag);
void XMLNoIndent();
void XMLIncIndent();
void XMLDecIndent();
void AccuIndent(AccuPtr a);

// functions for parsing XML
short GetNextToken(TokenInfo *tokenInfo);
PStr TokenToString(TokenInfo *pInfo, StringPtr s);
OSErr TokenToURL(long *offset,AccuPtr a,TokenInfo *tokenInfo);
short GetTokenIdx(AccuPtr accuKeywordList,StringPtr sToken);
short GetNumAttribute(TokenInfo *pInfo,long *value);
short GetAttribute(TokenInfo *pInfo,StringPtr sValue);
unsigned char SkipWhiteSpace(TokenInfo *pInfo);
unsigned char GetNextTokenChar(TokenInfo *tokenInfo);
Boolean IsWhiteSpace(char c);

// High-Level parser

typedef struct XMLNameValuePairStruct
{
	short type;
	short id;
	long numValue;
	Str63 shortValue;
	UHandle longValue;
	void *userData;
} XMLNVP, *XMLNVPPtr, **XMLNVPH;

typedef struct XMLBinaryStruct XMLBinary, *XMLBinPtr, **XMLBinH;

struct XMLBinaryStruct
{
	XMLNVP self;
	StackHandle attribs;
	StackHandle contents;
};

OSErr XMLParse(UHandle text,short keyWordStrn,AccuPtr keywords,StackHandle xmlbStack);
void XMLBinDispose(XMLBinPtr xmlb);
void XMLBinStackDispose(StackHandle xmlbStack);
void XMLNVPDispose(XMLNVPPtr nvpP);
void XMLNVPStackDispose(StackHandle nvpStack);
#ifdef DEBUG
OSErr XMLBinStackPrint(AccuPtr a,StackHandle xmlbStack);
OSErr XMLBinPrint(AccuPtr a,XMLBinPtr xmlbP);
OSErr XMLNVPStackPrint(AccuPtr a,StackHandle nvpStack);
OSErr XMLNVPPrint(AccuPtr a,XMLNVPPtr nvpP);
#endif
#endif
