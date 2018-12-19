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
  
#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API
#if PX_SUPPORT_GPU_PHYSX

#include "PxParticleDeviceExclusive.h"
#include "NpParticleSystem.h"
#include "NpParticleFluid.h"

using namespace physx;

//-------------------------------------------------------------------------------------------------------------------//

void PxParticleDeviceExclusive::enable(PxParticleBase& particleBase)
{
	if (particleBase.is<PxParticleSystem>())
		static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->enableDeviceExclusiveModeGpu();
	else if (particleBase.is<PxParticleFluid>())
		static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->enableDeviceExclusiveModeGpu();
}

//-------------------------------------------------------------------------------------------------------------------//

bool PxParticleDeviceExclusive::isEnabled(PxParticleBase& particleBase)
{
	if (particleBase.is<PxParticleSystem>())
		return static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->isDeviceExclusiveModeEnabledGpu();
	else if (particleBase.is<PxParticleFluid>())
		return static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->isDeviceExclusiveModeEnabledGpu();

	return false;
}

//-------------------------------------------------------------------------------------------------------------------//

void PxParticleDeviceExclusive::getReadWriteCudaBuffers(PxParticleBase& particleBase, PxCudaReadWriteParticleBuffers& buffers)
{
	if (particleBase.is<PxParticleSystem>())
		static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->getReadWriteCudaBuffersGpu(buffers);
	else if (particleBase.is<PxParticleFluid>())
		static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->getReadWriteCudaBuffersGpu(buffers);
}

//-------------------------------------------------------------------------------------------------------------------//

void PxParticleDeviceExclusive::setValidParticleRange(PxParticleBase& particleBase, PxU32 validParticleRange)
{
	if (particleBase.is<PxParticleSystem>())
		static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->setValidParticleRangeGpu(validParticleRange);
	else if (particleBase.is<PxParticleFluid>())
		static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->setValidParticleRangeGpu(validParticleRange);
}

//-------------------------------------------------------------------------------------------------------------------//

void PxParticleDeviceExclusive::setFlags(PxParticleBase& particleBase, PxParticleDeviceExclusiveFlags flags)
{
	if (particleBase.is<PxParticleSystem>())
		static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->setDeviceExclusiveModeFlagsGpu(reinterpret_cast<PxU32&>(flags));
	else if (particleBase.is<PxParticleFluid>())
		static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->setDeviceExclusiveModeFlagsGpu(reinterpret_cast<PxU32&>(flags));
}

//-------------------------------------------------------------------------------------------------------------------//

class physx::PxBaseTask* PxParticleDeviceExclusive::getLaunchTask(class PxParticleBase& particleBase)
{
	if (particleBase.is<PxParticleSystem>())
		return static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->getLaunchTaskGpu();
	else if (particleBase.is<PxParticleFluid>())
		return static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->getLaunchTaskGpu();

	return NULL;
}

//-------------------------------------------------------------------------------------------------------------------//

void PxParticleDeviceExclusive::addLaunchTaskDependent(class PxParticleBase& particleBase, class physx::PxBaseTask& dependent)
{
	if (particleBase.is<PxParticleSystem>())
		static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->addLaunchTaskDependentGpu(dependent);
	else if (particleBase.is<PxParticleFluid>())
		static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->addLaunchTaskDependentGpu(dependent);
}

//-------------------------------------------------------------------------------------------------------------------//

CUstream PxParticleDeviceExclusive::getCudaStream(class PxParticleBase& particleBase)
{
	if (particleBase.is<PxParticleSystem>())
		return (CUstream)static_cast<NpParticleSystem*>(particleBase.is<PxParticleSystem>())->getCudaStreamGpu();
	else if (particleBase.is<PxParticleFluid>())
		return (CUstream)static_cast<NpParticleFluid*>(particleBase.is<PxParticleFluid>())->getCudaStreamGpu();

	return NULL;
}

//-------------------------------------------------------------------------------------------------------------------//

#endif // PX_SUPPORT_GPU_PHYSX
#endif // PX_USE_PARTICLE_SYSTEM_API
