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

 /* ======================================================================

    Note: This is old stuff for EMSAPI v1.0
    
    Eudora Extended Message Services API SDK 1.0b3 (May 30 1996)
    This SDK supports API version 1
    Copyright 1995, 1996 QUALCOMM Inc.
    Laurence Lundblade <lgl@qualcomm.com>

    Generated: Thu May 30 16:02:56 PDT 1996
    Filename: tlapi-MacGlue.h

    THIS IS TO BUILD THE EUDORA APPLICATION SIDE OF THE API. DON'T
    USE THIS TO BUILD A TRANSLATOR OR COMPONENT

    Note: this file is generated automatically by scripts and must be
    kept in synch with other translation API definitions, so it should
    probably not ever be changed.

 */
#ifndef TLAPI_MACGLUE_H_INCLUDED
#define TLAPI_MACGLUE_H_INCLUDED

/* ========== CONSTANTS AND RETURN VALUES ================================ */

/* ----- Translator return codes --- store as a long --------------------- */ 
#define TLR_OK              (0L)     /* The translation operation succeeded */
#define TLR_UNKNOWN_FAIL    (1L)     /* Failed for unspecified reason */
#define TLR_CANT_TRANS      (2L)     /* Don't know how to translate this */
#define TLR_INVALID_TRANS   (3L)     /* The translator ID given was invalid */
#define TLR_NO_ENTRY        (4L)     /* The value requested doesn't exist */
#define TLR_NO_INPUT_FILE   (5L)     /* Couldn't find input file */
#define TLR_CANT_CREATE     (6L)     /* Couldn't create the output file */
#define TLR_TRANS_FAILED    (7L)     /* The translation failed. */
#define TLR_INVALID         (8L)     /* Invalid argument(s) given */
#define TLR_NOT_NOW         (9L)     /* Translation can be done not in current
                                        context */
#define TLR_NOW            (10L)     /* Indicates translation can be performed
                                        right away */
#define TLR_ABORTED        (11L)     /* Translation was aborted by user */
 
 
/* ----- Translator types --- store as a long ---------------------------- */ 
#define TLT_NO_TYPE          (-1L)
#define TLT_LANGUAGE         (0x10L)
#define TLT_TEXT_FORMAT      (0x20L)
#define TLT_GRAPHIC_FORMAT   (0x30L)
#define TLT_COMPRESSION      (0x40L)
#define TLT_COALESCED        (0x50L)
#define TLT_SIGNATURE        (0x60L)
#define TLT_PREPROCESS       (0x70L)
#define TLT_CERT_MANAGEMENT  (0x80L)


/* ----- Translator info flags and contexts --- store as a long ---------- */
/* Used both as bit flags and as constants */
#define TLF_ON_ARRIVAL      (0x0001L) /* Call on message arrivial */
#define TLF_ON_DISPLAY      (0x0002L) /* Call when user views message */
#define TLF_ON_REQUEST      (0x0004L) /* Call when selected from menu */
#define TLF_Q4_COMPLETION   (0x0008L) /* Queue and call on complete 
                                         composition of a message */
#define TLF_Q4_TRANSMISSION (0x0010L) /* Queue and call on transmission
                                         of a message */
#define TLF_WHOLE_MESSAGE   (0x0200L) /* Works on the whole message even if
                                         it has sub-parts. (e.g. signature) */
#define TLF_REQUIRES_MIME   (0x0400L) /* Items presented for translation
                                         should be MIME entities with
                                         canonical end of line representation,
                                         proper transfer encoding
                                         and headers */
#define TLF_GENERATES_MIME  (0x0800L) /* Data produced will be MIME format */
/* all other flag bits in the long are RESERVED and may not be used */


/* ----- The version of the API defined by this include file ------------- */
#define TL_VERSION          (1)       /* Used in module init */
#define TL_COMPONENT        'EuTL'    /* Macintosh component type */


/* ----- Translator and translator type specific return codes ------------ */
#define TLC_SIGOK           (1L)      /* A signature verification succeeded */
#define TLC_SIGBAD          (2L)      /* A signature verification failed */
#define TLC_SIGUNKNOWN      (3L)      /* Result of verification unknown */

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_STRUCT_ALIGN
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

/* ========== DATA STRUCTURES ============================================ */

/* True Mac-style declarations aren't used yet but are included in comments. 
   All strings on the Mac are PASCAL strings and all are allocated as Handles. 
 */

/* ----- Macintosh: MIME type data passed across the API ----------------- */

/* typedef struct tlMIMEparamS **tlMIMEparamHandle; /*Mac style declaration */
typedef struct tlMIMEparamS **tlMIMEParamP;
typedef struct tlMIMEparamS {
    Str63                 name;      /* MIME parameter name */
    Handle                value;     /* handle size determines string length */
    struct tlMIMEparamS **next;      /* Handle for next param in list */
/*  tlMIMEparamHandle     next;      /* Mac style declaration for above */ 
} tlMIMEparam;

/* typedef struct tlMIMEtypeS **tlMIMEtypeHandle; /* Mac style declaration */
typedef struct tlMIMEtypeS **tlMIMEtypeP; 
typedef struct tlMIMEtypeS {
    Str63              mime_version; /* MIME-Version: header */
    Str63              mime_type;    /* Top-level MIME type: text,message...*/
    Str63              sub_type;     /* sub-type */
    tlMIMEparam      **params;       /* Handle to first parameter in list */
/*  tlMIMEparamHandle  params;       /* Mac style declaration of above */
} tlMIMEtype;







/* ========== set up for call backs ======================================*/
typedef pascal short (*tlProgress_t)(short percent); 

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr tlProgress;
#else
typedef tlProgress_t tlProgress; 
#endif
typedef tlProgress tlProgressUPP;

enum {
    upptlProgressProcInfo = kPascalStackBased
        | RESULT_SIZE(SIZE_CODE(sizeof(short)))
        | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
};


#if TARGET_RT_MAC_CFM
#define NewtlProgressProc(userRoutine)	 (UniversalProcPtr) NewRoutineDescriptor((ProcPtr)(userRoutine), upptlProgressProcInfo, GetCurrentArchitecture()) 
#else
#define NewtlProgressProc(userRoutine)	 ((tlProgress_t) (userRoutine))
#endif



/* ========== ENTRY POINT ENUM CONSTANTS ==================================*/

enum {
    ktl_module_versionRtn = 256,
    ktl_module_initRtn = 257,
    ktl_translator_infoRtn = 258,
    ktl_can_translate_fileRtn = 259,
    ktl_can_translate_bufRtn = 260,
    ktl_translate_fileRtn = 261,
    ktl_translate_bufRtn = 262,
    ktl_module_finishRtn = 263
};


/* ========== FUNCTION PROTOTYPES ==========================================*/
/* The following is so the same headers can be used for component routines. */
/* FIVEWORDINLINE is magic inline stuff for 68K application and nada for    */
/* PPC */
#define CalltlComponent(callType) \
    FIVEWORDINLINE(0x2F3C,\
                   sizeof(tl_##callType##Params)-sizeof(ComponentInstance),\
                   ktl_##callType##Rtn,0x7000,0xA82A)



#define TLGCPCALL(x) \
    (tl_GCP.what = ktl_##x##Rtn, \
     tl_GCP.paramSize = (sizeof(tl_##x##Params)-sizeof(ComponentInstance)), \
     tl_GCP.flags = 0, \
     Calltl_ComponentFunction(tl_GCP))




/* ----- Get the API Version number this module implements ----------------- */
typedef struct tl_module_versionParams {
    ComponentInstance tl_instance;
} tl_module_versionParams;

pascal ComponentResult tl_module_version(
    ComponentInstance tl_instance
)
CalltlComponent(module_version);

#if TARGET_RT_MAC_CFM
#define tl_module_version(p1)\
    (\
        tl_GCP.params.tl_module_version.tl_instance = p1,\
    TLGCPCALL(module_version))
#endif



/* ----- Initialize module and get its basic info -------------------------- */
typedef struct tl_module_initParams {
    Handle *trans_icon;
    short *module_id;
    StringPtr **module_desc;
    short    *num_trans;
    ComponentInstance tl_instance;
} tl_module_initParams;

pascal ComponentResult tl_module_init(
    ComponentInstance tl_instance,
    short    *num_trans,        /* Place to return num of translators */
    StringPtr **module_desc,    /* Return for string description of module */
    short *module_id,           /* Place to return unique module id */
    Handle *trans_icon          /* Return for translator icon suite */
)
CalltlComponent(module_init);

#if TARGET_RT_MAC_CFM
#define tl_module_init(p1,p2,p3,p4,p5)\
    (\
        tl_GCP.params.tl_module_init.tl_instance = p1,\
        tl_GCP.params.tl_module_init.num_trans = p2,\
        tl_GCP.params.tl_module_init.module_desc = p3,\
        tl_GCP.params.tl_module_init.module_id = p4,\
        tl_GCP.params.tl_module_init.trans_icon = p5,\
    TLGCPCALL(module_init))
#endif



/* ----- Get details about a translator in a module ------------------------ */
typedef struct tl_translator_infoParams {
    Handle *trans_icon;
    StringPtr **trans_desc;
    unsigned long *trans_flags;
    long *trans_sub_type;
    long *trans_type;
    short trans_id;
    ComponentInstance tl_instance;
} tl_translator_infoParams;

pascal ComponentResult tl_translator_info(
    ComponentInstance tl_instance,
    short trans_id,             /* ID of translator to get info for */
    long *trans_type,           /* Return for translator type, e.g., TLT_xxx */
    long *trans_sub_type,       /* Return for translator subtype */
    unsigned long *trans_flags, /* Return for translator flags */
    StringPtr **trans_desc,     /* Return for translator string description */
    Handle *trans_icon          /* Return for translator icon suite */
)
CalltlComponent(translator_info);

#if TARGET_RT_MAC_CFM
#define tl_translator_info(p1,p2,p3,p4,p5,p6,p7)\
    (\
        tl_GCP.params.tl_translator_info.tl_instance = p1,\
        tl_GCP.params.tl_translator_info.trans_id = p2,\
        tl_GCP.params.tl_translator_info.trans_type = p3,\
        tl_GCP.params.tl_translator_info.trans_sub_type = p4,\
        tl_GCP.params.tl_translator_info.trans_flags = p5,\
        tl_GCP.params.tl_translator_info.trans_desc = p6,\
        tl_GCP.params.tl_translator_info.trans_icon = p7,\
    TLGCPCALL(translator_info))
#endif



/* ----- Check and see if a translation can be performed (file version) ---- */
typedef struct tl_can_translate_fileParams {
    long *out_code;
    StringPtr **out_error;
    long *out_len;
    StringPtr ***addresses;
    FSSpec *in_file;
    tlMIMEtypeP in_mime;
    short trans_id;
    long context;
    ComponentInstance tl_instance;
} tl_can_translate_fileParams;

pascal ComponentResult tl_can_translate_file(
    ComponentInstance tl_instance,
    long context,               /* Context for check; e.g. TLF_ON_xxx */
    short trans_id,             /* ID of translator to call */
    tlMIMEtypeP in_mime,        /* MIME type of data to check */
    FSSpec *in_file,            /* The input file FSSpec */
    StringPtr ***addresses,     /* List of addresses (sender and recipients) */
    long *out_len,              /* Place to return estimated length (unused) */
    StringPtr **out_error,      /* Place to return string with error message */
    long *out_code              /* Return for translator-specific result code */
)
CalltlComponent(can_translate_file);

#if TARGET_RT_MAC_CFM
#define tl_can_translate_file(p1,p2,p3,p4,p5,p6,p7,p8,p9)\
    (\
        tl_GCP.params.tl_can_translate_file.tl_instance = p1,\
        tl_GCP.params.tl_can_translate_file.context = p2,\
        tl_GCP.params.tl_can_translate_file.trans_id = p3,\
        tl_GCP.params.tl_can_translate_file.in_mime = p4,\
        tl_GCP.params.tl_can_translate_file.in_file = p5,\
        tl_GCP.params.tl_can_translate_file.addresses = p6,\
        tl_GCP.params.tl_can_translate_file.out_len = p7,\
        tl_GCP.params.tl_can_translate_file.out_error = p8,\
        tl_GCP.params.tl_can_translate_file.out_code = p9,\
    TLGCPCALL(can_translate_file))
#endif



/* ----- Check and see if a translation can be performed (buffer version) -- */
typedef struct tl_can_translate_bufParams {
    long *out_code;
    StringPtr **out_error;
    long *out_len;
    StringPtr ***addresses;
    long in_buffer_len;
    unsigned char **in_buffer;
    tlMIMEtypeP in_mime;
    short trans_id;
    long context;
    ComponentInstance tl_instance;
} tl_can_translate_bufParams;

pascal ComponentResult tl_can_translate_buf(
    ComponentInstance tl_instance,
    long context,               /* Context for check; e.g. TLF_ON_xxx */
    short trans_id,             /* ID of translator to call */
    tlMIMEtypeP in_mime,        /* MIME type of data to check */
    unsigned char **in_buffer,  /* Handle to buffer of data to check */
    long in_buffer_len,         /* Length of data in buffer */
    StringPtr ***addresses,     /* List of addresses (sender and recipients) */
    long *out_len,              /* Place to return estimated length */
    StringPtr **out_error,      /* Place to return string with error message */
    long *out_code              /* Return for translator-specific result code */
)
CalltlComponent(can_translate_buf);

#if TARGET_RT_MAC_CFM
#define tl_can_translate_buf(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)\
    (\
        tl_GCP.params.tl_can_translate_buf.tl_instance = p1,\
        tl_GCP.params.tl_can_translate_buf.context = p2,\
        tl_GCP.params.tl_can_translate_buf.trans_id = p3,\
        tl_GCP.params.tl_can_translate_buf.in_mime = p4,\
        tl_GCP.params.tl_can_translate_buf.in_buffer = p5,\
        tl_GCP.params.tl_can_translate_buf.in_buffer_len = p6,\
        tl_GCP.params.tl_can_translate_buf.addresses = p7,\
        tl_GCP.params.tl_can_translate_buf.out_len = p8,\
        tl_GCP.params.tl_can_translate_buf.out_error = p9,\
        tl_GCP.params.tl_can_translate_buf.out_code = p10,\
    TLGCPCALL(can_translate_buf))
#endif



/* ----- Actually perform a translation on a file -------------------------- */
typedef struct tl_translate_fileParams {
    long *out_code;
    StringPtr **out_error;
    StringPtr **out_desc;
    Handle *out_icon;
    FSSpec *out_file;
    tlMIMEtypeP *out_mime;
    tlProgress progress;
    StringPtr ***addresses;
    FSSpec *in_file;
    tlMIMEtypeP in_mime;
    short trans_id;
    long context;
    ComponentInstance tl_instance;
} tl_translate_fileParams;

pascal ComponentResult tl_translate_file(
    ComponentInstance tl_instance,
    long context,               /* Context for translation; e.g. TLF_ON_xxx */
    short trans_id,             /* ID of translator to call */
    tlMIMEtypeP in_mime,        /* MIME type of input data */
    FSSpec *in_file,            /* The input file name */
    StringPtr ***addresses,     /* List of addresses (sender and recipients) */
    tlProgress progress,        /* Func to report progress / check for abort */
    tlMIMEtypeP *out_mime,      /* Place to return MIME type of result */
    FSSpec *out_file,           /* The output file (specified by Eudora) */
    Handle *out_icon,           /* Returned icon suite representing the result */
    StringPtr **out_desc,       /* Returned string for display with the result */
    StringPtr **out_error,      /* Place to return string with error message */
    long *out_code              /* Return for translator-specific result code */
)
CalltlComponent(translate_file);

#if TARGET_RT_MAC_CFM
#define tl_translate_file(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13)\
    (\
        tl_GCP.params.tl_translate_file.tl_instance = p1,\
        tl_GCP.params.tl_translate_file.context = p2,\
        tl_GCP.params.tl_translate_file.trans_id = p3,\
        tl_GCP.params.tl_translate_file.in_mime = p4,\
        tl_GCP.params.tl_translate_file.in_file = p5,\
        tl_GCP.params.tl_translate_file.addresses = p6,\
        tl_GCP.params.tl_translate_file.progress = p7,\
        tl_GCP.params.tl_translate_file.out_mime = p8,\
        tl_GCP.params.tl_translate_file.out_file = p9,\
        tl_GCP.params.tl_translate_file.out_icon = p10,\
        tl_GCP.params.tl_translate_file.out_desc = p11,\
        tl_GCP.params.tl_translate_file.out_error = p12,\
        tl_GCP.params.tl_translate_file.out_code = p13,\
    TLGCPCALL(translate_file))
#endif



/* ----- Actually perform a translation on a buffer ------------------------ */
typedef struct tl_translate_bufParams {
    long *out_code;
    StringPtr **out_error;
    StringPtr **out_desc;
    Handle *out_icon;
    unsigned char **out_buffer;
    tlMIMEtypeP *out_mime;
    tlProgress progress;
    StringPtr ***addresses;
    long *in_buffer_len;
    long in_offset;
    unsigned char **in_buffer;
    tlMIMEtypeP in_mime;
    short trans_id;
    long context;
    ComponentInstance tl_instance;
} tl_translate_bufParams;

pascal ComponentResult tl_translate_buf(
    ComponentInstance tl_instance,
    long context,               /* Context for translation; e.g. TLF_ON_xxx */
    short trans_id,             /* ID of translator to call */
    tlMIMEtypeP in_mime,        /* MIME type of input data */
    unsigned char **in_buffer,  /* Handle to buffer of data to translate */
    long in_offset,             /* Offset to start of input data in buffer */
    long *in_buffer_len,        /* Amount of input & returns input consumed */
    StringPtr ***addresses,     /* List of addresses (sender and recipients) */
    tlProgress progress,        /* Func to report progress/check for abort */
    tlMIMEtypeP *out_mime,      /* Place to return MIME type of result */
    unsigned char **out_buffer, /* Handle to buffer output should be placed in */
    Handle *out_icon,           /* Returned icon suite representing the result */
    StringPtr **out_desc,       /* Returned string for display with the result */
    StringPtr **out_error,      /* Place to return string with error message */
    long *out_code              /* Return for translator-specific result code */
)
CalltlComponent(translate_buf);

#if TARGET_RT_MAC_CFM
#define tl_translate_buf(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15)\
    (\
        tl_GCP.params.tl_translate_buf.tl_instance = p1,\
        tl_GCP.params.tl_translate_buf.context = p2,\
        tl_GCP.params.tl_translate_buf.trans_id = p3,\
        tl_GCP.params.tl_translate_buf.in_mime = p4,\
        tl_GCP.params.tl_translate_buf.in_buffer = p5,\
        tl_GCP.params.tl_translate_buf.in_offset = p6,\
        tl_GCP.params.tl_translate_buf.in_buffer_len = p7,\
        tl_GCP.params.tl_translate_buf.addresses = p8,\
        tl_GCP.params.tl_translate_buf.progress = p9,\
        tl_GCP.params.tl_translate_buf.out_mime = p10,\
        tl_GCP.params.tl_translate_buf.out_buffer = p11,\
        tl_GCP.params.tl_translate_buf.out_icon = p12,\
        tl_GCP.params.tl_translate_buf.out_desc = p13,\
        tl_GCP.params.tl_translate_buf.out_error = p14,\
        tl_GCP.params.tl_translate_buf.out_code = p15,\
    TLGCPCALL(translate_buf))
#endif



/* ----- End use of a module and clean up ---------------------------------- */
typedef struct tl_module_finishParams {
    ComponentInstance tl_instance;
} tl_module_finishParams;

pascal ComponentResult tl_module_finish(
    ComponentInstance tl_instance
)
CalltlComponent(module_finish);

#if TARGET_RT_MAC_CFM
#define tl_module_finish(p1)\
    (\
        tl_GCP.params.tl_module_finish.tl_instance = p1,\
    TLGCPCALL(module_finish))
#endif



/* ========== UNION DEFINITION PPC only ====================================*/
#if TARGET_RT_MAC_CFM
typedef union tl_ComponentParamUnion
{
    tl_module_versionParams     tl_module_version;
    tl_module_initParams        tl_module_init;
    tl_translator_infoParams    tl_translator_info;
    tl_can_translate_fileParams tl_can_translate_file;
    tl_can_translate_bufParams  tl_can_translate_buf;
    tl_translate_fileParams     tl_translate_file;
    tl_translate_bufParams      tl_translate_buf;
    tl_module_finishParams      tl_module_finish;
} tl_ComponentParamUnion;

typedef struct tl_ComponentParameters
{
	unsigned char flags;		/* Flags - set to zero */
	unsigned char paramSize;	/* Size of the params struct */
	short what;                     /* The component request selector */
	tl_ComponentParamUnion params;
} tl_ComponentParameters;


/* This global takes the routine parameters for each component function call */
extern tl_ComponentParameters tl_GCP;


/* Routine to put a copy of the component params on the stack and call
   CallComponentUPP.  This function is implemented in tlapi-gum.c */
ComponentResult Calltl_ComponentFunction(tl_ComponentParameters params);

#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#endif


#ifdef __cplusplus
}
#endif
#endif /* TLAPI_MACGLUE_H_INCLUDED */

