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

#ifndef PRINT_H
#define PRINT_H

#include <PMCore.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for printing
 ************************************************************************/
int PrintOneMessage(MyWindowPtr win,Boolean select,Boolean now);
int PrintSelectedMessages(TOCHandle tocH,Boolean select,Boolean now, long beginSel, long endSel, PETEHandle printMe);
void PrintMessageHeader(UPtr title, short pageNum, short height, short bottom, short left, short right);
void PrintBottomHeader(short pageNum,Rect *uRect);
void GetURect(Rect *useRect);
void SetupPrintFont(void);
OSErr PrintPreamble(PMPrintContext *printContext, Rect *uRect,Boolean now);
OSErr PrintCleanup(PMPrintContext printContext,OSErr err);
OSErr PrintYield(void);
int PrintClosedMessage(TOCHandle tocH,short sumNum,Boolean now);
void DoPageSetup(void);

#ifdef	GX_PRINTING
typedef struct MySpoolDataRec {
		gxRectangle		pageArea;					// Page rectangle.
		gxViewPort		printViewPort;		// ViewPort we're printing with.
		Boolean				printThisShape;		// indicates whether we should actually draw the shape
} MySpoolDataRec, *MySpoolDataPtr;

//Initialization and cleanup
void 	InitGXPrinting(void);
void 	CleanUpGXPrinting(void);

//Handling the GXPageSetup gxJob
OSErr GetGXPageSetup(void);
OSErr SaveGXPageSetup(void);
OSErr UpdateGXPageSetup(void);

//QDGX Print and Page Setup dialogs
OSErr GXPrintingEventOverride(EventRecord *anEvent, Boolean filterEvent);
void	DisableMenusForGXDialogs(void);
OSErr GXPrintPreamble(gxRectangle *GXURect, Str255 docTitle, long *firstPage, long *numPages, Boolean now);
void	GXGetURect(gxRectangle *useRect);

//Actual printing
int 	GXPrintSelectedMessages(TOCHandle tocH, Boolean select, Boolean now, long beginSel, long endSel);
void 	MyGXInstallQDTranslator(CGrafPtr port, gxRectangle *pageArea, gxViewPort *viewPort, Boolean printThisPage);
void 	MyGXRemoveQDTranslator(CGrafPtr port);
OSErr MyPrintAShape(gxShape currentShape, long refCon);
OSErr GXPrintCleanup(void);

//nasty fixed to int conversion routines
void  GXRectToRect(gxRectangle *theGXRect, Rect *theRect);
#endif	//GX_PRINTING

#endif
