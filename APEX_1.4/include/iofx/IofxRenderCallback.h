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



#ifndef IOFX_RENDER_CALLBACK_H
#define IOFX_RENDER_CALLBACK_H

#include "foundation/Px.h"
#include "UserRenderCallback.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class IofxRenderable;
class IofxSpriteRenderable;
class IofxMeshRenderable;

/**
\brief Enumerates the potential IOFX render semantics
*/
struct IofxRenderSemantic
{
	/**
	\brief Enum of IOFX render semantics
	*/
	enum Enum
	{
		POSITION = 0,	//!< Position of particle
		COLOR,			//!< Color of particle
		VELOCITY,		//!< Linear velocity of particle
		SCALE,			//!< Scale of particle
		LIFE_REMAIN,	//!< 1.0 (new) .. 0.0 (dead)
		DENSITY,		//!< Particle density
		SUBTEXTURE,		//!< Sub-texture index of particle
		ORIENTATION,	//!< 2D particle orientation (angle in radians, CCW in screen plane)
		ROTATION,		//!< 3D particle rotation
		USER_DATA,		//!< User data - 32 bits (passed from Emitter)

		NUM_SEMANTICS	//!< Count of semantics, not a valid semantic.
	};
};


/**
\brief Enumerates the potential IOFX mesh render layout elements
*/
struct IofxSpriteRenderLayoutElement
{
	/**
	\brief Enum of IOFX sprite render layout elements
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

		MAX_COUNT
	};

	/**
	\brief Get layout element format
	*/
	static PX_INLINE RenderDataFormat::Enum getFormat(Enum element)
	{
		switch (element)
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
};

/**
\brief Enumerates the potential IOFX sprite render layout surface elements
*/
struct IofxSpriteRenderLayoutSurfaceElement
{
	/**
	\brief Enum of IOFX sprite render layout surface elements
	*/
	enum Enum
	{
		POSITION_FLOAT4, //!< float4(POSITION.x, POSITION.y, POSITION.z, 1)
		SCALE_ORIENT_SUBTEX_FLOAT4, //!< float4(SCALE.x, SCALE.y, ORIENTATION, SUBTEXTURE)
		COLOR_RGBA8, //<! COLOR in RGBA8 format
		COLOR_BGRA8, //<! COLOR in BGRA8 format
		COLOR_FLOAT4, //<! COLOR in FLOAT4 format

		MAX_COUNT
	};

	/**
	\brief Get layout element format
	*/
	static PX_INLINE RenderDataFormat::Enum getFormat(Enum element)
	{
		switch (element)
		{
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
\brief Describes the layout for sprite rendering
*/
struct IofxSpriteRenderLayout
{
	IofxSpriteRenderLayout(void)
	{
		setDefaults();
	}

	/**
	\brief Reset to default values
	*/
	void setDefaults()
	{
		bufferDesc.setDefaults();
		for (uint32_t i = 0; i < IofxSpriteRenderLayoutElement::MAX_COUNT; i++)
		{
			offsets[i] = uint32_t(-1);
		}
		stride = 0;
		surfaceCount = 0;
		for (uint32_t i = 0; i < MAX_SURFACE_COUNT; i++)
		{
			surfaceElements[i] = IofxSpriteRenderLayoutSurfaceElement::MAX_COUNT;
		}
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;

		numFailed += (surfaceCount == 0) && !bufferDesc.isValid();
		numFailed += (surfaceCount == 0) && (stride == 0);
		numFailed += (surfaceCount == 0) && (offsets[IofxSpriteRenderLayoutElement::POSITION_FLOAT3] == uint32_t(-1));

		numFailed += ((stride & 0x03) != 0);
		for (uint32_t i = 0; i < IofxSpriteRenderLayoutElement::MAX_COUNT; i++)
		{
			if (offsets[i] != static_cast<uint32_t>(-1))
			{
				numFailed += (offsets[i] >= stride);
				numFailed += ((offsets[i] & 0x03) != 0);
			}
		}
		for (uint32_t i= 0; i < surfaceCount; ++i)
		{
			numFailed += (surfaceElements[i] >= IofxSpriteRenderLayoutSurfaceElement::MAX_COUNT);
			numFailed += !surfaceDescs[i].isValid();
			numFailed += (surfaceDescs[i].width == 0) || (surfaceDescs[i].height == 0);
			numFailed += (IofxSpriteRenderLayoutSurfaceElement::getFormat(surfaceElements[i]) != surfaceDescs[i].format);
		}

		return (numFailed == 0);
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const IofxSpriteRenderLayout& other) const
	{
		if (surfaceCount != other.surfaceCount) return false;
		if (surfaceCount == 0)
		{
			if (!bufferDesc.isTheSameAs(other.bufferDesc)) return false;
			if (stride != other.stride) return false;
			for (uint32_t i = 0; i < IofxSpriteRenderLayoutElement::MAX_COUNT; i++)
			{
				if (offsets[i] != other.offsets[i]) return false;
			}
		}
		else
		{
			for (uint32_t i = 0; i < surfaceCount; ++i)
			{
				if (surfaceElements[i] != other.surfaceElements[i]) return false;
				if (!surfaceDescs[i].isTheSameAs(other.surfaceDescs[i])) return false;
			}
		}
		return true;
	}

public:
	UserRenderBufferDesc		bufferDesc; //!< Render buffer desc.
	
	/**
	\brief Array of the corresponding offsets (in bytes) for each layout element.
	*/
	uint32_t					offsets[IofxSpriteRenderLayoutElement::MAX_COUNT];
	uint32_t					stride; //!< The stride between objects in render buffer.

	uint32_t					surfaceCount; //!< Number of render surfaces (if zero render buffer is used)

	/**
	\brief Max number of supported render surfaces
	*/
	static const uint32_t MAX_SURFACE_COUNT = 4;

	/**
	\brief Layout element for each render surface (should be valid for all indices < surfaceCount)
	*/
	IofxSpriteRenderLayoutSurfaceElement::Enum	surfaceElements[MAX_SURFACE_COUNT];
	/**
	\brief Description for each render surface (should be valid for all indices < surfaceCount)
	*/
	UserRenderSurfaceDesc							surfaceDescs[MAX_SURFACE_COUNT];
};


/**
\brief Enumerates the potential IOFX mesh render layout elements
*/
struct IofxMeshRenderLayoutElement
{
	/**
	\brief Enum of IOFX mesh render layout elements
	*/
	enum Enum
	{
		POSITION_FLOAT3,
		ROTATION_SCALE_FLOAT3x3,
		POSE_FLOAT3x4,
		VELOCITY_LIFE_FLOAT4,
		DENSITY_FLOAT1,
		COLOR_RGBA8,
		COLOR_BGRA8,
		COLOR_FLOAT4,
		USER_DATA_UINT1,

		MAX_COUNT
	};

	/**
	\brief Get layout element format
	*/
	static PX_INLINE RenderDataFormat::Enum getFormat(Enum element)
	{
		switch (element)
		{
		case POSITION_FLOAT3:
			return RenderDataFormat::FLOAT3;		
		case ROTATION_SCALE_FLOAT3x3:
			return RenderDataFormat::FLOAT3x3;
		case POSE_FLOAT3x4:
			return RenderDataFormat::FLOAT3x4;
		case VELOCITY_LIFE_FLOAT4:
			return RenderDataFormat::FLOAT4;
		case DENSITY_FLOAT1:
			return RenderDataFormat::FLOAT1;
		case COLOR_RGBA8:
			return RenderDataFormat::R8G8B8A8;
		case COLOR_BGRA8:
			return RenderDataFormat::B8G8R8A8;
		case COLOR_FLOAT4:
			return RenderDataFormat::FLOAT4;
		case USER_DATA_UINT1:
			return RenderDataFormat::UINT1;
		default:
			PX_ALWAYS_ASSERT();
			return RenderDataFormat::NUM_FORMATS;
		}
	}
};

/**
\brief Describes the layout for mesh rendering
*/
struct IofxMeshRenderLayout
{
	IofxMeshRenderLayout(void)
	{
		setDefaults();
	}

	/**
	\brief Reset to default values
	*/
	void setDefaults()
	{
		bufferDesc.setDefaults();
		for (uint32_t i = 0; i < IofxMeshRenderLayoutElement::MAX_COUNT; i++)
		{
			offsets[i] = uint32_t(-1);
		}
		stride = 0;
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;

		numFailed += !bufferDesc.isValid();
		numFailed += (stride == 0);
		numFailed += (offsets[IofxMeshRenderLayoutElement::POSITION_FLOAT3] == uint32_t(-1))
			&& (offsets[IofxMeshRenderLayoutElement::POSE_FLOAT3x4] == uint32_t(-1));

		numFailed += ((stride & 0x03) != 0);
		for (uint32_t i = 0; i < IofxMeshRenderLayoutElement::MAX_COUNT; i++)
		{
			if (offsets[i] != static_cast<uint32_t>(-1))
			{
				numFailed += (offsets[i] >= stride);
				numFailed += ((offsets[i] & 0x03) != 0);
			}
		}

		return (numFailed == 0);
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const IofxMeshRenderLayout& other) const
	{
		if (!bufferDesc.isTheSameAs(other.bufferDesc)) return false;
		if (stride != other.stride) return false;
		for (uint32_t i = 0; i < IofxMeshRenderLayoutElement::MAX_COUNT; i++)
		{
			if (offsets[i] != other.offsets[i]) return false;
		}
		return true;
	}

public:
	UserRenderBufferDesc		bufferDesc; //!< Render buffer desc.
	
	/**
	\brief Array of the corresponding offsets (in bytes) for each layout element.
	*/
	uint32_t					offsets[IofxMeshRenderLayoutElement::MAX_COUNT];
	uint32_t					stride; //!< The stride between objects in render buffer.
};


/**
\brief User defined callback for IOFX rendering
*/
class IofxRenderCallback : public UserRenderCallback
{
public:
	/**
	\brief Called just after a new IOFX renderable was created
	*/
	virtual void onCreatedIofxRenderable(IofxRenderable& ) {}

	/**
	\brief Called just after an IOFX renderable was updated
	*/
	virtual void onUpdatedIofxRenderable(IofxRenderable& ) {}

	/**
	\brief Called just before an IOFX renderable is going to be released
	*/
	virtual void onReleasingIofxRenderable(IofxRenderable& ) {}

	/** \brief Called by the IOFX module to query layout for sprite rendering.
		Should return true in case IofxSpriteRenderLayout is properly filled, 
		and then IOFX will pass this layout to createRender[Buffer|Surface] method(s) to create needed render resources.
		In case this method is not implemented and returns false the IOFX module will create a layout to hold all input semantics.
		Also in case this method is not implemented or/and createRender[Buffer|Surface] method is not implemented,
		then the IOFX module will create its own render buffer/surfaces which user can map to access rendering content.
	*/
	virtual bool getIofxSpriteRenderLayout(IofxSpriteRenderLayout& spriteRenderLayout, uint32_t spriteCount, uint32_t spriteSemanticsBitmap, RenderInteropFlags::Enum interopFlags)
	{
		PX_UNUSED(spriteRenderLayout);
		PX_UNUSED(spriteCount);
		PX_UNUSED(spriteSemanticsBitmap);
		PX_UNUSED(interopFlags);
		return false;
	}

	/** \brief Called by the IOFX module to query layout for mesh rendering.
		Should return true in case IofxMeshRenderLayout is properly filled, 
		and then IOFX will pass this layout to createRenderBuffer method to create needed render resources.
		In case this method is not implemented and returns false the IOFX module will create a layout to hold all input semantics.
		Also in case this method is not implemented or/and createRenderBuffer method is not implemented,
		then the IOFX module will create its own render buffer which user can map to access rendering content.
	*/
	virtual bool getIofxMeshRenderLayout(IofxMeshRenderLayout& meshRenderLayout, uint32_t meshCount, uint32_t meshSemanticsBitmap, RenderInteropFlags::Enum interopFlags)
	{
		PX_UNUSED(meshRenderLayout);
		PX_UNUSED(meshCount);
		PX_UNUSED(meshSemanticsBitmap);
		PX_UNUSED(interopFlags);
		return false;
	}

};

PX_POP_PACK

}
} // end namespace nvidia

#endif // IOFX_RENDER_CALLBACK_H
