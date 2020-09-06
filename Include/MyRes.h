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

#ifndef MYRES_H
#define MYRES_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * This file contains resource definitions for Pop mail
 **********************************************************************/
#pragma mark File Types
#define CREATOR 'CSOm'
#define SETTINGS_TYPE 'PREF'
#define MAILBOX_TYPE	'TEXT'
#define TOC_TYPE			'TOCF'
#ifdef VCARD
#define	VCARD_TYPE		'vCRD'
#endif
#define kFakeAppType	'eApp'
#ifdef CTB
#define CTB_CFIG_TYPE 'CTfg'
#define CTB_CFIG_ID 1001
#endif
#define SAVE_POS_TYPE 'S_WP'
#define PLUG_TYPE			'rsrc'
#define SAVER_RTYPE	'XEuS'
#define DECODER_RTYPE 'XEuD'
#define INCOME_MIME_MAP	'EuIM'
#define OUTGO_MIME_MAP	'EuOM'
#define WS_ALIAS_TYPE	'eWSa'
#define STATIONERY_TYPE	'EuSn'
#define NOTIFY_TYPE	'eNot'
#define FINDER_LIST_TYPE	'EuFl'
#define FINDER_LIST_ID	1001
#define SEA_LIST_TYPE 'sea#'
#define URLMAP_TYPE	'urlM'
#define PGP_ENCRYPTED_TYPE	'euEn'
#define PGP_JUSTSIG_TYPE	'euSi'
#define URLMAP_TYPE	'urlM'
#define HELP_TYPE 'EuHl'
#define MIME_FTYPE	'MiME'
#define TL_INFO_TYPE	'TlIn'
#define FILTER_FTYPE	'EuFi'
#define PRE_PROMISE_OK	'EuPD'
#define PROMISE_OK	'EuPr'
#define APPLET_LIST_TYPE 'APLT'
#define PRE_FILTER_TYPE	'PrFi'
#define POST_FILTER_TYPE	'PsFi'
#define FILE_GRAPHIC_LIST_TYPE	'FGlt'
#define FILE_MOVIE_LIST_TYPE	'MVlt'
#define DATELOC_TYPE 'DtMp'
#define LDAP_FILTER_TYPE	'FUtf'
#define LDAP_CN_FILTER_TYPE	'FUcf'
#define SPELLER_CREATOR	'Blac'
#define EXECUTABLE_TYPE_LIST	'eXeC'
#define IMAP_MAILBOX_TYPE	'iMbx'
#define SEARCH_FILE_TYPE 'eSrc'
#define REG_FILE_TYPE 'eReg'
#define LINK_HISTORY_PREVIEW_TYPE 'lhpr'
#define MACOSXSUCKS_TYPE_LIST 'XskT'
#define MACOSXSUCKS_CREATOR_LIST 'XskC'
#define OUTGOING_MSG_MID_LIST	'midL'

typedef struct
{
	Rect r;
	Boolean zoomed;
} PositionType, *PositionPtr, **PositionHandle;

#pragma mark Printing
/**********************************************************************
 * for holding page setup stuff
 **********************************************************************/
#define PRINT_RTYPE 'CSOp'
#define PRINT_CSOp	1001
#define PRINT_PAGE_SETUP	1002	//	carbon
#define PRINT_SETTINGS	1003		//	carbon

#ifdef	GX_PRINTING
/**********************************************************************
 * for holding GX page setup stuff
 **********************************************************************/
#define GX_PRINT_RTYPE 'CSOg'
#define GX_PRINT_CSOg	1001
#endif	//GX_PRINTING

#pragma mark Strings
/**********************************************************************
 * Strings - these are STR# resources for use for various things
 * They are organized as an enum for the STR#'s resource ids, then
 * enums (in the form of typedefs) for the individual strings
 **********************************************************************/
typedef enum {
	PRIOR_STRN=600,						/* priority strings */
	RECIPIENT_STRN=610,					/* save recipient menu values */
	PREF3_STRN=700,						/* more, more, I'm still not satisfied */
	PW_STRN=800,							/* strn for password change protocol */
	PREF2_STRN=900,						/* strn for the rest of the settings */
	PREF4_STRN=20100,
	PREF_STRN=1000, 					/* strn for user preferences and settings */
	OLD_BOX_LINES_STRN=1200,	/* places to draw lines in mailbox windows */
	FILE_STRN=1400, 					/* file system errors */
	HEADER_STRN=1600, 				/* headers for outgoing messages */
	UNDO_STRN=1800, 					/* menu items for undo */
	MACTCP_ERR_STRN=2000, 		/* mactcp error list */
	ICMP_STRN=2200, 					/* icmp message list */
#ifdef CTB
	CTB_NAV_IN_STRN=2400,
	CTB_NAV_OUT_STRN=2600,
#endif
	SMTP_STRN=2800, 					/* STMP commands */
	POP_STRN=3000,						/* POP commands */
	FROM_STRN=3400, 					/* "from " header is toast, baby */
#ifdef CTB
	CTB_ERR_STRN=3600,				/* CTB errors */
#endif
	MONTH_STRN=3800,
	WEEKDAY_STRN=4000,
	EX_HEADERS_STRN=4400,
	COMP_HELP_STRN=4600,
	MBWIN_HELP_STRN=4800,
	NICK_HELP_STRN=5000,
	MBOX_HELP_STRN=5200,
	TXT_FMT_BAR_HELP_STRN=15400, /* MJN *//* formatting toolbar */
	OTPPP_ERR_STRN=15500,		/* OT PPP error messages */
	OTTCP_ERR_STRN=15600,		/* OT TCP error messages */
	OTPPP_MSG_STRN=17400,		/* OT PPP connection messages */
	PERS_HELP_STRN=17000,
	SIG_HELP_STRN=17100,
	STNRY_HELP_STRN=18000,
	CONJ_STRN=18300,
	IMAP_OPERATIONS_STRN=19100,		/* name of IMAP operations */
	IMAP_EXPLANATIONS_STRN=19200,	/* explanations of IMAP errors */
	SEARCH_HELP_STRN=19300,
	LINK_HELP_STRN=19700,
	HTTP_ERR_STRN=20000,
	PLAYLIST_SERVER_STRN=20800,
	PHWIN_HELP_STRN=24000,
	HEADER_LABEL_STRN=24200,
	SUM_SENDER_HEADS=24400,
	MIMETEXT_STRN=25000,
	FILT_CMD_STRN=25200,
	VERB_STRN=25400,
	BOX_LINES_STRN=25700,
	FILT_HELP_STRN=25800,
	ATT_HELP_STRN=25900,
	FNAME_STRN=0,//28100,
	FindHelpStrn=31000,
	IMPORT_HELP_STRN=22000,
	STRN_LIMIT								/* placeholder */
} StrnEnum;

#define HEAD_LIMIT REPLYTO_HEAD
#define BODY (HEAD_LIMIT-1)

typedef enum {
	CTB_TOOL_STR=1001,
	STR_LIMIT
} StrEnum;

typedef enum {
	TRANS_IN_TABL=1001,
	TRANS_OUT_TABL_8859_1,
	ktIdendity,
	ktFlatten,
	ktMacUSHidden,
	ktFSMapString,
	ktISO15Mac,
	ktMacISO15,
	ktWindowsMac,
	ktMacWindows,
	ktMacUS = 2002,
	ktISOMac = 2991,
	ktMacISO,
	ktISO15Mac2993,
	ktMacISO152994,
	ktWindowsMac2995,
	ktMacWindows2996,
	kt2022=4000,
} TransEnum;

#pragma mark Dialogs
/**********************************************************************
 * Dialog Item Lists
 **********************************************************************/
typedef enum {
	OK_ALRT_DITL=1001,				/* DITL for OK Alert */
	ABOUT_ALRT_DITL,
	ERR_ALRT_DITL,
	NPREF_DLOG_DITL,
	BIG_ALRT_DITL,
	DUMP_ALRT_DITL,
	NEW_TOC_ALRT_DITL,
	NEW_MAILBOX_DITL,
	UNUSED_NOT_HOME_DITL,
	WANNA_SAVE_DITL,
	PROTO_ERR_ALRT_DITL,
	WANNA_SEND_DITL,
	PASSWORD_DITL,
	ADDR_ALRT_DITL,
	FIND_DITL,
	NOT_FOUND_DITL,
	DELETE_NON_EMPTY_ALRT_DITL,
	DELETE_EMPTY_ALRT_DITL,
	RENAME_DLOG_DITL,
	ICMP_ALRT_DITL,
	NEW_MAIL_ALRT_DITL,
	TIMEOUT_ALRT_DITL,
	QUIT_QUEUE_DITL,
	UPREF_DLOG_DITL,
	DUNNO_DITL,
	TOC_SALV_DITL,
	DITL_LIMIT								/* placeholder */
} DitlEnum;

#define DEBUG_BUTTON	5
#define EXIT_BUTTON 7

typedef enum {
	TOC_CREATE_NEW = 1,
	TOC_USE_OLD,
	TOC_CANCEL
} RebTOCDitlEnum;
	
typedef enum {
	DELETE_REMOVE_ALL = kAlertStdAlertOtherButton,
	DELETE_REMOVE_IT = kAlertStdAlertOKButton,
	DELETE_CANCEL = kAlertStdAlertCancelButton
} DeleteEmptyDitlEnum;

typedef enum {
	NEW_MAILBOX_OK = 1,
	NEW_MAILBOX_NAME,
	NEW_MAILBOX_CANCEL,
	NEW_MAILBOX_ICON,
	NEW_MAILBOX_TITLE,
	NEW_MAILBOX_FOLDER,
	NEW_MAILBOX_NOXF,
	NEW_MAILBOX_LIMIT
} NewMDitlEnum; 						/* item list for new mailbox DITL */

typedef enum {
	NEW_NICK_OK						= 1,
	NEW_NICK_CANCEL				= 2,
	NEW_NICK_ADD_DETAILS	= 3,
	NEW_NICK_NAME					= 6,
	NEW_NICK_RECIP				= 8,
	NEW_NICK_FILE					= 9,
	NEW_NICK_FULL_NAME		= 11,
	NEW_NICK_FIRST_NAME		= 14,
	NEW_NICK_LAST_NAME		= 15,
	NEW_NICK_SWAP_NAMES		= 16,
	NEW_NICK_ADDRESS			= 17,
	NEW_NICK_IN_ADDRESS		= 19
} NewNickDitlEnum; 					/* item list for new/rename nickname */

typedef enum {
	NEW_GROUP_OK						= 1,
	NEW_GROUP_CANCEL				= 2,
	NEW_GROUP_ADD_DETAILS		= 3,
	NEW_GROUP_NICK_NAME			= 6,
	NEW_GROUP_RECIP					= 8,
	NEW_GROUP_FILE					= 9,
	NEW_GROUP_GROUP_NAME		= 11,
	NEW_GROUP_ADDRESSES			= 12,
	NEW_GROUP_IN_ADDRESSES	= 14
} NewGroupDitlEnum;

typedef enum {
	WANNA_SAVE_SAVE = 1,
	WANNA_SAVE_CANCEL,
	WANNA_SAVE_DISCARD,
	WANNA_SAVE_LIMIT
} WannaSaveDitlEnum;

typedef enum {
	PASSWORD_OK = 1,
	PASSWORD_CANCEL,
	PASSWORD_WORD,
	PASSWORD_WARNING,
	PASSWORD_SAVE,
	PASSWORD_LIMIT
} PasswordDitlEnum;

typedef enum {
	SADL_PARAGRAPHS=13,
	SADL_EXCLUDE_HEADERS,
#ifndef ONE
	SADL_STATIONERY,
	SADL_STATIONERY_FOLDER,
#endif
	SADL_LIMIT
} SaveAsDitlEnum;

typedef enum {
	QQL_SEND=1,
	QQL_CANCEL,
	QQL_QUIT,
	QQL_LIMIT
} QuitQueueDitlEnum;

typedef enum {
  MQDL_OK=1,
	MQDL_CANCEL,
	MQDL_NOW,
	MQDL_QUEUE,
	MQDL_LATER,
	MQDL_UNQUEUE,
	MQDL_TIME,
	MQDL_DATE,
	MQDL_LIMIT
} ModQueueItems;

typedef enum {
  QMQ_SEND=1,
	QMQ_DONT,
	QMQ_SEND_ALL,
	QMQ_CANCEL,
	QMQ_LIMIT
} QuitModQueueItems;

#ifdef THREADING_ON
typedef enum {
  QTR_WAIT=1,
	QTR_CANCEL,
	QTR_DONT,
	QTR_LIMIT
} QuitThreadRunning;
#endif

typedef enum {
	MEMORY_QUIT=1,
	MEMORY_CONTINUE,
	MEMORY_LIMIT
} MemoryDitlEnum;

#ifdef NAG

typedef enum {
	NAG_PLEASE_REGISTER_REGISTER_BTN = 1,
	NAG_PLEASE_REGISTER_LATER_BTN = 2
} NagRegisterDitlEnum;

typedef enum {
	NAG_DOWNGRADE_YES_I_MEAN_IT_BTN = 1,
	NAG_DOWNGRADE_CANCEL_BTN = 2,
	NAG_DOWNGRADE_FEATURE_LIST = 6
} NagDowngradeDitlEnum;

typedef enum {
	NAG_FEATURES_YES_BTN = 1,
	NAG_FEATURES_CANCEL_BTN = 2,
	NAG_FEATURES_FEATURE_LIST = 6
} NagFeaturesDitlEnum;

typedef enum {
	NAG_NOT_GETTING_ADS_MORE_INFO_BTN = 1,
	NAG_NOT_GETTING_ADS_ERROR_TEXT = 5
} NagNotGettingAdsDitlEnum;

typedef enum {
	NAG_NO_ADS_AT_ALL_OK_BTN = 1,
	NAG_NO_ADS_AT_ALL_MORE_INFO_BTN = 2
} NagNoAdsAtAllDitlEnum;

typedef enum {
	REFUND_DITL_OK = 1,
	REFUND_DITL_CODE_TEXT = 3
} RefundDitlEnum;

typedef enum {
	PROFILE_RCVD_DITL_REPLACE = 1,
	PROFILE_RCVD_DITL_CANCEL = 2,
	PROFILE_RCVD_DITL_FAQ = 3,
	PROFILE_RCVD_DITL_PROFILEID = 6
} ProfileIDReceivedDitlEnum;
#endif

typedef enum {
	REPAY_DITL_PAY_NOW				= 1,
	REPAY_DITL_SPONSORED			= 2,
	REPAY_DITL_SHOW_VERSIONS	= 3
} RepayDitlEnum;

typedef enum {
	JUNKDOWN_DITL_SPONSORED			= 1,
	JUNKDOWN_DITL_PAY_NOW				= 2,
	JUNKDOWN_DITL_MORE	= 3
} JunkDownDitlEnum;

#pragma mark Cursors
/************************************************************************
 * cursors
 ************************************************************************/
#define PENDULUM_CURS 1001
#define MENU_CURS 1005
#define DIVIDER_CURS 1006
#define SPREAD_CURS 1007
#define SPREAD_CURS_V 1008
#define DIVIDER_CURS_V 1009
#define POINTER_FINGER	1010
#define BALL_CURS 128

#pragma mark Icons
/**********************************************************************
 * Icons
 **********************************************************************/
typedef enum {
/* 1001 */ 	APP_ICON=1001,						/* application icon */
/* 1002 */ 	SETTINGS_ICON,
/* 1003 */ 	MAILBOX_ICON,
/* 1004 */ 	TOC_ICON,
/* 1005 */ 	PLUGIN_ICON,
/* 1006 */ 	STATIONERY_ICON,
/* 1007 */ 	DUNNO_ICON,
/* 1008 */ 	HELP_ICON,
/* 1009 */ 	TRUCK_ICON,
/* 1010 */ 	EAPP_ICON,
/* 1011 */ 	FLAG_SICN, 							/* a mailbox "flag" */
/* 1012 */ 	EUDORA_SICN, 									/* our SICN */
/* 1013 */ 	SIGNATURE_SICN,
/* 1014 */ 	WRAP_SICN,
/* 1015 */ 	KEEPCOPY_SICN,
/* 1016 */ 	TAB_SICN,
/* 1017 */ 	QP_SICN,
/* 1018 */ 	BX_TEXT_SICN,
/* 1019 */ 	SERVER_SICN,
/* 1020 */ 	UNUSED_SICN,
/* 1021 */ 	FETCH_SICN,
/* 1022 */ 	BLAH_SICN,
/* 1023 */ 	RR_SICN,
/* 1024 */ 	LOCK_SICN,
/* 1025 */ 	DOWN_ARROW_SICN,
/* 1026 */ 	SIGN_SICN,
/* 1027 */ 	KEY_ICON,
/* 1028 */ 	MENU_ICON,
/* 1029 */ 	PENCIL_SICN,
/* 1030 */ 	LEFT_ARROW_SICN,
/* 1031 */ 	RIGHT_ARROW_SICN,
/* 1032 */ 	UP_ARROW_SICN,
/* 1033 */ 	DN_ARROW_SICN,
/* 1034 */ 	BEGIN_SICN,
/* 1035 */ 	BACK_BOX_SICN,
/* 1036 */ 	BACK_MSG_SICN,
/* 1037 */ 	FWD_MSG_SICN,
/* 1038 */ 	FWD_BOX_SICN,
/* 1039 */ 	MIME_ICON,
/* 1040 */ 	ATTACH_ICON,
/* 1041 */ 	ON_SERVER_ICON,
/* 1042 */ 	ON_FETCH_ICON,
/* 1043 */ 	ON_DELETE_ICON,
/* 1044 */	ON_BOTH_ICON,
/* 1045 */	FILTER_ICON,
/* 1046 */	PH_LIST_ICON,
/* 1047 */  TRIANGLE_SICN,
/* 1048 */	IN_MB_ICON,
/* 1049 */	OUT_MB_ICON,
/* 1050 */	TRIANGLE_TRANSITION_ICON,
/* 1051 */	BOLD_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1052 */	ITALIC_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1053 */	UNDERLINE_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1054 */	INCREASE_TEXT_SIZE_SICN, /* MJN *//* formatting toolbar */
/* 1055 */	DECREASE_TEXT_SIZE_SICN, /* MJN *//* formatting toolbar */
/* 1056 */	LEFT_JUST_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1057 */	CENTER_JUST_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1058 */	RIGHT_JUST_TEXT_SICN, /* MJN *//* formatting toolbar */
/* 1059 */	TEXT_INDENT_IN_SICN, /* MJN *//* formatting toolbar */
/* 1060 */	TEXT_INDENT_OUT_SICN, /* MJN *//* formatting toolbar */
/* 1061 */	TEXT_HANGING_INDENT_IN_SICN, /* MJN *//* formatting toolbar */
/* 1062 */	TEXT_HANGING_INDENT_OUT_SICN, /* MJN *//* formatting toolbar */
/* 1063 */	CLEAR_TEXT_FORMATTING_SICN, /* MJN *//* formatting toolbar */
/* 1064 */	WINDOW_HORIZ_ZOOM_SICN,
/* 1065 */	NEW_MB_ICON,
/* 1066 */	NEW_MB_FOLDER_ICON,
/* 1067 */	DELETE_MB_ICON,
/* 1068 */	PERSONALITIES_ICON,
/* 1069 */	TRANSFER_ICON,
/* 1070 */	COPY_ICON,
/* 1071 */	OLD_FILTER_ICON,
/* 1072 */	STOP_ICON,
/* 1073 */	NEW_PERSONALITY_ICON,
/* 1074 */	DELETE_PERSONALITY_ICON,
/* 1075 */	EDIT_PERSONALITY_ICON,
/* 1076 */	NEW_SIGNATURE_ICON,
/* 1077 */	DELETE_SIGNATURE_ICON,
/* 1078 */	NEW_STATIONERY_ICON,
/* 1079 */	DELETE_STATIONERY_ICON,
/* 1080 */	EDIT_STATIONERY_ICON,
/* 1081 */	MESS_ERR_ICON,
/* 1082 */	PERS_CHECK_ICON,
/* 1083 */	FORMATBAR_ICON,
/* 1084 */	BULLET_ICON,
/* 1085 */	FIXED_WIDTH_ICON,
/* 1086 */  NO_IMAGE_ICON,
/* 1087 */	RULE_ICON,
/* 1088 */	LINK_ICON,
/* 1089 */	TP_NETWORK_ICON,
/* 1090 */	TOOLBAR_COLOR_SICN_TEMPLATE,
/* 1091 */	EDIT_SIGNATURE_ICON,
/* 1092 */	MISSING_IMAGE_ICON,
/* 1093 */	PRIORITY_HOLLOW_ICON,
/* 1094 */	MAILBOX_ONLY_ICON,
/* 1095 */	TASK_PROGRESS_ICON,
/* 1096 */	TASK_PROGRESS2_ICON,
/* 1097 */	TASK_PROGRESS3_ICON,
/* 1098 */	TASK_PROGRESS4_ICON,
/* 1099 */	TASK_PROGRESS5_ICON,
/* 1100 */	TASK_PROGRESS_ERROR_ICON,
/* 1101 */	PREVIEW_DIVIDE_ICON,
/* 1102 */	LOCKED_ADDRESS_BOOK_ICON,
/* 1103 */	ADDRESS_BOOK_ICON,
/* 1104 */	NICKNAME_ICON,
/* 1105 */	GROUP_ICON,
/* 1106 */	RECIPIENT_NICKNAME_ICON,
/* 1107 */	IMAP_NOTHING_FETCHED,
/* 1108 */	IMAP_DELETED_ICON,
/* 1109 */  IMAP_MAILBOX_FILE_ICON,
/* 1110 */  QUOTE_TEXT_SICN,	// fmtbar
/* 1111 */  UNQUOTE_TEXT_SICN,	// fmtbar
/* 1112 */	IMAP_FETCHED_BODY,
/* 1113 */	IMAP_FETCHED_ALL,
/* 1114 */	SEARCH_FILE_ICON,
/* 1115 */	PRO_ONLY_ICON,
/* 1116 */	AD_VERSION_ICON,
/* 1117 */	PAY_VERSION_ICON,
/* 1118 */	FREE_VERSION_ICON,
/* 1119 */	REGISTER_ICON,
/* 1120 */	CUSTOMIZE_ADS_ICON,
/* 1121 */	UPDATES_ICON,
/* 1122 */	CHANGE_REG_ICON,
/* 1123 */	VIEW_LINK_ICON,
/* 1124 */	DELETE_LINK_ICON,
/* 1125 */	HTTP_LINK_TYPE_ICON,
/* 1126 */	MAILTO_LINK_TYPE_ICON,
/* 1127 */	FTP_LINK_TYPE_ICON,
/* 1128 */	TECH_SUPPORT_ICON,
/* 1129 */	SWAP_FIRSTLAST_NAME_ICON,
/* 1130 */	NEW_NICKNAME_ICON,
/* 1131 */	NEW_ADDRESS_BOOK_ICON,
/* 1132 */	DELETE_NICK_ADDRESS_BOOK_ICON,
/* 1133 */	RECIPIENT_GROUP_ICON,
/* 1134 */	PERSONAL_NICKNAME_ICON,
/* 1135 */	VCARD_ICON,
/* 1136 */	COLORS_MENU_ICON_TEMPLATE,
/* 1137 */	COLORS_MENU_ICON_BASE,
/* 1397 */	COLORS_MENU_ICON_LIMIT=COLORS_MENU_ICON_BASE+260,
/* 1398 */	DRAWER_ICON,
/* 1399 */  IMAP_POLLED_MAILBOX_FILE_ICON,
/* 1400 */  BAD_IMAGE_ICON,
/* 2000 */	TOOLBAR_COLOR_SICN_BASE=2000,
/* 2050 */	TOOLBAR_COLOR_SICN_LIMIT=2050,
/* 2051 */	COLUMN_ICON_BASE,
/* 2058 */  REFRESH_ICON=2058,				/* this and 2059 probably should be renumbered */
/* 2059 */	RESYNC_ICON=2059,				/* they don't belong in the column icon range. */
/* 2070 */	COLUMN_ICON_LIMIT=2070,
/* 2071 */	ANAL_ICON_BASE=2071,
/* 2090 */	ANAL_ICON_LIMIT=2090,
/* 2091 */  MAKE_JUNK_ICON=2091,
/* 2092 */  SEARCH_WEB_ICON=2092,
/* 2093 */  EMOTICON_ICON,
/*  480 */ 	TL_ICON_START = 480,
/*  500 */ 	TL_ICON_END = 500,
	ICON_LIMIT								/* placeholder */
} IconEnum;

#define TRASH_SICN -3993
#define SIGN_SICN	2019
#define ENCRYPT_SICN 1026

typedef enum {
/*  257 */ 	PRIOR_SICN_BASE=257,
/*  262 */ 	ATYPE_SICN_BASE=262,
/*  270 */ 	SIG_SICN=270,
/*  271 */ 	NO_SIG_SICN,
/*  272 */ 	ALT_SIG_SICN,
/*  273 */ 	N_SIG_SICN,
/*  279 */ 	RAISE_PRIOR_SICN=279,
/*  280 */ 	LOWER_PRIOR_SICN,
/*  281 */ 	LABEL_SICN_BASE,
/*  295 */ 	LABEL_SICN_LAST=LABEL_SICN_BASE+14,
/*  296 */ 	LABEL_SICN_TEMPLATE,
/*  297 */ 	MESSAGE_STATUS_ICON_BASE,
/*  305 */ 	MESSAGE_STATUS_ICON_CHECKMARK=305,	// Defined in weasel-like fashion to be used as a checkmark for the downgrade dialog
/*  317 */ 	MESSAGE_STATUS_ICON_LIMIT=317,
/*  318 */	NO_LABEL_SICN,
	SICN_LIMIT
} SIconEnum;

#pragma mark Alerts
/**********************************************************************
 * Alerts and dialogs
 **********************************************************************/
typedef enum {
/* 1001 */ 	UNUSED__OK_ALRT=1001, 				/* an alert with just an ok button */
/* 1002 */ 	ABOUT_ALRT, 					/* About UIUCmail */
/* 1003 */ 	NON_AM_ERR_ALRT, 						/* non-appearance-mgr alert for fatal errors */
/* 1004 */ 	NPREF_DLOG, 					/* dialog for preferences */
/* 1005 */ 	URL_SETTING_CONFIRM_DLOG,					/* Dialog for confirming URL settings changes */
/* 1006 */ 	UNUSED__DUMP_ALRT,						/* for debugging */
/* 1007 */ 	UNUSED__NEW_TOC_ALRT, 				/* create a new toc? */
/* 1008 */ 	NEW_MAILBOX_DLOG, 		/* ask for the name of a mailbox */
/* 1009 */ 	UNUSED_LONGER_NOT_HOME_ALRT,				/* nobody is listening to keystrokes */
/* 1010 */ 	UNUSED_1010,
/* 1011 */ 	UNUSED__PROTO_ERR_ALRT, 			/* error with SMTP */
/* 1012 */ 	UNUSED__WANNA_SEND_ALRT,			/* do we want to save changes before sending? */
/* 1013 */ 	PASSWORD_DLOG,				/* used to get someone's password */
/* 1014 */ 	UNUSED__BAD_ADDR_ALRT,				/* to report a bad address */
/* 1015 */ 	URL_DLOG,							/* attach a url to some text */
/* 1016 */ 	UNUSED__NOT_FOUND_ALRT, 			/* report failure of find */
/* 1017 */ 	UNUSED__DELETE_NON_EMPTY_ALRT,				/* confirm delete of non-empty mailbox */
/* 1018 */ 	UNUSED__DELETE_EMPTY_ALRT,		/* confirm delete of empty mailbox */
/* 1019 */ 	UNUSED_DLOG_1,				//	RENAME_BOX_DLOG,			/* rename mailbox */
/* 1020 */ 	UNUSED__ICMP_ALRT,						/* ICMP message alert */
/* 1021 */ 	NEW_MAIL_ALRT,				/* new mail has arrived */
/* 1022 */ 	UNUSED__TIMEOUT_ALRT, 				/* does the user want to keep waiting, or cancel? */
/* 1023 */ 	UNUSED__QUIT_QUEUE_ALRT,			/* you have queued messages. quit? */
/* 1024 */ 	UNUSED__YES_CANCEL_ALRT,		/* trash unread messages? */
/* 1025 */ 	UPREF_DLOG, 					/* the other half of the preferences */
/* 1026 */ 	SAVEAS_DLOG,					/* my hacked SFPUTFILE */
/* 1027 */ 	GETFOLDER_DLOG, 			/* tim maroney's hacked SFGETFILE */
/* 1028 */ 	UNUSED__BAD_HEXBIN_ALRT,			/* a hexbin has failed */
/* 1029 */ 	UNUSED__REB_TOC_ALRT, 				/* create a new toc? */
/* 1030 */ 	UNUSED__CLEAR_DROP_ALRT,			/* errors UUPC'ing.  Clear drop anyway? */
/* 1031 */ 	UNUSED__TOC_SALV_ALRT,				/* can't salvage TOC.  Rebuild? */
/* 1032 */ 	MODQ_DLOG,						/* modify queue dialog */
/* 1033 */ 	QUIT_MQ_ALRT,					/* quitting with modified queue */
/* 1034 */ 	NEW_NICK_DLOG,				/* new nickname dialog */
/* 1035 */ 	UNUSED__MEMORY_ALRT,					/* dialog for memory warning */
/* 1036 */ 	UNUSED__READ_ONLY_ALRT,				/* read only */
/* 1037 */ 	NO_MAIL_ALRT,					/* no mail */
/* 1038 */ 	XFER_MENU_DLOG,				/* choose from xfer menu */
/* 1039 */ 	UNUSED__XFER_TO_OUT,					/* xfer to out loses stuff */
/* 1040 */ 	UNUSED__REMOVE_SPELL_ALRT,		/* remove speller from menu? */
/* 1041 */ 	UNUSED__NICK_REP_ALRT,				/* replace nickname? */
/* 1042 */ 	UNUSED__ALIAS_OR_REAL_ALRT,		/* delete alias or real file? */
/* 1043 */ 	UNUSED__INSIST_SETTINGS_ALRT,	/* owners insist open from Eudora Settings */
/* 1044 */ 	ANSWER_DLOG,					/* get an answer to a nav query */
/* 1045 */ 	UNUSED__ATTACH_APP_ALRT,			/* does the user really want to unpack this application? */
/* 1046 */ 	SF_PROMPT_DITL,				/* for prompting in SF boxes */
/* 1047 */ 	UNUSED__ATTACH_APP2_ALRT,			/* does the user really want to unpack this application? */
/* 1048 */ 	UNUSED__PURCHASE_ALRT,				/* does the user want information on Eudora? */
/* 1049 */ 	UNUSED__QUEST_ALRT,						/* does the user want mailing list? */
/* 1050 */	TBAR_MENU_DLOG,
/* 1051 */	UNUSED__TBAR_REM_ALRT,				/* should we remove a file from the toolbar? */
/* 1052 */	UNUSED__ONLINE_ALRT,					/* should we check even though offline? */
/* 1053 */	UNUSED__OPEN_ERR_ALRT,				/* error during connection opening? */
/* 1054 */	TEXT_INSERT_DITL,			/* insert text file */
/* 1055 */	MOM_ALRT,							/* alert for Mom warnings. */
/* 1056 */	SPECIAL_CHECK_ALRT,		/* alert for special mail checks. */
/* 1057 */	unusedDlog,		//	NEW_SIG_DLOG,					/* request new signature name. */
/* 1058 */	UNUSED__MDN_ALRT,							/* message disposition notification alert. */
/* 1059 */	UNUSED__USE_BACKUP_ALRT,			/* alert for using backup file. */
/* 1060 */	UNUSED_INVALID_REG_NUM_ALRT,	/* alert for invalid registration number */
/* 1061 */	UNUSED_EXPIRE_ALRT,					/* alert the user that the demo has expired.  No longer used in 43 */
/* 1062 */	UNUSED_WILL_EXPIRE_ALRT,			/* seven day demo expiration alert.  No longer used in 43 */
/* 1063 */	PERS_LIST_DITL,				/* DITL for adding the personality list to check mail specially */
/* 1064 */	WIFE_ALRT,						/* alert for Wife warnings. */
/* 1065 */	UNUSED__FILT_MB_RENAME_ALRT,	/* alert when filters refer to changed mailbox names */
/* 1066 */	SFODOC_DITL,					/* DITL for adding options to SFODoc dialog */
/* 1067 */	URL_DITL,							/* DITL for adding options to URL helper dialog */
/* 1068 */	MAKEFILTER_DLOG,			/* the makefilter dialog for creating filters */
/* 1069 */	EDIT_PERSONALITY_DLOG,	/* edit personalities */
/* 1070 */	ACAP_LOGIN_DLOG,	/* gather ACAP login info */
/* 1071 */	UNUSED__ACAP_RETRY_ALRT,	/* retry acap with quit */
/* 1072 */	UNUSED__ACAP_RETRY_CNCL_ALRT,	/* retry acap with cancel */
/* 1073 */	CHOOSE_PERSONALITY_DLOG,	/* select a personality from a list */
/* 1074 */	UNUSED__QUIT_THREAD_RUN_ALRT,	/* threads running, quit? */
/* 1075 */	SEND_STYLE_ALRT,	// alert to determine how the user wants styles to be sent
/* 1076 */	ATTACH_NAV_DITL,	// 'DITL' used for custom Nav Services attachment dialog
/* 1077	*/	SAVEAS_NAV_DITL,	// 'DITL' used for custom Nav Services Save As dialog
/* 1078 */	NAG_INTRO_DLOG,									// Dialog presented to first-time users of Eudora Select
/* 1079	*/	NAG_PLEASE_REGISTER_DLOG,				// Dialog to nag unregistered users
/* 1080	*/	NAG_UNUSED3,										// Currently reserved by nagging
/* 1081	*/	NAG_DOWNGRADE_DLOG,							// Dialog to be displayed when the user considers downgrading to freeware
/* 1082 */	NAG_FEATURES_DLOG,							// Dialog to nag free users that the world is a better place with ads
/* 1083	*/	NAG_NOT_GETTING_ADS_DLOG,				// Dialog to nag adware users when they aren't getting ads
/* 1084	*/	NAG_FRIGGING_HACKER_DLOG,				// Dialog to politely inform the user that they ar being downgraded because they do not get ads at all
/* 1085	*/	PRE_PAYMENT_DLOG,								// Dialog we'll display right before whisking the user off to our web site prior to payment
/* 1086	*/	PRE_REGISTRATION_DLOG,					// Dialog we'll display right before whisking the user off to our web site prior to registration
/* 1087 */	NAG_DEBUG,											// Currently reserved by nagging
/* 1088 */	AUDIT_XMIT,	// is it ok to send audit?
/* 1089 */	AD_OBSCURED_DLOG,	// The ad window is currently obscured
/* 1090 */	OFFLINE_LINK_DLOG,	// You can't get there from here.
/* 1091 */	NAG_LINK_REMIND_DLOG,	// Don't forget to visit these sites
/* 1092 */	REFUND_CODE_DLOG,			// Smoke'n'mirrors
/* 1093	*/	PRE_PROFILING_DLOG,					// Dialog we'll display right before whisking the user off to our web site prior to registration
/* 1094	*/	PROFILE_RECEIVED_DLOG,			// Dialog we'll display once a profiling ID has been received
/* 1095 */ 	NEW_GROUP_DLOG,				/* new group dialog */
/* 1096	*/	IMPORT_WHAT_DLOG,				// Dialog we'll display right before importing mail
/* 1097 */	IMPORTER_SELECT_DLOG,			// Diaog we'll display to allow the user to select which importer to use
/* 1098 */	PLNAG_LEVEL0_DLOG,			// Dialog for level 0 (brief) nag to profile user
/* 1099 */	PLNAG_LEVEL1_DLOG,			// Dialog for level 1 (feature) nag to profile user
/* 1100	*/	REPAY_DLOG,							// Dialog to inform the user that they have to open their wallet to use this update in paid mode
/* 1101	*/	SSL_CERT_DLOG_9,					// Dialog for unknown SSL certificates (Mac OS 9)
/* 1102	*/	SSL_CERT_DLOG_X,					// Dialog for unknown SSL certificates (Mac OS X)
/* 1103	*/	JUNK_INTRO_DLOG,					// Dialog for junk intro
/* 1104	*/	JUNKDOWN_DLOG,					// Dialog for downgrading the Junk feature
/* 1109 */  CHECK_CONNECTION_DLOG,			// Check settings in pref dialog
ALRT_LIMIT						/* placeholder */
} AlrtDialEnum;

/* use STR# defines with StandardAlert */
typedef enum {
/*  */ 	OK_ALRT=OK_ASTR+ALRTStringsStrn, 				/* an alert with just an ok button */
/*  */ 	ERR_ALRT=ERR_ASTR+ALRTStringsStrn, 						/* alert for fatal errors */
/*  */ 	BIG_OK_ALRT=OK_ASTR+ALRTStringsStrn,					/* like the OK_ASTR, but bigger */
/*  */ 	DUMP_ALRT=DUMP_ASTR+ALRTStringsOnlyStrn,						/* for debugging */
/*  */ 	NEW_TOC_ALRT=NEW_TOC_ASTR+ALRTStringsStrn, 				/* create a new toc? */
/*  */ 	UNUSED_NOT_HOME_ALRT=UNUSED_NOT_HOME_ASTR+ALRTStringsOnlyStrn,				/* nobody is listening to keystrokes */
/*  */ 	PROTO_ERR_ALRT=PROTO_ERR_ASTR+ALRTStringsStrn, 			/* error with SMTP */
/*  */ 	WANNA_SEND_ALRT=WANNA_SEND_ASTR+ALRTStringsStrn,			/* do we want to save changes before sending? */
/*  */ 	NOT_FOUND_ALRT=NOT_FOUND_ASTR+ALRTStringsOnlyStrn, 			/* report failure of find */
/*  */ 	DELETE_NON_EMPTY_ALRT=DELETE_NON_EMPTY_ASTR+ALRTStringsOnlyStrn,				/* confirm delete of non-empty mailbox */
/*  */ 	DELETE_EMPTY_ALRT=DELETE_EMPTY_ASTR+ALRTStringsOnlyStrn,		/* confirm delete of empty mailbox */
/*  */ 	ICMP_ALRT=ICMP_ASTR+ALRTStringsStrn,						/* ICMP message alert */
/*  */ 	TIMEOUT_ALRT=TIMEOUT_ASTR+ALRTStringsStrn, 				/* does the user want to keep waiting, or cancel? */
/*  */ 	QUIT_QUEUE_ALRT=QUIT_QUEUE_ASTR+ALRTStringsOnlyStrn,			/* you have queued messages. quit? */
/*  */ 	YES_CANCEL_ALRT=YES_CANCEL_ASTR+ALRTStringsStrn,		/* trash unread messages? */
/*  */ 	REB_TOC_ALRT=REB_TOC_ASTR+ALRTStringsStrn, 				/* create a new toc? */
/*  */ 	CLEAR_DROP_ALRT=CLEAR_DROP_ASTR+ALRTStringsOnlyStrn,			/* errors UUPC'ing.  Clear drop anyway? */
/*  */ 	TOC_SALV_ALRT=TOC_SALV_ASTR+ALRTStringsStrn,				/* can't salvage TOC.  Rebuild? */
/*  */ 	MEMORY_ALRT=MEMORY_ASTR+ALRTStringsOnlyStrn,					/* dialog for memory warning */
/*  */ 	READ_ONLY_ALRT=READ_ONLY_ASTR+ALRTStringsOnlyStrn,				/* read only */
/*  */ 	XFER_TO_OUT=XFER_TO_OUT_ASTR+ALRTStringsOnlyStrn,					/* xfer to out loses stuff */
/*  */ 	REMOVE_SPELL_ALRT=REMOVE_SPELL_ASTR+ALRTStringsOnlyStrn,		/* remove speller from menu? */
/*  */ 	NICK_REP_ALRT=NICK_REP_ASTR+ALRTStringsOnlyStrn,				/* replace nickname? */
/*  */ 	ALIAS_OR_REAL_ALRT=ALIAS_OR_REAL_ASTR+ALRTStringsOnlyStrn,		/* delete alias or real file? */
/*  */ 	INSIST_SETTINGS_ALRT=INSIST_SETTINGS_ASTR+ALRTStringsOnlyStrn,	/* owners insist open from Eudora Settings */
/*  */ 	ATTACH_APP_ALRT=ATTACH_APP_ASTR+ALRTStringsOnlyStrn,			/* does the user really want to unpack this application? */
/*  */ 	ATTACH_APP2_ALRT=ATTACH_APP2_ASTR+ALRTStringsOnlyStrn,			/* does the user really want to unpack this application? */
/*  */	TBAR_REM_ALRT=TBAR_REM_ASTR+ALRTStringsOnlyStrn,				/* should we remove a file from the toolbar? */
/*  */	ONLINE_ALRT=ONLINE_ASTR+ALRTStringsOnlyStrn,					/* should we check even though offline? */
/*  */	OPEN_ERR_ALRT=OPEN_ERR_ASTR+ALRTStringsStrn,				/* error during connection opening? */
/*  */	MDN_ALRT=MDN_ASTR+ALRTStringsOnlyStrn,							/* message disposition notification alert. */
/*  */	USE_BACKUP_ALRT=USE_BACKUP_ASTR+ALRTStringsStrn,			/* alert for using backup file. */
/*  */	FILT_MB_RENAME_ALRT=FILT_MB_RENAME_ASTR+ALRTStringsOnlyStrn,	/* alert when filters refer to changed mailbox names */
/*  */	ACAP_RETRY_ALRT=ACAP_RETRY_ASTR+ALRTStringsOnlyStrn,	/* retry acap with quit */
/*  */	ACAP_RETRY_CNCL_ALRT=ACAP_RETRY_CNCL_ASTR+ALRTStringsOnlyStrn,	/* retry acap with cancel */
/*  */	QUIT_THREAD_RUN_ALRT=QUIT_THREAD_RUN_ASTR+ALRTStringsOnlyStrn,	/* threads running, quit? */
/*  */ 	NEW_IMAP_TOC_ALRT=NEW_IMAP_TOC_ASTR+ALRTStringsOnlyStrn, 				/* create a new toc for an IMAP mailbox? */
/*  */ 	RESET_STATS_ALRT=RESET_STATS_ASTR+ALRTStringsOnlyStrn, 				/* create a new toc for an IMAP mailbox? */
} StandardAlrtDialEnum;

#define SFGETFILE_ID -4000
#define SFPUTFILE_ID -3999

typedef enum
{
	uscSetItem=1,
	uscCancelItem,
	uscRevertItem,
	uscNewValueItem,
	uscHelpItem,
	uscCurValueItem,
	uscDefValueItem,
} USCEnum;

typedef enum
{
	mdnSend=1,
	mdnDont,
	mdnLimit
} MDNEnum;

typedef enum
{
	nsigOk = 1,
	nsigCancel,
	nsigName,
	nsigLimit
} NewSigEnum;

typedef enum
{
	adlOk = 1,
	adlCancel,
	adlAnswer,
	adlLimit
}	AnswerEnum;

typedef enum
{
	bbhLMOS = 1,
	bbhGetRid,
	bbhRefetch
} BadBinHexEnum;

typedef enum
{
	nrNone=-1,
	nrAdd=1,
	nrDifferent,
	nrReplace,
	nrOk,
	nrCancel,
	nrLimit
}	NickReplaceEnum;

typedef enum
{
	aorJustAlias=1,
	aorCancel,
	aorBoth,
	aorLimit
} AliasOrRealEnum;

typedef enum
{
	purchSend=1,
	purchCancel,
	purchInfo,
	purchLimit
} PurchaseEnum;

typedef enum
{
	questSubscribe=1,
	questCancel,
	questUnsubscribe,
	questLimit
} QuestEnum;

#define OK 1
#pragma mark Menus
/**********************************************************************
 * menus - add 1000 to get resource id's
 **********************************************************************/
typedef enum {
/*  501 */ 	APPLE_MENU=501,
/*  502 */ 	FILE_MENU,
/*  503 */ 	EDIT_MENU,
/*  504 */ 	MAILBOX_MENU,
/*  505 */ 	MESSAGE_MENU,
/*  506 */ 	TRANSFER_MENU,
/*  507 */ 	SPECIAL_MENU,
/*  508 */ 	MENU_LIMIT,
/*  509 */ 	DEBUG_MENU,
/*  510 */ 	FONT_NAME_MENU,
/*  511 */ 	PRINT_FONT_MENU,
/*  512 */ 	FROM_MB_MENU,
/*  513 */ 	TO_MB_MENU,
/*  514 */ 	WINDOW_MENU,
/*  515 */ 	HELP_MENU,	//	for OS X
/* 1516 */ 	VERB_MENU=1516,
/* 1517 */ 	VERB2_MENU,
/* 1518 */ 	CONJ_MENU,
/* 1519 */ 	unusedMenu1519,//SIG_MENU,
/* 520 */ 	ATTACH_MENU=520,
/* 1521 */ 	NICK_FILE_MENU=1521,
/* 1522 */ 	HEAD_ACCEL_MENU,
/* 1523 */ 	PH_IN_MENU,
/* 1524 */ 	PH_OUT_MENU,
/* 1525 */ 	UNUSED_1525_MENU,
/* 1526 */ 	FILT_SOUND_MENU,
/* 1527 */ 	HELP_NOTMENU,
/* 1528 */ 	WAS_SERVER_MENU, // not anymore
/* 1529 */ 	MB_POPUP_MENU,	//	No resource, just menu ID
/* 1530 */	SCHIZO_POPUP_MENU,
/* 1531 */	SCHIZO_STATION_MENU,
/* 1532 */	UNUSED_1532_MENU,
/* 1533 */	MF_ANYR_POPUP_MENU,	//	No resource, just menu ID
/* 1534 */	ETL_CONTEXT_POPUP_MENU,	//	No resource, just menu ID
/* 1535 */	FLA_HI_MENU,
/* 1536 */	FILT_STATUS_MENU,
/* 1537 */  FILT_PRIOR_MENU,
/* 1538 */	UNUSED_1538_MENU,
/* 1539 */	FILT_PERS_MENU,
/* 1540 */	FILT_STATION_MENU,
/* 1541 */	TP_NETWORK_MENU,
/* 1542 */	BOX_ATTACH_MENU,
/* 1543 */	SRCH_CATEGORY_MENU,
/* 1544 */	SRCH_EQUAL_MENU,
/* 1545 */	SRCH_COMPARE_MENU,
/* 1546 */	SRCH_TEXT_COMPARE_MENU,
/* 1547 */	SRCH_AGE_UNITS_MENU,
/* 1548 */	SRCH_AND_OR_MENU,
/* 1549 */	SRCH_DATE_COMPARE_MENU,
/* 1550	*/	FILT_VOICE_MENU,
/* 1551 */	STAT_TIME_MENU,
/* 1552 */	STAT_GRAPH_TYPE,
/* 1553 */ 	IMPORTER_SELECT_MENU,
/* 1554 */ 	SCRIPTS_MENU,
/* 1555 */ 	SSL_MENU,
/* 1556 */	FILT_NICKFILE1_MENU,
/* 1557 */	FILT_NICKFILE2_MENU,
/* 1558 */	PROXY_MENU,
/* 1559 */	CONCON_PREVIEW_POP_MENU,
/* 1560 */	CONCON_MESSAGE_POP_MENU,
/* 1561 */	CONCON_MULTI_POP_MENU,
/* 1562 */	SMTP_RELAY_PERS_POP_MENU,
/* 1563 */ 	MAILBOX_OPTIONS_MENU,
/* 2000 */ 	NICK_VIEW_BY_MENU=2000,
/* 3019 */ 	COUNTRY_MENU=3019,
/* 4000 */  CONTEXT_MENU=4000, // No resource, just menu ID
	MENU_LIMIT2
} MenuEnum;
#define FLA_MENU (FLA_HI_MENU-1000)

#define TFER_HMNU 3506
#define FCC_HMNU 2506

typedef enum {
	urldOK = 1,
	urldCancel,
	urldRemove,
	urldText,
	urldLimit
} URLDialogEnum;

typedef enum {
	svmNone=1,
	svmFetch,
	svmDelete,
	svmBoth,
	svmLimit
} ServerMenuEnum;

#ifdef	IMAP
typedef enum {
	isvmFetchMessage=1,
	isvmFetchAttachments,
	isvmDelete,
	isvmRemoveCache,
	isvmLimit
} IMAPServerMenuEnum;
#endif

typedef enum {
	tmPlain=1,
	tmBold,
	tmItalic,
	tmUnderline,
	tmBar1,
	tmQuote,
	tmUnquote,
	tmBar2,
	tmLeft,
	tmCenter,
	tmRight,
	tmBar3,
	tmMargin,
	tmColor,
	tmFont,
	tmSize,
	tmBar4,
	tmRule,
	tmGraphic,
	tmURL,
	tmLimit
} TextMenuEnum;

/* MJN *//* formatting toolbar */
typedef enum {
	tfbSizeSmallest=1,
	tfbSizeSmaller,
	tfbSizeSmall,
	tfbSizeNormal,
	tfbSizeBig,
	tfbSizeBigger,
	tfbSizeBiggest,
} TextSizePopupEnum;

typedef enum {
	olaConnect=1,
	olaCancel,
	olaOnline,
	olaLimit
} OnlineAlertEnum;

typedef enum {
	tblInsert=1,
	tblDelete,
	tblBar1,
	tblGridLine,
	tblBorder,
	tblBar2,
	tblRowHt,
	tblColWd
} TableMenuEnum;

typedef enum {
	/* 201 */	FIND_HIER_MENU=201,
	/* 202 */	NEW_TO_HIER_MENU,
	/* 203 */	FORWARD_TO_HIER_MENU,
	/* 204 */	REDIST_TO_HIER_MENU,
	/* 205 */	INSERT_TO_HIER_MENU,
	/* 206 */ NEW_WITH_HIER_MENU,
	/* 207 */	REPLY_WITH_HIER_MENU,
	/* 208 */	SORT_HIER_MENU,
	/* 209 */	STATE_HIER_MENU,
	/* 210 */	PRIOR_HIER_MENU,
	/* 211 */	LABEL_HIER_MENU,
	/* 212 */	CHANGE_HIER_MENU = 212,
	/* 213 */	TABLE_HIER_MENU,
	/* 214 */	SIG_HIER_MENU,
	/* 215 */	TIME_UNITS_HIER_MENU,
	/* 216 */	COLOR_HIER_MENU,
	/* 217 */	FONT_HIER_MENU,
	/* 218 */	MARGIN_HIER_MENU,
	/* 219 */ TEXT_HIER_MENU,
	/* 220 */ TLATE_SEL_HIER_MENU,
//#ifndef	LIGHT	//No Personalities menu in Light
	/* 221 */	PERS_HIER_MENU,
//#else
//	/* 221 */	UNUSED_PERS_HIER_MENU,
//#endif	//LIGHT
	/* 222 */ TLATE_ATT_HIER_MENU,
	/* 223 */ TLATE_SET_HIER_MENU,
	/* 224 */ WIND_PROP_HIER_MENU,
	/* 225 */ SERVER_HIER_MENU,
	/* 226 */ TEXT_SIZE_HIER_MENU,
	/* 227 */ SPELL_HIER_MENU,
	/* 228 */ AD_SELECT_HIER_MENU,	//	Only used in debug version	
	/* 229 */ CONCON_PROF_PREVIEW_HIER_MENU,
	/* 230 */ EMOTICON_HIER_MENU,
	HIER_MENU_LIMIT
} HierMenuEnum;

#define BOX_MENU_START 4096
#define BOX_MENU_LIMIT 32768
#define OLD_MAX_BOX_LEVELS 100
#define MAX_BOX_LEVELS gMaxBoxLevels

typedef enum {MAILBOX,TRANSFER,MENU_ARRAY_LIMIT} MTypeEnum;

typedef enum {spellCheckItem=1,spellNextItem,spellBar1Item,spellAddItem,spellRemoveItem,spellItemLimit} SpellHierEnum;
typedef enum {shmStandard=1,shmAlternate,shmBar,shmNew} SigHierEnum;

typedef enum {
	APPLE_ABOUT_ITEM=1,
	APPLE_ABOUT_TRANS_ITEM,
	APPLE_MENU_LIMIT
} AppleMenuEnum;

typedef enum {
	FILE_NEW_ITEM=1,
	FILE_OPENDOC_ITEM,
	FILE_OPENSEL_ITEM,
	FILE_BROWSE_ITEM,
	/*------------*/ FILE_BAR1_ITEM,
	FILE_CLOSE_ITEM,
	FILE_SAVE_ITEM,
	FILE_SAVE_AS_ITEM,
	/*------------*/ FILE_BAR2_ITEM,
	FILE_IMPORT_MAIL_ITEM,
	FILE_EXPORT_ITEM,
	/*------------*/ FILE_BAR3_ITEM,
	FILE_SEND_ITEM,
	FILE_CHECK_ITEM,
	/*------------*/ FILE_BAR4_ITEM,
	FILE_PAGE_ITEM,
	FILE_PRINT_ITEM,
	FILE_PRINT_ONE_ITEM,
	/*------------*/ FILE_BAR5_ITEM,
	FILE_QUIT_ITEM,
	FILE_MENU_LIMIT
} FileMenuEnum;

typedef enum {
	EDIT_UNDO_ITEM=1,
	/*------------*/ EDIT_BAR1_ITEM,
	EDIT_CUT_ITEM,
	EDIT_COPY_ITEM,
	EDIT_PASTE_ITEM,
	EDIT_QUOTE_ITEM,
	EDIT_CLEAR_ITEM,
	/*------------*/ EDIT_BAR2_ITEM,
	EDIT_TEXT_ITEM,
	/*------------*/ EDIT_BAR3_ITEM,
	EDIT_SELECT_ITEM,
	EDIT_WRAP_ITEM,
	EDIT_FINISH_ITEM,
	EDIT_INSERT_TO_ITEM,
	EDIT_INSERT_EMOTICON_ITEM,
	/*------------*/ EDIT_BAR4_ITEM,
	EDIT_PROCESS_ITEM,
	EDIT_SPEAK_ITEM,
#ifdef DIAL
	EDIT_DIAL_ITEM,
#endif
#ifdef WINTERTREE
	EDIT_ISPELL_HOOK_ITEM,
#endif
	EDIT_SPELL_ITEM,
	EDIT_MENU_LIMIT
} EditMenuEnum;

typedef enum {
	MESSAGE_NEW_ITEM=1,
	MESSAGE_REPLY_ITEM,
	MESSAGE_FORWARD_ITEM,
	MESSAGE_REDISTRIBUTE_ITEM,
	MESSAGE_SALVAGE_ITEM,
	/*------------*/ MESSAGE_BAR0_ITEM,
	MESSAGE_NEW_TO_ITEM,
	MESSAGE_FORWARD_TO_ITEM,
	MESSAGE_REDIST_TO_ITEM,
//#ifndef LIGHT
	/*------------*/ MESSAGE_BAR1_ITEM,
	MESSAGE_NEW_WITH_ITEM,
	MESSAGE_REPLY_WITH_ITEM,
//#endif	//LIGHT
	/*------------*/ MESSAGE_BAR2_ITEM,
	MESSAGE_ATTACH_ITEM,
	MESSAGE_ATTACH_TLATE_ITEM,
	MESSAGE_QUEUE_ITEM,
	MESSAGE_CHANGE_ITEM,
	/*------------*/ MESSAGE_BAR3_ITEM,
	MESSAGE_JUNK_ITEM,
	MESSAGE_NOTJUNK_ITEM,
	/*------------*/ MESSAGE_BAR4_ITEM,
	MESSAGE_DELETE_ITEM,
	MESSAGE_MENU_LIMIT
} MessageMenuEnum;

typedef enum {
	CHANGE_QUEUEING_ITEM=1,
#ifndef ONE
	CHANGE_STATUS_ITEM,
#endif
	CHANGE_PRIOR_ITEM,
	CHANGE_LABEL_ITEM,
//#ifndef LIGHT //No Change Label item in Light, or personality
	CHANGE_PERS_ITEM,
//#endif
	CHANGE_SERVER_ITEM,
	CHANGE_TABLE_ITEM,
	CHANGE_MENU_LIMIT
} ChangeMenuEnum;


typedef enum {
	MAILBOX_IN_ITEM=1,
	MAILBOX_OUT_ITEM,
	MAILBOX_JUNK_ITEM,
	MAILBOX_TRASH_ITEM,
	/*------------*/ MAILBOX_BAR1_ITEM,
#ifndef ONE
	MAILBOX_NEW_ITEM,
	MAILBOX_OTHER_ITEM,
#endif
	MAILBOX_FIRST_USER_ITEM,
	MAILBOX_MENU_LIMIT
}MailboxMenuEnum;

typedef enum {
	TRANSFER_IN_ITEM=1,
#ifndef ONE
	TRANSFER_OUT_ITEM,
#endif
	TRANSFER_JUNK_ITEM,
	TRANSFER_TRASH_ITEM,
	/*------------*/ TRANSFER_BAR1_ITEM,
	TRANSFER_NEW_ITEM,
	TRANSFER_OTHER_ITEM,
	TRANSFER_FIRST_USER_ITEM,
	TRANSFER_MENU_LIMIT
} TransferMenuEnum;

typedef enum {
	SPECIAL_FILLER_ITEM,
	SPECIAL_FILTER_ITEM,
	SPECIAL_MAKE_NICK_ITEM,
	SPECIAL_MAKEFILTER_ITEM,
	SPECIAL_FIND_ITEM,
	SPECIAL_SORT_ITEM,
	/*------------*/ SPECIAL_BAR1_ITEM,
	SPECIAL_SETTINGS_ITEM,
	SPECIAL_TLATE_SETTINGS_ITEM,
#ifdef CTB
	SPECIAL_CTB_ITEM,
#endif
	/*------------*/ SPECIAL_BAR3_ITEM,
	SPECIAL_CHANGE_ITEM,
	SPECIAL_FORGET_ITEM,
	SPECIAL_TRASH_ITEM,
	/*------------*/ SPECIAL_BAR4_ITEM,
	SPECIAL_MENU_LIMIT
} SpecialMenuEnum;

typedef enum
{
	WIN_FILLER_ITEM,
	WIN_MINIMIZE_ITEM,
	WIN_BRINGALLTOFRONT_ITEM,
	WIN_BEHIND_ITEM,
	WIN_PROPERTIES_ITEM,
	WIN_DRAWER_ITEM,
	/*------------*/ WIN_BAR2_ITEM,
	WIN_ALIASES_ITEM,
	WIN_PH_ITEM,
	WIN_FILTERS_ITEM,
	WIN_LINK_ITEM,
	WIN_MAILBOX_ITEM,
//#ifndef	LIGHT	//No multiple personatlities in Light
	WIN_PERSONALITIES_ITEM,
//#endif
	WIN_SIGNATURES_ITEM,
//#ifndef	LIGHT	//No stationery in Light
	WIN_STATIONERY_ITEM,
//#endif
	WIN_STATISTICS_ITEM,
#ifdef TASK_PROGRESS_ON
	WIN_TASKS_ITEM,
#endif
	/*------------*/ WIN_BAR_ITEM,
	WIN_LIMIT
} WinMenuEnum;

typedef enum {
	FIND_FIND_ITEM=1,
	FIND_AGAIN_ITEM,
	FIND_ENTER_ITEM,
	FIND_BAR1_ITEM,
	FIND_SEARCH_ITEM,
	FIND_SEARCH_ALL_ITEM,
	FIND_SEARCH_BOX_ITEM,
	FIND_SEARCH_FOLDER_ITEM,
	FIND_BAR2_ITEM,
	FIND_SEARCH_WEB_ITEM,
	FIND_MENU_LIMIT
} FindMenuEnum;

typedef enum {
	SORT_STATUS_ITEM=1,
	SORT_JUNK_ITEM,
	SORT_PRIORITY_ITEM,
	SORT_ATTACHMENTS_ITEM,
	SORT_LABEL_ITEM,
	SORT_SENDER_ITEM,
	SORT_TIME_ITEM,
	SORT_SIZE_ITEM,
	SORT_MAILBOX_ITEM,
	SORT_MOOD_ITEM,
	SORT_SUBJECT_ITEM,
	SORT_BAR1_ITEM,
	SORT_GROUP_ITEM,
	SORT_BAR2_ITEM,
	SORT_LIMIT
} SortMenuEnum;

typedef enum {
	pymPriorLabel=0,
	pymHighest,
	pymHigh,
	pymNormal,
	pymLow,
	pymLowest,
	pymBar1,
	pymRaise,
	pymLower,
	pymLimit
} PriorityMenuEnum;

typedef enum {
	atmDouble=1,
	atmSingle,
	atmHQX,
	atmUU,
	atmLimit
} ATypeMenuEnum;

typedef enum
{
	statmUnread=1,
	statmRead,
	statmReplied,
	statmForwarded,
	statmRedirected,
	statmRecovered,
	statmBar,
	statmUnsendable,
	statmSendable,
	statmQueued,
	statmTimed,
	statmSent,
	statmUnsent,
	statmMesgError,
	statmBusySending,
	statmLimit
} StateMenuEnum;

#ifndef ONE
typedef enum {
	sigmNone=1,
	sigmStandard,
	sigmAlternate,
	sigmLimit
} SigMenuEnum;

typedef enum {
	wpmTabs=1,
	wpmFloating,
	wpmDockable
} WinPropMenuEnum;
#endif

#ifdef TASK_PROGRESS_ON
typedef enum {
	NETWORK_NO_CONNECTION=1,
	NETWORK_NO_BATTERIES,
	NETWORK_GO_OFFLINE
} NetworkMenuEnum;
#endif

typedef enum {
	OPTIONS_RESYNC_ITEM=1,
	OPTIONS_RESYNCSUB_ITEM,
	OPTIONS_AUTOSYNC_ITEM,
	OPTIONS_BAR1_ITEM,
	OPTIONS_REFRESH_ITEM,
	OPTIONS_BAR2_ITEM,
	OPTIONS_SHOWDELETED_ITEM,
	OPTIONS_EXPUNGE_ITEM,
	OPTIONS_COMPACT_ITEM
} MailboxOptionsMenuEnum;
	

/**********************************************************************
 * Window templates
 **********************************************************************/
typedef enum {
  /* 1001 */	MAILBOX_WIND = 1001,
  /* 1002 */	MESSAGE_WIND,
  /* 1003 */	PROGRESS_WIND,
  /* 1004 */	ALIAS_WIND,
  /* 1005 */	FIND_WIND,
  /* 1006 */	PH_WIND,
  /* 1007 */	MBWIN_WIND,
  /* 1008 */	FILT_WIND,
  /* 1009 */	TBAR_WIND,
  /* 1010 */	ETL_ABOUT_WIND,
  /* 1011 */	PERSONALITIES_WIND,
  /* 1012 */	SIGNATURES_WIND,
  /* 1013 */	STATIONERY_WIND,
  /* 1014 */	THREADED_PROGRESS_WIND,
  /* 1015 */	PROP_WIND,
  /* 1016 */	TASKS_WIND,
  /* 1017 */	LOG_WIND,
  /* 1018 */	HELP_WIND,
  /* 1019 */	SEARCH_WIND,
  /* 1020 */	AD_WIND,
  /* 1021 */	LINK_WIND,
  /* 1022 */	PAY_WIND,
  /* 1023 */	STAT_WIND,
  /* 1024 */	IMPORTER_WIND,
  /* 1025	*/	TOOLBAR_POPUP_WIND,
	WIND_LIMIT
} WindEnum;


/************************************************************************
 * 
 ************************************************************************/
typedef enum {
	TZ_NAMES=1001,
	TZ_LIMIT
} TZEnum;

#pragma mark Controls
/**********************************************************************
 * controls
 **********************************************************************/
typedef enum {
  /* 1001 */	SCROLL_CNTL=1001,
  /* 1002 */	NEW_ALIAS_CNTL,
  /* 1003 */	REMOVE_ALIAS_CNTL,
  /* 1004 */	PH_CNTL,
  /* 1005 */	MAKE_TO_CNTL,
  /* 1006 */	INSERT_CNTL,
  /* 1007 */	FONT_NAME_CNTL,
  /* 1008 */	PRINT_FONT_CNTL,
  /* 1009 */	SEND_NOW_CNTL,
  /* 1010 */	FIXED_FONT_POPUP_CNTL,
  /* 1011 */	UNUSED_CNTL_2,	//	TO_MB_CNTL,
  /* 1012 */	FINGER_CNTL,
  /* 1013 */	NOTIFY_CNTL,
  /* 1014 */  CNTL_14,
  /* 1015 */	FILT_CNTL_BASE,
  /* 1027 */	FILT_CNTL_LIMIT=FILT_CNTL_BASE+12,
  /* 1028 */	NICK_MNU_CNTL,
  /* 1029 */	PH_XLIN_CNTL,
  /* 1030 */	PH_XLOUT_CNTL,
  /* 1031 */	UNUSED_1031_CNTL,
  /* 1032 */	DELETE_CNTL,
  /* 1033 */	FETCH_CNTL,
  /* 1034 */	STOP_CNTL,
  /* 1035 */	NEWMAIL_SND_CNTL,
  /* 1036 */	ERROR_SND_CNTL,
  /* 1037 */	SOME_CNTL_OR_OTHER,
  /* 1038 */	OPEN_BOX_CNTL,
  /* 1039 */	OPEN_MESSAGE_CNTL,
  /* 1040 */	DO_FILTER_CNTL,
  /* 1041 */	DO_USER_CNTL,
  /* 1042 */	TB_CNTL,
  /* 1043 */	LAB8_CNTL,
  /* 1050 */	LAB15_CNTL=1050,
  /* 1051 */	DO_FETCH_CNTL,
  /* 1052 */	DO_TRASH_CNTL,
  /* 1053 */	FILT_SOUND_CNTL,
  /* 1054 */	TB_SEARCH_USER_CNTL,
	/* 1055 */	BG_COLOR_CNTL,
	/* 1056 */  FG_COLOR_CNTL,
	/* 1057 */  PH_SERVER_CNTL,
	/* 1058 */  ETL_OK_CNTL,
	/* 1059 */  SCHIZO_CNTL,
	/* 1060 */  SCHIZO_STATION_CNTL,
	/* 1061 */  SCHIZO_SIG_CNTL,
	/* 1062 */  CHECK_SPECIAL_LIST_CNTL,
	/* 1063 */  CHOOSE_PERSONALITY_LIST_CNTL,
	/* 1064 */  TASK_PROGRESS_STOP_CNTL,
	/* 1065 */  TASK_PROGRESS_FILTER_CNTL,
	/* 1066 */	GENERIC_BEVEL_CNTL,
	/* 1067 */	MB_REMOVE_CNTL,
	/* 1068 */	MB_NEWMB_CNTL,
	/* 1069 */	MB_NEWFOLDER_CNTL,
	/* 1070 */	STNY_EDIT_CNTL,
	/* 1071 */	STNY_NEW_CNTL,
	/* 1072 */	STNY_REMOVE_CNTL,
	/* 1073 */	PERS_EDIT_CNTL,
	/* 1074 */	PERS_NEW_CNTL,
	/* 1075 */	PERS_REMOVE_CNTL,
	/* 1076 */	SIG_NEW_CNTL,
	/* 1077 */	SIG_REMOVE_CNTL,
	/* 1078 */	PRIOR_CNTL,
	/* 1079 */	SIG_CNTL,
	/* 1080 */	ATTACH_CNTL,
	/* 1081 */	ICON_BAR_CNTL,
	/* 1082 */	BOX_HEAD_CNTL,
	/* 1083 */	NICK_HZOOM_CNTL,
	/* 1084 */	TOPMARGIN_CNTL,
	/* 1085 */	TEXT_BAR_FONT_CNTL,
	/* 1086 */  TEXT_BAR_SIZE_CNTL,
	/* 1087 */	TEXT_BAR_COLOR_CNTL,
	/* 1088 */	BAD_TOPMARGIN_CNTL,
	/* 1089 */	OBSOLETE_LDAP_CNTL,
	/* 1090 */	PERS_CHECK_CNTL,
	/* 1091 */	FILT_CNTL_2ND_BASE,
	/* 1099 */	FILT_CNTL_2ND_LIMIT=1099,
	/* 1100 */	FILT_LABEL_POP,
	/* 1101 */	FILT_PERS_POP,
	/* 1102 */	FILT_REPLY_POP,
	/* 1103 */	SIG_EDIT_CNTL,
	/* 1104 */	BOX_SIZE_CNTL,
	/* 1105 */	SEP_CNTL,
	/* 1106 */	SEP2_CNTL,
	/* 1107 */	SEP3_CNTL,
	/* 1108 */	QUOTE_COLOR_CNTL,
	/* 1109 */	PREVIEW_TOGGLE_CNTL,
	/* 1110 */	PREVIEW_DIVIDE_CNTL,
	/* 1111 */  NICK_RECIPIENT_CNTL,
	/* 1112 */	NICK_INFO_LIST_CNTL,
	/* 1113 */  NICK_BUSINESS_CARD_CNTL,
	/* 1114 */  MB_REFRESH_CNTL,
	/* 1115 */	FILT_SPEAK_SENDER_CNTL,
	/* 1116 */	FILT_SPEAK_SUBJECT_CNTL,
	/* 1117	*/	FILT_SPEAK_POPUP_CNTL,
	/* 1118 */	PLACARD_CNTL,	// just a placard
	/* 1119 */	MAKE_FILT_FROM_PETE,
	/* 1120 */	MAKE_FILT_ANY_PETE,
	/* 1121 */	MAKE_FILT_SUBJECT_PETE,
	/* 1122 */	MAKE_FILT_TRANSFER_PETE,
	/* 1123 */	NAG_INTRO_TITLE,
	/* 1124	*/	NAG_PLEASE_REGISTER_TITLE,
	/* 1125	*/	PAY_WHICH_EUDORA_TITLE,
	/* 1126	*/	PAY_KEEP_CURRENT_TITLE,
	/* 1127	*/	PAY_REG_INFO_TITLE,
	/* 1128	*/	NAG_UPDATE_TITLE,
	/* 1129	*/	NAG_FEATURES_LIST_CNTL,
	/* 1130	*/	NAG_FEATURES_TITLE,
	/* 1131 */	NAG_NOT_GETTING_ADS_TITLE,
	/* 1132 */	NAG_NO_ADS_TIME_TO_DOWNGRADE_TITLE,
	/* 1133 */	MB_RESYNC_CNTL,
	/* 1134 */	LINK_NEW_CNTL,
	/* 1135 */	LINK_REMOVE_CNTL,
	/* 1136 */	LINK_THUMB_COL_CNTL,
	/* 1137 */	LINK_NAME_COL_CNTL,
	/* 1138 */	LINK_DATE_COL_CNTL,
	/* 1139 */	AUDIT_GROUP1_CNTL,
	/* 1140 */	AUDIT_GROUP2_CNTL,
	/* 1141 */	AD_OBSCURE_GROUP_CNTL,
	/* 1142 */	CANT_GET_THERE_FROM_HERE_TITLE,
	/* 1143 */	REMIND_ME_TITLE,
	/* 1144	*/	MORE_INFO_CNTL,
	/* 1145	*/	PRE_PAYMENT_TITLE,
	/* 1146 */	PRE_REGISTRATION_TITLE,
	/* 1147 */	PRE_PROFILING_TITLE,
	/* 1148 */	PROFILE_ID_CONFIRM_TITLE,
	/* 1149 */	STAT_TIME_MENU_CNTL,
	/* 1150 */	STAT_MORE_CNTL,
	/* 1151 */	STAT_CURRENT_COLOR,
	/* 1152 */	STAT_PREVIOUS_COLOR,
	/* 1153 */	STAT_AVERAGE_COLOR,
	/* 1154 */	STAT_CURRENT_TYPE,
	/* 1155 */	STAT_PREVIOUS_TYPE,
	/* 1156 */	STAT_AVERAGE_TYPE,
	/* 1157 */	NICK_REALNAME_EDIT_CNTL,
	/* 1158 */	NICK_NICKNAME_EDIT_CNTL,
	/* 1159 */	NICK_FISTNAME_EDIT_CNTL,
	/* 1160 */	NICK_LASTNAME_EDIT_CNTL,
	/* 1161 */	NICK_ADDRESSES_EDIT_CNTL,
	/* 1162 */	NICK_NAME_GROUP_CNTL,
	/* 1163	*/	NICK_SWAP_NAMES_CNTL,
	/* 1164	*/	GROUP_ADDRESSES_EDIT_CNTL,
	/* 1165 */	GROUP_NAME_GROUP_CNTL,
	/* 1166 */ 	IMPORT_IMPORT_CNTL,
	/* 1167 */ 	IMPORT_SELECT_CNTL,
  /* 1168 */	IMPORTER_SELECT_CNTL,
  /* 1169 */	AB_NEW_NICKNAME_CNTL,
  /* 1170 */	AB_NEW_ADDRESSBOOK_CNTL,
  /* 1171 */	AB_REMOVE_CNTL,
  /* 1172 */	PROFILE_REQ_CNTL,
  /* 1173 */	PROFILE_REQ2_GROUP_CNTL,
  /* 1174 */	PROFILE_REQ2_FEAT_CNTL,
  /* 1175 */	hmmmmm____in_common_dot_r_but_not_reserved_here,
  /* 1176 */	hmmmmm____in_common_dot_r_but_not_reserved_here_two,
  /* 1177 */	REPAY_GROUP_CNTL,
  /* 1178 */	URL_COLOR_CNTL,
  /* 1179 */	SPELL_COLOR_CNTL,
  /* 1180 */	TIGHT_ANAL_COLOR_CNTL,
  /* 1181 */	LOOSE_ANAL_COLOR_CNTL,
  /* 1182 */ 	IMPORT_OTHER_CNTL,
  /* 1183 */	FV_NAME_CNTL,  
  /* 1184 */	FV_DATE_CNTL,  
  /* 1185 */	FV_SIZE_CNTL,  
  /* 1186 */	POP_SSL_CNTL,  
  /* 1187 */	SMTP_SSL_CNTL,  
  /* 1188 */	IMAP_SSL_CNTL,  
  /* 1189 */	SSL_CERT_GROUP_CNTL, 
  /* 1190 */	TP_HELP_BUTTON, 
	/* 1191 (was 1098) */	FILT_STATUS_POP,
	/* 1192 (was 1099) */	FILT_PRIORITY_POP,
	/* 1193 */	SLIDER_CNTL,
	/* 1194 */	JUNK_INTRO_GROUP_CNTL,
	/* 1195 */	PROXY_POPUP_CNTL,
	/* 1196 */	CONCON_PREVIEW_POP_CNTL,
	/* 1197 */	CONCON_MESSAGE_POP_CNTL,
	/* 1198 */	CONCON_MULTI_POP_CNTL,
	/* 1199 */	PERS_SMTP_RELAY_CNTL,
	/* 1200 */	JUNKDOWN_GROUP_CNTL,
	/* 1201 */	CONCON_PROF_PREVIEW_POP,
	/* 1202 */	TB_SEARCH_GO_CNTL,
	/* 1203 */	TB_SEARCH_POPUP,
	/* 1204 */	FILTER_ACTION_GROUP_CNTL,
	/* 1205 */  FILTER_MATCH_GROUP_CNTL,
  /* 1501 */	LIVE_SCROLL_CNTL=1501,
	CNTL_LIMIT
} CntlEnum;



/************************************************************************
 * sounds
 ************************************************************************/
typedef enum {
	NEW_MAIL_SND=1001,
	ATTENTION_SND,
	SHORT_WARN_SND,
	DIAL_0_SND,
	DIAL_1_SND,
	DIAL_2_SND,
	DIAL_3_SND,
	DIAL_4_SND,
	DIAL_5_SND,
	DIAL_6_SND,
	DIAL_7_SND,
	DIAL_8_SND,
	DIAL_9_SND,
	DIAL_STAR_SND,
	DIAL_POUND_SND,
	SND_LIMIT
} SndEnum;

/************************************************************************
 * list definitions (mapped one to one with any 'ldes' resources)
 ************************************************************************/
typedef enum {
	LISTVIEW_LDEF=1001,
	NICKWIN_LDEF,
	ICON_LDEF,
	TLATE_LDEF,
	FILT_LDEF,
	TASK_LDEF,
	FEATURE_LDEF,
	LDEF_LIMIT
} LDEFEnum;

/************************************************************************
 * Patterns
 ************************************************************************/
typedef enum {
	OFFSET_GREY=1001,
	PAT_LIMIT
}	PATEnum;

/************************************************************************
 * Pixel patterns
 ************************************************************************/
typedef enum {
	DRAG_BAR_PIXPAT=1001,
	PIXPAT_LIMIT
}	PIXPATEnum;

/************************************************************************
 * PICT resources
 ************************************************************************/
typedef enum {
	ROOSTER_PICT=1001,
	UIUC_PICT,
	COPYRIGHT_PICT,
	EUDORA_PICT,
	LINE_PICT,
	QUALCOMM_PICT,
	STATUS_HELP_PICT,
	SNAKE_PICT,
	PRIOR_HELP_PICT,
	SOME_RANDOM_PICT,
	GREYLINE_PICT,
	FILT_LIST_HELP_PICT=1021,
	IMAP_BOX_SERVER_COLUMN_HELP_PICT=1022,
	GRAY_LOGO_PICT,
	HELP_PICT_BASE=2000,
	PICT_LIMIT
} PICTEnum;

/************************************************************************
 * CDEF's	
 ************************************************************************/
typedef enum {
	COL_CDEF = 1002,
	UNUSED_CDEF,
	LIST_CDEF,
	DEBUG_CDEF,
	APP_CDEF, // this caused crashes in LaserWriter 8 Page setup if its id was 1001
	CDEF_LIMIT
} CDEFEnum;

/************************************************************************
 * TEXT resources
 ************************************************************************/
typedef enum {
  /* 1001 */	DAEMON_TEXT=1001,
  /* 1002 */	INTRO_WINDOW_TEXT,
  /* 1003 */	REGISTRATION_NAG_TEXT,
  /* 1004 */	FREEWARE_NAG_HEADER_TEXT,
  /* 1005 */	AUDIT_SEND_INTRO,
  /* 1006 */	FREEWARE_NAG_FOOTER_TEXT,
  /* 1007 */	FEATURES_NAG_HEADER_TEXT,
  /* 1008 */	FEATURES_NAG_FOOTER_TEXT,
  /* 1009 */	NOT_GETTING_ADS_TEXT,
  /* 1010 */	NOT_GETTING_ADS_TIME_TO_DOWNGRADE_TEXT,
  /* 1011 */	NOT_GETTING_ADS_BOLD_WARNING_TEXT,
  /* 1012 */	AD_OBSCURE_TEXT,
  /* 1013 */	AUDIT_LEGEND_TEXT,
  /* 1014 */	PRE_PAYMENT_TEXT,
  /* 1015 */	PRE_REGISTRATION_TEXT,
  /* 1016 */	PRE_PROFILING_TEXT,
  /* 1017 */  used_but_not_defined_here,
  /* 1018 */	REPAY_TEXT,
  /* 1019 */	JUNK_INTRO_TEXT,
  /* 1020 */	CONCON_PROFILES_TEXT,
  /* 1021 */	JUNKDOWN_TEXT,
  /* 1022 */	SUGGEST_TEXT,
  /* 1023 */	BUG_TEXT,
	TEXT_LIMIT
} TEXTREnum;


/************************************************************************
 * resource for saving POP descriptors
 ************************************************************************/
#define OLD_POPD_TYPE 'popd'
#define POPD_ID		1001
#define FETCH_ID	1002
#define DELETE_ID	1003

#define TOC_FLAVOR	'tocH'
#define MESS_FLAVOR	'mesH'
#define A822_FLAVOR	'a822'
#define SPEC_FLAVOR	'SPEC'

/************************************************************************
 * tab# resources
 ************************************************************************/
typedef enum {
	SEARCH_WIN_TABS=1001,
	ADDRESS_BOOK_TABS,
	FILE_VIEW_TABS,
	TAB_LIMIT
} TABREnum;


/************************************************************************
 * eTAB resources (tabs on steroids the Eudora nicknames way)
 ************************************************************************/
#define	TAB_TYPE					'eTAB'		// See 'TabResourceRec' in tabgeometry.h


#ifdef NAG
//
//	Nag resources
//

#define	NAG_TYPE					'Nag '		// See 'NagRec'
#define	NAG_LIST_TYPE			'Nag#'		// See 'NagStateRec'
#define	NAG_USAGE_TYPE		'NgU2'		// See 'NagUsageRec'
#define NAG_REQUEST_TYPE	'pRNg'		// Record of nag that playlist manager wants

//
//	Nags that are periodic
//
typedef enum {
	INTRO_NAG = 1001,					// 1001
	REGISTRATION_NAG,					// 1002
	FEATURE_NAG,							// 1003
	UPDATE_NAG,								// 1004
	UPDATE_CHECK,							// 1005
	AD_FAILURE_CHECK,					// 1006
	NAG_LIMIT
} NagEnum;

//
//	User states
//

typedef enum {
	NAG_EP4_USER = 1001,							// 1001
	NAG_EP4_REG_USER,									// 1002
	NAG_NEW_USER,											// 1003
	NAG_PAID_USER,										// 1004
	NAG_FREE_USER,										// 1005
	NAG_AD_USER,											// 1006
	NAG_REG_FREE_USER,								// 1007
	NAG_REG_ADD_USER,									// 1008
	NAG_DEADBEAT_USER,								// 1009
	defunct_NAG_OLD_USER,							// 1010
	NAG_BOX_USER,											// 1011
	NAG_PROFILE_DEADBEAT_USER,				// 1012
	NAG_UNPAID_USER,									// 1013
	NAG_REPAY_USER,										// 1014
	NAG_STATE_LIMIT
} NagStateEnum;

#define	NAG_STATE_TYPE		'NgS2'		// See 'NagStateRec'

#define	NAG_STATE_ID			1001


#endif

#define REG_PREF_TYPE				'EuUD' // As in "user data"; 3 C-strings concatenated: first name, last name, reg code
#define REG_PREF_BASE_ID		1000
#define	REG_PREF_SPONSORED	1032
#define	REG_PREF_LIGHT			1033
#define	REG_PREF_PAID				1034

#define USER_PROFILE_TYPE	'EuPF' // As in "ProFile"; it's a bucket of bits just as it came in MIME
#define USER_PROFILE_ID		1000

#define	TAG_MAP_TYPE			'TGMP'		// See NicknameTagMapRec in nickmng.h

#define SEARCH_SAVED_SORT_ID	666
#endif