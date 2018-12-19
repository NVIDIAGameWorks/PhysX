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



#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

/*!
\file
\brief class RenderContext
*/

#include "ApexUsingNamespace.h"
#include "foundation/PxMat44.h"

namespace nvidia
{
namespace apex
{

class UserRenderResource;
class UserOpaqueMesh;

PX_PUSH_PACK_DEFAULT

/**
\brief Describes the context of a renderable object
*/
class RenderContext
{
public:
	RenderContext(void)
	{
		renderResource = 0;
		isScreenSpace = false;
        renderMeshName = NULL;
	}

public:
	UserRenderResource*	renderResource;		//!< The renderable resource to be rendered
	bool				isScreenSpace;		//!< The data is in screenspace and should use a screenspace projection that transforms X -1 to +1 and Y -1 to +1 with zbuffer disabled.
	PxMat44				local2world;		//!< Reverse world pose transform for this renderable
	PxMat44				world2local;		//!< World pose transform for this renderable
    const char*         renderMeshName;     //!< The name of the render mesh this context is associated with.
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RENDER_CONTEXT_H
