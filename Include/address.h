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

#ifndef ADDRESS_H
#define ADDRESS_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
OSErr SuckAddresses(BinAddrHandle * addresses, TextAddrHandle text,
		    Boolean wantComments, Boolean wantErrors,
		    Boolean wantAutoQual, long ***addrSpots);
OSErr SuckPtrAddresses(BinAddrHandle * addresses, TextAddrPtr text,
		       long size, Boolean wantComments, Boolean wantErrors,
		       Boolean wantAutoQual, long ***addrSpots);
PStr ShortAddr(PStr shortAddr, PStr longAddr);
Boolean IsFCCAddr(PStr addr);
Boolean SameAddressStr(PStr addr1, PStr addr2);
PStr CanonAddr(PStr into, PStr addr, PStr name);
short CountAddresses(Handle addresses,
		     short justTellMeIfThereAreAtLeastThisMany);
#define IsNewsgroupAddr(s)	((Byte)(s)[1]==(Byte)'�')
Boolean AddAddressHashUniq(PStr address, AccuPtr a);
#define NextBinAddrOffset(addr,offs) ((offs)+(*(addr))[offs]+2)
#define MakeBinAddrStr(addr,offs,s) PCopy(s,*(addr)+offs)
#endif
