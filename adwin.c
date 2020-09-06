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

//depot/projects/eudora/mac/Current/adwin.c#127 - edit change 17326 (text)
#ifdef AD_WINDOW
#include "adwin.h"
#include "buildversion.h"
/* Window for ads */

#define FILE_NUM 122
#pragma segment ADWIN

#ifdef DEBUG
#define LOGADS
#define kAdScheduleAccelerator 60	//      show ads 60 times faster
#endif
//#define LOGINFO       //      Sends ads server date/time and user id

#ifdef LOGADS
#define LogAd(s) { Str255 sLog; DoLogAds(s); }
#else
#define LogAd(s)
#endif

#define kMaxDownloads	4	//      Don't request more than this many ads at one time
#define kPlayListVersion 22	//      Playlist version we support
#define kReqInverval 1		//      Check for a new playlist after this many days
#define kFaceTimeQuota	60	// default facetime quota minutes
#define kRerunInterval 7	// default return interval, days
#define kPurgeAdDays 90		//      Purge cached ads after this many days
#define kCheckCoverAd 5		//      Check every 5 seconds to see if ad has been covered
#define kCoveredAdDelay 60	//      After we've discovered that the ad window is covered, wait before alerting user
#define kMaxObscuredPixels 500	//      Nag user if this many pixels are obscured in ad window
#define kSaveStateSecs 15*60	//      Save playlist state every 15 minutes
#define kMaxReqInterval 14*24	//      Don't wait more than 14 days before downloading a new playlist
#define kMinRetryInterval (4*3600)	// Wait at least 4 hrs between playlist requests
#define kAdWidth 144
#define kAdHeight 144
#define kHistLength	30	//      For how many do we report info on ads?
#define kReqInterval 24
#define kSchedType kLinear
#define kPlayListServerId -1	//      Pseudo server id indicating playlist, not ad
#define kAdDragBarHeight 8
#define kNewAdTypingTicks 300	//      Don't display a new ad unless the user hasn't typed for this many ticks
#define kAdDownloadAttemptTicks 7200	// Don't attempt to download ads if we've tried to recently
#define kCheckSumSize 16
#define kQuotaSlop	(60)
typedef long StrOffset;		//      For accessing strings at end of playlist data

typedef enum { vaCheckNormalAdOnly = 1 << 0, vaCheckDayMax =
	    1 << 1, vaCheckShowForMax = 1 << 2, vaCheckRerun =
	    1 << 3 } ValidAdFlags;

//      PlayList data. Saved in one resource for each playlist
#define kPlayListVers 4
#define kPlayListResType	'Eupl'
enum SchedType { kLinear, kRandom };
typedef enum { kIsButton = 1, kIsSponsor, kIsRunout } AdType;
enum MixType { kMix, kBlock };

//      Increment kPlayListVers if AdInfo or PlayList is changed
typedef struct {
	AdType type;
	long showFor;		//      How many seconds to display?
	long dayMax;		//      Maximum times to display per day
	long showForMax;	//      Maximum seconds to show total
	StrOffset srcURL;	//      URL to ad (c-string)
	long blackBefore;	//      Seconds of black before ad
	long blackAfter;	//      Seconds of black before ad
	uLong startDate;	//      Don't start before this time
	uLong endDate;		//      Don't display after this time
	AdId adId;
	StrOffset title;	//      Title (used in link window)
	StrOffset clickURL;	//      URL to where we go when we click
	unsigned char checksum[kCheckSumSize];
} AdInfo, *AdInfoPtr;

typedef struct PlayListStruct PlayList, *PlayListPtr, **PlayListHandle;
struct PlayListStruct {
	long version;
	PlayListHandle next;
	long strOffset;		//      Offset to beginning of strings
	StrOffset playListIDString;
	StrOffset clickBase;
	long playListId;	//      hash of playlist id string
	enum MixType mixType;
	enum SchedType blockScheduleType;
	long totalShowMax;	//      Sum of showMax values for all ads
	short numAds;
	AdInfo ads[];
};

//      Play state data - what is the state of ad display, saved in a resource
//      Increment kStateVersion if AdState or PlayState is changed
#define kStateVersion 12
#define kStateResType	'Euas'
#define kStateResId	1000
#define kPastryResId 1001

//      State info for ads. Use for playlist state also.
typedef struct {
	AdId adId;		//      Ad Id, or playlist ID if adId.server == -1
	long shownToday;	//      How many times displayed today
	long shownTotal;	//      How many total seconds displayed for this ad
	uLong lastDisplayTime;	//      When did we last display the ad? Used for aging.
	Boolean unused;		//      was: flushed
	Boolean deleted;	//      Toolbar button deleted?
} AdState, *AdStatePtr;

typedef struct {
	short version;		//      version # for this data (kStateVersion)
	long showCur;		//      How long will current ad be shown
	short numAds;

	//      Client info
	long adWidth;
	long adHeight;
	long reqInterval;
	long faceTimeQuota;
	enum SchedType schedType;
	short histLength;

	long curPlayList;
	AdId curAd;

	DateTimeRec today;
	DateTimeRec getNextPlaylist;
	short unused;
	short adsNotFetched;	//      How many ads have we been unable to fetch into cache?
	short errCode;		//      Are we unable to get ads?
	Boolean outOfAds;
	Boolean dirty;
	long faceTimeToday;	//      Today's total facetime
	long faceTimeWeek[7];	//      Facetime for past 7 days
	AdState stateAds[];
} PlayState11, *PlayState11Ptr, **PlayState11Handle;

typedef struct {
	short version;		//      version # for this data (kStateVersion)
	long showCur;		//      How long will current ad be shown
	short numAds;

	//      Client info
	long adWidth;
	long adHeight;
	long reqInterval;
	long faceTimeQuota;
	enum SchedType schedType;
	short histLength;

	long curPlayList;
	AdId curAd;

	DateTimeRec today;
	DateTimeRec getNextPlaylist;
	short unused;
	short adsNotFetched;	//      How many ads have we been unable to fetch into cache?
	short errCode;		//      Are we unable to get ads?
	Boolean outOfAds;
	Boolean dirty;
	long faceTimeToday;	//      Today's total facetime
	long faceTimeWeek[7];	//      Facetime for past 7 days
	long adFaceTimeToday;	// Today's facetime for ads
	long rerunInterval;
	long spare[19];		// Leave some room, fer goodness' sake!
	AdState stateAds[];
} PlayState, *PlayStatePtr, **PlayStateHandle;

//      List of ads yet to be downloaded
typedef struct DownloadList **DownloadListHandle;
typedef struct DownloadList {
	DownloadListHandle next;
	Handle url;
	AdId adId;
	Boolean display;
	AdType adType;
} DownloadList;

#define REMOVE_AND_DESTROY(hPlayList) \
	do { \
		LL_Remove(gPlayListQueue,hPlayList,(PlayListHandle)); \
		RemoveResource((Handle)hPlayList); \
		if (gPlayList==hPlayList) gPlayList = gPlayListQueue; \
		ZapHandle(hPlayList); \
		UpdateResFile(SettingsRefN); \
	} while (0)

/************************************************************************
 * globals
 ************************************************************************/
long Secret3 = 0x65B9DE33;
char PlayListURL[] = "\phttp://adserver.eudora.com/adjoin/playlists";
long Secret1 = 0x6867B92E;
StringPtr PlayListKeywords[] = {
	//      Client update response parameters
	"\pclientInfo",
	"\preqInterval", "\phistLength", "\ppastry", "\pflush",
	    "\pscheduleAlgorithm",
	"\pwidth", "\pheight", "\pfaceTimeQuota", "\prerunInterval",
	"\pfault", "\pfaultCode", "\pfaultString",
	// Nag
	"\pnag", "\plevel",
	//      Reset
	"\preset",
	//      Client update request parameters
	"\pclientUpdate",
	"\puserAgent", "\plocale", "\pclientMode", "\pfaceTime",
	    "\pfaceTimeLeft", "\pdistributorID", "\pscreen", "\pprofile",
	"\pfaceTimeUsedToday", "\pplayListVersion",
#ifdef LOGINFO
	"\puserID", "\pdateAndTime",
#endif
	//      Playlist parameters
	"\pplaylist",
	"\pplaylistID", "\pmixAlgorighm", "\pblockScheduleAlgorithm",
	    "\pclickBase",
	//      Ad entry parameters
	"\pentry",
	"\pdayMax", "\pblackBefore", "\pblackAfter",
	"\pshowFor", "\pstartDT", "\pendDT",
	"\pshowForMax", "\pstartTime", "\pendTime",
	"\psrc", "\padID",
	"\ptitle", "\purl",
	//      Attributes
	"\plinear", "\prandom",	// scheduleAlgorithm
	"\pmix", "\pblock",	// mixAlgorithm
	"\pisButton", "\pisSponsor", "\pisRunout",	// entry type
	"\pentity", "\pid",	// for flush
	"\pchecksum",		// checksum for entry.src
	""
};

enum {
	kClientInfoTkn,
	kReqIntervalTkn, kHistLengthTkn, kPastryTkn, kFlushTkn,
	    kScheduleTkn,
	kWidthTkn, kHeightTkn, kFaceTimeQuotaTkn, kRerunIntervalTkn,
	kFaultTkn, kFaultCodeTkn, kFaultStringTkn,
	kNagTkn, kLevelTkn,
	kResetTkn,
	kClientUpdateTkn,
	kUserAgentTkn, kLocaleTkn, kClientModeTkn, kFaceTimeTkn,
	    kFaceTimeLeftTkn, kDistIDTkn, kScreenTkn, kUserProfileTkn,
	kFaceTimeUsedTodayTkn, kPlayListVersionTkn,
#ifdef LOGINFO
	kUserId, kDateAndTime,
#endif
	kPlayListTkn,
	kPlayListIdTkn, kMixAlgorithmTkn, kBlockScheduleAlgorithmTkn,
	    kClickBaseTkn,
	kEntryTkn,
	kDayMaxTkn, kBlackBeforeTkn, kBlackAfterTkn,
	kShowForTkn, kStartDateTkn, kEndDateTkn,
	kShowForMax, kStartTime, kEndTime,
	kSrcTkn, kAdIdTkn,
	kTitleTkn, kClickTkn,
	//      Attributes
	kLinearTkn, kRandomTkn,
	kMixTkn, kBlockTkn,
	kIsButtonTkn, kIsSponsorTkn, kIsRunoutTkn,
	kEntityTkn, kIdTkn,
	kCheckSum,
};

//      Global variables
MyWindowPtr AdWin;
PETEHandle ADpte;
PlayListHandle gPlayList;
PlayStateHandle gPlayState;
PlayListHandle gPlayListQueue;
uLong gNextAdSecs;
long Secret2 = 0xA30580D2;
short gDisplayMinutes = 1;
long gBlackTime, gNextBlackTime;
FMBHandle gFaceMeasure;
FMBHandle gGlobalFaceMeasure;
Boolean gAdWinCanClose, gPLDownloadInProgress;
OSErr gDownloadErrCode;
AdId gWasCurAd;
Boolean gDoNextAd;
Boolean gPlayListDownloadFail;
DownloadListHandle gAdDownloadList;
short gDownloadCount;
StringHandle gDownloadErrString;
short gAdsMenuAdsStart;
TBAdHandle gTBAds;
uLong gPlayListFetchGMT;
PicHandle gSponsorAdPic;	//      "co-branding spot"
AdId gSponsorAdId;
AdId gInsertAdId;
Boolean gInsertAd;
short gAdFetchCount;
#ifdef DEBUG
enum { dbNewPlaylist =
	    1, dbReloadPlaylist, dbDumpPlaylists, dbDiv,
	    dbDefaultPLServer };
char sPlayListFileName[] = "\peudora ads";
#endif

/************************************************************************
 * prototypes
 ************************************************************************/
void AdClick(MyWindowPtr win, EventRecord * event);
void NextAd(long secsDisplayed);
void NewAd(short adIdx);
void InsertAd(short adIdx);
static OSErr BuildPlayList(UHandle text);
static void DisposePlaylists(void);
static void CleanupAdCache(Boolean withPrejuidice);
#ifdef LOGADS
static void DoLogAds(StringPtr sLog);
#endif
static Boolean AdClose(MyWindowPtr win);
Boolean AdPosition(Boolean save, MyWindowPtr win);
static void AdTransition(MyWindowPtr win, UserStateType oldState,
			 UserStateType newState);
static AdInfoPtr FindAd(AdId adId, short *idx, PlayListHandle * playList);
static AdInfoPtr FindCurrentAd(void);
static AdInfoPtr FindCurrentAdIdx(short *idx);
static void SavePlayStatus(void);
static void SavePastry(Ptr pPastry, long len);
static void AdCursor(Point mouse);
static void NewDayCheck(Boolean withPrejuidice);
static void PreFetchAds(void);
static OSErr GetAdSpec(FSSpecPtr spec, AdId adId);
static void ParseAdId(StringPtr sToken, AdId * pAdId);
static void AdUpdate(MyWindowPtr win);
static void ShowBlackTimeImage(void);
static Boolean ValidAd(PlayListHandle hPlayList, short adIdx,
		       uLong curTime, ValidAdFlags flags);
static OSErr LoadPlaylists(void);
void RequestPlaylist(void);
void NewPlaylist(FSSpecPtr spec, StringPtr checkSum);
#ifdef DEBUG
static void SetupAdDebugMenu();
void DumpPlaylists(Boolean fullDisplay);
static Boolean DumpClose(MyWindowPtr win);
static void AccuDump(StringPtr s);
static void AccuDumpTime(StringPtr sName, uLong seconds);
#endif
static StringPtr GetPlayListString(PlayListHandle hPlayList,
				   StrOffset offset, StringPtr s);
static Handle GetPlayListURL(PlayListHandle hPlayList, StrOffset offset);
static StringPtr GetPlayListURLString(PlayListHandle hPlayList,
				      StrOffset offset, StringPtr s);
static AdStatePtr FindAdState(AdId adId, short *idx);
static AdStatePtr FindCurrentAdState(void);
static OSErr AddStringToHandle(StrOffset * offset, AccuPtr a, StringPtr s);
static PlayListHandle FindPlayList(long playListId);
static void AddStates(PlayListHandle hPlayList, AccuPtr aState,
		      Boolean init);
static AdStatePtr FindPlayListState(PlayListHandle hPlayList);
static void SetCurrentPlayList(void);
static AdInfoPtr FindAdType(AdType adType, short *idx,
			    PlayListHandle * playList);
static AdInfoPtr FindAdTypeLo(AdType adType, short startIdx,
			      PlayListHandle startPlayList, short *idx,
			      PlayListHandle * playList);
static void Flush(TokenInfo * pInfo);
static Boolean QueueAdDownload(Handle url, AdId adId, AdType adType);
static void CheckDownloadList(void);
static void FinishedAdDownload(long refCon, OSErr err,
			       DownloadInfo * info);
static void PrunePlayState(Handle hKeepAdList);
static OSErr AdDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag);
Boolean AdCoveredRect(Rect * r);
static void SetupToolbarAds(void);
static void MakeAdIconFromFile(FSSpecPtr spec, Handle iconSuite);
static void MakeAdIcon(PlayListHandle hPlayList, StrOffset srcURL,
		       AdId adId, Handle iconSuite, Boolean downloadOK);
static void SetTBAdIcon(AdId adId);
static void DeleteAd(AdId adId);
long TotalFaceTimeLeft(void);
void RollGlobalFacetime(void);
OSErr PastryIsStale(Handle pastry);
static OSErr SetupAdState(Boolean reinit);
static void ResetAdState(void);
void LogPlayStatus(void);
OSErr SetupForDisplay(Ptr data, long dataLen,
		      unsigned char checksum[kCheckSumSize],
		      Boolean check);
static void ZapSponsorAd(void);
static OSErr CheckAd(AdId adId, Handle hData, FSSpec * spec);
static void InitAdPte(void);
static void ResetAdSchedule(void);
static Boolean IsChecksumFailAgain(AdId adId);
OSErr GetAdFolderSpec(FSSpecPtr spec);
OSErr HandleNag(long level, PStr text);
OSErr HandleUserProfile(PStr text);
OSErr HandleClientMode(PStr text);
OSErr ReadNoAdsRec(NoAdsAuxPtr noAds);
OSErr SaveNoAdsRec(NoAdsAuxPtr noAds);
OSErr TestWriteToAdFolder(void);

/************************************************************************
 * OpenAdWindow - open ad window
 ************************************************************************/
void OpenAdWindow(void)
{
	if (IsAdwareMode()) {
		if (!AdWin) {
			//      Make sure we have an ad cache folder
			FSSpec spec;

			GetAdFolderSpec(&spec);	//      Make sure we have an ad folder

			if (AdWin =
			    GetNewMyWindowWithClass(AD_WIND, nil, nil,
						    BehindModal, False,
						    False, AD_WIN,
						    // Give the ad window a very high float level under OS X so that
						    // nothing can obscure it
						    // Don't float so high for OS 9. It causes problems with dialogs
						    HaveOSX()?
						    kHelpWindowClass :
						    kToolbarWindowClass)) {
				WindowPtr AdWinWP;

				if (AdWinWP = GetMyWindowWindowPtr(AdWin)) {
					SetPort_(GetWindowPort(AdWinWP));
					MySetThemeWindowBackground(AdWin,
								   kThemeActiveModelessDialogBackgroundBrush,
								   False);

					AdWin->position = AdPosition;
					AdWin->isRunt = true;
					AdWin->windowType = kDockable;
					AdWin->click = AdClick;
					AdWin->close = AdClose;
					AdWin->transition = AdTransition;
					AdWin->cursor = AdCursor;
					AdWin->update = AdUpdate;
					AdWin->drag = AdDragHandler;

					InitAdPte();

					gFaceMeasure = NewFaceMeasure();
					FaceMeasureBegin(gFaceMeasure);

					gBlackTime = -1;	//      Negative value means display black until an ad shows up
					ShowMyWindow(AdWinWP);
					PositionDockedWindow(AdWinWP);

#ifdef DEBUG
					SetupAdDebugMenu();
#endif
					ReadNoAdsRec(&NoAdsRec);

					//      Get playlists
					if (!LoadPlaylists()) {
						if (!gPlayListQueue && !TCPWillDial(true))	// Don't connect the net on first launch!
							RequestPlaylist();
						SizeAdWin();
					} else
						//      Can't set up playlists
						CloseAdWindow();
					NewDayCheck(false);	//      Check for new day before displaying first ad
				}
			}
		}
		if (!AdWin || !ADpte)
			DieWithError(INIT_ADWIN_ERR,
				     MemError()? MemError() : ResError()?
				     ResError() : 1);
	} else if (IsFreeMode()) {
		LoadPlaylists();	//      Load playlists so we can check for sponsor ad
	}
	SetupToolbarAds();	//      Make sure toolbar ads are installed (or removed)
	SetupSponsorAd();
}

/************************************************************************
 * CloseAdWindow - close ad window
 ************************************************************************/
void CloseAdWindow(void)
{
	if (AdWin) {
		// if an ad was up, record that it's now gone.
		if (gPlayList && (gWasCurAd.server || gWasCurAd.ad)) {
			AuditAdClose(gWasCurAd.server, gWasCurAd.ad);
			gWasCurAd.server = gWasCurAd.ad = 0;
		}
		gAdWinCanClose = true;
		CloseMyWindow(GetMyWindowWindowPtr(AdWin));
		gAdWinCanClose = false;
		AdWin = nil;
	}
	RollGlobalFacetime();	// save faceTimeToday

	gNextAdSecs = 0;
	gBlackTime = gNextBlackTime = 0;
	SavePlayStatus();
	DisposePlaylists();
	DisposeFaceMeasure(gFaceMeasure);
	gFaceMeasure = nil;
	if (gPlayState) {
		ReleaseResource((Handle) gPlayState);
		gPlayState = nil;
	}
}

/************************************************************************
 * AdShutdown - Eudora is shutting down
 ************************************************************************/
void AdShutdown(void)
{
	SavePlayStatus();
}

/************************************************************************
 * AdPosition - position the ad window
 ************************************************************************/
Boolean AdPosition(Boolean save, MyWindowPtr win)
{
	WindowPtr winWP = GetMyWindowWindowPtr(win);
	Boolean did = PositionPrefsTitle(save, win);
	short spot;

	if (!save && !did && winWP) {
		// we have no idea where the ad window should go.  Position
		// it in the lower-right
		GDHandle gd = MyGetMainDevice();
		Rect r;
		Rect rCovered;

		SizeWindow(winWP, GetRLong(AD_WINDOW_SIZE_GUESS_X),
			   GetRLong(AD_WINDOW_SIZE_GUESS_Y), false);

		for (spot = 0; spot < 5; spot++) {
			r = (*gd)->gdRect;
			if (gd == GetMainDevice())
				r.top += GetMBarHeight();

			switch (spot) {
			case 0:
			case 4:
				// Bottom left
				r.right = r.left + WindowWi(winWP);
				r.top = r.bottom - WindowHi(winWP);
				break;
			case 1:
				// Bottom right, left of trash
				r.left = r.right - WindowWi(winWP) - 48;
				r.right -= 48;
				r.top = r.bottom - WindowHi(winWP);
				break;
			case 2:
				// Bottom right, above trash
				r.left = r.right - WindowWi(winWP);
				r.top = r.bottom - WindowHi(winWP) - 48;
				r.bottom -= 48;
				break;
			case 3:
				// Upper right, left of disk
				r.left = r.right - WindowWi(winWP) - 48;
				r.right -= 48;
				r.bottom = r.top + WindowHi(winWP);
				break;
			}

			utl_RestoreWindowPos(winWP, &r, false, 1, 0,
					     LeftRimWidth(winWP),
					     (void *) FigureZoom,
					     (void *) DefPosition);
			PositionDockedWindow(winWP);	//      Make sure positioned correctly if docked

			// Show it now so we can see if it's covered
			ShowHide(winWP, true);
			StashStructure(win);
			if (AdCoveredRect(&rCovered)) {
				if (rCovered.top > (r.top + r.bottom) / 2
				    && (spot == 0 || spot == 2)) {
					// Rat doot; try moving it up
					r.bottom = rCovered.top;
					r.top = r.bottom - WindowHi(winWP);
					utl_RestoreWindowPos(winWP, &r,
							     false, 1, 0,
							     LeftRimWidth
							     (winWP),
							     (void *)
							     FigureZoom,
							     (void *)
							     DefPosition);
					PositionDockedWindow(winWP);	//      Make sure positioned correctly if docked
				}
			}
			if (!AdCoveredRect(&rCovered))
				break;
		}
		if (AdCoveredRect(&rCovered))
			ComposeStdAlert(Note, SORRY_AD_COVERED);
		did = true;
	}
	return did;
}

/************************************************************************
 * AdClose - close ad window?
 ************************************************************************/
static Boolean AdClose(MyWindowPtr win)
{
	//      Normally we won't want anything to close the ad window
	//      It is allowed only when CloseAdWindow is called
	return gAdWinCanClose;
}

/************************************************************************
 * AdTransition - transition the window between user states
 ************************************************************************/
static void AdTransition(MyWindowPtr win, UserStateType oldState,
			 UserStateType newState)
{
	if (!AdwareMode(newState))
		CloseAdWindow();
	else if (!AdwareMode(oldState) && AdwareMode(newState))
		OpenAdWindow();
}

/************************************************************************
 * AdClick - click in ad window
 ************************************************************************/
void AdClick(MyWindowPtr win, EventRecord * event)
{
	Point pt;

	pt = event->where;
	GlobalToLocal(&pt);
	//      Drag window if no drag bar with click and drag, or inside dragbar
	if ((PrefIsSet(PREF_NO_ADWIN_DRAG_BAR)
	     && MyWaitMouseMoved(event->where, True))
	    || (!PtInRect(pt, &win->contR))) {
		//      In drag bar     
		DragMyWindow(GetMyWindowWindowPtr(win), event->where);
	} else if (!gBlackTime)
		AdUserClick((*gPlayState)->curAd);
}

/************************************************************************
 * AdUserClick - handle a click on an ad
 ************************************************************************/
void AdUserClick(AdId adId)
{
	Str255 s;
	AdInfoPtr pAd;
	PlayListHandle hPlayList;

	if ((pAd = FindAd(adId, nil, &hPlayList)) && hPlayList) {
		OSErr err = noErr;
		StrOffset clickURL = pAd->clickURL;

		if ((*hPlayList)->clickBase) {
			//      Need to append ad URL to this click base URL
			Str255 sURL;
			AdURLStringsRec adStrings;
			Str32 sAdId;

			GetPlayListURLString(hPlayList, clickURL, sURL);
			ComposeString(sAdId, "\p%d.%d", adId.server,
				      adId.ad);
			adStrings.adID = sAdId;
			adStrings.destination = sURL;
			GetPlayListURLString(hPlayList,
					     (*hPlayList)->clickBase, s);
			err =
			    OpenAdwareURLStr(GetNagState(), s,
					     actionClickThrough,
					     clickThruQuery,
					     (long) &adStrings);
		} else {
			Handle url = GetPlayListURL(hPlayList, clickURL);
			if (url) {
				char *urlCStr = LDRef(url);
				short urlLen = strlen(urlCStr);

				if (fnfErr ==
				    (err =
				     OpenLocalURLPtr(urlCStr, urlLen, nil,
						     nil, false)))
					if ((err =
					     ParseProtocolFromURLPtr
					     (urlCStr, urlLen,
					      s)) == noErr)
						err =
						    OpenOtherURLPtr(s,
								    urlCStr,
								    urlLen);
				ZapHandle(url);
			}
			AdWasClicked(adId, err);
		}
	}
	AuditAdHit(adId.server, adId.ad);
}

/************************************************************************
 * AdWinIdle - idle time handler for ad window
 ************************************************************************/
void AdWinIdle(void)
{
	static uLong nextIdle, nextCheckCover, nextReportCovered,
	    nextSaveStatus;
	static uLong lastAdAttempt = 0;
	static Boolean sponsorAdInited = false;
	WindowPtr AdWinWP;
	uLong faceTime, totalTime, seconds;
	DateTimeRec dt;

	if (IsPayMode())
		return;		//      Not in adware or free mode

	if (gInsertAd) {
		short adIdx;
		if (FindAd(gInsertAdId, &adIdx, &gPlayList))
			InsertAd(adIdx);
		gInsertAd = false;
	}

	if (nextIdle && TickCount() < nextIdle)
		return;		//      Once a second is sufficient
	nextIdle = TickCount() + 60;

	if (!gPlayState)
		SetupAdState(false);

	NewDayCheck(false);	//      Have we gone to a new day?

	if (gActiveConnections > 0)	// are we connected to the network right now somehow?
	{
		// have we tried downloading ads recently?
		if (TickCount() - lastAdAttempt > kAdDownloadAttemptTicks) {
			// go fetch the ads that we need
			AdCheckingMail();
			lastAdAttempt = TickCount();
		}
	}

	CheckDownloadList();	//      Any ads to download?

	seconds = LocalDateTime();

	if (nextSaveStatus) {
		if (seconds >= nextSaveStatus) {
			SavePlayStatus();
			nextSaveStatus = seconds + kSaveStateSecs;
		}
	} else
		nextSaveStatus = seconds + kSaveStateSecs;

	if (IsFreeMode() && !sponsorAdInited) {
		SetupToolbarAds();	//      We haven't set up the sponsor ad yet
		sponsorAdInited = true;
	}

	//      Has playlist expired?
	GetTime(&dt);
	if (TimeCompare(&dt, &(*gPlayState)->getNextPlaylist) >= 0) {
		//      Need a new playlist
		DateTimeRec nextPlaylist;

		//      Set next time for download in an hour so we don't
		//      continually request playlists
		SecondsToDate(LocalDateTime() + 60L * 60L, &nextPlaylist);
		(*gPlayState)->getNextPlaylist = nextPlaylist;
		RequestPlaylist();
	}

	if (!AdWin)
		return;		//      Ad window is not currently open. We may be in free mode.

	if (InBG
	    || FaceMeasureReport(gFaceMeasure, &faceTime, nil, nil,
				 &totalTime))
		return;

	//      End of face-to-black time? Don't start ad display unless user is looking.
	if (gBlackTime > 0) {
		if (totalTime >= gBlackTime) {
			//      Fade-to-black time is over. Can draw again
			LogAd(ComposeString
			      (sLog, "\pShow for: %d", gNextAdSecs));
			gBlackTime = 0;
			FaceMeasureReset(gFaceMeasure);
			InvalContent(AdWin);
		}
	}

	//      Time for a new ad?
	else if ((TickCount() - TypingTicks > kNewAdTypingTicks)
		 && ((gNextAdSecs && faceTime >= gNextAdSecs)
		     || gDoNextAd)) {
		gDoNextAd = false;
		NextAd(gNextAdSecs);
	}

	if (!AdWin)
		return;		//      Ad window has been closed

	if (AdWinWP = GetMyWindowWindowPtr(AdWin))
		if (!IsWindowVisible(AdWinWP)) {
			// Someone's trying to pull a fast one. Make sure ad window stays visible.
			ShowHide(AdWinWP, true);
			StashStructure(AdWin);
		}

#ifndef I_HATE_THE_BOX
	if (seconds >= nextCheckCover) {
		WindowPtr theWindow;
		Rect rSect;

		//      Search for any windows in front of ad window that overlap
		//      If so, bring ad window in front
		for (theWindow = FrontWindow();
		     theWindow && theWindow != AdWinWP;
		     theWindow = GetNextWindow(theWindow)) {
			Rect rStrucWin, rStrucAdWin;
			if (SectRect
			    (GetStructureRgnBounds(theWindow, &rStrucWin),
			     GetStructureRgnBounds(AdWinWP, &rStrucAdWin),
			     &rSect)) {
				if (!ModalWindow)
					BringToFront(AdWinWP);
				break;
			}
		}

		//      Make sure there aren't any system floating windows in front             
		if (AdCoveredRect(nil)) {
			//      Ad window is obscured. Start time to ask user to uncover.
			if (!nextReportCovered)
				nextReportCovered =
				    seconds + kCoveredAdDelay;
			else if (seconds >= nextReportCovered) {
				extern ModalFilterUPP DlgFilterUPP;
				//      Time to tell the user to uncover the ad window
				nextReportCovered = 0;
				Nag(AD_OBSCURED_DLOG, nil, HitMeHitMeHitMe,
				    DlgFilterUPP, false, 0);
			}
		} else
			nextReportCovered = 0;
		nextCheckCover = seconds + kCheckCoverAd;
	}
#endif

#ifdef DEBUG
	{
		static short fastAds;

		if (BUG4 != fastAds) {
			if (BUG4) {
				//      Catch on to change to fast ads quickly
				if (gNextAdSecs)
					gDoNextAd = true;
				gBlackTime = 0;
			}
			fastAds = BUG4;
		}
	}
#endif

}

/************************************************************************
 * AdCoveredRect - Find the covered part of the ad
 ************************************************************************/
Boolean AdCoveredRect(Rect * r)
{
	CGrafPtr AdWinPort = GetMyWindowCGrafPtr(AdWin);
	RgnHandle rgn = NewRgn();
	Rect rCovered;

	Zero(rCovered);

	if (rgn && AdWinPort && !ModalWindow)	// don't check if modal window up!
	{
		Rect rPortRect, rRgnBounds;

		//      The visRgn should cover this entire region
		RectRgn(rgn, GetPortBounds(AdWinPort, &rPortRect));
		//      It's OK if a few pixels around the edges are obscured
		InsetRgn(rgn, 5, 5);
		//      Subtract out the visRgn. Anything left over is an obscured area
		DiffRgn(rgn, MyGetPortVisibleRegion(AdWinPort), rgn);
		//  Globalize it
		PushGWorld();
		SetPort(AdWinPort);
		GlobalizeRgn(rgn);
		PopGWorld();
		//  Record rectangle
		rCovered = *GetRegionBounds(rgn, &rRgnBounds);
		DisposeRgn(rgn);
	}

	if (r)
		*r = rCovered;

	return (RectHi(rCovered) * RectWi(rCovered) > kMaxObscuredPixels);
}


/************************************************************************
 * AdCursor - set the cursor
 ************************************************************************/
static void AdCursor(Point mouse)
{
#ifdef DEBUG
	if (CurrentModifiers() & controlKey) {
		SetMyCursor(MENU_CURS);
		return;
	}
#endif
	if (!gBlackTime && PtInRect(mouse, &AdWin->contR))
		SetMyCursor(POINTER_FINGER);
	else
		//      In drag bar     
		SetMyCursor(arrowCursor);
}

/************************************************************************
 * AdUpdate - update ad window
 ************************************************************************/
static void AdUpdate(MyWindowPtr win)
{
	CGrafPtr AdWinPort = GetMyWindowCGrafPtr(AdWin);
	Rect r, rPortBounds;
	PixPatHandle hPixPat;

	if (!PrefIsSet(PREF_NO_ADWIN_DRAG_BAR)) {
		//      Draw the drag bar
		//      Start with some borders
		r = *GetPortBounds(AdWinPort, &rPortBounds);
		SetForeGrey(0);	//      black
		if (RectHi(r) * 2 <= RectWi(r)) {
			// Put title bar on the side of this short, wide window
			r.right = kAdDragBarHeight - 1;
			MoveTo(kAdDragBarHeight - 1, 0);
			LineTo(kAdDragBarHeight - 1, r.bottom);
		} else {
			// Put title bar on top of normal window, 
			r.bottom = kAdDragBarHeight - 1;
			MoveTo(0, kAdDragBarHeight - 1);
			LineTo(r.right, kAdDragBarHeight - 1);
		}

		//      Now the pattern
		if (hPixPat = GetPixPat(DRAG_BAR_PIXPAT)) {
			//InsetRect(&r,1,1);
			//r.bottom--;
			FillCRect(&r, hPixPat);
			DisposePixPat(hPixPat);
		}
	}

	if (gBlackTime || !gNextAdSecs) {
		ShowBlackTimeImage();
		SetEmptyVisRgn(AdWinPort);	//      Don't allow pete handle to be updated
	}
}

/************************************************************************
 * AdDragHandler - we don't accept any drags
 ************************************************************************/
static OSErr AdDragHandler(MyWindowPtr win, DragTrackingMessage which,
			   DragReference drag)
{
	//      We don't wany any window highlighting during a drag to the ad window
	//      Return an error, but not handlerNotFoundErr because PantyTrack will
	//      still do a PeteDrag
	return -1;
}

/************************************************************************
 * SavePlayStatus - save play status resource
 ************************************************************************/
static void SavePlayStatus(void)
{
	if (gPlayState && (*gPlayState)->dirty) {
		(*gPlayState)->dirty = false;
		ChangedResource((Handle) gPlayState);
		MyUpdateResFile(SettingsRefN);
#ifdef DEBUG
		LogPlayStatus();
#endif
	}
}

/************************************************************************
 * LogPlayStatus - log gPlayState
 ************************************************************************/
void LogPlayStatus(void)
{
	if (gPlayState) {
		ComposeLogS(LOG_PLIST, nil,
			    "\pversion %d showCur %d numAds %d",
			    (*gPlayState)->version, (*gPlayState)->showCur,
			    (*gPlayState)->numAds);

		ComposeLogS(LOG_PLIST, nil,
			    "\padWidth %d adHeight %d reqInterval %d faceTimeQuota %dm",
			    (*gPlayState)->adWidth,
			    (*gPlayState)->adHeight,
			    (*gPlayState)->reqInterval,
			    (*gPlayState)->faceTimeQuota / 60);

		ComposeLogS(LOG_PLIST, nil,
			    "\pschedType %d histLength %d curPlayList %x curAd %d.%d",
			    (*gPlayState)->schedType,
			    (*gPlayState)->histLength,
			    (*gPlayState)->curPlayList,
			    (*gPlayState)->curAd.server,
			    (*gPlayState)->curAd.ad);

		ComposeLogS(LOG_PLIST, nil,
			    "\padFetchCount %d adsNotFetched %d errCode %d outOfAds %d",
			    gAdFetchCount, (*gPlayState)->adsNotFetched,
			    (*gPlayState)->errCode,
			    (*gPlayState)->outOfAds);

		ComposeLogS(LOG_PLIST, nil,
			    "\pdirty %d adDay %d totalDay %d  S %d  M %d  T %d  W %d  R %d  F %d  S %d (mins)",
			    (*gPlayState)->dirty,
			    (*gPlayState)->adFaceTimeToday / 60,
			    (*gPlayState)->faceTimeToday / 60,
			    (*gPlayState)->faceTimeWeek[0] / 60,
			    (*gPlayState)->faceTimeWeek[1] / 60,
			    (*gPlayState)->faceTimeWeek[2] / 60,
			    (*gPlayState)->faceTimeWeek[3] / 60,
			    (*gPlayState)->faceTimeWeek[4] / 60,
			    (*gPlayState)->faceTimeWeek[5] / 60,
			    (*gPlayState)->faceTimeWeek[6] / 60);
	}
}

/************************************************************************
 * SizeAdWin - resize ad window
 ************************************************************************/
void SizeAdWin(void)
{
	WindowPtr AdWinWP = GetMyWindowWindowPtr(AdWin);
	if (AdWin && AdWinWP && gPlayState) {
		Rect r;
		short dragBarHt = 0, dragBarWd = 0;

		if (!PrefIsSet(PREF_NO_ADWIN_DRAG_BAR)) {
			if ((*gPlayState)->adHeight * 2 >
			    (*gPlayState)->adWidth)
				dragBarHt = kAdDragBarHeight;
			else
				// Title bar on left side of window
				dragBarWd = kAdDragBarHeight;
		}
		SetRect(&r, 0, 0, (*gPlayState)->adWidth + dragBarWd,
			(*gPlayState)->adHeight + dragBarHt);
		SizeWindow(AdWinWP, r.right, r.bottom, true);
		SetPrefLong(AD_WINDOW_SIZE_GUESS_X, RectWi(r));
		SetPrefLong(AD_WINDOW_SIZE_GUESS_Y, RectHi(r));
		r.left = dragBarWd;
		r.top = dragBarHt;
		AdWin->contR = r;
		PeteDidResize(ADpte, &r);
		PositionDockedWindow(AdWinWP);	//      Make sure positioned correctly if docked
	}
}


/**********************************************************************
 * FinishedPlayListDownload - we have finished downloading a play list
 **********************************************************************/
static void FinishedPlayListDownload(long refCon, OSErr err,
				     DownloadInfo * info)
{
	FSSpec spec;

	gPLDownloadInProgress = false;
#ifdef DEBUG
	ComposeLogS(LOG_PLIST, nil, "\pPlayList Fetch Done: %d", err);
#endif
	if (!err) {
		//      Use new playlist
		NewPlaylist(&info->spec, info->checksum);
		gPlayListDownloadFail = false;
	} else {
		//      Download failed.
		DateTimeRec nextPlaylist;

		gPlayListDownloadFail = true;
		//      Let's try again in an hour. Will also try on each mail check
		if (gPlayState) {
			SecondsToDate(LocalDateTime() + 60L * 60L,
				      &nextPlaylist);
			(*gPlayState)->getNextPlaylist = nextPlaylist;
		}
	}

	spec = info->spec;

#ifdef DEBUG
	if (RunType != Production) {
		FSSpec keepSpec;

		//      Let's keep playlist file for debugging

		//      Move file from temporary folder to ad folder
		GetAdFolderSpec(&keepSpec);
		FSpCatMove(&spec, &keepSpec);
		SubFolderSpec(AD_FOLDER_NAME, &keepSpec);
		PCopy(keepSpec.name, sPlayListFileName);
		FSpDelete(&keepSpec);
		PCopy(keepSpec.name, spec.name);
		FSpRename(&keepSpec, sPlayListFileName);
	} else
#endif
		FSpDelete(&info->spec);
	gDownloadErrCode = err;
	if (gPlayState) {
		(*gPlayState)->errCode = err;
	}
}

/**********************************************************************
 * FinishedAdDownload - we have finished downloading an ad
 **********************************************************************/
static void FinishedAdDownload(long refCon, OSErr err, DownloadInfo * info)
{
	FSSpec spec, tempSpec;
	OSErr getAdErr;
	DownloadListHandle hDownload = (DownloadListHandle) refCon;
	Handle url;

	if (!hDownload)
		return;

	if (gPlayState) {
		gAdFetchCount--;
		getAdErr = GetAdSpec(&tempSpec, (*hDownload)->adId);
		if (!err)
			err =
			    FindTemporaryFolder(Root.vRef, Root.dirId,
						&tempSpec.parID,
						&tempSpec.vRefNum);
		if (!err) {
			if (!getAdErr) {
				if (FileTypeOf(&tempSpec) == 'TEXT') {
					//      Is HTML. Download images.
				}

				//      Move file from temporary folder to ad folder
				if (!(err = GetAdFolderSpec(&spec)))
					err = FSpCatMove(&tempSpec, &spec);

				if (!err) {
					switch ((*hDownload)->adType) {
					case kIsButton:
						SetTBAdIcon((*hDownload)->
							    adId);
						break;
					case kIsSponsor:
						SetupSponsorAd();
						break;
					default:
						if (AdWin
						    && (*hDownload)->
						    display) {
							//      This ad should be displayed right away
							gInsertAd = true;
							gInsertAdId =
							    (*hDownload)->
							    adId;
						}
						break;
					}
				}
			}
		}

		if (err) {
			//      Unable to download/install this ad
			(*gPlayState)->adsNotFetched++;
			if (!getAdErr)
				FSpDelete(&tempSpec);

			if ((*hDownload)->display) {
				//      We were supposed to display this ad
				gDoNextAd = true;	//      Set up to display next ad
			}
		}

		(*gPlayState)->errCode = err;
	}
	gDownloadErrCode = err;
	gDownloadCount--;

	url = (*hDownload)->url;
	ZapHandle(url);
	ZapHandle(hDownload);
}


/**********************************************************************
 * NewPlaylist - use a new playlist
 **********************************************************************/
void NewPlaylist(FSSpecPtr spec, StringPtr checkSum)
{
	UHandle text;
	OSErr err = noErr;
	unsigned char key[kCheckSumSize];

	if (!Snarf(spec, &text, 0)) {
		//      validate checksum if we have one
		if (checkSum) {
			Hex2Bytes(checkSum + 1, 32, key);
			err =
			    SetupForDisplay(LDRef(text),
					    GetHandleSize(text), key,
					    true);
		}
		if (!err)
			err = BuildPlayList(text);
		ZapHandle(text);
		if (!err) {
			SizeAdWin();
			gDoNextAd = true;	//      Set up to display next ad
			PreFetchAds();	//      Load all ads into cache
			SetupToolbarAds();
			SetupSponsorAd();
		}
		if (!(*gPlayState)->faceTimeQuota
		    || (*gPlayState)->faceTimeToday <
		    (*gPlayState)->faceTimeQuota)
			// Go back to showing regular ads again if factTimeQuota hasn't expired
			(*gPlayState)->outOfAds = false;
		if (!gNextAdSecs)
			gDoNextAd = true;	//      We were out of ads. Start showing them again
	}
}

/**********************************************************************
 * LoadPlaylists - load and install playlists from playlist resources
 **********************************************************************/
static OSErr LoadPlaylists(void)
{
	short oldResFile, count;
	short i;
	Accumulator aState;
	OSErr err;

	if (gPlayListQueue)
		return noErr;	//      Already got 'em

	oldResFile = CurResFile();
	UseResFile(SettingsRefN);

	//      Setup ad state data
	if (err = SetupAdState(false))
		return err;

	//      Now get playlists
	AccuInitWithHandle(&aState, (Handle) gPlayState);
	count = Count1Resources(kPlayListResType);
	for (i = 1; i <= count; i++) {
		PlayListHandle hPlayList;
		AdStatePtr pPlayListState;

		if (hPlayList =
		    (PlayListHandle) Get1IndResource(kPlayListResType,
						     i)) {
#if kPlayListVers == 4
			if ((*hPlayList)->version == 3) {
				//      Version 3 may have bad checksums. Let's ignore them.
				short adIdx;

				for (adIdx = 0;
				     adIdx < (*hPlayList)->numAds; adIdx++)
					Zero((*hPlayList)->ads[adIdx].
					     checksum);

				(*hPlayList)->version = kPlayListVers;
			}
#else
#error Remove this entire conditional compile directive  if version is greater than 4
#endif
			if ((*hPlayList)->version == kPlayListVers) {
				long shownTotal = 0;
				long totalShowMax = 0;
				long curTime = LocalDateTime();
				short adIdx;

				(*hPlayList)->next = nil;	// old value; destroy
				LL_Queue(gPlayListQueue, hPlayList,
					 (PlayListHandle));
				AddStates(hPlayList, &aState, false);
				pPlayListState =
				    FindPlayListState(hPlayList);

				for (adIdx = 0;
				     adIdx < (*hPlayList)->numAds; adIdx++)
					if (ValidAd
					    (hPlayList, adIdx, curTime,
					     vaCheckNormalAdOnly +
					     vaCheckShowForMax)) {
						AdInfo *pAd =
						    &(*hPlayList)->
						    ads[adIdx];
						AdStatePtr pAdState =
						    FindAdState(pAd->adId,
								nil);
						if (pAdState) {
							shownTotal +=
							    pAdState->
							    shownTotal;
							totalShowMax +=
							    pAd->
							    showForMax;
						}
					}
				(*hPlayList)->totalShowMax = totalShowMax;
				pPlayListState->shownTotal = shownTotal;
			} else
				//      Old playlist format. Delete it.
				RemoveResource((Handle) hPlayList);
		}
	}
	AccuTrim(&aState);	//      Don't dispose of this accumulator!
	SetCurrentPlayList();

	UseResFile(oldResFile);
	gDoNextAd = true;	//      Set up to display first ad

#ifdef DEBUG
	SetupAdDebugMenu();
#endif
	return noErr;
}

/**********************************************************************
 * SetupAdState - setup ad state data
 **********************************************************************/
static OSErr SetupAdState(Boolean reinit)
{
	short oldResFile;
	PlayStatePtr pState;
	uLong stateSize;

	oldResFile = CurResFile();
	UseResFile(SettingsRefN);

	//      Start with play state
	if (!gPlayState)
		gPlayState = Get1Resource(kStateResType, kStateResId);
	if (!gPlayState) {
		//      Create a new play state resource
		gPlayState = NuHandle(sizeof(PlayState));
		if (!gPlayState)
			return MemError();
		AddResource_(gPlayState, kStateResType, kStateResId, "");
	}

#ifdef DEBUG
	LogPlayStatus();
#endif

	RollGlobalFacetime();	// kick off face time measurement for this session

	pState = *gPlayState;

	//      Can't use this if it's a wrong version or if it's not
	//      for the current playlist
	stateSize = sizeof(PlayState) + pState->numAds * sizeof(AdState);
	if (reinit || pState->version != kStateVersion ||
	    stateSize != GetHandleSize((Handle) gPlayState)) {
		OSErr err = 1;

		if (kStateVersion == 12 &&
		    pState->version == 11 &&
		    GetHandleSize(gPlayState) ==
		    sizeof(PlayState11) + pState->numAds * sizeof(AdState)
		    ) {
			// We can rebuild him!
			short sz = sizeof(PlayState);
			short sz11 = sizeof(PlayState11);
			short diff = sz - sz11;
			short szAds = pState->numAds * sizeof(AdState);

			// resize
			SetHandleBig(gPlayState,
				     GetHandleSize(gPlayState) + diff);
			if (!(err = MemError())) {
				// move ads out of the way of new data
				BMD(*gPlayState + sz11, *gPlayState + sz,
				    szAds);
				// zero new data
				WriteZero(*gPlayState + sz11, diff);
				// Update the version.  Forgot this first time
				(*gPlayState)->version = kStateVersion;
			}
		}

		//      Initialize a new play state
		if (err) {
			SetHandleSize((Handle) gPlayState,
				      sizeof(PlayState));
			ZeroHandle(gPlayState);
			pState = *gPlayState;
		}
	}

	if (!pState->version) {
		//      Setup new play state.   
		DateTimeRec today;

		pState->version = kStateVersion;
		pState->adWidth = kAdWidth;
		pState->adHeight = kAdHeight;
		pState->reqInterval = kReqInverval;
		pState->histLength = kHistLength;

		GetTime(&today);
		(*gPlayState)->today = today;
		(*gPlayState)->getNextPlaylist = today;
	}
	UseResFile(oldResFile);
	return noErr;
}

/**********************************************************************
 * InsertAd - insert the current ad
 **********************************************************************/
void InsertAd(short adIdx)
{
	WindowPtr AdWinWP = GetMyWindowWindowPtr(AdWin);
	CGrafPtr AdWinPort = GetWindowPort(AdWinWP);
	FSSpec spec, *pImageSpec = nil;
	Str255 sURL, s;
	AdStatePtr pAdState, pPlayListState;
	uLong seconds;
	AdId adId;

	GetPlayListURLString(gPlayList, (*gPlayList)->ads[adIdx].srcURL,
			     sURL);
	GetCacheSpec(sURL, &spec, true);

	//      Remove current ad but don't draw to make for a smoother transition
	//      between ads
	SetEmptyClipRgn(AdWinPort);
	PeteDelete(ADpte, 0, 0x7fffffff);

	if (FileTypeOf(&spec) == 'TEXT') {
		//      Text file means HTML document
		UHandle text;
		char base[256];

		if (!Snarf(&spec, &text, 0)) {
			long tStart = 0;
			long tOffset = -1;
			StringPtr spot;

			//      Need to insert base ref. Use directory containing html source
			if (spot = PRIndex(sURL, '/'))
				*sURL = spot - sURL;
			ComposeRString(base, MHTML_INFO_TAG, sURL, "", 0,
				       "");
			Munger(text, 0, nil, 0, base + 1, *base);
			ComposeString(base, "\p<%r>",
				      htmlXHTMLDir + HTMLDirectiveStrn);
			Munger(text, 0, nil, 0, base + 1, *base);
			InsertHTML(text, &tStart, GetHandleSize(text),
				   &tOffset, ADpte,
				   kDontEnsureCR + kNoMargins);
			ZapHandle(text);
		}
	} else {
		//      Graphic file. Insert into pte
		uLong start;
		OSErr err;

		PeteGetTextAndSelection(ADpte, nil, &start, nil);	//      Current selection start should be zero
		err = PeteInsertChar(ADpte, -1, ' ', nil);
		ASSERT(err == noErr);	//      Error in ad window pte
		if (err == errAECorruptData) {
			//      The pte is corrupted. Let's just get a new one and try it again.
			PeteDispose(AdWin, ADpte);
			ADpte = nil;
			InitAdPte();
			err = PeteInsertChar(ADpte, -1, ' ', nil);
		}

		if (!err)
			PeteFileGraphicRange(ADpte, start, start + 1,
					     &spec,
					     fgDisplayInline +
					     fgCenterInWindow);
		pImageSpec = &spec;
	}
	PeteLock(ADpte, 0, 0x7fffffff, peModLock | peSelectLock);
	InfiniteClip(GetWindowPort(AdWinWP));
	InvalContent(AdWin);

	seconds = LocalDateTime();
	// If the ad window is open, and there's an ad in in, the old ad was just closed.
	if (IsWindowVisible(AdWinWP) && (gWasCurAd.server || gWasCurAd.ad)) {
		AuditAdClose(gWasCurAd.server, gWasCurAd.ad);
		gWasCurAd.server = gWasCurAd.ad = 0;
	}

	if ((pAdState = FindCurrentAdState())) {
		//      Notify link window about ad
		AdInfoPtr pAd;

		if (!(*gPlayState)->outOfAds)
			pAdState->lastDisplayTime = seconds;
		if (pAd = FindCurrentAd()) {
			GetPlayListString(gPlayList, pAd->title, s);
			AddAdToLinkHistory(pAd->adId,
					   GetPlayListURLString(gPlayList,
								pAd->
								clickURL,
								sURL), s,
					   pImageSpec);
		}
	}
	if (!(*gPlayState)->outOfAds
	    && (pPlayListState = FindPlayListState(gPlayList)))
		pPlayListState->lastDisplayTime = seconds;

	gWasCurAd = (*gPlayState)->curAd;
	adId = (*gPlayList)->ads[adIdx].adId;
	AuditAdOpen(adId.server, adId.ad);	// audit the fact that an ad was displayed
}

/************************************************************************
 * NewDayCheck - see if we've gone to a new day
 ************************************************************************/
static void NewDayCheck(Boolean withPrejuidice)
{
	static short todayMonth, todayDay;
	DateTimeRec dt;
	PlayStatePtr pState;

	GetTime(&dt);
	if (!todayMonth) {
		todayMonth = dt.month;
		todayDay = dt.day;
	} else if (!AdWin
		   && (dt.month != todayMonth || dt.day != todayDay))
		//      Need to reopen ad window
		OpenAdWindow();

	if (AdWin && gPlayState) {
		if (withPrejuidice
		    || DateCompare(&dt, &(*gPlayState)->today)) {
			//      New day
			short i;
			AdInfoPtr pAd;

			RollGlobalFacetime();	// record facetime now

			AdFailureChecks(&NoAdsRec);
			SaveNoAdsRec(&NoAdsRec);

			if ((*gPlayState)->outOfAds
			    || ((pAd = FindCurrentAd())
				&& pAd->type == kIsRunout))
				ResetAdSchedule();	// Start from beginning of ad list

			//      Reset all shownToday times
			pState = *gPlayState;
			for (i = 0; i < pState->numAds; i++)
				pState->stateAds[i].shownToday = 0;

			//      Set up daily facetime info
			pState->faceTimeWeek[pState->today.dayOfWeek - 1] =
			    pState->faceTimeToday;
			pState->faceTimeToday = 0;
			pState->adFaceTimeToday = 0;

			pState->today.month = dt.month;
			pState->today.day = dt.day;
			pState->today.dayOfWeek = dt.dayOfWeek;
			pState->today.year = dt.year;
			pState->outOfAds = false;
			pState->dirty = true;

			gDoNextAd = true;	//      New day, new ad
		}
	}
}

/************************************************************************
 * ReadNoAdsRec - read the no ads record
 ************************************************************************/
OSErr ReadNoAdsRec(NoAdsAuxPtr noAds)
{
	TOCHandle outTOC = GetSpecialTOC(OUT);

	if (outTOC) {
		BMD(&(*outTOC)->internalUseOnly, noAds, sizeof(*noAds));
		return noErr;
	}
	return fnfErr;
}

/************************************************************************
 * SaveNoAdsRec - save the no ads record
 ************************************************************************/
OSErr SaveNoAdsRec(NoAdsAuxPtr noAds)
{
	TOCHandle outTOC = GetSpecialTOC(OUT);

	if (outTOC) {
		BMD(noAds, &(*outTOC)->internalUseOnly,
		    sizeof((*outTOC)->internalUseOnly));
		TOCSetDirty(outTOC, true);
		return noErr;
	}
	return fnfErr;
}

/************************************************************************
 * AdFailureChecks - check for ad failure
 ************************************************************************/
OSErr AdFailureChecks(NoAdsAuxPtr noAds)
{
	Boolean wereFailing = noAds->adsAreFailing;
	Boolean wereSucceeding = noAds->adsAreSucceeding;

	// roll the mailcheck value
	noAds->checkedMail = noAds->checkedMailToday;
	noAds->checkedMailToday = false;

	// assume life is good
	noAds->adsAreFailing = false;
	noAds->adsAreSucceeding = true;

	// and leave it at that.  Ads don't fail anymore, because we are OUTTA HERE!
	// no more Sponsored mode.  No more Eudora business.  Hasta la vista, baby.
	return noErr;

#ifndef I_HATE_THE_BOX
	if (AdWin) {
		// determine if we're failing or not
		if (!gPlayState) {
			// if no playlists, we're failing
			noAds->adsAreFailing = true;
			noAds->adsAreSucceeding = false;
		} else {
			// did the user use the program significantly?
			Boolean hadSignificantFacetime =
			    (*gPlayState)->faceTimeToday >=
			    ((*gPlayState)->faceTimeQuota) / 2;
			// is the ad facetime insufficient?
			Boolean adTimeTooSmall;
			long adFaceTime =
			    (*gPlayState)->adFaceTimeToday + kQuotaSlop +
			    gNextAdSecs;

			// half the facetime quota?
			if (adFaceTime >= (*gPlayState)->faceTimeQuota / 2)
				adTimeTooSmall = false;
			// half the actual facetime?
			else if (adFaceTime >=
				 (*gPlayState)->faceTimeToday / 2)
				adTimeTooSmall = false;
			else
				// something is wrong...
				adTimeTooSmall = true;

			noAds->adsAreFailing = adTimeTooSmall
			    && noAds->checkedMail;
			noAds->adsAreSucceeding = hadSignificantFacetime
			    && !adTimeTooSmall;
		}

		// Adjust our counters
		if (noAds->adsAreFailing) {
			if (wereSucceeding)
				noAds->consecutiveDays = 1;
			else
				noAds->consecutiveDays++;
			if (noAds->deadbeatCounter <= kDeadbeatDays)
				noAds->deadbeatCounter++;
		} else if (noAds->adsAreSucceeding) {
			noAds->deadbeatCounter =
			    MIN(noAds->deadbeatCounter, kDeadbeatDays - 1);
			if (wereFailing)
				noAds->consecutiveDays = 1;
			else
				noAds->consecutiveDays++;
			if (noAds->consecutiveDays > kAdGracePeriod
			    && noAds->deadbeatCounter > 0) {
				noAds->deadbeatCounter--;
				noAds->consecutiveDays = 0;
			}
		}

#ifdef DEBUG
		ComposeLogS(LOG_PLIST, nil,
			    "\pAreAdsFailing----->: fail %p succ %p gPS %x adFace %d face %d ftq %d",
			    noAds->adsAreFailing ? YesStr : NoStr,
			    noAds->adsAreSucceeding ? YesStr : NoStr,
			    gPlayState, (*gPlayState)->adFaceTimeToday,
			    (*gPlayState)->faceTimeToday,
			    (*gPlayState)->faceTimeQuota);
		ComposeLogS(LOG_PLIST, nil,
			    "\pAreAdsFailing----->: checked %p dbc %d cd %d",
			    noAds->checkedMail ? YesStr : NoStr,
			    noAds->deadbeatCounter,
			    noAds->consecutiveDays);
#endif
	}
#endif
	return noErr;
}


/************************************************************************
 * NextAd - get the next ad
 ************************************************************************/
void NextAd(long secsDisplayed)
{
	PlayListHandle hPlayList;
	AdStatePtr pAdState, pPlayListState;

	if (AdWin && gPlayListQueue) {
		uLong curTime = LocalDateTime();
		Boolean found = false;
		// If we're out of ads, we display ads ignoring how long they've already displayed
		ValidAdFlags vaFlags =
		    (*gPlayState)->outOfAds ? vaCheckNormalAdOnly +
		    vaCheckRerun : vaCheckNormalAdOnly + vaCheckDayMax +
		    vaCheckShowForMax;
		short adIdx;

		if (!gPlayList)
			gPlayList = gPlayListQueue;

		//      Add time to previous ad (if we haven't run out of ads including runout ads)
		if ((gWasCurAd.server || gWasCurAd.ad)
		    && !(*gPlayState)->outOfAds) {
			if (pAdState = FindCurrentAdState()) {
				pAdState->shownToday++;
				pAdState->shownTotal += secsDisplayed;
				(*gPlayState)->adFaceTimeToday +=
				    secsDisplayed;
				AuditAdClose(gWasCurAd.server,
					     gWasCurAd.ad);
				gWasCurAd.server = gWasCurAd.ad = 0;
			}
			if (pPlayListState = FindPlayListState(gPlayList))
				pPlayListState->shownTotal +=
				    secsDisplayed;
		}
		(*gPlayState)->showCur = 0;

		if (!(*gPlayState)->faceTimeQuota
		    || (*gPlayState)->faceTimeToday <
		    (*gPlayState)->faceTimeQuota
		    || (*gPlayState)->outOfAds) {
			//      Find ad to display
			if ((*gPlayState)->schedType == kRandom) {
				//      Find next ad randomly
				unsigned short randomCount = 0;
				Accumulator a;

				//      Build list of valid ads to select from
				AccuInit(&a);
				for (hPlayList = gPlayListQueue; hPlayList;
				     hPlayList = (*hPlayList)->next) {
					for (adIdx = 0;
					     adIdx < (*hPlayList)->numAds;
					     adIdx++) {
						if (ValidAd
						    (hPlayList, adIdx,
						     curTime, vaFlags)) {
							AccuAddPtr(&a,
								   &
								   (*hPlayList)->
								   ads
								   [adIdx].
								   adId,
								   sizeof
								   (AdId));
							randomCount++;
						}
					}
				}
				if (randomCount) {
					unsigned short randomValue =
					    ((unsigned short) Random()) %
					    randomCount;
					FindAd(((AdId *) *
						a.data)[randomValue],
					       &adIdx, &gPlayList);
					found = true;
				}
				AccuZap(a);
			} else {
				//      Find next ad sequentially
				short startAdIdx;
				short wrapCount;

				if (FindCurrentAdIdx(&adIdx))
					adIdx++;
				else {
					//      Couldn't find current ad, start with first ad
					adIdx = 0;
					gPlayList = gPlayListQueue;
				}
				startAdIdx = adIdx - 1;
				hPlayList = gPlayList;
				wrapCount = 0;
				do {
					while (adIdx < (*hPlayList)->numAds
					       && !found) {
						if (adIdx == startAdIdx
						    && hPlayList ==
						    gPlayList)
							goto NotFound;	//      Went though all ads. Didn't find any valid ones
						if (ValidAd
						    (hPlayList, adIdx,
						     curTime, vaFlags)) {
							//      Use it!
							found = true;
							gPlayList =
							    hPlayList;
						} else
							adIdx++;
					}
					if (!found) {
						hPlayList = (*hPlayList)->next;	//      Try next playlist
						if (!hPlayList) {
							//      Last playlist, wrap around to first
							if (wrapCount)
								goto NotFound;	//      We're already gone this route and haven't found a thing
							wrapCount++;
							hPlayList = gPlayListQueue;	//      Wrap around
						}
						adIdx = 0;
						if (startAdIdx < 0)
							startAdIdx = 0;	// Couldn't find current ad, started with first ad
					}
				} while (!found);
			}
		}

		if (found) {
			NewAd(adIdx);
		} else {
			AdInfoPtr pAd;
		      NotFound:
			//      We are out of ads or have reached our quota
			if ((*gPlayState)->outOfAds) {
				// Unlikely this will happen, but we just can't find
				// any ads we can display. Go to black.
				ShowBlackTimeImage();
				gNextAdSecs = 0;	//      No more ads until tomorrow
				gBlackTime = -1;	//      Negative value means display black until an ad shows up
			}
			//      Out of regular ads. Do we have a runout ad?
			else if (pAd =
				 FindAdType(kIsRunout, &adIdx,
					    &gPlayList)) {
				short showFor = pAd->showFor;
				NewAd(adIdx);
				if (pAdState = FindCurrentAdState()) {
					// Charge runout ad for full display time
					pAdState->shownToday++;
					pAdState->shownTotal += showFor;
				}
			} else {
				//      No regular ads and no runouts. Let's start showing
				//      regular ads again at no charge.
				(*gPlayState)->outOfAds = true;
				gNextAdSecs = 1;	// Do this instead of goto top of this function
				ResetAdSchedule();	// Start from beginning of ad list
			}
		}

		UL(gPlayList);
#ifdef DEBUG
		if (RunType == Debugging
		    && FindTextLo(nil, "\pPlaylist Info"))
			DumpPlaylists(false);
#endif
	}
}

/************************************************************************
 * ValidAd - can this ad be used?
 ************************************************************************/
static Boolean ValidAd(PlayListHandle hPlayList, short adIdx,
		       uLong curTime, ValidAdFlags flags)
{
	AdInfo *pAd = &(*hPlayList)->ads[adIdx];
	AdStatePtr pAdState;

	if (pAd->adId.server || pAd->adId.ad)
		if (pAdState = FindAdState(pAd->adId, nil))
			if (!(flags & vaCheckNormalAdOnly) || !pAd->type)
				if (!(flags & vaCheckDayMax)
				    || !pAd->dayMax
				    || pAdState->shownToday < pAd->dayMax)
					if (!(flags & vaCheckShowForMax)
					    || !pAd->showForMax
					    || pAdState->shownTotal <
					    pAd->showForMax)
						if (!(flags & vaCheckRerun)
						    || !(*gPlayState)->
						    rerunInterval
						    || curTime -
						    pAdState->
						    lastDisplayTime <
						    (*gPlayState)->
						    rerunInterval) {
							//      Not done displaying this ad.
							//      See if we are in range
							if ((pAd->
							     startDate == 0
							     || pAd->
							     startDate <=
							     curTime)
							    && (pAd->
								endDate ==
								0
								|| pAd->
								endDate >=
								curTime))
								return
								    true;
						}
	return false;
}

/************************************************************************
 * NewAd - use a new ad
 ************************************************************************/
static void NewAd(short adIdx)
{
	Str255 sURL;
	FSSpec spec;
	AdInfo *pAd = &(*gPlayList)->ads[adIdx];
	AdId adId;

	(*gPlayState)->curAd = pAd->adId;
	gNextAdSecs = pAd->showFor;
	(*gPlayState)->showCur = pAd->showFor;
	adId = pAd->adId;
	(*gPlayState)->curPlayList = (*gPlayList)->playListId;
	GetPlayListURLString(gPlayList, pAd->srcURL, sURL);
	GetCacheSpec(sURL, &spec, true);

#ifdef DEBUG
	LogAd(ComposeString(sLog, "\pLoading: %p", sURL));
#endif

	gBlackTime = gNextBlackTime;
	if (gBlackTime < pAd->blackBefore)
		gBlackTime = pAd->blackBefore;
	gNextBlackTime = pAd->blackAfter;

#ifdef DEBUG
	if (gBlackTime) {
		LogAd(ComposeString(sLog, "\pBlack for: %d", gBlackTime));
	} else {
		LogAd(ComposeString(sLog, "\pShow for: %d", gNextAdSecs));
	}
#endif

	FaceMeasureReset(gFaceMeasure);

	if (!FSpExists(&spec))
		InsertAd(adIdx);
	else {
		DownloadListHandle hDownload;

		//      Don't have ad in cache. Pre-cache all ads
		PreFetchAds();

		//      Put our ad first in list and mark it for display
		for (hDownload = gAdDownloadList; hDownload;
		     hDownload = (*hDownload)->next) {
			if (SameAd(&adId, &(*hDownload)->adId)) {
				LL_Remove(gAdDownloadList, hDownload,
					  (DownloadListHandle));
				LL_Push(gAdDownloadList, hDownload);
				(*hDownload)->display = true;
				break;
			}
		}
		gNextAdSecs += 60;	//      Give us a minute to download this thing, otherwise go on to the next ad
	}

	(*gPlayState)->dirty = true;

#ifdef DEBUG
	if (BUG4) {
		//      Make those ads display much faster
		if (gNextAdSecs) {
			gNextAdSecs /= kAdScheduleAccelerator;
			if (!gNextAdSecs)
				gNextAdSecs = 1;
		}
		if (gBlackTime) {
			gBlackTime /= kAdScheduleAccelerator;
			if (!gBlackTime)
				gBlackTime = 1;
		}
	}
#endif
}

/************************************************************************
 * RequestPlaylist - download a play list
 ************************************************************************/
void RequestPlaylist(void)
{
	FSSpec spec;
	long reference;
	HTTPinfo httpStuff;
	Accumulator a;
	Handle text;
	Str255 s, s2;
	Handle hPastry;
	Rect rScreen;
	PlayListHandle hPlayList;
	short i;
	long faceTimeLeft;
	OSErr err;
	unsigned char key[kCheckSumSize];
	short depth;

	if (gPLDownloadInProgress)
		return;		//      We're already downloading a playlist

	RollGlobalFacetime();	// make sure our counters are up-to-date

	Zero(httpStuff);
	httpStuff.post = true;
	PCopy(httpStuff.sContentType, "\ptext/xml");
	PCopy(httpStuff.sMessageType, "\pCall");

	//      Build request info
	AccuInit(&a);
	XMLNoIndent();
	AccuAddStr(&a, "\p<?xml version=\"1.0\"?>");
	AccuAddCRLF(&a);
	AccuAddCRLF(&a);
	AccuAddTagLine(&a, PlayListKeywords[kClientUpdateTkn], false);
	XMLIncIndent();

	//      userAgent
	AccuAddXMLObject(&a, PlayListKeywords[kUserAgentTkn],
			 ComposeString(s, "\pEudora/%d.%d.%d.%d (MacOS)",
				       MAJOR_VERSION, MINOR_VERSION,
				       INC_VERSION, BUILD_VERSION));

	//      locale
	AccuAddXMLWithAttr(&a, PlayListKeywords[kLocaleTkn],
			   ComposeString(s, "\planguage=\"%p\"",
					 GetLanguageCode(s2)), true);

	//      clientMode where 0=adware, 1=light, 2=paid
	AccuAddXMLObject(&a, PlayListKeywords[kClientModeTkn],
			 IsAdwareMode()? "\p0" : IsPayMode()? "\p2" :
			 IsProfileDeadbeatUser()? "\p3" : "\p1");

	//      playlists and ad id's
	if (gPlayListQueue) {
		PlayListHandle hNextPlayList;
		Handle hKeepAdList = NuHandleClear((*gPlayState)->numAds);

		for (hPlayList = gPlayListQueue; hPlayList;
		     hPlayList = hNextPlayList) {
			uLong curTime = LocalDateTime();
			uLong expiredTime =
			    curTime -
			    (*gPlayState)->histLength * 24 * 60 * 60;
			short i;
			Boolean foundKeeper = false;

			GetPlayListString(hPlayList,
					  (*hPlayList)->playListIDString,
					  s2);
			AccuAddXMLWithAttr(&a, PlayListKeywords[kPlayListTkn], ComposeString(s, "\pid=\"%p\"", s2), false);	//      playlist and id
			//      Do all ads
			XMLIncIndent();
			for (i = 0; i < (*hPlayList)->numAds; i++) {
				long active =
				    ValidAd(hPlayList, i, curTime,
					    vaCheckShowForMax) ? 1 : 0;
				AdId adId = (*hPlayList)->ads[i].adId;
				short adStateIdx;
				AdStatePtr pAdState;

				//      Report only ads that aren't too old
				if (adId.server || adId.ad)
					if ((pAdState =
					     FindAdState(adId,
							 &adStateIdx))
					    && pAdState->lastDisplayTime >
					    expiredTime
					    && (!(*hPlayList)->ads[i].
						endDate
						|| (*hPlayList)->ads[i].
						endDate > expiredTime)) {
						foundKeeper = true;
						if (hKeepAdList)
							(*hKeepAdList)
							    [adStateIdx] =
							    true;
						ComposeString(s,
							      "\pid=\"%d.%d\" active=\"%d\"",
							      adId.server,
							      adId.ad,
							      active);
						switch ((*hPlayList)->
							ads[i].type) {
						case kIsButton:
							PCat(s,
							     "\p isButton=\"1\"");
							if ((*gPlayState)->
							    stateAds
							    [adStateIdx].
							    deleted)
								//      deleted toolbar button
								PCat(s,
								     "\p deleted=\"1\"");
							break;
						case kIsSponsor:
							PCat(s,
							     "\p isSponsor=\"1\"");
							break;
						case kIsRunout:
							PCat(s,
							     "\p isRunout=\"1\"");
							break;
						}
						AccuAddXMLWithAttr(&a,
								   PlayListKeywords
								   [kEntryTkn],
								   s,
								   true);
					}
#ifdef I_EVER_FIX_THE_ENDDATE_BUG
					else {
						if (pAdState)
							ASSERT(!
							       (*hPlayList)->
							       ads[i].
							       endDate
							       ||
							       pAdState->
							       lastDisplayTime
							       <
							       (*hPlayList)->
							       ads[i].
							       endDate);
					}
#endif
			}

			hNextPlayList = (*hPlayList)->next;

			if (!foundKeeper) {
				//      There are no ads in this playlist that are less than "histLength" days old
				//      Get rid of it
				REMOVE_AND_DESTROY(hPlayList);
			} else {
				AdId adId;
				short playListStateIdx;
				AdStatePtr pPlayListState;

				adId.server = kPlayListServerId;
				adId.ad = (*hPlayList)->playListId;
				if (pPlayListState =
				    FindAdState(adId, &playListStateIdx))
					if (hKeepAdList)
						(*hKeepAdList)
						    [playListStateIdx] =
						    true;
			}
			XMLDecIndent();
			AccuAddTagLine(&a, PlayListKeywords[kPlayListTkn],
				       true);
		}

		if (hKeepAdList) {
			PrunePlayState(hKeepAdList);
			ZapHandle(hKeepAdList);
		}
	}

	//      screen size
	rScreen = GetScreenBounds();
	if (AdWin) {
		SetPort_(GetWindowPort(GetMyWindowWindowPtr(AdWin)));	//      RectDepth needs this
		depth = RectDepth(&AdWin->contR);
	} else
		depth = (*(*GetMainDevice())->gdPMap)->pixelSize;
	AccuAddXMLWithAttr(&a, PlayListKeywords[kScreenTkn],
			   ComposeString(s,
					 "\pheight=\"%d\" width=\"%d\" depth=\"%d\"",
					 RectHi(rScreen), RectWi(rScreen),
					 depth), true);

	//      distributorID
	if (text = GetDistributorID()) {
		AccuAddXMLObjectHandle(&a, PlayListKeywords[kDistIDTkn],
				       text);
		ZapHandle(text);
	}

	//      faceTime for every day of week
	faceTimeLeft = 0;
	*s = 0;
	for (i = 0; i < 7; i++) {
		long time = (*gPlayState)->faceTimeWeek[i];

		// if today's value, take larger of last week and this
		if (i == (*gPlayState)->today.dayOfWeek - 1
		    && (*gPlayState)->faceTimeToday >
		    (*gPlayState)->faceTimeWeek[i])
			time = (*gPlayState)->faceTimeToday;

		if (i)
			PCatC(s, ',');

		NumToString(time / 60, s2);
		PCat(s, s2);
	}
	AccuAddXMLObject(&a, PlayListKeywords[kFaceTimeTkn], s);

	//      faceTimeLeft
	faceTimeLeft = TotalFaceTimeLeft();

	NumToString(faceTimeLeft / 60, s);
	AccuAddXMLObject(&a, PlayListKeywords[kFaceTimeLeftTkn], s);

	//      faceTimeUsedToday
	NumToString((*gPlayState)->adFaceTimeToday / 60, s);
	AccuAddXMLObject(&a, PlayListKeywords[kFaceTimeUsedTodayTkn], s);

	//      pastry (cookie)
	if (hPastry =
	    GetResourceFromFile(kStateResType, kPastryResId,
				SettingsRefN)) {
		if (!PastryIsStale(hPastry))
			AccuAddXMLWithAttrPtr(&a,
					      PlayListKeywords[kPastryTkn],
					      LDRef(hPastry),
					      GetHandleSize(hPastry),
					      true);
		UL(hPastry);
		ReleaseResource(hPastry);
	}

	//      user profile
	if (text = GetProfileData()) {
		AccuAddXMLObjectHandle(&a,
				       PlayListKeywords[kUserProfileTkn],
				       text);
	}

	//      playListVersion
	NumToString(kPlayListVersion, s);
	AccuAddXMLObject(&a, PlayListKeywords[kPlayListVersionTkn], s);

#ifdef LOGINFO
	//      Send date/time and Id info for server log
	{
		Str255 sDate, sTime;
		long secs = LocalDateTime();

		IUTimeString(secs, false, sTime);
		IUDateString(secs, shortDate, sDate);
		AccuAddXMLObject(&a, PlayListKeywords[kDateAndTime],
				 ComposeString(s, "\p%p, %p", sDate,
					       sTime));
		AccuAddXMLObject(&a, PlayListKeywords[kUserId],
				 GetPref(s, PREF_POP));
	}
#endif

	XMLDecIndent();
	AccuAddTagLine(&a, PlayListKeywords[kClientUpdateTkn], true);
	AccuTrim(&a);

	//      Do not dispose of data in handle. DownloadURL will dispose of it
	httpStuff.hRequestData = a.data;
	NewTempSpec(Root.vRef, Root.dirId, nil, &spec);
	gPLDownloadInProgress = true;
	*s = 0;
#ifdef DEBUG
	GetRString(s, DEBUG_PLAYLIST_URL);
#endif
	if (!*s)
		PCopy(s, PlayListURL);
	s[*s + 1] = 0;
	//      Include checksum
	SetupForDisplay(LDRef(a.data), GetHandleSize(a.data), key, false);
	Bytes2Hex(key, sizeof(key), httpStuff.sCheckSum + 1);
	*httpStuff.sCheckSum = 32;
	err =
	    DownloadURL(s + 1, &spec, nil, FinishedPlayListDownload,
			&reference, &httpStuff);

	if (err) {
		gPlayListDownloadFail = true;
		gPLDownloadInProgress = false;
	}

	//      Now is a good time to tidy up the cache
	CleanupAdCache(false);

	// Remember when we did this
	gPlayListFetchGMT = GMTDateTime();
}

/************************************************************************
 * GetAdSpec - get filespec for ad
 ************************************************************************/
static OSErr GetAdSpec(FSSpecPtr spec, AdId adId)
{
	short adIdx;
	Str255 sURL;
	AdInfoPtr pAd;
	PlayListHandle hPlayList;

	if (pAd = FindAd(adId, &adIdx, &hPlayList)) {
		GetPlayListURLString(hPlayList, pAd->srcURL, sURL);
		GetCacheSpec(sURL, spec, true);
		return noErr;
	} else
		return fnfErr;
}


/************************************************************************
 * MakePastry - save a pastry (cookie)
 ************************************************************************/
static void SavePastry(Ptr pPastry, long len)
{
	short oldRes = CurResFile();

	if (SettingsRefN)
		UseResFile(SettingsRefN);
	ZapResourceLo(kStateResType, kPastryResId, true);
	AddPResource(pPastry, len, kStateResType, kPastryResId, nil);
	UseResFile(oldRes);
}

/************************************************************************
 * BuildPlayList - parse play list
 ************************************************************************/
static OSErr BuildPlayList(UHandle text)
{
#define kMaxTokenStack 64
	PlayList list;
	TokenInfo tokenInfo;
	short tokenType;
	Str255 sToken;
	short tokenStack[kMaxTokenStack];
	short stackIdx = -1;
	Accumulator accuKeywordList;
	OSErr err;
	AdInfo entryInfo;
	short i;
	PlayStatePtr pState;
	DateTimeRec nextPlaylist;
	Accumulator aStrings, aState, aPlayList;
	PlayListHandle hPlayList;
	long level;		// keep track of nag level

	//      Initialize tokenizer
	tokenInfo.pText = LDRef(text);
	tokenInfo.size = GetHandleSize(text);
	tokenInfo.offset = 0;

	//      Add our keywords to keyword list
	if (err = AccuInit(&accuKeywordList))
		return err;
	AccuAddPtr(&accuKeywordList, &i, sizeof(short));	//      Make room for count
	for (i = 0; *PlayListKeywords[i]; i++) {
		err =
		    AccuAddPtr(&accuKeywordList, PlayListKeywords[i],
			       *PlayListKeywords[i] + 1);
		if (err)
			return err;
	}
	*(short *) *accuKeywordList.data = i;
	tokenInfo.aKeywords = &accuKeywordList;
	ZapHandle(gDownloadErrString);

	AccuInitWithHandle(&aState, (Handle) gPlayState);

	//      Parse loop
	while ((tokenType = GetNextToken(&tokenInfo)) != kTokenDone) {
		TokenToString(&tokenInfo, sToken);

		//ComposeLogS(LOG_PLIST,nil,"\pxml token: %d %p",tokenType,sToken);

		if (tokenType == kContent) {
			//      Get element content
			long zone;

			if (stackIdx >= 1 &&
			    tokenStack[stackIdx - 1] == kPlayListTkn) {
				//      Defining play list elements
				switch (tokenStack[stackIdx]) {
				case kPlayListIdTkn:
					list.playListId = Hash(sToken);
					AddStringToHandle(&list.
							  playListIDString,
							  &aStrings,
							  sToken);
					break;
				case kMixAlgorithmTkn:
					list.mixType =
					    StringSame(PlayListKeywords
						       [kBlockTkn],
						       sToken) ? kBlock :
					    kMix;
					break;
				case kBlockScheduleAlgorithmTkn:
					list.blockScheduleType =
					    StringSame(PlayListKeywords
						       [kRandomTkn],
						       sToken) ? kRandom :
					    kLinear;
					break;
				case kClickBaseTkn:
					TokenToURL(&list.clickBase,
						   &aStrings, &tokenInfo);
					break;
				}
			} else if (stackIdx >= 1 &&
				   tokenStack[stackIdx - 1] ==
				   kClientInfoTkn) {
				//      Defining client info elements
				switch (tokenStack[stackIdx]) {
				case kReqIntervalTkn:
					(*gPlayState)->reqInterval =
					    PStrToNum(sToken);
					if ((*gPlayState)->reqInterval >
					    kMaxReqInterval)
						(*gPlayState)->
						    reqInterval =
						    kMaxReqInterval;
					break;
				case kHistLengthTkn:
					(*gPlayState)->histLength =
					    PStrToNum(sToken);
					break;
				case kScheduleTkn:
					(*gPlayState)->schedType =
					    StringSame(PlayListKeywords
						       [kRandomTkn],
						       sToken) ? kRandom :
					    kLinear;
					break;
				case kWidthTkn:
					(*gPlayState)->adWidth =
					    PStrToNum(sToken);
					break;
				case kHeightTkn:
					(*gPlayState)->adHeight =
					    PStrToNum(sToken);
					break;
				case kFaceTimeQuotaTkn:
					(*gPlayState)->faceTimeQuota =
					    PStrToNum(sToken) * 60;
					break;	//      Convert to seconds
				case kRerunIntervalTkn:
					(*gPlayState)->rerunInterval =
					    PStrToNum(sToken) * 24 * 60 *
					    60;
					break;	//      Convert days to seconds
				case kNagTkn:
					HandleNag(level, sToken);
					break;
				case kClientModeTkn:
					HandleClientMode(sToken);
					break;
				case kUserProfileTkn:
					HandleUserProfile(sToken);
					break;
				}
			} else if (stackIdx >= 2 &&
				   tokenStack[stackIdx - 2] == kPlayListTkn
				   && tokenStack[stackIdx - 1] ==
				   kEntryTkn) {
				//      Defining ad entry elements
				switch (tokenStack[stackIdx]) {
				case kShowForTkn:
					entryInfo.showFor =
					    PStrToNum(sToken);
					break;
				case kDayMaxTkn:
					entryInfo.dayMax =
					    PStrToNum(sToken);
					break;
				case kShowForMax:
					entryInfo.showForMax =
					    PStrToNum(sToken);
					list.totalShowMax +=
					    entryInfo.showForMax;
					break;
				case kSrcTkn:
					TokenToURL(&entryInfo.srcURL,
						   &aStrings, &tokenInfo);
					break;
				case kBlackBeforeTkn:
					entryInfo.blackBefore =
					    PStrToNum(sToken);
					break;
				case kBlackAfterTkn:
					entryInfo.blackAfter =
					    PStrToNum(sToken);
					break;
				case kStartDateTkn:
					entryInfo.startDate =
					    BeautifyDate(sToken, &zone);
					entryInfo.startDate += zone;
					break;
				case kEndDateTkn:
					entryInfo.endDate =
					    BeautifyDate(sToken, &zone);
					entryInfo.endDate += zone;
					break;
				case kAdIdTkn:
					ParseAdId(sToken, &entryInfo.adId);
					break;
				case kTitleTkn:
					AddStringToHandle(&entryInfo.title,
							  &aStrings,
							  sToken);
					break;
				case kClickTkn:
					TokenToURL(&entryInfo.clickURL,
						   &aStrings, &tokenInfo);
					break;
				}
			} else if (stackIdx >= 1 &&
				   tokenStack[stackIdx - 1] == kFaultTkn) {
				//      Defining fault elements
				switch (tokenStack[stackIdx]) {
				case kFaultCodeTkn:
					(*gPlayState)->errCode =
					    gDownloadErrCode =
					    PStrToNum(sToken);
					break;
				case kFaultStringTkn:
					gDownloadErrString =
					    NewString(sToken);
					Log(LOG_PLIST, sToken);
					break;
				}
			}
		} else {
			//      Process a tag
			short tokenIdx, attrIdx;
			long value;

			tokenIdx =
			    GetTokenIdx(&accuKeywordList, sToken) - 1;
			switch (tokenType) {
			case kElementTag:
			case kEmptyElementTag:
				switch (tokenIdx) {
				case kEntryTkn:
					//      New ad entry

					Zero(entryInfo);
					while (attrIdx =
					       GetNumAttribute(&tokenInfo,
							       &value)) {
						switch (attrIdx) {
						case kIsButtonTkn:
							if (value)
								entryInfo.
								    type =
								    kIsButton;
							break;
						case kIsSponsorTkn:
							if (value)
								entryInfo.
								    type =
								    kIsSponsor;
							break;
						case kIsRunoutTkn:
							if (value)
								entryInfo.
								    type =
								    kIsRunout;
							break;
						}
					}
					break;

				case kSrcTkn:
					if (GetAttribute
					    (&tokenInfo,
					     sToken) == kCheckSum
					    && *sToken == 32)
						//      Checksum for src
						Hex2Bytes(sToken + 1, 32,
							  entryInfo.
							  checksum);
					break;

				case kPlayListTkn:
					//      New playlist
					Zero(list);
					gPlayList =
					    NuHandleClear(sizeof
							  (PlayList));
					AccuInitWithHandle(&aPlayList,
							   (Handle)
							   gPlayList);
					if (!gPlayList)
						return MemError();
					//      Accumulator for strings
					AccuInit(&aStrings);
					AccuAddChar(&aStrings, 0);	//      We don't want any strings with offset zero
					//      Set up some default values
					list.version = kPlayListVers;
					break;

				case kFlushTkn:
					if (tokenStack[stackIdx] ==
					    kClientInfoTkn)
						Flush(&tokenInfo);
					break;

				case kPastryTkn:
					if (tokenStack[stackIdx] ==
					    kClientInfoTkn)
						SavePastry(tokenInfo.
							   pText +
							   tokenInfo.
							   attrStart,
							   tokenInfo.
							   attrLen);
					break;

				case kNagTkn:
					level = 0;
					if (GetAttribute
					    (&tokenInfo,
					     sToken) == kLevelTkn)
						StringToNum(sToken,
							    &level);
					break;

				case kResetTkn:
					//      The server has determined that we are insane
					//      Reset data and request a new playlist
					ResetAdState();
					AccuInitWithHandle(&aState, (Handle) gPlayState);	//      gPlayState has been resized
					break;

				case kClientInfoTkn:
					//      Set up default client info in case some values
					//      aren't included
					(*gPlayState)->reqInterval =
					    kReqInterval;
					(*gPlayState)->histLength =
					    kHistLength;
					(*gPlayState)->schedType =
					    kSchedType;
					(*gPlayState)->adWidth = kAdWidth;
					(*gPlayState)->adHeight =
					    kAdHeight;
					(*gPlayState)->faceTimeQuota =
					    kFaceTimeQuota * 60;
					(*gPlayState)->rerunInterval =
					    kRerunInterval * 24 * 3600;
					break;
				}
				if (tokenType == kElementTag) {
					//      New element. Push token on stack
					//      Don't put empty tags on stack. There is no end-tag
					if (stackIdx < kMaxTokenStack - 1)
						tokenStack[++stackIdx] =
						    tokenIdx;
				}
				break;

			case kEndTag:
				//      End of element. Pop off stack. Actually search whole stack
				//      in case document is not well-formed.
				while (stackIdx >= 0) {
					//      
					if (tokenStack[stackIdx--] ==
					    tokenIdx)
						break;
				}

				switch (tokenIdx) {
				case kEntryTkn:
					//      Save ad entry
					if (!AccuAddPtr
					    (&aPlayList, &entryInfo,
					     sizeof(entryInfo))) {
						list.numAds++;
						DeleteAd(entryInfo.adId);
					}
					break;

				case kPlayListTkn:
					//      Save playlist
					if (hPlayList =
					    FindPlayList(list.
							 playListId)) {
						//      We're replacing this playlist. Delete it.
						REMOVE_AND_DESTROY
						    (hPlayList);
					}
					AccuTrim(&aPlayList);
					list.strOffset =
					    GetHandleSize_(gPlayList);
					**gPlayList = list;	//      Save final playlist info
					//      Add strings to end of playlist
					AccuTrim(&aStrings);
					if (HandAndHand
					    (aStrings.data,
					     (Handle) gPlayList))
						return MemError();
					AccuZap(aStrings);
					//      Save playlist resource
					AddMyResource_(gPlayList,
						       kPlayListResType,
						       UniqueID
						       (kPlayListResType),
						       "");
					UpdateResFile(SettingsRefN);
					LL_Queue(gPlayListQueue, gPlayList,
						 (PlayListHandle));
					AddStates(gPlayList, &aState,
						  true);
					break;
				}
				break;
			}
		}
	}

	SecondsToDate(LocalDateTime() +
		      ((*gPlayState)->reqInterval ? (*gPlayState)->
		       reqInterval : kReqInverval) * 60L * 60L,
		      &nextPlaylist);

	pState = *gPlayState;
	pState->getNextPlaylist = nextPlaylist;
	pState->dirty = true;
	gAdFetchCount = 0;
	pState->adsNotFetched = 0;
	gDownloadErrCode = pState->errCode;
	AccuTrim(&aState);	//      Don't dispose of this accumulator!
	SavePlayStatus();
	SetCurrentPlayList();

	UL(text);
#ifdef DEBUG
	SetupAdDebugMenu();
#endif
	return noErr;
}

/************************************************************************
 * ResetAdState - throw away all ad data and request a new playlist
 ************************************************************************/
static void ResetAdState(void)
{
	//      Delete playlists
	while (gPlayListQueue) {
		PlayListHandle hPlayList;

		hPlayList = gPlayListQueue;
		REMOVE_AND_DESTROY(hPlayList);
	}
	//      Reinialize ad state
	SetupAdState(true);

	//      Get rid of all cached ads
	CleanupAdCache(true);

	//      Nothing to display right now
	ShowBlackTimeImage();
	gBlackTime = -1;	//      Negative value means display black until an ad shows up
	RollGlobalFacetime();
	RequestPlaylist();
#ifdef DEBUG
	{
		MenuHandle mh;

		if (mh = GetMHandle(AD_SELECT_HIER_MENU));
		TrashMenu(mh, gAdsMenuAdsStart);
	}
#endif
}

/************************************************************************
 * DeleteAd - delete this ad from every playlist
 ************************************************************************/
static void DeleteAd(AdId adId)
{
	short idx;
	PlayListHandle hPlayList;

	while (FindAd(adId, &idx, &hPlayList)) {
		PlayListPtr pPlayList = *hPlayList;

		pPlayList->numAds--;
		if (pPlayList->numAds) {
			long playListLen =
			    GetHandleSize((Handle) hPlayList);
			long moveBytes =
			    (long) *hPlayList + playListLen -
			    (long) &pPlayList->ads[idx + 1];

			pPlayList->strOffset -= sizeof(AdInfo);
			BlockMove(&pPlayList->ads[idx + 1],
				  &pPlayList->ads[idx], moveBytes);
			SetHandleSize((Handle) hPlayList,
				      playListLen - sizeof(AdInfo));
			ChangedResource((Handle) hPlayList);
		} else {
			//      Nothing left in this playlist. Let's get rid of it.
			REMOVE_AND_DESTROY(hPlayList);
		}
	}
#ifdef DEBUG
	SetupAdDebugMenu();
#endif
}

/************************************************************************
 * AddStates - make sure all ads are in playstate
 ************************************************************************/
static void AddStates(PlayListHandle hPlayList, AccuPtr aState,
		      Boolean init)
{
	AdId adId;
	AdState state;
	short i;
	AdStatePtr pAdState;

	for (i = -1; i < (*hPlayList)->numAds; i++) {
		if (i == -1) {
			//      Start with playlist state
			adId.server = kPlayListServerId;
			adId.ad = (*hPlayList)->playListId;
		} else
			adId = (*hPlayList)->ads[i].adId;

		Zero(state);
		state.adId = adId;
		state.lastDisplayTime = LocalDateTime();

		if (pAdState = FindAdState(adId, nil)) {
			//      Reinitialize ad state
			if (init) {
				*pAdState = state;
				(*gPlayState)->dirty = true;
			}
		} else {
			//      Not already there. Add it.
			if (!AccuAddPtr(aState, &state, sizeof(AdState))) {
				(*gPlayState)->numAds++;
				(*gPlayState)->dirty = true;
			}
		}
	}
}

/************************************************************************
 * DisposePlaylists - dispose of play lists
 ************************************************************************/
static void DisposePlaylists(void)
{
	PlayListHandle hPlayList, nextPlaylist;

	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = nextPlaylist) {
		nextPlaylist = (*hPlayList)->next;
		ReleaseResource((Handle) hPlayList);
	}
	gPlayListQueue = nil;
	gPlayList = nil;
}

/************************************************************************
 * PeteIsInAdWindow - dispose of play list
 ************************************************************************/
Boolean PeteIsInAdWindow(PETEHandle pte)
{
	WindowPtr pteWinWP = GetMyWindowWindowPtr((*PeteExtra(pte))->win);
	return pte && GetWindowKind(pteWinWP) == AD_WIN;
}

/************************************************************************
 * ValidAdImage - return error if image is in ad window and has invalid checksum
 ************************************************************************/
OSErr ValidAdImage(PETEHandle pte, Handle hData)
{
	OSErr err = noErr;

	if (PeteIsInAdWindow(pte))
		if (err = CheckAd((*gPlayState)->curAd, hData, nil))
			//      Bad ad. Display the next ad
			gDoNextAd = true;
	return err;
}

/************************************************************************
 * CheckAd - return error if ad has invalid checksum
 ************************************************************************/
static OSErr CheckAd(AdId adId, Handle hData, FSSpec * spec)
{
	OSErr err = noErr;
	AdInfoPtr pAd;
	PlayListHandle hPlayList;

	if (pAd = FindAd(adId, nil, &hPlayList)) {
		short i;
		Boolean hasChecksum = false;
		AdInfo ad = *pAd;

		for (i = 0; i < kCheckSumSize; i++)
			if (ad.checksum[i]) {
				hasChecksum = true;
				break;
			}

		if (hasChecksum)	// If no checksum specified, ad is OK
		{
			Boolean disposeData = false;

			if (!hData && spec) {
				Snarf(spec, &hData, 0);
				disposeData = true;
			}
			if (hData) {
				err =
				    SetupForDisplay(LDRef(hData),
						    GetHandleSize(hData),
						    ad.checksum, true);
				if (err) {
					//      Bad ad.
					if (IsChecksumFailAgain(adId))
						//      We've redownloaded and failed again. Get rid of this ad.
						DeleteAd(adId);
					else {

						//      Bad ad. Delete it and download again
						FSSpec delSpec;
						Handle url;

						if (spec)
							delSpec = *spec;
						else
							GetAdSpec(&delSpec,
								  adId);
						FSpDelete(&delSpec);

						//      Download ad again.
						if (url =
						    GetPlayListURL
						    (hPlayList,
						     ad.srcURL)) {
							if (!QueueAdDownload(url, ad.adId, ad.type))
								ZapHandle(url);	//      Error, didn't queue, is duplicate
						}
					}
				}
				if (disposeData)
					ZapHandle(hData);
			}
		}
	}
	return err;
}

/************************************************************************
 * IsChecksumFailAgain - has this ad failed a checksum previously?
 ************************************************************************/
static Boolean IsChecksumFailAgain(AdId adId)
{
	static AdId **hFailList;
	Boolean found = false;

	if (!hFailList)
		hFailList = NuHandle(0);
	else {
		short i, n = HandleCount(hFailList);

		for (i = 0; i < n; i++)
			if (SameAd(&adId, &(*hFailList)[i]))
				return true;
	}
	PtrAndHand(&adId, (Handle) hFailList, sizeof(adId));
	return false;
}

/************************************************************************
 * IsAdInPlaylist - is this ad in the current playlist?
 ************************************************************************/
Boolean IsAdInPlaylist(AdId adId)
{
	return FindAd(adId, nil, nil) != nil;
}

/************************************************************************
 * AdWinGotImage - a graphic was just inserted--do we need to report it?
 ************************************************************************/
void AdWinGotImage(PETEHandle pte, FSSpecPtr spec)
{
	if (PeteIsInAdWindow(pte)) {
		AdStatePtr pAd;

		if (pAd = FindCurrentAdState()) {
			//      Give link window graphic filespec
			(*gPlayState)->dirty = true;
			AddAdToLinkHistory(pAd->adId, nil, nil, spec);
		}
	}
}

/************************************************************************
 * FindPlayList - find this playlist
 ************************************************************************/
static PlayListHandle FindPlayList(long playListId)
{
	PlayListHandle hPlayList;

	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = (*hPlayList)->next)
		if ((*hPlayList)->playListId == playListId)
			return hPlayList;

	return nil;
}

/************************************************************************
 * SetCurrentPlayList - set gPlayList to current playlist
 ************************************************************************/
static void SetCurrentPlayList(void)
{
	PlayListHandle hPlayList;

	if (gPlayState
	    && (hPlayList = FindPlayList((*gPlayState)->curPlayList)))
		gPlayList = hPlayList;
	else
		gPlayList = gPlayListQueue;
}

/************************************************************************
 * FindAd - find this ad in a playlist
 ************************************************************************/
static AdInfoPtr FindAd(AdId adId, short *idx, PlayListHandle * playList)
{
	short adIdx;
	PlayListHandle hPlayList;
	AdInfoPtr pAd;

	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = (*hPlayList)->next)
		for (adIdx = 0, pAd = (*hPlayList)->ads;
		     adIdx < (*hPlayList)->numAds; adIdx++, pAd++)
			if (SameAd(&adId, &pAd->adId)) {
				if (idx)
					*idx = adIdx;
				if (playList)
					*playList = hPlayList;
				return pAd;
			}
	return nil;
}

/************************************************************************
 * FindAdType - find a valid ad of this type in a playlist
 ************************************************************************/
static AdInfoPtr FindAdType(AdType adType, short *idx,
			    PlayListHandle * playList)
{
	return FindAdTypeLo(adType, 0, gPlayListQueue, idx, playList);
}

/************************************************************************
 * FindAdTypeLo - find a valie ad of this type in a playlist
 ************************************************************************/
static AdInfoPtr FindAdTypeLo(AdType adType, short startIdx,
			      PlayListHandle startPlayList, short *idx,
			      PlayListHandle * playList)
{
	AdInfoPtr pAd;
	PlayListHandle hPlayList;
	short adIdx;
	AdStatePtr pAdState;
	uLong curTime = LocalDateTime();

	for (hPlayList = startPlayList; hPlayList;
	     hPlayList = (*hPlayList)->next) {
		for (adIdx = startIdx, pAd = (*hPlayList)->ads + startIdx;
		     adIdx < (*hPlayList)->numAds; adIdx++, pAd++)
			if (adType == pAd->type)
				if (pAdState = FindAdState(pAd->adId, nil))
					if (!pAdState->deleted)
						if (ValidAd
						    (hPlayList, adIdx,
						     curTime,
						     vaCheckDayMax +
						     vaCheckShowForMax)) {
							if (idx)
								*idx =
								    adIdx;
							if (playList)
								*playList =
								    hPlayList;
							return pAd;
						}
		startIdx = 0;
	}

	return nil;
}

/************************************************************************
 * FindCurrentAdIdx - find current ad in playlist
 ************************************************************************/
static AdInfoPtr FindCurrentAdIdx(short *idx)
{
	return FindAd((*gPlayState)->curAd, idx, &gPlayList);
}

/************************************************************************
 * FindCurrentAd - find current ad in playlist
 ************************************************************************/
static AdInfoPtr FindCurrentAd(void)
{
	return FindCurrentAdIdx(nil);
}

/************************************************************************
 * FindAdState - find this ad in the playstate
 ************************************************************************/
static AdStatePtr FindAdState(AdId adId, short *idx)
{
	short adIdx;
	AdStatePtr pAd;

	for (adIdx = 0, pAd = (*gPlayState)->stateAds;
	     adIdx < (*gPlayState)->numAds; adIdx++, pAd++)
		if (SameAd(&adId, &pAd->adId)) {
			if (idx)
				*idx = adIdx;
			return pAd;
		}

	//      Not found
	return nil;
}

/************************************************************************
 * FindCurrentAdState - find current ad in playState
 ************************************************************************/
static AdStatePtr FindCurrentAdState(void)
{
	return FindAdState((*gPlayState)->curAd, nil);
}

/************************************************************************
 * FindPlayListState - find current playlist in playState
 ************************************************************************/
static AdStatePtr FindPlayListState(PlayListHandle hPlayList)
{
	AdId adId;
	AdStatePtr pAdState = nil;

	if (hPlayList) {
		adId.server = kPlayListServerId;
		adId.ad = (*hPlayList)->playListId;
		if (!(pAdState = FindAdState(adId, nil))) {
			Accumulator aState;

			AccuInitWithHandle(&aState, (Handle) gPlayState);
			AddStates(hPlayList, &aState, false);
			AccuTrim(&aState);	//      Don't dispose of this accumulator!
			pAdState = FindAdState(adId, nil);
		}
	}
	return pAdState;
}

/************************************************************************
 * CleanupAdCache - purge old ads
 ************************************************************************/
static void CleanupAdCache(Boolean withPrejuidice)
{
	uLong cutoffDate = LocalDateTime() - kPurgeAdDays * 60 * 60 * 24;
	Str32 name;
	FSSpec spec;
	CInfoPBRec hfi;

	// (jp)  9-10-99 If we can't find the Ad folder, don't delete anything
	// You mean, if you CAN find the ad folder, don't delete anything.  Yikes!  SD 12/5/99
	if (!SubFolderSpec(AD_FOLDER_NAME, &spec)) {
		hfi.hFileInfo.ioNamePtr = name;
		hfi.hFileInfo.ioFDirIndex = 0;
		while (!DirIterate(spec.vRefNum, spec.parID, &hfi)) {
			if (withPrejuidice
			    || hfi.hFileInfo.ioFlMdDat < cutoffDate) {
				//      Old enough, delete it
				PCopy(spec.name, name);
				if (!FSpDelete(&spec))	// (jp)  9-10-99 Only decrement the index if there was no err
					hfi.hFileInfo.ioFDirIndex--;
			}
		}
	}
}

/************************************************************************
 * TotalFaceTimeLeft - how much face time do we have lying around?
 ************************************************************************/
long TotalFaceTimeLeft(void)
{
	PlayListHandle hPlayList;
	long faceTimeLeft = 0;

	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = (*hPlayList)->next) {
		AdStatePtr pPlayListState;
		if ((*hPlayList)->totalShowMax
		    && (pPlayListState = FindPlayListState(hPlayList))) {
			long thisFaceTimeLeft;
			thisFaceTimeLeft =
			    (*hPlayList)->totalShowMax -
			    pPlayListState->shownTotal;
			if (thisFaceTimeLeft < 0)
				thisFaceTimeLeft = 0;
			faceTimeLeft += thisFaceTimeLeft;
		}
	}

	return faceTimeLeft;
}

#ifdef LOGADS
/************************************************************************
 * DoLogAds - log ad scheduling
 ************************************************************************/
static void DoLogAds(StringPtr sLog)
{
	FSSpec spec;
	Str31 scratch;

	//      Don't log ads unless Log window is open
	SimpleMakeFSSpec(Root.vRef, Root.dirId,
			 GetRString(scratch, LOG_NAME), &spec);
	if (FindText(&spec))
		Log(LOG_MORE, sLog);
}
#endif

#ifdef DEBUG
/************************************************************************
 * ToggleAdWindow - open/close ad window
 ************************************************************************/
void ToggleAdWindow(Boolean resetPlayState)
{
	if (AdWin)
		CloseAdWindow();
	else {
		//      Reset play state before opening ad window
		short oldRes = CurResFile();

		if (resetPlayState) {
			if (SettingsRefN)
				UseResFile(SettingsRefN);
			Zap1Resource(kStateResType, kStateResId);
			UseResFile(oldRes);
		}
		gPlayState = nil;
		OpenAdWindow();
		RequestPlaylist();
	}
}

/************************************************************************
 * DebugAdMenu - do debug ad menu item
 ************************************************************************/
void DebugAdMenu(short item, short modifiers)
{
	FSSpec spec;

	switch (item) {
	      newPlayList:
	case dbNewPlaylist:
		if (modifiers & optionKey)
			ResetAdState();
		RequestPlaylist();
		break;
	case dbReloadPlaylist:
		SubFolderSpec(AD_FOLDER_NAME, &spec);
		PCopy(spec.name, sPlayListFileName);
		NewPlaylist(&spec, nil);
		break;
	case dbDumpPlaylists:
		DumpPlaylists((modifiers & optionKey) != 0);
		break;
	case dbDefaultPLServer:
		SetPref(DEBUG_PLAYLIST_URL, "");
		goto newPlayList;
	default:
		if (gPlayList && item >= gAdsMenuAdsStart) {
			PlayListHandle hPlayList;
			short adIdx;

			adIdx = item - gAdsMenuAdsStart;
			for (hPlayList = gPlayListQueue; hPlayList;
			     hPlayList = (*hPlayList)->next) {
				if (adIdx < (*hPlayList)->numAds)
					break;
				adIdx -= (*hPlayList)->numAds + 1;
			}
			if (hPlayList && adIdx < (*hPlayList)->numAds) {
				gPlayList = hPlayList;
				NewAd(adIdx);
			}
			gBlackTime = 0;	//      Start ad right away
		} else if (item > dbDefaultPLServer) {
			Str127 server;
			MyGetItem(GetMHandle(AD_SELECT_HIER_MENU), item,
				  server);
			SetPref(DEBUG_PLAYLIST_URL, server);
			goto newPlayList;
		}
		break;
	}
}

/************************************************************************
 * SetupAdDebugMenu - setup ad debug menu
 ************************************************************************/
static void SetupAdDebugMenu()
{
	MenuHandle mh;
	Str255 s;

	if (RunType != Production
	    && (mh = GetMHandle(AD_SELECT_HIER_MENU))) {
		short i;
		PlayListHandle hPlayList;

		if (!gAdsMenuAdsStart) {
			gAdsMenuAdsStart = dbDefaultPLServer;
			MyAppendMenu(mh, PlayListURL);
			gAdsMenuAdsStart++;
			for (i = 1;
			     *GetRString(s, PLAYLIST_SERVER_STRN + i);
			     i++) {
				MyAppendMenu(mh, s);
				gAdsMenuAdsStart++;
			}
			AppendMenu(mh, "\p-");
			gAdsMenuAdsStart++;
		}

		TrashMenu(mh, gAdsMenuAdsStart);

		for (hPlayList = gPlayListQueue; hPlayList;
		     hPlayList = (*hPlayList)->next) {
			for (i = 0; i < (*hPlayList)->numAds; i++) {
				GetPlayListString(hPlayList,
						  (*hPlayList)->ads[i].
						  title, s);
				if (!*s)
					PCopy(s, "\pUntitled");
				if ((*hPlayList)->ads[i].type) {
					PCatC(s, ' ');
					PCatC(s, '(');
					PCatC(s,
					      '0' +
					      (*hPlayList)->ads[i].type);
					PCatC(s, ')');
				}
				MyAppendMenu(mh, s);
			}
			if ((*hPlayList)->next)
				AppendMenu(mh, "\p-");
		}
	}
}

/************************************************************************
 * CheckCurrentAd - check the current ad in ad debug menu
 ************************************************************************/
void CheckCurrentAd(void)
{
	MenuHandle mh;
	Str127 server;
	short mItem;

	if (mh = GetMHandle(AD_SELECT_HIER_MENU)) {
		uLong curTime = LocalDateTime();
		CheckNone(mh);

		// Check playlist server
		GetRString(server, DEBUG_PLAYLIST_URL);
		if (!*server)
			PCopy(server, PlayListURL);
		SetItemMark(mh, FindItemByName(mh, server), checkMark);

		// check ad
		if (gPlayList) {
			short adIdx;
			PlayListHandle hPlayList;
			MCEntry menuColor, *pMenuColor;

			FindCurrentAdIdx(&adIdx);
			for (hPlayList = gPlayListQueue; hPlayList;
			     hPlayList = (*hPlayList)->next) {
				if (hPlayList == gPlayList)
					break;
				adIdx += (*hPlayList)->numAds + 1;
			}
			SetItemMark(mh, adIdx + gAdsMenuAdsStart,
				    checkMark);

			// Mark inactive ones
			mItem = gAdsMenuAdsStart;
			Zero(menuColor);
			menuColor.mctID = AD_SELECT_HIER_MENU;
			SetRGBGrey(&menuColor.mctRGB2, 0x8000);
			if (pMenuColor =
			    GetMCEntry(AD_SELECT_HIER_MENU, 0))
				menuColor.mctRGB4 = pMenuColor->mctRGB4;
			for (hPlayList = gPlayListQueue; hPlayList;
			     hPlayList = (*hPlayList)->next) {
				for (adIdx = 0;
				     adIdx < (*hPlayList)->numAds;
				     adIdx++, mItem++) {
					if (ValidAd
					    (hPlayList, adIdx, curTime,
					     vaCheckDayMax +
					     vaCheckShowForMax)) {
						DeleteMCEntries
						    (AD_SELECT_HIER_MENU,
						     mItem);
					} else {
						//      Make item appear to be disabled
						menuColor.mctItem = mItem;
						SetMCEntries(1,
							     &menuColor);
					}
					if (!ValidAd
					    (hPlayList, adIdx, curTime,
					     vaCheckShowForMax))
						SetItemStyle(mh, mItem,
							     italic);
				}
				mItem++;
			}
		}
	}
}

Accumulator DumpAcc;
short DumpIndent;

/************************************************************************
 * DumpPlaylists - dump playlist info to a text window
 ************************************************************************/
void DumpPlaylists(Boolean fullDisplay)
{
	//      For every block that's not in our list, dump it to log
	MyWindowPtr win;
	PlayListHandle hPlayList;
	Str255 s, s2;
	DateTimeRec date;

	AccuInit(&DumpAcc);

	DumpIndent = 0;
	AccuDump(ComposeString
		 (s, "\pTotal number of ads: %d", (*gPlayState)->numAds));
	AccuDump(ComposeString
		 (s, "\pHistory length: %d", (*gPlayState)->histLength));
	date = (*gPlayState)->getNextPlaylist;
	AccuDump(ComposeString
		 (s, "\pRequest next playlist: %d/%d/%d %d:%d", date.month,
		  date.day, date.year, date.hour, date.minute));
	AccuDump(ComposeString
		 (s, "\pFetch error code: %d", (*gPlayState)->errCode));
	AccuDump(ComposeString
		 (s, "\pRerun mode: %p",
		  (*gPlayState)->outOfAds ? "\pyes" : "\pno"));
	RollGlobalFacetime();	// update faceTimeToday
	AccuDump(ComposeString
		 (s, "\pFacetime today: %d",
		  (*gPlayState)->faceTimeToday));
	AccuDump(ComposeString
		 (s, "\pAd facetime today: %d",
		  (*gPlayState)->adFaceTimeToday));
	AccuDump(ComposeString
		 (s, "\pFacetime quota: %d",
		  (*gPlayState)->faceTimeQuota));
	AccuDump(ComposeString
		 (s, "\pFaceTime week: %d,%d,%d,%d,%d,%d,%d",
		  (*gPlayState)->faceTimeWeek[0],
		  (*gPlayState)->faceTimeWeek[1],
		  (*gPlayState)->faceTimeWeek[2],
		  (*gPlayState)->faceTimeWeek[3],
		  (*gPlayState)->faceTimeWeek[4],
		  (*gPlayState)->faceTimeWeek[5],
		  (*gPlayState)->faceTimeWeek[6]));
	AccuDump(ComposeString
		 (s, "\pRerun interval: %d",
		  (*gPlayState)->rerunInterval / (24 * 60 * 60)));
	AccuDump(ComposeString
		 (s,
		  "\pAd Failure States: Failed--%B  Succeeded--%B Checked Mail--%B Checked Mail Today--%B",
		  NoAdsRec.adsAreFailing, NoAdsRec.adsAreSucceeding,
		  NoAdsRec.checkedMail, NoAdsRec.checkedMailToday));
	AccuDump(ComposeString
		 (s,
		  "\pAd Failure Counters: DeadbeatCounter %d ConsecutiveDays %d",
		  NoAdsRec.deadbeatCounter, NoAdsRec.consecutiveDays));

	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = (*hPlayList)->next) {
		short adIdx;

		AccuAddChar(&DumpAcc, '\r');

		GetPlayListString(hPlayList,
				  (*hPlayList)->playListIDString, s2);
		AccuDump(ComposeString
			 (s,
			  "\pPlaylist \"%p\", Ad count: %d, Show max total: %d",
			  s2, (*hPlayList)->numAds,
			  (*hPlayList)->totalShowMax));
		DumpIndent++;
		for (adIdx = 0; adIdx < (*hPlayList)->numAds; adIdx++) {
			AdInfo ad = (*hPlayList)->ads[adIdx];
			AdStatePtr pAdState;

			GetPlayListString(hPlayList, ad.title, s2);
			AccuDump(ComposeString
				 (s, "\p\"%p\" Id: %d.%d", s2,
				  ad.adId.server, ad.adId.ad));
			DumpIndent++;
			if (ad.type)
				AccuDump(ComposeString
					 (s, "\pType: %p",
					  ad.type ==
					  1 ? "\pToolbar button" : ad.
					  type ==
					  2 ? "\pSponsor" : "\pRunout"));
			AccuDump(ComposeString
				 (s,
				  "\pshowFor: %d, dayMax: %d, showForMax: %d",
				  ad.showFor, ad.dayMax, ad.showForMax));
			if (ad.blackBefore || ad.blackAfter)
				AccuDump(ComposeString
					 (s,
					  "\pblackBefore: %d, blackAfter: %d",
					  ad.blackBefore, ad.blackAfter));
			AccuDumpTime("\pStart Date", ad.startDate);
			AccuDumpTime("\pEnd Date", ad.endDate);

			if (pAdState = FindAdState(ad.adId, nil)) {
				AdState state = *pAdState;

				AccuDump(ComposeString
					 (s,
					  "\pShown today: %d, shown total: %d",
					  state.shownToday,
					  state.shownTotal));
				AccuDumpTime("\pLast display",
					     state.lastDisplayTime);

			}

			if (fullDisplay) {
				GetPlayListURLString(hPlayList, ad.srcURL,
						     s2);
				AccuDump(ComposeString
					 (s, "\pSource URL: %p", s2));
				GetPlayListURLString(hPlayList,
						     ad.clickURL, s2);
				AccuDump(ComposeString
					 (s, "\pClick URL: %p", s2));
			}
			DumpIndent--;
		}
		DumpIndent--;
	}

	if (!(win = FindTextLo(nil, "\pPlaylist Info")))
		win =
		    OpenText(nil, nil, nil, nil, true, "\pPlaylist Info",
			     true, false);
	if (!win)
		return;
	win->close = DumpClose;
	(*PeteExtra(win->pte))->spelled = sprNeverSpell;
	AccuTrim(&DumpAcc);
	PeteSetText(win->pte, DumpAcc.data);
	AccuZap(DumpAcc);
}

/************************************************************************
 * DumpClose - don't prompt for save on window close
 ************************************************************************/
static Boolean DumpClose(MyWindowPtr win)
{
	return (true);
}

/************************************************************************
 * AccuDump - write a line to dump accumulator
 ************************************************************************/
static void AccuDump(StringPtr s)
{
	short i;

	for (i = 0; i < 3 * DumpIndent; i++)
		AccuAddChar(&DumpAcc, ' ');	//      Do indentation
	AccuAddStr(&DumpAcc, s);
	AccuAddChar(&DumpAcc, '\r');
}

/************************************************************************
 * AccuDumpTime - write date/time to dump accumulator
 ************************************************************************/
static void AccuDumpTime(StringPtr sName, uLong seconds)
{
	Str255 s;
	DateTimeRec date;

	if (seconds) {
		SecondsToDate(seconds, &date);
		AccuDump(ComposeString
			 (s, "\p%p: %d/%d/%d %d:%d", sName, date.month,
			  date.day, date.year, date.hour, date.minute));
	}
}

#endif

/************************************************************************
 * PreFetchAds - load all ads into cache
 ************************************************************************/
static void PreFetchAds(void)
{
	short i;
	Str255 sURL;
	FSSpec spec;
	PlayListHandle hPlayList;

	if (gAdDownloadList)
		return;		//      We're still fetching ads

	(*gPlayState)->adsNotFetched = 0;
	for (hPlayList = gPlayListQueue; hPlayList;
	     hPlayList = (*hPlayList)->next) {
		for (i = 0; i < (*hPlayList)->numAds; i++) {
			GetPlayListURLString(hPlayList,
					     (*hPlayList)->ads[i].srcURL,
					     sURL);
			GetCacheSpec(sURL, &spec, true);
			if (FSpExists(&spec)) {
				//      Don't have ad in cache. Go get it.
				Handle url;

				if (url =
				    GetPlayListURL(hPlayList,
						   (*hPlayList)->ads[i].
						   srcURL)) {
					if (!QueueAdDownload
					    (url,
					     (*hPlayList)->ads[i].adId,
					     (*hPlayList)->ads[i].type))
						ZapHandle(url);	//      Error, didn't queue, is duplicate
				}
			}
		}
	}
}

/************************************************************************
 * QueueAdDownload - put ad in download queue if not duplicate
 ************************************************************************/
static Boolean QueueAdDownload(Handle url, AdId adId, AdType adType)
{
	DownloadListHandle hDownload;

	for (hDownload = gAdDownloadList; hDownload;
	     hDownload = (*hDownload)->next) {
		//      See if we already have this ad in the queue
		if (SameAd(&adId, &(*hDownload)->adId)
		    || !strcmp(*url, *(*hDownload)->url))
			return false;	//      Already in queue

	}
	if (hDownload = NuHandleClear(sizeof(DownloadList))) {
		(*hDownload)->url = url;
		(*hDownload)->adId = adId;
		(*hDownload)->adType = adType;
		LL_Push(gAdDownloadList, hDownload);
		return true;
	}
	return false;
}

/************************************************************************
 * CheckDownloadList - see if we can download any ads
 ************************************************************************/
static void CheckDownloadList(void)
{
	while (gAdDownloadList && gDownloadCount < kMaxDownloads) {
		Handle url = (*gAdDownloadList)->url;
		long reference;
		Str255 sURL;
		FSSpec spec;
		DownloadListHandle hDownload = gAdDownloadList;

		gAdDownloadList = (*gAdDownloadList)->next;	//      Take ad off queue
		*sURL = strlen(LDRef(url));
		BlockMoveData(*url, sURL + 1, *sURL);
		GetCacheSpec(sURL, &spec, true);
		FindTemporaryFolder(Root.vRef, Root.dirId, &spec.parID,
				    &spec.vRefNum);
		if (TestWriteToAdFolder()
		    || DownloadURL(*url, &spec, (long) hDownload,
				   FinishedAdDownload, &reference, nil)) {
			ZapHandle(hDownload);
			ZapHandle(url);
			(*gPlayState)->adsNotFetched++;
		} else {
			gAdFetchCount++;
			gDownloadCount++;
		}
	}
}

/************************************************************************
 * TestWriteToAdFolder - is it ok to write to the ad folder?
 ************************************************************************/
OSErr TestWriteToAdFolder(void)
{
	FSSpec spec;
	OSErr err, deleteErr;
	Boolean can;

	SubFolderSpec(AD_FOLDER_NAME, &spec);
	PCopy(spec.name, "\ptest.gif");
	if (err = FSpCreate(&spec, 'GIF ', 'GIF ', smSystemScript))
		if (err != dupFNErr)
			return err;
	err = CanWrite(&spec, &can);
	deleteErr = FSpDelete(&spec);
	if (deleteErr && !err)
		err = deleteErr;

	return err;
}

/************************************************************************
 * AdCheckingMail - checking mail, user is online, need to fetch ads and/or playlist?
 ************************************************************************/
void AdCheckingMail(void)
{
	RollGlobalFacetime();	// record facetime now

	NoAdsRec.checkedMailToday = true;

	SavePlayStatus();

	if (AdWin && (gPlayListDownloadFail || !gPlayListQueue))
		//      Haven't been able to get a playlist yet
		RequestPlaylist();
	else if (gPlayState && !gAdFetchCount
		 && (*gPlayState)->adsNotFetched)
		//      There are ads that we haven't been able to fetch yet
		PreFetchAds();
	else if (gPlayState && TotalFaceTimeLeft() < (*gPlayState)->faceTimeQuota / 2	// Running low on ads
		 && GMTDateTime() - gPlayListFetchGMT > kMinRetryInterval)	// But don't check too often
	{
		RequestPlaylist();
	}
}

/************************************************************************
 * RollGlobalFacetime - record facetime so far today and start recording again
 ************************************************************************/
void RollGlobalFacetime(void)
{
	long time;

	if (!gGlobalFaceMeasure)
		if (gGlobalFaceMeasure = NewFaceMeasure())
			FaceMeasureBegin(gGlobalFaceMeasure);

	if (gGlobalFaceMeasure) {
		if (FaceMeasureReport
		    (gGlobalFaceMeasure, &time, nil, nil, nil))
			return;
		if (gPlayState && time) {
			(*gPlayState)->faceTimeToday += time;
			(*gPlayState)->dirty = true;
		}
#ifdef DEBUG
		ComposeLogS(LOG_PLIST, nil, "\pFaceTimeRoll %ds %dm", time,
			    gPlayState ? (*gPlayState)->faceTimeToday /
			    60 : -1);
#endif
		FaceMeasureReset(gGlobalFaceMeasure);
	}
}

/************************************************************************
 * PastryIsStale - is the pastry stale?
 ************************************************************************/
OSErr PastryIsStale(Handle pastry)
{
	UPtr spot, end;
	short count = 0;

	// non-ascii chars?
	if (AnyFunny(pastry, 0))
		return 1;

	// Unbalanced quotes?
	end = *pastry + GetHandleSize(pastry);
	for (spot = *pastry; spot < end; spot++)
		if (*spot == '"')
			count++;
	if (count % 2)
		return 2;

	return 0;
}

/************************************************************************
 * AdCheckingMail - checking mail, user is online, need to fetch ads and/or playlist?
 ************************************************************************/
static void ParseAdId(StringPtr sToken, AdId * pAdId)
{
	UPtr spot;
	Str32 sNum;

	spot = sToken + 1;
	PToken(sToken, sNum, &spot, ".");
	StringToNum(sNum, &pAdId->server);
	PToken(sToken, sNum, &spot, ".");
	StringToNum(sNum, &pAdId->ad);
}

/************************************************************************
 * ShowBlackTimeImage - display image between ads
 ************************************************************************/
static void ShowBlackTimeImage(void)
{
	WindowPtr AdWinWP = GetMyWindowWindowPtr(AdWin);
	PicHandle pic = GetResource_('PICT', GRAY_LOGO_PICT);
	Rect rPic, rWin;

	if (pic && AdWinWP) {
		HNoPurge_(pic);
		SetPort_(GetWindowPort(AdWinWP));
		rPic = (*pic)->picFrame;
		rWin = AdWin->contR;
		CenterRectIn(&rPic, &rWin);
		if (rPic.top > 0 || rPic.left > 0) {
			//      Pic is too small for window. Erase first.
			EraseRect(&rWin);
		}
		DrawPicture(pic, &rPic);
		HPurge((Handle) pic);
	}
}

/************************************************************************
 * AddStringToHandle - add a string to a handle
 ************************************************************************/
static OSErr AddStringToHandle(StrOffset * offset, AccuPtr a, StringPtr s)
{
	*offset = a->offset;
	return AccuAddPtr(a, s, (*s) + 1);
}

/************************************************************************
 * GetPlayListString - get a string from a playlist
 ************************************************************************/
static StringPtr GetPlayListString(PlayListHandle hPlayList,
				   StrOffset offset, StringPtr s)
{
	if (hPlayList && offset)
		PCopy(s,
		      (*(Handle) hPlayList) + (*hPlayList)->strOffset +
		      offset);
	else
		*s = 0;
	return s;
}

/************************************************************************
 * GetPlayListURL - get a URL string from a playlist
 *			Returns a handle the caller must dispose of
 ************************************************************************/
static Handle GetPlayListURL(PlayListHandle hPlayList, StrOffset offset)
{
	Handle hURL = nil;
	char *pURL;

	if (hPlayList && offset) {
		LDRef(hPlayList);
		pURL =
		    (*(Handle) hPlayList) + (*hPlayList)->strOffset +
		    offset;
		hURL = NuDHTempOK(pURL, strlen(pURL) + 1);
		UL(hPlayList);
	}
	return hURL;
}

/************************************************************************
 * GetPlayListURLString - get a URL as a p-string from a playlist
 ************************************************************************/
static StringPtr GetPlayListURLString(PlayListHandle hPlayList,
				      StrOffset offset, StringPtr s)
{
	if (hPlayList && offset) {
		Ptr dest;
		uLong len;

		dest = *(Handle) hPlayList;
		dest += (*hPlayList)->strOffset + offset;
		len = strlen(dest);
		*s = min(len, 255);
		BlockMoveData(dest, s + 1, *s);
	} else
		*s = 0;
	return s;
}

/************************************************************************
 * Flush - flush a playlist or ad
 ************************************************************************/
static void Flush(TokenInfo * pInfo)
{
	enum { kIsPlayList = 1, kIsEntry, kIsProfileID } what;
	Str255 s, sId;
	short tokenIdx;

	what = 0;
	*sId = 0;
	while (tokenIdx = GetAttribute(pInfo, s)) {
		switch (tokenIdx) {
		case kEntityTkn:
			if (StringSame(PlayListKeywords[kPlayListTkn], s))
				what = kIsPlayList;
			else if (StringSame
				 (PlayListKeywords[kEntryTkn], s))
				what = kIsEntry;
			else if (StringSame
				 (PlayListKeywords[kUserProfileTkn], s))
				what = kIsProfileID;
			break;
		case kIdTkn:
			PCopy(sId, s);
			break;
		}
		if (what && (*sId || what == kIsProfileID)) {
			if (what == kIsPlayList) {
				//      Flush playlist
				PlayListHandle hPlayList;

				if (hPlayList = FindPlayList(Hash(sId)))
					REMOVE_AND_DESTROY(hPlayList);
			} else if (what == kIsProfileID) {
				SetProfileID("");
			} else {
				//      Flush entry
				AdId adId;

				ParseAdId(sId, &adId);
				DeleteAd(adId);
			}
			//      Set up to get some more. I think this shouldn't happen
			what = 0;
			*sId = 0;
		}
	}
}

/************************************************************************
 * SameAdId - are the ad id's the same?
 ************************************************************************/
Boolean SameAd(AdId * pAd1, AdId * pAd2)
{
	return pAd1->server == pAd2->server && pAd1->ad == pAd2->ad;
}

/************************************************************************
 * AdWinNeedsNetwork - do we currently need the network connection?
 ************************************************************************/
Boolean AdWinNeedsNetwork(void)
{
	return (gAdDownloadList || gPLDownloadInProgress
		|| gDownloadCount);
}


/************************************************************************
 * PrunePlayState - remove old ads from play state handle
 ************************************************************************/
static void PrunePlayState(Handle hKeepAdList)
{
	short i, delCount = 0;

	for (i = 0; i < (*gPlayState)->numAds; i++) {
		if ((*hKeepAdList)[i]) {
			//      Keep this one
			if (delCount)
				//      Need to move it
				(*gPlayState)->stateAds[i - delCount] =
				    (*gPlayState)->stateAds[i];
		} else
			//      Delete this one
			delCount++;
	}

	if (delCount) {
		//      We deleted some. Adjust play state handle
		(*gPlayState)->numAds -= delCount;
		SetHandleSize((Handle) gPlayState,
			      GetHandleSize((Handle) gPlayState) -
			      sizeof(AdState) * delCount);
		(*gPlayState)->dirty = true;
	}
}

/************************************************************************
 * MakeAdIconFromFile - create an ad icon from a file
 *
 *	32x32 and 16x16 icons are stored in the file in this format:
	+----------------------+-----------+
	|                      |           |
	|                      |16x16 image|
	|      32x32 image     |           |
	|                      |-----------+
	|                      |
	|                      |
	+----------------------+
 ************************************************************************/
static void MakeAdIconFromFile(FSSpecPtr spec, Handle iconSuite)
{
	OSErr err;
	GraphicsImportComponent importer;
	GWorldPtr gWorld = nil;
	Rect r;
	Handle hIcon;

	// figure out which importer can open the ad picture file ...
	if (!GetGraphicsImporterForFile(spec, &importer)) {
		// Create an offscreen GWorld to do the deed in ...
		SetRect(&r, 0, 0, 48, 32);
		err = NewGWorld(&gWorld, 8, &r, nil, nil, useTempMem);	//      Try temp memory first
		if (err)
			err = NewGWorld(&gWorld, 8, &r, nil, nil, nil);	//      Failed, use application heap
		if (!err) {
			//      Draw into offscreen world
			RGBColor color, *transparentColor = nil;

			if (GetPNGTransColor(importer, spec, &color))
				transparentColor = &color;	//      Use PNG transparent color when setting up mask
			PushGWorld();
			SetGWorld(gWorld, nil);
			if (!GraphicsImportSetGWorld
			    (importer, gWorld, nil))
				if (!GraphicsImportDraw(importer)) {
					SetRect(&r, 32, 0, 48, 16);
					if (hIcon =
					    MakeIcon(gWorld, &r, 8, 16))
						AddIconToSuite(hIcon,
							       iconSuite,
							       kSmall8BitData);
					if (hIcon =
					    MakeICN_pound(gWorld, &r, 16,
							  transparentColor))
						AddIconToSuite(hIcon,
							       iconSuite,
							       kSmall1BitMask);
					SetRect(&r, 0, 0, 32, 32);
					if (hIcon =
					    MakeIcon(gWorld, &r, 8, 32))
						AddIconToSuite(hIcon,
							       iconSuite,
							       kLarge8BitData);
					if (hIcon =
					    MakeICN_pound(gWorld, &r, 32,
							  transparentColor))
						AddIconToSuite(hIcon,
							       iconSuite,
							       kLarge1BitMask);
				}
			PopGWorld();
			DisposeGWorld(gWorld);
		}
		CloseComponent(importer);
	}
}

/************************************************************************
 * SetupSponsorAd - setup sponsor ad
 ************************************************************************/
void SetupSponsorAd(void)
{
	AdInfoPtr pAd;
	PlayListHandle hPlayList;
	Str255 sURL;
	FSSpec spec;

	if (!IsPayMode()	//      Free and adware only
	    && HaveQuickTime(0x0300))	//      In free mode, QuickTime is not required thus may not be installed
	{
		if (pAd = FindAdType(kIsSponsor, nil, &hPlayList)) {
			if (!gSponsorAdPic
			    || !SameAd(&gSponsorAdId, &pAd->adId)) {
				WindowPtr winWP;
				MyWindowPtr win;
				GraphicsImportComponent importer;
				StrOffset srcURL = pAd->srcURL;

				gSponsorAdId = pAd->adId;
				GetPlayListURLString(hPlayList, srcURL,
						     sURL);
				GetCacheSpec(sURL, &spec, true);
				ZapSponsorAd();
				if (!FSpExists(&spec)) {
					short oldResFile = CurResFile();	//      Graphic importing changes curresfile

					if (!CheckAd
					    (gSponsorAdId, nil, &spec))
						if (!GetGraphicsImporterForFile(&spec, &importer)) {
							SetupPNGTransparency
							    (importer,
							     &spec);
							GraphicsImportGetAsPicture
							    (importer,
							     &gSponsorAdPic);
							CloseComponent
							    (importer);
						}
					if (!gSponsorAdPic)
						Zero(gSponsorAdId);
					UseResFile(oldResFile);
				} else {
					//      Need to download this sponsor ad
					Handle url;

					if (url =
					    GetPlayListURL(hPlayList,
							   srcURL)) {
						if (!QueueAdDownload
						    (url, gSponsorAdId,
						     kIsSponsor))
							ZapHandle(url);	//      Error, didn't queue, is duplicate
					}
				}

				//      Set up windows to display sponsor ad
				//      Telling it to resize will do
				for (winWP = FrontWindow(); winWP;
				     winWP = GetNextWindow(winWP))
					if (IsKnownWindowMyWindow(winWP))
						if (win =
						    GetWindowMyWindowPtr
						    (winWP))
							if (win->
							    showsSponsorAd)
								MyWindowDidResize
								    (win,
								     nil);
			}
		} else
			//      Make sure we don't have a sponsor ad anymore
			ZapSponsorAd();
	}
}

/************************************************************************
 * ClickSponsorAd - process click if it's in sponsor ad
 ************************************************************************/
Boolean ClickSponsorAd(MyWindowPtr win, EventRecord * event, Point pt)
{
	if (win->showsSponsorAd && win->sponsorAdExists
	    && PtInRect(pt, &win->sponsorAdRect)) {
		AdUserClick(gSponsorAdId);
		return true;
	}
	return false;
}

/************************************************************************
 * DrawSponsorAd - draw sponsor ad if there is one
 ************************************************************************/
void DrawSponsorAd(MyWindowPtr win)
{
	if (gSponsorAdPic && win->showsSponsorAd && win->sponsorAdExists)
		DrawPicture(gSponsorAdPic, &win->sponsorAdRect);
}

/************************************************************************
 * ZapSponsorAd - get rid of sponsor ad if there is one
 ************************************************************************/
static void ZapSponsorAd(void)
{
	if (gSponsorAdPic) {
		//      Get rid of old ad
		KillPicture(gSponsorAdPic);
		gSponsorAdPic = nil;
	}
}

/************************************************************************
 * ResetAdSchedule - start from top on ad scheduling
 ************************************************************************/
static void ResetAdSchedule(void)
{
	(*gPlayState)->curAd.server = 0;
	(*gPlayState)->curAd.ad = 0;
	(*gPlayState)->dirty = true;
}


/************************************************************************
 * SetupWinSponsorInfo - make sure this window had sponsor ad info if it needs it
 ************************************************************************/
void SetupWinSponsorInfo(MyWindowPtr win)
{
	if (win->showsSponsorAd) {
		if (gSponsorAdPic) {
			Rect rPortRect;
			CGrafPtr winPort = GetMyWindowCGrafPtr(win);
			Rect rSponsor = (*gSponsorAdPic)->picFrame;
			Rect rWin = *GetPortBounds(winPort, &rPortRect);

			OffsetRect(&rSponsor,
				   rWin.right - RectWi(rSponsor) -
				   kSponsorBorderMargin - GROW_SIZE -
				   rSponsor.left,
				   rWin.bottom - RectHi(rSponsor) -
				   kSponsorBorderMargin - rSponsor.top);
			win->sponsorAdRect = rSponsor;
			win->sponsorAdExists = true;
		} else
			win->sponsorAdExists = false;
	}
}


/************************************************************************
 * MakeAdIcon - create a toolbar ad icon from URL
 ************************************************************************/
static void MakeAdIcon(PlayListHandle hPlayList, StrOffset srcURL,
		       AdId adId, Handle iconSuite, Boolean downloadOK)
{
	FSSpec spec;
	Str255 s;

	GetPlayListURLString(hPlayList, srcURL, s);
	GetCacheSpec(s, &spec, true);
	if (!FSpExists(&spec) && !CheckAd(adId, nil, &spec))
		MakeAdIconFromFile(&spec, iconSuite);
	else if (downloadOK) {
		//      Need to download icon
		Handle url;

		//      Put into c-string handle
		if (url = NuDHTempOK(s + 1, (*s) + 1)) {
			(*url)[*s] = 0;	//      null-terminate
			if (!QueueAdDownload(url, adId, kIsButton))
				ZapHandle(url);	//      Error, didn't queue, is duplicate
		}
	}
}

/************************************************************************
 * SetTBAdIcon - set this toolbar ad icon that just finished downloading
 ************************************************************************/
static void SetTBAdIcon(AdId adId)
{
	AdInfoPtr pAd;
	Handle iconSuite = nil;
	short i;
	TBAdInfo *pAdInfo;
	PlayListHandle hPlayList;

	if (!NewIconSuite(&iconSuite)) {
		if (pAd = FindAd(adId, nil, &hPlayList)) {
			MakeAdIcon(hPlayList, pAd->srcURL, adId, iconSuite,
				   false);
			TBUpdateAdButtonIcon(adId, iconSuite);
		}
	}

	if (gTBAds) {
		for (pAdInfo = *gTBAds, i = HandleCount(gTBAds); i--;
		     pAdInfo++)
			if (SameAd(&pAdInfo->adId, &adId)) {
				pAdInfo->iconSuite = iconSuite;
				break;
			}
	}
}

/************************************************************************
 * AdGetTBIcon - get icons for toolbar ad
 ************************************************************************/
Handle AdGetTBIcon(AdId adId)
{
	AdInfoPtr pAd;
	PlayListHandle hPlayList;
	Handle iconSuite = nil;

	LoadPlaylists();	//      Ad window may not be initialized yet

	if (pAd = FindAd(adId, nil, &hPlayList)) {
		StrOffset srcURL = pAd->srcURL;
		if (!NewIconSuite(&iconSuite))
			MakeAdIcon(hPlayList, srcURL, adId, iconSuite,
				   true);
	}
	return iconSuite;
}

/************************************************************************
 * SetupToolbarAds - set up ad toolbar ads
 ************************************************************************/
static void SetupToolbarAds(void)
{
	Accumulator a;
	PlayListHandle hPlayList;
	AdInfoPtr pAd;
	short adIdx;
	TBAdInfo adInfo;
	Str255 s;
	short i;
	TBAdInfo *pAdInfo;

	if (gTBAds) {
		//      Add to existing list
		AccuInitWithHandle(&a, (Handle) gTBAds);
		for (pAdInfo = *gTBAds, i = a.offset / sizeof(**gTBAds);
		     i--; pAdInfo++)
			pAdInfo->deleted = true;
	} else {
		//      New list of toolbar ads
		if (AccuInit(&a))
			return;
		gTBAds = (TBAdHandle) a.data;
	}

	if (IsAdwareMode())	// toolbar ads in Light?  I don't think so.  (!IsPayMode())
	{
		for (adIdx = 0, hPlayList = gPlayListQueue;
		     pAd =
		     FindAdTypeLo(kIsButton, adIdx, hPlayList, &adIdx,
				  &hPlayList); adIdx++) {
			Boolean found;

			//      Do we already have this ad?
			found = false;
			for (pAdInfo = *gTBAds, i =
			     a.offset / sizeof(**gTBAds); i--; pAdInfo++) {
				if (SameAd(&pAdInfo->adId, &pAd->adId)) {
					found = true;
					pAdInfo->deleted = false;
					break;
				}
			}

			if (!found) {
				//      Add new one to list
				StrOffset srcURL = pAd->srcURL;
				adInfo.adId = pAd->adId;
				GetPlayListString(hPlayList, pAd->title,
						  s);
				PStrCopy(adInfo.title, s,
					 sizeof(adInfo.title));
				if (!NewIconSuite(&adInfo.iconSuite))
					MakeAdIcon(hPlayList, srcURL,
						   adInfo.adId,
						   adInfo.iconSuite, true);
				adInfo.deleted = false;
				AccuAddPtr(&a, &adInfo, sizeof(adInfo));
			}
		}
	} else {
		a.offset = 0;	//      Remove ads if in pay mode
	}

	AccuTrim(&a);
	TBAddAdButtons(gTBAds);
	if (!GetHandleSize_(gTBAds))
		ZapHandle(gTBAds);
}

/************************************************************************
 * AdDeleteButton - a toolbar ad has been deleted by user
 ************************************************************************/
void AdDeleteButton(AdId adId)
{
	AdStatePtr pAdState;

	if (pAdState = FindAdState(adId, nil)) {
		pAdState->deleted = true;
		(*gPlayState)->dirty = true;
	}
}

/************************************************************************
 * AdValidate - validate ad/playlist with MD5
 *	This function is intentionally a bit messy to try and confuse any
 *	hacker looking for our secret. It also has a non-descriptive function
 *  name for the same reason.
 ************************************************************************/
OSErr SetupForDisplay(Ptr data, long dataLen,
		      unsigned char checksum[kCheckSumSize], Boolean check)
{
	MD5_CTX md5;
	static Secret0 = 0x15028A81;
	long *in = (long *) &md5.in[0];
	OSErr result = noErr;
//      char secret[] = "\pJello be dim.";

	MD5Init(&md5);

//      MD5Update(&md5,secret+1,*secret);
//      The following does the same as the previous line but
//      without revealing the secret string
	in[3] = Secret3 ^ 0x4b99fe13;
	md5.i[0] = 104;
	in[1] = Secret1 ^ 0x0747db4b;
	md5.i[1] = 0;
	md5.buf[0] = 0x67452301;
	md5.buf[1] = 0xEFCDAB89;
	in[0] = Secret0 ^ 0x5f67e6ed;
	md5.buf[2] = 0x98BADCFE;
	md5.buf[3] = 0x10325476;
	in[2] = Secret2 ^ 0x8361e9bf;

	MD5Update(&md5, data, dataLen);
	MD5Final(&md5);

	if (check) {
		//      Check this checksum
		if (memcmp(md5.digest, checksum, kCheckSumSize))
			result = (*gPlayState)->errCode = badCksmErr;	//      Validation failure!
	} else {
		//      Return this checksum
		BMD(md5.digest, checksum, kCheckSumSize);
	}

	return result;
}

/************************************************************************
 * InitAdPte - setup pte for ad window
 ************************************************************************/
static void InitAdPte(void)
{
	CGrafPtr AdWinPort = GetMyWindowCGrafPtr(AdWin);
	PETEDocInitInfo initInfo;
	RGBColor bkColor;

	DefaultPII(AdWin, false, 0, &initInfo);
	initInfo.inParaInfo.startMargin = 0;
	initInfo.inParaInfo.endMargin =
	    WindowWi(GetMyWindowWindowPtr(AdWin)) + 1;
	PeteCreate(AdWin, &ADpte, 0, &initInfo);
	CleanPII(&initInfo);
	PeteLock(ADpte, 0, 0x7fffffff, peModLock | peSelectLock);
	PETESetDefaultBGColor(PETE, ADpte,
			      GetPortBackColor(AdWinPort, &bkColor));
	PeteDidResize(ADpte, &AdWin->contR);
}

/************************************************************************
 * GetAdFolderSpec - get spec for ad folder, creating if necessary
 ************************************************************************/
OSErr GetAdFolderSpec(FSSpecPtr spec)
{
	Str255 s;
	CInfoPBRec hfi;
	long junk;
	OSErr err = noErr;

	FSMakeFSSpec(Root.vRef, Root.dirId, GetRString(s, AD_FOLDER_NAME),
		     spec);
	IsAlias(spec, spec);
	if (FSpGetHFileInfo(spec, &hfi))
		err = FSpDirCreate(spec, smSystemScript, &junk);
	else {
		//      Make sure user hasn't locked folder or replaced it with a file
		if (hfi.hFileInfo.ioFlAttrib & 0x01)
			//      It was locked. Unlock it.
			err = FSpRstFLock(spec);
		if (!(hfi.hFileInfo.ioFlAttrib & 0x10)) {
			//      Not a folder. Delete it and create folder.
			if (!(err = FSpDelete(spec)))
				err =
				    FSpDirCreate(spec, smSystemScript,
						 &junk);
		}
	}
	return err;
}

/************************************************************************
 * GetLanguageCode - get language code in "xx-xx" format
 ************************************************************************/
UPtr GetLanguageCode(UPtr sResult)
{
	Str255 s;
	short index;

	index = (short) GetScriptManagerVariable(smRegionCode) + 1;
	if (index >= 100)
		index++;	//      1-99 and 101-109
	GetRString(s, LanguageIdStrn + index);
	PStrCopy(sResult, s, 6);
	return sResult;
}

/************************************************************************
 * HandleNag - tell Eudora the user needs to be nagged
 ************************************************************************/
OSErr HandleNag(long level, PStr text)
{
	if (level == 0) {
		ZapResource(NAG_REQUEST_TYPE, PLNAG_LEVEL0_DLOG);
		AddPResource(text + 1, *text, NAG_REQUEST_TYPE,
			     PLNAG_LEVEL0_DLOG, "");
	}
	if (level == 1) {
		ZapResource(NAG_REQUEST_TYPE, PLNAG_LEVEL1_DLOG);
		AddPResource(text + 1, *text, NAG_REQUEST_TYPE,
			     PLNAG_LEVEL1_DLOG, "");
	}
	if (level == 2) {
		ZapResource(NAG_REQUEST_TYPE, NAG_INTRO_DLOG);
		AddPResource(text + 1, *text, NAG_REQUEST_TYPE,
			     NAG_INTRO_DLOG, "");
	}
	PlaylistNagCount++;
	return noErr;
}

/************************************************************************
 * HandleUserProfile - store a profile given to us by the playlist server
 ************************************************************************/
OSErr HandleUserProfile(PStr text)
{
	return SetProfileID(text);
}

/************************************************************************
 * HandleClientMode - switch modes based on playlist server orders;
 * or, actually, tell the main loop to do so
 ************************************************************************/
OSErr HandleClientMode(PStr text)
{
	long newMode = -1;
	StringToNum(text, &newMode);
	if (newMode >= 0)
		NewClientModePlusOne = newMode + 1;
	return noErr;
}

/************************************************************************
 * ForecPlaylistRequest - force a playlist request
 ************************************************************************/
void ForcePlaylistRequest(void)
{
	RequestPlaylist();
}

/************************************************************************
 * GetSponsorAdInfo - get some information about the sponsor ad
 ************************************************************************/
void GetSponsorAdTitle(UPtr sTitle)
{
	AdInfoPtr pAd;
	PlayListHandle hPlayList;

	if (pAd = FindAdType(kIsSponsor, nil, &hPlayList))
		GetPlayListString(hPlayList, pAd->title, sTitle);
	else
		*sTitle = 0;
}

#endif
