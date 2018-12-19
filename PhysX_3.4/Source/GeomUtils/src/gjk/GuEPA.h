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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GU_EPA_H
#define GU_EPA_H

#include "GuGJKUtil.h"
#include "GuGJKType.h"

namespace physx
{
  
namespace Gu
{
	//ML: The main entry point for EPA.
	// 
	//This function returns one of three status codes:
	//(1)EPA_FAIL:	the algorithm failed to create a valid polytope(the origin wasn't inside the polytope) from the input simplex.
	//(2)EPA_CONTACT : the algorithm found the MTD and converged successfully.
	//(3)EPA_DEGENERATE: the algorithm cannot make further progress and the result is unknown.

	GjkStatus epaPenetration(const GjkConvex& a, const GjkConvex& b, 					// two convexes, in the same space							   
							   PxU8* PX_RESTRICT aInd, PxU8* PX_RESTRICT bInd,			// warm start index points to create an initial simplex that EPA will work on
							   PxU8 _size,												// count of warm-start indices
							   Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB,		// a point on each body: when B is translated by normal*penetrationDepth, these are coincident
							   Ps::aos::Vec3V& normal, Ps::aos::FloatV& depth,			// MTD normal & penetration depth							    
							   const bool takeCoreShape = false);						// indicates whether we take support point from the core shape of the convexes
}

}

#endif
