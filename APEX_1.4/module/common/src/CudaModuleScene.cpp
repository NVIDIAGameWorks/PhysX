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



#include "ApexDefs.h"
#if APEX_CUDA_SUPPORT

#include "Apex.h"
#include "ApexSDKIntl.h"
#include "SceneIntl.h"
#include "ApexCutil.h"
#include "CudaModuleScene.h"
#include <cuda.h>
#include <texture_types.h>

#include "PxTaskManager.h"
#include "PxGpuDispatcher.h"
#include "PxCudaContextManager.h"

#define CUDA_KERNEL_CHECK_ALWAYS 0

namespace nvidia
{
class PhysXGpuIndicator;

namespace apex
{

/**
 * Workaround hacks for using nvcc --compiler output object files
 * without linking with CUDART.  We must implement our own versions
 * of these functions that the object files are hard-coded to call into.
 */

#define MAX_MODULES					64
static void* moduleTable[ MAX_MODULES ];
static int numRegisteredModules = 0;

#define MAX_FUNCTIONS				256
typedef struct
{
	int         modIndex;
	const char* funcName;
} cuFuncDesc;
static cuFuncDesc functionTable[ MAX_FUNCTIONS ];
static int numRegisteredFunctions = 0;

const char* funcNameTable[ MAX_FUNCTIONS ];


#define MAX_TEXTURES				256
typedef struct
{
	int                             modIndex;
	const char*                     texRefName;
	const struct textureReference*  texRefData;
	int                             dim;
	int                             read_normalized_float;
} cuTexRefDesc;
static cuTexRefDesc textureTable[ MAX_TEXTURES ];
static int numRegisteredTextures = 0;


#define MAX_SURFACES				256
typedef struct
{
	int                             modIndex;
	const char*                     surfRefName;
	const struct surfaceReference*  surfRefData;
	int                             dim;
} cuSurfRefDesc;
static cuSurfRefDesc surfaceTable[ MAX_SURFACES ];
static int numRegisteredSurfaces = 0;


#define MAX_VARIABLES				256
typedef struct
{
	int			modIndex;
	const char* varName;
	int			size;
} cuVarDesc;
static cuVarDesc variableTable[ MAX_VARIABLES ];
static int numRegisteredVariables = 0;

CudaModuleScene::CudaModuleScene(SceneIntl& scene, Module& module, const char* modulePrefix)
	: mSceneIntl(scene)
{
	PX_UNUSED(modulePrefix);

	PxTaskManager* tm = scene.getTaskManager();
	PxGpuDispatcher* gd = tm->getGpuDispatcher();
	PX_ASSERT(gd != NULL);
	PxScopedCudaLock _lock_(*gd->getCudaContextManager());

	ApexCudaObjManager::init(&module, &scene.getApexCudaTestManager(), gd);

	mCudaModules.resize((uint32_t)numRegisteredModules);

	ApexSDKIntl* apexSdk = GetInternalApexSDK();
	mPhysXGpuIndicator = apexSdk->registerPhysXIndicatorGpuClient();
}

void CudaModuleScene::destroy(SceneIntl&)
{
	{
		PxScopedCudaLock _lock_(*getGpuDispatcher()->getCudaContextManager());

		ApexCudaObjManager::releaseAll();

		for (uint32_t i = 0 ; i < mCudaModules.size() ; i++)
		{
			mCudaModules[i].release();
		}
	}

	ApexSDKIntl* apexSdk = GetInternalApexSDK();
	apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
	mPhysXGpuIndicator = NULL;
}

void CudaModuleScene::onBeforeLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream)
{
	if (mCudaProfileSession)
	{
		mCudaProfileSession->onFuncStart(func.getProfileId(), stream);
	}
}

void CudaModuleScene::onAfterLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream)
{
	if (mCudaProfileSession)
	{
		mCudaProfileSession->onFuncFinish(func.getProfileId(), stream);
	}

#if !CUDA_KERNEL_CHECK_ALWAYS
	if (mSceneIntl.getCudaKernelCheckEnabled())
#endif
	{
		CUresult ret = cuStreamSynchronize(stream);
		if ( CUDA_SUCCESS != ret )
		{
			APEX_INTERNAL_ERROR("Cuda Error %d after launch of func '%s'", ret, func.getName());
			PX_ALWAYS_ASSERT();
		}
	}
}

ApexCudaModule* CudaModuleScene::getCudaModule(int modIndex)
{
	mCudaModules[(uint32_t)modIndex].init(moduleTable[(uint32_t)modIndex]);
	return &mCudaModules[(uint32_t)modIndex];
}

void CudaModuleScene::initCudaObj(ApexCudaTexRef& texRef)
{
	const char* texRefName = texRef.getName();

	for (int j = 0 ; j < numRegisteredTextures ; j++)
	{
		if (nvidia::strcmp(textureTable[j].texRefName, texRefName) == 0)
		{
			ApexCudaModule* cudaModule = getCudaModule(textureTable[j].modIndex);
			PX_ASSERT(cudaModule->isValid());

			CUtexref cuTexRef;
			CUT_SAFE_CALL(cuModuleGetTexRef(&cuTexRef, cudaModule->getCuModule(), texRefName));

			const struct textureReference* texRefData = textureTable[j].texRefData;

			PX_ASSERT(texRefData->channelDesc.x > 0);
			int numChannels = 1;
			if (texRefData->channelDesc.y > 0)
			{
				PX_ASSERT(texRefData->channelDesc.y == texRefData->channelDesc.x);
				++numChannels;
			}
			if (texRefData->channelDesc.z > 0)
			{
				PX_ASSERT(texRefData->channelDesc.z == texRefData->channelDesc.x);
				++numChannels;
			}
			if (texRefData->channelDesc.w > 0)
			{
				PX_ASSERT(texRefData->channelDesc.w == texRefData->channelDesc.x);
				++numChannels;
			}

			CUarray_format cuFormat = CUarray_format(0);
			switch (texRefData->channelDesc.f)
			{
			case cudaChannelFormatKindSigned:
				switch (texRefData->channelDesc.x)
				{
				case  8:
					cuFormat = CU_AD_FORMAT_SIGNED_INT8;
					break;
				case 16:
					cuFormat = CU_AD_FORMAT_SIGNED_INT16;
					break;
				case 32:
					cuFormat = CU_AD_FORMAT_SIGNED_INT32;
					break;
				}
				break;
			case cudaChannelFormatKindUnsigned:
				switch (texRefData->channelDesc.x)
				{
				case  8:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT8;
					break;
				case 16:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT16;
					break;
				case 32:
					cuFormat = CU_AD_FORMAT_UNSIGNED_INT32;
					break;
				}
				break;
			case cudaChannelFormatKindFloat:
				cuFormat = CU_AD_FORMAT_FLOAT;
				break;
			default:
				PX_ASSERT(0);
			};
			PX_ASSERT(cuFormat != 0);

			int cuFlags = 0;
			if (textureTable[j].read_normalized_float == 0)
			{
				cuFlags |= CU_TRSF_READ_AS_INTEGER;
			}
			if (textureTable[j].texRefData->normalized != 0)
			{
				cuFlags |= CU_TRSF_NORMALIZED_COORDINATES;
			}

			texRef.init(this, cuTexRef, cudaModule, cuFormat, numChannels, textureTable[j].dim, cuFlags);
			break;
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaVar& var)
{
	const char* varName = var.getName();

	for (int j = 0 ; j < numRegisteredVariables ; j++)
	{
		if (nvidia::strcmp(variableTable[j].varName, varName) == 0)
		{
			ApexCudaModule* cudaModule = getCudaModule(variableTable[j].modIndex);
			PX_ASSERT(cudaModule->isValid());

			CUdeviceptr cuDevPtr;
			size_t size;
			cuModuleGetGlobal(&cuDevPtr, &size, cudaModule->getCuModule(), varName);

			var.init(this, cudaModule, cuDevPtr, size, getGpuDispatcher()->getCudaContextManager());
			break;
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaFunc& func)
{
	for (int j = 0 ; j < numRegisteredFunctions ; j++)
	{
		const char* funcName = functionTable[j].funcName;
		if (func.testNameMatch(funcName))
		{
			ApexCudaModule* cudaModule = getCudaModule(functionTable[j].modIndex);
			PX_ASSERT(cudaModule->isValid());

			CUfunction cuFunc = 0;
			CUT_SAFE_CALL(cuModuleGetFunction(&cuFunc, cudaModule->getCuModule(), funcName));

			func.init(this, funcName, cuFunc, cudaModule);
		}
	}
}

void CudaModuleScene::initCudaObj(ApexCudaSurfRef& surfRef)
{
	if (getGpuDispatcher()->getCudaContextManager()->supportsArchSM20() == false)
	{
		return;
	}

	const char* surfRefName = surfRef.getName();

	for (int j = 0 ; j < numRegisteredSurfaces ; j++)
	{
		if (nvidia::strcmp(surfaceTable[j].surfRefName, surfRefName) == 0)
		{
			ApexCudaModule* cudaModule = getCudaModule(surfaceTable[j].modIndex);
			PX_ASSERT(cudaModule->isValid());

			CUsurfref cuSurfRef;
			CUT_SAFE_CALL(cuModuleGetSurfRef(&cuSurfRef, cudaModule->getCuModule(), surfRefName));

			surfRef.init(this, cuSurfRef, cudaModule);
			break;
		}
	}

}

/*
 * These calls are all made _before_ main() during static initialization
 * of your APEX module.  So calling into APEX Framework or other
 * external code modules is out of the question.
 */

#include "driver_types.h"

#define CUDARTAPI __stdcall

typedef struct uint3_t
{
	unsigned int x, y, z;
} uint3;

typedef struct dim3_t
{
	unsigned int x, y, z;
} dim3;

extern "C"
void** CUDARTAPI __cudaRegisterFatBinary(void* fatBin)
{
	//HACK to get real fatbin in CUDA 4.0
	struct CUIfatbinStruct
	{
		int magic;
		int version;
		void* fatbinArray;
		char* fatbinFile;
	};
	const CUIfatbinStruct* fatbinStruct = (const CUIfatbinStruct*)fatBin;
	if (fatbinStruct->magic == 0x466243B1)
	{
		fatBin = fatbinStruct->fatbinArray;
	}

	if (numRegisteredModules < MAX_MODULES)
	{
		moduleTable[ numRegisteredModules ] = fatBin;
		return (void**)(size_t) numRegisteredModules++;
	}
	return NULL;
}

extern "C"
void CUDARTAPI __cudaUnregisterFatBinary(void** fatCubinHandle)
{
	moduleTable[(int)(size_t) fatCubinHandle ] = 0;
}

extern "C"
void CUDARTAPI __cudaRegisterTexture(
    void**                    fatCubinHandle,
    const struct textureReference*  hostvar,
    const void**                    deviceAddress,
    const char*                     deviceName,
    int                       dim,
    int                       read_normalized_float,
    int                       ext)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostvar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(dim);
	PX_UNUSED(read_normalized_float);
	PX_UNUSED(ext);

	if (numRegisteredTextures < MAX_TEXTURES)
	{
		//Fix for CUDA 5.5 - remove leading "::"
		while (*deviceName == ':')
		{
			++deviceName;
		}

		// We need this association of function to module in order to find textures and globals
		textureTable[ numRegisteredTextures ].modIndex = (int)(size_t) fatCubinHandle;
		textureTable[ numRegisteredTextures ].texRefName = deviceName;
		textureTable[ numRegisteredTextures ].texRefData = hostvar;
		textureTable[ numRegisteredTextures ].dim = dim;
		textureTable[ numRegisteredTextures ].read_normalized_float = read_normalized_float;
		numRegisteredTextures++;
	}
}

extern "C"
void CUDARTAPI __cudaRegisterSurface(
    void**                          fatCubinHandle,
    const struct surfaceReference*        hostvar,
    const void**                          deviceAddress,
    const char*                           deviceName,
    int                             dim,
    int                             ext)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostvar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(dim);
	PX_UNUSED(ext);

	if (numRegisteredSurfaces < MAX_SURFACES)
	{
		//Fix for CUDA 5.5 - remove leading "::"
		while (*deviceName == ':')
		{
			++deviceName;
		}

		surfaceTable[ numRegisteredSurfaces ].modIndex = (int)(size_t) fatCubinHandle;
		surfaceTable[ numRegisteredSurfaces ].surfRefName = deviceName;
		surfaceTable[ numRegisteredSurfaces ].surfRefData = hostvar;
		surfaceTable[ numRegisteredSurfaces ].dim = dim;
		numRegisteredSurfaces++;
	}
}

extern "C" void CUDARTAPI __cudaRegisterVar(
    void**                    fatCubinHandle,
    char*                    hostVar,
    char*                    deviceAddress,
    const char*                    deviceName,
    int                      ext,
    int                      size,
    int                      constant,
    int                      global)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(hostVar);
	PX_UNUSED(deviceAddress);
	PX_UNUSED(deviceName);
	PX_UNUSED(ext);
	PX_UNUSED(size);
	PX_UNUSED(constant);
	PX_UNUSED(global);

	if (constant != 0 && numRegisteredVariables < MAX_VARIABLES)
	{
		variableTable[ numRegisteredVariables ].modIndex = (int)(size_t) fatCubinHandle;
		variableTable[ numRegisteredVariables ].varName = deviceName;
		variableTable[ numRegisteredVariables ].size = size;
		numRegisteredVariables++;
	}
}


extern "C" void CUDARTAPI __cudaRegisterShared(
    void**                    fatCubinHandle,
    void**                    devicePtr
)
{
	PX_UNUSED(fatCubinHandle);
	PX_UNUSED(devicePtr);
}



extern "C"
void CUDARTAPI __cudaRegisterFunction(
    void**  fatCubinHandle,
    const char*   hostFun,
    char*   deviceFun,
    const char*   deviceName,
    int     thread_limit,
    uint3*  tid,
    uint3*  bid,
    dim3*   bDim,
    dim3*   gDim,
    int*    wSize)
{
	PX_UNUSED(hostFun);
	PX_UNUSED(deviceFun);
	PX_UNUSED(thread_limit);
	PX_UNUSED(tid);
	PX_UNUSED(bid);
	PX_UNUSED(bDim);
	PX_UNUSED(gDim);
	PX_UNUSED(wSize);

	if (numRegisteredFunctions < MAX_FUNCTIONS)
	{
		// We need this association of function to module in order to find textures and globals
		functionTable[ numRegisteredFunctions ].modIndex = (int)(size_t) fatCubinHandle;
		functionTable[ numRegisteredFunctions ].funcName = deviceName;
		funcNameTable[ numRegisteredFunctions ] = deviceName;
		numRegisteredFunctions++;
	}
}

/* These functions are implemented just to resolve link dependencies */

extern "C"
cudaError_t CUDARTAPI cudaLaunch(const char* entry)
{
	PX_UNUSED(entry);
	return cudaSuccess;
}

extern "C"
cudaError_t CUDARTAPI cudaSetupArgument(
    const void*   arg,
    size_t  size,
    size_t  offset)
{
	PX_UNUSED(arg);
	PX_UNUSED(size);
	PX_UNUSED(offset);
	return cudaSuccess;
}

extern "C"
struct cudaChannelFormatDesc CUDARTAPI cudaCreateChannelDesc(
    int x, int y, int z, int w, enum cudaChannelFormatKind f)
{
	struct cudaChannelFormatDesc desc;
	desc.x = x;
	desc.y = y;
	desc.z = z;
	desc.w = w;
	desc.f = f;
	return desc;
}

}
} // namespace nvidia

#endif
