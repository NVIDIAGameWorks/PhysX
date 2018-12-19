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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

template <typename T> PX_INLINE void PlatformOutputStream::align()
{
	align(mTargetParams.getAlignment<T>());
}

template <typename T> PX_INLINE uint32_t PlatformOutputStream::storeSimple(T x)
{
	align(mTargetParams.getAlignment<T>());

	uint32_t off = size();

	data.append(x);

	char *p = getData() + off;

	//It is unsafe to call SwapBytes directly on floats (doubles, vectors, etc.)
	//because values may become NaNs which will lead to all sorts of errors
	if( mCurParams.endian != mTargetParams.endian )
		SwapBytes(p, sizeof(T), GetDataType<T>::value);

	return off;
}

PX_INLINE uint32_t PlatformOutputStream::storeSimple(float* x, uint32_t num)
{
	align(mTargetParams.getAlignment<float>());

	uint32_t off = size();

	for (uint32_t k = 0; k < num; ++k)
	{
		data.append(x[k]);
	}

	char *p = getData() + off;

	//It is unsafe to call SwapBytes directly on floats (doubles, vectors, etc.)
	//because values may become NaNs which will lead to all sorts of errors
	if( mCurParams.endian != mTargetParams.endian )
		SwapBytes(p, sizeof(float) * num, GetDataType<float>::value);

	return off;
}

template <typename T> PX_INLINE int32_t PlatformOutputStream::storeSimpleArraySlow(Handle &handle)
{
	int32_t n;
	handle.getArraySize(n);

	align<T>();
	uint32_t off = size();

	data.reserve(size() + n * physx::PxMax(mTargetParams.getAlignment<T>(), mTargetParams.getSize<T>()));

	for(int32_t i = 0; i < n; ++i)
	{
		handle.set(i);

		T val;
		handle.getParam<T>(val);
		storeSimple<T>(val);

		handle.popIndex();
	}

	int32_t offsetAsInt = static_cast<int32_t>(off);
	PX_ASSERT(offsetAsInt >= 0);
	return offsetAsInt;
}

template <typename T> PX_INLINE uint32_t PlatformOutputStream::storeSimpleArray(Handle &handle)
{
	if( TYPE_BOOL != handle.parameterDefinition()->child(0)->type() //Bools are special (see custom version of storeSimple)
			&& mTargetParams.getSize<T>() == mCurParams.getSize<T>()
			&& mTargetParams.getSize<T>() >= mTargetParams.getAlignment<T>()
			&& mCurParams.getSize<T>() >= mCurParams.getAlignment<T>() ) //No gaps between array elements on both platforms
	{
		//Fast path

		int32_t n;
		handle.getArraySize(n);

		align<T>();
		uint32_t off = size();

		const int32_t elemSize = (int32_t)sizeof(T);

		skipBytes((uint32_t)(elemSize * n));

		char *p = getData() + off;
		handle.getParamArray<T>((T *)p, n);

		if( mCurParams.endian != mTargetParams.endian )
		{
			for(int32_t i = 0; i < n; ++i)
			{
				SwapBytes(p, elemSize, GetDataType<T>::value);
					p += elemSize;
			}
		}

		return off;
	}
	else
	{
		return static_cast<uint32_t>(storeSimpleArraySlow<T>(handle));
	}
}
