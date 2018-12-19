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

#ifndef PXPVDSDK_PXPVDFOUNDATION_H
#define PXPVDSDK_PXPVDFOUNDATION_H

#include "foundation/PxVec3.h"
#include "foundation/PxTransform.h"
#include "foundation/PxBounds3.h"

#include "PsArray.h"
#include "PsHashMap.h"
#include "PsHashSet.h"
#include "PsPool.h"
#include "PsString.h"

#include "PxPvdObjectModelBaseTypes.h"

namespace physx
{
namespace pvdsdk
{

extern PxAllocatorCallback* gPvdAllocatorCallback;

class ForwardingAllocator : public PxAllocatorCallback
{
	void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		return shdfnd::getAllocator().allocate(size, typeName, filename, line);
	}
	void deallocate(void* ptr)
	{
		shdfnd::getAllocator().deallocate(ptr);
	}
};

class RawMemoryBuffer
{
	uint8_t* mBegin;
	uint8_t* mEnd;
	uint8_t* mCapacityEnd;
	const char* mBufDataName;

  public:
	RawMemoryBuffer(const char* name) : mBegin(0), mEnd(0), mCapacityEnd(0),mBufDataName(name)
	{
		PX_UNUSED(mBufDataName);
	}
	~RawMemoryBuffer()
	{
		if(mBegin)
			PX_FREE(mBegin);
	}
	uint32_t size() const
	{
		return static_cast<uint32_t>(mEnd - mBegin);
	}
	uint32_t capacity() const
	{
		return static_cast<uint32_t>(mCapacityEnd - mBegin);
	}
	uint8_t* begin()
	{
		return mBegin;
	}
	uint8_t* end()
	{
		return mEnd;
	}
	const uint8_t* begin() const
	{
		return mBegin;
	}
	const uint8_t* end() const
	{
		return mEnd;
	}
	void clear()
	{
		mEnd = mBegin;
	}
	const char* cStr()
	{
		if(mEnd && (*mEnd != 0))
			write(0);
		return reinterpret_cast<const char*>(mBegin);
	}
	uint32_t write(uint8_t inValue)
	{
		*growBuf(1) = inValue;
		return 1;
	}

	template <typename TDataType>
	uint32_t write(const TDataType& inValue)
	{
		const uint8_t* __restrict readPtr = reinterpret_cast<const uint8_t*>(&inValue);
		uint8_t* __restrict writePtr = growBuf(sizeof(TDataType));
		for(uint32_t idx = 0; idx < sizeof(TDataType); ++idx)
			writePtr[idx] = readPtr[idx];
		return sizeof(TDataType);
	}

	template <typename TDataType>
	uint32_t write(const TDataType* inValue, uint32_t inLength)
	{
		uint32_t writeSize = inLength * sizeof(TDataType);
		if(inValue && inLength)
		{
			physx::intrinsics::memCopy(growBuf(writeSize), inValue, writeSize);
		}
		if(inLength && !inValue)
		{
			PX_ASSERT(false);
			// You can't not write something, because that will cause
			// the receiving end to crash.
			for(uint32_t idx = 0; idx < writeSize; ++idx)
				write(0);
		}
		return writeSize;
	}

	uint8_t* growBuf(uint32_t inAmount)
	{
		uint32_t offset = size();
		uint32_t newSize = offset + inAmount;
		reserve(newSize);
		mEnd += inAmount;
		return mBegin + offset;
	}
	void writeZeros(uint32_t inAmount)
	{
		uint32_t offset = size();
		growBuf(inAmount);
		physx::intrinsics::memZero(begin() + offset, inAmount);
	}
	void reserve(uint32_t newSize)
	{
		uint32_t currentSize = size();
		if(newSize && newSize >= capacity())
		{
			uint32_t newDataSize = newSize > 4096 ? newSize + (newSize >> 2) : newSize*2;
			uint8_t* newData = static_cast<uint8_t*>(PX_ALLOC(newDataSize, mBufDataName));
			if(mBegin)
			{
				physx::intrinsics::memCopy(newData, mBegin, currentSize);
				PX_FREE(mBegin);
			}
			mBegin = newData;
			mEnd = mBegin + currentSize;
			mCapacityEnd = mBegin + newDataSize;
		}
	}
};

struct ForwardingMemoryBuffer : public RawMemoryBuffer
{
	ForwardingMemoryBuffer(const char* bufDataName) : RawMemoryBuffer(bufDataName)
	{
	}

	ForwardingMemoryBuffer& operator<<(const char* inString)
	{
		if(inString && *inString)
		{
			uint32_t len = static_cast<uint32_t>(strlen(inString));
			write(inString, len);
		}
		return *this;
	}

	template <typename TDataType>
	inline ForwardingMemoryBuffer& toStream(const char* inFormat, const TDataType inData)
	{
		char buffer[128] = { 0 };
		shdfnd::snprintf(buffer, 128, inFormat, inData);
		*this << buffer;
		return *this;
	}

	inline ForwardingMemoryBuffer& operator<<(bool inData)
	{
		*this << (inData ? "true" : "false");
		return *this;
	}
	inline ForwardingMemoryBuffer& operator<<(int32_t inData)
	{
		return toStream("%d", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(uint16_t inData)
	{
		return toStream("%u", uint32_t(inData));
	}
	inline ForwardingMemoryBuffer& operator<<(uint8_t inData)
	{
		return toStream("%u", uint32_t(inData));
	}
	inline ForwardingMemoryBuffer& operator<<(char inData)
	{
		return toStream("%c", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(uint32_t inData)
	{
		return toStream("%u", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(uint64_t inData)
	{
		return toStream("%I64u", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(int64_t inData)
	{
		return toStream("%I64d", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(const void* inData)
	{
		return *this << static_cast<uint64_t>(reinterpret_cast<size_t>(inData));
	}
	inline ForwardingMemoryBuffer& operator<<(float inData)
	{
		return toStream("%g", double(inData));
	}
	inline ForwardingMemoryBuffer& operator<<(double inData)
	{
		return toStream("%g", inData);
	}
	inline ForwardingMemoryBuffer& operator<<(const PxVec3& inData)
	{
		*this << inData[0];
		*this << " ";
		*this << inData[1];
		*this << " ";
		*this << inData[2];
		return *this;
	}

	inline ForwardingMemoryBuffer& operator<<(const PxQuat& inData)
	{
		*this << inData.x;
		*this << " ";
		*this << inData.y;
		*this << " ";
		*this << inData.z;
		*this << " ";
		*this << inData.w;
		return *this;
	}

	inline ForwardingMemoryBuffer& operator<<(const PxTransform& inData)
	{
		*this << inData.q;
		*this << " ";
		*this << inData.p;
		return *this;
	}

	inline ForwardingMemoryBuffer& operator<<(const PxBounds3& inData)
	{
		*this << inData.minimum;
		*this << " ";
		*this << inData.maximum;
		return *this;
	}

};

template <typename TObjectType, typename TGetSetIndexOp, typename TSetSetIndexOp>
class InvasiveSet
{
    shdfnd::Array<TObjectType*> mSet;

	InvasiveSet(const InvasiveSet& other);
	InvasiveSet& operator=(const InvasiveSet& other);

  public:
	InvasiveSet(const char* allocName) : mSet(allocName)
	{
	}

	bool insert(TObjectType& inObject)
	{
		uint32_t currentIdx = TGetSetIndexOp()(inObject);
		if(currentIdx == UINT32_MAX)
		{
			TSetSetIndexOp()(inObject, mSet.size());
			mSet.pushBack(&inObject);
			return true;
		}
		return false;
	}

	bool remove(TObjectType& inObject)
	{
		uint32_t currentIdx = TGetSetIndexOp()(inObject);
		if(currentIdx != UINT32_MAX)
		{
			TObjectType* theEnd = mSet.back();
			TObjectType* theObj = &inObject;
			if(theEnd != theObj)
			{
				TSetSetIndexOp()(*theEnd, currentIdx);
				mSet[currentIdx] = theEnd;
			}
			mSet.popBack();
			TSetSetIndexOp()(inObject, UINT32_MAX);
			return true;
		}
		return false;
	}

	bool contains(TObjectType& inObject)
	{
		return TGetSetIndexOp()(inObject) != UINT32_MAX;
	}

	void clear()
	{
		for(uint32_t idx = 0; idx < mSet.size(); ++idx)
			TSetSetIndexOp()(*(mSet[idx]), UINT32_MAX);
		mSet.clear();
	}

	TObjectType* operator[](uint32_t idx)
	{
		return mSet[idx];
	}
	const TObjectType* operator[](uint32_t idx) const
	{
		return mSet[idx];
	}
	uint32_t size() const
	{
		return mSet.size();
	}
	TObjectType** begin()
	{
		return mSet.begin();
	}
	TObjectType** end()
	{
		return mSet.end();
	}
	const TObjectType** begin() const
	{
		return mSet.begin();
	}
	const TObjectType** end() const
	{
		return mSet.end();
	}
	const TObjectType* back() const
	{
		return mSet.back();
	}
	TObjectType* back()
	{
		return mSet.back();
	}
};

template <typename TDataType>
inline void* PvdAllocate(const char* typeName, const char* file, int line)
{
	PX_ASSERT(gPvdAllocatorCallback);
	return gPvdAllocatorCallback->allocate(sizeof(TDataType), typeName, file, line);
}

template <typename TDataType>
inline void PvdDeleteAndDeallocate(TDataType* inDType)
{
	PX_ASSERT(gPvdAllocatorCallback);
	if(inDType)
	{
		inDType->~TDataType();
		gPvdAllocatorCallback->deallocate(inDType);
	}
}
}
}

#define PVD_NEW(dtype) new (PvdAllocate<dtype>(#dtype, __FILE__, __LINE__)) dtype
#define PVD_DELETE(obj) PvdDeleteAndDeallocate(obj);
//#define PVD_NEW(dtype) PX_NEW(dtype)
//#define PVD_DELETE(obj) PX_DELETE(obj)
#define PVD_FOREACH(varname, stop) for(uint32_t varname = 0; varname < stop; ++varname)

namespace physx
{
namespace pvdsdk
{

template <typename TKeyType, typename TValueType, typename THashType, typename TBufType, typename TOperator>
uint32_t getMapKeysOp(shdfnd::HashMap<TKeyType, TValueType, THashType>& map, TBufType* buffer, uint32_t bufSize,
                      uint32_t startIdx, TOperator op)
{
	uint32_t numItems = static_cast<uint32_t>(map.size());
	if(numItems == 0 || bufSize == 0)
		return 0;

	startIdx = PxMin(numItems - 1, startIdx);
	uint32_t retval = 0;
    for(typename shdfnd::HashMap<TKeyType, TValueType, THashType>::Iterator iter = map.getIterator();
	    iter.done() == false && bufSize; ++iter)
	{
		if(startIdx)
			--startIdx;
		else
		{
			buffer[retval] = op(iter->first);
			--bufSize;
			++retval;
		}
	}
	return retval;
}

struct IdOp
{
	template <typename TDataType>
	TDataType operator()(const TDataType& item)
	{
		return item;
	}
};

template <typename TKeyType, typename TValueType, typename THashType>
uint32_t getMapKeys(shdfnd::HashMap<TKeyType, TValueType, THashType>& map, TKeyType* buffer, uint32_t bufSize, uint32_t startIdx)
{
	return getMapKeysOp(map, buffer, bufSize, startIdx, IdOp());
}

struct DerefOp
{
	template <typename TDataType>
	TDataType operator()(const TDataType* item)
	{
		return *item;
	}
};

template <typename TKeyType, typename TValueType, typename TBufType, typename TOp>
uint32_t getMapValues(shdfnd::HashMap<TKeyType, TValueType>& map, TBufType* buffer, uint32_t bufSize, uint32_t startIdx, TOp op)
{
	uint32_t numItems = static_cast<uint32_t>(map.size());
	if(numItems == 0 || bufSize == 0)
		return 0;

	startIdx = PxMin(numItems - 1, startIdx);
	uint32_t retval = 0;
    for(typename shdfnd::HashMap<TKeyType, TValueType>::Iterator iter = map.getIterator(); iter.done() == false && bufSize; ++iter)
	{
		if(startIdx)
			--startIdx;
		else
		{
			buffer[retval] = op(iter->second);
			--bufSize;
			++retval;
		}
	}
	return retval;
}

template <typename TValueType, typename TBufType>
uint32_t getArrayEntries(shdfnd::Array<TValueType>& data, TBufType* buffer, uint32_t bufSize, uint32_t startIdx)
{
	uint32_t numItems = static_cast<uint32_t>(data.size());
	if(numItems == 0 || bufSize == 0)
		return 0;

	startIdx = PxMin(numItems - 1, startIdx);
	uint32_t available = PxMin(numItems - startIdx, bufSize);
	PVD_FOREACH(idx, available)
	buffer[idx] = data[idx + startIdx];
	return available;
}
#define PVD_POINTER_TO_U64(ptr) static_cast<uint64_t>(reinterpret_cast<size_t>(ptr))
}
}
#endif // PXPVDSDK_PXPVDFOUNDATION_H
