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

void collideWithCapsuleNonContinuous(ParticleCollData& collData, const PxVec3& q, const PxReal& h, const PxReal& r,
                                     const PxReal& proxRadius)
{
	if(collData.localFlags & ParticleCollisionFlags::CC)
		return; // Only apply discrete and proximity collisions if no continuous collisions was detected so far (for any
	// colliding shape)

	PxVec3 segPoint;
	segPoint = PxVec3(q.x, 0.0f, 0.0f);
	segPoint.x = PxMax(segPoint.x, -h);
	segPoint.x = PxMin(segPoint.x, h);
	collData.localSurfaceNormal = q - segPoint;
	PxReal dist = collData.localSurfaceNormal.magnitude();
	if(dist < (r + proxRadius))
	{
		if(dist != 0.0f)
			collData.localSurfaceNormal *= (1.0f / dist);
		else
			collData.localSurfaceNormal = PxVec3(0);

		// Push particle to surface such that the distance to the surface is equal to the collision radius
		collData.localSurfacePos = segPoint + (collData.localSurfaceNormal * (r + collData.restOffset));
		collData.localFlags |= ParticleCollisionFlags::L_PROX;

		if(dist < (r + collData.restOffset))
			collData.localFlags |= ParticleCollisionFlags::L_DC;
	}
}

void collideWithCapsuleTestSphere(ParticleCollData& collData, const PxVec3& p, const PxVec3& q, const PxVec3& d,
                                  const PxReal& h, const PxReal& r, const PxReal& sphereH, const PxReal& discS,
                                  const PxReal& aS, const PxReal& bS, const PxReal& proxRadius)
{
	if(discS <= 0.0f || aS == 0.0f)
	{
		collideWithCapsuleNonContinuous(collData, q, h, r, proxRadius);
	}
	else
	{
		PxReal t = -(bS + PxSqrt(discS)) / aS;
		if(t < 0.0f || t > 1.0f)
		{
			// intersection lies outside p-q interval
			collideWithCapsuleNonContinuous(collData, q, h, r, proxRadius);
		}
		else if(t < collData.ccTime)
		{
			// intersection point lies on sphere, add lcc
			// collData.localSurfacePos = p + (d * t);
			// collData.localSurfaceNormal = collData.localSurfacePos;
			// collData.localSurfaceNormal.x -= sphereH;
			// collData.localSurfaceNormal *= (1.0f / r);
			// collData.localSurfacePos += (collData.localSurfaceNormal * collData.restOffset);
			PxVec3 relativePOSITION = (d * t);
			collData.localSurfaceNormal = p + relativePOSITION;
			collData.localSurfaceNormal.x -= sphereH;
			collData.localSurfaceNormal *= (1.0f / r);
			computeContinuousTargetPosition(collData.localSurfacePos, p, relativePOSITION, collData.localSurfaceNormal,
			                                collData.restOffset);
			collData.ccTime = t;
			collData.localFlags |= ParticleCollisionFlags::L_CC;
		}
	}
}

// ----------------------------------------------------------------
//
//		Note: this code is based on the hardware implementation
//
//      Terminology:
//      Starting point: p
//      End point:      q
//      Ray direction:  d
//
//      Infinite cylinder I:  all (y,z)   : y^2 + z^2 < r^2
//      "Fat plane"       F:  all (x)     : -h < x < h
//      Top sphere        S0: all (x,y,z) : y^2 + z^2 + (x-h)^2 < r^2
//      Bottom sphere     S1: all (x,y,z) : y^2 + z^2 + (x+h)^2 < r^2
//
//      Cylinder          Z = (I & F)
//      Capsule           C = Z | S0 | S1
//
//      coefficients a, b, c for the squared distance functions sqd(t) = a * t^2 + b * t + c, for I, S0 and S1:
//
//      aI =  d.y*d.y + d.z*d.z
//      aS0 = d.y*d.y + d.z*d.z + d.x*d.x
//      aS1 = d.y*d.y + d.z*d.z + d.x*d.x
//
//      bI =  d.y*p.y + d.z*p.z
//      bS0 = d.y*p.y + d.z*p.z + d.x*p.x - h*d.x
//      bS1 = d.y*p.y + d.z*p.z + d.x*p.x + h*d.x
//
//      cI =  p.y*p.y + p.z*p.z - r*r.
//      cS0 = p.y*p.y + p.z*p.z - r*r + p.x*p.x + h*h - 2*h*p.x
//      cS1 = p.y*p.y + p.z*p.z - r*r + p.x*p.x + h*h + 2*h*p.x
//
//      these will be treated in vectorized fashion:
//      I  <--> .y
//      S0 <--> .x
//      S1 <--> .z
//
//      for p, we have sqd(0) = c
//      ( for q, we have sqd(1) = a + b + c )
//
// ----------------------------------------------------------------
PX_FORCE_INLINE void collideWithCapsule(ParticleCollData& collData, const PxCapsuleGeometry& capsuleShapeData,
                                        PxReal proxRadius)
{
	// Note: The local coordinate system of a capsule is defined such that the cylindrical part is
	//       wrapped around the x-axis

	PxVec3& p = collData.localOldPos;
	PxVec3& q = collData.localNewPos;

	PxReal r = capsuleShapeData.radius;
	PxReal h = capsuleShapeData.halfHeight;

	PxVec3 a, b, c;

	// all c values
	PxReal tmp;
	c.y = p.y * p.y + p.z * p.z - r * r;
	tmp = c.y + p.x * p.x + h * h;
	c.x = tmp - 2 * h * p.x;
	c.z = tmp + 2 * h * p.x;

	bool pInI = c.y < 0.0f;  // Old particle position inside the infinite zylinder
	bool pInS0 = c.x < 0.0f; // Old particle position inside the right sphere
	bool pInS1 = c.z < 0.0f; // Old particle position inside the left sphere
	bool pRightOfH = p.x > h;
	bool pLeftOfMinusH = p.x < -h;
	bool pInZ = (!pRightOfH && !pLeftOfMinusH && pInI);

	if(pInZ || pInS0 || pInS1)
	{
		// p is inside the skeleton
		// add ccd with time 0.0

		PxVec3 segPoint;
		segPoint = PxVec3(p.x, 0.0f, 0.0f);
		segPoint.x = PxMax(segPoint.x, -h);
		segPoint.x = PxMin(segPoint.x, h);
		PxVec3 normal = p - segPoint;
		collData.localSurfaceNormal = normal.isZero() ? PxVec3(0.0f, 1.0f, 0.0f) : normal.getNormalized();
		// Push particle to surface such that the distance to the surface is equal to the collision radius
		collData.localSurfacePos = segPoint + (collData.localSurfaceNormal * (r + collData.restOffset));
		collData.ccTime = 0.0;
		collData.localFlags |= ParticleCollisionFlags::L_CC;
	}
	else
	{
		// p is outside of the skeleton

		PxVec3 d = q - p;

		// all b values
		b.y = d.y * p.y + d.z * p.z;
		tmp = b.y + d.x * p.x;
		b.x = tmp - h * d.x;
		b.z = tmp + h * d.x;

		// all a values
		a.y = d.y * d.y + d.z * d.z;
		a.x = a.y + d.x * d.x;
		a.z = a.x;

		// all discriminants
		PxVec3 tmpVec0, tmpVec1;
		tmpVec0 = b.multiply(b);
		tmpVec1 = c.multiply(a);
		PxVec3 discs = tmpVec0 - tmpVec1;

		// this made cases fail with d.y == 0.0 and d.z == 0.0
		// bool dInI  = discs.y > 0.0f;
		bool dInI = discs.y >= 0.0f;

		// bool dInS0 = discs.x > 0.0f;
		// bool dInS1 = discs.z > 0.0f;

		if(!dInI)
		{
			// the ray does not intersect the infinite cylinder
			collideWithCapsuleNonContinuous(collData, q, h, r, proxRadius);
		}
		else
		{
			// d intersects the infinite cylinder
			if(pInI)
			{
				// p is contained in the infinite cylinder, either above the top sphere or below the bottom sphere.
				// -> directly test against the nearest sphere
				if(p.x > 0)
				{
					// check sphere 0
					collideWithCapsuleTestSphere(collData, p, q, d, h, r, h, discs.x, a.x, b.x, proxRadius);
				}
				else
				{
					// check sphere 1
					collideWithCapsuleTestSphere(collData, p, q, d, h, r, -h, discs.z, a.z, b.z, proxRadius);
				}
			}
			else if(discs.y <= 0.0f || a.y == 0.0f)
			{
				// d is zero or tangential to cylinder surface
				collideWithCapsuleNonContinuous(collData, q, h, r, proxRadius);
			}
			else
			{
				// p lies outside of infinite cylinder, compute intersection point with it
				PxReal t = -(b.y + PxSqrt(discs.y)) / a.y;
				if(t < 0.0f || t > 1.0f)
				{
					// intersection lies outside p-q interval
					collideWithCapsuleNonContinuous(collData, q, h, r, proxRadius);
				}
				else
				{
					PxVec3 relativePOSITION = (d * t);
					PxVec3 impact = p + relativePOSITION;
					if(impact.x > h)
					{
						// if above the actual cylinder, check sphere 0
						collideWithCapsuleTestSphere(collData, p, q, d, h, r, h, discs.x, a.x, b.x, proxRadius);
					}
					else if(impact.x < -h)
					{
						// if below the actual cylinder, check sphere 1
						collideWithCapsuleTestSphere(collData, p, q, d, h, r, -h, discs.z, a.z, b.z, proxRadius);
					}
					else if(t < collData.ccTime)
					{
						// intersection point lies on cylinder, add cc
						// collData.localSurfaceNormal = collData.localSurfacePos / r;
						// collData.localSurfaceNormal.x = 0.0f;
						// collData.localSurfacePos += (collData.localSurfaceNormal * collData.restOffset);
						collData.localSurfaceNormal = impact / r;
						collData.localSurfaceNormal.x = 0.0f;
						computeContinuousTargetPosition(collData.localSurfacePos, p, relativePOSITION,
						                                collData.localSurfaceNormal, collData.restOffset);
						collData.ccTime = t;
						collData.localFlags |= ParticleCollisionFlags::L_CC;
					}
				}
			}
		}
	}
}

} // namespace

void physx::Pt::collideWithCapsule(ParticleCollData* collShapeData, PxU32 numCollData,
                                   const Gu::GeometryUnion& capsuleShape, PxReal proxRadius)
{
	PX_ASSERT(collShapeData);
	PX_ASSERT(capsuleShape.getType() == PxGeometryType::eCAPSULE);

	const PxCapsuleGeometry& capsuleShapeData = capsuleShape.get<const PxCapsuleGeometry>();

	for(PxU32 p = 0; p < numCollData; p++)
	{
		::collideWithCapsule(collShapeData[p], capsuleShapeData, proxRadius);
	}
}

#endif // PX_USE_PARTICLE_SYSTEM_API
