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

//
//	DEMO		JDB	June 28, 1996
//
//	Added this file and altered it slightly to incorporate the TimeBomb demo Feature.
//	All related Eudora code changes are commented or #ifdef'ed with DEMO
//

//-------------------------------------------------------------------------
// timebomb.c	(was Timebomb.c)
// Source for implementing a timebomb function on the Macintosh
//
// Usage:
// Call GetTimeStampData.  This will set up the global with the relevant data.
// Call timeStampValid it a value from GetDateTime or LMGetTime.  To help thwart
// hackers, put a little separation between the getting of the date and the check.
// If TimeStampValid does not pass, send a message or apple event back to the app
// to decouple the cause and effect.
//
//	To make things a little harder for hackers, let's introduce another global
//	called today.  We will set today to the current date every time we call "SetYesterday".
//	Note, Yesterday is the number of seconds since the beginning of the current day.
//	We make a call to get the current time there, so that's why I choose to set today there.
//-------------------------------------------------------------------------
#include "timebomb.h"

#ifdef	DEMO

void	QuitExpiredEudora(void);				//quit Eudora after it has expired.

TimeBombDataRecord	gTimeBombData;
long								today;

//-------------------------------------------------------------------------
//	TimeBomb
//
//	This routine installs the timebomb resource in the right place.  It should
//	only be called once per program execution.  
//
//	the time bomb resource should only be installed if it doesn't already exist.
//
//	The initial call to GetTimeStampData will see if there is a timebomb resource
//	already installed.  
//-------------------------------------------------------------------------
void TimeBomb(void)
{
	OSErr	timeBombErr = noErr;
	
	gTimeBombData.creator = 'CSOm';
	gTimeBombData.version = (MAJOR_VERSION<<8)|MINOR_VERSION;
	gTimeBombData.days = 0;
	gTimeBombData.date = 0;
	
	timeBombErr = GetTimeStampData();
	
	if ((timeBombErr == resNotFound) || (timeBombErr == fnfErr) || (gTimeBombData.date == 0))
	{
		//the resource was not found, or the date was bad.  Install a new one.
		gTimeBombData.creator = 'CSOm';
		gTimeBombData.version = (MAJOR_VERSION<<8)|MINOR_VERSION;
		gTimeBombData.date = today;
		gTimeBombData.days = DEMO;
	
		SetTimeStampData();
	}
}


//-------------------------------------------------------------------------
//	DemoExpired
//
//	This routine is called every time we wish to make sure this demo version as not yet
//	expired.
//
//	Depending on the code returned by TimeStampValid, the demo may have expired.
//	If the current date is past the demo expiration date, or if the current date
//	is _before_ the date the demo was first run, then the demo has expired.
//-------------------------------------------------------------------------

Boolean	DemoExpired(void)
{
	unsigned short 	validCode;
	Boolean					retVal = false;
	
	//Get the time stamp data ...
	gTimeBombData.creator = 'CSOm';
	gTimeBombData.version = (MAJOR_VERSION<<8)|MINOR_VERSION;
	gTimeBombData.days = 0;
	gTimeBombData.date = 0;
	
	GetTimeStampData();
	
	validCode = TimeStampValid(today, 0);

	switch(validCode)
	{
		case TB_DATE_TOO_YOUNG:			//Someone's been screwing with the clock!
		case TB_DATE_TOO_OLD:				//Demo versions expire on this machine at this time.
			QuitExpiredEudora();		//Demo Expired Alert
			//DecayIntoLight();
			retVal = true;
			break;
		
		case TB_DATE_JUST_RIGHT:		//This demo has not yet expired
		case TB_DISARMED:						//Someone registered a demo on this machine.
		default:
			break;
	}
	
	return (retVal);
}


//-------------------------------------------------------------------------
// QuitExpiredEudora			
//
//	Displays an alert explaining the demo has expired.  A second button
//	in the alert connects a browser to the eudora order page from the aleart.
//	When the alert is dismissed, Eudora quits.
//
//-------------------------------------------------------------------------
void QuitExpiredEudora(void)
{	
	//Make sure the arrow cursor is displayed
	SetMyCursor(arrowCursor);
	
	//If the user pressed "Buy Now", open up the eudora link
	if (Alert(EXPIRE_ALRT,nil) == BUY_NOW_BUTTON_EXPIRED)
		PurchaseEudora();
		
	EjectBuckaroo = true;
}

//-------------------------------------------------------------------------
// Sets up the gTimeBombData record.
//
//	Notes:		the global record must have a creator and version set
//				to find the resource.
//
//				This routine first searches for the Alternate secret file (i.e.,
//				the invisible file Eudora creates).  If not found, it searches for
//				the Scrapbook file, the secret file of choice.
//
//	Returns:	fnfErr if the secret file is not found
//				resNotFound if the time stamp is not found in the sb
//-------------------------------------------------------------------------
OSErr GetTimeStampData()
{
	long				foundDirID;
	short				foundVRefNum;
	short 				refNum;
	OSErr				err;
	FSSpec				secretFile;
	TimeBombDataHandle	timeBombData = 0;
	short oldResF = CurResFile();
	
	err = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder, &foundVRefNum, &foundDirID);
		
	if (!err)
	{
		secretFile.vRefNum = foundVRefNum;
		secretFile.parID = foundDirID;
		BlockMove(ALT_SECRET_FILE, secretFile.name, ALT_SECRET_FILE_LEN);
		
		SetResLoad(FALSE);
		refNum = FSpOpenResFile(&secretFile, fsRdPerm);
		SetResLoad(TRUE);
		
		if (refNum < 0)		//alternate secret file not found, try locating the first choice
		{
			BlockMove(SECRET_FILE, secretFile.name, SECRET_FILE_LEN);
			SetResLoad(FALSE);
			refNum = FSpOpenResFile(&secretFile, fsRdPerm);
			SetResLoad(TRUE);
		}
		
		if (0 < refNum)
		{
			timeBombData = (TimeBombDataHandle) Get1Resource(gTimeBombData.creator, gTimeBombData.version);
			if (timeBombData)
			{
				gTimeBombData = **timeBombData;
				ReleaseResource((Handle) timeBombData);
				DeScrambleTimeBombData();	
			}
			else
			{
				gTimeBombData.date = 0;
				gTimeBombData.days = 0;
			}
			CloseResFile(refNum);
		}
		else				//the file we want to store our timestamp was not found. 
			err = fnfErr;
	}
	UseResFile (oldResF);
	return err;
}

//-------------------------------------------------------------------------
// Setup the data structure gTimeBombData before calling this.  Fill in all
//	the fields of the global record.
//-------------------------------------------------------------------------
OSErr SetTimeStampData()
{
	OSType				creator;
	long				foundDirID;
	short				foundVRefNum;
	short 				refNum;
	short				currentResFile;
	short				version;
	OSErr				err;
	FSSpec				secretFile;
	TimeBombDataHandle	timeBombData = 0;
	HParamBlockRec		secretModDate;
	
	err = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder, &foundVRefNum, &foundDirID);
		
	if (!err)
	{
		secretFile.vRefNum = foundVRefNum;
		secretFile.parID = foundDirID;
		BlockMove(SECRET_FILE, secretFile.name, SECRET_FILE_LEN);
		
		//Locate the secret file and save it's modification date
		secretModDate.ioParam.ioCompletion = NULL;
		secretModDate.ioParam.ioNamePtr = secretFile.name;
		secretModDate.ioParam.ioVRefNum = foundVRefNum;
		secretModDate.fileParam.ioFDirIndex = 0;
		secretModDate.fileParam.ioDirID = foundDirID;
		err = PBHGetFInfoSync(&secretModDate);
		
		if (err != noErr) 		//scrapbook file wasn't found, search for the alternate
		{
			BlockMove(ALT_SECRET_FILE, secretFile.name, ALT_SECRET_FILE_LEN);
			secretModDate.ioParam.ioNamePtr = secretFile.name;
			secretModDate.fileParam.ioDirID = foundDirID;
			secretModDate.ioParam.ioVRefNum = foundVRefNum;
			secretModDate.fileParam.ioFDirIndex = 0;
			err = PBHGetFInfoSync(&secretModDate);
			
			if (err != noErr)	//alternate wasn't found.  Create it.
			{
				FSpCreateResFile(&secretFile,'CSOm','TEXT',smSystemScript);
							
				//Alter the mod and creation date for the newly created file
				secretModDate.ioParam.ioNamePtr = secretFile.name;
				secretModDate.fileParam.ioDirID = foundDirID;
				secretModDate.ioParam.ioVRefNum = foundVRefNum;
				secretModDate.fileParam.ioFDirIndex = 0;
				err = PBHGetFInfoSync(&secretModDate);
				
				secretModDate.fileParam.ioFlCrDat -= (NUMBER_OF_SECS_DAY * 3375)/100;
				secretModDate.fileParam.ioFlMdDat -= (NUMBER_OF_SECS_DAY * 3315)/100;
				secretModDate.fileParam.ioFlFndrInfo.fdFlags |= fInvisible;
			}
		}
		
		//Once we've located the secret file or created the alternate, open it. 
		if (err == noErr)		
		{
			currentResFile = CurResFile();
			SetResLoad(FALSE);
			refNum = FSpOpenResFile(&secretFile, fsRdWrShPerm);
			SetResLoad(TRUE);
		
			//Now save the timestamp data to the secret file
			if (0 < refNum)
			{
				creator = gTimeBombData.creator;
				version = gTimeBombData.version;
				ScrambleTimeBombData();
				UseResFile(refNum);
				
				timeBombData = (TimeBombDataHandle) Get1Resource(creator, version);
				if (!timeBombData)
				{
					timeBombData = (TimeBombDataHandle) NewHandle(sizeof(TimeBombDataRecord));
					**timeBombData = gTimeBombData;
					AddResource((Handle) timeBombData, creator, version, "\p");
				}
				
				UseResFile(currentResFile);
				CloseResFile(refNum);
				DeScrambleTimeBombData();	
			}
		
			// write the modification date back to the file, whatever file it may be ...
			secretModDate.ioParam.ioNamePtr = secretFile.name;
			secretModDate.ioParam.ioVRefNum = foundVRefNum;
			secretModDate.fileParam.ioFDirIndex = 0;
			secretModDate.fileParam.ioDirID = foundDirID;
			PBHSetFInfoSync(&secretModDate);
		}
	}
	return err;
}

#ifdef	NOT_USED

//-------------------------------------------------------------------------
// Call this to convert the demo to a full version.  TimeStampValid
//	will return valid now.  If the user tries to upgrade using a demo
//	version of a major upgrade, the demo will once again be invoked
//	forcing the user to pay for the upgrade.
//
//	Not used in Eudora 3.0 Demo
//-------------------------------------------------------------------------

OSErr ClearTimeStamp()
{
	OSType				creator;
	long				foundDirID;
	short				foundVRefNum;
	short 				refNum;
	short				currentResFile;
	short				version;
	OSErr				err;
	FSSpec				secretFile;
	TimeBombDataHandle	timeBombData;
	HParamBlockRec		secretModDate;
	
	err = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder, &foundVRefNum, &foundDirID);
		
	if (!err)
	{
		secretFile.vRefNum = foundVRefNum;
		secretFile.parID = foundDirID;
		BlockMove(SECRET_FILE, secretFile.name, SECRET_FILE_LEN);
		
		// Capture modification date
		secretModDate.ioParam.ioCompletion = NULL;
		secretModDate.ioParam.ioNamePtr = secretFile.name;
		secretModDate.ioParam.ioVRefNum = foundVRefNum;
		secretModDate.fileParam.ioFDirIndex = 0;
		secretModDate.fileParam.ioDirID = foundDirID;
		PBHGetFInfoSync(&secretModDate);
		
		// Open RF of scrapbook file
		currentResFile = CurResFile();
		SetResLoad(FALSE);
		refNum = FSpOpenResFile(&secretFile, fsRdWrShPerm);
		SetResLoad(TRUE);
		
		if (0 < refNum)
		{
			gTimeBombData.days = TB_DISARMED;
			
			creator = gTimeBombData.creator;
			version = gTimeBombData.version;
			ScrambleTimeBombData();
			UseResFile(refNum);
			
			timeBombData = (TimeBombDataHandle) GetResource(creator, version);
			
			// Add resource if it doesn't exist
			if (!timeBombData)
			{
				timeBombData = (TimeBombDataHandle) NewHandle(sizeof(TimeBombDataRecord));
				**timeBombData = gTimeBombData;
				AddResource((Handle) timeBombData, creator, version, "\p");
			}
			else
			{
				**timeBombData = gTimeBombData;
				ChangedResource((Handle) timeBombData);
			}
			
			UseResFile(currentResFile);
			CloseResFile(refNum);
			DeScrambleTimeBombData();	
		}
		
		// restore modification date
		secretModDate.ioParam.ioNamePtr = secretFile.name;
		secretModDate.ioParam.ioVRefNum = foundVRefNum;
		secretModDate.fileParam.ioFDirIndex = 0;
		secretModDate.fileParam.ioDirID = foundDirID;
		PBHSetFInfoSync(&secretModDate);
	}
	
	return err;
}

//-------------------------------------------------------------------------
// Call this to uninstall the time stamp
//
//	Not used in Eudora 3.0 Demo
//-------------------------------------------------------------------------
OSErr RemoveTimeStampData()
{
	long				foundDirID;
	short				foundVRefNum;
	short 				refNum;
	short				currentResFile;
	OSErr				err;
	FSSpec				secretFile;
	TimeBombDataHandle	timeBombData;
	HParamBlockRec		secretModDate;
	
	err = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder, &foundVRefNum, &foundDirID);
		
	if (!err)
	{
		secretFile.vRefNum = foundVRefNum;
		secretFile.parID = foundDirID;
		BlockMove(SECRET_FILE, secretFile.name, SECRET_FILE_LEN);
		
		// Capture modification date
		secretModDate.ioParam.ioCompletion = NULL;
		secretModDate.ioParam.ioNamePtr = secretFile.name;
		secretModDate.ioParam.ioVRefNum = foundVRefNum;
		secretModDate.fileParam.ioFDirIndex = 0;
		secretModDate.fileParam.ioDirID = foundDirID;
		PBHGetFInfoSync(&secretModDate);
		
		// Open RF of scrapbook file
		currentResFile = CurResFile();
		SetResLoad(FALSE);
		refNum = FSpOpenResFile(&secretFile, fsRdWrShPerm);
		SetResLoad(TRUE);
		
		if (0 < refNum)
		{
			UseResFile(refNum);
			
			timeBombData = (TimeBombDataHandle) GetResource(gTimeBombData.creator, gTimeBombData.version);
			
			// Delete resource if it exists
			if (timeBombData)
			{
				RemoveResource((Handle) timeBombData);
				DisposeHandle((Handle) timeBombData);
			}
			
			UseResFile(currentResFile);
			CloseResFile(refNum);
			
		}
		
		// restore modification date
		secretModDate.ioParam.ioNamePtr = secretFile.name;
		secretModDate.ioParam.ioVRefNum = foundVRefNum;
		secretModDate.fileParam.ioFDirIndex = 0;
		secretModDate.fileParam.ioDirID = foundDirID;
		PBHSetFInfoSync(&secretModDate);
	}
	
	return err;
}

#endif	//NOT_USED


//-------------------------------------------------------------------------
//	We return values instead of simple TRUE/FALSE.
//-------------------------------------------------------------------------
unsigned short TimeStampValid(unsigned long currentTime, unsigned short graceDays)
{	
	if (gTimeBombData.days == TB_DISARMED)
		return TB_DISARMED;
		
	if (currentTime < gTimeBombData.date)
		return TB_DATE_TOO_YOUNG; 
	
	if (currentTime > ((gTimeBombData.days + graceDays) * NUMBER_OF_SECS_DAY + gTimeBombData.date))
		return TB_DATE_TOO_OLD;
		
	return TB_DATE_JUST_RIGHT;
}

//-------------------------------------------------------------------------
// Scramble TimeBombData
//-------------------------------------------------------------------------
void ScrambleTimeBombData()
{
	gTimeBombData.creator ^= gTimeBombData.date;
	gTimeBombData.date ^= 0xFFFFFFFF;
	
	gTimeBombData.version ^= gTimeBombData.days;
	gTimeBombData.days ^= 0xFFFF;
}

//-------------------------------------------------------------------------
// Descramble TimeBombData
//-------------------------------------------------------------------------
void DeScrambleTimeBombData()
{	
	gTimeBombData.date ^= 0xFFFFFFFF;
	gTimeBombData.creator ^= gTimeBombData.date;
	
	gTimeBombData.days ^= 0xFFFF;
	gTimeBombData.version ^= gTimeBombData.days;
}

#endif	//DEMO