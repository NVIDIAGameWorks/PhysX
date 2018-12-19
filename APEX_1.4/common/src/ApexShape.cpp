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



#include "ApexShape.h"
#include "RenderDebugInterface.h"

#include "PsMathUtils.h"

namespace nvidia
{
namespace apex
{

bool isBoundsEmpty(PxBounds3 bounds)
{
	return bounds.minimum.x >= bounds.maximum.x || bounds.minimum.y >= bounds.maximum.y || bounds.minimum.z >= bounds.maximum.z;
}


ApexSphereShape::ApexSphereShape()
{
	mRadius = 0.5;
	mTransform4x4 = PxMat44(PxIdentity);
	mOldTransform4x4 = PxMat44(PxIdentity);
	calculateAABB();
}

void ApexSphereShape::setRadius(float radius)
{
	mRadius = radius;
	calculateAABB();
};

void ApexSphereShape::setPose(PxMat44 pose)
{
	mOldTransform4x4 = mTransform4x4;
	mTransform4x4 = pose;
	calculateAABB();
};

void ApexSphereShape::calculateAABB()
{
	//find extreme points of the (transformed by mPose and mTranslation) sphere and construct an AABB from it
	PxVec3 points[6];
	points[0] = PxVec3( mRadius, 0.0f, 0.0f);
	points[1] = PxVec3(-mRadius, 0.0f, 0.0f);
	points[2] = PxVec3(0.0f,  mRadius, 0.0f);
	points[3] = PxVec3(0.0f, -mRadius, 0.0f);
	points[4] = PxVec3(0.0f, 0.0f,  mRadius);
	points[5] = PxVec3(0.0f, 0.0f, -mRadius);

	PxVec3 maxBounds(0.0f), minBounds(0.0f);
	for (int i = 0; i < 6; i++)
	{
		PxVec3 tempPoint = mTransform4x4.transform(points[i]);
		if (i == 0)
		{
			minBounds = maxBounds = tempPoint;
		}
		else
		{
			if (tempPoint.x < minBounds.x)
			{
				minBounds.x = tempPoint.x;
			}
			if (tempPoint.x > maxBounds.x)
			{
				maxBounds.x = tempPoint.x;
			}
			if (tempPoint.y < minBounds.y)
			{
				minBounds.y = tempPoint.y;
			}
			if (tempPoint.y > maxBounds.y)
			{
				maxBounds.y = tempPoint.y;
			}
			if (tempPoint.z < minBounds.z)
			{
				minBounds.z = tempPoint.z;
			}
			if (tempPoint.z > maxBounds.z)
			{
				maxBounds.z = tempPoint.z;
			}
		}
	}
	mBounds = PxBounds3(minBounds, maxBounds);
}

bool ApexSphereShape::intersectAgainstAABB(PxBounds3 bounds)
{
	if (!isBoundsEmpty(mBounds) && !isBoundsEmpty(bounds)) //if the bounds have some real volume then check bounds intersection
	{
		return bounds.intersects(mBounds);
	}
	else
	{
		return false;    //if the bounds had no volume then return false
	}
}

void ApexSphereShape::visualize(RenderDebugInterface* renderer) const
{
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(renderer)->getPoseTyped();
	RENDER_DEBUG_IFACE(renderer)->setIdentityPose();
	RENDER_DEBUG_IFACE(renderer)->debugSphere(mTransform4x4.getPosition(), 2);
	RENDER_DEBUG_IFACE(renderer)->setPose(savedPose);
}

ApexCapsuleShape::ApexCapsuleShape()
{
	mRadius = 1.0f;
	mHeight = 1.0f;
	mTransform4x4 = PxMat44(PxIdentity);
	mOldTransform4x4 = PxMat44(PxIdentity);
	calculateAABB();
}

void ApexCapsuleShape::setDimensions(float height, float radius)
{
	mRadius = radius;
	mHeight = height;
	calculateAABB();
};

void ApexCapsuleShape::setPose(PxMat44 pose)
{
	mOldTransform4x4 = mTransform4x4;
	mTransform4x4 = pose;
	calculateAABB();
};

void ApexCapsuleShape::calculateAABB()
{
	//find extreme points of the (transformed by mPose and mTranslation) capsule and construct an AABB from it
	PxVec3 points[6];
	points[0] = PxVec3(mRadius, 0.0f, 0.0f);
	points[1] = PxVec3(-mRadius, 0.0f, 0.0f);
	points[2] = PxVec3(0.0f, mRadius + mHeight / 2.0f, 0.0f);
	points[3] = PxVec3(0.0f, -(mRadius + mHeight / 2.0f), 0.0f);
	points[4] = PxVec3(0.0f, 0.0f, mRadius);
	points[5] = PxVec3(0.0f, 0.0f, -mRadius);

	PxVec3 maxBounds(0.0f), minBounds(0.0f);
	for (int i = 0; i < 6; i++)
	{
		PxVec3 tempPoint = mTransform4x4.transform(points[i]);
		if (i == 0)
		{
			minBounds = maxBounds = tempPoint;
		}
		else
		{
			if (tempPoint.x < minBounds.x)
			{
				minBounds.x = tempPoint.x;
			}
			if (tempPoint.x > maxBounds.x)
			{
				maxBounds.x = tempPoint.x;
			}
			if (tempPoint.y < minBounds.y)
			{
				minBounds.y = tempPoint.y;
			}
			if (tempPoint.y > maxBounds.y)
			{
				maxBounds.y = tempPoint.y;
			}
			if (tempPoint.z < minBounds.z)
			{
				minBounds.z = tempPoint.z;
			}
			if (tempPoint.z > maxBounds.z)
			{
				maxBounds.z = tempPoint.z;
			}
		}
	}
	mBounds = PxBounds3(minBounds, maxBounds);
}

bool ApexCapsuleShape::intersectAgainstAABB(PxBounds3 bounds)
{
	if (!isBoundsEmpty(mBounds) && !isBoundsEmpty(bounds)) //if the bounds have some real volume then check bounds intersection
	{
		return bounds.intersects(mBounds);
	}
	else
	{
		return false;    //if the bounds had no volume then return false
	}
}

void ApexCapsuleShape::visualize(RenderDebugInterface* renderer) const
{
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(renderer)->getPoseTyped();
	RENDER_DEBUG_IFACE(renderer)->setPose(&mTransform4x4.column0.x);
	RENDER_DEBUG_IFACE(renderer)->debugCapsule(mRadius, mHeight);
	RENDER_DEBUG_IFACE(renderer)->setPose(savedPose);
}

ApexBoxShape::ApexBoxShape()
{
	mSize = PxVec3(1, 1, 1);
	mTransform4x4 = PxMat44(PxIdentity);
	mOldTransform4x4 = PxMat44(PxIdentity);
	calculateAABB();
}

void ApexBoxShape::setSize(PxVec3 size)
{
	mSize = size;
	calculateAABB();
};

void ApexBoxShape::setPose(PxMat44 pose)
{
	mOldTransform4x4 = mTransform4x4;
	mTransform4x4 = pose;
	calculateAABB();
};

void ApexBoxShape::calculateAABB()
{
	//find extreme points of the (transformed by mPose and mTranslation) box and construct an AABB from it
	PxVec3 points[8];
	PxVec3 sizeHalf = PxVec3(mSize.x / 2.0f, mSize.y / 2.0f, mSize.z / 2.0f);
	points[0] = PxVec3(sizeHalf.x, sizeHalf.y, sizeHalf.z);
	points[1] = PxVec3(-sizeHalf.x, sizeHalf.y, sizeHalf.z);
	points[2] = PxVec3(sizeHalf.x, -sizeHalf.y, sizeHalf.z);
	points[3] = PxVec3(sizeHalf.x, sizeHalf.y, -sizeHalf.z);
	points[4] = PxVec3(sizeHalf.x, -sizeHalf.y, -sizeHalf.z);
	points[5] = PxVec3(-sizeHalf.x, -sizeHalf.y, sizeHalf.z);
	points[6] = PxVec3(-sizeHalf.x, sizeHalf.y, -sizeHalf.z);
	points[7] = PxVec3(-sizeHalf.x, -sizeHalf.y, -sizeHalf.z);

	PxVec3 maxBounds(0.0f), minBounds(0.0f);
	for (int i = 0; i < 8; i++)
	{
		PxVec3 tempPoint = mTransform4x4.transform(points[i]);
		if (i == 0)
		{
			minBounds = maxBounds = tempPoint;
		}
		else
		{
			if (tempPoint.x < minBounds.x)
			{
				minBounds.x = tempPoint.x;
			}
			if (tempPoint.x > maxBounds.x)
			{
				maxBounds.x = tempPoint.x;
			}
			if (tempPoint.y < minBounds.y)
			{
				minBounds.y = tempPoint.y;
			}
			if (tempPoint.y > maxBounds.y)
			{
				maxBounds.y = tempPoint.y;
			}
			if (tempPoint.z < minBounds.z)
			{
				minBounds.z = tempPoint.z;
			}
			if (tempPoint.z > maxBounds.z)
			{
				maxBounds.z = tempPoint.z;
			}
		}
	}
	mBounds = PxBounds3(minBounds, maxBounds);
}

bool ApexBoxShape::intersectAgainstAABB(PxBounds3 bounds)
{
	if (!isBoundsEmpty(mBounds) && !isBoundsEmpty(bounds)) //if the bounds have some real volume then check bounds intersection
	{
		return bounds.intersects(mBounds);
	}
	else
	{
		return false;    //if the bounds had no volume then return false
	}
}

void ApexBoxShape::visualize(RenderDebugInterface* renderer) const
{
	PxVec3 halfSize = mSize / 2;
	const PxBounds3 bounds(-halfSize, halfSize);
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(renderer)->getPoseTyped();
	RENDER_DEBUG_IFACE(renderer)->setPose(&mTransform4x4.column0.x);
	RENDER_DEBUG_IFACE(renderer)->debugBound(bounds);
	RENDER_DEBUG_IFACE(renderer)->setPose(savedPose);
}

bool ApexHalfSpaceShape::isPointInside(PxVec3 pos)
{
	PxVec3 vecToPos = pos - mOrigin;
	if (mNormal.dot(vecToPos) < 0.0f)
	{
		return true;
	}
	return false;
}

//if any of the corners of the bounds is inside then the bounds is inside
bool ApexHalfSpaceShape::intersectAgainstAABB(PxBounds3 bounds)
{
	PxVec3 center = bounds.getCenter();
	PxVec3 sizeHalf = bounds.getDimensions() / 2.0f;

	if (isPointInside(PxVec3(center.x + sizeHalf.x, center.y + sizeHalf.y, center.z + sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x - sizeHalf.x, center.y + sizeHalf.y, center.z + sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x + sizeHalf.x, center.y - sizeHalf.y, center.z + sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x + sizeHalf.x, center.y + sizeHalf.y, center.z - sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x + sizeHalf.x, center.y - sizeHalf.y, center.z - sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x - sizeHalf.x, center.y - sizeHalf.y, center.z + sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x - sizeHalf.x, center.y + sizeHalf.y, center.z - sizeHalf.z)))
	{
		return true;
	}
	if (isPointInside(PxVec3(center.x - sizeHalf.x, center.y - sizeHalf.y, center.z - sizeHalf.z)))
	{
		return true;
	}

	return false;

}

ApexHalfSpaceShape::ApexHalfSpaceShape()
{
	mOrigin = mPreviousOrigin = mNormal = mPreviousNormal = PxVec3(0, 0, 0);
}

void ApexHalfSpaceShape::setOriginAndNormal(PxVec3 origin, PxVec3 normal)
{
	mPreviousOrigin = mOrigin;
	mOrigin = origin;
	mPreviousNormal = mNormal;
	mNormal = normal;
};

void ApexHalfSpaceShape::visualize(RenderDebugInterface* renderer) const
{
	float radius = 2 * mNormal.magnitude();
	const PxPlane plane(mOrigin, mNormal);
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(renderer)->getPoseTyped();
	RENDER_DEBUG_IFACE(renderer)->setIdentityPose();
	RENDER_DEBUG_IFACE(renderer)->debugPlane(plane, radius, radius);
	RENDER_DEBUG_IFACE(renderer)->debugRay(mOrigin, mOrigin + mNormal);
	RENDER_DEBUG_IFACE(renderer)->setPose(savedPose);
}

static PX_INLINE PxMat44 OriginAndNormalToPose(const PxVec3& origin, const PxVec3& normal)
{
	return PxMat44(physx::shdfnd::rotFrom2Vectors(PxVec3(0, 1, 0), normal), origin);
}

PxMat44 ApexHalfSpaceShape::getPose() const
{
	return OriginAndNormalToPose(mOrigin, mNormal);
}

PxMat44 ApexHalfSpaceShape::getPreviousPose() const
{
	return OriginAndNormalToPose(mPreviousOrigin, mPreviousNormal);
}

}
} // end namespace nvidia::apex

