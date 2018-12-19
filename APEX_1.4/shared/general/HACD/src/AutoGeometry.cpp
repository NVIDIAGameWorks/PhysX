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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AutoGeometry.h"
#include "ConvexHull.h"
#include "ApexUsingNamespace.h"

namespace HACD
{

	void  fm_transform(const float matrix[16],const float v[3],float t[3]) // rotate and translate this point
	{
		if ( matrix )
		{
			float tx = (matrix[0*4+0] * v[0]) +  (matrix[1*4+0] * v[1]) + (matrix[2*4+0] * v[2]) + matrix[3*4+0];
			float ty = (matrix[0*4+1] * v[0]) +  (matrix[1*4+1] * v[1]) + (matrix[2*4+1] * v[2]) + matrix[3*4+1];
			float tz = (matrix[0*4+2] * v[0]) +  (matrix[1*4+2] * v[1]) + (matrix[2*4+2] * v[2]) + matrix[3*4+2];
			t[0] = tx;
			t[1] = ty;
			t[2] = tz;
		}
		else
		{
			t[0] = v[0];
			t[1] = v[1];
			t[2] = v[2];
		}
	}
	inline float det(const float *p1,const float *p2,const float *p3)
	{
		return  p1[0]*p2[1]*p3[2] + p2[0]*p3[1]*p1[2] + p3[0]*p1[1]*p2[2] -p1[0]*p3[1]*p2[2] - p2[0]*p1[1]*p3[2] - p3[0]*p2[1]*p1[2];
	}


	float  fm_computeMeshVolume(const float *vertices,size_t tcount,const uint32_t *indices)
	{
		float volume = 0;

		for (uint32_t i=0; i<tcount; i++,indices+=3)
		{
			const float *p1 = &vertices[ indices[0]*3 ];
			const float *p2 = &vertices[ indices[1]*3 ];
			const float *p3 = &vertices[ indices[2]*3 ];
			volume+=det(p1,p2,p3); // compute the volume of the tetrahedran relative to the origin.
		}

		volume*=(1.0f/6.0f);
		if ( volume < 0 )
			volume*=-1;
		return volume;
	}



	class Vec3
	{
	public:
		Vec3(const float *pos)
		{
			x = pos[0];
			y = pos[1];
			z = pos[2];
		}
		float x;
		float y;
		float z;
	};

	typedef hacd::vector< Vec3 > Vec3Vector;

	class MyHull : public HACD::SimpleHull, public UANS::UserAllocated
	{
	public:
		MyHull(void)
		{
			mValidHull = false;
		}

		~MyHull(void)
		{
			release();
		}

		void release(void)
		{
			HACD_FREE(mIndices);
			HACD_FREE(mVertices);
			mIndices = 0;
			mVertices = 0;
			mTriCount = 0;
			mVertexCount = 0;
		}

		void addPos(const float *p)
		{
			Vec3 v(p);
			mPoints.push_back(v);
		}

		float generateHull(void)
		{
			release();
			if ( mPoints.size() >= 3 ) // must have at least 3 vertices to create a hull.
			{
				// now generate the convex hull.
				HullDesc desc((uint32_t)mPoints.size(),&mPoints[0].x, sizeof(float)*3);
				desc.mMaxVertices = 32;
				desc.mSkinWidth = 0.001f;

				HullLibrary h;
				HullResult result;
				HullError e = h.CreateConvexHull(desc,result);
				if ( e == QE_OK )
				{
					mTriCount = result.mNumTriangles;
					mIndices  = (uint32_t *)HACD_ALLOC(sizeof(uint32_t)*mTriCount*3);
					memcpy(mIndices,result.mIndices,sizeof(uint32_t)*mTriCount*3);
					mVertexCount = result.mNumOutputVertices;
					mVertices = (float *)HACD_ALLOC(sizeof(float)*mVertexCount*3);
					memcpy(mVertices,result.mOutputVertices,sizeof(float)*mVertexCount*3);
					mValidHull = true;
					mMeshVolume = fm_computeMeshVolume( mVertices, mTriCount, mIndices ); // compute the volume of this mesh.
					h.ReleaseResult(result);
				}
			}
			return mMeshVolume;
		}

		void inherit(MyHull &c)
		{
			Vec3Vector::iterator i;
			for (i=c.mPoints.begin(); i!=c.mPoints.end(); ++i)
			{
				mPoints.push_back( (*i) );
			}
			c.mPoints.clear();
			c.mValidHull = false;
			generateHull();
		}

		void setTransform(const HACD::SimpleBone &b,int32_t bone_index)
		{
			mBoneName    = b.mBoneName;
			mBoneIndex   = bone_index;
			mParentIndex = b.mParentIndex;
			memcpy(mTransform,b.mTransform,sizeof(float)*16);
			if ( mVertexCount )
			{
				for (uint32_t i=0; i<mVertexCount; i++)
				{
					float *vtx = &mVertices[i*3];
					fm_transform(b.mInverseTransform,vtx,vtx); // inverse transform the point into bone relative object space
				}
			}

		}

		bool        mValidHull;
		Vec3Vector  mPoints;
	};

	class MyAutoGeometry : public HACD::AutoGeometry, public UANS::UserAllocated
	{
	public:
		MyAutoGeometry()
		{
			mHulls = 0;
			mSimpleHulls = 0;
		}

		virtual ~MyAutoGeometry(void)
		{
			release();
		}

		void release(void)
		{
			delete []mHulls;
			mHulls = 0;
			HACD_FREE(mSimpleHulls);
			mSimpleHulls = 0;
		}

#define MAX_BONE_COUNT 8

		void addBone(uint32_t bone,uint32_t *bones,uint32_t &bcount)
		{
			HACD_ASSERT(bcount < MAX_BONE_COUNT);
			if ( bcount < MAX_BONE_COUNT )
			{
				bool found = false;

				for (uint32_t i=0; i<bcount; i++)
				{
					if ( bones[i] == bone )
					{
						found = true;
						break;
					}
				}
				if ( !found )
				{
					bones[bcount] = bone;
					bcount++;
				}
			}
		}

		void addBones(const HACD::SimpleSkinnedVertex &v,uint32_t *bones,uint32_t &bcount, const HACD::SimpleBone *sbones)
		{
			if ( v.mWeight[0] >= sbones[v.mBone[0]].mBoneMinimalWeight) addBone(v.mBone[0],bones,bcount);
			if ( v.mWeight[1] >= sbones[v.mBone[1]].mBoneMinimalWeight) addBone(v.mBone[1],bones,bcount);
			if ( v.mWeight[2] >= sbones[v.mBone[2]].mBoneMinimalWeight) addBone(v.mBone[2],bones,bcount);
			if ( v.mWeight[3] >= sbones[v.mBone[3]].mBoneMinimalWeight) addBone(v.mBone[3],bones,bcount);
		}

		void addTri(const HACD::SimpleSkinnedVertex &v1,const HACD::SimpleSkinnedVertex &v2,const HACD::SimpleSkinnedVertex &v3,const HACD::SimpleBone *sbones)
		{
			uint32_t bcount = 0;
			uint32_t bones[MAX_BONE_COUNT];
			addBones(v1,bones,bcount, sbones);
			addBones(v2,bones,bcount, sbones);
			addBones(v3,bones,bcount, sbones);
			for (uint32_t i=0; i<bcount; i++)
			{
				addPos(v1.mPos, (int32_t)bones[i], sbones );
				addPos(v2.mPos, (int32_t)bones[i], sbones );
				addPos(v3.mPos, (int32_t)bones[i], sbones );
			}
		}

		virtual HACD::SimpleHull ** createCollisionVolumes(float collapse_percentage,
			uint32_t bone_count,
			const HACD::SimpleBone *bones,
			const HACD::SimpleSkinnedMesh *mesh,
			uint32_t &geom_count)
		{
			release();
			geom_count = 0;

			mHulls = HACD_NEW(MyHull)[bone_count];

			for (uint32_t i=0; i<bone_count; i++)
			{
				const HACD::SimpleBone &b = bones[i];
				mHulls[i].setTransform(b,(int32_t)i);
			}

			uint32_t tcount = mesh->mVertexCount/3;

			for (uint32_t i=0; i<tcount; i++)
			{
				const HACD::SimpleSkinnedVertex &v1 = mesh->mVertices[i*3+0];
				const HACD::SimpleSkinnedVertex &v2 = mesh->mVertices[i*3+0];
				const HACD::SimpleSkinnedVertex &v3 = mesh->mVertices[i*3+0];
				addTri(v1,v2,v3,bones);
			}

			float totalVolume = 0;
			for (uint32_t i=0; i<bone_count; i++)
			{
				totalVolume+=mHulls[i].generateHull();
			}

			// ok.. now do auto-collapse of hulls...
#if 1
			if ( collapse_percentage > 0 )
			{
				float ratio = collapse_percentage / 100.0f;
				for (int32_t i=(int32_t)(bone_count-1); i>=0; i--)
				{
					MyHull &h = mHulls[i];
					const HACD::SimpleBone &b = bones[i];
					if ( b.mParentIndex >= 0 )
					{
						MyHull &parent_hull = mHulls[b.mParentIndex];
						if ( h.mValidHull && parent_hull.mValidHull )
						{
							if ( h.mMeshVolume < (parent_hull.mMeshVolume*ratio) ) // if we are less than 1/3 the volume of our parent, copy our vertices to the parent..
							{
								parent_hull.inherit(h);
							}
						}
					}
				}
			}
#endif
			for (int32_t i=0; i<(int32_t)bone_count; i++)
			{
				MyHull &h = mHulls[i];
				if ( h.mValidHull )
					geom_count++;
			}

			if ( geom_count )
			{
				mSimpleHulls = (HACD::SimpleHull **)HACD_ALLOC(sizeof(HACD::SimpleHull *)*geom_count);
				int32_t index = 0;
				for (int32_t i=0; i<(int32_t)bone_count; i++)
				{
					MyHull *hull = &mHulls[i];
					if ( hull->mValidHull )
					{
						const HACD::SimpleBone &b = bones[i];
						hull->setTransform(b,i);
						mSimpleHulls[index] = hull;
						index++;
					}
				}
			}

			return mSimpleHulls;
		}

		void addPos(const float *p,int32_t bone,const HACD::SimpleBone *bones)
		{
			switch ( bones[bone].mOption )
			{
			case HACD::BO_INCLUDE:
				mHulls[bone].addPos(p);
				break;
			case HACD::BO_EXCLUDE:
				break;
			case HACD::BO_COLLAPSE:
				{
					while ( bone >= 0 )
					{
						bone = bones[bone].mParentIndex;
						if ( bones[bone].mOption == HACD::BO_INCLUDE )
							break;
						else if ( bones[bone].mOption == HACD::BO_EXCLUDE )
						{
							bone = -1;
							break;
						}
					}
					if ( bone >= 0 )
					{
						mHulls[bone].addPos(p); // collapse into the parent
					}
				}
				break;
			}
		}

		virtual HACD::SimpleHull ** createCollisionVolumes(float collapse_percentage,uint32_t &geom_count)
		{
			HACD::SimpleHull **ret = 0;

			if ( !mVertices.empty() && !mBones.empty() )
			{
				HACD::SimpleSkinnedMesh mesh;
				mesh.mVertexCount = (uint32_t)mVertices.size();
				mesh.mVertices    = &mVertices[0];
				ret = createCollisionVolumes(collapse_percentage,(uint32_t)mBones.size(),&mBones[0],&mesh,geom_count);
				mVertices.clear();
				mBones.clear();
			}

			return ret;
		}

		virtual void addSimpleSkinnedTriangle(const HACD::SimpleSkinnedVertex &v1,const HACD::SimpleSkinnedVertex &v2,const HACD::SimpleSkinnedVertex &v3)
		{
			mVertices.push_back(v1);
			mVertices.push_back(v2);
			mVertices.push_back(v3);
		}

		virtual void addSimpleBone(const HACD::SimpleBone &_b)
		{
			HACD::SimpleBone b = _b;
			mBones.push_back(b);
		}

		void mystrlwr(char *str)
		{
			while ( *str )
			{
				char c = *str;
				if ( c >= 'A' && c <= 'Z' )
				{
					c+=32;
					*str = c;
				}
				str++;
			}
		}

		virtual const char * stristr(const char *str,const char *key)  // case insensitive ststr
		{
			HACD_ASSERT( strlen(str) < 2048 );
			HACD_ASSERT( strlen(key) < 2048 );

			char istr[2048];
			char ikey[2048];

			strncpy(istr,str,2048);
			strncpy(ikey,key,2048);
			mystrlwr(istr);
			mystrlwr(ikey);

			char *foo = strstr(istr,ikey);
			if ( foo )
			{
				uint32_t loc = (uint32_t)(foo - istr);
				foo = (char *)str+loc;
			}

			return foo;
		}

	private:
		typedef hacd::vector< HACD::SimpleBone > SimpleBoneVector;
		typedef hacd::vector< HACD::SimpleSkinnedVertex > SimpleSkinnedVertexVector;
		SimpleBoneVector mBones;
		SimpleSkinnedVertexVector mVertices;

		MyHull      *mHulls;
		HACD::SimpleHull **mSimpleHulls;
	};


	HACD::AutoGeometry * createAutoGeometry()
	{
		HACD::MyAutoGeometry *g = HACD_NEW(HACD::MyAutoGeometry);
		return static_cast< HACD::AutoGeometry *>(g);
	}

	void           releaseAutoGeometry(HACD::AutoGeometry *g)
	{
		HACD::MyAutoGeometry * m = static_cast<HACD::MyAutoGeometry *>(g);
		delete m;
	}

}; // end of namespace
