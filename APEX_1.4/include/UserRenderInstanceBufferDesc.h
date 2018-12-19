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



#ifndef USER_RENDER_INSTANCE_BUFFER_DESC_H
#define USER_RENDER_INSTANCE_BUFFER_DESC_H

/*!
\file
\brief class UserRenderInstanceBufferDesc, structs RenderDataFormat and RenderInstanceSemantic
*/

#include "UserRenderResourceManager.h"
#include "RenderDataFormat.h"

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
\brief Enumerates the potential instance buffer semantics
*/
struct RenderInstanceSemantic
{
	/**
	\brief Enume of the potential instance buffer semantics
	*/
	enum Enum
	{
		POSITION = 0,	//!< position of instance
		ROTATION_SCALE,	//!< rotation matrix and scale baked together
		VELOCITY_LIFE,	//!< velocity and life remain (1.0=new .. 0.0=dead) baked together 
		DENSITY,		//!< particle density at instance location
		COLOR,			//!< color of instance
		UV_OFFSET,		//!< an offset to apply to all U,V coordinates
		LOCAL_OFFSET,	//!< the static initial position offset of the instance

		USER_DATA,		//!< User data - 32 bits

		POSE,			//!< pose matrix

		NUM_SEMANTICS	//!< Count of semantics, not a valid semantic.
	};
};

/**
\brief potential semantics of a sprite buffer
*/
struct RenderInstanceLayoutElement
{
	/**
	\brief Enum of sprite buffer semantics types
	*/
	enum Enum
	{
		POSITION_FLOAT3,
		ROTATION_SCALE_FLOAT3x3,
		VELOCITY_LIFE_FLOAT4,
		DENSITY_FLOAT1,
		COLOR_RGBA8,
		COLOR_BGRA8,
		COLOR_FLOAT4,
		UV_OFFSET_FLOAT2,
		LOCAL_OFFSET_FLOAT3,
		USER_DATA_UINT1,
		POSE_FLOAT3x4,

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
		case ROTATION_SCALE_FLOAT3x3:
			return RenderDataFormat::FLOAT3x3;
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
		case UV_OFFSET_FLOAT2:
			return RenderDataFormat::FLOAT2;
		case LOCAL_OFFSET_FLOAT3:
			return RenderDataFormat::FLOAT3;
		case USER_DATA_UINT1:
			return RenderDataFormat::UINT1;
		case POSE_FLOAT3x4:
			return RenderDataFormat::FLOAT3x4;
		default:
			PX_ALWAYS_ASSERT();
			return RenderDataFormat::NUM_FORMATS;
		}
	}
/**
	\brief Get semantic from layout element format
	*/
	static PX_INLINE RenderInstanceSemantic::Enum getSemantic(Enum semantic)
	{
		switch (semantic)
		{
		case POSITION_FLOAT3:
			return RenderInstanceSemantic::POSITION;		
		case ROTATION_SCALE_FLOAT3x3:
			return RenderInstanceSemantic::ROTATION_SCALE;
		case VELOCITY_LIFE_FLOAT4:
			return RenderInstanceSemantic::VELOCITY_LIFE;
		case DENSITY_FLOAT1:
			return RenderInstanceSemantic::DENSITY;
		case COLOR_RGBA8:
		case COLOR_BGRA8:
		case COLOR_FLOAT4:
			return RenderInstanceSemantic::COLOR;
		case UV_OFFSET_FLOAT2:
			return RenderInstanceSemantic::UV_OFFSET;
		case LOCAL_OFFSET_FLOAT3:
			return RenderInstanceSemantic::LOCAL_OFFSET;
		case USER_DATA_UINT1:
			return RenderInstanceSemantic::USER_DATA;
		case POSE_FLOAT3x4:
			return RenderInstanceSemantic::POSE;
		default:
			PX_ALWAYS_ASSERT();
			return RenderInstanceSemantic::NUM_SEMANTICS;
		}
	}
};

/**
\brief Describes the data and layout of an instance buffer
*/
class UserRenderInstanceBufferDesc
{
public:
	UserRenderInstanceBufferDesc(void)
	{
		setDefaults();
	}

	/**
	\brief Default values
	*/
	void setDefaults()
	{
		registerInCUDA = false;
		interopContext = 0;
		maxInstances = 0;
		hint         = RenderBufferHint::STATIC;
		for (uint32_t i = 0; i < RenderInstanceLayoutElement::NUM_SEMANTICS; i++)
		{
			semanticOffsets[i] = uint32_t(-1);
		}
		stride = 0;
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;

		numFailed += (maxInstances == 0);
		numFailed += (stride == 0);
		numFailed += (semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3] == uint32_t(-1))
			&& (semanticOffsets[RenderInstanceLayoutElement::POSE_FLOAT3x4] == uint32_t(-1));
		numFailed += registerInCUDA && (interopContext == 0);

		numFailed += ((stride & 0x03) != 0);
		for (uint32_t i = 0; i < RenderInstanceLayoutElement::NUM_SEMANTICS; i++)
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
	bool isTheSameAs(const UserRenderInstanceBufferDesc& other) const
	{
		if (registerInCUDA != other.registerInCUDA) return false;
		if (maxInstances != other.maxInstances) return false;
		if (hint != other.hint) return false;

		if (stride != other.stride) return false;
		for (uint32_t i = 0; i < RenderInstanceLayoutElement::NUM_SEMANTICS; i++)
		{
			if (semanticOffsets[i] != other.semanticOffsets[i]) return false;
		}
		return true;
	}

public:
	uint32_t					maxInstances; //!< The maximum amount of instances this buffer will ever hold.
	RenderBufferHint::Enum		hint; //!< Hint on how often this buffer is updated.
	
	/**
	\brief Array of the corresponding offsets (in bytes) for each semantic.
	*/
	uint32_t					semanticOffsets[RenderInstanceLayoutElement::NUM_SEMANTICS];

	uint32_t					stride; //!< The stride between instances of this buffer. Required when CUDA interop is used!

	bool						registerInCUDA; //!< Declare if the resource must be registered in CUDA upon creation

	/**
	This context must be used to register and unregister the resource every time the
	device is lost and recreated.
	*/
	PxCudaContextManager*   	interopContext;
};

#if !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_INSTANCE_BUFFER_DESC_H
