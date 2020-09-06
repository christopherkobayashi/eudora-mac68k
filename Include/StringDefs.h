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

#ifndef STRINGDEFS_H
#define STRINGDEFS_H

#define ADDR_TOO_LONG	5601	/*(One of) the address(es) is (are) too long.  Remember to use COMMAS (�,�) to separate addresses.  Mail cannot be sent with such addresses.*/	//
#define UNUSED_WAS_ALERT_TIMEOUT	5602	/**/	//Reserved.
#define ALIAS_A_LABEL	5603	/*Nickname:*/	//
#define ALIAS_A_WIDTH	5604	/*130*/	//Width of the Nickname list column in the Address Book.
#define ALIAS_CMD	5605	/*alias*/	//DON'T TOUCH.
#define ALIAS_E_LABEL	5606	/*Address(es):*/	//Label for the addresses section of the Address Book.
#define ALIAS_FILE	5607	/*Eudora Nicknames*/	//Name of nickname file--don't touch
#define ALIAS_ADDR_PC	5608	/*70*/	//Percent of vertical space to allot to addresses in nicknames.
#define ALIAS_TOO_LONG	5609	/*The nickname you've typed is too long.�Nicknames in Eudora are limited to %d characters.  The nickname you typed is too long.���OK�*/	//
#define ALIA_LOOP	5610	/*Your address book contains a nickname that refers to itself, directly or indirectly.*/	//
#define ALLO_ALIAS	5611	/*There is insufficient memory to read the address book.*/	//
#define ALLO_EXPAND	5612	/*There is insufficient memory to expand the nicknames.*/	//
#define ALLO_MBOX_LIST	5613	/*There is insufficient memory to read mailbox names.*/	//
#define ALREADY_READ	5614	/*R*/	//Status header (substring) for already-read message.
#define APPL_FONT	5615	/*Geneva*/	//Name of application font.
#define ATTACH_FMT	5616	/*:%p:%d:%p:*/	//
#define ATTENTION	5617	/*Eudora needs your attention.*/	//
#define BAD_ADDRESS	5618	/*An address is too long or otherwise malformed.  Mail cannot be sent with such addresses.*/	//
#define BAD_COMP_RENAME	5619	/*Couldn�t rename the mailbox; the mail has been saved under a new name.*/	//
#define BIND_ERR	5620	/*Error involving Domain Name System.*/	//
#define BINHEX	5801	/*(This file must be converted with BinHex*/	//BinHex intro line, used to recognize BinHex files.  DON'T TOUCH.
#define BINHEXEXCESS	5802	/*There were some extra data in the attachment.*/	//
#define BINHEX_BADCHAR	5803	/*The attachment has been corrupted; an illegal character was found.*/	//
#define BINHEX_CREATE	5804	/*Couldn�t create attachment to decode into.*/	//
#define BINHEX_RECV_FMT	5805	/*Receiving file �%p�...*/	//
#define BINHEX_MEM	5806	/*There is insufficient memory to decode the attachment.*/	//
#define BINHEX_OPEN	5807	/*There was an error opening the attachment.*/	//
#define BINHEX_OUT	5808	/*(This file must be converted with BinHex 4.0)*/	//BinHex intro line, put on outgoing messages.  DON'T TOUCH.
#define BINHEX_PROG_FMT	5809	/*Sending file �%p�...*/	//
#define BINHEX_PROMPT	5810	/*Save attachment as:*/	//
#define BINHEX_READ	5811	/*There was an error sending the attachment.*/	//
#define BINHEX_SHORT	5812	/*The attachment is corrupt; it was too short.*/	//
#define BINHEX_WRITE	5813	/*There was an error writing the attachment.*/	//
#define BOX_TOO_LONG1	5814	/*The mailbox name:*/	//
#define BOX_TOO_LONG2	5815	/*is too long; mailbox names must be 27 characters or less.*/	//
#define BUFFER_SIZE	5816	/*4096*/	//Disk I/O buffer size.
#define CANT_QUEUE	5817	/*Can't queue this message; all messages must have addresses in the to: or bcc: fields, and a valid From address.*/	//
#define CHECK_MAIL	5818	/*Check Mail*/	//
#define CLOSE_MBOX	5819	/*There was an error closing the mailbox--data may have been lost.*/	//
#define CNXN_OPENING	5820	/*Contacting %p (%i)... */	//
#define COMP_TOP_MARGIN	6001	/*34*/	//Height of icon bar.
#define COPY_FAILED	6002	/*Copy failed.*/	//
#define COULDNT_GET_TOOL	6003	/*Couldn�t get connection tool.*/	//
#define COULDNT_MBOX	6004	/*Couldn�t open Mailbox window.*/	//
#define COULDNT_MOD_ALIAS	6005	/*Couldn�t apply your changes to the previous address book entry.*/	//
#define COULDNT_PREF	6006	/*Couldn�t display the settings window.*/	//
#define COULDNT_SAVEAS	6007	/*Couldn�t save the message.*/	//
#define COULDNT_SETUP	6008	/*Couldn�t create Page Setup information.*/	//
#define COULDNT_SQUEEZE	6009	/*Couldn�t compact the mailbox.*/	//
#define COULDNT_WIN	6010	/*Couldn�t create another window.*/	//
#define CRC_ERROR	6011	/*A checksum error was found in decoding this attachment; use it with caution.*/	//
#define CREATE_FOLDER	6012	/*Couldn�t create mail folder.*/	//
#define CREATE_SETTINGS	6013	/*There was an error creating the settings file.*/	//
#define CREATING_ALIAS	6014	/*Couldn�t create your Address Book file.*/	//
#define CREATING_MAILBOX	6015	/*Couldn't create the mailbox.*/	//
#define CTB_CLOSING	6016	/*Closing the connection.*/	//
#define CTB_NEWLINE	6017	/*\015\012*/	//Newline used during CTB connections.
#define CTB_PROBLEM	6018	/*Communications Toolbox error.*/	//
#define CUT_FAILED	6019	/*Cut failed.*/	//
#define DATE_HEADER	6020	/*%r %p*/	//DON'T TOUCH.
#define DATE_STRING_FMT	6201	/*%r %r %d%d %d%d:%d%d:%d%d %d\n*/	//DON'T TOUCH.
#define DELETING_BOX	6202	/*Couldn't remove mailbox.*/	//
#define DELIMITERS	6203	/*, \t*/	//DON'T TOUCH.
#define DNR_LOOKUP	6204	/*Finding */	//
#define DOUBLE_TOLERANCE	6205	/*3*/	//Double-click mouse movement tolerance.
#define EXPL_INTERVAL	6206	/*The �\0xE9� in �Check for mail every \0xE9 minute(s)� must be a number.*/	//
#define EXPL_POP	6207	/*The �POP Account� should be an user name, followed by an �@�, followed by a host name.\nFOR EXAMPLE, �joe@ux8.cso.uiuc.edu�.*/	//
#define EXPL_SMTP	6208	/*The �SMTP Server� must be a host name.\nFOR EXAMPLE, �ux8.cso.uiuc.edu�.*/	//
#define FATAL	6209	/*Eudora cannot continue.*/	//
#define FILE_FOLDER_FMT	6210	/*Attachment converted: %p:%p (%p/%p) (%p)\n*/	//
#define FILE_LINE_FMT	6211	/*  {%d:%d}*/	//Don't touch.
#define FIRST_UNREAD	6212	/*Peeking at message %d...*/	//
#define FLUSH_SECS	6213	/*10*/	//Number of seconds between FlushVol calls.
#define FOLDER_NAME	6214	/*Eudora Folder*/	//Name of Eudora folder--don't touch
#define FONTSIZE_EXPL	6215	/*�%p� is too large or too small.*/	//
#define GENERAL	6216	/*An error occurred.*/	//
#define GET_MENU	6217	/*Couldn�t load menus.*/	//
#define HOUSEKEEPING	6218	/*Getting ready to make a connection.*/	//
#define ICMP_SECONDS	6219	/*3*/	//Number of seconds to leave ICMP messages on-screen.
#define IN	6220	/*In*/	//Name of In mailbox file--Don't touch.
#define INFINITE_STRING	6401	/*~~~~~~~*/	//DON'T TOUCH.
#define INIT_CTB	6402	/*Starting the Comm. Toolbox.*/	//
#define LOGGING_IN	6403	/*Logging into the POP server.*/	//
#define LOGIN_FAILED	6404	/*Invalid POP account or password.*/	//
#define LOOK_MAIL	6405	/*Looking for mail.*/	//
#define MAIL_FOLDER	6406	/*Unable to find or create your Eudora Folder.*/	//
#define MAKE_CONNECT	6407	/*Trying to make a connection.*/	//
#define FRAGMENT_SIZE	6408	/*200000*/	//Size of fragments of split messages.  Must be less than <x-eudora-setting:7619>.
#define MAYNT_DELETE_BOX	6409	/*That mailbox may not be removed.*/	//
#define MAYNT_RENAME_BOX	6410	/*That mailbox may not be renamed.*/	//
#define UNTITLED_FOLDER	6411	/*untitled folder*/	//
#define UNTITLED_MAILBOX	6412	/*untitled mailbox*/	//
#define NEW_BUTTON	6413	/*New*/	//
#define REMOVE_BUTTON	6414	/*Remove*/	//
#define LDAP_SEARCH_TOKEN_DELIMS	6415	/* ,*/	//Use these delimiters to break up LDAP queries into words.
#define NOTE_CMD	6416	/*note*/	//Command used for notes in nicknames window.
#define MEM_ERR	6417	/*�The operation failed; there was not enough memory.*/	//
#define MEM_LOW	6418	/*�Memory is tight.*/	//
#define MEM_PARTITION	6419	/*Memory sizes: Current %K, Minimum %K.*/	//
#define MESS_TE	6420	/*Couldn�t create a TextEdit record for this mailbox.*/	//
#define MOVE_MAILBOX	6601	/*Couldn�t move the mailbox.*/	//
#define NAVIN	6602	/*Navigate In*/	//Navigation In string name.
#define NAVOUT	6603	/*Navigate Out*/	//Navigation Out string name.
#define NEWLINE	6604	/*\015\012*/	//TCP newline string.
#define NEW_ITEM_TEXT	6605	/*New...<I*/	//
#define NO_COLONS_HERE	6606	/*Colons (�:�) are not allowed in mailbox names.*/	//
#define NO_CONTROL	6607	/*Couldn�t add scroll bars to the window.*/	//
#define NO_CTB	6608	/*The Communications Toolbox is not installed.*/	//
#define NO_CTBCM	6609	/*Couldn�t start the Communications Toolbox Connection Manager.*/	//
#define NO_CTBRM	6610	/*Couldn�t start the Communications Toolbox Resource Manager.*/	//
#define NO_CTBU	6611	/*Couldn�t start the Communications Toolbox Utilities.*/	//
#define NO_CTB_TOOLS	6612	/*No connection tools are available.*/	//
#define NO_FINFO	6613	/*Couldn�t get information on that application.*/	//
#define NO_MESS_BUF	6614	/*There is not enough memory to read that message.*/	//
#define NO_PPORT	6615	/*Couldn�t print the document; there was a problem allocating a GrafPort.*/	//
#define NO_PRINTER	6616	/*Couldn�t find your printer.*/	//
#define NO_SMTP_SERVER	6617	/*Server not responding.*/	//
#define NO_SUBJECT	6618	/*�No Subject�*/	//
#define NO_TO	6619	/*�No Recipient�*/	//
#define OLD_SYSTEM	6620	/*Eudora requires use of Macintosh System 7.0 or later.*/	//
#define OPEN_ALIAS	6801	/*Error opening address book file.*/	//
#define OPEN_MBOX	6802	/*Couldn�t open mailbox.*/	//
#define OPEN_SETTINGS	6803	/*Error opening your settings file.*/	//
#define OPEN_TIMEOUT	6804	/*60*/	//Timeout for opening connections.
#define OUT	6805	/*Out*/	//Name of Out mailbox file--don't touch
#define PARTIAL_TICKS	6806	/*60*/	//Number of ticks before Live Nicknames moves pointer.
#define PART_FMT	6807	/*(continuation #%d)\n*/	//
#define PART_PRINT_FAIL	6808	/*There was an error during printing.  At least one of your documents did not print.*/	//
#define PASTE_FAILED	6809	/*Paste failed.*/	//
#define PERFORMANCE	6810	/*Performance monitoring code failed to initialize.*/	//
#define PH_EXPL	6811	/*The �PH Server� must be a host name.\nFOR EXAMPLE, �ns.eudora.com�.*/	//
#define PH_HOST	6812	/*ns.eudora.com*/	//Default ph host.
#define PH_LABEL	6813	/*Enter query (%p server is %p):*/	//
#define PH_PORT	6814	/*105*/	//Port used for ph.
#define PH_QUERY	6815	/*query */	//Query command used in ph.  Should end with space. Do not localize.
#define PH_QUIT	6816	/*quit*/	//Quit command used in ph. Do not localize.
#define PH_SEPARATOR	6817	/*------------------------------------------------------------\n*/	//
#define PICK_CREATOR	6818	/*Please choose the application you use to open text files.*/	//
#define PLAIN_PROG_FMT	6819	/*Sending TEXT document �%p�...*/	//
#define POP_PORT	6820	/*110*/	//Port used for POP3.
#define POP_STATUS_FMT	7001	/*Transferring message %d of %d...*/	//
#define POSITION_NAME	7002	/*Window Position*/	//Names of window size resources.
#define PRINT_FAILED	7003	/*There was an error during printing.  Your document did not print.*/	//
#define PRINT_FONT	7004	/*Courier*/	//Default print font.
#define PRINT_H_FONT	7005	/*Times*/	//Print header font.
#define PRINT_H_MAR	7006	/*18*/	//Print header margin.
#define PRINT_H_SIZE	7007	/*12*/	//Print header fontsize.
#define QUEUE_BUTTON	7008	/*Queue*/	//Name of Queue button.
#define QUOTE_PREFIX	7009	/*>*/	//Prefix for quoted sections.
#define RCV_BUFFER_SIZE	7010	/*1024*/	//
#define READ_ALIAS	7011	/*Couldn't read address book.*/	//
#define READ_MBOX	7012	/*Couldn�t read mailbox.*/	//
#define READ_ONLY	7013	/*That text cannot be modified.*/	//
#define READ_SETTINGS	7014	/*Couldn't read your settings.*/	//
#define READ_TOC	7015	/*Couldn�t read the table of contents.*/	//
#define RECEIVED_HEAD	7016	/*Received:*/	//Don't touch.
#define RECV_TIMEOUT	7017	/*45*/	//Receive timeout, in seconds.
#define REDIST_ANNOTATE	7018	/* (by way of %p)*/	//
#define RENAMING_BOX	7019	/*Couldn't rename mailbox.*/	//
#define REPLY_INTRO	7020	/*Re: */	//String with which to prefix replies.
#define RETURN_PRINT_INTRO	7201	/*Printed for */	//
#define SAVEAS_PROMPT	7202	/*Save as:*/	//
#define SAVE_ALIAS	7203	/*Couldn�t save your address book.*/	//
#define SAVE_SETUP	7204	/*Couldn�t save Page Setup*/	//
#define SAVE_SUM	7205	/*Couldn't build table of contents.*/	//
#define SECRET	7206	/*�secret things�*/	//
#define SEND_BUTTON	7207	/*Send*/	//Name of Send button.
#define SETTINGS_FILE	7208	/*Eudora Settings*/	//Name of settings/signature file.
#define SHORT_TIMEOUT	7209	/*2*/	//Number of seconds for timeouts during navigation.
#define SIGNATURE	7210	/*Signature*/	//Name of default signature file--don't touch
#define SMTP_PORT	7211	/*25*/	//Port for SMTP.
#define MSG_GOT	7212	/*Successfully received %p (%d)*/	//
#define STATE_LABELS	7213	/*��RDʥQFS-T�X?*/	//Message state labels.
#define STATUS	7214	/*Status:*/	//Header that gives message status.
#define STEVE_FOLDER	7215	/*PMF*/	//
#define FWD_INTRO	7216	/**/	//String to insert before quoted Forward text
#define S_DORNER	7217	/*eudora-info@qualcomm.com*/	//
#define TAB_DISTANCE	7218	/*8*/	//Number of chars to indent for a tab.
#define TCP_BUFFER_SIZE	7219	/*8192*/	//Buffer size for network.
#define TCP_DRIVER	7220	/*.ipp*/	//Name of tcp/ip driver.
#define TCP_TROUBLE	7401	/*TCP/IP Error.*/	//
#define TEMP_SUFFIX	7402	/*.tmp*/	//Temp file suffix.
#define TEXT_CREATOR	7403	/*ttxt*/	//Default TEXT file creator.
#define TEXT_READ	7404	/*Couldn't read the document.*/	//
#define TEXT_TOO_BIG	7405	/*That document is too large to load.*/	//
#define TEXT_WRITE	7406	/*Couldn't save the document.*/	//
#define TE_TOO_MUCH	7407	/*The text of a message or document cannot be more than 32766 bytes.*/	//
#define TOC_SUFFIX	7408	/*.toc*/	//Suffix for table of contents files.
#define TOO_MANY_LEVELS	7409	/*You have more than 100 subfolders in your Eudora folder.  Use the Finder to remove some folders.*/	//
#define TO_TOO_LONG	7410	/*Recipient names must be less than 62 characters.*/	//
#define TRANSFER_PREFIX	7411	/*\0xd0> */	//Prefix for use in Transfer menu.
#define TRASH	7412	/*Trash*/	//Name of Trash mailbox file--don't touch.
#define TS_CONNECT_FMT	7413	/*telnet %p %d /stream\n*/	//CTB connect request string.
#define UNDO	7414	/*Undo*/	//
#define UNKNOWN_SENDER	7415	/*???@???*/	//
#define UNTITLED	7416	/*Untitled*/	//
#define WHO_AM_I	7417	/*Opening TCP/IP...*/	//
#define WONT_UNDO	7418	/*This operation can�t be undone.*/	//
#define WRAP_SPOT	7419	/*76*/	//Number of chars at which to wrap messages when not doing format=flowed.
#define WRITE_MBOX	7420	/*There was an error saving the mail.*/	//
#define WRITE_SETTINGS	7601	/*Couldn't save your settings.*/	//
#define WRITE_TOC	7602	/*Couldn�t write the table of contents.*/	//
#define WU_FMT	7603	/*\n%d, {%d:%d}*/	//
#define VOLUME_MARGIN	7604	/*2048*/	//Disk space safety margin.
#define CONCON_FORWARD_ON	7605	/*Original Message,begin forwarded text*/	//
#define PRINT_LEFT_MAR	7606	/*54*/	//Left printout margin.
#define PRINT_RIGHT_MAR	7607	/*36*/	//Right printout margin.
#define LEADING_PERIOD	7608	/*It�s a very bad idea to use names beginning with a period.*/	//
#define CTB_ME	7609	/*DialupEudora*/	//SMTP Helo argument for dialups.
#define TCP_ME	7610	/*[%i]*/	//SMTP Helo argument for networks.
#define LDAP_SEARCH_FILTER	7611	/*(|(cn=*^0*))*/	//Default LDAP search filter.  Used on each word, and resulting terms anded together.
#define DEF_MWIDTH	7612	/*80*/	//Message width when PREF_MWIDTH is not set.
#define UU_BAD_LENGTH	7613	/*Attachment corrupt; wrong number of characters on a line.*/	//Message when an incorrect line length is found in uuencoded data.
#define HEX_SIZE_PERCENT	7614	/*65*/	//Eudora will decode a BinHex file if the message is at least this percent of the size the BinHex file reports, or...
#define HEX_SIZE_THRESH	7615	/*20000*/	//if the message is less than this size.
#define BIG_MESSAGE	7616	/*40*/	//Size limit for Skip Big Messages. Setting to 0 (zero) will cause messages NOT to be skipped.
#define BIG_MESSAGE_MSG	7617	/*\n\nWARNING: The remainder of this %K message has not been transferred.  Turn off the �Skip big messages� option and check mail again to get the whole thing.*/	//
#define BREAKING	7618	/*BREAK [%d ticks]*/	//
#define SPLIT_THRESH	7619	/*250000*/	//Size at which a message must be split.  Must be greater than <x-eudora-setting:6408>.
#define BOX_SIZE_FMT	7620	/*%d/%K/%K*/	//Size display format for mailbox windows.
#define BOX_SIZE_SIZE	7801	/*128*/	//size of mailbox size display
#define SUB_EDIT_HELP	7802	/*You may change the subject that appears in the mailbox window and window title by editing this text.*/	//
#define MESS_HELP	7803	/*This is the text of the message you were sent; you may only change it if you select the pencil icon above.*/	//
#define OLD_BOX_SIZE_FONT_SIZE	7804	/*9*/	//Size of font for mailbox size box.
#define ALIAS_VERBOTEN	7805	/*:;,@<>()[]\"*/	//Characters forbidden in aliases.
#define WARN_VERBOTEN	7806	/*The characters �:;@<>()[]\"\',� are not allowed in nicknames (they can go in the Address(es) section, just not the nickname itself.*/	//
#define NEW_MAIL	7807	/*You have new mail.*/	//
#define LDAP_TERM_COMBINER	7808	/*(%c%p%p)*/	//
#define UU_BAD_VERSION	7809	/*The attachment is an unknown AppleSingle version:*/	//
#define UU_INVALID_MAP	7810	/*The attachment has an invalid map count:*/	//
#define UU_SKIP_MAP_INFO	7811	/*The attachment contained extra information, map: */	//
#define UU_INVALID_STATE	7812	/*The decoder has entered an invalid state, the attachment may be damaged.*/	//
#define BCC_ONLY	7813	/*Recipient List Suppressed:;*/	//Put in the To: field if a message has only Bcc:'s.
#define UUPC_COPY	7814	/*Picking up message:*/	//
#define UUPC_WRONG_SMTP	7815	/*UUCP misconfigured; must be !mymac!spoolvol:spooldir:!username!0000 */	//
#define UUPC_DMYMAC	7816	/*D.%p0%p*/	//
#define FWD_TRAIL	7817	/**/	//String to put after forwarded text
#define REP_SEND_ATTR	7818	/**/	//Reply to sender attribution
#define UUPC_XMYMAC	7819	/*X.%p0%p*/	//
#define WAIT_FOR_START_AE	7820	/*10*/	//# seconds to wait for ODoc, OApp, or PDoc
#define UUPC_U_CMD	8001	/*U %p %p\n*/	//
#define UUPC_F_CMD	8002	/*F %p\n*/	//
#define UUPC_I_CMD	8003	/*I %p\n*/	//
#define UUPC_C_CMD	8004	/*C rmail*/	//
#define UUPC_REMOTE	8005	/*remote from %p\n*/	//
#define UUPC_SECURE	8006	/*Can only do UUPC send if you do UUPC receive.*/	//
#define SALV_REPORT	8007	/*%d of the %d summar%* in the old table of contents used; %d new summar%* created.*/	//
#define DESK_LEFT_STRIP	8008	/*0*/	//Number of pixels to leave uncovered on the left of the screen when creating a new window.
#define DESK_RIGHT_STRIP	8009	/*72*/	//Number of pixels to leave uncovered on the right of the screen when creating a new window.
#define DESK_BOTTOM_STRIP	8010	/*0*/	//Number of pixels to leave uncovered on the bottom of the screen when creating a new window.
#define DESK_TOP_STRIP	8011	/*0*/	//Number of pixels to leave uncovered beneath the menu bar when creating a new window.
#define XSENDER_FMT	8012	/**/	//X-Sender: header; make it X-Sender: %p%p%p if you want the old behavior...
#define PH_TOO_BIG	8013	/*\nThe response was too long; the remainder has been lost.*/	//
#define ME	8014	/*me*/	//Nicknames for sender.
#define REPLY	8015	/*Reply*/	//text for normal reply
#define REPLY_ALL	8016	/*Reply to All*/	//text if PREF_REPLY_ALL is set
#define CONCON_FORWARD_OFF	8017	/*End Original Message,end forwarded text*/	//
#define LOG_SENT	8018	/*Sent: �%p�*/	//
#define LOG_GOT	8019	/*Rcvd: �%p�*/	//
#define DIALUP	8020	/*dialup*/	//
#define DATE_ERROR	8201	/*Invalid date format.*/	//
#define THE_PAST	8202	/*Dates in the past are not allowed.*/	//
#define NEVER_WARN	8203	/*12*/	//Never warn the user of mail to be sent this many hours into the future
#define BIG_MESSAGE_FRAGMENT	8204	/*20*/	//# of body lines of a skipped big message to download
#define ATTRIBUTION	8205	/*At ^3 ^1, ^0 wrote:*/	//# attribution line; from, date, subject, time
#define ALIAS_N_LABEL	8206	/*Notes:*/	//Label for private notes section of nicknames
#define LOCAL_PORT	8207	/*0*/	//Port from which Eudora connects; leave at 0
#define NICK_IN_USE	8208	/*That nickname already exists.��%p� already appears in the �%p� address book.  Nicknames within the same address book must be unique.���OK�*/	//
#define LOG_NAME	8209	/*Eudora Log*/	//
#define OLD_LOG	8210	/*Old Log*/	//
#define LOG_SUCCEEDED	8211	/*Succeeded.*/	//
#define LOG_FAILED	8212	/*Failed (%d).*/	//
#define SENDING	8213	/*Sending %p.*/	//
#define ALERT_DISMISSED_ITEM	8214	/*Dismissed with %d.*/	//
#define NO_ADDRESSES	8215	/*Couldn't make address book entry.�No address found.*/	//
#define PW_PORT	8216	/*106*/	//Port for password-change protocol
#define NEW	8217	/*enter the new*/	//
#define PW_ERROR	8218	/*Couldn�t change your password.*/	//
#define PW_MISMATCH	8219	/*Those two passwords didn't match.*/	//
#define VERIFY_NEW	8220	/*verify the new*/	//
#define ENTER	8401	/*enter the*/	//
#define WRAP_THRESH	8402	/*80*/	//Lines shorter than this will not be wrapped when not doing format=flowed.
#define FINGER_PORT	8403	/*79*/	//Port for finger requests
#define PRIORITY_FMT	8404	/*%r %d %r*/	//Format for the priority header.
#define PRIOR_MENU_HELP	8405	/*Use this menu to set the message's priority (priorities are informational only).*/	//
#define COMPACT_WASTE_PER	8406	/*25*/	//Percentage of waste space that triggers a compaction
#define COMPACT_FREE_PER	8407	/*1*/	//Percentage of free disk space that triggers a compaction
#define FINGER	8408	/*finger*/	//
#define DEFAULT	8409	/*Default*/	//
#define FWD_QUOTE	8410	/*>*/	//Quote character for forwards
#define ADD_REALNAME	8411	/*^1 <^0>*/	//Make the real name from pop account & real name.  ^0 is account (or email address), ^1 is real name.
#define START_POP_LOG	8412	/*%p %d (%d)*/	//
#define START_SEND_LOG	8413	/*%p %d*/	//
#define SQUISH_LEFTOVERS	8414	/*It looks like a previous compaction was attempted and failed.  Get help.*/	//
#define WDS_LIMIT	8415	/*16*/	//
#define PH_FAIL	8416	/*Couldn't open a connection to ^0, trying ^1...\n*/	//
#define PH_SUCCEED	8417	/*%p seems willing to talk to us.\n*/	//
#define NOONE	8418	/*No one*/	//
#define UNSENT_WARNING	8419	/*You haven't sent all those messages; %r them anyway?\n(They won't ever be sent.)*/	//
#define TRASH_VERB	8420	/*trash*/	//
#define UNREAD_WARNING	8601	/*You haven't read all those messages; %r them anyway?*/	//
#define XFER_VERB	8602	/*transfer*/	//
#define TEMP_WARNING	8603	/*Temporary file found. It has been put in the mailbox menus, but there may be a problem.*/	//
#define DATE_SUM_FMT	8604	/*^0 ^1^2*/	//Format for date summaries; time, date, timezone
#define WAITING	8605	/*Waiting...*/	//
#define LOG_EXPECT	8606	/*Expecting: �%p�*/	//
#define LOG_FOUND	8607	/*Found expected string.*/	//
#define LOG_NOTFOUND	8608	/*Didn�t find expected string.*/	//
#define NO_AUXUSR	8609	/*Your navigation script requires a dialup username.  Use the �Personal Information� section of the �Settings...� dialog to set one.*/	//
#define MESS_TITLE_PLUG	8610	/*^1, ^2, ^3*/	//Message titling string; 0 mailbox, 1 sender, 2 date, 3 subject.
#define BAD_HEXBIN_FORMAT	8611	/*The attachment is not in proper BinHex format.*/	//
#define INSTALL_AE	8612	/*Couldn�t initialize AppleEvents.*/	//
#define PH_RETURN	8613	/**/	//Return clause for ph.  Should be empty or begin with space.
#define SCROLL_THROTTLE	8614	/*7*/	//Number of ticks between page scrolls.
#define USE_MAP	8615	/*Please set the location and timezone of your Macintosh with the �Map� and/or �Date & Time� control panels.  Otherwise Eudora can�t handle Date: headers properly.*/	//
#define MIME_ATTR_MAX	8616	/*32*/	//
#define MIME_VAL_MAX	8617	/*128*/	//
#define MIME_VERSION	8618	/*1.0*/	//
#define MIME_MULTIPART	8619	/*multipart*/	//Don't touch.
#define MIME_TEXT	8620	/*text*/	//Don't touch.
#define MIME_MESSAGE	8801	/*message*/	//Don't touch.
#define MIME_BASE64	8802	/*base64*/	//Don't touch.
#define MIME_APPLICATION	8803	/*application*/	//Don't touch
#define MIME_APPLEFILE	8804	/*applefile*/	//Don't touch
#define MIME_APPLEDOUBLE	8805	/*appledouble*/	//Don't touch
#define MIME_DIGEST	8806	/*digest*/	//Don't touch
#define MIME_BINHEX	8807	/*mac-binhex-40*/	//Don't touch
#define MIME_ISO_LATIN1	8808	/*iso-8859-1*/	//Don't touch
#define MIME_PARTIAL	8809	/*partial*/	//Don't touch
#define BAD_HEX_MSG	8810	/*One or more attachments were corrupt.*/	//
#define BAD_ENC_MSG	8811	/*%d encoding error%# were found.*/	//
#define NAME	8812	/*name*/	//Don't touch
#define SINGLE_TEMP	8813	/*���Temp���*/	//
#define MIME_QP	8814	/*quoted-printable*/	//
#define MIME_V_FMT	8815	/*%r: %r%p*/	//
#define MIME_MP_FMT	8816	/*%r: %r/%r; %r=\"%p\"%p*/	//
#define PS_SUFFIX	8817	/*.ps*/	//
#define MIME_BOUND1_FMT	8818	/*============_%d==_%p%r*/	//
#define MIME_MIXED	8819	/*mixed*/	//
#define MIME_MAC	8820	/*macintosh*/	//
#define MIME_CSET	9001	/*; charset=\"%p\"*/	//
#define MIME_TEXTPLAIN	9002	/*%r: %r/%r%p%p*/	//
#define MAX_SMTP_LINE	9003	/*990*/	//Maximum line to send to SMTP
#define MIME_CT_PFMT	9004	/*%r: %p/%p; %r=\"%p\"%p*/	//
#define MIME_P_FMT	9005	/*%r: %p%p*/	//
#define MIME_OCTET_STREAM	9006	/*octet-stream*/	//
#define MIME_BINARY	9007	/*binary*/	//Don't touch.
#define ATTACH_REMOVED	9008	/*Some attachments could not be found and were not included in the new message.*/	//
#define MIMERICH_COMMENT	9009	/*param*/	//
#define MIMERICH_NOFILL	9010	/*nofill*/	//
#define GROUP_DONT_HIDE	9011	/*-WorkGroup*/	//This suffix on a group name causes Eudora not to hide the group members.
#define MIME_RICHTEXT	9012	/*enriched*/	//
#define BAD_XMIT_ERR_TEXT	9013	/*Couldn�t send message; %pserver says �%s�.*/	//
#define MIME_PLAIN	9014	/*plain*/	//
#define LDAP_CN_SEARCH_FILTER	9015	/**/	//LDAP search filter when you wish to search a full name.
#define MIME_RICH_ON	9016	/*<%r>*/	//
#define MIME_RICH_OFF	9017	/*</%r>*/	//
#define MIME_USASCII	9018	/*us-ascii*/	//
#define FILTERS_NAME	9019	/*Eudora Filters*/	//
#define OTHER_FN_BAD	9020	/*../\"\\*/	//
#define OTHER_FN_REP	9201	/*==='=*/	//
#define MAC_FN_BAD	9202	/*..:/*/	//
#define MAC_FN_REP	9203	/*==-/*/	//
#define OFFLINE	9204	/*No connections can be made, because you have set the connection method to �Offline.�*/	//
#define SIG_FOLDER	9205	/*Signature Folder*/	//Name of signature folder--don't touch
#define CANT_VOL_MOVE	9206	/*Eudora cannot move mailboxes from one volume to another.  Please use the Finder.*/	//
#define NICK_FOLDER	9207	/*Nicknames Folder*/	//Name of nicknames folder--don't touch
#define POSTSCRIPT	9208	/*PostScript*/	//Don't touch.
#define PS_MAGIC	9209	/*%!*/	//Magic bytes for PostScript file
#define OLD_QUEUE_M_ITEM	9210	/*Queue Message*/	//
#define OLD_SEND_M_ITEM	9211	/*Send Message Now*/	//
#define NOT_ENOUGH_ROOM	9212	/*There is insufficient disk space.*/	//
#define RFC1342_DELIMS	9213	/*, \t\r\n()[]:<*/	//
#define RFC1342_FMT	9214	/*=?%p?Q?%p?=*/	//Don't touch.
#define AE_TROUBLE	9215	/*Having trouble building AppleEvents.*/	//
#define COULDNT_LAUNCH	9216	/*Couldn't start that application.*/	//
#define MIME_BINHEX2	9217	/*mac-binhex40*/	//Don't touch
#define DOUBLE_RFORK_FMT	9218	/*%%%p*/	//Filename template for resource part of appledouble file
#define MIME_RFC822	9219	/*rfc822*/	//Don't touch.
#define CTB_SETCONFIG_FMT	9220	/*%p CMSetConfig*/	//Resource name for extra config params.
#define SETCONFIG	9401	/*CMSetConfig*/	//
#define CONTENT_DISPOSITION	9402	/*Content-Disposition*/	//Don't touch.
#define ATTACHMENT	9403	/*attachment*/	//Don't touch.
#define MIME_CD_FMT	9404	/*%r: %r; %r=\"%p\"%p*/	//Don't touch.
#define MIME_HEADERSET	9405	/*header-set*/	//Don't touch.
#define MIME_DOUBLE_SENDSUB	9406	/*appledouble*/	//Set to either appledouble (oldstyle) or header-set (newstyle)
#define BETA_EXPIRED	9407	/*This beta version of Eudora has expired.�Please visit our website to get a newer beta or the final version.��Quit-�Visit & Quit�*/	//
#define NOT_MAILBOX	9408	/*Selection is not a properly formatted mailbox.*/	//
#define MIME_CT_ANNOTATE	9409	/* ; %r=\"%p\"%p*/	//
#define SAVE_SUM_ERR	9410	/*Memory error; couldn�t add the message summary to the table of contents.*/	//
#define TEXT_QP_TASTE	9411	/*16000*/	//Size of chunk of text attachment to check for non-us-ascii characters.
#define SCROLL_ARROW_THROTTLE	9412	/*2*/	//# of ticks to wait between arrow scrolls
#define IDLE_SECS	9413	/*60*/	//Time in seconds Eudora must be idle before doing a timed check
#define MAX_QUOTE	9414	/*10*/	//Max # of quote chars Eudora will recognize as such
#define AE_TIMEOUT_TICKS	9415	/*18000*/	//Apple Event timeout, in ticks.
#define X_UUENCODE	9416	/*x-uuencode*/	//Label to use for uuencoded body parts.
#define NQ_ANNOTATE	9417	/* ; %r=%p%p*/	//Don't touch.
#define MIME_8BIT	9418	/*8bit*/	//Don't touch.
#define MSGID_FMT	9419	/*<v%p@%p>*/	//Don't touch.
#define SAVE_POPD	9420	/*Couldn't save information for Leave Mail On Server.  Some messages may be fetched again.*/	//
#define POP_SCAN_INTERVAL	9601	/*1*/	//# of days Eudora should wait before scanning for messages to delete on POP server
#define UNREAD_STYLE	9602	/*4*/	//Style to use for mailboxes with unread mail. Bold=1, italics=2, underline=4. Add numbers for combinations of styles.
#define UNREAD_LIMIT	9603	/*5*/	//# of days after which to consider mailboxes not to have unread mail.
#define ANSWER_RESPONSE	9604	/*response*/	//
#define SETTINGS_CELL_SIZE	9605	/*54*/	//Select the icon size displayed in the settings list. Can also display without name or icon.
#define LOG_HANGUP	9606	/*Will cancel on: �%p�*/	//
#define LOG_HANGUP_FOUND	9607	/*Found cancel string.*/	//
#define UNVERIFIED	9608	/* (Unverified)*/	//
#define PREFLIMIT_EXPLAIN	9609	/*\n\nMust be between %d and %d.*/	//
#define PREFERROR_INTRO	9610	/*Invalid setting:\n*/	//
#define KERBEROS_DRIVER	9611	/*.Kerberos*/	//Name of the kerberos driver.
#define NO_KERBEROS	9612	/*Cannot communicate with Kerberos.*/	//
#define KERBEROS_POP_SERVICE	9613	/*kpop*/	//Name of kerberized POP3 service.
#define KERBEROS_SERVICE_FMT	9614	/*^0.^1@^2*/	//Kerberos service name format.  ^0=service, ^1=host.domain, ^2=realm, ^3=host
#define KERBEROS_VERSION	9615	/*justjunk*/	//8 characters of junk for sendauth server
#define KERBEROS_BSIZE	9616	/*1400*/	//size of kerberos ticket buffer
#define KERBEROS_CHECKSUM	9617	/*0*/	//kerberos checksum
#define KERBEROS_TICK_FMT	9618	/*Getting ticket for �%p�...*/	//
#define KERBEROS_ESCAPES	9619	/*.*/	//Characters that should be backslashed in kerberos names
#define KERBEROS_FAKE_PASS	9620	/*meaningless*/	//Meaningless argument to Kerberos password
#define OLD_SETTINGS_FONT_SIZE	9801	/*10*/	//Size of font to use in settings window
#define CHECKING_MAIL	9802	/*Checking Mail...*/	//
#define SENDING_MAIL	9803	/*Sending Mail...*/	//
#define MOVING_MESSAGES	9804	/*Moving Messages...*/	//
#define FINDING	9805	/*Finding...*/	//
#define LEFT_TO_TRANSFER	9806	/*Messages remaining to transfer:*/	//
#define LEFT_TO_PROCESS	9807	/*Messages remaining to process:*/	//
#define CHANGING_PW	9808	/*Changing Password...*/	//
#define ATTACH_FOLDER	9809	/*Attachments Folder*/	//
#define PREPARING_CONNECTION	9810	/*Preparing to transfer...*/	//
#define CLEANUP_CONNECTION	9811	/*Cleaning up...*/	//
#define MESS_FETCH_HELP	9812	/*This message will be downloaded in full next time you check mail.  Click here to prevent the download.*/	//
#define MESS_FETCH_HELP2	9813	/*The bulk of this message was skipped.  Click here to have it downloaded on the next mail check.*/	//
#define MESS_DELETE_HELP	9814	/*This message will be deleted from the POP server the next time you check mail.  Click here to leave it on the server.*/	//
#define MESS_DELETE_HELP2	9815	/*This message is on your POP server.  Click here to have it deleted the next time you check mail.*/	//
#define MESS_SERVER_HELP	9816	/*This message is still on your POP server.  The icons in this box let you delete it (or fetch the rest, if part was skipped).*/	//
#define MESS_DRAG_HELP	9817	/*Use the tow truck to drag the message to a different mailbox window.*/	//
#define PROGRESS	9818	/*Progress*/	//Name of progress window
#define NO_ATTACH_FOLDER	9819	/*Your attachments folder cannot be found.�Eudora will use �Attachments Folder� in your Eudora folder for now.  Should Eudora use that permanently?�Choose New...�No-�Yes�*/	//
#define U_AN_APPLICATION	9820	/**/	//
#define MIN_COLOR_LIGHT	10001	/*15*/	//Minimum percent lightness for color displays.
#define MAX_COLOR_LIGHT	10002	/*85*/	//Maximum percent lightness for color displays.
#define KERB_POP_PORT	10003	/*110*/	//Port # for kerberized popper.
#define LOG_ROLLOVER	10004	/*100*/	//Size in K at which log file will be ended and a new one started.  One old log is kept, named �Old log.�
#define DEFAULT_CREATOR	10005	/*----*/	//Creator for files of unknown MIME types.
#define DEFAULT_TYPE	10006	/*????*/	//Type for files of unknown MIME types.
#define FIXED_DATE_FMT	10007	/*^0 ^1^3*/	//Format for �fixed� date summaries; time, date, weekday, zone.
#define SHOW_ALL_HELP	10008	/*All headers and other sludge are displayed in this message.*/	//
#define NO_SHOW_ALL_HELP	10009	/*Less important headers and formatting commands are hidden.*/	//
#define PURCHASE_EUDORA	10010	/*Purchasing Information...*/	//
#define PURCH_URL	10011	/*http://www.eudora.com/offer/?Mac.%p*/	//Don't touch.
#define PREFILTER_SUFFIX	10012	/*.pre*/	//DO NOT TOUCH
#define POSTFILTER_SUFFIX	10013	/*.pst*/	//DO NOT TOUCH
#define FORMAT_FLOWED	10014	/*flowed*/	//DO NOT TOUCH
#define WILL_EXPIRE	10015	/*This version of Eudora expires on %p.�Please visit our website to get a newer beta or the final version.��Later-�Visit Site�*/	//
#define NICK_FILE_GONE	10016	/*There were errors reading the address book �%p�.  Nicknames from that file will not be used.  Restart Eudora to try again.*/	//
#define TP_SINGLE_FMT	10017	/*%p: %r*/	//String for task progress title when checking or sending.
#define TP_DUAL_FMT	10018	/*%p: %r & %r*/	//String for task progress title when checking and sending.
#define NEWSGROUPS	10019	/*Newsgroups:*/	//
#define AE_INITIAL_TIMEOUT_SECS	10020	/*5*/	//Number of ticks to wait for initial AE on launch.
#define MAIL_QUEUED	10201	/*Your request has been placed in your Out mailbox, and will be sent next time you send queued messages.*/	//
#define MAIL_SENT	10202	/*Your request has been sent.  You should receive a reply shortly.*/	//
#define SETTINGS_BAD	10203	/*Your settings file appears to be corrupt.  ResEdit may be able to repair it.*/	//
#define MIME_BOUND2	10204	/*============*/	//2nd half of boundary strings
#define NORMAL_MFROM	10205	/*<%p>*/	//Mail From: argument for normal message.
#define EMPTY_MFROM	10206	/*<>*/	//Mail From: argument template for empty mail from.
#define SACRED_PREFS	10207	/*3,4,5,77,218*/	//Preferences to be preserved when cleaning a settings file.
#define RESET_PREFS_WARN	10208	/*Really reset your settings?�Save all your work first, Eudora will quit after the reset and not save anything.��Cancel-�Reset�*/	//
#define CTB_RETRY	10209	/*5*/	//# of times to retry cmnew
#define FIXED_STRN	10210	/*STR# id %d was corrupt and had to be repaired.  Should be ok now.*/	//
#define FIXED_COLON	10211	/*Colons are no longer allowed in nicknames.  Eudora changed your colons to plusses (+).*/	//
#define PLUS	10212	/*+*/	//Character to replace colons in nicknames.
#define NO_TEMP_FILE	10213	/*Couldn't create temporary file.*/	//
#define INLINE	10214	/*inline*/	//Don't touch.
#define CORRUPT_RFORK	10215	/*The resource fork of this file is corrupt.*/	//
#define STATION_FOLDER	10216	/*Stationery Folder*/	//Name of stationery folder--don't touch
#define QUEUED_WARNING	10217	/*Some of those messages are queued to be sent; %r them anyway?\n(They won't ever be sent.)*/	//
#define MAX_MESSAGE_SIZE	10218	/*900*/	//Maximum size of a message Eudora will send without complaint (approximate).
#define PETE_ERR	10219	/*There was an error during editing.*/	//
#define URL_PARSE	10220	/*Couldn�t figure out that URL.*/	//
#define URL	10401	/*URL*/	//Don't touch.
#define MAC_PGP_CREATOR	10402	/*MPGP*/	//
#define PH_ALIAS_FMT	10403	/*^0@^1*/	//Format for building a ph address out of alias, maildomain, mailbox.
#define PH_BOX_FMT	10404	/*^0*/	//Format for building a ph address out of mailfield, maildomain, mailbox.
#define TOC_SMALLDIRTY	10405	/*1800*/	//Number of ticks for Eudora to be idle before having to flush trivial toc changes.
#define PETE_NIBBLE	10406	/*8*/	//Number of K to initially load into a message window
#define OLD_CONFIG_FONT_SIZE	10407	/*10*/	//Font size for configuration windows.
#define STRING_CACHE	10408	/*150*/	//Size of string cache.
#define P_FMT	10409	/*%p*/	//Don't touch.
#define R_FMT	10410	/*%r*/	//Don't touch.
#define NUKE_VERB	10411	/*delete*/	//
#define CHECK_SPECIAL_ITEXT	10412	/*Check Mail Specially...*/	//
#define SEND_SPECIAL_ITEXT	10413	/*Send Messages Specially...*/	//
#define SEND_ITEXT	10414	/*Send Queued Messages*/	//
#define SAVE_ITEXT	10415	/*Save*/	//
#define SAVE_ALL_ITEXT	10416	/*Save All*/	//
#define CLOSE_ITEXT	10417	/*Close*/	//
#define CLOSE_ALL_ITEXT	10418	/*Close All*/	//
#define PRINT_ITEXT	10419	/*Print...*/	//
#define PRINT_SEL_ITEXT	10420	/*Print Selection...*/	//
#define COPY_ITEXT	10601	/*Copy*/	//
#define COPY_UNWRAP_ITEXT	10602	/*Copy & Unwrap*/	//
#define WRAP_ITEXT	10603	/*Wrap Selection*/	//
#define UNWRAP_ITEXT	10604	/*Unwrap Selection*/	//
#define FINISH_ITEXT	10605	/*Finish Address Book Entry*/	//
#define FINISH_EXP_ITEXT	10606	/*Finish & Expand Address Book Entry*/	//
#define INSERT_ITEXT	10607	/*Insert Recipient*/	//
#define INSERT_EXP_ITEXT	10608	/*Insert & Expand Recipient*/	//
#define SORT_ITEXT	10609	/*Sort*/	//
#define SORT_DESCEND_ITEXT	10610	/*Sort Descending*/	//
#define REP_SELECTION	10611	/* Quoting Selection*/	//
#define REP_ALL	10612	/* To All*/	//
#define REP_NOSEL	10613	/* Without Quoting*/	//
#define REP_WITH	10614	/* With*/	//
#define REDIRECT	10615	/*Redirect*/	//
#define TO_ITEXT	10616	/* To*/	//
#define TURBO	10617	/*Turbo */	//
#define TB_REDIR_NO_DEL	10618	/* Without Delete*/	//
#define CHANGE_QUEUEING	10619	/*Change Queueing...*/	//
#define MAKE_NICK_ITEXT	10620	/*Make Address Book Entry...*/	//
#define MAKE_SEL_NICK_ITEXT	10801	/*Make Address Book Entry From Selection...*/	//
#define DELETE_ITEXT	10802	/*Delete*/	//
#define NUKE_ITEXT	10803	/*Nuke*/	//
#define ALT_SIG	5501	/*Alternate*/	//Name of alternate signature file--don't touch
#define HEADER_LABEL	5502	/*Header:*/	//Name of header box in filters window.
#define MATCH_LABEL	5503	/*Match:*/	//Name of Match: section of filters window.
#define ACTION_LABEL	5504	/*Actions:*/	//Name of Action: section of filters window.
#define MAKE_SUBJECT_LABEL	5505	/*Make subject:*/	//Label for Make Subject
#define READ_FILTERS	5506	/*An error occurred whilst reading the filtering rules.*/	//
#define FILTER_OVERTERM	5507	/*This version of Eudora allows only two terms in rules; do not change the rules, or the extra terms will be removed.*/	//
#define FILT_BADKEY_FMT	5508	/*Unknown keyword in filters.�Did not understand keyword �%p.�  It may have been put there by a different version of Eudora.  The keyword will be removed if you save your Filters.���*/	//
#define FILT_XFER_SEL	5509	/*Please select a mailbox from the Transfer menu, or command-period to cancel.*/	//
#define SAVE_FILTERS	5510	/*Error saving filters.*/	//
#define SUBJ_REPLACE	5511	/*&*/	//Replacement metastring for subject area of filters window.
#define SPEC_INTRO	5512	/*\n%p Messages have been filtered into the following mailboxes:\n*/	//
#define SPEC_TITLE	5513	/*Filter Report*/	//
#define SPEC_FMT	5514	/*%p %d\n*/	//
#define FILT_XFER	5515	/*Transfer to:*/	//Label of transfer to button.
#define FILT_COPY	5516	/*Copy to:*/	//Label of copy to button.
#define CHOOSE_APP	5517	/*Please choose an application to open �%p�.*/	//
#define DT_TROUBLE	5518	/*Cannot read the Desktop database.*/	//
#define NOT_SPELLER	5519	/*That application doesn�t know how to provide word services to Eudora.*/	//
#define WORD_SERVICES	5520	/*WORDSERVICES*/	//
#define SPELL_PROMPT	5701	/*Choose a �Word Services� application.*/	//
#define SPELL_LABEL	5702	/*AppleEvent-aware Applications*/	//
#define SPELL_ALIAS_GONE	5703	/*That service cannot be found and will be removed from the menu.*/	//
#define X_STUFF	5704	/*X-Stuff: */	//
#define INVALID_STATIONERY	5705	/*That stationery is incompatible with this version of Eudora.*/	//
#define STATIONERY	5706	/*Default*/	//Name of default stationery file.
#define ONLY_IN_TREE	5707	/*Eudora can only open mailboxes that are inside your Eudora Folder.*/	//
#define NO_SETTING_HELP	5708	/*�No explanation available�*/	//
#define UUDECODE_FMT	5709	/*%pbegin 644 %p%p*/	//Don't touch.
#define BROKEN_LINK	5710	/*The original message couldn�t be found.*/	//
#define TOC_ALIAS_HEAD	5711	/*X-Link-Box:*/	//
#define XLINK_HEAD	5712	/*X-Link-Id:*/	//
#define RRT_FMT	5713	/*Return-Receipt-To: %p%p*/	//Return-receipt header.
#define SIG_MENU_HELP	5714	/*Use this menu to change the �signature� that will be attached to your mail.*/	//
#define NOSIG_HELP	5715	/*No signature text will be sent.*/	//
#define SIG_HELP	5716	/*Your standard signature text will be sent.*/	//
#define ARB_SIG_HELP	5717	/*A file of your own choosing will be sent.*/	//
#define ALT_SIG_HELP	5718	/*Your alternate signature text will be sent.*/	//
#define U_SHOW_ALL_HELP	5719	/**/	//
#define U_NO_SHOW_ALL_HELP	5720	/**/	//
#define OTHER_ITEM_TEXT	5901	/*Other...<I*/	//
#define CHOOSE_MBOX	5902	/*Please select a mailbox.*/	//
#define FLOWED_QUOTE	5903	/*>*/	//This is the quote character to use when doing format=flowed.  It must be either '>' or '> '.
#define SIG_SEPARATOR	5904	/*-- */	//Standard USENET signature separator. Do not change. Use <x-eudora-setting:13703> instead
#define UNDO_XFER	5905	/*Undo Transfer to %p*/	//
#define REDO_XFER	5906	/*Redo Transfer to %p*/	//
#define CANT_XF_UNDO	5907	/*Couldn�t undo the transfer; the transferred messages can�t be found.*/	//
#define ATTACH_GONE	5908	/*Can�t find that attachment.*/	//
#define FILTER_ANY	5909	/*�Any Header�*/	//Special string for filter to match any header. Do not localize.
#define FILTER_BODY	5910	/*�Body�*/	//Special string for filter to match body of mail.  Do not localize.
#define EXTERNAL_BODY	5911	/*external-body*/	//Don't touch.
#define MAIL_SERVER	5912	/*mail-server*/	//Don't touch.
#define EXTERNAL_MAIL	5913	/*\n[The following attachment must be fetched by mail. Command-click the URL below and send the resulting message to get the attachment.]\n*/	//
#define ANON_FTP	5914	/*anon-ftp*/	//Don't touch.
#define ANARCHIE_FMT	5915	/*%r %r://%p/%p/%p*/	//Don't touch.
#define ANARCHIE_GET	5916	/*GET*/	//Don't touch.
#define ANARCHIE_TXT	5917	/*TXT*/	//Don't touch.
#define ANARCHIE_FTP	5918	/*ftp*/	//Don't touch.
#define ANARCHIE_TYPE	5919	/*AURL*/	//
#define ANARCHIE_CREATOR	5920	/*Arch*/	//
#define EXTERNAL_FTP	6101	/*\n[The following attachment must be fetched by ftp.  Command-click the URL below to ask your ftp client to fetch it.]\n*/	//
#define IMAGE	6102	/*image*/	//Don't touch.
#define INVALID_MAP	6103	/*The EuOM maps are incorrect; cannot send the attachment.*/	//
#define ASCII	6104	/*ascii*/	//Don't touch.
#define FILTER_LABEL_LABEL	6105	/*Label:*/	//
#define TEXT_DARKER	6106	/*30*/	//Percent by which text should be darker when colored
#define AREA_LIGHTER	6107	/*0*/	//Percent by which large areas should be lighter when colored
#define FILTER_NAME	6108	/*^0:^1*/	//Template for building name of filter; ^0 is first header, ^1 is first value, ^2 is the filter id.
#define ANCIENT_LOCAL_DATE_FMT	6109	/*^1, ^0^3*/	//Format for �very old� date summaries; time, date, weekday, zone
#define OLD_LOCAL_DATE_FMT	6110	/*^2, ^0^3*/	//Format for �old� date summaries; time, date, weekday, zone.
#define LOCAL_DATE_FMT	6111	/*^0^3*/	//Format for recent date summaries; time, date, weekday, zone.
#define OLD_DATE	6112	/*-1*/	//How old is �old� for date display; in hours, or -1 for �not today�.
#define ANCIENT_DATE	6113	/*144*/	//How old is �very old� for date display; in hours.
#define PASTEL_LIGHT	6114	/*65535*/	//Lightness for a pastel color
#define PASTEL_SATUR8	6115	/*20000*/	//Saturation for a pastel color
#define FETCH_FN_FMT	6116	/*Get %p*/	//Format for naming stationery or bookmark files
#define FILTERING	6117	/*Filtering...*/	//
#define LEFT_TO_FILTER	6118	/*Messages remaining to filter:*/	//
#define BIG_MESSAGE_MSG2	6119	/*\n\nWARNING: The remainder of this %K message has not been transferred.  Turn on the �Fetch� button in the icon bar and check mail again to get the whole thing.\n*/	//
#define SPELLSWELL_CREATOR	6120	/*SPWE*/	//Creator code for default Word Services app
#define REGISTER_CREATOR	6301	/*BuyE*/	//Creator code for automatic registration program
#define REGISTER_EUDORA	6302	/*Register Eudora...*/	//Menu title for Eudora registration
#define NO_REGISTER	6303	/*Can�t find the registration program.*/	//
#define SUM_COPY_FMT	6304	/*%c\t%p\t%p\t%p\t%p\t%p\t%d\t%p\n*/	//Format for copying a summary; state, priority, attachments, label, who, date, K, subject
#define CHOOSE_URL_APP	6305	/*Please choose an application to do %p.*/	//
#define NO_KEY_FMT	6306	/*Can�t encrypt a message to �%p� without a key.  The message will be saved, but can't be sent unless you turn off encryption.*/	//
#define NO_ME_KEY	6307	/*Can't encrypt or sign a message unless you establish a key for �%p�.*/	//
#define PGP_PROTOCOL	6308	/*pgp*/	//Don't touch.
#define SECRET_KEYRING	6309	/*secring.pgp*/	//
#define ASC_SUFFIX	6310	/*.asc*/	//Don't touch.
#define MIME	6311	/*mime*/	//Don't touch.
#define MIME_ENC_PGP	6312	/*x-pgpMimeEncrypted*/	//
#define MIME_CLEAR_PGP	6313	/*x-pgpMimeClear*/	//
#define PGP_SIG_MATCH	6314	/*The signature for �%p� matches.*/	//
#define PGP_NO_MATCH	6315	/*The signature for �%r� does not match.  The message was either corrupted in transit, or possibly forged.*/	//
#define PGP_CANT_VERIFY	6316	/*Couldn't verify the signature.*/	//
#define PGP_CANT_DECRYPT	6317	/*Unable to decrypt the message.*/	//
#define PGP_CANT_ENCRYPT	6318	/*Unable to encrypt the message.*/	//
#define PGP_CANT_SIGN	6319	/*Unable to attach your signature to the file.*/	//
#define MR_PGP_BROKEN	6320	/*PGP gave no explanation*/	//
#define PGP_NOT_MESSAGE	6501	/*The encrypted message isn't in MIME format.  Please use the PGP application directly.*/	//
#define FILTER_LIST_PERCENT	6502	/*30*/	//% of the filter window the list box should take
#define BOX_SIZE_SELECT_FMT	6503	/*%d/%d/%K/%K*/	//Size display format for mailbox windows, including selection.
#define TB_FMT	6504	/*%d:%r:%p:%p*/	//DO NOT LOCALIZE
#define PGP_HAVEKEY_FMT	6505	/*pgp -kv \"%p\"*/	//PGP command to verify a key.  DO NOT LOCALIZE
#define WIPE_ERROR	6506	/*Couldn't overwrite decrypted information.*/	//
#define FILTER_ADDRESSEE	6507	/*�Any Recipient�*/	//Do not localize.
#define RNAME_QUOTE	6508	/*%q*/	//
#define KEY_FMT	6509	/*%p�%p�*/	//
#define NOSPACE_SKIP	6510	/*\n\nWARNING: The remainder of this %K message has not been transferred, because there was not enough disk space.  Make more space and check mail again to get the whole thing.*/	//
#define CONTROL_PLUS	6511	/*ctl+*/	//String to identify the control key.
#define OPTION_PLUS	6512	/*opt+*/	//String to identify the option key.
#define SHIFT_PLUS	6513	/*shft+*/	//String to identify the shift key.
#define COMMAND_PLUS	6514	/*cmd+*/	//String to identify the command key.
#define READ_STATION	6515	/*Couldn�t find stationery file.*/	//
#define TB_AUTOHELP_DELAY	6516	/*60*/	//# of ticks to wait before bringing up TB help balloon.
#define PGP_MISSING	6517	/*The PGP data in this message is incomplete.*/	//
#define SUBJECT_WARNING	6518	/*That message doesn't have a subject.  Send it anyway?*/	//
#define SIZE_WARNING	6519	/*That message will be around %d K in size.  Send it anyway?*/	//
#define SHOW_TOOLBAR	6520	/*Show Toolbar*/	//
#define HIDE_TOOLBAR	6701	/*Hide Toolbar*/	//
#define FCC_PREFIX	6702	/*�*/	//Prefix for addresses that represent mailbox names.
#define MPART_SIGNED	6703	/*signed*/	//Don't touch.
#define MPART_ENCRYPTED	6704	/*encrypted*/	//Don't touch.
#define PROTOCOL	6705	/*protocol*/	//Don't touch.
#define MICALG	6706	/*micalg*/	//PEM acronymn.  Don't touch.
#define TRANSFER_MNAME	6707	/*Transfer*/	//Name of Transfer menu.
#define FCC_NAME	6708	/*Fcc����*/	//Name of Transfer menu when it's the Fcc menu.
#define SHORT_OPEN_TIMEOUT	6709	/*5*/	//Number of seconds to timeout when testing servers, etc.
#define FILT_LIST_MIN	6710	/*35*/	//Min width of filter list box, expressed as % of smallest window size
#define FILT_REST_MIN	6711	/*60*/	//Min width of rest of filter window, expresses as % of smallest window size
#define NICK_BAD_CHAR	6712	/*.. @:,()[]<>\"*/	//Chars not allowed in nicknames, for replacement
#define NICK_REP_CHAR	6713	/*--_--�------�*/	//Replacements for above.
#define TB_NO_FILE	6714	/*There was an error launching that file.*/	//
#define TEXT_ONLY	6715	/*Can only insert plain text files.*/	//
#define ATTACH	6716	/*Attach*/	//Name of standard file button to attach file
#define IMPORTANCE_FMT	6717	/*%r %r%p*/	//Format string for generating Importance: header.
#define SELECTED	6718	/*Selected */	//Use in header when printing selected nicknames.
#define CONTINUED	6719	/*(continued)*/	//Use when continuing a nickname on another line
#define NICK_HEAD_FMT	6720	/*%pAddress Book Entries from %p*/	//Format string for nickname header.
#define NICK_PRINT_NICK_PER	6901	/*20*/	//% of printed page for use by nickname itself
#define KIDS_TODAY	6902	/*Kids today�they just won�t listen to their mothers.*/	//
#define JUST_PLAIN_HEADER	6903	/*Header*/	//
#define PAREN_STRING	6904	/*(%p)*/	//
#define MAILBOX_PRINT	6905	/*the mailbox*/	//for printing
#define MESSAGE_PRINT	6906	/*the message*/	//for printing
#define AND_PRINT	6907	/* and */	//for printing
#define NOTIFY_NORM	6908	/*as normal*/	//for printing
#define NOTIFY_REPORT	6909	/*in report*/	//for printing
#define DELETE_PRINT	6910	/*delete it*/	//for printing
#define FETCH_PRINT	6911	/*fetch it*/	//for printing
#define COULDNT_DRAG	6912	/*Couldn�t drag.*/	//
#define HESIOD	6913	/*hesiod*/	//Name of hesiod POP3 �host�
#define HESIOD_POBOX	6914	/*pobox*/	//Name of hesiod POBox service
#define HESIOD_SLOC	6915	/*sloc*/	//Name of hesiod service location param
#define HESIOD_POP3	6916	/*pop*/	//Name of hesiod POP3 service
#define HESIOD_DRIVER	6917	/*.AthenaMan 1*/	//Name of hesiod driver
#define HESIOD_LOOKUP	6918	/*Hesiod lookup (%p,%p)...*/	//
#define OUT_OUT_STYLE	6919	/*0*/	//Style for summaries of outgoing messages in Out mailbox
#define IN_OUT_STYLE	6920	/*2*/	//Style for summaries of outgoing messages outside of the Out mailbox
#define IN_IN_STYLE	7101	/*0*/	//Style for summaries of incoming messages
#define URL_COLOR	7102	/*6000,6000,48000*/	//Color to use on recognized URL�s
#define URL_STYLE	7103	/*4*/	//Style to use on recognized URL�s
#define URL_LEFT	7104	/*<[{(���`\'\"*/	//Left-side URL delimitters.
#define URL_RIGHT	7105	/*>]})���\'\'\"*/	//Right-side URL delimitters.
#define URL_IN_OK	7106	/*$-_.+!*'(),%;/?:@&=#~^*/	//Characters that do not terminate URL�s, in addition to alphanumerics.
#define EMPTY_BUTTON	7107	/*Empty*/	//
#define EMPTY_TRASH_FMT	7108	/*The trash contains %d message%#.  Do you wish to empty it now?*/	//
#define DONT	7109	/*Don�t %r*/	//
#define QUEUE_BTN	7110	/*Queue*/	//
#define XFER_BTN	7111	/*Transfer*/	//
#define NUKE_BTN	7112	/*Delete*/	//
#define TRASH_BTN	7113	/*Trash*/	//
#define AND_DONT_WARN	7114	/*%r & Don't Warn*/	//
#define HESIOD_ERR	7115	/*Hesiod lookup (%p,%p) returned error %d*/	//
#define HESIOD_NOTFOUND	7116	/*Couldn't find your information with Hesiod.*/	//
#define REPLY_COLOR	7117	/*48000,6000,6000*/	//Color used to hilite reply-to: and other such headers.
#define REPLY_SOUND	7118	/*Tink*/	//Sound played when replying to other than From:
#define STATION_SUBJ_FMT	7119	/*%p (was %p)*/	//Format for changing subject when doing a reply.
#define SPOOL_FOLDER	7120	/*Spool Folder*/	//Folder name to temporarily store outgoing attachments
#define COPY_ATTACHMENT	7301	/*Couldn't copy an attachment to add it to the new message.  Don't delete the original attachment until the new message is sent.*/	//
#define ENRICHED_MAX_WORD	7302	/*80*/	//Max length of word for unencoded text/enriched.
#define ENRICHED_SOFT_LINE	7303	/*70*/	//If we can, put a soft line break after this many characters.
#define INDENT_DISTANCE	7304	/*4*/	//How many characters indent to use for indent style.
#define COMPACTING	7305	/*Scheduling Compactions...*/	//
#define LAURENCE	7306	/*The translators are not working properly.*/	//
#define ALIGN_LIMIT	7307	/*15*/	//Max # of characters to indent headers.
#define CANT_SAVE_RICH	7308	/*Couldn�t save style information; trying to save as plain text.*/	//
#define FIXED_FONT	7309	/*Monaco*/	//Select the font to be used when someone sends you text that should be displayed in a fixed-width font.
#define MIME_RICH_PARAM	7310	/*<param>%p</param>*/	//Don't touch.
#define PLAIN_TEXT_MITEM	7311	/*Plain*/	//
#define PLAIN_ALL_ITEM	7312	/*Completely Plain*/	//
#define TEXT_COLOR	7313	/*0,0,0*/	//Eudora will draw text in this color.
#define BACK_COLOR	7314	/*65535,65535,65535*/	//Eudora will make the backgrounds of windows this color.
#define REPLY_INSET	7315	/*none,1,0,0*/	//Distances to inset left and right margins when doing rich text replies.
#define FORWARD_INSET	7316	/*none,1,0,0*/	//Distances to inset left and right margins when doing rich text forwards.
#define COMMA_SPACE	7317	/*, */	//Don�t touch.
#define FKEY_FMT	7318	/*F%d*/	//Labels for function keys.
#define DAEMON_NICKNAME	7319	/*daemon*/	//Address Book Entry for list of mailer-daemons.
#define ATTR_PASTE_AS_QUOTE	7320	/*At ^3 ^1, ^0 wrote:*/	//# attribution line for paste as quote; from, date, subject, time
#define SUBMISSION_PORT	7501	/*587*/	//New network port to use when sending mail
#define FIND_FIND	7502	/*Find*/	//
#define ATTACH_MESS_ERR	7503	/*Error %d sending attachment �%p�.*/	//
#define KEYCHAIN_WANNA	7504	/*Do you want to use the system keychain?�Eudora can store your passwords safely in the system keychain.  You will have to enter your passwords one more time, then never again.��No�Yes�*/	//
#define BOX_SIZE_SELECT_SRCH_FMT	7505	/*%d/%d*/	//Size display for search window, including selection.
#define BOX_SIZE_SELECT_EXTRA	7506	/*18*/	//Number of pixels to grow mailbox size box when also showing selection.
#define FIND_WORD	7507	/*Whole word*/	//
#define FIND_BACKWARD	7508	/*Backwards*/	//
#define FIND_CASE	7509	/*Match case*/	//
#define SORRY_AD_COVERED	7510	/*The Ad window can�t be positioned properly.�The usual positions for the ad window seem to be covered by something.  Please drag the ad window someplace where it will not be covered.  Thanks!���Yes�*/	//
#define FIND_OPTIONS	7511	/*Options:*/	//
#define FIND_FIND_LABEL	7512	/*Find:*/	//
#define CHOOSE_MBOX_TRANSFER	7513	/*Please choose a mailbox from the Transfer menu.*/	//
#define CHOOSE_MBOX_MAILBOX	7514	/*Please choose a mailbox from the Mailbox menu.*/	//
#define SEARCH_SOMETHING_FMT	7515	/*Search �%p�*/	//
#define ETL_TRANS_FAILED	7516	/*Translation failed; cannot open this item.*/	//
#define CREATE_SIG	7517	/*Couldn�t create signature.*/	//
#define ETL_CANT_INIT	7518	/*Translator initialization failed; consult translator documentation.*/	//
#define ETL_BAD_VERSION	7519	/*Translator versions don�t match; may need to upgrade Eudora or translator.*/	//
#define ETL_ERROR_FMT	7520	/*%p: %r %p\n\n{%d.%d}*/	//
#define ETL_CANT_ADD_TRANS	7701	/*Could not add translator request.*/	//
#define MIME_CHARSET	7702	/*charset*/	//DO NOT TOUCH
#define MIME_CTE	7703	/*content-transfer-encoding*/	//
#define MDN_ORIG_MID	7704	/*Original-Message-ID: %p%p*/	//
#define AD_WINDOW_SIZE_GUESS_X	7705	/*144*/	//
#define MDN_FINAL_RECIP	7706	/*Final-Recipient: rfc822; %p%p*/	//
#define MDN_DISPLAYED	7707	/*displayed*/	//Don't touch.
#define MDN_DISPLAYED_LOCAL	7708	/*displayed*/	//Localize this string for use in the human text of Message Delivery Notifications.
#define THING_SLASH_THING	7709	/*%r/%r*/	//DO NOT TOUCH
#define GRAPHIC_FILE_ERR	7710	/*Error %d with graphic file �%p�.*/	//
#define CLOSE_ENOUGH_PERCENT	7711	/*5*/	//Max distance from edge of screen, as percent of screen dimension, to consider a floating window �docked�.
#define VERY_SMALL_PERCENT	7712	/*1*/	//Max size of docked window portion, as percent of screen dimension, to just reduce the screen rect by for window placement.
#define MIN_WIN_HI	7713	/*15*/	//Minimum size for zoom rectangles, in lines.
#define NARROW_PERCENT	7714	/*15*/	//Max size of docked window portion to consider a �narrow� portion.
#define MDN_DISPOSITION	7715	/*Disposition: %r/%r; %r%p*/	//
#define MIME_REPORT	7716	/*report*/	//
#define MDN_DISPO_NOTIFY	7717	/*disposition-notification*/	//
#define MDN_DESCRIP	7718	/*Your message of %p regarding ``%p''\015has been %r by %p.\015\015*/	//
#define MDN_REPORT_PARAM	7719	/* ;report-type=disposition-notification\015*/	//
#define MDN_SUBJECT	7720	/*Notification for ``%p''*/	//
#define MDN_REQUEST	7901	/*The sender has requested notification that you have seen this.*/	//
#define CANT_REGISTER	7902	/*Cannot register now; internal error.*/	//
#define ETL_CANT_TRANSLATE	7903	/*Cannot translate.*/	//
#define ETL_CANT_FIND_TRANS	7904	/*One of the requested translators can't be found; the mail will not be sent.*/	//
#define QUOTH	7905	/**/	//No longer used.
#define UNQUOTH	7906	/**/	//No longer used.
#define NOTIFY_SOUND	7907	/*Tink*/	//Sound played when a Return Receipt is requested.
#define FILT_DATE_LABEL	7908	/*Last used %p*/	//Label for filter last use date
#define COPY_PLAIN_ITEXT	7909	/*Copy Without Styles*/	//
#define PASTE_PLAIN_ITEXT	7910	/*Paste Without Styles*/	//
#define COPY_UNWRAP_PLAIN_ITEXT	7911	/*Copy Without Styles & Unwrap*/	//
#define PASTE_ITEXT	7912	/*Paste*/	//
#define CUT_PLAIN_ITEXT	7913	/*Cut Without Styles*/	//
#define CUT_ITEXT	7914	/*Cut*/	//
#define CUT_UNWRAP_ITEXT	7915	/*Cut & Unwrap*/	//
#define CUT_UNWRAP_PLAIN_ITEXT	7916	/*Cut Without Styles & Unwrap*/	//
#define CANT_READ_SIG	7917	/*Couldn�t read the signature file for that message.*/	//
#define TB_H_DESK_MARGIN	7918	/*64*/	//Margin from the right of the toolbar to the right of the desktop.
#define TB_V_DESK_MARGIN	7919	/*48*/	//Margin from bottom of the toolbar to the bottom of the desktop.
#define MIME_TEXT_SUBTYPE_FMT	7920	/*%r: %r/%p; %r=\"%p\"%p*/	//
#define MIME_TEXTNOTPLAIN	8101	/*%r: %r/%p%p%p*/	//
#define URL_TRAIL_IGNORE	8102	/*.,)]*/	//Characters to ignore at the ends of improper URL's.
#define MIME_ALTERNATIVE	8103	/*alternative*/	//Don't touch.
#define HESIOD_PASSWD	8104	/*passwd*/	//Hesiod name for unix password information.
#define ETL_IM_STUPID	8105	/*Eudora does not support enough of the Translation Services API for this.*/	//
#define PICT_SPOOL_SIZE	8106	/*30*/	//Size in K over which Eudora will spool picture files
#define PICT_HIDE_SIZE	8107	/*90*/	//Size in K over which Eudora will not attempt to display picture files
#define NO_STATIONERY	8108	/*No Stationery*/	//
#define NO_TRANSLATORS	8109	/*None Installed*/	//
#define MESS_WRITE_HELP	8110	/*You may change the contents of this message.  Click the pencil again to save the contents.*/	//
#define NO_MESS_WRITE_HELP	8111	/*Click this icon to allow the contents of the message to be edited.*/	//
#define URL_FMT	8112	/* <%r://%p/>*/	//Don't touch.
#define TOO_MUCH_MEMORY	8113	/*This operation would require %d K more memory than is readily available.  Before trying again, please close some windows or quit Eudora and use the Finder�s �Get Info� command to increase Eudora�s memory size.*/	//
#define COPYING	8114	/*Copying*/	//Progress title when copying a file.
#define AD_WINDOW_SIZE_GUESS_Y	8115	/*152*/	//
#define IMPORT_MESSAGE_TRY_AGAIN	8116	/*Eudora couldn't find any files to import.�Would you like to try again using using a specific importer?��No-�Yes�*/	//
#define SETTINGS_BUSY	8117	/*Another program is using your settings file.  Might you be running another copy of Eudora?*/	//
#define PH_BG_IDLE	8118	/*30*/	//Number of seconds in background after which ph window will be disconnected
#define PH_FG_IDLE	8119	/*300*/	//Number of seconds in foreground after which ph window will be disconnected
#define INSERT_CONFIGURATION	8120	/*Insert System Configuration*/	//
#define PH_LIVE_MIN_CHARS	8301	/*3*/	//Minimum # of characters to trigger a live ph query
#define PH_LIVE_MIN_TICKS	8302	/*60*/	//Minimum # of ticks to trigger a live ph query
#define PH_RETURN_KEYWORD	8303	/*return*/	//Don't touch.
#define STUFF_FOLDER	8304	/*Eudora Stuff*/	//Name of folder for holding translators
#define NO_PETE	8305	/*Couldn�t find Eudora�s text editor component.*/	//
#define BAD_PETE	8306	/*The text editor component you have installed is the wrong version.*/	//
#define ETL_ICON_HELP_FMT	8307	/*�p�p%^1*/	//
#define ATTR_TIME_FMT	8308	/*%p%p*/	//Format for time and zone in attributions.
#define CANT_ADD_NESTED	8309	/*New� and Other� from subfolders cannot be added to the toolbar.*/	//
#define MESS_NOTIFY_HELP	8310	/*Click the �Notify Sender� button to send a receipt to the sender telling him you received the message.  Hold down the option key and click to get rid of the request without sending the notice.*/	//
#define FILT_LOG_FMT	8311	/*Filter �%p� matches �%p�*/	//Format for logging filter matches
#define NEED_COMPONENT_MGR	8312	/*Eudora requires the �Component Manager�; this is on your original disks, and is also part of QuickTime� and Macintosh Easy Open�.*/	//
#define FILTER_ADDRESSEE_OLD	8313	/*�Any Addressee�*/	//Do not localize.
#define MIME_MACTEXT	8314	/*x-mac-text*/	//Don't touch.
#define READING_FILTERS	8315	/*Reading filters...*/	//
#define SPECIAL_KEY_FMT	8316	/*%p%p*/	//Format for toolbar key name for special keys
#define NEW_MESSAGE_WITH	8317	/*New Message With*/	//
#define OPEN_HELP_ERR	8318	/*This file provides help texts to Eudora.�Put it in the same folder as the Eudora application.  The help will appear in the Help menu or in the menu with the help icon on the rightish side of your menu bar.*/	//
#define OPEN_PLUG_ERR	8319	/*This file is a resource plug-in for Eudora.  Put it in your Eudora Folder or the system Preferences folder or the same folder as the Eudora application.*/	//
#define ABOUT_TRANSLATORS	8320	/*About Message Plug-ins...*/	//
#define BULK	8501	/*bulk*/	//Don't touch.
#define NO_MAIN_NICK	8502	/*Cannot read main address book.*/	//
#define DOC_DAMAGED_ERR	8503	/*The window has internal errors and will be closed.*/	//
#define USA_EUDORA_FOLDER	8504	/*Eudora Folder*/	//If the Spool Folder isn't found, this filename will be looked for and renamed to Spool Folder.
#define USA_EUDORA_NICKNAMES	8505	/*Eudora Nicknames*/	//If Eudora Nicknames isn't found, this filename will be looked for and renamed to Eudora Nicknames.
#define USA_IN	8506	/*In*/	//If the In mailbox isn't found, this filename will be looked for and renamed to In.
#define USA_OUT	8507	/*Out*/	//If the Out mailbox isn't found, this filename will be looked for and renamed to Out.
#define USA_TRASH	8508	/*Trash*/	//If the Trash mailbox isn't found, this filename will be looked for and renamed to Trash.
#define USA_ATTACHMENTS_FOLDER	8509	/*Attachments Folder*/	//If the Attachments Folder isn't found, this filename will be looked for and renamed to Attachments Folder.
#define USA_SIG_FOLDER	8510	/*Signature Folder*/	//
#define USA_STATION_FOLDER	8511	/*Stationery Folder*/	//
#define USA_SPOOL_FOLDER	8512	/*Spool Folder*/	//
#define USA_NICKNAME_FOLDER	8513	/*Nicknames Folder*/	//
#define USA_SIGNATURE	8514	/*Standard*/	//
#define USA_ALTERNATE	8515	/*Alternate*/	//If the alternate signature isn't found, this filename will be looked for and renamed to Alternate.
#define USA_SETTINGS	8516	/*Eudora Settings*/	//If Eudora Settings isn't found, this filename will be looked for and renamed to Eudora Settings.
#define USA_DEFAULT	8517	/*Default*/	//If the default stationery isn't found, this filename will be looked for and renamed to Default.
#define NO_RECIPS	8518	/*No Quick Recipients*/	//
#define LIGHT_MSGID_FMT	8519	/*<l%p@%p>*/	//Don't touch.
#define ALIAS_NICK_LIST_PRCT	8901	/*50*/	//
#define USA_PERSONAL_ALIAS_FILE	8902	/*Personal Nicknames*/	//If Personal Nicknames isn't found, this filename will be looked for and renamed to Personal Nicknames.
#define ALIAS_ON_RECIPIENT_LIST	8903	/*Recipient List*/	//Title for on recipient list checkbox
#define ALIAS_UNTITLED_NICK	8904	/*untitled*/	//Title for new nickname created via AppleEvents
#define REGISTRATION_QUEUED	8905	/*Your registration has been placed in your Out mailbox, and will be sent next time you send queued messages.*/	//
#define REGISTRATION_SENT	8906	/*Your registration has been sent.  You should receive a reply shortly.*/	//
#define ALIAS_NEW_NICK_ERR_QUIT	8907	/*There is insufficient memory to create a new nickname. The nicknames window is going to be closed.*/	//
#define ALIAS_NEW_NICK_ERR	8908	/*There is insufficient memory to create a new nickname.*/	//
#define ALIAS_REPLACE_NICK_ERR	8909	/*There is insufficient memory to modify the current nickname.*/	//
#define ALIAS_GET_NICK_DATA_ERR	8910	/*There is insufficient memory to manipulate current nickname.*/	//
#define ALIAS_SORT_ERR	8911	/*There is insufficient memory to sort the nickname list.*/	//
#define ALIAS_NEW_NICK_FILE_ERR	8912	/*There is insufficient memory to create a new nickname file.*/	//
#define ALIAS_REMOVE_DEFAULT_NICK_FILE	8913	/*The address book file \"Eudora Nicknames\" cannot be removed.*/	//
#define UNUSED_SITE_TO_VISIT	8914	/**/	//
#define PRO_ONLY_FEATURE	8915	/* (PAY Only)*/	//
#define FILE_ALIAS_IN	8916	/*In*/	//Localize this to the name the user should see for the In mailbox.
#define FILE_ALIAS_OUT	8917	/*Out*/	//Localize this to the name the user should see for the Out mailbox.
#define FILE_ALIAS_EUDORA_FOLDER	8918	/*Eudora Folder*/	//Localize this to the name the user should see for the Eudora Folder.
#define FILE_ALIAS_TRASH	8919	/*Trash*/	//Localize this to the name the user should see for the Trash mailbox.
#define FILE_ALIAS_STANDARD	8920	/*Standard*/	//Localize this to the name the user should see for the standard signature.
#define FILE_ALIAS_ALTERNATE	9101	/*Alternate*/	//Localize this to the name the user should see for the alternate signature.
#define FILE_ALIAS_NICKNAMES	9102	/*Eudora Nicknames*/	//Localize this to the name the user should see for Eudora Nicknames.
#define PRO_FILT_WARNING	9103	/*Your Eudora Filters file appears to have been created with with Eudora in Paid or Sponsored Mode.  These filter actions are not available in Light Mode so your filters may not be fully functional.*/	//
#define PREVIEW_HEADER_HIDE	9104	/*12*/	//Preview height in lines above which headers are allowed to show in preview pane.
#define USA_PHOTO_FOLDER	9105	/*Photo Folder*/	//If Photo Folder isn't found, this filename will be looked for and renamed to Photo Folder.
#define PERS_SAVE_ERR	9106	/*Error %d saving information for personality �%p�*/	//
#define GX_MEMORY_INIT_ERR	9107	/*There is not enough memory available to use Quickdraw GX.  Plain old boring printing features will be enabled instead.*/	//
#define GX_INIT_ERR	9108	/*Could not initialize Quickdraw GX.  Plain old boring printing features will be enabled instead.*/	//
#define GX_MEMORY_ERR	9109	/*A QuickDraw GX memory related error has occurred.  Try giving Eudora more memory.*/	//
#define GX_UPDATE_ERR	9110	/*Could not update the page setup record for GX.  Your printer may be confused.*/	//
#define GX_PRINT_SELECTION	9111	/*selected messages*/	//
#define YOU_ARE_PSYCHO	9112	/*Cannot have more than 99 personalities.*/	//
#define DOMINANT	9113	/*�Dominant�*/	//Name of the dominant personality.
#define USED_PERSONALITY	9114	/*You already have a personality called that.*/	//
#define QUEUE_M_ITEM	9115	/*Queue For Delivery*/	//
#define SEND_M_ITEM	9116	/*Send Immediately*/	//
#define PERS_CHECK_SLOP	9117	/*5*/	//Number of minutes of slop allowed when auto-checking for multiple personalities.
#define OT_INIT_ERR	9118	/*An error occurred while initializing Open Transport.  MacTCP will be used instead.*/	//
#define HTML_SUFFIX	9119	/*html*/	//Don't touch.
#define HTTP	9120	/*http*/	//Don't touch.
#define SAVING_HTML	9301	/*Couldn�t send the html to your browser.*/	//
#define ITEMS_FOLDER	9302	/*Eudora Items*/	//Folder name for miscellaneous items in the Eudora settings folder
#define PLUGINS_FOLDER	9303	/*Plugins*/	//Folder name for plugin settings
#define OT_UNKNOWN_ERR	9304	/*An Open Transport related error has occurred.*/	//
#define AE_SHORT_TIMEOUT_TICKS	9305	/*600*/	//Number of ticks for really prompt AE operations.
#define PERS_CHECKING_MAIL	9306	/*Checking mail for %p�*/	//
#define PERS_SENDING_MAIL	9307	/*Sending mail for %p�*/	//
#define TOOLBAR_EXTRA_PIXELS	9308	/*4*/	//The number of extra pixels to put between groups of toolbar buttons.
#define TOOLBAR_EXTRA_COUNT	9309	/*4*/	//The number of toolbar buttons to put together in a group.
#define TOOLBAR_SEP_PIXELS	9310	/*-1*/	//The number of pixels between two adjacent toolbar buttons.
#define TXT_FMT_BAR_HEIGHT	9311	/*26*/	//height of text formatting toolbar
#define TXT_FMT_BAR_FONT_FAM_ID	9312	/*3*/	//font family ID for popups in text formatting toolbar
#define TXT_FMT_BAR_FONT_SIZE	9313	/*9*/	//font size for popups in text formatting toolbar
#define TXT_FMT_BAR_POPUP_CHECK_MARK	9314	/*�*/	//check mark character for popups in text formatting toolbar
#define TXT_FMT_BAR_FONT_POPUP_MAX_WIDTH	9315	/*35*/	//maximum width for font popup in text formatting toolbar
#define TXT_FMT_BAR_SIZE_POPUP_MAX_WIDTH	9316	/*35*/	//maximum width for size popup in text formatting toolbar
#define TXT_FMT_BAR_COLOR_POPUP_MAX_WIDTH	9317	/*40*/	//maximum width for color popup in text formatting toolbar
#define TRANSLATING	9318	/*Translating...*/	//
#define PERS_DELETE	9319	/*Really delete personality �%p�?  The deletion cannot be undone.*/	//
#define OT_INET_SVCS_ERR	9320	/*An error occurred while initializing Open Transport Internet Services.*/	//
#define OT_CON_MEM_ERR	9501	/*There was not enough memory to start a new connection.*/	//
#define OT_PPP_CONNECT	9502	/*Establishing PPP connection ...*/	//
#define OT_DIALUP_CONNECT_ERR	9503	/*An error occurred while establishing the connection.*/	//
#define OT_PPP_DISCONNECT	9504	/*Closing current connection ...*/	//
#define URL_BODY	9505	/*body*/	//Don't touch.
#define OT_PPP_STATE_ERR	9506	/*Unable to determine the current state of the PPP connection.*/	//
#define OT_PPP_CONNECT_MESSAGE	9507	/*Connecting ...*/	//
#define OT_TCPIP_PREF_ERR	9508	/*Unable to open your TCP/IP preferences file.*/	//
#define PH_NEWLINE	9509	/**/	//Newline for use with ph.  Don't set this if the POP/SMTP newline is ok.
#define UNUSEND_WAS_DOC_DAMAGED_FMT	9510	/**/	//
#define PH_SERVER_SERVER	9511	/**/	//If non-empty, this host will be used for getting the list of ph servers.
#define INTERNATIONAL_FAILURE	9512	/*Couldn�t setup the script manager.*/	//
#define OT_PPP_REDIALING	9513	/*Redialing ...*/	//
#define LOCALIZED_VERSION_LANG	9514	/*en*/	//Two-character language code for this localized version.
#define CREATING_PERSONAL_ALIAS	9515	/*Couldn�t create your Personal Nicknames file.*/	//
#define CREATE_STA	9516	/*Couldn�t create stationery file.*/	//
#define WRITE_STA	9517	/*There was an error saving the stationery.*/	//
#define THREAD_MANAGER_WARNING	9518	/*The Thread Manager is not installed in this version of your OS. For faster performance, you may want to upgrade.*/	//
#define SENDING_WARNING	9519	/*Eudora is busy sending some of these messages. You cannot modify them until they have been sent.*/	//
#define IN_TEMP	9520	/*In.temp*/	//Name of Temporary In mailbox file--Don't touch.
#define OUT_TEMP	9701	/*Out.temp*/	//Name of Temporary Out mailbox file--don't touch
#define R822_DATE_FMT	9702	/*%r, %d %r %d %d%d:%d%d:%d%d %c%d%d%d%d*/	//DON'T TOUCH.
#define PERS_RENAME	9703	/*Really rename this personality? If you rename, mail currently stored with this personality will belong to the dominant personality.*/	//
#define MAIL_FOLDER_NAME	9704	/*Mail Folder*/	//Name of the mail folder within the Eudora Folder.  DO NOT LOCALIZE.
#define MOVING_MAILBOXES	9705	/*An error occurred moving one of your mailboxes to the mail subfolder.  You may need to move this one with the Finder.*/	//
#define SAME_VOLUME_DUMMY	9706	/*Only folders on the same volume as your attachments folder may be used in a filter action.*/	//
#define OLD_FILTER	9707	/*30*/	//Number of days of non-use before a filter is considered old.
#define NICK_POPUP_FONT_FAM_ID	9708	/*3*/	//font family ID for nickname sticky popup menu
#define NICK_POPUP_FONT_SIZE	9709	/*9*/	//font size for nickname sticky popup menu
#define IDDB_FILE	9710	/*IDDB*/	//DO NOT LOCALIZE
#define MAILDB_FILE	9711	/*MailDB*/	//DO NOT LOCALIZE
#define TOC_FILE	9712	/*TOC*/	//DO NOT LOCALIZE
#define INFO_FILE	9713	/*Info*/	//DO NOT LOCALIZE
#define SHORT_MODAL_IDLE_SECS	9714	/*10*/	//Time in seconds Eudora must be idle before taking control from user.
#define TP_LEFT_TO_DELIVER	9715	/*Messages to deliver:*/	//
#define PROCESSING Processing...	9716	/**/	//
#define LEFT_TO_MOVE	9717	/*Messages remaining to move:*/	//
#define MAKEFILTER_ERR	9718	/*An error occurred while opening the Make Filter dialog.*/	//
#define OT_MISSING_LIBRARY	9719	/*An OT Library is missing.*/	//
#define MIME_RELATED	9720	/*Related*/	//DON'T TOUCH
#define FWD_PREFIX	9901	/*Fwd: */	//Prefix to add to subjects of forwarded messages.
#define MDN_MAN_ACTION	9902	/*manual-action*/	//DO NOT LOCALIZE
#define MDN_AUTO_ACTION	9903	/*automatic-action*/	//DO NOT LOCALIZE
#define MDN_MAN_SENT	9904	/*MDN-sent-manually*/	//DO NOT LOCALIZE
#define MDN_AUTO_SENT	9905	/*MDN-sent-automatically*/	//DO NOT LOCALIZE
#define CID_SEND_FMT	9906	/*%r: <%p>%p*/	//Format string for content-id header. DO NOT LOCALIZE
#define MF_DEFAULT_FOLDER_NOT_FOUND	9907	/*The default folder you specified for new mailboxes no longer exists.  Eudora will use the Mail Folder instead.*/	//
#define MF_SAVE_FILTER_ERROR	9908	/*There was an error saving this filter.*/	//
#define PERSONALITIES_SETTING	9909	/*Personalities�mPrs*/	//Name of Personalities settings panel
#define UNSYNCH_OUT_TEMP	9910	/*Eudora encountered some messages that it had problems sending. They have been transferred to your Out box.*/	//
#define MF_NO_COMMON_ERROR	9911	/*Eudora can't detect any common elements to make a filter with.  Try adjusting your selection, or use the filters window directly.*/	//
#define MF_SCAN_ERROR	9912	/*An error occurred while scanning the messages.*/	//
#define MF_MISC_ERR	9913	/*An error occurred with the Make Filter window.*/	//
#define BG_YIELD_INTERVAL	9914	/*30*/	//During mailcheck, # of ticks before Eudora gives up the CPU when Eudora is in the background.
#define FG_YIELD_INTERVAL	9915	/*60*/	//During mailcheck, # of ticks before Eudora gives up the CPU when Eudora is in the foreground.
#define RELATED_FMT	9916	/*%r: :%p:%p:%x:%x:%x:%x\n*/	//Format string for multipart/related attachments
#define MHTML_INFO_TAG	9917	/*<!x-stuff-for-pete base=\"%p\" src=\"%p\" id=\"%d\" charset=\"%p\">*/	//
#define PARTS_FOLDER	9918	/*Parts Folder*/	//DO NOT LOCALIZE
#define NICK_COLOR	9919	/*1,1,1*/	//Color to use for displaying nicknames in recipient fields.��  1,1,1 for default.
#define NICK_STYLE	9920	/*1*/	//QuickDraw style code of text style to use for displaying nicknames in recipient fields
#define NICK_WATCHER_LOAD_ERR	10101	/*An error occurred while initializing nickname utilities.*/	//
#define ACAPPING	10102	/*Auto-configuring...*/	//
#define VCARD_FILE_EXTENSION	10103	/*vcf*/	//
#define ACAP_FAILED	10104	/*Automatic configuration failed.  You will have to configure Eudora manually.*/	//
#define ACAP_SERVER_SEARCH	10105	/*ACAP Search...*/	//
#define MAX_MULTI_DEPTH	10106	/*20*/	//Do not parse MIME structures deeper than this
#define LOAD_SETTINGS_NOW	10107	/*Fetch Settings Now*/	//
#define LDAP_PORT_REALLY	10108	/*389*/	//Port for LDAP.
#define ACAP_PORT	10109	/*674*/	//Port for Auto Config.
#define DUMMY_POP_AUTHENTICATE	10110	/**/	//Dummy value for acap.
#define DUMMY_SEND_FORMAT	10111	/**/	//Dummy value for acap.
#define NO_APPEARANCE	10112	/*Can't find Appearance Manager. Please install the Appearance Manager extensions.*/	//
#define DRAG_EXPAND_TICKS	10113	/*30*/	//# of ticks to wait while mouse is over expansion triangle.
#define AUTHPLAIN_OUTER_FMT	10114	/*%r %p*/	//format for doing AUTH PLAIN with initial argument.
#define PLUGIN_FILTERS	10115	/*Plugin Filters*/	//Name of folder for filter plugins
#define PLUGIN_NICKNAMES	10116	/*Plugin Nicknames*/	//Name of folder for filter nicknames
#define BODY_EQUALS	10117	/* body=%r*/	//
#define MF_NONE_CHOSEN	10118	/*<<None Chosen>>*/	//
#define FSE_NAME_ERR	10119	/*%p (%d); */	//
#define TP_LAST_CHECK	10120	/*Last Check: %p*/	//
#define TP_NEXT_CHECK	10301	/*Next Check: %p*/	//
#define TP_CHECKING_NOW	10302	/*Checking now�*/	//
#define TP_NEVER	10303	/*Never*/	//
#define TP_NOT_SCHEDULED	10304	/*Not scheduled*/	//
#define TP_WAITING_TO_DELIVER	10305	/*Incoming mail waiting to be delivered at idle time.*/	//
#define TP_CLICK_TO_DELIVER	10306	/*Click button to deliver immediately.*/	//
#define THREAD_CANT_CHECK	10307	/*Eudora could not check mail in the background. Try closing some windows or switch background threading off.*/	//
#define THREAD_CANT_SEND	10308	/*Eudora could not send mail in the background. Try closing some windows or switch background threading off.*/	//
#define PROCESSING_OUT	10309	/*Processing outgoing messages...*/	//
#define THREAD_CANT_SEND_ALL	10310	/*Eudora may not be able to send all your queued messages.*/	//
#define WU_NOERR_FMT	10311	/*\n{%d:%d}*/	//
#define DEFAULT_LDAP_HOST	10312	/*ldap.four11.com*/	//Default LDAP host.
#define LDAP_NAME	10313	/*LDAP*/	//
#define THREAD_LOW_STACK	10314	/*Could not fetch this message because Eudora is low on stack memory.  Please try again.*/	//
#define LOW_STACK	10315	/*Could not fetch this message because Eudora is low on stack memory.*/	//
#define FILTER_ITEXT	10316	/*Filter Messages*/	//
#define FILTER_WAITING_ITEXT	10317	/*Filter Waiting Messages*/	//
#define REVERT_TO_DEFAULT_TABS	10318	/*Revert To Default Tabs*/	//Replaces �Tabs� menu item when option key down
#define TABS_ITEXT	10319	/*Tabs*/	//�Tabs� menu item when no option key
#define ADD_BULLETS	10320	/*With Bullets*/	//
#define DEF_FIXED_MWIDTH	10501	/*80*/	//Default width in chars when fixed button is hit.
#define LDAP_SHARED_LIB_TRUE_NAME	10502	/*Eudora LDAP Library*/	//
#define LDAP_INIT_ERR	10503	/*An error occurred while initializing LDAP.*/	//
#define LDAP_NOT_LOADED_MSG	10504	/*Error:\n\nEudora cannot do LDAP searches because the Eudora LDAP Library is not properly installed.*/	//
#define LDAP_OPEN_ERR_MSG	10505	/*Error:\n\nCannot open session with LDAP server\n*/	//
#define LDAP_RESULT_COUNT	10506	/*Number of matches: */	//
#define LDAP_RESULT_ERR	10507	/*An error occurred:\n*/	//
#define LDAP_SEPARATOR	10508	/*------------------------------------------------------------*/	//
#define MIN_LDAP_LABEL_LEN	10509	/*18*/	//
#define DEF_FIXED_SIZE	10510	/**/	//Enter the font size to use for displaying fixed-width text.
#define MESS_FIXED_HELP	10511	/*Message is currently being displayed in a fixed-width font; click to revert to normal font*/	//
#define NO_MESS_FIXED_HELP	10512	/*To display message in a fixed-width font, click this */	//
#define BUILD_POPD	10513	/*Error remembering what mail has been fetched.  Try again later.*/	//
#define PREVIEW	10514	/*Preview*/	//Internal use.
#define HTML_DOCTYPE	10515	/*html public \"-//W3C//DTD W3 HTML//EN\"*/	//DO NOT LOCALIZE
#define HTML_STYLE_TYPE	10516	/*text/css*/	//DO NOT LOCALIZE
#define TOLERABLE_CTLCHARS_PPT	10517	/*5*/	//Max # of control characters per thousand for an attachment still to be text.
#define ERR_PERS_CHECKING_MAIL	10518	/*Error while checking mail for %p:*/	//
#define ERR_PERS_SENDING_MAIL	10519	/*Error while sending mail for %p:*/	//
#define THREAD_RECV_TIMEOUT	10520	/*120*/	//Background receive timeout, in seconds.
#define SASL_CANCEL	10701	/***/	//SASL cancel string.  DO NOT LOCALIZE
#define ERR_PERS_UNKNOWN_TASK	10702	/*Error while performing unknown task for %p:*/	//
#define BAD_ADDRESS_ERR_TEXT	10703	/*Can�t send to �%p�; SMTP server says �%p�.*/	//
#define TP_CHECKING	10704	/*Checking*/	//
#define TP_SENDING	10705	/*Sending*/	//
#define TP_OFFLINE	10706	/*Offline*/	//
#define UNKNOWN_ERR_TEXT	10707	/*An unknown error occurred while transferring this message.*/	//
#define CHANGE_PW	10708	/*Change Password...*/	//
#define FORGET_PW	10709	/*Forget Password*/	//
#define CHANGE_PERS_PW	10710	/*Change Password for Selected Personalities...*/	//
#define FORGET_PERS_PW	10711	/*Forget Password for Selected Personalities*/	//
#define CID_ONLY_FMT	10712	/*%p.%d.%d*/	//
#define LINK_COLOR	10713	/*6000,6000,48000*/	//Color for HTML links
#define LINK_STYLE	10714	/*4*/	//Style for HTML links
#define NOT_URL	10715	/*Are you sure that is an URL?�The text you have entered doesn�t look like proper URL syntax.��Cancel-�Link As Is�*/	//
#define OPEN_AT_END_THRESH	10716	/*1024*/	//Max # of characters in a quote-selection reply for the quotation not to be selected.
#define LDAP_NOT_SUPPORTED_MSG	10717	/*Error:\n\nEudora cannot do LDAP searches because an error occurred\n*/	//
#define PH_NAME	10718	/*Ph*/	//
#define PH_HOST_COMMON_PREFIX	10719	/*ns*/	//
#define LDAP_HOST_COMMON_PREFIX	10720	/*ldap*/	//
#define BAD_HEXBIN_ERR_TEXT	10901	/*%p%pMessage and/or attachments are probably corrupt.*/	//
#define SEP_LINE_DARK_COLOR	10902	/*42000,42000,42000*/	//RGB Color for separator lines on light background.
#define SEP_LINE_LIGHT_COLOR	10903	/*65535,65535,65535*/	//Color for separator lines on dark background.
#define LIGHT_COLOR	10904	/*50000*/	//Threshold for use of Light color lines.
#define SOME_PART	10905	/*an image*/	//
#define CHOOSE_MBOX_FOLDER	10906	/*Please choose a folder from the Mailbox menu.*/	//
#define CHOOSE_MBOX_THIS_FOLDER	10907	/*This Folder����*/	//Text that replaces the New item in the mailbox menu
#define OT_PPP_WAIT	10908	/*Waiting for PPP connection ...*/	//
#define PH_LABEL_NO_TYPE	10909	/*Enter query (server is %p):*/	//
#define ISO_2022_JP	10910	/*iso-2022-jp*/	//DO NOT TOUCH
#define NICKFILE_TOO_LONG	10911	/*Address book names must be no more than 31 characters long.*/	//
#define NICKFILE_BADCHAR	10912	/*Address book names must not contain colons.*/	//
#define NICKFILE_DUPLICATE	10913	/*You already have an address book called that.*/	//
#define GRAPHIC_MEM	10914	/*Insufficient memory to display graphic.*/	//
#define MOVING_MESSAGES_TO_IN	10915	/*Moving Messages to In Box...*/	//
#define LDAP_SCOPE_BASE_STR	10916	/*base*/	//
#define LDAP_SCOPE_ONELEVEL_STR	10917	/*one*/	//
#define LDAP_SCOPE_SUBTREE_STR	10918	/*sub*/	//
#define MAILTO_BODY	10919	/*body*/	//
#define HTML_NOSPACESTYLE	10920	/*blockquote, dl, ul, ol, li { padding-top: 0 ; padding-bottom: 0 }*/	//
#define VICOM_FACTOR	11101	/*4*/	//Factor to divide FG hog time when VICOM Internet Gateway is running.
#define LDAP_SHARED_LIB_ARCH_TYPE	11102	/*????*/	//
#define LDAP_NOT_ENABLED_MSG	11103	/*Error:\n\nEudora cannot do LDAP searches because LDAP is not enabled in this version of Eudora.*/	//
#define LDAP_LIB_OPEN_ERR_MSG	11104	/*Error:\n\nAn error occurred while loading the Eudora LDAP library\n*/	//
#define LDAP_NEEDS_NEWER_CFM68K_MSG	11105	/*Error:\n\nEudora cannot do LDAP searches because the version of the CFM-68K Runtime Enabler installed is too old.  You must install version 4.0 or later of the CFM-68K Runtime Enabler, or be running Mac OS 8.0 or later.*/	//
#define NO_EF_DESKTOP	11106	/*Eudora Settings location problem.�You cannot use �%p� for an Eudora Folder.  Please move the settings file into its own folder and try again.���Cancel�*/	//
#define EF_WARNING	11107	/*Are you sure that�s an Eudora Folder?�The folder �%p� doesn�t seem to be a proper folder for mail.  Really convert �%p� for use as an Eudora Folder?��Cancel��Convert*/	//
#define TP_UNKNOWN_ERR_TEXT	11108	/*One or more unknown errors have occurred. Eudora could not report this due to a memory error.*/	//
#define TP_UNKNOWN_PERS	11109	/*Unknown personality*/	//
#define JUST_FOR_KIRAN	11110	/**/	//String to use as delimitter to sneak headers into the subject line.
#define LDAP_LIB_VERS_BAD_MSG	11111	/*Error:\n\nEudora cannot do LDAP searches because the currently installed Eudora LDAP Library is incompatible with this version of Eudora.*/	//
#define LDAP_LIB_OPEN_MEM_ERR_MSG	11112	/*Error:\n\nThere is not enough memory to load the Eudora LDAP library.  Try increasing Eudora's memory partition.*/	//
#define LDAP_OPEN_MEM_ERR_MSG	11113	/*Error:\n\nThere is not enough memory to open a session with the LDAP server.*/	//
#define MASTER_SERVER_SERVER	11114	/*ns.eudora.com*/	//Server to use to get serverlist if current server is dumb.
#define LDAP_NEEDS_CFM68K_MSG	11115	/*Error:\n\nEudora cannot do LDAP searches because the CFM-68K Runtime Enabler is not installed.*/	//
#define DIR_SVC_SERVER_RESOLVE_ERR_MSG	11116	/*Error:\n\nUnable to connect to the specified Directory Services server.*/	//
#define LDAP_SEARCH_REQUIRES_BASE_OBJECT	11117	/*Error:\n\nEudora cannot do an LDAP lookup because a Base Object has not been specified for the search.  You can specify a Base Object as part of the LDAP server�s URL.*/	//
#define LDAP_SEARCH_USER_CANCELED_MSG	11118	/*LDAP lookup canceled by user.*/	//
#define NICK_VIEW_ANNOTATE	11119	/*%p �%p�*/	//String to use to annotate nickname list when viewing by other than name or nickname
#define LZ_WIDTH	11120	/*WIDTH*/	//ActiveX width keyword; DO NOT LOCALIZE
#define LZ_HEIGHT	11301	/*HEIGHT*/	//ActiveX height keyword; DO NOT LOCALIZE
#define LZ_TRUE	11302	/*true*/	//ActiveX true; DO NOT LOCALIZE
#define LZ_FALSE	11303	/*false*/	//ActiveX false; DO NOT LOCALIZE
#define HTMLISH_SUFFICES	11304	/*.html�.htm*/	//File suffices (separated by bullets) that mean a file is html
#define LZ_URL	11305	/*url*/	//Parameter that lizzie uses to set initial URL.  DO NOT LOCALIZE
#define UVN	11306	/*Upgrade Validation Number*/	//DO NOT TOUCH
#define TOO_EARLY_FILE	11307	/*2398377600*/	//File dates older than this will not be believed; default is Jan 1, 1980.
#define CACHE_FOLDER	11308	/*Cache Folder*/	//DO NOT LOCALIZE
#define NICK_STORED_BAD_CHAR	11309	/*..@:,()[]<>\"*/	//Chars not allowed in existing nicknames
#define NICK_STORED_REP_CHAR	11310	/*----�------�*/	//Replacements for above.
#define DELIVERY_FOLDER	11311	/*Delivery Folder*/	//Folder name to temporarily store received mail
#define USA_DELIVERY_FOLDER	11312	/*Delivery Folder*/	//
#define DELIVERY_BATCH	11313	/*3*/	//Number of received messages required before deliverable.
#define FILTER_HOG_TICKS	11314	/*60*/	//Time in ticks filtering is allowed to run before giving up to UI.
#define COMP_EXTRA_LINES	11315	/**/	//# of extra lines to add to comp windows.
#define UNSPECIFIED_CHARSET	11316	/*iso-8859-1*/	//Name of charset to use if charset not specified.
#define MEM_EXPLANATION	11317	/*You may need to close some windows, move messages out of your In, Out, and Trash mailboxes, or increase Eudora�s memory size.*/	//
#define PROG_CONT_WIDTH	11318	/*330*/	//Width of progress information.
#define IMAP_PORT	11319	/*143*/	//Port used for IMAP4.
#define IMAP_MAIL_FOLDER_NAME	11320	/*IMAP Folder*/	//Name of the folder Eudora stores the IMAP cache in.  DO NOT LOCALIZE.
#define QUOTE_COLOR	11501	/*24000,24000,24000*/	//Color to use to display quoted text.
#define IMAP_THIS_MAILBOX_NAME	11502	/*This Mailbox<I*/	//Name of storage file inside IMAP cache mailbox.
#define IMAP_MAILBOX_LOCATION_PREFIX	11503	/**/	//This is the location prefix Eudora will use when locating your mailboxes on the IMAP server.  Ask your email administrator what to enter here if you're not sure, although most often you can leave it blank.
#define RECENT_DIR_LIMIT	11504	/*20*/	//Remember this many directory servers.
#define CONFIG_DIR_SERVER	11505	/*Configured server:\n*/	//
#define RECENT_DIR_SERVERS	11506	/*Servers used recently:\n*/	//
#define RECENT_DIR_FMT	11507	/*\t<%p>\n*/	//
#define PREVIEW_HI	11508	/*12*/	//Default preview height, in lines, for mailbox windows.
#define PREVIEW_USELESS	11509	/*3*/	//Number of lines below which the preview area will not be opened.
#define PREVIEW_READ_SECS	11510	/*5*/	//Number of seconds message must be in preview window before considered read.
#define WAZOO_TAB_HELP	11511	/*%p Window Tab\n\nClick on this tab to switch to the %p window. Click and drag the tab to reorder the tabs, to drag the tab to another tool window, or to create a new tool window.*/	//
#define MANGLE_ARGS	11512	/*(text=plain)*/	//
#define PREVIEW_STILL_TICKS	11513	/*15*/	//Number of ticks mouse must be still before preview.
#define IMAP_SPOOL_FMT	11514	/*IMAPSpool%x */	//Name of file we spool messages into.
#define IMAP_GETTING_MESSAGE_OFFLINE	11515	/*(This message can't be fetched from the remote server because you're offline.)*/	//
#define IMAP_ATTACH_FOLDER	11516	/*IMAP Attachments*/	//Name of the folder Eudora stores the IMAP attachments in.  DO NOT LOCALIZE.
#define IMAP_KERBEROS_SERVICE_FMT	11517	/*^0.^3@^2*/	//IMAP kerberos service name format, as determined by RFC 1731.  ^0=service, ^1=host.domain, ^2=realm, ^3=host
#define NAME_VAR	11518	/*$Name$*/	//Variable to expand into real name in Mailto url.
#define ADDR_VAR	11519	/*$Email$*/	//Variable to expand into email address in Mailto url.
#define AUTOSAVE_INTERVAL	11520	/*0*/	//Number of seconds before autosaving windows.
#define SPELL_COLOR	11701	/*56540,21600,44444*/	//Color for misspelled words.
#define SPELL_FACE	11702	/*4*/	//Style for misspelled words.
#define SPELL_DICTS	11703	/*Spelling Dictionaries*/	//Folder spelling dictionaries are located in.  DO NOT LOCALIZE.
#define SPELL_MEM_LIMIT	11704	/*0*/	//Maximum memory for any spelling dictionary.
#define SPELL_SUGGEST_DEPTH	11705	/*1*/	//Determines how deeply the spell-checker looks for suggestions.�� Range from -1 to 3.  0 is poorly but fast, 3 is hard but slow.  -1 lets the speller decide.
#define SPELL_WARNING	11706	/*Your message has misspellings.�You can send it anyway, or cancel and correct the misspellings.��Cancel-�Send Anyway�*/	//
#define SPELL_UDICT	11707	/* User Dictionary*/	//Filename of user dictionary.  DO NOT LOCALIZE
#define SPELL_UADICT	11708	/* User Anti-Dictionary*/	//Filename of user anti-dictionary.  DO NOT LOCALIZE
#define SPELL_UDICT_CONTENTS	11709	/*#LID 1033 1 3\nEudora\nQUALCOMM\ne-mail\n*/	//Initial contents of user dictionary.
#define SPELL_UADICT_CONTENTS	11710	/*#LID 1033 3 0\n*/	//Initial contents of user anti-dictionary.
#define CMDKEY_CONFLICT	11711	/*Keystroke is already in use.�The keystroke you�ve chosen is already in use for �%p�.  Do you really want to reassign it?��Cancel-�Reassign�*/	//
#define QUOTE_BLEND	11712	/*50*/	//Percentage of quote color to mix in with text color; 100% means just use the quote color, 0% means just use the text color.
#define SEARCH_BUTTON	11713	/*Search*/	//Name of Search button.
#define SEARCH_SEARCHING	11714	/*Searching: */	//
#define MORE_CHOICES	11715	/*More*/	//
#define FEWER_CHOICES	11716	/*Fewer*/	//
#define REG_EXP_ERR	11717	/*Error in regular expression*/	//
#define TOO_MANY_PAREN_ERR	11718	/*too many ()*/	//
#define UNMATCHED_PAREN_ERR	11719	/*unmatched ()*/	//
#define EMPTY_OPERAND_ERR	11720	/**+ operand could be empty*/	//
#define NESTED_OPERAND_ERR	11901	/*nested *?+*/	//
#define INVALID_BRACKET_RANGE_ERR	11902	/*invalid [] range*/	//
#define UNMATCHED_BRACKET_ERR	11903	/*unmatched []*/	//
#define REPEAT_NOTHING_ERR	11904	/*?+* follows nothing*/	//
#define TRAILING_SLASH_ERR	11905	/*trailing \\*/	//
#define USER_VAR	11906	/*$User$*/	//Variable to expand into userid in mailto url.
#define IMAP_RESYNC_MAILBOX	11907	/*Resynchronizing mailbox %p.*/	//
#define IMAP_GETTING_MESSAGE	11908	/*(This message will be fetched from the remote server.)*/	//
#define STOP_BUTTON	11909	/*Stop*/	//Name of Stop button.
#define IMAP_TEMP_MAILBOX_FORMAT	11910	/*%p.%p%p*/	//
#define SEARCH_RESULTS	11911	/*Search results*/	//
#define SEARCH_HITS_SIZE	11912	/*6*/	//max # of characters to display in search window's hit display
#define IMAP_FETCHING_MESSAGE	11913	/*Fetching message from %p.*/	//
#define ERR_PERS_RESYNCING	11914	/*Error while resyncing mailbox for %p:*/	//
#define ERR_PERS_FETCHING	11915	/*Error while fetching message for %p:*/	//
#define FILEID_PAUSE_LEN	11916	/*20*/	//Length of time to pause for file ID workaround.  May need to lengthen for slower machines.
#define FILEID_AFFECTED_SYSVERSION	11917	/*2064*/	//System version this workaround applies to.  Decimal here, 8.1 is 2064.
#define IMPORT_MESSAGE_BY_HAND	11918	/*Eudora couldn't find any files to import.�Would you like to manually select an account to import?��No-�Yes�*/	//
#define IMAP_DELETING_MESSAGES	11919	/*Deleting message(s) from %p.*/	//
#define IMAP_MARKING_MESSAGES	11920	/*Marking message %d of %d.*/	//
#define IMAP_EXPUNGE_MAILBOX	12101	/*Removing deleted messages from %p.*/	//
#define ERR_PERS_DELETING	12102	/*Error while deleting message for %p:*/	//
#define ERR_PERS_TRANSFERRING	12103	/*Error while transferring message for %p:*/	//
#define IMAP_REMOVE_DELETED_MESSAGES_ITEXT	12104	/*Remove Deleted Messages*/	//
#define ERR_PERS_EXPUNGING	12105	/*Error while expunging mailbox for %p:*/	//
#define IMAP_TRANSFER_MESSAGES	12106	/*Transferring IMAP message(s).*/	//
#define IMAP_COPY_MESSAGES	12107	/*Copying IMAP message(s).*/	//
#define UNDELETE_ITEXT	12108	/*Undelete*/	//
#define ERR_PERS_UNDELETING	12109	/*Error while undeleting messages for %p:*/	//
#define IMAP_UNDELETING_MESSAGES	12110	/*Undeleting message(s) in %p.*/	//
#define SPEAK_SAMPLE_TEXT	12111	/*Eudora is my friend.*/	//
#define SPEAK_SENDER_PREFIX	12112	/*From */	//
#define SPEAK_DOT	12113	/* dot */	//
#define SPEAK_SENDERS_NAME	12114	/*the sender's name*/	//
#define SPEAK_SUBJECT	12115	/*the subject line*/	//
#define SPEAK_NOTHING	12116	/*nothing*/	//
#define SPEAK_USING	12117	/* using */	//
#define SPEAK_DEFAULT_VOICE	12118	/*the default voice*/	//
#define IMAP_DELETE_CACHE	12119	/*Really delete personality �%p�?  The deletion cannot be undone.�Doing so will cause the local IMAP mail cache to be deleted.��Cancel-�OK�*/	//
#define IMAP_TO_POP	12120	/*Really change personality �%p� to POP?�Doing so will cause the local IMAP mail cache to be deleted.��Cancel-�Convert to POP�*/	//
#define IMAP_INBOX_NAME	12301	/*Inbox*/	//The name of the Inbox on your IMAP server - don't touch
#define IMAP_TRANSFER_BUFFER_SIZE	12302	/*16384*/	//Size of buffer for inter-server message transfers.
#define SPELL_HOLD_OPEN_SECS	12303	/*30*/	//Number of seconds to hold the speller open between spelling operations.  Low numbers save memory but may hurt performance.
#define LONG_MODAL_IDLE_SECS	12304	/*60*/	//Number of seconds user must be idle before Eudora performs a lengthy modal operation.
#define IMAP_WAITING_FOR_DECODER	12305	/*Waiting to decode attachment ...*/	//
#define IMAP_ERR_OPERATION_FMT	12306	/*Could not %p.*/	//
#define CHOOSE_IMAP_TRASH_MAILBOX	12307	/*Please choose a mailbox for deleted messages.*/	//
#define IMAP_TRASH_SELECT	12308	/*Really use �%p� as your trash mailbox?��Empty Trash� will remove all messages from it.��Cancel-�OK�*/	//
#define IMAP_EMPTY_REMOTE_TRASH	12309	/*Empty All Trash Mailboxes*/	//
#define IMAP_EMPTY_LOCAL_TRASH	12310	/*Empty Local Trash Mailbox*/	//
#define IMAP_EMPTY_TRASH	12311	/*Empty Trash*/	//
#define ALIAS_REMOVE_FILE_ALERT	12312	/*Permanently remove the nickname file �%p�?�This operation cannot be undone.��Cancel-�Remove It�*/	//
#define MIN_MB_SORT_TICKS	12313	/*180*/	//Minimum number of ticks between mailbox sorts
#define CANT_RENAME_ERR	12314	/*Unable to rename*/	//
#define MIME_CONTENT_DISP_FILENAME	12315	/*filename*/	//Don't touch
#define MIME_MAC_TYPE	12316	/*x-mac-type*/	//Don't touch
#define MIME_MAC_CREATOR	12317	/*x-mac-creator*/	//Don't touch
#define NO_TRANSLATORS_WITH_SETTINGS	12318	/*No plug-ins with settings*/	//
#define ATTACH_DOCUMENT_NAV_TITLE	12319	/*Attach Document*/	//
#define ATTACH_DOCUMENT_ACTION_BUTTON	12320	/*Attach*/	//
#define ATTACH_DOCUMENT_INSTRUCTIONS	12501	/*Select the file you wish to attach to the message and click �Attach�*/	//
#define INSERT_DOCUMENT_NAV_TITLE	12502	/*Insert Graphic*/	//
#define INSERT_DOCUMENT_ACTION_BUTTON	12503	/*Insert*/	//
#define INSERT_DOCUMENT_INSTRUCTIONS	12504	/*Select the graphic you wish to insert into the message and click �Insert�*/	//
#define NAV_CHOOSE_FOLDER_MESSAGE	12505	/*Please select a folder in the list, then click �Choose� to use that folder.*/	//
#define NAV_CHOOSE_SETTINGS	12506	/*Please locate the Eudora Settings file you wish to use, then click �Choose�.*/	//
#define CHOOSE_MAILBOX_NAV_TITLE	12507	/*Choose a Mailbox*/	//
#define CHOOSE_WORD_SERVICE_NAV_TITLE	12508	/*Choose a Word Service*/	//
#define MIME_CONTENT_PREFIX	12509	/*Content-*/	//Don't touch
#define IMAP_FETCH_ATTACHMENT	12510	/*Fetching attachment:*/	//
#define IMAP_FETCH_ATTACHMENT_NAME	12511	/*Fetching %p from the server.*/	//
#define IMAP_SEARCHING_MAILBOX	12512	/*Searching mailbox %p.*/	//
#define NAV_COULD_NOT_LOAD_ERR	12513	/*Could not load the Navigation Services Library.  Try closing some windows or allocating more memory to Eudora.*/	//
#define NAV_GENERAL_ERR	12514	/*There was a problem using Navigation Services.  You may need to close some windows or allocate more memory to Eudora.*/	//
#define ERR_PERS_ATTACHMENT_FETCH	12515	/*Error while fetching attachment for %p:*/	//
#define ERR_PERS_SEARCH	12516	/*Error while searching mailbox for %p:*/	//
#define IMAP_DECODING_ATTACHMENT	12517	/*Decoding attachment ...*/	//
#define IMAP_SEARCHING_PERS	12518	/*Doing search for %p.*/	//
#define COMP_OUT_INTRO	12519	/**/	//String to put before Who field of message in Out mailbox.
#define COMP_IN_INTRO	12520	/**/	//String to put before Who field of outgoing message in place other than Out mailbox.
#define IMAP_SEARCH_LOCAL_WARN	12701	/*For IMAP messages, the match term(s) you have specified will search only the local message cache. Messages whose headers have not been downloaded will not be searched.�You can cancel or search local cache only for IMAP messages.��Cancel-�Search�*/	//
#define IMAP_STUB_ENCODING	12702	/*imap_stub*/	//
#define SEARCHING_SERVER	12703	/*Searching on server...*/	//
#define SQUARE_LEFT	12704	/*[*/	//Characters to allow on left side of subject annotations
#define SQUARE_RIGHT	12705	/*]*/	//Characters to allow on right side of subject annotations
#define MAIL2NEWS	12706	/**/	//This address will be added to the Bcc: field of replies when doing Newsgroups: processing.
#define FLOW_WRAP_THRESH	12707	/*80*/	//Lines shorter than this will not be wrapped when doing format=flowed.
#define FLOW_WRAP_SPOT	12708	/*70*/	//Number of chars at which to wrap messages when doing format=flowed.
#define IMAP_MAIN_CON_TIMEOUT	12709	/*60*/	//Number of seconds the main connection to the IMAP server must be idle before closing it.
#define IMAP_MAX_CONNECTIONS	12710	/*5*/	//Allow up to this number of simultaneous connections to an IMAP server.
#define IMAP_WAITING_FOR_CONNECTION	12711	/*Waiting for free connection to the server ...*/	//
#define SEARCH_FOLDER	12712	/*Search Folder*/	//DO NOT LOCALIZE
#define MAX_FIND_SELECTION	12713	/*80*/	//Maximum # of characters for Enter Selection or auto search string
#define IMAP_LOGGING_IN	12714	/*Logging into the IMAP server.*/	//
#define IMAP_SAFE_ATTACH_FOLDER	12715	/*IMAP Attachment Stubs*/	//
#define SEARCH_SEARCH	12716	/*Search*/	//
#define REG_CODE_FILE	12717	/*RegCode*/	//Name of registration code file - don't touch
#define URL_JUMP_COMMAND	12718	/*jump.cgi*/	//
#define TYPING_THRESH	12719	/*5*/	//Number of ticks after a keystroke to assume the user is still typing
#define REG_SITE	12720	/*http://jump.eudora.com*/	//Don't touch.
#define TYPING_RECENTLY_THRESH	12901	/*30*/	//Number of ticks after a keystroke to assume the user has typed recently
#define VCARD_FOLDER	12902	/*Eudora Business Cards */	//Name of the folder that will contains your personal vCards - don't touch
#define USA_VCARD_FOLDER	12903	/*Eudora Business Cards*/	//Localize this to the name the user should see for Eudora Business Cards.
#define USA_MISPLACED_FOLDER	12904	/*Misplaced Items*/	//Localize this to the name the user should see for Misplaced Items.
#define REG_PLATFORM	12905	/*MacOS*/	//
#define REG_PRODUCT	12906	/*Eudora*/	//
#define REG_VERSION_FORMAT	12907	/*%d.%d.%d.%d*/	//
#define MISPLACED_FOLDER	12908	/*Misplaced Items */	//Name of the folder into which we dump files we find in the wrong place - don't touch
#define SASL_DONT	12909	/**/	//SASL mechanisms to never use.
#define GET_GRAPHICS_HELP	12910	/*Click to download images.*/	//
#define NO_GET_GRAPHICS_HELP	12911	/*Click to download images. Disabled because images have already been downloaded. */	//
#define SPEAK_NEXT_MESSAGE	12912	/*Next message.*/	//Phrase to be spoken at between messages when speaking messages from a mailbox.
#define SPEAK_QUOTE	12913	/*Quote*/	//Phrase to be spoken at the beginning of a quoted passage.
#define SPEAK_UNQUOTE	12914	/*Unquote*/	//Phrase to be spoken at the end of a quoted passage.
#define SPEAK_SILENCE_COMMAND	12915	/*[[slnc %r]]*/	//Don't touch!  Command sent to the Speech Manager to generate a pause.
#define SPEAK_MESSAGE_PAUSE_DURATION	12916	/*1000*/	//Duration to pause between messages when speaking (expressed in milliseconds)
#define SPEAK_QUOTE_MODIFY_VOICE_COMMAND	12917	/*[[pbas +4]]*/	//Command sent to the Speech Manager to modify the speaking voice for quoted passages
#define SPEAK_UNQUOTE_MODIFY_VOICE_COMMAND	12918	/*[[pbas -4]]*/	//Command sent to the Speech Manager to modify the speaking voice when unquoting a passage
#define PASSIVE_REPLY_TO_ASTR	12919	/*�Replying to: %p����OK�*/	//
#define SPEAK_TO_PREFIX	12920	/*To */	//
#define SPEAK_WITH	13101	/*with*/	//
#define OT_PPP_RACE_HACK	13102	/*0*/	//Number of seconds to delay after hearing PPP is connected before doing network operations.
#define OT_PPP_SMART_ASS	13103	/*Now pausing %p seconds for ARA 3.x bug ...*/	//
#define SPEAK_QUOTE_PAUSE_DURATION	13104	/*400*/	//Duration to pause when introducing quoted text (expressed in milliseconds)
#define SPEAK_DICTIONARY	13105	/*Pronunciation Dictionary*/	//Name of the default Speech Pronunciation file
#define SPEAK_DICTIONARY_BAD	13106	/*Your Speech Dictionary file appears to be damaged.*/	//
#define HTML_NOSPACESTYLE1	13107	/*blockquote, dl, ul, ol, li { margin-top: 0 ; margin-bottom: 0 }*/	//
#define HTML_NOSPACESTYLE2	13108	/*blockquote, dl, ul, ol, li { padding-top: 0 ; padding-bottom: 0 }*/	//
#define IMAP_COMPACT_HELP	13109	/*Number of messages / Combined size of all messages / Space wasted in the local cache.\n\nClick to remove deleted messages from this mailbox.*/	//
#define ERR_MULT_RESYNCING	13110	/*Error while resyncing IMAP mailboxes:*/	//
#define IMAP_CHECK_SPECIAL_ITEXT	13111	/*Resync current mailbox ...*/	//
#define IMAP_BAD_HEXBIN_ERR_TEXT	13112	/*%p%p  Try holding down the shift key and choosing Message->Change->Server Options->Redownload Entire Message to refetch this entire message.*/	//
#define ALIAS_MERGE_AND_REMOVE_FILE_ALERT	13113	/*Merge the nicknames from �%p� into �%p�?  The nickname file �%p� will be permanently removed.�This operation cannot be undone.��Cancel-�OK�*/	//
#define NICK_CACHE_SIZE	13114	/*100*/	//Maximum number of addresses to cache as �recently used�
#define CACHE_ALIAS_FILE	13115	/*History List*/	//Name of nickname cache file--don't touch
#define USA_CACHE_ALIAS_FILE	13116	/*History List*/	//Localize this to the name the user should see for History List.
#define SPEAK_ITEXT	13117	/*Speak*/	//
#define SPEAK_SEL_ITEXT	13118	/*Speak Selection*/	//
#define ERR_IMAP_ALERT	13119	/*IMAP ALERT message for %p:*/	//
#define EXPORT_PICT_LIST	13120	/*GIFf,PNGf,JPEG*/	//List of graphic types to convert PICT HTML images to, in order of preference
#define IMAP_EMPTY_CURRENT_TRASH	13301	/*Empty Trash on Current Server*/	//
#define IMAP_EMPTY_PERS_TRASH	13302	/*Empty Trash for Selected IMAP Personalities*/	//
#define IMAP_EMPTY_TRASH_WARN	13303	/*This will permanently remove messages from the trash mailboxes for the selected accounts.  Do you really wish to do this?*/	//
#define AUDIT_FILE	13304	/*UsageStats*/	//Filename for usage stats.  DO NOT LOCALIZE
#define AUDIT_INTRO_FORMAT	13305	/*%d%d%d%d%d%d%d%d%d%d %d %d */	//Format for beginning of audit data
#define AD_FOLDER_NAME	13306	/*Ad Folder*/	//Name of the folder where ads are stored.  DO NOT LOCALIZE.
#define PAY_AND_REGISTER	13307	/*Payment & Registration...*/	//Menu title for Eudora registration in the ad version
#define AUDIT_RELAX	13308	/*This file describes how you use Eudora; we might ask you for it someday to help us understand our users better.  It will n e v e r contain any of your email or personal information, and it will n e v e r be sent anywhere without your permission.\n\n*/	//
#define ADWARE_VERSION_BUTTON_TITLE	13309	/*Sponsored Mode\n(free, with ads)*/	//
#define PAY_VERSION_BUTTON_TITLE	13310	/*Paid Mode\n(costs money, no ads)*/	//
#define FREE_VERSION_BUTTON_TITLE	13311	/*Light Mode\n(free, fewer features)*/	//
#define REGISTER_BUTTON_TITLE	13312	/*Register with Us*/	//
#define CUSTOMIZE_ADS_BUTTON_TITLE	13313	/*Profile*/	//
#define UPDATES_BUTTON_TITLE	13314	/*Find the Latest Update to Eudora*/	//
#define CHANGE_REGISTRATION_BUTTON_TITLE	13315	/*Change your Code*/	//
#define NO_REG_NAME_TEXT	13316	/*<no registration name>*/	//
#define NO_REG_CODE_TEXT	13317	/*<no registration code>*/	//
#define CANT_AD	13318	/*QuickTime 3.0 or better is not installed.�QuickTime is necessary to display ads.  Would you like instructions about how to get QuickTime 3.0?  Alternately, you may switch to the free reduced-featured version of Eudora.��Reduce Features�Get QuickTime�*/	//
#define LINK_HISTORY_FILE	13319	/*Link History*/	//Name of link history file--don't touch
#define LINK_HISTORY_FOLDER	13320	/*Link History Folder*/	//Name of link history folder--don't touch
#define CREATING_LINK_HISTORY	13501	/*Couldn�t create your Link History file.*/	//
#define LINK_HISTORY_NEW_HISTORY_ERR	13502	/*There is insufficient memory to create a new history entry.*/	//
#define OPEN_LINK_HISTORY	13503	/*Error opening the link history file.*/	//
#define SAVE_LINK_HISTORY	13504	/*Couldn�t save the link history file.*/	//
#define LINK_HISTORY_GET_DATA_ERR	13505	/*There is insufficient memory to manipulate current nickname.*/	//
#define LINK_CMD	13506	/*link*/	//DON'T TOUCH.
#define LINK_AGE	13507	/*14*/	//Number of days to keep unused link history entries around for.
#define MIME_FLOWED_ON	13508	/*<%r %p>*/	//
#define AUDIT_SEND_THRESH	13509	/*4000*/	//One in this many startups ask to send audit info.
#define AUDIT_SEND_ADDR	13510	/*eudusage@eudora.com*/	//Address to which usage stats are sent.
#define AUDIT_PLEASE_SEND	13511	/*Please send this message after you�ve reviewed it and found it non-threatening (there is a legend at the bottom of the file explaining what the items mean).  Thank you for your help.\n\n-- Steve Dorner & the rest of the Mac Eudora development team\n\n*/	//
#define AUDIT_NUKE_DAYS	13512	/*7*/	//Days after which to nuke the usage log and begin a new one.
#define PAY_MSGID_FMT	13513	/*<p%p@%p>*/	//Don't touch.
#define ADWARE_MSGID_FMT	13514	/*<a%p@%p>*/	//Don't touch.
#define FREEWARE_MSGID_FMT	13515	/*<f%p@%p>*/	//Don't touch.
#define LINK_HISTORY_UI_FLAGS	13516	/*0*/	//This setting governs which column the link history is sorted by.�� Add 65536 to turn off custom preview icons.
#define AUDIT_SUBJECT	13517	/*Eudora usage statistics*/	//
#define UPDATE_FILE	13518	/*Eudora Updates*/	//Filename containing information about Eudora Updates.  DO NOT LOCALIZE
#define UPDATE_SITE	13519	/*http://jump.eudora.com*/	//DO NOT LOCALIZE
#define TOO_MANY_MESSAGES	13520	/*Mailbox �%p� is full.�You cannot have more than 32,000 messages in a mailbox.���OK�*/	//
#define UPDATE_WINDOW_PROGRESS	13701	/*Checking for Eudora updates�*/	//String to be displayed in the update nag while waiting for the server.
#define DIST_ID	13702	/*Distributor ID*/	//Name of file containing distributor ID for identifying ad referals
#define SIG_INTRO	13703	/*-- \n*/	//Standard introducer string for signature files.
#define HTTP_ERR_FORMAT	13704	/*%p (Error code: %d)*/	//Error reporting for HTTP errors
#define REG_INVALID	13705	/*That registration code is either not valid for this version or is not in the correct format.�Please verify you have entered your name and code correctly. For further help, click the ? button below.���OK�*/	//
#define MIME_ISO_LATIN15	13706	/*iso-8859-15*/	//
#define MIME_WIN_1252	13707	/*windows-1252*/	//
#define PROFILE_FILE_NAME	13708	/*Demog.txt*/	//Name of file containing demographic profiling information
#define TECH_SUPPORT_SITE	13709	/*http://jump.eudora.com*/	//DO NOT LOCALIZE
#define PREF_OFFLINE_LINK_ACTION	13710	/*0*/	//This setting governs what happens when the user clicks on a link while offline: (0) Do nothing, (1) Connect Automatically, (3) Bookmark the link, or (4) Remind me later.
#define OFFLINE_LINK_OPEN_TIMEOUT	13711	/*2*/	//Timeout for pinging the ad server.
#define OFFLINE_LINK_NAG_TIME	13712	/*300*/	//Number of ticks user should be idle before nagging about remembered links.
#define DEBUG_PLAYLIST_URL	13713	/**/	//Alternate playlist URL. Debug mode only.
#define IMAP_SENT_FLAG	13714	/*\\Draft*/	//Flag to use for sent messages on the IMAP server.  Leave blank to not mark sent messages at all.
#define STOLEN_REG_FILE	13715	/*The attached registration file has been discarded*/	//Why was someone sending you a registration file anyway?
#define SWITCH_TO_SPONSORED	13716	/*Switch permanently to sponsored mode?�In order to receive a refund for your Eudora purchase, you should contact a Eudora representative.  If you have not done so, press Cancel now.��Cancel�Permanently Switch*/	//
#define IMAP_QUEUED_COMMANDS	13717	/*Performing queued commands for %p*/	//
#define LINK_INTERESTING_PROTO	13718	/*http,https*/	//Comma seperated list of link types that will be remembered in the Link History.r--don't touch
#define ISO_2022	13719	/*iso-2022*/	//DO NOT TOUCH
#define TCP_PREF_REUSE_INTERVAL	13720	/*1800*/	//Minimum number of ticks between reading the TCP/IP preference file.  Higher numbers give better performance, but a better chance of using the wrong connection method.
#define IMAP_SECONDARY_CON_TIMEOUT	13901	/*60*/	//Number of seconds secondary connections to the IMAP server must be idle before closing them.
#define IMAP_SALV_REPORT	13902	/*%d of the %d summar%* in the old table of contents used; %d message(s) will be refetched.*/	//
#define INIT_ADWIN_ERR	13903	/*Unable to create ad window.*/	//
#define BAD_CHARSET_ERR	13904	/*This message had characters which were illegal for its character set encoding.*/	//
#define UPDATE_CHECK_ERR	13905	/*The update server does not appear to be responding at this time.  Please try again later.*/	//
#define REG_FROM_FILE_DESC	13906	/*The registration information you have received from our web site is below. If you wish to register using this information, click OK. Otherwise, click Cancel and you may register later.*/	//
#define REG_FROM_BUTTON_DESC	13907	/*To complete your registration, please enter the name you registered under and your registration code below.*/	//
#define STATE_INVALID_AT_STARTUP	13908	/*Eudora seems to be confused about the current operating mode.�Since we can't figure out if you prefer to run in Sponsored, Paid or Light mode, we'll default to Sponsored. You'll be able to choose the right operating mode from Payment & Registration.���OK�*/	//
#define REG_INVALID_AT_STARTUP	13909	/*Your paid registration information is invalid.�For now, Eudora will start in Sponsored mode.  You'll need to revalidate your registration information once Eudora has finished launching.���OK�*/	//
#define REG_THANK_YOU_PROMPT	13910	/*Thank you for your registration!*/	//
#define REG_INVALID_PROMPT	13911	/*Your registration information is invalid*/	//
#define REG_FROM_INVALID_CODE	13912	/*Double check the information below, or click on �No Code� to go to Eudora's web site for more information.*/	//
#define PICT_HELP_HTML	13913	/*<html><body><img src=\"x-eudora-pictres:%d\"></body></html>*/	//
#define X_EUDORA	13914	/*x-eudora*/	//Don�t touch.
#define ABOUT_REG_W_OLD	13915	/*Registered To: %p, %p %r (%p)*/	//
#define ABOUT_REG	13916	/*Registered To: %p, %p %r*/	//
#define AUTHPLAIN_FMT	13917	/*%r\000%p\000%p*/	//Format string for SASL PLAIN mechanism.  DO NOT LOCALIZE.
#define ANAL_WHITE	13918	/**/	//Comma-separated list of words that should NOT trigger moodwatch
#define RECONSIDER_AUTH	13919	/*Authentication is required.�The SMTP server for %p wants you to authorize, but you have forbidden it, so the send will probably fail.  Do you want to allow authorization?�Try Anyway�Cancel-�Allow�*/	//
#define IMAP_CACHE_MESSAGE	13920	/*Refreshing IMAP Cache*/	//
#define IMAP_MAILBOX_LIST_FETCH_GENERAL	14101	/*Fetching list of mailboxes.*/	//
#define IMAP_CACHE_CREATE	14102	/*Creating mailbox %p.*/	//
#define IMAP_CACHE_CREATE_GENERAL	14103	/*Updating cache mailboxes ...*/	//
#define IMAP_SHORT_HEADER_FIELDS	14104	/*Date,Subject,From,To,X-Priority,Content-Type*/	//Headers fetched during an IMAP minimal header download.  DO NOT LOCALIZE.
#define FACETIME_ERR	14105	/*Insufficient ad facetime.*/	//
#define PRE_PROFILE_UPDATE_NOTE	14106	/*Updating your profile�You will now be taken to the Eudora web site to update your profile information.��Cancel-�Continue�*/	//
#define FROG_CHARS	14107	/*aeiouy���������������h*/	//Characters that make a proceeding apostrophe be considered a word break by the speller.
#define IMAP_PREPARE_MESSAGES	14108	/*Preparing local messages.*/	//
#define IMAP_PREPARE_FMT	14109	/*Encoding message %d of %d...*/	//
#define UPDATE_REGISTER_BUTTON_TITLE	14110	/*Update your Registration*/	//
#define ENTER_REGISTRATION_BUTTON_TITLE	14111	/*Enter your Code*/	//
#define STAT_CURRENT_COLOR	14112	/*58981,6553,6553*/	//Color used for current stats
#define STAT_PREVIOUS_COLOR	14113	/*13107,13107,65535*/	//Color used for previous stats
#define STAT_AVERAGE_COLOR	14114	/*65535,65535,32767*/	//Color used for average stats
#define STAT_CURRENT_TYPE	14115	/*2*/	//Graph type for current stats
#define STAT_PREVIOUS_TYPE	14116	/*1*/	//Graph type for previous stats
#define STAT_AVERAGE_TYPE	14117	/*1*/	//Graph type for average stats
#define MAX_ANAL_IDLE	14118	/*1800*/	//Close down text analyzer after this many ticks of unuse.
#define FLAME_DICTIONARY	14119	/*Flame Dictionary*/	//Name of flaming dictionary for MoodMail.
#define MAX_ANAL_BITE	14120	/*8192*/	//Max size chunk for text analysis.
#define ANAL_WARNING	14301	/*Your message may cause offense.�Your message to %p regarding �%p� %r��Cancel-�Send Anyway�*/	//
#define ANAL_WARNING_LEVEL	14302	/*3*/	//Eudora can warn you when you queue a message with a high flame reading.  These settings control how high a reading gets a warning.
#define TOC_INVERSION_MATRIX	14303	/*1 12 2 3 4 5 6 7 8 9 11 10*/	//This controls the order of the columns in a mailbox. There be dragons here.
#define PW_CHANGE_SERVER	14304	/**/	//If not empty, this server will be used in lieu of mail server for password changing.
#define ANAL_COMP_HELP	14305	/*This message %r.*/	//Template for help on mood level icon.
#define IMPORT_PROGRESS_TITLE	14306	/*Importing mail*/	//
#define IMPORT_MESSAGE_PROGRESS_SUBTITLE	14307	/*Importing mail from %p*/	//
#define IMPORT_ON_STARTUP	14308	/*Would you like to import settings and mail from another email application?���No-�Yes�*/	//
#define UNTITLED_ADDRESS_BOOK	14309	/*untitled address book*/	//
#define UNTITLED_NICKNAME	14310	/*untitled nickname*/	//
#define PERSONAL_ALIAS_FILE	14311	/*Personal Nicknames*/	//Name of nickname file containing your personal contact information--don't touch
#define IMPORT_COMPLETE	14312	/*Eudora has finished importing your data.�You may import other accounts or use the Import Mail command to do so later.���OK�*/	//
#define STATISTICS_FILE	14313	/*Eudora Statistics.xml*/	//Name of statistics file--don't touch
#define USA_STATISTICS_FILE	14314	/*Eudora Statistics.xml*/	//If Eudora Statistics isn't found, this filename will be looked for and renamed to Eudora Statistics.
#define SCRIPTS_FOLDER	14315	/*Scripts*/	//Folder scripts are located in.  DO NOT LOCALIZE.
#define IMAP_COMPACT_SHOW_NUM_HELP	14316	/*Number of messages selected / Total number of messages / Combined size of all messages / Space wasted.\n\nClick to remove deleted messages from this mailbox.*/	//
#define PROFILE_FAILURE	14317	/*Downgrading your Eudora to Light Mode�Because you have not profiled yourself, you may no longer use Eudora in Sponsored mode.  You will now be placed in Light Mode until you fill out a profile.�Profile��OK�*/	//
#define PROFILING_NOW	14318	/*Waiting to Profile�When you have finished filling out your profile on our web site, click OK.  Eudora will then contact our ad server to verify your profile.��Cancel-�OK�*/	//
#define LIGHT_IMPORT_COMPLETE	14319	/*Eudora has finished importing your data.�You will be unable to use the accounts or signatures you just imported in Light mode.  Would you like to switch to Sponsored mode now, or ignore the data you just imported?��Ignore-�Switch�*/	//
#define BETA_INACTIVE_FIELD	14320	/*That field can't be edited in this early beta version of Eudora.*/	//
#define UPDATE_PROFILE_BUTTON_TITLE	14501	/*Update your Profile*/	//
#define VIEW_BY_LABEL	14502	/*View By:*/	//
#define ALIAS_REMOVE_NICK_FILE_ERR	14503	/*The selected address book could not be removed.*/	//
#define NICKLIST_MISSING_FIELD_VALUE	14504	/*�*/	//String to appear in the nickname list when no value is set for the current sort key
#define NICKLIST_PAREN	14505	/*{}*/	//Characters used to parenthesizing nicknames when sorting the nickname list
#define ALIAS_DISPLAY_ERR	14506	/*A problem was encountered when attempting to display the nickname.  Eudora is probably running low on memory.*/	//
#define FORBIDDEN_SETTING	14507	/*This setting cannot be changed with an x-eudora-setting URL.*/	//
#define IMAP_SEARCH_CHUNK_SIZE	14508	/*500*/	//IMAP SEARCH commands are run on this many messags at once.  Set to zero to perform searches with one single command.
#define SOME_BOZO	14509	/*Unspecified*/	//What we call someone who puts an empty phrase on their email
#define FRANKLIN	14510	/*Franklin Antonio*/	//
#define PLUGIN_INFO	14511	/*X-Eudora-Plugin-Info*/	//
#define SUBJ_TRIM_STR	14512	/*[]*/	//If this string appears first in a �make subject� filter action, square-bracketed text will be removed from the text represented by &
#define RESET_STATS_NOW	14513	/*Reset Statistics*/	//
#define NICK_PRINT_MARG1	14514	/*Nickname1,1,4,1*/	//
#define NICK_PRINT_MARG2	14515	/*Nickname2,2,4,1*/	//
#define NICK_PRINT_MARG3	14516	/*Nickname3,3,6,1*/	//
#define NAME_IN_USE	14517	/*That name is already in use.*/	//
#define SPONSOR_AD_HELP	14518	/*Eudora co-branding spot.\n\n�%p�\n\nClick to view more information in web browser*/	//Help balloon for sponsor ad
#define CREATE	14519	/*Create*/	//
#define MAILBOX_LEVEL_WARNING	14520	/*A mailbox or mail folder at this level will not display in the Mailbox and Transfer menus due to limitations in MacOS.  Create it anyway?*/	//
#define THE_DAVE_AND_CHUCK_LOVE_CONNECTION	14701	/*ihate thebox*/	//DO NOT TOUCH
#define UPDATE_FAILED	14702	/*An error occurred trying to find the latest version.*/	//
#define GRACE_PERIOD_PAY_NOW_ALRT	14703	/*Thank you for choosing to continue your support of Eudora.�This version of Eudora will remain in Paid Mode for about an hour or so, but will then switch to Sponsored Mode until your purchase has been completed.���OK�*/	//
#define GRACE_PERIOD_SHOW_VERSIONS_ALRT	14704	/*Other versions of Eudora are available.�We'll take you to our web site to show you what's currently available, and where you can pay for this update.  This version will switch to Sponsored Mode in about an hour.���OK�*/	//
#define REBUILD_TOC_ALRT_2	14705	/*You should choose Rebuild.�The most likely reason for the toc being out of date is that your machine crashed during a mail transfer, and you might lose mail if you don�t hit �Rebuild�.�Cancel-�Use Old�Rebuild�*/	//
#define REBUILD_TOC_ALRT_3	14706	/*Please choose Rebuild.��Use Old� may cause you to lose mail.�Cancel-�Use Old�Rebuild�*/	//
#define IMPORT_INCOMPLETE	14707	/*An error occurred while importing your data.�Some of your data may not have been imported.���OK�*/	//
#define ABOUT_REG_WITH_MONTH	14708	/*Registered To: %p, %p %r (%d)*/	//
#define ANAL_DELAY_LEVEL	14709	/*2*/	//Messages whose flame levels are this high will be automatically delayed.
#define ANAL_DELAY_MINUTES	14710	/*10*/	//Amount to delay messages with high flame levels.
#define MOOD_H_FACE	14711	/*1*/	//Style to use for inflammatory phrases.
#define MOOD_H_COLOR	14712	/*65535,0,0*/	//Color to use for inflammatory phrases.
#define MOOD_FACE	14713	/*1*/	//Style to use for potentially inflammatory phrases.
#define MOOD_COLOR	14714	/*65535,0,0*/	//Color to use for potentially inflammatory phrases.
#define CHOOSE_PICTURE_TITLE	14715	/*Choose a Photo*/	//
#define CHOOSE_NICK_PICTURE_PROMPT	14716	/*Locate the photo to be displayed in your Address Book.*/	//
#define PHOTO_FOLDER	14717	/*Photo Album*/	//Name of the folder containing pictures for the people in your Address Book--don't touch
#define SELECT	14718	/*Select*/	//
#define NICK_PHOTO_COULD_NOT_SAVE	14719	/*Problem while saving nickname photo.�Eudora was not able to save the photo for %p because of an error (%d).���OK�*/	//
#define UNTITLED_CSV	14720	/*untitled.csv*/	//
#define NICK_SAVE_AS_TITLE	14901	/*Save Address Book*/	//
#define NICK_EXPORT_FAIL	14902	/*There was an error while exporting nicknames.  Use the export file with extreme caution.*/	//
#define NICK_EXPORT_COMMA	14903	/*,*/	//Value separation delimeter when exporting nicknames in CSV format
#define NICK_EXPORT_EOL	14904	/*\015*/	//End of line character when exporting nicknames in CSV format
#define NICK_COMMA_REPLACE_CHAR	14905	/*\004*/	//Character used to replace embedded comma's when exporting nicknames.
#define NICK_CR_REPLACE_CHAR	14906	/*\003*/	//Character used to replace embedded carriage returns when exporting nicknames.
#define SAVE_AS_ITEXT	14907	/*Save As...*/	//
#define SAVE_AS_SEL_ITEXT	14908	/*Save Selection As...*/	//
#define EXPORTING_NICKNAMES	14909	/*Exporting Nicknames...*/	//
#define EXPORTING_SUBTITLE	14910	/*Now exporting:*/	//
#define DUP_NICKNAME_WARNING	14911	/*That nickname already exists.�A nickname called �%p� already appears in another address book.  Duplicate nicknames can cause some confusion when expanding addresses.  Do you want to give this nickname a unique name?��No-�Yes�*/	//
#define NO_NS_LIB_WARNING	14912	/*Can't find the Network Setup Extension.�This extension is required for Eudora's Internet Dialup features to work properly.  Would you like me to enable it for you?��No-�Yes�*/	//
#define NS_LIB_ENABLED	14913	/*The Network Setup Extensionhas been enabled.�You may need to restart your computer for the Internet Dialup features to work properly.���OK�*/	//
#define NS_LIB_NOT_ENABLED	14914	/*Could not enable the Network Setup Extension.�You will have to use the Extensions Manager to enable the extension yourself, then restart your computer before the Internet Dialup features will work properly.���OK�*/	//
#define NICK_BUTTON_DRAG_TICKS	14915	/*60*/	//Number of ticks to observer before drags to toolbar nicknames popup a message direction menu.
#define PALM_SYNC_USERNAME	14916	/*No User*/	//Name of the Palm User to sync mail and address book information with.
#define URL_STRIP_CHARS	14917	/*\n*/	//Strip these characters from URLs
#define NICK_CONCAT_NEWLINE	14918	/*\n*/	//New line string inserted into nickname data when appending vCard data into existing nickname fields as additional lines of information.
#define NICK_CONCAT_SEPARATOR	14919	/* � */	//String inserted into nickname data as a separator when appending vCard data into existing nickname fields.
#define ALIAS_DO_NOT_SYNC	14920	/*Do not include this nickname when syncing*/	//Title for the �Do not sync with�� checkbox
#define SCRIPT_EXECUTION_ERR	32401	/*Script execution error.�Error in script �%p�:\n\n%p*/	//
#define POP_SSL_PORT	32402	/*995*/	//Port for SPOP (POP over SSL)
#define SMTP_SSL_PORT	32403	/*465*/	//Port for SSMTP (SMTP over SSL)
#define IMAP_SSL_PORT	32404	/*993*/	//Port for IMAPS (IMAP over SSL)
#define LDAP_SSL_PORT	32405	/*636*/	//Port for SSL-LDAP (LDAP over SSL)
#define SSL_ERR_STRING	32406	/*SSL negotiation failed.*/	//
#define PH_CONNECT_ERROR	32407	/*Error connecting to the Ph server.\n%d; %p\n*/	//
#define SETTINGS_PREFILL_FILE	32408	/*Settings Prefill*/	//Do not localize.
#define SETTINGS_PREFILL_PROCESSED	32409	/*Settings Prefill.processed*/	//Do not localize.
#define STAT_GRAPH_HEIGHT	32410	/*150*/	//Height of statistics graphs
#define STAT_GRAPH_WIDTH	32411	/*350*/	//Width of statistics graphs
#define STAT_LEGEND_WIDTH	32412	/*85*/	//Width of statistics graph legends
#define CERT_FOLDER	32413	/*Eudora SSL Certificates*/	//Name of folder for downloaded SSL certificates
#define COMMAND_KEY_CHECKMAIL_REPLACEMENT	32414	/*1N*/	//Command key replacement for Check Mail. 0-none, 1-shift, 2-option, 4-control, followed by the command key.
#define COMMAND_KEY_ATTACH_REPLACEMENT	32415	/*1A*/	//Command key replacement for Attach Document. 0-none, 1-shift, 2-option, 4-control, followed by the command key.
#define SCROLL_WHEEL_LINES	32416	/*3*/	//Scroll this many lines per scroll wheel click.
#define SOUND_SUFFICES	32417	/*.aiff�.aif*/	//File suffices (separated by bullets) that mean a file is a sound file
#define NO_OSX_PRINTER	32418	/*Couldn�t find your printer.  Please use Print Center to set up your printer.*/	//
#define USA_DOCUMENTS	32419	/*Documents*/	//Name of system Documents folder. Used to find classic Documents folder.
#define NAT_FMT	32420	/*[%i]*/	//If the machine is behind a NAT, use this to format its domain name.
#define KERBEROS_IMAP_SERVICE	32421	/*imap*/	//Name of kerberized IMAP service.
#define K5_POP_SERVICE	32422	/*pop*/	//Name of Kerberos V POP service.
#define K5_SMTP_SERVICE	32423	/*smtp*/	//Name of Kerberos V SMTP service.
#define K5_SERVICE_FMT	32424	/*^0@^1*/	//Kerberos service name format.  ^0=service, ^1=host.domain, ^2=realm, ^3=host
#define ERROR_KEYWORD	32425	/*qt*/	//DO NOT LOCALIZE
#define EXPLANATION_KEYWORD	32426	/*explanation*/	//DO NOT LOCALIZE
#define GSSAPI_LOG_FMT	32427	/*GSSAPI: �%p�*/	//DO NOT LOCALIZE
#define ANY_ALIAS_FILE	32428	/*�Any Address Book�*/	//String to filter to match any address book. Do not localize.
#define FOR_PERSONALITY	32429	/* for personality �%p�*/	//
#define NO_SUCH_PERSONALITY	32430	/*You don�t have a personality named �%p�.*/	//
#define SSL_CERT_PROMPT	32431	/* You connected to a server which returned this SSL certificate that is not in your keychain. Would you like to add it to your keychain?*/	//
#define PERS_MUST_HAVE_NAME	32432	/*Personality names cannot be empty.*/	//
#define THREAD_SUBFOLDER_ERR	32433	/*Temporary error receiving mail; couldn�t find %r (%d).  Mail may be delayed, but will show up eventually.*/	//
#define THREAD_DELIVER_CREATE_ERR	32434	/*Temporary error receiving mail; couldn�t create %p (%d).  Mail may be delayed, but will show up eventually.*/	//
#define THREAD_DELIVER_EXCHANGE_ERR	32435	/*Temporary error receiving mail; couldn�t exchange %p and %p (%d).  Mail may be delayed, but will eventually be delivered.*/	//
#define SSL_CERTDLG_HOSTNAMAE	32436	/*Hostname: */	//
#define SSL_CERTDLG_EXPIRES	32437	/*Expires: */	//
#define SSL_CERTDLG_FINGERPRINTS	32438	/*Fingerprints*/	//
#define SSL_CERTDLG_SHA1	32439	/*SHA1: */	//
#define SSL_CERTDLG_MD5	32440	/*MD5: */	//
#define SSL_CERTDLG_TYPE	32441	/*Type: */	//
#define SSL_CERTDLG_SERIAL	32442	/*Serial #: */	//
#define SSL_CERTDLG_BEFORE	32443	/*Not Valid Before: */	//
#define SSL_CERTDLG_AFTER	32444	/*Not Valid After:*/	//
#define SSL_CERTDLG_ISSUER	32445	/*Issuer*/	//
#define SSL_CERTDLG_SUBJECT	32446	/*Subject*/	//
#define SSL_CERTDLG_COUNTRY	32447	/*Country: */	//
#define SSL_CERTDLG_STATE	32448	/*State: */	//
#define SSL_CERTDLG_LOCALITY	32449	/*Locality: */	//
#define SSL_CERTDLG_ORGANIZATION	32450	/*Organization: */	//
#define SSL_CERTDLG_ORGUNIT	32451	/*Organization Unit: */	//
#define SSL_CERTDLG_CNAME	32452	/*Common Name: */	//
#define SSL_CERTDLG_EMAIL	32453	/*Email: */	//
#define FILE_QUIT_ITEM_STR	32454	/*Quit*/	//
#define APPLE_ABOUT_EUDORA_ITEM_STR	32455	/*About Eudora*/	//
#define SSL_VERSION_STD_PORT	32456	/*769*/	//SSL protocol version to use when connecting on standard port
#define SSL_VERSION_ALT_PORT	32457	/*0*/	//SSL protocol version to use when connecting on alternate port
#define THREAD_PUNT_FILTER_ERR	32458	/*Temporary error receiving mail; will not filter right now (%d).  Mail may be delayed, but will show up eventually.*/	//
#define USERNAME_PROMPT	32459	/*Username:*/	//IMail does POP3 AUTH PLAIN completely wrong, as LOGIN.  When we see this string, we know it's IMail, and we adjust.  Idiots.
#define IMAIL_DOES_PLAIN_WRONG	32460	/*Adjusting to broken IMail...*/	//
#define AT_LEAST_ONE_GRAPHIC_MISSING	32461	/*The message you are sending is missing at least one of its images.  It will be sent without the missing image(s).*/	//
#define PPP_REACHABLE_HOST	32462	/**/	//If the mailhost of the dominant personality isn't appropriate to check for your PPP connection under OS X, fill this in with a hostname to check.
#define PACKAGE_PLUGINS_FOLDER	32463	/*PlugIns*/	//Name of folder used by OS X finder for application plugins.
#define PACKAGE_MACOS_FOLDER	32464	/*MacOS*/	//Name of folder used by OS X finder for application itself.
#define POP3_AUTH_RESP_CODE	32465	/*[auth*/	//DO NOT LOCALIZE.
#define IMAP_TRASH_REUSE	32466	/*Use �%p� as your Trash mailbox for your %p personality?��Empty Trash� will remove all messages from it.��Cancel-�OK�*/	//
#define WARNING_SIGNS_YOU_MIGHT_HAVE_OUTLOOK	32467	/*MsoNormal,margin-bottom:.0*/	//Any of these (comma-separated) phrases, detected in an HTML style sheet, will make Eudora NOT put a blank line after a P directive.
#define JUNK	32468	/*Junk*/	//DO NOT LOCALIZE.
#define FILE_ALIAS_JUNK	32469	/*Junk*/	//Name of mailbox that holds junk mail, as presented to the user.
#define JUNK_MAILBOX_THRESHHOLD	32470	/*50*/	//Mail must score at least this much on the Junk-O-Meter to land in the Junk mailbox.
#define JUNK_MAILBOX_EMPTY_DAYS	32471	/*30*/	//Number of days mail stays in Junk mailbox before being moved to Trash
#define JUNK_MAILBOX_EMPTY_DEST	32472	/*Trash*/	//Name of mailbox where old junk mail goes to die.  Set to �-� to destroy in-place
#define JUNK_MAILBOX_EMPTY_THRESH	32473	/*0*/	//Junk score threshold for mail that will be automatically emptied from Junk mailbox.
#define JUNK_EMPTY_WARNING	32474	/*Trim your %r mailbox now?�%d messages in your %r mailbox are at least %r days old, and due for deletion.  Delete them?�Don�t Warn�No-�Trim�*/	//
#define JUNK_PREEXISTING_RENAME_NAME	32475	/*My Junk*/	//Name to rename preexisting Junk mailbox to
#define JUNK_PREEXISTING_WARNING	32476	/*You have a mailbox called �%r�.�Eudora�s junk mail feature uses a mailbox named �%r�.  Would you like Eudora to use it, rename it, or disable the junk mailbox feature?�Disable-�Rename�Use It�*/	//
#define JUNK_XFER_SCORE	32477	/*100*/	//Junk score given to mail when you transfer it to the Junk mailbox
#define JUNK_JUNK_SCORE	32478	/*100*/	//Junk score given to mail when you mark it as junk
#define JUNK_MIN_REASONABLE	32479	/*25*/	//Minimum score considered a reasonable junk mail threshold.
#define JUNK_UNREASONABLE_WARNING	32480	/*Junk threshold too low�We recommend you not set the %r mailbox threshold to less than %r, or legitimate mail may be kept in the %r mailbox.  Set it to %r?�Use Value�Cancel-�OK�*/	//
#define JUNK_CANT_ARCHIVE	32481	/*Error trimming %r.�The old mail in your %r mailbox can�t be trimmed, because the mailbox �%p� can�t be found.���Cancel�*/	//
#define UNREFERENCED_PART	32482	/*The following document was sent as an embedded object but not referenced by the email above:\n*/	//
#define WAZOO_TOPMARGIN	32483	/*32*/	//Height of the tab area in a tabbed window.
#define WAZOO_TABHEIGHT	32484	/*21*/	//Height of the tabs in a tabbed window.
#define OUTLOOK_CRAP_FIND	32485	/*RE:,FW:*/	//
#define OUTLOOK_CRAP_FIX	32486	/*Re:,Fwd:*/	//
#define LOG_FLUSHED	32487	/*Flushed: �%p�*/	//
#define FLUSH_TIMEOUT	32488	/*1*/	//How long to wait (in seconds) for error messages from Cyrus after POP3 STLS fails.
#define JUNK_TRIM_SOON	32489	/*7*/	//Number of days until trimming that is considered soon enough to show the �will be trimmed soon� icon.
#define JUNK_TRIM_INTERVAL	32490	/*1*/	//Number of days (minimum) between trims of the junk mailbox.
#define WHITELIST_ADDRBOOK	32491	/**/	//Name of the address book to put whitelisted addresses in; blank for Eudora Nicknames.
#define PROXY_INIT_FAILED	32492	/*Unable to initialize proxy list.*/	//
#define LONGTERM_IDLE	32493	/*60*/	//Number of seconds of user idleness before we might start time-consuming bg tasks.
#define GRAPHICS_CACHE_MAX	32494	/*5000*/	//Max size (in Kbytes) of graphics cache for downloaded graphics.
#define COULDNT_INIT_CONCON	32495	/*Failed to initialize the Content Concentrator.*/	//
#define CONCON_PREVIEW_PROFILE	32496	/*Compact*/	//Name of Content Concentrator profile to use for the preview pane when showing a single message.
#define CONCON_ELLIPSIS_TRAIL	32497	/* ...*/	//Text added to the end of a partially-snipped paragraph
#define CONCON_ELLIPSIS_CENTER	32498	/*...snip...*/	//Text added to show paragraphs have been snipped
#define CONCON_ELLIPSIS_LEAD	32499	/*... */	//Text added to the start of a partially-snipped paragraph
#define CONCON_MULTI_PREVIEW_PROFILE	32601	/*Terse*/	//Name of Content Concentrator profile to use for previewing multiple messages
#define CONCON_MULTI_MAX	32602	/*20*/	//Max # of messages to preview at one time.
#define CONCON_QUOTE_ON	32603	/*Original Message*/	//Strings that might mean an outlook-style quote is coming
#define CONCON_QUOTE_OFF	32604	/*End Original*/	//Strings that might mean an outlook-style quote is ending
#define CONCON_DEBUG_FMT	32605	/*ConConInit: %p*/	//
#define CHARSET_FLUX_FMT	32606	/*%p/%p*/	//
#define CONCON_MESSAGE_PROFILE	32607	/*None*/	//Content Concentrator profile to use for message windows
#define ENCODED_FLOWED_WRAP_SPOT	32608	/*50*/	//When doing both format=flowed and quoted-printable encoding, wrap lines at this value.
#define JUNK_PREEXISTING_FOLDER_WARNING	32609	/*You have a mail folder called �%r�.�Eudora�s junk mail feature uses a mailbox named �%r�.  Your folder will be renamed.���OK�*/	//
#define USE_CMD_J_FOR_JUNK	32610	/*What should cmd-J do?�You might use Eudora 6�s Junk and Not Junk commands often.  You can use cmd-J and opt-cmd-J for them, or use cmd-J for manual message filtering. Which do you prefer?��Filter-�Junk�*/	//
#define TRIMMING_JUNK	32611	/*Trimming �%r� mailbox...*/	//
#define PREVIEW_SINGLE_DELAY	32612	/*5*/	//# of ticks before acting on a single message preview
#define PREVIEW_MULTI_DELAY	32613	/*30*/	//# of ticks before acting on a multiple message preview
#define SMALL_SYS_FONT_NAME	32614	/**/	//Put a font name here to override the small system font
#define SMALL_SYS_FONT_SIZE	32615	/**/	//Put a size here to override the small system font size
#define WIN_GEN_WARNING_THRESH	32616	/*10*/	//# of windows scheduled to be opened that generate a warning
#define MULT_RESPOND_WARNING_THRESH	32617	/*50*/	//# of messages to which responding all at once will generate a warning
#define WIN_GEN_WARNING	32618	/*Really open that many windows?�You�re about to open %d windows.  Is this what you intend?��Cancel-�OK�*/	//
#define MULT_RESPOND_WARNING	32619	/*Really respond to all those messages?�You�re about to respond to %d messages all at once. Is this what you intend?��Cancel-�OK�*/	//
#define AMBIG_SUBJ_FMT	32620	/*%p, etc.*/	//Format string to use when replying to multiple messages with different subjects
#define CONCON_MULTI_REPLY_PROFILE	32621	/*Terse*/	//Concentrator profile to use when replying to multiple messages.
#define IMAP_JUNK_SELECT	32622	/*Use �%p� as your Junk mailbox for your %p personality?�Messages may be removed from it automatically in the future.��Cancel-�OK�*/	//
#define CHOOSE_IMAP_JUNK_MAILBOX	32623	/*Please choose a Junk mailbox.*/	//
#define EUDORA_EUDORA	32624	/*Eudora*/	//Do Not Touch
#define IMAP_DEFAULT_JUNK_SCORE	32625	/*100*/	//Score to assign unscored messages in your IMAP Junk mailbox.
#define TRANSFER_CONTEXT_FMT	32626	/*Transfer to %p*/	//
#define ITS_LOCKED_DUMMY	32627	/*Could not remove; one or more items are locked.*/	//
#define IMAP_MAILBOXTITLE_FMT	32628	/*%p (%p)*/	//Format for titles of IMAP mailbox windows.
#define IMAP_FLAGGED_LABEL	32629	/*15*/	//The label to assign to IMAP messages marked as FLAGGED on the server.  Set to 0 to ignore the FLAGGED state.
#define JUNK_JUNK_IS_BAD_TRIM_DEST	32630	/*Cannot trim �%r� mailbox to itself!�Please select another mailbox, or turn off junk trimming. How about �%r�?��Cancel-�Use Trash�*/	//
#define JUNK_PREFNOIMAP_WARNING	32631	/*SpamWatch has been disabled.�Your IMAP server does not support the UIDPLUS extension.  SpamWatch has been disabled for your %p personality until your server is upgraded to send COPYUID responses.���OK�*/	//
#define JUNK_PREFIMAPAVAIL_WARNING	32632	/*SpamWatch has been re-enabled.�Your IMAP server now appears to support the required server extensions.  SpamWatch has been turned back on for your %p personality.���OK�*/	//
#define ALIAS_TO_NOWHERE	32633	/*The document you selected is an alias, but the original is no longer available.*/	//
#define X_FOLDER_ITEMS	32634	/*X-Folder*/	//MIME type(s) used by Apple when sending folders; completely non-standard.
#define MIME_X_FOLDER	32635	/*x-folder*/	//MIME type to be used when sending folders; not standard, made up by apple.
#define HELP_BUG_MTEXT	32636	/*Report a Bug*/	//
#define HELP_SUGGEST_MTEXT	32637	/*Make a Suggestion*/	//
#define SUM_SEARCH_COPY_FMT	32638	/*%c\t%p\t%p\t%p\t%p\t%p\t%d\t%p\t%p\n*/	//Format for copying a summary; state, priority, attachments, label, who, date, K, mailbox, subject
#define DEFAULT_MAILER_Q	32639	/*Do you want to set Eudora to be your default mail handler?�Eudora can handle 'mailto:' links on web pages and other sources. We'll only ask just once. ��No�Yes�*/	//
#define SEP_LINE_GREY	32640	/*44000*/	//Greyness of horizontal rules
#define SEARCH_FOR_FMT	32641	/*Search %p for �%p�*/	//Format string for context-based search menu items
#define SEARCH_FOR_WEB	32642	/*Web*/	//
#define SEARCH_SITE	32643	/*http://jump.eudora.com*/	//Do not touch
#define SEARCH_NOTHING_FMT	32644	/*Search %r*/	//Format string for non-context search web items
#define SEARCH_TEXT_WARNING	32645	/*This will search the web.�Note: when you click this toolbar button with some text selected, Eudora will search the web for that text.�Don�t Warn�Cancel-�OK�*/	//
#define BUYING_BY_HTTP	32646	/*http://www.eudora.com/buying*/	//
#define BUYING_BY_MAIL	32647	/*mailto:buying@eudora.com*/	//
#define REGCODE_PLUS_MONTH	32648	/*%p (%r %d)*/	//
#define IMAP_POLLING_MAILBOXES	32649	/*Checking %p for new messages.*/	//
#define IMAP_POLL_MAILBOX	32650	/*Looking for new messages in %p.*/	//
#define CONCON_PROF_SINGLE	32651	/*Single Message*/	//
#define CONCON_PROF_MULTI	32652	/*Multiple Messages*/	//
#define CONCON_PROF_DEFAULT	32653	/*Default (%p)*/	//
#define CONCON_PREV_PROF_WIDTH	32654	/*96*/	//
#define CONCON_NONE	32655	/*None*/	//
#define OSXAB_FILE	32656	/*OS X Address Book*/	//Name of nickname file that contains OS X AddressBook contacts.
#define SPIN_LENGTH	32657	/*1*/	//Number of ticks to be hyper about i/o completing
#define MAX_CONTEXT_FILE_CHOICES	32658	/*20*/	//Maximum number of contextual filing menu items to show
#define FAKE_CONTENT_COLOR	32659	/*15000,15000,15000*/	//
#define SEARCH_FOR_EMAIL	32660	/*Eudora*/	//
#define RECENT_SEARCH_LIMIT	32661	/*10*/	//Remember this many searches in the toolbar search widget.
#define TB_FKEY_LABEL_WIDE	32662	/*20*/	//Width of FKey labels in toolbar
#define GROUP_SUBJ_MAX_TIME	32663	/*30*/	//Maximum number of days of difference between messages whose subjects should be grouped together when sorting a mailbox
#define JUNK_NICK_FMT	32664	/*%p*/	//Format string for nicknames made with the �Not Junk� command
#define HIST_NICK_FMT	32665	/*%p*/	//Format string for nicknames made in the history list
#define SETTINGS_PANEL_KEYWORD	32666	/*panel*/	//DO NOT TOUCH
#define LOCKED_FILE_PERSISTENCE	32667	/*10*/	//Number of seconds to wait for a locked file to unlock on open
#define POP_BEFORE_SMTP_AUTH_AGE_LIMIT	32668	/*60*/	//Reserved
#define CONCON_DIGEST_ITEMS	32669	/*digest*/	//Words that indicate a message might be a digest
#define ATTCONV_BAD_CHARS	32670	/*--*/	//Characters forbidden in attachment converted creator/type display
#define ATTCONV_REP_CHARS	32671	/*  */	//Characters to replace forbidden characters in attachment converted creator/type display
#define CONCON_PROFILE_MISSING	32672	/*Missing Content Concentrator Profile�Your settings indicate use of the �%r� profile for this operation, but that profile doesn�t exist.���OK�*/	//
#define SPEECH_VOLUME	32673	/*10*/	//Volume at which to speak text, from 0 (silent) to 10 (loudest)
#define IMAP_HIDDEN_TOC_NAME	32674	/*#&$*- Hidden*/	//Name of cache mailbox that stores hidden messages.
#define IMAP_UNDO_DELETE	32675	/*Undo Delete from %p*/	//
#define NAUGHTY_URL_TLDS	32676	/*.com.,.org.,.net.,.edu.*/	//
#define NAUGHTY_URL_ALERT	32677	/*Warning: The URL you are about to visit may be deceptive.  Visit it anyway?�%p��Cancel��Visit*/	//
#define EMOTICON_FOLDER	32678	/*Emoticons*/	//DO NOT LOCALIZE
#define IMAP_MISSING_TRASH	32679	/*Couldn't find a Trash mailbox for %p.�Would you like to disable the IMAP Trash mailbox?��OK-�Cancel�*/	//
#define IMAP_MISSING_JUNK	32680	/*Couldn't find a Junk mailbox for %p.�Would you like to disable SpamWatch for this IMAP personality?��OK-�Cancel�*/	//
#define UNKNOWN_CHARSET_NAME	32681	/*unknown*/	//DO NOT LOCALIZE
#define IMAP_AUTOEXPUNGE_THRESHOLD	32682	/*20*/	//Remove deleted messages when their number exceeds this percentage of the mailbox.
#define IMAP_AUTOEXPUNGE_WARNING	32683	/*Should Eudora do automatic EXPUNGE commands on your IMAP mailboxes?�If you don't know what an EXPUNGE command is, just click �Yes�.  For more information, click the �?� button.��No-�Yes�*/	//
#define RFC2184FMT	32684	/*%p*%d*/	//DO NOT LOCALIZE
#define MIN_LEFT_APPEND	32685	/*64*/	//# of chars to leave on left side when appending 2184 stuff
#define ELIDE_2184_STRING	32686	/*...*/	//String to put between two pieces of a filename longer than we handle
#define EMO_MENU_FMT	32687	/*%p��(%p)*/	//Format for emoticon menu items.
#define COUNTRY_DOMAINS	32688	/*au,br,tw,hk,uk*/	//TLD's that are countries that have SLD's in our list of naughty url TLD's
#define SEARCH_DF_AGE	32689	/*5*/	//Default value for age search criterion
#define SEARCH_DF_AGE_REL	32690	/*4*/	//Default relation for age search criterion
#define SEARCH_DF_AGE_SPFY	32691	/*3*/	//Default specifier for age search criterion
#define SEARCH_DF_ATT	32692	/*0*/	//Default number of attachments for attachment count search criterion
#define SEARCH_DF_ATT_REL	32693	/*3*/	//Default relation for attachment count search criterion
#define DISCARD_LOG_FMT	32694	/*Discarding: �%p�*/	//
#define REBUILDING_TOC	32695	/*Rebuilding table of contents for �%p�*/	//
#define URL_NOTNAUGHTY_PREFIXES	32696	/*www.*/	//Things to ignore at the start of hostnames in url�s for purposes of deceptive link handling
#define URL_NAUGHTY_EXCEPTIONS	32697	/*com.com*/	//Certain TLD's that break the ordinary rules of naughtiness
#define HTML_MIN_IMAGE_SIZE	32698	/*4*/	//Do not automatically download HTML images less than this many square pixels
#define HTML_BAD_IMAGE_DIMENSIONS	32699	/*1x1*/	//Do not automatically download HTML images of these dimensions
#define PDF_QUOTE_EXTENSION_UNQUOTE	4501	/*.pdf*/	//Since Avi is stuck in 1975, we can't use just filetype to determine if a file is a PDF.
#define ERR_PERS_FILTERING	4502	/*Error while filtering for %p:*/	//
#define IMAP_FILTERING_MESSAGES	4503	/*Filtering messages for %p.*/	//
#define OUTGOING_MID_LIST_SIZE	4504	/*200*/	//Number of outgoing messages to remember we sent.
#define ICHAT_BTN_TEXT	4505	/*iChat*/	//Text of chat button in address book.
#define AIM_PROTO	4506	/*aim*/	//DO NOT LOCALIZE
#define AIM_URL_FMT	4507	/*aim:goim?screenname=%p*/	//DO NOT LOCALIZE
#define OUTLOOK_FW_PREFIX	4508	/*FW:*/	//Outlook uses FW in contravention to accepted Internet norms
#define URL_HOST_CHECK_PROTOS	4509	/*http,https,ftp*/	//Protocols to look for hostname mismatches in.
#define ALTERNATE_EMOTICONS	4510	/*18,Emoticons24,24,Emoticons32*/	//
#define BADGE_ATTENTION_COLOR	4511	/*0,48000,0*/	//Color for dock icon badge when Eudora wants attention
#define BADGE_NORMAL_COLOR	4512	/*0,0,48000*/	//Color for dock icon badge when Eudora doesn't want attention
#define BADGE_TEXT_COLOR	4513	/*65000,65000,65000*/	//Color for text of dock icon badge
#define BADGE_PERS_LIST	4514	/***/	//List of IMAP personalities for whom the INBOX unread counts should be added to the dock icon badge, or * for all personalities.
#define EMO_BATCH_SIZE	4515	/*10*/	//Max # of emoticons to convert in one swell foop.
#define NOT_MOVIE_EXTENSIONS	4516	/*.rtf*/	//List of extenstion NOT to be considered movies, no matter what QuickTime thinks
#define SLASHY_SCHEMES	4517	/*http,https,ftp*/	//
#define TB_BUTTON_FONT_SIZE	4518	/*9*/	//Size of text on buttons in the main toolbar. Relaunch to take effect.
#define DEMO_DAYS_REMAINING_FMT	4519	/*You have %d days remaining of your Paid mode preview.*/	//
#define DEMO_LAST_DAY_FMT	4520	/*Today is the last day of your Paid mode preview.  Eudora will switch to Sponsored mode tomorrow!*/	//
#define PAYMODE_DEMO_BTN_TITLE_FMT	4521	/*%p\n%d days left*/	//
#define CHECK_CONNECTION_SUCCESS	4522	/*Check completed successfully*/	//
#define CHECK_CONNECTION_FORMAT	4523	/*Host: �%p�   Port: %d*/	//
#define EXPORT_FOLDER_NAME	4524	/*Eudora Export*/	//Name of parent folder into which Eudora will export its mail, address book, etc.
#define EXPORT_MAIL_ERR	4525	/*Error exporting mail*/	//
#define EXPORT_MAIL_FOLDER	4526	/*Mail Folder*/	//Name of subfolder into which to export mail
#define EXPORTING_MAIL	4527	/*Exporting Mail...*/	//
#define MOZILLA_FLAGS_FMT	4528	/*X-Mozilla-Status: %4x%p*/	//Names of mozilla headers that hold (some) flags for export.  DO NOT LOCALIZE
#define MOZILLA_FLAGS2_FMT	4529	/*X-Mozilla-Status2: %8x%p*/	//Names of mozilla headers that hold (some) flags for export.  DO NOT LOCALIZE
#define MOZILLA_KEYWORD_HDR	4530	/*X-Mozilla-Keys: */	//Name of mozilla header that holds keywords (tags).  DO NOT LOCALIZE


#endif
