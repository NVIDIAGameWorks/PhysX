//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

// Mouse filter from ICE, refactored a bit

#include "SampleMouseFilter.h"

MouseFilter::MouseFilter() :
	mWeightModifier			(0.0f),
	mHistoryBufferLength	(0),
	mHistoryBufferX			(NULL),
	mHistoryBufferY			(NULL)
{
}

MouseFilter::MouseFilter(PxU32 length, PxReal weightModifier)
{
	mWeightModifier			= weightModifier;
	mHistoryBufferLength	= 0;
	mHistoryBufferX			= NULL;
	mHistoryBufferY			= NULL;
	SetHistoryBufferLength(length);
}

MouseFilter::~MouseFilter()
{
	SAMPLE_FREE(mHistoryBufferX);
	SAMPLE_FREE(mHistoryBufferY);
}

bool MouseFilter::SetHistoryBufferLength(PxU32 length)
{
	SAMPLE_FREE(mHistoryBufferX);
	SAMPLE_FREE(mHistoryBufferY);

	mHistoryBufferLength = length;
	if(length)
	{
		mHistoryBufferX = (float*)SAMPLE_ALLOC(sizeof(float)*length);
        memset(mHistoryBufferX, 0, length*sizeof(float)); 
		mHistoryBufferY = (float*)SAMPLE_ALLOC(sizeof(float)*length);
        memset(mHistoryBufferY, 0, length*sizeof(float));		
	}
	return true;
}

void MouseFilter::Apply(PxReal& deltaMouseX, PxReal& deltaMouseY)
{
	// Checkings
	if(!mHistoryBufferX || !mHistoryBufferY)	return;

	// Shift the buffer around. If you want performance from this, be sure
	// to use a circular buffer than these slow memmove()s.
	memmove(mHistoryBufferX+1, mHistoryBufferX, (mHistoryBufferLength-1)*sizeof(PxReal));
	memmove(mHistoryBufferY+1, mHistoryBufferY, (mHistoryBufferLength-1)*sizeof(PxReal));

	// Put the current values at the front of the history buffer
	*mHistoryBufferX = deltaMouseX;
	*mHistoryBufferY = deltaMouseY;

	// Filter the mouse
	PxReal CurAverageX	= 0.0f;
	PxReal CurAverageY	= 0.0f;
	PxReal AverageTot	= 0.0f;
	PxReal CurrentWeight	= 1.0f;
	for(PxU32 i=0;i<mHistoryBufferLength;i++)
	{
		CurAverageX += mHistoryBufferX[i] * CurrentWeight;
		CurAverageY += mHistoryBufferY[i] * CurrentWeight;

		// Note! Our total is also weighted
		AverageTot += 1.0f * CurrentWeight;

		// The weight for the next entry in the history buffer
		CurrentWeight *= mWeightModifier;
	}

	// Calculate the final weighted value
	deltaMouseX = CurAverageX / AverageTot;
	deltaMouseY = CurAverageY / AverageTot;
}
