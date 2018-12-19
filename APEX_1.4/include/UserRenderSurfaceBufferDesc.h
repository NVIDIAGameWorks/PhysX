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



#ifndef USER_RENDER_SURFACE_BUFFER_DESC_H
#define USER_RENDER_SURFACE_BUFFER_DESC_H

/*!
\file
\brief class UserRenderSurfaceBufferDesc, structs RenderDataFormat and RenderSurfaceSemantic
*/

#include "ApexUsingNamespace.h"
#include "UserRenderResourceManager.h"
#include "RenderDataFormat.h"
#include "ApexSDK.h"

namespace physx
{
	class PxCudaContextManager;
}

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Describes the semantics and layout of a Surface buffer
*/
class UserRenderSurfaceBufferDesc
{
public:
	UserRenderSurfaceBufferDesc(void)
	{
		setDefaults();
	}

	/**
	\brief Default values
	*/
	void setDefaults()
	{
//		hint   = RenderBufferHint::STATIC;		
		format = RenderDataFormat::UNSPECIFIED;
		
		width = 0;
		height = 0;
		depth = 1;

		//moduleIdentifier = 0;
		
		registerInCUDA = false;
		interopContext = 0;
//		stride = 0;
	}

	/**
	\brief Checks if the surface buffer descriptor is valid
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;
		numFailed += (format == RenderDataFormat::UNSPECIFIED);
		numFailed += (width == 0) && (height == 0) && (depth == 0);
		numFailed += registerInCUDA && (interopContext == 0);
//		numFailed += registerInCUDA && (stride == 0);
		
		return (numFailed == 0);
	}

public:
	/**
	\brief The size of U-dimension.
	*/
	uint32_t				width;
	
	/**
	\brief The size of V-dimension.
	*/
	uint32_t				height;

	/**
	\brief The size of W-dimension.
	*/
	uint32_t				depth;

	/**
	\brief Data format of suface buffer.
	*/
	RenderDataFormat::Enum	format;

	bool					registerInCUDA;  //!< Declare if the resource must be registered in CUDA upon creation

	/**
	This context must be used to register and unregister the resource every time the
	device is lost and recreated.
	*/
	PxCudaContextManager*	interopContext;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_SURFACE_BUFFER_DESC_H
