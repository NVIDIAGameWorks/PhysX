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

#ifndef PT_COLLISION_DATA_H
#define PT_COLLISION_DATA_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"
#include "foundation/PxTransform.h"
#include "particles/PxParticleFlag.h"
#include "PtConfig.h"

namespace physx
{

struct PxsShapeCore;
struct PxsBodyCore;

namespace Pt
{

#define PT_CCD_PROJECT 0         // Ocational leaking at static interfaces
#define PT_CCD_STAY 1            // Seems to work for static
#define PT_CCD_IMPACT 2          // Doesn't work at all for static interfaces
#define PT_CDD_BACKTRACK_SMALL 3 // Seems to work for static
#define PT_CDD_BACKTRACK_LARGE 4 // Seems to work for static

#define PT_CDD_BACKTRACK_SMALL_EPS 1e-4f

#define PT_CCD_MEDTHOD PT_CCD_STAY // Maybe we'll need to do something else for dynamics

PX_FORCE_INLINE void computeContinuousTargetPosition(PxVec3& surfacePos, const PxVec3& localOldPos,
                                                     const PxVec3& relativePOSITION, const PxVec3& surfaceNormal,
                                                     const PxF32 restOffset)
{
	PX_UNUSED(restOffset);
	PX_UNUSED(surfaceNormal);
	PX_UNUSED(relativePOSITION);

#if PT_CCD_MEDTHOD == PT_CCD_PROJECT
	surfacePos = localOldPos + relativePOSITION + (surfaceNormal * restOffset);
#elif PT_CCD_MEDTHOD == PT_CCD_STAY
	surfacePos = localOldPos;
#elif PT_CCD_MEDTHOD == PT_CCD_IMPACT
	surfacePos = localOldPos + relativePOSITION;
#else
	const PxF32 backtrackLength = (PT_CCD_MEDTHOD == PT_CDD_BACKTRACK_SMALL) ? PT_CDD_BACKTRACK_SMALL_EPS : restOffset;
	PxF32 relImpactLength = relativePOSITION.magnitude();
	PxF32 backtrackParam = (relImpactLength > 0.0f) ? PxMax(0.0f, relImpactLength - backtrackLength) : 0.0f;
	surfacePos = localOldPos + relativePOSITION * (backtrackParam / relImpactLength);
#endif
}

/*!
Fluid particle collision constraint
*/
struct Constraint
{
	PxVec3 normal; // Contact surface normal
	PxF32 d;       // Contact point projected on contact normal
	               // 16

  public:
	Constraint()
	{
		// Do we want to initialize the constraints on creation?
		// setZero();
	}

	Constraint(const PxVec3& _normal, const PxVec3& _p)
	{
		normal = _normal;
		d = normal.dot(_p);
	}

	PX_FORCE_INLINE PxVec3 project(const PxVec3& p) const
	{
		return (p + (normal * (d - normal.dot(p))));
	}
};

/*!
Fluid particle collision constraint data for dynamic rigid body
*/
struct ConstraintDynamic
{
	PxVec3 velocity;
	const PxsBodyCore* twoWayBody; // weak reference to rigid body.

  public:
	PX_FORCE_INLINE void setEmpty()
	{
		velocity = PxVec3(0);
		twoWayBody = NULL;
	}
};

/*!
Fluid particle collision constraint buffers
*/
struct ConstraintBuffers
{
	Constraint* constraint0Buf;
	Constraint* constraint1Buf;
	ConstraintDynamic* constraint0DynamicBuf;
	ConstraintDynamic* constraint1DynamicBuf;
};

/*!
Different types of collision
*/
struct ParticleCollisionFlags
{
	enum Enum
	{
		// Global collision flags. Used to track the latest collision status of a particle when
		// testing against potentially colliding shapes
		DC            = (1 << 0), // Discrete collision
		CC            = (1 << 1), // Continuous collision
		RESET_SNORMAL = (1 << 2), // Saves one PxVec3 in the ParticleCollData

		// When testing a particle against a shape, the following collision flags might be used
		L_CC          = (1 << 3), // Discrete collision: Predicted particle position inside discrete region of shape (shape
		// region + collision radius)
		L_DC          = (1 << 4), // Continuous collision: Predicted particle motion vector intersects shape region
		L_PROX        = (1 << 5), // Proximity collision: Predicted particle position inside proximity region of shape (shape
		// region + proximity radius)
		L_CC_PROX     = (L_CC | L_PROX),
		L_ANY         = (L_CC | L_DC | L_PROX)
	};
};

/*!
Structure to track collision data for a fluid particle
*/
struct ParticleCollData
{
	PxVec3 surfaceNormal; // Contact normal [world space]
	PxU32 flags;          // Latest collision status
	// 16

	PxVec3 surfacePos; // Contact point on shape surface [world space]
	PxF32 dcNum;       // Number of discrete collisions
	// 32

	PxVec3 surfaceVel; // Velocity of contact point on shape surface [world space]
	PxF32 ccTime;      // "Time of impact" for continuous collision
	// 48

	PxVec3 oldPos; // Old particle position
	ParticleFlags particleFlags;
	// 64

	PxVec3 newPos; // New particle position
	PxU32 origParticleIndex;
	// 80

	PxVec3 velocity; // Particle velocity
	PxF32 restOffset;
	// 96

	PxVec3 twoWayImpulse;
	const PxsBodyCore* twoWayBody; // Weak reference to colliding rigid body
	// 112

	PxVec3 localOldPos; // in
	PxU32 localFlags;   // in/out
	// 128

	PxVec3 localNewPos; // in
	Constraint* c0;     // in
	// 144

	PxVec3 localSurfaceNormal; // out
	Constraint* c1;            // in
	// 160

	PxVec3 localSurfacePos; // out
	PxF32 localDcNum;       // Number of discrete collisions
	                        // 176

  public:
	PX_FORCE_INLINE void init(const PxVec3& particlePos, const PxF32 particleRestOffset, const PxU32 particleIndex,
	                          const ParticleFlags _particleFlags)
	{
		// Initialize values

		surfaceNormal = PxVec3(0);
		flags = 0;

		surfacePos = PxVec3(0);
		dcNum = 0.0f;

		surfaceVel = PxVec3(0);
		ccTime = 1.0f; // No collision assumed.

		restOffset = particleRestOffset;

		oldPos = particlePos;

		// Remove collision flags from previous time step
		particleFlags.api = PxU16(_particleFlags.api & ((~PxU16(PxParticleFlag::eCOLLISION_WITH_STATIC)) &
		                                                (~PxU16(PxParticleFlag::eCOLLISION_WITH_DYNAMIC))));

		// Reduce cache bits
		// 11 -> 01
		// 01 -> 00
		// 00 -> 00
		PxU16 reducedCache = PxU16(((_particleFlags.low & InternalParticleFlag::eGEOM_CACHE_MASK) >> 1) &
		                           InternalParticleFlag::eGEOM_CACHE_MASK);
		particleFlags.low = PxU16((_particleFlags.low & ~PxU16(InternalParticleFlag::eGEOM_CACHE_MASK)) | reducedCache);

		origParticleIndex = particleIndex;

		twoWayBody = NULL;
		twoWayImpulse = PxVec3(0);
	}
};

struct PxVec3Pad
{
	PxVec3 v3;
	PxF32 pad;
};

struct ParticleCollDataV4
{
	ParticleCollData* collData[4];
	PX_ALIGN(16, PxVec3Pad localOldPos[4]);        // in
	PX_ALIGN(16, PxVec3Pad localNewPos[4]);        // in
	PX_ALIGN(16, PxF32 restOffset[4]);             // in
	PX_ALIGN(16, PxU32 localFlags[4]);             // in,out
	PX_ALIGN(16, PxF32 ccTime[4]);                 // out
	PX_ALIGN(16, PxVec3Pad localSurfaceNormal[4]); // out
	PX_ALIGN(16, PxVec3Pad localSurfacePos[4]);    // out
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_COLLISION_DATA_H
