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


#include "PsShare.h"
#include "foundation/PxSimpleTypes.h"
#include "PsUserAllocated.h"
#include "PsArray.h"
#include "PsHashMap.h"
#include "RemoveTjunctions.h"
#include "FloatMath.h"
#include <limits.h>

#pragma warning(disable:4189)

namespace physx
{
	namespace general_meshutils2
	{

class AABB : public physx::UserAllocated
{
public:
	physx::PxF32 mMin[3];
  physx::PxF32 mMax[3];
};

bool gDebug=false;
PxU32 gCount=0;

typedef physx::Array< PxU32 > PxU32Vector;

class Triangle : public physx::UserAllocated
{
public:
  Triangle(void)
  {
    mPending = false;
    mSplit = false;
    mI1 = mI2 = mI3 = 0xFFFFFFFF;
    mId = 0;
  }

  Triangle(PxU32 i1,PxU32 i2,PxU32 i3,const float *vertices,PxU32 id)
  {
    mPending = false;
    init(i1,i2,i3,vertices,id);
    mSplit = false;
  }

  void init(PxU32 i1,PxU32 i2,PxU32 i3,const float *vertices,PxU32 id)
  {
    mSplit = false;
    mI1 = i1;
    mI2 = i2;
    mI3 = i3;
    mId = id;

    const float *p1 = &vertices[mI1*3];
    const float *p2 = &vertices[mI2*3];
    const float *p3 = &vertices[mI3*3];

    initMinMax(p1,p2,p3);
  }

  void initMinMax(const float *p1,const float *p2,const float *p3)
  {
	physx::fm_copy3(p1,mBmin);
	physx::fm_copy3(p1,mBmax);
    physx::fm_minmax(p2,mBmin,mBmax);
    physx::fm_minmax(p3,mBmin,mBmax);
  }

  void init(const PxU32 *idx,const float *vertices,PxU32 id)
  {
    mSplit = false;
    mI1 = idx[0];
    mI2 = idx[1];
    mI3 = idx[2];
    mId = id;

    const float *p1 = &vertices[mI1*3];
    const float *p2 = &vertices[mI2*3];
    const float *p3 = &vertices[mI3*3];

    initMinMax(p1,p2,p3);

  }

  bool intersects(const float *pos,const float *p1,const float *p2,float epsilon) const
  {
    bool ret = false;

    float sect[3];
    physx::LineSegmentType type;

    float dist = fm_distancePointLineSegment(pos,p1,p2,sect,type,epsilon);

    if ( type == physx::LS_MIDDLE && dist < epsilon )
    {
      ret = true;
    }

    return ret;
  }

  bool intersects(PxU32 i,const float *vertices,PxU32 &edge,float epsilon) const
  {
    bool ret = true;

    const float *pos = &vertices[i*3];
    const float *p1  = &vertices[mI1*3];
    const float *p2  = &vertices[mI2*3];
    const float *p3  = &vertices[mI3*3];
    if ( intersects(pos,p1,p2,epsilon) )
    {
      edge = 0;
    }
    else if ( intersects(pos,p2,p3,epsilon) )
    {
      edge = 1;
    }
    else if ( intersects(pos,p3,p1,epsilon) )
    {
      edge = 2;
    }
    else
    {
      ret = false;
    }
    return ret;
  }

  bool intersects(const Triangle *t,const float *vertices,PxU32 &intersection_index,PxU32 &edge,float epsilon)
  {
    bool ret = false;

    if ( physx::fm_intersectAABB(mBmin,mBmax,t->mBmin,t->mBmax) ) // only if the AABB's of the two triangles intersect...
    {

      if ( t->intersects(mI1,vertices,edge,epsilon) )
      {
        intersection_index = mI1;
        ret = true;
      }

      if ( t->intersects(mI2,vertices,edge,epsilon) )
      {
        intersection_index = mI2;
        ret = true;
      }

      if ( t->intersects(mI3,vertices,edge,epsilon) )
      {
        intersection_index = mI3;
        ret = true;
      }

    }

    return ret;
  }

  bool    mSplit:1;
  bool    mPending:1;
  PxU32   mI1;
  PxU32   mI2;
  PxU32   mI3;
  PxU32   mId;
  float   mBmin[3];
  float   mBmax[3];
};

class Edge : public physx::UserAllocated
{
public:
  Edge(void)
  {
    mNextEdge = 0;
    mTriangle = 0;
    mHash = 0;
  }

  PxU32 init(Triangle *t,PxU32 i1,PxU32 i2)
  {
    mTriangle = t;
    mNextEdge = 0;
    PX_ASSERT( i1 < 65536 );
    PX_ASSERT( i2 < 65536 );
    if ( i1 < i2 )
    {
      mHash = (i1<<16)|i2;
    }
    else
    {
      mHash = (i2<<16)|i1;
    }
    return mHash;
  }
  Edge     *mNextEdge;
  Triangle *mTriangle;
  PxU32    mHash;
};


typedef physx::Array< Triangle * > TriangleVector;
typedef physx::HashMap< PxU32, Edge * > EdgeMap;

class MyRemoveTjunctions : public RemoveTjunctions, public physx::UserAllocated
{
public:
  MyRemoveTjunctions(void)
  {
    mInputTriangles = 0;
    mEdges = 0;
    mVcount = 0;
    mVertices = 0;
    mEdgeCount = 0;
  }
  virtual ~MyRemoveTjunctions(void)
  {
    release();
  }

  virtual PxU32 removeTjunctions(RemoveTjunctionsDesc &desc)
  {
    PxU32 ret = 0;

	mEpsilon = desc.mEpsilon;

	size_t TcountOut;

    desc.mIndicesOut = removeTjunctions(desc.mVcount, desc.mVertices, desc.mTcount, desc.mIndices, TcountOut, desc.mIds);

	PX_ASSERT( TcountOut < UINT_MAX );
	desc.mTcountOut = (PxU32)TcountOut;

    if ( !mIds.empty() )
    {
      desc.mIdsOut = &mIds[0];
    }

    ret = desc.mTcountOut;

#if 0
    bool check = ret != desc.mTcount;
    while ( check )
    {
        PxU32 tcount = ret;
		PxU32 *indices  = (PxU32 *)physx::PX_ALLOC(sizeof(PxU32)*tcount*3);
		PxU32 *ids      = (PxU32 *)physx::PX_ALLOC(sizeof(PxU32)*tcount);
        memcpy(indices,desc.mIndicesOut,sizeof(PxU32)*ret*3);
        memcpy(ids,desc.mIdsOut,sizeof(PxU32)*ret);
        desc.mIndicesOut = removeTjunctions(desc.mVcount, desc.mVertices, tcount, indices, desc.mTcountOut, ids );
        if ( !mIds.empty() )
        {
          desc.mIdsOut = &mIds[0];
        }
        ret = desc.mTcountOut;
        PX_FREE(indices);
        PX_FREE(ids);
        check = ret != tcount;
    }
#endif
    return ret;
  }

  Edge * addEdge(Triangle *t,Edge *e,PxU32 i1,PxU32 i2)
  {
    PxU32 hash = e->init(t,i1,i2);
    const EdgeMap::Entry *found = mEdgeMap.find(hash);
    if ( found == NULL )
    {
      mEdgeMap[hash] = e;
    }
    else
    {
      Edge *old_edge = (*found).second;
      e->mNextEdge = old_edge;
      mEdgeMap.erase(hash);
      mEdgeMap[hash] = e;
    }
    e++;
    mEdgeCount++;
    return e;
  }

  Edge * init(Triangle *t,const PxU32 *indices,const float *vertices,Edge *e,PxU32 id)
  {
    t->init(indices,vertices,id);
    e = addEdge(t,e,t->mI1,t->mI2);
    e = addEdge(t,e,t->mI2,t->mI3);
    e = addEdge(t,e,t->mI3,t->mI1);
    return e;
  }

  void release(void)
  {
    mIds.clear();
    mEdgeMap.clear();
    mIndices.clear();
    mSplit.clear();
    delete []mInputTriangles;
    delete []mEdges;
    mInputTriangles = 0;
    mEdges = 0;
    mVcount = 0;
    mVertices = 0;
    mEdgeCount = 0;

  }

  virtual PxU32 * removeTjunctions(PxU32 vcount,
                                    const float *vertices,
                                    size_t tcount,
                                    const PxU32 *indices,
                                    size_t &tcount_out,
                                    const PxU32 * ids)
  {
    PxU32 *ret  = 0;

    release();

    mVcount   = vcount;
    mVertices = vertices;
    mTcount   = (PxU32)tcount;
    tcount_out = 0;

    mMaxTcount      = (PxU32)tcount*2;
	mInputTriangles = PX_NEW(Triangle)[mMaxTcount];
    Triangle *t     = mInputTriangles;

    mEdges          = PX_NEW(Edge)[mMaxTcount*3];
    mEdgeCount      = 0;

    PxU32 id = 0;

    Edge *e = mEdges;
    for (PxU32 i=0; i<tcount; i++)
    {
      if ( ids ) id = *ids++;
      e =init(t,indices,vertices,e,id);
      indices+=3;
      t++;
    }

    {
      TriangleVector test;

      EdgeMap::Iterator i = mEdgeMap.getIterator();
      for (; !i.done(); ++i)
      {
        Edge *e = (*i).second;
        if ( e->mNextEdge == 0 ) // open edge!
        {
          Triangle *t = e->mTriangle;
          if ( !t->mPending )
          {
            test.pushBack(t);
            t->mPending = true;
          }
        }
      }

      if ( !test.empty() )
      {
        TriangleVector::Iterator i;
        for (i=test.begin(); i!=test.end(); ++i)
        {
          Triangle *t = (*i);
          locateIntersection(t);
        }
      }

    }

    while ( !mSplit.empty() )
    {
      TriangleVector scan = mSplit;
      mSplit.clear();
      TriangleVector::Iterator i;
      for (i=scan.begin(); i!=scan.end(); ++i)
      {
        Triangle *t = (*i);
        locateIntersection(t);
      }
    }


    mIndices.clear();
    mIds.clear();

    t = mInputTriangles;
    for (PxU32 i=0; i<mTcount; i++)
    {
      mIndices.pushBack(t->mI1);
      mIndices.pushBack(t->mI2);
      mIndices.pushBack(t->mI3);
      mIds.pushBack(t->mId);
      t++;
    }


   mEdgeMap.clear();
   delete []mEdges;
   mEdges = 0;
   delete []mInputTriangles;
   mInputTriangles = 0;
   tcount_out = mIndices.size()/3;
   ret = tcount_out ? &mIndices[0] : 0;
#ifdef _DEBUG
   if ( ret )
   {
	   const PxU32 *scan = ret;
	   for (PxU32 i=0; i<tcount_out; i++)
	   {
		   PxU32 i1 = scan[0];
		   PxU32 i2 = scan[1];
		   PxU32 i3 = scan[2];
		   PX_ASSERT( i1 != i2 && i1 != i3 && i2 != i3 );
		   scan+=3;
	   }
   }
#endif
    return ret;
  }

  Triangle * locateIntersection(Triangle *scan,Triangle *t)
  {
    Triangle *ret = 0;

    PxU32 t1 = (PxU32)(scan-mInputTriangles);
	PX_UNUSED(t1);

    PxU32 t2 = (PxU32)(t-mInputTriangles);
	PX_UNUSED(t2);

    PX_ASSERT( t1 < mTcount );
    PX_ASSERT( t2 < mTcount );

    PX_ASSERT( scan->mI1 < mVcount );
    PX_ASSERT( scan->mI2 < mVcount );
    PX_ASSERT( scan->mI3 < mVcount );

    PX_ASSERT( t->mI1 < mVcount );
    PX_ASSERT( t->mI2 < mVcount );
    PX_ASSERT( t->mI3 < mVcount );


    PxU32 intersection_index = 0;
    PxU32 edge = 0;

    if ( scan != t && scan->intersects(t,mVertices,intersection_index,edge,mEpsilon) )
    {

	  if ( t->mI1 == intersection_index || t->mI2 == intersection_index || t->mI3 == intersection_index )
	  {
	  }
	  else
	  {
		  // here is where it intersects!
		  PxU32 i1,i2,i3;
		  PxU32 j1,j2,j3;
		  PxU32 id = t->mId;

		  switch ( edge )
		  {
			case 0:
			  i1 = t->mI1;
			  i2 = intersection_index;
			  i3 = t->mI3;
			  j1 = intersection_index;
			  j2 = t->mI2;
			  j3 = t->mI3;
			  break;
			case 1:
			  i1 = t->mI2;
			  i2 = intersection_index;
			  i3 = t->mI1;
			  j1 = intersection_index;
			  j2 = t->mI3;
			  j3 = t->mI1;
			  break;
			case 2:
			  i1 = t->mI3;
			  i2 = intersection_index;
			  i3 = t->mI2;
			  j1 = intersection_index;
			  j2 = t->mI1;
			  j3 = t->mI2;
			  break;
			default:
			  PX_ALWAYS_ASSERT();
			  i1 = i2 = i3 = 0;
			  j1 = j2 = j3 = 0;
			  break;
		  }

		  if ( mTcount < mMaxTcount )
		  {
			t->init(i1,i2,i3,mVertices,id);
			Triangle *newt = &mInputTriangles[mTcount];
			newt->init(j1,j2,j3,mVertices,id);
			mTcount++;
			t->mSplit = true;
			newt->mSplit = true;

			mSplit.pushBack(t);
			mSplit.pushBack(newt);
			ret = scan;
		  }
	    }
    }
    return ret;
  }

  Triangle * testIntersection(Triangle *scan,Triangle *t)
  {
    Triangle *ret = 0;

    PxU32 t1 = (PxU32)(scan-mInputTriangles);
	PX_UNUSED(t1);

    PxU32 t2 = (PxU32)(t-mInputTriangles);
	PX_UNUSED(t2);

    PX_ASSERT( t1 < mTcount );
    PX_ASSERT( t2 < mTcount );

    PX_ASSERT( scan->mI1 < mVcount );
    PX_ASSERT( scan->mI2 < mVcount );
    PX_ASSERT( scan->mI3 < mVcount );

    PX_ASSERT( t->mI1 < mVcount );
    PX_ASSERT( t->mI2 < mVcount );
    PX_ASSERT( t->mI3 < mVcount );


    PxU32 intersection_index;
    PxU32 edge;

    PX_ASSERT( scan != t );

    if ( scan->intersects(t,mVertices,intersection_index,edge,mEpsilon) )
    {
      // here is where it intersects!
      PxU32 i1,i2,i3;
      PxU32 j1,j2,j3;
      PxU32 id = t->mId;

      switch ( edge )
      {
        case 0:
          i1 = t->mI1;
          i2 = intersection_index;
          i3 = t->mI3;
          j1 = intersection_index;
          j2 = t->mI2;
          j3 = t->mI3;
          break;
        case 1:
          i1 = t->mI2;
          i2 = intersection_index;
          i3 = t->mI1;
          j1 = intersection_index;
          j2 = t->mI3;
          j3 = t->mI1;
          break;
        case 2:
          i1 = t->mI3;
          i2 = intersection_index;
          i3 = t->mI2;
          j1 = intersection_index;
          j2 = t->mI1;
          j3 = t->mI2;
          break;
        default:
          PX_ALWAYS_ASSERT();
          i1 = i2 = i3 = 0;
          j1 = j2 = j3 = 0;
          break;
      }

      if ( mTcount < mMaxTcount )
      {
        t->init(i1,i2,i3,mVertices,id);
        Triangle *newt = &mInputTriangles[mTcount];
        newt->init(j1,j2,j3,mVertices,id);
        mTcount++;
        t->mSplit = true;
        newt->mSplit = true;

        mSplit.pushBack(t);
        mSplit.pushBack(newt);
        ret = scan;
      }
    }
    return ret;
  }

  Triangle * locateIntersection(Triangle *t)
  {
    Triangle *ret = 0;

    Triangle *scan = mInputTriangles;

    for (PxU32 i=0; i<mTcount; i++)
    {
      ret = locateIntersection(scan,t);
      if ( ret )
        break;
      scan++;
    }
    return ret;
  }


  Triangle             *mInputTriangles;
  PxU32                mVcount;
  PxU32                mMaxTcount;
  PxU32                mTcount;
  const float          *mVertices;
  PxU32Vector          mIndices;
  PxU32Vector          mIds;
  TriangleVector        mSplit;
  PxU32                mEdgeCount;
  Edge                 *mEdges;
  EdgeMap               mEdgeMap;
  PxF32                 mEpsilon;
};



RemoveTjunctions * createRemoveTjunctions(void)
{
  MyRemoveTjunctions *m = PX_NEW(MyRemoveTjunctions);
  return static_cast< RemoveTjunctions *>(m);
}

void               releaseRemoveTjunctions(RemoveTjunctions *tj)
{
  MyRemoveTjunctions *m = static_cast< MyRemoveTjunctions *>(tj);
  delete m;
}

	};
};