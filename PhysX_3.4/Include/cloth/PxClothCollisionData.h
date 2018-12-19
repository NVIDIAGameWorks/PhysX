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


#ifndef PX_PHYSICS_NX_CLOTH_COLLISION_DATA
#define PX_PHYSICS_NX_CLOTH_COLLISION_DATA
/** \addtogroup cloth
  @{
*/

#include "PxPhysXConfig.h"
#include "foundation/PxVec3.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Sphere representation used for cloth-sphere and cloth-capsule collision.
\details Cloth can collide with spheres and capsules.  Each capsule is represented by
a pair of spheres with possibly different radii.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
struct PX_DEPRECATED PxClothCollisionSphere
{
	PxVec3 pos;    //!< position of the sphere
	PxReal radius; //!< radius of the sphere.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothCollisionSphere() {}
	PxClothCollisionSphere(const PxVec3& p, PxReal r)
		: pos(p), radius(r) {}
};

/**
\brief Plane representation used for cloth-convex collision.
\details Cloth can collide with convexes.  Each convex is represented by
a mask of the planes that make up the convex.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
struct PX_DEPRECATED PxClothCollisionPlane
{
	PxVec3 normal;   //!< The normal to the plane
	PxReal distance; //!< The distance to the origin (in the normal direction)

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothCollisionPlane() {}
	PxClothCollisionPlane(const PxVec3& normal_, PxReal distance_)
		: normal(normal_), distance(distance_) {}
};

/**
\brief Triangle representation used for cloth-mesh collision.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
struct PX_DEPRECATED PxClothCollisionTriangle
{
	PxVec3 vertex0;
	PxVec3 vertex1;
	PxVec3 vertex2;

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothCollisionTriangle() {}
	PxClothCollisionTriangle(
		const PxVec3& v0, 
		const PxVec3& v1,
		const PxVec3& v2) :
		vertex0(v0),
		vertex1(v1),
		vertex2(v2) {}
};


#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
