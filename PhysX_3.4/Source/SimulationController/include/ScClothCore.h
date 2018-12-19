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


#ifndef PX_PHYSICS_SCP_CLOTH_CORE
#define PX_PHYSICS_SCP_CLOTH_CORE

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "ScActorCore.h"

#include "foundation/PxTransform.h"
#include "PxFiltering.h"
#include "PxCloth.h"
#include "PxClothTypes.h"
#include "PxClothCollisionData.h"

#include "PsArray.h"

namespace physx
{

struct PxClothCollisionSphere;

namespace cloth
{
	class Cloth;
	struct PhaseConfig;
}

namespace Sc
{
	class ClothFabricCore;
	class ClothSim;

	bool DefaultClothInterCollisionFilter(void* cloth0, void* cloth1);

	struct ClothBulkData : public Ps::UserAllocated
	{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for 
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================

		// constructor and destructor are needed because some compilers zero the memory in the
		// default constructor and that breaks the serialization related memory markers for 
		// implicitly padded bytes
		ClothBulkData() {}
		~ClothBulkData() {}

		void	exportExtraData(PxSerializationContext& stream);
		void	importExtraData(PxDeserializationContext& context);
		static void	getBinaryMetaData(PxOutputStream& stream);

		shdfnd::Array<PxClothParticle> mParticles;
		shdfnd::Array<PxU32> mVpData;
		shdfnd::Array<PxVec3> mVpWeightData;
		shdfnd::Array<PxClothCollisionSphere> mCollisionSpheres;
		shdfnd::Array<PxU32> mCollisionPairs;
		shdfnd::Array<PxClothCollisionPlane> mCollisionPlanes;
		shdfnd::Array<PxU32> mConvexMasks;
		shdfnd::Array<PxClothCollisionTriangle> mCollisionTriangles;
		shdfnd::Array<PxClothParticleMotionConstraint> mConstraints;
		shdfnd::Array<PxClothParticleSeparationConstraint> mSeparationConstraints;
		shdfnd::Array<PxVec4> mParticleAccelerations;
		shdfnd::Array<PxU32> mSelfCollisionIndices;
		shdfnd::Array<PxVec4> mRestPositions;
		
		// misc data
		PxReal mTetherConstraintScale;
		PxReal mTetherConstraintStiffness;
		PxReal mMotionConstraintScale;
		PxReal mMotionConstraintBias;
		PxReal mMotionConstraintStiffness;
		PxVec3 mAcceleration;
		PxVec3 mDamping;
		PxReal mFriction;
		PxReal mCollisionMassScale;
		PxVec3 mLinearDrag;
		PxVec3 mAngularDrag;
		PxVec3 mLinearInertia;
		PxVec3 mAngularInertia;
		PxVec3 mCentrifugalInertia;
		PxReal mSolverFrequency;
		PxReal mStiffnessFrequency;
		PxReal mSelfCollisionDistance;
		PxReal mSelfCollisionStiffness;
		PxTransform mGlobalPose;
		PxReal mSleepThreshold;
		PxReal mWakeCounter;
		PxVec3 mWindVelocity;
		PxReal mDragCoefficient;
		PxReal mLiftCoefficient;
	};


	class ClothCore : public ActorCore
	{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for 
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================
	public:
// PX_SERIALIZATION
								ClothCore(const PxEMPTY) : ActorCore(PxEmpty), mLowLevelCloth(NULL), mFabric(NULL) {}
		void					exportExtraData(PxSerializationContext& stream);
		void					importExtraData(PxDeserializationContext& context);
		void					resolveReferences(Sc::ClothFabricCore& fabric);
		static void				getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
								ClothCore(const PxTransform& globalPose, Sc::ClothFabricCore& fabric, const PxClothParticle* particles, PxClothFlags flags);
								~ClothCore();

		//---------------------------------------------------------------------------------
		// External API
		//---------------------------------------------------------------------------------
		PX_FORCE_INLINE ClothFabricCore* getFabric() const { return mFabric; }
		PX_FORCE_INLINE void	resetFabric() { mFabric = NULL; }

		void					setParticles(const PxClothParticle* currentParticles, const PxClothParticle* previousParticles);
		PxU32					getNbParticles() const;

		void					setMotionConstraints(const PxClothParticleMotionConstraint* motionConstraints);
		bool					getMotionConstraints(PxClothParticleMotionConstraint* motionConstraintsBuffer) const;
		PxU32					getNbMotionConstraints() const; 

		void					setMotionConstraintConfig(const PxClothMotionConstraintConfig& config);
		PxClothMotionConstraintConfig getMotionConstraintConfig() const;

		void					setSeparationConstraints(const PxClothParticleSeparationConstraint* separationConstraints);
		bool					getSeparationConstraints(PxClothParticleSeparationConstraint* separationConstraintsBuffer) const;
		PxU32					getNbSeparationConstraints() const; 

		void					clearInterpolation();

		void					setParticleAccelerations(const PxVec4* particleAccelerations);
		bool					getParticleAccelerations(PxVec4* particleAccelerationsBuffer) const;
		PxU32					getNbParticleAccelerations() const; 

		void					addCollisionSphere(const PxClothCollisionSphere& sphere);
		void					removeCollisionSphere(PxU32 index);
		void					setCollisionSpheres(const PxClothCollisionSphere* spheresBuffer, PxU32 count);
		PxU32					getNbCollisionSpheres() const;

		void					getCollisionData(PxClothCollisionSphere* spheresBuffer, PxU32* capsulesBuffer, 
									PxClothCollisionPlane* planesBuffer, PxU32* convexesBuffer, PxClothCollisionTriangle* trianglesBuffer) const;

		void					addCollisionCapsule(PxU32 first, PxU32 second);
		void					removeCollisionCapsule(PxU32 index);
		PxU32					getNbCollisionCapsules() const;

		void					addCollisionTriangle(const PxClothCollisionTriangle& triangle);
		void					removeCollisionTriangle(PxU32 index);
		void					setCollisionTriangles(const PxClothCollisionTriangle* trianglesBuffer, PxU32 count);
		PxU32					getNbCollisionTriangles() const;

		void					addCollisionPlane(const PxClothCollisionPlane& plane);
		void					removeCollisionPlane(PxU32 index);
		void					setCollisionPlanes(const PxClothCollisionPlane* planesBuffer, PxU32 count);
		PxU32					getNbCollisionPlanes() const;

		void					addCollisionConvex(PxU32 mask);
		void					removeCollisionConvex(PxU32 index);
		PxU32					getNbCollisionConvexes() const;

        void					setVirtualParticles(PxU32 numParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights);

		PxU32					getNbVirtualParticles() const;
		void					getVirtualParticles(PxU32* indicesBuffer) const;

		PxU32					getNbVirtualParticleWeights() const;
		void					getVirtualParticleWeights(PxVec3* weightsBuffer) const;

		PxTransform				getGlobalPose() const;
		void					setGlobalPose(const PxTransform& pose);

		void					setTargetPose(const PxTransform& pose);

		PxVec3					getExternalAcceleration() const;
		void					setExternalAcceleration(PxVec3 acceleration);

		PxVec3					getDampingCoefficient() const;
		void					setDampingCoefficient(PxVec3 dampingCoefficient);

		PxReal					getFrictionCoefficient() const;
		void					setFrictionCoefficient(PxReal frictionCoefficient);

		PxVec3					getLinearDragCoefficient() const;
		void					setLinearDragCoefficient(PxVec3 dragCoefficient);
		PxVec3					getAngularDragCoefficient() const;
		void					setAngularDragCoefficient(PxVec3 dragCoefficient);

		PxReal					getCollisionMassScale() const;
		void					setCollisionMassScale(PxReal scalingCoefficient);

		void					setSelfCollisionDistance(PxReal distance);
		PxReal					getSelfCollisionDistance() const;
		void					setSelfCollisionStiffness(PxReal stiffness);
		PxReal					getSelfCollisionStiffness() const;

		void					setSelfCollisionIndices(const PxU32* indices, PxU32 nbIndices);
		bool					getSelfCollisionIndices(PxU32* indices) const;
		PxU32					getNbSelfCollisionIndices() const;

		void					setRestPositions(const PxVec4* restPositions);
		bool					getRestPositions(PxVec4* restPositions) const;
		PxU32					getNbRestPositions() const;

		PxReal					getSolverFrequency() const;
		void					setSolverFrequency(PxReal);

		PxReal					getStiffnessFrequency() const;
		void					setStiffnessFrequency(PxReal);

		PxVec3                  getLinearInertiaScale() const;
		void                    setLinearInertiaScale( PxVec3 );
		PxVec3                  getAngularInertiaScale() const;
		void                    setAngularInertiaScale( PxVec3 );
		PxVec3                  getCentrifugalInertiaScale() const;
		void                    setCentrifugalInertiaScale( PxVec3 );

		PxClothStretchConfig	getStretchConfig(PxClothFabricPhaseType::Enum type) const;
		PxClothTetherConfig		getTetherConfig() const;

		void					setStretchConfig(PxClothFabricPhaseType::Enum type, const PxClothStretchConfig& config);
		void					setTetherConfig(const PxClothTetherConfig& config);

		PxClothFlags			getClothFlags() const;
		void					setClothFlags(PxClothFlags flags);

		PxVec3					getWindVelocity() const;
		void					setWindVelocity(PxVec3);
		PxReal					getDragCoefficient() const;
		void					setDragCoefficient(PxReal);
		PxReal					getLiftCoefficient() const;
		void					setLiftCoefficient(PxReal);


		bool					isSleeping() const;
		PxReal					getSleepLinearVelocity() const;
		void					setSleepLinearVelocity(PxReal threshold);
		void					setWakeCounter(PxReal wakeCounterValue);
		PxReal					getWakeCounter() const;
		void					wakeUp(PxReal wakeCounter);
		void					putToSleep();

		void					getParticleData(PxClothParticleData& readData);
		void					unlockParticleData();

		PxReal					getPreviousTimeStep() const;

		PxBounds3				getWorldBounds() const;

		void					setSimulationFilterData(const PxFilterData& data);
		PxFilterData			getSimulationFilterData() const;

		void					setContactOffset(PxReal);
		PxReal					getContactOffset() const;
		void					setRestOffset(PxReal);
		PxReal					getRestOffset() const;

	public:
		ClothSim*				getSim() const;

		PxCloth*				getPxCloth();

		PX_FORCE_INLINE cloth::Cloth* getLowLevelCloth() { return mLowLevelCloth; }

		void					onOriginShift(const PxVec3& shift);

		void					switchCloth(cloth::Cloth*);
		PX_FORCE_INLINE bool	isGpu() const { return mClothFlags & PxClothFlag::eCUDA; }

	private:

		void					updateBulkData(ClothBulkData& bulkData);
		void					initLowLevel(const PxTransform& globalPose, const PxClothParticle* particles);

	private:
		PxVec3						mExternalAcceleration;
		cloth::Cloth*				mLowLevelCloth;
		ClothFabricCore*			mFabric;
		ClothBulkData*				mBulkData;
		cloth::PhaseConfig*			mPhaseConfigs;
		PxFilterData				mFilterData;
		PxClothFlags				mClothFlags;
		PxReal						mContactOffset;
		PxReal						mRestOffset;
        
	public: // 
		PxU32						mNumUserSpheres;
		PxU32						mNumUserCapsules;
		PxU32						mNumUserPlanes;
		PxU32						mNumUserConvexes;
		PxU32						mNumUserTriangles;
    };

} // namespace Sc

}

#endif // PX_USE_CLOTH_API

#endif
