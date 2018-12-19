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

#ifndef PT_DYNAMICS_H
#define PT_DYNAMICS_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtConfig.h"
#include "PtParticle.h"
#include "PtDynamicsParameters.h"
#include "PtDynamicsTempBuffers.h"
#include "CmBitMap.h"
#include "CmTask.h"

namespace physx
{

namespace Pt
{

struct ParticleCell;
struct PacketSections;
struct PacketHaloRegions;

class Dynamics
{
  public:
	Dynamics(class ParticleSystemSimCpu& particleSystem);
	~Dynamics();

	void clear();

	void updateSph(physx::PxBaseTask& continuation);

	PX_FORCE_INLINE DynamicsParameters& getParameter()
	{
		return mParams;
	}

  private:
	// Table to get the neighboring halo region indices for a packet section
	struct SectionToHaloTable
	{
		PxU32 numHaloRegions;
		PxU32 haloRegionIndices[19]; // No packet section has more than 19 neighboring halo regions
	};

	struct OrderedIndexTable
	{
		OrderedIndexTable();
		PxU32 indices[PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY];
	};

	struct TaskData
	{
		PxU16 beginPacketIndex;
		PxU16 endPacketIndex;
	};

	void adjustTempBuffers(PxU32 count);

	void schedulePackets(SphUpdateType::Enum updateType, physx::PxBaseTask& continuation);
	void processPacketRange(PxU32 taskDataIndex);

	void updatePacket(SphUpdateType::Enum updateType, PxVec3* forceBuf, Particle* particles, const ParticleCell& packet,
	                  const PacketSections& packetSections, const PacketHaloRegions& haloRegions,
	                  struct DynamicsTempBuffers& tempBuffers);

	void updatePacketLocalHash(SphUpdateType::Enum updateType, PxVec3* forceBuf, Particle* particles,
	                           const ParticleCell& packet, const PacketSections& packetSections,
	                           const PacketHaloRegions& haloRegions, DynamicsTempBuffers& tempBuffers);

	void updateSubpacketPairHalo(PxVec3* __restrict forceBufA, Particle* __restrict particlesSpA, PxU32 numParticlesSpA,
	                             ParticleCell* __restrict particleCellsSpA, PxU32* __restrict particleIndicesSpA,
	                             bool& isLocalHashSpAValid, PxU32 numCellHashBucketsSpA,
	                             Particle* __restrict particlesSpB, PxU32 numParticlesSpB,
	                             ParticleCell* __restrict particleCellsSpB, PxU32* __restrict particleIndicesSpB,
	                             const PxVec3& packetCorner, SphUpdateType::Enum updateType,
	                             PxU16* __restrict hashKeyArray, DynamicsTempBuffers& tempBuffers);

	PX_FORCE_INLINE void updateParticlesBruteForceHalo(SphUpdateType::Enum updateType, PxVec3* forceBuf,
	                                                   Particle* particles, const PacketSections& packetSections,
	                                                   const PacketHaloRegions& haloRegions,
	                                                   DynamicsTempBuffers& tempBuffers);

	void mergeDensity(physx::PxBaseTask* continuation);
	void mergeForce(physx::PxBaseTask* continuation);

  private:
	Dynamics& operator=(const Dynamics&);
	static SectionToHaloTable sSectionToHaloTable[26]; // Halo region table for each packet section
	static OrderedIndexTable sOrderedIndexTable;

	PX_ALIGN(16, DynamicsParameters mParams);
	class ParticleSystemSimCpu& mParticleSystem;
	Particle* mTempReorderedParticles;
	PxVec3* mTempParticleForceBuf;

	typedef Cm::DelegateTask<Dynamics, &Dynamics::mergeDensity> MergeDensityTask;
	typedef Cm::DelegateTask<Dynamics, &Dynamics::mergeForce> MergeForceTask;

	MergeDensityTask mMergeDensityTask;
	MergeForceTask mMergeForceTask;
	PxU32 mNumTasks;
	SphUpdateType::Enum mCurrentUpdateType;
	PxU32 mNumTempBuffers;
	DynamicsTempBuffers mTempBuffers[PT_MAX_PARALLEL_TASKS_SPH];
	TaskData mTaskData[PT_MAX_PARALLEL_TASKS_SPH];
	friend class DynamicsSphTask;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_DYNAMICS_H
