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

#include "PhaseConfig.h"
#include "PsMathUtils.h"

namespace physx
{
namespace cloth
{
PhaseConfig transform(const PhaseConfig&);
}
}

using namespace physx;

namespace
{
float safeLog2(float x)
{
	float saturated = PxMax(0.0f, PxMin(x, 1.0f));
	return saturated ? shdfnd::log2(saturated) : -FLT_MAX_EXP;
}
}

cloth::PhaseConfig::PhaseConfig(uint16_t index)
: mPhaseIndex(index)
, mPadding(0xffff)
, mStiffness(1.0f)
, mStiffnessMultiplier(1.0f)
, mCompressionLimit(1.0f)
, mStretchLimit(1.0f)
{
}

// convert from user input to solver format
cloth::PhaseConfig cloth::transform(const PhaseConfig& config)
{
	PhaseConfig result(config.mPhaseIndex);

	result.mStiffness = safeLog2(1.0f - config.mStiffness);
	result.mStiffnessMultiplier = safeLog2(config.mStiffnessMultiplier);

	// negative for compression, positive for stretch
	result.mCompressionLimit = 1 - 1 / config.mCompressionLimit;
	result.mStretchLimit = 1 - 1 / config.mStretchLimit;

	return result;
}
