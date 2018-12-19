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


#include "ScParticlePacketShape.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScParticleBodyInteraction.h"
#include "ScNPhaseCore.h"
#include "ScScene.h"
#include "PtParticleSystemSim.h"
#include "ScParticleSystemCore.h"
#include "ScSimStats.h"
#include "ScSqBoundsManager.h"
#include "ScScene.h"
#include "PxsContext.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::ParticlePacketShape::ParticlePacketShape(ParticleSystemSim& particleSystem, PxU32 index, Pt::ParticleShape* llParticleShape) :
	ElementSim(particleSystem, ElementType::ePARTICLE_PACKET),
	mLLParticleShape(llParticleShape)
{
	// Initialize LL shape.
	PX_ASSERT(mLLParticleShape);
	mLLParticleShape->setUserDataV(this);

	setIndex(index);

	// Add particle actor element to broadphase
	createLowLevelVolume();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::ParticlePacketShape::~ParticlePacketShape()
{
	getParticleSystem().unlinkParticleShape(this);		// Let the particle system remove this shape from the list.

	// Remove particle actor element from broadphase and cleanup interactions
	destroyLowLevelVolume();

	// Destroy LowLevel shape
	if (mLLParticleShape)
	{
		mLLParticleShape->destroyV();
		mLLParticleShape = 0;
	}

	PX_ASSERT(mInteractions.size()==0);
	mInteractions.releaseMem(*this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ParticlePacketShape::computeWorldBounds(PxBounds3& b) const
{
	 b = getBounds();
	 PX_ASSERT(b.isFinite());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ParticlePacketShape::getFilterInfo(PxFilterObjectAttributes& filterAttr, PxFilterData& filterData) const
{
	filterAttr = 0;
	if (getParticleSystem().getInternalFlags() & Pt::InternalParticleSystemFlag::eSPH)
		ElementSim::setFilterObjectAttributeType(filterAttr, PxFilterObjectType::ePARTICLE_FLUID);
	else
		ElementSim::setFilterObjectAttributeType(filterAttr, PxFilterObjectType::ePARTICLE_SYSTEM);

	filterData = getParticleSystem().getSimulationFilterData();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::ParticleSystemSim& Sc::ParticlePacketShape::getParticleSystem() const
{ 
	return static_cast<ParticleSystemSim&>(getActor()); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ParticlePacketShape::setInteractionsDirty(InteractionDirtyFlag::Enum flag)
{
	ParticleElementRbElementInteraction** interactions = getInteractions();
	PxU32 nbInteractions = getInteractionsCount();

	while(nbInteractions--)
	{
		ParticleElementRbElementInteraction* interaction = *interactions++;

		PX_ASSERT(interaction->readInteractionFlag(InteractionFlag::eFILTERABLE));
		PX_ASSERT(interaction->readInteractionFlag(InteractionFlag::eELEMENT_ELEMENT));

		interaction->setDirty(flag);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PT: TODO: refactor with Sc::ActorSim::reallocInteractions
void Sc::ParticlePacketShape::reallocInteractions(Sc::ParticleElementRbElementInteraction**& mem, PxU16& capacity, PxU16 size, PxU16 requiredMinCapacity)
{
	ParticleElementRbElementInteraction** newMem;
	PxU16 newCapacity;

	if(requiredMinCapacity==0)
	{
		newCapacity = 0;
		newMem = 0;
	}
	else if(requiredMinCapacity<=INLINE_INTERACTION_CAPACITY)
	{
		newCapacity = INLINE_INTERACTION_CAPACITY;
		newMem = mInlineInteractionMem;
	}
	else
	{
		const PxU32 desiredCapacity = Ps::nextPowerOfTwo(PxU32(requiredMinCapacity-1));
		PX_ASSERT(desiredCapacity<=65536);

		const PxU32 limit = 0xffff;
		newCapacity = Ps::to16(PxMin(limit, desiredCapacity));
		newMem = reinterpret_cast<ParticleElementRbElementInteraction**>(getScene().allocatePointerBlock(newCapacity));
	}

	PX_ASSERT(newCapacity >= requiredMinCapacity && requiredMinCapacity>=size);

	PxMemCopy(newMem, mem, size*sizeof(ParticleElementRbElementInteraction*));

	if(mem && mem!=mInlineInteractionMem)
		getScene().deallocatePointerBlock(reinterpret_cast<void**>(mem), capacity);
	
	capacity = newCapacity;
	mem = newMem;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ParticlePacketShape::createLowLevelVolume()
{
	PX_ASSERT(getBounds().isFinite());

	getScene().getBoundsArray().setBounds(getBounds(), getElementID());
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	const PxU32 group = Bp::FilterGroup::ePARTICLES;
	const PxU32 type = Bp::FilterType::DYNAMIC;
	addToAABBMgr(0, Bp::FilterGroup::Enum((group<<2)|type), false);
#else
	addToAABBMgr(0, Bp::FilterGroup::ePARTICLES, false);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ParticlePacketShape::destroyLowLevelVolume()
{
	if (!isInBroadPhase())
		return;

	Sc::Scene& scene = getScene();
	PxsContactManagerOutputIterator outputs = scene.getLowLevelContext()->getNphaseImplementationContext()->getContactManagerOutputs();

	scene.getNPhaseCore()->onVolumeRemoved(this, 0, outputs, scene.getPublicFlags() & PxSceneFlag::eADAPTIVE_FORCE);
	removeFromAABBMgr();
}

#endif	// PX_USE_PARTICLE_SYSTEM_API
