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



#ifndef APEX_RENDERMESH_ACTOR_H
#define APEX_RENDERMESH_ACTOR_H

#include "ApexUsingNamespace.h"
#include "RenderMeshAssetIntl.h"
#include "ApexActor.h"

#include "ApexRenderMeshAsset.h"
#include "ApexSharedUtils.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace apex
{

// enables a hack that removes dead particles from the instance list prior to sending it to the application.
// this is a bad hack because it requires significant additional memory and copies.
#define ENABLE_INSTANCED_MESH_CLEANUP_HACK 0


/*
	ApexRenderMeshActor - an instantiation of an ApexRenderMeshAsset
 */
class ApexRenderMeshActor : public RenderMeshActorIntl, public ApexResourceInterface, public ApexResource, public ApexActor, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	void					release();
	Asset*			getOwner() const
	{
		READ_ZONE();
		return mRenderMeshAsset;
	}
	PxBounds3		getBounds() const
	{
		READ_ZONE();
		return ApexRenderable::getBounds();
	}
	void					lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}
	void					unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}

	// RenderMeshActors have global context, ignore ApexActor scene methods
#if PX_PHYSICS_VERSION_MAJOR == 3
	void					setPhysXScene(PxScene*)		{ }
	PxScene*				getPhysXScene() const
	{
		return NULL;
	}
#endif

	bool					getVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const;

	bool					setVisibility(bool visible, uint16_t partIndex = 0);
	bool					isVisible(uint16_t partIndex = 0) const
	{
		READ_ZONE();
		return mVisiblePartsForAPI.isUsed(partIndex);
	}

	uint32_t visiblePartCount() const
	{
		READ_ZONE();
		return mVisiblePartsForAPI.usedCount();
	}
	const uint32_t* getVisibleParts() const
	{
		READ_ZONE();
		return mVisiblePartsForAPI.usedIndices();
	}

	uint32_t getRenderVisiblePartCount() const
	{
		return mBufferVisibility ? mVisiblePartsForRendering.size() : mVisiblePartsForAPI.usedCount();
	}
	const uint32_t* getRenderVisibleParts() const
	{
		return mBufferVisibility ? mVisiblePartsForRendering.begin() : mVisiblePartsForAPI.usedIndices();
	}

	virtual uint32_t getBoneCount() const
	{
		READ_ZONE();
		return mRenderMeshAsset->getBoneCount();
	}

	void setTM(const PxMat44& tm, uint32_t boneIndex = 0);
	void setTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex = 0);

	const PxMat44 getTM(uint32_t boneIndex = 0) const
	{
		READ_ZONE();
		return accessTM(boneIndex);
	}

	void setLastFrameTM(const PxMat44& tm, uint32_t boneIndex = 0);
	void setLastFrameTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex = 0);

	void setSkinningMode(RenderMeshActorSkinningMode::Enum mode);
	RenderMeshActorSkinningMode::Enum getSkinningMode() const;

	void					syncVisibility(bool useLock = true);

	void					updateBounds();
	void					updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void					updateRenderResources(bool useBones, bool rewriteBuffers, void* userRenderData);
	void					dispatchRenderResources(UserRenderer&);
	void					dispatchRenderResources(UserRenderer&, const PxMat44&);

	void					setReleaseResourcesIfNothingToRender(bool value);

	void					setBufferVisibility(bool bufferVisibility);

	void					setOverrideMaterial(uint32_t index, const char* overrideMaterialName);

	//UserRenderVertexBuffer* getUserVertexBuffer(uint32_t submeshIndex) { if (submeshIndex < mSubmeshData.size()) return renderMeshAsset->vertexBuffers[submeshIndex]; return NULL; }
	//UserRenderIndexBuffer* getUserIndexBuffer(uint32_t submeshIndex) { if (submeshIndex < mSubmeshData.size()) return mSubmeshData[submeshIndex].indexBuffer; return NULL; }

	void					addVertexBuffer(uint32_t submeshIndex, bool alwaysDirty, PxVec3* position, PxVec3* normal, PxVec4* tangents);
	void					removeVertexBuffer(uint32_t submeshIndex);

	void					setStaticPositionReplacement(uint32_t submeshIndex, const PxVec3* staticPositions);
	void					setStaticColorReplacement(uint32_t submeshIndex, const ColorRGBA* staticColors);

	virtual UserRenderInstanceBuffer* getInstanceBuffer() const
	{
		READ_ZONE();
		return mInstanceBuffer;
	}
	/// \sa RenderMeshActor::setInstanceBuffer
	virtual void			setInstanceBuffer(UserRenderInstanceBuffer* instBuf);
	virtual void			setMaxInstanceCount(uint32_t count);
	/// \sa RenderMeshActor::setInstanceBufferRange
	virtual void			setInstanceBufferRange(uint32_t from, uint32_t count);

	// ApexResourceInterface methods
	void					setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	uint32_t					getListIndex() const
	{
		return m_listIndex;
	}

	virtual void			getLodRange(float& min, float& max, bool& intOnly) const;
	virtual float			getActiveLod() const;
	virtual void			forceLod(float lod);
	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		WRITE_ZONE();
		ApexActor::setEnableDebugVisualization(state);
	}

	virtual bool			rayCast(RenderMeshActorRaycastHitData& hitData,
	                                const PxVec3& worldOrig, const PxVec3& worldDisp,
	                                RenderMeshActorRaycastFlags::Enum flags = RenderMeshActorRaycastFlags::VISIBLE_PARTS,
	                                RenderCullMode::Enum winding = RenderCullMode::CLOCKWISE,
	                                int32_t partIndex = -1) const;

	virtual void			visualize(RenderDebugInterface& batcher, nvidia::apex::DebugRenderParams* debugParams, PxMat33* scaledRotations, PxVec3* translations, uint32_t stride, uint32_t numberOfTransforms) const;

protected:
	ApexRenderMeshActor(const RenderMeshActorDesc& desc, ApexRenderMeshAsset& _renderMeshData, ResourceList& list);
	virtual					~ApexRenderMeshActor();

	struct SubmeshData;
	void					loadMaterial(SubmeshData& submeshData);
	void					init(const RenderMeshActorDesc& desc, uint16_t partCount, uint16_t boneCount);
	void					destroy();

	PxMat44&				accessTM(uint32_t boneIndex = 0) const
	{
		return (PxMat44&)mTransforms[mKeepVisibleBonesPacked ? mVisiblePartsForAPI.getRank(boneIndex) : boneIndex];
	}

	PxMat44&				accessLastFrameTM(uint32_t boneIndex = 0) const
	{
		return (PxMat44&)mTransformsLastFrame[mKeepVisibleBonesPacked ? mVisiblePartsForAPILastFrame.getRank(boneIndex) : boneIndex];
	}

	/* Internal rendering APIs */
	void					createRenderResources(bool useBones, void* userRenderData);
	void					updatePartVisibility(uint32_t submeshIndex, bool useBones, void* userRenderData);
	void					updateBonePoses(uint32_t submeshIndex);
	void					updateInstances(uint32_t submeshIndex);
	void					releaseSubmeshRenderResources(uint32_t submeshIndex);
	void					releaseRenderResources();
	bool					submeshHasVisibleTriangles(uint32_t submeshIndex) const;

	// Fallback skinning
	void					createFallbackSkinning(uint32_t submeshIndex);
	void					distributeFallbackData(uint32_t submeshIndex);
	void					updateFallbackSkinning(uint32_t submeshIndex);
	void					writeUserBuffers(uint32_t submeshIndex);

	// Debug rendering
	void					visualizeTangentSpace(RenderDebugInterface& batcher, float normalScale, float tangentScale, float bitangentScale, PxMat33* scaledRotations, PxVec3* translations, uint32_t stride, uint32_t numberOfTransforms) const;

	ApexRenderMeshAsset*	mRenderMeshAsset;
	Array<PxMat44>	mTransforms;
	Array<PxMat44>	mTransformsLastFrame;

	struct ResourceData
	{
		ResourceData() : resource(NULL), vertexCount(0), boneCount(0) {}
		UserRenderResource*	resource;
		uint32_t					vertexCount;
		uint32_t					boneCount;
	};

	struct SubmeshData
	{
		SubmeshData();
		~SubmeshData();

		Array<ResourceData>				renderResources;
		UserRenderIndexBuffer*		indexBuffer;
		void*							fallbackSkinningMemory;
		UserRenderVertexBuffer*		userDynamicVertexBuffer;
		UserRenderInstanceBuffer*		instanceBuffer;

		PxVec3*							userPositions;
		PxVec3*							userNormals;
		PxVec4*							userTangents4;

		// And now we have colors
		const ColorRGBA*				staticColorReplacement;
		bool							staticColorReplacementDirty;

		// These are needed if the positions vary from what the asset has stored. Can happen with morph targets.
		// so far we only replace positions, more can be added here
		const PxVec3*					staticPositionReplacement;
		UserRenderVertexBuffer*		staticBufferReplacement;
		UserRenderVertexBuffer*		dynamicBufferReplacement;

		uint32_t							fallbackSkinningMemorySize;
		uint32_t							visibleTriangleCount;
		ResID							materialID;
		void*							material;
		bool							isMaterialPointerValid;
		uint32_t							maxBonesPerMaterial;
		uint32_t							indexBufferRequestedSize;
		bool							userSpecifiedData;
		bool							userVertexBufferAlwaysDirty;
		bool							userIndexBufferChanged;
		bool							fallbackSkinningDirty;
	};

	nvidia::Array<SubmeshData>			mSubmeshData;
	IndexBank<uint32_t>					mVisiblePartsForAPI;
	IndexBank<uint32_t>					mVisiblePartsForAPILastFrame;
	RenderBufferHint::Enum			mIndexBufferHint;

	Array<uint32_t>						mVisiblePartsForRendering;

	Array<UserRenderVertexBuffer*>	mPerActorVertexBuffers;

	// Instancing
	uint32_t								mMaxInstanceCount;
	uint32_t								mInstanceCount;
	uint32_t								mInstanceOffset;
	UserRenderInstanceBuffer*			mInstanceBuffer;
	UserRenderResource*				mRenderResource;

	// configuration
	bool								mRenderWithoutSkinning;
	bool								mForceBoneIndexChannel;
	bool								mApiVisibilityChanged;
	bool								mPartVisibilityChanged;
	bool								mInstanceCountChanged;
	bool								mKeepVisibleBonesPacked;
	bool								mForceFallbackSkinning;
	bool								mBonePosesDirty;
	bool								mOneUserVertexBufferChanged; // this will be true if one or more user vertex buffers changed
	bool								mBoneBufferInUse;
	bool								mReleaseResourcesIfNothingToRender; // is this ever set? I think we should remove it.
	bool								mCreateRenderResourcesAfterInit; // only needed when mRenderWithoutSkinning is set to true
	bool								mBufferVisibility;
	bool								mKeepPreviousFrameBoneBuffer;
	bool								mPreviousFrameBoneBufferValid;
	RenderMeshActorSkinningMode::Enum	mSkinningMode;
	Array<uint32_t>						mTMSwapBuffer;
	Array<PxMat44>						mRemappedPreviousBoneTMs;

	// temporary
	Array<uint16_t>						mBoneIndexTempBuffer;

	// CM: For some reason the new operator doesn't see these members and allocates not enough memory for the class (on a 64 bit build)
	// Even though ENABLE_INSTANCED_MESH_CLEANUP_HACK is defined to be 1 earlier in this file.
	// For now, these members are permanently here, since it's a tiny amount of memory if the define is off, and it fixes the allocation problem.
//#if ENABLE_INSTANCED_MESH_CLEANUP_HACK
	void*								mOrderedInstanceTemp;
	uint32_t							mOrderedInstanceTempSize;
//#endif

	Array<uint32_t>						mPartIndexTempBuffer;

	friend class ApexRenderMeshAsset;
};

} // namespace apex
} // namespace nvidia

#endif // APEX_RENDERMESH_ACTOR_H
