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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PX_TOOLKIT_RANDOM_H
#define PX_TOOLKIT_RANDOM_H

#include "common/PxPhysXCommonConfig.h"
#define TEST_MAX_RAND 0xffff

namespace PxToolkit
{
	using namespace physx;

	class BasicRandom
	{
		public:
									BasicRandom(PxU32 seed=0)	: mRnd(seed)	{}
									~BasicRandom()								{}

		PX_FORCE_INLINE	void		setSeed(PxU32 seed)			{ mRnd = seed;											}
		PX_FORCE_INLINE	PxU32		getCurrentValue()	const	{ return mRnd;											}
						PxU32		randomize()					{ mRnd = mRnd * 2147001325 + 715136305; return mRnd;	}

		PX_FORCE_INLINE	PxU32		rand()						{ return randomize() & 0xffff;							}
		PX_FORCE_INLINE	PxU32		rand32()					{ return randomize() & 0xffffffff;						}

						PxF32		rand(PxF32 a, PxF32 b)
									{
										const PxF32 r = rand32()/(static_cast<PxF32>(0xffffffff));
										return r*(b-a) + a;
									}

						PxI32		rand(PxI32 a, PxI32 b)
									{
										return a + static_cast<PxI32>(rand32()%(b-a));
									}

						PxF32		randomFloat()
									{
										return rand()/(static_cast<PxF32>(0xffff)) - 0.5f;
									}									
						PxF32		randomFloat32()
									{
										return rand32()/(static_cast<PxF32>(0xffffffff)) - 0.5f;
									}

						PxF32		randomFloat32(PxReal a, PxReal b) { return rand32()/PxF32(0xffffffff)*(b-a)+a; }
						void		unitRandomPt(physx::PxVec3& v);	
						void		unitRandomQuat(physx::PxQuat& v);

						PxVec3		unitRandomPt();
						PxQuat		unitRandomQuat();

	private:
						PxU32		mRnd;
	};

	//--------------------------------------
	// Fast, very good random numbers
	//
	// Period = 2^249
	//
	// Kirkpatrick, S., and E. Stoll, 1981; A Very Fast Shift-Register
	//       Sequence Random Number Generator, Journal of Computational Physics,
	// V. 40.
	//
	// Maier, W.L., 1991; A Fast Pseudo Random Number Generator,
	// Dr. Dobb's Journal, May, pp. 152 - 157

	class RandomR250
	{
	public:
		RandomR250(PxI32 s); 

		void	setSeed(PxI32 s);
		PxU32	randI();
		PxReal	randUnit();

		PxReal rand(PxReal lower, PxReal upper)
		{
			return lower + randUnit() * (upper - lower);
		}

	private:
		PxU32	mBuffer[250];
		PxI32	mIndex;
	};

	void SetSeed(PxU32 seed);
	PxU32 Rand();

	PX_INLINE PxF32 Rand(PxF32 a,PxF32 b)
	{
		const PxF32 r = static_cast<PxF32>(Rand())/(static_cast<PxF32>(TEST_MAX_RAND));
		return r*(b-a) + a;
	}

	PX_INLINE PxF32 RandLegacy(PxF32 a,PxF32 b)
	{
		const PxF32 r = static_cast<PxF32>(Rand())/(static_cast<PxF32>(0x7fff)+1.0f);
		return r*(b-a) + a;
	} 

	//returns numbers from [a, b-1] 
	PX_INLINE PxI32 Rand(PxI32 a,PxI32 b)
	{
		return a + static_cast<PxI32>(Rand()%(b-a));
	}
}

#endif
