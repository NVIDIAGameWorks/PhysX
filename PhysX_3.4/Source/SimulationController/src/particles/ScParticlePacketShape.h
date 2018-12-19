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


#ifndef PX_PHYSICS_PARTICLESHAPE
#define PX_PHYSICS_PARTICLESHAPE

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScElementSim.h"
#include "ScParticleSystemSim.h"
#include "PtParticleShape.h"

namespace physx
{
namespace Sc
{
	class ParticleElementRbElementInteraction;


	/**
	A collision detection primitive for particle systems.
	*/
	class ParticlePacketShape : public ElementSim
	{
		public:
													ParticlePacketShape(ParticleSystemSim& particleSystem, PxU32 uid, Pt::ParticleShape* llParticleShape);
													~ParticlePacketShape();
		
		// ElementSim implementation
		virtual			void						getFilterInfo(PxFilterObjectAttributes& filterAttr, PxFilterData& filterData) const;
		// ~ElementSim

		virtual			void						createLowLevelVolume();
		virtual			void						destroyLowLevelVolume();

		public:
						void						setIndex(PxU32 index) { PX_ASSERT(index < ((1 << 16) - 1)); mIndex = static_cast<PxU16>(index); }
		PX_FORCE_INLINE	PxU16						getIndex()	const { return mIndex; }

		PX_FORCE_INLINE	PxBounds3					getBounds()	const	{ return mLLParticleShape->getBoundsV(); }

						class ParticleSystemSim&	getParticleSystem()	const;

						void						computeWorldBounds(PxBounds3&)	const;

		PX_FORCE_INLINE	ParticleElementRbElementInteraction**	getInteractions()		const { return mInteractions.begin();	}
		PX_FORCE_INLINE	PxU32									getInteractionsCount()	const { return mInteractions.size();	}

		PX_FORCE_INLINE	Pt::ParticleShape*			getLowLevelParticleShape() const { return mLLParticleShape; }

						void						setInteractionsDirty(InteractionDirtyFlag::Enum flag);

		PX_INLINE		PxU16						addPacketShapeInteraction(ParticleElementRbElementInteraction* interaction);
		PX_INLINE		void						removePacketShapeInteraction(PxU16 id);
		PX_INLINE		ParticleElementRbElementInteraction*	getPacketShapeInteraction(PxU16 id) const;

		private:
						void						reallocInteractions(ParticleElementRbElementInteraction**& mem, PxU16& capacity, PxU16 size, PxU16 requiredMinCapacity);


		static const PxU32 INLINE_INTERACTION_CAPACITY = 4;
						ParticleElementRbElementInteraction* mInlineInteractionMem[INLINE_INTERACTION_CAPACITY];

		Cm::OwnedArray<ParticleElementRbElementInteraction*, ParticlePacketShape, PxU16, &ParticlePacketShape::reallocInteractions>
													mInteractions;

						Pt::ParticleShape*			mLLParticleShape;		// Low level handle of particle packet
						PxU16						mIndex;
	};

} // namespace Sc


//These are called from interaction creation/destruction
PX_INLINE PxU16 Sc::ParticlePacketShape::addPacketShapeInteraction(ParticleElementRbElementInteraction* interaction)
{
	mInteractions.pushBack(interaction, *this);
	return PxU16(mInteractions.size()-1);
}

PX_INLINE void Sc::ParticlePacketShape::removePacketShapeInteraction(PxU16 id)
{
	mInteractions.replaceWithLast(id);
}

PX_INLINE Sc::ParticleElementRbElementInteraction* Sc::ParticlePacketShape::getPacketShapeInteraction(PxU16 id) const
{
	PX_ASSERT(id<mInteractions.size());
	return mInteractions[id];
}


}

#endif	// PX_USE_PARTICLE_SYSTEM_API

#endif
