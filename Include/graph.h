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

#ifndef GRAPH_H
#define GRAPH_H

typedef enum
{
	kDefaultGraph,
	kLineGraph,
	kAreaGraph,
	kBarGraph,
	kCircleGraph,
	kDiamondGraph,
	kSquareGraph,
	kPieGraph
} GraphType;

typedef struct
{
	GraphType type;	// graph type, use default if zero
	uLong *data;	// pointer to data series
	RGBColor color;	// color for data points
	short dataCount; // number of valid points for this series
	StringPtr label; // legend label
	short divider;	// divide all values by this, 0 if don't modify
} SeriesInfo;

typedef struct
{
	Rect bounds;		// where to draw
	GraphType type;		// default graph type
	Handle xLabels;		// optional x-axis labels
	short seriesCount;	// how many data series
	short dataCount;	// # of data points in each series
	SeriesInfo *series;	// series info
	uLong divider;		// divide all values by this
	short decPlace;		// decimal places
	short legend;		// legend location, 0=none
	short largeTickInterval;	// do large ticks on x-axis
	short medTickInterval;	// do medium ticks on x-axis
} GraphData;


void DrawGraph(GraphData *data);

#endif //GRAPH_H
