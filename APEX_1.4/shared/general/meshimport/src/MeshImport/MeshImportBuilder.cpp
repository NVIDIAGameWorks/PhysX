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
#pragma  warning(disable:4555)
#include "MeshImportBuilder.h"
#include "VtxWeld.h"
#include "MiStringDictionary.h"
#include "MiFloatMath.h"
#include "MiMemoryBuffer.h"
#include "MiIOStream.h"
#include "ExportEZB.h"

#pragma warning(disable:4100 4189)

namespace mimp
{

typedef STDNAME::vector< MeshVertex >   MeshVertexVector;
typedef STDNAME::vector< MiU32 > MeshIndexVector;
typedef STDNAME::vector< SubMesh * >    SubMeshVector;
typedef STDNAME::vector< Mesh * >       MeshVector;
typedef STDNAME::vector< MeshAnimation * > MeshAnimationVector;
typedef STDNAME::vector< MeshSkeleton * > MeshSkeletonVector;
typedef STDNAME::vector< MeshRawTexture *> MeshRawTextureVector;
typedef STDNAME::vector< MeshTetra * > MeshTetraVector;
typedef STDNAME::vector< MeshUserData *> MeshUserDataVector;
typedef STDNAME::vector< MeshUserBinaryData * > MeshUserBinaryDataVector;
typedef STDNAME::vector< MeshPairCollision > MeshPairCollisionVector;

static MiI32 gSerializeFrame=1;

class MyMesh;

class MySubMesh : public SubMesh
{
public:
  MySubMesh(const char *mat,MiU32 vertexFlags)
  {
    mMaterialName = mat;
    mVertexFlags  = (MeshVertexFlag)vertexFlags;
  }

  ~MySubMesh(void)
  {
  }

  bool isSame(const char *mat,MiU32 vertexFlags) const
  {
    bool ret = false;
    assert(mat);
    assert(mMaterialName);
    if ( strcmp(mat,mMaterialName) == 0 && mVertexFlags == vertexFlags ) ret = true;
    return ret;
  }

  void gather(void)
  {
    mTriCount = (MiU32)mMyIndices.size()/3;
    mIndices  = &mMyIndices[0];
  }

  void add(const MeshVertex &v1,const MeshVertex &v2,const MeshVertex &v3,VertexPool< MeshVertex > &vpool)
  {
    mAABB.include(v1.mPos);
    mAABB.include(v2.mPos);
    mAABB.include(v3.mPos);

    MiU32 i1 = (MiU32)vpool.GetVertex(v1);
    MiU32 i2 = (MiU32)vpool.GetVertex(v2);
    MiU32 i3 = (MiU32)vpool.GetVertex(v3);

    mMyIndices.push_back(i1);
    mMyIndices.push_back(i2);
    mMyIndices.push_back(i3);
  }

  MeshIndexVector          mMyIndices;
  VertexPool< MeshVertex > mVertexPool;
};

class MyMesh : public Mesh
{
public:
  MyMesh(const char *meshName,const char *skeletonName)
  {
    mName = meshName;
    mSkeletonName = skeletonName;
    mCurrent = 0;
  }

  virtual ~MyMesh(void)
  {
    release();
  }

  void release(void)
  {
    mSubMeshes = 0;
    mSubMeshCount = 0;
    SubMeshVector::iterator i;
    for (i=mMySubMeshes.begin(); i!=mMySubMeshes.end(); ++i)
    {
      MySubMesh *s = static_cast<MySubMesh *>((*i));
      delete(s);
    }
    mMySubMeshes.clear();
  }

  bool isSame(const char *meshName) const
  {
    bool ret = false;
    assert(meshName);
    assert(mName);
    if ( strcmp(mName,meshName) == 0 ) ret = true;
    return ret;
  }

  void getCurrent(const char *materialName,MiU32 vertexFlags)
  {
    if ( materialName == 0 ) materialName = "default_material";
    if ( mCurrent == 0 || !mCurrent->isSame(materialName,vertexFlags) )
    {
      mCurrent =0;
      SubMeshVector::iterator i;
      for (i=mMySubMeshes.begin(); i!=mMySubMeshes.end(); ++i)
      {
        MySubMesh *s = static_cast< MySubMesh *>((*i));
        if ( s->isSame(materialName,vertexFlags) )
        {
          mCurrent = s;
          break;
        }
      }
      if ( mCurrent == 0 )
      {
        mCurrent = (MySubMesh *)MI_ALLOC(sizeof(MySubMesh));
		new (mCurrent ) MySubMesh(materialName,vertexFlags);
        mMySubMeshes.push_back(mCurrent);
      }
    }
  }

  virtual void        importTriangle(const char *materialName,
                                     MiU32 vertexFlags,
                                     const MeshVertex &_v1,
                                     const MeshVertex &_v2,
                                     const MeshVertex &_v3)
  {
	  MeshVertex v1 = _v1;
	  MeshVertex v2 = _v2;
	  MeshVertex v3 = _v3;
	  fm_normalize(v1.mNormal);
	  fm_normalize(v1.mBiNormal);
	  fm_normalize(v1.mTangent);

	  fm_normalize(v2.mNormal);
	  fm_normalize(v2.mBiNormal);
	  fm_normalize(v2.mTangent);

	  fm_normalize(v3.mNormal);
	  fm_normalize(v3.mBiNormal);
	  fm_normalize(v3.mTangent);


    mAABB.include( v1.mPos );
    mAABB.include( v2.mPos );
    mAABB.include( v3.mPos );
    getCurrent(materialName,vertexFlags);
    mVertexFlags|=vertexFlags;

    mCurrent->add(v1,v2,v3,mVertexPool);
  }

  virtual void        importIndexedTriangleList(const char *materialName,
                                                MiU32 vertexFlags,
                                                MiU32 /*vcount*/,
                                                const MeshVertex *vertices,
                                                MiU32 tcount,
                                                const MiU32 *indices)
  {
    for (MiU32 i=0; i<tcount; i++)
    {
      MiU32 i1 = indices[i*3+0];
      MiU32 i2 = indices[i*3+1];
      MiU32 i3 = indices[i*3+2];
      const MeshVertex &v1 = vertices[i1];
      const MeshVertex &v2 = vertices[i2];
      const MeshVertex &v3 = vertices[i3];
      importTriangle(materialName,vertexFlags,v1,v2,v3);
    }
  }

  void gather(MiI32 bone_count)
  {
    mSubMeshes = 0;
    mSubMeshCount = 0;
    if ( !mMySubMeshes.empty() )
    {
      mSubMeshCount = (MiU32)mMySubMeshes.size();
      mSubMeshes    = &mMySubMeshes[0];
      for (MiU32 i=0; i<mSubMeshCount; i++)
      {
        MySubMesh *m = static_cast<MySubMesh *>(mSubMeshes[i]);
        m->gather();
      }
    }
    mVertexCount = (MiU32)mVertexPool.GetSize();
    if ( mVertexCount > 0 )
    {
      mVertices = mVertexPool.GetBuffer();
      if ( bone_count > 0 )
      {
        for (MiU32 i=0; i<mVertexCount; i++)
        {
          MeshVertex &vtx = mVertices[i];
          if ( vtx.mBone[0] >= bone_count ) vtx.mBone[0] = 0;
          if ( vtx.mBone[1] >= bone_count ) vtx.mBone[1] = 0;
          if ( vtx.mBone[2] >= bone_count ) vtx.mBone[2] = 0;
          if ( vtx.mBone[3] >= bone_count ) vtx.mBone[3] = 0;
        }
      }
    }
  }

  VertexPool< MeshVertex > mVertexPool;
  MySubMesh        *mCurrent;
  SubMeshVector   mMySubMeshes;
};

typedef STDNAME::map< StringRef, StringRef > StringRefMap;
typedef STDNAME::vector< MeshMaterial >      MeshMaterialVector;
typedef STDNAME::vector< MeshInstance >      MeshInstanceVector;
typedef STDNAME::vector< MyMesh *>           MyMeshVector;
typedef STDNAME::vector< Mesh *>			 MeshVector;
typedef STDNAME::vector< MeshCollision * >   MeshCollisionVector;

class MyMeshCollisionRepresentation : public MeshCollisionRepresentation, public MeshImportAllocated
{
public:
	MyMeshCollisionRepresentation(const MeshCollisionRepresentation &mr,StringDict &dictionary)
	{
		bool first;
		mName = dictionary.Get(mr.mName,first);
		mInfo = dictionary.Get(mr.mInfo,first);
		mCollisionCount = mr.mCollisionCount;
		mCollisionGeometry = (MeshCollision **)MI_ALLOC( sizeof(MeshCollision *)*mCollisionCount);

		for (MiU32 i=0; i<mCollisionCount; i++)
		{
			const MeshCollision *source = mr.mCollisionGeometry[i];
			switch ( source->getType() )
			{
				case MCT_BOX:
					{
						MeshCollisionBox *dest = (MeshCollisionBox *)MI_ALLOC(sizeof(MeshCollisionBox));
						new ( dest ) MeshCollisionBox;
						const MeshCollisionBox *box = static_cast< const MeshCollisionBox *>(source);
						*dest = *box;
						dest->mName = dictionary.Get(box->mName,first);
						mCollisionGeometry[i] = dest;
					}
					break;
				case MCT_SPHERE:
					{
						MeshCollisionSphere *dest = (MeshCollisionSphere *)MI_ALLOC(sizeof(MeshCollisionSphere));
						new ( dest ) MeshCollisionSphere;
						const MeshCollisionSphere *sphere = static_cast< const MeshCollisionSphere *>(source);
						*dest = *sphere;
						dest->mName = dictionary.Get(sphere->mName,first);
						mCollisionGeometry[i] = dest;
					}
					break;
				case MCT_CONVEX:
					{
						MeshCollisionConvex *dest = (MeshCollisionConvex *)MI_ALLOC(sizeof(MeshCollisionConvex));
						new ( dest ) MeshCollisionConvex;
						const MeshCollisionConvex *convex = static_cast< const MeshCollisionConvex *>(source);
						*dest = *convex;
						if ( convex->mTriCount )
						{
							dest->mIndices = (MiU32 *)MI_ALLOC(sizeof(MiU32)*convex->mTriCount*3);
							memcpy(dest->mIndices,convex->mIndices,sizeof(MiU32)*convex->mTriCount*3);
						}
						else
						{
							dest->mIndices = NULL;
						}
						if ( convex->mVertexCount )
						{
							dest->mVertices = (MiF32 *)MI_ALLOC(sizeof(MiF32)*convex->mVertexCount*3);
							memcpy(dest->mVertices,convex->mVertices,sizeof(MiF32)*convex->mVertexCount*3);
						}
						else
						{
							dest->mVertices = NULL;
						}
						dest->mName = dictionary.Get(convex->mName,first);
						mCollisionGeometry[i] = dest;
					}
					break;
				default:
					assert(0);
					break;
			}
		}

		mPosition[0] = mr.mPosition[0];
		mPosition[1] = mr.mPosition[1];
		mPosition[2] = mr.mPosition[2];
		mOrientation[0] = mr.mOrientation[0];
		mOrientation[1] = mr.mOrientation[1];
		mOrientation[2] = mr.mOrientation[2];
		mOrientation[3] = mr.mOrientation[3];
		mSolverCount = mr.mSolverCount;
		mAwake = mr.mAwake;
	}

	~MyMeshCollisionRepresentation(void)
	{
		for (MiU32 i=0; i<mCollisionCount; i++)
		{
			MeshCollision *mc = mCollisionGeometry[i];
			delete(mc);
		}
		MI_FREE(mCollisionGeometry);
	}

	void gather(void)
	{
	}
};

typedef STDNAME::vector< MeshCollisionRepresentation * > MeshCollisionRepresentationVector;
typedef STDNAME::vector< MeshSimpleJoint > MeshSimpleJointVector;

class MyMeshBuilder : public MeshBuilder
{
public:
  MyMeshBuilder(const char *meshName,const void *data,MiU32 dlen,MeshImporter *mi,const char *options,MeshImportApplicationResource *appResource)
  {
    gSerializeFrame++;
    mCurrentMesh = 0;
    mAppResource = appResource;
    importAssetName(meshName,0);
	if ( mi )
	{
		mDeserializeBinary = false;
		mImportState = mi->importMesh(meshName,data,dlen,this,options,appResource);
	}
	else
	{
		mDeserializeBinary = true;
		mImportState = deserializeBinary(data,dlen,options,appResource);
	}
	if (mImportState)
	{
		gather();
	}
  }

  MyMeshBuilder(MeshImportApplicationResource *appResource)
  {
    gSerializeFrame++;
    mCurrentMesh = 0;
    mAppResource = appResource;
    mDeserializeBinary = false;
	mImportState = true;
  }


	virtual ~MyMeshBuilder(void)
	{
		if ( mDeserializeBinary )
		{
			for (MeshVector::iterator i=mFlatMeshes.begin(); i!=mFlatMeshes.end(); ++i)
			{
				Mesh *m = (*i);
				MI_FREE(m->mVertices);
				for (MiU32 j=0; j<m->mSubMeshCount; j++)
				{
					SubMesh *sm = m->mSubMeshes[j];
					MI_FREE(sm->mIndices);
					delete(sm);
				}
				delete(m);
			}
		}
		else
		{
			MI_FREE(mMeshes);
			MyMeshVector::iterator i;
			for (i=mMyMeshes.begin(); i!=mMyMeshes.end(); ++i)
			{
				MyMesh *src = (*i);
				delete src;
			}
		}

		if ( !mMyAnimations.empty() )
		{
			MeshAnimationVector::iterator i;
			for (i=mMyAnimations.begin(); i!=mMyAnimations.end(); ++i)
			{
				MeshAnimation *a = (*i);
				for (MiI32 j=0; j<a->mTrackCount; j++)
				{
					MeshAnimTrack *ma = a->mTracks[j];
					delete(ma->mPose);
					delete(ma);
				}
				MI_FREE(a->mTracks);
				delete(a);
			}
		}

		if ( !mMySkeletons.empty() )
		{
			MeshSkeletonVector::iterator i;
			for (i=mMySkeletons.begin(); i!=mMySkeletons.end(); ++i)
			{
				MeshSkeleton *s = (*i);
				delete(s->mBones);
				delete(s);
			}
		}
		if ( !mCollisionReps.empty() )
		{
			MeshCollisionRepresentationVector::iterator i;
			for (i=mCollisionReps.begin(); i!=mCollisionReps.end(); ++i)
			{
				MyMeshCollisionRepresentation *mcr = static_cast< MyMeshCollisionRepresentation *>(*i);
				delete mcr;
			}
		}
		for (MeshRawTextureVector::iterator i=mMyRawTextures.begin(); i!=mMyRawTextures.end(); ++i)
		{
			MeshRawTexture *t = (*i);
			MI_FREE(t->mData);
			delete(t);
		}
		for (MeshTetraVector::iterator i=mMyMeshTextra.begin(); i!=mMyMeshTextra.end(); ++i)
		{
			MeshTetra *m = (*i);
			delete(m);
		}
		for (MeshUserDataVector::iterator i=mMyMeshUserData.begin(); i!=mMyMeshUserData.end(); ++i)
		{
			MeshUserData *m = (*i);
			delete(m);
		}
		for (MeshUserBinaryDataVector::iterator i=mMyMeshUserBinaryData.begin(); i!=mMyMeshUserBinaryData.end(); ++i)
		{
			MeshUserBinaryData *m = (*i);
			MI_FREE(m->mUserData);
			delete(m);
		}
	}

	virtual void setMeshImportBinary(bool state)
	{
		mDeserializeBinary = state;
	}


	void gather(void)
	{
		if ( !mDeserializeBinary )
		{
			gatherMaterials();
			mMaterialCount = (MiU32)mMyMaterials.size();
			if ( mMaterialCount )
			{
				mMaterials     = &mMyMaterials[0];
			}
			else
			{
				mMaterials = 0;
			}
		}
		mMeshInstanceCount = (MiU32)mMyMeshInstances.size();
		if ( mMeshInstanceCount )
		{
			mMeshInstances = &mMyMeshInstances[0];
		}
		else
		{
			mMeshInstances = 0;
		}

		mAnimationCount = (MiU32)mMyAnimations.size();
		if ( mAnimationCount )
			mAnimations = &mMyAnimations[0];
		else
			mAnimations = 0;

		MiI32 bone_count = 0;
		mSkeletonCount = (MiU32)mMySkeletons.size();
		if ( mSkeletonCount )
		{
			mSkeletons = &mMySkeletons[0];
			bone_count = mSkeletons[0]->mBoneCount;
		}
		else
		{
			mSkeletons = 0;
		}

		if ( mDeserializeBinary )
		{
			mMeshCount = (MiU32)mFlatMeshes.size();
			if ( mMeshCount )
			{
				mMeshes = &mFlatMeshes[0];
			}
			else
			{
				mMeshes = NULL;
			}
		}
		else
		{
			mMeshCount = (MiU32)mMyMeshes.size();
			if ( mMeshCount )
			{
				MI_FREE(mMeshes);
				mMeshes    = (Mesh **)MI_ALLOC(sizeof(Mesh *)*mMeshCount);
				Mesh **dst = mMeshes;
				MyMeshVector::iterator i;
				for (i=mMyMeshes.begin(); i!=mMyMeshes.end(); ++i)
				{
					MyMesh *src = (*i);
					src->gather(bone_count);
					*dst++ = static_cast< Mesh *>(src);
				}
			}
			else
			{
				mMeshes = NULL;
			}
		}

		mMeshCollisionCount = (MiU32)mCollisionReps.size();

		if ( mMeshCollisionCount )
		{
			mMeshCollisionRepresentations = &mCollisionReps[0];
			for (MiU32 i=0; i<mMeshCollisionCount; i++)
			{
				MeshCollisionRepresentation *r = mMeshCollisionRepresentations[i];
				MyMeshCollisionRepresentation *mr = static_cast< MyMeshCollisionRepresentation *>(r);
				mr->gather();
			}
		}
		else
		{
			mMeshCollisionRepresentations = 0;
		}
		//
		mTextureCount = (MiU32)mMyRawTextures.size();
		if ( !mMyRawTextures.empty() )
		{
			mTextures = &mMyRawTextures[0];
		}
		mTetraMeshCount = (MiU32)mMyMeshTextra.size();
		if ( !mMyMeshTextra.empty() )
		{
			mTetraMeshes = &mMyMeshTextra[0];
		}
		mUserDataCount = (MiU32)mMyMeshUserData.size();
		if ( !mMyMeshUserData.empty() )
		{
			mUserData = &mMyMeshUserData[0];
		}
		mUserBinaryDataCount = (MiU32)mMyMeshUserBinaryData.size();
		if ( !mMyMeshUserBinaryData.empty() )
		{
			mUserBinaryData = &mMyMeshUserBinaryData[0];
		}

		mMeshJointCount = (MiU32)mMyJoints.size();
		if ( !mMyJoints.empty() )
		{
			mMeshJoints = &mMyJoints[0];
		}

		mMeshPairCollisionCount = mCollisionPairs.size();
		if ( !mCollisionPairs.empty() )
		{
			mMeshPairCollisions = &mCollisionPairs[0];
		}

		fixupNamedPointers();
	}

  void gatherMaterials(void)
  {
    mMyMaterials.clear();
    MiU32 mcount = (MiU32)mMaterialMap.size();
    mMyMaterials.reserve(mcount);
    StringRefMap::iterator i;
    for (i=mMaterialMap.begin(); i!=mMaterialMap.end(); ++i)
    {
      MeshMaterial m;
      m.mName = (*i).first.Get();
      m.mMetaData = (*i).second.Get();
      mMyMaterials.push_back(m);
    }
  }

  virtual void        importUserData(const char * /*userKey*/,const char * /*userValue*/)       // carry along raw user data as ASCII strings only..
  {
    assert(0); // not yet implemented
  }

  virtual void        importUserBinaryData(const char * /*name*/,MiU32 /*len*/,const MiU8 * /*data*/)
  {
    assert(0); // not yet implemented
  }

  virtual void        importTetraMesh(const char * /*tetraName*/,const char * /*meshName*/,MiU32 /*tcount*/,const MiF32 * /*tetraData*/)
  {
    assert(0); // not yet implemented
  }

  virtual void importMaterial(const char *matName,const char *metaData)
  {
    matName = getMaterialName(matName);
    StringRef m = mStrings.Get(matName);
    StringRef d = mStrings.Get(metaData);
	StringRefMap::iterator found = mMaterialMap.find(m);
	if ( found == mMaterialMap.end() )
	{
		mMaterialMap[m] = d;
	}
	else
	{
		// only overwrite it if we have additional meta data we didn't have before..
		if ( metaData && strlen(metaData))
		{
			mMaterialMap[m] = d;
		}
	}
  }

  virtual void        importAssetName(const char *assetName,const char *info)         // name of the overall asset.
  {
    mAssetName = mStrings.Get(assetName).Get();
    mAssetInfo = mStrings.Get(info).Get();
  }

  virtual void        importMesh(const char *meshName,const char *skeletonName)       // name of a mesh and the skeleton it refers to.
  {
    StringRef m1 = mStrings.Get(meshName);
    StringRef s1 = mStrings.Get(skeletonName);

    mCurrentMesh = 0;
    MyMeshVector::iterator found;
    for (found=mMyMeshes.begin(); found != mMyMeshes.end(); found++)
    {
      MyMesh *mm = (*found);
      if ( mm->mName == m1 )
      {
        mCurrentMesh = mm;
        mCurrentMesh->mSkeletonName = s1.Get();
        break;
      }
    }
    if ( mCurrentMesh == 0 )
    {
      MyMesh *m = (MyMesh *)MI_ALLOC(sizeof(MyMesh));
	  new (m ) MyMesh(m1.Get(),s1.Get());
      mMyMeshes.push_back(m);
      mCurrentMesh = m;
    }
  }

  void getCurrentMesh(const char *meshName)
  {
    if ( meshName == 0 )
    {
      meshName = "default_mesh";
    }
    if ( mCurrentMesh == 0 || !mCurrentMesh->isSame(meshName) )
    {
      importMesh(meshName,0); // make it the new current mesh
    }
  }

  virtual void importPlane(const MiF32 *p)
  {
	  mPlane[0] = p[0];
	  mPlane[1] = p[1];
	  mPlane[2] = p[2];
	  mPlane[3] = p[3];
  }



  virtual void        importTriangle(const char *_meshName,
                                     const char *_materialName,
                                     MiU32 vertexFlags,
                                     const MeshVertex &v1,
                                     const MeshVertex &v2,
                                     const MeshVertex &v3)
  {
    _materialName = getMaterialName(_materialName);
    const char *meshName = mStrings.Get(_meshName).Get();
    const char *materialName = mStrings.Get(_materialName).Get();
    getCurrentMesh(meshName);
    mAABB.include(v1.mPos);
    mAABB.include(v2.mPos);
    mAABB.include(v3.mPos);
    mCurrentMesh->importTriangle(materialName,vertexFlags,v1,v2,v3);
  }

  virtual void        importIndexedTriangleList(const char *_meshName,
                                                const char *_materialName,
                                                MiU32 vertexFlags,
                                                MiU32 vcount,
                                                const MeshVertex *vertices,
                                                MiU32 tcount,
                                                const MiU32 *indices)
  {
    _materialName = getMaterialName(_materialName);
    const char *meshName = mStrings.Get(_meshName).Get();
    const char *materialName = mStrings.Get(_materialName).Get();
    getCurrentMesh(meshName);
    for (MiU32 i=0; i<vcount; i++)
    {
      mAABB.include( vertices[i].mPos );
    }
    mCurrentMesh->importIndexedTriangleList(materialName,vertexFlags,vcount,vertices,tcount,indices);
  }

  // do a deep copy...
  virtual void        importAnimation(const MeshAnimation &animation)
  {
    MeshAnimation *a = (MeshAnimation *)MI_ALLOC(sizeof(MeshAnimation));
	new ( a ) MeshAnimation;
    a->mName = mStrings.Get(animation.mName).Get();
    a->mTrackCount = animation.mTrackCount;
    a->mFrameCount = animation.mFrameCount;
    a->mDuration = animation.mDuration;
    a->mDtime = animation.mDtime;
    a->mTracks = (MeshAnimTrack **)MI_ALLOC(sizeof(MeshAnimTrack *)*a->mTrackCount);
    for (MiI32 i=0; i<a->mTrackCount; i++)
    {
      const MeshAnimTrack &src =*animation.mTracks[i];
      MeshAnimTrack *t = (MeshAnimTrack *)MI_ALLOC(sizeof(MeshAnimTrack));
	  new ( t ) MeshAnimTrack;
      t->mName = mStrings.Get(src.mName).Get();
      t->mFrameCount = src.mFrameCount;
      t->mDuration = src.mDuration;
      t->mDtime = src.mDtime;
      t->mPose = (MeshAnimPose *)MI_ALLOC(sizeof(MeshAnimPose)*t->mFrameCount);
	  for (MiI32 j=0; j<t->mFrameCount; j++)
	  {
		  new ( &t->mPose[j] ) MeshAnimPose;
	  }
      memcpy(t->mPose,src.mPose,sizeof(MeshAnimPose)*t->mFrameCount);
      a->mTracks[i] = t;
    }
    mMyAnimations.push_back(a);
  }

  virtual void        importSkeleton(const MeshSkeleton &skeleton)
  {
    MeshSkeleton *sk = (MeshSkeleton *)MI_ALLOC(sizeof(MeshSkeleton));
	new (sk ) MeshSkeleton;
    sk->mName = mStrings.Get( skeleton.mName ).Get();
    sk->mBoneCount = skeleton.mBoneCount;
    sk->mBones = 0;
    if ( sk->mBoneCount > 0 )
    {
      sk->mBones = (MeshBone *)MI_ALLOC(sizeof(MeshBone)*sk->mBoneCount);
	  for (MiI32 j=0; j<sk->mBoneCount; j++)
	  {
		  new ( &sk->mBones[j])MeshBone;
	  }
      MeshBone *dest = sk->mBones;
      const MeshBone *src = skeleton.mBones;
      for (MiU32 i=0; i<(MiU32)sk->mBoneCount; i++)
      {
        *dest = *src;
        dest->mName = mStrings.Get(src->mName).Get();
        src++;
        dest++;
      }
    }
    mMySkeletons.push_back(sk);
  }

  virtual void        importRawTexture(const char * /*textureName*/,const MiU8 * /*pixels*/,MiU32 /*wid*/,MiU32 /*hit*/)
  {
//    assert(0); // not yet implemented
  }

  virtual void        importMeshInstance(const char *meshName,const MiF32 pos[3],const MiF32 rotation[4],const MiF32 scale[3])
  {
    StringRef ref = mStrings.Get(meshName);
    MeshInstance m;
    m.mMeshName = ref.Get();
    m.mPosition[0] = pos[0];
    m.mPosition[1] = pos[1];
    m.mPosition[2] = pos[2];
    m.mRotation[0] = rotation[0];
    m.mRotation[1] = rotation[1];
    m.mRotation[2] = rotation[2];
    m.mRotation[3] = rotation[3];
    m.mScale[0] = scale[0];
    m.mScale[1] = scale[1];
    m.mScale[2] = scale[2];
    mMyMeshInstances.push_back(m);
  }

  virtual void importCollisionRepresentation(const MeshCollisionRepresentation &mr) // the name of a new collision representation.
  {
	  MyMeshCollisionRepresentation *mmr = MI_NEW(MyMeshCollisionRepresentation)(mr,mStrings);
	  mCollisionReps.push_back(mmr);
  }

  virtual void importSimpleJoint(const MeshSimpleJoint &joint)
  {
	  MeshSimpleJoint msj = joint;
	  bool first;
	  msj.mName = mStrings.Get(joint.mName,first);
	  msj.mParent = mStrings.Get(joint.mParent,first);
	  mMyJoints.push_back(msj);
  }

  virtual void rotate(MiF32 rotX,MiF32 rotY,MiF32 rotZ)
  {
    MiF32 quat[4];
    fm_eulerToQuat(rotX*FM_DEG_TO_RAD,rotY*FM_DEG_TO_RAD,rotZ*FM_DEG_TO_RAD,quat);

    {
      MeshSkeletonVector::iterator i;
      for (i=mMySkeletons.begin(); i!=mMySkeletons.end(); ++i)
      {
        MeshSkeleton *ms = (*i);
        for (MiI32 j=0; j<ms->mBoneCount; j++)
        {
          MeshBone &b = ms->mBones[j];
          if ( b.mParentIndex == -1 )
          {
            fm_quatRotate(quat,b.mPosition,b.mPosition);
            fm_multiplyQuat(quat,b.mOrientation,b.mOrientation);
          }
        }
      }
    }

    {
      MyMeshVector::iterator i;
      for (i=mMyMeshes.begin(); i!=mMyMeshes.end(); ++i)
      {
        MyMesh *m = (*i);
        MiU32 vcount = (MiU32)m->mVertexPool.GetSize();
        if ( vcount > 0 )
        {
          MeshVertex *vb = m->mVertexPool.GetBuffer();
          for (MiU32 j=0; j<vcount; j++)
          {
            fm_quatRotate(quat,vb->mPos,vb->mPos);
            vb++;
          }
        }
      }
    }

    {
      MeshAnimationVector::iterator i;
      for (i=mMyAnimations.begin(); i!=mMyAnimations.end(); ++i)
      {
        MeshAnimation *ma = (*i);
        for (MiI32 j=0; j<ma->mTrackCount && j <1; j++)
        {
          MeshAnimTrack *t = ma->mTracks[j];
          for (MiI32 k=0; k<t->mFrameCount; k++)
          {
            MeshAnimPose &p = t->mPose[k];
            fm_quatRotate(quat,p.mPos,p.mPos);
            fm_multiplyQuat(quat,p.mQuat,p.mQuat);
          }
        }
      }
    }
  }

  virtual void scale(MiF32 s)
  {
    {
      MeshSkeletonVector::iterator i;
      for (i=mMySkeletons.begin(); i!=mMySkeletons.end(); ++i)
      {
        MeshSkeleton *ms = (*i);
        for (MiI32 j=0; j<ms->mBoneCount; j++)
        {
          MeshBone &b = ms->mBones[j];
          b.mPosition[0]*=s;
          b.mPosition[1]*=s;
          b.mPosition[2]*=s;
        }
      }
    }

    {
      MyMeshVector::iterator i;
      for (i=mMyMeshes.begin(); i!=mMyMeshes.end(); ++i)
      {
        MyMesh *m = (*i);
        MiU32 vcount = (MiU32)m->mVertexPool.GetSize();
        if ( vcount > 0 )
        {
          MeshVertex *vb = m->mVertexPool.GetBuffer();
          for (MiU32 j=0; j<vcount; j++)
          {
            vb->mPos[0]*=s;
            vb->mPos[1]*=s;
            vb->mPos[2]*=s;
            vb++;
          }
        }
      }
    }

    {
      MeshAnimationVector::iterator i;
      for (i=mMyAnimations.begin(); i!=mMyAnimations.end(); ++i)
      {
        MeshAnimation *ma = (*i);
        for (MiI32 j=0; j<ma->mTrackCount; j++)
        {
          MeshAnimTrack *t = ma->mTracks[j];
          for (MiI32 k=0; k<t->mFrameCount; k++)
          {
            MeshAnimPose &p = t->mPose[k];
            p.mPos[0]*=s;
            p.mPos[1]*=s;
            p.mPos[2]*=s;
          }
        }
      }
    }
  }

  virtual MiI32 getSerializeFrame(void)
  {
    return gSerializeFrame;
  }

  const char *getMaterialName(const char *matName)
  {
    const char *ret = matName;
    return ret;
  }
	
  bool getImportState() const { return mImportState; }

  const char * streamString(MiIOStream &stream)
  {
	  const char *ret = "";
	  const char *str;
	  stream >> str;
	  if ( str )
	  {
		  bool first;
		  ret = mStrings.Get(str,first);
	  }
	return ret;
  }

  void deserialize(MiIOStream &stream,MeshAABB &m)
  {
	  stream >> m.mMin[0];
	  stream >> m.mMin[1];
	  stream >> m.mMin[2];
	  stream >> m.mMax[0];
	  stream >> m.mMax[1];
	  stream >> m.mMax[2];
  }

  void deserialize(MiIOStream &stream,MeshRawTexture &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mWidth;
	  stream >> m.mHeight;
	  stream >> m.mBPP;
	  MiU32 dataLen = 0;
	  stream >> dataLen;
	  if ( dataLen )
	  {
		  m.mData = (MiU8 *)MI_ALLOC(dataLen);
		  stream.getStream().read(m.mData,dataLen);
	  }
  }

  void deserialize(MiIOStream & /*stream*/,MeshTetra & /*mrt*/)
  {
	  MI_ALWAYS_ASSERT(); // Not yet implemented
  }

  void deserialize(MiIOStream &stream,MeshBone &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mParentIndex;
	  stream >> m.mPosition[0];
	  stream >> m.mPosition[1];
	  stream >> m.mPosition[2];
	  stream >> m.mOrientation[0];
	  stream >> m.mOrientation[1];
	  stream >> m.mOrientation[2];
	  stream >> m.mOrientation[3];
	  stream >> m.mScale[0];
	  stream >> m.mScale[1];
	  stream >> m.mScale[2];
  }

  void deserialize(MiIOStream &stream,MeshSkeleton &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mBoneCount;
	  if ( m.mBoneCount )
	  {
		m.mBones = (MeshBone *)MI_ALLOC(sizeof(MeshBone)*m.mBoneCount);
		for (MiI32 i=0; i<m.mBoneCount; i++)
		{
			new ( &m.mBones[i] ) MeshBone;
		}
	  }
	  for (MiI32 i=0; i<m.mBoneCount; i++)
	  {
		  MeshBone &b = m.mBones[i];
		  deserialize(stream,b);
	  }
  }

  void deserialize(MiIOStream &stream,MeshAnimPose &m)
  {
	  stream >> m.mPos[0];
	  stream >> m.mPos[1];
	  stream >> m.mPos[2];
	  stream >> m.mQuat[0];
	  stream >> m.mQuat[1];
	  stream >> m.mQuat[2];
	  stream >> m.mQuat[3];
	  stream >> m.mScale[0];
	  stream >> m.mScale[1];
	  stream >> m.mScale[2];
  }

  void deserialize(MiIOStream &stream,MeshAnimTrack &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mFrameCount;
	  stream >> m.mDuration;
	  stream >> m.mDtime;
	  if ( m.mFrameCount )
	  {
		  m.mPose = (MeshAnimPose *)MI_ALLOC(sizeof(MeshAnimPose)*m.mFrameCount);
		  for (MiI32 i=0; i<m.mFrameCount; i++)
		  {
			  new ( &m.mPose[i] ) MeshAnimPose;
		  }
	  }
	  for (MiI32 i=0; i<m.mFrameCount; i++)
	  {
		  MeshAnimPose &p = m.mPose[i];
		  deserialize(stream,p);
	  }
  }

  void deserialize(MiIOStream &stream,MeshAnimation &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mTrackCount;
	  stream >> m.mFrameCount;
	  stream >> m.mDuration;
	  stream >> m.mDtime;
	  if ( m.mTrackCount )
	  {
		  m.mTracks = (MeshAnimTrack **)MI_ALLOC(sizeof(MeshAnimTrack *)*m.mTrackCount);
	  }
	  for (MiI32 i=0; i<m.mTrackCount; i++)
	  {
		  MeshAnimTrack *t = (MeshAnimTrack *)MI_ALLOC(sizeof(MeshAnimTrack));
		  deserialize(stream,*t);
		  m.mTracks[i] = t;
	  }
  }

  void deserialize(MiIOStream &stream,MeshMaterial &m)
  {
	  m.mName = streamString(stream);
	  m.mMetaData = streamString(stream);
  }

  void deserialize(MiIOStream &stream,MeshUserData &m)
  {
	  m.mUserKey = streamString(stream);
	  m.mUserValue = streamString(stream);
  }

  void deserialize(MiIOStream &stream,MeshUserBinaryData &m)
  {
	  m.mName = streamString(stream);
	  stream >> m.mUserLen;
	  if ( m.mUserLen )
	  {
		  m.mUserData = (MiU8 *)MI_ALLOC(m.mUserLen);
		  stream.getStream().read(m.mUserData,m.mUserLen);
	  }
  }

  void deserialize(MiIOStream &stream,SubMesh &m)
  {
	  m.mMaterialName = streamString(stream);
	  deserialize(stream,m.mAABB);
	  stream >> m.mVertexFlags;
	  stream >> m.mTriCount;
	  if ( m.mTriCount )
	  {
		  m.mIndices = (MiU32 *)MI_ALLOC(sizeof(MiU32)*m.mTriCount*3);
	  }
	  for (MiU32 i=0; i<m.mTriCount*3; i++)
	  {
		  stream >> m.mIndices[i];
	  }
  }

  void deserialize(MiIOStream &stream,MeshVertex &m,MiU32 vertexFlags)
  {

	  if ( vertexFlags & MIVF_POSITION )
	  {
		  stream >> m.mPos[0];
		  stream >> m.mPos[1];
		  stream >> m.mPos[2];
	  }
	  if ( vertexFlags & MIVF_NORMAL )
	  {
		  stream >> m.mNormal[0];
		  stream >> m.mNormal[1];
		  stream >> m.mNormal[2];
	  }
	  if ( vertexFlags & MIVF_COLOR )
	  {
		  stream >> m.mColor;
	  }
	  if ( vertexFlags & MIVF_TEXEL1 )
	  {
		  stream >> m.mTexel1[0];
		  stream >> m.mTexel1[1];
	  }
	  if ( vertexFlags & MIVF_TEXEL2 )
	  {
		  stream >> m.mTexel2[0];
		  stream >> m.mTexel2[1];
	  }
	  if ( vertexFlags & MIVF_TEXEL3 )
	  {
		  stream >> m.mTexel3[0];
		  stream >> m.mTexel3[1];
	  }
	  if ( vertexFlags & MIVF_TEXEL4 )
	  {
		  stream >> m.mTexel4[0];
		  stream >> m.mTexel4[1];
	  }
	  if ( vertexFlags & MIVF_TANGENT )
	  {
		  stream >> m.mTangent[0];
		  stream >> m.mTangent[1];
		  stream >> m.mTangent[2];
	  }
	  if ( vertexFlags & MIVF_BINORMAL )
	  {
		  stream >> m.mBiNormal[0];
		  stream >> m.mBiNormal[1];
		  stream >> m.mBiNormal[2];
	  }
	  if ( vertexFlags & MIVF_BONE_WEIGHTING  )
	  {
		  stream >> m.mBone[0];
		  stream >> m.mBone[1];
		  stream >> m.mBone[2];
		  stream >> m.mBone[3];
		  stream >> m.mWeight[0];
		  stream >> m.mWeight[1];
		  stream >> m.mWeight[2];
		  stream >> m.mWeight[3];
	  }
	  if ( vertexFlags & MIVF_RADIUS )
	  {
		  stream >> m.mRadius;
	  }
	  if ( vertexFlags & MIVF_INTERP1 )
	  {
		  stream >> m.mInterp1[0];
		  stream >> m.mInterp1[1];
		  stream >> m.mInterp1[2];
		  stream >> m.mInterp1[3];
	  }
	  if ( vertexFlags & MIVF_INTERP2 )
	  {
		  stream >> m.mInterp2[0];
		  stream >> m.mInterp2[1];
		  stream >> m.mInterp2[2];
		  stream >> m.mInterp2[3];
	  }
	  if ( vertexFlags & MIVF_INTERP3 )
	  {
		  stream >> m.mInterp3[0];
		  stream >> m.mInterp3[1];
		  stream >> m.mInterp3[2];
		  stream >> m.mInterp3[3];
	  }
	  if ( vertexFlags & MIVF_INTERP4 )
	  {
		  stream >> m.mInterp4[0];
		  stream >> m.mInterp4[1];
		  stream >> m.mInterp4[2];
		  stream >> m.mInterp4[3];
	  }
	  if ( vertexFlags & MIVF_INTERP5 )
	  {
		  stream >> m.mInterp5[0];
		  stream >> m.mInterp5[1];
		  stream >> m.mInterp5[2];
		  stream >> m.mInterp5[3];
	  }
	  if ( vertexFlags & MIVF_INTERP6 )
	  {
		  stream >> m.mInterp6[0];
		  stream >> m.mInterp6[1];
		  stream >> m.mInterp6[2];
		  stream >> m.mInterp6[3];
	  }
	  if ( vertexFlags & MIVF_INTERP7 )
	  {
		  stream >> m.mInterp7[0];
		  stream >> m.mInterp7[1];
		  stream >> m.mInterp7[2];
		  stream >> m.mInterp7[3];
	  }
	  if ( vertexFlags & MIVF_INTERP8 )
	  {
		  stream >> m.mInterp8[0];
		  stream >> m.mInterp8[1];
		  stream >> m.mInterp8[2];
		  stream >> m.mInterp8[3];
	  }


  }


  void deserialize(MiIOStream &stream,Mesh &m)
  {
	  m.mName = streamString(stream);
	  m.mSkeletonName = streamString(stream);
	  deserialize(stream,m.mAABB);
	  stream >> m.mSubMeshCount;
	  if ( m.mSubMeshCount )
	  {
		  m.mSubMeshes = (SubMesh **)MI_ALLOC(sizeof(SubMesh *)*m.mSubMeshCount);
	  }
	  for (MiU32 i=0; i<m.mSubMeshCount; i++)
	  {
		  SubMesh *s = (SubMesh *)MI_ALLOC(sizeof(SubMesh));
		  new (s ) SubMesh;
		  deserialize(stream,*s);
		  m.mSubMeshes[i] = s;
	  }
	  stream >> m.mVertexFlags;
	  stream >> m.mVertexCount;
	  if ( m.mVertexCount )
	  {
		  m.mVertices = (MeshVertex *)MI_ALLOC(sizeof(MeshVertex)*m.mVertexCount);
	  }
	  for (MiU32 i=0; i<m.mVertexCount; i++)
	  {
		  MeshVertex &v = m.mVertices[i];
		  new ( &v ) MeshVertex;
		  deserialize(stream,v,m.mVertexFlags);
	  }
  }

  void deserialize(MiIOStream &stream,MeshInstance &m)
  {
	  m.mMeshName = streamString(stream);
	  stream >> m.mPosition[0];
	  stream >> m.mPosition[1];
	  stream >> m.mPosition[2];
	  stream >> m.mRotation[0];
	  stream >> m.mRotation[1];
	  stream >> m.mRotation[2];
	  stream >> m.mRotation[3];
	  stream >> m.mScale[0];
	  stream >> m.mScale[1];
	  stream >> m.mScale[2];
  }

  void deserialize(MiIOStream & /*stream*/,const MeshCollisionRepresentation & /*m*/)
  {
	  MI_ALWAYS_ASSERT(); // need to implement
  }



	bool deserializeBinary(const void *data,MiU32 dlen,const char * /*options*/,MeshImportApplicationResource * /*appResource*/)
	{
		bool ret = false;

		MiMemoryBuffer mb(data,dlen);
		mb.setEndianMode(MiFileBuf::ENDIAN_BIG);
		MiIOStream stream(mb,dlen);

		MiU32 version = 0;
		stream >> version;
		const char *tag;
		stream >> tag;

		if ( version == MESH_BINARY_VERSION && tag && strcmp(tag,"EZMESH") == 0  )
		{
			mAssetName = streamString(stream);
			mAssetInfo = streamString(stream);
			stream >> mMeshSystemVersion;
			stream >> mAssetVersion;
			deserialize(stream,mAABB);
			stream >> mTextureCount;
			for (MiU32 i=0; i<mTextureCount; i++)
			{
				MeshRawTexture *t = (MeshRawTexture *)MI_ALLOC(sizeof(MeshRawTexture));
				new (t ) MeshRawTexture;
				deserialize(stream,*t);
				mMyRawTextures.push_back(t);
			}
			stream >> mTetraMeshCount;
			for (MiU32 i=0; i< mTetraMeshCount; i++)
			{
				MeshTetra *m = (MeshTetra *)MI_ALLOC(sizeof(MeshTetra));
				new ( m ) MeshTetra;
				deserialize(stream,*m);
				mMyMeshTextra.push_back(m);
			}
			stream >> mSkeletonCount;
			for (MiU32 i=0; i< mSkeletonCount; i++)
			{
				MeshSkeleton *m =  (MeshSkeleton *)MI_ALLOC(sizeof(MeshSkeleton));
				new ( m ) MeshSkeleton;
				deserialize(stream,*m);
				mMySkeletons.push_back(m);
			}
			stream >> mAnimationCount;
			for (MiU32 i=0; i<mAnimationCount; i++)
			{
				MeshAnimation *m = (MeshAnimation *)MI_ALLOC(sizeof(MeshAnimation));
				new ( m ) MeshAnimation;
				deserialize(stream,*m);
				mMyAnimations.push_back(m);
			}
			stream >> mMaterialCount; 
			if ( mMaterialCount )
			{
				mMaterials =  (MeshMaterial *)MI_ALLOC(sizeof(MeshMaterial)*mMaterialCount);
			}
			for (MiU32 i=0; i<mMaterialCount; i++)
			{
				MeshMaterial &m = mMaterials[i];
				new ( &m ) MeshMaterial;
				deserialize(stream,m);
			}
			stream >> mUserDataCount;
			for (MiU32 i=0; i<mUserDataCount; i++)
			{
				MeshUserData *m = (MeshUserData *)MI_ALLOC(sizeof(MeshUserData));
				new ( m ) MeshUserData;
				deserialize(stream,*m);
				mMyMeshUserData.push_back(m);
			}
			stream >> mUserBinaryDataCount;
			for (MiU32 i=0; i<mUserBinaryDataCount; i++)
			{
				MeshUserBinaryData *m = (MeshUserBinaryData *)MI_ALLOC(sizeof(MeshUserBinaryData));
				new ( m ) MeshUserBinaryData;
				deserialize(stream,*m);
				mMyMeshUserBinaryData.push_back(m);
			}
			stream >> mMeshCount;
			for (MiU32 i=0; i<mMeshCount; i++)
			{
				Mesh *m = (Mesh *)MI_ALLOC(sizeof(Mesh));
				new ( m ) Mesh;
				deserialize(stream,*m);
				mFlatMeshes.push_back(m);
			}
			stream >> mMeshInstanceCount;
			if ( mMeshInstanceCount )
			{
				mMeshInstances = (MeshInstance *)MI_ALLOC(sizeof(MeshInstance)*mMeshInstanceCount);
			}
			for (MiU32 i=0; i<mMeshInstanceCount; i++)
			{
				MeshInstance &m = mMeshInstances[i];
				new ( &m ) MeshInstance;
				deserialize(stream,m);
			}
			stream >> mMeshCollisionCount;
			for (MiU32 i=0; i<mMeshCollisionCount; i++)
			{
				MeshCollisionRepresentation *m = (MeshCollisionRepresentation *)MI_ALLOC(sizeof(MeshCollisionRepresentation));
				new ( m ) MeshCollisionRepresentation;
				deserialize(stream,*m);
				mCollisionReps.push_back(m);
			}
			stream >> mPlane[0];
			stream >> mPlane[1];
			stream >> mPlane[2];
			stream >> mPlane[3];
			ret = true;
		}
		return ret;
	}

	void fixupNamedPointers(void)
	{
		for (MiU32 i=0; i<mMeshCount; i++)
		{
			Mesh *m = mMeshes[i];
			// patch named pointeres for submesh materials
			for (MiU32 j=0; j<m->mSubMeshCount; j++)
			{
				SubMesh *s = m->mSubMeshes[j];
				for (MiU32 j=0; j<mMaterialCount; j++)
				{
					MeshMaterial &mm = mMaterials[j];
					if ( strcmp(s->mMaterialName,mm.mName) == 0 )
					{
						s->mMaterial = &mm;
						break;
					}
				}
			}
			// patch named pointers for skeletons
			for (MiU32 j=0; j<mSkeletonCount; j++)
			{
				MeshSkeleton *s = mSkeletons[j];
				if ( strcmp(s->mName,m->mSkeletonName) == 0 )
				{
					m->mSkeleton = s;
					break;
				}
			}
		}
	}

	virtual void	importMeshPairCollision(MeshPairCollision &mpc)
	{
		MeshPairCollision m;
		m.mBody1 = mStrings.Get(mpc.mBody1);
		m.mBody2 = mStrings.Get(mpc.mBody2);
		mCollisionPairs.push_back(m);
	}


private:
	bool							mDeserializeBinary;
	StringDict                          mStrings;
	StringRefMap                        mMaterialMap;
	MeshMaterialVector                  mMyMaterials;
	MeshInstanceVector                  mMyMeshInstances;
	MyMesh                             *mCurrentMesh;

	MyMeshVector                        mMyMeshes;
	MeshVector							mFlatMeshes;

	MeshAnimationVector                 mMyAnimations;
	MeshSkeletonVector                  mMySkeletons;
	MeshCollisionRepresentationVector   mCollisionReps;
	MeshSimpleJointVector				mMyJoints;
	MeshImportApplicationResource      *mAppResource;

	MeshRawTextureVector				  mMyRawTextures;
	MeshTetraVector				      mMyMeshTextra;
	MeshUserDataVector					mMyMeshUserData;
	MeshUserBinaryDataVector				mMyMeshUserBinaryData;
	MeshPairCollisionVector			mCollisionPairs;

	bool								  mImportState;
};

MeshBuilder * createMeshBuilder(const char *meshName,const void *data,MiU32 dlen,MeshImporter *mi,const char *options,MeshImportApplicationResource *appResource)
{
  MyMeshBuilder *b = (MyMeshBuilder *)MI_ALLOC(sizeof(MyMeshBuilder));
  new ( b ) MyMeshBuilder(meshName,data,dlen,mi,options,appResource);
  bool state = b->getImportState();
  if (!state)
	  return 0;
  return static_cast< MeshBuilder *>(b);
}

MeshBuilder * createMeshBuilder(MeshImportApplicationResource *appResource)
{
	MyMeshBuilder *b = (MyMeshBuilder *)MI_ALLOC(sizeof(MyMeshBuilder));
	new ( b ) MyMeshBuilder(appResource);
	bool state = b->getImportState();
	if (!state)
		return 0;
	return static_cast< MeshBuilder *>(b);
}

void          releaseMeshBuilder(MeshBuilder *m)
{
  MyMeshBuilder *b = static_cast< MyMeshBuilder *>(m);
  b->~MyMeshBuilder();
  MI_FREE(b);
}

}; // end of namesapace
