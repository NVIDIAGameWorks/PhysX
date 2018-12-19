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
#include "EmitterGeomCylinderImpl.h"
//#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"
#include "ApexPreview.h"
#include "EmitterGeomCylinderParams.h"

namespace nvidia
{
namespace emitter
{



EmitterGeomCylinderImpl::EmitterGeomCylinderImpl(NvParameterized::Interface* params)
{
	NvParameterized::Handle eh(*params);
	const NvParameterized::Definition* paramDef;
	const char* enumStr = 0;

	mGeomParams = (EmitterGeomCylinderParams*)params;
	mRadius = &(mGeomParams->parameters().radius);
	mHeight = &(mGeomParams->parameters().height);

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

EmitterGeom* EmitterGeomCylinderImpl::getEmitterGeom()
{
	return this;
}


#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomCylinderImpl::visualize(const PxTransform& , RenderDebugInterface&)
{
}
#else
void EmitterGeomCylinderImpl::visualize(const PxTransform& pose, RenderDebugInterface& renderDebug)
{
	PxVec3 p0, p1;
	p0 = PxVec3(0.0f, -*mHeight / 2.0f, 0.0f);
	p1 = PxVec3(0.0f, *mHeight / 2.0f, 0.0f);

	p0 = pose.transform(p0);
	p1 = pose.transform(p1);
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGreen));

	RENDER_DEBUG_IFACE(&renderDebug)->debugCylinder(p0, p1, *mRadius);
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomCylinderImpl::drawPreview(float , RenderDebugInterface*) const
{
}
#else
void EmitterGeomCylinderImpl::drawPreview(float scale, RenderDebugInterface* renderDebug) const
{
	PxVec3 p0, p1;
	// where should we put this thing???
	p0 = PxVec3(0.0f, -*mHeight / 2.0f, 0.0f);
	p1 = PxVec3(0.0f, *mHeight / 2.0f, 0.0f);
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen),
													RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen));

	RENDER_DEBUG_IFACE(renderDebug)->debugCylinder(p0, p1, *mRadius * scale);
	RENDER_DEBUG_IFACE(renderDebug)->popRenderState();
}
#endif

void EmitterGeomCylinderImpl::setEmitterType(EmitterType::Enum t)
{
	mType = t;

	NvParameterized::Handle eh(*mGeomParams);
	const NvParameterized::Definition* paramDef;

	//error check
	mGeomParams->getParameterHandle("emitterType", eh);
	paramDef = eh.parameterDefinition();

	mGeomParams->setParamEnum(eh, paramDef->enumVal((int)mType));
}

float EmitterGeomCylinderImpl::computeEmitterVolume() const
{
	return (*mHeight) * (*mRadius) * (*mRadius) * PxPi;
}


PxVec3 EmitterGeomCylinderImpl::randomPosInFullVolume(const PxMat44& pose, QDSRand& rand) const
{
	PxVec3 pos;

	float u, v, w;
	do
	{
		u = rand.getNext();
		w = rand.getNext();
	}
	while (u * u + w * w > 1.0f);

	v = *mHeight / 2 * rand.getNext();

	pos = PxVec3(u * (*mRadius), v, w * (*mRadius));

#if _DEBUG
	PxMat44 tmpPose(PxIdentity);
	PX_ASSERT(isInEmitter(pos, tmpPose));
#endif

	return pose.transform(pos);
}


bool EmitterGeomCylinderImpl::isInEmitter(const PxVec3& pos, const PxMat44& pose) const
{
	PxVec3 localPos = pose.inverseRT().transform(pos);

	if (localPos.x < -*mRadius)
	{
		return false;
	}
	if (localPos.x > *mRadius)
	{
		return false;
	}
	if (localPos.y < -*mHeight / 2.0f)
	{
		return false;
	}
	if (localPos.y > *mHeight / 2.0f)
	{
		return false;
	}
	if (localPos.z < -*mRadius)
	{
		return false;
	}
	if (localPos.z > *mRadius)
	{
		return false;
	}

	return true;
}


void EmitterGeomCylinderImpl::computeFillPositions(physx::Array<PxVec3>& positions,
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

	float halfHeight = *mHeight / 2;

	uint32_t numX = (uint32_t)PxFloor(*mRadius / objRadius);
	numX -= numX % 2;
	uint32_t numY = (uint32_t)PxFloor(halfHeight / objRadius);
	numY -= numY % 2;
	uint32_t numZ = (uint32_t)PxFloor(*mRadius / objRadius);
	numZ -= numZ % 2;

	for (float x = -(numX * objRadius); x <= *mRadius - objRadius; x += 2 * objRadius)
	{
		for (float y = -(numY * objRadius); y <= halfHeight - objRadius; y += 2 * objRadius)
		{
			for (float z = -(numZ * objRadius); z <= *mRadius - objRadius; z += 2 * objRadius)
			{
				if (x * x + z * z < (*mRadius - objRadius) * (*mRadius - objRadius))
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
