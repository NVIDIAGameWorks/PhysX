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



#ifndef USER_RENDER_RESOURCE_H
#define USER_RENDER_RESOURCE_H

/*!
\file
\brief class UserRenderResource
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

class UserRenderVertexBuffer;
class UserRenderIndexBuffer;
class UserRenderBoneBuffer;
class UserRenderInstanceBuffer;
class UserRenderSpriteBuffer;


PX_PUSH_PACK_DEFAULT

/**
\brief An abstract interface to a renderable resource
*/
class UserRenderResource
{
public:
	virtual ~UserRenderResource() {}

	/** \brief Set vertex buffer range */
	virtual void setVertexBufferRange(uint32_t firstVertex, uint32_t numVerts) = 0;
	/** \brief Set index buffer range */
	virtual void setIndexBufferRange(uint32_t firstIndex, uint32_t numIndices) = 0;
	/** \brief Set bone buffer range */
	virtual void setBoneBufferRange(uint32_t firstBone, uint32_t numBones) = 0;
	/** \brief Set instance buffer range */
	virtual void setInstanceBufferRange(uint32_t firstInstance, uint32_t numInstances) = 0;
	/** \brief Set sprite buffer range */
	virtual void setSpriteBufferRange(uint32_t firstSprite, uint32_t numSprites) = 0;
	/** \brief Set sprite visible count */
	virtual void setSpriteVisibleCount(uint32_t visibleCount) { PX_UNUSED(visibleCount); }
	/** \brief Set material */
	virtual void setMaterial(void* material) = 0;

	/** \brief Get number of vertex buffers */
	virtual uint32_t getNbVertexBuffers() const = 0;
	/** \brief Get vertex buffer */
	virtual UserRenderVertexBuffer*	getVertexBuffer(uint32_t index) const = 0;
	/** \brief Get index buffer */
	virtual UserRenderIndexBuffer* getIndexBuffer() const = 0;
	/** \brief Get bone buffer */
	virtual UserRenderBoneBuffer* getBoneBuffer() const = 0;
	/** \brief Get instance buffer */
	virtual UserRenderInstanceBuffer* getInstanceBuffer() const = 0;
	/** \brief Get sprite buffer */
	virtual UserRenderSpriteBuffer*	getSpriteBuffer() const = 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
