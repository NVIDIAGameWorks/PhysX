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

#include "IterationState.h"
#include "SwCollision.h"
#include "SwSelfCollision.h"

namespace physx
{
namespace cloth
{

class SwCloth;
struct SwClothData;

template <typename Simd4f>
class SwSolverKernel
{
  public:
	SwSolverKernel(SwCloth const&, SwClothData&, SwKernelAllocator&, IterationStateFactory&);

	void operator()();

	// returns a conservative estimate of the
	// total memory requirements during a solve
	static size_t estimateTemporaryMemory(const SwCloth& c);

  private:
	void integrateParticles();
	void constrainTether();
	void solveFabric();
	void applyWind();
	void constrainMotion();
	void constrainSeparation();
	void collideParticles();
	void selfCollideParticles();
	void updateSleepState();

	void iterateCloth();
	void simulateCloth();

	SwCloth const& mCloth;
	SwClothData& mClothData;
	SwKernelAllocator& mAllocator;

	SwCollision<Simd4f> mCollision;
	SwSelfCollision<Simd4f> mSelfCollision;
	IterationState<Simd4f> mState;

  private:
	SwSolverKernel<Simd4f>& operator=(const SwSolverKernel<Simd4f>&);
	template <typename AccelerationIterator>
	void integrateParticles(AccelerationIterator& accelIt, const Simd4f&);
};

#if PX_SUPPORT_EXTERN_TEMPLATE
//explicit template instantiation declaration
#if NV_SIMD_SIMD
extern template class SwSolverKernel<Simd4f>;
#endif
#if NV_SIMD_SCALAR
extern template class SwSolverKernel<Scalar4f>;
#endif
#endif

}
}
