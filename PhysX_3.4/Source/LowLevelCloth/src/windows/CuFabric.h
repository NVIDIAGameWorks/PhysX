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

#include "Fabric.h"
#include "Range.h"
#include "Types.h"
#include "Allocator.h"
#include "CuContextLock.h"
#include "CuDeviceVector.h"

namespace physx
{

namespace cloth
{

struct CuTether
{
	CuTether(uint16_t, uint16_t);
	uint16_t mAnchor;
	uint16_t mLength;
};

class CuFabric : public UserAllocated, private CuContextLock, public Fabric
{
	PX_NOCOPY(CuFabric)
  public:
	CuFabric(CuFactory& factory, uint32_t numParticles, Range<const uint32_t> phases, Range<const uint32_t> sets,
	         Range<const float> restvalues, Range<const uint32_t> indices, Range<const uint32_t> anchors,
	         Range<const float> tetherLengths, Range<const uint32_t> triangles, uint32_t id);

	virtual ~CuFabric();

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
	CuFactory& mFactory;

	uint32_t mNumParticles;

	CuDeviceVector<uint32_t> mPhases; // index of set to use
	CuDeviceVector<uint32_t> mSets;   // offset of first restvalue, with 0 prefix

	CuDeviceVector<float> mRestvalues;
	CuDeviceVector<uint16_t> mIndices;

	CuDeviceVector<CuTether> mTethers;
	float mTetherLengthScale;

	CuDeviceVector<uint16_t> mTriangles;

	Vector<uint32_t>::Type mNumConstraintsInPhase;
	Vector<CuDevicePointer<const float> >::Type mRestvaluesInPhase;
	Vector<CuDevicePointer<const uint16_t> >::Type mIndicesInPhase;

	uint32_t mId;
};
}
}
