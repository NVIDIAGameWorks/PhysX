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

#ifndef PT_GPU_RIGID_BODY_ACCESS_H
#define PT_GPU_RIGID_BODY_ACCESS_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API
#if PX_SUPPORT_GPU_PHYSX

#include "PxRigidBodyAccessGpu.h"
#include "PsUserAllocated.h"

namespace physx
{

namespace Pt
{

class BodyTransformVault;

class RigidBodyAccessGpu : public Ps::UserAllocated, public PxRigidBodyAccessGpu
{
  public:
	virtual void copyShapeProperties(ShapeProperties& shapeProperties, const size_t shape, const size_t body) const;
	virtual void copyBodyProperties(BodyProperties& bodyProperties, const size_t* bodies, PxU32 numBodies) const;

  public:
	RigidBodyAccessGpu(const BodyTransformVault& transformVault) : mTransformVault(transformVault)
	{
	}
	virtual ~RigidBodyAccessGpu()
	{
	}

  private:
	RigidBodyAccessGpu& operator=(const RigidBodyAccessGpu&);
	const BodyTransformVault& mTransformVault;
};

} // namespace Pt
} // namespace physx

#endif // PX_SUPPORT_GPU_PHYSX
#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_GPU_RIGID_BODY_ACCESS_H
