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

#include "PtParticleShapeCpu.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtContext.h"
#include "PtParticleSystemSimCpu.h"
#include "PtSpatialHash.h"

using namespace physx;
using namespace Pt;

ParticleShapeCpu::ParticleShapeCpu(Context*, PxU32 index)
: mIndex(index), mParticleSystem(NULL), mPacket(NULL), mUserData(NULL)
{
}

ParticleShapeCpu::~ParticleShapeCpu()
{
}

void ParticleShapeCpu::init(ParticleSystemSimCpu* particleSystem, const ParticleCell* packet)
{
	PX_ASSERT(mParticleSystem == NULL);
	PX_ASSERT(mPacket == NULL);
	PX_ASSERT(mUserData == NULL);

	PX_ASSERT(particleSystem);
	PX_ASSERT(packet);

	mParticleSystem = particleSystem;
	mPacket = packet;
	mPacketCoordinates = packet->coords; // this is needed for the remapping process.

	// Compute and store AABB of the assigned packet
	mParticleSystem->getPacketBounds(mPacketCoordinates, mBounds);
}

void ParticleShapeCpu::destroyV()
{
	PX_ASSERT(mParticleSystem);
	mParticleSystem->getContext().releaseParticleShape(this);

	mParticleSystem = NULL;
	mPacket = NULL;
	mUserData = NULL;
}

#endif // PX_USE_PARTICLE_SYSTEM_API
