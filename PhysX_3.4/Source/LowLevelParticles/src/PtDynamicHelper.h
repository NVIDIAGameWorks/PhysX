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
#ifndef PT_DYNAMIC_HELPER_H
#define PT_DYNAMIC_HELPER_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtDynamicsKernels.h"
#include "PtSpatialHash.h"
#include "PtDynamicsTempBuffers.h"

namespace physx
{

namespace Pt
{

//-------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void updateParticlesPrePass(const SphUpdateType::Enum updateType, PxVec3* forceBuf, Particle* particles,
                                            PxU32 numParticles, const DynamicsParameters& params)
{
	if(updateType == SphUpdateType::DENSITY)
	{
		for(PxU32 i = 0; i < numParticles; ++i)
		{
			Pt::Particle& particle = particles[i];

			// Initialize particle densities with self density value
			particle.density = params.selfDensity;
			forceBuf[i] = PxVec3(0);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void updateParticlesPostPass(const SphUpdateType::Enum updateType, PxVec3* forceBuf,
                                             Particle* particles, PxU32 numParticles, const DynamicsParameters& params)
{
	if(updateType == SphUpdateType::FORCE)
	{
		for(PxU32 i = 0; i < numParticles; ++i)
		{
			Particle& particle = particles[i];

			forceBuf[i] *= params.scaleToWorld * (1.0f / particle.density);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------//

/*!
Given a cell hash table, find neighboring cells and compute particle interactions.
*/
void updateCellsSubpacket(SphUpdateType::Enum updateType, PxVec3* __restrict forceBuf, Particle* __restrict particles,
                          const ParticleCell* __restrict cells, const PxU32* __restrict particleIndices,
                          const PxU32 numCellHashBuckets, const DynamicsParameters& params,
                          DynamicsTempBuffers& tempBuffers)
{
	PX_ASSERT(particles);
	PX_ASSERT(cells);
	PX_ASSERT(particleIndices);

	const ParticleCell* neighborCells[13];

	for(PxU32 c = 0; c < numCellHashBuckets; c++)
	{
		const ParticleCell& cell = cells[c];

		if(cell.numParticles == PX_INVALID_U32)
			continue;

		GridCellVector coords(cell.coords);

		//
		// To process each pair of neighboring cells only once, a special neighborhood layout can be
		// used. Thus, we do not need to consider all 26 neighbors of a cell but only half of them.
		// Going through the list of cells, a cell X might not be aware of a neighboring cell Y with
		// this layout, however, since cell Y in turn is aware of cell X the pair will still be processed
		// at the end.
		//

		// Complete back plane
		PxU32 cellIdx;

		PxI16 neighbor[13][3] = { { -1, -1, -1 },
			                      { 0, -1, -1 },
			                      { 1, -1, -1 },
			                      { -1, 0, -1 },
			                      { 0, 0, -1 },
			                      { 1, 0, -1 },
			                      { -1, 1, -1 },
			                      { 0, 1, -1 },
			                      { 1, 1, -1 },
			                      { 1, 0, 0 },
			                      { -1, 1, 0 },
			                      { 0, 1, 0 },
			                      { 1, 1, 0 } };

		for(PxU32 n = 0; n < 13; n++)
		{
			neighborCells[n] = SpatialHash::findConstCell(
			    cellIdx, GridCellVector(coords.x + neighbor[n][0], coords.y + neighbor[n][1], coords.z + neighbor[n][2]),
			    cells, numCellHashBuckets);
		}

		// Compute interaction between particles inside the current cell
		// These calls still produce a lot of LHS. Going from two way to one way updates didn't help. TODO, more
		// investigation.
		for(PxU32 p = 1; p < cell.numParticles; p++)
		{
			updateParticleGroupPair(forceBuf, forceBuf, particles, particles,
			                        particleIndices + cell.firstParticle + p - 1, 1,
			                        particleIndices + cell.firstParticle + p, cell.numParticles - p, true,
			                        updateType == SphUpdateType::DENSITY, params, tempBuffers.simdPositionsSubpacket,
			                        tempBuffers.indexStream);
		}

		// Compute interaction between particles of current cell and neighboring cells
		PxU32 srcIndexCount = 0;

		for(PxU32 n = 0; n < 13; n++)
		{
			if(!neighborCells[n])
				continue;

			const ParticleCell* nCell = neighborCells[n];

			for(PxU32 i = nCell->firstParticle, end = nCell->firstParticle + nCell->numParticles; i < end; i++)
				tempBuffers.mergedIndices[srcIndexCount++] = particleIndices[i];
		}

		if(srcIndexCount > 0)
		{
			updateParticleGroupPair(forceBuf, forceBuf, particles, particles, particleIndices + cell.firstParticle,
			                        cell.numParticles, tempBuffers.mergedIndices, srcIndexCount, true,
			                        updateType == SphUpdateType::DENSITY, params, tempBuffers.simdPositionsSubpacket,
			                        tempBuffers.indexStream);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------//

/*!
Given two subpackets, i.e., their cell hash tables and particle arrays, find for each cell of the first subpacket
the neighboring cells within the second subpacket and compute particle interactions for these neighboring cells.
*/
void updateCellsSubpacketPair(SphUpdateType::Enum updateType, PxVec3* __restrict forceBufA, PxVec3* __restrict forceBufB,
                              Particle* __restrict particlesSpA, Particle* __restrict particlesSpB,
                              const ParticleCell* __restrict cellsSpA, const ParticleCell* __restrict cellsSpB,
                              const PxU32* __restrict particleIndicesSpA, const PxU32* __restrict particleIndicesSpB,
                              const PxU32 numCellHashBucketsA, const PxU32 numCellHashBucketsB, bool twoWayUpdate,
                              const DynamicsParameters& params, DynamicsTempBuffers& tempBuffers, bool swapAB)
{
	PX_ASSERT(particlesSpA);
	PX_ASSERT(particlesSpB);
	PX_ASSERT(cellsSpA);
	PX_ASSERT(cellsSpB);
	PX_ASSERT(particleIndicesSpA);
	PX_ASSERT(particleIndicesSpB);

	const ParticleCell* __restrict srcCell;
	const ParticleCell* __restrict dstCell;
	const PxU32* __restrict dstIndices;
	PxU32 srcBuckets, dstBuckets;

	if(swapAB)
	{
		srcCell = cellsSpB;
		srcBuckets = numCellHashBucketsB;

		dstCell = cellsSpA;
		dstIndices = particleIndicesSpA;
		dstBuckets = numCellHashBucketsA;
	}
	else
	{
		srcCell = cellsSpA;
		srcBuckets = numCellHashBucketsA;

		dstCell = cellsSpB;
		dstIndices = particleIndicesSpB;
		dstBuckets = numCellHashBucketsB;
	}

	const ParticleCell* neighborCells[27];

	// For the cells of the subpacket A find neighboring cells in the subpacket B.
	const ParticleCell* pcell_end = srcCell + srcBuckets;
	for(const ParticleCell* pcell = srcCell; pcell < pcell_end; pcell++)
	{
		if(pcell->numParticles != PX_INVALID_U32)
		{
			GridCellVector coords(pcell->coords);

			//
			// Check the 26 neighboring cells plus the cell with the same coordinates but inside the other subpacket
			//

			// Back plane
			PxU32 cellIdx;
			PxI16 neighbor[27][3] = { { -1, -1, -1 },
				                      { 0, -1, -1 },
				                      { 1, -1, -1 },
				                      { -1, 0, -1 },
				                      { 0, 0, -1 },
				                      { 1, 0, -1 },
				                      { -1, 1, -1 },
				                      { 0, 1, -1 },
				                      { 1, 1, -1 },
				                      { -1, -1, 0 },
				                      { 0, -1, 0 },
				                      { 1, -1, 0 },
				                      { -1, 0, 0 },
				                      { 0, 0, 0 },
				                      { 1, 0, 0 },
				                      { -1, 1, 0 },
				                      { 0, 1, 0 },
				                      { 1, 1, 0 },
				                      { -1, -1, 1 },
				                      { 0, -1, 1 },
				                      { 1, -1, 1 },
				                      { -1, 0, 1 },
				                      { 0, 0, 1 },
				                      { 1, 0, 1 },
				                      { -1, 1, 1 },
				                      { 0, 1, 1 },
				                      { 1, 1, 1 } };

			for(PxU32 n = 0; n < 27; n++)
			{
				neighborCells[n] = SpatialHash::findConstCell(
				    cellIdx,
				    GridCellVector(coords.x + neighbor[n][0], coords.y + neighbor[n][1], coords.z + neighbor[n][2]),
				    dstCell, dstBuckets);
			}

			// Compute interaction between particles of current cell and neighboring cells
			PxU32 indexCount = 0;

			for(PxU32 n = 0; n < 27; n++)
			{
				if(!neighborCells[n])
					continue;

				const ParticleCell* nCell = neighborCells[n];

				for(PxU32 i = nCell->firstParticle, end = nCell->firstParticle + nCell->numParticles; i < end; i++)
					tempBuffers.mergedIndices[indexCount++] = dstIndices[i];
			}

			if(indexCount > 0)
			{

				if(swapAB)
				{
					updateParticleGroupPair(forceBufA, forceBufB, particlesSpA, particlesSpB, tempBuffers.mergedIndices,
					                        indexCount, particleIndicesSpB + pcell->firstParticle, pcell->numParticles,
					                        twoWayUpdate, updateType == SphUpdateType::DENSITY, params,
					                        tempBuffers.simdPositionsSubpacket, tempBuffers.indexStream);
				}
				else
				{
					updateParticleGroupPair(forceBufA, forceBufB, particlesSpA, particlesSpB,
					                        particleIndicesSpA + pcell->firstParticle, pcell->numParticles,
					                        tempBuffers.mergedIndices, indexCount, twoWayUpdate,
					                        updateType == SphUpdateType::DENSITY, params,
					                        tempBuffers.simdPositionsSubpacket, tempBuffers.indexStream);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------//

PX_FORCE_INLINE void normalizeParticleDensity(Particle& particle, const PxF32 selfDensity,
                                              const PxF32 densityNormalizationFactor)
{
	// normalize density
	particle.density = (particle.density - selfDensity) * densityNormalizationFactor;
}

//-------------------------------------------------------------------------------------------------------------------//

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_DYNAMIC_HELPER_H
