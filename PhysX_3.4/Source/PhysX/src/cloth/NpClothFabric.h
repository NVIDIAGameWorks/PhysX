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


#ifndef PX_PHYSICS_NP_CLOTH_FABRIC
#define PX_PHYSICS_NP_CLOTH_FABRIC

#include "PxPhysXConfig.h"

#if PX_USE_CLOTH_API

#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "CmRefCountable.h"
#include "PxClothFabric.h"
#include "PsArray.h"
#include "ScClothFabricCore.h"  // for the people asking: Why is it ok to use Sc directly here? Because there is no double buffering for fabrics (they are like meshes)
#include "PxSerialFramework.h"

namespace physx
{

class NpClothFabric : public PxClothFabric, public Ps::UserAllocated, public Cm::RefCountable
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================
public:
// PX_SERIALIZATION
											NpClothFabric(PxBaseFlags baseFlags)	: PxClothFabric(baseFlags), Cm::RefCountable(PxEmpty), mFabric(PxEmpty) {}
	virtual		void						onRefCountZero();
	virtual		void						exportExtraData(PxSerializationContext&);
				void						importExtraData(PxDeserializationContext&);
	static		NpClothFabric*				createObject(PxU8*& address, PxDeserializationContext&);
	static		void						getBinaryMetaData(PxOutputStream& stream);
				void						resolveReferences(PxDeserializationContext&) {}
	virtual		void						requiresObjects(PxProcessPxBaseCallback&){}
//~PX_SERIALIZATION
											NpClothFabric();

				bool						load(PxInputStream& stream);
				bool						load(const PxClothFabricDesc& desc);

// PxClothFabric
	virtual		void						release();

	virtual		PxU32						getNbParticles() const;
	virtual		PxU32						getNbPhases() const;
	virtual		PxU32						getNbSets() const;
	virtual		PxU32						getNbParticleIndices() const;
    virtual     PxU32                       getNbRestvalues() const;
	virtual		PxU32						getNbTethers() const;

	virtual		PxU32						getPhases(PxClothFabricPhase* userPhaseIndexBuffer, PxU32 bufferSize) const;
	virtual     PxU32                       getSets(PxU32* userSetBuffer, PxU32 bufferSize) const;
    virtual     PxU32                       getParticleIndices(PxU32* userParticleIndexBuffer, PxU32 bufferSize) const;
	virtual		PxU32						getRestvalues(PxReal* userRestvalueBuffer, PxU32 bufferSize) const;

	virtual		PxU32						getTetherAnchors(PxU32* userAnchorBuffer, PxU32 bufferSize) const;
	virtual		PxU32						getTetherLengths(PxReal* userLengthBuffer, PxU32 bufferSize) const;

				PxClothFabricPhaseType::Enum getPhaseType(PxU32 phaseIndex) const;
	
	virtual		void						scaleRestlengths(PxReal scale);

	virtual		PxU32						getReferenceCount() const;
	virtual		void						acquireReference();
		//~PxClothFabric

	PX_FORCE_INLINE Sc::ClothFabricCore&	getScClothFabric() { return mFabric; }
	PX_FORCE_INLINE const Sc::ClothFabricCore&	getScClothFabric() const { return mFabric; }

	virtual									~NpClothFabric();

protected:

	Sc::ClothFabricCore mFabric;  // Internal layer object
};

}

#endif // PX_USE_CLOTH_API
#endif
