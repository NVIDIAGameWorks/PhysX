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


#ifndef PX_RIGID_BODY_ACCESS_GPU_H
#define PX_RIGID_BODY_ACCESS_GPU_H

#include "foundation/PxTransform.h"
#include "foundation/PxStrideIterator.h"
#include "GuGeometryUnion.h"

namespace physx
{

class PxRigidBodyAccessGpu
{
public:

	struct ShapeProperties
	{
		PxTransform* ownerToWorld;
		PxTransform* shapeToOwner;
		Gu::GeometryUnion* geometry;
	};

	struct BodyProperties
	{
		PxStrideIterator<PxTransform>		currentTransforms;
		PxStrideIterator<PxTransform>		previousTransforms;
		PxStrideIterator<PxVec3>			linearVelocities;
		PxStrideIterator<PxVec3>			angularVelocities;
		PxStrideIterator<PxTransform>		body2ActorTransforms;
		PxStrideIterator<size_t>			cpuBodyHandle;
	};

	virtual void copyShapeProperties(ShapeProperties& shapeProperties, const size_t shape, const size_t body)	const = 0;
	virtual void copyBodyProperties(BodyProperties& bodyProperties, const size_t* bodies, PxU32 numBodies)		const = 0;
};

}

#endif // PX_RIGID_BODY_ACCESS_GPU_H
