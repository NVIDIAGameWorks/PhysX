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

#ifndef PT_CONTEXT_CPU_H
#define PT_CONTEXT_CPU_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "CmPool.h"
#include "PtContext.h"

namespace physx
{

class PxBaseTask;
class PxLightCpuTask;
class PxTaskManager;

namespace Pt
{

class Batcher;
class BodyTransformVault;
class ParticleShapeCpu;
class ParticleSystemSimCpu;
struct ParticleCell;

/**
Per scene manager class for particle systems.
*/
class ContextCpu : public Context, Ps::UserAllocated
{
	PX_NOCOPY(ContextCpu)
  public:
	/**
	Register particle functionality.
	Not calling this should allow the code to be stripped at link time.
	*/
	static void registerParticles();

	// Pt::Context implementation
	virtual void destroy();
	virtual ParticleSystemSim* addParticleSystem(class ParticleData* particleData,
	                                             const ParticleSystemParameter& parameter, bool useGpuSupport);
	virtual ParticleData* removeParticleSystem(ParticleSystemSim* system, bool acquireParticleData);
	virtual PxBaseTask& scheduleShapeGeneration(class ParticleSystemSim** particleSystems,
	                                            struct ParticleShapesUpdateInput* inputs, PxU32 batchSize,
	                                            PxBaseTask& continuation);
	virtual PxBaseTask& scheduleDynamicsCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                        PxBaseTask& continuation);
	virtual PxBaseTask& scheduleCollisionPrep(class ParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks,
	                                          PxU32 batchSize, PxBaseTask& continuation);
	virtual PxBaseTask& scheduleCollisionCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                         PxBaseTask& continuation);
	virtual PxBaseTask& schedulePipelineGpu(ParticleSystemSim** particleSystems, PxU32 batchSize,
	                                        PxBaseTask& continuation);
#if PX_SUPPORT_GPU_PHYSX
	virtual class PxSceneGpu* createOrGetSceneGpu();
#endif
	//~Pt::Context implementation

	ParticleShapeCpu* createParticleShape(ParticleSystemSimCpu* particleSystem, const ParticleCell* packet);
	void releaseParticleShape(ParticleShapeCpu* shape);

	Cm::FlushPool& getTaskPool()
	{
		return mTaskPool;
	}

  private:
	ContextCpu(physx::PxTaskManager* taskManager, Cm::FlushPool& taskPool);

	virtual ~ContextCpu();

	ParticleSystemSim* addParticleSystemImpl(ParticleData* particleData, const ParticleSystemParameter& parameter,
	                                         bool useGpuSupport);
	ParticleData* removeParticleSystemImpl(ParticleSystemSim* system, bool acquireParticleData);

	static Context* createContextImpl(physx::PxTaskManager* taskManager, Cm::FlushPool& taskPool);

	void destroyContextImpl();

	Cm::PoolList<ParticleSystemSimCpu, ContextCpu> mParticleSystemPool;
	Cm::PoolList<ParticleShapeCpu, ContextCpu> mParticleShapePool;
	Ps::Mutex mParticleShapePoolMutex;
	Batcher* mBatcher;

	physx::PxTaskManager* mTaskManager;
	Cm::FlushPool& mTaskPool;

#if PX_SUPPORT_GPU_PHYSX
	class RigidBodyAccessGpu* mGpuRigidBodyAccess;
#endif
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_CONTEXT_CPU_H
