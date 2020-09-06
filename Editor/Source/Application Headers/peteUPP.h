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
 * We're no longer using UPPs for PETE callbacks. (alb)
 */

/* UPP definitions for being called proc */
#define CallPETEHasBeenCalledProc(userRoutine, entry)		\
		(*(userRoutine))((entry))

/* UPP definitions for document change proc */
#define CallPETEDocHasChangedProc(userRoutine, ph, start, stop)		\
		(*(userRoutine))((ph), (start), (stop))

/* UPP definitions for set drag contents proc */
#define CallPETESetDragContentsProc(userRoutine, ph, theDragRef)		\
		(*(userRoutine))((ph), (theDragRef))

/* UPP definitions for get drag contents proc */
#define CallPETEGetDragContentsProc(userRoutine, ph, theText, theStyles, theParas, theDragRef, location)		\
		(*(userRoutine))((ph), (theText), (theStyles), (theParas), (theDragRef), (location))

/* UPP definitions for progress loop proc */
#define CallPETEProgressLoopProc(userRoutine)		\
		(*(userRoutine))()

/* UPP definitions for graphic handler proc */
#define CallPETEGraphicHandlerProc(userRoutine, ph, graphic, offset, message, data)		\
		(*(userRoutine))((ph), (graphic), (offset), (message), (data))

/* UPP definitions for word-break proc */
#define CallPETEWordBreakProc(userRoutine, ph, offset, leadingEdge, startOffset, endOffset, ws, charType)		\
		(*(userRoutine))((ph), (offset), (leadingEdge), (startOffset), (endOffset), (ws), (charType))

/* UPP definitions for intelligent cut proc */
#define CallPETEIntelligentCutProc(userRoutine, ph, start, stop)		\
		(*(userRoutine))((ph), (start), (stop))

/* UPP definitions for intelligent paste proc */
#define CallPETEIntelligentPasteProc(userRoutine, ph, offset, textScrap, styleScrap, paraScrap, added)		\
		(*(userRoutine))((ph), (offset), (textScrap), (styleScrap), (paraScrap), (added))