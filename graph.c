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

//#include "miscstuff.h"
#include "graph.h"
#define FILE_NUM 133
/* Copyright (c) 2000 by Qualcomm, Inc */

enum
{
	kBorderWidth = 12,	// blank border around outside
	kMinYHt = 12,
	kXAxisHt = 20,
	kDotSize = 8,	// height/width of circle, diamond, or square graph plot
	kLegendBoxHeight = 12,
	kLegendBoxWidth = 16,
	kLegendLineHt = 24,
	kLegendTextSpace = 4,
	kYAxisWidth = 25,
	kTickSize = 5
};

static uLong GetMaxDataValue(short seriesCount,SeriesInfo *series,short dataCount);
static void GetYAxisInfo(uLong maxValue,short maxGrids,short *yUnits,short *yGrids);
static void DrawYGrids(Rect *rBounds,short yUnits,short yGrids,short decPlaces);
static void DrawXGrids(Rect *rBounds,GraphData *data);
static short GetYValue(uLong value,Rect *rBounds,uLong yUnits,uLong yGrids);
void DrawPieGraph(GraphData *data,Rect *rBounds);
static void DrawLegend(GraphData *data, Rect *rBounds,short top);

/************************************************************************
 * DrawGraph - draw a graph
 ************************************************************************/
void DrawGraph(GraphData *data)
{
	uLong	maxValue;
	Rect	rBounds;
	Str255	s;
	short	yUnits,yGrids;
	short	seriesIdx;
	GraphType theType;
	uLong	divider = 1;
	short	barCount,barIdx,barWidth,width;
	short	xWidth,barMargin,yLegend;
	
	if (UseThemeFont)
		UseThemeFont(kThemeSmallSystemFont,smCurrentScript);
	else
	{
		TextFont(kFontIDGeneva);
		TextSize(9);
	}
	TextFace(0);
	
	rBounds = data->bounds;
	InsetRect(&rBounds,kBorderWidth,kBorderWidth);
	if (data->legend)
	{
		short	width,maxLabelWidth = 0;
		
		for(seriesIdx=0;seriesIdx<data->seriesCount;seriesIdx++)
		{
			width = StringWidth(data->series[seriesIdx].label);
			if (width > maxLabelWidth) maxLabelWidth = width;
		}
		width += kBorderWidth+kLegendTextSpace+kLegendBoxWidth;
		if (width < GetRLong(STAT_LEGEND_WIDTH)) width = GetRLong(STAT_LEGEND_WIDTH);
		rBounds.right -= width;
	}

	yLegend = rBounds.top+kBorderWidth;
	theType = data->series[seriesIdx].type ? data->series[seriesIdx].type : data->type;
	if (data->type == kPieGraph || data->series[0].type == kPieGraph)
	{
		short	size,ht,wi;
		ht = RectHi(rBounds);
		wi = RectWi(rBounds);
		size = MIN(ht,wi);
		rBounds.right = rBounds.left + size;
		rBounds.bottom = rBounds.top + size;
		DrawPieGraph(data,&rBounds);
	}
	else
	{	
		rBounds.bottom -= kXAxisHt;

		// get limits, etc
		maxValue = GetMaxDataValue(data->seriesCount,data->series,data->dataCount);	
		if (data->divider)
		{
			maxValue = (maxValue+data->divider-1) / data->divider;
			divider = data->divider;
		}
		
		NumToString(maxValue,s);
		if (data->decPlace)
		{
			short	decCount;
			char	cDecPoint = GetIntlNumberPart(tokDecPoint);
			
			if (cDecPoint) PSCatC(s,cDecPoint);
			for(decCount=data->decPlace;decCount;decCount--)
				PSCatC(s,'0');
			
		}
		width = StringWidth(s) + 8;
		width = MAX(width,kYAxisWidth);
		rBounds.left += width;

		GetYAxisInfo(maxValue,(rBounds.bottom-rBounds.top)/kMinYHt,&yUnits,&yGrids);
		
		SetForeGrey(49344);
		PaintRect(&rBounds);
		SetForeGrey(0);
		DrawYGrids(&rBounds,yUnits,yGrids,data->decPlace);
		DrawXGrids(&rBounds,data);
		xWidth = rBounds.right-rBounds.left;

		// how many bar graphs?
		barCount = 0;
		for(seriesIdx=0;seriesIdx<data->seriesCount;seriesIdx++)
			if ((data->series[seriesIdx].type ? data->series[seriesIdx].type : data->type) == kBarGraph)
				barCount++;
		if (barCount)
		{
			barWidth = xWidth/data->dataCount;
			barMargin = barWidth/8;
	//		if (!barMargin) barMargin = 1;
			barWidth -= barMargin*2;
			barWidth /= barCount;
			if (!barWidth) barWidth = 1;
			barIdx=0;	
		}

		if (data->decPlace && divider>10 && yUnits<10)
		{
			// get a little more precision
			yUnits *= 10;
			divider /= 10;
		}
		

		// plot the series
		for(seriesIdx=0;seriesIdx<data->seriesCount;seriesIdx++)
		{
			short	i;
			uLong *pData;
			short	x1,x2,x,y,firstX;
			Rect	rBar,rDot;
			uLong	dataValue,lastData;
			PolyHandle	poly=nil;
			short	dataCount;
			uLong	useYUnits;
			
			theType = data->series[seriesIdx].type ? data->series[seriesIdx].type : data->type;
			if (theType == kLineGraph) PenSize(2,2);	// fat lines
			else PenSize(1,1);
			pData = data->series[seriesIdx].data;
			x1 = rBounds.left;
			RGBForeColor(&data->series[seriesIdx].color);
			lastData = 0;
			if (theType==kAreaGraph) poly=OpenPoly();
			dataCount = MIN(data->dataCount,data->series[seriesIdx].dataCount);
			for(i=0;i<dataCount;i++)
			{
				dataValue = pData[i];
				useYUnits = yUnits;
				if (divider != 1) useYUnits *= divider;
				if (data->series[seriesIdx].divider) useYUnits *= data->series[seriesIdx].divider;
				y = GetYValue(dataValue,&rBounds,useYUnits,yGrids);
				x2 = rBounds.left + (i+1)*xWidth/data->dataCount;
				x = (x1+x2)/2;
				SetRect(&rDot,x-kDotSize/2,y-kDotSize/2,x+kDotSize/2,y+kDotSize/2);
				switch (theType)
				{
					case kBarGraph:
						if (dataValue)
						{
							short	xLeft = x1+barMargin+barIdx*barWidth;
							
							SetRect(&rBar,xLeft,y,xLeft+barWidth,rBounds.bottom+1);
							PaintRect(&rBar);
							SetForeGrey(0);
							if (barWidth > 3)
							{
								rBar.right++;
								FrameRect(&rBar);
							}
							RGBForeColor(&data->series[seriesIdx].color);
						}
						break;

					case kLineGraph:
						if (i && (lastData || dataValue)) LineTo(x,y);
						else
						{ 
							if (dataCount>1)
								MoveTo(x,y);
							else
							{
								//	Only one data item. Give it some width
								MoveTo(x1,y);
								LineTo(x,y);
							}
						}
						lastData = dataValue;
						break;				

					case kAreaGraph:
						if (i)
							LineTo(x,y);
						else
						{
							firstX = x;
							if (dataCount>1)
								MoveTo(x,y);
							else
							{
								//	Only one data item. Give it some width
								MoveTo(x1,y);
								LineTo(x,y);
								firstX = x1;
							}
						}
						break;
								
					case kCircleGraph:
						if (dataValue)
							PaintOval(&rDot);
						break;
						
					case kDiamondGraph:
						if (dataValue)
						{
							poly=OpenPoly();
							MoveTo(rDot.left,y);
							LineTo(x,rDot.top);
							LineTo(rDot.right,y);
							LineTo(x,rDot.bottom);
							ClosePoly();
							PaintPoly(poly);
							KillPoly(poly);
						}
						break;
						
					case kSquareGraph:
						if (dataValue)
							PaintRect(&rDot);
						break;
				}
				x1 = x2;	
			}
			if (theType==kBarGraph) barIdx++;
			else if (theType==kAreaGraph)
			{
				LineTo(x,rBounds.bottom);
				LineTo(firstX,rBounds.bottom);
				ClosePoly();
				PaintPoly(poly);
				KillPoly(poly);
			}
		}
	}
		
	if (data->legend)
		DrawLegend(data,&rBounds,yLegend);

	PenNormal();
}


/************************************************************************
 * DrawLegend - draw legend
 ************************************************************************/
static void DrawLegend(GraphData *data, Rect *rBounds,short top)
{
	Rect	rColor,rDot;
	PolyHandle	poly=nil;
	enum { kArea0=6,kArea1=11,kArea2=8,kArea3=12,kArea4=7 };
	short	seriesIdx;
	
	PenNormal();
	for(seriesIdx=0;seriesIdx<data->seriesCount;seriesIdx++)
	{
		SetRect(&rColor,rBounds->right+kBorderWidth,top,rBounds->right+kBorderWidth+kLegendBoxWidth,top+kLegendBoxHeight);
		SetRect(&rDot,rBounds->right+kBorderWidth+(kLegendBoxWidth-kDotSize)/2,top+(kLegendBoxHeight-kDotSize)/2,
			rBounds->right+kBorderWidth+(kLegendBoxWidth+kDotSize)/2,top+(kLegendBoxHeight+kDotSize)/2);
		RGBForeColor(&data->series[seriesIdx].color);
		switch (data->series[seriesIdx].type ? data->series[seriesIdx].type : data->type)
		{
			case kBarGraph:
			case kPieGraph:
				PaintRect(&rColor);
				SetForeGrey(0);
				FrameRect(&rColor);
				break;

			case kLineGraph:
				MoveTo(rColor.left,rColor.top+kLegendBoxHeight/2);
				PenSize(2,2);
				LineTo(rColor.right-1,rColor.top+kLegendBoxHeight/2);
				break;
						
			case kAreaGraph:
				poly=OpenPoly();
				MoveTo(rColor.left,rColor.bottom);
				LineTo(rColor.left,rColor.bottom-kArea0);
				LineTo(rColor.left+kLegendBoxWidth/4,rColor.bottom-kArea1);
				LineTo(rColor.left+kLegendBoxWidth/2,rColor.bottom-kArea2);
				LineTo(rColor.left+3*kLegendBoxWidth/4,rColor.bottom-kArea3);
				LineTo(rColor.right,rColor.bottom-kArea4);
				LineTo(rColor.right,rColor.bottom);
				LineTo(rColor.left,rColor.bottom);
				ClosePoly();
				PaintPoly(poly);
				SetForeGrey(0);
				FramePoly(poly);
				KillPoly(poly);
				break;
				
			case kCircleGraph:
				PaintOval(&rDot);
				break;
				
			case kDiamondGraph:
				poly=OpenPoly();
				MoveTo(rDot.left,rDot.top+kDotSize/2);
				LineTo(rDot.left+kDotSize/2,rDot.top);
				LineTo(rDot.right,rDot.top+kDotSize/2);
				LineTo(rDot.left+kDotSize/2,rDot.bottom);
				ClosePoly();
				PaintPoly(poly);
				KillPoly(poly);
				break;
				
			case kSquareGraph:
				PaintRect(&rDot);
				break;
		}

		PenNormal();
		SetForeGrey(0);
		MoveTo(rColor.right+kLegendTextSpace,rColor.bottom-3);
		DrawString(data->series[seriesIdx].label);
		top += kLegendLineHt;
	}
}

/************************************************************************
 * DrawGraph - draw a pie graph
 ************************************************************************/
void DrawPieGraph(GraphData *data,Rect *rBounds)
{
	uLong	sum = 0,total = 0,startAngle,arcAngle;
	short	i,dataCount;
	
	// get sum of all the data in this series
	for(i=0;i<data->dataCount;i++)
		total += data->series[0].data[i];
	
	dataCount = MIN(data->dataCount,data->series[0].dataCount);
	for(i=0;i<dataCount;i++)
	{
		RGBForeColor(&data->series[i].color);
		startAngle = sum*360L/total;
		if (i==dataCount-1)
			// last wedge, make sure it fits
			arcAngle = 360-startAngle;
		else
			arcAngle = data->series[0].data[i]*360L/total;
		PaintArc(rBounds,startAngle,arcAngle);
		sum += data->series[0].data[i];
	}
	SetForeGrey(0);
	FrameOval(rBounds);
}

/************************************************************************
 * GetMaxDataValue - get the maximum value we need to plot
 ************************************************************************/
static uLong GetMaxDataValue(short seriesCount,SeriesInfo *series,short dataCount)
{
	short	seriesIdx;
	uLong	max=0;
	
	for(seriesIdx=0;seriesIdx<seriesCount;seriesIdx++)
	{
		short	i;
		uLong *pData;
		uLong	seriesMax;
		
		seriesMax = 0;
		pData = series[seriesIdx].data;
		for(i=0;i<dataCount;i++)
			if (pData[i] > seriesMax)
				seriesMax = pData[i];
		if (series[seriesIdx].divider)
			seriesMax = (seriesMax + series[seriesIdx].divider - 1) / series[seriesIdx].divider;
		
		if (seriesMax > max) max = seriesMax;
	}
	return max;
}

/************************************************************************
 * GetYAxisInfo - get Y axis, grid info
 ************************************************************************/
static void GetYAxisInfo(uLong maxValue,short maxGrids,short *yUnits,short *yGrids)
{
	short	tens=1;
	short	rawScale;
	short	scale;
	uLong	max = maxValue;
	
	if (maxValue < 2) maxValue = 2;
	while (max > 10*maxGrids)
	{
		max/= 10;
		tens *= 10;
	}
	if (max) max--;
	rawScale = max/maxGrids;
	if (rawScale >= 5) scale = 10;
	else if (rawScale >= 2) scale = 5;
	else if (rawScale >= 1) scale = 2;
	else scale = 1;
	*yUnits = scale*tens;
	*yGrids = (maxValue+*yUnits-1)/(*yUnits);
}

/************************************************************************
 * DrawYGrids - draw horizontal grids
 ************************************************************************/
static void DrawYGrids(Rect *rBounds,short yUnits,short yGrids,short decPlaces)
{
	short	y,grid;
	short	i;
	Str32	s;
	char	cDecPoint;
	
	if (decPlaces) cDecPoint = GetIntlNumberPart(tokDecPoint);
	
	for(i=0,grid=0;i<=yGrids;i++,grid+=yUnits)
	{
		y = GetYValue(grid,rBounds,yUnits,yGrids);
		MoveTo(rBounds->left-kTickSize,y);
		LineTo(rBounds->right-1,y);
		NumToString(grid,s);
		if (decPlaces && grid)
		{
			if (*s==1)
				PInsertC(s,sizeof(s),'0',s+1);
			if (cDecPoint) 
				PInsertC(s,sizeof(s),cDecPoint,s+*s-decPlaces+1);
		}
		MoveTo(rBounds->left-StringWidth(s)-12,y+4);
		DrawString(s);
	}
}

/************************************************************************
 * DrawXGrids - draw x-axis tick marks and labels (if any)
 ************************************************************************/
static void DrawXGrids(Rect *rBounds,GraphData *data)
{
	short	x,i,lastX,lastPen,thisPen;
	short	xWidth = rBounds->right-rBounds->left-1;
	UPtr	sPtr;
	short	tickHeight;
	
	lastPen = 0;
	if (data->xLabels)
		sPtr = LDRef(data->xLabels) + sizeof(short);
	for(i=0;i<=data->dataCount;i++)
	{
		x = rBounds->left + i*xWidth/data->dataCount;

		// draw tick mark
		if (i==0 || i==data->dataCount)
			MoveTo(x,rBounds->top);
		else
			MoveTo(x,rBounds->bottom);
		tickHeight = kTickSize;
		if (data->largeTickInterval && (i%data->largeTickInterval==0))
			tickHeight += data->largeTickInterval;
		else if (data->medTickInterval && (i%data->medTickInterval==0))
			tickHeight += data->medTickInterval;
		LineTo(x,rBounds->bottom+tickHeight);
		
		// draw labels
		if (i)
		{
			if (data->xLabels)
			{
				short	sWd = StringWidth(sPtr);
				
				if (*sPtr)
				{
					// center between ticks if enough room, else draw close to left tick
					thisPen = sWd < x-lastX || !isdigit(sPtr[1]) ? (x+lastX-sWd+2)/2 : x-sWd+1;
					if (thisPen > lastPen && *sPtr)
					{
						MoveTo(thisPen,rBounds->bottom+kXAxisHt);
						DrawString(sPtr);
						lastPen = thisPen + sWd + 1;
					}
				}
				sPtr += *sPtr + 1;
			}
		}
		
		lastX = x;
	}
	if (data->xLabels) UL(data->xLabels);
}

/************************************************************************
 * GetYValue - where do we plot this value?
 ************************************************************************/
static short GetYValue(uLong value,Rect *rBounds,uLong yUnits,uLong yGrids)
{
	return rBounds->bottom - value*((uLong)(rBounds->bottom-rBounds->top))/(yUnits*yGrids);
}