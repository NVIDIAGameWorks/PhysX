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

#ifndef REMOVE_TJUNCTIONS_H

#define REMOVE_TJUNCTIONS_H

#include "Ps.h"
#include "foundation/PxSimpleTypes.h"

namespace physx
{
	namespace general_meshutils2
	{

class RemoveTjunctionsDesc
{
public:
  RemoveTjunctionsDesc(void)
  {
    mVcount = 0;
    mVertices = 0;
    mTcount = 0;
    mIndices = 0;
    mIds = 0;
    mTcountOut = 0;
    mIndicesOut = 0;
    mIdsOut = 0;
	mEpsilon = 0.00000001f;
  }

// input
  PxF32        mEpsilon;
  PxF32        mDistanceEpsilon;
  PxU32        mVcount;  // input vertice count.
  const PxF32 *mVertices; // input vertices as PxF32s or...
  PxU32        mTcount;    // number of input triangles.
  const PxU32 *mIndices;   // triangle indices.
  const PxU32 *mIds;       // optional triangle Id numbers.
// output..
  PxU32        mTcountOut;  // number of output triangles.
  const PxU32 *mIndicesOut; // output triangle indices
  const PxU32 *mIdsOut;     // output retained id numbers.
};

// Removes t-junctions from an input mesh.  Does not generate any new data points, but may possible produce additional triangles and new indices.
class RemoveTjunctions
{
public:

   virtual PxU32 removeTjunctions(RemoveTjunctionsDesc &desc) =0; // returns number of triangles output and the descriptor is filled with the appropriate results.


};

RemoveTjunctions * createRemoveTjunctions(void);
void               releaseRemoveTjunctions(RemoveTjunctions *tj);

	};
	using namespace general_meshutils2;
};

#endif
