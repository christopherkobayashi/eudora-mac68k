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

#ifndef __TRAP_PROFILE_GLUE_SOURCE__
#define CharType(textBuf, textOffset) _CharType_(textBuf, textOffset)
#define StyledLineBreak(textPtr, textLen, textStart, textEnd, flags, textWidth, textOffset) _StyledLineBreak_(textPtr, textLen, textStart, textEnd, flags, textWidth, textOffset)
#define GetScriptVariable(script, selector) _GetScriptVariable_(script, selector)
#define ParseTable(table) _ParseTable_(table)
#define GetIntlResourceTable(script, tableCode, itlHandle, offset, length) _GetIntlResourceTable_(script, tableCode, itlHandle, offset, length)
#define HGetState(h) _HGetState_(h)
#define HSetState(h, flags) _HSetState_(h, flags)
#define HLock(h) _HLock_(h)
#define FontScript() _FontScript_()
#define TextWidth(textBuf, firstByte, byteCount) _TextWidth_(textBuf, firstByte, byteCount)
#define CharWidth(ch) _CharWidth_(ch)
#define DrawJustified(textPtr, textLength, slop, styleRunPosition, numer, denom) _DrawJustified_(textPtr, textLength, slop, styleRunPosition, numer, denom)
#define DrawText(textBuf, firstByte, byteCount) _DrawText_(textBuf, firstByte, byteCount)
#define DrawChar(ch) _DrawChar_(ch)
#define Munger(h, offset, ptr1, len1, ptr2, len2) _Munger_(h, offset, ptr1, len1, ptr2, len2)
#define PtrAndHand(ptr1, hand2, size) _PtrAndHand_(ptr1, hand2, size)
#define NewRgn(void) _NewRgn_(void)
#define PtInRgn(pt, rgn) _PtInRgn_(pt, rgn)
#define DisposeRgn(rgn) _DisposeRgn_(rgn)
#define CopyRgn(srcRgn, dstRgn) _CopyRgn_(srcRgn, dstRgn)
#define SetEmptyRgn(rgn) _SetEmptyRgn_(rgn)
#define SetRectRgn(rgn, left, top, right, bottom) _SetRectRgn_(rgn, left, top, right, bottom)
#define RectRgn(rgn, r) _RectRgn_(rgn, r)
#define OffsetRgn(rgn, dh, dv) _OffsetRgn_(rgn, dh, dv)
#define InsetRgn(rgn, dh, dv) _InsetRgn_(rgn, dh, dv)
#define SectRgn(srcRgnA, srcRgnB, dstRgn) _SectRgn_(srcRgnA, srcRgnB, dstRgn)
#define UnionRgn(srcRgnA, srcRgnB, dstRgn) _UnionRgn_(srcRgnA, srcRgnB, dstRgn)
#define DiffRgn(srcRgnA, srcRgnB, dstRgn) _DiffRgn_(srcRgnA, srcRgnB, dstRgn)
#define XorRgn(srcRgnA, srcRgnB, dstRgn) _XorRgn_(srcRgnA, srcRgnB, dstRgn)
#define CopyBits(srcBits, dstBits, srcRect, dstRect, mode, maskRgn) _CopyBits_(srcBits, dstBits, srcRect, dstRect, mode, maskRgn)
#define RectInRgn(r, rgn) _RectInRgn_(r, rgn)
#define EqualRgn(rgnA, rgnB) _EqualRgn_(rgnA, rgnB)
#define EmptyRgn(rgn) _EmptyRgn_(rgn)
#define FrameRgn(rgn) _FrameRgn_(rgn)
#define PaintRgn(rgn) _PaintRgn_(rgn)
#define EraseRgn(rgn) _EraseRgn_(rgn)
#define InvertRgn(rgn) _InvertRgn_(rgn)
#define FillRgn(rgn, pat) _FillRgn_(rgn, pat)
#define ScrollRect(r, dh, dv, updateRgn) _ScrollRect_(r, dh, dv, updateRgn)
#define CharToPixel(textBuf, textLen, slop, offset, direction, styleRunPosition, numer, denom) _CharToPixel_(textBuf, textLen, slop, offset, direction, styleRunPosition, numer, denom)
#define PixelToChar(textBuf, textLen, slop, pixelWidth, leadingEdge, widthRemaining, styleRunPosition, numer, denom) _PixelToChar_(textBuf, textLen, slop, pixelWidth, leadingEdge, widthRemaining, styleRunPosition, numer, denom)
#define GetClip(rgn) _GetClip_(rgn)
#define SetClip(rgn) _SetClip_(rgn)
#define UpdateGWorld(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, flags) _UpdateGWorld_(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, flags)
#define LockPixels(pm) _LockPixels_(pm)
#define GetGWorld(port, gdh) _GetGWorld_(port, gdh)
#define SetGWorld(port, gdh) _SetGWorld_(port, gdh)
#define HNoPurge(h) _HNoPurge_(h)
#define InlineGetHandleSize(h) _InlineGetHandleSize_(h)
#define MoveTo(h, v) _MoveTo_(h, v)
#define Move(dh, dv) _Move_(dh, dv)
#define Line(dh, dv) _Line_(dh, dv)
#define GetGWorldPixMap(offscreenGWorld) _GetGWorldPixMap_(offscreenGWorld)
#define UnlockPixels(pm) _UnlockPixels_(pm)
#define AllowPurgePixels(pm) _AllowPurgePixels_(pm)
#define EmptyRect(r) _EmptyRect_(r)
#define FrameRect(r) _FrameRect_(r)
#define QDError() _QDError_()
#endif

pascal short _CharType_(Ptr textBuf, short textOffset);
pascal StyledLineBreakCode _StyledLineBreak_(Ptr textPtr, long textLen, long textStart, long textEnd, long flags, Fixed *textWidth, long *textOffset);
pascal long _GetScriptVariable_(short script, short selector);
pascal Boolean _ParseTable_(CharByteTable table);
pascal void _GetIntlResourceTable_(ScriptCode script, short tableCode, Handle *itlHandle, long *offset, long *length);
pascal SInt8 _HGetState_(Handle h);
pascal void _HSetState_(Handle h, SInt8 flags);
pascal void _HLock_(Handle h);
pascal short _FontScript_(void);
pascal short _TextWidth_(const void *textBuf, short firstByte, short byteCount);
pascal short _CharWidth_(short ch);
pascal void _DrawJustified_(Ptr textPtr, long textLength, Fixed slop, JustStyleCode styleRunPosition, Point numer, Point denom);
pascal void _DrawText_(const void *textBuf,short firstByte,short byteCount);
pascal void _DrawChar_(short ch);
pascal long _Munger_(Handle h,long offset,const void *ptr1,long len1,const void *ptr2,long len2);
pascal OSErr _PtrAndHand_(const void *ptr1, Handle hand2, long size);
pascal RgnHandle _NewRgn_(void);
pascal Boolean _PtInRgn_(Point pt, RgnHandle rgn);
pascal void _DisposeRgn_(RgnHandle rgn);
pascal void _CopyRgn_(RgnHandle srcRgn, RgnHandle dstRgn);
pascal void _SetEmptyRgn_(RgnHandle rgn);
pascal void _SetRectRgn_(RgnHandle rgn, short left, short top, short right, short bottom);
pascal void _RectRgn_(RgnHandle rgn, const Rect *r);
pascal void _OffsetRgn_(RgnHandle rgn, short dh, short dv);
pascal void _InsetRgn_(RgnHandle rgn, short dh, short dv);
pascal void _SectRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
pascal void _UnionRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
pascal void _DiffRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
pascal void _XorRgn_(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
pascal void _CopyBits_(const BitMap *srcBits, const BitMap *dstBits, const Rect *srcRect, const Rect *dstRect, short mode, RgnHandle maskRgn);
pascal Boolean _RectInRgn_(const Rect *r, RgnHandle rgn);
pascal Boolean _EqualRgn_(RgnHandle rgnA, RgnHandle rgnB);
pascal Boolean _EmptyRgn_(RgnHandle rgn);
pascal void _FrameRgn_(RgnHandle rgn);
pascal void _PaintRgn_(RgnHandle rgn);
pascal void _EraseRgn_(RgnHandle rgn);
pascal void _InvertRgn_(RgnHandle rgn);
pascal void _FillRgn_(RgnHandle rgn, const Pattern *pat);
pascal void _ScrollRect_(const Rect *r, short dh, short dv, RgnHandle updateRgn);
pascal short _CharToPixel_ (Ptr textBuf, long textLen, Fixed slop, long offset, short direction, JustStyleCode styleRunPosition, Point numer, Point denom);
pascal short _PixelToChar_ (Ptr textBuf, long textLen, Fixed slop, Fixed pixelWidth, Boolean *leadingEdge, Fixed *widthRemaining, JustStyleCode styleRunPosition, Point numer, Point denom);
pascal void _GetClip_(RgnHandle rgn);
pascal void _SetClip_(RgnHandle rgn);
pascal GWorldFlags _UpdateGWorld_(GWorldPtr *offscreenGWorld, short pixelDepth, const Rect *boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags);
pascal Boolean _LockPixels_(PixMapHandle pm);
pascal void _GetGWorld_(CGrafPtr *port, GDHandle *gdh);
pascal void _SetGWorld_(CGrafPtr port, GDHandle gdh);
pascal void _HNoPurge_(Handle h);
pascal Size _InlineGetHandleSize_(Handle h);
pascal void _MoveTo_(short h,short v);
pascal void _Move_(short dh,short dv);
pascal void _Line_(short dh,short dv);
pascal PixMapHandle _GetGWorldPixMap_(GWorldPtr offscreenGWorld);
pascal void _UnlockPixels_(PixMapHandle pm);
pascal void _AllowPurgePixels_(PixMapHandle pm);
pascal Boolean _EmptyRect_(const Rect *r);
pascal void _FrameRect_(const Rect *r);
pascal short _QDError_(void);