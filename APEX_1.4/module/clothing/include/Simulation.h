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



#ifndef SIMULATION_H
#define SIMULATION_H

#include "SimulationAbstract.h"

#include <PsHashMap.h>
#include <PxTask.h>
#include <PxProfileZone.h>
#include <PxProfileZoneManager.h>

#include "Range.h"
#include "Types.h"
#include "PhaseConfig.h"

#include "ClothStructs.h"

#if PX_WINDOWS_FAMILY
#include "ApexString.h"
#endif


namespace physx
{
	namespace pvdsdk
	{
		class ApexPvdClient;
	}
}

namespace nvidia
{
using namespace physx::profile;

namespace cloth
{
class Cloth;
class Factory;
class Solver;
}


namespace apex
{
class SceneIntl;
class DebugRenderParams;
class RenderDebugInterface;
}
namespace clothing
{
class ModuleClothingImpl;
class ClothingCookedPhysX3Param;
class ClothingDebugRenderParams;

class ClothingConvexImpl;


struct TriangleMeshId
{
	TriangleMeshId(uint32_t id_, uint32_t numTriangles_) : id(id_), numTriangles(numTriangles_)
	{
	}

	uint32_t id;
	uint32_t numTriangles;

	bool operator<(const TriangleMeshId& other) const
	{
		return id < other.id;
	}
};

class Simulation : public SimulationAbstract
{
public:

	Simulation(ClothingScene* clothingScene, bool useCuda);
	virtual ~Simulation();

	virtual bool needsExpensiveCreation();
	virtual bool needsAdaptiveTargetFrequency();
	virtual bool needsManualSubstepping();
	virtual bool needsLocalSpaceGravity();
	virtual uint32_t getNumSolverIterations() const;
	virtual SimulationType::Enum getType() const { return SimulationType::CLOTH3x; }
	virtual bool setCookedData(NvParameterized::Interface* cookedData, float actorScale);
	virtual bool initPhysics(uint32_t _physicalMeshId, uint32_t* indices, PxVec3* restPositions, tMaterial* material, const PxMat44& globalPose, const PxVec3& scaledGravity, bool localSpaceSim);

	virtual void initCollision(tBoneActor* boneActors, uint32_t numBoneActors,
		tBoneSphere* boneSpheres, uint32_t numBoneSpheres,
		uint16_t* spherePairIndices, uint32_t numSpherePairs,
		tBonePlane* bonePlanes, uint32_t numBonePlanes,
		uint32_t* convexes, uint32_t numConvexes,
		tBoneEntry* bones, const PxMat44* boneTransforms,
		ResourceList& actorPlanes,
		ResourceList& actorConvexes,
		ResourceList& actorSpheres,
		ResourceList& actorCapsules,
		ResourceList& actorTriangleMeshes,
		const tActorDescTemplate& actorDesc, const tShapeDescTemplate& shapeDesc, float actorScale,
		const PxMat44& globalPose, bool localSpaceSim);

	virtual void updateCollision(tBoneActor* boneActors, uint32_t numBoneActors,
		tBoneSphere* boneSpheres, uint32_t numBoneSpheres,
		tBonePlane* bonePlanes, uint32_t numBonePlanes,
		tBoneEntry* bones, const PxMat44* boneTransforms,
		ResourceList& actorPlanes,
		ResourceList& actorConvexes,
		ResourceList& actorSpheres,
		ResourceList& actorCapsules,
		ResourceList& actorTriangleMeshes,
		bool teleport);

	virtual void releaseCollision(ClothingCollisionImpl& releaseCollision);

	virtual void updateCollisionDescs(const tActorDescTemplate& actorDesc, const tShapeDescTemplate& shapeDesc);

	virtual void disablePhysX(Actor* dummy);
	virtual void reenablePhysX(Actor* newMaster, const PxMat44& globalPose);

	virtual void fetchResults(bool computePhysicsMeshNormals);
	virtual bool isSimulationMeshDirty() const;
	virtual void clearSimulationMeshDirt();

	virtual void setStatic(bool on);
	virtual bool applyPressure(float pressure);

	virtual bool raycast(const PxVec3& rayOrigin, const PxVec3& rayDirection, float& hitTime, PxVec3& hitNormal, uint32_t& vertexIndex);
	virtual void attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& globalPosition);
	virtual void freeVertex(uint32_t vertexIndex);

	virtual void setGlobalPose(const PxMat44& globalPose);
	virtual void applyGlobalPose();

	virtual NvParameterized::Interface* getCookedData();

	// debugging and debug rendering
	virtual void verifyTimeStep(float substepSize);
	virtual void visualize(RenderDebugInterface& renderDebug, ClothingDebugRenderParams& clothingDebugParams);
#ifndef WITHOUT_PVD
	virtual void updatePvd(pvdsdk::PvdDataStream& pvdStream, pvdsdk::PvdUserRenderer& pvdRenderer, ApexResourceInterface* clothingActor, bool localSpaceSim);
#endif
	virtual GpuSimMemType::Enum getGpuSimMemType() const;

	// R/W Access to simulation data
	virtual void setPositions(PxVec3* positions);
	virtual void setConstrainCoefficients(const tConstrainCoeffs* assetCoeffs, float maxDistanceBias, float maxDistanceScale, float maxDistanceDeform, float actorScale);
	virtual void getVelocities(PxVec3* velocities) const;
	virtual void setVelocities(PxVec3* velocities);
	virtual bool applyWind(PxVec3* velocities, const PxVec3* normals, const tConstrainCoeffs* assetCoeffs, const PxVec3& wind, float adaption, float dt);

	// actually important
	virtual void setTeleportWeight(float weight, bool reset, bool localSpaceSim);
	virtual void setSolverIterations(uint32_t iterations);
	virtual void updateConstrainPositions(bool isDirty);
	virtual bool applyClothingMaterial(tMaterial* material, PxVec3 scaledGravity);
	virtual void applyClothingDesc(tClothingDescTemplate& clothingTemplate);
	virtual void setInterCollisionChannels(uint32_t channels);
	virtual void setHalfPrecisionOption(bool isAllowed);

#if APEX_UE4
	virtual void simulate(float dt);
#endif

	// cleanup code
	static void releaseFabric(NvParameterized::Interface* cookedData);

private:
	void						applyCollision();

	void						setRestPositions(bool on);

#ifndef WITHOUT_DEBUG_VISUALIZE
	void						visualizeConvexes(RenderDebugInterface& renderDebug);
	void						visualizeConvexesInvalid(RenderDebugInterface& renderDebug);
	void						createAttenuationData();
#endif

	struct MappedArray
	{
		cloth::Range<PxVec4> deviceMemory;
		nvidia::Array<PxVec4> hostMemory;
	};

	static bool	allocateHostMemory(MappedArray& mappedMemory);

	// data owned by asset or actor
	ClothingCookedPhysX3Param*				mCookedData;

	// data owned by asset
	const uint32_t*							mIndices;
	const PxVec3*							mRestPositions;
	const tConstrainCoeffs*					mConstrainCoeffs;

	// own data
	cloth::Cloth*							mCloth;

	nvidia::Array<uint32_t>					mCollisionCapsules;
	nvidia::Array<uint32_t>					mCollisionCapsulesInvalid;
	nvidia::Array<PxVec4>					mCollisionSpheres;
	nvidia::Array<PxVec4>					mCollisionPlanes;
	nvidia::Array<uint32_t>					mCollisionConvexes;
	nvidia::Array<ClothingConvexImpl*>		mCollisionConvexesInvalid;
	nvidia::Array<PxVec3>					mCollisionTrianglesOld;
	nvidia::Array<PxVec3>					mCollisionTriangles;
	uint32_t								mNumAssetSpheres;
	uint32_t								mNumAssetCapsules;
	uint32_t								mNumAssetCapsulesInvalid;
	uint32_t								mNumAssetConvexes;
	nvidia::Array<uint32_t>					mReleasedSphereIds;
	nvidia::Array<uint32_t>					mReleasedPlaneIds;

	struct ConstrainConstants
	{
		ConstrainConstants() : motionConstrainDistance(0.0f), backstopDistance(0.0f), backstopRadius(0.0f) {}
		float motionConstrainDistance;
		float backstopDistance;
		float backstopRadius;
	};

	nvidia::Array<ConstrainConstants>		mConstrainConstants;
	bool									mConstrainConstantsDirty;
	float									mMotionConstrainScale;
	float									mMotionConstrainBias;
	int32_t									mNumBackstopConstraints;
	nvidia::Array<PxVec4>					mBackstopConstraints;

	PxVec3									mScaledGravity;
	float									mLastTimestep;

	nvidia::Array<cloth::PhaseConfig>		mPhaseConfigs;

	bool									mLocalSpaceSim;

	nvidia::Array<uint32_t>					mSelfCollisionAttenuationPairs;
	nvidia::Array<float>					mSelfCollisionAttenuationValues;

	PX_ALIGN(16, PxMat44 mGlobalPose);
	PxMat44 mGlobalPosePrevious;
	PxMat44 mGlobalPoseNormalized;
	PxMat44 mGlobalPoseNormalizedInv;
	float	mActorScale;
	float	mTetherLimit;
	bool	mTeleported;
	bool	mIsStatic;
};



class ClothingSceneSimulateTask : public PxTask, public UserAllocated
{
public:
	ClothingSceneSimulateTask(SceneIntl* apexScene, ClothingScene* scene, ModuleClothingImpl* module, profile::PxProfileZoneManager* manager);
	virtual ~ClothingSceneSimulateTask();

	void setWaitTask(PxBaseTask* waitForSolver);

	void setDeltaTime(float simulationDelta);
	float getDeltaTime();
	
	// this must only be called from ClothingScene::getClothSolver() !!!
	cloth::Solver*	getSolver(ClothFactory factory);

	void clearGpuSolver();

	virtual void        run();
	virtual const char* getName() const;

private:
	static bool interCollisionFilter(void* user0, void* user1);

	ModuleClothingImpl*		mModule;
	SceneIntl*				mApexScene;
	ClothingScene*			mScene;
	float					mSimulationDelta;

	cloth::Solver*			mSolverGPU;
	cloth::Solver*			mSolverCPU;
	PxProfileZone*			mProfileSolverGPU;
	PxProfileZone*			mProfileSolverCPU;

	PxBaseTask*				mWaitForSolverTask;

	profile::PxProfileZoneManager*	mProfileManager;
	bool					mFailedGpuFactory;
};


class WaitForSolverTask : public PxTask, public UserAllocated
{
public:
	WaitForSolverTask(ClothingScene* scene);

	virtual void        run();
	virtual const char* getName() const;

private:
	ClothingScene*	mScene;
};


}
} // namespace nvidia

#endif // SIMULATION_H
