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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "Apex.h"
#include "ApexSDKImpl.h"
#include "ProfilerCallback.h"
#include "PxTaskManager.h"
#include "PxCpuDispatcher.h"
#include "ApexCudaContextManager.h"
#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
#include "CudaContextManager.h"
#include "PhysXDeviceSettings.h"
#endif
#include "PxCudaContextManager.h"
#include "PxErrorCallback.h"

#if PX_PHYSICS_VERSION_MAJOR == 0
#include "ThreadPool.h"
#endif

namespace nvidia
{
namespace apex
{

ApexSDKImpl* gApexSdk = NULL;	// For an SDK singleton

APEX_API ApexSDK*	CALL_CONV GetApexSDK()
{
	return gApexSdk;
}
APEX_API ApexSDKIntl*	CALL_CONV GetInternalApexSDK()
{
	return gApexSdk;
}

ApexSDK* CreateApexSDK(const ApexSDKDesc& desc, ApexCreateError* errorCode, uint32_t APEXsdkVersion, PxAllocatorCallback* alloc)
{
	PX_UNUSED(alloc);

	if (errorCode)
	{
		*errorCode = APEX_CE_NO_ERROR;
	}

	if (gApexSdk != NULL)
	{
		return gApexSdk;
	}

	if (APEXsdkVersion != APEX_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	if (!desc.isValid())	//this checks for SDK and cooking version mismatch!
	{
		if (errorCode)
		{
			if (desc.physXSDKVersion != PX_PHYSICS_VERSION)
			{
				*errorCode = APEX_CE_WRONG_VERSION;
			}
			else
			{
				*errorCode = APEX_CE_DESCRIPTOR_INVALID;
			}
		}
		return NULL;
	}


	PX_ASSERT(alloc == 0);

	gApexSdk = PX_NEW(ApexSDKImpl)(errorCode, APEXsdkVersion);
	gApexSdk->init(desc);

	return gApexSdk;
}

int GetSuggestedCudaDeviceOrdinal(PxErrorCallback& errc)
{
#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
	return PhysXDeviceSettings::getSuggestedCudaDeviceOrdinal(errc);
#else
	PX_UNUSED(errc);
	return -1;
#endif
}

PxCudaContextManager* CreateCudaContextManager(const PxCudaContextManagerDesc& desc, PxErrorCallback& errorCallback)
{
#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
	return physx::createCudaContextManager(desc, errorCallback);
#else
	PX_UNUSED(desc);
	PX_UNUSED(errorCallback);
	return NULL;
#endif
}


#if PX_PHYSICS_VERSION_MAJOR == 0

/* We route allocations of these objects through the APEX SDK because PXTASK objects
 * require a foundation instance.
 */

PxCpuDispatcher* ApexSDKImpl::getDefaultThreadPool()
{
	if (!mApexThreadPool)
	{
		mApexThreadPool = createDefaultThreadPool(0);
	}

	return mApexThreadPool;
}

PxCpuDispatcher* ApexSDKImpl::createCpuDispatcher(uint32_t numThreads)
{
	PxCpuDispatcher* cd = createDefaultThreadPool(numThreads);
	mUserAllocThreadPools.pushBack(cd);
	return cd;
}

void ApexSDKImpl::releaseCpuDispatcher(PxCpuDispatcher& cd)
{
	if (&cd == mApexThreadPool)
	{
		PX_DELETE(mApexThreadPool);
		mApexThreadPool = 0;
		return;
	}
	for (uint32_t i = 0; i < mUserAllocThreadPools.size(); i++)
	{
		if (&cd == mUserAllocThreadPools[i])
		{
			PX_DELETE(&cd);
			mUserAllocThreadPools.replaceWithLast(i);
			return;
		}
	}
}

#endif

}
} // end namespace nvidia::apex
