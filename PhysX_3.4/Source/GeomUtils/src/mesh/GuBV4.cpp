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

#include "foundation/PxMemory.h"
#include "GuBV4.h"
#include "GuSerialize.h"
#include "CmUtils.h"
#include "PsUtilities.h"

using namespace physx;
using namespace Gu;

#define DELETEARRAY(x)		if (x) { delete []x;	x = NULL; }

SourceMesh::SourceMesh()
{
	reset();
}

SourceMesh::~SourceMesh()
{
	PX_FREE_AND_RESET(mRemap);
}

void SourceMesh::reset()
{
	mNbVerts		= 0;
	mVerts			= NULL;
	mNbTris			= 0;
	mTriangles32	= NULL;
	mTriangles16	= NULL;
	mRemap			= NULL;
}

void SourceMesh::operator=(SourceMesh& v)
{
	mNbVerts		= v.mNbVerts;
	mVerts			= v.mVerts;
	mNbTris			= v.mNbTris;
	mTriangles32	= v.mTriangles32;
	mTriangles16	= v.mTriangles16;
	mRemap			= v.mRemap;
	v.reset();
}

void SourceMesh::remapTopology(const PxU32* order)
{
	if(!mNbTris)
		return;

	if(mTriangles32)
	{
		IndTri32* newTopo = PX_NEW(IndTri32)[mNbTris];
		for(PxU32 i=0;i<mNbTris;i++)
			newTopo[i] = mTriangles32[order[i]];

		PxMemCopy(mTriangles32, newTopo, sizeof(IndTri32)*mNbTris);
		DELETEARRAY(newTopo);
	}
	else
	{
		PX_ASSERT(mTriangles16);
		IndTri16* newTopo = PX_NEW(IndTri16)[mNbTris];
		for(PxU32 i=0;i<mNbTris;i++)
			newTopo[i] = mTriangles16[order[i]];

		PxMemCopy(mTriangles16, newTopo, sizeof(IndTri16)*mNbTris);
		DELETEARRAY(newTopo);
	}

	{
		PxU32* newMap = reinterpret_cast<PxU32*>(PX_ALLOC(sizeof(PxU32)*mNbTris, "OPC2"));
		for(PxU32 i=0;i<mNbTris;i++)
			newMap[i] = mRemap ? mRemap[order[i]] : order[i];

		PX_FREE_AND_RESET(mRemap);
		mRemap = newMap;
	}
}

bool SourceMesh::isValid() const
{
	if(!mNbTris || !mNbVerts)			return false;
	if(!mVerts)							return false;
	if(!mTriangles32 && !mTriangles16)	return false;
	return true;
}

/////

BV4Tree::BV4Tree(SourceMesh* meshInterface, const PxBounds3& localBounds)
{
	reset();
	init(meshInterface, localBounds);
}

BV4Tree::BV4Tree()
{
	reset();
}

void BV4Tree::release()
{
	if(!mUserAllocated)
	{
#ifdef GU_BV4_USE_SLABS
		PX_DELETE_AND_RESET(mNodes);
#else
		DELETEARRAY(mNodes);
#endif
	}

	mNodes = NULL;
	mNbNodes = 0;
}

BV4Tree::~BV4Tree()
{
	release();
}

void BV4Tree::reset()
{
	mMeshInterface		= NULL;
	mNbNodes			= 0;
	mNodes				= NULL;
	mInitData			= 0;
#ifdef GU_BV4_QUANTIZED_TREE
	mCenterOrMinCoeff	= PxVec3(0.0f);
	mExtentsOrMaxCoeff	= PxVec3(0.0f);
#endif
	mUserAllocated		= false;
}

void BV4Tree::operator=(BV4Tree& v)
{
	mMeshInterface		= v.mMeshInterface;
	mLocalBounds		= v.mLocalBounds;
	mNbNodes			= v.mNbNodes;
	mNodes				= v.mNodes;
	mInitData			= v.mInitData;
#ifdef GU_BV4_QUANTIZED_TREE
	mCenterOrMinCoeff	= v.mCenterOrMinCoeff;
	mExtentsOrMaxCoeff	= v.mExtentsOrMaxCoeff;
#endif
	mUserAllocated		= v.mUserAllocated;
	v.reset();
}

bool BV4Tree::init(SourceMesh* meshInterface, const PxBounds3& localBounds)
{
	mMeshInterface	= meshInterface;
	mLocalBounds.init(localBounds);
	return true;
}

// PX_SERIALIZATION
BV4Tree::BV4Tree(const PxEMPTY) : mLocalBounds(PxEmpty)
{
	mUserAllocated = true;
}

void BV4Tree::exportExtraData(PxSerializationContext& stream)
{
	if(mNbNodes)
	{
		stream.alignData(16);
		stream.writeData(mNodes, mNbNodes*sizeof(BVDataPacked));
	}
}

void BV4Tree::importExtraData(PxDeserializationContext& context)
{
	if(mNbNodes)
	{
		context.alignExtraData(16);
		mNodes = context.readExtraData<BVDataPacked>(mNbNodes);
	}
}
//~PX_SERIALIZATION

#ifdef GU_BV4_QUANTIZED_TREE
	static const PxU32 BVDataPackedNb = sizeof(BVDataPacked)/sizeof(PxU16);
	PX_COMPILE_TIME_ASSERT(BVDataPackedNb * sizeof(PxU16) == sizeof(BVDataPacked));
#else
	static const PxU32 BVDataPackedNb = sizeof(BVDataPacked)/sizeof(float);
	PX_COMPILE_TIME_ASSERT(BVDataPackedNb * sizeof(float) == sizeof(BVDataPacked));
#endif

bool BV4Tree::load(PxInputStream& stream, bool mismatch_)
{
	PX_ASSERT(!mUserAllocated);

	release();

	PxI8 a, b, c, d;
	readChunk(a, b, c, d, stream);
	if(a!='B' || b!='V' || c!='4' || d!=' ')
		return false;

	bool mismatch;
	PxU32 fileVersion;
	if(!readBigEndianVersionNumber(stream, mismatch_, fileVersion, mismatch))
		return false;

	readFloatBuffer(&mLocalBounds.mCenter.x, 3, mismatch, stream);
	mLocalBounds.mExtentsMagnitude = readFloat(mismatch, stream);

	mInitData = readDword(mismatch, stream);

#ifdef GU_BV4_QUANTIZED_TREE
	readFloatBuffer(&mCenterOrMinCoeff.x, 3, mismatch, stream);
	readFloatBuffer(&mExtentsOrMaxCoeff.x, 3, mismatch, stream);
#endif
	const PxU32 nbNodes = readDword(mismatch, stream);
	mNbNodes = nbNodes;

	if(nbNodes)
	{
#ifdef GU_BV4_USE_SLABS
		BVDataPacked* nodes = reinterpret_cast<BVDataPacked*>(PX_ALLOC(sizeof(BVDataPacked)*nbNodes, "BV4 nodes"));	// PT: PX_NEW breaks alignment here
#else
		BVDataPacked* nodes = PX_NEW(BVDataPacked)[nbNodes];
#endif
		mNodes = nodes;
		Cm::markSerializedMem(nodes, sizeof(BVDataPacked)*nbNodes);

		if(1)
		{
#ifdef GU_BV4_QUANTIZED_TREE
			readWordBuffer(&nodes[0].mAABB.mData[0].mExtents, BVDataPackedNb * nbNodes, false, stream);
#else
			readFloatBuffer(&nodes[0].mAABB.mCenter.x, BVDataPackedNb * nbNodes, false, stream);
#endif
			if(mismatch)
			{
				for(PxU32 i=0;i<nbNodes;i++)
				{
					BVDataPacked& node = nodes[i];
					for(PxU32 j=0;j<3;j++)
					{
#ifdef GU_BV4_QUANTIZED_TREE
						flip(node.mAABB.mData[j].mExtents);
						flip(node.mAABB.mData[j].mCenter);
#else
						flip(node.mAABB.mExtents[j]);
						flip(node.mAABB.mCenter[j]);
#endif
					}
					flip(node.mData);
				}
			}


		}
		else
		{
			// PT: initial slower code is like this:
			for(PxU32 i=0;i<nbNodes;i++)
			{
				BVDataPacked& node = nodes[i];
#ifdef GU_BV4_QUANTIZED_TREE
				readWordBuffer(&node.mAABB.mData[0].mExtents, 6, mismatch, stream);
#else
				readFloatBuffer(&node.mAABB.mCenter.x, 6, mismatch, stream);
#endif
				node.mData = readDword(mismatch, stream);
			}
		}
	}
	else mNodes = NULL;

	return true;
}
