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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "Apex.h"
#include "variable_oscillator.h"
#include "PxMath.h"

namespace nvidia
{
namespace apex
{

variableOscillator::variableOscillator(float min, float max, float initial, float period) :
	mMin(min),
	mMax(max),
	mPeriod(period),
	mStartVal(initial),
	mLastVal(initial)
{
	mCumTime = 0.0f;
	mGoingUp = true;
	mEndVal = computeEndVal(mMin, mMax);
}


variableOscillator::~variableOscillator()
{
}

float variableOscillator::computeEndVal(float current, float maxOrMin)
{
	float target;
	float maxDelta;
	float quarterVal;

	// compute the max range of the oscillator
	maxDelta = maxOrMin - current;
	// find the 'lower bound' of the oscillator peak
	quarterVal = current + (maxDelta / 4.0f);
	// get a rand between 0 and 1
	target = (float) ::rand() / (float) RAND_MAX;
	// scale the rand to the range we want
	target = target * PxAbs(quarterVal - maxOrMin);
	// add the offset to the scaled random number.
	if (current < maxOrMin)
	{
		target = target + quarterVal;
	}
	else
	{
		target = quarterVal - target;
	}
	return(target);
}

float variableOscillator::updateVariableOscillator(float deltaTime)
{
	float returnVal;
	float halfRange;

	mCumTime += deltaTime;

	// has the function crossed a max or a min?
	if ((mGoingUp  && (mCumTime > (mPeriod / 2.0f))) ||
	        (!mGoingUp && (mCumTime > mPeriod)))
	{
		mStartVal = mLastVal;
		if (mGoingUp)
		{
			mEndVal = computeEndVal(mStartVal, mMin);
		}
		else
		{
			mEndVal = computeEndVal(mStartVal, mMax);
			mCumTime = mCumTime - mPeriod;
		}
		mGoingUp = !mGoingUp;
	}
	halfRange = 0.5f * PxAbs(mEndVal - mStartVal);
	returnVal = -halfRange * PxCos(mCumTime * PxTwoPi / mPeriod) + halfRange + PxMin(mStartVal, mEndVal);
	mLastVal = returnVal;

	return(returnVal);
}

}
} // namespace nvidia::apex
