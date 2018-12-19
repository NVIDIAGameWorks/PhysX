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



#ifndef APEX_MIRRORED_H
#define APEX_MIRRORED_H

#include "ApexDefs.h"

#include "Apex.h"
#include "ApexCutil.h"
#include "SceneIntl.h"

#include "PxTaskManager.h"
#include "PxGpuDispatcher.h"

#if defined(__CUDACC__)
#error "Mirrored arrays should not be visible to CUDA code.  Send device pointers to CUDA kernels."
#endif


#if APEX_CUDA_SUPPORT

#include "PxGpuCopyDesc.h"
#include "PxGpuCopyDescQueue.h"
#include "PxCudaContextManager.h"
#include "PxCudaMemoryManager.h"
 //#include <cuda.h>
#else

#define PX_ALLOC_INFO(name, ID) __FILE__, __LINE__, name, physx::PxAllocId::ID
#define PX_ALLOC_INFO_PARAMS_DECL(p0, p1, p2, p3)  const char* file = p0, int line = p1, const char* allocName = p2, physx::PxAllocId::Enum allocId = physx::PxAllocId::p3
#define PX_ALLOC_INFO_PARAMS_DEF()  const char* file, int line, const char* allocName, physx::PxAllocId::Enum allocId
#define PX_ALLOC_INFO_PARAMS_INPUT()  file, line, allocName, allocId
#define PX_ALLOC_INFO_PARAMS_INPUT_INFO(info) info.getFileName(), info.getLine(), info.getAllocName(), info.getAllocId()

namespace physx
{

struct PxAllocId
{
	/**
	* \brief ID of the Feature which owns/allocated memory from the heap
	*/
	enum Enum
	{
		UNASSIGNED,		//!< default
		APEX,			//!< APEX stuff not further classified
		PARTICLES,		//!< all particle related
		GPU_UTIL,		//!< e.g. RadixSort (used in SPH and deformable self collision)
		CLOTH,			//!< all cloth related
		NUM_IDS			//!< number of IDs, be aware that ApexHeapStats contains PxAllocIdStats[NUM_IDS]
	};
};

/// \brief class to track allocation statistics, see PxgMirrored
class PxAllocInfo
{
public:
	/**
	* \brief AllocInfo default constructor
	*/
	PxAllocInfo() {}

	/**
	* \brief AllocInfo constructor that initializes all of the members
	*/
	PxAllocInfo(const char* file, int line, const char* allocName, PxAllocId::Enum allocId)
		: mFileName(file)
		, mLine(line)
		, mAllocName(allocName)
		, mAllocId(allocId)
	{
	}

	/// \brief get the allocation file name
	inline	const char*			getFileName() const
	{
		return mFileName;
	}

	/// \brief get the allocation line
	inline	int					getLine() const
	{
		return mLine;
	}

	/// \brief get the allocation name
	inline	const char*			getAllocName() const
	{
		return mAllocName;
	}

	/// \brief get the allocation ID
	inline	PxAllocId::Enum		getAllocId() const
	{
		return mAllocId;
	}

private:
	const char*			mFileName;
	int					mLine;
	const char*			mAllocName;
	PxAllocId::Enum		mAllocId;
};

}

#endif

namespace nvidia
{
namespace apex
{

struct ApexMirroredPlace
{
	enum Enum
	{
		DEFAULT = 0,
		CPU     = 0x01,
#if APEX_CUDA_SUPPORT
		GPU     = 0x02,
		CPU_GPU = (CPU | GPU),
#endif
	};
};


template <class T>
class ApexMirrored
{
	PX_NOCOPY(ApexMirrored);

public:
	ApexMirrored(SceneIntl& scene, PX_ALLOC_INFO_PARAMS_DECL(NULL, 0, NULL, UNASSIGNED))
		: mCpuPtr(0)
		, mByteCount(0)
		, mPlace(ApexMirroredPlace::CPU)
		, mAllocInfo(PX_ALLOC_INFO_PARAMS_INPUT())
#if APEX_CUDA_SUPPORT
		, mCpuBuffer(NULL)
		, mGpuPtr(0)
		, mGpuBuffer(NULL)
#endif
	{
		PX_UNUSED(scene);
#if APEX_CUDA_SUPPORT
		PxGpuDispatcher* gd = scene.getTaskManager()->getGpuDispatcher();
		if (gd)
		{
			mCtx = gd->getCudaContextManager();
		}
		else
		{
			mCtx = NULL;
			return;
		}
#endif
	};

	~ApexMirrored()
	{
	}

	//Operators for accessing the data pointed to on the host. Using these operators is guaranteed
	//to maintain the class invariants.  Note that these operators are only ever called on the host.
	//The GPU never sees this class as instances are converted to regular pointers upon kernel
	//invocation.

	PX_INLINE T& operator*()
	{
		return *getCpuPtr();
	}

	PX_INLINE const T& operator*() const
	{
		return *getCpuPtr();
	}

	PX_INLINE T* operator->()
	{
		return getCpuPtr();
	}

	PX_INLINE const T* operator->() const
	{
		return getCpuPtr();
	}

	PX_INLINE T& operator[](unsigned int i)
	{
		return getCpuPtr()[i];
	}

	//Methods for converting the pointer to a regular pointer for use on
	//the CPU After a pointer has been obtained with these methods, the
	//data can be accessed multiple times with no extra cost. This is the
	//fastest method for accessing the data on the cpu.

	PX_INLINE T* getCpuPtr() const
	{
		return mCpuPtr;
	}

	/*!
	\return
	returns whether CPU buffer has been allocated for this array
	*/
	PX_INLINE bool cpuPtrIsValid() const
	{
		return mCpuPtr != 0;
	}

	PX_INLINE size_t* getCpuHandle() const
	{
		return reinterpret_cast<size_t*>(&mCpuPtr);
	}

	PX_INLINE size_t getByteSize() const
	{
		return mByteCount;
	}

#if APEX_CUDA_SUPPORT
	/*!
	\return
	returns whether GPU buffer has been allocated for this array
	*/
	PX_INLINE bool gpuPtrIsValid() const
	{
		return mGpuPtr != 0;
	}

	PX_INLINE T* getGpuPtr() const
	{
		return mGpuPtr;
	}

	/*!
	Get opaque handle to the underlying gpu or cpu memory These must not
	be cast to a pointer or derefernced, they should only be used to
	identify the memory region to the allocator
	*/
	PX_INLINE size_t* getGpuHandle() const
	{
		return reinterpret_cast<size_t*>(&mGpuPtr);
	}

	PX_INLINE void copyDeviceToHostDesc(PxGpuCopyDesc& desc, size_t byteSize, size_t byteOffset) const
	{
		PX_ASSERT(mCpuPtr && mGpuPtr && mByteCount);
		desc.type = PxGpuCopyDesc::DeviceToHost;
		desc.bytes = byteSize;
		desc.source = ((size_t) mGpuPtr) + byteOffset;
		desc.dest = ((size_t) mCpuPtr) + byteOffset;
	}

	PX_INLINE void copyHostToDeviceDesc(PxGpuCopyDesc& desc, size_t byteSize, size_t byteOffset) const
	{
		PX_ASSERT(mCpuPtr && mGpuPtr && mByteCount);
		desc.type = PxGpuCopyDesc::HostToDevice;
		desc.bytes = byteSize;
		desc.source = ((size_t) mCpuPtr) + byteOffset;
		desc.dest = ((size_t) mGpuPtr) + byteOffset;
	}

	PX_INLINE void mallocGpu(size_t byteSize)
	{
		PxCudaBufferType bufferType(PxCudaBufferMemorySpace::T_GPU, PxCudaBufferFlags::F_READ_WRITE);
		PxCudaBuffer* buffer = mCtx->getMemoryManager()->alloc(bufferType, (uint32_t)byteSize);
		if (buffer)
		{
			// in case of realloc
			if (mGpuBuffer)
			{
				mGpuBuffer->free();
			}
			mGpuBuffer = buffer;
			mGpuPtr = reinterpret_cast<T*>(mGpuBuffer->getPtr());
			PX_ASSERT(mGpuPtr);
		}
		else
		{
			PX_ASSERT(!"Out of GPU Memory!");
		}
	}

	PX_INLINE void freeGpu()
	{
		if (mGpuBuffer)
		{
			bool success = mGpuBuffer->free();
			mGpuBuffer = NULL;
			mGpuPtr = NULL;
			PX_UNUSED(success);
			PX_ASSERT(success);
		}
	}

	PX_INLINE void mallocHost(size_t byteSize)
	{
		PxCudaBufferType bufferType(PxCudaBufferMemorySpace::T_PINNED_HOST, PxCudaBufferFlags::F_READ_WRITE);
		PxCudaBuffer* buffer = mCtx->getMemoryManager()->alloc(bufferType, (uint32_t)byteSize);
		if (buffer)
		{
			// in case of realloc
			if (mCpuBuffer)
			{
				mCpuBuffer->free();
			}
			mCpuBuffer = buffer;
			mCpuPtr = reinterpret_cast<T*>(mCpuBuffer->getPtr());
			PX_ASSERT(mCpuPtr);
		}
		else
		{
			PX_ASSERT(!"Out of Pinned Host Memory!");
		}
	}
	PX_INLINE void freeHost()
	{
		if (mCpuBuffer)
		{
			bool success = mCpuBuffer->free();
			mCpuBuffer = NULL;
			mCpuPtr = NULL;
			PX_UNUSED(success);
			PX_ASSERT(success);
		}
	}
	PX_INLINE void swapGpuPtr(ApexMirrored<T>& other)
	{
		nvidia::swap(mGpuPtr, other.mGpuPtr);
		nvidia::swap(mGpuBuffer, other.mGpuBuffer);
	}
#endif

	PX_INLINE const PxAllocInfo&	getAllocInfo() const
	{
		return  mAllocInfo;
	}

	PX_INLINE void mallocCpu(size_t byteSize)
	{
		mCpuPtr = (T*)getAllocator().allocate(byteSize, mAllocInfo.getAllocName(), mAllocInfo.getFileName(), mAllocInfo.getLine());
		PX_ASSERT(mCpuPtr && "Out of CPU Memory!");
	}
	PX_INLINE void freeCpu()
	{
		if (mCpuPtr)
		{
			getAllocator().deallocate(mCpuPtr);
			mCpuPtr = NULL;
		}
	}


	PX_INLINE const char* getName() const
	{
		return  mAllocInfo.getAllocName();
	}

	void realloc(size_t byteCount, ApexMirroredPlace::Enum place)
	{
		ApexMirroredPlace::Enum oldPlace = mPlace;
		ApexMirroredPlace::Enum newPlace = (place != ApexMirroredPlace::DEFAULT) ? place : oldPlace;
		if (oldPlace == newPlace && byteCount <= mByteCount)
		{
			return;
		}

		size_t newSize = PxMax(byteCount, mByteCount);

#if APEX_CUDA_SUPPORT
		if (oldPlace != ApexMirroredPlace::CPU && newPlace != ApexMirroredPlace::CPU)
		{
			PX_ASSERT(oldPlace != ApexMirroredPlace::CPU);
			PX_ASSERT(newPlace != ApexMirroredPlace::CPU);

			if ((mCpuPtr != NULL && byteCount > mByteCount) ||
			        (mCpuPtr == NULL && (place & ApexMirroredPlace::CPU) != 0))
			{
				PxCudaBuffer* oldCpuBuffer = mCpuBuffer;
				T* oldCpuPtr = mCpuPtr;

				mCpuBuffer = NULL;

				mallocHost(newSize);

				PxCudaBuffer* newCpuBuffer = mCpuBuffer;
				T* newCpuPtr = mCpuPtr;


				if (oldCpuPtr != NULL && newCpuPtr != NULL && mByteCount > 0)
				{
					memcpy(mCpuPtr, oldCpuPtr, mByteCount);
				}

				mCpuBuffer = oldCpuBuffer;
				mCpuPtr = newCpuPtr;

				freeHost();

				mCpuBuffer = newCpuBuffer;
				mCpuPtr = newCpuPtr;
			}
			if ((mGpuPtr != NULL && byteCount > mByteCount) ||
			        (mGpuPtr == NULL && (place & ApexMirroredPlace::GPU) != 0))
			{
				// we explicitly do not move old data to the new buffer

				freeGpu();
				mallocGpu(newSize);
			}
		}
		else
#endif
		{
			T* oldCpuPtr = mCpuPtr;
#if APEX_CUDA_SUPPORT
			if (newPlace != ApexMirroredPlace::CPU)
			{
				if (newPlace == ApexMirroredPlace::CPU_GPU)
				{
					mallocHost(newSize);
				}
				else
				{
					mCpuPtr = NULL;
				}
				mallocGpu(newSize);
			}
			else
#endif
			{
				mallocCpu(newSize);
			}
			T* newCpuPtr = mCpuPtr;

			if (oldCpuPtr != NULL && newCpuPtr != NULL && mByteCount > 0)
			{
				memcpy(newCpuPtr, oldCpuPtr, mByteCount);
			}

			mCpuPtr = oldCpuPtr;
#if APEX_CUDA_SUPPORT
			if (oldPlace != ApexMirroredPlace::CPU)
			{
				if (oldPlace == ApexMirroredPlace::CPU_GPU)
				{
					freeHost();
				}
				freeGpu();
			}
			else
#endif
			{
				freeCpu();
			}
			mCpuPtr = newCpuPtr;
		}
		mByteCount = newSize;
		mPlace = newPlace;
	}

	void free()
	{
		PX_ASSERT(mPlace != ApexMirroredPlace::DEFAULT);
#if APEX_CUDA_SUPPORT
		if (mPlace != ApexMirroredPlace::CPU)
		{
			freeHost();
			freeGpu();
		}
		else
#endif
		{
			freeCpu();
		}
		mByteCount = 0;
	}

private:
	mutable T*                  mCpuPtr;
	size_t	                    mByteCount;

	ApexMirroredPlace::Enum     mPlace;
	PxAllocInfo    mAllocInfo;

#if APEX_CUDA_SUPPORT
	mutable PxCudaBuffer*   mCpuBuffer;
	mutable T*                             mGpuPtr;
	mutable PxCudaBuffer*   mGpuBuffer;
	PxCudaContextManager*     mCtx;
#endif
};


}
} // end namespace nvidia::apex

#endif
