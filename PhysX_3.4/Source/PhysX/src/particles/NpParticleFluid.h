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


#ifndef PX_PHYSICS_NP_PARTICLEFLUID
#define PX_PHYSICS_NP_PARTICLEFLUID

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "NpParticleSystem.h"

namespace physx
{

class NpParticleFluid;

typedef NpParticleBaseTemplate<PxParticleFluid, NpParticleFluid> NpParticleFluidT;
class NpParticleFluid : public NpParticleFluidT
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================
public:
// PX_SERIALIZATION
											NpParticleFluid(PxBaseFlags baseFlags) : NpParticleFluidT(baseFlags)	{}
	virtual		void						requiresObjects(PxProcessPxBaseCallback&){}	
	virtual		void						exportExtraData(PxSerializationContext& stream) { mParticleSystem.exportExtraData(stream); }
				void						importExtraData(PxDeserializationContext& context) { mParticleSystem.importExtraData(context); }
	static		NpParticleFluid*			createObject(PxU8*& address, PxDeserializationContext& context);
	static		void						getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
											NpParticleFluid(PxU32 maxParticles, bool perParticleRestOffset);
	virtual									~NpParticleFluid();

	//---------------------------------------------------------------------------------
	// PxParticleFluid implementation
	//---------------------------------------------------------------------------------
	virtual		PxActorType::Enum			getType() const { return PxActorType::ePARTICLE_FLUID; }

	virtual		void*						is(PxActorType::Enum type);
	virtual		const void*					is(PxActorType::Enum type)								const;

	virtual		PxParticleFluidReadData*	lockParticleFluidReadData(PxDataAccessFlags flags);
	virtual		PxParticleFluidReadData*	lockParticleFluidReadData();

	virtual		PxReal						getStiffness()											const;
	virtual		void 						setStiffness(PxReal);
	virtual		PxReal						getViscosity()											const;
	virtual		void 						setViscosity(PxReal);
	
	virtual		PxReal						getRestParticleDistance()								const;
	virtual		void						setRestParticleDistance(PxReal);

private:
	typedef NpParticleBaseTemplate<PxParticleFluid, NpParticleFluid> ParticleSystemTemplateClass;
				
};

}

#endif // PX_USE_PARTICLE_SYSTEM_API

#endif
