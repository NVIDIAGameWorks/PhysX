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


#ifndef PX_PHYSICS_NP_VOLUMECACHE
#define PX_PHYSICS_NP_VOLUMECACHE

#include "PxVolumeCache.h"
#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "PsArray.h"

namespace physx
{

	struct MultiQueryInput;

namespace Sq { class SceneQueryManager; }


// internal implementation for PxVolumeCache
class NpVolumeCache : public PxVolumeCache, public Ps::UserAllocated
{
public:
									NpVolumeCache(Sq::SceneQueryManager* sqm, PxU32 maxNbStatic, PxU32 maxNbDynamic);
	virtual							~NpVolumeCache();
	virtual			bool			isValid() const;
					bool			isValid(PxU32 isDynamic) const;

	virtual			FillStatus		fill(const PxGeometry& cacheVolume, const PxTransform& pose);
					FillStatus		fillInternal(PxU32 isDynamic, const PxOverlapHit* buffer = NULL, PxI32 count = 0);

	virtual			bool			getCacheVolume(PxGeometryHolder& resultVolume, PxTransform& resultPose);
	virtual			PxI32			getNbCachedShapes();

	virtual			void			invalidate();
	virtual			void			release();

	virtual			void			forEach(Iterator& iter); 

	virtual			void			setMaxNbStaticShapes(PxU32 maxCount);
	virtual			PxU32			getMaxNbStaticShapes();
	virtual			void			setMaxNbDynamicShapes(PxU32 maxCount);
	virtual			PxU32			getMaxNbDynamicShapes();

	template<typename HitType>
					bool			multiQuery(
										const MultiQueryInput& multiInput, // type specific input data
										PxHitCallback<HitType>& hitCall, PxHitFlags hitFlags,
										const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall,
										PxF32 inflation = 0.0f) const;

	virtual			bool			raycast(
										const PxVec3& origin, const PxVec3& unitDir, const PxReal distance,
										PxRaycastCallback& hitCall, PxHitFlags hitFlags,
										const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall) const;

	virtual			bool			sweep(
										const PxGeometry& geometry, const PxTransform& pose, const PxVec3& unitDir, const PxReal distance,
										PxSweepCallback& hitCall, PxHitFlags hitFlags,
										const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall,
										const PxReal inflation) const;

	virtual			bool			overlap(
										const PxGeometry& geometry, const PxTransform& pose,
										PxOverlapCallback& hitCall, const PxQueryFilterData& filterData,
										PxQueryFilterCallback* filterCall) const;


					void			onOriginShift(const PxVec3& shift);


	PxGeometryHolder				mCacheVolume;
	PxTransform						mCachePose;
	PxU32							mMaxShapeCount[2];
	Sq::SceneQueryManager*			mSQManager;
	mutable Ps::Array<PxActorShape> mCache[2]; // AP todo: improve memory management, could we have one allocation for both?
	PxU32							mStaticTimestamp;
	PxU32							mDynamicTimestamp;
	bool							mIsInvalid[2]; // invalid for reasons other than timestamp, such as overflow on previous fill
};

}

#endif // PX_PHYSICS_NP_SCENE
