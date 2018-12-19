

#ifndef CONVEX_HULL_H

#define CONVEX_HULL_H

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

namespace HACD
{

class HullResult
{
public:
	HullResult(void)
	{
		mNumOutputVertices = 0;
		mOutputVertices = 0;
		mNumTriangles = 0;
		mIndices = 0;
	}
	uint32_t			mNumOutputVertices;         // number of vertices in the output hull
	float			*mOutputVertices;            // array of vertices, 3 floats each x,y,z
	uint32_t			mNumTriangles;                  // the number of faces produced
	uint32_t			*mIndices;                   // pointer to indices.

// If triangles, then indices are array indexes into the vertex list.
// If polygons, indices are in the form (number of points in face) (p1, p2, p3, ..) etc..
};

class HullDesc
{
public:
	HullDesc(void)
	{
		mVcount         = 0;
		mVertices       = 0;
		mVertexStride   = sizeof(float)*3;
		mNormalEpsilon  = 0.001f;
		mMaxVertices = 256; // maximum number of points to be considered for a convex hull.
		mSkinWidth = 0.0f; // default is one centimeter
		mUseWuQuantizer = false;
	};

	HullDesc(uint32_t vcount,
			 const float *vertices,
			 uint32_t stride)
	{
		mVcount         = vcount;
		mVertices       = vertices;
		mVertexStride   = stride;
		mNormalEpsilon  = 0.001f;
		mMaxVertices    = 4096;
		mSkinWidth = 0.01f; // default is one centimeter
	}

	bool				mUseWuQuantizer;		// if True, uses the WuQuantizer to clean-up the input point cloud.  Of false, it uses Kmeans clustering.  More accurate but slower.
	uint32_t			mVcount;          // number of vertices in the input point cloud
	const float	*mVertices;        // the array of vertices.
	uint32_t			mVertexStride;    // the stride of each vertex, in bytes.
	float			mNormalEpsilon;   // the epsilon for removing duplicates.  This is a normalized value, if normalized bit is on.
	float			mSkinWidth;
	uint32_t			mMaxVertices;               // maximum number of vertices to be considered for the hull!
};

enum HullError
{
	QE_OK,            // success!
	QE_FAIL,           // failed.
	QE_NOT_READY,
};

// This class is used when converting a convex hull into a triangle mesh.
class ConvexHullVertex
{
public:
	float         mPos[3];
	float         mNormal[3];
	float         mTexel[2];
};

// A virtual interface to receive the triangles from the convex hull.
class ConvexHullTriangleInterface
{
public:
	virtual void ConvexHullTriangle(const ConvexHullVertex &v1,const ConvexHullVertex &v2,const ConvexHullVertex &v3) = 0;
};


class HullLibrary
{
public:

	HullError CreateConvexHull(const HullDesc       &desc,           // describes the input request
								HullResult           &result);        // contains the resulst

	HullError ReleaseResult(HullResult &result); // release memory allocated for this result, we are done with it.

	HullError CreateTriangleMesh(HullResult &answer,ConvexHullTriangleInterface *iface);
private:
	float ComputeNormal(float *n,const float *A,const float *B,const float *C);
	void AddConvexTriangle(ConvexHullTriangleInterface *callback,const float *p1,const float *p2,const float *p3);

	void BringOutYourDead(const float *verts,uint32_t vcount, float *overts,uint32_t &ocount,uint32_t *indices,uint32_t indexcount);

	bool    NormalizeAndCleanupVertices(uint32_t svcount,
							const float *svertices,
							uint32_t stride,
							uint32_t &vcount,       // output number of vertices
							float *vertices,                 // location to store the results.
							float  normalepsilon,
							float *scale,
							float *center,
							uint32_t maxVertices,
							bool useWuQuantizer);
};

}; // end of namespace HACD

#endif
