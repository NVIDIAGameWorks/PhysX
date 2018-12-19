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



#ifndef APEX_MESH_HASH_H
#define APEX_MESH_HASH_H

#include "ApexDefs.h"

#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"
#include "PsArray.h"

#include "PxVec3.h"

namespace nvidia
{
namespace apex
{

struct MeshHashRoot
{
	int32_t first;
	uint32_t timeStamp;
};

struct MeshHashEntry
{
	int32_t next;
	uint32_t itemIndex;
};


class ApexMeshHash : public UserAllocated
{
public:
	ApexMeshHash();
	~ApexMeshHash();

	void   setGridSpacing(float spacing);
	float getGridSpacing()
	{
		return 1.0f / mInvSpacing;
	}
	void reset();
	void add(const PxBounds3& bounds, uint32_t itemIndex);
	void add(const PxVec3& pos, uint32_t itemIndex);

	void query(const PxBounds3& bounds, physx::Array<uint32_t>& itemIndices, int32_t maxIndices = -1);
	void queryUnique(const PxBounds3& bounds, physx::Array<uint32_t>& itemIndices, int32_t maxIndices = -1);

	void query(const PxVec3& pos, physx::Array<uint32_t>& itemIndices, int32_t maxIndices = -1);
	void queryUnique(const PxVec3& pos, physx::Array<uint32_t>& itemIndices, int32_t maxIndices = -1);

	// applied functions, only work if inserted objects are points!
	int32_t getClosestPointNr(const PxVec3* points, uint32_t numPoints, uint32_t pointStride, const PxVec3& pos);

private:
	enum
	{
		HashIndexSize = 170111
	};

	void compressIndices(physx::Array<uint32_t>& itemIndices);
	float mSpacing;
	float mInvSpacing;
	uint32_t mTime;

	inline uint32_t  hashFunction(int32_t xi, int32_t yi, int32_t zi)
	{
		uint32_t h = (uint32_t)((xi * 92837111) ^(yi * 689287499) ^(zi * 283923481));
		return h % HashIndexSize;
	}

	inline void cellCoordOf(const PxVec3& v, int& xi, int& yi, int& zi)
	{
		xi = (int)(v.x * mInvSpacing);
		if (v.x < 0.0f)
		{
			xi--;
		}
		yi = (int)(v.y * mInvSpacing);
		if (v.y < 0.0f)
		{
			yi--;
		}
		zi = (int)(v.z * mInvSpacing);
		if (v.z < 0.0f)
		{
			zi--;
		}
	}

	MeshHashRoot* mHashIndex;
	physx::Array<MeshHashEntry> mEntries;

	physx::Array<uint32_t> mTempIndices;
};

}
} // end namespace nvidia::apex

#endif