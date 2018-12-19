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



#ifndef USER_RENDER_BONE_BUFFER_DESC_H
#define USER_RENDER_BONE_BUFFER_DESC_H

/*!
\file
\brief class UserRenderBoneBufferDesc, structs RenderDataFormat and RenderBoneSemantic
*/

#include "RenderDataFormat.h"
#include "UserRenderResourceManager.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief The semantics available for bone buffers
*/
struct RenderBoneSemantic
{
	/**
	\brief Enum of the semantics available for bone buffers
	*/
	enum Enum
	{
		POSE = 0,		//!< A matrix that transforms from object space into animated object space or even world space
		PREVIOUS_POSE,	//!< The corresponding poses from the last frame
		NUM_SEMANTICS	//!< Count of semantics, not a valid semantic.
	};
};



/**
\brief Descriptor to generate a bone buffer

This descriptor is filled out by APEX and helps as a guide how the
bone buffer should be generated.
*/
class UserRenderBoneBufferDesc
{
public:
	UserRenderBoneBufferDesc(void)
	{
		maxBones = 0;
		hint     = RenderBufferHint::STATIC;
		for (uint32_t i = 0; i < RenderBoneSemantic::NUM_SEMANTICS; i++)
		{
			buffersRequest[i] = RenderDataFormat::UNSPECIFIED;
		}
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;
		return (numFailed == 0);
	}

public:
	/**
	\brief The maximum amount of bones this buffer will ever hold.
	*/
	uint32_t				maxBones;

	/**
	\brief Hint on how often this buffer is updated.
	*/
	RenderBufferHint::Enum	hint;

	/**
	\brief Array of semantics with the corresponding format.

	RenderDataFormat::UNSPECIFIED is used for semantics that are disabled
	*/
	RenderDataFormat::Enum	buffersRequest[RenderBoneSemantic::NUM_SEMANTICS];
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
