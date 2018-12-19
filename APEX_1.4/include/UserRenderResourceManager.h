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



#ifndef USER_RENDER_RESOURCE_MANAGER_H
#define USER_RENDER_RESOURCE_MANAGER_H

/*!
\file
\brief class UserRenderResourceManager, structs RenderPrimitiveType, RenderBufferHint, and RenderCullMode
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class UserRenderVertexBuffer;
class UserRenderVertexBufferDesc;
class UserRenderIndexBuffer;
class UserRenderIndexBufferDesc;
class UserRenderSurfaceBuffer;
class UserRenderSurfaceBufferDesc;
class UserRenderBoneBuffer;
class UserRenderBoneBufferDesc;
class UserRenderInstanceBuffer;
class UserRenderInstanceBufferDesc;
class UserRenderSpriteBuffer;
class UserRenderSpriteBufferDesc;
class UserRenderResource;
class UserRenderResourceDesc;
class UserOpaqueMesh;
class UserOpaqueMeshDesc;

class UserRenderSpriteTextureDesc;

/**
\brief Describe the implied vertex ordering
*/
struct RenderPrimitiveType
{
	/**
	\brief Enum of the implied vertex ordering types
	*/
	enum Enum
	{
		UNKNOWN = 0,

		TRIANGLES,
		TRIANGLE_STRIP,

		LINES,
		LINE_STRIP,

		POINTS,
		POINT_SPRITES,
	};
};

/**
\brief Possible triangle culling modes
*/
struct RenderCullMode
{
	/**
	\brief Enum of possible triangle culling mode types
	*/
	enum Enum
	{
		CLOCKWISE = 0,
		COUNTER_CLOCKWISE,
		NONE
	};
};

/**
\brief Hint of the buffer data lifespan
*/
struct RenderBufferHint
{
	/**
	\brief Enum of hints of the buffer data lifespan
	*/
	enum Enum
	{
		STATIC = 0,
		DYNAMIC,
		STREAMING,
	};
};

/**
\brief User defined renderable resource manager

A render resource manager is an object that creates and manages renderable resources...
This is given to the APEX SDK at creation time via the descriptor and must be persistent through the lifetime
of the SDK.
*/
class UserRenderResourceManager
{
public:
	virtual								~UserRenderResourceManager() {}

	/**
		The create methods in this class will only be called from the context of an Renderable::updateRenderResources()
		call, but the release methods can be triggered by any APEX API call that deletes an Actor.  It is up to
		the end-user to make the release methods thread safe.
	*/

	virtual UserRenderVertexBuffer*		createVertexBuffer(const UserRenderVertexBufferDesc& desc)     = 0;
	/** \brief Release vertex buffer */
	virtual void                      	releaseVertexBuffer(UserRenderVertexBuffer& buffer)            = 0;

	/** \brief Create index buffer */
	virtual UserRenderIndexBuffer*    	createIndexBuffer(const UserRenderIndexBufferDesc& desc)       = 0;
	/** \brief Release index buffer */
	virtual void                      	releaseIndexBuffer(UserRenderIndexBuffer& buffer)              = 0;

	/** \brief Create bone buffer */
	virtual UserRenderBoneBuffer*     	createBoneBuffer(const UserRenderBoneBufferDesc& desc)         = 0;
	/** \brief Release bone buffer */
	virtual void                      	releaseBoneBuffer(UserRenderBoneBuffer& buffer)                = 0;

	/** \brief Create instance buffer */
	virtual UserRenderInstanceBuffer* 	createInstanceBuffer(const UserRenderInstanceBufferDesc& desc) = 0;
	/** \brief Release instance buffer */
	virtual void                        releaseInstanceBuffer(UserRenderInstanceBuffer& buffer)        = 0;

	/** \brief Create sprite buffer */
	virtual UserRenderSpriteBuffer*   	createSpriteBuffer(const UserRenderSpriteBufferDesc& desc)     = 0;
	/** \brief Release sprite buffer */
	virtual void                        releaseSpriteBuffer(UserRenderSpriteBuffer& buffer)            = 0;

	/** \brief Create surface buffer */
	virtual UserRenderSurfaceBuffer*  	createSurfaceBuffer(const UserRenderSurfaceBufferDesc& desc)   = 0;
	/** \brief Release surface buffer */
	virtual void                        releaseSurfaceBuffer(UserRenderSurfaceBuffer& buffer)          = 0;

	/** \brief Create resource */
	virtual UserRenderResource*       	createResource(const UserRenderResourceDesc& desc)             = 0;

	/**
	releaseResource() should not release any of the included buffer pointers.  Those free methods will be
	called separately by the APEX SDK before (or sometimes after) releasing the UserRenderResource.
	*/
	virtual void                        releaseResource(UserRenderResource& resource)                  = 0;

	/**
	Get the maximum number of bones supported by a given material. Return 0 for infinite.
	For optimal rendering, do not limit the bone count (return 0 from this function).
	*/
	virtual uint32_t                    getMaxBonesForMaterial(void* material)                         = 0;


	/** \brief Get the sprite layout data 
		Returns true in case textureDescArray is set.
		In case user is not interested in setting specific layout for sprite PS,
		this function should return false. 
	*/
	virtual bool 						getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, UserRenderSpriteBufferDesc* textureDescArray) = 0;

	/** \brief Get the instance layout data 
		Returns true in case textureDescArray is set.
		In case user is not interested in setting specific layout for sprite PS,
		this function should return false. 
	*/
	virtual bool 						getInstanceLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, UserRenderInstanceBufferDesc* instanceDescArray) = 0;

};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
