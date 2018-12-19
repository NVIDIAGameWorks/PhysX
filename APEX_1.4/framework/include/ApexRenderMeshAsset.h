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



#ifndef APEX_RENDERMESH_ASSET_H
#define APEX_RENDERMESH_ASSET_H

#include "RenderMeshAssetIntl.h"
#include "ApexResource.h"
#include "ApexSDKHelpers.h"
#include "ResourceProviderIntl.h"
#include "ApexRenderSubmesh.h"

#include "RenderMeshAssetParameters.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace apex
{

/**
ApexRenderMeshAsset - a collection of ApexRenderMeshParts and submesh extra data
*/
class ApexRenderMeshAsset : public RenderMeshAssetIntl, public ApexResourceInterface, public ApexRWLockable, public ApexResource
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ApexRenderMeshAsset(ResourceList& list, const char* name, AuthObjTypeID ownerModuleID);
	~ApexRenderMeshAsset();

	struct SubmeshData
	{
		UserRenderVertexBuffer* staticVertexBuffer;
		UserRenderVertexBuffer* skinningVertexBuffer;
		UserRenderVertexBuffer* dynamicVertexBuffer;
		bool needsStaticData;
		bool needsDynamicData;
	};
	struct CustomSubmeshData
	{
		Array<RenderDataFormat::Enum>	customBufferFormats;
		Array<void*>					customBufferVoidPtrs;
	};


	void						release()
	{
		GetApexSDK()->releaseAsset(*this);
	}

	void						destroy();

	AuthObjTypeID				getObjTypeID() const
	{
		return mObjTypeID;
	}
	const char* 				getObjTypeName() const
	{
		return getClassName();
	}
	uint32_t						forceLoadAssets();
	void						deleteStaticBuffersAfterUse(bool set)
	{
		mParams->deleteStaticBuffersAfterUse = set;
	}

	RenderMeshActor* 			createActor(const RenderMeshActorDesc& desc);
	void                        releaseActor(RenderMeshActor& renderMeshActor);

	const char* 				getName(void) const
	{
		return mName.c_str();
	}
	uint32_t						getSubmeshCount() const
	{
		return (uint32_t)mParams->submeshes.arraySizes[0];
	}
	uint32_t						getPartCount() const
	{
		return (uint32_t)mParams->partBounds.arraySizes[0];
	}
	uint32_t						getBoneCount() const
	{
		return mParams->boneCount;
	}
	const RenderSubmesh&		getSubmesh(uint32_t submeshIndex) const
	{
		return *mSubmeshes[submeshIndex];
	}
	const PxBounds3&			getBounds(uint32_t partIndex = 0) const
	{
		return mParams->partBounds.buf[partIndex];
	}
	void						getStats(RenderMeshAssetStats& stats) const;

	// from RenderMeshAssetIntl
	RenderSubmeshIntl&		getInternalSubmesh(uint32_t submeshIndex)
	{
		return *mSubmeshes[submeshIndex];
	}
	void						permuteBoneIndices(const Array<int32_t>& old2new);
	void						applyTransformation(const PxMat44& transformation, float scale);
	void						reverseWinding();
	void						applyScale(float scale);
	bool						mergeBinormalsIntoTangents();
	void						setOwnerModuleId(AuthObjTypeID id)
	{
		mOwnerModuleID = id;
	}
	TextureUVOrigin::Enum		getTextureUVOrigin() const;

	const char* 				getMaterialName(uint32_t submeshIndex) const
	{
		return mParams->materialNames.buf[submeshIndex];
	}

	// ApexResourceInterface methods
	void						setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	uint32_t						getListIndex() const
	{
		return m_listIndex;
	}

	/* Common data for all ApexRenderMeshAssets */
	static AuthObjTypeID		mObjTypeID;
	static const char*          getClassName()
	{
		return RENDER_MESH_AUTHORING_TYPE_NAME;
	};

	const NvParameterized::Interface* getAssetNvParameterized() const
	{
		return mParams;
	}

	/**
	 * \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	 */
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
	NvParameterized::Interface* getDefaultActorDesc()
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	};

	NvParameterized::Interface* getDefaultAssetPreviewDesc()
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	};

	virtual Actor* createApexActor(const NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/)
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	}

	virtual AssetPreview* createApexAssetPreview(const NvParameterized::Interface& /*params*/, AssetPreviewScene* /*previewScene*/)
	{
		APEX_INVALID_OPERATION("Not yet implemented!");
		return NULL;
	}

	void setOpaqueMesh(UserOpaqueMesh* om)
	{
		mOpaqueMesh = om;
	}

	virtual UserOpaqueMesh* getOpaqueMesh(void) const
	{
		return mOpaqueMesh;
	}

	virtual bool isValidForActorCreation(const ::NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	ApexRenderMeshAsset() {}	// Nop constructor for use by RenderMeshAssetAuthoring

	void						setSubmeshCount(uint32_t submeshCount);


	void						createLocalData();

	bool						createFromParameters(RenderMeshAssetParameters* params);

	void						updatePartBounds();

	AuthObjTypeID				mOwnerModuleID;
	RenderMeshAssetParameters*	mParams;
	UserOpaqueMesh*			mOpaqueMesh;

	// Name should not be serialized
	ApexSimpleString			mName;

	// Local (not serialized) data
	Array<ApexRenderSubmesh*>	mSubmeshes;

	Array<ResID>				mMaterialIDs;
	ResourceList				mActorList;
	Array<SubmeshData>			mRuntimeSubmeshData;
	Array<CustomSubmeshData>	mRuntimeCustomSubmeshData;

	friend class ApexRenderMeshActor;
	friend class RenderMeshAuthorableObject;
};

} // namespace apex
} // namespace nvidia

#endif // APEX_RENDERMESH_ASSET_H
