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


#include "ScBodyCore.h"
#include "ScStaticCore.h"
#include "ScRigidSim.h"
#include "ScShapeSim.h"
#include "ScScene.h"
#include "ScPhysics.h"

using namespace physx;

Sc::RigidCore::RigidCore(const PxActorType::Enum type) 
: ActorCore(type, PxActorFlag::eVISUALIZATION, PX_DEFAULT_CLIENT, 0, 0)
{
}

Sc::RigidCore::~RigidCore()
{
}

void Sc::RigidCore::addShapeToScene(ShapeCore& shapeCore)
{
	Sc::RigidSim* sim = getSim();
	PX_ASSERT(sim);
	if(!sim)
		return;
	sim->getScene().addShape(*sim, shapeCore, NULL);
}

void Sc::RigidCore::removeShapeFromScene(ShapeCore& shapeCore, bool wakeOnLostTouch)
{
	Sc::RigidSim* sim = getSim();
	if(!sim)
		return;
	Sc::ShapeSim& s = sim->getSimForShape(shapeCore);
	sim->getScene().removeShape(s, wakeOnLostTouch);
}

void Sc::RigidCore::onShapeChange(Sc::ShapeCore& shape, ShapeChangeNotifyFlags notifyFlags, PxShapeFlags oldShapeFlags, bool forceBoundsUpdate)
{
	// DS: We pass flags to avoid searching multiple times or exposing RigidSim outside SC, and this form is
	// more convenient for the Scb::Shape::syncState method. If we start hitting this a lot we should do it
	// a different way, but shape modification after insertion is rare. 

	Sc::RigidSim* sim = getSim();
	if(!sim)
		return;
	Sc::ShapeSim& s = sim->getSimForShape(shape);

	if(notifyFlags & ShapeChangeNotifyFlag::eGEOMETRY)
		s.onVolumeOrTransformChange(forceBoundsUpdate);
	if(notifyFlags & ShapeChangeNotifyFlag::eMATERIAL)
		s.onMaterialChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eRESET_FILTERING)
		s.onResetFiltering();
	if(notifyFlags & ShapeChangeNotifyFlag::eSHAPE2BODY)
		s.onVolumeOrTransformChange(forceBoundsUpdate);
	if(notifyFlags & ShapeChangeNotifyFlag::eFILTERDATA)
		s.onFilterDataChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eFLAGS)
		s.onFlagChange(oldShapeFlags);
	if(notifyFlags & ShapeChangeNotifyFlag::eCONTACTOFFSET)
		s.onContactOffsetChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eRESTOFFSET)
		s.onRestOffsetChange();
}

Sc::RigidSim* Sc::RigidCore::getSim() const
{
	return static_cast<RigidSim*>(Sc::ActorCore::getSim());
}

PxActor* Sc::RigidCore::getPxActor() const
{
	return Ps::pointerOffset<PxActor*>(const_cast<Sc::RigidCore*>(this), Sc::gOffsetTable.scCore2PxActor[getActorCoreType()]);
}

