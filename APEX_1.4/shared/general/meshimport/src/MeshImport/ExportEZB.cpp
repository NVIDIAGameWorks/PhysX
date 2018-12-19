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
#include "ExportEZB.h"
#include "MeshImport.h"
#include "MiFloatMath.h"
#include "MiMemoryBuffer.h"
#include "MiIOStream.h"

#pragma warning(disable:4100)

namespace mimp
{

static void serialize(MiIOStream &stream,const MeshAABB &aabb)
{
	stream << aabb.mMin[0];
	stream << aabb.mMin[1];
	stream << aabb.mMin[2];
	stream << aabb.mMax[0];
	stream << aabb.mMax[1];
	stream << aabb.mMax[2];
}

static void serialize(MiIOStream &stream,const MeshRawTexture &m)
{
	stream << m.mName;
	stream << m.mWidth;
	stream << m.mHeight;
	stream << m.mBPP;

	MiU32 dataLen = m.mData ? m.mWidth*m.mHeight*m.mBPP : 0;
	stream << dataLen;
	if ( dataLen )
	{
		stream.getStream().write(m.mData,dataLen);
	}
}

static void serialize(MiIOStream & /*stream*/,const MeshTetra & /*mrt*/)
{
	MI_ALWAYS_ASSERT(); // Not yet implemented
}

static void serialize(MiIOStream &stream,const MeshBone &m)
{
	stream << m.mName;
	stream << m.mParentIndex;
	stream << m.mPosition[0];
	stream << m.mPosition[1];
	stream << m.mPosition[2];
	stream << m.mOrientation[0];
	stream << m.mOrientation[1];
	stream << m.mOrientation[2];
	stream << m.mOrientation[3];
	stream << m.mScale[0];
	stream << m.mScale[1];
	stream << m.mScale[2];
}

static void serialize(MiIOStream &stream,const MeshSkeleton &m)
{
	stream << m.mName;
	stream << m.mBoneCount;
	for (MiI32 i=0; i<m.mBoneCount; i++)
	{
		const MeshBone &b = m.mBones[i];
		serialize(stream,b);
	}
}

static void serialize(MiIOStream &stream,const MeshAnimPose &m)
{
	stream << m.mPos[0];
	stream << m.mPos[1];
	stream << m.mPos[2];
	stream << m.mQuat[0];
	stream << m.mQuat[1];
	stream << m.mQuat[2];
	stream << m.mQuat[3];
	stream << m.mScale[0];
	stream << m.mScale[1];
	stream << m.mScale[2];
}

static void serialize(MiIOStream &stream,const MeshAnimTrack &m)
{
	stream << m.mName;
	stream << m.mFrameCount;
	stream << m.mDuration;
	stream << m.mDtime;
	for (MiI32 i=0; i<m.mFrameCount; i++)
	{
		const MeshAnimPose &p = m.mPose[i];
		serialize(stream,p);
	}
}

static void serialize(MiIOStream &stream,const MeshAnimation &m)
{
	stream << m.mName;
	stream << m.mTrackCount;
	stream << m.mFrameCount;
	stream << m.mDuration;
	stream << m.mDtime;
	for (MiI32 i=0; i<m.mTrackCount; i++)
	{
		const MeshAnimTrack &t = *m.mTracks[i];
		serialize(stream,t);
	}
}

static void serialize(MiIOStream &stream,const MeshMaterial &m)
{
	stream << m.mName;
	stream << m.mMetaData;
}

static void serialize(MiIOStream &stream,const MeshUserData &m)
{
	stream << m.mUserKey;
	stream << m.mUserValue;
}

static void serialize(MiIOStream &stream,const MeshUserBinaryData &m)
{
	stream << m.mName;
	stream << m.mUserLen;
	if ( m.mUserLen )
	{
		stream.getStream().write(m.mUserData,m.mUserLen);
	}
}

static void serialize(MiIOStream &stream,const SubMesh &m)
{
	stream << m.mMaterialName;
	serialize(stream,m.mAABB);
	stream << m.mVertexFlags;
	stream << m.mTriCount;
	for (MiU32 i=0; i<m.mTriCount*3; i++)
	{
		stream << m.mIndices[i];
	}
}

static void serialize(MiIOStream &stream,const MeshVertex &m,MiU32 vertexFlags)
{

	if ( vertexFlags & MIVF_POSITION )
	{
		stream << m.mPos[0];
		stream << m.mPos[1];
		stream << m.mPos[2];
	}
	if ( vertexFlags & MIVF_NORMAL )
	{
		stream << m.mNormal[0];
		stream << m.mNormal[1];
		stream << m.mNormal[2];
	}
	if ( vertexFlags & MIVF_COLOR )
	{
		stream << m.mColor;
	}
	if ( vertexFlags & MIVF_TEXEL1 )
	{
		stream << m.mTexel1[0];
		stream << m.mTexel1[1];
	}
	if ( vertexFlags & MIVF_TEXEL2 )
	{
		stream << m.mTexel2[0];
		stream << m.mTexel2[1];
	}
	if ( vertexFlags & MIVF_TEXEL3 )
	{
		stream << m.mTexel3[0];
		stream << m.mTexel3[1];
	}
	if ( vertexFlags & MIVF_TEXEL4 )
	{
		stream << m.mTexel4[0];
		stream << m.mTexel4[1];
	}
	if ( vertexFlags & MIVF_TANGENT )
	{
		stream << m.mTangent[0];
		stream << m.mTangent[1];
		stream << m.mTangent[2];
	}
	if ( vertexFlags & MIVF_BINORMAL )
	{
		stream << m.mBiNormal[0];
		stream << m.mBiNormal[1];
		stream << m.mBiNormal[2];
	}
	if ( vertexFlags & MIVF_BONE_WEIGHTING  )
	{
		stream << m.mBone[0];
		stream << m.mBone[1];
		stream << m.mBone[2];
		stream << m.mBone[3];
		stream << m.mWeight[0];
		stream << m.mWeight[1];
		stream << m.mWeight[2];
		stream << m.mWeight[3];
	}
	if ( vertexFlags & MIVF_RADIUS )
	{
		stream << m.mRadius;
	}
	if ( vertexFlags & MIVF_INTERP1 )
	{
		stream << m.mInterp1[0];
		stream << m.mInterp1[1];
		stream << m.mInterp1[2];
		stream << m.mInterp1[3];
	}
	if ( vertexFlags & MIVF_INTERP2 )
	{
		stream << m.mInterp2[0];
		stream << m.mInterp2[1];
		stream << m.mInterp2[2];
		stream << m.mInterp2[3];
	}
	if ( vertexFlags & MIVF_INTERP3 )
	{
		stream << m.mInterp3[0];
		stream << m.mInterp3[1];
		stream << m.mInterp3[2];
		stream << m.mInterp3[3];
	}
	if ( vertexFlags & MIVF_INTERP4 )
	{
		stream << m.mInterp4[0];
		stream << m.mInterp4[1];
		stream << m.mInterp4[2];
		stream << m.mInterp4[3];
	}
	if ( vertexFlags & MIVF_INTERP5 )
	{
		stream << m.mInterp5[0];
		stream << m.mInterp5[1];
		stream << m.mInterp5[2];
		stream << m.mInterp5[3];
	}
	if ( vertexFlags & MIVF_INTERP6 )
	{
		stream << m.mInterp6[0];
		stream << m.mInterp6[1];
		stream << m.mInterp6[2];
		stream << m.mInterp6[3];
	}
	if ( vertexFlags & MIVF_INTERP7 )
	{
		stream << m.mInterp7[0];
		stream << m.mInterp7[1];
		stream << m.mInterp7[2];
		stream << m.mInterp7[3];
	}
	if ( vertexFlags & MIVF_INTERP8 )
	{
		stream << m.mInterp8[0];
		stream << m.mInterp8[1];
		stream << m.mInterp8[2];
		stream << m.mInterp8[3];
	}


}


static void serialize(MiIOStream &stream,const Mesh &m)
{
	stream << m.mName;
	stream << m.mSkeletonName;
	serialize(stream,m.mAABB);
	stream << m.mSubMeshCount;
	for (MiU32 i=0; i<m.mSubMeshCount; i++)
	{
		const SubMesh &s = *m.mSubMeshes[i];
		serialize(stream,s);
	}
	stream << m.mVertexFlags;
	stream << m.mVertexCount;
	for (MiU32 i=0; i<m.mVertexCount; i++)
	{
		const MeshVertex &v = m.mVertices[i];
		serialize(stream,v,m.mVertexFlags);
	}
}

static void serialize(MiIOStream &stream,const MeshInstance &m)
{
	stream << m.mMeshName;
	stream << m.mPosition[0];
	stream << m.mPosition[1];
	stream << m.mPosition[2];
	stream << m.mRotation[0];
	stream << m.mRotation[1];
	stream << m.mRotation[2];
	stream << m.mRotation[3];
	stream << m.mScale[0];
	stream << m.mScale[1];
	stream << m.mScale[2];
}

static void serialize(MiIOStream & /*stream*/,const MeshCollisionRepresentation & /*m*/)
{
	MI_ALWAYS_ASSERT(); // need to implement
}


void *serializeEZB(const MeshSystem *ms,MiU32 &len)
{
	void * ret = NULL;
	len = 0;

	if ( ms )
	{
		MiMemoryBuffer mb;
		mb.setEndianMode(MiFileBuf::ENDIAN_BIG);
		MiIOStream stream(mb,0);
		MiU32 version = MESH_BINARY_VERSION;
		stream << version;
		stream << "EZMESH";
		stream << ms->mAssetName;
		stream << ms->mAssetInfo;
		stream << ms->mMeshSystemVersion;
		stream << ms->mAssetVersion;
		serialize(stream,ms->mAABB);
		stream << ms->mTextureCount;
		for (MiU32 i=0; i<ms->mTextureCount; i++)
		{
			const MeshRawTexture &t = *ms->mTextures[i];
			serialize(stream,t);
		}
		stream << ms->mTetraMeshCount;
		for (MiU32 i=0; i< ms->mTetraMeshCount; i++)
		{
			const MeshTetra &mt = *ms->mTetraMeshes[i];
			serialize(stream,mt);
		}
		stream << ms->mSkeletonCount;
		for (MiU32 i=0; i< ms->mSkeletonCount; i++)
		{
			const MeshSkeleton &m = *ms->mSkeletons[i];
			serialize(stream,m);
		}
		stream << ms->mAnimationCount;
		for (MiU32 i=0; i<ms->mAnimationCount; i++)
		{
			const MeshAnimation &a = *ms->mAnimations[i];
			serialize(stream,a);
		}
		stream << ms->mMaterialCount; 
		for (MiU32 i=0; i<ms->mMaterialCount; i++)
		{
			const MeshMaterial &m = ms->mMaterials[i];
			serialize(stream,m);
		}
		stream << ms->mUserDataCount;
		for (MiU32 i=0; i<ms->mUserDataCount; i++)
		{
			const MeshUserData &m = *ms->mUserData[i];
			serialize(stream,m);
		}
		stream << ms->mUserBinaryDataCount;
		for (MiU32 i=0; i<ms->mUserBinaryDataCount; i++)
		{
			const MeshUserBinaryData &m = *ms->mUserBinaryData[i];
			serialize(stream,m);
		}
		stream << ms->mMeshCount;
		for (MiU32 i=0; i<ms->mMeshCount; i++)
		{
			const Mesh &m = *ms->mMeshes[i];
			serialize(stream,m);
		}
		stream << ms->mMeshInstanceCount;
		for (MiU32 i=0; i<ms->mMeshInstanceCount; i++)
		{
			const MeshInstance &m = ms->mMeshInstances[i];
			serialize(stream,m);
		}
		stream << ms->mMeshCollisionCount;
		for (MiU32 i=0; i<ms->mMeshCollisionCount; i++)
		{
			const MeshCollisionRepresentation &m = *ms->mMeshCollisionRepresentations[i];
			serialize(stream,m);
		}
		stream << ms->mPlane[0];
		stream << ms->mPlane[1];
		stream << ms->mPlane[2];
		stream << ms->mPlane[3];

		ret = mb.getWriteBufferOwnership(len);
	}
	return ret;
}


};// end of namespace

