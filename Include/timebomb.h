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

//
//      JDB DEMO        960628
//
//      Added this file and altered it slightly to incorporate the TimeBomb demo feature
//

#ifndef TIMEBOMB_H
#define TIMEBOMB_H

// Obscure values other than just TRUE/FALSE for the valid test
#define TB_DATE_TOO_YOUNG		327
#define TB_DATE_TOO_OLD			412
#define TB_DATE_JUST_RIGHT		512
#define TB_DISARMED				31667

#define NUMBER_OF_SECS_DAY		86400

#define	BUY_NOW_BUTTON_EXPIRED			6
#define	BUY_NOW_BUTTON_WILL_EXPIRE	6

#define	SECRET_FILE						"\pScrapbook File"
#define	SECRET_FILE_LEN				15
#define	ALT_SECRET_FILE				"\pQC Application Registry"
#define	ALT_SECRET_FILE_LEN		24
#define	ALT_SECRET_FILE_TYPE	//Some sort of file type?

typedef struct TIME_BOMB_DATA {
	OSType creator;
	unsigned short version;
	unsigned short days;
	unsigned long date;
} TimeBombDataRecord;

typedef TimeBombDataRecord *TimeBombDataPtr;
typedef TimeBombDataRecord **TimeBombDataHandle;

extern TimeBombDataRecord gTimeBombData;
extern Boolean gTimeStampValid;
extern long today;

#ifdef __cplusplus
extern "C" {
#endif
	OSErr ClearTimeStamp(void);
	OSErr GetTimeStampData(void);
	OSErr RemoveTimeStampData(void);
	OSErr SetTimeStampData(void);
	unsigned short TimeStampValid(unsigned long currentTime,
				      unsigned short graceDays);
	void DeScrambleTimeBombData(void);
	void ScrambleTimeBombData(void);

	Boolean DemoExpired(void);
	void TimeBomb(void);
#ifdef __cplusplus
};
#endif

#endif
