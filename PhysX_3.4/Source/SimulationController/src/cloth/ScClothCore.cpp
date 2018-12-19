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


#include "ScClothCore.h"
#if PX_USE_CLOTH_API

using namespace physx;
using namespace cloth;

#include "PxClothParticleData.h"
#include "PxClothTypes.h"

#include "ScClothFabricCore.h"
#include "ScClothSim.h"
#include "ScPhysics.h"
#include "ScScene.h"

#include "Types.h"
#include "Range.h"
#include "Factory.h"
#include "Cloth.h"
#include "Fabric.h"
#include "PsHash.h"

namespace 
{
	template <typename D, typename S>
	PX_FORCE_INLINE cloth::Range<D> createRange(S begin, PxU32 size)
	{
		D* start = reinterpret_cast<D*>(begin);
		D* end = start + size;

		return cloth::Range<D>(start, end);
	}
}

bool Sc::DefaultClothInterCollisionFilter(void* cloth0, void* cloth1)
{
	PX_ASSERT(cloth0);
	PX_ASSERT(cloth1);

	Sc::ClothCore* scCloth0 = static_cast<Sc::ClothCore*>(cloth0);
	Sc::ClothCore* scCloth1 = static_cast<Sc::ClothCore*>(cloth1);

	const Sc::Scene& scene = scCloth0->getSim()->getScene();
	PX_ASSERT(&scCloth1->getSim()->getScene() == &scene);

	PxSimulationFilterShader shader = scene.getFilterShaderFast();

	PxPairFlags pairFlags;
	PxFilterFlags filterFlags = shader(
		PxFilterObjectType::eCLOTH, scCloth0->getSimulationFilterData(),
		PxFilterObjectType::eCLOTH, scCloth1->getSimulationFilterData(),
		pairFlags,
		scene.getFilterShaderDataFast(),
		scene.getFilterShaderDataSizeFast());

	if (filterFlags&PxFilterFlag::eCALLBACK)
	{	
		// create unique pair id from cloth core ptrs
		const PxU32 pairId = shdfnd::hash(shdfnd::Pair<Sc::ClothCore*, Sc::ClothCore*>(scCloth0, scCloth1));

		filterFlags = scene.getFilterCallbackFast()->pairFound(pairId,
			PxFilterObjectType::eCLOTH, scCloth0->getSimulationFilterData(), scCloth0->getPxCloth(), NULL, 
			PxFilterObjectType::eCLOTH, scCloth1->getSimulationFilterData(), scCloth1->getPxCloth(), NULL, pairFlags);
	}

	if (filterFlags&PxFilterFlag::eKILL || filterFlags&PxFilterFlag::eSUPPRESS)
		return false;

	return true;
}


Sc::ClothCore::ClothCore(const PxTransform& globalPose, Sc::ClothFabricCore& fabric, const PxClothParticle* particles, PxClothFlags flags) : 
	ActorCore(PxActorType::eCLOTH, PxActorFlag::eVISUALIZATION, PX_DEFAULT_CLIENT, 0, 0),
	mExternalAcceleration(0.0f),
	mFabric(&fabric),
	mBulkData(NULL),
	mClothFlags(flags),
	mContactOffset(0.0f),
	mRestOffset(0.0f),
	mNumUserSpheres(0),
	mNumUserCapsules(0),
	mNumUserPlanes(0),
	mNumUserConvexes(0),
	mNumUserTriangles(0)
{
	initLowLevel(globalPose, particles);
}


Sc::ClothCore::~ClothCore()
{
	PX_ASSERT(getSim() == NULL);

	if (mPhaseConfigs)
		PX_FREE(mPhaseConfigs);

	cloth::Fabric* fabric = &mLowLevelCloth->getFabric();

	PX_DELETE(mLowLevelCloth);

	// If a cloth fabric gets deleted while the simulation is running, the fabric has to be delayed destroyed but
	// ScClothFabric is then gone already. So we delete here in this case. It would work automatically if the fabric
	// deleted itself on refCount 0 but doing it the way below keeps the LL interface independent.
	if (fabric->getRefCount() == 0)
		PX_DELETE(fabric);
}

void Sc::ClothCore::initLowLevel(const PxTransform& globalPose, const PxClothParticle* particles)
{
	PX_ASSERT(mFabric);

	PxU32 nbPhases = mFabric->getNbPhases();
	mPhaseConfigs = reinterpret_cast<cloth::PhaseConfig*>(PX_ALLOC(nbPhases * sizeof(cloth::PhaseConfig), "cloth::PhaseConfig"));

	if (mPhaseConfigs)
	{
		for(PxU16 i=0; i < nbPhases; i++)
		{
			cloth::PhaseConfig config(i);
			PX_PLACEMENT_NEW(mPhaseConfigs + i, cloth::PhaseConfig)(config);
		}

		// Create the low level object
		cloth::Range<const PxVec4> particlesRange = createRange<const PxVec4>(particles, mFabric->getNbParticles());

		//!!!CL todo: error processing if creation fails
		mLowLevelCloth = Sc::Physics::getInstance().getLowLevelClothFactory().createCloth(particlesRange, mFabric->getLowLevelFabric());

		if (mLowLevelCloth)
		{
			setGlobalPose(globalPose);

			cloth::Range<const cloth::PhaseConfig> phaseConfigRange = createRange<const cloth::PhaseConfig>(mPhaseConfigs, nbPhases);
			mLowLevelCloth->setPhaseConfig(phaseConfigRange);

			mLowLevelCloth->enableContinuousCollision(mClothFlags & PxClothFlag::eSWEPT_CONTACT);

			mLowLevelCloth->setUserData(this);

			wakeUp(Physics::sWakeCounterOnCreation);
		}
	}
}

// PX_SERIALIZATION

void Sc::ClothCore::updateBulkData(ClothBulkData& bulkData)
{
	cloth::MappedRange<const PxVec4> particles = cloth::readCurrentParticles(*mLowLevelCloth);
	bulkData.mParticles.resize(getNbParticles(), PxClothParticle(PxVec3(0.0f), 0.0f));
	PxMemCopy(bulkData.mParticles.begin(), particles.begin(), particles.size() * sizeof(PxVec4));

	if (getNbVirtualParticles())
	{
		// could avoid a lot of the construction overhead inside
		// resize() if Array supported insertRange() instead
		bulkData.mVpData.resize(getNbVirtualParticles()*4);
		bulkData.mVpWeightData.resize(getNbVirtualParticleWeights(), PxVec3(0.0f));

		getVirtualParticles(bulkData.mVpData.begin());
		getVirtualParticleWeights(bulkData.mVpWeightData.begin());
	}

	if (getNbCollisionSpheres() || getNbCollisionConvexes() || getNbCollisionTriangles())
	{
		bulkData.mCollisionSpheres.resize(getNbCollisionSpheres());
		bulkData.mCollisionPairs.resize(getNbCollisionCapsules()*2);
		bulkData.mCollisionPlanes.resize(getNbCollisionPlanes());
		bulkData.mConvexMasks.resize(getNbCollisionConvexes());
		bulkData.mCollisionTriangles.resize(getNbCollisionTriangles());

		getCollisionData(bulkData.mCollisionSpheres.begin(), bulkData.mCollisionPairs.begin(),
			bulkData.mCollisionPlanes.begin(), bulkData.mConvexMasks.begin(), bulkData.mCollisionTriangles.begin());
	}

	if (mLowLevelCloth->getNumMotionConstraints())
	{
		bulkData.mConstraints.resize(mLowLevelCloth->getNumMotionConstraints(), PxClothParticleMotionConstraint(PxVec3(0.0f), 0.0f));
		getMotionConstraints(bulkData.mConstraints.begin());
	}

	if (mLowLevelCloth->getNumSeparationConstraints())
	{
		bulkData.mSeparationConstraints.resize(mLowLevelCloth->getNumSeparationConstraints(), PxClothParticleSeparationConstraint(PxVec3(0.0f), 0.0f));
		getSeparationConstraints(bulkData.mSeparationConstraints.begin());
	}

	if (mLowLevelCloth->getNumParticleAccelerations())
	{
		bulkData.mParticleAccelerations.resize(mLowLevelCloth->getNumParticleAccelerations());
		getParticleAccelerations(bulkData.mParticleAccelerations.begin());
	}

	if(mLowLevelCloth->getNumSelfCollisionIndices())
	{
		bulkData.mSelfCollisionIndices.resize(mLowLevelCloth->getNumSelfCollisionIndices());
		getSelfCollisionIndices(bulkData.mSelfCollisionIndices.begin());
	}

	if (mLowLevelCloth->getNumRestPositions())
	{
		bulkData.mRestPositions.resize(mLowLevelCloth->getNumRestPositions(), PxVec4(0.0f));
		getRestPositions(bulkData.mRestPositions.begin());
	}

	bulkData.mTetherConstraintScale = mLowLevelCloth->getTetherConstraintScale();
	bulkData.mTetherConstraintStiffness = mLowLevelCloth->getTetherConstraintStiffness();
	bulkData.mMotionConstraintScale = mLowLevelCloth->getMotionConstraintScale();
	bulkData.mMotionConstraintBias = mLowLevelCloth->getMotionConstraintBias();
	bulkData.mMotionConstraintStiffness = mLowLevelCloth->getMotionConstraintStiffness();
	bulkData.mAcceleration = getExternalAcceleration();
	bulkData.mDamping = mLowLevelCloth->getDamping();
	bulkData.mFriction = mLowLevelCloth->getFriction();
	bulkData.mCollisionMassScale = mLowLevelCloth->getCollisionMassScale();
	bulkData.mLinearDrag = mLowLevelCloth->getLinearDrag();
	bulkData.mAngularDrag = mLowLevelCloth->getAngularDrag();
	bulkData.mLinearInertia = mLowLevelCloth->getLinearInertia();
	bulkData.mAngularInertia = mLowLevelCloth->getAngularInertia();
	bulkData.mCentrifugalInertia = mLowLevelCloth->getCentrifugalInertia();
	bulkData.mSolverFrequency = mLowLevelCloth->getSolverFrequency();
	bulkData.mStiffnessFrequency = mLowLevelCloth->getStiffnessFrequency();
	bulkData.mSelfCollisionDistance = mLowLevelCloth->getSelfCollisionDistance();
	bulkData.mSelfCollisionStiffness = mLowLevelCloth->getSelfCollisionStiffness();
	bulkData.mGlobalPose = getGlobalPose();
	bulkData.mSleepThreshold = mLowLevelCloth->getSleepThreshold();
	bulkData.mWakeCounter = getWakeCounter();
	bulkData.mWindVelocity = getWindVelocity();
	bulkData.mDragCoefficient = getDragCoefficient();
	bulkData.mLiftCoefficient = getLiftCoefficient();
}

void Sc::ClothCore::resolveReferences(Sc::ClothFabricCore& fabric)
{
	PX_ASSERT(mBulkData);
	
	mFabric = &fabric;
	
	// at this point we have all our bulk data and a reference
	// to the Sc::ClothFabricCore object so we can go ahead
	// and create the low level cloth object

	initLowLevel(mBulkData->mGlobalPose, mBulkData->mParticles.begin());

	mLowLevelCloth->setSpheres(createRange<PxVec4>(mBulkData->mCollisionSpheres.begin(), mBulkData->mCollisionSpheres.size()), 0, 0);
	mLowLevelCloth->setCapsules(createRange<PxU32>(mBulkData->mCollisionPairs.begin(), mBulkData->mCollisionPairs.size()/2), 0, 0);
	mLowLevelCloth->setPlanes(createRange<PxVec4>(mBulkData->mCollisionPlanes.begin(), mBulkData->mCollisionPlanes.size()), 0, 0);
	mLowLevelCloth->setConvexes(createRange<PxU32>(mBulkData->mConvexMasks.begin(), mBulkData->mConvexMasks.size()), 0, 0);
	mLowLevelCloth->setTriangles(createRange<PxVec3>(mBulkData->mCollisionTriangles.begin(), mBulkData->mCollisionTriangles.size()*3), 0, 0);

	// update virtual particles
	if (mBulkData->mVpData.size())
		setVirtualParticles(mBulkData->mVpData.size()/4, mBulkData->mVpData.begin(),
			mBulkData->mVpWeightData.size(), mBulkData->mVpWeightData.begin());

	// update motion constraints 
	if (mBulkData->mConstraints.size())
		setMotionConstraints(mBulkData->mConstraints.begin());

	// update separation constraints 
	if (mBulkData->mSeparationConstraints.size())
		setSeparationConstraints(mBulkData->mSeparationConstraints.begin());

	// update particle accelerations  
	if (mBulkData->mParticleAccelerations.size())
		setParticleAccelerations(mBulkData->mParticleAccelerations.begin());

	if (mBulkData->mSelfCollisionIndices.size())
	{
		setSelfCollisionIndices(mBulkData->mSelfCollisionIndices.begin(), 
			mBulkData->mSelfCollisionIndices.size());
	}

	// update rest positions
	if (mBulkData->mRestPositions.size())
		setRestPositions(mBulkData->mRestPositions.begin());

	mLowLevelCloth->setTetherConstraintScale(mBulkData->mTetherConstraintScale);
	mLowLevelCloth->setTetherConstraintStiffness(mBulkData->mTetherConstraintStiffness);
	mLowLevelCloth->setMotionConstraintScaleBias(mBulkData->mMotionConstraintScale, mBulkData->mMotionConstraintBias);
	mLowLevelCloth->setMotionConstraintStiffness(mBulkData->mMotionConstraintStiffness);
	setExternalAcceleration(mBulkData->mAcceleration);
	mLowLevelCloth->setDamping(mBulkData->mDamping);
	mLowLevelCloth->setFriction(mBulkData->mFriction);
	mLowLevelCloth->setCollisionMassScale(mBulkData->mCollisionMassScale);
	mLowLevelCloth->setLinearDrag(mBulkData->mLinearDrag);
	mLowLevelCloth->setAngularDrag(mBulkData->mAngularDrag);
	mLowLevelCloth->setLinearInertia(mBulkData->mLinearInertia);
	mLowLevelCloth->setAngularInertia(mBulkData->mAngularInertia);
	mLowLevelCloth->setCentrifugalInertia(mBulkData->mCentrifugalInertia);
	mLowLevelCloth->setSolverFrequency(mBulkData->mSolverFrequency);
	mLowLevelCloth->setStiffnessFrequency(mBulkData->mStiffnessFrequency);
	mLowLevelCloth->setSelfCollisionDistance(mBulkData->mSelfCollisionDistance);
	mLowLevelCloth->setSelfCollisionStiffness(mBulkData->mSelfCollisionStiffness);

	mLowLevelCloth->setSleepThreshold(mBulkData->mSleepThreshold);
	setWakeCounter(mBulkData->mWakeCounter);

	mLowLevelCloth->setWindVelocity(mBulkData->mWindVelocity);
	mLowLevelCloth->setDragCoefficient(mBulkData->mDragCoefficient);
	mLowLevelCloth->setLiftCoefficient(mBulkData->mLiftCoefficient);

	mBulkData = NULL;
}

void Sc::ClothBulkData::exportExtraData(PxSerializationContext& context)
{
	Cm::exportArray(mParticles, context);
	Cm::exportArray(mVpData, context);
	Cm::exportArray(mVpWeightData, context);
	Cm::exportArray(mCollisionSpheres, context);
	Cm::exportArray(mCollisionPairs, context);
	Cm::exportArray(mCollisionPlanes, context);
	Cm::exportArray(mConvexMasks, context);
	Cm::exportArray(mCollisionTriangles, context);
	Cm::exportArray(mConstraints, context);
	Cm::exportArray(mSeparationConstraints, context);
	Cm::exportArray(mParticleAccelerations, context);
	Cm::exportArray(mRestPositions, context);
}

void Sc::ClothBulkData::importExtraData(PxDeserializationContext& context)
{
	Cm::importArray(mParticles, context);
	Cm::importArray(mVpData, context);
	Cm::importArray(mVpWeightData, context);
	Cm::importArray(mCollisionSpheres, context);
	Cm::importArray(mCollisionPairs, context);
	Cm::importArray(mCollisionPlanes, context);
	Cm::importArray(mConvexMasks, context);
	Cm::importArray(mCollisionTriangles, context);
	Cm::importArray(mConstraints, context);
	Cm::importArray(mSeparationConstraints, context);
	Cm::importArray(mParticleAccelerations, context);
	Cm::importArray(mRestPositions, context);
}

void Sc::ClothCore::exportExtraData(PxSerializationContext& stream)
{
	PX_ALLOCA(buf, ClothBulkData, 1);
	Cm::markSerializedMem(buf, sizeof(ClothBulkData));
	ClothBulkData* bulkData = PX_PLACEMENT_NEW(buf, ClothBulkData);
	updateBulkData(*bulkData);	
	stream.writeData(bulkData, sizeof(ClothBulkData));
	bulkData->exportExtraData(stream);
	bulkData->~ClothBulkData();
}


void Sc::ClothCore::importExtraData(PxDeserializationContext& context)
{
	mBulkData = context.readExtraData<ClothBulkData>();
	mBulkData->importExtraData(context);
}

//~PX_SERIALIZATION


void Sc::ClothCore::setParticles(const PxClothParticle* currentParticles, const PxClothParticle* previousParticles)
{
	if (currentParticles)
	{
		cloth::MappedRange<PxVec4> particlesRange = mLowLevelCloth->getCurrentParticles();
		if(reinterpret_cast<const PxClothParticle*>(particlesRange.begin()) != currentParticles)
			PxMemCopy(particlesRange.begin(), currentParticles, particlesRange.size() * sizeof(PxVec4));
	}

	if (previousParticles)
	{
		cloth::MappedRange<PxVec4> particlesRange = mLowLevelCloth->getPreviousParticles();
		if(reinterpret_cast<const PxClothParticle*>(particlesRange.begin()) != previousParticles)
			PxMemCopy(particlesRange.begin(), previousParticles, particlesRange.size() * sizeof(PxVec4));
	}
}

PxU32 Sc::ClothCore::getNbParticles() const 
{
	return mLowLevelCloth->getNumParticles();
}

void Sc::ClothCore::setMotionConstraints(const PxClothParticleMotionConstraint* motionConstraints)
{
	if(motionConstraints)
	{
		cloth::Range<PxVec4> motionConstraintsRange = mLowLevelCloth->getMotionConstraints();
		PxMemCopy(motionConstraintsRange.begin(), motionConstraints, 
			PxU32(motionConstraintsRange.size() * sizeof(PxClothParticleMotionConstraint)));
	} else {
		mLowLevelCloth->clearMotionConstraints();
	}
}

bool Sc::ClothCore::getMotionConstraints(PxClothParticleMotionConstraint* motionConstraintsBuffer) const
{
	PxU32 nbConstraints = mLowLevelCloth->getNumMotionConstraints();

	if (nbConstraints)
	{
		PxVec4* constrData = reinterpret_cast<PxVec4*>(motionConstraintsBuffer);
		cloth::Range<PxVec4> constrRange = createRange<PxVec4>(constrData, nbConstraints);

		mLowLevelCloth->getFactory().extractMotionConstraints(*mLowLevelCloth, constrRange);

		return true;
	}
	else
		return false;
}

PxU32 Sc::ClothCore::getNbMotionConstraints() const
{
	return mLowLevelCloth->getNumMotionConstraints();
}

void Sc::ClothCore::setMotionConstraintConfig(const PxClothMotionConstraintConfig& config)
{
	mLowLevelCloth->setMotionConstraintScaleBias(config.scale, config.bias);
	mLowLevelCloth->setMotionConstraintStiffness(config.stiffness);
}


PxClothMotionConstraintConfig Sc::ClothCore::getMotionConstraintConfig() const
{
	PxClothMotionConstraintConfig result;
	result.scale = mLowLevelCloth->getMotionConstraintScale();
	result.bias = mLowLevelCloth->getMotionConstraintBias();
	result.stiffness = mLowLevelCloth->getMotionConstraintStiffness();
	return result;
}

void Sc::ClothCore::setSeparationConstraints(const PxClothParticleSeparationConstraint* separationConstraints)
{
	if (separationConstraints)
	{
		cloth::Range<PxVec4> separationConstraintsRange = mLowLevelCloth->getSeparationConstraints();
		PxMemCopy(separationConstraintsRange.begin(), separationConstraints, 
			PxU32(separationConstraintsRange.size() * sizeof(PxClothParticleSeparationConstraint)));
	} else {
		mLowLevelCloth->clearSeparationConstraints();
	}
}

bool Sc::ClothCore::getSeparationConstraints(PxClothParticleSeparationConstraint* separationConstraintsBuffer) const
{
	PxU32 nbConstraints = mLowLevelCloth->getNumSeparationConstraints();

	if (nbConstraints)
	{
		PxVec4* constrData = reinterpret_cast<PxVec4*>(separationConstraintsBuffer);
		cloth::Range<PxVec4> constrRange = createRange<PxVec4>(constrData, nbConstraints);

		mLowLevelCloth->getFactory().extractSeparationConstraints(*mLowLevelCloth, constrRange);

		return true;
	}
	else
		return false;
}

PxU32 Sc::ClothCore::getNbSeparationConstraints() const
{
	return mLowLevelCloth->getNumSeparationConstraints();
}

void Sc::ClothCore::clearInterpolation()
{
	return mLowLevelCloth->clearInterpolation();
}

void Sc::ClothCore::setParticleAccelerations(const PxVec4* particleAccelerations)
{
	if(particleAccelerations)
	{
		cloth::Range<PxVec4> accelerationsRange = mLowLevelCloth->getParticleAccelerations();
		PxMemCopy(accelerationsRange.begin(), 
			particleAccelerations, accelerationsRange.size() * sizeof(PxVec4));
	} else {
		mLowLevelCloth->clearParticleAccelerations();
	}
}

bool Sc::ClothCore::getParticleAccelerations(PxVec4* particleAccelerationBuffer) const
{
	PxU32 nbAccelerations = mLowLevelCloth->getNumParticleAccelerations();

	if (nbAccelerations)
	{
		PxVec4* accelData = reinterpret_cast<PxVec4*>(particleAccelerationBuffer);
		cloth::Range<PxVec4> accelRange = createRange<PxVec4>(accelData, nbAccelerations);

		mLowLevelCloth->getFactory().extractParticleAccelerations(*mLowLevelCloth, accelRange);

		return true;
	}
	else
		return false;
}

PxU32 Sc::ClothCore::getNbParticleAccelerations() const
{
	return mLowLevelCloth->getNumParticleAccelerations();
}


void Sc::ClothCore::addCollisionSphere(const PxClothCollisionSphere& sphere)
{
	const PxVec4* data = reinterpret_cast<const PxVec4*>(&sphere);
	mLowLevelCloth->setSpheres(createRange<const PxVec4>(data, 1), mNumUserSpheres, mNumUserSpheres);
	++mNumUserSpheres;
}
void Sc::ClothCore::removeCollisionSphere(PxU32 index)
{
	PxU32 numCapsules = mLowLevelCloth->getNumCapsules();
	mLowLevelCloth->setSpheres(cloth::Range<const PxVec4>(), index, index+1);
	mNumUserCapsules -= numCapsules - mLowLevelCloth->getNumCapsules();
	--mNumUserSpheres;
}
void Sc::ClothCore::setCollisionSpheres(const PxClothCollisionSphere* spheresBuffer, PxU32 count)
{
	const PxVec4* data = reinterpret_cast<const PxVec4*>(spheresBuffer);
	PxU32 numCapsules = mLowLevelCloth->getNumCapsules();
	mLowLevelCloth->setSpheres(createRange<const PxVec4>(data, count), 0, mNumUserSpheres);
	mNumUserCapsules -= numCapsules - mLowLevelCloth->getNumCapsules();
	mNumUserSpheres = count;
}
PxU32 Sc::ClothCore::getNbCollisionSpheres() const
{
	return mLowLevelCloth->getNumSpheres();
}

void Sc::ClothCore::getCollisionData(PxClothCollisionSphere* spheresBuffer, PxU32* capsulesBuffer,
	PxClothCollisionPlane* planesBuffer, PxU32* convexesBuffer, PxClothCollisionTriangle* trianglesBuffer) const
{
	PxVec4* sphereData = reinterpret_cast<PxVec4*>(spheresBuffer);
	cloth::Range<PxVec4> sphereRange = createRange<PxVec4>(sphereData, sphereData ? mLowLevelCloth->getNumSpheres() : 0);
	uint32_t* capsuleData = reinterpret_cast<uint32_t*>(capsulesBuffer);
	cloth::Range<uint32_t> capsuleRange = createRange<uint32_t>(capsuleData, capsuleData ? 2*mLowLevelCloth->getNumCapsules() : 0);
	PxVec4* planeData = reinterpret_cast<PxVec4*>(planesBuffer);
	cloth::Range<PxVec4> planeRange = createRange<PxVec4>(planeData, planeData ? mLowLevelCloth->getNumPlanes() : 0);
	uint32_t* convexData = reinterpret_cast<uint32_t*>(convexesBuffer);
	cloth::Range<uint32_t> convexRange = createRange<uint32_t>(convexData, convexData ? mLowLevelCloth->getNumConvexes() : 0);
	PxVec3* triangleData = reinterpret_cast<PxVec3*>(trianglesBuffer);
	cloth::Range<PxVec3> triangleRange = createRange<PxVec3>(triangleData, triangleData ? mLowLevelCloth->getNumTriangles() * 3 : 0);
	
	mLowLevelCloth->getFactory().extractCollisionData(*mLowLevelCloth, sphereRange, capsuleRange, planeRange, convexRange, triangleRange);
}

void Sc::ClothCore::addCollisionCapsule(PxU32 first, PxU32 second)
{
	PxU32 indices[2] = { first, second };
	mLowLevelCloth->setCapsules(createRange<const PxU32>(indices, 2), mNumUserCapsules, mNumUserCapsules);
	++mNumUserCapsules;
}
void Sc::ClothCore::removeCollisionCapsule(PxU32 index)
{
	mLowLevelCloth->setCapsules(cloth::Range<const PxU32>(), index, index+1);
	--mNumUserCapsules;
}
PxU32 Sc::ClothCore::getNbCollisionCapsules() const
{
	return mLowLevelCloth->getNumCapsules();
}

void Sc::ClothCore::addCollisionTriangle(const PxClothCollisionTriangle& triangle)
{
	mLowLevelCloth->setTriangles(createRange<const PxVec3>(&triangle, 3), mNumUserTriangles, mNumUserTriangles);
	++mNumUserTriangles;
}
void Sc::ClothCore::removeCollisionTriangle(PxU32 index)
{
	mLowLevelCloth->setTriangles(cloth::Range<const PxVec3>(), index, index+1);
	--mNumUserTriangles;
}
void Sc::ClothCore::setCollisionTriangles(const PxClothCollisionTriangle* trianglesBuffer, PxU32 count)
{
	mLowLevelCloth->setTriangles(createRange<const PxVec3>(trianglesBuffer, count*3), 0, mNumUserTriangles);
	mNumUserTriangles = count;
}
PxU32 Sc::ClothCore::getNbCollisionTriangles() const
{
	return mLowLevelCloth->getNumTriangles();
}

void Sc::ClothCore::addCollisionPlane(const PxClothCollisionPlane& plane)
{
	mLowLevelCloth->setPlanes(createRange<const PxVec4>(&plane, 1), mNumUserPlanes, mNumUserPlanes);
	++mNumUserPlanes;
}
void Sc::ClothCore::removeCollisionPlane(PxU32 index)
{
	PxU32 numConvexes = mLowLevelCloth->getNumConvexes();
	mLowLevelCloth->setPlanes(cloth::Range<const PxVec4>(), index, index+1);
	mNumUserConvexes -= numConvexes - mLowLevelCloth->getNumConvexes();
	--mNumUserPlanes;
}
void Sc::ClothCore::setCollisionPlanes(const PxClothCollisionPlane* planesBuffer, PxU32 count)
{
	PxU32 numConvexes = mLowLevelCloth->getNumConvexes();
	mLowLevelCloth->setPlanes(createRange<const PxVec4>(planesBuffer, count), 0, mNumUserPlanes);
	mNumUserConvexes -= numConvexes - mLowLevelCloth->getNumConvexes();
	mNumUserPlanes = count;
}
PxU32 Sc::ClothCore::getNbCollisionPlanes() const
{
	return mLowLevelCloth->getNumPlanes();
}

void Sc::ClothCore::addCollisionConvex(PxU32 mask)
{
	mLowLevelCloth->setConvexes(createRange<const PxU32>(&mask, 1), mNumUserConvexes, mNumUserConvexes);
	++mNumUserConvexes;
}
void Sc::ClothCore::removeCollisionConvex(PxU32 index)
{
	mLowLevelCloth->setConvexes(cloth::Range<const PxU32>(), index, index+1);
	--mNumUserConvexes;
}
PxU32 Sc::ClothCore::getNbCollisionConvexes() const
{
	return mLowLevelCloth->getNumConvexes();
}

void Sc::ClothCore::setVirtualParticles(PxU32 numParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights)
{
	cloth::Range<const PxU32[4]> vertexAndWeightIndicesRange = createRange<const PxU32[4]>(indices, numParticles);
	cloth::Range<const PxVec3> weightTableRange = createRange<const PxVec3>(weights, numWeights);

	mLowLevelCloth->setVirtualParticles(vertexAndWeightIndicesRange, weightTableRange);
}


PxU32 Sc::ClothCore::getNbVirtualParticles() const
{
	return mLowLevelCloth->getNumVirtualParticles();
}


void Sc::ClothCore::getVirtualParticles(PxU32* indicesBuffer) const
{
	PxU32 numVParticles = PxU32(mLowLevelCloth->getNumVirtualParticles());
	PxU32 (*data)[4] = reinterpret_cast<PxU32(*)[4]>(indicesBuffer);
	cloth::Range<PxU32[4]> vParticleRange(data, data + numVParticles);
	cloth::Range<PxVec3> weightTableRange;

	mLowLevelCloth->getFactory().extractVirtualParticles(*mLowLevelCloth, vParticleRange, weightTableRange);
}


PxU32 Sc::ClothCore::getNbVirtualParticleWeights() const
{
	return mLowLevelCloth->getNumVirtualParticleWeights();
}


void Sc::ClothCore::getVirtualParticleWeights(PxVec3* weightsBuffer) const
{
	PxU32 numWeights = PxU32(mLowLevelCloth->getNumVirtualParticleWeights());
	cloth::Range<PxU32[4]> indicesRange;
	cloth::Range<PxVec3> weightsRange(weightsBuffer, weightsBuffer + numWeights);

	mLowLevelCloth->getFactory().extractVirtualParticles(*mLowLevelCloth, indicesRange, weightsRange);
}


PxTransform Sc::ClothCore::getGlobalPose() const
{
	PxTransform pose(mLowLevelCloth->getTranslation(), mLowLevelCloth->getRotation());	
	return pose;
}


void Sc::ClothCore::setGlobalPose(const PxTransform& pose)
{
	mLowLevelCloth->setTranslation(pose.p);
	mLowLevelCloth->setRotation(pose.q);
	mLowLevelCloth->clearInertia();
}

PxVec3 Sc::ClothCore::getLinearInertiaScale() const
{
	return mLowLevelCloth->getLinearInertia();
}

void Sc::ClothCore::setLinearInertiaScale(PxVec3 scale)
{
	mLowLevelCloth->setLinearInertia(scale);
}

PxVec3 Sc::ClothCore::getAngularInertiaScale() const
{
	return mLowLevelCloth->getAngularInertia();
}

void Sc::ClothCore::setAngularInertiaScale(PxVec3 scale)
{
	mLowLevelCloth->setAngularInertia(scale);
}

PxVec3 Sc::ClothCore::getCentrifugalInertiaScale() const
{
	return mLowLevelCloth->getCentrifugalInertia();
}

void Sc::ClothCore::setCentrifugalInertiaScale(PxVec3 scale)
{
	mLowLevelCloth->setCentrifugalInertia(scale);
}

void Sc::ClothCore::setTargetPose(const PxTransform& pose)
{
	//!!!CL todo: behavior for free-standing cloth?
	mLowLevelCloth->setTranslation(pose.p);
	mLowLevelCloth->setRotation(pose.q);
}


void Sc::ClothCore::setExternalAcceleration(PxVec3 acceleration)
{
	mExternalAcceleration = acceleration;
}


PxVec3 Sc::ClothCore::getExternalAcceleration() const
{
	return mExternalAcceleration;
}


void Sc::ClothCore::setDampingCoefficient(PxVec3 dampingCoefficient)
{
	mLowLevelCloth->setDamping(dampingCoefficient);
}


PxVec3 Sc::ClothCore::getDampingCoefficient() const
{
	return mLowLevelCloth->getDamping();
}

void Sc::ClothCore::setFrictionCoefficient(PxReal frictionCoefficient)
{
	mLowLevelCloth->setFriction(frictionCoefficient);
}


PxReal Sc::ClothCore::getFrictionCoefficient() const
{
	return mLowLevelCloth->getFriction();
}

void Sc::ClothCore::setLinearDragCoefficient(PxVec3 dragCoefficient)
{
	mLowLevelCloth->setLinearDrag(dragCoefficient);
}

PxVec3 Sc::ClothCore::getLinearDragCoefficient() const
{
	return mLowLevelCloth->getLinearDrag();
}
void Sc::ClothCore::setAngularDragCoefficient(PxVec3 dragCoefficient)
{
	mLowLevelCloth->setAngularDrag(dragCoefficient);
}

PxVec3 Sc::ClothCore::getAngularDragCoefficient() const
{
	return mLowLevelCloth->getAngularDrag();
}

void Sc::ClothCore::setCollisionMassScale(PxReal scalingCoefficient)
{
	mLowLevelCloth->setCollisionMassScale(scalingCoefficient);
}
PxReal Sc::ClothCore::getCollisionMassScale() const
{
	return mLowLevelCloth->getCollisionMassScale();
}

void Sc::ClothCore::setSelfCollisionDistance(PxReal distance)
{
	mLowLevelCloth->setSelfCollisionDistance(distance);
}
PxReal Sc::ClothCore::getSelfCollisionDistance() const
{
	return mLowLevelCloth->getSelfCollisionDistance();
}
void Sc::ClothCore::setSelfCollisionStiffness(PxReal stiffness)
{
	mLowLevelCloth->setSelfCollisionStiffness(stiffness);
}
PxReal Sc::ClothCore::getSelfCollisionStiffness() const
{
	return mLowLevelCloth->getSelfCollisionStiffness();
}

void Sc::ClothCore::setSelfCollisionIndices(const PxU32* indices, PxU32 nbIndices)
{
	mLowLevelCloth->setSelfCollisionIndices(
		cloth::Range<const PxU32>(indices, indices + nbIndices));
}

bool Sc::ClothCore::getSelfCollisionIndices(PxU32* indices) const
{
	PxU32 nbIndices = mLowLevelCloth->getNumSelfCollisionIndices();
	mLowLevelCloth->getFactory().extractSelfCollisionIndices(
		*mLowLevelCloth, cloth::Range<PxU32>(indices, indices + nbIndices));
	return !!nbIndices;
}

PxU32 Sc::ClothCore::getNbSelfCollisionIndices() const
{
	return mLowLevelCloth->getNumSelfCollisionIndices();
}

void Sc::ClothCore::setRestPositions(const PxVec4* restPositions)
{
	PxU32 size = restPositions ? mLowLevelCloth->getNumParticles() : 0;
	mLowLevelCloth->setRestPositions(cloth::Range<const PxVec4>(restPositions, restPositions + size));
}

bool Sc::ClothCore::getRestPositions(PxVec4* restPositions) const
{
	PxU32 size = mLowLevelCloth->getNumRestPositions();
	mLowLevelCloth->getFactory().extractRestPositions(*mLowLevelCloth, 
		cloth::Range<PxVec4>(restPositions, restPositions + size));
	return true;
}

PxU32 Sc::ClothCore::getNbRestPositions() const
{
	return mLowLevelCloth->getNumRestPositions();
}

void Sc::ClothCore::setSolverFrequency(PxReal solverFreq)
{
	mLowLevelCloth->setSolverFrequency(solverFreq);
}

PxReal Sc::ClothCore::getSolverFrequency() const
{
	return mLowLevelCloth->getSolverFrequency();
}

void Sc::ClothCore::setStiffnessFrequency(PxReal frequency)
{
	mLowLevelCloth->setStiffnessFrequency(frequency);
}


PxReal Sc::ClothCore::getStiffnessFrequency() const
{
	return mLowLevelCloth->getStiffnessFrequency();
}

void physx::Sc::ClothCore::setStretchConfig(PxClothFabricPhaseType::Enum type, const PxClothStretchConfig& config)
{
	cloth::PhaseConfig pc;
	pc.mStiffness = config.stiffness;
	pc.mStiffnessMultiplier = config.stiffnessMultiplier;
	pc.mStretchLimit = config.stretchLimit;
	pc.mCompressionLimit = config.compressionLimit;

	PxU32 nbPhases = mFabric->getNbPhases();
	for(PxU32 i=0; i < nbPhases; i++)
	{
		// update phase config copies (but preserve existing phase index)
		if (mFabric->getPhaseType(i) == type)
		{		
			mPhaseConfigs[i].mStiffness = config.stiffness;
			mPhaseConfigs[i].mStiffnessMultiplier = config.stiffnessMultiplier;
			mPhaseConfigs[i].mCompressionLimit = config.compressionLimit;
			mPhaseConfigs[i].mStretchLimit = config.stretchLimit;
		}
	}

	// set to low level
	cloth::Range<const cloth::PhaseConfig> phaseConfigRange = createRange<const cloth::PhaseConfig>(mPhaseConfigs, nbPhases);
	mLowLevelCloth->setPhaseConfig(phaseConfigRange);
}

void Sc::ClothCore::setTetherConfig(const PxClothTetherConfig& config)
{
	mLowLevelCloth->setTetherConstraintScale(config.stretchLimit);
	mLowLevelCloth->setTetherConstraintStiffness(config.stiffness);
}

PxClothStretchConfig Sc::ClothCore::getStretchConfig(PxClothFabricPhaseType::Enum type) const
{
	cloth::PhaseConfig pc;
	PxU32 nbPhases = mFabric->getNbPhases();
	for(PxU32 i=0; i < nbPhases; i++)
	{
		if (mFabric->getPhaseType(i) == type)
		{
			pc = mPhaseConfigs[i];
			break;
		}
	}

	return PxClothStretchConfig(pc.mStiffness, pc.mStiffnessMultiplier, pc.mCompressionLimit, pc.mStretchLimit);
}

PxClothTetherConfig Sc::ClothCore::getTetherConfig() const
{
	return PxClothTetherConfig(
		mLowLevelCloth->getTetherConstraintStiffness(),
		mLowLevelCloth->getTetherConstraintScale());
}


void Sc::ClothCore::setClothFlags(PxClothFlags flags)
{
	PxClothFlags diff = mClothFlags ^ flags;
	mClothFlags = flags;

	if((diff & ~flags) & PxClothFlag::eSCENE_COLLISION)
		getSim()->clearCollisionShapes();

	if(diff & PxClothFlag::eSWEPT_CONTACT)
		mLowLevelCloth->enableContinuousCollision(flags.isSet(PxClothFlag::eSWEPT_CONTACT));

	if((diff & PxClothFlag::eCUDA) && getSim())
		getSim()->reinsert();
}


PxClothFlags Sc::ClothCore::getClothFlags() const
{
	return mClothFlags;
}

void Sc::ClothCore::setWindVelocity(PxVec3 value)
{
	mLowLevelCloth->setWindVelocity(value);
}
PxVec3 Sc::ClothCore::getWindVelocity() const
{
	return mLowLevelCloth->getWindVelocity();
}
void Sc::ClothCore::setDragCoefficient(PxReal value)
{
	mLowLevelCloth->setDragCoefficient(value);
}
PxReal Sc::ClothCore::getDragCoefficient() const
{
	return mLowLevelCloth->getDragCoefficient();
}
void Sc::ClothCore::setLiftCoefficient(PxReal value)
{
	mLowLevelCloth->setLiftCoefficient(value);
}
PxReal Sc::ClothCore::getLiftCoefficient() const
{
	return mLowLevelCloth->getLiftCoefficient();
}

bool Sc::ClothCore::isSleeping() const
{
	return mLowLevelCloth->isAsleep();
}


PxReal Sc::ClothCore::getSleepLinearVelocity() const
{
	return mLowLevelCloth->getSleepThreshold();
}


void Sc::ClothCore::setSleepLinearVelocity(PxReal threshold)
{
	mLowLevelCloth->setSleepThreshold(threshold);  // does wake the cloth up automatically
}


void Sc::ClothCore::setWakeCounter(PxReal wakeCounterValue)
{
	if(wakeCounterValue > PX_MAX_U32 / 1000)
		return mLowLevelCloth->setSleepTestInterval(PX_MAX_U32);

	PxU32 intervalsToSleep = PxU32(wakeCounterValue * 1000);

	// test at least 5 times until sleep, or 5 times per second
	PxU32 testInterval = PxMax(1u, PxMin(intervalsToSleep / 5, 200u));
	PxU32 afterCount = intervalsToSleep / testInterval;

	bool wasSleeping = mLowLevelCloth->isAsleep();

	// does wake the cloth up automatically
	mLowLevelCloth->setSleepTestInterval(testInterval);
	mLowLevelCloth->setSleepAfterCount(afterCount);  

	// setWakeCounter(0) should not change the sleep state, put back to sleep
	if (wasSleeping && wakeCounterValue == 0.0f)  
		mLowLevelCloth->putToSleep();
}

PxReal Sc::ClothCore::getWakeCounter() const
{
	PxU32 testInterval = mLowLevelCloth->getSleepTestInterval();
	if(testInterval == PX_MAX_U32)
		return PX_MAX_REAL;

	PxU32 afterCount = mLowLevelCloth->getSleepAfterCount();
	PxU32 passCount = mLowLevelCloth->getSleepPassCount();

	if (passCount >= afterCount)
		return 0.0f;

	return (afterCount - passCount) * testInterval / 1000.0f;
}

void Sc::ClothCore::wakeUp(PxReal wakeCounter)
{
	setWakeCounter(wakeCounter);
}


void Sc::ClothCore::putToSleep()
{
	mLowLevelCloth->putToSleep();
}


void Sc::ClothCore::getParticleData(PxClothParticleData& data)
{
	if(data.getDataAccessFlags() & PxDataAccessFlag::eDEVICE)
	{
		cloth::GpuParticles particles = mLowLevelCloth->getGpuParticles();
		data.particles = reinterpret_cast<PxClothParticle*>(particles.mCurrent);
		data.previousParticles = reinterpret_cast<PxClothParticle*>(particles.mPrevious);
	} 
	else
	{
		mLowLevelCloth->lockParticles();
		// When eWRITABLE flag is set, write happens during PxClothParticleData::unlock()
		data.particles = reinterpret_cast<PxClothParticle*>(const_cast<PxVec4*>(
			cloth::readCurrentParticles(*mLowLevelCloth).begin()));
		data.previousParticles = reinterpret_cast<PxClothParticle*>(const_cast<PxVec4*>(
			cloth::readPreviousParticles(*mLowLevelCloth).begin()));
	}
}

void Sc::ClothCore::unlockParticleData()
{
	mLowLevelCloth->unlockParticles();
}


PxReal Sc::ClothCore::getPreviousTimeStep() const
{
	return mLowLevelCloth->getPreviousIterationDt();
}


PxBounds3 Sc::ClothCore::getWorldBounds() const
{
	const PxVec3& center = reinterpret_cast<const PxVec3&>(mLowLevelCloth->getBoundingBoxCenter());
	const PxVec3& extent = reinterpret_cast<const PxVec3&>(mLowLevelCloth->getBoundingBoxScale());

	PxBounds3 localBounds = PxBounds3::centerExtents(center, extent);

	PX_ASSERT(!localBounds.isEmpty());
	return PxBounds3::transformFast(getGlobalPose(), localBounds);
}

void Sc::ClothCore::setSimulationFilterData(const PxFilterData& data)
{
	mFilterData = data;
}

PxFilterData Sc::ClothCore::getSimulationFilterData() const
{
	return mFilterData;
}

void Sc::ClothCore::setContactOffset(PxReal offset)
{
	mContactOffset = offset;
}

PxReal Sc::ClothCore::getContactOffset() const
{
	return mContactOffset;
}

void Sc::ClothCore::setRestOffset(PxReal offset)
{
	mRestOffset = offset;
}

PxReal Sc::ClothCore::getRestOffset() const
{
	return mRestOffset;
}

Sc::ClothSim* Sc::ClothCore::getSim() const
{
	return static_cast<ClothSim*>(Sc::ActorCore::getSim());
}

PxCloth* Sc::ClothCore::getPxCloth()
{		
	return gOffsetTable.convertScCloth2Px(this);
}

void Sc::ClothCore::onOriginShift(const PxVec3& shift)
{
	mLowLevelCloth->teleport(-shift);
}

void Sc::ClothCore::switchCloth(cloth::Cloth* cl)
{
	cloth::Fabric* fabric = &mLowLevelCloth->getFabric();
	cloth::Factory::Platform platform = mLowLevelCloth->getFactory().getPlatform();

	PX_DELETE(mLowLevelCloth);
	mLowLevelCloth = cl;

	// delete fabric if referenced by no lowlevel Cloth or ClothFabricCore.
	if (0 == fabric->getRefCount())
	{
		if(platform != cloth::Factory::CPU)
			mFabric->setLowLevelGpuFabric(NULL);
		PX_DELETE(fabric);
	}

	if(cl->getFactory().getPlatform() != cloth::Factory::CPU)
		mFabric->setLowLevelGpuFabric(&cl->getFabric());
}

#endif	// PX_USE_CLOTH_API
