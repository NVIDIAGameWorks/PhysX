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



#ifndef USER_RENDER_SPRITE_BUFFER_DESC_H
#define USER_RENDER_SPRITE_BUFFER_DESC_H

/*!
\file
\brief class UserRenderSpriteBufferDesc, structs RenderDataFormat and RenderSpriteSemantic
*/

#include "ApexUsingNamespace.h"
#include "RenderDataFormat.h"
#include "UserRenderResourceManager.h"

namespace physx
{
	class PxCudaContextManager;
};

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#if !PX_PS4
	#pragma warning(push)
	#pragma warning(disable:4121)
#endif	//!PX_PS4

/**
\brief potential semantics of a sprite buffer
*/
struct RenderSpriteSemantic
{
	/**
	\brief Enum of sprite buffer semantics types
	*/
	enum Enum
	{
		POSITION = 0,	//!< Position of sprite
		COLOR,			//!< Color of sprite
		VELOCITY,		//!< Linear velocity of sprite
		SCALE,			//!< Scale of sprite
		LIFE_REMAIN,	//!< 1.0 (new) .. 0.0 (dead)
		DENSITY,		//!< Particle density at sprite location
		SUBTEXTURE,		//!< Sub-texture index of sprite
		ORIENTATION,	//!< 2D sprite orientation (angle in radians, CCW in screen plane)

		USER_DATA,		//!< User data - 32 bits (passed from Emitter)

		NUM_SEMANTICS	//!< Count of semantics, not a valid semantic.
	};
};

/**
\brief potential semantics of a sprite buffer
*/
struct RenderSpriteLayoutElement
{
	/**
	\brief Enum of sprite buffer semantics types
	*/
	enum Enum
	{
		POSITION_FLOAT3,
		COLOR_RGBA8,
		COLOR_BGRA8,
		COLOR_FLOAT4,
		VELOCITY_FLOAT3,
		SCALE_FLOAT2,
		LIFE_REMAIN_FLOAT1,
		DENSITY_FLOAT1,
		SUBTEXTURE_FLOAT1,
		ORIENTATION_FLOAT1,
		USER_DATA_UINT1,

		NUM_SEMANTICS
	};

	/**
	\brief Get semantic format
	*/
	static PX_INLINE RenderDataFormat::Enum getSemanticFormat(Enum semantic)
	{
		switch (semantic)
		{
		case POSITION_FLOAT3:
			return RenderDataFormat::FLOAT3;
		case COLOR_RGBA8:
			return RenderDataFormat::R8G8B8A8;
		case COLOR_BGRA8:
			return RenderDataFormat::B8G8R8A8;
		case COLOR_FLOAT4:
			return RenderDataFormat::FLOAT4;
		case VELOCITY_FLOAT3:
			return RenderDataFormat::FLOAT3;
		case SCALE_FLOAT2:
			return RenderDataFormat::FLOAT2;
		case LIFE_REMAIN_FLOAT1:
			return RenderDataFormat::FLOAT1;
		case DENSITY_FLOAT1:
			return RenderDataFormat::FLOAT1;
		case SUBTEXTURE_FLOAT1:
			return RenderDataFormat::FLOAT1;
		case ORIENTATION_FLOAT1:
			return RenderDataFormat::FLOAT1;
		case USER_DATA_UINT1:
			return RenderDataFormat::UINT1;
		default:
			PX_ALWAYS_ASSERT();
			return RenderDataFormat::NUM_FORMATS;
		}
	}
/**
	\brief Get semantic from layout element format
	*/
	static PX_INLINE RenderSpriteSemantic::Enum getSemantic(Enum semantic)
	{
		switch (semantic)
		{
		case POSITION_FLOAT3:
			return RenderSpriteSemantic::POSITION;
		case COLOR_RGBA8:
		case COLOR_BGRA8:
		case COLOR_FLOAT4:
			return RenderSpriteSemantic::COLOR;
		case VELOCITY_FLOAT3:
			return RenderSpriteSemantic::VELOCITY;
		case SCALE_FLOAT2:
			return RenderSpriteSemantic::SCALE;
		case LIFE_REMAIN_FLOAT1:
			return RenderSpriteSemantic::LIFE_REMAIN;
		case DENSITY_FLOAT1:
			return RenderSpriteSemantic::DENSITY;
		case SUBTEXTURE_FLOAT1:
			return RenderSpriteSemantic::SUBTEXTURE;
		case ORIENTATION_FLOAT1:
			return RenderSpriteSemantic::ORIENTATION;
		case USER_DATA_UINT1:
			return RenderSpriteSemantic::USER_DATA;
		default:
			PX_ALWAYS_ASSERT();
			return RenderSpriteSemantic::NUM_SEMANTICS;
		}
	}
};

/**
\brief Struct for sprite texture layout info
*/
struct RenderSpriteTextureLayout
{
	/**
	\brief Enum of sprite texture layout info
	*/
	enum Enum
	{
		NONE = 0,
		POSITION_FLOAT4, //float4(POSITION.x, POSITION.y, POSITION.z, 1)
		SCALE_ORIENT_SUBTEX_FLOAT4, //float4(SCALE.x, SCALE.y, ORIENTATION, SUBTEXTURE)
		COLOR_RGBA8,
		COLOR_BGRA8,
		COLOR_FLOAT4,
		NUM_LAYOUTS
	};

	/**
	\brief Get layout format
	*/
	static PX_INLINE RenderDataFormat::Enum getLayoutFormat(Enum layout)
	{
		switch (layout)
		{
		case NONE:
			return RenderDataFormat::UNSPECIFIED;
		case POSITION_FLOAT4:
			return RenderDataFormat::FLOAT4;
		case SCALE_ORIENT_SUBTEX_FLOAT4:
			return RenderDataFormat::FLOAT4;
		case COLOR_RGBA8:
			return RenderDataFormat::R8G8B8A8;
		case COLOR_BGRA8:
			return RenderDataFormat::B8G8R8A8;
		case COLOR_FLOAT4:
			return RenderDataFormat::R32G32B32A32_FLOAT;
		default:
			PX_ALWAYS_ASSERT();
			return RenderDataFormat::NUM_FORMATS;
		}
	}

};

/**
\brief Class for storing sprite texture render data
*/
class UserRenderSpriteTextureDesc
{
public:
	RenderSpriteTextureLayout::Enum layout;	//!< texture layout
	uint32_t width;							//!< texture width
	uint32_t height;						//!< texture height
	uint32_t pitchBytes;					//!< texture pitch bytes
	uint32_t arrayIndex;					//!< array index for array textures or cubemap face index
	uint32_t mipLevel;						//!< mipmap level

public:
	PX_INLINE UserRenderSpriteTextureDesc(void)
	{
		layout = RenderSpriteTextureLayout::NONE;
		width = 0;
		height = 0;
		pitchBytes = 0;

		arrayIndex = 0;
		mipLevel = 0;
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const UserRenderSpriteTextureDesc& other) const
	{
		if (layout != other.layout) return false;
		if (width != other.width) return false;
		if (height != other.height) return false;
		if (pitchBytes != other.pitchBytes) return false;
		if (arrayIndex != other.arrayIndex) return false;
		if (mipLevel != other.mipLevel) return false;
		return true;
	}
};

/**
\brief describes the semantics and layout of a sprite buffer
*/
class UserRenderSpriteBufferDesc
{
public:
	/**
	\brief Max number of sprite textures
	*/
	static const uint32_t MAX_SPRITE_TEXTURES = 4;

	UserRenderSpriteBufferDesc(void)
	{
		setDefaults();
	}

	/**
	\brief Default values
	*/
	void setDefaults()
	{
		maxSprites = 0;
		hint         = RenderBufferHint::STATIC;
		registerInCUDA = false;
		interopContext = 0;

		for (uint32_t i = 0; i < RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
		{
			semanticOffsets[i] = static_cast<uint32_t>(-1);
		}
		stride = 0;

		textureCount = 0;
	}

	/**
	\brief Checks if data is correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;

		numFailed += (maxSprites == 0);
		numFailed += (textureCount == 0) && (stride == 0);
		numFailed += (textureCount == 0) && (semanticOffsets[RenderSpriteLayoutElement::POSITION_FLOAT3] == uint32_t(-1));
		numFailed += registerInCUDA && (interopContext == 0);

		numFailed += ((stride & 0x03) != 0);
		for (uint32_t i = 0; i < RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
		{
			if (semanticOffsets[i] != static_cast<uint32_t>(-1))
			{
				numFailed += (semanticOffsets[i] >= stride);
				numFailed += ((semanticOffsets[i] & 0x03) != 0);
			}
		}

		return (numFailed == 0);
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const UserRenderSpriteBufferDesc& other) const
	{
		if (registerInCUDA != other.registerInCUDA) return false;
		if (maxSprites != other.maxSprites) return false;
		if (hint != other.hint) return false;
		if (textureCount != other.textureCount) return false;
		if (textureCount == 0)
		{
			if (stride != other.stride) return false;
			for (uint32_t i = 0; i < RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
			{
				if (semanticOffsets[i] != other.semanticOffsets[i]) return false;
			}
		}
		else
		{
			for (uint32_t i = 0; i < textureCount; i++)
			{
				if (textureDescs[i].isTheSameAs(other.textureDescs[i]) == false) return false;
			}
		}
		return true;
	}

public:
	uint32_t					maxSprites;		//!< The maximum number of sprites that APEX will store in this buffer
	RenderBufferHint::Enum		hint;			//!< A hint about the update frequency of this buffer

	/**
	\brief Array of the corresponding offsets (in bytes) for each semantic.
	*/
	uint32_t					semanticOffsets[RenderSpriteLayoutElement::NUM_SEMANTICS];

	uint32_t					stride;			//!< The stride between sprites of this buffer. Required when CUDA interop is used!

	bool						registerInCUDA;  //!< Declare if the resource must be registered in CUDA upon creation

	/**
	This context must be used to register and unregister the resource every time the
	device is lost and recreated.
	*/
	PxCudaContextManager* 		interopContext;

	uint32_t					textureCount;						//!< the number of textures
	UserRenderSpriteTextureDesc	textureDescs[MAX_SPRITE_TEXTURES];	//!< an array of texture descriptors
};

#if !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_SPRITE_BUFFER_DESC_H
