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

/*
	A template class that holds a pointer to a Mach-O framework entry point.

Author: Marshall Clow

Change Log:
		13-Aug-2002		mc			Initial Version
		
*/

#ifndef	__MACHOWRAPPER__
#define	__MACHOWRAPPER__

#include <MacTypes.h>
#include <CFBundle.h>

#if !TARGET_RT_MAC_CFM
#error	"This code only works for CFM Targets!"
#endif

OSStatus LoadFrameworkBundle ( CFStringRef framework, CFBundleRef *bundlePtr );

template <typename Proc>
class MachOWrapper {
private:
	CFBundleRef 	fBundle;		//	The reference to the framework pointer
	Proc			fProcPtr;		//	The actual procedure pointer

public:
	MachOWrapper ( CFStringRef framework, CFStringRef routine ) : fBundle ( NULL ), fProcPtr ( NULL ) {
	//	Get the framework bundle
		OSStatus err = ::LoadFrameworkBundle ( framework, &fBundle );
	//	Don't call "CFBundleGetFunctionPointerForName" with an invalid bundle - it will crash.
		if ( err != noErr )
			fBundle = NULL;
		else {
			fProcPtr = (Proc) CFBundleGetFunctionPointerForName ( fBundle, routine );
			if ( fProcPtr == nil )
				err = cfragNoSymbolErr;
			}
	//	Have to throw something if we don't link up!
		}

	MachOWrapper ( CFBundleRef 	aBundle, CFStringRef routine ) : fBundle ( aBundle ), fProcPtr ( NULL ) {
		OSStatus err = noErr;
		CFRetain ( fBundle );
		fProcPtr = (Proc) CFBundleGetFunctionPointerForName ( fBundle, routine );
		if ( fProcPtr == nil )
			err = cfragNoSymbolErr;
	//	Have to throw something if we don't link up!
		}

	~MachOWrapper () { if ( fBundle != NULL ) CFRelease ( fBundle ); }

	Boolean IsValid () const { return fProcPtr != NULL; }
	
//	Marshall dreams:
//	A whole set of templates for different numbers of arguments
//	The beauty of this is that only one will be instantiated by the compiler
//	for each specialization of this template.

#if 0
	template <typename R>
	R operator () () const
		{ return fProcPtr (); }
	
	template <typename R, typename A1>
	R operator () (A1 arg1) const
		{ return fProcPtr (arg1); }

	template <typename R, typename A1, typename A2>
	R operator () (A1 arg1, A2 arg2) const
		{ return fProcPtr (arg1, arg2); }

	template <typename R, typename A1, typename A2, typename A3>
	R operator () (A1 arg1, A2 arg2, A3 arg3) const
		{ return fProcPtr (arg1, arg2, arg3); }

	template <typename R, typename A1, typename A2, typename A3, typename A4>
	R operator () (A1 arg1, A2 arg2, A3 arg3, A4 arg4) const
		{ return fProcPtr (arg1, arg2, arg3, arg4); }

	template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
	R operator () (A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5) const
		{ return fProcPtr (arg1, arg2, arg3, arg4, arg5); }

	template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	R operator () (A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6) const
		{ return fProcPtr (arg1, arg2, arg3, arg4, arg5, arg6); }
#else
//	Non-template test of general idea
	operator Proc () const { return fProcPtr; }
#endif

private:
//	Defined but not declared
	MachOWrapper ();
	MachOWrapper ( const MachOWrapper &rhs );
	MachOWrapper & operator = ( const MachOWrapper &rhs );	
	};

#endif