
#ifndef CONVEX_DECOMPOSITION_H

#define CONVEX_DECOMPOSITION_H

#include "PlatformConfigHACD.h"

/*!
**
** Copyright (c) 2015 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**

*/

namespace CONVEX_DECOMPOSITION
{

class ConvexResult
{
public:
  ConvexResult(void)
  {
    mHullVcount = 0;
    mHullVertices = 0;
    mHullTcount = 0;
    mHullIndices = 0;
    mHullVolume = 0;
  }

// the convex hull.result
  uint32_t		   	mHullVcount;			// Number of vertices in this convex hull.
  float 			*mHullVertices;			// The array of vertices (x,y,z)(x,y,z)...
  uint32_t       	mHullTcount;			// The number of triangles int he convex hull
  uint32_t			*mHullIndices;			// The triangle indices (0,1,2)(3,4,5)...
  float           mHullVolume;		    // the volume of the convex hull.

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
		mMeshVolumePercent = 0.01f;
		mMaxVertices = 32;
		mCallback = NULL;
  }

// describes the input triangle.
	uint32_t			mVcount;   // the number of vertices in the source mesh.
	const float	*mVertices; // start of the vertex position array.  Assumes a stride of 3 doubles.
	uint32_t			mTcount;   // the number of triangles in the source mesh.
	const uint32_t	*mIndices;  // the indexed triangle list array (zero index based)
// options
	uint32_t			mDepth;    // depth to split, a maximum of 10, generally not over 7.
	float			mCpercent; // the concavity threshold percentage.  0=20 is reasonable.
	float			mMeshVolumePercent; // if less than this percentage of the overall mesh volume, ignore
	hacd::ICallback		*mCallback;

// hull output limits.
	uint32_t		mMaxVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
};

class ConvexDecomposition
{
public:
	virtual uint32_t performConvexDecomposition(const DecompDesc &desc) = 0; // returns the number of hulls produced.
	virtual void release(void) = 0;
	virtual ConvexResult * getConvexResult(uint32_t index,bool takeMemoryOwnership) = 0;
protected:
	virtual ~ConvexDecomposition(void) { };
};

ConvexDecomposition * createConvexDecomposition(void);

};

#endif
