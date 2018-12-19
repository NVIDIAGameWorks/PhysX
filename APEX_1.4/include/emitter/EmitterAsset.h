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



#ifndef EMITTER_ASSET_H
#define EMITTER_ASSET_H

#include "Apex.h"
#include "EmitterGeoms.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define EMITTER_AUTHORING_TYPE_NAME "ApexEmitterAsset"

class EmitterActor;
class EmitterPreview;
class EmitterLodParamDesc;

///APEX Emitter asset. Emits particles within some shape.
class EmitterAsset : public Asset
{
protected:
	virtual ~EmitterAsset() {}

public:
	
	/// Returns the explicit geometry for the all actors based on this asset if the asset is explicit, NULL otherwise
	virtual EmitterGeomExplicit* 	isExplicitGeom() = 0;

	/// Returns the geometry used for the all actors based on this asset
	virtual const EmitterGeom*		getGeom() const = 0;

	/// Gets IOFX asset name that is used to visualize partices of this emitter
	virtual const char* 			getInstancedObjectEffectsAssetName(void) const = 0;
	
	/// Gets IOS asset name that is used to simulate partices of this emitter
	virtual const char* 			getInstancedObjectSimulatorAssetName(void) const = 0;
	
	/// Gets IOS asset class name that is used to simulate partices of this emitter
	virtual const char* 			getInstancedObjectSimulatorTypeName(void) const = 0;

	virtual const float & 			getDensity() const = 0; ///< Gets the range used to choose the density of particles	
	virtual const float & 			getRate() const = 0;	///< Gets the range used to choose the emission rate
	virtual const PxVec3 & 			getVelocityLow() const = 0; ///< Gets the range used to choose the velocity of particles
	virtual const PxVec3 & 			getVelocityHigh() const = 0; ///< Gets the range used to choose the velocity of particles
	virtual const float & 			getLifetimeLow() const = 0; ///< Gets the range used to choose the lifetime of particles
	virtual const float & 			getLifetimeHigh() const = 0; ///< Gets the range used to choose the lifetime of particles
	
	/// For an explicit emitter, Max Samples is ignored.  For shaped emitters, it is the maximum number of objects spawned in a step.
	virtual uint32_t                getMaxSamples() const = 0;

	/**
	\brief Gets the emitter duration in seconds
	\note If EmitterActor::startEmit() is called with persistent=true, then this duration is ignored.
	*/
	virtual float					getEmitDuration() const = 0;

	/// Gets LOD settings for this asset
	virtual const EmitterLodParamDesc& getLodParamDesc() const = 0;

};

///APEX Emitter Asset Authoring. Used to create APEX Emitter assets.
class EmitterAssetAuthoring : public AssetAuthoring
{
protected:
	virtual ~EmitterAssetAuthoring() {}
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // EMITTER_ASSET_H
