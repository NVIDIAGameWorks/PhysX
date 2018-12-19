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


#include "ScClothFabricCore.h"
#if PX_USE_CLOTH_API

using namespace physx;
using namespace cloth;

#include "foundation/PxAssert.h"
#include "ScPhysics.h"
#include "Range.h"
#include "Factory.h"
#include "Fabric.h"
#include "GuSerialize.h"
#include "CmPhysXCommon.h"
#include "CmUtils.h"
#include "PsAlloca.h"
#include "PsFoundation.h"

Sc::ClothFabricCore::ClothFabricCore(const PxEMPTY) 
	: mLowLevelGpuFabric(NULL), mPhaseTypes(PxEmpty)
{
}

Sc::ClothFabricCore::ClothFabricCore() :
    mLowLevelFabric(NULL), mLowLevelGpuFabric(NULL)
{
}


Sc::ClothFabricCore::~ClothFabricCore()
{
	if(mLowLevelFabric)
	{
		mLowLevelFabric->decRefCount();
		if (0 == mLowLevelFabric->getRefCount())
			PX_DELETE(mLowLevelFabric);
	}
}


// PX_SERIALIZATION
void Sc::ClothFabricBulkData::exportExtraData(PxSerializationContext& context)
{
	Cm::exportArray(mPhases, context);
	Cm::exportArray(mSets, context);
	Cm::exportArray(mRestvalues, context);
	Cm::exportArray(mIndices, context);
	Cm::exportArray(mTetherAnchors, context);
	Cm::exportArray(mTetherLengths, context);
	Cm::exportArray(mTriangles, context);
}

void Sc::ClothFabricBulkData::importExtraData(PxDeserializationContext& context)
{
	Cm::importArray(mPhases, context);
	Cm::importArray(mSets, context);
	Cm::importArray(mRestvalues, context);
	Cm::importArray(mIndices, context);
	Cm::importArray(mTetherAnchors, context);
	Cm::importArray(mTetherLengths, context);
	Cm::importArray(mTriangles, context);
}

void Sc::ClothFabricCore::exportExtraData(PxSerializationContext& stream)
{
	Cm::exportArray(mPhaseTypes, stream);

	if(!mLowLevelFabric)
		return;
	
	PX_ALLOCA(buf, ClothFabricBulkData, 1);
	Cm::markSerializedMem(buf, sizeof(ClothFabricBulkData));
	ClothFabricBulkData* bulkData = PX_PLACEMENT_NEW(buf, ClothFabricBulkData);

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	bulkData->mNbParticles = mLowLevelFabric->getNumParticles();
	bulkData->mPhases.resize(mLowLevelFabric->getNumPhases());
	bulkData->mSets.resize(mLowLevelFabric->getNumSets());
	bulkData->mRestvalues.resize(mLowLevelFabric->getNumRestvalues());
	bulkData->mIndices.resize(mLowLevelFabric->getNumIndices());
	bulkData->mTetherAnchors.reserve(mLowLevelFabric->getNumTethers());
	bulkData->mTetherLengths.reserve(mLowLevelFabric->getNumTethers());
	bulkData->mTriangles.resize(mLowLevelFabric->getNumTriangles() * 3);

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(bulkData->mPhases.begin(), bulkData->mPhases.end()), 
		Range<uint32_t>(bulkData->mSets.begin(), bulkData->mSets.end()), 
		Range<float>(bulkData->mRestvalues.begin(), bulkData->mRestvalues.end()), 
		Range<uint32_t>(bulkData->mIndices.begin(), bulkData->mIndices.end()),
		Range<uint32_t>(bulkData->mTetherAnchors.begin(), bulkData->mTetherAnchors.end()),
		Range<float>(bulkData->mTetherLengths.begin(), bulkData->mTetherLengths.end()),
		Range<uint32_t>(bulkData->mTriangles.begin(), bulkData->mTriangles.end()));

	stream.writeData(bulkData, sizeof(ClothFabricBulkData));
	bulkData->exportExtraData(stream);	
	bulkData->~ClothFabricBulkData();
}


void Sc::ClothFabricCore::importExtraData(PxDeserializationContext& context)
{
	Cm::importArray(mPhaseTypes, context);

	if(mLowLevelFabric)
	{
		ClothFabricBulkData* bulkData = context.readExtraData<ClothFabricBulkData>();
		bulkData->importExtraData(context);

		cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

		mLowLevelFabric = factory.createFabric(bulkData->mNbParticles, 
			cloth::Range<const uint32_t>(bulkData->mPhases.begin(), bulkData->mPhases.end()),
			cloth::Range<const uint32_t>(bulkData->mSets.begin(), bulkData->mSets.end()), 
			Range<const float>(bulkData->mRestvalues.begin(), bulkData->mRestvalues.end()), 
			Range<const uint32_t>(bulkData->mIndices.begin(), bulkData->mIndices.end()),
			Range<const uint32_t>(bulkData->mTetherAnchors.begin(), bulkData->mTetherAnchors.end()),
			Range<const float>(bulkData->mTetherLengths.begin(), bulkData->mTetherLengths.end()),
			Range<const uint32_t>(bulkData->mTriangles.begin(), bulkData->mTriangles.end()));

		mLowLevelFabric->incRefCount();  // account for the user reference
	}
}
//~PX_SERIALIZATION

/**
   Load cloth fabric data from the given stream.

   @param[in] stream input stream to load fabric data from
   @return true if loading was successful

   @sa For cooker implementation, see ClothFabricBuilder.cpp in PhysXCooking/src
 */
bool Sc::ClothFabricCore::load(PxInputStream& stream)
{
    // Import header
    PxU32 version;

	stream.read(&version, sizeof(PxU32));

    // check if incompatible version is used. Version number changed 
	// from PX_PHYSICS_VERSION to ordinal number in 3.3.2.
	// see ExtClothFabricCooker.cpp (PxFabricCookerImpl::save)
    if (version != 0x030300 && version != 0x030301 && version != 1)
    {
        Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Loading cloth fabric failed: mismatching version of cloth fabric stream.");
        return false;
    }

	PxClothFabricDesc desc;

	stream.read(&desc.nbParticles, sizeof(PxU32));
	stream.read(&desc.nbPhases, sizeof(PxU32));
	stream.read(&desc.nbSets, sizeof(PxU32));
	stream.read(&desc.nbTethers, sizeof(PxU32));

	Ps::Array<PxClothFabricPhase> phases(desc.nbPhases);
	PX_ASSERT(sizeof(PxClothFabricPhaseType::Enum) == sizeof(PxU32));
	stream.read(phases.begin(), desc.nbPhases * sizeof(PxClothFabricPhase));
	desc.phases = phases.begin();

	Ps::Array<PxU32> sets(desc.nbSets);
	stream.read(sets.begin(), desc.nbSets * sizeof(PxU32));
	desc.sets = sets.begin();

	PxU32 nbRestvalues = sets.back();
	Ps::Array<PxReal> restvalues(nbRestvalues);
	stream.read(restvalues.begin(), nbRestvalues * sizeof(PxReal));
	desc.restvalues = restvalues.begin();

	PxU32 nbParticleIndices = nbRestvalues * 2;
	Ps::Array<PxU32> indices(nbParticleIndices);
	stream.read(indices.begin(), nbParticleIndices * sizeof(PxU32));
	desc.indices = indices.begin();

	Ps::Array<PxU32> tetherAnchors(desc.nbTethers);
	stream.read(tetherAnchors.begin(), desc.nbTethers * sizeof(PxU32));
	desc.tetherAnchors = tetherAnchors.begin();

	Ps::Array<PxReal> tetherLengths(desc.nbTethers);
	stream.read(tetherLengths.begin(), desc.nbTethers * sizeof(PxReal));
	desc.tetherLengths = tetherLengths.begin();

	return load(desc);
}

bool Sc::ClothFabricCore::load(const PxClothFabricDesc& desc)
{
	PX_ASSERT( desc.isValid() );

	// allocate array
	//!!!CL could be optimized by creating a single buffer that can hold all data. Would have to sacrifice the PxArrays but they don't
	//      seem very useful in this case anyway because it is mostly const data.

	mPhaseTypes.resize(desc.nbPhases);
	Ps::Array<PxU32> llphases(desc.nbPhases);
	Ps::Array<PxU32> offsets(desc.nbPhases);

	for(PxU32 i=0; i<desc.nbPhases; ++i)
	{
		if (desc.phases[i].phaseType == PxClothFabricPhaseType::eINVALID || 
			desc.phases[i].phaseType >= PxClothFabricPhaseType::eCOUNT)
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
				"createClothFabric() failed, invalid phase type specified");

			return false;
		}

		mPhaseTypes[i] = desc.phases[i].phaseType;
		llphases[i] = desc.phases[i].setIndex;
	}

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	PxU32 nbConstraints = desc.sets[desc.nbSets-1];

	mLowLevelFabric = factory.createFabric(desc.nbParticles,
		cloth::Range<const uint32_t>(llphases.begin(), llphases.end()), 
		cloth::Range<const uint32_t>(desc.sets, desc.sets + desc.nbSets), 
		cloth::Range<const float>(desc.restvalues, desc.restvalues + nbConstraints), 
		cloth::Range<const uint32_t>(desc.indices, desc.indices + 2*nbConstraints),
		cloth::Range<const uint32_t>(desc.tetherAnchors, desc.tetherAnchors + desc.nbTethers), 
		cloth::Range<const float>(desc.tetherLengths, desc.tetherLengths + desc.nbTethers),
		cloth::Range<const uint32_t>(desc.triangles, desc.triangles + 3 * desc.nbTriangles));

	mLowLevelFabric->incRefCount();  // account for the user reference

	return true;
}

/**
   returns number of particles
 */
PxU32 Sc::ClothFabricCore::getNbParticles() const
{
    return mLowLevelFabric->getNumParticles();
}

PxU32 Sc::ClothFabricCore::getNbPhases() const
{
	return mLowLevelFabric->getNumPhases();
}

/**
   This function returns number of sets
   @return number of phases
 */ 
PxU32 Sc::ClothFabricCore::getNbSets() const
{
    return mLowLevelFabric->getNumSets();
}

/**
   get total number of particle (vertex) indices in the fabric array.
 */
PxU32 Sc::ClothFabricCore::getNbParticleIndices() const
{
    return mLowLevelFabric->getNumIndices();
}

/**
   get total number of rest length array
 */
PxU32 Sc::ClothFabricCore::getNbRestvalues() const
{
    return mLowLevelFabric->getNumRestvalues();
}

PxU32 Sc::ClothFabricCore::getNbTethers() const
{
	return mLowLevelFabric->getNumTethers();
}

PxU32 Sc::ClothFabricCore::getPhases(PxClothFabricPhase* userPhaseBuffer, PxU32 bufferSize) const
{
	// calculate buffer size
	PxU32 nbPhases = getNbPhases();
	if (bufferSize < nbPhases)
		return 0;

	shdfnd::Array<PxU32> phases(nbPhases);

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(phases.begin(), phases.end()), 
		Range<uint32_t>(), Range<float>(), Range<uint32_t>(), 
		Range<uint32_t>(), Range<float>(), Range<uint32_t>());

	for(PxU32 i=0; i<nbPhases; ++i)
	{
		userPhaseBuffer[i].setIndex = phases[i];
		userPhaseBuffer[i].phaseType = mPhaseTypes[i];
	}

	return nbPhases;
}

PxU32 Sc::ClothFabricCore::getRestvalues(PxReal* userRestvalueBuffer, PxU32 bufferSize) const
{
	// calculate buffer size
	PxU32 nbRestvalues = getNbRestvalues();
	if (bufferSize < nbRestvalues)
		return 0;

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(), cloth::Range<uint32_t>(), 
		Range<float>(userRestvalueBuffer, userRestvalueBuffer + nbRestvalues), 
		Range<uint32_t>(), Range<uint32_t>(), Range<float>(), Range<uint32_t>());

	return nbRestvalues;
}

PxU32 Sc::ClothFabricCore::getSets(PxU32* userSetBuffer, PxU32 bufferSize) const
{
    // calculate buffer size
	const PxU32 nbSets = getNbSets();
	if (bufferSize < nbSets)
        return 0;

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(), Range<uint32_t>(userSetBuffer, userSetBuffer + nbSets), 
		Range<float>(), Range<uint32_t>(), Range<uint32_t>(), Range<float>(), Range<uint32_t>());

    return nbSets;
}

PxU32 Sc::ClothFabricCore::getParticleIndices(PxU32* userParticleIndexBuffer, PxU32 bufferSize) const
{
    // calculate buffer size
	const PxU32 nbParticleIndices = getNbParticleIndices();
	if (bufferSize < nbParticleIndices)
        return 0;

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(), cloth::Range<uint32_t>(), Range<float>(), 
		Range<uint32_t>(userParticleIndexBuffer, userParticleIndexBuffer + nbParticleIndices),
		Range<uint32_t>(), Range<float>(), Range<uint32_t>());

    return nbParticleIndices;
}

PxU32 Sc::ClothFabricCore::getTetherAnchors(PxU32* userAnchorBuffer, PxU32 bufferSize) const
{
	// calculate buffer size
	const PxU32 nbTethers = getNbTethers();
	if (bufferSize < nbTethers)
		return 0;

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(), cloth::Range<uint32_t>(), 
		Range<float>(), Range<uint32_t>(),
		Range<uint32_t>(userAnchorBuffer, userAnchorBuffer + nbTethers), Range<float>(), Range<uint32_t>());

	return nbTethers;
}

PxU32 Sc::ClothFabricCore::getTetherLengths(PxReal* userLengthBuffer, PxU32 bufferSize) const
{
	// calculate buffer size
	const PxU32 nbTethers = getNbTethers();
	if (bufferSize < nbTethers)
		return 0;

	cloth::Factory& factory = Sc::Physics::getInstance().getLowLevelClothFactory();

	factory.extractFabricData(*mLowLevelFabric, 
		cloth::Range<uint32_t>(), cloth::Range<uint32_t>(), 
		Range<float>(), Range<uint32_t>(), Range<uint32_t>(), 
		Range<float>(userLengthBuffer, userLengthBuffer + nbTethers), Range<uint32_t>());

	return nbTethers;
}

/**
   scale the rest length of constraints by given value.

   @param[in] scale The scale factor (each rest length is multiplied by this value)
 */
void Sc::ClothFabricCore::scaleRestlengths(PxReal scale)
{
	mLowLevelFabric->scaleRestvalues(scale);
	mLowLevelFabric->scaleTetherLengths(scale);

	if(mLowLevelGpuFabric)
	{
		mLowLevelGpuFabric->scaleRestvalues(scale);
		mLowLevelGpuFabric->scaleTetherLengths(scale);
	}
}


#endif	// PX_USE_CLOTH_API
