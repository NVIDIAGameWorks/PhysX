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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "ConvexDecomposition.h"
#include "StanHull.h"
#include "FloatMath.h"
#include "SplitMesh.h"
#include "PsArray.h"
#include "PsUserAllocated.h"
#include "foundation/PxErrorCallback.h"
#include "PsFoundation.h"
#include "foundation/PxAllocatorCallback.h"
#include "HACD.h"

#pragma warning(disable:4702)
#pragma warning(disable:4996 4100)
#pragma warning(disable:4267)

static const physx::PxF32 EPSILON=0.0001;

typedef physx::Array< physx::PxU32 > UintVector;

using namespace physx::general_floatmath2;
using namespace physx::stanhull;
using namespace physx;
using namespace physx::shdfnd;

namespace CONVEX_DECOMPOSITION
{
#if 0
static bool saveObj(SPLIT_MESH::SimpleMesh &mesh,int saveCount,const char *prefix)
{
	bool ret = false;

	if ( mesh.mVcount )
	{
		char fname[512];
		sprintf_s(fname,512,"%s_%02d.obj", prefix, saveCount );
		int vcount = mesh.mVcount;
		float *vertices = mesh.mVertices;
		int tcount = mesh.mTcount;
		const int *indices = (const int *)mesh.mIndices;


		FILE *fph = fopen(fname,"wb");
		if ( fph )
		{
			for (int i=0; i<vcount; i++)
			{
				fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", vertices[0], vertices[1], vertices[2] );
				vertices+=3;
			}
			for (int i=0; i<tcount; i++)
			{
				fprintf(fph,"f %d %d %d\r\n", indices[0]+1, indices[1]+1, indices[2]+1 );
				indices+=3;
			}
			fclose(fph);
			ret = true;
		}
	}
	return ret;
}
#endif

	class MyConvexResult : public ConvexResult, public physx::UserAllocated
	{
	public:
		MyConvexResult(physx::PxU32 hvcount,const physx::PxF32 *hvertices,physx::PxU32 htcount,const physx::PxU32 *hindices)
		{
			mHullVcount = 0;
			mHullTcount = 0;
			mHullIndices = NULL;
			mHullVertices = NULL;
			if ( hvcount && hvertices && hindices == NULL )
			{
				HullResult  result;
				HullLibrary hl;
				HullDesc    desc;
				desc.mMaxVertices = 256;
				desc.SetHullFlag(QF_TRIANGLES);
				desc.mVcount       = hvcount;
				desc.mVertices     = hvertices;
				desc.mVertexStride = sizeof(physx::PxF32)*3;
				HullError ret = hl.CreateConvexHull(desc,result);
				if ( ret == QE_OK )
				{
					mHullVcount = result.mNumOutputVertices;
					mHullVertices = (physx::PxF32 *)PX_ALLOC(mHullVcount*sizeof(physx::PxF32)*3, PX_DEBUG_EXP("ConvexResult"));
					memcpy(mHullVertices,result.mOutputVertices,mHullVcount*sizeof(physx::PxF32)*3);
					mHullTcount = result.mNumFaces;
					mHullIndices =(physx::PxU32 *)PX_ALLOC(sizeof(physx::PxU32)*mHullTcount*3, PX_DEBUG_EXP("ConvexResult"));
					memcpy(mHullIndices,result.mIndices,sizeof(physx::PxU32)*mHullTcount*3);
					hl.ReleaseResult(result);
				}
			}
			else
			{
				mHullVcount = hvcount;
				if ( mHullVcount )
				{
					mHullVertices = (physx::PxF32 *)PX_ALLOC(mHullVcount*sizeof(physx::PxF32)*3, PX_DEBUG_EXP("ConvexResult"));
					memcpy(mHullVertices, hvertices, sizeof(physx::PxF32)*3*mHullVcount );
				}
				mHullTcount = htcount;
				if ( mHullTcount )
				{
					mHullIndices = (physx::PxU32 *)PX_ALLOC(sizeof(physx::PxU32)*mHullTcount*3, PX_DEBUG_EXP("ConvexResult"));
					memcpy(mHullIndices,hindices, sizeof(physx::PxU32)*mHullTcount*3 );
				}
			}
		}

		MyConvexResult(const MyConvexResult &r) // copy constructor, perform a deep copy of the data.
		{
			mHullVcount = r.mHullVcount;
			if ( mHullVcount )
			{
				mHullVertices = (physx::PxF32 *)PX_ALLOC(mHullVcount*sizeof(physx::PxF32)*3, PX_DEBUG_EXP("ConvexResult"));
				memcpy(mHullVertices, r.mHullVertices, sizeof(physx::PxF32)*3*mHullVcount );
			}
			else
			{
				mHullVertices = 0;
			}
			mHullTcount = r.mHullTcount;
			if ( mHullTcount )
			{
				mHullIndices = (physx::PxU32 *)PX_ALLOC(sizeof(physx::PxU32)*mHullTcount*3, PX_DEBUG_EXP("ConvexResult"));
				memcpy(mHullIndices, r.mHullIndices, sizeof(physx::PxU32)*mHullTcount*3 );
			}
			else
			{
				mHullIndices = 0;
			}
		}

		~MyConvexResult(void)
		{
			PX_FREE(mHullVertices);
			PX_FREE(mHullIndices);
		}





	};


class QuickSortPointers
{
public:
	void qsort(void **base,PxI32 num); // perform the qsort.
protected:
	// -1 less, 0 equal, +1 greater.
	virtual PxI32 compare(void **p1,void **p2) = 0;
private:
	void inline swap(char **a,char **b);
};

void QuickSortPointers::swap(char **a,char **b)
{
	char *tmp;

	if ( a != b )
	{
		tmp = *a;
		*a++ = *b;
		*b++ = tmp;
	}
}


void QuickSortPointers::qsort(void **b,PxI32 num)
{
	char *lo,*hi;
	char *mid;
	char *bottom, *top;
	PxI32 size;
	char *lostk[30], *histk[30];
	PxI32 stkptr;
	char **base = (char **)b;

	if (num < 2 ) return;

	stkptr = 0;

	lo = (char *)base;
	hi = (char *)base + sizeof(char **) * (num-1);

nextone:

	size = (hi - lo) / sizeof(char**) + 1;

	mid = lo + (size / 2) * sizeof(char **);
	swap((char **)mid,(char **)lo);
	bottom = lo;
	top = hi + sizeof(char **);

	for (;;)
	{
		do
		{
			bottom += sizeof(char **);
		} while (bottom <= hi && compare((void **)bottom,(void **)lo) <= 0);

		do
		{
			top -= sizeof(char **);
		} while (top > lo && compare((void **)top,(void **)lo) >= 0);

		if (top < bottom) break;

		swap((char **)bottom,(char **)top);

	}

	swap((char **)lo,(char **)top);

	if ( top - 1 - lo >= hi - bottom )
	{
		if (lo + sizeof(char **) < top)
		{
			lostk[stkptr] = lo;
			histk[stkptr] = top - sizeof(char **);
			stkptr++;
		}
		if (bottom < hi)
		{
			lo = bottom;
			goto nextone;
		}
	}
	else
	{
		if ( bottom < hi )
		{
			lostk[stkptr] = bottom;
			histk[stkptr] = hi;
			stkptr++;
		}
		if (lo + sizeof(char **) < top)
		{
			hi = top - sizeof(char **);
			goto nextone; 					/* do small recursion */
		}
	}

	stkptr--;

	if (stkptr >= 0)
	{
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto nextone;
	}
	return;
}



class ConvexDecompInterface
{
public:
		virtual void ConvexDecompResult(MyConvexResult &result) = 0;
};



class Cdesc
{
public:
  ConvexDecompInterface *mCallback;
  physx::PxF32                 mMasterVolume;
  bool                   mUseIslandGeneration;
  bool						mClosedSplit;
  physx::PxF32                 mMasterMeshVolume;
  physx::PxU32           mMaxDepth;
  physx::PxF32                 mConcavePercent;
  physx::PxF32                 mMergePercent;
  physx::PxF32                 mMeshVolumePercent;
};

template <class Type> class Vector3d
{
public:
	Vector3d(void) { };  // null constructor, does not inialize point.

	Vector3d(const Vector3d &a) // constructor copies existing vector.
	{
		x = a.x;
		y = a.y;
		z = a.z;
	};

	Vector3d(Type a,Type b,Type c) // construct with initial point.
	{
		x = a;
		y = b;
		z = c;
	};

	Vector3d(const physx::PxF32 *t)
	{
		x = t[0];
		y = t[1];
		z = t[2];
	};

  void Set(const physx::PxF32 *p)
  {
    x = (Type)p[0];
    y = (Type)p[1];
    z = (Type)p[2];
  }



	const Type* Ptr() const { return &x; }
	Type* Ptr() { return &x; }


	Type x;
	Type y;
	Type z;
};



#define WSCALE 4
#define CONCAVE_THRESH 0.05f


class Wpoint
{
public:
  Wpoint(const Vector3d<physx::PxF32> &p,physx::PxF32 w)
  {
    mPoint = p;
    mWeight = w;
  }

  Vector3d<physx::PxF32> mPoint;
  physx::PxF32           mWeight;
};

typedef physx::Array< Wpoint > WpointVector;


class CTri
{
public:
	CTri(void) { };

  CTri(const physx::PxF32 *p1,const physx::PxF32 *p2,const physx::PxF32 *p3,physx::PxU32 i1,physx::PxU32 i2,physx::PxU32 i3)
  {
    mProcessed = 0;
    mI1 = i1;
    mI2 = i2;
    mI3 = i3;

  	mP1.Set(p1);
  	mP2.Set(p2);
  	mP3.Set(p3);
  	mPlaneD = fm_computePlane(mP1.Ptr(),mP2.Ptr(),mP3.Ptr(),mNormal.Ptr());
	}

  physx::PxF32 Facing(const CTri &t)
  {
		physx::PxF32 d = fm_dot(mNormal.Ptr(),t.mNormal.Ptr());
		return d;
  }

  // clip this line segment against this triangle.
  bool clip(const Vector3d<physx::PxF32> &start,Vector3d<physx::PxF32> &end) const
  {
    Vector3d<physx::PxF32> sect;

    bool hit = fm_lineIntersectsTriangle(start.Ptr(), end.Ptr(), mP1.Ptr(), mP2.Ptr(), mP3.Ptr(), sect.Ptr() );

    if ( hit )
    {
      end = sect;
    }
    return hit;
  }

	bool Concave(const Vector3d<physx::PxF32> &p,physx::PxF32 &distance,Vector3d<physx::PxF32> &n) const
	{
    fm_nearestPointInTriangle(p.Ptr(),mP1.Ptr(),mP2.Ptr(),mP3.Ptr(),n.Ptr());
    distance = fm_distance(p.Ptr(),n.Ptr());
		return true;
	}

	void addTri(physx::PxU32 *indices,physx::PxU32 i1,physx::PxU32 i2,physx::PxU32 i3,physx::PxU32 &tcount) const
	{
		indices[tcount*3+0] = i1;
		indices[tcount*3+1] = i2;
		indices[tcount*3+2] = i3;
		tcount++;
	}

	physx::PxF32 getVolume(ConvexDecompInterface *) const
	{
		physx::PxU32 indices[8*3];


    physx::PxU32 tcount = 0;

    addTri(indices,0,1,2,tcount);
    addTri(indices,3,4,5,tcount);

    addTri(indices,0,3,4,tcount);
    addTri(indices,0,4,1,tcount);

    addTri(indices,1,4,5,tcount);
    addTri(indices,1,5,2,tcount);

    addTri(indices,0,3,5,tcount);
    addTri(indices,0,5,2,tcount);

		physx::PxF32 v = fm_computeMeshVolume(mP1.Ptr(), tcount, indices );

		return v;

	}

	physx::PxF32 raySect(const Vector3d<physx::PxF32> &p,const Vector3d<physx::PxF32> &dir,Vector3d<physx::PxF32> &sect) const
	{
		physx::PxF32 plane[4];

    plane[0] = mNormal.x;
    plane[1] = mNormal.y;
    plane[2] = mNormal.z;
    plane[3] = mPlaneD;

		Vector3d<physx::PxF32> dest;

    dest.x = p.x+dir.x*10000;
    dest.y = p.y+dir.y*10000;
    dest.z = p.z+dir.z*10000;


    fm_intersectPointPlane( p.Ptr(), dest.Ptr(), sect.Ptr(), plane );

    return fm_distance(sect.Ptr(),p.Ptr()); // return the intersection distance.

	}

  physx::PxF32 planeDistance(const Vector3d<physx::PxF32> &p) const
  {
		physx::PxF32 plane[4];

    plane[0] = mNormal.x;
    plane[1] = mNormal.y;
    plane[2] = mNormal.z;
    plane[3] = mPlaneD;

		return fm_distToPlane(plane,p.Ptr());

  }

	bool samePlane(const CTri &t) const
	{
		const physx::PxF32 THRESH = 0.001f;
    physx::PxF32 dd = fabs( t.mPlaneD - mPlaneD );
    if ( dd > THRESH ) return false;
    dd = fabs( t.mNormal.x - mNormal.x );
    if ( dd > THRESH ) return false;
    dd = fabs( t.mNormal.y - mNormal.y );
    if ( dd > THRESH ) return false;
    dd = fabs( t.mNormal.z - mNormal.z );
    if ( dd > THRESH ) return false;
    return true;
	}

	bool hasIndex(physx::PxU32 i) const
	{
		if ( i == mI1 || i == mI2 || i == mI3 ) return true;
		return false;
	}

  bool sharesEdge(const CTri &t) const
  {
    bool ret = false;
    physx::PxU32 count = 0;

		if ( t.hasIndex(mI1) ) count++;
	  if ( t.hasIndex(mI2) ) count++;
		if ( t.hasIndex(mI3) ) count++;

    if ( count >= 2 ) ret = true;

    return ret;
  }

  physx::PxF32 area(void)
  {
		physx::PxF32 a = mConcavity * fm_areaTriangle(mP1.Ptr(),mP2.Ptr(),mP3.Ptr());
    return a;
  }

  void addWeighted(WpointVector &list,ConvexDecompInterface * /* callback */)
  {

    Wpoint p1(mP1,mC1);
    Wpoint p2(mP2,mC2);
    Wpoint p3(mP3,mC3);

    Vector3d<physx::PxF32> d1,d2,d3;

    fm_subtract(mNear1.Ptr(),mP1.Ptr(),d1.Ptr());
    fm_subtract(mNear2.Ptr(),mP2.Ptr(),d2.Ptr());
    fm_subtract(mNear3.Ptr(),mP3.Ptr(),d3.Ptr());

    fm_multiply(d1.Ptr(),WSCALE);
    fm_multiply(d2.Ptr(),WSCALE);
    fm_multiply(d3.Ptr(),WSCALE);

    fm_add(d1.Ptr(), mP1.Ptr(), d1.Ptr());
    fm_add(d2.Ptr(), mP2.Ptr(), d2.Ptr());
    fm_add(d3.Ptr(), mP3.Ptr(), d3.Ptr());

    Wpoint p4(d1,mC1);
    Wpoint p5(d2,mC2);
    Wpoint p6(d3,mC3);

    list.pushBack(p1);
    list.pushBack(p2);
    list.pushBack(p3);

    list.pushBack(p4);
    list.pushBack(p5);
    list.pushBack(p6);

  }

  Vector3d<physx::PxF32>	mP1;
  Vector3d<physx::PxF32>	mP2;
  Vector3d<physx::PxF32>	mP3;
  Vector3d<physx::PxF32> mNear1;
  Vector3d<physx::PxF32> mNear2;
  Vector3d<physx::PxF32> mNear3;
  Vector3d<physx::PxF32> mNormal;
  physx::PxF32           mPlaneD;
  physx::PxF32           mConcavity;
  physx::PxF32           mC1;
  physx::PxF32           mC2;
  physx::PxF32           mC3;
  physx::PxU32    mI1;
  physx::PxU32    mI2;
  physx::PxU32    mI3;
  physx::PxI32             mProcessed; // already been added...
};

typedef physx::Array< CTri > CTriVector;

bool featureMatch(CTri &m,const CTriVector &tris,ConvexDecompInterface * /* callback */,const CTriVector & /* input_mesh */)
{

  bool ret = false;

  physx::PxF32 neardot = 0.707f;

  m.mConcavity = 0;

  CTriVector::ConstIterator i;

	CTri nearest;

	for (i=tris.begin(); i!=tris.end(); ++i)
	{
		const CTri &t = (*i);


		if ( t.samePlane(m) )
		{
			ret = false;
			break;
		}

	  physx::PxF32 dot = fm_dot(t.mNormal.Ptr(),m.mNormal.Ptr());

	  if ( dot > neardot )
	  {

      physx::PxF32 d1 = t.planeDistance( m.mP1 );
      physx::PxF32 d2 = t.planeDistance( m.mP2 );
      physx::PxF32 d3 = t.planeDistance( m.mP3 );

      if ( d1 > 0.001f || d2 > 0.001f || d3 > 0.001f ) // can't be near coplaner!
      {

  	  	neardot = dot;

        Vector3d<physx::PxF32> n1,n2,n3;

        t.raySect( m.mP1, m.mNormal, m.mNear1 );
        t.raySect( m.mP2, m.mNormal, m.mNear2 );
        t.raySect( m.mP3, m.mNormal, m.mNear3 );

				nearest = t;

	  		ret = true;
		  }

	  }
	}

	if ( ret )
	{
		m.mC1 = fm_distance(m.mP1.Ptr(), m.mNear1.Ptr() );
		m.mC2 = fm_distance(m.mP2.Ptr(), m.mNear2.Ptr() );
		m.mC3 = fm_distance(m.mP3.Ptr(), m.mNear3.Ptr() );

		m.mConcavity = m.mC1;

		if ( m.mC2 > m.mConcavity ) m.mConcavity = m.mC2;
		if ( m.mC3 > m.mConcavity ) m.mConcavity = m.mC3;


	}
	return ret;
}

bool isFeatureTri(CTri &t,CTriVector &flist,physx::PxF32 fc,ConvexDecompInterface * /* callback */,physx::PxU32 /* color */)
{
  bool ret = false;

  if ( t.mProcessed == 0 ) // if not already processed
  {

    physx::PxF32 c = t.mConcavity / fc; // must be within 80% of the concavity of the parent.

    if ( c > 0.85f )
    {
      // see if this triangle is a 'feature' triangle.  Meaning it shares an
      // edge with any existing feature triangle and is within roughly the same
      // concavity of the parent.
			if ( flist.size() )
			{
			  CTriVector::Iterator i;
			  for (i=flist.begin(); i!=flist.end(); ++i)
			  {
				  CTri &ftri = (*i);
				  if ( ftri.sharesEdge(t) )
				  {
					  t.mProcessed = 2; // it is now part of a feature.
					  flist.pushBack(t); // add it to the feature list.
					  ret = true;
					  break;
          }
				}
			}
			else
			{
				t.mProcessed = 2;
				flist.pushBack(t); // add it to the feature list.
				ret = true;
			}
    }
    else
    {
      t.mProcessed = 1; // eliminated for this feature, but might be valid for the next one..
    }

  }
  return ret;
}

physx::PxF32 computeConcavity(physx::PxU32 vcount,
                       const physx::PxF32 *vertices,
                       physx::PxU32 tcount,
                       const physx::PxU32 *indices,
                       ConvexDecompInterface *callback,
                       physx::PxF32 *plane,      // plane equation to split on
                       physx::PxF32 &volume)
{


	physx::PxF32 cret = 0;
	volume = 1;

	HullResult  result;
  HullLibrary hl;
  HullDesc    desc;

	desc.mMaxVertices = 256;
	desc.SetHullFlag(QF_TRIANGLES);


  desc.mVcount       = vcount;
  desc.mVertices     = vertices;
  desc.mVertexStride = sizeof(physx::PxF32)*3;

  HullError ret = hl.CreateConvexHull(desc,result);

  if ( ret == QE_OK )
  {

		physx::PxF32 bmin[3];
		physx::PxF32 bmax[3];

    fm_computeBestFitAABB( result.mNumOutputVertices, result.mOutputVertices, sizeof(physx::PxF32)*3, bmin, bmax );

		physx::PxF32 dx = bmax[0] - bmin[0];
		physx::PxF32 dy = bmax[1] - bmin[1];
		physx::PxF32 dz = bmax[2] - bmin[2];

		Vector3d<physx::PxF32> center;

		center.x = bmin[0] + dx*0.5f;
		center.y = bmin[1] + dy*0.5f;
		center.z = bmin[2] + dz*0.5f;

		volume = fm_computeMeshVolume( result.mOutputVertices, result.mNumFaces, result.mIndices );

#if 1
		// ok..now..for each triangle on the original mesh..
		// we extrude the points to the nearest point on the hull.
		const physx::PxU32 *source = result.mIndices;

		CTriVector tris;

    for (physx::PxU32 i=0; i<result.mNumFaces; i++)
    {
    	physx::PxU32 i1 = *source++;
    	physx::PxU32 i2 = *source++;
    	physx::PxU32 i3 = *source++;

    	const physx::PxF32 *p1 = &result.mOutputVertices[i1*3];
    	const physx::PxF32 *p2 = &result.mOutputVertices[i2*3];
    	const physx::PxF32 *p3 = &result.mOutputVertices[i3*3];

			CTri t(p1,p2,p3,i1,i2,i3); //
			tris.pushBack(t);
		}

    // we have not pre-computed the plane equation for each triangle in the convex hull..

		physx::PxF32 totalVolume = 0;

		CTriVector ftris; // 'feature' triangles.

		const physx::PxU32 *src = indices;


    physx::PxF32 maxc=0;


		{
      CTriVector input_mesh;
      {
		    const physx::PxU32 *src = indices;
  			for (physx::PxU32 i=0; i<tcount; i++)
  			{

      		physx::PxU32 i1 = *src++;
      		physx::PxU32 i2 = *src++;
      		physx::PxU32 i3 = *src++;

      		const physx::PxF32 *p1 = &vertices[i1*3];
      		const physx::PxF32 *p2 = &vertices[i2*3];
      		const physx::PxF32 *p3 = &vertices[i3*3];

   				CTri t(p1,p2,p3,i1,i2,i3);
          input_mesh.pushBack(t);
        }
      }

      CTri  maxctri;

			for (physx::PxU32 i=0; i<tcount; i++)
			{

    		physx::PxU32 i1 = *src++;
    		physx::PxU32 i2 = *src++;
    		physx::PxU32 i3 = *src++;

    		const physx::PxF32 *p1 = &vertices[i1*3];
    		const physx::PxF32 *p2 = &vertices[i2*3];
    		const physx::PxF32 *p3 = &vertices[i3*3];

 				CTri t(p1,p2,p3,i1,i2,i3);

				featureMatch(t, tris, callback, input_mesh );

				if ( t.mConcavity > CONCAVE_THRESH )
				{

          if ( t.mConcavity > maxc )
          {
            maxc = t.mConcavity;
            maxctri = t;
          }

  				physx::PxF32 v = t.getVolume(0);
  				totalVolume+=v;
   				ftris.pushBack(t);
   			}

			}
		}
   	fm_computeSplitPlane( vcount, vertices, tcount, indices, plane );
#endif

		cret = totalVolume;

	  hl.ReleaseResult(result);
  }


	return cret;
}





class FaceTri
{
public:
	FaceTri(void) { };

  FaceTri(const physx::PxF32 *vertices,physx::PxU32 i1,physx::PxU32 i2,physx::PxU32 i3)
  {
  	fm_copy3(&vertices[i1*3],mP1 );
  	fm_copy3(&vertices[i2*3],mP2 );
  	fm_copy3(&vertices[i3*3],mP3 );
  }

  physx::PxF32	mP1[3];
  physx::PxF32	mP2[3];
  physx::PxF32 	mP3[3];
  physx::PxF32  mNormal[3];

};





class CHull : public UserAllocated
{
public:
  CHull(const MyConvexResult &result)
  {
	  if ( result.mHullVertices )
	  {
		mResult = PX_NEW(MyConvexResult)(result);
		mVolume = fm_computeMeshVolume( result.mHullVertices, result.mHullTcount, result.mHullIndices );
		mDiagonal = fm_computeBestFitAABB( result.mHullVcount, result.mHullVertices, sizeof(physx::PxF32)*3, mMin, mMax );

		physx::PxF32 dx = mMax[0] - mMin[0];
		physx::PxF32 dy = mMax[1] - mMin[1];
		physx::PxF32 dz = mMax[2] - mMin[2];

		dx*=0.1f; // inflate 1/10th on each edge
		dy*=0.1f; // inflate 1/10th on each edge
		dz*=0.1f; // inflate 1/10th on each edge

		mMin[0]-=dx;
		mMin[1]-=dy;
		mMin[2]-=dz;

		mMax[0]+=dx;
		mMax[1]+=dy;
		mMax[2]+=dz;
	  }
	  else
	  {
		  mResult = NULL;
		  mVolume = 0;
		  mDiagonal = 0;
	  }


  }

  ~CHull(void)
  {
    delete mResult;
  }

  bool overlap(const CHull &h) const
  {
    return fm_intersectAABB(mMin,mMax, h.mMin, h.mMax );
  }

  physx::PxF32          mMin[3];
  physx::PxF32          mMax[3];
	physx::PxF32          mVolume;
  physx::PxF32          mDiagonal; // long edge..
  MyConvexResult  *mResult;
};

// Usage: std::sort( list.begin(), list.end(), StringSortRef() );
class CHullSort
{
	public:

	 bool operator()(const CHull *a,const CHull *b) const
	 {
		 return a->mVolume < b->mVolume;
	 }
};


typedef physx::Array< CHull * > CHullVector;
typedef physx::Array<MyConvexResult *> ConvexResultVector;


class ConvexBuilder : public ConvexDecompInterface, public QuickSortPointers, public HACD::HACD_API::UserCallback
{
public:
	ConvexBuilder(void)
	{
	};

	virtual ~ConvexBuilder(void)
	{
		CHullVector::Iterator i;
		for (i=mChulls.begin(); i!=mChulls.end(); ++i)
		{
			CHull *cr = (*i);
			delete cr;
		}
		for (ConvexResultVector::Iterator i=mResults.begin(); i!=mResults.end(); ++i)
		{
			MyConvexResult *cr = (*i);
			delete cr;
		}
	}

	virtual bool hacdProgressUpdate(const char * /*message*/, physx::PxF32 /*progress*/, physx::PxF32 /*concavity*/, physx::PxU32 /*nVertices*/) 
	{
//		printf("%s",message);
		return true;
	}

	bool isDuplicate(physx::PxU32 i1,physx::PxU32 i2,physx::PxU32 i3,physx::PxU32 ci1,physx::PxU32 ci2,physx::PxU32 ci3)
	{
		physx::PxU32 dcount = 0;

		assert( i1 != i2 && i1 != i3 && i2 != i3 );
		assert( ci1 != ci2 && ci1 != ci3 && ci2 != ci3 );

		if ( i1 == ci1 || i1 == ci2 || i1 == ci3 ) dcount++;
		if ( i2 == ci1 || i2 == ci2 || i2 == ci3 ) dcount++;
		if ( i3 == ci1 || i3 == ci2 || i3 == ci3 ) dcount++;

		return dcount == 3;
	}

	void getMesh(const MyConvexResult &cr,fm_VertexIndex *vc)
	{
		physx::PxU32 *src = cr.mHullIndices;

		for (physx::PxU32 i=0; i<cr.mHullTcount; i++)
		{
			size_t i1 = *src++;
			size_t i2 = *src++;
			size_t i3 = *src++;

			const physx::PxF32 *p1 = &cr.mHullVertices[i1*3];
			const physx::PxF32 *p2 = &cr.mHullVertices[i2*3];
			const physx::PxF32 *p3 = &cr.mHullVertices[i3*3];
			bool newPos;
			i1 = vc->getIndex(p1,newPos);
			i2 = vc->getIndex(p2,newPos);
			i3 = vc->getIndex(p3,newPos);
		}
	}

	CHull * canMerge(CHull *a,CHull *b)
	{
		if ( !a->overlap(*b) ) return 0; // if their AABB's (with a little slop) don't overlap, then return.

		if ( mMergePercent < 0 ) return 0;

		assert( a->mVolume > 0 );
		assert( b->mVolume > 0 );

		CHull *ret = 0;

		// ok..we are going to combine both meshes into a single mesh
		// and then we are going to compute the concavity...

		fm_VertexIndex *vc = fm_createVertexIndex((physx::PxF32)EPSILON,false);

		getMesh( *a->mResult, vc);
		getMesh( *b->mResult, vc);

		size_t vcount = vc->getVcount();
		const physx::PxF32 *vertices = vc->getVerticesFloat();

		HullResult hresult;
		HullLibrary hl;
		HullDesc   desc;

		desc.SetHullFlag(QF_TRIANGLES);

		desc.mVcount       = (physx::PxU32)vcount;
		desc.mVertices     = vertices;
		desc.mVertexStride = sizeof(physx::PxF32)*3;

		HullError hret = hl.CreateConvexHull(desc,hresult);

		if ( hret == QE_OK )
		{
			physx::PxF32 combineVolume  = fm_computeMeshVolume( hresult.mOutputVertices, hresult.mNumFaces, hresult.mIndices );
			physx::PxF32 sumVolume      = a->mVolume + b->mVolume;

			physx::PxF32 percent = (sumVolume*100) / combineVolume;

			if ( percent >= (100.0f-mMergePercent)  )
			{
				MyConvexResult cr(hresult.mNumOutputVertices, hresult.mOutputVertices, hresult.mNumFaces, hresult.mIndices);
				ret = PX_NEW(CHull)(cr);
			}
			hl.ReleaseResult(hresult);
		}
		fm_releaseVertexIndex(vc);
		return ret;
	}

	bool combineHulls(void)
	{

		bool combine = false;

		sortChulls(mChulls); // sort the convex hulls, largest volume to least...

		CHullVector output; // the output hulls...


		CHullVector::Iterator i;

		for (i=mChulls.begin(); i!=mChulls.end() && !combine; ++i)
		{
			CHull *cr = (*i);

			CHullVector::Iterator j = i + 1;
			for (; j!=mChulls.end(); ++j)
			{
				CHull *match = (*j);

				if ( cr != match ) // don't try to merge a hull with itself, that be stoopid
				{

					CHull *merge = canMerge(cr,match); // if we can merge these two....
					if ( !merge )
					{
						merge = canMerge(match,cr);
					}

					if ( merge )
					{
						output.pushBack(merge);
						++i;
						while ( i != mChulls.end() )
						{
							CHull *cr = (*i);
							if ( cr != match )
							{
							output.pushBack(cr);
						}
							i++;
						}

						delete cr;
						delete match;
						combine = true;
						break;
					}
				}
			}

			if ( combine )
			{
				break;
			}
			else
			{
				output.pushBack(cr);
			}
		}

		if ( combine )
		{
			mChulls.clear();
			mChulls = output;
			output.clear();
		}


		return combine;
	}

	MyConvexResult * getHull(PxU32 index)
	{
		return mResults[index];
	}

	physx::PxU32 process(const DecompDesc &desc)
	{

		physx::PxU32 ret = 0;

		Cdesc cdesc;
		cdesc.mMaxDepth         = desc.mDepth;
		cdesc.mConcavePercent   = desc.mCpercent;
		cdesc.mMeshVolumePercent = desc.mVpercent;
		mMergePercent = cdesc.mMergePercent     = desc.mPpercent;
		cdesc.mUseIslandGeneration = desc.mUseIslandGeneration;
		cdesc.mClosedSplit  = desc.mClosedSplit;
		cdesc.mCallback = this;


		HullResult result;
		HullLibrary hl;
		HullDesc   hdesc;
		hdesc.SetHullFlag(QF_TRIANGLES);
		hdesc.mVcount       = desc.mVcount;
		hdesc.mVertices     = desc.mVertices;
		hdesc.mVertexStride = sizeof(physx::PxF32)*3;
		hdesc.mMaxVertices  = desc.mMaxVertices; // maximum number of vertices allowed in the output
		HullError eret = hl.CreateConvexHull(hdesc,result);

		if ( eret == QE_OK )
		{
			cdesc.mMasterVolume = fm_computeMeshVolume( result.mOutputVertices, result.mNumFaces, result.mIndices ); // the volume of the hull.
			cdesc.mMasterMeshVolume = fm_computeMeshVolume( desc.mVertices, desc.mTcount, desc.mIndices );
			hl.ReleaseResult(result);


			if ( desc.mUseHACD )
			{
				HACD::HACD_API *hacd = HACD::createHACD_API();
				if ( hacd )
				{
					HACD::HACD_API::Desc hdesc;
					hdesc.mCallback		= this;
					hdesc.mTriangleCount = desc.mTcount;
					hdesc.mIndices		= desc.mIndices;
					hdesc.mVertexCount	= desc.mVcount;
					hdesc.mVertices		= desc.mVertices;
					hdesc.mMinHullCount	= desc.mMinClusterSizeHACD;
					hdesc.mConcavity	= desc.mConcavityHACD;
					hdesc.mMaxHullVertices	= desc.mMaxVertices;
					hdesc.mConnectDistance	= desc.mConnectionDistanceHACD;
					physx::PxU32 hullCount = hacd->performHACD(hdesc);
					for (physx::PxU32 i=0; i<hullCount; i++)
					{
						const HACD::HACD_API::Hull *hull = hacd->getHull(i);
						if ( hull )
						{
							MyConvexResult result(hull->mVertexCount,hull->mVertices,hull->mTriangleCount,hull->mIndices);
							ConvexDecompResult(result);
						}
					}
					hacd->release();
				}
			}
			else
			{
				const physx::PxU32 *indices = desc.mIndices;
				size_t tcount               = desc.mTcount;
				doConvexDecomposition(desc.mVcount, desc.mVertices, tcount, indices, cdesc, 0);
			}


			while ( combineHulls() ); // keep combinging hulls until I can't combine any more...

			CHullVector::Iterator i;
			for (i=mChulls.begin(); i!=mChulls.end(); ++i)
			{
				CHull *cr = (*i);

				// before we hand it back to the application, we need to regenerate the hull based on the
				// limits given by the user.

				const MyConvexResult &c = *cr->mResult; // the high resolution hull...

				HullResult result;
				HullLibrary hl;
				HullDesc   hdesc;

				hdesc.SetHullFlag(QF_TRIANGLES);

				hdesc.mVcount       = c.mHullVcount;
				hdesc.mVertices     = c.mHullVertices;
				hdesc.mVertexStride = sizeof(physx::PxF32)*3;
				hdesc.mMaxVertices  = desc.mMaxVertices; // maximum number of vertices allowed in the output

				if ( desc.mSkinWidth > 0 )
				{
					hdesc.mSkinWidth = desc.mSkinWidth;
					hdesc.SetHullFlag(QF_SKIN_WIDTH); // do skin width computation.
				}

				HullError ret = hl.CreateConvexHull(hdesc,result);

				if ( ret == QE_OK )
				{
					MyConvexResult *r = PX_NEW(MyConvexResult)(result.mNumOutputVertices, result.mOutputVertices, result.mNumFaces, result.mIndices);
					r->mHullVolume = fm_computeMeshVolume( result.mOutputVertices, result.mNumFaces, result.mIndices ); // the volume of the hull.
					mResults.pushBack(r);
					hl.ReleaseResult(result);
				}
				delete cr;
			}
			mChulls.clear();
			ret = mResults.size();
		}

		return ret;
	}


  virtual void ConvexDecompResult(MyConvexResult &result)
  {
    CHull *ch = PX_NEW(CHull)(result);
    if ( ch->mVolume > 0.00001f )
    {
		  mChulls.pushBack(ch);
    }
    else
    {
      delete ch;
    }
  }

  	virtual PxI32 compare(void **p1,void **p2)
	{
		CHull **cp1 = (CHull **)p1;
		CHull **cp2 = (CHull **)p2;
		CHull *h1 = cp1[0];
		CHull *h2 = cp2[0];

		if ( h1->mVolume > h2->mVolume )
			return -1;
		else if ( h1->mVolume < h2->mVolume )
			return 1;
		return 0;
	}

	void sortChulls(CHullVector & hulls)
	{
        if ( hulls.size() )
        {
		    CHull **hptr = &hulls[0];
		    QuickSortPointers::qsort((void **)hptr,hulls.size());
        }
	}

  bool addTri(fm_VertexIndex *vl,
              UintVector &list,
              const physx::PxF32 *p1,
              const physx::PxF32 *p2,
              const physx::PxF32 *p3)
  {
    bool ret = false;

    bool newPos;
    physx::PxU32 i1 = vl->getIndex(p1,newPos );
    physx::PxU32 i2 = vl->getIndex(p2,newPos );
    physx::PxU32 i3 = vl->getIndex(p3,newPos );

    // do *not* process degenerate triangles!

    if ( i1 != i2 && i1 != i3 && i2 != i3 )
    {

      list.pushBack(i1);
      list.pushBack(i2);
      list.pushBack(i3);
      ret = true;
    }
    return ret;
  }

#pragma warning(disable:4702)



  void doConvexDecomposition(physx::PxU32           vcount,
                             const physx::PxF32           *vertices,
                             physx::PxU32           tcount,
                             const physx::PxU32    *indices,
                             const Cdesc            &cdesc,
                             physx::PxU32           depth)

  {

    physx::PxF32 plane[4];

    bool split = false;

    bool isCoplanar = fm_isMeshCoplanar(tcount,indices,vertices,true);

    if ( isCoplanar ) // we can't do convex decomposition on co-planar meshes!
    {
      // skipping co-planar mesh here...
    }
    else
    {
      if ( depth < cdesc.mMaxDepth )
      {
        if ( cdesc.mConcavePercent >= 0 )
        {
      		physx::PxF32 volume;
      		physx::PxF32 c = computeConcavity( vcount, vertices, tcount, indices, cdesc.mCallback, plane, volume );
      		physx::PxF32 percent = (c*100.0f)/cdesc.mMasterVolume;
      		if ( percent > cdesc.mConcavePercent ) // if great than 5% of the total volume is concave, go ahead and keep splitting.
      		{
            split = true;
          }
        }


        physx::PxF32 mvolume = fm_computeMeshVolume(vertices, tcount, indices );
        physx::PxF32 mpercent = (mvolume*100.0f)/cdesc.mMasterMeshVolume;
        if ( mpercent < cdesc.mMeshVolumePercent )
        {
          split = false; // it's too tiny to bother with!
        }

        if ( split )
        {
          split = fm_computeSplitPlane(vcount,vertices,tcount,indices,plane);
        }

      }

      if ( depth >= cdesc.mMaxDepth || !split )
      {

        HullResult result;
        HullLibrary hl;
        HullDesc   desc;

      	desc.SetHullFlag(QF_TRIANGLES);

        desc.mVcount       = vcount;
        desc.mVertices     = vertices;
        desc.mVertexStride = sizeof(physx::PxF32)*3;

        HullError ret = hl.CreateConvexHull(desc,result);

        if ( ret == QE_OK )
        {
    			MyConvexResult r(result.mNumOutputVertices, result.mOutputVertices, result.mNumFaces, result.mIndices);
				cdesc.mCallback->ConvexDecompResult(r);
				hl.ReleaseResult(result);
        }

        return;
      }

//	  printf("Performing split operation.\r\n");
      SPLIT_MESH::SimpleMesh mesh(vcount, tcount, vertices, indices);
      SPLIT_MESH::SimpleMesh leftMesh;
      SPLIT_MESH::SimpleMesh rightMesh;
      SPLIT_MESH::splitMesh(plane,mesh,leftMesh,rightMesh,cdesc.mClosedSplit);
#if 0
	  static int splitCount=0;
	  splitCount++;
	  saveObj(mesh,splitCount,"InputMesh");
	  saveObj(leftMesh,splitCount,"LeftMesh");
	  saveObj(rightMesh,splitCount,"RightMesh");
#endif

      if ( leftMesh.mTcount )
      {
        doConvexDecomposition(leftMesh.mVcount, leftMesh.mVertices, leftMesh.mTcount,leftMesh.mIndices,cdesc,depth+1);
      }

      if ( rightMesh.mTcount )
      {
        doConvexDecomposition(rightMesh.mVcount, rightMesh.mVertices, rightMesh.mTcount,rightMesh.mIndices, cdesc, depth+1);
      }
    }
  }





physx::PxF32          mMergePercent;
CHullVector     mChulls;
ConvexResultVector	mResults;
};

class MyConvexDecomposition : public ConvexDecomposition, public UserAllocated, public ConvexBuilder
{
public:
	MyConvexDecomposition(void)
	{

	}
	~MyConvexDecomposition(void)
	{

	}

	virtual physx::PxU32 performConvexDecomposition(const DecompDesc &desc) // returns the number of hulls produced.
	{
		return ConvexBuilder::process(desc);
	}

	virtual void release(void) 
	{
		delete this;
	}

	virtual ConvexResult * getConvexResult(physx::PxU32 index) 
	{
		MyConvexResult *r = ConvexBuilder::getHull(index);
		return static_cast< ConvexResult *>(r);
	}

	void setEmpty(SPLIT_MESH::SimpleMesh &m)
	{
		m.mVcount = 0;
		m.mVertices = NULL;
		m.mTcount = 0;
		m.mIndices = NULL;
	}

	virtual void splitMesh(const physx::PxF32 *planeEquation,const TriangleMesh &input,TriangleMesh &left,TriangleMesh &right,bool closedMesh) 
	{

		SPLIT_MESH::SimpleMesh sinput(input.mVcount,input.mTriCount,input.mVertices,input.mIndices);
		SPLIT_MESH::SimpleMesh sleft;
		SPLIT_MESH::SimpleMesh sright;

		SPLIT_MESH::splitMesh(planeEquation,sinput,sleft,sright,closedMesh);

		left.mVcount = sleft.mVcount;
		left.mVertices = sleft.mVertices;
		left.mTriCount = sleft.mTcount;
		left.mIndices = sleft.mIndices;
		right.mVcount = sright.mVcount;
		right.mTriCount = sright.mTcount;
		right.mVertices = sright.mVertices;
		right.mIndices = sright.mIndices;

		setEmpty(sleft);
		setEmpty(sright);

	}

	virtual void releaseTriangleMeshMemory(TriangleMesh &mesh) 
	{
		PX_FREE(mesh.mVertices);
		PX_FREE(mesh.mIndices);
		mesh.mIndices = NULL;
		mesh.mVertices = NULL;
		mesh.mTriCount = 0;
		mesh.mVcount = 0;
	}

};


ConvexDecomposition * createConvexDecomposition(void)
{
	MyConvexDecomposition *m = PX_NEW(MyConvexDecomposition);
	return static_cast<ConvexDecomposition *>(m);
}


}; // end of namespace

