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



#ifndef EFFECT_PACKAGE_ACTOR_H
#define EFFECT_PACKAGE_ACTOR_H

#include "Actor.h"
#include "EmitterActor.h"

/*!
\file
\brief An EffectPackageActor represents an instantiation of a collection of related particle actors
The EffectPackage system allows for multiple related particle actors to be instantiated as a single
unit with a single root pose transform.  This can be one or more emitters as well as associated
field samplers, heat sources, etc.  This single instantiation can be subject to level of detail culling
and otherwise manipulated as a single unit.

EffectPackages are authored using the 'ParticleEffectTool' which allows the developer to rapidly
prototype a combined 'particle effect'.  This is a PhysX 3.x only feature.
*/

struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;

namespace physx
{
	class PxRigidDynamic;
}

namespace nvidia
{

namespace general_renderdebug4
{
///Forward reference the RenderDebugInterface class
class RenderDebugInterface;
};

namespace apex
{

/**
	This enum is used to identity what 'type' of effect
*/
enum EffectType
{
	ET_EMITTER,
	ET_HEAT_SOURCE,
	ET_SUBSTANCE_SOURCE,
	ET_TURBULENCE_FS,
	ET_JET_FS,
	ET_ATTRACTOR_FS,
	ET_FORCE_FIELD,
	ET_NOISE_FS,
	ET_VORTEX_FS,
	ET_WIND_FS,
	ET_RIGID_BODY,
	ET_VELOCITY_SOURCE,
	ET_FLAME_EMITTER,
	ET_LAST
};

class Actor;
class RenderVolume;

/**
\brief Defines the EffectPackageActor API which is instantiated from an EffectPackageAsset
*/
class EffectPackageActor : public Actor
{
public:

	/**
	\brief Fade out the EffectPackageActor over a period of time.
	This method will cause the actors within the EffectPackage to be 'faded out' over a period of time.
	The EffectPackageAsset will define how this fadeOut operation is to be applied.  Some actors may
	simply be turned off over this duration while others are modulated in some way.  For example, by default,
	emitter actors will have their emitter rate reduced over this duration so they do not appear to
	immediately cut off

	\param [in] fadetime : The duration to fade out the associated effects in seconds
	*/
	virtual void fadeOut(float fadetime) = 0;

	/**
	\brief Fade in the EffectPackageActor over a period of time.
	This method will cause the associated effects to be faded in over a period of time.
	It will begin the linear interpolation fade process relative to the current fade time.
	Meaning if you had previously specified a fadeOut duration of 1 second but then execute
	a fadeIn just a 1/2 second later, will will begin fading in from a 50% level; not zero.

	\param [in] fadetime : The duration to fade this effect in, in seconds
	*/
	virtual void fadeIn(float fadetime) = 0;

	/**
	\brief Returns the number of effects within this effect package.
	*/
	virtual uint32_t getEffectCount() const = 0;

	/**
	\brief Returns the type of effect at this index location

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	*/
	virtual EffectType getEffectType(uint32_t effectIndex) const = 0;

	/**
	\brief Returns the name of the effect at this index.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	*/
	virtual const char* getEffectName(uint32_t effectIndex) const = 0;

	/**
	\brief Returns true if this sub-effect is currently enabled.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	*/
	virtual bool isEffectEnabled(uint32_t effectIndex) const = 0;

	/**
	\brief Sets the enabled state of this sub-effect

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [in] state : Whether the effect should be enabled or not.
	*/
	virtual bool setEffectEnabled(uint32_t effectIndex, bool state) = 0;

	/**
	\brief Sets the over all effect scale, this is a uniform scaling factor.

	\param [in] scale : The uniform scale to apply to all effects in this effect package
	*/
	virtual void setCurrentScale(float scale) = 0;

	/**
	\brief Gets the current overall uniform effect scale
	*/
	virtual float getCurrentScale(void) const = 0;

	/**
	\brief Returns the pose of this sub-effect; returns as a bool the active state of this effect.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [pose] : Contains the pose requested
	\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
	*/
	virtual bool getEffectPose(uint32_t effectIndex, PxTransform& pose, bool worldSpace) = 0;

	/**
	\brief Sets the pose of this sub-effect; returns as a bool the active state of this effect.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount
	\param [pose] : Contains the pose to be set
	\param [worldSpace] : Whether to return the pose in world-space or in parent-relative space.
	*/
	virtual bool setEffectPose(uint32_t effectIndex, const PxTransform& pose, bool worldSpace) = 0;

	/**
	\brief Returns the base Actor pointer for this effect.
	Use this method if you need direct access to the base Actor pointer relative to this
	effect.  You must use 'getEffectType' to know what type of actor this is referring to before
	you cast it to either an emitter, or heat source, or turbulence actor, etc.
	It is important to note that the application cannot cache this pointer as it may be deleted in
	subsequent frames due to level of detail calculations or timing values such as fade out operations.

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount

	\return Returns the Actor pointer for this effect.  This may be NULL if that effect is not currently active.
	*/
	virtual Actor* getEffectActor(uint32_t effectIndex) const  = 0;

	/**
	\brief Returns the base PxRigidDynamic actor pointer for this effect.
	Use this method if you need direct access to the base PxRigidDynamic actor pointer relative to this
	effect.  

	\param [in] effectIndex : The effect number to refer to; must be less than the result of getEffectCount, must be a rigid dynamic effect

	\return Returns the PxRigidDynamic pointer for this effect.  This may be NULL if that effect is not currently active.
	*/
	virtual PxRigidDynamic* getEffectRigidDynamic(uint32_t effectIndex) const  = 0;

	/**
	\brief Forces the state of all emitters associated with this effect package to be on or off.
	This method allows the application to manually force all emitters to be active or inactive
	based on an explicit call.

	\param [state] : Determines whether to set all emitters on or off
	*/
	virtual void setEmitterState(bool state) = 0;

	/**
	\brief A helper method to return the active number of particles being simulated by all related effects in this EffectPackageActor

	\return Returns the number of particles active relative to this EffectPackageActor
	*/
	virtual uint32_t getActiveParticleCount() const = 0;

	/**
	\brief This helper method reports if any emitters within this EffectPackageActor are still actively emitting particles.

	This method allows the application to determine whether or not any emitters are actively running
	relative to this actor.  Typically this would be used to defer deleting an effect while it is still active.

	\return Returns true if any emitters in the actor are still active.
	*/
	virtual bool isStillEmitting() const = 0;

	/**
	Returns true if any portion of the effect is still considered 'alive' based on LOD and duration settings.
	*/
	virtual bool isAlive(void) const = 0;

	/**
	\brief Allows the application to manually enable this EffectPackageActor

	This method is used by the application to manually force the EffectPackageActor to be enabled.
	This is generally used based on some kind of game logic where the EffectPackageActor has been
	pre-allocated and placed into the world but is initially inactive until some event occurs.

	\param [state] Set the current enabled state to true or false
	*/
	virtual void setEnabled(bool state) = 0;

	/**
	\brief Returns the current enabled state for this actor
	*/
	virtual bool getEnabled() const = 0;

	/**
	\brief Sets the root pose for this EffectPackageActor
	It's important to note that child-relative poses for each sub-component in an EffectPackageActor
	are authored in the ParticleEffectTool.

	\param [pose] : Sets the current root pose for this EffectPackageActor
	*/
	virtual void setPose(const PxTransform& pose) = 0;

	/**
	\brief Returns the current root pose for this EffectPackageActor
	*/
	virtual const PxTransform& getPose() const = 0;

	/**
	\brief This method is used by the ParticleEffectTool to refresh a currently running effect with real-time property changes
	*/
	virtual void refresh() = 0;

	/**
	\brief Releases this EffectPackageActor and all associated children effects
	*/
	virtual void release() = 0;

	/**
	\brief A convenient helper method to return the name of the asset used to to create this actor.
	*/
	virtual const char* getName() const = 0;

	/**
	\brief returns the duration of the effect; accounting for maximum duration of each sub-effect.  A duration of 'zero' means it is an infinite lifetime effect.
	*/
	virtual float getDuration() const = 0;

	/**
	\brief Returns the current lifetime of the particle.
	*/
	virtual float getCurrentLife() const = 0;

	/**
	\brief Sets the preferred render volume for emitters in this effect package
	*/
	virtual void                 setPreferredRenderVolume(RenderVolume* volume) = 0;

	/**
	\brief Returns true if this effect package contains a turbulence effect that specifies a volume-render material; if so then the application may want to set up volume rendering
	*/
	virtual const char * hasVolumeRenderMaterial(uint32_t &index) const = 0;

	/**
	\brief Sets the emitter validation callback for any newly created emitter actors.
	*/
	virtual void setApexEmitterValidateCallback(EmitterActor::EmitterValidateCallback *callback) = 0;
};


}; // end of apex namespace
}; // end of physx namespace

#endif
