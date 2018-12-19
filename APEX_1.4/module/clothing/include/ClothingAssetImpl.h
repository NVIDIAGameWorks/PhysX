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



#ifndef CLOTHING_ASSET_IMPL_H
#define CLOTHING_ASSET_IMPL_H

#include "ClothingAsset.h"

#include "ApexResource.h"
#include "PsMemoryBuffer.h"
#include "ApexRWLockable.h"
#include "ClothingCooking.h"
#include "ModuleClothingImpl.h"
#include "ModuleClothingHelpers.h"

#include "RenderDebugInterface.h"
#include "ApexAssetTracker.h"
#include "ParamArray.h"

#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace apex
{
struct AbstractMeshDescription;
class ApexActorSource;
class ApexRenderMeshAsset;
class PhysXObjectDescIntl;
class RenderMeshAssetIntl;

template <class T_Module, class T_Asset, class T_AssetAuthoring>
class ApexAuthorableObject;
}
namespace clothing
{
typedef ClothingGraphicalLodParametersNS::SkinClothMapB_Type SkinClothMapB;
typedef ClothingGraphicalLodParametersNS::SkinClothMapD_Type SkinClothMap;
typedef ClothingGraphicalLodParametersNS::TetraLink_Type TetraLink;


class ClothingActorImpl;
class ClothingActorProxy;
class ClothingAssetData;
class ClothingMaterial;
class ClothingPhysicalMeshImpl;
class CookingAbstract;
class ClothingPreviewProxy;

class SimulationAbstract;
class ClothingPlaneImpl;

#define NUM_VERTICES_PER_CACHE_BLOCK 8 // 128 / sizeof boneWeights per vertex (4 float), this is the biggest per vertex data


#define DEFAULT_PM_OFFSET_ALONG_NORMAL_FACTOR 0.1f // Magic value that phil introduced for the mesh-mesh-skinning

struct TetraEncoding
{
	float sign[4];
	uint32_t lastVtxIdx;
};



#define TETRA_LUT_SIZE 6
static const TetraEncoding tetraTable[TETRA_LUT_SIZE] =
{
	{ { 0,   0,  0, 1 },  0},
	{ { 1,   0,  0, 1 },  2},
	{ { 1,   0,  1, 1 },  1},

	{ { -1, -1, -1, 0 },  0},
	{ { 0,  -1, -1, 0 },  2},
	{ { 0,  -1,  0, 0 },  1}
};



struct ClothingGraphicalMeshAssetWrapper
{
	ClothingGraphicalMeshAssetWrapper(const RenderMeshAsset* renderMeshAsset) : meshAsset(renderMeshAsset)
	{
	}
	const RenderMeshAsset* meshAsset;

	uint32_t getSubmeshCount() const
	{
		if (meshAsset == NULL)
			return 0;

		return meshAsset->getSubmeshCount();
	}

	uint32_t getNumTotalVertices() const
	{
		if (meshAsset == NULL)
			return 0;

		uint32_t count = 0;
		for (uint32_t i = 0; i < meshAsset->getSubmeshCount(); i++)
		{
			count += meshAsset->getSubmesh(i).getVertexCount(0); // only 1 part is supported
		}
		return count;
	}

	uint32_t getNumVertices(uint32_t submeshIndex) const
	{
		if (meshAsset == NULL)
			return 0;

		return meshAsset->getSubmesh(submeshIndex).getVertexBuffer().getVertexCount();
	}

	uint32_t getNumBonesPerVertex(uint32_t submeshIndex) const
	{
		if (meshAsset == NULL)
			return 0;

		if (submeshIndex < meshAsset->getSubmeshCount())
		{
			const VertexFormat& format = meshAsset->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
			return vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, 
				format.getBufferFormat((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BONE_INDEX))));
		}
		return 0;
	}

	const void* getVertexBuffer(uint32_t submeshIndex, RenderVertexSemantic::Enum semantic, RenderDataFormat::Enum& outFormat) const
	{
		if (meshAsset == NULL)
			return NULL;

		const VertexBuffer& vb = meshAsset->getSubmesh(submeshIndex).getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
		return vb.getBufferAndFormat(outFormat, bufferIndex);
	}

	uint32_t getNumIndices(uint32_t submeshIndex)
	{
		if (meshAsset == NULL)
			return 0;

		return meshAsset->getSubmesh(submeshIndex).getIndexCount(0);
	}

	const void* getIndexBuffer(uint32_t submeshIndex)
	{
		if (meshAsset == NULL)
			return NULL;

		return meshAsset->getSubmesh(submeshIndex).getIndexBuffer(0);
	}

	bool hasChannel(const char* bufferName = NULL, RenderVertexSemantic::Enum semantic = RenderVertexSemantic::NUM_SEMANTICS) const
	{
		if (meshAsset == NULL)
			return false;

		PX_ASSERT((bufferName != NULL) != (semantic != RenderVertexSemantic::NUM_SEMANTICS));
		PX_ASSERT((bufferName == NULL) != (semantic == RenderVertexSemantic::NUM_SEMANTICS));

		for (uint32_t i = 0; i < meshAsset->getSubmeshCount(); i++)
		{
			RenderDataFormat::Enum outFormat = RenderDataFormat::UNSPECIFIED;
			const VertexFormat& format = meshAsset->getSubmesh(i).getVertexBuffer().getFormat();

			VertexFormat::BufferID id = bufferName ? format.getID(bufferName) : format.getSemanticID(semantic);
			outFormat = format.getBufferFormat((uint32_t)format.getBufferIndexFromID(id));

			if (outFormat != RenderDataFormat::UNSPECIFIED)
			{
				return true;
			}
		}
		return false;
	}
private:
	void operator=(ClothingGraphicalMeshAssetWrapper&);
};



class ClothingAssetImpl : public ClothingAsset, public ApexResourceInterface, public ApexResource, public ClothingCookingLock, public NvParameterized::SerializationCallback, public ApexRWLockable
{
protected:
	// used for authoring asset creation only!
	ClothingAssetImpl(ModuleClothingImpl* module, ResourceList& list, const char* name);
	ClothingAssetImpl(ModuleClothingImpl*, ResourceList&, NvParameterized::Interface*, const char*);

public:
	APEX_RW_LOCKABLE_BOILERPLATE

	uint32_t initializeAssetData(ClothingAssetData& assetData, const uint32_t uvChannel);

	// from Asset
	PX_INLINE const char*							getName() const
	{
		return mName.c_str();
	}
	PX_INLINE AuthObjTypeID							getObjTypeID() const
	{
		return mAssetTypeID;
	}
	PX_INLINE const char*							getObjTypeName() const
	{
		return CLOTHING_AUTHORING_TYPE_NAME;
	}

	virtual uint32_t								forceLoadAssets();
	virtual NvParameterized::Interface*				getDefaultActorDesc();
	virtual NvParameterized::Interface*				getDefaultAssetPreviewDesc();
	virtual const NvParameterized::Interface*		getAssetNvParameterized() const
	{
		return mParams;
	}
	virtual Actor*									createApexActor(const NvParameterized::Interface& params, Scene& apexScene);

	virtual AssetPreview*							createApexAssetPreview(const ::NvParameterized::Interface& /*params*/, AssetPreviewScene* /*previewScene*/)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual NvParameterized::Interface*				releaseAndReturnNvParameterizedInterface();
	virtual bool									isValidForActorCreation(const ::NvParameterized::Interface& parms, Scene& apexScene) const;
	virtual bool									isDirty() const;

	// from ApexInterface
	virtual void									release();

	// from ClothingAsset
	PX_INLINE uint32_t								getNumActors() const
	{
		READ_ZONE();
		return mActors.getSize();
	}
	virtual ClothingActor*							getActor(uint32_t index);
	PX_INLINE PxBounds3								getBoundingBox() const
	{
		READ_ZONE();
		return mParams->boundingBox;
	}
	virtual float									getMaximumSimulationBudget(uint32_t solverIterations) const;
	virtual uint32_t								getNumGraphicalLodLevels() const;
	virtual uint32_t								getGraphicalLodValue(uint32_t lodLevel) const;
	virtual float									getBiggestMaxDistance() const;
	virtual bool									remapBoneIndex(const char* name, uint32_t newIndex);
	PX_INLINE uint32_t								getNumBones() const
	{
		READ_ZONE();
		return mBones.size();
	}
	PX_INLINE uint32_t								getNumUsedBones() const
	{
		READ_ZONE();
		return mParams->bonesReferenced;
	}
	PX_INLINE uint32_t								getNumUsedBonesForMesh() const
	{
		return mParams->bonesReferencedByMesh;
	}
	virtual const char*								getBoneName(uint32_t internalIndex) const;
	virtual bool									getBoneBasePose(uint32_t internalIndex, PxMat44& result) const;
	virtual void									getBoneMapping(uint32_t* internal2externalMap) const;
	virtual uint32_t								prepareMorphTargetMapping(const PxVec3* originalPositions, uint32_t numPositions, float epsilon);

	// from ApexResource
	uint32_t										getListIndex() const
	{
		READ_ZONE();
		return m_listIndex;
	}
	void											setListIndex(class ResourceList& list, uint32_t index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	// from NvParameterized::SerializationCallback
	virtual void									preSerialize(void* userData_);

	// graphical meshes
	PX_INLINE uint32_t								getNumGraphicalMeshes() const
	{
		return mGraphicalLods.size();
	}

	RenderMeshAssetIntl*							getGraphicalMesh(uint32_t index);
	const ClothingGraphicalLodParameters*			getGraphicalLod(uint32_t index) const;

	// actor handling
	void											releaseClothingActor(ClothingActor& actor);
	void											releaseClothingPreview(ClothingPreview& preview);

	// module stuff
	PX_INLINE ModuleClothingImpl*					getModuleClothing() const
	{
		PX_ASSERT(mModule != NULL);
		return mModule;
	}

	// actor access to the asset
	bool											writeBoneMatrices(PxMat44 localPose, const PxMat44* newBoneMatrices,
	        const uint32_t byteStride, const uint32_t numBones, PxMat44* dest, bool isInternalOrder, bool multInvBindPose);
	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* getPhysicalMeshFromLod(uint32_t graphicalLodId) const;

	void											releaseCookedInstances();

	PX_INLINE bool									getSimulationDisableCCD() const
	{
		return mParams->simulation.disableCCD;
	}
	ClothingPhysicalMeshParametersNS::SkinClothMapB_Type* getTransitionMapB(uint32_t dstPhysicalMeshId, uint32_t srcPhysicalMeshId, float& thickness, float& offset);
	ClothingPhysicalMeshParametersNS::SkinClothMapD_Type* getTransitionMap(uint32_t dstPhysicalMeshId, uint32_t srcPhysicalMeshId, float& thickness, float& offset);

	// cooked stuff
	NvParameterized::Interface*						getCookedData(float actorScale);
	uint32_t										getCookedPhysXVersion() const;
	ClothSolverMode::Enum							getClothSolverMode() const;

	// create deformables
	SimulationAbstract*								getSimulation(uint32_t physicalMeshId, NvParameterized::Interface* cookedParam, ClothingScene* clothingScene);
	void											returnSimulation(SimulationAbstract* simulation);
	void											destroySimulation(SimulationAbstract* simulation);

	void											initCollision(	SimulationAbstract* simulation, const PxMat44* boneTansformations,
																	ResourceList& actorPlanes,
																	ResourceList& actorConvexes,
																	ResourceList& actorSpheres,
																	ResourceList& actorCapsules,
																	ResourceList& actorTriangleMeshes,
																	const ClothingActorParam* actorParam,
																	const PxMat44& globalPose, bool localSpaceSim);

	void											updateCollision(SimulationAbstract* simulation, const PxMat44* boneTansformationse,
																	ResourceList& actorPlanes,
																	ResourceList& actorConvexes,
																	ResourceList& actorSpheres,
																	ResourceList& actorCapsules,
																	ResourceList& actorTriangleMeshes,
																	bool teleport);

	uint32_t										getPhysicalMeshID(uint32_t graphicalLodId) const;

	// bone stuff
	PX_INLINE const PxMat44&						getBoneBindPose(uint32_t i)
	{
		PX_ASSERT(i < mBones.size());
		return mBones[i].bindPose;
	}

	PX_INLINE uint32_t								getBoneExternalIndex(uint32_t i)
	{
		PX_ASSERT(i < mBones.size());
		return (uint32_t)mBones[i].externalIndex;
	}

	// debug rendering
	void											visualizeSkinCloth(RenderDebugInterface& renderDebug,
																		AbstractMeshDescription& srcPM, bool showTets, float actorScale) const;
	void											visualizeSkinClothMap(RenderDebugInterface& renderDebug, AbstractMeshDescription& srcPM,
																		SkinClothMapB* skinClothMapB, uint32_t skinClothMapBSize,
																		SkinClothMap* skinClothMap, uint32_t skinClothMapSize,
																		float actorScale, bool onlyBad, bool invalidBary) const;
	void											visualizeBones(RenderDebugInterface& renderDebug, const PxMat44* matrices, bool skeleton, float boneFramesScale, float boneNamesScale);


	// expose render data
	virtual const RenderMeshAsset*					getRenderMeshAsset(uint32_t lodLevel) const;
	virtual uint32_t								getMeshSkinningMapSize(uint32_t lod);
	virtual void									getMeshSkinningMap(uint32_t lod, ClothingMeshSkinningMap* map);
	virtual bool									releaseGraphicalData();

	void											setupInvBindMatrices();

	// unified cooking
	void											prepareCookingJob(CookingAbstract& job, float scale, PxVec3* gravityDirection, PxVec3* morphedPhysicalMesh);

	// morph targets
	uint32_t*										getMorphMapping(uint32_t graphicalLod, uint32_t submeshIndex);
	uint32_t										getPhysicalMeshOffset(uint32_t physicalMeshId);
	void											getDisplacedPhysicalMeshPositions(PxVec3* morphDisplacements, ParamArray<PxVec3> displacedMeshPositions);

	// faster cpu skinning
	void											initializeCompressedNumBonesPerVertex();
	uint32_t										getRootBoneIndex();

	uint32_t										getInterCollisionChannels();

protected:
	void											destroy();

	int32_t											getBoneInternalIndex(const char* boneName) const;
	int32_t											getBoneInternalIndex(uint32_t boneIndex) const;

	bool											reorderGraphicsVertices(uint32_t graphicalLodId, bool perfWarning);
	bool											reorderDeformableVertices(ClothingPhysicalMeshImpl& physicalMesh);

	float											getMaxMaxDistance(ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, 
																		uint32_t index, uint32_t numIndices) const;

	uint32_t										getCorrespondingPhysicalVertices(const ClothingGraphicalLodParameters& graphLod, uint32_t submeshIndex,
																					uint32_t graphicalVertexIndex, const AbstractMeshDescription& pMesh,
																					uint32_t submeshVertexOffset, uint32_t indices[4], float trust[4]) const;

	void											getNormalsAndVerticesForFace(PxVec3* vtx, PxVec3* nrm, uint32_t i,
																					const AbstractMeshDescription& srcPM) const;
	bool											setBoneName(uint32_t internalIndex, const char* name);
	void											clearMapping(uint32_t graphicalLodId);
	void											updateBoundingBox();

	bool											mergeMapping(ClothingGraphicalLodParameters* graphicalLod);
	bool											findTriangleForImmediateVertex(uint32_t& faceIndex, uint32_t& indexInTriangle, uint32_t physVertIndex, 
																					ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh) const;

#ifndef WITHOUT_PVD
	void											initPvdInstances(pvdsdk::PvdDataStream& pvdStream);
	void											destroyPvdInstances();
#endif

	// the parent
	ModuleClothingImpl* mModule;

	// the parameterized object
	ClothingAssetParameters* mParams;

	// the meshes
	ParamArray<ClothingPhysicalMeshParameters*> mPhysicalMeshes;

	ParamArray<ClothingGraphicalLodParameters*> mGraphicalLods;

	mutable ParamArray<ClothingAssetParametersNS::BoneEntry_Type> mBones;
	Array<PxMat44> mInvBindPoses; // not serialized!

	mutable ParamArray<ClothingAssetParametersNS::BoneSphere_Type> mBoneSpheres;
	mutable ParamArray<uint16_t> mSpherePairs;
	mutable ParamArray<ClothingAssetParametersNS::ActorEntry_Type> mBoneActors;
	mutable ParamArray<PxVec3> mBoneVertices;
	mutable ParamArray<ClothingAssetParametersNS::BonePlane_Type> mBonePlanes;
	mutable ParamArray<uint32_t> mCollisionConvexes; // bitmap for indices into mBonePlanes array

private:
	// internal methods
	float												getMaxDistReduction(ClothingPhysicalMeshParameters& physicalMesh, float maxDistanceMultiplier) const;

	static const char*									getClassName()
	{
		return CLOTHING_AUTHORING_TYPE_NAME;
	}

	Array<uint32_t>										mCompressedNumBonesPerVertex;
	Array<uint32_t>										mCompressedTangentW;
	nvidia::Mutex										mCompressedNumBonesPerVertexMutex;

	ApexSimpleString									mName;

	// Keep track of all ClothingActorImpl objects created from this ClothingAssetImpl
	ResourceList										mActors;
	ResourceList										mPreviews;


	Array<SimulationAbstract*>							mUnusedSimulation;
	nvidia::Mutex										mUnusedSimulationMutex;

	static AuthObjTypeID mAssetTypeID;
	friend class ModuleClothingImpl;


	Array<uint32_t>										mExt2IntMorphMapping;
	uint32_t											mExt2IntMorphMappingMaxValue; // this is actually one larger than max

	bool												mDirty;
	bool												mMorphMappingWarning;

	template <class T_Module, class T_Asset, class T_AssetAuthoring>
	friend class nvidia::apex::ApexAuthorableObject;
};

}
} // namespace nvidia

#endif // CLOTHING_ASSET_IMPL_H
