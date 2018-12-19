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



#ifndef __MODULE_DESTRUCTIBLE_IMPL_H__
#define __MODULE_DESTRUCTIBLE_IMPL_H__

#include "Apex.h"
#include "ApexSDKIntl.h"
#include "ModuleIntl.h"
#include "ModuleBase.h"

#include "FractureTools.h"

#include "ModuleDestructible.h"

#include "DestructibleActorJoint.h"
#include "DestructibleAssetImpl.h"
#include "PsMutex.h"
#include "ApexAuthorableObject.h"

#include "ApexSDKCachedDataImpl.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"
#include "ApexRand.h"

#include "ModuleDestructibleRegistration.h"
#ifndef USE_DESTRUCTIBLE_RWLOCK
#define USE_DESTRUCTIBLE_RWLOCK 0
#endif

#define DECLARE_DISABLE_COPY_AND_ASSIGN(Class) private: Class(const Class &); Class & operator = (const Class &);

namespace nvidia
{
namespace destructible
{

/*** SyncParams ***/
typedef UserDestructibleSyncHandler<DamageEventHeader>		UserDamageEventHandler;
typedef UserDestructibleSyncHandler<FractureEventHeader>	UserFractureEventHandler;
typedef UserDestructibleSyncHandler<ChunkTransformHeader>	UserChunkMotionHandler;

class DestructibleActorImpl;
class DestructibleScene;

class DestructibleModuleCachedData : public ModuleCachedDataIntl, public UserAllocated
{
public:
	DestructibleModuleCachedData(AuthObjTypeID moduleID);
	virtual ~DestructibleModuleCachedData();

	virtual AuthObjTypeID				getModuleID() const
	{
		return mModuleID;
	}

	virtual NvParameterized::Interface*	getCachedDataForAssetAtScale(Asset& asset, const PxVec3& scale);
	virtual PxFileBuf&			serialize(PxFileBuf& stream) const;
	virtual PxFileBuf&			deserialize(PxFileBuf& stream);
	virtual void						clear(bool force = true);

	void								clearAssetCollisionSet(const DestructibleAssetImpl& asset);
	physx::Array<PxConvexMesh*>*		getConvexMeshesForActor(const DestructibleActorImpl& destructible);	// increments ref counts
	void								releaseReferencesToConvexMeshesForActor(const DestructibleActorImpl& destructible); // decrements ref counts
	
	// The DestructibleActor::cacheModuleData() method needs to avoid incrementing the ref count
	physx::Array<PxConvexMesh*>*		getConvexMeshesForScale(const DestructibleAssetImpl& asset, const PxVec3& scale, bool incRef = true);

	DestructibleAssetCollision*			getAssetCollisionSet(const DestructibleAssetImpl& asset);

	virtual PxFileBuf& serializeSingleAsset(Asset& asset, PxFileBuf& stream);
	virtual PxFileBuf& deserializeSingleAsset(Asset& asset, PxFileBuf& stream);
private:
	DestructibleAssetCollision*			findAssetCollisionSet(const char* name);

	struct Version
	{
		enum Enum
		{
			First = 0,

			Count,
			Current = Count - 1
		};
	};

	AuthObjTypeID								mModuleID;
	physx::Array<DestructibleAssetCollision*>	mAssetCollisionSets;
};

class ModuleDestructibleImpl : public ModuleDestructible, public ModuleIntl, public ModuleBase, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ModuleDestructibleImpl(ApexSDKIntl* sdk);
	~ModuleDestructibleImpl();

	void						destroy();

	// base class methods
	void						init(NvParameterized::Interface& desc);
	NvParameterized::Interface* getDefaultModuleDesc();
	void						release()
	{
		ModuleBase::release();
	}
	const char*					getName() const
	{
		READ_ZONE();
		return ModuleBase::getName();
	}
	bool isInitialized()
	{
		return m_isInitialized;
	}

	bool setRenderLockMode(RenderLockMode::Enum, Scene&);

	RenderLockMode::Enum getRenderLockMode(const Scene&) const;

	bool lockModuleSceneRenderLock(Scene&);

	bool unlockModuleSceneRenderLock(Scene&);

	void						setIntValue(uint32_t parameterIndex, uint32_t value);
	ModuleSceneIntl* 				createInternalModuleScene(SceneIntl&, RenderDebugInterface*);
	void						releaseModuleSceneIntl(ModuleSceneIntl&);
	uint32_t				forceLoadAssets();
	AuthObjTypeID				getModuleID() const;
	RenderableIterator* 	createRenderableIterator(const Scene&);

	DestructibleActorJoint*	createDestructibleActorJoint(const DestructibleActorJointDesc&, Scene&);
	bool                        isDestructibleActorJointActive(const DestructibleActorJoint*, Scene&) const;

	void						setMaxDynamicChunkIslandCount(uint32_t maxCount);
	void						setMaxChunkCount(uint32_t maxCount);

	void						setSortByBenefit(bool sortByBenefit);
	void						setMaxChunkDepthOffset(uint32_t offset);
	void						setMaxChunkSeparationLOD(float separationLOD);

	void						setChunkReport(UserChunkReport* chunkReport);
	void						setImpactDamageReportCallback(UserImpactDamageReport* impactDamageReport);
	void						setChunkReportBitMask(uint32_t chunkReportBitMask);
	void						setDestructiblePhysXActorReport(UserDestructiblePhysXActorReport* destructiblePhysXActorReport);
	void						setChunkReportMaxFractureEventDepth(uint32_t chunkReportMaxFractureEventDepth);
	void						scheduleChunkStateEventCallback(DestructibleCallbackSchedule::Enum chunkStateEventCallbackSchedule);
	void						setChunkCrumbleReport(UserChunkParticleReport* chunkCrumbleReport);
	void						setChunkDustReport(UserChunkParticleReport* chunkDustReport);
	void						setWorldSupportPhysXScene(Scene& apexScene, PxScene* physxScene);

	bool						owns(const PxRigidActor* actor) const;
#if APEX_RUNTIME_FRACTURE
	bool						isRuntimeFractureShape(const PxShape& shape) const;
#endif

	DestructibleActor*		getDestructibleAndChunk(const PxShape* shape, int32_t* chunkIndex) const;

	void						applyRadiusDamage(Scene& scene, float damage, float momentum,
	        const PxVec3& position, float radius, bool falloff);

	void						setMaxActorCreatesPerFrame(uint32_t maxActorsPerFrame);
	void						setMaxFracturesProcessedPerFrame(uint32_t maxActorsPerFrame);
	void                        setValidBoundsPadding(float);

#if 0 // dead code
	void						releaseBufferedConvexMeshes();
#endif

	ModuleCachedDataIntl*		getModuleDataCache()
	{
		return mCachedData;
	}

	PX_INLINE NvParameterized::Interface* getApexDestructiblePreviewParams(void) const
	{
		return mApexDestructiblePreviewParams;
	}

	void						setUseLegacyChunkBoundsTesting(bool useLegacyChunkBoundsTesting);
	bool						getUseLegacyChunkBoundsTesting() const
	{
		return mUseLegacyChunkBoundsTesting;
	}

	void						setUseLegacyDamageRadiusSpread(bool useLegacyDamageRadiusSpread);
	bool						getUseLegacyDamageRadiusSpread() const
	{
		return mUseLegacyDamageRadiusSpread;
	}

	bool						setMassScaling(float massScale, float scaledMassExponent, Scene& apexScene);

	void						invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount, Scene& apexScene);
	
	void						setDamageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::Enum flags, Scene& apexScene);

	bool						setChunkCollisionHullCookingScale(const PxVec3& scale);

	PxVec3				getChunkCollisionHullCookingScale() const { READ_ZONE(); return mChunkCollisionHullCookingScale; }

	virtual class FractureToolsAPI*	getFractureTools() const
	{
		READ_ZONE();
#ifdef WITHOUT_APEX_AUTHORING
		APEX_DEBUG_WARNING("FractureTools are not available in release builds.");
#endif
		return (FractureToolsAPI*)mFractureTools;
	}

	DestructibleModuleParameters* getModuleParameters()
	{
		return mModuleParams;
	}

#if 0 // dead code
	physx::Array<PxConvexMesh*>			convexMeshKillList;
#endif

private:
	//	Private interface, used by Destructible* classes

	DestructibleScene* 				getDestructibleScene(const Scene& apexScene) const;

	bool									m_isInitialized;

	// Max chunk depth offset (for LOD) - effectively reduces the max chunk depth in all destructibles by this number
	uint32_t							m_maxChunkDepthOffset;
	// Where in the assets' min-max range to place the lifetime and max. separation
	float							m_maxChunkSeparationLOD;
	float							m_validBoundsPadding;
	uint32_t							m_maxFracturesProcessedPerFrame;
	uint32_t							m_maxActorsCreateablePerFrame;
	uint32_t							m_dynamicActorFIFOMax;
	uint32_t							m_chunkFIFOMax;
	bool									m_sortByBenefit;
	UserChunkReport*						m_chunkReport;
	UserImpactDamageReport*				m_impactDamageReport;
	uint32_t							m_chunkReportBitMask;
	UserDestructiblePhysXActorReport*		m_destructiblePhysXActorReport;
	uint32_t							m_chunkReportMaxFractureEventDepth;
	DestructibleCallbackSchedule::Enum	m_chunkStateEventCallbackSchedule;
	UserChunkParticleReport*				m_chunkCrumbleReport;
	UserChunkParticleReport*				m_chunkDustReport;

	float							m_massScale;
	float							m_scaledMassExponent;

	ResourceList							m_destructibleSceneList;

	ResourceList							mAuthorableObjects;

	NvParameterized::Interface*					mApexDestructiblePreviewParams;
	DestructibleModuleParameters*				mModuleParams;

	DestructibleModuleCachedData*				mCachedData;

	nvidia::QDSRand								mRandom;

	bool										mUseLegacyChunkBoundsTesting;
	bool										mUseLegacyDamageRadiusSpread;

	PxVec3								mChunkCollisionHullCookingScale;

	class FractureTools*						mFractureTools;

    /*** ModuleDestructibleImpl::SyncParams ***/
public:
	bool setSyncParams(UserDamageEventHandler * userDamageEventHandler, UserFractureEventHandler * userFractureEventHandler, UserChunkMotionHandler * userChunkMotionHandler);
public:
    class SyncParams
    {
		friend bool ModuleDestructibleImpl::setSyncParams(UserDamageEventHandler *, UserFractureEventHandler *, UserChunkMotionHandler *);
    public:
        SyncParams();
        ~SyncParams();
		UserDamageEventHandler *			getUserDamageEventHandler() const;
		UserFractureEventHandler *			getUserFractureEventHandler() const;
		UserChunkMotionHandler *			getUserChunkMotionHandler() const;
		template<typename T> uint32_t	getSize() const;
	private:
        DECLARE_DISABLE_COPY_AND_ASSIGN(SyncParams);
		UserDamageEventHandler *			userDamageEventHandler;
		UserFractureEventHandler *			userFractureEventHandler;
		UserChunkMotionHandler *			userChunkMotionHandler;
    };
	const ModuleDestructibleImpl::SyncParams &	getSyncParams() const;
private:
	SyncParams								mSyncParams;

private:
	friend class DestructibleActorImpl;
	friend class DestructibleActorJointImpl;
	friend class DestructibleAssetImpl;
	friend class DestructibleStructure;
	friend class DestructibleScene;
	friend class DestructibleContactReport;
	friend class DestructibleContactModify;
	friend class ApexDamageEventReportDataImpl;
	friend struct DestructibleNXActorCreator;
};



#ifndef WITHOUT_APEX_AUTHORING
// API for FractureTools from our shared/internal code
class FractureTools : public nvidia::apex::FractureToolsAPI, public UserAllocated 
{
public:
	virtual ~FractureTools() {};

	virtual ::FractureTools::CutoutSet* createCutoutSet()
	{
		return ::FractureTools::createCutoutSet();
	}

	virtual void buildCutoutSet(::FractureTools::CutoutSet& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight, float snapThreshold, bool periodic)
	{
		return ::FractureTools::buildCutoutSet(cutoutSet, pixelBuffer, bufferWidth, bufferHeight, snapThreshold, periodic);
	}

	virtual bool calculateCutoutUVMapping(const nvidia::ExplicitRenderTriangle& triangle, PxMat33& theMapping)
	{
		return ::FractureTools::calculateCutoutUVMapping(triangle, theMapping);
	}

	virtual bool calculateCutoutUVMapping(nvidia::ExplicitHierarchicalMesh& hMesh, const PxVec3& targetDirection, PxMat33& theMapping)
	{
		return ::FractureTools::calculateCutoutUVMapping(hMesh, targetDirection, theMapping);
	}

	virtual bool	createVoronoiSplitMesh
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		nvidia::ExplicitHierarchicalMesh& iHMeshCore,
		bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
		uint32_t randomSeed,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createVoronoiSplitMesh(hMesh, iHMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual uint32_t	createVoronoiSitesInsideMesh
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		PxVec3* siteBuffer,
		uint32_t* siteChunkIndices,
		uint32_t siteCount,
		uint32_t* randomSeed,
		uint32_t* microgridSize,
		BSPOpenMode::Enum meshMode,
		nvidia::IProgressListener& progressListener,
		uint32_t chunkIndex = 0xFFFFFFFF
	)
	{
		return ::FractureTools::createVoronoiSitesInsideMesh(hMesh, siteBuffer, siteChunkIndices, siteCount, randomSeed, microgridSize, meshMode, progressListener, chunkIndex);
	}

	virtual uint32_t	createScatterMeshSites
	(
		uint8_t*						meshIndices,
		PxMat44*						relativeTransforms,
		uint32_t*						chunkMeshStarts,
		uint32_t						scatterMeshInstancesBufferSize,
		nvidia::ExplicitHierarchicalMesh&	hMesh,
		uint32_t						targetChunkCount,
		const uint16_t*					targetChunkIndices,
		uint32_t*						randomSeed,
		uint32_t						scatterMeshAssetCount,
		nvidia::RenderMeshAsset**			scatterMeshAssets,
		const uint32_t*					minCount,
		const uint32_t*					maxCount,
		const float*					minScales,
		const float*					maxScales,
		const float*					maxAngles
	)
	{
		return ::FractureTools::createScatterMeshSites(meshIndices, relativeTransforms, chunkMeshStarts, scatterMeshInstancesBufferSize, hMesh, targetChunkCount,
								targetChunkIndices, randomSeed, scatterMeshAssetCount, scatterMeshAssets, minCount, maxCount, minScales, maxScales, maxAngles);
	}

	virtual void	visualizeVoronoiCells
	(
		nvidia::RenderDebugInterface& debugRender,
		const PxVec3* sites,
		uint32_t siteCount,
		const uint32_t* cellColors,
		uint32_t cellColorCount,
		const PxBounds3& bounds,
		uint32_t cellIndex = 0xFFFFFFFF
	)
	{
		::FractureTools::visualizeVoronoiCells(debugRender, sites, siteCount, cellColors, cellColorCount, bounds, cellIndex);
	}

	virtual bool buildExplicitHierarchicalMesh
	(
		nvidia::ExplicitHierarchicalMesh& iHMesh,
		const nvidia::ExplicitRenderTriangle* meshTriangles,
		uint32_t meshTriangleCount,
		const nvidia::ExplicitSubmeshData* submeshData,
		uint32_t submeshCount,
		uint32_t* meshPartition = NULL,
		uint32_t meshPartitionCount = 0,
		int32_t* parentIndices = NULL,
		uint32_t parentIndexCount = 0

	)
	{
		return ::FractureTools::buildExplicitHierarchicalMesh(iHMesh, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount, parentIndices, parentIndexCount);
	}

	virtual void setBSPTolerances
	(
		float linearTolerance,
		float angularTolerance,
		float baseTolerance,
		float clipTolerance,
		float cleaningTolerance
	)
	{
		::FractureTools::setBSPTolerances(linearTolerance, angularTolerance, baseTolerance, clipTolerance, cleaningTolerance);
	}

	virtual void setBSPBuildParameters
	(
		float logAreaSigmaThreshold,
		uint32_t testSetSize,
		float splitWeight,
		float imbalanceWeight
	)
	{
		::FractureTools::setBSPBuildParameters(logAreaSigmaThreshold, testSetSize, splitWeight, imbalanceWeight);
	}

	virtual bool buildExplicitHierarchicalMeshFromRenderMeshAsset(nvidia::ExplicitHierarchicalMesh& iHMesh, const nvidia::RenderMeshAsset& renderMeshAsset, uint32_t maxRootDepth = UINT32_MAX)
	{
		return ::FractureTools::buildExplicitHierarchicalMeshFromRenderMeshAsset(iHMesh, renderMeshAsset, maxRootDepth);
	}

	virtual bool buildExplicitHierarchicalMeshFromDestructibleAsset(nvidia::ExplicitHierarchicalMesh& iHMesh, const nvidia::DestructibleAsset& destructibleAsset, uint32_t maxRootDepth = UINT32_MAX)
	{
		return ::FractureTools::buildExplicitHierarchicalMeshFromDestructibleAsset(iHMesh, destructibleAsset, maxRootDepth);
	}

	virtual bool createHierarchicallySplitMesh
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		nvidia::ExplicitHierarchicalMesh& iHMeshCore,
		bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureSliceDesc& desc,
		const CollisionDesc& collisionDesc,
		uint32_t randomSeed,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createHierarchicallySplitMesh(hMesh, iHMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool createChippedMesh
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureCutoutDesc& desc,
		const ::FractureTools::CutoutSet& iCutoutSet,
		const ::FractureTools::FractureSliceDesc& sliceDesc,
		const ::FractureTools::FractureVoronoiDesc& voronoiDesc,
		const CollisionDesc& collisionDesc,
		uint32_t randomSeed,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createChippedMesh(hMesh, meshProcessingParams, desc, iCutoutSet, sliceDesc, voronoiDesc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool hierarchicallySplitChunk
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		uint32_t chunkIndex,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureSliceDesc& desc,
		const CollisionDesc& collisionDesc,
		uint32_t* randomSeed,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::hierarchicallySplitChunk(hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool voronoiSplitChunk
	(
		nvidia::ExplicitHierarchicalMesh& hMesh,
		uint32_t chunkIndex,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
		uint32_t* randomSeed,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::voronoiSplitChunk(hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool buildSliceMesh
	(
		IntersectMesh& intersectMesh,
		nvidia::ExplicitHierarchicalMesh& referenceMesh,
		const PxPlane& slicePlane,
		const ::FractureTools::NoiseParameters& noiseParameters,
		uint32_t randomSeed
	)
	{
		return ::FractureTools::buildSliceMesh(intersectMesh, referenceMesh, slicePlane, noiseParameters, randomSeed);
	}

	virtual nvidia::ExplicitHierarchicalMesh*	createExplicitHierarchicalMesh()
	{
		return ::FractureTools::createExplicitHierarchicalMesh();
	}

	virtual nvidia::ExplicitHierarchicalMesh::ConvexHull*	createExplicitHierarchicalMeshConvexHull()
	{
		return ::FractureTools::createExplicitHierarchicalMeshConvexHull();
	}
};
#endif



}
} // end namespace nvidia


#endif // __MODULE_DESTRUCTIBLE_IMPL_H__
