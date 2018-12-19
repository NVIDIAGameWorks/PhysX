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

#ifndef PT_COLLISION_METHODS_H
#define PT_COLLISION_METHODS_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxVec3.h"
#include "PtConfig.h"
#include "PtCollisionData.h"
#include "PtSpatialHash.h"
#include "PtParticleOpcodeCache.h"
#include "GuGeometryUnion.h"

namespace physx
{

namespace Pt
{

/*!
Collision routines for fluid particles
*/

void collideWithPlane(ParticleCollData* particleCollData, PxU32 numCollData, const Gu::GeometryUnion& planeShape,
                      PxReal proxRadius);

void collideWithConvexPlanes(ParticleCollData& collData, const PxPlane* planes, const PxU32 numPlanes,
                             const PxReal proxRadius);
void collideWithConvexPlanesSIMD(ParticleCollDataV4& collDataV4, const PxPlane* convexPlanes, PxU32 numPlanes,
                                 const PxReal proxRadius);

/**
input scaledPlaneBuf needs a capacity of the number of planes in convexShape
*/
void collideWithConvex(PxPlane* scaledPlaneBuf, ParticleCollData* particleCollData, PxU32 numCollData,
                       const Gu::GeometryUnion& convexShape, const PxReal proxRadius);

void collideWithBox(ParticleCollData* particleCollData, PxU32 numCollData, const Gu::GeometryUnion& boxShape,
                    PxReal proxRadius);

void collideWithCapsule(ParticleCollData* particleCollData, PxU32 numCollData, const Gu::GeometryUnion& capsuleShape,
                        PxReal proxRadius);

void collideWithSphere(ParticleCollData* particleCollData, PxU32 numCollData, const Gu::GeometryUnion& sphereShape,
                       PxReal proxRadius);

void collideCellsWithStaticMesh(ParticleCollData* particleCollData, const LocalCellHash& localCellHash,
                                const Gu::GeometryUnion& meshShape, const PxTransform& world2Shape,
                                const PxTransform& shape2World, PxReal cellSize, PxReal collisionRange,
                                PxReal proxRadius, const PxVec3& packetCorner);

void collideWithStaticMesh(PxU32 numParticles, ParticleCollData* particleCollData, ParticleOpcodeCache* opcodeCaches,
                           const Gu::GeometryUnion& meshShape, const PxTransform& world2Shape,
                           const PxTransform& shape2World, PxReal cellSize, PxReal collisionRange, PxReal proxRadius);

void collideWithStaticHeightField(ParticleCollData* particleCollData, PxU32 numCollData,
                                  const Gu::GeometryUnion& heightFieldShape, PxReal proxRadius,
                                  const PxTransform& shape2World);

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_COLLISION_METHODS_H
