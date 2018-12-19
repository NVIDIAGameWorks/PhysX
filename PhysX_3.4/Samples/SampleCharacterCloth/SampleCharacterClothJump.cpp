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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SampleCharacterClothJump.h"

using namespace physx;

static PxF32 gJumpGravity = -25.0f;

SampleCharacterClothJump::SampleCharacterClothJump() :
	mV          (0.0f),
	mTime	(0.0f),
	mJump		(false),
	mFreefall   (false)
{
}

void SampleCharacterClothJump::startJump(PxF32 v0)
{
	if (mJump) return;
	if (mFreefall) return;
	mV = v0;
	mJump = true;
}

void SampleCharacterClothJump::reset()
{
	// reset jump state at most every half a second
	// otherwise we might miss jump events
	if (mTime < 0.5f)
		return;

	mFreefall = false;
	mJump = false;
	mV = 0.0f;
	mTime = 0.0f;
}

void SampleCharacterClothJump::tick(PxF32 dtime)
{
	mTime += dtime;
	mV += gJumpGravity * dtime;
	const PxReal vlimit = -25.0f;
	if (mV < vlimit)
	{
		// limit velocity in freefall so that character does not fall at rocket speed.
		mV = vlimit;
		mFreefall = true;
		mJump = false;
	}
}

PxF32 SampleCharacterClothJump::getDisplacement(PxF32 dtime)
{
	return mV * dtime;
}

