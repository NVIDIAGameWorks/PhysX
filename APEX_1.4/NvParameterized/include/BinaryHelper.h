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

#ifndef PX_SERIALIZE_BINARY_HELPER_H
#define PX_SERIALIZE_BINARY_HELPER_H

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "PxAssert.h"

#include <PsArray.h>

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"

namespace NvParameterized
{

template<typename T> static PX_INLINE T NvMax3(T x, T y, T z)
{
	return physx::PxMax<T>(x, physx::PxMax<T>(y, z));
}

#ifndef offsetof
#	define offsetof(StructType, field) reinterpret_cast<size_t>(&((StructType *)0)->field)
#endif

// Alignment calculator
template<typename T> class GetAlignment {
	struct TestStruct {
		char _;
		T x;
	};

public:
	static const size_t value = offsetof(struct TestStruct, x);
};

// Maps C type to NvParameterized::DataType
template<typename T> struct GetDataType {
	static const DataType value = NvParameterized::TYPE_UNDEFINED;
};

// Currently we only need to distinguish PxVec2 from other 64-bit stuff
// (like uint64_t, int64_t, double, pointer) in SwapBytes

template<> struct GetDataType <physx::PxVec2> {
	static const DataType value = NvParameterized::TYPE_VEC2;
};

//Copied from NvApexStream
PX_INLINE static bool IsBigEndian()
{
	uint32_t i = 1;
	return 0 == *(char *)&i;
}

PX_INLINE static void SwapBytes(char *data, uint32_t size, NvParameterized::DataType type)
{
	// XDK compiler does not like switch here
	if( 1 == size )
	{
		// Do nothing
	}
	else if( 2 == size )
	{
		char one_byte;
		one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
	}
	else if( 4 == size )
	{
		char one_byte;
		one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
		one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
	}
	else if( 8 == size )
	{
		//Handling of PxVec2 agregate is different from 64-bit atomic types
		if( TYPE_VEC2 == type )
		{
			//PxVec2 => swap each field separately
			SwapBytes(data + 0, 4, TYPE_F32);
			SwapBytes(data + 4, 4, TYPE_F32);
		}
		else
		{
			char one_byte;
			one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
			one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
			one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
			one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
		}
	}
	else
	{
		//Generic algorithm for containers of float

		const size_t elemSize = sizeof(float); //We assume that float sizes match on both platforms
		PX_ASSERT( elemSize >= GetAlignment<float>::value ); //If alignment is non-trivial below algorithm will not work
		PX_ASSERT( size > elemSize );

		//Just swap all PxReals
		for(size_t i = 0; i < size; i += elemSize)
			SwapBytes(data + i, elemSize, TYPE_F32);
	}
}

//Convert value to platform-independent format (network byte order)
template<typename T> PX_INLINE static T Canonize(T x)
{
	if( !IsBigEndian() )
		SwapBytes((char *)&x, sizeof(T), GetDataType<T>::value);

	return x;
}

//Convert value to native format (from network byte order)
template<typename T> PX_INLINE static T Decanonize(T x)
{
	return Canonize(x);
}

//Read platform-independent value from stream and convert it to native format
template<typename T> static PX_INLINE T readAndConvert(const char *&p)
{
	T val;
	memcpy((char *)&val, p, sizeof(T));
	val = Decanonize(val);
	p += sizeof(T);

	return val;
}

//Byte array used for data serialization
//TODO: replace this with Array?
class StringBuf
{
	Traits *mTraits;
	char *mData;
	uint32_t mSize, mCapacity;

	PX_INLINE void internalAppend(const char *data, uint32_t size, bool doCopy = true)
	{
		if( 0 == size )
			return;

		if( mCapacity < mSize + size )
			reserve(physx::PxMax(mSize + size, 3 * mCapacity / 2));

		if( doCopy )
			memcpy(mData + mSize, data, size);
		else
			memset(mData + mSize, 0, size); //We want padding bytes filled with 0

		mSize += size;
	}

public:

	PX_INLINE StringBuf(Traits *traits)
		: mTraits(traits), mData(0), mSize(0), mCapacity(0)
	{}

	PX_INLINE StringBuf(const StringBuf &s)
		: mTraits(s.mTraits), mSize(s.mSize), mCapacity(s.mSize)
	{
		mData = (char *)mTraits->alloc(mSize);
		memcpy(mData, s.mData, mSize);
	}

	PX_INLINE ~StringBuf() { mTraits->free((void *)mData); }

	PX_INLINE void reserve(uint32_t newCapacity)
	{
		if( mCapacity >= newCapacity )
			return;

		char *newData = (char *)mTraits->alloc(newCapacity);
		PX_ASSERT(newData);

		if( mData )
		{
			memcpy(newData, mData, mSize);
			mTraits->free(mData);
		}

		mData = newData;
		mCapacity = newCapacity;
	}

	PX_INLINE char *getBuffer()
	{
		char *data = mData;

		mSize = mCapacity = 0;
		mData = 0;

		return data;
	}

	PX_INLINE uint32_t size() const { return mSize; }

	template< typename T > PX_INLINE void append(T x)
	{
		internalAppend((char *)&x, sizeof(T));
	}

	template< typename T > PX_INLINE void append(T *x)
	{
		PX_UNUSED(x);
		PX_ASSERT(0 && "Unable to append pointer");
	}

	PX_INLINE void appendBytes(const char *data, uint32_t size) { internalAppend(data, size); }

	PX_INLINE void skipBytes(uint32_t size) { internalAppend(0, size, false); }

	PX_INLINE char &operator [](uint32_t i)
	{
		PX_ASSERT( i < mSize );
		return mData[i];
	}

	PX_INLINE operator char *() { return mData; }

	PX_INLINE operator const char *() const { return mData; }
};

//Dictionary of strings used for binary serialization
class Dictionary
{
	struct Entry
	{
		const char *s;
		uint32_t offset;
	};

	physx::shdfnd::Array<Entry, Traits::Allocator> entries; //TODO: use hash map after DE402 is fixed

public:
	Dictionary(Traits *traits_): entries(Traits::Allocator(traits_)) {}

	uint32_t put(const char *s);

	void setOffset(const char *s, uint32_t off);

	PX_INLINE void setOffset(uint32_t i, uint32_t off) { setOffset(get(i), off); }

	uint32_t getOffset(const char *s) const;

	uint32_t getOffset(uint32_t i) const { return getOffset(get(i)); }

	const char *get(uint32_t i) const { return entries[i].s; }

	PX_INLINE uint32_t size() const { return entries.size(); }

	void serialize(StringBuf &res) const;
};

//Binary file pretty-printer (mimics xxd)
void dumpBytes(const char *data, uint32_t nbytes);

}

#endif
