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


#ifndef PX_PHYSX_INDICATOR_DEVICE_EXCLUSIVE_H
#define PX_PHYSX_INDICATOR_DEVICE_EXCLUSIVE_H

#include "PxPhysXCommonConfig.h"

namespace physx
{

/**
\brief API for gpu specific PhysX Indicator functionality.
*/
class PxPhysXIndicatorDeviceExclusive
{
public:

	/**
	\brief Register external Gpu client of PhysX Indicator.
	
	By calling this method, the PhysX Indicator will increment the number of external Cpu clients by one.

	\param[in] physics PxPhysics to register the client in.

	@see PxPhysXIndicatorDeviceExclusive.unregisterPhysXIndicatorGpuClient
	*/
	PX_PHYSX_CORE_API static void registerPhysXIndicatorGpuClient(class PxPhysics& physics);

	/**
	\brief Unregister external Gpu client of PhysX Indicator.
	
	By calling this method, the PhysX Indicator will decrement the number of external Cpu clients by one.

	\param[in] physics PxPhysics to unregister the client in.

	@see PxPhysXIndicatorDeviceExclusive.registerPhysXIndicatorGpuClient
	*/
	PX_PHYSX_CORE_API static void unregisterPhysXIndicatorGpuClient(class PxPhysics& physics);
};

}

#endif // PX_PHYSX_INDICATOR_DEVICE_EXCLUSIVE_H
