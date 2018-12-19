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



#ifndef APEX_RENDERMESH_ASSET_AUTHORING_H
#define APEX_RENDERMESH_ASSET_AUTHORING_H

#include "ApexRenderMeshAsset.h"
#include "ApexSharedUtils.h"
#include "RenderMeshAssetIntl.h"
#include "ResourceProviderIntl.h"
#include "ApexResource.h"
#include "ApexActor.h"
#include "ApexAssetAuthoring.h"
#include "ApexString.h"
#include "ApexVertexFormat.h"
#include "ApexSDKImpl.h"
#include "ApexUsingNamespace.h"
#include "ApexRWLockable.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace nvidia
{
namespace apex
{

// PHTODO, put this into the authoring asset
struct VertexReductionExtraData
{
	void	set(const ExplicitRenderTriangle& xTriangle)
	{
		smoothingMask = xTriangle.smoothingMask;
	}

	bool	canMerge(const VertexReductionExtraData& other) const
	{
		return (smoothingMask & other.smoothingMask) != 0 || smoothingMask == 0 || other.smoothingMask == 0;
	}

	uint32_t smoothingMask;
};


class ApexRenderMeshAssetAuthoring : public ApexRenderMeshAsset, public ApexAssetAuthoring, public RenderMeshAssetAuthoringIntl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ApexRenderMeshAssetAuthoring(ResourceList& list, RenderMeshAssetParameters* params, const char* name);

	void						release()
	{
		GetApexSDK()->releaseAssetAuthoring(*this);
	}

	void						createRenderMesh(const MeshDesc& desc, bool createMappingInformation);
	uint32_t				createReductionMap(uint32_t* map, const Vertex* vertices, const uint32_t* smoothingGroups, uint32_t vertexCount,
	        const PxVec3& positionTolerance, float normalTolerance, float UVTolerance);

	void						deleteStaticBuffersAfterUse(bool set)
	{
		ApexRenderMeshAsset::deleteStaticBuffersAfterUse(set);
	}

	const char*					getName(void) const
	{
		return ApexRenderMeshAsset::getName();
	}
	const char* 				getObjTypeName() const
	{
		return ApexRenderMeshAsset::getClassName();
	}
	bool						prepareForPlatform(nvidia::apex::PlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void						setToolString(const char* toolName, const char* toolVersion, uint32_t toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}
	uint32_t				getSubmeshCount() const
	{
		return ApexRenderMeshAsset::getSubmeshCount();
	}
	uint32_t				getPartCount() const
	{
		return ApexRenderMeshAsset::getPartCount();
	}
	const char* 				getMaterialName(uint32_t submeshIndex) const
	{
		return ApexRenderMeshAsset::getMaterialName(submeshIndex);
	}
	void						setMaterialName(uint32_t submeshIndex, const char* name);
	virtual void				setWindingOrder(uint32_t submeshIndex, RenderCullMode::Enum winding);
	virtual RenderCullMode::Enum	getWindingOrder(uint32_t submeshIndex) const;
	const RenderSubmesh&		getSubmesh(uint32_t submeshIndex) const
	{
		return ApexRenderMeshAsset::getSubmesh(submeshIndex);
	}
	RenderSubmesh&			getSubmeshWritable(uint32_t submeshIndex)
	{
		return *mSubmeshes[submeshIndex];
	}
	const PxBounds3&		getBounds(uint32_t partIndex = 0) const
	{
		return ApexRenderMeshAsset::getBounds(partIndex);
	}
	void						getStats(RenderMeshAssetStats& stats) const
	{
		ApexRenderMeshAsset::getStats(stats);
	}

	// From RenderMeshAssetAuthoringIntl
	RenderSubmeshIntl&		getInternalSubmesh(uint32_t submeshIndex)
	{
		return *ApexRenderMeshAsset::mSubmeshes[submeshIndex];
	}
	void						permuteBoneIndices(const physx::Array<int32_t>& old2new)
	{
		ApexRenderMeshAsset::permuteBoneIndices(old2new);
	}
	void						applyTransformation(const PxMat44& transformation, float scale)
	{
		ApexRenderMeshAsset::applyTransformation(transformation, scale);
	}
	void						applyScale(float scale)
	{
		ApexRenderMeshAsset::applyScale(scale);
	}
	void						reverseWinding()
	{
		ApexRenderMeshAsset::reverseWinding();
	}
	NvParameterized::Interface*	getNvParameterized() const
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

protected:
	// helper structs
	struct VertexPart
	{
		uint32_t	part, vertexIndex;
		PX_INLINE bool operator()(const VertexPart& a, const VertexPart& b) const
		{
			if (a.part != b.part)
			{
				return a.part < b.part;
			}
			return a.vertexIndex < b.vertexIndex;
		}
		PX_INLINE static int cmp(const void* A, const void* B)
		{
			// Sorts by part, then vertexIndex
			const int delta = (int)((VertexPart*)A)->part - (int)((VertexPart*)B)->part;
			return delta != 0 ? delta : ((int)((VertexPart*)A)->vertexIndex - (int)((VertexPart*)B)->vertexIndex);
		}
	};

	// helper methods
	template<typename PxU>
	bool fillSubmeshMap(physx::Array<VertexPart>& submeshMap, const void* const partIndicesVoid, uint32_t numParts,
	                    const void* const vertexIndicesVoid, uint32_t numSubmeshIndices, uint32_t numSubmeshVertices);

	// protected constructors
	ApexRenderMeshAssetAuthoring(ResourceList& list);
	virtual ~ApexRenderMeshAssetAuthoring();
};

}
} // end namespace nvidia::apex

#endif // WITHOUT_APEX_AUTHORING

#endif // APEX_RENDERMESH_ASSET_H
