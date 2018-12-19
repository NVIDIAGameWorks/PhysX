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



#ifndef USER_RENDER_SURFACE_BUFFER_H
#define USER_RENDER_SURFACE_BUFFER_H

/*!
\file
\brief classes UserRenderSurfaceBuffer and RenderSurfaceBufferData
*/

#include "UserRenderSurfaceBufferDesc.h"

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
\brief surface buffer data
*/
class RenderSurfaceBufferData {};

/**
\brief Used for storing per-vertex data for rendering.
*/
class UserRenderSurfaceBuffer
{
public:
	virtual		~UserRenderSurfaceBuffer() {}

	/**
	\brief Called when APEX wants to update the contents of the surface buffer.

	The source data type is assumed to be the same as what was defined in the descriptor.

	\param [in] srcData				contains the source data for the surface buffer.
	\param [in] srcPitch			source data pitch (in bytes).
	\param [in] srcHeight			source data height.
	\param [in] dstX				first element to start writing in X-dimension.
	\param [in] dstY				first element to start writing in Y-dimension.
	\param [in] dstZ				first element to start writing in Z-dimension.
	\param [in] width				number of elements in X-dimension.
	\param [in] height				number of elements in Y-dimension.
	\param [in] depth				number of elements in Z-dimension.
	*/
	virtual void writeBuffer(const void* srcData, uint32_t srcPitch, uint32_t srcHeight, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth) = 0;


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

};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_SURFACE_BUFFER_H
