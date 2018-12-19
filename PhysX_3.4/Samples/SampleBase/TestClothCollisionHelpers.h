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

#ifndef WRAP_CLOTH_H
#define WRAP_CLOTH_H

//----------------------------------------------------------------------------//
#include "PxPhysicsAPI.h"
#include "TestArray.h"
#include "TestMotionGenerator.h"

#include "cloth/PxCloth.h"

#include "PhysXSample.h"

//----------------------------------------------------------------------------//

namespace physx
{
	class PxClothMeshDesc;
	class PxClothFabric;
}

namespace Test
{
    // wrapper for collision data for cloth
    class ClothCollision 
    {
    public:
		friend class ::RenderClothActor;
        ClothCollision();

        // Test::ClothCollision
        virtual PxU32 addSphere(const PxVec3& position, PxReal radius);
        virtual PxU32 addCapsuleIndex(PxU32 i0, PxU32 i1);
		virtual PxU32 addCapsule(const PxVec3& position1, PxReal radius1, const PxVec3& position2, PxReal radius2);
		virtual PxU32 getNbSpheres() const;
		virtual PxU32 getNbCapsules() const;

		virtual PxBounds3 getWorldBounds() const; 
		virtual const PxClothCollisionSphere* getSpheres() const;
		virtual const PxU32* getCapsules() const;

		virtual void setClothPose(const PxTransform &clothPose);
        virtual void setCapsuleMotion(PxU32 id, const PxVec3 &linear, const PxVec3 &angular);
		virtual void setSphereMotion(PxU32 id, const PxVec3 &linear);
        virtual void updateMotion(PxReal time, PxReal timestep);

		virtual void release();

        
		virtual ~ClothCollision();
    protected:
        // convert sphere positions to local pose of cloth
        void applyLocalTransform();

    protected:
		PxTransform mClothPose;
		Test::Array<PxVec3> mOrigPositions;
		Test::Array<PxVec3> mWorldPositions;
		Test::Array<PxClothCollisionSphere> mSpheres;
		Test::Array<MotionGenerator> mSphereMotion;
		Test::Array<MotionGenerator> mCapsuleMotion;
		Test::Array<PxU32> mCapsuleIndices;
	};
}


//----------------------------------------------------------------------------//

#endif // WRAP_CLOTH_H

