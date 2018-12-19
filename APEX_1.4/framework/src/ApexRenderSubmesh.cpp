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



#include "ApexRenderSubmesh.h"

//#include "ApexStream.h"
//#include "ApexSharedSerialization.h"
#include "ApexSDKIntl.h"


namespace nvidia
{
namespace apex
{

PX_INLINE uint32_t findIndexedNeighbors(uint32_t indexedNeighbors[3], uint32_t triangleIndex,
        const uint32_t* indexBuffer, const uint32_t* vertexTriangleRefs, const uint32_t* vertexToTriangleMap)
{
	uint32_t indexedNeighborCount = 0;
	const uint32_t* triangleVertexIndices = indexBuffer + 3 * triangleIndex;
	for (uint32_t v = 0; v < 3; ++v)
	{
		const uint32_t vertexIndex = triangleVertexIndices[v];
		const uint32_t prevVertexIndex = triangleVertexIndices[(3 >> v) ^ 1];
		// Find all other triangles which have this vertex
		const uint32_t mapStart = vertexTriangleRefs[vertexIndex];
		const uint32_t mapStop = vertexTriangleRefs[vertexIndex + 1];
		for (uint32_t i = mapStart; i < mapStop; ++i)
		{
			const uint32_t neighborTriangleIndex = vertexToTriangleMap[i];
			// See if the previous vertex on the triangle matches the next vertex on the neighbor.  (This will
			// automatically exclude the triangle itself, so no check to exclude a self-check is made.)
			const uint32_t* neighborTriangleVertexIndices = indexBuffer + 3 * neighborTriangleIndex;
			const uint8_t indexMatch = (uint8_t)((uint8_t)(neighborTriangleVertexIndices[0] == vertexIndex) |
													     (uint8_t)(neighborTriangleVertexIndices[1] == vertexIndex) << 1 |
													     (uint8_t)(neighborTriangleVertexIndices[2] == vertexIndex) << 2);
			const uint32_t nextNeighborVertexIndex = neighborTriangleVertexIndices[indexMatch & 3];
			if (nextNeighborVertexIndex == prevVertexIndex)
			{
				// Found a neighbor
				indexedNeighbors[indexedNeighborCount++] = neighborTriangleIndex;
			}
		}
	}

	return indexedNeighborCount;
}



void ApexRenderSubmesh::applyPermutation(const Array<uint32_t>& old2new, const Array<uint32_t>& new2old)
{
	if (mParams->vertexPartition.arraySizes[0] == 2)
	{
		mVertexBuffer.applyPermutation(new2old);
	}

	const uint32_t numIndices = (uint32_t)mParams->indexBuffer.arraySizes[0];
	for (uint32_t i = 0; i < numIndices; i++)
	{
		PX_ASSERT(mParams->indexBuffer.buf[i] < old2new.size());
		mParams->indexBuffer.buf[i] = old2new[mParams->indexBuffer.buf[i]];
	}
}



bool ApexRenderSubmesh::createFromParameters(SubmeshParameters* params)
{
	mParams = params;

	if (mParams->vertexBuffer == NULL)
	{
		NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
		mParams->vertexBuffer = traits->createNvParameterized(VertexBufferParameters::staticClassName());
	}
	mVertexBuffer.setParams(static_cast<VertexBufferParameters*>(mParams->vertexBuffer));

	return true;
}



void ApexRenderSubmesh::setParams(SubmeshParameters* submeshParams, VertexBufferParameters* vertexBufferParams)
{

	if (vertexBufferParams == NULL && submeshParams != NULL)
	{
		vertexBufferParams = static_cast<VertexBufferParameters*>(submeshParams->vertexBuffer);
		PX_ASSERT(vertexBufferParams != NULL);
	}
	else if (submeshParams != NULL && submeshParams->vertexBuffer == NULL)
	{
		submeshParams->vertexBuffer = vertexBufferParams;
	}
	else if (mParams == NULL)
	{
		// Only emit this warning if mParams is empty yet (not on destruction of the object)
		APEX_INTERNAL_ERROR("Confliciting parameterized objects!");
	}
	mParams = submeshParams;

	mVertexBuffer.setParams(vertexBufferParams);
}



void ApexRenderSubmesh::addStats(RenderMeshAssetStats& stats) const
{
	stats.vertexCount += mVertexBuffer.getVertexCount();
	stats.indexCount += mParams->indexBuffer.arraySizes[0];

	const uint32_t submeshVertexBytes = mVertexBuffer.getAllocationSize();
	stats.vertexBufferBytes += submeshVertexBytes;
	stats.totalBytes += submeshVertexBytes;

	const uint32_t submeshIndexBytes = mParams->indexBuffer.arraySizes[0] * sizeof(uint32_t);
	stats.indexBufferBytes += submeshIndexBytes;
	stats.totalBytes += submeshIndexBytes;

	stats.totalBytes += mParams->smoothingGroups.arraySizes[0] * sizeof(uint32_t);
}



void ApexRenderSubmesh::buildVertexBuffer(const VertexFormat& format, uint32_t vertexCount)
{
	mVertexBuffer.build(format, vertexCount);
}


} // namespace apex
} // namespace nvidia
