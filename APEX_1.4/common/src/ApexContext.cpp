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



#include "Apex.h"
#include "ApexContext.h"
#include "ApexActor.h"

namespace nvidia
{
namespace apex
{

//*** Apex Context ***
ApexContext::~ApexContext()
{
	if (mIterator)
	{
		mIterator->release();
	}
	removeAllActors();
	mActorArray.clear();
	mActorArrayCallBacks.clear();
}

uint32_t ApexContext::addActor(ApexActor& actor, ApexActor* actorPtr)
{
	mActorListLock.lockWriter();
	uint32_t index = mActorArray.size();
	mActorArray.pushBack(&actor);
	if (actorPtr != NULL)
	{
		mActorArrayCallBacks.pushBack(actorPtr);
	}
	callContextCreationCallbacks(&actor);
	mActorListLock.unlockWriter();
	return index;
}

void ApexContext::callContextCreationCallbacks(ApexActor* actorPtr)
{
	Asset*	assetPtr;
	AuthObjTypeID	ObjectTypeID;
	uint32_t	numCallBackActors;

	numCallBackActors = mActorArrayCallBacks.size();
	for (uint32_t i = 0; i < numCallBackActors; i++)
	{
		assetPtr = actorPtr->getAsset();
		if (assetPtr != NULL)
		{
			// get the resIds
			ObjectTypeID	= assetPtr->getObjTypeID();
			// call the call back function
			mActorArrayCallBacks[i]->ContextActorCreationNotification(ObjectTypeID,
			        actorPtr);
		}
	}
}

void ApexContext::callContextDeletionCallbacks(ApexActor* actorPtr)
{
	Asset*	assetPtr;
	AuthObjTypeID	ObjectTypeID;
	uint32_t			numCallBackActors;

	numCallBackActors = mActorArrayCallBacks.size();
	for (uint32_t i = 0; i < numCallBackActors; i++)
	{
		assetPtr = actorPtr->getAsset();
		if (assetPtr != NULL)
		{
			// get the resIds
			ObjectTypeID	= assetPtr->getObjTypeID();
			// call the call back function
			mActorArrayCallBacks[i]->ContextActorDeletionNotification(ObjectTypeID,
			        actorPtr);
		}
	}

}

void ApexContext::removeActorAtIndex(uint32_t index)
{
	ApexActor*	actorPtr;

	// call the callbacks so they know this actor is going to be deleted!
	callContextDeletionCallbacks(mActorArray[index]);

	mActorListLock.lockWriter();

	// remove the actor from the call back array if it is in it
	actorPtr = mActorArray[index];

	for (uint32_t i = 0; i < mActorArrayCallBacks.size(); i++)
	{
		if (actorPtr == mActorArrayCallBacks[i])			// is this the actor to be removed?
		{
			mActorArrayCallBacks.replaceWithLast(i);	// yes, remove it
		}
	}

	if (mIterator)
	{
		Renderable* renderable = mActorArray[ index ]->getRenderable();
		if (renderable)
		{
			mIterator->removeCachedActor(*(mActorArray[ index ]));
		}
	}
	mActorArray.replaceWithLast(index);
	if (index < mActorArray.size())
	{
		mActorArray[index]->updateIndex(*this, index);
	}
	mActorListLock.unlockWriter();
}

void ApexContext::renderLockAllActors()
{
	// Hold the render lock of all actors in the scene.  Used to protect PxScene::fetchResults()
	mActorListLock.lockReader();
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		mActorArray[i]->renderDataLock();
	}

	// NOTE: We are unlocking here now, and locking at the beginning of renderUnLockAllActors, below.
	// This is under the assumption that this lock is ONLY to protect a loop over mActorArray, not
	// anything between renderLockAllActors() and renderUnLockAllActors() in fetchResults().
	mActorListLock.unlockReader();
}

void ApexContext::renderUnLockAllActors()
{
	// NOTE: We are unlocking at the end of renderLockAllActors(), above, and unlocking here now.
	// This is under the assumption that this lock is ONLY to protect a loop over mActorArray, not
	// anything between renderLockAllActors() and renderUnLockAllActors() in fetchResults().
	mActorListLock.lockReader();

	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		mActorArray[i]->renderDataUnLock();
	}
	mActorListLock.unlockReader();
}

void ApexContext::removeAllActors()
{
	while (mActorArray.size())
	{
		mActorArray.back()->release();
	}
	mActorArrayCallBacks.clear();
}

RenderableIterator* ApexContext::createRenderableIterator()
{
	if (mIterator)
	{
		PX_ALWAYS_ASSERT(); // Only one per context at a time, please
		return NULL;
	}
	else
	{
		mIterator = PX_NEW(ApexRenderableIterator)(*this);
		return mIterator;
	}
}
void ApexContext::releaseRenderableIterator(RenderableIterator& iter)
{
	if (mIterator == DYNAMIC_CAST(ApexRenderableIterator*)(&iter))
	{
		mIterator->destroy();
		mIterator = NULL;
	}
	else
	{
		PX_ASSERT(mIterator == DYNAMIC_CAST(ApexRenderableIterator*)(&iter));
	}
}

ApexRenderableIterator::ApexRenderableIterator(ApexContext& _ctx) :
	ctx(&_ctx),
	curActor(0),
	mLockedActor(NULL)
{
	// Make copy of list of renderable actors currently in the context.
	// If an actor is later removed, we mark it as NULL in our cached
	// array.  If an actor is added, we do _NOT_ add it to our list since
	// it would be quite dangerous to call dispatchRenderResources() on an
	// actor that has never had updateRenderResources() called to it.

	mCachedActors.reserve(ctx->mActorArray.size());
	ctx->mActorListLock.lockWriter();
	for (uint32_t i = 0 ; i < ctx->mActorArray.size() ; i++)
	{
		Renderable* renderable = ctx->mActorArray[ i ]->getRenderable();
		if (renderable)
		{
			mCachedActors.pushBack(ctx->mActorArray[ i ]);
		}
	}
	ctx->mActorListLock.unlockWriter();
}

void ApexRenderableIterator::removeCachedActor(ApexActor& actor)
{
	// This function is called with a locked context, so we can modify our
	// internal lists at will.

	for (uint32_t i = 0 ; i < mCachedActors.size() ; i++)
	{
		if (&actor == mCachedActors[ i ])
		{
			mCachedActors[ i ] = NULL;
			break;
		}
	}
	for (uint32_t i = 0 ; i < mSkippedActors.size() ; i++)
	{
		if (&actor == mSkippedActors[ i ])
		{
			mSkippedActors.replaceWithLast(i);
			break;
		}
	}
	if (&actor == mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}
}

Renderable* ApexRenderableIterator::getFirst()
{
	curActor = 0;
	mSkippedActors.reserve(mCachedActors.size());
	return getNext();
}

Renderable*  ApexRenderableIterator::getNext()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}

	//physx::ScopedReadLock scopedContextLock( ctx->mActorListLock );
	ctx->mActorListLock.lockReader();
	while (curActor < mCachedActors.size())
	{
		ApexActor* actor = mCachedActors[ curActor++ ];
		if (actor)
		{
			if (actor->renderDataTryLock())
			{
				mLockedActor = actor;
				ctx->mActorListLock.unlockReader();
				return actor->getRenderable();
			}
			else
			{
				mSkippedActors.pushBack(actor);
			}
		}
	}
	if (mSkippedActors.size())
	{
		ApexActor* actor = mSkippedActors.back();
		mSkippedActors.popBack();
		ctx->mActorListLock.unlockReader();
		actor->renderDataLock();
		mLockedActor = actor;
		return actor->getRenderable();
	}
	ctx->mActorListLock.unlockReader();
	return NULL;
}

void ApexRenderableIterator::release()
{
	ctx->releaseRenderableIterator(*this);
}

void ApexRenderableIterator::reset()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}

	curActor = 0;

	mCachedActors.reserve(ctx->mActorArray.size());
	mCachedActors.reset();
	ctx->mActorListLock.lockWriter();
	for (uint32_t i = 0 ; i < ctx->mActorArray.size() ; i++)
	{
		Renderable* renderable = ctx->mActorArray[ i ]->getRenderable();
		if (renderable)
		{
			mCachedActors.pushBack(ctx->mActorArray[ i ]);
		}
	}
	ctx->mActorListLock.unlockWriter();
}

void ApexRenderableIterator::destroy()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}
	delete this;
}

}
} // end namespace nvidia::apex

