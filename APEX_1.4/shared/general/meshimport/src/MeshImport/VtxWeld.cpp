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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "VtxWeld.h"

namespace mimp
{

template<> MeshVertex VertexLess<MeshVertex>::mFind = MeshVertex();
template<> STDNAME::vector<MeshVertex > *VertexLess<MeshVertex>::mList=0;


template<>
bool VertexLess<MeshVertex>::operator()(MiI32 v1,MiI32 v2) const
{

	const MeshVertex& a = Get(v1);
	const MeshVertex& b = Get(v2);

	if ( a.mPos[0] < b.mPos[0] ) return true;
	if ( a.mPos[0] > b.mPos[0] ) return false;

	if ( a.mPos[1] < b.mPos[1] ) return true;
	if ( a.mPos[1] > b.mPos[1] ) return false;

	if ( a.mPos[2] < b.mPos[2] ) return true;
	if ( a.mPos[2] > b.mPos[2] ) return false;


	if ( a.mNormal[0] < b.mNormal[0] ) return true;
	if ( a.mNormal[0] > b.mNormal[0] ) return false;

	if ( a.mNormal[1] < b.mNormal[1] ) return true;
	if ( a.mNormal[1] > b.mNormal[1] ) return false;

	if ( a.mNormal[2] < b.mNormal[2] ) return true;
	if ( a.mNormal[2] > b.mNormal[2] ) return false;

  if ( a.mColor < b.mColor ) return true;
  if ( a.mColor > b.mColor ) return false;


	if ( a.mTexel1[0] < b.mTexel1[0] ) return true;
	if ( a.mTexel1[0] > b.mTexel1[0] ) return false;

	if ( a.mTexel1[1] < b.mTexel1[1] ) return true;
	if ( a.mTexel1[1] > b.mTexel1[1] ) return false;

	if ( a.mTexel2[0] < b.mTexel2[0] ) return true;
	if ( a.mTexel2[0] > b.mTexel2[0] ) return false;

	if ( a.mTexel2[1] < b.mTexel2[1] ) return true;
	if ( a.mTexel2[1] > b.mTexel2[1] ) return false;

	if ( a.mTexel3[0] < b.mTexel3[0] ) return true;
	if ( a.mTexel3[0] > b.mTexel3[0] ) return false;

	if ( a.mTexel3[1] < b.mTexel3[1] ) return true;
	if ( a.mTexel3[1] > b.mTexel3[1] ) return false;


	if ( a.mTexel4[0] < b.mTexel4[0] ) return true;
	if ( a.mTexel4[0] > b.mTexel4[0] ) return false;

	if ( a.mTexel4[1] < b.mTexel4[1] ) return true;
	if ( a.mTexel4[1] > b.mTexel4[1] ) return false;

	if ( a.mTangent[0] < b.mTangent[0] ) return true;
	if ( a.mTangent[0] > b.mTangent[0] ) return false;

	if ( a.mTangent[1] < b.mTangent[1] ) return true;
	if ( a.mTangent[1] > b.mTangent[1] ) return false;

	if ( a.mTangent[2] < b.mTangent[2] ) return true;
	if ( a.mTangent[2] > b.mTangent[2] ) return false;

	if ( a.mBiNormal[0] < b.mBiNormal[0] ) return true;
	if ( a.mBiNormal[0] > b.mBiNormal[0] ) return false;

	if ( a.mBiNormal[1] < b.mBiNormal[1] ) return true;
	if ( a.mBiNormal[1] > b.mBiNormal[1] ) return false;

	if ( a.mBiNormal[2] < b.mBiNormal[2] ) return true;
	if ( a.mBiNormal[2] > b.mBiNormal[2] ) return false;

	if ( a.mWeight[0] < b.mWeight[0] ) return true;
	if ( a.mWeight[0] > b.mWeight[0] ) return false;

	if ( a.mWeight[1] < b.mWeight[1] ) return true;
	if ( a.mWeight[1] > b.mWeight[1] ) return false;

	if ( a.mWeight[2] < b.mWeight[2] ) return true;
	if ( a.mWeight[2] > b.mWeight[2] ) return false;

	if ( a.mWeight[3] < b.mWeight[3] ) return true;
	if ( a.mWeight[3] > b.mWeight[3] ) return false;

	if ( a.mBone[0] < b.mBone[0] ) return true;
	if ( a.mBone[0] > b.mBone[0] ) return false;

	if ( a.mBone[1] < b.mBone[1] ) return true;
	if ( a.mBone[1] > b.mBone[1] ) return false;

	if ( a.mBone[2] < b.mBone[2] ) return true;
	if ( a.mBone[2] > b.mBone[2] ) return false;

	if ( a.mBone[3] < b.mBone[3] ) return true;
	if ( a.mBone[3] > b.mBone[3] ) return false;

  if ( a.mRadius < b.mRadius ) return true;
  if ( a.mRadius > b.mRadius ) return false;

	return false;
};



}; // end of name space
