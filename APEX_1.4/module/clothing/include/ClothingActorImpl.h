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



#ifndef CLOTHING_ACTOR_IMPL_H
#define CLOTHING_ACTOR_IMPL_H

#include "ApexActor.h"
#include "ModuleClothingHelpers.h"

#include "ClothingActorParam.h"
#include "ClothingMaterialLibraryParameters.h"
#include "ClothingActorTasks.h"

#include "ClothingActor.h"
#include "ClothingActorData.h"

#include "ClothingCollisionImpl.h"

#include "ClothingRenderProxyImpl.h"

#if !APEX_UE4
#include "PsSync.h"
#endif

#pragma warning(push)
#pragma warning(disable:4324)

namespace physx
{
	namespace pvdsdk
	{
		class PvdDataStream;
		class PvdConnectionManager;
	}
}

namespace nvidia
{
namespace apex
{
class RenderMeshActorIntl;
class RenderDebugInterface;
class ClothingVelocityCallback;
}
namespace clothing
{

class ClothingActorProxy;
class ClothingAssetImpl;
class ClothingCookingTask;
class ClothingMaterial;
class ClothingPreviewProxy;
class ClothingScene;
class ClothingCookedParam;
class SimulationAbstract;

namespace ClothingMaterialLibraryParametersNS
{
struct ClothingMaterial_Type;
}

namespace ClothingPhysicalMeshParametersNS
{
struct SkinClothMapB_Type;
struct SkinClothMapD_Type;
struct PhysicalMesh_Type;
}



struct ClothingGraphicalMeshActor
{
	ClothingGraphicalMeshActor() : active(false), needsTangents(false), renderProxy(NULL)
	{
	}

	bool active;
	bool needsTangents;
	Array<uint32_t> morphTargetVertexOffsets;

	ClothingRenderProxyImpl* renderProxy;
};


#if !APEX_UE4
class ClothingWaitForFetchTask : public PxTask
{
public:
	ClothingWaitForFetchTask()
	{
		mWaiting.set();
	}

	virtual void run();
	virtual void release();
	virtual const char* getName() const;

	Sync mWaiting;
};
#endif


class ClothingActorImpl : public ApexActor, public ApexResource
{
public:
	ClothingActorImpl(const NvParameterized::Interface& desc, ClothingActorProxy* apiProxy, ClothingPreviewProxy*, ClothingAssetImpl* asset, ClothingScene* scene);

	ClothingActorProxy*		mActorProxy;
	ClothingPreviewProxy*	mPreviewProxy;

	// from ApexInterface
	void release();

	// from Actor
	PX_INLINE ClothingAssetImpl* getOwner() const
	{
		return mAsset;
	}
	Renderable* getRenderable();

	// from Renderable
	void dispatchRenderResources(UserRenderer& api);

	void initializeActorData();

	void fetchResults();
	void waitForFetchResults();

	void syncActorData();
	void reinitActorData()
	{
		bReinitActorData = 1;
	}

	ClothingActorData& getActorData();

	void markRenderProxyReady();

	// from ResourceProvider
	void lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}
	void unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}
	void updateRenderResources(bool rewriteBuffers, void* userRenderData);

	// from ClothingActor
	NvParameterized::Interface* getActorDesc();
	void updateState(const PxMat44& globalPose, const PxMat44* newBoneMatrices, uint32_t boneMatricesByteStride, uint32_t numBoneMatrices, ClothingTeleportMode::Enum teleportMode);
	void updateMaxDistanceScale(float scale, bool multipliable);
	const PxMat44& getGlobalPose() const;
	void setWind(float windAdaption, const PxVec3& windVelocity);
	void setMaxDistanceBlendTime(float blendTime);
	float getMaxDistanceBlendTime() const;
	void setVisible(bool enable);
	bool isVisibleBuffered() const;
	bool isVisible() const;
	void setFrozen(bool enable);
	bool isFrozenBuffered() const;
	bool shouldComputeRenderData() const;
	ClothSolverMode::Enum getClothSolverMode() const;
	void setGraphicalLOD(uint32_t lod);
	uint32_t getGraphicalLod();

	ClothingRenderProxy*	acquireRenderProxy();

	bool rayCast(const PxVec3& worldOrigin, const PxVec3& worldDirection, float& time, PxVec3& normal, uint32_t& vertexIndex);
	void attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& worldPosition);
	void freeVertex(uint32_t vertexIndex);

	uint32_t getClothingMaterial() const;
	void setClothingMaterial(uint32_t index);
	void setOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName);
	void setVelocityCallback(ClothingVelocityCallback* callback)
	{
		mVelocityCallback = callback;
	}
	void setInterCollisionChannels(uint32_t channels)
	{
		mInterCollisionChannels = channels;
	}
	uint32_t getInterCollisionChannels()
	{
		return mInterCollisionChannels;
	}
	bool isHalfPrecisionAllowed() const
	{
		return mIsAllowedHalfPrecisionSolver;
	}
	void setHalfPrecision(bool isAllowed)
	{
		mIsAllowedHalfPrecisionSolver = isAllowed;
	}

	virtual void getLodRange(float& min, float& max, bool& intOnly) const;
	virtual float getActiveLod() const;
	virtual void forceLod(float lod);

	virtual void getPhysicalMeshPositions(void* buffer, uint32_t byteStride);
	virtual void getPhysicalMeshNormals(void* buffer, uint32_t byteStride);
	virtual float getMaximumSimulationBudget() const;
	virtual uint32_t getNumSimulationVertices() const;
	virtual const PxVec3* getSimulationPositions();
	virtual const PxVec3* getSimulationNormals();
	virtual bool getSimulationVelocities(PxVec3* velocities);
	virtual uint32_t getNumGraphicalVerticesActive(uint32_t submeshIndex) const;
	virtual PxMat44 getRenderGlobalPose() const;
	virtual const PxMat44* getCurrentBoneSkinningMatrices() const;

	// PhysX scene management
#if PX_PHYSICS_VERSION_MAJOR == 3
	void setPhysXScene(PxScene*);
	PxScene* getPhysXScene() const;
#endif

	// scene ticks
	void tickSynchBeforeSimulate_LocksPhysX(float simulationDelta, float substepSize, uint32_t substepNumber, uint32_t numSubSteps);
	void applyLockingTasks();
	void updateConstrainPositions_LocksPhysX();
	void applyCollision_LocksPhysX();
	void applyGlobalPose_LocksPhysX();
	void applyClothingMaterial_LocksPhysX();
	void skinPhysicsMesh(bool useInterpolatedMatrices, float substepFraction);
	void tickAsynch_NoPhysX();
	bool needsManualSubstepping();
	bool isSkinningDirty();

	// LoD stuff
	float getCost() const; // Should be removed?

	// debug rendering
#ifndef WITHOUT_PVD
	void initPvdInstances(pvdsdk::PvdDataStream& pvdStream);
	void destroyPvdInstances();
	void updatePvd();
#endif
	void visualize();

	// cleanup
	void destroy();

	// Tasks
#if APEX_UE4
	void initBeforeTickTasks(PxF32 deltaTime, PxF32 substepSize, PxU32 numSubSteps, PxTaskManager* taskManager, PxTaskID before, PxTaskID after);
#else
	void initBeforeTickTasks(float deltaTime, float substepSize, uint32_t numSubSteps);
#endif

	void submitTasksDuring(PxTaskManager* taskManager);
	void setTaskDependenciesBefore(PxBaseTask* after);
	PxTaskID setTaskDependenciesDuring(PxTaskID before, PxTaskID after);

	void startBeforeTickTask();

	PxTaskID getDuringTickTaskID()
	{
		return mDuringTickTask.getTaskID();
	}

#if !APEX_UE4
	void setFetchContinuation();
#endif
	void startFetchTasks();

	// teleport
	void applyTeleport(bool skinningReady, uint32_t substepNumber);

	// validation
	static bool isValidDesc(const NvParameterized::Interface& params);

	// Per Actor runtime cooking stuff
	float getActorScale()
	{
		return mActorDesc->actorScale;
	}
	bool getHardwareAllowed()
	{
		return mActorDesc->useHardwareCloth;
	}
	ClothingCookedParam* getRuntimeCookedDataPhysX();


	// collision functions
	virtual ClothingPlane* createCollisionPlane(const PxPlane& pose);
	virtual ClothingConvex* createCollisionConvex(ClothingPlane** planes, uint32_t numPlanes);
	virtual ClothingSphere* createCollisionSphere(const PxVec3& positions, float radius);
	virtual ClothingCapsule* createCollisionCapsule(ClothingSphere& sphere1, ClothingSphere& sphere2);
	virtual ClothingTriangleMesh* createCollisionTriangleMesh();

	void releaseCollision(ClothingCollisionImpl& collision);
	void notifyCollisionChange()
	{
		bActorCollisionChanged = 1;
	}

#if APEX_UE4
	void simulate(PxF32 dt);
	void setFetchResultsSync() { mFetchResultsSync.set(); }
#endif

protected:
	struct WriteBackInfo;

	// rendering
	void updateBoneBuffer(ClothingRenderProxyImpl* renderProxy);
	void updateRenderMeshActorBuffer(bool freeBuffers, uint32_t graphicalLodId);
	PxBounds3 getRenderMeshAssetBoundsTransformed();
	void updateRenderProxy();

	// handling interpolated skinning matrices
	bool allocateEnoughBoneBuffers_NoPhysX(bool prepareForSubstepping);

	// double buffering internal stuff
	bool isSimulationRunning() const;
	void updateScaledGravity(float substepSize);
	void updateStateInternal_NoPhysX(bool prepareForSubstepping);

	// compute intensive skinning stuff
	template<bool withBackstop>
	void skinPhysicsMeshInternal(bool useInterpolatedMatrices, float substepFraction);
	void fillWritebackData_LocksPhysX(const WriteBackInfo& writeBackInfo);

	// wind
	void applyVelocityChanges_LocksPhysX(float simulationDelta);

	// handling entities
	bool isCookedDataReady();
	void getSimulation(const WriteBackInfo& writeBackInfo);
	void createPhysX_LocksPhysX(float simulationDelta);
	void removePhysX_LocksPhysX();
	void changePhysicsMesh_LocksPhysX(uint32_t oldGraphicalLodId, float simulationDelta);
	void updateCollision_LocksPhysX(bool useInterpolatedMatrices);
	void updateConstraintCoefficients_LocksPhysX();
	void copyPositionAndNormal_NoPhysX(uint32_t numCopyVertices, SimulationAbstract* oldClothingSimulation);
	void copyAndComputeVelocities_LocksPhysX(uint32_t numCopyVertices, SimulationAbstract* oldClothingSimulation, PxVec3* velocities, float simulationDelta) const;
	void transferVelocities_LocksPhysX(const SimulationAbstract& oldClothingSimulation,
									const ClothingPhysicalMeshParametersNS::SkinClothMapB_Type* pTCMB,
									const ClothingPhysicalMeshParametersNS::SkinClothMapD_Type* pTCM,
									uint32_t numVerticesInMap, const uint32_t* srcIndices, uint32_t numSrcIndices, uint32_t numSrcVertices,
									PxVec3* oldVelocities, PxVec3* newVelocites, float simulationDelta);
	PxVec3 computeVertexVelFromAnim(uint32_t vertexIndex, const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh, float simulationDelta) const;
	void freeze_LocksPhysX(bool on);

	// handling Abstract Simulation
	void createSimulation(uint32_t physicalMeshId, NvParameterized::Interface* cookedData, const WriteBackInfo& writeBackInfo);


	// handling Lod
	uint32_t getGraphicalMeshIndex(uint32_t lod) const;
	void lodTick_LocksPhysX(float simulationDelta);

	// debug rendering
	void visualizeSkinnedPositions(RenderDebugInterface& renderDebug, float positionRadius, bool maxDistanceOut, bool maxDistanceIn) const;
	void visualizeBackstop(RenderDebugInterface& renderDebug) const;
	void visualizeBackstopPrecise(RenderDebugInterface& renderDebug, float scale) const;
	void visualizeBoneConnections(RenderDebugInterface& renderDebug, const PxVec3* positions, const uint16_t* boneIndices,
								const float* boneWeights, uint32_t numBonesPerVertex, uint32_t numVertices) const;
	void visualizeSpheres(RenderDebugInterface& renderDebug, const PxVec3* positions, uint32_t numPositions, float radius, uint32_t color, bool wire) const;

	RenderMeshActorIntl* createRenderMeshActor(RenderMeshAssetIntl* renderMeshAsset);

	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* getCurrentClothingMaterial() const;
	bool clothingMaterialsEqual(ClothingMaterialLibraryParametersNS::ClothingMaterial_Type& a, ClothingMaterialLibraryParametersNS::ClothingMaterial_Type& b);

	struct WriteBackInfo
	{
		WriteBackInfo() : oldSimulation(NULL), oldGraphicalLodId(0), simulationDelta(0.0f) {}
		SimulationAbstract* oldSimulation;
		uint32_t oldGraphicalLodId;
		float simulationDelta;
	};

	// internal variables
	ClothingAssetImpl*		mAsset;
	ClothingScene*			mClothingScene;
#if PX_PHYSICS_VERSION_MAJOR == 3
	PxScene*				mPhysXScene;
#endif

	ClothingActorParam*		mActorDesc;
	const char*				mBackendName;

	// current pose of this actor. If using skinning it could be set to idenftity matrix and simply rely on the skinning matrices (bones)
	PX_ALIGN(16, PxMat44)	mInternalGlobalPose;
	PxMat44					mOldInternalGlobalPose;
	PxMat44					mInternalInterpolatedGlobalPose;

	// Bone matrices for physical mesh skinning - provided by the application either in the initial ClothingActorDesc or "updateBoneMatrices"
	// number 0 is current, 1 is one frame old, 2 is 2 frames old
	//PxMat44* mInternalBoneMatrices[2];
	PxMat44*				mInternalInterpolatedBoneMatrices;

	// Number of cloth solver iterations that will be performed.
	//  If this is set to 0 the simulation enters "static" mode
	uint32_t				mCurrentSolverIterations;

	PxVec3					mInternalScaledGravity;

	float					mInternalMaxDistanceBlendTime;
	float					mMaxDistReduction;

	uint32_t				mBufferedGraphicalLod;
	uint32_t				mCurrentGraphicalLodId;


	Array<ClothingGraphicalMeshActor> mGraphicalMeshes;
	
	Mutex					mRenderProxyMutex;
	ClothingRenderProxyImpl*	mRenderProxyReady;

	// for backwards compatibility, to make sure that dispatchRenderResources is
	// called on the same render proxy as updateRenderResources
	ClothingRenderProxyImpl*	mRenderProxyURR;

	ClothingActorData		mData;

	// PhysX SDK simulation objects
	// Per-actor "skinned physical mesh" data is in mSimulationBulk.dynamicSimulationData
	//  these are skinned by APEX in "tickAsynch" using bone matrices provided by the application
	//  they are passed as constraints to the PhysX SDK (mCloth) in "tickSync"
	// Per-actor "skinned rendering mesh" data - only used in if the ClothingAssetImpl uses the "skin cloth" approach
	//  these are skinned by APEX in "tickSynch" using the appropriate mesh-to-mesh skinning algorithm in the ClothingAssetImpl
	SimulationAbstract*		mClothingSimulation;

	// Wind
	ClothingActorParamNS::WindParameters_Type mInternalWindParams;

	ClothingActorParamNS::ClothingActorFlags_Type mInternalFlags;

	// max distance scale
	ClothingActorParamNS::MaxDistanceScale_Type mInternalMaxDistanceScale;

	// only needed to detect when it changes on the fly
	float							mCurrentMaxDistanceBias;

	// bounds that are computed for the current frame, get copied over to RenderMeshActor during fetchResults
	PxBounds3	mNewBounds;

	bool							bIsSimulationOn;
	int32_t							mForceSimulation;
	PxVec3							mLodCentroid;
	float							mLodRadiusSquared;

	// The Clothing Material
	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type mClothingMaterial;
	
	// velocity callback
	ClothingVelocityCallback*		mVelocityCallback;

	// inter collision
	uint32_t						mInterCollisionChannels;

	bool							mIsAllowedHalfPrecisionSolver;

	// The tasks
	ClothingActorBeforeTickTask		mBeforeTickTask;
	ClothingActorDuringTickTask		mDuringTickTask;
	ClothingActorFetchResultsTask	mFetchResultsTask;

	ClothingCookingTask*			mActiveCookingTask;

#if APEX_UE4
	physx::shdfnd::Sync				mFetchResultsSync;
#else
	ClothingWaitForFetchTask		mWaitForFetchTask;
#endif
	bool							mFetchResultsRunning;
	Mutex							mFetchResultsRunningMutex;

	nvidia::Array<PxVec3>			mWindDebugRendering;

	// TODO make a better overrideMaterials API
	HashMap<uint32_t, ApexSimpleString>	mOverrideMaterials;

	ResourceList mCollisionPlanes;
	ResourceList mCollisionConvexes;
	ResourceList mCollisionSpheres;
	ResourceList mCollisionCapsules;
	ResourceList mCollisionTriangleMeshes;

	// bit flags - aggregate them at the end
	uint32_t bGlobalPoseChanged : 1;						// mBufferedGlobalPose was updated
	uint32_t bBoneMatricesChanged : 1;						// mBufferedBoneMatrices were updated
	uint32_t bBoneBufferDirty : 1;							// need to sync bones to the render mesh actor
	uint32_t bMaxDistanceScaleChanged : 1;					// mBufferedMaxDistanceScale was updated
	uint32_t bBlendingAllowed : 1;

	uint32_t bDirtyActorTemplate : 1;
	uint32_t bDirtyShapeTemplate : 1;
	uint32_t bDirtyClothingTemplate : 1;

	uint32_t bBufferedVisible : 1;
	uint32_t bInternalVisible : 1;

	uint32_t bUpdateFrozenFlag : 1;
	uint32_t bBufferedFrozen : 1;
	uint32_t bInternalFrozen : 1;

	uint32_t bPressureWarning : 1;
	uint32_t bUnsucessfullCreation : 1;

	ClothingTeleportMode::Enum bInternalTeleportDue : 3;

	uint32_t bInternalScaledGravityChanged : 1;
	uint32_t bReinitActorData : 1;

	uint32_t bInternalLocalSpaceSim : 1;

	uint32_t bActorCollisionChanged : 1;
};

}
} // namespace nvidia


#pragma warning(pop)

#endif // CLOTHING_ACTOR_IMPL_H
