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


#ifndef PX_PHYSICS_EXTENSIONS_CLOTH_FABRIC_COOKER_H
#define PX_PHYSICS_EXTENSIONS_CLOTH_FABRIC_COOKER_H

/** \addtogroup extensions
  @{
*/

#include "common/PxPhysXCommonConfig.h"
#include "PxClothMeshDesc.h"
#include "cloth/PxClothFabric.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxPhysics;

struct PxFabricCookerImpl;

/**
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
class PX_DEPRECATED PxClothFabricCooker
{
public:
	/**
	\brief Cooks a triangle mesh to a PxClothFabricDesc.
	\param desc The cloth mesh descriptor on which the generation of the cooked mesh depends.
	\param gravity A normalized vector which specifies the direction of gravity. 
	This information allows the cooker to generate a fabric with higher quality simulation behavior.
	\param useGeodesicTether A flag to indicate whether to compute geodesic distance for tether constraints.
	\note The geodesic option for tether only works for manifold input.  For non-manifold input, a simple Euclidean distance will be used.
	For more detailed cooker status for such cases, try running PxClothGeodesicTetherCooker directly.
	*/
	PxClothFabricCooker(const PxClothMeshDesc& desc, const PxVec3& gravity, bool useGeodesicTether = true);
	~PxClothFabricCooker();

	/** \brief Returns the fabric descriptor to create the fabric. */
	PxClothFabricDesc getDescriptor() const;
	/** \brief Saves the fabric data to a platform and version dependent stream. */
	void save(PxOutputStream& stream, bool platformMismatch) const;

private:
	PxFabricCookerImpl* mImpl;
};

/**
\brief Cooks a triangle mesh to a PxClothFabric.

\param physics The physics instance.
\param desc The cloth mesh descriptor on which the generation of the cooked mesh depends.
\param gravity A normalized vector which specifies the direction of gravity. 
This information allows the cooker to generate a fabric with higher quality simulation behavior.
\param useGeodesicTether A flag to indicate whether to compute geodesic distance for tether constraints.
\return The created cloth fabric, or NULL if creation failed.
*/
PxClothFabric* PxClothFabricCreate(PxPhysics& physics, 
	const PxClothMeshDesc& desc, const PxVec3& gravity, bool useGeodesicTether = true);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif // PX_PHYSICS_EXTENSIONS_CLOTH_FABRIC_COOKER_H
