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



#ifndef USER_RENDER_INSTANCE_BUFFER_H
#define USER_RENDER_INSTANCE_BUFFER_H

/*!
\file
\brief class UserRenderInstanceBuffer
*/

#include "RenderBufferData.h"
#include "UserRenderInstanceBufferDesc.h"

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
\brief The instance buffer type (deprecated)
*/
class PX_DEPRECATED RenderInstanceBufferData : public RenderBufferData<RenderInstanceSemantic, RenderInstanceSemantic::Enum> {};

/**
\brief Used for storing per-instance data for rendering.
*/
class UserRenderInstanceBuffer
{
public:
	virtual		~UserRenderInstanceBuffer() {}

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
	\brief Called when APEX wants to update the contents of the instance buffer.

	The source data type is assumed to be the same as what was defined in the descriptor.
	APEX should call this function and supply data for ALL semantics that were originally
	requested during creation every time its called.

	\param [in] data				Contains the source data for the instance buffer.
	\param [in] firstInstance		first instance to start writing to.
	\param [in] numInstances		number of instance to write.
	*/
	virtual void writeBuffer(const void* data, uint32_t firstInstance, uint32_t numInstances)
	{
		PX_UNUSED(data);
		PX_UNUSED(firstInstance);
		PX_UNUSED(numInstances);
	}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // USER_RENDER_INSTANCE_BUFFER_H
