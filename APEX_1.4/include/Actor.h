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



#ifndef ACTOR_H
#define ACTOR_H

/*!
\file
\brief classes ApexActor, ApexActorSource
*/

#include "ApexInterface.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxActor.h"
#include "PxShape.h"
#include "PxFiltering.h"
namespace nvidia { namespace apex
{

/** Corresponds to 20 frames for a time step of 0.02, PhysX 3.3 took PX_SLEEP_INTERVAL away */
#define APEX_DEFAULT_WAKE_UP_COUNTER 0.4f

/** Used to define generic get/set methods for 1-parameter values in PhysX3DescTemplate. */
#define APEX_ACTOR_TEMPLATE_PARAM(_type, _name, _valid, _default) \
bool is##_name##Valid(_type x) const { PX_UNUSED(x); return _valid; } \
_type getDefault##_name() const { return _default; } \
virtual _type get##_name() const = 0; \
virtual bool set##_name(_type) = 0

/**
	Template for PhysX3.x actor, body and shape.  Used by the Destruction module when creating PhysX objects.

	See ApexActorSource::setPhysX3Template and ActorSource::getPhysX3Template.
*/
class PhysX3DescTemplate
{
public:
	/*
		For each Name below, there are functions: getName(), setName(), isNameValid(), and getDefaultName().  For example:

		PxDominanceGroup	getDominanceGroup() const;
		bool				setDominanceGroup(PxDominanceGroup);	// Returns true iff the passed-in value is valid.  (Note, will set the internal values even if they are not valid.)
		bool				isDominanceGroupValid() const;
		PxDominanceGroup	getDefaultDominanceGroup() const;
	*/

	/*							 Type						Name					Validity Condition		Default Value */
	// Actor
	/** getX(), setX(), getDefaultX(), and isXValid() for X=DominanceGroup (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxDominanceGroup,	DominanceGroup,		(1),					0);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ActorFlags (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				ActorFlags,				(1),					physx::PxActorFlag::eSEND_SLEEP_NOTIFIES);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=OwnerClient (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxClientID,	OwnerClient,			(1),					0);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ClientBehaviorBits (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				ClientBehaviorBits,		(1),					0);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ContactReportFlags (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint16_t,				ContactReportFlags,		(1),					0);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=UserData (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(void*,				UserData,				(1),					NULL);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=Name (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(const char*,			Name,					(1),					NULL);

	// Body
	/** getX(), setX(), getDefaultX(), and isXValid() for X=Density (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				Density,				(x >= 0.0f),			1.0f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=BodyFlags (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				BodyFlags,				(1),					0);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=WakeUpCounter (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				WakeUpCounter,			(x >= 0.0f),			APEX_DEFAULT_WAKE_UP_COUNTER);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=LinearDamping (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				LinearDamping,			(x >= 0.0f),			0.0f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=AngularDamping (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				AngularDamping,			(x >= 0.0f),			0.05f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=MaxAngularVelocity (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				MaxAngularVelocity,		(1),					7.0f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=SleepLinearVelocity (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				SleepLinearVelocity,	(1),					0.0f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=SolverIterationCount (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				SolverIterationCount,	(x >= 1 && x <= 255),	4);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=VelocityIterationCount (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				VelocityIterationCount,	(x >= 1 && x <= 255),	1);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ContactReportThreshold (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				ContactReportThreshold,	(1),					PX_MAX_F32);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=SleepThreshold (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				SleepThreshold,			(1),					0.005f);

	// Shape
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ShapeFlags (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				ShapeFlags,				(1),					physx::PxShapeFlag::eSIMULATION_SHAPE | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eVISUALIZATION);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ShapeUserData (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(void*,				ShapeUserData,			(1),					NULL);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ShapeName (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(const char*,			ShapeName,				(1),					NULL);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=SimulationFilterData (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxFilterData,	SimulationFilterData,	(1),					physx::PxFilterData(0, 0, 0, 0));
	/** getX(), setX(), getDefaultX(), and isXValid() for X=QueryFilterData (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxFilterData,	QueryFilterData,		(1),					physx::PxFilterData(0, 0, 0, 0));
	/** getX(), setX(), getDefaultX(), and isXValid() for X=ContactOffset (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				ContactOffset,			(1),					-1.0f);
	/** getX(), setX(), getDefaultX(), and isXValid() for X=RestOffset (see APEX_ACTOR_TEMPLATE_PARAM) */
	APEX_ACTOR_TEMPLATE_PARAM(float,				RestOffset,				(1),					PX_MAX_F32);
	// Shape materials get explicitly defined API:
	/** getMaterials function is non-generic, as it returns two parameters (the materials array and the array size in materialCount) */
	virtual physx::PxMaterial**	getMaterials(uint32_t& materialCount) const = 0;
	/** setMaterials function is non-generic, as it sets two parameters (the materials array in materialArray and the array size in materialCount) */
	virtual bool				setMaterials(physx::PxMaterial** materialArray, uint32_t materialCount) = 0;	// Must have non-zero sized array of materials to be valid.


	/** Use this method to release this object */
	virtual void release() = 0;

protected:
	virtual ~PhysX3DescTemplate() {}
};	// PhysX3DescTemplate

#undef APEX_ACTOR_TEMPLATE_PARAM

}}
#endif

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class Asset;

/**
\brief Base class for APEX module objects.
*/
class Actor : public ApexInterface
{
public:
	/**
	\brief Returns the owning asset
	*/
	virtual Asset* getOwner() const = 0;

	/**
	\brief Returns the range of possible values for physical Lod overwrite

	\param [out] min		The minimum lod value
	\param [out] max		The maximum lod value
	\param [out] intOnly	Only integers are allowed if this is true, gets rounded to nearest

	\note The max value can change with different graphical Lods
	\see Actor::forceLod()
	*/
	virtual void getLodRange(float& min, float& max, bool& intOnly) const = 0;

	/**
	\brief Get current physical lod.
	*/
	virtual float getActiveLod() const = 0;

	/**
	\brief Force an APEX Actor to use a certian physical Lod

	\param [in] lod	Overwrite the Lod system to use this Lod.

	\note Setting the lod value to a negative number will turn off the overwrite and proceed with regular Lod computations
	\see Actor::getLodRange()
	*/
	virtual void forceLod(float lod) = 0;

	/**
	\brief Ensure that all module-cached data is cached.
	*/
	virtual void cacheModuleData() const {}

	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state) = 0;
	

protected:
	virtual ~Actor() {} // use release() method instead!
};

#if PX_PHYSICS_VERSION_MAJOR == 3
/**
\brief Base class for APEX classes that spawn PhysX SDK Actors
*/
class ActorSource
{
public:
	/**
	\brief Sets the current body template

	User can specify a descriptor template for bodies that this object may create.  APEX may customize these suggested settings.
	Already created / existing bodies will not be changed if the body template is changed!  The body template will only be used for
	new bodies created after this is called!

	members that are ignored:
	massLocalPose
	massSpaceInertia
	mass
	linearVelocity
	angularVelocity

	These fields should be left at their default values as set by the desc constructor.
	*/
	virtual void setPhysX3Template(const PhysX3DescTemplate*) = 0;

	/**
	\brief Retrieve the current body template
	*/
	virtual bool getPhysX3Template(PhysX3DescTemplate& dest) const = 0;

	/**
	\brief Create an PhysX3DescTemplate object to pass into the get/set methods.
	*/
	virtual PhysX3DescTemplate* createPhysX3DescTemplate() const = 0;
};

#endif

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // ACTOR_H
