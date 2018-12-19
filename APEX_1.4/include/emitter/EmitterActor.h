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



#ifndef EMITTER_ACTOR_H
#define EMITTER_ACTOR_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class EmitterAsset;
class EmitterGeomExplicit;
class EmitterLodParamDesc;
class RenderVolume;

/// Apex emitter actor class. Emits particles within a given shape.
class EmitterActor : public Actor
{
protected:
	virtual ~EmitterActor() {}

public:

	/// This is an optional user validation callback interface.
	/// If the application wants to confirm/verify all emitted particles they can provide
	/// this callback interface by using the setApexEmitterValidateCallback method.
	class EmitterValidateCallback
	{
	public:
		/**
		\brief This application callback is used to verify an emitted particle position.
		
		If the user returns false, then the particle will not be emitted.  If the user returns true
		if will be emitted but using the position value which is passed by reference.  The application
		can choose to leave the emitter position alone, or modify it to a new location.  All locations
		are in world space.  For convenience to the application the world space emitter position is provided.
		*/
		virtual bool validateEmitterPosition(const PxVec3 &emitterOrigin,PxVec3 &position) = 0;
	};

	/// Returns the asset the instance has been created from.
	virtual EmitterAsset* 			getEmitterAsset() const = 0;

	/// Returns the explicit geometry for THIS ACTOR only
	virtual EmitterGeomExplicit* 	isExplicitGeom() = 0;

	/// Gets the global pose
	virtual PxMat44	     			getGlobalPose() const = 0;
	
	/// Sets the curent pose of the emitter
	virtual void				 	setCurrentPose(const PxTransform& pose) = 0;
	
	/// Sets the curent position of the emitter
	virtual void				 	setCurrentPosition(const PxVec3& pos) = 0;

	/**
	\brief PhysX SDK 3.X.  Attaches the emitter to an actor
	
	PxActor pointer can be NULL to detach existing actor
	*/
	virtual void				 	setAttachActor(PxActor*) = 0;

	/// sets the relative pose of the emitter in the space of the actor to which it is attached
	virtual void				 	setAttachRelativePose(const PxTransform& pose) = 0;

	/// PhysX SDK 3.X. Retrieves the actor, to which the emitter is attached. NULL is returned for an unattached emitter.
	virtual const PxActor* 		 	getAttachActor() const = 0;

	/// Retrieves the relative pose of the emitter in the space of the actor to which it is attached
	virtual const PxMat44 			getAttachRelativePose() const = 0;
	
	///	Retrieves the particle radius
	virtual float        			getObjectRadius() const = 0;

	/// Sets collision groups used to reject particles that overlap the geometry
	virtual void					setOverlapTestCollisionGroups(uint32_t) = 0;

	/**	
	 \brief start emitting particles
	 
	 If persistent is true, the emitter will emit every frame until stopEmit() is
	 called.
	 */
	virtual void                 	startEmit(bool persistent = true) = 0;
	
	/// Stop emitting particles
	virtual void                 	stopEmit() = 0;
	
	/// Returns true if the emitter is emitting particles
	virtual bool                 	isEmitting() const = 0;
	
	///	Gets LOD settings
	virtual const EmitterLodParamDesc& getLodParamDesc() const = 0;
	///	Sets LOD settings
	virtual void				 	setLodParamDesc(const EmitterLodParamDesc&) = 0;

	/** Override authored scalable parameters, if necessary */
	/// Sets the range from which the density of particles within the volume is randomly chosen
	virtual void                 	setDensity(const float&) = 0;
	
	/// Sets the range from which the emission rate is randomly chosen
	virtual void                 	setRate(const float&) = 0;

	/// Sets the range from which the velocity of a particle is randomly chosen
	virtual void                 	setVelocityLow(const PxVec3&) = 0;
	
	/// Sets the range from which the velocity of a particle is randomly chosen
	virtual void                 	setVelocityHigh(const PxVec3&) = 0;
	
	/// Sets the range from which the lifetime of a particle is randomly chosen
	virtual void                 	setLifetimeLow(const float&) = 0;
	
	/// Sets the range from which the lifetime of a particle is randomly chosen
	virtual void                 	setLifetimeHigh(const float&) = 0;

	/// Sets whether or not authored asset particles are emitted
	virtual void				 	emitAssetParticles(bool enable) = 0;
	
	/// Gets whether or not authored asset particles are emitted
	virtual bool				 	getEmitAssetParticles() const = 0;

	/**
	 \brief Emitted particles are injected to specified render volume on initial frame.
	
	 This will work only if you have one renderVolume for each emitter.
	 Set to NULL to clear the preferred volume.
	 */
	virtual void                 	setPreferredRenderVolume(RenderVolume* volume) = 0;

	/// Gets the range from which the emission rate is randomly chosen
	virtual void                 	getRate(float&) const = 0;

	/// Returns the number of particles in simulation
	virtual uint32_t				getSimParticlesCount() const = 0;

	/// Returns the number of particles still alive
	virtual uint32_t				getActiveParticleCount() const = 0;

	/// Sets the origin of the density grid used by this emitter.  Important, this density grid may be shared with lots of other emitters as well, it is based on the underlying IOS
	virtual void					setDensityGridPosition(const PxVec3 &pos) = 0;

	/// Sets the ApexEmitterPosition validation callback interface
	virtual void 					setApexEmitterValidateCallback(EmitterValidateCallback *callback) = 0;

	/// Sets the uniform overall object scale
	PX_DEPRECATED virtual void		setObjectScale(float scale) = 0;

	/// Retrieves the uniform overall object scale
	PX_DEPRECATED virtual float		getObjectScale(void) const = 0;

	/// Sets the uniform overall object scale
	virtual void					setCurrentScale(float scale) = 0;

	/// Retrieves the uniform overall object scale
	virtual float					getCurrentScale(void) const = 0;


};

PX_POP_PACK

}
} // end namespace nvidia

#endif // EMITTER_ACTOR_H
