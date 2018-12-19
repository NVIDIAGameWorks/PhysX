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

#include "foundation/PxVec4.h"
#include "Allocator.h"
#include "Fabric.h"
#include "Types.h"
#include "Range.h"

namespace physx
{

namespace cloth
{

class SwFactory;

struct SwTether
{
	SwTether(uint16_t, float);
	uint16_t mAnchor;
	float mLength;
};

class SwFabric : public UserAllocated, public Fabric
{
  public:
#if PX_WINDOWS
	typedef AlignedVector<float, 32>::Type RestvalueContainer; // avx
#else
	typedef AlignedVector<float, 16>::Type RestvalueContainer;
#endif

	SwFabric(SwFactory& factory, uint32_t numParticles, Range<const uint32_t> phases, Range<const uint32_t> sets,
	         Range<const float> restvalues, Range<const uint32_t> indices, Range<const uint32_t> anchors,
	         Range<const float> tetherLengths, Range<const uint32_t> triangles, uint32_t id);

	SwFabric& operator=(const SwFabric&);

	virtual ~SwFabric();

	virtual Factory& getFactory() const;

	virtual uint32_t getNumPhases() const;
	virtual uint32_t getNumRestvalues() const;

	virtual uint32_t getNumSets() const;
	virtual uint32_t getNumIndices() const;

	virtual uint32_t getNumParticles() const;

	virtual uint32_t getNumTethers() const;

	virtual uint32_t getNumTriangles() const;

	virtual void scaleRestvalues(float);
	virtual void scaleTetherLengths(float);

  public:
	SwFactory& mFactory;

	uint32_t mNumParticles;

	Vector<uint32_t>::Type mPhases; // index of set to use
	Vector<uint32_t>::Type mSets;   // offset of first restvalue, with 0 prefix

	RestvalueContainer mRestvalues;  // rest values (edge length)
	Vector<uint16_t>::Type mIndices; // particle index pairs

	Vector<SwTether>::Type mTethers;
	float mTetherLengthScale;

	Vector<uint16_t>::Type mTriangles;

	uint32_t mId;

	uint32_t mOriginalNumRestvalues;

} PX_ALIGN_SUFFIX(16);
}
}
