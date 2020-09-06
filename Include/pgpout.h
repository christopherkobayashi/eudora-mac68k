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

#ifndef PGPOUT_H
#define PGPOUT_H


/**********************************************************************
 * MacPGP/ViaCrypt constants
 **********************************************************************/

/*
 * class id
 */
#define kPGPClass		'MPGP'

/*
 * events
 */
#define kPGPEncrypt	'encr'
#define kPGPSign		'sign'
#define kPGPDecrypt	'decr'
#define kPGPAddKey	'addk'
#define kPGPExtract	'extr'
#define kPGPFingerPrint	'fing'
#define	kPGPExecute	'exec'

/*
 * keywords
 */
#define keyPGPToUser		'recv'
#define keyPGPToFile		'recv'
#define keyPGPFromUser	'usid'
#define keyPGPSignature	'sign'
#define keyPGPReadMode	'read'
#define keyPGPOutput		'outp'
#define keyPGPKeyRing		'keyr'
#define keyPGPNoSuffix	'nsfx'

/*
 * read modes
 */
#define ePGPReadMacBinary		'macb'
#define ePGPReadNotText			'norm'
#define ePGPReadText				'text'

/*
 * write modes
 */
#define ePGPWriteBinary			'bina'
#define ePGPWriteAscii			'asci'

/*
 * signature options
 */
#define ePGPSigSeparate			'sepa'
#define ePGPSigIncluded			'incl'
#define ePGPSigOmitted			'omit'
#define ePGPSigClear				'clea'

/**********************************************************************
 * Routines
 **********************************************************************/
Boolean HaveKeyFor(PStr address,Boolean fromPrivate);
Boolean HaveAllKeys(MessHandle messH);
Boolean HaveKeysFrom(Handle text,PStr missing,Boolean fromPrivate);
short PGPSendMessage(TransStream stream,MessHandle messH,Boolean chatter);
OSErr BuildAddressList(Handle addresses,AEDescList *list);
OSErr PGPFetchResult(AppleEvent *reply, FSSpecPtr spec);

OSErr StartPGP(ProcessSerialNumber *psn);

#define kNoAsciiArmor	False
#define kAsciiArmor	True

#define kPublicRing False
#define kPrivateRing True
#endif
