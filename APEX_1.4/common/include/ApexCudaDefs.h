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



#ifndef APEX_CUDA_DEFS_H
#define APEX_CUDA_DEFS_H

#include <cuda.h>

const unsigned int MAX_CONST_MEM_SIZE = 65536;

const unsigned int APEX_CUDA_MEM_ALIGNMENT = 256;
const unsigned int APEX_CUDA_TEX_MEM_ALIGNMENT = 512;

const unsigned int MAX_SMEM_BANKS = 32;


#define APEX_CUDA_ALIGN_UP(value, alignment) (((value) + (alignment)-1) & ~((alignment)-1))
#define APEX_CUDA_MEM_ALIGN_UP_32BIT(count) APEX_CUDA_ALIGN_UP(count, APEX_CUDA_MEM_ALIGNMENT >> 2)

const unsigned int LOG2_WARP_SIZE = 5;
const unsigned int WARP_SIZE = (1U << LOG2_WARP_SIZE);

//if you would like to make this value larger than 32 for future GPUs, 
//then you'll need to fix some kernels (like reduce and scan) to support more than 32 warps per block!!!
const unsigned int MAX_WARPS_PER_BLOCK = 32;
const unsigned int MAX_THREADS_PER_BLOCK = (MAX_WARPS_PER_BLOCK << LOG2_WARP_SIZE);

//uncomment this line to force bound kernels to use defined number of CTAs
//#define APEX_CUDA_FORCED_BLOCKS 80


namespace nvidia
{
namespace apex
{

struct ApexCudaMemFlags
{
	enum Enum
	{
		UNUSED = 0,
		IN = 0x01,
		OUT = 0x02,
		IN_OUT = IN | OUT
	};
};

#ifndef __CUDACC__

class ApexCudaArray : public UserAllocated
{
	PX_NOCOPY(ApexCudaArray)

	void init()
	{
		switch (mDesc.Format)
		{
		case CU_AD_FORMAT_UNSIGNED_INT8:
		case CU_AD_FORMAT_SIGNED_INT8:
			mElemSize = 1;
			break;
		case CU_AD_FORMAT_UNSIGNED_INT16:
		case CU_AD_FORMAT_SIGNED_INT16:
		case CU_AD_FORMAT_HALF:
			mElemSize = 2;
			break;
		case CU_AD_FORMAT_UNSIGNED_INT32:
		case CU_AD_FORMAT_SIGNED_INT32:
		case CU_AD_FORMAT_FLOAT:
			mElemSize = 4;
			break;
		default:
			PX_ALWAYS_ASSERT();
			mElemSize = 0;
			break;
		};
		mElemSize *= mDesc.NumChannels;
	}

public:
	ApexCudaArray() : mCuArray(NULL), mHasOwnership(false), mElemSize(0) {}
	~ApexCudaArray() { release(); }

	void assign(CUarray cuArray, bool bTakeOwnership)
	{
		release();

		mCuArray = cuArray;
		mHasOwnership = bTakeOwnership;
		CUT_SAFE_CALL(cuArray3DGetDescriptor(&mDesc, mCuArray));
		init();
	}

	void create(CUDA_ARRAY3D_DESCRIPTOR desc)
	{
		if (mCuArray != NULL && mHasOwnership && 
			mDesc.Width == desc.Width && mDesc.Height == desc.Height && mDesc.Depth == desc.Depth && 
			mDesc.Format == desc.Format && mDesc.NumChannels == desc.NumChannels && mDesc.Flags == desc.Flags)
		{
			return;
		}
		release();

		// Allocate CUDA 3d array in device memory
		mDesc = desc;
		CUT_SAFE_CALL(cuArray3DCreate(&mCuArray, &mDesc));
		mHasOwnership = true;
		init();
	}

	void create(CUarray_format format, unsigned int numChannels, unsigned int width, unsigned int height, unsigned int depth = 0, bool surfUsage = false)
	{
		CUDA_ARRAY3D_DESCRIPTOR desc;
		desc.Format = format;
		desc.NumChannels = numChannels;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		desc.Flags = surfUsage ? CUDA_ARRAY3D_SURFACE_LDST : 0u;

		create(desc);
	}

	void release()
	{
		if (mCuArray != NULL)
		{
			if (mHasOwnership)
			{
				CUT_SAFE_CALL(cuArrayDestroy(mCuArray));
			}
			mCuArray = NULL;
			mHasOwnership = false;
			mElemSize = 0;
		}
	}

	void copyToHost(CUstream stream, void* dstHost, size_t dstPitch = 0, size_t dstHeight = 0, 
		size_t copyWidth = 0, size_t copyHeight = 0, size_t copyDepth = 0)
	{
		if (mDesc.Width > 0)
		{
			if (mDesc.Height > 0)
			{
				if (mDesc.Depth > 0)
				{
					//3D
					CUDA_MEMCPY3D copyDesc;
					copyDesc.WidthInBytes = size_t(copyWidth ? copyWidth : mDesc.Width) * mElemSize;
					copyDesc.Height = copyHeight ? copyHeight : mDesc.Height;
					copyDesc.Depth = copyDepth ? copyDepth : mDesc.Depth;

					copyDesc.srcXInBytes = copyDesc.srcY = copyDesc.srcZ = copyDesc.srcLOD = 0;
					copyDesc.srcMemoryType = CU_MEMORYTYPE_ARRAY;
					copyDesc.srcArray = mCuArray;

					copyDesc.dstXInBytes = copyDesc.dstY = copyDesc.dstZ = copyDesc.dstLOD = 0;
					copyDesc.dstMemoryType = CU_MEMORYTYPE_HOST;
					copyDesc.dstHost = dstHost;
					copyDesc.dstPitch = (dstPitch > 0) ? dstPitch : copyDesc.WidthInBytes;
					copyDesc.dstHeight = (dstHeight > 0) ? dstHeight : copyDesc.Height;
					CUT_SAFE_CALL(cuMemcpy3DAsync(&copyDesc, stream));
				}
				else
				{
					//2D
					CUDA_MEMCPY2D copyDesc;
					copyDesc.WidthInBytes = size_t(copyWidth ? copyWidth : mDesc.Width) * mElemSize;
					copyDesc.Height = copyHeight ? copyHeight : mDesc.Height;

					copyDesc.srcXInBytes = copyDesc.srcY = 0;
					copyDesc.srcMemoryType = CU_MEMORYTYPE_ARRAY;
					copyDesc.srcArray = mCuArray;

					copyDesc.dstXInBytes = copyDesc.dstY = 0;
					copyDesc.dstMemoryType = CU_MEMORYTYPE_HOST;
					copyDesc.dstHost = dstHost;
					copyDesc.dstPitch = (dstPitch > 0) ? dstPitch : copyDesc.WidthInBytes;
					CUT_SAFE_CALL(cuMemcpy2DAsync(&copyDesc, stream));
				}
			}
			else
			{
				//1D
				CUT_SAFE_CALL(cuMemcpyAtoHAsync(dstHost, mCuArray, 0, size_t(copyWidth ? copyWidth : mDesc.Width) * mElemSize, stream));
			}
		}
	}

	void copyFromHost(CUstream stream, const void* srcHost, size_t srcPitch = 0, size_t srcHeight = 0)
	{
		if (mDesc.Width > 0)
		{
			if (mDesc.Height > 0)
			{
				if (mDesc.Depth > 0)
				{
					//3D
					CUDA_MEMCPY3D copyDesc;
					copyDesc.WidthInBytes = size_t(mDesc.Width) * mElemSize;
					copyDesc.Height = mDesc.Height;
					copyDesc.Depth = mDesc.Depth;

					copyDesc.srcXInBytes = copyDesc.srcY = copyDesc.srcZ = copyDesc.srcLOD = 0;
					copyDesc.srcMemoryType = CU_MEMORYTYPE_HOST;
					copyDesc.srcHost = srcHost;
					copyDesc.srcPitch = (srcPitch > 0) ? srcPitch : copyDesc.WidthInBytes;
					copyDesc.srcHeight = (srcHeight > 0) ? srcHeight : copyDesc.Height;

					copyDesc.dstXInBytes = copyDesc.dstY = copyDesc.dstZ = copyDesc.dstLOD = 0;
					copyDesc.dstMemoryType = CU_MEMORYTYPE_ARRAY;
					copyDesc.dstArray = mCuArray;

					CUT_SAFE_CALL(cuMemcpy3DAsync(&copyDesc, stream));
				}
				else
				{
					//2D
					CUDA_MEMCPY2D copyDesc;
					copyDesc.WidthInBytes = size_t(mDesc.Width) * mElemSize;
					copyDesc.Height = mDesc.Height;

					copyDesc.srcXInBytes = copyDesc.srcY = 0;
					copyDesc.srcMemoryType = CU_MEMORYTYPE_HOST;
					copyDesc.srcHost = srcHost;
					copyDesc.srcPitch = (srcPitch > 0) ? srcPitch : copyDesc.WidthInBytes;

					copyDesc.dstXInBytes = copyDesc.dstY = 0;
					copyDesc.dstMemoryType = CU_MEMORYTYPE_ARRAY;
					copyDesc.dstArray = mCuArray;

					CUT_SAFE_CALL(cuMemcpy2DAsync(&copyDesc, stream));
				}
			}
			else
			{
				//1D
				CUT_SAFE_CALL(cuMemcpyHtoAAsync(mCuArray, 0, srcHost, size_t(mDesc.Width) * mElemSize, stream));
			}
		}
	}

	void copyToArray(CUstream stream, CUarray dstArray)
	{
		//copy array to array
		CUDA_MEMCPY3D desc;
		desc.srcXInBytes = desc.srcY = desc.srcZ = desc.srcLOD = 0;
		desc.srcMemoryType = CU_MEMORYTYPE_ARRAY;
		desc.srcArray = mCuArray;

		desc.dstXInBytes = desc.dstY = desc.dstZ = desc.dstLOD = 0;
		desc.dstMemoryType = CU_MEMORYTYPE_ARRAY;
		desc.dstArray = dstArray;

		desc.WidthInBytes = size_t(mDesc.Width) * mElemSize;
		desc.Height = mDesc.Height;
		desc.Depth = mDesc.Depth;
		CUT_SAFE_CALL(cuMemcpy3DAsync(&desc, stream));
	}

	PX_INLINE CUarray getCuArray() const
	{
		return mCuArray;
	}
	PX_INLINE bool isValid() const
	{
		return (mCuArray != NULL);
	}

	PX_INLINE unsigned int	getWidth() const  { return (unsigned int)mDesc.Width; }
	PX_INLINE unsigned int	getHeight() const { return (unsigned int)mDesc.Height; }
	PX_INLINE unsigned int	getDepth() const  { return (unsigned int)mDesc.Depth; }
	PX_INLINE CUarray_format	getFormat() const { return mDesc.Format; }
	PX_INLINE unsigned int	getNumChannels() const { return mDesc.NumChannels; }

	PX_INLINE bool			hasOwnership() const { return mHasOwnership; }

	PX_INLINE const CUDA_ARRAY3D_DESCRIPTOR& getDesc() const { return mDesc; }

	PX_INLINE size_t		getByteSize() const
	{
		size_t size = mDesc.Width * mElemSize;
		if (mDesc.Height > 0) size *= mDesc.Height;
		if (mDesc.Depth > 0) size *= mDesc.Depth;
		return size;
	}

private:
	CUarray					mCuArray;
	bool					mHasOwnership;
	unsigned int			mElemSize;
	CUDA_ARRAY3D_DESCRIPTOR	mDesc;
};

#endif //__CUDACC__

}
} // end namespace nvidia::apex

#endif //APEX_CUDA_DEFS_H
