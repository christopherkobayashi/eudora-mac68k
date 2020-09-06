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

#include <Types.h>
#include "PrefDefs.h"
#define Audit	true
Boolean PrefAudit(short prefN)
{
	switch (ABS(prefN))
	{
		case PREF_FONT_NAME: return(Audit);
		case PREF_FONT_SIZE: return(Audit);
		case PREF_INTERVAL: return(Audit);
		case PREF_WRAP_OUT: return(Audit);
		case PREF_PPP_DISC: return(Audit);
		case PREF_PARAGRAPHS: return(Audit);
		case PREF_LASTSCAN: return(Audit);
		case PREF_SEND_CHECK: return(Audit);
		case PREF_ZOOM_OPEN: return(Audit);
		case UNUSED_PREF_AUTO_DISMISS: return(Audit);
		case PREF_AUTO_EMPTY: return(Audit);
		case PREF_UNWRAP_OPTIONS: return(Audit);
		case PREF_LMOS: return(Audit);
		case PREF_KEEP_OUTGOING: return(Audit);
		case PREF_NEW_ALERT: return(Audit);
		case PREF_NEW_SOUND: return(Audit);
		case PREF_SIG: return(Audit);
		case PREF_AUTO_SEND: return(Audit);
		case PREF_BX_TEXT: return(Audit);
		case PREF_TRANS_METHOD: return(Audit);
		case PREF_TAB_IN_TEXT: return(Audit);
		case PREF_PRINT_FONT: return(Audit);
		case PREF_PRINT_FONT_SIZE: return(Audit);
		case PREF_NO_APPLE_FLASH: return(Audit);
		case PREF_CAN_PIPELINE: return(Audit);
		case PREF_EASY_DEL_UNREAD: return(Audit);
		case PREF_SUPERCLOSE: return(Audit);
		case PREF_SENSITIVE: return(Audit);
		case PREF_MWIDTH: return(Audit);
		case PREF_MHEIGHT: return(Audit);
		case PREF_EXCLUDE_HEADERS: return(Audit);
		case PREF_SWITCH_MODIFIERS: return(Audit);
		case PREF_AUTOHEX: return(Audit);
		case PREF_WARN_RICH: return(Audit);
		case PREF_NO_BIGGIES: return(Audit);
		case PREF_NICK_AUTO_EXPAND: return(Audit);
		case PREF_SUM_ONLY: return(Audit);
		case PREF_NO_OPEN_IN: return(Audit);
		case PREF_MANUAL_BOX_SELECTION: return(Audit);
		case PREF_IGNORE_OUTDATE: return(Audit);
		case PREF_REPLY_ALL: return(Audit);
		case PREF_NOT_ME: return(Audit);
		case PREF_APOP: return(Audit);
		case PREF_NW_DEV: return(Audit);
		case PREF_CORVAIR: return(Audit);
		case PREF_NO_RF_TOC_BACKUP: return(Audit);
		case PREF_LOCK_CODE: return(Audit);
		case PREF_BOX_HIDDEN: return(Audit);
		case PREF_LOCK_REDIR: return(Audit);
		case PREF_RECEIPT: return(Audit);
		case PREF_RRT: return(Audit);
		case PREF_NO_DELKEY: return(Audit);
		case PREF_POP_MODE: return(Audit);
		case PREF_OLD_DIGEST: return(Audit);
		case PREF_LOG: return(Audit);
		case PREF_NO_IN_PRIOR: return(Audit);
		case PREF_NO_XF_PRIOR: return(Audit);
		case PREF_IN_XLATE: return(Audit);
		case PREF_OUT_XLATE: return(Audit);
		case PREF_NO_AUTO_OPEN: return(Audit);
		case PREF_NO_LINES: return(Audit);
		case PREF_HEX_PERMISSIVE: return(Audit);
		case PREF_PROMISCUOUS_REATTACH: return(Audit);
		case PREF_POP3_LIMIT: return(Audit);
		case PREF_PERS_NO_SEND: return(Audit);
		case PREF_OFFLINE: return(Audit);
		case PREF_NO_FLATTEN: return(Audit);
		case PREF_TIDY_FOLDER: return(Audit);
		case PREF_POP_SEND: return(Audit);
		case PREF_PH_IN: return(Audit);
		case PREF_PH_OUT: return(Audit);
		case PREF_NO_QP: return(Audit);
		case PREF_CC_REPLY: return(Audit);
		case PREF_SMALL_COLOR: return(Audit);
		case PREF_LOCAL_DATE: return(Audit);
		case PREF_NO_EZ_OPEN: return(Audit);
		case PREF_LAST_SCAN: return(Audit);
		case UNUSED_PREF_OLD_USE_STATUS: return(Audit);
		case PREF_LMOS_XDAYS: return(Audit);
		case PREF_SERVER_DEL: return(Audit);
		case PREF_IGNORE_GX: return(Audit);
		case PREF_CHECK_HOURS: return(Audit);
		case PREF_DNS_BALANCE: return(Audit);
		case PREF_BINHEX: return(Audit);
		case PREF_SINGLE: return(Audit);
		case PREF_DOUBLE: return(Audit);
		case PREF_UUENCODE: return(Audit);
		case PREF_OLD_BUILDMENU: return(Audit);
		case PREF_EDIT_FROM: return(Audit);
		case PREF_TURBO_REDIRECT: return(Audit);
		case PREF_KERBEROS: return(Audit);
		case PREF_DONT_PASS: return(Audit);
		case PREF_NO_BATT_CHECK: return(Audit);
		case PREF_FIXED_DATES: return(Audit);
		case PREF_SYNC_IO: return(Audit);
		case PREF_SHOW_ALL: return(Audit);
		case PREF_NO_PROGRESS: return(Audit);
		case PREF_F_LIST_WIDE: return(Audit);
		case PREF_NEW_TOC: return(Audit);
		case PREF_127: return(Audit);
		case PREF_REPORT: return(Audit);
		case PREF_TOOLBAR: return(Audit);
		case PREF_TB_VARIATION: return(Audit);
		case PREF_TB_VERT: return(Audit);
		case PREF_WIPE: return(Audit);
		case PREF_3D: return(Audit);
		case PREF_LAST_SCREEN: return(Audit);
		case PREF_EASY_DEL_QUEUED: return(Audit);
		case PREF_EASY_DEL_UNSENT: return(Audit);
		case PREF_SUBJECT_WARNING: return(Audit);
		case PREF_EASY_QUIT: return(Audit);
		case PREF_AUTO_FCC: return(Audit);
		case PREF_NO_DRAG: return(Audit);
		case PREF_PPP_NOAUTO: return(Audit);
		case PREF_MA: return(Audit);
		case PREF_GEN_IMPORTANCE: return(Audit);
		case PREF_SUP_PRIORITY: return(Audit);
		case PREF_UNCOMMA: return(Audit);
		case PREF_NEWSGROUP_HANDLING: return(Audit);
		case PREF_HEAD_RETURN: return(Audit);
		case PREF_EZ_SAVE: return(Audit);
		case PREF_SLOW_RESORT: return(Audit);
		case PREF_NO_TRASH_WARN: return(Audit);
		case PREF_AUTO_CHECK: return(Audit);
		case PREF_IGNORE_PPP: return(Audit);
		case PREF_NO_TB_BALL: return(Audit);
		case PREF_SMTP_DOES_AUTH: return(Audit);
		case PREF_GV_BRAINDAMAGE: return(Audit);
		case PREF_DIE_DOG: return(Audit);
		case PREF_NO_LOWER_HESIOD: return(Audit);
		case PREF_SEND_CSET: return(Audit);
		case PREF_INTERPRET_ENRICHED: return(Audit);
		case PREF_DIE_DOG_USER: return(Audit);
		case PREF_DONT_ALIGN_HEADERS: return(Audit);
		case PREF_TOSHIBA_FLUSH: return(Audit);
		case PREF_NO_SCAMWATCH_TIPS: return(Audit);
		case UNUSED_PREF_READ_PEST: return(Audit);
		case PREF_TB_FKEYS: return(Audit);
		case PREF_FURRIN_SORT: return(Audit);
		case PREF_SHOW_FKEYS: return(Audit);
		case PREF_FIND_BACK: return(Audit);
		case PREF_FIND_WORD: return(Audit);
		case PREF_REGISTER: return(Audit);
		case PREF_DRAG_OPTIONS: return(Audit);
		case PREF_INTERPRET_HTML: return(Audit);
		case PREF_SEND_STYLE: return(Audit);
		case PREF_DELDUP: return(Audit);
		case PREF_NO_CHECK_MEM: return(Audit);
		case PREF_INTERPRET_PARA: return(Audit);
		case PREF_AMO_AVOIDANCE: return(Audit);
		case PREF_PH_LIVE: return(Audit);
		case PREF_NO_OFF_OFFER: return(Audit);
		case PREF_JUST_SAY_NO: return(Audit);
		case PREF_REG_VERS: return(Audit);
		case PREF_PRO_FILT_WARN: return(Audit);
		case PREF_SIGFILE: return(Audit);
		case PREF_MB_FILT_WARN: return(Audit);
		case PREF_COMP_TOOLBAR: return(Audit);
		case PREF_NICK_NOSORT: return(Audit);
		case PREF_IGNORE_MX: return(Audit);
		case PREF_NO_SELF_RECEIPT: return(Audit);
		case PREF_NICK_BACKUP: return(Audit);
		case PREF_SPOOL_DESTROY: return(Audit);
		case PREF_POP_SENDHOST: return(Audit);
		case PREF_EVIL_SENDMAIL: return(Audit);
		case PREF_NO_SERVICES: return(Audit);
		case PREF_PPP_FOREGROUND: return(Audit);
		case PREF_DUMB_PASTE: return(Audit);
		case PREF_SLOW_QUIT: return(Audit);
		case PREF_NICK_EXP_TYPE_AHEAD: return(Audit);
		case PREF_NICK_WATCH_IMMED: return(Audit);
		case PREF_NICK_WATCH_WAIT_KEYTHRESH: return(Audit);
		case PREF_NICK_WATCH_WAIT_NO_KEYDOWN: return(Audit);
		case PREF_NICK_POPUP_ON_MULTMATCH: return(Audit);
		case PREF_NICK_POPUP_ON_DEFOCUS: return(Audit);
		case PREF_LIVE_SCROLL: return(Audit);
		case PREF_OPEN_IN_ERR_MESS: return(Audit);
		case PREF_NICK_HILITING: return(Audit);
		case PREF_NICK_TYPE_AHEAD_HILITING: return(Audit);
		case PREF_NO_FUZZY_HELPERS: return(Audit);
		case PREF_ACAP: return(Audit);
		case PREF_THREADING_SEND_OFF: return(Audit);
		case PREF_FIXIFY: return(Audit);
		case PREF_TASK_PROGRESS_AUTO: return(Audit);
		case PREF_NEW_HTML_RENDER: return(Audit);
		case PREF_LDAP_NO_LABEL_TRANS: return(Audit);
		case PREF_LAST_APP_VERSION: return(Audit);
		case PREF_HIGHEST_APP_VERSION: return(Audit);
		case PREF_NO_SUMMARY_ICONS: return(Audit);
		case PREF_NO_2022: return(Audit);
		case PREF_UNUSED_235: return(Audit);
		case PREF_NO_INLINE: return(Audit);
		case PREF_IMAP_DONT_AUTHENTICATE: return(Audit);
		case PREF_LDAP_REQUIRE_BASE_OBJECT: return(Audit);
		case PREF_LDAP_OVERRIDE_DFL_STUB: return(Audit);
		case PREF_NO_NICK_FAST_SAVE: return(Audit);
		case PREF_NO_IE: return(Audit);
		case PREF_ALLOW_8BITMIME: return(Audit);
		case PREF_PASTE_PLAIN: return(Audit);
		case PREF_IS_IMAP: return(Audit);
		case PREF_IMAP_FULL_MESSAGE: return(Audit);
		case PREF_NO_PREVIEW: return(Audit);
		case PREF_NO_PREVIEW_READ: return(Audit);
		case PREF_NO_GRAB_TEXT: return(Audit);
		case PREF_NO_WINTERTREE: return(Audit);
		case PREF_WINTERTREE_OPTS: return(Audit);
		case PREF_USE_LIZZIE: return(Audit);
		case PREF_DONT_FETCH_GRAPHICS: return(Audit);
		case PREF_NO_ATT_COMMENT: return(Audit);
		case PREF_IMAP_NOTHING_BUT_HEAD: return(Audit);
		case PREF_IMAP_FETCH_ATTACHMENTS_WITH_MESSAGE: return(Audit);
		case PREF_IMAP_NO_FANCY_TRASH: return(Audit);
		case PREF_SPEAK_NO_NICE_ADDRESSES: return(Audit);
		case PREF_CMD_DONT_FRONT: return(Audit);
		case PREF_SEARCH_PREVIEW: return(Audit);
		case PREF_FLOWED_BITS: return(Audit);
		case PREF_IMAP_EXTRA_LOGGING: return(Audit);
		case PREF_ATT_SUBFOLDER_TRASH: return(Audit);
		case UNUSED_PREF_SMTP_AUTH: return(Audit);
		case PREF_PRELOAD_NAV: return(Audit);
		case PREF_OPEN_WHERE: return(Audit);
		case PREF_LIST_REPLYTO: return(Audit);
		case PREF_IMAP_NO_FILTER_INBOX: return(Audit);
		case PREF_IMAP_NOT_JUST_MINIMAL_HEADERS: return(Audit);
		case PREF_IMAP_FULL_MESSAGE_AND_ATTACHMENTS: return(Audit);
		case PREF_NO_CUSTOM_MB_ICONS: return(Audit);
		case PREF_IMAP_NO_CONNECTION_MANAGEMENT: return(Audit);
		case PREF_BOMBS_AWAY: return(Audit);
		case PREF_IMAP_TP_BRING_TO_FRONT: return(Audit);
		case PREF_IMAP_SKIP_TOP_LEVEL_INBOX: return(Audit);
		case UNUSED_PREF_USE_LIST_COLORS: return(Audit);
		case PREF_EXCHANGE_FIND_SEARCH: return(Audit);
		case PREF_NO_NOT_FOUND_ALERT: return(Audit);
		case PREF_IMAP_SAVE_ORPHANED_ATT: return(Audit);
		case PREF_TEXT_MOVIES: return(Audit);
		case PREF_ANIMATED_GIFS: return(Audit);
		case PREF_SCHEERDER: return(Audit);
		case PREF_SPEAK_BODY_GYMNASTICS_BITS: return(Audit);
		case PREF_NO_SPOKEN_WARNINGS: return(Audit);
		case PREF_IMAP_NO_EXAMINE_ON_DELETE: return(Audit);
		case PREF_DIAL_MODE: return(Audit);
		case PREF_IMAP_STUPID_PASSWORD: return(Audit);
		case PREF_USE_NETWORK_SETUP: return(Audit);
		case PREF_TB_FLOATING: return(Audit);
		case PREF_NICK_CACHE: return(Audit);
		case PREF_NICK_CACHE_NOT_VISIBLE: return(Audit);
		case PREF_NICK_CACHE_NOT_ADD_REPLY_TO: return(Audit);
		case PREF_NO_PICT_CONVERSION: return(Audit);
		case PREF_NO_LINK_HISTORY: return(Audit);
		case UNUSED_PREF_NO_URL_ACCESS: return(Audit);
		case PREF_NO_FILT_LWSP: return(Audit);
		case PREF_IMAP_VERBOSE_RESYNC: return(Audit);
		case PREF_SEND_ENRICHED_NEW: return(Audit);
		case PREF_SHOW_NUM_SELECTED: return(Audit);
		case PREF_IMAP_DONT_USE_UID_RANGE: return(Audit);
		case PREF_DISABLE_AUTO_UPDATE_CHECK: return(Audit);
		case PREF_NO_ADWIN_DRAG_BAR: return(Audit);
		case PREF_IMAP_EXPUNGE_EXCLUSIVITY: return(Audit);
		case PREF_SMTP_AUTH_NOTOK: return(Audit);
		case PREF_IMAP_POLITE_LOGOUT: return(Audit);
		case PREF_LINK_OFFLINE_WARN: return(Audit);
		case PREF_USE_HTTP_PROXY: return(Audit);
		case PREF_SMTP_GAVE_530: return(Audit);
		case PREF_NICK_CACHE_NOT_ADD_TYPING: return(Audit);
		case PREF_ALWAYS_CHARSET: return(Audit);
		case IMAP_RESYNC_OPEN_MAILBOXES: return(Audit);
		case PREF_NICK_GEN_OPTIONS: return(Audit);
		case PREF_NO_COURIC: return(Audit);
		case PREF_COUNT_ALL_IMAP: return(Audit);
		case PREF_SUPPRESS_NICKS_IN_VIEW_BY: return(Audit);
		case PREF_ALLOW_NICK_RENAME_IN_LIST: return(Audit);
		case PREF_ANCHORED_SELECTION: return(Audit);
		case PREF_NICK_DROP_ON_GROUPS: return(Audit);
		case PREF_NICK_EXPAND_DRAGS: return(Audit);
		case PREF_HTML_TABLES: return(Audit);
		case PREF_NO_MAILBOX_LEVEL_WARNING: return(Audit);
		case PREF_ETL_IGNORE_DELETES: return(Audit);
		case PREF_LAST_NICK_TAB: return(Audit);
		case PREF_CACHE_ATLESS_NICKS: return(Audit);
		case PREF_ALLOW_EMPTY_NICK_EXPANSIONS: return(Audit);
		case PREF_NOT_SMALLER: return(Audit);
		case PREF_NO_INLINE_SIG: return(Audit);
		case PREF_NO_NS_LIB_WARNING: return(Audit);
		case PREF_REDIRECT_MSG_DROPS_ON_TB_NICKS: return(Audit);
		case PREF_CHANGE_BITS_FOR_CONDUIT: return(Audit);
		case PREF_INCLUDE_HIDDEN_NICK_FIELDS_IN_DRAGS: return(Audit);
		case PREF_REMOVE_NICKS_WITH_DELETE_KEY: return(Audit);
		case PREF_VCARD_QUIT_ON_ERROR: return(Audit);
		case PREF_HOME_IS_NICER_THAN_WORK: return(Audit);
		case PREF_ALLOW_IMAP_NUKE: return(Audit);
		case PREF_PERSONAL_NICKNAMES_NOT_VISIBLE: return(Audit);
		case PREF_SSL_POP_SETTING: return(Audit);
		case PREF_HIDE_VCARD_BUTTON: return(Audit);
		case PREF_SSL_SMTP_SETTING: return(Audit);
		case PREF_SSL_IMAP_SETTING: return(Audit);
		case PREF_VCARD: return(Audit);
		case PREF_ALTERNATE_CHECK_MAIL_CMD: return(Audit);
		case PREF_DONT_DISPLAY_PDF: return(Audit);
		case PREF_NO_ALTERNATE_ATTACH_CMD: return(Audit);
		case PREF_URL_ACCESS: return(Audit);
		case PREF_K5_POP: return(Audit);
		case PREF_NO_BULK_SEARCH: return(Audit);
		case PREF_IGNORE_IMAP_ALERTS: return(Audit);
		case PREF_USE_OWN_DOC_HELPERS: return(Audit);
		case PREF_USE_OWN_URL_HELPERS: return(Audit);
		case PREF_NO_COMP_BEAUTIFY: return(Audit);
		case PREF_JUNK_MAILBOX: return(Audit);
		case PREF_FOREGROUND_IMAP_FILTERING: return(Audit);
		case PREF_NO_OUTLOOK_FIX: return(Audit);
		case PREF_PROXY: return(Audit);
		case PREF_DRAWER_WIDTH: return(Audit);
		case PREF_DRAWER_EDGE: return(Audit);
		case PREF_NO_LIVE_RESIZE: return(Audit);
		case PREF_CONCON_BITS: return(Audit);
		case PREF_MULTI_REPLY: return(Audit);
		case PREF_THREADING_OFF: return(Audit);
		case PREF_NO_USE_UIDPLUS: return(Audit);
		case PREF_DROP_TRASH_CACHE: return(Audit);
		case PREF_RELAY_PERSONALITY: return(Audit);
		case PREF_NO_RELAY_PARTICIPATE: return(Audit);
		case PREF_K5_STRICT_HOSTNAMES: return(Audit);
		case PREF_MAKE_EUDORA_DEFAULT_MAILER: return(Audit);
		case PREF_HAVE_WE_ASKED_ABOUT_DEFAULT_MAILER: return(Audit);
		case PREF_SEARCH_WEB_BITS: return(Audit);
		case PREF_PRINT_BLACK: return(Audit);
		case PREF_SYNC_OSXAB: return(Audit);
		case PREF_NO_TB_SEARCH: return(Audit);
		case PREF_POP_BEFORE_SMTP: return(Audit);
		case PREF_BELIEVE_STATUS: return(Audit);
		case PREF_POP_LAST_AUTH: return(Audit);
		case PREF_TEXT_FONT: return(Audit);
		case PREF_TEXT_SIZE: return(Audit);
		case PREF_IMAP_EXECUTE_QUEUED_COMMANDS_ON_QUIT: return(Audit);
		case PREF_IMAP_VISIBLE_SUM_FILTER: return(Audit);
		case PREF_BITE_ME_EMO: return(Audit);
		case PREF_IMAP_AUTOEXPUNGE: return(Audit);
		case PREF_TOC_REBUILD_ALERTS: return(Audit);
		case PREF_QD_TEXT_ROUTINES: return(Audit);
		case PREF_NO_LIVE_SEARCHES: return(Audit);
		case PREF_NO_STEENKEEN_BATCHES: return(Audit);
		case PREF_SAVE_IMAP_CACHE_SERVER: return(Audit);
		case PREF_SUBMISSION_PORT: return(Audit);
		case PREF_SHOW_CHANGE_PASSWORD: return(Audit);
		case POP_SCAN_INTERVAL: return(Audit);
		case UNREAD_LIMIT: return(Audit);
		case OPEN_TIMEOUT: return(Audit);
		case RECV_TIMEOUT: return(Audit);
		case SHORT_TIMEOUT: return(Audit);
		case AE_TIMEOUT_TICKS: return(Audit);
		case BIG_MESSAGE: return(Audit);
		case MAX_MESSAGE_SIZE: return(Audit);
		case TOOLBAR_EXTRA_PIXELS: return(Audit);
		case TOOLBAR_SEP_PIXELS: return(Audit);
		case VICOM_FACTOR: return(Audit);
		case JUNK_MAILBOX_THRESHHOLD: return(Audit);
		case JUNK_MAILBOX_EMPTY_DAYS: return(Audit);
		case JUNK_MAILBOX_EMPTY_THRESH: return(Audit);
		case JUNK_XFER_SCORE: return(Audit);
		case JUNK_JUNK_SCORE: return(Audit);
		case JUNK_MIN_REASONABLE: return(Audit);
		case FLUSH_TIMEOUT: return(Audit);
		case JUNK_TRIM_SOON: return(Audit);
		case JUNK_TRIM_INTERVAL: return(Audit);
		case GRAPHICS_CACHE_MAX: return(Audit);
		case CONCON_MULTI_MAX: return(Audit);
		case ENCODED_FLOWED_WRAP_SPOT: return(Audit);
		case PREVIEW_SINGLE_DELAY: return(Audit);
		case PREVIEW_MULTI_DELAY: return(Audit);
		case WIN_GEN_WARNING_THRESH: return(Audit);
		case MULT_RESPOND_WARNING_THRESH: return(Audit);
		case IMAP_DEFAULT_JUNK_SCORE: return(Audit);
		case IMAP_FLAGGED_LABEL: return(Audit);
		case SEARCH_DF_AGE_REL: return(Audit);
		case SEARCH_DF_AGE_SPFY: return(Audit);
		case SEARCH_DF_ATT: return(Audit);
		case SEARCH_DF_ATT_REL: return(Audit);
		default: return(false);
	}
}