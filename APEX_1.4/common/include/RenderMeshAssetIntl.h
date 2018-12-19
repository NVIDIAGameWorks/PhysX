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



#ifndef RENDER_MESH_ASSET_INTL_H
#define RENDER_MESH_ASSET_INTL_H

#include "ApexUsingNamespace.h"
#include "RenderMeshActor.h"
#include "RenderMeshAsset.h"
#include "PsArray.h"
#include "PxMat44.h"


/**
\brief Describes how bones are to be assigned to render resources.
*/
struct RenderMeshActorSkinningMode
{
	enum Enum
	{
		Default,			// Currently the same as OneBonePerPart
		OneBonePerPart,		// Used by destruction, default behavior
		AllBonesPerPart,	// All bones are written to each render resource, up to the maximum bones per material given by UserRenderResourceManager::getMaxBonesForMaterial

		Count
	};
};


namespace nvidia
{
namespace apex
{

// Forward declarations
class DebugRenderParams;
class RenderDebugInterface;

class VertexBufferIntl : public VertexBuffer
{
public:
	virtual ~VertexBufferIntl() {}
	virtual VertexFormat& getFormatWritable() = 0;
	virtual void build(const VertexFormat& format, uint32_t vertexCount) = 0;
	virtual void applyTransformation(const PxMat44& transformation) = 0;
	virtual void applyScale(float scale) = 0;
	virtual bool mergeBinormalsIntoTangents() = 0;
};

class RenderSubmeshIntl : public RenderSubmesh
{
public:
	virtual ~RenderSubmeshIntl() {}

	virtual VertexBufferIntl& getVertexBufferWritable() = 0;
	virtual uint32_t* getIndexBufferWritable(uint32_t partIndex) = 0;
	virtual void applyPermutation(const Array<uint32_t>& old2new, const Array<uint32_t>& new2old) = 0;
};


/**
* Framework interface to ApexRenderMesh for use by modules
*/
class RenderMeshAssetIntl : public RenderMeshAsset
{
public:
	virtual RenderSubmeshIntl&	getInternalSubmesh(uint32_t submeshIndex) = 0;
	virtual void					permuteBoneIndices(const physx::Array<int32_t>& old2new) = 0;
	virtual void					applyTransformation(const PxMat44& transformation, float scale) = 0;
	virtual void					reverseWinding() = 0;
	virtual void					applyScale(float scale) = 0;
	virtual bool					mergeBinormalsIntoTangents() = 0;
	virtual void					setOwnerModuleId(AuthObjTypeID id) = 0;
	virtual TextureUVOrigin::Enum getTextureUVOrigin() const = 0;

};

class RenderMeshAssetAuthoringIntl : public RenderMeshAssetAuthoring
{
public:
	virtual RenderSubmeshIntl&	getInternalSubmesh(uint32_t submeshIndex) = 0;
	virtual void					permuteBoneIndices(const physx::Array<int32_t>& old2new) = 0;
	virtual void					applyTransformation(const PxMat44& transformation, float scale) = 0;
	virtual void					reverseWinding() = 0;
	virtual void					applyScale(float scale) = 0;
};


class RenderMeshActorIntl : public RenderMeshActor
{
public:
	virtual void updateRenderResources(bool useBones, bool rewriteBuffers, void* userRenderData) = 0;

	// add a buffer that will replace the dynamic buffer for the submesh
	virtual void addVertexBuffer(uint32_t submeshIndex, bool alwaysDirty, PxVec3* position, PxVec3* normal, PxVec4* tangents) = 0;
	virtual void removeVertexBuffer(uint32_t submeshIndex) = 0;

	virtual void setStaticPositionReplacement(uint32_t submeshIndex, const PxVec3* staticPositions) = 0;
	virtual void setStaticColorReplacement(uint32_t submeshIndex, const ColorRGBA* staticColors) = 0;

	virtual void visualize(RenderDebugInterface& batcher, nvidia::apex::DebugRenderParams* debugParams, PxMat33* scaledRotations = NULL, PxVec3* translations = NULL, uint32_t stride = 0, uint32_t numberOfTransforms = 0) const = 0;

	virtual void dispatchRenderResources(UserRenderer& renderer, const PxMat44& globalPose) = 0;

	// Access to previous frame's transforms (if the buffer exists)
	virtual void setLastFrameTM(const PxMat44& tm, uint32_t boneIndex = 0) = 0;
	virtual void setLastFrameTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex = 0) = 0;

	virtual void setSkinningMode(RenderMeshActorSkinningMode::Enum mode) = 0;
	virtual RenderMeshActorSkinningMode::Enum getSkinningMode() const = 0;
};

} // end namespace apex
} // end namespace nvidia

#endif // RENDER_MESH_ASSET_INTL_H
