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

#include "PtParticleData.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxIO.h"
#include "particles/PxParticleCreationData.h"
#include "PxMetaData.h"
#include "PsAlignedMalloc.h"
#include "CmUtils.h"
#include "PtParticle.h"

using namespace physx;
using namespace Cm;
using namespace Pt;
//----------------------------------------------------------------------------//

ParticleData::ParticleData(PxU32 maxParticles, bool perParticleRestOffset)
{
	mOwnMemory = true;
	mMaxParticles = maxParticles;
	mHasRestOffsets = perParticleRestOffset;
	mValidParticleCount = 0;
	mValidParticleRange = 0;
	mWorldBounds = PxBounds3::empty();

	fixupPointers();
	mParticleMap.resizeAndClear(mMaxParticles);

#if PX_CHECKED
	{
		PxU32 numWords = mMaxParticles * sizeof(Particle) >> 2;
		for(PxU32 i = 0; i < numWords; ++i)
			reinterpret_cast<PxU32*>(mParticleBuffer)[i] = 0xDEADBEEF;
	}
#endif
}

//----------------------------------------------------------------------------//

ParticleData::ParticleData(ParticleSystemStateDataDesc& particles, const PxBounds3& bounds)
{
	mOwnMemory = true;
	mMaxParticles = particles.maxParticles;
	mHasRestOffsets = (particles.restOffsets.ptr() != NULL);
	mValidParticleCount = particles.numParticles;
	mValidParticleRange = particles.validParticleRange;
	mWorldBounds = bounds;

	fixupPointers();
	if(particles.bitMap)
		mParticleMap.copy(*particles.bitMap);
	else
		mParticleMap.resizeAndClear(mMaxParticles);

	if(mValidParticleRange > 0)
	{
		for(PxU32 i = 0; i < mValidParticleRange; ++i)
			mParticleBuffer[i].flags.api = PxParticleFlags(0);

		for(PxU32 w = 0; w <= (mValidParticleRange - 1) >> 5; w++)
			for(PxU32 b = mParticleMap.getWords()[w]; b; b &= b - 1)
			{
				PxU32 index = (w << 5 | Ps::lowestSetBit(b));
				Particle& dstParticle = mParticleBuffer[index];
				dstParticle.position = particles.positions[index];
				dstParticle.velocity = particles.velocities[index];
				dstParticle.density = 0.0f;
				dstParticle.flags.low = 0;
				dstParticle.flags.api = PxParticleFlag::eVALID;
			}

		if(mHasRestOffsets)
		{
			PX_ASSERT(mRestOffsetBuffer);
			for(PxU32 w = 0; w <= (mValidParticleRange - 1) >> 5; w++)
				for(PxU32 b = mParticleMap.getWords()[w]; b; b &= b - 1)
				{
					PxU32 index = (w << 5 | Ps::lowestSetBit(b));
					mRestOffsetBuffer[index] = particles.restOffsets[index];
				}
		}
	}
}

//----------------------------------------------------------------------------//

ParticleData::ParticleData(PxU8* address)
{
	PX_ASSERT(address == reinterpret_cast<PxU8*>(this));
	PX_UNUSED(address);
	mOwnMemory = false;
	fixupPointers();
}

//----------------------------------------------------------------------------//

ParticleData::~ParticleData()
{
	Ps::AlignedAllocator<16> align16;

	if(mParticleBuffer)
		align16.deallocate(mParticleBuffer);
}

//----------------------------------------------------------------------------//

void ParticleData::fixupPointers()
{
	PX_ASSERT(size_t(this) % 16 == 0);
	PxU8* address = reinterpret_cast<PxU8*>(this);

	address += getHeaderSize();
	PxU32 bitmapSize = getBitmapSize(mMaxParticles);
	mParticleMap.importData(bitmapSize / 4, reinterpret_cast<PxU32*>(address));
	address += (bitmapSize + 15) & ~15;
	mParticleBuffer = reinterpret_cast<Particle*>(address);
	address += getParticleBufferSize(mMaxParticles);
	mRestOffsetBuffer = mHasRestOffsets ? reinterpret_cast<PxF32*>(address) : NULL;
	address += getRestOffsetBufferSize(mMaxParticles, mHasRestOffsets);
}

//----------------------------------------------------------------------------//

void ParticleData::exportData(PxSerializationContext& stream)
{
	clearSimState();
	stream.alignData(16);
	stream.writeData(this, ParticleData::getTotalSize(mMaxParticles, mHasRestOffsets));
}

void ParticleData::getBinaryMetaData(PxOutputStream& stream)
{
	// define ParticleFlags
	PX_DEF_BIN_METADATA_CLASS(stream, Pt::ParticleFlags)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleFlags, PxU16, api, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleFlags, PxU16, low, 0)

	// define Particle
	PX_DEF_BIN_METADATA_CLASS(stream, Pt::Particle)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::Particle, PxVec3, position, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::Particle, PxReal, density, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::Particle, PxVec3, velocity, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::Particle, Pt::ParticleFlags, flags, 0)

	// define ParticleData
	PX_DEF_BIN_METADATA_VCLASS(stream, Pt::ParticleData)

	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, bool, mOwnMemory, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, PxU32, mMaxParticles, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, bool, mHasRestOffsets, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, PxU32, mValidParticleRange, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, PxU32, mValidParticleCount, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, PxBounds3, mWorldBounds, 0)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, Pt::Particle, mParticleBuffer, PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, PxReal, mRestOffsetBuffer, PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream, Pt::ParticleData, BitMap, mParticleMap, 0)

	// extra data
	PX_DEF_BIN_METADATA_EXTRA_ARRAY(stream, Pt::ParticleData, Pt::Particle, mMaxParticles, 16, 0)
	PX_DEF_BIN_METADATA_EXTRA_ITEMS(stream, Pt::ParticleData, PxReal, mHasRestOffsets, mMaxParticles, 0, 16)
}

//----------------------------------------------------------------------------//

void ParticleData::clearSimState()
{
	if(mValidParticleRange > 0)
	{
		for(PxU32 w = 0; w <= (mValidParticleRange - 1) >> 5; w++)
			for(PxU32 b = mParticleMap.getWords()[w]; b; b &= b - 1)
			{
				PxU32 index = (w << 5 | Ps::lowestSetBit(b));
				Particle& dstParticle = mParticleBuffer[index];
				dstParticle.flags.low = 0;
				dstParticle.density = 0.0f;
			}
	}
}

//----------------------------------------------------------------------------//

void ParticleData::onOriginShift(const PxVec3& shift)
{
	if(mValidParticleRange > 0)
	{
		for(PxU32 w = 0; w <= (mValidParticleRange - 1) >> 5; w++)
			for(PxU32 b = mParticleMap.getWords()[w]; b; b &= b - 1)
			{
				PxU32 index = (w << 5 | Ps::lowestSetBit(b));
				Particle& particle = mParticleBuffer[index];
				particle.position -= shift;
			}
	}

	mWorldBounds.minimum -= shift;
	mWorldBounds.maximum -= shift;
}

//----------------------------------------------------------------------------//

ParticleData* ParticleData::create(ParticleSystemStateDataDesc& particles, const PxBounds3& bounds)
{
	Ps::AlignedAllocator<16, Ps::ReflectionAllocator<ParticleData> > align16;
	PxU32 totalSize = getTotalSize(particles.maxParticles, particles.restOffsets.ptr() != NULL);
	ParticleData* mem = reinterpret_cast<ParticleData*>(align16.allocate(totalSize, __FILE__, __LINE__));
	markSerializedMem(mem, totalSize);
	PX_PLACEMENT_NEW(mem, ParticleData)(particles, bounds);
	return mem;
}

//----------------------------------------------------------------------------//

ParticleData* ParticleData::create(PxU32 maxParticles, bool perParticleRestOffsets)
{
	Ps::AlignedAllocator<16, Ps::ReflectionAllocator<ParticleData> > align16;
	PxU32 totalSize = getTotalSize(maxParticles, perParticleRestOffsets);
	ParticleData* mem = reinterpret_cast<ParticleData*>(align16.allocate(totalSize, __FILE__, __LINE__));
	markSerializedMem(mem, totalSize);
	PX_PLACEMENT_NEW(mem, ParticleData)(maxParticles, perParticleRestOffsets);
	return mem;
}

//----------------------------------------------------------------------------//

ParticleData* ParticleData::create(PxDeserializationContext& context)
{
	ParticleData* mem = context.readExtraData<ParticleData, PX_SERIAL_ALIGN>();
	new (mem) ParticleData(reinterpret_cast<PxU8*>(mem));
	context.readExtraData<PxU8>(getDataSize(mem->getMaxParticles(), mem->getRestOffsetBuffer() != NULL));
	return mem;
}

//----------------------------------------------------------------------------//

void ParticleData::release()
{
	if(!mOwnMemory)
		return;

	Ps::AlignedAllocator<16> align16;
	align16.deallocate(this);
}

//----------------------------------------------------------------------------//

bool ParticleData::addParticlesV(const PxParticleCreationData& creationData)
{
	PX_ASSERT(creationData.numParticles <= mMaxParticles);
	PX_ASSERT(creationData.indexBuffer.ptr() && creationData.positionBuffer.ptr());
	PX_ASSERT((mRestOffsetBuffer != NULL) == (creationData.restOffsetBuffer.ptr() != NULL));

	const PxVec3 zeroVector(0.0f);

	PxStrideIterator<const PxU32> indexIt = creationData.indexBuffer;
	PxStrideIterator<const PxVec3> positionIt = creationData.positionBuffer;
	PxStrideIterator<const PxVec3> velocityIt =
	    creationData.velocityBuffer.ptr() ? creationData.velocityBuffer : PxStrideIterator<const PxVec3>(&zeroVector, 0);

	for(PxU32 i = 0; i < creationData.numParticles; i++)
	{
		const PxU32 particleIndex = *indexIt;
		PX_ASSERT(particleIndex <= mMaxParticles);

		Particle& particle = mParticleBuffer[particleIndex];
		PX_ASSERT(!mParticleMap.test(particleIndex));
		mParticleMap.set(particleIndex);

		if(particleIndex + 1 > mValidParticleRange)
		{
			mValidParticleRange = particleIndex + 1;
		}
		else
		{
			PX_ASSERT(!(particle.flags.api & PxParticleFlag::eVALID));
		}

		particle.position = *positionIt;
		particle.velocity = *velocityIt;
		particle.flags.low = 0;
		particle.flags.api = PxParticleFlag::eVALID;
		particle.density = 0.0f;

		mWorldBounds.include(particle.position);

		positionIt++;
		velocityIt++;
		indexIt++;
	}

	if(mRestOffsetBuffer)
	{
		PxStrideIterator<const PxF32> restOffsetIt = creationData.restOffsetBuffer;
		indexIt = creationData.indexBuffer;

		for(PxU32 i = 0; i < creationData.numParticles; i++)
		{
			const PxU32 particleIndex = *indexIt;
			mRestOffsetBuffer[particleIndex] = *restOffsetIt;
			restOffsetIt++;
			indexIt++;
		}
	}

	mValidParticleCount += creationData.numParticles;
	return true;
}

//----------------------------------------------------------------------------//

void ParticleData::removeParticlesV(PxU32 count, const PxStrideIterator<const PxU32>& indices)
{
	for(PxU32 i = 0; i < count; ++i)
		removeParticle(indices[i]);

	mValidParticleCount -= count;
	mValidParticleRange = (mValidParticleCount > 0) ? mParticleMap.findLast() + 1 : 0;
}

//----------------------------------------------------------------------------//

void ParticleData::removeParticlesV()
{
	Cm::BitMap::Iterator it(mParticleMap);
	for(PxU32 particleIndex = it.getNext(); particleIndex != Cm::BitMap::Iterator::DONE; particleIndex = it.getNext())
		removeParticle(particleIndex);

	mValidParticleCount = 0;
	mValidParticleRange = 0;
	PX_ASSERT(mValidParticleCount == 0);
}

//----------------------------------------------------------------------------//

PxU32 ParticleData::getParticleCountV() const
{
	return mValidParticleCount;
}

//----------------------------------------------------------------------------//

/**
In the non-gpu implementation the full state is always available.
*/
void ParticleData::getParticlesV(ParticleSystemStateDataDesc& particles, bool /*fullState*/, bool) const
{
	PX_ASSERT(mValidParticleCount <= mMaxParticles);

	particles.bitMap = &mParticleMap;
	particles.numParticles = mValidParticleCount;
	particles.maxParticles = mMaxParticles;
	particles.validParticleRange = mValidParticleRange;

	if(mValidParticleCount == 0)
	{
		particles.positions = PxStrideIterator<const PxVec3>();
		particles.velocities = PxStrideIterator<const PxVec3>();
		particles.flags = PxStrideIterator<const ParticleFlags>();
		particles.restOffsets = PxStrideIterator<const PxF32>();
	}
	else
	{
		PX_ASSERT(mParticleBuffer);
		particles.positions = PxStrideIterator<const PxVec3>(&mParticleBuffer->position, sizeof(Particle));
		particles.velocities = PxStrideIterator<const PxVec3>(&mParticleBuffer->velocity, sizeof(Particle));
		particles.flags = PxStrideIterator<const ParticleFlags>(&mParticleBuffer->flags, sizeof(Particle));
		particles.restOffsets =
		    mRestOffsetBuffer ? PxStrideIterator<const PxF32>(mRestOffsetBuffer) : PxStrideIterator<const PxF32>();
	}
}

//----------------------------------------------------------------------------//

void ParticleData::setPositionsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
                                 const PxStrideIterator<const PxVec3>& positions)
{
	PX_ASSERT(indices.ptr() && positions.ptr());

	PxStrideIterator<const PxU32> indexIt(indices);
	PxStrideIterator<const PxVec3> positionIt(positions);

	for(PxU32 i = 0; i != numParticles; ++i)
	{
		PxU32 particleIndex = *indexIt++;
		PX_ASSERT(particleIndex <= mMaxParticles);
		PX_ASSERT(mParticleMap.test(particleIndex));
		Particle& particle = mParticleBuffer[particleIndex];
		particle.position = *positionIt++;
		mWorldBounds.include(particle.position);
	}
}

//----------------------------------------------------------------------------//

void ParticleData::setVelocitiesV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
                                  const PxStrideIterator<const PxVec3>& velocities)
{
	PX_ASSERT(indices.ptr() && velocities.ptr());

	PxStrideIterator<const PxU32> indexIt(indices);
	PxStrideIterator<const PxVec3> velocityIt(velocities);

	for(PxU32 i = 0; i != numParticles; ++i)
	{
		PxU32 particleIndex = *indexIt++;
		PX_ASSERT(particleIndex <= mMaxParticles);
		PX_ASSERT(mParticleMap.test(particleIndex));
		Particle& particle = mParticleBuffer[particleIndex];
		particle.velocity = *velocityIt++;
	}
}

//----------------------------------------------------------------------------//

void ParticleData::setRestOffsetsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
                                   const PxStrideIterator<const PxF32>& restOffsets)
{
	PX_ASSERT(indices.ptr() && restOffsets.ptr());

	PxStrideIterator<const PxU32> indexIt(indices);
	PxStrideIterator<const PxF32> restOffsetIt(restOffsets);

	for(PxU32 i = 0; i != numParticles; ++i)
	{
		PxU32 particleIndex = *indexIt++;
		PX_ASSERT(particleIndex <= mMaxParticles);
		PX_ASSERT(mParticleMap.test(particleIndex));
		mRestOffsetBuffer[particleIndex] = *restOffsetIt++;
	}
}

//----------------------------------------------------------------------------//

void ParticleData::addDeltaVelocitiesV(const Cm::BitMap& bufferMap, const PxVec3* buffer, PxReal multiplier)
{
	Cm::BitMap::Iterator it(bufferMap);
	for(PxU32 particleIndex = it.getNext(); particleIndex != Cm::BitMap::Iterator::DONE; particleIndex = it.getNext())
	{
		PX_ASSERT(mParticleMap.boundedTest(particleIndex));
		mParticleBuffer[particleIndex].velocity += buffer[particleIndex] * multiplier;
	}
}

//----------------------------------------------------------------------------//

PxBounds3 ParticleData::getWorldBoundsV() const
{
	return mWorldBounds;
}

//----------------------------------------------------------------------------//

PxU32 ParticleData::getMaxParticlesV() const
{
	return mMaxParticles;
}

//----------------------------------------------------------------------------//

PX_FORCE_INLINE void ParticleData::removeParticle(PxU32 particleIndex)
{
	PX_ASSERT(particleIndex <= mMaxParticles);

	Particle& particle = mParticleBuffer[particleIndex];
	PX_ASSERT(particle.flags.api & PxParticleFlag::eVALID);
	PX_ASSERT(mParticleMap.test(particleIndex));

#if PX_CHECKED
	for(PxU32 i = 0; i<sizeof(Particle)>> 2; ++i)
		reinterpret_cast<PxU32*>(&particle)[i] = 0xDEADBEEF;
#endif
	particle.flags.api = PxParticleFlags(0);
	mParticleMap.reset(particleIndex);
}

//----------------------------------------------------------------------------//

#endif // PX_USE_PARTICLE_SYSTEM_API
