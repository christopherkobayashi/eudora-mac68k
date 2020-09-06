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

#define FILE_NUM 106
#pragma segment HTML

#define htmlNoDir ((HTMLDirectiveEnum)0)
#define htmlNoAttr ((HTMLAttributeEnum)0)

#define NewPStr(ptr,len,pstr) do{*pstr=MIN(len,sizeof(Str255)-1);BMD(ptr,pstr+1,(Byte)*pstr);}while(false)
#define CatChar(s,c) do{s[++s[0]] = c;} while(false)
#define DoAddWordSpace(a,b) (((a) && (b)) ? addWordSpaceConditional : ((a) ? addWordSpaceUnconditional : dontAddWordSpace))

typedef enum {
	htmlNoValue,
	htmlPtrValue,
	htmlHandleValue,
	htmlStringValue,
	htmlResValue,
	htmlFontValue,
	htmlNumValue,
	htmlSignedNumValue,
	htmlPercentNumValue,
	htmlRelativeNumValue,
	htmlColorValue,
	htmlCIDValue,
	htmlFileValue,
	htmlValueMakeLong=0x40000000
} HTMLValueTypeEnum;

typedef enum {
	htmlEndToken,
	htmlTextToken,
	htmlStringToken,
	htmlLiteralToken,
	htmlWSToken,
	htmlNameToken,
	htmlAttrToken,
	htmlTagToken,
	htmlTagNotToken,
	htmlTagEndToken,
	htmlCommentToken,
	htmlProcessToken,
	htmlStrayToken,
	htmlTokenMakeLong=0x40000000
} HTMLTokenType;

typedef enum {
	htmlEndState,
	htmlTextState,
	htmlStringState,
	htmlLiteralState,
	htmlWSState,
	htmlNameState,
	htmlAttrState,
	htmlTagState,
	htmlTagNotState,
	htmlTagEndState,
	htmlCommentState,
	htmlProcessState,
	htmlStrayState,
	htmlLiteralHexState,
	htmlLiteralHexQuoteState,
	htmlLiteralQuoteState,
	htmlLiteralNumState1,
	htmlLiteralNumState2,
	htmlLiteralNumQuoteState1,
	htmlLiteralNumQuoteState2,
	htmlTagNotState1,
	htmlCommentState1,
	htmlCommentState2,
	htmlCommentState3,
	htmlCommentState4,
	htmlCommentState5,
	htmlProcessState1,
	htmlInTagState,
	htmlStateMakeLong=0x40000000
} HTMLStateType;

typedef enum {
	htmlClearList = 0x00,
	htmlCompactListBit = 0x01,
	htmlNumberList = 0x02,
	htmlLowAlphaList = 0x04,
	htmlUpAlphaList = 0x08,
	htmlLowRomanList = 0x10,
	htmlUpRomanList = 0x20,
	htmlNumberCompactList = 0x03,
	htmlLowAlphaCompactList = 0x05,
	htmlUpAlphaCompactList = 0x09,
	htmlLowRomanCompactList = 0x11,
	htmlUpRomanCompactList = 0x21,
	htmlAnyNumericList = 0x3E,
	htmlListMakeLong=0x40000000
} HTMLListType;

typedef enum {
	htmlBreakNot = 0x00,
	htmlBreakBefore = 0x01,
	htmlBreakAfter = 0x02,
	htmlBreakBoth = 0x03,
	htmlBreakMakeLong=0x40000000
} HTMLBreakEnum;

typedef struct {
	HTMLAlignEnum align;
	short firstIndents;
} HTMLParaAttr;

typedef struct {
	HTMLDirectiveEnum directive;
	short styleBits;
} HTMLStyleAttr;

typedef struct {
	Handle html;
	long hSize;
	long used;
	long tStart;
	TokenType curToken;
	TokenType secondToken;
	TokenType thirdToken;
	TokenType stringToken;
	HTMLStateType state;
	HTMLStateType wsState;
	Boolean wsHasNewLine;
} HTMLTokenContext;

typedef struct {
	long tokenLen;
	HTMLTokenType tokenType;
#ifdef DEBUG
	Str31 dbTokenStr;
#endif
} HTMLTokenAndLen;

typedef struct {
	short fontID;
	short fontSize;
	RGBColor color;
	Byte label;
	HTMLDirectiveEnum directive;
	OptionBits uniFlags;
} HTMLFontChange;

typedef struct {
	short fontID;
	short fontSize;
	RGBColor color;
	short direction;
	short justification;
	short startMargin;
	short endMargin;
	short indent;
	short quoteLevel;
	OptionBits uniFlags;
	Byte paraFlags;
	HTMLDirectiveEnum directive;
	HTMLListType listType;
	long listNum;
	long listCount;
	long preformCount;
	long emptyP;
	long emptyDiv;
} HTMLParaChange;

typedef struct {
	HTMLListType listType;
	HTMLDirectiveEnum directive;
	long listNum;
	long listCount;
	long preformCount;
	long emptyP;
	long emptyDiv;
	StackHandle paraStack;
} HTMLParaContext;

typedef struct {
	AccuPtr html;
	long newLine;
	long lastSpace;
	long lastEqual;
	long lastGreater;
	long lastChar;
	long eightBits;
	long lineLimit;
	long hardLimit;
	Str15 spanString;
} HTMLOutContext;

typedef struct {
	unsigned short unicode;
	unsigned char filler;
	unsigned char theChar;
	unsigned char literal[];
} LiteralEntry, *LiteralPtr;

struct LiteralResource {
	short numEntries;
	LiteralEntry entries[];
} **litResH = nil;
static short **litOffsets = nil;

typedef struct MatrixRec
{
	struct MatrixRec **last;
	TableHandle table;
	long offset;
	short row;
	short col;
	short rowGroup;
	Boolean colGroup;
	Boolean cells[];
} MatrixRec, **MatrixHandle;

short FindSTRNIndexCase(short resId,PStr string);
short FindSTRNIndexResCase(UHandle resource,PStr string);
Boolean UPtrIsChar(UPtr s1,long l,UPtr s2);
void HTMLInitTokenState(HTMLTokenContext *htmlContext, Handle htmlText, long htmlOffset, long htmlLen);
void HTMLGetNextToken(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken);
void HTMLGetTokenForXMP(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, HTMLDirectiveEnum curDirective, StringPtr string);
OSErr HTMLGetLiteralHandles(Byte *rState, Byte *oState);
UniChar HTMLLiteral(Ptr literal, long len);
void HTMLAdvanceChar(HTMLTokenContext *htmlContext);
TokenType CharToToken(unsigned short c);
void HTMLSetToken(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, HTMLTokenType type, Boolean previousChar, Boolean advance);
OSErr InsertHTML(UHandle text, long *htmlOffset, long textLen, long *inOffset, PETEHandle pte, long flags);
OSErr HTMLParsePETEStuff(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, Handle *base, Handle *src, long *id, IntlConverter *converter, AccuPtr a, StringPtr string);
OSErr HTMLParseBase(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, Handle *base, AccuPtr a, StringPtr string);
OSErr HTMLParseStyle(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, Boolean *pSpaces, Boolean *bqSpaces, Boolean *listSpaces, AccuPtr a, StringPtr string);
OSErr HTMLParseImage(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, long *offset, Handle base, Handle src, long id, PETEStyleEntry *pse, AccuPtr a, StringPtr string,StackHandle partRefStack);
OSErr HTMLParseMeta(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, IntlConverter *converter, AccuPtr a, StringPtr string);
OSErr HTMLParseTable(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, long offset, MatrixHandle *matrix, HTMLParaContext *paraContext, OptionBits uniFlags, AccuPtr a, StringPtr string);
OSErr HTMLPopTable(PETEHandle pte, long *offset, MatrixHandle *matrix, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags);
OSErr HTMLParseCell(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, PETEHandle pte, long offset, OptionBits *uniFlags, MatrixHandle matrix, AccuPtr a, StringPtr string);
OSErr HTMLSetCellPara(PETEHandle pte, long offset, MatrixHandle matrix, OptionBits *uniFlags);
OSErr HTMLPopCell(PETEHandle pte, long *offset, MatrixHandle matrix, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags);
OSErr HTMLInsertHR(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, long *offset, AccuPtr a, StringPtr string);
uLong GetURLHash(StringPtr sURL,long id);
OSErr HTMLChangeFont(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, short basefont, StackHandle fontStack, PETEStyleEntry *pse, OptionBits *uniFlags, uLong validMask, AccuPtr a, StringPtr string);
OSErr HTMLPopFont(StackHandle fontStack, HTMLDirectiveEnum curDirective, PETEStyleEntry *pse, OptionBits *uniFlags);
OSErr HTMLGetParaState(PETEHandle pte, long offset, HTMLParaChange *oldPara, PETEParaInfo *pinfo, long *index, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits uniFlags, HTMLDirectiveEnum curDirective);
OSErr HTMLChangePara(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, long offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags, uLong validMask, uLong validParaMask, PETEHandle pte, AccuPtr a, StringPtr string);
OSErr HTMLPopPara(HTMLDirectiveEnum curDirective, long offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags, PETEHandle pte);
OSErr HTMLStartLink(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, StackHandle fontStack, PETEHandle pte, PETEStyleEntry *pse, OptionBits *uniFlags, long *offset, Handle base, Handle src, AccuPtr a, StringPtr string);
void HTMLStringToColor(StringPtr colorString, RGBColor *color);
Boolean HTMLHasQuotes(StringPtr ptr, long len);
void HTMLCopyQuoteString(StringPtr ptr, long len, StringPtr pstr);
void HTMLUnquoteAndTrim(AccuPtr a);
OSErr HTMLGetAttrValue(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLAttributeEnum *attr, AccuPtr value, StringPtr string);
OSErr HTMLEnsureBreak(PETEHandle pte,long inOffset);
OSErr HTMLEnsureCrAndBreak(PETEHandle pte, long inOffset, long *newOffset, PETEStyleEntry *pse);
OSErr HTMLInsertText(PETEHandle pte, long *offset, long len, StringHandle h, long hOff, StringPtr p, PETEStyleEntry *pse, AccuPtr a, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, WordSpaceEnum addSpace, PETEStyleEntry *spaceStyle);
OSErr HTMLDumpText(PETEHandle pte, long offset, AccuPtr a, PETEStyleEntry *ase, Boolean trimNbsp);
Boolean HTMLGetFontID(StringPtr name, short *fontID);
void HTMLNumToListNum(long num, StringPtr string, HTMLListType listType);
OSErr HTMLInsertListItem(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, long *offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, PETEHandle pte, AccuPtr a, StringPtr string);
OSErr HTMLFlushText(PETEHandle pte, long *offset, AccuPtr a, PETEStyleEntry *pse, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, Boolean trimNbsp);
OSErr HTMLInsertUniChar(PETEHandle pte, long *offset, UniChar c, PETEStyleEntry *pse, AccuPtr a, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, WordSpaceEnum addSpace, PETEStyleEntry *spaceStyle);

OSErr BuildHTMLLo(HTMLOutContext *htmlContext,PETEHandle pte,Handle text,long len,long offset,PETEStyleListHandle pslh,PETEParaScrapHandle paraScrap,long partID,PStr mid,StackHandle specStack,FSSpecPtr errSpec);
void HTMLInitOutContext(HTMLOutContext *htmlContext, AccuPtr html);
OSErr HTMLAddDirective(HTMLOutContext *htmlContext,HTMLBreakEnum breaks,Boolean close,HTMLTokenType token,...);
OSErr HTMLAddValue(HTMLOutContext *htmlContext, StringPtr s, long offset, long len);
OSErr HTMLAddChar(HTMLOutContext *htmlContext, unsigned char c, Boolean literal, Boolean allowBreak);
OSErr HTMLRemoveStyles(HTMLOutContext *htmlContext, short styleBits, short *styleRemoved, StackHandle styleStack);
OSErr HTMLPushStyle(HTMLDirectiveEnum directive, short styleBits, StackHandle styleStack);
unsigned char HTMLGetQuoteChar(StringPtr s, long len);
void HTMLTextStyleDiff(PETETextStylePtr oldStyle, PETETextStylePtr newStyle, short *styleOff, short *styleOn);
short HTMLGetSizeIndex(short fontSize, Boolean fixed);
void HTMLColorToString(RGBColorPtr color, StringPtr string);
long HTMLAccuStringToNum(AccuPtr a, StringPtr s);
long HTMLAccuStringToNumLo(AccuPtr a, StringPtr s, Boolean *prop);
OSErr HTMLPictToDisk(FSSpecPtr spec, MessHandle messH, uLong uidHash, PicHandle thePict);
OSErr HTMLPutInSpool(FSSpecPtr spec, MessHandle messH, uLong uidHash);
OSErr HTMLAddHR(HTMLOutContext *htmlContext, HRGraphicHandle hrg, Boolean close);
Boolean HTMLHasFrom(AccuPtr html, long offset);
Boolean HTMLEqualFonts(short fontID1, short fontID2);
short HTMLFontWithAClue(short fontID);
MessHandle Pte2MessH(PETEHandle pte);
Boolean HTMLIsNetGraphic(FGIHandle fgi);
OSErr HTMLCheckLineBreak(HTMLOutContext *htmlContext, Boolean allowBreakHere, Boolean wantSpace);
OSErr HTMLBuildTable(HTMLOutContext *htmlContext,Handle text,long offset,TableHandle t,long partID,PStr mid,StackHandle specStack,FSSpecPtr errSpec);
OSErr HTMLAddMeasureAttr(HTMLOutContext *htmlContext, HTMLBreakEnum breaks, Boolean close, HTMLAttributeEnum attr, short measure, Boolean prop);
OSErr HTMLAddAlignAttr(HTMLOutContext *htmlContext, HTMLBreakEnum breaks, Boolean close, TabAlignData *align);

OSErr AddCell(MatrixHandle matrix, short colspan, short rowspan);

OSErr InsertHTML(UHandle text, long *htmlOffset, long textLen, long *inOffset, PETEHandle pte, long flags)
{
	return InsertHTMLLo(text, htmlOffset, textLen, inOffset, pte, kTextEncodingUnknown, flags, nil);
}

OSErr InsertHTMLLo(UHandle text, long *htmlOffset, long textLen, long *inOffset, PETEHandle pte, TextEncoding encoding, long flags, StackHandle partRefStack)
{
	OSErr err;
	Boolean needWS = false, wasCR = false, inHead = false, inTab = false, emptyPara, brLastTag = false;
	Boolean justBold, bqSpaces = true, listSpaces = true, pSpaces = true, firstPara;
	long offset, index, id = 0L, boldCount = 0L, italicCount = 0L, underlineCount = 0L, htmlCount = 0L, xHtmlCount = 0L, nlCount = 2L;
	PETEDocInfo docInfo;
	HTMLTokenContext htmlContext;
	HTMLTokenAndLen htmlToken;
	HTMLTokenType tagToken = htmlEndToken, lastTagToken;
	HTMLDirectiveEnum lastDirective = htmlNoDir, curDirective = htmlNoDir;
	HTMLParaContext paraContext = {htmlClearList,0L,0L,0L,0L,0L,nil};
	StackHandle fontStack = nil;
	Str255 string;
	PETEStyleEntry pse, spaceStyle, charsStyle;
	PETEParaInfo pinfo;
	Accumulator a = {0L,0L,nil}, chars;
	Handle base = nil, src = nil;
	short basefont = 3;
	uLong validMask = ~GetPrefLong(PREF_INTERPRET_ENRICHED);
	uLong validParaMask = ~GetPrefLong(PREF_INTERPRET_PARA);
	IntlConverter converter;
	UniChar theChar;
	MatrixHandle matrix = nil;
	
	err = PETEGetDocInfo(PETE,pte,&docInfo);
	if(err) return err;
	
	err = PETESetRecalcState(PETE,pte,false);
	if(err) return err;
	
	Zero(pse);
	pse.psStyle.textStyle.tsFont = kPETEDefaultFont;
	pse.psStyle.textStyle.tsSize = kPETERelativeSizeBase;
	SetPETEDefaultColor(pse.psStyle.textStyle.tsColor);
	
	if (flags & kDontEnsureCR)
	{
		firstPara = true;
		if (*inOffset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
		else offset = *inOffset;
	}
	else
	{
		firstPara = false;
		err = HTMLEnsureCrAndBreak(pte,*inOffset,&offset,&pse);
	}
	
	*inOffset = offset;
	if(err) return err;

	Zero(pinfo);
	err = PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&pinfo);
	if(err) return err;
	
	err = PETEGetParaIndex(PETE,pte,offset,&index);
	if(err) return err;
	
	if (flags & kNoMargins)
	{
		Rect	peteRect;		
		PeteRect(pte,&peteRect);
		pinfo.startMargin = 0;	//	We don't want margins
		pinfo.endMargin = RectWi(peteRect)+1;
	}
	
	err = PETESetParaInfo(PETE,pte,index,&pinfo,peAllParaValid & ~peTabsValid);
	if(err) return err;
	
	emptyPara = true;
	
	HTMLInitTokenState(&htmlContext, text, *htmlOffset, textLen);
	
	err = CreateIntlConverter(&converter, encoding);
	if(err) return err;
	
	err = StackInit(sizeof(HTMLFontChange), &fontStack);
	if(err) goto CleanConverter;
	
	err = StackInit(sizeof(HTMLParaChange), &paraContext.paraStack);
	if(err) goto CleanFontStack;
	
	err = AccuInit(&chars);
	if(err) goto CleanParaStack;

	/* Loop through the tokens in the html until we run out */
	do {
		if((tagToken == htmlTagToken) && ((curDirective == htmlXmpDir) || (curDirective == htmlListDir) || (curDirective == htmlPlainDir))) {
			HTMLGetTokenForXMP(&htmlContext, &htmlToken, curDirective, string);
		} else {
			HTMLGetNextToken(&htmlContext, &htmlToken);
		}

#ifdef DEBUG
		MakePStr(P1,*text + *htmlOffset, htmlToken.tokenLen);
#endif

		/* Take this out once background color is properly supported */
		if((pse.psStyle.textStyle.tsColor.red == 0xFFFF) &&
		   (pse.psStyle.textStyle.tsColor.green == 0xFFFF) &&
		   (pse.psStyle.textStyle.tsColor.blue == 0xFFFF)) {
			SetPETEDefaultColor(pse.psStyle.textStyle.tsColor);
		}

		if (!a.data)
			AccuInit(&a);

		switch(htmlToken.tokenType) {
			case htmlEndToken :
				break;
			/* WS while we're in the middle of text just set the flag if not preformatted */
			case htmlWSToken :
				if(inHead || inTab) break;
				if(!paraContext.preformCount) {
					spaceStyle = pse;
					needWS = true;
					if(htmlContext.wsHasNewLine) {
						wasCR = true;
					}
					tagToken = htmlEndToken;
					break;
				}
				/* Preformatted, so just insert as if it were text */
				needWS = wasCR = false;
			case htmlLiteralToken :
			case htmlTextToken :
				tagToken = htmlEndToken;
				if(inHead) break;
				/* Insert the text */
				if(htmlToken.tokenLen > 0) {
					if((htmlToken.tokenType != htmlLiteralToken) ||
					   ((theChar = HTMLLiteral(*text + *htmlOffset, htmlToken.tokenLen)) == 0xFFFF) ||
					    ((theChar == kUnicodeNonBreakingSpace) && inTab && (theChar = 0xFFFF, false)) ||
					   EncodingError(err = HTMLInsertUniChar(pte, &offset, theChar, &pse, &chars, &charsStyle, string, &converter, DoAddWordSpace((needWS && (nlCount == 0)), wasCR), &spaceStyle)))
					{
						err = HTMLInsertText(pte, &offset, htmlToken.tokenLen, text, *htmlOffset, nil, &pse, &chars, &charsStyle, string, &converter, DoAddWordSpace((needWS && (nlCount == 0)), wasCR), &spaceStyle);
					}
					needWS = wasCR = emptyPara = false;
					nlCount = 0L;
					brLastTag = (paraContext.preformCount && (*htmlOffset + htmlToken.tokenLen > 0L) && ((*text)[*htmlOffset + htmlToken.tokenLen - 1L] == 13));
				}
				break;
			/* Some sort of tag, so we go into "tag mode" */
			case htmlTagToken :
			case htmlTagNotToken :
			case htmlCommentToken :
			case htmlProcessToken :
				lastTagToken = tagToken;
				tagToken = htmlToken.tokenType;
				/* Go get the directive name, looping if it's not immediately first */
				do {
					*htmlOffset += htmlToken.tokenLen;
					HTMLGetNextToken(&htmlContext, &htmlToken);
				} while((htmlToken.tokenType != htmlTagEndToken) &&
				        (htmlToken.tokenType != htmlNameToken) &&
				        (htmlToken.tokenType != htmlEndToken));
				
				/* Whoops! Ran out of tokens. */
				if(htmlToken.tokenType == htmlEndToken) {
					break;
				}
				
				/* We've got the name, so go get the directive number */
				if(htmlToken.tokenType == htmlNameToken) {
					NewPStr(*text + *htmlOffset, htmlToken.tokenLen, string);
					curDirective = (HTMLDirectiveEnum)FindSTRNIndex(HTMLDirectiveStrn, string);
					*htmlOffset += htmlToken.tokenLen;
					if(curDirective == htmlScriptDir) {
						HTMLGetTokenForXMP(&htmlContext, &htmlToken, htmlScriptDir, string);
					} else {
						HTMLGetNextToken(&htmlContext, &htmlToken);
					}
				/* If it's a null not tag, use whatever directive was last */
				} else if((tagToken == htmlTagNotToken) && (lastDirective != htmlNoDir)) {
					curDirective = lastDirective;
				} else if(tagToken == htmlCommentToken) {
					curDirective = htmlCommentDir;
				}
				
				if(tagToken == htmlTagToken) {
					lastDirective = curDirective;
				}
				
				if(((tagToken == htmlTagToken) || (tagToken == htmlTagNotToken)) && !inHead) {
					
					Boolean needBreak = false;
					
					/* Insert paragraph break for block-level directives */
					switch(curDirective) {
						case htmlH1Dir :
						case htmlH2Dir :
						case htmlH3Dir :
						case htmlH4Dir :
						case htmlH5Dir :
						case htmlH6Dir :
								needBreak = true;
								goto DoBreak;
								break;
						// blockquote and p normally both cause a blank line after
						// However, that can be modified by style sheets, and this
						// is where we will skip the spacing if need be
						case htmlPDir :
						case htmlBQDir :
							if(curDirective==htmlBQDir && bqSpaces ||
								 curDirective==htmlPDir && pSpaces) {
								needBreak = true;
								goto DoBreak;
							}
						case htmlDivDir :
						case htmlCenterDir :
							if(tagToken == htmlTagNotToken) {
								err = HTMLFlushText(pte, &offset, &chars, &pse, &charsStyle, string, &converter, true);
								if(err) break;

								Zero(pinfo);
								err = PETEGetParaInfo(PETE,pte,PeteParaAt(pte,offset),&pinfo);
								if(err) break;
								
								if((pinfo.paraLength == 0L) && (nlCount == 0L)) {
									err = HTMLInsertUniChar(pte, &offset, needWS ? ' ' : 13, needWS ? &spaceStyle : &pse, &chars, &charsStyle, string, &converter, dontAddWordSpace, nil);
									if(err) break;
									if(!needWS) ++nlCount;
								}
								needBreak = brLastTag && !emptyPara;
							} else {
								needBreak = brLastTag;
							}
							goto DoBreak;
							
						case htmlTHDir :
						case htmlTDDir :
						case htmlTRDir :
							if(matrix == nil) break;
							goto DoBreak;

						case htmlDLDir :
						case htmlULDir :
						case htmlOLDir :
							if((((paraContext.listCount == 0L) && (tagToken == htmlTagToken)) ||
							   ((paraContext.listCount == 1L) && (tagToken == htmlTagNotToken))) &&
							   listSpaces) {
								needBreak = true;
							}
						case htmlPreDir :
						case htmlXmpDir :
						case htmlListDir :
						case htmlAddrDir :
						case htmlDirDir :
						case htmlMenuDir :
						case htmlLIDir :
						case htmlDTDir :
						case htmlHRDir :
						case htmlTableDir :
						case htmlCaptionDir :
					DoBreak :
							needWS = wasCR = false;
							nlCount -= offset;
							err = HTMLFlushText(pte, &offset, &chars, &pse, &charsStyle, string, &converter, true);
							if(err) break;
							if(!firstPara) {
								err = HTMLEnsureCrAndBreak(pte, offset, &offset, &pse);
								nlCount += offset;
								if(!err && needBreak && (nlCount < 2L)) {
									++nlCount;
									**Pslh = pse;
									err = PeteInsertChar(pte,offset - 1L,13,Pslh);
									if(!err) ++offset;
								}
							} else {
								firstPara = false;
							}
							brLastTag = false;
							emptyPara = true;
					}
					if(err) break;
				}
				justBold = true;

				/* Now go do the real processing of the directive */
				switch(tagToken) {
					case htmlTagToken :
						switch(curDirective) {
							case htmlHTMLDir :
								++htmlCount;
								break;
							case htmlXHTMLDir :
								++xHtmlCount;
								break;
							case htmlPETEDir :
								goto DoPETEStuff;
							case htmlMetaDir :
								err = HTMLParseMeta(&htmlContext, &htmlToken, htmlOffset, encoding == kTextEncodingUnknown ? &converter : nil, &a, string);
								break;
							case htmlHeadDir :
								inHead = true;
								break;
							case htmlBaseDir :
								if(!inHead && (base != nil)) break;
								err = HTMLParseBase(&htmlContext, &htmlToken, htmlOffset, &base, &a, string);
								break;
							case htmlStyleDir :
							case htmlXStyleDir :
								if(!inHead) break;
								err = HTMLParseStyle(&htmlContext, &htmlToken, htmlOffset, curDirective, &pSpaces, &bqSpaces, &listSpaces, &a, string);
								break;
							case htmlTableDir :
								if (PrefIsSet(PREF_HTML_TABLES))
								{
									if(inHead) break;
									err = HTMLParseTable(&htmlContext, &htmlToken, htmlOffset, pte, offset, &matrix, &paraContext, converter.flags, &a, string);
								}
								break;
							case htmlColGroupDir :
								if(matrix) (**matrix).colGroup = true;
							case htmlColDir :
							case htmlCaptionDir :
								if(!matrix || ((**matrix).row >= 0)) break;
								goto CellParse;
							case htmlTHeadDir :
								if(!matrix || ((**matrix).rowGroup > 0)) break;
								(**matrix).rowGroup = tableHeadRowGroup;
								goto CellParse;
							case htmlTFootDir :
								if(!matrix || ((**matrix).rowGroup > 0)) break;
								(**matrix).rowGroup = tableFootRowGroup;
								goto CellParse;
							case htmlTBodyDir :
								if(!matrix) break;
								if((**matrix).rowGroup < 0)
									(**matrix).rowGroup = 1;
								else
									(**matrix).rowGroup++;
								goto CellParse;
							case htmlTDDir :
							case htmlTHDir :
								if(matrix && ((**matrix).row < 0))
									(**matrix).row = 0;
							case htmlTRDir :
								if(!matrix) break;
								err = HTMLPopCell(pte, &offset, matrix, &paraContext, &pse, &converter.flags);
								if(err) break;
						CellParse :
								err = HTMLParseCell(&htmlContext, &htmlToken, htmlOffset, curDirective, pte, offset, &converter.flags, matrix, &a, string);;
								break;
							case htmlBRDir :
								if(inHead) break;
								needWS = wasCR = false;
								++nlCount;
								brLastTag = true;
								err = HTMLInsertUniChar(pte, &offset, 13, &pse, &chars, &charsStyle, string, &converter, dontAddWordSpace, nil);
								break;
							case htmlXTabDir :
								if(inHead) break;
								needWS = wasCR = false;
								inTab = true;
								err = HTMLInsertUniChar(pte, &offset, 9, &pse, &chars, &charsStyle, string, &converter, dontAddWordSpace, nil);
								break;
							case htmlEmbedDir :
							case htmlObjectDir :
							case htmlImgDir :
								err = HTMLFlushText(pte, &offset, &chars, &pse, &charsStyle, string, &converter, false);
								if(err) break;
								err = HTMLParseImage(&htmlContext, &htmlToken, htmlOffset, pte, &offset, base, src, id, &pse, &a, string, partRefStack);
								break;
							case htmlHRDir:
								if(inHead) break;
								nlCount = 2;
								err = HTMLFlushText(pte, &offset, &chars, &pse, &charsStyle, string, &converter, true);
								if(err) break;
								err = HTMLInsertHR(&htmlContext, &htmlToken, htmlOffset, pte, &offset, &a, string);
								break;
							case htmlADir :
								if(inHead) break;
								err = HTMLInsertText(pte, &offset, 0L, nil, 0L, (StringPtr)-1L, &pse, &chars, &charsStyle, string, &converter, DoAddWordSpace((needWS && (nlCount == 0)), wasCR), &spaceStyle);
								if(!err) {
									needWS = wasCR = false;
									nlCount = 0L;
									err = HTMLDumpText(pte, offset, &chars, &charsStyle, false);
									if(!err) {
										err = HTMLStartLink(&htmlContext, &htmlToken, htmlOffset, curDirective, fontStack, pte, &pse, &converter.flags, &offset, base, src, &a, string);
									}
								}
								break;
							case htmlFontDir:
							case htmlBigDir :
							case htmlSmallDir :
							case htmlTTDir :
								if(inHead) break;
								err = HTMLChangeFont(&htmlContext, &htmlToken, htmlOffset, curDirective, basefont, fontStack, &pse, &converter.flags, validMask, &a, string);
								break;
							case htmlEmDir :
							case htmlIDir :
								if(inHead || (!(validMask & italic))) break;
								++italicCount;
								pse.psStyle.textStyle.tsFace |= italic;
								break;
							case htmlUDir :
								if(inHead || (!(validMask & underline))) break;
								++underlineCount;
								pse.psStyle.textStyle.tsFace |= underline;
								break;
							case htmlH1Dir :
							case htmlH2Dir :
							case htmlH3Dir :
							case htmlH4Dir :
							case htmlH5Dir :
							case htmlH6Dir :
								justBold = false;
							case htmlStrongDir :
							case htmlBDir :
								if(inHead) break;
								if(validMask & bold) {
									++boldCount;
									pse.psStyle.textStyle.tsFace |= bold;
								}
								if(justBold) break;
							case htmlULDir :
							case htmlOLDir :
							case htmlDLDir :
							case htmlPDir :
							case htmlDivDir :
							case htmlCenterDir :
							case htmlBQDir :
							case htmlPreDir :
							case htmlXmpDir :
							case htmlListDir :
								if(inHead) break;
							case htmlBodyDir :
								inHead = false;
								err = HTMLChangePara(&htmlContext, &htmlToken, htmlOffset, curDirective, offset, &paraContext, &pse, &converter.flags, validMask, validParaMask, pte, &a, string);
								break;
							case htmlDDDir :
								err = HTMLInsertUniChar(pte, &offset, (paraContext.listType & htmlCompactListBit) ? 9 : 13, &pse, &chars, &charsStyle, string, &converter, dontAddWordSpace, nil);
								break;
							case htmlLIDir :
								err = HTMLInsertListItem(&htmlContext, &htmlToken, htmlOffset, &offset, &paraContext, &pse, pte, &a, string);
						}
						break;
					case htmlTagNotToken :
						switch(curDirective) {
							case htmlHTMLDir :
								--htmlCount;
								if((htmlCount > 0L) || (xHtmlCount > 0L)) {
									break;
								}
							case htmlXHTMLDir :
								--xHtmlCount;
								if(xHtmlCount <= 0L) {
									while((htmlToken.tokenType != htmlTagEndToken) &&
							              (htmlToken.tokenType != htmlEndToken)) {
										*htmlOffset += htmlToken.tokenLen;
										HTMLGetNextToken(&htmlContext, &htmlToken);
									}
									htmlToken.tokenType = htmlEndToken;
								}
								break;
							case htmlHeadDir :
								inHead = false;
								break;
							case htmlXTabDir :
								inTab = false;
								break;
							case htmlTableDir :
								if(!matrix) break;
								err = HTMLPopTable(pte, &offset, &matrix, &paraContext, &pse, &converter.flags);
								break;
							case htmlColGroupDir :
								if(matrix) (**matrix).colGroup = false;
								break;
							case htmlTBodyDir :
							case htmlTFootDir :
							case htmlTHeadDir :
								if(matrix) (**matrix).rowGroup = tableNoRowGroup;
								break;
							case htmlADir :
							case htmlFontDir :
							case htmlBigDir :
							case htmlSmallDir :
							case htmlTTDir :
								if(inHead) break;
								err = HTMLPopFont(fontStack, curDirective, &pse, &converter.flags);
								break;
							case htmlEmDir :
							case htmlIDir :
								if(inHead) break;
								if(italicCount && !--italicCount) {
									pse.psStyle.textStyle.tsFace &= ~italic;
								}
								break;
							case htmlUDir :
								if(inHead) break;
								if(underlineCount && !--underlineCount) {
									pse.psStyle.textStyle.tsFace &= ~underline;
								}
								break;
							case htmlH1Dir :
							case htmlH2Dir :
							case htmlH3Dir :
							case htmlH4Dir :
							case htmlH5Dir :
							case htmlH6Dir :
								justBold = false;
							case htmlStrongDir :
							case htmlBDir :
								if(inHead) break;
								if(boldCount && !--boldCount) {
									pse.psStyle.textStyle.tsFace &= ~bold;
								}
								if(justBold) break;
							case htmlULDir :
							case htmlOLDir :
							case htmlDLDir :
							case htmlPDir :
							case htmlDivDir :
							case htmlCenterDir :
							case htmlBQDir :
							case htmlPreDir :
							case htmlXmpDir :
							case htmlListDir :
//							case htmlBodyDir :
								if(inHead) break;
								err = HTMLPopPara(curDirective, offset, &paraContext, &pse, &converter.flags, pte);
						}
						break;
					case htmlCommentToken :
						switch(curDirective) {
							case htmlPETEDir :
						DoPETEStuff :
								err = HTMLParsePETEStuff(&htmlContext, &htmlToken, htmlOffset, &base, &src, &id, encoding == kTextEncodingUnknown ? &converter : nil, &a, string);
						}
						
				}
				while((htmlToken.tokenType != htmlTagEndToken) &&
				      (htmlToken.tokenType != htmlEndToken)) {
					*htmlOffset += htmlToken.tokenLen;
					HTMLGetNextToken(&htmlContext, &htmlToken);
				}
		}
		*htmlOffset += htmlToken.tokenLen;
		
		CycleBalls();
		if(!err && CommandPeriod) {
			err = userCanceledErr;
		}
	} while((htmlToken.tokenType != htmlEndToken) && (err == noErr));
	
	if(!err) {
		err = HTMLFlushText(pte, &offset, &chars, &pse, &charsStyle, string, &converter, false);
	}
	
	while(matrix)
	{
		HTMLPopTable(pte, &offset, &matrix, &paraContext, &pse, &converter.flags);
	}
	
	PETESetRecalcState(PETE,pte,docInfo.recalcOn);
	
	*inOffset = offset - chars.offset;
	AccuZap(chars);
	AccuZap(a);
CleanParaStack :
	ZapHandle(paraContext.paraStack);
CleanFontStack :
	ZapHandle(fontStack);
CleanConverter :
	DisposeIntlConverter(converter);
	return err;
}

/* HTMLInitTokenState - Just set everything up and get the first characters */
void HTMLInitTokenState(HTMLTokenContext *htmlContext, Handle htmlText, long htmlOffset, long htmlLen)
{
	htmlContext->hSize = htmlLen + htmlOffset;
	htmlContext->html = htmlText;
	htmlContext->used = htmlOffset;
	htmlContext->tStart = htmlOffset;
	htmlContext->state = htmlTextState;
	HTMLAdvanceChar(htmlContext);
}

/* HTMLGetNextToken - return the next token type and length in htmlToken given htmlContext */
void HTMLGetNextToken(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken)
{
#ifdef DEBUG
	*htmlToken->dbTokenStr = 0;
#endif
	/* Whoops! Ran out of tokens */
	if(htmlContext->state == htmlEndState) {
		htmlToken->tokenType = htmlEndToken;
		return;
	}

	while(true) {
		switch(htmlContext->state) {
			case htmlTagEndState :
				HTMLSetToken(htmlContext, htmlToken, (HTMLTokenType)htmlContext->state, false, false);
				htmlContext->state = htmlTextState;
				return;
			case htmlTextState :
				switch(htmlContext->curToken) {
					/* Ampersand - see if the next two characters make it a literal */
					case tokenAmpersand :
						if(htmlContext->secondToken == tokenAlpha) {
							htmlContext->state = htmlLiteralState;
						} else if(htmlContext->secondToken == tokenHash) {
							if(htmlContext->thirdToken == tokenNumeric)
								htmlContext->state = htmlLiteralNumState1;
							else if(htmlContext->thirdToken == tokenAlpha)
								htmlContext->state = htmlLiteralHexState;
						}
						break;
					/* Less than - see if the next character makes it a tag */
					case tokenLess :
						switch(htmlContext->secondToken) {
							case tokenQuestion :
								htmlContext->state = htmlProcessState1;
								break;
							case tokenExclam :
								htmlContext->state = htmlCommentState1;
								break;
							case tokenSlash :
								htmlContext->state = htmlTagNotState1;
								break;
							case tokenAlpha :
							case tokenNumeric :
							case tokenPeriod :
							case tokenMinus :
								htmlContext->state = htmlTagState;
						}
						break;
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
				}
				/* We may have come in from an initialization state, so make sure we've got something to add */
				if((htmlContext->state != htmlTextState) && (htmlContext->tStart != htmlContext->used - 1L)) {
					HTMLSetToken(htmlContext, htmlToken, htmlTextToken, false, true);
					return;
				}
				break;
			case htmlProcessState1 :
				htmlContext->state = htmlProcessState;
				break;
			case htmlCommentState1 :
				if((htmlContext->secondToken == tokenMinus) && (htmlContext->thirdToken == tokenMinus)) {
					htmlContext->state = htmlCommentState2;
				} else {
					htmlContext->state = htmlCommentState;
				}
				break;
			case htmlCommentState2 :
				HTMLSetToken(htmlContext, htmlToken, htmlCommentToken, false, true);
				htmlContext->state = htmlCommentState3;
				return;
			case htmlCommentState3 :
				switch(htmlContext->curToken) {
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						HTMLSetToken(htmlContext, htmlToken, htmlStrayToken, false, false);
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
						return;
					case tokenMinus :
						if(htmlContext->secondToken == tokenMinus) {
							if((htmlContext->thirdToken == tokenGreat) || (htmlContext->thirdToken == tokenWhite) || (htmlContext->thirdToken == tokenNewLine)) {
								htmlContext->state = htmlCommentState4;
								break;
							}
						}
						break;
				}
				break;
			case htmlCommentState4 :
				htmlContext->state = htmlCommentState5;
				break;
			case htmlCommentState5 :
				switch(htmlContext->curToken) {
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						HTMLSetToken(htmlContext, htmlToken, htmlStrayToken, false, true);
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
						return;
					case tokenGreat :
						if(htmlContext->tStart != htmlContext->used - 1L) {
							HTMLSetToken(htmlContext, htmlToken, htmlStrayToken, false, true);
						}
						htmlContext->state = htmlTagEndState;
						return;
					default :
						htmlContext->state = htmlCommentState3;
						continue;
				}
				break;
			case htmlTagNotState1 :
				htmlContext->state = htmlTagNotState;
				break;
			case htmlStrayState :
				switch(htmlContext->curToken) {
					case tokenGreat :
						htmlContext->state = htmlTagEndState;
						break;
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
						break;
					case tokenAlpha :
					case tokenNumeric :
					case tokenPeriod :
					case tokenMinus :
						htmlContext->state = htmlNameState;
				}
				if(htmlContext->state != htmlStrayState) {
					HTMLSetToken(htmlContext, htmlToken, htmlStrayToken, false, true);
					return;
				}
				break;
			case htmlNameState :
				switch(htmlContext->curToken) {
					case tokenAlpha :
					case tokenNumeric :
					case tokenPeriod :
					case tokenMinus :
						goto AdvanceAndLoop;
				}
			case htmlCommentState :
			case htmlTagNotState :
			case htmlProcessState :
			case htmlTagState :
				HTMLSetToken(htmlContext, htmlToken, (HTMLTokenType)htmlContext->state, false, false);
				htmlContext->state = htmlInTagState;
				return;
			case htmlInTagState :
				switch(htmlContext->curToken) {
//					case token1Quote :
//					case token2Quote :
//						htmlContext->stringToken = htmlContext->curToken;
//						htmlContext->state = htmlStringState;
//						break;
					case tokenGreat :
						htmlContext->state = htmlTagEndState;
						break;
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
						break;
					case tokenEqual :
						htmlContext->state = htmlAttrState;
						break;
					case tokenAlpha :
					case tokenNumeric :
					case tokenPeriod :
					case tokenMinus :
						htmlContext->state = htmlNameState;
						break;
					default :
						htmlContext->state = htmlStrayState;
				}
				break;
			case htmlAttrState :
				HTMLSetToken(htmlContext, htmlToken, (HTMLTokenType)htmlContext->state, false, false);
				switch(htmlContext->curToken) {
					case token1Quote :
					case token2Quote :
						htmlContext->stringToken = htmlContext->curToken;
						htmlContext->state = htmlStringState;
						break;
					case tokenGreat :
						htmlContext->state = htmlTagEndState;
						break;
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						htmlContext->wsState = htmlContext->state;
						htmlContext->state = htmlWSState;
						break;
					default :
						htmlContext->stringToken = tokenUnknown;
						htmlContext->state = htmlStringState;
						return;
				}
				HTMLAdvanceChar(htmlContext);
				return;
			case htmlStringState :
				switch(htmlContext->curToken) {
					case token1Quote :
					case token2Quote :
						if(htmlContext->stringToken == htmlContext->curToken) {
							htmlContext->state = htmlInTagState;
						}
						break;
					case tokenGreat :
						if(htmlContext->stringToken == tokenUnknown) {
							htmlContext->state = htmlTagEndState;
						}
						break;
					case tokenNewLine :
						if(true) {
							htmlContext->wsHasNewLine = true;
						} else
					case tokenWhite :
						{
							htmlContext->wsHasNewLine = false;
						}
						if(htmlContext->stringToken == tokenUnknown) {
							htmlContext->wsState = htmlInTagState;
							htmlContext->state = htmlWSState;
						}
						break;
					case tokenAmpersand :
						if(htmlContext->secondToken == tokenAlpha) {
							htmlContext->state = htmlLiteralQuoteState;
						} else if(htmlContext->secondToken == tokenHash) {
							if(htmlContext->thirdToken == tokenNumeric)
								htmlContext->state = htmlLiteralNumQuoteState1;
							else if(htmlContext->thirdToken == tokenAlpha)
								htmlContext->state = htmlLiteralHexQuoteState;
						}
				}
				if(htmlContext->state != htmlStringState) {
					HTMLSetToken(htmlContext, htmlToken, htmlStringToken, (htmlContext->state == htmlInTagState), true);
					return;
				}
				break;
			case htmlLiteralHexState :
				htmlContext->state = htmlLiteralState;
				break;
			case htmlLiteralHexQuoteState :
			case htmlLiteralNumQuoteState1 :
			case htmlLiteralNumState1 :
				++htmlContext->state;
				break;
			case htmlLiteralQuoteState :
			case htmlLiteralState :
				if(htmlContext->curToken == tokenAlpha) {
					break;
				}
			case htmlLiteralNumQuoteState2 :
			case htmlLiteralNumState2 :
				if(htmlContext->curToken == tokenNumeric) {
					break;
				}
				htmlContext->state = (((htmlContext->state == htmlLiteralNumState2) || (htmlContext->state == htmlLiteralState)) ? htmlTextState : htmlStringState);
				HTMLSetToken(htmlContext, htmlToken, htmlLiteralToken, (htmlContext->curToken == tokenSemicolon), (htmlContext->curToken == tokenSemicolon));
				return;
			case htmlWSState :
				switch(htmlContext->curToken) {
					case tokenNewLine :
						htmlContext->wsHasNewLine = true;
					case tokenWhite :
						break;
					default :
						switch(htmlContext->wsState) {
							case htmlCommentState3 :
							case htmlCommentState5 :
							case htmlTextState :
								htmlContext->state = htmlContext->wsState;
								break;
							case htmlAttrState :
								switch(htmlContext->curToken) {
									case tokenGreat :
										htmlContext->state = htmlTagEndState;
										break;
									case token1Quote :
									case token2Quote :
										htmlContext->state = htmlStringState;
										htmlContext->stringToken = htmlContext->curToken;
										break;
									default :
										htmlContext->state = htmlStringState;
										htmlContext->stringToken = tokenUnknown;
								}
								break;
							default :
								htmlContext->state = htmlInTagState;
						}
						HTMLSetToken(htmlContext, htmlToken, htmlWSToken, false, (htmlContext->wsState == htmlAttrState));
						return;
				}
				break;
		}
	AdvanceAndLoop :
		if(htmlContext->secondToken == tokenEmpty) {
			break;
		}
		HTMLAdvanceChar(htmlContext);
	}

	if(htmlContext->tStart < htmlContext->hSize) {
		switch(htmlContext->state) {
			case htmlTextState :
			case htmlStringState :
			case htmlLiteralState :
			case htmlWSState :
			case htmlNameState :
			case htmlAttrState :
			case htmlTagState :
			case htmlTagNotState :
			case htmlTagEndState :
			case htmlCommentState :
			case htmlProcessState :
				break;
			default :
				htmlContext->state = htmlStrayState;
		}
		htmlContext->used = htmlContext->hSize;
		HTMLSetToken(htmlContext, htmlToken, (HTMLTokenType)htmlContext->state, true, false);
	}
	htmlContext->state = htmlEndState;
}

void HTMLGetTokenForXMP(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, HTMLDirectiveEnum curDirective, StringPtr string)
{
	long endOffset;
	Ptr endLoc;
	Byte hState;
	
	if(curDirective == htmlPlainDir) {
		endLoc = nil;
	} else {
		string[0] = 2;
		string[1] = '<';
		string[2] = '/';
		PCatR(string, HTMLDirectiveStrn+curDirective);
		hState = HGetState(htmlContext->html);
		HLock(htmlContext->html);
		endLoc = PPtrFindSub(string, *htmlContext->html + (htmlContext->used - 1L), htmlContext->hSize - htmlContext->used - 1L);
		endOffset = (unsigned long)endLoc - (unsigned long)*htmlContext->html;
		HSetState(htmlContext->html, hState);
	}
	if((endLoc == nil) || (endOffset < 0)) {
		htmlToken->tokenType = htmlTextToken;
		htmlToken->tokenLen = htmlContext->hSize - htmlContext->tStart;
		htmlContext->used = htmlContext->tStart = htmlContext->hSize;
		htmlContext->curToken = htmlContext->secondToken = htmlContext->thirdToken = tokenEmpty;
		htmlContext->state = htmlEndState;
	} else {
		htmlContext->used = endOffset;
		HTMLSetToken(htmlContext, htmlToken, htmlTextToken, false, (endOffset > htmlContext->tStart));
		htmlContext->state = htmlTextState;
	}
}

void HTMLSetToken(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, HTMLTokenType type, Boolean previousChar, Boolean advance)
{
	htmlToken->tokenType = type;
	htmlToken->tokenLen = htmlContext->used - htmlContext->tStart;
	htmlContext->tStart = htmlContext->used;
#ifdef DEBUG
	MakePStr(htmlToken->dbTokenStr,*htmlContext->html+htmlContext->tStart,htmlToken->tokenLen);
#endif
	if(!previousChar) {
		--htmlToken->tokenLen;
		--htmlContext->tStart;
	}
	if(advance) {
		HTMLAdvanceChar(htmlContext);
	}
}

void HTMLAdvanceChar(HTMLTokenContext *htmlContext) {
	Byte *chars;
	
	chars = &(*((Byte **)htmlContext->html))[htmlContext->used++];
	htmlContext->curToken = CharToToken(*chars++);
	if(htmlContext->hSize > htmlContext->used) {
		htmlContext->secondToken = CharToToken(*chars++);
		if(htmlContext->hSize <= htmlContext->used + 1L) {
			goto EmptyThird;
		}
		htmlContext->thirdToken = CharToToken(*chars);
	} else {
		htmlContext->secondToken = tokenEmpty;
	EmptyThird :
		htmlContext->thirdToken = tokenEmpty;
	}
}

/* Get the resource for HTML literals and make an array of offsets into that table */
OSErr HTMLGetLiteralHandles(Byte *rState, Byte *oState)
{
	OSErr errCode;
	UPtr p, b;
	short i, n;
	
	/* If they are already loaded up, just get the states and make them non-purgeable */
	if((litResH != nil) && (*litResH != nil)) {
		*rState = HGetState((Handle)litResH);
		HNoPurge((Handle)litResH);
	}
	if((litOffsets != nil) && (*litOffsets != nil)) {
		*oState = HGetState((Handle)litOffsets);
		HNoPurge((Handle)litOffsets);
	}
	
	/* If the resource is not loaded or purged, reload it */
	if((litResH == nil) || (*litResH == nil)) {
		if(litResH == nil) {
			litResH = (struct LiteralResource **)GetIndResource_('HLIT',1);
		} else {
			LoadResource((Handle)litResH);
		}
		/* If the resource can't be loaded, might as well get rid of the offsets */
		if(((litResH == nil) && ((errCode = resNotFound), true)) || ((errCode = ResError()) != noErr)) {
			ZapHandle(litOffsets);
			return errCode;
		}
		/* Get the resource state and set it to not purgeable */
		*rState = HGetState((Handle)litResH);
		HNoPurge((Handle)litResH);
	}
	
	/* If the offsets array hasn't been created or is purged, get it */
	if((litOffsets == nil) || (*litOffsets == nil)) {
		if(litOffsets == nil) {
			litOffsets = (short **)NuHTempBetter((**litResH).numEntries * sizeof(short));
		} else {
			ReallocateHandle((Handle)litOffsets, (**litResH).numEntries * sizeof(short));
		}
		
		/* If the memory isn't there to make the array, might as well make the resource purgeable */
		if((errCode = MemError()) != noErr) {
			HSetState((Handle)litResH, *rState);
			return errCode;
		}
		
		/* Handle state is going to be non-purgeable, so get the full state and set the purgeable bit */
		*oState = HGetState((Handle)litOffsets);
		*oState |= 0x40;
		
		/* Loop through the resource and make an array of offsets */
		for(i = 0, n = (**litResH).numEntries, b = p = &(**litResH).entries[0]; i < n; ++i) {
			(*litOffsets)[i] = (long)p - (long)b;
			p += sizeof(LiteralEntry) + ((LiteralPtr)p)->literal[0] + 1;
		}
	}
	return noErr;
}

UniChar HTMLLiteral(Ptr literal, long len)
{
	UPtr newLit;
	long newLen;
	Byte rState, oState;
	UniChar value = 0xFFFF;
	
	newLit = literal;
	newLen = len;
	
	if(newLen >= sizeof(Str255)) newLen = sizeof(Str255) - 1;
	
	/* Trim any trailing ';' */
	if(newLit[newLen - 1] == ';') {
		--newLen;
	}
	
	/* Skip the '&' */
	--newLen;
	++newLit;

	/* Special case nbsp */
	if((newLen == 4) && ((*(long *)newLit) == 'nbsp')) {
		return kUnicodeNonBreakingSpace;
	/* Load up the tables */
	} else if((newLen > 0) && (HTMLGetLiteralHandles(&rState, &oState) == noErr)) {
		short index;
		Ptr basePtr = &(**litResH).entries[0];
		LiteralPtr entry;
		
		/* Numeric literal if it starts '#' */
		if(*newLit == '#') {

			value = 0;

			/* Go by the '#' */
			--newLen;
			++newLit;
			
			/* Add up the digits */
			if(*newLit == 'x' || *newLit == 'X') {
			
#define Hex2Nyb(c) (c<='9'?c-'0':(c>='a'?c-'a'+10:c-'A'+10))
				/* Go by the 'X' */
				++newLit;
				--newLen;
				
				while(--newLen >= 0) {
					value *= 16;
					value += Hex2Nyb(*newLit);
					++newLit;
				}
			} else {
			
				while(--newLen >= 0) {
					value *= 10;
					value += *newLit++ - '0';
				}
			}
			
			if(value >= 0x0080 && value <= 0x009F) {
	 			/* Look for the value in the table */
	 			for(index = (**litResH).numEntries; --index >= 0; ) {
	 				entry = (LiteralPtr)(basePtr + (*litOffsets)[index]);
	 				if(entry->unicode == value) {
						newLit = entry->literal;
						newLen = *newLit++;
						if(newLen > 0) {
							goto DoStringLiteral;
						} else {
		 					break;
		 				}
	 				}
	 			}
			}
		/* It's a string literal */
		} else  {
	DoStringLiteral :
			/* Look for the string in the table */
			for(index = 0; index < (**litResH).numEntries; ++index) {
				entry = (LiteralPtr)(basePtr + (*litOffsets)[index]);
				if(UPtrIsChar(newLit, newLen, entry->literal)) {
					value = entry->unicode;
					break;
				}
			}
		}
	}
	
	/* Set the handles back to purgeable */
	if(litResH != nil) {
		HSetState((Handle)litResH, rState);
	}
	if(litOffsets != nil) {
		HSetState((Handle)litOffsets, oState);
	}
	return value;
}

TokenType CharToToken(unsigned short c)
{
	switch(c) {
		case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' : case 'G' : case 'H' : case 'I' : case 'J' : case 'K' : case 'L' : case 'M' : case 'N' : case 'O' : case 'P' : case 'Q' : case 'R' : case 'S' : case 'T' : case 'U' : case 'V' : case 'W' : case 'X' : case 'Y' : case 'Z' :
		case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' : case 'g' : case 'h' : case 'i' : case 'j' : case 'k' : case 'l' : case 'm' : case 'n' : case 'o' : case 'p' : case 'q' : case 'r' : case 's' : case 't' : case 'u' : case 'v' : case 'w' : case 'x' : case 'y' : case 'z' :
			return tokenAlpha;
		case '0' : case '1' : case '2' : case '3' : case '4' : case '5' : case '6' : case '7' : case '8' : case '9' :
			return tokenNumeric;
		case '.' :
			return tokenPeriod;
		case '-' :
			return tokenMinus;
		case '?' :
			return tokenQuestion;
		case '!' :
			return tokenExclam;
		case '/' :
			return tokenSlash;
		case 13 :
		case 10 :
			return tokenNewLine;
		case ' ' :
			return tokenWhite;
		case '&' :
			return tokenAmpersand;
		case '<' :
			return tokenLess;
		case '>' :
			return tokenGreat;
		case '=' :
			return tokenEqual;
		case '\'' :
			return token1Quote;
		case '"' :
			return token2Quote;
		case ';' :
			return tokenSemicolon;
		case '#' :
			return tokenHash;
		case ',' :
			return tokenComma;
		case ':' :
			return tokenColon;
		case '{' :
			return tokenLeftCurly;
		case '}' :
			return tokenRightCurly;
		default :
			return tokenUnknown;
	}
}

OSErr HTMLParsePETEStuff(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, Handle *base, Handle *src, long *id, IntlConverter *converter, AccuPtr a, StringPtr string)
{
	OSErr err;
	HTMLAttributeEnum attr;
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlIDAttr :
				*id = HTMLAccuStringToNum(a, string);
				break;
			case htmlBaseAttr :
				HTMLUnquoteAndTrim(a);
				if (a->offset)
				{
					if(*base != nil) {
						ZapHandle(*base);
					}
					*base = a->data;
					Zero(*a);
				}
				break;
			case htmlSrcAttr :
				if(*src != nil) {
					ZapHandle(*src);
				}
				HTMLUnquoteAndTrim(a);
				if (a->offset)
				{
					*src = a->data;
					Zero(*a);
				}
				break;
			case htmlCharsetAttr :
				if(converter != nil) {
					HTMLCopyQuoteString(*a->data, a->offset, string);
					UpdateIntlConverter(converter, string);
				}
		}
	}
	return (err == errEndOfBody) ? noErr : err;
}

OSErr HTMLParseBase(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, Handle *base, AccuPtr a, StringPtr string)
{
	OSErr err;
	HTMLAttributeEnum attr;
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlHrefAttr :
				HTMLUnquoteAndTrim(a);
				if (a->offset)
				{
					if(*base != nil) {
						ZapHandle(*base);
					}
					*base = a->data;
					Zero(*a);
				}
		}
	}
	return (err == errEndOfBody) ? noErr : err;
}

OSErr HTMLParseStyle(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, Boolean *pSpaces, Boolean *bqSpaces, Boolean *listSpaces, AccuPtr a, StringPtr string)
{
	short i, styleSheets[3] = {HTML_NOSPACESTYLE, HTML_NOSPACESTYLE1, HTML_NOSPACESTYLE2};
	Str255 s;
	Str63 sWord;
	UPtr spot;
	
	*htmlOffset += htmlToken->tokenLen;
	while((htmlToken->tokenType != htmlTagEndToken) &&
	      (htmlToken->tokenType != htmlEndToken)) {
		HTMLGetNextToken(htmlContext, htmlToken);
		*htmlOffset += htmlToken->tokenLen;
	}
	HTMLGetTokenForXMP(htmlContext, htmlToken, curDirective, string);
	for(i = 0; i < sizeof(styleSheets) / sizeof(short); ++i) {
		GetRString(string, styleSheets[i]);
		if(PPtrFindSub(string, *(htmlContext->html) + *htmlOffset, htmlToken->tokenLen) != nil) {
			*bqSpaces = *listSpaces = false;
			break;
		}
	}
	
	/*
	 * Outlook--it sucks
	 * 
	 * Outlook likes to use the <P> directive, but to then override the normal
	 * space-after behavior with a style sheet.  As we don't do style sheets,
	 * this makes the html look funny.  Detect Outlook's trick, and disable spaces
	 * after <P> directives
	 */
	*pSpaces = true;
	GetRString(s,WARNING_SIGNS_YOU_MIGHT_HAVE_OUTLOOK);
	spot = s+1;
	while (*pSpaces && PToken(s,sWord,&spot,","))
		*pSpaces = !PPtrFindSub(sWord, *(htmlContext->html) + *htmlOffset, htmlToken->tokenLen);
	
	return noErr;
}

OSErr HTMLParseMeta(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, IntlConverter *converter, AccuPtr a, StringPtr string)
{
	Boolean gotContentType = false;
	Handle content = nil;
	HTMLAttributeEnum attr;
	OSErr err;
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlHTTPEquivAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				if(FindSTRNIndex(InterestHeadStrn, string) == hContentType) {
					gotContentType = true;
				}
				break;
			case htmlContentAttr :
				HTMLUnquoteAndTrim(a);
				if (a->offset)
				{
					content = a->data;
					Zero(*a);
				}
		}
	}
	if(err == errEndOfBody)
		err = noErr;
	else if(err)
		goto done;
	
	if(converter != nil && content != nil && gotContentType) {
		HeaderDHandle hdh;

		GetRString(string, InterestHeadStrn+hContentType);
		string[++string[0]] = ':';
		string[++string[0]] = ' ';
		Munger(content, 0L, nil, 0L, string + 1, string[0]);
		if((err = MemError()) != noErr) goto done;
		if((err = ParseAHeader(content, &hdh)) != noErr) goto done;
		if(AAFetchResData((*hdh)->contentAttributes, AttributeStrn + aCharSet, string) == noErr) {
			// if alreadyTransliterated is true, we (for now) ignore charsets in META tags.  See UpdateIntlConverter comment
			if (converter && !converter->alreadyTransliterated) UpdateIntlConverter(converter, string);
		}
		DisposeHeaderDesc(hdh);
	}
	
done :
	ZapHandle(content);
	return err;
}

OSErr HTMLParseImage(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, 
	long *offset, Handle base, Handle src, long id, PETEStyleEntry *pse, AccuPtr a, StringPtr string,StackHandle partRefStack)
{
	OSErr err;
	HTMLAttributeEnum attr;
	Handle	alt = nil;
	Str255	sURL;
	HTMLGraphicInfo	graphicInfo;
	PETEStyleEntry temppse;
	
	Zero(graphicInfo);
	*sURL = 0;
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
	
		// Malformed HTML?
		if (!a->offset) continue;
		
		switch(attr) {
			case htmlDataAttr :
			case htmlSrcAttr :
				HTMLCopyQuoteString(*a->data, a->offset, sURL);
				break;
			case htmlAltAttr :
					HTMLUnquoteAndTrim(a);
					if (a->offset)
					{
						alt = a->data;
						Zero(*a);
					}
				break;
			case htmlAlignAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				graphicInfo.align = (HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string);
				if(graphicInfo.align > htmlRightAlign) graphicInfo.align = 0;
				break;
			case htmlHeightAttr :
				graphicInfo.height = HTMLAccuStringToNum(a, string);
				break;
			case htmlWidthAttr :
				graphicInfo.width = HTMLAccuStringToNum(a, string);
				break;
			case htmlBorderAttr :
				graphicInfo.border = HTMLAccuStringToNum(a, string);
				break;
			case htmlHSpaceAttr :
				graphicInfo.hSpace = HTMLAccuStringToNum(a, string);
				break;
			case htmlVSpaceAttr :
				graphicInfo.vSpace = HTMLAccuStringToNum(a, string);
			
		}
	}
	
	if(err == errEndOfBody)
		err = noErr;
	else if(err)
		goto done;
	
	if (*sURL)
	{
		FSSpec	spec;
		Boolean	localFile = false;
		StringPtr colon;
		uLong colonOffset;
		short index;
		MessHandle	messH;

		// If there's a ':', get the scheme in index and the contents in string		
		if((colon = PIndex(sURL, ':')) != nil) {
			colonOffset = ((uLong)colon) - ((uLong)sURL);
			NewPStr(&sURL[1], colonOffset - 1, string);
			index = FindSTRNIndex(ProtocolStrn,string);
			NewPStr(colon + 1, sURL[0] - colonOffset, string);
		} else {
			index = 0;
		}
		switch(index) {
			case proCID :
				graphicInfo.cid = GetURLHash(string,id);
				if (partRefStack) StackPush(&graphicInfo.cid,partRefStack);
				break;
			case proCompFile :
				if (messH = Pte2MessH(pte))
				{
					err = MakeAttSubFolder(messH, SumOf(messH)->uidHash,&spec);
					if(err) goto done;
					FixURLString(string);
					string[0] = MIN(string[0],sizeof(spec.name) - 1);
					PCopy(spec.name, string);
					localFile = true;
				}
				break;
			case proPictRes :
				if(string[0])
				{
					long num;
					
					StringToNum(string, &num);
					graphicInfo.pictResID = num;
				}
				break;
			case proPictHandle :
				if(string[0])
				{
					long num;
					
					StringToNum(string, &num);
					graphicInfo.pictHandle = (PicHandle)num;
				}
				break;
			default :
				//	No cid or x-eudora-file. Try raw URL
				PCopy(string,sURL);
				graphicInfo.url = GetURLHash(string,id);
				if (partRefStack) StackPush(&graphicInfo.url,partRefStack);
			
				//	Add to base
				if (base)
				{
					NewPStr(*base,InlineGetHandleSize(base),string);
					URLCombine(string,string,sURL);
					graphicInfo.withBase = GetURLHash(string,id);
					if (partRefStack) StackPush(&graphicInfo.withBase,partRefStack);
					graphicInfo.absURL = NewString(string);
				}
				
				//	Add to source
				if (src)
				{
					NewPStr(*src,InlineGetHandleSize(src),string);
					URLCombine(string,string,sURL);
					graphicInfo.withSource = GetURLHash(string,id);
					if (partRefStack) StackPush(&graphicInfo.withSource,partRefStack);
				}
				
				if(graphicInfo.absURL == nil) {
					graphicInfo.absURL = NewString(string);
				}
		}
		
		if (alt)
		{
			//	Pass Alt string in an FSSpec
			NewPStr(*alt, InlineGetHandleSize(alt), string);
			if (*string > 63) *string = 63;
			graphicInfo.alt = string;
		}

		//	Insert into message
		temppse = *pse;
		err = PeteFileGraphicStyle(pte, localFile?&spec:nil, &graphicInfo, &temppse, (PeteIsInAdWindow(pte) || !PrefIsSet(PREF_NO_INLINE)) ?fgDisplayInline:0);
		if(!err) {
			**Pslh = temppse;
			err = PeteInsertChar(pte, *offset, ' ', Pslh);	//	Put on top of a space
			if(!err) ++*offset;
		}
	}
	
done :
	ZapHandle(alt);
	return err;
}

OSErr HTMLParseTable(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, long offset, MatrixHandle *matrix, HTMLParaContext *paraContext, OptionBits uniFlags, AccuPtr a, StringPtr string)
{
	MatrixHandle m;
	TableHandle t;
	OSErr err;
	HTMLAttributeEnum attr;
	HTMLParaChange oldPara;
	Boolean	p;
	
	err = HTMLGetParaState(pte, offset, &oldPara, nil, nil, paraContext, nil, uniFlags, htmlTableDir);
	if(err != noErr) return err;
	
	err = StackPush(&oldPara, paraContext->paraStack);
	if(err != noErr) return err;

	m = NuHTempBetter(sizeof(MatrixRec));
	if((err = MemError()) != noErr) return err;
	WriteZero(*m, sizeof(MatrixRec));
	
	t = NuHTempBetter(sizeof(TableData));
	if((err = MemError()) != noErr) goto DisposeMatrix;
	WriteZero(*t, sizeof(TableData));
	
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlAlignAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				(**t).align = (HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string);
				if((**t).align > htmlCharAlign) (**t).align = 0;
				break;
			case htmlWidthAttr :
				(**t).width = HTMLAccuStringToNumLo(a, string, &p);
				break;
			case htmlBorderAttr :
				(**t).border = HTMLAccuStringToNum(a, string);
				break;
			case htmlCellSpacingAttr :
				(**t).cellSpacing = HTMLAccuStringToNum(a, string);
				(**t).cellSpacingSpecified = true;
				break;
			case htmlCellPaddingAttr :
				(**t).cellPadding = HTMLAccuStringToNum(a, string);
				break;
		}
	}

	if(err == errEndOfBody) err = noErr;
	
	if(err)
	{
		DisposeHandle(t);
DisposeMatrix :
		DisposeHandle(m);
	}
	else
	{
		(**t).pteEdit = pte;
		(**m).row = -1;
		(**m).offset = offset;
		(**m).table = t;
		(**m).last = *matrix;
		*matrix = m;
	}
	return err;
}

OSErr HTMLPopTable(PETEHandle pte, long *offset, MatrixHandle *matrix, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags)
{
	OSErr err;
	MatrixHandle last;
	
	err = HTMLPopCell(pte, offset, *matrix, paraContext, pse, uniFlags);
	if(!err)
	{
		err = HTMLPopPara(htmlTableDir, *offset, paraContext, pse, uniFlags, pte);
		if(!err)
		{
			PETEStyleEntry tse;
			
			err = MakeTableGraphic(pte, (***matrix).offset, (***matrix).table, &tse);
			if(!err)
			{
				err = PETESetStyle(PETE, pte, (***matrix).offset, *offset, &tse.psStyle, (peAllValid & ~peColorValid) | peGraphicValid | peGraphicColorChangeValid);
			}
		}
	}
	
	if(err) TableDispose((***matrix).table);
	last = (***matrix).last;
	DisposeHandle(*matrix);
	*matrix = last;
	
	return err;
}

OSErr HTMLParseCell(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, PETEHandle pte, long offset, OptionBits *uniFlags, MatrixHandle matrix, AccuPtr a, StringPtr string)
{
	OSErr err;
	TabAlignData align = {0,0,0,0,kHilite};
	short rowSpan = 0, colSpan = 0, width = 0, height = 0;
	Boolean p = false;
	HTMLAttributeEnum attr;
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr)
	{
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlRowSpanAttr :
				rowSpan = HTMLAccuStringToNum(a, string);
				break;
			case htmlSpanAttr :
			case htmlColSpanAttr :
				colSpan = HTMLAccuStringToNum(a, string);
				break;
			case htmlWidthAttr :
				width = HTMLAccuStringToNumLo(a, string, &p);
				break;
			case htmlHeightAttr :
				height = HTMLAccuStringToNum(a, string);
				break;
			case htmlAlignAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				align.hAlign = (HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string);
				switch(align.hAlign)
				{
					case htmlTopAlign :
					case htmlBottomAlign :
						if(curDirective == htmlCaptionDir) break;
					default :
						align.hAlign = 0;
					case htmlLeftAlign :
					case htmlRightAlign :
						break;
					case htmlCenterAlign :
					case htmlJustifyAlign :
					case htmlCharAlign :
						if(curDirective == htmlCaptionDir) align.hAlign = 0;
				}
				break;
			case htmlVAlignAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				align.vAlign = (HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string);
				if(align.vAlign >= htmlLeftAlign) align.vAlign = 0;
				break;
			case htmlCharAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				if(string[0]) align.alignChar = string[1];
				break;
			case htmlCharOffAttr :
				align.charOff = HTMLAccuStringToNum(a, string);
				break;
			case htmlDirAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch((HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string)) {
					case htmlRTLDir :
						align.direction = rightCaret;
						break;
					case htmlLTRDir :
						align.direction = leftCaret;
				}
		}
	}

	if(err == errEndOfBody) err = noErr;
	if(!err)
	{
		switch(curDirective)
		{
			case htmlColDir :
			case htmlColGroupDir :
			{
				TabColData col;
				long groups = 0, cols = 0;
				short groupCol = 0, lastCol = 0, groupSpan = 0, lastSpan = 0;
				
				col.span = colSpan ? colSpan : 1;
				col.width = width;
				col.propWidth = p;
				col.align = align;
				
				if((**(**matrix).table).columnGroupData)
				{
					groups = GetHandleSize((**(**matrix).table).columnGroupData) / sizeof(TabColData);
					groupCol = (*(**(**matrix).table).columnGroupData)[groups-1].column;
					groupSpan = (*(**(**matrix).table).columnGroupData)[groups-1].span;
				}
				if(!(**matrix).colGroup && (**(**matrix).table).columnData)
				{
					cols = GetHandleSize((**(**matrix).table).columnData) / sizeof(TabColData);
					lastCol = (*(**(**matrix).table).columnData)[cols-1].column;
					lastSpan = (*(**(**matrix).table).columnData)[cols-1].span;
				}
				
				if((**matrix).colGroup || (groupCol > lastCol))
				{
					col.column = groupCol + groupSpan;
				}
				else
				{
					col.column = lastCol + lastSpan;
				}
				if(curDirective == htmlColDir)
				{
					if((**(**matrix).table).columnData == nil)
					{
						(**(**matrix).table).columnData = NuDHTempBetter(&col, sizeof(col));
						err = MemError();
					}
					else
						err = PtrPlusHand(&col, (**(**matrix).table).columnData, sizeof(col));

					if((**(**matrix).table).columnGroupData)
						(*(**(**matrix).table).columnGroupData)[groups-1].span = 0;
					(**matrix).colGroup = false;
				}
				else
				{
					(**matrix).colGroup = true;
					if((**(**matrix).table).columnGroupData == nil)
					{
						(**(**matrix).table).columnGroupData = NuDHTempBetter(&col, sizeof(col));
						err = MemError();
					}
					else
						err = PtrPlusHand(&col, (**(**matrix).table).columnGroupData, sizeof(col));
				}
				break;
			}
			case htmlTHeadDir :
			case htmlTFootDir :
			case htmlTBodyDir :
			{
				TabRowGroupData row;
				
				row.align = align;
				row.rowGroup = (**matrix).rowGroup;
				row.row = (**matrix).row;
				if((**(**matrix).table).rowGroupData == nil)
				{
					(**(**matrix).table).rowGroupData = NuDHTempBetter(&row, sizeof(row));
					err = MemError();
				}
				else
					err = PtrPlusHand(&row, (**(**matrix).table).rowGroupData, sizeof(row));
				break;
			}
			case htmlTRDir :
			{
				TabRowData row;
				
				row.align = align;
				row.row = ++(**matrix).row;
				(**matrix).col = 0;
				if((**(**matrix).table).rowData == nil)
				{
					(**(**matrix).table).rowData = NuDHTempBetter(&row, sizeof(row));
					err = MemError();
				}
				else
					err = PtrPlusHand(&row, (**(**matrix).table).rowData, sizeof(row));
				break;
			}
			case htmlCaptionDir :
			default :
			{
				TabCellData cell = {0};
				
				cell.rowSpan = rowSpan ? rowSpan : 1;
				cell.colSpan = colSpan ? colSpan : 1;
				cell.width = width;
				cell.height = height;
				cell.align = align;
				cell.row = (**matrix).row;
				cell.column = (**matrix).col;
				cell.textOffset = offset - ((**matrix).offset);
				cell.textLength = -1L;

				if((**(**matrix).table).cellData == nil)
				{
					(**(**matrix).table).cellData = NuDHTempBetter(&cell, sizeof(cell));
					err = MemError();
				}
				else
					err = PtrPlusHand(&cell, (**(**matrix).table).cellData, sizeof(cell));
				if(!err)
				{
					++(**(**matrix).table).cells;
					if(curDirective != htmlCaptionDir)
						err = AddCell(matrix, cell.colSpan, cell.rowSpan);
					if(!err)
						err = HTMLSetCellPara(pte, offset, matrix, uniFlags);
				}
			}
		}
	}
	return err;
}

OSErr HTMLSetCellPara(PETEHandle pte, long offset, MatrixHandle matrix, OptionBits *uniFlags)
{
	TabAlignData align;
	PETEParaInfo pinfo;
	OSErr err;
	long index;
	uLong validParaMask = 0;
	
	GetCellHAlign((**matrix).table, (**(**matrix).table).cells - 1, &align.hAlign, &align.alignChar, &align.charOff);
	GetCellDirection((**matrix).table, (**(**matrix).table).cells - 1, &align.direction);

	if(align.hAlign >= htmlLeftAlign || align.hAlign <= htmlCenterAlign)
	{
		pinfo.justification = align.hAlign - htmlDefaultAlign;
		validParaMask |= peJustificationValid;
	}
	if(align.direction != kHilite)
	{
		if(align.direction == leftCaret)
		{
			*uniFlags |= kUnicodeLeftToRightMask;
			*uniFlags &= ~kUnicodeRightToLeftMask;
		}
		else
		{
			*uniFlags |= kUnicodeRightToLeftMask;
			*uniFlags &= ~kUnicodeLeftToRightMask;
		}
		pinfo.direction = align.direction;
		validParaMask |= peDirectionValid;
	}
	if(validParaMask)
	{
		err = PETEGetParaIndex(PETE,pte,offset,&index);
		if(!err)
		{
			err = PETESetParaInfo(PETE,pte,index,&pinfo, validParaMask);
		}
		return err;	
	}
	return noErr;
}

OSErr HTMLPopCell(PETEHandle pte, long *offset, MatrixHandle matrix, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags)
{
	long startIndex, endIndex, tableIndex, startOffset, curCell;
	OSErr err = noErr;
	PETEStyleListHandle styleHandle;
	PETEParaScrapHandle paraHandle;
	TabCellHandle cellData;
	
	cellData = (**(**matrix).table).cellData;
	curCell = (**(**matrix).table).cells - 1;
	if((cellData == nil) || ((*cellData)[curCell].textLength >= 0))
		return noErr;
	
	/* Pop htmlTRDir since it will bring us down to the base table level */
	err = HTMLPopPara(htmlTRDir, *offset, paraContext, pse, uniFlags, pte);
	if(err) return err;
	
	startOffset = (**matrix).offset + (*cellData)[curCell].textOffset;
	err = PETEGetParaIndex(PETE,pte,startOffset,&startIndex);
	if(err) return err;

	err = PETEGetParaIndex(PETE,pte,*offset-1,&endIndex);
	if(err) return err;
	
	err = PETEGetParaIndex(PETE,pte,(**matrix).offset,&tableIndex);
	if(err) return err;
	
	if(*offset - 1 > startOffset)
	{
		err = PeteGetTextStyleScrap(pte, startOffset, *offset - 1, nil, &styleHandle, &paraHandle);
		if(err) return err;
		
		(*cellData)[curCell].styleInfo = styleHandle;
		(*cellData)[curCell].paraInfo = paraHandle;
		(*cellData)[curCell].textLength = *offset - startOffset - 1;
		
		err = PetePlain(pte, startOffset, *offset, peAllValid);
		while(!err && endIndex-- > tableIndex)
		{
			err = PETEDeleteParaBreak(PETE, pte, tableIndex);
		}
	}
	else
	{
		(*cellData)[curCell].textLength = 0;
	}
	
	if(!err)
	{
		char tab = 9;
		short tabCount;
		
		PETEHonorLock(PETE, pte, peNoLock);
		
		if((curCell > 0) && ((*cellData)[curCell-1].row == (*cellData)[curCell].row))
		{
			tabCount = (*cellData)[curCell-1].colSpan;
			if(!tabCount) tabCount = 1;
			
			if((*cellData)[curCell-1].textLength > 0)
			{
				err = PETEReplaceTextPtr(PETE, pte, startOffset - 1, 1, &tab, 1, nil);
			}
			else
			{
				++tabCount;
			}
		}
		else
		{
			tabCount = (*cellData)[curCell].column + 1;
		}
		while(--tabCount > 0)
		{
			err = PETEInsertTextPtr(PETE, pte, startOffset, &tab, 1, nil);
			if(!err)
			{
				++*offset;
				++(*cellData)[curCell].textOffset;
			}
		}
		
		PETEHonorLock(PETE, pte, peModLock | peSelectLock);
	}
	return err;
}

OSErr HTMLInsertHR(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, PETEHandle pte, long *offset, AccuPtr a, StringPtr string)
{
	OSErr err;
	HTMLAttributeEnum attr;
	Boolean FiftyNinthStreetBridge = true;
	long width = -100L;
	short height = 0;
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
//			case htmlAlignAttr :
//				HTMLCopyQuoteString(*a->data, a->offset, string);
//				break;
			case htmlSizeAttr :
				height = HTMLAccuStringToNum(a, string);
				break;
			case htmlWidthAttr :
				width = HTMLAccuStringToNum(a, string);
				break;
			case htmlNoShadeAttr :
				FiftyNinthStreetBridge = false;
		}
	}

	if(err == errEndOfBody) err = noErr;
	
	if(!err) {
		err = PeteInsertRule(pte, *offset, width, height, FiftyNinthStreetBridge, false, false);
		if(!err) {
			++*offset;
			err = HTMLEnsureBreak(pte, *offset);
		}
	}

	return err;
}

/************************************************************************
 * GetURLHash - get a hash for a URL
 ************************************************************************/
uLong GetURLHash(StringPtr s,long id)
{
	Str255	sHash;
	
	PCopy(sHash,s);
	MyLowerStr(sHash);	
	return HashWithSeed(sHash,id);
}

OSErr HTMLChangeFont(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, short basefont, StackHandle fontStack, PETEStyleEntry *pse, OptionBits *uniFlags, uLong validMask, AccuPtr a, StringPtr string)
{
	HTMLAttributeEnum attr;
	OSErr err;
	HTMLFontChange oldFont;
	short nameLen, fontID;
	Boolean quoted;
	
	oldFont.fontID = pse->psStyle.textStyle.tsFont;
	oldFont.fontSize = pse->psStyle.textStyle.tsSize;
	oldFont.color = pse->psStyle.textStyle.tsColor;
	oldFont.label = pse->psStyle.textStyle.tsLabel;
	oldFont.directive = curDirective;
	oldFont.uniFlags = *uniFlags;
	err = StackPush(&oldFont, fontStack);
	if(err) return err;
	
	switch(curDirective) {
		case htmlFontDir :
			break;
		case htmlTTDir :
			pse->psStyle.textStyle.tsFont = kPETEDefaultFixed;
			goto ClearValidBits;
		case htmlBigDir :
			if(validMask & peSizeValid) {
				++pse->psStyle.textStyle.tsSize;
			}
			goto ClearValidBits;
		case htmlSmallDir :
			if(validMask & peSizeValid) {
				--pse->psStyle.textStyle.tsSize;
			}
		default :
	ClearValidBits :
			validMask = 0L;
	}
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlDirAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch((HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string)) {
					case htmlRTLDir :
						*uniFlags |= kUnicodeRightToLeftMask;
						*uniFlags &= ~kUnicodeLeftToRightMask;
						break;
					case htmlLTRDir :
						*uniFlags |= kUnicodeLeftToRightMask;
						*uniFlags &= ~kUnicodeRightToLeftMask;
				}
				break;
			case htmlColorAttr :
				if(validMask & peColorValid) {
					HTMLCopyQuoteString(*a->data, a->offset, string);
					HTMLStringToColor(string, &pse->psStyle.textStyle.tsColor);
				}
				break;
			case htmlFaceAttr :
				if(!(validMask & peFontValid)) break;
				quoted = HTMLHasQuotes(*a->data, a->offset);
				nameLen = (quoted ? 1 : 0);
				do {
					short nameStart;

					for(nameStart = nameLen; nameLen < a->offset - (quoted ? 1 : 0); ++nameLen) {
						if((*a->data)[nameLen] == ',') break;
					}
					NewPStr(*a->data + nameStart, (nameLen - nameStart), string);
					
					if(HTMLGetFontID(string, &fontID)) {
						pse->psStyle.textStyle.tsFont = fontID;
						break;
					}

					if(++nameLen >= a->offset - (quoted ? 1 : 0)) {
						pse->psStyle.textStyle.tsFont = kPETEDefaultFont;
						break;
					}
				} while(true);
				break;
			case htmlSizeAttr :
				if(!(validMask & peSizeValid)) break;
				pse->psStyle.textStyle.tsSize = kPETERelativeSizeBase + HTMLAccuStringToNum(a, string);
				if(CharToToken(string[1]) != tokenNumeric) {
					pse->psStyle.textStyle.tsSize += basefont;
				}
				pse->psStyle.textStyle.tsSize -= 3L;
				if(PrefIsSet(PREF_NOT_SMALLER) && (pse->psStyle.textStyle.tsSize < kPETERelativeSizeBase))
					pse->psStyle.textStyle.tsSize = kPETERelativeSizeBase;
		}
	}
	return (err == errEndOfBody) ? noErr : err;
}

OSErr HTMLPopFont(StackHandle fontStack, HTMLDirectiveEnum curDirective, PETEStyleEntry *pse, OptionBits *uniFlags)
{
	HTMLFontChange oldFont;
	OSErr err;
	
	do {
		err = StackPop(&oldFont, fontStack);
	} while((err == noErr) && (oldFont.directive != curDirective));

	if(err == noErr) {

		pse->psStyle.textStyle.tsFont = oldFont.fontID;
		pse->psStyle.textStyle.tsSize = oldFont.fontSize;
		pse->psStyle.textStyle.tsColor = oldFont.color;
		pse->psStyle.textStyle.tsLabel = oldFont.label;
		*uniFlags = oldFont.uniFlags;
	}
	return noErr;
}

OSErr HTMLGetParaState(PETEHandle pte, long offset, HTMLParaChange *oldPara, PETEParaInfo *pinfo, long *index, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits uniFlags, HTMLDirectiveEnum curDirective)
{
	PETEParaInfo tempPinfo;
	OSErr err;
	long tempIndex;
	
	if(!index)
		index = &tempIndex;
	
	err = PETEGetParaIndex(PETE,pte,offset,index);
	if(err) return err;
	
	if(!pinfo)
		pinfo = &tempPinfo;
	
	pinfo->tabHandle = nil;
		
	err = PETEGetParaInfo(PETE,pte,*index,pinfo);
	if(err) return err;
	
	if(pse)
	{
		oldPara->fontID = pse->psStyle.textStyle.tsFont;
		oldPara->fontSize = pse->psStyle.textStyle.tsSize;
		oldPara->color = pse->psStyle.textStyle.tsColor;
	}
	else
	{
		oldPara->fontID = kPETEDefaultFont;
		oldPara->fontSize = kPETERelativeSizeBase;
		SetPETEDefaultColor(oldPara->color);
	}
	oldPara->direction = pinfo->direction;
	oldPara->justification = pinfo->justification;
	oldPara->startMargin = pinfo->startMargin;
	oldPara->endMargin = pinfo->endMargin;
	oldPara->indent = pinfo->indent;
	oldPara->quoteLevel = pinfo->quoteLevel;
	oldPara->paraFlags = pinfo->paraFlags;
	oldPara->listType = paraContext->listType;
	oldPara->listNum = paraContext->listNum;
	oldPara->listCount = paraContext->listCount;
	oldPara->preformCount = paraContext->preformCount;
	oldPara->emptyP = paraContext->emptyP;
	oldPara->emptyDiv = paraContext->emptyDiv;
	oldPara->directive = curDirective;
	oldPara->uniFlags = uniFlags;

	return noErr;
}

OSErr HTMLChangePara(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, long offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags, uLong validMask, uLong validParaMask, PETEHandle pte, AccuPtr a, StringPtr string)
{
	PETEParaInfo pinfo;
	OSErr err;
	long index, fontSize = -2L;
	Boolean cite = false;
	HTMLParaChange oldPara;
	HTMLAttributeEnum attr;
	HTMLAlignEnum align;
	short halfTabWidth;
	
	if(paraContext->emptyP || (paraContext->directive == htmlPDir))
	{
		err = HTMLPopPara(htmlPDir, offset, paraContext, pse, uniFlags, pte);
		if(err) return err;
	}
	
	err = HTMLGetParaState(pte, offset, &oldPara, &pinfo, &index, paraContext, pse, *uniFlags, curDirective);
	if(err) return err;
	
	pinfo.paraFlags &= ~peListBits;
	switch(curDirective) {
		case htmlPreDir :
			++paraContext->preformCount;
		case htmlXmpDir :
		case htmlListDir :
			pse->psStyle.textStyle.tsFont = kPETEDefaultFixed;
			break;
		case htmlOLDir :
			if(!(validParaMask & peFlagsValid)) break;
			paraContext->listType = htmlNumberList;
			paraContext->listNum = 1L;
			break;
		case htmlULDir :
			if(!(validParaMask & peFlagsValid)) break;
			switch((paraContext->listCount + 1L) % 3L) {
				case 1 :
					pinfo.paraFlags |= peDiskList;
					break;
				case 2 :
					pinfo.paraFlags |= peSquareList;
					break;
				case 0 :
					pinfo.paraFlags |= peCircleList;
					break;
			}
		case htmlDLDir :
			if(!(validParaMask & peFlagsValid)) break;
			paraContext->listType = htmlClearList;
	}
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlDirAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch((HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string)) {
					case htmlRTLDir :
						pinfo.direction = rightCaret;
						*uniFlags |= kUnicodeRightToLeftMask;
						*uniFlags &= ~kUnicodeLeftToRightMask;
						break;
					case htmlLTRDir :
						pinfo.direction = leftCaret;
						*uniFlags |= kUnicodeLeftToRightMask;
						*uniFlags &= ~kUnicodeRightToLeftMask;
				}
				break;
			case htmlBGColorAttr :
				if(!(validMask & peColorValid)) break;
				HTMLCopyQuoteString(*a->data, a->offset, string);
//				HTMLStringToColor(string, &color);
				break;
			case htmlTextAttr :
				if(!(validMask & peColorValid)) break;
				HTMLCopyQuoteString(*a->data, a->offset, string);
//				HTMLStringToColor(string, &pse->psStyle.textStyle.tsColor);
				break;
			case htmlAlignAttr :
				if(!(validParaMask & peJustificationValid)) break;
				HTMLCopyQuoteString(*a->data, a->offset, string);
				align = (HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string);
				if((align >= htmlLeftAlign) && (align <= htmlCenterAlign)) {
					pinfo.justification = align - htmlDefaultAlign;
				}
				break;
			case htmlTypeAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch(FindSTRNIndex(HTMLTypesStrn, string)) {
					case htmlCiteType :
						cite = true;
						break;
					case htmlDiskType :
						if((curDirective == htmlULDir) && (validParaMask & peFlagsValid)) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peDiskList;
						}
						break;
					case htmlSquareType :
						if((curDirective == htmlULDir) && (validParaMask & peFlagsValid)) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peSquareList;
						}
						break;
					case htmlCircleType :
						if((curDirective == htmlULDir) && (validParaMask & peFlagsValid)) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peCircleList;
						}
						break;
					case htmlNumberType :
						if((curDirective == htmlOLDir) && (validParaMask & peFlagsValid)) {
							paraContext->listType = (paraContext->listType & htmlCompactListBit) | htmlNumberList;
						}
						break;
					case htmlLowAlphaType :
					case htmlUpAlphaType :
					case htmlLowRomanType :
					case htmlUpRomanType :
						switch(FindSTRNIndexCase(HTMLTypesStrn, string)) {
							case htmlLowAlphaType :
								if((curDirective == htmlOLDir) && (validParaMask & peFlagsValid)) {
									paraContext->listType = (paraContext->listType & htmlCompactListBit) | htmlLowAlphaList;
								}
								break;
							case htmlUpAlphaType :
								if((curDirective == htmlOLDir) && (validParaMask & peFlagsValid)) {
									paraContext->listType = (paraContext->listType & htmlCompactListBit) | htmlUpAlphaList;
								}
								break;
							case htmlLowRomanType :
								if((curDirective == htmlOLDir) && (validParaMask & peFlagsValid)) {
									paraContext->listType = (paraContext->listType & htmlCompactListBit) | htmlLowRomanList;
								}
								break;
							case htmlUpRomanType :
								if((curDirective == htmlOLDir) && (validParaMask & peFlagsValid)) {
									paraContext->listType = (paraContext->listType & htmlCompactListBit) | htmlUpRomanList;
								}
						}
				}
				break;
			case htmlCiteAttr :
				if(curDirective == htmlBQDir) {
					cite = true;
				}
				break;
			case htmlStartAttr :
				if(!(validParaMask & peFlagsValid)) break;
				paraContext->listNum = HTMLAccuStringToNum(a, string);
				break;
			case htmlCompactAttr :
				if(((curDirective == htmlULDir) || (curDirective == htmlOLDir) || (curDirective == htmlDLDir)) && (validParaMask & peFlagsValid)) {
					paraContext->listType |= htmlCompactListBit;
				}
		}
	}
	
	if(err == errEndOfBody) err = noErr;
	if(err) return err;
	
	halfTabWidth = (FontWidth*GetRLong(TAB_DISTANCE)) / 2;
	
	if(cite && (validParaMask & peQuoteLevelValid)) ++pinfo.quoteLevel;
	
	switch(curDirective) {
		case htmlBQDir :
			if(!cite) {
				if(validParaMask & peStartMarginValid) {
					pinfo.startMargin += halfTabWidth;
				}
				if(validParaMask & peEndMarginValid) {
					pinfo.endMargin -= halfTabWidth;
				}
			}
			break;
		case htmlCenterDir :
			if(!(validParaMask & peJustificationValid)) break;
			pinfo.justification = htmlCenterAlign - htmlDefaultAlign;
			break;
		case htmlOLDir :
		case htmlDLDir :
			if(!(validParaMask & peFlagsValid)) break;
			if(paraContext->listCount != 0L) {
				pinfo.startMargin += halfTabWidth;
			}
			pinfo.startMargin += halfTabWidth;
			pinfo.indent = -halfTabWidth;
			++paraContext->listCount;
			break;
		case htmlULDir :
			if(!(validParaMask & peFlagsValid)) break;
			pinfo.indent = 0L;
			pinfo.startMargin += halfTabWidth;
			++paraContext->listCount;
			break;
		case htmlH1Dir :
			++fontSize;
		case htmlH2Dir :
			++fontSize;
		case htmlH3Dir :
			++fontSize;
		case htmlH4Dir :
			++fontSize;
		case htmlH5Dir :
			++fontSize;
		case htmlH6Dir :
			if(validMask & peSizeValid) {
				pse->psStyle.textStyle.tsSize = kPETERelativeSizeBase + fontSize;
			}
	}
	
	paraContext->directive = curDirective;
	if((pinfo.startMargin == oldPara.startMargin) &&
	   (pinfo.endMargin == oldPara.endMargin) &&
	   (pinfo.quoteLevel == oldPara.quoteLevel) &&
	   (pinfo.justification == oldPara.justification) &&
	   (pinfo.direction == oldPara.direction) &&
	   (pinfo.paraFlags == oldPara.paraFlags) &&
	   ((curDirective == htmlPDir) || (curDirective == htmlDivDir))) {
	
		if(curDirective == htmlPDir) {
			++paraContext->emptyP;
		} else {
			++paraContext->emptyDiv;
		}
	} else {
		if(curDirective == htmlPDir) {
			paraContext->emptyP = 0L;
		} else {
			paraContext->emptyDiv = 0L;
		}
		err = StackPush(&oldPara, paraContext->paraStack);
		if(!err) {
			err = PETESetParaInfo(PETE,pte,index,&pinfo,peStartMarginValid | peEndMarginValid | peIndentValid | peDirectionValid | peJustificationValid | peQuoteLevelValid | peFlagsValid);
		}
	}
	return err;
}

OSErr HTMLPopPara(HTMLDirectiveEnum curDirective, long offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, OptionBits *uniFlags, PETEHandle pte)
{
	HTMLParaChange oldPara;
	PETEParaInfo pinfo;
	long index;
	OSErr err;
	
	if((curDirective == htmlPDir) && (paraContext->emptyP != 0L)) {
		--paraContext->emptyP;
		goto ResetDirective;
	} else if((curDirective == htmlDivDir) && (paraContext->emptyDiv != 0L)) {
		--paraContext->emptyDiv;
		goto ResetDirective;
	}
	
	do {
		err = StackTop(&oldPara, paraContext->paraStack);
		if(err != noErr)
			break;
		if((oldPara.directive == htmlTableDir) && (curDirective != htmlTableDir))
			break;
		err = StackPop(nil, paraContext->paraStack);
		if(err != noErr)
			break;
		if(oldPara.directive == curDirective)
			break;
	} while(true);

	if(err == noErr) {

		err = PETEGetParaIndex(PETE,pte,offset,&index);
		if(err) return err;
		
		pinfo.tabHandle = nil;
		
		err = PETEGetParaInfo(PETE,pte,index,&pinfo);
		if(err) return err;
		
		pse->psStyle.textStyle.tsFont = oldPara.fontID;
		pse->psStyle.textStyle.tsSize = oldPara.fontSize;
		pse->psStyle.textStyle.tsColor = oldPara.color;
		pinfo.direction = oldPara.direction;
		pinfo.justification = oldPara.justification;
		pinfo.startMargin = oldPara.startMargin;
		pinfo.endMargin = oldPara.endMargin;
		pinfo.indent = oldPara.indent;
		pinfo.quoteLevel = oldPara.quoteLevel;
		pinfo.paraFlags = oldPara.paraFlags;
		if((curDirective == htmlDLDir) || (curDirective == htmlOLDir) || (curDirective == htmlULDir)) {
			paraContext->listType = oldPara.listType;
			paraContext->listNum = oldPara.listNum;
			paraContext->listCount = oldPara.listCount;
		}
		paraContext->preformCount = oldPara.preformCount;
		paraContext->emptyP = oldPara.emptyP;
		paraContext->emptyDiv = oldPara.emptyDiv;
		
		err = PETESetParaInfo(PETE,pte,index,&pinfo,peStartMarginValid | peEndMarginValid | peIndentValid | peDirectionValid | peJustificationValid | peQuoteLevelValid | peFlagsValid);
		if(err) return err;
		
		paraContext->emptyP = oldPara.emptyP;
		paraContext->emptyDiv = oldPara.emptyDiv;
		*uniFlags = oldPara.uniFlags;
	ResetDirective :
		if(!paraContext->emptyP) {
			if(paraContext->emptyDiv) {
				paraContext->directive = htmlDivDir;
			} else if(noErr == StackTop(&oldPara, paraContext->paraStack)) {
				paraContext->directive = oldPara.directive;
			} else {
				paraContext->directive = htmlNoDir;
			}
		} else {
			paraContext->directive = htmlPDir;
		}
	}
	return noErr;
}

Boolean HTMLHasQuotes(StringPtr ptr, long len)
{
	TokenType quoteChar;
	
	return((len > 1) && (*ptr == *(ptr + len - 1L)) && (((quoteChar = CharToToken(*ptr)) == token1Quote) || (quoteChar == token2Quote)));
}

void HTMLCopyQuoteString(StringPtr ptr, long len, StringPtr pstr)
{
	if(len >= sizeof(Str255)) len = sizeof(Str255) - 1;
	
	if(HTMLHasQuotes(ptr, len)) {
		++ptr;
		len -= 2;
	}
	NewPStr(ptr, len, pstr);
}

void HTMLUnquoteAndTrim(AccuPtr a)
{
	if(a->offset >= 2 && HTMLHasQuotes(*a->data, a->offset)) {
		a->offset -= 2;
		if (a->offset) BMD(*a->data + 1, *a->data, a->offset);
	}
	AccuTrim(a);
}

OSErr HTMLStartLink(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLDirectiveEnum curDirective, StackHandle fontStack, PETEHandle pte, PETEStyleEntry *pse, OptionBits *uniFlags, long *offset, Handle base, Handle src, AccuPtr a, StringPtr string)
{
	HTMLAttributeEnum attr;
	OSErr err;
	HTMLFontChange oldFont;
	Str255 url;
	long baseLen;
	Boolean autourl = false, href = false;
	
	oldFont.fontID = pse->psStyle.textStyle.tsFont;
	oldFont.fontSize = pse->psStyle.textStyle.tsSize;
	oldFont.color = pse->psStyle.textStyle.tsColor;
	oldFont.label = pse->psStyle.textStyle.tsLabel;
	oldFont.directive = curDirective;
	oldFont.uniFlags = *uniFlags;
	err = StackPush(&oldFont, fontStack);
	if(err != noErr) return err;
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlDirAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch((HTMLAlignEnum)FindSTRNIndex(HTMLAlignStrn, string)) {
					case htmlRTLDir :
						*uniFlags |= kUnicodeRightToLeftMask;
						*uniFlags &= ~kUnicodeLeftToRightMask;
						break;
					case htmlLTRDir :
						*uniFlags |= kUnicodeLeftToRightMask;
						*uniFlags &= ~kUnicodeRightToLeftMask;
				}
				break;
			case htmlHrefAttr :
				if(!autourl) {
					HTMLCopyQuoteString(*a->data, a->offset, string);
					if((base == nil) || ((baseLen = InlineGetHandleSize(base)) == 0L)) {
						if((base = src) != nil) {
							baseLen = InlineGetHandleSize(base);
						}
					}
					if(base != nil) {
						NewPStr(*base, baseLen, url);
					} else {
						url[0] = 0;
					}
					URLCombine(url,url,string);
					href = true;
				}
				break;
			case htmlEudoraAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				if(FindSTRNIndex(HTMLTypesStrn, string) == htmlAutoURLType) {
					autourl = true;
				}
		}
	}
	if(err == errEndOfBody) {
		err = noErr;
	} else if(err != noErr) {
		return err;
	}
	if(!autourl && href) {
		pse->psStyle.textStyle.tsLabel |= pLinkLabel;
		**Pslh = *pse;
		(*Pslh)->psGraphic = true;
		(*Pslh)->psStyle.graphicStyle.graphicInfo = nil;
		(*Pslh)->psStyle.graphicStyle.filler0 = 0;
		err = PeteInsertChar(pte, *offset, '<', Pslh);
		if(err != noErr) return err;
		++*offset;

		(*Pslh)->psStyle.textStyle.tsLabel |= pURLLabel;
		err = PETEInsertTextPtr(PETE, pte, *offset, &url[1], url[0], Pslh);
		if(err != noErr) return err;
		*offset += url[0];

		(*Pslh)->psStyle.textStyle.tsLabel = pse->psStyle.textStyle.tsLabel;
		err = PeteInsertChar(pte, *offset, '>', Pslh);
		if(!err) ++*offset;
	}
	return err;
}

void HTMLStringToColor(StringPtr colorString, RGBColor *color)
{
	StringPtr origString = colorString;
	Byte colorHex[13];
	Byte *colorHexPtr = colorHex;
	short count;
	Boolean first;
	RGBColor **colorHandle;
	
	if((*colorString++ == 7) && (*colorString++ == '#')) {
		*colorHexPtr++ = 12;
		for(count = 0; count < 3; ++count) {
			first = true;
			*colorHexPtr++ = '0';
			*colorHexPtr++ = '0';
			do {
				switch(*colorHexPtr++ = *colorString++) {
					case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
					case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
					case '0' : case '1' : case '2' : case '3' : case '4' : case '5' : case '6' : case '7' : case '8' : case '9' :
						break;
					default :
						goto TryResource;
				}
			} while(first && (first = false, true));
		}
		StuffHex(color, colorHex);
		color->red *= 0x0101;
		color->blue *= 0x0101;
		color->green *= 0x0101;
	} else {
TryResource :
		colorHandle = (RGBColor **)GetNamedResource('RGB ', origString);
		if((colorHandle != nil) && (ResError() == noErr)) {
			*color = **colorHandle;
		}
	}
}

OSErr HTMLGetAttrValue(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, HTMLAttributeEnum *attr, AccuPtr value, StringPtr string)
{
	OSErr err = noErr;
	Boolean gotName = false, gotAttr = false, gotValue = false;
	Byte hState;

	value->offset = 0L;	
	while(err == noErr) {
		switch(htmlToken->tokenType) {
			case htmlEndToken :
				return errEndOfBody;
			case htmlTagEndToken :
				return gotName ? noErr : errEndOfBody;
			case htmlNameToken :
				if(gotName) {
					return noErr;
				} else {
					NewPStr(*(htmlContext->html) + *htmlOffset, htmlToken->tokenLen, string);
					*attr = (HTMLAttributeEnum)FindSTRNIndex(HTMLAttributeStrn, string);
					gotName = true;
				}
				break;
			case htmlStringToken :
				if(gotAttr) {
					gotValue = true;
					hState = HGetState(htmlContext->html);
					HLock(htmlContext->html);
					err = AccuAddPtr(value, *(htmlContext->html) + *htmlOffset, htmlToken->tokenLen);
					HSetState(htmlContext->html, hState);
				}
				break;
			case htmlLiteralToken :
				if(gotAttr) {
					UniChar literal;
					
					gotValue = true;
					hState = HGetState(htmlContext->html);
					HLock(htmlContext->html);
					literal = HTMLLiteral(*(htmlContext->html) + *htmlOffset, htmlToken->tokenLen);
					if(literal == 0xFFFF) {
						err = AccuAddPtr(value, *(htmlContext->html) + *htmlOffset, htmlToken->tokenLen);
					}
					HSetState(htmlContext->html, hState);
					if(literal <= 0x7F) {
						err = AccuAddChar(value, literal);
					} else if (literal != 0xFFFF) {
						unsigned char hex[3];
						StringPtr c;
						
						hex[0] = '%';
						c = &string[1];
						string[0] = UniCharToUTF8(literal, c, sizeof(Str255) - 1);
						while(string[0]--) {
							Bytes2Hex(c++, 1, &hex[1]);
							err = AccuAddPtr(value, hex, sizeof(hex));
						}
					}
				}
				break;
			case htmlAttrToken :
				if(gotName) {
					gotAttr = true;
				}
			default :
				if(gotValue) {
					return noErr;
				}
		}
		*htmlOffset += htmlToken->tokenLen;
		HTMLGetNextToken(htmlContext, htmlToken);
	}
	return err;			
}

/**********************************************************************
 * UPtrIsChar - Checks if a memory is identical to a string, paying attention to case
 **********************************************************************/
Boolean UPtrIsChar(UPtr s1,long l,UPtr s2)
{
	if(l != *s2++) return false;
	while(l-- != 0) {
		if(*s1++ != *s2++) return false;
	}
	return true;
}

/************************************************************************
 * FindSTRNIndexCase - find a string in a resource id
 ************************************************************************/
short FindSTRNIndexCase(short resId,PStr string)
{
	UHandle resH = GetResource_('STR#',resId);
	short index;
	Byte hState;
	
	hState = HGetState(resH);
	HNoPurge(resH);
	index = FindSTRNIndexResCase(resH,string);
	HSetState(resH, hState);
	return index;
}

/**********************************************************************
 * FindSTRNIndexResCase - Find a string in an STR# resource
 **********************************************************************/
short FindSTRNIndexResCase(UHandle resource,PStr string)
{
	short n = CountStrnRes(resource);
	short i;
	UPtr p,np,s;
	
	if (n && resource && *resource)
	{
		for (i=1,np=p=*resource+2;i<=n;i++,p=np)
		{
			s=string;
			np=p+*p+1;
			do {
				if(p==np) {
					return i;
				}
			} while(*p++ == *s++);
		}
	}
	return(0);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr HTMLEnsureBreak(PETEHandle pte,long inOffset)
{
	PETEParaInfo pinfo;
	long offset;
	long pIndex;
	OSErr err = noErr;
	
	if (inOffset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
	else offset = inOffset;
	
	if (offset>1)
	{
		Zero(pinfo);
		pIndex = PeteParaAt(pte,offset);
		PETEGetParaInfo(PETE,pte,pIndex,&pinfo);
		if (pinfo.paraOffset!=offset || inOffset==-1&&pinfo.paraLength)
		{
			err = PETEInsertParaBreak(PETE,pte,offset);
		}
	}
	return err;
}

/************************************************************************
 * PeteEnsureCrAndBreak - make sure there's a cr and a break at the offset
 ************************************************************************/
OSErr HTMLEnsureCrAndBreak(PETEHandle pte, long inOffset, long *newOffset, PETEStyleEntry *pse)
{
	long offset;
	unsigned char text;
	OSErr err;
	PETEDocInfo docInfo;

	err = PETEGetDocInfo(PETE,pte,&docInfo);
	if(err) return err;
	
	offset = docInfo.selStart;
	
	// Have we been given an absolute offset that happens to be
	// the insertion point?
	if (inOffset==docInfo.selStart) inOffset = -1;
	
	// If we've been given an offset, use that
	if (inOffset!=-1) offset = inOffset;
	
	if(offset) {
		err = PETEGetTextPtr(PETE, pte, offset - 1L, offset, &text, sizeof(text), nil);
	}
	if(err) return err;

	// Work to do?
	if (text!='\015')
	{
		**Pslh = *pse;
		err = PeteInsertChar(pte,offset,'\015',Pslh);
		if (!err) {
			err = PETEInsertParaBreak(PETE,pte,++offset);
		
			if (!err && inOffset==-1) PeteSelect((*PeteExtra(pte))->win,pte,offset,docInfo.selStop+1);
		}
	}
	else
		err = HTMLEnsureBreak(pte,offset);
	if (newOffset) *newOffset = offset;
	return(err);
}

OSErr HTMLInsertUniChar(PETEHandle pte, long *offset, UniChar c, PETEStyleEntry *pse, AccuPtr a, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, WordSpaceEnum addSpace, PETEStyleEntry *spaceStyle)
{
	OSErr err;
	
	if(c == 0xFFFF)
	{
		err = noErr;
	
	} else if(!HasUnicode()) {
		unsigned char insertChar = '?';
		
		if(c == kUnicodeNonBreakingSpace) {
			insertChar = nbSpaceChar;
		} else if(c < 128) {
			insertChar = c;
		} else {
			LiteralPtr entry;
			long index;
			Byte rState, oState;
			Ptr basePtr;
			
			err = HTMLGetLiteralHandles(&rState, &oState);
			if(err == noErr) {
				basePtr = &(**litResH).entries[0];
				for(index = 0; index < (**litResH).numEntries; ++index) {
					entry = (LiteralPtr)(basePtr + (*litOffsets)[index]);
					if(entry->unicode == c) {
						insertChar = entry->theChar;
						break;
					}
				}
				HSetState((Handle)litResH, rState);
				HSetState((Handle)litOffsets, oState);
			}
		}
		err = HTMLInsertText(pte, offset, sizeof(insertChar), nil, 0L, &insertChar, pse, a, ase, string, converter, addSpace, spaceStyle);
	} else {
		TECObjectRef inToUnicode = converter->inToUnicode;

		converter->inToUnicode = nil;
		err = HTMLInsertText(pte, offset, sizeof(c), nil, 0L, &c, pse, a, ase, string, converter, addSpace, spaceStyle);
		converter->inToUnicode = inToUnicode;
	}
	return err;
}

OSErr HTMLInsertText(PETEHandle pte, long *offset, long len, StringHandle h, long hOff, StringPtr p, PETEStyleEntry *pse, AccuPtr a, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, WordSpaceEnum addSpace, PETEStyleEntry *spaceStyle)
{
	OSErr err;
	PETEParaInfo pinfo;
	StringPtr x;
	Boolean stripNBSP, flush = false;
	TextEncoding convEncoding;
	PETEStyleEntry cse;
	Byte hState;
	ByteCount inLen, outLen, nbspLen;
	ScriptCode script;
	
	if((a->offset != 0L) && memcmp(pse, ase, sizeof(PETEStyleEntry))) {
		err = HTMLDumpText(pte, *offset, a, ase, false);
		if(err) return err;
	}
	if(a->offset == 0) {
		Zero(pinfo);
		err = PETEGetParaInfo(PETE,pte,PeteParaAt(pte,*offset),&pinfo);
		if(err) return err;
		stripNBSP = ((pinfo.paraLength == 0L) || (PeteCharAt(pte, *offset-1L) == 13));
	} else {
		stripNBSP = ((*(a->data))[a->offset-1L] == 13);
	}
	
	do {

		cse = *pse;
		
		if(h != nil) {
			hState = HGetState(h);
			HLock(h);
			p = *h + hOff;
		}
		
		flush = (len == 0);

		inLen = len;
		outLen = sizeof(Str255);
		err = ConvertIntlText(converter, p, &inLen, string, &outLen, &convEncoding, addSpace, nil);
		
		if(h != nil) {
			HSetState(h, hState);
			hOff += inLen;
		} else if(!flush) {
			p += inLen;
		}
		len -= inLen;
		
		if(err && (err != kTECOutputBufferFullStatus) && (err != kTECArrayFullErr)) return err;

		addSpace = dontAddWordSpace;
		
		if(outLen == 0) {
			if(flush) {
				break;
			} else {
				continue;
			}
		}
		
		if(err) flush = true;

		if(convEncoding != kTextEncodingUnknown) {
			err = EncodingPlusPeteStyle(convEncoding, &cse, &script);
			if(err) return err;
		} else {
			script = smRoman;
			err = noErr;
		}
		
		if((a->offset > 0L) && (memcmp(&cse, ase, sizeof(PETEStyleEntry)) || a->offset > 4 K)) {
			err = HTMLDumpText(pte, *offset, a, ase, false);
			if(err) return err;
		}
		*ase = cse;

		nbspLen = 0L;

		if(script == smRoman) {
			if((outLen > 0) && ((*string == ' ') || ((*string == nbSpaceChar) && stripNBSP)) && (a->data != nil) && (a->offset > 0L)) {
				for(x = (StringPtr)(*(a->data)) + a->offset; (--x >= *(a->data)) && ((*x == nbSpaceChar) || (*x == ' ')); ) {
					*x = ' ';
				}
			}
			
			if(stripNBSP) {
				while((outLen > nbspLen) && (string[nbspLen] == nbSpaceChar)) {
					++nbspLen;
					err = AccuAddChar(a, ' ');
					if(err) return err;
				}
				if(nbspLen != outLen) {
					stripNBSP = false;
				}
			} else {
				stripNBSP = false;
			}
		}
		
		if(outLen > nbspLen) {
			err = AccuAddPtr(a, &string[nbspLen], outLen - nbspLen);
			if(err) return err;
		}
		*offset += outLen;

	} while(len > 0L || flush);
	
	return err;
}

OSErr HTMLDumpText(PETEHandle pte, long offset, AccuPtr a, PETEStyleEntry *ase, Boolean trimNbsp)
{
	long len = a->offset;

	if(len == 0L) return noErr;
	if(trimNbsp) {
		long i = len;
		StringPtr p = (StringPtr)(*(a->data)) + i;

		while(--i >= 0) {
			if(*--p == nbSpaceChar) {
				*p = ' ';
			} else if(*p != ' ') {
				break;
			}
		}
	}
	a->offset = 0L;
	**Pslh = *ase;
	return PETEInsertTextHandle(PETE, pte, offset - len, a->data, len, 0L, Pslh);
}

Boolean HTMLGetFontID(StringPtr name, short *fontID)
{
	GetFNum(name, fontID);
	if(*fontID == 0) {
		GetFontName(0, GlobalTemp);
		if(!EqualString(name, GlobalTemp, false, true)) {
			PTr(name,"_"," ");
			GetFNum(name, fontID);
			if(*fontID == 0) {
				if(!EqualString(name, GlobalTemp, false, true)) {
					return false;
				}
			}
		}
	}
	if(*fontID == 1) {
		*fontID = GetAppFont();
	}
	return true;
}

void HTMLNumToListNum(long num, StringPtr string, HTMLListType listType)
{
	Byte index;
	Boolean lower = false;
	
	switch(listType) {
		case htmlNumberList :
			NumToString(num, string);
			break;
		case htmlLowAlphaList :
			string[1] = 'a';
			goto DoAlpha;
		case htmlUpAlphaList :
			string[1] = 'A';
	DoAlpha :
			string[0] = ((num + 1L) / 26) + 1;
			string[1] += (((num % 26) == 0 ? 26 : (num % 26)) - 1);
			for(index = string[0]; index > 1; --index) {
				string[index] = string[1];
			}
			break;
		case htmlLowRomanList :
			lower = true;
		case htmlUpRomanList :
			string[0] = 0;
			while (num >= 1000) {
				CatChar(string,lower?'m':'M');
				num -= 1000;
			}
			if (num >= 900) {
				CatChar(string,lower?'c':'C');
				CatChar(string,lower?'m':'M');
				num -= 900;
			}
			if (num >= 500) {
				CatChar(string,lower?'d':'D');
				num -= 500;
			}
			if (num >= 400) {
				CatChar(string,lower?'c':'C');
				CatChar(string,lower?'d':'D');
				num -= 400;
			}
			while (num >= 100) {
				CatChar(string,lower?'c':'C');
				num -= 100;
			}
			if (num >= 90) {
				CatChar(string,lower?'x':'X');
				CatChar(string,lower?'c':'C');
				num -= 90;
			}
			if (num >= 50) {
				CatChar(string,lower?'l':'L');
				num -= 50;
			}
			if (num >= 40) {
				CatChar(string,lower?'x':'X');
				CatChar(string,lower?'l':'L');
				num -= 40;
			}
			while (num >= 10) {
				CatChar(string,lower?'x':'X');
				num -= 10;
			}
			if (num >= 9) {
				CatChar(string,lower?'i':'I');
				CatChar(string,lower?'x':'X');
				num -= 9;
			}
			if (num >= 5) {
				CatChar(string,lower?'v':'V');
				num -= 5;
			}
			if (num >= 4) {
				CatChar(string,lower?'i':'I');
				CatChar(string,lower?'v':'V');
				num -= 4;
			}
			while (num >= 1) {
				CatChar(string,lower?'i':'I');
				num -= 1;
			}
	}
}

/* Does this need internationalization? */
OSErr HTMLInsertListItem(HTMLTokenContext *htmlContext, HTMLTokenAndLen *htmlToken, long *htmlOffset, long *offset, HTMLParaContext *paraContext, PETEStyleEntry *pse, PETEHandle pte, AccuPtr a, StringPtr string)
{
	OSErr err;
	Boolean ordered;
	long index;
	PETEParaInfo pinfo;
	Byte paraFlags;
	HTMLAttributeEnum attr;
	
	err = PETEGetParaIndex(PETE,pte,*offset,&index);
	if(err) return err;
	
	pinfo.tabHandle = nil;
		
	err = PETEGetParaInfo(PETE,pte,index,&pinfo);
	if(err) return err;
	
	paraFlags = pinfo.paraFlags;
	
	ordered = !(!(paraContext->listType & htmlAnyNumericList));
	
	while((err = HTMLGetAttrValue(htmlContext, htmlToken, htmlOffset, &attr, a, string)) == noErr) {
		if (!(a->offset)) continue;	//malformed html?
		switch(attr) {
			case htmlTypeAttr :
				HTMLCopyQuoteString(*a->data, a->offset, string);
				switch(FindSTRNIndex(HTMLTypesStrn, string)) {
					case htmlDiskType :
						if(!ordered) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peDiskList;
						}
						break;
					case htmlSquareType :
						if(!ordered) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peSquareList;
						}
						break;
					case htmlCircleType :
						if(!ordered) {
							pinfo.paraFlags &= ~peListBits;
							pinfo.paraFlags |= peCircleList;
						}
						break;
					case htmlNumberType :
						if(ordered) {
							paraContext->listType = (paraContext->listType & ~htmlAnyNumericList) | htmlNumberList;
						}
						break;
					case htmlLowAlphaType :
						if(ordered) {
							paraContext->listType = (paraContext->listType & ~htmlAnyNumericList) | htmlLowAlphaList;
						}
						break;
					case htmlUpAlphaType :
						if(ordered) {
							paraContext->listType = (paraContext->listType & ~htmlAnyNumericList) | htmlUpAlphaList;
						}
						break;
					case htmlLowRomanType :
						if(ordered) {
							paraContext->listType = (paraContext->listType & ~htmlAnyNumericList) | htmlLowRomanList;
						}
						break;
					case htmlUpRomanType :
						if(ordered) {
							paraContext->listType = (paraContext->listType & ~htmlAnyNumericList) | htmlUpRomanList;
						}
				}
				break;
			case htmlValueAttr :
				if(ordered) {
					paraContext->listNum = HTMLAccuStringToNum(a, string);
				}
		}
	}
	
	if(err == errEndOfBody) {
		err = noErr;
	}
	
	if((paraFlags != pinfo.paraFlags) && !err) {
		err = PETESetParaInfo(PETE,pte,index,&pinfo,peFlagsValid);
	}

	if(ordered && !err) {
		HTMLNumToListNum(paraContext->listNum++, string, (paraContext->listType & htmlAnyNumericList));
		string[++string[0]] = '.';
		string[++string[0]] = 9;
		**Pslh = *pse;
		err = PETEInsertTextPtr(PETE, pte, *offset, &string[1], string[0], Pslh);
		if(!err) *offset += string[0];
	}
	return err;
}

OSErr HTMLFlushText(PETEHandle pte, long *offset, AccuPtr a, PETEStyleEntry *pse, PETEStyleEntry *ase, StringPtr string, IntlConverter *converter, Boolean trimNbsp)
{
	OSErr err;
	
	err = HTMLInsertText(pte, offset, 0L, nil, 0L, (StringPtr)-1L, pse, a, ase, string, converter, dontAddWordSpace, nil);
	if(!err) {
		err = HTMLDumpText(pte, *offset, a, ase, trimNbsp);
	}
	return err;
}

OSErr BuildHTML(AccuPtr html,PETEHandle pte,Handle text,long len,long offset,PETEStyleListHandle pslh,PETEParaScrapHandle paraScrap,long partID,PStr mid,StackHandle specStack,FSSpecPtr errSpec)
{
	HTMLOutContext htmlContext;
	
	HTMLInitOutContext(&htmlContext, html);
	return BuildHTMLLo(&htmlContext, pte, text, len, offset, pslh, paraScrap, partID, mid, specStack, errSpec);
}

OSErr BuildHTMLLo(HTMLOutContext *htmlContext,PETEHandle pte,Handle text,long len,long offset,PETEStyleListHandle pslh,PETEParaScrapHandle paraScrap,long partID,PStr mid,StackHandle specStack,FSSpecPtr errSpec)
{
	PETEParaInfo paraInfo;
	long indentCount = 0L, quoteCount = 0L, newIndentCount, paraIndex;
	StackHandle paraStack = nil, styleStack = nil;
	Byte listType = 0;
	Boolean wasDiv = false, changeAlign, changeIndent, doParaChanges, doLI, allReturns, hasPara = true;
	HTMLParaAttr paraAttr, newParaAttr;
	long runLen;
	PETEStyleEntry oldStyle, newStyle;
	short styleBit, styleOn, styleOff, styleRemoved;
	long charLen;
	long paraOffset = 0L, styleIndex = 0L;
	PETEParaScrapPtr paraPtr;
	OSErr err;
	FGIHandle fgi;
	Boolean tempFGI = false;
	Boolean alreadyWarnedAboutMissingGraphics = false;
	PETEStyleEntry lastTextStyle;
	
	if(pte) {
		paraInfo.tabHandle = nil;
		err = PETEGetRawText(PETE, pte, &text);
		if(err) goto done;
	} else if(paraScrap == nil) {
		hasPara = false;
	}
	
	err = StackInit(sizeof(HTMLStyleAttr), &styleStack);
	if(err) goto done;
	
	if(hasPara)
	{
		err = StackInit(sizeof(HTMLParaAttr), &paraStack);
		if(err) goto done;
	}

	if(pte) {
		/* Get the default paraInfo */
		paraInfo.tabHandle = nil;
		err = PETEGetParaInfo(PETE, pte, kPETEDefaultPara, &paraInfo);
		if(err) goto done;

		/* Initialize the paraStack */
		paraAttr.align = paraInfo.justification + htmlDefaultAlign;
		paraAttr.firstIndents = paraInfo.indent / ((FontWidth*GetRLong(TAB_DISTANCE)) / 2);

		/* Get the initial paragraph index */
		err = PETEGetParaIndex(PETE,pte,offset,&paraIndex);
		if(err) goto done;
		
	} else {
		paraInfo.paraOffset = 0L;
		paraInfo.paraLength = 0L;
		paraAttr.align = htmlDefaultAlign;
		paraAttr.firstIndents = 0L;
		paraIndex = 0L;
	}

	if(hasPara)
	{
		err = StackPush(&paraAttr, paraStack);
		if(err) goto done;
	}
	
	while(offset < len) {
		
		CycleBalls();
		if(CommandPeriod) {
			err = userCanceledErr;
			goto done;
		}

		charLen = 0L;
		
		doLI = false;
		
		if(pte) {
			/* Get the paraInfo for this paragraph */
			err = PETEGetParaInfo(PETE, pte, paraIndex, &paraInfo);
			if(err) goto done;
		} else if(paraScrap != nil) {
			paraInfo.paraOffset += paraInfo.paraLength;
			paraPtr = (PETEParaScrapPtr)((*(Handle)paraScrap) + paraOffset);
			paraInfo.paraLength = paraPtr->paraLength;
			paraInfo.startMargin = paraPtr->startMargin;
			paraInfo.indent = paraPtr->indent;
			paraInfo.direction = paraPtr->direction;
			paraInfo.justification = paraPtr->justification;
			paraInfo.quoteLevel = paraPtr->quoteLevel;
			paraInfo.paraFlags = paraPtr->paraFlags;
			paraOffset += sizeof(PETEParaScrapEntry) + (((paraPtr->tabCount < 0) ? -paraPtr->tabCount : paraPtr->tabCount) * sizeof(paraPtr->tabStops[0]));
		} else {
			Zero(paraInfo);
			paraInfo.paraLength = len;
			paraInfo.justification = teFlushDefault;
			goto DoText;
		}
		
		/* Figure out how many indents */
		newIndentCount = paraInfo.startMargin / ((FontWidth*GetRLong(TAB_DISTANCE)) / 2);
		/* Lists are indented one by default */
		if(paraInfo.paraFlags & peListBits && newIndentCount>0) {
			--newIndentCount;
		}

		/* Initialize the next para attributes to push */
		newParaAttr.align = paraInfo.justification + htmlDefaultAlign;
		newParaAttr.firstIndents = paraInfo.indent / ((FontWidth*GetRLong(TAB_DISTANCE)) / 2);

		/* If it's just another list item, put in the <LI> and short circuit */
		if((listType != 0) && (paraInfo.paraFlags & peListBits) && (newIndentCount == indentCount) && (paraInfo.quoteLevel == quoteCount)) {
			err = StackTop(&paraAttr, paraStack);
			if(err) goto done;
			
			changeAlign = (paraAttr.align != newParaAttr.align);
			changeIndent = false /*(paraAttr.firstIndents != newParaAttr.firstIndents)*/;
			
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, !(!(paraInfo.paraFlags & listType) || changeAlign || changeIndent), htmlTagToken, htmlLIDir, htmlNoAttr);
			if(!(paraInfo.paraFlags & listType)) {
				if(!(listType = (paraInfo.paraFlags & peDiskList))) {
					if(!(listType = (paraInfo.paraFlags & peSquareList))) {
						listType = peCircleList;
					}
				}
				err = HTMLAddDirective(htmlContext, htmlBreakNot, !(changeAlign || changeIndent), htmlAttrToken, htmlTypeAttr, htmlResValue, (long)HTMLTypesStrn+((listType & peDiskList) ? htmlDiskType : ((listType & peSquareList) ? htmlSquareType : htmlCircleType)), htmlNoAttr);
			}
			goto DoText;
		}
		
		/* Undo all of the text level styles */
		err = HTMLRemoveStyles(htmlContext, 0xFFFF, nil, styleStack);
		if(err) goto done;
		
		/* Get the default style */
		if(pte) {
			err = PETEGetStyle(PETE, pte, kPETEDefaultStyle, nil, &oldStyle);
			if(err) goto done;
		} else {
			Zero(oldStyle);
			oldStyle.psStyle.textStyle.tsFont = kPETEDefaultFont;
			oldStyle.psStyle.textStyle.tsSize = kPETERelativeSizeBase;
			SetPETEDefaultColor(oldStyle.psStyle.textStyle.tsColor);
			newStyle = oldStyle;
		}
		
		//
		// the default style is a text style, so save it now
		lastTextStyle = oldStyle;
		
		/* Undo the outstanding list */
		if(listType != 0) {
			err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlULDir, htmlNoAttr);
			if(err) goto done;
			
			listType = 0;
			
			err = StackPop(nil, paraStack);
			if(err) goto done;
		}
		
		/* Undo outstanding BLOCKQUOTEs from indents and excerpts */
		while((newIndentCount < indentCount) || (paraInfo.quoteLevel < quoteCount)) {
			
			err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlBQDir, htmlNoAttr);
			if(err) goto done;
			
			/* Indents first, then excerpts outside */
			if(indentCount != 0L) {
				--indentCount;
			} else {
				--quoteCount;
			}
			
			err = StackPop(nil, paraStack);
			if(err) goto done;
		}
		
		/* If there was a surrounding DIV tag, undo that */
		if(wasDiv) {
			err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlDivDir, htmlNoAttr);
			if(err) goto done;
			
			wasDiv = false;
			
			err = StackPop(nil, paraStack);
			if(err) goto done;
		/* Undo the last BLOCKQUOTE if there were any left */
		} else if((indentCount == newIndentCount) && (quoteCount == paraInfo.quoteLevel) && ((indentCount != 0) || (quoteCount != 0))) {
			err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlBQDir, htmlNoAttr);
			if(err) goto done;

			if(indentCount != 0L) {
				--indentCount;
			} else {
				--quoteCount;
			}
			
			err = StackPop(nil, paraStack);
			if(err) goto done;
		}
		
		err = StackTop(&paraAttr, paraStack);
		if(err) goto done;
		
		changeAlign = (paraAttr.align != newParaAttr.align);
		changeIndent = false /*(paraAttr.firstIndents != newParaAttr.firstIndents)*/;
		
		/* First see if there are excerpts */
		while(paraInfo.quoteLevel > quoteCount) {
			++quoteCount;
			
			/* If this is the last one to do and there is align or indent info, leave the tag open */
			doParaChanges = ((quoteCount == paraInfo.quoteLevel) && (newIndentCount == indentCount) && !(paraInfo.paraFlags & peListBits) && (changeAlign || changeIndent));

			if(!doParaChanges) {
				err = StackPush(&paraAttr, paraStack);
				if(err) goto done;
			}
			
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, !doParaChanges, htmlTagToken, htmlBQDir, htmlTypeAttr, htmlResValue, (long)HTMLTypesStrn+htmlCiteType, htmlCiteAttr, htmlNoValue, htmlNoAttr);
			if(err) goto done;
		}
		
		/* Next do the indents */
		while(newIndentCount > indentCount) {
			++indentCount;

			/* If this is the last one to do and there is align or indent info, leave the tag open */
			doParaChanges = ((newIndentCount == indentCount) && !(paraInfo.paraFlags & peListBits) && (changeAlign || changeIndent));

			if(!doParaChanges) {
				err = StackPush(&paraAttr, paraStack);
				if(err) goto done;
			}
			
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, !doParaChanges, htmlTagToken, htmlBQDir, htmlNoAttr);
			if(err) goto done;
		}
		
		/* Next do the list */
		if(paraInfo.paraFlags & peListBits) {

			if(!(listType = (paraInfo.paraFlags & peDiskList))) {
				if(!(listType = (paraInfo.paraFlags & peSquareList))) {
					listType = peCircleList;
				}
			}
			
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, !(changeAlign || changeIndent || (listType != peDiskList)), htmlTagToken, htmlULDir, htmlNoAttr);
			if(err) goto done;
			
			/* If the list is other than the default disk, add that on */
			if(listType != peDiskList) {
				/* Leave it open if it needs align or indent info */
				err = HTMLAddDirective(htmlContext, htmlBreakNot, !(changeAlign || changeIndent), htmlAttrToken, htmlTypeAttr, htmlResValue, (long)HTMLTypesStrn+((listType & peDiskList) ? htmlDiskType : ((listType & peSquareList) ? htmlSquareType : htmlCircleType)), htmlNoAttr);
				if(err) goto done;
			}
			
			/* Add the LI after the tag is closed */
			doLI = true;
		}
		
		/* Check for graphic that is a paragraph */
		/* First get the style and its length */
		if(pte) {
			err = PETEGetStyle(PETE, pte, offset, &runLen, &newStyle);
			if(err) goto done;
			runLen -= offset - newStyle.psStartChar;
		} else if(pslh != nil) {
			newStyle = (*pslh)[styleIndex++];
			if(InlineGetHandleSize(pslh) <= styleIndex * sizeof(PETEStyleEntry)) {
				runLen = (*pslh)[styleIndex].psStartChar - newStyle.psStartChar;
			} else {
				runLen = len - offset;
			}
		} else {
			runLen = len - offset;
		}
		
		//
		// If the style is a text style, save it for use next time
		// we see an emoticon
		if (!newStyle.psGraphic) lastTextStyle = newStyle;
		
		//
		// If the style is an emoticon, replace it with the last text style
		if (newStyle.psGraphic && IsEmoticonStyle(&newStyle.psStyle.graphicStyle))
		{
			long startChar = newStyle.psStartChar;
			newStyle = lastTextStyle;
			newStyle.psStartChar = startChar;
		}

		/* Check if the style is a graphic and it takes up the entire paragraph */
		if((paraInfo.paraLength == runLen) &&
		   newStyle.psGraphic &&
		   newStyle.psStyle.graphicStyle.graphicInfo &&
		   (((**newStyle.psStyle.graphicStyle.graphicInfo).privateType == pgtBodySigSep) || ((**newStyle.psStyle.graphicStyle.graphicInfo).privateType == pgtHorizontalRule))) {

			/* Close off the list and blockquote directives if any */
			if(((paraInfo.paraFlags & peListBits) || (newIndentCount != 0L) || (paraInfo.quoteLevel != 0)) && (changeAlign || changeIndent)) {
				if(changeIndent) {
//							err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlStyleAttr, ???, htmlNoAttr);
					if(err) goto done;
					changeIndent = false;
				}
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlAttrToken, htmlNoAttr);
			}
			
			err = StackPush(&paraAttr, paraStack);
			if(err) goto done;
		
			if((**newStyle.psStyle.graphicStyle.graphicInfo).privateType == pgtHorizontalRule) {
				err = HTMLAddHR(htmlContext, (HRGraphicHandle)newStyle.psStyle.graphicStyle.graphicInfo, !changeAlign);
				if(err) goto done;
					
				offset += runLen;
			} else {
				err = HTMLAddDirective(htmlContext, htmlBreakBefore, true, htmlTagToken, htmlSigSepDir, htmlNoAttr);
				if(err) goto done;
				
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlPreDir, htmlNoAttr);
				if(err) goto done;
				
				while(--runLen >= 0)
				{
					err = HTMLAddChar(htmlContext, (*text)[offset++], false, true);
					if(err) goto done;
				}
				
				err = HTMLAddDirective(htmlContext, htmlBreakBefore, true, htmlTagNotToken, htmlPreDir, htmlNoAttr);
				if(err) goto done;
				
				err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlSigSepDir, htmlNoAttr);
				if(err) goto done;
			}
			++styleIndex;
		} else {
			
			err = StackPush(&newParaAttr, paraStack);
			if(err) goto done;
			
			if(!(paraInfo.paraFlags & peListBits) && (newIndentCount == 0L) && (paraInfo.quoteLevel == 0)) {
				
				/* Just do a DIV */
				err = HTMLAddDirective(htmlContext, htmlBreakBefore, !(changeAlign || changeIndent), htmlTagToken, htmlDivDir, htmlNoAttr);
				if(err) goto done;
				wasDiv = true;
			}
		}
		
DoText :

		allReturns = true;
		
		/* Add the align and indent stuff */
		if(changeAlign) {
			err = HTMLAddDirective(htmlContext, htmlBreakNot, !changeIndent, htmlAttrToken, htmlAlignAttr, htmlResValue, (long)HTMLAlignStrn+newParaAttr.align, htmlNoAttr);
			if(err) goto done;
		}
		if(changeIndent) {
//			err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlAttrToken, htmlStyleAttr, ???, htmlNoAttr);
			if(err) goto done;
		}
		
		/* Do an LI tag if needed */
		if(doLI) {
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, true, htmlTagToken, htmlLIDir, htmlNoAttr);
			if(err) goto done;
		}
		
		/* Start on the styles */
		while(offset < MIN(len, paraInfo.paraOffset + paraInfo.paraLength)) {

			CycleBalls();
			if(CommandPeriod) {
				err = userCanceledErr;
				goto done;
			}

			if(pte) {
				err = PETEGetStyle(PETE, pte, offset, &runLen, &newStyle);
				if(err) goto done;
				runLen -= offset - newStyle.psStartChar;
			} else if(pslh != nil) {
				newStyle = (*pslh)[styleIndex++];
				if(InlineGetHandleSize(pslh) <= styleIndex * sizeof(PETEStyleEntry)) {
					runLen = (*pslh)[styleIndex].psStartChar - newStyle.psStartChar;
				} else {
					runLen = len - offset;
				}
			} else {
				runLen = len - offset;
			}
		
			//
			// If the style is a text style, save it for use next time
			// we see an emoticon
			if (!newStyle.psGraphic) lastTextStyle = newStyle;
			
			//
			// If the style is an emoticon, replace it with the last text style
			if (newStyle.psGraphic && IsEmoticonStyle(&newStyle.psStyle.graphicStyle))
			{
				long startChar = newStyle.psStartChar;
				newStyle = lastTextStyle;
				newStyle.psStartChar = startChar;
			}

			if(newStyle.psGraphic) {
				fgi = (FGIHandle)newStyle.psStyle.graphicStyle.graphicInfo;
				
				if((fgi == nil) && (newStyle.psStyle.graphicStyle.tsLabel & pLinkLabel)) {
					long tempOffset, tempRunLen;
					
					/* Undo all of the text level styles */
					err = HTMLRemoveStyles(htmlContext, 0xFFFF, nil, styleStack);
					if(err) goto done;
					
					/* Set the old style back to having no styles */
					if(pte) {
						err = PETEGetStyle(PETE, pte, kPETEDefaultStyle, nil, &oldStyle);
						if(err) goto done;
					} else {
						Zero(oldStyle);
						oldStyle.psStyle.textStyle.tsFont = kPETEDefaultFont;
						oldStyle.psStyle.textStyle.tsSize = kPETERelativeSizeBase;
						SetPETEDefaultColor(oldStyle.psStyle.textStyle.tsColor);
					}
					
					do {
						/* Start of a hidden tag; '<' run followed by run of URL, '>' run, and text */
						offset += runLen;
						
						/* Get the URL run */
						if(pte) {
							err = PETEGetStyle(PETE, pte, offset, &runLen, &newStyle);
							if(err) goto done;
						} else {
							newStyle = (*pslh)[styleIndex++];
						}
						
						fgi = (FGIHandle)newStyle.psStyle.graphicStyle.graphicInfo;

						/* Make sure we've got the URL run */
						if(newStyle.psGraphic && (fgi == nil) && (newStyle.psStyle.graphicStyle.tsLabel & pURLLabel)) {
							tempRunLen = runLen;
							tempOffset = offset;
							offset += runLen;

							/* Get the closing '>' run */
							if(pte) {
								err = PETEGetStyle(PETE, pte, offset, &runLen, &newStyle);
								if(err) goto done;
							} else {
								newStyle = (*pslh)[styleIndex++];
							}

							fgi = (FGIHandle)newStyle.psStyle.graphicStyle.graphicInfo;

							if(newStyle.psGraphic && (fgi == nil) && (newStyle.psStyle.graphicStyle.tsLabel & pLinkLabel)) {
								offset += runLen;

								/* Get the link run */
								if(pte) {
									err = PETEGetStyle(PETE, pte, offset, &runLen, &newStyle);
									if(err) goto done;
								} else {
									newStyle = (*pslh)[styleIndex++];
								}
								
								fgi = (FGIHandle)newStyle.psStyle.graphicStyle.graphicInfo;
							} else {
								tempRunLen = tempOffset = 0L;
							}
						} else {
							tempRunLen = tempOffset = 0L;
						}
					} while(newStyle.psGraphic && (fgi == nil) && (newStyle.psStyle.graphicStyle.tsLabel & pLinkLabel) && (offset < MIN(len, paraInfo.paraOffset + paraInfo.paraLength)));
										
					/* Add the anchor with the URL */
					err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlADir, htmlHrefAttr, htmlHandleValue, text, tempOffset, tempRunLen, htmlNoAttr);
					if(err) goto done;
					
					err = HTMLPushStyle(htmlADir, peLabelValid, styleStack);
					if(err) goto done;

					if((offset >= MIN(len, paraInfo.paraOffset + paraInfo.paraLength)) || !(newStyle.psStyle.textStyle.tsLabel & pLinkLabel))
					{
						continue;
					}

					if(newStyle.psGraphic) {
						goto DoGraphic;
					}
				} else {
			DoGraphic :
					/* It's a not a link. Make sure there is something there. */
					if(fgi != nil) {
						FSSpec spec;
						HTMLDirectiveEnum directive = htmlImgDir;
						MessHandle	messH;

						switch((**fgi).pgi.privateType) {
							/* HR hasn't been done yet, so put it in now and break. */
							case pgtHorizontalRule :
								err = HTMLAddHR(htmlContext, (HRGraphicHandle)fgi, true);
								allReturns = false;
								break;
							
							case pgtIndexPict :
								{
									unsigned char colon = ':';

									err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlTagToken, directive, htmlSrcAttr, htmlResValue, (long)ProtocolStrn+proPictRes, (**fgi).htmlInfo.pictResID, htmlNoAttr);
									if(err) goto done;
									goto FinishTag;
								}

							/* Icon means either non-inline image or missing graphic. */
							case pgtIcon :
								if(!(*fgi)->noImage) {
									goto GetSpec;

							/* Movies and Netscape plug-ins get the EMBED directive. */
							case pgtMovie:
								if (!(*fgi)->u.movie.animatedGraphic)	//	animated GIFs are images, not movies
							case pgtNPlugin:
									directive = htmlEmbedDir;
								}
								/* Missing graphic icons fall through here; wasDownloaded is false. */
							case pgtPICT :
							case pgtImage :
							case pgtResPict :
								if(!HTMLIsNetGraphic(fgi)) {
						GetSpec :
									spec = (*fgi)->spec;
									goto GotSpec;
								}

							case pgtHTMLPending :
								if ((**fgi).htmlInfo.absURL)
									err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlTagToken, directive, htmlSrcAttr, htmlHandleValue, (**fgi).htmlInfo.absURL, 1L, (long)(*(**fgi).htmlInfo.absURL)[0], htmlNoAttr);
								else
									err = noErr;	// no image found; so just omit the tag
								if(err) goto done;
								goto FinishTag;
							
							/* Pasted picture or PicHandle*/
							case 0 :
							case pgtPictHandle:
								if(!PeteIsValid(pte)) break;							
								if (!(messH = Pte2MessH(pte))) break;
								
								err = HTMLPictToDisk(&spec, messH, SumOf(messH)->uidHash,
									 (**fgi).pgi.privateType==pgtPictHandle ? (*fgi)->htmlInfo.pictHandle : *(PicHandle *)&(**fgi).pgi.privateData);
								if(err) goto done;
								
								{
								PETEStyleEntry tempStyle;
								
								err = PeteFileGraphicStyle(pte, &spec, nil, &tempStyle,fgDisplayInline);
								if(err) goto done;
								
								tempFGI = true;
								fgi = tempStyle.psStyle.graphicStyle.graphicInfo;
								
								}
								
						GotSpec :
								/* If there's no stack, it's local storage so use eudora-file URLs */
								if(specStack == nil) {
									if(!PeteIsValid(pte)) break;
									if (!(messH = Pte2MessH(pte))) break;
									err = HTMLPutInSpool(&spec, messH, SumOf(messH)->uidHash);
									if (err)
									{
										if (!alreadyWarnedAboutMissingGraphics)
										{
											WarnUser(AT_LEAST_ONE_GRAPHIC_MISSING,0);
											alreadyWarnedAboutMissingGraphics = true;
										}
										if (tempFGI)
										{
											DisposeHandle(fgi);
											tempFGI = false;
										}
										err = 0;
										goto PastAddDirective;
									}
									if ((*fgi)->pgi.privateType)	//	No storage for spec with pasted picture
										(*fgi)->spec = spec;	//	Now point to file in spool folder so we don't resave
									
									err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlTagToken, directive, htmlSrcAttr, htmlFileValue, &spec.name, htmlNoAttr);
									if(err)
									{
							SpecErr :
										if (errSpec) *errSpec = spec;
										goto done;
									}
								} else {
									/* If there's no message ID, it would be bad, so just go on */
									if(mid == nil) break;
									
									err = StackQueue(&spec, specStack);
									if(err) goto done;
									
									err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlTagToken, directive, htmlSrcAttr, htmlCIDValue, mid, partID, (**specStack).elCount - 1L, htmlNoAttr);
									if(err) goto done;
								                       
								}
						FinishTag :
								if((**fgi).pgi.privateType) {
									if((**fgi).htmlInfo.alt) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAltAttr, htmlHandleValue, (Handle)fgi, (long)offsetof(FileGraphicInfo, name) + 1, (long)(**fgi).name[0], htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.align) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAlignAttr, htmlResValue, (long)HTMLAlignStrn+(**fgi).htmlInfo.align, htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.width) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlWidthAttr, htmlNumValue, (long)(**fgi).htmlInfo.width, htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.height) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlHeightAttr, htmlNumValue, (long)(**fgi).htmlInfo.height, htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.border) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlBorderAttr, htmlNumValue, (long)(**fgi).htmlInfo.border, htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.hSpace) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlHSpaceAttr, htmlNumValue, (long)(**fgi).htmlInfo.hSpace, htmlNoAttr);
										if(err) goto done;
									}
									if((**fgi).htmlInfo.vSpace) {
										err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlVSpaceAttr, htmlNumValue, (long)(**fgi).htmlInfo.vSpace, htmlNoAttr);
										if(err) goto done;
									}
								}
								if(tempFGI)
								{
									DisposeHandle(fgi);
									tempFGI = false;
								}
								err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlAttrToken, htmlNoAttr);
							PastAddDirective :
								if(err) goto done;
								allReturns = false;
						}
					}
					oldStyle.psStyle.graphicStyle.tsLabel = newStyle.psStyle.graphicStyle.tsLabel;
					offset += runLen;
					continue;
				}
			}
			
			/* Now apply the text styles */
			
			if (HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, FontID)) newStyle.psStyle.textStyle.tsFont=kPETEDefaultFont;
			if (HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, FixedID)) newStyle.psStyle.textStyle.tsFont=kPETEDefaultFixed;
			if (newStyle.psStyle.textStyle.tsSize==0) newStyle.psStyle.textStyle.tsSize=ScriptVar(smScriptSysFondSize);
			if (newStyle.psStyle.textStyle.tsSize == (HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, kPETEDefaultFixed) ? FixedSize : FontSize)) newStyle.psStyle.textStyle.tsSize == kPETERelativeSizeBase;
			
			HTMLTextStyleDiff(&oldStyle.psStyle.textStyle,&newStyle.psStyle.textStyle,&styleOff,&styleOn);
			
			/* Unwind first */
			styleRemoved = 0;
			for(styleBit = 1; styleBit != 0; styleBit <<= 1) {
				if(styleOff & styleBit) {
					err = HTMLRemoveStyles(htmlContext, styleBit, &styleRemoved, styleStack);
					if(err) goto done;
				}
			}
			styleRemoved &= ~styleOff;
			styleOn |= styleRemoved;
			
			/* Now put the new styles on */
			if((styleOn & peFontValid) && (HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, FixedID) || HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, kPETEDefaultFixed))) {
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlTTDir, htmlNoAttr);
				if(err) goto done;
				styleOn &= ~peFontValid;
				err= HTMLPushStyle(htmlTTDir, peFontValid, styleStack);
				if(err) goto done;
			}
			if(styleOn & (peFontValid | peColorValid | peSizeValid)) {
				err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlTagToken, htmlFontDir, htmlNoAttr);
				if(err) goto done;
				
				if(styleOn & peFontValid) {
					err = HTMLAddDirective(htmlContext, htmlBreakNot, !(styleOn & (peColorValid | peSizeValid)), htmlAttrToken, htmlFaceAttr, htmlFontValue, (long)newStyle.psStyle.textStyle.tsFont, htmlNoAttr);
					if(err) goto done;
				}
				
				if(styleOn & peSizeValid) {
					err = HTMLAddDirective(htmlContext, htmlBreakNot, !(styleOn & peColorValid), htmlAttrToken, htmlSizeAttr, htmlSignedNumValue, HTMLGetSizeIndex(newStyle.psStyle.textStyle.tsSize, HTMLEqualFonts(newStyle.psStyle.textStyle.tsFont, FixedID)), htmlNoAttr);
					if(err) goto done;
				}
				
				if(styleOn & peColorValid) {
					err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlAttrToken, htmlColorAttr, htmlColorValue, &newStyle.psStyle.textStyle.tsColor, htmlNoAttr);
					if(err) goto done;
				}
				err = HTMLPushStyle(htmlFontDir, styleOn & (peFontValid | peColorValid | peSizeValid), styleStack);
				if(err) goto done;
			}
			if(styleOn & underline) {
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlUDir, htmlNoAttr);
				if(err) goto done;
				err = HTMLPushStyle(htmlUDir, underline, styleStack);
				if(err) goto done;
			}
			if(styleOn & italic) {
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlIDir, htmlNoAttr);
				if(err) goto done;
				err = HTMLPushStyle(htmlIDir, italic, styleStack);
				if(err) goto done;
			}
			if(styleOn & bold) {
				err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlBDir, htmlNoAttr);
				if(err) goto done;
				err = HTMLPushStyle(htmlBDir, bold, styleStack);
				if(err) goto done;
			}
			
			oldStyle = newStyle;

			/* Now insert the text */
			while((--runLen >= 0L) && (offset < MIN(len, paraInfo.paraOffset + paraInfo.paraLength))) {

				CycleBalls();
				if(CommandPeriod) {
					err = userCanceledErr;
					goto done;
				}
				
				switch((*text)[offset]) {
					case 13 :
						if((paraInfo.paraLength == 1L) || ((offset != paraInfo.paraOffset + paraInfo.paraLength - 1L) || allReturns)) {
							err = HTMLAddDirective(htmlContext, (paraInfo.paraLength == 1L)?htmlBreakNot:htmlBreakAfter, true, htmlTagToken, htmlBRDir, htmlNoAttr);
							charLen == 0L;
						}
						break;
					case 9 :
						err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagToken, htmlXTabDir, htmlNoAttr);
						if(err) goto done;
						
						{
							short tabLen = 8 - (charLen % 8);
							
							charLen += tabLen;
							while(--tabLen > 0) {
								err = HTMLAddChar(htmlContext, nbSpaceChar, true, true);
								if(err) goto done;
							}
						}
						err = HTMLAddChar(htmlContext, ' ', false, true);
						if(err) goto done;

						err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagNotToken, htmlXTabDir, htmlNoAttr);
						allReturns = false;
						break;
					case ' ' :
						if(((offset + 1 < len) && ((*text)[offset + 1] == ' ')) || (offset == 0) || ((*text)[offset - 1] == 13)) {
							err = HTMLAddChar(htmlContext, nbSpaceChar, true, true);
							++charLen;
							allReturns = false;
							break;
						}
					default :
						err = HTMLAddChar(htmlContext, (*text)[offset], false, true);
						++charLen;
						allReturns = false;
				}
				++offset;
				if(err) goto done;
			}
		}
		++paraIndex;
	}
	
	/* Clean up */
	
	/* Unwind the text styles */
	err = HTMLRemoveStyles(htmlContext, 0xFFFF, nil, styleStack);
	if(err) goto done;
	
	/* Undo the list, if any */
	if(listType) {
		err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlULDir, htmlNoAttr);
		if(err) goto done;
	}
	
	/* Unwind the BLOCKQUOTEs */
	quoteCount += indentCount;
	while(--quoteCount >= 0L) {
		err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlBQDir, htmlNoAttr);
		if(err) goto done;
	}
	
	/* Undo the DIV, if any */
	if(wasDiv) {
		err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlDivDir, htmlNoAttr);
		if(err) goto done;
	}
	
done :
	
	if(tempFGI)
	{
		DisposeHandle(fgi);
		tempFGI = false;
	}
	ZapHandle(styleStack);
	ZapHandle(paraStack);
	return err;
}

void HTMLInitOutContext(HTMLOutContext *htmlContext, AccuPtr html)
{
	htmlContext->html = html;
	htmlContext->newLine =
		htmlContext->lastSpace =
		htmlContext->lastEqual =
		htmlContext->lastGreater =
		htmlContext->lastChar = html->offset;
	htmlContext->eightBits = 0;
	htmlContext->lineLimit = GetRLong(FLOW_WRAP_SPOT);
	htmlContext->hardLimit = GetRLong(FLOW_WRAP_THRESH);
	GetRString(htmlContext->spanString, HTMLDirectiveStrn+htmlSpanDir);
	
	while(htmlContext->newLine > 0)
		if((*(html->data))[htmlContext->newLine - 1] == 13)
			break;
		else
			--htmlContext->newLine;
	
	while(htmlContext->lastSpace > 0)
		if((*(html->data))[--htmlContext->lastSpace] == ' ')
			break;
}

OSErr HTMLPreamble(AccuPtr html, StringPtr title, long id, Boolean local)
{
	OSErr err;
	Byte len;
	HTMLOutContext htmlContext;
	
	HTMLInitOutContext(&htmlContext, html);

	if (local)
	{
		err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagToken, htmlXHTMLDir, htmlNoAttr);
		if(err) return err;
		err = HTMLAddDirective(&htmlContext, htmlBreakAfter, true, htmlCommentToken, htmlPETEDir, htmlIDAttr, htmlNumValue, id, htmlNoAttr);
		if(err) return err;
	}
	err = HTMLAddDirective(&htmlContext, htmlBreakAfter, false, htmlCommentToken, htmlDoctypeDir, htmlNoAttr);
	if(err) return err;
	err = AccuAddChar(html, ' ');
	if(err) return err;
	err = AccuAddRes(html, HTML_DOCTYPE);
	if(err) return err;
	err = AccuAddChar(html, '>');
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakBefore, true, htmlTagToken, htmlHTMLDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagToken, htmlHeadDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagToken, htmlStyleDir, htmlTypeAttr, htmlResValue, (long)HTML_STYLE_TYPE, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, false, htmlCommentToken, htmlCommentDir, htmlNoAttr);
	if(err) return err;
	err = AccuAddChar(html, 13);
	if(err) return err;
	err = AccuAddRes(html, HTML_NOSPACESTYLE);
	if(err) return err;
	err = AccuAddChar(html, 13);
	if(err) return err;
	htmlContext.newLine = html->offset;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlAttrToken, htmlCommentAttr, htmlNoValue, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagNotToken, htmlStyleDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagToken, htmlTitleDir, htmlNoAttr);
	if(err) return err;
	for(len = *title++; len > 0; --len) {
		err = HTMLAddChar(&htmlContext, *title++, false, true);
		if(err) return err;
	}
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagNotToken, htmlTitleDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagNotToken, htmlHeadDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakAfter, true, htmlTagToken, htmlBodyDir, htmlNoAttr);
	return err;
}

OSErr HTMLPostamble(AccuPtr html, Boolean local)
{
	OSErr err;
	HTMLOutContext htmlContext;
	
	HTMLInitOutContext(&htmlContext, html);
	
	err = HTMLAddDirective(&htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlBodyDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlHTMLDir, htmlNoAttr);
	if(!err && local) {
		err = HTMLAddDirective(&htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlXHTMLDir, htmlNoAttr);
	}
	return err;
}

OSErr HTMLAddDirective(HTMLOutContext *htmlContext,HTMLBreakEnum breaks,Boolean close,HTMLTokenType token,...)
{
	OSErr err;
	HTMLAttributeEnum attr;
	HTMLValueTypeEnum valueType;
	StringPtr valuePtr;
	long len, offset;
	Str255 string;
	va_list args;
	va_start(args,token);
	
	if(token != htmlAttrToken) {
		if((breaks & htmlBreakBefore) && (((htmlContext->html)->offset == 0) || ((*(htmlContext->html)->data)[(htmlContext->html)->offset - 1] != 13))) {
			err = AccuAddChar(htmlContext->html, 13);
			if(err) goto Done;
			htmlContext->newLine = (htmlContext->html)->offset;
			htmlContext->eightBits = 0;
		}
		
		err = AccuAddChar(htmlContext->html, '<');
		if(err) goto Done;
		
		if((token == htmlCommentToken) || (token == htmlTagNotToken)) {
			err = AccuAddChar(htmlContext->html, (token == htmlCommentToken) ? '!' : '/');
			if(err) goto Done;
		}
		
		err = AccuAddRes(htmlContext->html, HTMLDirectiveStrn+va_arg(args,HTMLDirectiveEnum));
		if(err) goto Done;
	}
	
	while((attr = va_arg(args,HTMLAttributeEnum)) != htmlNoAttr) {
		
		CycleBalls();
		if(CommandPeriod) {
			err = userCanceledErr;
			break;
		}

		err = HTMLCheckLineBreak(htmlContext, true, true);
		if(err) break;
		
		err = AccuAddRes(htmlContext->html, HTMLAttributeStrn+attr);
		if(err) break;
		
		err = HTMLCheckLineBreak(htmlContext, true, false);
		if(err) break;
		
		if((valueType = va_arg(args,HTMLValueTypeEnum)) != htmlNoValue) {
		
			htmlContext->lastEqual = (htmlContext->html)->offset;

			err = AccuAddChar(htmlContext->html, '=');
			if(err) break;
			
			err = HTMLCheckLineBreak(htmlContext, true, false);
			if(err) break;

			valuePtr = string;
			switch(valueType) {
				case htmlFileValue :
					GetRString(string, ProtocolStrn+proCompFile);
					string[++string[0]] = ':';
					PCat(string, va_arg(args,StringPtr));
					goto DoURLEscape;
				case htmlCIDValue :
					GetRString(string, ProtocolStrn+proCID);
					{
						PStr mid;
						long part;
						short i;
						
						mid = va_arg(args,StringPtr);
						part = va_arg(args,long);
						i = va_arg(args,long);
						BuildContentID(&string[++string[0]],mid,part,i);
					}
					len = string[string[0]];
					string[string[0]] = ':';
					string[0] += len;
			DoURLEscape :
					URLEscapeLo(string, true);
					goto DoString;
				case htmlColorValue :
					HTMLColorToString(va_arg(args,RGBColorPtr), string);
					goto DoString;
				case htmlRelativeNumValue :
				case htmlPercentNumValue :
				case htmlSignedNumValue :
				case htmlNumValue :
					NumToString(va_arg(args,long), string);
					if((valueType == htmlSignedNumValue) && (string[1] != '-')) {
						BMD(&string[1], &string[2], string[0]++);
						string[1] = '+';
					}
					if(valueType == htmlPercentNumValue) {
						string[++string[0]] = '%';
					}
					if(valueType == htmlRelativeNumValue) {
						string[++string[0]] = '*';
					}
					goto DoString;
				case htmlFontValue :
					GetFontName(HTMLFontWithAClue(va_arg(args,long)), string);
					goto DoString;
				case htmlResValue :
					GetRString(string, va_arg(args,long));
					goto DoString;
				case htmlStringValue :
					valuePtr = va_arg(args,StringPtr);
			DoString :
					len = *valuePtr++;
					offset = -1L;
					goto AddIt;
				case htmlPtrValue :
					valuePtr = va_arg(args,StringPtr);
					offset = va_arg(args,long);
					len = va_arg(args,long);
					valuePtr += offset;
					offset = -1L;
					goto AddIt;
				case htmlHandleValue :
					valuePtr = va_arg(args,StringPtr);
					offset = va_arg(args,long);
					len = va_arg(args,long);
			AddIt :
					err = HTMLAddValue(htmlContext, valuePtr, offset, len);
			}
		}
	}
	
	err = HTMLCheckLineBreak(htmlContext, true, false);
	if(err) goto Done;

	if(close) {
		htmlContext->lastGreater = (htmlContext->html)->offset;
		err = AccuAddChar(htmlContext->html, '>');
		if(err) goto Done;
		
		if(breaks & htmlBreakAfter) {
			err = AccuAddChar(htmlContext->html, 13);
			htmlContext->newLine = (htmlContext->html)->offset;
			htmlContext->eightBits = 0;
		}
	}
	
Done :
	va_end(args);
	return(err);
}


OSErr HTMLAddValue(HTMLOutContext *htmlContext, StringPtr s, long offset, long len)
{
	unsigned char quote, c;
	OSErr err;
	
	quote = HTMLGetQuoteChar((offset == -1L) ? s : (*(StringHandle)s) + offset, len);
	
	err = AccuAddChar(htmlContext->html, quote);
	
	while((err == noErr) && (--len >= 0L)) {
		if(offset == -1L) {
			c = *s++;
		} else {
			c = (*(StringHandle)s)[offset++];
		}
		err = HTMLAddChar(htmlContext, c, false, false);
	}
	
	if(err == noErr) {
		err = AccuAddChar(htmlContext->html, quote);
	}
	return err;
}

OSErr HTMLAddChar(HTMLOutContext *htmlContext, unsigned char c, Boolean literal, Boolean allowBreak)
{
	static UHandle stringH = nil;
	static UHandle tableH = nil;
	OSErr err;
	Byte hState;
	long offset = 2;
	Boolean justSpace;
	
	CycleBalls();
	if(CommandPeriod) {
		return userCanceledErr;
	}
	
	justSpace = ((c == ' ') && allowBreak && !literal);
	
	err = HTMLCheckLineBreak(htmlContext, false, justSpace);
	if(err || justSpace) return err;
	
	if(allowBreak)
		htmlContext->lastChar = (htmlContext->html)->offset;
	
	switch(c) {
		default :
			if(!literal) {
				if(c > 127)
				{
					++htmlContext->eightBits;
				}
				else if(allowBreak && (c == 13))
				{
					htmlContext->newLine = (htmlContext->html)->offset + 1;
					htmlContext->eightBits = 0;
				}
				break;
			}
		case '"' :
		case '<' :
		case '>' :
		case '&' :
		case nbSpaceChar :
			err = AccuAddChar(htmlContext->html, '&');
			if(err != noErr) break;
			
			if(tableH == nil) {
				Handle tempH = GetResource_('taBL', TransOutTablID());
				if(err = ResError()) return err;
				if(tempH == nil) return resNotFound;
				
				tableH = tempH;
				hState = HGetState(tempH);
				HNoPurge(tempH);
				if((err = HandToHand(&tableH)) != noErr) return err;
				HSetState(tempH, hState);
			}
			
			if(stringH == nil) {
				stringH = GetResource_('STR#', HTMLLiteralsStrn);
				if(err = ResError()) return err;
			} else if(*stringH == nil) {
				LoadResource(stringH);
				if(err = ResError()) return err;
			}
			
			if(stringH == nil) return resNotFound;
			
			c = (*tableH)[c];
			
			while(--c != 0) {
				offset += (*stringH)[offset] + 1;
			}
			
			hState = HGetState(stringH);
			HNoPurge(stringH);
			err =  AccuAddFromHandle(htmlContext->html, stringH, offset + 1, (*stringH)[offset]);
			HSetState(stringH, hState);

			if(err != noErr) break;
			
			c = ';';
	}
	
	err = AccuAddChar(htmlContext->html, c);
	if(!err) return err;
	
	return HTMLCheckLineBreak(htmlContext, false, false);	
}

OSErr HTMLRemoveStyles(HTMLOutContext *htmlContext, short styleBits, short *styleRemoved, StackHandle styleStack)
{
	OSErr err;
	HTMLStyleAttr aStyle;
	
	while((styleBits != 0) && (StackPop(&aStyle, styleStack) == noErr)) {
		err = HTMLAddDirective(htmlContext, htmlBreakNot, true, htmlTagNotToken, aStyle.directive, htmlNoAttr);
		if(err) return err;
		
		if((styleBits != aStyle.styleBits) && (styleRemoved != nil)) {
			*styleRemoved |= (aStyle.styleBits & ~styleBits);
		}
		styleBits &= ~(aStyle.styleBits);
	}
	return noErr;
}

OSErr HTMLPushStyle(HTMLDirectiveEnum directive, short styleBits, StackHandle styleStack)
{
	HTMLStyleAttr aStyle;
	
	aStyle.directive = directive;
	aStyle.styleBits = styleBits;
	return StackPush(&aStyle, styleStack);
}

unsigned char HTMLGetQuoteChar(StringPtr s, long len)
{
	if((memchr(s, '"', len) == nil) || (memchr(s, '\'', len) != nil))
		return '"';
	else
		return '\'';
}

void HTMLTextStyleDiff(PETETextStylePtr oldStyle, PETETextStylePtr newStyle, short *styleOff, short *styleOn)
{
	short diff;
	
	diff = oldStyle->tsFace ^ newStyle->tsFace;
	*styleOff = oldStyle->tsFace & diff;
	*styleOn = newStyle->tsFace & diff;
	if (oldStyle->tsSize != newStyle->tsSize) {
		if((oldStyle->tsSize != kPETEDefaultSize) && (oldStyle->tsSize != kPETERelativeSizeBase)) {
			*styleOff |= peSizeValid;
		}
		if(HTMLGetSizeIndex(newStyle->tsSize, (HTMLEqualFonts(newStyle->tsFont, kPETEDefaultFixed) || HTMLEqualFonts(newStyle->tsFont, FixedID))) != 0) {
			*styleOn |= peSizeValid;
		}
	}
	if (!HTMLEqualFonts(oldStyle->tsFont, newStyle->tsFont)) {
		if(!HTMLEqualFonts(oldStyle->tsFont, kPETEDefaultFont)) {
			*styleOff |= peFontValid;
		}
		if(!HTMLEqualFonts(newStyle->tsFont, kPETEDefaultFont)) {
			*styleOn |= peFontValid;
		}
	}
	if (oldStyle->tsColor.red != newStyle->tsColor.red ||
			oldStyle->tsColor.blue != newStyle->tsColor.blue ||
			oldStyle->tsColor.green != newStyle->tsColor.green) {
		if(!IsPETEDefaultColor(oldStyle->tsColor)) {
			*styleOff |= peColorValid;
		}
		if(!IsPETEDefaultColor(newStyle->tsColor)) {
			*styleOn |= peColorValid;
		}
	}
	if((oldStyle->tsLabel & pLinkLabel) != (newStyle->tsLabel & pLinkLabel)) {
		if(newStyle->tsLabel & pLinkLabel) {
			*styleOn |= peLabelValid;
		} else {
			*styleOff |= peLabelValid;
		}
	}
}

short HTMLGetSizeIndex(short fontSize, Boolean fixed)
{
	long fontIndex, newFontIndex = -1L;
	short n = HandleCount(StdSizes);
	short defSize;
	
	if(fontSize < 0) {
		if(fontSize == kPETEDefaultSize) {
			return 0;
		} else {
			return fontSize - kPETERelativeSizeBase;
		}
	}
	
	if(fontSize == 0) {
		fontSize = LoWord(ScriptVar(smScriptSysFondSize));
	}
	
	if(fixed) {
		defSize = FixedSize;
	} else {
		defSize = FontSize;
	}
	
	if(fontSize == defSize) {
		return 0;
	}
	
	if(fontSize < defSize) {
		for(fontIndex = 0L; fontIndex < n && (*StdSizes)[fontIndex] < defSize; ++fontIndex) {
			if((newFontIndex == -1L) && (fontSize <= (*StdSizes)[fontIndex]))
				newFontIndex = fontIndex;
		}
		if(newFontIndex == -1L)
			newFontIndex == fontIndex - 1L;
	} else {
		for(fontIndex = n - 1L; fontIndex >= 0L && (*StdSizes)[fontIndex] > defSize; --fontIndex) {
			if((newFontIndex == -1L) && (fontSize >= (*StdSizes)[fontIndex]))
				newFontIndex = fontIndex;
		}
		if(newFontIndex == -1L)
			newFontIndex == fontIndex + 1L;
	}
	return newFontIndex - fontIndex;
}

void HTMLColorToString(RGBColorPtr color, StringPtr string)
{
	string[0] = 0;
	string[++string[0]] = '#';
	Bytes2Hex((UPtr)&color->red, 1, &string[++string[0]]);
	++string[0];
	Bytes2Hex((UPtr)&color->green, 1, &string[++string[0]]);
	++string[0];
	Bytes2Hex((UPtr)&color->blue, 1, &string[++string[0]]);
	++string[0];
}

long HTMLAccuStringToNum(AccuPtr a, StringPtr s)
{
	return HTMLAccuStringToNumLo(a, s, nil);
}

long HTMLAccuStringToNumLo(AccuPtr a, StringPtr s, Boolean *prop)
{
	long t;
	Byte i;
	Boolean p;
	
	if (!a->offset) return 0;
	
	HTMLCopyQuoteString(*a->data, a->offset, s);
	if(p = (s[s[0]] == '*')) {
		--s[0];
	} else if(s[s[0]] == '%') {
		BMD(&s[1], &s[2], s[0] - 1L);
		s[1] = '-';
	}
	if(prop != nil) *prop = p;
	for(i=1; i<=s[0]; ++i) {
		switch(s[i]) {
			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' :
			case '-' :
			case '+' :
				break;
			default :
				s[0] = i-1;
		}
	}
	if(s[0]!=0) {
		StringToNum(s, &t);
		return t;
	} else {
		return 0L;
	}
}

OSErr HTMLPictToDisk(FSSpecPtr spec, MessHandle messH, uLong uidHash, PicHandle thePict)
{
	Byte hState;
	short refN;
	long size;
	OSErr err;
	char buf[512] = {0};
	
	err = MakeAttSubFolder(messH, uidHash, spec);
	if(err) return err;

	spec->name[0] = 9;
	spec->name[1] = 'P';
	Bytes2Hex((StringPtr)&uidHash, 4, &spec->name[2]);
	err = UniqueSpec(spec, SHRT_MAX);
	if(err) return err;

	err = FSpCreate(spec, 'ttxt', 'PICT', smSystemScript);
	if(err) return err;

	if((err=FSpOpenDF(spec,fsRdWrPerm,&refN)) == noErr) {
		size = 512;
		if((err = AWrite(refN,&size,buf)) == noErr) {
			size = InlineGetHandleSize((Handle)thePict);
			hState = HGetState((Handle)thePict);
			HLock((Handle)thePict);
			err=AWrite(refN,&size,*(Handle)thePict);
			HSetState((Handle)thePict, hState);
			if(err == noErr) {
				err=TruncAtMark(refN);
			}
		}
		MyFSClose(refN);
	}
	if(err != noErr) {
		FSpDelete(spec);
	}
	return err;
}

OSErr HTMLPutInSpool(FSSpecPtr spec, MessHandle messH, uLong uidHash)
{
	FSSpec tempSpec;
	OSErr err;

	err = MakeAttSubFolder(messH, uidHash, &tempSpec);
	if(err) return err;
	
	if((spec->parID == tempSpec.parID) && (spec->vRefNum == tempSpec.vRefNum)) {
		return noErr;
	}
	
	tempSpec.name[0] = 9;
	tempSpec.name[1] = 'P';
	Bytes2Hex((StringPtr)&uidHash, 4, &tempSpec.name[2]);
	err = UniqueSpec(&tempSpec, SHRT_MAX);
	if(err) return err;

	if ((*messH)->hStationerySpec || Stationery)
		//	Stationery. Copy file instead of creating alias
		err = FSpDupFile(&tempSpec,spec,false,false);
	else
		err = MakeAFinderAlias(spec, &tempSpec);
	
	if (!err) *spec = tempSpec;

	return err;
}

OSErr HTMLAddHR(HTMLOutContext *htmlContext, HRGraphicHandle hrg, Boolean close)
{
	OSErr err;
	
	err = HTMLAddDirective(htmlContext, htmlBreakBoth, false, htmlTagToken, htmlHRDir, htmlNoAttr);
	if(err) return err;

	if((**hrg).hi) {
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlSizeAttr, htmlNumValue, (long)(**hrg).hi, htmlNoAttr);
		if(err) return err;
	}

	if((**hrg).wi && ((**hrg).wi != -100)) {
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlWidthAttr, (**hrg).wi < 0 ? htmlPercentNumValue : htmlNumValue, (long)ABS((**hrg).wi), htmlNoAttr);
		if(err) return err;
	}

	if(!(**hrg).groovy || close) {
		err = HTMLAddDirective(htmlContext, htmlBreakNot, close, htmlAttrToken, (**hrg).groovy ? htmlNoAttr : htmlNoShadeAttr, htmlNoValue, htmlNoAttr);
		if(err) return err;
	}
	return err;
}

OSErr HTMLWriteForBrowser(UHandle text, long htmlOffset, long textLen, short refNum)
{
	HTMLTokenContext htmlContext;
	HTMLOutContext htmlOutContext;
	HTMLTokenAndLen htmlToken;
	HTMLTokenType tagToken = htmlEndToken;
	HTMLDirectiveEnum lastDirective = htmlNoDir, curDirective = htmlNoDir;
	Str255 string;
	Accumulator a = {0L,0L,nil};
	Handle base = nil, src = nil;
	long id, baseLen, startOffset, writeLen;
	OSErr err = noErr;
	Boolean baseFound = false, inHead = false;
	Byte hState;
	
	HTMLInitTokenState(&htmlContext, text, htmlOffset, textLen);
	
	do {
		startOffset = htmlOffset;

		if((tagToken == htmlTagToken) && ((curDirective == htmlXmpDir) || (curDirective == htmlListDir) || (curDirective == htmlPlainDir))) {
			HTMLGetTokenForXMP(&htmlContext, &htmlToken, curDirective, string);
		} else {
			HTMLGetNextToken(&htmlContext, &htmlToken);
		}

		tagToken = htmlToken.tokenType;

		switch(htmlToken.tokenType) {
			case htmlEndToken :
				break;
			case htmlTagToken :
			case htmlTagNotToken :
			case htmlCommentToken :
			case htmlProcessToken :
				/* Go get the directive name, looping if it's not immediately first */
				do {
					htmlOffset += htmlToken.tokenLen;
					HTMLGetNextToken(&htmlContext, &htmlToken);
				} while((htmlToken.tokenType != htmlTagEndToken) &&
				        (htmlToken.tokenType != htmlNameToken) &&
				        (htmlToken.tokenType != htmlEndToken));
				
				/* Whoops! Ran out of tokens. */
				if(htmlToken.tokenType == htmlEndToken) {
					break;
				}

				/* We've got the name, so go get the directive number */
				if(htmlToken.tokenType == htmlNameToken) {
					NewPStr(*text + htmlOffset, htmlToken.tokenLen, string);
					curDirective = (HTMLDirectiveEnum)FindSTRNIndex(HTMLDirectiveStrn, string);
					htmlOffset += htmlToken.tokenLen;
					HTMLGetNextToken(&htmlContext, &htmlToken);
				/* If it's a null not tag, use whatever directive was last */
				} else if((tagToken == htmlTagNotToken) && (lastDirective != htmlNoDir)) {
					curDirective = lastDirective;
				} else if(tagToken == htmlCommentToken) {
					curDirective = htmlCommentDir;
				}
				
				if(tagToken == htmlTagToken) {
					lastDirective = curDirective;
				}
				
				switch(tagToken) {
					case htmlTagToken :
						switch(curDirective) {
							case htmlXHTMLDir :
								goto SkipDirective;
							case htmlPETEDir :
								goto DoPETEStuff;
							case htmlHeadDir :
								inHead = true;
								break;
							case htmlBaseDir :
								if(!inHead) break;
								baseFound = true;
								break;
							case htmlBodyDir :
								if(inHead) goto NotHead;
						}
						break;
						
					case htmlTagNotToken :
						switch(curDirective) {
							case htmlXHTMLDir :
								goto SkipDirective;
							case htmlHeadDir :
						NotHead :
								if(!baseFound && (base != nil)) {
									a.offset = 0L;
									HTMLInitOutContext(&htmlOutContext, &a);
									err = HTMLAddDirective(&htmlOutContext, htmlBreakBoth, true, htmlTagToken, htmlBaseDir, htmlHrefAttr, htmlHandleValue, base, 0L, baseLen, htmlNoAttr);
									if(!err) {
										hState = HGetState(a.data);
										HLock(a.data);
										writeLen = a.offset;
										err = AWrite(refNum, &writeLen, *a.data);
										HSetState(a.data, hState);
									}
								}
								baseFound = true;
								break;
						}
						break;
					case htmlCommentToken :
						switch(curDirective) {
							case htmlPETEDir :
						DoPETEStuff :
								if(!baseFound) {
									err = HTMLParsePETEStuff(&htmlContext, &htmlToken, &htmlOffset, &base, &src, &id, nil, &a, string);
									if(!err && ((base == nil) || ((baseLen = InlineGetHandleSize(base)) <= 0L))) {
										DisposeHandle(base);
										base = src;
										src = nil;
										if((baseLen = InlineGetHandleSize(base)) <= 0L) {
											ZapHandle(base);
										}
									}
									ZapHandle(src);
									if(err) break;
								}
							SkipDirective :
								while((htmlToken.tokenType != htmlTagEndToken) &&
								      (htmlToken.tokenType != htmlEndToken)) {
									htmlOffset += htmlToken.tokenLen;
									HTMLGetNextToken(&htmlContext, &htmlToken);
								}
								goto NextToken;
						}
				}
				if(err) break;
			default :
				hState = HGetState(text);
				HLock(text);
				writeLen = (htmlOffset + htmlToken.tokenLen) - startOffset;
				err = AWrite(refNum, &writeLen, *text + startOffset);
				HSetState(text, hState);
		}
	
	NextToken :
	
		htmlOffset += htmlToken.tokenLen;
	
		CycleBalls();
		if(!err && CommandPeriod) {
			err = userCanceledErr;
		}
	} while((htmlToken.tokenType != htmlEndToken) && (err == noErr));
	
	AccuZap(a);
	DisposeHandle(base);
	return err;
}
/*
OSErr HTMLToText(UHandle html, long htmlOffset, long textLen, AccuPtr out)
{
	HTMLTokenContext htmlContext;
	HTMLTokenAndLen htmlToken;
	HTMLTokenType tagToken = htmlEndToken;
	HTMLDirectiveEnum lastDirective = htmlNoDir, curDirective = htmlNoDir;
	Str255 string;
	Accumulator a = {0L,0L,nil};
	Handle base = nil, src = nil;
	long id, baseLen, startOffset, writeLen;
	OSErr err = noErr;
	Boolean baseFound = false, inHead = false;
	Byte hState;
	
	HTMLInitTokenState(&htmlContext, html, htmlOffset, textLen);
	
	do {
		startOffset = htmlOffset;

		if((tagToken == htmlTagToken) && ((curDirective == htmlXmpDir) || (curDirective == htmlListDir) || (curDirective == htmlPlainDir))) {
			HTMLGetTokenForXMP(&htmlContext, &htmlToken, curDirective, string);
		} else {
			HTMLGetNextToken(&htmlContext, &htmlToken);
		}

		tagToken = htmlToken.tokenType;
		
}
*/
Boolean HTMLHasFrom(AccuPtr html, long offset)
{
	if(html->offset < offset + 6) return false;
	return (((*html->data)[++offset] == 'F') &&
		((*html->data)[++offset] == 'r') &&
		((*html->data)[++offset] == 'o') &&
		((*html->data)[++offset] == 'm') &&
		((*html->data)[++offset] == ' '));
		
}

Boolean HTMLEqualFonts(short fontID1, short fontID2)
{
	return HTMLFontWithAClue(fontID1) == HTMLFontWithAClue(fontID2);
}

short HTMLFontWithAClue(short fontID)
{
	if(fontID == systemFont) return GetSysFont();
	if(fontID == applFont) return GetAppFont();
	return fontID;
}


/************************************************************************
 * Pte2MessH - get message handle if window is message or comp
 ************************************************************************/
MessHandle Pte2MessH(PETEHandle pte)
{
	MyWindowPtr	pteWin = (*PeteExtra(pte))->win;
	WindowPtr	pteWinWP = GetMyWindowWindowPtr (pteWin);
	if (GetWindowKind(pteWinWP)==MESS_WIN || GetWindowKind(pteWinWP)==COMP_WIN)
		return Win2MessH(pteWin);
	else
		return nil;
}

Boolean HTMLIsNetGraphic(FGIHandle fgi)
{
	return((*fgi)->wasDownloaded || (*fgi)->notDownloaded);
}

OSErr AddCell(MatrixHandle matrix, short colspan, short rowspan)
{
	short row, col, rows, cols, newCols, newRows, r, c;
	
	row = (**matrix).row;
	if(row < 0) row = 0;
	col = (**matrix).col;
	rows = (**(**matrix).table).rows;
	cols = (**(**matrix).table).columns;
	newCols = MAX(((col + colspan) - cols), 0);
	newRows = MAX(((row + rowspan) - rows), 0);
	if(newCols || newRows)
	{
		OSErr err;
		
		SetHandleSize(matrix, sizeof(MatrixRec) + (rows + newRows) * (cols + newCols));
		if((err=MemError()) != noErr) return err;

		if(newCols)
		{
			Boolean *from, *to;
			
			from = &(**matrix).cells[rows*cols];
			to = &(**matrix).cells[(rows + newRows)*(cols + newCols)];
			for(r = rows; --r > 0; )
			{
				for(c = newCols; c > 0; --c)
				{
					*--to = false;
				}
				for(c = cols; c > 0; --c)
				{
					*--to = *--from;
				}
			}
		}
		
		if(newRows)
		{
			WriteZero(&(**matrix).cells[rows*(cols+newCols)], (cols+newCols) * newRows);
		}
	}

	(**(**matrix).table).rows += newRows;
	(**(**matrix).table).columns += newCols;
	cols = (**(**matrix).table).columns;
	
	for(c = colspan; --c >= 0;)
	{
		for(r = rowspan; --r >= 0;)
		{
			(**matrix).cells[((row + r) * cols) + col + c] = true;
		}
	}

	col += colspan;
	while((col < cols) && (**matrix).cells[(row * cols) + col])
	{
		++col;
	}
	(**matrix).col = col;
	return noErr;
}

/************************************************************************
 * TurnIntoEudoraHTML - given a handle full of HTML, turn it into
 *	something we can render
 ************************************************************************/
OSErr TurnIntoEudoraHTML(Handle *t)
{
	OSErr err = noErr;
	HTMLOutContext htmlContext;
	Accumulator html;
	long startOffset;
	
	err = AccuInit(&html);
	if(err) return err;
	
	HTMLInitOutContext(&htmlContext, &html);

	// Preramble
	err = HTMLAddDirective(&htmlContext, htmlBreakNot, true, htmlTagToken, htmlXHTMLDir, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakAfter, true, htmlCommentToken, htmlPETEDir, htmlIDAttr, htmlNumValue, 0, htmlNoAttr);
	if(err) return err;
	err = HTMLAddDirective(&htmlContext, htmlBreakAfter, false, htmlCommentToken, htmlDoctypeDir, htmlNoAttr);
	if(err) return err;
	err = AccuAddChar(htmlContext.html, ' ');
	if(err) return err;
	err = AccuAddRes(htmlContext.html, HTML_DOCTYPE);
	if(err) return err;
	err = AccuAddChar(htmlContext.html, '>');
	if(err) return err;
	
	// text
	err = AccuAddHandle(htmlContext.html, *t);
	ZapHandle(*t);
	if (err) return (err);
	
	// Postramble
	startOffset = htmlContext.html->offset;				
	err = HTMLAddDirective(&htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlXHTMLDir, htmlNoAttr);
	if(err) return err;

	// Cleanup
	AccuTrim(htmlContext.html);
	*t = htmlContext.html->data;
	Zero(*htmlContext.html);			
	
	return (err);				
}

OSErr HTMLCheckLineBreak(HTMLOutContext *htmlContext, Boolean allowBreakHere, Boolean wantSpace)
{
	long curLen, equalOrGreater, hardOffset;
	OSErr err = noErr;
	
	curLen = ((htmlContext->html)->offset - htmlContext->newLine) + (htmlContext->eightBits * 2);
	hardOffset = htmlContext->newLine + htmlContext->hardLimit;
	equalOrGreater = MAX(MAX(MIN(hardOffset, htmlContext->lastEqual), MIN(hardOffset, htmlContext->lastGreater)), MIN(hardOffset, htmlContext->lastEqual+1));
	if(curLen >= htmlContext->lineLimit)
	{
		if(curLen < htmlContext->hardLimit && wantSpace)
		{
			err = AccuAddChar(htmlContext->html, 13);
			htmlContext->newLine = (htmlContext->html)->offset;
			htmlContext->eightBits = 0;
		}
		else if(htmlContext->lastSpace > htmlContext->newLine && htmlContext->lastSpace < hardOffset)
		{
			(*(htmlContext->html)->data)[htmlContext->lastSpace] = 13;
			htmlContext->newLine = htmlContext->lastSpace + 1;
			for(curLen = (htmlContext->html)->offset, htmlContext->eightBits = 0; --curLen > htmlContext->newLine; )
			{
				if((*(htmlContext->html)->data)[curLen] > 127)
					++htmlContext->eightBits;
			}
		}
		else if(curLen < htmlContext->hardLimit && allowBreakHere)
		{
			err = AccuAddChar(htmlContext->html, 13);
			htmlContext->newLine = (htmlContext->html)->offset;
			htmlContext->eightBits = 0;
		}
		else if(equalOrGreater < hardOffset && equalOrGreater > htmlContext->newLine)
		{
			err = AccuInsertChar(htmlContext->html, 13, equalOrGreater);
			htmlContext->newLine = ++equalOrGreater;
			if(htmlContext->lastSpace > equalOrGreater) ++htmlContext->lastSpace;
			if(htmlContext->lastEqual > equalOrGreater) ++htmlContext->lastEqual;
			if(htmlContext->lastGreater > equalOrGreater) ++htmlContext->lastGreater;
			if(htmlContext->lastChar > equalOrGreater) ++htmlContext->lastChar;
			for(htmlContext->eightBits = 0; equalOrGreater < (htmlContext->html)->offset; ++equalOrGreater)
			{
				if((*(htmlContext->html)->data)[equalOrGreater] > 127)
					++htmlContext->eightBits;
			}
		}
		else if(htmlContext->lastChar > htmlContext->newLine)
		{
			long offset;
			
			offset = htmlContext->lastChar;
			err = AccuInsertChar(htmlContext->html, '<', offset++);
			if(err) return err;
			err = AccuInsertPtr(htmlContext->html, &htmlContext->spanString[1], htmlContext->spanString[0], offset);
			if(err) return err;
			offset += htmlContext->spanString[0];
			err = AccuInsertChar(htmlContext->html, 13, offset++);
			if(err) return err;
			htmlContext->newLine = offset;
			err = AccuInsertChar(htmlContext->html, '>', offset++);
			if(err) return err;
			err = AccuInsertChar(htmlContext->html, '<', offset++);
			if(err) return err;
			err = AccuInsertChar(htmlContext->html, '/', offset++);
			if(err) return err;
			err = AccuInsertPtr(htmlContext->html, &htmlContext->spanString[1], htmlContext->spanString[0], offset);
			if(err) return err;
			offset += htmlContext->spanString[0];
			htmlContext->lastGreater = offset;
			err = AccuInsertChar(htmlContext->html, '>', offset++);
			if(htmlContext->lastSpace >= htmlContext->lastChar) htmlContext->lastSpace += offset - htmlContext->lastChar;
			if(htmlContext->lastEqual >= htmlContext->lastChar) htmlContext->lastEqual += offset - htmlContext->lastChar;
			for(htmlContext->eightBits = 0; offset < (htmlContext->html)->offset; ++offset)
			{
				if((*(htmlContext->html)->data)[offset] > 127)
					++htmlContext->eightBits;
			}
		}
	}
	else if(wantSpace)
	{
		htmlContext->lastSpace = (htmlContext->html)->offset;
		err = AccuAddChar(htmlContext->html, ' ');
	}
	return err;
}

OSErr HTMLBuildTable(HTMLOutContext *htmlContext,Handle text,long offset,TableHandle t,long partID,PStr mid,StackHandle specStack,FSSpecPtr errSpec)
{
	OSErr err;
	TabCellHandle c = (**t).cellData;
	long cIndex, numC;
	
	numC = c ? GetHandleSize(c) / sizeof(TabCellData) : 0;
	
	/* First the TABLE tag */
	err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, htmlTableDir, htmlNoAttr);
	if(err) return err;
	if((**t).align)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAlignAttr, htmlResValue, (long)HTMLAlignStrn+(**t).align, htmlNoAttr);
		if(err) return err;
	}
	
	if((**t).direction != kHilite)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlDirAttr, htmlResValue, (long)HTMLAlignStrn+((**t).direction ? htmlRTLDir : htmlLTRDir), htmlNoAttr);
	}
	
	if((**t).border)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlBorderAttr, htmlNumValue, (**t).border, htmlNoAttr);
		if(err) return err;
	}
	if((**t).cellSpacingSpecified)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlCellSpacingAttr, htmlNumValue, (**t).cellSpacing, htmlNoAttr);
		if(err) return err;
	}
	if((**t).cellPadding)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlCellPaddingAttr, htmlNumValue, (**t).cellPadding, htmlNoAttr);
		if(err) return err;
	}
	err = HTMLAddMeasureAttr(htmlContext, htmlBreakAfter, true, htmlWidthAttr, (**t).width, false);
	if(err) return err;
	
	/* Caption; could be more than 1 if broken */
	for(cIndex = 0; cIndex < numC && (*c)[cIndex].row < 0; ++cIndex)
	{
		TabAlignData align;
		
		err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, htmlCaptionDir, htmlNoAttr);
		if(err) return err;
		
		align = (*c)[cIndex].align;
		err = HTMLAddAlignAttr(htmlContext, htmlBreakNot, true, &align);
		if(err) return err;
		
		if((*c)[cIndex].textLength)
		{
			err = BuildHTMLLo(htmlContext, nil, text, (*c)[cIndex].textLength, (*c)[cIndex].textOffset + offset, (*c)[cIndex].styleInfo, (*c)[cIndex].paraInfo, partID, mid, specStack, errSpec);
			if(err) return err;
		}
		
		err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, htmlCaptionDir, htmlNoAttr);
		if(err) return err;
	}
	
	/* Now the COLGROUP and/or COL tags */
	if((**t).columnGroupData || (**t).columnData)
	{
		TabColHandle g = (**t).columnGroupData, cols = (**t).columnData;
		long numG, numCols, colIndex, gIndex;
		TabColData col;
		short directive;
		Boolean openGroup = false;
		
		numG = g ? GetHandleSize(g) / sizeof(TabColData) : 0;
		numCols = cols ? GetHandleSize(cols) / sizeof(TabColData) : 0;
		
		for(colIndex = gIndex = 0; colIndex < numCols || gIndex < numG; )
		{
			if(colIndex == numCols)
			{
				directive = htmlColGroupDir;
				col = (*g)[gIndex];
			}
			else if(gIndex == numG)
			{
				directive = htmlColDir;
				col = (*cols)[colIndex];
			}
			else if((*cols)[colIndex].column < (*g)[gIndex].column)
			{
				directive = htmlColDir;
				col = (*cols)[colIndex];
				++colIndex;
			}
			else
			{
				directive = htmlColGroupDir;
				col = (*g)[gIndex];
				++gIndex;
			}
			
			if(directive == htmlColGroupDir)
			{
				if(openGroup)
				{
					err = HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagToken, htmlColGroupDir, htmlNoAttr);
					if(err) return err;
				}
				openGroup = true;
			}
			
			err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, directive, htmlNoAttr);
			if(err) return err;
			
			if(col.span > 1)
			{
				err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlSpanAttr, htmlNumValue, col.span, htmlNoAttr);
				if(err) return err;
			}
			
			err = HTMLAddMeasureAttr(htmlContext, htmlBreakNot, false, htmlWidthAttr, col.width, col.propWidth);
			if(err) return err;
			
			err = HTMLAddAlignAttr(htmlContext, htmlBreakAfter, true, &col.align);
			if(err) return err;
		}

		if(openGroup)
		{
			err = HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagToken, htmlColGroupDir, htmlNoAttr);
			if(err) return err;
		}
	}
	
	/* Now the rows and cells */
	{
		TabRowGroupHandle g = (**t).rowGroupData;
		TabRowHandle r = (**t).rowData;
		long numG, numR, gIndex, rIndex;
		short row;
		
		numG = g ? GetHandleSize(g) / sizeof(TabRowGroupData) : 0;
		numR = r ? GetHandleSize(r) / sizeof(TabRowData) : 0;
		
		for(gIndex = rIndex = 0; gIndex < numG || rIndex < numR || cIndex < numC; )
		{
			TabAlignData align = {0,0,0,0,kHilite};
			
			if(gIndex < numG && (rIndex == numR || (*g)[gIndex].row <= (*r)[rIndex].row) && (cIndex == numC || (*g)[gIndex].row <= (*c)[cIndex].row))
			{
				if(gIndex != 0)
				{
					err = HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagNotToken, (*g)[gIndex-1].rowGroup == tableHeadRowGroup ? htmlTHeadDir : ((*g)[gIndex-1].rowGroup == tableFootRowGroup ? htmlTFootDir : htmlTBodyDir), htmlNoAttr);
					if(err) return err;
				}
				row = (*g)[gIndex].row;
				err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, (*g)[gIndex].rowGroup == tableHeadRowGroup ? htmlTHeadDir : ((*g)[gIndex].rowGroup == tableFootRowGroup ? htmlTFootDir : htmlTBodyDir), htmlNoAttr);
				if(err) return false;
				
				align = (*g)[gIndex].align;
				++gIndex;

				err = HTMLAddAlignAttr(htmlContext, htmlBreakAfter, true, &align);
				if(err) return err;
				
			}
			else
			{
				row = (*g)[gIndex].row;
				err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, htmlTRDir, htmlNoAttr);
				if(rIndex < numR && (cIndex == numC || (*r)[rIndex].row <= (*c)[cIndex].row))
				{
					row = (*r)[rIndex].row;
					align = (*r)[rIndex].align;
					++rIndex;
				}

				err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlAttrToken, htmlNoAttr);
				if(err) return err;
				
				for(; cIndex < numC && (*c)[cIndex].row == row; ++cIndex)
				{
					err = HTMLAddDirective(htmlContext, htmlBreakBefore, false, htmlTagToken, (*c)[cIndex].header ? htmlTHDir : htmlTDDir, htmlNoAttr);
					if(err) return err;
					
					if((*c)[cIndex].rowSpan)
					{
						err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlRowSpanAttr, htmlNumValue, (*c)[cIndex].rowSpan, htmlNoAttr);
						if(err) return err;
					}
					
					if((*c)[cIndex].colSpan)
					{
						err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlColSpanAttr, htmlNumValue, (*c)[cIndex].colSpan, htmlNoAttr);
						if(err) return err;
					}
					
					err = HTMLAddMeasureAttr(htmlContext, htmlBreakNot, false, htmlWidthAttr, (*c)[cIndex].width, false);
					if(err) return err;

					err = HTMLAddMeasureAttr(htmlContext, htmlBreakNot, false, htmlHeightAttr, (*c)[cIndex].height, false);
					if(err) return err;
					
					if((*c)[cIndex].abbr)
					{
						err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAbbrAttr, htmlHandleValue, (*c)[cIndex].abbr, 0L, GetHandleSize((*c)[cIndex].abbr), htmlNoAttr);
					}

					align = (*c)[cIndex].align;
					err = HTMLAddAlignAttr(htmlContext, htmlBreakNot, true, &align);
					if(err) return err;
					
					if((*c)[cIndex].textLength)
					{
						err = BuildHTMLLo(htmlContext, nil, text, (*c)[cIndex].textLength, (*c)[cIndex].textOffset + offset, (*c)[cIndex].styleInfo, (*c)[cIndex].paraInfo, partID, mid, specStack, errSpec);
						if(err) return err;
					}
					
					err = HTMLAddDirective(htmlContext, htmlBreakAfter, true, htmlTagNotToken, (*c)[cIndex].header ? htmlTHDir : htmlTDDir, htmlNoAttr);
					if(err) return err;
				}
				err = HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlTRDir, htmlNoAttr);
				if(err) return err;
			}
		}
		if(numG != 0)
		{
			err = HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagNotToken, (*g)[numG-1].rowGroup == tableHeadRowGroup ? htmlTHeadDir : ((*g)[numG-1].rowGroup == tableFootRowGroup ? htmlTFootDir : htmlTBodyDir), htmlNoAttr);
			if(err) return err;
		}
	}
	return HTMLAddDirective(htmlContext, htmlBreakBoth, true, htmlTagNotToken, htmlTableDir, htmlNoAttr);
}

OSErr HTMLAddMeasureAttr(HTMLOutContext *htmlContext, HTMLBreakEnum breaks, Boolean close, HTMLAttributeEnum attr, short measure, Boolean prop)
{
	Boolean neg;
	
	if((neg = (measure < 0))) measure = -measure;
	return HTMLAddDirective(htmlContext, breaks, close, htmlAttrToken, (!measure && !prop) ? htmlNoAttr : attr, neg ? htmlPercentNumValue : (prop ? htmlRelativeNumValue : htmlNumValue), measure, htmlNoAttr);
}

OSErr HTMLAddAlignAttr(HTMLOutContext *htmlContext, HTMLBreakEnum breaks, Boolean close, TabAlignData *align)
{
	OSErr err = noErr;
	
	if(align->hAlign)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAlignAttr, htmlResValue, (long)HTMLAlignStrn+align->hAlign, htmlNoAttr);
		if(err) return err;
		
		if(align->hAlign == htmlCharAlign)
		{
			unsigned char alignChar;
			
			alignChar = align->alignChar;
			err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlCharAttr, htmlPtrValue, &alignChar, 0L, 1L, htmlNoAttr);
			if(err) return err;
			
			err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlCharOffAttr, htmlNumValue, align->charOff, htmlNoAttr);
			if(err) return err;
		}
	}
	if(align->vAlign)
	{
		err = HTMLAddDirective(htmlContext, htmlBreakNot, false, htmlAttrToken, htmlAlignAttr, htmlResValue, (long)HTMLAlignStrn+align->vAlign, htmlNoAttr);
		if(err) return err;
	}
	
	return HTMLAddDirective(htmlContext, breaks, close, htmlAttrToken, (align->direction == kHilite) ? htmlNoAttr : htmlDirAttr, htmlResValue, (long)HTMLAlignStrn+(align->direction ? htmlRTLDir : htmlLTRDir), htmlNoAttr);
}
