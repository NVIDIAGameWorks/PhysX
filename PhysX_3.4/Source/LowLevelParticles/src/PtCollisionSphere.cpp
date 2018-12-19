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

#include "PtCollisionMethods.h"
#if PX_USE_PARTICLE_SYSTEM_API

using namespace physx;
using namespace Pt;

namespace
{

void collideWithSphereNonContinuous(ParticleCollData& collData, const PxVec3& pos, const PxReal& radius,
                                    const PxReal& proxRadius)
{
	if(collData.localFlags & ParticleCollisionFlags::CC)
		return; // Only apply discrete and proximity collisions if no continuous collisions was detected so far (for any
	// colliding shape)

	PxReal dist = pos.magnitude();
	collData.localSurfaceNormal = pos;
	if(dist < (radius + proxRadius))
	{
		if(dist != 0.0f)
			collData.localSurfaceNormal *= (1.0f / dist);
		else
			collData.localSurfaceNormal = PxVec3(0);

		// Push particle to surface such that the distance to the surface is equal to the collision radius
		collData.localSurfacePos = collData.localSurfaceNormal * (radius + collData.restOffset);
		collData.localFlags |= ParticleCollisionFlags::L_PROX;

		if(dist < (radius + collData.restOffset))
			collData.localFlags |= ParticleCollisionFlags::L_DC;
	}
}

PX_FORCE_INLINE void collideWithSphere(ParticleCollData& collData, const PxSphereGeometry& sphereShapeData,
                                       PxReal proxRadius)
{
	PxVec3& oldPos = collData.localOldPos;
	PxVec3& newPos = collData.localNewPos;

	PxReal radius = sphereShapeData.radius;

	PxReal oldPosDist2 = oldPos.magnitudeSquared();
	PxReal radius2 = radius * radius;

	bool oldInSphere = (oldPosDist2 < radius2);

	if(oldInSphere)
	{
		// old position inside the skeleton
		// add ccd with time 0.0

		collData.localSurfaceNormal = oldPos;
		if(oldPosDist2 > 0.0f)
			collData.localSurfaceNormal *= PxRecipSqrt(oldPosDist2);
		else
			collData.localSurfaceNormal = PxVec3(0, 1.0f, 0);

		// Push particle to surface such that the distance to the surface is equal to the collision radius
		collData.localSurfacePos = collData.localSurfaceNormal * (radius + collData.restOffset);
		collData.ccTime = 0.0;
		collData.localFlags |= ParticleCollisionFlags::L_CC;
	}
	else
	{
		// old position is outside of the skeleton

		PxVec3 motion = newPos - oldPos;

		// Discriminant
		PxReal b = motion.dot(oldPos) * 2.0f;
		PxReal a2 = 2.0f * motion.magnitudeSquared();
		PxReal disc = (b * b) - (2.0f * a2 * (oldPosDist2 - radius2));

		bool intersection = disc > 0.0f;

		if((!intersection) || (a2 == 0.0f))
		{
			// the ray does not intersect the sphere
			collideWithSphereNonContinuous(collData, newPos, radius, proxRadius);
		}
		else
		{
			// the ray intersects the sphere
			PxReal t = -(b + PxSqrt(disc)) / a2; // Compute intersection point

			if(t < 0.0f || t > 1.0f)
			{
				// intersection point lies outside motion vector
				collideWithSphereNonContinuous(collData, newPos, radius, proxRadius);
			}
			else if(t < collData.ccTime)
			{
				// intersection point lies on sphere, add lcc
				// collData.localSurfacePos = oldPos + (motion * t);
				// collData.localSurfaceNormal = collData.localSurfacePos;
				// collData.localSurfaceNormal *= (1.0f / radius);
				// collData.localSurfacePos += (collData.localSurfaceNormal * collData.restOffset);
				PxVec3 relativeImpact = motion * t;
				collData.localSurfaceNormal = oldPos + relativeImpact;
				collData.localSurfaceNormal *= (1.0f / radius);
				computeContinuousTargetPosition(collData.localSurfacePos, collData.localOldPos, relativeImpact,
				                                collData.localSurfaceNormal, collData.restOffset);

				collData.ccTime = t;
				collData.localFlags |= ParticleCollisionFlags::L_CC;
			}
		}
	}
}

} // namespace

void physx::Pt::collideWithSphere(ParticleCollData* particleCollData, PxU32 numCollData,
                                  const Gu::GeometryUnion& sphereShape, PxReal proxRadius)
{
	PX_ASSERT(particleCollData);

	const PxSphereGeometry& sphereShapeData = sphereShape.get<const PxSphereGeometry>();

	for(PxU32 p = 0; p < numCollData; p++)
	{
		::collideWithSphere(particleCollData[p], sphereShapeData, proxRadius);
	}
}

#endif // PX_USE_PARTICLE_SYSTEM_API
