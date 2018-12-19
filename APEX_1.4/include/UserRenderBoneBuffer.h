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



#ifndef USER_RENDER_BONE_BUFFER_H
#define USER_RENDER_BONE_BUFFER_H

/*!
\file
\brief class UserRenderBoneBuffer
*/

#include "RenderBufferData.h"
#include "UserRenderBoneBufferDesc.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief type of the bone buffer data
*/
class RenderBoneBufferData : public RenderBufferData<RenderBoneSemantic, RenderBoneSemantic::Enum> {};

/**
\brief Used for storing skeletal bone information used during skinning.

Separated out into its own interface as we don't know how the user is going to implement this
it could be in another vertex buffer, a texture or just an array and skinned on CPU depending
on engine and hardware support.
*/
class UserRenderBoneBuffer
{
public:
	virtual		~UserRenderBoneBuffer() {}

	/**
	\brief Called when APEX wants to update the contents of the bone buffer.

	The source data type is assumed to be the same as what was defined in the descriptor.
	APEX should call this function and supply data for ALL semantics that were originally
	requested during creation every time its called.

	\param [in] data				Contains the source data for the bone buffer.
	\param [in] firstBone			first bone to start writing to.
	\param [in] numBones			number of bones to write.
	*/
	virtual void writeBuffer(const nvidia::RenderBoneBufferData& data, uint32_t firstBone, uint32_t numBones)	= 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
