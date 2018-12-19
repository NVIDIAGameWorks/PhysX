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



#ifndef APEX_CUDA_H
#define APEX_CUDA_H

#include <cuda.h>
#include "ApexCudaDefs.h"

#define APEX_CUDA_CONCAT_I(arg1, arg2) arg1 ## arg2
#define APEX_CUDA_CONCAT(arg1, arg2) APEX_CUDA_CONCAT_I(arg1, arg2)

#define APEX_CUDA_TO_STR_I(arg) # arg
#define APEX_CUDA_TO_STR(arg) APEX_CUDA_TO_STR_I(arg)

const unsigned int APEX_CUDA_SINGLE_BLOCK_LAUNCH = 0xFFFFFFFF;

#define APEX_CUDA_KERNEL_DEFAULT_CONFIG ()
#define APEX_CUDA_KERNEL_2D_CONFIG(x, y) (0, 0, nvidia::apex::DimBlock(x, y, 1))
#define APEX_CUDA_KERNEL_3D_CONFIG(x, y, z) (0, 0, nvidia::apex::DimBlock(x, y, z))

#define APEX_CUDA_TEX_FILTER_POINT	CU_TR_FILTER_MODE_POINT
#define APEX_CUDA_TEX_FILTER_LINEAR	CU_TR_FILTER_MODE_LINEAR


#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/cat.hpp>


#define __APEX_CUDA_FUNC_ARG_NAME(elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)

#define __APEX_CUDA_FUNC_ARG_I(r, data, i, elem) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 0, elem) __APEX_CUDA_FUNC_ARG_NAME(elem)
#define __APEX_CUDA_FUNC_ARGS(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_ARG_I, _, argseq)

#define __APEX_CUDA_FUNC_ARG_NAME_I(r, data, i, elem) BOOST_PP_COMMA_IF(i) __APEX_CUDA_FUNC_ARG_NAME(elem)
#define __APEX_CUDA_FUNC_ARG_NAMES(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_ARG_NAME_I, _, argseq)


#define __APEX_CUDA_FUNC_$ARG_NAME(elem) BOOST_PP_CAT(_$arg_, BOOST_PP_TUPLE_ELEM(2, 1, elem))

#define __APEX_CUDA_FUNC_$ARG_I(r, data, i, elem) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 0, elem) __APEX_CUDA_FUNC_$ARG_NAME(elem)
#define __APEX_CUDA_FUNC_$ARGS(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_$ARG_I, _, argseq)

#define __APEX_CUDA_FUNC_$ARG_NAME_I(r, data, i, elem) BOOST_PP_COMMA_IF(i) __APEX_CUDA_FUNC_$ARG_NAME(elem)
#define __APEX_CUDA_FUNC_$ARG_NAMES(argseq) BOOST_PP_SEQ_FOR_EACH_I(__APEX_CUDA_FUNC_$ARG_NAME_I, _, argseq)


#define __APEX_CUDA_FUNC_SET_PARAM(r, data, elem) setParam( data, __APEX_CUDA_FUNC_$ARG_NAME(elem) );

#define __APEX_CUDA_FUNC_COPY_PARAM(r, data, elem) copyParam( APEX_CUDA_TO_STR(__APEX_CUDA_FUNC_ARG_NAME(elem)), __APEX_CUDA_FUNC_$ARG_NAME(elem) );

#define APEX_CUDA_NAME(name) APEX_CUDA_CONCAT(APEX_CUDA_MODULE_PREFIX, name)
#define APEX_CUDA_NAME_STR(name) APEX_CUDA_TO_STR( APEX_CUDA_NAME(name) )

#define APEX_CUDA_TEX_REF_NAME(name) APEX_CUDA_NAME( APEX_CUDA_CONCAT(texRef, name) )
#define APEX_CUDA_SURF_REF_NAME(name) APEX_CUDA_NAME( APEX_CUDA_CONCAT(surfRef, name) )

#define APEX_CUDA_STORAGE(name) APEX_CUDA_STORAGE_SIZE(name, MAX_CONST_MEM_SIZE)

#ifdef __CUDACC__

#define APEX_MEM_BLOCK(format) format*

#define APEX_CUDA_TEXTURE_1D(name, format) texture<format, cudaTextureType1D, cudaReadModeElementType> APEX_CUDA_NAME(name);
#define APEX_CUDA_TEXTURE_2D(name, format) texture<format, cudaTextureType2D, cudaReadModeElementType> APEX_CUDA_NAME(name);
#define APEX_CUDA_TEXTURE_3D(name, format) texture<format, cudaTextureType3D, cudaReadModeElementType> APEX_CUDA_NAME(name);
#define APEX_CUDA_TEXTURE_3D_FILTER(name, format, filter) APEX_CUDA_TEXTURE_3D(name, format)

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 200

#define APEX_CUDA_SURFACE_1D(name)
#define APEX_CUDA_SURFACE_2D(name)
#define APEX_CUDA_SURFACE_3D(name)

#else

#define APEX_CUDA_SURFACE_1D(name) surface<void, cudaSurfaceType1D> APEX_CUDA_NAME(name);
#define APEX_CUDA_SURFACE_2D(name) surface<void, cudaSurfaceType2D> APEX_CUDA_NAME(name);
#define APEX_CUDA_SURFACE_3D(name) surface<void, cudaSurfaceType3D> APEX_CUDA_NAME(name);

#endif

#define APEX_CUDA_STORAGE_SIZE(name, size) \
	__constant__ unsigned char APEX_CUDA_NAME( APEX_CUDA_CONCAT(name, _ConstMem) )[size]; \
	texture<int, 1, cudaReadModeElementType> APEX_CUDA_NAME( APEX_CUDA_CONCAT(name, _Texture) );


#define APEX_CUDA_FREE_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, uint16_t _kernelEnum, uint32_t _threadCount, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_FREE_KERNEL_2D(kernelDim, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, uint16_t _kernelEnum, uint32_t _threadCountX, uint32_t _threadCountY, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_FREE_KERNEL_3D(kernelDim, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, uint16_t _kernelEnum, uint32_t _threadCountX, uint32_t _threadCountY, uint32_t _threadCountZ, uint32_t _blockCountY, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_BOUND_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, uint16_t _kernelEnum, uint32_t _threadCount, uint32_t _maxGridSize, __APEX_CUDA_FUNC_ARGS(argseq) );

#define APEX_CUDA_SYNC_KERNEL(kernelWarps, kernelName, argseq) \
	extern "C" __global__ void APEX_CUDA_NAME(kernelName)(int* _extMem, uint16_t _kernelEnum, __APEX_CUDA_FUNC_ARGS(argseq) );

#else

#define APEX_CUDA_CLASS_NAME(name) APEX_CUDA_CONCAT(CudaClass_, APEX_CUDA_NAME(name) )
#define APEX_CUDA_OBJ_NAME(name) APEX_CUDA_CONCAT(cudaObj_, APEX_CUDA_NAME(name) )

#define APEX_MEM_BLOCK(format) const ApexCudaMemRef<format>&

#define __APEX_CUDA_TEXTURE(name, filter) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaTexRef { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaTexRef( APEX_CUDA_NAME_STR(name), filter ) {} \
	} APEX_CUDA_OBJ_NAME(name); \

#define APEX_CUDA_TEXTURE_1D(name, format) __APEX_CUDA_TEXTURE(name, APEX_CUDA_TEX_FILTER_POINT)
#define APEX_CUDA_TEXTURE_2D(name, format) __APEX_CUDA_TEXTURE(name, APEX_CUDA_TEX_FILTER_POINT)
#define APEX_CUDA_TEXTURE_3D(name, format) __APEX_CUDA_TEXTURE(name, APEX_CUDA_TEX_FILTER_POINT)
#define APEX_CUDA_TEXTURE_3D_FILTER(name, format, filter) __APEX_CUDA_TEXTURE(name, filter)


#define __APEX_CUDA_SURFACE(name) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaSurfRef { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaSurfRef( APEX_CUDA_NAME_STR(name) ) {} \
	} APEX_CUDA_OBJ_NAME(name); \
	 
#define APEX_CUDA_SURFACE_1D(name) __APEX_CUDA_SURFACE(name)
#define APEX_CUDA_SURFACE_2D(name) __APEX_CUDA_SURFACE(name)
#define APEX_CUDA_SURFACE_3D(name) __APEX_CUDA_SURFACE(name)

#define APEX_CUDA_STORAGE_SIZE(name, size) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaConstStorage { \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaConstStorage( APEX_CUDA_NAME_STR( APEX_CUDA_CONCAT(name, _ConstMem) ), APEX_CUDA_NAME_STR( APEX_CUDA_CONCAT(name, _Texture) ) ) {} \
	} APEX_CUDA_OBJ_NAME(name); \


#define __APEX_CUDA_KERNEL_START($blocksPerSM, $config, name, argseq) \
	class APEX_CUDA_CLASS_NAME(name) : public ApexCudaFunc \
	{ \
	public: \
		APEX_CUDA_CLASS_NAME(name) () : ApexCudaFunc( APEX_CUDA_NAME_STR(name) ) {} \
		PX_INLINE bool isSingleBlock(unsigned int threadCount) const \
		{ \
			const FuncInstData& fid = getFuncInstData(); \
			return (threadCount <= fid.mWarpsPerBlock * WARP_SIZE); \
		} \
	protected: \
		void launch1(const FuncInstData& fid, ApexCudaFuncParams& params, CUstream stream) \
		{ \
			PX_UNUSED(fid); \
			PX_ASSERT( isValid() ); \
			setParam(params, (int*)NULL); \
			setParam(params, uint16_t(0)); \
			mCTContext = mManager->getCudaTestManager()->isTestKernel(mName, mManager->getModule()->getName());\
			if (mCTContext) \
			{ \
				mCTContext->setCuStream(stream); \
				resolveContext(); \
			} \
		} \
		void launch2(const FuncInstData& fid, const DimBlock& blockDim, uint32_t sharedSize, ApexCudaFuncParams& params, CUstream stream, const DimGrid& gridDim, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			if (mCTContext)	{ \
				mCTContext->setGridDim(gridDim.x, gridDim.y); \
				mCTContext->setBlockDim(blockDim.x, blockDim.y, blockDim.z); \
				mCTContext->setSharedSize(sharedSize); \
				mCTContext->setFuncInstId(int(&fid - mFuncInstData)); \
				BOOST_PP_SEQ_FOR_EACH(__APEX_CUDA_FUNC_COPY_PARAM, , argseq); \
			} \
			BOOST_PP_SEQ_FOR_EACH(__APEX_CUDA_FUNC_SET_PARAM, params, argseq); \
			void *config[5] = { \
				CU_LAUNCH_PARAM_BUFFER_POINTER, params.mParams, \
				CU_LAUNCH_PARAM_BUFFER_SIZE,    &params.mOffset, \
				CU_LAUNCH_PARAM_END \
			}; \
			onBeforeLaunch(stream); \
			CUT_SAFE_CALL(cuLaunchKernel(fid.mCuFunc, gridDim.x, gridDim.y, 1, blockDim.x, blockDim.y, blockDim.z, sharedSize, stream, 0, (void **)config)); \
			onAfterLaunch(stream); \
			if (mCTContext) mCTContext->setKernelStatus(); \
		} \
		PX_INLINE void evalLaunchParams(const ApexKernelConfig& kernelConfig, const FuncInstData& fid, uint32_t &outWarpsPerBlock, uint32_t &outDynamicShared) \
		{ \
			const ApexCudaDeviceTraits& devTraits = mManager->getDeviceTraits(); \
			const uint32_t fixedSharedMem = (kernelConfig.fixedSharedMemDWords << 2); \
			const uint32_t sharedMemPerWarp = (kernelConfig.sharedMemDWordsPerWarp << 2); \
			const uint32_t staticSharedMem = fid.mStaticSharedSize + fixedSharedMem; \
			PX_ASSERT(staticSharedMem + sharedMemPerWarp * 1 <= devTraits.mMaxSharedMemPerBlock); \
			if (kernelConfig.blockDim.x == 0) \
			{ \
				const uint32_t maxThreadsPerSM = physx::PxMin(devTraits.mMaxRegistersPerSM / fid.mNumRegsPerThread, devTraits.mMaxThreadsPerSM); \
				outWarpsPerBlock = physx::PxMin(fid.mMaxThreadsPerBlock, maxThreadsPerSM / $blocksPerSM) >> LOG2_WARP_SIZE; \
				if (sharedMemPerWarp > 0) \
				{ \
					const uint32_t sharedMemLimit4SM = (devTraits.mMaxSharedMemPerSM - staticSharedMem * $blocksPerSM) / (sharedMemPerWarp * $blocksPerSM); \
					const uint32_t sharedMemLimit4Block = (devTraits.mMaxSharedMemPerBlock - staticSharedMem) / sharedMemPerWarp; \
					const uint32_t sharedMemLimit = physx::PxMin(sharedMemLimit4SM, sharedMemLimit4Block); \
					outWarpsPerBlock = PxMin<uint32_t>(outWarpsPerBlock, sharedMemLimit); \
				} \
				PX_ASSERT(outWarpsPerBlock > 0); \
			} \
			else \
			{ \
				outWarpsPerBlock = (kernelConfig.blockDim.x * kernelConfig.blockDim.y * kernelConfig.blockDim.z + WARP_SIZE-1) / WARP_SIZE; \
			} \
			outDynamicShared = fixedSharedMem + sharedMemPerWarp * outWarpsPerBlock; \
			PX_ASSERT(fid.mStaticSharedSize + outDynamicShared <= devTraits.mMaxSharedMemPerBlock); \
			PX_ASSERT(outWarpsPerBlock * WARP_SIZE <= fid.mMaxThreadsPerBlock); \
			PX_ASSERT(outWarpsPerBlock > 0); \
		} \
		virtual void init( PxCudaContextManager* ctx, int funcInstIndex ) \
		{ \
			PX_UNUSED(ctx); \
			ApexKernelConfig kernelConfig = ApexKernelConfig $config; \
			FuncInstData& fid = mFuncInstData[(uint32_t)funcInstIndex]; \
			evalLaunchParams(kernelConfig, fid, fid.mWarpsPerBlock, fid.mDynamicShared); \
			 

#define __APEX_CUDA_KERNEL_WARPS_END(name, argseq) \
			PX_ASSERT(mMaxBlocksPerGrid > 0); \
		} \
	private: \
		uint32_t mMaxBlocksPerGrid; \
		uint32_t launch(const FuncInstData& fid, uint32_t warpsPerBlock, uint32_t dynamicShared, CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			warpsPerBlock = PxMin(warpsPerBlock, MAX_WARPS_PER_BLOCK); /* TODO: refactor old kernels to avoid this */ \
			uint32_t threadsPerBlock = warpsPerBlock * WARP_SIZE; \
			uint32_t blocksPerGrid = 1; \
			if (_threadCount == APEX_CUDA_SINGLE_BLOCK_LAUNCH) \
			{ \
				_threadCount = threadsPerBlock; \
			} \
			else \
			{ \
				if (_threadCount > threadsPerBlock) \
				{ \
					blocksPerGrid = PxMin((_threadCount + threadsPerBlock - 1) / threadsPerBlock, mMaxBlocksPerGrid); \
				} \
				else \
				{ \
					threadsPerBlock = APEX_CUDA_ALIGN_UP(_threadCount, WARP_SIZE); \
				} \
			} \
			ApexCudaFuncParams params; \
			launch1(fid, params, stream); \
			if (mCTContext)	mCTContext->setBoundKernel(_threadCount); \
			setParam(params, _threadCount); \
			if (mMaxBlocksPerGrid != UINT_MAX) setParam(params, mMaxBlocksPerGrid); \
			launch2(fid, DimBlock(threadsPerBlock), dynamicShared, params, stream, DimGrid(blocksPerGrid), __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
			return blocksPerGrid; \
		} \
	public: \
		uint32_t operator() ( CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			const FuncInstData& fid = getFuncInstData(); \
			return launch(fid, fid.mWarpsPerBlock, fid.mDynamicShared, stream, _threadCount, __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
		} \
		uint32_t operator() ( const ApexKernelConfig& kernelConfig, CUstream stream, unsigned int _threadCount, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			PX_ASSERT(kernelConfig.maxGridSizeMul == 0); \
			const FuncInstData& fid = getFuncInstData(); \
			uint32_t warpsPerBlock; \
			uint32_t dynamicShared; \
			evalLaunchParams(kernelConfig, fid, warpsPerBlock, dynamicShared); \
			return launch(fid, warpsPerBlock, dynamicShared, stream, _threadCount, __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
		} \
		uint32_t getMaxGridSize() const { return mMaxBlocksPerGrid; } \
	} APEX_CUDA_OBJ_NAME(name); \


#define APEX_CUDA_SYNC_KERNEL(config, name, argseq) \
	__APEX_CUDA_KERNEL_START(1, config, name, argseq) \
			mBlocksPerGrid = (uint32_t)ctx->getMultiprocessorCount(); \
		} \
	private: \
		uint32_t mBlocksPerGrid; \
	public: \
		void operator() ( CUstream stream, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			const FuncInstData& fid = getFuncInstData(); \
			ApexCudaFuncParams params; \
			launch1(fid, params, stream); \
			if (mCTContext)	mCTContext->setSyncKernel(); \
			/* alloc full dynamic shared memory for correct block distrib. on GF100 */ \
			uint32_t dynamicSharedSize = mManager->getDeviceTraits().mMaxSharedMemPerBlock - fid.mStaticSharedSize; \
			launch2(fid, DimBlock(fid.mWarpsPerBlock * WARP_SIZE), dynamicSharedSize, params, stream, DimGrid(mBlocksPerGrid), __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
		} \
		uint32_t getMaxGridSize() const { return mBlocksPerGrid; } \
	} APEX_CUDA_OBJ_NAME(name); \


#define APEX_CUDA_BOUND_KERNEL(config, name, argseq) \
	__APEX_CUDA_KERNEL_START(mManager->getDeviceTraits().mBlocksPerSM, config, name, argseq) \
			mMaxBlocksPerGrid = mManager->getDeviceTraits().mMaxBlocksPerGrid; \
			if (kernelConfig.maxGridSize != 0) \
			{ \
				mMaxBlocksPerGrid = PxMin(mMaxBlocksPerGrid, kernelConfig.maxGridSize); \
			} \
			if (kernelConfig.maxGridSizeMul != 0) \
			{ \
				const unsigned int maxGridSizeFromBlockDim = (fid.mWarpsPerBlock * WARP_SIZE * kernelConfig.maxGridSizeMul) / kernelConfig.maxGridSizeDiv; \
				PX_ASSERT(maxGridSizeFromBlockDim > 0); \
				mMaxBlocksPerGrid = PxMin(mMaxBlocksPerGrid, maxGridSizeFromBlockDim); \
			} \
	__APEX_CUDA_KERNEL_WARPS_END(name, argseq) \


#define APEX_CUDA_FREE_KERNEL(config, name, argseq) \
	__APEX_CUDA_KERNEL_START(mManager->getDeviceTraits().mBlocksPerSM, config, name, argseq) \
			mMaxBlocksPerGrid = UINT_MAX; \
	__APEX_CUDA_KERNEL_WARPS_END(name, argseq) \


#define APEX_CUDA_FREE_KERNEL_2D($config, name, argseq) \
	__APEX_CUDA_KERNEL_START(mManager->getDeviceTraits().mBlocksPerSM_2D, $config, name, argseq) \
		} \
	public: \
		void operator() ( CUstream stream, unsigned int _threadCountX, unsigned int _threadCountY, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			const FuncInstData& fid = getFuncInstData(); \
			DimBlock blockDim = (ApexKernelConfig $config).blockDim; \
			if (blockDim.x == 0) \
			{ \
				uint32_t threadsPerBlock = fid.mWarpsPerBlock * WARP_SIZE; \
				blockDim.x = PxMin(_threadCountX, threadsPerBlock); \
				threadsPerBlock /= blockDim.x; \
				blockDim.y = PxMin(_threadCountY, threadsPerBlock); \
			} \
			DimGrid gridDim; \
			gridDim.x = (_threadCountX + blockDim.x - 1) / blockDim.x; \
			gridDim.y = (_threadCountY + blockDim.y - 1) / blockDim.y; \
			ApexCudaFuncParams params; \
			launch1(fid, params, stream); \
			if (mCTContext)	mCTContext->setFreeKernel(_threadCountX, _threadCountY); \
			setParam(params, _threadCountX); \
			setParam(params, _threadCountY); \
			launch2(fid, blockDim, fid.mDynamicShared, params, stream, gridDim, __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
		} \
	} APEX_CUDA_OBJ_NAME(name); \


#define APEX_CUDA_FREE_KERNEL_3D($config, name, argseq) \
	__APEX_CUDA_KERNEL_START(mManager->getDeviceTraits().mBlocksPerSM_3D, $config, name, argseq) \
		} \
	public: \
		void operator() ( CUstream stream, unsigned int _threadCountX, unsigned int _threadCountY, unsigned int _threadCountZ, __APEX_CUDA_FUNC_$ARGS(argseq) ) \
		{ \
			const FuncInstData& fid = getFuncInstData(); \
			DimBlock blockDim = (ApexKernelConfig $config).blockDim; \
			if (blockDim.x == 0) \
			{ \
				uint32_t threadsPerBlock = fid.mWarpsPerBlock * WARP_SIZE; \
				blockDim.x = PxMin(_threadCountX, threadsPerBlock); \
				threadsPerBlock /= blockDim.x; \
				blockDim.y = PxMin(_threadCountY, threadsPerBlock); \
				threadsPerBlock /= blockDim.y; \
				blockDim.z = PxMin(_threadCountZ, threadsPerBlock); \
			} \
			const uint32_t blockCountX = (_threadCountX + blockDim.x - 1) / blockDim.x; \
			const uint32_t blockCountY = (_threadCountY + blockDim.y - 1) / blockDim.y; \
			const uint32_t blockCountZ = (_threadCountZ + blockDim.z - 1) / blockDim.z; \
			DimGrid gridDim(blockCountX, blockCountY * blockCountZ); \
			ApexCudaFuncParams params; \
			launch1(fid, params, stream); \
			if (mCTContext)	mCTContext->setFreeKernel(_threadCountX, _threadCountY, _threadCountZ, blockCountY); \
			setParam(params, _threadCountX); \
			setParam(params, _threadCountY); \
			setParam(params, _threadCountZ); \
			setParam(params, blockCountY); \
			launch2(fid, blockDim, fid.mDynamicShared, params, stream, gridDim, __APEX_CUDA_FUNC_$ARG_NAMES(argseq) ); \
		} \
	} APEX_CUDA_OBJ_NAME(name); \


#endif // #ifdef __CUDACC__

#endif //APEX_CUDA_H
