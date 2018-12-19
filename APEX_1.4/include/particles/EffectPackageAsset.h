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



#ifndef EFFECT_PACKAGE_ASSET_H
#define EFFECT_PACKAGE_ASSET_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define PARTICLES_EFFECT_PACKAGE_AUTHORING_TYPE_NAME "EffectPackageAsset"

/**
\brief Describes an EffectPackageAsset; a collection of particle related effects (emitters, field samplers, etc.)
*/
class EffectPackageAsset : public Asset
{
protected:

	virtual ~EffectPackageAsset() {}

public:
	/**
	\brief returns the duration of the effect; accounting for maximum duration of each sub-effect.  A duration of 'zero' means it is an infinite lifetime effect.
	*/
	virtual float getDuration() const = 0;

	/**
	\brief This method returns true if the authored asset has the 'hint' set to indicate the actors should be created with a unique render volume.
	*/
	virtual bool useUniqueRenderVolume() const = 0;


}; //

/**
 \brief Describes an EffectPackageAssetAuthoring class; not currently used.  The ParticleEffectTool is used to
		author EffectPackageAssets
 */
class EffectPackageAssetAuthoring : public AssetAuthoring
{
protected:
	virtual ~EffectPackageAssetAuthoring() {}
};

PX_POP_PACK

} // end of apex namespace
} // end of nvidia namespace

#endif
