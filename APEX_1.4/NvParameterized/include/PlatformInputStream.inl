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

template<typename T> PX_INLINE Serializer::ErrorType PlatformInputStream::read(T &x, bool doAlign)
{
	if( doAlign )
		align(mTargetParams.getAlignment<T>());

	if( mCurParams.getSize<T>() != mTargetParams.getSize<T>() )
	{
		PX_ALWAYS_ASSERT();
		return Serializer::ERROR_INVALID_PLATFORM;
	}

	mStream.read(&x, sizeof(T));

	if( mCurParams.endian != mTargetParams.endian )
		SwapBytes((char *)&x, sizeof(T), GetDataType<T>::value);

	return Serializer::ERROR_NONE;
}

//Deserialize array of primitive type (slow path)
template<typename T> PX_INLINE Serializer::ErrorType PlatformInputStream::readSimpleArraySlow(Handle &handle)
{
	int32_t n;
	handle.getArraySize(n);

	align(mTargetParams.getAlignment<T>());

	for(int32_t i = 0; i < n; ++i)
	{
		handle.set(i);
		T val;
		NV_ERR_CHECK_RETURN( read(val) );
		handle.setParam<T>(val);

		handle.popIndex();
	}

	return Serializer::ERROR_NONE;
}

template<typename T> PX_INLINE Serializer::ErrorType PlatformInputStream::readSimpleArray(Handle &handle)
{
	if( mTargetParams.getSize<T>() == mCurParams.getSize<T>()
			&& mTargetParams.getSize<T>() >= mTargetParams.getAlignment<T>()
			&& mCurParams.getSize<T>() >= mCurParams.getAlignment<T>() ) //No gaps between array elements on both platforms
	{
		//Fast path

		int32_t n;
		handle.getArraySize(n);

		align(mTargetParams.getAlignment<T>());

		const int32_t elemSize = (int32_t)sizeof(T);

		if( mStream.tellRead() + elemSize * n >= mStream.getFileLength() )
		{
			DEBUG_ALWAYS_ASSERT();
			return Serializer::ERROR_INVALID_INTERNAL_PTR;
		}

		PX_ASSERT(elemSize * n >= 0);
		char *p = (char *)mTraits->alloc(static_cast<uint32_t>(elemSize * n));
		mStream.read(p, static_cast<uint32_t>(elemSize * n));

		if( mCurParams.endian != mTargetParams.endian )
		{
			char *elem = p;
			for(int32_t i = 0; i < n; ++i)
			{
				SwapBytes(elem, elemSize, GetDataType<T>::value);
				elem += elemSize;
			}
		}

		handle.setParamArray<T>((const T *)p, n);

		mTraits->free(p);
	}
	else
	{
		return readSimpleArraySlow<T>(handle);
	}

	return Serializer::ERROR_NONE;
}

