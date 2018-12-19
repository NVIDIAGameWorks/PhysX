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

#pragma once

#ifndef CU_SOLVER_KERNEL_CU
#error include CuSelfCollision.h only from CuSolverKernel.cu
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif

namespace
{
#if __CUDA_ARCH__ >= 300
template <int>
__device__ void scanWarp(Pointer<Shared, int32_t> counts)
{
	asm volatile("{"
	             "	.reg .s32 tmp;"
	             "	.reg .pred p;"
	             "	shfl.up.b32 tmp|p, %0, 0x01, 0x0;"
	             "@p add.s32 %0, tmp, %0;"
	             "	shfl.up.b32 tmp|p, %0, 0x02, 0x0;"
	             "@p add.s32 %0, tmp, %0;"
	             "	shfl.up.b32 tmp|p, %0, 0x04, 0x0;"
	             "@p add.s32 %0, tmp, %0;"
	             "	shfl.up.b32 tmp|p, %0, 0x08, 0x0;"
	             "@p add.s32 %0, tmp, %0;"
	             "	shfl.up.b32 tmp|p, %0, 0x10, 0x0;"
	             "@p add.s32 %0, tmp, %0;"
	             "}"
	             : "+r"(*generic(counts))
	             :);
}
#else
template <int stride>
__device__ void scanWarp(Pointer<Shared, int32_t> counts)
{
	volatile int32_t* ptr = generic(counts);
	const int32_t laneIdx = threadIdx.x & warpSize - 1;
	if(laneIdx >= 1)
		*ptr += ptr[-stride];
	if(laneIdx >= 2)
		*ptr += ptr[-2 * stride];
	if(laneIdx >= 4)
		*ptr += ptr[-4 * stride];
	if(laneIdx >= 8)
		*ptr += ptr[-8 * stride];
	if(laneIdx >= 16)
		*ptr += ptr[-16 * stride];
}
#endif

// sorts array by upper 16bits
// [keys] must be at least 2*n in length, in/out in first n elements
// [histogram] must be at least 34*16 = 544 in length
__device__ void radixSort(int32_t* keys, int32_t n, Pointer<Shared, int32_t> histogram)
{
	const int32_t numWarps = blockDim.x >> 5;
	const int32_t warpIdx = threadIdx.x >> 5;
	const int32_t laneIdx = threadIdx.x & warpSize - 1;

	const uint32_t laneMask = (1u << laneIdx) - 1;
	const uint32_t mask1 = (threadIdx.x & 1) - 1;
	const uint32_t mask2 = !!(threadIdx.x & 2) - 1;
	const uint32_t mask4 = !!(threadIdx.x & 4) - 1;
	const uint32_t mask8 = !!(threadIdx.x & 8) - 1;

	const int32_t tn = (n + blockDim.x - 1) / blockDim.x;
	const int32_t startIndex = tn * (threadIdx.x - laneIdx) + laneIdx;
	const int32_t endIndex = min(startIndex + tn * warpSize, n + 31 & ~31); // full warps for ballot

	int32_t* srcKeys = keys;
	int32_t* dstKeys = keys + n;

	Pointer<Shared, int32_t> hIt = histogram + 16 * warpIdx;
	Pointer<Shared, int32_t> pIt = histogram + 16 * laneIdx + 16;
	Pointer<Shared, int32_t> tIt = histogram + 16 * numWarps + laneIdx;

	for(int32_t p = 16; p < 32; p += 4) // radix passes (4 bits each)
	{
		// gather bucket histograms per warp
		int32_t warpCount = 0;
		for(int32_t i = startIndex; i < endIndex; i += 32)
		{
			int32_t key = i < n ? srcKeys[i] >> p : 15;
			uint32_t ballot1 = __ballot(key & 1);
			uint32_t ballot2 = __ballot(key & 2);
			uint32_t ballot4 = __ballot(key & 4);
			uint32_t ballot8 = __ballot(key & 8);
			warpCount += __popc((mask1 ^ ballot1) & (mask2 ^ ballot2) & (mask4 ^ ballot4) & (mask8 ^ ballot8));
		}

		if(laneIdx >= 16)
			hIt[laneIdx] = warpCount;

		__syncthreads();

		// prefix sum of histogram buckets
		for(int32_t i = warpIdx; i < 16; i += numWarps)
			scanWarp<16>(pIt + i);

		__syncthreads();

		// prefix sum of bucket totals (exclusive)
		if(threadIdx.x < 16)
		{
			*tIt = tIt[-1] & !threadIdx.x - 1;
			scanWarp<1>(tIt);
			hIt[threadIdx.x] = 0;
		}

		__syncthreads();

		if(laneIdx < 16)
			hIt[laneIdx] += *tIt;

		// split indices
		for(int32_t i = startIndex; i < endIndex; i += 32)
		{
			int32_t key = i < n ? srcKeys[i] >> p : 15;
			uint32_t ballot1 = __ballot(key & 1);
			uint32_t ballot2 = __ballot(key & 2);
			uint32_t ballot4 = __ballot(key & 4);
			uint32_t ballot8 = __ballot(key & 8);
			uint32_t bits = ((key & 1) - 1 ^ ballot1) & (!!(key & 2) - 1 ^ ballot2) & (!!(key & 4) - 1 ^ ballot4) &
			                (!!(key & 8) - 1 ^ ballot8);
			int32_t index = hIt[key & 15] + __popc(bits & laneMask);

			if(i < n)
				dstKeys[index] = srcKeys[i];

			if(laneIdx < 16)
				hIt[laneIdx] += __popc((mask1 ^ ballot1) & (mask2 ^ ballot2) & (mask4 ^ ballot4) & (mask8 ^ ballot8));
		}

		__syncthreads();

		::swap(srcKeys, dstKeys);
	}

#ifndef NDEBUG
	for(int32_t i = threadIdx.x; i < n; i += blockDim.x)
		assert(!i || keys[i - 1] >> 16 <= keys[i] >> 16);
#endif
}
}

namespace
{
struct CuSelfCollision
{
	template <typename CurrentT>
	__device__ void operator()(CurrentT& current);

  private:
	template <typename CurrentT>
	__device__ void buildAcceleration(const CurrentT& current);
	template <bool useRestPositions, typename CurrentT>
	__device__ void collideParticles(CurrentT& current) const;

  public:
	float mPosBias[3];
	float mPosScale[3];
	const float* mPosPtr[3];
};
}

__shared__ uninitialized<CuSelfCollision> gSelfCollideParticles;

template <typename CurrentT>
__device__ void CuSelfCollision::operator()(CurrentT& current)
{
	if(min(gClothData.mSelfCollisionDistance, gFrameData.mSelfCollisionStiffness) <= 0.0f)
		return;

	if(threadIdx.x < 3)
	{
		float upper = gFrameData.mParticleBounds[threadIdx.x * 2];
		float negativeLower = gFrameData.mParticleBounds[threadIdx.x * 2 + 1];

		// expand bounds
		float eps = (upper + negativeLower) * 1e-4f;
		float expandedUpper = upper + eps;
		float expandedNegativeLower = negativeLower + eps;
		float expandedEdgeLength = expandedUpper + expandedNegativeLower;

		float* edgeLength = mPosBias; // use as temp
		edgeLength[threadIdx.x] = expandedEdgeLength;

		__threadfence_block();

		// calculate shortest axis
		int32_t shortestAxis = edgeLength[0] > edgeLength[1];
		if(edgeLength[shortestAxis] > edgeLength[2])
			shortestAxis = 2;

		uint32_t writeAxis = threadIdx.x - shortestAxis;
		writeAxis += writeAxis >> 30;

		float maxInvCellSize = __fdividef(127.0f, expandedEdgeLength);
		float invCollisionDistance = __fdividef(1.0f, gClothData.mSelfCollisionDistance);
		float invCellSize = min(maxInvCellSize, invCollisionDistance);

		mPosScale[writeAxis] = invCellSize;
		mPosBias[writeAxis] = invCellSize * expandedNegativeLower;
		mPosPtr[writeAxis] = generic(current[threadIdx.x]);
	}

	__syncthreads();

	buildAcceleration(current);

	if(gFrameData.mRestPositions)
		collideParticles<true>(current);
	else
		collideParticles<false>(current);
}

template <typename CurrentT>
__device__ void CuSelfCollision::buildAcceleration(const CurrentT& current)
{
	int32_t numIndices = gClothData.mNumSelfCollisionIndices;
	const int32_t* indices = reinterpret_cast<const int32_t*>(gClothData.mSelfCollisionIndices);
	int32_t* sortedKeys = reinterpret_cast<int32_t*>(gClothData.mSelfCollisionKeys);
	int16_t* cellStart = reinterpret_cast<int16_t*>(gClothData.mSelfCollisionCellStart);

	typedef typename CurrentT::ConstPointerType ConstPointerType;
	ConstPointerType rowPtr = ConstPointerType(mPosPtr[1]);
	ConstPointerType colPtr = ConstPointerType(mPosPtr[2]);

	float rowScale = mPosScale[1], rowBias = mPosBias[1];
	float colScale = mPosScale[2], colBias = mPosBias[2];

	// calculate keys
	for(int32_t i = threadIdx.x; i < numIndices; i += blockDim.x)
	{
		int32_t index = indices ? indices[i] : i;
		assert(index < gClothData.mNumParticles);

		int32_t rowIndex = int32_t(max(0.0f, min(rowPtr[index] * rowScale + rowBias, 127.5f)));
		int32_t colIndex = int32_t(max(0.0f, min(colPtr[index] * colScale + colBias, 127.5f)));
		assert(rowIndex >= 0 && rowIndex < 128 && colIndex >= 0 && colIndex < 128);

		int32_t key = (colIndex << 7 | rowIndex) + 129; // + row and column sentinel
		assert(key <= 0x4080);

		sortedKeys[i] = key << 16 | index; // (key, index) pair in a single int32_t
	}
	__syncthreads();

	// get scratch shared mem buffer used for radix sort(histogram)
	Pointer<Shared, int32_t> buffer =
	    reinterpret_cast<Pointer<Shared, int32_t> const&>(gCollideParticles.get().mCurData.mSphereX);

	// sort keys (__synchthreads inside radix sort)
	radixSort(sortedKeys, numIndices, buffer);

	// mark cell start if keys are different between neighboring threads
	for(int32_t i = threadIdx.x; i < numIndices; i += blockDim.x)
	{
		int32_t key = sortedKeys[i] >> 16;
		int32_t prevKey = i ? sortedKeys[i - 1] >> 16 : key - 1;
		if(key != prevKey)
		{
			cellStart[key] = i;
			cellStart[prevKey + 1] = i;
		}
	}
	__syncthreads();
}

template <bool useRestPositions, typename CurrentT>
__device__ void CuSelfCollision::collideParticles(CurrentT& current) const
{
	const int32_t* sortedKeys = reinterpret_cast<const int32_t*>(gClothData.mSelfCollisionKeys);
	float* sortedParticles = gClothData.mSelfCollisionParticles;
	int16_t* cellStart = reinterpret_cast<int16_t*>(gClothData.mSelfCollisionCellStart);

	const float cdist = gClothData.mSelfCollisionDistance;
	const float cdistSq = cdist * cdist;

	const int32_t numIndices = gClothData.mNumSelfCollisionIndices;
	const int32_t numParticles = gClothData.mNumParticles;

	// point to particle copied in device memory that is being updated
	float* xPtr = sortedParticles;
	float* yPtr = sortedParticles + numParticles;
	float* zPtr = sortedParticles + 2 * numParticles;
	float* wPtr = sortedParticles + 3 * numParticles;

	// copy current particles to temporary array
	for(int32_t i = threadIdx.x; i < numParticles; i += blockDim.x)
	{
		xPtr[i] = current(i, 0);
		yPtr[i] = current(i, 1);
		zPtr[i] = current(i, 2);
		wPtr[i] = current(i, 3);
	}
	__syncthreads();

	// copy only sorted (indexed) particles to shared mem
	for(int32_t i = threadIdx.x; i < numIndices; i += blockDim.x)
	{
		int32_t index = sortedKeys[i] & UINT16_MAX;
		current(i, 0) = xPtr[index];
		current(i, 1) = yPtr[index];
		current(i, 2) = zPtr[index];
		current(i, 3) = wPtr[index];
	}
	__syncthreads();

	typedef typename CurrentT::ConstPointerType ConstPointerType;
	ConstPointerType rowPtr = ConstPointerType(mPosPtr[1]);
	ConstPointerType colPtr = ConstPointerType(mPosPtr[2]);

	float rowScale = mPosScale[1], rowBias = mPosBias[1];
	float colScale = mPosScale[2], colBias = mPosBias[2];

	for(int32_t i = threadIdx.x; i < numIndices; i += blockDim.x)
	{
		const int32_t index = sortedKeys[i] & UINT16_MAX;
		assert(index < gClothData.mNumParticles);

		float restX, restY, restZ;
		if(useRestPositions)
		{
			const float* restIt = gFrameData.mRestPositions + index * 4;
			restX = restIt[0];
			restY = restIt[1];
			restZ = restIt[2];
		}

		float posX = current(i, 0);
		float posY = current(i, 1);
		float posZ = current(i, 2);
		float posW = current(i, 3);

		float deltaX = 0.0f;
		float deltaY = 0.0f;
		float deltaZ = 0.0f;
		float deltaW = FLT_EPSILON;

		// get cell index for this particle
		int32_t rowIndex = int32_t(max(0.0f, min(rowPtr[i] * rowScale + rowBias, 127.5f)));
		int32_t colIndex = int32_t(max(0.0f, min(colPtr[i] * colScale + colBias, 127.5f)));
		assert(rowIndex >= 0 && rowIndex < 128 && colIndex >= 0 && colIndex < 128);

		int32_t key = colIndex << 7 | rowIndex;
		assert(key <= 0x4080);

		// check cells in 3 columns
		for(int32_t keyEnd = key + 256; key <= keyEnd; key += 128)
		{
			// cellStart keys of unoccupied cells have a value of -1
			uint32_t startIndex; // min<unsigned>(cellStart[key+0..2])
			uint32_t endIndex;   // max<signed>(0, cellStart[key+1..3])

			asm volatile("{\n\t"
			             "	.reg .u32 start1, start2;\n\t"
			             "	ld.global.s16 %1, [%2+6];\n\t"
			             "	ld.global.s16 %0, [%2+0];\n\t"
			             "	ld.global.s16 start1, [%2+2];\n\t"
			             "	ld.global.s16 start2, [%2+4];\n\t"
			             "	max.s32 %1, %1, 0;\n\t"
			             "	min.u32 %0, %0, start1;\n\t"
			             "	max.s32 %1, %1, start1;\n\t"
			             "	min.u32 %0, %0, start2;\n\t"
			             "	max.s32 %1, %1, start2;\n\t"
			             "}\n\t"
			             : "=r"(startIndex), "=r"(endIndex)
			             : POINTER_CONSTRAINT(cellStart + key));

			// comparison must be unsigned to skip cells with negative startIndex
			for(uint32_t j = startIndex; j < endIndex; ++j)
			{
				if(j != i) // avoid same particle
				{
					float dx = posX - current(j, 0);
					float dy = posY - current(j, 1);
					float dz = posZ - current(j, 2);

					float distSqr = dx * dx + dy * dy + dz * dz;
					if(distSqr > cdistSq)
						continue;

					if(useRestPositions)
					{
						const int32_t jndex = sortedKeys[j] & UINT16_MAX;
						assert(jndex < gClothData.mNumParticles);

						// calculate distance in rest configuration
						const float* restJt = gFrameData.mRestPositions + jndex * 4;
						float rx = restX - restJt[0];
						float ry = restY - restJt[1];
						float rz = restZ - restJt[2];

						if(rx * rx + ry * ry + rz * rz <= cdistSq)
							continue;
					}

					// premultiply ratio for weighted average
					float ratio = fmaxf(0.0f, cdist * rsqrtf(FLT_EPSILON + distSqr) - 1.0f);
					float scale = __fdividef(ratio * ratio, FLT_EPSILON + posW + current(j, 3));

					deltaX += scale * dx;
					deltaY += scale * dy;
					deltaZ += scale * dz;
					deltaW += ratio;
				}
			}
		}

		const float stiffness = gFrameData.mSelfCollisionStiffness * posW;
		float scale = __fdividef(stiffness, deltaW);

		// apply collision impulse
		xPtr[index] += deltaX * scale;
		yPtr[index] += deltaY * scale;
		zPtr[index] += deltaZ * scale;

		assert(!isnan(xPtr[index] + yPtr[index] + zPtr[index]));
	}
	__syncthreads();

	// copy temporary particle array back to shared mem
	// (need to copy whole array)
	for(int32_t i = threadIdx.x; i < numParticles; i += blockDim.x)
	{
		current(i, 0) = xPtr[i];
		current(i, 1) = yPtr[i];
		current(i, 2) = zPtr[i];
		current(i, 3) = wPtr[i];
	}

	// unmark occupied cells to empty again (faster than clearing all the cells)
	for(int32_t i = threadIdx.x; i < numIndices; i += blockDim.x)
	{
		int32_t key = sortedKeys[i] >> 16;
		cellStart[key] = 0xffff;
		cellStart[key + 1] = 0xffff;
	}
	__syncthreads();
}
