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



#ifndef SHAPE_H
#define SHAPE_H

/*!
\file
\brief class Shape
*/

#include "foundation/PxPreprocessor.h"
#include "ApexUsingNamespace.h"

#include "ApexDefs.h"

/*#if PX_PHYSICS_VERSION_MAJOR < 3
#include "Nxp.h"
#endif*/

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


class SphereShape;
class CapsuleShape;
class BoxShape;
class HalfSpaceShape;
class RenderDebugInterface;

/**
\brief Describes the format of an Shape.
*/
class Shape
{
public:

	///intersect the collision shape against a given AABB
	virtual bool intersectAgainstAABB(PxBounds3) = 0;

	///AABB of collision shape
	virtual PxBounds3 getAABB() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual const BoxShape* isBoxGeom() const
	{
		return NULL;
	}

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual const SphereShape* isSphereGeom() const
	{
		return NULL;
	}

	///If it is a capsule, cast to the capsule class, return NULL otherwise
	virtual const CapsuleShape* isCapsuleGeom() const
	{
		return NULL;
	}

	///If it is a half space, cast to half space class, return NULL otherwise
	virtual const HalfSpaceShape* isHalfSpaceGeom() const
	{
		return NULL;
	}

	///Release shape
	virtual void releaseApexShape() = 0;

	///Visualize shape
	virtual void visualize(RenderDebugInterface* renderer) const = 0;

	///get the pose of the shape
	virtual PxMat44 getPose() const = 0;

	///set the pose of the shape - this pose will modify the scale, orientation and position of the shape
	virtual void setPose(PxMat44 pose) = 0;

	///get the previous pose of the shape
	virtual PxMat44 getPreviousPose() const = 0;
};

/// helper Sphere shape
class SphereShape : public Shape
{
public:
	///get radius of the sphere
	virtual float getRadius() const = 0;

	///set radius of the sphere
	virtual void setRadius(float radius) = 0;

};

/// helper the capsule is oriented along the y axis by default and its total height is height+2*radius
class CapsuleShape : public Shape
{
public:
	///get height and radius of the capsule
	virtual void getDimensions(float& height, float& radius) const = 0;

	///set height and radius of the capsule
	virtual void setDimensions(float height, float radius) = 0;

};

/// helper Box shape
class BoxShape : public Shape
{
public:
	///set the length of the sides of the box. The default value is 1.0f for each of the sides.
	///the size of the sides can also be changed by using the setPose method
	virtual void setSize(PxVec3 size) = 0;

	///get size of the box
	virtual PxVec3 getSize() const = 0;

};

/// helper HalfSpace shape
class HalfSpaceShape : public Shape
{
public:
	///set origin and normal of the halfspace
	virtual void setOriginAndNormal(PxVec3 origin, PxVec3 normal) = 0;

	///get normal of the halfspace
	virtual PxVec3 getNormal() const = 0;

	///get origin of the halfspace
	virtual PxVec3 getOrigin() const = 0;

	///get previous origin of the halfspace
	virtual PxVec3 getPreviousOrigin() const = 0;
};


PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // SHAPE_H
