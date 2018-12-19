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

#ifndef PT_HEIGHT_FIELD_AABB_TEST_H
#define PT_HEIGHT_FIELD_AABB_TEST_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

//----------------------------------------------------------------------------//

#include "GuHeightField.h"
#include "GuHeightFieldData.h"
#include "GuHeightFieldUtil.h"
#include "PsUtilities.h"

namespace physx
{

namespace Pt
{

//----------------------------------------------------------------------------//

/**
Can be used for querying an AABB against a heightfield, without copying triangles to a temporary buffer.
An iterator can be created to walk the triangles which intersect the AABB and have not a hole material assigned.
This isn't really optimized yet.
*/
class HeightFieldAabbTest
{
  public:
	HeightFieldAabbTest(const PxBounds3& localBounds, const Gu::HeightFieldUtil& hfUtil)
	: mHfUtil(hfUtil), mIsEmpty(false)
	{
		const PxHeightFieldGeometry& hfGeom = mHfUtil.getHeightFieldGeometry();

		PxVec3 minimum = localBounds.minimum;
		PxVec3 maximum = localBounds.maximum;
		minimum = hfUtil.shape2hfp(minimum);
		maximum = hfUtil.shape2hfp(maximum);

		// if (heightField.getRowScale() < 0)
		if(hfGeom.rowScale < 0)
			Ps::swap(minimum.x, maximum.x);

		// if (heightField.getColumnScale() < 0)
		if(hfGeom.columnScale < 0)
			Ps::swap(minimum.z, maximum.z);

		// early exit for aabb does not overlap in XZ plane
		// DO NOT MOVE: since rowScale / columnScale may be negative this has to be done after scaling the bounds
		// if ((minimum.x > (heightField.getNbRowsFast()-1)) ||
		if((minimum.x > (mHfUtil.getHeightField().getNbRowsFast() - 1)) ||
		   //(minimum.z > (heightField.getNbColumnsFast()-1)) ||
		   (minimum.z > (mHfUtil.getHeightField().getNbColumnsFast() - 1)) || (maximum.x < 0) || (maximum.z < 0))
		{
			mIsEmpty = true;
			return;
		}

		mMinRow = mHfUtil.getHeightField().getMinRow(minimum.x);
		mMaxRow = mHfUtil.getHeightField().getMaxRow(maximum.x);
		mMinColumn = mHfUtil.getHeightField().getMinColumn(minimum.z);
		mMaxColumn = mHfUtil.getHeightField().getMaxColumn(maximum.z);

		if(mMinRow == mMaxRow || mMinColumn == mMaxColumn)
		{
			mIsEmpty = true;
			return;
		}

		mMiny = minimum.y;
		mMaxy = maximum.y;

		// Check if thickness / vertical extent is negative or positive. Set the triangle vertex indices
		// such that the collision triangles of the heightfield have the correct orientation, i.e., the correct normal
		// -
		// If the row and column scale have different signs, the orientation of the collision triangle vertices
		// need to be swapped
		mSwapVertIdx12 = ((mHfUtil.getHeightField().getThicknessFast() > 0.0f) !=
		                  Ps::differentSign(hfGeom.rowScale, hfGeom.columnScale));
	}

	//----------------------------------------------------------------------------//

	class Iterator
	{

	  public:
		bool operator!=(const Iterator& it) const
		{
			return (it.mTri != mTri) || (it.mOffset != mOffset);
		}

		//----------------------------------------------------------------------------//

		Iterator& operator++()
		{
			bool isec = (mTri == 1) || mTest.intersectsSegment(mOffset);
			PX_ASSERT(!(mTri == 1) || mTest.intersectsSegment(mOffset));

			PxU32 endOffset = mTest.getMaxOffset();
			while(mOffset < endOffset)
			{
				PX_ASSERT(mColumn < mTest.mMaxColumn);
				PX_ASSERT(mRow < mTest.mMaxRow);
				PX_ASSERT(mColumn >= mTest.mMinColumn);
				PX_ASSERT(mRow >= mTest.mMinRow);

				if(mTri == 0 && isec)
				{
					mTri++;
					if(mTest.isHole(mTri, mOffset))
						continue;

					return *this;
				}

				mTri = 0;
				mColumn++;
				mOffset++;

				if(mColumn == mTest.mMaxColumn)
				{
					mRow++;
					mOffset +=
					    (mTest.mHfUtil.getHeightField().getNbColumnsFast() - (mTest.mMaxColumn - mTest.mMinColumn));

					if(mRow == mTest.mMaxRow)
					{
						mOffset += (mTest.mMaxColumn - mTest.mMinColumn);
						continue;
					}
					mColumn = mTest.mMinColumn;
				}

				isec = mTest.intersectsSegment(mOffset);
				if(!isec || mTest.isHole(mTri, mOffset))
					continue;

				return *this;
			}
			PX_ASSERT(mOffset == endOffset);
			return *this;
		}

		//----------------------------------------------------------------------------//

		PX_INLINE void getTriangleVertices(PxVec3* triangle) const
		{
			mTest.getTriangleVertices(triangle, *this);
		}

		//----------------------------------------------------------------------------//

	  private:
		Iterator& operator=(const Iterator&);

		Iterator(PxU32 row, PxU32 column, const HeightFieldAabbTest& test) : mRow(row), mColumn(column), mTest(test)
		{
			mTri = 0;
			mOffset = mRow * mTest.mHfUtil.getHeightField().getNbColumnsFast() + mColumn;
		}

		//----------------------------------------------------------------------------//

		bool isValid()
		{
			return !mTest.isHole(mTri, mOffset) && mTest.intersectsSegment(mOffset);
		}

		//----------------------------------------------------------------------------//

		PxU32 mRow;
		PxU32 mColumn;
		PxU32 mTri;
		PxU32 mOffset;
		const HeightFieldAabbTest& mTest;

		friend class HeightFieldAabbTest;
	};

	//----------------------------------------------------------------------------//

	Iterator end() const
	{
		if(mIsEmpty)
			return Iterator(0, 0, *this);

		return Iterator(mMaxRow, mMaxColumn, *this);
	}

	//----------------------------------------------------------------------------//

	Iterator begin() const
	{
		if(mIsEmpty)
			return Iterator(0, 0, *this);

		Iterator itBegin(mMinRow, mMinColumn, *this);
		if(itBegin != end() && !itBegin.isValid())
			++itBegin;

		return itBegin;
	}

  private:
	HeightFieldAabbTest& operator=(const HeightFieldAabbTest&);

	PxU32 getMinOffset() const
	{
		return mMinRow * mHfUtil.getHeightField().getNbColumnsFast() + mMinColumn;
	}

	//----------------------------------------------------------------------------//

	PxU32 getMaxOffset() const
	{
		return mMaxRow * mHfUtil.getHeightField().getNbColumnsFast() + mMaxColumn;
	}

	//----------------------------------------------------------------------------//

	bool isHole(PxU32 triangleIndex, PxU32 offset) const
	{
		return mHfUtil.getHeightField().getTriangleMaterial((offset << 1) + triangleIndex) ==
		       PxHeightFieldMaterial::eHOLE;
	}

	//----------------------------------------------------------------------------//

	bool intersectsSegment(PxU32 offset) const
	{
		// should we cache this?
		PxReal h0 = mHfUtil.getHeightField().getHeight(offset);
		PxReal h1 = mHfUtil.getHeightField().getHeight(offset + 1);
		PxReal h2 = mHfUtil.getHeightField().getHeight(offset + mHfUtil.getHeightField().getNbColumnsFast());
		PxReal h3 = mHfUtil.getHeightField().getHeight(offset + mHfUtil.getHeightField().getNbColumnsFast() + 1);

		// Optimization: Could store the two left height field cell vertices and thus avoid some comparisons here
		//               (if the bounds covers more than one height field cell)
		return (!((mMaxy < h0 && mMaxy < h1 && mMaxy < h2 && mMaxy < h3) ||
		          (mMiny > h0 && mMiny > h1 && mMiny > h2 && mMiny > h3)));
	}

	//----------------------------------------------------------------------------//

	void getTriangleVertices(PxVec3* triangleVertices, const Iterator& iterator) const
	{
		PX_ASSERT(iterator.mOffset != getMaxOffset());
		PX_ASSERT(!isHole(iterator.mTri, iterator.mOffset));

		PxU32 triangleIndex = (iterator.mOffset << 1) + iterator.mTri;
		PxU32 vertIdx1 = PxU32(mSwapVertIdx12 ? 2 : 1);
		PxU32 vertIdx2 = PxU32(mSwapVertIdx12 ? 1 : 2);

		mHfUtil.getHeightField().getTriangleVertices(triangleIndex, iterator.mRow, iterator.mColumn, triangleVertices[0],
		                                             triangleVertices[vertIdx1], triangleVertices[vertIdx2]);

		triangleVertices[0] = mHfUtil.hf2shapep(triangleVertices[0]);
		triangleVertices[1] = mHfUtil.hf2shapep(triangleVertices[1]);
		triangleVertices[2] = mHfUtil.hf2shapep(triangleVertices[2]);
	}

	//----------------------------------------------------------------------------//

	const Gu::HeightFieldUtil& mHfUtil;
	bool mIsEmpty;

	PxU32 mMinRow;
	PxU32 mMaxRow;
	PxU32 mMinColumn;
	PxU32 mMaxColumn;
	PxReal mMiny;
	PxReal mMaxy;
	bool mSwapVertIdx12;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_HEIGHT_FIELD_AABB_TEST_H
