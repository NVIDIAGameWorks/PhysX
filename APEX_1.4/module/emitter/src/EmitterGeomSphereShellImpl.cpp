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
#include "EmitterGeomSphereShellImpl.h"
//#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"
#include "RenderDebugInterface.h"
#include "ApexPreview.h"
#include "EmitterGeomSphereShellParams.h"

namespace nvidia
{
namespace emitter
{



EmitterGeomSphereShellImpl::EmitterGeomSphereShellImpl(NvParameterized::Interface* params)
{
	NvParameterized::Handle eh(*params);
	const NvParameterized::Definition* paramDef;
	const char* enumStr = 0;

	mGeomParams = (EmitterGeomSphereShellParams*)params;
	mRadius = &(mGeomParams->parameters().radius);
	mShellThickness = &(mGeomParams->parameters().shellThickness);
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

EmitterGeom* EmitterGeomSphereShellImpl::getEmitterGeom()
{
	return this;
}


#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomSphereShellImpl::visualize(const PxTransform& , RenderDebugInterface&)
{
}
#else
void EmitterGeomSphereShellImpl::visualize(const PxTransform& pose, RenderDebugInterface& renderDebug)
{
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGreen));

	// outer sphere
	RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose);
	RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(PxVec3(0.0f), *mRadius);

	// intter sphere
	RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(PxVec3(0.0f), *mRadius + *mShellThickness);

	const float radius = *mRadius + *mShellThickness;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	// cone depicting the hemisphere
	if(hemisphere > 0.0f) 
	{
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkPurple));
		PxMat44 circlePose = PxMat44(PxIdentity);
		circlePose.setPosition(PxVec3(0.0f, sphereCapBaseHeight, 0.0f));
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(circlePose);
		RENDER_DEBUG_IFACE(&renderDebug)->debugCircle(PxVec3(0.0f), sphereCapBaseRadius, 3);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(circlePose.getPosition(), circlePose.getPosition() + PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		for(float t = 0.0f; t < 2 * PxPi; t += PxPi / 3)
		{
			PxVec3 offset(PxSin(t) * sphereCapBaseRadius, 0.0f, PxCos(t) * sphereCapBaseRadius);
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(circlePose.getPosition() + offset, circlePose.getPosition() + PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		}
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(PxIdentity);
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif


#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomSphereShellImpl::drawPreview(float , RenderDebugInterface*) const
{
}
#else
void EmitterGeomSphereShellImpl::drawPreview(float scale, RenderDebugInterface* renderDebug) const
{
	using RENDER_DEBUG::DebugColors;

	RENDER_DEBUG_IFACE(renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::Yellow),
	                             RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::Yellow));

	RENDER_DEBUG_IFACE(renderDebug)->debugSphere(PxVec3(0.0f), *mRadius * scale);

	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen),
	                             RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen));

	RENDER_DEBUG_IFACE(renderDebug)->debugSphere(PxVec3(0.0f), (*mRadius + *mShellThickness) * scale);

	const float radius = *mRadius + *mShellThickness;
	const float radiusSquared = radius * radius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -radius + 2 * radius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
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
			RENDER_DEBUG_IFACE(renderDebug)->debugLine(offset, PxVec3(0.0f, radius - sphereCapBaseHeight, 0.0f));
		}
		RENDER_DEBUG_IFACE(renderDebug)->setPose(PxIdentity);
	}
	RENDER_DEBUG_IFACE(renderDebug)->popRenderState();
}
#endif

void EmitterGeomSphereShellImpl::setEmitterType(EmitterType::Enum t)
{
	mType = t;

	NvParameterized::Handle eh(*mGeomParams);
	const NvParameterized::Definition* paramDef;

	//error check
	mGeomParams->getParameterHandle("emitterType", eh);
	paramDef = eh.parameterDefinition();

	mGeomParams->setParamEnum(eh, paramDef->enumVal((int)mType));
}

float EmitterGeomSphereShellImpl::computeEmitterVolume() const
{
	const float radius = *mRadius;
	const float bigRadius = *mRadius + *mShellThickness;
	float hemisphere = 2 * radius * (*mHemisphere);
	float bigHemisphere = 2 * bigRadius * (*mHemisphere);

	bool moreThanHalf = true;

	if (hemisphere > radius)
	{
		hemisphere -= radius;
		bigHemisphere -= bigRadius;
		moreThanHalf = false;
	}

	const float volumeBigSphere = 4.0f / 3.0f * PxPi * bigRadius * bigRadius * bigRadius;
	const float volumeSmallSphere = 4.0f / 3.0f * PxPi * *mRadius * *mRadius * *mRadius;
	const float halfSphereShellVolume = (volumeBigSphere - volumeSmallSphere) / 2.0f;

	const float bigCapVolume = 1.0f / 3.0f * PxPi * bigHemisphere * bigHemisphere * (3 * bigRadius - bigHemisphere);
	const float smallCapVolume = 1.0f / 3.0f * PxPi * hemisphere * hemisphere * (3 * radius - hemisphere);

	const float sphereShellCapVolume = bigCapVolume - smallCapVolume;

	if (moreThanHalf)
	{
		return halfSphereShellVolume + sphereShellCapVolume;
	}
	else
	{
		return sphereShellCapVolume;
	}
}


PxVec3 EmitterGeomSphereShellImpl::randomPosInFullVolume(const PxMat44& pose, QDSRand& rand) const
{
	float hemisphere = 2.0f * *mHemisphere - 1.0f;

	bool moreThanHalf = true;

	if (*mHemisphere > 0.5f)
	{
		moreThanHalf = false;
	}

	// There are two cases here - 1-st for hemisphere cut above the center of the sphere
	// and 2-nd for hemisphere cut below the center of the sphere.
	// The reason for this is that in case very high hemisphere cut is set, so the area
	// of the actual emitter is very small in compare to the whole sphere emitter, it would take too
	// much time [on average] to generate suitable point using randomPointOnUnitSphere
	// function, so in this case it is more efficient to use another method.
	// in case we have at least half of the sphere shell present the randomPointOnUnitSphere should
	// be sufficient.
	PxVec3 pos;
	if(!moreThanHalf)
	{
		// 1-st case :
		// * generate random unit vector within a cone
		// * clamp to big radius
		const float sphereCapBaseHeight = -1.0f + 2 * (*mHemisphere);
		const float phi = rand.getScaled(0.0f, PxTwoPi);
		const float cos_theta = sphereCapBaseHeight;
		const float z = rand.getScaled(cos_theta, 1.0f);
		const float oneMinusZSquared = PxSqrt(1.0f - z * z);
		pos = PxVec3(oneMinusZSquared * PxCos(phi), z, oneMinusZSquared * PxSin(phi));
	}
	else
	{
		// 2-nd case :
		// * get random pos on unit sphere, until its height is above hemisphere cut
		do
		{
			pos = randomPointOnUnitSphere(rand);
		} while(pos.y < hemisphere);
	}

	// * add negative offset withing the thickness
	// * solve edge case [for the 1-st case] - regenerate offset from the previous step
	// in case point is below hemisphere cut	

	PxVec3 tmp;
	const float sphereCapBaseHeight = -(*mRadius + *mShellThickness) + 2 * (*mRadius + *mShellThickness) * (*mHemisphere);
	do
	{
		float thickness = rand.getScaled(0, *mShellThickness);
		tmp = pos * (*mRadius + *mShellThickness - thickness);
	} while(tmp.y < sphereCapBaseHeight);

	pos = tmp;
	pos += pose.getPosition();

	return pos;
}


bool EmitterGeomSphereShellImpl::isInEmitter(const PxVec3& pos, const PxMat44& pose) const
{
	PxVec3 localPos = pose.inverseRT().transform(pos);
	const float sphereCapBaseHeight = -(*mRadius + *mShellThickness) + 2 * (*mRadius + *mShellThickness) * (*mHemisphere);
	float d2 = localPos.x * localPos.x + localPos.y * localPos.y + localPos.z * localPos.z;
	bool isInBigSphere = d2 < (*mRadius + *mShellThickness) * (*mRadius + *mShellThickness);
	bool isInSmallSphere = d2 < *mRadius * *mRadius;
	bool higherThanHemisphereCut = pos.y > sphereCapBaseHeight;
	return isInBigSphere && !isInSmallSphere && higherThanHemisphereCut;
}


void EmitterGeomSphereShellImpl::computeFillPositions(physx::Array<PxVec3>& positions,
        physx::Array<PxVec3>& velocities,
        const PxTransform& pose,
		const PxVec3& scale,
        float objRadius,
        PxBounds3& outBounds,
        QDSRand&) const
{
	PX_UNUSED(scale);

	const float bigRadius = *mRadius + *mShellThickness;
	const float radiusSquared = bigRadius * bigRadius;
	const float hemisphere = *mHemisphere;
	const float sphereCapBaseHeight = -bigRadius + 2 * bigRadius * hemisphere;
	const float sphereCapBaseRadius = PxSqrt(radiusSquared - sphereCapBaseHeight * sphereCapBaseHeight);
	const float horizontalExtents = hemisphere < 0.5f ? bigRadius : sphereCapBaseRadius;

	// we're not doing anything with the velocities array
	PX_UNUSED(velocities);

	// we don't want anything outside the emitter
	uint32_t numX = (uint32_t)PxFloor(horizontalExtents / objRadius);
	numX -= numX % 2;
	uint32_t numY = (uint32_t)PxFloor((bigRadius - sphereCapBaseHeight) / objRadius);
	numY -= numY % 2;
	uint32_t numZ = (uint32_t)PxFloor(horizontalExtents / objRadius);
	numZ -= numZ % 2;

	for (float x = -(numX * objRadius); x <= bigRadius - objRadius; x += 2 * objRadius)
	{
		for (float y = -(numY * objRadius); y <= bigRadius - objRadius; y += 2 * objRadius)
		{
			for (float z = -(numZ * objRadius); z <= bigRadius - objRadius; z += 2 * objRadius)
			{
				const float magnitudeSquare = PxVec3(x, y, z).magnitudeSquared();
				if ((magnitudeSquare > (*mRadius + objRadius) * (*mRadius + objRadius)) &&
				        (magnitudeSquare < (bigRadius - objRadius) * (bigRadius - objRadius)))
				{
					positions.pushBack(pose.transform(PxVec3(x, y, z)));
					outBounds.include(positions.back());
				}
			}
		}
	}
}


PxVec3 EmitterGeomSphereShellImpl::randomPointOnUnitSphere(QDSRand& rand) const
{
	// uniform distribution on the sphere around pos (Cook, Marsaglia Method. TODO: is other method cheaper?)
	float x0, x1, x2, x3, div;
	do
	{
		x0 = rand.getNext();
		x1 = rand.getNext();
		x2 = rand.getNext();
		x3 = rand.getNext();
		div = x0 * x0 + x1 * x1 + x2 * x2 + x3 * x3;
	}
	while (div >= 1.0f);

	// coordinates on unit sphere
	float x = 2 * (x1 * x3 + x0 * x2) / div;
	float y = 2 * (x2 * x3 - x0 * x1) / div;
	float z = (x0 * x0 + x3 * x3 - x1 * x1 - x2 * x2) / div;

	return PxVec3(x, y, z);
}

}
} // namespace nvidia::apex
