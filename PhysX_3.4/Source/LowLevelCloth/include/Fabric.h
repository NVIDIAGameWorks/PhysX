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

#include "foundation/PxAssert.h"
#include "Types.h"
#include "Range.h"

namespace physx
{
namespace cloth
{

class Factory;

// abstract cloth constraints and triangle indices
class Fabric
{
  protected:
	Fabric(const Fabric&);
	Fabric& operator=(const Fabric&);

  protected:
	Fabric() : mRefCount(0)
	{
	}

  public:
	virtual ~Fabric()
	{
		PX_ASSERT(!mRefCount);
	}

	virtual Factory& getFactory() const = 0;

	virtual uint32_t getNumPhases() const = 0;
	virtual uint32_t getNumRestvalues() const = 0;

	virtual uint32_t getNumSets() const = 0;
	virtual uint32_t getNumIndices() const = 0;

	virtual uint32_t getNumParticles() const = 0;

	virtual uint32_t getNumTethers() const = 0;

	virtual uint32_t getNumTriangles() const = 0;

	virtual void scaleRestvalues(float) = 0;
	virtual void scaleTetherLengths(float) = 0;

	uint16_t getRefCount() const
	{
		return mRefCount;
	}
	void incRefCount()
	{
		++mRefCount;
		PX_ASSERT(mRefCount > 0);
	}
	void decRefCount()
	{
		PX_ASSERT(mRefCount > 0);
		--mRefCount;
	}

  protected:
	uint16_t mRefCount;
};

} // namespace cloth
} // namespace physx
