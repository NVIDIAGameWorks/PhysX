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



#ifndef INSTANCED_OBJECT_SIMULATION_INTL_H
#define INSTANCED_OBJECT_SIMULATION_INTL_H

#include "ApexDefs.h"

#include "PxTask.h"
#include "ApexActor.h"
#include "IofxManagerIntl.h"

namespace nvidia
{
namespace apex
{

class RenderVolume;

/* Input data structure from Emitter ==> (injector) IOS */
struct IosNewObject
{
	PxVec3  initialPosition;
	PxVec3  initialVelocity;
	float   lifetime;			// in seconds
	float   lodBenefit;			// filled in by injector
	IofxActorIDIntl  iofxActorID;		    // filled in by injector

	uint32_t   userData;
};

/**
 * Note the difference between how lifetimes are provided by the emitter (in seconds), and how
 * they are reported to the IOFX (as a percentage of time remaining).  This implies a couple IOS
 * requirements.  1) The IOS must remember the initial total lifetime and each frame do a remain/total
 * division in order to report the percent remain.  2) In order for the liferemain to report 1.0 on
 * the object's inital frame, new objects cannot have their remain decremented.
 */

/**
 * An emitter will aquire an instance of this class from each IOS actor it instantiates.  The IOS
 * will hold an array of these instances to manage all of its emitters and object ID ranges.
 */
class IosInjectorIntl : public ApexActor
{
public:
	/**
	 * An emitter calls createObjects() at the end of its tick/step function to register its newly
	 * spawned objects with the IOS.  If the IOS has limits on the number of objects it can spawn each
	 * simulation step, it must provide buffering beneath this API.  The IOS must copy this data if it
	 * cannot create the objects within this function call.  Note that the IOFX is unaware of the
	 * object creation path, it discovers spawned objects when they show up with liferemain of 1.0
	 * An IOS may have built-in emitters that do not call this API  (Turbulence).
	 */
	virtual void createObjects(uint32_t count, const IosNewObject* createList) = 0;

	/**
	 * An emitter calls setLODWeights() as often as it needs to adjust the LOD paramters for its
	 * particles.
	 */
	// distanceWeight minimum squared distance from camera before distance is included in LOD weight
	// speedWeight minimum velocity parameter.  Particles slower than this are culled more aggressively.
	// lifeWeight lifetime minimum limit.  Particles with less lifetime than this remaining will be culled.
	virtual void setLODWeights(float maxDistance, float distanceWeight, float speedWeight, float lifeWeight, float separationWeight, float bias) = 0;

	/**
	 * When an emitter is being destroyed, it must call this release method on all of its injectors
	 * so those IOS instances can reclaim those ID ranges and destroy any active objects.
	 */
	virtual void release() = 0;

	virtual PxTaskID getCompletionTaskID() const = 0;

	virtual void setPreferredRenderVolume(nvidia::apex::RenderVolume* volume) = 0;

	/**
	 * Return the value of the least benefit particle to survive last frame's LOD culling.
	 * An emitter can query this value to voluntarily throttle itself.  However, to prevent
	 * feedback loops it should always try to emit at least a few particles when it is
	 * throttled.
	 */
	virtual float getLeastBenefitValue() const = 0;

	virtual uint32_t getSimParticlesCount() const = 0;

	/**
	 * This injector has particles in it that were unable to be inserted at the last simulation
	 * step because of an insertion limit in the IOS.  The emitter may chose to throttle its
	 * emissions when this returns true.
	 */
	virtual bool isBacklogged() const = 0;

	/**
	Returns the current number of particles/objects active in the simulation.
	 */
	virtual uint32_t getActivePaticleCount() const = 0;

	virtual void setObjectScale(float objectScale) = 0;

protected:
	virtual ~IosInjectorIntl() {}
};


/**
 * Base class for all particle simulation systems and other systems that can efficiently simulate
 * instanced geometry.  This is the interface to the IOS Actor (instance).
 */
class InstancedObjectSimulationIntl : public ApexActor
{
public:
	/**
	 * An emitter calls allocateInjector() to create an injector targeted at a particular
	 * IOFX Asset.  The IOS will allocate an IOFX actor as necessary.  The emitter has no
	 * knowledge of the size of the IOFX actor, or how many emitters are also using it.
	 */
	virtual IosInjectorIntl* allocateInjector(IofxAsset* iofxAsset) = 0;

	/**
	 * Query the authored radius of the instanced objects simulated by this IOS.  Emitters need this
	 * value for volume fill effects and an IOFX may need it for rendering purposes.
	 */
	virtual float getObjectRadius() const = 0;

	/**
	 * Query the authored density of the instanced objects simulated by this IOS.  Emitters need this
	 * value for constant density emitter effects.
	 */
	virtual float getObjectDensity() const = 0;

	/**
	 * An emitter may use this API functions to query particle positions from the most recent simulation step
	 * This IOS output buffer is updated each frame during fetchResults.
	 */
	virtual const PxVec3* getRecentPositions(uint32_t& count, uint32_t& stride) const = 0;

	/**
	* Set's the origin of the density grid; this is implemented for BasicIOS and ParticleIOS
	*/
	virtual void	setDensityOrigin(const PxVec3& v) 
	{
		PX_UNUSED(v);
	}

protected:
	virtual ~InstancedObjectSimulationIntl() {}
};

}
} // end namespace nvidia::apex

#endif // #ifndef INSTANCED_OBJECT_SIMULATION_INTL_H
