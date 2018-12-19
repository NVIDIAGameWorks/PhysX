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


#ifndef NP_SPATIALINDEX
#define NP_SPATIALINDEX

#include "PxSpatialIndex.h"
#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"

namespace physx
{
namespace Sq
{
	class IncrementalPruner;
}

class NpSpatialIndex: public PxSpatialIndex, public Ps::UserAllocated
{
public:
								NpSpatialIndex();
								~NpSpatialIndex();

	virtual	PxSpatialIndexItemId	insert(PxSpatialIndexItem& item,
										   const PxBounds3& bounds);

	virtual	void					update(PxSpatialIndexItemId id,
										   const PxBounds3& bounds);

	virtual	void					remove(PxSpatialIndexItemId id);

	virtual void					overlap(const PxBounds3& aabb,
											PxSpatialOverlapCallback& callback)		const;

	virtual void					raycast(const PxVec3& origin, 
											const PxVec3& unitDir, 
											PxReal maxDist, 
											PxSpatialLocationCallback& callback)	const;

	virtual	void					sweep(const PxBounds3& aabb, 
										  const PxVec3& unitDir, 
										  PxReal maxDist, 
										  PxSpatialLocationCallback& callback)		const;

	virtual void					flush()	{ flushUpdates(); }
	virtual void					rebuildFull();
	virtual void					setIncrementalRebuildRate(PxU32 rate);
	virtual void					rebuildStep();
	virtual void					release();
private:
	
	// const so that we can call it from const methods
	void							flushUpdates() const;

	mutable bool			mPendingUpdates;
	Sq::IncrementalPruner*	mPruner;
};


}
#endif


