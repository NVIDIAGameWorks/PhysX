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

#ifndef PT_BATCHER_H
#define PT_BATCHER_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "CmTask.h"

namespace physx
{

namespace Pt
{

class Batcher : public Ps::UserAllocated
{
  public:
	Batcher(class Context& _context);

	/**
	Issues shape update stages for a batch of particle systems.
	Ownership of Pt::ParticleShapeUpdateInput::shapes passed to callee!
	*/
	physx::PxBaseTask& scheduleShapeGeneration(class ParticleSystemSim** particleSystems,
	                                           struct ParticleShapesUpdateInput* inputs, PxU32 batchSize,
	                                           physx::PxBaseTask& continuation);

	/**
	Issues dynamics (SPH) update on CPUs.
	*/
	physx::PxBaseTask& scheduleDynamicsCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                       physx::PxBaseTask& continuation);

	/**
	Schedules collision prep work.
	*/
	physx::PxBaseTask& scheduleCollisionPrep(class ParticleSystemSim** particleSystems,
	                                         physx::PxLightCpuTask** inputPrepTasks, PxU32 batchSize,
	                                         physx::PxBaseTask& continuation);

	/**
	Schedules collision update stages for a batch of particle systems on CPU.
	Ownership of Pt::ParticleCollisionUpdateInput::contactManagerStream passed to callee!
	*/
	physx::PxBaseTask& scheduleCollisionCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                        physx::PxBaseTask& continuation);

	/**
	Schedule gpu pipeline.
	*/
	physx::PxBaseTask& schedulePipelineGpu(ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                       physx::PxBaseTask& continuation);

	Cm::FanoutTask shapeGenTask;
	Cm::FanoutTask dynamicsCpuTask;
	Cm::FanoutTask collPrepTask;
	Cm::FanoutTask collisionCpuTask;

	class Context& context;

  private:
	Batcher(const Batcher&);
	Batcher& operator=(const Batcher&);
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_BATCHER_H
