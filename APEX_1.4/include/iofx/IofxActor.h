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



#ifndef IOFX_ACTOR_H
#define IOFX_ACTOR_H

#include "Apex.h"
#include "IofxAsset.h"
#include "IofxRenderable.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief IOFX actor public interface
*/
class IofxActor : public Actor
{
public:
	///get the radius of the partice
	virtual float				getObjectRadius() const = 0;
	
	///get the number of particles
	virtual uint32_t			getObjectCount() const = 0;
	
	///get the number of visible particles (if sprite depth sorting is enabled, returns only number of particles in front of camera)
	virtual uint32_t			getVisibleCount() const = 0;
	
	///get the name of the IOS asset used to feed partices to the IOFX
	virtual const char*			getIosAssetName() const = 0;

	///returns AABB covering all objects in this actor, it's updated each frame during Scene::fetchResults().
	virtual physx::PxBounds3	getBounds() const = 0;
	
	/**
	\brief Acquire a pointer to the iofx's renderable proxy and increment its reference count.  
	
	The IofxRenderable will only be deleted when its reference count is zero.  
	Calls to IofxRenderable::release decrement the reference count, as does	a call to IofxActor::release().
	*/
	virtual IofxRenderable*		acquireRenderableReference() = 0;
};

PX_POP_PACK

}
} // namespace nvidia

#endif // IOFX_ACTOR_H
