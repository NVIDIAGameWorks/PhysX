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
#include "EmitterGeomSphereImpl.h"
//#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"
#include "RenderDebugInterface.h"
#include "ApexPreview.h"
#include "EmitterGeomSphereParams.h"

namespace nvidia
{
namespace emitter
{



EmitterGeomSphereImpl::EmitterGeomSphereImpl(NvParameterized::Interface* params)
{
	NvParameterized::Handle eh(*params);
	const NvParameterized::Definition* paramDef;
	const char* enumStr = 0;

	mGeomParams = (EmitterGeomSphereParams*)params;
	mRadius = &(mGeomParams->parameters().radius);
	mHemisphere = &(mGeomParams->parameters().hemisphere);

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

EmitterGeom* EmitterGeomSphereImpl::getEmitterGeom()
{
	return this;
}


#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomSphereImpl::visualize(const PxTransform& , RenderDebugInterface&)
{
}
#else
void EmitterGeomSphereImpl::visualize(const PxTransform& pose, RenderDebugInterface& renderDebug)
{
	const float radius = *mRadius;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose);
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGreen));
	RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(PxVec3(0.0f), *mRadius);
	if(hemisphere > 0.0f) 
	{
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkPurple));
		PxMat44 circlePose = PxMat44(PxIdentity);
		circlePose.setPosition(PxVec3(0.0f, sphereCapBaseHeight, 0.0f));
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(circlePose);
		RENDER_DEBUG_IFACE(&renderDebug)->debugCircle(PxVec3(0.0f), sphereCapBaseRadius, 3);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(PxVec3(0.0f), PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		for(float t = 0.0f; t < 2 * PxPi; t += PxPi / 3)
		{
			PxVec3 offset(PxSin(t) * sphereCapBaseRadius, 0.0f, PxCos(t) * sphereCapBaseRadius);
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(offset, PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		}
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(PxIdentity);
	}
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomSphereImpl::drawPreview(float , RenderDebugInterface*) const
{
}
#else
void EmitterGeomSphereImpl::drawPreview(float scale, RenderDebugInterface* renderDebug) const
{
	const float radius = *mRadius;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen),
	                             RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen));
	RENDER_DEBUG_IFACE(renderDebug)->debugSphere(PxVec3(0.0f), *mRadius * scale);
	if(hemisphere > 0.0f)
	{
		RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkPurple));
		PxMat44 circlePose = PxMat44(PxIdentity);
		circlePose.setPosition(PxVec3(0.0f, sphereCapBaseHeight, 0.0f));
		RENDER_DEBUG_IFACE(renderDebug)->setPose(circlePose);
		RENDER_DEBUG_IFACE(renderDebug)->debugCircle(PxVec3(0.0f), sphereCapBaseRadius, 3);
		RENDER_DEBUG_IFACE(renderDebug)->debugLine(PxVec3(0.0f), PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		for(float t = 0.0f; t < 2 * PxPi; t += PxPi / 3)
		{
			PxVec3 offset(PxSin(t) * sphereCapBaseRadius, 0.0f, PxCos(t) * sphereCapBaseRadius);
			RENDER_DEBUG_IFACE(renderDebug)->debugLine(offset,  PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		}
		RENDER_DEBUG_IFACE(renderDebug)->setPose(PxIdentity);
	}
	RENDER_DEBUG_IFACE(renderDebug)->popRenderState();
}
#endif

void EmitterGeomSphereImpl::setEmitterType(EmitterType::Enum t)
{
	mType = t;

	NvParameterized::Handle eh(*mGeomParams);
	const NvParameterized::Definition* paramDef;

	//error check
	mGeomParams->getParameterHandle("emitterType", eh);
	paramDef = eh.parameterDefinition();

	mGeomParams->setParamEnum(eh, paramDef->enumVal((int)mType));
}

float EmitterGeomSphereImpl::computeEmitterVolume() const
{
	PX_ASSERT(*mHemisphere >= 0.0f);
	PX_ASSERT(*mHemisphere <= 1.0f);
	float radius = *mRadius;
	float hemisphere = 2 * radius * (*mHemisphere);
	bool moreThanHalf = true;
	if (hemisphere > radius)
	{
		hemisphere -= radius;
		moreThanHalf = false;
	}
	const float halfSphereVolume = 2.0f / 3.0f * PxPi * radius * radius * radius;
	const float sphereCapVolume = 1.0f / 3.0f * PxPi * hemisphere * hemisphere * (3 * radius - hemisphere);
	if (moreThanHalf)
	{
		return halfSphereVolume + sphereCapVolume;
	}
	else
	{
		return sphereCapVolume;
	}
}

PxVec3 EmitterGeomSphereImpl::randomPosInFullVolume(const PxMat44& pose, QDSRand& rand) const
{
	PX_ASSERT(*mHemisphere >= 0.0f);
	PX_ASSERT(*mHemisphere <= 1.0f);

	const float radius = *mRadius;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	const float horizontalExtents = hemisphere < 0.5f ? radius : sphereCapBaseRadius;
	/* bounding box for a sphere cap */
	const PxBounds3 boundingBox(PxVec3(-horizontalExtents, sphereCapBaseHeight, -horizontalExtents),
									   PxVec3(horizontalExtents, radius, horizontalExtents));

	PxVec3 pos;
	do
	{
		pos = rand.getScaled(boundingBox.minimum, boundingBox.maximum);
	}
	while (pos.magnitudeSquared() > radiusSquared);

	return pose.transform(pos);
}


bool EmitterGeomSphereImpl::isInEmitter(const PxVec3& pos, const PxMat44& pose) const
{
	const PxVec3 localPos = pose.inverseRT().transform(pos);
	const float radius = *mRadius;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	const float horizontalExtents = hemisphere < 0.5f ? radius : sphereCapBaseRadius;
	/* bounding box for a sphere cap */
	const PxBounds3 boundingBox(PxVec3(-horizontalExtents, sphereCapBaseHeight, -horizontalExtents),
	                                   PxVec3(horizontalExtents, radius, horizontalExtents));

	bool isInSphere = localPos.magnitudeSquared() <= radiusSquared;
	bool isInBoundingBox = boundingBox.contains(localPos);

	return isInSphere & isInBoundingBox;
}


void EmitterGeomSphereImpl::computeFillPositions(physx::Array<PxVec3>& positions,
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

	const float radius = *mRadius;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	const float horizontalExtents = hemisphere < 0.5f ? radius : sphereCapBaseRadius;

	uint32_t numX = (uint32_t)PxFloor(horizontalExtents / objRadius);
	numX -= numX % 2;
	uint32_t numY = (uint32_t)PxFloor((radius - sphereCapBaseHeight) / objRadius);
	numY -= numY % 2;
	uint32_t numZ = (uint32_t)PxFloor(horizontalExtents / objRadius);
	numZ -= numZ % 2;

	const float radiusMinusObjRadius = radius - objRadius;
	const float radiusMinusObjRadiusSquared = radiusMinusObjRadius * radiusMinusObjRadius;
	for (float x = -(numX * objRadius); x <= radiusMinusObjRadius; x += 2 * objRadius)
	{
		for (float y = sphereCapBaseHeight; y <= radiusMinusObjRadius; y += 2 * objRadius)
		{
			for (float z = -(numZ * objRadius); z <= radiusMinusObjRadius; z += 2 * objRadius)
			{
				const PxVec3 p(x, y, z);
				if (p.magnitudeSquared() < radiusMinusObjRadiusSquared)
				{
					positions.pushBack(pose.transform(PxVec3(x, y, z)));
					outBounds.include(positions.back());
				}
			}
		}
	}
}

}
} // namespace nvidia::apex
