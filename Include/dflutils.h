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

#ifndef DFLUTILS_H
#define DFLUTILS_H


/* DFL stands for dynamically-loaded fragment library */
/* DFR stands for dynamically-loaded fragment routine */



#include "dflsuppl.h"



enum {
	kDFLBaseErrNum = 101,
	kDFLNeedMixedModeMgrErr = kDFLBaseErrNum, /* 101 */
	kDFLNeedCFMErr, /* 102 */
	kDFLBadCFM68KVersErr, /* 103 */
	kDFLBadLibVersErr, /* 104 */
	kDFLLibCodeNotLoadedErr /* 105 */
};


#define BUILD_UNIMPLEMENTED_ROUTINE_DESCRIPTOR()	\
	{																								\
	_Unimplemented,																	\
	kRoutineDescriptorVersion,											\
	kSelectorsAreNotIndexable,											\
	0,																							\
	0,																							\
	0,																							\
	0,																							\
	{																								\
	{																								\
	0,																							\
	0,																							\
	kOld68kRTA | kM68kISA, 													\
	kProcDescriptorIsAbsolute |	 										\
	kFragmentIsPrepared |														\
	kUseNativeISA,																	\
	(ProcPtr)0,																			\
	0,																							\
	0																								\
	}																								\
	}																								\
	}

#define BUILD_DFR_ROUTINE_DESCRIPTOR(routineFlags, procAddr, procISA)	\
	{																																		\
	_MixedModeMagic,																										\
	kRoutineDescriptorVersion,																					\
	kSelectorsAreNotIndexable,																					\
	0,																																	\
	0,																																	\
	0,																																	\
	0,																																	\
	{																																		\
	{																																		\
	(routineFlags),																											\
	0,																																	\
	(procISA),					 																								\
	kProcDescriptorIsAbsolute |																					\
	kFragmentIsPrepared |																								\
	kUseNativeISA,																											\
	(ProcPtr)(procAddr),																								\
	0,																																	\
	0																																		\
	}																																		\
	}																																		\
	}


#define DECLARE_DFR_UPP(routineName) \
RoutineDescriptor routineName##RD = BUILD_UNIMPLEMENTED_ROUTINE_DESCRIPTOR(); \
RoutineDescriptorPtr routineName##UPP = &routineName##RD

#define DECLARE_DFR_EXTERN_UPP(routineName, procInfo) \
enum {upp##routineName##ProcInfo=(procInfo)}; \
extern RoutineDescriptorPtr routineName##UPP

#define LoadDFR(routineName) \
InitDFR(#routineName, routineName##UPP, upp##routineName##ProcInfo)

#define UnloadDFR(routineName) \
InvalDFR(routineName##UPP)



/* FINISH *//* remove these */
#if 0
#if GENERATING68KSEGLOAD

void A0ResultToD0(void)
	= {0x2008};
	/*
		MOVE.L		A0,D0
	*/


void D0ResultToA0(void)
	= {0x2040};
	/*
		MOVEA.L		D0,A0
	*/


long CallThinkCRegA0ResultUPP(RoutineDescriptorPtr theProcPtr, ProcInfoType procInfo, ...)
	= {0x205F, 0x584F, 0x4E90, 0x2008};
	/*
		MOVEA.L		(A7)+,A0
		ADDQ.W		#4,A7
		JSR				(A0)
		MOVE.L		A0,D0
	*/


/* FINISH *//* which of these is right? */
#if 0
long CallThinkCRegD0ResultUPP(RoutineDescriptorPtr theProcPtr, ProcInfoType procInfo, ...)
	= {0x205F, 0x584F, 0x4E90, 0x2040};
	/*
		MOVEA.L		(A7)+,A0
		ADDQ.W		#4,A7
		JSR				(A0)
		MOVEA.L		D0,A0
	*/
#endif


long CallThinkCRegD0ResultUPP(RoutineDescriptorPtr theProcPtr, ProcInfoType procInfo, ...)
	= {0x205F, 0x584F, 0x4E90};
	/*
		MOVEA.L		(A7)+,A0
		ADDQ.W		#4,A7
		JSR				(A0)
	*/

#endif
#endif



extern Boolean useDFLStubLib;



void ResetDFRLoaderGlobals(void);
void SetDFRLoaderGlobals(CFragConnectionID connID, ISAType connISA);
OSErr DFRLoaderErr(void);
OSErr InitDFR(char* symbolNameCStr, RoutineDescriptorPtr routineRD, ProcInfoType procInfo);
void InvalDFR(RoutineDescriptorPtr routineRD);
Boolean CFM68KOkay(void);
OSErr SystemSupportsDFRLibraries(OSType libArchType);
OSErr LoadDFRLibrary(ConstStr63Param libName, OSType libArchType, CFragConnectionID *connID, Str255 errMessage);
OSErr UnloadDFRLibrary(CFragConnectionID *connID);


#endif