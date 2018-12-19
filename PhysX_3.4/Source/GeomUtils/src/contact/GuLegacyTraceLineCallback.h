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


#ifndef GU_CONTACTTRACESEGMENTCALLBACK_H
#define GU_CONTACTTRACESEGMENTCALLBACK_H

#include "CmMatrix34.h"
#include "GuGeometryUnion.h"

#include "GuHeightFieldUtil.h"
#include "CmRenderOutput.h"
#include "GuContactBuffer.h"

namespace physx
{
namespace Gu
{
#define DISTANCE_BASED_TEST

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GuContactTraceSegmentCallback
{
	PxVec3				mLine;
	ContactBuffer&	mContactBuffer;
	Cm::Matrix34		mTransform;
	PxReal				mContactDistance;
	PxU32				mPrevTriangleIndex; // currently only used to indicate first callback
//	Cm::RenderOutput&	mRO;

	PX_INLINE GuContactTraceSegmentCallback(
		const PxVec3& line, Gu::ContactBuffer& contactBuffer,
		Cm::Matrix34 transform, PxReal contactDistance
		/*, Cm::RenderOutput& ro*/)
	: mLine(line), mContactBuffer(contactBuffer), mTransform(transform),
	mContactDistance(contactDistance), mPrevTriangleIndex(0xFFFFffff)//, mRO(ro)
	{
	}

	bool onEvent(PxU32 , PxU32* )
	{
		return true;
	}

	PX_INLINE bool faceHit(const Gu::HeightFieldUtil& /*hfUtil*/, const PxVec3& /*hitPoint*/, PxU32 /*triangleIndex*/, PxReal, PxReal) { return true; }

	// x,z is the point of projected face entry intercept in hf coords, rayHeight is at that same point
	PX_INLINE bool underFaceHit(
		const Gu::HeightFieldUtil& hfUtil, const PxVec3& triangleNormal,
		const PxVec3& crossedEdge, PxF32 x, PxF32 z, PxF32 rayHeight, PxU32 triangleIndex)
	{
		if (mPrevTriangleIndex == 0xFFFFffff) // we only record under-edge contacts so we need at least 2 face hits to have the edge
		{
			mPrevTriangleIndex = triangleIndex;
			//mPrevTriangleNormal = hfUtil.getTriangleNormal(triangleIndex);
			return true;
		}

		const Gu::HeightField& hf = hfUtil.getHeightField();
		PxF32 y = hfUtil.getHeightAtShapePoint(x, z); // TODO: optmization opportunity - this can be derived cheaply inside traceSegment
		PxF32 dy = rayHeight - y;

		if (!hf.isDeltaHeightInsideExtent(dy, mContactDistance))
			return true;

		// add contact
		PxVec3 n = crossedEdge.cross(mLine);
		if (n.y < 0) // Make sure cross product is facing correctly before clipping
			n = -n;

		if (n.y < 0) // degenerate case
			return true;

		const PxReal ll = n.magnitudeSquared();
		if (ll > 0) // normalize
			n *= PxRecipSqrt(ll);
		else // degenerate case
			return true; 

		// Scale delta height so it becomes the "penetration" along the normal
		dy *= n.y;
		if (hf.getThicknessFast() > 0)
		{
			n = -n;
			dy = -dy;
		}

		// compute the contact point
		const PxVec3 point(x, rayHeight, z);
		//mRO << PxVec3(1,0,0) << Gu::Debug::convertToPxMat44(mTransform)
		//	<< Cm::RenderOutput::LINES << point << point + triangleNormal;
#ifdef DISTANCE_BASED_TEST
		mContactBuffer.contact(
			mTransform.transform(point), mTransform.rotate(triangleNormal), dy, triangleIndex);
#else
		// add gContactDistance to compensate for fact that we don't support dist based contacts in box/convex-hf!
		// See comment at start of those functs.
		mContactBuffer.contact(
			mTransform.transform(point), mTransform.rotate(triangleNormal),
			dy + mContactDistance, PXC_CONTACT_NO_FACE_INDEX, triangleIndex);
#endif
		mPrevTriangleIndex = triangleIndex;
		//mPrevTriangleNormal = triangleNormal;
		return true;
	}

private:
	GuContactTraceSegmentCallback& operator=(const GuContactTraceSegmentCallback&);
};

class GuContactHeightfieldTraceSegmentHelper
{
	PX_NOCOPY(GuContactHeightfieldTraceSegmentHelper)
public:
	GuContactHeightfieldTraceSegmentHelper(const HeightFieldTraceUtil& hfUtil) : mHfUtil(hfUtil)
	{
		mHfUtil.computeLocalBounds(mLocalBounds);
	}

	PX_INLINE void traceSegment(const PxVec3& aP0, const PxVec3& aP1, GuContactTraceSegmentCallback* aCallback) const
	{
		mHfUtil.traceSegment<GuContactTraceSegmentCallback, true, false>(aP0, aP1 - aP0, 1.0f, aCallback, mLocalBounds, true, NULL);
	}

private:
	const HeightFieldTraceUtil&	mHfUtil;
	PxBounds3					mLocalBounds;
};

}//Gu
}//physx

#endif
