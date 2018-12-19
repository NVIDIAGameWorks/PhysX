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



#ifndef IOFX_ASSET_H
#define IOFX_ASSET_H

#include "Apex.h"
#include "ModifierDefs.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define IOFX_AUTHORING_TYPE_NAME "IOFX"

class Modifier;
class ApexActor;

/**
 \brief IOFX Asset public interface. Used to define the way the visual parameters are created
		from physical parameters of a particle
*/
class IofxAsset : public Asset, public Context
{
public:
	///get the name of the material used for sprite-based particles visualization
	virtual const char*						getSpriteMaterialName() const = 0;

	///get the number of different mesh assets used for mesh-based particles visualization
	virtual uint32_t						getMeshAssetCount() const = 0;
	
	///get the name of one of the mesh assets used for mesh-based particles visualization
	/// \param index mesh asset internal index
	virtual const char*						getMeshAssetName(uint32_t index = 0) const = 0;
	
	///get the weight of one of the mesh assets used for mesh-based particles visualization. Can be any value; not normalized.
	/// \param index mesh asset internal index
	virtual uint32_t						getMeshAssetWeight(uint32_t index = 0) const = 0;

	///get the list of spawn modifiers
	virtual const Modifier*					getSpawnModifiers(uint32_t& outCount) const = 0;
	
	///get the list of continuous modifiers
	virtual const Modifier*					getContinuousModifiers(uint32_t& outCount) const = 0;

	///get the biggest possible scale given the current spawn- and continuous modifiers
	///note that some modifiers depend on velocity, so the scale can get arbitrarily large.
	/// \param maxVelocity this value defines what the highest expected velocity is to compute the upper bound
	virtual float							getScaleUpperBound(float maxVelocity) const = 0;

	///the IOFX asset needs to inform other actors when it is released
	/// \note only for internal use
	virtual void							addDependentActor(ApexActor* actor) = 0;
};

/**
 \brief IOFX Asset Authoring public interface.
 */
class IofxAssetAuthoring : public AssetAuthoring
{
};

PX_POP_PACK

}
} // namespace nvidia

#endif // IOFX_ASSET_H
