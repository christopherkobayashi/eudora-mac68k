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

#ifdef OLDHTML

#pragma segment HTML

#pragma optimization_level 1

short FindSTRNIndexCase(short resId,PStr string);
short FindSTRNIndexResCase(UHandle resource,PStr string);
void HiddenLinkStyle(PETEStyleListHandle slh);
OSErr HTMLColorParam(RGBColor *color, PStr text);

/**********************************************************************
 * HTML interpretation
 **********************************************************************/
#define htmlUnknown (0)
#define htmlText (-1)
#define htmlCR (-2)
#define htmlParamText (-3)
#define htmlDoubleLess (-4)
#define htmlLiteral (-5)
#define htmlIgnore (-6)
#define htmlStrayChar (-7)
#define htmlURL (-8)

/************************************************************************
 * PeteHTML - interpret HTML in a PETE record
 ************************************************************************/
OSErr PeteHTML(PETEHandle pte,long start,long stop,Boolean unwrap)
{
	//variables related to the current command
	short cmdId;
	long tStart,tStop;
	Boolean neg;
	short lastCmd;
	//interpreter state variables
	short noFill = 0;
	short inRich = 0;
	short inParam = 0;
	short	listType = -1;
	short	listItemNum = 0;
	short	specialChar = -1;
	long	position = -1;
	long	tempPos = -1;
	long	tempPos2 = -1;
	long	tempStart = -1;
	short	defList = 0;
	short	defDef = 0;
	short	doingList = 0;
	Boolean	doingPre = false;
	Boolean doingLink = false;
	Boolean doneWithTag;
	short sizeInc = 0;
	Boolean softBreak=0;
	short styleCounters[3];
	StackHandle fontStack=nil;
	StackHandle justStack=nil;
	StackHandle margStack=nil;
	StackHandle colorStack=nil;
	StackHandle	listStack = nil;
	StackHandle	listItemStack = nil;
	Str255 paramText;
	//global prefs
	//uLong validMask = ~GetPrefLong(PREF_INTERPRET_ENRICHED);
	uLong validMask = ~GetPrefLong(PREF_INTERPRET_HTML);
	//the text
	UHandle html=nil;
	long len;
	Uhandle text=nil;
	UHandle baseRef = nil;
	long baseLen = 0L;
	//scratch variables
	OSErr err = noErr;
	PETETextStyle style;
	Str255 scratch;
	PETEParaInfo pinfo;
	PeteSaneMargin marg, curMarg;
	PETEStyleEntry pse;
	unsigned char literal[5];
		
	/*
	 * initialize state
	 */
	styleCounters[0] = styleCounters[1] = styleCounters[2] = 0;
	if (!(err=StackInit(sizeof(style.tsFont),&fontStack)) &&
			!(err=StackInit(sizeof(pinfo.justification),&justStack)) &&
			!(err=StackInit(sizeof(PeteSaneMargin),&margStack)) &&
			!(err=StackInit(sizeof(RGBColor),&colorStack)) &&
			!(err=StackInit(sizeof(listType),&listStack)) &&
			!(err=StackInit(sizeof(listItemNum),&listItemStack)))
	{
		/*
		 * grab the html
		 */
		PETEGetRawText(PETE,pte,&text);
		len = stop ? stop-start : GetHandleSize_(text)-start;
		html = NuHTempBetter(len);
		if (!html) err = MemError();
		else
		{
			BMD(*text+start,*html,len);
			
			/*
			 * now remove the rich text from the edit record
			 */
			PeteDelete(pte,start,start+len);
			
			/*
			 * make sure we start at a paragraph
			 */
			PeteParaConvert(pte,start,start);
			PETESelect(PETE,pte,start,start);
			
			/*
			 * make the paragraph normal.
			 */
			Zero(marg);
			PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
			PETESetParaInfo(PETE,pte,-1,&pinfo,
				peStartMarginValid|peIndentValid|peEndMarginValid);
			
			/*
			 * operate on tokens from the html
			 */
			tStart = tStop = 0;
			while (!err && !(err=HTMLToken(html,&cmdId,&neg,&tStart,&tStop,&literal)))
			{
				if (lastCmd == htmlBaseRef)
					{
						baseLen = tStop - tStart - 1L;
						while (baseLen > 0L && *(*html + tStart +  baseLen) != '/') {
							--baseLen;
						}
						if(baseLen > 0L) {
							baseRef = NuHTempBetter(tStop - tStart - 2L);
							if(!baseRef) {
								err = MemError();
								break;
							}
							BMD(*html+tStart+1L, *baseRef, tStop - tStart - 2L);
						}
						cmdId = htmlIgnore;
					}
				if (lastCmd == htmlIgnore)
					cmdId = htmlStrayChar;

				CycleBalls();
				if (!inRich && (cmdId!=htmlTag  || neg) && cmdId != htmlBaseRef && cmdId != htmlIgnore && cmdId != htmlStrayChar) cmdId = htmlText;
				if (inParam && cmdId!=htmlParam) cmdId = htmlParamText;
				switch (cmdId)
				{
					case htmlTag:
					case htmlBody:
						if (neg) inRich = MAX(inRich-1,0);
						else inRich++;
						break;
					case htmlLinkStart:
							style.tsLabel = pLinkLabel;
							PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,&style,pLinkLabel << kPETELabelShift);
							PeteGetTextAndSelection(pte,nil,&start,nil);
							HiddenLinkStyle(Pslh);
							PETEInsertTextPtr(PETE,pte,-1L,"<",1L,Pslh);
							doingLink = true;
							break;
					case htmlBaseRef:
						break;
					case htmlATerminate:
								HiddenLinkStyle(Pslh);
								doneWithTag = false;
								if (!doingLink)
									break;
								tempPos = PeteFindString("\p/",start,pte);
								if (tempPos < 0 || (tempPos - start) != 1)
									{
										tempPos2 = PeteFindString("\p#",start,pte);
										tempPos = -1;
									}
								else if ((tempPos - start) == 1)
								{
									PeteDelete(pte,tempPos,tempPos + 1);
									if(baseRef != nil) {
										HLock(baseRef);
										PETEInsertTextPtr(PETE,pte,tempPos,*baseRef,baseLen,Pslh);
										HUnlock(baseRef);
									}
									doneWithTag = true;
								}
								if (tempPos < 0 && (tempPos2 - start) == 1 && !doneWithTag)
									{
									if(baseRef != nil) {
										HLock(baseRef);
										PETEInsertTextPtr(PETE,pte,tempPos2,*baseRef,GetHandleSize(baseRef),Pslh);
										HUnlock(baseRef);
									}
									doneWithTag = true;
									}
								if (!doneWithTag)
								{
									tempPos = PeteFindString("\p:",start,pte);
									if ((tempPos < 0) && (baseRef != nil)) {
										HLock(baseRef);
										PETEInsertTextPtr(PETE,pte,start+1,*baseRef,baseLen,Pslh);
										HUnlock(baseRef);
									}
								}
								style.tsLabel = 0;
								PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,&style,pLinkLabel << kPETELabelShift);
						break;
					case htmlCR:
						if (defDef)
							{
								// Set margin back
								softBreak = false;
								PeteEnsureCrAndBreak(pte,-1,nil);
								Zero(pinfo);
								StackPop(nil,margStack);	// drop top one
								if (StackTop(&marg,margStack)) Zero(marg);
								PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
								PETESetParaInfo(PETE,pte,-1,&pinfo,peStartMarginValid|peIndentValid|peEndMarginValid);
								defDef = MAX(defDef-1,0);
								//break;
							}						
							if (doingPre)
							{
									PeteInsertChar(pte,-1,'\015',nil);
									PeteGetTextAndSelection(pte,nil,&start,nil);
									PETEInsertParaBreak(PETE,pte,start);
									break;
	
							}
						if (doingList && StackTop(&listType,listStack) && !defDef)
							;
						/*else if (defList)
						{
								PeteInsertChar(pte,-1,' ',nil);
							//	softBreak = true;
						}*/
						else
						{
							if (unwrap && tStop-tStart==1 && !doingPre)
							{
								PeteInsertChar(pte,-1,' ',nil);
							}
						}
							/*else
							{
								if (unwrap) tStart++;
								if (softBreak) tStart++;
								for (;tStart<tStop;tStart++)
								{
									PeteInsertChar(pte,-1,'\015',nil);
									PeteGetTextAndSelection(pte,nil,&start,nil);
									PETEInsertParaBreak(PETE,pte,start);
									softBreak = False;
								}
							}*/
							break;
						
						
					case htmlDefList:
						if (neg) defList = MAX(defList-1,0);
						else defList++;

						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
	
						Zero(pinfo);
						if (neg) StackPop(nil,margStack);	// drop top one
						if (StackTop(&marg,margStack)) Zero(marg);
						if (!neg) StackPush(&marg,margStack);
						if (neg)
						{
							PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
							PETESetParaInfo(PETE,pte,-1,&pinfo,peStartMarginValid|peIndentValid|peEndMarginValid);
						}
						if (!neg)
						{
							Zero(pinfo);
							marg.second++; marg.first++;
							if (StackPop(&curMarg,margStack)) Zero(curMarg);
							marg.first += curMarg.first;
							marg.second += curMarg.second;
							marg.right += curMarg.right;
							PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
							StackPush(&marg,margStack);
							//PETESetParaInfo(PETE,pte,-1,&pinfo,
								//peStartMarginValid|peIndentValid|peEndMarginValid);
						}


						break;
						
					case htmlDefTerm:

						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);

						StackPop(&marg,margStack);	// drop top one
						/*PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);*/
						//softBreak = False;

						break;

					case htmlDefDef:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);


						if (!defDef)
						{
							Zero(pinfo);
							if (StackTop(&marg,margStack)) Zero(marg);
							StackPush(&marg,margStack);
							PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
							PETESetParaInfo(PETE,pte,-1,&pinfo,peStartMarginValid|peIndentValid|peEndMarginValid);
	
							Zero(pinfo);
							marg.second++; marg.first++;
							if (StackPop(&curMarg,margStack)) Zero(curMarg);
							marg.first += curMarg.first;
							marg.second += curMarg.second;
							marg.right += curMarg.right;
							PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
							StackPush(&marg,margStack);
							PETESetParaInfo(PETE,pte,-1,&pinfo,
								peStartMarginValid|peIndentValid|peEndMarginValid);
	
							defDef++;
						}
/*						PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);*/
						//softBreak = False;


						/*Zero(pinfo);
						//marg.second++; marg.first++;
						if (StackPop(&curMarg,margStack)) Zero(curMarg);
						marg.first += curMarg.first;
						marg.second += curMarg.second;
						marg.right += curMarg.right;
						PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
						StackPush(&marg,margStack);
						PETESetParaInfo(PETE,pte,-1,&pinfo,
							peStartMarginValid|peEndMarginValid);*/

						break;
					
					case htmlUnorderedList:
					case htmlOrderedList:
					case htmlMenu:
						if (neg) doingList = MAX(doingList-1,0);
						else doingList++;

						if (neg)
							{
								StackPop(nil,listStack);	// drop top one
								StackPop(nil,listItemStack);	// drop top one
							}
						else
							{
								listType = cmdId;
								StackPush(&listType,listStack);
								listItemNum = 0;
								StackPush(&listItemNum,listItemStack);

							}
								softBreak = false;
								PeteEnsureCrAndBreak(pte,-1,nil);
								PeteInsertChar(pte,-1,'\015',nil);
								PeteGetTextAndSelection(pte,nil,&start,nil);
								PETEInsertParaBreak(PETE,pte,start);
						break;
						
					case htmlListElement:
							softBreak = False;
							//PeteInsertChar(pte,-1,'\015',nil);
							PeteGetTextAndSelection(pte,nil,&start,nil);
							PETEInsertParaBreak(PETE,pte,start);

							StackPop(&listType,listStack);	
							StackPop(&listItemNum,listItemStack);	

							switch(listType) {
								case htmlUnorderedList :
									ComposeString(scratch, "\p� ");
									break;
								case htmlMenu :
									ComposeString(scratch,"\p- ");
									break;
								case htmlOrderedList :
									ComposeString(scratch,"\p%d. ",++listItemNum);
									break;
								default :
									*scratch = 0;
							}
								
							PETEInsertTextPtr(PETE,pte,-1,scratch + 1,*scratch,nil);
							StackPush(&listType,listStack);
							StackPush(&listItemNum,listItemStack);
						break;

					case htmlBR:
						softBreak = false;
						PeteInsertChar(pte,-1,'\015',nil);
						break;

					case htmlHR:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						break;

					case htmlP:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						break;

					case htmlHead:
					case htmlTitle:
						if (neg)
							{
								PeteGetTextAndSelection(pte,nil,&stop,nil);
								PeteDelete(pte,start,stop -1);
							}
						else
							PeteGetTextAndSelection(pte,nil,&start,nil);
						break;
						// fall through to htmlText if noFill
					case htmlURL:
						HiddenLinkStyle(Pslh);
					case htmlText:
						softBreak = False;
						LDRef(html);
						err = PETEInsertTextPtr(PETE,pte,-1,*html+tStart,tStop-tStart,cmdId == htmlURL ? Pslh : nil);
						UL(html);
 
						if(cmdId == htmlURL && !err) {
							err = PETEInsertTextPtr(PETE,pte,-1L,">",1L,Pslh);
						}
						break;
					case htmlLiteral:
						softBreak = False;
						err = PETEInsertTextPtr(PETE,pte,-1,&literal[1],literal[0],nil);
						break;
						
					case htmlDoubleLess:
						softBreak = False;
						err = PeteInsertChar(pte,-1,'<',nil);
						break;

					case htmlUUnderline:
						cmdId = htmlUnderline;
						goto DoStyle;
					case htmlEmphasis:
					case htmlStrong:
					case htmlCite:
					case htmlAddress:
					case htmlVar:
						if (cmdId == htmlStrong)
							cmdId = htmlBold;
						else
							cmdId = htmlItalic;
				DoStyle :
					case htmlUnderline:
					case htmlItalic:
					case htmlBold:
						if (neg)
							styleCounters[cmdId-1] = MAX(0,styleCounters[cmdId-1]-1);
						else
							styleCounters[cmdId-1]++;
						style.tsFace = styleCounters[cmdId-1] ? 1<<(cmdId-1) : 0;
						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,(1<<(cmdId-1))&validMask);
						break;
					case htmlColor:
						if (neg) StackPop(nil,colorStack);	// drop top one
						if (StackTop(&style.tsColor,colorStack))
						{
							PeteStyleAt(pte,kPETEDefaultStyle,&pse);
							style.tsColor = pse.psStyle.textStyle.tsColor;
						}
						if (!neg) StackPush(&style.tsColor,colorStack);
						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,peColorValid&validMask);
						break;
					case htmlSmaller:
					case htmlBigger:
						if (cmdId==htmlSmaller&&neg || cmdId==htmlBigger&&!neg)
							sizeInc++;
						else
							sizeInc--;
						style.tsSize = IncrementTextSize(FontSize,sizeInc);
						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,peSizeValid&validMask);
						break;

					case htmlh1:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							{
								styleCounters[htmlBold-1] = MAX(0,styleCounters[htmlBold-1]-1);
								sizeInc-=2;
								StackPop(nil,justStack);	// dump current one
								if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
							}
						else
							{
								styleCounters[htmlBold-1]++;
								sizeInc+=2;
								pinfo.justification = teCenter;
								StackPush(&pinfo.justification,justStack);
							}
						style.tsFace = styleCounters[htmlBold-1] ? 1<<(htmlBold-1) : 0;
						style.tsSize = IncrementTextSize(FontSize,sizeInc);


						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,((1<<(htmlBold-1)) | peSizeValid)&validMask);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);

						softBreak = false;
						PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,&stop);
						PETEInsertParaBreak(PETE,pte,start);

						break;

					case htmlh2:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							{
								styleCounters[htmlBold-1] = MAX(0,styleCounters[htmlBold-1]-1);
								sizeInc-=1;
								StackPop(nil,justStack);	// dump current one
								if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
							}
						else
							{
								styleCounters[htmlBold-1]++;
								sizeInc+=1;
								pinfo.justification = teFlushLeft;
								StackPush(&pinfo.justification,justStack);
							}
						style.tsFace = styleCounters[htmlBold-1] ? 1<<(htmlBold-1) : 0;
						style.tsSize = IncrementTextSize(FontSize,sizeInc);

						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,((1<<(htmlBold-1)) | peSizeValid)&validMask);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);

						softBreak = false;
						PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,&stop);
						PETEInsertParaBreak(PETE,pte,start);
						


						break;

					case htmlh3:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							{
								styleCounters[htmlBold-1] = MAX(0,styleCounters[htmlBold-1]-1);
								sizeInc-=1;
								StackPop(nil,justStack);	// dump current one
								if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
							}
						else
							{
								styleCounters[htmlBold-1]++;
								sizeInc+=1;
								pinfo.justification = teFlushLeft;
								StackPush(&pinfo.justification,justStack);
							}
						style.tsFace = styleCounters[htmlBold-1] ? 1<<(htmlBold-1) : 0;
						style.tsSize = IncrementTextSize(FontSize,sizeInc);

							
						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,((1<<(htmlBold-1)) | peSizeValid)&validMask);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);

						softBreak = false;
						PeteInsertChar(pte,-1,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						break;

					case htmlh4:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							{
								styleCounters[htmlBold-1] = MAX(0,styleCounters[htmlBold-1]-1);
								StackPop(nil,justStack);	// dump current one
								if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
							}
						else
							{
								styleCounters[htmlBold-1]++;
								pinfo.justification = teFlushLeft;
								StackPush(&pinfo.justification,justStack);
							}
						style.tsFace = styleCounters[htmlBold-1] ? 1<<(htmlBold-1) : 0;

						/* if (!neg)
						{
							PeteInsertChar(pte,-1,'\015',nil);
							PeteGetTextAndSelection(pte,nil,&start,&stop);
							PETEInsertParaBreak(PETE,pte,start);
							softBreak = False;
						}*/

						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,(1<<(htmlBold-1))&validMask);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);


						break;

					case htmlh5:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							{
								styleCounters[htmlItalic-1] = MAX(0,styleCounters[htmlItalic-1]-1);
								StackPop(nil,justStack);	// dump current one
								if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
							}
						else
							{
								styleCounters[htmlItalic-1]++;
								pinfo.justification = teFlushLeft;
								StackPush(&pinfo.justification,justStack);
							}
						style.tsFace = styleCounters[htmlItalic-1] ? 1<<(htmlItalic-1) : 0;

						/* if (!neg)
						{
							PeteInsertChar(pte,-1,'\015',nil);
							PeteGetTextAndSelection(pte,nil,&start,&stop);
							PETEInsertParaBreak(PETE,pte,start);
							softBreak = False;
						}*/

						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,(1<<(htmlItalic-1))&validMask);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);

						break;

					case htmlh6:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
							styleCounters[htmlBold-1] = MAX(0,styleCounters[htmlBold-1]-1);
						else
							styleCounters[htmlItalic-1]++;
						style.tsFace = styleCounters[htmlBold-1] ? 1<<(htmlBold-1) : 0;

						/* if (!neg)
						{
							PeteInsertChar(pte,-1,'\015',nil);
							PeteGetTextAndSelection(pte,nil,&start,&stop);
							PETEInsertParaBreak(PETE,pte,start);
							softBreak = False;
						}*/

						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,(1<<(htmlBold-1))&validMask);

						break;
					case htmlBlockQuote:
						softBreak = false;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg) StackPop(nil,margStack);	// drop top one
						if (StackTop(&marg,margStack)) Zero(marg);
						if (!neg) StackPush(&marg,margStack);
						PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peStartMarginValid|peIndentValid|peEndMarginValid);

						if (!neg)
						{
							Zero(pinfo);
							marg.second++; marg.first++;
							if (StackPop(&curMarg,margStack)) Zero(curMarg);
							marg.first += curMarg.first;
							marg.second += curMarg.second;
							marg.right += curMarg.right;
							PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
							StackPush(&marg,margStack);
							PETESetParaInfo(PETE,pte,-1,&pinfo,
								peStartMarginValid|peIndentValid|peEndMarginValid);
						}
						//else
						//{
							softBreak = false;
							PeteInsertChar(pte,-1,'\015',nil);
							PeteGetTextAndSelection(pte,nil,&start,nil);
							PETEInsertParaBreak(PETE,pte,start);

						//}

						break;
					
					/* Do stuff in a fixed width font */
					case htmlCode:
					case htmlKbd:
					case htmlSamp:
					case htmlTeleType:
						cmdId = htmlFixed;
					case htmlFont:
					case htmlFixed:
						//if (neg) doingPre = MAX(doingPre-1,0);
						//else doingPre++;
						if (cmdId != htmlTeleType && cmdId != htmlKbd)
							doingPre = !neg;

						if (neg)
						{
							StackPop(nil,fontStack);	//dump current one
							if (StackTop(&style.tsFont,fontStack)) style.tsFont = FontID;
						}
						else
						{
							if (cmdId==htmlFont)
							{
								if (StackTop(&style.tsFont,fontStack))
									style.tsFont = FontID;
							}
							else
								style.tsFont = GetFontID(GetRString(scratch,FIXED_FONT));
							StackPush(&style.tsFont,fontStack);
						}

						PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
													&style,peFontValid&validMask);
						break;
					case htmlNoFill:
						if (neg) noFill = MAX(noFill-1,0);
						else noFill++;
						break;
						
					case htmlLeft:
					case htmlCenter:
					case htmlRight:
						softBreak = 1;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg)
						{
							StackPop(nil,justStack);	// dump current one
							if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
						}
						else
						{
							switch(cmdId)
							{
								case htmlLeft: pinfo.justification = teFlushLeft; break;
								case htmlCenter: pinfo.justification = teCenter; break;
								case htmlRight: pinfo.justification = teFlushRight; break;
							}
							StackPush(&pinfo.justification,justStack);
						}
						PETESetParaInfo(PETE,pte,-1,&pinfo,peJustificationValid);
						break;
					
					case htmlPara:
						softBreak = 1;
						PeteEnsureCrAndBreak(pte,-1,nil);
						Zero(pinfo);
						if (neg) StackPop(nil,margStack);	// drop top one
						if (StackTop(&marg,margStack)) Zero(marg);
						if (!neg) StackPush(&marg,margStack);
						PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
						PETESetParaInfo(PETE,pte,-1,&pinfo,peStartMarginValid|peIndentValid|peEndMarginValid);
						break;

					case htmlParam:
						if (neg)
						{
							inParam = MAX(0,inParam-1);
							switch(lastCmd)
							{
								case htmlFont:
									style.tsFont=GetFontID(paramText);
									if (style.tsFont<=1)
									{
										PTr(paramText,"_"," ");
										style.tsFont = GetFontID(paramText);
									}
									if (style.tsFont>1)
									{
										// *replace* the top font with the new one
										StackPop(nil,fontStack);
										StackPush(&style.tsFont,fontStack);
										PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,peFontValid&validMask);
									}
									break;
								case htmlColor:
									if (!HTMLColorParam(&style.tsColor,paramText))
									{
										// *replace* the top color with the new one
										StackPop(nil,colorStack);
										StackPush(&style.tsColor,colorStack);
										PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,peColorValid&validMask);
									}
									break;
								case htmlPara:
									Zero(pinfo);
									if (!ParaIndent2Margin(&marg,paramText))
									{
										// *replace* the top margin with the new one
										if (StackPop(&curMarg,margStack)) Zero(curMarg);
										marg.first += curMarg.first;
										marg.second += curMarg.second;
										marg.right += curMarg.right;
										PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
										StackPush(&marg,margStack);
										PETESetParaInfo(PETE,pte,-1,&pinfo,
											peStartMarginValid|peIndentValid|peEndMarginValid);
									}
									break;
							}
						}
						else inParam++;
						break;
					case htmlParamText:
						MakePStr(scratch,*html+tStart,tStop-tStart);
						if(scratch[0]>0 && scratch[1]=='"' && scratch[scratch[0]]=='"') {
							scratch[1]=scratch[0]-1;
							PSCat(paramText,scratch+1);
						} else {
							PSCat(paramText,scratch);
						}
						break;
				}
				if (cmdId!=htmlParam && cmdId!=htmlParamText) lastCmd=cmdId;
				if (cmdId!=htmlParamText && !(cmdId==htmlParam && neg)) *paramText = 0;
			}
		}
	}
	ZapHandle(baseRef);
	ZapHandle(margStack);
	ZapHandle(fontStack);
	ZapHandle(justStack);
	ZapHandle(colorStack);
	ZapHandle(listStack);
	ZapHandle(listItemStack);
	ZapHandle(html);
	return err==eofErr ? noErr : err;
}

/**********************************************************************
 * HTMLToken - get the next token from a html stream
 **********************************************************************/
OSErr HTMLToken(UHandle html,short *cmdId,Boolean *neg,long *tStart,long *tStop, unsigned char *literal)
{
	UPtr start,stop,end,tempStop;
	Str63 cmd;
	long len = GetHandleSize(html), charval;
	
	LDRef(html);
	
	*tStart = *tStop;
	start = *html+*tStart;
	end = *html+len;
	
	// end of text?
	if (*tStart>=len) {UL(html);return(eofErr);}
	
	// carriage return?
	if (*start=='\015')
	{
		for (stop=start+1;stop<end && *stop=='\015';stop++);
		*cmdId = htmlCR;
	}
	// character entity?
	else if ((*start == '&') && (start+1<end) &&
	         (isalpha(*(start + 1)) ||
	          ((start+2<end) && ((*(start+1)=='#') && isdigit(*(start + 2))))))
	{
			*cmdId = htmlLiteral;
			if(isalpha(*(start + 1))) {
				for (stop = start+2; stop<end && (isalpha(*stop) || isdigit(*stop)); ++stop);
				MakePStr(cmd, start + 1, stop - start - 1);
				charval = FindSTRNIndexCase(HTMLLiteralsStrn, cmd);
			} else if((*(start + 1) == '#')) {
				for (stop = start+3; stop<end && isdigit(*stop); ++stop);
				MakePStr(cmd, start + 2, stop - start - 2);
				StringToNum(cmd, &charval);
			}
			literal[0] = 4;
			while((literal[0] > 0) && (*((unsigned char *)&charval) == 0)) {
				charval <<= 8;
				--literal[0];
			}
			if(*stop == ';') ++stop;
			MakePStr(literal, (unsigned char *)&charval, literal[0]);
			TransLitRes(&literal[1], literal[0], TRANS_IN_TABL);
	}
	// other stuff
	else if (*cmdId == htmlLinkStart)
	{
		*cmdId = htmlURL;
		if(*start!='"')
		{
			for (; start<end && *start!='"' && *start!='>' && !isalpha(*start) && !isdigit(*start); ++start, ++*tStart);
		}

		if(*start=='"')
		{
			++start;
			++*tStart;
			for(stop=start; stop<end && *stop!='"'; ++stop) ;
		}
		else
		{
			if(*start!='>')
			{
				for(stop=start+1; stop<end && *stop!=' ' && *stop!='>' && *stop!=';'; ++stop) ;
			}
			else
			{
				stop=start;
			}
		}
	}
	else if (*cmdId == htmlURL)
	{
		for(stop=start; stop<end && (*stop!='>' || (++stop, false)); ++stop);
		*cmdId = htmlStrayChar;
	}
// some random text
	else
	{
//		for (stop=start+1; stop<end && *stop!='<' && *stop!='>' && *stop!='\015' && *stop!='=' && *stop!='&' && *stop != ';' ;stop++);
		for (stop=start+1; stop<end && *stop!='<' && *stop!='>'  && *stop!='=' && *stop!='&' && *stop != ';' ;stop++);
		if (*stop=='=')
			{

				MakePStr(cmd,start+1,stop-start-1);
				stop++;
				*cmdId = FindSTRNIndex(HTMLTagsStrn,cmd);
				if (!*cmdId)
//					for (stop=stop-1; stop<end && *stop!='<' && *stop!='>' && *stop!='\015';stop++);
					for (stop=stop-1; stop<end && *stop!='<' && *stop!='>';stop++);
				else
				{
					*tStop = stop-*html;
					UL(html);
					return(noErr);
				}
			}

		if (*start=='<')
		{
			// "<<"
			if (*stop=='<' && stop-start==1)
			{
				stop++;
				*cmdId = htmlDoubleLess;
			}
			// a <>-delimited-thing
			else if (*stop=='>')
			{
				*neg = start[1]=='/';
				if (*neg) MakePStr(cmd,start+2,stop-start-2);
				else MakePStr(cmd,start+1,stop-start-1);
				stop++;
				*cmdId = FindSTRNIndex(HTMLTagsStrn,cmd);
				if (!*cmdId) // Strip off the extra baloney parameters
				{
						for (tempStop=start+1; tempStop<stop - 1 && *tempStop!=' ';tempStop++);
						MakePStr(cmd,start+1,tempStop-start-1);
						*cmdId = FindSTRNIndex(HTMLTagsStrn,cmd);
						//if (!*cmdId)
							//	*cmdId = htmlText;

				}
			}
		}
		else
			{
				for (stop=start+1; stop<end && *stop!='<' && *stop!='>' && *stop!='\015' && *stop!='=' && *stop!='&' && *stop != ';' ;stop++);
				*cmdId = htmlText;
			}
	}
	*tStop = stop-*html;
	UL(html);
	return(noErr);
}

void HiddenLinkStyle(PETEStyleListHandle slh)
{
	PETEStyleEntry pse;
	
	pse.psStartChar = 0L;
	pse.psGraphic = true;
	pse.psStyle.graphicStyle.tsFont = kPETEDefaultFont;
	pse.psStyle.graphicStyle.tsFace = 0;
	pse.psStyle.graphicStyle.filler = 0;
	pse.psStyle.graphicStyle.tsSize = kPETEDefaultSize;
	pse.psStyle.graphicStyle.graphicInfo = nil;
	pse.psStyle.graphicStyle.filler0 = 0;
	pse.psStyle.graphicStyle.tsLang = 0;
	pse.psStyle.graphicStyle.tsLock = 0;
	pse.psStyle.graphicStyle.tsLabel = pLinkLabel;
	**slh = pse;
}

OSErr HTMLColorParam(RGBColor *color, PStr text)
{
	if((text[0]==7) && text[1]=='#')
	{
		color->red = color->green = color->blue = 0;
		Hex2Bytes(&text[2],2,(void*)&color->red);
		Hex2Bytes(&text[4],2,(void*)&color->green);
		Hex2Bytes(&text[6],2,(void*)&color->blue);
		return noErr;
	}
	else
	{
		return fnfErr;
	}
}

#endif