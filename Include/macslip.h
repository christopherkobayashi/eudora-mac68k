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

#ifndef MACSLIP_H
#define MACSLIP_H
/*
 * Callable.h
 *
 * Definitions for external interface to MacSLIP
 */
 
/*
 * Copyright, (C), 1992-1995 Hyde Park Software.
 * All rights reserved.
 * May not be copied or distributed without permission.
 */

typedef long (*CallableProcPtr)(long selector, void *value1, void *value2, void *value3, long *ret_val);

enum {
    uppCallableProcInfo = kCStackBased
         | RESULT_SIZE(SIZE_CODE(sizeof(long)))
         | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
         | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
         | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Ptr)))
         | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(Ptr)))
         | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(Ptr)))
};

#if TARGET_RT_MAC_CFM
typedef UniversalProcPtr CallableUPP;

#define CallCallableProc(userRoutine, selector, value1, value2, value3, ret_val)		\
		CallUniversalProc((UniversalProcPtr)(userRoutine), uppCallableProcInfo, selector, value1, value2, value3, ret_val)
#define NewCallableProc(userRoutine)		\
		(CallableUPP) NewRoutineDescriptor((ProcPtr)(userRoutine), uppCallableProcInfo, GetCurrentISA())
#else
typedef CallableProcPtr CallableUPP;

#define CallCallableProc(userRoutine, selector, value1, value2, value3, ret_val)		\
		(*(userRoutine))(selector, value1, value2, value3, ret_val)
#define NewCallableProc(userRoutine)		\
		(CallableUPP)(userRoutine)
#endif

#define CALLVERS 4
struct ext_callmem {
    short vers;
	CallableUPP callableproc;
}; 


/*
 * request types
 *
 *                      what        value1        	value2          value3    
 */
enum CA {
    CA_CONNECT = 1,  		/* connect     	NULL,           NULL,           NULL        */
    CA_CONNECTI,     		/* NA se       	NULL,           NULL,           NULL        */
    CA_DISCONNECT,   		/* disc        	NULL,           NULL,           NULL        */
    CA_DROPDTR,			    /* drop dtr    	long *bool,     NULL,           NULL        */

    CA_CTSOUT,       		/* cts out     	long *bool,     NULL,           NULL        */
    CA_DTRIN,        		/* dtr in      	long *bool,     NULL,           NULL        */
    CA_MTU,          		/* mtu         	long *value,    NULL,           NULL        */
    CA_SPEED,        		/* speed       	long *value,    NULL,           NULL        */
    CA_COMPRESS,     		/* compress    	long *value,    NULL,           NULL        */
    CA_PORT,         		/* port        	char *portin,   char *portout,  NULL        */
    CA_LOGONOFF,     		/* log         	long *bool,     NULL,           NULL        */
    CA_SCRIPTONOFF,  		/* script      	long *bool,     NULL,           NULL        */
    CA_LOGNAME,      		/* logfile     	short *vref,    long *dirid,    char *name  */
    CA_SCRIPTNAME,   		/* scriptf     	short *vref,    long *dirid,    char *name  */
    CA_STARTUP,      		/* startup     	long *bool,     NULL,           NULL        */

    CA_GET,          		/* dummy */
    CA_GETDROPDTR,   		/* drop dtr    	long *bool,     NULL,           NULL        */
    CA_GETCTSOUT,    		/* cts out     	long *bool,     NULL,           NULL        */
    CA_GETDTRIN,     		/* dtr in      	long *bool,     NULL,           NULL        */
    CA_GETMTU,       		/* mtu         	long *value,    NULL,           NULL        */
    CA_GETSPEED,     		/* speed       	long *value,    NULL,           NULL        */
    CA_GETCOMPRESS,  		/* compress    	long *value,    NULL,           NULL        */
    CA_GETPORT,      		/* in port     	char *portin,   char *portout,  NULL        */
    CA_GETLOGONOFF,  		/* log         	long *bool,     NULL,           NULL        */
    CA_GETSCRIPTONOFF,		/* script      	long *bool,     NULL,           NULL        */
    CA_GETLOGNAME,   		/* logfile     	char *volname,  long *dirid,   	char *name   */
    CA_GETSCRIPTNAME,		/* scriptf     	char *volname,  long *dirid,   	char *name   */
    CA_GETSTARTUP,   		/* startup     	long *bool,     NULL,           NULL        */
    CA_GETSTATUS,    		/* status      	long *status,   NULL,           NULL        */
    
    /* callable version 2 */
    CA_SETNAME,      		/* setname     	char *name,     NULL,           NULL        */
    CA_GETSETNAME,   		/* setname     	char *name,     NULL,           NULL        */
    CA_SETVAR,       		/* set var     	char *var,      char *value,    long *sflags */
    CA_GETVAR,       		/* get var     	char *var,      char *value,    long *sflags */
    CA_CONNTCP,      		/* conn tcp    	long *bool,     long *bool,     NULL        */
    CA_GETCONNTCP,   		/* conn tcp    	long *bool,     long *bool,     NULL        */
    CA_NOTIFY,       		/* notify      	long *bool,     long *bool,     long *secs  */
    CA_GETNOTIFY,    		/* notify      	long *bool,     long *bool,     long *secs  */
    CA_MODEM,        		/* modem       	char *name,     long *bool,     NULL        */
    CA_GETMODEM,     		/* modem       	char *name,     long *bool,     NULL        */
    CA_USECTS,       		/* usects      	long *bool,     NULL,           NULL        */
    CA_GETUSECTS,    		/* usects      	long *bool,     NULL,           NULL        */

    /* callable version 3 */
    CA_AUTODISC,    		/* autodisc		long *bool,     NULL,           NULL        */
    CA_GETAUTODISC, 		/* autodisc     long *bool,     NULL,           NULL        */
    CA_AUTODISCIDLE,		/* idle         long *bool,     NULL,           NULL        */
    CA_GETAUTODISCIDLE,		/* idle         long *bool,     NULL,           NULL        */
    CA_AUTODISCNOTCP,		/* no tcp      	long *bool,     NULL,           NULL        */
    CA_GETAUTODISCNOTCP,	/* no tcp      	long *bool,     NULL,           NULL        */
    CA_AUTODISCNOTIFY,		/* notify     	long *bool,     NULL,           NULL        */
    CA_GETAUTODISCNOTIFY,	/* notify     	long *bool,     NULL,           NULL        */
    CA_AUTODISCINTERVAL,	/* interval 	long *interval, NULL,           NULL        */
    CA_GETAUTODISCINTERVAL,	/* interval 	long *interval, NULL,           NULL        */
    CA_PACKETCONNECT,		/* packet      	long *bool,     NULL,           NULL        */
    CA_GETPACKETCONNECT,	/* packet      	long *bool,     NULL,           NULL        */

    /* special configuration items */
    CA_IGNORESER,   		/* ignore serial long *bool,    NULL,           NULL        */
    CA_GETIGNORESER,   		/* ignore serial long *bool,    NULL,           NULL        */
    CA_BOOTPTIME,   		/* bootp time	long *interval, NULL,           NULL        */
    CA_GETBOOTPTIME,   		/* bootp time   long *interval, NULL,           NULL        */
    CA_EXPRESSBUG,  		/* express bug  long *bool,     NULL,           NULL        */
    CA_GETEXPRESSBUG,  		/* express bug  long *bool,     NULL,           NULL        */
    CA_IGNORESCC,   		/* ignore scc   long *bool,     NULL,           NULL        */
    CA_GETIGNORESCC,   		/* ignore scc   long *bool,     NULL,           NULL        */

    /* used internally */
    CA_DOSTATE,      		/* internal 	NULL,         	NULL,           NULL       	*/
    CA_CONNECTO,      		/* internal 	NULL,         	NULL,           NULL        */
    CA_DISCONNECTO,   		/* internal    	NULL,           NULL,           NULL        */

	/* callable version 4 */
	/* misc runtime options, not saved in config set*/
	CA_NOMESSAGES,			/* no messages  long *bool, 	NULL,			NULL		*/
	CA_LOSS_ALERT,			/* en/dis-able  long *bool, 	NULL,			NULL		*/
	CA_SETUP,				/* internal     long *setup,    NULL,           NULL        */
    CA_LOGAPPEND,   		/* append to log long *bool,    NULL,           NULL        */
	CA_GETLOGAPPEND			/* append to log long *bool,    NULL,           NULL        */

	/* NEW ITEMS MUST BE ADDED HERE !!! */

};


/*
 * error returns
 */
enum CE {
    CE_NO_RSRC_FILE = 1,	/* could not locate resource file */
    CE_NO_SLIP,				/* SLIP has not been initialized */
    CE_BAD_REQUEST,			/* illegal requet */
    CE_BAD_VOL,				/* bad vref */
    CE_BAD_STATECHANGE,		/* bad connect/disconnect */
	CE_NO_MEMORY,			/* out of memory */
	CE_NOT_FOUND			/* not found */
};

/*
 * status returns
 */
enum CES {
	CES_NOSLIP = 1,			/* slip is not enabled */
	CES_DISABLED,			/* disabled */
	CES_DISCONNECTED,		/* disconnected */
	CES_CONNECTED,			/* connected */
	CES_UNKNOWN				/* unknown */
};

/*
 * sflags
 */
#define CSF_GLOBAL	0x0001	/* set global variable */
#define CSF_HIDE	0x0002	/* hide variable */

/*
 * Junk so Emacs will set local variables to be compatible with Mac/MPW.
 * Should be at end of file.
 * 
 * Local Variables:
 * tab-width: 4
 * End:
 */
#endif