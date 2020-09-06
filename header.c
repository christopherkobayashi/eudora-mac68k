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

#include "header.h"
#define FILE_NUM 56
/* Copyright (c) 1993 by QUALCOMM Incorporated */
/************************************************************************
 * higher analysis of RFC 822 headers  (Blech)
 ************************************************************************/

#pragma segment Headers

Token822Enum WriteHeaderToken(Token822Enum tokenType, L822SPtr l822p,
			      AccuPtr a);
OSErr AddHeaderAttribute(HeaderDPtr(*hdh), PStr value, Boolean etl);
OSErr AddFunField(HeaderDPtr(*hdh), PStr value, short funFieldsID);
void TextPlain(HeaderDHandle hdh);

void Ungrok(HeaderDHandle hdh);
void Ungrok(HeaderDHandle hdh)
{
	// we do not full understand this header block
	(*hdh)->grokked = False;
}

#define OkStateAtWhichToEnd(s) ((s)==ExpectText || (s)==ExpectSem || (s)==ExpectVersion || (s)==ExpectStructuredValue)
/************************************************************************
 * ReadHeader - read a header block
 ************************************************************************/
OSErr ReadHeader(TransStream stream, HeaderDHandle hdh, long estSize,
		 short refN, Boolean isDigest)
{
	return ReadHeaderLo(stream, hdh, estSize, refN, isDigest,
			    InterestHeadStrn, InterestHeadLimit);
}

OSErr ReadHeaderLo(TransStream stream, HeaderDHandle hdh, long estSize,
		   short refN, Boolean isDigest, short funFieldsID,
		   short funFieldsLimit)
{
#pragma unused(estSize)
	short err;
	Str31 interesting[InterestHeadLimit + 1];
	Str31 scratch;
	short h;
	Boolean junk;
	Lex822State l822s;
	Token822Enum tokenType;
	Handle grumble;
	long lastNewline;
	long lastParam;
	AttributeEnum attr;
#ifndef SAVE_MIME
	short defaultTableId;
	short xlateResID;
#endif
	Accumulator fullHeaders;
	long hash;
	Accumulator a;

#define EXPECT_TEXT do{l822s.state = CollectText; (*hdh)->state = ExpectText;}while(0)

#ifndef SAVE_MIME
	defaultTableId = GetPrefLong(PREF_IN_XLATE);
	if (!defaultTableId)
		defaultTableId = TRANS_IN_TABL;
#endif

	if (funFieldsLimit > InterestHeadLimit)
		return paramErr;

	/*
	 * grab the interesting header labels
	 */
	for (h = 1; h < funFieldsLimit; h++)
		GetRString(interesting[h], funFieldsID + h);

	if (err = AccuInit(&a)) {
		WarnUser(MEM_ERR, MemError());
		return (ErrorToken);
	}

	/*
	 * initialize the attribute array
	 */
	ZapHandle((*hdh)->contentAttributes);
	if (!
	    (grumble =
	     (void *) AANew(GetRLong(MIME_ATTR_MAX),
			    GetRLong(MIME_VAL_MAX)))) {
		AccuZap(a);
		WarnUser(MEM_ERR, MemError());
		return (ErrorToken);
	}
	(*hdh)->contentAttributes = (void *) grumble;

	/*
	 * initialize the header array
	 */
	if (!(grumble = (void *) AANew(GetRLong(MIME_ATTR_MAX), 256))) {
		AccuZap(a);
		WarnUser(MEM_ERR, MemError());
		return (ErrorToken);
	}
	*GlobalTemp = 0;
	AAFetchResData((*hdh)->funFields, InterestHeadStrn + hContentBase,
		       GlobalTemp);
	ZapHandle((*hdh)->funFields);
	(*hdh)->funFields = (void *) grumble;
	if (*GlobalTemp)
		AAAddResItem((*hdh)->funFields, True,
			     InterestHeadStrn + hContentBase, GlobalTemp);

	/*
	 * make a copy of the full header accumulator
	 */
	fullHeaders = (*hdh)->fullHeaders;
	fullHeaders.offset = 0;

	/*
	 * record where we started
	 */
	if (refN && (err = GetFPos(refN, &LDRef(hdh)->diskStart))) {
		WarnUser(WRITE_MBOX, err);
		return (ErrorToken);
	}
	UL(hdh);
	lastNewline = (*hdh)->diskStart;
	TextPlain(hdh);

	/*
	 * initialize the lexical analyzer
	 */
	l822s.state = Init822;

	/*
	 * read the header
	 */
	for (tokenType = Lex822(stream, &l822s, &fullHeaders);;
	     tokenType = Lex822(stream, &l822s, &fullHeaders)) {
		/*
		 * Backward compatibility for old digests
		 * I hate this stupid kluge
		 */
		if (isDigest && tokenType == EndOfHeader) {
			isDigest = False;
			tokenType = Lex822(stream, &l822s, &fullHeaders);
			if (tokenType == Special)
				continue;
		}
		isDigest = False;

		/*
		 * write the token to disk
		 */
		if (!l822s.has1522) {
#ifndef SAVE_MIME
			if ((*hdh)->xlateResID)
				TransLitRes(l822s.token + 1, *l822s.token,
					    (*hdh)->xlateResID);
			else if ((*hdh)->isMIME && !(*hdh)->xlateResID)
				TransLitRes(l822s.token + 1, *l822s.token,
					    defaultTableId);
#endif
		} else if (PrefIsSet(PREF_ALWAYS_CHARSET)) {
			(*hdh)->hasCharset = AnyCharset = true;
		}
		if (refN)
			tokenType =
			    WriteHeaderToken(tokenType, &l822s, &a);
		if (tokenType == t822Comment)
			tokenType = LinearWhite;

		// if the header is Big, write all now
		if (refN && a.offset > 16 K) {
			err = AccuWrite(&a, refN);
			a.offset = 0;
			if (err) {
				WarnUser(WRITE_MBOX, err);
				tokenType = ErrorToken;
				goto out;
			}
		}

		/*
		 * first, we handle a few special token types
		 */
		switch (tokenType) {
			/*
			 * any of these things terminate the header collection
			 */
		case EndOfHeader:
		case EndOfMessage:
		case ErrorToken:
			goto out;
			break;

			/*
			 * if we have a return, we've hit the end of a header field
			 * the header can only legally end in ExpectText or ExpectSem or ExpectVersion states
			 */
		case Special:
			if (l822s.token[1] == '\015') {
				if (!OkStateAtWhichToEnd((*hdh)->state)) {
					Ungrok(hdh);
					if ((*hdh)->state == ExpectSlash)
						TextPlain(hdh);	/* hack for NASA's semi-MIME gateway */
				}
				(*hdh)->state = ExpectHeaderName;
				if (refN)
					lastNewline = AccuFTell(&a, refN);
				lastNewline--;
				break;
			}
			/* fall-through */

		default:
			/*
			 * we handle the remaining tokens on a state-by-state basis
			 */
			switch ((*hdh)->state) {
					/*********************************/
			case ExpectHeaderName:
				switch (tokenType) {
				case LinearWhite:
					break;	/* ignore */

				case Atom:
					for (h = 1; h < funFieldsLimit;
					     h++)
						if (StringSame
						    (interesting[h],
						     l822s.token))
							break;
					(*hdh)->hFound = h;
					(*hdh)->state = ExpectColon;
					if (funFieldsID != InterestHeadStrn
					    || h > hMimeVersion)
						Ungrok(hdh);
					break;

				default:
					(*hdh)->hFound = 0;
					Ungrok(hdh);
					EXPECT_TEXT;
					break;
				}
				break;

					/*********************************/
			case ExpectColon:
				switch (tokenType) {
				case LinearWhite:
					break;	/* ignore */

				case Special:
					if (l822s.token[1] == ':') {
						if (funFieldsID ==
						    InterestHeadStrn) {
							switch ((*hdh)->
								hFound) {
							case hContentType:
								(*hdh)->
								    state =
								    ExpectType;
								break;
							case hContentEncoding:
								(*hdh)->
								    state =
								    ExpectEnco;
								break;
							case hMimeVersion:
								(*hdh)->
								    state =
								    ExpectVersion;
								break;
							case hContentDisposition:
								(*hdh)->
								    state =
								    ExpectDisposition;
								break;
							case hContentDescription:
								EXPECT_TEXT;	// Throw away content-description; redundant junk!
								break;
							case hReceived:
								if ((*hdh)->foundRecvd)
									(*hdh)->hFound = 0;
								else
									(*hdh)->foundRecvd = True;
								Ungrok
								    (hdh);
								EXPECT_TEXT;
								break;

							case hMessageId:
								if ((*hdh)->foundMID)
									(*hdh)->hFound = 0;
								else
									(*hdh)->foundMID = True;
								Ungrok
								    (hdh);
								EXPECT_TEXT;
								break;

							case hContentLocation:
							case hContentBase:
							case hContentId:
								(*hdh)->
								    state =
								    ExpectStructuredValue;
								break;

							default:
								Ungrok
								    (hdh);
								EXPECT_TEXT;
								break;
							}
						} else {
							Ungrok(hdh);
							EXPECT_TEXT;
						}
						break;
					}
					/* fall through if not colon */

				default:
					Ungrok(hdh);
					EXPECT_TEXT;
					break;
				}
				break;
					/*********************************/
			case ExpectText:
				LDRef(hdh);
				if (funFieldsID == InterestHeadStrn) {
					switch ((*hdh)->hFound) {
					case hStatus:
						PCSTrim((*hdh)->status,
							l822s.token);
						break;
					case hSubject:
						PCSTrim((*hdh)->subj,
							l822s.token);
						break;
					case hWho:
						PCSTrim((*hdh)->who,
							l822s.token);
						if (PrefIsSet
						    (PREF_NO_SELF_RECEIPT)
						    && IsMe(l822s.token))
							(*hdh)->hasMDN =
							    False;
						AddFunField(hdh,
							    l822s.token,
							    funFieldsID);
						break;
					case hMDN:
						(*hdh)->hasMDN = True;
						break;
					case hMessageId:
						hash =
						    MIDHash(l822s.token +
							    1,
							    *l822s.token);
						(*hdh)->msgIdHash = hash;
						AddFunField(hdh,
							    l822s.token,
							    funFieldsID);
						break;
					case hDate:
						{
							uLong zone;
							uLong secs =
							    BeautifyDate
							    (l822s.token,
							     &zone);
							AddFunField(hdh,
								    l822s.
								    token,
								    funFieldsID);
							(*hdh)->gmtSecs =
							    secs;
							break;
						}

					default:
						//                                                      case hReceived:
						AddFunField(hdh,
							    l822s.token,
							    funFieldsID);
						break;
					}
				} else {
					AddFunField(hdh, l822s.token,
						    funFieldsID);
				}
				UL(hdh);
				break;

					/*********************************/
			case ExpectStructuredValue:
				LDRef(hdh);
				*GlobalTemp = 0;	// AAFetchResData won't clear this
				AAFetchResData((*hdh)->funFields,
					       funFieldsID +
					       (*hdh)->hFound, GlobalTemp);

				switch (tokenType) {
				case t822Comment:
				case LinearWhite:	/* ignore */
					break;
				case DomainLit:
					PCatC(GlobalTemp, '[');
					PSCat(GlobalTemp, l822s.token);
					PCatC(GlobalTemp, ']');
					break;
				default:
					PSCat(GlobalTemp, l822s.token);
					break;
				}
				AddFunField(hdh, GlobalTemp, funFieldsID);

				UL(hdh);
				break;

					/*********************************/
			case ExpectEnco:
				LDRef(hdh);
				switch (tokenType) {
				case LinearWhite:
					break;	/* ignore */
				default:
					PCSTrim((*hdh)->contentEnco,
						l822s.token);
					/*
					 * if we know the encoding, get rid of it
					 * that is, if it's safe to do so.
					 */
					if (refN && !NoAttachments
					    && FindMIMEDecoder((*hdh)->
							       contentEnco,
							       &junk,
							       False)) {
						AccuFSeek(&a, refN,
							  lastNewline);
					}
					(*hdh)->eightBit =
					    EqualStrRes((*hdh)->
							contentEnco,
							MIME_BINARY)
					    || EqualStrRes((*hdh)->
							   contentEnco,
							   MIME_8BIT);
					EXPECT_TEXT;
					break;
				}
				UL(hdh);
				break;

					/*********************************/
			case ExpectVersion:
				LDRef(hdh);
				switch (tokenType) {
				case LinearWhite:
					break;	/* ignore */
				default:
					PSCopy((*hdh)->mimeVersion,
					       l822s.token);
					if (!StringSame
					    (GetRString
					     (scratch, MIME_VERSION),
					     (*hdh)->mimeVersion))
						Ungrok(hdh);
					(*hdh)->isMIME = True;
					//      Add version to ems MIME info
					if (AddTLMIME
					    ((*hdh)->tlMIME,
					     TLMIME_VERSION, l822s.token,
					     nil)) {
						tokenType = ErrorToken;
						goto out;
					}
					break;
				}
				UL(hdh);
				break;

					/*********************************/
			case ExpectType:
				if (tokenType == LinearWhite)
					break;	/* skip this */
				if (tokenType == Atom
				    || tokenType == QText) {
					// hack to avoid over-deep mime structures
					if ((*hdh)->depth >
					    GetRLong(MAX_MULTI_DEPTH))
						GetRString(l822s.token,
							   MIME_TEXT);
					PSCopy((*hdh)->contentType,
					       l822s.token);
					(*hdh)->state = ExpectSlash;
					if (EqualStrRes
					    (l822s.token, MIME_TEXT)
					    && *GetRString(scratch,
							   UNSPECIFIED_CHARSET))
						if (xlateResID =
						    FindMIMECharset
						    (scratch))
							(*hdh)->
							    xlateResID =
							    defaultTableId
							    = xlateResID;
#ifdef ETL
					if (AddTLMIME
					    ((*hdh)->tlMIME, TLMIME_TYPE,
					     l822s.token, nil)) {
						tokenType = ErrorToken;
						goto out;
					}
#endif
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectSlash:
				if (tokenType == LinearWhite)
					break;	/* ignore */
				if (tokenType == Special
				    && l822s.token[1] == '/') {
					(*hdh)->state = ExpectSubType;
					break;
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectSubType:
				if (tokenType == LinearWhite)
					break;	/* skip this */
				if (tokenType == Atom
				    || tokenType == QText) {
					// hack to avoid over-deep mime structures
					if ((*hdh)->depth >
					    GetRLong(MAX_MULTI_DEPTH))
						GetRString(l822s.token,
							   MIME_PLAIN);
					PSCopy((*hdh)->contentSubType,
					       l822s.token);
					(*hdh)->state = ExpectSem;
					(*hdh)->isMIME = True;
					if (EqualStrRes
					    (l822s.token, MIME_PARTIAL))
						(*hdh)->isPartial = True;
					else
						(*hdh)->isPartial = False;
					if (EqualStrRes
					    (l822s.token, MIME_RICHTEXT))
						(*hdh)->hasRich = AnyRich =
						    True;
					if (EqualStrRes
					    (l822s.token,
					     HTMLTagsStrn + htmlTag))
						(*hdh)->hasHTML = AnyHTML =
						    True;
					if (refN)
						lastParam =
						    AccuFTell(&a, refN);
#ifdef ETL
					if (AddTLMIME
					    ((*hdh)->tlMIME,
					     TLMIME_SUBTYPE, l822s.token,
					     nil)) {
						tokenType = ErrorToken;
						goto out;
					}
#endif
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectDisposition:
				if (tokenType == LinearWhite)
					break;	/* skip this */
				if (tokenType == Atom) {
					if (EqualStrRes
					    (l822s.token, ATTACHMENT))
						(*hdh)->isAttach = True;
					(*hdh)->state = ExpectSem;
					if (AddTLMIME
					    ((*hdh)->tlMIME,
					     TLMIME_CONTENTDISP,
					     l822s.token, nil)) {
						tokenType = ErrorToken;
						goto out;
					}
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}

				break;

					/*********************************/
			case ExpectSem:
				if (tokenType == LinearWhite)
					break;
				if (tokenType == Special
				    && l822s.token[1] == ';') {
					(*hdh)->state = ExpectAttribute;
					break;
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectAttribute:
				if (tokenType == LinearWhite)
					break;	/* skip this */
				if (tokenType == Atom) {
					PSCopy((*hdh)->attributeName,
					       l822s.token);
					(*hdh)->state = ExpectEqual;
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectEqual:
				if (tokenType == LinearWhite)
					break;
				if (tokenType == Special
				    && l822s.token[1] == '=') {
					(*hdh)->state = ExpectValue;
					break;
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;

					/*********************************/
			case ExpectValue:
				if (tokenType == LinearWhite)
					break;	/* skip this */
				if (tokenType == Atom
				    || tokenType == QText) {
					err =
					    AddHeaderAttribute(hdh,
							       l822s.token,
							       funFieldsID
							       ==
							       InterestHeadStrn);
					if (err) {
						tokenType = ErrorToken;
						goto out;
					}
					(*hdh)->state = ExpectSem;
					attr =
					    FindSTRNIndex(AttributeStrn,
							  LDRef(hdh)->
							  attributeName);
					UL(hdh);
					if (attr == aCharSet) {
						Boolean foundCharset;
						short xlateResID =
						    FindMIMECharsetLo
						    (l822s.token,
						     &foundCharset);
						(*hdh)->xlateResID =
						    xlateResID;
						if ((HasUnicode()
						     && (!foundCharset
							 ||
							 PrefIsSet
							 (PREF_ALWAYS_CHARSET)))
						    && !EqualStrRes(l822s.
								    token,
								    MIME_USASCII))
						{
							(*hdh)->
							    hasCharset =
							    AnyCharset =
							    true;
						}
#ifndef SAVE_MIME
						if (!(*hdh)->hasCharset
						    && refN && xlateResID
						    && GetResource_('taBL',
								    xlateResID))
						{
							AccuFSeek(&a, refN,
								  lastParam);
						}
#endif
					} else if (attr == aMRType) {
						PSCopy((*hdh)->
						       contentSubType,
						       l822s.token);
					}
#ifdef TWO
					else if (attr == aAccessType
						 && !EqualStrRes(l822s.
								 token,
								 MAIL_SERVER))
						Ungrok(hdh);
#endif
					else if (attr == aFormat
						 && EqualStrRes(l822s.
								token,
								FORMAT_FLOWED))
						(*hdh)->hasFlow = AnyFlow =
						    true;
					else if (attr == aDelSP
						 && (l822s.token[1] == 'y'
						     || l822s.token[1] ==
						     'Y'
						     || l822s.token[1] ==
						     't'
						     || l822s.token[1] ==
						     'T'))
						AnyDelSP = true;
					if (refN)
						lastParam =
						    AccuFTell(&a, refN);
				} else {
					Ungrok(hdh);
					EXPECT_TEXT;
				}
				break;
			}
		}
	}

      out:
	// write out last headers
	if (refN && (err = AccuWrite(&a, refN))) {
		WarnUser(WRITE_MBOX, err);
		tokenType = ErrorToken;
	}
	// put this back
	(*hdh)->fullHeaders = fullHeaders;

	/*
	 * did we finish cleanly?
	 */
	if (!OkStateAtWhichToEnd((*hdh)->state)) {
		if ((*hdh)->state == ExpectSlash)
			TextPlain(hdh);	/* hack for NASA's semi-MIME gateway */
		Ungrok(hdh);
	}

	/*
	 * where we ended up
	 */
	if (refN && (err = GetFPos(refN, &LDRef(hdh)->diskEnd))) {
		WarnUser(WRITE_MBOX, err);
		tokenType = ErrorToken;
	}
	UL(hdh);
	AccuZap(a);

	return (tokenType);
}

/************************************************************************
 * TextPlain - set a headerdesc to text/plain
 ************************************************************************/
void TextPlain(HeaderDHandle hdh)
{
	Str63 scratch;

	GetRString(scratch, MIME_TEXT);
	PSCopy((*hdh)->contentType, scratch);
	GetRString(scratch, MIME_PLAIN);
	PSCopy((*hdh)->contentSubType, scratch);
}

/************************************************************************
 * NewHeaderDesc - create a header descriptor
 ************************************************************************/
HeaderDHandle NewHeaderDesc(HeaderDHandle parentHDH)
{
	HeaderDHandle hdh = NewZHTB(HeaderDesc);
	void *aah;
	Accumulator accu;
#ifdef ETL
	emsMIMEHandle tl;
#endif

	if (hdh) {
		(*hdh)->grokked = True;
		aah =
		    AANew(GetRLong(MIME_ATTR_MAX), GetRLong(MIME_VAL_MAX));
		(*hdh)->contentAttributes = aah;
		aah = AANew(GetRLong(MIME_ATTR_MAX), 256);
		(*hdh)->funFields = aah;
		AccuInit(&accu);
		(*hdh)->fullHeaders = accu;
		if (parentHDH) {
			PCopy((*hdh)->summaryInfo,
			      (*parentHDH)->summaryInfo);
			(*hdh)->gmtSecs = (*parentHDH)->gmtSecs;
			(*hdh)->depth = (*parentHDH)->depth + 1;
		}
#ifdef ETL
		NewTLMIME(&tl);
		(*hdh)->tlMIME = tl;
#endif
		if (!(*hdh)->contentAttributes || !(*hdh)->fullHeaders.data
		    || !(*hdh)->funFields
#ifdef ETL
		    || !(*hdh)->tlMIME
#endif
		    )
			ZapHeaderDesc(hdh);
	}
	return (hdh);
}

/************************************************************************
 * DisposeHeaderDesc - destroy a header descriptor
 ************************************************************************/
void DisposeHeaderDesc(HeaderDHandle hdh)
{
	if (hdh) {
		ZapHandle((*hdh)->contentAttributes);
		ZapHandle((*hdh)->funFields);
		ZapHandle((*hdh)->fullHeaders.data);
#ifdef ETL
		ZapTLMIME((*hdh)->tlMIME);
#endif
		ZapHandle(hdh);
	}
}

/************************************************************************
 * WriteHeaderToken - write a header token to disk
 ************************************************************************/
Token822Enum WriteHeaderToken(Token822Enum tokenType, L822SPtr l822p,
			      AccuPtr a)
{
	short err;

	switch (tokenType) {
		/*
		 * either of these get written as two returns
		 */
	case EndOfHeader:
	case EndOfMessage:
		err = AccuAddPtr(a, "\015\015", 2);
		break;

		/*
		 * these don't get written
		 */
	case ErrorToken:
		err = 0;
		break;

		/*
		 * sigh.  since the token has the quotes stripped, writing it
		 * takes three writes.  Gotta write a good disk io package...
		 */
	case QText:
		if (!(err = AccuAddChar(a, '"')))
			if (!(err = AccuAddStr(a, l822p->token)))
				err = AccuAddChar(a, '"');
		break;

		/*
		 * write out the token
		 */
	default:
		err = AccuAddStr(a, l822p->token);
		break;
	}

	if (err) {
		WarnUser(WRITE_MBOX, err);
		return (ErrorToken);
	}

	return (tokenType);
}

/************************************************************************
 * AddHeaderAttribute - add an attribute to a header descriptor
 ************************************************************************/
OSErr AddHeaderAttribute(HeaderDHandle hdh, PStr value, Boolean etl)
{
	short err;
	AttributeEnum attributeNum;
	short oldLen;

	oldLen = *value;
	if (*value >= (*(*hdh)->contentAttributes)->dataSize)
		*value = (*(*hdh)->contentAttributes)->dataSize - 1;
	err =
	    AAAddItem((*hdh)->contentAttributes, True,
		      (void *) LDRef(hdh)->attributeName, (void *) value);
	*value = oldLen;

	if (err) {
		UL(hdh);
		return (WarnUser(MEM_ERR, MemError()));
	}

	switch (attributeNum =
		FindSTRNIndex(AttributeStrn,
			      (void *) (*hdh)->attributeName)) {
	case aUnknown:
		if (!BeginsWith((*hdh)->attributeName, "\px-"))
			Ungrok(hdh);
		break;
	}

#ifdef ETL
	if (etl)
		err =
		    AddTLMIME((*hdh)->tlMIME,
			      (*hdh)->hFound ==
			      hContentDisposition ?
			      TLMIME_CONTENTDISP_PARAM : TLMIME_PARAM,
			      (*hdh)->attributeName, value);
#endif

	UL(hdh);
	return (err);
}

/************************************************************************
 * AddFunField - add a field to a header descriptor
 ************************************************************************/
OSErr AddFunField(HeaderDHandle hdh, PStr value, short funFieldsID)
{
	short err;
	Str31 fName;

	if ((*hdh)->hFound) {
		GetRString(fName, funFieldsID + (*hdh)->hFound);
		err = AAAddItem((*hdh)->funFields, True, fName, value);
		if (err)
			return (WarnUser(MEM_ERR, MemError()));
	}

	return (noErr);
}

/************************************************************************
 * ViewTable - set the viewing table based on the MIME character set
 ************************************************************************/
short ViewTable(HeaderDHandle hdh)
{
	/*
	 * if we were MIME-ing, we don't need a view table.
	 */
	if ((*hdh)->isMIME)
		return (NO_TABLE);

	/*
	 * for no MIME, use the default
	 */
	return (DEFAULT_TABLE);
}

OSErr HeaderRecvLine(TransStream stream, UPtr buffer, long *size);
OSErr HeaderRecvLine(TransStream stream, UPtr buffer, long *size)
{
	long endOffset = 0L;
	long bufLen;

	if (!buffer)
		return paramErr;
	bufLen = *size;
	if (parseHeaderSize) {
		for (endOffset = parseHeaderOffset;
		     endOffset - parseHeaderOffset < parseHeaderSize
		     && endOffset < parseHeaderOffset + bufLen;
		     ++endOffset) {
			if ((*parseHeaderHandle)[endOffset] == '\015') {
				++endOffset;
				break;
			}
		}
		BMD(*parseHeaderHandle + parseHeaderOffset, buffer,
		    endOffset - parseHeaderOffset);
		*size = (endOffset - parseHeaderOffset);
		bufLen -= (endOffset - parseHeaderOffset);
		parseHeaderSize -= (endOffset - parseHeaderOffset);
		parseHeaderOffset = endOffset;
	}
	if (!parseHeaderSize && bufLen > 1) {
		buffer[endOffset++] = '\015';
		buffer[endOffset] = '\015';
		*size += 2;
	}
	return noErr;
}

OSErr ParseAHeader(StringHandle h, HeaderDHandle * hdhp)
{
	return ParseAHeaderLo(h, hdhp, InterestHeadStrn,
			      InterestHeadLimit);
}

OSErr ParseAHeaderLo(StringHandle h, HeaderDHandle * hdhp,
		     short funFieldsID, short funFieldsLimit)
{
	Token822Enum tokenType;
	TransVector HeaderTrans =
	    { nil, nil, nil, nil, nil, nil, nil, nil, nil, HeaderRecvLine,
	nil };
	HeaderDHandle hdh = NewHeaderDesc(nil);
	TransVector saveCurTrans = CurTrans;

	if (hdh == nil)
		return MemError();

	parseHeaderHandle = h;
	parseHeaderOffset = 0L;
	parseHeaderSize = GetHandleSize(h);

	/* Set the trans routine */
	CurTrans = HeaderTrans;

	tokenType =
	    ReadHeaderLo(nil, hdh, 0, 0, false, funFieldsID,
			 funFieldsLimit);

	/* Reset the trans routine */
	CurTrans = saveCurTrans;

	if (tokenType != ErrorToken) {
		*hdhp = hdh;
		return noErr;
	} else {
		OSErr err = MemError();
		DisposeHeaderDesc(hdh);
		return err ? err : paramErr;
	}
}

/************************************************************************
 * HeaderDescInAddrBook - do these headers represent someone in the address book?
 ************************************************************************/
Boolean HeaderDescInAddrBook(HeaderDHandle hdh)
{
	Str255 addr;
	if (!AAFetchResData
	    ((*hdh)->funFields, InterestHeadStrn + hWho, addr))
		return AppearsInAliasFile(addr, nil);
	else
		return false;
}
