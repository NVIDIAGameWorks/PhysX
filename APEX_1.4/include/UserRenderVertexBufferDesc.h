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



#ifndef USER_RENDER_VERTEX_BUFFER_DESC_H
#define USER_RENDER_VERTEX_BUFFER_DESC_H

/*!
\file
\brief class UserRenderVertexBufferDesc, structs RenderDataFormat and RenderVertexSemantic
*/

#include "ApexUsingNamespace.h"
#include "UserRenderResourceManager.h"
#include "RenderDataFormat.h"
#include "ApexSDK.h"

namespace physx
{
	class PxCudaContextManager;
};

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Potential semantics of a vertex buffer
*/
struct RenderVertexSemantic
{
	/**
	\brief Enum of vertex buffer semantics types
	*/
	enum Enum
	{
		CUSTOM = -1,			//!< User-defined

		POSITION = 0,			//!< Position of vertex
		NORMAL,					//!< Normal at vertex
		TANGENT,				//!< Tangent at vertex
		BINORMAL,				//!< Binormal at vertex
		COLOR,					//!< Color at vertex
		TEXCOORD0,				//!< Texture coord 0 of vertex
		TEXCOORD1,				//!< Texture coord 1 of vertex
		TEXCOORD2,				//!< Texture coord 2 of vertex
		TEXCOORD3,				//!< Texture coord 3 of vertex
		BONE_INDEX,				//!< Bone index of vertex
		BONE_WEIGHT,			//!< Bone weight of vertex

		DISPLACEMENT_TEXCOORD,	//!< X Displacement map texture coord of vertex
		DISPLACEMENT_FLAGS,		//!< Displacement map flags of vertex

		NUM_SEMANTICS			//!< Count of standard semantics, not a valid semantic
	};
};

/**
\brief Texture UV direction convention
*/
struct TextureUVOrigin
{
	/**
	\brief Enum of texture UV directions types
	*/
	enum Enum
	{
		ORIGIN_TOP_LEFT = 0,		//!< Origin is at top left
		ORIGIN_TOP_RIGHT = 1,		//!< Origin is at top right
		ORIGIN_BOTTOM_LEFT = 2,		//!< Origin is at bottom left
		ORIGIN_BOTTOM_RIGHT = 3,	//!< Origin is at bottom right

		ORIGIN_UNKNOWN = 0xFFFFFFFF	//!< Unknown origin
	};
};


/**
\brief Describes the semantics and layout of a vertex buffer
*/
class UserRenderVertexBufferDesc
{
public:
	UserRenderVertexBufferDesc(void)
	{
		interopContext = 0;
		registerInCUDA = false;
		maxVerts = 0;
		hint     = RenderBufferHint::STATIC;
		for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
		{
			buffersRequest[i] = RenderDataFormat::UNSPECIFIED;
		}
		moduleIdentifier = 0;
		numCustomBuffers = 0;
		customBuffersIdents = NULL;
		customBuffersRequest = NULL;
		uvOrigin         = TextureUVOrigin::ORIGIN_TOP_LEFT;
		canBeShared      = true;
	}

	/**
	\brief Checks if dsta is correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;
		if (maxVerts == 0)
		{
			numFailed++;
		}
		if (!customBuffersRequest)
		{
			uint32_t vertexSize = 0;
			for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
			{
				vertexSize += RenderDataFormat::getFormatDataSize(buffersRequest[i]);
			}
			if (vertexSize == 0)
			{
				numFailed++;
			}
		}
		return (numFailed == 0);
	}

public:
	/**
	\brief The maximum number of vertices that APEX will store in this buffer.
	*/
	uint32_t					maxVerts;

	/**
	\brief A hint about the update frequency of this buffer
	*/
	RenderBufferHint::Enum		hint;

	/**
	\brief Array of semantics with the corresponding format.

	RenderDataFormat::UNSPECIFIED is used for semantics that are disabled
	*/
	RenderDataFormat::Enum		buffersRequest[RenderVertexSemantic::NUM_SEMANTICS];

	/**
	\brief List of custom semantics that will be provided for this VB.
	*/

	/**
	\brief The number of custom VB semantics.
	*/
	uint32_t					numCustomBuffers;

	/**
	\brief Array of custom buffer identifiers
	*/
	void** 						customBuffersIdents;

	/**
	\brief Array of custom VB format types
	*/
	RenderDataFormat::Enum*		customBuffersRequest;

	/**
	\brief Identifier of module generating this request
	*/
	AuthObjTypeID				moduleIdentifier;

	/**
	\brief Texture UV direction convention.
	*/
	TextureUVOrigin::Enum		uvOrigin;

	/**
	\brief Buffer can be shared by multiple render resources
	*/
	bool						canBeShared;
	
	/**
	\brief Declare if the resource must be registered in CUDA upon creation
	*/
	bool						registerInCUDA;

	/**
	\brief The CUDA context

	This context must be used to register and unregister the resource every time the
	device is lost and recreated.
	*/
	PxCudaContextManager*		interopContext;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_VERTEX_BUFFER_DESC_H
