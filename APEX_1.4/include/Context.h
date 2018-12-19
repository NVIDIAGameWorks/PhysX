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



#ifndef CONTEXT_H
#define CONTEXT_H

/*!
\file
\brief class Context
*/

#include "ApexInterface.h"

namespace nvidia
{
namespace apex
{

class Renderable;
class RenderableIterator;

PX_PUSH_PACK_DEFAULT

/**
\brief A container for Actors
*/
class Context
{
public:
	/**
	\brief Removes all actors from the context and releases them
	*/
	virtual void removeAllActors() = 0;

	/**
	\brief Create an iterator for all renderables in this context
	*/
	virtual RenderableIterator*	createRenderableIterator() = 0;

	/**
	\brief Release a renderable iterator

	Equivalent to calling the iterator's release method.
	*/
	virtual void releaseRenderableIterator(RenderableIterator&) = 0;

protected:
	virtual ~Context() {}
};

/**
\brief Iterate over all renderable Actors in an Context

An RenderableIterator is a lock-safe iterator over all renderable
Actors in an Context.  Actors which are locked are skipped in the initial
pass and deferred till the end.  The returned Renderable is locked by the
iterator and remains locked until you call getNext().

The RenderableIterator is also deletion safe.  If an actor is deleted
from the Context in another thread, the iterator will skip that actor.

An RenderableIterator should not be held for longer than a single simulation
step.  It should be allocated on demand and released after use.
*/
class RenderableIterator : public ApexInterface
{
public:
	/**
	\brief Return the first renderable in an Context
	*/
	virtual Renderable* getFirst() = 0;
	/**
	\brief Return the next unlocked renderable in an Context
	*/
	virtual Renderable* getNext() = 0;
	/**
	\brief Refresh the renderable actor list for this context

	This function is only necessary if you believe actors have been added or
	deleted since the iterator was created.
	*/
	virtual void reset() = 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // CONTEXT_H
