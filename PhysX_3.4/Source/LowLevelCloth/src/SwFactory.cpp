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
#include "SwFactory.h"
#include "SwFabric.h"
#include "SwCloth.h"
#include "SwSolver.h"
#include "ClothImpl.h"

using namespace physx;

namespace physx
{
namespace cloth
{
// defined in Factory.cpp
uint32_t getNextFabricId();
}
}

cloth::SwFactory::SwFactory() : Factory(CPU)
{
}

cloth::SwFactory::~SwFactory()
{
}

cloth::Fabric* cloth::SwFactory::createFabric(uint32_t numParticles, Range<const uint32_t> phases,
                                              Range<const uint32_t> sets, Range<const float> restvalues,
                                              Range<const uint32_t> indices, Range<const uint32_t> anchors,
                                              Range<const float> tetherLengths, Range<const uint32_t> triangles)
{
	return new SwFabric(*this, numParticles, phases, sets, restvalues, indices, anchors, tetherLengths, triangles,
	                    getNextFabricId());
}

#if PX_SUPPORT_EXTERN_TEMPLATE
//explicit template instantiation declaration
extern template class cloth::ClothImpl<cloth::SwCloth>;
#endif

cloth::Cloth* cloth::SwFactory::createCloth(Range<const PxVec4> particles, Fabric& fabric)
{
	return new SwClothImpl(*this, fabric, particles);
}

cloth::Solver* cloth::SwFactory::createSolver(physx::PxTaskManager* taskMgr)
{
#ifdef PX_PHYSX_GPU_EXPORTS
	// SwSolver not defined in PhysXGpu project
	PX_UNUSED(taskMgr);
	return 0;
#else
	return new SwSolver(taskMgr);
#endif
}

cloth::Cloth* cloth::SwFactory::clone(const Cloth& cloth)
{
	if(cloth.getFactory().getPlatform() != Factory::CPU)
		return cloth.clone(*this); // forward to CuCloth

	// copy construct
	return new SwClothImpl(*this, static_cast<const SwClothImpl&>(cloth));
}

void cloth::SwFactory::extractFabricData(const Fabric& fabric, Range<uint32_t> phases, Range<uint32_t> sets,
                                         Range<float> restvalues, Range<uint32_t> indices, Range<uint32_t> anchors,
                                         Range<float> tetherLengths, Range<uint32_t> triangles) const
{
	const SwFabric& swFabric = static_cast<const SwFabric&>(fabric);

	PX_ASSERT(phases.empty() || phases.size() == swFabric.getNumPhases());
	PX_ASSERT(restvalues.empty() || restvalues.size() == swFabric.getNumRestvalues());
	PX_ASSERT(sets.empty() || sets.size() == swFabric.getNumSets());
	PX_ASSERT(indices.empty() || indices.size() == swFabric.getNumIndices());
	PX_ASSERT(anchors.empty() || anchors.size() == swFabric.getNumTethers());
	PX_ASSERT(tetherLengths.empty() || tetherLengths.size() == swFabric.getNumTethers());

	for(uint32_t i = 0; !phases.empty(); ++i, phases.popFront())
		phases.front() = swFabric.mPhases[i];

	const uint32_t* sEnd = swFabric.mSets.end(), *sIt;
	const float* rBegin = swFabric.mRestvalues.begin(), *rIt = rBegin;
	const uint16_t* iIt = swFabric.mIndices.begin();

	uint32_t* sDst = sets.begin();
	float* rDst = restvalues.begin();
	uint32_t* iDst = indices.begin();

	uint32_t numConstraints = 0;
	for(sIt = swFabric.mSets.begin(); ++sIt != sEnd;)
	{
		const float* rEnd = rBegin + *sIt;
		for(; rIt != rEnd; ++rIt)
		{
			uint16_t i0 = *iIt++;
			uint16_t i1 = *iIt++;

			if(PxMax(i0, i1) >= swFabric.mNumParticles)
				continue;

			if(!restvalues.empty())
				*rDst++ = *rIt;

			if(!indices.empty())
			{
				*iDst++ = i0;
				*iDst++ = i1;
			}

			++numConstraints;
		}

		if(!sets.empty())
			*sDst++ = numConstraints;
	}

	for(uint32_t i = 0; !anchors.empty(); ++i, anchors.popFront())
		anchors.front() = swFabric.mTethers[i].mAnchor;

	for(uint32_t i = 0; !tetherLengths.empty(); ++i, tetherLengths.popFront())
		tetherLengths.front() = swFabric.mTethers[i].mLength * swFabric.mTetherLengthScale;

	for(uint32_t i = 0; !triangles.empty(); ++i, triangles.popFront())
		triangles.front() = swFabric.mTriangles[i];
}

void cloth::SwFactory::extractCollisionData(const Cloth& cloth, Range<PxVec4> spheres, Range<uint32_t> capsules,
                                            Range<PxVec4> planes, Range<uint32_t> convexes, Range<PxVec3> triangles) const
{
	PX_ASSERT(&cloth.getFactory() == this);

	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;

	PX_ASSERT(spheres.empty() || spheres.size() == swCloth.mStartCollisionSpheres.size());
	PX_ASSERT(capsules.empty() || capsules.size() == swCloth.mCapsuleIndices.size() * 2);
	PX_ASSERT(planes.empty() || planes.size() == swCloth.mStartCollisionPlanes.size());
	PX_ASSERT(convexes.empty() || convexes.size() == swCloth.mConvexMasks.size());
	PX_ASSERT(triangles.empty() || triangles.size() == swCloth.mStartCollisionTriangles.size());

	if(!swCloth.mStartCollisionSpheres.empty() && !spheres.empty())
		PxMemCopy(spheres.begin(), &swCloth.mStartCollisionSpheres.front(),
		       swCloth.mStartCollisionSpheres.size() * sizeof(PxVec4));

	if(!swCloth.mCapsuleIndices.empty() && !capsules.empty())
		PxMemCopy(capsules.begin(), &swCloth.mCapsuleIndices.front(), swCloth.mCapsuleIndices.size() * sizeof(IndexPair));

	if(!swCloth.mStartCollisionPlanes.empty() && !planes.empty())
		PxMemCopy(planes.begin(), &swCloth.mStartCollisionPlanes.front(),
		       swCloth.mStartCollisionPlanes.size() * sizeof(PxVec4));

	if(!swCloth.mConvexMasks.empty() && !convexes.empty())
		PxMemCopy(convexes.begin(), &swCloth.mConvexMasks.front(), swCloth.mConvexMasks.size() * sizeof(uint32_t));

	if(!swCloth.mStartCollisionTriangles.empty() && !triangles.empty())
		PxMemCopy(triangles.begin(), &swCloth.mStartCollisionTriangles.front(),
		       swCloth.mStartCollisionTriangles.size() * sizeof(PxVec3));
}

void cloth::SwFactory::extractMotionConstraints(const Cloth& cloth, Range<PxVec4> destConstraints) const
{
	PX_ASSERT(&cloth.getFactory() == this);

	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;

	Vec4fAlignedVector const& srcConstraints = !swCloth.mMotionConstraints.mTarget.empty()
	                                               ? swCloth.mMotionConstraints.mTarget
	                                               : swCloth.mMotionConstraints.mStart;

	if(!srcConstraints.empty())
	{
		// make sure dest array is big enough
		PX_ASSERT(destConstraints.size() == srcConstraints.size());

		PxMemCopy(destConstraints.begin(), &srcConstraints.front(), srcConstraints.size() * sizeof(PxVec4));
	}
}

void cloth::SwFactory::extractSeparationConstraints(const Cloth& cloth, Range<PxVec4> destConstraints) const
{
	PX_ASSERT(&cloth.getFactory() == this);

	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;

	Vec4fAlignedVector const& srcConstraints = !swCloth.mSeparationConstraints.mTarget.empty()
	                                               ? swCloth.mSeparationConstraints.mTarget
	                                               : swCloth.mSeparationConstraints.mStart;

	if(!srcConstraints.empty())
	{
		// make sure dest array is big enough
		PX_ASSERT(destConstraints.size() == srcConstraints.size());

		PxMemCopy(destConstraints.begin(), &srcConstraints.front(), srcConstraints.size() * sizeof(PxVec4));
	}
}

void cloth::SwFactory::extractParticleAccelerations(const Cloth& cloth, Range<PxVec4> destAccelerations) const
{
	PX_ASSERT(&cloth.getFactory() == this);

	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;

	if(!swCloth.mParticleAccelerations.empty())
	{
		// make sure dest array is big enough
		PX_ASSERT(destAccelerations.size() == swCloth.mParticleAccelerations.size());

		PxMemCopy(destAccelerations.begin(), &swCloth.mParticleAccelerations.front(),
		       swCloth.mParticleAccelerations.size() * sizeof(PxVec4));
	}
}

void cloth::SwFactory::extractVirtualParticles(const Cloth& cloth, Range<uint32_t[4]> indices, Range<PxVec3> weights) const
{
	PX_ASSERT(this == &cloth.getFactory());

	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;

	uint32_t numIndices = cloth.getNumVirtualParticles();
	uint32_t numWeights = cloth.getNumVirtualParticleWeights();

	PX_ASSERT(indices.size() == numIndices || indices.empty());
	PX_ASSERT(weights.size() == numWeights || weights.empty());

	if(weights.size() == numWeights)
	{
		PxVec3* wDestIt = reinterpret_cast<PxVec3*>(weights.begin());

		// convert weights from vec4 to vec3
		cloth::Vec4fAlignedVector::ConstIterator wIt = swCloth.mVirtualParticleWeights.begin();
		cloth::Vec4fAlignedVector::ConstIterator wEnd = wIt + numWeights;

		for(; wIt != wEnd; ++wIt, ++wDestIt)
			*wDestIt = PxVec3(wIt->x, wIt->y, wIt->z);

		PX_ASSERT(wDestIt == weights.end());
	}
	if(indices.size() == numIndices)
	{
		// convert indices
		Vec4u* iDestIt = reinterpret_cast<Vec4u*>(indices.begin());
		Vector<Vec4us>::Type::ConstIterator iIt = swCloth.mVirtualParticleIndices.begin();
		Vector<Vec4us>::Type::ConstIterator iEnd = swCloth.mVirtualParticleIndices.end();

		uint32_t numParticles = uint32_t(swCloth.mCurParticles.size());

		for(; iIt != iEnd; ++iIt)
		{
			// skip dummy indices
			if(iIt->x < numParticles)
				// byte offset to element index
				*iDestIt++ = Vec4u(*iIt);
		}

		PX_ASSERT(&array(*iDestIt) == indices.end());
	}
}

void cloth::SwFactory::extractSelfCollisionIndices(const Cloth& cloth, Range<uint32_t> destIndices) const
{
	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;
	PX_ASSERT(destIndices.size() == swCloth.mSelfCollisionIndices.size());
	PxMemCopy(destIndices.begin(), swCloth.mSelfCollisionIndices.begin(), destIndices.size() * sizeof(uint32_t));
}

void cloth::SwFactory::extractRestPositions(const Cloth& cloth, Range<PxVec4> destRestPositions) const
{
	const SwCloth& swCloth = static_cast<const SwClothImpl&>(cloth).mCloth;
	PX_ASSERT(destRestPositions.size() == swCloth.mRestPositions.size());
	PxMemCopy(destRestPositions.begin(), swCloth.mRestPositions.begin(), destRestPositions.size() * sizeof(PxVec4));
}
