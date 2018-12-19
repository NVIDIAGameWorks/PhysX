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

#ifndef PT_SPATIAL_HASH_H
#define PT_SPATIAL_HASH_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "CmBitMap.h"
#include "PtConfig.h"
#include "PtParticleCell.h"
#include "PtPacketSections.h"
#include "PtSpatialHashHelper.h"
#include "PtParticle.h"

namespace physx
{

class PxBaseTask;

namespace Pt
{

struct Particle;

// Structure describing the regions around a packet which are relevant for particle interactions.
// A packet has 26 neighbor packets:
// - 6 of these neighbors share a face with the packet. Each of these neighbors provide 9 halo regions.
// - 12 of these neighbors share an edge with the packet. Each of these neighbors provides 3 halo region.
// - 8 of these neighbors share a corner with the packet. Each of these neighbors provide 1 halo region.
//
// -> Number of halo regions for a packet:  6*9 + 12*3 + 8*1 = 98
#define PT_PACKET_HALO_REGIONS 98
struct PacketHaloRegions
{
	PxU32 numParticles[PT_PACKET_HALO_REGIONS];  //! Number of particles in each halo region
	PxU32 firstParticle[PT_PACKET_HALO_REGIONS]; //! Start index of the associated particle interval for each halo
	//! region
	PxU32 maxNumParticles; //! Maximum number of particles among all halo regions
};

// Structure to cache a local cell hash that was computed for a set of particles
struct LocalCellHash
{
	PxU32 numParticles;        // Number of particles the cell hash is based on
	PxU32* particleIndices;    // Particle indices (0..numParticles) with respect to the particle array that was used
	                           // to build the cell hash. Indices are ordered according to cells.
	PxU32 numHashEntries;      // Size of cell hash table
	ParticleCell* hashEntries; // Hash entry for cells

	bool isHashValid; // Marks whether the hash contains valid data or needs to be computed

	LocalCellHash()
	{
		numParticles = 0;
		particleIndices = NULL;
		numHashEntries = 0;
		hashEntries = NULL;
		isHashValid = false;
	}
};

class SpatialHash
{
  public:
	SpatialHash(PxU32 numHashBuckets, PxF32 cellSizeInv, PxU32 packetMultLog, bool supportSections);
	~SpatialHash();

	static PX_FORCE_INLINE ParticleCell* findCell(PxU32& cellIndex, const GridCellVector& coord, ParticleCell* cells,
	                                              PxU32 numHashBuckets);
	static PX_FORCE_INLINE const ParticleCell* findConstCell(PxU32& cellIndex, const GridCellVector& coord,
	                                                         const ParticleCell* cells, PxU32 numHashBuckets);

	PX_FORCE_INLINE PxF32 getCellSizeInv()
	{
		return mCellSizeInv;
	}
	PX_FORCE_INLINE PxU32 getPacketMultLog()
	{
		return mPacketMultLog;
	}

	PX_FORCE_INLINE PxU32 getNumPackets() const
	{
		return mNumCells;
	}
	PX_FORCE_INLINE const ParticleCell* getPackets()
	{
		return mCells;
	}
	PX_FORCE_INLINE const PacketSections* getPacketSections()
	{
		return mPacketSections;
	}

	PX_FORCE_INLINE const ParticleCell* findCell(PxU32& cellIndex, const GridCellVector& coord);
	PX_FORCE_INLINE ParticleCell* getCell(PxU32& cellIndex, const GridCellVector& coord);

	/*!
	Given the coordinates of a specific packet, the packet table, the packet sections and the packet table
	size, this function builds the halo region structure for the packet. The halo region specifies the relevant
	particles of neighboring packets.
	*/
	static void getHaloRegions(PacketHaloRegions& packetHalo, const GridCellVector& packetCoords,
	                           const ParticleCell* packets, const PacketSections* packetSections, PxU32 numHashBuckets);

	/*!
	Build local hash table for cells within a packet. Reorders a particle index array according to particle cells.

	The cell entry array must have more entries than the number of particles passed. The particle index
	table must have the size of the number of particles passed. The particle array is not declared const
	because hash keys might get stored temporarily in the particles.
	*/
	static void buildLocalHash(const Particle* particles, PxU32 numParticles, ParticleCell* cells, PxU32* particleIndices,
	                           PxU16* hashKeyArray, PxU32 numHashBuckets, PxF32 cellSizeInv, const PxVec3& packetCorner);

	/*!
	Builds the packet hash and reorders particle indices to packets. Particles are not declared const since
	each particle hash key  and cell gets precomputed.
	*/
	void updatePacketHash(PxU32& numSorted, PxU32* sortedIndices, Particle* particles, const Cm::BitMap& particleMap,
	                      const PxU32 validParticleRange, physx::PxBaseTask* continuation);

	/*!
	Divides each fluid packet into sections and reorders particle indices according to sections.
	Input particles are not declared const since for each particle the section index gets precomputed.
	*/
	void updatePacketSections(PxU32* particleIndices, Particle* particles, physx::PxBaseTask* continuation);

  private:
	static void reorderParticleIndicesToCells(const Particle* particles, PxU32 numParticles, ParticleCell* cells,
	                                          PxU32* particleIndices, PxU32 numHashBuckets, PxU16* hashKeyArray);

	void reorderParticleIndicesToPackets(PxU32* sortedIndices, PxU32 numHashedParticles, const Cm::BitMap& particleMap,
	                                     PxU16* hashKeyArray);

	/*!
	Splits the specified packet into 26 boundary sections (plus one inner section) and reorders the particles
	according to sections.
	*/
	void buildPacketSections(const ParticleCell& packet, PacketSections& sections, PxU32 packetMultLog,
	                         Particle* particles, PxU32* particleIndices);

	void reorderParticlesToPacketSections(const ParticleCell& packet, PacketSections& sections,
	                                      const Particle* particles, const PxU32* inParticleIndices,
	                                      PxU32* outParticleIndices, PxU16* sectionIndexBuf);

  private:
	ParticleCell* mCells;
	PxU32 mNumCells;
	PxU32 mNumHashBuckets;
	PxF32 mCellSizeInv;

	// Packet Hash data
	PxU32 mPacketMultLog;
	PacketSections* mPacketSections;
};

PX_FORCE_INLINE const ParticleCell* SpatialHash::findConstCell(PxU32& cellIndex, const GridCellVector& coord,
                                                               const ParticleCell* cells, PxU32 numHashBuckets)
{
	cellIndex = getCellIndex(coord, cells, numHashBuckets);
	const ParticleCell* cell = &cells[cellIndex];

	if(cell->numParticles == PX_INVALID_U32)
		return NULL;
	else
		return cell;
}

PX_FORCE_INLINE ParticleCell* SpatialHash::findCell(PxU32& cellIndex, const GridCellVector& coord, ParticleCell* cells,
                                                    PxU32 numHashBuckets)
{
	const ParticleCell* constCell = findConstCell(cellIndex, coord, cells, numHashBuckets);
	return const_cast<ParticleCell*>(constCell);
}

PX_FORCE_INLINE const ParticleCell* SpatialHash::findCell(PxU32& cellIndex, const GridCellVector& coord)
{
	return findCell(cellIndex, coord, mCells, mNumHashBuckets);
}

PX_FORCE_INLINE ParticleCell* SpatialHash::getCell(PxU32& cellIndex, const GridCellVector& coord)
{
	cellIndex = getCellIndex(coord, mCells, mNumHashBuckets);
	return &mCells[cellIndex];
}

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_SPATIAL_HASH_H
