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

#include "PsAlloca.h"
#include "PsUserAllocated.h"
#include "PsMathUtils.h"
#include "PsUtilities.h"

#include "foundation/PxMath.h"
#include "foundation/PxBounds3.h"
#include "foundation/PxPlane.h"
#include "foundation/PxMemory.h"

#include "InflationConvexHullLib.h"
#include "ConvexHullUtils.h"

using namespace physx;

namespace local
{
	//////////////////////////////////////////////////////////////////////////
	// constants	
	static const float DIMENSION_EPSILON_MULTIPLY = 0.001f; // used to scale down bounds dimension and set as epsilon used in the hull generator
	static const float DIR_ANGLE_MULTIPLY = 0.025f; // used in maxIndexInDirSterid for direction check modifier
	static const float VOLUME_EPSILON = (1e-20f); // volume epsilon used for simplex valid
	static const float MIN_ADJACENT_ANGLE = 3.0f;  // in degrees  - result wont have two adjacent facets within this angle of each other. !AB expose this parameter or use the one PxCookingParams
	static const float PAPERWIDTH = 0.001f; // used in hull construction from planes, within paperwidth its considered coplanar

	//////////////////////////////////////////////////////////////////////////
	// gets the most distant index along the given dir filtering allowed indices
	PxI32 maxIndexInDirFiltered(const PxVec3 *p,PxU32 count,const PxVec3 &dir, bool* tempNotAllowed)
	{
		PX_ASSERT(count);
		PxI32 m=-1;
		for(PxU32 i=0;i < count; i++)
		{
			if(!tempNotAllowed[i])
			{
				if(m==-1 || p[i].dot(dir) > p[m].dot(dir)) 
					m= PxI32(i);
			}
		}
		PX_ASSERT(m!=-1);
		return m;
	} 

	//////////////////////////////////////////////////////////////////////////
	// gets orthogonal more significant vector
	static PxVec3 orth(const PxVec3& v)
	{
		PxVec3 a= v.cross(PxVec3(0,0,1.f));
		PxVec3 b= v.cross(PxVec3(0,1.f,0));
		PxVec3 out = (a.magnitudeSquared() > b.magnitudeSquared())? a : b;
		out.normalize();
		return out;
	}

	//////////////////////////////////////////////////////////////////////////
	// find the most distant index in given direction dir
	PxI32 maxIndexInDirSterid(const PxVec3* p,PxU32 count,const PxVec3& dir,Ps::Array<PxU8> &allow)
	{
		// if the found vertex does not get hit by a slightly rotated ray, it
		// may not be the extreme we are looking for. Therefore it is marked
		// as disabled for the direction search and different candidate is chosen.
		PX_ALLOCA(tempNotAllowed,bool,count);
		PxMemSet(tempNotAllowed,0,count*sizeof(bool));

		PxI32 m=-1;
		while(m==-1)
		{
			// get the furthest index along dir
			m = maxIndexInDirFiltered(p,count,dir,tempNotAllowed);
			PX_ASSERT(m >= 0);

			if(allow[PxU32(m)] == 3) 
				return m;

			// get orthogonal vectors to the dir
			PxVec3 u = orth(dir);
			PxVec3 v = u.cross(dir);

			PxI32 ma=-1;
			// we shoot a ray close to the original dir and hope to get the same index
			// if we not hit the same index we try it with bigger precision
			// if we still fail to hit the same index we drop the index and iterate again
			for(float x = 0.0f ; x <= 360.0f ; x+= 45.0f)
			{
				float s0 = PxSin(Ps::degToRad(x));
				float c0 = PxCos(Ps::degToRad(x));
				PxI32 mb = maxIndexInDirFiltered(p,count,dir+(u*s0+v*c0)*DIR_ANGLE_MULTIPLY,tempNotAllowed);
				if(ma==m && mb==m)
				{
					allow[PxU32(m)]=3;
					return m;
				}
				if(ma!=-1 && ma!=mb)
				{
					PxI32 mc = ma;
					for(float xx = x-40.0f ; xx <= x ; xx+= 5.0f)
					{
						float s = PxSin(Ps::degToRad(xx));
						float c = PxCos(Ps::degToRad(xx));
						int md = maxIndexInDirFiltered(p,count,dir+(u*s+v*c)*DIR_ANGLE_MULTIPLY,tempNotAllowed);
						if(mc==m && md==m)
						{
							allow[PxU32(m)]=3;
							return m;
						}
						mc=md;
					}
				}
				ma=mb;
			}
			tempNotAllowed[m]=true;
			m=-1;
		}
		PX_ASSERT(0);
		return m;
	}

	//////////////////////////////////////////////////////////////////////////
	// Simplex helper class - just holds the 4 indices 
	class HullSimplex
	{
	public:
		PxI32 x,y,z,w;
		HullSimplex(){}
		HullSimplex(PxI32 _x,PxI32 _y, PxI32 _z,PxI32 _w){x=_x;y=_y;z=_z;w=_w;}
		const PxI32& operator[](PxI32 i) const
		{
			return reinterpret_cast<const PxI32*>(this)[i];
		}
		PxI32& operator[](PxI32 i)
		{
			return reinterpret_cast<PxI32*>(this)[i];
		}

		//////////////////////////////////////////////////////////////////////////
		// checks the volume of given simplex
		static bool hasVolume(const PxVec3* verts, PxU32 p0, PxU32 p1, PxU32 p2, PxU32 p3)
		{
			PxVec3 result3 = (verts[p1]-verts[p0]).cross(verts[p2]-verts[p0]);
			if ((result3).magnitude() < VOLUME_EPSILON && (result3).magnitude() > -VOLUME_EPSILON) // Almost collinear or otherwise very close to each other
				return false;
			result3.normalize();
			const float result = result3.dot(verts[p3]-verts[p0]);
			return (result > VOLUME_EPSILON || result < -VOLUME_EPSILON); // Returns true if volume is significantly non-zero
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// finds the hull simplex http://en.wikipedia.org/wiki/Simplex
	// in - vertices, vertex count, dimensions
	// out - indices forming the simplex
	static HullSimplex findSimplex(const PxVec3* verts, PxU32 verts_count, Ps::Array<PxU8>& allow,const PxVec3& minMax)
	{
		// pick the basis vectors
		PxVec3 basisVector[3];
		PxVec3 basis[3];
		basisVector[0] = PxVec3( 1.0f, 0.02f, 0.01f);
		basisVector[1] = PxVec3(-0.02f, 1.0f, -0.01f);
		basisVector[2] = PxVec3( 0.01f, 0.02f, 1.0f );

		PxU32 index0 = 0;
		PxU32 index1 = 1;
		PxU32 index2 = 2;

		// make the order of the basis vector depending on the points bounds, first basis test will be done 
		// along the longest axis
		if(minMax.z > minMax.x && minMax.z > minMax.y)
		{
			index0 = 2;
			index1 = 0;
			index2 = 1;
		}
		else
		{
			if(minMax.y > minMax.x && minMax.y > minMax.z)
			{
				index0 = 1;
				index1 = 2;
				index2 = 0;
			}
		}

		// pick the fist basis vector
		basis[0] = basisVector[index0];
		// find the indices along the pos/neg direction 
		PxI32 p0 = maxIndexInDirSterid(verts,verts_count, basis[0],allow);
		PxI32 p1 = maxIndexInDirSterid(verts,verts_count,-basis[0],allow);

		// set the first simplex axis
		basis[0] = verts[p0]-verts[p1];
		// if the points are the same or the basis vector is zero, terminate we failed to find a simplex
		if(p0==p1 || basis[0]==PxVec3(0.0f)) 
			return HullSimplex(-1,-1,-1,-1);

		// get the orthogonal vectors against the new basis[0] vector
		basis[1] = basisVector[index1].cross(basis[0]);  //cross(float3(     1, 0.02f, 0),basis[0]);
		basis[2] = basisVector[index2].cross(basis[0]); //cross(float3(-0.02f,     1, 0),basis[0]);
		// pick the longer basis vector
		basis[1] = ((basis[1]).magnitudeSquared() > (basis[2]).magnitudeSquared()) ? basis[1] : basis[2];
		basis[1].normalize();

		// get the index along the picked second axis
		PxI32 p2 = maxIndexInDirSterid(verts,verts_count,basis[1],allow);
		// if we got the same point, try the negative direction
		if(p2 == p0 || p2 == p1)
		{
			p2 = maxIndexInDirSterid(verts,verts_count,-basis[1],allow);
		}
		// we failed to create the simplex the points are the same as the base line
		if(p2 == p0 || p2 == p1) 
			return HullSimplex(-1,-1,-1,-1);

		// set the second simplex edge
		basis[1] = verts[p2] - verts[p0];
		// get the last orthogonal direction
		basis[2] = basis[1].cross(basis[0]);
		basis[2].normalize();

		// get the index along the last direction
		PxI32 p3 = maxIndexInDirSterid(verts,verts_count,basis[2],allow);
		if(p3==p0||p3==p1||p3==p2||!HullSimplex::hasVolume(verts, PxU32(p0), PxU32(p1), PxU32(p2), PxU32(p3))) 
		{
			p3 = maxIndexInDirSterid(verts,verts_count,-basis[2],allow);
		}
		// if this index was already chosen terminate we dont have the simplex
		if(p3==p0||p3==p1||p3==p2) 
			return HullSimplex(-1,-1,-1,-1);

		PX_ASSERT(!(p0==p1||p0==p2||p0==p3||p1==p2||p1==p3||p2==p3));

		// check the axis order
		if((verts[p3]-verts[p0]).dot((verts[p1]-verts[p0]).cross(verts[p2]-verts[p0])) < 0)
		{
			Ps::swap(p2,p3);
		}
		return HullSimplex(p0,p1,p2,p3);
	}

	//////////////////////////////////////////////////////////////////////////
	// helper struct for hull expand
	struct ExpandPlane
	{
		PxPlane		mPlane;
		int			mAdjacency[3];   // 1 - 0, 2 - 0, 2 - 1
		int			mExpandPoint;
		float		mExpandDistance;		
		int			mIndices[3];
		int			mTrisIndex;

		ExpandPlane()
		{
			for (int i = 0; i < 3; i++)
			{
				mAdjacency[i]  = -1;
				mIndices[i] = -1;
			}

			mExpandDistance = -FLT_MAX;
			mExpandPoint = -1;			
			mTrisIndex = -1;
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// helper class for triangle representation
	class int3  
	{
	public:
		PxI32 x,y,z;
		int3(){}
		int3(PxI32 _x,PxI32 _y, PxI32 _z){x=_x;y=_y;z=_z;}
		const PxI32& operator[](PxI32 i) const
		{
			return reinterpret_cast<const PxI32*>(this)[i];
		}
		PxI32& operator[](PxI32 i)
		{
			return reinterpret_cast<PxI32*>(this)[i];
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// helper class for triangle representation
	class Tri : public int3, public Ps::UserAllocated
	{
	public:	
		int3 n;
		PxI32 id;
		PxI32 vmax;
		float rise;

		// get the neighbor index for edge
		PxI32& neib(PxI32 a, PxI32 b)
		{
			static PxI32 er=-1;
			for(PxI32 i=0;i<3;i++) 
			{
				PxI32 i1= (i+1)%3;
				PxI32 i2= (i+2)%3;
				if((*this)[i]==a && (*this)[i1]==b) return n[i2];
				if((*this)[i]==b && (*this)[i1]==a) return n[i2];
			}
			PX_ASSERT(0);
			return er;
		}

		// get triangle normal
		PxVec3 getNormal(const PxVec3* verts) const
		{
			// return the normal of the triangle
			// inscribed by v0, v1, and v2
			const PxVec3& v0 = verts[(*this)[0]];
			const PxVec3& v1 = verts[(*this)[1]];
			const PxVec3& v2 = verts[(*this)[2]];
			PxVec3 cp= (v1-v0).cross(v2-v1);
			float m= (cp).magnitude();
			if(m==0) 
				return PxVec3(1.f,0.0f,0.0f);
			return cp*(1.0f/m);
		}

		float getArea2(const PxVec3* verts) const
		{
			const PxVec3& v0 = verts[(*this)[0]];
			const PxVec3& v1 = verts[(*this)[1]];
			const PxVec3& v2 = verts[(*this)[2]];
			return ((v0-v1).cross(v2-v0)).magnitudeSquared();			
		}

		friend class HullTriangles;
	protected:

		Tri(PxI32 a, PxI32 b, PxI32 c) : int3(a, b, c), n(-1,-1,-1)
		{
			vmax=-1;
			rise = 0.0f;
		}

		~Tri()
		{
		}		
	};

	//////////////////////////////////////////////////////////////////////////
	// checks if for given triangle the point is above the triangle in the normal direction
	// value is checked against an epsilon
	static PxI32 above(const PxVec3* vertices, const Tri& t, const PxVec3& p, float epsilon)
	{
		PxVec3 n = t.getNormal(vertices);
		return (n.dot(p-vertices[t[0]]) > epsilon); 
	}

	//////////////////////////////////////////////////////////////////////////
	// checks if given triangle does contain the vertex v
	static int hasVert(const int3& t, int v)
	{
		return (t[0]==v || t[1]==v || t[2]==v) ;
	}

	//////////////////////////////////////////////////////////////////////////
	// helper class for hull triangles management
	class HullTriangles
	{
	public:
		HullTriangles()
		{
			mTriangles.reserve(256);
		}

		~HullTriangles()
		{
			for (PxU32 i = 0; i < mTriangles.size(); i++)
			{
				if(mTriangles[i])
					delete mTriangles[i];
			}
			mTriangles.clear();
		}

		const Tri* operator[](PxU32 i) const
		{
			return mTriangles[i];
		}

		Tri* operator[](PxU32 i)
		{
			return mTriangles[i];
		}


		//////////////////////////////////////////////////////////////////////////

		const Ps::Array<Tri*>& getTriangles() const 
		{
			return mTriangles;
		}
		Ps::Array<Tri*>& getTriangles()
		{
			return mTriangles;
		}

		//////////////////////////////////////////////////////////////////////////

		PxU32 size() const
		{
			return mTriangles.size();
		}

		//////////////////////////////////////////////////////////////////////////
		// delete triangle from the array
		Tri* createTri(PxI32 a, PxI32 b, PxI32 c)
		{
			Tri* tri = PX_NEW_TEMP(Tri)(a, b, c);
			tri->id = PxI32(mTriangles.size());
			mTriangles.pushBack(tri);
			return tri;
		}

		//////////////////////////////////////////////////////////////////////////
		// delete triangle from the array
		void deleteTri(Tri* tri)
		{
			PX_ASSERT((mTriangles)[PxU32(tri->id)]==tri);
			(mTriangles)[PxU32(tri->id)] = NULL;
			delete tri;
		}

		//////////////////////////////////////////////////////////////////////////
		// check triangle
		void checkit(Tri* t) const
		{
			PX_ASSERT((mTriangles)[PxU32(t->id)]==t);
			for(int i=0;i<3;i++)
			{
				const int i1=(i+1)%3;
				const int i2=(i+2)%3;
				const int a = (*t)[i1];
				const int b = (*t)[i2];
				PX_ASSERT(a!=b);
				PX_ASSERT( (mTriangles)[PxU32(t->n[i])]->neib(b,a) == t->id);
				PX_UNUSED(a);
				PX_UNUSED(b);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// find the triangle, which has the greatest rise (distance in the direction of normal)
		// return such a triangle if it does exist and if the rise is bigger than given epsilon
		Tri* findExtrudable(float epsilon) const
		{
			Tri* t = NULL;
			for(PxU32 i=0; i < mTriangles.size(); i++)
			{
				if(!t || ((mTriangles)[i] && (t->rise < (mTriangles)[i]->rise)))
				{
					t = (mTriangles)[i];
				}
			}
			if(!t)
				return NULL;
			return (t->rise > epsilon) ? t : NULL;
		}

		//////////////////////////////////////////////////////////////////////////
		// extrude the given triangle t0 with triangle v
		void extrude(Tri* t0, PxI32 v)
		{
			int3 t= *t0;
			PxI32 n = PxI32(mTriangles.size());
			// create the 3 extruded triangles
			Tri* ta = createTri(v, t[1], t[2]);
			ta->n = int3(t0->n[0],n+1,n+2);
			(mTriangles)[PxU32(t0->n[0])]->neib(t[1],t[2]) = n+0;
			Tri* tb = createTri(v, t[2], t[0]);
			tb->n = int3(t0->n[1],n+2,n+0);
			(mTriangles)[PxU32(t0->n[1])]->neib(t[2],t[0]) = n+1;
			Tri* tc = createTri(v, t[0], t[1]);
			tc->n = int3(t0->n[2],n+0,n+1);
			(mTriangles)[PxU32(t0->n[2])]->neib(t[0],t[1]) = n+2;
			checkit(ta);
			checkit(tb);
			checkit(tc);

			// check if the added triangle is not already inserted
			// in that case we remove both and fix the neighbors 
			// for the remaining triangles
			if(hasVert(*(mTriangles)[PxU32(ta->n[0])],v)) 
				removeb2b(ta,(mTriangles)[PxU32(ta->n[0])]);
			if(hasVert(*(mTriangles)[PxU32(tb->n[0])],v)) 
				removeb2b(tb,(mTriangles)[PxU32(tb->n[0])]);
			if(hasVert(*(mTriangles)[PxU32(tc->n[0])],v)) 
				removeb2b(tc,(mTriangles)[PxU32(tc->n[0])]);
			deleteTri(t0);
		}

	protected:
		//////////////////////////////////////////////////////////////////////////
		// remove the 2 triangles which are the same and fix the neighbor triangles
		void b2bfix(Tri* s, Tri* t)
		{
			for(int i=0;i<3;i++) 
			{
				const int i1=(i+1)%3;
				const int i2=(i+2)%3;
				const int a = (*s)[i1];
				const int b = (*s)[i2];
				PX_ASSERT((mTriangles)[PxU32(s->neib(a,b))]->neib(b,a) == s->id);
				PX_ASSERT((mTriangles)[PxU32(t->neib(a,b))]->neib(b,a) == t->id);
				(mTriangles)[PxU32(s->neib(a,b))]->neib(b,a) = t->neib(b,a);
				(mTriangles)[PxU32(t->neib(b,a))]->neib(a,b) = s->neib(a,b);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// remove the 2 triangles which are the same and fix the neighbor triangles
		void removeb2b(Tri* s, Tri* t)
		{
			b2bfix(s,t);
			deleteTri(s);
			deleteTri(t);
		}


	private:
		Ps::Array<Tri*>			mTriangles;
	};

	}

	//////////////////////////////////////////////////////////////////////////

InflationConvexHullLib::InflationConvexHullLib(const PxConvexMeshDesc& desc, const PxCookingParams& params)
	: ConvexHullLib(desc,params), mFinished(false)
{
}

//////////////////////////////////////////////////////////////////////////
// Main function to create the hull.
// Construct the hull with set input parameters - ConvexMeshDesc and CookingParams
PxConvexMeshCookingResult::Enum InflationConvexHullLib::createConvexHull()
{
	PxConvexMeshCookingResult::Enum res = PxConvexMeshCookingResult::eFAILURE;

	PxU32 vcount = mConvexMeshDesc.points.count;
	if (vcount < 8) 
		vcount = 8;

	// allocate additional vec3 for V4 safe load in VolumeIntegration
	PxVec3* vsource  = reinterpret_cast<PxVec3*> (PX_ALLOC_TEMP( sizeof(PxVec3)*vcount + 1, "PxVec3"));
	PxVec3 scale;
	PxVec3 center;
	PxU32 ovcount;

	// cleanup the vertices first
	if(mConvexMeshDesc.flags & PxConvexFlag::eSHIFT_VERTICES)
	{
		if(!shiftAndcleanupVertices(mConvexMeshDesc.points.count, reinterpret_cast<const PxVec3*> (mConvexMeshDesc.points.data), mConvexMeshDesc.points.stride,
			ovcount, vsource, scale, center ))
		{
			PX_FREE(vsource);
			return res;
		}	
	}
	else
	{
		if(!cleanupVertices(mConvexMeshDesc.points.count, reinterpret_cast<const PxVec3*> (mConvexMeshDesc.points.data), mConvexMeshDesc.points.stride,
			ovcount, vsource, scale, center ))
		{
			PX_FREE(vsource);
			return res;
		}	
	}
	// scale vertices back to their original size.
	for (PxU32 i=0; i<ovcount; i++)
	{
		PxVec3& v = vsource[i];
		v.multiply(scale);
	}	

	// compute the actual hull
	ConvexHullLibResult::ErrorCode hullResult = computeHull(ovcount,vsource);
	if(hullResult == ConvexHullLibResult::eSUCCESS || hullResult == ConvexHullLibResult::ePOLYGON_LIMIT_REACHED)
	{
		mFinished = true;
		res = (hullResult == ConvexHullLibResult::eSUCCESS) ? PxConvexMeshCookingResult::eSUCCESS : PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED;
	}
	else
	{
		if(hullResult == ConvexHullLibResult::eZERO_AREA_TEST_FAILED)
			res = PxConvexMeshCookingResult::eZERO_AREA_TEST_FAILED;
	}

	if(vsource)
	{
		PX_FREE(vsource);
	}	

	return res;
}

//////////////////////////////////////////////////////////////////////////
// computes the hull and stores results into mHullResult
ConvexHullLibResult::ErrorCode InflationConvexHullLib::computeHull(PxU32 vertsCount, const PxVec3* verts)
{
	PX_ASSERT(verts);
	PX_ASSERT(vertsCount > 0);

	ConvexHull* hullOut = NULL;
	ConvexHullLibResult::ErrorCode res = calchull(verts, vertsCount, hullOut);
	if ((res == ConvexHullLibResult::eFAILURE) || (res == ConvexHullLibResult::eZERO_AREA_TEST_FAILED))
		return res;

	PX_ASSERT(hullOut);

	// parse the hullOut and fill the result with vertices and polygons
	mHullResult.mIndices = reinterpret_cast<PxU32*> (PX_ALLOC_TEMP(sizeof(PxU32)*(hullOut->getEdges().size()), "PxU32"));
	mHullResult.mIndexCount=hullOut->getEdges().size();

	mHullResult.mPolygonCount = hullOut->getFacets().size();
	mHullResult.mPolygons = reinterpret_cast<PxHullPolygon*> (PX_ALLOC_TEMP(sizeof(PxHullPolygon)*mHullResult.mPolygonCount, "PxHullPolygon"));

	// allocate additional vec3 for V4 safe load in VolumeInteration
	mHullResult.mVertices   = reinterpret_cast<PxVec3*> (PX_ALLOC_TEMP(sizeof(PxVec3)*hullOut->getVertices().size() + 1, "PxVec3"));
	mHullResult.mVcount     = hullOut->getVertices().size();
	PxMemCopy(mHullResult.mVertices,hullOut->getVertices().begin(),sizeof(PxVec3)*mHullResult.mVcount);

	PxU32 i=0;	
	PxU32 k=0;
	PxU32 j = 1;
	while(i<hullOut->getEdges().size())
	{
		j=1;
		PxHullPolygon& polygon = mHullResult.mPolygons[k];
		// get num indices per polygon
		while(j+i < hullOut->getEdges().size() && hullOut->getEdges()[i].p == hullOut->getEdges()[i+j].p)
		{ 
			j++; 
		}		
		polygon.mNbVerts = Ps::to16(j);
		polygon.mIndexBase = Ps::to16(i);

		// get the plane
		polygon.mPlane[0] = hullOut->getFacets()[k].n[0];
		polygon.mPlane[1] = hullOut->getFacets()[k].n[1];
		polygon.mPlane[2] = hullOut->getFacets()[k].n[2];

		polygon.mPlane[3] = hullOut->getFacets()[k].d;

		while(j--)
		{
			mHullResult.mIndices[i] = hullOut->getEdges()[i].v;			
			i++;
		}
		k++;
	}

	PX_ASSERT(k==hullOut->getFacets().size());
	PX_DELETE(hullOut);

	return res;
}

//////////////////////////////////////////////////////////////////////////
// internal function taking the cleaned vertices and constructing the 
// new hull from them.
// 1. using the incremental algorithm create base hull from the input vertices
// 2. if we reached the vertex limit, we expand the hull 
// 3. otherwise we compute the new planes and inflate them
// 4. we crop the AABB with the computed planes to construct the new hull
ConvexHullLibResult::ErrorCode InflationConvexHullLib::calchull(const PxVec3* verts, PxU32 verts_count, ConvexHull*& hullOut)
{
	// calculate the actual hull using the incremental algorithm
	local::HullTriangles  triangles;
	ConvexHullLibResult::ErrorCode rc = calchullgen(verts,verts_count, triangles);
	if ((rc == ConvexHullLibResult::eFAILURE) || (rc == ConvexHullLibResult::eZERO_AREA_TEST_FAILED))
		return rc;

	// count the triangles if we did not reached the polygons count hard limit 255
	PxU32 numTriangles = 0;
	for (PxU32 i = 0; i < triangles.size(); i++)
	{
		if((triangles)[i])
			numTriangles++;
	}

	// if the polygons hard limit has been reached run the overhull, that will terminate 
	// when either polygons or vertex limit has been reached
	if(numTriangles > 255)
	{
		Ps::Array<PxPlane> planes;
		if(!calchullplanes(verts,triangles,planes))
			return ConvexHullLibResult::eFAILURE;

		if(!overhull(verts, verts_count, planes,hullOut))
			return ConvexHullLibResult::eFAILURE;

		return ConvexHullLibResult::ePOLYGON_LIMIT_REACHED;
	}

	// if vertex limit reached construct the hullOut from the expanded planes
	if(rc == ConvexHullLibResult::eVERTEX_LIMIT_REACHED)
	{
		if(mConvexMeshDesc.flags & PxConvexFlag::ePLANE_SHIFTING)
			rc = expandHull(verts,verts_count,triangles,hullOut);
		else
			rc = expandHullOBB(verts,verts_count,triangles,hullOut);
		if ((rc == ConvexHullLibResult::eFAILURE) || (rc == ConvexHullLibResult::eZERO_AREA_TEST_FAILED))
			return rc;

		return ConvexHullLibResult::eSUCCESS;
	}

	Ps::Array<PxPlane> planes;
	if(!calchullplanes(verts,triangles,planes))
		return ConvexHullLibResult::eFAILURE;

	if(!overhull(verts, verts_count, planes,hullOut))
		return ConvexHullLibResult::eFAILURE;

	return ConvexHullLibResult::eSUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// computes the actual hull using the incremental algorithm
// in - vertices, numVertices
// out - triangles
// 1. construct the initial simplex
// 2. each step take the most furthers vertex from the hull and add it
// 3. terminate if we reached the hull limit or all verts are used
ConvexHullLibResult::ErrorCode InflationConvexHullLib::calchullgen(const PxVec3* verts, PxU32 verts_count, local::HullTriangles&  triangles)
{
	// at least 4 verts so we can construct a simplex
	// limit is 256 for OBB slicing or fixed limit for plane shifting
	PxU32 vlimit = (mConvexMeshDesc.flags & PxConvexFlag::ePLANE_SHIFTING) ? mConvexMeshDesc.vertexLimit : 256u;
	PxU32 numHullVerts = 4;
	if(verts_count < 4) 
		return ConvexHullLibResult::eFAILURE;

	PxU32 j;
	PxBounds3 bounds;
	bounds.setEmpty();

	Ps::Array<PxU8> isextreme;
	isextreme.reserve(verts_count);

	Ps::Array<PxU8> allow;
	allow.reserve(verts_count);	

	for(j=0; j < verts_count; j++) 
	{
		allow.pushBack(1);
		isextreme.pushBack(0);
		bounds.include(verts[j]);
	}

	const PxVec3 dimensions = bounds.getDimensions();
	const float epsilon = dimensions.magnitude() * local::DIMENSION_EPSILON_MULTIPLY;
	mTolerance = 0.001f;
	mPlaneTolerance = epsilon;

	const bool useAreaTest = mConvexMeshDesc.flags & PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES ? true : false;
	const float areaEpsilon = useAreaTest ?  mCookingParams.areaTestEpsilon * 2.0f : epsilon*epsilon*0.1f;

	// find the simplex
	local::HullSimplex p = local::findSimplex(verts,verts_count,allow, dimensions);
	if(p.x==-1)  // simplex failed
		return ConvexHullLibResult::eFAILURE; 

	// a valid interior point
	PxVec3 center = (verts[p[0]]+verts[p[1]]+verts[p[2]]+verts[p[3]]) /4.0f;  

	// add the simplex triangles into the triangle array	
	local::Tri *t0 = triangles.createTri(p[2], p[3], p[1]); t0->n=local::int3(2,3,1);
	local::Tri *t1 = triangles.createTri(p[3], p[2], p[0]); t1->n=local::int3(3,2,0);
	local::Tri *t2 = triangles.createTri(p[0], p[1], p[3]); t2->n=local::int3(0,1,3);
	local::Tri *t3 = triangles.createTri(p[1], p[0], p[2]); t3->n=local::int3(1,0,2);
	// mark the simplex indices as extremes
	isextreme[PxU32(p[0])]=isextreme[PxU32(p[1])]=isextreme[PxU32(p[2])]=isextreme[PxU32(p[3])]=1;

	// check if the added simplex triangles are valid
	triangles.checkit(t0);
	triangles.checkit(t1);
	triangles.checkit(t2);
	triangles.checkit(t3);

	// parse the initial triangles and set max vertex along the normal and its distance
	for(j=0;j< triangles.size(); j++)
	{
		local::Tri *t=(triangles.getTriangles())[j];
		PX_ASSERT(t);
		PX_ASSERT(t->vmax<0);
		PxVec3 n= (*t).getNormal(verts);
		t->vmax = local::maxIndexInDirSterid(verts,verts_count,n,allow);
		t->rise = n.dot(verts[t->vmax]-verts[(*t)[0]]);

		// use the areaTest to drop small triangles, which can cause trouble to the simulation, 
		// if we drop triangles from the initial simplex, we let the user know that the provided points form 
		// a simplex which is too small for given area threshold
		if(useAreaTest && ((verts[(*t)[1]]-verts[(*t)[0]]).cross(verts[(*t)[2]]-verts[(*t)[1]])).magnitude() < areaEpsilon)
		{
			triangles.deleteTri(t0);
			triangles.deleteTri(t1);
			triangles.deleteTri(t2);
			triangles.deleteTri(t3);
			return ConvexHullLibResult::eZERO_AREA_TEST_FAILED;
		}
	}

	local::Tri *te;
	// lower the vertex limit, we did already set 4 verts
	vlimit-=4;
	// iterate over triangles till we reach the limit or we dont have triangles with 
	// significant rise or we cannot add any triangles at all
	while(vlimit >0 && ((te = triangles.findExtrudable(epsilon)) != NULL))
	{		
		PxI32 v = te->vmax;
		PX_ASSERT(!isextreme[PxU32(v)]);  // wtf we've already done this vertex
		// set as extreme point
		isextreme[PxU32(v)]=1;

		j=triangles.size();
		// go through the triangles and extrude the extreme point if it is above it
		while(j--) 
		{
			if(!(triangles)[j]) 
				continue;
			const local::Tri& t= *(triangles)[j];			
			if(above(verts,t,verts[v],0.01f*epsilon))
			{
				triangles.extrude((triangles)[j],v);
			}
		}

		// now check for those degenerate cases where we have a flipped triangle or a really skinny triangle
		j=triangles.size();
		while(j--)
		{
			if(!(triangles)[j]) 
				continue;
			if(!hasVert(*(triangles)[j],v)) 
				break;
			local::int3 nt=*(triangles)[j];
			if(above(verts,*(triangles)[j],center,0.01f*epsilon)  || ((verts[nt[1]]-verts[nt[0]]).cross(verts[nt[2]]-verts[nt[1]])).magnitude() < areaEpsilon)
			{
				local::Tri *nb = (triangles)[PxU32((triangles)[j]->n[0])];
				PX_ASSERT(nb);
				PX_ASSERT(!hasVert(*nb,v));
				PX_ASSERT(PxU32(nb->id)<j);
				triangles.extrude(nb,v);
				j=triangles.size();
			}
		}

		// get new rise and vmax for the new triangles 
		j=triangles.size();
		while(j--)
		{
			local::Tri *t=(triangles)[j];
			if(!t) 
				continue;
			if(t->vmax >= 0) 
				break;
			PxVec3 n= t->getNormal(verts);
			t->vmax = local::maxIndexInDirSterid(verts,verts_count,n,allow);
			if(isextreme[PxU32(t->vmax)]) 
			{
				t->vmax=-1; // already done that vertex - algorithm needs to be able to terminate.
			}
			else
			{
				t->rise = n.dot(verts[t->vmax]-verts[(*t)[0]]);
			}
		}
		// we added a vertex we lower the limit
		vlimit --;
		numHullVerts++;
	}

	if((mConvexMeshDesc.flags & PxConvexFlag::ePLANE_SHIFTING) && vlimit == 0)
		return ConvexHullLibResult::eVERTEX_LIMIT_REACHED;
	if (!(mConvexMeshDesc.flags & PxConvexFlag::ePLANE_SHIFTING) && numHullVerts > mConvexMeshDesc.vertexLimit)
		return ConvexHullLibResult::eVERTEX_LIMIT_REACHED;
	
	return ConvexHullLibResult::eSUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// expand the hull with the from the limited triangles set
// expand hull will do following steps:
//	1. get planes from triangles that form the best hull with given vertices
//  2. compute the adjacency information for the planes
//  3. expand the planes to have all vertices inside the planes volume
//  4. compute new points by 3 adjacency planes intersections
//  5. take those points and create the hull from them
ConvexHullLibResult::ErrorCode InflationConvexHullLib::expandHull(const PxVec3* verts, PxU32 vertsCount, const local::HullTriangles& triangles, ConvexHull*& hullOut)
{
#if PX_DEBUG
	struct LocalTests
	{
		static bool PlaneCheck(const PxVec3* verts_, PxU32 verts_count_, Ps::Array<local::ExpandPlane>& planes)
		{
			for(PxU32 i=0;i<planes.size();i++)
			{
				const local::ExpandPlane& expandPlane = planes[i];
				if(expandPlane.mTrisIndex != -1)
				{
					for(PxU32 j=0;j<verts_count_;j++)
					{
						const PxVec3& vertex = verts_[j];

						PX_ASSERT(expandPlane.mPlane.distance(vertex) < 0.02f);

						if(expandPlane.mPlane.distance(vertex) > 0.02f)
						{
							return false;
						}
					}
				}
			}
			return true;
		}
	};
#endif


	Ps::Array<local::ExpandPlane> planes;	

	// need planes and the adjacency for the triangle
	int numPoints = 0;
	for(PxU32 i=0; i < triangles.size();i++)
	{
		local::ExpandPlane expandPlane;
		if((triangles)[i])
		{
			const local::Tri *t=(triangles)[i];			
			PxPlane p;
			p.n = t->getNormal(verts);
			p.d = -p.n.dot(verts[(*t)[0]]);
			expandPlane.mPlane = p;

			for (int l = 0; l < 3; l++)
			{
				if(t->n[l] > numPoints)
				{
					numPoints = t->n[l];
				}
			}

			for(PxU32 j=0;j<triangles.size();j++)
			{
				if((triangles)[j] && i != j)
				{
					const local::Tri *testTris=(triangles)[j];

					int numId0 = 0;
					int numId1 = 0;
					int numId2 = 0;

					for (int k = 0; k < 3; k++)
					{
						int testI = (*testTris)[k];
						if(testI == (*t)[0] || testI == (*t)[1])
						{
							numId0++;
						}
						if(testI == (*t)[0] || testI == (*t)[2])
						{
							numId1++;
						}
						if(testI == (*t)[2] || testI == (*t)[1])
						{
							numId2++;
						}
					}

					if(numId0 == 2)
					{
						PX_ASSERT(expandPlane.mAdjacency[0] == -1);
						expandPlane.mAdjacency[0] = int(j);
					}
					if(numId1 == 2)
					{
						PX_ASSERT(expandPlane.mAdjacency[1] == -1);
						expandPlane.mAdjacency[1] = int(j);
					}
					if(numId2 == 2)
					{
						PX_ASSERT(expandPlane.mAdjacency[2] == -1);
						expandPlane.mAdjacency[2] = int(j);
					}
				}
			}

			expandPlane.mTrisIndex = int(i);			
		}
		planes.pushBack(expandPlane);
	}
	numPoints++;

	// go over the planes now and expand them	
	for(PxU32 i=0;i< vertsCount;i++)
	{
		const PxVec3& vertex = verts[i];

		for(PxU32 j=0;j< triangles.size();j++)
		{
			local::ExpandPlane& expandPlane = planes[j];
			if(expandPlane.mTrisIndex != -1)
			{
				float dist = expandPlane.mPlane.distance(vertex);
				if(dist > 0 && dist > expandPlane.mExpandDistance)
				{
					expandPlane.mExpandDistance = dist;
					expandPlane.mExpandPoint = int(i);
				}
			}
		}	
	}

	// expand the planes
	for(PxU32 i=0;i<planes.size();i++)
	{
		local::ExpandPlane& expandPlane = planes[i];
		if(expandPlane.mTrisIndex != -1)
		{
			if(expandPlane.mExpandPoint >= 0)
				expandPlane.mPlane.d -= expandPlane.mExpandDistance;
		}
	}

	PX_ASSERT(LocalTests::PlaneCheck(verts,vertsCount,planes));

	Ps::Array <int>	translateTable;
	Ps::Array <PxVec3> points;
	numPoints = 0;	

	// find new triangle points and store them
	for(PxU32 i=0;i<planes.size();i++)
	{
		local::ExpandPlane& expandPlane = planes[i];		
		if(expandPlane.mTrisIndex != -1)
		{
			const local::Tri *expandTri=(triangles)[PxU32(expandPlane.mTrisIndex)];

			for (int j = 0; j < 3; j++)
			{
				local::ExpandPlane& plane1 = planes[PxU32(expandPlane.mAdjacency[j])];			
				local::ExpandPlane& plane2 = planes[PxU32(expandPlane.mAdjacency[(j + 1)%3])];	
				const local::Tri *tri1=(triangles)[PxU32(expandPlane.mAdjacency[j])];
				const local::Tri *tri2=(triangles)[PxU32(expandPlane.mAdjacency[(j + 1)%3])];

				int indexE = -1;
				int index1 = -1;
				int index2 = -1;
				for (int l = 0; l < 3; l++)
				{
					for (int k = 0; k < 3; k++)
					{
						for (int m = 0; m < 3; m++)
						{
							if((*expandTri)[l] == (*tri1)[k] && (*expandTri)[l] == (*tri2)[m])
							{
								indexE = l;
								index1 = k;
								index2 = m;
							}
						}
					}				
				}

				PX_ASSERT(indexE != -1);

				int foundIndex = -1;
				for (PxU32 u = 0; u < translateTable.size(); u++)
				{
					if(translateTable[u] == ((*expandTri)[indexE]))
					{
						foundIndex = int(u);
						break;
					}
				}

				PxVec3 point = threePlaneIntersection(expandPlane.mPlane, plane1.mPlane, plane2.mPlane);

				if(foundIndex == -1)
				{					
					expandPlane.mIndices[indexE] = numPoints;
					plane1.mIndices[index1] = numPoints;
					plane2.mIndices[index2] = numPoints;					

					points.pushBack(point);
					translateTable.pushBack((*expandTri)[indexE]);
					numPoints++;
				}
				else
				{
					if(expandPlane.mPlane.distance(points[PxU32(foundIndex)]) < -0.02f || plane1.mPlane.distance(points[PxU32(foundIndex)]) < -0.02f || plane2.mPlane.distance(points[PxU32(foundIndex)]) < -0.02f)
					{
						points[PxU32(foundIndex)] = point;
					}

					expandPlane.mIndices[indexE] = foundIndex;
					plane1.mIndices[index1] = foundIndex;
					plane2.mIndices[index2] = foundIndex;
				}

			}
		}
	}	

	// construct again the hull from the new points
	local::HullTriangles  outTriangles;
	ConvexHullLibResult::ErrorCode rc = calchullgen(points.begin(),PxU32(numPoints), outTriangles);
	if ((rc == ConvexHullLibResult::eFAILURE) || (rc == ConvexHullLibResult::eZERO_AREA_TEST_FAILED))
		return rc;

	// cleanup the unused vertices
	Ps::Array<PxVec3> usedVertices;
	translateTable.clear();
	translateTable.resize(points.size());
	for (PxU32 i = 0; i < points.size(); i++)
	{
		for (PxU32 j = 0; j < outTriangles.size(); j++)
		{
			const local::Tri* tri = outTriangles[j];
			if(tri)
			{
				if((*tri)[0] == int(i) || (*tri)[1] == int(i) || (*tri)[2] == int(i))
				{
					translateTable[i] = int(usedVertices.size());
					usedVertices.pushBack(points[i]);
					break;
				}
			}
		}
	}

	// now construct the hullOut
	Ps::Array<PxPlane> inputPlanes; // < just a blank input planes
	ConvexHull* c = PX_NEW_TEMP(ConvexHull)(inputPlanes);

	// copy the vertices
	for (PxU32 i = 0; i < usedVertices.size(); i++)
	{
		c->getVertices().pushBack(usedVertices[i]);
	}

	// copy planes and create edges
	PxU32 numFaces = 0;
	for (PxU32 i = 0; i < outTriangles.size(); i++)
	{
		const local::Tri* tri = outTriangles[i];
		if(tri)
		{
			PxPlane triPlane;
			triPlane.n = tri->getNormal(points.begin());
			triPlane.d = -triPlane.n.dot(points[PxU32((*tri)[0])]);
			c->getFacets().pushBack(triPlane);

			for (int j = 0; j < 3; j++)
			{
				ConvexHull::HalfEdge edge;
				edge.p = Ps::to8(numFaces);
				edge.v = Ps::to8(translateTable[PxU32((*tri)[j])]);
				c->getEdges().pushBack(edge);
			}
			numFaces++;
		}
	}
	hullOut = c;
	return ConvexHullLibResult::eSUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// expand the hull from the limited triangles set
// 1. collect all planes
// 2. create OBB from the input verts
// 3. slice the OBB with the planes
// 5. iterate till vlimit is reached
ConvexHullLibResult::ErrorCode InflationConvexHullLib::expandHullOBB(const PxVec3* verts, PxU32 vertsCount, const local::HullTriangles&  triangles, ConvexHull*& hullOut)
{
	Ps::Array<PxPlane> expandPlanes;
	expandPlanes.reserve(triangles.size());

	PxU32* indices = PX_NEW_TEMP(PxU32)[triangles.size()*3];
	PxHullPolygon* polygons = PX_NEW_TEMP(PxHullPolygon)[triangles.size()];

	PxU16 currentIndex = 0;
	PxU32 currentFace = 0;

	// collect expand planes
	for (PxU32 i = 0; i < triangles.size(); i++)
	{
		local::ExpandPlane expandPlane;
		if ((triangles)[i])
		{
			const local::Tri *t = (triangles)[i];
			PxPlane p;
			p.n = t->getNormal(verts);
			p.d = -p.n.dot(verts[(*t)[0]]);			

			// store the polygon
			PxHullPolygon& polygon = polygons[currentFace++];
			polygon.mIndexBase = currentIndex;
			polygon.mNbVerts = 3;
			polygon.mPlane[0] = p.n[0];
			polygon.mPlane[1] = p.n[1];
			polygon.mPlane[2] = p.n[2];
			polygon.mPlane[3] = p.d;

			// store the index list
			indices[currentIndex++] = PxU32((*t)[0]);
			indices[currentIndex++] = PxU32((*t)[1]);
			indices[currentIndex++] = PxU32((*t)[2]);

			expandPlanes.pushBack(p);
		}
	}

	PxTransform obbTransform;
	PxVec3 sides;

	// compute the OBB
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = vertsCount;
	convexDesc.points.data = verts;
	convexDesc.points.stride = sizeof(PxVec3);

	convexDesc.indices.count = currentIndex;
	convexDesc.indices.stride = sizeof(PxU32);
	convexDesc.indices.data = indices;

	convexDesc.polygons.count = currentFace;
	convexDesc.polygons.data = polygons;
	convexDesc.polygons.stride = sizeof(PxHullPolygon);

	convexDesc.flags = mConvexMeshDesc.flags;
	
	computeOBBFromConvex(convexDesc, sides, obbTransform);

	// free the memory used for the convex mesh desc
	PX_FREE_AND_RESET(indices);
	PX_FREE_AND_RESET(polygons);

	// crop the OBB
	PxU32 maxplanes = PxMin(PxU32(256), expandPlanes.size());

	ConvexHull* c = PX_NEW_TEMP(ConvexHull)(sides*0.5f, obbTransform, expandPlanes);

	const float planeTolerance = mPlaneTolerance;
	const float epsilon = mTolerance;

	PxI32 k;
	while (maxplanes-- && (k = c->findCandidatePlane(planeTolerance, epsilon)) >= 0)
	{
		ConvexHull* tmp = c;
		c = convexHullCrop(*tmp, expandPlanes[PxU32(k)], planeTolerance);
		if (c == NULL)
		{
			c = tmp;
			break;
		} // might want to debug this case better!!!
		if (!c->assertIntact(planeTolerance))
		{
			PX_DELETE(c);
			c = tmp;
			break;
		} // might want to debug this case better too!!!

		// check for vertex limit
		if (c->getVertices().size() > mConvexMeshDesc.vertexLimit)
		{
			PX_DELETE(c);
			c = tmp;
			maxplanes = 0;
			break;
		}
		PX_DELETE(tmp);
	}

	PX_ASSERT(c->assertIntact(planeTolerance));

	hullOut = c;

	return ConvexHullLibResult::eSUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// calculate the planes from given triangles
// 1. merge triangles with similar normal
// 2. inflate the planes
// 3. store the new triangles
bool InflationConvexHullLib::calchullplanes(const PxVec3* verts, local::HullTriangles&  triangles, Ps::Array<PxPlane>& planes)
{
	PxU32 i,j;
	float maxdot_minang = cosf(Ps::degToRad(local::MIN_ADJACENT_ANGLE));

	// parse the triangles and check the angle between them, if the angle is below MIN_ADJACENT_ANGLE
	// merge the triangles into single plane
	for(i=0;i<triangles.size();i++)
	{
		if(triangles[i])
		{
			for(j=i+1;j<triangles.size();j++)
			{
				if(triangles[i] && triangles[j])
				{
					local::Tri *ti = triangles[i];
					local::Tri *tj = triangles[j];
					PxVec3 ni = ti->getNormal(verts);
					PxVec3 nj = tj->getNormal(verts);
					if(ni.dot(nj) > maxdot_minang)
					{
						// somebody has to die, keep the biggest triangle
						if( ti->getArea2(verts) < tj->getArea2(verts))
						{							
							triangles.deleteTri(triangles[i]);
						}
						else
						{
							triangles.deleteTri(triangles[j]);
						}
					}
				}
			}
		}
	}

	// now add for each triangle that left a plane
	for(i=0;i<triangles.size();i++)
	{
		if(triangles[i])
		{

			local::Tri *t = triangles[i];
			PxVec3 n = t->getNormal(verts);
			float d   = -n.dot(verts[(*t)[0]]) - mCookingParams.skinWidth;
			PxPlane p(n,d);
			planes.pushBack(p);
		}
	}

	// delete the triangles we don't need them anymore
	for(i=0;i< triangles.size(); i++)
	{
		if(triangles[i])
		{			
			triangles.deleteTri(triangles[i]);
		}
	}
	triangles.getTriangles().clear();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// create new points from the given planes, which form the new hull
// 1. form an AABB from the input verts
// 2. slice the AABB with the planes
// 3. if sliced hull is still valid use it, otherwise step back, try different plane
// 4. exit if limit reached or all planes added
bool InflationConvexHullLib::overhull(const PxVec3* verts, PxU32 vertsCount,const Ps::Array<PxPlane>& planes, ConvexHull*& hullOut)
{
	PxU32 i,j;
	if(vertsCount < 4) 
		return false;

	const PxU32 planesLimit = 256;
	PxU32 maxplanes = PxMin(planesLimit,planes.size());

	// get the bounds
	PxBounds3 bounds;
	bounds.setEmpty();
	for(i=0;i<vertsCount;i++) 
	{
		bounds.include(verts[i]);
	}
	float diameter = bounds.getDimensions().magnitude();
	PxVec3 emin = bounds.minimum; 
	PxVec3 emax = bounds.maximum; 
	float epsilon  = 0.01f; // size of object is taken into account within candidate plane function.  Used to multiply here by magnitude(emax-emin) 
	float planetestepsilon = (emax-emin).magnitude() * local::PAPERWIDTH;
	// todo: add bounding cube planes to force bevel. or try instead not adding the diameter expansion ??? must think.
	// ConvexH *convex = ConvexHMakeCube(bmin - float3(diameter,diameter,diameter),bmax+float3(diameter,diameter,diameter));

	// now expand the axis aligned planes by half diameter, !AB what is the point here?
	float maxdot_minang = cosf(Ps::degToRad(local::MIN_ADJACENT_ANGLE));
	for(j=0;j<6;j++)
	{
		PxVec3 n(0,0,0);
		n[j/2] = (j%2) ? 1.0f : -1.0f;
		for(i=0; i < planes.size(); i++)
		{
			if(n.dot(planes[i].n) > maxdot_minang)
			{
				(*((j%2)?&emax:&emin)) += n * (diameter*0.5f);
				break;
			}
		}
	}

	ConvexHull* c = PX_NEW_TEMP(ConvexHull)(emin,emax, planes);
	PxI32 k;
	// find the candidate plane and crop the hull 
	while(maxplanes-- && (k= c->findCandidatePlane(planetestepsilon, epsilon))>=0)
	{
		ConvexHull* tmp = c;
		c = convexHullCrop(*tmp,planes[PxU32(k)], planetestepsilon);
		if(c==NULL) 
		{
			c=tmp; 
			break;
		} // might want to debug this case better!!!
		if(!c->assertIntact(planetestepsilon)) 
		{
			PX_DELETE(c); 
			c=tmp; 
			break;
		} // might want to debug this case better too!!!

		// check for vertex limit
		if(c->getVertices().size() > mConvexMeshDesc.vertexLimit)
		{
			PX_DELETE(c); 
			c=tmp; 
			maxplanes = 0;
			break;
		}
		// check for polygons hard limit
		if(c->getFacets().size() > 255)
		{
			PX_DELETE(c); 
			c=tmp; 
			maxplanes = 0;
			break;
		}
		// check for vertex limit per face if necessary, GRB supports max 32 verts per face
		if ((mConvexMeshDesc.flags & PxConvexFlag::eGPU_COMPATIBLE) && c->maxNumVertsPerFace() > gpuMaxVertsPerFace)
		{
			PX_DELETE(c);
			c = tmp;
			maxplanes = 0;
			break;
		}
		PX_DELETE(tmp);
	}

	PX_ASSERT(c->assertIntact(planetestepsilon));
	hullOut = c;

	return true;
}


//////////////////////////////////////////////////////////////////////////
// fill the data 
void InflationConvexHullLib::fillConvexMeshDesc(PxConvexMeshDesc& outDesc)
{
	PX_ASSERT(mFinished);

	outDesc.indices.count = mHullResult.mIndexCount;
	outDesc.indices.stride = sizeof(PxU32);
	outDesc.indices.data = mHullResult.mIndices;

	outDesc.points.count = mHullResult.mVcount;
	outDesc.points.stride = sizeof(PxVec3);
	outDesc.points.data = mHullResult.mVertices;

	outDesc.polygons.count = mHullResult.mPolygonCount;
	outDesc.polygons.stride = sizeof(PxHullPolygon);
	outDesc.polygons.data = mHullResult.mPolygons;

	swapLargestFace(outDesc);

	if(mConvexMeshDesc.flags & PxConvexFlag::eSHIFT_VERTICES)
		shiftConvexMeshDesc(outDesc);
}

//////////////////////////////////////////////////////////////////////////

InflationConvexHullLib::~InflationConvexHullLib()
{
	if(mHullResult.mIndices)
	{
		PX_FREE(mHullResult.mIndices);
	}

	if(mHullResult.mPolygons)
	{
		PX_FREE(mHullResult.mPolygons);
	}

	if(mHullResult.mVertices)
	{
		PX_FREE(mHullResult.mVertices);
	}
}

//////////////////////////////////////////////////////////////////////////
