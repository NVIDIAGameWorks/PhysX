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



#ifndef IMPACT_EMITTER_ACTOR_H
#define IMPACT_EMITTER_ACTOR_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{


PX_PUSH_PACK_DEFAULT

class ImpactEmitterAsset;
class RenderVolume;

///Impact emitter actor.  Emits particles at impact places
class ImpactEmitterActor : public Actor
{
protected:
	virtual ~ImpactEmitterActor() {}

public:
	///Gets the pointer to the underlying asset
	virtual ImpactEmitterAsset* 	getEmitterAsset() const = 0;

	/**
	\brief Registers an impact in the queue

	\param hitPos impact position
	\param hitDir impact direction
	\param surfNorm normal of the surface that is hit by the impact
	\param setID - id for the event set which should be spawned. Specifies the behavior. \sa ImpactEmitterAsset::querySetID

	*/
	virtual void registerImpact(const PxVec3& hitPos, const PxVec3& hitDir, const PxVec3& surfNorm, uint32_t setID) = 0;

	///Emitted particles are injected to specified render volume on initial frame.
	///Set to NULL to clear the preferred volume.
	virtual void setPreferredRenderVolume(RenderVolume* volume) = 0;
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // IMPACT_EMITTER_ACTOR_H
