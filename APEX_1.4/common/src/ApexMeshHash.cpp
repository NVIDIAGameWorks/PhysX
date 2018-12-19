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



#include "ApexMeshHash.h"

#include "PxBounds3.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

ApexMeshHash::ApexMeshHash() :
	mHashIndex(NULL)
{
	mHashIndex = (MeshHashRoot*)PX_ALLOC(sizeof(MeshHashRoot) * HashIndexSize, PX_DEBUG_EXP("ApexMeshHash"));
	mTime = 1;
	for (uint32_t i = 0; i < HashIndexSize; i++)
	{
		mHashIndex[i].first = -1;
		mHashIndex[i].timeStamp = 0;
	}
	mSpacing = 0.25f;
	mInvSpacing = 1.0f / mSpacing;
}



ApexMeshHash::~ApexMeshHash()
{
	if (mHashIndex != NULL)
	{
		PX_FREE(mHashIndex);
		mHashIndex = NULL;
	}
}



void ApexMeshHash::setGridSpacing(float spacing)
{
	mSpacing = spacing;
	mInvSpacing = 1.0f / spacing;
	reset();
}



void ApexMeshHash::reset()
{
	mTime++;
	mEntries.clear();
}



void ApexMeshHash::add(const PxBounds3& bounds, uint32_t itemIndex)
{
	int32_t x1, y1, z1;
	int32_t x2, y2, z2;
	cellCoordOf(bounds.minimum, x1, y1, z1);
	cellCoordOf(bounds.maximum, x2, y2, z2);
	MeshHashEntry entry;
	entry.itemIndex = itemIndex;

	for (int32_t x = x1; x <= x2; x++)
	{
		for (int32_t y = y1; y <= y2; y++)
		{
			for (int32_t z = z1; z <= z2; z++)
			{
				uint32_t h = hashFunction(x, y, z);
				MeshHashRoot& r = mHashIndex[h];
				uint32_t n = mEntries.size();
				if (r.timeStamp != mTime || r.first < 0)
				{
					entry.next = -1;
				}
				else
				{
					entry.next = r.first;
				}
				r.first = (int32_t)n;
				r.timeStamp = mTime;
				mEntries.pushBack(entry);
			}
		}
	}
}



void ApexMeshHash::add(const PxVec3& pos, uint32_t itemIndex)
{
	int x, y, z;
	cellCoordOf(pos, x, y, z);
	MeshHashEntry entry;
	entry.itemIndex = itemIndex;

	uint32_t h = hashFunction(x, y, z);
	MeshHashRoot& r = mHashIndex[h];
	uint32_t n = mEntries.size();
	if (r.timeStamp != mTime || r.first < 0)
	{
		entry.next = -1;
	}
	else
	{
		entry.next = r.first;
	}
	r.first = (int32_t)n;
	r.timeStamp = mTime;
	mEntries.pushBack(entry);
}



void ApexMeshHash::query(const PxBounds3& bounds, physx::Array<uint32_t>& itemIndices, int32_t maxIndices)
{
	int32_t x1, y1, z1;
	int32_t x2, y2, z2;
	cellCoordOf(bounds.minimum, x1, y1, z1);
	cellCoordOf(bounds.maximum, x2, y2, z2);
	itemIndices.clear();

	for (int32_t x = x1; x <= x2; x++)
	{
		for (int32_t y = y1; y <= y2; y++)
		{
			for (int32_t z = z1; z <= z2; z++)
			{
				uint32_t h = hashFunction(x, y, z);
				MeshHashRoot& r = mHashIndex[h];
				if (r.timeStamp != mTime)
				{
					continue;
				}
				int32_t i = r.first;
				while (i >= 0)
				{
					MeshHashEntry& entry = mEntries[(uint32_t)i];
					itemIndices.pushBack(entry.itemIndex);
					if (maxIndices >= 0 && itemIndices.size() >= (uint32_t)maxIndices)
					{
						return;
					}
					i = entry.next;
				}
			}
		}
	}
}



void ApexMeshHash::queryUnique(const PxBounds3& bounds, physx::Array<uint32_t>& itemIndices, int32_t maxIndices)
{
	query(bounds, itemIndices, maxIndices);
	compressIndices(itemIndices);
}



void ApexMeshHash::query(const PxVec3& pos, physx::Array<uint32_t>& itemIndices, int32_t maxIndices)
{
	int32_t x, y, z;
	cellCoordOf(pos, x, y, z);
	itemIndices.clear();

	uint32_t h = hashFunction(x, y, z);
	MeshHashRoot& r = mHashIndex[h];
	if (r.timeStamp != mTime)
	{
		return;
	}
	int32_t i = r.first;
	while (i >= 0)
	{
		MeshHashEntry& entry = mEntries[(uint32_t)i];
		itemIndices.pushBack(entry.itemIndex);
		if (maxIndices >= 0 && itemIndices.size() >= (uint32_t)maxIndices)
		{
			return;
		}
		i = entry.next;
	}
}



void ApexMeshHash::queryUnique(const PxVec3& pos, physx::Array<uint32_t>& itemIndices, int32_t maxIndices)
{
	query(pos, itemIndices, maxIndices);
	compressIndices(itemIndices);
}


class U32Less
{
public:
	bool operator()(uint32_t u1, uint32_t u2) const
	{
		return u1 < u2;
	}
};


void ApexMeshHash::compressIndices(physx::Array<uint32_t>& itemIndices)
{
	if (itemIndices.empty())
	{
		return;
	}

	shdfnd::sort(itemIndices.begin(), itemIndices.size(), U32Less());

	// mark duplicates
	uint32_t i = 0;
	while (i < itemIndices.size())
	{
		uint32_t j = i + 1;
		while (j < itemIndices.size() && itemIndices[i] == itemIndices[j])
		{
			itemIndices[j] = (uint32_t) - 1;
			j++;
		}
		i = j;
	}

	// remove duplicates
	i = 0;
	while (i < itemIndices.size())
	{
		if (itemIndices[i] == (uint32_t)-1)
		{
			itemIndices.replaceWithLast(i);
		}
		else
		{
			i++;
		}
	}
}

int32_t ApexMeshHash::getClosestPointNr(const PxVec3* points, uint32_t numPoints, uint32_t pointStride, const PxVec3& pos)
{
	PX_ASSERT(numPoints > 0);
	PxBounds3 queryBounds;
	queryBounds.minimum = pos;
	queryBounds.maximum = pos;
	PX_ASSERT(!queryBounds.isEmpty());
	queryBounds.fattenFast(mSpacing);
	query(queryBounds, mTempIndices);

	// remove false positives due to hash collisions
	uint32_t next = 0;
	for (uint32_t i = 0; i < mTempIndices.size(); i++)
	{
		uint32_t pointNr = mTempIndices[i];
		const PxVec3* p = (PxVec3*)((uint8_t*)points + (pointNr * pointStride));
		if (pointNr < numPoints && queryBounds.contains(*p))
		{
			mTempIndices[next++] = pointNr;
		}
	}
	mTempIndices.resize(next);
	bool fallBack = mTempIndices.size() == 0;
	uint32_t numRes = fallBack ? numPoints : mTempIndices.size();

	float min2 = 0.0f;
	int minNr = -1;
	for (uint32_t j = 0; j < numRes; j++)
	{
		uint32_t k = fallBack ? j : mTempIndices[j];
		const PxVec3* p = (PxVec3*)((uint8_t*)points + (k * pointStride));
		float d2 = (pos - *p).magnitudeSquared();
		if (minNr < 0 || d2 < min2)
		{
			min2 = d2;
			minNr = (int32_t)k;
		}
	}
	return minNr;
}

}
} // end namespace nvidia::apex
