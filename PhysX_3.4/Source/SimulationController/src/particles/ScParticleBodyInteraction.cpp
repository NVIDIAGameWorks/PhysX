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


#include "ScParticleBodyInteraction.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScParticleSystemSim.h"
#include "ScScene.h"
#include "PxsContext.h"
#include "ScParticleSystemCore.h"
#include "ScBodySim.h"
#include "ScNPhaseCore.h"

using namespace physx;


Sc::ParticleElementRbElementInteraction::ParticleElementRbElementInteraction(ParticlePacketShape &particleShape, ShapeSim& rbShape, ActorElementPair& actorElementPair, const PxU32 ccdPass) :
	ElementSimInteraction	(particleShape, rbShape, InteractionType::ePARTICLE_BODY, InteractionFlag::eFILTERABLE | InteractionFlag::eELEMENT_ELEMENT),
	mActorElementPair		(actorElementPair),
	mPacketShapeIndex		(PX_INVALID_PACKET_SHAPE_INDEX),
	mIsActiveForLowLevel	(false)
{
	registerInActors();

	getScene().getNPhaseCore()->registerInteraction(this);

	mPacketShapeIndex = getParticleShape().addPacketShapeInteraction(this);

	if (!isDisabled())
		activateForLowLevel(ccdPass);	// Collision with rigid body
}


Sc::ParticleElementRbElementInteraction::~ParticleElementRbElementInteraction()
{
	unregisterFromActors();
	getScene().getNPhaseCore()->unregisterInteraction(this);
}


void Sc::ParticleElementRbElementInteraction::destroy(bool isDyingRb, const PxU32 ccdPass)
{
	ParticlePacketShape& ps = getParticleShape();

	if (mIsActiveForLowLevel)
		deactivateForLowLevel(isDyingRb, ccdPass);

	const PxU16 idx = mPacketShapeIndex;
	ps.removePacketShapeInteraction(idx);
	if (idx < ps.getInteractionsCount())
		ps.getPacketShapeInteraction(idx)->setPacketShapeIndex(idx);
	mPacketShapeIndex = PX_INVALID_PACKET_SHAPE_INDEX;
}


bool Sc::ParticleElementRbElementInteraction::onActivate(void*)
{
	return false;
}


bool Sc::ParticleElementRbElementInteraction::onDeactivate(PxU32)
{
	return true;
}


void Sc::ParticleElementRbElementInteraction::activateForLowLevel(const PxU32 ccdPass)
{
	//update active cm count and update transform hash/mirroring
	getParticleShape().getParticleSystem().addInteraction(getParticleShape(), getRbShape(), ccdPass);
	mIsActiveForLowLevel = true;
}


void Sc::ParticleElementRbElementInteraction::deactivateForLowLevel(bool isDyingRb, const PxU32 ccdPass)
{
	//update active cm count and update transform hash/mirroring
	getParticleShape().getParticleSystem().removeInteraction(getParticleShape(), getRbShape(), isDyingRb, ccdPass);
	mIsActiveForLowLevel = false;
}



#endif	// PX_USE_PARTICLE_SYSTEM_API
