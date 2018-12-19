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


#ifndef PX_PHYSICS_SCP_PARTICLEBODYINTERACTION
#define PX_PHYSICS_SCP_PARTICLEBODYINTERACTION

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScElementSimInteraction.h"
#include "ScActorElementPair.h"
#include "ScParticlePacketShape.h"
#include "ScShapeSim.h"


#define PX_INVALID_PACKET_SHAPE_INDEX 0xffff


namespace physx
{
namespace Sc
{

	class ParticleElementRbElementInteraction : public ElementSimInteraction
	{
	public:

		// The ccdPass parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline is running.
		ParticleElementRbElementInteraction(ParticlePacketShape &particleShape, ShapeSim& rbShape, ActorElementPair& actorElementPair, const PxU32 ccdPass);
		virtual ~ParticleElementRbElementInteraction();
		PX_INLINE void* operator new(size_t s, void* memory);
		
		void destroy(bool isDyingRb, const PxU32 ccdPass);

		//---------- Interaction ----------
	protected:
		virtual bool onActivate(void*);
		virtual bool onDeactivate(PxU32 infoFlag);
		//-----------------------------------

	public:
		//----- ElementSimInteraction ------
		virtual bool isLastFilterInteraction() const { return (mActorElementPair.getRefCount() == 1); }
		//----------------------------------

		PX_INLINE ParticlePacketShape& getParticleShape() const;
		PX_INLINE ShapeSim& getRbShape() const;
		
		PX_INLINE void onRbShapeChange();

		PX_FORCE_INLINE ActorElementPair* getActorElementPair() const { return &mActorElementPair; }
		PX_FORCE_INLINE bool isDisabled() const { return (getActorElementPair()->isSuppressed() || isRbTrigger()); }

		PX_INLINE void setPacketShapeIndex(PxU16 idx);

		PX_INLINE void checkLowLevelActivationState();
	
	private:
		ParticleElementRbElementInteraction& operator=(const ParticleElementRbElementInteraction&);
		void activateForLowLevel(const PxU32 ccdPass);
		void deactivateForLowLevel(bool isDyingRb, const PxU32 ccdPass);
		static void operator delete(void*) {}

		PX_FORCE_INLINE PxU32 isRbTrigger() const { return (getRbShape().getFlags() & PxShapeFlag::eTRIGGER_SHAPE); }


				ActorElementPair&			mActorElementPair;
				PxU16						mPacketShapeIndex;
				bool						mIsActiveForLowLevel; 
	};

} // namespace Sc

PX_INLINE void* Sc::ParticleElementRbElementInteraction::operator new(size_t, void* memory)
{
	return memory;
}


PX_INLINE Sc::ParticlePacketShape& Sc::ParticleElementRbElementInteraction::getParticleShape() const
{
	PX_ASSERT(getElement0().getElementType() == ElementType::ePARTICLE_PACKET);
	return static_cast<ParticlePacketShape&>(getElement0());
}


PX_INLINE Sc::ShapeSim& Sc::ParticleElementRbElementInteraction::getRbShape() const
{
	PX_ASSERT(getElement1().getElementType() == ElementType::eSHAPE);
	PX_ASSERT(static_cast<ShapeSim&>(getElement1()).getActor().isDynamicRigid() || (static_cast<ShapeSim&>(getElement1()).getActor().getActorType() == PxActorType::eRIGID_STATIC));
	return static_cast<ShapeSim&>(getElement1());
}

PX_INLINE void Sc::ParticleElementRbElementInteraction::checkLowLevelActivationState()
{
	if (!isDisabled() && !mIsActiveForLowLevel)
	{
		// The interaction is now valid --> Create low level contact manager
		activateForLowLevel(false);
	}
	else if (isDisabled() && mIsActiveForLowLevel)
	{
		// The interaction is not valid anymore --> Release low level contact manager
		deactivateForLowLevel(false, false);
	}
}

PX_INLINE void Sc::ParticleElementRbElementInteraction::onRbShapeChange()
{
	getParticleShape().getParticleSystem().onRbShapeChange(getParticleShape(), getRbShape());
}

PX_INLINE void Sc::ParticleElementRbElementInteraction::setPacketShapeIndex(PxU16 idx) 
{ 
	PX_ASSERT(idx != PX_INVALID_PACKET_SHAPE_INDEX); 
	mPacketShapeIndex = idx;
}


}

#endif	// PX_USE_PARTICLE_SYSTEM_API

#endif
