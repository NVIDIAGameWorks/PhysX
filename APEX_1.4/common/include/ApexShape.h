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



#ifndef __APEX_SHAPE_H__
#define __APEX_SHAPE_H__

#include "Apex.h"
#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"
#include "Shape.h"

namespace nvidia
{
namespace apex
{

class ApexSphereShape : public SphereShape, public UserAllocated
{
private:
	float		mRadius;
	PxMat44		mTransform4x4;
	PxMat44		mOldTransform4x4;
	PxBounds3	mBounds;

	void calculateAABB();

public:

	ApexSphereShape();

	virtual ~ApexSphereShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexSphereShape* 	isSphereGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setRadius(float radius);

	float getRadius() const
	{
		return mRadius;
	};

	void setPose(PxMat44 pose);

	PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	void visualize(RenderDebugInterface* renderer) const;
};

//the capsule is oriented along the y axis by default and its total height is height+2*radius
class ApexCapsuleShape : public CapsuleShape, public UserAllocated
{
private:

	float mRadius;
	float mHeight;
	PxMat44 mTransform4x4;
	PxMat44 mOldTransform4x4;
	PxBounds3 mBounds;
	void calculateAABB();

public:

	ApexCapsuleShape();

	virtual ~ApexCapsuleShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexCapsuleShape* 	isCapsuleGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setDimensions(float height, float radius);

	void getDimensions(float& height, float& radius) const
	{
		radius = mRadius;
		height = mHeight;
	};

	void setPose(PxMat44 pose);

	PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	void visualize(RenderDebugInterface* renderer) const;
};

class ApexBoxShape : public BoxShape, public UserAllocated
{
private:

	PxVec3 mSize;
	PxMat44 mTransform4x4;
	PxMat44 mOldTransform4x4;
	PxBounds3 mBounds;
	void calculateAABB();

public:

	ApexBoxShape();

	virtual ~ApexBoxShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexBoxShape* 	isBoxGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setSize(PxVec3 size);

	void setPose(PxMat44 pose);

	PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	PxVec3 getSize() const
	{
		return mSize;
	}

	void visualize(RenderDebugInterface* renderer) const;
};

class ApexHalfSpaceShape : public HalfSpaceShape, public UserAllocated
{
private:
	PxVec3 mOrigin;
	PxVec3 mPreviousOrigin;
	PxVec3 mNormal;
	PxVec3 mPreviousNormal;
	bool isPointInside(PxVec3 pos);

public:
	ApexHalfSpaceShape();

	virtual ~ApexHalfSpaceShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexHalfSpaceShape* 	isHalfSpaceGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return PxBounds3(PxVec3(0), PxVec3(PX_MAX_F32));
	}

	void setOriginAndNormal(PxVec3 origin, PxVec3 normal);

	PxVec3 getNormal() const
	{
		return mNormal;
	};

	PxVec3 getPreviousNormal() const
	{
		return mPreviousNormal;
	};

	PxVec3 getOrigin() const
	{
		return mOrigin;
	};

	PxVec3 getPreviousOrigin() const
	{
		return mPreviousOrigin;
	};

	PxMat44 getPose() const;

	PxMat44 getPreviousPose() const;

	void visualize(RenderDebugInterface* renderer) const;

	virtual void setPose(PxMat44 pose)
	{
		PX_UNUSED(pose);
		//dummy
	}
};


}
} // end namespace nvidia::apex

#endif
