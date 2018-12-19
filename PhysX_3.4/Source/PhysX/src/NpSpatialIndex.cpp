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

#include "NpSpatialIndex.h"
#include "PsFPU.h"
#include "SqPruner.h"
#include "PxBoxGeometry.h"
#include "PsFoundation.h"
#include "GuBounds.h"

using namespace physx;
using namespace Sq;
using namespace Gu;

NpSpatialIndex::NpSpatialIndex()
: mPendingUpdates(false)
{
	mPruner = createAABBPruner(true);
}

NpSpatialIndex::~NpSpatialIndex()
{
	PX_DELETE(mPruner);
}

PxSpatialIndexItemId NpSpatialIndex::insert(PxSpatialIndexItem& item, const PxBounds3& bounds)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN_VAL(bounds.isValid(), "PxSpatialIndex::insert: bounds are not valid.", PX_SPATIAL_INDEX_INVALID_ITEM_ID);

	PrunerHandle output;
	PrunerPayload payload;
	payload.data[0] = reinterpret_cast<size_t>(&item);
	mPruner->addObjects(&output, &bounds, &payload, 1, false);
	mPendingUpdates = true;
	return output;
}
	
void NpSpatialIndex::update(PxSpatialIndexItemId id, const PxBounds3& bounds)
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN(bounds.isValid(), "PxSpatialIndex::update: bounds are not valid.");

	PxBounds3* b;
	mPruner->getPayload(id, b);
	*b = bounds;
	mPruner->updateObjectsAfterManualBoundsUpdates(&id, 1);

	mPendingUpdates = true;
}

void NpSpatialIndex::remove(PxSpatialIndexItemId id)
{
	PX_SIMD_GUARD;

	mPruner->removeObjects(&id, 1);
	mPendingUpdates = true;
}

namespace
{
	struct OverlapCallback: public PrunerCallback
	{
		OverlapCallback(PxSpatialOverlapCallback& callback) : mUserCallback(callback) {}

		virtual PxAgain invoke(PxReal& /*distance*/, const PrunerPayload& userData)
		{
			PxSpatialIndexItem& item = *reinterpret_cast<PxSpatialIndexItem*>(userData.data[0]);
			return mUserCallback.onHit(item);
		}

		PxSpatialOverlapCallback &mUserCallback;
	private:
		OverlapCallback& operator=(const OverlapCallback&);
	};

	struct LocationCallback: public PrunerCallback
	{
		LocationCallback(PxSpatialLocationCallback& callback) : mUserCallback(callback) {}

		virtual PxAgain invoke(PxReal& distance, const PrunerPayload& userData)
		{
			PxReal oldDistance = distance, shrunkDistance = distance;
			PxSpatialIndexItem& item = *reinterpret_cast<PxSpatialIndexItem*>(userData.data[0]);
			PxAgain result = mUserCallback.onHit(item, distance, shrunkDistance);

			if(shrunkDistance>distance)
				Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "PxSpatialLocationCallback: distance may not be extended.");

			if(!result)
				return false;

			distance = PxMin(oldDistance, distance);
			return true;
		}

		PxSpatialLocationCallback& mUserCallback;

	private:
		LocationCallback& operator=(const LocationCallback&);
	};
}

void NpSpatialIndex::flushUpdates() const
{
	if(mPendingUpdates)
		mPruner->commit();
	mPendingUpdates = false;
}

void NpSpatialIndex::overlap(const PxBounds3& aabb, PxSpatialOverlapCallback& callback) const
{
	PX_SIMD_GUARD;
	PX_CHECK_AND_RETURN(aabb.isValid(), "PxSpatialIndex::overlap: aabb is not valid.");

	flushUpdates();
	OverlapCallback cb(callback);
	PxBoxGeometry boxGeom(aabb.getExtents());
	PxTransform xf(aabb.getCenter());
	ShapeData shapeData(boxGeom, xf, 0.0f); // temporary rvalue not compatible with PX_NOCOPY 
	mPruner->overlap(shapeData, cb);
}

void NpSpatialIndex::raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal maxDist, PxSpatialLocationCallback& callback) const
{
	PX_SIMD_GUARD;

	PX_CHECK_AND_RETURN(origin.isFinite(),								"PxSpatialIndex::raycast: origin is not valid.");
	PX_CHECK_AND_RETURN(unitDir.isFinite() && unitDir.isNormalized(),	"PxSpatialIndex::raycast: unitDir is not valid.");
	PX_CHECK_AND_RETURN(maxDist > 0.0f,									"PxSpatialIndex::raycast: distance must be positive");

	flushUpdates();
	LocationCallback cb(callback);
	mPruner->raycast(origin, unitDir, maxDist, cb);
}

void NpSpatialIndex::sweep(const PxBounds3& aabb, const PxVec3& unitDir, PxReal maxDist, PxSpatialLocationCallback& callback) const
{
	PX_SIMD_GUARD;

	PX_CHECK_AND_RETURN(aabb.isValid(),									"PxSpatialIndex::sweep: aabb is not valid.");
	PX_CHECK_AND_RETURN(unitDir.isFinite() && unitDir.isNormalized(),	"PxSpatialIndex::sweep: unitDir is not valid.");
	PX_CHECK_AND_RETURN(maxDist > 0.0f,									"PxSpatialIndex::sweep: distance must be positive");

	flushUpdates();
	LocationCallback cb(callback);
	PxBoxGeometry boxGeom(aabb.getExtents());
	PxTransform xf(aabb.getCenter());
	ShapeData shapeData(boxGeom, xf, 0.0f); // temporary rvalue not compatible with PX_NOCOPY 
	mPruner->sweep(shapeData, unitDir, maxDist, cb);
}


void NpSpatialIndex::rebuildFull()
{
	PX_SIMD_GUARD;

	mPruner->purge();
	mPruner->commit();
	mPendingUpdates = false;
}

void NpSpatialIndex::setIncrementalRebuildRate(PxU32 rate)
{
	mPruner->setRebuildRateHint(rate);
}

void NpSpatialIndex::rebuildStep()
{
	PX_SIMD_GUARD;
	mPruner->buildStep();
	mPendingUpdates = true;
}

void NpSpatialIndex::release()
{
	delete this;
}

PxSpatialIndex* physx::PxCreateSpatialIndex()
{
	return PX_NEW(NpSpatialIndex)();
}

