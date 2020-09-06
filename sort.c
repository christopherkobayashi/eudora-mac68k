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

#include <conf.h>
#include <mydefs.h>

#include "sort.h"
#define FILE_NUM 36
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1994 by QUALCOMM Incorporated */

#pragma segment Misc

#define ARRAY(i)				(array+i*size)
void QSort(UPtr array, int size, int f, int l, int (*compare)(),
	   void(*swap)());
/**********************************************************************
 * QuickSort - sort an array
 * Algorithms, Reingold, Nievergelt, Deo, p. 286
 **********************************************************************/
void QuickSort(UPtr array, int size, int f, int l, int (*compare)(),
	       void(*swap)())
{
	int i, j;
	if (f >= l)
		return;

#ifdef NEVER
	ComposeLogS(-1, nil, "\pSort %d %d", f, l);
#endif
	CycleBalls();

	(*swap) (ARRAY(f), ARRAY((f + l) / 2));

	for (i = f + 1; i <= l && (*compare) (ARRAY(i), ARRAY(f)) < 0;
	     i++);

	for (j = l; j >= f && (*compare) (ARRAY(j), ARRAY(f)) > 0; j--);

	while (i < j) {
		(*swap) (ARRAY(i), ARRAY(j));
		for (i++; (*compare) (ARRAY(i), ARRAY(f)) < 0; i++);
		for (j--; (*compare) (ARRAY(j), ARRAY(f)) > 0; j--);
	}

	(*swap) (ARRAY(f), ARRAY(j));

	if ((j - 1) - f > l - (j + 1)) {
		QuickSort(array, size, j + 1, l, compare, swap);
		QuickSort(array, size, f, j - 1, compare, swap);
	} else {
		QuickSort(array, size, f, j - 1, compare, swap);
		QuickSort(array, size, j + 1, l, compare, swap);
	}
}

#define VectorSwap(x,y) do{M_T1=(*vector)[x];(*vector)[x]=(*vector)[y];(*vector)[y]=M_T1;}while(0)
/**********************************************************************
 * VQuickSort - sort a vector into something else
 * Algorithms, Reingold, Nievergelt, Deo, p. 286
 **********************************************************************/
void VQuickSort(short **vector, short f, short l, void *data,
		int (*compare)())
{
	int i, j;
	if (f >= l)
		return;

#ifdef NEVER
	ComposeLogS(-1, nil, "\pSort %d %d", f, l);
#endif
	CycleBalls();

	VectorSwap(f, (f + l) / 2);

	for (i = f + 1; i <= l && (*compare) (vector, data, i, f) < 0;
	     i++);

	for (j = l; j >= f && (*compare) (vector, data, j, f) > 0; j--);

	while (i < j) {
		VectorSwap(i, j);
		for (i++; (*compare) (vector, data, i, f) < 0; i++);
		for (j--; (*compare) (vector, data, j, f) > 0; j--);
	}

	VectorSwap(f, j);

	if ((j - 1) - f > l - (j + 1)) {
		VQuickSort(vector, j + 1, l, data, compare);
		VQuickSort(vector, f, j - 1, data, compare);
	} else {
		VQuickSort(vector, f, j - 1, data, compare);
		VQuickSort(vector, j + 1, l, data, compare);
	}
}

void StrSwap(UPtr s1, UPtr s2)
{
	Str255 temp;

	BMD(s1, temp, *(unsigned char *) s1 + 1);
	BMD(s2, s1, *(unsigned char *) s2 + 1);
	BMD(temp, s2, *(unsigned char *) temp + 1);
}

int CStrCompar(UPtr s1, UPtr s2)
{
#pragma unused(s1,s2)
	return (0);
}

void CStrSwap(UPtr s1, UPtr s2)
{
#pragma unused(s1,s2)
}

int SortStrCompare(UPtr * s1, UPtr * s2)
{
	return StringComp(*s1, *s2);
}
