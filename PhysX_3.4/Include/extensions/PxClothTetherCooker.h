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


#ifndef PX_PHYSICS_EXTENSIONS_CLOTH_TETHER_COOKER_H
#define PX_PHYSICS_EXTENSIONS_CLOTH_TETHER_COOKER_H

#include "common/PxPhysXCommonConfig.h"
#include "PxClothMeshDesc.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	
struct PxClothSimpleTetherCookerImpl;

/**
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
class PX_DEPRECATED PxClothSimpleTetherCooker
{
public:
	/**
	\brief Compute tether data from PxClothMeshDesc with simple distance measure.
	\details The tether constraint in PxCloth requires rest distance and anchor index to be precomputed during cooking time.
	This cooker computes a simple Euclidean distance to closest anchor point.
	The Euclidean distance measure works reasonably for flat cloth and flags and computation time is very fast.
	With this cooker, there is only one tether anchor point per particle.
	\see PxClothTetherGeodesicCooker for more accurate distance estimation.
	\param desc The cloth mesh descriptor prepared for cooking
	*/
	PxClothSimpleTetherCooker(const PxClothMeshDesc &desc);
	~PxClothSimpleTetherCooker();

    /** 
	\brief Returns computed tether data.
	\details This function returns anchor indices for each particle as well as desired distance between the tether anchor and the particle.
	The user buffers should be at least as large as number of particles.
	*/
    void getTetherData(PxU32* userTetherAnchors, PxReal* userTetherLengths) const;

private:
	PxClothSimpleTetherCookerImpl* mImpl;

};


struct PxClothGeodesicTetherCookerImpl;

/**
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
class PX_DEPRECATED PxClothGeodesicTetherCooker
{
public:
	/**
	\brief Compute tether data from PxClothMeshDesc using geodesic distance.
	\details The tether constraint in PxCloth requires rest distance and anchor index to be precomputed during cooking time.
	The provided tether cooker computes optimal tether distance with geodesic distance computation.
	For curved and complex meshes, geodesic distance provides the best behavior for tether constraints.
	But the cooking time is slower than the simple cooker.
	\see PxClothSimpleTetherCooker
	\param desc The cloth mesh descriptor prepared for cooking
	\note The geodesic distance is optimized to work for intended use in tether constraint.  
	This is by no means a general purpose geodesic computation code for arbitrary meshes.
	\note The geodesic cooker does not work with non-manifold input such as edges having more than two incident triangles, 
	or adjacent triangles following inconsitent winding order (e.g. clockwise vs counter-clockwise). 
	*/
	PxClothGeodesicTetherCooker(const PxClothMeshDesc &desc);
	~PxClothGeodesicTetherCooker();

	/**
	\brief Returns cooker status
	\details This function returns cooker status after cooker computation is done.
	A non-zero return value indicates a failure, 1 for non-manifold and 2 for inconsistent winding.
	*/
	PxU32 getCookerStatus() const;

	/**
	\brief Returns number of tether anchors per particle
	\note Returned number indicates the maximum anchors.  
	If some particles are assigned fewer anchors, the anchor indices will be PxU32(-1) 
	\note If there is no attached point in the input mesh descriptor, this will return 0 and no tether data will be generated.
	*/
	PxU32 getNbTethersPerParticle() const;

    /** 
	\brief Returns computed tether data.
	\details This function returns anchor indices for each particle as well as desired distance between the tether anchor and the particle.
	The user buffers should be at least as large as number of particles * number of tethers per particle.
	\see getNbTethersPerParticle()
	*/
    void getTetherData(PxU32* userTetherAnchors, PxReal* userTetherLengths) const;

private:
	PxClothGeodesicTetherCookerImpl* mImpl;

};


#if !PX_DOXYGEN
} // namespace physx
#endif

#endif // PX_PHYSICS_EXTENSIONS_CLOTH_TETHER_COOKER_H
