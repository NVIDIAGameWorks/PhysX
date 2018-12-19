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

#include "Factory.h"
#include "Allocator.h"

namespace physx
{

namespace cloth
{

class SwFabric;
class SwCloth;
template <typename>
class ClothImpl;

class SwFactory : public UserAllocated, public Factory
{
  public:
	typedef SwFabric FabricType;
	typedef ClothImpl<SwCloth> ImplType;

	SwFactory();
	virtual ~SwFactory();

	virtual Fabric* createFabric(uint32_t numParticles, Range<const uint32_t> phases, Range<const uint32_t> sets,
	                             Range<const float> restvalues, Range<const uint32_t> indices,
	                             Range<const uint32_t> anchors, Range<const float> tetherLengths,
	                             Range<const uint32_t> triangles);

	virtual Cloth* createCloth(Range<const PxVec4> particles, Fabric& fabric);

	virtual Solver* createSolver(physx::PxTaskManager*);

	virtual Cloth* clone(const Cloth& cloth);

	virtual void extractFabricData(const Fabric& fabric, Range<uint32_t> phases, Range<uint32_t> sets,
	                               Range<float> restvalues, Range<uint32_t> indices, Range<uint32_t> anchors,
	                               Range<float> tetherLengths, Range<uint32_t> triangles) const;

	virtual void extractCollisionData(const Cloth& cloth, Range<PxVec4> spheres, Range<uint32_t> capsules,
	                                  Range<PxVec4> planes, Range<uint32_t> convexes, Range<PxVec3> triangles) const;

	virtual void extractMotionConstraints(const Cloth& cloth, Range<PxVec4> destConstraints) const;

	virtual void extractSeparationConstraints(const Cloth& cloth, Range<PxVec4> destConstraints) const;

	virtual void extractParticleAccelerations(const Cloth& cloth, Range<PxVec4> destAccelerations) const;

	virtual void extractVirtualParticles(const Cloth& cloth, Range<uint32_t[4]> destIndices,
	                                     Range<PxVec3> destWeights) const;

	virtual void extractSelfCollisionIndices(const Cloth& cloth, Range<uint32_t> destIndices) const;

	virtual void extractRestPositions(const Cloth& cloth, Range<PxVec4> destRestPositions) const;

  public:
	Vector<SwFabric*>::Type mFabrics;
};
}
}
