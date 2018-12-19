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

#include "PtBatcher.h"
#if PX_USE_PARTICLE_SYSTEM_API

#if PX_SUPPORT_GPU_PHYSX
#include "PxPhysXGpu.h"
#endif

#include "task/PxTask.h"
#include "PtContext.h"
#include "PtParticleSystemSim.h"
#include "PtParticleSystemSimCpu.h"

using namespace physx;
using namespace Pt;

namespace
{
#if PX_SUPPORT_GPU_PHYSX
template <class T>
static void sortBatchedInputs(ParticleSystemSim** particleSystems, T* inputs, PxU32 batchSize, PxU32& cpuOffset,
                              PxU32& cpuCount, PxU32& gpuOffset, PxU32& gpuCount)
{
	PX_UNUSED(particleSystems);
	PX_UNUSED(inputs);

	cpuOffset = 0;
	gpuOffset = 0;

	// in place sort of both arrays
	PxU32 i = 0;
	PxU32 j = 0;

	while((i < batchSize) && (j < batchSize))
	{
#if PX_SUPPORT_GPU_PHYSX
		if(particleSystems[i]->isGpuV())
		{
			j = i + 1;
			while(j < batchSize && particleSystems[j]->isGpuV())
			{
				j++;
			}

			if(j < batchSize)
			{
				Ps::swap(particleSystems[i], particleSystems[j]);
				if(inputs)
				{
					Ps::swap(inputs[i], inputs[j]);
				}
				i++;
			}
		}
		else
#endif
		{
			i++;
		}
	}

	gpuOffset = i;
	cpuCount = gpuOffset;
	gpuCount = batchSize - cpuCount;
}
#endif
}

Batcher::Batcher(class Context& _context)
: shapeGenTask(0, "Pt::Batcher::shapeGen")
, dynamicsCpuTask(0, "Pt::Batcher::dynamicsCpu")
, collPrepTask(0, "Pt::Batcher::collPrep")
, collisionCpuTask(0, "Pt::Batcher::collisionCpu")
, context(_context)
{
}

PxBaseTask& Batcher::scheduleShapeGeneration(ParticleSystemSim** particleSystems, ParticleShapesUpdateInput* inputs,
                                             PxU32 batchSize, PxBaseTask& continuation)
{
	PxU32 cpuOffset = 0;
	PxU32 cpuCount = batchSize;

#if PX_SUPPORT_GPU_PHYSX
	PxU32 gpuOffset, gpuCount;
	sortBatchedInputs(particleSystems, inputs, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
	if(context.getSceneGpuFast() && gpuCount > 0)
	{
		PxBaseTask& task = context.getSceneGpuFast()->scheduleParticleShapeUpdate(
		    particleSystems + gpuOffset, inputs + gpuOffset, gpuCount, continuation);
		shapeGenTask.addDependent(task);
		task.removeReference();
	}
#endif
	for(PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
	{
		PxBaseTask& task =
		    static_cast<ParticleSystemSimCpu*>(particleSystems[i])->schedulePacketShapesUpdate(inputs[i], continuation);
		shapeGenTask.addDependent(task);
		task.removeReference();
	}

	if(shapeGenTask.getReference() == 0)
	{
		continuation.addReference();
		return continuation;
	}

	while(shapeGenTask.getReference() > 1)
		shapeGenTask.removeReference();

	return shapeGenTask;
}

PxBaseTask& Batcher::scheduleDynamicsCpu(ParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	PxU32 cpuOffset = 0;
	PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
	PxU32 gpuOffset, gpuCount;
	sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
#endif
	for(PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
	{
		PxBaseTask& task = static_cast<ParticleSystemSimCpu*>(particleSystems[i])->scheduleDynamicsUpdate(continuation);
		dynamicsCpuTask.addDependent(task);
		task.removeReference();
	}

	if(dynamicsCpuTask.getReference() == 0)
	{
		continuation.addReference();
		return continuation;
	}

	while(dynamicsCpuTask.getReference() > 1)
		dynamicsCpuTask.removeReference();

	return dynamicsCpuTask;
}

PxBaseTask& Batcher::scheduleCollisionPrep(ParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks,
                                           PxU32 batchSize, PxBaseTask& continuation)
{
	PxU32 cpuOffset = 0;
	PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
	PxU32 gpuOffset, gpuCount;
	sortBatchedInputs(particleSystems, inputPrepTasks, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
	if(context.getSceneGpuFast() && gpuCount > 0)
	{
		PxBaseTask& gpuCollisionInputTask = context.getSceneGpuFast()->scheduleParticleCollisionInputUpdate(
		    particleSystems + gpuOffset, gpuCount, continuation);
		for(PxU32 i = gpuOffset; i < (gpuOffset + gpuCount); ++i)
		{
			inputPrepTasks[i]->setContinuation(&gpuCollisionInputTask);
			collPrepTask.addDependent(*inputPrepTasks[i]);
			inputPrepTasks[i]->removeReference();
		}
		gpuCollisionInputTask.removeReference();
	}
#else
	PX_UNUSED(particleSystems);
	PX_UNUSED(batchSize);
#endif
	for(PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
	{
		inputPrepTasks[i]->setContinuation(&continuation);
		collPrepTask.addDependent(*inputPrepTasks[i]);
		inputPrepTasks[i]->removeReference();
	}

	if(collPrepTask.getReference() == 0)
	{
		continuation.addReference();
		return continuation;
	}

	while(collPrepTask.getReference() > 1)
		collPrepTask.removeReference();

	return collPrepTask;
}

PxBaseTask& Batcher::scheduleCollisionCpu(ParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	PxU32 cpuOffset = 0;
	PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
	PxU32 gpuOffset, gpuCount;
	sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
#endif
	for(PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
	{
		PxBaseTask& task = static_cast<ParticleSystemSimCpu*>(particleSystems[i])->scheduleCollisionUpdate(continuation);
		collisionCpuTask.addDependent(task);
		task.removeReference();
	}

	if(collisionCpuTask.getReference() == 0)
	{
		continuation.addReference();
		return continuation;
	}

	while(collisionCpuTask.getReference() > 1)
		collisionCpuTask.removeReference();

	return collisionCpuTask;
}

PxBaseTask& Batcher::schedulePipelineGpu(ParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
#if PX_SUPPORT_GPU_PHYSX
	PxU32 cpuOffset, cpuCount, gpuOffset, gpuCount;
	sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
	if(context.getSceneGpuFast() && gpuCount > 0)
	{
		return context.getSceneGpuFast()->scheduleParticlePipeline(particleSystems + gpuOffset, gpuCount, continuation);
	}
#else
	PX_UNUSED(batchSize);
	PX_UNUSED(particleSystems);
#endif
	continuation.addReference();
	return continuation;
}

#endif // PX_USE_PARTICLE_SYSTEM_API
