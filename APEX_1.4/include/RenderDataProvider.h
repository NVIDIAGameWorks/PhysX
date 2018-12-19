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



#ifndef RENDER_DATA_PROVIDER_H
#define RENDER_DATA_PROVIDER_H

/*!
\file
\brief class RenderDataProvider
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief An actor instance that provides renderable data
*/
class RenderDataProvider
{
public:
	/**
	\brief Lock the renderable resources of this Renderable actor

	Locks the renderable data of this Renderable actor.  If the user uses an RenderableIterator
	to retrieve the list of Renderables, then locking is handled for them automatically by APEX.  If the
	user is storing Renderable pointers and using them ad-hoc, then they must use this API to lock the
	actor while updateRenderResources() and/or dispatchRenderResources() is called.  If an iterator is not being
	used, the user is also responsible for insuring the Renderable has not been deleted by another game
	thread.
	*/
	virtual void lockRenderResources() = 0;

	/**
	\brief Unlocks the renderable data of this Renderable actor.

	See locking semantics for RenderDataProvider::lockRenderResources().
	*/
	virtual void unlockRenderResources() = 0;

	/**
	\brief Update the renderable data of this Renderable actor.

	When called, this method will use the UserRenderResourceManager interface to inform the user
	about its render resource needs.  It will also call the writeBuffer() methods of various graphics
	buffers.  It must be called by the user each frame before any calls to dispatchRenderResources().
	If the actor is not being rendered, this function may also be skipped.

	\param [in] rewriteBuffers If true then static buffers will be rewritten (in the case of a graphics 
	device context being lost if managed buffers aren't being used)

	\param [in] userRenderData A pointer used by the application for context information which will be sent in
	the UserRenderResourceManager::createResource() method as a member of the UserRenderResourceDesc class.
	*/
	virtual void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0) = 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RENDER_DATA_PROVIDER_H
