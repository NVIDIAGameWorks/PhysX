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

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "NpParticleFluid.h"
#include "ScbParticleSystem.h"
#include "NpWriteCheck.h"
#include "NpReadCheck.h"

using namespace physx;

NpParticleFluid::NpParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
: ParticleSystemTemplateClass(PxConcreteType::ePARTICLE_FLUID, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE, PxActorType::ePARTICLE_FLUID, maxParticles, perParticleRestOffset)
{}

NpParticleFluid::~NpParticleFluid()
{
}

// PX_SERIALIZATION
NpParticleFluid* NpParticleFluid::createObject(PxU8*& address, PxDeserializationContext& context)
{
	NpParticleFluid* obj = new (address) NpParticleFluid(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(NpParticleFluid);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

PxParticleFluidReadData* NpParticleFluid::lockParticleFluidReadData(PxDataAccessFlags flags)
{
	return static_cast<PxParticleFluidReadData*>(lockParticleReadData(flags));
}

PxParticleFluidReadData* NpParticleFluid::lockParticleFluidReadData()
{
	return static_cast<PxParticleFluidReadData*>(lockParticleReadData());
}

void* NpParticleFluid::is(PxActorType::Enum type)
{
	if (type == PxActorType::ePARTICLE_FLUID)
		return reinterpret_cast<void*>(static_cast<PxParticleFluid*>(this));
	else
		return NULL;
}


const void* NpParticleFluid::is(PxActorType::Enum type) const
{
	if (type == PxActorType::ePARTICLE_FLUID)
		return reinterpret_cast<const void*>(static_cast<const PxParticleFluid*>(this));
	else
		return NULL;
}


PxReal NpParticleFluid::getStiffness() const
{
	NP_READ_CHECK(getNpScene());
	return getScbParticleSystem().getStiffness();
}


void NpParticleFluid::setStiffness(PxReal s)
{
	NP_WRITE_CHECK(getNpScene());		
	PX_CHECK_AND_RETURN(s > 0.0f,"Stiffness needs to be positive, PxParticleFluid::setStiffness() ignored.");
	getScbParticleSystem().setStiffness(s);
}


PxReal NpParticleFluid::getViscosity() const
{
	NP_READ_CHECK(getNpScene());
	return getScbParticleSystem().getViscosity();
}


void NpParticleFluid::setViscosity(PxReal v)
{
	NP_WRITE_CHECK(getNpScene());		
	PX_CHECK_AND_RETURN(v > 0.0f,"Viscosity needs to be positive, PxParticleFluid::setViscosity() ignored.");
	getScbParticleSystem().setViscosity(v);
}


PxReal NpParticleFluid::getRestParticleDistance() const
{
	NP_READ_CHECK(getNpScene());
	return getScbParticleSystem().getRestParticleDistance();
}

void NpParticleFluid::setRestParticleDistance(PxReal r)
{
	NP_WRITE_CHECK(getNpScene());		
	PX_CHECK_AND_RETURN(r > 0.0f,"RestParticleDistance needs to be positive, PxParticleFluid::setRestParticleDistance() ignored.");
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"RestParticleDistance immutable when the particle system is part of a scene.");	
	getScbParticleSystem().setRestParticleDistance(r);
}

#endif // PX_USE_PARTICLE_SYSTEM_API
