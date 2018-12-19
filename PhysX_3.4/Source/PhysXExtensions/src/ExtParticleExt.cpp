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


#include "foundation/PxMemory.h"
#include "foundation/PxBounds3.h"

#include "PxParticleExt.h"
#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "PsArray.h"
#include "PsHash.h"
#include "PsBitUtils.h"
#include "PsMathUtils.h"
#include "PsIntrinsics.h"
#include "PsFoundation.h"

using namespace physx;

//----------------------------------------------------------------------------//

static const PxU32 sInvalidIndex = 0xffffffff;

//----------------------------------------------------------------------------//

struct CellCoords
{
	//! Set grid cell coordinates based on a point in space and a scaling factor
	PX_INLINE void set(const PxVec3& realVec, PxReal scale)
	{
		x = static_cast<PxI16>(Ps::floor(realVec.x * scale));
		y = static_cast<PxI16>(Ps::floor(realVec.y * scale));
		z = static_cast<PxI16>(Ps::floor(realVec.z * scale));
	}

	PX_INLINE bool operator==(const CellCoords& v) const
	{
		return ((x == v.x) && (y == v.y) && (z == v.z));
	}

	PxI16 x;
	PxI16 y;
	PxI16 z;
};

//----------------------------------------------------------------------------//

struct Cell
{
	CellCoords coords;
	PxBounds3 aabb;
	PxU32 start;
	PxU32 size;
};

//----------------------------------------------------------------------------//

PxU32 PX_INLINE hashFunction(const CellCoords& coords)
{
	PxU32 mix = PxU32(coords.x) + 101 * PxU32(coords.y) + 7919 * PxU32(coords.z);
	PxU32 hash = Ps::hash(mix);
	return hash;
}

//----------------------------------------------------------------------------//

PxU32 PX_INLINE getEntry(const CellCoords& coords, const PxU32 hashSize, const Cell* cells)
{
	PxU32 hash = hashFunction(coords);
	PxU32 index = hash & (hashSize - 1);
	for (;;)
	{
		const Cell& cell = cells[index];
		if (cell.size == sInvalidIndex || cell.coords == coords)
			break;
		index = (index + 1) & (hashSize - 1);
	}
	return index;
}

//----------------------------------------------------------------------------//

PxU32 PxParticleExt::buildBoundsHash(PxU32* sortedParticleIndices,
									 ParticleBounds* particleBounds,
									 const PxStrideIterator<const PxVec3>& positionBuffer,
									 const PxU32 validParticleRange,
									 const PxU32* validParticleBitmap,
									 const PxU32 hashSize,
									 const PxU32 maxBounds,
									 const PxReal gridSpacing)
{
	// test if hash size is a multiple of 2
	PX_ASSERT((((hashSize - 1) ^ hashSize) + 1) == (2 * hashSize));
	PX_ASSERT(maxBounds <= hashSize);

	PxReal cellSizeInv = 1.0f / gridSpacing;

	Ps::Array<PxU32> particleToCellMap PX_DEBUG_EXP("buildBoundsHashCellMap"); 	
	particleToCellMap.resize(validParticleRange);

	// initialize cells
	Ps::Array<Cell> cells PX_DEBUG_EXP("buildBoundsCells");
	cells.resize(hashSize);
	PxMemSet(cells.begin(), sInvalidIndex, sizeof(Cell) * hashSize);

	// count number of particles per cell
	PxU32 entryCounter = 0;

	if (validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
			for (PxU32 b = validParticleBitmap[w]; b; b &= b-1)
			{
				PxU32 index = (w<<5|Ps::lowestSetBit(b));
				const PxVec3& position = positionBuffer[index];

				PxU32& cellIndex = particleToCellMap[index];
				cellIndex = sInvalidIndex;	// initialize to invalid in case we reach maxBounds	
				if (entryCounter < maxBounds)
				{
					CellCoords particleCoords;
					particleCoords.set(position, cellSizeInv);
					cellIndex = getEntry(particleCoords, hashSize, cells.begin());
					PX_ASSERT(cellIndex != sInvalidIndex);

					Cell& cell = cells[cellIndex];
					if (cell.size == sInvalidIndex)
					{
						// this is the first particle in this cell
						cell.coords = particleCoords;
						cell.aabb = PxBounds3(position, position);
						cell.size = 1;
						++entryCounter;
					}
					else
					{
						// the cell is already occupied
						cell.aabb.include(position);
						++cell.size;
					}
				}
			}
	}


	// accumulate start indices from cell size histogram and write to the user's particleBounds buffer
	PxU32 numBounds = 0;
	for (PxU32 i = 0, counter = 0; i < cells.size(); i++)
	{
		Cell& cell = cells[i];
		if (cell.size != sInvalidIndex)
		{
			cell.start = counter;
			counter += cell.size;
			
			PxParticleExt::ParticleBounds& cellBounds = particleBounds[numBounds++];
			PX_ASSERT(cell.aabb.isValid());
			cellBounds.bounds = cell.aabb;
			cellBounds.firstParticle = cell.start;
			cellBounds.numParticles = cell.size;
			
			cell.size = 0;
		}
	}

	// sort output particle indices by cell
	if (validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
			for (PxU32 b = validParticleBitmap[w]; b; b &= b-1)
			{
				PxU32 index = (w<<5|Ps::lowestSetBit(b));
				PxU32 cellIndex = particleToCellMap[index];
				if (cellIndex != sInvalidIndex)
				{
					Cell& cell = cells[cellIndex];
					PX_ASSERT(cell.start != sInvalidIndex && cell.size != sInvalidIndex);
					sortedParticleIndices[cell.start + cell.size] = index;
					++cell.size;
				}
			}
	}
	
	return numBounds;
}

//----------------------------------------------------------------------------//
	
class InternalIndexPool : public PxParticleExt::IndexPool, public Ps::UserAllocated
{
public:
	InternalIndexPool(PxU32 maxParticles);
	InternalIndexPool(PxU32 maxParticles, PxU32 validParticleRange, const PxU32* validParticleBitmap);
	virtual ~InternalIndexPool() {}

	virtual	PxU32	allocateIndices(PxU32 num, const PxStrideIterator<PxU32>& indexBuffer);
	virtual	void	freeIndices(PxU32 num, const PxStrideIterator<const PxU32>& indexBuffer);
	virtual	void	freeIndices();
	virtual	void	release();
	
private:
	PxU32 mIndexCount;
	Ps::Array<PxU32> mFreeList;
};

InternalIndexPool::InternalIndexPool(PxU32 maxParticles) : mIndexCount(0), mFreeList(PX_DEBUG_EXP("InternalIndexPool:mFreeList")) 
{
	mFreeList.reserve(maxParticles);
}

InternalIndexPool::InternalIndexPool(PxU32 maxParticles, PxU32 validParticleRange, const PxU32* validParticleBitmap) : mIndexCount(0), mFreeList() 
{
	mFreeList.reserve(maxParticles);
	PX_ASSERT(validParticleRange <= maxParticles);
	
	mIndexCount = validParticleRange;
	
	if (validParticleRange > 0)
	{
		//find all the free indices in the valid range (for words fitting into validParticleRangle)
		for (PxU32 w = 0; w < (validParticleRange-1) >> 5; w++)
		{
			for (PxU32 b = ~validParticleBitmap[w]; b; b &= b-1)
			{
				PxU32 invalidIndex = (w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(invalidIndex < validParticleRange);
				mFreeList.pushBack(invalidIndex);
			}
		}

		//for the last word, we need to make sure the index is in the valid range
		{
			PxU32 w = (validParticleRange-1) >> 5;
			for (PxU32 b = ~validParticleBitmap[w]; b; b &= b-1)
			{
				PxU32 invalidIndex = (w<<5|Ps::lowestSetBit(b));
				if (invalidIndex >= validParticleRange)
					break;

				mFreeList.pushBack(invalidIndex);
			}
		}
	}
}

PxU32 InternalIndexPool::allocateIndices(PxU32 num, const PxStrideIterator<PxU32>& indexBuffer)
{
	PxU32 numAllocated = mIndexCount - mFreeList.size();
	PxU32 numFree = mFreeList.capacity() - numAllocated;
	PxU32 numAccepted = PxMin(num, numFree);

	if (numAccepted < num)
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
			"PxParticleExt::IndexPool::allocateIndices: Not all requested indices allocated; maximum reached.");

	PxU32 numAdded = 0;

	PxU32 numToAddFromFreeList = PxMin(numAccepted, mFreeList.size());
	PxU32 numToAddFromFullBlock = numAccepted - numToAddFromFreeList;
	
	//add from free list
	while (numToAddFromFreeList > 0)
	{
		indexBuffer[numAdded++] = mFreeList.popBack();
		numToAddFromFreeList--;
	}
	
	//add from full block
	while (numToAddFromFullBlock > 0)
	{
		indexBuffer[numAdded++] = mIndexCount++;
		numToAddFromFullBlock--;
	}
	
	PX_ASSERT(numAdded == numAccepted);
	return numAccepted;
}

void InternalIndexPool::freeIndices(PxU32 num, const PxStrideIterator<const PxU32>& indexBuffer)
{
	PxU32 numAllocated = mIndexCount - mFreeList.size();
	if (num > numAllocated)
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
			"PxParticleExt::IndexPool::freeIndices: Provided number of indices exceeds number of actually allocated indices. Call faild.");
		return;
	}

#ifdef PX_CHECK
	for (PxU32 i = 0; i < num; ++i)
	{
		if (indexBuffer[i] < mIndexCount)
			continue;
			
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
			"PxParticleExt::IndexPool::freeIndices: Provided indices which where not actually allocated before. Call failed.");
		return;
	}
#endif

	for (PxU32 i = 0; i < num; ++i)
		mFreeList.pushBack(indexBuffer[i]);
}

void InternalIndexPool::freeIndices()
{
	mIndexCount = 0;
	mFreeList.clear();	
}

void InternalIndexPool::release()
{
	PX_DELETE(this);
}

//----------------------------------------------------------------------------//

PxParticleExt::IndexPool* PxParticleExt::createIndexPool(PxU32 maxParticles)
{
	return PX_NEW(InternalIndexPool)(maxParticles);
}

//----------------------------------------------------------------------------//

PxParticleExt::IndexPool* PxParticleExt::createIndexPool(PxU32 maxParticles, PxU32 validParticleRange, const PxU32* validParticleBitmap)
{
	return PX_NEW(InternalIndexPool)(maxParticles, validParticleRange, validParticleBitmap);
}
	
//----------------------------------------------------------------------------//
