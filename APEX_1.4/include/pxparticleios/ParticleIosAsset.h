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



#ifndef PARTICLE_IOS_ASSET_H
#define PARTICLE_IOS_ASSET_H

#include "Apex.h"
#include <limits.h>

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define PARTICLE_IOS_AUTHORING_TYPE_NAME "ParticleIosAsset"

/**
 \brief APEX Particle System Asset
 */
class ParticleIosAsset : public IosAsset
{
public:
	///Get the radius of a particle
	virtual float						getParticleRadius() const = 0;

	///Get the maximum number of particles that are allowed to be newly created on each frame
	virtual float						getMaxInjectedParticleCount() const	= 0;
	
	///Get the maximum number of particles that this IOS can simulate
	virtual uint32_t					getMaxParticleCount() const = 0;
	
	///Get the mass of a particle
	virtual float						getParticleMass() const = 0;

protected:
	virtual ~ParticleIosAsset()	{}
};

/**
 \brief APEX Particle System Asset Authoring class
 */
class ParticleIosAssetAuthoring : public AssetAuthoring
{
public:
	///Set the radius of a particle
	virtual void setParticleRadius(float) = 0;

	///Set the maximum number of particles that are allowed to be newly created on each frame
	virtual void setMaxInjectedParticleCount(float count) = 0;
	
	///Set the maximum number of particles that this IOS can simulate
	virtual void setMaxParticleCount(uint32_t count) = 0;
	
	///Set the mass of a particle
	virtual void setParticleMass(float) = 0;

	///Set the (NRP) name for the collision group.
	virtual void setCollisionGroupName(const char* collisionGroupName) = 0;
	
	///Set the (NRP) name for the collision group mask.
	virtual void setCollisionGroupMaskName(const char* collisionGroupMaskName) = 0;

protected:
	virtual ~ParticleIosAssetAuthoring()	{}
};

PX_POP_PACK

}
} // namespace nvidia

#endif // PARTICLE_IOS_ASSET_H
