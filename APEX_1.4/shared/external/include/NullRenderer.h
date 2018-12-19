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


#ifndef __NULL_RENDERER_H_
#define __NULL_RENDERER_H_

#include "Apex.h"

/* This class is intended for use by command line tools that require an
 * APEX SDK.  Apps which use this renderer should _NOT_ call
 * updateRenderResources() or dispatchRenderResources().  You _WILL_
 * crash.
 */

namespace nvidia
{
namespace apex
{

class NullRenderResourceManager : public UserRenderResourceManager
{
public:
	UserRenderVertexBuffer*	createVertexBuffer(const UserRenderVertexBufferDesc&)
	{
		return NULL;
	}
	UserRenderIndexBuffer*	createIndexBuffer(const UserRenderIndexBufferDesc&)
	{
		return NULL;
	}
	UserRenderBoneBuffer*		createBoneBuffer(const UserRenderBoneBufferDesc&)
	{
		return NULL;
	}
	UserRenderInstanceBuffer*	createInstanceBuffer(const UserRenderInstanceBufferDesc&)
	{
		return NULL;
	}
	UserRenderSpriteBuffer*   createSpriteBuffer(const UserRenderSpriteBufferDesc&)
	{
		return NULL;
	}
	UserRenderSurfaceBuffer*   createSurfaceBuffer(const UserRenderSurfaceBufferDesc&)
	{
		return NULL;
	}
	UserRenderResource*		createResource(const UserRenderResourceDesc&)
	{
		return NULL;
	}
	void						releaseVertexBuffer(UserRenderVertexBuffer&) {}
	void						releaseIndexBuffer(UserRenderIndexBuffer&) {}
	void						releaseBoneBuffer(UserRenderBoneBuffer&) {}
	void						releaseInstanceBuffer(UserRenderInstanceBuffer&) {}
	void						releaseSpriteBuffer(UserRenderSpriteBuffer&) {}
	void						releaseSurfaceBuffer(UserRenderSurfaceBuffer&) {}
	void						releaseResource(UserRenderResource&) {}
	uint32_t						getMaxBonesForMaterial(void*)
	{
		return 0;
	}

	/** \brief Get the sprite layout data */
	virtual bool getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, nvidia::apex::UserRenderSpriteBufferDesc* textureDescArray) 
	{
		PX_UNUSED(spriteCount);
		PX_UNUSED(spriteSemanticsBitmap);
		PX_UNUSED(textureDescArray);
		return false;
	}

	/** \brief Get the instance layout data */
	virtual bool getInstanceLayoutData(uint32_t particleCount, uint32_t particleSemanticsBitmap, nvidia::apex::UserRenderInstanceBufferDesc* instanceDescArray) 
	{
		PX_UNUSED(particleCount);
		PX_UNUSED(particleSemanticsBitmap);
		PX_UNUSED(instanceDescArray);
		return false;
	}


};

}
} // end namespace nvidia::apex

#endif
