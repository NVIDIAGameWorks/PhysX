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



#ifndef RENDER_API_INTL_H
#define RENDER_API_INTL_H

#include "PsArray.h"
#include "PsMutex.h"
#include "UserRenderCallback.h"
#if APEX_CUDA_SUPPORT
#include <cuda.h>
#endif

namespace nvidia
{
namespace apex
{

struct RenderEntityMapStateIntl
{
	enum Enum
	{
		UNMAPPED = 0,
		PENDING_MAP,
		MAPPED,
		PENDING_UNMAP
	};
};

class RenderStorageStateIntl
{
public:
	PX_INLINE bool isMapped() const { return (mMapFlags & MAPPED) != 0; }

	virtual bool map(UserRenderStorage* renderStorage, RenderMapType::Enum mapType) = 0;
	virtual void unmap(UserRenderStorage* renderStorage) = 0;

#if APEX_CUDA_SUPPORT
	PX_INLINE bool isCudaMapped() const { return (mMapFlags & CUDA_MAPPED) != 0; }
	PX_INLINE void setCudaMapped() { mMapFlags |= CUDA_MAPPED; }
	PX_INLINE void resetCudaMapped() { mMapFlags &= ~CUDA_MAPPED; }

	virtual CUgraphicsResource getCudaHandle(UserRenderStorage* renderStorage) = 0;
	virtual bool getCudaMapppedResult(UserRenderStorage* renderStorage) = 0;

	PX_INLINE bool checkCudaHandle() const { return mInteropHandle != NULL; }
	PX_INLINE void resetCudaHandle() { mInteropHandle = NULL; }
#endif

protected:
	RenderStorageStateIntl()
		: mMapFlags(0)
#if APEX_CUDA_SUPPORT
		, mInteropHandle(NULL)
#endif
	{
	}

	enum MapFlag
	{
		MAPPED = 0x01,
#if APEX_CUDA_SUPPORT
		CUDA_MAPPED = 0x02,
#endif
	};
	uint32_t mMapFlags;

#if APEX_CUDA_SUPPORT
	CUgraphicsResource mInteropHandle;
#endif
};

class RenderBufferStateIntl : public RenderStorageStateIntl
{
public:
	RenderBufferStateIntl()
		: mMappedPtr(NULL)
#if APEX_CUDA_SUPPORT
		, mMappedCudaPtr(NULL)
#endif
		, mMapOffset(0)
		, mMapSize(SIZE_MAX)
	{
	}

	PX_INLINE void* getMappedPtr() const
	{
		return mMappedPtr;
	}

	PX_INLINE void setMapRange(size_t offset, size_t size)
	{
		mMapOffset = offset;
		mMapSize = size;
	}

	virtual bool map(UserRenderStorage* renderStorage, RenderMapType::Enum mapType)
	{
		if ((mMapFlags & MAPPED) == 0)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::BUFFER);
			UserRenderBuffer* renderBuffer = static_cast<UserRenderBuffer*>(renderStorage);
			mMappedPtr = renderBuffer->map(mapType, mMapOffset, mMapSize);
			if (mMappedPtr != NULL)
			{
				mMapFlags += MAPPED;
			}
		}
		return (mMapFlags & MAPPED) != 0;
	}
	virtual void unmap(UserRenderStorage* renderStorage)
	{
		if ((mMapFlags & MAPPED) != 0)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::BUFFER);
			UserRenderBuffer* renderBuffer = static_cast<UserRenderBuffer*>(renderStorage);
			renderBuffer->unmap();
			mMappedPtr = NULL;
			mMapFlags -= MAPPED;
			//reset map range
			mMapOffset = 0;
			mMapSize = SIZE_MAX;
		}
	}

#if APEX_CUDA_SUPPORT
	PX_INLINE CUdeviceptr getMappedCudaPtr() const
	{
		return mMappedCudaPtr;
	}

	virtual CUgraphicsResource getCudaHandle(UserRenderStorage* renderStorage)
	{
		if (mInteropHandle == NULL)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::BUFFER);
			UserRenderBuffer* renderBuffer = static_cast<UserRenderBuffer*>(renderStorage);
			renderBuffer->getCUDAgraphicsResource(mInteropHandle);
		}
		return mInteropHandle;
	}

	virtual bool getCudaMapppedResult(UserRenderStorage* renderStorage)
	{
		PX_UNUSED(renderStorage);
		if (mInteropHandle != NULL)
		{
			size_t size = 0;
			if (cuGraphicsResourceGetMappedPointer(&mMappedCudaPtr, &size, mInteropHandle) == CUDA_SUCCESS)
			{
				return true;
			}
			mMappedCudaPtr = NULL;
		}
		return false;
	}
#endif

private:
	void*	mMappedPtr;
#if APEX_CUDA_SUPPORT
	CUdeviceptr	mMappedCudaPtr;
#endif
	size_t	mMapOffset;
	size_t	mMapSize;
};

class RenderSurfaceStateIntl : public RenderStorageStateIntl
{
public:
	RenderSurfaceStateIntl()
#if APEX_CUDA_SUPPORT
		: mMappedCudaArray(NULL)
#endif
	{
	}

	PX_INLINE const UserRenderSurface::MappedInfo& getMappedInfo() const
	{
		return mMappedInfo;
	}

	virtual bool map(UserRenderStorage* renderStorage, RenderMapType::Enum mapType)
	{
		if ((mMapFlags & MAPPED) == 0)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::SURFACE);
			UserRenderSurface* renderSurface = static_cast<UserRenderSurface*>(renderStorage);
			if (renderSurface->map(mapType, mMappedInfo))
			{
				mMapFlags += MAPPED;
			}
		}
		return (mMapFlags & MAPPED) != 0;
	}
	virtual void unmap(UserRenderStorage* renderStorage)
	{
		if ((mMapFlags & MAPPED) != 0)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::SURFACE);
			UserRenderSurface* renderSurface = static_cast<UserRenderSurface*>(renderStorage);
			renderSurface->unmap();
			mMapFlags -= MAPPED;
		}
	}
#if APEX_CUDA_SUPPORT
	PX_INLINE CUarray getMappedCudaArray() const
	{
		return mMappedCudaArray;
	}

	virtual CUgraphicsResource getCudaHandle(UserRenderStorage* renderStorage)
	{
		if (mInteropHandle == NULL)
		{
			PX_ASSERT(renderStorage->getType() == UserRenderStorage::SURFACE);
			UserRenderSurface* renderSurface = static_cast<UserRenderSurface*>(renderStorage);
			renderSurface->getCUDAgraphicsResource(mInteropHandle);
		}
		return mInteropHandle;
	}

	virtual bool getCudaMapppedResult(UserRenderStorage* renderStorage)
	{
		PX_UNUSED(renderStorage);
		if (mInteropHandle != NULL)
		{
			//TODO: cuGraphicsSubResourceGetMappedArray arrayIndex & mipLevel???
			if (cuGraphicsSubResourceGetMappedArray(&mMappedCudaArray, mInteropHandle, 0, 0) == CUDA_SUCCESS)
			{
				return true;
			}
			mMappedCudaArray = NULL;
		}
		return false;
	}
#endif

private:
	UserRenderSurface::MappedInfo	mMappedInfo;
#if APEX_CUDA_SUPPORT
	CUarray							mMappedCudaArray;
#endif
};

class RenderEntityIntl : public UserAllocated
{
public:
	virtual ~RenderEntityIntl() { PX_ASSERT(mRefCount == 0); }

	virtual void free() = 0;


	virtual void map() = 0;
	virtual void unmap() = 0;

#if APEX_CUDA_SUPPORT
	virtual void fillMapUnmapArraysForInterop(nvidia::Array<CUgraphicsResource> &toMapArray, nvidia::Array<CUgraphicsResource> &toUnmapArray) = 0;
	virtual void mapBufferResultsForInterop(bool mapSuccess, bool unmapSuccess) = 0;
#endif

	//
	virtual void				release()
	{
		bool triggerDelete = false;
		mRefCountLock.lock();
		if (mRefCount > 0)
		{
			triggerDelete = (--mRefCount) == 0;
		}
		mRefCountLock.unlock();
		if (triggerDelete)
		{
			destroy();
		}
	}

	// Returns this if successful, NULL otherwise
	RenderEntityIntl*		incrementReferenceCount()
	{
		RenderEntityIntl* returnValue = NULL;
		mRefCountLock.lock();
		if (mRefCount > 0)
		{
			++mRefCount;
			returnValue = this;
		}
		mRefCountLock.unlock();
		return returnValue;
	}

	PX_INLINE int32_t			getReferenceCount() const
	{
		return mRefCount;
	}

protected:
	RenderEntityIntl()
		: mRefCount(1)	// Ref count initialized to 1, assuming that whoever calls this constructor will store a reference
	{
	}

	virtual void destroy()
	{
		PX_DELETE(this);
	}

	volatile int32_t		mRefCount;
	physx::shdfnd::Mutex	mRefCountLock;


private:
	RenderEntityIntl& operator=(const RenderEntityIntl&);
};

template <typename Impl, typename Base>
class RenderEntityIntlImpl : public Base
{
protected:
	PX_INLINE const Impl* getImpl() const { return static_cast<const Impl*>(this); }
	PX_INLINE Impl* getImpl() { return static_cast<Impl*>(this); }

public:
	virtual ~RenderEntityIntlImpl()
	{
		getImpl()->freeAllRenderStorage();
	}


	virtual void free()
	{
		getImpl()->freeAllRenderStorage();
		mMapState = RenderEntityMapStateIntl::UNMAPPED;
	}

	virtual void map()
	{
		if (mMapState == RenderEntityMapStateIntl::UNMAPPED)
		{
			if (mInteropFlags == RenderInteropFlags::CUDA_INTEROP)
			{
				mMapState = RenderEntityMapStateIntl::PENDING_MAP;
				return;
			}

			bool result = false;
			if (getImpl()->getRenderStorageCount() > 0)
			{
				//map render resources
				result = true;
				for (uint32_t i = 0; result && i < getImpl()->getRenderStorageCount(); ++i)
				{
					UserRenderStorage* renderStorage;
					RenderInteropFlags::Enum interopFlags;
					RenderStorageStateIntl& renderStorageState = getImpl()->getRenderStorage(i, renderStorage, interopFlags);
					if (renderStorage)
					{
						getImpl()->onMapRenderStorage(i);
						result &= renderStorageState.map(renderStorage, RenderMapType::MAP_WRITE_DISCARD);
					}
				}
				if (!result)
				{
					//unmap in case of failure
					unmapRenderResources();
				}
			}
			if (result)
			{
				mMapState = RenderEntityMapStateIntl::MAPPED;
			}
		}
	}

	virtual void unmap()
	{
		if (mMapState == RenderEntityMapStateIntl::MAPPED)
		{
			if (mInteropFlags == RenderInteropFlags::CUDA_INTEROP)
			{
				mMapState = RenderEntityMapStateIntl::PENDING_UNMAP;
				return;
			}

			unmapRenderResources();

			mMapState = RenderEntityMapStateIntl::UNMAPPED;
		}
	}

#if APEX_CUDA_SUPPORT
	virtual void fillMapUnmapArraysForInterop(physx::Array<CUgraphicsResource> &toMapArray, physx::Array<CUgraphicsResource> &toUnmapArray)
	{
		if (mMapState == RenderEntityMapStateIntl::PENDING_MAP || mMapState == RenderEntityMapStateIntl::PENDING_UNMAP)
		{
			for (uint32_t i = 0; i < getImpl()->getRenderStorageCount(); ++i)
			{
				UserRenderStorage* renderStorage;
				RenderInteropFlags::Enum interopFlags;
				RenderStorageStateIntl& renderStorageState = getImpl()->getRenderStorage(i, renderStorage, interopFlags);
				if (renderStorage && interopFlags == RenderInteropFlags::CUDA_INTEROP)
				{
					CUgraphicsResource interopHandle = renderStorageState.getCudaHandle(renderStorage);
					if (interopHandle != NULL)
					{
						if (mMapState == RenderEntityMapStateIntl::PENDING_MAP && !renderStorageState.isCudaMapped())
						{
							toMapArray.pushBack(interopHandle);
						}
						if (mMapState == RenderEntityMapStateIntl::PENDING_UNMAP && renderStorageState.isCudaMapped())
						{
							toUnmapArray.pushBack(interopHandle);
						}
					}
				}
			}
		}
	}

	virtual void mapBufferResultsForInterop(bool mapSuccess, bool unmapSuccess)
	{
		PX_UNUSED(unmapSuccess);
		if (mMapState == RenderEntityMapStateIntl::PENDING_MAP || mMapState == RenderEntityMapStateIntl::PENDING_UNMAP)
		{
			bool result = false;
			if (getImpl()->getRenderStorageCount() > 0)
			{
				//map render resources
				result = true;
				for (uint32_t i = 0; i < getImpl()->getRenderStorageCount(); ++i)
				{
					UserRenderStorage* renderStorage;
					RenderInteropFlags::Enum interopFlags;
					RenderStorageStateIntl& renderStorageState = getImpl()->getRenderStorage(i, renderStorage, interopFlags);
					if (renderStorage)
					{
						bool mappedResult = false;
						if (renderStorageState.checkCudaHandle())
						{
							if (mMapState == RenderEntityMapStateIntl::PENDING_MAP && mapSuccess)
							{
								renderStorageState.setCudaMapped();
								mappedResult = renderStorageState.getCudaMapppedResult(renderStorage);
							}
							if (mMapState == RenderEntityMapStateIntl::PENDING_UNMAP && unmapSuccess)
							{
								renderStorageState.resetCudaMapped();
							}
							renderStorageState.resetCudaHandle();
						}
						if (mMapState == RenderEntityMapStateIntl::PENDING_MAP && !mappedResult)
						{
							//fall back to CPU mapping
							if (result)
							{
								getImpl()->onMapRenderStorage(i);
								result &= renderStorageState.map(renderStorage, RenderMapType::MAP_WRITE_DISCARD);
							}
						}
					}
				}
			}

			if (result && mMapState == RenderEntityMapStateIntl::PENDING_MAP)
			{
				mMapState = RenderEntityMapStateIntl::MAPPED;
			}
			else
			{
				unmapRenderResources();

				mMapState = RenderEntityMapStateIntl::UNMAPPED;
			}
		}
	}
#endif

protected:
	RenderEntityIntlImpl(RenderInteropFlags::Enum interopFlags)
		: mInteropFlags(interopFlags)
		, mMapState(RenderEntityMapStateIntl::UNMAPPED)
	{
	}

	void unmapRenderResources()
	{
		for (uint32_t i = 0; i < getImpl()->getRenderStorageCount(); ++i)
		{
			UserRenderStorage* renderStorage;
			RenderInteropFlags::Enum interopFlags;
			RenderStorageStateIntl& renderStorageState = getImpl()->getRenderStorage(i, renderStorage, interopFlags);
			if (renderStorage)
			{
				renderStorageState.unmap(renderStorage);
			}
		}
	}

	RenderInteropFlags::Enum		mInteropFlags;
	RenderEntityMapStateIntl::Enum	mMapState;
};


}
} // end namespace nvidia::apex

#endif // #ifndef RENDER_API_INTL_H
