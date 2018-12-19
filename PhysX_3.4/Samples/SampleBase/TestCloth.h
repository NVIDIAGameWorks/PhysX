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

#ifndef TEST_CLOTH
#define TEST_CLOTH

#include "PxShape.h"
#include "cloth/PxCloth.h"

using namespace physx;

namespace Test
{
    class ClothCollision
    {
    public:
        virtual PxU32 addSphere(const PxVec3& position, PxReal radius) = 0;
        virtual PxU32 addCapsuleIndex(PxU32 i0, PxU32 i1) = 0;
		virtual PxU32 addCapsule(const PxVec3& position1, PxReal radius1, const PxVec3& position2, PxReal radius2) = 0;
		virtual PxU32 getNbSpheres() const = 0;
		virtual const PxClothCollisionSphere* getSpheres() const = 0;
		virtual void setClothPose(const PxTransform &pose) = 0;
		virtual void setCapsuleMotion(PxU32 id, const PxVec3 &linear, const PxVec3 &angular) = 0;
        virtual void setSphereMotion(PxU32 id, const PxVec3 &linear) = 0;
		virtual void updateMotion(PxReal time, PxReal timestep) = 0;
		virtual bool generateClothCollisionData(PxClothCollisionData &) const = 0;
		virtual ~ClothCollision() {};
    };

	class Cloth
	{
	public:

		// border flags
		enum 
		{
			NONE = 0,
			BORDER_TOP		= (1 << 0),
			BORDER_BOTTOM	= (1 << 1),
			BORDER_LEFT		= (1 << 2),
			BORDER_RIGHT	= (1 << 3)
		};

		// solver types
		enum SolverType 
        {
            eMIXED = 1 << 0, // eSTIFF for vertical fiber, eFAST for everything else
            eFAST = 1 << 1,  // eFAST for everything
            eSTIFF = 1 << 2, // eSTIFF for everything
			eZEROSTRETCH = 1 << 3 // eZEROSTRETCH for zero stretch fiber, eFAST for everything else
        };

		virtual void detachVertex(PxU32 vertexId) = 0;
		virtual void attachVertexToWorld(PxU32 vertexId) = 0;
		virtual void attachVertexToWorld(PxU32 vertexId, const PxVec3& pos) = 0;
		virtual void attachBorderToWorld(PxU32 borderFlags) = 0;
		virtual void attachOverlapToShape(PxShape* shape, PxReal radius = 0.1f) = 0;
		virtual void createVirtualParticles(int numSamples) = 0;
		virtual ClothCollision& getCollision() = 0;
		virtual const PxTransform& getClothPose() const = 0;
		virtual void release() = 0;
		virtual PxCloth& getCloth() = 0;
		virtual void setCloth(PxCloth&) = 0;
		virtual PxU32 getNbParticles() const = 0;
		virtual PxClothParticle* getParticles() = 0;
		virtual PxBounds3 getWorldBounds(bool includeColliders = false) = 0;
		virtual void setClothPose(const PxTransform &pose, bool keepIntertia = true) = 0;
		virtual void setAnimationSpeed(PxReal) = 0;
		virtual void setDampingCoefficient(PxReal d) = 0;
		virtual void setMassScalingCoefficient(PxReal s) = 0;
		virtual void setMotion(const PxVec3 &linear, const PxVec3 &angular) = 0;
		virtual void setSolverFrequency(PxReal v) = 0;
		virtual void setSolverType(Cloth::SolverType solverType) = 0;
		virtual void setStiffness(PxReal v) = 0;
		virtual void setSweptContact(bool val) = 0;
		virtual void setUseGPU(bool val) = 0;
        virtual void updateMotion(PxReal time, PxReal timestep, bool keepInertia = true) = 0;
		virtual ~Cloth() {};
	};
};

#endif // TEST_CLOTH
