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


#include "ScClothShape.h"
#if PX_USE_CLOTH_API

#include "ScNPhaseCore.h"
#include "ScScene.h"

#include "ScClothSim.h"
#include "PxsContext.h"
#include "BpSimpleAABBManager.h"
#include "ScSqBoundsManager.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::ClothShape::ClothShape(ClothSim& cloth) :
	ElementSim(cloth, ElementType::eCLOTH),
    mClothCore(cloth.getCore()),
	mHasCollision(mClothCore.getClothFlags() & PxClothFlag::eSCENE_COLLISION)
{
	if (mHasCollision)
		createLowLevelVolume();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::ClothShape::~ClothShape()
{
	if (isInBroadPhase())
		destroyLowLevelVolume();

	PX_ASSERT(!isInBroadPhase());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ClothShape::getFilterInfo(PxFilterObjectAttributes& filterAttr, PxFilterData& filterData) const
{
	filterAttr = 0;
	ElementSim::setFilterObjectAttributeType(filterAttr, PxFilterObjectType::eCLOTH);
	filterData = mClothCore.getSimulationFilterData();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ClothShape::updateBoundsInAABBMgr()
{
	if(~mClothCore.getClothFlags() & PxClothFlag::eSCENE_COLLISION)
	{
		if(mHasCollision)
		{
			destroyLowLevelVolume();
			mHasCollision = false;
		}
		return;
	}

	if(!mHasCollision)
	{
		createLowLevelVolume();
		mHasCollision = true;
	}

	Scene& scene = getScene();

	PxBounds3 worldBounds = mClothCore.getWorldBounds();
	worldBounds.fattenSafe(mClothCore.getContactOffset()); // fatten for fast moving colliders
    scene.getBoundsArray().setBounds(worldBounds, getElementID());
	scene.getAABBManager()->getChangedAABBMgActorHandleMap().growAndSet(getElementID());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ClothShape::createLowLevelVolume()
{
	PX_ASSERT(getWorldBounds().isFinite());
	getScene().getBoundsArray().setBounds(getWorldBounds(), getElementID());	
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	const PxU32 group = Bp::FilterGroup::eCLOTH_NO_PARTICLE_INTERACTION;
	const PxU32 type = Bp::FilterType::DYNAMIC;
	addToAABBMgr(0, Bp::FilterGroup::Enum((group<<2)|type), false);
#else
	addToAABBMgr(0, Bp::FilterGroup::eCLOTH_NO_PARTICLE_INTERACTION, false);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Sc::ClothShape::destroyLowLevelVolume()
{
	if (!isInBroadPhase())
		return;

	Sc::Scene& scene = getScene();
	PxsContactManagerOutputIterator outputs = scene.getLowLevelContext()->getNphaseImplementationContext()->getContactManagerOutputs();
	scene.getNPhaseCore()->onVolumeRemoved(this, 0, outputs, scene.getPublicFlags() & PxSceneFlag::eADAPTIVE_FORCE);
	removeFromAABBMgr();
}


#endif	// PX_USE_CLOTH_API
