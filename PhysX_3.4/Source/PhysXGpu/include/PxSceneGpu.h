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


#ifndef PX_SCENE_GPU_H
#define PX_SCENE_GPU_H

#include "Ps.h"

namespace physx
{

	class PxBaseTask;

namespace cloth
{
	class Factory;
	class Cloth;
}

namespace Pt
{
	class ParticleSystemSim;
	struct ParticleSystemStateDataDesc;
	struct ParticleSystemParameter;
	struct ParticleShapesUpdateInput;
}

/**
\brief Interface to manage a set of cuda accelerated feature instances that share the same physx::PxCudaContextManager and PxRigidBodyAccessGpu instance.
*/
class PxSceneGpu
{
public:

	/**
	\brief release instance.
	*/
	virtual		void					release() = 0;
	
	/**
	Adds a particle system to the cuda PhysX lowlevel. Currently the particle system is just associated with a CudaContextManager. 
	Later it will be will be is some kind of scene level context for batched stepping.

	\param state The particle state to initialize the particle system. For initialization with 0 particles, Pt::ParticleSystemStateDataDesc::validParticleRange == 0.
	\param parameter To configure the particle system pipeline
	*/
	virtual		Pt::ParticleSystemSim*	addParticleSystem(const Pt::ParticleSystemStateDataDesc& state, const Pt::ParticleSystemParameter& parameter) = 0;

	/**
	Removed a particle system from the cuda PhysX lowlevel.
	*/
	virtual		void					removeParticleSystem(Pt::ParticleSystemSim* particleSystem) = 0;

	/**
	Notify shape change
	*/
	virtual		void					onShapeChange(size_t shapeHandle, size_t bodyHandle, bool isDynamic) = 0;

	/**
	Batched scheduling of shape generation. Pt::ParticleShapesUpdateInput::shapes ownership transfered to callee.
	*/														
	virtual		physx::PxBaseTask&		scheduleParticleShapeUpdate(Pt::ParticleSystemSim** particleSystems, Pt::ParticleShapesUpdateInput* inputs, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;
	
	/**
	Batched scheduling of collision input update.
	*/
	virtual		physx::PxBaseTask&		scheduleParticleCollisionInputUpdate(Pt::ParticleSystemSim** particleSystems, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;
	
	/**
	Batched scheduling of particles update.
	*/														
	virtual		physx::PxBaseTask&		scheduleParticlePipeline(Pt::ParticleSystemSim** particleSystems, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;
};

}

#endif // PX_SCENE_GPU_H
