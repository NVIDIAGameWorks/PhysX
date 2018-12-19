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
#include "foundation/PxAssert.h"
#include "foundation/PxSimpleTypes.h"
#include "PsUserAllocated.h"

#include "MeshIslandGeneration.h"
#include "FloatMath.h"
#include "PsArray.h"
#include "PsHashMap.h"

#pragma warning(disable:4100)

namespace physx
{
	namespace general_meshutils2
	{

typedef physx::Array< PxU32 > PxU32Vector;

class Edge;
class Island;

class AABB : public physx::UserAllocated
{
public:
  PxF32 mMin[3];
  PxF32 mMax[3];
};

class Triangle : public physx::UserAllocated
{
public:
  Triangle(void)
  {
    mConsumed = false;
    mIsland   = 0;
    mHandle   = 0;
    mId       = 0;
  }

  void minmax(const PxF32 *p,AABB &box)
  {
    if ( p[0] < box.mMin[0] ) box.mMin[0] = p[0];
    if ( p[1] < box.mMin[1] ) box.mMin[1] = p[1];
    if ( p[2] < box.mMin[2] ) box.mMin[2] = p[2];

    if ( p[0] > box.mMax[0] ) box.mMax[0] = p[0];
    if ( p[1] > box.mMax[1] ) box.mMax[1] = p[1];
    if ( p[2] > box.mMax[2] ) box.mMax[2] = p[2];
  }

  void minmax(const PxF64 *p,AABB &box)
  {
    if ( (PxF32)p[0] < box.mMin[0] ) box.mMin[0] = (PxF32)p[0];
    if ( (PxF32)p[1] < box.mMin[1] ) box.mMin[1] = (PxF32)p[1];
    if ( (PxF32)p[2] < box.mMin[2] ) box.mMin[2] = (PxF32)p[2];
    if ( (PxF32)p[0] > box.mMax[0] ) box.mMax[0] = (PxF32)p[0];
    if ( (PxF32)p[1] > box.mMax[1] ) box.mMax[1] = (PxF32)p[1];
    if ( (PxF32)p[2] > box.mMax[2] ) box.mMax[2] = (PxF32)p[2];
  }

  void buildBox(const PxF32 *vertices_f,const PxF64 *vertices_d,PxU32 id);

  void render(PxU32 /*color*/)
  {
//    gRenderDebug->DebugBound(&mBox.mMin[0],&mBox.mMax[0],color,60.0f);
  }

  void getTriangle(PxF32 *tri,const PxF32 *vertices_f,const PxF64 *vertices_d);

  PxU32    mHandle;
  bool      mConsumed;
  Edge     *mEdges[3];
  Island   *mIsland;   // identifies which island it is a member of
  unsigned short  mId;
  AABB      mBox;
};


class Edge : public physx::UserAllocated
{
public:
  Edge(void)
  {
    mI1 = 0;
    mI2 = 0;
    mHash = 0;
    mNext = 0;
    mPrevious = 0;
    mParent = 0;
    mNextTriangleEdge = 0;
  }

  void init(PxU32 i1,PxU32 i2,Triangle *parent)
  {
    PX_ASSERT( i1 < 65536 );
    PX_ASSERT( i2 < 65536 );

    mI1 = i1;
    mI2 = i2;
    mHash        = (i2<<16)|i1;
    mReverseHash = (i1<<16)|i2;
    mNext = 0;
    mPrevious = 0;
    mParent = parent;
  }

  PxU32  mI1;
  PxU32  mI2;
  PxU32  mHash;
  PxU32  mReverseHash;

  Edge     *mNext;
  Edge     *mPrevious;
  Edge     *mNextTriangleEdge;
  Triangle *mParent;
};

typedef physx::HashMap< PxU32, Edge * > EdgeHashMap;
typedef physx::Array< Triangle * > TriangleVector;

class EdgeCheck //: public physx::UserAllocated
{
public:
  EdgeCheck(Triangle *t,Edge *e)
  {
    mTriangle = t;
    mEdge     = e;
  }

  Triangle  *mTriangle;
  Edge      *mEdge;
};

typedef physx::Array< EdgeCheck > EdgeCheckQueue;

class Island  : public physx::UserAllocated
{
public:
  Island(Triangle *t,Triangle * /*root*/)
  {
    mVerticesFloat = 0;
    mVerticesDouble = 0;
    t->mIsland = this;
    mTriangles.pushBack(t);
    mCoplanar = false;
	physx::fm_initMinMax(mMin,mMax);
  }

  void add(Triangle *t,Triangle * /*root*/)
  {
    t->mIsland = this;
    mTriangles.pushBack(t);
  }

  void merge(Island &isl)
  {
    TriangleVector::Iterator i;
    for (i=isl.mTriangles.begin(); i!=isl.mTriangles.end(); ++i)
    {
      Triangle *t = (*i);
      mTriangles.pushBack(t);
    }
    isl.mTriangles.clear();
  }

  bool isTouching(Island *isl,const PxF32 *vertices_f,const PxF64 *vertices_d)
  {
    bool ret = false;

    mVerticesFloat = vertices_f;
    mVerticesDouble = vertices_d;

    if ( physx::fm_intersectAABB(mMin,mMax,isl->mMin,isl->mMax) ) // if the two islands has an intersecting AABB
    {
      // todo..
    }


    return ret;
  }


  void SAP_DeletePair(const void* /*object0*/, const void* /*object1*/, void* /*user_data*/, void* /*pair_user_data*/)
  {
  }

  void render(PxU32 color)
  {
//    gRenderDebug->DebugBound(mMin,mMax,color,60.0f);
    TriangleVector::Iterator i;
    for (i=mTriangles.begin(); i!=mTriangles.end(); ++i)
    {
      Triangle *t = (*i);
      t->render(color);
    }
  }


  const PxF64   *mVerticesDouble;
  const PxF32    *mVerticesFloat;

  PxF32           mMin[3];
  PxF32           mMax[3];
  bool            mCoplanar; // marked as co-planar..
  TriangleVector  mTriangles;
};


void Triangle::getTriangle(PxF32 *tri,const PxF32 *vertices_f,const PxF64 *vertices_d)
{
  PxU32 i1 = mEdges[0]->mI1;
  PxU32 i2 = mEdges[1]->mI1;
  PxU32 i3 = mEdges[2]->mI1;
  if ( vertices_f )
  {
    const PxF32 *p1 = &vertices_f[i1*3];
    const PxF32 *p2 = &vertices_f[i2*3];
    const PxF32 *p3 = &vertices_f[i3*3];
    physx::fm_copy3(p1,tri);
    physx::fm_copy3(p2,tri+3);
    physx::fm_copy3(p3,tri+6);
  }
  else
  {
    const PxF64 *p1 = &vertices_d[i1*3];
    const PxF64 *p2 = &vertices_d[i2*3];
    const PxF64 *p3 = &vertices_d[i3*3];
    physx::fm_doubleToFloat3(p1,tri);
    physx::fm_doubleToFloat3(p2,tri+3);
    physx::fm_doubleToFloat3(p3,tri+6);
  }
}

void Triangle::buildBox(const PxF32 *vertices_f,const PxF64 *vertices_d,PxU32 id)
{
  mId = (unsigned short)id;
  PxU32 i1 = mEdges[0]->mI1;
  PxU32 i2 = mEdges[1]->mI1;
  PxU32 i3 = mEdges[2]->mI1;

  if ( vertices_f )
  {
    const PxF32 *p1 = &vertices_f[i1*3];
    const PxF32 *p2 = &vertices_f[i2*3];
    const PxF32 *p3 = &vertices_f[i3*3];
    mBox.mMin[0] = p1[0];
    mBox.mMin[1] = p1[1];
    mBox.mMin[2] = p1[2];
    mBox.mMax[0] = p1[0];
    mBox.mMax[1] = p1[1];
    mBox.mMax[2] = p1[2];
    minmax(p2,mBox);
    minmax(p3,mBox);
  }
  else
  {
    const PxF64 *p1 = &vertices_d[i1*3];
    const PxF64 *p2 = &vertices_d[i2*3];
    const PxF64 *p3 = &vertices_d[i3*3];
    mBox.mMin[0] = (PxF32)p1[0];
    mBox.mMin[1] = (PxF32)p1[1];
    mBox.mMin[2] = (PxF32)p1[2];
    mBox.mMax[0] = (PxF32)p1[0];
    mBox.mMax[1] = (PxF32)p1[1];
    mBox.mMax[2] = (PxF32)p1[2];
    minmax(p2,mBox);
    minmax(p3,mBox);
  }

  PX_ASSERT(mIsland);
  if ( mIsland )
  {
    if ( mBox.mMin[0] < mIsland->mMin[0] ) mIsland->mMin[0] = mBox.mMin[0];
    if ( mBox.mMin[1] < mIsland->mMin[1] ) mIsland->mMin[1] = mBox.mMin[1];
    if ( mBox.mMin[2] < mIsland->mMin[2] ) mIsland->mMin[2] = mBox.mMin[2];

    if ( mBox.mMax[0] > mIsland->mMax[0] ) mIsland->mMax[0] = mBox.mMax[0];
    if ( mBox.mMax[1] > mIsland->mMax[1] ) mIsland->mMax[1] = mBox.mMax[1];
    if ( mBox.mMax[2] > mIsland->mMax[2] ) mIsland->mMax[2] = mBox.mMax[2];
  }

}


typedef physx::Array< Island * > IslandVector;

class MyMeshIslandGeneration : public MeshIslandGeneration, public physx::UserAllocated
{
public:
  MyMeshIslandGeneration(void)
  {
    mTriangles = 0;
    mEdges     = 0;
    mVerticesDouble = 0;
    mVerticesFloat  = 0;
  }

  virtual ~MyMeshIslandGeneration(void)
  {
    reset();
  }

  void reset(void)
  {
    delete []mTriangles;
    delete []mEdges;
    mTriangles = 0;
    mEdges = 0;
    mTriangleEdges.clear();
    IslandVector::Iterator i;
    for (i=mIslands.begin(); i!=mIslands.end(); ++i)
    {
      Island *_i = (*i);
      delete _i;
    }
    mIslands.clear();
  }

  PxU32 islandGenerate(PxU32 tcount,const PxU32 *indices,const PxF64 *vertices)
  {
    mVerticesDouble = vertices;
    mVerticesFloat  = 0;
    return islandGenerate(tcount,indices);
  }

  PxU32 islandGenerate(PxU32 tcount,const PxU32 *indices,const PxF32 *vertices)
  {
    mVerticesDouble = 0;
    mVerticesFloat  = vertices;
    return islandGenerate(tcount,indices);
  }

  PxU32 islandGenerate(PxU32 tcount,const PxU32 *indices)
  {
    PxU32 ret = 0;

    reset();

    mTcount = tcount;
    mTriangles = PX_NEW(Triangle)[tcount];
    mEdges     = PX_NEW(Edge)[tcount*3];
    Edge *e = mEdges;

    for (PxU32 i=0; i<tcount; i++)
    {
      Triangle &t = mTriangles[i];

      PxU32 i1 = *indices++;
      PxU32 i2 = *indices++;
      PxU32 i3 = *indices++;

      t.mEdges[0] = e;
      t.mEdges[1] = e+1;
      t.mEdges[2] = e+2;

      e = addEdge(e,&t,i1,i2);
      e = addEdge(e,&t,i2,i3);
      e = addEdge(e,&t,i3,i1);

    }

    // while there are still edges to process...
    while ( mTriangleEdges.size() != 0 )
    {

      EdgeHashMap::Iterator iter = mTriangleEdges.getIterator();

      Triangle *t = iter->second->mParent;

      Island *i = PX_NEW(Island)(t,mTriangles);  // the initial triangle...
      removeTriangle(t); // remove this triangle from the triangle-edges hashmap

      mIslands.pushBack(i);

      // now keep adding to this island until we can no longer walk any shared edges..
      addEdgeCheck(t,t->mEdges[0]);
      addEdgeCheck(t,t->mEdges[1]);
      addEdgeCheck(t,t->mEdges[2]);

      while ( !mEdgeCheckQueue.empty() )
      {

        EdgeCheck e = mEdgeCheckQueue.popBack();

        // Process all triangles which share this edge
        Edge *edge = locateSharedEdge(e.mEdge);

        while ( edge )
        {
          Triangle *t = edge->mParent;
          PX_ASSERT(!t->mConsumed);
          i->add(t,mTriangles);
          removeTriangle(t); // remove this triangle from the triangle-edges hashmap

          // now keep adding to this island until we can no longer walk any shared edges..

          if ( edge != t->mEdges[0] )
          {
            addEdgeCheck(t,t->mEdges[0]);
          }

          if ( edge != t->mEdges[1] )
          {
            addEdgeCheck(t,t->mEdges[1]);
          }

          if ( edge != t->mEdges[2] )
          {
            addEdgeCheck(t,t->mEdges[2]);
          }

          edge = locateSharedEdge(e.mEdge); // keep going until all shared edges have been processed!
        }

      }
    }

    ret = (PxU32)mIslands.size();

    return ret;
  }

  PxU32 *   getIsland(PxU32 index,PxU32 &otcount)
  {
    PxU32 *ret  = 0;

    mIndices.clear();
    if ( index < mIslands.size() )
    {
      Island *i = mIslands[index];
      otcount = (PxU32)i->mTriangles.size();
      TriangleVector::Iterator j;
      for (j=i->mTriangles.begin(); j!=i->mTriangles.end(); ++j)
      {
        Triangle *t = (*j);
        mIndices.pushBack(t->mEdges[0]->mI1);
        mIndices.pushBack(t->mEdges[1]->mI1);
        mIndices.pushBack(t->mEdges[2]->mI1);
      }
      ret = &mIndices[0];
    }

    return ret;
  }

private:

  void removeTriangle(Triangle *t)
  {
    t->mConsumed = true;

    removeEdge(t->mEdges[0]);
    removeEdge(t->mEdges[1]);
    removeEdge(t->mEdges[2]);

  }


  Edge * locateSharedEdge(Edge *e)
  {
    Edge *ret = 0;

    const EdgeHashMap::Entry *found = mTriangleEdges.find( e->mReverseHash );
    if ( found != NULL )
    {
      ret = (*found).second;
      PX_ASSERT( ret->mHash == e->mReverseHash );
    }
    return ret;
  }

  void removeEdge(Edge *e)
  {
    const EdgeHashMap::Entry *found = mTriangleEdges.find( e->mHash );

    if ( found != NULL )
    {
      Edge *prev = 0;
      Edge *scan = (*found).second;
      while ( scan && scan != e )
      {
        prev = scan;
        scan = scan->mNextTriangleEdge;
      }

      if ( scan )
      {
        if ( prev == 0 )
        {
          if ( scan->mNextTriangleEdge )
          {
            mTriangleEdges.erase(e->mHash);
            mTriangleEdges[e->mHash] = scan->mNextTriangleEdge;
          }
          else
          {
            mTriangleEdges.erase(e->mHash); // no more polygons have an edge here
          }
        }
        else
        {
          prev->mNextTriangleEdge = scan->mNextTriangleEdge;
        }
      }
      else
      {
        PX_ASSERT(0);
      }
    }
    else
    {
      PX_ASSERT(0); // impossible!
    }
  }


  Edge * addEdge(Edge *e,Triangle *t,PxU32 i1,PxU32 i2)
  {

    e->init(i1,i2,t);

    const EdgeHashMap::Entry *found = mTriangleEdges.find(e->mHash);
    if ( found == NULL )
    {
      mTriangleEdges[ e->mHash ] = e;
    }
    else
    {
      Edge *pn = (*found).second;
      e->mNextTriangleEdge = pn;
      mTriangleEdges.erase(e->mHash);
      mTriangleEdges[e->mHash] = e;
    }

    e++;

    return e;
  }

  void addEdgeCheck(Triangle *t,Edge *e)
  {
    EdgeCheck ec(t,e);
    mEdgeCheckQueue.pushBack(ec);
  }

  PxU32 mergeCoplanarIslands(const PxF32 *vertices)
  {
    mVerticesFloat = vertices;
    mVerticesDouble = 0;
    return mergeCoplanarIslands();
  }

  PxU32 mergeCoplanarIslands(const PxF64 *vertices)
  {
    mVerticesDouble = vertices;
    mVerticesFloat = 0;
    return mergeCoplanarIslands();
  }

  // this island needs to be merged
  void mergeTouching(Island * /*isl*/)
  {
/*    Island *touching = 0;

    IslandVector::Iterator i;
    for (i=mIslands.begin(); i!=mIslands.end(); ++i)
    {
      Island *_i = (*i);
      if ( !_i->mCoplanar ) // can't merge with coplanar islands!
      {
        if ( _i->isTouching(isl,mVerticesFloat,mVerticesDouble) )
        {
          touching = _i;
        }
      }
    }*/
  }

  PxU32 mergeCoplanarIslands(void)
  {
    PxU32  ret = 0;

    if ( !mIslands.empty() )
    {


      PxU32  cp_count  = 0;
      PxU32  npc_count = 0;

      PxU32  count = (PxU32)mIslands.size();

      for (PxU32 i=0; i<count; i++)
      {

        PxU32 otcount;
        const PxU32 *oindices = getIsland(i,otcount);

        if ( otcount )
        {

          bool isCoplanar;

          if ( mVerticesFloat )
            isCoplanar = physx::fm_isMeshCoplanar(otcount, oindices, mVerticesFloat, true);
          else
            isCoplanar = physx::fm_isMeshCoplanar(otcount, oindices, mVerticesDouble, true);

          if ( isCoplanar )
          {
            Island *isl = mIslands[i];
            isl->mCoplanar = true;
            cp_count++;
          }
          else
          {
            npc_count++;
          }
        }
        else
        {
          PX_ASSERT(0);
        }
      }

      if ( cp_count )
      {
        if ( npc_count == 0 ) // all islands are co-planar!
        {
          IslandVector temp = mIslands;
          mIslands.clear();
          Island *isl = mIslands[0];
          mIslands.pushBack(isl);
          for (PxU32 i=1; i<cp_count; i++)
          {
            Island *_i = mIslands[i];
            isl->merge(*_i);
            delete _i;
          }
        }
        else
        {


          Triangle *t = mTriangles;
          for (PxU32 i=0; i<mTcount; i++)
          {
            t->buildBox(mVerticesFloat,mVerticesDouble,i);
            t++;
          }

          IslandVector::Iterator i;
          for (i=mIslands.begin(); i!=mIslands.end(); ++i)
          {
            Island *isl = (*i);

            PxU32 color = 0x00FF00;

            if ( isl->mCoplanar )
            {
              color = 0xFFFF00;
            }

			PX_UNUSED(color); // Make compiler happy

            mergeTouching(isl);

          }

          IslandVector temp = mIslands;
          mIslands.clear();
          for (i=temp.begin(); i!=temp.end(); i++)
          {
            Island *isl = (*i);
            if ( isl->mCoplanar )
            {
              delete isl; // kill it
            }
            else
            {
              mIslands.pushBack(isl);
            }
          }
          ret = (PxU32)mIslands.size();
        }
      }
      else
      {
        ret = npc_count;
      }
    }


    return ret;
  }

  PxU32 mergeTouchingIslands(const PxF32 * /*vertices*/)
  {
    PxU32 ret = 0;

    return ret;
  }

  PxU32 mergeTouchingIslands(const PxF64 * /*vertices*/)
  {
    PxU32 ret = 0;

    return ret;
  }

  PxU32           mTcount;
  Triangle        *mTriangles;
  Edge            *mEdges;
  EdgeHashMap      mTriangleEdges;
  IslandVector     mIslands;
  EdgeCheckQueue   mEdgeCheckQueue;
  const PxF64    *mVerticesDouble;
  const PxF32     *mVerticesFloat;
  PxU32Vector     mIndices;
};


MeshIslandGeneration * createMeshIslandGeneration(void)
{
  MyMeshIslandGeneration *mig = PX_NEW(MyMeshIslandGeneration);
  return static_cast< MeshIslandGeneration *>(mig);
}

void                   releaseMeshIslandGeneration(MeshIslandGeneration *cm)
{
  MyMeshIslandGeneration *mig = static_cast< MyMeshIslandGeneration *>(cm);
  delete mig;
}

}; // end of namespace
};

