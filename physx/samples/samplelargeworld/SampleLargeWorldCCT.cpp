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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SampleLargeWorld.h"

#include "KinematicPlatform.h"
#include "SampleCCTActor.h"
#include "SampleCCTCameraController.h"
extern const char* gDynamic;

void SampleLargeWorld::onShapeHit(const PxControllerShapeHit& hit)
{
	defaultCCTInteraction(hit);
}

PxControllerBehaviorFlags SampleLargeWorld::getBehaviorFlags(const PxShape& shape, const PxActor& actor)
{
	return PxControllerBehaviorFlags(0);
}

PxControllerBehaviorFlags SampleLargeWorld::getBehaviorFlags(const PxController&)
{
	return PxControllerBehaviorFlags(0);
}

PxControllerBehaviorFlags SampleLargeWorld::getBehaviorFlags(const PxObstacle&)
{
	return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT|PxControllerBehaviorFlag::eCCT_SLIDE;
}

PxQueryHitType::Enum SampleLargeWorld::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
{
	PX_UNUSED(actor);
	const char* actorName = shape->getActor()->getName();
	if(actorName==gDynamic)
		return PxQueryHitType::eNONE;

	return PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum SampleLargeWorld::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
{
	return PxQueryHitType::eBLOCK;
}