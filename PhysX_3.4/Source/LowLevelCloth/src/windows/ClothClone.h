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

#pragma once

#include "foundation/PxMemory.h"

#include "SwFactory.h"
#include "SwFabric.h"
#include "SwCloth.h"

#include "ClothImpl.h"
#include "ClothBase.h"

namespace physx
{
namespace cloth
{
class DxFactory;
class CuFactory;

// make range from vector
template <typename T, typename A>
Range<T> makeRange(shdfnd::Array<T, A>& vec)
{
	T* ptr = vec.empty() ? 0 : vec.begin();
	return Range<T>(ptr, ptr + vec.size());
}

template <typename T, typename A>
Range<const T> makeRange(const shdfnd::Array<T, A>& vec)
{
	const T* ptr = vec.empty() ? 0 : vec.begin();
	return Range<const T>(ptr, ptr + vec.size());
}

// fabric conversion
template <typename SrcClothType, typename DstFactoryType>
typename DstFactoryType::FabricType* convertFabric(const SrcClothType& srcFabric, DstFactoryType& dstFactory)
{
	typedef typename DstFactoryType::FabricType DstFabricType;

	// see if dstFactory already has a Fabric with this id
	DstFabricType* const* fIt = dstFactory.mFabrics.begin();
	DstFabricType* const* fEnd = dstFactory.mFabrics.end();
	for(; fIt != fEnd; ++fIt)
		if((*fIt)->mId == srcFabric.mId)
			return *fIt; // found id, return existing fabric

	// fabric does not exist so create a new one
	Vector<uint32_t>::Type phases(srcFabric.getNumPhases());
	Vector<uint32_t>::Type sets(srcFabric.getNumSets());
	Vector<float>::Type restvalues(srcFabric.getNumRestvalues());
	Vector<uint32_t>::Type indices(srcFabric.getNumIndices());
	Vector<uint32_t>::Type anchors(srcFabric.getNumTethers());
	Vector<float>::Type tetherLengths(srcFabric.getNumTethers());
	Vector<uint32_t>::Type triangles(srcFabric.getNumTriangles() * 3);

	Range<uint32_t> phaseRange = makeRange(phases);
	Range<float> restvalueRange = makeRange(restvalues);
	Range<uint32_t> setRange = makeRange(sets);
	Range<uint32_t> indexRange = makeRange(indices);
	Range<uint32_t> anchorRange = makeRange(anchors);
	Range<float> lengthRange = makeRange(tetherLengths);
	Range<uint32_t> triangleRange = makeRange(triangles);

	srcFabric.mFactory.extractFabricData(srcFabric, phaseRange, setRange, restvalueRange, indexRange, anchorRange,
	                                     lengthRange, triangleRange);

	DstFabricType* dstFabric =
	    static_cast<DstFabricType*>(dstFactory.createFabric(srcFabric.mNumParticles, phaseRange, setRange, restvalueRange,
	                                                        indexRange, anchorRange, lengthRange, triangleRange));

	// give new fabric the same id as the source so it can be matched
	dstFabric->mId = srcFabric.mId;

	return dstFabric;
}

inline Range<const PhaseConfig> getPhaseConfigs(const SwCloth& cloth)
{
	return makeRange(cloth.mPhaseConfigs);
}
inline void setPhaseConfigs(SwCloth& cloth, Range<const PhaseConfig> phaseConfigs)
{
	cloth.mPhaseConfigs.assign(phaseConfigs.begin(), phaseConfigs.end());
}
inline Range<const PxVec4> getParticleAccelerations(const SwCloth& cloth)
{
	return makeRange(cloth.mParticleAccelerations);
}
inline Range<const uint32_t> getSelfCollisionIndices(const SwCloth& cloth)
{
	return makeRange(cloth.mSelfCollisionIndices);
}

// cloth conversion
template <typename DstFactoryType, typename SrcImplType>
typename DstFactoryType::ImplType* convertCloth(DstFactoryType& dstFactory, const SrcImplType& srcImpl)
{
	typedef typename DstFactoryType::FabricType DstFabricType;
	typedef typename DstFactoryType::ImplType DstImplType;
	typedef typename DstImplType::ClothType DstClothType;
	typedef typename SrcImplType::ClothType SrcClothType;

	const SrcClothType& srcCloth = srcImpl.mCloth;
	const Factory& srcFactory = srcCloth.mFactory;

	typename DstClothType::ContextLockType dstLock(dstFactory);
	typename SrcClothType::ContextLockType srcLock(srcCloth.mFactory);

	// particles
	MappedRange<const PxVec4> curParticles = srcImpl.getCurrentParticles();

	// fabric
	DstFabricType& dstFabric = *convertFabric(srcCloth.mFabric, dstFactory);

	// create new cloth
	DstImplType* dstImpl = static_cast<DstImplType*>(dstFactory.createCloth(curParticles, dstFabric));
	DstClothType& dstCloth = dstImpl->mCloth;

	// copy across common parameters
	copy(dstCloth, srcCloth);

	// copy across previous particles
	MappedRange<const PxVec4> prevParticles = srcImpl.getPreviousParticles();
	PxMemCopy(dstImpl->getPreviousParticles().begin(), prevParticles.begin(), prevParticles.size() * sizeof(PxVec4));

	// copy across transformed phase configs
	setPhaseConfigs(dstCloth, getPhaseConfigs(srcCloth));

	// collision data
	Vector<PxVec4>::Type spheres(srcImpl.getNumSpheres(), PxVec4(0.0f));
	PxVec4* spherePtr = spheres.empty() ? 0 : &spheres.front();
	Range<PxVec4> sphereRange(spherePtr, spherePtr + spheres.size());
	Vector<uint32_t>::Type capsules(srcImpl.getNumCapsules() * 2);
	Range<uint32_t> capsuleRange = makeRange(capsules);
	Vector<PxVec4>::Type planes(srcImpl.getNumPlanes(), PxVec4(0.0f));
	PxVec4* planePtr = planes.empty() ? 0 : &planes.front();
	Range<PxVec4> planeRange(planePtr, planePtr + planes.size());
	Vector<uint32_t>::Type convexes(srcImpl.getNumConvexes());
	Range<uint32_t> convexRange = makeRange(convexes);
	Vector<PxVec3>::Type triangles(srcImpl.getNumTriangles() * 3, PxVec3(0.0f));
	PxVec3* trianglePtr = triangles.empty() ? 0 : &triangles.front();
	Range<PxVec3> triangleRange(trianglePtr, trianglePtr + triangles.size());

	srcFactory.extractCollisionData(srcImpl, sphereRange, capsuleRange, planeRange, convexRange, triangleRange);
	dstImpl->setSpheres(sphereRange, 0, 0);
	dstImpl->setCapsules(capsuleRange, 0, 0);
	dstImpl->setPlanes(planeRange, 0, 0);
	dstImpl->setConvexes(convexRange, 0, 0);
	dstImpl->setTriangles(triangleRange, 0, 0);

	// motion constraints, copy directly into new cloth buffer
	if(srcImpl.getNumMotionConstraints())
		srcFactory.extractMotionConstraints(srcImpl, dstImpl->getMotionConstraints());

	// separation constraints, copy directly into new cloth buffer
	if(srcImpl.getNumSeparationConstraints())
		srcFactory.extractSeparationConstraints(srcImpl, dstImpl->getSeparationConstraints());

	// particle accelerations
	if(srcImpl.getNumParticleAccelerations())
	{
		Range<const PxVec4> accelerations = getParticleAccelerations(srcCloth);
		PxMemCopy(dstImpl->getParticleAccelerations().begin(), accelerations.begin(),
		          accelerations.size() * sizeof(PxVec4));
	}

	// self-collision indices
	dstImpl->setSelfCollisionIndices(getSelfCollisionIndices(srcCloth));

	// rest positions
	Vector<PxVec4>::Type restPositions(srcImpl.getNumRestPositions());
	srcFactory.extractRestPositions(srcImpl, makeRange(restPositions));
	dstImpl->setRestPositions(makeRange(restPositions));

	// virtual particles
	if(srcImpl.getNumVirtualParticles())
	{
		Vector<Vec4u>::Type indices(srcImpl.getNumVirtualParticles());
		Vector<PxVec3>::Type weights(srcImpl.getNumVirtualParticleWeights(), PxVec3(0.0f));

		uint32_t(*indicesPtr)[4] = indices.empty() ? 0 : &array(indices.front());
		Range<uint32_t[4]> indicesRange(indicesPtr, indicesPtr + indices.size());

		PxVec3* weightsPtr = weights.empty() ? 0 : &weights.front();
		Range<PxVec3> weightsRange(weightsPtr, weightsPtr + weights.size());

		srcFactory.extractVirtualParticles(srcImpl, indicesRange, weightsRange);

		dstImpl->setVirtualParticles(indicesRange, weightsRange);
	}

	return dstImpl;
}

} // namespace cloth
} // namespace physx
