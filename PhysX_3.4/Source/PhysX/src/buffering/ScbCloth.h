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

#ifndef PX_PHYSICS_SCB_CLOTH
#define PX_PHYSICS_SCB_CLOTH

#include "PxPhysXConfig.h"

#if PX_USE_CLOTH_API

#include "ScbActor.h"

#include "ScClothCore.h"
#include "NpClothParticleData.h"

namespace physx
{
struct PxClothCollisionSphere;

namespace Scb
{
class Cloth : public Scb::Actor
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================

public:
// PX_SERIALIZATION
											Cloth(const PxEMPTY) : Scb::Actor(PxEmpty), mCloth(PxEmpty) {}
				void						exportExtraData(PxSerializationContext& stream) { mCloth.exportExtraData(stream); }
				void						importExtraData(PxDeserializationContext &context) { mCloth.importExtraData(context); }
				void						resolveReferences(Sc::ClothFabricCore& fabric) { mCloth.resolveReferences(fabric); }
				static void					getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
											Cloth(const PxTransform& globalPose, Sc::ClothFabricCore& fabric, const PxClothParticle* particles, PxClothFlags flags);
											~Cloth();

	//---------------------------------------------------------------------------------
	// Wrapper for Sc::ClothCore interface
	//---------------------------------------------------------------------------------

	PX_INLINE Sc::ClothFabricCore*		getFabric() const	{ return mCloth.getFabric();	}
	PX_INLINE void						resetFabric()		{ return mCloth.resetFabric();	}

	PX_INLINE void						setParticles(const PxClothParticle* currentParticles, const PxClothParticle* previousParticles);
	PX_INLINE PxU32						getNbParticles() const;

	PX_INLINE void						setMotionConstraints(const PxClothParticleMotionConstraint* motionConstraints);
	PX_INLINE bool						getMotionConstraints(PxClothParticleMotionConstraint* motionConstraintsBuffer) const;
	PX_INLINE PxU32						getNbMotionConstraints() const;

	PX_INLINE PxClothMotionConstraintConfig getMotionConstraintConfig() const;
	PX_INLINE void						setMotionConstraintConfig(const PxClothMotionConstraintConfig& config);

	PX_INLINE void						setSeparationConstraints(const PxClothParticleSeparationConstraint* separationConstraints);
	PX_INLINE bool						getSeparationConstraints(PxClothParticleSeparationConstraint* separationConstraintsBuffer) const;
	PX_INLINE PxU32						getNbSeparationConstraints() const;

	PX_INLINE void						clearInterpolation();

	PX_INLINE void						setParticleAccelerations(const PxVec4* particleAccelerations);
	PX_INLINE bool						getParticleAccelerations(PxVec4* particleAccelerationsBuffer) const;
	PX_INLINE PxU32						getNbParticleAccelerations() const;

	PX_INLINE void						addCollisionSphere(const PxClothCollisionSphere& sphere);
	PX_INLINE void						removeCollisionSphere(PxU32 index);
	PX_INLINE void						setCollisionSpheres(const PxClothCollisionSphere* spheresBuffer, PxU32 count);
	PX_INLINE PxU32						getNbCollisionSpheres() const;

	PX_INLINE void						getCollisionData(PxClothCollisionSphere* spheresBuffer, PxU32* capsulesBuffer, 
											PxClothCollisionPlane* planesBuffer, PxU32* convexesBuffer, PxClothCollisionTriangle* trianglesBuffer) const;

	PX_INLINE void						addCollisionCapsule(PxU32 first, PxU32 second);
	PX_INLINE void						removeCollisionCapsule(PxU32 index);
	PX_INLINE PxU32						getNbCollisionCapsules() const;

	PX_INLINE void						addCollisionTriangle(const PxClothCollisionTriangle& triangle);
	PX_INLINE void						removeCollisionTriangle(PxU32 index);
	PX_INLINE void						setCollisionTriangles(const PxClothCollisionTriangle* trianglesBuffer, PxU32 count);
	PX_INLINE PxU32						getNbCollisionTriangles() const;

	PX_INLINE void						addCollisionPlane(const PxClothCollisionPlane& plane);
	PX_INLINE void						removeCollisionPlane(PxU32 index);
	PX_INLINE void						setCollisionPlanes(const PxClothCollisionPlane* planesBuffer, PxU32 count);
	PX_INLINE PxU32						getNbCollisionPlanes() const;

	PX_INLINE void						addCollisionConvex(PxU32 mask);
	PX_INLINE void						removeCollisionConvex(PxU32 index);
	PX_INLINE PxU32						getNbCollisionConvexes() const;

	PX_INLINE void						setVirtualParticles(PxU32 numParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights);

	PX_INLINE PxU32						getNbVirtualParticles() const;
	PX_INLINE void						getVirtualParticles(PxU32* indicesBuffer) const;

	PX_INLINE PxU32						getNbVirtualParticleWeights() const;
	PX_INLINE void						getVirtualParticleWeights(PxVec3* weightsBuffer) const;

	PX_INLINE PxTransform				getGlobalPose() const;
	PX_INLINE void						setGlobalPose(const PxTransform& pose);

	PX_INLINE void						setTargetPose(const PxTransform& pose);

	PX_INLINE PxVec3					getExternalAcceleration() const;
	PX_INLINE void						setExternalAcceleration(PxVec3 acceleration);

	PX_INLINE PxVec3					getLinearInertiaScale() const;
	PX_INLINE void						setLinearInertiaScale(PxVec3 scale);
	PX_INLINE PxVec3					getAngularInertiaScale() const;
	PX_INLINE void						setAngularInertiaScale(PxVec3 scale);
	PX_INLINE PxVec3					getCentrifugalInertiaScale() const;
	PX_INLINE void						setCentrifugalInertiaScale(PxVec3 scale);

	PX_INLINE PxVec3					getDampingCoefficient() const;
	PX_INLINE void						setDampingCoefficient(PxVec3 dampingCoefficient);

	PX_INLINE PxReal					getFrictionCoefficient() const;
	PX_INLINE void						setFrictionCoefficient(PxReal frictionCoefficient);

	PX_INLINE PxVec3					getLinearDragCoefficient() const;
	PX_INLINE void						setLinearDragCoefficient(PxVec3 dragCoefficient);
	PX_INLINE PxVec3					getAngularDragCoefficient() const;
	PX_INLINE void						setAngularDragCoefficient(PxVec3 dragCoefficient);

	PX_INLINE PxReal					getCollisionMassScale() const;
	PX_INLINE void						setCollisionMassScale(PxReal scalingCoefficient);

	PX_INLINE void						setSelfCollisionDistance(PxReal distance);
	PX_INLINE PxReal					getSelfCollisionDistance() const;
	PX_INLINE void						setSelfCollisionStiffness(PxReal stiffness);
	PX_INLINE PxReal					getSelfCollisionStiffness() const;

	PX_INLINE void						setSelfCollisionIndices(const PxU32* indices, PxU32 nbIndices);
	PX_INLINE bool						getSelfCollisionIndices(PxU32* indices) const;
	PX_INLINE PxU32						getNbSelfCollisionIndices() const;

	PX_INLINE void						setRestPositions(const PxVec4* restPositions);
	PX_INLINE bool						getRestPositions(PxVec4* restPositions) const;
	PX_INLINE PxU32						getNbRestPositions() const; 

	PX_INLINE PxReal					getSolverFrequency() const;
	PX_INLINE void						setSolverFrequency(PxReal);

	PX_INLINE PxReal					getStiffnessFrequency() const;
	PX_INLINE void						setStiffnessFrequency(PxReal);

	PX_INLINE void						setStretchConfig(PxClothFabricPhaseType::Enum type, const PxClothStretchConfig& config);
	PX_INLINE void						setTetherConfig(const PxClothTetherConfig& config);

	PX_INLINE PxClothStretchConfig		getStretchConfig(PxClothFabricPhaseType::Enum type) const;
	PX_INLINE PxClothTetherConfig		getTetherConfig() const;

	PX_INLINE PxClothFlags				getClothFlags() const;
	PX_INLINE void						setClothFlags(PxClothFlags flags);

	PX_INLINE PxVec3					getWindVelocity() const;
	PX_INLINE void						setWindVelocity(PxVec3);
	PX_INLINE PxReal					getDragCoefficient() const;
	PX_INLINE void						setDragCoefficient(PxReal);
	PX_INLINE PxReal					getLiftCoefficient() const;
	PX_INLINE void						setLiftCoefficient(PxReal);

	PX_INLINE bool						isSleeping() const;
	PX_INLINE PxReal					getSleepLinearVelocity() const;
	PX_INLINE void						setSleepLinearVelocity(PxReal threshold);
	PX_INLINE void						setWakeCounter(PxReal wakeCounterValue);
	PX_INLINE PxReal					getWakeCounter() const;
	PX_INLINE void						wakeUp();
	PX_INLINE void						putToSleep();

	PX_INLINE void						getParticleData(NpClothParticleData&);

	PX_INLINE PxReal					getPreviousTimeStep() const;

	PX_INLINE PxBounds3					getWorldBounds() const;

	PX_INLINE void						setSimulationFilterData(const PxFilterData& data);
	PX_INLINE PxFilterData				getSimulationFilterData() const;

	PX_INLINE void						setContactOffset(PxReal);
	PX_INLINE PxReal					getContactOffset() const;
	PX_INLINE void						setRestOffset(PxReal);
	PX_INLINE PxReal					getRestOffset() const;

	//---------------------------------------------------------------------------------
	// Data synchronization
	//---------------------------------------------------------------------------------
				
	// Synchronously called with fetchResults.
	void syncState();

	//---------------------------------------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------------------------------------
	PX_FORCE_INLINE	const Sc::ClothCore&		getScCloth()		const	{ return mCloth; }  // Only use if you know what you're doing!
	PX_FORCE_INLINE	Sc::ClothCore&				getScCloth()				{ return mCloth; }  // Only use if you know what you're doing!

	static size_t getScOffset()	{ return reinterpret_cast<size_t>(&reinterpret_cast<Cloth*>(0)->mCloth);	}

private:
	Sc::ClothCore		mCloth;
};

PX_INLINE void Cloth::setParticles(const PxClothParticle* currentParticles, const PxClothParticle* previousParticles)
{
	if(!isBuffering())
		mCloth.setParticles(currentParticles, previousParticles);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setParticles() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbParticles() const 
{
	return mCloth.getNbParticles();
}

PX_INLINE void Cloth::setMotionConstraints(const PxClothParticleMotionConstraint* motionConstraints)
{
	if(!isBuffering())
		mCloth.setMotionConstraints(motionConstraints);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setMotionConstraints() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::getMotionConstraints(PxClothParticleMotionConstraint* motionConstraintsBuffer) const
{
	if(!isBuffering())
		return mCloth.getMotionConstraints(motionConstraintsBuffer);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getMotionConstraints() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxU32 Cloth::getNbMotionConstraints() const
{
	return mCloth.getNbMotionConstraints();
}

PX_INLINE PxClothMotionConstraintConfig Cloth::getMotionConstraintConfig() const
{
	if(!isBuffering())
		return mCloth.getMotionConstraintConfig();
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getMotionConstraintScaleBias() not allowed while simulation is running.");

	return PxClothMotionConstraintConfig();
}

PX_INLINE void Cloth::setMotionConstraintConfig(const PxClothMotionConstraintConfig& config)
{
	if(!isBuffering())
		mCloth.setMotionConstraintConfig(config);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setMotionConstraintConfig() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setSeparationConstraints(const PxClothParticleSeparationConstraint* separationConstraints)
{
	if(!isBuffering())
		mCloth.setSeparationConstraints(separationConstraints);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSeparationConstraints() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::getSeparationConstraints(PxClothParticleSeparationConstraint* separationConstraintsBuffer) const
{
	if(!isBuffering())
		return mCloth.getSeparationConstraints(separationConstraintsBuffer);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSeparationConstraints() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxU32 Cloth::getNbSeparationConstraints() const
{
	return mCloth.getNbSeparationConstraints();
}

PX_INLINE void Cloth::clearInterpolation()
{
	return mCloth.clearInterpolation();
}

PX_INLINE void Cloth::setParticleAccelerations(const PxVec4* particleAccelerations)
{
	if(!isBuffering())
		mCloth.setParticleAccelerations(particleAccelerations);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setParticleAccelerations() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::getParticleAccelerations(PxVec4* particleAccelerationsBuffer) const
{
	if(!isBuffering())
		return mCloth.getParticleAccelerations(particleAccelerationsBuffer);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getParticleAccelerations() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxU32 Cloth::getNbParticleAccelerations() const
{
	return mCloth.getNbParticleAccelerations();
}

PX_INLINE void Cloth::addCollisionSphere(const PxClothCollisionSphere& sphere)
{
	if(!isBuffering())
		mCloth.addCollisionSphere(sphere);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::addCollisionSphere() not allowed while simulation is running.");
}

PX_INLINE void Cloth::removeCollisionSphere(PxU32 index)
{
	if(!isBuffering())
		mCloth.removeCollisionSphere(index);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::removeCollisionSphere() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setCollisionSpheres(const PxClothCollisionSphere* spheresBuffer, PxU32 count)
{
	if(!isBuffering())
		mCloth.setCollisionSpheres(spheresBuffer, count);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setCollisionSpheres() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbCollisionSpheres() const
{
	if(!isBuffering())
		return mCloth.getNbCollisionSpheres();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbCollisionSpheres() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::getCollisionData( PxClothCollisionSphere* spheresBuffer, PxU32* capsulesBuffer, 
									   PxClothCollisionPlane* planesBuffer, PxU32* convexesBuffer, PxClothCollisionTriangle* trianglesBuffer ) const
{
	if(!isBuffering())
		mCloth.getCollisionData(spheresBuffer, capsulesBuffer, planesBuffer, convexesBuffer, trianglesBuffer);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getCollisionData() not allowed while simulation is running.");
}

PX_INLINE void Cloth::addCollisionCapsule(PxU32 first, PxU32 second)
{
	if(!isBuffering())
		mCloth.addCollisionCapsule(first, second);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::addCollisionCapsule() not allowed while simulation is running.");
}

PX_INLINE void Cloth::removeCollisionCapsule(PxU32 index)
{
	if(!isBuffering())
		mCloth.removeCollisionCapsule(index);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::removeCollisionCapsule() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbCollisionCapsules() const
{
	if(!isBuffering())
		return mCloth.getNbCollisionCapsules();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbCollisionCapsules() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::addCollisionTriangle(const PxClothCollisionTriangle& triangle)
{
	if(!isBuffering())
		mCloth.addCollisionTriangle(triangle);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::addCollisionTriangle() not allowed while simulation is running.");
}

PX_INLINE void Cloth::removeCollisionTriangle(PxU32 index)
{
	if(!isBuffering())
		mCloth.removeCollisionTriangle(index);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::removeCollisionTriangle() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setCollisionTriangles(const PxClothCollisionTriangle* trianglesBuffer, PxU32 count)
{
	if(!isBuffering())
		mCloth.setCollisionTriangles(trianglesBuffer, count);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setCollisionTriangles() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbCollisionTriangles() const
{
	if(!isBuffering())
		return mCloth.getNbCollisionTriangles();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbCollisionTriangles() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::addCollisionPlane(const PxClothCollisionPlane& plane)
{
	if(!isBuffering())
		mCloth.addCollisionPlane(plane);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::addCollisionPlane() not allowed while simulation is running.");
}

PX_INLINE void Cloth::removeCollisionPlane(PxU32 index)
{
	if(!isBuffering())
		mCloth.removeCollisionPlane(index);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::removeCollisionPlane() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setCollisionPlanes(const PxClothCollisionPlane* planesBuffer, PxU32 count)
{
	if(!isBuffering())
		mCloth.setCollisionPlanes(planesBuffer, count);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setCollisionPlanes() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbCollisionPlanes() const
{
	if(!isBuffering())
		return mCloth.getNbCollisionPlanes();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbCollisionPlanes() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::addCollisionConvex(PxU32 mask)
{
	if(!isBuffering())
		mCloth.addCollisionConvex(mask);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::addCollisionConvex() not allowed while simulation is running.");
}

PX_INLINE void Cloth::removeCollisionConvex(PxU32 index)
{
	if(!isBuffering())
		mCloth.removeCollisionConvex(index);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::removeCollisionConvex() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbCollisionConvexes() const
{
	if(!isBuffering())
		return mCloth.getNbCollisionConvexes();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbCollisionConvexes() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::setVirtualParticles(PxU32 numParticles, const PxU32* indices, PxU32 numWeights, const PxVec3* weights)
{
	if(!isBuffering())
		mCloth.setVirtualParticles(numParticles, indices, numWeights, weights);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setVirtualParticles() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbVirtualParticles() const
{
	if(!isBuffering())
		return mCloth.getNbVirtualParticles();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbVirtualParticles() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::getVirtualParticles(PxU32* indicesBuffer) const
{
	if(!isBuffering())
		mCloth.getVirtualParticles(indicesBuffer);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getVirtualParticles() not allowed while simulation is running.");
}

PX_INLINE PxU32 Cloth::getNbVirtualParticleWeights() const
{
	if(!isBuffering())
		return mCloth.getNbVirtualParticleWeights();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getNbVirtualParticleWeights() not allowed while simulation is running.");
		return 0;
	}
}

PX_INLINE void Cloth::getVirtualParticleWeights(PxVec3* weightsBuffer) const
{
	if(!isBuffering())
		mCloth.getVirtualParticleWeights(weightsBuffer);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getVirtualParticleWeights() not allowed while simulation is running.");
}

PX_INLINE PxTransform Cloth::getGlobalPose() const
{
	if(!isBuffering())
		return mCloth.getGlobalPose();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getGlobalPose() not allowed while simulation is running.");
		return PxTransform(PxIdentity);
	}
}

PX_INLINE void Cloth::setGlobalPose(const PxTransform& pose)
{
	if(!isBuffering())
		mCloth.setGlobalPose(pose);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setGlobalPose() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setTargetPose(const PxTransform& pose)
{
	if(!isBuffering())
		mCloth.setTargetPose(pose);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setTargetPose() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getExternalAcceleration() const
{
	if(!isBuffering())
		return mCloth.getExternalAcceleration();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getExternalAcceleration() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setExternalAcceleration(PxVec3 acceleration)
{
	if(!isBuffering())
		mCloth.setExternalAcceleration(acceleration);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setExternalAcceleration() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getLinearInertiaScale() const
{
	if(!isBuffering())
		return mCloth.getLinearInertiaScale();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getLinearInertiaScale() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setLinearInertiaScale(PxVec3 scale)
{
	if(!isBuffering())
		mCloth.setLinearInertiaScale(scale);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setLinearInertiaScale() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getAngularInertiaScale() const
{
	if(!isBuffering())
		return mCloth.getAngularInertiaScale();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getAngularInertiaScale() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setAngularInertiaScale(PxVec3 scale)
{
	if(!isBuffering())
		mCloth.setAngularInertiaScale(scale);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setAngularInertiaScale() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getCentrifugalInertiaScale() const
{
	if(!isBuffering())
		return mCloth.getCentrifugalInertiaScale();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getCentrifugalInertiaScale() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setCentrifugalInertiaScale(PxVec3 scale)
{
	if(!isBuffering())
		mCloth.setCentrifugalInertiaScale(scale);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setCentrifugalInertiaScale() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getDampingCoefficient() const
{
	if(!isBuffering())
		return mCloth.getDampingCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getDampingCoefficient() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setDampingCoefficient(PxVec3 dampingCoefficient)
{
	if(!isBuffering())
		mCloth.setDampingCoefficient(dampingCoefficient);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setDampingCoefficient() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getFrictionCoefficient() const
{
	if(!isBuffering())
		return mCloth.getFrictionCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getFrictionCoefficient() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setFrictionCoefficient(PxReal frictionCoefficient)
{
	if(!isBuffering())
		mCloth.setFrictionCoefficient(frictionCoefficient);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setFrictionCoefficient() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getLinearDragCoefficient() const
{
	if(!isBuffering())
		return mCloth.getLinearDragCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getLinearDragCoefficient() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setLinearDragCoefficient(PxVec3 dampingCoefficient)
{
	if(!isBuffering())
		mCloth.setLinearDragCoefficient(dampingCoefficient);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setLinearDragCoefficient() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getAngularDragCoefficient() const
{
	if(!isBuffering())
		return mCloth.getAngularDragCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getAngularDragCoefficient() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setAngularDragCoefficient(PxVec3 dampingCoefficient)
{
	if(!isBuffering())
		mCloth.setAngularDragCoefficient(dampingCoefficient);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setAngularDragCoefficient() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getCollisionMassScale() const
{
	if(!isBuffering())
		return mCloth.getCollisionMassScale();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getCollisionMassScale() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setCollisionMassScale(PxReal scalingCoefficient)
{
	if(!isBuffering())
		mCloth.setCollisionMassScale(scalingCoefficient);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setCollisionMassScale() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getSelfCollisionDistance() const
{
	if(!isBuffering())
		return mCloth.getSelfCollisionDistance();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSelfCollisionDistance() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setSelfCollisionDistance(PxReal distance)
{
	if(!isBuffering())
		mCloth.setSelfCollisionDistance(distance);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSelfCollisionDistance() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getSelfCollisionStiffness() const
{
	if(!isBuffering())
		return mCloth.getSelfCollisionStiffness();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSelfCollisionStiffness() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setSelfCollisionStiffness(PxReal stiffness)
{
	if(!isBuffering())
		mCloth.setSelfCollisionStiffness(stiffness);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSelfCollisionStiffness() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setSelfCollisionIndices(const PxU32* indices, PxU32 nbIndices)
{
	if(!isBuffering())
		mCloth.setSelfCollisionIndices(indices, nbIndices);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSelfCollisionIndices() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::getSelfCollisionIndices(PxU32* indices) const
{
	if(!isBuffering())
		return mCloth.getSelfCollisionIndices(indices);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSelfCollisionIndices() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxU32 Cloth::getNbSelfCollisionIndices() const
{
	return mCloth.getNbSelfCollisionIndices();
}

PX_INLINE void Cloth::setRestPositions(const PxVec4* restPositions)
{
	if(!isBuffering())
		mCloth.setRestPositions(restPositions);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setRestPositions() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::getRestPositions(PxVec4* restPositions) const
{
	if(!isBuffering())
		return mCloth.getRestPositions(restPositions);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getRestPositions() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxU32 Cloth::getNbRestPositions() const
{
	return mCloth.getNbRestPositions();
}

PX_INLINE PxReal Cloth::getSolverFrequency() const
{
	if(!isBuffering())
		return mCloth.getSolverFrequency();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSolverFrequency() not allowed while simulation is running.");
		return 60.0f;
	}
}

PX_INLINE void Cloth::setSolverFrequency(PxReal solverFreq)
{
	if(!isBuffering())
		mCloth.setSolverFrequency(solverFreq);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSolverFrequency() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getStiffnessFrequency() const
{
	if(!isBuffering())
		return mCloth.getStiffnessFrequency();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getStiffnessFrequency() not allowed while simulation is running.");
		return 60.0f;
	}
}

PX_INLINE void Cloth::setStiffnessFrequency(PxReal solverFreq)
{
	if(!isBuffering())
		mCloth.setStiffnessFrequency(solverFreq);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setStiffnessFrequency() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setStretchConfig(PxClothFabricPhaseType::Enum type, const PxClothStretchConfig& config)
{
	if(!isBuffering())
		mCloth.setStretchConfig(type, config);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setStretchConfig() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setTetherConfig(const PxClothTetherConfig& config)
{
	if(!isBuffering())
		mCloth.setTetherConfig(config);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setTetherConfig() not allowed while simulation is running.");
}

PX_INLINE PxClothStretchConfig Cloth::getStretchConfig(PxClothFabricPhaseType::Enum type) const
{
	if(!isBuffering())
		return mCloth.getStretchConfig(type);
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getStretchConfig() not allowed while simulation is running.");		
		return PxClothStretchConfig();
	}
}

PX_INLINE PxClothTetherConfig Cloth::getTetherConfig() const
{
	if(!isBuffering())
		return mCloth.getTetherConfig();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getTetherConfig() not allowed while simulation is running.");		
		return PxClothTetherConfig();
	}
}

PX_INLINE PxClothFlags Cloth::getClothFlags() const
{
	if(!isBuffering())
		return mCloth.getClothFlags();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getClothFlags() not allowed while simulation is running.");
		return PxClothFlags(0);
	}
}

PX_INLINE void Cloth::setClothFlags(PxClothFlags flags)
{
	if(!isBuffering())
		mCloth.setClothFlags(flags);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setClothFlag() not allowed while simulation is running.");
}

PX_INLINE PxVec3 Cloth::getWindVelocity() const
{
	if(!isBuffering())
		return mCloth.getWindVelocity();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getWindVelocity() not allowed while simulation is running.");
		return PxVec3(0.0f);
	}
}

PX_INLINE void Cloth::setWindVelocity(PxVec3 wind)
{
	if(!isBuffering())
		mCloth.setWindVelocity(wind);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setWindVelocity() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getDragCoefficient() const
{
	if(!isBuffering())
		return mCloth.getDragCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getDragCoefficient() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setDragCoefficient(PxReal value)
{
	if(!isBuffering())
		mCloth.setDragCoefficient(value);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setDragCoefficient() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getLiftCoefficient() const
{
	if(!isBuffering())
		return mCloth.getLiftCoefficient();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getLiftCoefficient() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setLiftCoefficient(PxReal value)
{
	if(!isBuffering())
		mCloth.setLiftCoefficient(value);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setLiftCoefficient() not allowed while simulation is running.");
}

PX_INLINE bool Cloth::isSleeping() const
{
	if(!isBuffering())
		return mCloth.isSleeping();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::isSleeping() not allowed while simulation is running.");
		return false;
	}
}

PX_INLINE PxReal Cloth::getSleepLinearVelocity() const
{
	if(!isBuffering())
		return mCloth.getSleepLinearVelocity();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSleepLinearVelocity() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setSleepLinearVelocity(PxReal threshold)
{
	if(!isBuffering())
		mCloth.setSleepLinearVelocity(threshold);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSleepLinearVelocity() not allowed while simulation is running.");
}

PX_INLINE void Cloth::setWakeCounter(PxReal wakeCounterValue)
{
	if(!isBuffering())
		mCloth.setWakeCounter(wakeCounterValue);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setWakeCounter() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getWakeCounter() const
{
	if(!isBuffering())
		return mCloth.getWakeCounter();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getWakeCounter() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::wakeUp()
{
	Scene* scene = getScbScene();
	PX_ASSERT(scene);  // only allowed for an object in a scene

	if(!isBuffering())
		mCloth.wakeUp(scene->getWakeCounterResetValue());
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::wakeUp() not allowed while simulation is running.");
}

PX_INLINE void Cloth::putToSleep()
{
	if(!isBuffering())
		mCloth.putToSleep();
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::putToSleep() not allowed while simulation is running.");
}

PX_INLINE void Cloth::getParticleData(NpClothParticleData& particleData)
{
	if(!isBuffering())
		return getScCloth().getParticleData(particleData);

	Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
		"Call to PxCloth::lockParticleData() not allowed while simulation is running.");

	particleData.particles = 0;
	particleData.previousParticles = 0;
}

PxReal Cloth::getPreviousTimeStep() const
{
	if(!isBuffering())
		return mCloth.getPreviousTimeStep();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getPreviousTimeStep() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE PxBounds3 Cloth::getWorldBounds() const
{
	if(!isBuffering())
		return mCloth.getWorldBounds();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getWorldBounds() not allowed while simulation is running.");
		return PxBounds3::empty();
	}
}

PX_INLINE void Cloth::setSimulationFilterData(const PxFilterData& data)
{
	if(!isBuffering())
		mCloth.setSimulationFilterData(data);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setSimulationFilterData() not allowed while simulation is running.");
}

PX_INLINE PxFilterData Cloth::getSimulationFilterData() const
{
	if(!isBuffering())
		return mCloth.getSimulationFilterData();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getSimulationFilterData() not allowed while simulation is running.");
		return PxFilterData();
	}
}

PX_INLINE void Cloth::setContactOffset(PxReal offset)
{
	if(!isBuffering())
		mCloth.setContactOffset(offset);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setContactOffset() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getContactOffset() const
{
	if(!isBuffering())
		return mCloth.getContactOffset();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getContactOffset() not allowed while simulation is running.");
		return 0.0f;
	}
}

PX_INLINE void Cloth::setRestOffset(PxReal offset)
{
	if(!isBuffering())
		mCloth.setRestOffset(offset);
	else
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::setRestOffset() not allowed while simulation is running.");
}

PX_INLINE PxReal Cloth::getRestOffset() const
{
	if(!isBuffering())
		return mCloth.getRestOffset();
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Call to PxCloth::getRestOffset() not allowed while simulation is running.");
		return 0.0f;
	}
}

}  // namespace Scb

}

#endif // PX_USE_CLOTH_API

#endif
