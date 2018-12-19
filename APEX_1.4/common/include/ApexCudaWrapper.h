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



#ifndef __APEX_CUDA_WRAPPER_H__
#define __APEX_CUDA_WRAPPER_H__

#include <cuda.h>
#include "ApexCutil.h"
#include "vector_types.h"
#include "ApexMirroredArray.h"
#include "InplaceStorage.h"
#include "PsMutex.h"
#include "ApexCudaTest.h"
#include "ApexCudaProfile.h"
#include "ApexCudaDefs.h"

namespace nvidia
{
namespace apex
{

struct DimGrid
{
	uint32_t x, y;

	DimGrid() {}
	DimGrid(uint32_t x, uint32_t y = 1)
	{
		this->x = x;
		this->y = y;
	}
};
struct DimBlock
{
	uint32_t x, y, z;

	DimBlock() {}
	DimBlock(uint32_t x, uint32_t y = 1, uint32_t z = 1)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
};

struct ApexKernelConfig
{
	uint32_t fixedSharedMemDWords;
	uint32_t sharedMemDWordsPerWarp;
	DimBlock blockDim;
	uint32_t maxGridSize;
	uint32_t maxGridSizeMul;
	uint32_t maxGridSizeDiv;

	ApexKernelConfig() { fixedSharedMemDWords = sharedMemDWordsPerWarp = 0; blockDim = DimBlock(0, 0, 0); maxGridSize = maxGridSizeMul = 0; maxGridSizeDiv = 1; }
	ApexKernelConfig(uint32_t fixedSharedMemDWords, uint32_t sharedMemDWordsPerWarp, int fixedWarpsPerBlock = 0, uint32_t maxGridSize = 0, uint32_t maxGridSizeMul = 0, uint32_t maxGridSizeDiv = 1)
	{
		this->fixedSharedMemDWords = fixedSharedMemDWords;
		this->sharedMemDWordsPerWarp = sharedMemDWordsPerWarp;
		this->blockDim = DimBlock(fixedWarpsPerBlock * WARP_SIZE);
		this->maxGridSize = maxGridSize;
		this->maxGridSizeMul = maxGridSizeMul;
		this->maxGridSizeDiv = maxGridSizeDiv;
		//final maxGridSize = min(SMcount, maxGridSize [if (maxGridSize != 0)], maxBlockSize * maxGridSizeMul / maxGridSizeDiv [if (maxGridSizeMul != 0)])
	}
	ApexKernelConfig(uint32_t fixedSharedMemDWords, uint32_t sharedMemDWordsPerWarp, const DimBlock& blockDim)
	{
		this->fixedSharedMemDWords = fixedSharedMemDWords;
		this->sharedMemDWordsPerWarp = sharedMemDWordsPerWarp;
		this->blockDim = blockDim;
		this->maxGridSize = 0;
		this->maxGridSizeMul = 0;
		this->maxGridSizeDiv = 1;
	}
};

struct ApexCudaMemRefBase
{
	typedef ApexCudaMemFlags::Enum Intent;

	const void*	ptr;
	size_t		size;	//size in bytes
	int32_t		offset;	//data offset for ptr
	Intent		intent;
	
	ApexCudaMemRefBase(const void* ptr, size_t byteSize, int32_t offset, Intent intent)
		: ptr(ptr), size(byteSize), offset(offset), intent(intent) {}
	virtual ~ApexCudaMemRefBase() {}
};

template <class T>
struct ApexCudaMemRef : public ApexCudaMemRefBase
{
	ApexCudaMemRef(T* ptr, size_t byteSize, Intent intent = ApexCudaMemFlags::IN_OUT) 
		: ApexCudaMemRefBase(ptr, byteSize, 0, intent) {}

	ApexCudaMemRef(T* ptr, size_t byteSize, int32_t offset, Intent intent) 
		: ApexCudaMemRefBase(ptr, byteSize, offset, intent) {}

	inline T* getPtr() const
	{
		return (T*)ptr;
	}

	virtual ~ApexCudaMemRef() {}
};

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(T* ptr, size_t size, ApexCudaMemRefBase::Intent intent = ApexCudaMemFlags::IN_OUT)
{
	return ApexCudaMemRef<T>(ptr, sizeof(T) * size, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(T* ptr, size_t size, int32_t offset, ApexCudaMemRefBase::Intent intent)
{
	return ApexCudaMemRef<T>(ptr, sizeof(T) * size, sizeof(T) * offset, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, ApexCudaMemRefBase::Intent intent = ApexCudaMemFlags::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), ma.getByteSize(), intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, size_t size, ApexCudaMemRefBase::Intent intent = ApexCudaMemFlags::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), sizeof(T) * size, intent);
}

template <class T>
inline ApexCudaMemRef<T> createApexCudaMemRef(const ApexMirroredArray<T>& ma, size_t size, int32_t offset, ApexCudaMemRefBase::Intent intent = ApexCudaMemFlags::IN_OUT)
{
	return ApexCudaMemRef<T>(ma.getGpuPtr(), sizeof(T) * size, sizeof(T) * offset, intent);
}

#ifndef ALIGN_OFFSET
#define ALIGN_OFFSET(offset, alignment) (offset) = ((offset) + (alignment) - 1) & ~((alignment) - 1)
#endif

#define CUDA_MAX_PARAM_SIZE		256


class ApexCudaTestKernelContext;


class ApexCudaConstStorage;

class ApexCudaModule
{
public:
	ApexCudaModule()
		: mCuModule(0), mStorage(0)
	{
	}

	PX_INLINE void init(const void* image)
	{
		if (mCuModule == 0)
		{
			CUT_SAFE_CALL(cuModuleLoadDataEx(&mCuModule, image, 0, NULL, NULL));
		}
	}
	PX_INLINE void release()
	{
		if (mCuModule != 0)
		{
			CUT_SAFE_CALL(cuModuleUnload(mCuModule));
			mCuModule = 0;
		}
	}

	PX_INLINE bool isValid() const
	{
		return (mCuModule != 0);
	}

	PX_INLINE CUmodule getCuModule() const
	{
		return mCuModule;
	}

	PX_INLINE ApexCudaConstStorage* getStorage() const
	{
		return mStorage;
	}

private:
	CUmodule mCuModule;
	ApexCudaConstStorage* mStorage;

	friend class ApexCudaConstStorage;
};

class ApexCudaObjManager;

class ApexCudaObj
{
	friend class ApexCudaObjManager;
	ApexCudaObj* mObjListNext;

protected:
	const char* mName;
	ApexCudaModule* mCudaModule;
	ApexCudaObjManager* mManager;

	ApexCudaObj(const char* name) : mObjListNext(0), mName(name), mCudaModule(NULL), mManager(NULL) {}
	virtual ~ApexCudaObj() {}

	PX_INLINE void init(ApexCudaObjManager* manager, ApexCudaModule* cudaModule);

public:
	const char* getName() const
	{
		return mName;
	}
	const ApexCudaModule* getCudaModule() const
	{
		return mCudaModule;
	}

	enum ApexCudaObjType
	{
		UNKNOWN,
		FUNCTION,
		TEXTURE,
		CONST_STORAGE,
		SURFACE
	};
	virtual ApexCudaObjType getType()
	{
		return UNKNOWN;
	}

	PX_INLINE ApexCudaObj* next()
	{
		return mObjListNext;
	}
	virtual void release() = 0;	
	virtual void formContext(ApexCudaTestKernelContext*) = 0;
};

struct ApexCudaDeviceTraits
{
	uint32_t mMaxSharedMemPerBlock;
	uint32_t mMaxSharedMemPerSM;
	uint32_t mMaxRegistersPerSM;
	uint32_t mMaxThreadsPerSM;

	uint32_t mBlocksPerSM;
	uint32_t mBlocksPerSM_2D;
	uint32_t mBlocksPerSM_3D;
	uint32_t mMaxBlocksPerGrid;
};

class ApexCudaObjManager
{
	ApexCudaObj* mObjListHead;

	Module*					mNxModule;
	ApexCudaTestManager*		mCudaTestManager;	
	PxGpuDispatcher*			mGpuDispatcher;

	ApexCudaDeviceTraits		mDeviceTraits;

protected:
	friend class ApexCudaFunc;
	ApexCudaProfileSession*		mCudaProfileSession;

public:
	ApexCudaObjManager() : mObjListHead(0), mNxModule(0), mCudaTestManager(0), mGpuDispatcher(0), mCudaProfileSession(0) {}

	void init(Module*	nxModule, ApexCudaTestManager* cudaTestManager, PxGpuDispatcher*	gpuDispatcher)
	{
		mNxModule = nxModule;
		mCudaTestManager = cudaTestManager;
		mGpuDispatcher = gpuDispatcher;

		//get device traits
		CUdevice device;
		CUT_SAFE_CALL(cuCtxGetDevice(&device));
		CUT_SAFE_CALL(cuDeviceGetAttribute((int*)&mDeviceTraits.mMaxSharedMemPerBlock, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK, device));
		CUT_SAFE_CALL(cuDeviceGetAttribute((int*)&mDeviceTraits.mMaxSharedMemPerSM, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_MULTIPROCESSOR, device));
		CUT_SAFE_CALL(cuDeviceGetAttribute((int*)&mDeviceTraits.mMaxRegistersPerSM, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR, device));
		CUT_SAFE_CALL(cuDeviceGetAttribute((int*)&mDeviceTraits.mMaxThreadsPerSM, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR, device));

#ifdef APEX_CUDA_FORCED_BLOCKS
		mDeviceTraits.mBlocksPerSM = (APEX_CUDA_FORCED_BLOCKS > 32) ? 2u : 1u;
		mDeviceTraits.mMaxBlocksPerGrid = APEX_CUDA_FORCED_BLOCKS;
#else
		int computeMajor;
		int smCount;
		CUT_SAFE_CALL(cuDeviceGetAttribute(&smCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, device));
		CUT_SAFE_CALL(cuDeviceGetAttribute(&computeMajor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device));

		mDeviceTraits.mBlocksPerSM = 2;//(computeMajor >= 5) ? 2u : 1u;
		mDeviceTraits.mMaxBlocksPerGrid = uint32_t(smCount) * mDeviceTraits.mBlocksPerSM;
#endif
		mDeviceTraits.mBlocksPerSM_2D = 4;
		mDeviceTraits.mBlocksPerSM_3D = 4;
	}

	PX_INLINE const ApexCudaDeviceTraits& getDeviceTraits() const
	{
		return mDeviceTraits;
	}

	PX_INLINE void addToObjList(ApexCudaObj* obj)
	{
		obj->mObjListNext = mObjListHead;
		mObjListHead = obj;
	}

	PX_INLINE ApexCudaObj* getObjListHead()
	{
		return mObjListHead;
	}

	void releaseAll()
	{
		for (ApexCudaObj* obj = mObjListHead; obj != 0; obj = obj->mObjListNext)
		{
			obj->release();
		}
	}

	PX_INLINE Module* getModule() const
	{
		return mNxModule;
	}
	PX_INLINE ApexCudaTestManager* getCudaTestManager() const
	{
		return mCudaTestManager;
	}
	PX_INLINE PxGpuDispatcher* getGpuDispatcher() const
	{
		return mGpuDispatcher;
	}

public:
	virtual void onBeforeLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream) = 0;
	virtual void onAfterLaunchApexCudaFunc(const ApexCudaFunc& func, CUstream stream) = 0;

};

PX_INLINE void ApexCudaObj::init(ApexCudaObjManager* manager, ApexCudaModule* cudaModule)
{
	mManager = manager;
	mManager->addToObjList(this);
	mCudaModule = cudaModule;
}


class ApexCudaTexRef : public ApexCudaObj
{
public:
	void init(ApexCudaObjManager* manager, CUtexref texRef, ApexCudaModule* cudaModule, CUarray_format format, int numChannels, int dim, int flags)
	{
		ApexCudaObj::init(manager, cudaModule);

		mTexRef = texRef;
		mDim = dim;
		mFormat = format;
		mNumChannels = numChannels;
		mFlags = flags;
		mIsBinded = false;

		CUT_SAFE_CALL(cuTexRefSetFilterMode(mTexRef, mFilterMode));

		for (int d = 0; d < dim; ++d)
		{
			CUT_SAFE_CALL(cuTexRefSetAddressMode(mTexRef, d, CU_TR_ADDRESS_MODE_CLAMP));
		}
	}

	ApexCudaTexRef(const char* name, CUfilter_mode filterMode = CU_TR_FILTER_MODE_POINT)
		: ApexCudaObj(name), mTexRef(0), mFilterMode(filterMode)
	{
	}

	void setNormalizedCoords()
	{
		mFlags |= CU_TRSF_NORMALIZED_COORDINATES;
	}

	void bindTo(const void* ptr, size_t bytes, size_t* retByteOffset = 0)
	{
		CUT_SAFE_CALL(cuTexRefSetFormat(mTexRef, mFormat, mNumChannels));
		CUT_SAFE_CALL(cuTexRefSetFlags(mTexRef, (uint32_t)mFlags));

		size_t byteOffset;
		CUT_SAFE_CALL(cuTexRefSetAddress(&byteOffset, mTexRef, CUT_TODEVICE(ptr), static_cast<unsigned int>(bytes)));

		if (retByteOffset != 0)
		{
			*retByteOffset = byteOffset;
		}
		else
		{
			PX_ASSERT(byteOffset == 0);
		}

		mBindedSize = bytes;
		mBindedPtr = ptr;
		mBindedArray = NULL;
		mIsBinded = true;
	}

	template <typename T>
	void bindTo(ApexMirroredArray<T>& mem, size_t* retByteOffset = 0)
	{
		bindTo(mem.getGpuPtr(), mem.getByteSize(), retByteOffset);
	}

	template <typename T>
	void bindTo(ApexMirroredArray<T>& mem, size_t size, size_t* retByteOffset = 0)
	{
		bindTo(mem.getGpuPtr(), sizeof(T) * size, retByteOffset);
	}

	void bindTo(CUarray cuArray)
	{
		CUT_SAFE_CALL(cuTexRefSetFlags(mTexRef, (uint32_t)mFlags));

		CUT_SAFE_CALL(cuTexRefSetArray(mTexRef, cuArray, CU_TRSA_OVERRIDE_FORMAT));

		mBindedSize = 0;
		mBindedPtr = NULL;
		mBindedArray = cuArray;
		mIsBinded = true;
	}

	void bindTo(const ApexCudaArray& cudaArray)
	{
		bindTo(cudaArray.getCuArray());
	}

	void unbind()
	{
		size_t byteOffset;
		CUT_SAFE_CALL(cuTexRefSetAddress(&byteOffset, mTexRef, CUdeviceptr(0), 0));
		mIsBinded = false;
	}
	
	virtual ApexCudaObjType getType()
	{
		return TEXTURE;
	}

	virtual void release() {}
	
	virtual void formContext(ApexCudaTestKernelContext* context) 
	{
		if (mIsBinded)
		{
			context->addTexRef(mName, mBindedPtr, mBindedSize, mBindedArray);
		}
	}

private:
	CUtexref	mTexRef;
	CUfilter_mode mFilterMode;

	CUarray_format mFormat;
	int			mNumChannels;
	int			mDim;
	int			mFlags;
	
	bool		mIsBinded;
	size_t		mBindedSize;
	const void*	mBindedPtr;
	CUarray		mBindedArray;
};


class ApexCudaSurfRef : public ApexCudaObj
{
public:
	void init(ApexCudaObjManager* manager, CUsurfref surfRef, ApexCudaModule* cudaModule)
	{
		ApexCudaObj::init(manager, cudaModule);

		mSurfRef = surfRef;

		mIsBinded = false;
	}

	ApexCudaSurfRef(const char* name) : ApexCudaObj(name), mSurfRef(0)
	{
	}

	void bindTo(CUarray cuArray, ApexCudaMemFlags::Enum flags)
	{
		CUDA_ARRAY3D_DESCRIPTOR desc;
		CUT_SAFE_CALL(cuArray3DGetDescriptor(&desc, cuArray));

		CUT_SAFE_CALL(cuSurfRefSetArray(mSurfRef, cuArray, 0));

		mIsBinded = true;
		mBindedArray = cuArray;
		mBindedFlags = flags;
	}

	void bindTo(const ApexCudaArray& cudaArray, ApexCudaMemFlags::Enum flags)
	{
		bindTo(cudaArray.getCuArray(), flags);
	}

	void unbind()
	{
		mIsBinded = false;
	}

	virtual ApexCudaObjType getType()
	{
		return SURFACE;
	}

	virtual void release() {}

	virtual void formContext(ApexCudaTestKernelContext* context)
	{
		if (mIsBinded)
		{
			context->addSurfRef(mName, mBindedArray, mBindedFlags);
		}
	}

private:
	CUsurfref   mSurfRef;

	bool		mIsBinded;
	CUarray		mBindedArray;
	ApexCudaMemFlags::Enum mBindedFlags;
};

class ApexCudaTexRefScopeBind
{
private:
	ApexCudaTexRefScopeBind& operator=(const ApexCudaTexRefScopeBind&);
	ApexCudaTexRef& mTexRef;

public:
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, void* ptr, size_t bytes, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(ptr, bytes, retByteOffset);
	}
	template <typename T>
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, ApexMirroredArray<T>& mem, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(mem, retByteOffset);
	}
	template <typename T>
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, ApexMirroredArray<T>& mem, size_t size, size_t* retByteOffset = 0)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(mem, size, retByteOffset);
	}
	ApexCudaTexRefScopeBind(ApexCudaTexRef& texRef, const ApexCudaArray& cudaArray)
		: mTexRef(texRef)
	{
		mTexRef.bindTo(cudaArray);
	}
	~ApexCudaTexRefScopeBind()
	{
		mTexRef.unbind();
	}
};

#define APEX_CUDA_TEXTURE_SCOPE_BIND(texRef, mem) ApexCudaTexRefScopeBind texRefScopeBind_##texRef (CUDA_OBJ(texRef), mem);
#define APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRef, mem, size) ApexCudaTexRefScopeBind texRefScopeBind_##texRef (CUDA_OBJ(texRef), mem, size);
#define APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRef, ptr, count) ApexCudaTexRefScopeBind texRefScopeBind_##texRef (CUDA_OBJ(texRef), ptr, sizeof(*ptr) * count);
#define APEX_CUDA_TEXTURE_BIND(texRef, mem) CUDA_OBJ(texRef).bindTo(mem);
#define APEX_CUDA_TEXTURE_BIND_PTR(texRef, ptr, count) CUDA_OBJ(texRef).bindTo(ptr, sizeof(*ptr) * count);
#define APEX_CUDA_TEXTURE_UNBIND(texRef) CUDA_OBJ(texRef).unbind();


class ApexCudaSurfRefScopeBind
{
private:
	ApexCudaSurfRefScopeBind& operator=(const ApexCudaSurfRefScopeBind&);
	ApexCudaSurfRef& mSurfRef;

public:
	ApexCudaSurfRefScopeBind(ApexCudaSurfRef& surfRef, ApexCudaArray& cudaArray, ApexCudaMemFlags::Enum flags)
		: mSurfRef(surfRef)
	{
		mSurfRef.bindTo(cudaArray, flags);
	}
	ApexCudaSurfRefScopeBind(ApexCudaSurfRef& surfRef, CUarray cuArray, ApexCudaMemFlags::Enum flags)
		: mSurfRef(surfRef)
	{
		mSurfRef.bindTo(cuArray, flags);
	}
	~ApexCudaSurfRefScopeBind()
	{
		mSurfRef.unbind();
	}
};

#define APEX_CUDA_SURFACE_SCOPE_BIND(surfRef, mem, flags) ApexCudaSurfRefScopeBind surfRefScopeBind_##surfRef (CUDA_OBJ(surfRef), mem, flags);
#define APEX_CUDA_SURFACE_BIND(surfRef, mem, flags) CUDA_OBJ(surfRef).bindTo(mem, flags);
#define APEX_CUDA_SURFACE_UNBIND(surfRef) CUDA_OBJ(surfRef).unbind();


class ApexCudaVar : public ApexCudaObj
{
public:
	size_t getSize() const
	{
		return mSize;
	}

	void init(ApexCudaObjManager* manager, ApexCudaModule* cudaModule, CUdeviceptr devPtr, size_t size, PxCudaContextManager* ctx)
	{
		ApexCudaObj::init(manager, cudaModule);

		mDevPtr = devPtr;
		mSize = size;
		init(manager, ctx);
	}

	virtual void release() {}
	virtual void formContext(ApexCudaTestKernelContext*) {}

protected:
	virtual void init(ApexCudaObjManager* , PxCudaContextManager*) = 0;

	ApexCudaVar(const char* name) : ApexCudaObj(name), mDevPtr(0), mSize(0)
	{
	}

protected:
	CUdeviceptr		mDevPtr;
	size_t			mSize;
};


class ApexCudaConstStorage : public ApexCudaVar, public InplaceStorage
{
public:
	ApexCudaConstStorage(const char* nameVar, const char* nameTexRef)
		: ApexCudaVar(nameVar), mCudaTexRef(nameTexRef), mStoreInTexture(false)
	{
		mStorageSize = 0;
		mStoragePtr = 0;

		mHostBuffer = 0;
		mDeviceBuffer = 0;
	}

	virtual ApexCudaObjType getType()
	{
		return CONST_STORAGE;
	}

	virtual void formContext(ApexCudaTestKernelContext* context) 
	{
		if (!mStoreInTexture && mHostBuffer != 0)
		{
			PX_ASSERT(mHostBuffer->getSize() >= ApexCudaVar::getSize());
			void* hostPtr = reinterpret_cast<void*>(mHostBuffer->getPtr());
			context->addConstMem(mName, hostPtr, ApexCudaVar::getSize());
		}
	}

	virtual void init(ApexCudaObjManager* manager, PxCudaContextManager* ctx)
	{
		PX_ASSERT(mCudaModule != 0);
		PX_ASSERT(mCudaModule->mStorage == 0);
		mCudaModule->mStorage = this;

		CUtexref cuTexRef;
		CUT_SAFE_CALL(cuModuleGetTexRef(&cuTexRef, mCudaModule->getCuModule(), mCudaTexRef.getName()));

		mCudaTexRef.init(manager, cuTexRef, mCudaModule, CU_AD_FORMAT_SIGNED_INT32, 1, 1, CU_TRSF_READ_AS_INTEGER);

		//prealloc. host buffer for Apex Cuda Test framework
		reallocHostBuffer(ctx, ApexCudaVar::getSize());
	}

	virtual void release()
	{
		InplaceStorage::release();

		if (mDeviceBuffer != 0)
		{
			mDeviceBuffer->free();
			mDeviceBuffer = 0;
		}
		if (mHostBuffer != 0)
		{
			mHostBuffer->free();
			mHostBuffer = 0;
		}

		if (mStoragePtr != 0)
		{
			getAllocator().deallocate(mStoragePtr);
			mStoragePtr = 0;
			mStorageSize = 0;
		}
	}

	bool copyToDevice(PxCudaContextManager* ctx, CUstream stream)
	{
		if (mStoragePtr == 0)
		{
			return false;
		}

		bool result = false;

		InplaceStorage* storage = static_cast<InplaceStorage*>(this);
		mMutex.lock();
		if (storage->isChanged())
		{
			if (!reallocHostBuffer(ctx, mStorageSize))
			{
				return false;
			}

			CUdeviceptr copyDevPtr = 0;
			if (mStoreInTexture)
			{
				if (mDeviceBuffer == 0)
				{
					mDeviceBuffer = ctx->getMemoryManager()->alloc(
						PxCudaBufferType(PxCudaBufferMemorySpace::T_GPU, PxCudaBufferFlags::F_READ_WRITE),
						mStorageSize);
					if (mDeviceBuffer == 0)
					{
						APEX_INTERNAL_ERROR("ApexCudaConstStorage failed to allocate GPU Memory!");
						return false;
					}
				}
				else if (mDeviceBuffer->getSize() < mStorageSize)
				{
					mDeviceBuffer->realloc(mStorageSize);
				}
				copyDevPtr = mDeviceBuffer->getPtr();
			}
			else
			{
				if (mDeviceBuffer != 0)
				{
					mDeviceBuffer->free();
					mDeviceBuffer = 0;
				}
				copyDevPtr = mDevPtr;
			}

			uint8_t* hostPtr = reinterpret_cast<uint8_t*>(mHostBuffer->getPtr());

			size_t size = storage->mapTo(hostPtr);
			// padding up to the next dword
			size = (size + 7) & ~7;
			if (size > mStorageSize) size = mStorageSize;

			CUT_SAFE_CALL(cuMemcpyHtoDAsync(copyDevPtr, hostPtr, size, stream));

			storage->setUnchanged();
			result = true;
		}
		mMutex.unlock();

		return result;
	}

	PX_INLINE bool getStoreInTexture() const
	{
		return mStoreInTexture;
	}

	PX_INLINE void onBeforeLaunch()
	{
		if (mStoreInTexture)
		{
			mCudaTexRef.bindTo( mDeviceBuffer ? reinterpret_cast<void*>(mDeviceBuffer->getPtr()) : 0, mStorageSize );
		}
	}

	PX_INLINE void onAfterLaunch()
	{
		if (mStoreInTexture)
		{
			mCudaTexRef.unbind();
		}
	}

protected:
	bool reallocHostBuffer(PxCudaContextManager* ctx, size_t size)
	{
		if (mHostBuffer == 0)
		{
			mHostBuffer = ctx->getMemoryManager()->alloc(
				PxCudaBufferType(PxCudaBufferMemorySpace::T_PINNED_HOST, PxCudaBufferFlags::F_READ_WRITE),
				size);
			if (mHostBuffer == 0)
			{
				APEX_INTERNAL_ERROR("ApexCudaConstStorage failed to allocate Pinned Host Memory!");
				return false;
			}
		}
		else if (mHostBuffer->getSize() < size)
		{
			mHostBuffer->realloc(size);
		}
		return true;
	}

	virtual uint8_t* storageResizeBuffer(uint32_t newSize)
	{
		if (!mStoreInTexture && newSize > ApexCudaVar::getSize())
		{
#if 0
			APEX_INTERNAL_ERROR("Out of CUDA constant memory");
			PX_ALWAYS_ASSERT();
			return 0;
#else
			//switch to texture
			mStoreInTexture = true;
#endif
		}
		else if (mStoreInTexture && newSize <= ApexCudaVar::getSize())
		{
			//switch back to const mem.
			mStoreInTexture = false;
		}

		const uint32_t PageSize = 4096;
		size_t allocSize = mStoreInTexture ? (newSize + (PageSize - 1)) & ~(PageSize - 1) : ApexCudaVar::getSize();

		if (allocSize > mStorageSize)
		{
			uint8_t* allocStoragePtr = static_cast<uint8_t*>(getAllocator().allocate(allocSize, "ApexCudaConstStorage", __FILE__, __LINE__));
			if (allocStoragePtr == 0)
			{
				APEX_INTERNAL_ERROR("ApexCudaConstStorage failed to allocate memory!");
				return 0;
			}
			if (mStoragePtr != 0)
			{
				memcpy(allocStoragePtr, mStoragePtr, mStorageSize);
				getAllocator().deallocate(mStoragePtr);
			}
			mStorageSize = allocSize;
			mStoragePtr = allocStoragePtr;
		}
		return mStoragePtr;
	}

	virtual void storageLock()
	{
		mMutex.lock();
	}
	virtual void storageUnlock()
	{
		mMutex.unlock();
	}

private:
	bool                    mStoreInTexture;
	ApexCudaTexRef          mCudaTexRef;

	size_t                  mStorageSize;
	uint8_t*            mStoragePtr;

	PxCudaBuffer*    mHostBuffer;
	PxCudaBuffer*    mDeviceBuffer;

	nvidia::Mutex            mMutex;

	friend class ApexCudaTestKernelContextReader;
};

typedef InplaceStorageGroup ApexCudaConstMemGroup;

#define APEX_CUDA_CONST_MEM_GROUP_SCOPE(group) INPLACE_STORAGE_GROUP_SCOPE(group)



struct ApexCudaFuncParams
{
	int		mOffset;
	char	mParams[CUDA_MAX_PARAM_SIZE];

	ApexCudaFuncParams() : mOffset(0) {}


};

class ApexCudaFunc : public ApexCudaObj
{
public:
	PX_INLINE bool testNameMatch(const char* name) const
	{
		if (const char* name$ = strrchr(name, '$'))
		{
			if (const char* name_ = strrchr(name, '_'))
			{
				return (nvidia::strncmp(name, mName, (uint32_t)(name_ - name)) == 0);
			}
		}
		return (nvidia::strcmp(name, mName) == 0);
	}

	void init(ApexCudaObjManager* manager, const char* name, CUfunction cuFunc, ApexCudaModule* cudaModule)
	{
		int funcInstIndex = 0;
		if (const char* name$ = strrchr(name, '$'))
		{
			funcInstIndex = atoi(name$ + 1);
		}
		if (funcInstIndex >= MAX_INST_COUNT)
		{
			PX_ALWAYS_ASSERT();
			return;
		}

		if (mFuncInstCount == 0)
		{
			ApexCudaObj::init(manager, cudaModule);
		}

		PxCudaContextManager* ctx = mManager->mGpuDispatcher->getCudaContextManager();
		{
			int funcMaxThreadsPerBlock;
			cuFuncGetAttribute(&funcMaxThreadsPerBlock, CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, cuFunc);

			int funcNumRegsPerThread;
			cuFuncGetAttribute(&funcNumRegsPerThread, CU_FUNC_ATTRIBUTE_NUM_REGS, cuFunc);

			int funcSharedMemSize;
			cuFuncGetAttribute(&funcSharedMemSize, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, cuFunc);
			const int sharedMemGranularity = (ctx->supportsArchSM20() ? 128 : 512) - 1;
			funcSharedMemSize = (funcSharedMemSize + sharedMemGranularity) & ~sharedMemGranularity;

			FuncInstData& fid = mFuncInstData[funcInstIndex];
			fid.mName = name;
			fid.mCuFunc = cuFunc;
			fid.mMaxThreadsPerBlock = (uint32_t)funcMaxThreadsPerBlock;

			fid.mNumRegsPerThread = (uint32_t)funcNumRegsPerThread;
			fid.mStaticSharedSize = (uint32_t)funcSharedMemSize;
			PX_ASSERT(fid.mStaticSharedSize <= mManager->getDeviceTraits().mMaxSharedMemPerBlock);

			fid.mWarpsPerBlock = 0;
			fid.mDynamicShared = 0;
		}

		init(ctx, funcInstIndex);
		mFuncInstCount = PxMax(mFuncInstCount, uint32_t(funcInstIndex) + 1);
	}

	virtual ApexCudaObjType getType()
	{
		return FUNCTION;
	}
	virtual void release() {}

	virtual void formContext(ApexCudaTestKernelContext*) {}

	/** This function force cuda stream syncronization that may slowdown application
	 */
	PX_INLINE void setProfileSession(ApexCudaProfileSession* cudaProfileSession)
	{
		mManager->mCudaProfileSession = cudaProfileSession;
		mProfileId = cudaProfileSession ? cudaProfileSession->getProfileId(mName, mManager->mNxModule->getName()) : 0;
	}

	PX_INLINE uint32_t getProfileId() const
	{
		return mProfileId;
	}

protected:
	static const int MAX_INST_COUNT = 2;

	struct FuncInstData
	{
		const char* mName;
		CUfunction  mCuFunc;

		uint32_t		mMaxThreadsPerBlock;
		uint32_t		mNumRegsPerThread;
		uint32_t		mStaticSharedSize;

		uint32_t		mWarpsPerBlock;
		uint32_t		mDynamicShared;
	};

	uint32_t			mFuncInstCount;
	FuncInstData	mFuncInstData[MAX_INST_COUNT];

	uint32_t			mProfileId;
	ApexCudaTestKernelContext*	mCTContext;

	ApexCudaFunc(const char* name) 
		: ApexCudaObj(name), mFuncInstCount(0), mProfileId(0), mCTContext(0)
	{
	}
	virtual void init(PxCudaContextManager* , int /*funcInstIndex*/) {}

	bool isValid() const
	{
		return (mFuncInstCount != 0) && (mCudaModule != 0);
	}

	const FuncInstData& getFuncInstData() const
	{
		PX_ASSERT(isValid());

		ApexCudaConstStorage* storage = mCudaModule->getStorage();
		if (storage != 0 && mFuncInstCount > 1)
		{
			PX_ASSERT(mFuncInstCount == 2);
			return mFuncInstData[ storage->getStoreInTexture() ? 1 : 0 ];
		}
		else
		{
			PX_ASSERT(mFuncInstCount == 1);
			return mFuncInstData[0];
		}
	}

	PX_INLINE void onBeforeLaunch(CUstream stream)
	{
		if (ApexCudaConstStorage* storage = mCudaModule->getStorage())
		{
			storage->onBeforeLaunch();
		}

		mManager->onBeforeLaunchApexCudaFunc(*this, stream);
	}
	PX_INLINE void onAfterLaunch(CUstream stream)
	{
		mManager->onAfterLaunchApexCudaFunc(*this, stream);

		if (ApexCudaConstStorage* storage = mCudaModule->getStorage())
		{
			storage->onAfterLaunch();
		}
	}

	template <typename T>
	void setParam(ApexCudaFuncParams& params, T* ptr)
	{
		ALIGN_OFFSET(params.mOffset, (int)__alignof(ptr));
		PX_ASSERT(params.mOffset + sizeof(ptr) <= CUDA_MAX_PARAM_SIZE);
		memcpy(params.mParams + params.mOffset, &ptr, sizeof(ptr));
		params.mOffset += sizeof(ptr);
		mCTContext = NULL;	// context can't catch pointers, use instead ApexCudaMemRef
	}

	template <typename T>
	void setParam(ApexCudaFuncParams& params, const ApexCudaMemRef<T>& memRef)
	{
		T* ptr = memRef.getPtr();
		ALIGN_OFFSET(params.mOffset, (int)__alignof(ptr));
		PX_ASSERT(params.mOffset + sizeof(ptr) <= CUDA_MAX_PARAM_SIZE);
		memcpy(params.mParams + params.mOffset, &ptr, sizeof(ptr));
		params.mOffset += sizeof(ptr);
	}

	template <typename T>
	void setParam(ApexCudaFuncParams& params, const T& val)
	{
		ALIGN_OFFSET(params.mOffset, (int)__alignof(val));
		PX_ASSERT(params.mOffset + sizeof(val) <= CUDA_MAX_PARAM_SIZE);
		memcpy(params.mParams + params.mOffset, (void*)&val, sizeof(val));
		params.mOffset += sizeof(val);
	}

	void resolveContext()
	{
		mCTContext->startObjList();
		ApexCudaObj* obj = mManager->getObjListHead();
		while(obj)
		{
			if ((CUmodule)obj->getCudaModule()->getCuModule() == mCudaModule->getCuModule())
			{
				obj->formContext(mCTContext);
			}
			obj = obj->next();
		}
		mCTContext->finishObjList();
	}

	template <typename T>
	void copyParam(const char* name, const ApexCudaMemRef<T>& memRef)
	{
		mCTContext->addParam(name, __alignof(void*), memRef.ptr, memRef.size, memRef.intent, memRef.offset);
	}

	template <typename T>
	void copyParam(const char* name, const T& val)
	{
		mCTContext->addParam(name, __alignof(val), (void*)&val, sizeof(val));
	}

private:
	template <typename T>
	void copyParam(const char* name, const ApexCudaMemRef<T>& memRef, uint32_t fpType)
	{
		mCTContext->addParam(name, __alignof(void*), memRef.ptr, memRef.size, memRef.intent, memRef.offset, fpType);
	}
	void setParam(ApexCudaFuncParams& params, unsigned align, unsigned size, void* ptr)
	{
		ALIGN_OFFSET(params.mOffset, (int)align);
		PX_ASSERT(params.mOffset + size <= CUDA_MAX_PARAM_SIZE);
		memcpy(params.mParams + params.mOffset, ptr, (uint32_t)size);
		params.mOffset += size;
	}	
	friend class ApexCudaTestKernelContextReader;
};

template <>
inline void ApexCudaFunc::copyParam<float>(const char* name, const ApexCudaMemRef<float>& memRef)
{
	copyParam(name, memRef, 4);
}

template <>
inline void ApexCudaFunc::copyParam<float2>(const char* name, const ApexCudaMemRef<float2>& memRef)
{
	copyParam(name, memRef, 4);
}

template <>
inline void ApexCudaFunc::copyParam<float3>(const char* name, const ApexCudaMemRef<float3>& memRef)
{
	copyParam(name, memRef, 4);
}

template <>
inline void ApexCudaFunc::copyParam<float4>(const char* name, const ApexCudaMemRef<float4>& memRef)
{
	copyParam(name, memRef, 4);
}

template <>
inline void ApexCudaFunc::copyParam<double>(const char* name, const ApexCudaMemRef<double>& memRef)
{
	copyParam(name, memRef, 8);
}


class ApexCudaTimer
{
public:
	ApexCudaTimer()
		:	mIsStarted(false)
		,	mIsFinished(false)
		,	mStart(NULL)
		,	mFinish(NULL)
	{
	}
	~ApexCudaTimer()
	{
		if (mStart != NULL)
		{
			CUT_SAFE_CALL(cuEventDestroy(mStart));
		}
		if (mFinish != NULL)
		{
			CUT_SAFE_CALL(cuEventDestroy(mFinish));
		}
	}
	void init()
	{
		if (mStart == NULL)
		{
			CUT_SAFE_CALL(cuEventCreate(&mStart, CU_EVENT_DEFAULT));
		}
		if (mFinish == NULL)
		{
			CUT_SAFE_CALL(cuEventCreate(&mFinish, CU_EVENT_DEFAULT));
		}
	}

	void onStart(CUstream stream)
	{
		if (mStart != NULL)
		{
			mIsStarted = true;
			CUT_SAFE_CALL(cuEventRecord(mStart, stream));
		}
	}
	void onFinish(CUstream stream)
	{
		if (mFinish != NULL && mIsStarted)
		{
			mIsFinished = true;
			CUT_SAFE_CALL(cuEventRecord(mFinish, stream));
		}
	}

	float getElapsedTime()
	{
		if (mIsStarted && mIsFinished)
		{
			mIsStarted = false;
			mIsFinished = false;
			CUT_SAFE_CALL(cuEventSynchronize(mStart));
			CUT_SAFE_CALL(cuEventSynchronize(mFinish));
			float time;
			CUT_SAFE_CALL(cuEventElapsedTime(&time, mStart, mFinish));
			return time;
		}
		else
		{
			return 0.0f;
		}
	}
private:
	CUevent mStart, mFinish;
	bool mIsStarted;
	bool mIsFinished;
};

}
} // end namespace nvidia::apex

#endif //__APEX_CUDA_WRAPPER_H__
