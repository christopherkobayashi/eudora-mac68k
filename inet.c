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

#include "inet.h"
#define FILE_NUM 18
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#pragma segment Util
/**********************************************************************
 * DotToNum - turn an address in dotted decimal into an internet address
 * returns True if the conversion was successful.  This routine is
 * somewhat limited, in that it will accept only four octets, and does
 * not permit the abbreviated forms for class A and B networks.
 **********************************************************************/
Boolean DotToNum(UPtr string,long *nPtr)
{
	long address=0;
	Byte b=0;
	UPtr cp;
	int dotcount=0;
	
	/*
	 * allow leading spaces
	 */
	for (cp=string+1;cp<=string+*string;cp++) if (*cp!=' ') break;
	
	/*
	 * the address
	 */
	for (;cp<=string+*string;cp++)
	{
		if (*cp=='.')
		{
			if (++dotcount > 3) return (False); /* only 4 octets allowed */
			address <<= 8;
			address |= b;
			b=0;
		}
		else if (isdigit(*cp))
		{
			b *= 10;
			b += (*cp - '0');
			if (b>255) return (False);					/* keep it under 256 */
		}
		else if (*cp==' ')										/* allow trailing spaces */
			break;
		else
			return (False); 										/* periods or digits ONLY */
	}
	
	/*
	 * final checks, assignment
	 */
	if (dotcount!=3) return (False);
	address <<= 8;
	address |= b;
	*nPtr = address;
	return(True);
}

/**********************************************************************
 * NumToDot - turn an address into a dotted decimal string
 **********************************************************************/
UPtr NumToDot(unsigned long num,UPtr string)
{
	unsigned char* bp=(unsigned char *)&num;
	UPtr cp=string;
	int count=4;
	int length;
	
	for (count=4;count;count--,bp++)
	{
		NumToString((unsigned long)*bp,cp);
		length = *cp;
		*cp = '.';
		cp += length+1;
	}
	*string = cp-string-1;
	return(string);
} 
