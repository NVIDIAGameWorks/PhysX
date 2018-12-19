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


#include "ScScene.h"
#include "ScRigidSim.h"
#include "ScShapeSim.h"
#include "ScObjectIDTracker.h"
#include "ScShapeIterator.h"
#include "PsFoundation.h"

using namespace physx;

/*
	PT:

	The BP group ID comes from a Cm::IDPool, and RigidSim is the only class releasing the ID.

	The rigid tracker ID comes from a Cm::IDPool internal to an ObjectIDTracker, and RigidSim
	is the only class using it.

	Thus we should:
	- promote the BP group ID stuff to a "tracker" object
	- use the BP group ID as a rigid ID
*/

Sc::RigidSim::RigidSim(Scene& scene, RigidCore& core) :
	ActorSim(scene, core)
{
	mRigidId = scene.getRigidIDTracker().createID();
}

Sc::RigidSim::~RigidSim()
{
	Sc::Scene& scScene = getScene();
	scScene.getRigidIDTracker().releaseID(mRigidId);
}

bool notifyActorInteractionsOfTransformChange(Sc::ActorSim& actor);
void Sc::RigidSim::notifyShapesOfTransformChange()
{
	if(0)
	{
		for(ElementSim* e = getElements_(); e!=0; e = e->mNextInActor)
		{
			if(e->getElementType() == ElementType::eSHAPE)
				static_cast<Sc::ShapeSim*>(e)->onVolumeOrTransformChange(true);
		}
	}
	else
	{
		const bool isDynamic = notifyActorInteractionsOfTransformChange(*this);

		ElementSim* current = getElements_();
		while(current)
		{
			if(current->getElementType() == ElementType::eSHAPE)
			{
				ShapeSim* sim = static_cast<ShapeSim*>(current);
				sim->markBoundsForUpdate(false, isDynamic);
			}
			current = current->mNextInActor;
		}
	}
}

Sc::ShapeSim& Sc::RigidSim::getSimForShape(Sc::ShapeCore& core) const
{
	// DS: looks painful to traverse a linked list this way
	Sc::ShapeIterator iterator(*this);
	Sc::ShapeSim* sim;
	while((sim = iterator.getNext())!=NULL)
	{
		if(&sim->getCore() == &core)
			return *sim;
	}
	PX_ASSERT(0); // should never fail
	return *reinterpret_cast<Sc::ShapeSim*>(1);
}

