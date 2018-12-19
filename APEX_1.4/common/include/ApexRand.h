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



#ifndef APEX_RAND_H
#define APEX_RAND_H

#include "PxMath.h"
#include "PxVec3.h"
#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

// "Quick and Dirty Symmetric Random number generator"	- returns a uniform deviate in [-1.0,1.0)
class QDSRand
{
	uint32_t mSeed;

public:

	PX_CUDA_CALLABLE PX_INLINE QDSRand(uint32_t seed = 0) : mSeed(seed) {}

	PX_CUDA_CALLABLE PX_INLINE void setSeed(uint32_t seed = 0)
	{
		mSeed = seed;
	}

	PX_CUDA_CALLABLE PX_INLINE uint32_t seed() const
	{
		return mSeed;
	}

	PX_CUDA_CALLABLE PX_INLINE uint32_t nextSeed()
	{
		mSeed = mSeed * 1664525L + 1013904223L;
		return mSeed;
	}

	PX_CUDA_CALLABLE PX_INLINE float getNext()
	{
		union U32F32
		{
			uint32_t   u;
			float   f;
		} r;
		r.u = 0x40000000 | (nextSeed() >> 9);
		return r.f - 3.0f;
	}

	PX_CUDA_CALLABLE PX_INLINE float getScaled(const float min, const float max)
	{
		const float scale = (max - min) / 2.0f;
		return ((getNext() + 1.0f) * scale) + min;
	}

	PX_CUDA_CALLABLE PX_INLINE PxVec3 getScaled(const PxVec3& min, const PxVec3& max)
	{
		return PxVec3(getScaled(min.x, max.x), getScaled(min.y, max.y), getScaled(min.z, max.z));
	}

	PX_CUDA_CALLABLE PX_INLINE float getUnit()
	{
		union U32F32
		{
			uint32_t   u;
			float   f;
		} r;
		r.u = 0x3F800000 | (nextSeed() >> 9);
		return r.f - 1.0f;
	}

};

// "Quick and Dirty Normal Random number generator"	- returns normally-distributed values
class QDNormRand
{
	QDSRand mBase;

public:

	PX_CUDA_CALLABLE PX_INLINE QDNormRand(uint32_t seed = 0) : mBase(seed) {}

	PX_CUDA_CALLABLE PX_INLINE void setSeed(uint32_t seed = 0)
	{
		mBase.setSeed(seed);
	}

	PX_CUDA_CALLABLE PX_INLINE uint32_t setSeed() const
	{
		return mBase.seed();
	}

	PX_CUDA_CALLABLE PX_INLINE uint32_t nextSeed()
	{
		return mBase.nextSeed();
	}

	PX_CUDA_CALLABLE PX_INLINE float getNext()
	{
		//Using Box-Muller transform (see http://en.wikipedia.org/wiki/Box_Muller_transform)

		float u, v, s;
		do
		{
			u = mBase.getNext();
			v = mBase.getNext();
			s = u * u + v * v;
		}
		while (s >= 1.0);

		return u * PxSqrt(-2.0f * PxLog(s) / s);
	}

	PX_CUDA_CALLABLE PX_INLINE float getScaled(const float m, const float s)
	{
		return m + s * getNext();
	}
};

}
} // end namespace nvidia::apex

#endif
