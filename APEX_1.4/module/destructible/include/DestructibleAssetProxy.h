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



#ifndef DESTRUCTIBLE_ASSET_PROXY_H
#define DESTRUCTIBLE_ASSET_PROXY_H

#include "ModuleDestructibleImpl.h"
#include "DestructibleAsset.h"
#include "DestructibleAssetImpl.h"
#include "PsUserAllocated.h"
#include "authoring/Fracturing.h"
#include "ApexUsingNamespace.h"
#include "ApexAuthorableObject.h"
#include "ApexAssetAuthoring.h"

#include "PsArray.h"
#include "nvparameterized/NvParameterized.h"

#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

#pragma warning(disable: 4355) // 'this' : used in base member initialization list

namespace nvidia
{
namespace destructible
{

class DestructibleAssetProxy : public DestructibleAsset, public ApexResourceInterface, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructibleAssetImpl impl;

	DestructibleAssetProxy(ModuleDestructibleImpl* module, ResourceList& list, const char* name) :
		impl(module, this, name)
	{
		list.add(*this);
	}

	DestructibleAssetProxy(ModuleDestructibleImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name) :
		impl(module, this, params, name)
	{
		list.add(*this);
	}

	~DestructibleAssetProxy()
	{
	}

	PxBounds3 getChunkActorLocalBounds(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkActorLocalBounds(chunkIndex);
	}

	const NvParameterized::Interface* getAssetNvParameterized() const
	{
		READ_ZONE();
		return impl.getAssetNvParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	*/
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = impl.acquireNvParameterizedInterface();
		release();
		return ret;
	}

	virtual void releaseDestructibleActor(DestructibleActor& actor)
	{
		WRITE_ZONE();
		impl.releaseDestructibleActor(actor);
	}
	virtual DestructibleParameters getDestructibleParameters() const
	{
		return impl.getParameters();
	}
	virtual DestructibleInitParameters getDestructibleInitParameters() const
	{
		return impl.getInitParameters();
	}
	virtual uint32_t getChunkCount() const
	{
		return impl.getChunkCount();
	}
	virtual uint32_t getDepthCount() const
	{
		READ_ZONE();
		return impl.getDepthCount();
	}
	virtual RenderMeshAsset* getRenderMeshAsset() const
	{
		READ_ZONE();
		return impl.getRenderMeshAsset();
	}
	virtual bool setRenderMeshAsset(RenderMeshAsset* renderMeshAsset)
	{
		WRITE_ZONE();
		return impl.setRenderMeshAsset(renderMeshAsset);
	}
	virtual uint32_t getScatterMeshAssetCount() const
	{
		READ_ZONE();
		return impl.getScatterMeshAssetCount();
	}
	virtual RenderMeshAsset* const * getScatterMeshAssets() const
	{
		READ_ZONE();
		return impl.getScatterMeshAssets();
	}
	virtual uint32_t getInstancedChunkMeshCount() const
	{
		READ_ZONE();
		return impl.getInstancedChunkMeshCount();
	}
	virtual const char* getCrumbleEmitterName() const
	{
		READ_ZONE();
		return impl.getCrumbleEmitterName();
	}
	virtual const char* getDustEmitterName() const
	{
		READ_ZONE();
		return impl.getDustEmitterName();
	}
	virtual const char* getFracturePatternName() const
	{
		return impl.getFracturePatternName();
	}
	virtual void getStats(DestructibleAssetStats& stats) const
	{
		READ_ZONE();
		impl.getStats(stats);
	}

	virtual void cacheChunkOverlapsUpToDepth(int32_t depth = -1)
	{
		WRITE_ZONE();
		impl.cacheChunkOverlapsUpToDepth(depth);
	}

	virtual void clearChunkOverlaps(int32_t depth = -1, bool keepCachedFlag = false)
	{
		impl.clearChunkOverlaps(depth, keepCachedFlag);
	}

	virtual void addChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges)
	{
		impl.addChunkOverlaps(supportGraphEdges, numSupportGraphEdges);
	}

	virtual void removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty)
	{
		impl.removeChunkOverlaps(supportGraphEdges, numSupportGraphEdges, keepCachedFlagIfEmpty);
	}

	virtual uint32_t getCachedOverlapCountAtDepth(uint32_t depth) const
	{
		READ_ZONE();
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		return uint32_t(pairArray != NULL ? pairArray->arraySizes[0] : 0);
	}

	virtual const IntPair* getCachedOverlapsAtDepth(uint32_t depth) const
	{
		READ_ZONE();
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		PX_ASSERT(sizeof(IntPair) == pairArray->elementSize);
		return pairArray != NULL ? (const IntPair*)pairArray->buf : NULL;
	}

	NvParameterized::Interface* getDefaultActorDesc()
	{
		return impl.getDefaultActorDesc();
	}

	NvParameterized::Interface* getDefaultAssetPreviewDesc()
	{
		return impl.getDefaultAssetPreviewDesc();
	}

	virtual Actor* createApexActor(const NvParameterized::Interface& parms, Scene& apexScene)
	{
		return impl.createApexActor(parms, apexScene);
	}

	virtual DestructibleActor* createDestructibleActorFromDeserializedState(NvParameterized::Interface* parms, Scene& apexScene)
	{
		WRITE_ZONE();
		return impl.createDestructibleActorFromDeserializedState(parms, apexScene);
	}

	virtual AssetPreview* createApexAssetPreview(const NvParameterized::Interface& params, AssetPreviewScene* /*previewScene*/)
	{
		return impl.createApexAssetPreview(params, NULL);
	}

	virtual bool isValidForActorCreation(const ::NvParameterized::Interface& parms, Scene& apexScene) const
	{
		return impl.isValidForActorCreation(parms, apexScene);
	}

	virtual bool isDirty() const
	{
		return false;
	}

	virtual PxVec3 getChunkPositionOffset(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkPositionOffset(chunkIndex);
	}

	virtual PxVec2 getChunkUVOffset(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkUVOffset(chunkIndex);
	}

	virtual uint32_t getChunkFlags(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkFlags(chunkIndex);
	}

	virtual uint16_t getChunkDepth(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkDepth(chunkIndex);
	}

	virtual int32_t getChunkParentIndex(uint32_t chunkIndex) const
	{
		READ_ZONE();
		const uint16_t chunkParentIndex = impl.getChunkParentIndex(chunkIndex);
		return chunkParentIndex != DestructibleAssetImpl::InvalidChunkIndex ? (int32_t)chunkParentIndex : -1;
	}

	virtual uint32_t getPartIndex(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getPartIndex(chunkIndex);
	}

	virtual uint32_t getPartConvexHullCount(const uint32_t partIndex) const
	{
		READ_ZONE();
		return impl.getPartHullIndexStop(partIndex) - impl.getPartHullIndexStart(partIndex);
	}

	virtual NvParameterized::Interface** getPartConvexHullArray(const uint32_t partIndex) const
	{
		READ_ZONE();
		return impl.getConvexHullParameters(impl.getPartHullIndexStart(partIndex));
	}

	uint32_t getActorTransformCount() const
	{
		READ_ZONE();
		return impl.getActorTransformCount();
	}
	const PxMat44* getActorTransforms() const
	{
		READ_ZONE();
		return impl.getActorTransforms();
	}

	void applyTransformation(const PxMat44& transformation, float scale)
	{
		WRITE_ZONE();
		impl.applyTransformation(transformation, scale);
	}
	void applyTransformation(const PxMat44& transformation)
	{
		WRITE_ZONE();
		impl.applyTransformation(transformation);
	}

	virtual bool rebuildCollisionGeometry(uint32_t partIndex, const DestructibleGeometryDesc& geometryDesc)
	{
		WRITE_ZONE();
		return impl.rebuildCollisionGeometry(partIndex, geometryDesc);
	}

	// ApexInterface methods
	virtual void release()
	{
		WRITE_ZONE();
		impl.getOwner()->mSdk->releaseAsset(*this);
	}
	virtual void destroy()
	{
		impl.cleanup();
		delete this;
	}

	// Asset methods
	virtual const char* getName() const
	{
		READ_ZONE();
		return impl.getName();
	}
	static const char* 		getClassName()
	{
		return DESTRUCTIBLE_AUTHORING_TYPE_NAME;
	}
	virtual AuthObjTypeID	getObjTypeID() const
	{
		return impl.getObjTypeID();
	}
	virtual const char* getObjTypeName() const
	{
		return impl.getObjTypeName();
	}
	virtual uint32_t forceLoadAssets()
	{
		return impl.forceLoadAssets();
	}

	// ApexResourceInterface methods
	virtual void	setListIndex(ResourceList& list, uint32_t index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual uint32_t	getListIndex() const
	{
		return impl.m_listIndex;
	}

	friend class DestructibleActorImpl;
};

#ifndef WITHOUT_APEX_AUTHORING
class DestructibleAssetAuthoringProxy : public DestructibleAssetAuthoring, public ApexResourceInterface, public ApexAssetAuthoring, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructibleAssetAuthoringImpl impl;

	DestructibleAssetAuthoringProxy(ModuleDestructibleImpl* module, ResourceList& list) :
		impl(module, NULL, "DestructibleAuthoring")
	{
		list.add(*this);
	}

	DestructibleAssetAuthoringProxy(ModuleDestructibleImpl* module, ResourceList& list, const char* name) :
		impl(module, NULL, name)
	{
		list.add(*this);
	}

	DestructibleAssetAuthoringProxy(ModuleDestructibleImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name) :
		impl(module, NULL, params, name)
	{
		list.add(*this);
	}

	virtual ~DestructibleAssetAuthoringProxy()
	{
	}

	// DestructibleAssetAuthoring methods

	virtual ExplicitHierarchicalMesh& getExplicitHierarchicalMesh()
	{
		return impl.hMesh;
	}

	virtual ExplicitHierarchicalMesh& getCoreExplicitHierarchicalMesh()
	{
		return impl.hMeshCore;
	}

	virtual CutoutSet&	getCutoutSet()
	{
		return impl.cutoutSet;
	}

	virtual uint32_t					partitionMeshByIslands
	(
		nvidia::ExplicitRenderTriangle* mesh,
		uint32_t meshTriangleCount,
	    uint32_t* meshPartition,
	    uint32_t meshPartitionMaxCount,
		float padding
	)
	{
		return ::FractureTools::partitionMeshByIslands(mesh, meshTriangleCount, meshPartition, meshPartitionMaxCount, padding);
	}

	virtual bool	setRootMesh
	(
	    const ExplicitRenderTriangle* meshTriangles,
	    uint32_t meshTriangleCount,
	    const ExplicitSubmeshData* submeshData,
	    uint32_t submeshCount,
	    uint32_t* meshPartition = NULL,
	    uint32_t meshPartitionCount = 0,
		int32_t* parentIndices = NULL,
		uint32_t parentIndexCount = 0
	)
	{
		return ::FractureTools::buildExplicitHierarchicalMesh(impl.hMesh, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount, parentIndices, parentIndexCount);
	}

	virtual bool	importRenderMeshAssetToRootMesh(const nvidia::RenderMeshAsset& renderMeshAsset, uint32_t maxRootDepth = UINT32_MAX)
	{
		return ::FractureTools::buildExplicitHierarchicalMeshFromRenderMeshAsset(impl.hMesh, renderMeshAsset, maxRootDepth);
	}

	virtual bool	importDestructibleAssetToRootMesh(const DestructibleAsset& destructibleAsset, uint32_t maxRootDepth = UINT32_MAX)
	{
		return ::FractureTools::buildExplicitHierarchicalMeshFromDestructibleAsset(impl.hMesh, destructibleAsset, maxRootDepth);
	}

	virtual bool	setCoreMesh
	(
	    const ExplicitRenderTriangle* meshTriangles,
	    uint32_t meshTriangleCount,
	    const ExplicitSubmeshData* submeshData,
	    uint32_t submeshCount,
	    uint32_t* meshPartition = NULL,
	    uint32_t meshPartitionCount = 0
	)
	{
		return ::FractureTools::buildExplicitHierarchicalMesh(impl.hMeshCore, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount);
	}

	virtual bool buildExplicitHierarchicalMesh
	(
		ExplicitHierarchicalMesh& hMesh,
		const ExplicitRenderTriangle* meshTriangles,
		uint32_t meshTriangleCount,
		const ExplicitSubmeshData* submeshData,
		uint32_t submeshCount,
		uint32_t* meshPartition = NULL,
		uint32_t meshPartitionCount = 0,
		int32_t* parentIndices = NULL,
		uint32_t parentIndexCount = 0
	)
	{
		return ::FractureTools::buildExplicitHierarchicalMesh(hMesh, meshTriangles, meshTriangleCount, submeshData, submeshCount, meshPartition, meshPartitionCount, parentIndices, parentIndexCount);
	}

	virtual bool	createHierarchicallySplitMesh
	(
	    const MeshProcessingParameters& meshProcessingParams,
	    const FractureSliceDesc& desc,
	    const CollisionDesc& collisionDesc,
	    bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
	    uint32_t randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createHierarchicallySplitMesh(impl.hMesh, impl.hMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex,
		        meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	createVoronoiSplitMesh
	(
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
		bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
		uint32_t randomSeed,
		IProgressListener& progressListener,
		volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createVoronoiSplitMesh(impl.hMesh, impl.hMeshCore, exportCoreMesh, coreMeshImprintSubmeshIndex,
				meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	hierarchicallySplitChunk
	(
		uint32_t chunkIndex,
	    const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const ::FractureTools::FractureSliceDesc& desc,
	    const CollisionDesc& collisionDesc,
	    uint32_t* randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::hierarchicallySplitChunk(impl.hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	voronoiSplitChunk
		(
		uint32_t chunkIndex,
		const ::FractureTools::MeshProcessingParameters& meshProcessingParams,
		const ::FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
		uint32_t* randomSeed,
		IProgressListener& progressListener,
		volatile bool* cancel = NULL
		)
	{
		return ::FractureTools::voronoiSplitChunk(impl.hMesh, chunkIndex, meshProcessingParams, desc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual bool	createChippedMesh
	(
	    const MeshProcessingParameters& meshProcessingParams,
	    const FractureCutoutDesc& desc,
	    const CutoutSet& iCutoutSet,
	    const FractureSliceDesc& sliceDesc,
		const FractureVoronoiDesc& voronoiDesc,
	    const CollisionDesc& collisionDesc,
	    uint32_t randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	)
	{
		return ::FractureTools::createChippedMesh(impl.hMesh, meshProcessingParams, desc, iCutoutSet, sliceDesc, voronoiDesc, collisionDesc, randomSeed, progressListener, cancel);
	}

	virtual void	buildCutoutSet
	(
	    const uint8_t* pixelBuffer,
	    uint32_t bufferWidth,
	    uint32_t bufferHeight,
	    float snapThreshold,
		bool periodic
	)
	{
		::FractureTools::buildCutoutSet(impl.cutoutSet, pixelBuffer, bufferWidth, bufferHeight, snapThreshold, periodic);
	}

	virtual bool	calculateCutoutUVMapping(PxMat33& mapping, const nvidia::ExplicitRenderTriangle& triangle)
	{
		return ::FractureTools::calculateCutoutUVMapping(triangle, mapping);
	}

	virtual bool	calculateCutoutUVMapping(PxMat33& mapping, const PxVec3& targetDirection)
	{
		return ::FractureTools::calculateCutoutUVMapping(impl.hMesh, targetDirection, mapping);
	}

	virtual uint32_t	createVoronoiSitesInsideMesh
	(
		PxVec3* siteBuffer,
		uint32_t* siteChunkIndices,
		uint32_t siteCount,
		uint32_t* randomSeed,
		uint32_t* microgridSize,
		BSPOpenMode::Enum meshMode,
		IProgressListener& progressListener,
		uint32_t chunkIndex = 0xFFFFFFFF
	)
	{
		return ::FractureTools::createVoronoiSitesInsideMesh(impl.hMesh, siteBuffer, siteChunkIndices, siteCount, randomSeed, microgridSize, meshMode, progressListener, chunkIndex);
	}

	uint32_t	createScatterMeshSites
	(
		uint8_t*						meshIndices,
		PxMat44*						relativeTransforms,
		uint32_t*						chunkMeshStarts,
		uint32_t						scatterMeshInstancesBufferSize,
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
		return ::FractureTools::createScatterMeshSites(meshIndices, relativeTransforms, chunkMeshStarts, scatterMeshInstancesBufferSize, impl.hMesh, targetChunkCount, targetChunkIndices,
													 randomSeed, scatterMeshAssetCount, scatterMeshAssets, minCount, maxCount, minScales, maxScales, maxAngles);
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

	virtual void	setBSPTolerances
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

	virtual void	setBSPBuildParameters
	(
		float logAreaSigmaThreshold,
		uint32_t testSetSize,
		float splitWeight,
		float imbalanceWeight
	)
	{
		::FractureTools::setBSPBuildParameters(logAreaSigmaThreshold, testSetSize, splitWeight, imbalanceWeight);
	}

	virtual ExplicitHierarchicalMesh::ConvexHull*	createExplicitHierarchicalMeshConvexHull()
	{
		return ::FractureTools::createExplicitHierarchicalMeshConvexHull();
	}

	virtual uint32_t buildSliceMesh(const ExplicitRenderTriangle*& mesh, const ::FractureTools::NoiseParameters& noiseParameters, const PxPlane& slicePlane, uint32_t randomSeed)
	{
		if( ::FractureTools::buildSliceMesh(impl.intersectMesh, impl.hMesh, slicePlane, noiseParameters, randomSeed) )
		{
			mesh = impl.intersectMesh.m_triangles.begin();
			return impl.intersectMesh.m_triangles.size();
		}

		mesh = NULL;
		return 0;
	}

	virtual void setChunkOverlapsCacheDepth(int32_t depth = -1)
	{
		impl.setChunkOverlapsCacheDepth(depth);
	}
	virtual RenderMeshAsset* getRenderMeshAsset() const
	{
		return impl.getRenderMeshAsset();
	}
	virtual bool setRenderMeshAsset(RenderMeshAsset* renderMeshAsset)
	{
		return impl.setRenderMeshAsset(renderMeshAsset);
	}
	virtual bool setScatterMeshAssets(RenderMeshAsset** scatterMeshAssetArray, uint32_t scatterMeshAssetArraySize)
	{
		return impl.setScatterMeshAssets(scatterMeshAssetArray, scatterMeshAssetArraySize);
	}
	virtual uint32_t getScatterMeshAssetCount() const
	{
		return impl.getScatterMeshAssetCount();
	}
	virtual RenderMeshAsset* const * getScatterMeshAssets() const
	{
		return impl.getScatterMeshAssets();
	}
	virtual uint32_t getInstancedChunkMeshCount() const
	{
		return impl.getInstancedChunkMeshCount();
	}
	virtual void setDestructibleParameters(const DestructibleParameters& parameters)
	{
		impl.setParameters(parameters);
	}
	virtual DestructibleParameters getDestructibleParameters() const
	{
		READ_ZONE();
		return impl.getParameters();
	}
	virtual void setDestructibleInitParameters(const DestructibleInitParameters& parameters)
	{
		impl.setInitParameters(parameters);
	}
	virtual DestructibleInitParameters	getDestructibleInitParameters() const
	{
		READ_ZONE();
		return impl.getInitParameters();
	}
	virtual void setCrumbleEmitterName(const char* name)
	{
		impl.setCrumbleEmitterName(name);
	}
	virtual void setDustEmitterName(const char* name)
	{
		impl.setDustEmitterName(name);
	}
	virtual void setFracturePatternName(const char* name)
	{
		impl.setFracturePatternName(name);
	}
	virtual void cookChunks(const DestructibleAssetCookingDesc& cookingDesc, bool cacheOverlaps, uint32_t* chunkIndexMapUser2Apex, uint32_t* chunkIndexMapApex2User, uint32_t chunkIndexMapCount)
	{
		impl.cookChunks(cookingDesc, cacheOverlaps, chunkIndexMapApex2User, chunkIndexMapUser2Apex, chunkIndexMapCount);
	}
	virtual void serializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding) const
	{
		impl.serializeFractureToolState(stream, embedding);
	}
	virtual void deserializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding)
	{
		impl.deserializeFractureToolState(stream, embedding);
	}
	virtual uint32_t getChunkCount() const
	{
		READ_ZONE();
		return impl.getChunkCount();
	}
	virtual uint32_t getDepthCount() const
	{
		return impl.getDepthCount();
	}
	virtual uint32_t getChunkChildCount(uint32_t chunkIndex) const
	{
		return impl.getChunkChildCount(chunkIndex);
	}
	virtual int32_t getChunkChild(uint32_t chunkIndex, uint32_t childIndex) const
	{
		return impl.getChunkChild(chunkIndex, childIndex);
	}
	virtual PxVec3 getChunkPositionOffset(uint32_t chunkIndex) const
	{
		return impl.getChunkPositionOffset(chunkIndex);
	}
	virtual PxVec2 getChunkUVOffset(uint32_t chunkIndex) const
	{
		return impl.getChunkUVOffset(chunkIndex);
	}
	virtual uint32_t getPartIndex(uint32_t chunkIndex) const
	{
		return impl.getPartIndex(chunkIndex);
	}
	virtual void trimCollisionGeometry(const uint32_t* partIndices, uint32_t partIndexCount, float maxTrimFraction = 0.2f)
	{
		impl.trimCollisionGeometry(partIndices, partIndexCount, maxTrimFraction);
	}
	virtual float getImpactVelocityThreshold() const
	{
		return impl.getImpactVelocityThreshold();
	}
	void setImpactVelocityThreshold(float threshold)
	{
		impl.setImpactVelocityThreshold(threshold);
	}
	virtual float getFractureImpulseScale() const
	{
		return impl.getFractureImpulseScale();
	}
	void setFractureImpulseScale(float scale)
	{
		impl.setFractureImpulseScale(scale);
	}
	virtual void getStats(DestructibleAssetStats& stats) const
	{
		impl.getStats(stats);
	}

	virtual void cacheChunkOverlapsUpToDepth(int32_t depth = -1)
	{
		impl.cacheChunkOverlapsUpToDepth(depth);
	}

	virtual void clearChunkOverlaps(int32_t depth = -1, bool keepCachedFlag = false)
	{
		impl.clearChunkOverlaps(depth, keepCachedFlag);
	}

	virtual void addChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges)
	{
		impl.addChunkOverlaps(supportGraphEdges, numSupportGraphEdges);
	}

	virtual void removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty)
	{
		impl.removeChunkOverlaps(supportGraphEdges, numSupportGraphEdges, keepCachedFlagIfEmpty);
	}

	virtual uint32_t getCachedOverlapCountAtDepth(uint32_t depth)
	{
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		return uint32_t(pairArray != NULL ? pairArray->arraySizes[0] : 0);
	}

	virtual const IntPair* getCachedOverlapsAtDepth(uint32_t depth)
	{
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* pairArray = impl.getOverlapsAtDepth(depth, false);
		PX_ASSERT(sizeof(IntPair) == pairArray->elementSize);
		return pairArray != NULL ? (const IntPair*)pairArray->buf : NULL;
	}
	virtual void setNeighborPadding(float neighborPadding)
	{
		impl.setNeighborPadding(neighborPadding);
	}
	virtual float getNeighborPadding() const
	{
		return impl.getNeighborPadding();
	}
	void applyTransformation(const PxMat44& transformation, float scale)
	{
		impl.applyTransformation(transformation, scale);
	}
	void applyTransformation(const PxMat44& transformation)
	{
		impl.applyTransformation(transformation);
	}

	bool setPlatformMaxDepth(PlatformTag platform, uint32_t maxDepth)
	{
		return impl.setPlatformMaxDepth(platform, maxDepth);
	}

	bool removePlatformMaxDepth(PlatformTag platform)
	{
		return impl.removePlatformMaxDepth(platform);
	}

	// AssetAuthoring methods

	const char* getName(void) const
	{
		return impl.getName();
	}

	const char* getObjTypeName() const
	{
		return impl.getObjTypeName();
	}

	virtual bool prepareForPlatform(nvidia::apex::PlatformTag platformTag)
	{
		return impl.prepareForPlatform(platformTag);
	}

	void setToolString(const char* toolName, const char* toolVersion, uint32_t toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	void setToolString(const char* toolString)
	{
		impl.setToolString(toolString);
	}

	uint32_t getActorTransformCount() const
	{
		return impl.getActorTransformCount();
	}

	const PxMat44* getActorTransforms() const
	{
		return impl.getActorTransforms();
	}

	void appendActorTransforms(const PxMat44* transforms, uint32_t transformCount)
	{
		impl.appendActorTransforms(transforms, transformCount);
	}

	void clearActorTransforms()
	{
		impl.clearActorTransforms();
	}

	NvParameterized::Interface* getNvParameterized() const
	{
		return (NvParameterized::Interface*)impl.getAssetNvParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	*/
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = impl.acquireNvParameterizedInterface();
		release();
		return ret;
	}


	// ApexResourceInterface methods
	virtual void	setListIndex(ResourceList& list, uint32_t index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual uint32_t	getListIndex() const
	{
		return impl.m_listIndex;
	}

	// ApexInterface methods
	virtual void release()
	{
		impl.getOwner()->mSdk->releaseAssetAuthoring(*this);
	}
	virtual void destroy()
	{
		impl.cleanup();
		delete this;
	}
};
#endif

}
} // end namespace nvidia

#endif // DESTRUCTIBLE_ASSET_PROXY_H
