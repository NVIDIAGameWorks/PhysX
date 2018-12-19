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



#include "RTdef.h"
#if RT_COMPILE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for box pruning.
 *	\file		IceBoxPruning.h
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEBOXPRUNING_BASE_H__
#define __ICEBOXPRUNING_BASE_H__

//#include "vector"
#include <PsArray.h>
#include "IceRevisitedRadixBase.h"
#include "PxVec3.h"
#include "PxBounds3.h"
#include <PsUserAllocated.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

	struct Axes
	{
		void set(uint32_t a0, uint32_t a1, uint32_t a2) {
			Axis0 = a0; Axis1 = a1; Axis2 = a2; 
		}
		uint32_t	Axis0;
		uint32_t	Axis1;
		uint32_t	Axis2;
	};

	class BoxPruning : public UserAllocated 
	{
	public:
		// Optimized versions
		bool completeBoxPruning(const nvidia::Array<PxBounds3> &bounds, nvidia::Array<uint32_t> &pairs, const Axes& axes);
		bool bipartiteBoxPruning(const nvidia::Array<PxBounds3> &bounds0, const nvidia::Array<PxBounds3> &bounds1, nvidia::Array<uint32_t>& pairs, const Axes& axes);

		// Brute-force versions
		bool bruteForceCompleteBoxTest(const nvidia::Array<PxBounds3> &bounds, nvidia::Array<uint32_t> &pairs, const Axes& axes);
		bool bruteForceBipartiteBoxTest(const nvidia::Array<PxBounds3> &bounds0, const nvidia::Array<PxBounds3> &bounds1, nvidia::Array<uint32_t>& pairs, const Axes& axes);

	protected:
		nvidia::Array<float> mMinPosBounds0;
		nvidia::Array<float> mMinPosBounds1;
		nvidia::Array<float> mPosList;
		RadixSort mRS0, mRS1;	
		RadixSort mRS;
	};

}
}
}

#endif // __ICEBOXPRUNING_H__
#endif
