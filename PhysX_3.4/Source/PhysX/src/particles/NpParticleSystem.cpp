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


#include "NpParticleSystem.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "PsFoundation.h"
#include "NpPhysics.h"

#include "NpScene.h"
#include "NpWriteCheck.h"

#include "PsArray.h"

using namespace physx;

NpParticleSystem::NpParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
: ParticleSystemTemplateClass(PxConcreteType::ePARTICLE_SYSTEM, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE, PxActorType::ePARTICLE_SYSTEM, maxParticles, perParticleRestOffset)
{}

NpParticleSystem::~NpParticleSystem()
{}

// PX_SERIALIZATION
NpParticleSystem* NpParticleSystem::createObject(PxU8*& address, PxDeserializationContext& context)
{
	NpParticleSystem* obj = new (address) NpParticleSystem(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(NpParticleSystem);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

void NpParticleSystem::setParticleReadDataFlag(PxParticleReadDataFlag::Enum flag, bool val)
{
	NP_WRITE_CHECK(getNpScene());		
	PX_CHECK_AND_RETURN( flag != PxParticleReadDataFlag::eDENSITY_BUFFER, 
		"ParticleSystem has unsupported PxParticleReadDataFlag::eDENSITY_BUFFER set.");	
	NpParticleSystemT::setParticleReadDataFlag(flag, val);
}
#endif
