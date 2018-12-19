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


#ifndef PX_PHYSICS_CLIENT
#define PX_PHYSICS_CLIENT

#include "PxScene.h"
#include "CmPhysXCommon.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace Sc
{

	class Client : public Ps::UserAllocated
	{
	public:
		Client() :
			activeTransforms		(PX_DEBUG_EXP("clientActiveTransforms")),
			activeActors			(PX_DEBUG_EXP("clientActiveActors")),
			posePreviewBodies		(PX_DEBUG_EXP("clientPosePreviewBodies")),
			posePreviewBuffer		(PX_DEBUG_EXP("clientPosePreviewBuffer")),
			behaviorFlags			(0),
			simulationEventCallback	(NULL),
			broadPhaseCallback		(NULL)
		{}

		Ps::Array<PxActiveTransform>	activeTransforms;
		Ps::Array<PxActor*>				activeActors;
		Ps::Array<const PxRigidBody*>	posePreviewBodies;	// buffer for bodies that requested early report of the integrated pose (eENABLE_POSE_INTEGRATION_PREVIEW).
															// This buffer gets exposed to users. Is officially accessible from PxSimulationEventCallback::onAdvance()
															// until the next simulate()/advance().
		Ps::Array<PxTransform>			posePreviewBuffer;	// buffer of newly integrated poses for the bodies that requested a preview. This buffer gets exposed
															// to users.
		PxClientBehaviorFlags			behaviorFlags;// Tracks behavior bits for clients.
		// User callbacks
		PxSimulationEventCallback*		simulationEventCallback;
		PxBroadPhaseCallback*			broadPhaseCallback;
	};

} // namespace Sc

}

#endif
