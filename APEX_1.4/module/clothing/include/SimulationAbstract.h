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



#ifndef SIMULATION_ABSTRACT_H
#define SIMULATION_ABSTRACT_H

#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"

#include "ApexDefs.h"

#include "ClothingActorParam.h"

// some params files
#include "ClothingMaterialLibraryParameters.h"
#include "ClothingAssetParameters.h"
#include "ClothingPhysicalMeshParameters.h"

#include "ApexPvdClient.h"

namespace physx
{
#if APEX_UE4
	class PxBaseTask;
#endif

	namespace pvdsdk
	{
		class PvdDataStream;
		class PvdUserRenderer;
	}
}

namespace nvidia
{
namespace apex
{
class ResourceList;
class RenderDebugInterface;
class ApexResourceInterface;
class Actor;
}
namespace clothing
{
class ClothingScene;
class ClothingDebugRenderParams;
class ClothingCollisionImpl;


struct GpuSimMemType
{
	enum Enum
	{
		UNDEFINED = -1,
		GLOBAL = 0,
		MIXED = 1,
		SHARED = 2
	};
};


struct SimulationType
{
	enum Enum
	{
		CLOTH2x,
		SOFTBODY2x,
		CLOTH3x
	};
};


class SimulationAbstract : public UserAllocated
{
public:
	typedef ClothingMaterialLibraryParametersNS::ClothingMaterial_Type	tMaterial;
	typedef ClothingActorParamNS::ClothDescTemplate_Type				tClothingDescTemplate;
	typedef ClothingActorParamNS::ActorDescTemplate_Type				tActorDescTemplate;
	typedef ClothingActorParamNS::ShapeDescTemplate_Type				tShapeDescTemplate;
	typedef ClothingAssetParametersNS::SimulationParams_Type			tSimParams;
	typedef ClothingAssetParametersNS::ActorEntry_Type					tBoneActor;
	typedef ClothingAssetParametersNS::BoneSphere_Type					tBoneSphere;
	typedef ClothingAssetParametersNS::BonePlane_Type					tBonePlane;
	typedef ClothingAssetParametersNS::BoneEntry_Type					tBoneEntry;
	typedef ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type	tConstrainCoeffs;

	SimulationAbstract(ClothingScene* clothingScene) : physicalMeshId(0xffffffff), submeshId(0xffffffff),
		skinnedPhysicsPositions(NULL), skinnedPhysicsNormals(NULL),
		sdkNumDeformableVertices(0), sdkWritebackPosition(NULL), sdkWritebackNormal(NULL), sdkNumDeformableIndices(0),
		mRegisteredActor(NULL), mClothingScene(clothingScene), mUseCuda(false)
	{
		::memset(&simulation, 0, sizeof(simulation));
	}

	virtual ~SimulationAbstract()
	{
		if (sdkWritebackPosition != NULL)
		{
			PX_FREE(sdkWritebackPosition);
			sdkWritebackPosition = sdkWritebackNormal = NULL;
		}

		if (skinnedPhysicsPositions != NULL)
		{
			PX_FREE(skinnedPhysicsPositions);
			skinnedPhysicsPositions = skinnedPhysicsNormals = NULL;
		}

		PX_ASSERT(mRegisteredActor == NULL);
	}

	void init(uint32_t numVertices, uint32_t numIndices, bool writebackNormals);
	void initSimulation(const ClothingAssetParametersNS::SimulationParams_Type& s);

	virtual bool needsExpensiveCreation() = 0;
	virtual bool needsAdaptiveTargetFrequency() = 0;
	virtual bool needsManualSubstepping() = 0;
	virtual bool needsLocalSpaceGravity() = 0;
	virtual SimulationType::Enum getType() const = 0;
	virtual bool isGpuSim() const { return mUseCuda; }
	virtual uint32_t getNumSolverIterations() const = 0;
	virtual GpuSimMemType::Enum getGpuSimMemType() const { return GpuSimMemType::UNDEFINED; }
	virtual bool setCookedData(NvParameterized::Interface* cookedData, float actorScale) = 0;
	virtual bool initPhysics(uint32_t physicalMeshId, uint32_t* indices, PxVec3* restPositions, tMaterial* material, const PxMat44& globalPose, const PxVec3& scaledGravity, bool localSpaceSim) = 0;

	// collision
	virtual void initCollision(		tBoneActor* boneActors, uint32_t numBoneActors,
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
									const PxMat44& globalPose, bool localSpaceSim) = 0;

	virtual void updateCollision(	tBoneActor* boneActors, uint32_t numBoneActors,
									tBoneSphere* boneSpheres, uint32_t numBoneSpheres,
									tBonePlane* bonePlanes, uint32_t numBonePlanes,
									tBoneEntry* bones, const PxMat44* boneTransforms,
									ResourceList& actorPlanes, 
									ResourceList& actorConvexes,
									ResourceList& actorSpheres,
									ResourceList& actorCapsules,
									ResourceList& actorTriangleMeshes,
									bool teleport) = 0;

	virtual void updateCollisionDescs(const tActorDescTemplate& actorDesc, const tShapeDescTemplate& shapeDesc) = 0;
	virtual void applyCollision() {};
	virtual void releaseCollision(ClothingCollisionImpl& /*collision*/) {}

	virtual void swapCollision(SimulationAbstract* /*oldSimulation*/) {}

	virtual void registerPhysX(Actor* actor)
	{
		PX_ASSERT(mRegisteredActor == NULL);
		mRegisteredActor = actor;

		PX_ASSERT(physicalMeshId != 0xffffffff);
	}

	virtual void unregisterPhysX()
	{
		PX_ASSERT(mRegisteredActor != NULL);
		mRegisteredActor = NULL;

		PX_ASSERT(physicalMeshId != 0xffffffff);
	}

	virtual void disablePhysX(Actor* dummy) = 0;
	virtual void reenablePhysX(Actor* newMaster, const PxMat44& globalPose) = 0;

	virtual void fetchResults(bool computePhysicsMeshNormals) = 0;
	virtual bool isSimulationMeshDirty() const = 0;
	virtual void clearSimulationMeshDirt() = 0;

	virtual void setStatic(bool on) = 0;
	virtual bool applyPressure(float pressure) = 0;

	virtual void setGlobalPose(const PxMat44& globalPose) = 0;
	virtual void applyGlobalPose() {};

	virtual bool raycast(const PxVec3& rayOrigin, const PxVec3& rayDirection, float& hitTime, PxVec3& hitNormal, uint32_t& vertexIndex) = 0;
	virtual void attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& globalPosition) = 0;
	virtual void freeVertex(uint32_t vertexIndex) = 0;

	virtual NvParameterized::Interface* getCookedData() = 0;

	// debugging and debug rendering
	virtual void verifyTimeStep(float substepSize) = 0;
	virtual void visualize(RenderDebugInterface& renderDebug, ClothingDebugRenderParams& clothingDebugParams) = 0;
#ifndef WITHOUT_PVD
	virtual void updatePvd(pvdsdk::PvdDataStream& /*pvdStream*/, pvdsdk::PvdUserRenderer& /*pvdRenderer*/, ApexResourceInterface* /*ClothingActorImpl*/, bool /*localSpaceSim*/) {};
#endif

	// R/W Access to simulation data
	virtual void setPositions(PxVec3* positions) = 0;
	virtual void setConstrainCoefficients(const tConstrainCoeffs* assetCoeffs, float maxDistanceBias, float maxDistanceScale, float maxDistanceDeform, float actorScale) = 0;
	virtual void getVelocities(PxVec3* velocities) const = 0;
	virtual void setVelocities(PxVec3* velocities) = 0;

	virtual bool applyWind(PxVec3* velocities, const PxVec3* normals, const tConstrainCoeffs* assetCoeffs, const PxVec3& wind, float adaption, float dt) = 0;

	// actually important
	virtual void setTeleportWeight(float weight, bool reset, bool localSpaceSim) = 0;
	virtual void setSolverIterations(uint32_t iterations) = 0;
	virtual void updateConstrainPositions(bool isDirty) = 0;
	virtual bool applyClothingMaterial(tMaterial* material, PxVec3 scaledGravity) = 0;
	virtual void applyClothingDesc(tClothingDescTemplate& clothingTemplate) = 0;
	virtual void setInterCollisionChannels(uint32_t /*channels*/) {};
	virtual void setHalfPrecisionOption(bool /*isAllowed*/) {};

	ClothingScene* getClothingScene() const
	{
		return mClothingScene;
	}

#if APEX_UE4
	virtual void simulate(float) {}
#endif

	uint32_t physicalMeshId;
	uint32_t submeshId;

	PxVec3* skinnedPhysicsPositions;
	PxVec3* skinnedPhysicsNormals;

	//  Results produced by PhysX SDK's Cloth simulation
	uint32_t sdkNumDeformableVertices;
	PxVec3* sdkWritebackPosition;
	PxVec3* sdkWritebackNormal;

	uint32_t sdkNumDeformableIndices;

protected:
	tSimParams simulation;

	Actor* mRegisteredActor;

	ClothingScene* mClothingScene;
	bool mUseCuda;
	
};

}
} // namespace nvidia


#endif // SIMULATION_ABSTRACT_H
