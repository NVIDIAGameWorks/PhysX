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



#ifndef COOKING_H
#define COOKING_H

#include "CookingAbstract.h"

#include <PxVec3.h>
#include <PsArray.h>

// Tae - 301 -> 302: fiberless cooker change 
#define COOKED_DATA_VERSION 302

namespace nvidia
{
namespace clothing
{

class ClothingCookedPhysX3Param;

class Cooking : public CookingAbstract
{
public:
	Cooking(bool withFibers) : mWithFibers(withFibers) {}

	virtual NvParameterized::Interface* execute();
	static uint32_t getCookingVersion()
	{
		return COOKED_DATA_VERSION;
	}

private:
	ClothingCookedPhysX3Param* trivialCooker(uint32_t subMeshIndex) const;
	ClothingCookedPhysX3Param* fiberCooker(uint32_t subMeshIndex) const;

	void computeUniqueEdges(uint32_t subMeshIndex);
	void refineUniqueEdges(uint32_t physicalMeshIndex);
	void computeVertexWeights(ClothingCookedPhysX3Param* cookedData, uint32_t subMeshIndex) const;
	void createVirtualParticles(ClothingCookedPhysX3Param* cookedData, uint32_t subMeshIndex);
	void createSelfcollisionIndices(ClothingCookedPhysX3Param* cookedData, uint32_t subMeshIndex) const;
	bool verifyValidity(const ClothingCookedPhysX3Param* cookedData, uint32_t subMeshIndex);
	void fillOutSetsDesc(ClothingCookedPhysX3Param* cookedData);
	void groupPhases(ClothingCookedPhysX3Param* cookedData, uint32_t subMeshIndex, uint32_t startIndex, uint32_t endIndex, Array<uint32_t>& phaseEnds) const;

	void dumpObj(const char* filename, uint32_t subMeshIndex) const;
	void dumpApx(const char* filename, const NvParameterized::Interface* data) const;

	bool mWithFibers;

	static bool mTetraWarning;

	struct Edge
	{
		Edge();
		Edge(uint32_t v0, uint32_t v1, uint32_t v2);

		uint32_t vertex0, vertex1;
		uint32_t vertex2, vertex3;
		float maxAngle;
		bool isQuadDiagonal;
		bool isUsed;

		PX_FORCE_INLINE bool operator()(const Edge& e1, const Edge& e2) const
		{
			return e1 < e2;
		}

		PX_FORCE_INLINE bool operator!=(const Edge& other) const
		{
			return vertex0 != other.vertex0 || vertex1 != other.vertex1;
		}
		PX_FORCE_INLINE bool operator==(const Edge& other) const
		{
			return vertex0 == other.vertex0 && vertex1 == other.vertex1;
		}
		PX_FORCE_INLINE bool operator<(const Edge& other) const
		{
			if (vertex0 != other.vertex0)
			{
				return vertex0 < other.vertex0;
			}

			return vertex1 < other.vertex1;
		}

		PX_FORCE_INLINE uint32_t largestIndex() const
		{
			uint32_t largest = PxMax(vertex0, vertex1);
			largest = PxMax(largest, vertex2);
			if (vertex3 != 0xffffffff)
			{
				largest = PxMax(largest, vertex3);
			}
			return largest;
		}
	};

	struct SortHiddenEdges
	{
		SortHiddenEdges(nvidia::Array<Edge>& uniqueEdges) : mUniqueEdges(uniqueEdges) {}

		bool operator()(uint32_t a, uint32_t b) const
		{
			return mUniqueEdges[a].maxAngle < mUniqueEdges[b].maxAngle;
		}

	private:
		SortHiddenEdges& operator=(const SortHiddenEdges&);

		nvidia::Array<Edge>& mUniqueEdges;
	};

	nvidia::Array<Edge> mUniqueEdges;
	uint32_t findUniqueEdge(uint32_t index1, uint32_t index2) const;

	struct VirtualParticle
	{
		VirtualParticle(uint32_t i0, uint32_t i1, uint32_t i2)
		{
			indices[0] = i0;
			indices[1] = i1;
			indices[2] = i2;
			tableIndex = 0;
		}

		void rotate(uint32_t count)
		{
			while (count--)
			{
				const uint32_t temp = indices[2];
				indices[2] = indices[1];
				indices[1] = indices[0];
				indices[0] = temp;
			}
		}

		uint32_t indices[3];
		uint32_t tableIndex;
	};

	struct EdgeAndLength
	{
		EdgeAndLength(uint32_t edgeNumber, float length) : mEdgeNumber(edgeNumber), mLength(length) {}
		uint32_t mEdgeNumber;
		float mLength;

		bool operator<(const EdgeAndLength& other) const
		{
			return mLength < other.mLength;
		}
	};
};

}
}


#endif // COOKING_H
