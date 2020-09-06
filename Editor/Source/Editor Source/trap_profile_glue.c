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

#if powerc
#include <MacHeadersPPC>
#else
#include <MacHeaders68K>
#endif
#define __TRAP_PROFILE_GLUE_SOURCE__
#include "trap_profile_glue.h"


pascal short _CharType_(Ptr textBuf, short textOffset)
{
	return CharType(textBuf, textOffset);
}

pascal StyledLineBreakCode _StyledLineBreak_(Ptr textPtr, long textLen, long textStart, long textEnd, long flags, Fixed *textWidth, long *textOffset)
{
	return StyledLineBreak(textPtr, textLen, textStart, textEnd, flags, textWidth, textOffset);
}

pascal void _GetIntlResourceTable_(ScriptCode script, short tableCode, Handle *itlHandle, long *offset, long *length)
{
	GetIntlResourceTable(script, tableCode, itlHandle, offset, length);
}

pascal Boolean _ParseTable_(CharByteTable table)
{
	return ParseTable(table);
}

pascal long _GetScriptVariable_(short script, short selector)
{
	return GetScriptVariable(script, selector);
}

pascal SInt8 _HGetState_(Handle h)
{
	return HGetState(h);
}

pascal void _HSetState_(Handle h, SInt8 flags)
{
	HSetState(h, flags);
}

pascal void _HLock_(Handle h)
{
	HLock(h);
}

pascal short _FontScript_(void)
{
	return FontScript();
}

pascal short _TextWidth_(const void *textBuf, short firstByte, short byteCount)
{
	return TextWidth(textBuf, firstByte, byteCount);
}

pascal short _CharWidth_(short ch)
{
	return CharWidth(ch);
}

pascal void _DrawJustified_(Ptr textPtr, long textLength, Fixed slop, JustStyleCode styleRunPosition, Point numer, Point denom)
{
	DrawJustified(textPtr, textLength, slop, styleRunPosition, numer, denom);
}

pascal void _DrawText_(const void *textBuf,short firstByte,short byteCount)
{
	DrawText(textBuf, firstByte, byteCount);
}

pascal void _DrawChar_(short ch)
{
	DrawChar(ch);
}

pascal long _Munger_(Handle h,long offset,const void *ptr1,long len1,const void *ptr2,long len2)
{
	return Munger(h, offset, ptr1, len1, ptr2, len2);
}

pascal OSErr _PtrAndHand_(const void *ptr1, Handle hand2, long size)
{
	return PtrAndHand(ptr1, hand2, size);
}

pascal RgnHandle _NewRgn_(void)
{
	return NewRgn();
}

pascal Boolean _PtInRgn_(Point pt, RgnHandle rgn)
{
	return PtInRgn(pt, rgn);
}

pascal void _DisposeRgn_(RgnHandle rgn)
{
	DisposeRgn(rgn);
}

pascal void _CopyRgn_(RgnHandle srcRgn, RgnHandle dstRgn)
{
	CopyRgn(srcRgn, dstRgn);
}

pascal void _SetEmptyRgn_(RgnHandle rgn)
{
	SetEmptyRgn(rgn);
}

pascal void _SetRectRgn_(RgnHandle rgn, short left, short top, short right, short bottom)
{
	SetRectRgn(rgn, left, top, right, bottom);
}

pascal void _RectRgn_(RgnHandle rgn, const Rect *r)
{
	RectRgn(rgn, r);
}

pascal void _OffsetRgn_(RgnHandle rgn, short dh, short dv)
{
	OffsetRgn(rgn, dh, dv);
}

pascal void _InsetRgn_(RgnHandle rgn, short dh, short dv)
{
	InsetRgn(rgn, dh, dv);
}

pascal void _SectRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
	SectRgn(srcRgnA, srcRgnB, dstRgn);
}

pascal void _UnionRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
	UnionRgn(srcRgnA, srcRgnB, dstRgn);
}

pascal void _DiffRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
	DiffRgn(srcRgnA, srcRgnB, dstRgn);
}

pascal void _XorRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
	XorRgn(srcRgnA, srcRgnB, dstRgn);
}

pascal void _CopyBits_(const BitMap *srcBits, const BitMap *dstBits, const Rect *srcRect, const Rect *dstRect, short mode, RgnHandle maskRgn)
{
	CopyBits(srcBits, dstBits, srcRect, dstRect, mode, maskRgn);
}

pascal Boolean _RectInRgn_(const Rect *r, RgnHandle rgn)
{
	return RectInRgn(r, rgn);
}

pascal Boolean _EqualRgn_(RgnHandle rgnA, RgnHandle rgnB)
{
	return EqualRgn(rgnA, rgnB);
}

pascal Boolean _EmptyRgn_(RgnHandle rgn)
{
	return EmptyRgn(rgn);
}

pascal void _FrameRgn_(RgnHandle rgn)
{
	FrameRgn(rgn);
}

pascal void _PaintRgn_(RgnHandle rgn)
{
	PaintRgn(rgn);
}

pascal void _EraseRgn_(RgnHandle rgn)
{
	EraseRgn(rgn);
}

pascal void _InvertRgn_(RgnHandle rgn)
{
	InvertRgn(rgn);
}

pascal void _FillRgn_(RgnHandle rgn, const Pattern *pat)
{
	FillRgn(rgn, pat);
}

pascal void _ScrollRect_(const Rect *r, short dh, short dv, RgnHandle updateRgn)
{
	ScrollRect(r, dh, dv, updateRgn);
}

pascal short _CharToPixel_ (Ptr textBuf, long textLen, Fixed slop, long offset, short direction, JustStyleCode styleRunPosition, Point numer, Point denom)
{
	return CharToPixel(textBuf, textLen, slop, offset, direction, styleRunPosition, numer, denom);
}

pascal short _PixelToChar_ (Ptr textBuf, long textLen, Fixed slop, Fixed pixelWidth, Boolean *leadingEdge, Fixed *widthRemaining, JustStyleCode styleRunPosition, Point numer, Point denom)
{
	return PixelToChar(textBuf, textLen, slop, pixelWidth, leadingEdge, widthRemaining, styleRunPosition, numer, denom);
}

pascal void _GetClip_(RgnHandle rgn)
{
	GetClip(rgn);
}

pascal void _SetClip_(RgnHandle rgn)
{
	SetClip(rgn);
}

pascal GWorldFlags _UpdateGWorld_(GWorldPtr *offscreenGWorld, short pixelDepth, const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags)
{
	return UpdateGWorld(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, flags);
}

pascal Boolean _LockPixels_(PixMapHandle pm)
{
	return LockPixels(pm);
}

pascal void _GetGWorld_(CGrafPtr *port, GDHandle *gdh)
{
	GetGWorld(port, gdh);
}

pascal void _SetGWorld_(CGrafPtr port, GDHandle gdh)
{
	SetGWorld(port, gdh);
}

pascal void _HNoPurge_(Handle h)
{
	HNoPurge(h);
}

pascal Size _InlineGetHandleSize_(Handle h)
{
	return InlineGetHandleSize(h);
}

pascal void _MoveTo_(short h,short v)
{
	MoveTo(h, v);
}

pascal void _Move_(short dh,short dv)
{
	Move(dh, dv);
}

pascal void _Line_(short dh,short dv)
{
	Line(dh, dv);
}

pascal PixMapHandle _GetGWorldPixMap_(GWorldPtr offscreenGWorld)
{
	return GetGWorldPixMap(offscreenGWorld);
}

pascal void _UnlockPixels_(PixMapHandle pm)
{
	UnlockPixels(pm);
}

pascal void _AllowPurgePixels_(PixMapHandle pm)
{
	AllowPurgePixels(pm);
}

pascal Boolean _EmptyRect_(const Rect *r)
{
	return EmptyRect(r);
}

pascal void _FrameRect_(const Rect *r)
{
	FrameRect(r);
}

pascal short _QDError_(void)
{
	return QDError();
}