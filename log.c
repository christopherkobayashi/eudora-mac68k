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

#include "log.h"
#define FILE_NUM 50
/* Copyright (c) 1992 by Qualcomm, Inc */
/**********************************************************************
 * the log file
 **********************************************************************/

#pragma segment Log

void OpenLog(void);

/************************************************************************
 * ComposeLogR - compose a log file entry, using a resource for a format
 ************************************************************************/
UPtr ComposeLogR(uLong level,UPtr into,short format,...)
{
	Str255 locl;
	UPtr reallyInto = into ? into : locl;
	va_list args;
	va_start(args,format);
	(void) VaComposeRString(reallyInto,format,args);
	va_end(args);
	Log(level,reallyInto);
	return(into);
}

/************************************************************************
 * ComposeLogS - compose a log file entry, using a string for a format
 ************************************************************************/
UPtr ComposeLogS(uLong level,UPtr into,UPtr format,...)
{
	Str255 locl;
	UPtr reallyInto = into ? into : locl;
	va_list args;
	va_start(args,format);
	(void) VaComposeString(reallyInto,format,args);
	va_end(args);
	Log(level,reallyInto);
	return(into);
}

/************************************************************************
 * Log - log a string, if the current log level includes the current string
 ************************************************************************/
UPtr Log(uLong level,UPtr string)
{
#ifdef THREADING_ON
	MyThreadBeginCritical();
#endif // THREADING_ON
	if (level==0xFFFFFFFF || (level&LogLevel)!=0)
	{
		Str31 stamp;
		long tickDiff = TickCount()-LogTicks;
		Str255 threadStr;

		snprintf(threadStr, 6, "\pMAIN");
		
		// so now the threadID is the first field of each log entry
		if (InAThread ())
		{
			ThreadID threadId;

			GetCurrentThread(&threadId);
			NumToString(threadId,threadStr);
		}
		ComposeString(stamp,"\p %d:%d.%d.%d ",level,
									tickDiff/3600,(tickDiff/60)%60,tickDiff%60);
									
		{
			FSSpec spec;
			Str31 scratch;
			MyWindowPtr win;
	
			SimpleMakeFSSpec(Root.vRef,Root.dirId,GetRString(scratch,LOG_NAME),&spec);
			
			if (win=FindText(&spec))
			{
				PeteAppendText(stamp+1,*stamp,win->pte);
				PeteAppendText(string+1,*string,win->pte);
				if (string[*string]!='\015') PeteAppendText("\015",1,win->pte);
				PeteScroll(win->pte,0,32767);
#ifdef THREADING_ON
	MyThreadEndCritical();
#endif // THREADING_ON
				return(string);
			}
		}

		OpenLog();
		if (LogRefN)
		{
			FSWriteP(LogRefN,threadStr);
			FSWriteP(LogRefN,stamp);
			FSWriteP(LogRefN,string);
			if (string[*string] != '\015') FSWriteP(LogRefN,Cr);
		}
	}
#ifdef THREADING_ON
	MyThreadEndCritical();
#endif // THREADING_ON
	return(string);
}

/************************************************************************
 * CloseLog - close, rolling over if needed
 ************************************************************************/
void CloseLog(void)
{
	long eof=0;

	if (LogRefN)
	{
	  eof = (GetEOF(LogRefN,&eof) || eof > GetRLong(LOG_ROLLOVER) K);
		MyFSClose(LogRefN);
		LogRefN = 0;
	}
	if (eof)
	{
		Str31 old,new;
		GetRString(old,OLD_LOG);
		GetRString(new,LOG_NAME);
		HDelete(Root.vRef,Root.dirId,old);
		HRename(Root.vRef,Root.dirId,new,old);
	}
}


/************************************************************************
 * OpenLog - open log file if needed, rolling over if needed
 ************************************************************************/
void OpenLog(void)
{
	long eof;
	Str255 str;
	Str15 ctext;
	short err;
	long creator;
	static int roll;
	
	if (LogRefN && (GetEOF(LogRefN,&eof) || eof > roll)) CloseLog();
	if (!LogRefN)
	{
		roll = GetRLong(LOG_ROLLOVER) K;
		GetRString(str,LOG_NAME);
		if ((err=AFSHOpen(str,Root.vRef,Root.dirId,&LogRefN,fsRdWrPerm))==fnfErr)
		{
			GetPref(ctext,PREF_CREATOR);
			if (*ctext!=4) GetRString(ctext,TEXT_CREATOR);
			BMD(ctext+1,&creator,4);
			HCreate(Root.vRef,Root.dirId,str,creator,'TEXT');
			err = AFSHOpen(str,Root.vRef,Root.dirId,&LogRefN,fsRdWrPerm);
	  }
		if (err) return;
		SetFPos(LogRefN,fsFromLEOF,0);
		LogTicks = TickCount();
		LocalDateTimeStr(str+1);
		str[0] = str[1]+1;
		str[1] = '\r';
		FSWriteP(LogRefN,str);
	}
}

/************************************************************************
 * 
 ************************************************************************/
void LogAlert(short template)
{
	ComposeLogS(LOG_ALRT,nil,"\pALRT %d",template);
}

/************************************************************************
 * 
 ************************************************************************/
void MyParamText(PStr p1,PStr p2,PStr p3,PStr p4)
{
	P1[0]=0;
	P2[0]=0;
	P3[0]=0;
	P4[0]=0;
	if (p1&&*p1)	{PCopy(P1, p1); Log(LOG_ALRT,p1);}
	if (p2&&*p2) 	{PCopy(P2, p2); Log(LOG_ALRT,p2);}
	if (p3&&*p3) 	{PCopy(P3, p3); Log(LOG_ALRT,p3);}
	if (p4&&*p4) 	{PCopy(P4, p4); Log(LOG_ALRT,p4);}
	ParamText(p1,p2,p3,p4);
}

/************************************************************************
 * CarefulLog - log a potentially long and nasty string
 ************************************************************************/
void CarefulLog(uLong level,short format,UPtr data,short dSize)
{
	Byte logString[120];
	UPtr to, dataEnd, logEnd;
	
	if (!(level&LogLevel)) return;
	
	dataEnd = data+dSize;
	do
	{
		to = logString+1;
		logEnd = to + sizeof(logString)-6;
		while (data<dataEnd)
		{
			if (*data < ' ')
			{
				*to++ = '\\';
				switch (*data)
				{
					case '\012': *to++ = 'n'; break;
					case '\015': *to++ = 'r'; break;
					case '\t': *to++ = 't'; break;
					default:
						*to++ = '0'+(*data/64)%8;
						*to++ = '0'+(*data/8)%8;
						*to++ = '0'+*data % 8;
						break;
				}
			}
			else
				*to++ = *data;
			data++;
			if (to>logEnd || data[-1]=='\012') break;
		}
		*logString = to-logString-1;
		ComposeLogR(level,nil,format,logString);
	}
	while (data<dataEnd);
}

/************************************************************************
 * LineLog - log data by lines
 ************************************************************************/
void LineLog(uLong level,short format,UPtr data,short dSize)
{
	Byte logString[120];
	UPtr to, dataEnd, logEnd;
	
	if (!(level&LogLevel)) return;
	
	dataEnd = data+dSize;
	do
	{
		to = logString+1;
		logEnd = to + sizeof(logString)-6;
		while (data<dataEnd)
		{
			if (*data < ' ')
			{
				*to++ = '\\';
				switch (*data)
				{
					case '\012': *to++ = 'n'; break;
					case '\015': to--; data++; goto output;	// quit when we hit return
					case '\t': to[-1] = '\t'; break;	// put tab in verbatim
					default:
						*to++ = '0'+(*data/64)%8;
						*to++ = '0'+(*data/8)%8;
						*to++ = '0'+*data % 8;
						break;
				}
			}
			else
				*to++ = *data;
			data++;
			if (to>logEnd || data[-1]=='\012') break;
		}
		output:
		*logString = to-logString-1;
		ComposeLogR(level,nil,format,logString);
	}
	while (data<dataEnd);
}

/************************************************************************
 * HexLog - log some binary data in hex
 ************************************************************************/
void HexLog(uLong level,short format,UPtr data,short dSize)
{
	Byte logString[128];
	UPtr to, dataEnd;
	
	if (!(level&LogLevel)) return;
	
	dataEnd = data+dSize;
	do
	{
		to = MIN(data+32,dataEnd);
		Bytes2Hex(data,to-data,logString+1);
		*logString = 2*(to-data);
		ComposeLogR(level,nil,format,logString);
		data = to;
	}
	while (data<dataEnd);
}
