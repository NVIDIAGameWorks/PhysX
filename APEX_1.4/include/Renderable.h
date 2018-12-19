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



#ifndef RENDERABLE_H
#define RENDERABLE_H

/*!
\file
\brief class Renderable
*/

#include "RenderDataProvider.h"

namespace nvidia
{
namespace apex
{

class UserRenderer;

PX_PUSH_PACK_DEFAULT

/**
\brief Base class of any actor that can be rendered
 */
class Renderable : public RenderDataProvider
{
public:
	/**
	When called, this method will use the UserRenderer interface to render itself (if visible, etc)
	by calling renderer.renderResource( RenderContext& ) as many times as necessary.   See locking
	semantics for RenderDataProvider::lockRenderResources().
	*/
	virtual void dispatchRenderResources(UserRenderer& renderer) = 0;

	/**
	Returns AABB covering rendered data.  The actor's world bounds is updated each frame
	during Scene::fetchResults().  This function does not require the Renderable actor to be locked.
	*/
	virtual PxBounds3 getBounds() const = 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
