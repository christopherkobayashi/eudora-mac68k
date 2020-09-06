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

#ifndef OSXABSYNCINC_H
#define OSXABSYNCINC_H

#if	TARGET_RT_MAC_MACHO
#include <Addressbook/Addressbook.h>
#else

/* Taken from /System/Library/Frameworks/AddressBook.framework/Versions/A/Headers/ABAddressBook.h  */

/////////////////////////////////////////////////////////////////////////
//
//	OS X Address Book Declarations
//
/////////////////////////////////////////////////////////////////////////

// ================================================================
//      Property Type
// ================================================================

#define kABMultiValueMask        0x100

typedef enum _ABPropertyType {
    kABErrorInProperty           = 0x0,
    kABStringProperty            = 0x1,
    kABIntegerProperty           = 0x2,
    kABRealProperty              = 0x3,
    kABDateProperty              = 0x4,
    kABArrayProperty             = 0x5,
    kABDictionaryProperty        = 0x6,
    kABDataProperty              = 0x7,
    kABMultiStringProperty       = kABMultiValueMask | kABStringProperty,
    kABMultiIntegerProperty      = kABMultiValueMask | kABIntegerProperty,
    kABMultiRealProperty         = kABMultiValueMask | kABRealProperty,
    kABMultiDateProperty         = kABMultiValueMask | kABDateProperty,
    kABMultiArrayProperty        = kABMultiValueMask | kABArrayProperty,
    kABMultiDictionaryProperty   = kABMultiValueMask | kABDictionaryProperty,
    kABMultiDataProperty         = kABMultiValueMask | kABDataProperty
} ABPropertyType;

// ================================================================
//      Search APIs
// ================================================================

typedef enum _ABSearchComparison {
        kABEqual,
        kABNotEqual,
        kABLessThan,
        kABLessThanOrEqual,
        kABGreaterThan,
        kABGreaterThanOrEqual,

        kABEqualCaseInsensitive,
        kABContainsSubString,
        kABContainsSubStringCaseInsensitive,
        kABPrefixMatch,
        kABPrefixMatchCaseInsensitive
} ABSearchComparison;

typedef enum _ABSearchConjunction {
        kABSearchAnd,
        kABSearchOr
} ABSearchConjunction;

/*

*/

typedef void                    	*ABRecordRef;
typedef struct __ABPerson       	*ABPersonRef;
typedef struct __ABGroup        	*ABGroupRef;
typedef struct __ABSearchElementRef	*ABSearchElementRef;
typedef struct __ABAddressBookRef	*ABAddressBookRef;
typedef const struct __ABMultiValue	*ABMultiValueRef;
typedef struct __ABMultiValue		*ABMutableMultiValueRef;

typedef void (*ABImageClientCallback) (CFDataRef imageData, int tag, void* refcon);

// ================================================================
//      Global Table properties
// ================================================================

// ----- Properties common to all Records

extern const char kABUIDProperty[];                   // The UID property
extern const char kABCreationDateProperty[];          // Creation Date (when first saved) (date)
extern const char kABModificationDateProperty[];      // Last saved date (date)

// ----- Person specific properties

extern const char kABFirstNameProperty[];             // First name (string)
extern const char kABLastNameProperty[];              // Last name  (string)

extern const char kABFirstNamePhoneticProperty[];     // First name Phonetic (string)
extern const char kABLastNamePhoneticProperty[];      // Last name Phonetic (string)

extern const char kABBirthdayProperty[];              // Birth date  (date)

extern const char kABOrganizationProperty[];          // Company name  (string)

extern const char kABJobTitleProperty[];              // Job Title  (string)

extern const char kABHomePageProperty[];              // Home Web page  (string)

extern const char kABEmailProperty[];                 // Email(s) (multi-string)
        extern const char kABEmailWorkLabel[];        // Work email
        extern const char kABEmailHomeLabel[];        // Home email

extern const char kABAddressProperty[];                // Street Addresses (multi-dictionary)
    extern const char kABAddressStreetKey[];           // Street
    extern const char kABAddressCityKey[];             // City
    extern const char kABAddressStateKey[];            // State
    extern const char kABAddressZIPKey[];              // Zip
    extern const char kABAddressCountryKey[];          // Country
    extern const char kABAddressCountryCodeKey[];      // Country Code
        extern const char kABAddressHomeLabel[];       // Home Address
        extern const char kABAddressWorkLabel[];       // Work Address

/*
 * kABAddressCountryCodeKey code must be one of the following:
 * iso country codes
 *
 *    at = Austria
 *    au = Australia
 *    be = Belgium
 *    ca = Canada
 *    ch = Switzerland
 *    cn = China
 *    de = Germany
 *    dk = Denmark
 *    es = Spain
 *    fi = Finland
 *    fr = France
 *    gl = Greenland
 *    ie = Ireland
 *    il = Israel
 *    id = Indonesia
 *    is = Iceland
 *    it = Italy
 *    ja = Japan
 *    kr = South Korea
 *    lu = Luxembourg
 *    mx = Mexico
 *    nl = Netherlands
 *    no = Norway
 *    nz = New Zealand
 *    pl = Poland
 *    pt = Portugal
 *    se = Sweden
 *    sg = Singapore
 *    tr = Turkey
 *    tw = Taiwan
 *    uk = United Kingdom
 *    us = United States
 *    za = South Africa
 *
 */

extern const char kABPhoneProperty[];                  // Generic phone number (multi-string)
        extern const char kABPhoneWorkLabel[];         // Work phone
        extern const char kABPhoneHomeLabel[];         // Home phone
        extern const char kABPhoneMobileLabel[];       // Cell phone
        extern const char kABPhoneMainLabel[];         // Main phone
        extern const char kABPhoneHomeFAXLabel[];      // FAX number
        extern const char kABPhoneWorkFAXLabel[];      // FAX number
        extern const char kABPhonePagerLabel[];        // Pager number

extern const char kABAIMInstantProperty[];             // AIM Instant Messaging (multi-string)
        extern const char kABAIMWorkLabel[];
        extern const char kABAIMHomeLabel[];

extern const char kABJabberInstantProperty[];          // Jabber Instant Messaging (multi-string)
        extern const char kABJabberWorkLabel[];
        extern const char kABJabberHomeLabel[];

extern const char kABMSNInstantProperty[];             // MSN Instant Messaging  (multi-string)
        extern const char kABMSNWorkLabel[];
        extern const char kABMSNHomeLabel[];

extern const char kABYahooInstantProperty[];           // Yahoo Instant Messaging  (multi-string)
        extern const char kABYahooWorkLabel[];
        extern const char kABYahooHomeLabel[];

extern const char kABICQInstantProperty[];             // ICQ Instant Messaging  (multi-string)
        extern const char kABICQWorkLabel[];
        extern const char kABICQHomeLabel[];

extern const char kABNoteProperty[];                   // Note (string)

// ----- Person Properties not Currently supported in the AddressBook UI
extern const char kABMiddleNameProperty[];		// string
extern const char kABMiddleNamePhoneticProperty[];	// string
extern const char kABTitleProperty[];			// string "Sir" "Duke" "General" "Lord"
extern const char kABSuffixProperty[];                	// string "Sr." "Jr." "III"
extern const char kABNicknameProperty[];                	// string
extern const char kABMaidenNameProperty[];		// string

// ----- Group Specific Properties

extern const char kABGroupNameProperty[];              // Name of the group

// ================================================================
//      Generic Labels
// ================================================================

    // All kABXXXXWorkLabel are equivalent to this label
extern const char kABWorkLabel[];

    // All kABXXXXHomeLabel are equivalent to this label
extern const char kABHomeLabel[];

    // Can be use with any Mutli-value property
extern const char kABOtherLabel[];

// ================================================================
//      RecordTypes
// ================================================================

    // Type of a ABPersonRef
extern const char kABPersonRecordType[];

    // Type of a ABGroupRef
extern const char kABGroupRecordType[];

// ================================================================
//      Notifications published when something changes
// ================================================================

    // This process has changed the DB
extern const char kABDatabaseChangedNotification[];

    // Another process has changed the DB
extern const char kABDatabaseChangedExternallyNotification[];


const char kABUIDProperty[] = "UID";
const char kABCreationDateProperty[] = "Creation";
const char kABModificationDateProperty[] = "Modification";
const char kABFirstNameProperty[] = "First";
const char kABLastNameProperty[] = "Last";
const char kABFirstNamePhoneticProperty[] = "FirstPhonetic";
const char kABLastNamePhoneticProperty[] = "LastPhonetic";
const char kABBirthdayProperty[] = "Birthday";
const char kABOrganizationProperty[] = "Organization";
const char kABJobTitleProperty[] = "JobTitle";
const char kABHomePageProperty[] = "HomePage";
const char kABEmailProperty[] = "Email";
const char kABAddressProperty[] = "Address";
const char kABAddressStreetKey[] = "Street";
const char kABAddressCityKey[] = "City";
const char kABAddressStateKey[] = "State";
const char kABAddressZIPKey[] = "ZIP";
const char kABAddressCountryKey[] = "Country";
const char kABAddressCountryCodeKey[] = "CountryCode";
const char kABPhoneProperty[] = "Phone";
const char kABPhoneMobileLabel[] = "_$!<Mobile>!$_";
const char kABPhoneMainLabel[] = "_$!<Main>!$_";
const char kABPhoneHomeFAXLabel[] = "_$!<HomeFAX>!$_";
const char kABPhoneWorkFAXLabel[] = "_$!<WorkFAX>!$_";
const char kABPhonePagerLabel[] = "_$!<Pager>!$_";
const char kABAIMInstantProperty[] = "AIMInstant";
const char kABJabberInstantProperty[] = "JabberInstant";
const char kABMSNInstantProperty[] = "MSNInstant";
const char kABYahooInstantProperty[] = "YahooInstant";
const char kABICQInstantProperty[] = "ICQInstant";
const char kABNoteProperty[] = "Note";
const char kABMiddleNameProperty[] = "Middle";
const char kABMiddleNamePhoneticProperty[] = "MiddlePhonetic";
const char kABTitleProperty[] = "Title";
const char kABSuffixProperty[] = "Suffix";
const char kABNicknameProperty[] = "Nickname";
const char kABMaidenNameProperty[] = "MaidenName";
const char kABGroupNameProperty[] = "GroupName";
const char kABOtherLabel[] = "_$!<Other>!$_";
const char kABPersonRecordType[] = "ABPerson";
const char kABGroupRecordType[] = "ABGroup";
const char kABDatabaseChangedNotification[] = "ABDatabaseChangedNotification"; 
const char kABDatabaseChangedExternallyNotification[] = "ABDatabaseChangedExternallyNotification"; 
	
const char kABWorkLabel[] = "_$!<Work>!$_";
const char kABHomeLabel[] = "_$!<Home>!$_";
	
const char kABEmailWorkLabel[] = "_$!<Work>!$_";
const char kABAddressWorkLabel[] =  "_$!<Work>!$_";
const char kABPhoneWorkLabel[] =  "_$!<Work>!$_";
const char kABAIMWorkLabel[] =  "_$!<Work>!$_";
const char kABJabberWorkLabel[] =  "_$!<Work>!$_";
const char kABMSNWorkLabel[] =  "_$!<Work>!$_";
const char kABICQWorkLabel[] =  "_$!<Work>!$_";
const char kABYahooWorkLabel[] = "_$!<Work>!$_";
	
const char kABEmailHomeLabel[] = "_$!<Home>!$_";
const char kABAddressHomeLabel[] = "_$!<Home>!$_";
const char kABPhoneHomeLabel[] = "_$!<Home>!$_";
const char kABAIMHomeLabel[] = "_$!<Home>!$_";
const char kABJabberHomeLabel[] = "_$!<Home>!$_";
const char kABMSNHomeLabel[] = "_$!<Home>!$_";
const char kABICQHomeLabel[] = "_$!<Home>!$_";
const char kABYahooHomeLabel[] = "_$!<Home>!$_";


/* Taken from /System/Library/Frameworks/CoreFoundation.framework/Versions/A/Headers/CFNotificationCenter.h */

/////////////////////////////////////////////////////////////////////////
//
//	OS X CFNotificationCenter Declarations
//
/////////////////////////////////////////////////////////////////////////


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct __CFNotificationCenter * CFNotificationCenterRef;

typedef void (*CFNotificationCallback)(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo);

typedef enum {
    CFNotificationSuspensionBehaviorDrop = 1,
        // The server will not queue any notifications with this name and object while the process/app is in the background.
    CFNotificationSuspensionBehaviorCoalesce = 2,
        // The server will only queue the last notification of the specified name and object; earlier notifications are dropped. 
    CFNotificationSuspensionBehaviorHold = 3,
        // The server will hold all matching notifications until the queue has been filled (queue size determined by the server) at which point the server may flush queued notifications.
    CFNotificationSuspensionBehaviorDeliverImmediately = 4
        // The server will deliver notifications matching this registration whether or not the process is in the background.  When a notification with this suspension behavior is matched, it has the effect of first flushing any queued notifications.
} CFNotificationSuspensionBehavior;

#if defined(__cplusplus)
}
#endif

#endif


#endif	//OSXABSYNCINC_H