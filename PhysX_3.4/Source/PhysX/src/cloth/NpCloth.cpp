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

#if PX_USE_CLOTH_API

#include "NpCloth.h"
#include "NpClothFabric.h"
#include "NpScene.h"
#include "NpPhysics.h"

using namespace physx;

// PX_SERIALIZATION
NpCloth::NpCloth(PxBaseFlags baseFlags) : NpClothT(baseFlags), mCloth(PxEmpty), mParticleData(*this)
{
}
//~PX_SERIALIZATION

NpCloth::NpCloth(const PxTransform& globalPose, NpClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags) :
	NpClothT(PxConcreteType::eCLOTH, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE, NULL, NULL),
	mCloth(globalPose, fabric.getScClothFabric(), particles, flags),
	mClothFabric(&fabric),
	mParticleData(*this)
{
	fabric.incRefCount();
}

NpCloth::~NpCloth()
{
	// ScClothCore deletion is buffered, but ScClothFabricCore is not
	// make sure we don't have a dangling pointer in ScClothCore.
	if(1 == mClothFabric->getRefCount())
		mCloth.resetFabric();

	mClothFabric->decRefCount();
}

// PX_SERIALIZATION
void NpCloth::resolveReferences(PxDeserializationContext& context)
{
	context.translatePxBase(mClothFabric);
	mClothFabric->incRefCount();	

	// pass fabric down to Scb
	mCloth.resolveReferences(mClothFabric->getScClothFabric());	
}

void NpCloth::requiresObjects(PxProcessPxBaseCallback& c)
{
	c.process(*mClothFabric);
}

NpCloth* NpCloth::createObject(PxU8*& address, PxDeserializationContext& context)
{
	NpCloth* obj = new (address) NpCloth(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(NpCloth);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

void NpCloth::release()
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	NpPhysics::getInstance().notifyDeletionListenersUserRelease(this, userData);

// PX_AGGREGATE
	// initially no support for aggregates
	//NpClothT::release();	// PT: added for PxAggregate
//~PX_AGGREGATE

	NpScene* npScene = NpActor::getAPIScene(*this);
	if(npScene) // scene is 0 after scheduling for remove
		npScene->removeCloth(*this);

	mCloth.destroy();
}


PxActorType::Enum NpCloth::getType() const
{
	return PxActorType::eCLOTH;
}


PxClothFabric* NpCloth::getFabric() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	Sc::ClothFabricCore* scFabric = mCloth.getFabric();

	size_t scOffset = reinterpret_cast<size_t>(&(reinterpret_cast<NpClothFabric*>(0)->getScClothFabric()));
	return reinterpret_cast<NpClothFabric*>(reinterpret_cast<char*>(scFabric)-scOffset);
}


void NpCloth::setParticles(const PxClothParticle* currentParticles, const PxClothParticle* previousParticles)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	
#if PX_CHECKED
	if (currentParticles)
		PX_CHECK_AND_RETURN(checkParticles(mCloth.getNbParticles(), currentParticles), "PxCloth::setParticles: values must be finite and inverse weight must not be negative");
	if (previousParticles)
		PX_CHECK_AND_RETURN(checkParticles(mCloth.getNbParticles(), previousParticles), "PxCloth::setParticles: values must be finite and inverse weight must not be negative");
#endif

	mCloth.setParticles(currentParticles, previousParticles);
}

PxU32 NpCloth::getNbParticles() const 
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbParticles();
}

void NpCloth::setMotionConstraints(const PxClothParticleMotionConstraint* motionConstraints)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkMotionConstraints(mCloth.getNbParticles(), motionConstraints), "PxCloth::setMotionConstraints: values must be finite and radius must not be negative");
#endif

	mCloth.setMotionConstraints(motionConstraints);
	sendPvdMotionConstraints();
}


bool NpCloth::getMotionConstraints(PxClothParticleMotionConstraint* motionConstraintsBuffer) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(motionConstraintsBuffer || !getNbMotionConstraints(), "PxCloth::getMotionConstraints: no motion constraint buffer provided!");

	return mCloth.getMotionConstraints(motionConstraintsBuffer);
}

PxU32 NpCloth::getNbMotionConstraints() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbMotionConstraints();
}

void NpCloth::setMotionConstraintConfig(const PxClothMotionConstraintConfig& config)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN((config.scale >= 0.0f), "PxCloth::setMotionConstraintConfig: scale must not be negative!");

	mCloth.setMotionConstraintConfig(config);
	sendPvdSimpleProperties();
}


PxClothMotionConstraintConfig NpCloth::getMotionConstraintConfig() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getMotionConstraintConfig();
}

void NpCloth::setSeparationConstraints(const PxClothParticleSeparationConstraint* separationConstraints)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkSeparationConstraints(mCloth.getNbParticles(), separationConstraints), "PxCloth::setSeparationConstraints: values must be finite and radius must not be negative");
#endif

	mCloth.setSeparationConstraints(separationConstraints);
	sendPvdSeparationConstraints();
}


bool NpCloth::getSeparationConstraints(PxClothParticleSeparationConstraint* separationConstraintsBuffer) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(separationConstraintsBuffer || !getNbSeparationConstraints(), "PxCloth::getSeparationConstraints: no separation constraint buffer provided!");

	return mCloth.getSeparationConstraints(separationConstraintsBuffer);
}

PxU32 NpCloth::getNbSeparationConstraints() const
{
	return mCloth.getNbSeparationConstraints();
}

void NpCloth::clearInterpolation()
{
	return mCloth.clearInterpolation();
}

void NpCloth::setParticleAccelerations(const PxVec4* particleAccelerations)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkParticleAccelerations(mCloth.getNbParticles(), particleAccelerations), "PxCloth::setParticleAccelerations: values must be finite");
#endif

	mCloth.setParticleAccelerations(particleAccelerations);
	
	//TODO: PVD support for particle accelerations
	//sendPvdParticleAccelerations();
}


bool NpCloth::getParticleAccelerations(PxVec4* particleAccelerationsBuffer) const
{
	PX_CHECK_MSG(particleAccelerationsBuffer, "PxCloth::getParticleAccelerations: no particle accelerations buffer provided!");

	return mCloth.getParticleAccelerations(particleAccelerationsBuffer);
}

PxU32 NpCloth::getNbParticleAccelerations() const
{
	return mCloth.getNbParticleAccelerations();
}


void NpCloth::addCollisionSphere(const PxClothCollisionSphere& sphere)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(mCloth.getNbCollisionSpheres() < 32, "PxCloth::addCollisionSphere: more than 32 spheres is not supported");
	PX_CHECK_AND_RETURN(checkCollisionSpheres(1, &sphere), "PxCloth::addCollisionSphere: position must be finite and radius must not be negative");

	mCloth.addCollisionSphere(sphere);
}
void NpCloth::removeCollisionSphere(PxU32 index)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.removeCollisionSphere(index);
}
void NpCloth::setCollisionSpheres(const PxClothCollisionSphere* spheresBuffer, PxU32 count)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

#if PX_CHECKED
	PX_CHECK_AND_RETURN(count <= 32, "PxCloth::setCollisionSpheres: more than 32 spheres is not supported");
	PX_CHECK_AND_RETURN(checkCollisionSpheres(count, spheresBuffer), "PxCloth::setCollisionSpheres: positions must be finite and radius must not be negative");
#endif

	mCloth.setCollisionSpheres(spheresBuffer, count);
	sendPvdCollisionSpheres();
}
PxU32 NpCloth::getNbCollisionSpheres() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbCollisionSpheres();
}

void NpCloth::getCollisionData(PxClothCollisionSphere* spheresBuffer, PxU32* capsulesBuffer,
							   PxClothCollisionPlane* planesBuffer, PxU32* convexesBuffer, PxClothCollisionTriangle* trianglesBuffer) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	mCloth.getCollisionData(spheresBuffer, capsulesBuffer, planesBuffer, convexesBuffer, trianglesBuffer);
}


void NpCloth::addCollisionCapsule(PxU32 first, PxU32 second)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(mCloth.getNbCollisionCapsules() < 32, "PxCloth::addCollisionCapsule: more than 32 capsules is not supported");

	mCloth.addCollisionCapsule(first, second);

	sendPvdCollisionCapsules();
}
void NpCloth::removeCollisionCapsule(PxU32 index)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.removeCollisionCapsule(index);
	
	sendPvdCollisionCapsules();
}
PxU32 NpCloth::getNbCollisionCapsules() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbCollisionCapsules();
}

void NpCloth::addCollisionTriangle(const PxClothCollisionTriangle& triangle)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.addCollisionTriangle(triangle);
	sendPvdCollisionTriangles();
}
void NpCloth::removeCollisionTriangle(PxU32 index)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.removeCollisionTriangle(index);
	sendPvdCollisionTriangles();
}
void NpCloth::setCollisionTriangles(const PxClothCollisionTriangle* trianglesBuffer, PxU32 count)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.setCollisionTriangles(trianglesBuffer, count);
	sendPvdCollisionTriangles();
}
PxU32 NpCloth::getNbCollisionTriangles() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbCollisionTriangles();
}

void NpCloth::addCollisionPlane(const PxClothCollisionPlane& plane)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(mCloth.getNbCollisionPlanes() < 32, "PxCloth::addCollisionPlane: more than 32 planes is not supported");

	mCloth.addCollisionPlane(plane);
	sendPvdCollisionTriangles();
}
void NpCloth::removeCollisionPlane(PxU32 index)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.removeCollisionPlane(index);
	sendPvdCollisionTriangles();
}
void NpCloth::setCollisionPlanes(const PxClothCollisionPlane* planesBuffer, PxU32 count)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(count <= 32, "PxCloth::setCollisionPlanes: more than 32 planes is not supported");

	mCloth.setCollisionPlanes(planesBuffer, count);
	sendPvdCollisionTriangles();
}
PxU32 NpCloth::getNbCollisionPlanes() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbCollisionPlanes();
}

void NpCloth::addCollisionConvex(PxU32 mask)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.addCollisionConvex(mask);
	sendPvdCollisionTriangles();
}
void NpCloth::removeCollisionConvex(PxU32 index)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.removeCollisionConvex(index);
	sendPvdCollisionTriangles();
}
PxU32 NpCloth::getNbCollisionConvexes() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbCollisionConvexes();
}

void NpCloth::setVirtualParticles(PxU32 numParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(indices, "PxCloth::setVirtualParticles: no triangle vertex and weight index buffer provided!");
	PX_CHECK_MSG(weights, "PxCloth::setVirtualParticles: no triangle vertex weight buffer provided!");

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkVirtualParticles(mCloth.getNbParticles(), numParticles, indices, numWeights, weights), 
						"PxCloth::setVirtualParticles: out of range value detected");
#endif

	mCloth.setVirtualParticles(numParticles, indices, numWeights, weights);
	sendPvdVirtualParticles();
}


PxU32 NpCloth::getNbVirtualParticles() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbVirtualParticles();
}


void NpCloth::getVirtualParticles(PxU32* indicesBuffer) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(indicesBuffer, "PxCloth::getVirtualParticles: no triangle vertex and weight index buffer provided!");

	mCloth.getVirtualParticles(indicesBuffer);
}


PxU32 NpCloth::getNbVirtualParticleWeights() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getNbVirtualParticleWeights();
}


void NpCloth::getVirtualParticleWeights(PxVec3* weightsBuffer) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(weightsBuffer, "PxCloth::getVirtualParticleWeights: no triangle vertex weight buffer provided!");

	mCloth.getVirtualParticleWeights(weightsBuffer);
}


void NpCloth::setGlobalPose(const PxTransform& pose)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(pose.isSane(), "PxCloth::setGlobalPose: invalid transform!");

	mCloth.setGlobalPose(pose.getNormalized());
	sendPvdSimpleProperties();
}


PxTransform NpCloth::getGlobalPose() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getGlobalPose();
}


void NpCloth::setTargetPose(const PxTransform& pose)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(pose.isSane(), "PxCloth::setTargetPose: invalid transform!");

	mCloth.setTargetPose(pose.getNormalized());
	sendPvdSimpleProperties();
}


void NpCloth::setExternalAcceleration(PxVec3 acceleration)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(acceleration.isFinite(), "PxCloth::setExternalAcceleration: invalid values!");

	mCloth.setExternalAcceleration(acceleration);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getExternalAcceleration() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getExternalAcceleration();
}

namespace 
{
#if PX_CHECKED
	bool isInRange(const PxVec3 vec, float low, float high)
	{
		return vec.x >= low && vec.x <= high 
			&& vec.y >= low && vec.y <= high 
			&& vec.z >= low && vec.z <= high;
	}
#endif
}

void NpCloth::setLinearInertiaScale(PxVec3 scale)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(scale, 0.0f, 1.0f), "PxCloth::setLinearInertiaScale: scale value has to be between 0 and 1!");

	mCloth.setLinearInertiaScale(scale);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getLinearInertiaScale() const
{
	return mCloth.getLinearInertiaScale();
}

void NpCloth::setAngularInertiaScale(PxVec3 scale)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(scale, 0.0f, 1.0f), "PxCloth::setAngularInertiaScale: scale value has to be between 0 and 1!");

	mCloth.setAngularInertiaScale(scale);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getAngularInertiaScale() const
{
	return mCloth.getAngularInertiaScale();
}

void NpCloth::setCentrifugalInertiaScale(PxVec3 scale)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(scale, 0.0f, 1.0f), "PxCloth::setCentrifugalInertiaScale: scale value has to be between 0 and 1!");

	mCloth.setCentrifugalInertiaScale(scale);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getCentrifugalInertiaScale() const
{
	return mCloth.getCentrifugalInertiaScale();
}

void NpCloth::setInertiaScale(float scale)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(scale >= 0.0f && scale <= 1.0f, "PxCloth::setInertiaScale: scale value has to be between 0 and 1!");

	mCloth.setLinearInertiaScale(PxVec3(scale));
	mCloth.setAngularInertiaScale(PxVec3(scale));
	sendPvdSimpleProperties();
}

void NpCloth::setDampingCoefficient(PxVec3 dampingCoefficient)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(dampingCoefficient, 0.0f, 1.0f), "PxCloth::setDampingCoefficient: damping coefficient has to be between 0 and 1!");

	mCloth.setDampingCoefficient(dampingCoefficient);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getDampingCoefficient() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getDampingCoefficient();
}


void NpCloth::setFrictionCoefficient(PxReal frictionCoefficient)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(frictionCoefficient >= 0.0f || frictionCoefficient <= 1.0f, "PxCloth::setFrictionCoefficient: friction coefficient has to be between 0 and 1!");

	mCloth.setFrictionCoefficient(frictionCoefficient);
	sendPvdSimpleProperties();
}

PxReal NpCloth::getFrictionCoefficient() const
{
	return mCloth.getFrictionCoefficient();
}

void NpCloth::setLinearDragCoefficient(PxVec3 dragCoefficient)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(dragCoefficient, 0.0f, 1.0f), "PxCloth::setLinearDragCoefficient: damping coefficient has to be between 0 and 1!");

	mCloth.setLinearDragCoefficient(dragCoefficient);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getLinearDragCoefficient() const
{
	return mCloth.getLinearDragCoefficient();
}

void NpCloth::setAngularDragCoefficient(PxVec3 dragCoefficient)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(isInRange(dragCoefficient, 0.0f, 1.0f), "PxCloth::setAngularDragCoefficient: damping coefficient has to be between 0 and 1!");

	mCloth.setAngularDragCoefficient(dragCoefficient);
	sendPvdSimpleProperties();
}


PxVec3 NpCloth::getAngularDragCoefficient() const
{
	return mCloth.getAngularDragCoefficient();
}

void NpCloth::setDragCoefficient(float coefficient)
{
	setLinearDragCoefficient(PxVec3(coefficient));
	setAngularDragCoefficient(PxVec3(coefficient));
}

void NpCloth::setCollisionMassScale(PxReal scalingCoefficient)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(scalingCoefficient >= 0.0f, "PxCloth::setCollisionMassScale: scaling coefficient has to be greater or equal than 0!");

	mCloth.setCollisionMassScale(scalingCoefficient);
}


PxReal NpCloth::getCollisionMassScale() const
{
	return mCloth.getCollisionMassScale();
}

void NpCloth::setSelfCollisionDistance(PxReal distance)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(distance >= 0.0f, "PxCloth::setSelfCollisionDistance: distance has to be greater or equal than 0!");

	mCloth.setSelfCollisionDistance(distance);
}
PxReal NpCloth::getSelfCollisionDistance() const
{
	return mCloth.getSelfCollisionDistance();
}
void NpCloth::setSelfCollisionStiffness(PxReal stiffness)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(stiffness >= 0.0f, "PxCloth::setSelfCollisionStiffness: stiffness has to be greater or equal than 0!");

	mCloth.setSelfCollisionStiffness(stiffness);
}
PxReal NpCloth::getSelfCollisionStiffness() const
{
	return mCloth.getSelfCollisionStiffness();
}

void NpCloth::setSelfCollisionIndices(const PxU32* indices, PxU32 nbIndices)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.setSelfCollisionIndices(indices, nbIndices);

	sendPvdSelfCollisionIndices();
}

bool NpCloth::getSelfCollisionIndices(PxU32* indices) const
{
	return mCloth.getSelfCollisionIndices(indices);
}

PxU32 NpCloth::getNbSelfCollisionIndices() const
{
	return mCloth.getNbSelfCollisionIndices();
}

void NpCloth::setRestPositions(const PxVec4* restPositions)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	
	mCloth.setRestPositions(restPositions);

	sendPvdRestPositions();
}

bool NpCloth::getRestPositions(PxVec4* restPositions) const
{
	return mCloth.getRestPositions(restPositions);
}

PxU32 NpCloth::getNbRestPositions() const
{
	return mCloth.getNbRestPositions();
}

void NpCloth::setSolverFrequency(PxReal frequency)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(frequency > 0.0f, "PxCloth::setSolverFrequency: solver fequency has to be positive!");

	mCloth.setSolverFrequency(frequency);
	sendPvdSimpleProperties();
}


PxReal NpCloth::getSolverFrequency() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getSolverFrequency();
}

void NpCloth::setStiffnessFrequency(PxReal frequency)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(frequency > 0.0f, "PxCloth::setStiffnessFrequency: solver fequency has to be positive!");

	mCloth.setStiffnessFrequency(frequency);
	sendPvdSimpleProperties();
}


PxReal NpCloth::getStiffnessFrequency() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getStiffnessFrequency();
}

void NpCloth::setStretchConfig(PxClothFabricPhaseType::Enum type, const PxClothStretchConfig& config)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(config.stiffness >= 0.0f, "PxCloth::setStretchConfig: stiffness must not be negative!");
	PX_CHECK_MSG(config.stiffnessMultiplier >= 0.0f, "PxCloth::setStretchConfig: stiffnessMultiplier must not be negative!");
	PX_CHECK_MSG(config.compressionLimit >= 0.0f, "PxCloth::setStretchConfig: compressionLimit must not be negative!");
	PX_CHECK_MSG(config.compressionLimit <= 1.0f, "PxCloth::setStretchConfig: compressionLimit must not be larger than 1!");
	PX_CHECK_MSG(config.stretchLimit >= 1.0f, "PxCloth::setStretchConfig: stretchLimit must not be smaller than 1!");

	mCloth.setStretchConfig(type, config);
	sendPvdSimpleProperties();
}

void NpCloth::setTetherConfig(const PxClothTetherConfig& config)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_MSG(config.stiffness >= 0.0f, "PxCloth::setTetherConfig: stiffness must not be negative!");

	mCloth.setTetherConfig(config);
	sendPvdSimpleProperties();
}

PxClothStretchConfig NpCloth::getStretchConfig(PxClothFabricPhaseType::Enum type) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mCloth.getStretchConfig(type);
}

PxClothTetherConfig NpCloth::getTetherConfig() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mCloth.getTetherConfig();
}

void NpCloth::setClothFlags(PxClothFlags flags)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.setClothFlags(flags);
	sendPvdSimpleProperties();

	NpScene* scene = NpActor::getAPIScene(*this);
	if (scene)
		scene->updatePhysXIndicator();
}

void NpCloth::setClothFlag(PxClothFlag::Enum flag, bool val)
{
	PxClothFlags flags = mCloth.getClothFlags();
	setClothFlags(val ? flags | flag : flags & ~flag);
}


PxClothFlags NpCloth::getClothFlags() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getClothFlags();
}

void NpCloth::setWindVelocity(PxVec3 value)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	mCloth.setWindVelocity(value);
	sendPvdSimpleProperties();
}

PxVec3 NpCloth::getWindVelocity() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getWindVelocity();
}

void NpCloth::setWindDrag(PxReal value)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(value >= 0.0f, "PxCloth::setWindDrag: value has to be non-negative!");

	mCloth.setDragCoefficient(value);
	sendPvdSimpleProperties();
}

PxReal NpCloth::getWindDrag() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getDragCoefficient();
}

void NpCloth::setWindLift(PxReal value)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN(value >= 0.0f, "PxCloth::setWindLift: value has to be non-negative!");

	mCloth.setLiftCoefficient(value);
	sendPvdSimpleProperties();
}

PxReal NpCloth::getWindLift() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getLiftCoefficient();
}


bool NpCloth::isSleeping() const
{
	NpScene* scene = NpActor::getOwnerScene(*this);
	PX_UNUSED(scene);

	NP_READ_CHECK(scene);
	PX_CHECK_AND_RETURN_VAL(scene, "PxCloth::isSleeping: cloth must be in a scene.", true);

	return mCloth.isSleeping();
}


PxReal NpCloth::getSleepLinearVelocity() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getSleepLinearVelocity();
}


void NpCloth::setSleepLinearVelocity(PxReal threshold)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_MSG(threshold >= 0.0f, "PxCloth::setSleepLinearVelocity: threshold must not be negative!");

	mCloth.setSleepLinearVelocity(threshold);
	sendPvdSimpleProperties();
}


void NpCloth::setWakeCounter(PxReal wakeCounterValue)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(PxIsFinite(wakeCounterValue), "PxCloth::setWakeCounter: invalid float.");
	PX_CHECK_AND_RETURN(wakeCounterValue>=0.0f, "PxCloth::setWakeCounter: wakeCounterValue must be non-negative!");

	mCloth.setWakeCounter(wakeCounterValue);
}


PxReal NpCloth::getWakeCounter() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getWakeCounter();
}


void NpCloth::wakeUp()
{
	NpScene* scene = NpActor::getOwnerScene(*this);
	PX_UNUSED(scene);

	NP_WRITE_CHECK(scene);
	PX_CHECK_AND_RETURN(scene, "PxCloth::wakeUp: cloth must be in a scene.");
	
	mCloth.wakeUp();
}


void NpCloth::putToSleep()
{
	NpScene* scene = NpActor::getOwnerScene(*this);
	PX_UNUSED(scene);

	NP_WRITE_CHECK(scene);
	PX_CHECK_AND_RETURN(scene, "PxCloth::putToSleep: cloth must be in a scene.");

	mCloth.putToSleep();
}


PxClothParticleData* NpCloth::lockParticleData(PxDataAccessFlags flags)
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	if(!mParticleData.tryLock(flags))
	{
		shdfnd::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"PxClothParticleData access through PxCloth::lockParticleData() while its still locked by last call.");
		return 0;
	}

	mCloth.getParticleData(mParticleData);
	return &mParticleData;
}

PxClothParticleData* NpCloth::lockParticleData() const
{
	return const_cast<NpCloth*>(this)->lockParticleData(PxDataAccessFlag::eREADABLE);
}

void NpCloth::unlockParticleData()
{
	mCloth.getScCloth().unlockParticleData();
}

PxReal NpCloth::getPreviousTimeStep() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	return mCloth.getPreviousTimeStep();
}


PxBounds3 NpCloth::getWorldBounds(float inflation) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	const PxBounds3 bounds = mCloth.getWorldBounds();
	PX_ASSERT(bounds.isValid());

	// PT: unfortunately we can't just scale the min/max vectors, we need to go through center/extents.
	const PxVec3 center = bounds.getCenter();
	const PxVec3 inflatedExtents = bounds.getExtents() * inflation;
	return PxBounds3::centerExtents(center, inflatedExtents);
}

void NpCloth::setSimulationFilterData(const PxFilterData& data)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	mCloth.setSimulationFilterData(data);
}

PxFilterData NpCloth::getSimulationFilterData() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mCloth.getSimulationFilterData();
}

void NpCloth::setContactOffset(PxReal offset)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	mCloth.setContactOffset(offset);
}

PxReal NpCloth::getContactOffset() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mCloth.getContactOffset();
}

void NpCloth::setRestOffset(PxReal offset)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	mCloth.setRestOffset(offset);
}

PxReal NpCloth::getRestOffset() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return mCloth.getRestOffset();
}

#if PX_CHECKED 
bool NpCloth::checkParticles(PxU32 numParticles, const PxClothParticle* particles)
{
	for (PxU32 i = 0; i < numParticles; ++i)
	{
		if(!particles[i].pos.isFinite())
			return false;
		if(!PxIsFinite(particles[i].invWeight) || (particles[i].invWeight < 0.0f))
			return false;
	}
	return true;
}

bool NpCloth::checkMotionConstraints(PxU32 numConstraints, const PxClothParticleMotionConstraint* constraints)
{
	if(!constraints)
		return true;

	for (PxU32 i = 0; i < numConstraints; ++i)
	{
		if(!constraints[i].pos.isFinite())
			return false;
		if(!PxIsFinite(constraints[i].radius) || (constraints[i].radius < 0.0f))
			return false;
	}
	return true;
}


bool NpCloth::checkSeparationConstraints(PxU32 numConstraints, const PxClothParticleSeparationConstraint* constraints)
{
	if(!constraints)
		return true;

	for (PxU32 i = 0; i < numConstraints; ++i)
	{
		if(!constraints[i].pos.isFinite())
			return false;
		if(!PxIsFinite(constraints[i].radius) || (constraints[i].radius < 0.0f))
			return false;
	}
	return true;
}

bool NpCloth::checkParticleAccelerations(PxU32 numParticles, const PxVec4* accelerations)
{
	if(!accelerations)
		return true;

	for (PxU32 i = 0; i < numParticles; ++i)
	{
		if(!accelerations[i].isFinite())
			return false;
	}
	return true;
}

bool NpCloth::checkCollisionSpheres(PxU32 numSpheres, const PxClothCollisionSphere* spheres)
{
	for (PxU32 i = 0; i < numSpheres; ++i)
	{
		if(!spheres[i].pos.isFinite())
			return false;
		if(!PxIsFinite(spheres[i].radius) || (spheres[i].radius < 0.0f))
			return false;
	}
	return true;
}

bool NpCloth::checkCollisionSpherePairs(PxU32 numSpheres, PxU32 numPairs, const PxU32* pairIndices)
{
	for (PxU32 i = 0; i < numPairs; ++i)
	{
		if ((pairIndices[2*i] >= numSpheres) || (pairIndices[2*i + 1] >= numSpheres))
			return false;
	}
	return true;
}

bool NpCloth::checkVirtualParticles(PxU32 numParticles, PxU32 numVParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights)
{
	for (PxU32 i = 0; i < numVParticles; ++i)
	{
		if ((indices[4*i] >= numParticles) || 
			(indices[4*i + 1] >= numParticles) ||
			(indices[4*i + 2] >= numParticles) ||
			(indices[4*i + 3] >= numWeights))
			return false;
	}
	for (PxU32 i = 0; i < numWeights; ++i)
	{
		if(!weights[i].isFinite())
			return false;
	}
	return true;
}
#endif

#if PX_ENABLE_DEBUG_VISUALIZATION
void NpCloth::visualize(Cm::RenderOutput& out, NpScene* scene)
{
	PxClothParticleData* readData = lockParticleData();
	if (!readData)
		return;

	NpClothFabric* fabric = static_cast<NpClothFabric*> (getFabric());

	PxU32 nbSets = fabric->getNbSets();
	PxU32 nbPhases = fabric->getNbPhases();
	PxU32 nbIndices = fabric->getNbParticleIndices();

	shdfnd::Array<PxU32> sets(nbSets);
	shdfnd::Array<PxClothFabricPhase> phases(nbPhases);
	shdfnd::Array<PxU32> indices(nbIndices);

	fabric->getSets(&sets[0], nbSets);
	fabric->getPhases(&phases[0], nbPhases);
	fabric->getParticleIndices(&indices[0], nbIndices);

	const PxU32 lineColor[] = 
	{	
		PxU32(PxDebugColor::eARGB_RED),
		PxU32(PxDebugColor::eARGB_GREEN),
		PxU32(PxDebugColor::eARGB_BLUE),
		PxU32(PxDebugColor::eARGB_YELLOW),
		PxU32(PxDebugColor::eARGB_MAGENTA)
	};

	PxU32 colorIndex = 0;

	const PxClothParticle* particles = readData->particles;
	const PxTransform xform = getGlobalPose();

	out << Cm::RenderOutput::LINES;

	for (PxU32 p=0; p < nbPhases; ++p)
	{
		PxClothFabricPhaseType::Enum phaseType = fabric->getPhaseType(p);

		float scale = 0.0f;

		// check if visualization requested
		switch(phaseType)
		{
		case PxClothFabricPhaseType::eVERTICAL:
			scale = scene->getVisualizationParameter(PxVisualizationParameter::eCLOTH_VERTICAL);
			break;
		case PxClothFabricPhaseType::eHORIZONTAL:
			scale = scene->getVisualizationParameter(PxVisualizationParameter::eCLOTH_HORIZONTAL);
			break;
		case PxClothFabricPhaseType::eBENDING:
			scale = scene->getVisualizationParameter(PxVisualizationParameter::eCLOTH_BENDING);
			break;
		case PxClothFabricPhaseType::eSHEARING:
			scale = scene->getVisualizationParameter(PxVisualizationParameter::eCLOTH_SHEARING);
			break;
		case PxClothFabricPhaseType::eINVALID:
		case PxClothFabricPhaseType::eCOUNT:
			break;
		}

		if (scale == 0.0f)
			continue;

		out << lineColor[colorIndex];

		PxU32 set = phases[p].setIndex;

		// draw one set at a time
		PxU32 iIt = set ? 2*sets[set-1] : 0;
		PxU32 iEnd = 2*sets[set]; 

		// iterate over constraints
		while (iIt < iEnd)
		{
			PxU32 i0 = indices[iIt++];
			PxU32 i1 = indices[iIt++];

			// ideally we would know the mesh normals here and bias off
			// the surface slightly but scaling slightly around the center helps
			out << xform.transform(particles[i0].pos);
			out << xform.transform(particles[i1].pos);
		}

		colorIndex = (colorIndex+1)%5;
	}

	// draw virtual particles
	if (scene->getVisualizationParameter(PxVisualizationParameter::eCLOTH_VIRTUAL_PARTICLES) > 0.0f)
	{
		PxU32 nbVirtualParticles = getNbVirtualParticles();

		if (nbVirtualParticles)
		{
			out << Cm::RenderOutput::POINTS;
			out << PxU32(PxDebugColor::eARGB_WHITE);

			shdfnd::Array<PxU32> vpIndices(nbVirtualParticles*4);
			getVirtualParticles(&vpIndices[0]);

			// get weights table
			PxU32 nbVirtualParticleWeights = getNbVirtualParticleWeights();
			shdfnd::Array<PxVec3> vpWeights(nbVirtualParticleWeights);
			getVirtualParticleWeights(&vpWeights[0]);

			for (PxU32 i=0; i < nbVirtualParticles; ++i)
			{
				PxU32 i0 = vpIndices[i*4+0];
				PxU32 i1 = vpIndices[i*4+1];
				PxU32 i2 = vpIndices[i*4+2];

				PxVec3 v0 = xform.transform(readData->particles[i0].pos);
				PxVec3 v1 = xform.transform(readData->particles[i1].pos);
				PxVec3 v2 = xform.transform(readData->particles[i2].pos);

				PxVec3 weights = vpWeights[vpIndices[i*4+3]];

				out << (v0*weights.x + v1*weights.y + v2*weights.z);
			}
		}
	}

	readData->unlock();
}

#endif

#if PX_SUPPORT_PVD			
namespace 
{
	Vd::ScbScenePvdClient* getScenePvdClient( Scb::Scene* scene )
	{
		if(scene )
		{
			Vd::ScbScenePvdClient& pvdClient = scene->getScenePvdClient();
			if(pvdClient.checkPvdDebugFlag())
				return &pvdClient;
		}
		return NULL;
	}
}
#endif

void NpCloth::sendPvdSimpleProperties()
{
#if PX_SUPPORT_PVD			
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );
	if ( pvdClient ) pvdClient->sendSimpleProperties( &mCloth );
#endif
}

void NpCloth::sendPvdMotionConstraints()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendMotionConstraints( &mCloth );
#endif
}

void NpCloth::sendPvdSelfCollisionIndices()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendSelfCollisionIndices( &mCloth );
#endif
}

void NpCloth::sendPvdRestPositions()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendRestPositions( &mCloth );
#endif
}

void NpCloth::sendPvdSeparationConstraints()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
    if ( pvdClient ) pvdClient->sendSeparationConstraints( &mCloth );
#endif
}

void NpCloth::sendPvdCollisionSpheres()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendCollisionSpheres( &mCloth );
#endif
}

void NpCloth::sendPvdCollisionCapsules()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendCollisionCapsules( &mCloth );
#endif
}

void NpCloth::sendPvdCollisionTriangles()
{
#if PX_SUPPORT_PVD
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );					
	if ( pvdClient ) pvdClient->sendCollisionTriangles( &mCloth );
#endif
}

void NpCloth::sendPvdVirtualParticles()
{
#if PX_SUPPORT_PVD						
	Vd::ScbScenePvdClient* pvdClient = getScenePvdClient( mCloth.getScbSceneForAPI() );						
	if ( pvdClient ) pvdClient->sendVirtualParticles( &mCloth );
#endif
}
 
#endif // PX_USE_CLOTH_API
