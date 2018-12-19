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

#ifndef PT_PARTICLE_OPCODE_CACHE_H
#define PT_PARTICLE_OPCODE_CACHE_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxBounds3.h"
#include "GuGeometryUnion.h"
#include "PtParticleSystemFlags.h"
#include "PsUtilities.h"

namespace physx
{

namespace Pt
{

/**
Represents a per particle opcode cache for collision with meshes.
The cache contains
- number of triangle indices
- mesh pointer
- triangle indices
- bounds representing the volume within which the cache is valid

The cache is always guaranteed to reference ALL triangles that are relevant for a given mesh for a given volume.

There are four different data layouts to optimize access speed for fewer indices and maximize the amount of triangles
that can be cached.
1. regular PxBounds3 with up to 1 x 16 bit triangle indices.
2. compressed volume with up to 6 x 16 bit triangle indices.
3. compressed volume with up to 9 x 10 bit triangle indices. (The indices are compressed if the range of indices
allows).
4. compressed volume with up to 3 x 32 bit triangle indices. (For large meshes).
*/
struct ParticleOpcodeCache
{
	static const PxU32 sMaxCachedTriangles = 9;

	struct QuantizationParams
	{
		PxF32 dequantizationMultiplier;
		PxF32 quantizationMultiplier;
	};

	static PX_FORCE_INLINE QuantizationParams getQuantizationParams(const PxF32 maxExtents)
	{
		QuantizationParams params;
		params.quantizationMultiplier = 254 * (1.0f / maxExtents);
		params.dequantizationMultiplier = (1.0f / 254) * maxExtents;
		return params;
	}

	PX_FORCE_INLINE const Gu::GeometryUnion* getGeometry()
	{
		return mGeom;
	}

	PX_FORCE_INLINE ParticleOpcodeCache& operator=(const ParticleOpcodeCache& p)
	{
		const PxU32* src = reinterpret_cast<const PxU32*>(&p);
		PxU32* dist = reinterpret_cast<PxU32*>(this);
		dist[0] = src[0];
		dist[1] = src[1];
		dist[2] = src[2];
		dist[3] = src[3];
		dist[4] = src[4];
		dist[5] = src[5];
		dist[6] = src[6];
		dist[7] = src[7];

#if PX_P64_FAMILY
		dist[8] = src[8];
		dist[9] = src[9];
#endif
		return *this;
	}

	// set mGeom to a temp mem to store the triangles index
	// init for triangles mesh cache
	PX_FORCE_INLINE void init(PxU32* triangles)
	{
		mTriangleCount = 0;
		mGeom = reinterpret_cast<Gu::GeometryUnion*>(triangles);
	}

	// add triangles
	PX_FORCE_INLINE void add(const PxU32* triangles, const PxU32 numTriangles)
	{
		const PxU32 end = mTriangleCount + numTriangles;
		if(end <= sMaxCachedTriangles)
		{
			PxU32* tmp = const_cast<PxU32*>(reinterpret_cast<const PxU32*>(mGeom));
			for(PxU32 i = mTriangleCount; i < end; i++)
			{
				tmp[i] = *triangles++;
			}
			mTriangleCount = Ps::to8(end);
		}
		else
		{
			PX_COMPILE_TIME_ASSERT(sMaxCachedTriangles < PX_MAX_U8);
			//this result in marking the cache invalid
			mTriangleCount = PX_MAX_U8;
		}
	}

	PX_FORCE_INLINE void write(PxU16& internalParticleFlags, const PxBounds3& bounds,
	                           const QuantizationParams& quantizationParams, const Gu::GeometryUnion& mesh,
	                           const bool isSmallMesh)
	{
		PxU32* triangles = const_cast<PxU32*>(reinterpret_cast<const PxU32*>(mGeom));
		if(isSmallMesh && mTriangleCount <= 1)
		{
			// Layout of mData:
			// PxU8 pad
			// PxU16 index
			// PxBounds3 bounds
			PxU8* ptr = mData + 1;
			reinterpret_cast<PxU16&>(*ptr) = (mTriangleCount > 0) ? static_cast<PxU16>(triangles[0]) : PxU16(0);
			ptr += sizeof(PxU16);
			reinterpret_cast<PxBounds3&>(*ptr) = bounds;
		}
		else
		{
			// Layout of mData:
			// PxU8 extentX, extentY, extentZ
			// PxVec3 center
			// PxU8[12] indexData
			PxU8* ptr = mData;
			PxU8& extentX = *ptr++;
			PxU8& extentY = *ptr++;
			PxU8& extentZ = *ptr++;
			PxVec3& center = reinterpret_cast<PxVec3&>(*ptr);
			ptr += sizeof(PxVec3);
			quantizeBounds(center, extentX, extentY, extentZ, bounds, quantizationParams);

			if(isSmallMesh && mTriangleCount <= 6)
			{
				writeTriangles_6xU16(ptr, triangles, mTriangleCount);
			}
			else if(isSmallMesh && mTriangleCount <= 9)
			{
				bool success = writeTriangles_BaseU16_9xU10(ptr, triangles, mTriangleCount);
				if(!success)
				{
					internalParticleFlags &= ~PxU16(InternalParticleFlag::eGEOM_CACHE_MASK);
					return;
				}
			}
			else if(!isSmallMesh && mTriangleCount <= 3)
			{
				writeTriangles_3xU32(ptr, triangles, mTriangleCount);
			}
			else
			{
				internalParticleFlags &= ~PxU16(InternalParticleFlag::eGEOM_CACHE_MASK);
				return;
			}
		}

		// refresh the cache flags
		internalParticleFlags |= (InternalParticleFlag::eGEOM_CACHE_BIT_0 | InternalParticleFlag::eGEOM_CACHE_BIT_1);
		mGeom = &mesh;
	}

	PX_FORCE_INLINE bool read(PxU16& internalParticleFlags, PxU32& numTriangles, PxU32* triangleBuffer,
	                          const PxBounds3& bounds, const QuantizationParams& quantizationParams,
	                          const Gu::GeometryUnion* mesh, const bool isSmallMesh) const
	{
		// cache bits:
		// (00) -> no read (invalid)
		// (01) -> read
		// (11) -> no read (can't be the case with mGeom == mesh)
		PX_ASSERT(mGeom != mesh || !((internalParticleFlags & InternalParticleFlag::eGEOM_CACHE_BIT_0) != 0 &&
		                             (internalParticleFlags & InternalParticleFlag::eGEOM_CACHE_BIT_1) != 0));

		if((internalParticleFlags & InternalParticleFlag::eGEOM_CACHE_BIT_0) != 0 && mGeom == mesh)
		{
			numTriangles = mTriangleCount;
			if(isSmallMesh && numTriangles <= 1)
			{
				// Layout of mData:
				// PxU8 pad
				// PxU16 index
				// PxBounds3 bounds
				const PxU8* ptr = mData + 1;
				*triangleBuffer = reinterpret_cast<const PxU16&>(*ptr);
				ptr += sizeof(PxU16);
				const PxBounds3& cachedBounds = reinterpret_cast<const PxBounds3&>(*ptr);

				// if (bounds.isInside(cachedBounds)) //sschirm, we should implement the isInside to use fsels as well.
				PxVec3 dMin = (bounds.minimum - cachedBounds.minimum).minimum(PxVec3(0));
				PxVec3 dMax = (cachedBounds.maximum - bounds.maximum).minimum(PxVec3(0));
				PxF32 sum = dMin.x + dMin.y + dMin.z + dMax.x + dMax.y + dMax.z;
				if(sum == 0.0f)
				{
					// refresh the cache bits (11)
					internalParticleFlags |=
					    (InternalParticleFlag::eGEOM_CACHE_BIT_0 | InternalParticleFlag::eGEOM_CACHE_BIT_1);
					return true;
				}
			}
			else
			{
				// Layout of mData:
				// PxU8 extentX, extentY, extentZ
				// PxVec3 center
				// PxU8[12] indexData
				const PxU8* ptr = mData;
				const PxU8 extentX = *ptr++;
				const PxU8 extentY = *ptr++;
				const PxU8 extentZ = *ptr++;
				const PxVec3& center = reinterpret_cast<const PxVec3&>(*ptr);
				ptr += sizeof(PxVec3);
				PX_ASSERT(!bounds.isEmpty());

				// if (bounds.isInside(cachedBounds)) //sschirm, we should implement the isInside to use fsels as well.
				PxVec3 diffMin = bounds.minimum - center;
				PxVec3 diffMax = bounds.maximum - center;
				PxF32 diffx = PxMax(PxAbs(diffMin.x), PxAbs(diffMax.x));
				PxF32 diffy = PxMax(PxAbs(diffMin.y), PxAbs(diffMax.y));
				PxF32 diffz = PxMax(PxAbs(diffMin.z), PxAbs(diffMax.z));
				PxU8 dX = PxU8(diffx * quantizationParams.quantizationMultiplier);
				PxU8 dY = PxU8(diffy * quantizationParams.quantizationMultiplier);
				PxU8 dZ = PxU8(diffz * quantizationParams.quantizationMultiplier);
				if((dX < extentX) && (dY < extentY) && (dZ < extentZ))
				{
					if(isSmallMesh && numTriangles <= 6)
					{
						readTriangles_6xU16(triangleBuffer, ptr, numTriangles);
						// refresh the cache bits (11)
						internalParticleFlags |=
						    (InternalParticleFlag::eGEOM_CACHE_BIT_0 | InternalParticleFlag::eGEOM_CACHE_BIT_1);
						return true;
					}
					else if(isSmallMesh && numTriangles <= 9)
					{
						readTriangles_BaseU16_9xU10(triangleBuffer, ptr, numTriangles);
						// refresh the cache bits (11)
						internalParticleFlags |=
						    (InternalParticleFlag::eGEOM_CACHE_BIT_0 | InternalParticleFlag::eGEOM_CACHE_BIT_1);
						return true;
					}
					else if(!isSmallMesh && numTriangles <= 3)
					{
						readTriangles_3xU32(triangleBuffer, ptr, numTriangles);
						// refresh the cache bits (11)
						internalParticleFlags |=
						    (InternalParticleFlag::eGEOM_CACHE_BIT_0 | InternalParticleFlag::eGEOM_CACHE_BIT_1);
						return true;
					}
				}
			}
		}

		// cache invalid!
		numTriangles = 0;
		return false;
	}

  private:
	PxU8 mTriangleCount;
	PxU8 mData[27];
	const Gu::GeometryUnion* mGeom;

	static PX_FORCE_INLINE void quantizeBounds(PxVec3& center, PxU8& extentX, PxU8& extentY, PxU8& extentZ,
	                                           const PxBounds3& bounds, const QuantizationParams& quantizationParams)
	{
		center = bounds.getCenter();
		if(!bounds.isEmpty())
		{
			PxVec3 extents = bounds.getExtents();
			extentX = PxU8((extents.x * quantizationParams.quantizationMultiplier) + 1);
			extentY = PxU8((extents.y * quantizationParams.quantizationMultiplier) + 1);
			extentZ = PxU8((extents.z * quantizationParams.quantizationMultiplier) + 1);
			PX_ASSERT(extentX != 0 && extentY != 0 && extentZ != 0);
		}
		else
		{
			extentX = 0;
			extentY = 0;
			extentZ = 0;
		}
	}

	static PX_FORCE_INLINE void dequantizeBounds(PxBounds3& bounds, const PxVec3& center, const PxU8 extentX,
	                                             const PxU8 extentY, const PxU8 extentZ,
	                                             const QuantizationParams& quantizationParams)
	{
		PxVec3 extents(extentX * quantizationParams.dequantizationMultiplier,
		               extentY * quantizationParams.dequantizationMultiplier,
		               extentZ * quantizationParams.dequantizationMultiplier);
		bounds = PxBounds3::centerExtents(center, extents);
	}

	static PX_FORCE_INLINE void writeTriangles_6xU16(PxU8* data, const PxU32* triangles, const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles <= 6);
		PxU16* ptr = reinterpret_cast<PxU16*>(data);
		for(PxU32 t = 0; t < numTriangles; ++t)
			*ptr++ = Ps::to16(triangles[t]);
	}

	static PX_FORCE_INLINE bool writeTriangles_BaseU16_9xU10(PxU8* data, const PxU32* triangles, const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles <= 9);

		// check index range
		PxU32 min = 0xffffffff;
		PxU32 max = 0;
		PxU32 minIndex = 0xffffffff;
		for(PxU32 i = 0; i < numTriangles; ++i)
		{
			if(triangles[i] < min)
			{
				min = triangles[i];
				minIndex = i;
			}

			if(triangles[i] > max)
				max = triangles[i];
		}

		PxU32 range = max - min;
		if(range < (1 << 10))
		{
			// copy triangles to subtract base and remove 0 element
			PX_ASSERT(numTriangles > 6 && numTriangles <= 9);
			PxU16 triCopy[12];
			{
				for(PxU32 i = 0; i < numTriangles; ++i)
					triCopy[i] = PxU16(triangles[i] - min);

				PX_ASSERT(triCopy[minIndex] == 0);
				triCopy[minIndex] = triCopy[numTriangles - 1];
			}

			PxU16* buffer = reinterpret_cast<PxU16*>(data);
			buffer[0] = Ps::to16(min);
			buffer[1] = PxU16((triCopy[0] << 6) | (triCopy[1] >> 4));
			buffer[2] = PxU16((triCopy[1] << 12) | (triCopy[2] << 2) | (triCopy[3] >> 8));
			buffer[3] = PxU16((triCopy[3] << 8) | (triCopy[4] >> 2));
			buffer[4] = PxU16((triCopy[4] << 14) | (triCopy[5] << 4) | (triCopy[6] >> 6));
			buffer[5] = PxU16((triCopy[6] << 10));

			// copy rubbish, doesn't hurt since we are reading from large enough buffer
			buffer[5] |= triCopy[7];

			return true;
		}
		return false;
	}

	static PX_FORCE_INLINE void writeTriangles_3xU32(PxU8* data, const PxU32* triangles, const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles <= 3);
		PxU32* ptr = reinterpret_cast<PxU32*>(data);
		for(PxU32 t = 0; t < numTriangles; ++t)
			*ptr++ = triangles[t];
	}

	static PX_FORCE_INLINE void readTriangles_6xU16(PxU32* triangleBuffer, const PxU8* data, const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles <= 6);
		const PxU16* ptr = reinterpret_cast<const PxU16*>(data);
		const PxU16* end = ptr + numTriangles;
		PxU32 dstIndex = 0;
		while(ptr != end)
			triangleBuffer[dstIndex++] = *ptr++;
	}

	static PX_FORCE_INLINE void readTriangles_BaseU16_9xU10(PxU32* triangleBuffer, const PxU8* data,
	                                                        const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles > 6 && numTriangles <= 9);
		PX_UNUSED(numTriangles);

		const PxU16* buffer = reinterpret_cast<const PxU16*>(data);
		PxU32 offset = buffer[0];
		const PxU32 mask = 0xffffffff >> (6 + 16);
		triangleBuffer[0] = offset;
		triangleBuffer[1] = ((PxU32(buffer[1] >> 6)) & mask) + offset;
		triangleBuffer[2] = ((PxU32(buffer[1] << 4) | PxU32(buffer[2] >> 12)) & mask) + offset;
		triangleBuffer[3] = ((PxU32(buffer[2] >> 2)) & mask) + offset;
		triangleBuffer[4] = ((PxU32(buffer[2] << 8) | PxU32(buffer[3] >> 8)) & mask) + offset;
		triangleBuffer[5] = ((PxU32(buffer[3] << 2) | PxU32(buffer[4] >> 14)) & mask) + offset;
		triangleBuffer[6] = ((PxU32(buffer[4] >> 4)) & mask) + offset;

		// we can write the last two, even if they are rubbish.
		triangleBuffer[7] = ((PxU32(buffer[4] << 6) | PxU32(buffer[5] >> 10)) & mask) + offset;
		triangleBuffer[8] = (PxU32(buffer[5]) & mask) + offset;
	}

	static PX_FORCE_INLINE void readTriangles_3xU32(PxU32* triangleBuffer, const PxU8* data, const PxU32 numTriangles)
	{
		PX_ASSERT(numTriangles <= 3);
		const PxU32* ptr = reinterpret_cast<const PxU32*>(data);
		const PxU32* end = ptr + numTriangles;
		PxU32 dstIndex = 0;
		while(ptr != end)
			triangleBuffer[dstIndex++] = *ptr++;
	}
};

PX_COMPILE_TIME_ASSERT(sizeof(PxTriangleMeshGeometryLL*) > 4 || sizeof(ParticleOpcodeCache) == 32);

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_OPCODE_CACHE_H
