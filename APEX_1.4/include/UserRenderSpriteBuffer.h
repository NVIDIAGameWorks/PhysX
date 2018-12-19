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



#ifndef USER_RENDER_SPRITE_BUFFER_H
#define USER_RENDER_SPRITE_BUFFER_H

/*!
\file
\brief class UserRenderSpriteBuffer
*/

#include "RenderBufferData.h"
#include "UserRenderSpriteBufferDesc.h"

/**
\brief Cuda graphics resource
*/
typedef struct CUgraphicsResource_st* CUgraphicsResource;

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Sprite buffer data (deprecated)
*/
class PX_DEPRECATED RenderSpriteBufferData : public RenderBufferData<RenderSpriteSemantic, RenderSpriteSemantic::Enum>, public ModuleSpecificRenderBufferData
{
};

/**
\brief Used for storing per-sprite instance data for rendering.
*/
class UserRenderSpriteBuffer
{
public:
	virtual		~UserRenderSpriteBuffer() {}

	/**
	\brief Called when APEX wants to update the contents of the sprite buffer.

	The source data type is assumed to be the same as what was defined in the descriptor.
	APEX should call this function and supply data for ALL semantics that were originally
	requested during creation every time its called.

	\param [in] data				Contains the source data for the sprite buffer.
	\param [in] firstSprite			first sprite to start writing to.
	\param [in] numSprites			number of vertices to write.
	*/
	virtual void writeBuffer(const void* data, uint32_t firstSprite, uint32_t numSprites)
	{
		PX_UNUSED(data);
		PX_UNUSED(firstSprite);
		PX_UNUSED(numSprites);
	}

	///Get the low-level handle of the buffer resource
	///\return true if succeeded, false otherwise
	virtual bool getInteropResourceHandle(CUgraphicsResource& handle)
#if APEX_DEFAULT_NO_INTEROP_IMPLEMENTATION
	{
		PX_UNUSED(&handle);
		return false;
	}
#else
	= 0;
#endif

	/**
	\brief Get interop texture handle list
	*/
	virtual bool getInteropTextureHandleList(CUgraphicsResource* handleList)
	{
		PX_UNUSED(handleList);
		return false;
	}

	/**
	\brief Write data to the texture
	*/
	virtual void writeTexture(uint32_t textureId, uint32_t numSprites, const void* srcData, size_t srcSize)
	{
		PX_UNUSED(textureId);
		PX_UNUSED(numSprites);
		PX_UNUSED(srcData);
		PX_UNUSED(srcSize);

		PX_ALWAYS_ASSERT();
	}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_SPRITE_BUFFER_H
