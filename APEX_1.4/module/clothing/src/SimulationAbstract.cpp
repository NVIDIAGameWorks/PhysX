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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "ApexDefs.h"

#include "SimulationAbstract.h"

// for NUM_VERTICES_PER_CACHE_BLOCK
#include "ClothingAssetImpl.h"


namespace nvidia
{
namespace clothing
{

void SimulationAbstract::init(uint32_t numVertices, uint32_t numIndices, bool writebackNormals)
{
	sdkNumDeformableVertices = numVertices;
	sdkNumDeformableIndices = numIndices;

	const uint32_t alignedNumVertices = (numVertices + 15) & 0xfffffff0;
	const uint32_t writeBackDataSize = (sizeof(PxVec3) * alignedNumVertices) * (writebackNormals ? 2 : 1);

	PX_ASSERT(sdkWritebackPosition == NULL);
	PX_ASSERT(sdkWritebackNormal == NULL);
	sdkWritebackPosition = (PxVec3*)PX_ALLOC(writeBackDataSize, PX_DEBUG_EXP("SimulationAbstract::writebackData"));
	sdkWritebackNormal = writebackNormals ? sdkWritebackPosition + alignedNumVertices : NULL;

	const uint32_t allocNumVertices = (((numVertices + NUM_VERTICES_PER_CACHE_BLOCK - 1) / NUM_VERTICES_PER_CACHE_BLOCK)) * NUM_VERTICES_PER_CACHE_BLOCK;
	PX_ASSERT(skinnedPhysicsPositions == NULL);
	PX_ASSERT(skinnedPhysicsNormals == NULL);
	skinnedPhysicsPositions = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * allocNumVertices * 2, PX_DEBUG_EXP("SimulationAbstract::skinnedPhysicsPositions"));
	skinnedPhysicsNormals = skinnedPhysicsPositions + allocNumVertices;
}



void SimulationAbstract::initSimulation(const tSimParams& s)
{
	simulation = s;
}


}
} // namespace nvidia

