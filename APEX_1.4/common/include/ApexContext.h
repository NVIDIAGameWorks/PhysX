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



#ifndef APEX_CONTEXT_H
#define APEX_CONTEXT_H

#include "ApexUsingNamespace.h"
#include "Context.h"
#include "PsMutex.h"
#include "PsArray.h"
#include "PsUserAllocated.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace apex
{

class ApexActor;
class ApexRenderableIterator;

class ApexContext
{
public:
	ApexContext() : mIterator(NULL) {}
	virtual ~ApexContext();

	virtual uint32_t	addActor(ApexActor& actor, ApexActor* actorPtr = NULL);
	virtual void	callContextCreationCallbacks(ApexActor* actorPtr);
	virtual void	callContextDeletionCallbacks(ApexActor* actorPtr);
	virtual void	removeActorAtIndex(uint32_t index);

	void			renderLockAllActors();
	void			renderUnLockAllActors();

	void			removeAllActors();
	RenderableIterator* createRenderableIterator();
	void			releaseRenderableIterator(RenderableIterator&);

protected:
	physx::Array<ApexActor*> mActorArray;
	physx::Array<ApexActor*> mActorArrayCallBacks;
	nvidia::ReadWriteLock	mActorListLock;
	ApexRenderableIterator* mIterator;

	friend class ApexRenderableIterator;
	friend class ApexActor;
};

class ApexRenderableIterator : public RenderableIterator, public ApexRWLockable, public UserAllocated
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	Renderable* getFirst();
	Renderable* getNext();
	void			  reset();
	void			  release();

protected:
	void			  destroy();
	ApexRenderableIterator(ApexContext&);
	virtual ~ApexRenderableIterator() {}
	void			  removeCachedActor(ApexActor&);

	ApexContext*	  ctx;
	uint32_t             curActor;
	ApexActor*        mLockedActor;
	physx::Array<ApexActor*> mCachedActors;
	physx::Array<ApexActor*> mSkippedActors;

	friend class ApexContext;
};

}
} // end namespace nvidia::apex

#endif // APEX_CONTEXT_H