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



#ifndef USER_RENDER_INDEX_BUFFER_DESC_H
#define USER_RENDER_INDEX_BUFFER_DESC_H

/*!
\file
\brief class UserRenderIndexBufferDesc, structs RenderDataFormat and UserRenderIndexBufferDesc
*/

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
\brief describes the semantics and layout of an index buffer
*/
class UserRenderIndexBufferDesc
{
public:
	UserRenderIndexBufferDesc(void)
	{
		registerInCUDA = false;
		interopContext = 0;
		maxIndices = 0;
		hint       = RenderBufferHint::STATIC;
		format     = RenderDataFormat::UNSPECIFIED;
		primitives = RenderPrimitiveType::TRIANGLES;
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;
		numFailed += (registerInCUDA && !interopContext) ? 1 : 0;
		return (numFailed == 0);
	}

public:

	/**
	\brief The maximum amount of indices this buffer will ever hold.
	*/
	uint32_t					maxIndices;

	/**
	\brief Hint on how often this buffer is updated
	*/
	RenderBufferHint::Enum		hint;

	/**
	\brief The format of this buffer (only one implied semantic)
	*/
	RenderDataFormat::Enum		format;

	/**
	\brief Rendering primitive type (triangle, line strip, etc)
	*/
	RenderPrimitiveType::Enum	primitives;

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

#if !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_INDEX_BUFFER_DESC_H
