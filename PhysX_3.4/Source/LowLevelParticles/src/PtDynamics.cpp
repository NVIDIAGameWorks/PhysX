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

#include "PtDynamics.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PsBitUtils.h"
#include "PsIntrinsics.h"
#include "PsAllocator.h"
#include "CmFlushPool.h"

#include "PtDynamicHelper.h"
#include "PtParticleSystemSimCpu.h"
#include "PtContext.h"

#define MERGE_HALO_REGIONS 0

using namespace physx;
using namespace Pt;

PX_FORCE_INLINE void Dynamics::updateParticlesBruteForceHalo(SphUpdateType::Enum updateType, PxVec3* forceBuf,
                                                             Particle* particles, const PacketSections& packetSections,
                                                             const PacketHaloRegions& haloRegions,
                                                             DynamicsTempBuffers& tempBuffers)
{
	for(PxU32 i = 0; i < 26; i++)
	{
		if(packetSections.numParticles[i] == 0)
			continue;

		Particle* particlesA = &particles[packetSections.firstParticle[i]];
		PxVec3* forceBufA = &forceBuf[packetSections.firstParticle[i]];

		//
		// Get neighboring halo regions for the packet section
		//
		PxU32 numHaloRegions = sSectionToHaloTable[i].numHaloRegions;
		PxU32* haloRegionIndices = sSectionToHaloTable[i].haloRegionIndices;
		PxU32 mergedIndexCount = 0;
		//
		// Iterate over neighboring halo regions and update particles
		//
		for(PxU32 j = 0; j < numHaloRegions; j++)
		{
			PxU32 idx = haloRegionIndices[j];

			if(haloRegions.numParticles[idx] == 0)
				continue;

			if(mergedIndexCount + haloRegions.numParticles[idx] > PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY)
			{
				updateParticleGroupPair(forceBufA, forceBuf, particlesA, particles, tempBuffers.orderedIndicesSubpacket,
				                        packetSections.numParticles[i], tempBuffers.mergedIndices, mergedIndexCount,
				                        false, updateType == SphUpdateType::DENSITY, mParams,
				                        tempBuffers.simdPositionsSubpacket, tempBuffers.indexStream);
				mergedIndexCount = 0;
			}
			PxU32 hpIndex = haloRegions.firstParticle[idx];
			for(PxU32 k = 0; k < haloRegions.numParticles[idx]; k++)
				tempBuffers.mergedIndices[mergedIndexCount++] = hpIndex++;
		}

		if(mergedIndexCount > 0)
		{
			updateParticleGroupPair(forceBufA, forceBuf, particlesA, particles, tempBuffers.orderedIndicesSubpacket,
			                        packetSections.numParticles[i], tempBuffers.mergedIndices, mergedIndexCount, false,
			                        updateType == SphUpdateType::DENSITY, mParams, tempBuffers.simdPositionsSubpacket,
			                        tempBuffers.indexStream);
		}
	}
}

// The following table defines for each packet section (except the one in the centre) the number
// of neighboring halo region as well as the indices of these neighboring halo region
Dynamics::SectionToHaloTable Dynamics::sSectionToHaloTable[26] = {
	{ 19, { 0, 2, 6, 8, 18, 20, 24, 26, 36, 38, 42, 44, 54, 56, 66, 68, 78, 80, 90 } },     // 0
	{ 19, { 1, 2, 7, 8, 19, 20, 25, 26, 45, 47, 51, 53, 55, 56, 72, 74, 84, 86, 91 } },     // 1
	{ 15, { 0, 1, 2, 6, 7, 8, 18, 19, 20, 24, 25, 26, 54, 55, 56 } },                       // 2
	{ 19, { 3, 5, 6, 8, 27, 29, 33, 35, 37, 38, 43, 44, 60, 62, 67, 68, 81, 83, 92 } },     // 3
	{ 19, { 4, 5, 7, 8, 28, 29, 34, 35, 46, 47, 52, 53, 61, 62, 73, 74, 87, 89, 93 } },     // 4
	{ 15, { 3, 4, 5, 6, 7, 8, 27, 28, 29, 33, 34, 35, 60, 61, 62 } },                       // 5
	{ 15, { 0, 2, 3, 5, 6, 8, 36, 37, 38, 42, 43, 44, 66, 67, 68 } },                       // 6
	{ 15, { 1, 2, 4, 5, 7, 8, 45, 46, 47, 51, 52, 53, 72, 73, 74 } },                       // 7
	{ 9, { 0, 1, 2, 3, 4, 5, 6, 7, 8 } },                                                   // 8
	{ 19, { 9, 11, 15, 17, 21, 23, 24, 26, 39, 41, 42, 44, 57, 59, 69, 71, 79, 80, 94 } },  // 9
	{ 19, { 10, 11, 16, 17, 22, 23, 25, 26, 48, 50, 51, 53, 58, 59, 75, 77, 85, 86, 95 } }, // 10
	{ 15, { 9, 10, 11, 15, 16, 17, 21, 22, 23, 24, 25, 26, 57, 58, 59 } },                  // 11
	{ 19, { 12, 14, 15, 17, 30, 32, 33, 35, 40, 41, 43, 44, 63, 65, 70, 71, 82, 83, 96 } }, // 12
	{ 19, { 13, 14, 16, 17, 31, 32, 34, 35, 49, 50, 52, 53, 64, 65, 76, 77, 88, 89, 97 } }, // 13
	{ 15, { 12, 13, 14, 15, 16, 17, 30, 31, 32, 33, 34, 35, 63, 64, 65 } },                 // 14
	{ 15, { 9, 11, 12, 14, 15, 17, 39, 40, 41, 42, 43, 44, 69, 70, 71 } },                  // 15
	{ 15, { 10, 11, 13, 14, 16, 17, 48, 49, 50, 51, 52, 53, 75, 76, 77 } },                 // 16
	{ 9, { 9, 10, 11, 12, 13, 14, 15, 16, 17 } },                                           // 17
	{ 15, { 18, 20, 21, 23, 24, 26, 36, 38, 39, 41, 42, 44, 78, 79, 80 } },                 // 18
	{ 15, { 19, 20, 22, 23, 25, 26, 45, 47, 48, 50, 51, 53, 84, 85, 86 } },                 // 19
	{ 9, { 18, 19, 20, 21, 22, 23, 24, 25, 26 } },                                          // 20
	{ 15, { 27, 29, 30, 32, 33, 35, 37, 38, 40, 41, 43, 44, 81, 82, 83 } },                 // 21
	{ 15, { 28, 29, 31, 32, 34, 35, 46, 47, 49, 50, 52, 53, 87, 88, 89 } },                 // 22
	{ 9, { 27, 28, 29, 30, 31, 32, 33, 34, 35 } },                                          // 23
	{ 9, { 36, 37, 38, 39, 40, 41, 42, 43, 44 } },                                          // 24
	{ 9, { 45, 46, 47, 48, 49, 50, 51, 52, 53 } },                                          // 25
};

Dynamics::OrderedIndexTable Dynamics::sOrderedIndexTable;

Dynamics::OrderedIndexTable::OrderedIndexTable()
{
	for(PxU32 i = 0; i < PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY; ++i)
		indices[i] = i;
}

namespace physx
{

namespace Pt
{

class DynamicsSphTask : public Cm::Task
{
  public:
	DynamicsSphTask(Dynamics& context, PxU32 taskDataIndex) : Cm::Task(0), mDynamicsContext(context), mTaskDataIndex(taskDataIndex)
	{
	}

	virtual void runInternal()
	{
		mDynamicsContext.processPacketRange(mTaskDataIndex);
	}

	virtual const char* getName() const
	{
		return "Pt::Dynamics.sph";
	}

  private:
	DynamicsSphTask& operator=(const DynamicsSphTask&);
	Dynamics& mDynamicsContext;
	PxU32 mTaskDataIndex;
};

} // namespace Pt
} // namespace physx

Dynamics::Dynamics(ParticleSystemSimCpu& particleSystem)
: mParticleSystem(particleSystem)
, mTempReorderedParticles(NULL)
, mTempParticleForceBuf(NULL)
, mMergeDensityTask(0, this, "Pt::Dynamics.mergeDensity")
, mMergeForceTask(0, this, "Pt::Dynamics.mergeForce")
, mNumTempBuffers(0)
{
}

Dynamics::~Dynamics()
{
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::clear()
{
	if(mTempReorderedParticles)
	{
		mParticleSystem.mAlign16.deallocate(mTempReorderedParticles);
		mTempReorderedParticles = NULL;
	}

	adjustTempBuffers(0);
}

void Dynamics::adjustTempBuffers(PxU32 count)
{
	PX_ASSERT(count <= PT_MAX_PARALLEL_TASKS_SPH);
	PX_ASSERT(mNumTempBuffers <= PT_MAX_PARALLEL_TASKS_SPH);
	Ps::AlignedAllocator<16, Ps::ReflectionAllocator<char> > align16;

	// shrink
	for(PxU32 i = count; i < mNumTempBuffers; ++i)
	{
		DynamicsTempBuffers& tempBuffers = mTempBuffers[i];

		if(tempBuffers.indexStream)
			PX_FREE_AND_RESET(tempBuffers.indexStream);

		if(tempBuffers.hashKeys)
			PX_FREE_AND_RESET(tempBuffers.hashKeys);

		if(tempBuffers.mergedIndices)
			PX_FREE_AND_RESET(tempBuffers.mergedIndices);

		if(tempBuffers.indicesSubpacketA)
			PX_FREE_AND_RESET(tempBuffers.indicesSubpacketA);

		if(tempBuffers.indicesSubpacketB)
			PX_FREE_AND_RESET(tempBuffers.indicesSubpacketB);

		if(tempBuffers.cellHashTableSubpacketB)
			PX_FREE_AND_RESET(tempBuffers.cellHashTableSubpacketB);

		if(tempBuffers.cellHashTableSubpacketA)
			PX_FREE_AND_RESET(tempBuffers.cellHashTableSubpacketA);

		if(tempBuffers.simdPositionsSubpacket)
		{
			align16.deallocate(tempBuffers.simdPositionsSubpacket);
			tempBuffers.simdPositionsSubpacket = NULL;
		}

		if(tempBuffers.mergedHaloRegions)
		{
			align16.deallocate(tempBuffers.mergedHaloRegions);
			tempBuffers.mergedHaloRegions = NULL;
		}
	}

	// growing
	for(PxU32 i = mNumTempBuffers; i < count; ++i)
	{
		DynamicsTempBuffers& tempBuffers = mTempBuffers[i];

		// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
		tempBuffers.cellHashMaxSize = Ps::nextPowerOfTwo((PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY + 1));

		// Local hash tables for particle cells (for two subpackets A and B).
		tempBuffers.cellHashTableSubpacketA = reinterpret_cast<ParticleCell*>(
		    PX_ALLOC(tempBuffers.cellHashMaxSize * sizeof(ParticleCell), "ParticleCell"));
		tempBuffers.cellHashTableSubpacketB = reinterpret_cast<ParticleCell*>(
		    PX_ALLOC(tempBuffers.cellHashMaxSize * sizeof(ParticleCell), "ParticleCell"));

		// Particle index lists for local hash of particle cells (for two subpackets A and B).
		tempBuffers.indicesSubpacketA = reinterpret_cast<PxU32*>(
		    PX_ALLOC(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY * sizeof(PxU32), "Subpacket indices"));
		tempBuffers.indicesSubpacketB = reinterpret_cast<PxU32*>(
		    PX_ALLOC(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY * sizeof(PxU32), "Subpacket indices"));
		tempBuffers.mergedIndices = reinterpret_cast<PxU32*>(
		    PX_ALLOC(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY * sizeof(PxU32), "Subpacket merged indices"));
		tempBuffers.mergedHaloRegions = reinterpret_cast<Particle*>(
		    align16.allocate(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY * sizeof(Particle), __FILE__, __LINE__));

		tempBuffers.hashKeys = reinterpret_cast<PxU16*>(
		    PX_ALLOC(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY * sizeof(PxU16), "Subpacket hashKeys"));

		// SIMD buffer for storing intermediate particle positions of up to a subpacket size.
		// Ceil up to multiple of four + 4 for save unrolling.
		// For 4 particles we need three Vec4V.
		PxU32 paddedSubPacketMax = ((PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY + 3) & ~0x3) + 4;
		tempBuffers.simdPositionsSubpacket =
		    reinterpret_cast<PxU8*>(align16.allocate(3 * (paddedSubPacketMax / 4) * sizeof(Vec4V), __FILE__, __LINE__));

		tempBuffers.indexStream =
		    reinterpret_cast<PxU32*>(PX_ALLOC(MAX_INDEX_STREAM_SIZE * sizeof(PxU32), "indexStream"));
		tempBuffers.orderedIndicesSubpacket = sOrderedIndexTable.indices;
	}

	mNumTempBuffers = count;
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::updateSph(physx::PxBaseTask& continuation)
{
	Particle* particles = mParticleSystem.mParticleState->getParticleBuffer();
	PxU32 numParticles = mParticleSystem.mNumPacketParticlesIndices;
	const PxU32* particleIndices = mParticleSystem.mPacketParticlesIndices;
	const ParticleCell* packets = mParticleSystem.mSpatialHash->getPackets();
	const PacketSections* packetSections = mParticleSystem.mSpatialHash->getPacketSections();
	PX_ASSERT(packets);
	PX_ASSERT(packetSections);
	PX_ASSERT(numParticles > 0);
	PX_UNUSED(packetSections);

	{
		// sschirm: for now we reorder particles for sph exclusively, and scatter again after sph.
		if(!mTempReorderedParticles)
		{
			PxU32 maxParticles = mParticleSystem.mParticleState->getMaxParticles();
			mTempReorderedParticles = reinterpret_cast<Particle*>(
			    mParticleSystem.mAlign16.allocate(maxParticles * sizeof(Particle), __FILE__, __LINE__));
		}

		if(!mTempParticleForceBuf)
		{
			PxU32 maxParticles = mParticleSystem.mParticleState->getMaxParticles();
			// sschirm: Add extra float, since we are accessing this buffer later with: Vec4V_From_F32Array.
			// The last 4 element would contain unallocated memory otherwise.
			// Also initializing buffer that may only be used partially and non-contiguously with 0 to avoid
			// simd operations to use bad values.
			PxU32 byteSize = maxParticles * sizeof(PxVec3) + sizeof(PxF32);
			mTempParticleForceBuf =
			    reinterpret_cast<PxVec3*>(mParticleSystem.mAlign16.allocate(byteSize, __FILE__, __LINE__));
			memset(mTempParticleForceBuf, 0, byteSize);
		}

		for(PxU32 i = 0; i < numParticles; ++i)
		{
			PxU32 particleIndex = particleIndices[i];
			mTempReorderedParticles[i] = particles[particleIndex];
		}

		// would be nice to get available thread count to decide on task decomposition
		// mParticleSystem.getContext().getTaskManager().getCpuDispatcher();

		// use number of particles for task decomposition
		PxU32 targetParticleCountPerTask =
		    PxMax(PxU32(numParticles / PT_MAX_PARALLEL_TASKS_SPH), PxU32(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));
		PxU16 packetIndex = 0;
		PxU16 lastPacketIndex = 0;
		PxU32 numTasks = 0;
		for(PxU32 i = 0; i < PT_MAX_PARALLEL_TASKS_SPH; ++i)
		{
			// if this is the last interation, we need to gather all remaining packets
			if(i == PT_MAX_PARALLEL_TASKS_SPH - 1)
				targetParticleCountPerTask = 0xffffffff;

			lastPacketIndex = packetIndex;
			PxU32 currentParticleCount = 0;
			while(currentParticleCount < targetParticleCountPerTask && packetIndex < PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE)
			{
				const ParticleCell& packet = packets[packetIndex];
				currentParticleCount += (packet.numParticles != PX_INVALID_U32) ? packet.numParticles : 0;
				packetIndex++;
			}

			if(currentParticleCount > 0)
			{
				PX_ASSERT(lastPacketIndex != packetIndex);
				mTaskData[i].beginPacketIndex = lastPacketIndex;
				mTaskData[i].endPacketIndex = packetIndex;
				numTasks++;
			}
			else
			{
				mTaskData[i].beginPacketIndex = PX_INVALID_U16;
				mTaskData[i].endPacketIndex = PX_INVALID_U16;
			}
		}
		PX_ASSERT(packetIndex == PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE);

		mNumTasks = numTasks;
		adjustTempBuffers(PxMax(numTasks, mNumTempBuffers));

		mMergeForceTask.setContinuation(&continuation);
		mMergeDensityTask.setContinuation(&mMergeForceTask);

		schedulePackets(SphUpdateType::DENSITY, mMergeDensityTask);
		mMergeDensityTask.removeReference();
	}
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::mergeDensity(physx::PxBaseTask* /*continuation*/)
{
	schedulePackets(SphUpdateType::FORCE, mMergeForceTask);
	mMergeForceTask.removeReference();
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::mergeForce(physx::PxBaseTask* /*continuation*/)
{
	PxU32 numParticles = mParticleSystem.mNumPacketParticlesIndices;
	Particle* particles = mParticleSystem.mParticleState->getParticleBuffer();
	PxVec3* forces = mParticleSystem.mTransientBuffer;
	const PxU32* particleIndices = mParticleSystem.mPacketParticlesIndices;

	// reorder and normalize density.
	for(PxU32 i = 0; i < numParticles; ++i)
	{
		PxU32 particleIndex = particleIndices[i];
		Particle& particle = mTempReorderedParticles[i];
		normalizeParticleDensity(particle, mParams.selfDensity, mParams.densityNormalizationFactor);
		particles[particleIndex] = particle;
		forces[particleIndex] = mTempParticleForceBuf[i];
	}

	mParticleSystem.mAlign16.deallocate(mTempParticleForceBuf);
	mTempParticleForceBuf = NULL;
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::schedulePackets(SphUpdateType::Enum updateType, physx::PxBaseTask& continuation)
{
	mCurrentUpdateType = updateType;
	for(PxU32 i = 0; i < mNumTasks; ++i)
	{
		PX_ASSERT(mTaskData[i].beginPacketIndex != PX_INVALID_U16 && mTaskData[i].endPacketIndex != PX_INVALID_U16);
		void* ptr = mParticleSystem.getContext().getTaskPool().allocate(sizeof(DynamicsSphTask));
		DynamicsSphTask* task = PX_PLACEMENT_NEW(ptr, DynamicsSphTask)(*this, i);
		task->setContinuation(&continuation);
		task->removeReference();
	}
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::processPacketRange(PxU32 taskDataIndex)
{
	const ParticleCell* packets = mParticleSystem.mSpatialHash->getPackets();
	const PacketSections* packetSections = mParticleSystem.mSpatialHash->getPacketSections();
	Particle* particles = mTempReorderedParticles;
	PxVec3* forceBuf = mTempParticleForceBuf;

	TaskData& taskData = mTaskData[taskDataIndex];

	for(PxU16 p = taskData.beginPacketIndex; p < taskData.endPacketIndex; ++p)
	{
		const ParticleCell& packet = packets[p];
		if(packet.numParticles == PX_INVALID_U32)
			continue;

		// Get halo regions with neighboring particles
		PacketHaloRegions haloRegions;
		SpatialHash::getHaloRegions(haloRegions, packet.coords, packets, packetSections,
		                            PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE);

		updatePacket(mCurrentUpdateType, forceBuf, particles, packet, packetSections[p], haloRegions,
		             mTempBuffers[taskDataIndex]);
	}
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::updatePacket(SphUpdateType::Enum updateType, PxVec3* forceBuf, Particle* particles,
                            const ParticleCell& packet, const PacketSections& packetSections,
                            const PacketHaloRegions& haloRegions, DynamicsTempBuffers& tempBuffers)
{
	PX_COMPILE_TIME_ASSERT(PT_BRUTE_FORCE_PARTICLE_THRESHOLD <= PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY);

	updateParticlesPrePass(updateType, forceBuf + packet.firstParticle, particles + packet.firstParticle,
	                       packet.numParticles, mParams);
	bool bruteForceApproach = ((packet.numParticles <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD) &&
	                           (haloRegions.maxNumParticles <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD));

	if(bruteForceApproach)
	{
		// There are not enough particles in the packet and its neighbors to make it worth building the local cell hash.
		// So, we do a brute force approach testing each particle against each particle.
		// sschirm: TODO check whether one way is faster (fewer function calls... more math)
		Particle* packetParticles = particles + packet.firstParticle;
		PxVec3* packetForceBuf = forceBuf + packet.firstParticle;
		for(PxU32 p = 1; p < packet.numParticles; p++)
		{
			updateParticleGroupPair(packetForceBuf, packetForceBuf, packetParticles, packetParticles,
			                        tempBuffers.orderedIndicesSubpacket + p - 1, 1,
			                        tempBuffers.orderedIndicesSubpacket + p, packet.numParticles - p, true,
			                        updateType == SphUpdateType::DENSITY, mParams, tempBuffers.simdPositionsSubpacket,
			                        tempBuffers.indexStream);
		}

		// Compute particle interactions between particles of the current packet and particles of neighboring packets.
		updateParticlesBruteForceHalo(updateType, forceBuf, particles, packetSections, haloRegions, tempBuffers);
	}
	else
	{
		updatePacketLocalHash(updateType, forceBuf, particles, packet, packetSections, haloRegions, tempBuffers);
	}

	updateParticlesPostPass(updateType, forceBuf + packet.firstParticle, particles + packet.firstParticle,
	                        packet.numParticles, mParams);
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::updatePacketLocalHash(SphUpdateType::Enum updateType, PxVec3* forceBuf, Particle* particles,
                                     const ParticleCell& packet, const PacketSections& packetSections,
                                     const PacketHaloRegions& haloRegions, DynamicsTempBuffers& tempBuffers)
{
	// Particle index lists for local hash of particle cells (for two subpackets A and B).
	PxU32* particleIndicesSpA = tempBuffers.indicesSubpacketA;
	PxU32* particleIndicesSpB = tempBuffers.indicesSubpacketB;

	// Local hash tables for particle cells (for two subpackets A and B).
	ParticleCell* particleCellsSpA = tempBuffers.cellHashTableSubpacketA;
	ParticleCell* particleCellsSpB = tempBuffers.cellHashTableSubpacketB;

	PxVec3 packetCorner =
	    PxVec3(PxReal(packet.coords.x), PxReal(packet.coords.y), PxReal(packet.coords.z)) * mParams.packetSize;

	PxU32 particlesLeftA0 = packet.numParticles;
	Particle* particlesSpA0 = particles + packet.firstParticle;
	PxVec3* forceBufA0 = forceBuf + packet.firstParticle;

	while(particlesLeftA0)
	{
		PxU32 numParticlesSpA = PxMin(particlesLeftA0, static_cast<PxU32>(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));

		// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
		const PxU32 numCellHashBucketsSpA = Ps::nextPowerOfTwo(numParticlesSpA + 1);
		PX_ASSERT(numCellHashBucketsSpA <= tempBuffers.cellHashMaxSize);

		// Get local cell hash for the current subpacket
		SpatialHash::buildLocalHash(particlesSpA0, numParticlesSpA, particleCellsSpA, particleIndicesSpA,
		                            tempBuffers.hashKeys, numCellHashBucketsSpA, mParams.cellSizeInv, packetCorner);

		//---------------------------------------------------------------------------------------------------

		//
		// Compute particle interactions between particles within the current subpacket.
		//

		updateCellsSubpacket(updateType, forceBufA0, particlesSpA0, particleCellsSpA, particleIndicesSpA,
		                     numCellHashBucketsSpA, mParams, tempBuffers);

		//---------------------------------------------------------------------------------------------------

		//
		// Compute particle interactions between particles of current subpacket and particles
		// of other subpackets within the same packet (i.e., we process all subpacket pairs).
		//

		PxU32 particlesLeftB = particlesLeftA0 - numParticlesSpA;
		Particle* particlesSpB = particlesSpA0 + numParticlesSpA;
		PxVec3* forceBufB = forceBufA0 + numParticlesSpA;

		while(particlesLeftB)
		{
			PxU32 numParticlesSpB = PxMin(particlesLeftB, static_cast<PxU32>(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));

			// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
			const PxU32 numCellHashBucketsSpB = Ps::nextPowerOfTwo(numParticlesSpB + 1);
			PX_ASSERT(numCellHashBucketsSpB <= tempBuffers.cellHashMaxSize);

			// Get local cell hash for other subpacket
			SpatialHash::buildLocalHash(particlesSpB, numParticlesSpB, particleCellsSpB, particleIndicesSpB,
			                            tempBuffers.hashKeys, numCellHashBucketsSpB, mParams.cellSizeInv, packetCorner);

			// For the cells of subpacket A, find neighboring cells in the subpacket B and compute particle
			// interactions.
			updateCellsSubpacketPair(updateType, forceBufA0, forceBufB, particlesSpA0, particlesSpB, particleCellsSpA,
			                         particleCellsSpB, particleIndicesSpA, particleIndicesSpB, numCellHashBucketsSpA,
			                         numCellHashBucketsSpB, true, mParams, tempBuffers,
			                         numParticlesSpA < numParticlesSpB);

			particlesLeftB -= numParticlesSpB;
			particlesSpB += numParticlesSpB;
			forceBufB += numParticlesSpB;
		}

		particlesLeftA0 -= numParticlesSpA;
		particlesSpA0 += numParticlesSpA;
		forceBufA0 += numParticlesSpA;
	}

	//---------------------------------------------------------------------------------------------------

	//
	// Compute particle interactions between particles of sections of the current packet and particles of neighboring
	// halo regions
	//

	PX_ASSERT(PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION <= PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY);
	if(haloRegions.maxNumParticles != 0)
	{
		for(PxU32 s = 0; s < 26; s++)
		{
			PxU32 numSectionParticles = packetSections.numParticles[s];
			if(numSectionParticles == 0)
				continue;

			bool sectionEnablesBruteForce = (numSectionParticles <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION);

			SectionToHaloTable& neighborHaloRegions = sSectionToHaloTable[s];
			PxU32 numHaloNeighbors = neighborHaloRegions.numHaloRegions;

			PxU32 particlesLeftA = numSectionParticles;
			Particle* particlesSpA = particles + packetSections.firstParticle[s];
			PxVec3* forceBufA = forceBuf + packetSections.firstParticle[s];

			while(particlesLeftA)
			{
				PxU32 numParticlesSpA =
				    PxMin(particlesLeftA, static_cast<PxU32>(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));

				// Compute particle interactions between particles of the current subpacket (of the section)
				// and particles of neighboring halo regions relevant.

				// Process halo regions which need local hash building first.
				bool isLocalHashValid = false;

				// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
				const PxU32 numCellHashBucketsSpA = Ps::nextPowerOfTwo(numParticlesSpA + 1);
				PX_ASSERT(numCellHashBucketsSpA <= tempBuffers.cellHashMaxSize);
#if MERGE_HALO_REGIONS
				// Read halo region particles into temporary buffer
				PxU32 numMergedHaloParticles = 0;
				for(PxU32 h = 0; h < numHaloNeighbors; h++)
				{
					PxU32 haloRegionIdx = neighborHaloRegions.haloRegionIndices[h];
					PxU32 numHaloParticles = haloRegions.numParticles[haloRegionIdx];

					// chunk regions into subpackets!
					PxU32 particlesLeftB = numHaloParticles;
					Particle* particlesSpB = particles + haloRegions.firstParticle[haloRegionIdx];
					PxVec3* forceBufB = forceBuf + haloRegions.firstParticle[haloRegionIdx];
					while(particlesLeftB)
					{
						PxU32 numParticlesSpB =
						    PxMin(particlesLeftB, static_cast<PxU32>(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));

						// if there are plenty of particles already, don't bother to do the copy for merging.
						if(numParticlesSpB > PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION)
						{
							updateSubpacketPairHalo(forceBufA, particlesSpA, numParticlesSpA, particleCellsSpA,
							                        particleIndicesSpA, isLocalHashValid, numCellHashBucketsSpA,
							                        forceBufB, particlesSpB, numParticlesSpB, particleCellsSpB,
							                        particleIndicesSpB, packetCorner, updateType, hashKeyArray,
							                        tempBuffers);
						}
						else
						{
							if(numMergedHaloParticles + numParticlesSpB > PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY)
							{
								// flush
								updateSubpacketPairHalo(forceBufA, particlesSpA, numParticlesSpA, particleCellsSpA,
								                        particleIndicesSpA, isLocalHashValid, numCellHashBucketsSpA,
								                        tempBuffers.mergedHaloRegions, numMergedHaloParticles,
								                        particleCellsSpB, particleIndicesSpB, packetCorner, updateType,
								                        hashKeyArray, tempBuffers);
								numMergedHaloParticles = 0;
							}

							for(PxU32 k = 0; k < numParticlesSpB; ++k)
								tempBuffers.mergedHaloRegions[numMergedHaloParticles++] = particlesSpB[k];
						}

						particlesLeftB -= numParticlesSpB;
						particlesSpB += numParticlesSpB;
					}
				}

				// flush
				updateSubpacketPairHalo(forceBufA, particlesSpA, numParticlesSpA, particleCellsSpA, particleIndicesSpA,
				                        isLocalHashValid, numCellHashBucketsSpA, tempBuffers.mergedHaloRegions,
				                        numMergedHaloParticles, particleCellsSpB, particleIndicesSpB, packetCorner,
				                        updateType, hashKeyArray, tempBuffers);
#else  // MERGE_HALO_REGIONS
				for(PxU32 h = 0; h < numHaloNeighbors; h++)
				{
					PxU32 haloRegionIdx = neighborHaloRegions.haloRegionIndices[h];
					PxU32 numHaloParticles = haloRegions.numParticles[haloRegionIdx];

					bool haloRegionEnablesBruteForce =
					    (numHaloParticles <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION);

					if(sectionEnablesBruteForce && haloRegionEnablesBruteForce)
						continue;

					if(!isLocalHashValid)
					{
						// Get local cell hash for the current subpacket
						SpatialHash::buildLocalHash(particlesSpA, numParticlesSpA, particleCellsSpA, particleIndicesSpA,
						                            tempBuffers.hashKeys, numCellHashBucketsSpA, mParams.cellSizeInv,
						                            packetCorner);
						isLocalHashValid = true;
					}

					PxU32 particlesLeftB = numHaloParticles;
					Particle* particlesSpB = particles + haloRegions.firstParticle[haloRegionIdx];

					while(particlesLeftB)
					{
						PxU32 numParticlesSpB =
						    PxMin(particlesLeftB, static_cast<PxU32>(PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY));

						// It is important that no data is written to particles in halo regions since they belong to
						// a neighboring packet. The interaction effect of the current packet on the neighboring packet
						// will be
						// considered when the neighboring packet is processed.

						// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
						const PxU32 numCellHashBucketsSpB = Ps::nextPowerOfTwo(numParticlesSpB + 1);
						PX_ASSERT(numCellHashBucketsSpB <= tempBuffers.cellHashMaxSize);

						// Get local cell hash for other subpacket
						SpatialHash::buildLocalHash(particlesSpB, numParticlesSpB, particleCellsSpB, particleIndicesSpB,
						                            tempBuffers.hashKeys, numCellHashBucketsSpB, mParams.cellSizeInv,
						                            packetCorner);

						// For the cells of subpacket A, find neighboring cells in the subpacket B and compute particle
						// interactions.
						updateCellsSubpacketPair(updateType, forceBufA, NULL, particlesSpA, particlesSpB,
						                         particleCellsSpA, particleCellsSpB, particleIndicesSpA,
						                         particleIndicesSpB, numCellHashBucketsSpA, numCellHashBucketsSpB,
						                         false, mParams, tempBuffers, numParticlesSpA > numParticlesSpB);

						particlesLeftB -= numParticlesSpB;
						particlesSpB += numParticlesSpB;
					}
				}

				// Now process halo regions which don't need local hash building.
				PxU32 mergedIndexCount = 0;
				for(PxU32 h = 0; h < numHaloNeighbors; h++)
				{
					PxU32 haloRegionIdx = neighborHaloRegions.haloRegionIndices[h];
					PxU32 numHaloParticles = haloRegions.numParticles[haloRegionIdx];
					if(numHaloParticles == 0)
						continue;

					bool haloRegionEnablesBruteForce =
					    (numHaloParticles <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION);

					if(!sectionEnablesBruteForce || !haloRegionEnablesBruteForce)
						continue;

					// The section and the halo region do not have enough particles to make it worth
					// building a local cell hash --> use brute force approach

					// This is given by the brute force condition (haloRegionEnablesBruteForce). Its necessary to
					// make sure a halo region alone fits into the merge buffer.
					PX_ASSERT(numHaloParticles <= PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY);

					if(mergedIndexCount + numHaloParticles > PT_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY)
					{
						updateParticleGroupPair(forceBufA, NULL, particlesSpA, particles,
						                        tempBuffers.orderedIndicesSubpacket, numSectionParticles,
						                        tempBuffers.mergedIndices, mergedIndexCount, false,
						                        updateType == SphUpdateType::DENSITY, mParams,
						                        tempBuffers.simdPositionsSubpacket, tempBuffers.indexStream);
						mergedIndexCount = 0;
					}

					PxU32 hpIndex = haloRegions.firstParticle[haloRegionIdx];
					for(PxU32 k = 0; k < numHaloParticles; k++)
						tempBuffers.mergedIndices[mergedIndexCount++] = hpIndex++;
				}

				if(mergedIndexCount > 0)
				{
					updateParticleGroupPair(forceBufA, NULL, particlesSpA, particles, tempBuffers.orderedIndicesSubpacket,
					                        numSectionParticles, tempBuffers.mergedIndices, mergedIndexCount, false,
					                        updateType == SphUpdateType::DENSITY, mParams,
					                        tempBuffers.simdPositionsSubpacket, tempBuffers.indexStream);
				}
#endif // MERGE_HALO_REGIONS

				particlesLeftA -= numParticlesSpA;
				particlesSpA += numParticlesSpA;
				forceBufA += numParticlesSpA;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------//

void Dynamics::updateSubpacketPairHalo(PxVec3* __restrict forceBufA, Particle* __restrict particlesSpA,
                                       PxU32 numParticlesSpA, ParticleCell* __restrict particleCellsSpA,
                                       PxU32* __restrict particleIndicesSpA, bool& isLocalHashSpAValid,
                                       PxU32 numCellHashBucketsSpA, Particle* __restrict particlesSpB,
                                       PxU32 numParticlesSpB, ParticleCell* __restrict particleCellsSpB,
                                       PxU32* __restrict particleIndicesSpB, const PxVec3& packetCorner,
                                       SphUpdateType::Enum updateType, PxU16* __restrict hashKeyArray,
                                       DynamicsTempBuffers& tempBuffers)
{
	bool sectionEnablesBruteForce = (numParticlesSpA <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION);
	bool haloRegionEnablesBruteForce = (numParticlesSpB <= PT_BRUTE_FORCE_PARTICLE_THRESHOLD_HALO_VS_SECTION);

	// It is important that no data is written to particles in halo regions since they belong to
	// a neighboring packet. The interaction effect of the current packet on the neighboring packet will be
	// considered when the neighboring packet is processed.

	if(sectionEnablesBruteForce && haloRegionEnablesBruteForce)
	{
		// Now process halo regions which don't need local hash building.
		// The section and the halo region do not have enough particles to make it worth
		// building a local cell hash --> use brute force approach

		updateParticleGroupPair(forceBufA, NULL, particlesSpA, particlesSpB, tempBuffers.orderedIndicesSubpacket,
		                        numParticlesSpA, tempBuffers.orderedIndicesSubpacket, numParticlesSpB, false,
		                        updateType == SphUpdateType::DENSITY, mParams, tempBuffers.simdPositionsSubpacket,
		                        tempBuffers.indexStream);
	}
	else
	{
		if(!isLocalHashSpAValid)
		{
			// Get local cell hash for the current subpacket
			SpatialHash::buildLocalHash(particlesSpA, numParticlesSpA, particleCellsSpA, particleIndicesSpA,
			                            hashKeyArray, numCellHashBucketsSpA, mParams.cellSizeInv, packetCorner);
			isLocalHashSpAValid = true;
		}

		// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
		const PxU32 numCellHashBucketsSpB = Ps::nextPowerOfTwo(numParticlesSpB + 1);
		PX_ASSERT(numCellHashBucketsSpB <= tempBuffers.cellHashMaxSize);

		// Get local cell hash for other subpacket
		SpatialHash::buildLocalHash(particlesSpB, numParticlesSpB, particleCellsSpB, particleIndicesSpB, hashKeyArray,
		                            numCellHashBucketsSpB, mParams.cellSizeInv, packetCorner);

		// For the cells of subpacket A, find neighboring cells in the subpacket B and compute particle interactions.
		updateCellsSubpacketPair(updateType, forceBufA, NULL, particlesSpA, particlesSpB, particleCellsSpA,
		                         particleCellsSpB, particleIndicesSpA, particleIndicesSpB, numCellHashBucketsSpA,
		                         numCellHashBucketsSpB, false, mParams, tempBuffers, numParticlesSpA < numParticlesSpB);
	}
}
//-------------------------------------------------------------------------------------------------------------------//

#endif // PX_USE_PARTICLE_SYSTEM_API
