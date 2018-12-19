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



#ifndef __APEXSDKHELPERS_H__
#define __APEXSDKHELPERS_H__


#include "PsArray.h"
#include "PsSort.h"
#include "ApexString.h"
#include "ApexSDKIntl.h"
#include "ApexResource.h"
#include "ResourceProviderIntl.h"
#include "PxMat33.h"
#include "PsMutex.h"

namespace physx
{
	namespace pvdsdk
	{
		class PvdDataStream;
	}
}
namespace nvidia
{
namespace apex
{

enum StreamPointerToken
{
	SPT_INVALID_PTR,
	SPT_VALID_PTR
};


/*
	Resource list - holds a list of ApexResourceInterface objects, for quick removal
 */
class ResourceList: public nvidia::UserAllocated
{
	physx::Array<ApexResourceInterface*>	mArray;
	nvidia::ReadWriteLock					mRWLock;

#ifndef WITHOUT_PVD
	// for PVD
	const void*						mOwner;
	ApexSimpleString				mListName;
	ApexSimpleString				mEntryName;
#endif

public:

	ResourceList()
#ifndef WITHOUT_PVD
	 : mOwner(NULL) 
#endif
	{}
	~ResourceList();

	void			clear(); // explicitely free children

	void			add(ApexResourceInterface& resource);
	void			remove(uint32_t index);
	uint32_t			getSize() const
	{
		ScopedReadLock scopedLock(const_cast<nvidia::ReadWriteLock&>(mRWLock));
		return mArray.size();
	}
	ApexResourceInterface*	getResource(uint32_t index) const
	{
		ScopedReadLock scopedLock(const_cast<nvidia::ReadWriteLock&>(mRWLock));
		return mArray[index];
	}

	template<typename Predicate>
	void			sort(const Predicate& compare)
	{
		ScopedWriteLock scopedLock(mRWLock);
		uint32_t size = mArray.size();
		if (size > 0)
		{
			nvidia::sort(&mArray[0], size, compare);
		}

		for (uint32_t i = 0; i < size; ++i)
		{
			mArray[i]->setListIndex(*this, i);
		}
	}

#ifndef WITHOUT_PVD
	void setupForPvd(const void* owner, const char* listName, const char* entryName);
	void initPvdInstances(pvdsdk::PvdDataStream& pvdStream);
#endif
};


#ifndef M_SQRT1_2	//1/sqrt(2)
#define M_SQRT1_2 double(0.7071067811865475244008443621048490)
#endif


/*
	Creates a rotation matrix which rotates about the axisAngle vector.  The length of
	the axisAngle vector is the desired rotation angle.  In this approximation, however,
	this is only the case for small angles.  As the length of axisAngle grows (is no
	longer very much less than 1 radian) the approximation becomes worse.  As the length
	of axisAngle approaches infinity, the rotation angle approaches pi.  The exact
	relation is:

		rotation_angle = 2*atan( axisAngle.magnitude()/2 )

	One use for this construction is the rotation applied to mesh particle system particles.  With a
	decent frame rate, the rotation angle should be small, unless the particle is going
	very fast or has very small radius.  In that case, or if the frame rate is poor,
	the inaccuracy in this construction probably won't be noticed.

	Error: The rotation angle is accurate to:
		1%	up to 20 degrees
		10% up to 70 degrees
*/
PX_INLINE void approxAxisAngleToMat33(const PxVec3& axisAngle, PxMat33& rot)
{
	const float x = 0.5f * axisAngle.x;
	const float y = 0.5f * axisAngle.y;
	const float z = 0.5f * axisAngle.z;
	const float xx = x * x;
	const float yy = y * y;
	const float zz = z * z;
	const float xy = x * y;
	const float yz = y * z;
	const float zx = z * x;
	const float twoRecipNorm2 = 2.0f / (1.0f + xx + yy + zz);	// w = 1
	rot(0, 0) = 1.0f - twoRecipNorm2 * (yy + zz);
	rot(0, 1) = twoRecipNorm2 * (xy - z);
	rot(0, 2) = twoRecipNorm2 * (zx + y);
	rot(1, 0) = twoRecipNorm2 * (xy + z);
	rot(1, 1) = 1.0f - twoRecipNorm2 * (zz + xx);
	rot(1, 2) = twoRecipNorm2 * (yz - x);
	rot(2, 0) = twoRecipNorm2 * (zx - y);
	rot(2, 1) = twoRecipNorm2 * (yz + x);
	rot(2, 2) = 1.0f - twoRecipNorm2 * (xx + yy);
}


// stl hash
PX_INLINE uint32_t hash(const char* str, uint32_t len)
{
	uint32_t hash = 0;

	for (uint32_t i = 0; i < len; i++)
	{
		hash = 5 * hash + str[i];
	}

	return hash;
}

PX_INLINE uint32_t GetStamp(ApexSimpleString& name)
{
	return hash(name.c_str(), name.len());
}

#if 0
// these are poison
void writeStreamHeader(PxFileBuf& stream, ApexSimpleString& streamName, uint32_t versionStamp);
uint32_t readStreamHeader(const PxFileBuf& stream, ApexSimpleString& streamName);
#endif

PX_INLINE uint32_t MaxElementIndex(const PxVec3& v)
{
	const uint32_t m01 = (uint32_t)(v.y > v.x);
	const uint32_t m2 = (uint32_t)(v.z > v[m01]);
	return m2 << 1 | m01 >> m2;
}

PX_INLINE uint32_t MinElementIndex(const PxVec3& v)
{
	const uint32_t m01 = (uint32_t)(v.y < v.x);
	const uint32_t m2 = (uint32_t)(v.z < v[m01]);
	return m2 << 1 | m01 >> m2;
}

PX_INLINE uint32_t MaxAbsElementIndex(const PxVec3& v)
{
	const PxVec3 a(PxAbs(v.x), PxAbs(v.y), PxAbs(v.z));
	const uint32_t m01 = (uint32_t)(a.y > a.x);
	const uint32_t m2 = (uint32_t)(a.z > a[m01]);
	return m2 << 1 | m01 >> m2;
}

PX_INLINE uint32_t MinAbsElementIndex(const PxVec3& v)
{
	const PxVec3 a(PxAbs(v.x), PxAbs(v.y), PxAbs(v.z));
	const uint32_t m01 = (uint32_t)(a.y < a.x);
	const uint32_t m2 = (uint32_t)(a.z < a[m01]);
	return m2 << 1 | m01 >> m2;
}


/******************************************************************************
 * Helper functions for loading assets
 *****************************************************************************/
class ApexAssetHelper
{
public:
	static void* getAssetFromName(ApexSDKIntl*	sdk,
	                              const char*	authoringTypeName,
	                              const char*	assetName,
	                              ResID&		inOutResID,
	                              ResID		optionalPsID = INVALID_RESOURCE_ID);

	static void* getAssetFromNameList(ApexSDKIntl*	sdk,
	                                  const char* authoringTypeName,
	                                  physx::Array<AssetNameIDMapping*>& nameIdList,
	                                  const char* assetName,
	                                  ResID assetPsId = INVALID_RESOURCE_ID);

	static void* getIosAssetFromName(ApexSDKIntl*	sdk,
	                                 const char* iosTypeName,
	                                 const char* iosAssetName);

};

}
} // end namespace nvidia::apex

#endif	// __APEXSDKHELPERS_H__
