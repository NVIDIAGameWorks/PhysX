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

#include "PtContextCpu.h"
#if PX_USE_PARTICLE_SYSTEM_API

#if PX_SUPPORT_GPU_PHYSX
#include "task/PxGpuDispatcher.h"
#include "PxvGlobals.h"
#include "PxPhysXGpu.h"
#include "PxSceneGpu.h"
#include "gpu/PtRigidBodyAccessGpu.h"
#endif

#include "foundation/PxFoundation.h"
#include "PtParticleData.h"
#include "PtParticleSystemSimCpu.h"
#include "PtParticleShapeCpu.h"
#include "PtBatcher.h"
#include "PtBodyTransformVault.h"
#include "PsFoundation.h"

using namespace physx::shdfnd;
using namespace physx;
using namespace Pt;

namespace
{
ParticleSystemSim* (ContextCpu::*addParticleSystemFn)(ParticleData*, const ParticleSystemParameter&, bool);
ParticleData* (ContextCpu::*removeParticleSystemFn)(ParticleSystemSim*, bool);
Context* (*createContextFn)(physx::PxTaskManager*, Cm::FlushPool&);
void (ContextCpu::*destroyContextFn)();

PxBaseTask& (Batcher::*scheduleShapeGenerationFn)(ParticleSystemSim** particleSystems, ParticleShapesUpdateInput* inputs,
                                                  PxU32 batchSize, PxBaseTask& continuation) = 0;
PxBaseTask& (Batcher::*scheduleDynamicsCpuFn)(ParticleSystemSim** particleSystems, PxU32 batchSize,
                                              PxBaseTask& continuation) = 0;
PxBaseTask& (Batcher::*scheduleCollisionPrepFn)(ParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks,
                                                PxU32 batchSize, PxBaseTask& continuation) = 0;
PxBaseTask& (Batcher::*scheduleCollisionCpuFn)(ParticleSystemSim** particleSystems, PxU32 batchSize,
                                               PxBaseTask& continuation) = 0;
PxBaseTask& (Batcher::*schedulePipelineGpuFn)(ParticleSystemSim** particleSystems, PxU32 batchSize,
                                              PxBaseTask& continuation) = 0;
}

namespace physx
{
namespace Pt
{
void registerParticles()
{
	ContextCpu::registerParticles();
}

Context* createParticleContext(class physx::PxTaskManager* taskManager, Cm::FlushPool& taskPool)
{
	if(::createContextFn)
	{
		return ::createContextFn(taskManager, taskPool);
	}
	return NULL;
}
} // namespace Pt
} // namespace physx

void ContextCpu::registerParticles()
{
	::createContextFn = &ContextCpu::createContextImpl;
	::destroyContextFn = &ContextCpu::destroyContextImpl;
	::addParticleSystemFn = &ContextCpu::addParticleSystemImpl;
	::removeParticleSystemFn = &ContextCpu::removeParticleSystemImpl;

	::scheduleShapeGenerationFn = &Batcher::scheduleShapeGeneration;
	::scheduleDynamicsCpuFn = &Batcher::scheduleDynamicsCpu;
	::scheduleCollisionPrepFn = &Batcher::scheduleCollisionPrep;
	::scheduleCollisionCpuFn = &Batcher::scheduleCollisionCpu;
	::schedulePipelineGpuFn = &Batcher::schedulePipelineGpu;
}

Context* ContextCpu::createContextImpl(PxTaskManager* taskManager, Cm::FlushPool& taskPool)
{
	return PX_NEW(ContextCpu)(taskManager, taskPool);
}

void ContextCpu::destroy()
{
	(this->*destroyContextFn)();
}

void ContextCpu::destroyContextImpl()
{
	PX_DELETE(this);
}

ParticleSystemSim* ContextCpu::addParticleSystem(ParticleData* particleData, const ParticleSystemParameter& parameter,
                                                 bool useGpuSupport)
{
	return (this->*addParticleSystemFn)(particleData, parameter, useGpuSupport);
}

ParticleData* ContextCpu::removeParticleSystem(ParticleSystemSim* particleSystem, bool acquireParticleData)
{
	return (this->*removeParticleSystemFn)(particleSystem, acquireParticleData);
}

ContextCpu::ContextCpu(PxTaskManager* taskManager, Cm::FlushPool& taskPool)
: mParticleSystemPool("mParticleSystemPool", this, 16, 1024)
, mParticleShapePool("mParticleShapePool", this, 256, 1024)
, mBatcher(NULL)
, mTaskManager(taskManager)
, mTaskPool(taskPool)
#if PX_SUPPORT_GPU_PHYSX
, mGpuRigidBodyAccess(NULL)
#endif
{
	mBatcher = PX_NEW(Batcher)(*this);
	mBodyTransformVault = PX_NEW(BodyTransformVault);
	mSceneGpu = NULL;
}

ContextCpu::~ContextCpu()
{
#if PX_SUPPORT_GPU_PHYSX
	if(mSceneGpu)
	{
		mSceneGpu->release();
	}

	if(mGpuRigidBodyAccess)
	{
		PX_DELETE(mGpuRigidBodyAccess);
	}
#endif

	PX_DELETE(mBatcher);
	PX_DELETE(mBodyTransformVault);
}

ParticleSystemSim* ContextCpu::addParticleSystemImpl(ParticleData* particleData,
                                                     const ParticleSystemParameter& parameter, bool useGpuSupport)
{
	PX_ASSERT(particleData);

#if PX_SUPPORT_GPU_PHYSX
	if(useGpuSupport)
	{
		PxSceneGpu* sceneGPU = createOrGetSceneGpu();
		if(sceneGPU)
		{
			ParticleSystemStateDataDesc particles;
			particleData->getParticlesV(particles, true, false);
			ParticleSystemSim* sim = sceneGPU->addParticleSystem(particles, parameter);

			if(sim)
			{
				particleData->release();
				return sim;
			}
		}
		return NULL;
	}
	else
	{
		ParticleSystemSimCpu* sim = mParticleSystemPool.get();
		sim->init(*particleData, parameter);
		return sim;
	}
#else
	PX_UNUSED(useGpuSupport);
	ParticleSystemSimCpu* sim = mParticleSystemPool.get();
	sim->init(*particleData, parameter);
	return sim;
#endif
}

ParticleData* ContextCpu::removeParticleSystemImpl(ParticleSystemSim* particleSystem, bool acquireParticleData)
{
	ParticleData* particleData = NULL;

#if PX_SUPPORT_GPU_PHYSX
	if(particleSystem->isGpuV())
	{
		PX_ASSERT(getSceneGpuFast());
		if(acquireParticleData)
		{
			ParticleSystemStateDataDesc particles;
			particleSystem->getParticleStateV().getParticlesV(particles, true, false);
			particleData = ParticleData::create(particles, particleSystem->getParticleStateV().getWorldBoundsV());
		}
		getSceneGpuFast()->removeParticleSystem(particleSystem);
		return particleData;
	}
#endif

	ParticleSystemSimCpu& sim = *static_cast<ParticleSystemSimCpu*>(particleSystem);

	if(acquireParticleData)
		particleData = sim.obtainParticleState();

	sim.clear();
	mParticleSystemPool.put(&sim);
	return particleData;
}

ParticleShapeCpu* ContextCpu::createParticleShape(ParticleSystemSimCpu* particleSystem, const ParticleCell* packet)
{
	// for now just lock the mParticleShapePool for concurrent access from different tasks
	Ps::Mutex::ScopedLock lock(mParticleShapePoolMutex);
	ParticleShapeCpu* shape = mParticleShapePool.get();

	if(shape)
		shape->init(particleSystem, packet);

	return shape;
}

void ContextCpu::releaseParticleShape(ParticleShapeCpu* shape)
{
	// for now just lock the mParticleShapePool for concurrent access from different tasks
	Ps::Mutex::ScopedLock lock(mParticleShapePoolMutex);
	mParticleShapePool.put(shape);
}

#if PX_SUPPORT_GPU_PHYSX

PxSceneGpu* ContextCpu::createOrGetSceneGpu()
{
	if(mSceneGpu)
		return mSceneGpu;

	// get PxCudaContextManager

	if(!mTaskManager || !mTaskManager->getGpuDispatcher() || !mTaskManager->getGpuDispatcher()->getCudaContextManager())
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__,
		                          "GPU operation failed. No PxCudaContextManager available.");
		return NULL;
	}
	physx::PxCudaContextManager& contextManager = *mTaskManager->getGpuDispatcher()->getCudaContextManager();

	// load PhysXGpu dll interface

	PxPhysXGpu* physXGpu = PxvGetPhysXGpu(true);
	if(!physXGpu)
	{
		getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__,
		                      "GPU operation failed. PhysXGpu dll unavailable.");
		return NULL;
	}

	// create PxsGpuRigidBodyAccess

	PX_ASSERT(!mGpuRigidBodyAccess);
	mGpuRigidBodyAccess = PX_NEW(RigidBodyAccessGpu)(*mBodyTransformVault);

	// finally create PxSceneGpu
	mSceneGpu = physXGpu->createScene(contextManager, *mGpuRigidBodyAccess);
	if(!mSceneGpu)
	{
		PX_DELETE_AND_RESET(mGpuRigidBodyAccess);
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__,
		                          "GPU operation failed. PxSceneGpu creation unsuccessful.");
	}

	return mSceneGpu;
}
#endif // PX_SUPPORT_GPU_PHYSX

PxBaseTask& ContextCpu::scheduleShapeGeneration(class ParticleSystemSim** particleSystems,
                                                struct ParticleShapesUpdateInput* inputs, PxU32 batchSize,
                                                PxBaseTask& continuation)
{
	return (mBatcher->*::scheduleShapeGenerationFn)(particleSystems, inputs, batchSize, continuation);
}

PxBaseTask& ContextCpu::scheduleDynamicsCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
                                            PxBaseTask& continuation)
{
	return (mBatcher->*::scheduleDynamicsCpuFn)(particleSystems, batchSize, continuation);
}

PxBaseTask& ContextCpu::scheduleCollisionPrep(class ParticleSystemSim** particleSystems,
                                              PxLightCpuTask** inputPrepTasks, PxU32 batchSize, PxBaseTask& continuation)
{
	return (mBatcher->*::scheduleCollisionPrepFn)(particleSystems, inputPrepTasks, batchSize, continuation);
}

PxBaseTask& ContextCpu::scheduleCollisionCpu(class ParticleSystemSim** particleSystems, PxU32 batchSize,
                                             PxBaseTask& continuation)
{
	return (mBatcher->*::scheduleCollisionCpuFn)(particleSystems, batchSize, continuation);
}

PxBaseTask& ContextCpu::schedulePipelineGpu(ParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	return (mBatcher->*::schedulePipelineGpuFn)(particleSystems, batchSize, continuation);
}

#endif // PX_USE_PARTICLE_SYSTEM_API
