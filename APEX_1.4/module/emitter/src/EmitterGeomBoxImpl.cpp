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
#include "ApexUsingNamespace.h"
#include "EmitterAsset.h"
#include "EmitterGeomBoxImpl.h"
//#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"
#include "RenderDebugInterface.h"
#include "ApexPreview.h"
#include "EmitterGeomBoxParams.h"

namespace nvidia
{
namespace emitter
{



EmitterGeomBoxImpl::EmitterGeomBoxImpl(NvParameterized::Interface* params)
{
	NvParameterized::Handle eh(*params);
	const NvParameterized::Definition* paramDef;
	const char* enumStr = 0;

	mGeomParams = (EmitterGeomBoxParams*)params;
	mExtents = (PxVec3*)(&(mGeomParams->parameters().extents));

	//error check
	mGeomParams->getParameterHandle("emitterType", eh);
	mGeomParams->getParamEnum(eh, enumStr);
	paramDef = eh.parameterDefinition();

	mType = EmitterType::ET_RATE;
	for (int i = 0; i < paramDef->numEnumVals(); ++i)
	{
		if (!nvidia::strcmp(paramDef->enumVal(i), enumStr))
		{
			mType = (EmitterType::Enum)i;
			break;
		}
	}
}

EmitterGeom* EmitterGeomBoxImpl::getEmitterGeom()
{
	return this;
}

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomBoxImpl::visualize(const PxTransform& , RenderDebugInterface&)
{
}
#else
void EmitterGeomBoxImpl::visualize(const PxTransform& pose, RenderDebugInterface& renderDebug)
{
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGreen));
	RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose);
	RENDER_DEBUG_IFACE(&renderDebug)->debugBound(PxBounds3(PxVec3(0.0f), 2.0f * *mExtents));
	RENDER_DEBUG_IFACE(&renderDebug)->setPose(PxIdentity);
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomBoxImpl::drawPreview(float , RenderDebugInterface*) const
{
}
#else
void EmitterGeomBoxImpl::drawPreview(float scale, RenderDebugInterface* renderDebug) const
{
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen),
	                             RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen));
	RENDER_DEBUG_IFACE(renderDebug)->debugBound(PxBounds3(-(*mExtents) * scale, *mExtents * scale));
	RENDER_DEBUG_IFACE(renderDebug)->popRenderState();
}
#endif


void EmitterGeomBoxImpl::setEmitterType(EmitterType::Enum t)
{
	mType = t;

	NvParameterized::Handle eh(*mGeomParams);
	const NvParameterized::Definition* paramDef;

	//error check
	mGeomParams->getParameterHandle("emitterType", eh);
	paramDef = eh.parameterDefinition();

	mGeomParams->setParamEnum(eh, paramDef->enumVal((int)mType));
}

/* ApexEmitterActor callable methods */


float EmitterGeomBoxImpl::computeEmitterVolume() const
{
	return 8.0f * mExtents->x * mExtents->y * mExtents->z;
}


/* Return percentage of new volume not covered by old volume */
float EmitterGeomBoxImpl::computeNewlyCoveredVolume(
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


void EmitterGeomBoxImpl::computeFillPositions(physx::Array<PxVec3>& positions,
        physx::Array<PxVec3>& velocities,
        const PxTransform& pose,
		const PxVec3& scale,
        float objRadius,
        PxBounds3& outBounds,
        QDSRand&) const
{
	// we're not doing anything with the velocities array
	PX_UNUSED(velocities);
	PX_UNUSED(scale);

	// we don't want anything outside the emitter
	uint32_t numX = (uint32_t)PxFloor(mExtents->x / objRadius);
	numX -= numX % 2;
	uint32_t numY = (uint32_t)PxFloor(mExtents->y / objRadius);
	numY -= numY % 2;
	uint32_t numZ = (uint32_t)PxFloor(mExtents->z / objRadius);
	numZ -= numZ % 2;

	for (float x = -(numX * objRadius); x <= mExtents->x - objRadius; x += 2 * objRadius)
	{
		for (float y = -(numY * objRadius); y <= mExtents->y - objRadius; y += 2 * objRadius)
		{
			for (float z = -(numZ * objRadius); z <= mExtents->z - objRadius; z += 2 * objRadius)
			{
				positions.pushBack(pose.transform(PxVec3(x, y, z)));
				outBounds.include(positions.back());
			}
		}
	}
}

/* internal methods */

PxVec3 EmitterGeomBoxImpl::randomPosInFullVolume(const PxMat44& pose, QDSRand& rand) const
{
	float u =  rand.getScaled(-mExtents->x, mExtents->x);
	float v =  rand.getScaled(-mExtents->y, mExtents->y);
	float w =  rand.getScaled(-mExtents->z, mExtents->z);

	PxVec3 pos(u, v, w);
	return pose.transform(pos);
}

bool EmitterGeomBoxImpl::isInEmitter(const PxVec3& pos, const PxMat44& pose) const
{
	PxVec3 localPos = pose.inverseRT().transform(pos);

	if (localPos.x < -mExtents->x)
	{
		return false;
	}
	if (localPos.x > mExtents->x)
	{
		return false;
	}
	if (localPos.y < -mExtents->y)
	{
		return false;
	}
	if (localPos.y > mExtents->y)
	{
		return false;
	}
	if (localPos.z < -mExtents->z)
	{
		return false;
	}
	if (localPos.z > mExtents->z)
	{
		return false;
	}

	return true;
}

PxVec3 EmitterGeomBoxImpl::randomPosInNewlyCoveredVolume(const PxMat44& pose, const PxMat44& oldPose, QDSRand& rand) const
{
	// TODO make better, this is very slow when emitter moves slowly
	// SJB: I'd go one further, this seems mildly retarted
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
