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


#include "SimdMath.h"

#include "PxMat44.h"
#include "PxMat33.h"
#include "PxVec3.h"
#include "PxVec4.h"
#include "PxQuat.h"

using physx::PxMat44;
using physx::PxMat33;
using physx::PxVec3;
using physx::PxVec4;
using physx::PxQuat;

#include "PxPreprocessor.h"
#include "PxAssert.h"
#include "ApexMath.h"

namespace nvidia
{

using namespace clothing;

	bool operator != (const PxMat44& a, const PxMat44& b)
	{
		PX_ASSERT((((size_t)&a) & 0xf) == 0); // verify 16 byte alignment
		PX_ASSERT((((size_t)&b) & 0xf) == 0); // verify 16 byte alignment

		int allEq = true;
		
		typedef Simd4fLoadFactory loadFactory;

		const Simd4f ca1 = loadFactory(&a.column0.x);
		const Simd4f cb1 = loadFactory(&b.column0.x);
		allEq &= allEqual(ca1, cb1);

		const Simd4f ca2 = loadFactory(&a.column1.x);
		const Simd4f cb2 = loadFactory(&b.column1.x);
		allEq &= allEqual(ca2, cb2);

		const Simd4f ca3 = loadFactory(&a.column2.x);
		const Simd4f cb3 = loadFactory(&b.column2.x);
		allEq &= allEqual(ca3, cb3);

		const Simd4f ca4 = loadFactory(&a.column3.x);
		const Simd4f cb4 = loadFactory(&b.column3.x);
		allEq &= allEqual(ca4, cb4);
		
		return allEq == 0;
	}
}
