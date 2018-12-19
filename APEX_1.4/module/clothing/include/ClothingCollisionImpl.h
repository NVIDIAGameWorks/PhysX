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



#ifndef CLOTHING_COLLISION_IMPL_H
#define CLOTHING_COLLISION_IMPL_H

#include "ApexResource.h"
#include "ApexSDKHelpers.h"
#include "ClothingCollision.h"
#include "ApexRWLockable.h"

#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace clothing
{

class ClothingActorImpl;


class ClothingCollisionImpl : public ApexResource, public ApexResourceInterface, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingCollisionImpl(ResourceList& list, ClothingActorImpl& owner);

	/* ApexResourceInterface */
	virtual void release();

	uint32_t getListIndex() const
	{
		return m_listIndex;
	}
	void setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	void destroy();

	void setId(int32_t id)
	{
		mId = id;
	}
	int32_t getId() const
	{
		return mId;
	}

	virtual ClothingPlane* isPlane() { READ_ZONE(); return NULL;}
	virtual ClothingConvex* isConvex() { READ_ZONE(); return NULL;}
	virtual ClothingSphere* isSphere() { READ_ZONE(); return NULL;}
	virtual ClothingCapsule* isCapsule() { READ_ZONE(); return NULL;}
	virtual ClothingTriangleMesh* isTriangleMesh() { READ_ZONE(); return NULL;}

protected:

	ClothingActorImpl& mOwner;
	bool mInRelease;

	int32_t mId;

private:
	ClothingCollisionImpl& operator=(const ClothingCollisionImpl&);
};


/************************************************************************/
// ClothingPlaneImpl
/************************************************************************/
class ClothingPlaneImpl : public ClothingPlane, public ClothingCollisionImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingPlaneImpl(ResourceList& list, ClothingActorImpl& owner, const PxPlane& plane) :
		ClothingCollisionImpl(list, owner),
		mPlane(plane),
		mRefCount(0)
	{
	}

	virtual ClothingCollisionType::Enum getType() const
	{
		READ_ZONE();
		return ClothingCollisionType::Plane;
	}
	virtual ClothingPlane* isPlane() { return this; }
	virtual ClothingConvex* isConvex() { return NULL; }
	virtual ClothingSphere* isSphere() { return NULL; }
	virtual ClothingCapsule* isCapsule() { return NULL; }
	virtual ClothingTriangleMesh* isTriangleMesh() { return NULL; }

	virtual void release()
	{
		if (mRefCount > 0)
		{
			APEX_DEBUG_WARNING("Cannot release ClothingPlane that is referenced by a ClothingConvex. Release convex first.");
			return;
		}

		ClothingCollisionImpl::release();
	}

	virtual void setPlane(const PxPlane& plane);
	virtual PxPlane& getPlane()
	{
		return mPlane;
	}

	void incRefCount()
	{
		++mRefCount;
	}
	void decRefCount()
	{
		PX_ASSERT(mRefCount > 0);
		--mRefCount;
	}
	virtual uint32_t getRefCount() const
	{
		return (uint32_t)mRefCount;
	}

protected:
	PxPlane mPlane;

	int32_t mRefCount;
};



/************************************************************************/
// ClothingConvexImpl
/************************************************************************/
class ClothingConvexImpl : public ClothingConvex, public ClothingCollisionImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingConvexImpl(ResourceList& list, ClothingActorImpl& owner, ClothingPlane** planes, uint32_t numPlanes) :
		ClothingCollisionImpl(list, owner)
	{
		mPlanes.resize(numPlanes);
		for (uint32_t i = 0; i < numPlanes; ++i)
		{
			mPlanes[i] = DYNAMIC_CAST(ClothingPlaneImpl*)(planes[i]);
			mPlanes[i]->incRefCount();
		}
	}

	virtual ClothingCollisionType::Enum getType() const
	{
		return ClothingCollisionType::Convex;
	}
	virtual ClothingPlane* isPlane() { return NULL; }
	virtual ClothingConvex* isConvex() { return this; }
	virtual ClothingSphere* isSphere() { return NULL; }
	virtual ClothingCapsule* isCapsule() { return NULL; }
	virtual ClothingTriangleMesh* isTriangleMesh() { return NULL; }

	virtual void release()
	{
		for (uint32_t i = 0; i < mPlanes.size(); ++i)
		{
			mPlanes[i]->decRefCount();
		}

		ClothingCollisionImpl::release();
	}

	virtual void releaseWithPlanes()
	{
		for (uint32_t i = 0; i < mPlanes.size(); ++i)
		{
			mPlanes[i]->decRefCount();
			mPlanes[i]->release();
		}

		ClothingCollisionImpl::release();
	}

	virtual uint32_t getNumPlanes()
	{
		return mPlanes.size();
	}

	virtual ClothingPlane** getPlanes()
	{
		return (ClothingPlane**)&mPlanes[0];
	}

protected:
	Array<ClothingPlaneImpl*> mPlanes;
};




/************************************************************************/
// ClothingSphereImpl
/************************************************************************/
class ClothingSphereImpl : public ClothingSphere, public ClothingCollisionImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingSphereImpl(ResourceList& list, ClothingActorImpl& owner, const PxVec3& position, float radius) :
		ClothingCollisionImpl(list, owner),
		mPosition(position),
		mRadius(radius),
		mRefCount(0)
	{
	}

	virtual ClothingCollisionType::Enum getType() const
	{
		return ClothingCollisionType::Sphere;
	}
	virtual ClothingPlane* isPlane() { return NULL; }
	virtual ClothingConvex* isConvex() { return NULL; }
	virtual ClothingSphere* isSphere() { return this; }
	virtual ClothingCapsule* isCapsule() { return NULL; }
	virtual ClothingTriangleMesh* isTriangleMesh() { return NULL; }

	virtual void release()
	{
		if (mRefCount > 0)
		{
			APEX_DEBUG_WARNING("Cannot release ClothingSphere that is referenced by an ClothingCapsule. Release capsule first.");
			return;
		}

		ClothingCollisionImpl::release();
	}
	
	virtual void setPosition(const PxVec3& position);
	virtual const PxVec3& getPosition() const
	{
		return mPosition;
	}

	virtual void setRadius(float radius);
	virtual float getRadius() const
	{
		return mRadius;
	}

	void incRefCount()
	{
		++mRefCount;
	}
	void decRefCount()
	{
		PX_ASSERT(mRefCount > 0);
		--mRefCount;
	}
	virtual uint32_t getRefCount() const
	{
		return mRefCount;
	}

protected:
	PxVec3 mPosition;
	float mRadius;

	uint32_t mRefCount;
};




/************************************************************************/
// ClothingCapsuleImpl
/************************************************************************/
class ClothingCapsuleImpl : public ClothingCapsule, public ClothingCollisionImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingCapsuleImpl(ResourceList& list, ClothingActorImpl& owner, ClothingSphere& sphere1, ClothingSphere& sphere2) :
		ClothingCollisionImpl(list, owner)
	{
		mSpheres[0] = (DYNAMIC_CAST(ClothingSphereImpl*)(&sphere1));
		mSpheres[0]->incRefCount();

		mSpheres[1] = (DYNAMIC_CAST(ClothingSphereImpl*)(&sphere2));
		mSpheres[1]->incRefCount();
	}

	virtual ClothingCollisionType::Enum getType() const
	{
		return ClothingCollisionType::Capsule;
	}
	virtual ClothingPlane* isPlane() { return NULL; }
	virtual ClothingConvex* isConvex() { return NULL; }
	virtual ClothingSphere* isSphere() { return NULL; }
	virtual ClothingCapsule* isCapsule() { return this; }
	virtual ClothingTriangleMesh* isTriangleMesh() { return NULL; }

	virtual void release()
	{
		mSpheres[0]->decRefCount();
		mSpheres[1]->decRefCount();

		ClothingCollisionImpl::release();
	}

	virtual void releaseWithSpheres()
	{
		mSpheres[0]->decRefCount();
		mSpheres[1]->decRefCount();

		mSpheres[0]->release();
		mSpheres[1]->release();

		ClothingCollisionImpl::release();
	}

	virtual ClothingSphere** getSpheres()
	{
		return (ClothingSphere**)mSpheres;
	}

protected:
	ClothingSphereImpl* mSpheres[2];
};




/************************************************************************/
// ClothingTriangleMeshImpl
/************************************************************************/

struct ClothingTriangle
{
	PxVec3 v[3];
	uint32_t id;

	bool operator<(const ClothingTriangle& other) const
	{
		return id < other.id;
	}
};

class ClothingTriangleMeshImpl : public ClothingTriangleMesh, public ClothingCollisionImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingTriangleMeshImpl(ResourceList& list, ClothingActorImpl& owner) :
		ClothingCollisionImpl(list, owner),
		mPose(PxMat44(PxIdentity))
	{
	}

	virtual ClothingCollisionType::Enum getType() const
	{
		return ClothingCollisionType::TriangleMesh;
	}
	virtual ClothingPlane* isPlane() { return NULL; }
	virtual ClothingConvex* isConvex() { return NULL; }
	virtual ClothingSphere* isSphere() { return NULL; }
	virtual ClothingCapsule* isCapsule() { return NULL; }
	virtual ClothingTriangleMesh* isTriangleMesh() { return this; }

	virtual uint32_t lockTriangles(const uint32_t** ids, const PxVec3** triangles);
	virtual uint32_t lockTrianglesWrite(const uint32_t** ids, PxVec3** triangles);
	virtual void unlockTriangles();

	virtual void addTriangle(uint32_t id, const PxVec3& v0, const PxVec3& v1, const PxVec3& v2);
	virtual void addTriangles(const uint32_t* ids, const PxVec3* triangleVertices, uint32_t numTriangles);

	virtual void removeTriangle(uint32_t id);
	virtual void removeTriangles(const uint32_t* ids, uint32_t numTriangles);
	virtual void clearTriangles();

	virtual void release()
	{
		ClothingCollisionImpl::release();
	}

	virtual void setPose(PxMat44 pose);
	virtual const PxMat44& getPose() const
	{
		return mPose;
	}

	void update(const PxTransform& tm, const nvidia::Array<PxVec3>& allTrianglesOld, nvidia::Array<PxVec3>& allTrianglesOldTemp, nvidia::Array<PxVec3>& allTriangles);

protected:
	void sortAddAndRemoves();

	PxMat44 mPose;
	Array<PxVec3> mTriangles;
	Array<uint32_t> mIds;

	Array<uint32_t> mRemoved;
	Array<ClothingTriangle> mAddedTriangles;

	Mutex mLock;
};


}
} // namespace nvidia


#endif // CLOTHING_COLLISION_IMPL_H