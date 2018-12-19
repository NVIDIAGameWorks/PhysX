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



#ifndef __CUDA_MODULE_SCENE__
#define __CUDA_MODULE_SCENE__

// Make this header is safe for inclusion in headers that are shared with device code.
#if !defined(__CUDACC__)

#include "ApexDefs.h"
#if APEX_CUDA_SUPPORT
#include "SceneIntl.h"
#include "ApexUsingNamespace.h"
#include "PsArray.h"
#include <cuda.h>
#include "ApexCudaWrapper.h"

namespace physx
{
	class PxGpuDispatcher;
}

namespace nvidia
{
class PhysXGpuIndicator;
namespace apex
{
class CudaTestManager;

/* Every CUDA capable ModuleSceneIntl will derive this class.  It
 * provides the access methods to your CUDA kernels that were compiled
 * into object files by nvcc.
 */
class CudaModuleScene : public ApexCudaObjManager
{
public:
	CudaModuleScene(SceneIntl& scene, Module& module, const char* modulePrefix = "");
	virtual ~CudaModuleScene() {}

	void destroy(SceneIntl& scene);

	void*	getHeadCudaObj()
	{
		return ApexCudaObjManager::getObjListHead();
	}

	nvidia::PhysXGpuIndicator*		mPhysXGpuIndicator;

	//ApexCudaObjManager
	virtual void onBeforeLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream);
	virtual void onAfterLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream);

protected:
	ApexCudaModule* getCudaModule(int modIndex);

	void initCudaObj(ApexCudaTexRef& obj);
	void initCudaObj(ApexCudaVar& obj);
	void initCudaObj(ApexCudaFunc& obj);
	void initCudaObj(ApexCudaSurfRef& obj);

	physx::Array<ApexCudaModule> mCudaModules;

	SceneIntl& mSceneIntl;

private:
	CudaModuleScene& operator=(const CudaModuleScene&);
};

}
} // namespace nvidia

#endif
#endif

#endif // __CUDA_MODULE_SCENE__
