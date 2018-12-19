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

#ifndef PT_PARTICLE_SHAPE_CPU_H
#define PT_PARTICLE_SHAPE_CPU_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxTransform.h"
#include "foundation/PxBounds3.h"
#include "PtConfig.h"
#include "PtSpatialHash.h"
#include "PtParticleShape.h"

namespace physx
{

namespace Pt
{

class Context;

class ParticleShapeCpu : public ParticleShape
{
  public:
	ParticleShapeCpu(Context* context, PxU32 index);
	virtual ~ParticleShapeCpu();

	void init(class ParticleSystemSimCpu* particleSystem, const ParticleCell* packet);

	// Implements ParticleShapeCpu
	virtual PxBounds3 getBoundsV() const
	{
		return mBounds;
	}
	virtual void setUserDataV(void* data)
	{
		mUserData = data;
	}
	virtual void* getUserDataV() const
	{
		return mUserData;
	}
	virtual void destroyV();
	//~Implements ParticleShapeCpu

	PX_FORCE_INLINE void setFluidPacket(const ParticleCell* packet)
	{
		PX_ASSERT(packet);
		mPacket = packet;
	}
	PX_FORCE_INLINE const ParticleCell* getFluidPacket() const
	{
		return mPacket;
	}

	PX_FORCE_INLINE PxU32 getIndex() const
	{
		return mIndex;
	}
	PX_FORCE_INLINE class ParticleSystemSimCpu* getParticleSystem()
	{
		return mParticleSystem;
	}
	PX_FORCE_INLINE const class ParticleSystemSimCpu* getParticleSystem() const
	{
		return mParticleSystem;
	}
	PX_FORCE_INLINE GridCellVector getPacketCoordinates() const
	{
		return mPacketCoordinates;
	}

  private:
	PxU32 mIndex;
	class ParticleSystemSimCpu* mParticleSystem;
	PxBounds3 mBounds;
	GridCellVector mPacketCoordinates; // This is needed for the remapping process.
	const ParticleCell* mPacket;
	void* mUserData;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_SHAPE_CPU_H
