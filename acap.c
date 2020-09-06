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

#include "acap.h"
#define FILE_NUM 103
#pragma segment ACAP

/************************************************************************
 * Simplified ACAP handling
 *   There are lots of things not up to snuff here.  If we're really
 *   going to use ACAP as intended by ACAP Nazis, it will have to be made
 *   much less efficient.
 ************************************************************************/
typedef enum
{
	adOK = 1,
	adManual,
	adQuit,
	adCancel,
	adServer,
	adUser,
	adPassword,
	adCapsLockWarning,
	adLimit
} ACAPDlogEnum;

typedef enum
{
	acsTag,	// expecting tag
	acsResponse,	// expecting response
	acsText,	// expecting open paren
	acsFlush,	// flush to newline
	acsLimit
} ACAPCmdState, *ACAPCmdStatePtr, **ACAPCmdStateHandle;

typedef enum
{
	alsFree,	// not looking at anything yet
	alsAtom,	// collecting atom
	alsQuote,	// collecting quote
	alsEscapedQuote,	// collecting quote, just saw a backslash
	alsDigit,	// saw an open brace
	alsCloseBrace,	// saw a close brace
	alsBraceCR,	// saw the cr after the close brace
	alsLiteral,	// inside a literal
	alsCR,	// just saw a CR
	alsLF,	// just saw an LF
	alsLimit
} ACAPLexState, *ACAPLexStatePtr, **ACAPLexStateHandle;
	
typedef enum
{
	acaptString,
	acaptOpenParen,
	acaptCloseParen,
	acaptNewline,
	acaptLimit
} ACAPToken, *ACAPTokenPtr, **ACAPTokenHandle;

typedef struct AccuLL **AccuLLHandle;
typedef struct AccuLL
{
	Accumulator strings;
	AccuLLHandle next;
} AccuLL, *AccuLLPtr, **AccuLLHandle;

typedef struct
{
	TransStream stream;	// open stream
	ACAPCmdState state; // current command state
	long tag;					// current tag
	ACAPEnum response;	// current response
	Str255 responseValue;	// text of the response
	Str255 tokenValue;	// text of the token
	ACAPToken token;	// token type
	Accumulator text;	// lists of strings of the result.  Each list is separated by a nul.
	UHandle buffer;			// incoming data buffer
	long spot;					// spot in the buffer
	long end;						// end of the buffer
	Boolean hasPlain;		// has plaintext authentication
	Boolean hasAPOP;		// has apop authentication
	Boolean hasCram;		// has cram-md5 authentication
} ACAPState, *ACAPStatePtr, **ACAPStateHandle;

typedef struct
{
	short pref;
	short type;
	long reserved;
	Str255 name;
} ACAPResTemplate, *ACAPResTemplatePtr, **ACAPResTemplateHandle;

#define ACAPDone(r) (r==acapBad || r==acapOK || r==acapNo || r==acapBye)

OSErr GetACAPLogin(PStr server, PStr user, PStr password, Boolean giveQuit);
OSErr ACAPLogin(PStr server, PStr user, PStr password, ACAPStateHandle state);
OSErr ACAPLoadLo(ACAPStateHandle state);
OSErr ParseACAP(ACAPStateHandle state);
OSErr ACAPFill(ACAPStateHandle state);
ACAPStateHandle NewACAPState(void);
OSErr ACAPSetPrefRes(PStr name,PStr value,Handle acapRes);

void DisposeACAPState(ACAPStateHandle state);
OSErr ACAPBanner(ACAPStateHandle state,PStr challenge);
OSErr ACAPGetToken(ACAPStateHandle state);
OSErr ACAPLoginWAPOP(PStr user, PStr password, ACAPStateHandle state,PStr challenge);
OSErr ACAPLoginPlain(PStr user, PStr password, ACAPStateHandle state);
OSErr ACAPLoginWCram(PStr user, PStr password, ACAPStateHandle state);
OSErr ACAPLoginWFunnyCram(PStr user, PStr password, ACAPStateHandle state,PStr challenge);
PStr NextAccuString(PStr string,AccuPtr a,long *context);
OSErr ACAPReadAndSet(ACAPStateHandle state);
OSErr ACAPGetServer(PStr server);
OSErr ACAPTryServer(PStr server);
OSErr ACAPGetServerLo(PStr remaining,PStr server);
pascal Boolean ACAPLoginFilter(DialogPtr dgPtr,EventRecord *event,short *item);

/************************************************************************
 * ACAPLoad - load initial settings with acap
 ************************************************************************/
OSErr ACAPLoad(Boolean giveQuit)
{
	Str63 server;
	Str63 user;
	Str63 password;
	Str255 s;
	ACAPStateHandle state=NewACAPState();
	OSErr err = fnfErr;
	short retry;
	Boolean hunt = True;
	Boolean failedOnce = False;
	
	if (state)
	{
		OTInitOpenTransport();	// just in case
		*user = 0;
		*server = 0;
		do
		{
			if (!hunt || !(err=ACAPGetServer(server)))
			{
				if (!*user) GetPref(user,PREF_ACAP_USER);
				if (!*user) GetPOPInfo(user,password);
				GetPref(password,PREF_ACAP_PASS);
				if (!*password) PCopy(password,(*CurPersSafe)->password);
				if (!*password) GetPref(password,PREF_PASS_TEXT);
				if (!failedOnce && *user && *password && *server || !(err=GetACAPLogin(server,user,password,giveQuit)))
				{
					failedOnce = True;
					if (!*server) err = ACAPGetServer(server);
					if (!err)
					{
						OpenProgress();
						ProgressMessageR(kpTitle,ACAPPING);
						if (!(err = ACAPLogin(server,user,password,state)))
						{
							err = ACAPLoadLo(state);
							if (!*(*CurPersSafe)->password) PSCopy((*CurPersSafe)->password,password);
						}
						else *password = 0;
						if ((*state)->stream) DisTrans((*state)->stream);
						CloseProgress();
					}
				}
				if (!err)
				{
					// we found a server.  remember it
					SetPref(PREF_ACAP_SERVER,server);
					SetPref(PREF_ACAP_USER,user);
					if (PrefIsSet(PREF_SAVE_PASSWORD)) SetPref(PREF_ACAP_PASS,password);
					if (!*GetPOPPref(s)) WarnUser(ACAP_FAILED,err);
					break;
				}
			}
			hunt = False;
			if (err)
			{
				if (err!=userCanceledErr)
				{
					retry = ReallyDoAnAlert(giveQuit ? ACAP_RETRY_ALRT:ACAP_RETRY_CNCL_ALRT,Normal);
					if (retry==adOK) err = 0;
					else if (retry == adQuit || !giveQuit && retry==adManual)
					{
						if (giveQuit) {Cleanup(); ExitToShell();}
					}
				}
			}
		}
		while (!err);
	}
	DisposeACAPState(state);
	return(err);
}

/************************************************************************
 * ACAPLoginFilter - a ModalDialog filter for the acap login
 ************************************************************************/
pascal Boolean ACAPLoginFilter(DialogPtr dgPtr,EventRecord *event,short *item)
{
	extern UHandle PwChars;
	MyWindowPtr	win = GetDialogMyWindowPtr (dgPtr);
	
	if (MiniMainLoop(event))
	{
		*item = adManual;
		return(True);
	}
	if (win && win->dialogRefcon=='acap')
	{
		if (event->what==keyDown || event->what==autoKey)
		{
			char key = event->message & charCodeMask;
			switch (key)
			{
				case enterChar:
				case returnChar:
					*item = 1;
					return(True);
					break;
				case backSpace:
					if (*item==adPassword && **PwChars) --**PwChars;
					return(False);
					break;
				case tabChar:
					return(False);
					break;
				case '.':
					if (event->modifiers & cmdKey)
					{
						*item=2;
						return(True);
						break;
					}
					/* fall through */
				default:
					if (GetDialogKeyboardFocusItem(dgPtr) == adPassword)
					{
						if (**PwChars < 255)
						{
							PCatC(*PwChars,key);
							event->message = ((event->message >> 8)<<8) | bulletChar;
						}
						else
						{
							SysBeep(20);
							event->what = nullEvent;
						}
					}
					break;
			}
		}
		else if (event->what==updateEvt)
		{
			if (GetDialogFromWindow((WindowPtr) event->message) == dgPtr)
				HiliteButtonOne(dgPtr);
			else
				UpdateMyWindow((WindowPtr)event->message);
		}
		else
		{
			if (TickCount()%120<100 && CurrentModifiers()&alphaLock)
				ShowDialogItem(dgPtr,adCapsLockWarning);
			else
				HideDialogItem(dgPtr,adCapsLockWarning);
			EnableDItemIf(dgPtr,adOK,**PwChars);
		}
	}

	return(False);
}

/************************************************************************
 * GetACAPLogin - get login info from the user
 ************************************************************************/
OSErr GetACAPLogin(PStr server, PStr user, PStr password,Boolean giveQuit)
{
	extern OSErr ResetPassword(void);
	extern void CopyPassword(PStr);
	MyWindowPtr	dgPtrWin;
	DialogPtr dgPtr;
	short item;
	DECLARE_UPP(ACAPLoginFilter,ModalFilter);
	
	INIT_UPP(ACAPLoginFilter,ModalFilter);
	if (!MommyMommy(ATTENTION,nil)) return(userCanceledErr);
	if ((dgPtrWin = GetNewMyDialog(ACAP_LOGIN_DLOG,nil,nil,InFront))==nil)
	{
		WarnUser(GENERAL,MemError());
		return(userCanceledErr);
	}
	
	dgPtr = GetMyWindowDialogPtr (dgPtrWin);
	
	if (!(CurrentModifiers()&alphaLock)) HideDialogItem(dgPtr,adCapsLockWarning);

	SetDIText(dgPtr,adServer,server);
	SetDIText(dgPtr,adUser,user);
	SetDIText(dgPtr,adPassword,"");
	HideDialogItem(dgPtr,giveQuit ? adCancel : adQuit);
	if (!*server) item = adServer;
	else if (!*user) item = adUser;
	else item = adPassword;
	SelectDialogItemText(dgPtr,item,0,REAL_BIG);
	
	StartMovableModal(dgPtr);
	ShowWindow(dgPtr);
	HiliteButtonOne(dgPtr);
	PushCursor(arrowCursor);
	dgPtrWin->dialogRefcon = 'acap';
	
	do
	{
		if (ResetPassword()) item = adQuit;
		else
		{
			MovableModalDialog(dgPtr,ACAPLoginFilterUPP,&item);
			CopyPassword(password);
		}
	}
	while (item==adOK && !*password);
		
	PopCursor();
	if (item==adOK)
	{
		GetDIText(dgPtr,adServer,server);
		GetDIText(dgPtr,adUser,user);
	}
	else if (item==adQuit)
	{
		Cleanup();
		ExitToShell();
	}
	EndMovableModal(dgPtr);
	DisposDialog_(dgPtr);
	InBG = False; 
	return(item==adOK ? noErr : userCanceledErr);
}

/************************************************************************
 * ACAPGetServer - figure out what ACAP server to use
 ************************************************************************/
OSErr ACAPGetServer(PStr server)
{
	uLong addr, mask;
	Str127 remaining;
	OSErr err;
	
	if (*GetPref(server,PREF_ACAP_SERVER)) return(noErr);

	OpenProgress();
	ProgressMessageR(kpTitle,ACAP_SERVER_SEARCH);

	GetMyHostid((void*)&addr,(void*)&mask);
	ComposeString(remaining,"\p%I",addr);
	
	if ((err=ACAPGetServerLo(remaining,server)) && !CommandPeriod && gUseOT)
	{
		DNSHostid(&addr);
		ComposeString(remaining,"\p%I",addr);
		err = ACAPGetServerLo(remaining,server);
	}
	
	CloseProgress();
	
	return(err);
}

/************************************************************************
 * ACAPGetServerLo - look for the acap server, given a name to start from
 ************************************************************************/
OSErr ACAPGetServerLo(PStr remaining,PStr server)
{
	UPtr dot;
	Str127 hostname;
	
	while ((dot = PIndex(remaining,'.')) && !CommandPeriod)
	{
		if (*remaining - (dot-remaining) < 3) break;	// don't do queries like acap.co.nz
		GetRString(hostname,ACAPStrn+acapACAP);
		PCatC(hostname,'.');
		PCat(hostname,remaining);
		if (!ACAPTryServer(hostname))
		{
			PCopy(server,hostname);
			return(noErr);
		}
		*remaining -= dot-remaining;
		BMD(dot+1,remaining+1,*remaining);
	}
	return(fnfErr);
}

/************************************************************************
 * ACAPTryServer - see if there is an acap server here
 ************************************************************************/
OSErr ACAPTryServer(PStr server)
{
	TransStream stream=nil;
	OSErr err = NewTransStream(&stream);
	
	if (!err)
	{
		err = ConnectTrans(stream,server,GetRLong(ACAP_PORT),True,GetRLong(SHORT_OPEN_TIMEOUT));
		if (!err) DestroyTrans(stream);
		ZapTransStream(&stream);
	}
	return(err);
}

/************************************************************************
 * ACAPLogin - perform login on the ACAP server
 ************************************************************************/
OSErr ACAPLogin(PStr server, PStr user, PStr password, ACAPStateHandle state)
{
	TransStream stream=nil;
	OSErr err = NewTransStream(&stream);
	Str255 challenge;
	
	(*state)->stream = stream;
	if (!err) err = ConnectTrans(stream,server,GetRLong(ACAP_PORT),False,GetRLong(OPEN_TIMEOUT));

	if (!err)
	{
		err = ParseACAP(state);	// read banner
		if (!err)
		{
			ACAPBanner(state,challenge);
			if ((*state)->hasCram)
				if (*challenge)
					err = ACAPLoginWFunnyCram(user,password,state,challenge);
				else
					err = ACAPLoginWCram(user,password,state);
			else if ((*state)->hasAPOP && *challenge)
				err = ACAPLoginWAPOP(user,password,state,challenge);
			else if ((*state)->hasPlain)
				err = ACAPLoginPlain(user,password,state);
			else
				err = 1;
		}
	}
	
	if (err)
	{
		(*state)->stream = nil;
		if (stream)
		{
			DestroyTrans(stream);
			ZapTransStream(&stream);
		}
	}
	return(err);
}

/************************************************************************
 * ACAPLoginWFunnyCram - login with cram-md5, Randy's way
 ************************************************************************/
OSErr ACAPLoginWFunnyCram(PStr user, PStr password, ACAPStateHandle state,PStr challenge)
{
	Str255 string, scratch;
	long context = 0;
	OSErr err = fnfErr;
	short myTag = ++StupidTagForACAPandI4;
	
	GenKeyedDigest(challenge,password,string);
	ComposeRString(scratch,ACAPStrn+acapLoginParms,ACAPStrn+acapCram,user,string);
	err = ComposeRTrans((*state)->stream,ACAPStrn+acapCmd,
											myTag,
											ACAPStrn+acapAuth,
											scratch,
											NewLine);
	if (!err)
		while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)));
	
	if (!err && (*state)->response!=acapOK) err = 1;

	return(err);
}

/************************************************************************
 * ACAPLoginWCram - login with cram-md5
 ************************************************************************/
OSErr ACAPLoginWCram(PStr user, PStr password, ACAPStateHandle state)
{
	Str255 string, challenge;
	long context = 0;
	OSErr err = fnfErr;
	short myTag = ++StupidTagForACAPandI4;
	
	/*
	 * tell it we want cram-md5
	 */
	*string = 0;
	PCatC(string,'"');
	PCatR(string,ACAPStrn+acapCram);
	PCatC(string,'"');
	err = ComposeRTrans((*state)->stream,ACAPStrn+acapCmd,
											myTag,
											ACAPStrn+acapAuth,
											string,
											NewLine);
	
	/*
	 * wait for the challenge
	 */
	if (!err) while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)))
	{
		if ((*state)->tag==-1)
		{
			// got the challenge!
			PCopy(challenge,(*state)->responseValue);
			if (*challenge>5 && challenge[1]=='<')
			{
				// got a banner
				GenKeyedDigest(challenge,password,string);
				err = ComposeRTrans((*state)->stream,ACAPStrn+acapCramMore,user,string,NewLine);
				if (!err) while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)));
			}
			else err = fnfErr;
			break;
		}
	}
	if (!err && (*state)->response!=acapOK) err = 1;
	return(err);
}

/************************************************************************
 * ACAPLoginWAPOP - login with apop
 ************************************************************************/
OSErr ACAPLoginWAPOP(PStr user, PStr password, ACAPStateHandle state,PStr challenge)
{
	Str255 string, scratch;
	long context = 0;
	Accumulator a = (*state)->text;
	OSErr err = fnfErr;
	short myTag = ++StupidTagForACAPandI4;
	
	GenDigest(challenge,password,string);
	ComposeRString(scratch,ACAPStrn+acapLoginParms,ACAPStrn+acapAPOP,user,string);
	err = ComposeRTrans((*state)->stream,ACAPStrn+acapCmd,
											myTag,
											ACAPStrn+acapAuth,
											scratch,
												NewLine);
	if (!err)
		while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)));
	
	if (!err && (*state)->response!=acapOK) err = 1;

	return(err);
}

/************************************************************************
 * ACAPLoginPlain - plain login
 ************************************************************************/
OSErr ACAPLoginPlain(PStr user, PStr password, ACAPStateHandle state)
{
	Str255 scratch;
	OSErr err;
	short myTag = ++StupidTagForACAPandI4;
	
	ComposeRString(scratch,ACAPStrn+acapLoginParms,ACAPStrn+acapPlain,user,password);
	err = ComposeRTrans((*state)->stream,ACAPStrn+acapCmd,
											myTag,
											ACAPStrn+acapAuth,
											scratch,
											NewLine);
	if (!err)
		while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)));
	
	if (!err && (*state)->response!=acapOK) err = 1;

	return(err);
}

/************************************************************************
 * ACAPLoadLo - load settings from a stream
 ************************************************************************/
OSErr ACAPLoadLo(ACAPStateHandle state)
{
	OSErr err;
	Str255 scratch;
	short myTag = ++StupidTagForACAPandI4;
	
	err = ComposeRTrans((*state)->stream,ACAPStrn+acapCmd,
											myTag,
											ACAPStrn+acapSearch,
											GetRString(scratch,ACAPStrn+acapSearchMumboJumbo),
											NewLine);
	while (!(err=ParseACAP(state)) && ((*state)->tag!=myTag || !ACAPDone((*state)->response)))
	{
		if ((*state)->tag==myTag && (*state)->response==acapEntry)
			ACAPReadAndSet(state);
	}
	
	return(err);
}

/************************************************************************
 * ACAPReadAndSet - read acap response and set prefs from it
 ************************************************************************/
OSErr ACAPReadAndSet(ACAPStateHandle state)
{
	Str255 name;
	Str255 value;
	PStr which=name;
	Accumulator a = (*state)->text;
	long context = 0;
	Handle acapRes = GetResource('EuAC',128);
	OSErr err = noErr;
	short paren = 0;
	
	if (!acapRes) return(fnfErr);
	else HNoPurge(acapRes);
	
	NextAccuString(which,&a,&context);	// clear the entry name
	
	while (NextAccuString(which,&a,&context))
	{
		if (EqualStrRes(which,ACAPStrn+acapOpen))
		{
			which = name;
			paren++;
		}
		else if (EqualStrRes(which,ACAPStrn+acapClose))
		{
			if (paren) paren--;
			which = name;
		}
		else if (paren==1 || paren==2)
		{
			if (which==name) which = value;
			else if (which==value)	// got one!
			{
				ACAPSetPrefRes(name,value,acapRes);
				which = name;
			}
		}
	}
	return(err);
}
	
/************************************************************************
 * ACAPSetPrefRes - set a pref from the acap stream
 ************************************************************************/
OSErr ACAPSetPrefRes(PStr name,PStr value,Handle acapRes)
{
	ACAPResTemplate template;
	UPtr spot,end;
	long bytes;
	
	spot = LDRef(acapRes);
	end = spot + GetHandleSize(acapRes);
	
	for (;spot<end;spot += 9+spot[8])
	{
		if (StringSame(name,spot+8))
		{
			BMD(spot,&template,9+spot[8]);
			
			if (value[value[0]] == ' ')		// remove spurious trailing space
				value[0]--;
			if (template.type == ACAPBoolean)
			{
					if (*value && value[1]=='1')
						SetPref(template.pref,YesStr);
					else
						SetPref(template.pref,NoStr);
			}
			else if (template.type == ACAPRevBoolean)
			{
				if (*value && value[1] == '1')
					SetPref(template.pref,NoStr);
				else
					SetPref(template.pref,YesStr);
				break;
			}
			else if (template.type == ACAPWeKeepK)
			{
				StringToNum(value,&bytes);
				SetPrefLong(template.pref,bytes/(1 K));
			}
			else
			{
				switch(template.pref)
				{
					case PREF_INTERVAL:
						if (value[1] == '0')
							SetPref(PREF_AUTO_CHECK,NoStr);
						else
							SetPref(PREF_AUTO_CHECK,YesStr);
						SetPref(PREF_INTERVAL,value);
						break;
						
					case PREF_POP_MODE:
						if (StringSame(value,"\pNormal"))
							SetPrefLong(PREF_POP_MODE,1);
						else if (StringSame(value,"\pSTATUS"))
							SetPrefLong(PREF_POP_MODE,2);
						else if (StringSame(value,"\pPOP Last"))
							SetPrefLong(PREF_POP_MODE,3);
						break;
						
					case DUMMY_POP_AUTHENTICATE:
						SetPref(PREF_DONT_PASS,YesStr);
						SetPref(PREF_KERBEROS,NoStr);
						SetPref(PREF_APOP,NoStr);
						switch (FindSTRNIndex(ACAPExtraStrn,value))
						{
							case acapsKerberos: SetPref(PREF_KERBEROS,YesStr); break;
							case acapsAPOP:			SetPref(PREF_APOP,YesStr); break;
							default:
							case acapsPassword: SetPref(PREF_DONT_PASS,NoStr); break;
						}
						break;
					
					case PREF_FINGER:
						if (FindSTRNIndex(ACAPExtraStrn,value)==acapsFinger)
							SetPref(PREF_FINGER,YesStr);
						else
							SetPref(PREF_FINGER,NoStr);
						break;
						
					case DUMMY_SEND_FORMAT:
						SetPref(PREF_SINGLE,NoStr);
						SetPref(PREF_DOUBLE,NoStr);
						SetPref(PREF_BINHEX,NoStr);
						SetPref(PREF_UUENCODE,NoStr);
						switch (FindSTRNIndex(ACAPExtraStrn,value))
						{
							case acapsSingle: SetPref(PREF_SINGLE,YesStr); break;
							case acapsBinHex: SetPref(PREF_BINHEX,YesStr); break;
							case acapsUuencode: SetPref(PREF_UUENCODE,YesStr); break;
							default:
							case acapsDouble: SetPref(PREF_DOUBLE,YesStr); break;
						}
						break;
					case BIG_MESSAGE:
						if (value[value[0]] == 'K')
							value[0]--;
						if (value[1] == '0')
							SetPref(PREF_NO_BIGGIES,NoStr);
						else
							SetPref(PREF_NO_BIGGIES,YesStr);
						SetPref(BIG_MESSAGE,value);
					break;
				default:
					SetPref(template.pref,value);
					break;
			}
		}
	}
	}
	return noErr;	/* really should check the values of all those SetPrefs, but .... */
}

/************************************************************************
 * ParseACAP - interpret the acap data stream.  Currently does not know what
 * commands have been issued; unacceptable in the long run, but good
 * enough for now.  Eventually, we will probably want to register handlers
 * for each tag, and this routine will call the handlers.
 * Returns after every line of data
 ************************************************************************/
OSErr ParseACAP(ACAPStateHandle state)
{
	OSErr err;
	Str255 scratch;
	ACAPEnum response;
	long val;
	UPtr spot;
	Accumulator text = (*state)->text;
	
	// housecleaning
	(*state)->response = (*state)->tag = -1;
	text.offset = 0;
	
	/*
	 * read tokens so long as there is no error
	 */
	while (!(err=ACAPGetToken(state)))
	{
		switch((*state)->state)
		{
			/* 
			 * tag - read its value
			 */
			case acsTag:
				if ((*state)->token==acaptString)
				{
					if ((*state)->tokenValue[0]==1 && (*state)->tokenValue[1]=='+')
						(*state)->tag = -1;
					else
					{
						PSCopy(scratch,(*state)->tokenValue);
						*scratch = MIN(*scratch,sizeof(scratch)-2);
						scratch[*scratch+1] = 0;
						for (spot=scratch+1;*spot && !isdigit(*spot);spot++);
						val = atoi(spot);
						(*state)->tag = val;
					}
					(*state)->state = acsResponse;
				}
				else (*state)->state = acsFlush;
				break;
				
			
			/*
			 * response; look it up in our table and give it a numeric value
			 */
			case acsResponse:
				if ((*state)->token==acaptString)
				{
					PSCopy(scratch,(*state)->tokenValue);
					PSCopy((*state)->responseValue,(*state)->tokenValue);
					response = FindSTRNIndex(ACAPStrn,scratch);
					(*state)->response = response;
					(*state)->state = acsText;
				}
				else (*state)->state = acsFlush;
				break;
			
			/*
			 * read text value
			 */
			case acsText:
				switch((*state)->token)
				{
					case acaptOpenParen:
						GetRString(scratch,ACAPStrn+acapOpen);
						break;
					case acaptCloseParen:
						GetRString(scratch,ACAPStrn+acapClose);
						break;
					case acaptString:
						PCopy(scratch,(*state)->tokenValue);
						break;
					default:
						(*state)->state = acsFlush;
						break;
				}
				if ((*state)->state != acsFlush)
					AccuAddPtr(&text,scratch,*scratch+1);
				break;
			
			/*
			 * we don't like what we've seen; dump the rest of the response
			 */
			case acsFlush:
				break; // keep flushing
		}
		
		/*
		 * bail out after every line
		 */
		if ((*state)->token==acaptNewline)
		{
			(*state)->state = acsTag;
			break;
		}
	}
	
	(*state)->text = text; // copy it back
	
	return(err);
}

/************************************************************************
 * ACAPGetToken - tokenize the acap stream
 ************************************************************************/
OSErr ACAPGetToken(ACAPStateHandle state)
{
	ACAPLexState ls = alsFree;
	OSErr err=noErr;
	Byte c;
	Str255 token;
	Boolean white;
	Boolean done = False;
	long count = 0;
	
	*token = 0;
	
	while (!done)
	{
		// fill buffer if need be
		if ((*state)->spot==(*state)->end && (err = ACAPFill(state))) break;
		c = (*(*state)->buffer)[(*state)->spot++];
		white = IsWhite(c);
		
		switch (ls)
		{
			case alsFree:
				switch (c)
				{
					case '"': ls = alsQuote; break;
					case '\015': ls = alsCR; break;
					case '{': ls = alsDigit; break;
					case '(':
					case ')': PCatC(token,c); done = True; break;
					default:
						if (!white)
						{
							ls = alsAtom;
							PCatC(token,c);
						}
						break;
				}
				break;
			
			case alsAtom:
				if (white || c=='"' || c=='\015' || c=='{' || c=='(' || c==')')
				{
					done = True;
					(*state)->spot--;
				}
				else
					PCatC(token,c);
				break;
			
			case alsQuote:
				if (c=='\\') ls = alsEscapedQuote;
				else if (c=='"') done = True;
				else if (c=='\015')
				{
					done = True;
					(*state)->spot--;
				}
				else PCatC(token,c);
				break;
			
			case alsEscapedQuote:
				if (c=='\015')
				{
					done = True;
					(*state)->spot--;
				}
				else
					PCatC(token,c);
				ls = alsQuote;
				break;
				
			case alsDigit:
				if (isdigit(c)) count = 10*count + (c-'0');
				else ls = alsCloseBrace;
				break;
			
			case alsCloseBrace:
				ls = alsBraceCR;
				break;
			
			case alsBraceCR:
				ls = alsLiteral;
				break;
			
			case alsLiteral:
				if (!count)
				{
					(*state)->spot--;
					done = True;
				}
				else
				{
					count--;
					PCatC(token,c);
				}
				break;
			
			case alsCR:
				if (c=='\012')
				{
					ls = alsLF;
					done = True;
				}
				break;
		}
	}
	
	if (!err)
	{
		// WARNING -- this stuff is UTF-8, and we're not dealing with that
		PCopy((*state)->tokenValue,token);
		switch (ls)
		{
			case alsLiteral:
			case alsEscapedQuote:
			case alsQuote:
			case alsAtom: (*state)->token = acaptString; break;
			case alsLF: (*state)->token = acaptNewline; break;
			case alsFree: (*state)->token = c=='(' ? acaptOpenParen : acaptCloseParen; break;
			default: (*state)->token = acaptLimit; break;
		}
	}
	
	return(err);
}

/************************************************************************
 * ACAPFill - fill the buffer
 ************************************************************************/
OSErr ACAPFill(ACAPStateHandle state)
{
	long len = GetHandleSize((*state)->buffer);
	OSErr err;
	
	if (err=RecvTrans((*state)->stream,LDRef((*state)->buffer),&len)) 	/* get some chars */
		len=0;
	if (len)
	{
		UTF8To88591(*(*state)->buffer,len,*(*state)->buffer,&len);
		TransLitRes(*(*state)->buffer,len,ktISOMac);
	}
	(*state)->spot = 0;
	(*state)->end = len;
	UL((*state)->buffer);
	return(err);
}

/************************************************************************
 * ACAPBanner - parse the banner
 ************************************************************************/
OSErr ACAPBanner(ACAPStateHandle state,PStr challenge)
{
	Accumulator a = (*state)->text;
	Str255 s;
	long context = 0;
	short bannerState = 0;
	
	*challenge = 0;
	
	if ((*state)->response==acapBye) return(fnfErr);
	while (NextAccuString(s,&a,&context))
	{
		switch(FindSTRNIndex(ACAPStrn,s))
		{
			case acapPlain: if (bannerState == acapSASL) (*state)->hasPlain = True; break;
			case acapAPOP: if (bannerState == acapSASL) (*state)->hasAPOP = True; break;
			case acapCram: if (bannerState == acapSASL) (*state)->hasCram = True; break;
			case acapXCram:	if (bannerState == 0) bannerState = acapCram; break;
			case acapOpen: bannerState = 0; break;
			case acapClose: bannerState = -1; break;
			case acapSASL: if (bannerState == 0) bannerState = acapSASL; break;
			default:
				if (bannerState == acapCram) PCopy(challenge,s);
				else if (bannerState != acapSASL) bannerState = -1;
				break;
		}
	}
	return(noErr);
}

/************************************************************************
 * NewACAPState - a new acap state
 ************************************************************************/
ACAPStateHandle NewACAPState(void)
{
	ACAPStateHandle state = NewZH(ACAPState);
	UHandle buffer;
	Accumulator accu;
	OSErr err;
	
	if (state)
	{
		buffer = NewIOBHandle(255,OPTIMAL_BUFFER);
		(*state)->buffer = buffer;
		if (!(err = AccuInit(&accu))) (*state)->text = accu;
		if (err || !buffer)
		{
			DisposeACAPState(state);
			state = nil;
		}
	}
	return(state);
}

/************************************************************************
 * DisposeACAPState - destroy one
 ************************************************************************/
void DisposeACAPState(ACAPStateHandle state)
{
	TransStream stream;
	
	if (state)
	{
		ZapHandle((*state)->buffer);
		AccuZap((*state)->text);
		if (stream = (*state)->stream)
		{
			DestroyTrans(stream);
			ZapTransStream(&stream);
		}
	}
	ZapHandle(state);
}

/************************************************************************
 * NextAccuString - read a string from an accumulator
 ************************************************************************/
PStr NextAccuString(PStr string,AccuPtr a,long *context)
{
	*string = 0;
	if (*context>=a->offset) return(nil);
	PCopy(string,(*a->data)+*context);
	*context += *string+1;
	return(string);
}