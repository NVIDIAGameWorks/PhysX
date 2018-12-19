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
#include "EmitterGeomBase.h"

namespace nvidia
{
namespace emitter
{

/* Return percentage of new volume not covered by old volume */
float EmitterGeomBase::computeNewlyCoveredVolume(
    const PxMat44& oldPose,
    const PxMat44& newPose,
	float scale,
    QDSRand& rand) const
{
	// estimate by sampling
	const uint32_t numSamples = 100;
	uint32_t numOutsideOldVolume = 0;
	for (uint32_t i = 0; i < numSamples; i++)
	{
		if (!isInEmitter(randomPosInFullVolume(PxMat44(newPose) * scale, rand), PxMat44(oldPose) * scale))
		{
			numOutsideOldVolume++;
		}
	}

	return (float) numOutsideOldVolume / numSamples;
}


// TODO make better, this is very slow when emitter moves slowly
// SJB: I'd go one further, this seems mildly retarted
PxVec3 EmitterGeomBase::randomPosInNewlyCoveredVolume(const PxMat44& pose, const PxMat44& oldPose, QDSRand& rand) const
{
	PxVec3 pos;
	do
	{
		pos = randomPosInFullVolume(pose, rand);
	}
	while (isInEmitter(pos, oldPose));
	return pos;
}

}
} // namespace nvidia::apex
