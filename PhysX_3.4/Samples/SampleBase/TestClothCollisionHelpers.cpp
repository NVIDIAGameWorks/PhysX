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
#include "TestClothCollisionHelpers.h"

#include "cloth/PxCloth.h"
#include "cloth/PxClothCollisionData.h"

//needed for sync primitives.
#if PX_WINDOWS
#define NOMINMAX
#	include <windows.h>
#elif defined(_XBOX)
#	include "xbox360\xtl_XBOX.h"
#endif

//----------------------------------------------------------------------------//
Test::ClothCollision::ClothCollision() :
    mClothPose(PxVec3(0.0f), PxQuat(PxIdentity))
{
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::release()
{
	delete this;
}

//----------------------------------------------------------------------------//

Test::ClothCollision::~ClothCollision()
{
	mSphereMotion.clear();
}

//----------------------------------------------------------------------------//
PxU32 Test::ClothCollision::addSphere(const PxVec3& position, PxReal radius)
{
    PxClothCollisionSphere sphere;

    sphere.pos = position;
    sphere.radius = radius;

    mSpheres.pushBack(sphere);
	mOrigPositions.pushBack(sphere.pos);
	mWorldPositions.pushBack(sphere.pos);

	MotionGenerator mg;

	mg.mTransform.p = position;
	mSphereMotion.pushBack(mg);

	return mSpheres.size() - 1;
}

//----------------------------------------------------------------------------//
PxU32 Test::ClothCollision::addCapsuleIndex(PxU32 i0, PxU32 i1)
{
    mCapsuleIndices.pushBack(i0);
    mCapsuleIndices.pushBack(i1);

	MotionGenerator mg;

	mg.mTransform.p = 0.5f * (mOrigPositions[i0] + mOrigPositions[i1]);
	mCapsuleMotion.pushBack(mg);

	PX_ASSERT(( mCapsuleIndices.size() % 2) == 0);
	return (mCapsuleIndices.size() / 2) - 1;
}

//----------------------------------------------------------------------------//
PxU32 Test::ClothCollision::addCapsule(const PxVec3& position1, PxReal radius1, 
										 const PxVec3& position2, PxReal radius2)
{
	PxU32 i0 = addSphere(position1, radius1);
	PxU32 i1 = addSphere(position2, radius2);

	return addCapsuleIndex(i0, i1);
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::setClothPose(const PxTransform &clothPose)
{
    mClothPose = clothPose;
    applyLocalTransform();
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::applyLocalTransform()
{
	PxTransform clothInv = mClothPose.getInverse();

	PX_ASSERT(mSpheres.size() == mWorldPositions.size());

	Test::Array<PxClothCollisionSphere>::Iterator sp = mSpheres.begin();
	Test::Array<PxVec3>::Iterator o = mWorldPositions.begin();

	for ( ; sp != mSpheres.end(); ++sp, ++o)
	{
		sp->pos = clothInv.transform(*o); 
	}
}

//----------------------------------------------------------------------------//
PxU32 Test::ClothCollision::getNbSpheres() const
{
	return mSpheres.size();
}

//----------------------------------------------------------------------------//
PxU32 Test::ClothCollision::getNbCapsules() const
{
	return mCapsuleIndices.size()/2;
}

//----------------------------------------------------------------------------//
const PxClothCollisionSphere* Test::ClothCollision::getSpheres() const
{
	return mSpheres.begin();
}

//----------------------------------------------------------------------------//
const PxU32* Test::ClothCollision::getCapsules() const
{
	return mCapsuleIndices.begin();
}

//----------------------------------------------------------------------------//
PxBounds3 Test::ClothCollision::getWorldBounds() const
{
	PxBounds3 totalBounds = PxBounds3::empty();
    for (PxU32 i = 0; i < mSpheres.size(); i++)
    {
		PxVec3 p = mClothPose.transform(mSpheres[i].pos);
		PxBounds3 bounds = PxBounds3::centerExtents(p, PxVec3(mSpheres[i].radius));
		totalBounds.minimum = totalBounds.minimum.minimum(bounds.minimum);
		totalBounds.maximum = totalBounds.maximum.maximum(bounds.maximum);
    }
	return totalBounds;
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::setSphereMotion(PxU32 id, const PxVec3 &linear)
{
    mSphereMotion[id].mLinear = linear;
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::setCapsuleMotion(PxU32 id, const PxVec3 &linear, const PxVec3& angular)
{
    mCapsuleMotion[id].mLinear = linear;
    mCapsuleMotion[id].mAngular = angular;
}

//----------------------------------------------------------------------------//
void Test::ClothCollision::updateMotion(PxReal time, PxReal timestep)
{
    for (PxU32 i = 0; i < mSphereMotion.size(); i++)
		mSphereMotion[i].update(time, timestep);

    for (PxU32 i = 0; i < mCapsuleMotion.size(); i++)
		mCapsuleMotion[i].update(time, timestep);

	// update independent sphere positions first
    PxU32 nbSpheres = mWorldPositions.size();
    for (PxU32 i = 0; i < nbSpheres; i++)
        mWorldPositions[i] = mSphereMotion[i].mTransform.p;

	// update capsule position and orientation
	PxU32 numCapsules = mCapsuleIndices.size() / 2;
	PX_ASSERT((mCapsuleIndices.size() % 2) == 0);
	for (PxU32 i = 0; i < numCapsules; i++)
	{
		PxU32 i0 = mCapsuleIndices[i*2];
		PxU32 i1 = mCapsuleIndices[i*2+1];

		PxTransform transform = mCapsuleMotion[i].mTransform;
		PxVec3 o0 = mOrigPositions[i0];
		PxVec3 o1 = mOrigPositions[i1];
		PxVec3 c = 0.5f * (o0 + o1);

		mWorldPositions[i0] = transform.rotate(o0 - c) + transform.p;
		mWorldPositions[i1] = transform.rotate(o1 - c) + transform.p;
	}

    applyLocalTransform();
}

