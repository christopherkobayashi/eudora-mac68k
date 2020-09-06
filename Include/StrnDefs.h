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

#ifndef STRNDEFS_H
#define STRNDEFS_H

#define HeaderStrn 1600
typedef enum {
	TO_HEAD=1,
	FROM_HEAD, /*2*/ 
	SUBJ_HEAD, /*3*/ 
	CC_HEAD, /*4*/ 
	BCC_HEAD, /*5*/ 
	ATTACH_HEAD, /*6*/ 
	BODY_HEAD, /*7*/ 
	REPLYTO_HEAD, /*8*/ 
	PRIORITY_HEAD, /*9*/ 
	MSGID_HEAD, /*10*/ 
	IN_REPLY_TO_HEAD, /*11*/ 
	REFERENCES_HEAD, /*12*/ 
	TRANSLATOR_HEAD, /*13*/ 
	DATE_HEAD, /*14*/ 
	PERSONA_HEAD, /*15*/ 
	SIG_HEAD, /*16*/ 
	HeaderLimit,
	HeaderMakeLong=-1000000
} HeaderEnum;
#define BadHeadStrn 3200
typedef enum {
	BadHeadLimit,
	BadHeadMakeLong=-1000000
} BadHeadEnum;
#define ReplyStrn 5400
typedef enum {
	ReplyLimit,
	ReplyMakeLong=-1000000
} ReplyEnum;
#define AttributeStrn 24800
typedef enum {
#define aUnknown 0
	aName=1,
	aCharSet, /*2*/ 
	aBoundary, /*3*/ 
	aMacType, /*4*/ 
	aMacCreator, /*5*/ 
	aFilename, /*6*/ 
	aMRType, /*7*/ 
	aAccessType, /*8*/ 
	aServer, /*9*/ 
	aSubject, /*10*/ 
	aMode, /*11*/ 
	aDirectory, /*12*/ 
	aSite, /*13*/ 
	aFormat, /*14*/ 
	aModDate, /*15*/ 
	aCreateDate, /*16*/ 
	aStart, /*17*/ 
	aType, /*18*/ 
	aTypes, /*19*/ 
	aRegFile, /*20*/ 
	aDelSP, /*21*/ 
	aSize, /*22*/ 
	AttributeLimit,
	AttributeMakeLong=-1000000
} AttributeEnum;
#define TOCHeaderStrn 1300
typedef enum {
	tchDate=1,
	tchFrom, /*2*/ 
	tchStatus, /*3*/ 
	tchTo, /*4*/ 
	tchXPrior, /*5*/ 
	tchBcc, /*6*/ 
	tchSubject, /*7*/ 
	tchImportance, /*8*/ 
	tchPrecedence, /*9*/ 
	tchMessageId, /*10*/ 
	TOCHeaderLimit,
	TOCHeaderMakeLong=-1000000
} TOCHeaderEnum;
#define HeaderLabelStrn 24200
typedef enum {
	HeaderLabelLimit,
	HeaderLabelMakeLong=-1000000
} HeaderLabelEnum;
#define InterestHeadStrn 24600
typedef enum {
	hContentType=1,
	hContentEncoding, /*2*/ 
	hContentDisposition, /*3*/ 
	hContentBase, /*4*/ 
	hContentLocation, /*5*/ 
	hContentId, /*6*/ 
	hContentDescription, /*7*/ 
	hMimeVersion, /*8*/ 
	hStatus, /*9*/ 
	hSubject, /*10*/ 
	hWho, /*11*/ 
	hReceived, /*12*/ 
	hMessageId, /*13*/ 
	hDate, /*14*/ 
	hMDN, /*15*/ 
	hSpamFirst, /*16*/ 
	hSpamPostini, /*17*/ 
	hSpamPerlMX, /*18*/ 
	hSpamAssassinFlag, /*19*/ 
	hSpamAssassinScore, /*20*/ 
	hSpamBrownScore, /*21*/ 
	hSpamRazor, /*22*/ 
	hSpamPureMessaging, /*23*/ 
	hSpamMessageFire, /*24*/ 
	hSpamIrvine, /*25*/ 
	hSpamLevel, /*26*/ 
	hSpamLimit, /*27*/ 
	InterestHeadLimit,
	InterestHeadMakeLong=-1000000
} InterestHeadEnum;
#define BoxLinesStrn 25700
typedef enum {
	blStat=1,
	blPrior, /*2*/ 
	blAttach, /*3*/ 
	blLabel, /*4*/ 
	blFrom, /*5*/ 
	blDate, /*6*/ 
	blSize, /*7*/ 
	blServer, /*8*/ 
	blMailbox, /*9*/ 
	blSubject, /*10*/ 
	blAnal, /*11*/ 
	blJunk, /*12*/ 
	BoxLinesLimit,
	BoxLinesMakeLong=-1000000
} BoxLinesEnum;
#define NoClearPassStrn 26000
typedef enum {
	NoClearPassLimit,
	NoClearPassMakeLong=-1000000
} NoClearPassEnum;
#define ColumnHeadStrn 26100
typedef enum {
	ColumnHeadLimit,
	ColumnHeadMakeLong=-1000000
} ColumnHeadEnum;
#define ProtocolStrn 26300
typedef enum {
	proFinger=1,
	proPh, /*2*/ 
	proMail, /*3*/ 
	proPh2, /*4*/ 
	proFile, /*5*/ 
	proCID, /*6*/ 
	proLDAP, /*7*/ 
	proCompFile, /*8*/ 
	proSetting, /*9*/ 
	proPictRes, /*10*/ 
	proPictHandle, /*11*/ 
	proXEudoraJump, /*12*/ 
	ProtocolLimit,
	ProtocolMakeLong=-1000000
} ProtocolEnum;
#define PGPStrn 26500
typedef enum {
#define pgpNone 0
	pgpKey=1,
	pgpSigned, /*2*/ 
	pgpSignature, /*3*/ 
	pgpMessage, /*4*/ 
	pgpBegin, /*5*/ 
	pgpEnd, /*6*/ 
	pgpUnknown, /*7*/ 
	PGPLimit,
	PGPMakeLong=-1000000
} PGPEnum;
#define PhStrn 26700
typedef enum {
	phfName=1,
	phfMaildomain, /*2*/ 
	phfMailfield, /*3*/ 
	phfMailbox, /*4*/ 
	phfEmail, /*5*/ 
	phfAlias, /*6*/ 
	phfQuery, /*7*/ 
	phfSiteInfo, /*8*/ 
	phfServerName, /*9*/ 
	phfType, /*10*/ 
	phfServer, /*11*/ 
	phfServerList, /*12*/ 
	phfSite, /*13*/ 
	PhLimit,
	PhMakeLong=-1000000
} PhEnum;
#define ToolbarProStrn 26900
typedef enum {
	ToolbarProLimit,
	ToolbarProMakeLong=-1000000
} ToolbarProEnum;
#define ToolbarSizesStrn 27100
typedef enum {
	ToolbarSizesLimit,
	ToolbarSizesMakeLong=-1000000
} ToolbarSizesEnum;
#define AddrHeadsStrn 27300
typedef enum {
	AddrHeadsLimit,
	AddrHeadsMakeLong=-1000000
} AddrHeadsEnum;
#define TBButtonStrn 27500
typedef enum {
	tbkMenu=1,
	tbkKey, /*2*/ 
	tbkFile, /*3*/ 
	tbkAd, /*4*/ 
	tbkNick, /*5*/ 
	TBButtonLimit,
	TBButtonMakeLong=-1000000
} TBButtonEnum;
#define PrivLabelsStrn 27700
typedef enum {
	PrivLabelsLimit,
	PrivLabelsMakeLong=-1000000
} PrivLabelsEnum;
#define PrivColorsStrn 27900
typedef enum {
	PrivColorsLimit,
	PrivColorsMakeLong=-1000000
} PrivColorsEnum;
#define ImportanceStrn 28300
typedef enum {
	ImportanceLimit,
	ImportanceMakeLong=-1000000
} ImportanceEnum;
#define ImportanceOutStrn 28500
typedef enum {
	ImportanceOutLimit,
	ImportanceOutMakeLong=-1000000
} ImportanceOutEnum;
#define FiltVerbPrintStrn 28700
typedef enum {
	FiltVerbPrintLimit,
	FiltVerbPrintMakeLong=-1000000
} FiltVerbPrintEnum;
#define FiltConjPrintStrn 28900
typedef enum {
	FiltConjPrintLimit,
	FiltConjPrintMakeLong=-1000000
} FiltConjPrintEnum;
#define FiltKindPrintStrn 29100
typedef enum {
	fkpIn=1,
	fkpOut, /*2*/ 
	fkpMan, /*3*/ 
	fkpNone, /*4*/ 
	FiltKindPrintLimit,
	FiltKindPrintMakeLong=-1000000
} FiltKindPrintEnum;
#define UrlColorStrn 29300
typedef enum {
	UrlColorLimit,
	UrlColorMakeLong=-1000000
} UrlColorEnum;
#define EnrichedStrn 29500
typedef enum {
	enBold=1,
	enItalic, /*2*/ 
	enUnderline, /*3*/ 
	enNoFill, /*4*/ 
	enParam, /*5*/ 
	enXRich, /*6*/ 
	enFixed, /*7*/ 
	enSmaller, /*8*/ 
	enBigger, /*9*/ 
	enCenter, /*10*/ 
	enLeft, /*11*/ 
	enRight, /*12*/ 
	enFont, /*13*/ 
	enColor, /*14*/ 
	enPara, /*15*/ 
	enExcerpt, /*16*/ 
	enPLeft, /*17*/ 
	enPRight, /*18*/ 
	enPIn, /*19*/ 
	enPOut, /*20*/ 
	enXHTML, /*21*/ 
	enHTML, /*22*/ 
	enXFlowed, /*23*/ 
	enXCharset, /*24*/ 
	EnrichedLimit,
	EnrichedMakeLong=-1000000
} EnrichedEnum;
#define EsmtpStrn 29700
typedef enum {
	esmtp8BMIME=1,
	esmtpSize, /*2*/ 
	esmtpAmd5, /*3*/ 
	esmtpPipeline, /*4*/ 
	esmtpAuth, /*5*/ 
	esmtpdead1, /*6*/ 
	esmtpdead2, /*7*/ 
	esmtpdead3, /*8*/ 
	esmtpStartTLS, /*9*/ 
	EsmtpLimit,
	EsmtpMakeLong=-1000000
} EsmtpEnum;
#define MarginStrn 29900
typedef enum {
	MarginLimit,
	MarginMakeLong=-1000000
} MarginEnum;
#define Unused30500Strn 30500
typedef enum {
	Unused30500Limit,
	Unused30500MakeLong=-1000000
} Unused30500Enum;
#define PhNickPhStrn 30600
typedef enum {
	PhNickPhLimit,
	PhNickPhMakeLong=-1000000
} PhNickPhEnum;
#define PhNickNickStrn 30700
typedef enum {
	PhNickNickLimit,
	PhNickNickMakeLong=-1000000
} PhNickNickEnum;
#define DaemonStrn 30900
typedef enum {
	DaemonLimit,
	DaemonMakeLong=-1000000
} DaemonEnum;
#define PluralStrn 31100
typedef enum {
	PluralLimit,
	PluralMakeLong=-1000000
} PluralEnum;
#define ToolbarLightStrn 31300
typedef enum {
	ToolbarLightLimit,
	ToolbarLightMakeLong=-1000000
} ToolbarLightEnum;
#define UndoStrn 1800
typedef enum {
	undoTyping=1,
	redoTyping, /*2*/ 
	undoCut, /*3*/ 
	redoCut, /*4*/ 
	undoPaste, /*5*/ 
	redoPaste, /*6*/ 
	undoClear, /*7*/ 
	redoClear, /*8*/ 
	undoStyle, /*9*/ 
	redoStyle, /*10*/ 
	undoDrag, /*11*/ 
	redoDrag, /*12*/ 
	undoPara, /*13*/ 
	redoPara, /*14*/ 
	undoCutPlain, /*15*/ 
	redoCutPlain, /*16*/ 
	UndoStylePara, /*17*/ 
	RedoStylePara, /*18*/ 
	undoLink, /*19*/ 
	redoLink, /*20*/ 
	undoLimit, /*21*/ 
	redoLimit, /*22*/ 
	UndoLimit,
	UndoMakeLong=-1000000
} UndoEnum;
#define RegMessageTagsStrn 32000
typedef enum {
	regTAG_SEPARATOR=1,
#if defined(LIGHT)
	regADDRESS, /*2*/ 
#elif defined(DEMO)
	regADDRESS, /*3*/ 
#else
	regADDRESS, /*4*/ 
#endif
#if defined(LIGHT)
	regSUBJECT_LINE, /*5*/ 
#elif defined(DEMO)
	regSUBJECT_LINE, /*6*/ 
#else
	regSUBJECT_LINE, /*7*/ 
#endif
	regDO_NOT_MODIFY, /*8*/ 
	regFIRST_NAME, /*9*/ 
	regLAST_NAME, /*10*/ 
	regCOMPANY_NAME, /*11*/ 
	regTITLE, /*12*/ 
	regADDRESS1, /*13*/ 
	regADDRESS2, /*14*/ 
	regCITY, /*15*/ 
	regSTATE, /*16*/ 
	regZIP, /*17*/ 
	regCOUNTRY, /*18*/ 
	regPHONE, /*19*/ 
	regFAX, /*20*/ 
	regEMAIL_ADDRESS, /*21*/ 
#if defined(LIGHT)||defined(DEMO)
	regREGISTRATION_CODE, /*22*/ 
#else
	regREGISTRATION_CODE, /*23*/ 
#endif
	regJUNK_MAIL, /*24*/ 
	regSYSTEM_INFORMATION, /*25*/ 
	regMACHINE_TYPE, /*26*/ 
	regUNIDENTIFIED_MACHINE, /*27*/ 
	regSYSTEM_VERSION, /*28*/ 
	regPROCESSOR, /*29*/ 
	regPHYSICAL_RAM_INSTALLED, /*30*/ 
	regRAM_INSTALLED, /*31*/ 
#if defined(LIGHT) 
	regEUDORA_VERSION, /*32*/ 
#elif defined(DEMO)
	regEUDORA_VERSION, /*33*/ 
#else
	regEUDORA_VERSION, /*34*/ 
#endif
	regTCP_IP_VERSION, /*35*/ 
	regCURRENT_MEM, /*36*/ 
	regGETINFO_MEM, /*37*/ 
	regSUGGEST_MEM, /*38*/ 
	regEXTEN_AND_CPS, /*39*/ 
	regOT_TITLE, /*40*/ 
	regMACTCP_TITLE, /*41*/ 
	regSYSTEM_FOLDER, /*42*/ 
	regEXTEN_FOLDER, /*43*/ 
	regCPS_FOLDER, /*44*/ 
	regCHANGED_SETTINGS, /*45*/ 
	regEND_INFO, /*46*/ 
#if defined(LIGHT)||defined(DEMO)
	regPART_NUMBER, /*47*/ 
#else
	regPART_NUMBER, /*48*/ 
#endif
	regUSER, /*49*/ 
	regDECISION, /*50*/ 
	regPERSONAL, /*51*/ 
	regORGANIZATION, /*52*/ 
	regMAIN_MONITOR, /*53*/ 
	RegMessageTagsLimit,
	RegMessageTagsMakeLong=-1000000
} RegMessageTagsEnum;
#define KeyNameStrn 32200
typedef enum {
	kn00=1,
	kn01, /*2*/ 
	kn02, /*3*/ 
	kn03, /*4*/ 
	kn04, /*5*/ 
	kn05, /*6*/ 
	kn06, /*7*/ 
	kn07, /*8*/ 
	kn08, /*9*/ 
	kn09, /*10*/ 
	kn0a, /*11*/ 
	kn0b, /*12*/ 
	kn0c, /*13*/ 
	kn0d, /*14*/ 
	kn0e, /*15*/ 
	kn0f, /*16*/ 
	kn10, /*17*/ 
	kn11, /*18*/ 
	kn12, /*19*/ 
	kn13, /*20*/ 
	kn14, /*21*/ 
	kn15, /*22*/ 
	kn16, /*23*/ 
	kn17, /*24*/ 
	kn18, /*25*/ 
	kn19, /*26*/ 
	kn1a, /*27*/ 
	kn1b, /*28*/ 
	kn1c, /*29*/ 
	kn1d, /*30*/ 
	kn1e, /*31*/ 
	kn1f, /*32*/ 
	kn20, /*33*/ 
	KeyNameLimit,
	KeyNameMakeLong=-1000000
} KeyNameEnum;
#define OnlyHostsStrn 32300
typedef enum {
	OnlyHostsLimit,
	OnlyHostsMakeLong=-1000000
} OnlyHostsEnum;
#define HTMLLiteralsStrn 15000
typedef enum {
	htmlLiteralCtlA=1,
	htmlLiteralCtlB, /*2*/ 
	htmlLiteralCtlC, /*3*/ 
	htmlLiteralCtlD, /*4*/ 
	htmlLiteralCtlE, /*5*/ 
	htmlLiteralCtlF, /*6*/ 
	htmlLiteralCtlG, /*7*/ 
	htmlLiteralCtlH, /*8*/ 
	htmlLiteralCtlI, /*9*/ 
	htmlLiteralCtlJ, /*10*/ 
	htmlLiteralCtlK, /*11*/ 
	htmlLiteralCtlL, /*12*/ 
	htmlLiteralCtlM, /*13*/ 
	htmlLiteralCtlN, /*14*/ 
	htmlLiteralCtlO, /*15*/ 
	htmlLiteralCtlP, /*16*/ 
	htmlLiteralCtlQ, /*17*/ 
	htmlLiteralCtlR, /*18*/ 
	htmlLiteralCtlS, /*19*/ 
	htmlLiteralCtlT, /*20*/ 
	htmlLiteralCtlU, /*21*/ 
	htmlLiteralCtlV, /*22*/ 
	htmlLiteralCtlW, /*23*/ 
	htmlLiteralCtlX, /*24*/ 
	htmlLiteralCtlY, /*25*/ 
	htmlLiteralCtlZ, /*26*/ 
	htmlLiteralEscape, /*27*/ 
	htmlLiteralFileSep, /*28*/ 
	htmlLiteralGroupSep, /*29*/ 
	htmlLiteralRecordSep, /*30*/ 
	htmlLiteralUnitSep, /*31*/ 
	htmlLiteralSpace, /*32*/ 
	htmlLiteralExclamation, /*33*/ 
	htmlLiteralQuote, /*34*/ 
	htmlLiteralNumber, /*35*/ 
	htmlLiteralDollar, /*36*/ 
	htmlLiteralPercent, /*37*/ 
	htmlLiteralAmpersand, /*38*/ 
	htmlLiteralApostrophe, /*39*/ 
	htmlLiteralLParen, /*40*/ 
	htmlLiteralRParen, /*41*/ 
	htmlLiteralAsterisk, /*42*/ 
	htmlLiteralPlus, /*43*/ 
	htmlLiteralComma, /*44*/ 
	htmlLiteralHyphen, /*45*/ 
	htmlLiteralPeriod, /*46*/ 
	htmlLiteralSlash, /*47*/ 
	htmlLiteral0, /*48*/ 
	htmlLiteral1, /*49*/ 
	htmlLiteral2, /*50*/ 
	htmlLiteral3, /*51*/ 
	htmlLiteral4, /*52*/ 
	htmlLiteral5, /*53*/ 
	htmlLiteral6, /*54*/ 
	htmlLiteral7, /*55*/ 
	htmlLiteral8, /*56*/ 
	htmlLiteral9, /*57*/ 
	htmlLiteralColon, /*58*/ 
	htmlLiteralSemicolon, /*59*/ 
	htmlLiteralLessThan, /*60*/ 
	htmlLiteralEqual, /*61*/ 
	htmlLiteralGreaterThan, /*62*/ 
	htmlLiteralQuestion, /*63*/ 
	htmlLiteralAt, /*64*/ 
	htmlLiteralA, /*65*/ 
	htmlLiteralB, /*66*/ 
	htmlLiteralC, /*67*/ 
	htmlLiteralD, /*68*/ 
	htmlLiteralE, /*69*/ 
	htmlLiteralF, /*70*/ 
	htmlLiteralG, /*71*/ 
	htmlLiteralH, /*72*/ 
	htmlLiteralI, /*73*/ 
	htmlLiteralJ, /*74*/ 
	htmlLiteralK, /*75*/ 
	htmlLiteralL, /*76*/ 
	htmlLiteralM, /*77*/ 
	htmlLiteralN, /*78*/ 
	htmlLiteralO, /*79*/ 
	htmlLiteralP, /*80*/ 
	htmlLiteralQ, /*81*/ 
	htmlLiteralR, /*82*/ 
	htmlLiteralS, /*83*/ 
	htmlLiteralT, /*84*/ 
	htmlLiteralU, /*85*/ 
	htmlLiteralV, /*86*/ 
	htmlLiteralW, /*87*/ 
	htmlLiteralX, /*88*/ 
	htmlLiteralY, /*89*/ 
	htmlLiteralZ, /*90*/ 
	htmlLiteralLSquare, /*91*/ 
	htmlLiteralBackslash, /*92*/ 
	htmlLiteralRSquare, /*93*/ 
	htmlLiteralCircumflex, /*94*/ 
	htmlLiteralUnderscore, /*95*/ 
	htmlLiteralGraveAccent, /*96*/ 
	htmlLiterala, /*97*/ 
	htmlLiteralb, /*98*/ 
	htmlLiteralc, /*99*/ 
	htmlLiterald, /*100*/ 
	htmlLiterale, /*101*/ 
	htmlLiteralf, /*102*/ 
	htmlLiteralg, /*103*/ 
	htmlLiteralh, /*104*/ 
	htmlLiterali, /*105*/ 
	htmlLiteralj, /*106*/ 
	htmlLiteralk, /*107*/ 
	htmlLiterall, /*108*/ 
	htmlLiteralm, /*109*/ 
	htmlLiteraln, /*110*/ 
	htmlLiteralo, /*111*/ 
	htmlLiteralp, /*112*/ 
	htmlLiteralq, /*113*/ 
	htmlLiteralr, /*114*/ 
	htmlLiterals, /*115*/ 
	htmlLiteralt, /*116*/ 
	htmlLiteralu, /*117*/ 
	htmlLiteralv, /*118*/ 
	htmlLiteralw, /*119*/ 
	htmlLiteralx, /*120*/ 
	htmlLiteraly, /*121*/ 
	htmlLiteralz, /*122*/ 
	htmlLiteralLCurly, /*123*/ 
	htmlLiteralVerticalBar, /*124*/ 
	htmlLiteralRCurley, /*125*/ 
	htmlLiteralTilde, /*126*/ 
	htmlLiteralDelete, /*127*/ 
	htmlLiteralR128, /*128*/ 
	htmlLiteralR129, /*129*/ 
	htmlLiteralR130, /*130*/ 
	htmlLiteralR131, /*131*/ 
	htmlLiteralR132, /*132*/ 
	htmlLiteralR133, /*133*/ 
	htmlLiteralR134, /*134*/ 
	htmlLiteralR135, /*135*/ 
	htmlLiteralR136, /*136*/ 
	htmlLiteralR137, /*137*/ 
	htmlLiteralR138, /*138*/ 
	htmlLiteralR139, /*139*/ 
	htmlLiteralR140, /*140*/ 
	htmlLiteralR141, /*141*/ 
	htmlLiteralR142, /*142*/ 
	htmlLiteralR143, /*143*/ 
	htmlLiteralR144, /*144*/ 
	htmlLiteralR145, /*145*/ 
	htmlLiteralR146, /*146*/ 
	htmlLiteralR147, /*147*/ 
	htmlLiteralR148, /*148*/ 
	htmlLiteralR149, /*149*/ 
	htmlLiteralR150, /*150*/ 
	htmlLiteralR151, /*151*/ 
	htmlLiteralR152, /*152*/ 
	htmlLiteralR153, /*153*/ 
	htmlLiteralR154, /*154*/ 
	htmlLiteralR155, /*155*/ 
	htmlLiteralR156, /*156*/ 
	htmlLiteralR157, /*157*/ 
	htmlLiteralR158, /*158*/ 
	htmlLiteralR159, /*159*/ 
	htmlLiteralNBSp, /*160*/ 
	htmlLiteralInvExclamation, /*161*/ 
	htmlLiteralCent, /*162*/ 
	htmlLiteralPound, /*163*/ 
	htmlLiteralCurrency, /*164*/ 
	htmlLiteralYen, /*165*/ 
	htmlLiteralBrokenBar, /*166*/ 
	htmlLiteralSection, /*167*/ 
	htmlLiteralUmlaut, /*168*/ 
	htmlLiteralCopyright, /*169*/ 
	htmlLiteralFOrdinal, /*170*/ 
	htmlLiteralAngleQuote, /*171*/ 
	htmlLiteralNot, /*172*/ 
	htmlLiteralSoftHyphen, /*173*/ 
	htmlLiteralRegistered, /*174*/ 
	htmlLiteralMacron, /*175*/ 
	htmlLiteralDegree, /*176*/ 
	htmlLiteralPlusMinus, /*177*/ 
	htmlLiteralSuper2, /*178*/ 
	htmlLiteralSuper3, /*179*/ 
	htmlLiteralAccuteAccent, /*180*/ 
	htmlLiteralMicro, /*181*/ 
	htmlLiteralParagraph, /*182*/ 
	htmlLiteralMiddleDot, /*183*/ 
	htmlLiteralCedilla, /*184*/ 
	htmlLiteralSuper1, /*185*/ 
	htmlLiteralMOrdinal, /*186*/ 
	htmlLiteralRAngleQuote, /*187*/ 
	htmlLiteralOneQuater, /*188*/ 
	htmlLiteralOneHalf, /*189*/ 
	htmlLiteralThreeQuaters, /*190*/ 
	htmlLiteralInvQuestion, /*191*/ 
	htmlLiteralAGrave, /*192*/ 
	htmlLiteralAAccute, /*193*/ 
	htmlLiteralACircumflex, /*194*/ 
	htmlLiteralATilde, /*195*/ 
	htmlLiteralAUmlaut, /*196*/ 
	htmlLiteralARing, /*197*/ 
	htmlLiteralAELigature, /*198*/ 
	htmlLiteralCCedilla, /*199*/ 
	htmlLiteralEGrave, /*200*/ 
	htmlLiteralEAccute, /*201*/ 
	htmlLiteralECircumflex, /*202*/ 
	htmlLiteralEUmlaut, /*203*/ 
	htmlLiteralIGrave, /*204*/ 
	htmlLiteralIAccute, /*205*/ 
	htmlLiteralICircumflex, /*206*/ 
	htmlLiteralIUmlaut, /*207*/ 
	htmlLiteralEth, /*208*/ 
	htmlLiteralNTilde, /*209*/ 
	htmlLiteralOGrave, /*210*/ 
	htmlLiteralOAccute, /*211*/ 
	htmlLiteralOCircumflex, /*212*/ 
	htmlLiteralOTilde, /*213*/ 
	htmlLiteralOUmlaut, /*214*/ 
	htmlLiteralTimes, /*215*/ 
	htmlLiteralOSlash, /*216*/ 
	htmlLiteralUGrave, /*217*/ 
	htmlLiteralUAccute, /*218*/ 
	htmlLiteralUCircumflex, /*219*/ 
	htmlLiteralUUmlaut, /*220*/ 
	htmlLiteralYAcute, /*221*/ 
	htmlLiteralThorn, /*222*/ 
	htmlLiteralss, /*223*/ 
	htmlLiteralaGrave, /*224*/ 
	htmlLiteralaAccute, /*225*/ 
	htmlLiteralaCircumflex, /*226*/ 
	htmlLiteralaTilde, /*227*/ 
	htmlLiteralaUmlaut, /*228*/ 
	htmlLiteralaRing, /*229*/ 
	htmlLiteralaeLigature, /*230*/ 
	htmlLiteralcCedilla, /*231*/ 
	htmlLiteraleGrave, /*232*/ 
	htmlLiteraleAccute, /*233*/ 
	htmlLiteraleCircumflex, /*234*/ 
	htmlLiteraleUmlaut, /*235*/ 
	htmlLiteraliGrave, /*236*/ 
	htmlLiteraliAccute, /*237*/ 
	htmlLiteraliCircumflex, /*238*/ 
	htmlLiteraliUmlaut, /*239*/ 
	htmlLiteraleth, /*240*/ 
	htmlLiteralnTilde, /*241*/ 
	htmlLiteraloGrave, /*242*/ 
	htmlLiteraloAccute, /*243*/ 
	htmlLiteraloCircumflex, /*244*/ 
	htmlLiteraloTilde, /*245*/ 
	htmlLiteraloUmlaut, /*246*/ 
	htmlLiteralDivide, /*247*/ 
	htmlLiteraloSlash, /*248*/ 
	htmlLiteraluGrave, /*249*/ 
	htmlLiteraluAccute, /*250*/ 
	htmlLiteraluCircumflex, /*251*/ 
	htmlLiteraluUmlaut, /*252*/ 
	htmlLiteralyAcute, /*253*/ 
	htmlLiteralthorn, /*254*/ 
	htmlLiteralyUmlaut, /*255*/ 
	HTMLLiteralsLimit,
	HTMLLiteralsMakeLong=-1000000
} HTMLLiteralsEnum;
#define NickManKeyStrn 18400
typedef enum {
	NickManKeyLimit,
	NickManKeyMakeLong=-1000000
} NickManKeyEnum;
#define NickManLabelStrn 30100
typedef enum {
	NickManLabelLimit,
	NickManLabelMakeLong=-1000000
} NickManLabelEnum;
#define NickOptKeyStrn 30200
typedef enum {
	NickOptKeyLimit,
	NickOptKeyMakeLong=-1000000
} NickOptKeyEnum;
#define NickOptLabelStrn 30300
typedef enum {
	NickOptLabelLimit,
	NickOptLabelMakeLong=-1000000
} NickOptLabelEnum;
#define NickFileCollapseStrn 30400
typedef enum {
	NickFileCollapseLimit,
	NickFileCollapseMakeLong=-1000000
} NickFileCollapseEnum;
#define HTMLTagsStrn 32100
typedef enum {
	htmlBold=1,
	htmlItalic, /*2*/ 
	htmlUnderline, /*3*/ 
	htmlUUnderline, /*4*/ 
	htmlNoFill, /*5*/ 
	htmlParam, /*6*/ 
	htmlTag, /*7*/ 
	htmlFixed, /*8*/ 
	htmlSmaller, /*9*/ 
	htmlBigger, /*10*/ 
	htmlCenter, /*11*/ 
	htmlLeft, /*12*/ 
	htmlRight, /*13*/ 
	htmlFont, /*14*/ 
	htmlColor, /*15*/ 
	htmlPara, /*16*/ 
	htmlh1, /*17*/ 
	htmlh2, /*18*/ 
	htmlh3, /*19*/ 
	htmlh4, /*20*/ 
	htmlh5, /*21*/ 
	htmlh6, /*22*/ 
	htmlEmphasis, /*23*/ 
	htmlStrong, /*24*/ 
	htmlCite, /*25*/ 
	htmlAddress, /*26*/ 
	htmlTitle, /*27*/ 
	htmlHead, /*28*/ 
	htmlBody, /*29*/ 
	htmlBlockQuote, /*30*/ 
	htmlCode, /*31*/ 
	htmlKbd, /*32*/ 
	htmlVar, /*33*/ 
	htmlSamp, /*34*/ 
	htmlTeleType, /*35*/ 
	htmlBR, /*36*/ 
	htmlHR, /*37*/ 
	htmlATerminate, /*38*/ 
	htmlLinkStart, /*39*/ 
	htmlP, /*40*/ 
	htmlUnorderedList, /*41*/ 
	htmlOrderedList, /*42*/ 
	htmlListElement, /*43*/ 
	htmlMenu, /*44*/ 
	htmlDefList, /*45*/ 
	htmlDefTerm, /*46*/ 
	htmlDefDef, /*47*/ 
	htmlBaseRef, /*48*/ 
	HTMLTagsLimit,
	HTMLTagsMakeLong=-1000000
} HTMLTagsEnum;
#define DontUseMeStrn 16000
typedef enum {
	DontUseMeLimit,
	DontUseMeMakeLong=-1000000
} DontUseMeEnum;
#define HTMLDirectiveStrn 18200
typedef enum {
	htmlHTMLDir=1,
	htmlXHTMLDir, /*2*/ 
	htmlDoctypeDir, /*3*/ 
	htmlTitleDir, /*4*/ 
	htmlBodyDir, /*5*/ 
	htmlHeadDir, /*6*/ 
	htmlStyleDir, /*7*/ 
	htmlPETEDir, /*8*/ 
	htmlBRDir, /*9*/ 
	htmlPDir, /*10*/ 
	htmlH1Dir, /*11*/ 
	htmlH2Dir, /*12*/ 
	htmlH3Dir, /*13*/ 
	htmlH4Dir, /*14*/ 
	htmlH5Dir, /*15*/ 
	htmlH6Dir, /*16*/ 
	htmlCenterDir, /*17*/ 
	htmlPreDir, /*18*/ 
	htmlBQDir, /*19*/ 
	htmlDivDir, /*20*/ 
	htmlSpanDir, /*21*/ 
	htmlXmpDir, /*22*/ 
	htmlListDir, /*23*/ 
	htmlPlainDir, /*24*/ 
	htmlOLDir, /*25*/ 
	htmlULDir, /*26*/ 
	htmlAddrDir, /*27*/ 
	htmlDirDir, /*28*/ 
	htmlMenuDir, /*29*/ 
	htmlDLDir, /*30*/ 
	htmlLIDir, /*31*/ 
	htmlDTDir, /*32*/ 
	htmlDDDir, /*33*/ 
	htmlDfnDir, /*34*/ 
	htmlCiteDir, /*35*/ 
	htmlCodeDir, /*36*/ 
	htmlEmDir, /*37*/ 
	htmlKbdDir, /*38*/ 
	htmlSampDir, /*39*/ 
	htmlStrongDir, /*40*/ 
	htmlVarDir, /*41*/ 
	htmlBDir, /*42*/ 
	htmlIDir, /*43*/ 
	htmlUDir, /*44*/ 
	htmlTTDir, /*45*/ 
	htmlADir, /*46*/ 
	htmlHRDir, /*47*/ 
	htmlImgDir, /*48*/ 
	htmlTableDir, /*49*/ 
	htmlTRDir, /*50*/ 
	htmlTHDir, /*51*/ 
	htmlTDDir, /*52*/ 
	htmlFontDir, /*53*/ 
	htmlSmallDir, /*54*/ 
	htmlBigDir, /*55*/ 
	htmlBaseDir, /*56*/ 
	htmlBasefontDir, /*57*/ 
	htmlXTabDir, /*58*/ 
	htmlCommentDir, /*59*/ 
	htmlEmbedDir, /*60*/ 
	htmlObjectDir, /*61*/ 
	htmlXStyleDir, /*62*/ 
	htmlScriptDir, /*63*/ 
	htmlMetaDir, /*64*/ 
	htmlBDODir, /*65*/ 
	htmlCaptionDir, /*66*/ 
	htmlColGroupDir, /*67*/ 
	htmlColDir, /*68*/ 
	htmlTHeadDir, /*69*/ 
	htmlTFootDir, /*70*/ 
	htmlTBodyDir, /*71*/ 
	htmlSigSepDir, /*72*/ 
	HTMLDirectiveLimit,
	HTMLDirectiveMakeLong=-1000000
} HTMLDirectiveEnum;
#define HTMLAttributeStrn 16100
typedef enum {
	htmlBaseAttr=1,
	htmlSrcAttr, /*2*/ 
	htmlIDAttr, /*3*/ 
	htmlAlignAttr, /*4*/ 
	htmlFaceAttr, /*5*/ 
	htmlSizeAttr, /*6*/ 
	htmlColorAttr, /*7*/ 
	htmlHrefAttr, /*8*/ 
	htmlAltAttr, /*9*/ 
	htmlStyleAttr, /*10*/ 
	htmlCiteAttr, /*11*/ 
	htmlTypeAttr, /*12*/ 
	htmlStartAttr, /*13*/ 
	htmlValueAttr, /*14*/ 
	htmlCompactAttr, /*15*/ 
	htmlHeightAttr, /*16*/ 
	htmlWidthAttr, /*17*/ 
	htmlNoShadeAttr, /*18*/ 
	htmlBorderAttr, /*19*/ 
	htmlHSpaceAttr, /*20*/ 
	htmlVSpaceAttr, /*21*/ 
	htmlBGColorAttr, /*22*/ 
	htmlTextAttr, /*23*/ 
	htmlLinkAttr, /*24*/ 
	htmlCommentAttr, /*25*/ 
	htmlDataAttr, /*26*/ 
	htmlEudoraAttr, /*27*/ 
	htmlHTTPEquivAttr, /*28*/ 
	htmlContentAttr, /*29*/ 
	htmlCharsetAttr, /*30*/ 
	htmlDirAttr, /*31*/ 
	htmlCellSpacingAttr, /*32*/ 
	htmlCellPaddingAttr, /*33*/ 
	htmlVAlignAttr, /*34*/ 
	htmlCharAttr, /*35*/ 
	htmlCharOffAttr, /*36*/ 
	htmlSpanAttr, /*37*/ 
	htmlRowSpanAttr, /*38*/ 
	htmlColSpanAttr, /*39*/ 
	htmlNoWrapAttr, /*40*/ 
	htmlAbbrAttr, /*41*/ 
	HTMLAttributeLimit,
	HTMLAttributeMakeLong=-1000000
} HTMLAttributeEnum;
#define HTMLAlignStrn 16200
typedef enum {
	htmlTopAlign=1,
	htmlBottomAlign, /*2*/ 
	htmlMiddleAlign, /*3*/ 
	htmlLeftAlign, /*4*/ 
	htmlRightAlign, /*5*/ 
	htmlDefaultAlign, /*6*/ 
	htmlCenterAlign, /*7*/ 
	htmlJustifyAlign, /*8*/ 
	htmlCharAlign, /*9*/ 
	htmlRTLDir, /*10*/ 
	htmlLTRDir, /*11*/ 
	HTMLAlignLimit,
	HTMLAlignMakeLong=-1000000
} HTMLAlignEnum;
#define HTMLTypesStrn 16400
typedef enum {
	htmlCiteType=1,
	htmlDiskType, /*2*/ 
	htmlSquareType, /*3*/ 
	htmlCircleType, /*4*/ 
	htmlNumberType, /*5*/ 
	htmlLowAlphaType, /*6*/ 
	htmlUpAlphaType, /*7*/ 
	htmlLowRomanType, /*8*/ 
	htmlUpRomanType, /*9*/ 
	htmlAutoURLType, /*10*/ 
	HTMLTypesLimit,
	HTMLTypesMakeLong=-1000000
} HTMLTypesEnum;
#define UnusedStrn 16500
typedef enum {
	UnusedLimit,
	UnusedMakeLong=-1000000
} UnusedEnum;
#define HTMLStylesStrn 16900
typedef enum {
	htmlMarginTopStyle=1,
	htmlMarginBottomStyle, /*2*/ 
	htmlTextIndentStyle, /*3*/ 
	HTMLStylesLimit,
	HTMLStylesMakeLong=-1000000
} HTMLStylesEnum;
#define MailboxCollapseStrn 15100
typedef enum {
	MailboxCollapseLimit,
	MailboxCollapseMakeLong=-1000000
} MailboxCollapseEnum;
#define FiltMetaEnglishStrn 15200
typedef enum {
	fmeRecip=1,
	fmeAddrsee, /*2*/ 
	fmeAny, /*3*/ 
	fmeBody, /*4*/ 
	fmePersonality, /*5*/ 
	fmeJunk, /*6*/ 
	FiltMetaEnglishLimit,
	FiltMetaEnglishMakeLong=-1000000
} FiltMetaEnglishEnum;
#define FiltMetaLocalStrn 15300
typedef enum {
	fmlRecip=1,
	fmlRecip2, /*2*/ 
	fmlAny, /*3*/ 
	fmlBody, /*4*/ 
	fmlPersonality, /*5*/ 
	fmlJunk, /*6*/ 
	FiltMetaLocalLimit,
	FiltMetaLocalMakeLong=-1000000
} FiltMetaLocalEnum;
#define WhatWillLotusCallItNextStrn 15700
typedef enum {
	WhatWillLotusCallItNextLimit,
	WhatWillLotusCallItNextMakeLong=-1000000
} WhatWillLotusCallItNextEnum;
#define RegValStrn 15800
typedef enum {
	rvOne=1,
	RegValLimit,
	RegValMakeLong=-1000000
} RegValEnum;
#define ACAPStrn 15900
typedef enum {
	acapCmd=1,
	acapPlain, /*2*/ 
	acapAPOP, /*3*/ 
	acapSearch, /*4*/ 
	acapAuth, /*5*/ 
	acapEntry, /*6*/ 
	acapOK, /*7*/ 
	acapNo, /*8*/ 
	acapBad, /*9*/ 
	acapBye, /*10*/ 
	acapSettings, /*11*/ 
	acapLoginParms, /*12*/ 
	acapOpen, /*13*/ 
	acapClose, /*14*/ 
	acapACAP, /*15*/ 
	acapCram, /*16*/ 
	acapCramMore, /*17*/ 
	acapSearchMumboJumbo, /*18*/ 
	acapSASL, /*19*/ 
	acapXCram, /*20*/ 
	ACAPLimit,
	ACAPMakeLong=-1000000
} ACAPEnum;
#define ACAPExtraStrn 16300
typedef enum {
	acapsNormal=1,
	acapsStatus, /*2*/ 
	acapsLast, /*3*/ 
	acapsPassword, /*4*/ 
	acapsKerberos, /*5*/ 
	acapsAPOP, /*6*/ 
	acapsDouble, /*7*/ 
	acapsSingle, /*8*/ 
	acapsBinHex, /*9*/ 
	acapsUuencode, /*10*/ 
	acapsFinger, /*11*/ 
	ACAPExtraLimit,
	ACAPExtraMakeLong=-1000000
} ACAPExtraEnum;
#define StdSizesStrn 16600
typedef enum {
	StdSizesLimit,
	StdSizesMakeLong=-1000000
} StdSizesEnum;
#define ALRTStringsStrn 16700
typedef enum {
	OK_ASTR=1,
	OK_ASTR_TP, /*2*/ 
	ERR_ASTR, /*3*/ 
	ERR_ASTR_TP, /*4*/ 
	NEW_TOC_ASTR, /*5*/ 
	NEW_TOC_ASTR_TP, /*6*/ 
	PROTO_ERR_ASTR, /*7*/ 
	PROTO_ERR_ASTR_TP, /*8*/ 
	WANNA_SEND_ASTR, /*9*/ 
	WANNA_SEND_ASTR_TP, /*10*/ 
	ICMP_ASTR, /*11*/ 
	ICMP_ASTR_TP, /*12*/ 
	TIMEOUT_ASTR, /*13*/ 
	TIMEOUT_ASTR_TP, /*14*/ 
	YES_CANCEL_ASTR, /*15*/ 
	YES_CANCEL_ASTR_TP, /*16*/ 
	REB_TOC_ASTR, /*17*/ 
	REB_TOC_ASTR_TP, /*18*/ 
	TOC_SALV_ASTR, /*19*/ 
	TOC_SALV_ASTR_TP, /*20*/ 
	OPEN_ERR_ASTR, /*21*/ 
	OPEN_ERR_ASTR_TP, /*22*/ 
	USE_BACKUP_ASTR, /*23*/ 
	USE_BACKUP_ASTR_TP, /*24*/ 
	NO_SERVER_SSL, /*25*/ 
	NO_SERVER_SSL_TP, /*26*/ 
	LIMIT_ASTR, /*27*/ 
	ALRTStringsLimit,
	ALRTStringsMakeLong=-1000000
} ALRTStringsEnum;
#define ALRTStringsOnlyStrn 16800
typedef enum {
	DUMP_ASTR=1,
	UNUSED_NOT_HOME_ASTR, /*2*/ 
	NOT_FOUND_ASTR, /*3*/ 
	DELETE_NON_EMPTY_ASTR, /*4*/ 
	DELETE_EMPTY_ASTR, /*5*/ 
	QUIT_QUEUE_ASTR, /*6*/ 
	CLEAR_DROP_ASTR, /*7*/ 
	READ_ONLY_ASTR, /*8*/ 
	XFER_TO_OUT_ASTR, /*9*/ 
	REMOVE_SPELL_ASTR, /*10*/ 
	NICK_REP_ASTR, /*11*/ 
	ALIAS_OR_REAL_ASTR, /*12*/ 
	INSIST_SETTINGS_ASTR, /*13*/ 
	UNUSED_ASTR, /*14*/ 
	TBAR_REM_ASTR, /*15*/ 
	ONLINE_ASTR, /*16*/ 
	MDN_ASTR, /*17*/ 
	FILT_MB_RENAME_ASTR, /*18*/ 
	ACAP_RETRY_ASTR, /*19*/ 
	ACAP_RETRY_CNCL_ASTR, /*20*/ 
	QUIT_THREAD_RUN_ASTR, /*21*/ 
	SAVE_CHANGES_ASTR, /*22*/ 
	ATTACH_APP_ASTR, /*23*/ 
	ATTACH_APP2_ASTR, /*24*/ 
	DOC_DAMAGED_FMT_ASTR, /*25*/ 
	DELETE_NON_EMPTY_SINGLE_ASTR, /*26*/ 
	DELETE_EMPTY_SINGLE_ASTR, /*27*/ 
	MEMORY_ASTR, /*28*/ 
	DELETE_NON_EMPTY_IMAP_ASTR, /*29*/ 
	DELETE_NON_EMPTY_SINGLE_IMAP_ASTR, /*30*/ 
	NEW_IMAP_TOC_ASTR, /*31*/ 
	RESET_STATS_ASTR, /*32*/ 
	LIMIT_ASTR_ONLY, /*33*/ 
	ALRTStringsOnlyLimit,
	ALRTStringsOnlyMakeLong=-1000000
} ALRTStringsOnlyEnum;
#define UserCustomizeStrn 18100
typedef enum {
	UserCustomizeLimit,
	UserCustomizeMakeLong=-1000000
} UserCustomizeEnum;
#define IEInitStrn 18500
typedef enum {
	IEInitLimit,
	IEInitMakeLong=-1000000
} IEInitEnum;
#define ClockInitStrn 18600
typedef enum {
	ClockInitLimit,
	ClockInitMakeLong=-1000000
} ClockInitEnum;
#define RecentDirServStrn 18700
typedef enum {
	RecentDirServLimit,
	RecentDirServMakeLong=-1000000
} RecentDirServEnum;
#define POPCapaStrn 18800
typedef enum {
	pcapaTop=1,
	pcapaPipelining, /*2*/ 
	pcapaUser, /*3*/ 
	pcapaExpire, /*4*/ 
	pcapaUIDL, /*5*/ 
	pcapaMangle, /*6*/ 
	pcapaXMangle, /*7*/ 
	pcapaSTLS, /*8*/ 
	pcapaSASL, /*9*/ 
	pcapaAuthRespCode, /*10*/ 
	pcapaLimit, /*11*/ 
	POPCapaLimit,
	POPCapaMakeLong=-1000000
} POPCapaEnum;
#define POPCmdsStrn 3000
typedef enum {
	kpcUser=1,
	kpcPass, /*2*/ 
	kpcStat, /*3*/ 
	kpcRetr, /*4*/ 
	kpcDele, /*5*/ 
	kpcQuit, /*6*/ 
	kpcTop, /*7*/ 
	kpcList, /*8*/ 
	kpcApop, /*9*/ 
	kpcLast, /*10*/ 
	kpcXmit, /*11*/ 
	kpcXlst, /*12*/ 
	kpcUidl, /*13*/ 
	kpcCapa, /*14*/ 
	kpcStls, /*15*/ 
	kpcAuth, /*16*/ 
	POPCmdsLimit,
	POPCmdsMakeLong=-1000000
} POPCmdsEnum;
#define CmdKeyStrn 18900
typedef enum {
	CmdKeyLimit,
	CmdKeyMakeLong=-1000000
} CmdKeyEnum;
#define SortsStrn 19000
typedef enum {
	SortsLimit,
	SortsMakeLong=-1000000
} SortsEnum;
#define IMAPOperationsStrn 19100
typedef enum {
	kIMAPUndefined=1,
	kIMAPResync, /*2*/ 
	kIMAPFetch, /*3*/ 
	kIMAPDelete, /*4*/ 
	kIMAPUndelete, /*5*/ 
	kIMAPTransfer, /*6*/ 
	kIMAPExpunge, /*7*/ 
	kIMAPList, /*8*/ 
	kIMAPTrashLocate, /*9*/ 
	kIMAPCreateMailbox, /*10*/ 
	kIMAPDeleteMailbox, /*11*/ 
	kIMAPRenameMailbox, /*12*/ 
	kIMAPMoveMailbox, /*13*/ 
	kIMAPFetchAttachment, /*14*/ 
	kIMAPSearching, /*15*/ 
	kIMAPUpdateLocal, /*16*/ 
	kIMAPEmptyTrash, /*17*/ 
	kIMAPQueueFlagChange, /*18*/ 
	kIMAPCompleteResync, /*19*/ 
	kIMAPJunkLocate, /*20*/ 
	IMAPOperationsLimit,
	IMAPOperationsMakeLong=-1000000
} IMAPOperationsEnum;
#define IMAPErrorsStrn 19200
typedef enum {
	kIMAPMemErr=1,
	kIMAPNoServerErr, /*2*/ 
	kIMAPNoMailstreamErr, /*3*/ 
	kIMAPNoAccountErr, /*4*/ 
	kIMAPNoMailboxErr, /*5*/ 
	kIMAPSelectMailboxErr, /*6*/ 
	kIMAPMailboxNameInvalid, /*7*/ 
	kIMAPCreateStreamErr, /*8*/ 
	kIMAPStreamIsLockedErr, /*9*/ 
	kIMAPCreateMailboxErr, /*10*/ 
	kIMAPDeleteMailboxErr, /*11*/ 
	kIMAPRenameMailboxErr, /*12*/ 
	kIMAPMoveMailboxErr, /*13*/ 
	kIMAPNotConnectedErr, /*14*/ 
	kIMAPNoMessagesErr, /*15*/ 
	kIMAPDeleteMessage, /*16*/ 
	kIMAPUndeleteMessage, /*17*/ 
	kIMAPCopyErr, /*18*/ 
	kIMAPNotIMAPPersErr, /*19*/ 
	kIMAPNotIMAPMailboxErr, /*20*/ 
	kIMAPListErr, /*21*/ 
	kIMAPListInUseErr, /*22*/ 
	kIMAPTrashLocateErr, /*23*/ 
	kIMAPStubFileErr, /*24*/ 
	kIMAPTempFileExistErr, /*25*/ 
	kIMAPAttachmentFetchErr, /*26*/ 
	kIMAPAttachmentDecodeErr, /*27*/ 
	kIMAPSearchMailboxErr, /*28*/ 
	kIMAPMailboxChangedErr, /*29*/ 
	kIMAPMailboxReadOnly, /*30*/ 
	kIMAPExpungeMailbox, /*31*/ 
	kIMAPJunkLocateErr, /*32*/ 
	kIMAPPartialFetchErr, /*33*/ 
	IMAPErrorsLimit,
	IMAPErrorsMakeLong=-1000000
} IMAPErrorsEnum;
#define ServerMenuStrnStrn 19400
typedef enum {
	ksvmNone=1,
	ksvmFetch, /*2*/ 
	ksvmDelete, /*3*/ 
	ksvmBoth, /*4*/ 
	ksvmLimit, /*5*/ 
	kisvmFetchMessage, /*6*/ 
	kisvmFetchAttachments, /*7*/ 
	kisvmDelete, /*8*/ 
	kisvmRemoveCache, /*9*/ 
	kisvmLimit, /*10*/ 
	kimsvmFetchMessage, /*11*/ 
	kimsvmFetchAttachments, /*12*/ 
	kimsvmDelete, /*13*/ 
	kimsvmRemoveCache, /*14*/ 
	kimsvmLimit, /*15*/ 
	ServerMenuStrnLimit,
	ServerMenuStrnMakeLong=-1000000
} ServerMenuStrnEnum;
#define BulkHeadersStrn 19500
typedef enum {
	BulkHeadersLimit,
	BulkHeadersMakeLong=-1000000
} BulkHeadersEnum;
#define LinkHistoryLabelsStrn 19800
typedef enum {
	klhlRemind=1,
	klhlBookmark, /*2*/ 
	klhlAttempted, /*3*/ 
	klhlNone, /*4*/ 
	klhlNotDisplayed, /*5*/ 
	klhlLimit, /*6*/ 
	LinkHistoryLabelsLimit,
	LinkHistoryLabelsMakeLong=-1000000
} LinkHistoryLabelsEnum;
#define URLQueryPartStrn 19900
typedef enum {
	queryRealname=1,
	queryRegFirst, /*2*/ 
	queryRegLast, /*3*/ 
	queryRegCode, /*4*/ 
	queryOldReg, /*5*/ 
	queryEmail, /*6*/ 
	queryProfile, /*7*/ 
	queryDestination, /*8*/ 
	queryAdID, /*9*/ 
	queryPlatform, /*10*/ 
	queryProduct, /*11*/ 
	queryVersion, /*12*/ 
	queryDistributorID, /*13*/ 
	queryMode, /*14*/ 
	queryAction, /*15*/ 
	queryTopic, /*16*/ 
	queryRegLevel, /*17*/ 
	queryLang, /*18*/ 
	queryQuery, /*19*/ 
	URLQueryPartLimit,
	URLQueryPartMakeLong=-1000000
} URLQueryPartEnum;
#define URLActionStrn 20300
typedef enum {
	actionPay=1,
	actionRegisterFree, /*2*/ 
	actionRegisterAd, /*3*/ 
	actionLostCode, /*4*/ 
	actionUpdate, /*5*/ 
	actionArchived, /*6*/ 
	actionProfile, /*7*/ 
	actionClickThrough, /*8*/ 
	actionSupport, /*9*/ 
	actionHelpNoAds, /*10*/ 
	actionHelpNoQT, /*11*/ 
	actionIntro, /*12*/ 
	actionRegisterBox, /*13*/ 
	actionProfileidFAQ, /*14*/ 
	actionProfileidReqd, /*15*/ 
	actionPayReg, /*16*/ 
	actionRegister50box, /*17*/ 
	actionSearch, /*18*/ 
	actionHelpText, /*19*/ 
	actionSite, /*20*/ 
	URLActionLimit,
	URLActionMakeLong=-1000000
} URLActionEnum;
#define URLSupportTopicStrn 20400
typedef enum {
	topicAdFailure=1,
	topicNoQuickTime, /*2*/ 
	topicTutorial, /*3*/ 
	topicFAQ, /*4*/ 
	topicSearch, /*5*/ 
	topicNewsgroups, /*6*/ 
	topicQuery, /*7*/ 
	topicJunkDown, /*8*/ 
	URLSupportTopicLimit,
	URLSupportTopicMakeLong=-1000000
} URLSupportTopicEnum;
#define URLModeStrn 20500
typedef enum {
	modePaid=1,
	modeAd, /*2*/ 
	modeFree, /*3*/ 
	URLModeLimit,
	URLModeMakeLong=-1000000
} URLModeEnum;
#define RegCodeHeadStrn 20600
typedef enum {
	hFileType=1,
	hRegFirst, /*2*/ 
	hRegLast, /*3*/ 
	hRegCode, /*4*/ 
	hProfile, /*5*/ 
	hRegNeed, /*6*/ 
	hMailedTo, /*7*/ 
	hDelete, /*8*/ 
	RegCodeHeadLimit,
	RegCodeHeadMakeLong=-1000000
} RegCodeHeadEnum;
#define RegCodeFileTypeStrn 20700
typedef enum {
	typeProfile=1,
	typeRegCode, /*2*/ 
	RegCodeFileTypeLimit,
	RegCodeFileTypeMakeLong=-1000000
} RegCodeFileTypeEnum;
#define ModeNamesStrn 20900
typedef enum {
	freeModeName=1,
	adModeName, /*2*/ 
	paidModeName, /*3*/ 
	unknownModeName, /*4*/ 
	ModeNamesLimit,
	ModeNamesMakeLong=-1000000
} ModeNamesEnum;
#define LanguageIdStrn 21000
typedef enum {
	LanguageIdLimit,
	LanguageIdMakeLong=-1000000
} LanguageIdEnum;
#define LanguageId2Strn 21100
typedef enum {
	LanguageId2Limit,
	LanguageId2MakeLong=-1000000
} LanguageId2Enum;
#define StatHTMLStrn 21200
typedef enum {
	sStatRecdEmail=1,
	sStatSentEmail, /*2*/ 
	sStatJunkPercent, /*3*/ 
	sStatDay, /*4*/ 
	sStatToday, /*5*/ 
	sStatYesterday, /*6*/ 
	sStatWeek, /*7*/ 
	sStatThisWeek, /*8*/ 
	sStatLastWeek, /*9*/ 
	sStatMonth, /*10*/ 
	sStatThisMonth, /*11*/ 
	sStatLastMonth, /*12*/ 
	sStatYear, /*13*/ 
	sStatThisYear, /*14*/ 
	sStatLastYear, /*15*/ 
	sStatMsgZero, /*16*/ 
	sStatMsgOne, /*17*/ 
	sStatMsgMore, /*18*/ 
	sStatPercentZero, /*19*/ 
	sStatPercentOne, /*20*/ 
	sStatPercentMore, /*21*/ 
	sStatAttachZero, /*22*/ 
	sStatAttachOne, /*23*/ 
	sStatAttachMore, /*24*/ 
	sStatMsgsEntry, /*25*/ 
	sStatNoAveEntry, /*26*/ 
	sStatUsageEntry, /*27*/ 
	sStatDivider, /*28*/ 
	sStatSince, /*29*/ 
	sStatHTMLBegin, /*30*/ 
	sStatHTMLEnd, /*31*/ 
	sStatProjected, /*32*/ 
	sStatMsgsRead, /*33*/ 
	sStatAttachRecv, /*34*/ 
	sStatAttachSent, /*35*/ 
	sStatFRR, /*36*/ 
	sStatUsageAct, /*37*/ 
	sStatTopRecd, /*38*/ 
	sStatTopSent, /*39*/ 
	sStatAverage, /*40*/ 
	sStatPicImage, /*41*/ 
	sStatReading, /*42*/ 
	sStatComposing, /*43*/ 
	sStatOther, /*44*/ 
	sStatForward, /*45*/ 
	sStatReply, /*46*/ 
	sStatRedirect, /*47*/ 
	sStatCurrentTime, /*48*/ 
	sStatJunkJunk, /*49*/ 
	sStatMsgAndPercent, /*50*/ 
	sStatJunkFalsePos, /*51*/ 
	sStatJunkFalseNeg, /*52*/ 
	sStatJunkFalseWhite, /*53*/ 
	sStatAccuracy, /*54*/ 
	sStatPercent, /*55*/ 
	StatHTMLLimit,
	StatHTMLMakeLong=-1000000
} StatHTMLEnum;
#define NameQualifiersStrn 21300
typedef enum {
	qualJr=1,
	qualSr, /*2*/ 
	qualTheFirst, /*3*/ 
	qualTheSecond, /*4*/ 
	qualTheThird, /*5*/ 
	qualTheFourth, /*6*/ 
	qualEsq, /*7*/ 
	qualPhd, /*8*/ 
	qualMD, /*9*/ 
	NameQualifiersLimit,
	NameQualifiersMakeLong=-1000000
} NameQualifiersEnum;
#define NameHonorificsStrn 21400
typedef enum {
	honorDr=1,
	honorMr, /*2*/ 
	honorMrs, /*3*/ 
	honorMiss, /*4*/ 
	honorMs, /*5*/ 
	honorMme, /*6*/ 
	honorProf, /*7*/ 
	NameHonorificsLimit,
	NameHonorificsMakeLong=-1000000
} NameHonorificsEnum;
#define OffensivenessStrn 21500
typedef enum {
	OffensivenessLimit,
	OffensivenessMakeLong=-1000000
} OffensivenessEnum;
#define ImportOperationsStrn 21700
typedef enum {
	kImportSetupErr=1,
	kImportSettingsErr, /*2*/ 
	kImportMessagesErr, /*3*/ 
	kImportSignaturesErr, /*4*/ 
	kImportAddressesErr, /*5*/ 
	ImportOperationsLimit,
	ImportOperationsMakeLong=-1000000
} ImportOperationsEnum;
#define ImportErrorsStrn 21800
typedef enum {
	kImportUserCancelled=1,
	kImportQueryError, /*2*/ 
	kImportMemError, /*3*/ 
	kImportNoSettingsError, /*4*/ 
	kImportSettingsError, /*5*/ 
	kImportNoDBError, /*6*/ 
	kImportMailboxesError, /*7*/ 
	kImportMessagesError, /*8*/ 
	kImportNoSignaturesError, /*9*/ 
	kImportSignaturesError, /*10*/ 
	kImportNoAddressesError, /*11*/ 
	kImportAddressesError, /*12*/ 
	kCreateAddressbookError, /*13*/ 
	ImportErrorsLimit,
	ImportErrorsMakeLong=-1000000
} ImportErrorsEnum;
#define ImportedFieldNamesStrn 21900
typedef enum {
	kImportedName=1,
	kImportedAddress, /*2*/ 
	kImportedPhone, /*3*/ 
	kImportedFax, /*4*/ 
	ImportedFieldNamesLimit,
	ImportedFieldNamesMakeLong=-1000000
} ImportedFieldNamesEnum;
#define FRRStrn 22100
typedef enum {
	ksForward=1,
	ksReply, /*2*/ 
	ksRedirect, /*3*/ 
	FRRLimit,
	FRRMakeLong=-1000000
} FRREnum;
#define StatXMLStrn 22200
typedef enum {
	ksStatXMLVersion=1,
	ksStatDocEntry, /*2*/ 
	ksStatVersion, /*3*/ 
	ksStatStartTime, /*4*/ 
	ksStatJunkStartTime, /*5*/ 
	ksStatCurrentTime, /*6*/ 
	ksStatReceivedMail, /*7*/ 
	ksStatSentMail, /*8*/ 
	ksStatFaceTime, /*9*/ 
	ksStatScoredJunk, /*10*/ 
	ksStatScoredNotJunk, /*11*/ 
	ksStatWhiteList, /*12*/ 
	ksStatFalsePositives, /*13*/ 
	ksStatFalseNegatives, /*14*/ 
	ksStatFalseWhiteList, /*15*/ 
	ksStatReceivedAttach, /*16*/ 
	ksStatSentAttach, /*17*/ 
	ksStatReadMsg, /*18*/ 
	ksStatForwardMsg, /*19*/ 
	ksStatReplyMsg, /*20*/ 
	ksStatRedirectMsg, /*21*/ 
	ksStatFaceTimeRead, /*22*/ 
	ksStatFaceTimeCompose, /*23*/ 
	ksStatFaceTimeOther, /*24*/ 
	ksStatCurrent, /*25*/ 
	ksStatPrevious, /*26*/ 
	ksStatAveSum, /*27*/ 
	ksStatTotal, /*28*/ 
	ksStatDay, /*29*/ 
	ksStatWeek, /*30*/ 
	ksStatMonth, /*31*/ 
	ksStatYear, /*32*/ 
	StatXMLLimit,
	StatXMLMakeLong=-1000000
} StatXMLEnum;
#define IntlFontsStrn 22300
typedef enum {
	IntlFontsLimit,
	IntlFontsMakeLong=-1000000
} IntlFontsEnum;
#define ABReservedTagsStrn 22400
typedef enum {
	abTagName=1,
	abTagFirst, /*2*/ 
	abTagLast, /*3*/ 
	abTagEmail, /*4*/ 
	abTagAddress, /*5*/ 
	abTagCity, /*6*/ 
	abTagState, /*7*/ 
	abTagCountry, /*8*/ 
	abTagZip, /*9*/ 
	abTagPhone, /*10*/ 
	abTagFax, /*11*/ 
	abTagMobile, /*12*/ 
	abTagPrimary, /*13*/ 
	abTagWeb, /*14*/ 
	abTagTitle, /*15*/ 
	abTagCompany, /*16*/ 
	abTagAddress2, /*17*/ 
	abTagCity2, /*18*/ 
	abTagState2, /*19*/ 
	abTagCountry2, /*20*/ 
	abTagZip2, /*21*/ 
	abTagPhone2, /*22*/ 
	abTagFax2, /*23*/ 
	abTagMobile2, /*24*/ 
	abTagWeb2, /*25*/ 
	abTagOtherEmail, /*26*/ 
	abTagOtherPhone, /*27*/ 
	abTagOtherWeb, /*28*/ 
	abTagNote, /*29*/ 
	abTagPicture, /*30*/ 
	abNickname, /*31*/ 
	abTagAIM, /*32*/ 
	abTagAIM2, /*33*/ 
	ABReservedTagsLimit,
	ABReservedTagsMakeLong=-1000000
} ABReservedTagsEnum;
#define ABViewByTagsStrn 22500
typedef enum {
	abSortName=1,
	abSortFirst, /*2*/ 
	abSortLast, /*3*/ 
	abSortDash1, /*4*/ 
	abSortEmail, /*5*/ 
	abSortDash2, /*6*/ 
	abSortCity, /*7*/ 
	abSortState, /*8*/ 
	abSortCountry, /*9*/ 
	abSortDash3, /*10*/ 
	abSortTitle, /*11*/ 
	abSortCompany, /*12*/ 
	ABViewByTagsLimit,
	ABViewByTagsMakeLong=-1000000
} ABViewByTagsEnum;
#define ABSuppressNickInListTagsStrn 22600
typedef enum {
	abSuppressName=1,
	abSuppressFirst, /*2*/ 
	abSuppressLast, /*3*/ 
	ABSuppressNickInListTagsLimit,
	ABSuppressNickInListTagsMakeLong=-1000000
} ABSuppressNickInListTagsEnum;
#define StatHoursStrn 22700
typedef enum {
	kHourMidnight=1,
	kHour1am, /*2*/ 
	kHour2am, /*3*/ 
	kHour3am, /*4*/ 
	kHour4am, /*5*/ 
	kHour5am, /*6*/ 
	kHour6am, /*7*/ 
	kHour7am, /*8*/ 
	kHour8am, /*9*/ 
	kHour9am, /*10*/ 
	kHour10am, /*11*/ 
	kHour11am, /*12*/ 
	kHourNoon, /*13*/ 
	kHour1pm, /*14*/ 
	kHour2pm, /*15*/ 
	kHour3pm, /*16*/ 
	kHour4pm, /*17*/ 
	kHour5pm, /*18*/ 
	kHour6pm, /*19*/ 
	kHour7pm, /*20*/ 
	kHour8pm, /*21*/ 
	kHour9pm, /*22*/ 
	kHour10pm, /*23*/ 
	kHour11pm, /*24*/ 
	StatHoursLimit,
	StatHoursMakeLong=-1000000
} StatHoursEnum;
#define ABTabOrderStrn 22800
typedef enum {
	ABTabOrderLimit,
	ABTabOrderMakeLong=-1000000
} ABTabOrderEnum;
#define ABHiddenTagsStrn 22900
typedef enum {
	abTagUniqueID=1,
	abTagCategory, /*2*/ 
	abTagChangeBits, /*3*/ 
	ABHiddenTagsLimit,
	ABHiddenTagsMakeLong=-1000000
} ABHiddenTagsEnum;
#define VCardKeywordStrn 23000
typedef enum {
	vcBegin=1,
	vcVCard, /*2*/ 
	vcEnd, /*3*/ 
	vcAdr, /*4*/ 
	vcOrg, /*5*/ 
	vcN, /*6*/ 
	vcAgent, /*7*/ 
	vcType, /*8*/ 
	vcValue, /*9*/ 
	vcEncoding, /*10*/ 
	vcCharset, /*11*/ 
	vcLanguage, /*12*/ 
	vcInline, /*13*/ 
	vcURL, /*14*/ 
	vcContentID, /*15*/ 
	vcCID, /*16*/ 
	vc7Bit, /*17*/ 
	vc8Bit, /*18*/ 
	vcQuotedPrintable, /*19*/ 
	vcBase64, /*20*/ 
	vcLogo, /*21*/ 
	vcPhoto, /*22*/ 
	vcLabel, /*23*/ 
	vcFN, /*24*/ 
	vcTitle, /*25*/ 
	vcSound, /*26*/ 
	vcVersion, /*27*/ 
	vcLang, /*28*/ 
	vcTel, /*29*/ 
	vcEmail, /*30*/ 
	vcTZ, /*31*/ 
	vcGeo, /*32*/ 
	vcNote, /*33*/ 
	vcBday, /*34*/ 
	vcRole, /*35*/ 
	vcRev, /*36*/ 
	vcUID, /*37*/ 
	vcKey, /*38*/ 
	vcMailer, /*39*/ 
	vcDom, /*40*/ 
	vcIntl, /*41*/ 
	vcPostal, /*42*/ 
	vcParcel, /*43*/ 
	vcHome, /*44*/ 
	vcWork, /*45*/ 
	vcPref, /*46*/ 
	vcVoice, /*47*/ 
	vcFax, /*48*/ 
	vcMSG, /*49*/ 
	vcCell, /*50*/ 
	vcPager, /*51*/ 
	vcBBS, /*52*/ 
	vcModem, /*53*/ 
	vcCar, /*54*/ 
	vcISDN, /*55*/ 
	vcVideo, /*56*/ 
	vcAOL, /*57*/ 
	vcAppleLink, /*58*/ 
	vcATTMail, /*59*/ 
	vcCIS, /*60*/ 
	vcEWorld, /*61*/ 
	vcInternet, /*62*/ 
	vcIBMMail, /*63*/ 
	vcMCIMail, /*64*/ 
	vcPowerShare, /*65*/ 
	vcProdigy, /*66*/ 
	vcTLX, /*67*/ 
	vcX400, /*68*/ 
	vcGIF, /*69*/ 
	vcCGM, /*70*/ 
	vcWMF, /*71*/ 
	vcBMP, /*72*/ 
	vcMET, /*73*/ 
	vcPMB, /*74*/ 
	vcDIB, /*75*/ 
	vcPICT, /*76*/ 
	vcTIFF, /*77*/ 
	vcPDF, /*78*/ 
	vcPS, /*79*/ 
	vcJPEG, /*80*/ 
	vcQTime, /*81*/ 
	vcWave, /*82*/ 
	vcAIFF, /*83*/ 
	vcPCM, /*84*/ 
	vcX509, /*85*/ 
	vcPGP, /*86*/ 
	vcXDash, /*87*/ 
	vcUSAscii, /*88*/ 
	vcISO_8859, /*89*/ 
	VCardKeywordLimit,
	VCardKeywordMakeLong=-1000000
} VCardKeywordEnum;
#define ABUnseachableTagsStrn 23200
typedef enum {
	ABUnseachableTagsLimit,
	ABUnseachableTagsMakeLong=-1000000
} ABUnseachableTagsEnum;
#define SASLStrn 23300
typedef enum {
	saslGSSAPI=1,
	saslCramMD5, /*2*/ 
	saslPlain, /*3*/ 
	saslLogin, /*4*/ 
	saslKerbIV, /*5*/ 
	SASLLimit,
	SASLMakeLong=-1000000
} SASLEnum;
#define GSSAPIErrorsStrn 23400
typedef enum {
	kGSSAPIFailure=1,
	kGSSAPIUnknownFailure, /*2*/ 
	kGSSAPIStatus, /*3*/ 
	kGSSAPICredExpiredError, /*4*/ 
	kGSSAPINoCredError, /*5*/ 
	GSSAPIErrorsLimit,
	GSSAPIErrorsMakeLong=-1000000
} GSSAPIErrorsEnum;
#define ConConKeyStrn 4100
typedef enum {
	conConKeyProfile=1,
	conConKeyElement, /*2*/ 
	conConKeyName, /*3*/ 
	conConKeyOutput, /*4*/ 
	conConTypeAny, /*5*/ 
	conConTypeHeader, /*6*/ 
	conConTypeSeparator, /*7*/ 
	conConTypeBodyInitial, /*8*/ 
	conConTypeBody, /*9*/ 
	conConTypeQuote, /*10*/ 
	conConTypeAttribution, /*11*/ 
	conConTypeForwardOn, /*12*/ 
	conConTypeForwardOff, /*13*/ 
	conConTypeQuoteOn, /*14*/ 
	conConTypeQuoteOff, /*15*/ 
	conConTypeSigIntro, /*16*/ 
	conConTypeForward, /*17*/ 
	conConTypePlainForward, /*18*/ 
	conConTypeOutlookQuote, /*19*/ 
	conConTypeSignature, /*20*/ 
	conConTypeMessage, /*21*/ 
	conConTypeAttachment, /*22*/ 
	conConTypeComplete, /*23*/ 
	conConOutTypeRaw, /*24*/ 
	conConOutTypeDisplay, /*25*/ 
	conConOutTypeTruncate, /*26*/ 
	conConOutTypeElide, /*27*/ 
	conConOutTypeRemove, /*28*/ 
	conConOutTypeSummary, /*29*/ 
	conConOutQualLines, /*30*/ 
	conConQualBytes, /*31*/ 
	conConQualHeight, /*32*/ 
	conConQualWidth, /*33*/ 
	conConOutModTrim, /*34*/ 
	conConOutModFlatten, /*35*/ 
	conConOutModQuoteIncrement, /*36*/ 
	conConOutSeparatorRule, /*37*/ 
	ConConKeyLimit,
	ConConKeyMakeLong=-1000000
} ConConKeyEnum;
#define URLNaughtyReasonStrn 4200
typedef enum {
	kURLNSyntax=1,
	kURLNLinkMismatch, /*2*/ 
	kURLNTLD, /*3*/ 
	kURLNIP, /*4*/ 
	kURLNEncoded, /*5*/ 
	URLNaughtyReasonLimit,
	URLNaughtyReasonMakeLong=-1000000
} URLNaughtyReasonEnum;
#define NextAvailableStrnStrn 4300
typedef enum {
	next1=1,
	NextAvailableStrnLimit,
	NextAvailableStrnMakeLong=-1000000
} NextAvailableStrnEnum;

#endif