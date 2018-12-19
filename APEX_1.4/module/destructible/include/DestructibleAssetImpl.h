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



#ifndef DESTRUCTIBLE_ASSET_IMPL_H
#define DESTRUCTIBLE_ASSET_IMPL_H

#include "Apex.h"
#include "PsArray.h"
#include "ApexSDKHelpers.h"
#include "DestructibleAsset.h"
#include "ApexSharedUtils.h"
#include "ApexAssetTracker.h"
#include "DestructibleActorParam.h"
#include "ApexUsingNamespace.h"
#include "DestructibleAssetParameters.h"
#include "MeshCookedCollisionStreamsAtScale.h"
#include "MeshCookedCollisionStream.h"
#include "DestructibleAssetCollisionDataSet.h"
#include "CachedOverlaps.h"

#include "PxConvexMesh.h"

#include "authoring/Fracturing.h"

#include "ApexActor.h"

namespace nvidia
{
namespace apex
{
class DestructiblePreview;
}
namespace destructible
{
class ModuleDestructibleImpl;


/**
	Descriptor used to create a Destructible actor.
*/
class DestructibleActorDesc : public ApexDesc
{
public:

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE DestructibleActorDesc() : ApexDesc()
	{
		setToDefault();
	}

	/**
	\brief Resets descriptor to default settings.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();

		physX3Template	= NULL;
		crumbleEmitterName = NULL;
		dustEmitterName = NULL;
		globalPose = PxMat44(PxIdentity);
		scale = PxVec3(1.0f);
		dynamic = false;
		supportDepth = 0;
		formExtendedStructures = false;
		useAssetDefinedSupport = false;
		useWorldSupport = false;
		overrideSkinnedMaterials = NULL;
		overrideSkinnedMaterialCount = 0;
		overrideStaticMaterials = NULL;
		overrideStaticMaterialCount = 0;
		renderStaticChunksSeparately = true;
		destructibleParameters.setToDefault();
	}

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		const float det = (globalPose.column0.getXYZ().cross(globalPose.column1.getXYZ())).dot(globalPose.column2.getXYZ());
		if (!PxEquals(det, 1.0f, 0.001f))
		{
			APEX_DEBUG_WARNING("Mirror transformations are not allowed in the DestructibleActor descriptor.");
			return false;
		}

		if (scale.x <= 0 || scale.y <= 0 || scale.z <= 0)
		{
			APEX_DEBUG_WARNING("Negative scales are not allowed in the DestructibleActor descriptor.");
			return false;
		}

		if (physX3Template && !physX3Template->isValid() )
		{
			APEX_DEBUG_WARNING("Invalid physx3 descriptor template in the DestructibleActor descriptor.");
			return false;
		}

		if (overrideSkinnedMaterialCount > 0 && overrideSkinnedMaterials == NULL)
		{
			APEX_DEBUG_WARNING("overrideSkinnedMaterials is NULL, but overrideSkinnedMaterialCount > 0.");
			return false;
		}

		if (overrideStaticMaterialCount > 0 && overrideStaticMaterials == NULL)
		{
			APEX_DEBUG_WARNING("overrideStaticMaterials is NULL, but overrideStaticMaterialCount > 0.");
			return false;
		}

		// Note - Not checking shapeDescTemplate, since meshData is not supplied

		return ApexDesc::isValid();
	}

	PhysX3DescTemplateImpl*			physX3Template;

	/**
		The name of the MeshParticleSystem to use for crumbling.  This overrides the crumble system defined
		in the DestructibleAsset if specified.
	*/
	const char*					crumbleEmitterName;

	/**
		The name of the MeshParticleSystem to use for fracture-line dust.  This overrides the dust system defined
		in the DestructibleAsset if specified.
	*/
	const char*					dustEmitterName;

	/**
		Initial global pose of undamaged destructible
	*/
	PxMat44				globalPose;

	/**
		3D scale
	*/
	PxVec3				scale;

	/**
		Whether or not the destructible starts life as a dynamic actor
	*/
	bool						dynamic;

	/**
		The chunk hierarchy depth at which to create a support graph.  Higher depth levels give more detailed support,
		but will give a higher computational load.  Chunks below the support depth will never be supported.
	*/
	uint32_t				supportDepth;

	/**
		If set, and the destructible is initially static, it will become part of an extended support
		structure if it is in contact with another static destructible that also has this flag set.
	*/
	bool						formExtendedStructures;

	/**
		If set, then chunks which are tagged as "support" chunks (via DestructibleChunkDesc::isSupportChunk)
		will have environmental support in static destructibles.

		Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as
		"support" chunks AND overlap the PxScene's static geometry in order to be environmentally supported.
	*/
	bool						useAssetDefinedSupport;

	/**
		If set, then chunks which overlap the PxScene's static geometry will have environmental support in
		static destructibles.

		Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as
		"support" chunks AND overlap the PxScene's static geometry in order to be environmentally supported.
	*/
	bool						useWorldSupport;

	/*
		If true, static chunks will be renderered separately from dynamic chunks, as a single mesh (not using skinning).
		This parameter is ignored if the 'dynamic' parameter is true.
		Default value = false.
	*/
	bool						renderStaticChunksSeparately;

	/**
		Per-actor material names for the skinned mesh
	*/
	const char**				overrideSkinnedMaterials;

	/**
		Size of overrideSkinnedMaterials array
	*/
	uint32_t				overrideSkinnedMaterialCount;

	/**
		Per-actor material names for the static mesh
	*/
	const char**				overrideStaticMaterials;

	/**
		Size of overrideStaticMaterials array
	*/
	uint32_t				overrideStaticMaterialCount;

	/**
		Initial destructible parameters.  These may be changed at runtime.
	*/
	DestructibleParameters	destructibleParameters;
};


/**
	Set of sets of convex meshes [scale#][part#]
*/
class DestructibleConvexMeshContainer
{
public:
	struct ConvexMeshSet : public UserAllocated
	{
		ConvexMeshSet() : mReferenceCount(0) {}

		physx::Array<PxConvexMesh*> mSet;
		uint32_t				mReferenceCount;
	};

					~DestructibleConvexMeshContainer() { reset(); }

	uint32_t					size() const
									{ 
										return mConvexMeshSets.size();
									}

	physx::Array<PxConvexMesh*>&	operator []	(uint32_t i) const
									{ 
										return mConvexMeshSets[i]->mSet;
									}

	uint32_t					getReferenceCount(uint32_t i) const 
									{ 
										return mConvexMeshSets[i]->mReferenceCount;
									}
	void							incReferenceCount(uint32_t i) 
									{
										++mConvexMeshSets[i]->mReferenceCount;
									}
	bool							decReferenceCount(uint32_t i) 
									{
										if (mConvexMeshSets[i]->mReferenceCount == 0) 
											return false; 
										--mConvexMeshSets[i]->mReferenceCount; 
										return true;
									}

	void							resize(uint32_t newSize)
									{
										const uint32_t oldSize = size();
										mConvexMeshSets.resize(newSize);
										for (uint32_t i = oldSize; i < newSize; ++i)
										{
											mConvexMeshSets[i] = PX_NEW(ConvexMeshSet);
										}
									}

	void							reset(bool force = true)
									{
										for (uint32_t i = mConvexMeshSets.size(); i--;)
										{
											ConvexMeshSet* meshSet = mConvexMeshSets[i];
											if (meshSet != NULL)
											{
												if (force)
												{
													PX_DELETE(meshSet);
													mConvexMeshSets[i] = NULL;
												}
												else
												{
													if (meshSet->mReferenceCount == 0)
													{
														// Release the PhysX convex mesh pointers
														for (uint32_t j=0; j<meshSet->mSet.size(); j++)
														{
															PxConvexMesh* convexMesh = meshSet->mSet[j];
															if (convexMesh == NULL)
															{
																continue;
															}
															convexMesh->release();
														}
														
														meshSet->mSet.resize(0);
														PX_DELETE(meshSet);
														mConvexMeshSets.replaceWithLast(i);
													}
												}
											}
										}
										if (force)
										{
											mConvexMeshSets.reset();
										}
									}

private:
	physx::Array<ConvexMeshSet*>	mConvexMeshSets;
};


/**
	Destructible asset collision data.  Caches collision data for a DestructibleAsset at
	various scales.
*/

#define kDefaultDestructibleAssetCollisionScaleTolerance	(0.0001f)

class DestructibleAssetCollision : public ApexResource
{
public:

	struct Version
	{
		enum Enum
		{
			First = 0,
			// New versions must be put here.  There is no need to explicitly number them.  The
			// numbers above were put there to conform to the old DestructionToolStreamVersion enum.

			Count,
			Current = Count - 1
		};
	};

	DestructibleAssetCollision();
	DestructibleAssetCollision(NvParameterized::Interface* params);
	~DestructibleAssetCollision();

	void								setDestructibleAssetToCook(class DestructibleAssetImpl* asset);

	bool								addScale(const PxVec3& scale);

	bool								cookAll();

	bool								cookScale(const PxVec3& scale);

	void								resize(uint32_t hullCount);

	const char*							getAssetName() const
	{
		return mParams->assetName;
	}
	DestructibleAssetImpl*					getAsset() { return mAsset; }
	PxConvexMesh*						getConvexMesh(uint32_t hullIndex, const PxVec3& scale);

	PxFileBuf&					deserialize(PxFileBuf& stream, const char* assetName);
	PxFileBuf&					serialize(PxFileBuf& stream) const;

	bool								platformAndVersionMatch() const;
	void								setPlatformAndVersion();

	uint32_t								memorySize() const;

	MeshCookedCollisionStreamsAtScale*	getCollisionAtScale(const PxVec3& scale);

	physx::Array<PxConvexMesh*>*			getConvexMeshesAtScale(const PxVec3& scale);

	void								clearUnreferencedSets();
	// Spit out warnings to the error stream for any referenced sets
	void								reportReferencedSets();
	bool								incReferenceCount(int scaleIndex);
	bool								decReferenceCount(int scaleIndex);

	void								merge(DestructibleAssetCollision& collisionSet);

	int32_t						getScaleIndex(const PxVec3& scale, float tolerance) const;

private:
	DestructibleAssetCollisionDataSet*				mParams;
	bool											mOwnsParams;

	class DestructibleAssetImpl*						mAsset;
	DestructibleConvexMeshContainer					mConvexMeshContainer;
};

#define OFFSET_FN(_classname, _membername) static uint32_t _membername##Offset() { return PX_OFFSET_OF(_classname, _membername); }

class DestructibleAssetImpl : public ApexResource
{
public:

	enum
	{
		InvalidChunkIndex =	0xFFFF
	};

	enum ChunkFlags
	{
		SupportChunk =	(1 << 0),
		UnfracturableChunk =	(1 << 1),
		DescendantUnfractureable =	(1 << 2),
		UndamageableChunk =	(1 << 3),
		UncrumbleableChunk =	(1 << 4),
#if APEX_RUNTIME_FRACTURE
		RuntimeFracturableChunk =	(1 << 5),
#endif

		Instanced = (1 << 8),
	};

	struct ChunkInstanceBufferDataElement
	{
		PxVec3	translation;
		PxMat33	scaledRotation;
		PxVec2	uvOffset;
		PxVec3	localOffset;

		OFFSET_FN(ChunkInstanceBufferDataElement, translation)
		OFFSET_FN(ChunkInstanceBufferDataElement, scaledRotation)
		OFFSET_FN(ChunkInstanceBufferDataElement, uvOffset)
		OFFSET_FN(ChunkInstanceBufferDataElement, localOffset)
	};

	struct ScatterInstanceBufferDataElement
	{
		PxVec3	translation;
		PxMat33	scaledRotation;
		float	alpha;

		OFFSET_FN(ScatterInstanceBufferDataElement, translation)
		OFFSET_FN(ScatterInstanceBufferDataElement, scaledRotation)
		OFFSET_FN(ScatterInstanceBufferDataElement, alpha)
	};

	struct ScatterMeshInstanceInfo
	{
		ScatterMeshInstanceInfo() 
		: m_actor(NULL)
		, m_instanceBuffer(NULL)
		, m_IBSize(0) 
		{}

		~ScatterMeshInstanceInfo();

		RenderMeshActor*								m_actor;
		UserRenderInstanceBuffer*						m_instanceBuffer;
		uint32_t									m_IBSize;
		physx::Array<ScatterInstanceBufferDataElement>	m_instanceBufferData;
	};

	DestructibleAssetImpl(ModuleDestructibleImpl* module, DestructibleAsset* api, const char* name);
	DestructibleAssetImpl(ModuleDestructibleImpl* module, DestructibleAsset* api, NvParameterized::Interface* params, const char* name);
	~DestructibleAssetImpl();

	const NvParameterized::Interface* 	getAssetNvParameterized() const
	{
		return mParams;
	}

	DestructibleActor*				createDestructibleActorFromDeserializedState(NvParameterized::Interface* params, Scene&);
	DestructibleActor*				createDestructibleActor(const NvParameterized::Interface& params, Scene&);
	void								releaseDestructibleActor(DestructibleActor& actor);

	RenderMeshAsset* 					getRenderMeshAsset() const
	{
		return renderMeshAsset;
	}
	bool								setRenderMeshAsset(RenderMeshAsset* newRenderMeshAsset);

	bool								setScatterMeshAssets(RenderMeshAsset** scatterMeshAssetArray, uint32_t scatterMeshAssetArraySize);

	void								createScatterMeshInstanceInfo();

	UserRenderInstanceBufferDesc		getScatterMeshInstanceBufferDesc();

	uint32_t						getScatterMeshAssetCount() const
	{
		return scatterMeshAssets.size();
	}

	virtual RenderMeshAsset* const *			getScatterMeshAssets() const
	{
		return scatterMeshAssets.size() > 0 ? &scatterMeshAssets[0] : NULL;
	}

	RenderMeshAsset*					getRuntimeRenderMeshAsset() const
	{
		return runtimeRenderMeshAsset;
	}

	uint32_t						getInstancedChunkMeshCount() const
	{
		return m_instancedChunkMeshCount;
	}

	uint32_t						getChunkCount() const
	{
		return (uint32_t)mParams->chunks.arraySizes[0];
	}
	uint32_t						getDepthCount() const
	{
		return mParams->depthCount;
	}
	bool getScatterMeshAlwaysDrawFlag() const
	{
		return mParams->destructibleParameters.alwaysDrawScatterMesh;
	}
	uint32_t getChunkChildCount(uint32_t chunkIndex) const
	{
		if (chunkIndex >= (uint32_t)mParams->chunks.arraySizes[0])
		{
			return 0;
		}
		return (uint32_t)mParams->chunks.buf[chunkIndex].numChildren;
	}
	uint16_t getChunkDepth(uint32_t chunkIndex) const
	{
		if (chunkIndex >= (uint32_t)mParams->chunks.arraySizes[0])
		{
			return 0;
		}
		return mParams->chunks.buf[chunkIndex].depth;
	}
	int32_t getChunkChild(uint32_t chunkIndex, uint32_t childIndex) const
	{
		if (childIndex >= getChunkChildCount(chunkIndex))
		{
			return -1;
		}
		return int32_t(mParams->chunks.buf[chunkIndex].firstChildIndex + childIndex);
	}
	PxBounds3					getBounds() const
	{
		return mParams->bounds;
	}
	DestructibleParameters			getParameters() const;
	DestructibleInitParameters		getInitParameters() const;
	const char*							getCrumbleEmitterName() const;
	const char*							getDustEmitterName() const;
	const char*							getFracturePatternName() const;
	float						getFractureImpulseScale() const
	{
		return mParams->destructibleParameters.fractureImpulseScale;
	}
	float						getImpactVelocityThreshold() const
	{
		return mParams->destructibleParameters.impactVelocityThreshold;
	}
	void								getStats(DestructibleAssetStats& stats) const;
	void								cacheChunkOverlapsUpToDepth(int32_t depth = -1);
	float						getNeighborPadding() const
	{
		return mParams->neighborPadding;
	}

	uint16_t						getChunkParentIndex(uint32_t chunkIndex) const 
	{
		PX_ASSERT(chunkIndex < (uint16_t)mParams->chunks.arraySizes[0]);
		return mParams->chunks.buf[chunkIndex].parentIndex;
	}

	PxVec3						getChunkPositionOffset(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mParams->chunks.arraySizes[0]);
		DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mParams->chunks.buf[chunkIndex];
		return (sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0 ? PxVec3(0.0f) : mParams->chunkInstanceInfo.buf[sourceChunk.meshPartIndex].chunkPositionOffset;
	}

	PxVec2						getChunkUVOffset(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mParams->chunks.arraySizes[0]);
		DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mParams->chunks.buf[chunkIndex];
		return (sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0 ? PxVec2(0.0f) : mParams->chunkInstanceInfo.buf[sourceChunk.meshPartIndex].chunkUVOffset;
	}

	uint32_t						getChunkFlags(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mParams->chunks.arraySizes[0]);
		DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mParams->chunks.buf[chunkIndex];
		uint32_t flags = 0;
		if (sourceChunk.flags & DestructibleAssetImpl::SupportChunk)
		{
			flags |= DestructibleAsset::ChunkEnvironmentallySupported;
		}
		if (sourceChunk.flags & DestructibleAssetImpl::UnfracturableChunk)
		{
			flags |= DestructibleAsset::ChunkAndDescendentsDoNotFracture;
		}
		if (sourceChunk.flags & DestructibleAssetImpl::UndamageableChunk)
		{
			flags |= DestructibleAsset::ChunkDoesNotFracture;
		}
		if (sourceChunk.flags & DestructibleAssetImpl::UncrumbleableChunk)
		{
			flags |= DestructibleAsset::ChunkDoesNotCrumble;
		}
		if (sourceChunk.flags & DestructibleAssetImpl::Instanced)
		{
			flags |= DestructibleAsset::ChunkIsInstanced;
		}
#if APEX_RUNTIME_FRACTURE
		if (sourceChunk.flags & DestructibleAssetImpl::RuntimeFracturableChunk)
		{
			flags |= DestructibleAsset::ChunkRuntimeFracture;
		}
#endif
		return flags;
	}

	uint32_t						getPartIndex(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mParams->chunks.arraySizes[0]);
		DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mParams->chunks.buf[chunkIndex];
		return (sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0 ? sourceChunk.meshPartIndex : mParams->chunkInstanceInfo.buf[sourceChunk.meshPartIndex].partIndex;
	}

	uint32_t						getChunkHullIndexStart(uint32_t chunkIndex) const
	{
		return mParams->chunkConvexHullStartIndices.buf[getPartIndex(chunkIndex)];
	}

	uint32_t						getChunkHullIndexStop(uint32_t chunkIndex) const
	{
		return mParams->chunkConvexHullStartIndices.buf[getPartIndex(chunkIndex)+1];
	}

	uint32_t						getChunkHullCount(uint32_t chunkIndex) const
	{
		const uint32_t partIndex = getPartIndex(chunkIndex);
		return mParams->chunkConvexHullStartIndices.buf[partIndex+1] - mParams->chunkConvexHullStartIndices.buf[partIndex];
	}

	uint32_t						getPartHullIndexStart(uint32_t partIndex) const
	{
		return mParams->chunkConvexHullStartIndices.buf[partIndex];
	}

	uint32_t						getPartHullIndexStop(uint32_t partIndex) const
	{
		return mParams->chunkConvexHullStartIndices.buf[partIndex+1];
	}

	NvParameterized::Interface**		getConvexHullParameters(uint32_t hullIndex) const
	{
		return mParams->chunkConvexHulls.buf + hullIndex;
	}

	PxBounds3					getChunkActorLocalBounds(uint32_t chunkIndex) const
	{
		const uint32_t partIndex = getPartIndex(chunkIndex);
		PxBounds3 bounds = renderMeshAsset->getBounds(partIndex);
		const PxVec3 offset = getChunkPositionOffset(chunkIndex);
		bounds.minimum += offset;
		bounds.maximum += offset;
		return bounds;
	}

	const PxBounds3&				getChunkShapeLocalBounds(uint32_t chunkIndex) const
	{
		const uint32_t partIndex = getPartIndex(chunkIndex);
		return renderMeshAsset->getBounds(partIndex);
	}

	void								applyTransformation(const PxMat44& transformation, float scale);
	void								applyTransformation(const PxMat44& transformation);
	bool                                setPlatformMaxDepth(PlatformTag platform, uint32_t maxDepth);
	bool                                removePlatformMaxDepth(PlatformTag platform);

	CachedOverlapsNS::IntPair_DynamicArray1D_Type*	getOverlapsAtDepth(uint32_t depth, bool create = true) const;

	void											clearChunkOverlaps(int32_t depth = -1, bool keepCachedFlag = false);
	void											addChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges);
	void											removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty);

	void								traceSurfaceBoundary(physx::Array<PxVec3>& outPoints, uint16_t chunkIndex, const PxTransform& localToWorldRT, const PxVec3& scale,
	        float spacing, float jitter, float surfaceDistance, uint32_t maxPoints);

	AuthObjTypeID						getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 						getObjTypeName() const
	{
		return getClassName();
	}
	const char*                         getName() const
	{
		return mName.c_str();
	}

	void								prepareForNewInstance();

	void								resetInstanceData();

	// DestructibleAssetAuthoring methods
	virtual bool						prepareForPlatform(nvidia::apex::PlatformTag platform) const;

	void								setFractureImpulseScale(float scale)
	{
		mParams->destructibleParameters.fractureImpulseScale = scale;
	}
	void								setImpactVelocityThreshold(float threshold)
	{
		mParams->destructibleParameters.impactVelocityThreshold = threshold;
	}
	void								setChunkOverlapsCacheDepth(int32_t depth = -1);
	void								setParameters(const DestructibleParameters&);
	void								setInitParameters(const DestructibleInitParameters&);
	void								setCrumbleEmitterName(const char*);
	void								setDustEmitterName(const char*);
	void								setFracturePatternName(const char*);
	void								setNeighborPadding(float neighborPadding)
	{
		mParams->neighborPadding = neighborPadding;
	}

	uint32_t						forceLoadAssets();
	void								initializeAssetNameTable();

	void								cleanup();
	ModuleDestructibleImpl* 				getOwner()
	{
		return module;
	}
	DestructibleAsset* 				getAsset()
	{
		return mNxAssetApi;
	}

	static AuthObjTypeID				getAssetTypeID()
	{
		return mAssetTypeID;
	}

	virtual NvParameterized::Interface* getDefaultActorDesc();
	virtual NvParameterized::Interface* getDefaultAssetPreviewDesc();
	virtual Actor*				createApexActor(const NvParameterized::Interface& params, Scene& apexScene);

	virtual AssetPreview*			createApexAssetPreview(const ::NvParameterized::Interface& /*params*/, AssetPreviewScene* /*previewScene*/);

	virtual bool						isValidForActorCreation(const ::NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/) const;

	uint32_t						getActorTransformCount() const
	{
		return (uint32_t)mParams->actorTransforms.arraySizes[0];
	}

	const PxMat44*				getActorTransforms() const
	{
		return mParams->actorTransforms.buf;
	}

	void								appendActorTransforms(const PxMat44* transforms, uint32_t transformCount);

	void								clearActorTransforms();

	bool								rebuildCollisionGeometry(uint32_t partIndex, const DestructibleGeometryDesc& geometryDesc);


	NvParameterized::Interface*			acquireNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = mParams;
		
		// don't set mParams to NULL here, because it's read during the asset release
		mOwnsParams = false;

		return ret;
	}

protected:

	void init();

	void calculateChunkDepthStarts();
	void calculateChunkOverlaps(physx::Array<IntPair>& overlaps, uint32_t depth) const;

	void reduceAccordingToLOD();

	void updateChunkInstanceRenderResources(bool rewriteBuffers, void* userRenderData);

	static bool chunksInProximity(const DestructibleAssetImpl& asset0, uint16_t chunkIndex0, const PxTransform& tm0, const PxVec3& scale0, 
								  const DestructibleAssetImpl& asset1, uint16_t chunkIndex1, const PxTransform& tm1, const PxVec3& scale1,
								  float padding);

	bool chunkAndSphereInProximity(uint16_t chunkIndex, const PxTransform& chunkTM, const PxVec3& chunkScale, 
								   const PxVec3& sphereWorldCenter, float sphereRadius, float padding, float* distance);

	static DestructibleParameters getParameters(const DestructibleAssetParametersNS::DestructibleParameters_Type&,
												  const DestructibleAssetParametersNS::DestructibleDepthParameters_DynamicArray1D_Type*);
	static void setParameters(const DestructibleParameters&, DestructibleAssetParametersNS::DestructibleParameters_Type&);

	static AuthObjTypeID		mAssetTypeID;
	static const char* 			getClassName()
	{
		return DESTRUCTIBLE_AUTHORING_TYPE_NAME;
	}
	ApexSimpleString			mName;

	DestructibleAsset* 		mNxAssetApi;
	ModuleDestructibleImpl* 		module;

	DestructibleAssetParameters*	mParams;
	bool							mOwnsParams;

	// Has a parameterized internal representation
	Array<ConvexHullImpl>			chunkConvexHulls;

	// Runtime / derived
	int32_t						chunkOverlapCacheDepth;
	Array<ResID>				mStaticMaterialIDs;

	ApexAssetTracker			mCrumbleAssetTracker;
	ApexAssetTracker			mDustAssetTracker;

	RenderMeshAsset*			renderMeshAsset;
	RenderMeshAsset*			runtimeRenderMeshAsset;

	physx::Array<PxConvexMesh*>*	mCollisionMeshes;

	uint32_t				mRuntimeCookedConvexCount;

	Array<RenderMeshAsset*>	scatterMeshAssets;

	// Instanced chunks
	uint16_t								m_instancedChunkMeshCount;
	physx::Array<RenderMeshActor*>			m_instancedChunkRenderMeshActors;	// One per render mesh actor per instanced chunk
	physx::Array<uint16_t>					m_instancedChunkActorVisiblePart;
	physx::Array<uint16_t>					m_instancedChunkActorMap;	// from instanced chunk instanceInfo index to actor index
	physx::Array<UserRenderInstanceBuffer*>	m_chunkInstanceBuffers;
	physx::Array< physx::Array< ChunkInstanceBufferDataElement > >	m_chunkInstanceBufferData;

	// Scatter meshes
	physx::Array<ScatterMeshInstanceInfo>		m_scatterMeshInstanceInfo;	// One per scatter mesh asset

	uint32_t				m_currentInstanceBufferActorAllowance;
	bool						m_needsInstanceBufferDataResize;
	bool						m_needsInstanceBufferResize;
	nvidia::Mutex				m_chunkInstanceBufferDataLock;

	bool						m_needsScatterMeshInstanceInfoCreation;

	int32_t				m_instancingRepresentativeActorIndex;	// -1 => means it's not set

	ResourceList				m_destructibleList;
	ResourceList				m_previewList;

	friend class DestructibleStructure;
	friend class DestructiblePreviewImpl;
	friend class DestructibleActorImpl;
	friend class DestructibleActorJointImpl;
	friend class DestructibleScene;
	friend class ModuleDestructibleImpl;
	friend class DestructibleAssetCollision;
	friend class DestructibleModuleCachedData;
	friend class ApexDamageEventReportDataImpl;
	friend class DestructibleRenderableImpl;
	
	NvParameterized::Interface* mApexDestructibleActorParams;

private:
	struct PlatformKeyValuePair
	{
		PlatformKeyValuePair(nvidia::apex::PlatformTag k, uint32_t v): key(k), val(v) {}
		~PlatformKeyValuePair() {}
		nvidia::apex::PlatformTag key;
		uint32_t val;
	private:
		PlatformKeyValuePair();
	};
	Array<PlatformKeyValuePair> m_platformFractureDepthMap;
	bool setDepthCount(uint32_t targetDepthCount) const;
};

#ifndef WITHOUT_APEX_AUTHORING

void gatherPartMesh(physx::Array<PxVec3>& vertices, physx::Array<uint32_t>& indices, const RenderMeshAsset* renderMeshAsset, uint32_t partIndex);

class DestructibleAssetAuthoringImpl : public DestructibleAssetImpl
{
public:

	ExplicitHierarchicalMeshImpl		hMesh;
	ExplicitHierarchicalMeshImpl		hMeshCore;
	CutoutSetImpl						cutoutSet;
	IntersectMesh					intersectMesh;

	DestructibleAssetAuthoringImpl(ModuleDestructibleImpl* module, DestructibleAsset* api, const char* name) :
		DestructibleAssetImpl(module, api, name) {}

	DestructibleAssetAuthoringImpl(ModuleDestructibleImpl* module, DestructibleAsset* api, NvParameterized::Interface* params, const char* name) :
		DestructibleAssetImpl(module, api, params, name) {}

	void	setToolString(const char* toolString);

	void	cookChunks(const DestructibleAssetCookingDesc&, bool cacheOverlaps, uint32_t* chunkIndexMapUser2Apex, uint32_t* chunkIndexMapApex2User, uint32_t chunkIndexMapCount);

	void	trimCollisionGeometry(const uint32_t* partIndices, uint32_t partIndexCount, float maxTrimFraction = 0.2f);

	void	serializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding) const;
	void	deserializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding);

private:
	void	trimCollisionGeometryInternal(const uint32_t* chunkIndices, uint32_t chunkIndexCount, const physx::Array<IntPair>& parentDepthOverlaps, uint32_t depth, float maxTrimFraction);
};
#endif

}
} // end namespace nvidia

#endif // DESTRUCTIBLE_ASSET_IMPL_H
