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

#include "gpu/PtRigidBodyAccessGpu.h"
#if PX_USE_PARTICLE_SYSTEM_API
#if PX_SUPPORT_GPU_PHYSX

#include "PxvGeometry.h"
#include "PxvDynamics.h"
#include "PtBodyTransformVault.h"

using namespace physx;
using namespace Pt;

void RigidBodyAccessGpu::copyShapeProperties(ShapeProperties& shapeProperties, const size_t shape, const size_t body) const
{
	const PxsShapeCore* shapeCore = reinterpret_cast<const PxsShapeCore*>(shape);
	*shapeProperties.geometry = shapeCore->geometry;

	const PxsRigidCore* rigidCore = reinterpret_cast<const PxsRigidCore*>(body);
	*shapeProperties.ownerToWorld = rigidCore->body2World;
	*shapeProperties.shapeToOwner = shapeCore->transform;
}

void RigidBodyAccessGpu::copyBodyProperties(BodyProperties& bodyProperties, const size_t* bodies, PxU32 numBodies) const
{
	const PxsBodyCore* const* bodyIt = reinterpret_cast<const PxsBodyCore* const*>(bodies);
	PxStrideIterator<PxTransform> currentTransformIt(bodyProperties.currentTransforms);
	PxStrideIterator<PxTransform> previousTransformIt(bodyProperties.previousTransforms);
	PxStrideIterator<PxVec3> linearVelocityIt(bodyProperties.linearVelocities);
	PxStrideIterator<PxVec3> angularVelocityIt(bodyProperties.angularVelocities);
	PxStrideIterator<PxTransform> body2ActorTransformIt(bodyProperties.body2ActorTransforms);
	PxStrideIterator<size_t> bodyHandleIt(bodyProperties.cpuBodyHandle);

	for(PxU32 i = 0; i < numBodies; ++i)
	{
		const PxsBodyCore& body = **bodyIt;
		*currentTransformIt = body.body2World;
		const PxTransform* preTransform = mTransformVault.getTransform(body);
		if(preTransform)
		{
			*previousTransformIt = *preTransform;
			*linearVelocityIt = body.linearVelocity;
			*angularVelocityIt = body.angularVelocity;
			*body2ActorTransformIt = body.getBody2Actor();
			*bodyHandleIt = (size_t) * bodyIt;
		}
		else
		{
			PX_ASSERT(0);
			*previousTransformIt = PxTransform(PxIdentity);
			*linearVelocityIt = PxVec3(0.f);
			*angularVelocityIt = PxVec3(0.f);
			*body2ActorTransformIt = PxTransform(PxIdentity);
			*bodyHandleIt = 0;
		}

		++bodyIt;
		++currentTransformIt;
		++previousTransformIt;
		++linearVelocityIt;
		++angularVelocityIt;
		++body2ActorTransformIt;
		++bodyHandleIt;
	}
}

#endif // PX_SUPPORT_GPU_PHYSX
#endif // PX_USE_PARTICLE_SYSTEM_API
