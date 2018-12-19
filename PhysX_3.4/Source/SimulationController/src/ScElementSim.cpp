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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "PsFoundation.h"
#include "PxsContext.h"
#include "BpSimpleAABBManager.h"
#include "ScElementSim.h"
#include "ScElementSimInteraction.h"
#include "ScSqBoundsManager.h"
#include "ScSimStats.h"

using namespace physx;
using namespace Sc;

static PX_FORCE_INLINE bool interactionHasElement(const Interaction* it, const ElementSim* elem)
{
	if(it->readInteractionFlag(InteractionFlag::eELEMENT_ELEMENT))
	{
#if PX_USE_PARTICLE_SYSTEM_API
		PX_ASSERT(	(it->getType() == InteractionType::eMARKER) ||
					(it->getType() == InteractionType::eOVERLAP) ||
					(it->getType() == InteractionType::eTRIGGER) ||
					(it->getType() == InteractionType::ePARTICLE_BODY) );
#else
		PX_ASSERT(	(it->getType() == InteractionType::eMARKER) ||
					(it->getType() == InteractionType::eOVERLAP) ||
					(it->getType() == InteractionType::eTRIGGER) );
#endif
		const ElementSimInteraction* ei = static_cast<const ElementSimInteraction*>(it);
		if((&ei->getElement0() == elem) || (&ei->getElement1() == elem))
			return true;
	}
	return false;
}

Sc::ElementSimInteraction* Sc::ElementSim::ElementInteractionIterator::getNext()
{
	while(mInteractions!=mInteractionsLast)
	{
		Interaction* it = *mInteractions++;
		if(interactionHasElement(it, mElement))
			return static_cast<ElementSimInteraction*>(it);
	}
	return NULL;
}

Sc::ElementSimInteraction* Sc::ElementSim::ElementInteractionReverseIterator::getNext()
{
	while(mInteractions!=mInteractionsLast)
	{
		Interaction* it = *--mInteractionsLast;
		if(interactionHasElement(it, mElement))
			return static_cast<ElementSimInteraction*>(it);
	}
	return NULL;
}

Sc::ElementSim::ElementSim(ActorSim& actor, ElementType::Enum type) :
	mNextInActor	(NULL),
	mActor			(actor),
	mType			(Ps::to8(type)),
	mInBroadPhase	(false)
{
	initID();

	PX_ASSERT((type & 0x03) == type);	// we use 2 bits to store

	actor.onElementAttach(*this);
}

Sc::ElementSim::~ElementSim()
{
	PX_ASSERT(!mInBroadPhase);
	releaseID();
	mActor.onElementDetach(*this);
}

void Sc::ElementSim::setElementInteractionsDirty(InteractionDirtyFlag::Enum flag, PxU8 interactionFlag)
{
	ElementSim::ElementInteractionIterator iter = getElemInteractions();
	ElementSimInteraction* interaction = iter.getNext();
	while(interaction)
	{
		if (interaction->readInteractionFlag(interactionFlag))
		{
			static_cast<ElementSimInteraction*>(interaction)->setDirty(flag);
		}

		interaction = iter.getNext();
	}
}

// we pun these constants in order to increment the stats when adding and removing from BP
PX_COMPILE_TIME_ASSERT(PxU32(PxSimulationStatistics::eRIGID_BODY) == PxU32(Sc::ElementType::eSHAPE));
#if PX_USE_CLOTH_API
PX_COMPILE_TIME_ASSERT(PxU32(PxSimulationStatistics::eCLOTH) == PxU32(Sc::ElementType::eCLOTH));
#endif

#if PX_USE_PARTICLE_SYSTEM_API
PX_COMPILE_TIME_ASSERT(PxU32(PxSimulationStatistics::ePARTICLE_SYSTEM) == PxU32(Sc::ElementType::ePARTICLE_PACKET));
#endif

void Sc::ElementSim::addToAABBMgr(PxReal contactDistance, Bp::FilterGroup::Enum group, Ps::IntBool isTrigger)
{
	Sc::Scene& scene = getScene();
	bool success = scene.getAABBManager()->addBounds(mElementID, contactDistance, group, this, mActor.getActorCore().getAggregateID(), isTrigger ? PxU8(Sc::ElementType::eTRIGGER) : PxU8(mType) );
	if (!success)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Unable to create broadphase entity because only 32768 shapes are supported");
		return;
	}
	mInBroadPhase = true;
#if PX_ENABLE_SIM_STATS
	scene.getStatsInternal().incBroadphaseAdds(PxSimulationStatistics::VolumeType(getElementType()));
#endif
}

void Sc::ElementSim::removeFromAABBMgr()
{
	PX_ASSERT(mInBroadPhase);
	Sc::Scene& scene = getScene();
	scene.getAABBManager()->removeBounds(mElementID);
	scene.getAABBManager()->getChangedAABBMgActorHandleMap().growAndReset(mElementID);

	mInBroadPhase = false;
#if PX_ENABLE_SIM_STATS
	scene.getStatsInternal().incBroadphaseRemoves(PxSimulationStatistics::VolumeType(getElementType()));
#endif
}
