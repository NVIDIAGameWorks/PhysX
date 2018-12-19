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



#ifndef __DESTRUCTIBLEHELPERS_H__
#define __DESTRUCTIBLEHELPERS_H__

#include "Apex.h"
#include "ApexSDKHelpers.h"
#include "PsUserAllocated.h"
#include "PsMemoryBuffer.h"

#include "DestructibleAsset.h"
#include "NxPhysicsSDK.h"

/*
	For managing mesh cooking at various scales
 */

namespace nvidia
{
namespace destructible
{

class ConvexHullAtScale
{
public:
	ConvexHullAtScale() : scale(1), convexMesh(NULL) {}
	ConvexHullAtScale(const PxVec3& inScale) : scale(inScale), convexMesh(NULL) {}

	PxVec3		scale;
	PxConvexMesh* 		convexMesh;
	physx::Array<uint8_t>	cookedHullData;
};

class MultiScaledConvexHull : public UserAllocated
{
public:
	ConvexHullAtScale* 	getConvexHullAtScale(const PxVec3& scale, float tolerance = 0.0001f)
	{
		// Find mesh at scale.  If not found, create one.
		for (uint32_t index = 0; index < meshes.size(); ++index)
		{
			if (PxVec3equals(meshes[index].scale, scale, tolerance))
			{
				return &meshes[index];
			}
		}
		meshes.insert();
		return new(&meshes.back()) ConvexHullAtScale(scale);
	}

	void	releaseConvexMeshes()
	{
		for (uint32_t index = 0; index < meshes.size(); ++index)
		{
			if (meshes[index].convexMesh != NULL)
			{
				GetApexSDK()->getPhysXSDK()->releaseConvexMesh(*meshes[index].convexMesh);
				meshes[index].convexMesh = NULL;
			}
		}
	}

	Array<ConvexHullAtScale> meshes;
};


}
} // end namespace nvidia

#endif	// __DESTRUCTIBLEHELPERS_H__
