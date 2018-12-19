#ifndef STAN_HULL_H

#define STAN_HULL_H

/*----------------------------------------------------------------------
		Copyright (c) 2004 Open Dynamics Framework Group
					www.physicstools.org
		All rights reserved.

		Redistribution and use in source and binary forms, with or without modification, are permitted provided
		that the following conditions are met:

		Redistributions of source code must retain the above copyright notice, this list of conditions
		and the following disclaimer.

		Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.

		Neither the name of the Open Dynamics Framework Group nor the names of its contributors may
		be used to endorse or promote products derived from this software without specific prior written permission.

		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES,
		INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
		DISCLAIMED. IN NO EVENT SHALL THE INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
		EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
		LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
		IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
		THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

#include "StanHullConfig.h"

namespace nvidia
{
	namespace stanhull
	{

class HullResult
{
public:
	HullResult(void)
	{
		mPolygons			= true;
		mNumOutputVertices	= 0;
		mOutputVertices		= NULL;
		mNumFaces			= 0;
		mNumIndices			= 0;
		mIndices			= NULL;
		mFaces				= NULL;
	}
	bool			mPolygons;			// true if indices represents polygons, false indices are triangles
	uint32_t			mNumOutputVertices;	// number of vertices in the output hull
	float			*mOutputVertices;	// array of vertices, 3 floats each x,y,z
	uint32_t			mNumFaces;			// the number of faces produced
	uint32_t			mNumIndices;		// the total number of indices
	uint32_t			*mIndices;			// pointer to indices.
	uint8_t			*mFaces;			// Number of points in each polygon face
};

enum HullFlag
{
	QF_TRIANGLES         = (1<<0),             // report results as triangles, not polygons.
	QF_REVERSE_ORDER     = (1<<1),             // reverse order of the triangle indices.
	QF_SKIN_WIDTH        = (1<<2),             // extrude hull based on this skin width
	QF_DEFAULT           = 0
};


class HullDesc
{
public:
	HullDesc(void)
	{
		mFlags          = QF_DEFAULT;
		mVcount         = 0;
		mVertices       = 0;
		mVertexStride   = 0;
		mNormalEpsilon  = 0.001f;
		mMaxVertices = 4096; // maximum number of points to be considered for a convex hull.
		mSkinWidth = 0.01f; // default is one centimeter
	};

	HullDesc(uint32_t flags,
			 uint32_t vcount,
			 const float *vertices,
			 uint32_t stride)
	{
		mFlags          = flags;
		mVcount         = vcount;
		mVertices       = vertices;
		mVertexStride   = stride;
		mNormalEpsilon  = 0.001f;
		mMaxVertices    = 4096;
		mSkinWidth = 0.01f; // default is one centimeter
	}

	bool HasHullFlag(uint32_t flags) const
	{
		if ( mFlags & flags ) return true;
		return false;
	}

	void SetHullFlag(uint32_t flags)
	{
		mFlags|=flags;
	}

	void ClearHullFlag(uint32_t flags)
	{
		mFlags&=~flags;
	}

	uint32_t      mFlags;           // flags to use when generating the convex hull.
	uint32_t      mVcount;          // number of vertices in the input point cloud
	const float      *mVertices;        // the array of vertices.
	uint32_t      mVertexStride;    // the stride of each vertex, in bytes.
	float             mNormalEpsilon;   // the epsilon for removing duplicates.  This is a normalized value, if normalized bit is on.
	float             mSkinWidth;
	uint32_t      mMaxVertices;               // maximum number of vertices to be considered for the hull!
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

	bool    CleanupVertices(uint32_t svcount,
													const float *svertices,
													uint32_t stride,
													uint32_t &vcount,       // output number of vertices
													float *vertices,                 // location to store the results.
													float  normalepsilon,
													float *scale);
};

}; // end of namespace
};

#endif
