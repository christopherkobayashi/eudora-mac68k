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

#include "address.h"
#define FILE_NUM 1
// Copyright (c) 1990-1992 by the University of Illinois Board of Trustees
// Copyright (c) 1993-1996 by Qualcomm Incorporated

#pragma segment Addresses

#define AddrWhite(c) (c<=' ')
#define PState(cc,rc,ss)

typedef enum
{
	Regular, Comma, lParen, rParen, lAngle, rAngle,
	lBrak, rBrak, dQuote, Colon, Semicolon, aDone
} AddressCharEnum;
typedef enum
{
	sNoChange= -1,/* remain in current state */
	sPlain, 			/* nothing special going on */
	sParen, 			/* within parentheses */
	sAngle, 			/* within angle brackets */
	sBrak,				/* within square brackets */
	sQuot,				/* with double quotes */
	sTrail, 			/* trailer after an angle bracket */
	sTDone, 			/* the token is done for this address */
/* the following are states that decay immediately into other states */
	sError, 			/* bad address; causes routine to punt */
	sToken, 			/* process completed token (sPlain) */
	sBrakE, 			/* close [] (pushed state (sPlain,sAngle,sTDone)) */
	sQuotE, 			/* close "" (pushed state (sPlain,sAngle,sTDone)) */
	incPar, 			/* increment () level (sParen) */
	decPar, 			/* decrement () level (sParen or pushed state) */
	sColon,				/* rescans as sTDone */
	sSem,					/* just saw a sem.  Finish current address, rescan as sColon */
	sTrailB 			/* beginning of trailer state */
} AddressStateEnum;
char AddrStateTable[sTDone+1][aDone+1] = {
/*							. 			, 			( 			) 			< 			> 			[ 			] 			"       :				;			aDone*/
/* sPlain */		-1, 		sToken, incPar, sError, sAngle, sError, sBrak,	sError, sQuot,	sColon,	sSem,	sToken,
/* sParen */		-1, 		-1, 		incPar, decPar, -1, 		-1, 		-1, 		-1, 		-1, 		-1,			-1,		sError,
/* sAngle */		-1, 		-1, 		incPar, decPar, sError, sTrailB, sBrak, sError, sQuot,	-1,			-1,		sError,
/* sBrak */ 		-1, 		-1, 		-1, 		-1, 		-1, 		-1, 		sError, sBrakE, -1, 		-1,			-1,		sError,
/* sQuot */ 		-1, 		-1, 		-1, 		-1, 		-1, 		-1, 		-1, 		-1, 		sQuotE, -1,			-1,		sError,
/* sTrail */		-1, 		sToken, incPar, decPar, sError, sError, -1, 		-1, 		sQuot,	-1,			sSem,	sToken,
/* sTDone */		-1, 		sToken, incPar, sError, sAngle, sError, sBrak,	sError, sQuot,	sColon,	sSem,	sToken
};
/************************************************************************
 * SuckAddresses - parse the RFC 822 Address format
 * Returns a handle to some storage of the form:
 *			[<length byte>address<nil>]...<nil> 
 ************************************************************************/
OSErr SuckAddresses(BinAddrHandle *addresses,TextAddrHandle	text,Boolean wantComments, Boolean wantErrors,Boolean wantAutoQual,long ***spots)
{
	OSErr err=noErr;
	
	ASSERT(text && *text);
	if (text && *text)
	{
		err = SuckPtrAddresses(addresses,LDRef(text),GetHandleSize(text),wantComments, wantErrors, wantAutoQual,spots);
		UL(text);
	}
	
	return(err);
}

OSErr SuckPtrAddresses(BinAddrHandle *addresses,TextAddrPtr text,long size,Boolean wantComments, Boolean wantErrors, Boolean wantAutoQual,long ***addrSpots)
{
	BinAddrHandle newAddresses;
	UPtr spot;
	short paren=0;
	short count=0;
	AddressStateEnum oldState, state, nextState;
	Byte c;
	AddressCharEnum cClass;
	Str255 addressBuffer;
	UPtr ap;
	Boolean sawAt;
	UPtr addrEnd;
	Str127 autoQual;
	OSErr err = noErr;
	long theSpot;
	Byte lastC;
	
#define AddrFull				(ap-addressBuffer >= sizeof(addressBuffer)-2)
#define AddrEmpty 			(ap==addressBuffer+1)
#define AddrChar(c) 		do {if ((wantComments&&c!='\015'||!AddrWhite(c)) && oldState!=sTDone) {*ap++ = c;if (!AddrWhite(c)) addrEnd=ap;if (c=='@') sawAt = True;}} while (0)
#define CmmntChar(c)		do {if (wantComments && oldState<sTDone && c!='\015') *ap++ = c;} while (0)
#define AddrAny(c)			do {if (oldState<sTDone) *ap++ = c;} while (0)
#define RestartAddr() 	do {ap=addressBuffer+1;sawAt=False;addrEnd=nil;lastC=0;} while(0)

	*addresses = nil;
	if (addrSpots) *addrSpots = NuHandleClear(4);
	
	newAddresses = (UHandle)NuHTempBetter(0L);
	if (newAddresses==nil) return(WarnUser(MEM_ERR,MemError()));
	
	oldState = state = sPlain;
	for (spot=text;(spot<text+size) && (AddrWhite(*spot));spot++);
	if (wantAutoQual)
	{
		GetPref(autoQual,PREF_AUTOQUAL);
		if (*autoQual && autoQual[1] != '@')
			PInsert(autoQual,sizeof(autoQual),"\p@",autoQual+1);
	}
	
	RestartAddr();
	do
	{
		if (spot >= text + size)
			cClass = aDone;
		else
		{
			switch (c = *spot++)
			{
				case ',': 			cClass = Comma; break;
				case '(': 			cClass = lParen; break;
				case ')': 			cClass = rParen; break;
				case '[': 			cClass = lBrak; break;
				case ']': 			cClass = rBrak; break;
				case '<': 			cClass = lAngle; break;
				case '>': 			cClass = rAngle; break;
				case '"':       cClass = dQuote; break;
				case ':':
					if (spot>text+1 && spot[-2]==':' || spot<text+size && spot[0]==':')
						cClass = Regular;
					else
						cClass = Colon; break;
				case ';':       cClass = Semicolon; break;
				default:				cClass = Regular; break;
			}
			// Hacky way of dealing with backslash
			if (lastC=='\\') cClass = Regular;
			lastC = c;
		}
		nextState = AddrStateTable[state][cClass];
rescan:
		PState(cClass,spot[-1],nextState);
		if (nextState == sNoChange) nextState = state;
		switch (nextState)
		{
			case sPlain:
				AddrChar(c);
				break;
			case sAngle:
				if (state==sAngle || wantComments)
					AddrChar(c);
				else
					RestartAddr();
				break;
			case sColon:
				AddrChar(c);
				nextState = sToken;
				goto rescan;
			case sBrak:
				if (state!=sBrak)
					oldState = state;
				AddrAny(c);
				break;
			case sQuot:
				if (state!=sQuot) oldState = state;
				if (oldState==sTrail) CmmntChar(c); else AddrAny(c);
				break;
			case sTDone:
				break;
			case sError:
				count = 0;
				goto parsePunt;
				break;
			case sSem:
			case sToken:
				while (!AddrEmpty && AddrWhite(ap[-1])) ap--; /* strip trailing space */
				*ap++ = '\0';
				if (addressBuffer[1])
				{
					*addressBuffer = ap-addressBuffer - 2;
					if (wantAutoQual && *autoQual && !sawAt && addrEnd && addressBuffer[1]!=';')
					if (!HasFeature (featureFcc) || (HasFeature (featureFcc) && !IsFCCAddr(addressBuffer) && !IsNewsgroupAddr(addressBuffer)))
						{
							PInsert(addressBuffer,sizeof(addressBuffer)-2,autoQual,addrEnd);
							addressBuffer[*addressBuffer+1] = 0;	/* null terminate */
						}
					TrimWhite(addressBuffer);
					TrimInitialWhite(addressBuffer);
					if (PtrPlusHand_(addressBuffer,(Handle)newAddresses,*addressBuffer+2))
					{
						err = MemError();
						SetHandleBig_((Handle)newAddresses,0L);
						WarnUser(MEM_ERR,err);
						count = 0;
						goto parsePunt;
					}
					else
					{
						if (addrSpots && *addrSpots)
						{
							theSpot = spot-text;
							if (PtrPlusHand(&theSpot,(void*)*addrSpots,4))
								ZapHandle(*addrSpots);
						}
						count++;
					}
				}
				RestartAddr();
				oldState = sPlain;
				if (nextState==sSem)
				{
					AddrChar(c);
					nextState = sTDone;
				}
				nextState = sPlain;
				PState(cClass,' ',nextState);
				while ((spot<text+size) && (AddrWhite(*spot))) spot++;
				break;
			case sTrailB:
				nextState = sTrail;
				CmmntChar('>');
				break;
			case sBrakE:
				nextState = oldState;
				PState(cClass,' ',nextState);
				AddrChar(c);
				break;
			case sQuotE:
				nextState = oldState;
				PState(cClass,' ',nextState);
				if (oldState==sTrail) CmmntChar(c); else AddrAny(c);
				break;
			case incPar:
				if (!paren++) oldState=state;
				nextState = sParen;
				PState(cClass,' ',nextState);
				CmmntChar('(');
				break;
			case decPar:
				if (!--paren)
					nextState = oldState;
				else
					nextState = sParen;
				PState(cClass,' ',nextState);
				CmmntChar(')');
				break;
			default:
				CmmntChar(c);
				break;
		}
		state = nextState;
	}
	while (cClass!=aDone && !AddrFull);
	if (PtrPlusHand_("",(UHandle)newAddresses,1))
	{
		err = MemError();
		SetHandleBig_(newAddresses,0L);
		WarnUser(MEM_ERR,err);
	}
parsePunt:	
	if (err||(nextState==sError)||AddrFull)
	{
		ZapHandle(newAddresses);
		if (addrSpots) ZapHandle(*addrSpots);
		if (!err)
		{
			if (wantErrors)
			{
#ifdef CLARENCE_LOOK_HERE
	attach errors to mesg 
#endif
				if (AddrFull) WarnUser(ADDR_TOO_LONG,0);
				else if (nextState==sError) WarnUser(BAD_ADDRESS,0);
			}
			err = paramErr;
		}
	}
	*addresses = newAddresses;
	return(err);
}

/**********************************************************************
 * IsFCCAddr - is an address an FCC?
 **********************************************************************/
Boolean IsFCCAddr(PStr addr)
{
	Str15 prefix;
	UPtr spot;
	
	if (!HasFeature (featureFcc))	//Folder Carbon Copy no FCC in Eudora Light!
		return (false);
		
	spot = PPtrFindSub(GetRString(prefix,FCC_PREFIX),addr+1,*addr);
	return (spot && (spot==addr+1 || spot==addr+2 && addr[1]=='"'));
}

/**********************************************************************
 * SameAddressStr - do two strings hold the same address?
 **********************************************************************/
SameAddressStr(PStr addr1,PStr addr2)
{
	Str255 short1, short2;
	
	return(StringSame(ShortAddr(short1,addr1),ShortAddr(short2,addr2)));
}

/**********************************************************************
 * CanonAddr - put a mailbox and name into canonical form
 **********************************************************************/
PStr CanonAddr(PStr into,PStr addr,PStr name)
{
	Str255 lAddr, lName, fmt;
	
	PCopy(lAddr,name);
	if (PrefIsSet(PREF_UNCOMMA)) Uncomma(lAddr);
	ComposeRString(lName,RNAME_QUOTE,lAddr);

	ShortAddr(lAddr,addr);

	GetRString(fmt,ADD_REALNAME);
	utl_PlugParams(fmt,into,lAddr,lName,nil,nil);
	return(into);
}
		
/**********************************************************************
 * ShortAddr - return just the short form of an address; user@host
 **********************************************************************/
PStr ShortAddr(PStr shortAddr, PStr longAddr)
{
	OSErr err;
	UHandle addresses;
	
	err = SuckPtrAddresses(&addresses,longAddr+1,*longAddr,False,False,True,nil);
	
	if (addresses && **addresses)
		PCopy(shortAddr,*addresses);
	else
		PCopy(shortAddr,longAddr);
	DisposeHandle(addresses);
	return(shortAddr);
}


short CountAddresses (Handle addresses, short justTellMeIfThereAreAtLeastThisMany)

{
  UPtr	spot,
  			end;
  short	count;
	
	count	= 0;
	spot	= *addresses;
	end		= spot + GetHandleSize (addresses);
	
	while (spot < end && *spot) {
		++count;
		if (justTellMeIfThereAreAtLeastThisMany && count == justTellMeIfThereAreAtLeastThisMany)	
			spot = end;
		else
			spot += (*spot + 2);
	}
	return (count);
}

/************************************************************************
 * AddAddressHashUniq - add the hash of an address to an accumulator, if
 *  it's not already there.  Returns true if the address is not already there
 ************************************************************************/
Boolean AddAddressHashUniq(PStr address,AccuPtr a)
{
	uLong addressHash;
	Str255 shortAddr;
	
	// form a very canonical hash
	ShortAddr(shortAddr,address);
	MyLowerStr(shortAddr);
	addressHash = Hash(shortAddr);
	
	// look for it
	if (0>AccuFindLong(a,addressHash))
	{
		// add it
		AccuAddPtr(a,&addressHash,sizeof(addressHash));
		return true;
	}
	else
		// already there!
		return false;
}