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

#ifndef PT_PARTICLE_DATA_H
#define PT_PARTICLE_DATA_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtParticleSystemCore.h"
#include "PtParticle.h"

namespace physx
{

namespace Pt
{

class ParticleData : public ParticleSystemState
{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================
	PX_NOCOPY(ParticleData)
  public:
	//---------------------------
	// Implements ParticleSystemState
	virtual bool addParticlesV(const PxParticleCreationData& creationData);
	virtual void removeParticlesV(PxU32 count, const PxStrideIterator<const PxU32>& indices);
	virtual void removeParticlesV();
	virtual PxU32 getParticleCountV() const;
	virtual void getParticlesV(ParticleSystemStateDataDesc& particles, bool fullState, bool devicePtr) const;
	virtual void setPositionsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                           const PxStrideIterator<const PxVec3>& positions);
	virtual void setVelocitiesV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                            const PxStrideIterator<const PxVec3>& velocities);
	virtual void setRestOffsetsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                             const PxStrideIterator<const PxF32>& restOffsets);
	virtual void addDeltaVelocitiesV(const Cm::BitMap& bufferMap, const PxVec3* buffer, PxReal multiplier);

	virtual PxBounds3 getWorldBoundsV() const;
	virtual PxU32 getMaxParticlesV() const;
	//~Implements ParticleSystemState
	//---------------------------

	PX_FORCE_INLINE PxU32 getMaxParticles() const
	{
		return mMaxParticles;
	}
	PX_FORCE_INLINE PxU32 getParticleCount() const
	{
		return mValidParticleCount;
	}
	PX_FORCE_INLINE const Cm::BitMap& getParticleMap() const
	{
		return mParticleMap;
	}
	PX_FORCE_INLINE PxU32 getValidParticleRange() const
	{
		return mValidParticleRange;
	}

	PX_FORCE_INLINE Particle* getParticleBuffer()
	{
		return mParticleBuffer;
	}
	PX_FORCE_INLINE PxF32* getRestOffsetBuffer()
	{
		return mRestOffsetBuffer;
	}
	PX_FORCE_INLINE PxBounds3& getWorldBounds()
	{
		return mWorldBounds;
	}

	// creation with copy
	static ParticleData* create(ParticleSystemStateDataDesc& particles, const PxBounds3& bounds);

	// creation with init
	static ParticleData* create(PxU32 maxParticles, bool perParticleRestOffsets);
	static ParticleData* create(PxDeserializationContext& context);

	// creation from memory

	// release this instance and associated memory
	void release();

	// exports particle state and aggregated data to the binary stream.
	void exportData(PxSerializationContext& stream);
	static void getBinaryMetaData(PxOutputStream& stream);
	// special function to get rid of non transferable state
	void clearSimState();

	void onOriginShift(const PxVec3& shift);

  private:
	// placement serialization
	ParticleData(PxU32 maxParticles, bool perParticleRestOffset);
	ParticleData(ParticleSystemStateDataDesc& particles, const PxBounds3& bounds);

	// inplace deserialization
	ParticleData(PxU8* address);

	virtual ~ParticleData();

	PX_FORCE_INLINE static PxU32 getHeaderSize()
	{
		return (sizeof(ParticleData) + 15) & ~15;
	}
	PX_FORCE_INLINE static PxU32 getParticleBufferSize(PxU32 maxParticles)
	{
		return maxParticles * sizeof(Particle);
	}
	PX_FORCE_INLINE static PxU32 getRestOffsetBufferSize(PxU32 maxParticles, bool perParticleRestOffsets)
	{
		return perParticleRestOffsets ? maxParticles * sizeof(PxF32) : 0;
	}
	PX_FORCE_INLINE static PxU32 getBitmapSize(PxU32 maxParticles)
	{
		return ((maxParticles + 31) >> 5) * 4;
	}

	PX_FORCE_INLINE static PxU32 getTotalSize(PxU32 maxParticles, bool perParticleRestOffsets)
	{
		return getHeaderSize() + getDataSize(maxParticles, perParticleRestOffsets);
	}

	static PxU32 getDataSize(PxU32 maxParticles, bool perParticleRestOffsets)
	{
		PxU32 size = (getBitmapSize(maxParticles) + 15) & ~15;
		size += getParticleBufferSize(maxParticles);
		size += getRestOffsetBufferSize(maxParticles, perParticleRestOffsets);
		return size;
	}

	PX_FORCE_INLINE void removeParticle(PxU32 particleIndex);

	void fixupPointers();

  private:
	// This class is laid out following a strict convention for serialization/deserialization
	bool mOwnMemory;
	PxU32 mMaxParticles;       // Maximal number of particles.
	bool mHasRestOffsets;      // Whether per particle offsets are supported.
	PxU32 mValidParticleRange; // Index range in which valid particles are situated.
	PxU32 mValidParticleCount; // The number of valid particles.
	PxBounds3 mWorldBounds;    // World bounds including all particles.
	Particle* mParticleBuffer; // Main particle data buffer.
	PxF32* mRestOffsetBuffer;  // Per particle rest offsets.
	Cm::BitMap mParticleMap;   // Contains occupancy of all per particle data buffers.
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_DATA_H
