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

#ifndef CONVEX_DECOMPOSITION_H

#define CONVEX_DECOMPOSITION_H

#include "foundation/PxSimpleTypes.h"

namespace CONVEX_DECOMPOSITION
{

class TriangleMesh
{
public:
	TriangleMesh(void)
	{
		mVcount		= 0;
		mVertices	= NULL;
		mTriCount	= 0;
		mIndices	= NULL;
	}

	physx::PxU32	mVcount;
	physx::PxF32	*mVertices;
	physx::PxU32	mTriCount;
	physx::PxU32	*mIndices;
};

class ConvexResult
{
public:
  ConvexResult(void)
  {
    mHullVcount = 0;
    mHullVertices = 0;
    mHullTcount = 0;
    mHullIndices = 0;
  }

// the convex hull.result
  physx::PxU32		   	mHullVcount;			// Number of vertices in this convex hull.
  physx::PxF32 			*mHullVertices;			// The array of vertices (x,y,z)(x,y,z)...
  physx::PxU32       	mHullTcount;			// The number of triangles int he convex hull
  physx::PxU32			*mHullIndices;			// The triangle indices (0,1,2)(3,4,5)...
  physx::PxF32           mHullVolume;		    // the volume of the convex hull.

};

// just to avoid passing a zillion parameters to the method the
// options are packed into this descriptor.
class DecompDesc
{
public:
	DecompDesc(void)
	{
		mVcount = 0;
		mVertices = 0;
		mTcount   = 0;
		mIndices  = 0;
		mDepth    = 10;
		mCpercent = 4;
		mPpercent = 4;
		mVpercent = 0.2f;
		mMaxVertices = 32;
		mSkinWidth   = 0;
		mRemoveTjunctions = false;
		mInitialIslandGeneration = false;
		mUseIslandGeneration = false;
		mClosedSplit = false;
		mUseHACD = false;
		mConcavityHACD = 100;
		mMinClusterSizeHACD = 10;
		mConnectionDistanceHACD = 0;
  }

// describes the input triangle.
	physx::PxU32		mVcount;   // the number of vertices in the source mesh.
	const physx::PxF32  *mVertices; // start of the vertex position array.  Assumes a stride of 3 doubles.
	physx::PxU32		mTcount;   // the number of triangles in the source mesh.
	const physx::PxU32	*mIndices;  // the indexed triangle list array (zero index based)
// options
	physx::PxU32		mDepth;    // depth to split, a maximum of 10, generally not over 7.
	physx::PxF32		mCpercent; // the concavity threshold percentage.  0=20 is reasonable.
	physx::PxF32		mPpercent; // the percentage volume conservation threshold to collapse hulls. 0-30 is reasonable.
	physx::PxF32		mVpercent; // the pecentage of total mesh volume before we stop splitting.

	bool				mInitialIslandGeneration; // true if the initial mesh should be subjected to island generation.
	bool				mUseIslandGeneration; // true if the decomposition code should break the input mesh and split meshes into discrete 'islands/pieces'
	bool				mRemoveTjunctions; // whether or not to initially remove tjunctions found in the original mesh.
	bool				mClosedSplit;		// true if the hulls should be closed as they are split
	bool				mUseHACD;			// True if using the hierarchical approximate convex decomposition algorithm; recommended 
	physx::PxF32		mConcavityHACD;		// The concavity value to use for HACD (not the same as concavity percentage used with ACD. 
	physx::PxU32		mMinClusterSizeHACD;	// Minimum number of clusters to consider.
	physx::PxF32		mConnectionDistanceHACD; // The connection distance for HACD

// hull output limits.
	physx::PxU32		mMaxVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
	physx::PxF32		mSkinWidth;   // a skin width to apply to the output hulls.
};

class ConvexDecomposition
{
public:
	virtual physx::PxU32 performConvexDecomposition(const DecompDesc &desc) = 0; // returns the number of hulls produced.
	virtual void release(void) = 0;
	virtual ConvexResult * getConvexResult(physx::PxU32 index) = 0;


	virtual void splitMesh(const physx::PxF32 *planeEquation,const TriangleMesh &input,TriangleMesh &left,TriangleMesh &right,bool closedMesh) = 0;
	virtual void releaseTriangleMeshMemory(TriangleMesh &mesh) = 0;

protected:
	virtual ~ConvexDecomposition(void) { };
};

ConvexDecomposition * createConvexDecomposition(void);

};

#endif
